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
#pragma once

#include "..\stdafx.h"
#include "..\include\Response_RequestStructs.h"
/*
*Structure used by the HTTP protocol to describe a thread
*/
struct httpThreadDescriptor
{
	char *buffer;
	char *buffer2;	
	DWORD buffersize;
	DWORD buffersize2;
	DWORD id;
	DWORD  nBytesToRead;
	HTTP_RESPONSE_HEADER  response;
	HTTP_REQUEST_HEADER  request;
	char filenamePath[MAX_PATH];
	LOGGEDUSERID hImpersonation;
};
/*
*Functions used by the HTTP parser.
*The main function is controlHTTPConnection(...), that parsing the request builds a response.
*/
BOOL controlHTTPConnection(LPCONNECTION a,char *b1,char *b2,int bs1,int bs2,DWORD nbtr,LOGGEDUSERID *imp);
BOOL sendHTTPRESOURCE(httpThreadDescriptor*,LPCONNECTION s,char *filename,BOOL systemrequest=FALSE,BOOL OnlyHeader=FALSE,int firstByte=0,int lastByte=-1);
BOOL sendHTTPFILE(httpThreadDescriptor*,LPCONNECTION s,char *filenamePath,BOOL OnlyHeader=FALSE,int firstByte=0,int lastByte=-1);
BOOL sendHTTPDIRECTORY(httpThreadDescriptor*,LPCONNECTION s,char* folder);
void buildHTTPResponseHeader(char *str,HTTP_RESPONSE_HEADER*);
void buildDefaultHTTPResponseHeader(HTTP_RESPONSE_HEADER*);
BOOL sendMSCGI(httpThreadDescriptor*,LPCONNECTION s,char* exec,char* cmdLine=0);
BOOL sendCGI(httpThreadDescriptor*,LPCONNECTION s,char* filename,char* ext,char* exec);
BOOL raiseHTTPError(httpThreadDescriptor*,LPCONNECTION a,int ID);
void getPath(char *path,char *filename,BOOL systemrequest);
BOOL getMIME(char *MIME,char *filename,char *dest,char *dest2);
void buildCGIEnvironmentString(httpThreadDescriptor*,char *cgiEnvString);