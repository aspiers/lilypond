/*
  dstream.hh -- declare Dstream

  source file of the Flower Library

  (c) 1997 Han-Wen Nienhuys <hanwen@stack.nl>
*/

#ifndef DSTREAM_HH
#define DSTREAM_HH

#include "string.hh"

const char eol= '\n';

template<class K,class V>
struct Assoc;

/**  debug stream. 
   a class for providing debug output of nested structures,
   with indents according to \{\}()[].

   One can turn on and off specific messages using the Assoc silent.
   This can be done automatically:

   #define DEBUG  dstream_.identify_as(__PRETTY_FUNCTION__) 

   DEBUG << "a message\n";
   
   Init for the class names which should be silent can be given in a rc file. 
   
   TODO:
   make a baseclass for indentable streams.
  */
class Dstream
{
    ostream *os_l_;
    int indent_level_i_;
    bool local_silence_b_;
    String current_classname_str_;
    void output(String s);
    Assoc<String, bool> *silent_assoc_p_;
public:
    void clear_silence();
    bool silence(String);
    
    /**
      if rcfile == 0, then do not read any rc file.
      */
      
    Dstream(ostream *r, char const * rcfile);
    virtual ~Dstream();
    Dstream &identify_as(String s);

    Dstream &operator << (String s);
    /**
      Output memory locations.
     */
    Dstream &operator << (void const *);
    Dstream &operator << (char const *);
};
#endif

