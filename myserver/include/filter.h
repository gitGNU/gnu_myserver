/*
MyServer
Copyright (C) 2005, 2007 The MyServer Team
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

#ifndef FILTER_H
#define FILTER_H
#include "../stdafx.h"
#include "../include/stream.h"
#include "../include/protocol.h"

/*!
 *Abstract class to handle virtual data filters.
 */
class Filter : public Stream
{
protected:
  Protocol *protocol;
  Stream *parent;
  void* protocolData;
public:
  Protocol* getProtocol()
  {
    return protocol;
  }
  void setProtocol(Protocol* pr)
  {
    protocol = pr;
  }
  void* getProtocolData()
  {
    return protocolData;
  }
  void setProtocolData(void* prd)
  {
    protocolData = prd;
  }
  virtual int getHeader(char* buffer, u_long len, u_long* nbw);
  virtual int getFooter(char* buffer, u_long len, u_long* nbw);
  virtual int read(char* buffer, u_long len, u_long*);
  virtual int write(const char* buffer, u_long len, u_long*);
	virtual int flush(u_long*);
	virtual int modifyData();
  virtual const char* getName(char*, u_long);
  void setParent(Stream*);
  Stream* getParent();
  Filter();
  /*! Avoid direct instances of this class. */
  virtual ~Filter()=0;
};


#endif
