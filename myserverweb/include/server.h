/*
MyServer
Copyright (C) 2002, 2003, 2004, 2005, 2006 The MyServer Team
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef SERVER_IN
#define SERVER_IN

#include "../stdafx.h"
#include "../include/clients_thread.h"
#include "../include/utility.h"
#include "../include/xml_parser.h"
#include "../include/utility.h"
#include "../include/connection.h"
#include "../include/sockets.h"
#include "../include/mime_manager.h"
#include "../include/vhosts.h"
#include "../include/protocols_manager.h"
#include "../include/connection.h"
#include "../include/log_manager.h"
#include "../include/filters_factory.h"
#include "../include/dyn_filter.h"
#include "../include/hash_map.h"
#include "../include/home_dir.h"
#include <string>
using namespace std;

/*!
 *Definition for new threads entry-point.
 */
#ifdef WIN32
unsigned int __stdcall listenServer(void* pParam);
#endif
#ifdef NOT_WIN
void* listenServer(void* pParam);
#endif

extern int rebootMyServerConsole;

struct listenThreadArgv
{
	u_long port;
	Socket *serverSocket;
	int SSLsocket;
};

class Server
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

	/*! Singleton instance.  Call createInstance before use it.  */
	static Server* instance;

	/*! Do not allow to create directly objects.  */
	Server();

	HomeDir homeDir;
  HashMap<string, string*> hashedData;
  FiltersFactory filtersFactory;
  DynamicFiltersManager filters;
  u_long uid;
  u_long gid;
  int currentThreadID;
	void stopThreads();
	/*! Used when rebooting to load new configuration files.  */
	int pausing;
	ProtocolsManager protocols;
	XmlParser configurationFileManager;
	XmlParser languageParser;
  int autoRebootEnabled;
  int toReboot;
  LogManager logManager;
  int serverReady;
	u_long verbosity;
  u_long throttlingRate;
	u_long buffersize;
	u_long buffersize2;
	u_long getNumConnections();
	/*! Buffer that contains all the local machine IP values.  */
	string *ipAddresses;
	char serverName[HOST_NAME_MAX + 1];
	string* path;
  string* externalPath;
	string* serverAdmin;
	int initialize(int);
	ConnectionPtr addConnectionToList(Socket s, MYSERVER_SOCKADDRIN *asock_in,
                                    char *ipAddr, char *localIpAddr,
                                    u_short port, u_short localPort, int);
  u_long nConnections;
	u_long maxConnections;
	u_long maxConnectionsToAccept;
	void clearAllConnections();
	int freeHashedData();
	int deleteConnection(ConnectionPtr, int, int = 1);
	u_long connectionTimeout;
	u_long maxLogFileSize;
	int createServerAndListener(u_short);
	int loadSettings();
	Mutex* connectionsMutex;
	ConnectionPtr connectionToParse;
	u_long nStaticThreads;
  u_long nMaxThreads;
  u_long nThreads;

  Mutex* threadsMutex;
  ClientsThread* threads;

  int purgeThreads();
	ConnectionPtr connections;
	void createListenThreads();
	int reboot();
	u_int listeningThreads;
	string* languageFile;
	string* languagesPath;
	string* mainConfigurationFile;
	string* vhostConfigurationFile;
	string* mimeConfigurationFile;
public:
	HomeDir* getHomeDir();
	static void createInstance();
	static Server* getInstance()
	{
		return instance;
	}

  const char* getHashedData(const char* name);
  FiltersFactory* getFiltersFactory();
	int getMaxThreads();
  u_long getUid();
  u_long getGid();
  int countAvailableThreads();
  int addThread(int staticThread = 0);
  int removeThread(u_long ID);
  int isServerReady();
  ProtocolsManager* getProtocolsManager();
  void disableAutoReboot();
  void enableAutoReboot();
  int isAutorebootEnabled();
  void rebootOnNextLoop();
  const char* getMainConfFile();
  const char* getVhostConfFile();
  const char* getMIMEConfFile();
  const char* getLanguagesPath();
  const char* getLanguageFile();
  const char* getExternalPath();
  XmlParser* getLanguageParser();
	~Server();
	DynamicProtocol* getDynProtocol(const char *protocolName);
	int addConnection(Socket,MYSERVER_SOCKADDRIN*);
	int connectionsMutexLock();
	int connectionsMutexUnlock();
  ConnectionPtr getConnections();
	ConnectionPtr getConnection(int);
	ConnectionPtr findConnectionBySocket(Socket);
	ConnectionPtr findConnectionByID(u_long ID);
	u_long getTimeout();
	int getListeningThreadCount();
	void increaseListeningThreadCount();
	void decreaseListeningThreadCount();
	const char *getAddresses();
	void *envString;
	VhostManager *vhostList;
	MimeManager *mimeManager;
	const char *getPath();
	u_long getNumThreads();
	const char *getDefaultFilenamePath(u_long ID = 0);
	const char *getServerName();
	u_long getVerbosity();
	const char *getServerAdmin();
	int getMaxLogFileSize();
	int mustUseLogonOption();
	void setVerbosity(u_long);
	void start();
	void stop();
	void finalCleanup();
	int terminate();
  int logWriteln(const char*);
  int logWriteln(string const &str)
    {return logWriteln(str.c_str());};
  int logPreparePrintError();
  int logEndPrintError();
  int logLockAccess();
  int logUnlockAccess();
  int setLogFile(char*);
	u_long getBuffersize();
	u_long getBuffersize2();
  u_long getThrottlingRate();
};

#endif
