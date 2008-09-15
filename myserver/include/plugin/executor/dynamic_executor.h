/* -*- mode: cpp-mode */
/*
MyServer
Copyright (C) 2007 Free Software Foundation, Inc.
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

#ifndef DYNAMIC_EXECUTOR_H
#define DYNAMIC_EXECUTOR_H

#include "stdafx.h"
#include <include/base/xml/xml_parser.h>
#include <include/protocol/protocol.h>
#include <include/connection/connection.h>
#include <include/base/dynamic_lib/dynamiclib.h>
#include <include/protocol/http/http_headers.h>
#include <include/base/hash_map/hash_map.h>
#include <include/plugin/plugin.h>
#include <include/plugin/plugins_namespace_manager.h>
#include <string>
using namespace std;

class DynamicExecutor : public Plugin
{
public:
	DynamicExecutor();
	virtual ~DynamicExecutor();
	int execute(char* buffer, u_long length);
	int executeFromFile(char* fileName);
private:
	int loadFileAndExecute(char* fileName);
  XmlParser *errorParser;
	string filename;
};

#endif
