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
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef CSERVER_IN
#define CSERVER_IN

#include "../stdafx.h"
#include "../include/clientsThread.h"
#include "../include/utility.h"
#include "../include/cXMLParser.h"
#include "../include/utility.h"
#include "../include/connectionstruct.h"
#include "../include/sockets.h"
#include "../include/MIME_manager.h"
#include "../include/vhosts.h"
#include "../include/protocols_manager.h"
#include "../include/connectionstruct.h"
#include "../include/log_manager.h"

/*!
 *Definition for new threads entry-point.
 */
#ifdef WIN32
unsigned int __stdcall listenServer(void* pParam);
#endif
#ifdef NOT_WIN
void* listenServer(void* pParam);
#endif

/*!
 *On systems with a MAX_PATH limit use it.
 */
#ifdef MAX_PATH
#define CONF_FILES_MAX_PATH MAX_PATH
#else
#define CONF_FILES_MAX_PATH 260
#endif

/*!
 *Define the max number of network interfaces to consider.
 */
#define MAX_ALLOWED_IPs 8
/*
 *Defined in myserver.cpp
 */
extern int rebootMyServerConsole;

struct listenThreadArgv
{
	u_long port;
	MYSERVER_SOCKET *serverSocket;
	int SSLsocket;
};


class cserver
{
  friend class ClientsThread;
#ifdef WIN32
	friend  unsigned int __stdcall listenServer(void* pParam);
	friend  unsigned int __stdcall startClientsTHREAD(void* pParam);
#endif
#ifdef HAVE_PTHREAD
	friend  void* listenServer(void* pParam);
	friend  void* startClientsTHREAD(void* pParam);
#endif
#ifdef WIN32
	friend int __stdcall control_handler (u_long control_type);
#endif
#ifdef NOT_WIN
	friend int control_handler (u_long control_type);
#endif
private:
  int currentThreadID;
	void stopThreads();
	/*! Used when rebooting to load new configuration files.  */
	int pausing;
	protocols_manager protocols;
	cXMLParser configurationFileManager;
	cXMLParser languageParser;
  int autoRebootEnabled;
  int toReboot;
  MYSERVER_LOG_MANAGER *logManager;
  int serverReady;
	u_long verbosity;
	u_long buffersize;
	u_long buffersize2;
	u_long getNumConnections();
	/*! Buffer that contains all the local machine IP values.  */
	char ipAddresses[MAX_IP_STRING_LEN*MAX_ALLOWED_IPs];
	char serverName[MAX_COMPUTERNAME_LENGTH+1];
	char *path;
	char serverAdmin[32];
	int initialize(int);
	LPCONNECTION addConnectionToList(MYSERVER_SOCKET s,MYSERVER_SOCKADDRIN *asock_in,
                                   char *ipAddr,char *localIpAddr,int port,
                                   int localPort,int);
  u_long nConnections;
	u_long maxConnections;
	u_long maxConnectionsToAccept;
	void clearAllConnections();
	int deleteConnection(LPCONNECTION,int);
	u_long connectionTimeout;
	u_long socketRcvTimeout;
	u_long maxLogFileSize;
	int createServerAndListener(u_long);
	int loadSettings();
	myserver_mutex *connections_mutex;
	LPCONNECTION connectionToParse;
	u_long nStaticThreads;
  u_long nMaxThreads;
  u_long nThreads;

  myserver_mutex *threads_mutex;
  ClientsThread *threads;

  int purgeThreads();
	LPCONNECTION connections;
	void createListenThreads();
	int reboot();
	u_int listeningThreads;
	char *languageFile;
	char *languages_path;
	char *main_configuration_file;
	char *vhost_configuration_file;
	char *mime_configuration_file;
public:
  int countAvailableThreads();
  int addThread(int staticThread = 0);
  int removeThread(u_long ID);
  int isServerReady();
  protocols_manager *getProtocolsManager();
  void disableAutoReboot();
  void enableAutoReboot();
  int isAutorebootEnabled();
  void rebootOnNextLoop();
  char *getMainConfFile();
  char *getVhostConfFile();
  char *getMIMEConfFile();
  char *getLanguagesPath();
  char *getLanguageFile();
	cserver();
	~cserver();
	dynamic_protocol* getDynProtocol(char *protocolName);
	int addConnection(MYSERVER_SOCKET,MYSERVER_SOCKADDRIN*);
	int connections_mutex_lock();
	int connections_mutex_unlock();
  LPCONNECTION getConnections();
	LPCONNECTION getConnectionToParse(int);
	LPCONNECTION findConnectionBySocket(MYSERVER_SOCKET);
	LPCONNECTION findConnectionByID(u_long ID);
	u_long getTimeout();
	int getListeningThreadCount();
	void increaseListeningThreadCount();
	void decreaseListeningThreadCount();
	char *getAddresses();
	void *envString;
	vhostmanager *vhostList;
	MIME_Manager mimeManager;
	char  *getPath();
	u_long getNumThreads();
	char  *getDefaultFilenamePath(u_long ID=0);
	char  *getServerName();
	u_long  getVerbosity();
	char *getServerAdmin();
	int getMaxLogFileSize();
	int  mustUseLogonOption();
	void  setVerbosity(u_long);
	void start();
	void stop();
	void finalCleanup();
	int terminate();
  int logWriteln(char*);
  int logPreparePrintError();
  int logEndPrintError();  
  int logLockAccess();
  int logUnlockAccess(); 
  int setLogFile(char*);
	u_long getBuffersize();
	u_long getBuffersize2();
}; 
extern class cserver *lserver;
#ifdef WIN32
LRESULT CALLBACK MainWndProc(HWND,UINT,WPARAM,LPARAM); 
#endif

#endif
