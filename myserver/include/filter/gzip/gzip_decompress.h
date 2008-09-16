/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2002, 2003, 2004 Free Software Foundation, Inc.
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

#ifndef GZIP_DECOMPRESS_H
#define GZIP_DECOMPRESS_H
#include "stdafx.h"
#include <include/filter/filter.h>
#include <include/filter/gzip/gzip.h>

/*! If is defined DO_NOT_USE_GZIP don't use the zlib library. */
#ifndef DO_NOT_USE_GZIP	
/*! Include the ZLIB library header file. */
#include "zlib.h"		
#endif

#ifdef DO_NOT_USE_GZIP	
#define z_stream (void*)
#endif


class GzipDecompress : public Filter
{
public:
	struct GzipData
	{
		z_stream stream;
		u_long crc;
		u_long data_size;
		u_long initialized;
	};
	

  GzipDecompress();
  ~GzipDecompress();

	u_long updateCRC(char* buffer,int size);
	u_long getFooter(char *str,int size);
	u_long initialize();
	u_long decompress(const char* in, u_long sizeIn, 
										char *out,u_long sizeOut);
	u_long free();
	u_long flush(char *out,u_long sizeOut);
	u_long getHeader(char *buffer,u_long buffersize);

  static Filter* factory(const char* name);

	static u_long headerSize();
	static u_long footerSize();
 
  /*! From Filter*/
  virtual int getHeader(char* buffer, u_long len, u_long* nbw);
  virtual int getFooter(char* buffer, u_long len, u_long* nbw);
  virtual int read(char* buffer, u_long len, u_long*);
  virtual int write(const char* buffer, u_long len, u_long*);
	virtual int flush(u_long*);
	virtual int modifyData();
  virtual const char* getName(char* name, u_long len);
private:
	int active;
	GzipData data;

};

#endif
