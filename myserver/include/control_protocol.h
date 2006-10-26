/*
MyServer
Copyright (C) 2004, 2005 The MyServer Team
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

#ifndef CONTROL_PROTOCOL_H
#define CONTROL_PROTOCOL_H
#include "../stdafx.h"
#include "../include/protocol.h"
#include "../include/control_header.h"

#include <string>

using namespace std;

class ControlProtocol : public Protocol
{
  static char adminLogin[64];
  static char adminPassword[64];
  static int controlEnabled;
  /*! Thread ID. */
  int id;
  /*! Input file. */
  File *Ifile;
  /*! Output file. */
  File *Ofile;
  /*! Protocol level disable */
  bool Reboot;

  /*! Use control_header to parse the request. */
  ControlHeader header;
  int checkAuth();
  int showConnections(ConnectionPtr,File* out, char *b1,int bs1);
  int showDynamicProtocols(ConnectionPtr,File* out, char *b1,int bs1);
  int showLanguageFiles(ConnectionPtr, File* out, char *b1,int bs1);
  int killConnection(ConnectionPtr,u_long ID, File* out, char *b1,int bs1);
  int getFile(ConnectionPtr, char*, File* in, File* out, 
              char *b1,int bs1 );
  int putFile(ConnectionPtr,char*, File* in, File* out, 
              char *b1,int bs1 );
  int getVersion(ConnectionPtr,File* out, char *b1,int bs1);
  int addToErrorLog(ConnectionPtr con, const char *b1, int bs1);
  int addToLog(int retCode, ConnectionPtr con, char *b1, int bs1);
  int addToErrorLog(ConnectionPtr con, string& m)
    {return addToErrorLog(con, m.c_str(), m.size());}

public:
  int sendResponse(char*, int, ConnectionPtr, int, File* = 0);
  static int loadProtocol(XmlParser* languageParser);
	int controlConnection(ConnectionPtr a, char *b1, char *b2, int bs1, 
                        int bs2, u_long nbtr, u_long id);
	virtual char* registerName(char*,int len);
	ControlProtocol();
	virtual ~ControlProtocol();
};

#endif
