/*
*MyServer
*Copyright (C) 2005 The MyServer Team
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


#include "../include/http.h"
#include "../include/http_headers.h"
#include "../include/http_dir.h"


#undef min
#define min( a, b )( ( a < b ) ? a : b  )

extern "C" 
{
#ifdef WIN32
#include <direct.h>
#include <errno.h>
#endif

#ifdef NOT_WIN
#include <string.h>
#include <errno.h>
#endif
}

#include "../include/lfind.h"
#ifdef NOT_WIN
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif


/*!
 *Constructor for the class.
 */
HttpDir::HttpDir()
{

}

/*!
 *Destroy the object.
 */
HttpDir::~HttpDir()
{

}

/*!
 *Load the static elements.
 */
int HttpDir::load()
{
  return 0;
}

/*!
 *Unload the static elements.
 */
int HttpDir::unload()
{
  return 0;
}



/*!
 *Browse a directory printing its contents in an HTML file.
 */
int HttpDir::send(httpThreadContext* td, ConnectionPtr s, char* directory, 
                  char* /*cgi*/, int only_header)
{
	u_long nbw;
	int ret;
  int outputDataPathLen = getdefaultwdlen() + 20;
	myserver_finddata_t fd;
	char *filename = 0;
	int startchar=0;
	int nDirectories=0;
	int i;
	char fileTime[32];
	char* bufferloop;
  char* browseDirCSSpath;
	MYSERVER_FILE_HANDLE outputDataHandle = td->outputData.getHandle();
  /*!
   *Create a new file if the old is not valid.
   */
	if( outputDataHandle == 0 )
	{

    if(td->outputDataPath)
      delete [] td->outputDataPath;
    td->outputDataPath = new char[outputDataPathLen];
    if(td->outputDataPath==0)
      return ((http*)td->lhttp)->sendHTTPhardError500(td, s);
    
    getdefaultwd(td->outputDataPath, outputDataPathLen);
		sprintf(&(td->outputDataPath)[strlen(td->outputDataPath)], 
            "/stdOutFile_%u", (u_int) td->id);
		ret = td->outputData.openFile(td->outputDataPath, 
                     MYSERVER_FILE_CREATE_ALWAYS |MYSERVER_FILE_OPEN_READ |
                                  MYSERVER_FILE_OPEN_WRITE);
		if(ret)
		{
			/*! Return an internal server error.  */
			return ((http*)td->lhttp)->raiseHTTPError(td, s, e_500);
		}
	}

	for(i=0;td->request.URI[i];i++)
	{
		if(td->request.URI[i]=='/')
			nDirectories++;
	}
	for(startchar=0, i=0;td->request.URI[i];i++)
	{
		if(td->request.URI[i]=='/')
		{
			startchar++;
			if(startchar==nDirectories)
			{
				/*!
         *At the end of the loop set startchar to te real value.
         *startchar indicates the initial position in td->request.URI 
         *of the file path.
         */
				startchar=i+1;
				break;
			}
		}
	}
	td->buffer2->SetLength(0);
	*td->buffer2<<"<html>\r\n<head>\r\n<title>" ;
	*td->buffer2<< td->request.URI ;
	*td->buffer2<< "</title>\r\n</head>\r\n" ; 
	ret = td->outputData.writeToFile((char*)td->buffer2->GetBuffer(), 
                                    (u_long)td->buffer2->GetLength(), &nbw);
	if(ret)
	{
		td->outputData.closeFile();
		return ((http*)td->lhttp)->raiseHTTPError(td, s, e_500);/*Return an internal server error*/
	}
  browseDirCSSpath = ((http*)td->lhttp)->getBrowseDirCSSFile();

	/*! 
   *If it is defined a CSS file for the graphic layout of 
   *the browse directory insert it in the page.  
   */
	if(browseDirCSSpath != 0)
	{
		MYSERVER_FILE cssHandle;
		ret=cssHandle.openFile(browseDirCSSpath, MYSERVER_FILE_OPEN_IFEXISTS | 
                           MYSERVER_FILE_OPEN_READ);
		if(ret == 0)
		{
			u_long nbr;
			ret = cssHandle.readFromFile((char*)td->buffer->GetBuffer(), 
                                   td->buffer->GetRealLength(), &nbr);
			if(ret == 0)
			{
				td->buffer2->SetLength(0);
				*td->buffer2 << "<style>\r\n<!--\r\n" ;
				ret=td->outputData.writeToFile((char*)td->buffer2->GetBuffer(), 
                                       (u_long)td->buffer2->GetLength(), &nbw);
				if(ret)
				{
					td->outputData.closeFile();
					/* Return an internal server error.  */
					return ((http*)td->lhttp)->raiseHTTPError(td, s, e_500);
				}
				ret = td->outputData.writeToFile((char*)td->buffer->GetBuffer(),
                                          (u_long)nbr, &nbw);
				if(ret)
				{
					td->outputData.closeFile();
					/* Return an internal server error.  */
					return ((http*)td->lhttp)->raiseHTTPError(td, s, e_500);
				}

				td->buffer2->SetLength(0);
				*td->buffer2 << "-->\r\n</style>\r\n";
				ret=td->outputData.writeToFile((char*)td->buffer2->GetBuffer(),
                                       (u_long)td->buffer2->GetLength(), &nbw);
				if(ret)
				{
					td->outputData.closeFile();
					/* Return an internal server error.  */
					return ((http*)td->lhttp)->raiseHTTPError(td, s, e_500);
				}
			}
			cssHandle.closeFile();

		}
	}

#ifdef WIN32
  filename = new char[strlen(directory)+3];
  if(filename==0)
  {
    delete [] td->outputDataPath;
    td->outputDataPath = 0;
    return  ((http*)td->lhttp)->sendHTTPhardError500(td, s);
  }
	sprintf(filename, "%s/*", directory);
#endif
#ifdef NOT_WIN
  filename = new char[strlen(directory)+2];
  if(filename==0)
  {
    delete [] td->outputDataPath;
    td->outputDataPath=0;
    return  ((http*)td->lhttp)->sendHTTPhardError500(td, s);
  }
	sprintf(filename, "%s/", directory);
#endif
	td->buffer2->SetLength(0);
	*td->buffer2 << "\r\n<body>\r\n<h1> Contents of directory ";
	*td->buffer2 <<  &td->request.URI[startchar] ;
	*td->buffer2 << "</h1>\r\n<p>\r\n<hr />\r\n";
	ret = td->outputData.writeToFile((char*)td->buffer2->GetBuffer(), 
                                   (u_long)td->buffer2->GetLength(), &nbw);
	if(ret)
	{
		td->outputData.closeFile();
		/*! Return an internal server error.  */
		return ((http*)td->lhttp)->raiseHTTPError(td, s, e_500);
	}
	ret=fd.findfirst(filename);
  delete [] filename;
  filename = 0;
	if(ret==-1)
	{
		return ((http*)td->lhttp)->raiseHTTPError(td, s, e_404);
	}
	/*! 
   *With the current code we build the HTML TABLE to indicize the
   *files in the directory.
   */
	td->buffer2->SetLength(0);
	*td->buffer2 << "<table width=\"100\%\">\r\n<tr>\r\n<td><b>File</b></td>\r\n  \
                 <td><b>Last Modified</b></td>\r\n<td><b>Size</b></td>\r\n</tr>\r\n";
	ret = td->outputData.writeToFile((char*)td->buffer2->GetBuffer(), 
                                    (u_long)td->buffer2->GetLength(), &nbw);
	if(ret)
	{
		td->outputData.closeFile();
		/* Return an internal server error.  */
		return ((http*)td->lhttp)->raiseHTTPError(td, s, e_500);
	}

	do
	{	
		if(fd.name[0]=='.')
			continue;
		/*! Do not show the security file.  */
		if(!strcmp(fd.name, "security"))
			continue;
		td->buffer2->SetLength(0);
		*td->buffer2 << "<tr>\r\n<td><a href=\"";
		if(!td->request.uriEndsWithSlash)
		{
			*td->buffer2 << &td->request.URI[startchar];
			*td->buffer2 << "/" ;
		}
		*td->buffer2 << fd.name ;
		*td->buffer2 << "\">" ;
		*td->buffer2 << fd.name;
		*td->buffer2 << "</td>\r\n<td>";
	
		getRFC822GMTTime((time_t)fd.time_write, fileTime, 32);

		*td->buffer2 << fileTime ;
		*td->buffer2 << "</td>\r\n<td>";
		
		if(fd.attrib & FILE_ATTRIBUTE_DIRECTORY)
		{
			*td->buffer2 << "[directory]";
		}
		else
		{
      char fileSize[20];
			sprintf(fileSize, "%u bytes", (u_int) fd.size);
			*td->buffer2 << fileSize;
		}
		*td->buffer2 << "</td>\r\n</tr>\r\n";
		ret = td->outputData.writeToFile((char*)td->buffer2->GetBuffer(), 
                                     (u_long)td->buffer2->GetLength(), &nbw);
		if(ret)
		{
			fd.findclose();
			td->outputData.closeFile();
			/* Return an internal server error.  */
			return ((http*)td->lhttp)->raiseHTTPError(td, s, e_500);
		}
	}while(!fd.findnext());
	td->buffer2->SetLength(0);
	*td->buffer2 << "</table>\r\n<hr />\r\n<address>MyServer " << versionOfSoftware;
              
  if(td->request.HOST[0])
  {    
    char port_buff[6];
    *td->buffer2 << " on ";
    *td->buffer2 << td->request.HOST ;
    *td->buffer2 << " Port ";

    u_short port = td->connection->getLocalPort();
    sprintf(port_buff,"%u", port);
    *td->buffer2 << port_buff;
  }
	*td->buffer2 << "</address>\r\n</body>\r\n</html>\r\n";
	ret = td->outputData.writeToFile((char*)td->buffer2->GetBuffer(),
                                   (u_long)td->buffer2->GetLength(), &nbw);
	if(ret)
	{
		fd.findclose();
		td->outputData.closeFile();
		/* Return an internal server error.  */
		return ((http*)td->lhttp)->raiseHTTPError(td, s, e_500);
	}	
	fd.findclose();
  *td->buffer2 << end_str;
	/*! Changes the \ character in the / character.  */
	bufferloop=(char*)td->buffer2->GetBuffer();
	while(*bufferloop++)
		if(*bufferloop=='\\')
			*bufferloop='/';
	if(!lstrcmpi(td->request.CONNECTION, "Keep-Alive"))
		strcpy(td->response.CONNECTION, "Keep-Alive");
	sprintf(td->response.CONTENT_LENGTH, "%u", (u_int)td->outputData.getFileSize());

	/*! If we haven't to append the output build the HTTP header and send the data.  */
	if(!td->appendOutputs)
  {
		u_long nbr=0;
    int nbs=0;
		td->outputData.setFilePointer(0);
		HttpHeaders::buildHTTPResponseHeader((char*)td->buffer->GetBuffer(), 
                                          &(td->response));
		nbs=s->socket.send((char*)td->buffer->GetBuffer(), 
                       (u_long)strlen((char*)td->buffer->GetBuffer()), 0);
		if(nbs == SOCKET_ERROR)
		{
			/* Remove the connection.  */
			return 0;
		}	
    
    if(only_header)
      return 1;
    
    do
		{
			ret = td->outputData.readFromFile((char*)td->buffer->GetBuffer(), 
                                        td->buffer->GetRealLength(), &nbr);
			if(ret)
			{
				td->outputData.closeFile();
				/* Return an internal server error.  */
				return 0;
			}
			if(nbr)
				nbs=s->socket.send((char*)td->buffer->GetBuffer(), nbr, 0);
      else
        nbs = 0;
			if(nbs==SOCKET_ERROR)
			{
				/* Return an internal server error.  */
				return 0;
			}
		}while(nbr && nbs);
	}
  else
  {
    HttpHeaders::buildHTTPResponseHeader((char*)td->buffer->GetBuffer(), 
                                          &(td->response));
  }
	return 1;

}
