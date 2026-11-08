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

// cl_effectswap.cpp -- a player's own effects played as other ones: the effect manager's Mine tab
//
// An effect is a player's own when it's bolted to them, or starts nearer them than anyone else,
// as force powers come out of the caster's hand (see CL_ShaderManager_NearestPlayer). Whatever
// cgame plays as this player's own, and is swapped, plays as the effect they picked instead: on
// this screen, and on the screens of everyone with this client when it's shared. Everyone else's
// stays standard. Kept in effectmanager/mine.txt, an effect and its replacement a line, by their
// path under effects/ without .efx.
//
// Shared, they go to everyone with this client as cmd:fxmine/<client>/<part>/<parts>:
// <effect>><effect>|..., cmd:fxmine/<client>/clear: when the map is under way, when they change
// and when someone joins. Someone without the replacement in their pk3s sees the standard one.

#include <map>
#include <string>
#include <vector>
#include "client.h"
#include "FXExport.h"
#include "FxScheduler.h"

#define ES_FILE				"effectmanager/mine.txt"
#define ES_MAX				64			// swaps a player
#define ES_MAX_RECENT		24			// effects of this player's own played lately, listed
#define ES_SHARE_LEN		700			// a hook's swaps, well inside what RPMod passes on
#define ES_SHARE_WORD		32			// longer words go in pieces
#define ES_SHARE_GAP_MS		400			// between hooks, for the server's flood protection
#define ES_SHARE_START_MS	6000		// shared this long into a map
#define ES_SHARE_JOIN_MS	4000		// and this long after someone joins

typedef struct {
	std::vector<std::string>	from, to;
} esSwaps_t;

typedef struct {
	char	name[MAX_QPATH];
	int		count;
	int		time;
} esRecent_t;

static struct {
	qboolean		loaded;
	esSwaps_t		mine;
	esSwaps_t		other[MAX_CLIENTS];
	// the replacement's id by player and effect id, worked out once: 0 not yet, -1 none
	short			cache[MAX_CLIENTS][FX_MAX_EFFECTS];

	esRecent_t		recent[ES_MAX_RECENT];
	int				numRecent;

	qboolean		active;
	int				shareAt;
	int				sharePlayers;
	int				shareNext;
	std::vector<std::string>	shareQueue;
	qboolean		shareWarned;
} es;

static cvar_t *cl_effectShareMine;

// an effect's name as kept: lower case, under effects/, without .efx
static void ES_Key( const char *name, char *out, int size ) {
	char buf[MAX_QPATH];

	Q_strncpyz( buf, name ? name : "", sizeof( buf ) );
	for ( char *c = buf; *c; c++ ) {
		*c = (char)tolower( (unsigned char)*c );
		if ( *c == '\\' )
			*c = '/';
	}
	if ( strlen( buf ) > 4 && !strcmp( buf + strlen( buf ) - 4, ".efx" ) )
		buf[strlen( buf ) - 4] = '\0';
	Q_strncpyz( out, !Q_strncmp( buf, "effects/", 8 ) ? buf + 8 : buf, size );
}

// a name as a player sent it: a path, nothing that could be read another way
static qboolean ES_SafeName( const char *name ) {
	if ( !name[0] || strlen( name ) >= MAX_QPATH - 12 || strstr( name, ".." ) )
		return qfalse;
	for ( ; *name; name++ ) {
		if ( !isalnum( (unsigned char)*name ) && !strchr( "_-./", *name ) )
			return qfalse;
	}
	return qtrue;
}

static void ES_Forget( void ) {
	memset( es.cache, 0, sizeof( es.cache ) );
}

static void ES_Load( void ) {
	char *buf, from[MAX_QPATH], to[MAX_QPATH];
	const char *p, *token;

	es.loaded = qtrue;
	es.mine.from.clear();
	es.mine.to.clear();
	if ( FS_ReadFile( ES_FILE, (void **)&buf ) > 0 && buf ) {
		p = buf;
		COM_BeginParseSession( ES_FILE );
		while ( (int)es.mine.from.size() < ES_MAX ) {
			token = COM_ParseExt( &p, qtrue );
			if ( !token[0] )
				break;
			ES_Key( token, from, sizeof( from ) );
			token = COM_ParseExt( &p, qfalse );
			if ( !token[0] )
				continue;
			ES_Key( token, to, sizeof( to ) );
			if ( strcmp( from, to ) && ES_SafeName( from ) && ES_SafeName( to ) && CL_EffectSwap_Find( from ) < 0 ) {
				es.mine.from.push_back( from );
				es.mine.to.push_back( to );
			}
		}
		FS_FreeFile( buf );
	}
	ES_Forget();
}

static void ES_Save( void ) {
	std::string out = "// the effect manager's Mine tab: your own effects played as others, an effect and what plays instead\n";

	for ( size_t i = 0; i < es.mine.from.size(); i++ )
		out += es.mine.from[i] + " " + es.mine.to[i] + "\n";
	FS_WriteFile( ES_FILE, out.c_str(), (int)out.size() );
}

static void ES_Changed( void ) {
	ES_Save();
	ES_Forget();
	es.shareAt = cls.realtime + 500;
}

/*
===============================================================================

PLAYING

===============================================================================
*/

static void ES_NoteRecent( const char *key ) {
	int i;

	for ( i = 0; i < es.numRecent; i++ ) {
		if ( !strcmp( es.recent[i].name, key ) )
			break;
	}
	if ( i == es.numRecent ) {
		if ( es.numRecent < ES_MAX_RECENT )
			es.numRecent++;
		i = es.numRecent - 1;
		Q_strncpyz( es.recent[i].name, key, sizeof( es.recent[i].name ) );
		es.recent[i].count = 0;
	}
	es.recent[i].count++;
	es.recent[i].time = cls.realtime;
	// the newest first
	for ( ; i > 0 && es.recent[i - 1].time < es.recent[i].time; i-- ) {
		const esRecent_t t = es.recent[i - 1];

		es.recent[i - 1] = es.recent[i];
		es.recent[i] = t;
	}
}

// from the effects system as it plays an effect of a player's own: the effect to play instead
int CL_EffectSwap_Play( int owner, int id, const char *name ) {
	short *cached;
	char key[MAX_QPATH];
	const esSwaps_t *swaps;
	int to = -1;
	const char *toName = "";

	if ( owner < 0 || owner >= MAX_CLIENTS || id < 1 || id >= FX_MAX_EFFECTS )
		return id;
	if ( !es.loaded )
		ES_Load();
	if ( owner == clc.clientNum ) {
		ES_Key( name, key, sizeof( key ) );
		ES_NoteRecent( key );
	}
	cached = &es.cache[owner][id];
	if ( *cached > 0 )
		return *cached;
	if ( *cached < 0 )
		return id;
	swaps = owner == clc.clientNum ? &es.mine : &es.other[owner];
	if ( !swaps->from.empty() ) {
		if ( owner != clc.clientNum )
			ES_Key( name, key, sizeof( key ) );
		for ( size_t i = 0; i < swaps->from.size(); i++ ) {
			if ( swaps->from[i] == key ) {
				toName = swaps->to[i].c_str();
				to = FX_RegisterEffect( va( "effects/%s", toName ) );
				break;
			}
		}
	}
	*cached = (short)( to > 0 && to < FX_MAX_EFFECTS && to != id ? to : -1 );
	if ( *cached > 0 )
		Com_DPrintf( "Effect replacer: %s plays as %s for client %i\n", key, toName, owner );
	return *cached > 0 ? *cached : id;
}

// the effects system cleared its effects: their ids mean other effects now
void CL_EffectSwap_Reset( void ) {
	memset( es.cache, 0, sizeof( es.cache ) );
}

/*
===============================================================================

THE MINE TAB'S

===============================================================================
*/

int CL_EffectSwap_Count( void ) {
	if ( !es.loaded )
		ES_Load();
	return (int)es.mine.from.size();
}

qboolean CL_EffectSwap_Get( int i, const char **from, const char **to ) {
	if ( i < 0 || i >= CL_EffectSwap_Count() )
		return qfalse;
	*from = es.mine.from[i].c_str();
	*to = es.mine.to[i].c_str();
	return qtrue;
}

int CL_EffectSwap_Find( const char *from ) {
	char key[MAX_QPATH];

	ES_Key( from, key, sizeof( key ) );
	for ( size_t i = 0; i < es.mine.from.size(); i++ ) {
		if ( es.mine.from[i] == key )
			return (int)i;
	}
	return -1;
}

// from played as to from now on; false if there are too many or they're the same
qboolean CL_EffectSwap_Set( const char *from, const char *to ) {
	char f[MAX_QPATH], t[MAX_QPATH];
	int i;

	ES_Key( from, f, sizeof( f ) );
	ES_Key( to, t, sizeof( t ) );
	if ( !strcmp( f, t ) || !ES_SafeName( f ) || !ES_SafeName( t ) )
		return qfalse;
	i = CL_EffectSwap_Find( f );
	if ( i >= 0 ) {
		es.mine.to[i] = t;
	} else {
		if ( (int)es.mine.from.size() >= ES_MAX )
			return qfalse;
		es.mine.from.push_back( f );
		es.mine.to.push_back( t );
	}
	ES_Changed();
	return qtrue;
}

void CL_EffectSwap_Remove( int i ) {
	if ( i < 0 || i >= CL_EffectSwap_Count() )
		return;
	es.mine.from.erase( es.mine.from.begin() + i );
	es.mine.to.erase( es.mine.to.begin() + i );
	ES_Changed();
}

void CL_EffectSwap_Clear( void ) {
	if ( !CL_EffectSwap_Count() )
		return;
	es.mine.from.clear();
	es.mine.to.clear();
	ES_Changed();
}

int CL_EffectSwap_NumRecent( void ) {
	return es.numRecent;
}

const char *CL_EffectSwap_Recent( int i, int *count, int *msAgo ) {
	if ( i < 0 || i >= es.numRecent )
		return NULL;
	if ( count )
		*count = es.recent[i].count;
	if ( msAgo )
		*msAgo = cls.realtime - es.recent[i].time;
	return es.recent[i].name;
}

qboolean CL_EffectSwap_Shared( void ) {
	return (qboolean)( cl_effectShareMine && cl_effectShareMine->integer );
}

void CL_EffectSwap_SetShared( qboolean on ) {
	Cvar_Set( "cl_effectShareMine", on ? "1" : "0" );
	// switched off: the others go back to the standard ones
	es.shareAt = cls.realtime;
}

/*
===============================================================================

SHARED

===============================================================================
*/

// a word, a space put in after each / and every ES_SHARE_WORD characters: RPMod chokes on long ones
static void ES_AppendSplit( std::string &out, const char *word ) {
	int run = 0;

	for ( ; *word; word++ ) {
		out += *word;
		run++;
		if ( word[1] && ( *word == '/' || run >= ES_SHARE_WORD ) ) {
			out += ' ';
			run = 0;
		}
	}
}

static void ES_Queue( const char *text ) {
	es.shareQueue.push_back( va( "rpnotify -1 1 %s", text ) );
}

// this player's swaps, in as few hooks as fit; none when sharing is off
static void ES_Share( void ) {
	std::vector<std::string> parts;
	std::string part, piece;

	es.shareQueue.clear();
	if ( es.mine.from.empty() || !CL_EffectSwap_Shared() ) {
		ES_Queue( va( "cmd:fxmine/%i/clear", clc.clientNum ) );
		return;
	}
	for ( size_t i = 0; i < es.mine.from.size(); i++ ) {
		piece.clear();
		ES_AppendSplit( piece, ( es.mine.from[i] + ">" + es.mine.to[i] ).c_str() );
		if ( !part.empty() && part.size() + piece.size() + 3 > ES_SHARE_LEN ) {
			parts.push_back( part );
			part.clear();
		}
		if ( !part.empty() )
			part += " | ";
		part += piece;
	}
	parts.push_back( part );
	for ( size_t i = 0; i < parts.size(); i++ )
		ES_Queue( va( "cmd:fxmine/%i/%i/%i: %s", clc.clientNum, (int)i + 1, (int)parts.size(), parts[i].c_str() ) );
}

// once a frame: shared when due, a hook at a time; a player gone takes theirs with them
void CL_EffectSwap_Frame( void ) {
	int players = 0;

	if ( cls.state != CA_ACTIVE || !cls.cgameStarted ) {
		if ( es.active ) {
			es.active = qfalse;
			es.shareQueue.clear();
			es.shareAt = 0;
			for ( int c = 0; c < MAX_CLIENTS; c++ ) {
				es.other[c].from.clear();
				es.other[c].to.clear();
			}
			ES_Forget();
		}
		return;
	}
	if ( !es.active ) {
		// a map under way: shared a little later, if there's anything to say
		es.active = qtrue;
		es.sharePlayers = 0;
		if ( !es.loaded )
			ES_Load();
		if ( !es.mine.from.empty() && CL_EffectSwap_Shared() )
			es.shareAt = cls.realtime + ES_SHARE_START_MS;
	}
	for ( int c = 0; c < MAX_CLIENTS; c++ ) {
		const char *info = cl.gameState.stringData + cl.gameState.stringOffsets[CS_PLAYERS + c];

		if ( info[0] ) {
			players++;
		} else if ( !es.other[c].from.empty() ) {
			es.other[c].from.clear();
			es.other[c].to.clear();
			ES_Forget();
		}
	}
	if ( players > es.sharePlayers && es.sharePlayers && !es.mine.from.empty() && CL_EffectSwap_Shared() )
		es.shareAt = cls.realtime + ES_SHARE_JOIN_MS;
	es.sharePlayers = players;

	if ( es.shareAt && cls.realtime >= es.shareAt ) {
		es.shareAt = 0;
		if ( CL_RpHooks_Experimental() ) {
			ES_Share();
		} else if ( !es.shareWarned && !es.mine.from.empty() && CL_EffectSwap_Shared() ) {
			es.shareWarned = qtrue;
			Com_Printf( S_COLOR_YELLOW "Effect manager: your effects aren't shared: TOGGLE EXPERIMENTAL in the npcmanager first\n" );
		}
	}
	if ( !es.shareQueue.empty() && cls.realtime >= es.shareNext ) {
		Com_DPrintf( "Effect manager: sharing your effects: %s\n", es.shareQueue.front().c_str() );
		CL_AddReliableCommand( es.shareQueue.front().c_str(), qfalse );
		es.shareQueue.erase( es.shareQueue.begin() );
		es.shareNext = cls.realtime + ES_SHARE_GAP_MS;
	}
}

// cmd:fxmine/<client>/<part>/<parts>: <effect>><effect>|...; cmd:fxmine/<client>/clear
void CL_EffectSwap_Hook( char **argv, int argc, const char *message ) {
	char buf[MAX_STRING_CHARS], *pair, *next, *to, from[MAX_QPATH], into[MAX_QPATH];
	int client, len = 0, missing = 0;
	esSwaps_t *swaps;

	if ( argc < 2 )
		return;
	client = atoi( argv[0] );
	if ( client < 0 || client >= MAX_CLIENTS || client == clc.clientNum )
		return;
	swaps = &es.other[client];
	if ( !Q_stricmp( argv[1], "clear" ) || atoi( argv[1] ) <= 1 ) {
		swaps->from.clear();
		swaps->to.clear();
	}
	if ( Q_stricmp( argv[1], "clear" ) && message ) {
		// the words came split for the server: the spaces go again
		for ( const char *c = message; *c && len < (int)sizeof( buf ) - 1; c++ ) {
			if ( *c != ' ' )
				buf[len++] = *c;
		}
		buf[len] = '\0';
		for ( pair = buf; pair && *pair; pair = next ) {
			next = strchr( pair, '|' );
			if ( next )
				*next++ = '\0';
			to = strchr( pair, '>' );
			if ( !to )
				continue;
			*to++ = '\0';
			ES_Key( pair, from, sizeof( from ) );
			ES_Key( to, into, sizeof( into ) );
			if ( !ES_SafeName( from ) || !ES_SafeName( into ) || !strcmp( from, into ) || (int)swaps->from.size() >= ES_MAX )
				continue;
			swaps->from.push_back( from );
			swaps->to.push_back( into );
		}
	}
	for ( size_t i = 0; i < swaps->to.size(); i++ ) {
		if ( FS_ReadFile( va( "effects/%s.efx", swaps->to[i].c_str() ), NULL ) <= 0 )
			missing++;
	}
	if ( !swaps->from.empty() || !Q_stricmp( argv[1], "clear" ) )
		Com_DPrintf( "Effect manager: %s shared %i effect%s%s\n",
			Info_ValueForKey( cl.gameState.stringData + cl.gameState.stringOffsets[CS_PLAYERS + client], "n" ),
			(int)swaps->from.size(), swaps->from.size() == 1 ? "" : "s",
			missing ? va( ", %i you don't have: those stay standard", missing ) : "" );
	ES_Forget();
}

void CL_EffectSwap_Init( void ) {
	cl_effectShareMine = Cvar_Get( "cl_effectShareMine", "1", CVAR_ARCHIVE_ND,
		"Share the effects you swapped in the effect manager's Mine tab with others who have this client" );
}
