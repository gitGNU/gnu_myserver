/*
*MyServer
*Copyright (C) 2002 The MyServer Team
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

#ifndef GZIP_H
#define GZIP_H
#include "../stdafx.h"

#ifndef DO_NOT_USE_GZIP	/*If is defined DO_NOT_USE_GZIP don't use the zlib library */
#include "zlib.h"		/*Include the ZLIB library header file*/
#endif

#ifdef DO_NOT_USE_GZIP	
#define z_stream (void*)
#endif

#define GZIP_HEADER_LENGTH		10
#define GZIP_FOOTER_LENGTH		8
extern char GZIP_HEADER[];
class gzip
{
public:
	struct gzip_data
	{
		z_stream stream;
		u_long crc;
		u_long data_size;
		u_long initialized;
	};
	u_long gzip_updateCRC(char* buffer,int size,gzip_data* data);
	u_long gzip_getFOOTER(char *str,int size,gzip_data* data);
	u_long gzip_initialize(char* in,u_long sizeIN,char *out,u_long sizeOUT,gzip_data* data);
	u_long gzip_compress(char* in,u_long sizeIN,char *out,u_long sizeOUT,gzip_data* data);
	u_long gzip_free(char* in,u_long sizeIN,char *out,u_long sizeOUT,gzip_data* data);
	u_long gzip_flush(char *out,u_long sizeOUT,gzip_data* data);
private:
	gzip_data data;
};
#endif
