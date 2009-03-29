/*
MyServer
Copyright (C) 2002, 2003, 2004, 2006, 2007, 2008 Free Software Foundation, Inc.
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "stdafx.h"
#include <include/server/clients_thread.h>

#include <include/base/thread/thread.h>
#include <include/server/server.h>
#include <include/base/socket/socket.h>
#include <include/base/string/stringutils.h>

#include <include/protocol/http/http.h>
#include <include/base/mem_buff/mem_buff.h>
#include <include/protocol/https/https.h>
#include <include/protocol/control/control_protocol.h>
#include <include/protocol/ftp/ftp.h>

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

/*!
 *Construct the object.
 */
ClientsThread::ClientsThread(Server* server)
{
  this->server = server;
  busy = 0;
  initialized = 0;
  toDestroy = 0;
  staticThread = 0;
  nBytesToRead = 0;

  httpParser = 0;
  httpsParser = 0;
  controlProtocolParser = 0;
  ftpParser = 0;
}

/*!
 *Destroy the ClientsThread object.
 */
ClientsThread::~ClientsThread()
{
  if(initialized == 0)
    return;

  threadIsRunning = 0;

  if(httpParser)
    delete httpParser;

  if(httpsParser)
    delete httpsParser;

  if(controlProtocolParser)
    delete controlProtocolParser;

  if ( ftpParser != NULL )
    delete ftpParser;

  httpParser = 0;
  httpsParser = 0;
  controlProtocolParser = 0;

  buffer.free();
  secondaryBuffer.free();
}

/*!
 *Get the timeout value.
 */
int ClientsThread::getTimeout()
{
  return timeout;
}

/*!
 *Set the timeout value for the thread.
 *\param newTimeout The new timeout value.
 */
void ClientsThread::setTimeout(int newTimeout)
{
  timeout = newTimeout;
}

/*!
 *This function starts a new thread controlled by a ClientsThread 
 *class instance.
 *\param pParam Params to pass to the new thread.
 */
#ifdef WIN32
#define ClientsThread_TYPE int
unsigned int __stdcall clients_thread(void* pParam)
#endif

#ifdef HAVE_PTHREAD
#define ClientsThread_TYPE void*
void* clients_thread(void* pParam)
#endif

{
#ifdef NOT_WIN
  /* Block SigTerm, SigInt, and SigPipe in threads.  */
  sigset_t sigmask;
  sigemptyset(&sigmask);
  sigaddset(&sigmask, SIGPIPE);
  sigaddset(&sigmask, SIGINT);
  sigaddset(&sigmask, SIGTERM);
  sigprocmask(SIG_SETMASK, &sigmask, NULL);
#endif
  ClientsThread *ct = (ClientsThread*)pParam;

  /* Return an error if the thread is initialized.  */
  if(ct->initialized)
#ifdef WIN32
    return 1;
#endif
#ifdef HAVE_PTHREAD
  return (void*)1;
#endif

  ct->threadIsRunning = 1;
  ct->threadIsStopped = 0;
  ct->buffersize = ct->server->getBuffersize();
  ct->secondaryBufferSize = ct->server->getBuffersize2();
  
  ct->buffer.setLength(ct->buffersize);
  ct->buffer.m_nSizeLimit = ct->buffersize;
  ct->secondaryBuffer.setLength(ct->secondaryBufferSize);
  ct->secondaryBuffer.m_nSizeLimit = ct->secondaryBufferSize;

  /* Built-in protocols will be initialized at the first use.  */
  ct->httpParser = 0;
  ct->httpsParser = 0;
  ct->controlProtocolParser = 0;

  ct->initialized = 1;

  ct->server->increaseFreeThread();

  /* Wait that the server is ready before go in the running loop.  */
  while(!ct->server->isServerReady() && ct->threadIsRunning)
  {
    Thread::wait(500);
  }

  /*
   *This function when is alive only call the controlConnections(...) function
   *of the ClientsThread class instance used for control the thread.
   */
  while(ct->threadIsRunning) 
  {
    int ret;
    try
    {
      /*
       *If the thread can be destroyed don't use it.
       */
      if((!ct->isStatic()) && ct->isToDestroy())
      {
        Thread::wait(1000);
        continue;
      }

      ret = ct->controlConnections();
      ct->server->increaseFreeThread();
      ct->busy = 0;

      /*
       *The thread served the connection, so update the timeout value.
       */
      if(ret != 1)
      {
        ct->setTimeout(getTicks());
      }
    }
    catch( bad_alloc &ba)
    {
      ostringstream s;
      s << "Bad alloc: " << ba.what();
      
      ct->server->logWriteln(s.str().c_str(), MYSERVER_LOG_MSG_ERROR);
    }
    catch( exception &e)
    {
      ostringstream s;
      s << "Error: " << e.what();

      ct->server->logWriteln(s.str().c_str(), MYSERVER_LOG_MSG_ERROR);
    };
    
  }

  ct->server->decreaseFreeThread();

  delete ct;

  Thread::terminate();
  return 0;
}

/*!
 *Join the thread.
 *
 */
int ClientsThread::join()
{
  return Thread::join(tid);
}


/*!
 *Create the new thread.
 */
int ClientsThread::run()
{
  tid = 0;
  return Thread::create(&tid, &::clients_thread,
                        (void *)this);
}

/*!
 *Returns if the thread can be destroyed.
 */
int ClientsThread::isToDestroy()
{
  return toDestroy;
}

/*!
 *Check if the thread is a static one.
 */
int ClientsThread::isStatic()
{
  return staticThread;
}

/*!
 *Set the thread to be static.
 *\param value The new static value.
 */
void ClientsThread::setStatic(int value)
{
  staticThread = value;
}

/*!
 *Set if the thread can be destroyed.
 *\param value The new destroy value.
 */
void ClientsThread::setToDestroy(int value)
{
  toDestroy = value;
}

/*!
 *This is the main loop of the thread.
 *Here are controlled all the connections that belongs to the 
 *ClientsThread class instance.
 *Every connection is controlled by its protocol.
 *Return 1 if no connections to serve are available.
 *Return 0 in all other cases.
 */
int ClientsThread::controlConnections()
{
  /*
   *Control the protocol used by the connection.
   */
  int retcode = 0;
  int err = 0;
  ConnectionPtr c;
  Protocol* protocol = 0;
  u_long dataRead = 0;

  c = server->getConnection(this->id);

  server->decreaseFreeThread();


  /*
   *Check if c is a valid connection structure.
   */
  if(!c)
    return 1;

  busy = 1;
  dataRead = c->connectionBuffer.getLength();

  bool bSocketChanged = false;
  if ( strcmp(c->host->getProtocolName(), "ftp") == 0 )
    {
      FileHandle sh = c->socket->getHandle();
      int flags = fcntl((int)sh, F_GETFL, 0);
      if ( (flags >= 0) && ((flags & O_NONBLOCK) == 0) )
	{
	  c->socket->setNonBlocking(1);
	  bSocketChanged = true;
	}
    }
  err = c->socket->recv(&((char*)(buffer.getBuffer()))[dataRead],
                        MYSERVER_KB(8) - dataRead - 1, 0);

  if(err == -1 && !server->deleteConnection(c))
    return 0;

  if ( bSocketChanged )
       c->socket->setNonBlocking(0);

  buffer.setLength(dataRead + err);    

  c->setForceControl(0);


  /* Refresh with the right value.  */
  nBytesToRead = dataRead + err;

  if((dataRead + err) < MYSERVER_KB(8))
  {
    ((char*)buffer.getBuffer())[dataRead + err] = '\0';
  }
  else
  {
    server->deleteConnection(c);
    return 0;
  }


  if(getTicks() - c->getTimeout() > 5000)
    c->setnTries(0);

  if(dataRead)
  {
    memcpy((char*)buffer.getBuffer(), c->connectionBuffer, dataRead);
  }

  c->setActiveThread(this);
  try
  {
    if (c->hasContinuation())
    {
      retcode = c->getContinuation()(c, 
                                     (char*)buffer.getBuffer(), 
                                     (char*)secondaryBuffer.getBuffer(), 
                                     buffer.getRealLength(), 
                                     secondaryBuffer.getRealLength(), 
                                     nBytesToRead, 
                                     id);
      c->setContinuation(NULL);
    }
    else
    {
      protocol = server->getProtocol(c->host->getProtocolName());
      if(protocol)
      {
        retcode = protocol->controlConnection(c, 
                                              (char*)buffer.getBuffer(), 
                                              (char*)secondaryBuffer.getBuffer(), 
                                              buffer.getRealLength(), 
                                              secondaryBuffer.getRealLength(), 
                                              nBytesToRead, 
                                              id);
      }
      else
        retcode = DELETE_CONNECTION;
    }
  }
  catch(...)
  {
    retcode = DELETE_CONNECTION;
  };

  c->setTimeout( getTicks() );

  /*! Delete the connection.  */
  if(retcode == DELETE_CONNECTION)
  {
    server->deleteConnection(c);
    return 0;
  }
  /*! Keep the connection.  */
  else if(retcode == KEEP_CONNECTION)
  {
    c->connectionBuffer.setLength(0);
    server->getConnectionsScheduler()->addWaitingConnection(c);
  }
  /*! Incomplete request to buffer.  */
  else if(retcode == INCOMPLETE_REQUEST)
  {
    /*
     *If the header is incomplete save the current received
     *data in the connection buffer.
     *Save the header in the connection buffer.
     */
    c->connectionBuffer.setBuffer(buffer.getBuffer(), nBytesToRead);
    server->getConnectionsScheduler()->addWaitingConnection(c);
  }
  /* Incomplete request to check before new data is available.  */
  else if(retcode == INCOMPLETE_REQUEST_NO_WAIT)
  {
    c->setForceControl(1);
    server->getConnectionsScheduler()->addReadyConnection(c);
  }    

  return 0;
}

/*!
 *Stop the thread.
 */
void ClientsThread::stop()
{
  /*
   *Set the run flag to False.
   *When the current thread find the threadIsRunning
   *flag setted to 0 automatically destroy the
   *thread.
   */
  threadIsRunning = 0;
}

/*!
 *Returns a non-null value if the thread is active.
 */
int ClientsThread::isRunning()
{
  return threadIsRunning;
}

/*!
 *Returns 1 if the thread is stopped.
 */
int ClientsThread::isStopped()
{
  return threadIsStopped;
}

/*!
 *Get a pointer to the buffer.
 */
MemBuf* ClientsThread::getBuffer()
{
  return &buffer;
}
/*!
 *Get a pointer to the secondaryBuffer.
 */
MemBuf *ClientsThread::getSecondaryBuffer()
{
  return &secondaryBuffer;
}

/*!
 *Check if the thread is working.
 */
int ClientsThread::isBusy()
{
  return busy;
}
