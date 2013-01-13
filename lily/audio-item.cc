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

#include "audio-item.hh"

#include "midi-item.hh"
#include "audio-column.hh"

Audio_instrument::Audio_instrument (string instrument_string)
{
  str_ = instrument_string;
}

void
Audio_item::render ()
{
}

Audio_column *
Audio_item::get_column () const
{
  return audio_column_;
}

Audio_item::Audio_item ()
  : audio_column_ (0),
    channel_ (0)
{
}

Audio_note::Audio_note (Pitch p, Moment m, bool tie_event, Pitch transposing)
  : pitch_ (p),
    length_mom_ (m),
    transposing_ (transposing),
    dynamic_ (0),
    tied_ (0),
    tie_event_ (tie_event)
{
}

void
Audio_note::tie_to (Audio_note *t, Moment skip)
{
  tied_ = t;
  Audio_note *first = tie_head();
  // Add the skip to the tied note and the length of the appended note
  // to the full duration of the tie...
  first->length_mom_ += skip + length_mom_;
  length_mom_ = 0;
}

Audio_note *
Audio_note::tie_head ()
{
  Audio_note *first = this;
  while (first->tied_)
    first = first->tied_;
  return first;
}

string
Audio_note::to_string () const
{
  string s = "#<Audio_note pitch ";
  s += pitch_.to_string();
  s += " len ";
  s += length_mom_.to_string();
  if (tied_)
    {
      s += " tied to " + tied_->to_string();
    }
  if (tie_event_)
    {
      s += " tie_event";
    }
  s += ">";
  return s;
}

Audio_key::Audio_key (int acc, bool major)
{
  accidentals_ = acc;
  major_ = major;
}

Audio_dynamic::Audio_dynamic ()
  : volume_ (-1),
    silent_ (false)
{
}

Audio_span_dynamic::Audio_span_dynamic (Real min_volume, Real max_volume)
{
  grow_dir_ = CENTER;
  min_volume_ = min_volume;
  max_volume_ = max_volume;
}

void
Audio_span_dynamic::add_absolute (Audio_dynamic *d)
{
  assert (d);
  dynamics_.push_back (d);
}

Moment
remap_grace_duration (Moment m)
{
  return Moment (m.main_part_ + Rational (9, 40) * m.grace_part_,
                 Rational (0));
}

Real
moment_to_real (Moment m)
{
  return remap_grace_duration (m).main_part_.to_double ();
}

int
moment_to_ticks (Moment m)
{
  return int (moment_to_real (m) * 384 * 4);
}

void
Audio_span_dynamic::render ()
{
  if (dynamics_.size () <= 1)
    return;

  assert (dynamics_[0]->volume_ >= 0);

  while (dynamics_.back ()->volume_ > 0
         && dynamics_.size () > 1
         && sign (dynamics_.back ()->volume_ - dynamics_[0]->volume_) != grow_dir_)
    {
      dynamics_.erase (dynamics_.end () - 1);
    }

  if (dynamics_.size () <= 1)
    {
      programming_error ("Impossible or ambiguous (de)crescendo in MIDI.");
      return;
    }

  Real start_v = dynamics_[0]->volume_;
  if (dynamics_.back ()->volume_ < 0)
    {
      // The dynamic spanner does not end with an explicit dynamic script
      // event.  Adjust the end volume by at most 1/4 of the available
      // volume range in this case.
      dynamics_.back ()->volume_ = max (min (start_v + grow_dir_ * (max_volume_ - min_volume_) * 0.25, max_volume_), min_volume_);
    }

  Real delta_v = dynamics_.back ()->volume_ - dynamics_[0]->volume_;

  Moment start = dynamics_[0]->get_column ()->when ();

  Real total_t = moment_to_real (dynamics_.back ()->get_column ()->when () - start);

  for (vsize i = 1; i < dynamics_.size (); i++)
    {
      Moment dt_moment = dynamics_[i]->get_column ()->when ()
                         - start;

      Real dt = moment_to_real (dt_moment);

      Real v = start_v + delta_v * (dt / total_t);

      dynamics_[i]->volume_ = v;
    }
}

Audio_tempo::Audio_tempo (int per_minute_4)
{
  per_minute_4_ = per_minute_4;
}

Audio_time_signature::Audio_time_signature (int beats, int one_beat)
{
  beats_ = beats;
  one_beat_ = one_beat;
}

Audio_text::Audio_text (Audio_text::Type type, string text_string)
{
  text_string_ = text_string;
  type_ = type;
}

