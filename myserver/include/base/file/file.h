/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2002, 2003, 2004, 2008, 2009 Free Software Foundation, Inc.
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

#ifndef FILE_H
#define FILE_H

#include "stdafx.h"
#include <include/filter/stream.h>
#include <string>

#include <include/base/socket/socket.h>
#include <include/base/mem_buff/mem_buff.h>

using namespace std;

class File : public Stream
{
public:
  static const u_long READ;
  static const u_long WRITE;
  static const u_long TEMPORARY;
  static const u_long HIDDEN;
  static const u_long FILE_OPEN_ALWAYS;
  static const u_long OPEN_IF_EXISTS;
  static const u_long APPEND;
  static const u_long FILE_CREATE_ALWAYS;
  static const u_long NO_INHERIT;

  File();
  File(char *,int);
  virtual Handle getHandle();
  virtual int setHandle(Handle);
  virtual int writeToFile(const char* ,u_long ,u_long* );
  virtual int createTemporaryFile(const char* );

  virtual int openFile(const char*, u_long );
  virtual int openFile(string const &file, u_long opt)
    {return openFile(file.c_str(), opt);}

  virtual u_long getFileSize();
  virtual int seek (u_long);
  virtual u_long getSeek ();

  virtual time_t getLastModTime();
  virtual time_t getCreationTime();
  virtual time_t getLastAccTime();
  virtual const char *getFilename();
  virtual int setFilename(const char*);
  virtual int setFilename(string const &name)
    {return setFilename(name.c_str());}

  virtual int operator =(File);
  virtual int close();

  /*! Inherithed from Stream. */
  virtual int read(char* buffer, u_long len, u_long *nbr);
  virtual int write(const char* buffer, u_long len, u_long *nbw);

  virtual int fastCopyToSocket (Socket *dest, u_long offset, 
                                MemBuf *buf, u_long *nbw);
protected:
  FileHandle handle;
  string filename;
};
#endif
