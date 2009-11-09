/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2006, 2008, 2009 Free Software Foundation, Inc.
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

#ifndef MEMBUF_FILE_H
# define MEMBUF_FILE_H

# include "stdafx.h"
# include <include/filter/stream.h>
# include <include/base/file/file.h>
# include <string>

# include <include/base/mem_buff/mem_buff.h>

using namespace std;

class MemBufFile : public File
{
public:
  MemBufFile (MemBuf* buffer);
  virtual Handle getHandle ();
  virtual int setHandle (Handle);
  virtual int read (char* ,u_long ,u_long*);
  virtual int writeToFile (const char* ,u_long ,u_long*);
  virtual int createTemporaryFile (const char*);

  virtual int openFile (const char*, u_long);
  virtual int openFile (string const &file, u_long opt)
  {return openFile (file.c_str (), opt);}

  virtual u_long getFileSize ();
  virtual int seek (u_long);

  virtual int close ();

  virtual int fastCopyToSocket (Socket *dest, u_long offset,
                                MemBuf *buf, u_long *nbw);

  virtual u_long getSeek ();
  virtual int write (const char* buffer, u_long len, u_long *nbw);
protected:
  u_long fseek;
  MemBuf *buffer;
};
#endif
