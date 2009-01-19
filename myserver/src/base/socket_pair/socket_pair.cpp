/*
MyServer
Copyright (C) 2009 Free Software Foundation, Inc.
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
#include <include/base/utility.h>
#include <include/base/socket_pair/socket_pair.h>

#ifdef NOT_WIN
extern "C" {
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
}
#else
#include <windows.h>
#endif

#include <string>
#include <sstream>

using namespace std;



SocketPair::SocketPair ()
{

}

/*!
 *Create a Socket pair.
 *\return 0 on success.
 */
int SocketPair::create ()
{
#ifdef WIN32
#define LOCAL_SOCKETPAIR_AF AF_INET
#else
#define LOCAL_SOCKETPAIR_AF AF_UNIX
#endif

  int af = LOCAL_SOCKETPAIR_AF;
  int type = SOCK_STREAM;
  int protocol = 0;

#ifndef WIN32
  return socketpair (af, type, protocol, (int*)handles);
#else
  struct sockaddr_in addr;
  SOCKET listener;
  int e;
  int addrlen = sizeof(addr);
  DWORD flags = 0;
  
  if (handles == 0)
    return -1;

  handles[0] = handles[1] = INVALID_SOCKET;
  listener = socket (AF_INET, type, 0);

  if (listener == INVALID_SOCKET)
    return -1;

  memset(&addr, 0, sizeof (addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl (0x7f000001);
  addr.sin_port = 0;

  e = bind(listener, (const struct sockaddr*) &addr, sizeof(addr));
  if (e == SOCKET_ERROR)
    {
      close(listener);
      return -1;
    }
  
  e = getsockname(listener, (struct sockaddr*) &addr, &addrlen);
  if (e == SOCKET_ERROR)
    {
      close (listener);
      return -1;
    }
  
  do
    {
      if (listen (listener, 1) == SOCKET_ERROR)
        break;
      if ((handles[0] = socket (AF_INET, type, 0)) == INVALID_SOCKET)
        break;
      if (connect(handles[0], (const struct sockaddr*) &addr, sizeof (addr)) == SOCKET_ERROR)
        break;
      if ((handles[1] = accept (listener, NULL, NULL)) == INVALID_SOCKET)
        break;
    
      close (listener);
      return 0;
    } while (0);
  
  close (listener);
  close (handles[0]);
  close (handles[1]);
  
  return -1;
#endif

  return 0;
}

/*!
 *Get the first handle used by the socket pair.
 */
FileHandle SocketPair::getFirstHandle ()
{
  return handles[0];
}

/*!
 *Get the second handle used by the socket pair.
 */
FileHandle SocketPair::getSecondHandle ()
{
  return handles[1];
}


/*!
 *Invert the current socket pair on the passed argument.
 *The first handle will be the second handle on the inverted SocketPair
 *and the second handle will be the first one.
 *\param inverted It will be the inverted SocketPair.
 */
void SocketPair::inverted (SocketPair& inverted)
{
  inverted.handles[0] = handles[1];
  inverted.handles[1] = handles[0];
}

/*!
 *Read using the first handle.
 *\see Stream#read.
 */
int SocketPair::read (char* buffer, u_long len, u_long *nbr)
{
  Socket sock (handles[0]);
  return sock.read (buffer, len, nbr);

}

/*!
 *Write using the first handle.
 *\see Stream#write.
 */
int SocketPair::write (const char* buffer, u_long len, u_long *nbw)
{
  Socket sock (handles[0]);
  return sock.write (buffer, len, nbw);
}

/*!
 *Close both handles.
 */
int SocketPair::close ()
{
  closeFirstHandle ();
  closeSecondHandle ();

  return 0;
}

/*!
 *Close the first handle.
 */
void SocketPair::closeFirstHandle ()
{
  Socket sock (handles[0]);
  sock.close ();
}

/*!
 *Close the second handle.
 */
void SocketPair::closeSecondHandle ()
{
  Socket sock (handles[1]);
  sock.close ();
}
