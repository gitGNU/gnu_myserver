/*
*MyServer
*Copyright (C) 2002,2003,2004 The MyServer Team
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

#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include "../stdafx.h"
#include <string>

using namespace std;

char *getRFC822GMTTime(char* out,int len);
char *getRFC822GMTTime(const time_t,char* out,int len);
char *getRFC822LocalTime(char* out,int len);
char *getRFC822LocalTime(const time_t,char* out,int len);

const char *getRFC822GMTTime(string& out,int len);
const char *getRFC822GMTTime(const time_t, string& out, int len);
const char *getRFC822LocalTime(string& out,int len);
const char *getRFC822LocalTime(const time_t, string &out,int len);

int getCharInString(char*,const char*,int max);

time_t getTime(const char* str);
inline time_t getTime(const string& str){ return getTime(str.c_str()); };

void StrTrim(char* str, char* trimChars);

void gotoNextLine(char** cmd);

int hexVal(char c);

void translateEscapeString(char *TargetStr);

int hexToInt(const char *str);
inline time_t hexToInt(const string& str){ return hexToInt(str.c_str()); };

int getEndLine(const char* str, int max);
inline int getEndLine(const string& str, int max)
                {return getEndLine(str.c_str(), max); };

string trim(const string& s , const string& t = " ");
string trimLeft ( const string& s , const string& t = " " );
string trimRight ( const string& s , const string& t = " " );

int stringcmpi(string& a, string& b);
int stringcmp(string& a, string& b);

int stringcmpi(const string& a, const char* b);
int stringcmp(const string& a, const char* b);

#ifdef NOT_WIN
extern "C" {
char* strupr(char * string);

#include <string.h>
}
#endif

#ifndef lstrcmp

#define lstrcmp strcmp
#endif
#ifndef strcmpi
#define strcmpi strcasecmp
#endif
#ifndef lstrcmpi
#define lstrcmpi strcasecmp
#endif
#ifndef _stricmp
#define _stricmp strcasecmp
#endif
#ifndef lstrcpy
#define lstrcpy strcpy
#endif
#ifndef lstrcat
#define lstrcat strcat
#endif
#ifndef lstrlen
#define lstrlen strlen
#endif
#ifndef lstrcpyn
#define lstrcpyn strncpy
#endif

#endif

