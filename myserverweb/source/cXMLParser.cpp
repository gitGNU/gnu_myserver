/*
*myServer
*Copyright (C) 2002 Giuseppe Scrivano
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

#include "../include/cXMLParser.h"
#include "../include/utility.h"

extern "C" {
#include <string.h>
}

#ifndef WIN32
#define lstrlen strlen
#endif

/*
*This code is used to parse a pseudo-xml file.
*With the open function we open a file and store it in memory.
*/
void cXMLParser::open(char* filename)
{
	file.ms_OpenFile(filename,MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_IFEXISTS);
	buffersize=file.ms_getFileSize();
	buffer=(char*)malloc(buffersize);
	u_long nbr;
	if(buffer)
		file.ms_ReadFromFile(buffer,buffersize,&nbr);
	else
		nbr=0;
	if(nbr==0)
		buffer[0]='\0';
}
/*
*Constructor of the cXMLParser class.
*/
cXMLParser::cXMLParser()
{
	buffer=0;
	buffersize=0;
	data[0]='\0';
}
/*
*Only get the the text "T" in <VALUENAME>T</VALUENAME>.
*/
char *cXMLParser::getValue(char* vName)
{
	if(buffer==0)
		return 0;
	int found;
	char *ret=NULL;
	u_long i,j;
	u_long len=(u_long)strlen(vName);
	for(i=0;i<buffersize;i++)
	{
 		if(buffer[i]=='<')
		{
			/*
			*If there is a comment go to end of this.
			*/
			if(buffer[i+1]=='!')
			if(buffer[i+2]=='-')
			if(buffer[i+3]=='-')
			{
				i+=3;
				while((buffer[i]!='-')&&(buffer[i+1]!='>'))
					i++;
				continue;
			}
			/*
			*If we arrive here this is not a comment.
			*/
			found=true;
			for(j=0;j<len;j++)
			{
				if(buffer[i+j+1]!=vName[j])
				{
					found=false;
					break;
				}
			}
			if(buffer[i+j+1]!='>')
				found=false;
				
			while(buffer[i++]!='>');
			if(found)
			{
				for(j=0;;j++)
				{
					data[j]=buffer[i+j];
					data[j+1]='\0';
					if(buffer[i+j+1]=='<')
						break;
				}
				ret=data;
				break;
			}
		}
	}
	return ret;
}

/*
*Free the memory used by the class.
*/
void cXMLParser::close()
{
	if(file.ms_GetHandle())
		file.ms_CloseFile();
	if(buffer)
		free(buffer);
}
