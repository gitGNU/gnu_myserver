/*
MyServer
Copyright (C) 2002, 2003, 2004, 2007 The MyServer Team
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

#ifndef SCGI_H
#define SCGI_H

#include "../stdafx.h"
#include "../include/http_headers.h"
#include "../include/utility.h"
#include "../include/sockets.h"
#include "../include/vhosts.h"
#include "../include/http_constants.h"
#include "../include/connection.h"
#include "../include/stringutils.h"
#include "../include/thread.h"
#include "../include/mutex.h"
#include "../include/http_data_handler.h"
#include "../include/hash_map.h"
#include "../include/process_server_manager.h"
#include "../include/filters_chain.h"
#include <string>

using namespace std;


typedef ProcessServerManager::Server ScgiServer;


struct ScgiContext
{
	HttpThreadContext* td;
  ScgiServer* server;
	Socket sock;
	File tempOut;
};

class Scgi : public HttpDataHandler
{
public:
  static int getTimeout();
  static void setTimeout(int);
	Scgi();
	static int load(XmlParser*);
	int send(HttpThreadContext* td, ConnectionPtr connection,
                  const char* scriptpath, const char *cgipath, int execute,
                  int onlyHeader);
	static int unload();
private:
	static ProcessServerManager *processServerManager;
	static int timeout;
	static int initialized;

	int appendDataToHTTPChannel(HttpThreadContext* td, char* buffer, 
															u_long size, File* appendFile,
															FiltersChain *chain,
															bool append, bool useChunks);
	Socket getScgiConnection();
	int sendPostData(ScgiContext* ctx);
	int sendResponse(ScgiContext* ctx, int onlyHeader, FiltersChain*);
	int buildScgiEnvironmentString(HttpThreadContext*, char*, char*);
	int sendNetString(ScgiContext*, const char*, int);
	ScgiServer* isScgiServerRunning(const char*);
  ScgiServer* runScgiServer(ScgiContext*, const char*);
	ScgiServer* connect(ScgiContext*, const char*);
};
#endif
