/*
*myServer
*Copyright (C) 2002 The MyServer team
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

#ifndef ISAPI_H
#define ISAPI_H

#ifdef WIN32
#include "../stdafx.h"
#include "../include/http.h"
#include "../include/utility.h"
#include "../include/cserver.h"
#include "../include/HTTPmsg.h"
#include "../include/connectionstruct.h"


typedef LPVOID HCONN;

#define HSE_VERSION_MAJOR 5
#define HSE_VERSION_MINOR 1
#define HSE_LOG_BUFFER_LEN 80
#define HSE_MAX_EXT_DLL_NAME_LEN 256

#define HSE_STATUS_SUCCESS 1
#define HSE_STATUS_SUCCESS_AND_KEEP_CONN 2
#define HSE_STATUS_PENDING 3
#define HSE_STATUS_ERROR 4

#define HSE_REQ_BASE 0
#define HSE_REQ_SEND_URL_REDIRECT_RESP (HSE_REQ_BASE + 1)
#define HSE_REQ_SEND_URL (HSE_REQ_BASE + 2)
#define HSE_REQ_SEND_RESPONSE_HEADER (HSE_REQ_BASE + 3)
#define HSE_REQ_DONE_WITH_SESSION (HSE_REQ_BASE + 4)
#define HSE_REQ_END_RESERVED 1000
#define HSE_REQ_MAP_URL_TO_PATH (HSE_REQ_END_RESERVED+1)
#define HSE_REQ_GET_SSPI_INFO (HSE_REQ_END_RESERVED+2)
#define HSE_REQ_TRANSMIT_FILE (HSE_REQ_END_RESERVED+6)
#define HSE_REQ_MAP_URL_TO_PATH_EX (HSE_REQ_END_RESERVED+12)
#define HSE_REQ_ASYNC_READ_CLIENT (HSE_REQ_END_RESERVED+10)
#define HSE_REQ_IS_KEEP_CONN (HSE_REQ_END_RESERVED+8)

#define HSE_URL_FLAGS_READ				0x00000001
#define HSE_URL_FLAGS_WRITE				0x00000002
#define HSE_URL_FLAGS_EXECUTE			0x00000004
#define HSE_URL_FLAGS_SSL				0x00000008
#define HSE_URL_FLAGS_DONT_CACHE		0x00000010
#define HSE_URL_FLAGS_NEGO_CERT			0x00000020
#define HSE_URL_FLAGS_REQUIRE_CERT		0x00000040
#define HSE_URL_FLAGS_MAP_CERT			0x00000080
#define HSE_URL_FLAGS_SSL128			0x00000100
#define HSE_URL_FLAGS_SCRIPT			0x00000200



typedef struct _HSE_VERSION_INFO 
{
  DWORD dwExtensionVersion;
  CHAR lpszExtensionDesc[HSE_MAX_EXT_DLL_NAME_LEN];
} HSE_VERSION_INFO, *LPHSE_VERSION_INFO;

typedef struct _HSE_URL_MAPEX_INFO  
{
	CHAR   lpszPath[MAX_PATH]; 
	DWORD  dwFlags;
	DWORD  cchMatchingPath; 
	DWORD  cchMatchingURL;  
	DWORD  dwReserved1;
	DWORD  dwReserved2;
} HSE_URL_MAPEX_INFO, * LPHSE_URL_MAPEX_INFO;

typedef struct _EXTENSION_CONTROL_BLOCK 
{
  DWORD cbSize;
  DWORD dwVersion;
  HCONN ConnID;
  DWORD dwHttpStatusCode;
  CHAR lpszLogData[HSE_LOG_BUFFER_LEN];
  LPSTR lpszMethod;
  LPSTR lpszQueryString;
  LPSTR lpszPathInfo;
  LPSTR lpszPathTranslated;
  DWORD cbTotalBytes;
  DWORD cbAvailable;
  LPBYTE lpbData;
  LPSTR lpszContentType;
  BOOL (WINAPI * GetServerVariable)(HCONN hConn, LPSTR lpszVariableName, 						                      							  					
				                            LPVOID lpvBuffer, LPDWORD lpdwSize);
  BOOL (WINAPI * WriteClient)(HCONN ConnID, LPVOID Buffer, LPDWORD lpdwBytes, DWORD dwReserved);
  BOOL (WINAPI * ReadClient)(HCONN ConnID, LPVOID lpvBuffer, LPDWORD lpdwSize);
  BOOL (WINAPI * ServerSupportFunction)(HCONN hConn, DWORD dwHSERRequest, LPVOID lpvBuffer,
                                        LPDWORD lpdwSize, LPDWORD lpdwDataType);
} EXTENSION_CONTROL_BLOCK, *LPEXTENSION_CONTROL_BLOCK;

struct ConnTableRecord
{
  BOOL Allocated;
  httpThreadContext *td;
  char* envString;
  LPCONNECTION connection;
  HANDLE ISAPIDoneEvent;
};
void initISAPI();
void cleanupISAPI();
int ISAPIRedirect(httpThreadContext* td,LPCONNECTION a,char *URL);
int ISAPISendURI(httpThreadContext* td,LPCONNECTION a,char *URL);
int ISAPISendHeader(httpThreadContext* td,LPCONNECTION a,char *URL);
ConnTableRecord *HConnRecord(HCONN hConn);

typedef BOOL (WINAPI * PFN_GETEXTENSIONVERSION)(HSE_VERSION_INFO *pVer);
typedef DWORD (WINAPI * PFN_HTTPEXTENSIONPROC)(EXTENSION_CONTROL_BLOCK *pECB);
/*
*Use this to execute an ISAPI file on the server.
*/
int sendISAPI(httpThreadContext* td,LPCONNECTION connection,char* scriptpath,char* /*ext*/,char *cgipath);

BOOL WINAPI ServerSupportFunctionExport(HCONN hConn, DWORD dwHSERRequest,LPVOID lpvBuffer, LPDWORD lpdwSize, LPDWORD lpdwDataType);
BOOL WINAPI ReadClientExport(HCONN hConn, LPVOID lpvBuffer, LPDWORD lpdwSize ) ;
BOOL WINAPI WriteClientExport(HCONN hConn, LPVOID Buffer, LPDWORD lpdwBytes, DWORD dwReserved);
BOOL WINAPI GetServerVariableExport(HCONN, LPSTR, LPVOID, LPDWORD);
BOOL buildAllHttpHeaders(httpThreadContext* td,LPCONNECTION a,LPVOID output,LPDWORD maxLen);
BOOL buildAllRawHeaders(httpThreadContext* td,LPCONNECTION a,LPVOID output,LPDWORD maxLen);
#endif
#endif
