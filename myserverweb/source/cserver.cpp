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

#include "..\stdafx.h"
#include "..\include\cserver.h"
#include "..\include\security.h"
#include <Ws2tcpip.h>
#include <direct.h>
#include "..\include\sockets.h"

/*
*These variables are the unique istance of the class cserver in the application and the flag
*mustEndServer. When mustEndServer is true all the threads are stopped and the application stop
*its execution.
*/
cserver *lserver=0;
BOOL mustEndServer;
void cserver::start(INT hInst)
{
	mustEndServer=FALSE;
	cserver::hInst=hInst;
	ZeroMemory(this,sizeof(cserver));


	/*
	*If another instance of the class exists destroy it
	*/
	if(lserver)
	{
		mustEndServer=TRUE;
		lserver->terminate();
		mustEndServer=FALSE;
	}
	/*
	*Save the unique instance of this class
	*/
	lserver=this;
	logFile=openFile("myServer.log",MYSERVER_FILE_OPEN_APPEND|MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_WRITE|MYSERVER_FILE_OPEN_ALWAYS);
	setLogFile(logFile);
	controlSizeLogFile();

	/*
	*Setup the server configuration
	*/

	printf("\nInitializing server configuration...\n");

	INT OSVer=getOSVersion();

	initialize(OSVer);
	languageParser.open(languageFile);
	printf("%s\n",languageParser.getValue("MSG_LANGUAGE"));

	/*
	*This are the defaults values of this strings
	*/
	lstrcpy(msgSending,"Sending");
	lstrcpy(msgRunOn,"Running on");
	lstrcpy(msgFolderContents,"Contents of folder");
	lstrcpy(msgNewConnection,"Connection from");
	lstrcpy(msgErrorConnection,"Error connection from");
	lstrcpy(msgAtTime,"at time");
	

	/*
	*Load the strings buffers with the right values.
	*We do this for don't call parser while the application execution
	*There are two good reasons to do this:
	*1)Application performance
	*2)Avoid of errors due to the parser
	*/
	if(lstrcmpi(languageParser.getValue("MSG_SENDING"),"NONE"))
		lstrcpy(msgSending,languageParser.getValue("MSG_SENDING"));
	if(lstrcmpi(languageParser.getValue("MSG_RUNON"),"NONE"))
		lstrcpy(msgRunOn,languageParser.getValue("MSG_RUNON"));
	if(lstrcmpi(languageParser.getValue("MSG_FOLDERCONT"),"NONE"))
		lstrcpy(msgFolderContents,languageParser.getValue("MSG_FOLDERCONT"));
	if(lstrcmpi(languageParser.getValue("MSG_FILE"),"NONE"))
		lstrcpy(msgFile,languageParser.getValue("MSG_FILE"));
	if(lstrcmpi(languageParser.getValue("MSG_LMODIFY"),"NONE"))
		lstrcpy(msgLModify,languageParser.getValue("MSG_LMODIFY"));
	if(lstrcmpi(languageParser.getValue("MSG_SIZE"),"NONE"))
		lstrcpy(msgSize,languageParser.getValue("MSG_SIZE"));
	if(lstrcmpi(languageParser.getValue("MSG_NEWCONNECTION"),"NONE"))
		lstrcpy(msgNewConnection,languageParser.getValue("MSG_NEWCONNECTION"));
	if(lstrcmpi(languageParser.getValue("MSG_ERRORCONNECTION"),"NONE"))
		lstrcpy(msgErrorConnection,languageParser.getValue("MSG_ERRORCONNECTION"));
	if(lstrcmpi(languageParser.getValue("MSG_ATTIME"),"NONE"))
		lstrcpy(msgAtTime,languageParser.getValue("MSG_ATTIME"));



	printf("%s\n\n",languageParser.getValue("MSG_SERVER_CONF"));

	/*
	*The guestLoginHandle value is filled by the call to cserver::initialize
	*/
	if(guestLoginHandle==0)
	{
		
		printf("%s\n",languageParser.getValue("AL_NO_SECURITY"));
		useLogonOption=FALSE;
	}

	/*
	*Startup the socket library
	*/
	printf("%s\n",languageParser.getValue("MSG_ISOCK"));
	int err= ms_startupSocketLib(/*MAKEWORD( 2, 2 )*/MAKEWORD( 1, 1));
	if (err != 0) 
	{ 
		printf("%s\n",languageParser.getValue("ERR_ISOCK"));
		return; 
	} 
	printf("%s\n",languageParser.getValue("MSG_SOCKSTART"));

	/*
	*Create the HTTP server socket
	*/
	printf("%s\n",languageParser.getValue("MSG_SSOCKCREATE"));
	serverSocketHTTP=ms_socket(AF_INET,SOCK_STREAM,0);
	if(serverSocketHTTP==INVALID_SOCKET)
	{
		printf("%s\n",languageParser.getValue("ERR_OPENP"));
		return;

	}
	printf("%s\n",languageParser.getValue("MSG_SSOCKRUN"));
	sock_inserverSocketHTTP.sin_family=AF_INET;
	sock_inserverSocketHTTP.sin_addr.s_addr=htonl(INADDR_ANY);
	sock_inserverSocketHTTP.sin_port=htons(port_HTTP);

	/*
	*Bind the HTTP port
	*/
	printf("%s\n",languageParser.getValue("MSG_BIND_PORTHTTP"));

	if(ms_bind(serverSocketHTTP,(sockaddr*)&sock_inserverSocketHTTP,sizeof(sock_inserverSocketHTTP))!=0)
	{
		printf("%s\n",languageParser.getValue("ERR_BINDHTTP"));
		return;
	}
	printf("%s\n",languageParser.getValue("MSG_PORT_BINDEDHTTP"));



	/*
	*Set connections listen queque to max allowable
	*/
	printf("%s\n",languageParser.getValue("MSG_SLHTTP"));
	if (ms_listen(serverSocketHTTP,SOMAXCONN))
	{ 
		printf("%s\n",languageParser.getValue("ERR_LISTEN"));
		return; 
	}

	printf("%s: %u\n",languageParser.getValue("MSG_LHTTP"),port_HTTP);

	/*
	*Get the machine name
	*/
	getComputerName(serverName,(DWORD)sizeof(serverName));

	printf("%s:%s\n",languageParser.getValue("MSG_GETNAME"),serverName);

#ifdef WIN32
	/*
	*If the OS support the getaddrinfo function call it
	*to get info about the addresses of the current machine
	*/
	if((OSVer==OS_WINDOWS_2000)|(OSVer==OS_WINDOWS_XP))
	{
		addrinfo *ai;
		addrinfo *iai;
		getaddrinfo(serverName,NULL,NULL,&ai);
		iai=ai;
		WORD addresses_count=0;
		do
		{
			addresses_count++;
			sockaddr_in *sai=(sockaddr_in*)(iai->ai_addr);
			printf("%s #%u: %u.%u.%u.%u\n",languageParser.getValue("MSG_ADDRESS"),addresses_count,sai->sin_addr.S_un.S_un_b.s_b1,sai->sin_addr.S_un.S_un_b.s_b2,sai->sin_addr.S_un.S_un_b.s_b3,sai->sin_addr.S_un.S_un_b.s_b4);
			iai=ai->ai_next;
		}while(iai);
		freeaddrinfo(ai);
	}
#endif

	/*
	*Load the MIME types
	*/

	printf("%s\n",languageParser.getValue("MSG_LOADMIME"));
	if(!mimeManager.load("MIMEtypes.txt"))
		printf("%s\n",languageParser.getValue("MSG_MIMERUN"));
	else
		printf("%s\n",languageParser.getValue("ERR_LOADMIME"));

	/*
	*Create the threads
	*/
	printf("%s %u\n",languageParser.getValue("MSG_NUM_CPU"),getCPUCount());
	
	/*
	*Create a thread for every CPU.
	*/
	nThreads=getCPUCount();

	unsigned int ID;
	for(DWORD i=0;i<nThreads;i++)
	{
		printf("%s %u...\n",languageParser.getValue("MSG_CREATET"),i);
		threads[i].id=i;
		_beginthreadex(NULL,0,&::startClientsTHREAD,&threads[i].id,0,&ID);
		printf("%s\n",languageParser.getValue("MSG_THREADR"));
	}
	printf("%s\n",languageParser.getValue("MSG_LISTENT"));
	listenServerHTTPHandle=(int)_beginthreadex(NULL,0,&::listenServerHTTP,0,0,&ID);


	printf("%s\n",languageParser.getValue("MSG_LISTENTR"));

	printf("%s\n",languageParser.getValue("MSG_READY"));
	printf("%s\n",languageParser.getValue("MSG_BREAK"));
	

	/*
	*Keep thread alive.
	*When the mustEndServer flag is set to True exit
	*from the loop and terminate the server execution
	*/

	while(!mustEndServer);

	this->terminate();
}

unsigned int __stdcall listenServerHTTP(void*)

{
	INT asock_inLenHTTP=sizeof(lserver->asock_inHTTP);
	while(!mustEndServer)
	{
		/*
		*Accept connections.
		*Every new connection is sended to cserver::addConnection
		*function; this function dispatch connections 
		*between the various threads.
		*/
		lserver->asockHTTP=ms_accept(lserver->serverSocketHTTP,(struct sockaddr*)&lserver->asock_inHTTP,(LPINT)&asock_inLenHTTP);
		if(lserver->asockHTTP==0)
			continue;
		if(lserver->asockHTTP==INVALID_SOCKET)
			continue;
		
		lserver->addConnection(lserver->asockHTTP,PROTOCOL_HTTP);
	}	
	/*
	*When the flag mustEndServer is TRUE end current thread
	*/
	_endthreadex( 0 );

	return 0;

}

DWORD cserver::getNumConnections()
{
	/*
	*Get the number of connections in all the threads.
	*/
	DWORD nConnections=0;
	for(DWORD i=0;i<nThreads;i++)
	{
		nConnections+=threads[i].nConnections;
	}
	return nConnections;
}



DWORD cserver::getVerbosity()
{
	return verbosity;
}

void  cserver::setVerbosity(DWORD nv)
{
	verbosity=nv;
}

void cserver::stop()
{
	mustEndServer=TRUE;
}

void cserver::terminate()
{
	ms_shutdown(serverSocketHTTP, 2);
	/*
	*If the guestLoginHandle is allocated close it.
	*/
	if(useLogonOption)
		cleanLogonUser(&guestLoginHandle);

	/*
	*Stop server
	*/
	for(DWORD i=0;i<nThreads;i++)
	{
		if(verbosity>1)
			printf("%s\n",languageParser.getValue("MSG_STOPT"));
		threads[i].stop();
		if(verbosity>1)
			printf("%s\n",languageParser.getValue("MSG_TSTOPPED"));
	}
	if(verbosity>1)
	{
		printf("%s\n",languageParser.getValue("MSG_MEMCLEAN"));
	}
	/*
	*Clean memory allocated here
	*/
	languageParser.close();
	
	mimeManager.clean();
	for(i=0;i<nThreads;i++)
		threads[i].clean();
	if(verbosity>1)
	{
		printf("myServer is stopped\n\n");
	}
	ms_closesocket(serverSocketHTTP);

	closeFile(logFile);
}
void cserver::initialize(INT OSVer)
{
	/*
	*Here is loaded the configuration of the server.
	*The configuration file is a pseudo-XML file.
	*/
	socketRcvTimeout = 10;
	useLogonOption = TRUE;
	guestLoginHandle=0;
	connectionTimeout = SEC(25);
	lstrcpy(guestLogin,"myServerUnknown");
	lstrcpy(languageFile,"languages/english.xml");
	lstrcpy(guestPassword,"myServerUnknown");
	lstrcpy(defaultFilename,"default.html");
	mustEndServer=FALSE;
	port_HTTP=80;
	verbosity=1;
	buffersize=1024*1024;
	buffersize2=1024*1024;
	_getcwd(path,MAX_PATH);
	lstrcat(path,"/web");
	_getcwd(systemPath,MAX_PATH);
	lstrcat(systemPath,"/system");
	for(int i=0;i<lstrlen(path);i++)
		if(path[i]='\\')
			path[i]='/';
	for(i=0;i<lstrlen(systemPath);i++)
		if(systemPath[i]='\\')
			systemPath[i]='/';

	useMessagesFiles=TRUE;
	configurationFileManager.open("myserver.xml");
	CHAR *data;

	data=configurationFileManager.getValue("HTTP_PORT");
	if(data)
	{
		port_HTTP=(WORD)atoi(data);
	}

	data=configurationFileManager.getValue("VERBOSITY");
	if(data)
	{
		verbosity=(DWORD)atoi(data);
	}

	data=configurationFileManager.getValue("LANGUAGE");
	if(data)
	{
		sprintf(languageFile,"languages/%s",data);	
	}


	data=configurationFileManager.getValue("BUFFER_SIZE");
	if(data)
	{
		buffersize=buffersize2=(DWORD)atol(data);
	}

	data=configurationFileManager.getValue("CONNECTION_TIMEOUT");
	if(data)
	{

		connectionTimeout=SEC((DWORD)atol(data));

	}

	data=configurationFileManager.getValue("GUEST_LOGIN");
	if(data)
	{
		lstrcpy(guestLogin,data);
	}


	data=configurationFileManager.getValue("GUEST_PASSWORD");
	if(data)
	{
		lstrcpy(guestPassword,data);
	}

	data=configurationFileManager.getValue("DEFAULT_FILENAME");
	if(data)
	{
		lstrcpy(defaultFilename,data);
	}


	data=configurationFileManager.getValue("WEB_DIRECTORY");
	if(data)
	{
		_getcwd(path,MAX_PATH);
		lstrcat(path,"/");
		lstrcat(path,data);
	}

	data=configurationFileManager.getValue("SYSTEM_DIRECTORY");
	if(data)
	{
		_getcwd(systemPath,MAX_PATH);
		lstrcat(systemPath,"/");
		lstrcat(systemPath,data);
	}

	data=configurationFileManager.getValue("USE_ERRS_FILES");
	if(data)
	{
		if(!lstrcmpi(data,"YES"))
			useMessagesFiles=TRUE;
		else
			useMessagesFiles=FALSE;
	}

	data=configurationFileManager.getValue("USE_LOGON_OPTIONS");
	if(data)
	{
		if(!lstrcmpi(data,"YES"))
			useLogonOption=TRUE;
		else
			useLogonOption=FALSE;
	}

	data=configurationFileManager.getValue("MAX_LOG_FILE_SIZE");
	if(data)
	{
		maxLogFileSize=(DWORD)atol(data);
		controlSizeLogFile();
	}

	configurationFileManager.close();
	
	/*
	*Actually is supported only the WinNT access
	*security options.
	*The useLogonOption variable is used like a boolean,
	*in the future this can be used like an ID of the
	*security access engine used.
	*Logon options cannot be used onto Win9X family.
	*/
	useLogonOption=useLogonOption && (OSVer!=OS_WINDOWS_9X);

	logonGuest();

}


VOID cserver::controlSizeLogFile()
{
	/*
	*Control the size of the log file.
	*If this is bigger than maxLogFileSize
	*delete it.
	*/
	if(!logFile)
	{
		logFile=openFile("myServer.log",MYSERVER_FILE_OPEN_APPEND|MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_WRITE|MYSERVER_FILE_OPEN_ALWAYS);
	}
	DWORD fs=0;
	fs=getFileSize(logFile);
	if(fs>maxLogFileSize)
	{
		closeFile(logFile);
		deleteFile("myServer.log");
		logFile=openFile("myServer.log",MYSERVER_FILE_OPEN_APPEND|MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_WRITE|MYSERVER_FILE_OPEN_ALWAYS);
	}
	setLogFile(logFile);
}

BOOL cserver::addConnection(MYSERVER_SOCKET s,CONNECTION_PROTOCOL protID)
{
	if(s==0)
		return FALSE;
	static BOOL ret;
	ret=TRUE;
	if(verbosity>0)
	{
		char msg[500];
		sprintf(msg,"%s:%u.%u.%u.%u -> %s:%i %s:%s\r\n",msgNewConnection,lserver->asock_inHTTP.sin_addr.S_un.S_un_b.s_b1, lserver->asock_inHTTP.sin_addr.S_un.S_un_b.s_b2, lserver->asock_inHTTP.sin_addr.S_un.S_un_b.s_b3, lserver->asock_inHTTP.sin_addr.S_un.S_un_b.s_b4,serverName,lserver->port_HTTP,msgAtTime,getHTTPFormattedTime());
		logFileWrite(msg);
	}

	static DWORD local_nThreads=0;
	ClientsTHREAD *ct=&threads[local_nThreads];
	if(!ct->addConnection(s,protID))
	{
		ret=FALSE;
		ms_closesocket(s);
		if(verbosity>0)
		{
			char buffer[500];
			sprintf(buffer,"%s:%i.%i.%i.%i -> %s:%i %s:%s\r\n",msgErrorConnection,lserver->asock_inHTTP.sin_addr.S_un.S_un_b.s_b1, lserver->asock_inHTTP.sin_addr.S_un.S_un_b.s_b2, lserver->asock_inHTTP.sin_addr.S_un.S_un_b.s_b3, lserver->asock_inHTTP.sin_addr.S_un.S_un_b.s_b4,serverName,lserver->port_HTTP,msgAtTime,getHTTPFormattedTime());
			logFileWrite(buffer);
		}
	}

	if(++local_nThreads>=nThreads)
		local_nThreads=0;
	return ret;
}
/*
*Find a connection passing its socket
*/
LPCONNECTION cserver::findConnection(MYSERVER_SOCKET s)
{
	LPCONNECTION c=NULL;
	for(DWORD i=0;i<nThreads;i++)
	{
		c=threads[i].findConnection(s);
		if((i==nThreads-1) && (c!=NULL))
			return c;
	}
	return NULL;
}
/*
*Returns the full path of the system folder
*/
char *cserver::getSystemPath()
{
	return systemPath;
}
/*
*Returns the full path of the web folder
*/
char *cserver::getPath()
{
	return path;
}
/*
*Returns the default filename
*/
char *cserver::getDefaultFilenamePath(DWORD)
{
	return defaultFilename;
}
/*
*Returns the name of the server(the name of the current PC)
*/
char *cserver::getServerName()
{
	return serverName;
}
/*
*Returns true if we use personalized errors page
*false if we don't use personalized errors page
*/
BOOL cserver::mustUseMessagesFiles()
{
	return useMessagesFiles;
}
BOOL cserver::mustUseLogonOption()
{
	return useLogonOption;
}
