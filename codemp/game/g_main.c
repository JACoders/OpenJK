/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2005 - 2015, ioquake3 contributors
Copyright (C) 2013 - 2015, OpenJK contributors

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


#include "g_local.h"
#include "g_ICARUScb.h"
#include "g_nav.h"
#include "bg_saga.h"
#include "b_local.h"

level_locals_t	level;

int		eventClearTime = 0;
static int navCalcPathTime = 0;
extern int fatalErrors;

int killPlayerTimer = 0;

gentity_t		g_entities[MAX_GENTITIES];
gclient_t		g_clients[MAX_CLIENTS];

qboolean gDuelExit = qfalse;

void G_InitGame					( int levelTime, int randomSeed, int restart );
void G_RunFrame					( int levelTime );
void G_ShutdownGame				( int restart );
void CheckExitRules				( void );
void G_ROFF_NotetrackCallback	( gentity_t *cent, const char *notetrack);

extern stringID_table_t setTable[];

qboolean G_ParseSpawnVars( qboolean inSubBSP );
void G_SpawnGEntityFromSpawnVars( qboolean inSubBSP );


qboolean NAV_ClearPathToPoint( gentity_t *self, vec3_t pmins, vec3_t pmaxs, vec3_t point, int clipmask, int okToHitEntNum );
qboolean NPC_ClearLOS2( gentity_t *ent, const vec3_t end );
int NAVNEW_ClearPathBetweenPoints(vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, int ignore, int clipmask);
qboolean NAV_CheckNodeFailedForEnt( gentity_t *ent, int nodeNum );
qboolean G_EntIsUnlockedDoor( int entityNum );
qboolean G_EntIsDoor( int entityNum );
qboolean G_EntIsBreakable( int entityNum );
qboolean G_EntIsRemovableUsable( int entNum );
void CP_FindCombatPointWaypoints( void );

/*
================
G_FindTeams

Chain together all entities with a matching team field.
Entity teams are used for item groups and multi-entity mover groups.

All but the first will have the FL_TEAMSLAVE flag set and teammaster field set
All but the last will have the teamchain field set to the next one
================
*/
void G_FindTeams( void ) {
	gentity_t	*e, *e2;
	int		i, j;
	int		c, c2;

	c = 0;
	c2 = 0;
	for ( i=MAX_CLIENTS, e=g_entities+i ; i < level.num_entities ; i++,e++ ) {
		if (!e->inuse)
			continue;
		if (!e->team)
			continue;
		if (e->flags & FL_TEAMSLAVE)
			continue;
		if (e->r.contents==CONTENTS_TRIGGER)
			continue;//triggers NEVER link up in teams!
		e->teammaster = e;
		c++;
		c2++;
		for (j=i+1, e2=e+1 ; j < level.num_entities ; j++,e2++)
		{
			if (!e2->inuse)
				continue;
			if (!e2->team)
				continue;
			if (e2->flags & FL_TEAMSLAVE)
				continue;
			if (!strcmp(e->team, e2->team))
			{
				c2++;
				e2->teamchain = e->teamchain;
				e->teamchain = e2;
				e2->teammaster = e;
				e2->flags |= FL_TEAMSLAVE;

				// make sure that targets only point at the master
				if ( e2->targetname ) {
					e->targetname = e2->targetname;
					e2->targetname = NULL;
				}
			}
		}
	}

//	trap->Print ("%i teams with %i entities\n", c, c2);
}

sharedBuffer_t gSharedBuffer;

void WP_SaberLoadParms( void );
void BG_VehicleLoadParms( void );

void G_CacheGametype( void )
{
	// check some things
	if ( g_gametype.string[0] && isalpha( g_gametype.string[0] ) )
	{
		int gt = BG_GetGametypeForString( g_gametype.string );
		if ( gt == -1 )
		{
			trap->Print( "Gametype '%s' unrecognised, defaulting to FFA/Deathmatch\n", g_gametype.string );
			level.gametype = GT_FFA;
		}
		else
			level.gametype = gt;
	}
	else if ( g_gametype.integer < 0 || level.gametype >= GT_MAX_GAME_TYPE )
	{
		trap->Print( "g_gametype %i is out of range, defaulting to 0\n", level.gametype );
		level.gametype = GT_FFA;
	}
	else
		level.gametype = atoi( g_gametype.string );

	trap->Cvar_Set( "g_gametype", va( "%i", level.gametype ) );
	trap->Cvar_Update( &g_gametype );
}

void G_CacheMapname( const vmCvar_t *mapname )
{
	Com_sprintf( level.mapname, sizeof( level.mapname ), "maps/%s.bsp", mapname->string );
	Com_sprintf( level.rawmapname, sizeof( level.rawmapname ), "maps/%s", mapname->string );
}

// zyk: this function spawns an info_player_deathmatch entity in the map
extern void zyk_set_entity_field(gentity_t *ent, char *key, char *value);
extern void zyk_spawn_entity(gentity_t *ent);
void zyk_create_info_player_deathmatch(int x, int y, int z, int yaw)
{
	gentity_t *spawn_ent = NULL;

	spawn_ent = G_Spawn();
	if (spawn_ent)
	{
		int i = 0;
		gentity_t *this_ent;
		gentity_t *spawn_point_ent = NULL;

		for (i = 0; i < level.num_entities; i++)
		{
			this_ent = &g_entities[i];
			if (Q_stricmp( this_ent->classname, "info_player_deathmatch") == 0)
			{ // zyk: found the original SP map spawn point
				spawn_point_ent = this_ent;
				break;
			}
		}

		zyk_set_entity_field(spawn_ent,"classname","info_player_deathmatch");
		zyk_set_entity_field(spawn_ent,"origin",va("%d %d %d",x,y,z));
		zyk_set_entity_field(spawn_ent,"angles",va("0 %d 0",yaw));
		if (spawn_point_ent && spawn_point_ent->target)
		{ // zyk: setting the target for SP map spawn points so they will work properly
			zyk_set_entity_field(spawn_ent,"target",spawn_point_ent->target);
		}

		zyk_spawn_entity(spawn_ent);
	}
}

// zyk: creates a ctf flag spawn point
void zyk_create_ctf_flag_spawn(int x, int y, int z, qboolean redteam)
{
	gentity_t *spawn_ent = NULL;

	spawn_ent = G_Spawn();
	if (spawn_ent)
	{
		if (redteam == qtrue)
			zyk_set_entity_field(spawn_ent,"classname","team_CTF_redflag");
		else
			zyk_set_entity_field(spawn_ent,"classname","team_CTF_blueflag");

		zyk_set_entity_field(spawn_ent,"origin",va("%d %d %d",x,y,z));
		zyk_spawn_entity(spawn_ent);
	}
}

// zyk: creates a ctf player spawn point
void zyk_create_ctf_player_spawn(int x, int y, int z, int yaw, qboolean redteam, qboolean team_begin_spawn_point)
{
	gentity_t *spawn_ent = NULL;

	spawn_ent = G_Spawn();
	if (spawn_ent)
	{
		if (redteam == qtrue)
		{
			if (team_begin_spawn_point == qtrue)
				zyk_set_entity_field(spawn_ent,"classname","team_CTF_redplayer");
			else
				zyk_set_entity_field(spawn_ent,"classname","team_CTF_redspawn");
		}
		else
		{
			if (team_begin_spawn_point == qtrue)
				zyk_set_entity_field(spawn_ent,"classname","team_CTF_blueplayer");
			else
				zyk_set_entity_field(spawn_ent,"classname","team_CTF_bluespawn");
		}

		zyk_set_entity_field(spawn_ent,"origin",va("%d %d %d",x,y,z));
		zyk_set_entity_field(spawn_ent,"angles",va("0 %d 0",yaw));

		zyk_spawn_entity(spawn_ent);
	}
}

// zyk: used to fix func_door entities in SP maps that wont work and must be removed without causing the door glitch
void fix_sp_func_door(gentity_t *ent)
{
	ent->spawnflags = 0;
	ent->flags = 0;
	GlobalUse(ent,ent,ent);
	G_FreeEntity( ent );
}


extern gentity_t *NPC_Spawn_Do( gentity_t *ent );

// zyk: spawn an npc at a given x, y and z coordinates
gentity_t *Zyk_NPC_SpawnType( char *npc_type, int x, int y, int z, int yaw )
{
	gentity_t		*NPCspawner;
	vec3_t			forward, end, viewangles;
	trace_t			trace;
	vec3_t origin;

	origin[0] = x;
	origin[1] = y;
	origin[2] = z;

	viewangles[0] = 0;
	viewangles[1] = yaw;
	viewangles[2] = 0;

	NPCspawner = G_Spawn();

	if(!NPCspawner)
	{
		Com_Printf( S_COLOR_RED"NPC_Spawn Error: Out of entities!\n" );
		return NULL;
	}

	NPCspawner->think = G_FreeEntity;
	NPCspawner->nextthink = level.time + FRAMETIME;

	//rwwFIXMEFIXME: Care about who is issuing this command/other clients besides 0?
	//Spawn it at spot of first player
	//FIXME: will gib them!
	AngleVectors(viewangles, forward, NULL, NULL);
	VectorNormalize(forward);
	VectorMA(origin, 0, forward, end);
	trap->Trace(&trace, origin, NULL, NULL, end, 0, MASK_SOLID, qfalse, 0, 0);
	VectorCopy(trace.endpos, end);
	end[2] -= 24;
	trap->Trace(&trace, trace.endpos, NULL, NULL, end, 0, MASK_SOLID, qfalse, 0, 0);
	VectorCopy(trace.endpos, end);
	end[2] += 24;
	G_SetOrigin(NPCspawner, end);
	VectorCopy(NPCspawner->r.currentOrigin, NPCspawner->s.origin);
	//set the yaw so that they face away from player
	NPCspawner->s.angles[1] = viewangles[1];

	trap->LinkEntity((sharedEntity_t *)NPCspawner);

	NPCspawner->NPC_type = G_NewString( npc_type );

	NPCspawner->count = 1;

	NPCspawner->delay = 0;

	NPCspawner = NPC_Spawn_Do( NPCspawner );

	if ( NPCspawner != NULL )
		return NPCspawner;

	G_FreeEntity( NPCspawner );

	return NULL;
}

/*
============
G_InitGame

============
*/
extern void RemoveAllWP(void);
extern void BG_ClearVehicleParseParms(void);
gentity_t *SelectRandomDeathmatchSpawnPoint( void );
void SP_info_jedimaster_start( gentity_t *ent );
void G_InitGame( int levelTime, int randomSeed, int restart ) {
	int					i;
	vmCvar_t	mapname;
	vmCvar_t	ckSum;
	char serverinfo[MAX_INFO_STRING] = {0};
	// zyk: variable used in the SP buged maps fix
	char zyk_mapname[128] = {0};
	FILE *zyk_entities_file = NULL;
	FILE *zyk_remap_file = NULL;

	//Init RMG to 0, it will be autoset to 1 if there is terrain on the level.
	trap->Cvar_Set("RMG", "0");
	RMG.integer = 0;

	//Clean up any client-server ghoul2 instance attachments that may still exist exe-side
	trap->G2API_CleanEntAttachments();

	BG_InitAnimsets(); //clear it out

	B_InitAlloc(); //make sure everything is clean

	trap->SV_RegisterSharedMemory( gSharedBuffer.raw );

	//Load external vehicle data
	BG_VehicleLoadParms();

	trap->Print ("------- Game Initialization -------\n");
	trap->Print ("gamename: %s\n", GAMEVERSION);
	trap->Print ("gamedate: %s\n", __DATE__);

	srand( randomSeed );

	G_RegisterCvars();

	G_ProcessIPBans();

	G_InitMemory();

	// set some level globals
	memset( &level, 0, sizeof( level ) );
	level.time = levelTime;
	level.startTime = levelTime;

	level.follow1 = level.follow2 = -1;

	level.snd_fry = G_SoundIndex("sound/player/fry.wav");	// FIXME standing in lava / slime

	level.snd_hack = G_SoundIndex("sound/player/hacking.wav");
	level.snd_medHealed = G_SoundIndex("sound/player/supp_healed.wav");
	level.snd_medSupplied = G_SoundIndex("sound/player/supp_supplied.wav");

	//trap->SP_RegisterServer("mp_svgame");

	if ( g_log.string[0] )
	{
		trap->FS_Open( g_log.string, &level.logFile, g_logSync.integer ? FS_APPEND_SYNC : FS_APPEND );
		if ( level.logFile )
			trap->Print( "Logging to %s\n", g_log.string );
		else
			trap->Print( "WARNING: Couldn't open logfile: %s\n", g_log.string );
	}
	else
		trap->Print( "Not logging game events to disk.\n" );

	trap->GetServerinfo( serverinfo, sizeof( serverinfo ) );
	G_LogPrintf( "------------------------------------------------------------\n" );
	G_LogPrintf( "InitGame: %s\n", serverinfo );

	if ( g_securityLog.integer )
	{
		if ( g_securityLog.integer == 1 )
			trap->FS_Open( SECURITY_LOG, &level.security.log, FS_APPEND );
		else if ( g_securityLog.integer == 2 )
			trap->FS_Open( SECURITY_LOG, &level.security.log, FS_APPEND_SYNC );

		if ( level.security.log )
			trap->Print( "Logging to "SECURITY_LOG"\n" );
		else
			trap->Print( "WARNING: Couldn't open logfile: "SECURITY_LOG"\n" );
	}
	else
		trap->Print( "Not logging security events to disk.\n" );


	G_LogWeaponInit();

	G_CacheGametype();

	G_InitWorldSession();

	// initialize all entities for this game
	memset( g_entities, 0, MAX_GENTITIES * sizeof(g_entities[0]) );
	level.gentities = g_entities;

	// initialize all clients for this game
	level.maxclients = sv_maxclients.integer;
	memset( g_clients, 0, MAX_CLIENTS * sizeof(g_clients[0]) );
	level.clients = g_clients;

	// set client fields on player ents
	for ( i=0 ; i<level.maxclients ; i++ ) {
		g_entities[i].client = level.clients + i;
	}

	// always leave room for the max number of clients,
	// even if they aren't all used, so numbers inside that
	// range are NEVER anything but clients
	level.num_entities = MAX_CLIENTS;

	for ( i=0 ; i<MAX_CLIENTS ; i++ ) {
		g_entities[i].classname = "clientslot";
	}

	// let the server system know where the entites are
	trap->LocateGameData( (sharedEntity_t *)level.gentities, level.num_entities, sizeof( gentity_t ),
		&level.clients[0].ps, sizeof( level.clients[0] ) );

	//Load sabers.cfg data
	WP_SaberLoadParms();

	NPC_InitGame();

	TIMER_Clear();
	//
	//ICARUS INIT START

//	Com_Printf("------ ICARUS Initialization ------\n");

	trap->ICARUS_Init();

//	Com_Printf ("-----------------------------------\n");

	//ICARUS INIT END
	//

	// reserve some spots for dead player bodies
	InitBodyQue();

	ClearRegisteredItems();

	//make sure saber data is loaded before this! (so we can precache the appropriate hilts)
	InitSiegeMode();

	trap->Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );
	G_CacheMapname( &mapname );
	trap->Cvar_Register( &ckSum, "sv_mapChecksum", "", CVAR_ROM );

	// navCalculatePaths	= ( trap->Nav_Load( mapname.string, ckSum.integer ) == qfalse );
	// zyk: commented line above. Was taking a lot of time to load some maps, example mp/duel7 and mp/siege_desert in FFA Mode
	// zyk: now it will always force calculating paths
	navCalculatePaths = qtrue;

	// zyk: getting mapname
	Q_strncpyz(zyk_mapname, Info_ValueForKey( serverinfo, "mapname" ), sizeof(zyk_mapname));

	level.is_vjun3_map = qfalse;
	if (Q_stricmp(zyk_mapname, "vjun3") == 0)
	{ // zyk: fixing vjun3 map. It will not load protocol_imp npc to prevent exceeding the npc model limit (16) and crashing clients
		level.is_vjun3_map = qtrue;
	}

	if (Q_stricmp(zyk_mapname, "yavin1") == 0 || Q_stricmp(zyk_mapname, "yavin1b") == 0 || Q_stricmp(zyk_mapname, "yavin2") == 0 || 
		Q_stricmp(zyk_mapname, "t1_danger") == 0 || Q_stricmp(zyk_mapname, "t1_fatal") == 0 || Q_stricmp(zyk_mapname, "t1_inter") == 0 ||
		Q_stricmp(zyk_mapname, "t1_rail") == 0 || Q_stricmp(zyk_mapname, "t1_sour") == 0 || Q_stricmp(zyk_mapname, "t1_surprise") == 0 ||
		Q_stricmp(zyk_mapname, "hoth2") == 0 || Q_stricmp(zyk_mapname, "hoth3") == 0 || Q_stricmp(zyk_mapname, "t2_dpred") == 0 ||
		Q_stricmp(zyk_mapname, "t2_rancor") == 0 || Q_stricmp(zyk_mapname, "t2_rogue") == 0 || Q_stricmp(zyk_mapname, "t2_trip") == 0 ||
		Q_stricmp(zyk_mapname, "t2_wedge") == 0 || Q_stricmp(zyk_mapname, "vjun1") == 0 || Q_stricmp(zyk_mapname, "vjun2") == 0 ||
		Q_stricmp(zyk_mapname, "vjun3") == 0 || Q_stricmp(zyk_mapname, "t3_bounty") == 0 || Q_stricmp(zyk_mapname, "t3_byss") == 0 ||
		Q_stricmp(zyk_mapname, "t3_hevil") == 0 || Q_stricmp(zyk_mapname, "t3_rift") == 0 || Q_stricmp(zyk_mapname, "t3_stamp") == 0 ||
		Q_stricmp(zyk_mapname, "taspir1") == 0 || Q_stricmp(zyk_mapname, "taspir2") == 0 || Q_stricmp(zyk_mapname, "kor1") == 0 ||
		Q_stricmp(zyk_mapname, "kor2") == 0)
	{
		level.sp_map = qtrue;
	}

	// parse the key/value pairs and spawn gentities
	G_SpawnEntitiesFromString(qfalse);

	if (level.gametype == GT_CTF)
	{ // zyk: maps that will now have support to CTF gametype (like some SP maps) must have the CTF flags placed before the G_CheckTeamItems function call
		if (Q_stricmp(zyk_mapname, "t1_fatal") == 0)
		{
			zyk_create_ctf_flag_spawn(-2366,-2561,4536,qtrue);
			zyk_create_ctf_flag_spawn(2484,1732,4656,qfalse);
		}
		else if (Q_stricmp(zyk_mapname, "t1_rail") == 0)
		{
			zyk_create_ctf_flag_spawn(-2607,-4,24,qtrue);
			zyk_create_ctf_flag_spawn(23146,-3,216,qfalse);
		}
		else if (Q_stricmp(zyk_mapname, "t1_surprise") == 0)
		{
			zyk_create_ctf_flag_spawn(1337,-6492,224,qtrue);
			zyk_create_ctf_flag_spawn(2098,4966,800,qfalse);
		}
		else if (Q_stricmp(zyk_mapname, "t2_dpred") == 0)
		{
			zyk_create_ctf_flag_spawn(3,-3974,664,qtrue);
			zyk_create_ctf_flag_spawn(-701,126,24,qfalse);
		}
		else if (Q_stricmp(zyk_mapname, "t2_trip") == 0)
		{
			zyk_create_ctf_flag_spawn(-20421,18244,1704,qtrue);
			zyk_create_ctf_flag_spawn(19903,-2638,1672,qfalse);
		}
		else if (Q_stricmp(zyk_mapname, "t3_bounty") == 0)
		{
			zyk_create_ctf_flag_spawn(-7538,-545,-327,qtrue);
			zyk_create_ctf_flag_spawn(614,-509,344,qfalse);
		}
	}

	// general initialization
	G_FindTeams();

	// make sure we have flags for CTF, etc
	if( level.gametype >= GT_TEAM ) {
		G_CheckTeamItems();
	}
	else if ( level.gametype == GT_JEDIMASTER )
	{
		trap->SetConfigstring ( CS_CLIENT_JEDIMASTER, "-1" );
	}

	if (level.gametype == GT_POWERDUEL)
	{
		trap->SetConfigstring ( CS_CLIENT_DUELISTS, va("-1|-1|-1") );
	}
	else
	{
		trap->SetConfigstring ( CS_CLIENT_DUELISTS, va("-1|-1") );
	}
// nmckenzie: DUEL_HEALTH: Default.
	trap->SetConfigstring ( CS_CLIENT_DUELHEALTHS, va("-1|-1|!") );
	trap->SetConfigstring ( CS_CLIENT_DUELWINNER, va("-1") );

	if (1)
	{ // zyk: registering all items because of entity system
		int item_it = 0;

		for (item_it = 0; item_it < bg_numItems; item_it++)
		{
			gitem_t *this_item = &bg_itemlist[item_it];
			if (this_item)
			{
				RegisterItem(this_item);
			}
		}
	}

	SaveRegisteredItems();

	//trap->Print ("-----------------------------------\n");

	if( level.gametype == GT_SINGLE_PLAYER || trap->Cvar_VariableIntegerValue( "com_buildScript" ) ) {
		G_ModelIndex( SP_PODIUM_MODEL );
		G_SoundIndex( "sound/player/gurp1.wav" );
		G_SoundIndex( "sound/player/gurp2.wav" );
	}

	if ( trap->Cvar_VariableIntegerValue( "bot_enable" ) ) {
		BotAISetup( restart );
		BotAILoadMap( restart );
		G_InitBots( );
	} else {
		G_LoadArenas();
	}

	if ( level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL )
	{
		G_LogPrintf("Duel Tournament Begun: kill limit %d, win limit: %d\n", fraglimit.integer, duel_fraglimit.integer );
	}

	if ( navCalculatePaths )
	{//not loaded - need to calc paths
		navCalcPathTime = level.time + START_TIME_NAV_CALC;//make sure all ents are in and linked
	}
	else
	{//loaded
		//FIXME: if this is from a loadgame, it needs to be sure to write this
		//out whenever you do a savegame since the edges and routes are dynamic...
		//OR: always do a navigator.CheckBlockedEdges() on map startup after nav-load/calc-paths
		//navigator.pathsCalculated = qtrue;//just to be safe?  Does this get saved out?  No... assumed
		trap->Nav_SetPathsCalculated(qtrue);
		//need to do this, because combatpoint waypoints aren't saved out...?
		CP_FindCombatPointWaypoints();
		navCalcPathTime = 0;

		/*
		if ( g_eSavedGameJustLoaded == eNO )
		{//clear all the failed edges unless we just loaded the game (which would include failed edges)
			trap->Nav_ClearAllFailedEdges();
		}
		*/
		//No loading games in MP.
	}

	if (level.gametype == GT_SIEGE)
	{ //just get these configstrings registered now...
		while (i < MAX_CUSTOM_SIEGE_SOUNDS)
		{
			if (!bg_customSiegeSoundNames[i])
			{
				break;
			}
			G_SoundIndex((char *)bg_customSiegeSoundNames[i]);
			i++;
		}
	}

	if ( level.gametype == GT_JEDIMASTER ) {
		gentity_t *ent = NULL;
		int i=0;
		for ( i=0, ent=g_entities; i<level.num_entities; i++, ent++ ) {
			if ( ent->isSaberEntity )
				break;
		}

		if ( i == level.num_entities ) {
			// no JM saber found. drop one at one of the player spawnpoints
			gentity_t *spawnpoint = SelectRandomDeathmatchSpawnPoint();

			if( !spawnpoint ) {
				trap->Error( ERR_DROP, "Couldn't find an FFA spawnpoint to drop the jedimaster saber at!\n" );
				return;
			}

			ent = G_Spawn();
			G_SetOrigin( ent, spawnpoint->s.origin );
			SP_info_jedimaster_start( ent );
		}
	}

	// zyk: initializing race mode
	level.race_mode = 0;

	// zyk: initializing quest_map value
	level.quest_map = 0;

	// zyk: initializing quest_note_id value
	level.quest_note_id = -1;
	level.universe_quest_note_id = -1;

	// zyk: initializing quest_effect_id value
	level.quest_effect_id = -1;

	level.chaos_portal_id = -1;

	// zyk: initializing bounty_quest_target_id value
	level.bounty_quest_target_id = 0;
	level.bounty_quest_choose_target = qtrue;

	// zyk: initializing guardian quest values
	level.guardian_quest = 0;
	level.initial_map_guardian_weapons = 0;

	level.boss_battle_music_reset_timer = 0;

	level.voting_player = -1;

	level.server_empty_change_map_timer = 0;
	level.num_fully_connected_clients = 0;

	level.guardian_quest_timer = 0;

	level.last_spawned_entity = NULL;

	level.ent_origin_set = qfalse;

	level.load_entities_timer = 0;
	strcpy(level.load_entities_file,"");

	if (1)
	{
		int zyk_iterator = 0;

		for (zyk_iterator = 0; zyk_iterator < MAX_RACERS; zyk_iterator++)
		{ // zyk: initializing race vehicle ids
			level.race_mode_vehicle[zyk_iterator] = -1;
		}

		for (zyk_iterator = 0; zyk_iterator < ENTITYNUM_MAX_NORMAL; zyk_iterator++)
		{ // zyk: initializing special power variables
			level.special_power_effects[zyk_iterator] = -1;
			level.special_power_effects_timer[zyk_iterator] = 0;
		}

		for (zyk_iterator = 0; zyk_iterator < MAX_CLIENTS; zyk_iterator++)
		{
			level.read_screen_message[zyk_iterator] = qfalse;
			level.screen_message_timer[zyk_iterator] = 0;
			level.ignored_players[zyk_iterator][0] = 0;
			level.ignored_players[zyk_iterator][1] = 0;
		}

		// zyk: initializing quest_crystal_id value
		for (zyk_iterator = 0; zyk_iterator < 3; zyk_iterator++)
		{
			level.quest_crystal_id[zyk_iterator] = -1;
		}
	}

	// zyk: added this fix for SP maps
	if (Q_stricmp(zyk_mapname, "academy1") == 0)
	{
		zyk_create_info_player_deathmatch(-1308,272,729,-90);
		zyk_create_info_player_deathmatch(-1508,272,729,-90);
	}
	else if (Q_stricmp(zyk_mapname, "academy2") == 0)
	{
		zyk_create_info_player_deathmatch(-1308,272,729,-90);
		zyk_create_info_player_deathmatch(-1508,272,729,-90);
	}
	else if (Q_stricmp(zyk_mapname, "academy3") == 0)
	{
		zyk_create_info_player_deathmatch(-1308,272,729,-90);
		zyk_create_info_player_deathmatch(-1508,272,729,-90);
	}
	else if (Q_stricmp(zyk_mapname, "academy4") == 0)
	{
		zyk_create_info_player_deathmatch(-1308,272,729,-90);
		zyk_create_info_player_deathmatch(-1508,272,729,-90);
	}
	else if (Q_stricmp(zyk_mapname, "academy5") == 0)
	{
		zyk_create_info_player_deathmatch(-1308,272,729,-90);
		zyk_create_info_player_deathmatch(-1508,272,729,-90);
	}
	else if (Q_stricmp(zyk_mapname, "academy6") == 0)
	{
		zyk_create_info_player_deathmatch(-1308,272,729,-90);
		zyk_create_info_player_deathmatch(-1508,272,729,-90);

		// zyk: hangar spawn points
		zyk_create_info_player_deathmatch(-23,458,-486,0);
		zyk_create_info_player_deathmatch(2053,3401,-486,-90);
		zyk_create_info_player_deathmatch(4870,455,-486,-179);
	}
	else if (Q_stricmp(zyk_mapname, "yavin1") == 0)
	{
		int i = 0;
		gentity_t *ent;

		for (i = 0; i < level.num_entities; i++)
		{
			ent = &g_entities[i];
			if (Q_stricmp( ent->targetname, "end_level") == 0)
			{ // zyk: remove the map change entity
				G_FreeEntity( ent );
			}
		}
		zyk_create_info_player_deathmatch(472,-4833,437,74);
		zyk_create_info_player_deathmatch(-167,-4046,480,0);
	}
	else if (Q_stricmp(zyk_mapname, "yavin1b") == 0)
	{
		int i = 0;
		gentity_t *ent;

		level.quest_map = 1;

		for (i = 0; i < level.num_entities; i++)
		{
			ent = &g_entities[i];
			if (Q_stricmp( ent->targetname, "door1") == 0)
			{
				fix_sp_func_door(ent);
			}
			else if (Q_stricmp( ent->classname, "trigger_hurt") == 0 && Q_stricmp( ent->targetname, "tree_hurt_trigger") != 0)
			{ // zyk: trigger_hurt entity of the bridge area
				G_FreeEntity( ent );
			}
		}
		zyk_create_info_player_deathmatch(472,-4833,437,74);
		zyk_create_info_player_deathmatch(-167,-4046,480,0);
	}
	else if (Q_stricmp(zyk_mapname, "yavin2") == 0)
	{
		int i = 0;
		gentity_t *ent;

		level.quest_map = 10;

		for (i = 0; i < level.num_entities; i++)
		{
			ent = &g_entities[i];
			if (Q_stricmp( ent->targetname, "t530") == 0 || Q_stricmp( ent->targetname, "Putz_door") == 0 || Q_stricmp( ent->targetname, "afterdroid_door") == 0 || Q_stricmp( ent->targetname, "pit_door") == 0 || Q_stricmp( ent->targetname, "door1") == 0)
			{
				fix_sp_func_door(ent);
			}
			else if (Q_stricmp( ent->classname, "trigger_hurt") == 0 && ent->spawnflags == 62)
			{ // zyk: removes the trigger hurt entity of the second bridge
				G_FreeEntity( ent );
			}
		}
		zyk_create_info_player_deathmatch(2516,-5593,89,-179);
		zyk_create_info_player_deathmatch(2516,-5443,89,-179);
	}
	else if (Q_stricmp(zyk_mapname, "hoth2") == 0)
	{
		int i = 0;
		gentity_t *ent;

		level.quest_map = 5;

		for (i = 0; i < level.num_entities; i++)
		{
			ent = &g_entities[i];
			if (Q_stricmp( ent->targetname, "end_level") == 0)
			{ // zyk: remove the map change entity
				G_FreeEntity( ent );
			}
		}

		zyk_create_info_player_deathmatch(-2114,10195,1027,-14);
		zyk_create_info_player_deathmatch(-1808,9640,982,-17);
	}
	else if (Q_stricmp(zyk_mapname, "hoth3") == 0)
	{
		int i = 0;
		gentity_t *ent;

		level.quest_map = 20;

		for (i = 0; i < level.num_entities; i++)
		{
			ent = &g_entities[i];
			if (Q_stricmp( ent->targetname, "end_level") == 0)
			{ // zyk: remove the map change entity
				G_FreeEntity( ent );
			}
		}

		zyk_create_info_player_deathmatch(-1908,562,992,-90);
		zyk_create_info_player_deathmatch(-1907,356,801,-90);
	}
	else if (Q_stricmp(zyk_mapname, "t1_danger") == 0)
	{
		int i = 0;
		gentity_t *ent;

		level.quest_map = 18;

		for (i = 0; i < level.num_entities; i++)
		{
			ent = &g_entities[i];
			if (Q_stricmp( ent->classname, "NPC_Monster_Sand_Creature") == 0)
			{ // zyk: remove the map change entity
				G_FreeEntity( ent );
			}
		}

		zyk_create_info_player_deathmatch(-3705,-3362,1121,90);
		zyk_create_info_player_deathmatch(-3705,-2993,1121,90);
	}
	else if (Q_stricmp(zyk_mapname, "t1_fatal") == 0)
	{
		int i = 0;
		gentity_t *ent;

		level.quest_map = 13;

		for (i = 0; i < level.num_entities; i++)
		{
			ent = &g_entities[i];
			if (Q_stricmp( ent->targetname, "door_trap") == 0)
			{
				fix_sp_func_door(ent);
			}
			if (Q_stricmp( ent->targetname, "lobbydoor1") == 0)
			{
				fix_sp_func_door(ent);
			}
			if (Q_stricmp( ent->targetname, "lobbydoor2") == 0)
			{
				fix_sp_func_door(ent);
			}
			if (Q_stricmp( ent->targetname, "t7708018") == 0)
			{
				fix_sp_func_door(ent);
			}
			if (Q_stricmp( ent->targetname, "t7708017") == 0)
			{
				fix_sp_func_door(ent);
			}
			if (Q_stricmp( ent->classname, "trigger_hurt") == 0 && i > 300)
			{ // zyk: trigger_hurt at the spawn area
				G_FreeEntity( ent );
			}
		}
		zyk_create_info_player_deathmatch(-1563,-4241,4569,-157);
		zyk_create_info_player_deathmatch(-1135,-4303,4569,179);

		if (level.gametype == GT_CTF)
		{ // zyk: in CTF, add the team player spawns
			zyk_create_ctf_player_spawn(-3083,-2683,4696,-90,qtrue,qtrue);
			zyk_create_ctf_player_spawn(-2371,-3325,4536,90,qtrue,qtrue);
			zyk_create_ctf_player_spawn(-1726,-2957,4536,90,qtrue,qtrue);

			zyk_create_ctf_player_spawn(1277,2947,4540,-45,qfalse,qtrue);
			zyk_create_ctf_player_spawn(3740,482,4536,135,qfalse,qtrue);
			zyk_create_ctf_player_spawn(2489,1451,4536,135,qfalse,qtrue);

			zyk_create_ctf_player_spawn(-3083,-2683,4696,-90,qtrue,qfalse);
			zyk_create_ctf_player_spawn(-2371,-3325,4536,90,qtrue,qfalse);
			zyk_create_ctf_player_spawn(-1726,-2957,4536,90,qtrue,qfalse);

			zyk_create_ctf_player_spawn(1277,2947,4540,-45,qfalse,qfalse);
			zyk_create_ctf_player_spawn(3740,482,4536,135,qfalse,qfalse);
			zyk_create_ctf_player_spawn(2489,1451,4536,135,qfalse,qfalse);
		}
	}
	else if (Q_stricmp(zyk_mapname, "t1_inter") == 0)
	{
		zyk_create_info_player_deathmatch(-65,-686,89,90);
		zyk_create_info_player_deathmatch(56,-686,89,90);
	}
	else if (Q_stricmp(zyk_mapname, "t1_rail") == 0)
	{
		zyk_create_info_player_deathmatch(-3135,1,33,0);
		zyk_create_info_player_deathmatch(-3135,197,25,0);

		if (level.gametype == GT_CTF)
		{ // zyk: in CTF, add the team player spawns
			zyk_create_ctf_player_spawn(-2569,-2,25,179,qtrue,qtrue);
			zyk_create_ctf_player_spawn(-1632,257,136,-90,qtrue,qtrue);
			zyk_create_ctf_player_spawn(-1743,0,500,0,qtrue,qtrue);

			zyk_create_ctf_player_spawn(22760,-128,152,90,qfalse,qtrue);
			zyk_create_ctf_player_spawn(22866,0,440,-179,qfalse,qtrue);
			zyk_create_ctf_player_spawn(21102,2,464,179,qfalse,qtrue);

			zyk_create_ctf_player_spawn(-2569,-2,25,179,qtrue,qfalse);
			zyk_create_ctf_player_spawn(-1632,257,136,-90,qtrue,qfalse);
			zyk_create_ctf_player_spawn(-1743,0,500,0,qtrue,qfalse);

			zyk_create_ctf_player_spawn(22760,-128,152,90,qfalse,qfalse);
			zyk_create_ctf_player_spawn(22866,0,440,-179,qfalse,qfalse);
			zyk_create_ctf_player_spawn(21102,2,464,179,qfalse,qfalse);
		}
	}
	else if (Q_stricmp(zyk_mapname, "t1_sour") == 0)
	{
		level.quest_map = 2;

		zyk_create_info_player_deathmatch(9828,-5521,153,90);
		zyk_create_info_player_deathmatch(9845,-5262,153,153);
	}
	else if (Q_stricmp(zyk_mapname, "t1_surprise") == 0)
	{
		int i = 0;
		gentity_t *ent;
		qboolean found_bugged_switch = qfalse;

		level.quest_map = 3;

		for (i = 0; i < level.num_entities; i++)
		{
			ent = &g_entities[i];

			if (Q_stricmp( ent->targetname, "fire_hurt") == 0)
			{
				G_FreeEntity( ent );
			}
			if (Q_stricmp( ent->targetname, "droid_door") == 0)
			{
				fix_sp_func_door(ent);
			}
			if (Q_stricmp( ent->targetname, "tube_door") == 0)
			{
				fix_sp_func_door(ent);
			}
			if (found_bugged_switch == qfalse && Q_stricmp( ent->classname, "misc_model_breakable") == 0 && Q_stricmp( ent->model, "models/map_objects/desert/switch3.md3") == 0)
			{
				G_FreeEntity(ent);
				found_bugged_switch = qtrue;
			}
			if (Q_stricmp( ent->classname, "func_static") == 0 && (int)ent->s.origin[0] == 3064 && (int)ent->s.origin[1] == 5040 && (int)ent->s.origin[2] == 892)
			{ // zyk: elevator inside sand crawler near the wall fire
				G_FreeEntity( ent );
			}
			if (Q_stricmp( ent->classname, "func_door") == 0 && i > 200 && Q_stricmp( ent->model, "*63") == 0)
			{ // zyk: tube door in which the droid goes in SP
				G_FreeEntity( ent );
			}
		}
		zyk_create_info_player_deathmatch(1913,-6151,222,153);
		zyk_create_info_player_deathmatch(1921,-5812,222,-179);

		if (level.gametype == GT_CTF)
		{ // zyk: in CTF, add the team player spawns
			zyk_create_ctf_player_spawn(1948,-6020,222,138,qtrue,qtrue);
			zyk_create_ctf_player_spawn(1994,-4597,908,19,qtrue,qtrue);
			zyk_create_ctf_player_spawn(404,-4521,249,-21,qtrue,qtrue);

			zyk_create_ctf_player_spawn(2341,4599,1056,83,qfalse,qtrue);
			zyk_create_ctf_player_spawn(1901,5425,916,-177,qfalse,qtrue);
			zyk_create_ctf_player_spawn(918,3856,944,0,qfalse,qtrue);

			zyk_create_ctf_player_spawn(1948,-6020,222,138,qtrue,qfalse);
			zyk_create_ctf_player_spawn(1994,-4597,908,19,qtrue,qfalse);
			zyk_create_ctf_player_spawn(404,-4521,249,-21,qtrue,qfalse);

			zyk_create_ctf_player_spawn(2341,4599,1056,83,qfalse,qfalse);
			zyk_create_ctf_player_spawn(1901,5425,916,-177,qfalse,qfalse);
			zyk_create_ctf_player_spawn(918,3856,944,0,qfalse,qfalse);
		}
	}
	else if (Q_stricmp(zyk_mapname, "t2_rancor") == 0)
	{
		int i = 0;
		gentity_t *ent;

		for (i = 0; i < level.num_entities; i++)
		{
			ent = &g_entities[i];
			
			if (Q_stricmp( ent->targetname, "t857") == 0)
			{
				fix_sp_func_door(ent);
			}
			if (Q_stricmp( ent->targetname, "Kill_Brush_Canyon") == 0)
			{ // zyk: trigger_hurt at the spawn area
				G_FreeEntity( ent );
			}
		}
		zyk_create_info_player_deathmatch(-898,1178,1718,90);
		zyk_create_info_player_deathmatch(-898,1032,1718,90);
	}
	else if (Q_stricmp(zyk_mapname, "t2_rogue") == 0)
	{
		int i = 0;
		gentity_t *ent;

		level.quest_map = 7;

		for (i = 0; i < level.num_entities; i++)
		{
			ent = &g_entities[i];
			if (Q_stricmp( ent->targetname, "t475") == 0)
			{ // zyk: remove the invisible wall at the end of the bridge at start
				G_FreeEntity( ent );
			}
			if (Q_stricmp( ent->target, "field_counter1") == 0)
			{
				G_FreeEntity( ent );
			}
			if (Q_stricmp( ent->target, "field_counter2") == 0)
			{
				G_FreeEntity( ent );
			}
			if (Q_stricmp( ent->target, "field_counter3") == 0)
			{
				G_FreeEntity( ent );
			}
			if (Q_stricmp( ent->targetname, "end_level") == 0)
			{ // zyk: remove the map change entity
				G_FreeEntity( ent );
			}
		}

		zyk_create_info_player_deathmatch(1974,-1983,-550,90);
		zyk_create_info_player_deathmatch(1779,-1983,-550,90);
	}
	else if (Q_stricmp(zyk_mapname, "t2_trip") == 0)
	{
		int i = 0;
		gentity_t *ent;

		level.quest_map = 17;

		for (i = 0; i < level.num_entities; i++)
		{
			ent = &g_entities[i];
			if (Q_stricmp( ent->targetname, "t546") == 0)
			{
				G_FreeEntity( ent );
			}
			else if (Q_stricmp( ent->targetname, "end_level") == 0)
			{
				G_FreeEntity( ent );
			}
			else if (Q_stricmp( ent->targetname, "cin_door") == 0)
			{
				G_FreeEntity( ent );
			}
			else if (Q_stricmp( ent->targetname, "endJaden") == 0)
			{
				G_FreeEntity( ent );
			}
			else if (Q_stricmp( ent->targetname, "endJaden2") == 0)
			{
				G_FreeEntity( ent );
			}
			else if (Q_stricmp( ent->targetname, "endswoop") == 0)
			{
				G_FreeEntity( ent );
			}
			else if (Q_stricmp( ent->classname, "func_door") == 0 && i > 200)
			{ // zyk: door after the teleports of the race mode
				G_FreeEntity( ent );
			}
			else if (Q_stricmp( ent->targetname, "t547") == 0)
			{ // zyk: removes swoop at end of map. Must be removed to prevent bug in racemode
				G_FreeEntity( ent );
			}
		}
		zyk_create_info_player_deathmatch(-5698,-22304,1705,90);
		zyk_create_info_player_deathmatch(-5433,-22328,1705,90);

		if (level.gametype == GT_CTF)
		{ // zyk: in CTF, add the team player spawns
			zyk_create_ctf_player_spawn(-20705,18794,1704,0,qtrue,qtrue);
			zyk_create_ctf_player_spawn(-20729,17692,1704,0,qtrue,qtrue);
			zyk_create_ctf_player_spawn(-20204,18736,1503,0,qtrue,qtrue);

			zyk_create_ctf_player_spawn(20494,-2922,1672,90,qfalse,qtrue);
			zyk_create_ctf_player_spawn(19321,-2910,1672,90,qfalse,qtrue);
			zyk_create_ctf_player_spawn(19428,-2404,1470,90,qfalse,qtrue);

			zyk_create_ctf_player_spawn(-20705,18794,1704,0,qtrue,qfalse);
			zyk_create_ctf_player_spawn(-20729,17692,1704,0,qtrue,qfalse);
			zyk_create_ctf_player_spawn(-20204,18736,1503,0,qtrue,qfalse);

			zyk_create_ctf_player_spawn(20494,-2922,1672,90,qfalse,qfalse);
			zyk_create_ctf_player_spawn(19321,-2910,1672,90,qfalse,qfalse);
			zyk_create_ctf_player_spawn(19428,-2404,1470,90,qfalse,qfalse);
		}
	}
	else if (Q_stricmp(zyk_mapname, "t2_wedge") == 0)
	{
		zyk_create_info_player_deathmatch(6328,539,-110,-178);
		zyk_create_info_player_deathmatch(6332,743,-110,-178);
	}
	else if (Q_stricmp(zyk_mapname, "t2_dpred") == 0)
	{
		int i = 0;
		gentity_t *ent;

		for (i = 0; i < level.num_entities; i++)
		{
			ent = &g_entities[i];
			if (Q_stricmp( ent->targetname, "prisonshield1") == 0)
			{
				G_FreeEntity( ent );
			}
			if (Q_stricmp( ent->targetname, "t556") == 0)
			{
				fix_sp_func_door(ent);
			}
			if (Q_stricmp( ent->target, "field_counter1") == 0)
			{
				G_FreeEntity( ent );
			}
			if (Q_stricmp( ent->target, "t62241") == 0)
			{
				G_FreeEntity( ent );
			}
			if (Q_stricmp( ent->target, "t62243") == 0)
			{
				G_FreeEntity( ent );
			}
		}

		zyk_create_info_player_deathmatch(-2152,-3885,-134,90);
		zyk_create_info_player_deathmatch(-2152,-3944,-134,90);

		if (level.gametype == GT_CTF)
		{ // zyk: in CTF, add the team player spawns
			zyk_create_ctf_player_spawn(0,-4640,664,90,qtrue,qtrue);
			zyk_create_ctf_player_spawn(485,-3721,632,-179,qtrue,qtrue);
			zyk_create_ctf_player_spawn(-212,-3325,656,-179,qtrue,qtrue);

			zyk_create_ctf_player_spawn(0,125,24,-90,qfalse,qtrue);
			zyk_create_ctf_player_spawn(-1242,128,24,0,qfalse,qtrue);
			zyk_create_ctf_player_spawn(369,67,296,-179,qfalse,qtrue);

			zyk_create_ctf_player_spawn(0,-4640,664,90,qtrue,qfalse);
			zyk_create_ctf_player_spawn(485,-3721,632,-179,qtrue,qfalse);
			zyk_create_ctf_player_spawn(-212,-3325,656,-179,qtrue,qfalse);

			zyk_create_ctf_player_spawn(0,125,24,-90,qfalse,qfalse);
			zyk_create_ctf_player_spawn(-1242,128,24,0,qfalse,qfalse);
			zyk_create_ctf_player_spawn(369,67,296,-179,qfalse,qfalse);
		}
	}
	else if (Q_stricmp(zyk_mapname, "vjun1") == 0)
	{
		int i = 0;
		gentity_t *ent;
		for (i = 0; i < level.num_entities; i++)
		{
			ent = &g_entities[i];
			if (i == 123 || i == 124)
			{ // zyk: removing tie fighter misc_model_breakable entities to prevent client crashes
				G_FreeEntity( ent );
			}
		}
		zyk_create_info_player_deathmatch(-6897,7035,857,-90);
		zyk_create_info_player_deathmatch(-7271,7034,857,-90);
	}
	else if (Q_stricmp(zyk_mapname, "vjun2") == 0)
	{
		zyk_create_info_player_deathmatch(-831,166,217,90);
		zyk_create_info_player_deathmatch(-700,166,217,90);
	}
	else if (Q_stricmp(zyk_mapname, "vjun3") == 0)
	{
		int i = 0;
		gentity_t *ent;

		for (i = 0; i < level.num_entities; i++)
		{
			ent = &g_entities[i];
			if (Q_stricmp( ent->targetname, "end_level") == 0)
			{
				G_FreeEntity( ent );
			}
		}

		zyk_create_info_player_deathmatch(-8272,-391,1433,179);
		zyk_create_info_player_deathmatch(-8375,-722,1433,179);
	}
	else if (Q_stricmp(zyk_mapname, "t3_hevil") == 0)
	{
		int i = 0;
		gentity_t *ent;

		level.quest_map = 8;

		for (i = 0; i < level.num_entities; i++)
		{
			ent = &g_entities[i];
			if (i == 42)
			{
				G_FreeEntity( ent );
			}
		}
		zyk_create_info_player_deathmatch(512,-2747,-742,90);
		zyk_create_info_player_deathmatch(872,-2445,-742,108);
	}
	else if (Q_stricmp(zyk_mapname, "t3_bounty") == 0)
	{
		level.quest_map = 6;

		zyk_create_info_player_deathmatch(-3721,-726,73,75);
		zyk_create_info_player_deathmatch(-3198,-706,73,90);

		if (level.gametype == GT_CTF)
		{ // zyk: in CTF, add the team player spawns
			zyk_create_ctf_player_spawn(-7740,-543,-263,0,qtrue,qtrue);
			zyk_create_ctf_player_spawn(-8470,-210,24,90,qtrue,qtrue);
			zyk_create_ctf_player_spawn(-7999,-709,-7,132,qtrue,qtrue);

			zyk_create_ctf_player_spawn(616,-978,344,0,qfalse,qtrue);
			zyk_create_ctf_player_spawn(595,482,360,-90,qfalse,qtrue);
			zyk_create_ctf_player_spawn(1242,255,36,-179,qfalse,qtrue);

			zyk_create_ctf_player_spawn(-7740,-543,-263,0,qtrue,qfalse);
			zyk_create_ctf_player_spawn(-8470,-210,24,90,qtrue,qfalse);
			zyk_create_ctf_player_spawn(-7999,-709,-7,132,qtrue,qfalse);

			zyk_create_ctf_player_spawn(616,-978,344,0,qfalse,qfalse);
			zyk_create_ctf_player_spawn(595,482,360,-90,qfalse,qfalse);
			zyk_create_ctf_player_spawn(1242,255,36,-179,qfalse,qfalse);
		}
	}
	else if (Q_stricmp(zyk_mapname, "t3_byss") == 0)
	{
		int i = 0;
		gentity_t *ent;

		for (i = 0; i < level.num_entities; i++)
		{
			ent = &g_entities[i];
			
			if (Q_stricmp( ent->targetname, "wall_door1") == 0)
			{
				fix_sp_func_door(ent);
			}
			if (Q_stricmp( ent->target, "field_counter1") == 0)
			{
				G_FreeEntity( ent );
			}
			if (Q_stricmp( ent->targetname, "wave1_tie1") == 0)
			{
				G_FreeEntity( ent );
			}
			if (Q_stricmp( ent->targetname, "wave1_tie2") == 0)
			{
				G_FreeEntity( ent );
			}
			if (Q_stricmp( ent->targetname, "wave1_tie3") == 0)
			{
				G_FreeEntity( ent );
			}
			if (Q_stricmp( ent->targetname, "wave2_tie1") == 0)
			{
				G_FreeEntity( ent );
			}
			if (Q_stricmp( ent->targetname, "wave2_tie2") == 0)
			{
				G_FreeEntity( ent );
			}
			if (Q_stricmp( ent->targetname, "wave2_tie3") == 0)
			{
				G_FreeEntity( ent );
			}
		}
		zyk_create_info_player_deathmatch(968,111,25,-90);
		zyk_create_info_player_deathmatch(624,563,25,-90);
	}
	else if (Q_stricmp(zyk_mapname, "t3_rift") == 0)
	{
		int i = 0;
		gentity_t *ent;

		level.quest_map = 4;

		for (i = 0; i < level.num_entities; i++)
		{
			ent = &g_entities[i];
			if (Q_stricmp( ent->targetname, "fakewall1") == 0)
			{
				G_FreeEntity( ent );
			}
			if (Q_stricmp( ent->targetname, "t778") == 0)
			{
				G_FreeEntity( ent );
			}
			if (Q_stricmp( ent->targetname, "t779") == 0)
			{
				G_FreeEntity( ent );
			}
		}

		zyk_create_info_player_deathmatch(2195,7611,4380,-90);
		zyk_create_info_player_deathmatch(2305,7640,4380,-90);
	}
	else if (Q_stricmp(zyk_mapname, "t3_stamp") == 0)
	{
		zyk_create_info_player_deathmatch(1208,445,89,179);
		zyk_create_info_player_deathmatch(1208,510,89,179);
	}
	else if (Q_stricmp(zyk_mapname, "kor1") == 0)
	{
		int i = 0;
		gentity_t *ent;

		for (i = 0; i < level.num_entities; i++)
		{
			ent = &g_entities[i];
			if (i >= 418 && i <= 422)
			{ // zyk: remove part of the door on the floor on the first puzzle
				G_FreeEntity( ent );
			}
			if (Q_stricmp( ent->targetname, "end_level") == 0)
			{ // zyk: remove the map change entity
				G_FreeEntity( ent );
			}
			if (Q_stricmp( ent->classname, "func_static") == 0 && (int)ent->s.origin[0] == 1456 && (int)ent->s.origin[1] == -32 && (int)ent->s.origin[2] == -3560)
			{ // zyk: remove the last door entity
				G_FreeEntity( ent );
			}
		}
		zyk_create_info_player_deathmatch(190,632,-1006,-89);
		zyk_create_info_player_deathmatch(-249,952,-934,-89);
	}
	else if (Q_stricmp(zyk_mapname, "kor2") == 0)
	{
		zyk_create_info_player_deathmatch(2977,3137,-2526,0);
		zyk_create_info_player_deathmatch(3072,2992,-2526,0);
	}
	else if (Q_stricmp(zyk_mapname, "taspir1") == 0)
	{
		int i = 0;
		gentity_t *ent;

		level.quest_map = 25;

		for (i = 0; i < level.num_entities; i++)
		{
			ent = &g_entities[i];
			if (Q_stricmp( ent->targetname, "t278") == 0)
			{
				G_FreeEntity( ent );
			}
			if (Q_stricmp( ent->targetname, "bldg2_ext_door") == 0)
			{
				fix_sp_func_door( ent );
			}
			if (Q_stricmp( ent->targetname, "end_level") == 0)
			{
				G_FreeEntity( ent );
			}
		}
		zyk_create_info_player_deathmatch(-1609,-1792,649,112);
		zyk_create_info_player_deathmatch(-1791,-1838,649,90);
	}
	else if (Q_stricmp(zyk_mapname, "taspir2") == 0)
	{
		int i = 0;
		gentity_t *ent;

		for (i = 0; i < level.num_entities; i++)
		{
			ent = &g_entities[i];
			if (Q_stricmp( ent->targetname, "force_field") == 0)
			{
				G_FreeEntity( ent );
			}
			if (Q_stricmp( ent->targetname, "kill_toggle") == 0)
			{
				G_FreeEntity( ent );
			}
		}

		zyk_create_info_player_deathmatch(286,-2859,345,92);
		zyk_create_info_player_deathmatch(190,-2834,345,90);
	}
	else if (Q_stricmp(zyk_mapname, "mp/duel5") == 0 && g_gametype.integer == GT_FFA)
	{
		level.quest_map = 11;
	}
	else if (Q_stricmp(zyk_mapname, "mp/duel6") == 0 && g_gametype.integer == GT_FFA)
	{
		level.quest_map = 9;
	}
	else if (Q_stricmp(zyk_mapname, "mp/duel8") == 0 && g_gametype.integer == GT_FFA)
	{
		level.quest_map = 14;
	}
	else if (Q_stricmp(zyk_mapname, "mp/duel9") == 0 && g_gametype.integer == GT_FFA)
	{
		level.quest_map = 15;
	}
	else if (Q_stricmp(zyk_mapname, "mp/siege_korriban") == 0 && g_gametype.integer == GT_FFA)
	{ // zyk: if its a FFA game, then remove some entities
		int i = 0;
		gentity_t *ent;

		level.quest_map = 12;

		for (i = 0; i < level.num_entities; i++)
		{
			ent = &g_entities[i];
			if (Q_stricmp( ent->targetname, "cyrstalsinplace") == 0)
			{
				G_FreeEntity( ent );
			}
			if (i >= 236 && i <= 238)
			{ // zyk: removing the trigger_hurt from the lava in Guardian of Universe arena
				G_FreeEntity( ent );
			}
		}
	}
	else if (Q_stricmp(zyk_mapname, "mp/siege_desert") == 0 && g_gametype.integer == GT_FFA)
	{ // zyk: if its a FFA game, then remove the shield in the final part
		int i = 0;
		gentity_t *ent;

		level.quest_map = 24;

		for (i = 0; i < level.num_entities; i++)
		{
			ent = &g_entities[i];
			if (Q_stricmp( ent->targetname, "rebel_obj_2_doors") == 0)
			{
				G_FreeEntity( ent );
			}
			if (Q_stricmp( ent->targetname, "shield") == 0)
			{
				G_FreeEntity( ent );
			}
			if (Q_stricmp( ent->targetname, "gatedestroy_doors") == 0)
			{
				G_FreeEntity( ent );
			}
			if (i >= 153 && i <= 160)
			{
				G_FreeEntity( ent );
			}
		}
	}
	else if (Q_stricmp(zyk_mapname, "mp/siege_destroyer") == 0 && g_gametype.integer == GT_FFA)
	{ // zyk: if its a FFA game, then remove the shield at the destroyer
		int i = 0;
		gentity_t *ent;

		for (i = 0; i < level.num_entities; i++)
		{
			ent = &g_entities[i];
			if (Q_stricmp( ent->targetname, "ubershield") == 0)
			{
				G_FreeEntity( ent );
			}
			else if (Q_stricmp( ent->classname, "info_player_deathmatch") == 0)
			{
				G_FreeEntity( ent );
			}
		}
		// zyk: rebel area spawnpoints
		zyk_create_info_player_deathmatch(31729,-32219,33305,90);
		zyk_create_info_player_deathmatch(31229,-32219,33305,90);
		zyk_create_info_player_deathmatch(30729,-32219,33305,90);
		zyk_create_info_player_deathmatch(32229,-32219,33305,90);
		zyk_create_info_player_deathmatch(32729,-32219,33305,90);
		zyk_create_info_player_deathmatch(31729,-32019,33305,90);

		// zyk: imperial area spawnpoints
		zyk_create_info_player_deathmatch(2545,8987,1817,-90);
		zyk_create_info_player_deathmatch(2345,8987,1817,-90);
		zyk_create_info_player_deathmatch(2745,8987,1817,-90);

		zyk_create_info_player_deathmatch(2597,7403,1817,90);
		zyk_create_info_player_deathmatch(2397,7403,1817,90);
		zyk_create_info_player_deathmatch(2797,7403,1817,90);
	}

	level.sp_map = qfalse;

	if (Q_stricmp(level.default_map_music, "") == 0)
	{ // zyk: if the default map music is empty (the map has no music) then set a default music
		if (level.quest_map == 1)
			strcpy(level.default_map_music,"music/yavin1/swamp_explore.mp3");
		else if (level.quest_map == 7)
			strcpy(level.default_map_music,"music/t2_rogue/narshaada_explore.mp3");
		else if (level.quest_map == 10)
			strcpy(level.default_map_music,"music/yavin2/yavtemp2_explore.mp3");
		else if (level.quest_map == 13)
			strcpy(level.default_map_music,"music/t1_fatal/tunnels_explore.mp3");
		else
			strcpy(level.default_map_music,"music/hoth2/hoth2_explore.mp3");
	}

	// zyk: creating directories where the entity files will be loaded from
#if defined(__linux__)
	system(va("mkdir -p entities/%s",zyk_mapname));
#else
	system(va("mkdir \"entities/%s\"",zyk_mapname));
#endif

	// zyk: loading entities set as default (Entity System)
	zyk_entities_file = fopen(va("entities/%s/default.txt",zyk_mapname),"r");

	if (zyk_entities_file != NULL)
	{ // zyk: default file exists. Load entities from it
		fclose(zyk_entities_file);

		// zyk: cleaning entities. Only the ones from the file will be in the map. Do not remove CTF flags
		for (i = (MAX_CLIENTS + BODY_QUEUE_SIZE); i < level.num_entities; i++)
		{
			gentity_t *target_ent = &g_entities[i];

			if (target_ent && Q_stricmp(target_ent->classname, "team_CTF_redflag") != 0 && Q_stricmp(target_ent->classname, "team_CTF_blueflag") != 0)
				G_FreeEntity( target_ent );
		}

		strcpy(level.load_entities_file, va("entities/%s/default.txt",zyk_mapname));

		level.load_entities_timer = level.time + 1050;
	}

	// zyk: loading default remaps
	zyk_remap_file = fopen(va("remaps/%s/default.txt",zyk_mapname),"r");

	if (zyk_remap_file != NULL)
	{
		char old_shader[128];
		char new_shader[128];
		char time_offset[128];

		strcpy(old_shader,"");
		strcpy(new_shader,"");
		strcpy(time_offset,"");

		while(fscanf(zyk_remap_file,"%s",old_shader) != EOF)
		{
			fscanf(zyk_remap_file,"%s",new_shader);
			fscanf(zyk_remap_file,"%s",time_offset);

			AddRemap(G_NewString(old_shader), G_NewString(new_shader), atof(time_offset));
		}
		
		fclose(zyk_remap_file);

		trap->SetConfigstring(CS_SHADERSTATE, BuildShaderStateConfig());
	}
}



/*
=================
G_ShutdownGame
=================
*/
void G_ShutdownGame( int restart ) {
	int i = 0;
	gentity_t *ent;

//	trap->Print ("==== ShutdownGame ====\n");

	G_CleanAllFakeClients(); //get rid of dynamically allocated fake client structs.

	BG_ClearAnimsets(); //free all dynamic allocations made through the engine

//	Com_Printf("... Gameside GHOUL2 Cleanup\n");
	while (i < MAX_GENTITIES)
	{ //clean up all the ghoul2 instances
		ent = &g_entities[i];

		if (ent->ghoul2 && trap->G2API_HaveWeGhoul2Models(ent->ghoul2))
		{
			trap->G2API_CleanGhoul2Models(&ent->ghoul2);
			ent->ghoul2 = NULL;
		}
		if (ent->client)
		{
			int j = 0;

			while (j < MAX_SABERS)
			{
				if (ent->client->weaponGhoul2[j] && trap->G2API_HaveWeGhoul2Models(ent->client->weaponGhoul2[j]))
				{
					trap->G2API_CleanGhoul2Models(&ent->client->weaponGhoul2[j]);
				}
				j++;
			}
		}
		i++;
	}
	if (g2SaberInstance && trap->G2API_HaveWeGhoul2Models(g2SaberInstance))
	{
		trap->G2API_CleanGhoul2Models(&g2SaberInstance);
		g2SaberInstance = NULL;
	}
	if (precachedKyle && trap->G2API_HaveWeGhoul2Models(precachedKyle))
	{
		trap->G2API_CleanGhoul2Models(&precachedKyle);
		precachedKyle = NULL;
	}

//	Com_Printf ("... ICARUS_Shutdown\n");
	trap->ICARUS_Shutdown ();	//Shut ICARUS down

//	Com_Printf ("... Reference Tags Cleared\n");
	TAG_Init();	//Clear the reference tags

	G_LogWeaponOutput();

	if ( level.logFile ) {
		G_LogPrintf( "ShutdownGame:\n------------------------------------------------------------\n" );
		trap->FS_Close( level.logFile );
		level.logFile = 0;
	}

	if ( level.security.log )
	{
		G_SecurityLogPrintf( "ShutdownGame\n\n" );
		trap->FS_Close( level.security.log );
		level.security.log = 0;
	}

	// write all the client session data so we can get it back
	G_WriteSessionData();

	trap->ROFF_Clean();

	if ( trap->Cvar_VariableIntegerValue( "bot_enable" ) ) {
		BotAIShutdown( restart );
	}

	B_CleanupAlloc(); //clean up all allocations made with B_Alloc
}

/*
========================================================================

PLAYER COUNTING / SCORE SORTING

========================================================================
*/

/*
=============
AddTournamentPlayer

If there are less than two tournament players, put a
spectator in the game and restart
=============
*/
void AddTournamentPlayer( void ) {
	int			i;
	gclient_t	*client;
	gclient_t	*nextInLine;

	if ( level.numPlayingClients >= 2 ) {
		return;
	}

	// never change during intermission
//	if ( level.intermissiontime ) {
//		return;
//	}

	nextInLine = NULL;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		client = &level.clients[i];
		if ( client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if (!g_allowHighPingDuelist.integer && client->ps.ping >= 999)
		{ //don't add people who are lagging out if cvar is not set to allow it.
			continue;
		}
		if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
			continue;
		}
		// never select the dedicated follow or scoreboard clients
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD ||
			client->sess.spectatorClient < 0  ) {
			continue;
		}

		if ( !nextInLine || client->sess.spectatorNum > nextInLine->sess.spectatorNum )
			nextInLine = client;
	}

	if ( !nextInLine ) {
		return;
	}

	level.warmupTime = -1;

	// set them to free-for-all team
	SetTeam( &g_entities[ nextInLine - level.clients ], "f" );
}

/*
=======================
AddTournamentQueue

Add client to end of tournament queue
=======================
*/

void AddTournamentQueue( gclient_t *client )
{
	int index;
	gclient_t *curclient;

	for( index = 0; index < level.maxclients; index++ )
	{
		curclient = &level.clients[index];

		if ( curclient->pers.connected != CON_DISCONNECTED )
		{
			if ( curclient == client )
				curclient->sess.spectatorNum = 0;
			else if ( curclient->sess.sessionTeam == TEAM_SPECTATOR )
				curclient->sess.spectatorNum++;
		}
	}
}

/*
=======================
RemoveTournamentLoser

Make the loser a spectator at the back of the line
=======================
*/
void RemoveTournamentLoser( void ) {
	int			clientNum;

	if ( level.numPlayingClients != 2 ) {
		return;
	}

	clientNum = level.sortedClients[1];

	if ( level.clients[ clientNum ].pers.connected != CON_CONNECTED ) {
		return;
	}

	// make them a spectator
	SetTeam( &g_entities[ clientNum ], "s" );
}

void G_PowerDuelCount(int *loners, int *doubles, qboolean countSpec)
{
	int i = 0;
	gclient_t *cl;

	while (i < MAX_CLIENTS)
	{
		cl = g_entities[i].client;

		if (g_entities[i].inuse && cl && (countSpec || cl->sess.sessionTeam != TEAM_SPECTATOR))
		{
			if (cl->sess.duelTeam == DUELTEAM_LONE)
			{
				(*loners)++;
			}
			else if (cl->sess.duelTeam == DUELTEAM_DOUBLE)
			{
				(*doubles)++;
			}
		}
		i++;
	}
}

qboolean g_duelAssigning = qfalse;
void AddPowerDuelPlayers( void )
{
	int			i;
	int			loners = 0;
	int			doubles = 0;
	int			nonspecLoners = 0;
	int			nonspecDoubles = 0;
	gclient_t	*client;
	gclient_t	*nextInLine;

	if ( level.numPlayingClients >= 3 )
	{
		return;
	}

	nextInLine = NULL;

	G_PowerDuelCount(&nonspecLoners, &nonspecDoubles, qfalse);
	if (nonspecLoners >= 1 && nonspecDoubles >= 2)
	{ //we have enough people, stop
		return;
	}

	//Could be written faster, but it's not enough to care I suppose.
	G_PowerDuelCount(&loners, &doubles, qtrue);

	if (loners < 1 || doubles < 2)
	{ //don't bother trying to spawn anyone yet if the balance is not even set up between spectators
		return;
	}

	//Count again, with only in-game clients in mind.
	loners = nonspecLoners;
	doubles = nonspecDoubles;
//	G_PowerDuelCount(&loners, &doubles, qfalse);

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		client = &level.clients[i];
		if ( client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
			continue;
		}
		if (client->sess.duelTeam == DUELTEAM_FREE)
		{
			continue;
		}
		if (client->sess.duelTeam == DUELTEAM_LONE && loners >= 1)
		{
			continue;
		}
		if (client->sess.duelTeam == DUELTEAM_DOUBLE && doubles >= 2)
		{
			continue;
		}

		// never select the dedicated follow or scoreboard clients
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD ||
			client->sess.spectatorClient < 0  ) {
			continue;
		}

		if ( !nextInLine || client->sess.spectatorNum > nextInLine->sess.spectatorNum )
			nextInLine = client;
	}

	if ( !nextInLine ) {
		return;
	}

	level.warmupTime = -1;

	// set them to free-for-all team
	SetTeam( &g_entities[ nextInLine - level.clients ], "f" );

	//Call recursively until everyone is in
	AddPowerDuelPlayers();
}

qboolean g_dontFrickinCheck = qfalse;

void RemovePowerDuelLosers(void)
{
	int remClients[3];
	int remNum = 0;
	int i = 0;
	gclient_t *cl;

	while (i < MAX_CLIENTS && remNum < 3)
	{
		//cl = &level.clients[level.sortedClients[i]];
		cl = &level.clients[i];

		if (cl->pers.connected == CON_CONNECTED)
		{
			if ((cl->ps.stats[STAT_HEALTH] <= 0 || cl->iAmALoser) &&
				(cl->sess.sessionTeam != TEAM_SPECTATOR || cl->iAmALoser))
			{ //he was dead or he was spectating as a loser
                remClients[remNum] = i;
				remNum++;
			}
		}

		i++;
	}

	if (!remNum)
	{ //Time ran out or something? Oh well, just remove the main guy.
		remClients[remNum] = level.sortedClients[0];
		remNum++;
	}

	i = 0;
	while (i < remNum)
	{ //set them all to spectator
		SetTeam( &g_entities[ remClients[i] ], "s" );
		i++;
	}

	g_dontFrickinCheck = qfalse;

	//recalculate stuff now that we have reset teams.
	CalculateRanks();
}

void RemoveDuelDrawLoser(void)
{
	int clFirst = 0;
	int clSec = 0;
	int clFailure = 0;

	if ( level.clients[ level.sortedClients[0] ].pers.connected != CON_CONNECTED )
	{
		return;
	}
	if ( level.clients[ level.sortedClients[1] ].pers.connected != CON_CONNECTED )
	{
		return;
	}

	clFirst = level.clients[ level.sortedClients[0] ].ps.stats[STAT_HEALTH] + level.clients[ level.sortedClients[0] ].ps.stats[STAT_ARMOR];
	clSec = level.clients[ level.sortedClients[1] ].ps.stats[STAT_HEALTH] + level.clients[ level.sortedClients[1] ].ps.stats[STAT_ARMOR];

	if (clFirst > clSec)
	{
		clFailure = 1;
	}
	else if (clSec > clFirst)
	{
		clFailure = 0;
	}
	else
	{
		clFailure = 2;
	}

	if (clFailure != 2)
	{
		SetTeam( &g_entities[ level.sortedClients[clFailure] ], "s" );
	}
	else
	{ //we could be more elegant about this, but oh well.
		SetTeam( &g_entities[ level.sortedClients[1] ], "s" );
	}
}

/*
=======================
RemoveTournamentWinner
=======================
*/
void RemoveTournamentWinner( void ) {
	int			clientNum;

	if ( level.numPlayingClients != 2 ) {
		return;
	}

	clientNum = level.sortedClients[0];

	if ( level.clients[ clientNum ].pers.connected != CON_CONNECTED ) {
		return;
	}

	// make them a spectator
	SetTeam( &g_entities[ clientNum ], "s" );
}

/*
=======================
AdjustTournamentScores
=======================
*/
void AdjustTournamentScores( void ) {
	int			clientNum;

	if (level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE] ==
		level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE] &&
		level.clients[level.sortedClients[0]].pers.connected == CON_CONNECTED &&
		level.clients[level.sortedClients[1]].pers.connected == CON_CONNECTED)
	{
		int clFirst = level.clients[ level.sortedClients[0] ].ps.stats[STAT_HEALTH] + level.clients[ level.sortedClients[0] ].ps.stats[STAT_ARMOR];
		int clSec = level.clients[ level.sortedClients[1] ].ps.stats[STAT_HEALTH] + level.clients[ level.sortedClients[1] ].ps.stats[STAT_ARMOR];
		int clFailure = 0;
		int clSuccess = 0;

		if (clFirst > clSec)
		{
			clFailure = 1;
			clSuccess = 0;
		}
		else if (clSec > clFirst)
		{
			clFailure = 0;
			clSuccess = 1;
		}
		else
		{
			clFailure = 2;
			clSuccess = 2;
		}

		if (clFailure != 2)
		{
			clientNum = level.sortedClients[clSuccess];

			level.clients[ clientNum ].sess.wins++;
			ClientUserinfoChanged( clientNum );
			trap->SetConfigstring ( CS_CLIENT_DUELWINNER, va("%i", clientNum ) );

			clientNum = level.sortedClients[clFailure];

			level.clients[ clientNum ].sess.losses++;
			ClientUserinfoChanged( clientNum );
		}
		else
		{
			clSuccess = 0;
			clFailure = 1;

			clientNum = level.sortedClients[clSuccess];

			level.clients[ clientNum ].sess.wins++;
			ClientUserinfoChanged( clientNum );
			trap->SetConfigstring ( CS_CLIENT_DUELWINNER, va("%i", clientNum ) );

			clientNum = level.sortedClients[clFailure];

			level.clients[ clientNum ].sess.losses++;
			ClientUserinfoChanged( clientNum );
		}
	}
	else
	{
		clientNum = level.sortedClients[0];
		if ( level.clients[ clientNum ].pers.connected == CON_CONNECTED ) {
			level.clients[ clientNum ].sess.wins++;
			ClientUserinfoChanged( clientNum );

			trap->SetConfigstring ( CS_CLIENT_DUELWINNER, va("%i", clientNum ) );
		}

		clientNum = level.sortedClients[1];
		if ( level.clients[ clientNum ].pers.connected == CON_CONNECTED ) {
			level.clients[ clientNum ].sess.losses++;
			ClientUserinfoChanged( clientNum );
		}
	}
}

/*
=============
SortRanks

=============
*/
int QDECL SortRanks( const void *a, const void *b ) {
	gclient_t	*ca, *cb;

	ca = &level.clients[*(int *)a];
	cb = &level.clients[*(int *)b];

	if (level.gametype == GT_POWERDUEL)
	{
		//sort single duelists first
		if (ca->sess.duelTeam == DUELTEAM_LONE && ca->sess.sessionTeam != TEAM_SPECTATOR)
		{
			return -1;
		}
		if (cb->sess.duelTeam == DUELTEAM_LONE && cb->sess.sessionTeam != TEAM_SPECTATOR)
		{
			return 1;
		}

		//others will be auto-sorted below but above spectators.
	}

	// sort special clients last
	if ( ca->sess.spectatorState == SPECTATOR_SCOREBOARD || ca->sess.spectatorClient < 0 ) {
		return 1;
	}
	if ( cb->sess.spectatorState == SPECTATOR_SCOREBOARD || cb->sess.spectatorClient < 0  ) {
		return -1;
	}

	// then connecting clients
	if ( ca->pers.connected == CON_CONNECTING ) {
		return 1;
	}
	if ( cb->pers.connected == CON_CONNECTING ) {
		return -1;
	}


	// then spectators
	if ( ca->sess.sessionTeam == TEAM_SPECTATOR && cb->sess.sessionTeam == TEAM_SPECTATOR ) {
		if ( ca->sess.spectatorNum > cb->sess.spectatorNum ) {
			return -1;
		}
		if ( ca->sess.spectatorNum < cb->sess.spectatorNum ) {
			return 1;
		}
		return 0;
	}
	if ( ca->sess.sessionTeam == TEAM_SPECTATOR ) {
		return 1;
	}
	if ( cb->sess.sessionTeam == TEAM_SPECTATOR ) {
		return -1;
	}

	// then sort by score
	if ( ca->ps.persistant[PERS_SCORE]
		> cb->ps.persistant[PERS_SCORE] ) {
		return -1;
	}
	if ( ca->ps.persistant[PERS_SCORE]
		< cb->ps.persistant[PERS_SCORE] ) {
		return 1;
	}
	return 0;
}

qboolean gQueueScoreMessage = qfalse;
int gQueueScoreMessageTime = 0;

//A new duel started so respawn everyone and make sure their stats are reset
qboolean G_CanResetDuelists(void)
{
	int i;
	gentity_t *ent;

	i = 0;
	while (i < 3)
	{ //precheck to make sure they are all respawnable
		ent = &g_entities[level.sortedClients[i]];

		if (!ent->inuse || !ent->client || ent->health <= 0 ||
			ent->client->sess.sessionTeam == TEAM_SPECTATOR ||
			ent->client->sess.duelTeam <= DUELTEAM_FREE)
		{
			return qfalse;
		}
		i++;
	}

	return qtrue;
}

qboolean g_noPDuelCheck = qfalse;
void G_ResetDuelists(void)
{
	int i;
	gentity_t *ent = NULL;

	i = 0;
	while (i < 3)
	{
		ent = &g_entities[level.sortedClients[i]];

		g_noPDuelCheck = qtrue;
		player_die(ent, ent, ent, 999, MOD_SUICIDE);
		g_noPDuelCheck = qfalse;
		trap->UnlinkEntity ((sharedEntity_t *)ent);
		ClientSpawn(ent);
		i++;
	}
}

/*
============
CalculateRanks

Recalculates the score ranks of all players
This will be called on every client connect, begin, disconnect, death,
and team change.
============
*/
void CalculateRanks( void ) {
	int		i;
	int		rank;
	int		score;
	int		newScore;
//	int		preNumSpec = 0;
	//int		nonSpecIndex = -1;
	gclient_t	*cl;

//	preNumSpec = level.numNonSpectatorClients;

	level.follow1 = -1;
	level.follow2 = -1;
	level.numConnectedClients = 0;
	level.num_fully_connected_clients = 0;
	level.numNonSpectatorClients = 0;
	level.numPlayingClients = 0;
	level.numVotingClients = 0;		// don't count bots

	for ( i = 0; i < ARRAY_LEN(level.numteamVotingClients); i++ ) {
		level.numteamVotingClients[i] = 0;
	}
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected != CON_DISCONNECTED ) {
			level.sortedClients[level.numConnectedClients] = i;
			level.numConnectedClients++;

			if (level.clients[i].pers.connected == CON_CONNECTED)
			{
				level.num_fully_connected_clients++;
			}

			if ( level.clients[i].sess.sessionTeam != TEAM_SPECTATOR || level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL )
			{
				if (level.clients[i].sess.sessionTeam != TEAM_SPECTATOR)
				{
					level.numNonSpectatorClients++;
					//nonSpecIndex = i;
				}

				// decide if this should be auto-followed
				if ( level.clients[i].pers.connected == CON_CONNECTED )
				{
					if (level.clients[i].sess.sessionTeam != TEAM_SPECTATOR || level.clients[i].iAmALoser)
					{
						level.numPlayingClients++;
					}
					if ( !(g_entities[i].r.svFlags & SVF_BOT) )
					{
						level.numVotingClients++;
						if ( level.clients[i].sess.sessionTeam == TEAM_RED )
							level.numteamVotingClients[0]++;
						else if ( level.clients[i].sess.sessionTeam == TEAM_BLUE )
							level.numteamVotingClients[1]++;
					}
					if ( level.follow1 == -1 ) {
						level.follow1 = i;
					} else if ( level.follow2 == -1 ) {
						level.follow2 = i;
					}
				}
			}
		}
	}

	if ( !g_warmup.integer || level.gametype == GT_SIEGE )
		level.warmupTime = 0;

	/*
	if (level.numNonSpectatorClients == 2 && preNumSpec < 2 && nonSpecIndex != -1 && level.gametype == GT_DUEL && !level.warmupTime)
	{
		gentity_t *currentWinner = G_GetDuelWinner(&level.clients[nonSpecIndex]);

		if (currentWinner && currentWinner->client)
		{
			trap->SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s %s\n\"",
			currentWinner->client->pers.netname, G_GetStringEdString("MP_SVGAME", "VERSUS"), level.clients[nonSpecIndex].pers.netname));
		}
	}
	*/
	//NOTE: for now not doing this either. May use later if appropriate.

	qsort( level.sortedClients, level.numConnectedClients,
		sizeof(level.sortedClients[0]), SortRanks );

	// set the rank value for all clients that are connected and not spectators
	if ( level.gametype >= GT_TEAM ) {
		// in team games, rank is just the order of the teams, 0=red, 1=blue, 2=tied
		for ( i = 0;  i < level.numConnectedClients; i++ ) {
			cl = &level.clients[ level.sortedClients[i] ];
			if ( level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE] ) {
				cl->ps.persistant[PERS_RANK] = 2;
			} else if ( level.teamScores[TEAM_RED] > level.teamScores[TEAM_BLUE] ) {
				cl->ps.persistant[PERS_RANK] = 0;
			} else {
				cl->ps.persistant[PERS_RANK] = 1;
			}
		}
	} else {
		rank = -1;
		score = 0;
		for ( i = 0;  i < level.numPlayingClients; i++ ) {
			cl = &level.clients[ level.sortedClients[i] ];
			newScore = cl->ps.persistant[PERS_SCORE];
			if ( i == 0 || newScore != score ) {
				rank = i;
				// assume we aren't tied until the next client is checked
				level.clients[ level.sortedClients[i] ].ps.persistant[PERS_RANK] = rank;
			} else if(i != 0 ){
				// we are tied with the previous client
				level.clients[ level.sortedClients[i-1] ].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
				level.clients[ level.sortedClients[i] ].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
			}
			score = newScore;
			if ( level.gametype == GT_SINGLE_PLAYER && level.numPlayingClients == 1 ) {
				level.clients[ level.sortedClients[i] ].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
			}
		}
	}

	// set the CS_SCORES1/2 configstrings, which will be visible to everyone
	if ( level.gametype >= GT_TEAM ) {
		trap->SetConfigstring( CS_SCORES1, va("%i", level.teamScores[TEAM_RED] ) );
		trap->SetConfigstring( CS_SCORES2, va("%i", level.teamScores[TEAM_BLUE] ) );
	} else {
		if ( level.numConnectedClients == 0 ) {
			trap->SetConfigstring( CS_SCORES1, va("%i", SCORE_NOT_PRESENT) );
			trap->SetConfigstring( CS_SCORES2, va("%i", SCORE_NOT_PRESENT) );
		} else if ( level.numConnectedClients == 1 ) {
			trap->SetConfigstring( CS_SCORES1, va("%i", level.clients[ level.sortedClients[0] ].ps.persistant[PERS_SCORE] ) );
			trap->SetConfigstring( CS_SCORES2, va("%i", SCORE_NOT_PRESENT) );
		} else {
			trap->SetConfigstring( CS_SCORES1, va("%i", level.clients[ level.sortedClients[0] ].ps.persistant[PERS_SCORE] ) );
			trap->SetConfigstring( CS_SCORES2, va("%i", level.clients[ level.sortedClients[1] ].ps.persistant[PERS_SCORE] ) );
		}

		if (level.gametype != GT_DUEL && level.gametype != GT_POWERDUEL)
		{ //when not in duel, use this configstring to pass the index of the player currently in first place
			if ( level.numConnectedClients >= 1 )
			{
				trap->SetConfigstring ( CS_CLIENT_DUELWINNER, va("%i", level.sortedClients[0] ) );
			}
			else
			{
				trap->SetConfigstring ( CS_CLIENT_DUELWINNER, "-1" );
			}
		}
	}

	// see if it is time to end the level
	CheckExitRules();

	// if we are at the intermission or in multi-frag Duel game mode, send the new info to everyone
	if ( level.intermissiontime || level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL ) {
		gQueueScoreMessage = qtrue;
		gQueueScoreMessageTime = level.time + 500;
		//SendScoreboardMessageToAllClients();
		//rww - Made this operate on a "queue" system because it was causing large overflows
	}
}


/*
========================================================================

MAP CHANGING

========================================================================
*/

/*
========================
SendScoreboardMessageToAllClients

Do this at BeginIntermission time and whenever ranks are recalculated
due to enters/exits/forced team changes
========================
*/
void SendScoreboardMessageToAllClients( void ) {
	int		i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[ i ].pers.connected == CON_CONNECTED ) {
			DeathmatchScoreboardMessage( g_entities + i );
		}
	}
}

/*
========================
MoveClientToIntermission

When the intermission starts, this will be called for all players.
If a new client connects, this will be called after the spawn function.
========================
*/
extern void G_LeaveVehicle( gentity_t *ent, qboolean ConCheck );
void MoveClientToIntermission( gentity_t *ent ) {
	// take out of follow mode if needed
	if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
		StopFollowing( ent );
	}

	FindIntermissionPoint();
	// move to the spot
	VectorCopy( level.intermission_origin, ent->s.origin );
	VectorCopy( level.intermission_origin, ent->client->ps.origin );
	VectorCopy (level.intermission_angle, ent->client->ps.viewangles);
	ent->client->ps.pm_type = PM_INTERMISSION;

	// clean up powerup info
	memset( ent->client->ps.powerups, 0, sizeof(ent->client->ps.powerups) );

	G_LeaveVehicle( ent, qfalse );

	ent->client->ps.rocketLockIndex = ENTITYNUM_NONE;
	ent->client->ps.rocketLockTime = 0;

	ent->client->ps.eFlags = 0;
	ent->s.eFlags = 0;
	ent->client->ps.eFlags2 = 0;
	ent->s.eFlags2 = 0;
	ent->s.eType = ET_GENERAL;
	ent->s.modelindex = 0;
	ent->s.loopSound = 0;
	ent->s.loopIsSoundset = qfalse;
	ent->s.event = 0;
	ent->r.contents = 0;
}

/*
==================
FindIntermissionPoint

This is also used for spectator spawns
==================
*/
extern qboolean	gSiegeRoundBegun;
extern qboolean	gSiegeRoundEnded;
extern int	gSiegeRoundWinningTeam;
void FindIntermissionPoint( void ) {
	gentity_t	*ent = NULL;
	gentity_t	*target;
	vec3_t		dir;

	// find the intermission spot
	if ( level.gametype == GT_SIEGE
		&& level.intermissiontime
		&& level.intermissiontime <= level.time
		&& gSiegeRoundEnded )
	{
	   	if (gSiegeRoundWinningTeam == SIEGETEAM_TEAM1)
		{
			ent = G_Find (NULL, FOFS(classname), "info_player_intermission_red");
			if ( ent && ent->target2 )
			{
				G_UseTargets2( ent, ent, ent->target2 );
			}
		}
	   	else if (gSiegeRoundWinningTeam == SIEGETEAM_TEAM2)
		{
			ent = G_Find (NULL, FOFS(classname), "info_player_intermission_blue");
			if ( ent && ent->target2 )
			{
				G_UseTargets2( ent, ent, ent->target2 );
			}
		}
	}
	if ( !ent )
	{
		ent = G_Find (NULL, FOFS(classname), "info_player_intermission");
	}
	if ( !ent ) {	// the map creator forgot to put in an intermission point...
		SelectSpawnPoint ( vec3_origin, level.intermission_origin, level.intermission_angle, TEAM_SPECTATOR, qfalse );
	} else {
		VectorCopy (ent->s.origin, level.intermission_origin);
		VectorCopy (ent->s.angles, level.intermission_angle);
		// if it has a target, look towards it
		if ( ent->target ) {
			target = G_PickTarget( ent->target );
			if ( target ) {
				VectorSubtract( target->s.origin, level.intermission_origin, dir );
				vectoangles( dir, level.intermission_angle );
			}
		}
	}
}

qboolean DuelLimitHit(void);

/*
==================
BeginIntermission
==================
*/
void BeginIntermission( void ) {
	int			i;
	gentity_t	*client;

	if ( level.intermissiontime ) {
		return;		// already active
	}

	// if in tournament mode, change the wins / losses
	if ( level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL ) {
		trap->SetConfigstring ( CS_CLIENT_DUELWINNER, "-1" );

		if (level.gametype != GT_POWERDUEL)
		{
			AdjustTournamentScores();
		}
		if (DuelLimitHit())
		{
			gDuelExit = qtrue;
		}
		else
		{
			gDuelExit = qfalse;
		}
	}

	level.intermissiontime = level.time;

	// move all clients to the intermission point
	for (i=0 ; i< level.maxclients ; i++) {
		client = g_entities + i;
		if (!client->inuse)
			continue;
		// respawn if dead
		if (client->health <= 0) {
			if (level.gametype != GT_POWERDUEL ||
				!client->client ||
				client->client->sess.sessionTeam != TEAM_SPECTATOR)
			{ //don't respawn spectators in powerduel or it will mess the line order all up
				ClientRespawn(client);
			}
		}
		MoveClientToIntermission( client );
	}

	// send the current scoring to all clients
	SendScoreboardMessageToAllClients();
}

qboolean DuelLimitHit(void)
{
	int i;
	gclient_t *cl;

	for ( i=0 ; i< sv_maxclients.integer ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}

		if ( duel_fraglimit.integer && cl->sess.wins >= duel_fraglimit.integer )
		{
			return qtrue;
		}
	}

	return qfalse;
}

void DuelResetWinsLosses(void)
{
	int i;
	gclient_t *cl;

	for ( i=0 ; i< sv_maxclients.integer ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}

		cl->sess.wins = 0;
		cl->sess.losses = 0;
	}
}

/*
=============
ExitLevel

When the intermission has been exited, the server is either killed
or moved to a new level based on the "nextmap" cvar

=============
*/
extern void SiegeDoTeamAssign(void); //g_saga.c
extern siegePers_t g_siegePersistant; //g_saga.c
void ExitLevel (void) {
	int		i;
	gclient_t *cl;

	// if we are running a tournament map, kick the loser to spectator status,
	// which will automatically grab the next spectator and restart
	if ( level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL ) {
		if (!DuelLimitHit())
		{
			if ( !level.restarted ) {
				trap->SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
				level.restarted = qtrue;
				level.changemap = NULL;
				level.intermissiontime = 0;
			}
			return;
		}

		DuelResetWinsLosses();
	}


	if (level.gametype == GT_SIEGE &&
		g_siegeTeamSwitch.integer &&
		g_siegePersistant.beatingTime)
	{ //restart same map...
		trap->SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
	}
	else
	{
		trap->SendConsoleCommand( EXEC_APPEND, "vstr nextmap\n" );
	}
	level.changemap = NULL;
	level.intermissiontime = 0;

	if (level.gametype == GT_SIEGE &&
		g_siegeTeamSwitch.integer)
	{ //switch out now
		SiegeDoTeamAssign();
	}

	// reset all the scores so we don't enter the intermission again
	level.teamScores[TEAM_RED] = 0;
	level.teamScores[TEAM_BLUE] = 0;
	for ( i=0 ; i< sv_maxclients.integer ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		cl->ps.persistant[PERS_SCORE] = 0;
	}

	// we need to do this here before chaning to CON_CONNECTING
	G_WriteSessionData();

	// change all client states to connecting, so the early players into the
	// next level will know the others aren't done reconnecting
	for (i=0 ; i< sv_maxclients.integer ; i++) {
		if ( level.clients[i].pers.connected == CON_CONNECTED ) {
			level.clients[i].pers.connected = CON_CONNECTING;
		}
	}

}

/*
=================
G_LogPrintf

Print to the logfile with a time stamp if it is open
=================
*/
void QDECL G_LogPrintf( const char *fmt, ... ) {
	va_list		argptr;
	char		string[1024] = {0};
	int			mins, seconds, msec, l;

	msec = level.time - level.startTime;

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds %= 60;
//	msec %= 1000;

	Com_sprintf( string, sizeof( string ), "%i:%02i ", mins, seconds );

	l = strlen( string );

	va_start( argptr, fmt );
	Q_vsnprintf( string + l, sizeof( string ) - l, fmt, argptr );
	va_end( argptr );

	if ( dedicated.integer )
		trap->Print( "%s", string + l );

	if ( !level.logFile )
		return;

	trap->FS_Write( string, strlen( string ), level.logFile );
}
/*
=================
G_SecurityLogPrintf

Print to the security logfile with a time stamp if it is open
=================
*/
void QDECL G_SecurityLogPrintf( const char *fmt, ... ) {
	va_list		argptr;
	char		string[1024] = {0};
	time_t		rawtime;
	int			timeLen=0;

	time( &rawtime );
	localtime( &rawtime );
	strftime( string, sizeof( string ), "[%Y-%m-%d] [%H:%M:%S] ", gmtime( &rawtime ) );
	timeLen = strlen( string );

	va_start( argptr, fmt );
	Q_vsnprintf( string+timeLen, sizeof( string ) - timeLen, fmt, argptr );
	va_end( argptr );

	if ( dedicated.integer )
		trap->Print( "%s", string + timeLen );

	if ( !level.security.log )
		return;

	trap->FS_Write( string, strlen( string ), level.security.log );
}

/*
================
LogExit

Append information about this game to the log file
================
*/
void LogExit( const char *string ) {
	int				i, numSorted;
	gclient_t		*cl;
//	qboolean		won = qtrue;
	G_LogPrintf( "Exit: %s\n", string );

	level.intermissionQueued = level.time;

	// this will keep the clients from playing any voice sounds
	// that will get cut off when the queued intermission starts
	trap->SetConfigstring( CS_INTERMISSION, "1" );

	// don't send more than 32 scores (FIXME?)
	numSorted = level.numConnectedClients;
	if ( numSorted > 32 ) {
		numSorted = 32;
	}

	if ( level.gametype >= GT_TEAM ) {
		G_LogPrintf( "red:%i  blue:%i\n",
			level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE] );
	}

	for (i=0 ; i < numSorted ; i++) {
		int		ping;

		cl = &level.clients[level.sortedClients[i]];

		if ( cl->sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}
		if ( cl->pers.connected == CON_CONNECTING ) {
			continue;
		}

		ping = cl->ps.ping < 999 ? cl->ps.ping : 999;

		if (level.gametype >= GT_TEAM) {
			G_LogPrintf( "(%s) score: %i  ping: %i  client: [%s] %i \"%s^7\"\n", TeamName(cl->ps.persistant[PERS_TEAM]), cl->ps.persistant[PERS_SCORE], ping, cl->pers.guid, level.sortedClients[i], cl->pers.netname );
		} else {
			G_LogPrintf( "score: %i  ping: %i  client: [%s] %i \"%s^7\"\n", cl->ps.persistant[PERS_SCORE], ping, cl->pers.guid, level.sortedClients[i], cl->pers.netname );
		}
//		if (g_singlePlayer.integer && (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)) {
//			if (g_entities[cl - level.clients].r.svFlags & SVF_BOT && cl->ps.persistant[PERS_RANK] == 0) {
//				won = qfalse;
//			}
//		}
	}

	//yeah.. how about not.
	/*
	if (g_singlePlayer.integer) {
		if (level.gametype >= GT_CTF) {
			won = level.teamScores[TEAM_RED] > level.teamScores[TEAM_BLUE];
		}
		trap->SendConsoleCommand( EXEC_APPEND, (won) ? "spWin\n" : "spLose\n" );
	}
	*/
}

qboolean gDidDuelStuff = qfalse; //gets reset on game reinit

/*
=================
CheckIntermissionExit

The level will stay at the intermission for a minimum of 5 seconds
If all players wish to continue, the level will then exit.
If one or more players have not acknowledged the continue, the game will
wait 10 seconds before going on.
=================
*/
void CheckIntermissionExit( void ) {
	int			ready, notReady;
	int			i;
	gclient_t	*cl;
	int			readyMask;

	// see which players are ready
	ready = 0;
	notReady = 0;
	readyMask = 0;
	for (i=0 ; i< sv_maxclients.integer ; i++) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( g_entities[i].r.svFlags & SVF_BOT ) {
			continue;
		}

		if ( cl->readyToExit ) {
			ready++;
			if ( i < 16 ) {
				readyMask |= 1 << i;
			}
		} else {
			notReady++;
		}
	}

	if ( (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL) && !gDidDuelStuff &&
		(level.time > level.intermissiontime + 2000) )
	{
		gDidDuelStuff = qtrue;

		if ( g_austrian.integer && level.gametype != GT_POWERDUEL )
		{
			G_LogPrintf("Duel Results:\n");
			//G_LogPrintf("Duel Time: %d\n", level.time );
			G_LogPrintf("winner: %s, score: %d, wins/losses: %d/%d\n",
				level.clients[level.sortedClients[0]].pers.netname,
				level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE],
				level.clients[level.sortedClients[0]].sess.wins,
				level.clients[level.sortedClients[0]].sess.losses );
			G_LogPrintf("loser: %s, score: %d, wins/losses: %d/%d\n",
				level.clients[level.sortedClients[1]].pers.netname,
				level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE],
				level.clients[level.sortedClients[1]].sess.wins,
				level.clients[level.sortedClients[1]].sess.losses );
		}
		// if we are running a tournament map, kick the loser to spectator status,
		// which will automatically grab the next spectator and restart
		if (!DuelLimitHit())
		{
			if (level.gametype == GT_POWERDUEL)
			{
				RemovePowerDuelLosers();
				AddPowerDuelPlayers();
			}
			else
			{
				if (level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE] ==
					level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE] &&
					level.clients[level.sortedClients[0]].pers.connected == CON_CONNECTED &&
					level.clients[level.sortedClients[1]].pers.connected == CON_CONNECTED)
				{
					RemoveDuelDrawLoser();
				}
				else
				{
					RemoveTournamentLoser();
				}
				AddTournamentPlayer();
			}

			if ( g_austrian.integer )
			{
				if (level.gametype == GT_POWERDUEL)
				{
					G_LogPrintf("Power Duel Initiated: %s %d/%d vs %s %d/%d and %s %d/%d, kill limit: %d\n",
						level.clients[level.sortedClients[0]].pers.netname,
						level.clients[level.sortedClients[0]].sess.wins,
						level.clients[level.sortedClients[0]].sess.losses,
						level.clients[level.sortedClients[1]].pers.netname,
						level.clients[level.sortedClients[1]].sess.wins,
						level.clients[level.sortedClients[1]].sess.losses,
						level.clients[level.sortedClients[2]].pers.netname,
						level.clients[level.sortedClients[2]].sess.wins,
						level.clients[level.sortedClients[2]].sess.losses,
						fraglimit.integer );
				}
				else
				{
					G_LogPrintf("Duel Initiated: %s %d/%d vs %s %d/%d, kill limit: %d\n",
						level.clients[level.sortedClients[0]].pers.netname,
						level.clients[level.sortedClients[0]].sess.wins,
						level.clients[level.sortedClients[0]].sess.losses,
						level.clients[level.sortedClients[1]].pers.netname,
						level.clients[level.sortedClients[1]].sess.wins,
						level.clients[level.sortedClients[1]].sess.losses,
						fraglimit.integer );
				}
			}

			if (level.gametype == GT_POWERDUEL)
			{
				if (level.numPlayingClients >= 3 && level.numNonSpectatorClients >= 3)
				{
					trap->SetConfigstring ( CS_CLIENT_DUELISTS, va("%i|%i|%i", level.sortedClients[0], level.sortedClients[1], level.sortedClients[2] ) );
					trap->SetConfigstring ( CS_CLIENT_DUELWINNER, "-1" );
				}
			}
			else
			{
				if (level.numPlayingClients >= 2)
				{
					trap->SetConfigstring ( CS_CLIENT_DUELISTS, va("%i|%i", level.sortedClients[0], level.sortedClients[1] ) );
					trap->SetConfigstring ( CS_CLIENT_DUELWINNER, "-1" );
				}
			}

			return;
		}

		if ( g_austrian.integer && level.gametype != GT_POWERDUEL )
		{
			G_LogPrintf("Duel Tournament Winner: %s wins/losses: %d/%d\n",
				level.clients[level.sortedClients[0]].pers.netname,
				level.clients[level.sortedClients[0]].sess.wins,
				level.clients[level.sortedClients[0]].sess.losses );
		}

		if (level.gametype == GT_POWERDUEL)
		{
			RemovePowerDuelLosers();
			AddPowerDuelPlayers();

			if (level.numPlayingClients >= 3 && level.numNonSpectatorClients >= 3)
			{
				trap->SetConfigstring ( CS_CLIENT_DUELISTS, va("%i|%i|%i", level.sortedClients[0], level.sortedClients[1], level.sortedClients[2] ) );
				trap->SetConfigstring ( CS_CLIENT_DUELWINNER, "-1" );
			}
		}
		else
		{
			//this means we hit the duel limit so reset the wins/losses
			//but still push the loser to the back of the line, and retain the order for
			//the map change
			if (level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE] ==
				level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE] &&
				level.clients[level.sortedClients[0]].pers.connected == CON_CONNECTED &&
				level.clients[level.sortedClients[1]].pers.connected == CON_CONNECTED)
			{
				RemoveDuelDrawLoser();
			}
			else
			{
				RemoveTournamentLoser();
			}

			AddTournamentPlayer();

			if (level.numPlayingClients >= 2)
			{
				trap->SetConfigstring ( CS_CLIENT_DUELISTS, va("%i|%i", level.sortedClients[0], level.sortedClients[1] ) );
				trap->SetConfigstring ( CS_CLIENT_DUELWINNER, "-1" );
			}
		}
	}

	if ((level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL) && !gDuelExit)
	{ //in duel, we have different behaviour for between-round intermissions
		if ( level.time > level.intermissiontime + 4000 )
		{ //automatically go to next after 4 seconds
			ExitLevel();
			return;
		}

		for (i=0 ; i< sv_maxclients.integer ; i++)
		{ //being in a "ready" state is not necessary here, so clear it for everyone
		  //yes, I also thinking holding this in a ps value uniquely for each player
		  //is bad and wrong, but it wasn't my idea.
			cl = level.clients + i;
			if ( cl->pers.connected != CON_CONNECTED )
			{
				continue;
			}
			cl->ps.stats[STAT_CLIENTS_READY] = 0;
		}
		return;
	}

	// copy the readyMask to each player's stats so
	// it can be displayed on the scoreboard
	for (i=0 ; i< sv_maxclients.integer ; i++) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		cl->ps.stats[STAT_CLIENTS_READY] = readyMask;
	}

	// never exit in less than five seconds
	if ( level.time < level.intermissiontime + 5000 ) {
		return;
	}

	if (d_noIntermissionWait.integer)
	{ //don't care who wants to go, just go.
		ExitLevel();
		return;
	}

	// if nobody wants to go, clear timer
	if ( !ready ) {
		level.readyToExit = qfalse;
		return;
	}

	// if everyone wants to go, go now
	if ( !notReady ) {
		ExitLevel();
		return;
	}

	// the first person to ready starts the ten second timeout
	if ( !level.readyToExit ) {
		level.readyToExit = qtrue;
		level.exitTime = level.time;
	}

	// if we have waited ten seconds since at least one player
	// wanted to exit, go ahead
	if ( level.time < level.exitTime + 10000 ) {
		return;
	}

	ExitLevel();
}

/*
=============
ScoreIsTied
=============
*/
qboolean ScoreIsTied( void ) {
	int		a, b;

	if ( level.numPlayingClients < 2 ) {
		return qfalse;
	}

	if ( level.gametype >= GT_TEAM ) {
		return level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE];
	}

	a = level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE];
	b = level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE];

	return a == b;
}

/*
=================
CheckExitRules

There will be a delay between the time the exit is qualified for
and the time everyone is moved to the intermission spot, so you
can see the last frag.
=================
*/
qboolean g_endPDuel = qfalse;
void CheckExitRules( void ) {
 	int			i;
	gclient_t	*cl;
	char *sKillLimit;
	qboolean printLimit = qtrue;
	// if at the intermission, wait for all non-bots to
	// signal ready, then go to next level
	if ( level.intermissiontime ) {
		CheckIntermissionExit ();
		return;
	}

	if (gDoSlowMoDuel)
	{ //don't go to intermission while in slow motion
		return;
	}

	if (gEscaping)
	{
		int numLiveClients = 0;

		for ( i=0; i < MAX_CLIENTS; i++ )
		{
			if (g_entities[i].inuse && g_entities[i].client && g_entities[i].health > 0)
			{
				if (g_entities[i].client->sess.sessionTeam != TEAM_SPECTATOR &&
					!(g_entities[i].client->ps.pm_flags & PMF_FOLLOW))
				{
					numLiveClients++;
				}
			}
		}
		if (gEscapeTime < level.time)
		{
			gEscaping = qfalse;
			LogExit( "Escape time ended." );
			return;
		}
		if (!numLiveClients)
		{
			gEscaping = qfalse;
			LogExit( "Everyone failed to escape." );
			return;
		}
	}

	if ( level.intermissionQueued ) {
		//int time = (g_singlePlayer.integer) ? SP_INTERMISSION_DELAY_TIME : INTERMISSION_DELAY_TIME;
		int time = INTERMISSION_DELAY_TIME;
		if ( level.time - level.intermissionQueued >= time ) {
			level.intermissionQueued = 0;
			BeginIntermission();
		}
		return;
	}

	/*
	if (level.gametype == GT_POWERDUEL)
	{
		if (level.numPlayingClients < 3)
		{
			if (!level.intermissiontime)
			{
				if (d_powerDuelPrint.integer)
				{
					Com_Printf("POWERDUEL WIN CONDITION: Duel forfeit (1)\n");
				}
				LogExit("Duel forfeit.");
				return;
			}
		}
	}
	*/

	// check for sudden death
	if (level.gametype != GT_SIEGE)
	{
		if ( ScoreIsTied() ) {
			// always wait for sudden death
			if ((level.gametype != GT_DUEL) || !timelimit.value)
			{
				if (level.gametype != GT_POWERDUEL)
				{
					return;
				}
			}
		}
	}

	if (level.gametype != GT_SIEGE)
	{
		if ( timelimit.value > 0.0f && !level.warmupTime ) {
			if ( level.time - level.startTime >= timelimit.value*60000 ) {
//				trap->SendServerCommand( -1, "print \"Timelimit hit.\n\"");
				trap->SendServerCommand( -1, va("print \"%s.\n\"",G_GetStringEdString("MP_SVGAME", "TIMELIMIT_HIT")));
				if (d_powerDuelPrint.integer)
				{
					Com_Printf("POWERDUEL WIN CONDITION: Timelimit hit (1)\n");
				}
				LogExit( "Timelimit hit." );
				return;
			}
		}

		if (zyk_server_empty_change_map_time.integer > 0)
		{
			if (level.num_fully_connected_clients == 0)
			{ // zyk: changes map if server has no one for some time
				if (level.server_empty_change_map_timer == 0)
					level.server_empty_change_map_timer = level.time;

				if ((level.time - level.server_empty_change_map_timer) > zyk_server_empty_change_map_time.integer)
					ExitLevel();
			}
			else
			{ // zyk: if someone connects, reset the counter
				level.server_empty_change_map_timer = 0;
			}
		}
	}

	if (level.gametype == GT_POWERDUEL && level.numPlayingClients >= 3)
	{
		if (g_endPDuel)
		{
			g_endPDuel = qfalse;
			LogExit("Powerduel ended.");
		}

		//yeah, this stuff was completely insane.
		/*
		int duelists[3];
		duelists[0] = level.sortedClients[0];
		duelists[1] = level.sortedClients[1];
		duelists[2] = level.sortedClients[2];

		if (duelists[0] != -1 &&
			duelists[1] != -1 &&
			duelists[2] != -1)
		{
			if (!g_entities[duelists[0]].inuse ||
				!g_entities[duelists[0]].client ||
				g_entities[duelists[0]].client->ps.stats[STAT_HEALTH] <= 0 ||
				g_entities[duelists[0]].client->sess.sessionTeam != TEAM_FREE)
			{ //The lone duelist lost, give the other two wins (if applicable) and him a loss
				if (g_entities[duelists[0]].inuse &&
					g_entities[duelists[0]].client)
				{
					g_entities[duelists[0]].client->sess.losses++;
					ClientUserinfoChanged(duelists[0]);
				}
				if (g_entities[duelists[1]].inuse &&
					g_entities[duelists[1]].client)
				{
					if (g_entities[duelists[1]].client->ps.stats[STAT_HEALTH] > 0 &&
						g_entities[duelists[1]].client->sess.sessionTeam == TEAM_FREE)
					{
						g_entities[duelists[1]].client->sess.wins++;
					}
					else
					{
						g_entities[duelists[1]].client->sess.losses++;
					}
					ClientUserinfoChanged(duelists[1]);
				}
				if (g_entities[duelists[2]].inuse &&
					g_entities[duelists[2]].client)
				{
					if (g_entities[duelists[2]].client->ps.stats[STAT_HEALTH] > 0 &&
						g_entities[duelists[2]].client->sess.sessionTeam == TEAM_FREE)
					{
						g_entities[duelists[2]].client->sess.wins++;
					}
					else
					{
						g_entities[duelists[2]].client->sess.losses++;
					}
					ClientUserinfoChanged(duelists[2]);
				}

				//Will want to parse indecies for two out at some point probably
				trap->SetConfigstring ( CS_CLIENT_DUELWINNER, va("%i", duelists[1] ) );

				if (d_powerDuelPrint.integer)
				{
					Com_Printf("POWERDUEL WIN CONDITION: Coupled duelists won (1)\n");
				}
				LogExit( "Coupled duelists won." );
				gDuelExit = qfalse;
			}
			else if ((!g_entities[duelists[1]].inuse ||
				!g_entities[duelists[1]].client ||
				g_entities[duelists[1]].client->sess.sessionTeam != TEAM_FREE ||
				g_entities[duelists[1]].client->ps.stats[STAT_HEALTH] <= 0) &&
				(!g_entities[duelists[2]].inuse ||
				!g_entities[duelists[2]].client ||
				g_entities[duelists[2]].client->sess.sessionTeam != TEAM_FREE ||
				g_entities[duelists[2]].client->ps.stats[STAT_HEALTH] <= 0))
			{ //the coupled duelists lost, give the lone duelist a win (if applicable) and the couple both losses
				if (g_entities[duelists[1]].inuse &&
					g_entities[duelists[1]].client)
				{
					g_entities[duelists[1]].client->sess.losses++;
					ClientUserinfoChanged(duelists[1]);
				}
				if (g_entities[duelists[2]].inuse &&
					g_entities[duelists[2]].client)
				{
					g_entities[duelists[2]].client->sess.losses++;
					ClientUserinfoChanged(duelists[2]);
				}

				if (g_entities[duelists[0]].inuse &&
					g_entities[duelists[0]].client &&
					g_entities[duelists[0]].client->ps.stats[STAT_HEALTH] > 0 &&
					g_entities[duelists[0]].client->sess.sessionTeam == TEAM_FREE)
				{
					g_entities[duelists[0]].client->sess.wins++;
					ClientUserinfoChanged(duelists[0]);
				}

				trap->SetConfigstring ( CS_CLIENT_DUELWINNER, va("%i", duelists[0] ) );

				if (d_powerDuelPrint.integer)
				{
					Com_Printf("POWERDUEL WIN CONDITION: Lone duelist won (1)\n");
				}
				LogExit( "Lone duelist won." );
				gDuelExit = qfalse;
			}
		}
		*/
		return;
	}

	if ( level.numPlayingClients < 2 ) {
		return;
	}

	if (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
	{
		if (fraglimit.integer > 1)
		{
			sKillLimit = "Kill limit hit.";
		}
		else
		{
			sKillLimit = "";
			printLimit = qfalse;
		}
	}
	else
	{
		sKillLimit = "Kill limit hit.";
	}
	if ( level.gametype < GT_SIEGE && fraglimit.integer ) {
		if ( level.teamScores[TEAM_RED] >= fraglimit.integer ) {
			trap->SendServerCommand( -1, va("print \"Red %s\n\"", G_GetStringEdString("MP_SVGAME", "HIT_THE_KILL_LIMIT")) );
			if (d_powerDuelPrint.integer)
			{
				Com_Printf("POWERDUEL WIN CONDITION: Kill limit (1)\n");
			}
			LogExit( sKillLimit );
			return;
		}

		if ( level.teamScores[TEAM_BLUE] >= fraglimit.integer ) {
			trap->SendServerCommand( -1, va("print \"Blue %s\n\"", G_GetStringEdString("MP_SVGAME", "HIT_THE_KILL_LIMIT")) );
			if (d_powerDuelPrint.integer)
			{
				Com_Printf("POWERDUEL WIN CONDITION: Kill limit (2)\n");
			}
			LogExit( sKillLimit );
			return;
		}

		for ( i=0 ; i< sv_maxclients.integer ; i++ ) {
			cl = level.clients + i;
			if ( cl->pers.connected != CON_CONNECTED ) {
				continue;
			}
			if ( cl->sess.sessionTeam != TEAM_FREE ) {
				continue;
			}

			if ( (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL) && duel_fraglimit.integer && cl->sess.wins >= duel_fraglimit.integer )
			{
				if (d_powerDuelPrint.integer)
				{
					Com_Printf("POWERDUEL WIN CONDITION: Duel limit hit (1)\n");
				}
				LogExit( "Duel limit hit." );
				gDuelExit = qtrue;
				trap->SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " hit the win limit.\n\"",
					cl->pers.netname ) );
				return;
			}

			if ( cl->ps.persistant[PERS_SCORE] >= fraglimit.integer ) {
				if (d_powerDuelPrint.integer)
				{
					Com_Printf("POWERDUEL WIN CONDITION: Kill limit (3)\n");
				}
				LogExit( sKillLimit );
				gDuelExit = qfalse;
				if (printLimit)
				{
					trap->SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " %s.\n\"",
													cl->pers.netname,
													G_GetStringEdString("MP_SVGAME", "HIT_THE_KILL_LIMIT")
													)
											);
				}
				return;
			}
		}
	}

	if ( level.gametype >= GT_CTF && capturelimit.integer ) {

		if ( level.teamScores[TEAM_RED] >= capturelimit.integer )
		{
			trap->SendServerCommand( -1,  va("print \"%s \"", G_GetStringEdString("MP_SVGAME", "PRINTREDTEAM")));
			trap->SendServerCommand( -1,  va("print \"%s.\n\"", G_GetStringEdString("MP_SVGAME", "HIT_CAPTURE_LIMIT")));
			LogExit( "Capturelimit hit." );
			return;
		}

		if ( level.teamScores[TEAM_BLUE] >= capturelimit.integer ) {
			trap->SendServerCommand( -1,  va("print \"%s \"", G_GetStringEdString("MP_SVGAME", "PRINTBLUETEAM")));
			trap->SendServerCommand( -1,  va("print \"%s.\n\"", G_GetStringEdString("MP_SVGAME", "HIT_CAPTURE_LIMIT")));
			LogExit( "Capturelimit hit." );
			return;
		}
	}
}



/*
========================================================================

FUNCTIONS CALLED EVERY FRAME

========================================================================
*/

void G_RemoveDuelist(int team)
{
	int i = 0;
	gentity_t *ent;
	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent->inuse && ent->client && ent->client->sess.sessionTeam != TEAM_SPECTATOR &&
			ent->client->sess.duelTeam == team)
		{
			SetTeam(ent, "s");
		}
        i++;
	}
}

/*
=============
CheckTournament

Once a frame, check for changes in tournament player state
=============
*/
int g_duelPrintTimer = 0;
void CheckTournament( void ) {
	// check because we run 3 game frames before calling Connect and/or ClientBegin
	// for clients on a map_restart
//	if ( level.numPlayingClients == 0 && (level.gametype != GT_POWERDUEL) ) {
//		return;
//	}

	if (level.gametype == GT_POWERDUEL)
	{
		if (level.numPlayingClients >= 3 && level.numNonSpectatorClients >= 3)
		{
			trap->SetConfigstring ( CS_CLIENT_DUELISTS, va("%i|%i|%i", level.sortedClients[0], level.sortedClients[1], level.sortedClients[2] ) );
		}
	}
	else
	{
		if (level.numPlayingClients >= 2)
		{
			trap->SetConfigstring ( CS_CLIENT_DUELISTS, va("%i|%i", level.sortedClients[0], level.sortedClients[1] ) );
		}
	}

	if ( level.gametype == GT_DUEL )
	{
		// pull in a spectator if needed
		if ( level.numPlayingClients < 2 && !level.intermissiontime && !level.intermissionQueued ) {
			AddTournamentPlayer();

			if (level.numPlayingClients >= 2)
			{
				trap->SetConfigstring ( CS_CLIENT_DUELISTS, va("%i|%i", level.sortedClients[0], level.sortedClients[1] ) );
			}
		}

		if (level.numPlayingClients >= 2)
		{
// nmckenzie: DUEL_HEALTH
			if ( g_showDuelHealths.integer >= 1 )
			{
				playerState_t *ps1, *ps2;
				ps1 = &level.clients[level.sortedClients[0]].ps;
				ps2 = &level.clients[level.sortedClients[1]].ps;
				trap->SetConfigstring ( CS_CLIENT_DUELHEALTHS, va("%i|%i|!",
					ps1->stats[STAT_HEALTH], ps2->stats[STAT_HEALTH]));
			}
		}

		//rww - It seems we have decided there will be no warmup in duel.
		//if (!g_warmup.integer)
		{ //don't care about any of this stuff then, just add people and leave me alone
			level.warmupTime = 0;
			return;
		}
#if 0
		// if we don't have two players, go back to "waiting for players"
		if ( level.numPlayingClients != 2 ) {
			if ( level.warmupTime != -1 ) {
				level.warmupTime = -1;
				trap->SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
				G_LogPrintf( "Warmup:\n" );
			}
			return;
		}

		if ( level.warmupTime == 0 ) {
			return;
		}

		// if the warmup is changed at the console, restart it
		if ( g_warmup.modificationCount != level.warmupModificationCount ) {
			level.warmupModificationCount = g_warmup.modificationCount;
			level.warmupTime = -1;
		}

		// if all players have arrived, start the countdown
		if ( level.warmupTime < 0 ) {
			if ( level.numPlayingClients == 2 ) {
				// fudge by -1 to account for extra delays
				level.warmupTime = level.time + ( g_warmup.integer - 1 ) * 1000;

				if (level.warmupTime < (level.time + 3000))
				{ //rww - this is an unpleasent hack to keep the level from resetting completely on the client (this happens when two map_restarts are issued rapidly)
					level.warmupTime = level.time + 3000;
				}
				trap->SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
			}
			return;
		}

		// if the warmup time has counted down, restart
		if ( level.time > level.warmupTime ) {
			level.warmupTime += 10000;
			trap->Cvar_Set( "g_restarted", "1" );
			trap->SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
			level.restarted = qtrue;
			return;
		}
#endif
	}
	else if (level.gametype == GT_POWERDUEL)
	{
		if (level.numPlayingClients < 2)
		{ //hmm, ok, pull more in.
			g_dontFrickinCheck = qfalse;
		}

		if (level.numPlayingClients > 3)
		{ //umm..yes..lets take care of that then.
			int lone = 0, dbl = 0;

			G_PowerDuelCount(&lone, &dbl, qfalse);
			if (lone > 1)
			{
				G_RemoveDuelist(DUELTEAM_LONE);
			}
			else if (dbl > 2)
			{
				G_RemoveDuelist(DUELTEAM_DOUBLE);
			}
		}
		else if (level.numPlayingClients < 3)
		{ //hmm, someone disconnected or something and we need em
			int lone = 0, dbl = 0;

			G_PowerDuelCount(&lone, &dbl, qfalse);
			if (lone < 1)
			{
				g_dontFrickinCheck = qfalse;
			}
			else if (dbl < 1)
			{
				g_dontFrickinCheck = qfalse;
			}
		}

		// pull in a spectator if needed
		if (level.numPlayingClients < 3 && !g_dontFrickinCheck)
		{
			AddPowerDuelPlayers();

			if (level.numPlayingClients >= 3 &&
				G_CanResetDuelists())
			{
				gentity_t *te = G_TempEntity(vec3_origin, EV_GLOBAL_DUEL);
				te->r.svFlags |= SVF_BROADCAST;
				//this is really pretty nasty, but..
				te->s.otherEntityNum = level.sortedClients[0];
				te->s.otherEntityNum2 = level.sortedClients[1];
				te->s.groundEntityNum = level.sortedClients[2];

				trap->SetConfigstring ( CS_CLIENT_DUELISTS, va("%i|%i|%i", level.sortedClients[0], level.sortedClients[1], level.sortedClients[2] ) );
				G_ResetDuelists();

				g_dontFrickinCheck = qtrue;
			}
			else if (level.numPlayingClients > 0 ||
				level.numConnectedClients > 0)
			{
				if (g_duelPrintTimer < level.time)
				{ //print once every 10 seconds
					int lone = 0, dbl = 0;

					G_PowerDuelCount(&lone, &dbl, qtrue);
					if (lone < 1)
					{
						trap->SendServerCommand( -1, va("cp \"%s\n\"", G_GetStringEdString("MP_SVGAME", "DUELMORESINGLE")) );
					}
					else
					{
						trap->SendServerCommand( -1, va("cp \"%s\n\"", G_GetStringEdString("MP_SVGAME", "DUELMOREPAIRED")) );
					}
					g_duelPrintTimer = level.time + 10000;
				}
			}

			if (level.numPlayingClients >= 3 && level.numNonSpectatorClients >= 3)
			{ //pulled in a needed person
				if (G_CanResetDuelists())
				{
					gentity_t *te = G_TempEntity(vec3_origin, EV_GLOBAL_DUEL);
					te->r.svFlags |= SVF_BROADCAST;
					//this is really pretty nasty, but..
					te->s.otherEntityNum = level.sortedClients[0];
					te->s.otherEntityNum2 = level.sortedClients[1];
					te->s.groundEntityNum = level.sortedClients[2];

					trap->SetConfigstring ( CS_CLIENT_DUELISTS, va("%i|%i|%i", level.sortedClients[0], level.sortedClients[1], level.sortedClients[2] ) );

					if ( g_austrian.integer )
					{
						G_LogPrintf("Duel Initiated: %s %d/%d vs %s %d/%d and %s %d/%d, kill limit: %d\n",
							level.clients[level.sortedClients[0]].pers.netname,
							level.clients[level.sortedClients[0]].sess.wins,
							level.clients[level.sortedClients[0]].sess.losses,
							level.clients[level.sortedClients[1]].pers.netname,
							level.clients[level.sortedClients[1]].sess.wins,
							level.clients[level.sortedClients[1]].sess.losses,
							level.clients[level.sortedClients[2]].pers.netname,
							level.clients[level.sortedClients[2]].sess.wins,
							level.clients[level.sortedClients[2]].sess.losses,
							fraglimit.integer );
					}
					//trap->SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
					//FIXME: This seems to cause problems. But we'd like to reset things whenever a new opponent is set.
				}
			}
		}
		else
		{ //if you have proper num of players then don't try to add again
			g_dontFrickinCheck = qtrue;
		}

		level.warmupTime = 0;
		return;
	}
	else if ( level.warmupTime != 0 ) {
		int		counts[TEAM_NUM_TEAMS];
		qboolean	notEnough = qfalse;

		if ( level.gametype > GT_TEAM ) {
			counts[TEAM_BLUE] = TeamCount( -1, TEAM_BLUE );
			counts[TEAM_RED] = TeamCount( -1, TEAM_RED );

			if (counts[TEAM_RED] < 1 || counts[TEAM_BLUE] < 1) {
				notEnough = qtrue;
			}
		} else if ( level.numPlayingClients < 2 ) {
			notEnough = qtrue;
		}

		if ( notEnough ) {
			if ( level.warmupTime != -1 ) {
				level.warmupTime = -1;
				trap->SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
				G_LogPrintf( "Warmup:\n" );
			}
			return; // still waiting for team members
		}

		if ( level.warmupTime == 0 ) {
			return;
		}

		// if the warmup is changed at the console, restart it
		/*
		if ( g_warmup.modificationCount != level.warmupModificationCount ) {
			level.warmupModificationCount = g_warmup.modificationCount;
			level.warmupTime = -1;
		}
		*/

		// if all players have arrived, start the countdown
		if ( level.warmupTime < 0 ) {
			// fudge by -1 to account for extra delays
			if ( g_warmup.integer > 1 ) {
				level.warmupTime = level.time + ( g_warmup.integer - 1 ) * 1000;
			} else {
				level.warmupTime = 0;
			}
			trap->SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
			return;
		}

		// if the warmup time has counted down, restart
		if ( level.time > level.warmupTime ) {
			level.warmupTime += 10000;
			trap->Cvar_Set( "g_restarted", "1" );
			trap->Cvar_Update( &g_restarted );
			trap->SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
			level.restarted = qtrue;
			return;
		}
	}
}

void G_KickAllBots(void)
{
	int i;
	gclient_t	*cl;

	for ( i=0 ; i< sv_maxclients.integer ; i++ )
	{
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED )
		{
			continue;
		}
		if ( !(g_entities[i].r.svFlags & SVF_BOT) )
		{
			continue;
		}
		trap->SendConsoleCommand( EXEC_INSERT, va("clientkick %d\n", i) );
	}
}

/*
==================
CheckVote
==================
*/
void CheckVote( void ) {
	if ( level.voteExecuteTime && level.voteExecuteTime < level.time ) {
		level.voteExecuteTime = 0;
		trap->SendConsoleCommand( EXEC_APPEND, va( "%s\n", level.voteString ) );

		if (level.votingGametype)
		{
			if (level.gametype != level.votingGametypeTo)
			{ //If we're voting to a different game type, be sure to refresh all the map stuff
				const char *nextMap = G_RefreshNextMap(level.votingGametypeTo, qtrue);

				if (level.votingGametypeTo == GT_SIEGE)
				{ //ok, kick all the bots, cause the aren't supported!
                    G_KickAllBots();
					//just in case, set this to 0 too... I guess...maybe?
					//trap->Cvar_Set("bot_minplayers", "0");
				}

				if (nextMap && nextMap[0] && zyk_change_map_gametype_vote.integer)
				{
					trap->SendConsoleCommand( EXEC_APPEND, va("map %s\n", nextMap ) );
				}
				else
				{ // zyk: if zyk_change_map_gametype_vote is 0, just restart the current map
					trap->SendConsoleCommand( EXEC_APPEND, "map_restart 0\n"  );
				}
			}
			else
			{ //otherwise, just leave the map until a restart
				G_RefreshNextMap(level.votingGametypeTo, qfalse);
			}

			if (g_fraglimitVoteCorrection.integer)
			{ //This means to auto-correct fraglimit when voting to and from duel.
				const int currentGT = level.gametype;
				const int currentFL = fraglimit.integer;
				const int currentTL = timelimit.integer;

				if ((level.votingGametypeTo == GT_DUEL || level.votingGametypeTo == GT_POWERDUEL) && currentGT != GT_DUEL && currentGT != GT_POWERDUEL)
				{
					if (currentFL > 1 || !currentFL)
					{ //if voting to duel, and fraglimit is more than 1 (or unlimited), then set it down to 1
						trap->SendConsoleCommand(EXEC_APPEND, "fraglimit 1\n"); // zyk: changed from 3 to 1
					}
					if (currentTL)
					{ //if voting to duel, and timelimit is set, make it unlimited
						trap->SendConsoleCommand(EXEC_APPEND, "timelimit 0\n");
					}
				}
				else if ((level.votingGametypeTo != GT_DUEL && level.votingGametypeTo != GT_POWERDUEL) &&
					(currentGT == GT_DUEL || currentGT == GT_POWERDUEL))
				{
					if (currentFL != 0)
					{ //if voting from duel, an fraglimit is different than 0, then set it up to 0
						trap->SendConsoleCommand(EXEC_APPEND, "fraglimit 0\n"); // zyk: changed from 20 to 0
					}
				}
			}

			level.votingGametype = qfalse;
			level.votingGametypeTo = 0;
		}
	}
	if ( !level.voteTime ) {
		return;
	}
	if ( level.time-level.voteTime >= VOTE_TIME) // || level.voteYes + level.voteNo == 0 ) zyk: no longer does this
	{
		if (level.voteYes > level.voteNo)
		{ // zyk: now vote pass if number of Yes is greater than number of No
			trap->SendServerCommand( -1, va("print \"%s (%s)\n\"", G_GetStringEdString("MP_SVGAME", "VOTEPASSED"), level.voteStringClean) );
			level.voteExecuteTime = level.time + level.voteExecuteDelay;
		}
		else
		{
			trap->SendServerCommand( -1, va("print \"%s (%s)\n\"", G_GetStringEdString("MP_SVGAME", "VOTEFAILED"), level.voteStringClean) );
		}

		// zyk: set the timer for the next vote of this player
		if (zyk_vote_timer.integer > 0 && level.voting_player > -1)
			g_entities[level.voting_player].client->sess.vote_timer = zyk_vote_timer.integer;
	}
	else 
	{
		if ( level.voteYes > level.numVotingClients/2 ) {
			// execute the command, then remove the vote
			trap->SendServerCommand( -1, va("print \"%s (%s)\n\"", G_GetStringEdString("MP_SVGAME", "VOTEPASSED"), level.voteStringClean) );
			level.voteExecuteTime = level.time + level.voteExecuteDelay;
			// zyk: set the timer for the next vote of this player
			if (zyk_vote_timer.integer > 0 && level.voting_player > -1)
				g_entities[level.voting_player].client->sess.vote_timer = zyk_vote_timer.integer;
		}
		// same behavior as a timeout
		else if ( level.voteNo >= (level.numVotingClients+1)/2 )
		{
			trap->SendServerCommand( -1, va("print \"%s (%s)\n\"", G_GetStringEdString("MP_SVGAME", "VOTEFAILED"), level.voteStringClean) );
			// zyk: set the timer for the next vote of this player
			if (zyk_vote_timer.integer > 0 && level.voting_player > -1)
				g_entities[level.voting_player].client->sess.vote_timer = zyk_vote_timer.integer;
		}
		else // still waiting for a majority
			return;
	}
	level.voteTime = 0;
	trap->SetConfigstring( CS_VOTE_TIME, "" );
}

/*
==================
PrintTeam
==================
*/
void PrintTeam(int team, char *message) {
	int i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].sess.sessionTeam != team)
			continue;
		trap->SendServerCommand( i, message );
	}
}

/*
==================
SetLeader
==================
*/
void SetLeader(int team, int client) {
	int i;

	if ( level.clients[client].pers.connected == CON_DISCONNECTED ) {
		PrintTeam(team, va("print \"%s is not connected\n\"", level.clients[client].pers.netname) );
		return;
	}
	if (level.clients[client].sess.sessionTeam != team) {
		PrintTeam(team, va("print \"%s is not on the team anymore\n\"", level.clients[client].pers.netname) );
		return;
	}
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].sess.sessionTeam != team)
			continue;
		if (level.clients[i].sess.teamLeader) {
			level.clients[i].sess.teamLeader = qfalse;
			ClientUserinfoChanged(i);
		}
	}
	level.clients[client].sess.teamLeader = qtrue;
	ClientUserinfoChanged( client );
	PrintTeam(team, va("print \"%s %s\n\"", level.clients[client].pers.netname, G_GetStringEdString("MP_SVGAME", "NEWTEAMLEADER")) );
}

/*
==================
CheckTeamLeader
==================
*/
void CheckTeamLeader( int team ) {
	int i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].sess.sessionTeam != team)
			continue;
		if (level.clients[i].sess.teamLeader)
			break;
	}
	if (i >= level.maxclients) {
		for ( i = 0 ; i < level.maxclients ; i++ ) {
			if (level.clients[i].sess.sessionTeam != team)
				continue;
			if (!(g_entities[i].r.svFlags & SVF_BOT)) {
				level.clients[i].sess.teamLeader = qtrue;
				break;
			}
		}
		if ( i >= level.maxclients ) {
			for ( i = 0 ; i < level.maxclients ; i++ ) {
				if ( level.clients[i].sess.sessionTeam != team )
					continue;
				level.clients[i].sess.teamLeader = qtrue;
				break;
			}
		}
	}
}

/*
==================
CheckTeamVote
==================
*/
void CheckTeamVote( int team ) {
	int cs_offset;

	if ( team == TEAM_RED )
		cs_offset = 0;
	else if ( team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( level.teamVoteExecuteTime[cs_offset] && level.teamVoteExecuteTime[cs_offset] < level.time ) {
		level.teamVoteExecuteTime[cs_offset] = 0;
		if ( !Q_strncmp( "leader", level.teamVoteString[cs_offset], 6) ) {
			//set the team leader
			SetLeader(team, atoi(level.teamVoteString[cs_offset] + 7));
		}
		else {
			trap->SendConsoleCommand( EXEC_APPEND, va("%s\n", level.teamVoteString[cs_offset] ) );
		}
	}

	if ( !level.teamVoteTime[cs_offset] ) {
		return;
	}

	if ( level.time-level.teamVoteTime[cs_offset] >= VOTE_TIME || level.teamVoteYes[cs_offset] + level.teamVoteNo[cs_offset] == 0 ) {
		trap->SendServerCommand( -1, va("print \"%s (%s)\n\"", G_GetStringEdString("MP_SVGAME", "TEAMVOTEFAILED"), level.teamVoteStringClean[cs_offset]) );
	}
	else {
		if ( level.teamVoteYes[cs_offset] > level.numteamVotingClients[cs_offset]/2 ) {
			// execute the command, then remove the vote
			trap->SendServerCommand( -1, va("print \"%s (%s)\n\"", G_GetStringEdString("MP_SVGAME", "TEAMVOTEPASSED"), level.teamVoteStringClean[cs_offset]) );
			level.voteExecuteTime = level.time + 3000;
		}

		// same behavior as a timeout
		else if ( level.teamVoteNo[cs_offset] >= (level.numteamVotingClients[cs_offset]+1)/2 )
			trap->SendServerCommand( -1, va("print \"%s (%s)\n\"", G_GetStringEdString("MP_SVGAME", "TEAMVOTEFAILED"), level.teamVoteStringClean[cs_offset]) );

		else // still waiting for a majority
			return;
	}
	level.teamVoteTime[cs_offset] = 0;
	trap->SetConfigstring( CS_TEAMVOTE_TIME + cs_offset, "" );
}


/*
==================
CheckCvars
==================
*/
void CheckCvars( void ) {
	static int lastMod = -1;

	if ( g_password.modificationCount != lastMod ) {
		char password[MAX_INFO_STRING];
		char *c = password;
		lastMod = g_password.modificationCount;

		strcpy( password, g_password.string );
		while( *c )
		{
			if ( *c == '%' )
			{
				*c = '.';
			}
			c++;
		}
		trap->Cvar_Set("g_password", password );

		if ( *g_password.string && Q_stricmp( g_password.string, "none" ) ) {
			trap->Cvar_Set( "g_needpass", "1" );
		} else {
			trap->Cvar_Set( "g_needpass", "0" );
		}
	}
}

/*
=============
G_RunThink

Runs thinking code for this frame if necessary
=============
*/
void G_RunThink (gentity_t *ent) {
	float	thinktime;

	thinktime = ent->nextthink;
	if (thinktime <= 0) {
		goto runicarus;
	}
	if (thinktime > level.time) {
		goto runicarus;
	}

	ent->nextthink = 0;
	if (!ent->think) {
		//trap->Error( ERR_DROP, "NULL ent->think");
		goto runicarus;
	}
	ent->think (ent);

runicarus:
	if ( ent->inuse )
	{
		SaveNPCGlobals();
		if(NPCS.NPCInfo == NULL && ent->NPC != NULL)
		{
			SetNPCGlobals( ent );
		}
		trap->ICARUS_MaintainTaskManager(ent->s.number);
		RestoreNPCGlobals();
	}
}

int g_LastFrameTime = 0;
int g_TimeSinceLastFrame = 0;

qboolean gDoSlowMoDuel = qfalse;
int gSlowMoDuelTime = 0;

//#define _G_FRAME_PERFANAL

void NAV_CheckCalcPaths( void )
{
	if ( navCalcPathTime && navCalcPathTime < level.time )
	{//first time we've ever loaded this map...
		vmCvar_t	mapname;
		vmCvar_t	ckSum;

		trap->Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );
		trap->Cvar_Register( &ckSum, "sv_mapChecksum", "", CVAR_ROM );

		//clear all the failed edges
		trap->Nav_ClearAllFailedEdges();

		//Calculate all paths
		NAV_CalculatePaths( mapname.string, ckSum.integer );

		trap->Nav_CalculatePaths(qfalse);

#ifndef FINAL_BUILD
		if ( fatalErrors )
		{
			Com_Printf( S_COLOR_RED"Not saving .nav file due to fatal nav errors\n" );
		}
		else
#endif
		if ( trap->Nav_Save( mapname.string, ckSum.integer ) == qfalse )
		{
			Com_Printf("Unable to save navigations data for map \"%s\" (checksum:%d)\n", mapname.string, ckSum.integer );
		}
		navCalcPathTime = 0;
	}
}

//so shared code can get the local time depending on the side it's executed on
int BG_GetTime(void)
{
	return level.time;
}

// zyk: similar to TeleportPlayer(), but this one doesnt spit the player out at the destination
void zyk_TeleportPlayer( gentity_t *player, vec3_t origin, vec3_t angles ) {
	gentity_t	*tent;
	qboolean	isNPC = qfalse;

	if (player->s.eType == ET_NPC)
	{
		isNPC = qtrue;
	}

	// use temp events at source and destination to prevent the effect
	// from getting dropped by a second player event
	if ( player->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		tent = G_TempEntity( player->client->ps.origin, EV_PLAYER_TELEPORT_OUT );
		tent->s.clientNum = player->s.clientNum;

		tent = G_TempEntity( origin, EV_PLAYER_TELEPORT_IN );
		tent->s.clientNum = player->s.clientNum;
	}

	// unlink to make sure it can't possibly interfere with G_KillBox
	trap->UnlinkEntity ((sharedEntity_t *)player);

	VectorCopy ( origin, player->client->ps.origin );
	player->client->ps.origin[2] += 1;

	// set angles
	SetClientViewAngle( player, angles );

	// toggle the teleport bit so the client knows to not lerp
	player->client->ps.eFlags ^= EF_TELEPORT_BIT;

	// kill anything at the destination
	if ( player->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		G_KillBox (player);
	}

	// save results of pmove
	BG_PlayerStateToEntityState( &player->client->ps, &player->s, qtrue );
	if (isNPC)
	{
		player->s.eType = ET_NPC;
	}

	// use the precise origin for linking
	VectorCopy( player->client->ps.origin, player->r.currentOrigin );

	if ( player->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		trap->LinkEntity ((sharedEntity_t *)player);
	}
}

// zyk: function to kill npcs with the name as parameter
void zyk_NPC_Kill_f( char *name )
{
	int	n = 0;
	gentity_t *player = NULL;

	for ( n = level.maxclients; n < level.num_entities; n++) 
	{
		player = &g_entities[n];
		if ( player && player->NPC && player->client )
		{
			if( (Q_stricmp( name, player->NPC_type ) == 0 || Q_stricmp( name, "all" ) == 0) && player->client->pers.guardian_invoked_by_id == -1)
			{ // zyk: do not kill guardians
				player->health = 0;
				player->client->ps.stats[STAT_HEALTH] = 0;
				if (player->die)
				{
					player->die(player, player, player, 100, MOD_UNKNOWN);
				}
			}
		}
	}
}

// zyk: tests if ent has other as ally
qboolean zyk_is_ally(gentity_t *ent, gentity_t *other)
{
	if (ent && other && !ent->NPC && !other->NPC && ent != other && ent->client && other->client && other->client->pers.connected == CON_CONNECTED)
	{
		if (other->s.number > 15 && (ent->client->sess.ally2 & (1 << (other->s.number-16))))
		{
			return qtrue;
		}
		else if (ent->client->sess.ally1 & (1 << other->s.number))
		{
			return qtrue;
		}
	}

	return qfalse;
}

// zyk: counts how many allies this player has
int zyk_number_of_allies(gentity_t *ent, qboolean in_rpg_mode)
{
	int i = 0;
	int number_of_allies = 0;

	for (i = 0; i < level.maxclients; i++)
	{
		gentity_t *allied_player = &g_entities[i];

		if (zyk_is_ally(ent,allied_player) == qtrue && (in_rpg_mode == qfalse || allied_player->client->sess.amrpgmode == 2))
			number_of_allies++;
	}

	return number_of_allies;
}

// zyk: starts the boss battle music
void zyk_start_boss_battle_music(gentity_t *ent)
{
	if (ent->client->pers.player_settings & (1 << 14)) // Custom
		trap->SetConfigstring( CS_MUSIC, "music/boss_custom.mp3" );
	else if (ent->client->pers.player_settings & (1 << 24)) // Korriban Action
		trap->SetConfigstring( CS_MUSIC, "music/kor_lite/korrib_action.mp3" );
	else if (ent->client->pers.player_settings & (1 << 25)) // MP Duel
		trap->SetConfigstring( CS_MUSIC, "music/mp/duel.mp3" );
	else // Hoth2 Action
		trap->SetConfigstring( CS_MUSIC, "music/hoth2/hoth2_action.mp3" );
}

// zyk: spawns a RPG quest boss and set his HP based in the quantity of allies the quest player has now
extern void clean_effect();
extern gentity_t *NPC_SpawnType( gentity_t *ent, char *npc_type, char *targetname, qboolean isVehicle ); // zyk: used in boss battles
extern void do_scale(gentity_t *ent, int new_size);
void spawn_boss(gentity_t *ent,int x,int y,int z,int yaw,char *boss_name,int gx,int gy,int gz,int gyaw,int guardian_mode)
{
	vec3_t player_origin;
	vec3_t player_yaw;
	vec3_t boss_spawn_point;
	gentity_t *npc_ent = NULL;
	int i = 0;
	float boss_bonus_hp = 0;

	// zyk: exploding trip mines and detpacks near the boss to prevent 1-hit-kill exploits
	if ((guardian_mode >= 1 && guardian_mode <= 7) || guardian_mode == 11 || guardian_mode >= 14)
		VectorSet(boss_spawn_point, gx, gy, gz);
	else
		VectorSet(boss_spawn_point, x, y, z);

	for (i = (MAX_CLIENTS + BODY_QUEUE_SIZE); i < level.num_entities; i++)
	{
		gentity_t *this_ent = &g_entities[i];

		if (this_ent && Q_stricmp(this_ent->classname, "laserTrap") == 0 && 
			Distance(this_ent->r.currentOrigin, boss_spawn_point) < 384.0)
		{
			this_ent->think = G_FreeEntity;
			this_ent->nextthink = level.time;
		}
		else if (this_ent && Q_stricmp(this_ent->classname, "detpack") == 0 &&
			Distance(this_ent->r.currentOrigin, boss_spawn_point) < 384.0)
		{
			this_ent->think = G_FreeEntity;
			this_ent->nextthink = level.time;
		}
	}

	// zyk: removing scale from allies
	for (i = 0; i < level.maxclients; i++)
	{
		gentity_t *allied_player = &g_entities[i];

		if (zyk_is_ally(ent,allied_player) == qtrue && allied_player->client->sess.amrpgmode == 2)
		{
			allied_player->client->pers.guardian_mode = guardian_mode;
			do_scale(allied_player, 100);
			allied_player->client->noclip = qfalse;
		}
	}

	ent->client->pers.guardian_mode = guardian_mode;

	player_origin[0] = x;
	player_origin[1] = y;
	player_origin[2] = z;
	player_yaw[0] = 0;
	player_yaw[1] = yaw;
	player_yaw[2] = 0;

	zyk_TeleportPlayer(ent,player_origin,player_yaw);

	if ((guardian_mode >= 1 && guardian_mode <= 7) || guardian_mode == 11 || guardian_mode >= 14)
		npc_ent = Zyk_NPC_SpawnType(boss_name,gx,gy,gz,gyaw);
	else
		npc_ent = NPC_SpawnType(ent,boss_name,NULL,qfalse);

	if (ent->client->pers.universe_quest_counter & (1 << 29))
	{ // zyk: Challenge Mode increases boss hp more
		boss_bonus_hp = 0.8 * (1 + zyk_number_of_allies(ent, qtrue));
	}
	else
	{
		boss_bonus_hp = 0.2 * zyk_number_of_allies(ent, qtrue);
	}

	if (npc_ent)
	{
		npc_ent->NPC->stats.health += (npc_ent->NPC->stats.health * boss_bonus_hp);
		npc_ent->client->ps.stats[STAT_MAX_HEALTH] = npc_ent->NPC->stats.health;
		npc_ent->health = npc_ent->client->ps.stats[STAT_MAX_HEALTH];

		npc_ent->client->pers.guardian_invoked_by_id = ent-g_entities;
		npc_ent->client->pers.hunter_quest_messages = 0;
		npc_ent->client->pers.light_quest_messages = 0;
		npc_ent->client->pers.light_quest_timer = level.time + 7000;
		npc_ent->client->pers.guardian_timer = level.time + 5000;
		npc_ent->client->pers.universe_quest_timer = level.time + 11000;
		npc_ent->client->pers.guardian_mode = guardian_mode;

		if (ent->client->pers.universe_quest_counter & (1 << 29))
		{ // zyk: if quest player is in Challenge Mode, bosses always use the improved version of powers (Universe Power)
			npc_ent->client->pers.quest_power_status |= (1 << 13);
		}
	}

	if (guardian_mode != 14)
	{
		clean_effect();
		zyk_NPC_Kill_f("all");
	}

	// zyk: boss battle music
	zyk_start_boss_battle_music(ent);

	// zyyk: removing noclip from the player
	ent->client->noclip = qfalse;
}

// zyk: tests if the target player can be hit by the attacker gun/saber damage, force power or special power
qboolean zyk_can_hit_target(gentity_t *attacker, gentity_t *target)
{
	if (attacker && attacker->client && target && target->client && !attacker->NPC && !target->NPC)
	{ // zyk: in a boss battle, non-quest players cannot hit quest players and vice-versa
		if (attacker->client->sess.amrpgmode == 2 && attacker->client->pers.guardian_mode > 0 && 
			(target->client->sess.amrpgmode != 2 || target->client->pers.guardian_mode == 0))
		{
			return qfalse;
		}

		if (target->client->sess.amrpgmode == 2 && target->client->pers.guardian_mode > 0 && 
			(attacker->client->sess.amrpgmode != 2 || attacker->client->pers.guardian_mode == 0))
		{
			return qfalse;
		}
	}

	return qtrue;
}

qboolean npcs_on_same_team(gentity_t *attacker, gentity_t *target)
{
	if (attacker->NPC && target->NPC && attacker->client->playerTeam == target->client->playerTeam)
	{
		return qtrue;
	}

	return qfalse;
}

qboolean zyk_unique_ability_can_hit_target(gentity_t *attacker, gentity_t *target)
{
	int i = target->s.number;

	if (attacker && target && attacker->s.number != i && target->client && target->health > 0 && zyk_can_hit_target(attacker, target) == qtrue &&
		(i > MAX_CLIENTS || (target->client->pers.connected == CON_CONNECTED && target->client->sess.sessionTeam != TEAM_SPECTATOR &&
			target->client->ps.duelInProgress == qfalse)))
	{ // zyk: target is a player or npc that can be hit by the attacker
		int is_ally = 0;

		if (i < level.maxclients && !attacker->NPC &&
			zyk_is_ally(attacker, target) == qtrue)
		{ // zyk: allies will not be hit by this power
			is_ally = 1;
		}

		if (OnSameTeam(attacker, target) == qtrue || npcs_on_same_team(attacker, target) == qtrue)
		{ // zyk: if one of them is npc, also check for allies
			is_ally = 1;
		}

		if (is_ally == 0 &&
			(attacker->client->pers.guardian_mode == target->client->pers.guardian_mode ||
			((attacker->client->pers.guardian_mode == 12 || attacker->client->pers.guardian_mode == 13) && target->NPC &&
				(Q_stricmp(target->NPC_type, "guardian_of_universe") || Q_stricmp(target->NPC_type, "quest_reborn") ||
					Q_stricmp(target->NPC_type, "quest_reborn_blue") || Q_stricmp(target->NPC_type, "quest_reborn_red") ||
					Q_stricmp(target->NPC_type, "quest_reborn_boss"))
				)
				))
		{ // zyk: target cannot be attacker ally and cannot be using Immunity Power. Also, non-quest players cannot hit quest players and his allies or bosses and vice-versa
			return qtrue;
		}
	}

	return qfalse;
}

// zyk: tests if the target entity can be hit by the attacker special power
qboolean zyk_special_power_can_hit_target(gentity_t *attacker, gentity_t *target, int i, int min_distance, int max_distance, qboolean hit_breakable, int *targets_hit)
{
	if ((*targets_hit) >= zyk_max_special_power_targets.integer)
		return qfalse;

	if (attacker->s.number != i && target && target->client && target->health > 0 && zyk_can_hit_target(attacker, target) == qtrue && 
		(i > MAX_CLIENTS || (target->client->pers.connected == CON_CONNECTED && target->client->sess.sessionTeam != TEAM_SPECTATOR && 
		 target->client->ps.duelInProgress == qfalse)))
	{ // zyk: target is a player or npc that can be hit by the attacker
		int player_distance = (int)Distance(attacker->client->ps.origin,target->client->ps.origin);

		if (player_distance > min_distance && player_distance < max_distance)
		{
			int is_ally = 0;

			if (i < level.maxclients && !attacker->NPC && 
				zyk_is_ally(attacker,target) == qtrue)
			{ // zyk: allies will not be hit by this power
				is_ally = 1;
			}

			if (OnSameTeam(attacker, target) == qtrue || npcs_on_same_team(attacker, target) == qtrue)
			{ // zyk: if one of them is npc, also check for allies
				is_ally = 1;
			}

			if (is_ally == 0 && !(target->client->pers.quest_power_status & (1 << 0)) && 
				(attacker->client->pers.guardian_mode == target->client->pers.guardian_mode || 
				  ((attacker->client->pers.guardian_mode == 12 || attacker->client->pers.guardian_mode == 13) && target->NPC && 
					(Q_stricmp(target->NPC_type, "guardian_of_universe") || Q_stricmp(target->NPC_type, "quest_reborn") || 
					Q_stricmp(target->NPC_type, "quest_reborn_blue") || Q_stricmp(target->NPC_type, "quest_reborn_red") || 
					Q_stricmp(target->NPC_type, "quest_reborn_boss"))
				  )
				))
			{ // zyk: target cannot be attacker ally and cannot be using Immunity Power. Also, non-quest players cannot hit quest players and his allies or bosses and vice-versa
				(*targets_hit)++;

				return qtrue;
			}
		}
	}
	else if (i >= MAX_CLIENTS && hit_breakable == qtrue && target && !target->client && target->health > 0 && target->takedamage == qtrue)
	{
		int entity_distance = (int)Distance(attacker->client->ps.origin,target->r.currentOrigin);

		if (entity_distance > min_distance && entity_distance < max_distance)
		{
			(*targets_hit)++;

			return qtrue;
		}
	}

	return qfalse;
}

// zyk: Earthquake
void earthquake(gentity_t *ent, int stun_time, int strength, int distance)
{
	int i = 0;
	int targets_hit = 0;

	// zyk: Universe Power
	if (ent->client->pers.quest_power_status & (1 << 13))
		distance += (distance/2);

	for ( i = 0; i < level.num_entities; i++)
	{
		gentity_t *player_ent = &g_entities[i];

		if (zyk_special_power_can_hit_target(ent, player_ent, i, 0, distance, qfalse, &targets_hit) == qtrue)
		{
			if (player_ent->client->ps.groundEntityNum != ENTITYNUM_NONE)
			{ // zyk: player can only be hit if he is on floor
				player_ent->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
				player_ent->client->ps.forceHandExtendTime = level.time + stun_time;
				player_ent->client->ps.velocity[2] += strength;
				player_ent->client->ps.forceDodgeAnim = 0;
				player_ent->client->ps.quickerGetup = qtrue;

				G_Damage(player_ent,ent,ent,NULL,NULL,strength/5,0,MOD_UNKNOWN);
			}

			if (i < level.maxclients)
			{
				G_ScreenShake(player_ent->client->ps.origin, player_ent,  10.0f, 4000, qtrue);
			}
			
			G_Sound(player_ent, CHAN_AUTO, G_SoundIndex("sound/effects/stone_break1.mp3"));
		}
	}
}

// zyk: Flame Burst
void flame_burst(gentity_t *ent, int duration)
{
	// zyk: Universe Power
	if (ent->client->pers.quest_power_status & (1 << 13))
	{
		duration += 3000;
	}

	ent->client->pers.flame_thrower = level.time + duration;
	ent->client->pers.quest_power_status |= (1 << 12);
}

// zyk: Blowing Wind
void blowing_wind(gentity_t *ent, int distance, int duration)
{
	int i = 0;
	int targets_hit = 0;

	// zyk: Universe Power
	if (ent->client->pers.quest_power_status & (1 << 13))
	{
		distance += 200;
	}

	for ( i = 0; i < level.num_entities; i++)
	{
		gentity_t *player_ent = &g_entities[i];

		if (zyk_special_power_can_hit_target(ent, player_ent, i, 0, distance, qfalse, &targets_hit) == qtrue)
		{
			player_ent->client->pers.quest_power_user3_id = ent->s.number;
			player_ent->client->pers.quest_power_status |= (1 << 8);
			player_ent->client->pers.quest_target6_timer = level.time + duration;

			// zyk: gives fall kill to the owner of this power
			player_ent->client->ps.otherKiller = ent->s.number;
			player_ent->client->ps.otherKillerTime = level.time + duration;
			player_ent->client->ps.otherKillerDebounceTime = level.time + 100;
							
			G_Sound(player_ent, CHAN_AUTO, G_SoundIndex("sound/effects/vacuum.mp3"));
		}
	}
}

// zyk: Reverse Wind
void reverse_wind(gentity_t *ent, int distance, int duration)
{
	int i = 0;
	int targets_hit = 0;

	// zyk: Universe Power
	if (ent->client->pers.quest_power_status & (1 << 13))
	{
		distance += 200;
	}

	for (i = 0; i < level.num_entities; i++)
	{
		gentity_t *player_ent = &g_entities[i];

		if (zyk_special_power_can_hit_target(ent, player_ent, i, 0, distance, qfalse, &targets_hit) == qtrue)
		{
			player_ent->client->pers.quest_power_user3_id = ent->s.number;
			player_ent->client->pers.quest_power_status |= (1 << 20);
			player_ent->client->pers.quest_target6_timer = level.time + duration;

			// zyk: gives fall kill to the owner of this power
			player_ent->client->ps.otherKiller = ent->s.number;
			player_ent->client->ps.otherKillerTime = level.time + duration;
			player_ent->client->ps.otherKillerDebounceTime = level.time + 100;

			G_Sound(player_ent, CHAN_AUTO, G_SoundIndex("sound/effects/vacuum.mp3"));
		}
	}
}

// zyk: Enemy Weakening
void enemy_nerf(gentity_t *ent, int distance)
{
	int i = 0;
	int targets_hit = 0;
	int duration = 7000;

	// zyk: Universe Power
	if (ent->client->pers.quest_power_status & (1 << 13))
	{
		duration += 2000;
	}

	for (i = 0; i < level.num_entities; i++)
	{
		gentity_t *player_ent = &g_entities[i];

		if (zyk_special_power_can_hit_target(ent, player_ent, i, 0, distance, qfalse, &targets_hit) == qtrue)
		{
			player_ent->client->pers.quest_target7_timer = level.time + duration;
			player_ent->client->pers.quest_power_status |= (1 << 21);

			G_Sound(player_ent, CHAN_AUTO, G_SoundIndex("sound/effects/woosh10.mp3"));
		}
	}
}

// zyk: Poison Mushrooms
void poison_mushrooms(gentity_t *ent, int min_distance, int max_distance)
{
	int i = 0;
	int targets_hit = 0;

	// zyk: Universe Power
	if (ent->client->pers.quest_power_status & (1 << 13))
		min_distance = 0;

	for (i = 0; i < level.num_entities; i++)
	{
		gentity_t *player_ent = &g_entities[i];

		if (zyk_special_power_can_hit_target(ent, player_ent, i, min_distance, max_distance, qfalse, &targets_hit) == qtrue && 
			(i < MAX_CLIENTS || player_ent->client->NPC_class != CLASS_VEHICLE))
		{
			player_ent->client->pers.quest_power_user2_id = ent->s.number;
			player_ent->client->pers.quest_power_status |= (1 << 4);
			player_ent->client->pers.quest_target3_timer = level.time + 1000;
			player_ent->client->pers.quest_power_hit_counter = 10;

			G_Sound(player_ent, CHAN_AUTO, G_SoundIndex("sound/effects/air_burst.mp3"));
		}
	}
}

// zyk: Chaos Power
void chaos_power(gentity_t *ent, int distance, int first_damage)
{
	int i = 0;
	int targets_hit = 0;

	for (i = 0; i < level.num_entities; i++)
	{
		gentity_t *player_ent = &g_entities[i];
					
		if (zyk_special_power_can_hit_target(ent, player_ent, i, 0, distance, qfalse, &targets_hit) == qtrue)
		{
			player_ent->client->pers.quest_power_user1_id = ent->s.number;
			player_ent->client->pers.quest_power_status |= (1 << 1);
			player_ent->client->pers.quest_power_hit_counter = 1;
			player_ent->client->pers.quest_target1_timer = level.time + 1000;

			// zyk: removing emotes to prevent exploits
			if (player_ent->client->pers.player_statuses & (1 << 1))
			{
				player_ent->client->pers.player_statuses &= ~(1 << 1);
				player_ent->client->ps.forceHandExtendTime = level.time;
			}

			if (player_ent->client->jetPackOn)
			{
				Jetpack_Off(player_ent);
			}

			// zyk: First Chaos Power hit
			player_ent->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
			player_ent->client->ps.forceHandExtendTime = level.time + 5000;
			player_ent->client->ps.velocity[2] += 150;
			player_ent->client->ps.forceDodgeAnim = 0;
			player_ent->client->ps.quickerGetup = qtrue;
			player_ent->client->ps.electrifyTime = level.time + 5000;

			// zyk: gives fall kill to the owner of this power
			player_ent->client->ps.otherKiller = ent->s.number;
			player_ent->client->ps.otherKillerTime = level.time + 5000;
			player_ent->client->ps.otherKillerDebounceTime = level.time + 100;

			G_Damage(player_ent,ent,ent,NULL,NULL,first_damage,0,MOD_UNKNOWN);
		}
	}
}

// zyk: Inner Area Damage
void inner_area_damage(gentity_t *ent, int distance, int damage)
{
	int i = 0;
	int targets_hit = 0;

	// zyk: Universe Power
	if (ent->client->pers.quest_power_status & (1 << 13))
		damage += 10;

	for (i = 0; i < level.num_entities; i++)
	{
		gentity_t *player_ent = &g_entities[i];

		if (zyk_special_power_can_hit_target(ent, player_ent, i, 0, distance, qtrue, &targets_hit) == qtrue)
		{
			if (ent->client->sess.amrpgmode == 2 && ent->client->pers.rpg_class == 8 && 
				ent->client->ps.powerups[PW_NEUTRALFLAG] > level.time && !(ent->client->pers.player_statuses & (1 << 21)) && 
				!(ent->client->pers.player_statuses & (1 << 22)) && !(ent->client->pers.player_statuses & (1 << 23)))
			{ // zyk: Magic Master Unique Skill increases damage
				G_Damage(player_ent,ent,ent,NULL,NULL,damage * 2,0,MOD_UNKNOWN);
			}
			else
			{
				G_Damage(player_ent,ent,ent,NULL,NULL,damage,0,MOD_UNKNOWN);
			}
		}
	}
}

// zyk: Lightning Dome
extern void zyk_lightning_dome_detonate( gentity_t *ent );
void lightning_dome(gentity_t *ent, int damage)
{
	gentity_t *missile;
	vec3_t origin;
	trace_t	tr;

	VectorSet(origin, ent->client->ps.origin[0], ent->client->ps.origin[1], ent->client->ps.origin[2] - 22);

	trap->Trace( &tr, ent->client->ps.origin, NULL, NULL, origin, ent->s.number, MASK_SHOT, qfalse, 0, 0);

	missile = G_Spawn();

	G_SetOrigin(missile, origin);
	//In SP the impact actually travels as a missile based on the trace fraction, but we're
	//just going to be instant. -rww

	VectorCopy( tr.plane.normal, missile->pos1 );

	if (ent->client->sess.amrpgmode == 2 && ent->client->pers.rpg_class == 3) // zyk: Armored Soldier Lightning Shield has less radius
		missile->count = 6;
	else
		missile->count = 9;

	missile->classname = "demp2_alt_proj";
	missile->s.weapon = WP_DEMP2;

	missile->think = zyk_lightning_dome_detonate;
	missile->nextthink = level.time;

	// zyk: Magic Master Unique Skill increases damage
	if (ent->client->sess.amrpgmode == 2 && ent->client->pers.rpg_class == 8 && ent->client->ps.powerups[PW_NEUTRALFLAG] > level.time &&
		!(ent->client->pers.player_statuses & (1 << 21)) && !(ent->client->pers.player_statuses & (1 << 22)) && 
		!(ent->client->pers.player_statuses & (1 << 23)))
	{
		damage *= 2;
	}

	missile->splashDamage = missile->damage = damage;
	missile->splashMethodOfDeath = missile->methodOfDeath = MOD_DEMP2;

	if (ent->client->sess.amrpgmode == 2 && ent->client->pers.rpg_class == 3) // zyk: Armored Soldier Lightning Shield has less radius
		missile->splashRadius = 512;
	else
		missile->splashRadius = 768;

	missile->r.ownerNum = ent->s.number;

	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to ever bounce
	missile->bounceCount = 0;

	if (ent->s.number < level.maxclients)
		G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/ambience/thunder_close1.mp3"));
}

// zyk: Ultra Strength. Increases damage and resistance to damage
void ultra_strength(gentity_t *ent, int duration)
{
	ent->client->pers.quest_power_status |= (1 << 3);
	ent->client->pers.quest_power2_timer = level.time + duration;

	if (ent->s.number < level.maxclients)
		G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/ambience/thunder1.mp3"));
}

// zyk: Ultra Resistance. Increases resistance to damage
void ultra_resistance(gentity_t *ent, int duration)
{
	ent->client->pers.quest_power_status |= (1 << 7);
	ent->client->pers.quest_power3_timer = level.time + duration;

	if (ent->s.number < level.maxclients)
		G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/player/enlightenment.mp3"));
}

// zyk: Immunity Power. Becomes immune against other special powers
void immunity_power(gentity_t *ent, int duration)
{
	ent->client->pers.quest_power_status |= (1 << 0);
	ent->client->pers.quest_power1_timer = level.time + duration;
	if (ent->s.number < level.maxclients)
		G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/player/boon.mp3"));
}

void zyk_quest_effect_spawn(gentity_t *ent, gentity_t *target_ent, char *targetname, char *spawnflags, char *effect_path, int start_time, int damage, int radius, int duration)
{
	gentity_t *new_ent = G_Spawn();

	if (!strstr(effect_path,".md3"))
	{// zyk: effect power
		zyk_set_entity_field(new_ent,"classname","fx_runner");
		zyk_set_entity_field(new_ent,"spawnflags",spawnflags);
		zyk_set_entity_field(new_ent,"targetname",targetname);

		if (Q_stricmp(targetname, "zyk_effect_scream") == 0)
			zyk_set_entity_field(new_ent, "origin", va("%d %d %d", (int)target_ent->r.currentOrigin[0], (int)target_ent->r.currentOrigin[1], (int)target_ent->r.currentOrigin[2] + 50));
		else
			zyk_set_entity_field(new_ent,"origin",va("%d %d %d",(int)target_ent->r.currentOrigin[0],(int)target_ent->r.currentOrigin[1],(int)target_ent->r.currentOrigin[2]));

		new_ent->s.modelindex = G_EffectIndex( effect_path );

		zyk_spawn_entity(new_ent);

		if (damage > 0)
			new_ent->splashDamage = damage;

		if (radius > 0)
			new_ent->splashRadius = radius;

		if (start_time > 0) 
			new_ent->nextthink = level.time + start_time;

		level.special_power_effects[new_ent->s.number] = ent->s.number;
		level.special_power_effects_timer[new_ent->s.number] = level.time + duration;

		if (Q_stricmp(targetname, "zyk_quest_effect_drain") == 0)
			G_Sound(new_ent, CHAN_AUTO, G_SoundIndex("sound/effects/arc_lp.wav"));

		if (Q_stricmp(targetname, "zyk_quest_effect_sand") == 0)
			ent->client->pers.quest_power_effect1_id = new_ent->s.number;
	}
	else
	{ // zyk: model power
		zyk_set_entity_field(new_ent,"classname","misc_model_breakable");
		zyk_set_entity_field(new_ent,"spawnflags",spawnflags);

		if (Q_stricmp(targetname, "zyk_tree_of_life") == 0)
			zyk_set_entity_field(new_ent, "origin", va("%d %d %d", (int)target_ent->r.currentOrigin[0], (int)target_ent->r.currentOrigin[1], (int)target_ent->r.currentOrigin[2] + 350));
		else
			zyk_set_entity_field(new_ent,"origin",va("%d %d %d",(int)target_ent->r.currentOrigin[0],(int)target_ent->r.currentOrigin[1],(int)target_ent->r.currentOrigin[2]));

		zyk_set_entity_field(new_ent,"model",effect_path);

		zyk_set_entity_field(new_ent,"targetname",targetname);

		zyk_spawn_entity(new_ent);

		level.special_power_effects[new_ent->s.number] = ent->s.number;
		level.special_power_effects_timer[new_ent->s.number] = level.time + duration;
	}
}

// zyk: used by Duelist Vertical DFA ability
void zyk_vertical_dfa_effect(gentity_t *ent)
{
	gentity_t *new_ent = G_Spawn();

	zyk_set_entity_field(new_ent, "classname", "fx_runner");
	zyk_set_entity_field(new_ent, "spawnflags", "4");
	zyk_set_entity_field(new_ent, "targetname", "zyk_vertical_dfa");

	zyk_set_entity_field(new_ent, "origin", va("%d %d %d", (int)ent->r.currentOrigin[0], (int)ent->r.currentOrigin[1], (int)ent->r.currentOrigin[2] - 20));

	new_ent->s.modelindex = G_EffectIndex("repeater/concussion");

	zyk_spawn_entity(new_ent);

	new_ent->splashDamage = 100;

	new_ent->splashRadius = 100;

	new_ent->nextthink = level.time + 900;

	level.special_power_effects[new_ent->s.number] = ent->s.number;
	level.special_power_effects_timer[new_ent->s.number] = level.time + 1550;
}

void zyk_bomb_model_think(gentity_t *ent)
{
	// zyk: bomb timer seconds to explode. Each call to this function decrease counter until it reaches 0
	ent->count--;

	if (ent->count == 0)
	{ // zyk: explodes the bomb
		zyk_quest_effect_spawn(ent->parent, ent, "zyk_timed_bomb_explosion", "4", "explosions/hugeexplosion1", 0, 530, 430, 800);

		ent->think = G_FreeEntity;
		ent->nextthink = level.time + 500;
	}
	else
	{
		G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/effects/mpalarm.wav"));
		ent->nextthink = level.time + 1000;
	}
}

void zyk_add_bomb_model(gentity_t *ent)
{
	gentity_t *new_ent = G_Spawn();

	zyk_set_entity_field(new_ent, "classname", "misc_model_breakable");
	zyk_set_entity_field(new_ent, "spawnflags", "0");
	zyk_set_entity_field(new_ent, "origin", va("%d %d %d", (int)ent->r.currentOrigin[0], (int)ent->r.currentOrigin[1], (int)ent->r.currentOrigin[2] - 20));

	zyk_set_entity_field(new_ent, "model", "models/map_objects/factory/bomb_new_deact.md3");

	zyk_set_entity_field(new_ent, "targetname", "zyk_timed_bomb");

	zyk_set_entity_field(new_ent, "count", "3");

	new_ent->parent = ent;
	new_ent->think = zyk_bomb_model_think;
	new_ent->nextthink = level.time + 1000;

	zyk_spawn_entity(new_ent);

	G_Sound(new_ent, CHAN_AUTO, G_SoundIndex("sound/effects/cloth1.mp3"));
}

void zyk_spawn_ice_element(gentity_t *ent, gentity_t *player_ent)
{
	int i = 0;
	int initial_angle = -179;

	for (i = 0; i < 4; i++)
	{
		gentity_t *new_ent = G_Spawn();

		zyk_set_entity_field(new_ent, "classname", "misc_model_breakable");
		zyk_set_entity_field(new_ent, "spawnflags", "65537");
		zyk_set_entity_field(new_ent, "origin", va("%d %d %d", (int)player_ent->r.currentOrigin[0], (int)player_ent->r.currentOrigin[1], (int)player_ent->r.currentOrigin[2]));

		zyk_set_entity_field(new_ent, "angles", va("0 %d 0", initial_angle + (i * 89)));

		zyk_set_entity_field(new_ent, "mins", "-70 -70 -70");
		zyk_set_entity_field(new_ent, "maxs", "70 70 70");

		zyk_set_entity_field(new_ent, "model", "models/map_objects/rift/crystal_wall.md3");

		zyk_set_entity_field(new_ent, "targetname", "zyk_elemental_ice");

		zyk_spawn_entity(new_ent);

		level.special_power_effects[new_ent->s.number] = ent->s.number;
		level.special_power_effects_timer[new_ent->s.number] = level.time + 4000;
	}
}

// zyk: Elemental Attack
void elemental_attack(gentity_t *ent)
{
	int i = 0;
	int targets_hit = 0;
	int min_distance = 100;

	for (i = 0; i < level.num_entities; i++)
	{
		gentity_t *player_ent = &g_entities[i];

		if (zyk_special_power_can_hit_target(ent, player_ent, i, min_distance, 500, qfalse, &targets_hit) == qtrue)
		{
			// zyk: first element, Ice
			zyk_spawn_ice_element(ent, player_ent);

			// zyk: second element, Fire
			zyk_quest_effect_spawn(ent, player_ent, "zyk_elemental_fire", "4", "env/flame_jet", 1000, 40, 35, 2500);

			// zyk: third element, Earth
			zyk_quest_effect_spawn(ent, player_ent, "zyk_elemental_earth", "4", "env/rock_smash", 2500, 40, 35, 4000);

			// zyk: fourth element, Wind
			player_ent->client->pers.quest_power_status |= (1 << 5);
			player_ent->client->pers.quest_power_hit_counter = -179;
			player_ent->client->pers.quest_target4_timer = level.time + 7000;

			G_Sound(player_ent, CHAN_AUTO, G_SoundIndex("sound/effects/glass_tumble3.wav"));
		}
	}
}

// zyk: Super Beam ability
void zyk_super_beam(gentity_t *ent)
{
	gentity_t *new_ent = G_Spawn();
	int angle_yaw = ent->client->ps.viewangles[1];

	if (angle_yaw == 0)
		angle_yaw = 1;

	zyk_set_entity_field(new_ent, "classname", "fx_runner");
	zyk_set_entity_field(new_ent, "spawnflags", "0");
	zyk_set_entity_field(new_ent, "targetname", "zyk_super_beam");

	zyk_set_entity_field(new_ent, "origin", va("%d %d %d", (int)ent->client->ps.origin[0], (int)ent->client->ps.origin[1], ((int)ent->client->ps.origin[2] + ent->client->ps.viewheight)));
	zyk_set_entity_field(new_ent, "angles", va("%d %d 0", (int)ent->client->ps.viewangles[0], angle_yaw));

	new_ent->s.modelindex = G_EffectIndex("env/hevil_bolt");

	new_ent->parent = ent;

	zyk_spawn_entity(new_ent);

	level.special_power_effects[new_ent->s.number] = ent->s.number;
	level.special_power_effects_timer[new_ent->s.number] = level.time + 2000;
}

// zyk: Force Storm ability
void zyk_force_storm(gentity_t *ent)
{
	int i = 0;

	zyk_quest_effect_spawn(ent, ent, "zyk_force_storm", "4", "env/huge_lightning", 0, 40, 120, 3000);

	for (i = 0; i < level.num_entities; i++)
	{
		gentity_t *player_ent = &g_entities[i];

		if (player_ent && player_ent->client && ent != player_ent &&
			zyk_unique_ability_can_hit_target(ent, player_ent) == qtrue &&
			Distance(ent->client->ps.origin, player_ent->client->ps.origin) < 300)
		{
			zyk_quest_effect_spawn(ent, player_ent, "zyk_force_storm", "4", "env/huge_lightning", 0, 40, 120, 3000);
		}
	}
}

// zyk: Force Scream ability
void force_scream(gentity_t *ent)
{
	zyk_quest_effect_spawn(ent, ent, "zyk_effect_scream", "4", "howler/sonic", 0, 25, 300, 6000);

	ent->client->ps.forceHandExtend = HANDEXTEND_TAUNT;
	ent->client->ps.forceDodgeAnim = BOTH_FORCE_RAGE;
	ent->client->ps.forceHandExtendTime = level.time + 4500;

	G_Sound(ent, CHAN_VOICE, G_SoundIndex("sound/chars/howler/howl.mp3"));
}

// zyk: Healing Water
void healing_water(gentity_t *ent, int heal_amount)
{
	// zyk: Universe Power
	if (ent->client->pers.quest_power_status & (1 << 13))
		heal_amount += 30;

	if ((ent->health + heal_amount) < ent->client->ps.stats[STAT_MAX_HEALTH])
		ent->health += heal_amount;
	else
		ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];

	G_Sound( ent, CHAN_ITEM, G_SoundIndex("sound/weapons/force/heal.wav") );
}

// zyk: Sleeping Flowers
void sleeping_flowers(gentity_t *ent, int stun_time, int distance)
{
	int i = 0;
	int targets_hit = 0;

	// zyk: Universe Power
	if (ent->client->pers.quest_power_status & (1 << 13))
	{
		distance += 100;
	}

	for (i = 0; i < level.num_entities; i++)
	{
		gentity_t *player_ent = &g_entities[i];

		if (zyk_special_power_can_hit_target(ent, player_ent, i, 0, distance, qfalse, &targets_hit) == qtrue)
		{
			// zyk: removing emotes to prevent exploits
			if (player_ent->client->pers.player_statuses & (1 << 1))
			{
				player_ent->client->pers.player_statuses &= ~(1 << 1);
				player_ent->client->ps.forceHandExtendTime = level.time;
			}

			player_ent->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
			player_ent->client->ps.forceHandExtendTime = level.time + stun_time;
			player_ent->client->ps.velocity[2] += 150;
			player_ent->client->ps.forceDodgeAnim = 0;
			player_ent->client->ps.quickerGetup = qtrue;

			zyk_quest_effect_spawn(ent, player_ent, "zyk_quest_effect_sleeping", "0", "force/heal2", 0, 0, 0, 800);

			G_Sound(player_ent, CHAN_AUTO, G_SoundIndex("sound/effects/air_burst.mp3"));
		}
	}
}

// zyk: Acid Water
void acid_water(gentity_t *ent, int distance, int damage)
{
	int i = 0;
	int targets_hit = 0;

	// zyk: Universe Power
	if (ent->client->pers.quest_power_status & (1 << 13))
	{
		damage += 10;
	}

	for (i = 0; i < level.num_entities; i++)
	{
		gentity_t *player_ent = &g_entities[i];

		if (zyk_special_power_can_hit_target(ent, player_ent, i, 0, distance, qfalse, &targets_hit) == qtrue)
		{
			zyk_quest_effect_spawn(ent, player_ent, "zyk_quest_effect_acid", "4", "env/acid_splash", 200, damage, 40, 9000);
		}
	}
}

// zyk Shifting Sand
void shifting_sand(gentity_t *ent, int distance)
{
	int time_to_teleport = 2000;
	int i = 0;
	int targets_hit = 0;
	int min_distance = distance;
	int enemy_dist = 0;
	gentity_t *this_enemy = NULL;

	// zyk: Universe Power
	if (ent->client->pers.quest_power_status & (1 << 13))
	{
		time_to_teleport = 1000;
	}

	for (i = 0; i < level.num_entities; i++)
	{
		gentity_t *player_ent = &g_entities[i];

		if (zyk_special_power_can_hit_target(ent, player_ent, i, 0, distance, qfalse, &targets_hit) == qtrue)
		{ // zyk: teleport to the nearest enemy
			enemy_dist = Distance(ent->client->ps.origin, player_ent->client->ps.origin);

			if (enemy_dist < min_distance)
			{
				min_distance = enemy_dist;
				this_enemy = player_ent;
			}
		}
	}

	if (this_enemy)
	{ // zyk: found an enemy
		ent->client->pers.quest_power_status |= (1 << 17);

		ent->client->pers.quest_power_user4_id = this_enemy->s.number;

		// zyk: used to bring the player back if he gets stuck
		VectorCopy(ent->client->ps.origin, ent->client->pers.teleport_angles);
	}

	ent->client->pers.quest_power5_timer = level.time + time_to_teleport;
	zyk_quest_effect_spawn(ent, ent, "zyk_quest_effect_sand", "0", "env/sand_spray", 0, 0, 0, time_to_teleport);
}

// zyk: Time Power
void time_power(gentity_t *ent, int distance, int duration)
{
	int i = 0;
	int targets_hit = 0;

	for (i = 0; i < level.num_entities; i++)
	{
		gentity_t *player_ent = &g_entities[i];
					
		if (zyk_special_power_can_hit_target(ent, player_ent, i, 0, distance, qfalse, &targets_hit) == qtrue)
		{
			player_ent->client->pers.quest_power_status |= (1 << 2);
			player_ent->client->pers.quest_target2_timer = level.time + duration;

			zyk_quest_effect_spawn(ent, player_ent, "zyk_quest_effect_time", "0", "misc/genrings", 0, 0, 0, duration);

			G_Sound(player_ent, CHAN_AUTO, G_SoundIndex("sound/effects/electric_beam_lp.wav"));
		}
	}
}

// zyk: Water Splash. Damages the targets and heals the user
void water_splash(gentity_t *ent, int distance, int damage)
{
	int i = 0;
	int targets_hit = 0;

	// zyk: Universe Power
	if (ent->client->pers.quest_power_status & (1 << 13))
	{
		damage += 5;
	}

	for (i = 0; i < level.num_entities; i++)
	{
		gentity_t *player_ent = &g_entities[i];

		if (zyk_special_power_can_hit_target(ent, player_ent, i, 0, distance, qfalse, &targets_hit) == qtrue)
		{
			zyk_quest_effect_spawn(ent, player_ent, "zyk_quest_effect_watersplash", "4", "world/waterfall3", 0, damage, 200, 2500);

			G_Sound(player_ent, CHAN_AUTO, G_SoundIndex("sound/ambience/yavin/waterfall_medium_lp.wav"));
		}
	}
}

// zyk: Rockfall
void rock_fall(gentity_t *ent, int distance, int damage)
{
	int i = 0;
	int targets_hit = 0;

	// zyk: Universe Power
	if (ent->client->pers.quest_power_status & (1 << 13))
	{
		distance += (distance/2);
	}

	for (i = 0; i < level.num_entities; i++)
	{
		gentity_t *player_ent = &g_entities[i];

		if (zyk_special_power_can_hit_target(ent, player_ent, i, 0, distance, qtrue, &targets_hit) == qtrue)
		{
			zyk_quest_effect_spawn(ent, player_ent, "zyk_quest_effect_rockfall", "4", "env/rockfall_noshake", 0, damage, 100, 8000);
		}
	}
}

// zyk: Dome of Damage
void dome_of_damage(gentity_t *ent, int distance, int damage)
{
	int i = 0;
	int targets_hit = 0;

	// zyk: Universe Power
	if (ent->client->pers.quest_power_status & (1 << 13))
	{
		distance += 100;
		zyk_quest_effect_spawn(ent, ent, "zyk_quest_effect_dome", "4", "env/dome", 1200, damage, 290, 8000);
	}

	for (i = 0; i < level.num_entities; i++)
	{
		gentity_t *player_ent = &g_entities[i];

		if (zyk_special_power_can_hit_target(ent, player_ent, i, 0, distance, qtrue, &targets_hit) == qtrue)
		{
			zyk_quest_effect_spawn(ent, player_ent, "zyk_quest_effect_dome", "4", "env/dome", 1200, damage, 290, 8000);
		}
	}
}

// zyk: Magic Shield
void magic_shield(gentity_t *ent, int duration)
{
	// zyk: Universe Power
	if (ent->client->pers.quest_power_status & (1 << 13))
	{
		duration += 1500;
	}

	ent->client->pers.quest_power_status |= (1 << 11);
	ent->client->pers.quest_power4_timer = level.time + duration;
	ent->client->invulnerableTimer = level.time + duration;
}

// zyk: Tree of Life
void tree_of_life(gentity_t *ent)
{
	ent->client->pers.quest_power_status |= (1 << 19);
	ent->client->pers.quest_power6_timer = level.time;
	ent->client->pers.quest_power_hit2_counter = 4;

	zyk_quest_effect_spawn(ent, ent, "zyk_tree_of_life", "1", "models/map_objects/yavin/tree10_b.md3", 0, 0, 0, 4000);
}

// zyk: Magic Drain
void magic_drain(gentity_t *ent, int distance)
{
	int i = 0;
	int targets_hit = 0;
	int mp_amount = 25;

	// zyk: Universe Power
	if (ent->client->pers.quest_power_status & (1 << 13))
	{
		mp_amount += 15;
	}

	for (i = 0; i < level.num_entities; i++)
	{
		gentity_t *player_ent = &g_entities[i];

		if (zyk_special_power_can_hit_target(ent, player_ent, i, 0, distance, qfalse, &targets_hit) == qtrue)
		{
			if (i < MAX_CLIENTS && player_ent->client->sess.amrpgmode == 2 && player_ent->client->pers.magic_power >= mp_amount)
			{ // zyk: rpg player, drain his mp
				player_ent->client->pers.magic_power -= mp_amount;

				if ((ent->health + mp_amount) < ent->client->ps.stats[STAT_MAX_HEALTH])
					ent->health += mp_amount;
				else
					ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];

				send_rpg_events(2000);

				G_Sound(ent, CHAN_ITEM, G_SoundIndex("sound/weapons/force/heal.wav"));
			}
			else if (player_ent->NPC && player_ent->client->pers.guardian_mode > 0)
			{ // zyk: bosses always have mp
				if ((ent->health + mp_amount) < ent->client->ps.stats[STAT_MAX_HEALTH])
					ent->health += mp_amount;
				else
					ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];

				G_Sound(ent, CHAN_ITEM, G_SoundIndex("sound/weapons/force/heal.wav"));
			}
			else if (player_ent->NPC && Q_stricmp(player_ent->NPC_type, "quest_mage") == 0)
			{ // zyk: quest_mage always have mp
				if ((ent->health + mp_amount) < ent->client->ps.stats[STAT_MAX_HEALTH])
					ent->health += mp_amount;
				else
					ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];

				G_Sound(ent, CHAN_ITEM, G_SoundIndex("sound/weapons/force/heal.wav"));
			}
			else if (player_ent->client->ps.fd.forcePower >= mp_amount)
			{ // zyk: enemy has enough force to be drained
				player_ent->client->ps.fd.forcePower -= mp_amount;

				if ((ent->health + mp_amount) < ent->client->ps.stats[STAT_MAX_HEALTH])
					ent->health += mp_amount;
				else
					ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];

				G_Sound(ent, CHAN_ITEM, G_SoundIndex("sound/weapons/force/heal.wav"));
			}
		}
	}
}

// zyk: Ice Stalagmite
void ice_stalagmite(gentity_t *ent, int distance, int damage)
{
	int i = 0;
	int targets_hit = 0;
	int min_distance = 50;

	// zyk: Universe Power
	if (ent->client->pers.quest_power_status & (1 << 13))
	{
		damage += 30;
		min_distance = 0;
	}

	for (i = 0; i < level.num_entities; i++)
	{
		gentity_t *player_ent = &g_entities[i];

		if (zyk_special_power_can_hit_target(ent, player_ent, i, min_distance, distance, qfalse, &targets_hit) == qtrue)
		{
			zyk_quest_effect_spawn(ent, player_ent, "zyk_ice_stalagmite", "0", "models/map_objects/hoth/stalagmite_small.md3", 0, 0, 0, 2000);

			G_Damage(player_ent,ent,ent,NULL,player_ent->client->ps.origin,damage,DAMAGE_NO_PROTECTION,MOD_UNKNOWN);
		}
	}
}

// zyk: Ice Boulder
void ice_boulder(gentity_t *ent, int distance, int damage)
{
	int i = 0;
	int targets_hit = 0;

	// zyk: Universe Power
	if (ent->client->pers.quest_power_status & (1 << 13))
	{
		distance += 50;
	}

	for (i = 0; i < level.num_entities; i++)
	{
		gentity_t *player_ent = &g_entities[i];

		if (zyk_special_power_can_hit_target(ent, player_ent, i, 50, distance, qfalse, &targets_hit) == qtrue)
		{
			zyk_quest_effect_spawn(ent, player_ent, "zyk_ice_boulder", "1", "models/map_objects/hoth/rock_b.md3", 0, 20, 50, 4000);

			G_Damage(player_ent,ent,ent,NULL,player_ent->client->ps.origin,damage,DAMAGE_NO_PROTECTION,MOD_UNKNOWN);
		}
	}
}

void zyk_spawn_ice_block(gentity_t *ent, int duration, int pitch, int yaw, int x_offset, int y_offset, int z_offset)
{
	gentity_t *new_ent = G_Spawn();

	zyk_set_entity_field(new_ent, "classname", "misc_model_breakable");
	zyk_set_entity_field(new_ent, "spawnflags", "65537");
	zyk_set_entity_field(new_ent, "origin", va("%d %d %d", (int)ent->r.currentOrigin[0], (int)ent->r.currentOrigin[1], (int)ent->r.currentOrigin[2]));

	zyk_set_entity_field(new_ent, "angles", va("%d %d 0", pitch, yaw));

	if (x_offset == 0 && y_offset != 0)
	{
		zyk_set_entity_field(new_ent, "mins", va("%d -50 %d", y_offset * -1, y_offset * -1));
		zyk_set_entity_field(new_ent, "maxs", va("%d 50 %d", y_offset, y_offset));
	}
	else if (x_offset != 0 && y_offset == 0)
	{
		zyk_set_entity_field(new_ent, "mins", va("-50 %d %d", x_offset * -1, x_offset * -1));
		zyk_set_entity_field(new_ent, "maxs", va("50 %d %d", x_offset, x_offset));
	}
	else if (x_offset == 0 && y_offset == 0)
	{
		zyk_set_entity_field(new_ent, "mins", va("%d %d -50", z_offset * -1, z_offset * -1));
		zyk_set_entity_field(new_ent, "maxs", va("%d %d 50", z_offset, z_offset));
	}

	zyk_set_entity_field(new_ent, "model", "models/map_objects/rift/crystal_wall.md3");

	zyk_set_entity_field(new_ent, "targetname", "zyk_ice_block");

	zyk_set_entity_field(new_ent, "zykmodelscale", "200");

	zyk_spawn_entity(new_ent);

	level.special_power_effects[new_ent->s.number] = ent->s.number;
	level.special_power_effects_timer[new_ent->s.number] = level.time + duration;
}

// zyk: Ice Block
void ice_block(gentity_t *ent, int duration)
{
	// zyk: Universe Power
	if (ent->client->pers.quest_power_status & (1 << 13))
	{
		duration += 1000;
	}

	zyk_spawn_ice_block(ent, duration, 0, 0, -140, 0, 0);
	zyk_spawn_ice_block(ent, duration, 0, 90, 140, 0, 0);
	zyk_spawn_ice_block(ent, duration, 0, 179, 0, -140, 0);
	zyk_spawn_ice_block(ent, duration, 0, -90, 0, 140, 0);
	zyk_spawn_ice_block(ent, duration, 90, 0, 0, 0, -140);
	zyk_spawn_ice_block(ent, duration, -90, 0, 0, 0, 140);

	ent->client->pers.quest_power_status |= (1 << 22);
	ent->client->pers.quest_power7_timer = level.time + duration;

	G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/effects/glass_tumble3.wav"));
}

// zyk: Ultra Drain
void ultra_drain(gentity_t *ent, int radius, int damage, int duration)
{
	zyk_quest_effect_spawn(ent, ent, "zyk_quest_effect_drain", "4", "misc/possession", 1000, damage, radius, duration);
}

// zyk: Magic Explosion
void magic_explosion(gentity_t *ent, int radius, int damage, int duration)
{
	// zyk: Universe Power
	if (ent->client->pers.quest_power_status & (1 << 13))
	{
		zyk_quest_effect_spawn(ent, ent, "zyk_quest_effect_explosion", "4", "explosions/hugeexplosion1", 1500, damage, radius, duration + 1000);
	}

	if (ent->client->sess.amrpgmode == 2 && ent->client->pers.rpg_class == 8 && ent->client->ps.powerups[PW_NEUTRALFLAG] > level.time && 
		!(ent->client->pers.player_statuses & (1 << 21)) && !(ent->client->pers.player_statuses & (1 << 22)) && 
		!(ent->client->pers.player_statuses & (1 << 23)))
	{ // zyk: Magic Master Unique Skill increases damage
		zyk_quest_effect_spawn(ent, ent, "zyk_quest_effect_explosion", "4", "explosions/hugeexplosion1", 500, damage * 2, radius, duration);
	}
	else
	{
		zyk_quest_effect_spawn(ent, ent, "zyk_quest_effect_explosion", "4", "explosions/hugeexplosion1", 500, damage, radius, duration);
	}
}

// zyk: Healing Area
void healing_area(gentity_t *ent, int damage, int duration)
{
	// zyk: Universe Power
	if (ent->client->pers.quest_power_status & (1 << 13))
	{
		damage += 1;
	}

	if (ent->client->sess.amrpgmode == 2 && ent->client->pers.rpg_class == 8 && ent->client->ps.powerups[PW_NEUTRALFLAG] > level.time && 
		!(ent->client->pers.player_statuses & (1 << 21)) && !(ent->client->pers.player_statuses & (1 << 22)))
	{ // zyk: Magic Master Unique Skill increases damage
		zyk_quest_effect_spawn(ent, ent, "zyk_quest_effect_healing", "4", "env/red_cyc", 0, damage * 2, 228, duration);
	}
	else
	{
		zyk_quest_effect_spawn(ent, ent, "zyk_quest_effect_healing", "4", "env/red_cyc", 0, damage, 228, duration);
	}
}

// zyk: Slow Motion
void slow_motion(gentity_t *ent, int distance, int duration)
{
	int i = 0;
	int targets_hit = 0;

	// zyk: Universe Power
	if (ent->client->pers.quest_power_status & (1 << 13))
	{
		duration += 3000;
	}

	for (i = 0; i < level.num_entities; i++)
	{
		gentity_t *player_ent = &g_entities[i];

		if (zyk_special_power_can_hit_target(ent, player_ent, i, 0, distance, qfalse, &targets_hit) == qtrue)
		{
			player_ent->client->pers.quest_power_status |= (1 << 6);
			player_ent->client->pers.quest_target5_timer = level.time + duration;

			G_Sound(player_ent, CHAN_AUTO, G_SoundIndex("sound/effects/woosh10.mp3"));
		}
	}
}

// zyk: Ultra Speed
void ultra_speed(gentity_t *ent, int duration)
{
	// zyk: Universe Power
	if (ent->client->pers.quest_power_status & (1 << 13))
	{
		duration += 3000;
	}

	ent->client->pers.quest_power_status |= (1 << 9);
	ent->client->pers.quest_power3_timer = level.time + duration;

	G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/effects/woosh1.mp3"));
}

// zyk: Fast and Slow
void fast_and_slow(gentity_t *ent, int distance, int duration)
{
	int i = 0;
	int targets_hit = 0;

	// zyk: Universe Power
	if (ent->client->pers.quest_power_status & (1 << 13))
	{
		duration += 2000;
	}

	for (i = 0; i < level.num_entities; i++)
	{
		gentity_t *player_ent = &g_entities[i];

		if (zyk_special_power_can_hit_target(ent, player_ent, i, 0, distance, qfalse, &targets_hit) == qtrue)
		{
			player_ent->client->pers.quest_power_status |= (1 << 6);
			player_ent->client->pers.quest_target5_timer = level.time + duration;

			G_Sound(player_ent, CHAN_AUTO, G_SoundIndex("sound/effects/woosh10.mp3"));
		}
	}

	ent->client->pers.quest_power_status |= (1 << 9);
	ent->client->pers.quest_power3_timer = level.time + duration;

	G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/effects/woosh1.mp3"));
}

// zyk: spawns the circle of fire around the player
void ultra_flame_circle(gentity_t *ent, char *targetname, char *spawnflags, char *effect_path, int start_time, int damage, int radius, int duration, int xoffset, int yoffset)
{
	gentity_t *new_ent = G_Spawn();

	zyk_set_entity_field(new_ent,"classname","fx_runner");
	zyk_set_entity_field(new_ent,"spawnflags",spawnflags);
	zyk_set_entity_field(new_ent,"targetname",targetname);
	zyk_set_entity_field(new_ent,"origin",va("%d %d %d",(int)ent->r.currentOrigin[0] + xoffset,(int)ent->r.currentOrigin[1] + yoffset,(int)ent->r.currentOrigin[2]));

	new_ent->s.modelindex = G_EffectIndex( effect_path );

	zyk_spawn_entity(new_ent);

	if (damage > 0)
		new_ent->splashDamage = damage;

	if (radius > 0)
		new_ent->splashRadius = radius;

	if (start_time > 0) 
		new_ent->nextthink = level.time + start_time;

	level.special_power_effects[new_ent->s.number] = ent->s.number;
	level.special_power_effects_timer[new_ent->s.number] = level.time + duration;
}

// zyk: Ultra Flame
void ultra_flame(gentity_t *ent, int distance, int damage)
{
	int i = 0;
	int targets_hit = 0;

	// zyk: Universe Power
	if (ent->client->pers.quest_power_status & (1 << 13))
	{
		ultra_flame_circle(ent,"zyk_quest_effect_flame","4", "env/flame_jet", 200, damage, 35, 5000, 30, 30);
		ultra_flame_circle(ent,"zyk_quest_effect_flame","4", "env/flame_jet", 200, damage, 35, 5000, -30, 30);
		ultra_flame_circle(ent,"zyk_quest_effect_flame","4", "env/flame_jet", 200, damage, 35, 5000, 30, -30);
		ultra_flame_circle(ent,"zyk_quest_effect_flame","4", "env/flame_jet", 200, damage, 35, 5000, -30, -30);
	}

	for (i = 0; i < level.num_entities; i++)
	{
		gentity_t *player_ent = &g_entities[i];

		if (zyk_special_power_can_hit_target(ent, player_ent, i, 0, distance, qfalse, &targets_hit) == qtrue)
		{
			zyk_quest_effect_spawn(ent, player_ent, "zyk_quest_effect_flame", "4", "env/flame_jet", 200, damage, 35, 25000);
		}
	}
}

// zyk: spawns the flames around the player
void flaming_area_flames(gentity_t *ent, char *targetname, char *spawnflags, char *effect_path, int start_time, int damage, int radius, int duration, int xoffset, int yoffset)
{
	gentity_t *new_ent = G_Spawn();

	zyk_set_entity_field(new_ent, "classname", "fx_runner");
	zyk_set_entity_field(new_ent, "spawnflags", spawnflags);
	zyk_set_entity_field(new_ent, "targetname", targetname);
	zyk_set_entity_field(new_ent, "origin", va("%d %d %d", (int)ent->r.currentOrigin[0] + xoffset, (int)ent->r.currentOrigin[1] + yoffset, (int)ent->r.currentOrigin[2]));

	new_ent->s.modelindex = G_EffectIndex(effect_path);

	zyk_spawn_entity(new_ent);

	if (damage > 0)
		new_ent->splashDamage = damage;

	if (radius > 0)
		new_ent->splashRadius = radius;

	if (start_time > 0)
		new_ent->nextthink = level.time + start_time;

	G_Sound(new_ent, CHAN_AUTO, G_SoundIndex("sound/effects/fire_lp.wav"));

	level.special_power_effects[new_ent->s.number] = ent->s.number;
	level.special_power_effects_timer[new_ent->s.number] = level.time + duration;
}

// zyk: Flaming Area
void flaming_area(gentity_t *ent, int damage)
{
	// zyk: Universe Power
	if (ent->client->pers.quest_power_status & (1 << 13))
	{
		damage *= 1.5;
	}

	flaming_area_flames(ent, "zyk_quest_effect_flaming_area", "4", "env/fire", 0, damage, 60, 5000, -60, -60);
	flaming_area_flames(ent, "zyk_quest_effect_flaming_area", "4", "env/fire", 0, damage, 60, 5000, -60, 0);
	flaming_area_flames(ent, "zyk_quest_effect_flaming_area", "4", "env/fire", 0, damage, 60, 5000, -60, 60);
	flaming_area_flames(ent, "zyk_quest_effect_flaming_area", "4", "env/fire", 0, damage, 60, 5000, 0, -60);
	flaming_area_flames(ent, "zyk_quest_effect_flaming_area", "4", "env/fire", 0, damage, 60, 5000, 0, 0);
	flaming_area_flames(ent, "zyk_quest_effect_flaming_area", "4", "env/fire", 0, damage, 60, 5000, 0, 60);
	flaming_area_flames(ent, "zyk_quest_effect_flaming_area", "4", "env/fire", 0, damage, 60, 5000, 50, -60);
	flaming_area_flames(ent, "zyk_quest_effect_flaming_area", "4", "env/fire", 0, damage, 60, 5000, 60, 0);
	flaming_area_flames(ent, "zyk_quest_effect_flaming_area", "4", "env/fire", 0, damage, 60, 5000, 60, 60);
}

// zyk: Hurricane
void hurricane(gentity_t *ent, int distance, int duration)
{
	int i = 0;
	int targets_hit = 0;

	// zyk: Universe Power
	if (ent->client->pers.quest_power_status & (1 << 13))
	{
		distance += 100;
		duration += 1000;
	}

	for ( i = 0; i < level.num_entities; i++)
	{
		gentity_t *player_ent = &g_entities[i];

		if (zyk_special_power_can_hit_target(ent, player_ent, i, 0, distance, qfalse, &targets_hit) == qtrue)
		{
			player_ent->client->pers.quest_power_status |= (1 << 5);
			player_ent->client->pers.quest_power_hit_counter = -179;
			player_ent->client->pers.quest_target4_timer = level.time + duration;

			// zyk: gives fall kill to the owner of this power
			player_ent->client->ps.otherKiller = ent->s.number;
			player_ent->client->ps.otherKillerTime = level.time + duration;
			player_ent->client->ps.otherKillerDebounceTime = level.time + 100;
							
			G_Sound(player_ent, CHAN_AUTO, G_SoundIndex("sound/effects/vacuum.mp3"));
		}
	}
}

// zyk: fires the Boba Fett flame thrower
void Player_FireFlameThrower( gentity_t *self )
{
	trace_t		tr;
	gentity_t	*traceEnt = NULL;

	int entityList[MAX_GENTITIES];
	int numListedEntities;
	int e = 0;
	int damage = zyk_flame_thrower_damage.integer;

	vec3_t	tfrom, tto, fwd;
	vec3_t thispush_org, a;
	vec3_t mins, maxs, fwdangles, forward, right, center;
	vec3_t		origin, dir;

	int i;
	float visionArc = 120;
	float radius = 144;

	self->client->cloakDebReduce = level.time + zyk_flame_thrower_cooldown.integer;

	// zyk: Flame Burst magic power has more damage
	if (self->client->pers.quest_power_status & (1 << 12))
	{
		damage += 2;
	}

	origin[0] = self->r.currentOrigin[0];
	origin[1] = self->r.currentOrigin[1];
	origin[2] = self->r.currentOrigin[2] + 20.0f;

	dir[0] = (-1) * self->client->ps.viewangles[0];
	dir[2] = self->client->ps.viewangles[2];
	dir[1] = (-1) * (180 - self->client->ps.viewangles[1]);

	if ((self->client->pers.flame_thrower - level.time) > 500)
		G_PlayEffectID( G_EffectIndex("boba/fthrw"), origin, dir);

	if ((self->client->pers.flame_thrower - level.time) > 1250)
		G_Sound( self, CHAN_WEAPON, G_SoundIndex("sound/effects/fire_lp") );

	//Check for a direct usage on NPCs first
	VectorCopy(self->client->ps.origin, tfrom);
	tfrom[2] += self->client->ps.viewheight;
	AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
	tto[0] = tfrom[0] + fwd[0]*radius/2;
	tto[1] = tfrom[1] + fwd[1]*radius/2;
	tto[2] = tfrom[2] + fwd[2]*radius/2;

	trap->Trace( &tr, tfrom, NULL, NULL, tto, self->s.number, MASK_PLAYERSOLID, qfalse, 0, 0 );

	VectorCopy( self->client->ps.viewangles, fwdangles );
	AngleVectors( fwdangles, forward, right, NULL );
	VectorCopy( self->client->ps.origin, center );

	for ( i = 0 ; i < 3 ; i++ ) 
	{
		mins[i] = center[i] - radius;
		maxs[i] = center[i] + radius;
	}

	numListedEntities = trap->EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	while (e < numListedEntities)
	{
		traceEnt = &g_entities[entityList[e]];

		if (traceEnt)
		{ //not in the arc, don't consider it
			if (traceEnt->client)
			{
				VectorCopy(traceEnt->client->ps.origin, thispush_org);
			}
			else
			{
				VectorCopy(traceEnt->s.pos.trBase, thispush_org);
			}

			VectorCopy(self->client->ps.origin, tto);
			tto[2] += self->client->ps.viewheight;
			VectorSubtract(thispush_org, tto, a);
			vectoangles(a, a);

			if (!InFieldOfVision(self->client->ps.viewangles, visionArc, a))
			{ //only bother with arc rules if the victim is a client
				entityList[e] = ENTITYNUM_NONE;
			}
			else if (traceEnt->client && (traceEnt->client->sess.amrpgmode == 2 || traceEnt->NPC) && 
					 self->client->pers.quest_power_status & (1 << 12) && traceEnt->client->pers.quest_power_status & (1 << 0))
			{ // zyk: Immunity Power protects from Flame Burst
				entityList[e] = ENTITYNUM_NONE;
			}
		}
		traceEnt = &g_entities[entityList[e]];
		if (traceEnt && traceEnt != self)
		{
			G_Damage( traceEnt, self, self, self->client->ps.viewangles, tr.endpos, damage, DAMAGE_NO_KNOCKBACK|DAMAGE_IGNORE_TEAM, MOD_LAVA );
		}
		e++;
	}
}

// zyk: clear effects of some special powers
void clear_special_power_effect(gentity_t *ent)
{
	if (level.special_power_effects[ent->s.number] != -1 && level.special_power_effects_timer[ent->s.number] < level.time)
	{ 
		level.special_power_effects[ent->s.number] = -1;

		// zyk: if it is a misc_model_breakable power, remove it right now
		if (Q_stricmp(ent->classname, "misc_model_breakable") == 0)
			G_FreeEntity(ent);
		else
			ent->think = G_FreeEntity;
	}
}

qboolean magic_master_has_this_power(gentity_t *ent, int selected_power)
{
	if (selected_power == MAGIC_HEALING_WATER && !(ent->client->pers.defeated_guardians & (1 << 4)) &&
		ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS)
	{
		return qfalse;
	}
	else if (selected_power == MAGIC_WATER_SPLASH && !(ent->client->pers.defeated_guardians & (1 << 4)) &&
		ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS)
	{
		return qfalse;
	}
	else if (selected_power == MAGIC_ACID_WATER && !(ent->client->pers.defeated_guardians & (1 << 4)) &&
		ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS)
	{
		return qfalse;
	}
	else if (selected_power == MAGIC_EARTHQUAKE && !(ent->client->pers.defeated_guardians & (1 << 5)) &&
		ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS)
	{
		return qfalse;
	}
	else if (selected_power == MAGIC_ROCKFALL && !(ent->client->pers.defeated_guardians & (1 << 5)) &&
		ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS)
	{
		return qfalse;
	}
	else if (selected_power == MAGIC_SHIFTING_SAND && !(ent->client->pers.defeated_guardians & (1 << 5)) &&
		ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS)
	{
		return qfalse;
	}
	else if (selected_power == MAGIC_SLEEPING_FLOWERS && !(ent->client->pers.defeated_guardians & (1 << 6)) &&
		ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS)
	{
		return qfalse;
	}
	else if (selected_power == MAGIC_POISON_MUSHROOMS && !(ent->client->pers.defeated_guardians & (1 << 6)) &&
		ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS)
	{
		return qfalse;
	}
	else if (selected_power == MAGIC_TREE_OF_LIFE && !(ent->client->pers.defeated_guardians & (1 << 6)) &&
		ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS)
	{
		return qfalse;
	}
	else if (selected_power == MAGIC_MAGIC_SHIELD && !(ent->client->pers.defeated_guardians & (1 << 7)) &&
		ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS)
	{
		return qfalse;
	}
	else if (selected_power == MAGIC_DOME_OF_DAMAGE && !(ent->client->pers.defeated_guardians & (1 << 7)) &&
		ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS)
	{
		return qfalse;
	}
	else if (selected_power == MAGIC_MAGIC_DRAIN && !(ent->client->pers.defeated_guardians & (1 << 7)) &&
		ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS)
	{
		return qfalse;
	}
	else if (selected_power == MAGIC_ULTRA_SPEED && !(ent->client->pers.defeated_guardians & (1 << 8)) &&
		ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS)
	{
		return qfalse;
	}
	else if (selected_power == MAGIC_SLOW_MOTION && !(ent->client->pers.defeated_guardians & (1 << 8)) &&
		ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS)
	{
		return qfalse;
	}
	else if (selected_power == MAGIC_FAST_AND_SLOW && !(ent->client->pers.defeated_guardians & (1 << 8)) &&
		ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS)
	{
		return qfalse;
	}
	else if (selected_power == MAGIC_FLAME_BURST && !(ent->client->pers.defeated_guardians & (1 << 9)) &&
		ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS)
	{
		return qfalse;
	}
	else if (selected_power == MAGIC_ULTRA_FLAME && !(ent->client->pers.defeated_guardians & (1 << 9)) &&
		ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS)
	{
		return qfalse;
	}
	else if (selected_power == MAGIC_FLAMING_AREA && !(ent->client->pers.defeated_guardians & (1 << 9)) &&
		ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS)
	{
		return qfalse;
	}
	else if (selected_power == MAGIC_BLOWING_WIND && !(ent->client->pers.defeated_guardians & (1 << 10)) &&
		ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS)
	{
		return qfalse;
	}
	else if (selected_power == MAGIC_HURRICANE && !(ent->client->pers.defeated_guardians & (1 << 10)) &&
		ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS)
	{
		return qfalse;
	}
	else if (selected_power == MAGIC_REVERSE_WIND && !(ent->client->pers.defeated_guardians & (1 << 10)) &&
		ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS)
	{
		return qfalse;
	}
	else if (selected_power == MAGIC_ULTRA_RESISTANCE && !(ent->client->pers.defeated_guardians & (1 << 11)) &&
		ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS)
	{
		return qfalse;
	}
	else if (selected_power == MAGIC_ULTRA_STRENGTH && !(ent->client->pers.defeated_guardians & (1 << 11)) &&
		ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS)
	{
		return qfalse;
	}
	else if (selected_power == MAGIC_ENEMY_WEAKENING && !(ent->client->pers.defeated_guardians & (1 << 11)) &&
		ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS)
	{
		return qfalse;
	}
	else if (selected_power == MAGIC_ICE_STALAGMITE && !(ent->client->pers.defeated_guardians & (1 << 12)) &&
		ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS)
	{
		return qfalse;
	}
	else if (selected_power == MAGIC_ICE_BOULDER && !(ent->client->pers.defeated_guardians & (1 << 12)) &&
		ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS)
	{
		return qfalse;
	}
	else if (selected_power == MAGIC_ICE_BLOCK && !(ent->client->pers.defeated_guardians & (1 << 12)) &&
		ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS)
	{
		return qfalse;
	}
	else if (selected_power == MAGIC_HEALING_AREA && ent->client->pers.skill_levels[55] < 1)
	{
		return qfalse;
	}
	else if (selected_power == MAGIC_MAGIC_EXPLOSION && ent->client->pers.skill_levels[55] < 2)
	{
		return qfalse;
	}
	else if (selected_power == MAGIC_LIGHTNING_DOME && ent->client->pers.skill_levels[55] < 3)
	{
		return qfalse;
	}
	else if (selected_power < MAGIC_INNER_AREA_DAMAGE || selected_power >= MAX_MAGIC_POWERS)
	{ // zyk: if, for some reason, there is an invalid selected power value, does not allow it
		return qfalse;
	}
	else if (ent->client->sess.magic_master_disabled_powers & (1 << selected_power))
	{ // zyk: this power was disabled by the player
		return qfalse;
	}

	return qtrue;
}

void zyk_print_special_power(gentity_t *ent, int selected_power, char direction)
{
	if (selected_power == MAGIC_INNER_AREA_DAMAGE)
	{
		trap->SendServerCommand( ent->s.number, va("chat \"^1%c ^7Inner Area Damage   ^3MP: ^7%d\"",direction,ent->client->pers.magic_power));
	}
	else if (selected_power == MAGIC_HEALING_WATER)
	{
		trap->SendServerCommand( ent->s.number, va("chat \"^1%c ^4Healing Water       ^3MP: ^7%d\"",direction,ent->client->pers.magic_power));
	}
	else if (selected_power == MAGIC_WATER_SPLASH)
	{
		trap->SendServerCommand( ent->s.number, va("chat \"^1%c ^4Water Splash        ^3MP: ^7%d\"",direction,ent->client->pers.magic_power));
	}
	else if (selected_power == MAGIC_ACID_WATER)
	{
		trap->SendServerCommand(ent->s.number, va("chat \"^1%c ^4Acid Water           ^3MP: ^7%d\"", direction, ent->client->pers.magic_power));
	}
	else if (selected_power == MAGIC_EARTHQUAKE)
	{
		trap->SendServerCommand( ent->s.number, va("chat \"^1%c ^3Earthquake          ^3MP: ^7%d\"",direction,ent->client->pers.magic_power));
	}
	else if (selected_power == MAGIC_ROCKFALL)
	{
		trap->SendServerCommand( ent->s.number, va("chat \"^1%c ^3Rockfall            ^3MP: ^7%d\"",direction,ent->client->pers.magic_power));
	}
	else if (selected_power == MAGIC_SHIFTING_SAND)
	{
		trap->SendServerCommand(ent->s.number, va("chat \"^1%c ^3Shifting Sand        ^3MP: ^7%d\"", direction, ent->client->pers.magic_power));
	}
	else if (selected_power == MAGIC_SLEEPING_FLOWERS)
	{
		trap->SendServerCommand( ent->s.number, va("chat \"^1%c ^2Sleeping Flowers    ^3MP: ^7%d\"",direction,ent->client->pers.magic_power));
	}
	else if (selected_power == MAGIC_POISON_MUSHROOMS)
	{	
		trap->SendServerCommand( ent->s.number, va("chat \"^1%c ^2Poison Mushrooms    ^3MP: ^7%d\"",direction,ent->client->pers.magic_power));
	}
	else if (selected_power == MAGIC_TREE_OF_LIFE)
	{
		trap->SendServerCommand(ent->s.number, va("chat \"^1%c ^2Tree of Life         ^3MP: ^7%d\"", direction, ent->client->pers.magic_power));
	}
	else if (selected_power == MAGIC_MAGIC_SHIELD)
	{
		trap->SendServerCommand( ent->s.number, va("chat \"^1%c ^5Magic Shield        ^3MP: ^7%d\"",direction,ent->client->pers.magic_power));
	}
	else if (selected_power == MAGIC_DOME_OF_DAMAGE)
	{
		trap->SendServerCommand( ent->s.number, va("chat \"^1%c ^5Dome of Damage      ^3MP: ^7%d\"",direction,ent->client->pers.magic_power));
	}
	else if (selected_power == MAGIC_MAGIC_DRAIN)
	{
		trap->SendServerCommand(ent->s.number, va("chat \"^1%c ^5Magic Drain          ^3MP: ^7%d\"", direction, ent->client->pers.magic_power));
	}
	else if (selected_power == MAGIC_ULTRA_SPEED)
	{
		trap->SendServerCommand( ent->s.number, va("chat \"^1%c ^6Ultra Speed         ^3MP: ^7%d\"",direction,ent->client->pers.magic_power));
	}
	else if (selected_power == MAGIC_SLOW_MOTION)
	{
		trap->SendServerCommand( ent->s.number, va("chat \"^1%c ^6Slow Motion         ^3MP: ^7%d\"",direction,ent->client->pers.magic_power));
	}
	else if (selected_power == MAGIC_FAST_AND_SLOW)
	{
		trap->SendServerCommand(ent->s.number, va("chat \"^1%c ^6Fast and Slow        ^3MP: ^7%d\"", direction, ent->client->pers.magic_power));
	}
	else if (selected_power == MAGIC_FLAME_BURST)
	{
		trap->SendServerCommand( ent->s.number, va("chat \"^1%c ^1Flame Burst         ^3MP: ^7%d\"",direction,ent->client->pers.magic_power));
	}
	else if (selected_power == MAGIC_ULTRA_FLAME)
	{
		trap->SendServerCommand( ent->s.number, va("chat \"^1%c ^1Ultra Flame         ^3MP: ^7%d\"",direction,ent->client->pers.magic_power));
	}
	else if (selected_power == MAGIC_FLAMING_AREA)
	{
		trap->SendServerCommand(ent->s.number, va("chat \"^1%c ^1Flaming Area         ^3MP: ^7%d\"", direction, ent->client->pers.magic_power));
	}
	else if (selected_power == MAGIC_BLOWING_WIND)
	{
		trap->SendServerCommand( ent->s.number, va("chat \"^1%c ^7Blowing Wind        ^3MP: ^7%d\"",direction,ent->client->pers.magic_power));
	}
	else if (selected_power == MAGIC_HURRICANE)
	{
		trap->SendServerCommand( ent->s.number, va("chat \"^1%c ^7Hurricane           ^3MP: ^7%d\"",direction,ent->client->pers.magic_power));
	}
	else if (selected_power == MAGIC_REVERSE_WIND)
	{
		trap->SendServerCommand(ent->s.number, va("chat \"^1%c ^7Reverse Wind         ^3MP: ^7%d\"", direction, ent->client->pers.magic_power));
	}
	else if (selected_power == MAGIC_ULTRA_RESISTANCE)
	{
		trap->SendServerCommand( ent->s.number, va("chat \"^1%c ^3Ultra Resistance    ^3MP: ^7%d\"",direction,ent->client->pers.magic_power));
	}
	else if (selected_power == MAGIC_ULTRA_STRENGTH)
	{
		trap->SendServerCommand( ent->s.number, va("chat \"^1%c ^3Ultra Strength      ^3MP: ^7%d\"",direction,ent->client->pers.magic_power));
	}
	else if (selected_power == MAGIC_ENEMY_WEAKENING)
	{
		trap->SendServerCommand(ent->s.number, va("chat \"^1%c ^3Enemy Weakening      ^3MP: ^7%d\"", direction, ent->client->pers.magic_power));
	}
	else if (selected_power == MAGIC_ICE_STALAGMITE)
	{
		trap->SendServerCommand( ent->s.number, va("chat \"^1%c ^5Ice Stalagmite      ^3MP: ^7%d\"",direction,ent->client->pers.magic_power));
	}
	else if (selected_power == MAGIC_ICE_BOULDER)
	{
		trap->SendServerCommand( ent->s.number, va("chat \"^1%c ^5Ice Boulder         ^3MP: ^7%d\"",direction,ent->client->pers.magic_power));
	}
	else if (selected_power == MAGIC_ICE_BLOCK)
	{
		trap->SendServerCommand(ent->s.number, va("chat \"^1%c ^5Ice Block            ^3MP: ^7%d\"", direction, ent->client->pers.magic_power));
	}
	else if (selected_power == MAGIC_HEALING_AREA)
	{
		trap->SendServerCommand( ent->s.number, va("chat \"^1%c ^7Healing Area        ^3MP: ^7%d\"",direction,ent->client->pers.magic_power));
	}
	else if (selected_power == MAGIC_MAGIC_EXPLOSION)
	{
		trap->SendServerCommand( ent->s.number, va("chat \"^1%c ^7Magic Explosion     ^3MP: ^7%d\"",direction,ent->client->pers.magic_power));
	}
	else if (selected_power == MAGIC_LIGHTNING_DOME)
	{
		trap->SendServerCommand( ent->s.number, va("chat \"^1%c ^7Lightning Dome      ^3MP: ^7%d\"",direction,ent->client->pers.magic_power));
	}
}

// zyk: returns the amount of magic powers that are enabled with /magic command
int zyk_number_of_enabled_magic_powers(gentity_t *ent)
{
	int i = 0;
	int number_of_enabled_powers = 0;

	for (i = MAGIC_INNER_AREA_DAMAGE; i < MAX_MAGIC_POWERS; i++)
	{
		if (!(ent->client->sess.magic_master_disabled_powers & (1 << i)))
		{
			number_of_enabled_powers++;
		}
	}

	return number_of_enabled_powers;
}

void zyk_show_magic_master_powers(gentity_t *ent, qboolean next_power)
{
	if (zyk_number_of_enabled_magic_powers(ent) == 0)
	{
		return;
	}

	if (next_power == qtrue)
	{
		do
		{
			ent->client->sess.selected_special_power++;
			if (ent->client->sess.selected_special_power >= MAX_MAGIC_POWERS)
				ent->client->sess.selected_special_power = MAGIC_INNER_AREA_DAMAGE;
		} while (magic_master_has_this_power(ent, ent->client->sess.selected_special_power) == qfalse);
	}
	else
	{
		do
		{
			ent->client->sess.selected_special_power--;
			if (ent->client->sess.selected_special_power < MAGIC_INNER_AREA_DAMAGE)
				ent->client->sess.selected_special_power = MAX_MAGIC_POWERS - 1;
		} while (magic_master_has_this_power(ent, ent->client->sess.selected_special_power) == qfalse);
	}

	zyk_print_special_power(ent,ent->client->sess.selected_special_power,'^');
}

void zyk_show_left_magic_master_powers(gentity_t *ent, qboolean next_power)
{
	if (zyk_number_of_enabled_magic_powers(ent) == 0)
	{
		return;
	}

	if (next_power == qtrue)
	{
		do
		{
			ent->client->sess.selected_left_special_power++;
			if (ent->client->sess.selected_left_special_power >= MAX_MAGIC_POWERS)
				ent->client->sess.selected_left_special_power = MAGIC_INNER_AREA_DAMAGE;
		} while (magic_master_has_this_power(ent, ent->client->sess.selected_left_special_power) == qfalse);
	}
	else
	{
		do
		{
			ent->client->sess.selected_left_special_power--;
			if (ent->client->sess.selected_left_special_power < MAGIC_INNER_AREA_DAMAGE)
				ent->client->sess.selected_left_special_power = MAX_MAGIC_POWERS - 1;
		} while (magic_master_has_this_power(ent, ent->client->sess.selected_left_special_power) == qfalse);
	}

	zyk_print_special_power(ent,ent->client->sess.selected_left_special_power,'<');
}

void zyk_show_right_magic_master_powers(gentity_t *ent, qboolean next_power)
{
	if (zyk_number_of_enabled_magic_powers(ent) == 0)
	{
		return;
	}

	if (next_power == qtrue)
	{
		do
		{
			ent->client->sess.selected_right_special_power++;
			if (ent->client->sess.selected_right_special_power >= MAX_MAGIC_POWERS)
				ent->client->sess.selected_right_special_power = MAGIC_INNER_AREA_DAMAGE;
		} while (magic_master_has_this_power(ent, ent->client->sess.selected_right_special_power) == qfalse);
	}
	else
	{
		do
		{
			ent->client->sess.selected_right_special_power--;
			if (ent->client->sess.selected_right_special_power < MAGIC_INNER_AREA_DAMAGE)
				ent->client->sess.selected_right_special_power = MAX_MAGIC_POWERS - 1;
		} while (magic_master_has_this_power(ent, ent->client->sess.selected_right_special_power) == qfalse);
	}

	zyk_print_special_power(ent,ent->client->sess.selected_right_special_power,'>');
}

// zyk: controls the quest powers stuff
extern void initialize_rpg_skills(gentity_t *ent);
void quest_power_events(gentity_t *ent)
{
	if (ent && ent->client)
	{
		if (ent->health > 0)
		{
			if (ent->client->pers.quest_power_status & (1 << 0) && ent->client->pers.quest_power1_timer < level.time)
			{ // zyk: Immunity Power
				ent->client->pers.quest_power_status &= ~(1 << 0);
			}

			if (ent->client->pers.quest_power_status & (1 << 1))
			{ // zyk: Chaos Power
				if (ent->client->pers.quest_power_hit_counter > 0 && ent->client->pers.quest_target1_timer < level.time)
				{
					G_Damage(ent,&g_entities[ent->client->pers.quest_power_user1_id],&g_entities[ent->client->pers.quest_power_user1_id],NULL,NULL,80,0,MOD_UNKNOWN);
					ent->client->pers.quest_power_hit_counter--;
					ent->client->pers.quest_target1_timer = level.time + 1000;
				}
				else if (ent->client->pers.quest_power_hit_counter == 0 && ent->client->pers.quest_target1_timer < level.time)
				{
					G_Damage(ent,&g_entities[ent->client->pers.quest_power_user1_id],&g_entities[ent->client->pers.quest_power_user1_id],NULL,NULL,80,0,MOD_UNKNOWN);

					ent->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
					ent->client->ps.forceHandExtendTime = level.time + 1500;
					ent->client->ps.velocity[2] += 550;
					ent->client->ps.forceDodgeAnim = 0;
					ent->client->ps.quickerGetup = qtrue;

					ent->client->pers.quest_power_status &= ~(1 << 1);
				}
			}

			if (ent->client->pers.quest_power_status & (1 << 3) && ent->client->pers.quest_power2_timer < level.time)
			{ // zyk: Ultra Strength
				ent->client->pers.quest_power_status &= ~(1 << 3);
			}

			if (ent->client->pers.quest_power_status & (1 << 4))
			{ // zyk: Poison Mushrooms
				if (ent->client->pers.quest_power_status & (1 << 0))
				{ // zyk: testing for Immunity Power in target player
					ent->client->pers.quest_power_status &= ~(1 << 4);
				}

				if (ent->client->pers.quest_power_hit_counter > 0 && ent->client->pers.quest_target3_timer < level.time)
				{
					gentity_t *poison_mushrooms_user = &g_entities[ent->client->pers.quest_power_user2_id];

					if (poison_mushrooms_user && poison_mushrooms_user->client)
					{
						zyk_quest_effect_spawn(poison_mushrooms_user, ent, "zyk_quest_effect_poison", "0", "noghri_stick/gas_cloud", 0, 0, 0, 800);

						// zyk: Universe Power
						if (poison_mushrooms_user->client->pers.quest_power_status & (1 << 13))
							G_Damage(ent,poison_mushrooms_user,poison_mushrooms_user,NULL,NULL,35,0,MOD_UNKNOWN);
						else
							G_Damage(ent,poison_mushrooms_user,poison_mushrooms_user,NULL,NULL,32,0,MOD_UNKNOWN);
					}

					ent->client->pers.quest_power_hit_counter--;
					ent->client->pers.quest_target3_timer = level.time + 1000;
				}
				else if (ent->client->pers.quest_power_hit_counter == 0 && ent->client->pers.quest_target3_timer < level.time)
				{
					ent->client->pers.quest_power_status &= ~(1 << 4);
				}
			}

			if (ent->client->pers.quest_power_status & (1 << 5))
			{ // zyk: Hurricane
				if (ent->client->pers.quest_power_status & (1 << 0))
				{ // zyk: testing for Immunity Power in target player
					ent->client->pers.quest_power_status &= ~(1 << 5);
				}

				if (ent->client->pers.quest_target4_timer > level.time)
				{
					static vec3_t forward;
					vec3_t blow_dir, dir;

					VectorSet(blow_dir,-70,ent->client->pers.quest_power_hit_counter,0);

					AngleVectors(blow_dir, forward, NULL, NULL );

					VectorNormalize(forward);

					if (ent->client->ps.groundEntityNum != ENTITYNUM_NONE)
						VectorScale(forward,50.0,dir);
					else
						VectorScale(forward,25.0,dir);

					VectorAdd(ent->client->ps.velocity, dir, ent->client->ps.velocity);

					ent->client->pers.quest_power_hit_counter += 4;
					if (ent->client->pers.quest_power_hit_counter >= 180)
						ent->client->pers.quest_power_hit_counter = -179;
				}
				else
				{
					ent->client->pers.quest_power_status &= ~(1 << 5);
				}
			}

			if (ent->client->pers.quest_power_status & (1 << 6))
			{ // zyk: Slow Motion
				if (ent->client->pers.quest_power_status & (1 << 0))
				{ // zyk: testing for Immunity Power in target player
					ent->client->pers.quest_power_status &= ~(1 << 6);
				}

				if (ent->client->pers.quest_target5_timer < level.time)
				{ // zyk: Slow Motion run out
					ent->client->pers.quest_power_status &= ~(1 << 6);
				}
			}

			if (ent->client->pers.quest_power_status & (1 << 7) && ent->client->pers.quest_power3_timer < level.time)
			{ // zyk: Ultra Resistance
				ent->client->pers.quest_power_status &= ~(1 << 7);
			}

			if (ent->client->pers.quest_power_status & (1 << 8))
			{ // zyk: Blowing Wind
				if (ent->client->pers.quest_power_status & (1 << 0))
				{ // zyk: testing for Immunity Power in target player
					ent->client->pers.quest_power_status &= ~(1 << 8);
				}

				if (ent->client->pers.quest_target6_timer > level.time)
				{
					gentity_t *blowing_wind_user = &g_entities[ent->client->pers.quest_power_user3_id];

					if (blowing_wind_user && blowing_wind_user->client)
					{
						static vec3_t forward;
						vec3_t dir;

						AngleVectors( blowing_wind_user->client->ps.viewangles, forward, NULL, NULL );

						VectorNormalize(forward);

						if (ent->client->ps.groundEntityNum != ENTITYNUM_NONE)
							VectorScale(forward,90.0,dir);
						else
							VectorScale(forward,25.0,dir);

						VectorAdd(ent->client->ps.velocity, dir, ent->client->ps.velocity);
					}
				}
				else
				{
					ent->client->pers.quest_power_status &= ~(1 << 8);
				}
			}

			if (ent->client->pers.quest_power_status & (1 << 9) && ent->client->pers.quest_power3_timer < level.time)
			{ // zyk: Ultra Speed
				ent->client->pers.quest_power_status &= ~(1 << 9);
			}

			if (ent->client->pers.quest_power_status & (1 << 11))
			{ // zyk: Magic Shield
				if (ent->client->pers.quest_power4_timer < level.time)
				{ // zyk: Magic Shield run out
					ent->client->pers.quest_power_status &= ~(1 << 11);
				}
				else
				{
					ent->client->ps.eFlags |= EF_INVULNERABLE;
					ent->client->invulnerableTimer = ent->client->pers.quest_power4_timer;
				}
			}

			if (ent->client->pers.quest_power_status & (1 << 12))
			{ // zyk: Flame Burst
				if (ent->client->pers.flame_thrower < level.time)
				{
					ent->client->pers.quest_power_status &= ~(1 << 12);
				}
				else if (ent->client->cloakDebReduce < level.time)
				{ // zyk: fires the flame thrower
					Player_FireFlameThrower(ent);
				}
			}

			if (ent->client->pers.quest_power_status & (1 << 17))
			{ // zyk: Shifting Sand
				if (ent->client->pers.quest_power5_timer < level.time)
				{ // zyk: after this time, teleports to the new location and add effect there too
					if (Distance(ent->client->ps.origin, g_entities[ent->client->pers.quest_power_effect1_id].s.origin) < 100)
					{ // zyk: only teleports if the player is near the effect
						vec3_t origin;
						int random_x = Q_irand(0, 1);
						int random_y = Q_irand(0, 1);

						if (random_x == 0)
							random_x = -1;
						if (random_y == 0)
							random_y = -1;

						gentity_t *this_enemy = &g_entities[ent->client->pers.quest_power_user4_id];

						origin[0] = this_enemy->client->ps.origin[0] + (Q_irand(100, 150) * random_x);
						origin[1] = this_enemy->client->ps.origin[1] + (Q_irand(100, 150) * random_y);
						origin[2] = this_enemy->client->ps.origin[2] + Q_irand(20, 100);

						zyk_TeleportPlayer(ent, origin, ent->client->ps.viewangles);

						VectorCopy(ent->client->ps.origin, ent->client->pers.teleport_point);
					}

					ent->client->pers.quest_power_status &= ~(1 << 17);
					ent->client->pers.quest_power_status |= (1 << 18);

					ent->client->pers.quest_power5_timer = level.time + 4000;
					zyk_quest_effect_spawn(ent, ent, "zyk_quest_effect_sand", "0", "env/sand_spray", 0, 0, 0, 2000);
				}
			}

			if (ent->client->pers.quest_power_status & (1 << 18))
			{ // zyk: Shifting Sand after the teleport, validating if player is not suck
				if (ent->client->pers.quest_power5_timer < level.time)
				{
					if (VectorCompare(ent->client->ps.origin, ent->client->pers.teleport_point) == qtrue)
					{ // zyk: stuck, teleport back
						zyk_quest_effect_spawn(ent, ent, "zyk_quest_effect_sand", "0", "env/sand_spray", 0, 0, 0, 1000);
						zyk_TeleportPlayer(ent, ent->client->pers.teleport_angles, ent->client->ps.viewangles);
						zyk_quest_effect_spawn(ent, ent, "zyk_quest_effect_sand", "0", "env/sand_spray", 0, 0, 0, 1000);
					}

					ent->client->pers.quest_power_status &= ~(1 << 18);
				}
			}

			if (ent->client->pers.quest_power_status & (1 << 19))
			{ // zyk: Tree of Life
				if (ent->client->pers.quest_power_hit2_counter > 0)
				{
					if (ent->client->pers.quest_power6_timer < level.time)
					{
						int heal_amount = 20;

						// zyk: Universe Power
						if (ent->client->pers.quest_power_status & (1 << 13))
						{
							heal_amount = 40;
						}

						if ((ent->health + heal_amount) < ent->client->ps.stats[STAT_MAX_HEALTH])
							ent->health += heal_amount;
						else
							ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];

						ent->client->pers.quest_power_hit2_counter--;
						ent->client->pers.quest_power6_timer = level.time + 1000;

						G_Sound(ent, CHAN_ITEM, G_SoundIndex("sound/weapons/force/heal.wav"));
					}
				}
				else
				{
					ent->client->pers.quest_power_status &= ~(1 << 19);
				}
			}

			if (ent->client->pers.quest_power_status & (1 << 20))
			{ // zyk: Reverse Wind
				if (ent->client->pers.quest_power_status & (1 << 0))
				{ // zyk: testing for Immunity Power in target player
					ent->client->pers.quest_power_status &= ~(1 << 20);
				}

				if (ent->client->pers.quest_target6_timer > level.time)
				{
					gentity_t *reverse_wind_user = &g_entities[ent->client->pers.quest_power_user3_id];

					if (reverse_wind_user && reverse_wind_user->client)
					{
						vec3_t dir, forward;

						VectorSubtract(reverse_wind_user->client->ps.origin, ent->client->ps.origin, forward);
						VectorNormalize(forward);

						if (ent->client->ps.groundEntityNum != ENTITYNUM_NONE)
							VectorScale(forward, 90.0, dir);
						else
							VectorScale(forward, 25.0, dir);

						VectorAdd(ent->client->ps.velocity, dir, ent->client->ps.velocity);
					}
				}
				else
				{
					ent->client->pers.quest_power_status &= ~(1 << 20);
				}
			}

			if (ent->client->pers.quest_power_status & (1 << 21))
			{ // zyk: Enemy Weakening
				if (ent->client->pers.quest_power_status & (1 << 0))
				{ // zyk: testing for Immunity Power in target player
					ent->client->pers.quest_power_status &= ~(1 << 21);
				}

				if (ent->client->pers.quest_target7_timer < level.time)
				{
					ent->client->pers.quest_power_status &= ~(1 << 21);
				}
			}

			if (ent->client->pers.quest_power_status & (1 << 22))
			{ // zyk: Ice Block
				if (ent->client->pers.quest_power_status & (1 << 0))
				{ // zyk: testing for Immunity Power in target player
					ent->client->pers.quest_power_status &= ~(1 << 22);
				}

				if (ent->client->pers.quest_power7_timer < level.time)
				{
					ent->client->pers.quest_power_status &= ~(1 << 22);
				}
			}
		}
		else if (!ent->NPC && ent->client->pers.quest_power_status & (1 << 10) && ent->client->pers.quest_power1_timer < level.time && 
				!(ent->client->ps.eFlags & EF_DISINTEGRATION)) 
		{ // zyk: Resurrection Power
			ent->r.contents = CONTENTS_BODY;
			ent->client->ps.pm_type = PM_NORMAL;
			ent->client->ps.fallingToDeath = 0;
			ent->client->noCorpse = qtrue;
			ent->client->ps.eFlags &= ~EF_NODRAW;
			ent->client->ps.eFlags2 &= ~EF2_HELD_BY_MONSTER;
			ent->flags = 0;
			ent->die = player_die; // zyk: must set this function again
			initialize_rpg_skills(ent);
			ent->client->pers.jetpack_fuel = MAX_JETPACK_FUEL;
			ent->client->ps.jetpackFuel = 100;
			ent->client->ps.cloakFuel = 100;
			ent->client->pers.quest_power_status &= ~(1 << 10);
		}
	}
}

// zyk: damages target player with posion hits
void poison_dart_hits(gentity_t *ent)
{
	if (ent && ent->client && ent->health > 0 && ent->client->pers.player_statuses & (1 << 20) && ent->client->pers.poison_dart_hit_counter > 0 && 
		ent->client->pers.poison_dart_hit_timer < level.time)
	{
		gentity_t *poison_user = &g_entities[ent->client->pers.poison_dart_user_id];

		G_Damage(ent,poison_user,poison_user,NULL,NULL,30,0,MOD_UNKNOWN);

		ent->client->pers.poison_dart_hit_counter--;
		ent->client->pers.poison_dart_hit_timer = level.time + 1000;

		// zyk: no more do poison damage if counter is 0
		if (ent->client->pers.poison_dart_hit_counter == 0)
			ent->client->pers.player_statuses &= ~(1 << 20);
	}
}

// zyk: tests if player already finished the Revelations mission of Universe Quest
void first_second_act_objective(gentity_t *ent)
{
	int i = 0;
	int count = 0;

	for (i = 1; i < 3; i++)
	{
		if (ent->client->pers.universe_quest_counter & (1 << i))
			count++;
	}

	if (count == 2)
	{
		ent->client->pers.universe_quest_progress = 9;

		if (ent->client->pers.universe_quest_counter & (1 << 29))
		{ // zyk: if player is in Challenge Mode, do not remove this bit value
			ent->client->pers.universe_quest_counter = 0;
			ent->client->pers.universe_quest_counter |= (1 << 29);
		}
		else
		{
			ent->client->pers.universe_quest_counter = 0;
		}
	}
}

// zyk: checks if the player has already all artifacts
extern void save_account(gentity_t *ent);
extern int number_of_artifacts(gentity_t *ent);
void universe_quest_artifacts_checker(gentity_t *ent)
{
	if (number_of_artifacts(ent) == 8)
	{ // zyk: after collecting all artifacts, go to next objective
		trap->SendServerCommand( -1, va("chat \"%s^7: I have all artifacts! Now I must go to ^3yavin1b ^7to know about the mysterious voice I heard when I begun this quest.\"", ent->client->pers.netname));

		if (ent->client->pers.universe_quest_counter & (1 << 29))
		{ // zyk: if player is in Challenge Mode, do not remove this bit value
			ent->client->pers.universe_quest_counter = 0;
			ent->client->pers.universe_quest_counter |= (1 << 29);
		}
		else
			ent->client->pers.universe_quest_counter = 0;

		ent->client->pers.universe_quest_progress = 3;
		ent->client->pers.universe_quest_timer = level.time + 1000;
		ent->client->pers.universe_quest_objective_control = 4; // zyk: fourth Universe Quest objective
		ent->client->pers.universe_quest_messages = 0;
		
		save_account(ent);
	}
}

// zyk: checks if the player got all crystals
extern int number_of_crystals(gentity_t *ent);
extern void quest_get_new_player(gentity_t *ent);
void universe_crystals_check(gentity_t *ent)
{
	if (number_of_crystals(ent) == 3)
	{
		ent->client->pers.universe_quest_progress = 10;
		if (ent->client->pers.universe_quest_counter & (1 << 29))
		{ // zyk: if player is in Challenge Mode, do not remove this bit value
			ent->client->pers.universe_quest_counter = 0;
			ent->client->pers.universe_quest_counter |= (1 << 29);
		}
		else
			ent->client->pers.universe_quest_counter = 0;

		save_account(ent);
		quest_get_new_player(ent);
	}
	else
	{
		save_account(ent);
	}
}

extern void clean_note_model();
void zyk_try_get_dark_quest_note(gentity_t *ent, int note_bitvalue)
{
	if (ent->client->pers.hunter_quest_progress != NUMBER_OF_OBJECTIVES && ent->client->pers.guardian_mode == 0 && 
		!(ent->client->pers.hunter_quest_progress & (1 << note_bitvalue)) && ent->client->pers.can_play_quest == 1 &&
		level.quest_note_id != -1 && (int)Distance(ent->client->ps.origin, g_entities[level.quest_note_id].r.currentOrigin) < 40)
	{
		trap->SendServerCommand( ent->s.number, "chat \"^3Quest System: ^7Found an ancient note.\"");
		ent->client->pers.hunter_quest_progress |= (1 << note_bitvalue);
		clean_note_model();
		save_account(ent);
		quest_get_new_player(ent);
	}
}

// zyk: returns a value from the entity file
char *zyk_get_file_value(FILE *this_file)
{
	char content[256];

	fgets(content,sizeof(content),this_file);
	if (content[strlen(content) - 1] == '\n')
		content[strlen(content) - 1] = '\0';

	return G_NewString(content);
}

/*
================
G_RunFrame

Advances the non-player objects in the world
================
*/
void ClearNPCGlobals( void );
void AI_UpdateGroups( void );
void ClearPlayerAlertEvents( void );
void SiegeCheckTimers(void);
void WP_SaberStartMissileBlockCheck( gentity_t *self, usercmd_t *ucmd );
extern void Jedi_Cloak( gentity_t *self );
qboolean G_PointInBounds( vec3_t point, vec3_t mins, vec3_t maxs );

int g_siegeRespawnCheck = 0;
void SetMoverState( gentity_t *ent, moverState_t moverState, int time );

extern void add_credits(gentity_t *ent, int credits);
extern void remove_credits(gentity_t *ent, int credits);
extern void try_finishing_race();
extern gentity_t *load_crystal_model(int x,int y,int z, int yaw, int crystal_number);
extern void clean_crystal_model(int crystal_number);
extern qboolean dark_quest_collected_notes(gentity_t *ent);
extern qboolean light_quest_defeated_guardians(gentity_t *ent);
extern void set_max_health(gentity_t *ent);
extern void set_max_shield(gentity_t *ent);
extern gentity_t *load_effect(int x,int y,int z, int spawnflags, char *fxFile);

extern void G_Kill( gentity_t *ent );

void G_RunFrame( int levelTime ) {
	int			i;
	gentity_t	*ent;
#ifdef _G_FRAME_PERFANAL
	int			iTimer_ItemRun = 0;
	int			iTimer_ROFF = 0;
	int			iTimer_ClientEndframe = 0;
	int			iTimer_GameChecks = 0;
	int			iTimer_Queues = 0;
	void		*timer_ItemRun;
	void		*timer_ROFF;
	void		*timer_ClientEndframe;
	void		*timer_GameChecks;
	void		*timer_Queues;
#endif

	if (level.gametype == GT_SIEGE &&
		g_siegeRespawn.integer &&
		g_siegeRespawnCheck < level.time)
	{ //check for a respawn wave
		gentity_t *clEnt;
		for ( i=0; i < MAX_CLIENTS; i++ )
		{
			clEnt = &g_entities[i];

			if (clEnt->inuse && clEnt->client &&
				clEnt->client->tempSpectate >= level.time &&
				clEnt->client->sess.sessionTeam != TEAM_SPECTATOR)
			{
				ClientRespawn(clEnt);
				clEnt->client->tempSpectate = 0;
			}
		}

		g_siegeRespawnCheck = level.time + g_siegeRespawn.integer * 1000;
	}

	if (gDoSlowMoDuel)
	{
		if (level.restarted)
		{
			char buf[128];
			float tFVal = 0;

			trap->Cvar_VariableStringBuffer("timescale", buf, sizeof(buf));

			tFVal = atof(buf);

			trap->Cvar_Set("timescale", "1");
			if (tFVal == 1.0f)
			{
				gDoSlowMoDuel = qfalse;
			}
		}
		else
		{
			float timeDif = (level.time - gSlowMoDuelTime); //difference in time between when the slow motion was initiated and now
			float useDif = 0; //the difference to use when actually setting the timescale

			if (timeDif < 150)
			{
				trap->Cvar_Set("timescale", "0.1f");
			}
			else if (timeDif < 1150)
			{
				useDif = (timeDif/1000); //scale from 0.1 up to 1
				if (useDif < 0.1f)
				{
					useDif = 0.1f;
				}
				if (useDif > 1.0f)
				{
					useDif = 1.0f;
				}
				trap->Cvar_Set("timescale", va("%f", useDif));
			}
			else
			{
				char buf[128];
				float tFVal = 0;

				trap->Cvar_VariableStringBuffer("timescale", buf, sizeof(buf));

				tFVal = atof(buf);

				trap->Cvar_Set("timescale", "1");
				if (timeDif > 1500 && tFVal == 1.0f)
				{
					gDoSlowMoDuel = qfalse;
				}
			}
		}
	}

	// if we are waiting for the level to restart, do nothing
	if ( level.restarted ) {
		return;
	}

	level.framenum++;
	level.previousTime = level.time;
	level.time = levelTime;

	if (g_allowNPC.integer)
	{
		NAV_CheckCalcPaths();
	}

	AI_UpdateGroups();

	if (g_allowNPC.integer)
	{
		if ( d_altRoutes.integer )
		{
			trap->Nav_CheckAllFailedEdges();
		}
		trap->Nav_ClearCheckedNodes();

		//remember last waypoint, clear current one
		for ( i = 0; i < level.num_entities ; i++)
		{
			ent = &g_entities[i];

			if ( !ent->inuse )
				continue;

			if ( ent->waypoint != WAYPOINT_NONE
				&& ent->noWaypointTime < level.time )
			{
				ent->lastWaypoint = ent->waypoint;
				ent->waypoint = WAYPOINT_NONE;
			}
			if ( d_altRoutes.integer )
			{
				trap->Nav_CheckFailedNodes( (sharedEntity_t *)ent );
			}
		}

		//Look to clear out old events
		ClearPlayerAlertEvents();
	}

	g_TimeSinceLastFrame = (level.time - g_LastFrameTime);

	// get any cvar changes
	G_UpdateCvars();



#ifdef _G_FRAME_PERFANAL
	trap->PrecisionTimer_Start(&timer_ItemRun);
#endif

	if (level.boss_battle_music_reset_timer > 0 && level.boss_battle_music_reset_timer < level.time)
	{ // zyk: resets music to default one
		level.boss_battle_music_reset_timer = 0;
		trap->SetConfigstring( CS_MUSIC, G_NewString(level.default_map_music) );
	}

	if (level.race_mode == 1 && level.race_start_timer < level.time)
	{ // zyk: Race Mode. Tests if we should start the race
		level.race_countdown = 3;
		level.race_countdown_timer = level.time;
		level.race_last_player_position = 0;
		level.race_mode = 2;
	}
	else if (level.race_mode == 2 && level.race_countdown_timer < level.time)
	{ // zyk: Race Mode. Shows the countdown messages in players screens and starts the race
		level.race_countdown_timer = level.time + 1000;

		if (level.race_countdown > 0)
		{
			trap->SendServerCommand( -1, va("cp \"^7Race starts in ^3%d\"", level.race_countdown));
			level.race_countdown--;
		}
		else if (level.race_countdown == 0)
		{
			level.race_mode = 3;
			trap->SendServerCommand( -1, "cp \"^2Go!\"");
		}
	}

	// zyk: Guardian of Map abilities
	if (level.guardian_quest > 0)
	{
		gentity_t *npc_ent = &g_entities[level.guardian_quest];

		if (npc_ent && npc_ent->client && npc_ent->health > 0)
		{		
			npc_ent->client->ps.stats[STAT_WEAPONS] = level.initial_map_guardian_weapons;

			if (npc_ent->client->pers.hunter_quest_timer < level.time)
			{
				if (npc_ent->client->pers.hunter_quest_messages == 0)
				{
					lightning_dome(npc_ent,90);
					trap->SendServerCommand( -1, "chat \"^3Guardian of Map: ^7Lightning Dome!\"");
					npc_ent->client->pers.hunter_quest_messages++;
				}
				else if (npc_ent->client->pers.hunter_quest_messages == 1)
				{
					inner_area_damage(npc_ent,400,120);
					trap->SendServerCommand( -1, "chat \"^3Guardian of Map: ^7Inner Area Damage!\"");
					npc_ent->client->pers.hunter_quest_messages++;
				}
				else if (npc_ent->client->pers.hunter_quest_messages == 2)
				{
					healing_area(npc_ent,5,10000);
					trap->SendServerCommand( -1, "chat \"^3Guardian of Map: ^7Healing Area!\"");
					npc_ent->client->pers.hunter_quest_messages = 0;
				}

				npc_ent->client->pers.hunter_quest_timer = level.time + Q_irand(5000,10000);
			}
		}
	}

	if (level.load_entities_timer != 0 && level.load_entities_timer < level.time)
	{ // zyk: loading entities from the file specified in entload command
		char content[256];
		int x = 0, y = 0, z = 0;
		FILE *this_file = NULL;

		strcpy(content,"");

		// zyk: loading the entities from the file
		this_file = fopen(level.load_entities_file,"r");

		if (this_file != NULL)
		{
			while (fgets(content,sizeof(content),this_file) != NULL)
			{
				gentity_t *new_ent = G_Spawn();

				if (content[strlen(content) - 1] == '\n')
					content[strlen(content) - 1] = '\0';

				if (new_ent)
				{
					if (strncmp(content, "info_player", 11) == 0 || Q_stricmp(content, "info_notnull") == 0 || 
						Q_stricmp(content, "info_null") == 0 || Q_stricmp(content, "func_group") == 0 || 
						strncmp(content, "team_", 5) == 0)
					{
						float fx, fy, fz;

						zyk_set_entity_field(new_ent,"classname",va("%s", content));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target",zyk_get_file_value(this_file));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "target_position") == 0)
					{
						zyk_set_entity_field(new_ent,"classname","target_position");

						x = atoi(zyk_get_file_value(this_file));
						y = atoi(zyk_get_file_value(this_file));
						z = atoi(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%d %d %d",x,y,z));

						x = atoi(zyk_get_file_value(this_file));
						y = atoi(zyk_get_file_value(this_file));
						z = atoi(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles",va("%d %d %d",x,y,z));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "target_push") == 0)
					{
						float fx, fy, fz;

						zyk_set_entity_field(new_ent,"classname","target_push");

						x = atoi(zyk_get_file_value(this_file));
						y = atoi(zyk_get_file_value(this_file));
						z = atoi(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%d %d %d",x,y,z));

						x = atoi(zyk_get_file_value(this_file));
						y = atoi(zyk_get_file_value(this_file));
						z = atoi(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles",va("%d %d %d",x,y,z));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"speed",zyk_get_file_value(this_file));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"mins",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"maxs",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target",zyk_get_file_value(this_file));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "trigger_push") == 0)
					{
						float fx, fy, fz;

						zyk_set_entity_field(new_ent,"classname","trigger_push");

						x = atoi(zyk_get_file_value(this_file));
						y = atoi(zyk_get_file_value(this_file));
						z = atoi(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%d %d %d",x,y,z));

						x = atoi(zyk_get_file_value(this_file));
						y = atoi(zyk_get_file_value(this_file));
						z = atoi(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles",va("%d %d %d",x,y,z));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"speed",zyk_get_file_value(this_file));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"mins",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"maxs",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"wait",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target",zyk_get_file_value(this_file));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "misc_bsp") == 0)
					{
						float fx, fy, fz;

						zyk_set_entity_field(new_ent,"classname",va("%s", content));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"message",zyk_get_file_value(this_file));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "target_print") == 0)
					{
						float fx, fy, fz;

						zyk_set_entity_field(new_ent,"classname",va("%s", content));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"message",zyk_get_file_value(this_file));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "target_speaker") == 0)
					{
						zyk_set_entity_field(new_ent,"classname","target_speaker");

						x = atoi(zyk_get_file_value(this_file));
						y = atoi(zyk_get_file_value(this_file));
						z = atoi(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%d %d %d",x,y,z));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"message",zyk_get_file_value(this_file));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "target_play_music") == 0)
					{
						float fx, fy, fz;

						zyk_set_entity_field(new_ent,"classname",va("%s", content));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"message",zyk_get_file_value(this_file));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "target_random") == 0)
					{
						float fx, fy, fz;

						zyk_set_entity_field(new_ent,"classname",va("%s", content));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target2",zyk_get_file_value(this_file));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "target_relay") == 0)
					{
						float fx, fy, fz;

						zyk_set_entity_field(new_ent,"classname",va("%s", content));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target2",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"wait",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetshadername",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetshadernewname",zyk_get_file_value(this_file));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "target_counter") == 0)
					{
						zyk_set_entity_field(new_ent,"classname","target_counter");

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target2",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"count",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"bouncecount",zyk_get_file_value(this_file));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "target_delay") == 0)
					{
						zyk_set_entity_field(new_ent,"classname","target_delay");

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target2",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"wait",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"random",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetshadername",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetshadernewname",zyk_get_file_value(this_file));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "target_kill") == 0)
					{
						float fx, fy, fz;

						zyk_set_entity_field(new_ent,"classname",va("%s", content));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "light") == 0)
					{
						float fx, fy, fz;

						zyk_set_entity_field(new_ent,"classname",va("%s", content));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"count",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"bouncecount",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"fly_sound_debounce_time",zyk_get_file_value(this_file));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "trigger_hurt") == 0)
					{
						float fx, fy, fz;

						zyk_set_entity_field(new_ent,"classname",va("%s", content));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target",zyk_get_file_value(this_file));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"mins",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"maxs",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"wait",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"model",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"dmg",zyk_get_file_value(this_file));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "trigger_multiple") == 0)
					{
						float fx, fy, fz;

						zyk_set_entity_field(new_ent,"classname",va("%s", content));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target2",zyk_get_file_value(this_file));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"mins",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"maxs",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"wait",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"model",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"speed",va("%f",atof(zyk_get_file_value(this_file))/1000));

						zyk_set_entity_field(new_ent,"delay",va("%d",atoi(zyk_get_file_value(this_file))/1000));

						zyk_set_entity_field(new_ent,"genericvalue7",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"message",zyk_get_file_value(this_file));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "trigger_teleport") == 0 || Q_stricmp(content, "trigger_once") == 0)
					{
						float fx, fy, fz;

						zyk_set_entity_field(new_ent,"classname",va("%s", content));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target",zyk_get_file_value(this_file));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"mins",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"maxs",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"wait",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"model",zyk_get_file_value(this_file));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "trigger_always") == 0)
					{
						float fx, fy, fz;

						zyk_set_entity_field(new_ent,"classname",va("%s", content));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target2",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetshadername",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetshadernewname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"count",zyk_get_file_value(this_file));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "misc_model_breakable") == 0)
					{
						float fx, fy, fz;

						zyk_set_entity_field(new_ent,"classname","misc_model_breakable");

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target",zyk_get_file_value(this_file));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"mins",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"maxs",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"model",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"script_targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"usescript",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"zykmodelscale",zyk_get_file_value(this_file));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "misc_ammo_floor_unit") == 0 || Q_stricmp(content, "misc_shield_floor_unit") == 0 ||
							 Q_stricmp(content, "misc_model_health_power_converter") == 0 || 
							 Q_stricmp(content, "misc_model_shield_power_converter") == 0)
					{
						zyk_set_entity_field(new_ent,"classname",va("%s",content));

						x = atoi(zyk_get_file_value(this_file));
						y = atoi(zyk_get_file_value(this_file));
						z = atoi(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%d %d %d",x,y,z));

						x = atoi(zyk_get_file_value(this_file));
						y = atoi(zyk_get_file_value(this_file));
						z = atoi(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles",va("%d %d %d",x,y,z));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "emplaced_gun") == 0)
					{
						zyk_set_entity_field(new_ent,"classname","emplaced_gun");

						x = atoi(zyk_get_file_value(this_file));
						y = atoi(zyk_get_file_value(this_file));
						z = atoi(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%d %d %d",x,y,z));

						x = atoi(zyk_get_file_value(this_file));
						y = atoi(zyk_get_file_value(this_file));
						z = atoi(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles",va("%d %d %d",x,y,z));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "misc_turret") == 0 || Q_stricmp(content, "misc_turretG2") == 0)
					{
						zyk_set_entity_field(new_ent,"classname",va("%s", content));

						x = atoi(zyk_get_file_value(this_file));
						y = atoi(zyk_get_file_value(this_file));
						z = atoi(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%d %d %d",x,y,z));

						x = atoi(zyk_get_file_value(this_file));
						y = atoi(zyk_get_file_value(this_file));
						z = atoi(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles",va("%d %d %d",x,y,z));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"mass",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"dmg",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"health",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"radius",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"count",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"wait",zyk_get_file_value(this_file));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "fx_runner") == 0)
					{
						zyk_set_entity_field(new_ent,"classname","fx_runner");

						x = atoi(zyk_get_file_value(this_file));
						y = atoi(zyk_get_file_value(this_file));
						z = atoi(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%d %d %d",x,y,z));

						x = atoi(zyk_get_file_value(this_file));
						y = atoi(zyk_get_file_value(this_file));
						z = atoi(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles",va("%d %d %d",x,y,z));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target",zyk_get_file_value(this_file));

						new_ent->message = zyk_get_file_value(this_file); // zyk: used by Entity System to save the effect fxFile, so the effect is loaded properly by entload command
						new_ent->s.modelindex = G_EffectIndex( G_NewString(new_ent->message) );

						zyk_set_entity_field(new_ent,"soundset",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"splashdamage",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"splashradius",zyk_get_file_value(this_file));

						zyk_spawn_entity(new_ent);
					}
					else if (strncmp(content, "NPC_", 4) == 0 || strncmp(content, "npc_", 4) == 0)
					{
						zyk_set_entity_field(new_ent,"classname",va("%s", content));

						x = atoi(zyk_get_file_value(this_file));
						y = atoi(zyk_get_file_value(this_file));
						z = atoi(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%d %d %d",x,y,z));

						x = atoi(zyk_get_file_value(this_file));
						y = atoi(zyk_get_file_value(this_file));
						z = atoi(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles",va("%d %d %d",x,y,z));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target",zyk_get_file_value(this_file));

						new_ent->NPC_type = zyk_get_file_value(this_file);

						zyk_spawn_entity(new_ent);
					}
					else if (strncmp(content, "weapon_", 7) == 0 || strncmp(content, "ammo_", 5) == 0 || 
							 strncmp(content, "item_", 5) == 0)
					{
						zyk_set_entity_field(new_ent,"classname", va("%s", content));

						x = atoi(zyk_get_file_value(this_file));
						y = atoi(zyk_get_file_value(this_file));
						z = atoi(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%d %d %d",x,y,z));

						x = atoi(zyk_get_file_value(this_file));
						y = atoi(zyk_get_file_value(this_file));
						z = atoi(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles",va("%d %d %d",x,y,z));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"wait",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"count",zyk_get_file_value(this_file));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "fx_rain") == 0 || Q_stricmp(content, "fx_snow") == 0 ||
							 Q_stricmp(content, "fx_spacedust") == 0)
					{
						zyk_set_entity_field(new_ent,"classname",va("%s", content));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"count",zyk_get_file_value(this_file));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "zyk_weather") == 0)
					{
						zyk_set_entity_field(new_ent,"classname","zyk_weather");

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						x = atoi(zyk_get_file_value(this_file));
						y = atoi(zyk_get_file_value(this_file));
						z = atoi(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"mins",va("%d %d %d",x,y,z));

						x = atoi(zyk_get_file_value(this_file));
						y = atoi(zyk_get_file_value(this_file));
						z = atoi(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"maxs",va("%d %d %d",x,y,z));

						zyk_set_entity_field(new_ent,"message",zyk_get_file_value(this_file));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "func_door") == 0)
					{
						float fx, fy, fz;

						zyk_set_entity_field(new_ent,"classname","func_door");

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target2",zyk_get_file_value(this_file));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"mins",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"maxs",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"model",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"model2",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"soundset",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"wait",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"delay",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"speed",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"random",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"dmg",zyk_get_file_value(this_file));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles2",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"zykmodelscale",zyk_get_file_value(this_file));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "func_breakable") == 0)
					{
						float fx, fy, fz;

						zyk_set_entity_field(new_ent,"classname","func_breakable");

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target",zyk_get_file_value(this_file));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"mins",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"maxs",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"model",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"model2",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"mass",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"message",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"radius",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"material",zyk_get_file_value(this_file));

						new_ent->health = atoi(zyk_get_file_value(this_file));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles2",va("%f %f %f",fx,fy,fz));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "func_bobbing") == 0)
					{
						float fx, fy, fz;

						zyk_set_entity_field(new_ent,"classname","func_bobbing");

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target2",zyk_get_file_value(this_file));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"mins",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"maxs",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"model",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"model2",zyk_get_file_value(this_file));

						new_ent->s.pos.trTime = atoi(zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"dmg",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"speed",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"message",zyk_get_file_value(this_file));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles2",va("%f %f %f",fx,fy,fz));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "func_button") == 0)
					{
						float fx, fy, fz;

						zyk_set_entity_field(new_ent,"classname","func_button");

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target",zyk_get_file_value(this_file));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"mins",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"maxs",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"model",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"model2",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"wait",va("%f",atof(zyk_get_file_value(this_file))/1000));

						zyk_set_entity_field(new_ent,"speed",zyk_get_file_value(this_file));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles2",va("%f %f %f",fx,fy,fz));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "func_static") == 0)
					{
						float fx, fy, fz;

						zyk_set_entity_field(new_ent,"classname","func_static");

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target",zyk_get_file_value(this_file));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"mins",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"maxs",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"model",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"model2",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"message",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"zykmodelscale",zyk_get_file_value(this_file));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles2",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"script_targetname",zyk_get_file_value(this_file));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "func_glass") == 0)
					{
						float fx, fy, fz;

						zyk_set_entity_field(new_ent,"classname","func_glass");

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target",zyk_get_file_value(this_file));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"mins",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"maxs",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"model",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"model2",zyk_get_file_value(this_file));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles2",va("%f %f %f",fx,fy,fz));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "func_pendulum") == 0)
					{
						float fx, fy, fz;

						zyk_set_entity_field(new_ent,"classname","func_pendulum");

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target2",zyk_get_file_value(this_file));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"mins",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"maxs",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"model",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"model2",zyk_get_file_value(this_file));

						new_ent->s.apos.trDelta[2] = atof(zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"dmg",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"message",zyk_get_file_value(this_file));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles2",va("%f %f %f",fx,fy,fz));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "func_plat") == 0)
					{
						float fx, fy, fz;

						zyk_set_entity_field(new_ent,"classname","func_plat");

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target2",zyk_get_file_value(this_file));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"mins",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"maxs",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"model",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"model2",zyk_get_file_value(this_file));

						new_ent->random = atof(zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"dmg",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"speed",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"wait",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"message",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"soundset",zyk_get_file_value(this_file));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles2",va("%f %f %f",fx,fy,fz));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "func_rotating") == 0)
					{
						float fx, fy, fz;

						zyk_set_entity_field(new_ent,"classname","func_rotating");

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target",zyk_get_file_value(this_file));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"mins",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"maxs",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"model",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"model2",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"zykmodelscale",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"speed",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"material",zyk_get_file_value(this_file));

						new_ent->health = atoi(zyk_get_file_value(this_file));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"trdelta",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles2",va("%f %f %f",fx,fy,fz));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "func_timer") == 0)
					{
						float fx, fy, fz;

						zyk_set_entity_field(new_ent,"classname","func_timer");

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target2",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"random",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"wait",zyk_get_file_value(this_file));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "path_corner") == 0)
					{
						float fx, fy, fz;

						zyk_set_entity_field(new_ent,"classname","path_corner");

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target2",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"speed",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"wait",zyk_get_file_value(this_file));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "func_train") == 0)
					{
						float fx, fy, fz;

						zyk_set_entity_field(new_ent,"classname","func_train");

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target2",zyk_get_file_value(this_file));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"mins",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"maxs",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"model",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"model2",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"dmg",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"speed",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"wait",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"soundset",zyk_get_file_value(this_file));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles2",va("%f %f %f",fx,fy,fz));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "func_usable") == 0)
					{
						float fx, fy, fz;

						zyk_set_entity_field(new_ent,"classname","func_usable");

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target2",zyk_get_file_value(this_file));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"mins",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"maxs",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"model",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"model2",zyk_get_file_value(this_file));

						new_ent->health = atoi(zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"wait",zyk_get_file_value(this_file));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles2",va("%f %f %f",fx,fy,fz));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "func_wall") == 0)
					{
						float fx, fy, fz;

						zyk_set_entity_field(new_ent,"classname","func_wall");

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target2",zyk_get_file_value(this_file));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"mins",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"maxs",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"model",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"model2",zyk_get_file_value(this_file));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles2",va("%f %f %f",fx,fy,fz));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "target_activate") == 0)
					{
						float fx, fy, fz;

						zyk_set_entity_field(new_ent,"classname","target_activate");

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target",zyk_get_file_value(this_file));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "target_deactivate") == 0)
					{
						float fx, fy, fz;

						zyk_set_entity_field(new_ent,"classname","target_deactivate");

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"target",zyk_get_file_value(this_file));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "target_scriptrunner") == 0)
					{
						float fx, fy, fz;

						zyk_set_entity_field(new_ent,"classname","target_scriptrunner");

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"script_targetname",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"usescript",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"count",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"delay",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"wait",zyk_get_file_value(this_file));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "zyk_regen_unit") == 0)
					{
						float fx, fy, fz;

						zyk_set_entity_field(new_ent,"classname","zyk_regen_unit");

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"mins",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"maxs",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"count",zyk_get_file_value(this_file));

						zyk_set_entity_field(new_ent,"wait",zyk_get_file_value(this_file));

						zyk_spawn_entity(new_ent);
					}
					else if (Q_stricmp(content, "misc_model_ammo_rack") == 0 || Q_stricmp(content, "misc_model_gun_rack") == 0)
					{
						float fx, fy, fz;

						zyk_set_entity_field(new_ent,"classname",va("%s", content));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"origin",va("%f %f %f",fx,fy,fz));

						fx = atof(zyk_get_file_value(this_file));
						fy = atof(zyk_get_file_value(this_file));
						fz = atof(zyk_get_file_value(this_file));
						zyk_set_entity_field(new_ent,"angles",va("%f %f %f",fx,fy,fz));

						zyk_set_entity_field(new_ent,"spawnflags",zyk_get_file_value(this_file));

						zyk_spawn_entity(new_ent);
					}
				}
			}

			fclose(this_file);
		}

		// zyk: CTF need to have the flags spawned again when an entity file is loaded
		// general initialization
		G_FindTeams();

		// make sure we have flags for CTF, etc
		if( level.gametype >= GT_TEAM ) {
			G_CheckTeamItems();
		}

		level.load_entities_timer = 0;
	}

	//
	// go through all allocated objects
	//
	ent = &g_entities[0];
	for (i=0 ; i<level.num_entities ; i++, ent++) {
		if ( !ent->inuse ) {
			continue;
		}

		// clear events that are too old
		if ( level.time - ent->eventTime > EVENT_VALID_MSEC ) {
			if ( ent->s.event ) {
				ent->s.event = 0;	// &= EV_EVENT_BITS;
				if ( ent->client ) {
					ent->client->ps.externalEvent = 0;
					// predicted events should never be set to zero
					//ent->client->ps.events[0] = 0;
					//ent->client->ps.events[1] = 0;
				}
			}
			if ( ent->freeAfterEvent ) {
				// tempEntities or dropped items completely go away after their event
				if (ent->s.eFlags & EF_SOUNDTRACKER)
				{ //don't trigger the event again..
					ent->s.event = 0;
					ent->s.eventParm = 0;
					ent->s.eType = 0;
					ent->eventTime = 0;
				}
				else
				{
					G_FreeEntity( ent );
					continue;
				}
			} else if ( ent->unlinkAfterEvent ) {
				// items that will respawn will hide themselves after their pickup event
				ent->unlinkAfterEvent = qfalse;
				trap->UnlinkEntity( (sharedEntity_t *)ent );
			}
		}

		// temporary entities don't think
		if ( ent->freeAfterEvent ) {
			continue;
		}

		if ( !ent->r.linked && ent->neverFree ) {
			continue;
		}

		if ( ent->s.eType == ET_MISSILE ) {
			G_RunMissile( ent );
			continue;
		}

		if ( ent->s.eType == ET_ITEM || ent->physicsObject ) {
#if 0 //use if body dragging enabled?
			if (ent->s.eType == ET_BODY)
			{ //special case for bodies
				float grav = 3.0f;
				float mass = 0.14f;
				float bounce = 1.15f;

				G_RunExPhys(ent, grav, mass, bounce, qfalse, NULL, 0);
			}
			else
			{
				G_RunItem( ent );
			}
#else
			G_RunItem( ent );
#endif
			continue;
		}

		if ( ent->s.eType == ET_MOVER ) {
			G_RunMover( ent );
			continue;
		}

		//fix for self-deactivating areaportals in Siege
		if ( ent->s.eType == ET_MOVER && level.gametype == GT_SIEGE && level.intermissiontime)
		{
			if ( !Q_stricmp("func_door", ent->classname) && ent->moverState != MOVER_POS1 )
			{
				SetMoverState( ent, MOVER_POS1, level.time );
				if ( ent->teammaster == ent || !ent->teammaster )
				{
					trap->AdjustAreaPortalState( (sharedEntity_t *)ent, qfalse );
				}

				//stop the looping sound
				ent->s.loopSound = 0;
				ent->s.loopIsSoundset = qfalse;
			}
			continue;
		}

		clear_special_power_effect(ent);

		if ( i < MAX_CLIENTS )
		{
			G_CheckClientTimeouts ( ent );

			if (ent->client->inSpaceIndex && ent->client->inSpaceIndex != ENTITYNUM_NONE)
			{ //we're in space, check for suffocating and for exiting
                gentity_t *spacetrigger = &g_entities[ent->client->inSpaceIndex];

				if (!spacetrigger->inuse ||
					!G_PointInBounds(ent->client->ps.origin, spacetrigger->r.absmin, spacetrigger->r.absmax))
				{ //no longer in space then I suppose
                    ent->client->inSpaceIndex = 0;
				}
				else
				{ //check for suffocation
                    if (ent->client->inSpaceSuffocation < level.time)
					{ //suffocate!
						if (ent->health > 0 && ent->takedamage)
						{ //if they're still alive..
							G_Damage(ent, spacetrigger, spacetrigger, NULL, ent->client->ps.origin, Q_irand(50, 70), DAMAGE_NO_ARMOR, MOD_SUICIDE);

							if (ent->health > 0)
							{ //did that last one kill them?
								//play the choking sound
								G_EntitySound(ent, CHAN_VOICE, G_SoundIndex(va( "*choke%d.wav", Q_irand( 1, 3 ) )));

								//make them grasp their throat
								ent->client->ps.forceHandExtend = HANDEXTEND_CHOKE;
								ent->client->ps.forceHandExtendTime = level.time + 2000;
							}
						}

						ent->client->inSpaceSuffocation = level.time + Q_irand(100, 200);
					}
				}
			}

			if (ent->client->isHacking)
			{ //hacking checks
				gentity_t *hacked = &g_entities[ent->client->isHacking];
				vec3_t angDif;

				VectorSubtract(ent->client->ps.viewangles, ent->client->hackingAngles, angDif);

				//keep him in the "use" anim
				if (ent->client->ps.torsoAnim != BOTH_CONSOLE1)
				{
					G_SetAnim( ent, NULL, SETANIM_TORSO, BOTH_CONSOLE1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0 );
				}
				else
				{
					ent->client->ps.torsoTimer = 500;
				}
				ent->client->ps.weaponTime = ent->client->ps.torsoTimer;

				if (!(ent->client->pers.cmd.buttons & BUTTON_USE))
				{ //have to keep holding use
					ent->client->isHacking = 0;
					ent->client->ps.hackingTime = 0;
				}
				else if (!hacked || !hacked->inuse)
				{ //shouldn't happen, but safety first
					ent->client->isHacking = 0;
					ent->client->ps.hackingTime = 0;
				}
				else if (!G_PointInBounds( ent->client->ps.origin, hacked->r.absmin, hacked->r.absmax ))
				{ //they stepped outside the thing they're hacking, so reset hacking time
					ent->client->isHacking = 0;
					ent->client->ps.hackingTime = 0;
				}
				else if (VectorLength(angDif) > 10.0f)
				{ //must remain facing generally the same angle as when we start
					ent->client->isHacking = 0;
					ent->client->ps.hackingTime = 0;
				}
			}

			// zyk: new jetpack debounce and recharge code. It uses the new attribute jetpack_fuel in the pers struct
			//      then we scale and set it to the jetpackFuel attribute to display the fuel bar correctly to the player
			if (ent->client->jetPackOn && ent->client->jetPackDebReduce < level.time)
			{
				int jetpack_debounce_amount = 20;

				if (ent->client->sess.amrpgmode == 2)
				{ // zyk: RPG Mode jetpack skill. Each level decreases fuel debounce
					if (ent->client->pers.rpg_class == 2)
					{ // zyk: Bounty Hunter can have a more efficient jetpack
						jetpack_debounce_amount -= ((ent->client->pers.skill_levels[34] * 3) + (ent->client->pers.skill_levels[55]));
					}
					else if (ent->client->pers.rpg_class == 8)
					{ // zyk: Magic Master has the best jetpack
						jetpack_debounce_amount -= ((ent->client->pers.skill_levels[34] * 3) + (ent->client->pers.skill_levels[55] * 2));
					}
					else
					{
						jetpack_debounce_amount -= (ent->client->pers.skill_levels[34] * 3);
					}

					if (ent->client->pers.secrets_found & (1 << 17)) // zyk: Jetpack Upgrade decreases fuel usage
						jetpack_debounce_amount -= 2;
				}

				if (ent->client->pers.cmd.upmove > 0)
				{ // zyk: jetpack thrusting
					jetpack_debounce_amount *= 2;
				}

				ent->client->pers.jetpack_fuel -= jetpack_debounce_amount;

				if (ent->client->pers.jetpack_fuel <= 0)
				{ // zyk: out of fuel. Turn jetpack off
					ent->client->pers.jetpack_fuel = 0;
					Jetpack_Off(ent);
				}

				ent->client->ps.jetpackFuel = ent->client->pers.jetpack_fuel/JETPACK_SCALE;
				ent->client->jetPackDebReduce = level.time + 200; // zyk: JETPACK_DEFUEL_RATE. Original value: 200
			}

			if (ent->client->pers.race_position > 0)
			{ // zyk: Race Mode management
				if (level.race_mode == 3)
				{ // zyk: if race already started
					if (ent->client->ps.m_iVehicleNum != level.race_mode_vehicle[ent->client->pers.race_position - 1] && ent->health > 0)
					{ // zyk: if player loses his vehicle, he loses the race
						trap->SendServerCommand( -1, va("chat \"^3Race System: ^7%s ^7lost his vehicle and so he lost the race!\n\"",ent->client->pers.netname) );

						ent->client->pers.race_position = 0;

						try_finishing_race();
					}
					else if ((int) ent->client->ps.origin[0] > 3085 && (int) ent->client->ps.origin[0] < 5264 && (int) ent->client->ps.origin[1] > -11136 && (int) ent->client->ps.origin[1] < -9229)
					{ // zyk: player reached the finish line
						level.race_last_player_position++;
						ent->client->pers.race_position = 0;

						if (level.race_last_player_position == 1)
						{ // zyk: this player won the race. Send message to everyone and give his prize
							if (ent->client->sess.amrpgmode == 2)
							{ // zyk: give him credits
								add_credits(ent, 2000);
								save_account(ent);
								G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/player/pickupenergy.wav"));
								trap->SendServerCommand( -1, va("chat \"^3Race System: ^7Winner: %s^7 - Prize: 2000 Credits!\"",ent->client->pers.netname));
							}
							else
							{ // zyk: give him some stuff
								ent->client->ps.stats[STAT_WEAPONS] |= (1 << WP_BLASTER) | (1 << WP_DISRUPTOR) | (1 << WP_REPEATER);
								ent->client->ps.ammo[AMMO_BLASTER] = zyk_max_blaster_pack_ammo.integer;
								ent->client->ps.ammo[AMMO_POWERCELL] = zyk_max_power_cell_ammo.integer;
								ent->client->ps.ammo[AMMO_METAL_BOLTS] = zyk_max_metal_bolt_ammo.integer;
								ent->client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << HI_SENTRY_GUN) | (1 << HI_SEEKER);
								G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/player/pickupenergy.wav"));
								trap->SendServerCommand( -1, va("chat \"^3Race System: ^7Winner: %s^7 - Prize: Nice Stuff!\"",ent->client->pers.netname));
							}
						}
						else if (level.race_last_player_position == 2)
						{ // zyk: second place
							trap->SendServerCommand( -1, va("chat \"^3Race System: ^72nd Place - %s\"", ent->client->pers.netname));
						}
						else if (level.race_last_player_position == 3)
						{ // zyk: third place
							trap->SendServerCommand( -1, va("chat \"^3Race System: ^73rd Place - %s\"", ent->client->pers.netname));
						}
						else
						{
							trap->SendServerCommand( -1, va("chat \"^3Race System: ^7%dth Place - %s\"", level.race_last_player_position, ent->client->pers.netname));
						}

						try_finishing_race();
					}
					else if ((int) ent->client->ps.origin[0] > -14795 && (int) ent->client->ps.origin[0] < -13830 && (int) ent->client->ps.origin[1] > -11483 && (int) ent->client->ps.origin[1] < -8474)
					{ // zyk: teleporting to get through the wall
						vec3_t origin, yaw;

						origin[0] = -14785;
						origin[1] = -9252;
						origin[2] = 1848;

						yaw[0] = 0.0f;
						yaw[1] = 179.0f;
						yaw[2] = 0.0f;

						zyk_TeleportPlayer( &g_entities[ent->client->ps.m_iVehicleNum], origin, yaw);
					}
					else if ((int) ent->client->ps.origin[0] > -18845 && (int) ent->client->ps.origin[0] < -17636 && (int) ent->client->ps.origin[1] > -7530 && (int) ent->client->ps.origin[1] < -6761)
					{ // zyk: teleporting to get through the door
						vec3_t origin, yaw;

						origin[0] = -18248;
						origin[1] = -6152;
						origin[2] = 1722;

						yaw[0] = 0.0f;
						yaw[1] = 90.0f;
						yaw[2] = 0.0f;

						zyk_TeleportPlayer( &g_entities[ent->client->ps.m_iVehicleNum], origin, yaw);
					}
				}
				else if ((int) ent->client->ps.origin[0] < -4536 || (int) ent->client->ps.origin[0] > -2322 || (int) ent->client->ps.origin[1] < -22283 || (int) ent->client->ps.origin[1] > -18520)
				{ // zyk: player cant start racing before the countdown timer
					ent->client->pers.race_position = 0;
					trap->SendServerCommand( -1, va("chat \"^3Race System: ^7%s ^7lost for trying to race before it starts!\n\"",ent->client->pers.netname) );

					try_finishing_race();
				}
			}

			quest_power_events(ent);
			poison_dart_hits(ent);

			if (zyk_chat_protection_timer.integer > 0)
			{ // zyk: chat protection. If 0, it is off. If greater than 0, set the timer to protect the player
				if (ent->client->ps.eFlags & EF_TALK && ent->client->pers.chat_protection_timer == 0)
				{
					ent->client->pers.chat_protection_timer = level.time + zyk_chat_protection_timer.integer;
				}
				else if (ent->client->ps.eFlags & EF_TALK && ent->client->pers.chat_protection_timer < level.time)
				{
					ent->client->pers.player_statuses |= (1 << 5);
				}
				else if (ent->client->pers.chat_protection_timer != 0 && !(ent->client->ps.eFlags & EF_TALK))
				{
					ent->client->pers.player_statuses &= ~(1 << 5);
					ent->client->pers.chat_protection_timer = 0;
				}
			}

			if (ent->client->sess.amrpgmode == 2 && ent->client->sess.sessionTeam != TEAM_SPECTATOR)
			{ // zyk: RPG Mode skills and quests actions. Must be done if player is not at Spectator Mode
				// zyk: Weapon Upgrades
				if (ent->client->ps.weapon == WP_DISRUPTOR && ent->client->pers.skill_levels[21] == 2 && ent->client->ps.weaponTime > (weaponData[WP_DISRUPTOR].fireTime * 1.0)/1.4)
				{
					ent->client->ps.weaponTime = (weaponData[WP_DISRUPTOR].fireTime * 1.0)/1.4;
				}

				// zyk: Stealth Attacker using his Unique Skill, increase firerate of disruptor
				if (ent->client->ps.weapon == WP_DISRUPTOR && ent->client->pers.rpg_class == 5 && ent->client->pers.skill_levels[38] > 0 && 
					ent->client->ps.powerups[PW_NEUTRALFLAG] > level.time && ent->client->ps.weaponTime > (weaponData[WP_DISRUPTOR].fireTime * 1.0)/3.0)
				{
					ent->client->ps.weaponTime = (weaponData[WP_DISRUPTOR].fireTime * 1.0)/3.0;
				}

				if (ent->client->ps.weapon == WP_REPEATER && ent->client->pers.skill_levels[23] == 2 && ent->client->ps.weaponTime > weaponData[WP_REPEATER].altFireTime/2)
				{
					ent->client->ps.weaponTime = weaponData[WP_REPEATER].altFireTime/2;
				}

				// zyk: Monk class has a faster melee fireTime
				if (ent->client->pers.rpg_class == 4 && ent->client->ps.weapon == WP_MELEE && ent->client->pers.skill_levels[55] > 0 && 
					ent->client->ps.weaponTime > (weaponData[WP_MELEE].fireTime * 1.8)/(ent->client->pers.skill_levels[55] + 1))
				{
					ent->client->ps.weaponTime = (weaponData[WP_MELEE].fireTime * 1.8)/(ent->client->pers.skill_levels[55] + 1);
				}

				if (ent->client->pers.flame_thrower > level.time && ent->client->cloakDebReduce < level.time)
				{ // zyk: fires the flame thrower
					Player_FireFlameThrower(ent);
				}

				if (ent->client->pers.rpg_class == 2)
				{
					// zyk: if the bounty hunter still has sentries, give the item to him
					if (ent->client->pers.bounty_hunter_sentries > 0)
						ent->client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << HI_SENTRY_GUN);

					if (ent->client->pers.thermal_vision == qtrue && ent->client->ps.zoomMode == 0)
					{ // zyk: if the bounty hunter stops using binoculars, stop the Thermal Vision
						ent->client->pers.thermal_vision = qfalse;
						ent->client->ps.fd.forcePowersActive &= ~(1 << FP_SEE);
						ent->client->ps.fd.forcePowersKnown &= ~(1 << FP_SEE);
						ent->client->ps.fd.forcePowerLevel[FP_SEE] = FORCE_LEVEL_0;
					}
					else if (ent->client->pers.thermal_vision == qfalse && ent->client->ps.zoomMode == 2 && ent->client->pers.secrets_found & (1 << 1))
					{ // zyk: Bounty Hunter with Upgrade, activate the Thermal Vision
						ent->client->pers.thermal_vision = qtrue;
						ent->client->ps.fd.forcePowersKnown |= (1 << FP_SEE);
						ent->client->ps.fd.forcePowerLevel[FP_SEE] = FORCE_LEVEL_3;
						ent->client->ps.fd.forcePowersActive |= (1 << FP_SEE);
					}
				}
				else if (ent->client->pers.rpg_class == 4 && 
						(ent->client->pers.player_statuses & (1 << 22) || ent->client->pers.player_statuses & (1 << 23)) &&
						 ent->client->pers.monk_unique_timer < level.time)
				{ // zyk: Monk unique abilities
					int player_it = 0;
					int push_scale = 100;

					for (player_it = 0; player_it < level.num_entities; player_it++)
					{
						gentity_t *player_ent = &g_entities[player_it];

						if (player_ent && player_ent->client && ent != player_ent &&
							zyk_unique_ability_can_hit_target(ent, player_ent) == qtrue)
						{ // zyk: can only hit the target if he is not knocked down yet
							if (ent->client->pers.player_statuses & (1 << 22) && 
								Distance(ent->client->ps.origin, player_ent->client->ps.origin) < 80 &&
								player_ent->client->ps.forceHandExtend != HANDEXTEND_KNOCKDOWN)
							{ // zyk: Spin Kick
								vec3_t dir;

								VectorSubtract(player_ent->client->ps.origin, ent->client->ps.origin, dir);
								VectorNormalize(dir);

								G_Damage(player_ent, ent, ent, NULL, NULL, 22, 0, MOD_MELEE);

								// zyk: removing emotes to prevent exploits
								if (player_ent->client->pers.player_statuses & (1 << 1))
								{
									player_ent->client->pers.player_statuses &= ~(1 << 1);
									player_ent->client->ps.forceHandExtendTime = level.time;
								}

								player_ent->client->ps.velocity[0] = dir[0] * push_scale;
								player_ent->client->ps.velocity[1] = dir[1] * push_scale;
								player_ent->client->ps.velocity[2] = 250;

								player_ent->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
								player_ent->client->ps.forceHandExtendTime = level.time + 1000;
								player_ent->client->ps.forceDodgeAnim = 0;
								player_ent->client->ps.quickerGetup = qtrue;

								G_Sound(ent, CHAN_AUTO, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
							}
							else if (ent->client->pers.player_statuses & (1 << 23) && 
									 Distance(ent->client->ps.origin, player_ent->client->ps.origin) < 300)
							{ // zyk: Meditation Drain
								int heal_amount = 8;

								G_Damage(player_ent, ent, ent, NULL, NULL, heal_amount/3, 0, MOD_MELEE);

								player_ent->client->ps.electrifyTime = level.time + 1000;

								if ((ent->health + heal_amount) < ent->client->pers.max_rpg_health)
								{
									ent->health += heal_amount;
								}
								else if (ent->health < ent->client->pers.max_rpg_health)
								{
									ent->health = ent->client->pers.max_rpg_health;
								}
								else if ((ent->client->ps.stats[STAT_ARMOR] + heal_amount) < ent->client->pers.max_rpg_shield)
								{
									ent->client->ps.stats[STAT_ARMOR] += heal_amount;
								}
								else
								{
									ent->client->ps.stats[STAT_ARMOR] = ent->client->pers.max_rpg_shield;
								}
							}
						}
					}

					ent->client->pers.monk_unique_timer = level.time + 200;
				}
				else if (ent->client->pers.rpg_class == 5)
				{
					if (ent->client->pers.thermal_vision == qtrue && ent->client->ps.zoomMode == 0)
					{ // zyk: if the stealth attacker stops using sniper scope, stop the Thermal Detector
						ent->client->pers.thermal_vision = qfalse;
						ent->client->ps.fd.forcePowersActive &= ~(1 << FP_SEE);
						ent->client->ps.fd.forcePowersKnown &= ~(1 << FP_SEE);
						ent->client->ps.fd.forcePowerLevel[FP_SEE] = FORCE_LEVEL_0;
					}
					else if (ent->client->pers.thermal_vision == qfalse && ent->client->ps.zoomMode == 1 && ent->client->pers.secrets_found & (1 << 7))
					{ // zyk: Stealth Attacker with Upgrade, activate the Thermal Detector
						ent->client->pers.thermal_vision = qtrue;
						ent->client->ps.fd.forcePowersKnown |= (1 << FP_SEE);
						ent->client->ps.fd.forcePowerLevel[FP_SEE] = FORCE_LEVEL_1;
						ent->client->ps.fd.forcePowersActive |= (1 << FP_SEE);
					}
				}

				if (level.quest_map > 0 && ent->client->ps.duelInProgress == qfalse && ent->health > 0)
				{ // zyk: control the quest events which happen in the quest maps, if player can play quests now, is alive and is not in a private duel
					// zyk: fixing exploit in boss battles. If player is in a vehicle, kill the player
					if (ent->client->pers.guardian_mode > 0 && ent->client->ps.m_iVehicleNum > 0)
						G_Kill( ent );

					if (level.quest_map == 1)
					{
						zyk_try_get_dark_quest_note(ent, 4);

						if (ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS && !(ent->client->pers.defeated_guardians & (1 << 4)) && ent->client->pers.can_play_quest == 1 && ent->client->pers.guardian_mode == 0 && (int) ent->r.currentOrigin[0] > 1962 && (int) ent->r.currentOrigin[0] < 2162 && (int) ent->r.currentOrigin[1] > 3989 && (int) ent->r.currentOrigin[1] < 4189 && (int) ent->r.currentOrigin[2] >= 360 && (int) ent->r.currentOrigin[2] <= 369)
						{
							if (ent->client->pers.light_quest_timer < level.time)
							{
								if (ent->client->pers.light_quest_messages == 0)
									trap->SendServerCommand( ent->s.number, va("chat \"^4Guardian of Water: ^7I am the Guardian of Water, %s^7... You must now prove yourself...\"",ent->client->pers.netname));
								else if (ent->client->pers.light_quest_messages == 1)
								{
									spawn_boss(ent,2062,4089,361,90,"guardian_boss_1",2062,4189,500,90,1);
								}
								ent->client->pers.light_quest_messages++;
								ent->client->pers.light_quest_timer = level.time + 3000;
							}
						}

						if (ent->client->pers.universe_quest_progress == 3 && ent->client->pers.can_play_quest == 1 && ent->client->pers.universe_quest_objective_control == 4 && ent->client->pers.universe_quest_timer < level.time && (int) ent->client->ps.origin[0] > 2720 && (int) ent->client->ps.origin[0] < 2840 && (int) ent->client->ps.origin[1] > 3944 && (int) ent->client->ps.origin[1] < 3988 && (int) ent->client->ps.origin[2] == 1432)
						{ // zyk: fourth Universe Quest objective
							if (ent->client->pers.universe_quest_messages == 0)
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: Where are the sages?\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 1)
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: Oh...a note. They must have left it here for me, maybe...\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 2)
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: Ok, let's see what they want from me.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 3)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Note^7: %s^7, we have some information for you.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 4)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Note^7: We left yavin1b to find out what happened to the ^2Sage of Universe^7!\""));
							else if (ent->client->pers.universe_quest_messages == 5)
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: Sage of Universe?\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 6)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Note^7: He has the amulet of Universe, about which we did not tell you before.\""));
							else if (ent->client->pers.universe_quest_messages == 7)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Note^7: Forgive us for that, please.\""));
							else if (ent->client->pers.universe_quest_messages == 8)
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: So there is a fourth sage and his amulet.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 9)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Note^7: We believe that the sage is hidden in t3_hevil, but we are not sure.\""));
							else if (ent->client->pers.universe_quest_messages == 10)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Note^7: Find him and he will give you the amulet of Universe.\""));
							else if (ent->client->pers.universe_quest_messages == 11)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Note^7: After knowing the sage whereabouts, we went to taspir1 to spy on Master of Evil actions there.\""));
							else if (ent->client->pers.universe_quest_messages == 12)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Note^7: We will tell you all of this in a more detailed way later.\""));							
							else if (ent->client->pers.universe_quest_messages == 13)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Note^7: Please find all four amulets and come to taspir1.\""));
							else if (ent->client->pers.universe_quest_messages == 14)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Note^7: Good luck, %s^7.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 15)
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: Maybe the Sage of Universe has more info about the guardian amulets.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 16)
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: I hope that he is really in ^3t3_hevil^7.\"", ent->client->pers.netname));

							ent->client->pers.universe_quest_messages++;
							ent->client->pers.universe_quest_timer = level.time + 5000;

							if (ent->client->pers.universe_quest_messages == 17)
							{ // zyk: complete the objective
								ent->client->pers.universe_quest_objective_control = -1;
								if (ent->client->pers.universe_quest_counter & (1 << 29))
								{ // zyk: if player is in Challenge Mode, do not remove this bit value
									ent->client->pers.universe_quest_counter = 0;
									ent->client->pers.universe_quest_counter |= (1 << 29);
								}
								else
									ent->client->pers.universe_quest_counter = 0;
								ent->client->pers.universe_quest_progress = 4;
								clean_note_model();
								save_account(ent);
								quest_get_new_player(ent);
							}
						}

						if (ent->client->pers.universe_quest_progress == 2 && ent->client->pers.can_play_quest == 1 && ent->client->pers.universe_quest_objective_control == 3 && ent->client->pers.universe_quest_timer < level.time)
						{ // zyk: third Universe Quest objective, sages should appear in this map
							gentity_t *npc_ent = NULL;
							int change_player = 0;

							if (ent->client->pers.universe_quest_messages == 0)
							{
								if (!(ent->client->pers.universe_quest_counter & (1 << 3)))
									trap->SendServerCommand( ent->s.number, va("chat \"%s^7: I sense the presence of an artifact here.\"", ent->client->pers.netname));
								
								npc_ent = Zyk_NPC_SpawnType("sage_of_light",2780,4034,1583,0);
							}
							else if (ent->client->pers.universe_quest_messages == 1)
								npc_ent = Zyk_NPC_SpawnType("sage_of_eternity",2780,3966,1583,0);
							else if (ent->client->pers.universe_quest_messages == 2)
								npc_ent = Zyk_NPC_SpawnType("sage_of_darkness",2780,3904,1583,0);
							else if (ent->client->pers.universe_quest_messages == 6)
								trap->SendServerCommand( ent->s.number, "chat \"^5Sage of Light: ^7Your quest is very difficult, but at the end you will see it was worthy the effort...\"");
							else if (ent->client->pers.universe_quest_messages == 8)
							{
								ent->client->ps.powerups[PW_FORCE_BOON] = level.time + 3000;

								trap->SendServerCommand( ent->s.number, va("chat \"^3Sage of Eternity: ^7Well done, %s^7. Now I can give you the artifact.\"",ent->client->pers.netname));
								ent->client->pers.universe_quest_counter |= (1 << 1);
								save_account(ent);

								universe_quest_artifacts_checker(ent);

								change_player = 1;
							}
							else if (ent->client->pers.universe_quest_messages == 9)
							{
								trap->SendServerCommand( ent->s.number, "chat \"^3Sage of Eternity: ^7Never give up. The fate of the Universe depends upon you.\"");
								change_player = 1;
							}
							else if (ent->client->pers.universe_quest_messages == 12)
								trap->SendServerCommand( ent->s.number, va("chat \"^1Sage of Darkness: ^7You are the legendary hero, %s^7! Go on and defeat the evil guys!\"",ent->client->pers.netname));
							
							if (npc_ent)
							{
								npc_ent->client->pers.universe_quest_objective_control = ent->client->pers.universe_quest_messages;
							}

							if (ent->client->pers.universe_quest_messages == 3 && ent->client->pers.universe_quest_artifact_holder_id == -1 && !(ent->client->pers.universe_quest_counter & (1 << 3)))
							{
								npc_ent = Zyk_NPC_SpawnType("quest_mage",-396,-287,-150,-153);
								if (npc_ent)
								{ // zyk: spawning the quest_mage artifact holder
									npc_ent->client->ps.powerups[PW_FORCE_BOON] = level.time + 5500;
									npc_ent->client->pers.universe_quest_artifact_holder_id = ent-g_entities;

									ent->client->pers.universe_quest_artifact_holder_id = 3;
								}
							}

							if (ent->client->pers.universe_quest_messages < 3)
								ent->client->pers.universe_quest_messages++;

							// zyk: after displaying the message, sets this to 15 (above 12, the last message possible) so only when player talks to a sage the message appears again
							if (ent->client->pers.universe_quest_messages > 3)
								ent->client->pers.universe_quest_messages = 15;

							ent->client->pers.universe_quest_timer = level.time + 2000;

							if (change_player == 1)
								quest_get_new_player(ent);
						}
					}
					else if (level.quest_map == 2)
					{
						zyk_try_get_dark_quest_note(ent, 5);
					}
					else if (level.quest_map == 3)
					{
						zyk_try_get_dark_quest_note(ent, 6);
					}
					else if (level.quest_map == 4)
					{
						zyk_try_get_dark_quest_note(ent, 7);
					}
					else if (level.quest_map == 5)
					{
						// zyk: Dark Quest Note
						zyk_try_get_dark_quest_note(ent, 8);

						if (ent->client->pers.universe_quest_progress == 2 && ent->client->pers.can_play_quest == 1 && ent->client->pers.universe_quest_objective_control == 3 && !(ent->client->pers.universe_quest_counter & (1 << 9)) && ent->client->pers.universe_quest_timer < level.time)
						{
							gentity_t *npc_ent = NULL;
							if (ent->client->pers.universe_quest_messages == 0)
							{
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: I sense the presence of an artifact here.\"", ent->client->pers.netname));
								npc_ent = Zyk_NPC_SpawnType("quest_mage",724,5926,951,31);
								if (npc_ent)
								{
									npc_ent->client->ps.powerups[PW_FORCE_BOON] = level.time + 5500;

									npc_ent->client->pers.universe_quest_artifact_holder_id = ent-g_entities;
									ent->client->pers.universe_quest_artifact_holder_id = 9;
								}
							}

							if (ent->client->pers.universe_quest_messages < 1)
							{
								ent->client->pers.universe_quest_messages++;
								ent->client->pers.universe_quest_timer = level.time + 5000;
							}
						}

						if (ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS && !(ent->client->pers.defeated_guardians & (1 << 12)) && ent->client->pers.can_play_quest == 1 && ent->client->pers.guardian_mode == 0 && (int) ent->r.currentOrigin[0] > -5648 && (int) ent->r.currentOrigin[0] < -5448 && (int) ent->r.currentOrigin[1] > 11448 && (int) ent->r.currentOrigin[1] < 11648 && (int) ent->r.currentOrigin[2] >= 980 && (int) ent->r.currentOrigin[2] <= 1000)
						{
							if (ent->client->pers.light_quest_timer < level.time)
							{
								if (ent->client->pers.light_quest_messages == 0)
									trap->SendServerCommand( ent->s.number, va("chat \"^5Guardian of Ice: ^7I am the Guardian of Ice, %s^7. I will freeze you!\"",ent->client->pers.netname));
								else if (ent->client->pers.light_quest_messages == 1)
								{
									spawn_boss(ent,-4652,11607,991,179,"guardian_boss_10",-5623,11598,991,0,16);
								}
								ent->client->pers.light_quest_messages++;
								ent->client->pers.light_quest_timer = level.time + 3000;
							}
						}
					}
					else if (level.quest_map == 6)
					{
						// zyk: Dark Quest Note
						zyk_try_get_dark_quest_note(ent, 9);

						if (ent->client->pers.universe_quest_progress == 2 && ent->client->pers.can_play_quest == 1 && ent->client->pers.universe_quest_objective_control == 3 && !(ent->client->pers.universe_quest_counter & (1 << 6)) && ent->client->pers.universe_quest_timer < level.time)
						{ // zyk: Universe Quest artifact
							gentity_t *npc_ent = NULL;
							if (ent->client->pers.universe_quest_messages == 0)
							{
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: I sense the presence of an artifact here.\"", ent->client->pers.netname));
								npc_ent = Zyk_NPC_SpawnType("quest_reborn_red",2120,-1744,39,90);
							}
							else if (ent->client->pers.universe_quest_messages == 1)
								npc_ent = Zyk_NPC_SpawnType("quest_reborn_red",2318,-1744,39,90);
							else if (ent->client->pers.universe_quest_messages == 2)
							{
								npc_ent = Zyk_NPC_SpawnType("quest_mage",2214,-1744,39,90);
								if (npc_ent)
								{
									npc_ent->client->ps.powerups[PW_FORCE_BOON] = level.time + 5500;

									npc_ent->client->pers.universe_quest_artifact_holder_id = ent-g_entities;
									ent->client->pers.universe_quest_artifact_holder_id = 6;
								}
							}

							if (ent->client->pers.universe_quest_messages < 3)
							{
								ent->client->pers.universe_quest_messages++;
								ent->client->pers.universe_quest_timer = level.time + 5000;
							}
						}
					}
					else if (level.quest_map == 7)
					{
						zyk_try_get_dark_quest_note(ent, 10);

						if (ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS && !(ent->client->pers.defeated_guardians & (1 << 7)) && ent->client->pers.can_play_quest == 1 && ent->client->pers.guardian_mode == 0 && (int) ent->client->ps.origin[0] > 2400 && (int) ent->client->ps.origin[0] < 2600 && (int) ent->client->ps.origin[1] > 2040 && (int) ent->client->ps.origin[1] < 2240 && (int) ent->client->ps.origin[2] == -551)
						{
							if (ent->client->pers.light_quest_timer < level.time)
							{
								if (ent->client->pers.light_quest_messages == 0)
									trap->SendServerCommand( ent->s.number, va("chat \"^5Guardian of Intelligence: ^7I am the Guardian of Intelligence, %s^7. Face the power of my advanced mind.\"",ent->client->pers.netname));
								else if (ent->client->pers.light_quest_messages == 1)
								{
									spawn_boss(ent,1920,2068,729,-90,"guardian_boss_4",1920,982,729,90,4);
								}
								ent->client->pers.light_quest_messages++;
								ent->client->pers.light_quest_timer = level.time + 3000;
							}
						}
					}
					else if (level.quest_map == 8)
					{
						if (ent->client->pers.universe_quest_progress == 4 && ent->client->pers.can_play_quest == 1 && ent->client->pers.universe_quest_objective_control == 5 && ent->client->pers.universe_quest_timer < level.time)
						{
							gentity_t *npc_ent = NULL;
							if (ent->client->pers.universe_quest_messages == 0)
								Zyk_NPC_SpawnType("quest_reborn_red",785,-510,1177,-179);
							else if (ent->client->pers.universe_quest_messages == 1)
								Zyk_NPC_SpawnType("quest_reborn_red",253,-510,1177,-179);
							else if (ent->client->pers.universe_quest_messages == 2)
								Zyk_NPC_SpawnType("quest_reborn_boss",512,-315,1177,-90);
							else if (ent->client->pers.universe_quest_messages == 3)
								npc_ent = Zyk_NPC_SpawnType("sage_of_universe",507,-623,537,90);
							else if (ent->client->pers.universe_quest_messages == 5)
								trap->SendServerCommand( ent->s.number, va("chat \"^2Sage of Universe^7: Greetings, %s^7. I am the Sage of Universe.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 6)
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: I finally found you!\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 7)
								trap->SendServerCommand( ent->s.number, va("chat \"^2Sage of Universe^7: Yes, %s^7. I must tell you some things.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 8)
								trap->SendServerCommand( ent->s.number, va("chat \"^2Sage of Universe^7: I will give you the amulet of Universe, but I must ask you a favor.\""));
							else if (ent->client->pers.universe_quest_messages == 9)
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: Thanks! Now what I can do for you.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 10)
								trap->SendServerCommand( ent->s.number, va("chat \"^2Sage of Universe^7: You must find the other three amulets in the ^3City of the Merchants^7.\""));
							else if (ent->client->pers.universe_quest_messages == 11)
								trap->SendServerCommand( ent->s.number, va("chat \"^2Sage of Universe^7: It is located at ^3mp/siege_desert^7. Now I will go to taspir1 to find the other sages.\""));
							else if (ent->client->pers.universe_quest_messages == 12)
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: Ok! But how did you know about the sages?\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 13)
								trap->SendServerCommand( ent->s.number, va("chat \"^2Sage of Universe^7: %s^7...the mysterious voice at the beginning of your quest was mine.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 14)
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: So you knew about me since the beginning!\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 15)
								trap->SendServerCommand( ent->s.number, va("chat \"^2Sage of Universe^7: yes %s^7. We will explain everything after you find all amulets.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 16)
								trap->SendServerCommand( ent->s.number, va("chat \"^2Sage of Universe^7: After you found them, go to taspir1. We will be waiting for you.\""));
							else if (ent->client->pers.universe_quest_messages == 17)
								trap->SendServerCommand( ent->s.number, va("chat \"^2Sage of Universe^7: Now go %s^7! I wish you luck on your quest.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 18)
							{
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: Thanks! Don't worry, I will find them in ^3mp/siege_desert ^7and meet you all in taspir1.\"", ent->client->pers.netname));

								if (ent->client->pers.universe_quest_counter & (1 << 29))
								{ // zyk: if player is in Challenge Mode, do not remove this bit value
									ent->client->pers.universe_quest_counter = 0;
									ent->client->pers.universe_quest_counter |= (1 << 29);
								}
								else
								{
									ent->client->pers.universe_quest_counter = 0;
								}

								ent->client->pers.universe_quest_progress = 5;
								ent->client->pers.universe_quest_objective_control = -1;
								ent->client->pers.universe_quest_messages = 0;

								save_account(ent);

								quest_get_new_player(ent);
							}

							if (npc_ent)
							{ // zyk: Sage of Universe, set this player id on him so we can test it to see if the player found him
								npc_ent->client->pers.universe_quest_objective_control = ent-g_entities;
							}

							if (ent->client->pers.universe_quest_messages != 4 && ent->client->pers.universe_quest_messages < 18)
							{
								ent->client->pers.universe_quest_messages++;
							}

							ent->client->pers.universe_quest_timer = level.time + 5000;
						}
					}
					else if (level.quest_map == 9)
					{
						if (ent->client->pers.universe_quest_progress == 0 && ent->client->pers.can_play_quest == 1 && ent->client->pers.universe_quest_timer < level.time && ent->client->pers.universe_quest_objective_control != -1)
						{ // zyk: first objective of Universe Quest
							gentity_t *npc_ent = NULL;

							if (ent->client->pers.universe_quest_messages == 0)
							{
								trap->SendServerCommand( ent->s.number, "chat \"^2Mysterious Voice^7: Go, hero... save the Guardian Sages... they need your help...\"");
								npc_ent = Zyk_NPC_SpawnType("sage_of_light",904,-1079,353,179);
							}
							else if (ent->client->pers.universe_quest_messages == 1)
							{
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: This voice...\"", ent->client->pers.netname));
								npc_ent = Zyk_NPC_SpawnType("sage_of_darkness",904,-991,353,179);
							}
							else if (ent->client->pers.universe_quest_messages == 2)
							{
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: I don't understand... maybe I should do as it says...\"", ent->client->pers.netname));
								npc_ent = Zyk_NPC_SpawnType("sage_of_eternity",904,-897,353,179);
							}
							else if (ent->client->pers.universe_quest_messages == 3)
							{
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: I hope they are able to tell me about this strange voice.\"", ent->client->pers.netname));
							}
							else if (ent->client->pers.universe_quest_messages == 5)
							{
								trap->SendServerCommand( ent->s.number, "chat \"^5Sage of Light: ^7The enemies are coming!\"");
							}
							else if (ent->client->pers.universe_quest_messages == 6)
								trap->SendServerCommand( ent->s.number, "chat \"^1Sage of Darkness: ^7We will fight to the death!\"");
							else if (ent->client->pers.universe_quest_messages == 7)
								trap->SendServerCommand( ent->s.number, "chat \"^3Sage of Eternity: ^7Hero... please help us!\"");
							else if (ent->client->pers.universe_quest_messages == 8)
							{
								if (ent->client->pers.light_quest_messages > 10)
								{
									npc_ent = Zyk_NPC_SpawnType("quest_reborn",750,-908,497,0);
									ent->client->pers.light_quest_messages--;
								}
								else if (ent->client->pers.light_quest_messages > 5)
								{
									npc_ent = Zyk_NPC_SpawnType("quest_reborn_blue",750,-995,497,0);
									ent->client->pers.light_quest_messages--;
								}
								else if (ent->client->pers.light_quest_messages > 1)
								{
									npc_ent = Zyk_NPC_SpawnType("quest_reborn_red",750,-1079,497,0);
									ent->client->pers.light_quest_messages--;
								}
								else if (ent->client->pers.light_quest_messages == 1 && ent->client->pers.universe_quest_objective_control == 1)
								{
									ent->client->pers.universe_quest_messages = 9;
									ent->client->pers.light_quest_messages = 0;
									npc_ent = Zyk_NPC_SpawnType("quest_reborn_boss",730,-995,497,0);
								}
							}
							else if (ent->client->pers.universe_quest_messages == 10)
							{
								trap->SendServerCommand( ent->s.number, "chat \"^3Sage of Eternity: ^7Thank you, brave warrior! You saved our lives.\"");
							}
							else if (ent->client->pers.universe_quest_messages == 11)
							{
								ent->client->pers.universe_quest_progress = 1;
								save_account(ent);
								quest_get_new_player(ent);
							}

							if (ent->client->pers.universe_quest_messages == 12)
							{ // zyk: pass the rurn if one of the sages died
								quest_get_new_player(ent);
							}
							else if (ent->client->pers.universe_quest_progress == 0)
							{
								if (npc_ent)
								{ // zyk: sets the player id who must kill this quest reborn, or protect this sage
									npc_ent->client->pers.universe_quest_objective_control = ent-g_entities;
								}

								if (ent->client->pers.universe_quest_messages != 4 && ent->client->pers.universe_quest_messages != 8 && ent->client->pers.universe_quest_messages != 9)
									ent->client->pers.universe_quest_messages++;
								
								if (ent->client->pers.universe_quest_messages < 8)
									ent->client->pers.universe_quest_timer = level.time + 3000;
								else
									ent->client->pers.universe_quest_timer = level.time + 1200;
							}
						}
						else if (ent->client->pers.universe_quest_progress == 1 && ent->client->pers.can_play_quest == 1 && ent->client->pers.universe_quest_timer < level.time && ent->client->pers.universe_quest_objective_control > -1)
						{ // zyk: second Universe Quest mission
							gentity_t *npc_ent = NULL;
							if (ent->client->pers.universe_quest_messages == 0)
								npc_ent = Zyk_NPC_SpawnType("sage_of_light",904,-1079,353,179);
							else if (ent->client->pers.universe_quest_messages == 1)
								npc_ent = Zyk_NPC_SpawnType("sage_of_eternity",904,-991,353,179);
							else if (ent->client->pers.universe_quest_messages == 2)
								npc_ent = Zyk_NPC_SpawnType("sage_of_darkness",904,-897,353,179);
							else if (ent->client->pers.universe_quest_messages == 4)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Sage of Eternity: ^7Welcome, hero %s^7!\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 5)
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: Hero? What are you talking about?\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 6)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Sage of Eternity: ^7%s...^7you are the legendary hero.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 7)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Sage of Eternity: ^7Legend says that a hero with great power would save the Guardian Sages and fight against the Master of Evil.\""));
							else if (ent->client->pers.universe_quest_messages == 8)
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: But i can't be this legendary hero...\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 9)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Sage of Eternity: ^7You are! You saved us, the Brotherhood of Guardian Sages.\""));
							else if (ent->client->pers.universe_quest_messages == 10)
								trap->SendServerCommand( ent->s.number, va("chat \"^5Sage of Light: ^7Indeed...now allow us to introduce ourselves.\""));
							else if (ent->client->pers.universe_quest_messages == 11)
								trap->SendServerCommand( ent->s.number, va("chat \"^5Sage of Light: ^7I am the Sage of Light...I am the follower of the Guardian of Light.\""));
							else if (ent->client->pers.universe_quest_messages == 12)
								trap->SendServerCommand( ent->s.number, va("chat \"^1Sage of Darkness: ^7I am the Sage of Darkness...I am the follower of the Guardian of Darkness.\""));
							else if (ent->client->pers.universe_quest_messages == 13)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Sage of Eternity: ^7I am the Sage of Eternity...I am the follower of the Guardian of Eternity.\""));
							else if (ent->client->pers.universe_quest_messages == 14)
								trap->SendServerCommand( ent->s.number, va("chat \"^5Sage of Light: ^7Together, we have the knowledge about the guardians powers.\""));
							else if (ent->client->pers.universe_quest_messages == 15)
								trap->SendServerCommand( ent->s.number, va("chat \"^1Sage of Darkness: ^7With our strength... we should protect the three guardian amulets.\""));
							else if (ent->client->pers.universe_quest_messages == 16)
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: Guardian amulets?\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 17)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Sage of Eternity: ^7They have the essence of the guardians powers.\""));
							else if (ent->client->pers.universe_quest_messages == 18)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Sage of Eternity: ^7But we couldn't stand against the rise of an evil force.\""));
							else if (ent->client->pers.universe_quest_messages == 19)
								trap->SendServerCommand( ent->s.number, va("chat \"^5Sage of Light: ^7We had an apprentice called Thor.\""));
							else if (ent->client->pers.universe_quest_messages == 20)
								trap->SendServerCommand( ent->s.number, va("chat \"^1Sage of Darkness: ^7He was strong but had an evil mind.\""));
							else if (ent->client->pers.universe_quest_messages == 21)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Sage of Eternity: ^7We had no other choice but to ask him to leave the Brotherhood...\""));
							else if (ent->client->pers.universe_quest_messages == 22)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Sage of Eternity: ^7He then...fought us...and we couldn't stand against his power.\""));
							else if (ent->client->pers.universe_quest_messages == 23)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Sage of Eternity: ^7We flee from him, and he got the guardian amulets.\""));
							else if (ent->client->pers.universe_quest_messages == 24)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Sage of Eternity: ^7With his new power, he sealed the Guardian of Universe!\""));
							else if (ent->client->pers.universe_quest_messages == 25)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Sage of Eternity: ^7He is now immortal and became the Master of Evil!\""));
							else if (ent->client->pers.universe_quest_messages == 26)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Sage of Eternity: ^7To achieve immortality, he divided his life force in 8 artifacts.\""));
							else if (ent->client->pers.universe_quest_messages == 27)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Sage of Eternity: ^7They are hidden in different locations and protected by his soldiers.\""));
							else if (ent->client->pers.universe_quest_messages == 28)
								trap->SendServerCommand( ent->s.number, va("chat \"^5Sage of Light: ^7You must find the artifacts, and the amulets, which he left somewhere, because he no longer needed them.\""));
							else if (ent->client->pers.universe_quest_messages == 29)
								trap->SendServerCommand( ent->s.number, va("chat \"^1Sage of Darkness: ^7Then take them to taspir1, where he resides now.\""));
							else if (ent->client->pers.universe_quest_messages == 30)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Sage of Eternity: ^7You cannot refuse it! We really need your help.\""));
							else if (ent->client->pers.universe_quest_messages == 31)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Sage of Eternity: ^7The fate of the Universe is at your hands!\""));							
							else if (ent->client->pers.universe_quest_messages == 32)
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: Ok then. I accept it. I will find the artifacts and the amulets.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 33)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Sage of Eternity: ^7Thank you, brave hero!\""));
							else if (ent->client->pers.universe_quest_messages == 34)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Sage of Eternity: ^7We have an artifact, come to yavin1b and talk to us to get it.\""));
							else if (ent->client->pers.universe_quest_messages == 35)
							{
								trap->SendServerCommand( ent->s.number, va("chat \"^3Sage of Eternity: ^7Now go, legendary hero! Courage in your quest!\""));
								
								ent->client->pers.universe_quest_progress = 2;
								
								save_account(ent);

								quest_get_new_player(ent);
							}

							if (npc_ent)
							{ // zyk: the sage cant die or the player fails
								npc_ent->client->pers.universe_quest_objective_control = ent-g_entities;
							}

							if (ent->client->pers.universe_quest_messages < 3)
							{
								ent->client->pers.universe_quest_messages++;
								ent->client->pers.universe_quest_timer = level.time + 1500;
							}
							else if (ent->client->pers.universe_quest_messages >= 4)
							{ // zyk: universe_quest_messages will be 4 or higher when player reaches and press USE key on one of the sages
								ent->client->pers.universe_quest_messages++;
								ent->client->pers.universe_quest_timer = level.time + 5000;
							}
						}

						if (ent->client->pers.universe_quest_messages < 5)
						{ // zyk: universe_quest_messages must be less than 5. If it is at least 5, player is in the universe quest missions in this map
							zyk_try_get_dark_quest_note(ent, 12);
						}
					}
					else if (level.quest_map == 10)
					{   
						// zyk: battle against the Guardian of Light
						if (ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS && light_quest_defeated_guardians(ent) == qtrue && ent->client->pers.can_play_quest == 1 && ent->client->pers.guardian_mode == 0 && (int) ent->client->ps.origin[0] > -1350 && (int) ent->client->ps.origin[0] < -630 && (int) ent->client->ps.origin[1] > -1900 && (int) ent->client->ps.origin[1] < -1400 && (int) ent->client->ps.origin[2] > 5 && (int) ent->client->ps.origin[2] < 56)
						{
							if (ent->client->pers.light_quest_timer < level.time)
							{
								if (ent->client->pers.light_quest_messages == 0)
									trap->SendServerCommand( ent->s.number, "chat \"^5Guardian of Light: ^7Well done, brave warrior!\"");
								else if (ent->client->pers.light_quest_messages == 1)
									trap->SendServerCommand( ent->s.number, "chat \"^5Guardian of Light: ^7Now I shall test your skills...\"");
								else if (ent->client->pers.light_quest_messages == 2)
									trap->SendServerCommand( ent->s.number, "chat \"^5Guardian of Light: ^7Defeat me and I will grant you the Light Power!\"");
								else if (ent->client->pers.light_quest_messages == 3)
								{
									spawn_boss(ent,-992,-1802,25,90,"guardian_boss_9",0,0,0,0,8);
								}

								ent->client->pers.light_quest_messages++;
								ent->client->pers.light_quest_timer = level.time + 3000;
							}
						}

						// zyk: battle against the Guardian of Darkness
						if (ent->client->pers.hunter_quest_progress != NUMBER_OF_OBJECTIVES && dark_quest_collected_notes(ent) == qtrue && ent->client->pers.can_play_quest == 1 && ent->client->pers.guardian_mode == 0 && (int) ent->client->ps.origin[0] > -200 && (int) ent->client->ps.origin[0] < 100 && (int) ent->client->ps.origin[1] > 252 && (int) ent->client->ps.origin[1] < 552 && (int) ent->client->ps.origin[2] == -231)
						{
							if (ent->client->pers.hunter_quest_timer < level.time)
							{
								if (ent->client->pers.hunter_quest_messages == 0)
									trap->SendServerCommand( ent->s.number, "chat \"^1Guardian of Darkness: ^7Excelent, mighty warrior...\"");
								else if (ent->client->pers.hunter_quest_messages == 1)
									trap->SendServerCommand( ent->s.number, "chat \"^1Guardian of Darkness: ^7Now your might will be tested...\"");
								else if (ent->client->pers.hunter_quest_messages == 2)
									trap->SendServerCommand( ent->s.number, "chat \"^1Guardian of Darkness: ^7Defeat me and I shall grant you the Dark Power...\"");
								else if (ent->client->pers.hunter_quest_messages == 3)
								{
									spawn_boss(ent,-34,402,-231,90,"guardian_of_darkness",0,0,0,0,9);
								}
								ent->client->pers.hunter_quest_messages++;
								ent->client->pers.hunter_quest_timer = level.time + 3000;
							}
						}

						if (ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS && !(ent->client->pers.defeated_guardians & (1 << 6)) && ent->client->pers.can_play_quest == 1 && ent->client->pers.guardian_mode == 0 && (int) ent->client->ps.origin[0] > 412 && (int) ent->client->ps.origin[0] < 612 && (int) ent->client->ps.origin[1] > 4729 && (int) ent->client->ps.origin[1] < 4929 && (int) ent->client->ps.origin[2] >= 55 && (int) ent->client->ps.origin[2] <= 64)
						{
							if (ent->client->pers.light_quest_timer < level.time)
							{
								if (ent->client->pers.light_quest_messages == 0)
									trap->SendServerCommand( ent->s.number, va("chat \"^2Guardian of Forest: ^7I am the Guardian of Forest, %s^7! You cant overcome the power of trees!\"",ent->client->pers.netname));
								else if (ent->client->pers.light_quest_messages == 1)
								{
									spawn_boss(ent,119,4819,33,0,"guardian_boss_3",512,4829,62,179,3);
								}
								ent->client->pers.light_quest_messages++;
								ent->client->pers.light_quest_timer = level.time + 3000;
							}
						}

						if (ent->client->pers.eternity_quest_progress < NUMBER_OF_ETERNITY_QUEST_OBJECTIVES && ent->client->pers.eternity_quest_timer < level.time && ent->client->pers.can_play_quest == 1 && ent->client->pers.guardian_mode == 0 && (int) ent->client->ps.origin[0] > -676 && (int) ent->client->ps.origin[0] < -296 && (int) ent->client->ps.origin[1] > 1283 && (int) ent->client->ps.origin[1] < 1663 && (int) ent->client->ps.origin[2] > 60 && (int) ent->client->ps.origin[2] < 120)
						{
							if (ent->client->pers.eternity_quest_progress == 0)
							{
								trap->SendServerCommand( ent->s.number, "chat \"^3Riddle of Earth: ^7It opens the locked ways, by a turn of itself... it reveals the passageways, or can be guarded on the shelf...\"");
							}
							else if (ent->client->pers.eternity_quest_progress == 1)
							{
								trap->SendServerCommand( ent->s.number, "chat \"^1Riddle of Fire: ^7It can measure time, when a tick is done... it's an useful item, and is also a nice one...\"");
							}
							else if (ent->client->pers.eternity_quest_progress == 2)
							{
								trap->SendServerCommand( ent->s.number, "chat \"^1Riddle of Darkness: ^7Its hit can hurt a person, with the power of the blade...it can kill and destroy, making someone's life fade...\"");
							}
							else if (ent->client->pers.eternity_quest_progress == 3)
							{
								trap->SendServerCommand( ent->s.number, "chat \"^4Riddle of Water: ^7Its size is immense, and having its energy is a must... it keeps life on Earth, on its power we can trust...\"");
							}
							else if (ent->client->pers.eternity_quest_progress == 4)
							{
								trap->SendServerCommand( ent->s.number, "chat \"^7Riddle of Wind: ^7One can feel warm, with the power of its energy... its power can also be evil, burning to ashes all the harmony...\"");
							}
							else if (ent->client->pers.eternity_quest_progress == 5)
							{
								trap->SendServerCommand( ent->s.number, "chat \"^5Riddle of Light: ^7It has a pure essence, it can create life... but it can also be furious, like a sharp cut of a knife...\"");
							}
							else if (ent->client->pers.eternity_quest_progress == 6)
							{
								trap->SendServerCommand( ent->s.number, "chat \"^6Riddle of Agility: ^7From the beginning of everything, to the end of everyone... it's there in all that exists, and from it escapes no one...\"");
							}
							else if (ent->client->pers.eternity_quest_progress == 7)
							{
								trap->SendServerCommand( ent->s.number, "chat \"^2Riddle of Forest: ^7It has a lot of energy, to sustain a celestial object... it has a shining light, with another of its kind it can connect...\"");
							}		
							else if (ent->client->pers.eternity_quest_progress == 8)
							{
								trap->SendServerCommand( ent->s.number, "chat \"^5Riddle of Intelligence: ^7It represents natural life, it's in animals and trees... in water and mountains, in these that someone sees...\"");
							}
							else if (ent->client->pers.eternity_quest_progress == 9)
							{
								trap->SendServerCommand( ent->s.number, "chat \"^3Riddle of Eternity: ^7The pure feeling of affection, even the evil ones can sustain... if one can feel and share, his life will not be in vain...\"");
							}
							else if (ent->client->pers.eternity_quest_progress == (NUMBER_OF_ETERNITY_QUEST_OBJECTIVES - 1))
							{
								if (ent->client->pers.eternity_quest_timer == 0)
								{
									trap->SendServerCommand( ent->s.number, "chat \"^3Guardian of Eternity: ^7You answered all my riddles. Now prove that you are worthy of my power.\"");
									ent->client->pers.eternity_quest_timer = level.time + 3000;
								}
								else
								{ // zyk: Guardian of Eternity battle
									spawn_boss(ent,-994,2975,25,90,"guardian_of_eternity",0,0,0,0,10);
								}
							}

							if (ent->client->pers.eternity_quest_progress < (NUMBER_OF_ETERNITY_QUEST_OBJECTIVES - 1))
							{
								ent->client->pers.eternity_quest_timer = level.time + 30000;
							}
						}

						if (ent->client->pers.universe_quest_progress == 2 && ent->client->pers.can_play_quest == 1 && ent->client->pers.guardian_mode == 0 && ent->client->pers.universe_quest_objective_control == 3 && ent->client->pers.universe_quest_timer < level.time && ent->client->pers.universe_quest_messages < 8 && !(ent->client->pers.universe_quest_counter & (1 << 8)))
						{
							gentity_t *npc_ent = NULL;
							if (ent->client->pers.universe_quest_messages == 0)
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: I sense the presence of an artifact here.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 1)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Spooky voice^7: Indeed you do, %s^7...\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 2)
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: What is going on here?\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 3)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Spooky voice^7: I know you are the legendary hero...I defeated the protector of the artifact here, he was serving the Master of Evil...\""));
							else if (ent->client->pers.universe_quest_messages == 4)
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: Well... could you give me the artifact pls?\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 5)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Spooky voice^7: Find me, %s^7, and I will give you the artifact you seek...\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 6)
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: I can't believe it... :/\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 7)
							{
								npc_ent = Zyk_NPC_SpawnType("quest_ragnos",-1462,1079,2062,0);
								if (npc_ent)
								{
									npc_ent->client->pers.universe_quest_artifact_holder_id = ent-g_entities;
									ent->client->pers.universe_quest_artifact_holder_id = npc_ent-g_entities;
								}
							}
							ent->client->pers.universe_quest_messages++;
							ent->client->pers.universe_quest_timer = level.time + 5000;
						}

						if (ent->client->pers.universe_quest_artifact_holder_id == -3 && ent->client->pers.can_play_quest == 1 && ent->client->pers.guardian_mode == 0 && ent->client->pers.universe_quest_timer < level.time && ent->client->ps.powerups[PW_FORCE_BOON])
						{ // zyk: player got the artifact, save it to his account
							ent->client->pers.universe_quest_artifact_holder_id = -1;
							ent->client->pers.universe_quest_counter |= (1 << 8);
							save_account(ent);

							trap->SendServerCommand( ent->s.number, va("chat \"%s^7: Thanks for the artifact! :)\"", ent->client->pers.netname));

							universe_quest_artifacts_checker(ent);

							quest_get_new_player(ent);
						}
						else if (ent->client->pers.universe_quest_artifact_holder_id > -1 && ent->client->pers.can_play_quest == 1 && ent->client->pers.guardian_mode == 0 && ent->client->ps.hasLookTarget == qtrue && ent->client->ps.lookTarget == ent->client->pers.universe_quest_artifact_holder_id)
						{ // zyk: found quest_ragnos npc, set artifact effect on player
							ent->client->ps.powerups[PW_FORCE_BOON] = level.time + 10000;
							ent->client->pers.universe_quest_timer = level.time + 5000;

							G_FreeEntity(&g_entities[ent->client->pers.universe_quest_artifact_holder_id]);

							ent->client->pers.universe_quest_artifact_holder_id = -3;

							trap->SendServerCommand( ent->s.number, va("chat \"^3Spooky voice^7: Nicely done...now, hero %s^7, receive this artifact...\"",ent->client->pers.netname));
						}
					}
					else if (level.quest_map == 11)
					{   
						if (ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS && !(ent->client->pers.defeated_guardians & (1 << 9)) && ent->client->pers.can_play_quest == 1 && ent->client->pers.guardian_mode == 0 && (int) ent->client->ps.origin[0] > -100 && (int) ent->client->ps.origin[0] < 100 && (int) ent->client->ps.origin[1] > -95 && (int) ent->client->ps.origin[1] < 105 && (int) ent->client->ps.origin[2] == -375)
						{
							if (ent->client->pers.light_quest_timer < level.time)
							{
								if (ent->client->pers.light_quest_messages == 0)
									trap->SendServerCommand( ent->s.number, va("chat \"^1Guardian of Fire: ^7I am the Guardian of Fire, %s^7! Now you will feel my fire burning!\"",ent->client->pers.netname));
								else if (ent->client->pers.light_quest_messages == 1)
								{
									spawn_boss(ent,0,5,-374,-90,"guardian_boss_6",0,-269,-374,90,6);
								}
								ent->client->pers.light_quest_messages++;
								ent->client->pers.light_quest_timer = level.time + 3000;
							}
						}
					}
					else if (level.quest_map == 12)
					{
						if (ent->client->pers.universe_quest_progress == 8 && ent->client->pers.can_play_quest == 1 && !(ent->client->pers.universe_quest_counter & (1 << 1)) && ent->client->pers.universe_quest_timer < level.time)
						{ // zyk: Revelations mission of Universe Quest
							gentity_t *npc_ent = NULL;

							if (ent->client->pers.universe_quest_messages == 0)
								npc_ent = Zyk_NPC_SpawnType("sage_of_light",-1082,4337,505,45);
							else if (ent->client->pers.universe_quest_messages == 1)
								npc_ent = Zyk_NPC_SpawnType("sage_of_darkness",-912,4503,505,-135);
							else if (ent->client->pers.universe_quest_messages == 2)
								npc_ent = Zyk_NPC_SpawnType("sage_of_eternity",-892,4340,505,135);
							else if (ent->client->pers.universe_quest_messages == 3)
								npc_ent = Zyk_NPC_SpawnType("sage_of_universe",-1091,4520,505,-45);
							else if (ent->client->pers.universe_quest_messages == 4)
								npc_ent = Zyk_NPC_SpawnType("guardian_boss_9",-1112,4418,505,0);
							else if (ent->client->pers.universe_quest_messages == 5)
								npc_ent = Zyk_NPC_SpawnType("guardian_of_darkness",-882,4418,505,179);
							else if (ent->client->pers.universe_quest_messages == 6)
								npc_ent = Zyk_NPC_SpawnType("guardian_of_eternity",-993,4273,505,90);
							else if (ent->client->pers.universe_quest_messages == 7)
								npc_ent = Zyk_NPC_SpawnType("guardian_of_universe",-988,4543,505,-90);

							if (npc_ent)
							{
								npc_ent->client->playerTeam = NPCTEAM_PLAYER;
								npc_ent->client->enemyTeam = NPCTEAM_ENEMY;
							}

							if (ent->client->pers.universe_quest_messages == 9)
								trap->SendServerCommand( ent->s.number, va("chat \"^2Guardian of Universe^7: Welcome, %s.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 10)
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: Guardians. Sages. I had a strange vision.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 11)
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: It is about a place which I believe it is the sacred t2_trip obelisk.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 12)
								trap->SendServerCommand( ent->s.number, va("chat \"^5Guardian of Light^7: Interesting. The place you are talking about...\""));
							else if (ent->client->pers.universe_quest_messages == 13)
								trap->SendServerCommand( ent->s.number, va("chat \"^1Guardian of Darkness^7: Is the Guardian of Time seal place!\""));
							else if (ent->client->pers.universe_quest_messages == 14)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Guardian of Eternity^7: Then the time has come!\""));
							else if (ent->client->pers.universe_quest_messages == 15)
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: Please tell me everything about it.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 16)
								trap->SendServerCommand( ent->s.number, va("chat \"^2Guardian of Universe^7: There is an old prophecy called the Prophecy of Time.\""));
							else if (ent->client->pers.universe_quest_messages == 17)
								trap->SendServerCommand( ent->s.number, va("chat \"^2Sage of Universe^7: It says the legendary hero will face...\""));
							else if (ent->client->pers.universe_quest_messages == 18)
								trap->SendServerCommand( ent->s.number, va("chat \"^5Sage of Light^7: The most powerful being ever.\""));
							else if (ent->client->pers.universe_quest_messages == 19)
								trap->SendServerCommand( ent->s.number, va("chat \"^1Sage of Darkness^7: He is sealed in a place called the Sacred Dimension...\""));
							else if (ent->client->pers.universe_quest_messages == 20)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Sage of Eternity^7: The only way to defeat him is going there.\""));
							else if (ent->client->pers.universe_quest_messages == 21)
								trap->SendServerCommand( ent->s.number, va("chat \"^2Guardian of Universe^7: But be aware that the Master of Evil is back.\""));
							else if (ent->client->pers.universe_quest_messages == 22)
								trap->SendServerCommand( ent->s.number, va("chat \"^2Guardian of Universe^7: He used my Resurrection Power to come back.\""));
							else if (ent->client->pers.universe_quest_messages == 23)
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: So I did not defeat him at all.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 24)
								trap->SendServerCommand( ent->s.number, va("chat \"^2Sage of Universe^7: He is trying to find the sacred crystals.\""));
							else if (ent->client->pers.universe_quest_messages == 25)
								trap->SendServerCommand( ent->s.number, va("chat \"^5Guardian of Light^7: these three crystals are the keys to open the gate to the Sacred Dimension.\""));
							else if (ent->client->pers.universe_quest_messages == 26)
								trap->SendServerCommand( ent->s.number, va("chat \"^1Guardian of Darkness^7: He wants to have all of the guardian powers.\""));
							else if (ent->client->pers.universe_quest_messages == 27)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Guardian of Eternity^7: You must find the crystals before him.\""));
							else if (ent->client->pers.universe_quest_messages == 28)
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: I will find the crystals. Thank you for the information.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 29)
							{
								trap->SendServerCommand( ent->s.number, va("chat \"^2Guardian of Universe^7: Now go hero. Do not give up.\""));

								ent->client->pers.universe_quest_counter |= (1 << 1);
								first_second_act_objective(ent);
								save_account(ent);
								quest_get_new_player(ent);
							}

							if (ent->client->pers.universe_quest_messages < 8)
							{
								ent->client->pers.universe_quest_messages++;
								ent->client->pers.universe_quest_timer = level.time + 1200;
							}
							else if (ent->client->pers.universe_quest_messages > 8)
							{
								ent->client->pers.universe_quest_messages++;
								ent->client->pers.universe_quest_timer = level.time + 5000;
							}
						}

						// zyk: Guardian of Universe battle
						if (ent->client->pers.universe_quest_progress == 7 && ent->client->pers.can_play_quest == 1 && ent->client->pers.guardian_mode == 0 && ent->client->pers.universe_quest_timer < level.time)
						{
							if ((int) ent->client->ps.origin[0] > 2746 && (int) ent->client->ps.origin[0] < 3123 && (int) ent->client->ps.origin[1] > 4728 && (int) ent->client->ps.origin[1] < 4994 && (int) ent->client->ps.origin[2] == 24)
							{
								if (ent->client->pers.universe_quest_messages == 0)
									trap->SendServerCommand( ent->s.number, va("chat \"^2Guardian of Universe: ^7Welcome, legendary hero %s^7! You finally reached my challenge.\"",ent->client->pers.netname));
								else if (ent->client->pers.universe_quest_messages == 1)
									trap->SendServerCommand( ent->s.number, "chat \"^2Guardian of Universe: ^7This battle will grant you the Universe Power if you defeat me.\"");
								else if (ent->client->pers.universe_quest_messages == 2)
									trap->SendServerCommand( ent->s.number, va("chat \"^2Guardian of Universe: ^7Get ready, %s^7!\"",ent->client->pers.netname));
								else if (ent->client->pers.universe_quest_messages == 3)
								{
									spawn_boss(ent,2742,4863,25,0,"guardian_of_universe",0,0,0,0,13);
								}

								if (ent->client->pers.universe_quest_messages < 4)
								{
									ent->client->pers.universe_quest_messages++;
									ent->client->pers.universe_quest_timer = level.time + 5000;
								}
							}
							
							if (ent->client->pers.universe_quest_messages == 5)
							{
								zyk_NPC_Kill_f("all"); // zyk: killing the guardian spawns
								trap->SendServerCommand( ent->s.number, va("chat \"^2Guardian of Universe: ^7Very well done, %s^7!\"",ent->client->pers.netname));
							}
							else if (ent->client->pers.universe_quest_messages == 6)
							{
								trap->SendServerCommand( ent->s.number, va("chat \"^2Guardian of Universe: ^7With your courage and hope, you defeated the Master of Evil and brought balance to the Universe once again.\""));
							}
							else if (ent->client->pers.universe_quest_messages == 7)
							{
								trap->SendServerCommand( ent->s.number, va("chat \"^2Guardian of Universe: ^7Now you will have the full strength of Universe Power.\""));
							}
							else if (ent->client->pers.universe_quest_messages == 8)
							{
								trap->SendServerCommand( ent->s.number, va("chat \"^2Guardian of Universe: ^7Farewell... brave hero! May the power of the guardians guide you in your journey!\""));
								
								ent->client->pers.universe_quest_progress = 8;
								ent->client->pers.universe_quest_messages = 0;

								save_account(ent);

								ent->client->pers.quest_power_status |= (1 << 13);

								quest_get_new_player(ent);
							}

							if (ent->client->pers.universe_quest_messages > 4 && ent->client->pers.universe_quest_messages < 8)
							{
								ent->client->pers.universe_quest_messages++;
								ent->client->pers.universe_quest_timer = level.time + 5000;
							}
						}
					}
					else if (level.quest_map == 13)
					{
						if (ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS && !(ent->client->pers.defeated_guardians & (1 << 5)) && ent->client->pers.can_play_quest == 1 && ent->client->pers.guardian_mode == 0 && (int) ent->client->ps.origin[0] > -2249 && (int) ent->client->ps.origin[0] < -2049 && (int) ent->client->ps.origin[1] > -4287 && (int) ent->client->ps.origin[1] < -4087 && (int) ent->client->ps.origin[2] == 3644)
						{
							if (ent->client->pers.light_quest_timer < level.time)
							{
								if (ent->client->pers.light_quest_messages == 0)
									trap->SendServerCommand( ent->s.number, va("chat \"^3Guardian of Earth: ^7I am the Guardian of Earth, %s^7! Try to defeat my strength and power!\"",ent->client->pers.netname));
								else if (ent->client->pers.light_quest_messages == 1)
								{
									spawn_boss(ent,-2149,-4187,3645,90,"guardian_boss_2",-2149,-4037,3645,-90,2);
								}
								ent->client->pers.light_quest_messages++;
								ent->client->pers.light_quest_timer = level.time + 3000;
							}
						}

						if (ent->client->pers.universe_quest_progress == 2 && ent->client->pers.can_play_quest == 1 && ent->client->pers.universe_quest_objective_control == 3 && ent->client->pers.universe_quest_timer < level.time && ent->client->pers.universe_quest_messages < 1 && !(ent->client->pers.universe_quest_counter & (1 << 5)))
						{
							gentity_t *npc_ent = NULL;
							npc_ent = Zyk_NPC_SpawnType("quest_mage",-584,296,5977,0);
							if (npc_ent)
							{
								npc_ent->client->ps.powerups[PW_FORCE_BOON] = level.time + 5500;

								npc_ent->client->pers.universe_quest_artifact_holder_id = ent-g_entities;
								ent->client->pers.universe_quest_artifact_holder_id = 5;
							}
							trap->SendServerCommand( ent->s.number, va("chat \"%s^7: I sense the presence of an artifact here.\"", ent->client->pers.netname));
						
							ent->client->pers.universe_quest_messages++;
							ent->client->pers.universe_quest_timer = level.time + 5000;
						}
					}
					else if (level.quest_map == 14)
					{
						if (ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS && !(ent->client->pers.defeated_guardians & (1 << 11)) && ent->client->pers.can_play_quest == 1 && ent->client->pers.guardian_mode == 0 && (int) ent->client->ps.origin[0] > -100 && (int) ent->client->ps.origin[0] < 100 && (int) ent->client->ps.origin[1] > 1035 && (int) ent->client->ps.origin[1] < 1235 && (int) ent->client->ps.origin[2] == 24)
						{
							if (ent->client->pers.light_quest_timer < level.time)
							{
								if (ent->client->pers.light_quest_messages == 0)
									trap->SendServerCommand( ent->s.number, va("chat \"^3Guardian of Resistance: ^7I am the Guardian of Resistance, %s^7! Your attacks are powerless!\"",ent->client->pers.netname));
								else if (ent->client->pers.light_quest_messages == 1)
								{
									spawn_boss(ent,0,1135,25,-90,"guardian_boss_8",0,905,25,90,11);
								}
								ent->client->pers.light_quest_messages++;
								ent->client->pers.light_quest_timer = level.time + 3000;
							}
						}
					}
					else if (level.quest_map == 15)
					{
						if (ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS && !(ent->client->pers.defeated_guardians & (1 << 10)) && ent->client->pers.can_play_quest == 1 && ent->client->pers.guardian_mode == 0 && (int) ent->client->ps.origin[0] > -253 && (int) ent->client->ps.origin[0] < -53 && (int) ent->client->ps.origin[1] > -555 && (int) ent->client->ps.origin[1] < -355 && (int) ent->client->ps.origin[2] == 216)
						{
							if (ent->client->pers.light_quest_timer < level.time)
							{
								if (ent->client->pers.light_quest_messages == 0)
									trap->SendServerCommand( ent->s.number, va("chat \"^7Guardian of Wind: ^7I am the Guardian of Wind, %s^7! Try to reach me in the air if you can!\"",ent->client->pers.netname));
								else if (ent->client->pers.light_quest_messages == 1)
								{
									spawn_boss(ent,-156,-298,217,-90,"guardian_boss_7",-156,-584,300,90,7);
								}
								ent->client->pers.light_quest_messages++;
								ent->client->pers.light_quest_timer = level.time + 3000;
							}
						}
					}
					else if (level.quest_map == 17)
					{
						if (ent->client->pers.universe_quest_progress == 8 && ent->client->pers.can_play_quest == 1 && !(ent->client->pers.universe_quest_counter & (1 << 2)) && ent->client->pers.universe_quest_timer < level.time && (int) ent->client->ps.origin[0] > -18684 && (int) ent->client->ps.origin[0] < -17485 && (int) ent->client->ps.origin[1] > 17652 && (int) ent->client->ps.origin[1] < 18781 && (int) ent->client->ps.origin[2] > 1505 && (int) ent->client->ps.origin[2] < 1850)
						{ // zyk: nineth Universe Quest mission. Guardian of Time part
							if (ent->client->pers.universe_quest_messages == 1)
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: This is the place of my vision.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 2)
								trap->SendServerCommand( ent->s.number, va("chat \"^7Guardian of Time^7: Indeed, %s.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 3)
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: Wait a second...are you...\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 4)
								trap->SendServerCommand( ent->s.number, va("chat \"^7Guardian of Time^7: Yes. I am the Guardian of Time.\""));
							else if (ent->client->pers.universe_quest_messages == 5)
								trap->SendServerCommand( ent->s.number, va("chat \"^7Guardian of Time^7: I am sealed in this place.\""));
							else if (ent->client->pers.universe_quest_messages == 6)
								trap->SendServerCommand( ent->s.number, va("chat \"^7Guardian of Time^7: The Guardian of Chaos did it to me.\""));
							else if (ent->client->pers.universe_quest_messages == 7)
								trap->SendServerCommand( ent->s.number, va("chat \"^7Guardian of Time^7: He wanted to conquer the entire Universe with my power.\""));
							else if (ent->client->pers.universe_quest_messages == 8)
								trap->SendServerCommand( ent->s.number, va("chat \"^7Guardian of Time^7: But before he sealed me, I actually sealed him in the Sacred Dimension.\""));
							else if (ent->client->pers.universe_quest_messages == 9)
								trap->SendServerCommand( ent->s.number, va("chat \"^7Guardian of Time^7: Probably you are having a lot of questions in your mind.\""));
							else if (ent->client->pers.universe_quest_messages == 10)
								trap->SendServerCommand( ent->s.number, va("chat \"^7Guardian of Time^7: Well, I am the true Guardian.\""));
							else if (ent->client->pers.universe_quest_messages == 11)
								trap->SendServerCommand( ent->s.number, va("chat \"^7Guardian of Time^7: The other guardians are just beings who got the amulets\""));
							else if (ent->client->pers.universe_quest_messages == 12)
								trap->SendServerCommand( ent->s.number, va("chat \"^7Guardian of Time^7: Understand this, their amulets are in fact parts of one.\""));
							else if (ent->client->pers.universe_quest_messages == 13)
								trap->SendServerCommand( ent->s.number, va("chat \"^7Guardian of Time^7: The amulet of time.\""));
							else if (ent->client->pers.universe_quest_messages == 14)
								trap->SendServerCommand( ent->s.number, va("chat \"^7Guardian of Time^7: This item, together with the sacred crystals, will open the gate...\""));
							else if (ent->client->pers.universe_quest_messages == 15)
								trap->SendServerCommand( ent->s.number, va("chat \"^7Guardian of Time^7: to the Sacred Dimension.\""));
							else if (ent->client->pers.universe_quest_messages == 16)
								trap->SendServerCommand( ent->s.number, va("chat \"^7Guardian of Time^7: you are the chosen one to do that hero.\""));
							else if (ent->client->pers.universe_quest_messages == 17)
								trap->SendServerCommand( ent->s.number, va("chat \"^7Guardian of Time^7: find the crystals and bring the guardian amulets here.\""));
							else if (ent->client->pers.universe_quest_messages == 18)
								trap->SendServerCommand( ent->s.number, va("chat \"^7Guardian of Time^7: the crystals will set me free, and I will recreate the amulet of time.\""));
							else if (ent->client->pers.universe_quest_messages == 19)
								trap->SendServerCommand( ent->s.number, va("chat \"^7Guardian of Time^7: Now go hero, someone is already searching for them, for evil purposes.\""));
							else if (ent->client->pers.universe_quest_messages == 20)
							{
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: I will find the crystals and set you free!\"", ent->client->pers.netname));

								ent->client->pers.universe_quest_counter |= (1 << 2);
								first_second_act_objective(ent);
								save_account(ent);
								quest_get_new_player(ent);
							}

							if (ent->client->pers.universe_quest_messages < 20)
							{
								ent->client->pers.universe_quest_messages++;
								ent->client->pers.universe_quest_timer = level.time + 5000;
							}
						}

						if (ent->client->pers.universe_quest_progress == 9 && ent->client->pers.can_play_quest == 1 && ent->client->pers.universe_quest_timer < level.time)
						{ // zyk: Universe Quest crystals mission
							gentity_t *npc_ent = NULL;

							if (!(ent->client->pers.universe_quest_counter & (1 << 0)))
							{ // zyk: Crystal of Destiny
								if (ent->client->pers.universe_quest_messages == 0)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",-3702,-2251,2232,90);
								else if (ent->client->pers.universe_quest_messages == 1)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",-3697,-2191,2232,91);
								else if (ent->client->pers.universe_quest_messages == 2)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",-3692,-2335,2232,-91);
								else if (ent->client->pers.universe_quest_messages == 3)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",-3813,-2252,2488,179);
								else if (ent->client->pers.universe_quest_messages == 4)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",-2588,-2252,2488,4);
								else if (ent->client->pers.universe_quest_messages == 5)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",-3702,-2252,2488,100);
								else if (ent->client->pers.universe_quest_messages == 6)
								{
									load_crystal_model(-3702,-2251,2211,90,0);
								}
							}
							else if (ent->client->pers.universe_quest_messages == 0)
							{ // zyk: if player has this crystal, try the next one
								ent->client->pers.universe_quest_messages = 6;
							}

							if (!(ent->client->pers.universe_quest_counter & (1 << 1)))
							{ // zyk: Crystal of Truth
								if (ent->client->pers.universe_quest_messages == 7)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",-14365,-696,1554,-179);
								else if (ent->client->pers.universe_quest_messages == 8)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",-14358,-284,1565,-179);
								else if (ent->client->pers.universe_quest_messages == 9)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",-13880,-520,1557,-179);
								else if (ent->client->pers.universe_quest_messages == 10)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",-15361,-600,1851,-179);
								else if (ent->client->pers.universe_quest_messages == 11)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",-14152,-523,1455,-179);
								else if (ent->client->pers.universe_quest_messages == 12)
								{
									load_crystal_model(-14152,-523,1455,-179,1);
								}
							}
							else if (ent->client->pers.universe_quest_messages == 6)
							{ // zyk: if player has this crystal, try the next one
								ent->client->pers.universe_quest_messages = 12;
							}

							if (!(ent->client->pers.universe_quest_counter & (1 << 2)))
							{ // zyk: Crystal of Time
								if (ent->client->pers.universe_quest_messages == 13)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",19904,-2740,1672,90);
								else if (ent->client->pers.universe_quest_messages == 14)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",19903,-2684,1672,91);
								else if (ent->client->pers.universe_quest_messages == 15)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",19751,-2669,1672,87);
								else if (ent->client->pers.universe_quest_messages == 16)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",20039,-2674,1672,86);
								else if (ent->client->pers.universe_quest_messages == 17)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",20073,-2326,1672,89);
								else if (ent->client->pers.universe_quest_messages == 18)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",19713,-2316,1672,83);
								else if (ent->client->pers.universe_quest_messages == 19)
								{
									load_crystal_model(19904,-2740,1651,90,2);
								}
							}
							else if (ent->client->pers.universe_quest_messages == 12)
							{ // zyk: if player has this crystal, do the crystal check
								ent->client->pers.universe_quest_messages = 19;
							}

							if (ent->client->pers.universe_quest_messages < 20)
							{ // zyk: only increments to next step if npc was properly placed in the map or a crystal was placed
								if (npc_ent || ent->client->pers.universe_quest_messages == 6 || ent->client->pers.universe_quest_messages == 12 ||
									ent->client->pers.universe_quest_messages == 19)
								{
									ent->client->pers.universe_quest_messages++;
								}

								ent->client->pers.universe_quest_timer = level.time + 1200;
							}
							else
							{
								ent->client->pers.universe_quest_timer = level.time + 200;

								if (level.quest_crystal_id[0] != -1 && 
									(int)Distance(ent->client->ps.origin,g_entities[level.quest_crystal_id[0]].r.currentOrigin) < 50)
								{
									ent->client->pers.universe_quest_counter |= (1 << 0);
									trap->SendServerCommand( ent->s.number, "chat \"^3Quest System^7: Got the ^4Crystal of Destiny^7.\"");
									clean_crystal_model(0);
									universe_crystals_check(ent);
								}
								else if (level.quest_crystal_id[1] != -1 && 
									(int)Distance(ent->client->ps.origin,g_entities[level.quest_crystal_id[1]].r.currentOrigin) < 50)
								{
									ent->client->pers.universe_quest_counter |= (1 << 1);
									trap->SendServerCommand( ent->s.number, "chat \"^3Quest System^7: Got the ^1Crystal of Truth^7.\"");
									clean_crystal_model(1);
									universe_crystals_check(ent);
								}
								else if (level.quest_crystal_id[2] != -1 && 
									(int)Distance(ent->client->ps.origin,g_entities[level.quest_crystal_id[2]].r.currentOrigin) < 50)
								{
									ent->client->pers.universe_quest_counter |= (1 << 2);
									trap->SendServerCommand( ent->s.number, "chat \"^3Quest System^7: Got the ^2Crystal of Time^7.\"");
									clean_crystal_model(2);
									universe_crystals_check(ent);
								}
							}
						}

						if (ent->client->pers.universe_quest_progress == 10 && ent->client->pers.can_play_quest == 1 && ent->client->pers.universe_quest_timer < level.time && (int) ent->client->ps.origin[0] > -18684 && (int) ent->client->ps.origin[0] < -17485 && (int) ent->client->ps.origin[1] > 17652 && (int) ent->client->ps.origin[1] < 18781 && (int) ent->client->ps.origin[2] > 1505 && (int) ent->client->ps.origin[2] < 1850)
						{ // zyk: eleventh objective of Universe Quest. Setting Guardian of Time free
							if (ent->client->pers.universe_quest_messages == 1)
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: This is it. I will use the crystals.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 2)
								Zyk_NPC_SpawnType("guardian_of_time",-18084,17970,1658,-90);
							else if (ent->client->pers.universe_quest_messages == 3)
								trap->SendServerCommand( ent->s.number, va("chat \"^7Guardian of Time^7: I am finally free.\""));
							else if (ent->client->pers.universe_quest_messages == 4)
								trap->SendServerCommand( ent->s.number, va("chat \"^7Guardian of Time^7: Thank you, hero.\""));
							else if (ent->client->pers.universe_quest_messages == 5)
								trap->SendServerCommand( ent->s.number, va("chat \"^7Guardian of Time^7: Give me your amulets, I will recreate the Amulet of Time.\""));
							else if (ent->client->pers.universe_quest_messages == 6)
								trap->SendServerCommand( ent->s.number, va("chat \"^7Guardian of Time^7: It is recreated. Now it will be possible to open the gate.\""));
							else if (ent->client->pers.universe_quest_messages == 7)
								trap->SendServerCommand( ent->s.number, va("chat \"^7Guardian of Time^7: To do that we also need the other chosen people here.\""));
							else if (ent->client->pers.universe_quest_messages == 8)
								trap->SendServerCommand( ent->s.number, va("chat \"^7Guardian of Time^7: I will call the other guardians and the sages with telepathy.\""));
							else if (ent->client->pers.universe_quest_messages == 9)
								trap->SendServerCommand( ent->s.number, va("chat \"^7Guardian of Time^7: They are needed in this moment.\""));
							else if (ent->client->pers.universe_quest_messages == 10)
								trap->SendServerCommand( ent->s.number, va("chat \"^7Guardian of Time^7: As the Prophecy of Time says, the sages, the guardians, you and me...\""));
							else if (ent->client->pers.universe_quest_messages == 11)
								trap->SendServerCommand( ent->s.number, va("chat \"^7Guardian of Time^7: And even the Master of Evil are required...\""));
							else if (ent->client->pers.universe_quest_messages == 12)
								trap->SendServerCommand( ent->s.number, va("chat \"^7Guardian of Time^7: To open the gate to the Sacred Dimension.\""));
							else if (ent->client->pers.universe_quest_messages == 13)
								trap->SendServerCommand( ent->s.number, va("chat \"^7Guardian of Time^7: You will understand why later.\""));
							else if (ent->client->pers.universe_quest_messages == 14)
								trap->SendServerCommand( ent->s.number, va("chat \"^7Guardian of Time^7: We must go to the Temple of the Gate.\""));
							else if (ent->client->pers.universe_quest_messages == 15)
								trap->SendServerCommand( ent->s.number, va("chat \"^7Guardian of Time^7: Just continue the path from here and you will find the temple.\""));
							else if (ent->client->pers.universe_quest_messages == 16)
								trap->SendServerCommand( ent->s.number, va("chat \"^7Guardian of Time^7: Beware, Master of Evil soldiers are guarding it right now.\""));
							else if (ent->client->pers.universe_quest_messages == 17)
								trap->SendServerCommand( ent->s.number, va("chat \"^7Guardian of Time^7: You will need to fight them.\""));
							else if (ent->client->pers.universe_quest_messages == 18)
							{
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: I understand. I will meet you all there.\"", ent->client->pers.netname));

								ent->client->pers.universe_quest_progress = 11;
								save_account(ent);
								quest_get_new_player(ent);
							}
							
							if (ent->client->pers.universe_quest_progress == 10 && ent->client->pers.universe_quest_messages < 18)
							{
								ent->client->pers.universe_quest_messages++;
								ent->client->pers.universe_quest_timer = level.time + 5000;
							}
						}

						if (ent->client->pers.universe_quest_progress == 11 && ent->client->pers.can_play_quest == 1)
						{ // zyk: mission of the battle for the temple in Universe Quest
							if (ent->client->pers.universe_quest_timer < level.time && (int) ent->client->ps.origin[0] > 7658 && (int) ent->client->ps.origin[0] < 17707 && (int) ent->client->ps.origin[1] > 5160 && (int) ent->client->ps.origin[1] < 12097 && (int) ent->client->ps.origin[2] > 1450 && (int) ent->client->ps.origin[2] < 5190)
							{
								gentity_t *npc_ent = NULL;

								if (ent->client->pers.universe_quest_messages == 0)
								{
									npc_ent = Zyk_NPC_SpawnType("guardian_of_time",9869,11957,1593,-52);
									trap->SendServerCommand( ent->s.number, "chat \"^7Guardian of Time: ^7Hero, we must defeat all of the soldiers.\"");
								}
								else if (ent->client->pers.universe_quest_messages == 1)
								{
									npc_ent = Zyk_NPC_SpawnType("sage_of_universe",9800,11904,1593,-52);
									trap->SendServerCommand( ent->s.number, "chat \"^2Sage of Universe: ^7You can count on us too.\"");
								}
								else if (ent->client->pers.universe_quest_messages == 2)
									npc_ent = Zyk_NPC_SpawnType("sage_of_light",9744,11862,1593,-52);
								else if (ent->client->pers.universe_quest_messages == 3)
									npc_ent = Zyk_NPC_SpawnType("sage_of_darkness",9687,11818,1593,-52);
								else if (ent->client->pers.universe_quest_messages == 4)
									npc_ent = Zyk_NPC_SpawnType("sage_of_eternity",9626,11771,1593,-52);
								else if (ent->client->pers.universe_quest_messages == 5)
								{
									npc_ent = Zyk_NPC_SpawnType("guardian_of_universe",9562,11722,1593,-52);
									if (npc_ent)
									{
										npc_ent->client->playerTeam = NPCTEAM_PLAYER;
										npc_ent->client->enemyTeam = NPCTEAM_ENEMY;
									}
									trap->SendServerCommand( ent->s.number, "chat \"^2Guardian of Universe: ^7We will also help.\"");
								}
								else if (ent->client->pers.universe_quest_messages == 6)
								{
									npc_ent = Zyk_NPC_SpawnType("guardian_boss_9",9498,11673,1593,-52);
									if (npc_ent)
									{
										npc_ent->client->playerTeam = NPCTEAM_PLAYER;
										npc_ent->client->enemyTeam = NPCTEAM_ENEMY;
									}
								}
								else if (ent->client->pers.universe_quest_messages == 7)
								{
									npc_ent = Zyk_NPC_SpawnType("guardian_of_darkness",9433,11623,1593,-52);
									if (npc_ent)
									{
										npc_ent->client->playerTeam = NPCTEAM_PLAYER;
										npc_ent->client->enemyTeam = NPCTEAM_ENEMY;
									}
								}
								else if (ent->client->pers.universe_quest_messages == 8)
								{
									npc_ent = Zyk_NPC_SpawnType("guardian_of_eternity",9335,11547,1593,-52);
									if (npc_ent)
									{
										npc_ent->client->playerTeam = NPCTEAM_PLAYER;
										npc_ent->client->enemyTeam = NPCTEAM_ENEMY;
									}
								}

								if (npc_ent)
									npc_ent->client->pers.universe_quest_objective_control = ent->s.number;

								if (ent->client->pers.universe_quest_messages < 9)
								{
									ent->client->pers.universe_quest_messages++;
									ent->client->pers.universe_quest_timer = level.time + 3000;
								}
							}

							if (ent->client->pers.hunter_quest_timer < level.time)
							{ // zyk: spawns the npcs at the temple
								gentity_t *npc_ent = NULL;

								if (ent->client->pers.hunter_quest_messages == 0)
								{
									int ent_iterator = 0;
									gentity_t *this_ent = NULL;

									// zyk: cleaning entities, except the spawn points. This will prevent server from crashing in this mission
									for (ent_iterator = level.maxclients; ent_iterator < level.num_entities; ent_iterator++)
									{
										this_ent = &g_entities[ent_iterator];

										if (this_ent && Q_stricmp( this_ent->classname, "info_player_deathmatch" ) != 0)
											G_FreeEntity(this_ent);
									}

									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",10212,8937,1593,-179);
								}
								else if (ent->client->pers.hunter_quest_messages == 1)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",10212,8828,1593,-179);
								else if (ent->client->pers.hunter_quest_messages == 2)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",10212,8680,1593,-179);
								else if (ent->client->pers.hunter_quest_messages == 3)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",10212,8550,1593,-179);
								else if (ent->client->pers.hunter_quest_messages == 4)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",10212,8355,1593,-179);
								else if (ent->client->pers.hunter_quest_messages == 5)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",10212,8164,1593,-179);
								else if (ent->client->pers.hunter_quest_messages == 6)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",9280,9246,1593,-179);
								else if (ent->client->pers.hunter_quest_messages == 7)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",9528,9425,1593,-179);
								else if (ent->client->pers.hunter_quest_messages == 8)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",9779,9426,1593,-179);
								else if (ent->client->pers.hunter_quest_messages == 9)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",9954,10016,1800,-179);
								else if (ent->client->pers.hunter_quest_messages == 10)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",9450,10524,1800,-179);
								else if (ent->client->pers.hunter_quest_messages == 12)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",9294,10583,1800,-179);
								else if (ent->client->pers.hunter_quest_messages == 13)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",9305,10418,1800,-179);
								else if (ent->client->pers.hunter_quest_messages == 14)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",9065,10415,1800,-179);
								else if (ent->client->pers.hunter_quest_messages == 15)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",9069,10244,1800,-179);
								else if (ent->client->pers.hunter_quest_messages == 16)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",10094,9199,1800,-179);
								else if (ent->client->pers.hunter_quest_messages == 17)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",10139,9282,1800,-179);
								else if (ent->client->pers.hunter_quest_messages == 18)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",10199,9392,1800,-179);
								else if (ent->client->pers.hunter_quest_messages == 19)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",10200,9777,1800,-179);
								else if (ent->client->pers.hunter_quest_messages == 20)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",10220,8239,1800,-179);
								else if (ent->client->pers.hunter_quest_messages == 21)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",10123,8343,1800,-179);

								if (npc_ent)
									npc_ent->client->pers.universe_quest_objective_control = ent->s.number;

								if (ent->client->pers.hunter_quest_messages < 11)
								{
									ent->client->pers.hunter_quest_messages++;
									ent->client->pers.hunter_quest_timer = level.time + 1500;
								}
								else if (ent->client->pers.hunter_quest_messages > 11 && ent->client->pers.hunter_quest_messages < 22)
								{
									ent->client->pers.hunter_quest_messages++;
									ent->client->pers.hunter_quest_timer = level.time + 2000;
								}
								else if (ent->client->pers.hunter_quest_messages == 40)
								{ // zyk: completed the mission
									ent->client->pers.universe_quest_progress = 12;
									save_account(ent);
									quest_get_new_player(ent);
								}
								else if (ent->client->pers.hunter_quest_messages == 50)
								{ // zyk: failed the mission
									quest_get_new_player(ent);
								}
							}
						}

						if (ent->client->pers.universe_quest_progress == 12 && ent->client->pers.can_play_quest == 1)
						{ // zyk: Universe Quest, The Final Revelation mission
							if (ent->client->pers.universe_quest_timer < level.time && (int) ent->client->ps.origin[0] > 9658 && (int) ent->client->ps.origin[0] < 12707 && (int) ent->client->ps.origin[1] > 8160 && (int) ent->client->ps.origin[1] < 9097 && (int) ent->client->ps.origin[2] > 1450 && (int) ent->client->ps.origin[2] < 2190)
							{
								if (ent->client->pers.universe_quest_messages == 1)
									trap->SendServerCommand( ent->s.number, "chat \"^7Guardian of Time: ^7Let's go inside the temple.\"");
								else if (ent->client->pers.universe_quest_messages == 2)
									trap->SendServerCommand( ent->s.number, "chat \"^1Master of Evil: ^7We meet again, hero.\"");
								else if (ent->client->pers.universe_quest_messages == 3)
									trap->SendServerCommand( ent->s.number, va("chat \"%s: ^7This time you will not resurrect.\"", ent->client->pers.netname));
								else if (ent->client->pers.universe_quest_messages == 4)
									trap->SendServerCommand( ent->s.number, "chat \"^1Master of Evil: ^7I can resurrect and you can beat me, so there is no point to fight.\"");
								else if (ent->client->pers.universe_quest_messages == 5)
									trap->SendServerCommand( ent->s.number, "chat \"^1Master of Evil: ^7I am here to show you the truth about your friends.\"");
								else if (ent->client->pers.universe_quest_messages == 6)
									trap->SendServerCommand( ent->s.number, va("chat \"%s: ^7Don't try to fool me.\"", ent->client->pers.netname));
								else if (ent->client->pers.universe_quest_messages == 7)
									trap->SendServerCommand( ent->s.number, "chat \"^1Master of Evil: ^7Listen to me! Your friends are fooling you.\"");
								else if (ent->client->pers.universe_quest_messages == 8)
									trap->SendServerCommand( ent->s.number, "chat \"^1Master of Evil: ^7I know about the Prophecy of Time, and with it I found out the truth.\"");
								else if (ent->client->pers.universe_quest_messages == 9)
									trap->SendServerCommand( ent->s.number, "chat \"^1Master of Evil: ^7Your friends helped you just to get access to the Sacred Dimension.\"");
								else if (ent->client->pers.universe_quest_messages == 10)
									trap->SendServerCommand( ent->s.number, "chat \"^1Master of Evil: ^7And only the hero can open the gate using the Amulet of Time and the Sacred Crystals.\"");
								else if (ent->client->pers.universe_quest_messages == 11)
									trap->SendServerCommand( ent->s.number, "chat \"^1Master of Evil: ^7The Prophecy also says that whoever kills the hero could also access it with these items.\"");
								else if (ent->client->pers.universe_quest_messages == 12)
									trap->SendServerCommand( ent->s.number, "chat \"^1Master of Evil: ^7That is why I tried to kill you. But you are too strong for me.\"");
								else if (ent->client->pers.universe_quest_messages == 13)
									trap->SendServerCommand( ent->s.number, "chat \"^1Master of Evil: ^7Ask the Guardian of Time. She is the True Guardian, she would not lie.\"");
								else if (ent->client->pers.universe_quest_messages == 14)
									trap->SendServerCommand( ent->s.number, "chat \"^7Guardian of Time: ^7Unfortunately, he is saying the truth.\"");
								else if (ent->client->pers.universe_quest_messages == 15)
									trap->SendServerCommand( ent->s.number, va("chat \"%s: ^7I can't believe it...so I was used by all of you...\"", ent->client->pers.netname));
								else if (ent->client->pers.universe_quest_messages == 16)
									trap->SendServerCommand( ent->s.number, "chat \"^7Guardian of Time: ^7All of us are the chosen people. We all have some powers of the Amulet of Time.\"");
								else if (ent->client->pers.universe_quest_messages == 17)
									trap->SendServerCommand( ent->s.number, "chat \"^7Guardian of Time: ^7We were necessary here so you can make the decisive choice.\"");
								else if (ent->client->pers.universe_quest_messages == 18)
									trap->SendServerCommand( ent->s.number, "chat \"^7Guardian of Time: ^7Each one of the chosen people have his own reasons to go to the Sacred Dimension.\"");
								else if (ent->client->pers.universe_quest_messages == 19)
									trap->SendServerCommand( ent->s.number, "chat \"^7Guardian of Time: ^7You will have to choose between four choices.\"");
								else if (ent->client->pers.universe_quest_messages == 20)
									trap->SendServerCommand( ent->s.number, "chat \"^7Guardian of Time: ^7Understand that, after choosing, the others will lose their powers.\"");
								else if (ent->client->pers.universe_quest_messages == 21)
									trap->SendServerCommand( ent->s.number, "chat \"^7Guardian of Time: ^7And these powers will be absorbed into the Amulet of Time.\"");
								else if (ent->client->pers.universe_quest_messages == 22)
									trap->SendServerCommand( ent->s.number, va("chat \"%s: ^7That is a difficult decision.\"", ent->client->pers.netname));
								else if (ent->client->pers.universe_quest_messages == 23)
									trap->SendServerCommand( ent->s.number, "chat \"^7Guardian of Time: ^7It is in the Prophecy of Time, there is no other way.\"");
								else if (ent->client->pers.universe_quest_messages == 24)
									trap->SendServerCommand( ent->s.number, "chat \"^7Guardian of Time: ^7Your decision will set the fate of the Universe.\"");
								else if (ent->client->pers.universe_quest_messages == 25)
								{
									ent->client->pers.universe_quest_progress = 13;
									save_account(ent);
									quest_get_new_player(ent);
								}

								if (ent->client->pers.universe_quest_progress == 12 && ent->client->pers.universe_quest_messages < 26)
								{
									ent->client->pers.universe_quest_messages++;
									ent->client->pers.universe_quest_timer = level.time + 5000;
								}
							}

							if (ent->client->pers.hunter_quest_timer < level.time)
							{
								gentity_t *npc_ent = NULL;

								if (ent->client->pers.hunter_quest_messages == 0)
								{
									npc_ent = Zyk_NPC_SpawnType("guardian_of_time",10894,8521,1700,0);
								}
								else if (ent->client->pers.hunter_quest_messages == 1)
								{
									npc_ent = Zyk_NPC_SpawnType("master_of_evil",11433,8499,1700,179);
									if (npc_ent)
									{
										npc_ent->client->playerTeam = NPCTEAM_PLAYER;
										npc_ent->client->enemyTeam = NPCTEAM_ENEMY;
									}
								}
								else if (ent->client->pers.hunter_quest_messages == 2)
								{
									npc_ent = Zyk_NPC_SpawnType("guardian_of_universe",10840,8627,1700,-90);
									if (npc_ent)
									{
										npc_ent->client->playerTeam = NPCTEAM_PLAYER;
										npc_ent->client->enemyTeam = NPCTEAM_ENEMY;
									}
								}
								else if (ent->client->pers.hunter_quest_messages == 3)
								{
									npc_ent = Zyk_NPC_SpawnType("guardian_of_eternity",10956,8627,1700,-90);
									if (npc_ent)
									{
										npc_ent->client->playerTeam = NPCTEAM_PLAYER;
										npc_ent->client->enemyTeam = NPCTEAM_ENEMY;
									}
								}
								else if (ent->client->pers.hunter_quest_messages == 4)
								{
									npc_ent = Zyk_NPC_SpawnType("guardian_boss_9",11085,8627,1700,-90);
									if (npc_ent)
									{
										npc_ent->client->playerTeam = NPCTEAM_PLAYER;
										npc_ent->client->enemyTeam = NPCTEAM_ENEMY;
									}
								}
								else if (ent->client->pers.hunter_quest_messages == 5)
								{
									npc_ent = Zyk_NPC_SpawnType("guardian_of_darkness",11234,8627,1700,-90);
									if (npc_ent)
									{
										npc_ent->client->playerTeam = NPCTEAM_PLAYER;
										npc_ent->client->enemyTeam = NPCTEAM_ENEMY;
									}
								}
								else if (ent->client->pers.hunter_quest_messages == 6)
								{
									npc_ent = Zyk_NPC_SpawnType("sage_of_universe",10840,8416,1700,90);
								}
								else if (ent->client->pers.hunter_quest_messages == 7)
								{
									npc_ent = Zyk_NPC_SpawnType("sage_of_light",10956,8416,1700,90);
								}
								else if (ent->client->pers.hunter_quest_messages == 8)
								{
									npc_ent = Zyk_NPC_SpawnType("sage_of_darkness",11085,8416,1700,90);
								}
								else if (ent->client->pers.hunter_quest_messages == 9)
								{
									npc_ent = Zyk_NPC_SpawnType("sage_of_eternity",11234,8416,1700,90);
								}

								if (ent->client->pers.hunter_quest_messages < 10)
								{
									ent->client->pers.hunter_quest_messages++;
									ent->client->pers.hunter_quest_timer = level.time + 1500;
								}
							}
						}

						if (ent->client->pers.universe_quest_progress == 13 && ent->client->pers.can_play_quest == 1)
						{ // zyk: Universe Quest, the choosing mission
							if (ent->client->pers.universe_quest_timer < level.time && (int) ent->client->ps.origin[0] > 9758 && (int) ent->client->ps.origin[0] < 14000 && (int) ent->client->ps.origin[1] > 8160 && (int) ent->client->ps.origin[1] < 9097 && (int) ent->client->ps.origin[2] > 1450 && (int) ent->client->ps.origin[2] < 2190)
							{
								if (ent->client->pers.universe_quest_messages == 1)
									trap->SendServerCommand( ent->s.number, "chat \"^2Sage of Universe: ^7Hero, you have to choose us.\"");
								else if (ent->client->pers.universe_quest_messages == 2)
									trap->SendServerCommand( ent->s.number, "chat \"^5Sage of Light: ^7the True Guardians must be people who listen to other opinions.\"");
								else if (ent->client->pers.universe_quest_messages == 3)
									trap->SendServerCommand( ent->s.number, "chat \"^1Sage of Darkness: ^7We will be guardians who will keep balance to the Universe.\"");
								else if (ent->client->pers.universe_quest_messages == 4)
									trap->SendServerCommand( ent->s.number, "chat \"^3Sage of Eternity: ^7And by choosing us, we will give you a new power.\"");
								else if (ent->client->pers.universe_quest_messages == 5)
									trap->SendServerCommand( ent->s.number, "chat \"^2Sage of Universe: ^7It is the ^2Ultra Drain^7. Choose wisely, hero.\"");
								else if (ent->client->pers.universe_quest_messages == 6)
									trap->SendServerCommand( ent->s.number, "chat \"^2Guardian of Universe: ^7Hero, by choosing us...\"");
								else if (ent->client->pers.universe_quest_messages == 7)
									trap->SendServerCommand( ent->s.number, "chat \"^1Guardian of Darkness: ^7We will use our wisdom and power to keep the balance in the Universe.\"");
								else if (ent->client->pers.universe_quest_messages == 8)
									trap->SendServerCommand( ent->s.number, "chat \"^3Guardian of Eternity: ^7We are old beings, we have much wisdom.\"");
								else if (ent->client->pers.universe_quest_messages == 9)
									trap->SendServerCommand( ent->s.number, "chat \"^5Guardian of Light: ^7If you choose us, we will give you the ^3Immunity Power^7.\"");
								else if (ent->client->pers.universe_quest_messages == 10)
									trap->SendServerCommand( ent->s.number, "chat \"^2Guardian of Universe: ^7This power protects you from other special powers.\"");
								else if (ent->client->pers.universe_quest_messages == 11)
									trap->SendServerCommand( ent->s.number, "chat \"^2Guardian of Universe: ^7Think about that when making your choice.\"");
								else if (ent->client->pers.universe_quest_messages == 12)
									trap->SendServerCommand( ent->s.number, "chat \"^1Master of Evil: ^7Don't be a fool, don't listen to these guys.\"");
								else if (ent->client->pers.universe_quest_messages == 13)
									trap->SendServerCommand( ent->s.number, "chat \"^1Master of Evil: ^7They used you just to get more power.\"");
								else if (ent->client->pers.universe_quest_messages == 14)
									trap->SendServerCommand( ent->s.number, "chat \"^1Master of Evil: ^7At least I am true in my ambitions.\"");
								else if (ent->client->pers.universe_quest_messages == 15)
									trap->SendServerCommand( ent->s.number, "chat \"^1Master of Evil: ^7Hero, only power matters in this life. The weak deserve to die!\"");
								else if (ent->client->pers.universe_quest_messages == 16)
									trap->SendServerCommand( ent->s.number, "chat \"^1Master of Evil: ^7There will never be peace in Universe.\"");
								else if (ent->client->pers.universe_quest_messages == 17)
									trap->SendServerCommand( ent->s.number, "chat \"^1Master of Evil: ^7Choose me and I will make you powerful.\"");
								else if (ent->client->pers.universe_quest_messages == 18)
									trap->SendServerCommand( ent->s.number, "chat \"^1Master of Evil: ^7I will give you the ^1Chaos Power^7. I read about it in the Prophecy of Time.\"");
								else if (ent->client->pers.universe_quest_messages == 19)
									trap->SendServerCommand( ent->s.number, "chat \"^1Master of Evil: ^7The Guardian of Chaos has it, and I will give it to you.\"");
								else if (ent->client->pers.universe_quest_messages == 20)
									trap->SendServerCommand( ent->s.number, "chat \"^1Master of Evil: ^7Choose me and you won't regret it.\"");
								else if (ent->client->pers.universe_quest_messages == 21)
									trap->SendServerCommand( ent->s.number, "chat \"^7Guardian of Time: ^7Hero, listen to me.\"");
								else if (ent->client->pers.universe_quest_messages == 22)
									trap->SendServerCommand( ent->s.number, "chat \"^7Guardian of Time: ^7I am one of the oldest beings in the Universe.\"");
								else if (ent->client->pers.universe_quest_messages == 23)
									trap->SendServerCommand( ent->s.number, "chat \"^7Guardian of Time: ^7Only I have the wisdom to be the True Guardian.\"");
								else if (ent->client->pers.universe_quest_messages == 24)
									trap->SendServerCommand( ent->s.number, "chat \"^7Guardian of Time: ^7Choose me, and I will give you the Time Power.\"");
								else if (ent->client->pers.universe_quest_messages == 25)
									trap->SendServerCommand( ent->s.number, "chat \"^7Guardian of Time: ^7It will allow you to paralyze enemies for a short time.\"");
								else if (ent->client->pers.universe_quest_messages == 26)
									trap->SendServerCommand( ent->s.number, "chat \"^7Guardian of Time: ^7Now, make your decision...\"");
								else if (ent->client->pers.universe_quest_messages == 27)
									trap->SendServerCommand( ent->s.number, "chat \"^7Guardian of Time: ^7Remember, after choosing someone, you cannot regret it.\"");
								else if (ent->client->pers.universe_quest_messages == 29)
								{ // zyk: completed the mission
									ent->client->pers.universe_quest_progress = 14;
									save_account(ent);
									quest_get_new_player(ent);
								}

								if (ent->client->pers.universe_quest_progress == 13 && ent->client->pers.universe_quest_messages < 28)
								{
									ent->client->pers.universe_quest_messages++;
									ent->client->pers.universe_quest_timer = level.time + 5000;
								}
							}

							if (ent->client->pers.hunter_quest_timer < level.time)
							{
								gentity_t *npc_ent = NULL;

								if (ent->client->pers.hunter_quest_messages == 0)
								{
									npc_ent = Zyk_NPC_SpawnType("guardian_of_time",10894,8521,1700,0);
								}
								else if (ent->client->pers.hunter_quest_messages == 1)
								{
									npc_ent = Zyk_NPC_SpawnType("master_of_evil",11433,8499,1700,179);
									if (npc_ent)
									{
										npc_ent->client->playerTeam = NPCTEAM_PLAYER;
										npc_ent->client->enemyTeam = NPCTEAM_ENEMY;
									}
								}
								else if (ent->client->pers.hunter_quest_messages == 2)
								{
									npc_ent = Zyk_NPC_SpawnType("guardian_of_universe",10840,8627,1700,-90);
									if (npc_ent)
									{
										npc_ent->client->playerTeam = NPCTEAM_PLAYER;
										npc_ent->client->enemyTeam = NPCTEAM_ENEMY;
									}
								}
								else if (ent->client->pers.hunter_quest_messages == 3)
								{
									npc_ent = Zyk_NPC_SpawnType("guardian_of_eternity",10956,8627,1700,-90);
									if (npc_ent)
									{
										npc_ent->client->playerTeam = NPCTEAM_PLAYER;
										npc_ent->client->enemyTeam = NPCTEAM_ENEMY;
									}
								}
								else if (ent->client->pers.hunter_quest_messages == 4)
								{
									npc_ent = Zyk_NPC_SpawnType("guardian_boss_9",11085,8627,1700,-90);
									if (npc_ent)
									{
										npc_ent->client->playerTeam = NPCTEAM_PLAYER;
										npc_ent->client->enemyTeam = NPCTEAM_ENEMY;
									}
								}
								else if (ent->client->pers.hunter_quest_messages == 5)
								{
									npc_ent = Zyk_NPC_SpawnType("guardian_of_darkness",11234,8627,1700,-90);
									if (npc_ent)
									{
										npc_ent->client->playerTeam = NPCTEAM_PLAYER;
										npc_ent->client->enemyTeam = NPCTEAM_ENEMY;
									}
								}
								else if (ent->client->pers.hunter_quest_messages == 6)
								{
									npc_ent = Zyk_NPC_SpawnType("sage_of_universe",10840,8416,1700,90);
								}
								else if (ent->client->pers.hunter_quest_messages == 7)
								{
									npc_ent = Zyk_NPC_SpawnType("sage_of_light",10956,8416,1700,90);
								}
								else if (ent->client->pers.hunter_quest_messages == 8)
								{
									npc_ent = Zyk_NPC_SpawnType("sage_of_darkness",11085,8416,1700,90);
								}
								else if (ent->client->pers.hunter_quest_messages == 9)
								{
									npc_ent = Zyk_NPC_SpawnType("sage_of_eternity",11234,8416,1700,90);
								}

								if (ent->client->pers.hunter_quest_messages < 10)
								{
									ent->client->pers.hunter_quest_messages++;
									ent->client->pers.hunter_quest_timer = level.time + 1500;
								}
							}
						}

						if (level.chaos_portal_id != -1)
						{ // zyk: portal at the last universe quest mission. Teleports players to Sacred Dimension
							gentity_t *chaos_portal = &g_entities[level.chaos_portal_id];

							if (chaos_portal && (int)Distance(chaos_portal->s.origin,ent->client->ps.origin) < 40)
							{
								vec3_t origin;
								vec3_t angles;
								int npc_iterator = 0;
								gentity_t *this_ent = NULL;

								// zyk: cleaning vehicles to prevent some exploits
								for (npc_iterator = level.maxclients; npc_iterator < level.num_entities; npc_iterator++)
								{
									this_ent = &g_entities[npc_iterator];

									if (this_ent && this_ent->NPC && this_ent->client->NPC_class == CLASS_VEHICLE && this_ent->die)
										this_ent->die(this_ent, this_ent, this_ent, 100, MOD_UNKNOWN);
								}

								origin[0] = -1915.0f;
								origin[1] = -26945.0f;
								origin[2] = 300.0f;
								angles[0] = 0.0f;
								angles[1] = -179.0f;
								angles[2] = 0.0f;

								zyk_TeleportPlayer(ent,origin,angles);
							}
						}

						if (ent->client->pers.universe_quest_progress == 14 && ent->client->pers.can_play_quest == 1)
						{ // zyk: Universe Quest final mission
							if (ent->client->pers.guardian_mode == 0 && ent->client->pers.universe_quest_timer < level.time)
							{
								gentity_t *new_ent = NULL;

								if (ent->client->pers.universe_quest_messages == 1)
									new_ent = load_crystal_model(12614,8430,1497,179,0);
								else if (ent->client->pers.universe_quest_messages == 2)
									new_ent = load_crystal_model(12614,8570,1497,179,1);
								else if (ent->client->pers.universe_quest_messages == 3)
									new_ent = load_crystal_model(12734,8500,1497,179,2);
								else if (ent->client->pers.universe_quest_messages == 4)
								{
									new_ent = load_effect(12614,8430,1512,0,"env/vbolt");
									G_Sound(new_ent, CHAN_AUTO, G_SoundIndex("sound/effects/tram_boost.mp3"));
								}
								else if (ent->client->pers.universe_quest_messages == 5)
								{
									new_ent = load_effect(12614,8570,1512,0,"env/vbolt");
									G_Sound(new_ent, CHAN_AUTO, G_SoundIndex("sound/effects/tram_boost.mp3"));
								}
								else if (ent->client->pers.universe_quest_messages == 6)
								{
									new_ent = load_effect(12734,8500,1512,0,"env/vbolt");
									G_Sound(new_ent, CHAN_AUTO, G_SoundIndex("sound/effects/tram_boost.mp3"));
								}
								else if (ent->client->pers.universe_quest_messages == 7)
									new_ent = load_effect(12668,8500,1510,0,"env/btend");
								else if (ent->client->pers.universe_quest_messages == 8)
								{
									new_ent = load_effect(12668,8500,1512,0,"env/hevil_bolt");
									G_Sound(new_ent, CHAN_AUTO, G_SoundIndex("sound/effects/tractorbeam.mp3"));

									level.chaos_portal_id = new_ent->s.number;
								}
								else if (ent->client->pers.universe_quest_messages == 9)
								{ // zyk: teleports the quest player to the Sacred Dimension
									if ((int) ent->client->ps.origin[0] > -2415 && (int) ent->client->ps.origin[0] < -1415 && (int) ent->client->ps.origin[1] > -27445 && (int) ent->client->ps.origin[1] < -26445 && (int) ent->client->ps.origin[2] > 100 && (int) ent->client->ps.origin[2] < 400)
									{
										ent->client->pers.universe_quest_messages = 10;
									}
								}
								else if (ent->client->pers.universe_quest_messages == 11)
								{
									trap->SendServerCommand( ent->s.number, "chat \"^1Guardian of Chaos: ^7So, the legendary hero is finally here.\"");
								}
								else if (ent->client->pers.universe_quest_messages == 12)
								{
									trap->SendServerCommand( ent->s.number, va("chat \"%s^7: ^7I'm here to defeat you and restore the balance to the Universe.\"", ent->client->pers.netname));
								}
								else if (ent->client->pers.universe_quest_messages == 13)
								{
									trap->SendServerCommand( ent->s.number, "chat \"^1Guardian of Chaos: ^7The Guardian of Time could not defeat me. She was weak.\"");
								}
								else if (ent->client->pers.universe_quest_messages == 14)
								{
									trap->SendServerCommand( ent->s.number, "chat \"^1Guardian of Chaos: ^7In fact, you fell into my trap. I am finally free!\"");
								}
								else if (ent->client->pers.universe_quest_messages == 15)
								{
									trap->SendServerCommand( ent->s.number, "chat \"^1Guardian of Chaos: ^7The time of the Prophecy of Time has come. I have waited so much for this moment.\"");
								}
								else if (ent->client->pers.universe_quest_messages == 16)
								{
									trap->SendServerCommand( ent->s.number, "chat \"^1Guardian of Chaos: ^7I will be able to conquer the Universe! I will fill it with chaos, destruction and suffering!\"");
								}
								else if (ent->client->pers.universe_quest_messages == 17)
								{
									trap->SendServerCommand( ent->s.number, "chat \"^1Guardian of Chaos: ^7You cannot win this battle. You will meet your doom!\"");
								}
								else if (ent->client->pers.universe_quest_messages == 18)
								{ // zyk: here starts the battle against the Guardian of Chaos
									int npc_iterator = 0;
									gentity_t *this_ent = NULL;

									// zyk: cleaning npcs that are not the quest ones
									for (npc_iterator = MAX_CLIENTS; npc_iterator < level.num_entities; npc_iterator++)
									{
										this_ent = &g_entities[npc_iterator];

										if (this_ent && this_ent->NPC && this_ent->die && Q_stricmp( this_ent->NPC_type, "sage_of_light" ) != 0 && Q_stricmp( this_ent->NPC_type, "sage_of_darkness" ) != 0 && Q_stricmp( this_ent->NPC_type, "sage_of_eternity" ) != 0 && Q_stricmp( this_ent->NPC_type, "sage_of_universe" ) != 0 && Q_stricmp( this_ent->NPC_type, "guardian_of_time" ) != 0 && Q_stricmp( this_ent->NPC_type, "guardian_boss_9" ) != 0 && Q_stricmp( this_ent->NPC_type, "guardian_of_darkness" ) != 0 && Q_stricmp( this_ent->NPC_type, "guardian_of_eternity" ) != 0 && Q_stricmp( this_ent->NPC_type, "guardian_of_universe" ) != 0 && Q_stricmp( this_ent->NPC_type, "master_of_evil" ) != 0 && Q_stricmp( this_ent->NPC_type, "jawa_seller" ) != 0)
											this_ent->die(this_ent, this_ent, this_ent, 100, MOD_UNKNOWN);
									}

									spawn_boss(ent,-3136,-26946,200,179,"guardian_of_chaos",-4228,-26946,393,0,14);
								}
								else if (ent->client->pers.universe_quest_messages == 20)
								{
									vec3_t origin;
									vec3_t angles;

									int npc_iterator = 0;
									char npc_name[128];
									gentity_t *npc_ent = NULL;

									origin[0] = -1915.0f;
									origin[1] = -26945.0f;
									origin[2] = 300.0f;
									angles[0] = 0.0f;
									angles[1] = -179.0f;
									angles[2] = 0.0f;

									strcpy(npc_name,"");

									if (ent->client->pers.universe_quest_counter & (1 << 0))
										strcpy(npc_name,"sage_of_universe");
									else if (ent->client->pers.universe_quest_counter & (1 << 1))
										strcpy(npc_name,"guardian_of_universe");
									else if (ent->client->pers.universe_quest_counter & (1 << 2))
										strcpy(npc_name,"master_of_evil");
									else if (ent->client->pers.universe_quest_counter & (1 << 3))
										strcpy(npc_name,"guardian_of_time");

									for (npc_iterator = 0; npc_iterator < level.num_entities; npc_iterator++)
									{
										npc_ent = &g_entities[npc_iterator];

										if (Q_stricmp( npc_ent->NPC_type, npc_name ) == 0)
										{
											zyk_TeleportPlayer(npc_ent,origin,angles);
											break;
										}
									}

									trap->SendServerCommand( ent->s.number, "chat \"^1Guardian of Chaos: ^7How is this possible! ... I cannot lose...\"");
								}
								else if (ent->client->pers.universe_quest_messages == 21)
								{
									if (ent->client->pers.universe_quest_counter & (1 << 0))
										trap->SendServerCommand( ent->s.number, "chat \"^2Sage of Universe: ^7Amazing! You beat the Guardian of Chaos! Well done!\"");
									else if (ent->client->pers.universe_quest_counter & (1 << 1))
										trap->SendServerCommand( ent->s.number, "chat \"^2Guardian of Universe: ^7Thank you, hero! You defeated him!\"");
									else if (ent->client->pers.universe_quest_counter & (1 << 2))
										trap->SendServerCommand( ent->s.number, "chat \"^1Master of Evil: ^7Muahahahaha! That's really nice what you have done here!\"");
									else if (ent->client->pers.universe_quest_counter & (1 << 3))
										trap->SendServerCommand( ent->s.number, "chat \"^7Guardian of Time: ^7Well done, hero. You finished what I started a long time ago.\"");
								}
								else if (ent->client->pers.universe_quest_messages == 22)
								{
									if (ent->client->pers.universe_quest_counter & (1 << 0))
										trap->SendServerCommand( ent->s.number, "chat \"^2Sage of Universe: ^7I will now receive the full power of the Amulet of Time.\"");
									else if (ent->client->pers.universe_quest_counter & (1 << 1))
										trap->SendServerCommand( ent->s.number, "chat \"^2Guardian of Universe: ^7Now the Amulet of Time will have its full power.\"");
									else if (ent->client->pers.universe_quest_counter & (1 << 2))
										trap->SendServerCommand( ent->s.number, "chat \"^1Master of Evil: ^7Power! That's it! The Amulet of Time, fully powered, and it is all mine!\"");
									else if (ent->client->pers.universe_quest_counter & (1 << 3))
										trap->SendServerCommand( ent->s.number, "chat \"^7Guardian of Time: ^7The Amulet of Time will again receive its full power.\"");
								}
								else if (ent->client->pers.universe_quest_messages == 23)
								{
									if (ent->client->pers.universe_quest_counter & (1 << 0))
										trap->SendServerCommand( ent->s.number, "chat \"^2Sage of Universe: ^7I am the True Guardian now. We sages will keep balance to the Universe.\"");
									else if (ent->client->pers.universe_quest_counter & (1 << 1))
										trap->SendServerCommand( ent->s.number, "chat \"^2Guardian of Universe: ^7I have become the True Guardian. With this power, the guardians will keep balance to the Universe.\"");
									else if (ent->client->pers.universe_quest_counter & (1 << 2))
										trap->SendServerCommand( ent->s.number, "chat \"^1Master of Evil: ^7I am the True Guardian! With this power I can now be unstopable!\"");
									else if (ent->client->pers.universe_quest_counter & (1 << 3))
										trap->SendServerCommand( ent->s.number, "chat \"^7Guardian of Time: ^7Once again, I am the True Guardian. This time, the Universe will truly be in balance.\"");
								}
								else if (ent->client->pers.universe_quest_messages == 24)
								{
									if (ent->client->pers.universe_quest_counter & (1 << 0))
										trap->SendServerCommand( ent->s.number, "chat \"^2Sage of Universe: ^7Receive the ^2Resurrection Power ^7now. This will really be useful to you.\"");
									else if (ent->client->pers.universe_quest_counter & (1 << 1))
										trap->SendServerCommand( ent->s.number, "chat \"^2Guardian of Universe: ^7Now I will give you the ^3Resurrection Power. ^7Use it when necessary.\"");
									else if (ent->client->pers.universe_quest_counter & (1 << 2))
										trap->SendServerCommand( ent->s.number, "chat \"^1Master of Evil: ^7Now, you will become my instrument of conquest. Get this ^1Resurrection Power^7.\"");
									else if (ent->client->pers.universe_quest_counter & (1 << 3))
										trap->SendServerCommand( ent->s.number, "chat \"^7Guardian of Time: ^7As I told you, you will now receive the Resurrection Power. Use it wisely, hero.\"");
								}
								else if (ent->client->pers.universe_quest_messages == 25)
								{
									if (ent->client->pers.universe_quest_counter & (1 << 0))
										trap->SendServerCommand( ent->s.number, "chat \"^2Sage of Universe: ^7Go now and enjoy your life, my friend.\"");
									else if (ent->client->pers.universe_quest_counter & (1 << 1))
										trap->SendServerCommand( ent->s.number, "chat \"^2Guardian of Universe: ^7You have to go now hero. Be in peace.\"");
									else if (ent->client->pers.universe_quest_counter & (1 << 2))
										trap->SendServerCommand( ent->s.number, "chat \"^1Master of Evil: ^7Now go and conquer everything. Destroy all the weak!\"");
									else if (ent->client->pers.universe_quest_counter & (1 << 3))
										trap->SendServerCommand( ent->s.number, "chat \"^7Guardian of Time: ^7It is time for you to go. I wish you well in your life.\"");
								}
								else if (ent->client->pers.universe_quest_messages == 26)
								{ // zyk: end of the Universe Quest
									vec3_t origin;
									vec3_t angles;

									origin[0] = 12500.0f;
									origin[1] = 8500.0f;
									origin[2] = 1520.0f;
									angles[0] = 0.0f;
									angles[1] = 179.0f;
									angles[2] = 0.0f;

									zyk_TeleportPlayer(ent,origin,angles);

									ent->client->pers.universe_quest_progress = 15;
									save_account(ent);
									quest_get_new_player(ent);
								}

								if (new_ent)
								{
									new_ent->targetname = "zyk_quest_models";
									new_ent = NULL;
								}

								if (ent->client->pers.universe_quest_messages > 0 && ent->client->pers.universe_quest_messages < 9)
								{
									ent->client->pers.universe_quest_messages++;

									// zyk: teleport can instantly teleport player, so no delay should be added
									if (ent->client->pers.universe_quest_messages < 9)
										ent->client->pers.universe_quest_timer = level.time + 2000;
								}
								else if (ent->client->pers.universe_quest_messages > 9 && ent->client->pers.universe_quest_messages < 19)
								{
									ent->client->pers.universe_quest_messages++;
									ent->client->pers.universe_quest_timer = level.time + 5000;
								}
								else if (ent->client->pers.universe_quest_messages > 19 && ent->client->pers.universe_quest_messages < 27)
								{
									ent->client->pers.universe_quest_messages++;
									ent->client->pers.universe_quest_timer = level.time + 5000;
								}
							}

							if (ent->client->pers.guardian_mode == 0 && ent->client->pers.hunter_quest_timer < level.time && (int) ent->client->ps.origin[0] > 10000 && (int) ent->client->ps.origin[0] < 14000 && (int) ent->client->ps.origin[1] > 8160 && (int) ent->client->ps.origin[1] < 9097 && (int) ent->client->ps.origin[2] > 1450 && (int) ent->client->ps.origin[2] < 2000)
							{
								gentity_t *npc_ent = NULL;
								
								if (ent->client->pers.hunter_quest_messages == 0)
								{
									int effect_iterator = 0;
									gentity_t *this_ent = NULL;

									// zyk: cleaning the crystals and effects if they are already spawned
									for (effect_iterator = (MAX_CLIENTS + BODY_QUEUE_SIZE); effect_iterator < level.num_entities; effect_iterator++)
									{
										this_ent = &g_entities[effect_iterator];

										if (this_ent && (this_ent->NPC || Q_stricmp(this_ent->targetname, "zyk_quest_models") == 0 || 
											Q_stricmp(this_ent->classname, "npc_spawner") == 0 || Q_stricmp(this_ent->classname, "npc_vehicle") == 0 || 
											Q_stricmp(this_ent->classname, "npc_human_merc") == 0))
										{ // zyk: cleaning quest models, npcs and vehicles
											G_FreeEntity(this_ent);
										}
									}

									if (ent->client->pers.universe_quest_counter & (1 << 3))
									{
										npc_ent = Zyk_NPC_SpawnType("guardian_of_time",12834,8500,1700,179);
										trap->SendServerCommand( ent->s.number, "chat \"^7Guardian of Time: ^7I will use the Amulet of Time and the Sacred Crystals to open the gate to the Sacred Dimension.\"");
									}
									else
										npc_ent = Zyk_NPC_SpawnType("guardian_of_time",10894,8521,1700,0);

									if (npc_ent)
										npc_ent->client->pers.universe_quest_messages = -2000;
								}
								else if (ent->client->pers.hunter_quest_messages == 1)
								{
									if (ent->client->pers.universe_quest_counter & (1 << 2))
									{
										npc_ent = Zyk_NPC_SpawnType("master_of_evil",12834,8500,1700,179);
										trap->SendServerCommand( ent->s.number, "chat \"^7Guardian of Time: ^7Master of Evil, use the Amulet of Time and the Sacred Crystals to open the gate to the Sacred Dimension.\"");
									}
									else
										npc_ent = Zyk_NPC_SpawnType("master_of_evil",11433,8499,1700,179);
									if (npc_ent)
									{
										npc_ent->client->playerTeam = NPCTEAM_PLAYER;
										npc_ent->client->enemyTeam = NPCTEAM_ENEMY;
										npc_ent->client->pers.universe_quest_messages = -2000;
									}
								}
								else if (ent->client->pers.hunter_quest_messages == 2)
								{
									if (ent->client->pers.universe_quest_counter & (1 << 1))
									{
										npc_ent = Zyk_NPC_SpawnType("guardian_of_universe",12834,8500,1700,179);
										trap->SendServerCommand( ent->s.number, "chat \"^7Guardian of Time: ^7Guardian of Universe, use the Amulet of Time and the Sacred Crystals to open the gate to the Sacred Dimension.\"");
									}
									else
										npc_ent = Zyk_NPC_SpawnType("guardian_of_universe",10840,8627,1700,-90);
									if (npc_ent)
									{
										npc_ent->client->playerTeam = NPCTEAM_PLAYER;
										npc_ent->client->enemyTeam = NPCTEAM_ENEMY;
										npc_ent->client->pers.universe_quest_messages = -2000;
									}
								}
								else if (ent->client->pers.hunter_quest_messages == 3)
								{
									npc_ent = Zyk_NPC_SpawnType("guardian_of_eternity",10956,8627,1700,-90);
									if (npc_ent)
									{
										npc_ent->client->playerTeam = NPCTEAM_PLAYER;
										npc_ent->client->enemyTeam = NPCTEAM_ENEMY;
										npc_ent->client->pers.universe_quest_messages = -2000;
									}
								}
								else if (ent->client->pers.hunter_quest_messages == 4)
								{
									npc_ent = Zyk_NPC_SpawnType("guardian_boss_9",11085,8627,1700,-90);
									if (npc_ent)
									{
										npc_ent->client->playerTeam = NPCTEAM_PLAYER;
										npc_ent->client->enemyTeam = NPCTEAM_ENEMY;
										npc_ent->client->pers.universe_quest_messages = -2000;
									}
								}
								else if (ent->client->pers.hunter_quest_messages == 5)
								{
									npc_ent = Zyk_NPC_SpawnType("guardian_of_darkness",11234,8627,1700,-90);
									if (npc_ent)
									{
										npc_ent->client->playerTeam = NPCTEAM_PLAYER;
										npc_ent->client->enemyTeam = NPCTEAM_ENEMY;
										npc_ent->client->pers.universe_quest_messages = -2000;
									}
								}
								else if (ent->client->pers.hunter_quest_messages == 6)
								{
									if (ent->client->pers.universe_quest_counter & (1 << 0))
									{
										npc_ent = Zyk_NPC_SpawnType("sage_of_universe",12834,8500,1700,179);
										trap->SendServerCommand( ent->s.number, "chat \"^7Guardian of Time: ^7Sage of Universe, use the Amulet of Time and the Sacred Crystals to open the gate to the Sacred Dimension.\"");
									}
									else
										npc_ent = Zyk_NPC_SpawnType("sage_of_universe",10840,8416,1700,90);

									if (npc_ent)
										npc_ent->client->pers.universe_quest_messages = -2000;
								}
								else if (ent->client->pers.hunter_quest_messages == 7)
								{
									npc_ent = Zyk_NPC_SpawnType("sage_of_light",10956,8416,1700,90);
									if (npc_ent)
										npc_ent->client->pers.universe_quest_messages = -2000;
								}
								else if (ent->client->pers.hunter_quest_messages == 8)
								{
									npc_ent = Zyk_NPC_SpawnType("sage_of_darkness",11085,8416,1700,90);
									if (npc_ent)
										npc_ent->client->pers.universe_quest_messages = -2000;
								}
								else if (ent->client->pers.hunter_quest_messages == 9)
								{
									npc_ent = Zyk_NPC_SpawnType("sage_of_eternity",11234,8416,1700,90);
									ent->client->pers.universe_quest_messages = 1;
									if (npc_ent)
										npc_ent->client->pers.universe_quest_messages = -2000;
								}

								if (ent->client->pers.hunter_quest_messages < 10)
								{
									ent->client->pers.hunter_quest_messages++;
									ent->client->pers.hunter_quest_timer = level.time + 1200;
								}
							}
						}
					}
					else if (level.quest_map == 18)
					{ 
						zyk_try_get_dark_quest_note(ent, 11);

						// zyk: Universe Quest artifact
						if (ent->client->pers.universe_quest_progress == 2 && ent->client->pers.can_play_quest == 1 && ent->client->pers.universe_quest_objective_control == 3 && !(ent->client->pers.universe_quest_counter & (1 << 4)) && ent->client->pers.universe_quest_timer < level.time)
						{
							gentity_t *npc_ent = NULL;
							if (ent->client->pers.universe_quest_messages == 0)
							{
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: I sense the presence of an artifact here.\"", ent->client->pers.netname));
								npc_ent = Zyk_NPC_SpawnType("quest_reborn",-3703,-3575,1121,90);
							}
							else if (ent->client->pers.universe_quest_messages == 1)
								npc_ent = Zyk_NPC_SpawnType("quest_reborn_blue",-1796,-802,1353,-90);
							else if (ent->client->pers.universe_quest_messages == 2)
								npc_ent = Zyk_NPC_SpawnType("quest_reborn_red",-730,-1618,729,179);
							else if (ent->client->pers.universe_quest_messages == 3)
								npc_ent = Zyk_NPC_SpawnType("quest_reborn_red",-735,-1380,729,179);
							else if (ent->client->pers.universe_quest_messages == 4)
								npc_ent = Zyk_NPC_SpawnType("quest_reborn_boss",-792,-1504,729,179);
							else if (ent->client->pers.universe_quest_messages == 5)
							{
								npc_ent = Zyk_NPC_SpawnType("quest_mage",-120,-1630,857,179);
								if (npc_ent)
								{
									npc_ent->client->ps.powerups[PW_FORCE_BOON] = level.time + 5500;

									npc_ent->client->pers.universe_quest_artifact_holder_id = ent-g_entities;
									ent->client->pers.universe_quest_artifact_holder_id = 4;
								}
							}

							if (ent->client->pers.universe_quest_messages < 6)
							{
								ent->client->pers.universe_quest_messages++;
								ent->client->pers.universe_quest_timer = level.time + 1200;
							}
						}
					}
					else if (level.quest_map == 20)
					{
						// zyk: Guardian of Agility
						if (ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS && !(ent->client->pers.defeated_guardians & (1 << 8)) && ent->client->pers.can_play_quest == 1 && ent->client->pers.guardian_mode == 0 && (int) ent->client->ps.origin[0] > 8374 && (int) ent->client->ps.origin[0] < 8574 && (int) ent->client->ps.origin[1] > -1422 && (int) ent->client->ps.origin[1] < -1222 && (int) ent->client->ps.origin[2] > -165 && (int) ent->client->ps.origin[2] < -160)
						{
							if (ent->client->pers.light_quest_timer < level.time)
							{
								if (ent->client->pers.light_quest_messages == 0)
									trap->SendServerCommand( ent->s.number, va("chat \"^6Guardian of Agility: ^7I am the Guardian of Agility, %s^7! Im too fast for you, snail!\"",ent->client->pers.netname));
								else if (ent->client->pers.light_quest_messages == 1)
								{
									spawn_boss(ent,9773,-1779,-162,90,"guardian_boss_5",9773,-1199,-162,90,5);
								}
								ent->client->pers.light_quest_messages++;
								ent->client->pers.light_quest_timer = level.time + 3000;
							}
						}

						// zyk: Universe Quest artifact
						if (ent->client->pers.universe_quest_progress == 2 && ent->client->pers.can_play_quest == 1 && ent->client->pers.universe_quest_objective_control == 3 && !(ent->client->pers.universe_quest_counter & (1 << 7)) && ent->client->pers.universe_quest_timer < level.time)
						{
							gentity_t *npc_ent = NULL;
							if (ent->client->pers.universe_quest_messages == 0)
							{
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: I sense the presence of an artifact here.\"", ent->client->pers.netname));
								npc_ent = Zyk_NPC_SpawnType("quest_mage",8480,-1084,-90,90);
								if (npc_ent)
								{
									npc_ent->client->ps.powerups[PW_FORCE_BOON] = level.time + 5500;

									npc_ent->client->pers.universe_quest_artifact_holder_id = ent-g_entities;
									ent->client->pers.universe_quest_artifact_holder_id = 7;
								}
							}

							if (ent->client->pers.universe_quest_messages < 1)
							{
								ent->client->pers.universe_quest_messages++;
								ent->client->pers.universe_quest_timer = level.time + 5000;
							}
						}
					}
					else if (level.quest_map == 24)
					{ // zyk: amulets objective of Universe Quest in mp/siege_desert
						if (ent->client->pers.universe_quest_progress == 5 && ent->client->pers.can_play_quest == 1 && ent->client->pers.universe_quest_objective_control != -1 && ent->client->pers.universe_quest_timer < level.time)
						{
							gentity_t *npc_ent = NULL;
							if (ent->client->pers.universe_quest_messages == 0)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",9102,2508,-358,-179);
							else if (ent->client->pers.universe_quest_messages == 1)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",9290,2236,-486,-84);
							else if (ent->client->pers.universe_quest_messages == 2)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",10520,1236,-486,-174);
							else if (ent->client->pers.universe_quest_messages == 3)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",11673,751,-486,175);
							else if (ent->client->pers.universe_quest_messages == 4)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",12570,-860,-486,177);
							else if (ent->client->pers.universe_quest_messages == 5)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",11540,-1677,-486,179);
							else if (ent->client->pers.universe_quest_messages == 6)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",11277,-2915,-486,179);
							else if (ent->client->pers.universe_quest_messages == 7)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",10386,-3408,-486,2);
							else if (ent->client->pers.universe_quest_messages == 8)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",9906,-2373,-487,2);
							else if (ent->client->pers.universe_quest_messages == 9)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",9097,-919,-486,-176);
							else if (ent->client->pers.universe_quest_messages == 10)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",6732,-1208,-486,-174);
							else if (ent->client->pers.universe_quest_messages == 11)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",6802,-654,-486,-60);
							else if (ent->client->pers.universe_quest_messages == 12)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",5734,-2395,-486,92);
							else if (ent->client->pers.universe_quest_messages == 13)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",4594,-1727,-486,173);
							else if (ent->client->pers.universe_quest_messages == 14)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",2505,-1616,-486,170);
							else if (ent->client->pers.universe_quest_messages == 15)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",3298,-564,-486,-86);
							else if (ent->client->pers.universe_quest_messages == 16)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",3532,231,-486,-8);
							else if (ent->client->pers.universe_quest_messages == 17)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",1832,-1103,-486,6);
							else if (ent->client->pers.universe_quest_messages == 18)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",1727,-480,-486,7);
							else if (ent->client->pers.universe_quest_messages == 19)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",2653,1014,-486,0);
							else if (ent->client->pers.universe_quest_messages == 20)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",4346,-1209,-486,-177);
							else if (ent->client->pers.universe_quest_messages == 21)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",2372,-2413,-486,90);
							else if (ent->client->pers.universe_quest_messages == 22)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",-5549,-841,57,178);
							else if (ent->client->pers.universe_quest_messages == 23)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",-6035,-2285,-486,-179);
							else if (ent->client->pers.universe_quest_messages == 24)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",-7149,-2482,-486,176);
							else if (ent->client->pers.universe_quest_messages == 25)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",-7304,-1155,-486,-177);
							else if (ent->client->pers.universe_quest_messages == 26)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",-8071,-381,-486,-1);
							else if (ent->client->pers.universe_quest_messages == 27)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",-9596,-1116,-486,1);
							else if (ent->client->pers.universe_quest_messages == 28)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",-9762,-191,-486,5);
							else if (ent->client->pers.universe_quest_messages == 29)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",-11311,-638,9,-1);
							else if (ent->client->pers.universe_quest_messages == 30)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",-11437,-662,-486,179);
							else if (ent->client->pers.universe_quest_messages == 31)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",-9344,837,-66,90);
							else if (ent->client->pers.universe_quest_messages == 32)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",-7710,-1665,-358,178);
							else if (ent->client->pers.universe_quest_messages == 33)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",-8724,-1275,-486,176);
							else if (ent->client->pers.universe_quest_messages == 34)
								npc_ent = Zyk_NPC_SpawnType("quest_protocol_imp",-12810,325,-422,-90);
							else if (ent->client->pers.universe_quest_messages == 35)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",10350,-357,-486,179);
							else if (ent->client->pers.universe_quest_messages == 36)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",5935,-1304,-486,125);
							else if (ent->client->pers.universe_quest_messages == 37)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",4516,-679,-486,-144);
							else if (ent->client->pers.universe_quest_messages == 38)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",-6327,-1071,-486,-179);
							else if (ent->client->pers.universe_quest_messages == 39)
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",-8120,781,-486,-96);
							else if (ent->client->pers.universe_quest_messages == 60)
							{
								npc_ent = Zyk_NPC_SpawnType("quest_sand_raider_green",12173,-304,-486,-179);
								trap->SendServerCommand( ent->s.number, "chat \"^2Green Sand Raider^7: We are the sand raiders tribe! Surrender or die!\"");
							}
							else if (ent->client->pers.universe_quest_messages == 61)
							{
								npc_ent = Zyk_NPC_SpawnType("quest_sand_raider_brown",12173,-225,-486,-179);
								trap->SendServerCommand( ent->s.number, "chat \"^3Brown Sand Raider^7: We will get the treasures of the city, and nothing can stop us!\"");
							}
							else if (ent->client->pers.universe_quest_messages == 62)
							{
								npc_ent = Zyk_NPC_SpawnType("quest_sand_raider_blue",12173,-137,-486,-179);
								trap->SendServerCommand( ent->s.number, "chat \"^4Blue Sand Raider^7: Anyone who resists will be dead so don't stand on our way!\"");
							}
							else if (ent->client->pers.universe_quest_messages == 63)
							{
								npc_ent = Zyk_NPC_SpawnType("quest_sand_raider_red",12173,-41,-486,-179);
								trap->SendServerCommand( ent->s.number, "chat \"^1Red Sand Raider^7: Ok, brothers! Let's strike!\"");
							}
							else if (ent->client->pers.universe_quest_messages == 199)
							{
								npc_ent = Zyk_NPC_SpawnType("quest_jawa",-7710,-1665,-358,178);
								if (npc_ent)
									npc_ent->client->pers.universe_quest_objective_control = -330;

								npc_ent = NULL;
							}
							else if (ent->client->pers.universe_quest_messages == 200)
							{
								npc_ent = Zyk_NPC_SpawnType("sage_of_light",-7867,-1484,-358,-90);
								trap->SendServerCommand( ent->s.number, "chat \"^3Sage of Eternity: ^7Hero. Come quickly to the Mayor's house. We need to talk to you.\"");
							}
							else if (ent->client->pers.universe_quest_messages == 201)
							{
								npc_ent = Zyk_NPC_SpawnType("sage_of_darkness",-7867,-1759,-358,90);
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: The sages! They are here! Something really serious must have happened in taspir1.\"", ent->client->pers.netname));
							}
							else if (ent->client->pers.universe_quest_messages == 202)
							{
								npc_ent = Zyk_NPC_SpawnType("sage_of_eternity",-7746,-1782,-358,90);
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: I will go to the mayor's house and find out.\"", ent->client->pers.netname));
							}
							else if (ent->client->pers.universe_quest_messages == 203)
							{
								npc_ent = Zyk_NPC_SpawnType("sage_of_universe",-7775,-1492,-358,-90);
							}
							else if (ent->client->pers.universe_quest_messages == 205)
								trap->SendServerCommand( ent->s.number, va("chat \"^5Sage of Light: ^7Now, %s^7, we will reveal you everything.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 206)
								trap->SendServerCommand( ent->s.number, va("chat \"^1Sage of Darkness: ^7%s^7, since the beginning we knew you were the chosen one.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 207)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Sage of Eternity: ^7The guardians told us about the arrival of a hero here that would be able to save the Guardian of Universe.\""));
							else if (ent->client->pers.universe_quest_messages == 208)
								trap->SendServerCommand( ent->s.number, va("chat \"^2Sage of Universe: ^7It is your destiny.\""));
							else if (ent->client->pers.universe_quest_messages == 209)
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: It is more complicated than I thought.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 210)
								trap->SendServerCommand( ent->s.number, va("chat \"^1Sage of Darkness: ^7Yes %s^7, but it is the truth.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 211)
								trap->SendServerCommand( ent->s.number, va("chat \"^5Sage of Light: ^7If the hero fails to defeat the Master of Evil, the entire Universe is doomed!\""));
							else if (ent->client->pers.universe_quest_messages == 212)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Sage of Eternity: ^7But if the hero succeeds, then the Universe will be in balance once again.\""));
							else if (ent->client->pers.universe_quest_messages == 213)
								trap->SendServerCommand( ent->s.number, va("chat \"^2Sage of Universe: ^7%s^7, You must defeat the Master of Evil, or everything will be taken over by him!\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 214)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Samir: ^7What a complicated story! But it looks like you don't have much choice!\""));
							else if (ent->client->pers.universe_quest_messages == 215)
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: I understand. But, sages, why did you all came back from taspir1? What happened there?\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 216)
								trap->SendServerCommand( ent->s.number, va("chat \"^3Sage of Eternity: ^7The Master of Evil felt our presence there and sent his soldiers to find us.\""));
							else if (ent->client->pers.universe_quest_messages == 217)
								trap->SendServerCommand( ent->s.number, va("chat \"^5Sage of Light: ^7We had to flee from there, but now he is aware of our quest to stop him.\""));
							else if (ent->client->pers.universe_quest_messages == 218)
								trap->SendServerCommand( ent->s.number, va("chat \"^1Sage of Darkness: ^7Then, we had the idea of searching for you here.\""));
							else if (ent->client->pers.universe_quest_messages == 219)
								trap->SendServerCommand( ent->s.number, va("chat \"^2Sage of Universe: ^7%s^7, you collected all artifacts and amulets. Well done.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 220)
								trap->SendServerCommand( ent->s.number, va("chat \"^2Sage of Universe: ^7Now you must go to ^3taspir1 ^7and defeat the Master of Evil.\""));
							else if (ent->client->pers.universe_quest_messages == 221)
								trap->SendServerCommand( ent->s.number, va("chat \"^2Sage of Universe: ^7The power of the amulets will merge all artifacts into his life force and make him vulnarable again.\""));
							else if (ent->client->pers.universe_quest_messages == 222)
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: Thanks for your help. Now I will go there to put an end to this.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 223)
							{
								trap->SendServerCommand( ent->s.number, va("chat \"^2Sage of Universe: ^7You must be aware that his soldiers are there and you will need to fight them. Courage, hero!\""));

								ent->client->pers.universe_quest_progress = 6;
								if (ent->client->pers.universe_quest_counter & (1 << 29))
								{ // zyk: if player is in Challenge Mode, do not remove this bit value
									ent->client->pers.universe_quest_counter = 0;
									ent->client->pers.universe_quest_counter |= (1 << 29);
								}
								else
									ent->client->pers.universe_quest_counter = 0;
								ent->client->pers.universe_quest_objective_control = -1;

								save_account(ent);

								quest_get_new_player(ent);
							}

							if (ent->client->pers.universe_quest_messages < 40 && npc_ent)
							{ // zyk: tests npc_ent so if for some reason the npc dont get spawned, the server tries to spawn it again
								ent->client->pers.universe_quest_messages++;
								ent->client->pers.universe_quest_timer = level.time + 200;

								// zyk: sets the universe_quest_objective_control based in universe_quest_messages value so each npc can say a different message. If its 35 (protocol npc), sets the player id
								if (ent->client->pers.universe_quest_messages == 35)
									npc_ent->client->pers.universe_quest_objective_control = ent-g_entities;
								else
									npc_ent->client->pers.universe_quest_objective_control = ent->client->pers.universe_quest_messages * (-10);
							}
							else if (ent->client->pers.universe_quest_messages >= 60 && ent->client->pers.universe_quest_messages <= 63 && npc_ent)
							{ // zyk: invoking the sand raiders
								npc_ent->client->pers.universe_quest_objective_control = ent-g_entities;
								ent->client->pers.universe_quest_messages++;
								ent->client->pers.universe_quest_timer = level.time + 2000;
							}
							else if (ent->client->pers.universe_quest_messages >= 199 && ent->client->pers.universe_quest_messages <= 203)
							{ // zyk: invoking the sages
								ent->client->pers.universe_quest_messages++;
								ent->client->pers.universe_quest_timer = level.time + 3000;

								if (npc_ent)
									npc_ent->client->pers.universe_quest_objective_control = -205; // zyk: flag to set this npc as a sage in this map
							}
							else if (ent->client->pers.universe_quest_messages >= 205 && ent->client->pers.universe_quest_messages <= 223)
							{ // zyk: talking to the sages
								ent->client->pers.universe_quest_messages++;
								ent->client->pers.universe_quest_timer = level.time + 5000;
							}
							else
							{
								ent->client->pers.universe_quest_timer = level.time + 1000;
							}
						}
					}
					else if (level.quest_map == 25)
					{ // zyk: seventh objective of Universe Quest
						if (ent->client->pers.universe_quest_progress == 6 && ent->client->pers.can_play_quest == 1 && ent->client->pers.universe_quest_timer < level.time)
						{
							gentity_t *npc_ent = NULL;
							if (ent->client->pers.universe_quest_messages == 0)
							{
								trap->SendServerCommand( ent->s.number, va("chat \"%s^7: The time has come. I must defeat the Master of Evil to save the Guardian of Universe.\"", ent->client->pers.netname));
								npc_ent = Zyk_NPC_SpawnType("quest_reborn_boss",1800,-2900,2785,90);
							}
							else if (ent->client->pers.universe_quest_messages == 2)
								trap->SendServerCommand( ent->s.number, va("chat \"^1Master of Evil: ^7Nice one, %s^7! You finally reached me!\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 3)
								trap->SendServerCommand( ent->s.number, va("chat \"%s: ^7Master of Evil? You won't escape from me!\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 4)
								trap->SendServerCommand( ent->s.number, va("chat \"^1Master of Evil: ^7You are wrong, %s^7. You won't escape from my power!\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 5)
								trap->SendServerCommand( ent->s.number, va("chat \"^1Master of Evil: ^7I know you have got all amulets and artifacts. I can feel myself vulnerable again...\""));
							else if (ent->client->pers.universe_quest_messages == 6)
								trap->SendServerCommand( ent->s.number, va("chat \"^1Master of Evil: ^7But I am still the most powerful being in existence!\""));
							else if (ent->client->pers.universe_quest_messages == 7)
								trap->SendServerCommand( ent->s.number, va("chat \"^1Master of Evil: ^7You can't stop me, %s^7! I will rule the Universe!\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 8)
								trap->SendServerCommand( ent->s.number, va("chat \"%s: ^7We will see about that!\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 9)
								trap->SendServerCommand( ent->s.number, va("chat \"^1Master of Evil: ^7Now, %s^7. Prepare yourself to die!\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 10)
							{
								spawn_boss(ent,2135,-2857,2620,-90,"master_of_evil",0,0,0,0,12);

								npc_ent = NULL;
							}
							else if (ent->client->pers.universe_quest_messages == 12)
							{ // zyk: defeated Master of Evil
								zyk_NPC_Kill_f("all"); // zyk: killing the guardian spawns
								trap->SendServerCommand( ent->s.number, va("chat \"^1Master of Evil: ^7It can't be! It's not possible! I... am... gone...\""));
							}
							else if (ent->client->pers.universe_quest_messages == 13)
							{ // zyk: defeated Master of Evil
								trap->SendServerCommand( ent->s.number, va("chat \"^2Guardian of Universe: ^7Greetings, hero. I am the Guardian of Universe.\""));
							}
							else if (ent->client->pers.universe_quest_messages == 14)
							{ // zyk: defeated Master of Evil
								trap->SendServerCommand( ent->s.number, va("chat \"^2Guardian of Universe: ^7You saved me, %s^7.\"", ent->client->pers.netname));
							}
							else if (ent->client->pers.universe_quest_messages == 15)
							{ // zyk: defeated Master of Evil
								trap->SendServerCommand( ent->s.number, va("chat \"^2Guardian of Universe: ^7Thanks to your efforts, the Universe will be in balance once again.\""));
							}
							else if (ent->client->pers.universe_quest_messages == 16)
							{ // zyk: defeated Master of Evil
								trap->SendServerCommand( ent->s.number, va("chat \"^2Guardian of Universe: ^7I will meet the other guardians in mp/siege_korriban. Farewell, %s^7.\"", ent->client->pers.netname));
							
								ent->client->pers.universe_quest_progress = 7;
								if (ent->client->pers.universe_quest_counter & (1 << 29))
								{ // zyk: if player is in Challenge Mode, do not remove this bit value
									ent->client->pers.universe_quest_counter = 0;
									ent->client->pers.universe_quest_counter |= (1 << 29);
								}
								else
									ent->client->pers.universe_quest_counter = 0;

								save_account(ent);

								quest_get_new_player(ent);
							}

							if (ent->client->pers.universe_quest_messages < 1)
							{ // zyk: spawning the reborn npcs
								ent->client->pers.universe_quest_messages++;
								ent->client->pers.universe_quest_timer = level.time + 1500;
							}
							else if (ent->client->pers.universe_quest_messages > 1 && ent->client->pers.universe_quest_messages < 11)
							{ // zyk: talking to the Master of Evil
								ent->client->pers.universe_quest_messages++;
								ent->client->pers.universe_quest_timer = level.time + 5000;
							}
							else if (ent->client->pers.universe_quest_messages == 11)
							{ // zyk: fighting the Master of Evil
								ent->client->pers.universe_quest_timer = level.time + 2000;
							}
							else if (ent->client->pers.universe_quest_messages > 11)
							{ // zyk: defeated the Master of Evil
								ent->client->pers.universe_quest_messages++;
								ent->client->pers.universe_quest_timer = level.time + 5000;
							}

							if (npc_ent)
							{ // zyk: setting the player who invoked this npc
								npc_ent->client->pers.universe_quest_objective_control = ent-g_entities;
							}
						}
					}
				}
			}

			if (level.gametype == GT_SIEGE &&
				ent->client->siegeClass != -1 &&
				(bgSiegeClasses[ent->client->siegeClass].classflags & (1<<CFL_STATVIEWER)))
			{ //see if it's time to send this guy an update of extended info
				if (ent->client->siegeEDataSend < level.time)
				{
                    G_SiegeClientExData(ent);
					ent->client->siegeEDataSend = level.time + 1000; //once every sec seems ok
				}
			}

			if((!level.intermissiontime)&&!(ent->client->ps.pm_flags&PMF_FOLLOW) && ent->client->sess.sessionTeam != TEAM_SPECTATOR)
			{
				WP_ForcePowersUpdate(ent, &ent->client->pers.cmd );
				WP_SaberPositionUpdate(ent, &ent->client->pers.cmd);
				WP_SaberStartMissileBlockCheck(ent, &ent->client->pers.cmd);
			}

			if (g_allowNPC.integer)
			{
				//This was originally intended to only be done for client 0.
				//Make sure it doesn't slow things down too much with lots of clients in game.
				NAV_FindPlayerWaypoint(i);
			}

			trap->ICARUS_MaintainTaskManager(ent->s.number);

			G_RunClient( ent );
			continue;
		}
		else if (ent->s.eType == ET_NPC)
		{
			int j;
			// turn off any expired powerups
			for ( j = 0 ; j < MAX_POWERUPS ; j++ ) {
				if ( ent->client->ps.powerups[ j ] < level.time ) {
					ent->client->ps.powerups[ j ] = 0;
				}
			}

			WP_ForcePowersUpdate(ent, &ent->client->pers.cmd );
			WP_SaberPositionUpdate(ent, &ent->client->pers.cmd);
			WP_SaberStartMissileBlockCheck(ent, &ent->client->pers.cmd);

			quest_power_events(ent);
			poison_dart_hits(ent);

			if (ent->client->pers.universe_quest_artifact_holder_id != -1 && ent->health > 0 && ent->client->ps.powerups[PW_FORCE_BOON] < (level.time + 1000))
			{ // zyk: artifact holder npcs. Keep their artifact (force boon) active
				ent->client->ps.powerups[PW_FORCE_BOON] = level.time + 1000;
			}

			// zyk: quest guardians special abilities
			if (ent->client->pers.guardian_invoked_by_id != -1 && ent->health > 0)
			{
				if (ent->client->pers.guardian_mode == 1)
				{ // zyk: Guardian of Water
					if (ent->client->pers.guardian_timer < level.time)
					{
						gentity_t *player_ent = &g_entities[ent->client->pers.guardian_invoked_by_id];
						int distance = (int)Distance(ent->client->ps.origin,player_ent->client->ps.origin);

						if (distance > 400)
						{
							healing_water(ent,200);
							trap->SendServerCommand( -1, "chat \"^4Guardian of Water: ^7Healing Water!\"");
						}
						else
						{
							water_splash(ent,400,15);
							trap->SendServerCommand( -1, "chat \"^4Guardian of Water: ^7Water Splash!\"");
						}

						ent->client->pers.guardian_timer = level.time + 12000;
					}

					if (ent->client->pers.light_quest_timer < level.time)
					{
						acid_water(ent, 600, 55);
						trap->SendServerCommand(-1, "chat \"^4Guardian of Water: ^7Acid Water!\"");
						ent->client->pers.light_quest_timer = level.time + 10000;
					}
				}
				else if (ent->client->pers.guardian_mode == 2)
				{ // zyk: Guardian of Earth
					if (ent->client->pers.guardian_timer < level.time)
					{ // zyk: uses earthquake ability
						earthquake(ent,2000,350,3000);
						ent->client->pers.guardian_timer = level.time + 3000 + ent->health;
						trap->SendServerCommand( -1, "chat \"^3Guardian of Earth: ^7Earthquake!\"");
					}

					if (ent->client->pers.light_quest_timer < level.time)
					{
						rock_fall(ent,2000,55);
						ent->client->pers.light_quest_timer = level.time + 10000;
						trap->SendServerCommand( -1, "chat \"^3Guardian of Earth: ^7Rockfall!\"");
					}

					if (ent->client->pers.universe_quest_timer < level.time)
					{
						shifting_sand(ent, 4000);
						ent->client->pers.universe_quest_timer = level.time + 11000;
						trap->SendServerCommand(-1, "chat \"^3Guardian of Earth: ^7Shifting Sand!\"");
					}
				}
				else if (ent->client->pers.guardian_mode == 3)
				{ // zyk: Guardian of Forest
					if (ent->client->pers.guardian_timer < level.time)
					{ // zyk: uses sleeping flowers or poison mushrooms
						if (Q_irand(0,3) != 0)
						{
							sleeping_flowers(ent,3000,1500);
							trap->SendServerCommand( -1, "chat \"^2Guardian of Forest: ^7Sleeping Flowers!\"");
							ent->client->pers.guardian_timer = level.time + 12000;
						}
						else
						{
							poison_mushrooms(ent,100,3000);
							trap->SendServerCommand( -1, va("chat \"^2Guardian of Forest: ^7Poison Mushrooms!\""));
							ent->client->pers.guardian_timer = level.time + 11000;
						}
					}

					if (ent->client->pers.light_quest_timer < level.time)
					{
						tree_of_life(ent);
						ent->client->pers.light_quest_timer = level.time + 15000;
						trap->SendServerCommand(-1, va("chat \"^2Guardian of Forest: ^7Tree of Life!\""));
					}
				}
				else if (ent->client->pers.guardian_mode == 4)
				{ // zyk: Guardian of Intelligence
					if (ent->client->pers.guardian_timer < level.time)
					{
						if (ent->client->pers.light_quest_messages == 0)
						{
							dome_of_damage(ent,1700,40);
							ent->client->pers.light_quest_messages = 1;
							trap->SendServerCommand( -1, "chat \"^5Guardian of Intelligence: ^7Dome of Damage!\"");
						}
						else
						{
							magic_shield(ent, 6000);
							ent->client->pers.light_quest_messages = 0;
							trap->SendServerCommand( -1, "chat \"^5Guardian of Intelligence: ^7Magic Shield!\"");
						}

						ent->client->pers.guardian_timer = level.time + ent->health + 8000;
					}

					if (ent->client->pers.light_quest_timer < level.time)
					{
						magic_drain(ent, 800);
						ent->client->pers.light_quest_timer = level.time + 14000;
						trap->SendServerCommand(-1, "chat \"^5Guardian of Intelligence: ^7Magic Drain!\"");
					}
				}
				else if (ent->client->pers.guardian_mode == 5)
				{ // zyk: Guardian of Agility
					// zyk: adding jetpack to this boss
					ent->client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << HI_JETPACK);

					if (ent->client->pers.light_quest_timer < level.time)
					{ // zyk: after losing half HP, uses his special ability
						ultra_speed(ent,10000);
						trap->SendServerCommand( -1, "chat \"^6Guardian of Agility: ^7Ultra Speed!\"");
						ent->client->pers.light_quest_timer = level.time + 12000;
					}

					if (ent->client->pers.guardian_timer < level.time)
					{
						slow_motion(ent,600,12000);
						trap->SendServerCommand( -1, "chat \"^6Guardian of Agility: ^7Slow Motion!\"");
						ent->client->pers.guardian_timer = level.time + 14000;
					}

					if (ent->client->pers.universe_quest_timer < level.time)
					{
						fast_and_slow(ent, 600, 6000);
						ent->client->pers.universe_quest_timer = level.time + 20000;
						trap->SendServerCommand(-1, "chat \"^6Guardian of Agility: ^7Fast and Slow!\"");
					}
				}
				else if (ent->client->pers.guardian_mode == 6)
				{ // zyk: Guardian of Fire
					// zyk: take him back if he falls
					if (ent->client->ps.origin[2] < -600)
					{
						vec3_t origin;
						vec3_t yaw;

						origin[0] = 0.0f;
						origin[1] = -269.0f;
						origin[2] = -374.0f;
						yaw[0] = 0.0f;
						yaw[1] = 90.0f;
						yaw[2] = 0.0f;
						zyk_TeleportPlayer( ent, origin, yaw );
					}

					if (ent->client->pers.guardian_timer < level.time)
					{ // zyk: fire ability
						flame_burst(ent, 5000);
						trap->SendServerCommand( -1, "chat \"^1Guardian of Fire: ^7Flame Burst!\"");
						ent->client->pers.guardian_timer = level.time + 18000;
					}

					if (ent->client->pers.light_quest_timer < level.time)
					{
						ultra_flame(ent,4000,55);
						trap->SendServerCommand( -1, "chat \"^1Guardian of Fire: ^7Ultra Flame!\"");
						ent->client->pers.light_quest_timer = level.time + 14000;
					}

					if (ent->client->pers.universe_quest_timer < level.time)
					{
						flaming_area(ent, 30);
						ent->client->pers.universe_quest_timer = level.time + 16000;
						trap->SendServerCommand(-1, "chat \"^1Guardian of Fire: ^7Flaming Area!\"");
					}
				}
				else if (ent->client->pers.guardian_mode == 7)
				{ // zyk: Guardian of Wind
					if (ent->client->pers.guardian_timer < level.time)
					{
						if (!Q_irand(0,1))
						{ // zyk: randomly choose between Blowing Wind and Reverse Wind
							blowing_wind(ent, 2500, 5000);
							trap->SendServerCommand(-1, "chat \"^7Guardian of Wind: ^7Blowing Wind!\"");
						}
						else
						{
							reverse_wind(ent, 2500, 5000);
							trap->SendServerCommand(-1, "chat \"^7Guardian of Wind: ^7Reverse Wind!\"");
						}

						ent->client->pers.guardian_timer = level.time + 12000;
					}

					if (ent->client->pers.light_quest_timer < level.time)
					{
						hurricane(ent,700,5000);
						trap->SendServerCommand( -1, "chat \"^7Guardian of Wind: ^7Hurricane!\"");
						ent->client->pers.light_quest_timer = level.time + 12000;
					}
				}
				else if (ent->client->pers.guardian_mode == 16)
				{ // zyk: Guardian of Ice
					if (ent->client->pers.guardian_timer < level.time)
					{
						ice_stalagmite(ent,500,160);
						ent->client->pers.guardian_timer = level.time + (ent->client->ps.stats[STAT_MAX_HEALTH] * 2);
						trap->SendServerCommand( -1, "chat \"^5Guardian of Ice: ^7Ice Stalagmite!\"");
					}

					if (ent->client->pers.light_quest_timer < level.time)
					{
						ice_boulder(ent,400,80);
						trap->SendServerCommand( -1, "chat \"^5Guardian of Ice: ^7Ice Boulder!\"");
						ent->client->pers.light_quest_timer = level.time + (ent->client->ps.stats[STAT_MAX_HEALTH] * 2);
					}

					if (ent->client->pers.universe_quest_timer < level.time)
					{
						ice_block(ent, 3500);
						ent->client->pers.universe_quest_timer = level.time + (ent->client->ps.stats[STAT_MAX_HEALTH] * 2);
						trap->SendServerCommand(-1, "chat \"^5Guardian of Ice: ^7Ice Block!\"");
					}
				}
				else if (ent->client->pers.guardian_mode == 8)
				{ // zyk: Guardian of Light
					if (ent->client->pers.hunter_quest_messages == 0 && ent->health < (ent->client->ps.stats[STAT_MAX_HEALTH]))
					{ // zyk: after losing half HP, uses his special ability
						ent->client->pers.hunter_quest_messages = 1;
						ent->client->pers.quest_power_status |= (1 << 14);
						trap->SendServerCommand( -1, "chat \"^5Guardian of Light: ^7Light Power!\"");
					}

					if (ent->client->pers.guardian_timer < level.time)
					{
						lightning_dome(ent,90);
						trap->SendServerCommand( -1, "chat \"^5Guardian of Light: ^7Lightning Dome!\"");
						ent->client->pers.guardian_timer = level.time + 14000;
					}
				}
				else if (ent->client->pers.guardian_mode == 9)
				{ // zyk: Guardian of Darkness
					if (ent->client->pers.hunter_quest_messages == 0 && ent->health < (ent->client->ps.stats[STAT_MAX_HEALTH]))
					{ // zyk: after losing half HP, uses his special ability
						ent->client->pers.hunter_quest_messages = 1;
						ent->client->pers.quest_power_status |= (1 << 15);
						trap->SendServerCommand( -1, "chat \"^1Guardian of Darkness: ^7Dark Power!\"");
					}

					if (ent->client->pers.guardian_timer < level.time)
					{
						inner_area_damage(ent,400,90);
						trap->SendServerCommand( -1, "chat \"^1Guardian of Darkness: ^7Inner Area Damage!\"");
						ent->client->pers.guardian_timer = level.time + 14000;
					}
				}
				else if (ent->client->pers.guardian_mode == 10)
				{ // zyk: Guardian of Eternity
					if (ent->client->pers.hunter_quest_messages == 0 && ent->health < (ent->client->ps.stats[STAT_MAX_HEALTH]))
					{ // zyk: after losing half HP, uses his special ability
						ent->client->pers.hunter_quest_messages = 1;
						ent->client->pers.quest_power_status |= (1 << 16);
						trap->SendServerCommand( -1, "chat \"^3Guardian of Eternity: ^7Eternity Power!\"");
					}

					if (ent->client->pers.guardian_timer < level.time)
					{
						healing_area(ent,2,5000);
						trap->SendServerCommand( -1, "chat \"^3Guardian of Eternity: ^7Healing Area!\"");
						ent->client->pers.guardian_timer = level.time + 14000;
					}
				}
				else if (ent->client->pers.guardian_mode == 11)
				{ // zyk: Guardian of Resistance
					if (ent->client->pers.guardian_timer < level.time)
					{
						ultra_resistance(ent, 10000);
						ent->client->pers.guardian_timer = level.time + 14000;
						trap->SendServerCommand( -1, "chat \"^3Guardian of Resistance: ^7Ultra Resistance!\"");
					}

					if (ent->client->pers.light_quest_timer < level.time)
					{
						ultra_strength(ent, 10000);
						ent->client->pers.light_quest_timer = level.time + 14000;
						trap->SendServerCommand( -1, "chat \"^3Guardian of Resistance: ^7Ultra Strength!\"");
					}

					if (ent->client->pers.universe_quest_timer < level.time)
					{
						enemy_nerf(ent, 1000);
						ent->client->pers.universe_quest_timer = level.time + 11000;
						trap->SendServerCommand(-1, "chat \"^3Guardian of Resistance: ^7Enemy Weakening!\"");
					}
				}
				else if (ent->client->pers.guardian_mode == 12)
				{ // zyk: Master of Evil
					// zyk: adding jetpack to this boss
					ent->client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << HI_JETPACK);

					// zyk: take him back if he falls
					if (ent->client->ps.origin[0] < 400 || ent->client->ps.origin[0] > 3600 || ent->client->ps.origin[1] < -4450 || ent->client->ps.origin[1] > -1250 || ent->client->ps.origin[2] < 2500 || ent->client->ps.origin[2] > 4000)
					{
						vec3_t origin;
						vec3_t yaw;

						origin[0] = 2135.0f;
						origin[1] = -2857.0f;
						origin[2] = 2800.0f;
						yaw[0] = 0.0f;
						yaw[1] = -90.0f;
						yaw[2] = 0.0f;
						zyk_TeleportPlayer( ent, origin, yaw );
					}

					if (ent->client->pers.guardian_timer < level.time)
					{
						if (ent->client->NPC_class == CLASS_REBORN)
							ent->client->NPC_class = CLASS_BOBAFETT;
						else
							ent->client->NPC_class = CLASS_REBORN;
						
						if (ent->health < (ent->client->ps.stats[STAT_MAX_HEALTH] / 5))
							Zyk_NPC_SpawnType("quest_mage", 2135, -2857, 2800, 90);
						else if (ent->health < ((ent->client->ps.stats[STAT_MAX_HEALTH]/5) * 2))
							Zyk_NPC_SpawnType("quest_reborn_boss",2135,-2857,2800,90);
						else if (ent->health < ((ent->client->ps.stats[STAT_MAX_HEALTH]/5) * 3))
							Zyk_NPC_SpawnType("quest_reborn_red",2135,-2857,2800,90);
						else if (ent->health < ((ent->client->ps.stats[STAT_MAX_HEALTH]/5) * 4))
							Zyk_NPC_SpawnType("quest_reborn_blue",2135,-2857,2800,90);
						else if (ent->health < ent->client->ps.stats[STAT_MAX_HEALTH])
							Zyk_NPC_SpawnType("quest_reborn",2135,-2857,2800,-90);

						if (!ent->client->ps.powerups[PW_CLOAKED])
							Jedi_Cloak(ent);

						ultra_drain(ent,450,55,8000);
						trap->SendServerCommand( -1, "chat \"^1Master of Evil: ^7Ultra Drain!\"");

						ent->client->pers.guardian_timer = level.time + 25000;
					}

					if (ent->client->pers.light_quest_timer < level.time)
					{
						ultra_flame(ent, 4000, 55);
						trap->SendServerCommand(-1, "chat \"^1Master of Evil: ^7Ultra Flame!\"");
						ent->client->pers.light_quest_timer = level.time + 29000;
					}
				}
				else if (ent->client->pers.guardian_mode == 13)
				{ // zyk: Guardian of Universe
					if (ent->client->pers.guardian_timer < level.time)
					{
						gentity_t *player_ent = &g_entities[ent->client->pers.guardian_invoked_by_id];
						gentity_t *npc_ent = NULL;
						vec3_t origin;
						vec3_t yaw;

						origin[0] = player_ent->client->ps.origin[0];
						origin[1] = player_ent->client->ps.origin[1];
						if (player_ent->client->ps.origin[2] > 200)
							origin[2] = player_ent->client->ps.origin[2] - 100;
						else
							origin[2] = player_ent->client->ps.origin[2] + 200;

						yaw[0] = 0.0f;
						yaw[1] = -179.0f;
						yaw[2] = 0.0f;

						zyk_TeleportPlayer( ent, origin, yaw );

						if (ent->health < ent->client->ps.stats[STAT_MAX_HEALTH] && ent->client->pers.hunter_quest_messages < 15)
						{ // zyk: she can spawn up to 15 clones
							ent->client->pers.hunter_quest_messages++;
							npc_ent = NPC_SpawnType(player_ent, "guardian_of_universe",NULL,qfalse);
						}

						if (npc_ent)
						{
							npc_ent->health = ent->client->ps.stats[STAT_MAX_HEALTH]/10;
							npc_ent->client->ps.stats[STAT_MAX_HEALTH] = npc_ent->health;
						}

						immunity_power(ent,20000);
						trap->SendServerCommand( -1, "chat \"^2Guardian of Universe: ^7Immunity Power!\"");

						ent->client->pers.guardian_timer = level.time + 35000;
					}

					if (ent->health < (ent->client->ps.stats[STAT_MAX_HEALTH]/2) && ent->client->pers.light_quest_messages == 0)
					{
						ent->client->pers.light_quest_messages = 1;
						ent->client->pers.quest_power_status |= (1 << 13);
						trap->SendServerCommand( -1, "chat \"^2Guardian of Universe: ^7Universe Power!\"");
					}

					if (ent->client->pers.light_quest_timer < level.time)
					{
						magic_explosion(ent,320,170,900);
						trap->SendServerCommand( -1, "chat \"^2Guardian of Universe: ^7Magic Explosion!\"");
						ent->client->pers.light_quest_timer = level.time + 15000;
					}

					if (ent->client->pers.universe_quest_timer < level.time)
					{
						magic_shield(ent, 6000);
						ent->client->pers.universe_quest_timer = level.time + 29000;
						trap->SendServerCommand(-1, "chat \"^2Guardian of Universe: ^7Magic Shield!\"");
					}
				}
				else if (ent->client->pers.guardian_mode == 14)
				{ // zyk: Guardian of Chaos
					if (ent->client->pers.guardian_timer < level.time)
					{
						if (ent->client->pers.hunter_quest_messages == 0)
						{
							if (!ent->client->ps.powerups[PW_CLOAKED])
								Jedi_Cloak(ent);

							magic_shield(ent,6000);
							trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7Magic Shield!\"");
							ent->client->pers.hunter_quest_messages++;
						}
						else if (ent->client->pers.hunter_quest_messages == 1)
						{
							blowing_wind(ent,3000,5000);
							trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7Blowing Wind!\"");
							ent->client->pers.hunter_quest_messages++;
						}
						else if (ent->client->pers.hunter_quest_messages == 2)
						{
							ice_block(ent, 3500);
							trap->SendServerCommand(-1, "chat \"^1Guardian of Chaos: ^7Ice Block!\"");
							ent->client->pers.hunter_quest_messages++;
						}
						else if (ent->client->pers.hunter_quest_messages == 3)
						{
							healing_water(ent,200);
							trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7Healing Water!\"");
							ent->client->pers.hunter_quest_messages++;
						}
						else if (ent->client->pers.hunter_quest_messages == 4)
						{
							immunity_power(ent,15000);
							trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7Immunity Power!\"");
							ent->client->pers.hunter_quest_messages++;
						}
						else if (ent->client->pers.hunter_quest_messages == 5)
						{
							sleeping_flowers(ent,3200,1000);
							trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7Sleeping Flowers!\"");
							ent->client->pers.hunter_quest_messages++;
						}
						else if (ent->client->pers.hunter_quest_messages == 6)
						{
							ultra_strength(ent,12000);
							trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7Ultra Strength!\"");
							ent->client->pers.hunter_quest_messages++;
						}
						else if (ent->client->pers.hunter_quest_messages == 7)
						{
							ultra_drain(ent,450,55,8000);
							trap->SendServerCommand( -1, va("chat \"^1Guardian of Chaos: ^7Ultra Drain!\""));
							ent->client->pers.hunter_quest_messages++;
						}
						else if (ent->client->pers.hunter_quest_messages == 8)
						{
							ice_stalagmite(ent,2000,180);
							trap->SendServerCommand( -1, va("chat \"^1Guardian of Chaos: ^7Ice Stalagmite!\""));
							ent->client->pers.hunter_quest_messages++;
						}
						else if (ent->client->pers.hunter_quest_messages == 9)
						{
							ice_boulder(ent,1000,90);
							trap->SendServerCommand( -1, va("chat \"^1Guardian of Chaos: ^7Ice Boulder!\""));
							ent->client->pers.hunter_quest_messages++;
						}
						else if (ent->client->pers.hunter_quest_messages == 10)
						{
							acid_water(ent, 1000, 55);
							trap->SendServerCommand(-1, va("chat \"^1Guardian of Chaos: ^7Acid Water!\""));
							ent->client->pers.hunter_quest_messages++;
						}
						else if (ent->client->pers.hunter_quest_messages == 11)
						{
							poison_mushrooms(ent,100,1800);
							trap->SendServerCommand( -1, va("chat \"^1Guardian of Chaos: ^7Poison Mushrooms!\""));
							ent->client->pers.hunter_quest_messages++;
						}
						else if (ent->client->pers.hunter_quest_messages == 12)
						{
							ultra_speed(ent,15000);
							trap->SendServerCommand( -1, va("chat \"^1Guardian of Chaos: ^7Ultra Speed!\""));
							ent->client->pers.hunter_quest_messages++;
						}
						else if (ent->client->pers.hunter_quest_messages == 13)
						{
							slow_motion(ent,1000,10000);
							trap->SendServerCommand( -1, va("chat \"^1Guardian of Chaos: ^7Slow Motion!\""));
							ent->client->pers.hunter_quest_messages++;
						}
						else if (ent->client->pers.hunter_quest_messages == 14)
						{
							water_splash(ent,1400,18);
							trap->SendServerCommand( -1, va("chat \"^1Guardian of Chaos: ^7Water Splash!\""));
							ent->client->pers.hunter_quest_messages++;
						}
						else if (ent->client->pers.hunter_quest_messages == 15)
						{
							rock_fall(ent,1600,55);
							trap->SendServerCommand( -1, va("chat \"^1Guardian of Chaos: ^7Rockfall!\""));
							ent->client->pers.hunter_quest_messages++;
						}
						else if (ent->client->pers.hunter_quest_messages == 16)
						{
							ultra_flame(ent,2200,50);
							trap->SendServerCommand( -1, va("chat \"^1Guardian of Chaos: ^7Ultra Flame!\""));
							ent->client->pers.hunter_quest_messages++;
						}
						else if (ent->client->pers.hunter_quest_messages == 17)
						{
							magic_drain(ent, 2200);
							trap->SendServerCommand(-1, va("chat \"^1Guardian of Chaos: ^7Magic Drain!\""));
							ent->client->pers.hunter_quest_messages++;
						}
						else if (ent->client->pers.hunter_quest_messages == 18)
						{
							dome_of_damage(ent,2000,40);
							trap->SendServerCommand( -1, va("chat \"^1Guardian of Chaos: ^7Dome of Damage!\""));
							ent->client->pers.hunter_quest_messages++;
						}
						else if (ent->client->pers.hunter_quest_messages == 19)
						{
							shifting_sand(ent, 2500);
							trap->SendServerCommand(-1, va("chat \"^1Guardian of Chaos: ^7Shifting Sand!\""));
							ent->client->pers.hunter_quest_messages++;
						}
						else if (ent->client->pers.hunter_quest_messages == 20)
						{
							flame_burst(ent, 5000);
							trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7Flame Burst!\"");
							ent->client->pers.hunter_quest_messages++;
						}
						else if (ent->client->pers.hunter_quest_messages == 21)
						{
							fast_and_slow(ent, 1000, 6000);
							trap->SendServerCommand(-1, "chat \"^1Guardian of Chaos: ^7Fast and Slow!\"");
							ent->client->pers.hunter_quest_messages++;
						}
						else if (ent->client->pers.hunter_quest_messages == 22)
						{
							flaming_area(ent, 35);
							trap->SendServerCommand(-1, "chat \"^1Guardian of Chaos: ^7Flaming Area!\"");
							ent->client->pers.hunter_quest_messages++;
						}
						else if (ent->client->pers.hunter_quest_messages == 23)
						{
							hurricane(ent,1200,5000);
							trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7Hurricane!\"");
							ent->client->pers.hunter_quest_messages++;
						}
						else if (ent->client->pers.hunter_quest_messages == 24)
						{
							enemy_nerf(ent, 2000);
							trap->SendServerCommand(-1, "chat \"^1Guardian of Chaos: ^7Enemy Weakening!\"");
							ent->client->pers.hunter_quest_messages++;
						}
						else if (ent->client->pers.hunter_quest_messages == 25)
						{
							lightning_dome(ent,120);
							trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7Lightning Dome!\"");
							ent->client->pers.hunter_quest_messages++;
						}
						else if (ent->client->pers.hunter_quest_messages == 26)
						{
							earthquake(ent,2000,500,3000);
							trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7Earthquake!\"");
							ent->client->pers.hunter_quest_messages++;
						}
						else if (ent->client->pers.hunter_quest_messages == 27)
						{
							time_power(ent,1600,5000);
							trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7Time Power!\"");
							ent->client->pers.hunter_quest_messages++;
						}
						else if (ent->client->pers.hunter_quest_messages == 28)
						{
							healing_area(ent,3,5000);
							trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7Healing Area!\"");
							ent->client->pers.hunter_quest_messages++;
						}
						else if (ent->client->pers.hunter_quest_messages == 29)
						{
							tree_of_life(ent);
							trap->SendServerCommand(-1, "chat \"^1Guardian of Chaos: ^7Tree of Life!\"");
							ent->client->pers.hunter_quest_messages++;
						}
						else if (ent->client->pers.hunter_quest_messages == 30)
						{
							vec3_t origin, angles;

							VectorSet(origin,-2836,-26946,500);
							VectorSet(angles,0,0,0);

							zyk_TeleportPlayer(ent,origin,angles);

							ultra_resistance(ent,12000);
							trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7Ultra Resistance!\"");
							ent->client->pers.hunter_quest_messages++;
						}
						else if (ent->client->pers.hunter_quest_messages == 31)
						{
							reverse_wind(ent, 2500, 5000);
							trap->SendServerCommand(-1, va("chat \"^1Guardian of Chaos: ^7Reverse Wind!\""));
							ent->client->pers.hunter_quest_messages++;
						}
						else if (ent->client->pers.hunter_quest_messages == 32)
						{
							magic_explosion(ent,320,190,900);
							trap->SendServerCommand( -1, va("chat \"^1Guardian of Chaos: ^7Magic Explosion!\""));
							ent->client->pers.hunter_quest_messages++;
						}
						else if (ent->client->pers.hunter_quest_messages == 33)
						{
							chaos_power(ent,1600,300);
							trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7Chaos Power!\"");
							ent->client->pers.hunter_quest_messages = 0;
						}

						ent->client->pers.guardian_timer = level.time + (ent->health/2) + 3000;
					}

					if (ent->client->pers.light_quest_timer < level.time)
					{
						if (ent->client->pers.light_quest_messages == 0)
						{
							ent->client->pers.light_quest_messages = 1;
						}
						else if (ent->client->pers.light_quest_messages == 1)
						{
							ent->client->pers.light_quest_messages = 2;
							ent->client->pers.quest_power_status |= (1 << 14);
							trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7Light Power!\"");
						}
						else if (ent->client->pers.light_quest_messages == 2)
						{
							ent->client->pers.light_quest_messages = 3;
							ent->client->pers.quest_power_status |= (1 << 15);
							trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7Dark Power!\"");
						}
						else if (ent->client->pers.light_quest_messages == 3)
						{
							ent->client->pers.light_quest_messages = 4;
							ent->client->pers.quest_power_status |= (1 << 16);
							trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7Eternity Power!\"");
						}
						else if (ent->client->pers.light_quest_messages == 4)
						{
							ent->client->pers.light_quest_messages = 5;
							ent->client->pers.quest_power_status |= (1 << 13);
							trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7Universe Power!\"");
						}

						ent->client->pers.light_quest_timer = level.time + 25000;
					}
				}
			}
			else if (ent->health > 0 && Q_stricmp(ent->NPC_type, "quest_mage") == 0 && ent->client->pers.guardian_timer < level.time)
			{ // zyk: powers used by the quest_mage npc
				int random_magic = Q_irand(0, 26);

				if (random_magic == 0)
				{
					ultra_strength(ent, 30000);
				}
				else if (random_magic == 1)
				{
					poison_mushrooms(ent, 100, 600);
				}
				else if (random_magic == 2)
				{
					water_splash(ent, 400, 15);
				}
				else if (random_magic == 3)
				{
					ultra_flame(ent, 500, 50);
				}
				else if (random_magic == 4)
				{
					rock_fall(ent, 500, 55);
				}
				else if (random_magic == 5)
				{
					dome_of_damage(ent, 500, 35);
				}
				else if (random_magic == 6)
				{
					hurricane(ent, 600, 5000);
				}
				else if (random_magic == 7)
				{
					slow_motion(ent, 400, 15000);
				}
				else if (random_magic == 8)
				{
					ultra_resistance(ent, 30000);
				}
				else if (random_magic == 9)
				{
					sleeping_flowers(ent, 2500, 350);
				}
				else if (random_magic == 10)
				{
					healing_water(ent, 120);
				}
				else if (random_magic == 11)
				{
					flame_burst(ent, 5000);
				}
				else if (random_magic == 12)
				{
					earthquake(ent, 2000, 300, 500);
				}
				else if (random_magic == 13)
				{
					magic_shield(ent, 6000);
				}
				else if (random_magic == 14)
				{
					blowing_wind(ent, 700, 5000);
				}
				else if (random_magic == 15)
				{
					ultra_speed(ent, 15000);
				}
				else if (random_magic == 16)
				{
					ice_stalagmite(ent, 500, 160);
				}
				else if (random_magic == 17)
				{
					ice_boulder(ent, 380, 70);
				}
				else if (random_magic == 18)
				{
					acid_water(ent, 500, 55);
				}
				else if (random_magic == 19)
				{
					shifting_sand(ent, 800);
				}
				else if (random_magic == 20)
				{
					tree_of_life(ent);
				}
				else if (random_magic == 21)
				{
					magic_drain(ent, 400);
				}
				else if (random_magic == 22)
				{
					fast_and_slow(ent, 400, 6000);
				}
				else if (random_magic == 23)
				{
					flaming_area(ent, 30);
				}
				else if (random_magic == 24)
				{
					reverse_wind(ent, 700, 5000);
				}
				else if (random_magic == 25)
				{
					enemy_nerf(ent, 400);
				}
				else if (random_magic == 26)
				{
					ice_block(ent, 3500);
				}

				ent->client->pers.guardian_timer = level.time + Q_irand(3000, 6000);
			}
		}

		// zyk: added check for mind control on npcs here. NPCs being mind controlled cant think
		if (!(ent && ent->client && ent->client->pers.being_mind_controlled != -1))
			G_RunThink( ent );

		if (g_allowNPC.integer)
		{
			ClearNPCGlobals();
		}
	}
#ifdef _G_FRAME_PERFANAL
	iTimer_ItemRun = trap->PrecisionTimer_End(timer_ItemRun);
#endif

	SiegeCheckTimers();

#ifdef _G_FRAME_PERFANAL
	trap->PrecisionTimer_Start(&timer_ROFF);
#endif
	trap->ROFF_UpdateEntities();
#ifdef _G_FRAME_PERFANAL
	iTimer_ROFF = trap->PrecisionTimer_End(timer_ROFF);
#endif



#ifdef _G_FRAME_PERFANAL
	trap->PrecisionTimer_Start(&timer_ClientEndframe);
#endif
	// perform final fixups on the players
	ent = &g_entities[0];
	for (i=0 ; i < level.maxclients ; i++, ent++ ) {
		if ( ent->inuse ) {
			ClientEndFrame( ent );
		}
	}
#ifdef _G_FRAME_PERFANAL
	iTimer_ClientEndframe = trap->PrecisionTimer_End(timer_ClientEndframe);
#endif



#ifdef _G_FRAME_PERFANAL
	trap->PrecisionTimer_Start(&timer_GameChecks);
#endif
	// see if it is time to do a tournament restart
	CheckTournament();

	// see if it is time to end the level
	CheckExitRules();

	// update to team status?
	CheckTeamStatus();

	// cancel vote if timed out
	CheckVote();

	// check team votes
	CheckTeamVote( TEAM_RED );
	CheckTeamVote( TEAM_BLUE );

	// for tracking changes
	CheckCvars();

#ifdef _G_FRAME_PERFANAL
	iTimer_GameChecks = trap->PrecisionTimer_End(timer_GameChecks);
#endif



#ifdef _G_FRAME_PERFANAL
	trap->PrecisionTimer_Start(&timer_Queues);
#endif
	//At the end of the frame, send out the ghoul2 kill queue, if there is one
	G_SendG2KillQueue();

	if (gQueueScoreMessage)
	{
		if (gQueueScoreMessageTime < level.time)
		{
			SendScoreboardMessageToAllClients();

			gQueueScoreMessageTime = 0;
			gQueueScoreMessage = 0;
		}
	}
#ifdef _G_FRAME_PERFANAL
	iTimer_Queues = trap->PrecisionTimer_End(timer_Queues);
#endif



#ifdef _G_FRAME_PERFANAL
	Com_Printf("---------------\nItemRun: %i\nROFF: %i\nClientEndframe: %i\nGameChecks: %i\nQueues: %i\n---------------\n",
		iTimer_ItemRun,
		iTimer_ROFF,
		iTimer_ClientEndframe,
		iTimer_GameChecks,
		iTimer_Queues);
#endif

	g_LastFrameTime = level.time;
}

const char *G_GetStringEdString(char *refSection, char *refName)
{
	/*
	static char text[1024]={0};
	trap->SP_GetStringTextString(va("%s_%s", refSection, refName), text, sizeof(text));
	return text;
	*/

	//Well, it would've been lovely doing it the above way, but it would mean mixing
	//languages for the client depending on what the server is. So we'll mark this as
	//a stringed reference with @@@ and send the refname to the client, and when it goes
	//to print it will get scanned for the stringed reference indication and dealt with
	//properly.
	static char text[1024]={0};
	Com_sprintf(text, sizeof(text), "@@@%s", refName);
	return text;
}

static void G_SpawnRMGEntity( void ) {
	if ( G_ParseSpawnVars( qfalse ) )
		G_SpawnGEntityFromSpawnVars( qfalse );
}

static void _G_ROFF_NotetrackCallback( int entID, const char *notetrack ) {
	G_ROFF_NotetrackCallback( &g_entities[entID], notetrack );
}

static int G_ICARUS_PlaySound( void ) {
	T_G_ICARUS_PLAYSOUND *sharedMem = &gSharedBuffer.playSound;
	return Q3_PlaySound( sharedMem->taskID, sharedMem->entID, sharedMem->name, sharedMem->channel );
}
static qboolean G_ICARUS_Set( void ) {
	T_G_ICARUS_SET *sharedMem = &gSharedBuffer.set;
	return Q3_Set( sharedMem->taskID, sharedMem->entID, sharedMem->type_name, sharedMem->data );
}
static void G_ICARUS_Lerp2Pos( void ) {
	T_G_ICARUS_LERP2POS *sharedMem = &gSharedBuffer.lerp2Pos;
	Q3_Lerp2Pos( sharedMem->taskID, sharedMem->entID, sharedMem->origin, sharedMem->nullAngles ? NULL : sharedMem->angles, sharedMem->duration );
}
static void G_ICARUS_Lerp2Origin( void ) {
	T_G_ICARUS_LERP2ORIGIN *sharedMem = &gSharedBuffer.lerp2Origin;
	Q3_Lerp2Origin( sharedMem->taskID, sharedMem->entID, sharedMem->origin, sharedMem->duration );
}
static void G_ICARUS_Lerp2Angles( void ) {
	T_G_ICARUS_LERP2ANGLES *sharedMem = &gSharedBuffer.lerp2Angles;
	Q3_Lerp2Angles( sharedMem->taskID, sharedMem->entID, sharedMem->angles, sharedMem->duration );
}
static int G_ICARUS_GetTag( void ) {
	T_G_ICARUS_GETTAG *sharedMem = &gSharedBuffer.getTag;
	return Q3_GetTag( sharedMem->entID, sharedMem->name, sharedMem->lookup, sharedMem->info );
}
static void G_ICARUS_Lerp2Start( void ) {
	T_G_ICARUS_LERP2START *sharedMem = &gSharedBuffer.lerp2Start;
	Q3_Lerp2Start( sharedMem->entID, sharedMem->taskID, sharedMem->duration );
}
static void G_ICARUS_Lerp2End( void ) {
	T_G_ICARUS_LERP2END *sharedMem = &gSharedBuffer.lerp2End;
	Q3_Lerp2End( sharedMem->entID, sharedMem->taskID, sharedMem->duration );
}
static void G_ICARUS_Use( void ) {
	T_G_ICARUS_USE *sharedMem = &gSharedBuffer.use;
	Q3_Use( sharedMem->entID, sharedMem->target );
}
static void G_ICARUS_Kill( void ) {
	T_G_ICARUS_KILL *sharedMem = &gSharedBuffer.kill;
	Q3_Kill( sharedMem->entID, sharedMem->name );
}
static void G_ICARUS_Remove( void ) {
	T_G_ICARUS_REMOVE *sharedMem = &gSharedBuffer.remove;
	Q3_Remove( sharedMem->entID, sharedMem->name );
}
static void G_ICARUS_Play( void ) {
	T_G_ICARUS_PLAY *sharedMem = &gSharedBuffer.play;
	Q3_Play( sharedMem->taskID, sharedMem->entID, sharedMem->type, sharedMem->name );
}
static int G_ICARUS_GetFloat( void ) {
	T_G_ICARUS_GETFLOAT *sharedMem = &gSharedBuffer.getFloat;
	return Q3_GetFloat( sharedMem->entID, sharedMem->type, sharedMem->name, &sharedMem->value );
}
static int G_ICARUS_GetVector( void ) {
	T_G_ICARUS_GETVECTOR *sharedMem = &gSharedBuffer.getVector;
	return Q3_GetVector( sharedMem->entID, sharedMem->type, sharedMem->name, sharedMem->value );
}
static int G_ICARUS_GetString( void ) {
	T_G_ICARUS_GETSTRING *sharedMem = &gSharedBuffer.getString;
	char *crap = NULL; //I am sorry for this -rww
	char **morecrap = &crap; //and this
	int r = Q3_GetString( sharedMem->entID, sharedMem->type, sharedMem->name, morecrap );

	if ( crap )
		strcpy( sharedMem->value, crap );

	return r;
}
static void G_ICARUS_SoundIndex( void ) {
	T_G_ICARUS_SOUNDINDEX *sharedMem = &gSharedBuffer.soundIndex;
	G_SoundIndex( sharedMem->filename );
}
static int G_ICARUS_GetSetIDForString( void ) {
	T_G_ICARUS_GETSETIDFORSTRING *sharedMem = &gSharedBuffer.getSetIDForString;
	return GetIDForString( setTable, sharedMem->string );
}
static qboolean G_NAV_ClearPathToPoint( int entID, vec3_t pmins, vec3_t pmaxs, vec3_t point, int clipmask, int okToHitEnt ) {
	return NAV_ClearPathToPoint( &g_entities[entID], pmins, pmaxs, point, clipmask, okToHitEnt );
}
static qboolean G_NPC_ClearLOS2( int entID, const vec3_t end ) {
	return NPC_ClearLOS2( &g_entities[entID], end );
}
static qboolean	G_NAV_CheckNodeFailedForEnt( int entID, int nodeNum ) {
	return NAV_CheckNodeFailedForEnt( &g_entities[entID], nodeNum );
}

/*
============
GetModuleAPI
============
*/

gameImport_t *trap = NULL;

Q_EXPORT gameExport_t* QDECL GetModuleAPI( int apiVersion, gameImport_t *import )
{
	static gameExport_t ge = {0};

	assert( import );
	trap = import;
	Com_Printf	= trap->Print;
	Com_Error	= trap->Error;

	memset( &ge, 0, sizeof( ge ) );

	if ( apiVersion != GAME_API_VERSION ) {
		trap->Print( "Mismatched GAME_API_VERSION: expected %i, got %i\n", GAME_API_VERSION, apiVersion );
		return NULL;
	}

	ge.InitGame							= G_InitGame;
	ge.ShutdownGame						= G_ShutdownGame;
	ge.ClientConnect					= ClientConnect;
	ge.ClientBegin						= ClientBegin;
	ge.ClientUserinfoChanged			= ClientUserinfoChanged;
	ge.ClientDisconnect					= ClientDisconnect;
	ge.ClientCommand					= ClientCommand;
	ge.ClientThink						= ClientThink;
	ge.RunFrame							= G_RunFrame;
	ge.ConsoleCommand					= ConsoleCommand;
	ge.BotAIStartFrame					= BotAIStartFrame;
	ge.ROFF_NotetrackCallback			= _G_ROFF_NotetrackCallback;
	ge.SpawnRMGEntity					= G_SpawnRMGEntity;
	ge.ICARUS_PlaySound					= G_ICARUS_PlaySound;
	ge.ICARUS_Set						= G_ICARUS_Set;
	ge.ICARUS_Lerp2Pos					= G_ICARUS_Lerp2Pos;
	ge.ICARUS_Lerp2Origin				= G_ICARUS_Lerp2Origin;
	ge.ICARUS_Lerp2Angles				= G_ICARUS_Lerp2Angles;
	ge.ICARUS_GetTag					= G_ICARUS_GetTag;
	ge.ICARUS_Lerp2Start				= G_ICARUS_Lerp2Start;
	ge.ICARUS_Lerp2End					= G_ICARUS_Lerp2End;
	ge.ICARUS_Use						= G_ICARUS_Use;
	ge.ICARUS_Kill						= G_ICARUS_Kill;
	ge.ICARUS_Remove					= G_ICARUS_Remove;
	ge.ICARUS_Play						= G_ICARUS_Play;
	ge.ICARUS_GetFloat					= G_ICARUS_GetFloat;
	ge.ICARUS_GetVector					= G_ICARUS_GetVector;
	ge.ICARUS_GetString					= G_ICARUS_GetString;
	ge.ICARUS_SoundIndex				= G_ICARUS_SoundIndex;
	ge.ICARUS_GetSetIDForString			= G_ICARUS_GetSetIDForString;
	ge.NAV_ClearPathToPoint				= G_NAV_ClearPathToPoint;
	ge.NPC_ClearLOS2					= G_NPC_ClearLOS2;
	ge.NAVNEW_ClearPathBetweenPoints	= NAVNEW_ClearPathBetweenPoints;
	ge.NAV_CheckNodeFailedForEnt		= G_NAV_CheckNodeFailedForEnt;
	ge.NAV_EntIsUnlockedDoor			= G_EntIsUnlockedDoor;
	ge.NAV_EntIsDoor					= G_EntIsDoor;
	ge.NAV_EntIsBreakable				= G_EntIsBreakable;
	ge.NAV_EntIsRemovableUsable			= G_EntIsRemovableUsable;
	ge.NAV_FindCombatPointWaypoints		= CP_FindCombatPointWaypoints;
	ge.BG_GetItemIndexByTag				= BG_GetItemIndexByTag;

	return &ge;
}

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
Q_EXPORT intptr_t vmMain( int command, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4,
	intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11 )
{
	switch ( command ) {
	case GAME_INIT:
		G_InitGame( arg0, arg1, arg2 );
		return 0;

	case GAME_SHUTDOWN:
		G_ShutdownGame( arg0 );
		return 0;

	case GAME_CLIENT_CONNECT:
		return (intptr_t)ClientConnect( arg0, arg1, arg2 );

	case GAME_CLIENT_THINK:
		ClientThink( arg0, NULL );
		return 0;

	case GAME_CLIENT_USERINFO_CHANGED:
		ClientUserinfoChanged( arg0 );
		return 0;

	case GAME_CLIENT_DISCONNECT:
		ClientDisconnect( arg0 );
		return 0;

	case GAME_CLIENT_BEGIN:
		ClientBegin( arg0, qtrue );
		return 0;

	case GAME_CLIENT_COMMAND:
		ClientCommand( arg0 );
		return 0;

	case GAME_RUN_FRAME:
		G_RunFrame( arg0 );
		return 0;

	case GAME_CONSOLE_COMMAND:
		return ConsoleCommand();

	case BOTAI_START_FRAME:
		return BotAIStartFrame( arg0 );

	case GAME_ROFF_NOTETRACK_CALLBACK:
		_G_ROFF_NotetrackCallback( arg0, (const char *)arg1 );
		return 0;

	case GAME_SPAWN_RMG_ENTITY:
		G_SpawnRMGEntity();
		return 0;

	case GAME_ICARUS_PLAYSOUND:
		return G_ICARUS_PlaySound();

	case GAME_ICARUS_SET:
		return G_ICARUS_Set();

	case GAME_ICARUS_LERP2POS:
		G_ICARUS_Lerp2Pos();
		return 0;

	case GAME_ICARUS_LERP2ORIGIN:
		G_ICARUS_Lerp2Origin();
		return 0;

	case GAME_ICARUS_LERP2ANGLES:
		G_ICARUS_Lerp2Angles();
		return 0;

	case GAME_ICARUS_GETTAG:
		return G_ICARUS_GetTag();

	case GAME_ICARUS_LERP2START:
		G_ICARUS_Lerp2Start();
		return 0;

	case GAME_ICARUS_LERP2END:
		G_ICARUS_Lerp2End();
		return 0;

	case GAME_ICARUS_USE:
		G_ICARUS_Use();
		return 0;

	case GAME_ICARUS_KILL:
		G_ICARUS_Kill();
		return 0;

	case GAME_ICARUS_REMOVE:
		G_ICARUS_Remove();
		return 0;

	case GAME_ICARUS_PLAY:
		G_ICARUS_Play();
		return 0;

	case GAME_ICARUS_GETFLOAT:
		return G_ICARUS_GetFloat();

	case GAME_ICARUS_GETVECTOR:
		return G_ICARUS_GetVector();

	case GAME_ICARUS_GETSTRING:
		return G_ICARUS_GetString();

	case GAME_ICARUS_SOUNDINDEX:
		G_ICARUS_SoundIndex();
		return 0;

	case GAME_ICARUS_GETSETIDFORSTRING:
		return G_ICARUS_GetSetIDForString();

	case GAME_NAV_CLEARPATHTOPOINT:
		return G_NAV_ClearPathToPoint( arg0, (float *)arg1, (float *)arg2, (float *)arg3, arg4, arg5 );

	case GAME_NAV_CLEARLOS:
		return G_NPC_ClearLOS2( arg0, (const float *)arg1 );

	case GAME_NAV_CLEARPATHBETWEENPOINTS:
		return NAVNEW_ClearPathBetweenPoints((float *)arg0, (float *)arg1, (float *)arg2, (float *)arg3, arg4, arg5);

	case GAME_NAV_CHECKNODEFAILEDFORENT:
		return NAV_CheckNodeFailedForEnt(&g_entities[arg0], arg1);

	case GAME_NAV_ENTISUNLOCKEDDOOR:
		return G_EntIsUnlockedDoor(arg0);

	case GAME_NAV_ENTISDOOR:
		return G_EntIsDoor(arg0);

	case GAME_NAV_ENTISBREAKABLE:
		return G_EntIsBreakable(arg0);

	case GAME_NAV_ENTISREMOVABLEUSABLE:
		return G_EntIsRemovableUsable(arg0);

	case GAME_NAV_FINDCOMBATPOINTWAYPOINTS:
		CP_FindCombatPointWaypoints();
		return 0;

	case GAME_GETITEMINDEXBYTAG:
		return BG_GetItemIndexByTag(arg0, arg1);
	}

	return -1;
}
