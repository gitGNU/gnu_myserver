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

#ifdef WIN32
 
#include "../include/isapi.h"

static  u_long max_Connections;
static ConnTableRecord *connTable;
static CRITICAL_SECTION GetTableEntryCritSec;
#define ISAPI_TIMEOUT (10000)
/*
*Initialize the ISAPI engine under WIN32.
*/
void initISAPI()
{
	max_Connections=lserver->getNumThreads();
	connTable=(ConnTableRecord *)malloc(sizeof(ConnTableRecord)*max_Connections);
	ZeroMemory(connTable,sizeof(ConnTableRecord)*max_Connections);
	InitializeCriticalSection(&GetTableEntryCritSec);	
}
/*
*Cleanup the memory used by ISAPI
*/
void cleanupISAPI()
{
	DeleteCriticalSection(&GetTableEntryCritSec);
	if(connTable)
		free(connTable);
}
/*
*Main procedure to call an ISAPI module.
*/
int sendISAPI(httpThreadContext* td,LPCONNECTION connection,char* scriptpath,char* /*ext*/,char *cgipath,int execute)
{
	DWORD Ret;
	EXTENSION_CONTROL_BLOCK ExtCtrlBlk;
	HMODULE AppHnd;
	HSE_VERSION_INFO Ver;
	u_long connIndex;
	PFN_GETEXTENSIONVERSION GetExtensionVersion;
	PFN_HTTPEXTENSIONPROC HttpExtensionProc;

	char fullpath[MAX_PATH*2];
	if(execute)
	{
		if(cgipath[0])
			sprintf(fullpath,"%s \"%s\"",cgipath,td->filenamePath);
		else
			sprintf(fullpath,"%s",td->filenamePath);
	}
	else
	{
		sprintf(fullpath,"%s",cgipath);
	}
	EnterCriticalSection(&GetTableEntryCritSec);
	connIndex = 0;
	while ((connTable[connIndex].Allocated != FALSE) && (connIndex < max_Connections)) 
	{
		connIndex++;
	}
	LeaveCriticalSection(&GetTableEntryCritSec);

	if (connIndex == max_Connections) 
	{
		sprintf(td->buffer,"Error ISAPI max connections\r\n");
		((vhost*)td->connection->host)->warningsLogWrite(td->buffer);
		return raiseHTTPError(td,connection,e_500);
	}
	AppHnd = LoadLibrary(fullpath);

	connTable[connIndex].connection = connection;
	connTable[connIndex].td = td;
	connTable[connIndex].Allocated = TRUE;
	connTable[connIndex].ISAPIDoneEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (AppHnd == NULL) 
	{
		((vhost*)(td->connection->host))->warningsLogWrite("Failure to load ISAPI application module: ");
		((vhost*)(td->connection->host))->warningsLogWrite(cgipath);
		((vhost*)(td->connection->host))->warningsLogWrite("\r\n");
		if(!FreeLibrary(AppHnd))
		{
			((vhost*)(td->connection->host))->warningsLogWrite("Failure to FreeLibrary in ISAPI module");
			((vhost*)(td->connection->host))->warningsLogWrite(cgipath);
			((vhost*)(td->connection->host))->warningsLogWrite("\r\n");
		}
		return raiseHTTPError(td,connection,e_500);
	}

	GetExtensionVersion = (PFN_GETEXTENSIONVERSION) GetProcAddress(AppHnd, "GetExtensionVersion");
	if (GetExtensionVersion == NULL) 
	{
		((vhost*)(td->connection->host))->warningsLogWrite("Failure to get pointer to GetExtensionVersion() in ISAPI application\r\n");
		if(!FreeLibrary(AppHnd))
		{
			((vhost*)(td->connection->host))->warningsLogWrite("Failure to FreeLibrary in ISAPI module");
			((vhost*)(td->connection->host))->warningsLogWrite(cgipath);
			((vhost*)(td->connection->host))->warningsLogWrite("\r\n");
		}
		return raiseHTTPError(td,connection,e_500);
	}
	if(!GetExtensionVersion(&Ver)) 
	{
		((vhost*)(td->connection->host))->warningsLogWrite("ISAPI GetExtensionVersion() returned FALSE\r\n");
		if(!FreeLibrary(AppHnd))
		{
			((vhost*)(td->connection->host))->warningsLogWrite("Failure to FreeLibrary in ISAPI module");
			((vhost*)(td->connection->host))->warningsLogWrite(cgipath);
			((vhost*)(td->connection->host))->warningsLogWrite("\r\n");
		}
		return raiseHTTPError(td,connection,e_500);
	}
	if (Ver.dwExtensionVersion > MAKELONG(HSE_VERSION_MINOR, HSE_VERSION_MAJOR)) 
	{
		((vhost*)(td->connection->host))->warningsLogWrite("ISAPI version not supported\r\n");
		if(!FreeLibrary(AppHnd))
		{
			((vhost*)(td->connection->host))->warningsLogWrite("Failure to FreeLibrary in ISAPI module");
			((vhost*)(td->connection->host))->warningsLogWrite(cgipath);
			((vhost*)(td->connection->host))->warningsLogWrite("\r\n");
		}
		return raiseHTTPError(td,connection,e_500);
	}
	/*
	*Store the environment string in the buffer2.
	*/
	connTable[connIndex].envString=td->buffer2;
	/*
	*Build the environment string.
	*/
	lstrcpy(td->scriptPath,scriptpath);
	MYSERVER_FILE::splitPath(scriptpath,td->scriptDir,td->scriptFile);
	MYSERVER_FILE::splitPath(cgipath,td->cgiRoot,td->cgiFile);
	connTable[connIndex].envString[0]='\0';
	buildCGIEnvironmentString(td,connTable[connIndex].envString);

	ZeroMemory(&ExtCtrlBlk, sizeof(ExtCtrlBlk));
	ExtCtrlBlk.cbSize = sizeof(ExtCtrlBlk);
	ExtCtrlBlk.dwVersion = MAKELONG(HSE_VERSION_MINOR, HSE_VERSION_MAJOR);
	ExtCtrlBlk.GetServerVariable = GetServerVariableExport;
	ExtCtrlBlk.ReadClient  = ReadClientExport;
	ExtCtrlBlk.WriteClient = WriteClientExport;
	ExtCtrlBlk.ServerSupportFunction = ServerSupportFunctionExport;
	ExtCtrlBlk.ConnID = (HCONN) (connIndex + 1);
	ExtCtrlBlk.dwHttpStatusCode = 200;
	ExtCtrlBlk.lpszLogData[0] = '0';
	ExtCtrlBlk.lpszMethod = td->request.CMD;
	ExtCtrlBlk.lpszQueryString = td->request.URIOPTS;
	ExtCtrlBlk.lpszPathInfo = td->pathInfo;
	if(td->pathInfo[0])
		ExtCtrlBlk.lpszPathTranslated = td->pathTranslated;
	else
		ExtCtrlBlk.lpszPathTranslated = td->filenamePath;
	ExtCtrlBlk.cbTotalBytes = td->inputData.getFileSize();
	ExtCtrlBlk.cbAvailable = 0;
	ExtCtrlBlk.lpbData = 0;
	ExtCtrlBlk.lpszContentType = (LPSTR)(&(td->request.CONTENT_TYPE[0]));

	HttpExtensionProc = (PFN_HTTPEXTENSIONPROC)GetProcAddress(AppHnd, "HttpExtensionProc");
	if (HttpExtensionProc == NULL) 
	{
		((vhost*)(td->connection->host))->warningsLogWrite("Failure to get pointer to HttpExtensionProc() in ISAPI application module\r\n");
		FreeLibrary(AppHnd);
		return raiseHTTPError(td,connection,e_500);
	}

	Ret = HttpExtensionProc(&ExtCtrlBlk);
	if (Ret == HSE_STATUS_PENDING) 
	{
		WaitForSingleObject(connTable[connIndex].ISAPIDoneEvent, ISAPI_TIMEOUT);
	}
	/*
	*Flush the output to the client.
	*/
	u_long len=0;
	if(connTable[connIndex].td->outputData.getHandle())
	{
		connTable[connIndex].td->outputData.setFilePointer(0);
		connTable[connIndex].td->outputData.readFromFile(connTable[connIndex].td->buffer,connTable[connIndex].td->buffersize,&len);
	}

	u_long headerSize=0;
	for(u_long i=0;i<len;i++)
	{
		if(connTable[connIndex].td->buffer[i]=='\n')
		{
			if(connTable[connIndex].td->buffer[i+1]=='\n')
			{
				/*
				*The HTTP header ends with a \n\n sequence so 
				*determinate where it ends and set the header size
				*to i + 2.
				*/
				headerSize=i+2;
				break;
			}
		}
		if(connTable[connIndex].td->buffer[i]=='\r')
		{
			if(connTable[connIndex].td->buffer[i+1]=='\n')
			{
				if(connTable[connIndex].td->buffer[i+2]=='\r')
				{
					if(connTable[connIndex].td->buffer[i+3]=='\n')
					{
						/*
						*The HTTP header ends with a \r\n\r\n sequence so 
						*determinate where it ends and set the header size
						*to i + 4.
						*/
						connTable[connIndex].td->buffer[i+2]='\0';
						headerSize=i+4;
						break;
					}
				}
			}
		}

	}

	if(ExtCtrlBlk.dwHttpStatusCode==200)/*HTTP status code is 200*/
	{
		char chunk_size[15];
		strcpy(td->response.TRANSFER_ENCODING,"chunked");
		buildHTTPResponseHeaderStruct(&td->response,td,connTable[connIndex].td->buffer);
		sprintf(connTable[connIndex].td->response.CONTENT_LENGTH,"%u",connTable[connIndex].td->outputData.getFileSize()-headerSize);
		buildHTTPResponseHeader(connTable[connIndex].td->buffer2,&(connTable[connIndex].td->response));
		connTable[connIndex].connection->socket.send(connTable[connIndex].td->buffer2,(int)strlen(connTable[connIndex].td->buffer2), 0);

		/*Send the first chunk*/
		sprintf(chunk_size,"%x\r\n",len-headerSize);
		connTable[connIndex].connection->socket.send(chunk_size,strlen(chunk_size), 0);

		connTable[connIndex].connection->socket.send((char*)(connTable[connIndex].td->buffer+headerSize),len-headerSize, 0);

		/*Continue to send chunks*/
		for(;;)
		{
			connTable[connIndex].td->outputData.readFromFile(connTable[connIndex].td->buffer,connTable[connIndex].td->buffersize,&len);
			if(len==0)
				break;
			sprintf(chunk_size,"\r\n%x\r\n",len);
			connTable[connIndex].connection->socket.send(chunk_size,strlen(chunk_size), 0);
			connTable[connIndex].connection->socket.send((char*)(connTable[connIndex].td->buffer),len, 0);
		}
		/*To terminate the chunks transfer send a zero len chunk*/
		connTable[connIndex].connection->socket.send("\r\n0\r\n\r\n",7, 0);
		connTable[connIndex].td->outputData.closeFile();
	}
	else	
	{
		/*
		*Return the correct HTTP error
		*/
		raiseHTTPError(td,connection,getErrorIDfromHTTPStatusCode(ExtCtrlBlk.dwHttpStatusCode));
	}
	int retvalue=0;

	switch(Ret) 
	{
		case HSE_STATUS_SUCCESS_AND_KEEP_CONN:
			retvalue=1;
			break;
		case 0:
		case HSE_STATUS_SUCCESS:
		case HSE_STATUS_ERROR:
		default:
			retvalue=0;
			break;
	}
	if(!FreeLibrary(AppHnd))
	{
		((vhost*)(td->connection->host))->warningsLogWrite("Failure to FreeLibrary in ISAPI module");
		((vhost*)(td->connection->host))->warningsLogWrite(cgipath);
		((vhost*)(td->connection->host))->warningsLogWrite("\r\n");
	}

	connTable[connIndex].Allocated = FALSE;
	return retvalue;
}


BOOL WINAPI ServerSupportFunctionExport(HCONN hConn, DWORD dwHSERRequest,LPVOID lpvBuffer, LPDWORD lpdwSize, LPDWORD lpdwDataType) 
{
	ConnTableRecord *ConnInfo;

	ConnInfo = HConnRecord(hConn);
	if (ConnInfo == NULL) 
	{
		preparePrintError();
		printf("ServerSupportFunctionExport: invalid hConn\r\n");
		endPrintError();
		return FALSE;
	}
	switch (dwHSERRequest) 
	{
		case HSE_REQ_MAP_URL_TO_PATH_EX:
			HSE_URL_MAPEX_INFO  *mapInfo;
			mapInfo=(HSE_URL_MAPEX_INFO*)lpdwDataType;
			getPath(ConnInfo->td,mapInfo->lpszPath,(char*)lpvBuffer,FALSE);
			mapInfo->cchMatchingURL=(DWORD)strlen((char*)lpvBuffer);
			mapInfo->cchMatchingPath=(DWORD)strlen(mapInfo->lpszPath);
			mapInfo->dwFlags = HSE_URL_FLAGS_WRITE|HSE_URL_FLAGS_SCRIPT|HSE_URL_FLAGS_EXECUTE;
			break;
		case HSE_REQ_MAP_URL_TO_PATH:
			char URI[MAX_PATH];
			if(((char*)lpvBuffer)[0])
				strcpy(URI,(char*)lpvBuffer);
			else
				lstrcpyn(URI,ConnInfo->td->request.URI,strlen(ConnInfo->td->request.URI)-strlen(ConnInfo->td->pathInfo)+1);
			getPath(ConnInfo->td,(char*)lpvBuffer,URI,FALSE);
			MYSERVER_FILE::completePath((char*)lpvBuffer);
			*lpdwSize=(DWORD)strlen((char*)lpvBuffer);
			break;
		case HSE_REQ_SEND_URL_REDIRECT_RESP:
			return ISAPIRedirect(ConnInfo->td,ConnInfo->connection,(char *)lpvBuffer);
			break;
		case HSE_REQ_SEND_URL:
			return ISAPISendURI(ConnInfo->td,ConnInfo->connection,(char *)lpvBuffer);
			break;
		case HSE_REQ_SEND_RESPONSE_HEADER:
			return ISAPISendHeader(ConnInfo->td,ConnInfo->connection,(char *)lpvBuffer);
			break;
		case HSE_REQ_DONE_WITH_SESSION:
			ConnInfo->td->response.httpStatus=*(DWORD*)lpvBuffer;
			SetEvent(ConnInfo->ISAPIDoneEvent);
			break;
		case HSE_REQ_IS_KEEP_CONN:
			if(!lstrcmpi(ConnInfo->td->request.CONNECTION,"Keep-Alive"))
				*((BOOL*)lpvBuffer)=TRUE;
			else
				*((BOOL*)lpvBuffer)=FALSE;
			break;
		default:
			return FALSE;
	}
	return TRUE;
}

/*
*Add a connection to the table.
*/
ConnTableRecord *HConnRecord(HCONN hConn) 
{
	u_long connIndex;

	connIndex =((u_long) hConn) - 1;
	ConnTableRecord *ConnInfo;
	if ((connIndex < 0) || (connIndex >= max_Connections)) 
	{
		return NULL;
	}
	ConnInfo = &(connTable[connIndex]);
	if (ConnInfo->Allocated == FALSE) 
	{
		return NULL;
	}
	return ConnInfo;
}

/*
*Send an HTTP redirect.
*/
int ISAPIRedirect(httpThreadContext* td,LPCONNECTION a,char *URL) 
{
    return sendHTTPRedirect(td,a,URL);
}
/*
*Send an HTTP URI.
*/
int ISAPISendURI(httpThreadContext* td,LPCONNECTION a,char *URL)
{
	return sendHTTPRESOURCE(td,a,URL,FALSE,FALSE);
}
/*
*Send the ISAPI header.
*/
int ISAPISendHeader(httpThreadContext* td,LPCONNECTION a,char *URL)
{
	return sendHTTPRESOURCE(td,a,URL,FALSE,TRUE);
}
/*
*Write directly to the output.
*/
BOOL WINAPI WriteClientExport(HCONN hConn, LPVOID Buffer, LPDWORD lpdwBytes, DWORD /*dwReserved*/)
{
	ConnTableRecord *ConnInfo;

	if(*lpdwBytes==0)
		return TRUE;
	ConnInfo = HConnRecord(hConn);
	if (ConnInfo == NULL) 
	{
		((vhost*)(ConnInfo->td->connection->host))->warningsLogWrite("WriteClientExport: invalid hConn\r\n");
		return FALSE;
	}
	if(ConnInfo->td->outputData.getHandle()==0)
	{
		char stdOutFilePath[MAX_PATH];
		getdefaultwd(stdOutFilePath,MAX_PATH);
		sprintf(&stdOutFilePath[strlen(stdOutFilePath)],"/stdOutFile_%u",ConnInfo->td->id);
		ConnInfo->td->outputData.createTemporaryFile(stdOutFilePath);
	}
	DWORD nbw;
	ConnInfo->td->outputData.writeToFile((char*)Buffer,*lpdwBytes,&nbw);

	*lpdwBytes = nbw;
	if (nbw) 
	{
		return TRUE;
	}
	else 
	{
		return FALSE;
	}
}
/*
*Read directly from the client.
*/
BOOL WINAPI ReadClientExport(HCONN hConn, LPVOID lpvBuffer, LPDWORD lpdwSize ) 
{
	ConnTableRecord *ConnInfo;
	u_long NumRead;

	ConnInfo = HConnRecord(hConn);
	if (ConnInfo == NULL) 
	{
		((vhost*)(ConnInfo->td->connection->host))->warningsLogWrite("ReadClientExport: invalid hConn\r\n");
		return FALSE;
	}
	ConnInfo->td->inputData.readFromFile((char*)lpvBuffer,*lpdwSize,&NumRead);
	if (NumRead == -1) 
	{
		*lpdwSize = 0;
		return FALSE;
	}
	else 
	{
		*lpdwSize = (DWORD)NumRead;
		return TRUE;
	}
}


/*
*Get server environment variable.
*/
BOOL WINAPI GetServerVariableExport(HCONN hConn, LPSTR lpszVariableName, LPVOID lpvBuffer, LPDWORD lpdwSize) 
{
	ConnTableRecord *ConnInfo;
	BOOL ret =TRUE;
	ConnInfo = HConnRecord(hConn);
	if (ConnInfo == NULL) 
	{
		preparePrintError();
		printf("GetServerVariableExport: invalid hConn\r\n");
		endPrintError();
		return FALSE;
	}

	if (!strcmp(lpszVariableName, "ALL_HTTP")) 
	{
		if(buildAllHttpHeaders(ConnInfo->td,ConnInfo->connection,lpvBuffer,lpdwSize))
			ret=TRUE;
		else
		{
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
			ret=FALSE;
		}
			
	}else if(!strcmp(lpszVariableName, "ALL_RAW")) 
	{
		if(buildAllRawHeaders(ConnInfo->td,ConnInfo->connection,lpvBuffer,lpdwSize))
			ret=TRUE;
		else
		{
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
			ret=FALSE;
		}
			
	}
	else
	{
		/*
		*Find in ConnInfo->envString the value lpszVariableName and copy next string in lpvBuffer.
		*/
		((char*)lpvBuffer)[0]='\0';
		char *localEnv=ConnInfo->envString;
		int variableNameLen=(int)strlen(lpszVariableName);
		for(u_long i=0;;i+=(u_long)strlen(&localEnv[i])+1)
		{
			if(((localEnv[i+variableNameLen])=='=')&&(!strncmp(&localEnv[i],lpszVariableName,variableNameLen)))
			{
				strncpy((char*)lpvBuffer,&localEnv[i+variableNameLen+1],*lpdwSize);
				break;
			}
			else if((localEnv[i]=='\0') && (localEnv[i+1]=='\0'))
			{
				break;
			}
		}
	}
	*lpdwSize=strlen((char*)lpvBuffer);
	return ret;
}
/*
*Build the string that contains all the HTTP headers.
*/
BOOL buildAllHttpHeaders(httpThreadContext* td,LPCONNECTION /*a*/,LPVOID output,LPDWORD dwMaxLen)
{
	DWORD valLen=0;
	DWORD maxLen=*dwMaxLen;
	char *ValStr=(char*)output;

	if(td->request.ACCEPT[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_ACCEPT:%s\n",td->request.ACCEPT);
	else if(valLen+30<maxLen) 
		return 0;

	if(td->request.ACCEPTLAN[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_ACCEPT_LANGUAGE:%s\n",td->request.ACCEPTLAN);
	else if(valLen+30<maxLen) 
		return 0;

	if(td->request.ACCEPTENC[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_ACCEPT_ENCODING:%s\n",td->request.ACCEPTENC);
	else if(valLen+30<maxLen) 
		return 0;

	if(td->request.CONNECTION[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_CONNECTION:%s\n",td->request.CONNECTION);
	else if(valLen+30<maxLen) 
		return 0;

	if(td->request.COOKIE[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_COOKIE:%s\n",td->request.COOKIE);
	else if(valLen+30<maxLen) 
		return 0;

	if(td->request.HOST[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_HOST:%s\n",td->request.HOST);
	else if(valLen+30<maxLen) 
		return 0;

	if(td->request.DATE[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_DATE:%s\n",td->request.DATE);
	else if(valLen+30<maxLen) 
		return 0;

	if(td->request.MODIFIED_SINCE[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_IF_MODIFIED_SINCE:%s\n",td->request.MODIFIED_SINCE);
	else if(valLen+30<maxLen) 
		return 0;

	if(td->request.REFERER[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_REFERER:%s\n",td->request.REFERER);
	else if(valLen+30<maxLen) 
		return 0;

	if(td->request.PRAGMA[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_PRAGMA:%s\n",td->request.PRAGMA);
	else if(valLen+30<maxLen) 
		return 0;

	if(td->request.USER_AGENT[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_USER_AGENT:%s\n",td->request.USER_AGENT);
	else if(valLen+30<maxLen)
		return 0;

	if(td->request.VER[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_MIME_VERSION:%s\n",td->request.VER);
	else if(valLen+30<maxLen) 
		return 0;

	if(td->request.FROM[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"HTTP_FROM:%s\n",td->request.FROM);
	else if(valLen+30<maxLen) 
		return 0;
	return 1;
}

/*
*Build the string that contains all the headers.
*/
BOOL buildAllRawHeaders(httpThreadContext* td,LPCONNECTION a,LPVOID output,LPDWORD dwMaxLen)
{
	DWORD valLen=0;
	DWORD maxLen=*dwMaxLen;
	char *ValStr=(char*)output;
	if(buildAllHttpHeaders(td,a,output,dwMaxLen)==0)
		return 0;
	valLen=(DWORD)strlen(ValStr);

	if(td->pathInfo[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"PATH_INFO:%s\n",td->pathInfo);
	else if(valLen+30<maxLen) 
		return 0;
	
	if(td->pathTranslated[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"PATH_INFO:%s\n",td->pathTranslated);
	else if(valLen+30<maxLen) 
		return 0;

	if(td->request.URIOPTS[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"QUERY_STRING:%s\n",td->request.URIOPTS[0]);
	else if(valLen+30<maxLen) 
		return 0;

	if(td->request.CMD[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"REQUEST_METHOD:%s\n",td->request.CMD[0]);
	else if(valLen+30<maxLen) 
		return 0;

	if(td->filenamePath[0] && (valLen+30<maxLen))
		valLen+=sprintf(&ValStr[valLen],"SCRIPT_FILENAME:%s\n",td->filenamePath[0]);
	else if(valLen+30<maxLen) 
		return 0;

	if(valLen+30<maxLen)
		valLen+=sprintf(&ValStr[valLen],"SERVER_PORT:%u\n",td->connection->localPort);
	else if(valLen+30<maxLen) 
		return 0;

	if(valLen+30<maxLen)
		valLen+=sprintf(&ValStr[valLen],"SERVER_SIGNATURE:<address>%s</address>\n",versionOfSoftware);
	else if(valLen+30<maxLen) 
		return 0;

	if(td->connection->ipAddr[0] && valLen+30<maxLen)
		valLen+=sprintf(&ValStr[valLen],"REMOTE_ADDR:\n",td->connection->ipAddr);
	else if(valLen+30<maxLen) 
		return 0;

	if(td->connection->port && valLen+30<maxLen)
		valLen+=sprintf(&ValStr[valLen],"REMOTE_PORT:%u\n",td->connection->port);
	else if(valLen+30<maxLen) 
		return 0;

	if(valLen+30<maxLen)
		valLen+=sprintf(&ValStr[valLen],"SERVER_ADMIN:%s\n",lserver->getServerAdmin());
	else if(valLen+30<maxLen) 
		return 0;

	if(valLen+MAX_PATH<maxLen)
	{
		valLen+=sprintf(&ValStr[valLen],"SCRIPT_NAME:");
		lstrcpyn(&ValStr[valLen],td->request.URI,strlen(td->request.URI)-strlen(td->pathInfo)+1);
		valLen+=(DWORD)strlen(td->request.URI)-strlen(td->pathInfo)+1;
		valLen+=(DWORD)sprintf(&ValStr[valLen],"\n");
	}
	else if(valLen+30<maxLen) 
		return 0;
	return 1;
}

#endif