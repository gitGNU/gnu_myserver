/*
*MyServer
*Copyright (C) 2002,2003,2004 The MyServer Team
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "../include/gzip.h"
#include "../include/securestr.h"

extern "C" {
#ifdef WIN32
#include <direct.h>
#include <errno.h>
#pragma comment (lib,"libz.lib")
#endif
#ifdef NOT_WIN
#include <string.h>
#include <errno.h>
#endif
}
#ifdef WIN32
#include <algorithm>
#endif
#ifdef NOT_WIN
#include "../include/lfind.h"

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

// Bloodshed Dev-C++ Helper
#ifndef intptr_t
#define intptr_t int
#endif


char GZIP_HEADER[]={(char)0x1f,(char)0x8b,Z_DEFLATED,0,0,0,0,0,0,0x03};


/*!
*Initialize the gzip structure value.
*/
u_long Gzip::initialize()
{
#ifndef DO_NOT_USE_GZIP		
	long level = Z_DEFAULT_COMPRESSION;

	data.initialized=1;
	data.data_size=0;
	data.crc = crc32(0L, Z_NULL, 0);
	data.stream.zalloc = Z_NULL;
	data.stream.zfree = Z_NULL;
	data.stream.opaque = Z_NULL;
	data.stream.data_type=Z_BINARY;
	return deflateInit2(&(data.stream), level, Z_DEFLATED,-MAX_WBITS, MAX_MEM_LEVEL,0);
#else 
	return 0;
#endif

};

#ifdef GZIP_CHECK_BOUNDS	
u_long Gzip::compressBound(int size)
{
#ifdef compressBound 
	return compressBound(size);
#else
	return 0;
#endif	
}
#endif

/*!
*Compress the in buffer to the out buffer using the gzip compression.
*/
u_long Gzip::compress(const char* in, u_long sizeIn, 
                      char *out, u_long sizeOut)
{
#ifndef DO_NOT_USE_GZIP	
	u_long old_total_out=data.stream.total_out;
	uLongf destLen=sizeOut;
	u_long ret;

#ifdef GZIP_CHECK_BOUNDS
	if(compressBound(sizeIn)>(u_long)sizeOut)
		return 0;
#endif
	data.stream.data_type=Z_BINARY;
	data.stream.next_in = (Bytef*) in;
	data.stream.avail_in = sizeIn;
	data.stream.next_out = (Bytef*) out;
	data.stream.avail_out = destLen;
	ret = deflate(&(data.stream), Z_FULL_FLUSH);

	data.data_size+=data.stream.total_out-old_total_out;
	data.crc = crc32(data.crc, (const Bytef *) in, sizeIn);
	return data.stream.total_out-old_total_out;
#else 
	/*!
   *If is specified DO_NOT_USE_GZIP copy the input buffer to the output one as it is.
   */
	memcpy(out, in, std::min(sizeIn, sizeOut));
	return std::min(sizeIn, sizeOut);
#endif
}

/*!
 *Close the gzip compression.
 */
u_long Gzip::free()
{
  u_long ret=0;
#ifndef DO_NOT_USE_GZIP
	
	if(data.initialized==0)
		return 0;
	data.initialized=0;
	ret = deflateEnd(&(data.stream));
#endif
	return ret;
}

/*! 
 *Inherited from Filter.
 */
int Gzip::getHeader(char* buffer, u_long len, u_long* nbw)
{
  *nbw = getHeader(buffer, len);
  return !(*nbw);
}

/*! 
 *Inherited from Filter.
 */
int Gzip::getFooter(char* buffer, u_long len, u_long* nbw)
{
  if(len < GZIP_FOOTER_LENGTH)
    return -1;
  getFooter(buffer, GZIP_FOOTER_LENGTH);
  *nbw = GZIP_FOOTER_LENGTH;
  return 0;

}

/*!
 *The Gzip filter modifies the data. 
 */
int Gzip::modifyData()
{
  return 1;
}

/*!
 *Flush all the remaining data.
 */
u_long Gzip::flush(char *out, u_long sizeOut)
{
#ifndef DO_NOT_USE_GZIP	
	u_long old_total_out=data.stream.total_out;
	uLongf destLen=sizeOut;

	data.stream.data_type=Z_BINARY;
	data.stream.next_in = 0;
	data.stream.avail_in = 0;
	data.stream.next_out = (Bytef*) out;
	data.stream.avail_out = destLen;
	deflate(&(data.stream), Z_FINISH);

	data.data_size+=data.stream.total_out-old_total_out;
	return data.stream.total_out-old_total_out;
#else 
	return 0;
#endif
}

/*! Constructor for the class. */
Gzip::Gzip()
{
  initialize();
}

/*! Destructor for the class. */
Gzip::~Gzip()
{
  free();
}

/*!
 *Update the existent CRC.
 */
u_long Gzip::updateCRC(char* buffer, int size)
{
#ifndef DO_NOT_USE_GZIP		
	data.crc = crc32(data.crc, (const Bytef *) buffer,(u_long)size);
	return data.crc;
#else
	return 0;
#endif
}

/*!
 *Get the GZIP footer.
 */
u_long Gzip::getFooter(char *str,int /*size*/)
{
#ifndef DO_NOT_USE_GZIP		
	char *footer =  str;
	footer[0] = (char) (data.crc) & 0xFF;
	footer[1] = (char) ((data.crc) >> 8) & 0xFF;
	footer[2] = (char) ((data.crc) >> 16) & 0xFF;
	footer[3] = (char) ((data.crc) >> 24) & 0xFF;
	footer[4] = (char) data.stream.total_in & 0xFF;
	footer[5] = (char) (data.stream.total_in >> 8) & 0xFF;
	footer[6] = (char) (data.stream.total_in >> 16) & 0xFF;
	footer[7] = (char) (data.stream.total_in >> 24) & 0xFF;
	footer[8] = '\0';
	return GZIP_FOOTER_LENGTH;
#else
	return 0;
#endif
}

/*!
 *Copy the GZIP header in the buffer.
 */
u_long Gzip::getHeader(char *buffer,u_long buffersize)
{
	if(buffersize<GZIP_HEADER_LENGTH)
		return 0;
	memcpy(buffer, GZIP_HEADER, GZIP_HEADER_LENGTH);
	return GZIP_HEADER_LENGTH;
}

/*! 
 *Inherited from Filter.
 *This function uses an internal buffer slowing it. 
 *It is better to use directly the Gzip::compress routine where possible.
 */
int Gzip::read(char* buffer, u_long len, u_long *nbr)
{
  char *tmp_buff;
  int ret;
  u_long nbr_parent;
  if(!parent)
    return -1;
  tmp_buff = new char[len/2];
  if(!tmp_buff)
    return -1; 
  
  ret = parent->read(tmp_buff, len/2, &nbr_parent);

  if(ret == -1)
  {
    delete [] tmp_buff;
    return  -1;
  }
  *nbr = compress(tmp_buff, nbr_parent, buffer, len);
  delete [] tmp_buff;
  return 0;
}

/*! 
 *Inherited from Filter.
 */
int Gzip::write(const char* buffer, u_long len, u_long *nbw)
{
  char tmpBuffer[1024];
  u_long written=0;
  *nbw=0;

  /*! No stream to write to. */
  if(!parent)
    return -1;

  while(len)
  {
    u_long nbw_parent;
    u_long size=std::min(len, 512UL);
    u_long ret=compress(buffer, size, tmpBuffer, 1024);

    if(ret)
      if(parent->write(tmpBuffer, ret, &nbw_parent) == -1 )
        return -1;

    written+=ret;
    buffer+=size;
    len-=size;
    *nbw+=nbw_parent;
  }
  return 0;
}

/*! 
 *Inherited from Filter.
 */
int Gzip::flush(u_long *nbw)
{
  char buffer[512];
  *nbw = flush(buffer, 512);
  if(*nbw)
  {
    u_long nbwParent;
    u_long nbwParentFlush;
    if(!parent)
      return -1;
    if(parent->write(buffer, *nbw, &nbwParent) != 0)
      return -1;
    if(parent->flush(&nbwParentFlush) != 0)
      return -1;  
    *nbw=nbwParentFlush+nbwParent;
   }
  return 0;
}

/*!
 *Returns a new Gzip object.
 */
Filter* Gzip::factory(const char* name)
{
  return new Gzip();
}

/*!
 *Return a string with the filter name. 
 *If an external buffer is provided write the name there too.
 */
const char* Gzip::getName(char* name, u_long len)
{
  /*! No name by default. */
  if(name)
  {
    myserver_strlcpy(name, "gzip", len);
  }
  return "gzip";
}
