/*
MyServer
Copyright (C) 2002, 2003, 2004 The MyServer Team
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

#ifndef MSCGI_H
#define MSCGI_H
#include "../stdafx.h"
#include "../include/response_request.h"
#include "../include/connectionstruct.h"
#include "../include/mime_manager.h"
#include "../include/cgi.h"
#include "../include/filemanager.h"
#include "../include/http_headers.h"
#include "../include/http_data_handler.h"
#include "../include/dynamiclib.h"
struct HttpThreadContext;

struct MsCgiData
{
	char *envString;
	HttpThreadContext* td;
	int errorPage;
	File stdOut;

};
typedef int (*CGIMAIN)(char*, MsCgiData*); 

class MsCgi : public HttpDataHandler
{
public:
	/*!
	*Functions to Load and free the MSCGI library.
	*/
	static int load(XmlParser*);
	static int unload();
	/*!
	*Use this to send a MSCGI file through the HTTP protocol.
	*/
	int send(HttpThreadContext*, ConnectionPtr s, const char* exec,
                char* cmdLine=0, int execute=0, int onlyHeader=0);
	typedef int (*CGIMAIN)(char*, MsCgiData*); 
};
#endif
