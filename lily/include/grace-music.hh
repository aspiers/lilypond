/*   
  grace-music.hh -- declare Grace_music
  
  source file of the GNU LilyPond music typesetter
  
  (c)  1999--2003 Han-Wen Nienhuys <hanwen@cs.uu.nl>
  
 */

#ifndef NGRACE_MUSIC_HH
#define NGRACE_MUSIC_HH

#include "music-wrapper.hh"

class Grace_music : public Music_wrapper
{
public:
  VIRTUAL_COPY_CONS (Music);
  Grace_music ();
protected:
  virtual void compress (Moment);
  virtual Moment get_length () const;
  virtual Moment start_mom () const;
};

#endif /* GRACE_MUSIC_HH */

