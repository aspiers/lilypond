/*
  script.cc -- implement Script

  source file of the GNU LilyPond music typesetter

  (c) 1997 Han-Wen Nienhuys <hanwen@stack.nl>
*/

#include "musical-request.hh"
#include "paper-def.hh"
#include "script.hh"
#include "stem.hh"
#include "molecule.hh"
#include "lookup.hh"



void
Script::set_stem(Stem*st_l)
{
    stem_l_ = st_l;
    add_support(st_l);
}


Script::Script(Script_req* rq)
{    
    specs_l_ = rq->scriptdef_p_;
    inside_staff_b_ = specs_l_->inside_b();
    stem_l_ = 0;
    pos_i_ = 0;
    dir_i_ =rq->dir_i_;
}
void
Script::set_default_dir()
{
    int s_i=specs_l_->rel_stem_dir_i();
    if (s_i && stem_l_)
	dir_i_ = stem_l_->dir_i_ * s_i;
    else {
	dir_i_ =specs_l_->staff_dir_i();
    }
}

void
Script::set_default_index()
{
    pos_i_ = get_position_i(specs_l_->get_atom(paper(), dir_i_).extent().y);
}

Interval
Script::do_width() const
{
    return specs_l_->get_atom(paper(), dir_i_).extent().x;
}

void
Script::do_pre_processing()
{
    if (!dir_i_)
	set_default_dir();

}

void
Script::do_post_processing()
{
    set_default_index();
}

Molecule*
Script::brew_molecule_p() const
{
    Real dy = paper()->internote_f();
    
    Molecule*out = new Molecule(specs_l_->get_atom(paper(), dir_i_));
    out->translate_y(dy * pos_i_);
    return out;
}

IMPLEMENT_STATIC_NAME(Script);
IMPLEMENT_IS_TYPE_B2(Script,Item,Staff_side);

int 
Script::compare(Script  *const&l1, Script *const&l2) 
{
    return l1->specs_l_->priority_i() - l2->specs_l_->priority_i();
}
    
