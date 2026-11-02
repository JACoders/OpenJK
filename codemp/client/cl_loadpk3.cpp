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

// cl_loadpk3.cpp -- pk3s put in the game's folders while it runs, used without a vid_restart
//
//   loadpk3 [name]   loads the new pk3s in the game's folders (base and the mod's), or only that
//                    one, in front of every other
//
// The renderer forgets the models, skins and shaders it couldn't find, and the sound system its
// missing sounds: asked for again, they're looked for in the new pk3s. What the game already has
// is asked for again where the new pk3s have it:
//   - players whose model is in them: the cgame is handed their info with another model, then
//     their own, so it loads it anew (with cg_deferPlayers off for it, so it doesn't wait)
//   - the map's models and sounds (CS_MODELS, CS_SOUNDS) in them: their configstring again
// A cgame that doesn't put the animation on a player's new model leaves it in its bind pose until
// the animation next changes: as the cgame attaches it to the player, their animation is put on it.
// What changes files the game had already loaded, and maps, need a vid_restart or the next map.
// Not on a pure server: it only lets the client use the pk3s it lists.

#include "client.h"

#define LP_MAX_PAKS			64
#define LP_MAX_SHADER_FILES	256
#define LP_PLACEHOLDER		"kyle/default"
#define LP_PLACEHOLDER_ALT	"jan/default"	// for a player whose own is kyle
#define LP_DEFER_MS			3000		// cg_deferPlayers is put back this long after

static struct {
	qboolean	deferSaved;
	char		defer[MAX_CVAR_VALUE_STRING];
	int			deferUntil;

	// the players loaded again, whose new model is given their animation
	qboolean	reanim[MAX_CLIENTS];
	qboolean	reanimPending;
	int			reanimCmd;		// the cgame's number for the reload's last command
} lp;

// the players whose model is in the new pk3s: the cgame loads them again
static int LP_ReloadPlayers( void ) {
	static char info[MAX_CLIENTS][MAX_INFO_STRING];
	qboolean reload[MAX_CLIENTS];
	int count = 0;

	for ( int c = 0; c < MAX_CLIENTS; c++ ) {
		const int ofs = cl.gameState.stringOffsets[CS_PLAYERS + c];
		char model[MAX_QPATH], *slash;

		reload[c] = qfalse;
		if ( !ofs || !cl.gameState.stringData[ofs] )
			continue;
		Q_strncpyz( info[c], cl.gameState.stringData + ofs, sizeof( info[c] ) );
		Q_strncpyz( model, Info_ValueForKey( info[c], "model" ), sizeof( model ) );
		if ( ( slash = strchr( model, '/' ) ) != NULL )
			*slash = '\0';
		if ( !model[0] || !FS_AddedPaksHave( va( "models/players/%s/", model ) ) )
			continue;
		reload[c] = qtrue;
		count++;
	}
	if ( !count )
		return 0;

	// loaded at once, not when the scoreboard next shows
	if ( !lp.deferSaved && Cvar_Flags( "cg_deferPlayers" ) != CVAR_NONEXISTENT ) {
		Q_strncpyz( lp.defer, Cvar_VariableString( "cg_deferPlayers" ), sizeof( lp.defer ) );
		lp.deferSaved = qtrue;
	}
	if ( lp.deferSaved ) {
		Cvar_Set( "cg_deferPlayers", "0" );
		lp.deferUntil = cls.realtime + LP_DEFER_MS;
	}

	// all of them on another model first: one still on the old would be copied, not loaded
	for ( int c = 0; c < MAX_CLIENTS; c++ ) {
		char other[MAX_INFO_STRING];

		if ( !reload[c] )
			continue;
		// a \ at its start, or the key put in front runs into the first one
		Com_sprintf( other, sizeof( other ), "%s%s", info[c][0] == '\\' ? "" : "\\", info[c] );
		Info_SetValueForKey( other, "model", Q_stricmpn( Info_ValueForKey( info[c], "model" ), "kyle", 4 ) ? LP_PLACEHOLDER : LP_PLACEHOLDER_ALT );
		CL_QueueCgameCommand( va( "cs %i \"%s\"", CS_PLAYERS + c, other ) );
	}
	for ( int c = 0; c < MAX_CLIENTS; c++ ) {
		if ( !reload[c] )
			continue;
		lp.reanimCmd = CL_QueueCgameCommand( va( "cs %i \"%s\"", CS_PLAYERS + c, info[c] ) );
		lp.reanim[c] = qtrue;
	}
	lp.reanimPending = qtrue;
	return count;
}

// the map's models and sounds that are in the new pk3s: registered again by the cgame
static int LP_ReloadConfigstrings( int first, int count ) {
	int reloaded = 0;

	for ( int i = 1; i < count; i++ ) {
		const int ofs = cl.gameState.stringOffsets[first + i];
		const char *s = cl.gameState.stringData + ofs;
		char name[MAX_QPATH];

		if ( !ofs || !s[0] || s[0] == '*' || s[0] == '$' )
			continue;
		// a sound may be a .wav asked for and an .mp3 found, a model any of its kinds
		COM_StripExtension( s, name, sizeof( name ) );
		if ( !FS_AddedPaksHave( va( "%s.", name ) ) )
			continue;
		if ( strchr( s, '"' ) )
			continue;
		CL_QueueCgameCommand( va( "cs %i \"%s\"", first + i, s ) );
		reloaded++;
	}
	return reloaded;
}

static void CL_LoadPk3_f( void ) {
	static char names[LP_MAX_PAKS][MAX_QPATH];
	static char shaderNames[LP_MAX_SHADER_FILES][MAX_QPATH];
	const char *shaderFiles[LP_MAX_SHADER_FILES];
	const char *only = Cmd_Argv( 1 );
	int numPaks, numShaders, forgotten = 0, players = 0, models = 0, sounds = 0;

	if ( !Q_stricmp( only, "help" ) || !Q_stricmp( only, "?" ) ) {
		Com_Printf( "loadpk3 : loads the pk3s put in the game's folders since it started, without a vid_restart\n"
			"loadpk3 <name> : only that one\n" );
		return;
	}
	if ( FS_ServerIsPure() ) {
		Com_Printf( S_COLOR_YELLOW "loadpk3: this server is pure: it only lets you use the pk3s it lists. Nothing was loaded.\n" );
		return;
	}

	numPaks = FS_AddNewPaks( only, names, LP_MAX_PAKS );
	if ( !numPaks ) {
		if ( only[0] )
			Com_Printf( "loadpk3: no new %s in the game's folders (it isn't there, or it's loaded already)\n", only );
		else
			Com_Printf( "loadpk3: no new pk3s in the game's folders\n" );
		return;
	}
	for ( int i = 0; i < numPaks && i < LP_MAX_PAKS; i++ )
		Com_Printf( "loadpk3: loaded %s\n", names[i] );

	// what couldn't be found is looked for again
	numShaders = FS_AddedPaksFiles( "shaders/", ".shader", shaderNames, LP_MAX_SHADER_FILES );
	for ( int i = 0; i < numShaders; i++ )
		shaderFiles[i] = shaderNames[i];
	if ( cls.rendererStarted && re && re->ext.ForgetMissing )
		forgotten += re->ext.ForgetMissing( shaderFiles, numShaders );
	else if ( cls.rendererStarted )
		Com_Printf( S_COLOR_YELLOW "loadpk3: this renderer can't look again for what it couldn't find: use vid_restart\n" );
	forgotten += S_ForgetMissing();

	// and what the game already has asked for again
	if ( cls.state == CA_ACTIVE && !clc.demoplaying ) {
		players = LP_ReloadPlayers();
		models = LP_ReloadConfigstrings( CS_MODELS, MAX_MODELS );
		sounds = LP_ReloadConfigstrings( CS_SOUNDS, MAX_SOUNDS );
	}
	Com_Printf( "loadpk3: %i shader file%s read; %i missing things to look for again; %i player%s, %i model%s and %i sound%s loaded again\n",
		numShaders, numShaders == 1 ? "" : "s", forgotten, players, players == 1 ? "" : "s",
		models, models == 1 ? "" : "s", sounds, sounds == 1 ? "" : "s" );
}

// the cgame attaching a model to an entity: a player loaded again gets their animation on it, as
// the latest snapshot has it
void CL_LoadPk3_Attached( void *ghoul2, int entityNum ) {
	if ( !lp.reanimPending || entityNum < 0 || entityNum >= MAX_CLIENTS || !lp.reanim[entityNum] || !cl.snap.valid )
		return;
	if ( entityNum == cl.snap.ps.clientNum ) {
		CL_EmotePreview_PlayerAnims( ghoul2, cl.snap.ps.legsAnim, cl.snap.ps.torsoAnim );
		Com_DPrintf( "loadpk3: player %i's new model given animations %i %i\n", entityNum, cl.snap.ps.legsAnim, cl.snap.ps.torsoAnim );
		return;
	}
	for ( int i = 0; i < cl.snap.numEntities; i++ ) {
		const entityState_t *es = &cl.parseEntities[( cl.snap.parseEntitiesNum + i ) & ( MAX_PARSE_ENTITIES - 1 )];

		if ( es->number == entityNum ) {
			CL_EmotePreview_PlayerAnims( ghoul2, es->legsAnim, es->torsoAnim );
			Com_DPrintf( "loadpk3: player %i's new model given animations %i %i\n", entityNum, es->legsAnim, es->torsoAnim );
			return;
		}
	}
}

// cg_deferPlayers back as it was, once the players are loaded
void CL_LoadPk3_Frame( void ) {
	// the reload ran (its models attached as it did), or a new connection numbers its commands anew
	if ( lp.reanimPending && ( cls.state != CA_ACTIVE || CL_LastCgameCommand() >= lp.reanimCmd ) ) {
		lp.reanimPending = qfalse;
		memset( lp.reanim, 0, sizeof( lp.reanim ) );
	}
	if ( !lp.deferSaved || cls.realtime < lp.deferUntil )
		return;
	Cvar_Set( "cg_deferPlayers", lp.defer );
	lp.deferSaved = qfalse;
}

void CL_LoadPk3_Init( void ) {
	memset( &lp, 0, sizeof( lp ) );
	Cmd_AddCommand( "loadpk3", CL_LoadPk3_f, "Loads the pk3s put in the game's folders since it started, without a vid_restart" );
}
