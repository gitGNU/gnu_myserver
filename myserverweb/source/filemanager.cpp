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
#include "..\stdafx.h"
#include "..\include\utility.h"
#include <string.h>
extern BOOL mustEndServer; 
static FILE* logFile=0;

/*
*Functions to manage a log file
*/
DWORD logFileWrite(char* str)
{
	return fprintf(logFile,"%s",str);
}
void setLogFile(FILE* nlg)
{
	if(logFile)
		fclose(logFile);
	logFile=nlg;
}
/*
*Return the size of the file in bytes
*/
void getFileSize(DWORD* fsd,FILE *f)
{
	static __int64 fs;
	fseek(f, 0, SEEK_END);
	fgetpos(f, &fs);
	fseek(f, 0, SEEK_SET);
	*fsd=(DWORD)fs;
}
/*
*Return the recursion of the path
*/
int getPathRecursionLevel(char* path)
{
	static char lpath[MAX_PATH];
	lstrcpy(lpath,path);
	int rec=0;
	char *token = strtok( lpath, "\\/" );
	do
	{
		if(lstrcmpi(token,".."))
			rec++;
		else
			rec--;
		token = strtok( NULL, "\\/" );
	}
	while( token != NULL );
	return rec;
}
/*
*Read data from a file to a buffer
*/
INT	readFromFile(MYSERVER_FILE_HANDLE f,char* buffer,DWORD buffersize,DWORD* nbr)
{
#ifdef WIN32
	SetFilePointer((HANDLE)f,0,0,SEEK_SET);
	ReadFile((HANDLE)f,buffer,buffersize,nbr,NULL);
	buffer[max(buffersize,*nbr)]='\0';
	/*
	*Return 1 if we don't have problem with the buffersize
	*/
	return (*nbr<buffersize)? 1 : 0 ;
#endif
}

/*
*Open(or create if not exists) a file
*/
MYSERVER_FILE_HANDLE openFile(char* filename,DWORD opt)
{
#ifdef WIN32
	SECURITY_ATTRIBUTES sa = {0};  
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;
	DWORD creationFlag=0;
	DWORD openFlag=0;
	DWORD attributeFlag=0;

	if(opt & MYSERVER_FILE_OPEN_ALWAYS)
		creationFlag|=OPEN_ALWAYS;
	if(opt & MYSERVER_FILE_OPEN_IFEXISTS)
		creationFlag|=OPEN_EXISTING;

	if(opt & MYSERVER_FILE_OPEN_READ)
		openFlag|=GENERIC_READ;
	if(opt & MYSERVER_FILE_OPEN_WRITE)
		openFlag|=GENERIC_WRITE;

	if(opt & MYSERVER_FILE_OPEN_TEMPORARY)
		openFlag|=FILE_ATTRIBUTE_TEMPORARY;
	if(opt & MYSERVER_FILE_OPEN_HIDDEN)
		openFlag|=FILE_ATTRIBUTE_HIDDEN;

	return CreateFile(filename, openFlag,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,creationFlag,attributeFlag, NULL);
#endif
}
/*
*Create a temporary file
*/
MYSERVER_FILE_HANDLE createTemporaryFile(char* filename)
{
#ifdef WIN32
	return openFile(filename,MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_WRITE|MYSERVER_FILE_OPEN_TEMPORARY|MYSERVER_FILE_OPEN_HIDDEN|MYSERVER_FILE_OPEN_ALWAYS);
#endif
}
/*
*Close an open file handle
*/
INT closeFile(MYSERVER_FILE_HANDLE fh)
{
#ifdef WIN32
	CloseHandle(fh);
#endif
	return 0;
}
/*
*Delete an existing file passing its path
*/
INT deleteFile(char *filename)
{
#ifdef WIN32
	DeleteFile(filename);
#endif
	return 0;
}
