/*
*MyServer
*Copyright (C) 2002 The MyServer Team
*
*   strlcpy and strlcat by codingmaster
*
*   (c)2004 by codingmaster
*
*
*
*
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
#ifndef SECURESTR_H
#define SECURESTR_H
unsigned int myserver_strlcat(char *destination, const char *source, unsigned int size);
unsigned int myserver_strlcpy(char *destination, const char *source, unsigned int size);
#endif