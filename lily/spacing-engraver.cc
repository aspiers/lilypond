/*   
  spacing-engraver.cc --  implement Spacing_engraver
  
  source file of the GNU LilyPond music typesetter
  
  (c) 1999--2000 Han-Wen Nienhuys <hanwen@cs.uu.nl>
  
 */

#include "musical-request.hh"
#include "paper-column.hh"

#include "spacing-spanner.hh"
#include "engraver.hh"
#include "pqueue.hh"

struct Rhythmic_tuple
{
  Grob_info info_;
  Moment end_;
  
  Rhythmic_tuple ()
    {
    }
  Rhythmic_tuple (Grob_info i, Moment m )
    {
      info_ = i;
      end_ = m;
    }
  static int time_compare (Rhythmic_tuple const &, Rhythmic_tuple const &);  
};

/**
   Acknowledge rhythmic elements, for initializing spacing fields in
   the columns.

   should be the  last one of the toplevel context
*/
class Spacing_engraver : public Engraver
{
  PQueue<Rhythmic_tuple> playing_durations_;
  Array<Rhythmic_tuple> now_durations_;
  Array<Rhythmic_tuple> stopped_durations_;

  Spanner * spacing_p_;
protected:
  VIRTUAL_COPY_CONS(Translator);
  virtual void acknowledge_grob (Grob_info);
  virtual void start_translation_timestep ();
  virtual void stop_translation_timestep ();
  virtual void initialize ();
  virtual void finalize ();
public:
  Spacing_engraver ();
};

inline int
compare (Rhythmic_tuple const &a, Rhythmic_tuple const &b)
{
  return Rhythmic_tuple::time_compare (a,b);
}

int
Rhythmic_tuple::time_compare (Rhythmic_tuple const &h1,
			      Rhythmic_tuple const &h2)
{
  return (h1.end_ - h2.end_ ).sign ();
}

Spacing_engraver::Spacing_engraver()
{
  spacing_p_ = 0;
}

void
Spacing_engraver::initialize ()
{
  spacing_p_  =new Spanner (get_property ("SpacingSpanner"));
  Spacing_spanner::set_interface (spacing_p_);
  spacing_p_->set_bound (LEFT, unsmob_grob (get_property ("currentCommandColumn")));  
  announce_grob (spacing_p_, 0);
}

void
Spacing_engraver::finalize ()
{
  Grob * p = unsmob_grob (get_property ("currentCommandColumn"));
  spacing_p_->set_bound (RIGHT, p);
  typeset_grob (spacing_p_);
  spacing_p_ =0;
}

void
Spacing_engraver::acknowledge_grob (Grob_info i)
{
  if (to_boolean (i.elem_l_->get_grob_property ("grace")))
    return;

  if (to_boolean (i.elem_l_->get_grob_property ("non-rhythmic")))
    return;
  
  if (Rhythmic_req * r = dynamic_cast<Rhythmic_req*>(i.req_l_))
    {
      Rhythmic_tuple t(i, now_mom () + r->length_mom ());
      now_durations_.push (t);
    }
}

void
Spacing_engraver::stop_translation_timestep ()
{
  Moment shortest_playing;
  shortest_playing.set_infinite (1);
  for (int i=0; i < playing_durations_.size (); i++)
    {
      Moment m = (playing_durations_[i].info_.req_l_)->length_mom ();
      if (m)
	{
	  shortest_playing = shortest_playing <? m;
	}
    }
  
  Moment starter, inf;
  inf.set_infinite (1);
  starter=inf;
  for (int i=0; i < now_durations_.size (); i++)
    {
      Moment m = now_durations_[i].info_.req_l_->length_mom ();
      if (m)
	starter = starter <? m;

      playing_durations_.insert (now_durations_[i]);
    }
  now_durations_.clear ();
  
  shortest_playing = shortest_playing <? starter;
  
  Paper_column * sc
    = dynamic_cast<Paper_column*> (unsmob_grob (get_property ("currentMusicalColumn")));

  SCM sh = shortest_playing.smobbed_copy( );
  SCM st = starter.smobbed_copy();

  sc->set_grob_property ("shortest-playing-duration", sh);  
  sc->set_grob_property ("shortest-starter-duration", st);
}

void
Spacing_engraver::start_translation_timestep ()
{
  Moment now = now_mom ();
  stopped_durations_.clear ();
  while (playing_durations_.size () && playing_durations_.front ().end_ < now)
    playing_durations_.delmin ();
  while (playing_durations_.size () && playing_durations_.front ().end_ == now)
    stopped_durations_.push (playing_durations_.get ());
}

ADD_THIS_TRANSLATOR(Spacing_engraver);


