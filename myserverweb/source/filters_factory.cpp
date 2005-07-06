/*
*MyServer
*Copyright (C) 2002,2003,2004 The MyServer Team
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

 
#include "../stdafx.h"
#include "../include/filters_factory.h"

#include <string>
#include <sstream>

using namespace std;


/*!
 *Initialize the object.
 */
FiltersFactory::FiltersFactory()
{
  dictionary.free();
}

/*!
 *Destroy the object.
 */
FiltersFactory::~FiltersFactory()
{

}

/*!
 *Insert a filter by name and factory routine. Returns 0 if the entry
 *was added correctly.
 */
int FiltersFactory::insert(const char* name, FILTERCREATE fnc)
{
  return dictionary.insert(name, fnc);
}

/*!
 *Get a new filter by its name. 
 *The object have to be freed after its use to avoid memory leaks.
 *Returns the new created object on success.
 *Returns 0 on errors.
 */
Filter *FiltersFactory::getFilter(const char* name)
{
  FILTERCREATE factory = dictionary.getData(name);
  /*! If the filter exists create a new object and return it. */
  if(factory)
    return factory(name);

  return 0;
}

/*!
 *Create a FiltersChain starting from a list of strings. 
 *On success returns the new object.
 *If specified [onlyNotModifiers] the method wil check that all the filters
 *will not modify the data.
 *On errors returns 0.
 */
FiltersChain* FiltersFactory::chain(list<string*> l, Stream* out, u_long *nbw, int onlyNotModifiers)
{
  FiltersChain *ret = new FiltersChain();
  list<string*>::iterator i=l.begin();
  if(ret == 0)
    return 0;
  ret->setStream(out);
  *nbw=0;
  for( ; i != l.end(); i++)
  {
    u_long tmp;
    Filter *n=getFilter((*i)->c_str());
    if( !n || ( onlyNotModifiers && n->modifyData() )  )
    {
      ret->clearAllFilters();
      return 0;
    }
    ret->addFilter(n, &tmp);
    *nbw+=tmp;
  }
  return ret;
}

/*!
 *free the object.
 */
void FiltersFactory::free()
{
  dictionary.free();
}
