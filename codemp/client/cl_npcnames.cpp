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
// without. Only RPMod's "npc list" says who's who, and only admins may ask for it. Nothing is
// read or shown until an admin with this client and experimental on runs /npcnames: the list is
// read then, once, and the names go to everyone with this client, the admin too. Names not made
// for roleplay stay out of the way until an admin wants them. /npcnames clear takes them back:
//
//   rpnotify -1 1 cmd:npcnames/<part>/<parts>: <entity number>=<name>|<entity number>=<name>...
//
// Part 1 starts the list over; part 1 of 1 with nothing clears it.

#include "client.h"
#include "qcommon/cm_public.h"

#define NN_NAME_LEN			48
#define NN_SHARE_SETTLE_MS	400		// the names shared once the list's lines stop coming this long
#define NN_SHARE_WAIT_MS	5000	// /npcnames shares what's known if no fresh list came by then
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

static cvar_t *cl_npcNames;

static struct {
	char		names[MAX_GENTITIES][NN_NAME_LEN];	// shown, by entity number, "" none known

	// the main view, as the scene was last drawn
	qboolean	haveView;
	vec3_t		viewOrg, viewFwd;

	// the admin's side: the list read for /npcnames, shown only once it's shared
	char		read[MAX_GENTITIES][NN_NAME_LEN];
	int			readList;			// the NPC manager's list read into read, by its time
	int			readCount;			// and how many were in it then
	int			changedTime;		// the list's lines last came then
	int			shareAsked;			// /npcnames asked then, 0 not waiting to share
	int			shareFromList;		// the list read when it did

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
	memset( nn.read, 0, sizeof( nn.read ) );
	for ( int i = 0; i < count; i++ ) {
		const char *type, *name;
		int num;

		if ( !CL_NpcManager_GetNpc( i, &num, &type, &name ) || num < 0 || num >= MAX_GENTITIES )
			continue;
		if ( name && name[0] )
			NN_CleanName( name, nn.read[num], NN_NAME_LEN );
	}
	return qtrue;
}

// the names read to everyone with this client, in parts, and shown here as they are there
static void NN_Share( void ) {
	char parts[NN_MAX_PARTS][NN_PART_LEN + 1];
	int numParts = 1, len = 0, named = 0;

	parts[0][0] = '\0';
	for ( int num = 0; num < MAX_GENTITIES; num++ ) {
		const char *entry;
		int entryLen;

		if ( !nn.read[num][0] )
			continue;
		entry = va( "%s%i=%s", parts[numParts - 1][0] ? "|" : "", num, nn.read[num] );
		entryLen = strlen( entry );
		if ( len + entryLen > NN_PART_LEN ) {
			if ( numParts == NN_MAX_PARTS )
				break;
			parts[numParts++][0] = '\0';
			len = 0;
			entry = va( "%i=%s", num, nn.read[num] );
			entryLen = strlen( entry );
		}
		Q_strcat( parts[numParts - 1], sizeof( parts[0] ), entry );
		len += entryLen;
		named++;
	}
	if ( !named ) {
		Com_Printf( "npcnames: no NPC on the map has a name to share\n" );
		return;
	}
	for ( int i = 0; i < numParts; i++ ) {
		CL_AddReliableCommand( parts[i][0] ? va( "rpnotify -1 1 cmd:npcnames/%i/%i: %s", i + 1, numParts, parts[i] )
			: va( "rpnotify -1 1 cmd:npcnames/%i/%i", i + 1, numParts ), qfalse );
	}
	memcpy( nn.names, nn.read, sizeof( nn.names ) );
	Com_Printf( "npcnames: %i NPC name%s shared with everyone with this client\n", named, named == 1 ? "" : "s" );
}

// /npcnames waiting: shared once the fresh list's lines stop coming, or with what's known if none came
static void NN_AdminFrame( void ) {
	if ( !nn.shareAsked )
		return;
	if ( NN_ReadList() )
		nn.changedTime = cls.realtime;
	if ( ( nn.readList != nn.shareFromList && cls.realtime - nn.changedTime >= NN_SHARE_SETTLE_MS )
		|| cls.realtime - nn.shareAsked >= NN_SHARE_WAIT_MS ) {
		nn.shareAsked = 0;
		NN_Share();
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
	NN_AdminFrame();
}

// npcnames [clear]: an admin shares the NPC names with everyone with this client, or takes them back
static void NN_Cmd_f( void ) {
	const qboolean clear = (qboolean)!Q_stricmp( Cmd_Argv( 1 ), "clear" );

	if ( Cmd_Argc() > 1 && !clear ) {
		Com_Printf( "npcnames: shares the NPC names with everyone with this client; npcnames clear takes them back\n" );
		return;
	}
	if ( cls.state != CA_ACTIVE || com_sv_running->integer ) {
		Com_Printf( "npcnames: only on an RPMod server\n" );
		return;
	}
	if ( !CL_RpHooks_Experimental() ) {
		Com_Printf( "npcnames: needs experimental on (/rpexperimental on)\n" );
		return;
	}
	if ( !CL_RpAdmin_Allow( "npcnames", "npcnames" ) )
		return;
	if ( clear ) {
		nn.shareAsked = 0;
		memset( nn.names, 0, sizeof( nn.names ) );
		CL_AddReliableCommand( "rpnotify -1 1 cmd:npcnames/1/1", qfalse );
		Com_Printf( "npcnames: NPC names taken back from everyone with this client\n" );
		return;
	}
	// the list read again first, so the names are as they are now
	nn.shareAsked = cls.realtime;
	nn.shareFromList = nn.readList;
	if ( !CL_NpcManager_Waiting() )
		CL_NpcManager_RequestList();
	Com_Printf( "npcnames: reading the NPC list to share it...\n" );
}

void CL_NpcNames_Disconnect( void ) {
	memset( &nn, 0, sizeof( nn ) );
	nn.shownNum = -1;
}

void CL_NpcNames_Init( void ) {
	cl_npcNames = Cvar_Get( "cl_npcNames", "1", CVAR_ARCHIVE_ND, "Show an NPC's name when the crosshair is on it" );
	Cmd_AddCommand( "npcnames", NN_Cmd_f, "Admins: share the NPC names with everyone with this client; npcnames clear takes them back" );
	CL_NpcNames_Disconnect();
}
