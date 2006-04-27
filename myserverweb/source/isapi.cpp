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

 
#include "../include/isapi.h"
#include "../include/http.h"
#include "../include/server.h"
#include "../include/file.h"
#include "../include/http_constants.h"
#include "../include/cgi.h"
#include "../include/dynamiclib.h"

#include <string>
#include <sstream>
using namespace std;

/*!
 *Initialize the timeout value to 15 seconds.
 */
u_long Isapi::timeout=MYSERVER_SEC(15);


#ifdef WIN32

u_long Isapi::maxConnections=0;
static CRITICAL_SECTION GetTableEntryCritSec;
int Isapi::initialized=0;
Mutex *Isapi::isapi_mutex=0;
ConnTableRecord *Isapi::connTable=0;


BOOL WINAPI ISAPI_ServerSupportFunctionExport(HCONN hConn, DWORD dwHSERRequest,
                                              LPVOID lpvBuffer, LPDWORD lpdwSize, 
                                              LPDWORD lpdwDataType) 
{
  string tmp;
	ConnTableRecord *ConnInfo;
  int ret;
	char *buffer=0;	
	char uri[MAX_PATH];/*! Under windows use MAX_PATH. */	
  Isapi::isapi_mutex->lock();
	ConnInfo = Isapi::HConnRecord(hConn);
	Isapi::isapi_mutex->unlock();
	if (ConnInfo == NULL) 
	{
    Server::getInstance()->logLockAccess();
		Server::getInstance()->logPreparePrintError();
		Server::getInstance()->logWriteln("isapi::ServerSupportFunctionExport: invalid hConn");
		Server::getInstance()->logEndPrintError();
    Server::getInstance()->logUnlockAccess();
		return 0;
	}
	HttpRequestHeader::Entry *connection = ConnInfo->td->request.other.get("Connection");

 	switch (dwHSERRequest) 
	{
		case HSE_REQ_MAP_URL_TO_PATH_EX:
			HSE_URL_MAPEX_INFO  *mapInfo;
			mapInfo=(HSE_URL_MAPEX_INFO*)lpdwDataType;
      mapInfo->lpszPath = 0;
      tmp.assign(mapInfo->lpszPath);
			ret=ConnInfo->td->http->getPath(tmp,(char*)lpvBuffer,0);
      if(ret!=e_200)
        return 1;
			
      mapInfo->cchMatchingURL=(DWORD)strlen((char*)lpvBuffer);
			mapInfo->cchMatchingPath=(DWORD)strlen(mapInfo->lpszPath);
      delete [] mapInfo->lpszPath;
      mapInfo->lpszPath = new char[tmp.length()+1];
      if(mapInfo->lpszPath == 0)
        if(buffer==0)
        {
          SetLastError(ERROR_INSUFFICIENT_BUFFER);
          return 0;            
        }
      strcpy(mapInfo->lpszPath, tmp.c_str());
			mapInfo->dwFlags = HSE_URL_FLAGS_WRITE|HSE_URL_FLAGS_SCRIPT 
                                            | HSE_URL_FLAGS_EXECUTE;
			break;
		case HSE_REQ_MAP_URL_TO_PATH:
			if(((char*)lpvBuffer)[0])
				strcpy(uri,(char*)lpvBuffer);
			else
        lstrcpyn(uri,ConnInfo->td->request.uri.c_str(),
								 (int)ConnInfo->td->request.uri.length()-ConnInfo->td->pathInfo.length() +1);
			ret = ConnInfo->td->http->getPath(tmp ,uri,0);
      if(ret!=e_200)
      {
        if(buffer)
          delete [] buffer;
        return 1;
      }
      
      if(tmp.length() >= *lpdwSize)
      {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return 0;
      }    
      strcpy((char*)lpvBuffer, tmp.c_str());
      if(File::completePath((char**)&lpvBuffer,(int*)lpdwSize,  1))
      {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return 0;
      }
      *lpdwSize=(DWORD)strlen((char*)lpvBuffer);
      break;
  case HSE_REQ_SEND_URL_REDIRECT_RESP:
    return ((Isapi*)ConnInfo->lisapi)->Redirect(ConnInfo->td,
                              ConnInfo->connection,(char *)lpvBuffer);
    break;
  case HSE_REQ_SEND_URL:
    return ((Isapi*)ConnInfo->lisapi)->Senduri(ConnInfo->td,
                                           ConnInfo->connection,(char *)lpvBuffer);
    break;
  case HSE_REQ_SEND_RESPONSE_HEADER:
    return ((Isapi*)ConnInfo->lisapi)->SendHeader(ConnInfo->td,
                                           ConnInfo->connection,(char *)lpvBuffer);
    break;
  case HSE_REQ_DONE_WITH_SESSION:
    ConnInfo->td->response.httpStatus=*(DWORD*)lpvBuffer;
    SetEvent(ConnInfo->ISAPIDoneEvent);
    break;
  case HSE_REQ_IS_KEEP_CONN:
    if(connection && !stringcmpi(connection->value->c_str(), "keep-alive"))
      *((BOOL*)lpvBuffer)=1;
    else
      *((BOOL*)lpvBuffer)=0;
    break;
  default:
    return 0;
	}
	return 1;
}

/*!
 *Add a connection to the table.
 */
ConnTableRecord *Isapi::HConnRecord(HCONN hConn) 
{
	u_long connIndex;

	connIndex =((u_long) hConn) - 1;
	ConnTableRecord *ConnInfo;
	if ((connIndex < 0) || (connIndex >= maxConnections)) 
	{
		return NULL;
	}
	ConnInfo = &(connTable[connIndex]);
	if (ConnInfo->Allocated == 0) 
	{
		return NULL;
	}
	return ConnInfo;
}

/*!
 *Send an HTTP redirect.
 */
int Isapi::Redirect(HttpThreadContext* td, ConnectionPtr a, char *URL) 
{
	return td->http->sendHTTPRedirect(URL);
}

/*!
 *Send an HTTP URI.
 */
int Isapi::Senduri(HttpThreadContext* td, ConnectionPtr a, char *URL)
{
  string tmp;
  tmp.assign(URL);
	return td->http->sendHTTPResource(tmp, 0, 0);
}

/*!
 *Send the ISAPI header.
 */
int Isapi::SendHeader(HttpThreadContext* td,ConnectionPtr a,char *data)
{
  HttpHeaders::buildHTTPResponseHeaderStruct(&(td->response), td, data);
  return 1;
}

/*!
 *Write directly to the output.
 */
BOOL WINAPI ISAPI_WriteClientExport(HCONN hConn, LPVOID Buffer, LPDWORD lpdwBytes, 
                                    DWORD /*!dwReserved*/)
{
	int keepalive;
	char* buffer;
	ConnTableRecord *ConnInfo;
	char chunk_size[15];
	u_long nbw=0;

	if(*lpdwBytes==0)
		return 1;
	
  Isapi::isapi_mutex->lock();
	ConnInfo = Isapi::HConnRecord(hConn);
	Isapi::isapi_mutex->unlock();
	
  if(ConnInfo == NULL)
    return 1;
	buffer=(char*)ConnInfo->td->buffer->getBuffer();
	HttpRequestHeader::Entry *connection = ConnInfo->td->request.other.get("Connection");

	if (ConnInfo == NULL) 
	{
		((Vhost*)(ConnInfo->td->connection->host))->warningslogRequestAccess(
                                                                ConnInfo->td->id);
		((Vhost*)(ConnInfo->td->connection->host))->warningsLogWrite(
                                        "ISAPI: WriteClientExport: invalid hConn");
		((Vhost*)(ConnInfo->td->connection->host))->warningslogTerminateAccess(
                                                                ConnInfo->td->id);
		return 0;
	}
	keepalive =connection && (!stringcmpi(connection->value->c_str(), "keep-alive")) ;

  /*If the HTTP header was sent do not send it again. */
	if(!ConnInfo->headerSent)
	{
	  int headerSize=0;
    u_long size = (u_long)strlen(buffer);
		strncat(buffer,(char*)Buffer,*lpdwBytes);
		ConnInfo->headerSize+=*lpdwBytes;
	  if(buffer[0]=='\r')
	  {
		  if(buffer[1]=='\n')
        headerSize = 2;
    }
    else
    {
		  for(u_long i=0;i<size;i++)
		  {
		   	if(buffer[i]=='\r')
			  	if(buffer[i+1]=='\n')
			  		if(buffer[i+2]=='\r')
			  			if(buffer[i+3]=='\n')
			  			{
			  				headerSize=i+4;
			  				buffer[i+2]='\0';
			  				break;
			  			}
  			if(buffer[i]=='\n')
  			{
  				if(buffer[i+1]=='\n')
  				{
  					headerSize=i+2;
  					buffer[i+1]='\0';
  					break;
  				}
  			}
  		}
    } 
    /*!
     *Handle the HTTP header if exists.
     */
		if(headerSize)
		{
			int len = ConnInfo->headerSize-headerSize;
      HttpHeaders::buildHTTPResponseHeaderStruct(&ConnInfo->td->response,
                                  ConnInfo->td,(char*)ConnInfo->td->buffer->getBuffer());
			if(!ConnInfo->td->appendOutputs)
			{
				if(keepalive)
				{
					HttpResponseHeader::Entry *e;
					e = ConnInfo->td->response.other.get("Transfer-Encoding");
					if(e)
						e->value->assign("chunked");
					else
					{
						e = new HttpResponseHeader::Entry();
						e->name->assign("Transfer-Encoding");
						e->value->assign("chunked");
						ConnInfo->td->response.other.put(*(e->name), e);
					}
				}
				else
					ConnInfo->td->response.connection.assign("Close");
				
				HttpHeaders::buildHTTPResponseHeader(
                 (char*)ConnInfo->td->buffer2->getBuffer(),&(ConnInfo->td->response));
	
				if(ConnInfo->connection->socket.send(
                     (char*)ConnInfo->td->buffer2->getBuffer(),
                     (int)strlen((char*)ConnInfo->td->buffer2->getBuffer()), 0)==-1)
					return 0;
			}
      /*! Save the headerSent status. */
			ConnInfo->headerSent=1;
      
      /*! If only the header was requested return. */
      if(ConnInfo->headerSent && ConnInfo->onlyHeader)
        return 0;

			/*!Send the first chunk. */
			if(len)
			{
        /*! With keep-alive connections use chunks.*/
				if(keepalive && (!ConnInfo->td->appendOutputs))
				{
					sprintf(chunk_size,"%x\r\n",len);
				  if(ConnInfo->chain.write(chunk_size, (int)strlen(chunk_size), &nbw))			
						return 0;
				}
				
				if(ConnInfo->td->appendOutputs)
				{
					if(ConnInfo->td->outputData.writeToFile((char*)(buffer+headerSize),len,
                                                   &nbw))
						return 0;
					ConnInfo->dataSent += nbw;
				}
				else
				{
          if(ConnInfo->chain.write((char*)(buffer+headerSize), 
                                   len, &nbw))
						return 0;
          ConnInfo->dataSent += nbw;
				}

        /*! Send the chunk tailer.*/
				if(keepalive && (!ConnInfo->td->appendOutputs))
				{
          if(ConnInfo->chain.write("\r\n", 2, &nbw))					
						return 0;
				}
			}
		}
		else
    {
			nbw=*lpdwBytes;
    }
	}
	else/*!Continue to send data chunks*/
	{
		if(keepalive  && (!ConnInfo->td->appendOutputs))
		{
			sprintf(chunk_size,"%x\r\n",*lpdwBytes);
			nbw = ConnInfo->connection->socket.send(chunk_size,(int)strlen(chunk_size), 0);
			if((nbw == (u_long)-1) || (!nbw))
				return 0;
		}
		
  	if(ConnInfo->td->appendOutputs)
		{
			if(ConnInfo->td->outputData.writeToFile((char*)Buffer,*lpdwBytes, &nbw))
				return 0;
			ConnInfo->dataSent += nbw;
		}
		else
		{
      if(ConnInfo->chain.write((char*)Buffer,*lpdwBytes, &nbw))
				return 0;
      ConnInfo->dataSent += nbw;
		}

		if(keepalive  && (!ConnInfo->td->appendOutputs))
    {
      nbw = ConnInfo->connection->socket.send("\r\n",2, 0);
			if((nbw == (u_long)-1) || (!nbw))
				return 0;
		}
	}

	*lpdwBytes = nbw;

	ConnInfo->td->sentData += ConnInfo->dataSent;

	return (nbw != -1) ? 1 : 0;
}

/*!
 *Read directly from the client.
 */
BOOL WINAPI ISAPI_ReadClientExport(HCONN hConn, LPVOID lpvBuffer, 
																	 LPDWORD lpdwSize ) 
{
	ConnTableRecord *ConnInfo;
	u_long NumRead;
	
  Isapi::isapi_mutex->lock();
	ConnInfo = Isapi::HConnRecord(hConn);
  Isapi::isapi_mutex->unlock();
	if (ConnInfo == NULL) 
	{
		((Vhost*)(ConnInfo->td->connection->host))->warningslogRequestAccess(ConnInfo->td->id);
		((Vhost*)(ConnInfo->td->connection->host))->warningsLogWrite("ISAPI: ReadClientExport: invalid hConn");
		((Vhost*)(ConnInfo->td->connection->host))->warningslogTerminateAccess(ConnInfo->td->id);
		return 0;
	}
	ConnInfo->td->inputData.readFromFile((char*)lpvBuffer, *lpdwSize, &NumRead);
	if (NumRead == -1) 
	{
		*lpdwSize = 0;
		return 0;
	}
	else 
	{
		*lpdwSize = (DWORD)NumRead;
		return 1;
	}
}

/*!
 *Get server environment variable.
 */
BOOL WINAPI ISAPI_GetServerVariableExport(HCONN hConn, 
																					LPSTR lpszVariableName, 
                                          LPVOID lpvBuffer, 
																					LPDWORD lpdwSize) 
{
	ConnTableRecord *ConnInfo;
	BOOL ret =1;
	Isapi::isapi_mutex->lock();
	ConnInfo = Isapi::HConnRecord(hConn);
	Isapi::isapi_mutex->unlock();
	if (ConnInfo == NULL) 
	{
    Server::getInstance()->logLockAccess();
    Server::getInstance()->logPreparePrintError();
		Server::getInstance()->logWriteln("isapi::GetServerVariableExport: invalid hConn");
		Server::getInstance()->logEndPrintError();
    Server::getInstance()->logUnlockAccess();
		return 0;
	}

	if (!strcmp(lpszVariableName, "ALL_HTTP")) 
	{

        if(Isapi::buildAllHttpHeaders(ConnInfo->td,ConnInfo->connection, lpvBuffer, lpdwSize))
			ret=1;
		
        else
		{
      SetLastError(ERROR_INSUFFICIENT_BUFFER);
			ret=0;
		}
			
	}else if(!strcmp(lpszVariableName, "ALL_RAW")) 
	{
		if(Isapi::buildAllRawHeaders(ConnInfo->td,ConnInfo->connection,lpvBuffer,lpdwSize))
			ret=1;
		else
		{
      SetLastError(ERROR_INSUFFICIENT_BUFFER);
			ret=0;
		}
			
	}
	else
	{
		/*!
     *find in ConnInfo->envString the value lpszVariableName 
     *and copy next string in lpvBuffer.
     */
		char *localEnv;
		int variableNameLen;
		((char*)lpvBuffer)[0]='\0';
	  localEnv = ConnInfo->envString;
		variableNameLen = (int)strlen(lpszVariableName);
		for(u_long i = 0;;i += (u_long)strlen(&localEnv[i]) + 1)
		{
			if(((localEnv[i + variableNameLen]) == '=') &&
         (!strncmp(&localEnv[i], lpszVariableName, variableNameLen)))
			{
				strncpy((char*)lpvBuffer, &localEnv[i + variableNameLen + 1], 
								*lpdwSize);
				break;
			}
			else if((localEnv[i] == '\0') && (localEnv[i + 1] == '\0'))
			{
				break;
			}
		}
	}
	*lpdwSize =(DWORD)strlen((char*)lpvBuffer);
	return ret;
}

/*!
 *Build the string that contains all the HTTP headers.
 */
BOOL Isapi::buildAllHttpHeaders(HttpThreadContext* td, ConnectionPtr /*!a*/,
                                LPVOID output, LPDWORD dwMaxLen)
{
	DWORD valLen = 0;
	DWORD maxLen = *dwMaxLen;
	char *ValStr=(char*)output;
	HttpRequestHeader::Entry *accept = td->request.other.get("Accept");
	HttpRequestHeader::Entry *cache = td->request.other.get("Cache-Control");

	if(accept && accept->value->length() && (valLen+30<maxLen))
		valLen += sprintf(&ValStr[valLen],"HTTP_ACCEPT:%s\n",
										accept->value->c_str());
	else if(valLen + 30 < maxLen) 
		return 0;

	if(cache && cache->value->length() && (valLen+30<maxLen))
		valLen += sprintf(&ValStr[valLen], "HTTP_CACHE_CONTROL:%s\n", 
										cache->value->c_str());
	else if(valLen + 30 < maxLen) 
		return 0;

	if((td->request.rangeByteBegin || td->request.rangeByteEnd) && (valLen+30<maxLen))
	{
    ostringstream rangeBuffer;
		rangeBuffer << "HTTP_RANGE:" << td->request.rangeType << "=" ;
    if(td->request.rangeByteBegin)
    {
      rangeBuffer << (int)td->request.rangeByteBegin;
    }
    rangeBuffer << "-";
    if(td->request.rangeByteEnd)
    {
      rangeBuffer << td->request.rangeByteEnd;
     }   
		valLen += sprintf(&ValStr[valLen], "%s\n", rangeBuffer.str().c_str());
	}

	{
		HttpRequestHeader::Entry* e = td->request.other.get("Accept-Encoding");
		if(e && (valLen + 30 < maxLen))
			valLen += sprintf(&ValStr[valLen], "HTTP_ACCEPT_ENCODING:%s\n",
												e->value->c_str());
		else if(valLen + 30 < maxLen) 
			return 0;
	}


	{
		HttpRequestHeader::Entry* e = td->request.other.get("Accept-Language");
		if(e && (valLen + 30 < maxLen))
			valLen += sprintf(&ValStr[valLen],"HTTP_ACCEPT_LANGUAGE:%s\n",
											e->value->c_str());
		else if(valLen + 30 < maxLen) 
			return 0;
	}


	{
		HttpRequestHeader::Entry* e = td->request.other.get("Accept-Charset");
		if(e && (valLen + 30 < maxLen))
			valLen += sprintf(&ValStr[valLen],"HTTP_ACCEPT_CHARSET:%s\n",
											e->value->c_str());
		else if(valLen + 30 < maxLen) 
			return 0;
	}

	{
		HttpRequestHeader::Entry* e = td->request.other.get("Pragma");
		if(e && (valLen + 30 < maxLen))
			valLen += sprintf(&ValStr[valLen],"HTTP_PRAGMA:%s\n",
											e->value->c_str());
		else if(valLen + 30 < maxLen) 
			return 0;
	}

	{
		HttpRequestHeader::Entry* e = td->request.other.get("Connection");
		if(e && (valLen + 30< maxLen))
			valLen += sprintf(&ValStr[valLen],"HTTP_CONNECTION:%s\n",
											e->value->c_str());
		else if(valLen + 30 < maxLen) 
			return 0;
	}

	{
		HttpRequestHeader::Entry* e = td->request.other.get("Cookie");
		if(e && (valLen + 30 < maxLen))
			valLen += sprintf(&ValStr[valLen],"HTTP_COOKIE:%s\n",
											e->value->c_str());
		else if(valLen + 30 < maxLen) 
			return 0;
	}

	{
		HttpRequestHeader::Entry* e = td->request.other.get("Host");
		if(e && (valLen + 30 < maxLen))
			valLen += sprintf(&ValStr[valLen],"HTTP_HOST:%s\n",
											e->value->c_str());
		else if(valLen + 30 < maxLen) 
			return 0;
	}

	{
		HttpRequestHeader::Entry* e = td->request.other.get("Date");
		if(e && (valLen + 30 < maxLen))
			valLen += sprintf(&ValStr[valLen],"HTTP_DATE:%s\n",
											e->value->c_str());
		else if(valLen + 30 < maxLen) 
			return 0;
	}

	{
		HttpRequestHeader::Entry* e = td->request.other.get("If-Modified-Since");
		if(e && (valLen + 30 < maxLen))
			valLen += sprintf(&ValStr[valLen],"HTTP_IF_MODIFIED_SINCE:%s\n",
											e->value->c_str());
		else if(valLen + 30 < maxLen) 
			return 0;
	}

	{
		HttpRequestHeader::Entry* e = td->request.other.get("Referer");
		if(e && (valLen + 30 < maxLen))
			valLen += sprintf(&ValStr[valLen],"HTTP_REFERER:%s\n",
											e->value->c_str());
		else if(valLen + 30 < maxLen) 
			return 0;
	}

	{
		HttpRequestHeader::Entry* e = td->request.other.get("User-Agent");
		if(e && (valLen + 30 < maxLen))
			valLen += sprintf(&ValStr[valLen],"HTTP_USER_AGENT:%s\n",
											e->value->c_str());
		else if(valLen + 30 < maxLen) 
			return 0;
	}

	{
		HttpRequestHeader::Entry* e = td->request.other.get("From");
		if(e && (valLen + 30 < maxLen))
			valLen += sprintf(&ValStr[valLen],"HTTP_FROM:%s\n",
											e->value->c_str());
		else if(valLen + 30 < maxLen) 
			return 0;
	}

	return 1;
}

/*!
 *Build the string that contains all the headers.
 */
BOOL Isapi::buildAllRawHeaders(HttpThreadContext* td,ConnectionPtr a,
                               LPVOID output,LPDWORD dwMaxLen)
{
	DWORD valLen = 0;
	DWORD maxLen = *dwMaxLen;
	char *ValStr = (char*)output;
	if(buildAllHttpHeaders(td, a, output, dwMaxLen) == 0)
		return 0;
	valLen = (DWORD)strlen(ValStr);

	if(td->pathInfo.length() && (valLen + 30 < maxLen))
		valLen += sprintf(&ValStr[valLen], "PATH_INFO:%s\n",
											td->pathInfo.c_str());
	else if(valLen + 30 < maxLen) 
		return 0;
	
	if(td->pathTranslated.length() && (valLen + 30 < maxLen))
		valLen += sprintf(&ValStr[valLen], "PATH_INFO:%s\n",
											td->pathTranslated.c_str());
	else if(valLen + 30 < maxLen) 
		return 0;

	if(td->request.uriOpts[0] && (valLen + 30 < maxLen))
		valLen += sprintf(&ValStr[valLen], "QUERY_STRING:%s\n",
											td->request.uriOpts[0]);
	else if(valLen + 30 < maxLen) 
		return 0;

	if(td->request.cmd[0] && (valLen + 30 < maxLen))
		valLen += sprintf(&ValStr[valLen], "REQUEST_METHOD:%s\n",
											td->request.cmd[0]);
	else if(valLen + 30 < maxLen) 
		return 0;

	if(td->filenamePath.length() && (valLen + 30 < maxLen))
		valLen += sprintf(&ValStr[valLen], "SCRIPT_FILENAME:%s\n", 
											td->filenamePath[0]);
	else if(valLen + 30 < maxLen) 
		return 0;

	if(valLen + 30 < maxLen)
		valLen += sprintf(&ValStr[valLen], "SERVER_PORT:%u\n", 
											td->connection->getLocalPort());
	else if(valLen + 30 < maxLen) 
		return 0;

	if(valLen + 30 < maxLen)
		valLen += sprintf(&ValStr[valLen], 
											"SERVER_SIGNATURE:<address>%s</address>\n",
											versionOfSoftware);
	else if(valLen + 30 < maxLen) 
		return 0;

	if(td->connection->getIpAddr()[0] && valLen + 30 < maxLen)
		valLen += sprintf(&ValStr[valLen], "REMOTE_ADDR:\n",
											td->connection->getIpAddr());
	else if(valLen + 30 < maxLen) 
		return 0;

	if(td->connection->getPort() && valLen + 30 < maxLen)
		valLen += sprintf(&ValStr[valLen], "REMOTE_PORT:%u\n",
											td->connection->getPort());
	else if(valLen + 30 < maxLen) 
		return 0;

	if(valLen + 30 < maxLen)
		valLen += sprintf(&ValStr[valLen], "SERVER_ADMIN:%s\n", 
											Server::getInstance()->getServerAdmin());
	else if(valLen + 30 < maxLen) 
		return 0;

	if(valLen + MAX_PATH < maxLen)
	{
		valLen += sprintf(&ValStr[valLen],"SCRIPT_NAME:");
		lstrcpyn(&ValStr[valLen], td->request.uri.c_str(),
             td->request.uri.length()- td->pathInfo.length() + 1);
		valLen += (DWORD)td->request.uri.length()-td->pathInfo.length() + 1;
		valLen += (DWORD)sprintf(&ValStr[valLen],"\n");
	}
	else if(valLen + 30 < maxLen) 
		return 0;
	return 1;
}

#endif

/*!
 *Main procedure to call an ISAPI module.
 */
int Isapi::send(HttpThreadContext* td,ConnectionPtr connection, 
                const char* scriptpath, const char *cgipath, 
                int execute, int onlyHeader)
{
/*!
 *ISAPI works only on the windows architecture.
 */
#ifdef NOT_WIN
  return td->http->raiseHTTPError(e_501);
#endif

#ifdef WIN32
	DWORD Ret;
	EXTENSION_CONTROL_BLOCK ExtCtrlBlk;
	DynamicLibrary appHnd;
	HSE_VERSION_INFO Ver;
	u_long connIndex;
	PFN_GETEXTENSIONVERSION GetExtensionVersion;
	PFN_HTTPEXTENSIONPROC HttpExtensionProc;
  const char *loadLib;
	int retvalue = 0;
	/*! Under windows there is MAX_PATH then we can use it. */
	char fullpath[MAX_PATH * 2];
	if(!execute)
	{
    if(cgipath && strlen(cgipath))
			sprintf(fullpath, "%s \"%s\"", cgipath, td->filenamePath.c_str());
		else
			sprintf(fullpath, "%s", td->filenamePath.c_str());
	}
	else
	{
		sprintf(fullpath, "%s", cgipath);
	}

    td->inputData.setFilePointer(0);

	EnterCriticalSection(&GetTableEntryCritSec);
	connIndex = 0;
	Isapi::isapi_mutex->lock();
	while ((connTable[connIndex].Allocated != 0) && 
				 (connIndex < maxConnections)) 
	{
		connIndex++;
	}
	Isapi::isapi_mutex->unlock();
	LeaveCriticalSection(&GetTableEntryCritSec);

	if (connIndex == maxConnections) 
	{
		td->connection->host->warningslogRequestAccess(td->id);
		td->connection->host->warningsLogWrite(
                                            "ISAPI: Error max connections");
		td->connection->host->warningslogTerminateAccess(td->id);
		return td->http->raiseHTTPError(e_503);
	}
	if(execute)
  	loadLib = scriptpath;
	else
	  loadLib = cgipath;

  Ret = appHnd.loadLibrary(loadLib);

  if(Ret)
	{
    string msg;
    msg.assign("ISAPI: Cannot load library ");
    msg.append(loadLib);
		td->connection->host->warningslogRequestAccess(td->id);
		td->connection->host->warningsLogWrite(msg.c_str());
		td->connection->host->warningslogTerminateAccess(td->id);
		return td->http->raiseHTTPError(e_500);
	}

  connTable[connIndex].chain.setProtocol(td->http);
  connTable[connIndex].chain.setProtocolData(td);
  connTable[connIndex].chain.setStream(&(td->connection->socket));
  if(td->mime)
  {
    u_long nbw;
    if(td->mime && Server::getInstance()->getFiltersFactory()->chain(
																					&(connTable[connIndex].chain),
																					td->mime->filters, 
																					&(td->connection->socket), 
																					&nbw, 1))
      {
        td->connection->host->warningslogRequestAccess(td->id);
        td->connection->host->warningsLogWrite("Error loading filters");
        td->connection->host->warningslogTerminateAccess(td->id);
        connTable[connIndex].chain.clearAllFilters(); 
        return td->http->raiseHTTPError(e_500);
      }
  }

	connTable[connIndex].connection = connection;
	connTable[connIndex].td = td;
  connTable[connIndex].onlyHeader = onlyHeader;
	connTable[connIndex].headerSent = 0;
	connTable[connIndex].headerSize = 0;
	connTable[connIndex].dataSent = 0;
	connTable[connIndex].Allocated = 1;
	connTable[connIndex].lisapi = this;
	connTable[connIndex].ISAPIDoneEvent = CreateEvent(NULL, 1, 0, NULL);
  
	GetExtensionVersion = (PFN_GETEXTENSIONVERSION)appHnd.getProc("GetExtensionVersion");

	if (GetExtensionVersion == NULL) 
	{
		td->connection->host->warningslogRequestAccess(td->id);
		td->connection->host->warningsLogWrite(
           "ISAPI: Failed to get pointer to GetExtensionVersion() in ISAPI application");
		td->connection->host->warningslogTerminateAccess(td->id);
		if(!appHnd.close())
		{
      string msg;
      msg.assign("ISAPI: Failed to freeLibrary in ISAPI module: ");
      msg.append(cgipath);
      msg.append("\r\n");
			td->connection->host->warningslogRequestAccess(td->id);
			td->connection->host->warningsLogWrite(msg.c_str());
			td->connection->host->warningslogTerminateAccess(td->id);
		}
    connTable[connIndex].chain.clearAllFilters(); 
		return td->http->raiseHTTPError(e_500);
	}
	if(!GetExtensionVersion(&Ver)) 
	{
    td->connection->host->warningslogRequestAccess(td->id);
		td->connection->host->warningsLogWrite(
                             "ISAPI: GetExtensionVersion() returned FALSE");
		td->connection->host->warningslogTerminateAccess(td->id);
		if(!appHnd.close())
		{
      string msg;
      msg.assign("ISAPI: Failed to freeLibrary in ISAPI module: ");
      msg.append(cgipath);
			td->connection->host->warningslogRequestAccess(td->id);
			td->connection->host->warningsLogWrite(msg.c_str());
			td->connection->host->warningslogTerminateAccess(td->id);
		}
    connTable[connIndex].chain.clearAllFilters(); 
		return td->http->raiseHTTPError(e_500);
	}
	if (Ver.dwExtensionVersion > MAKELONG(HSE_VERSION_MINOR, 
																				HSE_VERSION_MAJOR)) 
	{
		td->connection->host->warningslogRequestAccess(td->id);
		td->connection->host->warningsLogWrite(
                                       "ISAPI: version not supported");
		td->connection->host->warningslogRequestAccess(td->id);
		if(!appHnd.close())
		{
      string msg;
      msg.assign("ISAPI: Failed to freeLibrary in ISAPI module: ");
      msg.append(cgipath);
			td->connection->host->warningslogRequestAccess(td->id);
			td->connection->host->warningsLogWrite(msg.c_str());
			td->connection->host->warningslogTerminateAccess(td->id);
		}
    connTable[connIndex].chain.clearAllFilters(); 
		return td->http->raiseHTTPError(e_500);
	}
	/*!
   *Store the environment string in the buffer2.
	 */
	connTable[connIndex].envString=td->buffer2->getBuffer();
	
	/*!
   *Build the environment string.
   */
	td->scriptPath.assign(scriptpath);

  {
    string tmp;
    tmp.assign(cgipath);
    File::splitPath(tmp, td->cgiRoot, td->cgiFile);
    tmp.assign(scriptpath);
    File::splitPath(tmp, td->scriptDir, td->scriptFile);
  }
	connTable[connIndex].envString[0]='\0';
	Cgi::buildCGIEnvironmentString(td,connTable[connIndex].envString);
  
	ZeroMemory(&ExtCtrlBlk, sizeof(ExtCtrlBlk));
	ExtCtrlBlk.cbSize = sizeof(ExtCtrlBlk);
	ExtCtrlBlk.dwVersion = MAKELONG(HSE_VERSION_MINOR, HSE_VERSION_MAJOR);
	ExtCtrlBlk.GetServerVariable = ISAPI_GetServerVariableExport;
	ExtCtrlBlk.ReadClient  = ISAPI_ReadClientExport;
	ExtCtrlBlk.WriteClient = ISAPI_WriteClientExport;
	ExtCtrlBlk.ServerSupportFunction = ISAPI_ServerSupportFunctionExport;
	ExtCtrlBlk.ConnID = (HCONN) (connIndex + 1);
	ExtCtrlBlk.dwHttpStatusCode = 200;
	ExtCtrlBlk.lpszLogData[0] = '0';
	ExtCtrlBlk.lpszMethod = (char*)td->request.cmd.c_str();
	ExtCtrlBlk.lpszQueryString =(char*) td->request.uriOpts.c_str();
	ExtCtrlBlk.lpszPathInfo = td->pathInfo.length() ? (char*)td->pathInfo.c_str() : (CHAR*)"" ;
	if(td->pathInfo.length())
		ExtCtrlBlk.lpszPathTranslated = (char*)td->pathTranslated.c_str();
	else
		ExtCtrlBlk.lpszPathTranslated = (char*)td->filenamePath.c_str();
	ExtCtrlBlk.cbTotalBytes = td->inputData.getFileSize();
	ExtCtrlBlk.cbAvailable = 0;
	ExtCtrlBlk.lpbData = 0;
	{
		HttpRequestHeader::Entry *content = 
			td->request.other.get("Content-Type");
		ExtCtrlBlk.lpszContentType = content ? (char*)content->value->c_str() 
			                                   : 0;
	}

	connTable[connIndex].td->buffer->setLength(0);
	connTable[connIndex].td->buffer->getAt(0)='\0';
	HttpExtensionProc = (PFN_HTTPEXTENSIONPROC)appHnd.getProc("HttpExtensionProc");
	if (HttpExtensionProc == NULL) 
	{
		td->connection->host->warningslogRequestAccess(td->id);
		td->connection->host->warningsLogWrite(
										 "ISAPI: Failed to get pointer to HttpExtensionProc() \
in ISAPI application module");

		td->connection->host->warningslogTerminateAccess(td->id);
    appHnd.close();
    connTable[connIndex].chain.clearAllFilters(); 
    return td->http->raiseHTTPError(e_500);
	}

	Ret = HttpExtensionProc(&ExtCtrlBlk);
	if (Ret == HSE_STATUS_PENDING) 
	{
		WaitForSingleObject(connTable[connIndex].ISAPIDoneEvent, timeout);
	}
	
	{
    u_long nbw = 0;
		HttpRequestHeader::Entry *connection = connTable[connIndex].td->request.other.get("Connection");

    if(connection && !stringcmpi(connection->value->c_str(), "keep-alive"))
	    Ret = connTable[connIndex].chain.write("0\r\n\r\n", 5, &nbw);
  }
  
	switch(Ret) 
	{
		case HSE_STATUS_SUCCESS_AND_KEEP_CONN:
			retvalue = 1;
			break;
		case 0:
		case HSE_STATUS_SUCCESS:
		case HSE_STATUS_ERROR:
		default:
			retvalue = 0;
			break;
	}

	if(!appHnd.close())
	{
    string msg;
    msg.assign("ISAPI: Failed to unload module "); 
    msg.append(cgipath);
		td->connection->host->warningslogRequestAccess(td->id);
		td->connection->host->warningsLogWrite(msg.c_str());
		td->connection->host->warningslogTerminateAccess(td->id);
	}

	{
    ostringstream tmp;
    tmp <<  connTable[connIndex].dataSent;
    connTable[connIndex].td->response.contentLength.assign(tmp.str()); 
  }                                      
  connTable[connIndex].chain.clearAllFilters(); 
	connTable[connIndex].Allocated = 0;
	return retvalue;
#else
  /*!
   *On other archs returns a non implemented error. 
   */
	td->connection->host->warningslogRequestAccess(td->id);
	td->connection->host->warningsLogWrite(
                                   "ISAPI: Not implemented");
	td->connection->host->warningslogTerminateAccess(td->id);	
	return td->http->raiseHTTPError(e_501);
#endif	
}

/*!
*Constructor for the ISAPI class.
*/
Isapi::Isapi()
{

}

/*!
*Initialize the ISAPI engine under WIN32.
*/
int Isapi::load(XmlParser*/* confFile*/)
{
#ifdef WIN32
  u_long nThreads = Server::getInstance()->getMaxThreads()/2;
	if(initialized)
		return 0;
	isapi_mutex = new Mutex;
	maxConnections = nThreads ? nThreads : 20 ;
	
	if(connTable)
		free(connTable);
		
	connTable = new ConnTableRecord[maxConnections];
  for(int i=0;i<maxConnections; i++)
  {
   	connTable[i].Allocated=0;
    connTable[i].onlyHeader=0;
	  connTable[i].headerSent=0;
	  connTable[i].headerSize=0;
	  connTable[i].td=0;
    connTable[i].dataSent=0;
  	connTable[i].envString=0;
  	connTable[i].connection=0;
  	connTable[i].ISAPIDoneEvent=0;
  	connTable[i].lisapi=0;         
  }
	InitializeCriticalSection(&GetTableEntryCritSec);	
	initialized=1;
#endif
  return 0;
}

/*!
 *Cleanup the memory used by ISAPI.
 */
int Isapi::unload()
{
#ifdef WIN32
	delete isapi_mutex;
	DeleteCriticalSection(&GetTableEntryCritSec);
	if(connTable)
		delete [] connTable;
	connTable=0;
	initialized=0;
#endif
  return 0;
}

/*!
 *Set a new timeout value used with the isapi modules.
 */
void Isapi::setTimeout(u_long ntimeout)
{
  timeout = ntimeout;
}

/*!
 *Return the timeout value used with the isapi modules.
 */
u_long Isapi::getTimeout()
{
  return timeout;
}
