/*
MyServer
Copyright (C) 2005, 2007 Free Software Foundation, Inc.
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

#include "../stdafx.h"
#include "../include/isapi.h"
#include "../include/stream.h"
#include "../include/filter.h"

#include <string>
#include <sstream>

using namespace std;

/*!
 *Read [len] characters using the filter. Returns -1 on errors.
 *\argument buffer The buffer where receive read data.
 *\param len The buffer length in bytes.
 *\param nbr A pointer to receive the number of read bytes.
 */
int Filter::read(char* buffer, u_long len, u_long *nbr)
{
  *nbr=0;
  return 0;
}

/*!
 *Write [len] characters to the stream. Returns -1 on errors.
 *\param buffer The buffer with the data to write.
 *\param len Number of bytes to use.
 *\param nbw A pointer to receive the number of written bytes.
 */
int Filter::write(const char* buffer, u_long len, u_long* nbw)
{
  *nbw = 0;
  return 0;
}

/*!
 *Get an header for the filter. Returns -1 on errors.
 *\param buffer The buffer where receive read data.
 *\param len The buffer length in bytes.
 *\param nbr A pointer to receive the number of read bytes.
 */
int Filter::getHeader(char* buffer, u_long len, u_long* nbr)
{
  *nbr = 0;
  return 0;
}

/*!
 *Get a footer for the filter. Returns -1 on errors.
 *\param buffer The buffer where receive read data.
 *\param len The buffer length in bytes.
 *\param nbr A pointer to receive the number of read bytes.
 */
int Filter::getFooter(char* buffer, u_long len, u_long* nbr)
{
  *nbr = 0;
  return 0;
}

/*!
 *Default constructor.
 */
Filter::Filter()
{
  protocol = 0;
  protocolData = 0;
  parent = 0;
}


/*!
 *Default destructor.
 */
Filter::~Filter()
{

}

/*!
 *Set the stream where apply the filter.
 *\param parentStream The stream to use as parent.
 */
void Filter::setParent(Stream* parentStream)
{
  parent = parentStream;
}

/*!
 *Flush everything to the stream. Returns -1 on errors.
 *\param nbw A pointer to receive the number of read bytes.
 */
int Filter::flush(u_long *nbw)
{
  *nbw = 0;
  return 0;
}

/*!
 *Get the stream used by the filter.
 */
Stream* Filter::getParent()
{
  return parent;
}

/*!
 *Returns a nonzero value if the filter modify the input/output data.
 */
int Filter::modifyData()
{
  return 0;
}

/*!
 *Return a string with the filter name. 
 *If an external buffer is provided write the name there too.
 *\param name The buffer where write the filter name.
 *\param len The buffer length.
 */
const char* Filter::getName(char* name, u_long len)
{
  /* No name by default.  */
  if(name)
  {
    name[0] = '\0';
  }
  return "\0";
}
