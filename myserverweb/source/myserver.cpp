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

#include "..\stdafx.h"
#include "..\include\cserver.h"
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"wsock32.lib")
#pragma comment(lib,"winmm.lib")
BOOL WINAPI control_handler (DWORD control_type);
void console_service (int, char **);
VOID WINAPI myServerCtrlHandler(DWORD fdwControl);
VOID WINAPI myServerMain (DWORD argc, LPTSTR *argv); 
VOID appendToLog(char* str);
BOOL isServiceInstalled();
void runService();
void printHelpInfo();

cserver server;
static char path[MAX_PATH];
HINSTANCE hInst;
int cmdShow;

int main (int , char **)
{ 
	GetModuleFileName(NULL,path,MAX_PATH);
	int len=lstrlen(path);
	while((path[len]!='\\')&(path[len]!='/'))
		len--;
	path[len]='\0';
	SetCurrentDirectory(path);

	hInst=(HINSTANCE)0;
	cmdShow=0;
	LPSTR cmdLine=GetCommandLine();
	if(lstrlen(cmdLine)==0)
		cmdLine="CONSOLE";
	for(int i=0;i<lstrlen(cmdLine);i++)
	{
		if(!lstrcmpi(&cmdLine[i],"CONSOLE"))
		{
			console_service(0,0);
			return 0;
		}
	}
	for(i=0;i<lstrlen(cmdLine);i++)
	{
		if(!lstrcmpi(&cmdLine[i],"SERVICE"))
		{
			runService();
			return 0;
		}
	}
	printHelpInfo();
	return 0;
} 

/*
*Use this before the file is opened by the class cserver
*/
VOID appendToLog(char* str)
{
	FILE*f=fopen("myServer.log","a+t");
	fwrite(str,lstrlen(str),1,f);
	fclose(f);
	return;
}
SERVICE_STATUS          MyServiceStatus; 
SERVICE_STATUS_HANDLE   MyServiceStatusHandle; 

VOID  WINAPI myServerMain (DWORD, LPTSTR*)
{
	MyServiceStatus.dwServiceType = SERVICE_WIN32;
	MyServiceStatus.dwCurrentState = SERVICE_STOPPED;
	MyServiceStatus.dwControlsAccepted = 0;
	MyServiceStatus.dwWin32ExitCode = NO_ERROR;
	MyServiceStatus.dwServiceSpecificExitCode = NO_ERROR;
	MyServiceStatus.dwCheckPoint = 0;
	MyServiceStatus.dwWaitHint = 0;

	MyServiceStatusHandle = RegisterServiceCtrlHandler( "myServer", myServerCtrlHandler );
	if ( MyServiceStatusHandle )
	{
		MyServiceStatus.dwCurrentState = SERVICE_START_PENDING;
		SetServiceStatus( MyServiceStatusHandle, &MyServiceStatus );

		MyServiceStatus.dwControlsAccepted |= (SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN);
		MyServiceStatus.dwCurrentState = SERVICE_RUNNING;
		SetServiceStatus( MyServiceStatusHandle, &MyServiceStatus );

		server.start(hInst);
	
		MyServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		SetServiceStatus( MyServiceStatusHandle, &MyServiceStatus );

		MyServiceStatus.dwControlsAccepted &= ~(SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN);
		MyServiceStatus.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus( MyServiceStatusHandle, &MyServiceStatus );
	}
}



VOID WINAPI myServerCtrlHandler(DWORD fdwControl)
{
	switch ( fdwControl )
	{
		case SERVICE_CONTROL_INTERROGATE:
			break;

		case SERVICE_CONTROL_SHUTDOWN:
		case SERVICE_CONTROL_STOP:
			MyServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
			SetServiceStatus( MyServiceStatusHandle, &MyServiceStatus );
			server.stop();
			return;

		case SERVICE_CONTROL_PAUSE:
			break;

		case SERVICE_CONTROL_CONTINUE:
			break;

		default:
			if ( fdwControl >= 128 && fdwControl <= 255 )
				break;
			else

				break;
	}
	SetServiceStatus( MyServiceStatusHandle, &MyServiceStatus );
}

void console_service (int, char **)
{
    printf ("starting in console mode\n");
    SetConsoleCtrlHandler (control_handler, TRUE);
	printf("started in console mode\n");
	server.start(hInst);
}

BOOL WINAPI control_handler (DWORD control_type)
{
    switch (control_type)
      {
        case CTRL_BREAK_EVENT:
        case CTRL_C_EVENT:
			printf ("stopping service\n");
            server.stop();
			printf ("service stopped\n");
            return (TRUE);

      }
    return (FALSE);
}


void runService()
{
	printf("Running service...\n");
	SERVICE_TABLE_ENTRY serviceTable[] =
	{
		{ "myServer", myServerMain },
		{ 0, 0 }
	};
	if(!StartServiceCtrlDispatcher( serviceTable ))
	{
		if(GetLastError()==ERROR_INVALID_DATA)
			printf("Invalid data\n");
		else if(GetLastError()==ERROR_SERVICE_ALREADY_RUNNING)
			printf("Already running\n");
		else
			printf("Error running service\n");
			printf("Use instead of this the control panel application to run myServer\n");
	}
}
void printHelpInfo()
{
	printf("On Windows 2000 platform use myServer as a service\n");
	printf("On Windows XP and .NET myServer can be used as a stand-alone program\n");
	printf("Older versions of Windows don't support security options\n");
	printOSInfo();
	printf("Press a key to continue\n");
	getchar();
}
BOOL isServiceInstalled()
{
	SC_HANDLE service,manager;

	manager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
	if (manager)
	{
		service = OpenService (manager, "myServer", SERVICE_ALL_ACCESS);
		if (service)
		{
			CloseServiceHandle (service);
			CloseServiceHandle (manager);
			return TRUE;
		}
		CloseServiceHandle (manager);
		return FALSE;
	}
	return FALSE;
}