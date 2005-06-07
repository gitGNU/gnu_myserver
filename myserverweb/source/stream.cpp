/*
*MyServer
*Copyright (C) 2002,2003,2004 The MyServer Team
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

 
#include "../include/isapi.h"
#include "../include/stream.h"

#include <string>
#include <sstream>
using namespace std;

/*!
 *Read [len] characters from the stream. Returns -1 on errors.
 */
u_long Stream::read(char* buffer, int len)
{
  return static_cast<u_long>(-1);
}

/*!
 *Write [len] characters to the stream. Returns -1 on errors.
 */
u_long Stream::write(char* buffer, int len)
{
  return static_cast<u_long>(-1);
}

Stream::Stream()
{

}

Stream::~Stream()
{

}
