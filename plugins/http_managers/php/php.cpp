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
#include "php.h"
#include <thread.h>
#include <mutex.h>
#include <hash_map.h>
#include <http_data_handler.h>
#include <http_response.h>
#include <cgi.h>
#include <thread.h>

#include <main/php.h>
#include <main/SAPI.h>


#ifdef ZTS
#include <TSRM.h>
#endif

static Server* server;
static zend_module_entry entries[16];
static int loadedEntries = 0;
static int initialized = 0;

static Mutex mainMutex;
static Mutex requestMutex;

int singleRequest;

struct PhpData
{
	HttpThreadContext* td;
	bool keepalive;
	bool useChunks;
	bool headerSent;
	FiltersChain chain;
};

extern "C" void addModule(zend_module_entry entry)
{
	entries[loadedEntries++] = entry;
}


static PhpData* getPhpData()
{
#ifdef ZTS
  void*** tsrm_ls = (void***)ts_resource(0);
#endif
	return (PhpData*)SG(server_context);
}

int modifyHeader(HttpResponseHeader *response, char* name, char* value)
{
	string* val = response->getValue(name, NULL);
	
	if(val)
		val->assign(value);
	else
	{
		string field(name);
		string valString(value);
		HttpResponseHeader::Entry* entry = new HttpResponseHeader::Entry(field, valString);

		response->other.put(field, entry);
	}
	return 0;
}

int sendHeader(PhpData* data)
{
	char* buffer;
	if(data->headerSent || data->td->appendOutputs)
		return 0;
	buffer = data->td->buffer2->getBuffer();

	if(data->keepalive)
		modifyHeader(&(data->td->response), "Connection", "keep-alive");
	else
		modifyHeader(&(data->td->response), "Connection", "closed");

	if(data->useChunks)
		modifyHeader(&(data->td->response), "Transfer-Encoding", "chunked");

	HttpHeaders::buildHTTPResponseHeader(buffer,
																			 &data->td->response);

	data->headerSent = true;
	return data->td->connection->socket->send( buffer,
																						 static_cast<int>(strlen(buffer)),
																						 0) != SOCKET_ERROR;



	
}

int myphp_header_handler(sapi_header_struct *sapi_header, sapi_headers_struct *sapi_headers TSRMLS_DC)
{
	PhpData* data = getPhpData();
	char* valInit;
	int sep;
	for(sep = 0; sapi_header->header[sep] != ':'; sep++)
		if(!sapi_header->header[sep])
			return 1;

	sapi_header->header[sep] = 0;

	valInit = sapi_header->header + sep + 1;
	if(*valInit == ' ')
		valInit++;

	modifyHeader(&(data->td->response), sapi_header->header, valInit);

	sapi_header->header[sep] = ':'; 

	return 0;
}

int myphp_read_post(char *buffer, uint count_bytes TSRMLS_DC)
{
	PhpData* data = getPhpData();
	u_long nbr = 0;

	if(data->td->inputData.getHandle() != -1 && data->td->inputData.getFileSize())
		if(data->td->inputData.readFromFile(buffer, count_bytes, &nbr))
			nbr = 0;

	return nbr;
	
}

int myphp_send_headers(sapi_headers_struct *sapi_headers TSRMLS_DC)
{
	return sendHeader(getPhpData());
}

char *myphp_read_cookies(TSRMLS_D)
{
	PhpData* data = getPhpData();
	string *cookies = data->td->request.getValue("Cookie", NULL);

	if(cookies)
		return (char*)cookies->c_str();

	return 0;
}

int myphp_ub_write(const char *str, unsigned int str_length TSRMLS_DC) 
{ 
	PhpData* data = getPhpData();

	if(!data->headerSent)
		if(sendHeader(data))
			return 1;


	data->chain.setStream(data->td->connection->socket);
	data->td->sentData += (u_long)str_length;
	return HttpDataHandler::appendDataToHTTPChannel(data->td,
																									(char*)str,
																									(u_long)str_length,
																									&(data->td->outputData), 
																									&(data->chain),
																									(bool)data->td->appendOutputs, 
																									data->useChunks);


}

void myphp_flush(void *server_context)
{

}


extern "C"
char* name(char* name, u_long len)
{
	char* str = "php";

	if(name)
		strncpy(name, str, len);

	return str;
}

void myphp_register_variables(zval *track_vars_array TSRMLS_DC)
{
	PhpData* data = getPhpData();
	char* ptr;
	Cgi::buildCGIEnvironmentString(data->td, data->td->buffer->getBuffer());

	ptr = data->td->buffer->getBuffer();

	while(ptr)
	{
		char* name = ptr;
		char* value = strchr(name, '=') + 1;
		if(name >= value)
			break;

		ptr += strlen(name) + 1;

		value[-1] = '\0';

		php_register_variable(name, value, track_vars_array TSRMLS_CC);

		value[-1] = '=';

	}
	php_register_variable("env", (char*)data->td->request.uriOpts.c_str(), track_vars_array TSRMLS_CC);
	php_register_variable("PHP_SELF", (char*)data->td->request.uri.c_str(), track_vars_array TSRMLS_CC);
}

int	myphp_startup(struct _sapi_module_struct *sapi_module)
{	
	if(php_module_startup(sapi_module, entries, loadedEntries) == FAILURE) 
	{
		return FAILURE;
	}
	
	return 0;
}

static sapi_module_struct myphp_module = 
{
	"myphp",
	"MyServer PHP Module",

	myphp_startup,                            /* startup */
	php_module_shutdown_wrapper,                    /* shutdown */
	NULL,                                           /* activate */
	NULL,                                           /* deactivate */

	myphp_ub_write,                       /* unbuffered write */
	myphp_flush,                          /* flush */
	NULL, //myphp_get_stat,                       /* get uid */
	NULL, //myphp_getenv,                         /* getenv */
	
	php_error,                                      /* error handler */
	
	myphp_header_handler,                 /* header handler */
	myphp_send_headers,                   /* send headers handler */
	NULL,                                           /* send header handler */
	
	myphp_read_post,                      /* read POST data */
	myphp_read_cookies,                   /* read Cookies */
	
	myphp_register_variables,
	NULL,//myphp_sapi_log_message,                    /* Log message */
	NULL,//myphp_sapi_get_request_time,               /* Request Time */
	
	STANDARD_SAPI_MODULE_PROPERTIES
};


#ifdef WIN32
	static unsigned int phpWatchdog(void * arg)
#else
	static void* phpWatchdog(void * arg)
#endif
{
#ifdef ZTS
		tsrm_startup(((Server*)server)->getMaxThreads(), 1, 0, NULL);
#endif

		sapi_startup(&myphp_module);
		myphp_module.startup(&myphp_module);
		initialized = 1;

		while(!Server::getInstance()->stopServer())
			Thread::wait(MYSERVER_SEC(2));

		myphp_module.shutdown(&myphp_module);
		sapi_shutdown();
	
#ifdef ZTS
    tsrm_shutdown();
#endif
		return 0;
}


static void checkPhpInitialization()
{
	mainMutex.lock();
	if(!initialized)
	{
		ThreadID tid;
		Thread::create(&tid, phpWatchdog, NULL);
	}

	mainMutex.unlock();
}

int load(void* server,void* parser)
{
	const char *data;
	::server = (Server*)server;
	data = ::server->getHashedData("PHP_SAFE_MODE");
	if(data && !strcmpi(data, "YES"))
		singleRequest = 1;
	else
		singleRequest = 0,

	mainMutex.init();
	requestMutex.init();
	checkPhpInitialization();
	return 0;
}



/*! Unload the plugin.  Called once.  Returns 0 on success.  */
int unload(void* p)
{
	mainMutex.destroy();
	requestMutex.destroy();
	return 0;
}



int sendManager(HttpThreadContext* td, ConnectionPtr s, const char *filenamePath,
								const char* cgi, int onlyHeader)
{
	PhpData* data;
	zend_file_handle script;
	int ret = SUCCESS;
	HttpRequestHeader *req = &(td->request);

	checkPhpInitialization();

	if(singleRequest)
		requestMutex.lock();

	TSRMLS_FETCH();

	td->inputData.setFilePointer(0);


	SG(headers_sent) = 0;
	SG(request_info).no_headers = 1;
	SG(request_info).headers_only = onlyHeader;

	SG(request_info).request_uri = (char*)req->uri.c_str();
	SG(request_info).request_method = req->cmd.c_str();

	if(!req->ver.compare("HTTP/1.1") || !req->ver.compare("HTTPS/1.1"))
		SG(request_info).proto_num = 1001;
	else 
		SG(request_info).proto_num = 1000;

	SG(server_context) = data = new PhpData();

	data->td = td;
	HttpDataHandler::checkDataChunks(td, &data->keepalive, &data->useChunks);
	data->headerSent = false;


	SG(sapi_headers).http_response_code = 200;

	SG(request_info).query_string =  (char*)req->uriOpts.c_str();

	SG(request_info).path_translated = (char*)filenamePath;

	if(td->connection->getLogin()[0])
		SG(request_info).auth_user = (char*)td->connection->getLogin();

	if(td->connection->getPassword()[0])
		SG(request_info).auth_password = (char*)td->connection->getPassword();

	if(req->getValue("Content-Type", NULL))
		SG(request_info).content_type = req->getValue("Content-Type", NULL)->c_str();

	SG(request_info).content_length = atoi(req->contentLength.c_str());

	//SG(options) |= SAPI_OPTION_NO_CHDIR;

	{
		char limit[15];
		char *name = "memory_limit";
 		sprintf(limit, "%d", 1 << 30);
		zend_alter_ini_entry(name, strlen(name), limit, strlen(limit), PHP_INI_SYSTEM, PHP_INI_STAGE_ACTIVATE);

		name = "html_errors";
		zend_alter_ini_entry(name, strlen(name), "0", 1, PHP_INI_SYSTEM, PHP_INI_STAGE_ACTIVATE);

	}	

	SG(request_info).post_data_length = SG(request_info).content_length;

	script.type = ZEND_HANDLE_FILENAME;
	script.filename = (char*)filenamePath;
	script.opened_path = NULL;
	script.free_filename = 0;

	zend_first_try 
	{

		if (php_request_startup(TSRMLS_C) == FAILURE) 
		{
			return FAILURE;
		}

		php_execute_script(&script TSRMLS_CC);
		
		if(data->useChunks)
			HttpDataHandler::appendDataToHTTPChannel(data->td,
																							 0,
																							 0,
																							 &(data->td->outputData), 
																							 &(data->chain),
																							 (bool)data->td->appendOutputs, 
																							 data->useChunks);

		php_request_shutdown(NULL);


	}
	zend_end_try();


	if(singleRequest)
		requestMutex.unlock();

	delete data;

	return ret;
}