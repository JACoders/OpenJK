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

// cl_discord.cpp -- Discord Rich Presence: what you're playing, on your Discord profile
//
// Spoken straight to the Discord app on this computer, through its local pipe, as Discord's own
// libraries do: a handshake with the application's ID, then SET_ACTIVITY each time what's shown
// changes. Nothing happens when Discord isn't running; it's looked for again every while.
//
// It shows the server ("On JEDI Temple"), who you play ("Playing as ..."), the players there and the
// time since you joined, and the map's picture: the application's art asset named after the map
// (lowercase, _ for anything else), or its "logo" when it has none. Which it has is asked of
// Discord once, in the background. A Join JEDI button goes to JEDI's Discord.
//
//   cl_discord 0          nothing goes to Discord
//   cl_discordPrivate 1   no server name or character name, only the map and the players
//   cl_discordAppId       the Discord application shown

#include "client.h"
#include <time.h>

#define DC_DEFAULT_APP		"1567013016313143328"
#define DC_INVITE			"https://discord.jediholo.net/"
#define DC_RETRY_MS			15000	// Discord looked for this often while it isn't there
#define DC_MIN_SEND_MS		5000	// Discord takes five updates in twenty seconds
#define DC_MAX_ASSETS		300
#define DC_KEY_LEN			33

static cvar_t *cl_discord;
static cvar_t *cl_discordPrivate;
static cvar_t *cl_discordAppId;

// the application's art, asked of Discord in the background: 2 once it's in
static char				dcAssets[DC_MAX_ASSETS][DC_KEY_LEN];
static int				dcNumAssets;
static volatile int		dcAssetsState;
static char				dcAssetsApp[32];

// a map's picture by its name: lowercase, _ for anything else, as the art was named
static void DC_Key( const char *map, char *out, int size ) {
	const char *base = COM_SkipPath( (char *)map );
	int len = 0;

	for ( ; *base && *base != '.' && len < size - 1 && len < DC_KEY_LEN - 1; base++ )
		out[len++] = isalnum( (unsigned char)*base ) ? (char)tolower( (unsigned char)*base ) : '_';
	out[len] = '\0';
}

static qboolean DC_HasAsset( const char *key ) {
	if ( dcAssetsState != 2 )
		return qfalse;
	for ( int i = 0; i < dcNumAssets; i++ ) {
		if ( !Q_stricmp( dcAssets[i], key ) )
			return qtrue;
	}
	return qfalse;
}

// text for JSON: quotes and backslashes escaped, Latin-1 made \u00XX, controls left out
static void DC_Json( const char *in, char *out, int size ) {
	int len = 0;

	for ( ; *in && len < size - 8; in++ ) {
		const unsigned char c = (unsigned char)*in;

		if ( c == '"' || c == '\\' ) {
			out[len++] = '\\';
			out[len++] = (char)c;
		} else if ( c >= 0x80 ) {
			len += Com_sprintf( out + len, size - len, "\\u%04x", c );
		} else if ( c >= 0x20 ) {
			out[len++] = (char)c;
		}
	}
	out[len] = '\0';
}

// colour codes out: the game's ^0-^9 and RPMod's ^A-^Z and ^a-^z
void CL_StripAllColors( char *text ) {
	char *out = text;

	for ( const char *p = text; *p; p++ ) {
		if ( p[0] == Q_COLOR_ESCAPE && isalnum( (unsigned char)p[1] ) ) {
			p++;
			continue;
		}
		*out++ = *p;
	}
	*out = '\0';
}

// a name as it reads: colours out, spaces trimmed
static void DC_Plain( const char *in, char *out, int size ) {
	char *start;
	int len;

	Q_strncpyz( out, in, size );
	CL_StripAllColors( out );
	for ( start = out; *start == ' '; start++ )
		;
	memmove( out, start, strlen( start ) + 1 );
	for ( len = strlen( out ); len > 0 && out[len - 1] == ' '; len-- )
		out[len - 1] = '\0';
}

#ifdef _WIN32

#include <windows.h>
#include <winhttp.h>

static struct {
	HANDLE		pipe;
	qboolean	ready;
	int			nextConnect;
	int			lastSend;
	int			nonce;
	char		sent[4096];			// the last activity sent
	char		server[MAX_QPATH];	// joined then, the time counted from
	int64_t		joinedAt;
	int64_t		startedAt;
	HANDLE		assetThread;
	qboolean	assetsSaid;
} dc;

/*
===============================================================================

THE APPLICATION'S ART

===============================================================================
*/

static DWORD WINAPI DC_AssetThread( LPVOID arg ) {
	wchar_t path[128];
	char body[65536];
	DWORD got, total = 0;
	HINTERNET session, connect = NULL, request = NULL;
	qboolean ok = qfalse;

	swprintf( path, ARRAY_LEN( path ), L"/api/v9/oauth2/applications/%hs/assets", dcAssetsApp );
	session = WinHttpOpen( L"OpenJK-rpmod", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0 );
	if ( session ) {
		WinHttpSetTimeouts( session, 8000, 8000, 8000, 8000 );
		connect = WinHttpConnect( session, L"discord.com", INTERNET_DEFAULT_HTTPS_PORT, 0 );
	}
	if ( connect )
		request = WinHttpOpenRequest( connect, L"GET", path, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE );
	if ( request && WinHttpSendRequest( request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0 )
		&& WinHttpReceiveResponse( request, NULL ) ) {
		while ( total < sizeof( body ) - 1 && WinHttpReadData( request, body + total, sizeof( body ) - 1 - total, &got ) && got )
			total += got;
		body[total] = '\0';
		ok = qtrue;
	}
	if ( request )
		WinHttpCloseHandle( request );
	if ( connect )
		WinHttpCloseHandle( connect );
	if ( session )
		WinHttpCloseHandle( session );

	// [{"id": "...", "type": 1, "name": "duneeden_temple"}, ...]
	dcNumAssets = 0;
	for ( const char *p = ok ? body : ""; ( p = strstr( p, "\"name\"" ) ) != NULL && dcNumAssets < DC_MAX_ASSETS; ) {
		const char *q;
		int len = 0;

		p = strchr( p + 6, '"' );
		if ( !p )
			break;
		for ( q = p + 1; *q && *q != '"' && len < DC_KEY_LEN - 1; q++ )
			dcAssets[dcNumAssets][len++] = *q;
		dcAssets[dcNumAssets][len] = '\0';
		dcNumAssets++;
		p = q;
	}
	dcAssetsState = 2;
	return 0;
}

static void DC_FetchAssets( void ) {
	if ( dcAssetsState == 1 || ( dcAssetsState == 2 && !Q_stricmp( dcAssetsApp, cl_discordAppId->string ) ) )
		return;
	Q_strncpyz( dcAssetsApp, cl_discordAppId->string, sizeof( dcAssetsApp ) );
	dcAssetsState = 1;
	dc.assetsSaid = qfalse;
	if ( dc.assetThread )
		CloseHandle( dc.assetThread );
	dc.assetThread = CreateThread( NULL, 0, DC_AssetThread, NULL, 0, NULL );
	if ( !dc.assetThread )
		dcAssetsState = 2;
}

/*
===============================================================================

THE PIPE

===============================================================================
*/

static void DC_Close( void ) {
	if ( dc.pipe != INVALID_HANDLE_VALUE && dc.pipe )
		CloseHandle( dc.pipe );
	dc.pipe = INVALID_HANDLE_VALUE;
	dc.ready = qfalse;
	dc.sent[0] = '\0';
}

// a frame: its opcode, its length, then the JSON
static qboolean DC_Write( int op, const char *json ) {
	const int len = (int)strlen( json );
	char buf[8192];
	DWORD wrote;

	if ( dc.pipe == INVALID_HANDLE_VALUE || len + 8 > (int)sizeof( buf ) )
		return qfalse;
	memcpy( buf, &op, 4 );
	memcpy( buf + 4, &len, 4 );
	memcpy( buf + 8, json, len );
	if ( !WriteFile( dc.pipe, buf, len + 8, &wrote, NULL ) || wrote != (DWORD)( len + 8 ) ) {
		DC_Close();
		return qfalse;
	}
	return qtrue;
}

static void DC_Connect( void ) {
	for ( int i = 0; i < 10; i++ ) {
		dc.pipe = CreateFileA( va( "\\\\.\\pipe\\discord-ipc-%i", i ), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL );
		if ( dc.pipe != INVALID_HANDLE_VALUE )
			break;
	}
	if ( dc.pipe == INVALID_HANDLE_VALUE )
		return;
	if ( DC_Write( 0, va( "{\"v\":1,\"client_id\":\"%s\"}", cl_discordAppId->string ) ) )
		Com_DPrintf( "Discord: connected\n" );
}

// what Discord says: ready, a ping answered, closed with its reason
static void DC_Read( void ) {
	char body[8192];
	DWORD avail, got;

	while ( dc.pipe != INVALID_HANDLE_VALUE ) {
		int head[2];

		if ( !PeekNamedPipe( dc.pipe, NULL, 0, NULL, &avail, NULL ) ) {
			DC_Close();
			return;
		}
		if ( avail < 8 )
			return;
		if ( !ReadFile( dc.pipe, head, 8, &got, NULL ) || got != 8 || head[1] < 0 ) {
			DC_Close();
			return;
		}
		got = 0;
		if ( head[1] > 0 ) {
			const DWORD want = (DWORD)Q_min( head[1], (int)sizeof( body ) - 1 );

			if ( !ReadFile( dc.pipe, body, want, &got, NULL ) ) {
				DC_Close();
				return;
			}
			// more than fits is read and let go
			for ( int left = head[1] - (int)want; left > 0; ) {
				char skip[512];
				DWORD n;

				if ( !ReadFile( dc.pipe, skip, (DWORD)Q_min( left, (int)sizeof( skip ) ), &n, NULL ) || !n )
					break;
				left -= (int)n;
			}
		}
		body[got] = '\0';
		switch ( head[0] ) {
		case 1:		// a frame
			if ( strstr( body, "\"READY\"" ) ) {
				dc.ready = qtrue;
				Com_DPrintf( "Discord: ready\n" );
			} else if ( strstr( body, "SET_ACTIVITY" ) && !strstr( body, "\"ERROR\"" ) ) {
				Com_DPrintf( "Discord: status set\n" );
			}
			if ( strstr( body, "\"READY\"" ) )
				break;
			else if ( strstr( body, "\"ERROR\"" ) )
				Com_DPrintf( "Discord: %s\n", body );
			break;
		case 2:		// closed: a wrong application ID, mostly
			Com_DPrintf( "Discord closed the connection: %s\n", body );
			DC_Close();
			dc.nextConnect = cls.realtime + DC_RETRY_MS * 4;
			return;
		case 3:		// ping
			DC_Write( 4, body );
			break;
		default:
			break;
		}
	}
}

/*
===============================================================================

WHAT'S SHOWN

===============================================================================
*/

// the activity as it is now, as JSON
static void DC_Activity( char *out, int size ) {
	const qboolean hide = (qboolean)( cl_discordPrivate && cl_discordPrivate->integer );
	char details[256], state[256], image[DC_KEY_LEN], imageText[MAX_QPATH], party[MAX_QPATH];
	int players = 0, maxPlayers = 0;
	int64_t start;

	if ( cls.state == CA_ACTIVE ) {
		const char *serverInfo = cl.gameState.stringData + cl.gameState.stringOffsets[CS_SERVERINFO];
		char host[MAX_QPATH], name[MAX_QPATH], map[MAX_QPATH];
		const int ofs = cl.gameState.stringOffsets[CS_PLAYERS + clc.clientNum];
		const char *addr = NET_AdrToString( &clc.serverAddress );

		// the time counted from when this server was joined
		if ( Q_stricmp( dc.server, addr ) ) {
			Q_strncpyz( dc.server, addr, sizeof( dc.server ) );
			dc.joinedAt = (int64_t)time( NULL );
		}
		start = dc.joinedAt;
		DC_Plain( Info_ValueForKey( serverInfo, "sv_hostname" ), host, sizeof( host ) );
		DC_Plain( ofs ? Info_ValueForKey( cl.gameState.stringData + ofs, "n" ) : "", name, sizeof( name ) );
		for ( int c = 0; c < MAX_CLIENTS; c++ ) {
			const int o = cl.gameState.stringOffsets[CS_PLAYERS + c];

			if ( o && cl.gameState.stringData[o] )
				players++;
		}
		maxPlayers = atoi( Info_ValueForKey( serverInfo, "sv_maxclients" ) );

		COM_StripExtension( COM_SkipPath( cl.mapname ), map, sizeof( map ) );
		DC_Key( map, image, sizeof( image ) );
		if ( !DC_HasAsset( image ) )
			Q_strncpyz( image, "logo", sizeof( image ) );
		DC_Json( map, imageText, sizeof( imageText ) );

		DC_Json( hide || !host[0] ? "On a server" : va( "On %s", host ), details, sizeof( details ) );
		DC_Json( hide || !name[0] ? "Playing" : va( "Playing as %s", name ), state, sizeof( state ) );
		DC_Json( addr, party, sizeof( party ) );
	} else {
		dc.server[0] = '\0';
		start = dc.startedAt;
		Q_strncpyz( image, "logo", sizeof( image ) );
		Q_strncpyz( imageText, "OpenJK-rpmod", sizeof( imageText ) );
		Q_strncpyz( details, cls.state >= CA_CONNECTING ? "Joining a server" : "In the main menu", sizeof( details ) );
		state[0] = party[0] = '\0';
	}

	Com_sprintf( out, size, "{\"details\":\"%s\"", details );
	if ( state[0] )
		Q_strcat( out, size, va( ",\"state\":\"%s\"", state ) );
	if ( party[0] && maxPlayers > 0 )
		Q_strcat( out, size, va( ",\"party\":{\"id\":\"%s\",\"size\":[%i,%i]}", party, Q_max( 1, players ), maxPlayers ) );
	Q_strcat( out, size, va( ",\"timestamps\":{\"start\":%lld}", (long long)start ) );
	Q_strcat( out, size, va( ",\"assets\":{\"large_image\":\"%s\",\"large_text\":\"%s\"}", image, imageText ) );
	Q_strcat( out, size, ",\"buttons\":[{\"label\":\"Join JEDI\",\"url\":\"" DC_INVITE "\"}]}" );
}

void CL_Discord_Frame( void ) {
	char activity[4096];

	if ( !cl_discord )
		return;
	if ( !cl_discord->integer || !cl_discordAppId->string[0] ) {
		// off: closing it takes us off the profile
		if ( dc.pipe != INVALID_HANDLE_VALUE )
			DC_Close();
		return;
	}
	if ( cl_discord->modified || cl_discordAppId->modified ) {
		cl_discord->modified = cl_discordAppId->modified = qfalse;
		DC_Close();
		dc.nextConnect = 0;
	}
	DC_FetchAssets();
	// said once the art's in, from here: the thread doesn't print
	if ( dcAssetsState == 2 && !dc.assetsSaid ) {
		dc.assetsSaid = qtrue;
		Com_DPrintf( "Discord: the application has %i pictures\n", dcNumAssets );
	}
	if ( dc.pipe == INVALID_HANDLE_VALUE ) {
		if ( cls.realtime < dc.nextConnect )
			return;
		dc.nextConnect = cls.realtime + DC_RETRY_MS;
		DC_Connect();
		return;
	}
	DC_Read();
	if ( !dc.ready || cls.realtime - dc.lastSend < DC_MIN_SEND_MS )
		return;
	DC_Activity( activity, sizeof( activity ) );
	if ( !strcmp( activity, dc.sent ) )
		return;
	if ( DC_Write( 1, va( "{\"cmd\":\"SET_ACTIVITY\",\"args\":{\"pid\":%lu,\"activity\":%s},\"nonce\":\"%i\"}",
		(unsigned long)GetCurrentProcessId(), activity, ++dc.nonce ) ) ) {
		Q_strncpyz( dc.sent, activity, sizeof( dc.sent ) );
		dc.lastSend = cls.realtime;
	}
}

void CL_Discord_Init( void ) {
	memset( &dc, 0, sizeof( dc ) );
	dc.pipe = INVALID_HANDLE_VALUE;
	dc.startedAt = (int64_t)time( NULL );
	cl_discord = Cvar_Get( "cl_discord", "0", CVAR_ARCHIVE_ND, "Show what you're playing on your Discord profile" );
	cl_discordPrivate = Cvar_Get( "cl_discordPrivate", "0", CVAR_ARCHIVE_ND, "Discord shows no server name or character name" );
	cl_discordAppId = Cvar_Get( "cl_discordAppId", DC_DEFAULT_APP, CVAR_ARCHIVE_ND, "The Discord application Rich Presence shows" );
}

void CL_Discord_Shutdown( void ) {
	DC_Close();
}

#else	// not Windows: nothing yet

void CL_Discord_Init( void ) {
	cl_discord = Cvar_Get( "cl_discord", "0", CVAR_ARCHIVE_ND, "Show what you're playing on your Discord profile (Windows)" );
	cl_discordPrivate = Cvar_Get( "cl_discordPrivate", "0", CVAR_ARCHIVE_ND, "Discord shows no server name or character name" );
	cl_discordAppId = Cvar_Get( "cl_discordAppId", DC_DEFAULT_APP, CVAR_ARCHIVE_ND, "The Discord application Rich Presence shows" );
}
void CL_Discord_Frame( void ) {}
void CL_Discord_Shutdown( void ) {}

#endif
