/*
  item.cc -- implement Item

  source file of the GNU LilyPond music typesetter

  (c)  1997--2002 Han-Wen Nienhuys <hanwen@cs.uu.nl>
*/


#include "paper-score.hh"
#include "warn.hh"
#include "item.hh"
#include "paper-column.hh"
#include "spanner.hh"
#include "lily-guile.hh"
#include "system.hh"
#include "group-interface.hh"

Item::Item (SCM s)
  : Grob (s)
{
  broken_to_drul_[LEFT] = broken_to_drul_[RIGHT]=0;
  Group_interface::add_thing (this, ly_symbol2scm ("interfaces"), ly_symbol2scm ("item-interface"));
}

/**
   Item copy ctor.  Copy nothing: everything should be a elt property
   or a special purpose pointer (such as broken_to_drul_[]) */
Item::Item (Item const &s)
  : Grob (s)
{
  broken_to_drul_[LEFT] = broken_to_drul_[RIGHT] =0;
}


bool
Item::breakable_b (Grob*me) 
{
  if (me->original_)
    return false;

  if (!dynamic_cast<Item*> (me))
    me->programming_error ("only items can be breakable.");
  
  Item * i  =dynamic_cast<Item*> (me->get_parent (X_AXIS));
  return (i) ?  Item::breakable_b (i) : to_boolean (me->get_grob_property ("breakable"));
}

Paper_column *
Item::get_column () const
{
  Item *parent = dynamic_cast<Item*> (get_parent (X_AXIS));
  return parent ? parent->get_column () : 0;
}

System *
Item::get_system () const
{
  Grob *g = get_parent (X_AXIS);
  return g ?  g->get_system () : 0;
}


void
Item::copy_breakable_items ()
{
  Drul_array<Item *> new_copies;
  Direction  i=LEFT;
  do 
    {
      Grob * dolly = clone ();
      Item * item = dynamic_cast<Item*> (dolly);
      pscore_->system_->typeset_grob (item);
      new_copies[i] =item;
    }
  while (flip (&i) != LEFT);
  broken_to_drul_= new_copies;
}


bool
Item::broken_b () const
{
  return broken_to_drul_[LEFT] || broken_to_drul_[RIGHT];
}


/*
  Generate items for begin and end-of line.
 */
void
Item::discretionary_processing ()
{
  if (broken_b ())
    return;

  if (Item::breakable_b (this))
    copy_breakable_items ();
}

Grob*
Item::find_broken_piece (System*l) const
{
  if (get_system () == l) 
    return (Item*) (this);

  Direction d = LEFT;
  do {
    Grob *s = broken_to_drul_[d];
    if (s && s->get_system () == l)
      return s;
  }
  while (flip (&d) != LEFT);

  return 0;
}


Item*
Item::find_prebroken_piece (Direction d) const
{
  Item * me = (Item *) (this);	
  if (!d)
    return me;
  return dynamic_cast<Item*> (broken_to_drul_[d]);
}


Direction
Item::break_status_dir () const
{
  if (original_)
    {
      Item * i = dynamic_cast<Item*> (original_);

      return (i->broken_to_drul_[LEFT] == this) ? LEFT : RIGHT;
    }
  else
    return CENTER;
}

void
Item::handle_prebroken_dependencies ()
{
  Grob::handle_prebroken_dependencies ();
  
  /*
    Can't do this earlier, because try_visibility_lambda () might set
    the elt property transparent, which would then be copied.

    TODO:

    give the item to break-visibility itself, so the function can do
    more complicated things.
  */
  SCM vis = get_grob_property ("break-visibility");
  if (gh_procedure_p (vis))
    {
      SCM args = scm_list_n (gh_int2scm (break_status_dir ()), SCM_UNDEFINED);
      SCM result = gh_apply (vis, args);
      bool trans = gh_scm2bool (ly_car (result));
      bool empty = gh_scm2bool (ly_cdr (result));
      
      if (empty && trans)
	suicide ();
      else if (empty)
	{
	  set_extent (SCM_EOL, X_AXIS);
	  set_extent (SCM_EOL, Y_AXIS);
	}
      else if (trans)
	set_grob_property ("molecule-callback", SCM_EOL);
    }
}

SCM
Item::do_derived_mark ()const
{
  if (broken_to_drul_[LEFT])
    scm_gc_mark (broken_to_drul_[LEFT]->self_scm ());
  if (broken_to_drul_[RIGHT])
    scm_gc_mark (broken_to_drul_[RIGHT]->self_scm ());
  return SCM_EOL;
}

Item*
unsmob_item (SCM s )
{
  return dynamic_cast<Item*> (unsmob_grob (s));
}



ADD_INTERFACE(Item,
	      "item-interface",
	      "

Grobs can be distinguished in their role in the horizontal spacing.
Many grobs define constraints on the spacing by their sizes. For
example, note heads, clefs, stems, and all other symbols with a fixed
shape.  These grobs form a subtype called @code{Item}.

",
	      "no-spacing-rods break-visibility breakable")
