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

#include "../stdafx.h"
#include "../include/plugins_namespace_manager.h"
#include "../include/lfind.h"
#include "../include/xml_parser.h"
#include "../include/server.h"
#include <string>
using namespace std;

/*!
 *Constructor for the class.
 *\param name The name for this namespace.
 */
PluginsNamespaceManager::PluginsNamespaceManager(string name) : 
	PluginsNamespace(name)
{

}

/*!
 *Add a plugin to the namespace.
 *\param file The plugin file name.
 *\param server The server object to use.
 *\param languageFile The language file to use to retrieve warnings/errors 
 *messages.
 */
int PluginsNamespaceManager::addPlugin(string& file, Server* server, 
																			 XmlParser* languageFile)
{
	Plugin *plugin = new Plugin();
	string logBuf;
	string name;
	const char* namePtr;
	if(plugin->load(file, server, languageFile))
	{
		delete plugin;
		return 1;
	}
	namePtr = plugin->getName(0, 0);
	if(namePtr)
		name.assign(namePtr);
	else
	{
		delete plugin;
		return 1;
	}
		
	plugins.put(name, plugin);

	logBuf.assign(languageFile->getValue("MSG_LOADED"));
	logBuf.append(" ");
	logBuf.append(file);
	logBuf.append(" --> ");
	logBuf.append(name);
	server->logWriteln( logBuf.c_str() );
	return 0;
}


/*!
 *Post load sequence, called when all the plugins are loaded.
 *\param server The server object to use.
 *\param languageFile The language file to use to retrieve warnings/errors 
 *messages.
 *\param resource The resource location to use to load plugins, in this 
 *implementation it is a directory name.
 */
int PluginsNamespaceManager::load(Server* server, XmlParser* languageFile, 
																	string& resource)
{
	FindData fd;
	string dirPattern;
	string filename;
  string completeFileName;	
	int ret;

  filename.assign(resource);
  filename.append("/");
  filename.append(getName());

	dirPattern.assign(filename);
#ifdef WIN32
  dirPattern.append("/*.*");
#endif	

	ret = fd.findfirst(dirPattern.c_str());	
	
  if(ret == -1)
  {
		return ret;	
  }

	ret = 0;

	do
	{	
		if(fd.name[0]=='.')
			continue;
		/*!
     *Do not consider file other than dynamic libraries.
     */
#ifdef WIN32
		if(!strstr(fd.name,".dll"))
#endif
#ifdef NOT_WIN
		if(!strstr(fd.name,".so"))
#endif		
			continue;
    completeFileName.assign(filename);
		if((fd.name[0] != '/') && (fd.name[0] != '\\')) 
			completeFileName.append("/");
    completeFileName.append(fd.name);

		ret |= addPlugin(completeFileName, server, languageFile);
	}while(!fd.findnext());
	fd.findclose();

	return ret;
}