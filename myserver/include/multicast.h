/*
MyServer
Copyright (C) 2007 The MyServer Team
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

#ifndef MULTICAST_H
#define MULTICAST_H

#include "../stdafx.h"
#include "../include/hash_map.h"

#include <string>
#include <list>
using namespace std;

template<typename MSG_TYPE, typename ARG_TYPE, typename RET_TYPE> 
class Multicast
{
public:
	RET_TYPE updateMulticast(MSG_TYPE, ARG_TYPE);
};

template<typename MSG_TYPE, typename ARG_TYPE, typename RET_TYPE> 
class MulticastRegistry
{
public:
	void addMulticast(MSG_TYPE, Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>*);
	void removeMulticast(MSG_TYPE, Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>*);
	void notifyMulticast(MSG_TYPE, ARG_TYPE);
protected:
	void removeMulticasts(MSG_TYPE);
	list<Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>*>* getHandlers(MSG_TYPE);
	void clearMulticastRegistry();
private:
	HashMap<MSG_TYPE, list<Multicast<MSG_TYPE, ARG_TYPE, RET_TYPE>*>*> handlers;
};


#include "../source/multicast.cpp"

#endif
