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
#include "utility.h"
#include "..\include\cXMLParser.h"
#include "..\resource.h"
#include "ClientsTHREAD.h"
extern BOOL mustEndServer;
unsigned int WINAPI listenServerHTTP(void* pParam);
class cserver
{
	friend  unsigned int WINAPI listenServerHTTP(void* pParam);
	friend  unsigned int WINAPI startClientsTHREAD(void* pParam);
	friend class ClientsTHREAD;
	friend LRESULT CALLBACK MainWndProc(HWND,UINT,WPARAM,LPARAM);
	friend BOOL WINAPI control_handler (DWORD control_type);
private:
	cXMLParser configurationFileManager;
	cXMLParser languageParser;
	HINSTANCE hInst;
	MIME_Manager mimeManager;
	char serverName[MAX_COMPUTERNAME_LENGTH+1];
	char languageFile[MAX_PATH];
	WORD port_HTTP;
	DWORD nThreads;
	ClientsTHREAD threads[MAXIMUM_PROCESSORS];
	FILE *logFile;
	DWORD verbosity;
	BOOL useMessagesFiles;
	SOCKET serverSocketHTTP,asockHTTP;
	sockaddr_in sock_inserverSocketHTTP,asock_inHTTP;
	DWORD buffersize;
	DWORD buffersize2;
	char guestLogin[20];
	char guestPassword[32];
	char systemPath[MAX_PATH];
	char path[MAX_PATH];
	char defaultFilename[MAX_PATH];
	DWORD getNumConnections();
	void initialize(INT);
	BOOL addConnection(SOCKET s,CONNECTION_PROTOCOL=PROTOCOL_FTP);
	LPCONNECTION findConnection(SOCKET s);
	BOOL useLogonOption;
	DWORD connectionTimeout;
	DWORD socketRcvTimeout;
	HANDLE guestLoginHandle;
	HANDLE listenServerHTTPHandle;
	DWORD maxLogFileSize;
	VOID controlSizeLogFile();
	char msgSending[33];
	char msgRunOn[33];
	char msgFolderContents[33];
public:
	DWORD getVerbosity();
	void  setVerbosity(DWORD);
	void start(HINSTANCE);
	void stop();
	void terminate();
}; LRESULT CALLBACK MainWndProc(HWND,UINT,WPARAM,LPARAM); 