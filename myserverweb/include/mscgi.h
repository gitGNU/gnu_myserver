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

#include "..\stdafx.h"
#include "..\include\Response_RequestStructs.h"
#include "..\include\mime_manager.h"
#include "..\include\cgi.h"
struct httpThreadContext;
BOOL loadMSCGILib();
BOOL freeMSCGILib();
BOOL sendMSCGI(httpThreadContext*,LPCONNECTION s,char* exec,char* cmdLine=0);
