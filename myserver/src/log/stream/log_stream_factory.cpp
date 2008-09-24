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

#include <include/log/stream/log_stream_factory.h>

LogStreamFactory::LogStreamFactory ()
{
  logStreamCreators["file://"] = new FileStreamCreator ();
  logStreamCreators["console://"] = new ConsoleStreamCreator ();
  logStreamCreators["socket://"] = new SocketStreamCreator ();
}

LogStreamFactory::~LogStreamFactory ()
{
  map<string, LogStreamCreator*>::iterator it;
  for (it = logStreamCreators.begin (); 
       it != logStreamCreators.end (); it++)
    {
      delete it->second;
    }
}

LogStream*
LogStreamFactory::createLogStream (FiltersFactory* filtersFactory, 
                                   string location,
                                   list<string>& filters,
                                   u_long cycleLog)
{
  string protocol (getProtocol (location));
  string path (getPath (location));
  if (protocolCheck (protocol))
    {
      return logStreamCreators[protocol]->create (filtersFactory,
                                                  path,
                                                  filters,
                                                  cycleLog);
    }
  return 0;
}

string
LogStreamFactory::getProtocol (string location)
{
  if (location.find ("://") != string::npos)
    return (location.substr (0, location.find("://"))).append ("://");
  return string ("");
}

string
LogStreamFactory::getPath (string location)
{
  if (protocolCheck (getProtocol (location)))
    return location.substr (getProtocol (location).size (), location.size ());
  return string ("");
}

bool
LogStreamFactory::protocolCheck (string protocol)
{
  return logStreamCreators.count (protocol) != 0;
}
