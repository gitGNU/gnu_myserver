/*
*myServer
*Copyright (C) Rocky_10_Balboa
*This library is free software; you can redistribute it and/or
*modify it under the terms of the GNU Library General Public
*License as published by the Free Software Foundation; either
*version 2 of the License, or (at your option) any later version.

*This library is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*Library General Public License for more details.

*You should have received a copy of the GNU Library General Public
*License along with this library; if not, write to the
*Free Software Foundation, Inc., 59 Temple Place - Suite 330,
*Boston, MA  02111-1307, USA.
*/

#include "..\include\HTTP.h"
#include "..\include\cserver.h"
#include "..\include\security.h"
#include "..\include\AMMimeUtils.h"
#include "..\include\filemanager.h"
#include "..\include\sockets.h"
#include "..\include\utility.h"
#include <direct.h>

BOOL sendHTTPDIRECTORY(httpThreadContext* td,LPCONNECTION s,char* folder)
{
	/*
	*Send the content of a folder if there are not any default
	*file to send.
	*/
	static char filename[MAX_PATH];
	static DWORD startChar=lstrlen(lserver->getPath())+1;
	
	if(getPathRecursionLevel(folder)<1)
	{
		return raiseHTTPError(td,s,e_401);
	}
	ZeroMemory(td->buffer2,200);
	_finddata_t fd;
	sprintf(filename,"%s/*.*",folder);
	sprintf(td->buffer2,"%s",msgFolderContents);
	lstrcat(td->buffer2," ");
	lstrcat(td->buffer2,&folder[startChar]);
	lstrcat(td->buffer2,"\\<P>\n<HR>");
	intptr_t ff;
	ff=_findfirst(filename,&fd);

	if(ff==-1)
	{
		if(GetLastError()==ERROR_ACCESS_DENIED)
		{
			/*
			*If client have tried to post a login
			*and a password more times send error 401.
			*/
			if(s->nTries > 2)
			{
				return raiseHTTPError(td,s,e_401);
			}
			else
			{
				s->nTries++;
				return raiseHTTPError(td,s,e_401AUTH);
			}
		}
		else
		{
			return raiseHTTPError(td,s,e_404);
		}
	}
	/*
	*With the current code we build the HTML TABLE that describes the files in the folder.
	*/
	sprintf(td->buffer2+lstrlen(td->buffer2),"<TABLE><TR><TD>%s</TD><TD>%s</TD><TD>%s</TD></TR>",msgFile,msgLModify,msgSize);
	static char fileSize[10];
	static char fileTime[20];
	do
	{	
		if(fd.name[0]=='.')
			continue;
		lstrcat(td->buffer2,"<TR><TD><A HREF=\"");
		lstrcat(td->buffer2,&folder[startChar]);
		lstrcat(td->buffer2,"/");
		lstrcat(td->buffer2,fd.name);
		lstrcat(td->buffer2,"\">");
		lstrcat(td->buffer2,fd.name);
		lstrcat(td->buffer2,"</TD><TD>");
			
		tm *st=gmtime(&fd.time_write);

		sprintf(fileTime,"%u\\%u\\%u-%u:%u:%u System time",st->tm_wday,st->tm_mon,st->tm_year,st->tm_hour,st->tm_min,st->tm_sec);
		lstrcat(td->buffer2,fileTime);

		lstrcat(td->buffer2,"</TD><TD>");
		if(fd.attrib & FILE_ATTRIBUTE_DIRECTORY)
		{
			lstrcat(td->buffer2,"<dir>");
		}
		else
		{
			sprintf(fileSize,"%i bytes",fd.size);
			lstrcat(td->buffer2,fileSize);
		}
		lstrcat(td->buffer2,"</TD></TR>\n");
	}while(!_findnext(ff,&fd));
	lstrcat(td->buffer2,"</TABLE>\n<HR>");
	lstrcat(td->buffer2,msgRunOn);
	lstrcat(td->buffer2," myServer ");
	lstrcat(td->buffer2,versionOfSoftware);
	_findclose(ff);
	char *buffer2Loop=td->buffer2;
	while(*buffer2Loop++)
		if(*buffer2Loop=='\\')
			*buffer2Loop='/';
	buildDefaultHTTPResponseHeader(&(td->response));
	sprintf(td->response.CONTENTS_DIM,"%u",lstrlen(td->buffer2));
	buildHTTPResponseHeader(td->buffer,&(td->response));
	ms_send(s->socket,td->buffer,lstrlen(td->buffer), 0);
	ms_send(s->socket,td->buffer2,lstrlen(td->buffer2), 0);

	return 1;

}
BOOL sendHTTPFILE(httpThreadContext* td,LPCONNECTION s,char *filenamePath,BOOL OnlyHeader,int firstByte,int lastByte)
{
	/*
	*With this code we send a file through the HTTP protocol.
	*/
	MYSERVER_FILE_HANDLE h;
	h=ms_OpenFile(filenamePath,MYSERVER_FILE_OPEN_IFEXISTS|MYSERVER_FILE_OPEN_READ);

	if(h==0)
	{	
		if(GetLastError()==ERROR_ACCESS_DENIED)
		{
			if(s->nTries > 2)
			{
				return raiseHTTPError(td,s,e_401);
			}
			else
			{
				s->nTries++;
				return raiseHTTPError(td,s,e_401AUTH);
			}
		}
		else
		{
			return 0;
		}
	}

	/*
	*If h!=0.
	*/
	DWORD bytesToSend=getFileSize(h);
	if(lastByte == -1)
	{
		lastByte=bytesToSend;
	}
	else
	{
		lastByte=min((DWORD)lastByte,bytesToSend);
	}
	/*
	*bytesToSend is the interval between the first and the last byte.
	*/
	bytesToSend=lastByte-firstByte;

	/*
	*If failed to set the file pointer returns an internal server error.
	*/
	if(setFilePointer(h,firstByte))
	{
		return raiseHTTPError(td,s,e_500);
	}

	td->buffer[0]='\0';

	sprintf(td->response.CONTENTS_DIM,"%u",bytesToSend);
	buildHTTPResponseHeader(td->buffer,&td->response);
	ms_send(s->socket,td->buffer,lstrlen(td->buffer), 0);

	/*
	*If is requested only the header returns from the function here; HEAD request.
	*/
	if(OnlyHeader)
		return 1;

	if(lserver->getVerbosity()>2)
	{
		char msg[500];
		sprintf(msg,"%s %s\n",msgSending,filenamePath);
		warningsLogWrite(msg);
	}
	for(;;)
	{
		DWORD nbr;
		/*
		*Read from file the bytes to sent.
		*/
		ms_ReadFromFile(h,td->buffer,min(bytesToSend,td->buffersize),&nbr);
		/*
		*When the bytes read from the file are zero, stop to send the file.
		*/
		if(nbr==0)
			break;
		/*
		*If there are bytes to send, send them.
		*/
		if(ms_send(s->socket,td->buffer,nbr, 0) == SOCKET_ERROR)
			break;
	}
	ms_CloseFile(h);
	return 1;

}

/*
*Main function to send a resource to a client.
*/
BOOL sendHTTPRESOURCE(httpThreadContext* td,LPCONNECTION s,char *filename,BOOL systemrequest,BOOL OnlyHeader,int firstByte,int lastByte)
{
	/*
	*With this code we manage a request of a file or a folder or anything that we must send
	*over the HTTP.
	*/
	td->buffer[0]='\0';
	buildDefaultHTTPResponseHeader(&td->response);

	static char ext[MAX_PATH];
	static char data[MAX_PATH];
	getPath(td->filenamePath,filename,systemrequest);

	/*
	*getMIME return TRUE if the ext is registered by a CGI.
	*/
	if(getMIME(td->response.MIME,filename,ext,data))
	{
		if(ms_FileExists(td->filenamePath))
			if(sendCGI(td,s,td->filenamePath,ext,data))
				return 1;
		return raiseHTTPError(td,s,e_404);
	}

	/*
	*If there are not any extension then we do one of this in order:
	1)We send the default file in the folder.
	2)We send the folder content.
	3)We send an error.
	*/
	if(ms_IsFolder(td->filenamePath))
	{
		static char defaultFileName[MAX_PATH];
		sprintf(defaultFileName,"%s%s",td->filenamePath,lserver->getDefaultFilenamePath());

		if(sendHTTPFILE(td,s,defaultFileName,OnlyHeader,firstByte,lastByte))
			return 1;

		if(sendHTTPDIRECTORY(td,s,td->filenamePath))
			return 1;	

		return raiseHTTPError(td,s,e_404);
	}
	

	/*
	*myServer CGI format.
	*/
	if(!lstrcmpi(ext,"mscgi"))
	{
		char *target;
		if(td->request.URIOPTSPTR)
			target=td->request.URIOPTSPTR;
		else
			target=(char*)&td->request.URIOPTS;
		if(sendMSCGI(td,s,td->filenamePath,target))
			return 1;
		return raiseHTTPError(td,s,e_404);
	}
	if(sendHTTPFILE(td,s,td->filenamePath,OnlyHeader,firstByte,lastByte))
		return 1;

	return raiseHTTPError(td,s,e_404);
}
/*
*Sends the myServer CGI; differently form standard CGI this don't need a new process to run
*so it is faster.
*/
BOOL sendMSCGI(httpThreadContext* td,LPCONNECTION s,char* exec,char* cmdLine)
{
	/*
	*This is the code for manage a .mscgi file.
	*This files differently from standard CGI don't need a new process to run
	*but are allocated in the caller process virtual space.
	*Usually these files are faster than standard CGI.
	*Actually myServerCGI(.mscgi) is only at an alpha status.
	*/
#ifdef WIN32
	static HMODULE hinstLib; 
    static CGIMAIN ProcMain;
	static CGIINIT ProcInit;
 
    hinstLib = LoadLibrary(exec); 
	td->buffer2[0]='\0';
	if (hinstLib) 
    { 
		ProcInit = (CGIINIT) GetProcAddress(hinstLib, "initialize");
		ProcMain = (CGIMAIN) GetProcAddress(hinstLib, "main"); 
		if(ProcInit && ProcMain)
		{
			(ProcInit)((LPVOID)&td->buffer[0],(LPVOID)&td->buffer2[0],(LPVOID)&td->response,(LPVOID)&td->request);
			(ProcMain)(cmdLine);
		}
        FreeLibrary(hinstLib); 
    } 
	else
	{
		if(GetLastError()==ERROR_ACCESS_DENIED)
		{
			if(s->nTries > 2)
			{
				return raiseHTTPError(td,s,e_403);
			}
			else
			{
				s->nTries++;
				return raiseHTTPError(td,s,e_401AUTH);
			}
		}
		else
		{
			return raiseHTTPError(td,s,e_404);
		}
	}
	static int len;
	len=lstrlen(td->buffer2);
	sprintf(td->response.CONTENTS_DIM,"%u",len);
	buildHTTPResponseHeader(td->buffer,&td->response);
	ms_send(s->socket,td->buffer,lstrlen(td->buffer), 0);
	ms_send(s->socket,td->buffer2,len, 0);
	return 1;
#endif
}

/*
*This is the HTTP protocol parser.
*/
BOOL controlHTTPConnection(LPCONNECTION a,char *b1,char *b2,int bs1,int bs2,DWORD nbtr,LOGGEDUSERID *imp)
{
	httpThreadContext td;
	td.buffer=b1;
	td.buffer2=b2;
	td.buffersize=bs1;
	td.buffersize2=bs2;
	td.nBytesToRead=nbtr;
	td.hImpersonation=*imp;
	/*
	*In this function there is the HTTP protocol parse.
	*The request is mapped into a HTTP_REQUEST_HEADER structure
	*And at the end of this every command is treated
	*differently. We use this mode for parse the HTTP
	*because especially in the CGI is requested a continous
	*HTTP header access.
	*Before mapping the header in the structure 
	*control if this is a regular request.
	*The HTTP header ends with a \r\n\r\n sequence.
	*/
	

	/*
	*Control if the HTTP header is a valid header.
	*/
	DWORD i=0,j=0,max=0;
	BOOL containOpts;
	DWORD nLines,maxTotChars;
	DWORD validRequest=validHTTPRequest(&td,&nLines,&maxTotChars);


	/*
	*If the server verbosity is > 4 then save the HTTP request header.
	*/
	if(lserver->getVerbosity()>4)
	{
		td.buffer[td.nBytesToRead]='\n';
		td.buffer[td.nBytesToRead+1]='\0';
		warningsLogWrite(td.buffer);
	}
	/*
	*If the header is an invalid request send the correct error message to the client.
	*/
	if(validRequest==0)
	{
		raiseHTTPError(&td,a,e_400);
		/*
		*Returning Zero we remove the connection from the connections list.
		*/
		return 0;
	}

	const int max_URI=MAX_PATH+200;
	const char seps[]   = " ,\t\n\r";
	const char cmdseps[]   = ": ,\t\n\r";

	static char *token=0;
	static char command[96];
	/*
	*Reset the request structure.
	*/
	ZeroMemory(&td.request,sizeof(td.request));

	static int nLineControlled;
	nLineControlled=0;
	static BOOL lineControlled;
	token=td.buffer;

	token = strtok( token, cmdseps );
	controlAnotherLine:
	/*
	*Reset the flag lineControlled.
	*/
	lineControlled=FALSE;

	/*
	*Copy in the command string the header field name.
	*/
	lstrcpy(command,token);
	
	
	nLineControlled++;
	if(nLineControlled==1)
	{
		/*
		*The first line has the form:
		*GET /index.html HTTP/1.1
		*Version of the protocol in the HTTP_REQUEST_HEADER
		*struct is leaved as a number.
		*For example HTTP/1.1 in the struct is 1.1
		*/


		/*GET*/
		if(!lstrcmpi(command,"GET"))
		{
			lineControlled=TRUE;
			lstrcpy(td.request.CMD,"GET");
		
			token = strtok( NULL, " ,\t\n\r" );
			max=lstrlen(token);
			containOpts=FALSE;
			for(i=0;i<max;i++)
			{
				if(token[i]=='?')
				{
					containOpts=TRUE;
					break;
				}
				td.request.URI[i]=token[i];				
			}
			td.request.URI[i]='\0';

			if(containOpts)
			{
				for(j=0;i<max;j++)
				{
					td.request.URIOPTS[j]=token[++i];
				}
			}
			token = strtok( NULL, seps );
			lstrcpy(td.request.VER,token);
			StrTrim(td.request.VER,"HTTP /");
			StrTrim(td.request.URI," /");
			StrTrim(td.request.URIOPTS," /");
			if(lstrlen(td.request.URI)>max_URI)
			{
				raiseHTTPError(&td,a,e_414);
				
				return 0;
			}
			else
			{
				max=lstrlen(td.request.URI);
			}
			td.request.URIOPTSPTR=0;
		}
		/*POST*/
		if(!lstrcmpi(command,"POST"))
		{
			lineControlled=TRUE;
			lstrcpy(td.request.CMD,"POST");
		
			token = strtok( NULL, " ,\t\n\r" );
			max=lstrlen(token);
			containOpts=FALSE;
			for(i=0;i<max;i++)
			{

				if(token[i]=='?')
				{
					containOpts=TRUE;
					break;
				}
				td.request.URI[i]=token[i];				
			}
			td.request.URI[i]='\0';
			if(containOpts)
			{
				for(j=0;i<max;j++)
				{
					td.request.URIOPTS[j]=token[++i];
				}
			}
			td.request.URIOPTS[0]='\0';

			/*
			*URIOPTSPTR points to the first byte in the buffer that are the data send by the
            *client.
			*/
			td.request.URIOPTSPTR=&td.buffer[maxTotChars];
			td.buffer[max(td.nBytesToRead,td.buffersize)]='\0';

			token = strtok( NULL, seps );
			lstrcpy(td.request.VER,token);
			StrTrim(td.request.VER,"HTTP /");
			StrTrim(td.request.URI," /");
			if(lstrlen(td.request.URI)>max_URI)
			{
				raiseHTTPError(&td,a,e_414);
				
				return 0;
			}
			else
			{
				max=lstrlen(td.request.URI);
			}
		}
		/*HEAD*/
		if(!lstrcmpi(command,"HEAD"))
		{
			lineControlled=TRUE;
			lstrcpy(td.request.CMD,"HEAD");
		
			token = strtok( NULL, seps );
			lstrcpy(td.request.URI,token);
			token = strtok( NULL, seps );
			lstrcpy(td.request.VER,token);

			StrTrim(td.request.URI," /");
			if(lstrlen(td.request.URI)>max_URI)
			{
				raiseHTTPError(&td,a,e_414);
				
				return 0;
			}
		}

		if(!lstrcmpi(command,""))
		{
			raiseHTTPError(&td,a,e_501);
			
			return 0;
		}
	}
	/*User-Agent*/
	if(!lstrcmpi(command,"User-Agent"))
	{
		token = strtok( NULL, "\r\n" );
		lineControlled=TRUE;
		lstrcpy(td.request.USER_AGENT,token);
		StrTrim(td.request.USER_AGENT," ");
	}
	/*Authorization*/
	if(!lstrcmpi(command,"Authorization"))
	{
		token = strtok( NULL, "\r\n" );
		lineControlled=TRUE;		
		ZeroMemory(td.buffer2,300);
		/*
		*Basic authorization in base64 is login:password.
		*Assume that it is Basic anyway.
		*/
		int len=lstrlen(token);
		char *base64=base64Utils.Decode(&token[lstrlen("Basic: ")],&len);
		char* lbuffer2=base64;
		while(*lbuffer2!=':')
		{
			a->login[lstrlen(a->login)]=*lbuffer2++;
		}
		lbuffer2++;
		while(*lbuffer2)
		{
			a->password[lstrlen(a->password)]=*lbuffer2++;
		}
		free(base64);
	}
	/*Host*/
	if(!lstrcmpi(command,"Host"))
	{
		token = strtok( NULL, seps );
		lineControlled=TRUE;
		lstrcpy(td.request.HOST,token);
		StrTrim(td.request.HOST," ");
	}
	/*Accept*/
	if(!lstrcmpi(command,"Accept"))
	{
		token = strtok( NULL, "\n\r" );
		lineControlled=TRUE;
		lstrcat(td.request.ACCEPT,token);
		StrTrim(td.request.ACCEPT," ");
	}
	/*Accept-Language*/
	if(!lstrcmpi(command,"Accept-Language"))
	{
		token = strtok( NULL, "\n\r" );
		lineControlled=TRUE;
		lstrcpy(td.request.ACCEPTLAN,token);
		StrTrim(td.request.ACCEPTLAN," ");
	}
	/*Accept-Charset*/
	if(!lstrcmpi(command,"Accept-Charset"))
	{
		token = strtok( NULL, "\n\r" );
		lineControlled=TRUE;
		lstrcpy(td.request.ACCEPTCHARSET,token);
		StrTrim(td.request.ACCEPTCHARSET," ");
	}

	/*Accept-Encoding*/
	if(!lstrcmpi(command,"Accept-Encoding"))
	{
		token = strtok( NULL, "\n\r" );
		lineControlled=TRUE;
		lstrcpy(td.request.ACCEPTENC,token);
		StrTrim(td.request.ACCEPTENC," ");
	}
	/*Connection*/
	if(!lstrcmpi(command,"Connection"))
	{
		token = strtok( NULL, "\n\r" );
		lineControlled=TRUE;
		lstrcpy(td.request.CONNECTION,token);
		StrTrim(td.request.CONNECTION," ");
	}
	/*Range*/
	if(!lstrcmpi(command,"Range"))
	{
		ZeroMemory(td.request.RANGETYPE,30);
		ZeroMemory(td.request.RANGEBYTEBEGIN,30);
		ZeroMemory(td.request.RANGEBYTEEND,30);
		lineControlled=TRUE;
		token = strtok( NULL, "\r\n\t" );
		do
		{
			td.request.RANGETYPE[lstrlen(td.request.RANGETYPE)]=*token;
		}
		while(*token++ != '=');
		do
		{
			td.request.RANGEBYTEBEGIN[lstrlen(td.request.RANGEBYTEBEGIN)]=*token;
		}
		while(*token++ != '-');
		do
		{
			td.request.RANGEBYTEEND[lstrlen(td.request.RANGEBYTEEND)]=*token;
		}
		while(*token++);
		StrTrim(td.request.RANGETYPE,"= ");
		StrTrim(td.request.RANGEBYTEBEGIN,"- ");
		StrTrim(td.request.RANGEBYTEEND,"- ");
		
		if(lstrlen(td.request.RANGEBYTEBEGIN)==0)
			lstrcpy(td.request.RANGEBYTEBEGIN,"0");
		if(lstrlen(td.request.RANGEBYTEEND)==0)
			lstrcpy(td.request.RANGEBYTEEND,"-1");

	}
	/*Referer*/
	if(!lstrcmpi(command,"Referer"))
	{
		token = strtok( NULL, seps );
		lineControlled=TRUE;
		lstrcpy(td.request.REFERER,token);
		StrTrim(td.request.REFERER," ");
	}
	/*
	*If the line is not controlled arrive with the token
	*at the end of the line.
	*/
	if(!lineControlled)
	{
		token = strtok( NULL, "\n" );
	}
	token = strtok( NULL, cmdseps );
	if((token-td.buffer)<maxTotChars)
		goto controlAnotherLine; 
	/*
	*END REQUEST STRUCTURE BUILD
	*/

	/*
	*Record the request in the log file.
	*/
	accessesLogWrite(a->ipAddr);
	accessesLogWrite(":");
	accessesLogWrite(td.request.CMD);
	accessesLogWrite(" ");
	accessesLogWrite(td.request.URI);
	if(td.request.URIOPTS[0])
	{
		accessesLogWrite("?");
		accessesLogWrite(td.request.URIOPTS);
	}
	accessesLogWrite("\r\n");
	/*
	*End record the request in the structure
	*/
	
	
	/*
	*Here we control all the HTTP commands.
	*/
	if(!lstrcmpi(td.request.CMD,"GET"))
	{
		/*
		*How is expressly said in the rfc2616 a client that sends an 
		*HTTP/1.1 request MUST send a Host header.
		*Servers MUST report a 400 (Bad request) error if an HTTP/1.1
        *request does not include a Host request-header.
		*/
		if(lstrlen(td.request.HOST)==0)
		{
			raiseHTTPError(&td,a,e_400);
			return 0;
		}

		if(!lstrcmpi(td.request.RANGETYPE,"bytes"))
			sendHTTPRESOURCE(&td,a,td.request.URI,FALSE,FALSE,atoi(td.request.RANGEBYTEBEGIN),atoi(td.request.RANGEBYTEEND));
		else
			sendHTTPRESOURCE(&td,a,td.request.URI);
	}
	else if(!lstrcmpi(td.request.CMD,"POST"))
	{
		if(lstrlen(td.request.HOST)==0)
		{
			raiseHTTPError(&td,a,e_400);
			return 0;
		}
		if(!lstrcmpi(td.request.RANGETYPE,"bytes"))
			sendHTTPRESOURCE(&td,a,td.request.URI,FALSE,FALSE,atoi(td.request.RANGEBYTEBEGIN),atoi(td.request.RANGEBYTEEND));
		else
			sendHTTPRESOURCE(&td,a,td.request.URI);
	}
	else if(!lstrcmpi(td.request.CMD,"HEAD"))
	{
		if(lstrlen(td.request.HOST)==0)
		{
			raiseHTTPError(&td,a,e_400);
			
			return 0;
		}
		sendHTTPRESOURCE(&td,a,td.request.URI,FALSE,TRUE);
	}
	else
	{
		raiseHTTPError(&td,a,e_501);
		
		return 0;
	}
	if(lstrcmpi(td.request.CONNECTION,"Keep-Alive"))
	{
		
		return 0;
	}
	return 1;
}
/*
*Builds an HTTP header string starting from an HTTP_RESPONSE_HEADER structure.
*/
void buildHTTPResponseHeader(char *str,HTTP_RESPONSE_HEADER* response)
{
	/*
	*Here is builded the HEADER of a HTTP response.
	*Passing a HTTP_RESPONSE_HEADER struct this builds an header string.
	*Every directive ends with a \r\n sequence.
    */
	if(response->isError)
		sprintf(str,"HTTP/%s %s\r\nServer:%s\r\nContent-Type:%s\r\nContent-Length: %s\r\nStatus: \r\n",response->VER,response->ERROR_TYPE,response->SERVER_NAME,response->MIME,response->CONTENTS_DIM,response->ERROR_TYPE);
	else
		sprintf(str,"HTTP/%s 200 OK\r\nServer:%s\r\nContent-Type:%s\r\nContent-Length: %s\r\n",response->VER,response->SERVER_NAME,response->MIME,response->CONTENTS_DIM);
	if(lstrlen(response->DATE)>5)
	{
		lstrcat(str,"Date:");
		lstrcat(str,response->DATE);
		lstrcat(str,"\r\n");
	}
	if(lstrlen(response->DATEEXP)>5)
	{
		lstrcat(str,"Expires:");
		lstrcat(str,response->DATEEXP);
		lstrcat(str,"\r\n");
	}
	if(lstrlen(response->OTHER)>5)
	{
		lstrcat(str,response->OTHER);
		lstrcat(str,"\r\n");
	}
	/*
	*myServer support the bytes range
	*/
	lstrcat(str,"Accept-Ranges: bytes\r\n");
	/*
	*The HTTP header ends with a \r\n sequence.
	*/
	lstrcat(str,"\r\n");

}
/*
*Set the defaults value for a HTTP_RESPONSE_HEADER structure.
*/
void buildDefaultHTTPResponseHeader(HTTP_RESPONSE_HEADER* response)
{
	ZeroMemory(response,sizeof(HTTP_RESPONSE_HEADER));
	/*
	*By default use:
	*1) the MIME type of the page equal to text/html.
	*2) the version of the HTTP protocol to 1.1.
	*3) the date of the page and the expiration date to the current time.
	*4) then set the name of the server.
	*5) set the page that it is not an error page.
	*/
	lstrcpy(response->MIME,"text/html");
	lstrcpy(response->VER,"1.1");
	response->isError=FALSE;
	lstrcpy(response->DATE,getHTTPFormattedTime());
	lstrcpy(response->DATEEXP,getHTTPFormattedTime());
	sprintf(response->SERVER_NAME,"MyServer %s",versionOfSoftware);
}
/*
*Sends an error page to the client described by the connection.
*/
BOOL raiseHTTPError(httpThreadContext* td,LPCONNECTION a,int ID)
{
	if(ID==e_401AUTH)
	{
		sprintf(td->buffer2,"HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate: Basic\r\nServer: %s\r\nDate: %s\r\nContent-type: text/html\r\nContent-length: 0\r\n\r\n",lserver->getServerName(),getHTTPFormattedTime());
		ms_send(a->socket,td->buffer2,lstrlen(td->buffer2),0);
		return 1;
	}
	if(lserver->mustUseMessagesFiles())
	{
		 return sendHTTPRESOURCE(td,a,HTTP_ERROR_HTMLS[ID],TRUE);
		
	}
	buildDefaultHTTPResponseHeader(&(td->response));
	/*
	*Set the isError member to TRUE to build an error page.
	*/
	td->response.isError=TRUE;
	lstrcpy(td->response.ERROR_TYPE,HTTP_ERROR_MSGS[ID]);
	sprintf(td->response.CONTENTS_DIM,"%i",lstrlen(HTTP_ERROR_MSGS[ID]));
	buildHTTPResponseHeader(td->buffer,&td->response);
	lstrcat(td->buffer,HTTP_ERROR_MSGS[ID]);
	ms_send(a->socket,td->buffer,lstrlen(td->buffer), 0);
	return 1;
}

/*
*Sends the standard CGI to a client.
*/
BOOL sendCGI(httpThreadContext* td,LPCONNECTION s,char* filename,char* /*ext*/,char *exec)
{
	/*
	*Change the owner of the thread to the creator of the process.
	*This because anonymous users cannot go through our files.
	*/
	if(lserver->mustUseLogonOption())
		revertToSelf();
	char cmdLine[MAX_PATH*2];
	
	sprintf(cmdLine,"%s \"%s\"",exec,filename);

    /*
    *Use a temporary file to store CGI output.
    *Every thread has it own tmp file name(stdOutFilePath),
    *so use this name for the file that is going to be
    *created because more threads can access more CGI in the same time.
    */

	char currentpath[MAX_PATH];
	char stdOutFilePath[MAX_PATH];
	char stdInFilePath[MAX_PATH];
	ms_getcwd(currentpath,MAX_PATH);
	static DWORD id=0;
	id++;
	sprintf(stdOutFilePath,"%s/stdOutFile_%u",currentpath,id);
	sprintf(stdInFilePath,"%s/stdInFile_%u",currentpath,id);
		
	/*
	*Standard CGI uses standard output to output the result and the standard 
	*input to get other params like in a POST request.
	*/
	MYSERVER_FILE_HANDLE stdOutFile = ms_CreateTemporaryFile(stdOutFilePath);
	MYSERVER_FILE_HANDLE stdInFile = ms_CreateTemporaryFile(stdInFilePath);
	
	DWORD nbw;
	if(td->request.URIOPTSPTR)
	{
		ms_WriteToFile(stdInFile,td->request.URIOPTSPTR,atoi(td->request.CONTENTS_DIM),&nbw);
		char *endFileStr="\r\n\r\n\0";
		ms_WriteToFile(stdInFile,endFileStr,lstrlen(endFileStr),&nbw);
	}

	/*
	*With this code we execute the CGI process.
	*Use the td->buffer2 to build the environment string.
	*/
	START_PROC_INFO spi;
	spi.cmdLine = cmdLine;
	spi.stdError = (MYSERVER_FILE_HANDLE)0;
	spi.stdIn = (MYSERVER_FILE_HANDLE)stdInFile;
	spi.stdOut = (MYSERVER_FILE_HANDLE)stdOutFile;
	spi.envString=td->buffer2;
	/*
	*Build the environment string used by the CGI started
	*by the execHiddenProcess(...) function.
	*/
	buildCGIEnvironmentString(td,td->buffer2);
	execHiddenProcess(&spi);

	
	/*
	*Read the CGI output.
	*/
	DWORD nBytesRead;
	if(!setFilePointer(stdOutFile,0))
		ms_ReadFromFile(stdOutFile,td->buffer2,td->buffersize2,&nBytesRead);
	else
		td->buffer2[0]='\0';
	/*
	*Standard CGI can include an extra HTTP header so do not 
	*terminate with \r\n the default myServer header.
	*/
	DWORD headerSize=0;
	for(DWORD i=0;i<nBytesRead;i++)
	{
		if(td->buffer2[i]=='\r')
			if(td->buffer2[i+1]=='\n')
				if(td->buffer2[i+2]=='\r')
					if(td->buffer2[i+3]=='\n')
					{
						/*
						*The HTTP header ends with a \r\n\r\n sequence so 
						*determinate where it ends and set the header size
						*to i + 4.
						*/
						headerSize=i+4;
						break;
					}
	}
	sprintf(td->response.CONTENTS_DIM,"%u",nBytesRead-headerSize);
	buildHTTPResponseHeader(td->buffer,&td->response);

	/*
	*If there is an extra header, send lstrlen(td->buffer)-2 because the
	*last two characters are \r\n that terminating the HTTP header.
	*/
	if(headerSize)
		ms_send(s->socket,td->buffer,lstrlen(td->buffer)-2, 0);
	else
		ms_send(s->socket,td->buffer,lstrlen(td->buffer), 0);

	/*
	*In the buffer2 there are the CGI HTTP header and the 
	*contents of the page requested through the CGI.
	*If the client do an HEAD request send only the HTTP header.
	*/
	if(!lstrcmpi(td->request.CMD,"HEAD"))
		ms_send(s->socket,td->buffer2,headerSize, 0);
	else
		ms_send(s->socket,td->buffer2,nBytesRead, 0);

	
	/*
	*Close and delete the stdin and stdout files used by the CGI.
	*/
	ms_CloseFile(stdOutFile);
	ms_DeleteFile(stdOutFilePath);
	ms_CloseFile(stdInFile);
	ms_DeleteFile(stdInFilePath);

	/*
	*Restore security on the current thread.
	*/
	if(lserver->mustUseLogonOption())
		impersonateLogonUser(&td->hImpersonation);
		
	return 1;
}
/*
*Returns the MIME type passing its extension.
*/
BOOL getMIME(char *MIME,char *filename,char *dest,char *dest2)
{
	getFileExt(dest,filename);
	/*
	*Returns true if file is registered by a CGI.
	*/
	return lserver->mimeManager.getMIME(dest,MIME,dest2);
}
/*
*Map an URL to the machine file system.
*/
void getPath(char *filenamePath,char *filename,BOOL systemrequest)
{
	if(systemrequest)
	{
		sprintf(filenamePath,"%s/%s",lserver->getSystemPath(),filename);
	}
	else
	{
		sprintf(filenamePath,"%s/%s",lserver->getPath(),filename);
	}
}
/*
*Build the string that contain the CGI environment.
*/
void buildCGIEnvironmentString(httpThreadContext* td,char *cgiEnvString)
{
	cgiEnvString[0]='\0';
	/*
	*The Environment string is a null-terminated block of null-terminated strings.
	*Cause we use the function lstrcat we use the character \r for the \0 character
	*and at the end we change every \r in \0.
	*/
	lstrcat(cgiEnvString,"SERVER_SOFTWARE=myServer");
	lstrcat(cgiEnvString,versionOfSoftware);

	lstrcat(cgiEnvString,"\rSERVER_NAME=");
	lstrcat(cgiEnvString,lserver->getServerName());

	lstrcat(cgiEnvString,"\rQUERY_STRING=");
	lstrcat(cgiEnvString,td->request.URIOPTS);

	lstrcat(cgiEnvString,"\rGATEWAY_INTERFACE=CGI/1.1");
	
	lstrcat(cgiEnvString,"\rSERVER_PROTOCOL=HTTP/1.1");
	
	lstrcat(cgiEnvString,"\rSERVER_PORT=");
	char port[10];
	sprintf(port,"%u",lserver->port_HTTP);
	lstrcat(cgiEnvString,port);
    
	lstrcat(cgiEnvString,"\rREQUEST_METHOD=");
	lstrcat(cgiEnvString,td->request.CMD);

	lstrcat(cgiEnvString,"\rHTTP_USER_AGENT=");
	lstrcat(cgiEnvString,td->request.USER_AGENT);


	lstrcat(cgiEnvString,"\rHTTP_ACCEPT=");
	lstrcat(cgiEnvString,td->request.ACCEPT);
	
	lstrcat(cgiEnvString,"\rCONTENT_TYPE=");
	lstrcat(cgiEnvString,td->request.CONTENTS_TYPE);
	
	lstrcat(cgiEnvString,"\rCONTENT_LENGTH=");
	lstrcat(cgiEnvString,td->request.CONTENTS_DIM);

/*
	lstrcat(cgiEnvString,"\rPATH_INFO=");
	lstrcat(cgiEnvString,td->request.URI);

	lstrcat(cgiEnvString,"\rREMOTE_HOST=");
	lstrcat(cgiEnvString,td->request.HOST);

	lstrcat(cgiEnvString,"\rPATH_TRANSLATED=");
	lstrcat(cgiEnvString,td->request.URI);

	lstrcat(cgiEnvString,"\rSCRIPT_NAME=");
	lstrcat(cgiEnvString,td->request.URI);

	lstrcat(cgiEnvString,"\rREMOTE_ADDR=");
	lstrcat(cgiEnvString,td->request.HOST);

	lstrcat(cgiEnvString,"\rAUTH_TYPE=");
	lstrcat(cgiEnvString,td->request.HOST);

	lstrcat(cgiEnvString,"\rREMOTE_USER=");
	lstrcat(cgiEnvString,td->request.HOST);

	lstrcat(cgiEnvString,"\rREMOTE_IDENT=");
	lstrcat(cgiEnvString,td->request.HOST);
*/
	lstrcat(cgiEnvString,"\0\0");
	int max=lstrlen(cgiEnvString);
	for(int i=0;i<max;i++)
		if(cgiEnvString[i]=='\r')
			cgiEnvString[i]='\0';
}
/*
*Controls if the req string is a valid HTTP request header.
*Returns 0 if req is an invalid header, a non-zero value if is a valid header.
*nLinesptr is a value of the lines number in the HEADER.
*nCharsptr is a value of the characters number in the HEADER.
*/
DWORD validHTTPRequest(httpThreadContext* td,DWORD* nLinesptr,DWORD* nCharsptr)
{
	DWORD i;
	char *req=td->buffer;
	DWORD buffersize=td->buffersize;
	DWORD nLineChars;
	BOOL isValidCommand;
	isValidCommand=FALSE;
	nLineChars=0;
	DWORD nLines=0;
	DWORD maxTotChars=0;
	if(req==0)
		return 0;
	/*
	*Count the number of lines in the header.
	*/
	for(nLines=i=0;;i++)
	{
		if(req[i]=='\n')
		{
			if(req[i+2]=='\n')
			{
				maxTotChars=i+3;
				if(maxTotChars>buffersize)
				{
					isValidCommand=FALSE;
					break;				
				}
				isValidCommand=TRUE;
				break;
			}
			nLines++;
		}
		else
			nLineChars++;
		/*
		*We set a maximum theorical number of characters in a line to 1024.
		*If a line contains more than 1024 lines we consider the header invalid.
		*/
		if(nLineChars>1024)
			break;
	}

	/*
	*Set the output variables
	*/
	*nLinesptr=nLines;
	*nCharsptr=maxTotChars;
	/*
	*Return if is a valid request header
	*/
	return((isValidCommand)?1:0);
}