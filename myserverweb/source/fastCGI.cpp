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
 
#include "../include/fastCGI.h"
#define MAX_FCGI_SERVERS	25
static struct sfCGIservers
{
	char path[MAX_PATH];/*exec path*/
	union 
	{
	    /*HANDLE*/unsigned long fileHandle;
		SOCKET sock;
		unsigned int value;
	}DESCRIPTOR;
	MYSERVER_SOCKET socket;
	MYSERVER_SOCKET dupsock;
	int pid; /*process ID*/
	u_short port;/*IP port*/
}fCGIservers[MAX_FCGI_SERVERS];
struct fourchar
{	
	union
	{
		unsigned char c[4];
		unsigned int i;
	};
};
static int fCGIserversN;
/*
*Entry-Point to manage a FastCGI request.
*/
int sendFASTCGI(httpThreadContext* td,LPCONNECTION connection,char* scriptpath,char* /*ext*/,char *cgipath)
{
	fCGIContext con;
	con.td=td;
	u_long nbr;
	FCGI_Header tHeader;
	strcpy(td->scriptPath,scriptpath);
	MYSERVER_FILE::splitPath(scriptpath,td->scriptDir,td->scriptFile);
	MYSERVER_FILE::splitPath(cgipath,td->cgiRoot,td->cgiFile);
	td->buffer[0]='\0';
	strcpy(td->buffer,"FCGI_ROLE=RESPONDER\r");
	buildCGIEnvironmentString(td,td->buffer);

	int sizeEnvString=buildFASTCGIEnvironmentString(td,td->buffer,td->buffer2);

	int pID = FcgiConnect(&con,cgipath);
	if(pID<0)
		return raiseHTTPError(td,connection,e_500);

	int id=td->id+1;
	FCGI_BeginRequestBody tBody;
	tBody.roleB1 = ( FCGI_RESPONDER >> 8 ) & 0xff;
	tBody.roleB0 = ( FCGI_RESPONDER ) & 0xff;
	tBody.flags = 0;
	memset( tBody.reserved, 0, sizeof( tBody.reserved ) );

	if(sendFcgiBody(&con,(char*)&tBody,sizeof(tBody),FCGI_BEGIN_REQUEST,id))
	{
		con.sock.ms_closesocket();
		return raiseHTTPError(td,connection,e_501);
	}

	if(sendFcgiBody(&con,td->buffer2,sizeEnvString,FCGI_PARAMS,id))
	{
		con.sock.ms_closesocket();
		return raiseHTTPError(td,connection,e_501);
	}
    if(atoi(td->request.CONTENTS_DIM))
	{
		generateFcgiHeader( tHeader, FCGI_STDIN, id, atoi(td->request.CONTENTS_DIM) );
		fCGIservers[con.fcgiPID].socket.ms_send((char*)&tHeader,sizeof(tHeader),0);
		td->inputData.ms_setFilePointer(0);
		while(nbr=td->inputData.ms_ReadFromFile(td->buffer,td->buffersize,&nbr))
		{
			fCGIservers[con.fcgiPID].socket.ms_send(td->buffer,nbr,0);
		}
	}
	else
	{
		if(sendFcgiBody(&con,0,0,FCGI_STDIN,id))
		{
			con.sock.ms_closesocket();
			return raiseHTTPError(td,connection,e_501);
		}	
	}
	
	/*Now read the output*/
	int exit=0;
	int timeout=200;
	do	
	{
		while(timeout && (!con.sock.ms_bytesToRead()))
		{
			ms_wait(1);
			timeout--;
		}
		if(con.sock.ms_bytesToRead())
			nbr=con.sock.ms_recv((char*)&tHeader,sizeof(FCGI_Header),0);
		else
		{
			nbr=0;
			return raiseHTTPError(td,connection,e_500);
		}
		if(nbr<sizeof(FCGI_Header))
		{
			con.sock.ms_closesocket();
			raiseHTTPError(td,connection,e_500);
		}
		int dim=(tHeader.contentLengthB1<<8) + tHeader.contentLengthB0;
		int headerSize=0;
		int dataSent=0;
		if(dim==0)
		{
			exit = 1;
		}
		else
		{
			switch(tHeader.type)
			{
				case FCGI_STDERR:
					con.sock.ms_closesocket();
					return raiseHTTPError(td,connection,e_501);
				case FCGI_STDOUT:
					nbr=con.sock.ms_recv(td->buffer,min(dim,td->buffersize),0);
		
					for(u_long i=0;i<nbr;i++)
					{
						if((td->buffer[i]=='\r')&&(td->buffer[i+1]=='\n')&&(td->buffer[i+2]=='\r')&&(td->buffer[i+3]=='\n'))
						{
							headerSize=i+4;
							break;
						}
					}
					sprintf(td->response.CONTENTS_DIM,"%u",dim-headerSize);
					buildHTTPResponseHeaderStruct(&td->response,td,td->buffer);
					buildHTTPResponseHeader(td->buffer2,&td->response);
					if(td->connection->socket.ms_send(td->buffer2,strlen(td->buffer2), 0)==0)
					{
						exit = 1;
						break;
					}
					dataSent=td->connection->socket.ms_send((char*)(td->buffer+headerSize),nbr-headerSize, 0)+headerSize;
					while(dataSent<dim)
					{
						if( con.sock.ms_bytesToRead() )
							nbr=con.sock.ms_recv(td->buffer,min(dim-dataSent,td->buffersize),0);
						else
						{
							exit = 1;
							break;
						}
						td->connection->socket.ms_send(td->buffer2,nbr, 0);
						dataSent+=nbr;
					}
					break;
				case FCGI_END_REQUEST:
					exit = 1;
					break;			
				case FCGI_GET_VALUES_RESULT:
				case FCGI_UNKNOWN_TYPE:
				default:
					break;
			}
		}
	}while((!exit) && nbr);
	con.sock.ms_closesocket();
	return 1;
}

int sendFcgiBody(fCGIContext* con,char* buffer,int len,int type,int id)
{
	FCGI_Header tHeader;
	generateFcgiHeader( tHeader, type, id, len );
	
	if(con->sock.ms_send((char*)&tHeader,sizeof(tHeader),0)==-1)
		return -1;
	if(con->sock.ms_send(buffer,len,0)==-1)
		return -1;
	return 0;
}
/*
*Trasform from an environment string to the FastCGI environment string.
*/
int buildFASTCGIEnvironmentString(httpThreadContext* td,char* sp,char* ep)
{
	char *ptr=ep;
	char *sptr=sp;
	char varName[100];
	char varValue[2500];
	int i;
	for(;;)
	{
		if(*(++sptr)=='\0')
			break;
		
		fourchar varNameLen;
		fourchar varValueLen;

		varNameLen.i=varValueLen.i=0;
		varName[0]='\0';
		varValue[0]='\0';
		while(*sptr != '=')
		{
			varName[varNameLen.i++]=*sptr++;
			varName[varNameLen.i]='\0';
		}
		sptr++;
		while(*sptr != '\0')
		{
			varValue[varValueLen.i++]=*sptr++;
			varValue[varValueLen.i]='\0';
		}
		if(varNameLen.i > 127)
		{
			unsigned char fb=varValueLen.c[3]|0x80;
			*ptr++=fb;
			*ptr++=varNameLen.c[2];
			*ptr++=varNameLen.c[1];
			*ptr++=varNameLen.c[0];
		}
		else
		{
			*ptr++=varNameLen.c[0];
		}

		if(varValueLen.i > 127)
		{
			unsigned char fb=varValueLen.c[3]|0x80;
			*ptr++=fb;
			*ptr++=varValueLen.c[2];
			*ptr++=varValueLen.c[1];
			*ptr++=varValueLen.c[0];
		}
		else
		{
			*ptr++=varValueLen.c[0];
		}



		for(i=0;i<varNameLen.i;i++)
			*ptr++=varName[i];
		for(i=0;i<varValueLen.i;i++)
			*ptr++=varValue[i];

	}
	return (int)(ptr-ep);
}
void generateFcgiHeader( FCGI_Header &tHeader, int iType,int iRequestId, int iContentLength )
{
	tHeader.version = FCGI_VERSION_1;
	tHeader.type = iType;
	tHeader.requestIdB1 = (iRequestId >> 8 ) & 0xff;
	tHeader.requestIdB0 = (iRequestId ) & 0xff;
	tHeader.contentLengthB1 = (iContentLength >> 8 ) & 0xff;
	tHeader.contentLengthB0 = (iContentLength ) & 0xff;
	tHeader.paddingLength = 0;
	tHeader.reserved = 0;
};

int initializeFASTCGI()
{
	fCGIserversN=0;

	return 1;
}
int cleanFASTCGI()
{
	for(int i=0;i<fCGIserversN;i++)
	{
		fCGIservers[i].socket.ms_closesocket();
#ifdef WIN32
		fCGIservers[fCGIserversN].dupsock.ms_closesocket();
#endif
		terminateProcess(fCGIservers[i].pid);
	}
	return 1;
}
int isFcgiServerRunning(char* path)
{
	for(int i=0;i<fCGIserversN;i++)
	{
		if(!_stricmp(path,fCGIservers[i].path))
			return i;
	}
	return -1;
}
int FcgiConnect(fCGIContext* con,char* path)
{
	int pID;
	unsigned long pLong = 1L;
	if((pID=runFcgiServer(con,path))>=0)
	{
		MYSERVER_HOSTENT *hp=MYSERVER_SOCKET::ms_gethostbyname("localhost");

		struct sockaddr_in sockAddr;
		int sockLen = sizeof(sockAddr);
        memset(&sockAddr, 0, sizeof(sockAddr));
        sockAddr.sin_family = AF_INET;
	    memcpy(&sockAddr.sin_addr, hp->h_addr, hp->h_length);
	    sockAddr.sin_port = htons(fCGIservers[pID].port);
		
		if(con->sock.ms_socket(AF_INET, SOCK_STREAM, 0) == -1)
		{
			return -1;
		}
		if(con->sock.ms_connect((MYSERVER_SOCKADDR*)&sockAddr, sockLen) == -1)
		{
			con->sock.ms_closesocket();
			return -1;
		}
#ifdef WIN32    // FIONBIO is win32 dependent
		con->sock.ms_ioctlsocket(FIONBIO, &pLong);
#endif
		con->fcgiPID=pID;
	}
	return pID;
}
int runFcgiServer(fCGIContext *con,char* path)
{
	int pID=isFcgiServerRunning(path);
	if(pID>=0)
		return pID;
	if(fCGIserversN==MAX_FCGI_SERVERS-2)
		return -1;
	static u_short port=3333;
	{
		/*SERVER SOCKET CREATION*/
		memset(&fCGIservers[fCGIserversN],0,sizeof(fCGIservers[fCGIserversN]));
		fCGIservers[fCGIserversN].socket.ms_socket(AF_INET,SOCK_STREAM,0);
		if(fCGIservers[fCGIserversN].socket.ms_getHandle()==INVALID_SOCKET)
			return -2;
		MYSERVER_SOCKADDRIN sock_inserverSocket;
		sock_inserverSocket.sin_family=AF_INET;
		sock_inserverSocket.sin_addr.s_addr=htonl(INADDR_ANY);
		fCGIservers[fCGIserversN].port=port+fCGIserversN;
		sock_inserverSocket.sin_port=htons(fCGIservers[fCGIserversN].port);
		if(fCGIservers[fCGIserversN].socket.ms_bind((sockaddr*)&sock_inserverSocket,sizeof(sock_inserverSocket)))
		{
			fCGIservers[fCGIserversN].socket.ms_closesocket();
			return -2;
		}
		if(fCGIservers[fCGIserversN].socket.ms_listen(SOMAXCONN))
		{
			fCGIservers[fCGIserversN].socket.ms_closesocket();
			return -2;
		}
#ifdef WIN32
		HANDLE sourceHandle=(HANDLE)fCGIservers[fCGIserversN].socket.ms_getHandle();
		HANDLE destHandle;
		DuplicateHandle(GetCurrentProcess(),sourceHandle,GetCurrentProcess(),&destHandle,0,TRUE,DUPLICATE_SAME_ACCESS);
		fCGIservers[fCGIserversN].dupsock.ms_setHandle((MYSERVER_SOCKET_HANDLE)destHandle);
		fCGIservers[fCGIserversN].DESCRIPTOR.fileHandle=(unsigned long)destHandle;
#else
		fCGIservers[fCGIserversN].DESCRIPTOR.sock=fCGIservers[fCGIserversN].dupsock.ms_getHandle();
#endif
	}
	START_PROC_INFO spi;
	memset(&spi,0,sizeof(spi));
	char cmd[MAX_PATH];
	spi.cmd=cmd;
	spi.stdIn = (MYSERVER_FILE_HANDLE)fCGIservers[fCGIserversN].DESCRIPTOR.fileHandle;
	spi.arg=con->td->buffer2;
	spi.cmdLine=cmd;

	sprintf(spi.cmd,"%s%s",con->td->cgiRoot,con->td->cgiFile);
	strcpy(fCGIservers[fCGIserversN].path,spi.cmd);
	spi.stdOut = spi.stdError =(MYSERVER_FILE_HANDLE) -1;
	fCGIservers[fCGIserversN].pid=execConcurrentProcess(&spi);
    
	
	if(fCGIservers[fCGIserversN].pid)
		fCGIserversN;
	else
		return -3;

	return fCGIserversN++;
}
