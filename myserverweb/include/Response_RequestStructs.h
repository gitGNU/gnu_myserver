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


#ifndef RESPONSE_REQUESTSTRUCTS_H
#define RESPONSE_REQUESTSTRUCTS_H
#define HTTP_RESPONSE_VER_DIM 10
#define HTTP_RESPONSE_SERVER_NAME_DIM 32
#define HTTP_RESPONSE_CONTENT_TYPE_DIM 32
#define HTTP_RESPONSE_CONNECTION_DIM 32
#define HTTP_RESPONSE_MIMEVER_DIM 8
#define HTTP_RESPONSE_P3P_DIM 200
#define HTTP_RESPONSE_COOKIE_DIM 81920
#define HTTP_RESPONSE_CONTENT_LENGTH_DIM 8
#define HTTP_RESPONSE_ERROR_TYPE_DIM 32
#define HTTP_RESPONSE_LOCATION_DIM MAX_PATH
#define HTTP_RESPONSE_DATE_DIM 32
#define HTTP_RESPONSE_CONTENT_ENCODING_DIM 16
#define HTTP_RESPONSE_TRANSFER_ENCODING_DIM 16
#define HTTP_RESPONSE_DATEEXP_DIM 32
#define HTTP_RESPONSE_CACHE_CONTROL_DIM 64
#define HTTP_RESPONSE_AUTH_DIM 48
#define HTTP_RESPONSE_OTHER_DIM 256
#define HTTP_RESPONSE_LAST_MODIFIED_DIM 32

/*!
*Structure to describe an HTTP response
*/
struct HTTP_RESPONSE_HEADER
{
	int httpStatus;
	char VER[HTTP_RESPONSE_VER_DIM+1];	
	char SERVER_NAME[HTTP_RESPONSE_SERVER_NAME_DIM+1];
	char CONTENT_TYPE[HTTP_RESPONSE_CONTENT_TYPE_DIM+1];
	char CONNECTION[HTTP_RESPONSE_CONNECTION_DIM+1];
	char MIMEVER[HTTP_RESPONSE_MIMEVER_DIM+1];
	char P3P[HTTP_RESPONSE_P3P_DIM+1];
	char COOKIE[HTTP_RESPONSE_COOKIE_DIM+1];
	char CONTENT_LENGTH[HTTP_RESPONSE_CONTENT_LENGTH_DIM+1];
	char ERROR_TYPE[HTTP_RESPONSE_ERROR_TYPE_DIM+1];
	char LAST_MODIFIED[HTTP_RESPONSE_LAST_MODIFIED_DIM+1];
	char LOCATION[HTTP_RESPONSE_LOCATION_DIM+1];
	char DATE[HTTP_RESPONSE_DATE_DIM+1];		
	char DATEEXP[HTTP_RESPONSE_DATEEXP_DIM+1];	
	char AUTH[HTTP_RESPONSE_AUTH_DIM+1];
	char OTHER[HTTP_RESPONSE_OTHER_DIM+1];	
	char CONTENT_ENCODING[HTTP_RESPONSE_CONTENT_ENCODING_DIM+1];
	char TRANSFER_ENCODING[HTTP_RESPONSE_TRANSFER_ENCODING_DIM+1];
	char CACHE_CONTROL[HTTP_RESPONSE_CACHE_CONTROL_DIM+1];

};

#define HTTP_REQUEST_CMD_DIM 16		
#define HTTP_REQUEST_VER_DIM 10		
#define HTTP_REQUEST_ACCEPT_DIM 128
#define HTTP_REQUEST_AUTH_DIM 48
#define HTTP_REQUEST_ACCEPTENC_DIM 64	
#define HTTP_REQUEST_ACCEPTLAN_DIM 64	
#define HTTP_REQUEST_ACCEPTCHARSET_DIM 64
#define HTTP_REQUEST_CONNECTION_DIM 32
#define HTTP_REQUEST_USER_AGENT_DIM 128
#define HTTP_REQUEST_COOKIE_DIM 2048
#define HTTP_REQUEST_CONTENT_TYPE_DIM 96
#define HTTP_REQUEST_CONTENT_LENGTH_DIM 8
#define HTTP_REQUEST_CONTENT_ENCODING_DIM 16
#define HTTP_REQUEST_TRANSFER_ENCODING_DIM 16
#define HTTP_REQUEST_DATE_DIM 32		
#define HTTP_REQUEST_DATEEXP_DIM 32	
#define HTTP_REQUEST_MODIFIED_SINCE_DIM 32
#define HTTP_REQUEST_LAST_MODIFIED_DIM 32	
#define HTTP_REQUEST_URI_DIM 1024
#define HTTP_REQUEST_PRAGMA_DIM 200
#define HTTP_REQUEST_IF_MODIFIED_SINCE_DIM 35
#define HTTP_REQUEST_URIOPTS_DIM 1024		
#define HTTP_REQUEST_REFERER_DIM MAX_PATH	
#define HTTP_REQUEST_FROM_DIM MAX_PATH
#define HTTP_REQUEST_HOST_DIM 128			
#define HTTP_REQUEST_OTHER_DIM 256
#define HTTP_REQUEST_CACHE_CONTROL_DIM 64
#define HTTP_REQUEST_RANGETYPE_DIM 12		
#define HTTP_REQUEST_RANGEBYTEBEGIN_DIM 10
#define HTTP_REQUEST_RANGEBYTEEND_DIM 10
/*!
*Structure to describe an HTTP request.
*/

struct HTTP_REQUEST_HEADER
{
	char CMD[HTTP_REQUEST_CMD_DIM+1];		
	char VER[HTTP_REQUEST_VER_DIM+1];		
	char ACCEPT[HTTP_REQUEST_ACCEPT_DIM+1];
	char CONTENT_ENCODING[HTTP_REQUEST_CONTENT_ENCODING_DIM+1];
	char TRANSFER_ENCODING[HTTP_REQUEST_TRANSFER_ENCODING_DIM+1];
	char AUTH[HTTP_REQUEST_AUTH_DIM+1];
	char ACCEPTENC[HTTP_REQUEST_ACCEPTENC_DIM+1];	
	char ACCEPTLAN[HTTP_REQUEST_ACCEPTLAN_DIM+1];	
	char ACCEPTCHARSET[HTTP_REQUEST_ACCEPTCHARSET_DIM+1];
	char IF_MODIFIED_SINCE[HTTP_REQUEST_IF_MODIFIED_SINCE_DIM+1];
	char CONNECTION[HTTP_REQUEST_CONNECTION_DIM+1];
	char USER_AGENT[HTTP_REQUEST_USER_AGENT_DIM+1];
	char COOKIE[HTTP_REQUEST_COOKIE_DIM+1];
	char CONTENT_TYPE[HTTP_REQUEST_CONTENT_TYPE_DIM+1];
	char CONTENT_LENGTH[HTTP_REQUEST_CONTENT_LENGTH_DIM+1];
	char DATE[HTTP_REQUEST_DATE_DIM+1];
	char DATEEXP[HTTP_REQUEST_DATEEXP_DIM+1];	
	char MODIFIED_SINCE[HTTP_REQUEST_MODIFIED_SINCE_DIM+1];
	char LAST_MODIFIED[HTTP_REQUEST_LAST_MODIFIED_DIM+1];	
	char URI[HTTP_REQUEST_URI_DIM+1];
	char CACHE_CONTROL[HTTP_REQUEST_CACHE_CONTROL_DIM+1];
	char PRAGMA[HTTP_REQUEST_PRAGMA_DIM+1];
	char URIOPTS[HTTP_REQUEST_URIOPTS_DIM+1];		
	char *URIOPTSPTR;		
	char REFERER[HTTP_REQUEST_REFERER_DIM+1];	
	char FROM[HTTP_REQUEST_FROM_DIM+1];
	char HOST[HTTP_REQUEST_HOST_DIM+1];			
	char OTHER[HTTP_REQUEST_OTHER_DIM+1];
	char RANGETYPE[HTTP_REQUEST_RANGETYPE_DIM+1];		
	char RANGEBYTEBEGIN[HTTP_REQUEST_RANGEBYTEBEGIN_DIM+1];
	char RANGEBYTEEND[HTTP_REQUEST_RANGEBYTEEND_DIM+1];
	int uriEndsWithSlash;
}; 
#endif
