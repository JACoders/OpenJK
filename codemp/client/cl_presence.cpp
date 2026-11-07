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

// cl_presence.cpp -- the hooks (rpnotify -1 1 cmd:..., see cl_rphooks.cpp) only to the players with
// this client
//
// Sent to everyone, a hook shows as a "cmd:" line to the players without this client. So who has
// it is kept by the feedback bot (cl_presenceUrl, the same bot as cl_feedbackUrl): each player's
// client tells it, once loaded into a server, which server and client number it is, its name and
// version, again when the map, the number or the name changes and every few minutes, and that it's
// gone as it disconnects. Nothing in the game shows it.
//
// A hook going out is held here: the bot is asked who on the server has the client, and the hook
// goes to them alone, the sender first. A player counts only while the name in their slot is the one
// they told the bot, so a number taken over by someone else gets nothing.
//
//   rpnotify <client number> 1 cmd:...   once for each, one a gap apart (the server's flood
//                                        protection, or cl_hookGap)
//
// While hooks are going out, the other commands wait behind them, so the order stays and the flood
// protection lets each through. A hook that can't be sent to the right players (the bot doesn't
// answer) isn't sent. On a local server, or with cl_presenceUrl empty, hooks go to everyone as before.

#include "client.h"
#include "cl_http.h"
#include "qcommon/game_version.h"
#include <deque>
#include <string>

#define PS_DEFAULT_URL		"https://vault.tail508ce5.ts.net"	// the feedback bot, as cl_feedbackUrl
#define PS_HOOK				"rpnotify -1 1 cmd:"
#define PS_HOOK_TO			"rpnotify -1 1 "	// what the players to send it to go in place of
#define PS_BEAT_MS			( 5 * 60 * 1000 )	// the bot told again this often (it forgets after 6 minutes)
#define PS_RETRY_MS			30000				// after the bot didn't answer
#define PS_LIST_FRESH_MS	30000				// the list asked again past this, while hooks go out
#define PS_LIST_USABLE_MS	( 2 * 60 * 1000 )	// a hook sent with a list this old at most
#define PS_WAIT_MS			5000				// a hook waits this long for the list, then isn't sent
#define PS_JOINED_MS		8000				// after someone joins: time for their client to tell the bot
#define PS_NOTE_MS			60000				// "not sent" said at most this often
#define PS_QUEUE_MAX		4096
#define PS_NAME_LEN			128

typedef struct {
	int		num;
	char	name[PS_NAME_LEN];
	char	version[32];
} psPlayer_t;

typedef struct {
	std::string	cmd;
	int			time;
	qboolean	hook;
} psWaiting_t;

static struct {
	char		server[64];			// the server all this is about

	// what the bot was told
	char		beatKey[512];
	int			beatTime;
	int			beatNext;			// not before this, after a failure
	qboolean	beatOn;				// the bot knows of us: told when we go
	char		beatServer[64];
	int			beatClient;

	// what the bot said
	qboolean	known;
	int			listTime;
	psPlayer_t	players[MAX_CLIENTS];
	int			numPlayers;
	int			askAt;				// the list asked again then (0: not)
	int			askNext;			// not before this, after a failure
	qboolean	print;				// /clientusers is waiting for it

	char		slots[MAX_CLIENTS][PS_NAME_LEN];	// the names in the player slots, to see who joins
	int			lastHook;
	int			noteTime;
	qboolean	sending;			// our own command going out: let through
} ps;

static std::deque<psWaiting_t>	psWaiting;	// waiting for the list, in order
static std::deque<std::string>	psQueue;	// going out, one a gap apart
static httpRequest_t			psBeatHttp, psListHttp, psLeaveHttp;
static cvar_t					*cl_presenceUrl, *cl_hookGap;

/*
===============================================================================

What's around

===============================================================================
*/

static qboolean PS_Active( void ) {
	return (qboolean)( cl_presenceUrl && cl_presenceUrl->string[0] && cls.state == CA_ACTIVE && !clc.demoplaying
		&& !( com_sv_running && com_sv_running->integer ) );
}

// a name as it's compared: no colours, spaces as one, lower case (the bot keeps it as sent, its
// spaces as one)
static void PS_Normalize( const char *in, char *out, int size ) {
	int len = 0;
	qboolean space = qfalse;

	for ( ; *in && len < size - 1; in++ ) {
		if ( Q_IsColorString( in ) ) {
			in++;
			continue;
		}
		if ( (unsigned char)*in <= ' ' ) {
			space = (qboolean)( len > 0 );
			continue;
		}
		if ( space && len < size - 2 )
			out[len++] = ' ';
		space = qfalse;
		out[len++] = (char)tolower( (unsigned char)*in );
	}
	out[len] = '\0';
}

// the name in a player slot, as the server has it; "" when nobody's there
static const char *PS_SlotName( int num ) {
	int ofs;

	if ( num < 0 || num >= MAX_CLIENTS || !( ofs = cl.gameState.stringOffsets[CS_PLAYERS + num] ) )
		return "";
	return Info_ValueForKey( cl.gameState.stringData + ofs, "n" );
}

// what the server lets through: one command a second, unless it says otherwise
static int PS_Gap( void ) {
	const char *info, *flood;
	int value;

	if ( cl_hookGap->integer > 0 )
		return cl_hookGap->integer;
	info = cl.gameState.stringData + cl.gameState.stringOffsets[CS_SERVERINFO];
	flood = Info_ValueForKey( info, "sv_floodProtect" );
	if ( !flood[0] )
		return 1100;
	value = atoi( flood );
	if ( value <= 0 )
		return 0;
	return ( value == 1 ? 1000 : value ) + 100;
}

/*
===============================================================================

Talking to the bot

===============================================================================
*/

// a string for JSON, quoted: the game's letters past ASCII taken as Latin-1
static void PS_Json( std::string &out, const char *s ) {
	out += '"';
	for ( ; *s; s++ ) {
		const unsigned char c = (unsigned char)*s;

		if ( c == '"' || c == '\\' ) {
			out += '\\';
			out += (char)c;
		} else if ( c < 32 || c >= 127 ) {
			out += va( "\\u%04x", c );
		} else {
			out += (char)c;
		}
	}
	out += '"';
}

static const char *PS_Url( const char *path ) {
	static char url[512];
	int len;

	Q_strncpyz( url, cl_presenceUrl->string, sizeof( url ) );
	len = (int)strlen( url );
	while ( len > 0 && url[len - 1] == '/' )
		url[--len] = '\0';
	Q_strcat( url, sizeof( url ), path );
	return url;
}

static qboolean PS_Send( httpRequest_t *req, const char *path, const std::string &body ) {
	char error[128];

	if ( req->state == HTTP_BUSY )
		return qfalse;
	if ( !HTTP_Start( req, PS_Url( path ), body.c_str(), error, sizeof( error ) ) ) {
		Com_DPrintf( "clientusers: %s\n", error );
		return qfalse;
	}
	return qtrue;
}

// the bot told who we are: as we come in, when something changes, and every few minutes
static void PS_Beat( void ) {
	const char *name = PS_SlotName( clc.clientNum );
	char key[512];
	std::string body;

	if ( psBeatHttp.state == HTTP_BUSY || cls.realtime < ps.beatNext )
		return;
	if ( !name[0] )
		name = Cvar_VariableString( "name" );
	Com_sprintf( key, sizeof( key ), "%s|%i|%s|%s", ps.server, clc.clientNum, name, cl.mapname );
	if ( !strcmp( key, ps.beatKey ) && cls.realtime - ps.beatTime < PS_BEAT_MS )
		return;
	body = "{\"server\":";
	PS_Json( body, ps.server );
	body += va( ",\"client\":%i,\"name\":", clc.clientNum );
	PS_Json( body, name );
	body += ",\"version\":";
	PS_Json( body, GIT_TAG );
	body += "}";
	if ( !PS_Send( &psBeatHttp, "/v1/presence", body ) ) {
		ps.beatNext = cls.realtime + PS_RETRY_MS;
		return;
	}
	Q_strncpyz( ps.beatKey, key, sizeof( ps.beatKey ) );
	ps.beatTime = cls.realtime;
	Q_strncpyz( ps.beatServer, ps.server, sizeof( ps.beatServer ) );
	ps.beatClient = clc.clientNum;
	ps.beatOn = qtrue;
}

static void PS_AskList( void ) {
	std::string body = "{\"server\":";

	if ( psListHttp.state == HTTP_BUSY || cls.realtime < ps.askNext )
		return;
	PS_Json( body, ps.server );
	body += "}";
	if ( !PS_Send( &psListHttp, "/v1/presence/list", body ) )
		ps.askNext = cls.realtime + PS_RETRY_MS;
}

static void PS_PrintList( void ) {
	Com_Printf( "Players with this client on %s: %s\n", ps.server,
		!ps.known ? "not known (the bot didn't answer)" : "hooks go to each of them, one after the other" );
	for ( int i = 0; ps.known && i < ps.numPlayers; i++ ) {
		const psPlayer_t *p = &ps.players[i];
		char inSlot[PS_NAME_LEN], told[PS_NAME_LEN];

		PS_Normalize( PS_SlotName( p->num ), inSlot, sizeof( inSlot ) );
		PS_Normalize( p->name, told, sizeof( told ) );
		Com_Printf( "  %2i  %s^7  %s%s\n", p->num, p->name, p->version, strcmp( inSlot, told ) ? S_COLOR_YELLOW "  (not in that slot now: left out)" : "" );
	}
}

// ok
// player <tab> client number <tab> version <tab> name
static void PS_ReadList( const char *answer ) {
	char line[512];
	const char *at = answer, *end;
	qboolean first = qtrue;

	ps.numPlayers = 0;
	for ( ; *at; at = *end ? end + 1 : end ) {
		char *field[4];
		int n = 0;

		end = strchr( at, '\n' );
		if ( !end )
			end = at + strlen( at );
		Q_strncpyz( line, at, Q_min( (int)sizeof( line ), (int)( end - at ) + 1 ) );
		for ( char *f = line; f && n < 4; n++ ) {
			field[n] = f;
			f = strchr( f, '\t' );
			if ( f )
				*f++ = '\0';
		}
		if ( first ) {
			first = qfalse;
			if ( n < 1 || strcmp( field[0], "ok" ) )
				return;
			ps.known = qtrue;
			ps.listTime = cls.realtime;
			continue;
		}
		if ( n == 4 && !strcmp( field[0], "player" ) && ps.numPlayers < MAX_CLIENTS ) {
			psPlayer_t *p = &ps.players[ps.numPlayers];

			p->num = atoi( field[1] );
			if ( p->num < 0 || p->num >= MAX_CLIENTS )
				continue;
			Q_strncpyz( p->version, field[2], sizeof( p->version ) );
			Q_strncpyz( p->name, field[3], sizeof( p->name ) );
			ps.numPlayers++;
		}
	}
}

static void PS_PollRequests( void ) {
	if ( HTTP_Poll( &psBeatHttp ) && psBeatHttp.status != 200 ) {
		// try again in a while
		Com_DPrintf( "clientusers: the bot didn't take who we are (%i)\n", psBeatHttp.status );
		ps.beatKey[0] = '\0';
		ps.beatNext = cls.realtime + PS_RETRY_MS;
	}
	if ( HTTP_Poll( &psListHttp ) ) {
		if ( psListHttp.status == 200 ) {
			PS_ReadList( psListHttp.answer );
		} else {
			Com_DPrintf( "clientusers: the bot didn't say who has the client (%i)\n", psListHttp.status );
			ps.askNext = cls.realtime + PS_RETRY_MS;
		}
		if ( ps.print ) {
			ps.print = qfalse;
			PS_PrintList();
		}
	}
	HTTP_Poll( &psLeaveHttp );
}

/*
===============================================================================

Hooks going out

===============================================================================
*/

static qboolean PS_ListUsable( void ) {
	return (qboolean)( ps.known && cls.realtime - ps.listTime < PS_LIST_USABLE_MS );
}

// who a hook goes to: us first, then each with the client whose slot still has the name they told
static int PS_Targets( int *out ) {
	char inSlot[PS_NAME_LEN], told[PS_NAME_LEN];
	int n = 0;

	out[n++] = clc.clientNum;
	for ( int i = 0; i < ps.numPlayers; i++ ) {
		const psPlayer_t *p = &ps.players[i];
		int k;

		if ( p->num == clc.clientNum )
			continue;
		for ( k = 0; k < n && out[k] != p->num; k++ )
			;
		if ( k < n )
			continue;
		PS_Normalize( PS_SlotName( p->num ), inSlot, sizeof( inSlot ) );
		PS_Normalize( p->name, told, sizeof( told ) );
		if ( inSlot[0] && !strcmp( inSlot, told ) )
			out[n++] = p->num;
	}
	return n;
}

static void PS_Queue( const std::string &cmd ) {
	if ( (int)psQueue.size() >= PS_QUEUE_MAX ) {
		Com_Printf( S_COLOR_YELLOW "Hooks: too many waiting to go out; one left out\n" );
		return;
	}
	psQueue.push_back( cmd );
}

// a hook made into what goes to the server: one for each player
static void PS_Expand( const std::string &cmd ) {
	const std::string rest = cmd.substr( strlen( PS_HOOK_TO ) );
	int targets[MAX_CLIENTS + 1];
	const int count = PS_Targets( targets );

	for ( int i = 0; i < count; i++ )
		PS_Queue( va( "rpnotify %i 1 ", targets[i] ) + rest );
}

// the hooks waiting for the list: sent once it's known, not sent if it doesn't come
static void PS_FlushWaiting( qboolean send ) {
	int dropped = 0;

	while ( !psWaiting.empty() ) {
		const psWaiting_t &w = psWaiting.front();

		if ( !w.hook )
			PS_Queue( w.cmd );
		else if ( send )
			PS_Expand( w.cmd );
		else
			dropped++;
		psWaiting.pop_front();
	}
	if ( dropped && ( !ps.noteTime || cls.realtime - ps.noteTime >= PS_NOTE_MS ) ) {
		ps.noteTime = cls.realtime;
		Com_Printf( S_COLOR_YELLOW "Hooks: %i not sent, as who has this client isn't known (the feedback bot didn't answer)\n", dropped );
	}
}

// everything about the server forgotten: a new one, or none
static void PS_Reset( void ) {
	psWaiting.clear();
	psQueue.clear();
	ps.known = qfalse;
	ps.numPlayers = 0;
	ps.askAt = 0;
	ps.askNext = 0;
	memset( ps.slots, 0, sizeof( ps.slots ) );
}

// someone in a slot who wasn't: the list asked again once their client has had time to tell the bot
static void PS_WatchJoins( void ) {
	char name[PS_NAME_LEN];

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		PS_Normalize( PS_SlotName( i ), name, sizeof( name ) );
		if ( !strcmp( name, ps.slots[i] ) )
			continue;
		Q_strncpyz( ps.slots[i], name, sizeof( ps.slots[i] ) );
		if ( name[0] && i != clc.clientNum && ps.known )
			ps.askAt = cls.realtime + PS_JOINED_MS;
	}
}

// from CL_AddReliableCommand: qtrue when it's taken here, to go out later
qboolean CL_Presence_Outgoing( const char *cmd ) {
	psWaiting_t w;

	if ( ps.sending || !PS_Active() )
		return qfalse;
	w.hook = (qboolean)!Q_stricmpn( cmd, PS_HOOK, strlen( PS_HOOK ) );
	if ( !w.hook && psWaiting.empty() && psQueue.empty() )
		return qfalse;	// nothing going out: as it always went
	if ( w.hook )
		ps.lastHook = cls.realtime;
	if ( w.hook && psWaiting.empty() && PS_ListUsable() ) {
		PS_Expand( cmd );
		return qtrue;
	}
	if ( !w.hook && psWaiting.empty() ) {
		PS_Queue( cmd );
		return qtrue;
	}
	w.cmd = cmd;
	w.time = cls.realtime;
	psWaiting.push_back( w );
	return qtrue;
}

void CL_Presence_Frame( void ) {
	const char *server;

	if ( !cl_presenceUrl )
		return;
	PS_PollRequests();
	if ( !PS_Active() ) {
		if ( !psWaiting.empty() || !psQueue.empty() || ps.server[0] )
			PS_Reset();
		ps.server[0] = '\0';
		return;
	}
	server = NET_AdrToString( &clc.serverAddress );
	if ( Q_stricmp( server, ps.server ) ) {
		PS_Reset();
		Q_strncpyz( ps.server, server, sizeof( ps.server ) );
	}
	PS_Beat();
	PS_WatchJoins();

	// the list: asked when a hook needs it, when it's getting old while hooks go out, and after a join
	if ( !psWaiting.empty() && !PS_ListUsable() )
		PS_AskList();
	else if ( ps.known && cls.realtime - ps.listTime >= PS_LIST_FRESH_MS && cls.realtime - ps.lastHook < PS_LIST_USABLE_MS )
		PS_AskList();
	if ( ps.askAt && cls.realtime >= ps.askAt ) {
		ps.askAt = 0;
		PS_AskList();
	}
	if ( !psWaiting.empty() ) {
		if ( PS_ListUsable() )
			PS_FlushWaiting( qtrue );
		else if ( cls.realtime - psWaiting.front().time >= PS_WAIT_MS )
			PS_FlushWaiting( qfalse );
	}

	// one out, once the flood protection lets it through
	if ( !psQueue.empty() && cls.realtime - CL_LastCommandTime() >= PS_Gap() ) {
		const std::string cmd = psQueue.front();

		psQueue.pop_front();
		ps.sending = qtrue;
		CL_AddReliableCommand( cmd.c_str(), qfalse );
		ps.sending = qfalse;
	}
}

// leaving the server: the bot told, if it knew of us
void CL_Presence_Disconnect( void ) {
	std::string body = "{\"server\":";

	if ( ps.beatOn && cl_presenceUrl && cl_presenceUrl->string[0] ) {
		PS_Json( body, ps.beatServer );
		body += va( ",\"client\":%i,\"leaving\":true}", ps.beatClient );
		PS_Send( &psLeaveHttp, "/v1/presence", body );
	}
	ps.beatOn = qfalse;
	ps.beatKey[0] = '\0';
	ps.beatNext = 0;
	PS_Reset();
	ps.server[0] = '\0';
}

static void PS_ClientUsers_f( void ) {
	if ( !PS_Active() ) {
		Com_Printf( "clientusers: only on a server, with cl_presenceUrl set\n" );
		return;
	}
	ps.print = qtrue;
	ps.askNext = 0;
	if ( psListHttp.state == HTTP_BUSY )
		return;
	PS_AskList();
}

void CL_Presence_Init( void ) {
	cl_presenceUrl = Cvar_Get( "cl_presenceUrl", PS_DEFAULT_URL, CVAR_ARCHIVE_ND | CVAR_PROTECTED,
		"Where the game says it's on a server with this client, so the tools' hooks reach only those who have it; empty: not said, hooks go to everyone" );
	cl_hookGap = Cvar_Get( "cl_hookGap", "0", CVAR_ARCHIVE_ND,
		"Milliseconds between the hooks sent to each player with this client; 0: as the server's flood protection lets them" );
	Cmd_AddCommand( "clientusers", PS_ClientUsers_f, "Who on this server has this client, as the feedback bot knows" );
}

void CL_Presence_Shutdown( void ) {
	Cmd_RemoveCommand( "clientusers" );
}
