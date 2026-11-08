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
//
// C1B3R (the bot, made bigger) carries the hooks itself (/v2): the game opens a session once it's on
// a server, which is also who's playing with the client, and keeps a poll open that brings the hooks
// the others send as soon as they're sent. A hook goes to C1B3R once, not to the server once for each
// player. Only an admin may send hooks, as only an admin may rpnotify: a session proves it by one
// rpnotify of its proof code to another player with the client (cmd:c1b3r/<code>), whose game tells
// C1B3R it saw it. Until then, with games from before (/v1 in the list), for NPC talk (the cgame needs
// it from the server, as chat) and when C1B3R doesn't answer, hooks go by rpnotify as above.

#include "client.h"
#include "cl_http.h"
#include "qcommon/game_version.h"
#include <deque>
#include <string>

#define PS_DEFAULT_URL		"https://c1b3r.keztor.tech"	// the feedback bot, as cl_feedbackUrl
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
#define PS_POLL_WAIT_MS		40000				// a poll's answer may take this long: C1B3R holds it 25 s
#define PS_POLL_RETRY_MS	5000				// after a poll that failed
#define PS_HELLO_RETRY_MS	30000
#define PS_PROOF_EVERY_MS	30000				// the proof sent again at most this often
#define PS_PROOF_LIST_MS	3000				// the list asked this long after, to see it's proven

typedef struct {
	int			num;
	char		name[PS_NAME_LEN];
	char		version[32];
	qboolean	relay;				// on a C1B3R session: hooks reach them through it
	qboolean	proven;
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

// the C1B3R session
static struct {
	qboolean	on;					// a session: hooks come through C1B3R
	qboolean	none;				// C1B3R has no /v2 (one from before): /v1 alone on this server
	char		id[64];
	char		proof[32];
	int			seq;				// the last hook seen
	char		key[512];			// the server, client number and name it's for
	char		asked[512];			// a hello going out for these
	int			helloNext;
	int			pollNext;
	qboolean	proven;				// may send hooks through it
	int			proofTime;
	int			helloStatus;		// C1B3R's last answer to a hello, for /clientusers
	char		seen[32];			// someone's proof code to tell C1B3R about
} psv;

static std::deque<psWaiting_t>	psWaiting;	// waiting for the list, in order
static std::deque<std::string>	psQueue;	// going out, one a gap apart
static std::deque<std::string>	psRelayOut;	// hooks going to C1B3R, one at a time
static std::string				psRelaySending;
static httpRequest_t			psBeatHttp, psListHttp, psLeaveHttp, psHelloHttp, psPollHttp, psHookHttp, psSeenHttp;
static cvar_t					*cl_presenceUrl, *cl_hookGap, *cl_presenceLocal;

/*
===============================================================================

What's around

===============================================================================
*/

static qboolean PS_Active( void ) {
	return (qboolean)( cl_presenceUrl && cl_presenceUrl->string[0] && cls.state == CA_ACTIVE && !clc.demoplaying
		&& ( !( com_sv_running && com_sv_running->integer ) || cl_presenceLocal->integer ) );
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

// the bot told who we are: as we come in, when something changes, and every few minutes; not while
// a C1B3R session says it
static void PS_Beat( void ) {
	const char *name = PS_SlotName( clc.clientNum );
	char key[512];
	std::string body;

	if ( psBeatHttp.state == HTTP_BUSY || cls.realtime < ps.beatNext || psv.on )
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
	std::string body = psv.on ? "{\"session\":" : "{\"server\":";

	if ( psListHttp.state == HTTP_BUSY || cls.realtime < ps.askNext )
		return;
	PS_Json( body, psv.on ? psv.id : ps.server );
	body += "}";
	if ( !PS_Send( &psListHttp, psv.on ? "/v2/list" : "/v1/presence/list", body ) )
		ps.askNext = cls.realtime + PS_RETRY_MS;
}

static void PS_PrintList( void ) {
	Com_Printf( "C1B3R: %s\n", psv.on ? ( psv.proven ? "session open, you may send hooks through it" : "session open, not proven an admin yet" )
		: psv.none ? "doesn't carry hooks (one from before)" : va( "no session yet (last answer %i)", psv.helloStatus ) );
	Com_Printf( "Players with this client on %s: %s\n", ps.server,
		!ps.known ? "not known (the bot didn't answer)" : psv.on && psv.proven ? "your hooks go through C1B3R"
		: psv.on ? "C1B3R carries their hooks to you; yours go by rpnotify until you're proven an admin" : "hooks go to each of them, one after the other" );
	for ( int i = 0; ps.known && i < ps.numPlayers; i++ ) {
		const psPlayer_t *p = &ps.players[i];
		char inSlot[PS_NAME_LEN], told[PS_NAME_LEN];

		PS_Normalize( PS_SlotName( p->num ), inSlot, sizeof( inSlot ) );
		PS_Normalize( p->name, told, sizeof( told ) );
		Com_Printf( "  %2i  %s^7  %s%s%s%s\n", p->num, p->name, p->version, p->relay ? S_COLOR_GREY "  C1B3R" : S_COLOR_GREY "  rpnotify",
			p->proven ? S_COLOR_GREEN "  sends hooks" : "", strcmp( inSlot, told ) ? S_COLOR_YELLOW "  (not in that slot now: left out)" : "" );
	}
}

// ok
// player <tab> client number <tab> version <tab> name                                   (/v1)
// player <tab> client number <tab> version <tab> v2|v1 <tab> proven 0|1 <tab> name      (/v2)
static void PS_ReadList( const char *answer ) {
	char line[512];
	const char *at = answer, *end;
	qboolean first = qtrue;

	ps.numPlayers = 0;
	for ( ; *at; at = *end ? end + 1 : end ) {
		char *field[6];
		int n = 0;

		end = strchr( at, '\n' );
		if ( !end )
			end = at + strlen( at );
		Q_strncpyz( line, at, Q_min( (int)sizeof( line ), (int)( end - at ) + 1 ) );
		for ( char *f = line; f && n < 6; n++ ) {
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
		if ( ( n == 4 || n == 6 ) && !strcmp( field[0], "player" ) && ps.numPlayers < MAX_CLIENTS ) {
			psPlayer_t *p = &ps.players[ps.numPlayers];

			p->num = atoi( field[1] );
			if ( p->num < 0 || p->num >= MAX_CLIENTS )
				continue;
			Q_strncpyz( p->version, field[2], sizeof( p->version ) );
			Q_strncpyz( p->name, field[n - 1], sizeof( p->name ) );
			p->relay = (qboolean)( n == 6 && !strcmp( field[3], "v2" ) );
			p->proven = (qboolean)( n == 6 && !strcmp( field[4], "1" ) );
			if ( p->num == clc.clientNum && p->relay )
				psv.proven = p->proven;
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

// who a hook goes to: us first, then each with the client whose slot still has the name they told;
// skipSelf: not us (we've run it already); oldOnly: only those C1B3R doesn't reach
static int PS_Targets( int *out, qboolean skipSelf, qboolean oldOnly ) {
	char inSlot[PS_NAME_LEN], told[PS_NAME_LEN];
	int n = 0;

	if ( !skipSelf )
		out[n++] = clc.clientNum;
	for ( int i = 0; i < ps.numPlayers; i++ ) {
		const psPlayer_t *p = &ps.players[i];
		int k;

		if ( p->num == clc.clientNum || ( oldOnly && p->relay ) )
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
static void PS_ExpandTo( const std::string &cmd, qboolean skipSelf, qboolean oldOnly ) {
	const std::string rest = cmd.substr( strlen( PS_HOOK_TO ) );
	int targets[MAX_CLIENTS + 1];
	const int count = PS_Targets( targets, skipSelf, oldOnly );

	for ( int i = 0; i < count; i++ )
		PS_Queue( va( "rpnotify %i 1 ", targets[i] ) + rest );
}

static void PS_Expand( const std::string &cmd ) {
	PS_ExpandTo( cmd, qfalse, qfalse );
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
	psRelayOut.clear();
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

/*
===============================================================================

C1B3R: the session, the poll, hooks through it

===============================================================================
*/

static const char *PS_MyName( void ) {
	const char *name = PS_SlotName( clc.clientNum );

	return name[0] ? name : Cvar_VariableString( "name" );
}

// a session for where we are: made when there's none, or the server, number or name changed
static void PS_Hello( void ) {
	char key[512];
	std::string body;

	if ( psv.none || psHelloHttp.state == HTTP_BUSY || cls.realtime < psv.helloNext )
		return;
	Com_sprintf( key, sizeof( key ), "%s|%i|%s", ps.server, clc.clientNum, PS_MyName() );
	if ( psv.on && !strcmp( key, psv.key ) )
		return;
	body = "{\"key\":";
	PS_Json( body, CL_Feedback_Key() );
	body += ",\"server\":";
	PS_Json( body, ps.server );
	body += va( ",\"client\":%i,\"name\":", clc.clientNum );
	PS_Json( body, PS_MyName() );
	body += ",\"version\":";
	PS_Json( body, GIT_TAG );
	body += "}";
	if ( !PS_Send( &psHelloHttp, "/v2/hello", body ) ) {
		psv.helloNext = cls.realtime + PS_HELLO_RETRY_MS;
		return;
	}
	Q_strncpyz( psv.asked, key, sizeof( psv.asked ) );
}

// the poll kept open: C1B3R answers it when there's a hook for us, or after a while with none
static void PS_Poll( void ) {
	std::string body = "{\"session\":";

	if ( !psv.on || psPollHttp.state == HTTP_BUSY || cls.realtime < psv.pollNext )
		return;
	PS_Json( body, psv.id );
	body += va( ",\"after\":%i}", psv.seq );
	psPollHttp.waitMs = PS_POLL_WAIT_MS;
	if ( !PS_Send( &psPollHttp, "/v2/poll", body ) )
		psv.pollNext = cls.realtime + PS_POLL_RETRY_MS;
}

// a field of C1B3R's answer as it was sent: \t \n \\ back, and its UTF-8 back to the game's Latin-1
static std::string PS_Unfield( const char *in ) {
	std::string out;

	for ( const unsigned char *p = (const unsigned char *)in; *p; p++ ) {
		if ( *p == '\\' && p[1] ) {
			p++;
			out += *p == 't' ? '\t' : *p == 'n' ? '\n' : (char)*p;
		} else if ( ( *p & 0xE0 ) == 0xC0 && ( p[1] & 0xC0 ) == 0x80 ) {
			const int c = ( ( *p & 0x1F ) << 6 ) | ( p[1] & 0x3F );

			out += (char)( c < 256 ? c : '?' );
			p++;
		} else {
			out += (char)*p;
		}
	}
	return out;
}

// ok <tab> seq, then: hook <tab> seq <tab> from client <tab> text
static void PS_ReadPoll( const char *answer ) {
	std::string all( answer );
	size_t at = 0;
	int last = psv.seq;
	qboolean first = qtrue;

	while ( at < all.size() ) {
		size_t end = all.find( '\n', at );
		std::string line;

		if ( end == std::string::npos )
			end = all.size();
		line = all.substr( at, end - at );
		at = end + 1;
		if ( first ) {
			first = qfalse;
			if ( line.compare( 0, 3, "ok\t" ) )
				return;
			last = Q_max( last, atoi( line.c_str() + 3 ) );
			continue;
		}
		if ( !line.compare( 0, 5, "hook\t" ) ) {
			const size_t a = line.find( '\t', 5 ), b = a == std::string::npos ? a : line.find( '\t', a + 1 );
			const int seq = atoi( line.c_str() + 5 );

			if ( b == std::string::npos || seq <= psv.seq )
				continue;
			psv.seq = seq;
			CL_RpHooks_Deliver( PS_Unfield( line.c_str() + b + 1 ).c_str() );
		}
	}
	psv.seq = Q_max( psv.seq, last );
}

// someone's proof, come through the server as an rpnotify: told to C1B3R
void CL_Presence_Seen( const char *code ) {
	Q_strncpyz( psv.seen, code, sizeof( psv.seen ) );
}

// our proof, sent by rpnotify to another player whose game has a session: the server lets only an
// admin's through, and C1B3R hears of it from theirs
static void PS_SendProof( void ) {
	char inSlot[PS_NAME_LEN], told[PS_NAME_LEN];

	if ( !psv.on || psv.proven || !psv.proof[0] || cls.realtime - psv.proofTime < PS_PROOF_EVERY_MS || !PS_ListUsable() )
		return;
	for ( int i = 0; i < ps.numPlayers; i++ ) {
		const psPlayer_t *p = &ps.players[i];

		if ( !p->relay || p->num == clc.clientNum )
			continue;
		PS_Normalize( PS_SlotName( p->num ), inSlot, sizeof( inSlot ) );
		PS_Normalize( p->name, told, sizeof( told ) );
		if ( !inSlot[0] || strcmp( inSlot, told ) )
			continue;
		psv.proofTime = cls.realtime;
		PS_Queue( va( "rpnotify %i 1 cmd:c1b3r/%s", p->num, psv.proof ) );
		ps.askAt = cls.realtime + PS_PROOF_LIST_MS;
		return;
	}
}

// NPC talk the cgame needs from the server, as the chat it's turned into: it goes by rpnotify
static qboolean PS_NeedsServer( const char *text ) {
	char head[64], *part[3];
	int n = 0;

	Q_strncpyz( head, text + strlen( "cmd:" ), sizeof( head ) );
	for ( char *p = head; p && n < 3; n++ ) {
		while ( *p == ' ' )
			p++;
		part[n] = p;
		p = strpbrk( p, "/:" );
		if ( p )
			*p++ = '\0';
		for ( char *t = part[n] + strlen( part[n] ); t > part[n] && t[-1] == ' '; )
			*--t = '\0';
	}
	if ( n < 2 || Q_stricmp( part[0], "npc" ) )
		return qfalse;
	for ( int i = 1; i < n; i++ ) {
		if ( !Q_stricmp( part[i], "talk" ) || !Q_stricmp( part[i], "say" ) )
			return qtrue;
	}
	return qfalse;
}

// what C1B3R didn't take, or wasn't asked, goes by rpnotify to the others (we've run it already)
static void PS_RelayFallback( void ) {
	if ( !psRelaySending.empty() )
		PS_ExpandTo( PS_HOOK_TO + psRelaySending, qtrue, qfalse );
	psRelaySending.clear();
	while ( !psRelayOut.empty() ) {
		PS_ExpandTo( PS_HOOK_TO + psRelayOut.front(), qtrue, qfalse );
		psRelayOut.pop_front();
	}
}

static void PS_RelayFrame( void ) {
	std::string body = "{\"session\":";

	if ( !psv.on || psHookHttp.state == HTTP_BUSY || psRelayOut.empty() )
		return;
	psRelaySending = psRelayOut.front();
	psRelayOut.pop_front();
	PS_Json( body, psv.id );
	body += ",\"text\":";
	PS_Json( body, psRelaySending.c_str() );
	body += "}";
	if ( !PS_Send( &psHookHttp, "/v2/hook", body ) )
		PS_RelayFallback();
}

static void PS_SeenFrame( void ) {
	std::string body = "{\"session\":";

	if ( !psv.seen[0] || !psv.on || psSeenHttp.state == HTTP_BUSY )
		return;
	PS_Json( body, psv.id );
	body += ",\"code\":";
	PS_Json( body, psv.seen );
	body += "}";
	PS_Send( &psSeenHttp, "/v2/seen", body );
	psv.seen[0] = '\0';
}

// C1B3R's answers: the session made, hooks come, a hook taken or not
static void PS_PollRelay( void ) {
	if ( HTTP_Poll( &psHelloHttp ) ) {
		char *field[4], *f = psHelloHttp.answer;
		int n = 0;

		psv.helloStatus = psHelloHttp.status;
		Com_DPrintf( "clientusers: C1B3R hello %i: %s\n", psHelloHttp.status, psHelloHttp.answer );
		for ( ; f && n < 4; n++ ) {
			field[n] = f;
			f = strpbrk( f, "\t\n" );
			if ( f )
				*f++ = '\0';
		}
		if ( psHelloHttp.status == 200 && n == 4 && !strcmp( field[0], "ok" ) && !Q_stricmpn( psv.asked, ps.server, strlen( ps.server ) ) ) {
			psv.on = qtrue;
			Q_strncpyz( psv.id, field[1], sizeof( psv.id ) );
			Q_strncpyz( psv.proof, field[2], sizeof( psv.proof ) );
			psv.seq = atoi( field[3] );
			Q_strncpyz( psv.key, psv.asked, sizeof( psv.key ) );
			psv.proven = qfalse;
			psv.proofTime = 0;
			psv.pollNext = 0;
			// who's here, and whether we're proven already (the same player in the same slot)
			ps.askNext = 0;
			PS_AskList();
		} else if ( psHelloHttp.status == 404 ) {
			// a C1B3R from before carrying hooks: who's here as before, hooks by rpnotify
			psv.none = qtrue;
			Com_DPrintf( "clientusers: C1B3R doesn't carry hooks; they go by rpnotify\n" );
		} else {
			psv.helloNext = cls.realtime + PS_HELLO_RETRY_MS;
		}
	}
	if ( HTTP_Poll( &psPollHttp ) ) {
		if ( psPollHttp.status == 200 ) {
			PS_ReadPoll( psPollHttp.answer );
			psv.pollNext = 0;
		} else if ( psPollHttp.status == 404 ) {
			// the session's gone (C1B3R restarted): a new one
			psv.on = qfalse;
			psv.key[0] = '\0';
		} else {
			psv.pollNext = cls.realtime + PS_POLL_RETRY_MS;
		}
	}
	if ( HTTP_Poll( &psHookHttp ) ) {
		if ( psHookHttp.status == 200 ) {
			psRelaySending.clear();
		} else {
			// not proven (any more), or C1B3R not there: by rpnotify, as before
			if ( psHookHttp.status == 403 )
				psv.proven = qfalse;
			if ( psHookHttp.status == 404 ) {
				psv.on = qfalse;
				psv.key[0] = '\0';
			}
			PS_RelayFallback();
		}
	}
	HTTP_Poll( &psSeenHttp );
}

// from CL_AddReliableCommand: qtrue when it's taken here, to go out later
qboolean CL_Presence_Outgoing( const char *cmd ) {
	psWaiting_t w;

	if ( ps.sending || !PS_Active() )
		return qfalse;
	w.hook = (qboolean)!Q_stricmpn( cmd, PS_HOOK, strlen( PS_HOOK ) );
	// through C1B3R, once proven: run here as the server's echo would have, sent to it once, and to
	// those it doesn't reach by rpnotify
	if ( w.hook && psv.on ) {
		const char *text = cmd + strlen( PS_HOOK_TO );

		if ( psv.proven && !PS_NeedsServer( text ) ) {
			ps.lastHook = cls.realtime;
			CL_RpHooks_Deliver( text );
			psRelayOut.push_back( text );
			if ( PS_ListUsable() )
				PS_ExpandTo( cmd, qtrue, qtrue );
			return qtrue;
		}
		PS_SendProof();
	}
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
	PS_PollRelay();
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
		// a session is for one server: another, made when it's known
		psv.on = qfalse;
		psv.none = qfalse;
		psv.proven = qfalse;
		psv.key[0] = '\0';
		psv.helloNext = 0;
	}
	PS_Hello();
	PS_Poll();
	PS_RelayFrame();
	PS_SeenFrame();
	// proven before our hooks need it, once we've sent one lately
	if ( psv.on && !psv.proven && ps.lastHook && cls.realtime - ps.lastHook < PS_LIST_USABLE_MS )
		PS_SendProof();
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

	// the session ended, rather than left to run out
	if ( psv.on && cl_presenceUrl && cl_presenceUrl->string[0] ) {
		std::string bye = "{\"session\":";

		PS_Json( bye, psv.id );
		bye += "}";
		PS_Send( &psSeenHttp, "/v2/bye", bye );
	}
	psv.on = qfalse;
	psv.none = qfalse;
	psv.proven = qfalse;
	psv.key[0] = '\0';
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
	cl_presenceLocal = Cvar_Get( "cl_presenceLocal", "0", CVAR_TEMP | CVAR_INTERNAL, "Talk to C1B3R on a server of your own too, to try it" );
	cl_hookGap = Cvar_Get( "cl_hookGap", "0", CVAR_ARCHIVE_ND,
		"Milliseconds between the hooks sent to each player with this client; 0: as the server's flood protection lets them" );
	Cmd_AddCommand( "clientusers", PS_ClientUsers_f, "Who on this server has this client, as the feedback bot knows" );
}

void CL_Presence_Shutdown( void ) {
	Cmd_RemoveCommand( "clientusers" );
}
