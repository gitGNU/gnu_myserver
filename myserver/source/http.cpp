/*
MyServer
Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007 The MyServer Team
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


#include "../include/http.h"
#include "../include/http_headers.h"
#include "../include/server.h"
#include "../include/security.h"
#include "../include/mime_utils.h"
#include "../include/cgi.h"
#include "../include/file.h"
#include "../include/files_utility.h"
#include "../include/clients_thread.h"
#include "../include/sockets.h"
#include "../include/wincgi.h"
#include "../include/fastcgi.h"
#include "../include/utility.h"
#include "../include/md5.h"
#include "../include/isapi.h"
#include "../include/stringutils.h"
#include "../include/securestr.h"

#include <string>
#include <ostream>

using namespace std;

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

#ifdef NOT_WIN
#include "../include/lfind.h"
#endif

/*! Store if the MSCGI library was loaded.  */
int Http::mscgiLoaded = 0;

/*! Allow the definition of a MIME file for host. */
int Http::allowVhostMime = 1;

/*! Path to the .css file used by directory browsing.  */
string Http::browseDirCSSpath;

/*! Threshold value to send data in gzip.  */
u_long Http::gzipThreshold = 0;

/*! Vector with default filenames.  */
vector<string> Http::defaultFilename;

/*! Is the HTTP protocol loaded?  */
int Http::initialized = 0;

/*! If not specified differently use a timeout of 15 seconds.  */
int Http::cgiTimeout = MYSERVER_SEC(15);

/*! Cache for security files. */
SecurityCache Http::secCache;

/*! Access the security cache safely. */
Mutex Http::secCacheMutex;

/*! Dynamic commands manager. */
DynHttpCommandManager Http::dynCmdManager;

/*! Dynamic managers. */
DynHttpManagerList Http::dynManagerList;

/*!
 *Build a response for an OPTIONS request.
 */
int Http::optionsHTTPRESOURCE(string& /*filename*/, int /*yetmapped*/)
{
	int ret;
	string time;
  try
  {
		HttpRequestHeader::Entry *connection = td.request.other.get("Connection");
    getRFC822GMTTime(time, HTTP_RESPONSE_DATE_DIM);
    td.buffer2->setLength(0);
    *td.buffer2 <<  "HTTP/1.1 200 OK\r\n";
    *td.buffer2 << "Date: " << time ;
    *td.buffer2 <<  "\r\nServer: MyServer "  << versionOfSoftware ;
    if(connection && connection->value->length())
      *td.buffer2 << "\r\nConnection:" << connection->value->c_str() ;
    *td.buffer2 <<"\r\nContent-Length: 0\r\nAccept-Ranges: bytes\r\n";
    *td.buffer2 << "Allow: OPTIONS, GET, POST, HEAD, DELETE, PUT";
    
    /*!
     *Check if the TRACE command is allowed on the virtual host.
     */
    if(allowHTTPTRACE())
      *td.buffer2 << ", TRACE\r\n\r\n";
    else
      *td.buffer2 << "\r\n\r\n";
    
    /*! Send the HTTP header. */
    ret = td.connection->socket->send(td.buffer2->getBuffer(), 
																			(u_long)td.buffer2->getLength(), 0);
    if( ret == SOCKET_ERROR )
    {
      return 0;
    }
    return 1;
  }
  catch(...)
  {
    return raiseHTTPError(500); 
  };
}

/*!
 *Handle the HTTP TRACE command.
 */
int Http::traceHTTPRESOURCE(string& /*filename*/, int /*yetmapped*/)
{
	int ret;
	char tmpStr[12];
	int contentLength = (int)td.nHeaderChars;
	string time;
  try
  {
    MemBuf tmp;
		HttpRequestHeader::Entry *connection;

    tmp.intToStr(contentLength, tmpStr, 12);
    getRFC822GMTTime(time, HTTP_RESPONSE_DATE_DIM);
    if(!allowHTTPTRACE())
      return raiseHTTPError(401);
    td.buffer2->setLength(0);
    *td.buffer2 << "HTTP/1.1 200 OK\r\n";
    *td.buffer2 << "Date: " << time ;
    *td.buffer2 << "\r\nServer: MyServer " << versionOfSoftware ;
		connection = td.request.other.get("Connection");
    if(connection && connection->value->length())
      *td.buffer2 << "\r\nConnection:" << connection->value->c_str();
    *td.buffer2 <<"\r\nContent-Length:" << tmp
								<< "\r\nContent-Type: message/http\r\n"
								<< "Accept-Ranges: bytes\r\n\r\n";
    
    /*! Send our HTTP header.  */
    ret = td.connection->socket->send(td.buffer2->getBuffer(), 
																			(u_long)td.buffer2->getLength(), 0);
    if( ret == SOCKET_ERROR )
    {
      return 0;
    }
    
    /*! Send the client request header as the HTTP body.  */
    ret = td.connection->socket->send(td.buffer->getBuffer(), 
																			contentLength, 0);
    if(ret == SOCKET_ERROR)
    {
      return 0;
    }
    return 1;
  }
  catch(...)
  {
    return raiseHTTPError(500); 
  };
}
  
/*!
 *Check if the host allows the HTTP TRACE command
 */
int Http::allowHTTPTRACE()
{
	int ret;
	/*! Check if the host allows HTTP trace. */
	ostringstream filename;
  char *httpTraceValue;
	XmlParser parser;
	
  filename << td.getVhostDir() << "/security" ;
	if(parser.open(filename.str().c_str()))
	{
		return 0;
	}
	httpTraceValue = parser.getAttr("HTTP", "TRACE");
	
  /*! 
   *If the returned value is equal to ON so the 
   *HTTP TRACE is active for this vhost.  
   *By default don't allow the trace.
   */
	if(httpTraceValue && !lstrcmpi(httpTraceValue, "ON"))
		ret = 1;
	else
		ret = 0;
	parser.close();
	return ret;
}

/*!
 *Get the timeout for the cgi.
 */
int Http::getCGItimeout()
{
  return cgiTimeout;
}

/*!
 *Main function to handle the HTTP PUT command.
 */
int Http::putHTTPRESOURCE(string& filename, int, int, 
													int yetmapped)
{
  u_long firstByte = td.request.rangeByteBegin; 
  int permissions = -1;
	int httpStatus = td.response.httpStatus;
	int keepalive = 0;
  int providedMask = 0;
  char authType[16];	
  SecurityToken st;
  string directory;
  string file;

  st.td = &td;
  st.authType = authType;
  st.authTypeLen = 16;
  try
  {
		HttpRequestHeader::Entry *connection = td.request.other.get("Connection");
    HttpHeaders::buildDefaultHTTPResponseHeader(&td.response);
    if(connection && !stringcmpi(connection->value->c_str(), "keep-alive"))
    {
      td.response.connection.assign("keep-alive");
      keepalive = 1;
    }

    FilesUtility::splitPath(td.filenamePath, directory, file);
    td.response.httpStatus = httpStatus;
    /*!
     *td.filenamePath is the file system mapped path while filename 
     *is the uri requested.
     *systemrequest is 1 if the file is in the system directory.
     *If filename is already mapped on the file system don't map it again.
     */
    if(yetmapped)
    {
      td.filenamePath.assign(filename);
    }
    else
    {
      int ret;
      /*! 
       *If the client tries to access files that aren't in the web directory 
       *send a 401 error.  
       */
      translateEscapeString(filename);
      if((filename[0] != '\0') && 
         (FilesUtility::getPathRecursionLevel(filename) < 1))
      {
        return raiseHTTPError(401);
      }
      ret = getPath(td.filenamePath, filename, 0);
      if(ret != 200)
        return raiseHTTPError(ret);
    }
    if(FilesUtility::isDirectory(td.filenamePath.c_str()))
    {
      directory.assign(td.filenamePath);
    }
    else
    {
      string fn;
      fn.assign(filename);
      FilesUtility::splitPath(td.filenamePath, directory, fn);
    }
    if(td.connection->protocolBuffer == 0)
    {
      td.connection->protocolBuffer=new HttpUserData;
      if(!td.connection->protocolBuffer)
      {
        return sendHTTPhardError500();
      }
      ((HttpUserData*)(td.connection->protocolBuffer))->reset();
    }

    if(td.request.auth.length())
    {
      st.user = td.connection->getLogin();
      st.password = td.connection->getPassword();
      st.directory = directory.c_str();
      st.sysdirectory = td.getVhostSys();
      st.filename = file.c_str();
      st.requiredPassword = 
				((HttpUserData*)td.connection->protocolBuffer)->requiredPassword;
      st.providedMask = &providedMask;
      secCacheMutex.lock();
      try
      {
        permissions=secCache.getPermissionMask(&st);
        secCacheMutex.unlock();  
      }
      catch(...)
      {
        secCacheMutex.unlock();
        throw;
      };
    }
    else/*! The default user is Guest with a null password. */
    {
      secCacheMutex.lock();
      try
      {
        st.user = "Guest";
        st.password = "";
        st.directory = directory.c_str();
        st.sysdirectory = td.getVhostSys();
        st.filename = file.c_str();
        st.requiredPassword = 0;
        st.providedMask = 0;
        permissions=secCache.getPermissionMask(&st);
        secCacheMutex.unlock();
      }
      catch(...)
      {
        secCacheMutex.unlock();
        throw;
      };
    }

    if(permissions == -1)
    {
      td.connection->host->warningsLogRequestAccess(td.id);
      td.connection->host->warningsLogWrite("Http: Error reading security file");
      td.connection->host->warningsLogTerminateAccess(td.id);
      return raiseHTTPError(500); 
    }
    /*! Check if we have to use digest for the current directory. */
    if(!lstrcmpi(authType, "Digest"))
    {
      if(!td.request.auth.compare("Digest"))
      {
        if(!((HttpUserData*)td.connection->protocolBuffer)->digestChecked)
          ((HttpUserData*)td.connection->protocolBuffer)->digest = 
						checkDigest();
        ((HttpUserData*)td.connection->protocolBuffer)->digestChecked = 1;
        if(((HttpUserData*)td.connection->protocolBuffer)->digest == 1)
        {
          td.connection->setPassword(
							 ((HttpUserData*)td.connection->protocolBuffer)->requiredPassword);
          permissions = providedMask;
        }
      }
      td.authScheme = HTTP_AUTH_SCHEME_DIGEST;
    }
    else/*! By default use the Basic authentication scheme. */
    {
      td.authScheme = HTTP_AUTH_SCHEME_BASIC;
    }	
    /*! If there are no permissions, use the Guest permissions. */
    if(td.request.auth.length() && (permissions == 0))
    {
      st.user = "Guest";
      st.password = "";
      st.directory = directory.c_str();
      st.sysdirectory = td.getVhostSys();
      st.filename = file.c_str();
      st.requiredPassword = 0;
      st.providedMask = 0;
      
      secCacheMutex.lock();
      try
      {
        permissions = secCache.getPermissionMask(&st);		
        secCacheMutex.unlock();
      }
      catch(...)
      {
        secCacheMutex.unlock();
        throw;
      };
    }
    if(permissions == -1)
    {
      td.connection->host->warningsLogRequestAccess(td.id);
      td.connection->host->warningsLogWrite(
																	 "Http: Error reading security file");
      td.connection->host->warningsLogTerminateAccess(td.id);
      return raiseHTTPError(500); 
    }
    if(!(permissions & MYSERVER_PERMISSION_WRITE))
    {
      return sendAuth();
    }
    if(FilesUtility::fileExists(td.filenamePath.c_str()))
    {
      /*! If the file exists update it. */
      File file;
      if(file.openFile(td.filenamePath.c_str(), File::MYSERVER_OPEN_IFEXISTS | 
                       File::MYSERVER_OPEN_WRITE))
      {
        /*! Return an internal server error. */
        return raiseHTTPError(500);
      }
      file.setFilePointer(firstByte);
      for(;;)
      {
        u_long nbr = 0, nbw = 0;
        if(td.inputData.readFromFile(td.buffer->getBuffer(), 
																		 td.buffer->getRealLength(), &nbr))
        {
          file.closeFile();
          /*! Return an internal server error.  */
          return raiseHTTPError(500);
        }
        if(nbr)
        {
          if(file.writeToFile(td.buffer->getBuffer(), nbr, &nbw))
          {
            file.closeFile();
            /*! Return an internal server error.  */
            return raiseHTTPError(500);
          }
        }
        else
          break;
        if(nbw != nbr)
        {
          file.closeFile();
          /*! Internal server error.  */
          return raiseHTTPError(500);
        }
      }
      file.closeFile();
      /*! Successful updated.  */
      raiseHTTPError(200);
      
      return keepalive;
    }
    else
    {
      /*!
       *If the file doesn't exist create it.
       */
      File file;
      if(file.openFile(td.filenamePath.c_str(), 
                       File::MYSERVER_CREATE_ALWAYS | 
											 File::MYSERVER_OPEN_WRITE))
      {
        /*! Internal server error. */
        return raiseHTTPError(500);
      }
      for(;;)
      {
        u_long nbr = 0, nbw = 0;
        if(td.inputData.readFromFile(td.buffer->getBuffer(), 
                                      td.buffer->getRealLength(), &nbr))
        {
          file.closeFile();
          return raiseHTTPError(500);
        }
        if(nbr)
        {
          if(file.writeToFile(td.buffer->getBuffer(), nbr, &nbw))
          {
            file.closeFile();
            return raiseHTTPError(500);
          }
        }
        else
          break;
        if( nbw != nbr )
        {
          file.closeFile();
          return raiseHTTPError(500);
        }
      }
      file.closeFile();
      /*! Successful created. */
      raiseHTTPError(201);
      return 1;
    }
	}
  catch(...)
  {
    return raiseHTTPError(500); 
  };
}

/*!
 *Delete the resource identified by filename.
 */
int Http::deleteHTTPRESOURCE(string& filename, int yetmapped)
{
  int permissions = -1;
  string directory;
	int httpStatus = td.response.httpStatus;
	int providedMask = 0;
	char authType[16];
  SecurityToken st;
  string file;
  try
  {
    HttpHeaders::buildDefaultHTTPResponseHeader(&td.response);
		HttpRequestHeader::Entry *connection = td.request.other.get("Connection");

    if(connection && !stringcmpi(connection->value->c_str(), "keep-alive"))
    {
      td.response.connection.assign( "keep-alive");
    }
    st.authType = authType;
    st.authTypeLen = 16;
    st.td=&td;
    FilesUtility::splitPath(td.filenamePath, directory, file);    
    td.response.httpStatus = httpStatus;
    /*!
     *td.filenamePath is the file system mapped path while filename 
     *is the uri requested.
     *systemrequest is 1 if the file is in the system directory.
     *If filename is already mapped on the file system don't map it again.
     */
    if(yetmapped)
    {
      td.filenamePath.assign(filename);
    }
    else
    {
      int ret;
      /*!
       *If the client tries to access files that aren't in the web directory 
       *send a HTTP 401 error page.
       */
      translateEscapeString(filename );
      if((filename[0] != '\0') && 
				 (FilesUtility::getPathRecursionLevel(filename)<1))
      {
        return raiseHTTPError(401);
      }
      ret = getPath(td.filenamePath, filename, 0);
      if(ret != 200)
        return raiseHTTPError(ret);
    }
    if(FilesUtility::isDirectory(td.filenamePath.c_str()))
    {
      directory.assign(td.filenamePath);
    }
    else
    {
      FilesUtility::splitPath(td.filenamePath, directory, file);
    }

    if(td.connection->protocolBuffer == 0)
    {
      td.connection->protocolBuffer = new HttpUserData;
      if(!td.connection->protocolBuffer)
      {
        return 0;
      }
      ((HttpUserData*)(td.connection->protocolBuffer))->reset();
    }

    if(td.request.auth.length())
    {
      st.user = td.connection->getLogin();
      st.password = td.connection->getPassword();
      st.directory = directory.c_str();
      st.sysdirectory = td.getVhostSys();
      st.filename = file.c_str();
      st.requiredPassword = 
				((HttpUserData*)td.connection->protocolBuffer)->requiredPassword;
      st.providedMask = &providedMask;
      secCacheMutex.lock();
      try
      {
        permissions = secCache.getPermissionMask(&st);
        secCacheMutex.unlock();
      }
      catch(...)
      {
        secCacheMutex.unlock();
        throw;
      };
    }
    else/*! The default user is Guest with a null password. */
    {
      st.user = "Guest";
      st.password = "";
      st.directory = directory.c_str();
      st.sysdirectory = td.getVhostSys();
      st.filename = file.c_str();
      st.requiredPassword = 0;
      st.providedMask = 0;
      secCacheMutex.lock();
      try
      {
        permissions = secCache.getPermissionMask(&st);
        secCacheMutex.unlock();
      }
      catch(...)
      {
        secCacheMutex.unlock();
        throw;
      };
    }
    if(permissions == -1)
    {
      td.connection->host->warningsLogRequestAccess(td.id);
      td.connection->host->warningsLogWrite(
																		 "Http: Error reading security file");
      td.connection->host->warningsLogTerminateAccess(td.id);
      return raiseHTTPError(500); 
    }
    /*! Check if we have to use digest for the current directory. */
    if(!lstrcmpi(authType, "Digest"))
    {
      if(!td.request.auth.compare("Digest"))
      {
        if(!((HttpUserData*)td.connection->protocolBuffer)->digestChecked)
          ((HttpUserData*)td.connection->protocolBuffer)->digest = 
						checkDigest();
        ((HttpUserData*)td.connection->protocolBuffer)->digestChecked = 1;
        if(((HttpUserData*)td.connection->protocolBuffer)->digest == 1)
        {
          td.connection->setPassword(
               ((HttpUserData*)td.connection->protocolBuffer)->requiredPassword);
					permissions = providedMask;
        }
      }
      td.authScheme = HTTP_AUTH_SCHEME_DIGEST;
    }
    /*! By default use the Basic authentication scheme. */
    else
    {
      td.authScheme = HTTP_AUTH_SCHEME_BASIC;
    }	
    /*! If there are no permissions, use the Guest permissions. */
    if(td.request.auth.length() && (permissions==0))
    {
      st.user = "Guest";
      st.password = "";
      st.directory = directory.c_str();
      st.sysdirectory = td.getVhostSys();
      st.filename = file.c_str();
      st.requiredPassword = 0;
      st.providedMask = 0;
      secCacheMutex.lock();
      try
      {
        permissions=secCache.getPermissionMask(&st);	
        secCacheMutex.unlock();
      }
      catch(...)
      {
        secCacheMutex.unlock();
        throw;
      };
    }
    if(permissions == -1)
    {
      td.connection->host->warningsLogRequestAccess(td.id);
      td.connection->host->warningsLogWrite(
                             "Http: Error reading security file");
      td.connection->host->warningsLogTerminateAccess(td.id);
      return raiseHTTPError(500);
    }
    if(!(permissions & MYSERVER_PERMISSION_DELETE))
	  {
      return sendAuth();
    }
    if(FilesUtility::fileExists(td.filenamePath))
	  {
      FilesUtility::deleteFile(td.filenamePath.c_str());
      /*! Successful deleted.  */
      return raiseHTTPError(202);
    }
    else
  	{
      /*! No content.  */
      return raiseHTTPError(204);
    }
  }
  catch(...)
  {
    return raiseHTTPError(500); 
  };

}

/*!
 *Check the Digest authorization
 */
u_long Http::checkDigest()
{
  Md5 md5;
	char A1[48];
	char A2[48];
	char response[48];
  char *uri;
	u_long digestCount;
  /*! Return 0 if the password is different.  */
	if(td.request.digestOpaque[0] && lstrcmp(td.request.digestOpaque, 
                      ((HttpUserData*)td.connection->protocolBuffer)->opaque))
		return 0;
  /*! If is not equal return 0.  */
	if(lstrcmp(td.request.digestRealm, 
						 ((HttpUserData*)td.connection->protocolBuffer)->realm))
		return 0;
	
	digestCount = hexToInt(td.request.digestNc);
	
	if(digestCount != ((HttpUserData*)td.connection->protocolBuffer)->nc + 1)
		return 0;
	else
		((HttpUserData*)td.connection->protocolBuffer)->nc++;
   
	md5.init();
	td.buffer2->setLength(0);
	*td.buffer2 << td.request.digestUsername << ":" << td.request.digestRealm 
			<< ":" << ((HttpUserData*)td.connection->protocolBuffer)->requiredPassword;

	md5.update((unsigned char const*)td.buffer2->getBuffer(), 
             (unsigned int)td.buffer2->getLength());
	md5.end(A1);
	
	md5.init();

	if(td.request.digestUri[0])
		uri = td.request.digestUri;
  else
    uri = (char*)td.request.uriOpts.c_str();

	td.buffer2->setLength(0);
	*td.buffer2 <<  td.request.cmd.c_str() <<  ":" << uri;
	md5.update((unsigned char const*)td.buffer2->getBuffer(), 
             (unsigned int)td.buffer2->getLength());
	md5.end( A2);
	
	md5.init();
	td.buffer2->setLength(0);
	*td.buffer2 << A1 << ":"  
							<< ((HttpUserData*)td.connection->protocolBuffer)->nonce << ":" 
							<< td.request.digestNc << ":"  << td.request.digestCnonce << ":" 
							<< td.request.digestQop  << ":" << A2;
	md5.update((unsigned char const*)td.buffer2->getBuffer(), 
             (unsigned int)td.buffer2->getLength());
	md5.end(response);	

	if(!lstrcmp(response, td.request.digestResponse))
		return 1;
	return 0;
}

/*!
 *Create the buffer.
 */
HttpUserData::HttpUserData()
{
  reset();
}

/*!
 *Destroy the buffer.
 */
HttpUserData::~HttpUserData()
{

}

/*!
 *Read primitive post data as it is sent by the client without apply any filter
 *in a contiguous manner, first read from the memory buffer and after from the
 *socket.
 *\param inBuffer Memory buffer with first part of POST data.
 *\param inBufferPos inBuffer size, this value is modified by the function, 
 *has to be 0 on first call.
 *\param inBufferSize inBuffer size.
 *\param inSocket Connection socket to read from.
 *\param outBuffer Out buffer where write.
 *\param outBufferSize outBuffer size.
 *\param nbr Number of bytes read.
 *\param timeout Timeout value to use on the socket.
 *\return Return 0 on success.
 */
int Http::readContiguousPrimitivePostData(char* inBuffer,
																					u_long *inBufferPos,
																					u_long inBufferSize,
																					Socket *inSocket,
																					char* outBuffer,
																					u_long outBufferSize,
																					u_long* nbr,
																					u_long timeout)
{
	int ret;

	*nbr = 0;
	if(inBufferSize - *inBufferPos)
	{
		*nbr = min(outBufferSize, inBufferSize - *inBufferPos);
		memcpy(outBuffer, inBuffer + *inBufferPos, *nbr);
		*inBufferPos += *nbr;
	}

	/* 
	 * No other space in the out buffer, return from the function with success.  
	 */
	if(outBufferSize == *nbr)
		return 0;

	if(!inSocket->bytesToRead())
		return 0;

	ret = inSocket->recv(outBuffer + *nbr,  outBufferSize - *nbr, timeout);

	if(ret == -1)
		return -1;

	*nbr += ret;
	return 0;
}

/*!
 *Read post data using the chunked transfer encoding.
 *This function uses the same arguments of readContiguousPrimitivePostData with
 *the additional destination file.
 *\param inBuffer Memory buffer with first part of POST data.
 *\param inBufferPos inBuffer size, this value is modified by the function, 
 *has to be 0 on first call.
 *\param inBufferSize inBuffer size.
 *\param inSocket Connection socket to read from.
 *\param outBuffer Out buffer where write.
 *\param outBufferSize outBuffer size.
 *\param outNbr Number of bytes read.
 *\param timeout Timeout value to use on the socket.
 *\return Return 0 on success.
 *\return -1 on internal error.
 *\return Any other value is the HTTP error code.
 */
int Http::readChunkedPostData(char* inBuffer,
															u_long *inBufferPos,
															u_long inBufferSize,
															Socket *inSocket,
															char* outBuffer,
															u_long outBufferSize,
															u_long* outNbr,
															u_long timeout,
															File* out)
{
	u_long nbr;
	*outNbr = 0;

	for(;;)
	{
		u_long chunkNbr;
		u_long dataToRead;
		u_long nbw;
		u_long bufferlen;
		char buffer[20];
		char c;
		bufferlen = 0;
		buffer[0] = '\0';
		for(;;)
		{
			if(readContiguousPrimitivePostData(inBuffer,
																				 inBufferPos,
																				 inBufferSize,
																				 inSocket,
																				 &c,
																				 1,
																				 &nbr,
																				 timeout))
				return -1;

			if(nbr != 1)
				return -1;

			if((c != '\r') && (bufferlen < 19))
			{
				buffer[bufferlen++] = c;
				buffer[bufferlen] = '\0';
			}
			else
				break;
		}

		/* Read the \n character too. */
		if(readContiguousPrimitivePostData(inBuffer,
																			 inBufferPos,
																			 inBufferSize,
																			 inSocket,
																			 &c,
																			 1,
																			 &nbr,
																			 timeout))
			 return -1;

		dataToRead = (u_long)hexToInt(buffer);

		/*! The last chunk length is 0.  */
		if(dataToRead == 0)
			break;

		chunkNbr = 0;

		while(chunkNbr < dataToRead)
		{
			u_long rs = min(outBufferSize , dataToRead - chunkNbr);

			if(readContiguousPrimitivePostData(inBuffer,
																				 inBufferPos,
																				 inBufferSize,
																				 inSocket,
																				 outBuffer,
																				 rs,
																				 &nbr,
																				 timeout))
			{
				return -1;
			}

			if(nbr == 0)
				return -1;

			chunkNbr += nbr;

			if(out->writeToFile(outBuffer, nbr, &nbw))
			{
				return -1;
			}
				 
			if(nbw != nbr)
				return -1;

			*outNbr += nbw;

			/* Read final chunk \r\n.  */
			if(readContiguousPrimitivePostData(inBuffer,
																				 inBufferPos,
																				 inBufferSize,
																				 inSocket,
																				 outBuffer,
																				 2,
																				 &nbr,
																				 timeout))
			{
				return -1;
			}

		}
			 
	}
	return 0;
}

/*!
 *Read POST data from the active connection.
 *\param td The Active thread context.
 *\param retcmd The protocol exit code if the response is yet processed.
 *\return Return 0 on success and if the response was not sent to the client.
 *\return Any other value is an error.
 */
int Http::readPostData(HttpThreadContext* td, int* retcmd)
{
	int retvalue = -1;
	int contentLength = -1;

	u_long nbw = 0;
	u_long bufferDataSize = 0;


	u_long timeout = MYSERVER_SEC(10);
	u_long inPos = 0;
	u_long nbr;
	u_long length;
	HttpRequestHeader::Entry *connection = td->request.other.get("Connection");

	HttpRequestHeader::Entry *contentType = 
		td->request.other.get("Content-Type");

	HttpRequestHeader::Entry *encoding = 
		td->request.other.get("Transfer-Encoding");
	
	/* Specify a type if it not specified by the client.  */
	if(contentType == 0)
	{
		contentType = new HttpRequestHeader::Entry();
		contentType->name->assign("Content-Type");
		contentType->value->assign("application/x-www-form-urlencoded");
	}
	else if(contentType->value->length() == 0)
	{
		contentType->value->assign("application/x-www-form-urlencoded");
	}
	td->request.uriOptsPtr = &(td->buffer->getBuffer())[td->nHeaderChars];
	td->buffer->getBuffer()[td->nBytesToRead < td->buffer->getRealLength() - 1 
									 ? td->nBytesToRead : td->buffer->getRealLength()-1] = '\0';

	if(td->request.contentLength.length())
	{
		contentLength = atoi(td->request.contentLength.c_str());
		if(contentLength < 0)
		{
			retvalue = raiseHTTPError(400);
			*retcmd = retvalue ? ClientsThread::KEEP_CONNECTION 
				: ClientsThread::DELETE_CONNECTION;
			return 1;
		}
	}

	/*!
	 *If the connection is Keep-Alive be sure that the client specify the
	 *HTTP CONTENT-LENGTH field.
	 *If a CONTENT-ENCODING is specified the CONTENT-LENGTH is not 
	 *always needed.
	 */
	if(!contentLength && connection
		 && !stringcmpi(connection->value->c_str(), "keep-alive"))
	{
		HttpRequestHeader::Entry *content = 
			td->request.other.get("Content-Encoding");
		
		if(content && (content->value->length() == '\0')
					 && (td->request.contentLength.length() == 0))
		{
			retvalue = raiseHTTPError(400);
			*retcmd = retvalue ? ClientsThread::KEEP_CONNECTION 
				: ClientsThread::DELETE_CONNECTION;
			return 1;
		}
	}

  /*!
	 *Create the file that contains the posted data.
	 *This data is the stdin file in the CGI.
	 */
	if(td->inputData.openFile(td->inputDataPath, File::MYSERVER_CREATE_ALWAYS | 
														File::MYSERVER_OPEN_READ | 
														File::MYSERVER_OPEN_WRITE))
	{
		*retcmd = ClientsThread::DELETE_CONNECTION;
		retvalue = raiseHTTPError(500);
		return 1;
	}

	length = contentLength;

	bufferDataSize = (td->nBytesToRead < td->buffer->getRealLength() - 1 
										? td->nBytesToRead 
										: td->buffer->getRealLength() - 1 ) - td->nHeaderChars;

	/* If it is specified a transfer encoding read data using it.  */
	if(encoding)
	{
		if(!encoding->value->compare("chunked"))
		{

			int ret = readChunkedPostData(td->request.uriOptsPtr,
																		&inPos,
																		bufferDataSize,
																		td->connection->socket,
																		td->buffer2->getBuffer(), 
																		td->buffer2->getRealLength() - 1,
																		&nbr,
																		timeout,
																		&(td->inputData));
			if(ret == -1)				
			{
				td->inputDataPath.assign("");
				td->outputDataPath.assign("");
				td->inputData.closeFile();
				*retcmd = ClientsThread::DELETE_CONNECTION;
				return 1;
			}
			else if(ret)
			{
				retvalue = raiseHTTPError(ret);
				*retcmd = retvalue ? ClientsThread::KEEP_CONNECTION 
					: ClientsThread::DELETE_CONNECTION;
				return 1;
			}

		}
		else
		{
			*retcmd = ClientsThread::DELETE_CONNECTION;
			retvalue = raiseHTTPError(501);
			return 1;
		}
	}
	/* If it is not specified an encoding, read the data as it is.  */
	else for(;;)
	{
        
		if(readContiguousPrimitivePostData(td->request.uriOptsPtr,
																			 &inPos,
																			 bufferDataSize,
																			 td->connection->socket,
																			 td->buffer2->getBuffer(), 
																			 td->buffer2->getRealLength() - 1,
																			 &nbr,
																			 timeout))
		{
			td->inputData.closeFile();
			FilesUtility::deleteFile(td->inputDataPath);
			retvalue = raiseHTTPError(400);
			*retcmd = retvalue ? ClientsThread::KEEP_CONNECTION 
				: ClientsThread::DELETE_CONNECTION;
			return 1;
		}

		if(nbr <= length)
			length -= nbr;
		else
		{
			td->inputData.closeFile();
			FilesUtility::deleteFile(td->inputDataPath);
			retvalue = raiseHTTPError(400);
			*retcmd = retvalue ? ClientsThread::KEEP_CONNECTION 
				: ClientsThread::DELETE_CONNECTION;
			return 1;
		}

		td->buffer2->getBuffer()[nbr] = '\0';

		if(nbr && td->inputData.writeToFile(td->buffer2->getBuffer(), nbr, &nbw))
		{
			td->inputDataPath.assign("");
			td->outputDataPath.assign("");
			td->inputData.closeFile();
			*retcmd = ClientsThread::DELETE_CONNECTION;
			return 1;
		}

		if(!length)
			break;

	}

	td->inputData.setFilePointer(0);
	return 0;
}

/*!
 *Reset the structure.
 */
void HttpUserData::reset()
{
	realm[0] = '\0';
	opaque[0] = '\0';
	nonce[0] = '\0';
	cnonce[0] = '\0';
	digestChecked = 0;
	requiredPassword[0] = '\0';
	nc = 0;
	digest = 0;
}

/*!
 *Main function to send a resource to a client.
 */
int Http::sendHTTPResource(string& uri, int systemrequest, int onlyHeader, 
													 int yetmapped)
{
	/*!
   *With this code we manage a request of a file or a directory or anything 
   *that we must send over the HTTP.
   */
	string filename;
  int permissions;
  int providedMask;
  string dirscan;
	int filenamePathLen;
  string data;
	int mimecmd;
	time_t lastMT;
  int ret;
  char authType[16];
  string tmpTime;
  SecurityToken st;
  string directory;
  string file;
  try
  {
		HttpRequestHeader::Entry *connection = td.request.other.get("Connection");

    st.authType = authType;
    st.authTypeLen = 16;
    st.td = &td;
    filename.assign(uri);
    td.buffer->setLength(0);

    if(connection && !stringcmpi(connection->value->c_str(), "keep-alive"))
    {
      td.response.connection.assign("keep-alive");
    }

    /*!
     *td.filenamePath is the file system mapped path while filename 
     *is the uri requested. systemrequest is 1 if the file is in 
     *the system directory.
     *If filename is already mapped on the file system don't map it again.
     */
    if(yetmapped)
    {
      td.filenamePath.assign(filename);
    }
    else
    {
      int ret;
      /*!
       *If the client tries to access files that aren't in the 
       *web directory send a 401 error.
       */
      translateEscapeString(filename);
      if(filename.length() && (filename[0] != '\0')&&
         (FilesUtility::getPathRecursionLevel(filename) < 1))
      {
        return raiseHTTPError(401);
      }
      /*! getPath will alloc the buffer for filenamePath. */
      ret = getPath(td.filenamePath, filename.c_str(), systemrequest);
      if(ret != 200)
        return raiseHTTPError(ret);
    }

    /*! By default allows only few actions. */
    permissions = MYSERVER_PERMISSION_READ | MYSERVER_PERMISSION_BROWSE ;

    if(!systemrequest)
    {
      if(FilesUtility::isLink(td.filenamePath.c_str())) 
      {
        const char *perm = td.connection->host->getHashedData("FOLLOW_LINKS");
        if(!perm || strcmpi(perm, "YES"))
          return raiseHTTPError(401);
      }

      if(FilesUtility::isDirectory(td.filenamePath.c_str()))
      {
				directory.assign(td.filenamePath);
      }
      else
      {
        FilesUtility::splitPath(td.filenamePath, directory, file);
      }

      if(td.connection->protocolBuffer == 0)
      {
        td.connection->protocolBuffer = new HttpUserData;
        if(!td.connection->protocolBuffer)
        {
          return sendHTTPhardError500();
        }
        ((HttpUserData*)td.connection->protocolBuffer)->reset();
      }
      providedMask = 0;
      if(td.request.auth.length())
      {
        st.user = td.connection->getLogin();
        st.password = td.connection->getPassword();
        st.directory = directory.c_str();
        st.sysdirectory = td.getVhostSys();
        st.filename = file.c_str();
        st.requiredPassword = 
					((HttpUserData*)td.connection->protocolBuffer)->requiredPassword;
        st.providedMask = &providedMask;
        secCacheMutex.lock();
        try
        {
          permissions = secCache.getPermissionMask(&st);
          secCacheMutex.unlock();
        }
        catch(...)
        {
          secCacheMutex.unlock();
          throw;
        };
      }
      else/*! The default user is Guest with a null password. */
      {
        st.user = "Guest";
        st.password = "";
        st.directory = directory.c_str();
        st.sysdirectory = td.getVhostSys();
        st.filename = file.c_str();
        st.requiredPassword = 0;
        st.providedMask = 0;
        secCacheMutex.lock();
        try
        {
          permissions = secCache.getPermissionMask(&st);
          secCacheMutex.unlock();
        }
        catch(...)
        {
          secCacheMutex.unlock();
          throw;
        };
      }	

      /*! Check if we have to use digest for the current directory. */
      if(!lstrcmpi(authType, "Digest"))
		  {
        if(!td.request.auth.compare("Digest"))
			  {
          if(!((HttpUserData*)td.connection->protocolBuffer)->digestChecked)
            ((HttpUserData*)td.connection->protocolBuffer)->digest = 
							checkDigest();

          ((HttpUserData*)td.connection->protocolBuffer)->digestChecked = 1;

          if(((HttpUserData*)td.connection->protocolBuffer)->digest == 1)
          {
            td.connection->setPassword(
               ((HttpUserData*)td.connection->protocolBuffer)->requiredPassword);
            permissions = providedMask;
          }
        }
        td.authScheme = HTTP_AUTH_SCHEME_DIGEST;
      }
      else/*! By default use the Basic authentication scheme.  */
      {
        td.authScheme = HTTP_AUTH_SCHEME_BASIC;
      }	
 
     /*! If there are no permissions, use the Guest permissions.  */
      if(td.request.auth.length() && (permissions == 0))
      {
        st.user = "Guest";
        st.password = "";
        st.directory = directory.c_str();
        st.sysdirectory = td.getVhostSys();
        st.filename = file.c_str();
        st.requiredPassword = 0;
        st.providedMask = 0;
        secCacheMutex.lock();
        try
        {
          permissions = secCache.getPermissionMask(&st);
          secCacheMutex.unlock();
        }
        catch(...)
        {
          secCacheMutex.unlock();
          throw;
        };
      }
    }

    if(permissions == -1)
    {
      td.connection->host->warningsLogRequestAccess(td.id);
      td.connection->host->warningsLogWrite(
                               "Http: Error reading security file");
      td.connection->host->warningsLogTerminateAccess(td.id);
      return raiseHTTPError(500); 
    }


    /*! If a throttling rate was specifed use it. */
    if(st.throttlingRate != -1)
      td.connection->socket->setThrottling(st.throttlingRate);


    /*!
     *Get the PATH_INFO value.
     *Use dirscan as a buffer for put temporary directory scan.
     *When an '/' character is present check if the path up to '/' character
     *is a file. If it is a file send the rest of the uri as PATH_INFO.
     */
    td.pathInfo.assign("");
    td.pathTranslated.assign("");
    filenamePathLen = (int)td.filenamePath.length();
    dirscan.assign("");
    for(int i = 0, len = 0; i < filenamePathLen ; i++)
	  {
      /*!
       *http://host/pathtofile/filetosend.php/PATH_INFO_VALUE?QUERY_INFO_VALUE
       *When a request has this form send the file filetosend.php with the
       *environment string PATH_INFO equals to PATH_INFO_VALUE and QUERY_INFO
       *to QUERY_INFO_VALUE.
       *
       *If there is the '/' character check if dirscan is a file.
       */
      if(i && (td.filenamePath[i] == '/'))
      {
        /*!
         *If the token is a file.
         */
        if(!FilesUtility::isDirectory(dirscan.c_str()))
        {
          td.pathInfo.assign((char*) & (td.filenamePath[i]));
          td.filenamePath.assign(dirscan);
          break;
        }
      }

      if(len + 1 < filenamePathLen)
      {
        char db[2];
        db[0] = (td.filenamePath)[i];
        db[1] = '\0';
        dirscan.append(db);
      }
    }

    /*!
     *If there is a PATH_INFO value the get the PATH_TRANSLATED too.
     *PATH_TRANSLATED is the local filesystem mapped version of PATH_INFO.
     */
    if(td.pathInfo.length() > 1)
    {
      int ret;
      /*!
       *Start from the second character because the first is a
       *slash character.  
       */
      ret = getPath(td.pathTranslated, &((td.pathInfo.c_str())[1]), 0);
      if(ret != 200)
        return raiseHTTPError(ret);
      FilesUtility::completePath(td.pathTranslated);
    }
    else
    {
      td.pathTranslated.assign("");
    }
    FilesUtility::completePath(td.filenamePath);

    /*!
     *If there are not any extension then we do one of this in order:
     *1)We send the default files in the directory in order.
     *2)We send the directory content.
     *3)We send an error.
     */
    if(FilesUtility::isDirectory(td.filenamePath.c_str()))
    {
      int i;
      if(!(permissions & MYSERVER_PERMISSION_BROWSE))
      {
        return sendAuth();
      }
      for(i = 0;; i++)
      {
        const char *defaultFileNamePath = getDefaultFilenamePath(i);
        ostringstream defaultFileName;
        defaultFileName.clear();
        if(defaultFileNamePath)
        {
          defaultFileName << td.filenamePath << "/" << defaultFileNamePath;
        }
        else
        {
          break;
        }

        if(FilesUtility::fileExists(defaultFileName.str().c_str()))
        { 
          ostringstream nUrl;

          if(td.request.uriEndsWithSlash)
          {
            nUrl << defaultFileNamePath;
          }
          else
          {
						u_long lastSlashOffset = uri.length();
            while(lastSlashOffset && uri[lastSlashOffset] != '/')
              --lastSlashOffset;
						
            nUrl << &(uri.c_str()[lastSlashOffset < uri.length() ? 
																	lastSlashOffset + 1 : 0]) 
								 << "/" << defaultFileNamePath;
          }

					if(td.pathInfo.length())
						nUrl << "/" << td.pathInfo;


					if(td.request.uriOpts.length())
						nUrl << "?" << td.request.uriOpts;

          /*! Send a redirect to the new location.  */
          if(sendHTTPRedirect(nUrl.str().c_str()))
            ret = 1;
          else
            ret = 0;
          return ret;
        }
      }
      return httpDir.send(&td, td.connection, td.filenamePath.c_str(), 0, 
													onlyHeader);
    }
    
    if(!FilesUtility::fileExists(td.filenamePath.c_str()))
      return raiseHTTPError(404);

    /*!
     *getMIME returns the type of command registered by the extension.
     */
    data.assign("");
    {
      td.response.contentType[0] = '\0';
      td.mime = getMIME(td.filenamePath);
      /*! Set the default content type, this can be changed later. */
      if(td.mime)
      {
        td.response.contentType.assign(td.mime->mimeType);
        mimecmd = td.mime->command;
        data.assign(td.mime->cgiManager);
      }
      else
      {
        td.response.contentType.assign("text/html");
        mimecmd = CGI_CMD_SEND;
        data.assign("");
      }
    }

    if(td.mime && 
       !td.mime->headerChecker.isAllowed(&(td.request)))
    {
      return sendAuth();
    }

    if(mimecmd == CGI_CMD_RUNCGI)
    {
      int allowCgi = 1;
	    const char *dataH = td.connection->host->getHashedData("ALLOW_CGI");
	    if(dataH)
	    {
        if(!strcmpi(dataH, "YES"))
          allowCgi = 1;
        else
          allowCgi = 0;      
	    }

      if(!allowCgi || !(permissions & MYSERVER_PERMISSION_EXECUTE))
      {
        return sendAuth();
      }
      ret = cgi.send(&td, td.connection, td.filenamePath.c_str(), 
										 data.c_str(), 0,  onlyHeader);
      return ret;
    }
    else if(mimecmd == CGI_CMD_EXECUTE )
    {
      int allowCgi = 1;
	    const char *dataH = td.connection->host->getHashedData("ALLOW_CGI");
	    if(dataH)
	    {
        if(!strcmpi(dataH, "YES"))
          allowCgi = 1;
        else
          allowCgi = 0;      
	    }

      if(!allowCgi || !(permissions & MYSERVER_PERMISSION_EXECUTE))
		  {
        return sendAuth();
      }
      ret = cgi.send(&td, td.connection, td.filenamePath.c_str(), 
											data.c_str(), 1, onlyHeader);
      return ret;
    }
    else if(mimecmd == CGI_CMD_RUNISAPI)
    {
      int allowIsapi = 1;
	    const char *dataH = td.connection->host->getHashedData("ALLOW_ISAPI");
	    if(dataH)
	    {
        if(!strcmpi(dataH, "YES"))
          allowIsapi = 1;
        else
          allowIsapi = 0;      
	    }
      if(!allowIsapi || !(permissions & MYSERVER_PERMISSION_EXECUTE))
      {
        return sendAuth();
      }
      ret = isapi.send(&td, td.connection, td.filenamePath.c_str(), 
											 data.c_str(), 0, onlyHeader);
      return ret;

    }
    else if(mimecmd == CGI_CMD_EXECUTEISAPI)
    {
      if(!(permissions & MYSERVER_PERMISSION_EXECUTE))
      {
        return sendAuth();
      }
      ret = isapi.send(&td, td.connection, td.filenamePath.c_str(), 
											 data.c_str(), 1, onlyHeader);
      return ret;
    }
    else if( mimecmd == CGI_CMD_RUNMSCGI )
    {
      char* target;
      int allowMscgi = 1;
	    const char *dataH = td.connection->host->getHashedData("ALLOW_MSCGI");
	    if(dataH)
	    {
        if(!strcmpi(dataH, "YES"))
          allowMscgi = 1;
        else
          allowMscgi = 0;      
	    }

      if(!allowMscgi || !(permissions & MYSERVER_PERMISSION_EXECUTE))
      {
        return sendAuth();
      }
      if(td.request.uriOptsPtr)
        target = td.request.uriOptsPtr;
      else
        target = (char*)td.request.uriOpts.c_str();

      /*! Check if the MSCGI library is loaded.  */
      if(mscgiLoaded)
      {
        ret = mscgi.send(&td, td.connection, td.filenamePath.c_str(), 
												 target, 1, onlyHeader);
        return ret;
      }
      return raiseHTTPError(500);
    }
    else if( mimecmd == CGI_CMD_EXECUTEWINCGI )
    {
      ostringstream cgipath;
      int allowWincgi = 1;
	    const char *dataH = td.connection->host->getHashedData("ALLOW_WINCGI");
	    if(dataH)
	    {
        if(!strcmpi(dataH, "YES"))
          allowWincgi = 1;
        else
          allowWincgi = 0;      
	    }

      if(!allowWincgi || !(permissions & MYSERVER_PERMISSION_EXECUTE))
      {
        return sendAuth();
      }
      if(data.length())
      {
        cgipath <<  data << " \""<< td.filenamePath <<  "\"";
      }
      else
      {
        cgipath << td.filenamePath;
      }
      ret = wincgi.send(&td, td.connection, cgipath.str().c_str(), 
												1, onlyHeader);
      return ret;
    }
    else if( mimecmd == CGI_CMD_RUNFASTCGI )
    {
      int allowFastcgi = 1;
	    const char *dataH = td.connection->host->getHashedData("ALLOW_FASTCGI");
	    if(dataH)
	    {
        if(!strcmpi(dataH, "YES"))
          allowFastcgi=1;
        else
          allowFastcgi=0;      
	    }
      if(!allowFastcgi || !(permissions & MYSERVER_PERMISSION_EXECUTE))
      {
        return sendAuth();
      }	
      ret = fastcgi.send(&td, td.connection, td.filenamePath.c_str(), 
												 data.c_str(), 0, onlyHeader);
      return ret;
    }
    else if(mimecmd == CGI_CMD_EXECUTEFASTCGI)
    {
      int allowFastcgi = 1;
	    const char *dataH = td.connection->host->getHashedData("ALLOW_FASTCGI");
	    if(dataH)
	    {
        if(!strcmpi(dataH, "YES"))
          allowFastcgi = 1;
        else
          allowFastcgi = 0;      
	    }
      if(!allowFastcgi || !(permissions & MYSERVER_PERMISSION_EXECUTE))
      {
        return sendAuth();
      }
      ret = fastcgi.send(&td, td.connection, td.filenamePath.c_str(), 
												 data.c_str(), 1, onlyHeader);
      return ret;
    }
    else if( mimecmd == CGI_CMD_RUNSCGI )
    {
      int allowScgi = 1;
	    const char *dataH = td.connection->host->getHashedData("ALLOW_SCGI");
	    if(dataH)
	    {
        if(!strcmpi(dataH, "YES"))
          allowScgi = 1;
        else
          allowScgi = 0;      
	    }
      if(!allowScgi || !(permissions & MYSERVER_PERMISSION_EXECUTE))
      {
        return sendAuth();
      }	
      ret = scgi.send(&td, td.connection, td.filenamePath.c_str(), 
											data.c_str(), 0, onlyHeader);
      return ret;
    }
    else if(mimecmd == CGI_CMD_EXECUTESCGI)
    {
      int allowScgi = 1;
	    const char *dataH = td.connection->host->getHashedData("ALLOW_SCGI");
	    if(dataH)
	    {
        if(!strcmpi(dataH, "YES"))
          allowScgi = 1;
        else
          allowScgi = 0;      
	    }
      if(!allowScgi || !(permissions & MYSERVER_PERMISSION_EXECUTE))
      {
        return sendAuth();
      }
      ret = scgi.send(&td, td.connection, td.filenamePath.c_str(), 
											data.c_str(), 1, onlyHeader);
      return ret;
    }

    else if( mimecmd == CGI_CMD_SENDLINK )
    {
      u_long nbr;
      char* linkpath;
      char* pathInfo;
      int linkpathSize;
      File h;
      int allowSendlink = 1;
	    const char *dataH = 
				td.connection->host->getHashedData("ALLOW_SEND_LINK");

	    if(dataH)
	    {
        if(!strcmpi(dataH, "YES"))
          allowSendlink = 1;
        else
          allowSendlink = 0;      
	    }

      if(!allowSendlink || !(permissions & MYSERVER_PERMISSION_READ))
      {
        return sendAuth();
      }

      if(h.openFile(td.filenamePath.c_str(), 
                    File::MYSERVER_OPEN_IFEXISTS|File::MYSERVER_OPEN_READ))
      {
        return raiseHTTPError(500);
      }

      linkpathSize = h.getFileSize() + td.pathInfo.length() + 1;

      if(linkpathSize > MYSERVER_KB(10))
        linkpathSize = MYSERVER_KB(10);

      linkpath=new char[linkpathSize];

      if(linkpath == 0)
      {
        return sendHTTPhardError500();
      }

      if(h.readFromFile(linkpath, linkpathSize, &nbr))
      {
        h.closeFile();
        delete [] linkpath;
        return raiseHTTPError(500);/*!Internal server error*/
      }

      h.closeFile();
      linkpath[nbr]='\0';

      pathInfo = new char[td.pathInfo.length() + 1];

      if(pathInfo == 0)
      {
        delete [] linkpath;
        return raiseHTTPError(500);/*!Internal server error*/
      }
      strcpy(pathInfo, td.pathInfo.c_str());
      translateEscapeString(pathInfo);
      strncat(linkpath, pathInfo,strlen(linkpath));

      if(nbr)
      {
        string uri;
        uri.assign(linkpath);
        ret = sendHTTPResource(uri, systemrequest, onlyHeader, 1);
      }
      else
        ret = raiseHTTPError(404);

      delete [] linkpath;
      delete [] pathInfo;
      return ret;
    }
    else if( mimecmd == CGI_CMD_EXTERNAL )
    {
      int allowExternal = 1;
	    const char *dataH = 
				td.connection->host->getHashedData("ALLOW_EXTERNAL_COMMANDS");

	    if(dataH)
	    {
        if(!strcmpi(dataH, "YES"))
          allowExternal = 1;
        else
          allowExternal = 0;      
	    }     

      if(allowExternal && td.mime)
      {
        DynamicHttpManager* manager = 
					dynManagerList.getPlugin(td.mime->cmdName);

        if(manager)
          return manager->send(&td, td.connection, td.filenamePath.c_str(), 
                               data.c_str(), onlyHeader);
        else
          return raiseHTTPError(501);
      }

    }

    {
      int allowSend = 1;
	    const char *data = td.connection->host->getHashedData("ALLOW_SEND_FILE");
	    if(data)
	    {
        if(!strcmpi(data, "YES"))
          allowSend = 1;
        else
          allowSend = 0;      
	    }   
      if(!allowSend)
      {
        return sendAuth();
      }       
    }

    /*! By default try to send the file as it is.  */
    if(!(permissions & MYSERVER_PERMISSION_READ))
    {     
      return sendAuth();
    }

    lastMT = FilesUtility::getLastModTime(td.filenamePath.c_str());
    if(lastMT == -1)
    {
      return raiseHTTPError(500);
    }
    getRFC822GMTTime(lastMT, tmpTime, HTTP_RESPONSE_LAST_MODIFIED_DIM);
		td.response.lastModified.assign(tmpTime);
    
		{
			HttpRequestHeader::Entry *ifModifiedSince = 
				td.request.other.get("Last-Modified");

			if(ifModifiedSince && ifModifiedSince->value->length())
			{
				if(!ifModifiedSince->value->compare(td.response.lastModified.c_str()))
				{
					return sendHTTPNonModified();
				}
			}
		}
    ret = httpFile.send(&td, td.connection, td.filenamePath.c_str(), 
												0, onlyHeader);
  }
  catch(...)
  {
    return raiseHTTPError(500); 
  };

	return ret;
}
/*!
 *Log the access using the Common Log Format or the Combined one.
 */
int Http::logHTTPaccess()
{
	char tmpStrInt[12];

  string time;

  try
  {
    td.buffer2->setLength(0);
    *td.buffer2 << td.connection->getIpAddr();
    *td.buffer2<< " ";
	
	  if(td.connection->getLogin())
      *td.buffer2 << td.connection->getLogin();
    else
      *td.buffer2 << "-";

    *td.buffer2<< " ";  

    if(td.connection->getLogin())
      *td.buffer2 << td.connection->getLogin();
    else
      *td.buffer2 << "-";
    
    *td.buffer2 << " [";
    
    getLocalLogFormatDate(time, HTTP_RESPONSE_DATE_DIM);
    *td.buffer2 <<  time  << "] \"";
    
    if(td.request.cmd.length())
      *td.buffer2 << td.request.cmd.c_str() << "";
  
    if(td.request.cmd.length() || td.request.uri.length())
      *td.buffer2 << " ";

    if(td.request.uri.length() == '\0')
      *td.buffer2 <<  "/";
    else
      *td.buffer2 << td.request.uri.c_str();
  

    if(td.request.uriOpts.length())
      *td.buffer2 << "?" << td.request.uriOpts.c_str();
    
    sprintf(tmpStrInt, "%u ",td.response.httpStatus);
  
    if(td.request.ver.length())
      *td.buffer2 << " " << td.request.ver.c_str()  ;
    
    *td.buffer2<< "\" " << tmpStrInt  << " ";
	

    sprintf(tmpStrInt, "%u", td.sentData);
		*td.buffer2 << tmpStrInt;

    if(td.connection->host)
    {
			HttpRequestHeader::Entry *userAgent = td.request.other.get("User-Agent");
			HttpRequestHeader::Entry *referer = td.request.other.get("Refer");

      if(strstr((td.connection->host)->getAccessLogOpt(), "type=combined"))
        *td.buffer2 << " "  << (referer   ? referer->value->c_str() : "") 
										<< " "  << (userAgent ? userAgent->value->c_str() : "");
    }  
#ifdef WIN32
    *td.buffer2  << "\r\n" << end_str;
#else
    *td.buffer2  << "\n" << end_str;
#endif
    /*!
     *Request the access to the log file then append the message.
     */
     if(td.connection->host)
     {
       td.connection->host->accessesLogRequestAccess(td.id);
       td.connection->host->accessesLogWrite(td.buffer2->getBuffer());
       td.connection->host->accessesLogTerminateAccess(td.id);
     }
    td.buffer2->setLength(0);
  }
  catch(...)
  {
    return 1;
  };
	return 0;
}

/*!
 *This is the HTTP protocol main procedure to parse a request 
 *over the HTTP.
 */
int Http::controlConnection(ConnectionPtr a, char* /*b1*/, char* /*b2*/, 
                            int bs1, int bs2, u_long nbtr, u_long id)
{
 	int retvalue = -1;
  int ret = 0;
	int validRequest;
  /*! Dimension of the POST data. */
	int contentLength = -1;
  DynamicHttpCommand *dynamicCommand;
  try
  {
    td.buffer = a->getActiveThread()->getBuffer();
    td.buffer2 = a->getActiveThread()->getBuffer2();
    td.buffersize = bs1;
    td.buffersize2 = bs2;
    td.nBytesToRead = nbtr;
    td.connection = a;
    td.id = id;
    td.lastError = 0;
    td.http = this;
    td.appendOutputs = 0;
    td.onlyHeader = 0;
    td.inputData.setHandle((FileHandle)0);
    td.outputData.setHandle((FileHandle)0);
    td.filenamePath.assign("");
    td.outputDataPath.assign("");
    td.inputDataPath.assign("");
    td.mime = 0;
		td.sentData = 0;
		td.vhostDir.assign("");
		td.vhostSys.assign("");
		{
			HashMap<string,string*>::Iterator it = td.other.begin();
			while(it != td.other.end())
				delete (*it);
		}
		td.other.clear();

    /*!
     *Reset the request and response structures.
     */
    HttpHeaders::resetHTTPRequest(&td.request);
    HttpHeaders::resetHTTPResponse(&td.response);
    
    /*! Reset the HTTP status once per request. */
    td.response.httpStatus = 200;
    
    /*!
     *If the connection must be removed, remove it.
     */
    if(td.connection->getToRemove())
    {
      switch(td.connection->getToRemove())
      {
        /*! Remove the connection from the list.  */
			  case CONNECTION_REMOVE_OVERLOAD:
					retvalue = raiseHTTPError(503);
					logHTTPaccess();
					return ClientsThread::DELETE_CONNECTION;
        default:
					return ClientsThread::DELETE_CONNECTION;
      }
    }
    validRequest = 
			HttpHeaders::buildHTTPRequestHeaderStruct(&td.request, &td);
  
    /*! If the header is incomplete returns 2. */
    if(validRequest == -1)/*!If the header is incomplete returns 2.  */
    {
      /*! Be sure that the client can handle the 100 status code.  */
      if(td.request.ver.compare("HTTP/1.0"))
      {
        char* msg = "HTTP/1.1 100 Continue\r\n\r\n";
        Thread::wait(2);
        if(a->socket->bytesToRead() == 0) 
        {
          if(a->socket->send(msg, (int)strlen(msg), 0)==-1)
            return ClientsThread::DELETE_CONNECTION;
        }
      }
      return ClientsThread::INCOMPLETE_REQUEST;
    }
	
    if(a->protocolBuffer)
      ((HttpUserData*)a->protocolBuffer)->digestChecked = 0;	
	
    /*!
     *If the validRequest cointains an error code send it to the user.
     */
    if(validRequest != 200)
    {
      retvalue = raiseHTTPError(validRequest);
      logHTTPaccess();
      return ClientsThread::DELETE_CONNECTION;
    }
    /*! Be sure that we can handle the HTTP version.  */
    if((td.request.ver.compare("HTTP/1.1")) && 
       (td.request.ver.compare("HTTP/1.0")) && 
       (td.request.ver.compare("HTTP/0.9")))
    {	
      raiseHTTPError(505);
      logHTTPaccess();
      /*! Remove the connection from the list.  */
      return ClientsThread::DELETE_CONNECTION;
    }

		td.response.ver.assign(td.request.ver.c_str());

    /*! Do not use Keep-Alive over HTTP version older than 1.1.  */
    if(td.request.ver.compare("HTTP/1.1") )
    {
			HttpRequestHeader::Entry *connection = 
				td.request.other.get("Connection");

      if(connection && connection->value->length())
			{
        connection->value->assign("close");
			}
    }

    /*!
     *For methods that accept data after the HTTP header set the correct 
     *pointer and create a file containing the informations after the header.
     */
		Server::getInstance()->temporaryFileName(td.id, td.inputDataPath);
		Server::getInstance()->temporaryFileName(td.id, td.outputDataPath);

    dynamicCommand = dynCmdManager.getPlugin(td.request.cmd);

		/* If the used method supports POST data, read it.  */
    if((!td.request.cmd.compare("POST")) || 
			 (!td.request.cmd.compare("PUT")) || 
       (dynamicCommand && dynamicCommand->acceptData() ))
    {
			int ret;
			if(readPostData(&td, &ret))
			{
				logHTTPaccess();
				return ret;
			}
    }
    else
    {
			 contentLength = 0;
			 td.request.uriOptsPtr = 0;
    }

    /*! If return value is not configured propertly.  */
    if(retvalue == -1)
    {
      /*!
       *How is expressly said in the RFC2616 a client that sends an 
       *HTTP/1.1 request MUST sends a Host header.
       *Servers MUST reports a 400 (Bad request) error if an HTTP/1.1
       *request does not include a Host request-header.
       */
			HttpRequestHeader::Entry *host = td.request.other.get("Host");
			HttpRequestHeader::Entry *connection = 
				td.request.other.get("Connection");
		
      if((!td.request.ver.compare("HTTP/1.1")) && 
				 ((host && host->value->length() == 0) || (host == 0)) )
      {
        raiseHTTPError(400);
        /*!
         *If the inputData file was not closed close it.
         */
        if(td.inputData.getHandle())
        {
          td.inputData.closeFile();
          FilesUtility::deleteFile(td.inputDataPath);
        }

				/*!
         *If the outputData file was not closed close it.
         */
        if(td.outputData.getHandle())
        {
          td.outputData.closeFile();
          FilesUtility::deleteFile(td.outputDataPath);
        }
        logHTTPaccess();
				return ClientsThread::DELETE_CONNECTION;
      }
      else
      {
        /*!
         *Find the virtual host to check both host name and IP value.
         */
        Vhost* newHost = Server::getInstance()->getVhosts()->getVHost(host ? 
																		host->value->c_str() : "", 
																		 a->getLocalIpAddr(), a->getLocalPort());
        if(a->host)
          a->host->removeRef();
        a->host = newHost;
        if(a->host == 0)
        {
          raiseHTTPError(400);
          /*!
           *If the inputData file was not closed close it.
           */
          if(td.inputData.getHandle())
          {
            td.inputData.closeFile();
            FilesUtility::deleteFile(td.inputDataPath);
          }
          /*!
           *If the outputData file was not closed close it.
           */
          if(td.outputData.getHandle())
          {
            td.outputData.closeFile();
            FilesUtility::deleteFile(td.outputDataPath);
          }
          logHTTPaccess();
					return ClientsThread::DELETE_CONNECTION;
        }
      }

			if(td.request.uri.length() > 2 && td.request.uri[1] == '~'){
				string documentRoot;
				u_long pos = 2;
				string user;
				while(pos < td.request.uri.length())
					if(td.request.uri[++pos] == '/')
						break;
				user.assign(td.request.uri.substr(2, pos - 2));
				Server::getInstance()->getHomeDir()->getHomeDir(user,
																												documentRoot);
				
				if(documentRoot.length())
				{

					const char *useHomeDir = td.connection-> host ? 
						td.connection->host->getHashedData("USE_HOME_DIRECTORY") : 0;

					const char *homeDir = td.connection-> host ? 
						td.connection->host->getHashedData("HOME_DIRECTORY") : 0;

					if(homeDir == 0)
						homeDir = "public_html";

					if(useHomeDir == 0 || strcmpi(useHomeDir, "YES"))
						return raiseHTTPError(404);


					td.vhostDir.assign(documentRoot);
					td.vhostDir.append("/");
					td.vhostDir.append(homeDir);
					
					if(!td.request.uriEndsWithSlash && !(td.request.uri.length() - pos))
					{
						td.request.uri.append("/");

						return sendHTTPRedirect(td.request.uri.c_str());
					}

					if(td.request.uri.length() - pos)
						td.request.uri.assign(td.request.uri.substr(pos,
																td.request.uri.length()));
					else
						td.request.uri.assign("");
				}
			}

		  /*! 
       *Check if there is a limit for the number of connections in the 
			 *virtual host A value of zero means no limit.
       */
		  {
        const char* val = a->host->getHashedData("MAX_CONNECTIONS");
        if(val)
        {
          u_long limit = (u_long)atoi(val);
          if(limit && (u_long)a->host->getRef() >= limit)
          {
            retvalue = raiseHTTPError(500);
            logHTTPaccess();
            return retvalue ? ClientsThread::KEEP_CONNECTION
                            : ClientsThread::DELETE_CONNECTION;
          }     
        }
      }
		
      if(connection && !stringcmpi(connection->value->c_str(), "keep-alive")) 
      {
        /*!
         *Support for HTTP pipelining.
         */
        if(contentLength == 0)
        {
          /*!
           *connectionBuffer is 8 KB, so don't copy more bytes.
           */
          a->setDataRead(MYSERVER_KB(8) < (int)strlen(td.buffer->getBuffer()) -
											 td.nHeaderChars ? 
											 MYSERVER_KB(8) : 
											 (int)strlen(td.buffer->getBuffer()) - td.nHeaderChars);

          if(a->getDataRead() )
          {
            memcpy(a->connectionBuffer, (td.buffer->getBuffer() +
                                         td.nHeaderChars), a->getDataRead());
            retvalue = ClientsThread::INCOMPLETE_REQUEST_NO_WAIT;
          }
          else
            retvalue = ClientsThread::KEEP_CONNECTION;
          
        }	
        else
          retvalue = ClientsThread::KEEP_CONNECTION;
      }
      else
      {
        retvalue = ClientsThread::DELETE_CONNECTION;
      }
      
      /*! 
       *Set the throttling rate for the socket. This setting can be 
       *changed later.
       */
      if(a->host->getThrottlingRate() == (u_long) -1)
        a->socket->setThrottling(Server::getInstance()->getThrottlingRate());
      else
        a->socket->setThrottling(a->host->getThrottlingRate());

      
      /*!
       *Here we control all the HTTP commands.
       */
       
      /*! GET REQUEST.  */
      if(!td.request.cmd.compare("GET"))
      {
				ret = sendHTTPResource(td.request.uri);
      }
      /*! POST REQUEST.  */
      else if(!td.request.cmd.compare("POST"))
      {
				ret = sendHTTPResource(td.request.uri);
      }
      /*! HEAD REQUEST.  */
      else if(!td.request.cmd.compare("HEAD"))
      {
        td.onlyHeader = 1;
        ret = sendHTTPResource(td.request.uri, 0, 1);
      }
      /*! DELETE REQUEST.  */
      else if(!td.request.cmd.compare("DELETE"))
      {
        ret = deleteHTTPRESOURCE(td.request.uri, 0);
      }
      /*! PUT REQUEST.  */
      else if(!td.request.cmd.compare("PUT"))
      {
        ret = putHTTPRESOURCE(td.request.uri, 0, 1);
      }
      /*! OPTIONS REQUEST.  */
      else if(!td.request.cmd.compare("OPTIONS"))
      {
        ret = optionsHTTPRESOURCE(td.request.uri, 0);
      }
      /*! TRACE REQUEST.  */
      else if(!td.request.cmd.compare("TRACE"))
      {
        ret = traceHTTPRESOURCE(td.request.uri, 0);
      }
      else
      {
        /*! 
         *Return Method not implemented(501) if there 
         *is not a dynamic methods manager. 
         */
        if(!dynamicCommand)
          ret = raiseHTTPError(501);
        else
          retvalue = dynamicCommand->send(&td, a, td.request.uri, 0, 0, 0) 
						? ClientsThread::KEEP_CONNECTION
						: ClientsThread::DELETE_CONNECTION;
      }
      logHTTPaccess();
    }

    /*!
     *If the inputData file was not closed close it.
     */
    if(td.inputData.getHandle())
		{
      td.inputData.closeFile();
      FilesUtility::deleteFile(td.inputDataPath);
    }
    /*!
     *If the outputData file was not closed close it.
     */
    if(td.outputData.getHandle())
    {
      td.outputData.closeFile();
      FilesUtility::deleteFile(td.outputDataPath);
    }
		
		{
			HttpRequestHeader::Entry *connection = 
				                            td.request.other.get("Connection");
			if(connection)
				ret &= !stringcmpi(connection->value->c_str(), "keep-alive");
			else
				ret = 0;
    }
		return (ret && retvalue != ClientsThread::DELETE_CONNECTION) 
			       ? retvalue 
			       : ClientsThread::DELETE_CONNECTION;
  }
  catch(...)
  {
    logHTTPaccess();
    return ClientsThread::DELETE_CONNECTION;
  }
}

/*!
 *Compute the Digest outputting it to a buffer.
 */
void Http::computeDigest(char* out, char* buffer)
{
	Md5 md5;
	if(!out)
		return;
	sprintf(buffer, "%i-%u-%s", (int)clock(), (u_int)td.id, 
          td.connection->getIpAddr());
	md5.init();
	md5.update((unsigned char const*)buffer, (unsigned int)strlen(buffer));
	md5.end(out);
}

/*!
 *Send to the client an authorization request.
 */
int Http::requestAuthorization()
{
	Md5 md5;
	string time;
	HttpRequestHeader::Entry *connection = td.request.other.get("Connection");
	HttpRequestHeader::Entry *host = td.request.other.get("Host");
	td.response.httpStatus = 401;
	td.buffer2->setLength(0);
	*td.buffer2 << "HTTP/1.1 401 Unauthorized\r\n" 
							<< "Accept-Ranges: bytes\r\nServer: MyServer " ;
	*td.buffer2 << versionOfSoftware ;
	*td.buffer2 << "\r\nContent-type: text/html\r\nConnection: ";
	*td.buffer2 << (connection ? connection->value->c_str() : "");
	*td.buffer2 << "\r\nContent-length: 0\r\n";
	if(td.authScheme == HTTP_AUTH_SCHEME_BASIC)
	{
		*td.buffer2 <<  "WWW-Authenticate: Basic realm=\"" 
								<< (host ? host->value->c_str() : "") <<  "\"\r\n";
	}
	else if(td.authScheme == HTTP_AUTH_SCHEME_DIGEST)
	{
		char md5Str[256];
		
		if(td.connection->protocolBuffer == 0)
		{
			td.connection->protocolBuffer = new HttpUserData;
			if(!td.connection->protocolBuffer)
			{
				sendHTTPhardError500();
				return 0;
			}
			((HttpUserData*)(td.connection->protocolBuffer))->reset();
		}
		myserver_strlcpy(((HttpUserData*)td.connection->protocolBuffer)->realm,
										 host ? host->value->c_str() : "", 48);

		/*! Just a random string.  */
		md5Str[0] = (char)td.id;
		md5Str[1] = (char)((clock() >> 24) & 0xFF);
		md5Str[2] = (char)((clock() >> 16) & 0xFF);
		md5Str[3] = (char)((clock() >>  8)   & 0xFF);
		md5Str[4] = (char) (clock() & 0xFF);
		strncpy(&(md5Str[5]), td.request.uri.c_str(), 256 - 5);
		md5.init();
		md5.update((unsigned char const*)md5Str, 
							 (unsigned int)strlen(md5Str));
		md5.end(((HttpUserData*)td.connection->protocolBuffer)->opaque);
		
		if(td.connection->protocolBuffer && 
			 (!(((HttpUserData*)td.connection->protocolBuffer)->digest)) || 
			 (((HttpUserData*)td.connection->protocolBuffer)->nonce[0]=='\0'))
		{
			computeDigest(((HttpUserData*)td.connection->protocolBuffer)->nonce,
										md5Str);
			((HttpUserData*)td.connection->protocolBuffer)->nc = 0;
		}
		
		*td.buffer2 << "WWW-Authenticate: digest "
								<< " qop=\"auth\", algorithm =\"MD5\", realm =\""
								<< ((HttpUserData*)td.connection->protocolBuffer)->realm 
								<< "\",  opaque =\"" 
								<< ((HttpUserData*)td.connection->protocolBuffer)->opaque
								<< "\",  nonce =\""
								<< ((HttpUserData*)td.connection->protocolBuffer)->nonce
								<<"\" ";

		if(((HttpUserData*)td.connection->protocolBuffer)->cnonce[0])
		{
			*td.buffer2 << ", cnonce =\""
									<<((HttpUserData*)td.connection->protocolBuffer)->cnonce
									<<"\" ";
		}
		*td.buffer2 << "\r\n";
	}		
	else
	{
		/*!
		 *Send a non implemented error page if the auth scheme is not known.
		 */
		return raiseHTTPError(501);
	}				
	*td.buffer2 << "Date: ";
	getRFC822GMTTime(time, HTTP_RESPONSE_DATE_DIM);
	*td.buffer2  << time
							 << "\r\n\r\n";
	if(td.connection->socket->send(td.buffer2->getBuffer(), 
																 td.buffer2->getLength(), 0) == -1)
	{
		return 0;
	}
	return 1;
}

/*!
 *Sends an error page to the client.
 *Nonzero to keep the connection.
 */
int Http::raiseHTTPError(int ID)
{
  try
  {
		string defFile;
		int ret = 0;
    string time;
    ostringstream errorFile;
		string errorMessage;
    int errorBodyLength = 0;
    int useMessagesFiles = 1;
		HttpRequestHeader::Entry *host = td.request.other.get("Host");
		HttpRequestHeader::Entry *connection = td.request.other.get("Connection");
    const char *useMessagesVal = td.connection->host ? 
			td.connection->host->getHashedData("USE_ERROR_FILE") : 0;
    if(useMessagesVal)
    {
	    if(!lstrcmpi(useMessagesVal, "YES"))
	   	  useMessagesFiles = 1;
	   	else
	      useMessagesFiles = 0;
   	}

    if(td.lastError)
      return sendHTTPhardError500();

    td.lastError = ID;

    HttpHeaders::buildDefaultHTTPResponseHeader(&(td.response));
    if(connection && !stringcmpi(connection->value->c_str(), "keep-alive"))
    {
      td.response.connection.assign("keep-alive");
    }
		
		td.response.httpStatus = ID;
		secCacheMutex.lock();
      
		/*! 
		 *The specified error file name must be in the web directory 
		 *of the virtual host. 
		 */
		if(td.connection->host)
			ret = secCache.getErrorFileName(td.getVhostDir(), ID,
																			td.getVhostSys(), defFile);
		else
			ret = -1;
		
		secCacheMutex.unlock();
    
		if(ret == -1)
    {
			sendHTTPhardError500();
			return 0;
		}
		else if(ret)
    {
			ostringstream nURL;
			int isPortSpecified = 0;
			const char* hostStr = host ? host->value->c_str() : "";
			/*!
			 *Change the URI to reflect the default file name.
			 */
			nURL << protocolPrefix << hostStr;
			for(int i = 0; hostStr[i]; i++)
      {
				if(hostStr[i]==':')
        {
					isPortSpecified = 1;
					break;
				}
			}
			if(!isPortSpecified)
				nURL << ":" << td.connection->host->getPort();
			if(nURL.str()[nURL.str().length()-1] != '/')
				nURL << "/";
			
			nURL << defFile;
			
			if(td.pathInfo.length())
				nURL << "/" << td.pathInfo;

			if(td.request.uriOpts.length())
				nURL << "?" << td.request.uriOpts;
			
        return sendHTTPRedirect(nURL.str().c_str());
		}
	    
		getRFC822GMTTime(time, HTTP_RESPONSE_DATE_EXPIRES_DIM);
		td.response.dateExp.assign(time);
	
		{
			string page;
			HttpErrors::getErrorMessage(ID, td.response.errorType);
			HttpErrors::getErrorPage(ID, page);
			errorFile << td.getVhostSys() << "/" << page;
		}

		if(useMessagesFiles && FilesUtility::fileExists(errorFile.str().c_str()))
		{
			string tmp;
			HttpErrors::getErrorPage(ID, tmp);
			return sendHTTPResource(tmp, 1, td.onlyHeader);
		}
		
		HttpErrors::getErrorMessage(ID, errorMessage);
		/*! Send only the header (and the body if specified). */
		{
			const char* value = 
				td.connection->host->getHashedData("ERRORS_INCLUDE_BODY");  
			if(value && !strcmpi(value, "YES"))
			{
				ostringstream s;
				errorBodyLength = errorMessage.length();
				s << errorBodyLength;
				td.response.contentLength.assign(s.str());
			}
			else
			{
				errorBodyLength = 0;
				td.response.contentLength.assign("0");
			}
		}
		HttpHeaders::buildHTTPResponseHeader(td.buffer->getBuffer(), 
																				 &td.response);
		if(td.connection->socket->send(td.buffer->getBuffer(), 
																	 (u_long)strlen(td.buffer->getBuffer()), 0)
			 == -1)
			return 0;
		
		if(errorBodyLength && (td.connection->socket->send(errorMessage.c_str(),
																											 errorBodyLength, 0) 
													 == -1))
			return 0;
		
		return 1;      
	}
	catch(bad_alloc &ba)
	{
		return 0;
	}
	catch(...)
	{
		return 0;
	};
}

/*!
 *Send a hard wired 500 error when we have a system error
 */
int Http::sendHTTPhardError500()
{
  MemBuf tmp;
	char tmpStr[12];
	string time;
	string errorMsg;
	const char hardHTML[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\"\n\
\"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n\
<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\">\n\
<head>\n\
<title>500 Internal Server Error</title>\n\
<meta http-equiv=\"content-type\" content=\"text/html;charset=UTF-8\" />\n\
</head>\n\
<body style=\"color: #666699;\">\n\
<br />\n\
<h1 style=\"text-align: center;\">\n\
Error 500\n\
<br /><br />\n\
Internal Server Error\n\
</h1>\n\
</body>\n\
</html>\r\n";
	HttpErrors::getErrorMessage(500, errorMsg);

	td.response.httpStatus = 500;
	td.buffer->setLength(0);
	*td.buffer <<  errorMsg;
	*td.buffer << " from: " ;
	*td.buffer << td.connection->getIpAddr() ;
	*td.buffer << "\r\n";	
	td.buffer2->setLength(0);
	*td.buffer2 << "HTTP/1.1 500 System Error\r\nServer: MyServer ";
	*td.buffer2 << versionOfSoftware;
	*td.buffer2 <<" \r\nContent-type: text/html\r\nContent-length: ";
  tmp.intToStr((int)strlen(hardHTML), tmpStr, 12);
	*td.buffer2 << tmp;
	*td.buffer2 << "\r\n";
	*td.buffer2 <<"Date: ";
	getRFC822GMTTime(time, HTTP_RESPONSE_DATE_DIM);
	*td.buffer2 << time;
	*td.buffer2 << "\r\n\r\n";
	/*! Send the header.  */
	if(td.connection->socket->send(td.buffer2->getBuffer(), 
																 (u_long)td.buffer2->getLength(), 0) != -1)
	{
		/*! Send the body.  */
    if(!td.onlyHeader)
   		td.connection->socket->send(hardHTML, (u_long)strlen(hardHTML), 0);
	}
	return 0;
}

/*!
 *Returns the MIME type passing its extension.
 *Returns zero if the file is registered.
 */
MimeRecord* Http::getMIME(string &filename)
{
  string ext;
	FilesUtility::getFileExt(ext, filename);
	
  if(allowVhostMime && td.connection->host->isMIME() )
  {
    return td.connection->host->getMIME()->getRecord(ext);
  }
	return Server::getInstance()->getMimeManager()->getRecord(ext);
}

/*!
 *Map an URL to the machine file system. Return 200 on success. 
 *Any other return value is the HTTP error.
 */
int Http::getPath(string& filenamePath, const char *filename, 
									int systemrequest)
{
	/*!
   *If it is a system request, search the file in the system directory.
   */
	if(systemrequest)
	{
    if(!strlen(td.getVhostSys()) 
       || FilesUtility::getPathRecursionLevel(filename)< 2 )
    {
      return 401;
    }
    filenamePath.assign(td.getVhostSys());
		if(filename[0] != '/')
			filenamePath.append("/");
    filenamePath.append(filename);
	}
	/*!
   *Else the file is in the web directory.
   */
	else
	{	
		if(filename[0])
		{
      const char *root;
      /*! 
       *uri starting with a /sys/ will use the system directory as
       *the root path. Be sure to don't allow access to the system root
       *but only to subdirectories.
       */
      if(filename[0] == '/' && filename[1] == 's' && filename[2] == 'y'
         && filename[3] == 's' && filename[4] == '/')
      {
        root = td.getVhostSys();
        /*! 
         *Do not allow access to the system directory root but only
         *to subdirectories. 
         */
        if(FilesUtility::getPathRecursionLevel(filename)< 2)
        {
          return 401;
        }
        filename = filename + 5;
      }
      else
      {
        root = td.getVhostDir();
      }
			filenamePath.assign(root);
			if(filename[0] != '/')
				filenamePath.append("/");
      filenamePath.append(filename);
		}
		else
		{
      filenamePath.append(td.getVhostDir());
		}

	}
  return 200;
}

/*!
 *Get the CSS file used in a browsed directory.
 */
const char* Http::getBrowseDirCSSFile()
{
  return browseDirCSSpath.c_str();
}

/*!
 *Get the GZIP threshold.
 */
u_long Http::getGzipThreshold()
{
  return gzipThreshold;
}

/*!
 *Send a redirect message to the client.
 */
int Http::sendHTTPRedirect(const char *newURL)
{
	string time;
	HttpRequestHeader::Entry *connection = td.request.other.get("Connection");

	td.response.httpStatus = 302;
	td.buffer2->setLength(0);
	*td.buffer2 << "HTTP/1.1 302 Moved\r\nAccept-Ranges: bytes\r\n"
							<< "Server: MyServer "	<< versionOfSoftware << "\r\n"
							<< "Content-type: text/html\r\n"
							<< "Location: " << newURL << "\r\n"
							<< "Content-length: 0\r\n";

	if(connection && !stringcmpi(connection->value->c_str(), "keep-alive"))
		*td.buffer2 << "Connection: keep-alive\r\n";	
	else
		*td.buffer2 << "Connection: close\r\n";	
		
	*td.buffer2<< "Date: ";
	getRFC822GMTTime(time, HTTP_RESPONSE_DATE_DIM);
	*td.buffer2 << time
							<< "\r\n\r\n";
	if(td.connection->socket->send(td.buffer2->getBuffer(), 
																 (int)td.buffer2->getLength(), 0) == -1)
		return 0;

	return 1;
}

/*!
 *Send a non-modified message to the client.
 */
int Http::sendHTTPNonModified()
{
	string time;
	HttpRequestHeader::Entry *connection = td.request.other.get("Connection");

	td.response.httpStatus = 304;
	td.buffer2->setLength(0);
	*td.buffer2 << "HTTP/1.1 304 Not Modified\r\nAccept-Ranges: bytes\r\n"
							<< "Server: MyServer "  << versionOfSoftware <<  "\r\n";

	if(connection && !stringcmpi(connection->value->c_str(), "keep-alive"))
		*td.buffer2 << "Connection: keep-alive\r\n";	
	else
		*td.buffer2 << "Connection: close\r\n";	

	getRFC822GMTTime(time, HTTP_RESPONSE_DATE_DIM);

	*td.buffer2 << "Date: " << time << "\r\n\r\n";

	if(td.connection->socket->send(td.buffer2->getBuffer(), 
																 (int)td.buffer2->getLength(), 0) == -1)
		return 0;
	return 1;
}

/*!
 *Send a 401 error.
 */
int Http::sendAuth()
{
	if(td.connection->getnTries() > 2)
	{
		return raiseHTTPError(401);
	}
	else
	{	
		td.connection->incnTries();
		return requestAuthorization();
	}
}

/*!
 *Load the HTTP protocol.
 */
int Http::loadProtocol(XmlParser* languageParser)
{
  char *data = 0;
  int  nDefaultFilename = 0;
	XmlParser *configurationFileManager = Server::getInstance()->getConfiguration();
	string pluginsResource(Server::getInstance()->getExternalPath());
	xmlDocPtr xmlDoc = configurationFileManager->getDoc();

  if(initialized)
		return 0;

  secCacheMutex.init();
		
	/*
   *Store defaults value.  
   *By default use GZIP with files bigger than a MB.  
   */
  cgiTimeout = MYSERVER_SEC(15);
	gzipThreshold = 1 << 20;
	browseDirCSSpath.assign("");


	/* Load the HTTP errors.  */
	HttpErrors::load();

	/* Initialize ISAPI.  */
	Isapi::load(configurationFileManager);
	
	/* Initialize FastCGI.  */
	FastCgi::load(configurationFileManager);	

	/* Initialize SCGI.  */
	Scgi::load(configurationFileManager);	

	/* Load the MSCGI library.  */
	mscgiLoaded = MsCgi::load(configurationFileManager) ? 0 : 1;
	if(mscgiLoaded)
  {
		Server::getInstance()->logWriteln(
																languageParser->getValue("MSG_LOADMSCGI") );
  }
	else
	{
		Server::getInstance()->logPreparePrintError();
		Server::getInstance()->logWriteln(
														 languageParser->getValue("ERR_LOADMSCGI") );
		Server::getInstance()->logEndPrintError();
	}
  HttpFile::load(configurationFileManager);
  HttpDir::load(configurationFileManager);

	Server::getInstance()->getPluginsManager()->addNamespace(&dynCmdManager);
	Server::getInstance()->getPluginsManager()->addNamespace(&dynManagerList);

	/*! Determine the min file size that will use GZIP compression.  */
	data = configurationFileManager->getValue("GZIP_THRESHOLD");
	if(data)
	{
		gzipThreshold = atoi(data);
	}	
	data = configurationFileManager->getValue("ALLOW_VHOST_MIME");
	if(data)
	{

    if(!strcmpi(data, "YES"))
      allowVhostMime = 1;
    else
      allowVhostMime = 0;      
	}	
	data = configurationFileManager->getValue("CGI_TIMEOUT");
	if(data)
	{
		cgiTimeout = MYSERVER_SEC(atoi(data));
	}	
	data = configurationFileManager->getValue("BROWSEFOLDER_CSS");
	if(data)
	{
    browseDirCSSpath.append(data);
	}

  Cgi::setTimeout(cgiTimeout);
  Scgi::setTimeout(cgiTimeout);
  WinCgi::setTimeout(cgiTimeout);
  Isapi::setTimeout(cgiTimeout);

	nDefaultFilename = 0;
  defaultFilename.clear();
  

	for(xmlNode *node = xmlDoc->children; node; node = node->next)
	{

		if(!xmlStrcmp(node->name, (const xmlChar *)"MYSERVER"))
		{
			for(node = node->children; node; node = node->next)
			{
				if(!xmlStrcmp(node->name, (const xmlChar *)"DEFAULT_FILENAME"))
				{
					defaultFilename.push_back((char*)node->children->content);
					nDefaultFilename++;
				}
			}
			break;
		}
	}

	/*!
   *Copy the right values in the buffer.
   */
	if(nDefaultFilename == 0)
	{
    defaultFilename.push_back("default.html");
		nDefaultFilename++;
	}

	initialized = 1;
	return 1;	
}

/*!
 *Unload the HTTP protocol.
 */
int Http::unLoadProtocol(XmlParser* languageParser)
{
	 if(!initialized)
		 return 0;

	 /* Unload the errors.  */
	HttpErrors::unLoad();

	/* Clean ISAPI.  */
	Isapi::unLoad();

	/* Clean FastCGI.  */
	FastCgi::unLoad();

	/* Clean SCGI.  */
	Scgi::unLoad();

	/* Clean MSCGI.  */
	MsCgi::unLoad();
 
  HttpFile::unLoad();
	
  HttpDir::unLoad();
  
  secCache.free();

  secCacheMutex.destroy();

  defaultFilename.clear();
  browseDirCSSpath.assign("");

	initialized = 0;
	return 1;
}

/*!
 *Returns the default filename.
 */
const char *Http::getDefaultFilenamePath(u_long ID)
{
  if(defaultFilename.size() <= ID)
    return 0;
  return defaultFilename[ID].c_str();
}

/*!
 *Returns the name of the protocol. If an out buffer 
 *is defined fullfill it with the name too.
 */
char* Http::registerName(char* out, int len)
{
	if(out)
	{
		myserver_strlcpy(out, "HTTP", len);
	}
	return "HTTP";
}

/*!
 *Constructor for the class http.
 */
Http::Http()
{
	protocolPrefix.assign("http://");
	protocolOptions = 0;
	td.filenamePath.assign("");
	td.pathInfo.assign("");
	td.pathTranslated.assign("");
	td.cgiRoot.assign("");
	td.cgiFile.assign("");
	td.scriptPath.assign("");
	td.scriptDir.assign("");
	td.scriptFile.assign("");
	td.inputDataPath.assign("");
	td.outputDataPath.assign("");
}

/*!
 *Destructor for the http class.
 */
Http::~Http()
{
  clean();
}

/*!
 *Clean the used memory.
 */
void Http::clean()
{
  td.filenamePath.assign("");
  td.pathInfo.assign("");
  td.pathTranslated.assign("");
  td.cgiRoot.assign("");
  td.cgiFile.assign("");
  td.scriptPath.assign("");
  td.scriptDir.assign("");
	td.scriptFile.assign("");
  td.inputDataPath.assign("");
  td.outputDataPath.assign("");
}
