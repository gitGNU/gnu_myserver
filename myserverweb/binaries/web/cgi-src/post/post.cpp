#ifdef WIN32
#pragma comment(lib,"../../../cgi-lib/CGI-LIB.lib")
#endif

#include "../../../cgi-lib/cgi_manager.h"

#ifdef WIN32
int EXPORTABLE main (char *cmd,MsCgiData* data)
#else
extern "C" int main (char *cmd,MsCgiData* data)
#endif
{
	CgiManager cm(data);
	if(strlen(cmd)==0)	
	{	
		cm.write("<title>MyServer</title><body bgcolor=\"#FFFFFF\" text=\"#666699\"><p align=\"center\"><img border=\"0\" src=\"logo.png\"></p><form method=\"POST\"><p align=\"center\">  <input type=\"text\" name=\"T1\" size=\"20\" value=\"POST\">	  <input type=\"submit\" value=\"Send\" name=\"B1\">	  <input type=\"reset\" value=\"Reset\" name=\"B2\"></p>	  </p>	</form><p align=\"center\">&nbsp;</p>	<p>&nbsp;</p>");
	}
	else
	{
		cm.write("<title>MyServer</title><body bgcolor=\"#FFFFFF\" text=\"#666699\"><p align=\"center\"><img border=\"0\" src=\"logo.png\"></p>Argument posted:");
		char *post=cm.postParam("T1");
		if(post==0)
			post=cm.getParam("T1");
		if(post)
			cm.write(post);
	}
	cm.clean();
	return 0;
}

#ifdef WIN32
BOOL APIENTRY DllMain( HANDLE,DWORD ul_reason_for_call,LPVOID)
{
	switch (ul_reason_for_call)	
	{
		case DLL_PROCESS_ATTACH:	
		case DLL_THREAD_ATTACH:	
		case DLL_THREAD_DETACH:	
		case DLL_PROCESS_DETACH:
		break;	
	}   
	return TRUE;
}
#endif
