/*
*MyServer
*Copyright (C) 2005 The MyServer Team
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef HASH_DICTIONARY_H
#define HASH_DICTIONARY_H

#include <map>

using namespace std;

/*! Handle a dictionary of strings trough hashing.*/
template<typename T> class HashDictionary
{
private:
  struct sNode
  {
    unsigned int hash;
    T data;
    sNode()
      {hash=0;}
    sNode(unsigned int h, T t)
      {hash=h; data=t;}
  };
  map<int, sNode*> data;
  static unsigned int hash(const char *);
public:
  int clone(HashDictionary<T>& hd);
  HashDictionary();
  explicit HashDictionary(HashDictionary<T>&);
  ~HashDictionary();
  T getData(const char*);
  T getData(int);
  int insert(const char*, T);
  T removeNode(const char*);
  T removeNodeAt(int);
  int nodesNumber();
  int size()
    {return nodesNumber(); }
  void free();
  void clear()
    {free();}
  int isEmpty();
};

/*! Needed when exporting templates. */
#ifndef HASH_DICTIONARY_H_NO_SRC
#include "../source/hash_dictionary.cpp"
#endif


#endif
