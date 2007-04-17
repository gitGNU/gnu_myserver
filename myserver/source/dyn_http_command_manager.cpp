/*
MyServer
Copyright (C) 2005, 2006, 2007 The MyServer Team
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


#include "../include/dyn_http_command_manager.h"
#include "../include/dyn_http_command.h"

#include <string>

/*!
 *Create the plugin object.
 */
Plugin* DynHttpCommandManager::createPluginObject()
{
	return new DynamicHttpCommand();
}


/*!
 *Initialize the object.
 */
DynHttpCommandManager::DynHttpCommandManager() : 
	PluginsNamespaceManager(string("http_commands"))
{

}

/*!
 *Destroy the object.
 */
DynHttpCommandManager::~DynHttpCommandManager()
{

}
