/*
*myServer
*Copyright (C) 2002 The MyServer team
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


#ifndef HTTPMSG_H
#define HTTPMSG_H

#define e_200			13
#define e_201			14
#define e_202			15
#define e_203			16
#define e_204			17
#define e_300			18
#define e_301			19
#define e_302			20
#define e_303			21
#define e_304			22
#define e_400			0
#define e_401			1
#define e_401AUTH		1001
#define e_403			2
#define e_404			3
#define e_405			4
#define e_406			5
#define e_407			6
#define e_412			7
#define e_413			8 
#define e_414			9
#define e_500			10
#define e_501			11
#define e_502			12

extern char HTTP_ERROR_MSGS[24][64];
extern char HTTP_ERROR_HTMLS[24][64];
int getErrorIDfromHTTPStatusCode(int statusCode);
int getHTTPStatusCodeFromErrorID(int statusCode);
#endif
