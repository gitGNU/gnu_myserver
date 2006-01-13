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

#include "../stdafx.h"
#include "../include/cgi.h"
#include "../include/http_headers.h"
#include "../include/http.h"
#include "../include/http_constants.h"
#include "../include/server.h"
#include "../include/security.h"
#include "../include/mime_utils.h"
#include "../include/filemanager.h"
#include "../include/sockets.h"
#include "../include/utility.h"
#include "../include/mem_buff.h"
#include "../include/filters_chain.h"

#include <string>
#include <sstream>

extern "C" {
#ifdef WIN32
#include <direct.h>
#endif
#include <string.h>
}

#ifndef lstrcpy
#define lstrcpy strcpy
#endif
#ifndef lstrlen
#define lstrlen strlen
#endif
#ifndef lstrcpyn
#define lstrcpyn strncpy
#endif
#ifndef strcat
#define strcat strcat
#endif
#ifndef strnicmp
#define strnicmp strncmp
#endif

using namespace std;

/*!
 *By default use a timeout of 15 seconds on new processes.
 */
int Cgi::cgiTimeout = MYSERVER_SEC(15);

/*!
 *Run the standard CGI and send the result to the client.
 */
int Cgi::send(HttpThreadContext* td, ConnectionPtr s, const char* scriptpath, 
              const char *cgipath, int execute, int onlyHeader)
{
 	/*! Use this flag to check if the CGI executable is nph(Non Parsed Header).  */
	int nph = 0;
	ostringstream cmdLine;
	int yetoutputted=0;
  u_long nbw=0;
  FiltersChain chain;

	ostringstream outputDataPath;
	StartProcInfo spi;
	string moreArg;
	string tmpCgiPath;
	u_long nBytesRead;
	u_long headerSize;
	
	/*!
   *Standard CGI uses STDOUT to output the result and the STDIN 
   *to get other params like in a POST request.
   */
	File stdOutFile;
	File stdInFile;

	td->scriptPath.assign(scriptpath);
  
  {
    /*! Do not modify the text between " and ". */
    int x;
    int subString = cgipath[0] == '"';
    int len=strlen(cgipath);
    for(x=1;x<len;x++)
    {
      if(!subString && cgipath[x]==' ')
        break;
      if(cgipath[x]=='"')
        subString = !subString;
    }

    /*!
     *Save the cgi path and the possible arguments.
     *the (x<len) case is when additional arguments are specified. 
     *If the cgipath is enclosed between " and " do not consider them 
     *when splitting directory and file name.
     */
    if(x<len)
    {
      string tmpString(cgipath);
      int begin = tmpString[0]=='"' ? 1: 0;
      int end = tmpString[x]=='"' ? x: x-1;
      tmpCgiPath.assign(tmpString.substr(begin, end-1));
      moreArg.assign(tmpString.substr(x, len-1));  
    }
    else
    {
      int begin = (cgipath[0] == '"') ? 1 : 0;
      int end   = (cgipath[len] == '"') ? len-1 : len;
      tmpCgiPath.assign(&cgipath[begin], end-begin);
      moreArg.assign("");
    }
    File::splitPath(tmpCgiPath, td->cgiRoot, td->cgiFile);
    
    tmpCgiPath.assign(scriptpath);
    File::splitPath(tmpCgiPath, td->scriptDir, td->scriptFile);
  }

  chain.setProtocol((Http*)td->lhttp);
  chain.setProtocolData(td);
  chain.setStream(&(td->connection->socket));
  if(td->mime)
  {
    if(td->mime && lserver->getFiltersFactory()->chain(&chain, 
                                                 ((MimeManager::MimeRecord*)td->mime)->filters, 
                                                       &(td->connection->socket) , &nbw, 1))
      {
        ((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
        ((Vhost*)td->connection->host)->warningsLogWrite("Cgi: Error loading filters");
        ((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
        stdOutFile.closeFile();
        chain.clearAllFilters(); 
        return ((Http*)td->lhttp)->raiseHTTPError(e_500);
      }
  }
	
  if(execute)
  {
    const char *args=0;
    if(td->request.uriOpts.length())
      args = td->request.uriOpts.c_str();
    else if(td->pathInfo.length())
      args = &td->pathInfo[1];

    if(cgipath && strlen(cgipath))
      cmdLine << td->cgiRoot << "/"  << td->cgiFile <<  " " << moreArg  << " " << 
          td->scriptFile <<  (args ? args : "" ) ;
    else
      cmdLine << td->scriptDir << "/" << td->scriptFile <<  " " << moreArg <<  " "
              << (args ? args : "" );

    if(td->scriptFile.length()>4 && td->scriptFile[0]=='n'  && td->scriptFile[1]=='p'
       && td->scriptFile[2]=='h' && td->scriptFile[3]=='-' )
      nph=1; 
    else
      nph=0;
   
    if(cgipath && strlen(cgipath))
    {
      spi.cmd.assign(td->cgiRoot);
      spi.cmd.append("/");
      spi.cmd.append(td->cgiFile);
      spi.arg.assign(moreArg);
      spi.arg.append(" ");
      spi.arg.append(td->scriptFile);
      if(args)
      {
        spi.arg.append(" ");
        spi.arg.append(args);
      }
    }
    else
    {
      spi.cmd.assign(scriptpath);
      spi.arg.assign(moreArg);
      if(args)
      {
        spi.arg.append(" ");
        spi.arg.append(args);
      }
    }

	}
	else
	{
     /*! Check if the CGI executable exists. */
		if((!cgipath) || (!File::fileExists(tmpCgiPath.c_str())))
		{
			((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
      if(cgipath && strlen(cgipath))
      {
        string msg;
        msg.assign("Cgi: Cannot find the ");
        msg.append(cgipath);
        msg.append("executable");
        ((Vhost*)td->connection->host)->warningsLogWrite(msg.c_str());
			}
      else
      {
        ((Vhost*)td->connection->host)->warningsLogWrite(
                                          "Cgi: Executable file not specified");
      }
      ((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);		
      td->scriptPath.assign("");
      td->scriptFile.assign("");
      td->scriptDir.assign("");
      chain.clearAllFilters(); 
			return ((Http*)td->lhttp)->raiseHTTPError(e_500);
		}

    spi.arg.assign(moreArg);
    spi.arg.append(" ");
    spi.arg.append(td->scriptFile);		
    
    cmdLine << "\"" << td->cgiRoot << "/" << td->cgiFile << "\" " << moreArg 
                    << " " << td->scriptFile;
  
    spi.cmd.assign(td->cgiRoot);
    spi.cmd.append("/");
    spi.cmd.append(td->cgiFile);
    
    if(td->cgiFile.length()>4 && td->cgiFile[0]=='n'  && td->cgiFile[1]=='p'
                              && td->cgiFile[2]=='h' && td->cgiFile[3]=='-' )
      nph=1;
    else
      nph=0;
	}
  
	/*!
   *Use a temporary file to store CGI output.
   *Every thread has its own tmp file name(td->outputDataPath), 
   *so use this name for the file that is going to be
   *created because more threads can access more CGI at the same time.
   */
  outputDataPath << getdefaultwd(0,0) << "/stdOutFileCGI_" 
                 <<  (unsigned int)td->id;
  
  /*!
   *Open the stdout file for the new CGI process. 
   */
	if(stdOutFile.createTemporaryFile( outputDataPath.str().c_str() ))
	{
		((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((Vhost*)td->connection->host)->warningsLogWrite
                                        ("Cgi: Cannot create CGI stdout file" );
		((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
    chain.clearAllFilters(); 
		return ((Http*)td->lhttp)->raiseHTTPError(e_500);
	}

  /*! Open the stdin file for the new CGI process. */
  if(stdInFile.openFile(td->inputDataPath, 
                        FILE_OPEN_TEMPORARY|FILE_OPEN_READ|FILE_OPEN_ALWAYS))
  {
		((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((Vhost*)td->connection->host)->warningsLogWrite("Cgi: Cannot open CGI stdin file");
		((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
		stdOutFile.closeFile();
    chain.clearAllFilters(); 
		return ((Http*)td->lhttp)->raiseHTTPError(e_500);
  }
  
	/*!
   *Build the environment string used by the CGI started
   *by the execHiddenProcess(...) function.
   *Use the td->buffer2 to build the environment string.
   */
	(td->buffer2->getBuffer())[0]='\0';
	buildCGIEnvironmentString(td, td->buffer2->getBuffer());
  
	/*!
   *With this code we execute the CGI process.
   *Fill the StartProcInfo struct with the correct values and use it
   *to run the process.
   */
	spi.cmdLine = cmdLine.str();
	spi.cwd.assign(td->scriptDir);

	spi.stdError = stdOutFile.getHandle();
	spi.stdIn = stdInFile.getHandle();
	spi.stdOut = stdOutFile.getHandle();
	spi.envString=td->buffer2->getBuffer();
  
  /*! Execute the CGI process. */
  {
    Process cgiProc;
    if( cgiProc.execHiddenProcess(&spi, cgiTimeout) )
    {
      stdInFile.closeFile();
      stdOutFile.closeFile();
      ((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
      ((Vhost*)(td->connection->host))->warningsLogWrite
                                       ("Cgi: Error in the CGI execution");
      ((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
      chain.clearAllFilters(); 
      return ((Http*)td->lhttp)->raiseHTTPError(e_500);
    }
  }
  /*! Reset the buffer2 length counter. */
	td->buffer2->setLength(0);

	/*! Read the CGI output.  */
	nBytesRead=0;

  /*! Return an internal error if we cannot seek on the file. */
	if(stdOutFile.setFilePointer(0))
  {
    stdInFile.closeFile();
		stdOutFile.closeFile();
		((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((Vhost*)(td->connection->host))->warningsLogWrite
                               ("Cgi: Error setting file pointer");
		((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
    chain.clearAllFilters(); 
		return ((Http*)td->lhttp)->raiseHTTPError(e_500);
  }
  
  if(stdOutFile.readFromFile(td->buffer2->getBuffer(), 
                             td->buffer2->getRealLength()-1, &nBytesRead))
  {
    stdInFile.closeFile();
		stdOutFile.closeFile();
		((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((Vhost*)(td->connection->host))->warningsLogWrite
                               ("Cgi: Error reading from CGI std out file");
		((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
    chain.clearAllFilters(); 
		return ((Http*)td->lhttp)->raiseHTTPError(e_500);
  }
		
	(td->buffer2->getBuffer())[nBytesRead]='\0';
		
	if(nBytesRead==0)
	{
		((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((Vhost*)td->connection->host)->warningsLogWrite("Cgi: Error CGI zero bytes read" );
		((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
		((Http*)td->lhttp)->raiseHTTPError(e_500);
		yetoutputted=1;
	}
	/*! Standard CGI can include an extra HTTP header.  */
	headerSize=0;
  nbw=0;
	for(u_long i=0; i<nBytesRead; i++)
	{
		char *buff=td->buffer2->getBuffer();
		if( (buff[i]=='\r') && (buff[i+1]=='\n') 
        && (buff[i+2]=='\r') && (buff[i+3]=='\n') )
		{
			/*!
       *The HTTP header ends with a \r\n\r\n sequence so 
       *determinate where it ends and set the header size
       *to i + 4.
       */
			headerSize= i + 4 ;
			break;
		}
    else if((buff[i]=='\n') && (buff[i+1]=='\n'))
    {
      /*!
       *\n\n case.
       */
      headerSize= i + 2;
      break;
    }
		/*! If it is present Location: xxx in the header send a redirect to xxx.  */
		else if(!strncmp(&(td->buffer2->getBuffer())[i], "Location:", 9))
		{
      /*! If no other data was send, send the redirect. */
			if(!yetoutputted)
			{
        string nURL;
        u_long len = 0;

        while( (td->buffer2->getBuffer())[i + len + 9] != '\r' )
        {
            len++;
        }
        nURL.assign(&(td->buffer2->getBuffer()[i + 9]), len);
				((Http*)td->lhttp)->sendHTTPRedirect(nURL.c_str());
        /*! Store the new flag. */
        yetoutputted=1;

      }
		}
	}

	if(!yetoutputted)
	{
    u_long nbw2=0;
		if(!lstrcmpi(td->request.connection.c_str(), "Keep-Alive"))
			td->response.connection.assign("Keep-Alive");
		/*!
     *Do not send any other HTTP header if the CGI executable
     *has the nph-. form name.  
     */
		if(nph)
		{
			/*! Resetting the structure we send only the information gived by the CGI. */
			HttpHeaders::resetHTTPResponse(&(td->response));
      td->response.ver.assign(td->request.ver.c_str());
		}

    /*! If we have not to append the output send data directly to the client. */
		if(!td->appendOutputs)
		{
      ostringstream tmp;
      /*! Send the header.  */
			if(headerSize)
				HttpHeaders::buildHTTPResponseHeaderStruct(&td->response, td, 
                                                    td->buffer2->getBuffer());
      /*! Always specify the size of the HTTP contents.  */
			tmp << (u_int) (stdOutFile.getFileSize()-headerSize);
      td->response.contentLength.assign(tmp.str());
			HttpHeaders::buildHTTPResponseHeader(td->buffer->getBuffer(),
                                            &td->response);

			td->buffer->setLength((u_int)strlen(td->buffer->getBuffer()));

			if(s->socket.send(td->buffer->getBuffer(),
                        static_cast<int>(td->buffer->getLength()), 0)==SOCKET_ERROR)
      {
        stdInFile.closeFile();
        stdOutFile.closeFile();
        chain.clearAllFilters(); 
        /*! Remove the connection on sockets error. */
        return 0;
      }
      if(onlyHeader)
      {
        stdOutFile.closeFile();
        stdInFile.closeFile();
        chain.clearAllFilters(); 
        return 1;
      }

      /*! Send other remaining data in the buffer. */
			if(chain.write((td->buffer2->getBuffer() + headerSize), 
                        nBytesRead-headerSize, &nbw2))
      {
        stdOutFile.closeFile();
        stdInFile.closeFile();
        chain.clearAllFilters(); 
        /*! Remove the connection on sockets error. */
        return 0;       
      }
      nbw += nbw2;
		}
		else
    {
      if(onlyHeader)
      {
        stdOutFile.closeFile();
        stdInFile.closeFile();
        chain.clearAllFilters(); 
        return 1;
      }

      /*! Do not put the HTTP header if appending. */
			td->outputData.writeToFile(td->buffer2->getBuffer()+headerSize, 
                                 nBytesRead-headerSize, &nbw2);
      nbw += nbw2;
    }
    do
    {
      /*! Flush other data. */
      if(stdOutFile.readFromFile(td->buffer2->getBuffer(), 
                                 td->buffer2->getRealLength(), &nBytesRead))
      {
        stdOutFile.closeFile();
        stdInFile.closeFile();
        chain.clearAllFilters(); 
        /*! Remove the connection on sockets error. */
        return 0;      
      }
      if(nBytesRead)
      {

        if(!td->appendOutputs)
        {
          if(chain.write(td->buffer2->getBuffer(), nBytesRead, &nbw2))
          {
            stdOutFile.closeFile();
            stdInFile.closeFile();
            chain.clearAllFilters(); 
            /*! Remove the connection on sockets error. */
            return 0;      
          }
          nbw += nbw2;
        }
        else
        {
          if(td->outputData.writeToFile(td->buffer2->getBuffer(), 
                                        nBytesRead, &nbw2))
          {
            stdOutFile.closeFile();
            stdInFile.closeFile();
            chain.clearAllFilters(); 
            File::deleteFile(td->inputDataPath);
            /*! Remove the connection on sockets error. */
            return 0;      
          }
          nbw += nbw2;
        }
      }

		}while(nBytesRead);

	}
   /*! Update the Content-Length field for logging activity. */
  {
    ostringstream buffer;
    buffer << nbw;
    td->response.contentLength.assign(buffer.str());
  }
  chain.clearAllFilters(); 	
	
  /*! Close the stdin and stdout files used by the CGI.  */
	stdOutFile.closeFile();
	stdInFile.closeFile();
	
	/*! Delete the file only if it was created by the CGI module. */
	if(!td->inputData.getHandle())
	  File::deleteFile(td->inputDataPath.c_str());
  
  File::deleteFile(outputDataPath.str().c_str());
	return 1;  
}

/*!
 *Write the string that contain the CGI environment to cgiEnvString.
 *This function is used by other server side protocols too.
 */
void Cgi::buildCGIEnvironmentString(HttpThreadContext* td, char *cgi_env_string, 
                                    int processEnv)
{
	/*!
   *The Environment string is a null-terminated block of null-terminated strings.
   *For no problems with the function strcat we use the character \r for 
   *the \0 character and at the end we change every \r in \0.
   */
	MemBuf memCgi;
	char strTmp[32];

	memCgi.setExternalBuffer(cgi_env_string, td->buffer2->getRealLength());
	memCgi << "SERVER_SOFTWARE=MyServer " << versionOfSoftware;

#ifdef WIN32
	memCgi << " (WIN32)";
#else
#ifdef HOST_STR
	memCgi << " " << HOST_STR;
#else
	memCgi << " (Unknown)";
#endif
#endif
	/*! *Must use REDIRECT_STATUS for php and others.  */
	memCgi << end_str << "REDIRECT_STATUS=TRUE";
	
	memCgi << end_str << "SERVER_NAME=";
 	memCgi << lserver->getServerName();

	memCgi << end_str << "SERVER_SIGNATURE=";
	memCgi << "<address>MyServer ";
	memCgi << versionOfSoftware;
	memCgi << "</address>";

	memCgi << end_str << "SERVER_PROTOCOL=";
	memCgi << td->request.ver.c_str();	
	
  {
    MemBuf portBuffer;
    portBuffer.uintToStr( td->connection->getLocalPort());
    memCgi << end_str << "SERVER_PORT="<< portBuffer;
  }
	memCgi << end_str << "SERVER_ADMIN=";
	memCgi << lserver->getServerAdmin();

	memCgi << end_str << "REQUEST_METHOD=";
	memCgi << td->request.cmd.c_str();

	memCgi << end_str << "REQUEST_URI=";
	
 	memCgi << td->request.uri.c_str();

	memCgi << end_str << "QUERY_STRING=";
	memCgi << td->request.uriOpts.c_str();

	memCgi << end_str << "GATEWAY_INTERFACE=CGI/1.1";

	if(td->request.contentType.length())
	{
		memCgi << end_str << "CONTENT_TYPE=";
		memCgi << td->request.contentType.c_str();
	}

	if(td->request.contentLength.length())
	{
		memCgi << end_str << "CONTENT_LENGTH=";
		memCgi << td->request.contentLength.c_str();
	}
	else
	{
		u_long fs=0;
    ostringstream stream;
 
		if(td->inputData.getHandle())
			fs=td->inputData.getFileSize();

    stream << fs;

		memCgi << end_str << "CONTENT_LENGTH=" << stream.str().c_str();
	}

	if(td->request.cookie.length())
	{
		memCgi << end_str << "HTTP_COOKIE=";
		memCgi << td->request.cookie.c_str();
	}

	if(td->request.rangeByteBegin || td->request.rangeByteEnd)
	{
    ostringstream rangeBuffer;
		memCgi << end_str << "HTTP_RANGE=" << td->request.rangeType << "=" ;
    if(td->request.rangeByteBegin)
    {
      rangeBuffer << static_cast<int>(td->request.rangeByteBegin);
      memCgi << rangeBuffer.str();
    }
    memCgi << "-";
    if(td->request.rangeByteEnd)
    {
      rangeBuffer << td->request.rangeByteEnd;
      memCgi << rangeBuffer.str();
    }   

	}

	if(td->request.referer.length())
	{
		memCgi << end_str << "HTTP_REFERER=";
		memCgi << td->request.referer.c_str();
	}

	if(td->request.cacheControl.length())
	{
		memCgi << end_str << "HTTP_CACHE_CONTROL=";
		memCgi << td->request.cacheControl.c_str();
	}

	if(td->request.accept.length())
{
		memCgi << end_str << "HTTP_ACCEPT=";
		memCgi << td->request.accept.c_str();
	}

	if(td->cgiRoot.length())
	{
		memCgi << end_str << "CGI_ROOT=";
		memCgi << td->cgiRoot;
	}

	if(td->request.host.length())
	{
		memCgi << end_str << "HTTP_HOST=";
		memCgi << td->request.host.c_str();
	}

	if(td->connection->getIpAddr()[0])
	{
		memCgi << end_str << "REMOTE_ADDR=";
		memCgi << td->connection->getIpAddr();
	}

	if(td->connection->getPort())
	{
    MemBuf remotePortBuffer;
    remotePortBuffer.MemBuf::uintToStr(td->connection->getPort() );
	 	memCgi << end_str << "REMOTE_PORT=" << remotePortBuffer;
	}

	if(td->connection->getLogin()[0])
	{
    memCgi << end_str << "REMOTE_USER=";
		memCgi << td->connection->getLogin();
	}
	
	if(((Vhost*)(td->connection->host))->getProtocol()==PROTOCOL_HTTPS)
		memCgi << end_str << "SSL=ON";
	else
		memCgi << end_str << "SSL=OFF";

	if(td->request.connection.length())
	{
		memCgi << end_str << "HTTP_CONNECTION=";
		memCgi << td->request.connection.c_str();
	}

	if(td->request.auth.length())
	{
		memCgi << end_str << "AUTH_TYPE=";
		memCgi << td->request.auth.c_str();
	}

	if(td->request.userAgent.length())
	{
		memCgi << end_str << "HTTP_USER_AGENT=";
		memCgi << td->request.userAgent.c_str();
	}

	if(td->request.acceptEncoding.length())
	{
		memCgi << end_str << "HTTP_ACCEPT_ENCODING=";
		memCgi << td->request.acceptEncoding.c_str();
	}

	if(td->request.acceptLanguage.length())
	{
		memCgi << end_str << "HTTP_ACCEPT_LANGUAGE=";
    memCgi << td->request.acceptLanguage.c_str();
	}

	if(td->pathInfo.length())
	{
		memCgi << end_str << "PATH_INFO=";
    memCgi << td->pathInfo;
	  	
    memCgi << end_str << "PATH_TRANSLATED=";
		memCgi << td->pathTranslated;
	}
	else
  {
    memCgi << end_str << "PATH_TRANSLATED=";
		memCgi << td->filenamePath;
	}

 	memCgi << end_str << "SCRIPT_FILENAME=";
	memCgi << td->filenamePath;
	
	/*!
   *For the DOCUMENT_URI and SCRIPT_NAME copy the 
   *requested uri without the pathInfo.
   */
	memCgi << end_str << "SCRIPT_NAME=";
	memCgi << td->request.uri.c_str();

	memCgi << end_str << "SCRIPT_URL=";
	memCgi << td->request.uri.c_str();

	memCgi << end_str << "DATE_GMT=";
	getRFC822GMTTime(strTmp, HTTP_RESPONSE_DATE_DIM);
	memCgi << strTmp;

 	memCgi << end_str << "DATE_LOCAL=";
	getRFC822LocalTime(strTmp, HTTP_RESPONSE_DATE_DIM);
	memCgi << strTmp;

	memCgi << end_str << "DOCUMENT_ROOT=";
	memCgi << ((Vhost*)(td->connection->host))->getDocumentRoot();

	memCgi << end_str << "DOCUMENT_URI=";
	memCgi << td->request.uri.c_str();
	
	memCgi << end_str << "DOCUMENT_NAME=";
	memCgi << td->filenamePath;

	if(td->identity[0])
	{
  		memCgi << end_str << "REMOTE_IDENT=";
      memCgi << td->identity;
  }
#ifdef WIN32
	if(processEnv)
	{
    LPTSTR lpszVariable; 
		LPVOID lpvEnv; 
		lpvEnv = lserver->envString; 
		memCgi << end_str;
		for (lpszVariable = (LPTSTR) lpvEnv; *lpszVariable; lpszVariable++) 
		{ 
			if(((char*)lpszVariable)[0]  != '=' )
			{
				memCgi << (char*)lpszVariable << end_str;
			}
			while (*lpszVariable)*lpszVariable++;
		} 
	}
#endif

	memCgi << end_str << end_str  << end_str  << end_str  << end_str  ;
}

/*!
 *Set the CGI timeout for the new processes.
 */
void Cgi::setTimeout(int nt)
{
   cgiTimeout = nt;
}

/*!
 *Get the timeout value for CGI processes.
 */
int Cgi::getTimeout()
{
  return cgiTimeout;
}
