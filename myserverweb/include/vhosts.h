/*
*MyServer
*Copyright (C) 2002 The MyServer Team
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

#ifndef VHOST_IN
#define VHOST_IN

#include "../stdafx.h"
#include "../include/filemanager.h"
#include "../include/connectionstruct.h"/*Used for protocols IDs*/
class vhost
{
	MYSERVER_FILE warningsLogFile;
	MYSERVER_FILE accessesLogFile;
public:
	struct sHostList
	{
		char hostName[MAX_COMPUTERNAME_LENGTH+1];
		sHostList *next;
	}*hostList;/*List of hosts allowed by the vhost*/

	struct sIpList
	{
		char hostIp[32];
		sIpList *next; 
	}*ipList;/*List of IPs allowed by the vhost*/

	u_short port;/*Port to listen on*/

	CONNECTION_PROTOCOL protocol;/*Protocol used by the virtual host*/

	char documentRoot[MAX_PATH];/*Path to the document root*/
	char systemRoot[MAX_PATH];/*Path to the system root*/
	char accessesLogFileName[MAX_PATH];/*Path to the accesses log file*/
	char warningsLogFileName[MAX_PATH];/*Path to the warnings log file*/
	char name[64];/*Description or name of the virtual host*/
	vhost();
	void addIP(char *);
	void addHost(char *);
	void removeIP(char *);
	void removeHost(char *);
	int areAllHostAllowed();
	int areAllIPAllowed();
	void clearIPList();
	void clearHostList();
	int isHostAllowed(char*);
	int isIPAllowed(char*);
	~vhost();
	/*
	*Functions to manage the logs file.
	*Derived directly from the filemanager utilities.
	*/
	u_long accessesLogWrite(char*);
	MYSERVER_FILE* getAccessesLogFile();

	u_long warningsLogWrite(char*);
	MYSERVER_FILE* getWarningsLogFile();
};


class vhostmanager
{
public:
	struct sVhostList
	{
		vhost* host;
		sVhostList* next;
	};
private:
	sVhostList *vhostList;/*List of virtual hosts*/
public:
	vhostmanager();
	~vhostmanager();
	int getHostsNumber();
	vhost* getVHostByNumber(int n);
	void clean();
	int removeVHost(int n);
	int switchVhosts(int n1,int n2);
	int switchVhosts(sVhostList*,sVhostList*);
	vhostmanager::sVhostList*  getvHostList();
	vhost*  getvHost(char*,char*,u_short);/*Get a pointer to a vhost*/
	void addvHost(vhost*);/*Add an element to the vhost list*/
	void loadConfigurationFile(char *,int maxlogSize=0);/*Load the virtual hosts list from a configuration file*/
	void saveConfigurationFile(char *);/*Save the virtual hosts list to a configuration file*/
	void loadXMLConfigurationFile(char *,int maxlogSize=0);/*Load the virtual hosts list from a xml configuration file*/
	void saveXMLConfigurationFile(char *);/*Save the virtual hosts list to a xml configuration file*/
};

#endif
