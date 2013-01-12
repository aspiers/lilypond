/*
  This file is part of LilyPond, the GNU music typesetter.

  Copyright (C) 1998--2012 Han-Wen Nienhuys <hanwen@xs4all.nl>

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

#include "performer.hh"

#include "audio-item.hh"
#include "context.hh"
#include "stream-event.hh"
#include "translator.icc"
#include <list>
#include <iostream>

struct Head_audio_event_tuple
{
  Audio_element_info head_;
  // The end moment of the note, so we can calculate a skip and check whether
  // the note still goes on
  Moment end_moment_;
  Head_audio_event_tuple () {}
  Head_audio_event_tuple (Audio_element_info h, Moment m)
  {
    head_ = h;
    end_moment_ = m;
  }
};

class Tie_performer : public Performer
{
  Stream_event *event_;
  list<Head_audio_event_tuple> now_heads_;
  list<Head_audio_event_tuple> now_tied_heads_; // new tied notes
  list<Head_audio_event_tuple> heads_to_tie_; // heads waiting for closing tie

protected:
  void stop_translation_timestep ();
  void start_translation_timestep ();
  virtual void acknowledge_audio_element (Audio_element_info);
  void process_music ();
  DECLARE_TRANSLATOR_LISTENER (tie);
public:
  TRANSLATOR_DECLARATIONS (Tie_performer);
};

Tie_performer::Tie_performer ()
{
  event_ = 0;
}

IMPLEMENT_TRANSLATOR_LISTENER (Tie_performer, tie);
void
Tie_performer::listen_tie (Stream_event *ev)
{
  event_ = ev;
}

void
Tie_performer::process_music ()
{
  if (event_)
    context ()->set_property ("tieMelismaBusy", SCM_BOOL_T);
}

void
Tie_performer::acknowledge_audio_element (Audio_element_info inf)
{
  if (Audio_note *an = dynamic_cast<Audio_note *> (inf.elem_))
    {
      cout << "acknowledge_audio_element(" << an->pitch_.to_string() <<
        ")" << endl;

      // for each tied note, store the info and its end moment, so we can
      // later on check whether (1) the note is still ongoing and (2) how
      // long the skip is with tieWaitForNote
      Head_audio_event_tuple inf_mom (inf, now_mom () + an->length_mom_);
      if (an->tie_event_)
        now_tied_heads_.push_back (inf_mom);
      else
        now_heads_.push_back (inf_mom);

      // Find a previous note that ties to the current note. If it exists,
      // remove it from the heads_to_tie vector and create the tie
      list<Head_audio_event_tuple>::iterator it;
      bool found = false;
      Stream_event *right_mus = inf.event_;
      cout << "  heads_to_tie_.size() == " << heads_to_tie_.size() << endl;
      for (it = heads_to_tie_.begin ();
           !found && (it != heads_to_tie_.end ());
           it++)
        {
          Audio_element_info et = (*it).head_;
          Audio_note *th = dynamic_cast<Audio_note *> (et.elem_);
          Stream_event *left_mus = et.event_;
          cout << "  it " << th->pitch_.to_string()
               << " len " << th->length_mom_.to_string()
               << endl;

          if (th && right_mus && left_mus
              && ly_is_equal (right_mus->get_property ("pitch"),
                              left_mus->get_property ("pitch")))
            {
              found = true;
              cout << "    matched on pitch" << endl;
              // (*it).moment_ already stores the end of the tied note!
              cout << "    now_mom " << now_mom().to_string() << endl;
              cout << "    end_mom " << (*it).end_moment_.to_string() << endl;
              Moment skip = now_mom () - (*it).end_moment_;
              cout << "    skip " << skip.to_string() << endl;
              an->tie_to (th, skip);
              it = heads_to_tie_.erase (it);
            }
        }
    }
}

void
Tie_performer::start_translation_timestep ()
{
  context ()->set_property ("tieMelismaBusy",
                            ly_bool2scm (heads_to_tie_.size ()));
}

// a predicate implemented as a class, used to delete all tied notes with end
// moment in the past:
class end_moment_passed
{
protected:
  Moment now;
public:
  end_moment_passed (Moment mom) : now (mom) {}
  bool operator () (const Head_audio_event_tuple &value)
  {
    cout << "** end_moment_passed? end_moment_ "
         << value.end_moment_.to_string()
         << " <= now " << now.to_string()
         << " ?" << endl;
      
    return (value.end_moment_ <= now);
  }
};

void
Tie_performer::stop_translation_timestep ()
{
  // We might have dangling open ties like c~ d. Close them, unless the first
  // note is still ongoing or we have we have tieWaitForNote set...
  if (!to_boolean (get_property ("tieWaitForNote")))
    {
      cout << "heads_to_tie_ size pre-removal " << heads_to_tie_.size() << endl;
      heads_to_tie_.remove_if (end_moment_passed (now_mom ()));
      cout << "heads_to_tie_ size post-removal " << heads_to_tie_.size() << endl;
    }

  // Append now_heads_ and now_tied_heads to heads_to_tie_ for the next time step
  if (event_)
    {
      cout << "heads_to_tie_ << now_heads_ size " << now_heads_.size() << endl;
      heads_to_tie_.splice (heads_to_tie_.end (), now_heads_);
      cout << "heads_to_tie_ size now " << heads_to_tie_.size() << endl;
    }
  cout << "heads_to_tie_ << now_tied_heads_ size " << now_tied_heads_.size() << endl;
  heads_to_tie_.splice (heads_to_tie_.end (), now_tied_heads_);
  cout << "heads_to_tie_ size now " << heads_to_tie_.size() << endl;

  event_ = 0;
  now_heads_.clear ();
  now_tied_heads_.clear ();
}

ADD_TRANSLATOR (Tie_performer,
                /* doc */
                "Generate ties between note heads of equal pitch.",

                /* create */
                "",

                /* read */
                "tieWaitForNote",

                /* write */
                "tieMelismaBusy"
               );
