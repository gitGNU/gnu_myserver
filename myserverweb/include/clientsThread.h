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
#ifndef CLIENTSTHREAD_H
#define CLIENTSTHREAD_H
#include "../stdafx.h"
#include "../include/http.h"
#include "../include/utility.h"
#include "../include/HTTPmsg.h"
#include "../include/Response_RequestStructs.h"
#include "../include/connectionstruct.h"
#include "../include/security.h"

class  ClientsTHREAD
{
	friend class cserver;
#ifdef WIN32
	friend  unsigned int __stdcall startClientsTHREAD(void* pParam);
#else
	friend  void* startClientsTHREAD(void* pParam);
#endif
#ifdef WIN32
	friend LRESULT CALLBACK MainWndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
#endif
private:
	int initialized;
	LOGGEDUSERID hImpersonation;
	u_long id;
	int err;
	int threadIsRunning;
	int threadIsStopped;
    u_long nConnections;
	u_long buffersize;
	u_long buffersize2;
	LPCONNECTION addConnection(MYSERVER_SOCKET,CONNECTION_PROTOCOL,char*,char*,int);
	LPCONNECTION findConnection(MYSERVER_SOCKET s);
	int isRunning();
	int isStopped();
	char *buffer;
	char *buffer2;
	void clearAllConnections();
	int deleteConnection(LPCONNECTION id);
	void controlConnections();
	u_long connectionWriteAccess;
	LPCONNECTION connections;
	u_long nBytesToRead;
public:
	ClientsTHREAD();
	~ClientsTHREAD();
	void stop();
	void clean();	
};
#ifdef WIN32
unsigned int __stdcall startClientsTHREAD(void* pParam); 
#else
void* startClientsTHREAD(void* pParam);
#endif

#endif
