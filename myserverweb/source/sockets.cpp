/*
*myServer
*Copyright (C) 2002 Giuseppe Scrivano
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


#include "../stdafx.h"
#include "../include/utility.h"
#include "../include/sockets.h"
extern "C" {
#include <string.h>
#include <stdio.h>
#ifndef WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#endif
}

#ifdef WIN32
#pragma comment(lib,"wsock32.lib")
#pragma comment(lib,"ws2_32.lib")
#endif

/*
*Source code to wrap the socket library to myServer project.
*/
int ms_startupSocketLib(u_short ver)
{
#ifdef WIN32	
	WSADATA wsaData;
	return WSAStartup(ver, &wsaData);
#else
	return 0;
#endif
}

MYSERVER_SOCKET ms_socket(int af,int type,int protocol)
{
	return	(MYSERVER_SOCKET)socket(af,type,protocol);
}

int ms_bind(MYSERVER_SOCKET s,MYSERVER_SOCKADDR* sa,int namelen)
{
#ifdef WIN32	
	return bind((SOCKET)s,sa,namelen);
#endif
#ifdef __linux__
	return bind((int)s,sa,namelen);
#endif
}

int ms_listen(MYSERVER_SOCKET s,int max)
{
#ifdef WIN32
	return listen(s,max);
#endif
#ifdef __linux__
	return listen((int)s,max);
#endif
}

MYSERVER_SOCKET ms_accept(MYSERVER_SOCKET s,MYSERVER_SOCKADDR* sa,int* sockaddrlen)
{
#ifdef WIN32
	return (MYSERVER_SOCKET)accept(s,sa,sockaddrlen);
#endif
#ifdef __linux__
	unsigned int Connect_Size = *sockaddrlen;
	int as = accept((int)s,sa,&Connect_Size);
	return (MYSERVER_SOCKET)as;
#endif
}

int ms_closesocket(MYSERVER_SOCKET s)
{
#ifdef WIN32
	return closesocket(s);
#endif
#ifdef __linux__
	return close((int)s);
#endif
}
MYSERVER_HOSTENT *ms_gethostbyaddr(char* addr,int len,int type)
{
#ifdef WIN32
	HOSTENT *he=gethostbyaddr(addr,len,type);
	return he;
#endif
#ifdef __linux__
	struct hostent * he=gethostbyaddr(addr,len,type);
	return he;
#endif
}
MYSERVER_HOSTENT *ms_gethostbyname(const char *hostname)
{	
	return (MYSERVER_HOSTENT *)gethostbyname(hostname);
}


int ms_shutdown(MYSERVER_SOCKET s,int how)
{
#ifdef WIN32
	return shutdown(s,how);
#endif
#ifdef __linux__
	return shutdown((int)s,how);
#endif
}

int	ms_setsockopt(MYSERVER_SOCKET s,int level,int optname,const char *optval,int optlen)
{
	return setsockopt(s,level, optname,optval,optlen);
}

int ms_send(MYSERVER_SOCKET s,const char* buffer,int len,int flags)
{
#ifdef WIN32
	return	send(s,buffer,len,flags);
#endif
#ifdef __linux__
	return	send((int)s,buffer,len,flags);
#endif
}

int ms_ioctlsocket(MYSERVER_SOCKET s,long cmd,unsigned long* argp)
{
#ifdef WIN32
	return ioctlsocket(s,cmd,argp);
#endif
#ifdef __linux__
	return ioctl((int)s,cmd,argp);
#endif
}

int ms_connect(MYSERVER_SOCKET s,MYSERVER_SOCKADDR* sa,int na)
{
#ifdef WIN32
	return connect((SOCKET)s,sa,na);
#endif
#ifdef __linux__
	return connect((int)s,sa,na);
#endif
}

int ms_recv(MYSERVER_SOCKET s,char* buffer,int len,int flags)
{
	int err;
#ifdef WIN32
	err=recv(s,buffer,len,flags);
	if(err==SOCKET_ERROR)
		return -1;
	else 
		return err;
#endif
#ifdef __linux__
	err=recv((int)s,buffer,len,flags);
	if(err == 0)
		err = -1;
	return err;
#endif
}

u_long ms_bytesToRead(MYSERVER_SOCKET c)
{
	u_long nBytesToRead;
	ms_ioctlsocket(c,FIONREAD,&nBytesToRead);
	return nBytesToRead;
}
int ms_gethostname(char *name,int namelen)
{
	return gethostname(name,namelen);
}
int ms_getsockname(MYSERVER_SOCKET s,MYSERVER_SOCKADDR *ad,int *namelen)
{
#ifdef WIN32
	return getsockname(s,ad,namelen);
#endif
#ifdef __linux__
	unsigned int len = *namelen;
	int ret = getsockname((int)s,ad,&len);
	*namelen = len;
	return ret;
#endif
}
