/*
  path.hh -- declare File_path

  source file of the Flower Library

  (c)  1997--2000 Han-Wen Nienhuys <hanwen@cs.uu.nl>
*/

#ifndef PATH_HH
#define PATH_HH
#include "string.hh"
#include "array.hh"


/**    
  search in directories for a file.

   Abstraction of PATH variable. An interface for searching input files.
   Search a number of dirs for a file.

   TODO: add a unix style PATH interface 
   Should use kpathsea?
   
*/

class Path
{
public:
  String root;
  String dir;
  String base;
  String ext;

  String path () const;
};

class File_path : private Array<String>
{
public:
  String find (String nm) const;

  Array<String>::push;
  String str ()const;
  bool try_add (String str);
  void add (String);
  void parse_path (String);
};

/** split a path into its components.

  @params path

  @return
  String &drive, String &dirs, String &filebase, String &extension
 */
Path split_path (String path);

#endif
