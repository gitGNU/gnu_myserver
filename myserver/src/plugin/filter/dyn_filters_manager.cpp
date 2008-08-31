/*
MyServer
Copyright (C) 2005, 2006, 2007, 2008 Free Software Foundation, Inc.
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "stdafx.h"
#include <include/filter/stream.h>
#include <include/filter/filter.h>
#include <include/plugin/filter/dyn_filter.h>
#include <include/find_data/find_data.h>
#include <include/server/server.h>
#include <include/plugin/filter/dyn_filters_manager.h>
#include <include/filter/filter.h>
#include <string>
#include <sstream>

using namespace std;

/*!
 *Construct the object.
 */
DynamicFiltersManager::DynamicFiltersManager() : 
  PluginsNamespaceManager(string("filters"))
{
  counter = 0;
  counterMutex.init();
}

/*!
 *Destroy the object.
 */
DynamicFiltersManager::~DynamicFiltersManager()
{
  clear();
}

/*!
 *Clear everything.
 */
void DynamicFiltersManager::clear()
{
  counterMutex.destroy();  
}

/*!
 *Factory method to create a dynamic filter. Return the new Filter on success.
 */
Filter* DynamicFiltersManager::createFilter(const char* name)
{
  DynamicFilterFile* file;
  DynamicFilter* filter;
  string nameStr(name);

  file = getPlugin(nameStr);

  if(!file)
    return 0;

  filter = new DynamicFilter(file);

  if(!file)
    return 0;

  counterMutex.lock();
  filter->setId(counter++);
  counterMutex.unlock();

  return filter; 
}

/*!
 *Register the loaded filters on the FiltersFactory object.
 */
int DynamicFiltersManager::registerFilters(FiltersFactory* ff)
{
  HashMap<string, Plugin*>::Iterator it = begin();
  
  for (;it != end(); it++)
  {
    DynamicFilterFile* dff = (DynamicFilterFile*) *it;
    if(ff->insert(dff->getName(0, 0), this))
      return -1;
  }
  return 0;
}
