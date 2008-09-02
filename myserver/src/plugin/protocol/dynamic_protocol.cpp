/*
MyServer
Copyright (C) 2002, 2003, 2004, 2007, 2008 Free Software Foundation, Inc.
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


#include <include/plugin/protocol/protocols_manager.h>
#include <include/base/xml/xml_parser.h>
#include <include/server/server.h>

#include <string>


typedef int (*controlConnectionPROC)(void*, char*, char*, int, 
                                     int, u_long, u_long); 



/*!
 *Get the options for the protocol.
 */
int DynamicProtocol::getOptions()
{
  return  protocolOptions;
}

/*!
 *Control the connection.
 */
int DynamicProtocol::controlConnection(ConnectionPtr a, char *b1, char *b2,
                                       int bs1, int bs2, 
                                       u_long nbtr, u_long id)
{
  controlConnectionPROC proc;
  proc = (controlConnectionPROC)hinstLib.getProc("controlConnection"); 

  if(proc)
    return proc((void*)a, b1, b2, bs1, bs2, nbtr, id);
  else
    return 0;
}


/*!
 *Constructor for the class protocol.
 */
DynamicProtocol::DynamicProtocol()
{
  protocolOptions = 0;
}

/*!
 *Destroy the protocol object.
 */
DynamicProtocol::~DynamicProtocol()
{
  unLoadProtocol(0);
  protocolOptions = 0;
}
