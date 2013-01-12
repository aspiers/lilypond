/*
  This file is part of LilyPond, the GNU music typesetter.

  Copyright (C) 1997--2012 Jan Nieuwenhuizen <janneke@gnu.org>

  LilyPond is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  LilyPond is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with LilyPond.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "midi-item.hh"

#include "duration.hh"
#include "international.hh"
#include "main.hh"
#include "midi-stream.hh"
#include "misc.hh"
#include "program-option.hh"
#include "string-convert.hh"
#include "warn.hh"
#include <iostream>
#include <cstdio>

#define PITCH_WHEEL_CENTER 0x2000
#define PITCH_WHEEL_SEMITONE 0X1000

Midi_item *
Midi_item::get_midi (Audio_item *a)
{
  if (Audio_key *i = dynamic_cast<Audio_key *> (a))
    return new Midi_key (i);
  else if (Audio_instrument *i = dynamic_cast<Audio_instrument *> (a))
    return i->str_.length () ? new Midi_instrument (i) : 0;
  else if (Audio_note *i = dynamic_cast<Audio_note *> (a))
    return new Midi_note (i);
  else if (Audio_dynamic *i = dynamic_cast<Audio_dynamic *> (a))
    return new Midi_dynamic (i);
  else if (Audio_piano_pedal *i = dynamic_cast<Audio_piano_pedal *> (a))
    return new Midi_piano_pedal (i);
  else if (Audio_tempo *i = dynamic_cast<Audio_tempo *> (a))
    return new Midi_tempo (i);
  else if (Audio_time_signature *i = dynamic_cast<Audio_time_signature *> (a))
    return new Midi_time_signature (i);
  else if (Audio_text *i = dynamic_cast<Audio_text *> (a))
    return new Midi_text (i);
  else
    assert (0);

  return 0;
}

Midi_duration::Midi_duration (Real seconds_f)
{
  seconds_ = seconds_f;
}

string
Midi_duration::to_string () const
{
  return string ("<duration: ") + ::to_string (seconds_) + ">";
}

Midi_instrument::Midi_instrument (Audio_instrument *a)
  : Midi_channel_item (a),
    audio_ (a)
{
  audio_->str_ = String_convert::to_lower (audio_->str_);
}

string
Midi_instrument::to_string () const
{
  Byte program_byte = 0;
  bool found = false;

  SCM proc = ly_lily_module_constant ("midi-program");
  SCM program = scm_call_1 (proc, ly_symbol2scm (audio_->str_.c_str ()));
  found = (program != SCM_BOOL_F);
  if (found)
    program_byte = (Byte) scm_to_int (program);
  else
    warning (_f ("no such MIDI instrument: `%s'", audio_->str_.c_str ()));

  string str = ::to_string ((char) (0xc0 + channel_)); //YIKES! FIXME : Should be track. -rz
  str += ::to_string ((char)program_byte);
  return str;
}

Midi_item::Midi_item ()
{
}

Midi_channel_item::Midi_channel_item (Audio_item *ai)
  : channel_ (ai->channel_)
{
}

Midi_item::~Midi_item ()
{
}

string
int2midi_varint_string (int i)
{
  int buffer = i & 0x7f;
  while ((i >>= 7) > 0)
    {
      buffer <<= 8;
      buffer |= 0x80;
      buffer += (i & 0x7f);
    }

  string str;
  while (1)
    {
      str += ::to_string ((char)buffer);
      if (buffer & 0x80)
        buffer >>= 8;
      else
        break;
    }
  return str;
}

Midi_key::Midi_key (Audio_key *a)
  : audio_ (a)
{
}

string
Midi_key::to_string () const
{
  string str = "ff5902";
  str += String_convert::int2hex (audio_->accidentals_, 2, '0');
  if (audio_->major_)
    str += String_convert::int2hex (0, 2, '0');
  else
    str += String_convert::int2hex (1, 2, '0');
  return String_convert::hex2bin (str);
}

Midi_time_signature::Midi_time_signature (Audio_time_signature *a)
  : audio_ (a),
    clocks_per_1_ (18)
{
}

string
Midi_time_signature::to_string () const
{
  int num = abs (audio_->beats_);
  if (num > 255)
    {
      warning (_ ("Time signature with more than 255 beats.  Truncating"));
      num = 255;
    }

  int den = audio_->one_beat_;

  string str = "ff5804";
  str += String_convert::int2hex (num, 2, '0');
  str += String_convert::int2hex (intlog2 (den), 2, '0');
  str += String_convert::int2hex (clocks_per_1_, 2, '0');
  str += String_convert::int2hex (8, 2, '0');
  return String_convert::hex2bin (str);
}

Midi_note::Midi_note (Audio_note *a)
  : Midi_channel_item (a),
    audio_ (a),
    dynamic_byte_ (a->dynamic_ && a->dynamic_->volume_ >= 0
                   ? Byte (a->dynamic_->volume_ * 0x7f) : Byte (0x5a))
{
}

int
Midi_note::get_fine_tuning () const
{
  Rational tune = (audio_->pitch_.tone_pitch ()
                   + audio_->transposing_.tone_pitch ()) * Rational (2);
  tune -= Rational (get_semitone_pitch ());

  tune *= PITCH_WHEEL_SEMITONE;
  return (int) double (tune);
}

int
Midi_note::get_semitone_pitch () const
{
  double tune = double ((audio_->pitch_.tone_pitch ()
                         + audio_->transposing_.tone_pitch ()) * Rational (2));
  return int (rint (tune));
}

string
Midi_note::to_string () const
{
  Byte status_byte = (char) (0x90 + channel_);
  string str = "";
  int finetune;

  // print warning if fine tuning was needed, HJJ
  if (get_fine_tuning () != 0)
    {
      finetune = PITCH_WHEEL_CENTER + get_fine_tuning ();

      str += ::to_string ((char) (0xE0 + channel_));
      str += ::to_string ((char) (finetune & 0x7F));
      str += ::to_string ((char) (finetune >> 7));
      str += ::to_string ((char) (0x00));
    }

  str += ::to_string ((char) status_byte);
  str += ::to_string ((char) (get_semitone_pitch () + c0_pitch_));
  str += ::to_string ((char) dynamic_byte_);

  return str;
}

void
Midi_note::print () const
{
  string str = "";
  string space = " ";
  string::iterator it;

  string orig = to_string();

  for ( it = orig.begin() ; it < orig.end() - 1; it++ )
    {
      printf("%hhu ", *it);
    }
}

Midi_note_off::Midi_note_off (Midi_note *n)
  : Midi_note (n->audio_)
{
  on_ = n;
  channel_ = n->channel_;

  // use note_on with velocity=0 instead of note_off
  aftertouch_byte_ = 0;
}

string
Midi_note_off::to_string () const
{
  Byte status_byte = (char) (0x90 + channel_);

  string str = ::to_string ((char)status_byte);
  str += ::to_string ((char) (get_semitone_pitch () + Midi_note::c0_pitch_));
  str += ::to_string ((char)aftertouch_byte_);

  if (get_fine_tuning () != 0)
    {
      // Move pitch wheel back to the central position.
      str += ::to_string ((char) 0x00);
      str += ::to_string ((char) (0xE0 + channel_));
      str += ::to_string ((char) (PITCH_WHEEL_CENTER & 0x7F));
      str += ::to_string ((char) (PITCH_WHEEL_CENTER >> 7));
    }

  return str;
}

Midi_dynamic::Midi_dynamic (Audio_dynamic *a)
  : Midi_channel_item (a),
    audio_ (a)
{
}

string
Midi_dynamic::to_string () const
{
  Byte status_byte = (char) (0xB0 + channel_);
  string str = ::to_string ((char)status_byte);

  /*
    Main volume controller (per channel):
    07 MSB
    27 LSB
  */
  static Real const full_scale = 127;

  int volume = (int) (audio_->volume_ * full_scale);
  if (volume <= 0)
    volume = 1;
  if (volume > full_scale)
    volume = (int)full_scale;

  int const volume_default = 100;
  if (audio_->volume_ < 0 || audio_->silent_)
    volume = volume_default;

  str += ::to_string ((char)0x07);
  str += ::to_string ((char)volume);
  return str;
}

Midi_piano_pedal::Midi_piano_pedal (Audio_piano_pedal *a)
  : Midi_channel_item (a),
    audio_ (a)
{
}

string
Midi_piano_pedal::to_string () const
{
  Byte status_byte = (char) (0xB0 + channel_);
  string str = ::to_string ((char)status_byte);

  if (audio_->type_string_ == "Sostenuto")
    str += ::to_string ((char)0x42);
  else if (audio_->type_string_ == "Sustain")
    str += ::to_string ((char)0x40);
  else if (audio_->type_string_ == "UnaCorda")
    str += ::to_string ((char)0x43);

  int pedal = ((1 - audio_->dir_) / 2) * 0x7f;
  str += ::to_string ((char)pedal);
  return str;
}

Midi_tempo::Midi_tempo (Audio_tempo *a)
  : audio_ (a)
{
}

string
Midi_tempo::to_string () const
{
  int useconds_per_4 = 60 * (int)1e6 / audio_->per_minute_4_;
  string str = "ff5103";
  str += String_convert::int2hex (useconds_per_4, 6, '0');
  return String_convert::hex2bin (str);
}

Midi_text::Midi_text (Audio_text *a)
  : audio_ (a)
{
}

string
Midi_text::to_string () const
{
  string str = "ff" + String_convert::int2hex (audio_->type_, 2, '0');
  str = String_convert::hex2bin (str);
  str += int2midi_varint_string (audio_->text_string_.length ());
  str += audio_->text_string_;
  return str;
}

char const *
Midi_item::name () const
{
  return this->class_name ();
}
