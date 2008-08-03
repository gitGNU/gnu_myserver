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


#include "../stdafx.h"
#include "../include/utility.h"
#include "../include/socket.h"
extern "C" {
#include <string.h>
#include <stdio.h>
#ifdef WIN32
#include <Ws2tcpip.h>
#endif
#ifndef WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
}

#include <sstream>

using namespace std;

#ifdef WIN32
#pragma comment(lib,"wsock32.lib")
#pragma comment(lib,"ws2_32.lib")
#endif

bool Socket::denyBlockingOperations = false;


/*!
 *Source code to wrap the socket library to MyServer project.
 */
int startupSocketLib(u_short ver)
{
#ifdef WIN32
  /*!
   *Under windows we need to initialize the socket library before use it.
   */
  WSADATA wsaData;
  return WSAStartup(ver, &wsaData);
#else
  return 0;
#endif
}

/*!
 *Returns the socket handle
 */
SocketHandle Socket::getHandle()
{
  return socketHandle;
}

/*!
 *Set the handle for the socket
 */
int Socket::setHandle(SocketHandle h)
{
  socketHandle = h;
  return 1;
}

/*!
 *Check if the two sockets have the same handle descriptor
 */
int Socket::operator==(Socket s)
{
  return socketHandle == s.socketHandle;
}

/*!
 *Set the socket using the = operator
 */
int Socket::operator=(Socket s)
{
  /*! Do a raw memory copy.  */
  memcpy(this, &s, sizeof(s));
  return 1;
}
/*!
 *Create the socket.
 */
int Socket::socket(int af, int type, int protocol)
{
  socketHandle = (SocketHandle)::socket(af, type, protocol);
  return  (int)socketHandle;
}

/*!
 *Set the socket handle.
 */
Socket::Socket(SocketHandle handle)
{
  throttlingRate = 0;
  setHandle(handle);
}

/*!
 *Set the socket handle.
 */
Socket::Socket(Socket* socket)
{
  socketHandle = socket->socketHandle;
  serverSocket = socket->serverSocket;
  throttlingRate = socket->throttlingRate;
}

/*!
 *Return the throttling rate(bytes/second) used by the socket. A return value
 *of zero means that no throttling is used.
 */
u_long Socket::getThrottling()
{
  return throttlingRate;
}

/*!
 *Set the throttling rate(bytes/second) for the socket.
 *Use a zero rate to disable throttling.
 */
void Socket::setThrottling(u_long tr)
{
  throttlingRate = tr;
}

/*!
 *Constructor of the class.
 */
Socket::Socket()
{
  /*! Reset everything.  */
  throttlingRate = 0;
  serverSocket = 0;
  setHandle(0);
}

/*!
 *Bind the port to the socket.
 */
int Socket::bind(MYSERVER_SOCKADDR* sa,int namelen)
{
   if ( sa == NULL || (sa->ss_family != AF_INET && sa->ss_family != AF_INET6) )
      return 1;//Andu: TODO our error code or what?
  if ( (sa->ss_family == AF_INET && namelen != sizeof(sockaddr_in)) || 
  (sa->ss_family == AF_INET6 && namelen != sizeof(sockaddr_in6)) )
     return 1;//Andu: TODO our error code or what?
#ifdef WIN32
  return ::bind((SOCKET)socketHandle,(const struct sockaddr*)sa,namelen);
#endif
#ifdef NOT_WIN
  return ::bind((int)socketHandle,(const struct sockaddr*)sa,namelen);
#endif
}

/*!
 *Listen for other connections.
 */
int Socket::listen(int max)
{
#ifdef WIN32
  return ::listen(socketHandle,max);
#endif
#ifdef NOT_WIN
  return ::listen((int)socketHandle,max);
#endif
}

/*!
 *Accept a new connection.
 */
Socket Socket::accept(MYSERVER_SOCKADDR* sa, int* sockaddrlen)
{
   if ( sa == NULL )
      return 1;//Andu: TODO our error code or what?
#ifdef NOT_WIN
  socklen_t connectSize;
  int acceptHandle;
#endif

  Socket s;

#ifdef WIN32
  SocketHandle h = (SocketHandle)::accept(socketHandle,(struct sockaddr*)sa,
                                          sockaddrlen);
  s.setHandle(h);
#endif

#ifdef NOT_WIN
  connectSize = (socklen_t) *sockaddrlen;

  acceptHandle = ::accept((int)socketHandle, (struct sockaddr *)sa,
                           (socklen_t*)&connectSize);
  s.setHandle(acceptHandle);
#endif

  return s;
}

/*!
 *Close the socket.
 */
int Socket::closesocket()
{
#ifdef WIN32
  if(socketHandle)
  {
    int ret = ::closesocket(socketHandle);
    socketHandle = 0;
    return ret;
  }
  else
    return 0;
#endif

#ifdef NOT_WIN
  if(socketHandle)
  {
    int ret = ::close((int)socketHandle);
    socketHandle = 0;
    return ret;
  }
  else
    return 0;
#endif
}

/*!
 *Returns an host by its address.
 */
MYSERVER_HOSTENT *Socket::gethostbyaddr(char* addr,int len,int type)
{
#ifdef WIN32
  HOSTENT *he = ::gethostbyaddr(addr,len,type);
#endif
#ifdef NOT_WIN
  struct hostent * he = ::gethostbyaddr(addr,len,type);
#endif
  return he;
}

/*!
*Returns an host by its name
*/
MYSERVER_HOSTENT *Socket::gethostbyname(const char *hostname)
{
  return (MYSERVER_HOSTENT *)::gethostbyname(hostname);
}

/*!
 *Shutdown the socket.
 */
int Socket::shutdown(int how)
{
#ifdef WIN32
  return ::shutdown(socketHandle,how);
#endif

#ifdef NOT_WIN
  return ::shutdown((int)socketHandle,how);
#endif
}

/*!
 *Set socket options.
 */
int  Socket::setsockopt(int level, int optname,
                       const char *optval, int optlen)
{
  return ::setsockopt(socketHandle, level, optname, optval, optlen);
}

/*!
 *Fill the out string with a list of the local IPs. Returns 0 on success.
 */
int Socket::getLocalIPsList(string &out)
{
  char serverName[HOST_NAME_MAX + 1];
  memset(serverName, 0, HOST_NAME_MAX + 1);

  Socket::gethostname(serverName, HOST_NAME_MAX);
#if ( HAVE_IPV6 )
  addrinfo aiHints = { 0 }, *pHostInfo = NULL, *pCrtHostInfo = NULL;
  /* only interested in socket types the that server will listen to.  */
  aiHints.ai_socktype = SOCK_STREAM;
  if ( getaddrinfo(serverName, NULL, &aiHints, &pHostInfo) == 0 && 
       pHostInfo != NULL )
  {
    sockaddr_storage *pCurrentSockAddr = NULL;
    char straddr[NI_MAXHOST] = "";
    memset(straddr, 0, NI_MAXHOST);
    ostringstream stream;
    for ( pCrtHostInfo = pHostInfo; pCrtHostInfo != NULL; 
          pCrtHostInfo = pCrtHostInfo->ai_next )
    {
      pCurrentSockAddr = 
        reinterpret_cast<sockaddr_storage *>(pCrtHostInfo->ai_addr);
      if ( pCurrentSockAddr == NULL )
        continue;

            if ( !getnameinfo(reinterpret_cast<sockaddr *>(pCurrentSockAddr), 
                              sizeof(sockaddr_storage), straddr, NI_MAXHOST, 
                              NULL, 0, NI_NUMERICHOST) )
            {
              stream << ( !stream.str().empty() ? ", " : "" ) << straddr;
            }
            else
        return -1;
        }
        out.assign(stream.str());
        freeaddrinfo(pHostInfo);
    }
    return 0;
#else// !HAVE_IPV6

  MYSERVER_HOSTENT *localhe;
  in_addr ia;
  localhe = Socket::gethostbyname(serverName);

  if(localhe)
  {
    ostringstream stream;
    int i;
    /*! Print all the interfaces IPs. */
    for(i= 0; (localhe->h_addr_list[i]); i++)
    {
#ifdef WIN32
      ia.S_un.S_addr = *((u_long FAR*) (localhe->h_addr_list[i]));
#endif
#ifdef NOT_WIN
      ia.s_addr = *((u_long *) (localhe->h_addr_list[i]));
#endif
      stream << ( (i != 0) ? ", " : "") << inet_ntoa(ia);
    }

    out.assign(stream.str());
    return 0;
  }
  else
  {
    out.assign("127.0.0.1");
    return 0;
  }
#endif//HAVE_IPV6
  return -1;
}

/*!
 *Set this to true to stop any operation that can block sockets.
 */
void Socket::stopBlockingOperations(bool value)
{
  denyBlockingOperations = value;
}

/*!
 *Send data over the socket.
 *Return -1 on error.
 *This routine is accessible only from the Socket class.
 */
int Socket::rawSend(const char* buffer, int len, int flags)
{
#ifdef WIN32
  int ret;
  SetLastError(0);
  while(!denyBlockingOperations)
  {
    ret = ::send(socketHandle, buffer, len, flags);
    if((ret == SOCKET_ERROR) && (GetLastError() == WSAEWOULDBLOCK))
    {
      Thread::wait(10);
    }
    else
      break;

  }
  return ret;
#else
  return ::send((int)socketHandle, buffer, len, flags);
#endif
}

/*!
 *Send data over the socket.
 *Returns -1 on error.
 *Returns the number of bytes sent on success.
 *If a throttling rate is specified, send will use it.
 */
int Socket::send(const char* buffer, int len, int flags)
{
  u_long toSend = (u_long) len;
  int ret;
  /*! If no throttling is specified, send only one big data chunk. */
  if(throttlingRate == 0)
  {
    ret = rawSend(buffer, len, flags);
    return ret;
  }
  else
  {
    while(!denyBlockingOperations)
    {
      /*! When we can send data again?  */
      u_long time = getTicks() + (1000 * 1024 / throttlingRate) ;
      /*! If a throttling rate is specified, send chunks of 1024 bytes.  */
      ret = rawSend(buffer + (len - toSend), toSend < 1024 ? 
                    toSend : 1024, flags);
      /*! On errors returns directly -1.  */
      if(ret < 0)
        return -1;
      toSend -= (u_long)ret;
      /*! 
       *If there are other bytes to send wait before cycle again. 
       */
      if(toSend)
      {
        while((getTicks() <= time) && !denyBlockingOperations)
          Thread::wait(1);
      }
      else
        break;
    }
    /*! Return the number of sent bytes. */
    return len - toSend;
  }
  return 0;
}

/*!
 *Function used to control the socket.
 */
int Socket::ioctlsocket(long cmd,unsigned long* argp)
{
#ifdef WIN32
  return ::ioctlsocket(socketHandle,cmd,argp);
#endif
#ifdef NOT_WIN
  int int_argp = 0;
  int ret = ::ioctl((int)socketHandle,cmd,&int_argp);
  *argp = int_argp;
  return ret;

#endif
}

/*!
 *Connect to the specified host:port.
 *Returns zero on success.
 */
int Socket::connect(const char* host, u_short port)
{
  if ( host == NULL )
    return -1;
    
#if ( HAVE_IPV6 )
  MYSERVER_SOCKADDRIN thisSock = { 0 };
  int nLength = sizeof(MYSERVER_SOCKADDRIN);
  int nSockLen = 0;
  bool bSocketConnected = false;
  int nGetaddrinfoRet = 0;
  addrinfo *pHostInfo = NULL, *pCrtHostInfo = NULL;
  MYSERVER_SOCKADDRIN *pCurrentSockAddr = NULL, connectionSockAddrIn = { 0 };
  char szPort[10];

  addrinfo aiHints = { 0 };

  /*
   *If the socket is not created, wait to see what address families 
   *have this host, than create socket. 
   */
  
  //getsockname(&thisSock, &nLength); 
  if ( getsockname(&thisSock, &nLength) == 0 )
  {
    aiHints.ai_family = thisSock.ss_family;
    aiHints.ai_socktype = SOCK_STREAM;
  }

  memset(szPort, 0, sizeof(char)*10);
  snprintf(szPort, 10, "%d", port);

  if (aiHints.ai_family != 0)
    nGetaddrinfoRet = getaddrinfo(host, NULL, &aiHints, &pHostInfo);
  else
    nGetaddrinfoRet = getaddrinfo(host, NULL, NULL, &pHostInfo);
  if (nGetaddrinfoRet != 0 || pHostInfo == NULL )
    return -1;

  for ( pCrtHostInfo = pHostInfo; pCrtHostInfo != NULL; 
        pCrtHostInfo = pCrtHostInfo->ai_next )
  {
    pCurrentSockAddr = reinterpret_cast<sockaddr_storage *>
      (pCrtHostInfo->ai_addr);
    if ( pCurrentSockAddr == NULL || 
        (thisSock.ss_family != 0 && 
         pCurrentSockAddr->ss_family != thisSock.ss_family) )
      continue;

    if ( thisSock.ss_family == 0 )// socket not created yet
    {
      //so try to create one now based on 'pCurrentSockAddr' 
      if ( pCurrentSockAddr->ss_family == AF_INET )
      {
        if(Socket::socket(AF_INET, SOCK_STREAM, 0) == -1)
          continue;
      }
      else if ( pCurrentSockAddr->ss_family == AF_INET6 )
      {
        if(Socket::socket(AF_INET6, SOCK_STREAM, 0) == -1)
          continue;
      }
      if ( getsockname(&thisSock, &nLength) != 0 )
        return -1;
    }

    memset(&connectionSockAddrIn, 0, sizeof(connectionSockAddrIn));
    connectionSockAddrIn.ss_family = pCurrentSockAddr->ss_family;
  
     if ( connectionSockAddrIn.ss_family != AF_INET && 
         connectionSockAddrIn.ss_family != AF_INET6 )
         continue;
         
    if ( connectionSockAddrIn.ss_family == AF_INET )
    {
      nSockLen = sizeof(sockaddr_in);
      memcpy((sockaddr_in *)&connectionSockAddrIn, 
             (sockaddr_in *)pCurrentSockAddr, nSockLen);
      ((sockaddr_in *)(&connectionSockAddrIn))->sin_port = htons(port);

    }
    else// if ( connectionSockAddrIn.ss_family == AF_INET6 )
    {
      nSockLen = sizeof(sockaddr_in6);
      memcpy((sockaddr_in6 *)&connectionSockAddrIn, 
             (sockaddr_in6 *)pCurrentSockAddr, nSockLen);
      ((sockaddr_in6 *)&connectionSockAddrIn)->sin6_port = htons(port);
    }

    if(Socket::connect((MYSERVER_SOCKADDR*)(&connectionSockAddrIn), 
                       nSockLen) == -1)
    {
      Socket::closesocket();
    }
    else
    {
       bSocketConnected = true;
       break;
    }
  }
  freeaddrinfo(pHostInfo);
  if ( !bSocketConnected )
     return -1;
#else
  MYSERVER_HOSTENT *hp = Socket::gethostbyname(host);
  struct sockaddr_in sockAddr;
  int sockLen;
  if(hp == 0)
    return -1;

  /*! If the socket is not created, create it before use. */
  if(socketHandle == 0)
  {
    if(Socket::socket(AF_INET, SOCK_STREAM, 0) == -1)
    {
      return -1;
    }                  
  }
  sockLen = sizeof(sockAddr);
  memset(&sockAddr, 0, sizeof(sockAddr));
  sockAddr.sin_family = AF_INET;
  memcpy(&sockAddr.sin_addr, hp->h_addr, hp->h_length);
  sockAddr.sin_port = htons(port);
  if(Socket::connect((MYSERVER_SOCKADDR*)&sockAddr, sockLen) == -1)
  {
    Socket::closesocket();
    return -1;
  }
#endif

  return 0;
}

/*!
 *Connect the socket.
 */
int Socket::connect(MYSERVER_SOCKADDR* sa, int na)
{
  if ( sa == NULL || (sa->ss_family != AF_INET && sa->ss_family != AF_INET6) )
    return 1;//Andu: TODO our error code or what?
  if ( (sa->ss_family == AF_INET && na != sizeof(sockaddr_in)) || 
       (sa->ss_family == AF_INET6 && na != sizeof(sockaddr_in6)) )
    return 1;//Andu: TODO our error code or what?
  
#ifdef WIN32
  return ::connect((SOCKET)socketHandle,(const sockaddr *)sa, na);
#endif
#ifdef NOT_WIN
  return ::connect((int)socketHandle,(const sockaddr *)sa,na);
#endif
}

/*!
 *Receive data from the socket.
 */
int Socket::recv(char* buffer, int len, int flags, u_long timeout)
{
  u_long time = getTicks();
  while(getTicks() - time < timeout)
  {
    /*! Check if there is data to read before do it. */
    if(bytesToRead())
      return recv(buffer, len, flags);
  }
  return 0;

}


/*!
 *Receive data from the socket.
 *Returns -1 on errors.
 */
int Socket::recv(char* buffer,int len,int flags)
{
  int err = 0;

#ifdef WIN32
  do
  {
    err = ::recv(socketHandle, buffer, len, flags);
  }while((err == SOCKET_ERROR) && (GetLastError() == WSAEWOULDBLOCK));

  if(err == SOCKET_ERROR)
    return -1;
  else
    return err;

#endif
#ifdef NOT_WIN
  err = ::recv((int)socketHandle, buffer, len, flags);

  if(err == 0)
    err = -1;

  return err;
#endif

}

/*!
 *Returns the number of bytes waiting to be read.
 */
u_long Socket::bytesToRead()
{
  u_long nBytesToRead = 0;

#ifdef FIONREAD
  ioctlsocket(FIONREAD,&nBytesToRead);
#else
#ifdef I_NREAD
  ::ioctlsocket( I_NREAD, &nBytesToRead ) ;
#endif
#endif
  return nBytesToRead;
}

/*!
 *Pass a nonzero value to set the socket to be nonblocking.
 */
int Socket::setNonBlocking(int non_blocking)
{
  int ret = -1;
#ifdef FIONBIO
  u_long nonblock = non_blocking ? 1 : 0;
  ret = ioctlsocket( FIONBIO, &nonblock);

#else

#ifdef NOT_WIN
  int flags;
  flags = fcntl((int)socketHandle, F_GETFL, 0);
  if (flags < 0)
    return -1;

  if(non_blocking)
    flags |= O_NONBLOCK;
  else
    flags &= ~O_NONBLOCK;

  ret = fcntl((int)socketHandle, F_SETFL, flags);
#endif

#endif
  return ret;
}

/*!
 *Returns the hostname.
 */
int Socket::gethostname(char *name,int namelen)
{
  return ::gethostname(name,namelen);
}

/*!
 *Returns the sockname.
 */
int Socket::getsockname(MYSERVER_SOCKADDR *ad,int *namelen)
{
#ifdef WIN32
     return ::getsockname(socketHandle,(sockaddr *)ad,namelen);
#endif
#ifdef NOT_WIN
  socklen_t len =(socklen_t) *namelen;
  int ret = ::getsockname((int)socketHandle, (struct sockaddr *)ad, &len);
  *namelen = (int)len;

  return ret;
#endif
}

/*!
 *Set the socket used by the server.
 */
void Socket::setServerSocket(Socket* sock)
{
  serverSocket = sock;
}
/*!
 *Returns the server socket.
 */
Socket* Socket::getServerSocket()
{
  return serverSocket;
}

/*!
 *Check if there is data ready to be read.
 *Returns 1 if there is data to read, 0 if not.
 */
int Socket::dataOnRead(int sec, int usec)
{
  struct timeval tv;
  fd_set readfds;
  int ret;
  tv.tv_sec = sec;
  tv.tv_usec = usec;

  FD_ZERO(&readfds);
#ifdef WIN32
  FD_SET((SOCKET)socketHandle, &readfds);
#else
  FD_SET(socketHandle, &readfds);
#endif
  ret = ::select(socketHandle + 1, &readfds, NULL, NULL, &tv);

  if(ret == -1 || ret == 0)
    return 0;

  if (FD_ISSET(socketHandle, &readfds))
    return 1;

  return 0;
}

/*!
 *Inherited from Stream.
 *Return zero on success.
 */
int Socket::read(char* buffer, u_long len, u_long *nbr)
{
  *nbr = static_cast<u_long>(recv(buffer, len, 0));

  if(*nbr == static_cast<u_long>(-1))
    return -1;

  return 0;
}

/*!
 *Inherited from Stream.
 *Return values are equals to send.
 */
int Socket::write(const char* buffer, u_long len, u_long *nbw)
{
  int ret = send(buffer, len, 0);

  if(ret == -1)
    return -1;

  *nbw = static_cast<u_long>(ret);

  return 0;
}