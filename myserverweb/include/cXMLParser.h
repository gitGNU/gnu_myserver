/*
*myServer
*Copyright (C) Rocky_10_Balboa
*This library is free software; you can redistribute it and/or
*modify it under the terms of the GNU Library General Public
*License as published by the Free Software Foundation; either
*version 2 of the License, or (at your option) any later version.

*This library is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*Library General Public License for more details.

*You should have received a copy of the GNU Library General Public
*License along with this library; if not, write to the
*Free Software Foundation, Inc., 59 Temple Place - Suite 330,
*Boston, MA  02111-1307, USA.
*/
#pragma once

#include "..\StdAfx.h"
#include "..\include\fileManager.h"
#ifndef cXMLParser_IN
#define cXMLParser_IN
/*
*This class is used to open a .xml file and read informations from it.
*/
class cXMLParser
{
	u_long buffersize;
	char *buffer;
	char data[MAX_PATH];
	MYSERVER_FILE_HANDLE file;
public:
	cXMLParser();
	void open(char* filename);
	char *getValue(char* field);
	void close();
};
#endif