/*
===========================================================================
Copyright (C) 2026, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

// cl_http.cpp -- a web request made on its own thread, out of the way of the game
//
// One request at a time per httpRequest_t: started, then polled each frame until it's done. The
// thread only touches the request's finished, status and answer; what's sent is copied first
// and freed once it's done, both on the game's side.
//
// Windows asks through WinHTTP. Linux and macOS through libcurl, looked for as it asks rather
// than linked, as the update check does (cl_update.cpp).

#include "client.h"
#include "cl_http.h"

#if defined(_WIN32)
#include <windows.h>
#include <winhttp.h>
#else
#include <dlfcn.h>
#include <pthread.h>
#endif

#define HTTP_TIMEOUT_MS		10000

static void HTTP_Finish( httpRequest_t *req ) {
#if defined(_WIN32)
	InterlockedExchange( &req->finished, 1 );
#else
	__atomic_store_n( &req->finished, 1, __ATOMIC_RELEASE );
#endif
}

static qboolean HTTP_Finished( httpRequest_t *req ) {
#if defined(_WIN32)
	return (qboolean)( InterlockedCompareExchange( &req->finished, 0, 0 ) != 0 );
#else
	return (qboolean)( __atomic_load_n( &req->finished, __ATOMIC_ACQUIRE ) != 0 );
#endif
}

#if defined(_WIN32)

static DWORD WINAPI HTTP_Thread( LPVOID arg ) {
	httpRequest_t *req = (httpRequest_t *)arg;
	HINTERNET session = NULL, connect = NULL, request = NULL;
	URL_COMPONENTSW parts;
	wchar_t url[512], host[256], path[512];
	DWORD code = 0, size = sizeof( code ), got = 0;

	req->status = 0;
	req->answerLen = 0;
	req->answer[0] = '\0';

	memset( &parts, 0, sizeof( parts ) );
	parts.dwStructSize = sizeof( parts );
	parts.lpszHostName = host;
	parts.dwHostNameLength = ARRAY_LEN( host );
	parts.lpszUrlPath = path;
	parts.dwUrlPathLength = ARRAY_LEN( path );
	if ( !MultiByteToWideChar( CP_UTF8, 0, req->url, -1, url, ARRAY_LEN( url ) ) || !WinHttpCrackUrl( url, 0, 0, &parts ) ) {
		HTTP_Finish( req );
		return 0;
	}

	session = WinHttpOpen( L"OpenJK-rpmod", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0 );
	if ( session ) {
		WinHttpSetTimeouts( session, HTTP_TIMEOUT_MS, HTTP_TIMEOUT_MS, HTTP_TIMEOUT_MS, HTTP_TIMEOUT_MS );
		connect = WinHttpConnect( session, host, parts.nPort, 0 );
	}
	if ( connect )
		request = WinHttpOpenRequest( connect, req->send ? L"POST" : L"GET", path[0] ? path : L"/", NULL, WINHTTP_NO_REFERER,
			WINHTTP_DEFAULT_ACCEPT_TYPES, parts.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0 );
	if ( request
		&& WinHttpSendRequest( request, req->send ? L"Content-Type: application/json\r\n" : WINHTTP_NO_ADDITIONAL_HEADERS, req->send ? (DWORD)-1 : 0,
			req->send ? req->send : WINHTTP_NO_REQUEST_DATA, req->sendLen, req->sendLen, 0 )
		&& WinHttpReceiveResponse( request, NULL ) ) {
		if ( WinHttpQueryHeaders( request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &code, &size, WINHTTP_NO_HEADER_INDEX ) )
			req->status = (int)code;
		while ( req->answerLen < HTTP_MAX_ANSWER - 1
			&& WinHttpReadData( request, req->answer + req->answerLen, HTTP_MAX_ANSWER - 1 - req->answerLen, &got ) && got )
			req->answerLen += (int)got;
		req->answer[req->answerLen] = '\0';
	}
	if ( request )
		WinHttpCloseHandle( request );
	if ( connect )
		WinHttpCloseHandle( connect );
	if ( session )
		WinHttpCloseHandle( session );
	HTTP_Finish( req );
	return 0;
}

static qboolean HTTP_StartThread( httpRequest_t *req, char *error, int errorSize ) {
	HANDLE thread = CreateThread( NULL, 0, HTTP_Thread, req, 0, NULL );

	if ( !thread ) {
		Q_strncpyz( error, "couldn't start asking", errorSize );
		return qfalse;
	}
	CloseHandle( thread );
	return qtrue;
}

#else

// the little of libcurl's interface used, its numbers as curl.h has them: they never change
typedef void CURL;
typedef size_t ( *curlWrite_t )( char *data, size_t size, size_t count, void *user );

#define HTTP_CURL_GLOBAL_DEFAULT		3L
#define HTTP_CURLOPT_WRITEDATA			10001
#define HTTP_CURLOPT_URL				10002
#define HTTP_CURLOPT_POSTFIELDS			10015
#define HTTP_CURLOPT_USERAGENT			10018
#define HTTP_CURLOPT_POSTFIELDSIZE		60
#define HTTP_CURLOPT_FOLLOWLOCATION		52
#define HTTP_CURLOPT_NOSIGNAL			99
#define HTTP_CURLOPT_TIMEOUT_MS			155
#define HTTP_CURLOPT_CONNECTTIMEOUT_MS	156
#define HTTP_CURLOPT_WRITEFUNCTION		20011
#define HTTP_CURLINFO_RESPONSE_CODE		0x200002

static struct {
	void		*lib;
	int			( *global_init )( long flags );
	CURL		*( *easy_init )( void );
	int			( *easy_setopt )( CURL *curl, int option, ... );
	int			( *easy_perform )( CURL *curl );
	int			( *easy_getinfo )( CURL *curl, int info, ... );
	void		( *easy_cleanup )( CURL *curl );
} curl;

// libcurl, found once; qfalse when the computer hasn't got it
static qboolean HTTP_LoadCurl( void ) {
	static const char *names[] = {
#if defined(__APPLE__)
		"libcurl.4.dylib", "libcurl.dylib", "/usr/lib/libcurl.4.dylib",
#else
		"libcurl.so.4", "libcurl-gnutls.so.4", "libcurl-nss.so.4", "libcurl.so",
#endif
	};

	if ( curl.lib )
		return qtrue;
	for ( size_t i = 0; i < ARRAY_LEN( names ) && !curl.lib; i++ )
		curl.lib = dlopen( names[i], RTLD_NOW | RTLD_LOCAL );
	if ( !curl.lib )
		return qfalse;
	*(void **)&curl.global_init = dlsym( curl.lib, "curl_global_init" );
	*(void **)&curl.easy_init = dlsym( curl.lib, "curl_easy_init" );
	*(void **)&curl.easy_setopt = dlsym( curl.lib, "curl_easy_setopt" );
	*(void **)&curl.easy_perform = dlsym( curl.lib, "curl_easy_perform" );
	*(void **)&curl.easy_getinfo = dlsym( curl.lib, "curl_easy_getinfo" );
	*(void **)&curl.easy_cleanup = dlsym( curl.lib, "curl_easy_cleanup" );
	if ( !curl.global_init || !curl.easy_init || !curl.easy_setopt || !curl.easy_perform || !curl.easy_getinfo || !curl.easy_cleanup
		|| curl.global_init( HTTP_CURL_GLOBAL_DEFAULT ) != 0 ) {
		dlclose( curl.lib );
		memset( &curl, 0, sizeof( curl ) );
		return qfalse;
	}
	return qtrue;
}

// what comes back, into the answer; past its end left out
static size_t HTTP_Write( char *data, size_t size, size_t count, void *user ) {
	httpRequest_t *req = (httpRequest_t *)user;
	const size_t total = size * count;
	const int room = HTTP_MAX_ANSWER - 1 - req->answerLen;
	const int n = (int)Q_min( (size_t)Q_max( room, 0 ), total );

	if ( n > 0 ) {
		memcpy( req->answer + req->answerLen, data, n );
		req->answerLen += n;
		req->answer[req->answerLen] = '\0';
	}
	return total;
}

static void *HTTP_Thread( void *arg ) {
	httpRequest_t *req = (httpRequest_t *)arg;
	CURL *c = curl.easy_init();

	req->status = 0;
	req->answerLen = 0;
	req->answer[0] = '\0';
	if ( c ) {
		long code = 0;

		curl.easy_setopt( c, HTTP_CURLOPT_URL, req->url );
		curl.easy_setopt( c, HTTP_CURLOPT_USERAGENT, "OpenJK-rpmod" );
		curl.easy_setopt( c, HTTP_CURLOPT_WRITEFUNCTION, (curlWrite_t)HTTP_Write );
		curl.easy_setopt( c, HTTP_CURLOPT_WRITEDATA, (void *)req );
		curl.easy_setopt( c, HTTP_CURLOPT_NOSIGNAL, 1L );
		curl.easy_setopt( c, HTTP_CURLOPT_FOLLOWLOCATION, 1L );
		curl.easy_setopt( c, HTTP_CURLOPT_CONNECTTIMEOUT_MS, (long)HTTP_TIMEOUT_MS );
		curl.easy_setopt( c, HTTP_CURLOPT_TIMEOUT_MS, (long)( HTTP_TIMEOUT_MS * 2 ) );
		if ( req->send ) {
			curl.easy_setopt( c, HTTP_CURLOPT_POSTFIELDS, req->send );
			curl.easy_setopt( c, HTTP_CURLOPT_POSTFIELDSIZE, (long)req->sendLen );
		}
		if ( curl.easy_perform( c ) == 0 && curl.easy_getinfo( c, HTTP_CURLINFO_RESPONSE_CODE, &code ) == 0 )
			req->status = (int)code;
		curl.easy_cleanup( c );
	}
	HTTP_Finish( req );
	return NULL;
}

static qboolean HTTP_StartThread( httpRequest_t *req, char *error, int errorSize ) {
	pthread_t thread;

	if ( !HTTP_LoadCurl() ) {
		Q_strncpyz( error, "libcurl isn't on this computer", errorSize );
		return qfalse;
	}
	if ( pthread_create( &thread, NULL, HTTP_Thread, req ) != 0 ) {
		Q_strncpyz( error, "couldn't start asking", errorSize );
		return qfalse;
	}
	pthread_detach( thread );
	return qtrue;
}
#endif

qboolean HTTP_Start( httpRequest_t *req, const char *url, const char *body, char *error, int errorSize ) {
	if ( req->state == HTTP_BUSY ) {
		Q_strncpyz( error, "still waiting for an answer", errorSize );
		return qfalse;
	}
	Q_strncpyz( req->url, url, sizeof( req->url ) );
	req->send = NULL;
	req->sendLen = 0;
	if ( body ) {
		req->sendLen = (int)strlen( body );
		req->send = (char *)malloc( req->sendLen + 1 );
		if ( !req->send ) {
			Q_strncpyz( error, "out of memory", errorSize );
			return qfalse;
		}
		memcpy( req->send, body, req->sendLen + 1 );
	}
	req->finished = 0;
	req->status = 0;
	if ( !HTTP_StartThread( req, error, errorSize ) ) {
		free( req->send );
		req->send = NULL;
		req->state = HTTP_IDLE;
		return qfalse;
	}
	req->state = HTTP_BUSY;
	return qtrue;
}

qboolean HTTP_Poll( httpRequest_t *req ) {
	if ( req->state != HTTP_BUSY || !HTTP_Finished( req ) )
		return qfalse;
	free( req->send );
	req->send = NULL;
	req->state = HTTP_DONE;
	return qtrue;
}
