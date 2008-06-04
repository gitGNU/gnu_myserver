/*
MyServer
Copyright (C) 2007, 2008 The MyServer Team
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

#include "../include/connections_scheduler.h"
#include "../include/server.h"

////////////////////////////////FROM LIBEVENT///////////////////////////////
#ifdef WIN32
#define evutil_socket_t intptr_t
#include <windows.h>
#else
#define evutil_socket_t int
#endif

static int
create_socketpair(int af, int type, int protocol, evutil_socket_t socks[2])
{
  #ifndef WIN32
  return socketpair(af, type, protocol, socks);
  #else
   struct sockaddr_in addr;
    SOCKET listener;
    int e;
    int addrlen = sizeof(addr);
    DWORD flags = 0;

    if (socks == 0)
    {
        return -1;
    }

    socks[0] = socks[1] = INVALID_SOCKET;
    listener = socket(AF_INET, type, 0);
    if (listener == INVALID_SOCKET)
        return -1;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(0x7f000001);
    addr.sin_port = 0;

    e = bind(listener, (const struct sockaddr*) &addr, sizeof(addr));
    if (e == SOCKET_ERROR)
    {
        closesocket(listener);
        return -1;
    }
    e = getsockname(listener, (struct sockaddr*) &addr, &addrlen);
    if (e == SOCKET_ERROR)
    {
        closesocket(listener);
        return -1;
    }

    do
    {
        if (listen(listener, 1) == SOCKET_ERROR)
            break;
        if ((socks[0] = socket(AF_INET, type, 0)) == INVALID_SOCKET)
            break;
        if (connect(socks[0], (const struct sockaddr*) &addr, sizeof(addr)) == SOCKET_ERROR)
            break;
        if ((socks[1] = accept(listener, NULL, NULL)) == INVALID_SOCKET)
            break;

        closesocket(listener);
        return 0;

    } while (0);

    closesocket(listener);
    closesocket(socks[0]);
    closesocket(socks[1]);
    return -1;
  #endif
}

static int
make_socket_nonblocking(evutil_socket_t fd)
{
  #ifdef WIN32
  {
    unsigned long nonblocking = 1;
    ioctlsocket(fd, FIONBIO, (unsigned long*) &nonblocking);
  }
  #else
  if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
    return -1;
  }
  #endif
  return 0;
}
///////////////////////////////////////////////////////////////////////////

#ifdef WIN32
static unsigned int __stdcall dispatcher(void* p)
#else
static void* dispatcher(void* p)
#endif
{
  ConnectionsScheduler::DispatcherArg *da = (ConnectionsScheduler::DispatcherArg*)p;

  da->mutex->lock();
  if(!da->terminated)
  {
    da->mutex->unlock();
    return 0;
  }
  da->terminated = false;
  da->terminate = false;
  da->mutex->unlock();

  while(!da->terminate)
  {    
    int res;

    da->mutex->lock();
    res = event_loop(EVLOOP_ONCE);
    da->mutex->unlock();

    if(res == 1)
    {
      Thread::wait(10);
    }

    while(da->pause)
      Thread::wait(1);
  }
  
  da->mutex->lock();
  da->terminated = true;
  da->mutex->unlock();

  return 0;
}

static void eventLoopHandler(int fd, short event, void *arg)
{
  ConnectionsScheduler::DispatcherArg *da = (ConnectionsScheduler::DispatcherArg*)arg;
 
  if(event == EV_READ || event == EV_TIMEOUT)
  {
    char buf[256];
    Socket sock(da->fd[0]);
    
    while(sock.bytesToRead())sock.recv(buf, sizeof(buf), 0);

    event_add(&(da->loopEvent), NULL);
  }
}

static void newDataHandler(int fd, short event, void *arg)
{
  ConnectionPtr connection = static_cast<ConnectionPtr>(arg);

  if(event == EV_TIMEOUT)
  {
    Server::getInstance()->deleteConnection(connection, 0);
  }
  else if(event == EV_READ)
  {
    Server::getInstance()->getConnectionsScheduler()->addReadyConnection(connection);
  }
}

static void listenerHandler(int fd, short event, void *arg)
{
  static timeval tv = {5, 0};
  ConnectionsScheduler::ListenerArg* s = (ConnectionsScheduler::ListenerArg*)arg;

  if(event == EV_TIMEOUT)
  {
    event_add (&(s->ev), &tv);
  }
  else if(event == EV_READ)
  {
    MYSERVER_SOCKADDRIN asockIn;
    int asockInLen = 0;
    Socket asock;

    asockInLen = sizeof(sockaddr_in);
    asock = s->serverSocket->accept(&asockIn, &asockInLen);
    if(asock.getHandle() != 0 &&
       asock.getHandle() != (SocketHandle)INVALID_SOCKET)
    {
      Server::getInstance()->addConnection(asock, &asockIn);
    }

    event_add (&(s->ev), &tv);
  }
}

/*!
 *Add a listener socket to the event queue.
 *This is used to renew the event after the listener thread is notified.
 *
 *\param sock Listening socket.
 *\param la Structure containing an Event to be notified on new data.
 */
void ConnectionsScheduler::listener(ConnectionsScheduler::ListenerArg *la)
{
  ConnectionsScheduler::ListenerArg *arg = new ConnectionsScheduler::ListenerArg(la);
  static timeval tv = {3, 0};

  event_set(&(arg->ev), la->serverSocket->getHandle(), EV_READ | EV_TIMEOUT, 
            listenerHandler, arg);

  arg->terminate = &dispatcherArg.terminate;
  arg->scheduler = this;
  arg->eventsMutex = &eventsMutex;
  la->serverSocket->setNonBlocking(1);

  listeners.push_front(arg);

  event_add(&(arg->ev), &tv);

  u_long nbw;
  Socket sock(dispatcherArg.fd[1]);
  sock.write("a", 1, &nbw);  
}

/*!
 *Remove a listener thread from the list.
 */
void ConnectionsScheduler::removeListener(ConnectionsScheduler::ListenerArg* la)
{
  eventsMutex.lock();
  event_del(&(la->ev));
  listeners.remove(la);
  eventsMutex.unlock();
}

/*!
 *C'tor.
 */
ConnectionsScheduler::ConnectionsScheduler()
{
  readyMutex.init();
  eventsMutex.init();
  connectionsMutex.init();
  readySemaphore = new Semaphore(0);
  currentPriority = 0;
  currentPriorityDone = 0;
  nTotalConnections = 0;
  ready = new queue<ConnectionPtr>[PRIORITY_CLASSES];
}

/*!
 *Get the number of all connections made to the server.
 */
u_long ConnectionsScheduler::getNumTotalConnections()
{
  return nTotalConnections;
}


/*!
 *Register the connection with a new ID.
 *\param connection The connection to register.
 */
void ConnectionsScheduler::registerConnectionID(ConnectionPtr connection)
{
  connectionsMutex.lock();
  connection->setID(nTotalConnections++);
  connectionsMutex.unlock();
}

/*!
 *Restart the scheduler.
 */
void ConnectionsScheduler::restart()
{
  readyMutex.init();
  connectionsMutex.init();
  eventsMutex.init();
  listeners.clear();

  if(readySemaphore)
    delete readySemaphore;

  readySemaphore = new Semaphore(0);

  initialize();
}

/*!
 *Static initialization.
 */
void ConnectionsScheduler::initialize()
{
  static timeval tv = {1, 0};

   event_init();

  dispatcherArg.terminated = true;
  dispatcherArg.mutex = &eventsMutex;

#ifdef WIN32
#define LOCAL_SOCKETPAIR_AF AF_INET
#else
#define LOCAL_SOCKETPAIR_AF AF_UNIX
#endif
  int err = create_socketpair(LOCAL_SOCKETPAIR_AF, SOCK_STREAM, 0,
			dispatcherArg.fd);
  if (err == -1)
  {
    Server::getInstance()->logLockAccess();
    Server::getInstance()->logPreparePrintError();
    Server::getInstance()->logWriteln("Error initializing socket pair.");
    Server::getInstance()->logEndPrintError();
    Server::getInstance()->logUnlockAccess();
    return;
  }

  make_socket_nonblocking(dispatcherArg.fd[0]);
  make_socket_nonblocking(dispatcherArg.fd[1]);

  event_set(&(dispatcherArg.loopEvent), dispatcherArg.fd[0], EV_READ | EV_TIMEOUT,
	       eventLoopHandler, &dispatcherArg);

  event_add(&(dispatcherArg.loopEvent), NULL);

  if(Thread::create(&dispatchedThreadId, dispatcher, &dispatcherArg))
   {
     Server::getInstance()->logLockAccess();
     Server::getInstance()->logPreparePrintError();
     Server::getInstance()->logWriteln("Error initializing dispatcher thread.");
     Server::getInstance()->logEndPrintError();
     Server::getInstance()->logUnlockAccess();
     dispatchedThreadId = 0;
   }

  dispatcherArg.pause = false;
  releasing = false;
}

/*!
 *D'tor.
 */
ConnectionsScheduler::~ConnectionsScheduler()
{
  readyMutex.destroy();
  eventsMutex.destroy();
  connectionsMutex.destroy();
  delete readySemaphore;
  delete [] ready;
}


/*!
 *Add an existent connection to ready connections queue.
 */
void ConnectionsScheduler::addReadyConnection(ConnectionPtr c)
{
  addReadyConnectionImpl(c);
}

/*!
 *Add a new connection to ready connections queue.
 */
void ConnectionsScheduler::addNewReadyConnection(ConnectionPtr c)
{
  addReadyConnectionImpl(c);
}


/*!
 *Add a connection to ready connections queue.
 */
void ConnectionsScheduler::addReadyConnectionImpl(ConnectionPtr c)
{
  int priority = c->getPriority();

  if(priority == -1 && c->host)
      priority = c->host->getDefaultPriority();
    
  priority = std::max(0, priority);
  priority = std::min(PRIORITY_CLASSES - 1, priority);

  c->setScheduled(1);

  readyMutex.lock();
  ready[priority].push(c);
  readyMutex.unlock();

  Server::getInstance()->checkThreadsNumber();

  readySemaphore->unlock();
}

/*!
 *Add a new connection to the scheduler.
 */
void ConnectionsScheduler::addNewWaitingConnection(ConnectionPtr c)
{
  addWaitingConnectionImpl(c, 0);
}

/*!
 *Reschedule a connection in the scheduler.
 */
void ConnectionsScheduler::addWaitingConnection(ConnectionPtr c)
{
  addWaitingConnectionImpl(c, 1);
}

/*!
 *Implementation to add a connection to waiting connections queue.
 */
void ConnectionsScheduler::addWaitingConnectionImpl(ConnectionPtr c, int lock)
{
  static timeval tv = {10, 0};
  SocketHandle handle = c->socket->getHandle();

  tv.tv_sec = Server::getInstance()->getTimeout() / 1000;
  c->setScheduled(0);

  connectionsMutex.lock();
  connections.put(handle, c);
  connectionsMutex.unlock();

  if(lock)
  {
    u_long nbw;
    Socket sock(dispatcherArg.fd[1]);

    dispatcherArg.pause = true;

    sock.write("a", 1, &nbw);

    eventsMutex.lock();
  }

  event_once(handle, EV_READ | EV_TIMEOUT, newDataHandler, c, &tv);

  if(lock)
  {
    dispatcherArg.pause = false;
    eventsMutex.unlock();
  }
}

/*!
 *Get a connection from the active connections queue.
 */
ConnectionPtr ConnectionsScheduler::getConnection()
{
  ConnectionPtr ret = 0;

  if(releasing)
    return NULL;

  readySemaphore->lock();

  if(releasing)
    return NULL;

  readyMutex.lock();

  for(int i = 0; i < PRIORITY_CLASSES; i++)
  {
    if(currentPriorityDone > currentPriority ||
       !ready[currentPriority].size())
    {
      currentPriority = (currentPriority + 1) % PRIORITY_CLASSES;
      currentPriorityDone = 0;
    }

    if(ready[currentPriority].size())
    {
      SocketHandle handle;

      ret = ready[currentPriority].front();
      ret->setScheduled(0);
      ready[currentPriority].pop();
      currentPriorityDone++;
      break;
    }
  }

  readyMutex.unlock();

  return ret;
}

/*!
 *Release all the blocking calls.
 */
void ConnectionsScheduler::release()
{
  releasing = true;
  dispatcherArg.terminate = true;

  for(u_long i = 0; i < Server::getInstance()->getNumThreads()*10; i++)
  {
    readySemaphore->unlock();
  }

  event_loopexit(NULL);
#if EVENT_LOOPBREAK | WIN32
   event_loopbreak();
#endif
   u_long nbw;
   Socket sock(dispatcherArg.fd[1]);
   sock.write("a", 1, &nbw);

  if(dispatchedThreadId)
    Thread::join(dispatchedThreadId);

  eventsMutex.lock();

  list<ListenerArg*>::iterator it = listeners.begin();

  while(it != listeners.end())
  {
    event_del(&((*it)->ev));
    delete (*it);
    it++;
  }

  event_del(&(dispatcherArg.loopEvent));

  close(dispatcherArg.fd[0]);
  close(dispatcherArg.fd[1]);

  listeners.clear();
  
  eventsMutex.unlock();

  terminateConnections();
}

/*!
 *Fullfill a list with all the connections.
 *\param out A list that will receive all the connections alive on the
 *server.
 */
void ConnectionsScheduler::getConnections(list<ConnectionPtr> &out)
{
  out.clear();

  connectionsMutex.lock();

  HashMap<SocketHandle, ConnectionPtr>::Iterator it = connections.begin();
  for(; it != connections.end(); it++)
    out.push_back(*it);

  connectionsMutex.unlock();
}

/*!
 *Get the alive connections number.
 */
int ConnectionsScheduler::getConnectionsNumber()
{
  return connections.size();
}

/*!
 *Remove a connection from the connections set.
 */
void ConnectionsScheduler::removeConnection(ConnectionPtr connection)
{
  connectionsMutex.lock();
  connections.remove(connection->socket->getHandle());
  connectionsMutex.unlock();
}

/*!
 *Terminate any active connection.
 */
void ConnectionsScheduler::terminateConnections()
{
  int i;
  try
  {
    connectionsMutex.lock();

    HashMap<SocketHandle, ConnectionPtr>::Iterator it = connections.begin();
    for(; it != connections.end(); it++)
    {
      if ( (*it)->allowDelete(true) )
        (*it)->socket->closesocket();
    }
  }
  catch(...)
  {
    connectionsMutex.unlock();
    throw;
  };

  connections.clear();

  connectionsMutex.unlock();

  readyMutex.lock();

  for(i = 0; i < PRIORITY_CLASSES; i++)
    while(ready[i].size())
      ready[i].pop();

  readyMutex.unlock();
}

/*!
 *Accept a visitor on the connections.
 */
int ConnectionsScheduler::accept(ConnectionsSchedulerVisitor* visitor, void* args)
{
  int ret = 0;
  connectionsMutex.lock();

  try
  {

    for(HashMap<SocketHandle, ConnectionPtr>::Iterator it = connections.begin(); 
        it != connections.end()  && !ret; 
        it++)
    {
      visitor->visitConnection(*it, args);
    }
  }
  catch(...)
  {
    connectionsMutex.unlock();
    return 1;
  }

  connectionsMutex.unlock();

  return ret;

}
