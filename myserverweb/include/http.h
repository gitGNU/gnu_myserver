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

#ifndef HTTP_H
#define HTTP_H
#include "../stdafx.h"
#include "../include/protocol.h"
#include "../include/http_headers.h"
#include "../include/cgi.h"
#include "../include/winCGI.h"
#include "../include/fastCGI.h"
#include "../include/mscgi.h"
#include "../include/isapi.h"
#include "../include/cXMLParser.h"
/*!
*Data used only by an HTTP user.
*/
struct http_user_data
{
	char realm[48];/*Realm string used by Digest authorization scheme*/
	char opaque[48];/*Opaque string used by Digest authorization scheme*/
	char nonce[48];/*Nonce string used by Digest authorization scheme*/
	char cnonce[48];/*Cnonce string used by Digest authorization scheme*/
	char needed_password[16];/*Password string used by Digest authorization scheme*/
	u_long nc;/*Nonce count used by Digest authorization scheme*/
	int digest;/*Nonzero if the user was authenticated trough the Digest scheme*/
};
class http : public protocol
{
private:
	static int mscgiLoaded;/*Store if the MSCGI library was loaded.*/
	static char browseDirCSSpath[MAX_PATH];
	static u_long gzip_threshold;
	static int useMessagesFiles;	
	static char *defaultFilename;
	static u_long nDefaultFilename;	
	mscgi lmscgi;
	wincgi lwincgi;
	isapi lisapi;
	cgi lcgi;
	fastcgi lfastcgi;
	struct httpThreadContext td;
protected:
	char protocolPrefix[12];	
public:
	int PROTOCOL_OPTIONS;
	char *getDefaultFilenamePath(u_long ID);
	int sendHTTPRESOURCE(httpThreadContext*,LPCONNECTION s,char *filename,int systemrequest=0,int OnlyHeader=0,int firstByte=0,int lastByte=-1,int yetmapped=0);
	int putHTTPRESOURCE(httpThreadContext*,LPCONNECTION s,char *filename,int systemrequest=0,int OnlyHeader=0,int firstByte=0,int lastByte=-1,int yetmapped=0);
	int deleteHTTPRESOURCE(httpThreadContext*,LPCONNECTION s,char *filename,int yetmapped=0);
	int sendHTTPFILE(httpThreadContext*,LPCONNECTION s,char *filenamePath,int OnlyHeader=0,int firstByte=0,int lastByte=-1);
	int sendHTTPDIRECTORY(httpThreadContext*,LPCONNECTION s,char* folder);
	int raiseHTTPError(httpThreadContext*,LPCONNECTION a,int ID);
	int sendHTTPhardError500(httpThreadContext* td,LPCONNECTION a);
	int sendAuth(httpThreadContext* td,LPCONNECTION a);
	void getPath(httpThreadContext* td,LPCONNECTION s,char *filenamePath,const char *filename,int systemrequest);
	int getMIME(char *MIME,char *filename,char *dest,char *dest2);
	int logHTTPaccess(httpThreadContext* td,LPCONNECTION a);
	int sendHTTPRedirect(httpThreadContext* td,LPCONNECTION a,char *newURL);
	int sendHTTPNonModified(httpThreadContext* td,LPCONNECTION a);
	void resetHTTPUserData(http_user_data*);
	http();
	u_long checkDigest(httpThreadContext* td,LPCONNECTION s);
	/*!
	*The function is used to the request and build a response.
	*/
	virtual char* registerName(char*,int len);
	int controlConnection(LPCONNECTION a,char *b1,char *b2,int bs1,int bs2,u_long nbtr,u_long id);
	static int loadProtocol(cXMLParser*,char*);
	static int unloadProtocol(cXMLParser*);
};


#endif
