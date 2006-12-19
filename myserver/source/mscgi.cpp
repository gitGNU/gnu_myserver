/*
MyServer
Copyright (C) 2002, 2003, 2004, 2006 The MyServer Team
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
#include "../include/server.h"
#include "../include/security.h"
#include "../include/cgi.h"
#include "../include/mime_utils.h"
#include "../include/file.h"
#include "../include/files_utility.h"
#include "../include/sockets.h"
#include "../include/utility.h"
#include "../include/filters_chain.h"
#include <sstream>
using namespace std; 

DynamicLibrary MsCgi::mscgiModule;

/*!
 *Sends the MyServer CGI; differently from standard CGI this doesn't 
 *need a new process to run making it faster.
 *\param td The HTTP thread context.
 *\param s A pointer to the connection structure.
 *\param exec The script path.
 *\param cmdLine The command line.
 *\param execute Specify if the script has to be executed.
 *\param onlyHeader Specify if send only the HTTP header.
 */
int MsCgi::send(HttpThreadContext* td, ConnectionPtr s,const char* exec,
                char* cmdLine, int /*execute*/, int onlyHeader)
{
	/*
   *This is the code for manage a .mscgi file.
   *This files differently from standard CGI don't need a new process to run
   *but are allocated in the caller process virtual space.
   *Usually these files are faster than standard CGI.
   *Actually myServerCGI(.mscgi) is only at an alpha status.
   */
  ostringstream tmpStream;
  ostringstream outDataPath;
  FiltersChain chain;
  u_long nbw;
#ifndef WIN32
#ifdef DO_NOT_USE_MSCGI
	/*!
   *On the platforms where is not available the MSCGI support send a 
   *non implemented error.
   */
	return td->http->raiseHTTPError(e_501);
#endif
#endif

#ifndef DO_NOT_USE_MSCGI 
	DynamicLibrary hinstLib; 
  CGIMAIN ProcMain = 0;
	u_long nbr = 0;
  int ret = 0;
  u_long nbs = 0;
  u_long nbw2 = 0;
	MsCgiData data;
	bool useChunks = false;
	bool keepalive = false;
 	data.envString = td->request.uriOptsPtr ?
                    td->request.uriOptsPtr : (char*) td->buffer->getBuffer();
	
	data.td = td;
	data.errorPage = 0;
	data.server = Server::getInstance();

 	td->scriptPath.assign(exec);

  {
    string tmp;
    tmp.assign(exec);
    FilesUtility::splitPath(tmp, td->cgiRoot, td->cgiFile);
    FilesUtility::splitPath(exec, td->scriptDir, td->scriptFile);
  }

	Cgi::buildCGIEnvironmentString(td,data.envString);
	
	if(!td->appendOutputs)
	{	
		outDataPath << getdefaultwd(0, 0 ) << "/stdOutFileMSCGI_" << (u_int)td->id;
		if(data.stdOut.createTemporaryFile(outDataPath.str().c_str()))
		{
      return td->http->raiseHTTPError(e_500);
    }
	}
	else
	{
		data.stdOut.setHandle(td->outputData.getHandle());
	}

  chain.setProtocol(td->http);
  chain.setProtocolData(td);
  chain.setStream(&(td->connection->socket));

	if(td->mime && Server::getInstance()->getFiltersFactory()->chain(&chain, 
																								 td->mime->filters, 
																								 &(td->connection->socket), 
																																	 &nbw, 1))
	{
		td->connection->host->warningslogRequestAccess(td->id);
		td->connection->host->warningsLogWrite("MSCGI: Error loading filters");
		td->connection->host->warningslogTerminateAccess(td->id);
		chain.clearAllFilters(); 
		return td->http->raiseHTTPError(e_500);
	}

	ret = hinstLib.loadLibrary(exec, 0);

	if (!ret) 
	{ 
		/*
     *Set the working directory to the MSCGI file one.
     */
		setcwd(td->scriptDir.c_str());
		td->buffer2->getAt(0) = '\0';

		ProcMain = (CGIMAIN) hinstLib.getProc( "main"); 

		if(ProcMain)
		{
			(ProcMain)(cmdLine, &data);
		}
    else
    {
      string msg;
      msg.assign("MSCGI: error accessing entrypoint for ");
      msg.append(exec);
      td->connection->host->warningslogRequestAccess(td->id);
      td->connection->host->warningsLogWrite(msg.c_str());
      td->connection->host->warningslogTerminateAccess(td->id);
    }
		hinstLib.close();

		/*
		 *Restore the working directory.
		 */
		setcwd(getdefaultwd(0, 0));
	} 
	else
	{
    if(!td->appendOutputs)
    {	
      data.stdOut.closeFile();
      FilesUtility::deleteFile(outDataPath.str().c_str());
    }
    chain.clearAllFilters(); 
    /* Internal server error.  */
    return td->http->raiseHTTPError(e_500);
	}
	if(data.errorPage)
	{
		int errID = getErrorIDfromHTTPStatusCode(data.errorPage);
		if(errID != -1)
    {
      chain.clearAllFilters(); 
			data.stdOut.closeFile();
			FilesUtility::deleteFile(outDataPath.str().c_str());
			return td->http->raiseHTTPError(errID);
    }
	}
	/* Compute the response length for logging.  */
  td->sentData += data.stdOut.getFileSize();

	/* Send all the data to the client if the append is not used.  */
	if(!td->appendOutputs)
	{
		char *buffer = td->buffer2->getBuffer();
		u_long bufferSize = td->buffer2->getRealLength();
		HttpRequestHeader::Entry* e = td->request.other.get("Connection");
		data.stdOut.setFilePointer(0);

		if(e)
			keepalive = !lstrcmpi(e->value->c_str(),"keep-alive");
		else
			keepalive = false;

		if(keepalive)
    {
			HttpResponseHeader::Entry *e;
			e = td->response.other.get("Transfer-Encoding");
			if(e)
				e->value->assign("chunked");
			else
  		{
				e = new HttpResponseHeader::Entry();
				e->name->assign("Transfer-Encoding");
				e->value->assign("chunked");
				td->response.other.put(*(e->name), e);
			}
			useChunks = true;
		}

		HttpHeaders::buildHTTPResponseHeader(buffer, &(td->response));
		if(s->socket.send(buffer, (int)strlen(buffer), 0) == SOCKET_ERROR)
		{
			if(!td->appendOutputs)
			{
				data.stdOut.closeFile();
				FilesUtility::deleteFile(outDataPath.str().c_str());
			}

      /* Internal server error.  */
      return td->http->raiseHTTPError(e_500);
		}

    if(onlyHeader)
    {
      data.stdOut.closeFile();
      FilesUtility::deleteFile(outDataPath.str().c_str());
      chain.clearAllFilters(); 
      return 1;
    }
		do
		{
			data.stdOut.readFromFile(buffer, bufferSize, &nbr);
			if(nbr)
			{

				if(useChunks)
				{
					ostringstream tmp;
					tmp << hex << nbr << "\r\n";
					td->response.contentLength.assign(tmp.str());
					if(chain.write(tmp.str().c_str(), tmp.str().length(), &nbw2))
					{
						if(!td->appendOutputs)
						{
							data.stdOut.closeFile();
							FilesUtility::deleteFile(outDataPath.str().c_str());
						}
						chain.clearAllFilters(); 
						return 0;
					}
				}

				if(chain.write(buffer, nbr, &nbs))
				{
					if(!td->appendOutputs)
					{
						data.stdOut.closeFile();
						FilesUtility::deleteFile(outDataPath.str().c_str());
          }
          chain.clearAllFilters(); 
          return 0;
				}	
        nbw += nbs;

				if(useChunks && chain.write("\r\n", 2, &nbw2))
				{
					if(!td->appendOutputs)
					{
						data.stdOut.closeFile();
						FilesUtility::deleteFile(outDataPath.str().c_str());
          }
          chain.clearAllFilters(); 
          return 0;
				}

			}
		}while(nbr && nbs);

		if(useChunks && chain.write("0\r\n\r\n", 5, &nbw2))
		{
			data.stdOut.closeFile();
			FilesUtility::deleteFile(outDataPath.str().c_str());
			return 0;
		}
		if(!td->appendOutputs)
		{
			data.stdOut.closeFile();
			FilesUtility::deleteFile(outDataPath.str().c_str());
		}
	}

	{
    ostringstream tmp;
    tmp <<  nbw;
    td->response.contentLength.assign(tmp.str()); 
  } 
  chain.clearAllFilters(); 
	return 1;

#endif
}


/*!
 *Map the library in the application address space.
 *\param confFile The xml parser with configuration.
 */
int MsCgi::load(XmlParser* /*confFile*/)
{
  int ret=1;
#ifdef WIN32
  ret =	mscgiModule.loadLibrary("CGI-LIB\\CGI-LIB.dll", 1);
#endif

#ifdef HAVE_DL
	ostringstream mscgi_path;
	
	if(FilesUtility::fileExists("cgi-lib/cgi-lib.so"))
	{
    mscgi_path << "cgi-lib/cgi-lib.so";
	}
	else
	{
#ifdef PREFIX
    mscgi_path << PREFIX << "/lib/myserver/cgi-lib.so";
#else
    mscgi_path << "/usr/lib/myserver/cgi-lib.so";
#endif
	}
  if(mscgi_path.str().length())
  {
    ret = mscgiModule.loadLibrary(mscgi_path.str().c_str(), 1);
  }
#endif
	return ret;
}

/*!
 *Free the memory allocated by the MSCGI library.
 */
int MsCgi::unload()
{
	/* Return 1 if the library was closed correctly returns successfully.  */
	return mscgiModule.close();
}
