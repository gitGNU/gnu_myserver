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
#include "..\include\utility.h"
#include "..\include\cXMLParser.h"
#include "..\resource.h"
#include "..\include\ClientsTHREAD.h"
#include "..\include\utility.h"
#include "..\include\HTTPmsg.h"
#include "..\include\Response_RequestStructs.h"
#include "..\include\ConnectionStruct.h"


/*
*Set MAX_MIME_TYPES to define the maximum
*number of MIME TYPES records to alloc
#define MAX_MIME_TYPES
*/
#include "..\include\MIME_manager.h"




unsigned int __stdcall listenServerHTTP(void* pParam);
class cserver
{
	friend  unsigned int __stdcall listenServerHTTP(void* pParam);
	friend  unsigned int __stdcall startClientsTHREAD(void* pParam);
	friend class ClientsTHREAD;
	friend LRESULT CALLBACK MainWndProc(HWND,UINT,WPARAM,LPARAM);
	friend BOOL __stdcall control_handler (DWORD control_type);
private:
	cXMLParser configurationFileManager;
	cXMLParser languageParser;
	char serverName[MAX_COMPUTERNAME_LENGTH+1];
	char languageFile[MAX_PATH];
	char systemPath[MAX_PATH];
	char path[MAX_PATH];
	char defaultFilename[MAX_PATH];
	WORD port_HTTP;
	DWORD nThreads;
	ClientsTHREAD threads[MAXIMUM_PROCESSORS];
	DWORD verbosity;
	BOOL useMessagesFiles;
	SOCKET serverSocketHTTP,asockHTTP;
	sockaddr_in sock_inserverSocketHTTP,asock_inHTTP;
	DWORD buffersize;
	DWORD buffersize2;
	DWORD getNumConnections();
	void initialize(INT);
	BOOL addConnection(SOCKET s,CONNECTION_PROTOCOL=PROTOCOL_FTP);
	LPCONNECTION findConnection(SOCKET s);
	DWORD connectionTimeout;
	DWORD socketRcvTimeout;
	int listenServerHTTPHandle;
	DWORD maxLogFileSize;
	VOID controlSizeLogFile();
public:
	MIME_Manager mimeManager;
	FILE *logFile;
	char  *getSystemPath();
	char  *getPath();
	char  *getDefaultFilenamePath();
	char  *getServerName();
	DWORD  getVerbosity();
	BOOL  mustUseMessagesFiles();
	BOOL  mustUseLogonOption();
	void  setVerbosity(DWORD);
	void start(INT);
	void stop();
	void terminate();
	int hInst;

}; LRESULT CALLBACK MainWndProc(HWND,UINT,WPARAM,LPARAM); 