/*
  tuplet-bracket.cc -- implement Tuplet_bracket

  source file of the GNU LilyPond music typesetter

  (c) 1997--2004 Jan Nieuwenhuizen <janneke@gnu.org>
  Han-Wen Nienhuys <hanwen@xs4all.nl>
*/

/*
  TODO:

  - tuplet bracket should probably be subject to the same rules as
  beam sloping/quanting.

  - There is no support for kneed brackets, or nested brackets.

  - number placement for parallel beams should be much more advanced:
    for sloped beams some extra horizontal offset must be introduced.

  - number placement is usually done over the center note, not the
    graphical center.
  
 */

#include <math.h>

#include "line-interface.hh"
#include "beam.hh"
#include "box.hh"
#include "warn.hh"
#include "font-interface.hh"
#include "molecule.hh"
#include "paper-def.hh"
#include "text-item.hh"
#include "tuplet-bracket.hh"
#include "stem.hh"
#include "note-column.hh"
#include "group-interface.hh"
#include "directional-element-interface.hh"
#include "spanner.hh"
#include "staff-symbol-referencer.hh"
#include "lookup.hh"


static Grob*
get_x_bound_grob (Grob *g, Direction my_dir)
{
  if (Note_column::get_stem (g)
      && Note_column::dir (g) == my_dir)
    {
      g = Note_column::get_stem (g);
    }
  return g;
}



Grob*
Tuplet_bracket::parallel_beam (Grob *me, Link_array<Grob> const &cols, bool *equally_long)
{
  /*
    ugh: code dup. 
  */
  Grob *s1 = Note_column::get_stem (cols[0]); 
  Grob *s2 = Note_column::get_stem (cols.top());    

  Grob*b1 = s1 ? Stem::get_beam (s1) : 0;
  Grob*b2 = s2 ? Stem::get_beam (s2) : 0;
  
  Spanner*sp = dynamic_cast<Spanner*> (me);  

  *equally_long= false;
  if (! (b1 && (b1 == b2) && !sp->broken_b()))
      return 0;

  Link_array<Grob> beam_stems = Pointer_group_interface__extract_grobs
    (b1, (Grob*)0, "stems");

  if (beam_stems.size() == 0)
    {
      programming_error ("Beam under tuplet bracket has no stems!");
      *equally_long = 0;
      return 0;
    }
  
  *equally_long = (beam_stems[0] == s1 && beam_stems.top() == s2);
  return b1;
}


/*
  TODO:

  in the case that there is no bracket, but there is a (single) beam,
  follow beam precisely for determining tuplet number location.
  
 */
MAKE_SCHEME_CALLBACK (Tuplet_bracket,brew_molecule,1);
SCM
Tuplet_bracket::brew_molecule (SCM smob) 
{
  Grob *me= unsmob_grob (smob);
  Molecule  mol;
  Link_array<Grob> columns=
    Pointer_group_interface__extract_grobs (me, (Grob*)0, "note-columns");

  if (!columns.size ())
    return mol.smobbed_copy ();

  

  {
    SCM lp = me->get_grob_property ("left-position");
    SCM rp = me->get_grob_property ("right-position");  

    if (!gh_number_p (rp) || !gh_number_p (lp))
      after_line_breaking (smob);
  }
  
  Real ly = robust_scm2double (me->get_grob_property ("left-position"), 0);
  Real ry = robust_scm2double (me->get_grob_property ("right-position"), 0);  
  
  bool equally_long = false;
  Grob * par_beam = parallel_beam (me, columns, &equally_long);

  Spanner*sp = dynamic_cast<Spanner*> (me);  

  bool bracket_visibility = !(par_beam && equally_long);
  bool number_visibility = true;

  /*
    Fixme: the type of this prop is sucky.
   */
  SCM bracket = me->get_grob_property ("bracket-visibility");
  if (gh_boolean_p (bracket))
    {
      bracket_visibility = gh_scm2bool (bracket);
    }
  else if (bracket == ly_symbol2scm ("if-no-beam"))
    bracket_visibility = !par_beam;

  SCM numb = me->get_grob_property ("number-visibility");  
  if (gh_boolean_p (numb))
    {
      number_visibility = gh_scm2bool (numb);
    }
  else if (numb == ly_symbol2scm ("if-no-beam"))
    number_visibility = !par_beam;
	
  Grob * commonx = columns[0]->common_refpoint (columns.top (),X_AXIS);

  /*
    Tuplet brackets are normally not broken, but we shouldn't crash if
    they are.
   */
  commonx = commonx->common_refpoint (sp->get_bound(LEFT), X_AXIS);
  commonx = commonx->common_refpoint (sp->get_bound(RIGHT), X_AXIS);  
  
  Direction dir = get_grob_direction (me);

  Grob * lgr = get_x_bound_grob (columns[0], dir);
  Grob * rgr = get_x_bound_grob (columns.top(), dir);  
  Real x0 = lgr->extent (commonx,X_AXIS)[LEFT];
  Real x1 = rgr->extent (commonx,X_AXIS)[RIGHT];

  Real w = x1 -x0;
  
  SCM number = me->get_grob_property ("text");

  Paper_def *pap = me->get_paper ();
  if (gh_string_p (number) && number_visibility)
    {
      SCM properties = Font_interface::font_alist_chain (me);
      SCM snum = Text_item::interpret_markup (pap->self_scm (), properties, number);
      Molecule num = *unsmob_molecule (snum);
      num.align_to (X_AXIS, CENTER);
      num.translate_axis (w/2, X_AXIS);
      num.align_to (Y_AXIS, CENTER);
	
      num.translate_axis ((ry-ly)/2, Y_AXIS);

      mol.add_molecule (num);
    }


  /*
    No bracket when it would be smaller than the number.

    TODO: should use GAP in calculation too.
   */
  if (bracket_visibility && number_visibility
      && mol.extent (X_AXIS).length () > w)
    {
      bracket_visibility = false;
    }
  
  if (bracket_visibility)      
    {
      Real ss =   Staff_symbol_referencer::staff_space (me);
      Real gap = robust_scm2double (me->get_grob_property ("gap"), 1.0)
	* ss;
      
      SCM fl = me->get_grob_property ("bracket-flare");
      SCM eh = me->get_grob_property ("edge-height");
      SCM sp = me->get_grob_property ("shorten-pair");
      
      Direction d = LEFT;
      Drul_array<Real> height, flare, shorten;
      do {
	flare[d] =  height[d] = shorten[d] = 0.0;
	if (is_number_pair (fl))
	  flare[d] +=  ss * gh_scm2double (index_get_cell (fl, d));
	if (is_number_pair (eh))
	  height[d] +=  - dir * ss *gh_scm2double (index_get_cell (eh, d));
	if (is_number_pair (sp))
	  shorten[d] +=  ss *gh_scm2double (index_get_cell (sp, d));
      }
      while (flip (&d) != LEFT);
      
      Molecule brack = make_bracket (me, Y_AXIS,
				     Offset (w, ry - ly), 
				     height, gap,
				     flare, shorten);
      mol.add_molecule (brack);
    }
  
  mol.translate_axis (ly, Y_AXIS);
  mol.translate_axis (x0  - sp->get_bound (LEFT)->relative_coordinate (commonx,X_AXIS),X_AXIS);
  return mol.smobbed_copy ();
}

/*
  should move to lookup?

  TODO: this will fail for very short (shorter than the flare)
  brackets.
 */
Molecule
Tuplet_bracket::make_bracket (Grob *me,	// for line properties.
			      Axis protusion_axis,
			      Offset dz,
			      Drul_array<Real> height,
			      Real gap,
			      Drul_array<Real> flare,
			      Drul_array<Real> shorten)
{
  Drul_array<Offset> corners (Offset(0,0), dz);
  
  Real length = dz.length ();
  Drul_array<Offset> gap_corners;

  
  Axis bracket_axis = other_axis (protusion_axis);

  Drul_array<Offset> straight_corners = corners;

  Direction d = LEFT;
  do {
    straight_corners[d] += - d * shorten[d] /length * dz;
  } while (flip (&d) != LEFT);

  /*
    UGH: the shortening factor is magic.
   */
  gap = gap <?
    (0.66 * (straight_corners[RIGHT] - straight_corners[LEFT]).length ());
  
  do {
    gap_corners[d] = (dz * 0.5) + d * 0.5 * gap / length * dz;
  } while (flip (&d) != LEFT);

  Drul_array<Offset> flare_corners = straight_corners;
  do {
    flare_corners[d][bracket_axis] = straight_corners[d][bracket_axis];
    flare_corners[d][protusion_axis] += height[d];
    straight_corners[d][bracket_axis] += - d * flare[d];
  } while (flip (&d) != LEFT);

  Molecule m;
  do {
    m.add_molecule (Line_interface::line (me, straight_corners[d],
						 gap_corners[d]));
    
    m.add_molecule (Line_interface::line (me,  straight_corners[d],
						 flare_corners[d]));
  } while (flip (&d) != LEFT);

  return m;  
}


/*
  use first -> last note for slope, and then correct for disturbing
  notes in between.  */
void
Tuplet_bracket::calc_position_and_height (Grob*me,Real *offset, Real * dy) 
{
  Link_array<Grob> columns=
    Pointer_group_interface__extract_grobs (me, (Grob*)0, "note-columns");


  SCM cols = me->get_grob_property ("note-columns");
  Grob * commony = common_refpoint_of_list (cols, me, Y_AXIS);
  Grob * commonx = common_refpoint_of_list (cols, me, X_AXIS);  

  Interval staff;
  if (Grob * st = Staff_symbol_referencer::get_staff_symbol (me))
    staff = st->extent (commony, Y_AXIS);
  
  Direction dir = get_grob_direction (me);

  /*
    Use outer non-rest columns to determine slope
   */
  int l = 0;
  while (l <columns.size () && Note_column::has_rests (columns[l]))
    l ++;

  int r = columns.size ()- 1;
  while (r >= l && Note_column::has_rests (columns[r]))
    r--;
  
  if (l < r)
    {
      Interval rv =columns[r]->extent (commony, Y_AXIS);
      Interval lv =columns[l]->extent (commony, Y_AXIS);
      rv.unite (staff);
      lv.unite (staff);
      Real graphical_dy =  rv[dir] - lv[dir];

      Slice ls = Note_column::head_positions_interval (columns[l]);
      Slice rs = Note_column::head_positions_interval (columns[r]);
      
      Interval musical_dy;
      musical_dy[UP] = rs[UP] - ls[UP];
      musical_dy[DOWN] = rs[DOWN] - ls[DOWN];
      if (sign (musical_dy[UP]) != sign (musical_dy[DOWN]))
	*dy = 0.0;
      else if (sign (graphical_dy) != sign (musical_dy[DOWN]))
	*dy = 0.0;
      else
	*dy = graphical_dy;
    }
  else
    * dy = 0;


  *offset = - dir * infinity_f;

  if (!columns.size ())
    return;


  
  Grob * lgr = get_x_bound_grob (columns[0], dir);
  Grob * rgr = get_x_bound_grob (columns.top(), dir);  
  Real x0 = lgr->extent (commonx,X_AXIS)[LEFT];
  Real x1 = rgr->extent (commonx,X_AXIS)[RIGHT];


  /*
    Slope.
  */
  Real factor = columns.size () > 1 ? 1/ (x1 - x0) : 1.0;
  
  for (int i = 0; i < columns.size ();  i++)
    {
      Interval note_ext =columns[i]->extent (commony, Y_AXIS);
      note_ext.unite (staff);
      Real notey = note_ext[dir] - me->relative_coordinate (commony, Y_AXIS);
      
      Real x = columns[i]->relative_coordinate (commonx, X_AXIS) - x0;
      Real tuplety =  *dy * x * factor;

      if (notey * dir > (*offset + tuplety) * dir)
	*offset = notey - tuplety; 
    }

  // padding
  *offset +=  gh_scm2double (me->get_grob_property ("padding")) *dir;

  
  /*
    horizontal brackets should not collide with staff lines.
    
   */
  Real ss= Staff_symbol_referencer::staff_space (me);
  if (*dy == 0 && fabs (*offset) <  ss * Staff_symbol_referencer::staff_radius (me))
    {
      // quantize, then do collision check.
      *offset *= 2 / ss;
      
      *offset = rint (*offset);
      if (Staff_symbol_referencer::on_staffline (me, (int) rint (*offset)))
	*offset += dir;

      *offset *= 0.5 * ss;
    }
  
}



/*
  We depend on the beams if there are any.
 */
MAKE_SCHEME_CALLBACK (Tuplet_bracket,before_line_breaking,1);
SCM
Tuplet_bracket::before_line_breaking (SCM smob)
{
  Grob *me = unsmob_grob (smob);
  Link_array<Grob> columns=
    Pointer_group_interface__extract_grobs (me, (Grob*)0, "note-columns");


  for (int i = columns.size(); i--;)
    {
      Grob * s =Note_column::get_stem (columns[i]);
      Grob * b = s ? Stem::get_beam (s): 0;
      if (b)
	me->add_dependency (b);
    }
  return SCM_UNDEFINED;
}

MAKE_SCHEME_CALLBACK (Tuplet_bracket,after_line_breaking,1);

SCM
Tuplet_bracket::after_line_breaking (SCM smob)
{
  Grob * me = unsmob_grob (smob);
  Link_array<Grob> columns=
    Pointer_group_interface__extract_grobs (me, (Grob*)0, "note-columns");

  if (!columns.size ())
    {
      me->suicide ();
      return SCM_UNSPECIFIED;
    }
  if (dynamic_cast<Spanner*> (me)->broken_b ())
    {
      me->warning (_("Killing tuplet bracket across linebreak."));
      me->suicide();
      return SCM_UNSPECIFIED;
    }
  
  Direction dir = get_grob_direction (me);
  if (!dir)
    {
      dir = Tuplet_bracket::get_default_dir (me);
      set_grob_direction (me, dir);
    }
  
  bool equally_long = false;
  Grob * par_beam = parallel_beam (me, columns, &equally_long);

  /*
    We follow the beam only if there is one, and we are next to it.
   */
  Real dy, offset;
  if (!par_beam
      || get_grob_direction (par_beam) != dir)
    {
      calc_position_and_height (me,&offset,&dy);
    }
  else
    {
      SCM ps =  par_beam->get_grob_property ("positions"); 

      Real lp = gh_scm2double (gh_car (ps));
      Real rp = gh_scm2double (gh_cdr (ps));

      /*
	duh. magic.
       */
      offset = lp + dir * (0.5 + gh_scm2double (me->get_grob_property ("padding")));
      dy = rp- lp;
    }
  
  
  SCM lp =  me->get_grob_property ("left-position");
  SCM rp = me->get_grob_property ("right-position");  
  
  if (gh_number_p (lp) && !gh_number_p (rp))
    {
      rp = gh_double2scm (gh_scm2double (lp) + dy);
    }
  else if (gh_number_p (rp) && !gh_number_p (lp))
    {
      lp = gh_double2scm (gh_scm2double (rp) - dy);
    }
  else if (!gh_number_p (rp) && !gh_number_p (lp))
    {
      lp = gh_double2scm (offset);
      rp = gh_double2scm (offset +dy);
    }

  me->set_grob_property ("left-position", lp);
  me->set_grob_property ("right-position", rp);

  return SCM_UNSPECIFIED;
}


/*
  similar to beam ? 
 */
Direction
Tuplet_bracket::get_default_dir (Grob*me)
{
  Drul_array<int> dirs(0,0);  
  for (SCM s = me->get_grob_property ("note-columns"); gh_pair_p (s); s = ly_cdr (s))
    {
      Grob * nc = unsmob_grob (ly_car (s));
      Direction d = Note_column::dir (nc);
      if (d)
	dirs[d]++;
    }

  return dirs[UP] >= dirs[DOWN] ? UP : DOWN;
}

void
Tuplet_bracket::add_column (Grob*me, Item*n)
{
  Pointer_group_interface::add_grob (me, ly_symbol2scm ("note-columns"), n);
  me->add_dependency (n);

  add_bound_item (dynamic_cast<Spanner*> (me), n);
}






ADD_INTERFACE (Tuplet_bracket,"tuplet-bracket-interface",
  "A bracket with a number in the middle, used for tuplets.",
  "note-columns bracket-flare edge-height shorten-pair padding gap left-position right-position bracket-visibility number-visibility thickness direction");

