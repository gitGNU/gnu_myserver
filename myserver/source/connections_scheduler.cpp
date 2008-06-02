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
  }
  
  da->mutex->lock();
  da->terminated = true;
  da->mutex->unlock();

  return 0;
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

    //    s->eventsMutex->lock();
    event_add (&(s->ev), &tv);
    //s->eventsMutex->unlock();
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

    //s->eventsMutex->lock();
    event_add (&(s->ev), &tv);
    // s->eventsMutex->unlock();
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

  // eventsMutex.lock();
  event_add(&(arg->ev), &tv);
  //eventsMutex.unlock();
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

  if(Thread::create(&dispatchedThreadId, dispatcher, &dispatcherArg))
   {
     Server::getInstance()->logLockAccess();
     Server::getInstance()->logPreparePrintError();
     Server::getInstance()->logWriteln("Error initializing dispatcher thread.");
     Server::getInstance()->logEndPrintError();
     Server::getInstance()->logUnlockAccess();
     dispatchedThreadId = 0;
   }

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
 *Add a connection to ready connections queue.
 */
void ConnectionsScheduler::addReadyConnection(ConnectionPtr c)
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
 *Add a connection to waiting connections queue.
 */
void ConnectionsScheduler::addWaitingConnection(ConnectionPtr c, int lock)
{
  static timeval tv = {10, 0};
  SocketHandle handle = c->socket->getHandle();

  tv.tv_sec = Server::getInstance()->getTimeout() / 1000;
  c->setScheduled(0);

  connectionsMutex.lock();
  connections.put(handle, c);
  connectionsMutex.unlock();

  if(lock)
    eventsMutex.lock();

  event_once(handle, EV_READ | EV_TIMEOUT, newDataHandler, c, &tv);

  if(lock)
    eventsMutex.unlock();
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
