/*
*MyServer
*Copyright (C) 2002, 2003, 2004 The MyServer Team
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

#include "../stdafx.h"
#include "../include/clientsThread.h"
#include "../include/cserver.h"
#include "../include/security.h"
#include "../include/Response_RequestStructs.h"
#include "../include/sockets.h"
#include "../include/stringutils.h"
#include "../include/MemBuf.h"

#ifdef NOT_WIN
extern "C" {
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include <sys/types.h>
#include <sys/wait.h>

}
#endif

/*! Define SD_BOTH in case it is not defined. */
#ifndef SD_BOTH
#define SD_BOTH 2 /*! to close TCP connection in both directions */
#endif

ClientsTHREAD::ClientsTHREAD()
{
	err=0;
	initialized=0;
}
ClientsTHREAD::~ClientsTHREAD()
{
	clean();
}
/*!
*This function starts a new thread controlled by a ClientsTHREAD class instance.
*/
#ifdef WIN32
#define ClientsTHREAD_TYPE int
unsigned int __stdcall startClientsTHREAD(void* pParam)
#endif

#ifdef HAVE_PTHREAD
#define ClientsTHREAD_TYPE void*
void * startClientsTHREAD(void* pParam)
#endif

{
#ifdef NOT_WIN
	// Block SigTerm, SigInt, and SigPipe in threads
	sigset_t sigmask;
	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGPIPE);
	sigaddset(&sigmask, SIGINT);
	sigaddset(&sigmask, SIGTERM);
	sigprocmask(SIG_SETMASK, &sigmask, NULL);
#endif
	u_long id=*((u_long*)pParam) - ClientsTHREAD::ID_OFFSET;

	ClientsTHREAD *ct=&lserver->threads[id];

  /*! Return an error if the thread is initialized. */
	if(ct->initialized)
		return (ClientsTHREAD_TYPE) -1;

	ct->threadIsRunning=1;
	ct->threadIsStopped=0;
	ct->buffersize=lserver->buffersize;
	ct->buffersize2=lserver->buffersize2;
	
	ct->buffer.SetLength(ct->buffersize);
	ct->buffer.m_nSizeLimit = ct->buffersize;
	ct->buffer2.SetLength(ct->buffersize2);
	ct->buffer2.m_nSizeLimit = ct->buffersize2;
	
	ct->http_parser = new http();
  if(ct->http_parser==0)
    return (ClientsTHREAD_TYPE)-1;

	ct->control_protocol_parser = new control_protocol();
  if(ct->control_protocol_parser == 0)
    return (ClientsTHREAD_TYPE)-1;

  ct->https_parser = new https();
	if(ct->https_parser==0)
    return (ClientsTHREAD_TYPE)-1;

	ct->initialized=1;

  /*! Reset thread buffers. */
	memset((char*)ct->buffer.GetBuffer(), 0, ct->buffer.GetRealLength());
	memset((char*)ct->buffer2.GetBuffer(), 0, ct->buffer2.GetRealLength());

	/*! Wait before go in the running loop. */
	wait(3000);

	/*!
   *This function when is alive only call the controlConnections(...) function
   *of the ClientsTHREAD class instance used for control the thread.
   */
	while(ct->threadIsRunning) 
	{
    ct->parsing = 1;
		ct->controlConnections();
    ct->parsing = 0;
		wait(1);
	}
	ct->threadIsStopped=1;
	
	myserver_thread::terminate();
	return 0;
}

/*!
 *This is the main loop of the thread.
 *Here are controlled all the connections that belongs to the 
 *ClientsTHREAD class instance.
 *Every connection is controlled by its protocol.
 */
void ClientsTHREAD::controlConnections()
{
	/*!
   *Get the access to the connections list.
   */
	lserver->connections_mutex_lock();
	LPCONNECTION c=lserver->getConnectionToParse(this->id);
	/*!
   *Check if c exists.
   *Check if c is a valid connection structure.
   *Do not parse a connection that is going to be parsed by another thread.
   */
	if((!c)  || c->isParsing())
	{
		lserver->connections_mutex_unlock();
		return;
	}
	/*!
   *Set the connection parsing flag to true.
   */
	c->setParsing(1);

	/*!
   *Unlock connections list access after setting parsing flag.
   */
	lserver->connections_mutex_unlock();

  /*!Number of bytes waiting to be read. */
  nBytesToRead=c->socket.bytesToRead();

	if(nBytesToRead || c->getForceParsing())
	{
		c->setForceParsing(0);
		if(nBytesToRead)
			err=c->socket.recv(&((char*)(buffer.GetBuffer()))[c->getDataRead()],
                         KB(8) - c->getDataRead(), 0);

    /*! Refresh with the right value. */
    nBytesToRead = c->getDataRead() + err;

		if(err==-1)
		{
			lserver->connections_mutex_lock();
			lserver->deleteConnection(c, this->id);
			lserver->connections_mutex_unlock();
			return;
		}
		if((c->getDataRead() + err)<KB(8))
		{
			((char*)buffer.GetBuffer())[c->getDataRead() + err]='\0';
		}
		else
		{
			lserver->connections_mutex_lock();
			lserver->deleteConnection(c, this->id);
			lserver->connections_mutex_unlock();
			return;
		}
		buffer.SetBuffer(c->connectionBuffer, c->getDataRead());

		/*!
     *Control the protocol used by the connection.
     */
		int retcode=0;
		c->thread=this;
    dynamic_protocol* dp=0;
		switch(((vhost*)(c->host))->protocol)
		{
			/*!
       *controlHTTPConnection returns 0 if the connection must be removed from
       *the active connections list.
       */
			case PROTOCOL_HTTP:
				retcode=http_parser->controlConnection(c, (char*)buffer.GetBuffer(), 
                     (char*)buffer2.GetBuffer(), buffer.GetRealLength(), 
                     buffer2.GetRealLength(), nBytesToRead, id);
				break;
			/*!
       *Parse an HTTPS connection request.
       */
			case PROTOCOL_HTTPS:
				retcode=https_parser->controlConnection(c, (char*)buffer.GetBuffer(), 
                     (char*)buffer2.GetBuffer(), buffer.GetRealLength(), 
                     buffer2.GetRealLength(), nBytesToRead, id);
				break;
			case PROTOCOL_CONTROL:
				retcode=control_protocol_parser->controlConnection(c, 
                     (char*)buffer.GetBuffer(), (char*)buffer2.GetBuffer(), 
                     buffer.GetRealLength(), buffer2.GetRealLength(), 
                                                           nBytesToRead, id);
				break;
			default:
        dp=lserver->getDynProtocol(((vhost*)(c->host))->protocol_name);
				if(dp==0)
				{
					retcode=0;
				}
				else
				{
					retcode=dp->controlConnection(c, (char*)buffer.GetBuffer(), 
                  (char*)buffer2.GetBuffer(), buffer.GetRealLength(), 
                  buffer2.GetRealLength(), nBytesToRead, id);
				}
				break;
		}
		/*!
     *The protocols parser functions return:
     *0 to delete the connection from the active connections list
     *1 to keep the connection active and clear the connectionBuffer
     *2 if the header is incomplete and to save it in a temporary buffer
     *3 if the header is incomplete without save it in a temporary buffer
     */
		if(retcode==0)/*Delete the connection*/
		{
			lserver->connections_mutex_lock();
			lserver->deleteConnection(c, this->id);
			lserver->connections_mutex_unlock();
			return;
		}
		else if(retcode==1)/*Keep the connection*/
		{
			c->setDataRead(0);
			c->connectionBuffer[0]='\0';
		}
		else if(retcode==2)/*Incomplete request to buffer*/
		{
			/*!
       *If the header is incomplete save the current received
       *data in the connection buffer.
       *Save the header in the connection buffer.
       */
			memcpy(c->connectionBuffer, (char*)buffer.GetBuffer(), c->getDataRead() + err);

			c->setDataRead(c->getDataRead() + err);
		}
		/*! Incomplete request bufferized by the protocol.  */
		else if(retcode == 3)
		{
			c->setForceParsing(1);
		}		
		c->setTimeout( get_ticks() );
	}
	else
	{
		/*! Reset nTries after 5 seconds.  */
		if(get_ticks() - c->getTimeout() > 5000)
			c->setnTries( 0 );

		/*!
     *If the connection is inactive for a time greater that the value
     *configured remove the connection from the connections pool
     */
		if((get_ticks()- c->getTimeout()) > lserver->connectionTimeout)
		{
			lserver->connections_mutex_lock();
			lserver->deleteConnection(c, this->id);
			lserver->connections_mutex_unlock();
			return;
		}
	}
	/*! Reset the parsing flag on the connection.  */
	c->setParsing(0);
}

/*!
 *Stop the thread.
 */
void ClientsTHREAD::stop()
{
	/*!
   *Set the run flag to False.
   *When the current thread find the threadIsRunning
   *flag setted to 0 automatically destroy the
   *thread.
   */
	threadIsRunning = 0;
}

/*!
 *Clean the memory used by the thread.
 */
void ClientsTHREAD::clean()
{
	/*! If the thread was not initialized return from the clean function.  */
	if(initialized == 0)
		return;
  /*! If the thread is parsing wait. */
  while(parsing)
  {
    wait(100);
  }
	threadIsRunning=0;
	delete http_parser;
	delete https_parser;
  delete control_protocol_parser;

	buffer.Free();
	buffer2.Free();

	initialized=0;
}


/*!
 *Returns a non-null value if the thread is active.
 */
int ClientsTHREAD::isRunning()
{
	return threadIsRunning;
}

/*!
 *Returns 1 if the thread is stopped.
 */
int ClientsTHREAD::isStopped()
{
	return threadIsStopped;
}

/*!
 *Get a pointer to the buffer.
 */
CMemBuf* ClientsTHREAD::GetBuffer()
{
	return &buffer;
}
/*!
 *Get a pointer to the buffer2.
 */
CMemBuf *ClientsTHREAD::GetBuffer2()
{
	return &buffer2;
}

