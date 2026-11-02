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

// cl_update.cpp -- whether this is the newest OpenJK-rpmod
//
// At start, at most once a day, the newest release is asked of GitHub, out of the way of the
// game. When there's a newer one than this, it's said in the console and shown on the main menu.
//
//   version          this version, the newest, whether this is up to date, and where to get it
//   version check    asks GitHub again now
//
// cl_updateCheck 0 never asks. A build between releases is named after the next one
// ("v0.7.4-dev-g5cc1ce8", the build's CMake): it's out of date once that release is out.
// Only Windows can ask for now.

#include "client.h"
#include "qcommon/game_version.h"
#include <time.h>

#if defined(_WIN32)
#include <windows.h>
#include <winhttp.h>
#endif

#define UP_HOST			"api.github.com"
#define UP_PATH			"/repos/Ber-til/OpenJK-rpmod/releases/latest"
#define UP_PAGE			"https://github.com/Ber-til/OpenJK-rpmod/releases/latest"
#define UP_EVERY		( 24 * 60 * 60 )	// seconds between asks
#define UP_TIMEOUT_MS	8000
#define UP_BODY			65536

typedef enum {
	UP_IDLE,
	UP_ASKING,
	UP_DONE,
	UP_FAILED
} upState_t;

static cvar_t *cl_updateCheck;
static cvar_t *cl_updateLast;			// "<time asked> <newest tag>", kept between runs

static struct {
	upState_t		state;
	qboolean		show;				// asked from the console: the answer is said either way
	char			latest[64];			// the newest release's tag, "" not known
	char			error[128];
	volatile long	finished;			// the asking thread is done with what follows
	char			body[UP_BODY];
	int				status;
} up;

// "v0.7.3" into its numbers; qfalse when it isn't one. *dev: more after them, a build between releases
static qboolean UP_Parse( const char *tag, int v[3], qboolean *dev ) {
	const char *p = tag;

	if ( *p == 'v' || *p == 'V' )
		p++;
	for ( int i = 0; i < 3; i++ ) {
		if ( !isdigit( (unsigned char)*p ) )
			return qfalse;
		v[i] = atoi( p );
		while ( isdigit( (unsigned char)*p ) )
			p++;
		if ( i < 2 ) {
			if ( *p != '.' )
				return qfalse;
			p++;
		}
	}
	if ( dev )
		*dev = (qboolean)( *p != '\0' );
	return qtrue;
}

// -1 this one's out of date, 0 up to date, 1 newer than the newest (a build towards the next
// release), 2 not known
static int UP_Compare( void ) {
	int mine[3], newest[3];
	qboolean dev;

	if ( !up.latest[0] || !UP_Parse( GIT_TAG, mine, &dev ) || !UP_Parse( up.latest, newest, NULL ) )
		return 2;
	for ( int i = 0; i < 3; i++ ) {
		if ( mine[i] != newest[i] )
			return mine[i] > newest[i] ? 1 : -1;
	}
	// a build towards this version, from before it came out
	return dev ? -1 : 0;
}

static void UP_Say( void ) {
	switch ( UP_Compare() ) {
	case -1:
		Com_Printf( S_COLOR_YELLOW "OpenJK-rpmod %s is out, you have %s: " UP_PAGE "\n", up.latest, GIT_TAG );
		break;
	case 0:
		Com_Printf( "OpenJK-rpmod %s is the newest\n", GIT_TAG );
		break;
	case 1:
		Com_Printf( "OpenJK-rpmod %s, a build towards the next release (newest %s)\n", GIT_TAG, up.latest );
		break;
	default:
		Com_Printf( "OpenJK-rpmod %s, newest release %s\n", GIT_TAG, up.latest[0] ? up.latest : "not known" );
		break;
	}
}

// the tag in GitHub's answer: "tag_name": "v0.7.4"
static qboolean UP_ReadTag( const char *body, char *tag, int size ) {
	const char *p = strstr( body, "\"tag_name\"" ), *end;

	if ( !p || !( p = strchr( p + 10, ':' ) ) || !( p = strchr( p, '"' ) ) || !( end = strchr( p + 1, '"' ) ) )
		return qfalse;
	p++;
	if ( end - p <= 0 || end - p >= size )
		return qfalse;
	Q_strncpyz( tag, p, (int)( end - p ) + 1 );
	return qtrue;
}

#if defined(_WIN32)
// the asking, on its own thread: only up.body, up.status and up.finished are touched
static DWORD WINAPI UP_Thread( LPVOID ) {
	HINTERNET session = NULL, connect = NULL, request = NULL;
	DWORD code = 0, size = sizeof( code ), got = 0;
	int len = 0;

	up.status = 0;
	up.body[0] = '\0';
	session = WinHttpOpen( L"OpenJK-rpmod", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0 );
	if ( session ) {
		WinHttpSetTimeouts( session, UP_TIMEOUT_MS, UP_TIMEOUT_MS, UP_TIMEOUT_MS, UP_TIMEOUT_MS );
		connect = WinHttpConnect( session, L"" UP_HOST, INTERNET_DEFAULT_HTTPS_PORT, 0 );
	}
	if ( connect )
		request = WinHttpOpenRequest( connect, L"GET", L"" UP_PATH, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE );
	if ( request
		&& WinHttpAddRequestHeaders( request, L"Accept: application/vnd.github+json", (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD )
		&& WinHttpSendRequest( request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0 )
		&& WinHttpReceiveResponse( request, NULL ) ) {
		if ( WinHttpQueryHeaders( request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &code, &size, WINHTTP_NO_HEADER_INDEX ) )
			up.status = (int)code;
		while ( len < UP_BODY - 1 && WinHttpReadData( request, up.body + len, UP_BODY - 1 - len, &got ) && got )
			len += (int)got;
		up.body[len] = '\0';
	}
	if ( request )
		WinHttpCloseHandle( request );
	if ( connect )
		WinHttpCloseHandle( connect );
	if ( session )
		WinHttpCloseHandle( session );
	InterlockedExchange( &up.finished, 1 );
	return 0;
}
#endif

static void UP_Ask( qboolean show ) {
	if ( up.state == UP_ASKING )
		return;
	up.show = show;
#if defined(_WIN32)
	{
		HANDLE thread;

		up.finished = 0;
		thread = CreateThread( NULL, 0, UP_Thread, NULL, 0, NULL );
		if ( thread ) {
			CloseHandle( thread );
			up.state = UP_ASKING;
			if ( show )
				Com_Printf( "Asking GitHub for the newest OpenJK-rpmod...\n" );
			return;
		}
		Q_strncpyz( up.error, "couldn't start asking", sizeof( up.error ) );
	}
#else
	Q_strncpyz( up.error, "only Windows can ask for now", sizeof( up.error ) );
#endif
	up.state = UP_FAILED;
	if ( show )
		Com_Printf( "Update check: %s\n", up.error );
}

// the answer in: kept, and said
void CL_Update_Frame( void ) {
	char tag[64];

	if ( up.state != UP_ASKING || !up.finished )
		return;
	if ( up.status == 200 && UP_ReadTag( up.body, tag, sizeof( tag ) ) ) {
		Q_strncpyz( up.latest, tag, sizeof( up.latest ) );
		Cvar_Set( "cl_updateLast", va( "%lld %s", (long long)time( NULL ), up.latest ) );
		up.state = UP_DONE;
		if ( up.show || UP_Compare() == -1 )
			UP_Say();
	} else {
		Com_sprintf( up.error, sizeof( up.error ), up.status ? "GitHub answered %i" : "GitHub couldn't be reached", up.status );
		up.state = UP_FAILED;
		if ( up.show )
			Com_Printf( "Update check: %s\n", up.error );
		else
			Com_DPrintf( "Update check: %s\n", up.error );
	}
}

// on the main menu, when there's a newer one
void CL_Update_Draw( void ) {
	static const vec4_t yellow = { 1.0f, 0.85f, 0.2f, 1.0f };
	const char *text;
	qhandle_t font;
	float scale, w;

	if ( cls.state != CA_DISCONNECTED || !( Key_GetCatcher() & KEYCATCH_UI ) || UP_Compare() != -1 )
		return;
	font = re->RegisterFont( "arialnb" );
	if ( !font )
		return;
	text = va( "OpenJK-rpmod %s is out (you have %s): /version for where to get it", up.latest, GIT_TAG );
	scale = 0.6f;
	w = (float)re->Font_StrLenPixels( text, font, scale );
	// at the bottom: overlays like ReShade's take the top
	re->Font_DrawString( (int)( ( SCREEN_WIDTH - w ) * 0.5f ), SCREEN_HEIGHT - 12, text, yellow, font | STYLE_DROPSHADOW, -1, scale );
}

static void UP_Version_f( void ) {
	const char *arg = Cmd_Argv( 1 );

	if ( !Q_stricmp( arg, "check" ) ) {
		UP_Ask( qtrue );
		return;
	}
	Com_Printf( "%s\n", Cvar_VariableString( "version" ) );
	if ( up.state == UP_ASKING )
		Com_Printf( "Asking GitHub for the newest...\n" );
	else if ( up.latest[0] )
		UP_Say();
	else
		Com_Printf( "Newest release not known%s%s\n", up.error[0] ? ": " : "", up.error );
	Com_Printf( "version check : ask GitHub again\n" );
}

void CL_Update_Init( void ) {
	long long when = 0;
	char tag[64] = "";

	// still asking from before: the thread has it
	if ( up.state != UP_ASKING )
		memset( &up, 0, sizeof( up ) );
	cl_updateCheck = Cvar_Get( "cl_updateCheck", "1", CVAR_ARCHIVE_ND,
		"1 asks GitHub at start, once a day at most, whether there's a newer OpenJK-rpmod" );
	cl_updateLast = Cvar_Get( "cl_updateLast", "", CVAR_ARCHIVE_ND | CVAR_INTERNAL, "When GitHub was last asked, and the newest then" );
	Cmd_AddCommand( "version", UP_Version_f, "This OpenJK-rpmod's version, and whether it's the newest" );

	if ( !cl_updateCheck->integer || up.state == UP_ASKING )
		return;
	// asked lately: what it said then stands
	if ( sscanf( cl_updateLast->string, "%lld %63s", &when, tag ) == 2 )
		Q_strncpyz( up.latest, tag, sizeof( up.latest ) );
	if ( up.latest[0] && when > 0 && (long long)time( NULL ) - when < UP_EVERY && (long long)time( NULL ) >= when ) {
		up.state = UP_DONE;
		if ( UP_Compare() == -1 )
			UP_Say();
		return;
	}
	UP_Ask( qfalse );
}

void CL_Update_Shutdown( void ) {
	Cmd_RemoveCommand( "version" );
}
