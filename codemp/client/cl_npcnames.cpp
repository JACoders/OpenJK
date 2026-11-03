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

// cl_npcnames.cpp -- an NPC's name under the crosshair, as the game shows a player's
//
// cl_npcNames 1 (the default) shows the targetname of the NPC the crosshair is on, nothing for one
// without. Only RPMod's "npc list" says who's who, and only admins may ask for it, so an admin
// with this client and experimental on reads it quietly every so often while NPCs are about, and
// shares the names with everyone with this client:
//
//   rpnotify -1 1 cmd:npcnames/<part>/<parts>: <entity number>=<name>|<entity number>=<name>...
//
// Part 1 starts the list over. Players see the names whenever an admin is on the map. Someone
// joining gets them too: the admin's client sends them again a few times after a player appears,
// as one still loading can't take them yet.

#include "client.h"
#include "qcommon/cm_public.h"

#define NN_NAME_LEN			48
#define NN_ASK_MS			30000	// the list read this often while NPCs are about
#define NN_SHARE_SETTLE_MS	400		// the names shared once the list's lines stop coming this long
#define NN_NEW_ASK_MS		2000	// a new NPC asks for the list again no sooner than this
#define NN_NEW_ASKS			2		// nor more often than this for one NPC
#define NN_RESEND_MS		120000	// the same names sent again this often, for who joined since
#define NN_PART_LEN			700		// of the names in one rpnotify
#define NN_MAX_PARTS		8
#define NN_RANGE			8192.0f
#define NN_LINGER_MS		1000	// a name stays this long after the crosshair leaves
#define NN_FADE_MS			400

// drawn as RPMod's game draws a player's name under the crosshair, matched to screenshots of it:
// ergoec at 1, a letter at a time, each one its width after the last, centred at the game's own
// height for it, a little see-through. Plain, no shadow
#define NN_NAME_FONT		"ergoec"
#define NN_NAME_SCALE		1.0f
#define NN_NAME_GAP			0.0f
#define NN_NAME_Y			170
#define NN_NAME_ALPHA		0.7f
#define NN_JOIN_SENDS		3		// after someone joins, the names sent again at these times
static const int nnJoinMs[NN_JOIN_SENDS] = { 8000, 25000, 60000 };

static cvar_t *cl_npcNames;

static struct {
	char		names[MAX_GENTITIES][NN_NAME_LEN];	// by entity number, "" none known

	// the main view, as the scene was last drawn
	qboolean	haveView;
	vec3_t		viewOrg, viewFwd;

	// the admin's side
	int			askTime;			// the list last asked for
	int			readList;			// the NPC manager's list read into names, by its time
	int			readCount;			// and how many were in it then
	int			changedTime;		// the names changed then, to be shared
	qboolean	shareDue;
	qboolean	listed[MAX_GENTITIES];	// in the last list read, named or not
	byte		newAsks[MAX_GENTITIES];	// lists asked for because of this new NPC
	qboolean	wasNear[MAX_GENTITIES];	// an NPC close by last snapshot
	int			nearSnap;				// the snapshot that was
	qboolean	rankAsked;
	char		sent[NN_PART_LEN * NN_MAX_PARTS];
	int			sentTime;
	qboolean	present[MAX_CLIENTS];	// who's on the server, to see someone join
	qboolean	presentRead;
	int			joinTime;				// someone joined then, 0 none since
	int			joinSends;				// of nnJoinMs, sent since

	// the name shown
	int			shownNum, shownTime;
} nn;

static qboolean NN_InSnapshot( int num );

static qboolean NN_On( void ) {
	return (qboolean)( cl_npcNames && cl_npcNames->integer );
}

static const entityState_t *NN_Ent( int i ) {
	return &cl.parseEntities[( cl.snap.parseEntitiesNum + i ) & ( MAX_PARSE_ENTITIES - 1 )];
}

static qboolean NN_NpcsAbout( void ) {
	for ( int i = 0; i < cl.snap.numEntities; i++ ) {
		if ( NN_Ent( i )->eType == ET_NPC )
			return qtrue;
	}
	return qfalse;
}

// no separators in what's sent, nor what would break the command
static void NN_CleanName( const char *in, char *out, int size ) {
	int len = 0;

	for ( ; *in && len < size - 1; in++ ) {
		if ( *in == '|' || *in == '=' || *in == '"' || *in == ';' || *in == '\n' || *in == '\r' || (unsigned char)*in < ' ' )
			continue;
		out[len++] = *in;
	}
	out[len] = '\0';
}

/*
===============================================================================

THE ADMIN'S SIDE

===============================================================================
*/

// the NPC manager's list, read as its lines come in: the names known from it
static qboolean NN_ReadList( void ) {
	const int listTime = CL_NpcManager_ListTime();
	const int count = CL_NpcManager_NumNpcs();

	if ( !listTime || ( listTime == nn.readList && count == nn.readCount ) )
		return qfalse;
	nn.readList = listTime;
	nn.readCount = count;
	memset( nn.names, 0, sizeof( nn.names ) );
	memset( nn.listed, 0, sizeof( nn.listed ) );
	for ( int i = 0; i < count; i++ ) {
		const char *type, *name;
		int num;

		if ( !CL_NpcManager_GetNpc( i, &num, &type, &name ) || num < 0 || num >= MAX_GENTITIES )
			continue;
		nn.listed[num] = qtrue;
		if ( name && name[0] )
			NN_CleanName( name, nn.names[num], NN_NAME_LEN );
	}
	return qtrue;
}

#define NN_GONE_NEAR		600.0f	// an NPC gone from this close was removed, not just out of sight

// an NPC that vanished close by was removed: the next one spawned may get its number, so it's
// asked about again instead of taking its name
static void NN_WatchGone( void ) {
	qboolean near[MAX_GENTITIES];

	if ( cl.snap.serverTime == nn.nearSnap )
		return;
	nn.nearSnap = cl.snap.serverTime;
	memset( near, 0, sizeof( near ) );
	for ( int i = 0; i < cl.snap.numEntities; i++ ) {
		const entityState_t *es = NN_Ent( i );

		if ( es->eType == ET_NPC && es->number >= 0 && es->number < MAX_GENTITIES
			&& Distance( es->pos.trBase, cl.snap.ps.origin ) < NN_GONE_NEAR )
			near[es->number] = qtrue;
	}
	for ( int num = 0; num < MAX_GENTITIES; num++ ) {
		if ( nn.wasNear[num] && !near[num] && !NN_InSnapshot( num ) ) {
			nn.listed[num] = qfalse;
			nn.newAsks[num] = 0;
			nn.names[num][0] = '\0';
		}
	}
	memcpy( nn.wasNear, near, sizeof( near ) );
}

// an NPC near us that the last list didn't have: just spawned, most likely
static qboolean NN_NewNpc( void ) {
	qboolean found = qfalse;

	for ( int i = 0; i < cl.snap.numEntities; i++ ) {
		const entityState_t *es = NN_Ent( i );

		if ( es->eType != ET_NPC || es->number < 0 || es->number >= MAX_GENTITIES || nn.listed[es->number]
			|| nn.newAsks[es->number] >= NN_NEW_ASKS )
			continue;
		nn.newAsks[es->number]++;
		found = qtrue;
	}
	return found;
}

// the names to everyone with this client, in parts, when they changed or a while after they went
static void NN_Share( qboolean again ) {
	char text[NN_PART_LEN * NN_MAX_PARTS];
	char parts[NN_MAX_PARTS][NN_PART_LEN + 1];
	int numParts = 1, len = 0;

	text[0] = '\0';
	parts[0][0] = '\0';
	for ( int num = 0; num < MAX_GENTITIES; num++ ) {
		const char *entry;
		int entryLen;

		if ( !nn.names[num][0] )
			continue;
		entry = va( "%s%i=%s", parts[numParts - 1][0] ? "|" : "", num, nn.names[num] );
		entryLen = strlen( entry );
		if ( len + entryLen > NN_PART_LEN ) {
			if ( numParts == NN_MAX_PARTS )
				break;
			parts[numParts++][0] = '\0';
			len = 0;
			entry = va( "%i=%s", num, nn.names[num] );
			entryLen = strlen( entry );
		}
		Q_strcat( parts[numParts - 1], sizeof( parts[0] ), entry );
		len += entryLen;
		Q_strcat( text, sizeof( text ), va( "%i=%s|", num, nn.names[num] ) );
	}
	if ( !again && !strcmp( text, nn.sent ) && cls.realtime - nn.sentTime < NN_RESEND_MS )
		return;
	// nothing named, ever: nothing to say
	if ( !text[0] && !nn.sent[0] && !nn.sentTime )
		return;
	Q_strncpyz( nn.sent, text, sizeof( nn.sent ) );
	nn.sentTime = cls.realtime;
	for ( int i = 0; i < numParts; i++ ) {
		CL_AddReliableCommand( parts[i][0] ? va( "rpnotify -1 1 cmd:npcnames/%i/%i: %s", i + 1, numParts, parts[i] )
			: va( "rpnotify -1 1 cmd:npcnames/%i/%i", i + 1, numParts ), qfalse );
	}
}

static qboolean NN_Present( int c ) {
	const int ofs = cl.gameState.stringOffsets[CS_PLAYERS + c];

	return (qboolean)( ofs && cl.gameState.stringData[ofs] );
}

// someone new on the server: the names go again for them, once they're likely in
static void NN_WatchJoins( void ) {
	for ( int c = 0; c < MAX_CLIENTS; c++ ) {
		const qboolean here = NN_Present( c );

		if ( here && !nn.present[c] && nn.presentRead && c != clc.clientNum ) {
			nn.joinTime = cls.realtime;
			nn.joinSends = 0;
		}
		nn.present[c] = here;
	}
	nn.presentRead = qtrue;
}

static void NN_AdminFrame( void ) {
	NN_WatchJoins();
	// RPMod's commands aren't on a server of this computer's, and hidden ones need experimental
	if ( com_sv_running->integer || !CL_RpHooks_Experimental() )
		return;
	if ( !CL_RpAdmin_IsAdmin() ) {
		// asked once, quietly
		if ( !nn.rankAsked ) {
			nn.rankAsked = qtrue;
			CL_RpAdmin_AskQuietly();
		}
		return;
	}
	NN_WatchGone();
	// shown here at once, shared once the lines stop coming
	if ( NN_ReadList() ) {
		nn.changedTime = cls.realtime;
		nn.shareDue = qtrue;
	}
	if ( nn.shareDue && cls.realtime - nn.changedTime >= NN_SHARE_SETTLE_MS ) {
		nn.shareDue = qfalse;
		NN_Share( qfalse );
	}
	if ( nn.joinTime && nn.readList && cls.realtime - nn.joinTime >= nnJoinMs[nn.joinSends] ) {
		NN_Share( qtrue );
		if ( ++nn.joinSends >= NN_JOIN_SENDS )
			nn.joinTime = 0;
	}
	// NPCs anywhere on the map, not only near: whoever joins may be by them. One just spawned
	// near us asks at once
	if ( CL_NpcManager_Waiting() )
		return;
	if ( ( !nn.askTime || cls.realtime - nn.askTime >= NN_ASK_MS )
		|| ( cls.realtime - nn.askTime >= NN_NEW_ASK_MS && NN_NewNpc() ) ) {
		nn.askTime = cls.realtime;
		CL_NpcManager_RequestList();
	}
}

/*
===============================================================================

EVERYONE

===============================================================================
*/

// cmd:npcnames/<part>/<parts>: <num>=<name>|...
void CL_NpcNames_Hook( char **argv, int argc, const char *message ) {
	char buf[MAX_STRING_CHARS];
	char *entry, *next;

	if ( argc < 1 )
		return;
	// an admin's own list is already in
	if ( CL_RpAdmin_IsAdmin() && CL_RpHooks_Experimental() && nn.readList )
		return;
	if ( atoi( argv[0] ) == 1 )
		memset( nn.names, 0, sizeof( nn.names ) );
	if ( !message || !message[0] )
		return;
	Q_strncpyz( buf, message, sizeof( buf ) );
	for ( entry = buf; entry && *entry; entry = next ) {
		char *eq;
		int num;

		next = strchr( entry, '|' );
		if ( next )
			*next++ = '\0';
		while ( *entry == ' ' )
			entry++;
		eq = strchr( entry, '=' );
		if ( !eq )
			continue;
		*eq = '\0';
		num = atoi( entry );
		if ( num >= 0 && num < MAX_GENTITIES )
			NN_CleanName( eq + 1, nn.names[num], NN_NAME_LEN );
	}
}

// the main view, from the scene the cgame draws
void CL_NpcNames_View( const refdef_t *fd ) {
	if ( ( fd->rdflags & RDF_NOWORLDMODEL ) || fd->width < cls.glconfig.vidWidth / 2 )
		return;
	VectorCopy( fd->vieworg, nn.viewOrg );
	VectorCopy( fd->viewaxis[0], nn.viewFwd );
	nn.haveView = qtrue;
}

// how far along the ray it enters the box, -1 if it misses
static float NN_RayBox( const vec3_t org, const vec3_t dir, const vec3_t mins, const vec3_t maxs ) {
	float tNear = 0.0f, tFar = NN_RANGE;

	for ( int k = 0; k < 3; k++ ) {
		if ( fabs( dir[k] ) < 0.00001f ) {
			if ( org[k] < mins[k] || org[k] > maxs[k] )
				return -1.0f;
			continue;
		}
		float t1 = ( mins[k] - org[k] ) / dir[k];
		float t2 = ( maxs[k] - org[k] ) / dir[k];
		if ( t1 > t2 ) {
			const float t = t1;
			t1 = t2;
			t2 = t;
		}
		tNear = Q_max( tNear, t1 );
		tFar = Q_min( tFar, t2 );
		if ( tNear > tFar )
			return -1.0f;
	}
	return tNear;
}

// the named NPC the crosshair is on, -1 none
static int NN_Aimed( void ) {
	int best = -1;
	float bestT = NN_RANGE;

	for ( int i = 0; i < cl.snap.numEntities; i++ ) {
		const entityState_t *es = NN_Ent( i );
		vec3_t org, mins, maxs;
		float t;

		if ( es->eType != ET_NPC || ( es->eFlags & ( EF_DEAD | EF_NODRAW ) ) || es->number < 0 || es->number >= MAX_GENTITIES
			|| !nn.names[es->number][0] )
			continue;
		if ( !CL_NpcWalk_PoseOf( es->number, org ) )
			VectorCopy( es->pos.trBase, org );
		// a person's box, a bit generous
		VectorSet( mins, org[0] - 18.0f, org[1] - 18.0f, org[2] - 24.0f );
		VectorSet( maxs, org[0] + 18.0f, org[1] + 18.0f, org[2] + 44.0f );
		t = NN_RayBox( nn.viewOrg, nn.viewFwd, mins, maxs );
		if ( t < 0.0f || t >= bestT )
			continue;
		best = es->number;
		bestT = t;
	}
	if ( best >= 0 ) {
		trace_t tr;
		vec3_t end;

		// a wall in between hides it
		VectorMA( nn.viewOrg, bestT, nn.viewFwd, end );
		CM_BoxTrace( &tr, nn.viewOrg, end, vec3_origin, vec3_origin, 0, CONTENTS_SOLID, qfalse );
		if ( tr.fraction < 1.0f )
			best = -1;
	}
	return best;
}

static qboolean NN_InSnapshot( int num ) {
	for ( int i = 0; i < cl.snap.numEntities; i++ ) {
		const entityState_t *es = NN_Ent( i );

		if ( es->number == num )
			return (qboolean)( es->eType == ET_NPC && !( es->eFlags & EF_DEAD ) );
	}
	return qfalse;
}

// a name, letter by letter, centred; colour codes in it are kept
static void NN_DrawName( const char *name, float alpha ) {
	static qhandle_t font;
	vec4_t color = { 1.0f, 1.0f, 1.0f, 1.0f };
	char ch[2] = { 0, 0 };
	float width = 0.0f, x;
	int letters = 0;

	if ( !font )
		font = re->RegisterFont( NN_NAME_FONT );
	if ( !font )
		font = cls.menuFont;
	for ( const char *p = name; *p; p++ ) {
		if ( Q_IsColorString( p ) ) {
			p++;
			continue;
		}
		ch[0] = *p;
		width += re->Font_StrLenPixels( ch, font, NN_NAME_SCALE ) + NN_NAME_GAP;
		letters++;
	}
	if ( !letters )
		return;
	width -= NN_NAME_GAP;
	x = ( SCREEN_WIDTH - width ) * 0.5f;
	if ( CL_RpCinematic_Hide2D( x, (float)NN_NAME_Y, width, 20.0f ) )
		return;

	color[3] = alpha * NN_NAME_ALPHA;
	for ( const char *p = name; *p; p++ ) {
		if ( Q_IsColorString( p ) ) {
			VectorCopy( g_color_table[ColorIndex( p[1] )], color );
			p++;
			continue;
		}
		ch[0] = *p;
		re->Font_DrawString( (int)x, NN_NAME_Y, ch, color, font, -1, NN_NAME_SCALE );
		x += re->Font_StrLenPixels( ch, font, NN_NAME_SCALE ) + NN_NAME_GAP;
	}
}

void CL_NpcNames_Draw( void ) {
	const char *name;
	float alpha = 1.0f;
	int aimed;

	if ( !NN_On() || cls.state != CA_ACTIVE || !nn.haveView || ( Key_GetCatcher() & KEYCATCH_UI ) || !Cvar_VariableIntegerValue( "cg_draw2D" ) )
		return;
	aimed = NN_Aimed();
	if ( aimed >= 0 ) {
		nn.shownNum = aimed;
		nn.shownTime = cls.realtime;
	}
	if ( nn.shownNum < 0 || !nn.shownTime || cls.realtime - nn.shownTime > NN_LINGER_MS || !NN_InSnapshot( nn.shownNum ) )
		return;
	name = nn.names[nn.shownNum];
	if ( !name[0] )
		return;
	if ( cls.realtime - nn.shownTime > NN_LINGER_MS - NN_FADE_MS )
		alpha = (float)( NN_LINGER_MS - ( cls.realtime - nn.shownTime ) ) / NN_FADE_MS;
	NN_DrawName( name, alpha );
}

void CL_NpcNames_Frame( void ) {
	if ( !NN_On() )
		return;
	NN_AdminFrame();
}

void CL_NpcNames_Disconnect( void ) {
	memset( &nn, 0, sizeof( nn ) );
	nn.shownNum = -1;
}

void CL_NpcNames_Init( void ) {
	cl_npcNames = Cvar_Get( "cl_npcNames", "1", CVAR_ARCHIVE_ND, "Show an NPC's name when the crosshair is on it" );
	CL_NpcNames_Disconnect();
}
