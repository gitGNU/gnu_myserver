/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2006, 2008 Free Software Foundation, Inc.
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

#ifndef SOCKET_STREAM_CREATOR_H
#define SOCKET_STREAM_CREATOR_H

#include <include/log/stream/log_stream_creator.h>
#include <include/log/stream/socket_stream.h>

#include <stdlib.h>

class SocketStreamCreator : public LogStreamCreator
{
 public:
  virtual LogStream* create (FiltersFactory* filtersFactory, 
			     string& location, 
			     list<string>& filters,
			     u_long cycleLog);


  /*!
   * A helper method to get the port part of an URL.
   */
  u_short getPort (string& location);


  /*!
   * A helper method to get the host part of an URL.
   */
  string getHost (string& location);
};

#endif
