/*   
  simple-spacer.cc -- implement Simple_spacer
  
  source file of the GNU LilyPond music typesetter
  
  (c) 1999--2000 Han-Wen Nienhuys <hanwen@cs.uu.nl>

  TODO:
  - add support for different stretch/shrink constants?
  
*/

#include <math.h>
#include <libc-extension.hh>

#include "simple-spacer.hh"
#include "paper-column.hh"
#include "spring.hh"
#include "rod.hh"
#include "warn.hh"
#include "column-x-positions.hh"
#include "spaceable-element.hh"
#include "dimensions.hh"

Simple_spacer::Simple_spacer ()
{
  force_f_ = 0.;
  indent_f_ =0.0;
  default_space_f_ = 20 PT;
}

void
Simple_spacer::add_rod (int l, int r, Real dist)
{
  if (isinf (dist) || isnan (dist))
    {
      programming_error ("Weird minimum distance. Ignoring");
      return;
    }
  
  
  Real c = range_stiffness (l,r);
  Real d = range_ideal_len (l,r);
  Real block_stretch = dist - d;
  
  Real block_force = c * block_stretch;
  force_f_ = force_f_ >? block_force;

  for (int i=l; i < r; i++)
    springs_[i].block_force_f_ = block_force >?
      springs_[i].block_force_f_ ;
}

Real
Simple_spacer::range_ideal_len (int l, int r)   const
{
  Real d =0.;
  for (int i=l; i < r; i++)
    d += springs_[i].ideal_f_;
  return d;
}

Real
Simple_spacer::range_stiffness (int l, int r) const
{
  Real den =0.0;
  for (int i=l; i < r; i++)
    den += 1 / springs_[i].hooke_f_;

  return 1 / den;
}

Real
Simple_spacer::active_blocking_force () const
{
  Real bf = - infinity_f; 
  for (int i=0; i < springs_.size (); i++)
    if (springs_[i].active_b_)
      {
	bf = bf >? springs_[i].block_force_f_;
      }
  return bf;
}

Real
Simple_spacer::active_springs_stiffness () const
{
  Real den = 0.0;
  for (int i=0; i < springs_.size (); i++)
    if (springs_[i].active_b_)
      {
	den += 1 / springs_[i].hooke_f_;
      }
  return 1/den;
}

void
Simple_spacer::set_active_states ()
{
  // safe, since
  // force is only copied.
  for (int i=0 ; i <springs_.size (); i++)
    if (springs_[i].block_force_f_ >= force_f_) 
      springs_[i].active_b_ = false;
}   

Real
Simple_spacer::configuration_length () const
{
  Real l =0.;
  for (int i=0; i < springs_.size (); i++)
    l += springs_[i].length (force_f_);

  return l;
}

Real
Spring_description::length (Real f) const
{
  if (!active_b_)
    f = block_force_f_;
  return ideal_f_ + f / hooke_f_ ;
}

bool
Simple_spacer::active_b () const
{
   for (int i=0; i < springs_.size (); i++)
    if (springs_[i].active_b_)
      return true;
   return false;
}

void
Simple_spacer::my_solve_linelen ()
{
  while (active_b ())
    {
      force_f_ = active_blocking_force ();
      Real conf = configuration_length ();

      if (conf < line_len_f_)
	{
	  force_f_ +=  (line_len_f_  - conf) * active_springs_stiffness ();
	  break;
	}
      else
	set_active_states ();
    }
}


void
Simple_spacer::my_solve_natural_len ()
{
  while (active_b ())
    {
      force_f_ = active_blocking_force () >? 0.0;

      if (force_f_ < 1e-8) // ugh.,
	break;
      
      set_active_states ();
    }
}

void
Simple_spacer::add_columns (Link_array<Grob> cols)
{
  for (int i=0; i < cols.size () - 1; i++)
    {
      SCM spring_params = SCM_UNDEFINED;
      for (SCM s = Spaceable_element::get_ideal_distances (cols[i]);
	   spring_params == SCM_UNDEFINED && gh_pair_p (s);
	   s = gh_cdr (s))
	{
	  Grob *other = unsmob_grob (gh_caar (s));
	  if (other != cols[i+1])
	    continue;

	  spring_params = gh_cdar (s);
	}

      Spring_description desc;
      if (spring_params != SCM_UNDEFINED)
	{
	  desc.ideal_f_ = gh_scm2double (gh_car (spring_params));
	  desc.hooke_f_ = gh_scm2double (gh_cdr (spring_params));
	}
      else
	{
	  programming_error ("No spring between adjacent columns");
	  desc.hooke_f_ = 1.0;
	  desc.ideal_f_ = default_space_f_;
	}

      if (!desc.sane_b ())
	{
	  programming_error ("Insane spring found. Setting to unit spring.");
	  desc.hooke_f_ = 1.0;
	  desc.ideal_f_ = 1.0;
	}
      
      desc.block_force_f_ = - desc.hooke_f_ * desc.ideal_f_; // block at distance 0
      springs_.push  (desc);
    }
  
  for (int i=0; i < cols.size () - 1; i++)
    {
      for (SCM s = Spaceable_element::get_minimum_distances (cols[i]);
	   gh_pair_p (s); s = gh_cdr (s))
	{
	  Grob * other = unsmob_grob (gh_caar (s));
	  int oi = cols.find_i (other);
	  if (oi >= 0)
	    {
	      add_rod (i, oi, gh_scm2double (gh_cdar (s)));
	    }
	}
    }

  /*
    TODO: should support natural length on only the last line.
   */
  if (line_len_f_ < 0)
    my_solve_natural_len ();
  else
    my_solve_linelen ();
}

void
Simple_spacer::solve (Column_x_positions *positions) const
{
  positions->force_f_ = force_f_;
  
  positions->config_.push (indent_f_);
  for (int i=0; i <springs_.size (); i++)
    {
      positions->config_.push (positions->config_.top () + springs_[i].length (force_f_));
    }

  positions->satisfies_constraints_b_ =  (line_len_f_ < 0) || active_b ();
}



Spring_description::Spring_description( )
{
  ideal_f_ =0.0;
  hooke_f_ =0.0;
  active_b_ = true;
  block_force_f_ = 0.0;
}


bool
Spring_description::sane_b () const
{
  return (hooke_f_ > 0) &&  ! isinf (ideal_f_) && !isnan (ideal_f_);
}


