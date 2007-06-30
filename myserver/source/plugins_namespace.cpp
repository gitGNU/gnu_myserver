/*
MyServer
Copyright (C) 2007 The MyServer Team
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

#include "../stdafx.h"
#include "../include/plugins_namespace.h"
#include <string>
using namespace std;

/*!
 *Constructor for the class.
 *\param name A name for the namespace.
 */
PluginsNamespace::PluginsNamespace(string name)
{
	this->name.assign(name);
	loaded = false;
}

/*!
 *Destroy the object.
 */
PluginsNamespace::~PluginsNamespace()
{
	unLoad(0);
	loaded = false;
}


/*!
 *Constructor for the class.
 *\param name A name for the namespace.
 *\param clone Another namespace to copy plugins from.  
 *A plugin can exist only in a namespace at once, this constructor will clean 
 *up automatically the clone namespace.
 */
PluginsNamespace::PluginsNamespace(string& name, PluginsNamespace& clone) 
{
	this->name.assign(name);
	HashMap<string, Plugin*>::Iterator it = clone.plugins.begin();
	while(it != clone.plugins.end())
	{
		string name((*it)->getName(0, 0));
		plugins.put(name, *it);
		it++;
	}
	loaded = clone.loaded;

}

/*!
 *Get the namespace name.
 */
string& PluginsNamespace::getName()
{
	return name;
}

/*!
 *Get a plugin by its name.
 *\param name The plugin name.
 */
Plugin* PluginsNamespace::getPlugin(string &name)
{
	return plugins.get((char*)name.c_str());
}

/*!
 *Post load sequence, called when all the plugins are loaded.
 *\param server The server object to use.
 *\param languageFile The language file to use to retrieve warnings/errors 
 *messages.
 */
int PluginsNamespace::postLoad(Server* server, XmlParser* languageFile)
{
	HashMap<string, Plugin*>::Iterator it = plugins.begin();
	while(it != plugins.end())
	{
		(*it)->postLoad(server, languageFile);
		it++;
	}
	loaded = true;
	return 0;
}

/*!
 *Unload the namespace.
 *\param languageFile The language file to use to retrieve warnings/errors 
 *messages.
 */
int PluginsNamespace::unLoad(XmlParser* languageFile)
{
	HashMap<string, Plugin*>::Iterator it = plugins.begin();
	HashMap<string, PluginOption*>::Iterator poit = pluginsOptions.begin();

	while(it != plugins.end())
	{
		(*it)->unLoad(languageFile);
		delete *it;
		it++;
	}

	while(poit != pluginsOptions.end())
	{
		delete *poit;
		poit++;
	}

	loaded = false;
	plugins.clear();
	pluginsOptions.clear();
	return 0;
}

/*!
 *Set a name for the namespace.
 *\param name The new name for the namespace.
 */
void PluginsNamespace::setName(string& name)
{
	this->name.assign(name);
}

/*!
 *Add a plugin that is already preloaded to the namespace.
 *\param plugin The plugin to add.
 */
int PluginsNamespace::addPreloadedPlugin(Plugin* plugin)
{
	string name(plugin->getName(0, 0));
	plugins.put(name, plugin);
	return 0;
}

/*!
 *Remove a plugin from the namespace without clean it.
 *\param name The plugin to remove.
 */
void PluginsNamespace::removePlugin(string& name)
{
	plugins.remove(name);
}

/*!
 *Add a plugin option structure to the namespace.
 *\param plugin The plugin name.
 *\param po The options for the plugin.
 */
int PluginsNamespace::addPluginOption(string& plugin, PluginOption& po)
{
	PluginOption* newPo = new PluginOption(po);
	PluginOption* oldPo = pluginsOptions.put(plugin, newPo);

	if(oldPo)
		delete oldPo;

	return 0;
}

/*!
 *Remove a plugin from the namespace without clean it.
 *\param name The plugin to remove.
 */
PluginsNamespace::PluginOption* PluginsNamespace::getPluginOption(string& plugin)
{
	return pluginsOptions.get(plugin);
}
