/*
*myServer
*Copyright (C) Rocky_10_Balboa
*This library is free software; you can redistribute it and/or
*modify it under the terms of the GNU Library General Public
*License as published by the Free Software Foundation; either
*version 2 of the License, or (at your option) any later version.

*This library is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*Library General Public License for more details.

*You should have received a copy of the GNU Library General Public
*License along with this library; if not, write to the
*Free Software Foundation, Inc., 59 Temple Place - Suite 330,
*Boston, MA  02111-1307, USA.
*/
#include "..\include\security.h"
/*
*Global values for useLogonOption flag and the guest handle
*/
BOOL useLogonOption;
HANDLE guestLoginHandle;
char guestLogin[20];
char guestPassword[32];

BOOL logonCurrentThread(char *name,char* password,PHANDLE handle)
{
	BOOL logon=FALSE;
#ifdef WIN32
	logon=LogonUser(name,NULL,password, LOGON32_LOGON_NETWORK, LOGON32_PROVIDER_DEFAULT,handle);
#endif
	return logon;
}
VOID impersonateLogonUser(HANDLE hImpersonation)
{
#ifdef WIN32
	ImpersonateLoggedOnUser(hImpersonation);
#endif	
}
VOID revertToSelf()
{
#ifdef WIN32
	RevertToSelf();
#endif
}

VOID cleanLogonUser(HANDLE hImpersonation)
{
#ifdef WIN32
	CloseHandle(hImpersonation);
#endif
}

VOID logon(LPCONNECTION c,BOOL *logonStatus,HANDLE *hImpersonation)
{
	*hImpersonation=0;
	if(useLogonOption)
	{
		if(c->login[0])
		{
			*logonStatus=logonCurrentThread(c->login,c->password,hImpersonation);
		}
		else
		{
			*logonStatus=FALSE;
			*hImpersonation=guestLoginHandle;
		}
		impersonateLogonUser(hImpersonation);
	}
	else
	{
		*logonStatus=FALSE;
	}
}
VOID logout(BOOL logon,HANDLE *hImpersonation)
{
	if(useLogonOption)
	{
		revertToSelf();
		if(*hImpersonation)
		{
			cleanLogonUser(*hImpersonation);
			hImpersonation=0;
		}
	}
}
VOID logonGuest()
{
#ifdef WIN32
	if(useLogonOption)
		LogonUser(guestLogin,NULL,guestPassword,LOGON32_LOGON_NETWORK, LOGON32_PROVIDER_DEFAULT, &guestLoginHandle);
#endif
}