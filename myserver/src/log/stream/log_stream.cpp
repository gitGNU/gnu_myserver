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

#include <include/log/stream/log_stream.h>

LogStream::LogStream (FiltersFactory* ff, u_long cycle, Stream* out,
                      FiltersChain* fc) : cycle (cycle), isOpened (1)
{
  this->ff = ff;
  this->out = out;
  this->fc = fc;
  mutex = new Mutex ();
  mutex->init ();
}

LogStream::~LogStream ()
{
  if (isOpened)
    close ();
  fc->clearAllFilters ();
  delete out;
  delete fc;
  delete mutex;
}

int
LogStream::resetFilters ()
{
  list<string> filters (fc->getFilters ());
  fc->clearAllFilters ();
  return ff->chain (fc, filters, out, &nbw);
}

int
LogStream::log (string message)
{
  mutex->lock ();
  int success = 0;
  if (needToCycle ())
    {
      success = doCycle () || write (message);
    }
  else
    {
      success = write (message);
    }
  mutex->unlock ();
  return success;
}

int
LogStream::needToCycle ()
{
  return cycle && (streamSize () >= cycle);
}

int
LogStream::doCycle ()
{
  return fc->flush (&nbw) || streamCycle () || resetFilters ();
}

int
LogStream::write (string message)
{
  return fc->write (message.c_str (), message.size (), &nbw);
}

int
LogStream::setCycle (u_long cycle)
{
  mutex->lock ();
  this->cycle = cycle;
  mutex->unlock ();
  return 0;
}

u_long
LogStream::getCycle ()
{
  return cycle;
}

Stream*
LogStream::getOutStream ()
{
  return out;
}

FiltersChain*
LogStream::getFiltersChain ()
{
  return fc;
}

int
LogStream::close ()
{
  mutex->lock ();
  int success = 1;
  if (isOpened)
    {
      if (!(isOpened = (fc->flush (&nbw) || out->close ())))
        {
          success = 0;
        }
    }
  mutex->unlock ();
  return success;
}

int
LogStream::update (LogStreamEvent evt, void* message, void* reply)
{
  switch (evt)
    {
    case EVT_SET_CYCLE:
      {
        return setCycle (*static_cast<u_long*>(message));
      }
      break;
    case EVT_LOG:
      {
        return log (*static_cast<string*>(message));
      }
      break;
    case EVT_CLOSE:
      {
        return close ();
      }
      break;
    case EVT_ADD_FILTER:
      {
        return addFilter (static_cast<Filter*>(message));
      }
      break;
    case EVT_CHOWN:
      {
        return chown (static_cast<int*>(message)[0], 
                      static_cast<int*>(message)[1]);
      }
    case EVT_ENTER_ERROR_MODE:
      {
        return enterErrorMode ();
      }
      break;
    case EVT_EXIT_ERROR_MODE:
      {
        return exitErrorMode ();
      }
      break;
    default:
      return 1;
    }
}

int
LogStream::addFilter (Filter* filter)
{
  mutex->lock ();
  int success = fc->addFilter (filter, &nbw);
  mutex->unlock ();
  return success;
}

int
LogStream::removeFilter (Filter* filter)
{
  mutex->lock ();
  int success = fc->removeFilter (filter);
  mutex->unlock ();
  return success;
}

FiltersFactory const*
LogStream::getFiltersFactory ()
{
  return ff;
}

u_long
LogStream::streamSize ()
{
  return 0;
}

int
LogStream::streamCycle ()
{
  return 0;
}

int
LogStream::getIsOpened ()
{
  return isOpened;
}

u_long
LogStream::getLatestWrittenBytes ()
{
  return nbw;
}

list<string>&
LogStream::getCycledStreams ()
{
  return cycledStreams;
}

int
LogStream::chown (int uid, int gid)
{
  return 0;
}

int
LogStream::enterErrorMode ()
{
  return 0;
}

int
LogStream::exitErrorMode ()
{
  return 0;
}
