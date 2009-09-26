/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2002, 2003, 2004, 2008, 2009 Free Software Foundation, Inc.
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <string>
#include <include/base/hash_map/hash_map.h>
#include <include/protocol/http/http_header.h>

#ifndef HTTP_RESPONSE_H
# define HTTP_RESPONSE_H

using namespace std;

/*! Max length for a HTTP response fields. */
# define HTTP_RESPONSE_VER_DIM 10
# define HTTP_RESPONSE_SERVER_NAME_DIM 64
# define HTTP_RESPONSE_CONTENT_TYPE_DIM 48
# define HTTP_RESPONSE_CONTENT_RANGE_DIM 32
# define HTTP_RESPONSE_CONNECTION_DIM 32
# define HTTP_RESPONSE_MIME_VER_DIM 8
# define HTTP_RESPONSE_COOKIE_DIM 8192
# define HTTP_RESPONSE_CONTENT_LENGTH_DIM 8
# define HTTP_RESPONSE_ERROR_TYPE_DIM 32
# define HTTP_RESPONSE_LOCATION_DIM MAX_PATH
# define HTTP_RESPONSE_DATE_DIM 32
# define HTTP_RESPONSE_DATE_EXPIRES_DIM 32
# define HTTP_RESPONSE_CACHE_CONTROL_DIM 64
# define HTTP_RESPONSE_AUTH_DIM 256
# define HTTP_RESPONSE_OTHER_DIM 512
# define HTTP_RESPONSE_LAST_MODIFIED_DIM 32

/*!
 *Structure to describe an HTTP response
 */
struct HttpResponseHeader : public HttpHeader
{
  const static int INFORMATIONAL = 100;
  const static int SUCCESSFUL = 200;
  const static int REDIRECTION = 300;
  const static int CLIENT_ERROR = 400;
  const static int SERVER_ERROR = 500;

  struct Entry
  {
    string *name;
    string *value;
		Entry()
		{
			name = new string();
			value = new string();
		}

		Entry(string& n, string& v)
		{
			name = new string();
			value = new string();

			name->assign(n);
			value->assign(v);
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
	string cookie;
	string contentLength;
	string errorType;

	HashMap<string,HttpResponseHeader::Entry*> other;
  HttpResponseHeader();
  ~HttpResponseHeader();

  int getStatusType();

  virtual string* getValue(const char* name, string* out);
  virtual string* setValue(const char* name, const char* in);
  void free();
};

#endif
