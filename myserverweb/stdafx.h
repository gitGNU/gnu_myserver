/*
*myServer
*Copyright (C) Rocky_10_Balboa
*This library is free software; you can redistribute it and/or
*modify it under the terms of the GNU Library General Public
*License as published by the Free Software Foundation; either
*version 2 of the License, or (at your option) any later version.

*This library is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*Library General Public License for more details.

*You should have received a copy of the GNU Library General Public
*License along with this library; if not, write to the
*Free Software Foundation, Inc., 59 Temple Place - Suite 330,
*Boston, MA  02111-1307, USA.
*/
#ifndef STDAFX_H
#define STDAFX_H

extern "C" {
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>

#include <math.h>

#include <time.h>
}

#ifdef WIN32
#include <winsock2.h>
#include <tchar.h>
#include <process.h>
#include <io.h>
#endif

#ifdef __linux__
#warning Magic number used.  Possable buffer overflow
#define MAX_PATH 256
#define MAX_COMPUTERNAME_LENGTH 256
#define MAXIMUM_PROCESSORS 256
#endif

typedef unsigned long DWORD;
typedef int BOOL;

typedef void* HANDLE;
extern class cserver *lserver;
extern class CBase64Utils base64Utils;
struct CONNECTION;
extern const char *versionOfSoftware;
extern BOOL mustEndServer;

extern BOOL mustEndServer;
#define Thread   __declspec( thread )
typedef int (*CGIMAIN)(char*); 
typedef int (*CGIINIT)(struct httpThreadContext*, struct CONNECTION*);
//typedef int (*CGIINIT)(void*,void*,void*,void*); 
typedef CONNECTION*  volatile LPCONNECTION;
typedef  void* LOGGEDUSERID;

#endif
