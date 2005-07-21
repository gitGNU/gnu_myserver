/*
*MyServer
*Copyright (C) 2002,2003,2004 The MyServer Team
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "../stdafx.h"
#include "../include/utility.h"
#include "../include/sockets.h"

extern "C" {
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#ifndef WIN32
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include <sys/wait.h>
#endif
}

#include <sys/types.h>

#ifdef WIN32
#include <direct.h>
#endif

/*!
*Sleep the caller thread.
*/
void Thread::wait(u_long time)
{
#ifdef WIN32
	Sleep(time);
#endif
#ifdef NOT_WIN
	usleep(time);
#endif

}

/*!
 *Constructor for the mutex class.
 */
Mutex::Mutex()
{
	initialized=0;
	init();
}
/*!
 *Initialize a mutex.
 */
int Mutex::init()
{
  int ret;
	if(initialized)
	{
		destroy();
		initialized=0;
	}
#ifdef HAVE_PTHREAD


#if 0
  pthread_mutexattr_t   mta = NULL;
	pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_NORMAL);
	ret = pthread_mutex_init(&mutex, &mta);
#else
	ret = pthread_mutex_init(&mutex,(pthread_mutexattr_t*) NULL);
#endif


#else
	mutex=CreateMutex(0,0,0);
  ret=!mutex;
#endif
	initialized=1;
	return ret ? 1 : 0;
}

/*!
 *Destroy a mutex.
 */
int Mutex::destroy()
{
#ifdef HAVE_PTHREAD
  if(initialized)
    pthread_mutex_destroy(&mutex);
#else
  if(initialized)
    CloseHandle(mutex);
#endif
	initialized=0;
	return 0;
}

/*!
 *Lock the mutex.
 */
int Mutex::lock(u_long /*id*/)
{
#ifdef HAVE_PTHREAD
#ifdef PTHREAD_ALTERNATE_LOCK
	pthread_mutex_lock(&mutex);
#else
	while(pthread_mutex_trylock(&mutex))
	{
		Thread::wait(1);
	}
#endif

#else	
	WaitForSingleObject(mutex,INFINITE);
#endif
	return 0;
}

/*!
*Unlock the mutex access.
*/
int Mutex::unlock(u_long/*! id*/)
{
#ifdef HAVE_PTHREAD
	int err;
	err = pthread_mutex_unlock(&mutex);
#else	
	ReleaseMutex(mutex);
#endif
	return 1;
}

/*!
*Create a new thread.
*/
#ifdef WIN32
int Thread::create(ThreadID*  ID, 
                            unsigned int  (_stdcall *start_routine)(void *), 
                            void * arg)
#endif
#ifdef HAVE_PTHREAD
int Thread::create(ThreadID*  ID, void * (*start_routine)(void *), 
                            void * arg)
#endif
{
#ifdef WIN32
	_beginthreadex(NULL, 0, start_routine, arg, 0, (unsigned int*)ID);
#endif
#ifdef HAVE_PTHREAD
	pthread_create((pthread_t*)ID, NULL, start_routine, (void *)(arg));
#endif
	return 0;
}

/*!
*Terminate the caller thread.
*/
void Thread::terminate()
{
#ifdef WIN32
	_endthread();
#endif
#ifdef HAVE_PTHREAD
	pthread_exit(0);
#endif
}

/*!
*Destroy the object.
*/
Mutex::~Mutex()
{
	destroy();
}
