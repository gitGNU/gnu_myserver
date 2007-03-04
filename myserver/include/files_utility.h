/*
MyServer
Copyright (C) 2002, 2003, 2004, 2006, 2007 The MyServer Team
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

#ifndef FILES_UTILITY_H
#define FILES_UTILITY_H

#include "../stdafx.h"
#include "../include/stream.h"
#include "../include/file.h"
#include <string>

using namespace std;

class FilesUtility 
{
	FileHandle handle;
	string filename;
	
private:
	/*! Don't allow instances for this class.  */
	FilesUtility();
public:
	static int getPathRecursionLevel(const char*);
  static int getPathRecursionLevel(string& filename)
    {return getPathRecursionLevel(filename.c_str()); }

	static time_t getLastModTime(const char *filename);
	static time_t getLastModTime(string const &filename)
    {return getLastModTime(filename.c_str());}

	static time_t getCreationTime(const char *filename);
	static time_t getCreationTime(string const &filename)
    {return getCreationTime(filename.c_str());}

	static time_t getLastAccTime(const char *filename);
	static time_t getLastAccTime(string const &filename)
    {return getLastAccTime(filename.c_str());}

  static int chown(const char* filename, int uid, int gid);
  static int chown(string const &filename, int uid, int gid)
    {return chown(filename.c_str(), uid, gid);}

	static int completePath(char**, int *size, int dontRealloc=0);
  static int completePath(string &fileName);

	static int isDirectory(const char*);
  static int isDirectory(string& dir){return isDirectory(dir.c_str());}

	static int isLink(const char*);
  static int isLink(string& dir){return isLink(dir.c_str());}

  static int getShortFileName(char*,char*,int);

	static int fileExists(const char * );
	static int fileExists(string const &file)
    {return fileExists(file.c_str());}

	static int deleteFile(const char * );
	static int deleteFile(string const &file)
    {return deleteFile(file.c_str());}

  static int renameFile(const char*, const char*);
	static int renameFile(string const &before, string const &after)
    {return renameFile(before.c_str(), after.c_str());}

	static void getFileExt(char* ext,const char* filename);
	static void getFileExt(string& ext, string const &filename);

  static void splitPathLength(const char *path, int *dir, int *filename);
	static void splitPath(const char* path, char* dir, char*filename);
	static void splitPath(string const &path, string& dir, string& filename);

  static int getFilenameLength(const char*, int *);
	static void getFilename(const char* path, char* filename);
	static void getFilename(string const &path, string& filename);
};
#endif
