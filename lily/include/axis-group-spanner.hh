/*
  axis-group-spanner.hh -- declare Axis_group_spanner

  source file of the GNU LilyPond music typesetter

  (c)  1997--1999 Han-Wen Nienhuys <hanwen@cs.uu.nl>
*/


#ifndef SPAN_AXIS_GROUP_HH
#define SPAN_AXIS_GROUP_HH

#include "spanner.hh"
#include "axis-group-element.hh"

/** An element which groups a line in a certain direction. The most
  useful example of this is the Vertical_group_spanner */
class Axis_group_spanner : public Spanner, public virtual Axis_group_element
{
  void do_break_processing_if_unbroken();
protected:
  virtual void do_break_processing();
  virtual void do_print() const;

public:
  
};

#endif // SPAN_AXIS_GROUP_HH
