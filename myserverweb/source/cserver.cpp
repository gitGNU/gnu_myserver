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

#include "../stdafx.h"
#include "../include/cserver.h"

#include "../include/http.h"	/*Include the HTTP protocol*/
#include "../include/https.h" /*Include the HTTPS protocol*/

#include "../include/security.h"
#include "../include/stringutils.h"
#include "../include/sockets.h"
extern "C" {
#ifdef WIN32
#include <Ws2tcpip.h>
#include <direct.h>
#endif
#ifdef NOT_WIN
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#endif
}

#ifdef NOT_WIN
#define LPINT int *
#define INVALID_SOCKET -1
#define WORD unsigned int
#define BYTE unsigned char
#define MAKEWORD(a, b) ((WORD) (((BYTE) (a)) | ((WORD) ((BYTE) (b))) << 8)) 
#define max(a,b) ((a>b)?a:b)
#endif

/*!
*This is the unique istance for the class cserver in the application.
*/
cserver *lserver=0;
/*!
When the flag mustEndServer is 1 all the threads are stopped and the application stop
*its execution.
*/
int mustEndServer;



void cserver::start()
{
	u_long i;
	nConnections=0;
	connections=0;
	connectionToParse=0;
	/*!
	*Reset flag
	*/
	mustEndServer=0;
	memset(this, 0, sizeof(cserver));


	/*!
	*If another instance of the class exists destroy it.
	*/
	if(lserver)
	{
		mustEndServer=1;
		lserver->terminate();
		wait(2000);/*!Wait for a while*/
		mustEndServer=0;
	}
	/*!
	*Save the unique instance of this class.
	*/
	lserver=this;

#ifdef WIN32
	/*!
	*Under the windows platform use the cls operating-system command to clear the screen.
	*/
	_flushall();
	system("cls");
#endif
#ifdef NOT_WIN
	/*!
	*Under an UNIX environment, clearing the screen can be done in a similar method
	*/
	sync();
	system("clear");
#endif
	/*!
	*Print the MyServer signature.
	*/
	char *software_signature=(char*)malloc(200);
	sprintf(software_signature,"************MyServer %s************",versionOfSoftware);

	i=(u_long)strlen(software_signature);
	while(i--)
		printf("*");
    	printf("\n%s\n",software_signature);
	i=(u_long)strlen(software_signature);
	while(i--)
		printf("*");
	printf("\n");
	free(software_signature);

	/*!
	*Set the current working directory.
	*/
	setcwdBuffer();
	
	
	/*!
	*Setup the server configuration.
	*/
    printf("Initializing server configuration...\n");
#ifdef WIN32
	envString=GetEnvironmentStrings();
#endif
	int OSVer=getOSVersion();

	initialize(OSVer);
	
	languageParser.open(languageFile);
	printf("%s\n",languageParser.getValue("MSG_LANGUAGE"));

	http::loadProtocol(&languageParser,"myserver.xml");
	https::loadProtocol(&languageParser,"myserver.xml");
	protocols.loadProtocols("external/protocols",&languageParser,"myserver.xml",this);

	/*!
	*Initialize the SSL library
	*/
#ifndef DO_NOT_USE_SSL
	SSL_library_init();
	SSL_load_error_strings();
#endif
	printf("%s\n",languageParser.getValue("MSG_SERVER_CONF"));

	/*!
	*The guestLoginHandle value is filled by the call to cserver::initialize.
	*/
	if(useLogonOption==0)
	{
        preparePrintError();		
		printf("%s\n",languageParser.getValue("AL_NO_SECURITY"));
        endPrintError();
	}

	/*!
	*Startup the socket library.
	*/
	printf("%s\n",languageParser.getValue("MSG_ISOCK"));
	int err= startupSocketLib(/*!MAKEWORD( 2, 2 )*/MAKEWORD( 1, 1));
	if (err != 0) 
	{ 

        preparePrintError();
		printf("%s\n",languageParser.getValue("ERR_ISOCK"));
		endPrintError();
		return; 
	} 
	printf("%s\n",languageParser.getValue("MSG_SOCKSTART"));

	
	/*!
	*Get the name of the local machine.
	*/
	MYSERVER_SOCKET::gethostname(serverName,(u_long)sizeof(serverName));
	printf("%s: %s\n",languageParser.getValue("MSG_GETNAME"),serverName);

	/*!
	*Determine all the IP addresses of the local machine.
	*/
	MYSERVER_HOSTENT *localhe=MYSERVER_SOCKET::gethostbyname(serverName);
	in_addr ia;
	ipAddresses[0]='\0';
	printf("Host: %s\r\n",serverName);
	if(localhe)
	{
		for(i=0;(localhe->h_addr_list[i])&&(i< MAX_ALLOWED_IPs);i++)
		{
#ifdef WIN32
			ia.S_un.S_addr = *((u_long FAR*) (localhe->h_addr_list[i]));
#endif
#ifdef NOT_WIN
			ia.s_addr = *((u_long *) (localhe->h_addr_list[i]));
#endif
			printf("%s #%u: %s\n",languageParser.getValue("MSG_ADDRESS"),i+1,inet_ntoa(ia));
			sprintf(&ipAddresses[strlen(ipAddresses)],"%s%s",strlen(ipAddresses)?",":"",inet_ntoa(ia));
		}
	}
	

	/*!
	*If the MIMEtypes.xml files doesn't exist copy it from the default one.
	*/
	if(!MYSERVER_FILE::fileExists("MIMEtypes.xml"))
	{
			printf("aaa\r\n\r\n\r\n\r\n");
			MYSERVER_FILE inputF;
			MYSERVER_FILE outputF;
			int ret=inputF.openFile("MIMEtypes.xml.default",MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_IFEXISTS);
			if(ret<1)
			{
				preparePrintError();
				printf("%s\n",languageParser.getValue("ERR_LOADMIME"));
				endPrintError();				
				return;
			}
			outputF.openFile("MIMEtypes.xml",MYSERVER_FILE_OPEN_WRITE|MYSERVER_FILE_OPEN_ALWAYS);
			char buffer[512];
			u_long nbr,nbw;
			for(;;)
			{
				inputF.readFromFile(buffer,512,&nbr );
				if(nbr==0)
					break;
				outputF.writeToFile(buffer,nbr,&nbw);
			}
			
			inputF.closeFile();
			outputF.closeFile();
	}
	/*!
	*Load the MIME types.
	*/		
	printf("%s\n",languageParser.getValue("MSG_LOADMIME"));
	if(int nMIMEtypes=mimeManager.loadXML("MIMEtypes.xml"))
	{
		printf("%s: %i\n",languageParser.getValue("MSG_MIMERUN"),nMIMEtypes);
	}
	else
	{
        preparePrintError();
		printf("%s\n",languageParser.getValue("ERR_LOADMIME"));
        endPrintError();
		return;
	}
	printf("%s %u\n",languageParser.getValue("MSG_NUM_CPU"),getCPUCount());

#ifdef WIN32
	unsigned int ID;
#endif
#ifdef HAVE_PTHREAD
	pthread_t ID;
#endif
	threads=new ClientsTHREAD[nThreads];
	if(threads==NULL)
	{
		preparePrintError();
		printf("%s: Threads creation\n",languageParser.getValue("ERR_ERROR"));
        endPrintError();	
	}
	for(i=0;i<nThreads;i++)
	{
		printf("%s %u...\n",languageParser.getValue("MSG_CREATET"),i);
		threads[i].id=(i+ClientsTHREAD::ID_OFFSET);
#ifdef WIN32
		_beginthreadex(NULL,0,&::startClientsTHREAD,&threads[i].id,0,&ID);
#endif
#ifdef HAVE_PTHREAD
		pthread_create(&ID, NULL, &::startClientsTHREAD, (void *)&(threads[i].id));
#endif
		printf("%s\n",languageParser.getValue("MSG_THREADR"));
	}

	getdefaultwd(path,MAX_PATH);
	/*!
	*Then we create here all the listens threads. Check that all the port used for listening
	*have a listen thread.
	*/
	printf("%s\n",languageParser.getValue("MSG_LISTENT"));
	
	/*!
	*If the virtualhosts.xml files doesn't exist copy it from the default one.
	*/
	if(!MYSERVER_FILE::fileExists("virtualhosts.xml"))
	{
			MYSERVER_FILE inputF;
			MYSERVER_FILE outputF;
			int ret=inputF.openFile("virtualhosts.xml.default",MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_IFEXISTS);
			if(ret<1)
			{
				preparePrintError();
				printf("%s\n",languageParser.getValue("ERR_LOADMIME"));
				endPrintError();				
				return;
			}
			outputF.openFile("virtualhosts.xml",MYSERVER_FILE_OPEN_WRITE|MYSERVER_FILE_OPEN_ALWAYS);
			char buffer[512];
			u_long nbr,nbw;
			for(;;)
			{
				inputF.readFromFile(buffer,512,&nbr );
				if(nbr==0)
					break;
				outputF.writeToFile(buffer,nbr,&nbw);
			}
			
			inputF.closeFile();
			outputF.closeFile();
	}	
	/*!
	*Load the virtual hosts configuration from the xml file
	*/
	vhostList.loadXMLConfigurationFile("virtualhosts.xml",this->getMaxLogFileSize());


	/*!
	*Create the listens threads.
	*Every port uses a thread.
	*/
	for(vhostmanager::sVhostList *list=vhostList.getvHostList();list;list=list->next )
	{
		int needThread=1;
		vhostmanager::sVhostList *list2=vhostList.getvHostList();
		for(;;)
		{
			list2=list2->next;
			if(list2==0)
				break;
			if(list2==list)
				break;
			if(list2->host->port==list->host->port)
				needThread=0;
		}
		if(list->host->port==0)
			continue;
		if(needThread)
		{
			if(createServerAndListener(list->host->port)==0)
			{
				preparePrintError();
				printf("%s: Listen threads\n",languageParser.getValue("ERR_ERROR"));
				endPrintError();
				return;
			}
		}
	}

	printf("%s\n",languageParser.getValue("MSG_READY"));
	printf("%s\n",languageParser.getValue("MSG_BREAK"));

	/*!
	*Keep thread alive.
	*When the mustEndServer flag is set to True exit
	*from the loop and terminate the server execution.
	*/
	while(!mustEndServer)
	{
#ifdef WIN32
		Sleep(1);
		DWORD cNumRead,i; 
		INPUT_RECORD irInBuf[128]; 
		ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE),irInBuf,128,&cNumRead);
		for (i = 0; i < cNumRead; i++) 
		{
			switch(irInBuf[i].EventType) 
			{ 
				case KEY_EVENT:
					if(irInBuf[i].Event.KeyEvent.wRepeatCount!=1)
						continue;
					if(irInBuf[i].Event.KeyEvent.wVirtualKeyCode=='c'||irInBuf[i].Event.KeyEvent.wVirtualKeyCode=='C')
					{
						if((irInBuf[i].Event.KeyEvent.dwControlKeyState & LEFT_CTRL_PRESSED)|(irInBuf[i].Event.KeyEvent.dwControlKeyState & RIGHT_CTRL_PRESSED))
						{
							printf ("%s\n",languageParser.getValue("MSG_SERVICESTOP"));
							this->stop();
						}
					}
					break; 
			} 
		}
#endif
#ifdef NOT_WIN
		sleep(1);
#endif
	}
	this->terminate();
}
/*!
*This function is used to create a socket server and a thread listener for a protocol.
*/
int cserver::createServerAndListener(u_long port)
{
	/*!
	*Create the server socket.
	*/
	printf("%s\n",languageParser.getValue("MSG_SSOCKCREATE"));
	MYSERVER_SOCKET serverSocket;
	serverSocket.socket(AF_INET,SOCK_STREAM,0);
	MYSERVER_SOCKADDRIN sock_inserverSocket;
	if(serverSocket.getHandle()==INVALID_SOCKET)
	{
        	preparePrintError();
		printf("%s\n",languageParser.getValue("ERR_OPENP"));
        	endPrintError();		
		return 0;

	}
	printf("%s\n",languageParser.getValue("MSG_SSOCKRUN"));
	sock_inserverSocket.sin_family=AF_INET;
	sock_inserverSocket.sin_addr.s_addr=htonl(INADDR_ANY);
	sock_inserverSocket.sin_port=htons((u_short)port);

#ifdef NOT_WIN
	/*!
	*Under the unix environment the application needs some time before create a new socket
	*for the same address. To avoid this behavior we use the current code.
	*/
	int optvalReuseAddr=1;
	if(serverSocket.setsockopt(SOL_SOCKET,SO_REUSEADDR,(const char *)&optvalReuseAddr,sizeof(optvalReuseAddr))<0)
	{
        	preparePrintError();
		printf("%s setsockopt\n",languageParser.getValue("ERR_ERROR"));
        	endPrintError();
		return 0;
	}

#endif

	/*!
	*Bind the port.
	*/
	printf("%s\n",languageParser.getValue("MSG_BIND_PORT"));

	if(serverSocket.bind((sockaddr*)&sock_inserverSocket,sizeof(sock_inserverSocket))!=0)
	{
		preparePrintError();
		printf("%s\n",languageParser.getValue("ERR_BIND"));
        	endPrintError();
		return 0;
	}
	printf("%s\n",languageParser.getValue("MSG_PORT_BINDED"));

	/*!
	*Set connections listen queque to max allowable.
	*/
	printf("%s\n",languageParser.getValue("MSG_SLISTEN"));
	if (serverSocket.listen(SOMAXCONN))
	{ 
        	preparePrintError();
		printf("%s\n",languageParser.getValue("ERR_LISTEN"));
        	endPrintError();	
		return 0; 
	}

	printf("%s: %u\n",languageParser.getValue("MSG_LISTEN"),port);


	printf("%s\n",languageParser.getValue("MSG_LISTENTR"));
	/*!
	*Create the listen thread.
	*/
	listenThreadArgv* argv=new listenThreadArgv;
	argv->port=port;
	argv->serverSocket=serverSocket;
#ifdef WIN32
	unsigned int ID;
	_beginthreadex(NULL,0,&::listenServer,argv,0,&ID);
#endif
#ifdef HAVE_PTHREAD
	pthread_t ID;
	pthread_create(&ID, NULL, &::listenServer, (void *)(argv));
#endif
	return (ID)?1:0;
}
/*!
*This is the thread that listens for a new connection on the port specified by the protocol.
*/
#ifdef WIN32
unsigned int __stdcall listenServer(void* params)
#endif
#ifdef HAVE_PTHREAD
void * listenServer(void* params)
#endif
{
#ifdef NOT_WIN
	// Block SigTerm, SigInt, and SigPipe in threads
	sigset_t sigmask;
	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGPIPE);
	sigaddset(&sigmask, SIGINT);
	sigaddset(&sigmask, SIGTERM);
	sigprocmask(SIG_SETMASK, &sigmask, NULL);
#endif
	listenThreadArgv *argv=(listenThreadArgv*)params;
	MYSERVER_SOCKET serverSocket=argv->serverSocket;
	delete argv;

	MYSERVER_SOCKADDRIN asock_in;
	int asock_inLen=sizeof(asock_in);
	MYSERVER_SOCKET asock;
	while(!mustEndServer)
	{
		/*!
		*Accept connections.
		*Every new connection is sended to cserver::addConnection function;
		*this function sends connections between the various threads.
		*/
		asock=serverSocket.accept((struct sockaddr*)&asock_in,(LPINT)&asock_inLen);
		asock.setServerSocket(&serverSocket);
		if(asock.getHandle()==0)
			continue;
		if(asock.getHandle()==INVALID_SOCKET)
			continue;
		lserver->addConnection(asock,&asock_in);
	}
	/*!
	*When the flag mustEndServer is 1 end current thread and clean the socket used for listening.
	*/
	serverSocket.shutdown( 2);
	serverSocket.closesocket();
	/*!
	*Automatically free the current thread
	*/	
#ifdef WIN32
	_endthread();
#endif
#ifdef HAVE_PTHREAD
	pthread_exit(0);
#endif

	return 0;

}
/*!
*Returns the numbers of active connections the list.
*/
u_long cserver::getNumConnections()
{
	return nConnections;
}


/*!
*Get the verbosity value.
*/
u_long cserver::getVerbosity()
{
	return verbosity;
}

/*!
*Set the verbosity value.
*/
void  cserver::setVerbosity(u_long nv)
{
	verbosity=nv;
}
/*!
*Stop the execution of the server.
*/
void cserver::stop()
{
	mustEndServer=1;
}

void cserver::terminate()
{
	/*!
	*Stop the server execution.
	*/
	u_long i;
	for(i=0;i<nThreads;i++)
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
	/*!
	*Clean here the memory allocated.
	*/
	u_long threadsStopped=0;

	/*!
	*Wait before clean the threads that all the threads are stopped.
	*/

	for(i=0;i<nThreads;i++)
		threads[i].clean();

	for(;;)
	{
		threadsStopped=0;
		for(i=0;i<nThreads;i++)
			if(threads[i].isStopped())
				threadsStopped++;
		/*!
		*If all the threads are stopped break the loop.
		*/
		if(threadsStopped==nThreads)
			break;
	}
	/*!
	*If there are open connections close them.
	*/
	if(connections)
	{
		clearAllConnections();
	}
	vhostList.clean();
	languageParser.close();
	mimeManager.clean();
#ifdef WIN32
	/*!
	*Under WIN32 cleanup ISAPI.
	*/
	FreeEnvironmentStrings((LPTSTR)envString);
#endif	
	http::unloadProtocol(&languageParser);
	https::unloadProtocol(&languageParser);
	protocols.unloadProtocols(&languageParser);
	
	delete[] threads;
	if(verbosity>1)
	{
		printf("MyServer is stopped.\n\n");
	}
}
/*!
*Get the server administrator e-mail address.
*To change this use the main configuration file.
*/
char *cserver::getServerAdmin()
{
	return serverAdmin;
}
/*!
*Here is loaded the configuration of the server.
*The configuration file is a XML file.
*/
void cserver::initialize(int /*!OSVer*/)
{
	/*!
	*Store the default values.
	*/
	u_long nThreadsA=1;
	u_long nThreadsB=0;
	socketRcvTimeout = 10;
	useLogonOption = 1;
	connectionTimeout = SEC(25);
	lstrcpy(languageFile,"languages/english.xml");
	mustEndServer=0;
	verbosity=1;
	maxConnections=0;
	serverAdmin[0]='\0';
	
	/*!
	*If the myserver.xml files doesn't exist copy it from the default one.
	*/
	if(!MYSERVER_FILE::fileExists("myserver.xml"))
	{
			MYSERVER_FILE inputF;
			MYSERVER_FILE outputF;
			int ret=inputF.openFile("myserver.xml.default",MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_IFEXISTS);
			if(ret<1)
			{
				preparePrintError();
				printf("%s\n",languageParser.getValue("ERR_LOADMIME"));
				endPrintError();				
				return;
			}
			outputF.openFile("myserver.xml",MYSERVER_FILE_OPEN_WRITE|MYSERVER_FILE_OPEN_ALWAYS);
			char buffer[512];
			u_long nbr,nbw;
			for(;;)
			{
				inputF.readFromFile(buffer,512,&nbr );
				if(nbr==0)
					break;
				outputF.writeToFile(buffer,nbr,&nbw);
			}
			
			inputF.closeFile();
			outputF.closeFile();
	}		
	
	configurationFileManager.open("myserver.xml");
	char *data;


	data=configurationFileManager.getValue("VERBOSITY");
	if(data)
	{
		verbosity=(u_long)atoi(data);
	}
	data=configurationFileManager.getValue("LANGUAGE");
	if(data)
	{
		sprintf(languageFile,"languages/%s",data);	
	}

	data=configurationFileManager.getValue("BUFFER_SIZE");
	if(data)
	{
		buffersize=buffersize2=max((u_long)atol(data),81920);
	}
	data=configurationFileManager.getValue("CONNECTION_TIMEOUT");
	if(data)
	{
		connectionTimeout=SEC((u_long)atol(data));
	}

	data=configurationFileManager.getValue("NTHREADS_A");
	if(data)
	{
		nThreadsA=atoi(data);
	}
	data=configurationFileManager.getValue("NTHREADS_B");
	if(data)
	{
		nThreadsB=atoi(data);
	}
	/*!
	*The number of the threads used by the server is:
	*N_THREADS=nThreadsForCPU*CPU_COUNT+nThreadsAlwaysActive;
	*/
	nThreads=nThreadsA*getCPUCount()+nThreadsB;


	/*!
	*Get the max connections number to allow.
	*/
	data=configurationFileManager.getValue("MAX_CONNECTIONS");
	if(data)
	{
		maxConnections=atoi(data);
	}			
	
	data=configurationFileManager.getValue("SERVER_ADMIN");
	if(data)
	{
		lstrcpy(serverAdmin,data);
	}

	data=configurationFileManager.getValue("USE_LOGON_OPTIONS");
	if(data)
	{
		if(!lstrcmpi(data,"YES"))
			useLogonOption=1;
		else
			useLogonOption=0;
	}

	data=configurationFileManager.getValue("MAX_LOG_FILE_SIZE");
	if(data)
	{
		maxLogFileSize=(u_long)atol(data);
	}
	
	configurationFileManager.close();

}
/*!
*This function returns the max size of the logs file as defined in the 
*configuration file.
*/
int cserver::getMaxLogFileSize()
{
	return maxLogFileSize;
}
/*!
*Returns the connection timeout.
*/
u_long cserver::getTimeout()
{
	return connectionTimeout;
}
/*!
*This function add a new connection to the list.
*/
int cserver::addConnection(MYSERVER_SOCKET s,MYSERVER_SOCKADDRIN *asock_in)
{
	if(s.getHandle()==0)
		return 0;
	static int ret;
	ret=1;
	/*!
	*ip is the string containing the address of the remote host connecting to the server
	*myip is the address of the local host on the same network.
	*/
	char ip[MAX_IP_STRING_LEN];
	char myIp[MAX_IP_STRING_LEN];
	MYSERVER_SOCKADDRIN  localsock_in;

	memset(&localsock_in, 0, sizeof(localsock_in));

	int dim=sizeof(localsock_in);
	s.getsockname((MYSERVER_SOCKADDR*)&localsock_in,&dim);

	strncpy(ip, inet_ntoa(asock_in->sin_addr), MAX_IP_STRING_LEN); // NOTE: inet_ntop supports IPv6
	strncpy(myIp, inet_ntoa(localsock_in.sin_addr), MAX_IP_STRING_LEN); // NOTE: inet_ntop supports IPv6

	int port=ntohs((*asock_in).sin_port);/*!Port used by the client*/
	int myport=ntohs(localsock_in.sin_port);/*!Port connected to*/

	if(!addConnectionToList(s,asock_in,&ip[0],&myIp[0],port,myport,1))
	{
		/*!If we report error to add the connection to the thread*/
		ret=0;
		s.shutdown(2);/*!Shutdown the socket both on receive that on send*/
		s.closesocket();/*!Then close it*/
	}
	return ret;
}

/*!
*Add a new connection.
*A connection is defined using a CONNECTION struct.
*/
LPCONNECTION cserver::addConnectionToList(MYSERVER_SOCKET s,MYSERVER_SOCKADDRIN* /*asock_in*/,char *ipAddr,char *localIpAddr,int port,int localPort,int id)
{
	requestAccess(&connectionWriteAccess,id);
	u_long cs=sizeof(CONNECTION);
	LPCONNECTION nc=(CONNECTION*)malloc(cs);
	if(!nc)
		return NULL;
	nc->check_value = CONNECTION::check_value_const;
	nc->connectionBuffer[0]='\0';
	nc->socket=s;
	nc->toRemove=0;
	nc->forceParsing=0;
	nc->parsing=0;
	nc->port=(u_short)port;
	nc->timeout=clock();
	nc->dataRead = 0;
	nc->localPort=(u_short)localPort;
	strncpy(nc->ipAddr,ipAddr,MAX_IP_STRING_LEN);
	strncpy(nc->localIpAddr,localIpAddr,MAX_IP_STRING_LEN);
	nc->next = connections;
	nc->host = (void*)lserver->vhostList.getvHost(0,localIpAddr,(u_short)localPort);
	if(nc->host == 0) /* No vhost for the connection so bail */
	{
		free(nc);
		return 0;
	}
	int doSSLhandshake=0;
	if(((vhost*)nc->host)->protocol > 1000	)
	{
		doSSLhandshake=1;
	}
	else if(((vhost*)nc->host)->protocol==PROTOCOL_UNKNOWN)
	{		
		/*Do nothing ATM*/
	}
	
	/*!
	*Do the SSL handshake if required.
	*/
	if(doSSLhandshake)
	{
#ifndef DO_NOT_USE_SSL		
		SSL_CTX* ctx=((vhost*)nc->host)->getSSLContext();
		nc->socket.setSSLContext(ctx);
#endif		
		int ret=nc->socket.sslAccept();
		if(ret<0)
		{
			/*
			*Free the connection on errors.
			*/
			free(nc);
			return 0;
		}
	}
	
	nc->login[0]='\0';
	nc->nTries=0;
	nc->password[0]='\0';
	nc->protocolBuffer=0;

	if(nc->host==0)
	{
		free(nc);
		return 0;
	}
    connections=nc;
	nConnections++;
	
	/*
	If defined maxConnections and the number of active connections is bigger than it
	*say to the protocol that will parse the connection to remove it from the active
	*connections list.
	*/
	if(maxConnections && (nConnections>maxConnections))
		nc->toRemove=CONNECTION_REMOVE_OVERLOAD;
	
	terminateAccess(&connectionWriteAccess,id);
	return nc;
}

/*!
*Delete a connection from the list.
*/
int cserver::deleteConnection(LPCONNECTION s,int id)
{
	if(!s)
		return 0;
	requestAccess(&connectionWriteAccess,id);
	int ret=0,err;
	/*!
	*Remove the connection from the active connections list.
	*/
	LPCONNECTION prev=0;
	for(LPCONNECTION i=connections;i;i=i->next )
	{
		if(i->socket == s->socket)
		{
			if(connectionToParse)
				if(connectionToParse->socket==s->socket)
					connectionToParse=connectionToParse->next;

			if(prev)
				prev->next =i->next;
			else
				connections=i->next;
			ret=1;
			break;
		}
		else
		{
			prev=i;
		}
	}
	terminateAccess(&connectionWriteAccess,id);
	nConnections--;
	/*!
	*Close the socket communication.
	*/
	s->socket.shutdown(SD_BOTH);
	char buffer[256];
	int buffersize=256;
	do
	{
		err=s->socket.recv(buffer,buffersize,0);
	}while(err!=-1);
	s->socket.closesocket();
	if(s->protocolBuffer)
		free(s->protocolBuffer);
	free(s);
	return ret;
}
/*!
*Get a connection to parse.
*/
LPCONNECTION cserver::getConnectionToParse(int id)
{
	if(connections==0)
		return 0;
	requestAccess(&connectionWriteAccess,id);
	if(connectionToParse)
	{
		/*!Be sure that connectionToParse is a valid connection struct*/
		if(connectionToParse->check_value!=CONNECTION::check_value_const)
			connectionToParse=connections;
		else
			connectionToParse=connectionToParse->next;
	}
	else
	{/*!Restart loop if the connectionToParse points to the last element*/
		connectionToParse=connections;
	}
	if(connectionToParse==0)
		connectionToParse=connections;
	terminateAccess(&connectionWriteAccess,id);
	return connectionToParse;
}
/*!
*Delete all the active connections.
*/
void cserver::clearAllConnections()
{
	/*!
	*Keep access to the connections list
	*/
	requestAccess(&connectionWriteAccess,1);
	LPCONNECTION c=connections;
	LPCONNECTION next=0;
	while(c)
	{
		next=c->next;
		deleteConnection(c,1);
		c=next;
	}
	/*!
	*Reset everything
	*/
	nConnections=0;
	connections=0;
	connectionToParse=0;
	terminateAccess(&connectionWriteAccess,1);
}


/*!
*Find a connection passing its socket.
*/
LPCONNECTION cserver::findConnection(MYSERVER_SOCKET a)
{
	requestAccess(&connectionWriteAccess,1);
	LPCONNECTION c;
	for(c=connections;c;c=c->next )
	{
		if(c->socket==a)
			return c;
	}
	terminateAccess(&connectionWriteAccess,1);
	return NULL;
}

/*!
*Returns the full path of the binaries folder.
*/
char *cserver::getPath()
{
	return path;
}
/*!
*Returns the name of the server(the name of the current PC).
*/
char *cserver::getServerName()
{
	return serverName;
}
/*!
*Returns if we use the logon.
*/
int cserver::mustUseLogonOption() 
{
	return useLogonOption;
}
/*!
*Gets the number of threads.
*/
u_long cserver::getNumThreads()
{
	return nThreads;
}
/*!
*Returns a comma-separated local machine IPs list.
*For example: 192.168.0.1,61.62.63.64,65.66.67.68.69
*/
char *cserver::getAddresses()
{
	return ipAddresses;
}
/*!
*Get the dynamic protocol to use for that connection.
*/
dynamic_protocol* cserver::getDynProtocol(char *protocolName)
{
	return protocols.getDynProtocol(protocolName);
}
