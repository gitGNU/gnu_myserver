/*
MyServer
Copyright (C) 2005, 2006, 2007 The MyServer Team
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "../stdafx.h"
#include "../include/stream.h"
#include "../include/filter.h"
#include "../include/dyn_filter_file.h"
#include "../include/lfind.h"
#include "../include/server.h"
#include <string>
#include <sstream>

using namespace std;

typedef int (*getHeaderPROC)(u_long, void*, char*, u_long, u_long*); 
typedef int (*getFooterPROC)(u_long, void*, char*, u_long, u_long*);
typedef int (*readPROC)(u_long, void*, char*, u_long, u_long*);
typedef int (*writePROC)(u_long, void*, const char*, u_long, u_long*);
typedef int (*flushPROC)(u_long, void*, u_long*);
typedef int (*modifyDataPROC)(u_long, void*);


/*!
 *Construct the object.
 */
DynamicFilterFile::DynamicFilterFile()
{

}

/*!
 *Destroy the object.
 */
DynamicFilterFile::~DynamicFilterFile()
{

}

/*!
 *Get the filter header.
 */
int DynamicFilterFile::getHeader(u_long id, Stream* s, char* buffer, 
                                 u_long len, u_long* nbw)
{
  getHeaderPROC proc;
  if(!hinstLib.validHandle())
    return -1;
  proc = (getHeaderPROC) hinstLib.getProc("getHeader");
  if(proc)
    return proc(id, s, buffer, len, nbw);

  return -1;
}

/*!
 *Get the filter footer.
 */
int DynamicFilterFile::getFooter(u_long id, Stream* s, char* buffer, 
                                 u_long len, u_long* nbw)
{
  getFooterPROC proc;
  if(!hinstLib.validHandle())
    return -1;
  proc = (getFooterPROC) hinstLib.getProc("getFooter");
  if(proc)
    return proc(id, s, buffer, len, nbw);

  return -1;
}

/*!
 *Read using the filter from the specified stream.
 */
int DynamicFilterFile::read(u_long id, Stream* s, char* buffer, 
                            u_long len, u_long* nbr)
{
  readPROC proc;
  if(!hinstLib.validHandle())
    return -1;
  proc = (readPROC) hinstLib.getProc("filteredRead");
  if(proc)
    return proc(id, s, buffer, len, nbr);

  return -1;
}

/*!
 *Write to the stream using the filter.
 */
int DynamicFilterFile::write(u_long id, Stream* s, const char* buffer, 
                             u_long len, u_long* nbw)
{
  writePROC proc;
  if(!hinstLib.validHandle())
    return -1;
  proc = (writePROC) hinstLib.getProc("filteredWrite");
  if(proc)
    return proc(id, s, buffer, len, nbw);

  return -1;
}

/*!
 *Flush remaining data to the stream.
 */
int DynamicFilterFile::flush(u_long id, Stream* s, u_long* nbw)
{
  flushPROC proc;
  if(!hinstLib.validHandle())
    return -1;
  proc = (flushPROC) hinstLib.getProc("filteredFlush");
  if(proc)
    return proc(id, s, nbw);

  return -1;
}

/*!
 *Check if the filter modifies the data.
 */
int DynamicFilterFile::modifyData(u_long id, Stream* s)
{
  modifyDataPROC proc;
  if(!hinstLib.validHandle())
    return -1;
  proc = (modifyDataPROC) hinstLib.getProc("modifyData");
  if(proc)
    return proc(id, s);

  return -1;
}
