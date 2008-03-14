/*
  accidental-placement.cc -- implement Accidental_placement

  source file of the GNU LilyPond music typesetter

  (c) 2002--2009 Han-Wen Nienhuys <hanwen@xs4all.nl>
*/


#include "accidental-placement.hh"

#include "item.hh"
#include "rhythmic-head.hh"
#include "accidental-interface.hh"
#include "music.hh"
#include "note-collision.hh"
#include "note-column.hh"
#include "pointer-group-interface.hh"
#include "skyline.hh"
#include "stream-event.hh"
#include "warn.hh"


void
Accidental_placement::add_accidental (Grob *me, Grob *a)
{
  a->set_parent (me, X_AXIS);
  a->set_property ("X-offset", Grob::x_parent_positioning_proc);
  SCM cause = a->get_parent (Y_AXIS)->get_property ("cause");

  Stream_event *mcause = unsmob_stream_event (cause);
  if (!mcause)
    {
      programming_error ("note head has no event cause");
      return;
    }

  Pitch *p = unsmob_pitch (mcause->get_property ("pitch"));

  int n = p->get_notename ();

  SCM accs = me->get_object ("accidental-grobs");
  SCM key = scm_from_int (n);
  SCM entry = scm_assq (key, accs);
  if (entry == SCM_BOOL_F)
    entry = SCM_EOL;
  else
    entry = scm_cdr (entry);

  entry = scm_cons (a->self_scm (), entry);

  accs = scm_assq_set_x (accs, key, entry);

  me->set_object ("accidental-grobs", accs);
}

/*
  Split into break reminders.
*/
void
Accidental_placement::split_accidentals (Grob *accs,
					 vector<Grob*> *break_reminder,
					 vector<Grob*> *real_acc)
{
  for (SCM acs = accs->get_object ("accidental-grobs"); scm_is_pair (acs);
       acs = scm_cdr (acs))
    for (SCM s = scm_cdar (acs); scm_is_pair (s); s = scm_cdr (s))
      {
	Grob *a = unsmob_grob (scm_car (s));

	if (unsmob_grob (a->get_object ("tie"))
	    && !to_boolean (a->get_property ("forced")))
	  break_reminder->push_back (a);
	else
	  real_acc->push_back (a);
      }
}

vector<Grob*>
Accidental_placement::get_relevant_accidentals (vector<Grob*> const &elts, Grob *left)
{
  vector<Grob*> br;
  vector<Grob*> ra;
  vector<Grob*> ret;
  bool right = dynamic_cast<Item *> (left)->break_status_dir () == RIGHT;

  for (vsize i = 0; i < elts.size (); i++)
    {
      split_accidentals (elts[i], &br, &ra);
      
      ret.insert (ret.end (), ra.begin (), ra.end ());

      if (right)
	ret.insert (ret.end (), br.begin (), br.end ());
    }
  return ret;
}

struct Accidental_placement_entry
{
  Skyline left_skyline_;
  Skyline right_skyline_;
  Interval vertical_extent_;
  vector<Box> extents_;
  vector<Grob*> grobs_;
  Real offset_;
  int notename_;
  Accidental_placement_entry ()
  {
    offset_ = 0.0;
    notename_ = -1;
  }
};

static Interval all_accidental_vertical_extent;
Real ape_priority (Accidental_placement_entry const *a)
{
  return a->vertical_extent_[UP];
}

int ape_compare (Accidental_placement_entry *const &a,
		 Accidental_placement_entry *const &b)
{
  return sign (ape_priority (a) - ape_priority (b));
}

bool ape_less (Accidental_placement_entry *const &a,
	       Accidental_placement_entry *const &b)
{
  return ape_priority (a) < ape_priority (b);
}

int ape_rcompare (Accidental_placement_entry *const &a,
		  Accidental_placement_entry *const &b)
{
  return -sign (ape_priority (a) - ape_priority (b));
}

/*
  TODO: should favor

  b
  b

  placement
*/
void
stagger_apes (vector<Accidental_placement_entry*> *apes)
{
  vector<Accidental_placement_entry*> asc = *apes;

  vector_sort (asc, &ape_less);

  apes->clear ();

  int parity = 1;
  for (vsize i = 0; i < asc.size ();)
    {
      Accidental_placement_entry *a = 0;
      if (parity)
	{
	  a = asc.back ();
	  asc.pop_back ();
	}
      else
	a = asc[i++];

      apes->push_back (a);
      parity = !parity;
    }

  reverse (*apes);
}

static void
set_ape_skylines (Accidental_placement_entry *ape,
		  Grob **common)
{
  for (vsize j = ape->grobs_.size (); j--;)
    {
      Grob *a = ape->grobs_[j];
      vector<Box> boxes = Accidental_interface::accurate_boxes (a, common);
      ape->extents_.insert (ape->extents_.end (), boxes.begin (), boxes.end ());
    }
  ape->left_skyline_ = Skyline (ape->extents_, 0, Y_AXIS, LEFT);
  ape->right_skyline_ = Skyline (ape->extents_, 0, Y_AXIS, RIGHT);
}

static vector<Grob*>
extract_heads_and_stems (vector<Accidental_placement_entry*> const &apes)
{
  vector<Grob*> note_cols;
  vector<Grob*> ret;

  for (vsize i = apes.size (); i--;)
    {
      Accidental_placement_entry *ape = apes[i];
      for (vsize j = ape->grobs_.size (); j--;)
	{
	  Grob *acc = ape->grobs_[j];
	  Grob *head = acc->get_parent (Y_AXIS);
	  Grob *col = head->get_parent (X_AXIS);

	  if (Note_column::has_interface (col))
	    note_cols.push_back (col);
	  else
	    ret.push_back (head);
	}
    }

  /*
    This is a little kludgy: in case there are note columns without
    accidentals, we get them from the Note_collision objects.
  */
  for (vsize i = note_cols.size (); i--;)
    {
      Grob *c = note_cols[i]->get_parent (X_AXIS);
      if (Note_collision_interface::has_interface (c))
	{
	  extract_grob_set (c, "elements", columns);
	  concat (note_cols, columns);
	}
    }

  /* Now that we have all of the columns, grab all of the note-heads */
  for (vsize i = note_cols.size (); i--;)
    concat (ret, extract_grob_array (note_cols[i], "note-heads"));

  /* Now that we have all of the heads, grab all of the stems */
  for (vsize i = ret.size (); i--;)
    if (Grob *s = Rhythmic_head::get_stem (ret[i]))
      ret.push_back (s);


  vector_sort (ret, less<Grob*> ());
  uniq (ret);
  return ret;
}

static Grob*
common_refpoint_of_accidentals (vector<Accidental_placement_entry*> const &apes, Axis a)
{
  Grob *ret = 0;

  for (vsize i = apes.size (); i--;)
    for (vsize j = apes[i]->grobs_.size (); j--;)
      {
	if (!ret)
	  ret = apes[i]->grobs_[j];
	else
	  ret = ret->common_refpoint (apes[i]->grobs_[j], a);
      }

  return ret;
}


/*
  This routine computes placements of accidentals. During
  add_accidental (), accidentals are already grouped by note, so that
  octaves are placed above each other; they form columns. Then the
  columns are sorted: the biggest columns go closest to the note.
  Then the columns are spaced as closely as possible (using skyline
  spacing).


  TODO: more advanced placement. Typically, the accs should be placed
  to form a C shape, like this


  ##
  b b
  # #
  b
  b b

  The naturals should be left of the C as well; they should
  be separate accs.

  Note that this placement problem looks NP hard, so we just use a
  simple strategy, not an optimal choice.
*/

/*
  TODO: there should be more space in the following situation


  Natural + downstem

  *
  *  |_
  *  | |    X
  *  |_|   |
  *    |   |
  *

*/

MAKE_SCHEME_CALLBACK (Accidental_placement, calc_positioning_done, 1);
SCM
Accidental_placement::calc_positioning_done (SCM smob)
{
  Grob *me = unsmob_grob (smob);
  if (!me->is_live ())
    return SCM_BOOL_T;

  me->set_property ("positioning-done", SCM_BOOL_T);
  
  SCM accs = me->get_object ("accidental-grobs");
  if (!scm_is_pair (accs))
    return SCM_BOOL_T;

  vector<Accidental_placement_entry*> apes;
  for (SCM s = accs; scm_is_pair (s); s = scm_cdr (s))
    {
      Accidental_placement_entry *ape = new Accidental_placement_entry;
      ape->notename_ = scm_to_int (scm_caar (s));

      for (SCM t = scm_cdar (s); scm_is_pair (t); t = scm_cdr (t))
	ape->grobs_.push_back (unsmob_grob (scm_car (t)));

      apes.push_back (ape);
    }

  Grob *common[] = {me, 0};

  vector<Grob*> heads_and_stems = extract_heads_and_stems (apes);

  common[Y_AXIS] = common_refpoint_of_accidentals (apes, Y_AXIS);
  common[Y_AXIS] = common_refpoint_of_array (heads_and_stems, common[Y_AXIS], Y_AXIS);
  common[X_AXIS] = common_refpoint_of_array (heads_and_stems, me, X_AXIS);

  for (vsize i = apes.size (); i--;)
    set_ape_skylines (apes[i], common);

  Interval total;
  for (vsize i = apes.size (); i--;)
    {
      Interval y;

      for (vsize j = apes[i]->extents_.size (); j--;)
	y.unite (apes[i]->extents_[j][Y_AXIS]);
      apes[i]->vertical_extent_ = y;
      total.unite (y);
    }
  all_accidental_vertical_extent = total;
  stagger_apes (&apes);

  Accidental_placement_entry *head_ape = new Accidental_placement_entry;
  
  vector<Box> head_extents;
  for (vsize i = heads_and_stems.size (); i--;)
    head_extents.push_back (Box (heads_and_stems[i]->extent (common[X_AXIS], X_AXIS),
				 heads_and_stems[i]->pure_height (common[Y_AXIS], 0, INT_MAX)));

  head_ape->left_skyline_ = Skyline (head_extents, 0, Y_AXIS, LEFT);
  head_ape->offset_ = 0.0;

  Real padding = robust_scm2double (me->get_property ("padding"), 0.2);

  Skyline left_skyline = head_ape->left_skyline_;
  left_skyline.raise (-robust_scm2double (me->get_property ("right-padding"), 0));
  
  /*
    Add accs entries right-to-left.
  */
  for (vsize i = apes.size (); i-- > 0;)
    {
      Real offset = -apes[i]->right_skyline_.distance (left_skyline);
      if (isinf (offset))
	offset = (i + 1 < apes.size ()) ? apes[i + 1]->offset_ : 0.0;
      else
	offset -= padding;

      apes[i]->offset_ = offset;

      Skyline new_left_skyline = apes[i]->left_skyline_;
      new_left_skyline.raise (apes[i]->offset_);
      new_left_skyline.merge (left_skyline);
      left_skyline = new_left_skyline;
    }

  for (vsize i = apes.size (); i--;)
    {
      Accidental_placement_entry *ape = apes[i];
      for (vsize j = ape->grobs_.size (); j--;)
	ape->grobs_[j]->translate_axis (ape->offset_, X_AXIS);
    }

  Interval left_extent, right_extent;
  Accidental_placement_entry *ape = apes[0];

  for (vsize i = ape->extents_.size (); i--;)
    left_extent.unite (ape->offset_ + ape->extents_[i][X_AXIS]);

  ape = apes.back ();
  for (vsize i = ape->extents_.size (); i--;)
    right_extent.unite (ape->offset_ + ape->extents_[i][X_AXIS]);

  left_extent[LEFT] -= robust_scm2double (me->get_property ("left-padding"), 0);
  Interval width (left_extent[LEFT], right_extent[RIGHT]);

  SCM scm_width = ly_interval2scm (width);
  me->flush_extent_cache (X_AXIS);
  me->set_property ("X-extent", scm_width);

  junk_pointers (apes);

  delete head_ape;
  
  return SCM_BOOL_T;
}

ADD_INTERFACE (Accidental_placement,
	       "Resolve accidental collisions.",

	       /* properties */
	       "accidental-grobs "
	       "direction "
	       "left-padding "
	       "padding "
	       "positioning-done "
	       "right-padding "
	       "script-priority "
	       )
