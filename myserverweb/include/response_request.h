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

#include <string>
#include "../include/hash_map.h"

#ifndef RESPONSE_REQUEST_STRUCTS_H
#define RESPONSE_REQUEST_STRUCTS_H

using namespace std;

/*! Max length for a HTTP response fields. */
#define HTTP_RESPONSE_VER_DIM 10
#define HTTP_RESPONSE_SERVER_NAME_DIM 64
#define HTTP_RESPONSE_CONTENT_TYPE_DIM 48
#define HTTP_RESPONSE_CONTENT_RANGE_DIM 32
#define HTTP_RESPONSE_CONNECTION_DIM 32
#define HTTP_RESPONSE_MIME_VER_DIM 8
#define HTTP_RESPONSE_COOKIE_DIM 8192
#define HTTP_RESPONSE_CONTENT_LENGTH_DIM 8
#define HTTP_RESPONSE_ERROR_TYPE_DIM 32
#define HTTP_RESPONSE_LOCATION_DIM MAX_PATH
#define HTTP_RESPONSE_DATE_DIM 32
#define HTTP_RESPONSE_DATE_EXPIRES_DIM 32
#define HTTP_RESPONSE_CACHE_CONTROL_DIM 64
#define HTTP_RESPONSE_AUTH_DIM 256
#define HTTP_RESPONSE_OTHER_DIM 512
#define HTTP_RESPONSE_LAST_MODIFIED_DIM 32

/*! Max length for a HTTP request fields. */
#define HTTP_REQUEST_CMD_DIM 16
#define HTTP_REQUEST_VER_DIM 10
#define HTTP_REQUEST_ACCEPT_DIM 4096
#define HTTP_REQUEST_AUTH_DIM 16
#define HTTP_REQUEST_CONNECTION_DIM 32
#define HTTP_REQUEST_USER_AGENT_DIM 128
#define HTTP_REQUEST_COOKIE_DIM 2048
#define HTTP_REQUEST_CONTENT_TYPE_DIM 96
#define HTTP_REQUEST_CONTENT_LENGTH_DIM 12
#define HTTP_REQUEST_DATE_DIM 32
#define HTTP_REQUEST_DATE_EXPIRES_DIM 32
#define HTTP_REQUEST_IF_MODIFIED_SINCE_DIM 35
#define HTTP_REQUEST_LAST_MODIFIED_DIM 32
#define HTTP_REQUEST_URI_DIM 1024
#define HTTP_REQUEST_URI_OPTS_DIM 1024
#define HTTP_REQUEST_REFERER_DIM MAX_PATH
#define HTTP_REQUEST_HOST_DIM 128
#define HTTP_REQUEST_OTHER_DIM 256
#define HTTP_REQUEST_CACHE_CONTROL_DIM 64
#define HTTP_REQUEST_RANGE_TYPE_DIM 16

struct HttpHeader
{
  virtual string* getValue(const char* name, string* out)=0;
  virtual ~HttpHeader(){}
};

/*!
 *Structure to describe an HTTP response
 */
struct HttpResponseHeader : public HttpHeader
{
  struct Entry
  {
    string *name;
    string *value;
		Entry()
		{
			name = new string();
			value = new string();
		}
		~Entry()
		{
			delete name;
			delete value;
			
		}
  };
	int httpStatus;
	string ver;	
	string serverName;
	string contentType;
	string connection;
	string mimeVer;
	string cookie;
	string contentLength;
	string errorType;
	string lastModified;
	string location;
	string date;		
	string dateExp;	
	string auth;
	string cacheControl;
	string contentRange;
	HashMap<string,HttpResponseHeader::Entry*> other;	
  HttpResponseHeader();
  ~HttpResponseHeader();

  virtual string* getValue(const char* name, string* out);

  void free();
};

/*!
 *Structure to describe an HTTP request.
 */
struct HttpRequestHeader : public HttpHeader
{
  struct Entry
  {
    string *name;
    string *value;
		Entry()
		{
			name = new string();
			value = new string();
		}
		~Entry()
		{
			delete name;
			delete value;
			
		}

  };
	string cmd;		
	string ver;		
	string accept;
	string auth;
	string ifModifiedSince;
	string connection;
	string userAgent;
	string cookie;
	string contentType;
	string contentLength;
	string date;
	string dateExp;	
	string lastModified;	
	string uri;
	string cacheControl;
	string uriOpts;		
	char *uriOptsPtr;		
	string referer;	
	string host;			
	string rangeType;	
	u_long  rangeByteBegin;
	u_long  rangeByteEnd;
	int uriEndsWithSlash;
	
	/*! Digest authorization scheme stuff.  */
	char digestRealm[48+1];
	char digestOpaque[48+1];
	char digestNonce[48+1];
	char digestCnonce[48+1];
	char digestUri[1024+1];
	char digestMethod[16+1];		
	char digestUsername[48+1];
	char digestResponse[48+1];
	char digestQop[16+1];
	char digestNc[10+1];


	HashMap<string, HttpRequestHeader::Entry*> other;
  virtual string* getValue(const char* name, string* out);

  HttpRequestHeader();
  ~HttpRequestHeader();
  void free();
 
};

#endif
