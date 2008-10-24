/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2002, 2003, 2004, 2007, 2008 Free Software Foundation, Inc.
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

#ifndef MIME_MANAGER_H
#define MIME_MANAGER_H

#include <include/base/utility.h>
#include <include/base/hash_map/hash_map.h>
#include <include/base/sync/mutex.h>
#include <include/base/xml/xml_parser.h>


#ifdef WIN32
#include <windows.h>
#endif
extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef WIN32
#include <tchar.h>
#include <io.h>
#endif
}

#include <string>
#include <map>
#include <vector>
#include <list>

using namespace std;

struct MimeRecord
{
	list<string> filters;
	string extension;
	string mimeType;
	string cmdName;
	string cgiManager;
	MimeRecord();
	MimeRecord(MimeRecord&);
	int addFilter(const char*, bool acceptDuplicate = true);
	~MimeRecord();
	void clear();
};

class MimeManager
{
public:
	MimeManager();
  ~MimeManager();
	u_long getNumMIMELoaded();

	int loadXML(const char *filename);
	int loadXML(string &filename)
    {return loadXML(filename.c_str());}

	MimeRecord* getMIME(const char* ext);
  MimeRecord* getMIME(string const &ext);

  bool isLoaded();
	void clean();

protected:
  MimeRecord *readRecord (xmlNodePtr node);
	const char *getFilename();
	int addRecord(MimeRecord *record);
	void removeAllRecords();
private:
  bool loaded;
  HashMap<string, int> extIndex;
  vector <MimeRecord*> records;

	u_long numMimeTypesLoaded;
	string *filename;
};

#endif 
