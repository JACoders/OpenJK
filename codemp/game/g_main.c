// Copyright (C) 1999-2000 Id Software, Inc.
//

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
		zyk_set_entity_field(spawn_ent,"classname","info_player_deathmatch");
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
	trap->Cvar_Register( &ckSum, "sv_mapChecksum", "", CVAR_ROM );

	navCalculatePaths	= ( trap->Nav_Load( mapname.string, ckSum.integer ) == qfalse );

	// parse the key/value pairs and spawn gentities
	G_SpawnEntitiesFromString(qfalse);

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

	// zyk: initializing quest_model_id value
	level.quest_model_id = -1;

	// zyk: initializing quest_note_id value
	level.quest_note_id = -1;

	// zyk: initializing quest_effect_id value
	level.quest_effect_id = -1;

	// zyk: initializing bounty_quest_target_id value
	level.bounty_quest_target_id = 0;
	level.bounty_quest_choose_target = qtrue;

	// zyk: initializing guardian quest values
	level.guardian_quest = 0;
	level.guardian_quest_timer = 0;
	level.validated_map_guardian = qfalse;
	level.map_guardian_counter = 0;
	level.initial_map_guardian_weapons = 0;

	// zyk: getting mapname
	Q_strncpyz(zyk_mapname, Info_ValueForKey( serverinfo, "mapname" ), sizeof(zyk_mapname));

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
		level.quest_map = 5;

		zyk_create_info_player_deathmatch(-2114,10195,1027,-14);
		zyk_create_info_player_deathmatch(-1808,9640,982,-17);
	}
	else if (Q_stricmp(zyk_mapname, "hoth3") == 0)
	{
		level.quest_map = 20;

		zyk_create_info_player_deathmatch(-1908,562,992,-90);
		zyk_create_info_player_deathmatch(-1907,356,801,-90);
	}
	else if (Q_stricmp(zyk_mapname, "t1_danger") == 0)
	{
		level.quest_map = 18;

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
			if (Q_stricmp( ent->classname, "func_door") == 0 && i > 200 && ent->spawnflags == 16 && Q_stricmp( ent->model, "*63") == 0)
			{ // zyk: tube door in which the droid goes in SP
				G_FreeEntity( ent );
			}
		}
		zyk_create_info_player_deathmatch(1913,-6151,222,153);
		zyk_create_info_player_deathmatch(1921,-5812,222,-179);
	}
	else if (Q_stricmp(zyk_mapname, "t2_rancor") == 0)
	{
		int i = 0;
		gentity_t *ent;

		level.quest_map = 26;

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
			if (Q_stricmp( ent->targetname, "t466") == 0)
			{ // zyk: big front door at Lannik Racto building
				G_FreeEntity( ent );
			}
			/*
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
			*/
		}
		// zyk: spawning Lannik Racto npc
		Zyk_NPC_SpawnType("lannik_racto",1950,1500,729,90);
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
			else if (Q_stricmp( ent->classname, "func_door") == 0 && i > 200)
			{ // zyk: door after the teleports of the race mode
				G_FreeEntity( ent );
			}
		}
		zyk_create_info_player_deathmatch(-5698,-22304,1705,90);
		zyk_create_info_player_deathmatch(-5433,-22328,1705,90);
	}
	else if (Q_stricmp(zyk_mapname, "t2_wedge") == 0)
	{
		level.quest_map = 15;

		zyk_create_info_player_deathmatch(6328,539,-110,-178);
		zyk_create_info_player_deathmatch(6332,743,-110,-178);
	}
	else if (Q_stricmp(zyk_mapname, "t2_dpred") == 0)
	{
		int i = 0;
		gentity_t *ent;

		level.quest_map = 22;

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
			/*
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
			*/
		}
		// zyk: spawning Rax Joris npc
		Zyk_NPC_SpawnType("rax",3080,-3700,921,90);
		zyk_create_info_player_deathmatch(-2152,-3885,-134,90);
		zyk_create_info_player_deathmatch(-2152,-3944,-134,90);
	}
	else if (Q_stricmp(zyk_mapname, "vjun1") == 0)
	{
		zyk_create_info_player_deathmatch(-6897,7035,857,-90);
		zyk_create_info_player_deathmatch(-7271,7034,857,-90);
	}
	else if (Q_stricmp(zyk_mapname, "vjun2") == 0)
	{
		int i = 0;
		gentity_t *ent;
		for (i = 0; i < level.num_entities; i++)
		{
			ent = &g_entities[i];
			if (Q_stricmp( ent->targetname, "kyleDoor1") == 0)
			{
				fix_sp_func_door(ent);
			}
		}
		zyk_create_info_player_deathmatch(-831,166,217,90);
		zyk_create_info_player_deathmatch(-700,166,217,90);
	}
	else if (Q_stricmp(zyk_mapname, "vjun3") == 0)
	{
		/*
		int i = 0;
		gentity_t *ent;

		for (i = 0; i < level.num_entities; i++)
		{
			ent = &g_entities[i];
			if (Q_stricmp( ent->targetname, "front_door") == 0)
			{
				G_FreeEntity( ent );
			}
		}
		*/
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
			/*
			if (Q_stricmp( ent->target, "field_counter1") == 0)
			{
				G_FreeEntity( ent );
			}
			*/
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

		level.quest_map = 11;

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
		level.quest_map = 9;

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
		// zyk: spawning Alora npc to fight against her in this map
		Zyk_NPC_SpawnType("alora_dual",-10423,-992,2417,0);
		zyk_create_info_player_deathmatch(286,-2859,345,92);
		zyk_create_info_player_deathmatch(190,-2834,345,90);
	}
	else if (Q_stricmp(zyk_mapname, "mp/duel8") == 0 && g_gametype.integer == GT_FFA)
	{
		level.quest_map = 14;
	}
	else if (Q_stricmp(zyk_mapname, "mp/siege_korriban") == 0 && g_gametype.integer == GT_FFA)
	{ // zyk: if its a FFA game, then remove the coffin and the coffin shield in the final part
		int i = 0;
		gentity_t *ent;

		level.quest_map = 12;

		for (i = 0; i < level.num_entities; i++)
		{
			ent = &g_entities[i];
			if (Q_stricmp( ent->targetname, "coffinguard_shield") == 0)
			{
				G_FreeEntity( ent );
			}
			if (Q_stricmp( ent->targetname, "cyrstalsinplace") == 0)
			{
				G_FreeEntity( ent );
			}
			if (i == 247)
			{ // zyk: removing the trigger_hurt of the lava in Guardian of Universe arena
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

				if (nextMap && nextMap[0])
				{
					trap->SendConsoleCommand( EXEC_APPEND, va("map %s\n", nextMap ) );
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
					if (currentFL > 3 || !currentFL)
					{ //if voting to duel, and fraglimit is more than 3 (or unlimited), then set it down to 3
						trap->SendConsoleCommand(EXEC_APPEND, "fraglimit 3\n");
					}
					if (currentTL)
					{ //if voting to duel, and timelimit is set, make it unlimited
						trap->SendConsoleCommand(EXEC_APPEND, "timelimit 0\n");
					}
				}
				else if ((level.votingGametypeTo != GT_DUEL && level.votingGametypeTo != GT_POWERDUEL) &&
					(currentGT == GT_DUEL || currentGT == GT_POWERDUEL))
				{
					if (currentFL && currentFL < 20)
					{ //if voting from duel, an fraglimit is less than 20, then set it up to 20
						trap->SendConsoleCommand(EXEC_APPEND, "fraglimit 20\n");
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
	if ( level.time-level.voteTime >= VOTE_TIME || level.voteYes + level.voteNo == 0 ) 
	{
		if (level.voteYes > level.voteNo)
		{ // zyk: now vote pass if number of Yes is greater than number of No
			trap->SendServerCommand( -1, va("print \"%s (%s)\n\"", G_GetStringEdString("MP_SVGAME", "VOTEPASSED"), level.voteStringClean) );
			level.voteExecuteTime = level.time + level.voteExecuteDelay;
		}
		else
			trap->SendServerCommand( -1, va("print \"%s (%s)\n\"", G_GetStringEdString("MP_SVGAME", "VOTEFAILED"), level.voteStringClean) );
	}
	else 
	{
		if ( level.voteYes > level.numVotingClients/2 ) {
			// execute the command, then remove the vote
			trap->SendServerCommand( -1, va("print \"%s (%s)\n\"", G_GetStringEdString("MP_SVGAME", "VOTEPASSED"), level.voteStringClean) );
			level.voteExecuteTime = level.time + level.voteExecuteDelay;
		}

		// same behavior as a timeout
		else if ( level.voteNo >= (level.numVotingClients+1)/2 )
			trap->SendServerCommand( -1, va("print \"%s (%s)\n\"", G_GetStringEdString("MP_SVGAME", "VOTEFAILED"), level.voteStringClean) );

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
	qboolean	noAngles;
	if (player->s.eType == ET_NPC)
	{
		isNPC = qtrue;
	}

	noAngles = (angles[0] > 999999.0) ? qtrue : qfalse;

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

// zyk: function to kill npcs with the name as parameter. It uses G_FreeEntity to prevent some bugs in quests because of player_die function call
void zyk_NPC_Kill_f( char *name )
{
	int	n = 0;
	gentity_t *player = NULL;

	for ( n = level.maxclients; n < level.num_entities; n++) 
	{
		player = &g_entities[n];
		if ( player && player->NPC && player->client )
		{
			if( (Q_stricmp( name, player->NPC_type ) == 0) || Q_stricmp( name, "all" ) == 0)
			{
				G_FreeEntity(player);
			}
		}
	}
}

// zyk: spawns a RPG quest boss and set his HP based in the quantity of allies the quest player has now
extern void clean_effect();
extern gentity_t *NPC_SpawnType( gentity_t *ent, char *npc_type, char *targetname, qboolean isVehicle ); // zyk: used in boss battles
void spawn_boss(gentity_t *ent,int x,int y,int z,int yaw,char *boss_name,int gx,int gy,int gz,int gyaw,int guardian_mode)
{
	vec3_t player_origin;
	vec3_t player_yaw;
	gentity_t *npc_ent = NULL;
	int number_of_allies = 0;

	if (ent->client->sess.ally1 != -1)
	{
		g_entities[ent->client->sess.ally1].client->pers.guardian_mode = guardian_mode;
		number_of_allies++;
	}
	if (ent->client->sess.ally2 != -1)
	{
		g_entities[ent->client->sess.ally2].client->pers.guardian_mode = guardian_mode;
		number_of_allies++;
	}
	if (ent->client->sess.ally3 != -1)
	{
		g_entities[ent->client->sess.ally3].client->pers.guardian_mode = guardian_mode;
		number_of_allies++;
	}

	if (ent->client->pers.guardian_mode == 0 && guardian_mode != 14)
		zyk_NPC_Kill_f("all");

	ent->client->pers.guardian_mode = guardian_mode;

	player_origin[0] = x;
	player_origin[1] = y;
	player_origin[2] = z;
	player_yaw[0] = 0;
	player_yaw[1] = yaw;
	player_yaw[2] = 0;

	if (guardian_mode != 14)
		zyk_TeleportPlayer(ent,player_origin,player_yaw);

	if ((guardian_mode >= 1 && guardian_mode <= 7) || guardian_mode == 11 || guardian_mode == 14)
		npc_ent = Zyk_NPC_SpawnType(boss_name,gx,gy,gz,gyaw);
	else
		npc_ent = NPC_SpawnType(ent,boss_name,NULL,qfalse);

	if (npc_ent)
	{
		if (guardian_mode != 14)
		{
			npc_ent->NPC->stats.health += (npc_ent->NPC->stats.health/5 * number_of_allies);
			npc_ent->client->ps.stats[STAT_MAX_HEALTH] = npc_ent->NPC->stats.health;
			npc_ent->health = npc_ent->client->ps.stats[STAT_MAX_HEALTH];
		}
		npc_ent->client->pers.guardian_invoked_by_id = ent-g_entities;
		npc_ent->client->pers.guardian_weapons_backup = npc_ent->client->ps.stats[STAT_WEAPONS];
		npc_ent->client->pers.hunter_quest_messages = 0;
		npc_ent->client->pers.guardian_timer = level.time + 5000;
		npc_ent->client->pers.guardian_mode = guardian_mode;
	}

	clean_effect();
}

// zyk: Healing Water
void healing_water(gentity_t *ent, int heal_amount)
{
	if ((ent->health + heal_amount) < ent->client->ps.stats[STAT_MAX_HEALTH])
		ent->health += heal_amount;
	else
		ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
}

// zyk: Earthquake
void earthquake(gentity_t *ent, int stun_time, int strength, int distance)
{
	int i = 0;

	for ( i = 0; i < level.num_entities; i++)
	{
		gentity_t *player_ent = &g_entities[i];

		if (ent->s.number != i && player_ent && player_ent->client)
		{
			int player_distance = (int)Distance(ent->client->ps.origin,player_ent->client->ps.origin);

			if (player_distance < distance)
			{
				int found = 0;

				// zyk: allies will not be hit by this power
				if (i < level.maxclients && !ent->NPC && (ent->client->sess.ally1 == i || ent->client->sess.ally2 == i || ent->client->sess.ally3 == i))
				{
					found = 1;
				}

				if (found == 0)
				{
					if (player_ent->client->ps.groundEntityNum != ENTITYNUM_NONE)
					{ // zyk: player can only be hit if he is on floor
						player_ent->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
						player_ent->client->ps.forceHandExtendTime = level.time + stun_time;
						player_ent->client->ps.velocity[2] += strength;
						player_ent->client->ps.forceDodgeAnim = 0;
						player_ent->client->ps.quickerGetup = qtrue;
					}

					if (i < level.maxclients)
					{
						G_ScreenShake(player_ent->client->ps.origin, player_ent,  10.0f, 4000, qtrue); 
						G_Sound(player_ent, CHAN_AUTO, G_SoundIndex("sound/effects/stone_break1.mp3"));
					}
				}
			}
		}
	}
}

// zyk: Sleeping Flowers
void sleeping_flowers(gentity_t *ent, int stun_time, int distance)
{
	int i = 0;

	for (i = 0; i < level.num_entities; i++)
	{
		gentity_t *player_ent = &g_entities[i];

		if (ent->s.number != i && player_ent && player_ent->client)
		{
			int player_distance = (int)Distance(ent->client->ps.origin,player_ent->client->ps.origin);

			if (player_distance < distance)
			{
				int found = 0;

				// zyk: allies will not be hit by this power
				if (i < level.maxclients && !ent->NPC && (ent->client->sess.ally1 == i || ent->client->sess.ally2 == i || ent->client->sess.ally3 == i))
				{
					found = 1;
				}

				if (found == 0)
				{
					player_ent->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
					player_ent->client->ps.forceHandExtendTime = level.time + stun_time;
					player_ent->client->ps.velocity[2] += 150;
					player_ent->client->ps.forceDodgeAnim = 0;
					player_ent->client->ps.quickerGetup = qtrue;

					if (i < level.maxclients)
						G_Sound(player_ent, CHAN_AUTO, G_SoundIndex("sound/effects/air_burst.mp3"));
				}
			}
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

	vec3_t	tfrom, tto, fwd;
	vec3_t thispush_org, a;
	vec3_t mins, maxs, fwdangles, forward, right, center;
	vec3_t		origin, dir;

	int i;
	float visionArc = 120;
	float radius = 144;

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
		}
		traceEnt = &g_entities[entityList[e]];
		if (traceEnt && traceEnt != self)
		{
			G_Damage( traceEnt, self, self, self->client->ps.viewangles, tr.endpos, 1, DAMAGE_NO_KNOCKBACK|DAMAGE_IGNORE_TEAM, MOD_LAVA );
		}
		e++;
	}
}

// zyk: controls Ultimate Power target events
void ultimate_power_events(gentity_t *ent)
{
	// zyk: controlling Ultimate Power target events
	if (ent && ent->client && ent->client->pers.ultimate_power_target_timer > level.time)
	{
		if (ent->client->pers.ultimate_power_target == 3 && ent->client->pers.ultimate_power_target_timer < (level.time + 1000))
		{ // zyk: removes Time Power
			ent->client->pers.ultimate_power_target = -1;
		}
		else if (ent->client->pers.ultimate_power_target == 4 && ent->client->pers.ultimate_power_target_timer < (level.time + 1000))
		{ // zyk: Second Chaos Power hit
			G_Damage(ent,NULL,NULL,NULL,NULL,100,0,MOD_UNKNOWN);

			ent->client->pers.ultimate_power_target = 5;
			ent->client->pers.ultimate_power_target_timer = level.time + 2000;
		}
		else if (ent->client->pers.ultimate_power_target == 5 && ent->client->pers.ultimate_power_target_timer < (level.time + 1000))
		{ // zyk: Third Chaos Power hit
			G_Damage(ent,NULL,NULL,NULL,NULL,100,0,MOD_UNKNOWN);

			ent->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
			ent->client->ps.forceHandExtendTime = level.time + 3000;
			ent->client->ps.velocity[2] += 800;
			ent->client->ps.forceDodgeAnim = 0;
			ent->client->ps.quickerGetup = qtrue;

			ent->client->pers.ultimate_power_target = 0;
		}
		else if (ent->client->pers.ultimate_power_target == 10)
		{ // zyk: being hit by Blowing Wind
			static vec3_t forward;
			vec3_t dir;

			AngleVectors( g_entities[ent->client->pers.ultimate_power_user].r.currentAngles, forward, NULL, NULL );

			VectorNormalize(forward);

			if (ent->client->ps.groundEntityNum != ENTITYNUM_NONE)
				VectorScale(forward,90.0,dir);
			else
				VectorScale(forward,25.0,dir);

			VectorAdd(ent->client->ps.velocity, dir, ent->client->ps.velocity);
		}
	}
}

// zyk: tests if player already finished the first Universe Quest Second Act objective
void first_second_act_objective(gentity_t *ent)
{
	int i = 0;
	int count = 0;

	for (i = 0; i < 3; i++)
	{
		if (ent->client->pers.universe_quest_counter & (1 << i))
			count++;
	}

	if (count == 3)
	{
		ent->client->pers.universe_quest_progress = 9;
		ent->client->pers.universe_quest_counter = 0;
	}
}

// zyk: checks if the player has already all artifacts
extern void save_account(gentity_t *ent);
extern int number_of_artifacts(gentity_t *ent);
void universe_quest_artifacts_checker(gentity_t *ent)
{
	if (number_of_artifacts(ent) == 10)
	{ // zyk: after collecting all artifacts, go to next objective
		trap->SendServerCommand( -1, va("chat \"%s^7: I have all artifacts! Now I must go to ^3yavin1b ^7to know about the mysterious voice I heard when I begun this quest.\"", ent->client->pers.netname));

		ent->client->pers.universe_quest_counter = 0;
		ent->client->pers.universe_quest_progress = 3;
		ent->client->pers.universe_quest_timer = level.time + 1000;
		ent->client->pers.universe_quest_objective_control = 4; // zyk: fourth Universe Quest objective
		ent->client->pers.universe_quest_messages = 0;
		
		save_account(ent);
	}
}

// zyk: checks if the player got all crystals
void universe_crystals_check(gentity_t *ent)
{
	int i = 0;
	int count = 0;

	for (i = 0; i < 3; i++)
	{
		if (ent->client->pers.universe_quest_counter & (1 << i))
			count++;
	}

	if (count == 3)
	{
		ent->client->pers.universe_quest_progress = 10;
		ent->client->pers.universe_quest_counter = 0;
	}
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
extern void Jedi_Decloak( gentity_t *self );
qboolean G_PointInBounds( vec3_t point, vec3_t mins, vec3_t maxs );

int g_siegeRespawnCheck = 0;
void SetMoverState( gentity_t *ent, moverState_t moverState, int time );

extern void add_credits(gentity_t *ent, int credits);
extern void remove_credits(gentity_t *ent, int credits);
extern void try_finishing_race();
extern void quest_get_new_player(gentity_t *ent);
extern void clean_note_model();
extern gentity_t *load_crystal_model(int x,int y,int z, int yaw, int crystal_number);
extern void clean_crystal_model();
extern qboolean dark_quest_collected_notes(gentity_t *ent);
extern qboolean light_quest_defeated_guardians(gentity_t *ent);
extern void set_max_health(gentity_t *ent);
extern void set_max_shield(gentity_t *ent);
extern gentity_t *load_effect(int x,int y,int z, int spawnflags, char *fxFile);

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

	if (level.race_mode == 1 && level.race_start_timer < level.time)
	{ // zyk: Race Mode. Tests if we should start the race
		level.race_countdown = 5;
		level.race_countdown_timer = level.time + 5100;
		level.race_last_player_position = 0;
		level.race_mode = 2;
	}
	else if (level.race_mode == 2)
	{ // zyk: Race Mode. Shows the countdown messages in players screens and starts the race
		if (level.race_countdown_timer < (level.time + 5000) && level.race_countdown == 5)
		{
			level.race_countdown = 4;
			trap->SendServerCommand( -1, "cp \"^7Race starts in ^35\"");
		}
		else if (level.race_countdown_timer < (level.time + 4000) && level.race_countdown == 4)
		{
			level.race_countdown = 3;
			trap->SendServerCommand( -1, "cp \"^7Race starts in ^34\"");
		}
		else if (level.race_countdown_timer < (level.time + 3000) && level.race_countdown == 3)
		{
			level.race_countdown = 2;
			trap->SendServerCommand( -1, "cp \"^7Race starts in ^33\"");
		}
		else if (level.race_countdown_timer < (level.time + 2000) && level.race_countdown == 2)
		{
			level.race_countdown = 1;
			trap->SendServerCommand( -1, "cp \"^7Race starts in ^32\"");
		}
		else if (level.race_countdown_timer < (level.time + 1000) && level.race_countdown == 1)
		{
			level.race_countdown = 0;
			trap->SendServerCommand( -1, "cp \"^7Race starts in ^31\"");
		}
		else if (level.race_countdown_timer < level.time)
		{
			level.race_mode = 3;
			trap->SendServerCommand( -1, "cp \"^2Go!\"");
		}
	}

	// zyk: Guardian Quest. Randomly spawns an npc in the map
	if (level.guardian_quest == 1 && level.guardian_quest_timer < level.time)
	{
		int random_number = Q_irand(0,level.numConnectedClients);

		if (random_number == level.numConnectedClients)
			random_number--;

		if (random_number >= 0 && random_number < level.numConnectedClients)
		{
			gentity_t *npc_ent = NULL;
			gentity_t *player_ent = &g_entities[random_number];

			if (player_ent && player_ent->client)
			{
				npc_ent = Zyk_NPC_SpawnType("map_guardian",player_ent->client->ps.origin[0],player_ent->client->ps.origin[1],(player_ent->client->ps.origin[2] + 150),0);

				if (npc_ent)
				{
					npc_ent->client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << HI_JETPACK);
					level.initial_map_guardian_weapons = npc_ent->client->ps.stats[STAT_WEAPONS];
					VectorCopy(npc_ent->client->ps.origin,level.initial_guardian_origin);
					level.guardian_quest = npc_ent->s.number;
				}
			}
		}

		level.guardian_quest_timer = level.time + 5000;
	}
	else if (level.guardian_quest > 1 && level.guardian_quest_timer < level.time)
	{
		gentity_t *npc_ent = &g_entities[level.guardian_quest];

		if (level.validated_map_guardian == qfalse && VectorCompare(npc_ent->client->ps.origin,level.initial_guardian_origin) == 1)
		{
			level.guardian_quest = 0;
			zyk_NPC_Kill_f("map_guardian");
			trap->SendServerCommand( -1, va("print \"Map guardian was stuck, try again later.\n\"") );
		}
		else
		{
			level.validated_map_guardian = qtrue;
		}

		if (level.validated_map_guardian == qtrue)
		{
			if (level.map_guardian_counter < 5)
			{
				npc_ent = NPC_SpawnType(npc_ent,"map_guardian_support",NULL,qfalse);
				if (npc_ent)
				{
					level.map_guardian_counter++;
				}
			}
		
			if (npc_ent && npc_ent->client)
				npc_ent->client->ps.stats[STAT_WEAPONS] = level.initial_map_guardian_weapons;
		}

		level.guardian_quest_timer = level.time + 5000;
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
				int jetpack_debounce_amount = 18;

				if (ent->client->sess.amrpgmode == 2)
				{ // zyk: RPG Mode jetpack skill. Each level decreases fuel debounce
					if (ent->client->pers.rpg_class == 2)
					{ // zyk: Bounty Hunter can have a more efficient jetpack
						jetpack_debounce_amount -= ((ent->client->pers.jetpack_level * 3) + (ent->client->pers.improvements_level * 2));
					}
					else
					{
						jetpack_debounce_amount -= (ent->client->pers.jetpack_level * 3);
					}
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
					if (ent->client->ps.m_iVehicleNum <= 0 && ent->health > 0)
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
				else if ((int) ent->client->ps.origin[0] < -4436 || (int) ent->client->ps.origin[0] > -3276 || (int) ent->client->ps.origin[1] < -21283 || (int) ent->client->ps.origin[1] > -19623)
				{ // zyk: player cant start racing before the countdown timer
					ent->client->pers.race_position = 0;
					trap->SendServerCommand( -1, va("chat \"^3Race System: ^7%s ^7lost for trying to race before it starts!\n\"",ent->client->pers.netname) );

					try_finishing_race();
				}
			}

			ultimate_power_events(ent);

			if (ent->client->sess.amrpgmode == 2 && ent->client->sess.sessionTeam != TEAM_SPECTATOR)
			{ // zyk: RPG Mode skills and quests actions. Must be done if player is not at Spectator Mode
				// zyk: Weapon Upgrades
				if (ent->client->ps.weapon == WP_DISRUPTOR && ent->client->pers.weapons_levels[2] == 2 && ent->client->ps.weaponTime > (weaponData[WP_DISRUPTOR].fireTime * 1.0)/1.4)
				{
					ent->client->ps.weaponTime = (weaponData[WP_DISRUPTOR].fireTime * 1.0)/1.4;
				}

				if (ent->client->ps.weapon == WP_REPEATER && ent->client->pers.weapons_levels[4] == 2 && ent->client->ps.weaponTime > weaponData[WP_REPEATER].altFireTime/2)
				{
					ent->client->ps.weaponTime = weaponData[WP_REPEATER].altFireTime/2;
				}

				// zyk: Monk class has a faster melee fireTime and altFireTime
				if (ent->client->pers.rpg_class == 4 && ent->client->ps.weapon == WP_MELEE)
				{
					if (ent->client->ps.weaponTime > (weaponData[WP_MELEE].fireTime * 1.5)/(ent->client->pers.improvements_level + 1))
					{
						ent->client->ps.weaponTime = (weaponData[WP_MELEE].fireTime * 1.5)/(ent->client->pers.improvements_level + 1);
					}
					else if (ent->client->ps.weaponTime > (weaponData[WP_MELEE].altFireTime * 1.5)/(ent->client->pers.improvements_level + 1))
					{
						ent->client->ps.weaponTime = (weaponData[WP_MELEE].altFireTime * 1.5)/(ent->client->pers.improvements_level + 1);
					}
				}

				if (ent->client->pers.flame_thrower > level.time)
				{ // zyk: fires the flame thrower
					Player_FireFlameThrower(ent);
				}

				if (level.quest_map > 0)
				{ // zyk: control the quest events which happen in the quest maps, if player can play quests now
					if (level.quest_map == 1)
					{
						if (ent->client->pers.universe_quest_progress == 8 && ent->client->pers.can_play_quest == 1 && !(ent->client->pers.universe_quest_counter & (1 << 0)) && ent->client->pers.universe_quest_timer < level.time)
						{ // zyk: first objective of the Second Act of Universe Quest. Sages part
							gentity_t *npc_ent = NULL;

							if (ent->client->pers.universe_quest_messages == 0)
								npc_ent = Zyk_NPC_SpawnType("sage_of_light",2780,4034,1433,0);
							else if (ent->client->pers.universe_quest_messages == 1)
								npc_ent = Zyk_NPC_SpawnType("sage_of_darkness",2780,3904,1433,0);
							else if (ent->client->pers.universe_quest_messages == 2)
								npc_ent = Zyk_NPC_SpawnType("sage_of_eternity",2780,3966,1433,0);
							else if (ent->client->pers.universe_quest_messages == 3)
								npc_ent = Zyk_NPC_SpawnType("sage_of_universe",2780,4096,1433,0);

							if (ent->client->pers.universe_quest_messages == 5)
								trap->SendServerCommand( -1, va("chat \"^2Sage of Universe^7: Welcome again, %s.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 6)
								trap->SendServerCommand( -1, va("chat \"%s^7: Sages. I have questions.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 7)
								trap->SendServerCommand( -1, va("chat \"%s^7: I had a strange dream about a place. I believe it is the sacred t2_trip obelisk.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 8)
								trap->SendServerCommand( -1, va("chat \"^5Sage of Light^7: Yes. It is a sacred place there.\""));
							else if (ent->client->pers.universe_quest_messages == 9)
								trap->SendServerCommand( -1, va("chat \"^1Sage of Darkness^7: Not only that. It is the place where a powerful being is sealed!\""));
							else if (ent->client->pers.universe_quest_messages == 10)
								trap->SendServerCommand( -1, va("chat \"^3Sage of Eternity^7: The Guardian of Time!\""));
							else if (ent->client->pers.universe_quest_messages == 11)
								trap->SendServerCommand( -1, va("chat \"%s^7: Please tell me everything you know about it.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 12)
								trap->SendServerCommand( -1, va("chat \"^2Sage of Universe^7: The Guardian of Time is a very old being.\""));
							else if (ent->client->pers.universe_quest_messages == 13)
								trap->SendServerCommand( -1, va("chat \"^2Sage of Universe^7: She has the power to control Time itself.\""));
							else if (ent->client->pers.universe_quest_messages == 14)
								trap->SendServerCommand( -1, va("chat \"^2Sage of Universe^7: But another very old and powerful being sealed her.\""));
							else if (ent->client->pers.universe_quest_messages == 15)
								trap->SendServerCommand( -1, va("chat \"^2Sage of Universe^7: We are not sure who he is...\""));
							else if (ent->client->pers.universe_quest_messages == 16)
								trap->SendServerCommand( -1, va("chat \"^2Sage of Universe^7: but he is certainly more powerful than the guardians...\""));
							else if (ent->client->pers.universe_quest_messages == 17)
								trap->SendServerCommand( -1, va("chat \"^2Sage of Universe^7: But there are more urgent matters now.\""));
							else if (ent->client->pers.universe_quest_messages == 18)
								trap->SendServerCommand( -1, va("chat \"^2Sage of Universe^7: The Master of Evil is back.\""));
							else if (ent->client->pers.universe_quest_messages == 19)
								trap->SendServerCommand( -1, va("chat \"%s^7: How is that possible? I defeated him.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 20)
								trap->SendServerCommand( -1, va("chat \"^2Sage of Universe^7: When he sealed the Guardian of Universe...\""));
							else if (ent->client->pers.universe_quest_messages == 21)
								trap->SendServerCommand( -1, va("chat \"^2Sage of Universe^7: he absorbed her Resurrection power...\""));
							else if (ent->client->pers.universe_quest_messages == 22)
								trap->SendServerCommand( -1, va("chat \"^2Sage of Universe^7: with it, he was able to revive.\""));
							else if (ent->client->pers.universe_quest_messages == 23)
								trap->SendServerCommand( -1, va("chat \"^2Sage of Universe^7: he has now a larger army with a new kind of enemy.\""));
							else if (ent->client->pers.universe_quest_messages == 24)
								trap->SendServerCommand( -1, va("chat \"^2Sage of Universe^7: Be careful hero.\""));
							else if (ent->client->pers.universe_quest_messages == 25)
								trap->SendServerCommand( -1, va("chat \"^2Sage of Universe^7: we are in difficult times.\""));
							else if (ent->client->pers.universe_quest_messages == 26)
								trap->SendServerCommand( -1, va("chat \"%s^7: I will defeat him. Probably he cannot resurrect again.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 27)
								trap->SendServerCommand( -1, va("chat \"%s^7: Because the Guardian of Universe is no longer sealed.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 28)
								trap->SendServerCommand( -1, va("chat \"^2Sage of Universe^7: Thanks %s^7, but if you are going to do this...\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 29)
								trap->SendServerCommand( -1, va("chat \"^2Sage of Universe^7: Be prepared for very difficult battles against his army.\""));
							else if (ent->client->pers.universe_quest_messages == 30)
							{
								trap->SendServerCommand( -1, va("chat \"^2Sage of Universe^7: Good Luck.\""));

								ent->client->pers.universe_quest_counter |= (1 << 0);
								first_second_act_objective(ent);
								save_account(ent);
								quest_get_new_player(ent);
							}

							if (ent->client->pers.universe_quest_messages < 4)
							{
								ent->client->pers.universe_quest_messages++;
								ent->client->pers.universe_quest_timer = level.time + 1500;
							}
							else if (ent->client->pers.universe_quest_messages > 4)
							{
								ent->client->pers.universe_quest_messages++;
								ent->client->pers.universe_quest_timer = level.time + 5000;
							}
						}

						if (ent->client->pers.hunter_quest_progress != NUMBER_OF_OBJECTIVES && ent->client->pers.guardian_mode == 0 && !(ent->client->pers.hunter_quest_progress & (1 << 4)) && ent->client->pers.can_play_quest == 1 && (int) ent->r.currentOrigin[0] > 2311 && (int) ent->r.currentOrigin[0] < 2458 && (int) ent->r.currentOrigin[1] > 4503 && (int) ent->r.currentOrigin[1] < 4697 && (int) ent->r.currentOrigin[2] == 1832)
						{
							trap->SendServerCommand( -1, "chat \"^3Quest System: ^7Found an ancient note.\"");
							ent->client->pers.hunter_quest_progress |= (1 << 4);
							clean_note_model();
							save_account(ent);
							quest_get_new_player(ent);
						}

						if (ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS && !(ent->client->pers.defeated_guardians & (1 << 4)) && ent->client->pers.can_play_quest == 1 && ent->client->pers.guardian_mode == 0 && (int) ent->r.currentOrigin[0] > 1962 && (int) ent->r.currentOrigin[0] < 2162 && (int) ent->r.currentOrigin[1] > 3989 && (int) ent->r.currentOrigin[1] < 4189 && (int) ent->r.currentOrigin[2] >= 360 && (int) ent->r.currentOrigin[2] <= 369)
						{
							if (ent->client->pers.light_quest_timer < level.time)
							{
								if (ent->client->pers.light_quest_messages == 0)
									trap->SendServerCommand( -1, va("chat \"^4Guardian of Water: ^7I am the Guardian of Water, %s^7... You must now prove yourself...\"",ent->client->pers.netname));
								else if (ent->client->pers.light_quest_messages == 1)
								{
									spawn_boss(ent,2062,4089,361,90,"guardian_boss_1",2062,4189,500,90,1);
								}
								ent->client->pers.light_quest_messages++;
								ent->client->pers.light_quest_timer = level.time + 3000;
							}
						}

						if (ent->client->pers.universe_quest_progress == 3 && ent->client->pers.can_play_quest == 1 && ent->client->pers.universe_quest_objective_control == 4 && ent->client->pers.universe_quest_timer < level.time && (int) ent->client->ps.origin[0] > 2720 && (int) ent->client->ps.origin[0] < 2840 && (int) ent->client->ps.origin[1] > 3944 && (int) ent->client->ps.origin[1] < 3988 && (int) ent->client->ps.origin[2] == 1432)
						{ // zyk: fourth Universe Quest objective, doesnt need player control like the third objective
							if (ent->client->pers.universe_quest_messages == 0)
								trap->SendServerCommand( -1, va("chat \"%s^7: Where are the sages?\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 1)
								trap->SendServerCommand( -1, va("chat \"%s^7: Oh...a note. They must have left it here for me, maybe...\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 2)
								trap->SendServerCommand( -1, va("chat \"%s^7: Ok, let's see what they want from me.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 3)
								trap->SendServerCommand( -1, va("chat \"^3Note^7: %s^7, We wanted to say this before but we couldn't.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 4)
								trap->SendServerCommand( -1, va("chat \"^3Note^7: We left yavin1b to find out what happened to the ^2Sage of Universe^7!\""));
							else if (ent->client->pers.universe_quest_messages == 5)
								trap->SendServerCommand( -1, va("chat \"%s^7: Sage of Universe?\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 6)
								trap->SendServerCommand( -1, va("chat \"^3Note^7: Somehow he got the amulet of Universe, about which we forgot to tell you before.\""));
							else if (ent->client->pers.universe_quest_messages == 7)
								trap->SendServerCommand( -1, va("chat \"^3Note^7: Forgive us for that, please.\""));
							else if (ent->client->pers.universe_quest_messages == 8)
								trap->SendServerCommand( -1, va("chat \"%s^7: So there is a fourth sage and his amulet\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 9)
								trap->SendServerCommand( -1, va("chat \"^3Note^7: We believe that the sage is hidden in t3_hevil, but we are not sure.\""));
							else if (ent->client->pers.universe_quest_messages == 10)
								trap->SendServerCommand( -1, va("chat \"^3Note^7: Find him and he will give you the amulet of Universe.\""));
							else if (ent->client->pers.universe_quest_messages == 11)
								trap->SendServerCommand( -1, va("chat \"^3Note^7: After knowing the sage whereabouts, we went to taspir1 to spy on Master of Evil actions there.\""));
							else if (ent->client->pers.universe_quest_messages == 12)
								trap->SendServerCommand( -1, va("chat \"^3Note^7: We will tell you all of this in a more detailed way later.\""));							
							else if (ent->client->pers.universe_quest_messages == 13)
								trap->SendServerCommand( -1, va("chat \"^3Note^7: Please find all four amulets and come to taspir1.\""));
							else if (ent->client->pers.universe_quest_messages == 14)
								trap->SendServerCommand( -1, va("chat \"^3Note^7: Good luck, %s^7.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 15)
								trap->SendServerCommand( -1, va("chat \"%s^7: Maybe the Sage of Universe has more info about the guardian amulets\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 16)
								trap->SendServerCommand( -1, va("chat \"%s^7: I hope that he is really in ^3t3_hevil^7.\"", ent->client->pers.netname));

							ent->client->pers.universe_quest_messages++;
							ent->client->pers.universe_quest_timer = level.time + 5000;

							if (ent->client->pers.universe_quest_messages == 17)
							{ // zyk: complete the objective
								ent->client->pers.universe_quest_objective_control = -1;
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
									trap->SendServerCommand( -1, va("chat \"%s^7: I sense the presence of an artifact here.\"", ent->client->pers.netname));
								
								npc_ent = Zyk_NPC_SpawnType("sage_of_light",2780,4034,1583,0);
							}
							else if (ent->client->pers.universe_quest_messages == 1)
								npc_ent = Zyk_NPC_SpawnType("sage_of_eternity",2780,3966,1583,0);
							else if (ent->client->pers.universe_quest_messages == 2)
								npc_ent = Zyk_NPC_SpawnType("sage_of_darkness",2780,3904,1583,0);
							else if (ent->client->pers.universe_quest_messages == 4)
							{
								trap->SendServerCommand( -1, "chat \"^5Sage of Light: ^7Finish ^5Light Quest ^7and come back so I can give you an artifact...\"");

								change_player = 1;
							}
							else if (ent->client->pers.universe_quest_messages == 5)
							{
								ent->client->ps.powerups[PW_FORCE_BOON] = level.time + 3000;

								trap->SendServerCommand( -1, va("chat \"^5Sage of Light: ^7Amazing, %s^7! Now receive an artifact...\"",ent->client->pers.netname));
								ent->client->pers.universe_quest_counter |= (1 << 0);
								save_account(ent);

								universe_quest_artifacts_checker(ent);

								change_player = 1;
							}
							else if (ent->client->pers.universe_quest_messages == 6)
								trap->SendServerCommand( -1, "chat \"^5Sage of Light: ^7Your quest is very difficult, but at the end you will see it was worthy the effort...\"");
							else if (ent->client->pers.universe_quest_messages == 7)
							{
								trap->SendServerCommand( -1, "chat \"^3Sage of Eternity: ^7Complete ^3Eternity Quest ^7and come back so I can give you one of the artifacts.\"");

								change_player = 1;
							}
							else if (ent->client->pers.universe_quest_messages == 8)
							{
								ent->client->ps.powerups[PW_FORCE_BOON] = level.time + 3000;

								trap->SendServerCommand( -1, va("chat \"^3Sage of Eternity: ^7Well done, %s^7. Now I can give you one of the artifacts.\"",ent->client->pers.netname));
								ent->client->pers.universe_quest_counter |= (1 << 1);
								save_account(ent);

								universe_quest_artifacts_checker(ent);

								change_player = 1;
							}
							else if (ent->client->pers.universe_quest_messages == 9)
								trap->SendServerCommand( -1, "chat \"^3Sage of Eternity: ^7Never give up. The destiny of the Universe depends upon you.\"");
							else if (ent->client->pers.universe_quest_messages == 10)
							{
								trap->SendServerCommand( -1, "chat \"^1Sage of Darkness: ^7Beat ^1Dark Quest ^7! Only then I will give you my artifact!\"");

								change_player = 1;
							}
							else if (ent->client->pers.universe_quest_messages == 11)
							{
								ent->client->ps.powerups[PW_FORCE_BOON] = level.time + 3000;

								trap->SendServerCommand( -1, va("chat \"^1Sage of Darkness: ^7Not bad, %s^7! Now get this artifact and continue your quest!\"",ent->client->pers.netname));
								ent->client->pers.universe_quest_counter |= (1 << 2);
								save_account(ent);

								universe_quest_artifacts_checker(ent);

								change_player = 1;
							}
							else if (ent->client->pers.universe_quest_messages == 12)
								trap->SendServerCommand( -1, va("chat \"^1Sage of Darkness: ^7You are the legendary hero, %s^7! Go on and defeat the evil guys!\"",ent->client->pers.netname));
							
							if (npc_ent)
							{
								npc_ent->client->pers.universe_quest_objective_control = ent->client->pers.universe_quest_messages;
							}

							if (ent->client->pers.universe_quest_messages == 3 && ent->client->pers.universe_quest_artifact_holder_id == -1 && !(ent->client->pers.universe_quest_counter & (1 << 3)))
							{
								npc_ent = Zyk_NPC_SpawnType("quest_reborn_boss",-396,-287,-150,-153);
								if (npc_ent)
								{ // zyk: spawning the quest_reborn_boss artifact holder
									npc_ent->client->ps.powerups[PW_FORCE_BOON] = level.time + 5500;
									npc_ent->client->pers.universe_quest_artifact_holder_id = ent-g_entities;

									ent->client->pers.universe_quest_artifact_holder_id = npc_ent-g_entities;
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

						if (ent->client->pers.universe_quest_artifact_holder_id == -2 && ent->client->pers.can_play_quest == 1 && ent->client->ps.powerups[PW_FORCE_BOON])
						{ // zyk: player got the artifact, save it to his account
							trap->SendServerCommand( -1, va("chat \"%s^7: This is one of the artifacts!\"", ent->client->pers.netname));
							ent->client->pers.universe_quest_artifact_holder_id = -1;
							ent->client->pers.universe_quest_counter |= (1 << 3);
							save_account(ent);

							universe_quest_artifacts_checker(ent);

							quest_get_new_player(ent);
						}
						else if (ent->client->pers.universe_quest_artifact_holder_id > -1 && ent->client->pers.can_play_quest == 1)
						{ // zyk: setting the force boon time in the artifact holder npc so it doesnt run out
							if (&g_entities[ent->client->pers.universe_quest_artifact_holder_id] && g_entities[ent->client->pers.universe_quest_artifact_holder_id].client && g_entities[ent->client->pers.universe_quest_artifact_holder_id].client->pers.universe_quest_artifact_holder_id != -1 && g_entities[ent->client->pers.universe_quest_artifact_holder_id].client->ps.powerups[PW_FORCE_BOON] < (level.time + 1000))
								g_entities[ent->client->pers.universe_quest_artifact_holder_id].client->ps.powerups[PW_FORCE_BOON] = level.time + 1000;
						}
					}
					else if (level.quest_map == 2)
					{
						if (ent->client->pers.hunter_quest_progress != NUMBER_OF_OBJECTIVES && !(ent->client->pers.hunter_quest_progress & (1 << 5)) && ent->client->pers.can_play_quest == 1 && (int) ent->client->ps.origin[0] > 7447 && (int) ent->client->ps.origin[0] < 7552 && (int) ent->client->ps.origin[1] > -816 && (int) ent->client->ps.origin[1] < -728 && (int) ent->client->ps.origin[2] == 24)
						{
							trap->SendServerCommand( -1, "chat \"^3Quest System: ^7Found an ancient note.\"");
							ent->client->pers.hunter_quest_progress |= (1 << 5);
							clean_note_model();
							save_account(ent);
							quest_get_new_player(ent);
						}
					}
					else if (level.quest_map == 3)
					{
						if (ent->client->pers.hunter_quest_progress != NUMBER_OF_OBJECTIVES && !(ent->client->pers.hunter_quest_progress & (1 << 6)) && ent->client->pers.can_play_quest == 1 && (int) ent->client->ps.origin[0] > -830 && (int) ent->client->ps.origin[0] < -700 && (int) ent->client->ps.origin[1] > 4755 && (int) ent->client->ps.origin[1] < 4839 && (int) ent->client->ps.origin[2] > 215 && (int) ent->client->ps.origin[2] < 222)
						{
							trap->SendServerCommand( -1, "chat \"^3Quest System: ^7Found an ancient note.\"");
							ent->client->pers.hunter_quest_progress |= (1 << 6);
							clean_note_model();
							save_account(ent);
							quest_get_new_player(ent);
						}
					}
					else if (level.quest_map == 4)
					{
						if (ent->client->pers.universe_quest_progress == 9 && ent->client->pers.can_play_quest == 1 && !(ent->client->pers.universe_quest_counter & (1 << 2)) && ent->client->pers.universe_quest_timer < level.time)
						{ // zyk: Crystal of Time
							gentity_t *npc_ent = NULL;

							if (ent->client->pers.universe_quest_messages == 0)
								npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",291,-1110,-487,-136);
							else if (ent->client->pers.universe_quest_messages == 1)
								npc_ent = Zyk_NPC_SpawnType("quest_reborn_boss",221,-1480,-487,150);
							else if (ent->client->pers.universe_quest_messages == 2)
								npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",-240,-1755,-487,58);
							else if (ent->client->pers.universe_quest_messages == 3)
								npc_ent = Zyk_NPC_SpawnType("quest_reborn_boss",-174,-1585,-487,66);
							else if (ent->client->pers.universe_quest_messages == 4)
								npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",50,-1637,-487,100);
							else if (ent->client->pers.universe_quest_messages == 5)
								npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",258,-1612,-487,122);
							else if (ent->client->pers.universe_quest_messages == 6)
								npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",82,-1515,-487,126);
							else if (ent->client->pers.universe_quest_messages == 7)
								npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",262,-1343,-247,-178);
							else if (ent->client->pers.universe_quest_messages == 8)
								npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",173,-1177,-487,-127);
							else if (ent->client->pers.universe_quest_messages == 9)
								npc_ent = Zyk_NPC_SpawnType("quest_reborn_boss",-246,-1771,-103,45);
							else if (ent->client->pers.universe_quest_messages == 10)
							{
								if (level.quest_model_id == -1)
									load_crystal_model(456,-945,-507,-134,2);
							}

							if (ent->client->pers.universe_quest_messages < 11)
							{
								ent->client->pers.universe_quest_messages++;
								ent->client->pers.universe_quest_timer = level.time + 1500;
							}
							else if (ent->client->pers.universe_quest_messages == 11 && (int) ent->client->ps.origin[0] > 441 && (int) ent->client->ps.origin[0] < 471 && (int) ent->client->ps.origin[1] > -960 && (int) ent->client->ps.origin[1] < -930 && (int) ent->client->ps.origin[2] > -507 && (int) ent->client->ps.origin[2] < -467)
							{
								ent->client->pers.universe_quest_messages = 12;
							}
							else if (ent->client->pers.universe_quest_messages > 11)
							{
								ent->client->pers.universe_quest_counter |= (1 << 2);
								trap->SendServerCommand( -1, "chat \"^3Quest System^7: Got the ^2Crystal of Time^7.\"");
								universe_crystals_check(ent);
								save_account(ent);
								clean_crystal_model();
								quest_get_new_player(ent);
							}
						}

						if (ent->client->pers.hunter_quest_progress != NUMBER_OF_OBJECTIVES && !(ent->client->pers.hunter_quest_progress & (1 << 7)) && ent->client->pers.can_play_quest == 1 && (int) ent->client->ps.origin[0] > 2326 && (int) ent->client->ps.origin[0] < 2473 && (int) ent->client->ps.origin[1] > 2966 && (int) ent->client->ps.origin[1] < 3025 && (int) ent->client->ps.origin[2] == -2071)
						{
							trap->SendServerCommand( -1, "chat \"^3Quest System: ^7Found an ancient note.\"");
							ent->client->pers.hunter_quest_progress |= (1 << 7);
							clean_note_model();
							save_account(ent);
							quest_get_new_player(ent);
						}
					}
					else if (level.quest_map == 5)
					{
						if (ent->client->pers.hunter_quest_progress != NUMBER_OF_OBJECTIVES && !(ent->client->pers.hunter_quest_progress & (1 << 8)) && ent->client->pers.can_play_quest == 1 && (int) ent->client->ps.origin[0] > -710 && (int) ent->client->ps.origin[0] < -336 && (int) ent->client->ps.origin[1] > -4857 && (int) ent->client->ps.origin[1] < -4504 && (int) ent->client->ps.origin[2] > 940 && (int) ent->client->ps.origin[2] < 951)
						{
							trap->SendServerCommand( -1, "chat \"^3Quest System: ^7Found an ancient note.\"");
							ent->client->pers.hunter_quest_progress |= (1 << 8);
							clean_note_model();
							save_account(ent);
							quest_get_new_player(ent);
						}
					}
					else if (level.quest_map == 6)
					{
						if (ent->client->pers.hunter_quest_progress != NUMBER_OF_OBJECTIVES && !(ent->client->pers.hunter_quest_progress & (1 << 9)) && ent->client->pers.can_play_quest == 1 && (int) ent->client->ps.origin[0] > -9888 && (int) ent->client->ps.origin[0] < -9788 && (int) ent->client->ps.origin[1] > -1597 && (int) ent->client->ps.origin[1] < -1497 && (int) ent->client->ps.origin[2] == 24)
						{ // zyk: Dark Quest Note
							trap->SendServerCommand( -1, "chat \"^3Quest System: ^7Found an ancient note.\"");
							ent->client->pers.hunter_quest_progress |= (1 << 9);
							clean_note_model();
							save_account(ent);
							quest_get_new_player(ent);
						}

						if (ent->client->pers.universe_quest_progress == 2 && ent->client->pers.can_play_quest == 1 && ent->client->pers.universe_quest_objective_control == 3 && !(ent->client->pers.universe_quest_counter & (1 << 6)) && ent->client->pers.universe_quest_timer < level.time)
						{ // zyk: Universe Quest artifact
							gentity_t *npc_ent = NULL;
							if (ent->client->pers.universe_quest_messages == 0)
							{
								trap->SendServerCommand( -1, va("chat \"%s^7: I sense the presence of an artifact here.\"", ent->client->pers.netname));
								npc_ent = Zyk_NPC_SpawnType("quest_reborn_red",2120,-1744,39,90);
							}
							else if (ent->client->pers.universe_quest_messages == 1)
								npc_ent = Zyk_NPC_SpawnType("quest_reborn_red",2318,-1744,39,90);
							else if (ent->client->pers.universe_quest_messages == 2)
							{
								npc_ent = Zyk_NPC_SpawnType("quest_reborn_boss",2214,-1744,39,90);
								if (npc_ent)
								{
									npc_ent->client->ps.powerups[PW_FORCE_BOON] = level.time + 5500;

									npc_ent->client->pers.universe_quest_artifact_holder_id = ent-g_entities;
									ent->client->pers.universe_quest_artifact_holder_id = npc_ent-g_entities;
								}
							}

							if (ent->client->pers.universe_quest_messages < 3)
							{
								ent->client->pers.universe_quest_messages++;
								ent->client->pers.universe_quest_timer = level.time + 5000;
							}
						}

						if (ent->client->pers.universe_quest_artifact_holder_id == -2 && ent->client->pers.can_play_quest == 1 && ent->client->ps.powerups[PW_FORCE_BOON])
						{ // zyk: player got the artifact, save it to his account
							trap->SendServerCommand( -1, va("chat \"%s^7: This is one of the artifacts!\"", ent->client->pers.netname));
							ent->client->pers.universe_quest_artifact_holder_id = -1;
							ent->client->pers.universe_quest_counter |= (1 << 6);
							save_account(ent);

							universe_quest_artifacts_checker(ent);

							quest_get_new_player(ent);
						}
						else if (ent->client->pers.universe_quest_artifact_holder_id > -1 && ent->client->pers.can_play_quest == 1)
						{ // zyk: setting the force boon time in the artifact holder npc so it doesnt run out
							if (&g_entities[ent->client->pers.universe_quest_artifact_holder_id] && g_entities[ent->client->pers.universe_quest_artifact_holder_id].client && g_entities[ent->client->pers.universe_quest_artifact_holder_id].client->pers.universe_quest_artifact_holder_id != -1 && g_entities[ent->client->pers.universe_quest_artifact_holder_id].client->ps.powerups[PW_FORCE_BOON] < (level.time + 1000))
								g_entities[ent->client->pers.universe_quest_artifact_holder_id].client->ps.powerups[PW_FORCE_BOON] = level.time + 1000;
						}
					}
					else if (level.quest_map == 7)
					{
						if (ent->client->pers.hunter_quest_progress != NUMBER_OF_OBJECTIVES && ent->client->pers.guardian_mode == 0 && !(ent->client->pers.hunter_quest_progress & (1 << 10)) && ent->client->pers.can_play_quest == 1 && (int) ent->client->ps.origin[0] > 1809 && (int) ent->client->ps.origin[0] < 2025 && (int) ent->client->ps.origin[1] > 1082 && (int) ent->client->ps.origin[1] < 1281 && (int) ent->client->ps.origin[2] == 728)
						{
							trap->SendServerCommand( -1, "chat \"^3Quest System: ^7Found an ancient note.\"");
							ent->client->pers.hunter_quest_progress |= (1 << 10);
							clean_note_model();
							save_account(ent);
							quest_get_new_player(ent);
						}

						if (ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS && !(ent->client->pers.defeated_guardians & (1 << 7)) && ent->client->pers.can_play_quest == 1 && ent->client->pers.guardian_mode == 0 && (int) ent->client->ps.origin[0] > 2400 && (int) ent->client->ps.origin[0] < 2600 && (int) ent->client->ps.origin[1] > 2040 && (int) ent->client->ps.origin[1] < 2240 && (int) ent->client->ps.origin[2] == -551)
						{
							if (ent->client->pers.light_quest_timer < level.time)
							{
								if (ent->client->pers.light_quest_messages == 0)
									trap->SendServerCommand( -1, va("chat \"^5Guardian of Intelligence: ^7I am the Guardian of Intelligence, %s^7. Face the power of my advanced mind.\"",ent->client->pers.netname));
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
							{
								trap->SendServerCommand( -1, va("chat \"%s^7: Oh great, Master of Evil sent some of his soldiers here too.\"", ent->client->pers.netname));
								Zyk_NPC_SpawnType("quest_reborn_blue",626,-1004,-742,-90);
							}
							else if (ent->client->pers.universe_quest_messages == 1)
							{
								trap->SendServerCommand( -1, va("chat \"%s^7: I hope I can find the Sage of Universe before them.\"", ent->client->pers.netname));
								Zyk_NPC_SpawnType("quest_reborn_blue",401,-1004,-742,-90);
							}
							else if (ent->client->pers.universe_quest_messages == 2)
								Zyk_NPC_SpawnType("quest_reborn_red",985,-510,1177,-179);
							else if (ent->client->pers.universe_quest_messages == 3)
								Zyk_NPC_SpawnType("quest_reborn_red",53,-510,1177,-179);
							else if (ent->client->pers.universe_quest_messages == 5)
								Zyk_NPC_SpawnType("quest_reborn_boss",512,-315,1177,-90);
							else if (ent->client->pers.universe_quest_messages == 6)
								npc_ent = Zyk_NPC_SpawnType("sage_of_universe",507,-623,537,90);
							else if (ent->client->pers.universe_quest_messages == 8)
								trap->SendServerCommand( -1, va("chat \"^2Sage of Universe^7: Greetings, %s^7. I am the Sage of Universe.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 9)
								trap->SendServerCommand( -1, va("chat \"%s^7: I finally found you!\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 10)
								trap->SendServerCommand( -1, va("chat \"^2Sage of Universe^7: Ok, %s^7. I must tell you some things.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 11)
								trap->SendServerCommand( -1, va("chat \"^2Sage of Universe^7: I will give you the amulet of Universe, but I must ask you a favor.\""));
							else if (ent->client->pers.universe_quest_messages == 12)
								trap->SendServerCommand( -1, va("chat \"%s^7: Thanks! Now what I can do for you.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 13)
								trap->SendServerCommand( -1, va("chat \"^2Sage of Universe^7: You must find the other three amulets in the ^3City of the Merchants^7.\""));
							else if (ent->client->pers.universe_quest_messages == 14)
								trap->SendServerCommand( -1, va("chat \"^2Sage of Universe^7: It is located at ^3mp/siege_desert^7. Now I will go to taspir1 to find the other sages.\""));
							else if (ent->client->pers.universe_quest_messages == 15)
								trap->SendServerCommand( -1, va("chat \"%s^7: Ok! But how did you know about the sages?\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 16)
								trap->SendServerCommand( -1, va("chat \"^2Sage of Universe^7: %s^7...the mysterious voice at the beginning of your quest was mine.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 17)
								trap->SendServerCommand( -1, va("chat \"%s^7: So you knew about me since the beginning!\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 18)
								trap->SendServerCommand( -1, va("chat \"^2Sage of Universe^7: yes %s^7. We will explain everything after you find all amulets.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 19)
								trap->SendServerCommand( -1, va("chat \"^2Sage of Universe^7: After you found them, go to taspir1. We will be waiting for you.\""));
							else if (ent->client->pers.universe_quest_messages == 20)
								trap->SendServerCommand( -1, va("chat \"^2Sage of Universe^7: Now go %s^7! I wish you luck on your quest.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 21)
							{
								trap->SendServerCommand( -1, va("chat \"%s^7: Thanks! Don't worry, I will find them in ^3mp/siege_desert ^7and meet you all in taspir1.\"", ent->client->pers.netname));

								ent->client->pers.universe_quest_counter = 0;
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

							if (ent->client->pers.universe_quest_messages != 7 && ent->client->pers.universe_quest_messages < 21)
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
								int ent_iterator = 0;
								gentity_t *this_ent = NULL;

								// zyk: cleaning entities, except the spawn points. This will prevent server from crashing in this mission
								for (ent_iterator = level.maxclients; ent_iterator < level.num_entities; ent_iterator++)
								{
									this_ent = &g_entities[ent_iterator];

									if (this_ent && this_ent->s.number != level.quest_note_id && Q_stricmp( this_ent->classname, "info_player_deathmatch" ) != 0)
										G_FreeEntity(this_ent);
								}

								trap->SendServerCommand( -1, "chat \"^2Mysterious Voice^7: Go, hero... save the Guardian Sages... they need your help...\"");
								npc_ent = Zyk_NPC_SpawnType("sage_of_light",14027,-795,-3254,90);
							}
							else if (ent->client->pers.universe_quest_messages == 1)
							{
								trap->SendServerCommand( -1, va("chat \"%s^7: This voice...\"", ent->client->pers.netname));
								npc_ent = Zyk_NPC_SpawnType("sage_of_darkness",14190,-795,-3254,90);
							}
							else if (ent->client->pers.universe_quest_messages == 2)
							{
								trap->SendServerCommand( -1, va("chat \"%s^7: I don't understand... maybe I should do as it says...\"", ent->client->pers.netname));
								npc_ent = Zyk_NPC_SpawnType("sage_of_eternity",14111,-795,-3254,90);
							}
							else if (ent->client->pers.universe_quest_messages == 3)
							{
								trap->SendServerCommand( -1, va("chat \"%s^7: I hope they are able to tell me about this strange voice.\"", ent->client->pers.netname));
							}
							else if (ent->client->pers.universe_quest_messages == 5)
							{
								trap->SendServerCommand( -1, "chat \"^5Sage of Light: ^7The enemies are coming!\"");
							}
							else if (ent->client->pers.universe_quest_messages == 6)
								trap->SendServerCommand( -1, "chat \"^1Sage of Darkness: ^7We will fight to the death!\"");
							else if (ent->client->pers.universe_quest_messages == 7)
								trap->SendServerCommand( -1, "chat \"^3Sage of Eternity: ^7Hero... please help us!\"");
							else if (ent->client->pers.universe_quest_messages == 8)
							{
								if (ent->client->pers.universe_quest_objective_control > 15)
									npc_ent = Zyk_NPC_SpawnType("quest_reborn",14027,-673,-3134,-90);
								else if (ent->client->pers.universe_quest_objective_control > 9)
									npc_ent = Zyk_NPC_SpawnType("quest_reborn_blue",14190,-673,-3134,-90);
								else if (ent->client->pers.universe_quest_objective_control > 1)
									npc_ent = Zyk_NPC_SpawnType("quest_reborn_red",14111,-673,-3134,-90);
								else
								{
									ent->client->pers.universe_quest_messages = 9;
									npc_ent = Zyk_NPC_SpawnType("quest_reborn_boss",14111,-550,-3134,-90);
								}
							}
							else if (ent->client->pers.universe_quest_messages == 10)
							{
								trap->SendServerCommand( -1, "chat \"^3Sage of Eternity: ^7Thank you, brave warrior! You saved our lives.\"");
							}
							else if (ent->client->pers.universe_quest_messages == 11)
							{
								ent->client->pers.universe_quest_progress = 1;
								save_account(ent);
								quest_get_new_player(ent);
							}

							if (ent->client->pers.universe_quest_progress == 0)
							{
								if (npc_ent)
								{ // zyk: sets the player id who must kill this quest reborn, or protect this sage
									npc_ent->client->pers.universe_quest_objective_control = ent-g_entities;
								}

								if (ent->client->pers.universe_quest_messages != 4 && ent->client->pers.universe_quest_messages != 8 && ent->client->pers.universe_quest_messages != 9)
									ent->client->pers.universe_quest_messages++;

								ent->client->pers.universe_quest_timer = level.time + 3000;
							}
						}
						else if (ent->client->pers.universe_quest_progress == 1 && ent->client->pers.can_play_quest == 1 && ent->client->pers.universe_quest_timer < level.time && ent->client->pers.universe_quest_messages < 42 && ent->client->pers.universe_quest_objective_control > -1)
						{ // zyk: second Universe Quest mission
							gentity_t *npc_ent = NULL;
							if (ent->client->pers.universe_quest_messages == 0)
								npc_ent = Zyk_NPC_SpawnType("sage_of_light",14027,-795,-3254,90);
							else if (ent->client->pers.universe_quest_messages == 1)
								npc_ent = Zyk_NPC_SpawnType("sage_of_eternity",14111,-795,-3254,90);
							else if (ent->client->pers.universe_quest_messages == 2)
								npc_ent = Zyk_NPC_SpawnType("sage_of_darkness",14190,-795,-3254,90);
							else if (ent->client->pers.universe_quest_messages == 4)
								trap->SendServerCommand( -1, va("chat \"^3Sage of Eternity: ^7Welcome, hero %s^7!\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 5)
								trap->SendServerCommand( -1, va("chat \"%s^7: Hero? What are you talking about?\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 6)
								trap->SendServerCommand( -1, va("chat \"^3Sage of Eternity: ^7%s...^7you are the legendary hero.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 7)
								trap->SendServerCommand( -1, va("chat \"^3Sage of Eternity: ^7Legend says that a hero with great power would save the Guardian Sages and fight against the Master of Evil.\""));
							else if (ent->client->pers.universe_quest_messages == 8)
								trap->SendServerCommand( -1, va("chat \"%s^7: Maybe...but i cant be this legendary hero...\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 9)
								trap->SendServerCommand( -1, va("chat \"^3Sage of Eternity: ^7You are! You saved us, the Brotherhood of Guardian Sages.\""));
							else if (ent->client->pers.universe_quest_messages == 10)
								trap->SendServerCommand( -1, va("chat \"^5Sage of Light: ^7Indeed...now allow us to introduce ourselves.\""));
							else if (ent->client->pers.universe_quest_messages == 11)
								trap->SendServerCommand( -1, va("chat \"^5Sage of Light: ^7I am the Sage of Light...I am the follower of the Guardian of Light.\""));
							else if (ent->client->pers.universe_quest_messages == 12)
								trap->SendServerCommand( -1, va("chat \"^1Sage of Darkness: ^7I am the Sage of Darkness...I am the follower of the Guardian of Darkness.\""));
							else if (ent->client->pers.universe_quest_messages == 13)
								trap->SendServerCommand( -1, va("chat \"^3Sage of Eternity: ^7I am the Sage of Eternity...I am the follower of the Guardian of Eternity.\""));
							else if (ent->client->pers.universe_quest_messages == 14)
								trap->SendServerCommand( -1, va("chat \"^5Sage of Light: ^7Together, we keep the knowledge about the balance of life in the Guardian's power.\""));
							else if (ent->client->pers.universe_quest_messages == 15)
								trap->SendServerCommand( -1, va("chat \"^1Sage of Darkness: ^7With our strength... we should protect the three guardian amulets.\""));
							else if (ent->client->pers.universe_quest_messages == 16)
								trap->SendServerCommand( -1, va("chat \"%s^7: Guardian amulets?\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 17)
								trap->SendServerCommand( -1, va("chat \"^3Sage of Eternity: ^7They have the essence of the guardians power.\""));
							else if (ent->client->pers.universe_quest_messages == 18)
								trap->SendServerCommand( -1, va("chat \"^3Sage of Eternity: ^7But we couldnt stand against the rise of an evil force.\""));
							else if (ent->client->pers.universe_quest_messages == 19)
								trap->SendServerCommand( -1, va("chat \"^5Sage of Light: ^7We had an apprentice called Thor.\""));
							else if (ent->client->pers.universe_quest_messages == 20)
								trap->SendServerCommand( -1, va("chat \"^1Sage of Darkness: ^7He was strong but had an evil mind.\""));
							else if (ent->client->pers.universe_quest_messages == 21)
								trap->SendServerCommand( -1, va("chat \"^3Sage of Eternity: ^7We had no other choice but to ask him to leave the Brotherhood...\""));
							else if (ent->client->pers.universe_quest_messages == 22)
								trap->SendServerCommand( -1, va("chat \"^3Sage of Eternity: ^7He then...fought us...and we couldnt stand against his power.\""));
							else if (ent->client->pers.universe_quest_messages == 23)
								trap->SendServerCommand( -1, va("chat \"^3Sage of Eternity: ^7Instead of killing us, he stunned us to get the guardian amulets.\""));
							else if (ent->client->pers.universe_quest_messages == 24)
								trap->SendServerCommand( -1, va("chat \"^3Sage of Eternity: ^7Then, he went in a quest to seal the most powerful guardian. The Guardian of Universe!\""));
							else if (ent->client->pers.universe_quest_messages == 25)
								trap->SendServerCommand( -1, va("chat \"^3Sage of Eternity: ^7He succeeded. He is now immortal and became the Master of Evil!\""));
							else if (ent->client->pers.universe_quest_messages == 26)
								trap->SendServerCommand( -1, va("chat \"^3Sage of Eternity: ^7To achieve immortality, he divided his life force in 10 artifacts.\""));
							else if (ent->client->pers.universe_quest_messages == 27)
								trap->SendServerCommand( -1, va("chat \"^3Sage of Eternity: ^7They are hidden in different locations and protected by his soldiers.\""));
							else if (ent->client->pers.universe_quest_messages == 28)
								trap->SendServerCommand( -1, va("chat \"^5Sage of Light: ^7You must find the artifacts and the guardian amulets.\""));
							else if (ent->client->pers.universe_quest_messages == 29)
								trap->SendServerCommand( -1, va("chat \"^1Sage of Darkness: ^7Then take them to taspir1, where Thor resides now.\""));
							else if (ent->client->pers.universe_quest_messages == 30)
								trap->SendServerCommand( -1, va("chat \"^3Sage of Eternity: ^7You cannot refuse it! We really need your help.\""));
							else if (ent->client->pers.universe_quest_messages == 31)
								trap->SendServerCommand( -1, va("chat \"^3Sage of Eternity: ^7The fate of the Universe is at your hands!\""));							
							else if (ent->client->pers.universe_quest_messages == 32)
								trap->SendServerCommand( -1, va("chat \"%s^7: Ok then. I accept it. I will find the artifacts and the amulets.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 33)
								trap->SendServerCommand( -1, va("chat \"^3Sage of Eternity: ^7Thank you, brave hero!\""));
							else if (ent->client->pers.universe_quest_messages == 34)
								trap->SendServerCommand( -1, va("chat \"^3Sage of Eternity: ^7You can visit us at yavin1b.\""));
							else if (ent->client->pers.universe_quest_messages == 35)
								trap->SendServerCommand( -1, va("chat \"^3Sage of Eternity: ^7We have 3 of the artifacts, but we will give you with a condition.\""));
							else if (ent->client->pers.universe_quest_messages == 36)
								trap->SendServerCommand( -1, va("chat \"^5Sage of Light: ^7I will give my artifact after you finish the Light Quest.\""));
							else if (ent->client->pers.universe_quest_messages == 37)
								trap->SendServerCommand( -1, va("chat \"^1Sage of Darkness: ^7I will give my artifact after you finish the Dark Quest.\""));
							else if (ent->client->pers.universe_quest_messages == 38)
								trap->SendServerCommand( -1, va("chat \"^3Sage of Eternity: ^7I will give my artifact after you finish the Eternity Quest.\""));
							else if (ent->client->pers.universe_quest_messages == 39)
								trap->SendServerCommand( -1, va("chat \"^3Sage of Eternity: ^7These quests will give you some of the guardians powers.\""));
							else if (ent->client->pers.universe_quest_messages == 40)
								trap->SendServerCommand( -1, va("chat \"^3Sage of Eternity: ^7You will need them to defeat the Master of Evil.\""));
							else if (ent->client->pers.universe_quest_messages == 41)
							{
								trap->SendServerCommand( -1, va("chat \"^3Sage of Eternity: ^7Now go, legendary hero! And good luck!\""));
								
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
								ent->client->pers.universe_quest_timer = level.time + 5000;
							}
							else if (ent->client->pers.universe_quest_messages >= 4)
							{ // zyk: universe_quest_messages will be 4 or higher when player reaches and press USE key on one of the sages
								ent->client->pers.universe_quest_messages++;
								ent->client->pers.universe_quest_timer = level.time + 5000;
							}
						}

						if (ent->client->pers.hunter_quest_progress != NUMBER_OF_OBJECTIVES && !(ent->client->pers.hunter_quest_progress & (1 << 12)) && ent->client->pers.can_play_quest == 1 && (int) ent->client->ps.origin[0] > 14012 && (int) ent->client->ps.origin[0] < 14181 && (int) ent->client->ps.origin[1] > -1641 && (int) ent->client->ps.origin[1] < -1528 && (int) ent->client->ps.origin[2] == -3143)
						{
							trap->SendServerCommand( -1, "chat \"^3Quest System: ^7Found an ancient note.\"");
							ent->client->pers.hunter_quest_progress |= (1 << 12);
							clean_note_model();
							save_account(ent);
							quest_get_new_player(ent);
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
									trap->SendServerCommand( -1, "chat \"^5Guardian of Light: ^7Well done, brave warrior!\"");
								else if (ent->client->pers.light_quest_messages == 1)
									trap->SendServerCommand( -1, "chat \"^5Guardian of Light: ^7Now I shall test your skills...\"");
								else if (ent->client->pers.light_quest_messages == 2)
									trap->SendServerCommand( -1, "chat \"^5Guardian of Light: ^7Defeat me and I will grant you the Light Power!\"");
								else if (ent->client->pers.light_quest_messages == 3)
								{
									spawn_boss(ent,-992,-1802,25,90,va("guardian_boss_%d",NUMBER_OF_GUARDIANS),0,0,0,0,8);
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
									trap->SendServerCommand( -1, "chat \"^1Guardian of Darkness: ^7Excelent, mighty warrior...\"");
								else if (ent->client->pers.hunter_quest_messages == 1)
									trap->SendServerCommand( -1, "chat \"^1Guardian of Darkness: ^7Now your might will be tested...\"");
								else if (ent->client->pers.hunter_quest_messages == 2)
									trap->SendServerCommand( -1, "chat \"^1Guardian of Darkness: ^7Defeat me and I shall grant you the Dark Power...\"");
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
									trap->SendServerCommand( -1, va("chat \"^2Guardian of Forest: ^7I am the Guardian of Forest, %s^7! You cant overcome the power of trees!\"",ent->client->pers.netname));
								else if (ent->client->pers.light_quest_messages == 1)
								{
									spawn_boss(ent,119,4819,33,0,"guardian_boss_3",512,4829,62,179,3);
								}
								ent->client->pers.light_quest_messages++;
								ent->client->pers.light_quest_timer = level.time + 3000;
							}
						}

						if (ent->client->pers.eternity_quest_progress < NUMBER_OF_ETERNITY_QUEST_OBJECTIVES && ent->client->pers.eternity_quest_timer < level.time && ent->client->pers.can_play_quest == 1 && ent->client->pers.guardian_mode == 0 && (int) ent->client->ps.origin[0] > -566 && (int) ent->client->ps.origin[0] < -406 && (int) ent->client->ps.origin[1] > 1393 && (int) ent->client->ps.origin[1] < 1550 && (int) ent->client->ps.origin[2] == 88)
						{
							if (ent->client->pers.eternity_quest_progress == 0)
							{
								trap->SendServerCommand( -1, "chat \"^3Riddle of Earth: ^7It opens the locked ways, by a turn of itself... it reveals the passageways, or can be guarded on the shelf...\"");
							}
							else if (ent->client->pers.eternity_quest_progress == 1)
							{
								trap->SendServerCommand( -1, "chat \"^1Riddle of Fire: ^7It can measure time, when a tick is done... it's an useful item, and is also a nice one...\"");
							}
							else if (ent->client->pers.eternity_quest_progress == 2)
							{
								trap->SendServerCommand( -1, "chat \"^1Riddle of Darkness: ^7Its hit can hurt a person, with the power of the blade...it can kill and destroy, making someone's life fade...\"");
							}
							else if (ent->client->pers.eternity_quest_progress == 3)
							{
								trap->SendServerCommand( -1, "chat \"^4Riddle of Water: ^7Its size is immense, and having its energy is a must... it keeps life on Earth, on its power we can trust...\"");
							}
							else if (ent->client->pers.eternity_quest_progress == 4)
							{
								trap->SendServerCommand( -1, "chat \"^7Riddle of Wind: ^7One can feel warm, with the power of its energy... its power can also be evil, burning to ashes all the harmony...\"");
							}
							else if (ent->client->pers.eternity_quest_progress == 5)
							{
								trap->SendServerCommand( -1, "chat \"^5Riddle of Light: ^7It has a pure essence, it can create life... but it can also be furious, like a sharp cut of a knife...\"");
							}
							else if (ent->client->pers.eternity_quest_progress == 6)
							{
								trap->SendServerCommand( -1, "chat \"^6Riddle of Agility: ^7From the beginning of everything, to the end of everyone... it's there in all that exists, and from it escapes no one...\"");
							}
							else if (ent->client->pers.eternity_quest_progress == 7)
							{
								trap->SendServerCommand( -1, "chat \"^2Riddle of Forest: ^7It has a lot of energy, to sustain a celestial object... it has a shining light, with another of its kind it can connect...\"");
							}		
							else if (ent->client->pers.eternity_quest_progress == 8)
							{
								trap->SendServerCommand( -1, "chat \"^5Riddle of Intelligence: ^7It represents natural life, it's in animals and trees... in water and mountains, in these that someone sees...\"");
							}
							else if (ent->client->pers.eternity_quest_progress == 9)
							{
								trap->SendServerCommand( -1, "chat \"^3Riddle of Eternity: ^7The pure feeling of affection, even the evil ones can sustain... if one can feel and share, his life will not be in vain...\"");
							}
							else if (ent->client->pers.eternity_quest_progress == (NUMBER_OF_ETERNITY_QUEST_OBJECTIVES - 1))
							{
								if (ent->client->pers.eternity_quest_timer == 0)
								{
									trap->SendServerCommand( -1, "chat \"^3Guardian of Eternity: ^7You answered all my riddles. Now prove that you are worthy of my power.\"");
									ent->client->pers.eternity_quest_timer = level.time + 3000;
								}
								else
								{ // zyk: Guardian of Eternity battle
									spawn_boss(ent,-994,2975,25,90,"guardian_of_eternity",0,0,0,0,10);
								}
							}

							if (ent->client->pers.eternity_quest_progress < (NUMBER_OF_ETERNITY_QUEST_OBJECTIVES - 1))
							{
								ent->client->pers.eternity_quest_timer = level.time + 45000;
							}
						}

						if (ent->client->pers.universe_quest_progress == 2 && ent->client->pers.can_play_quest == 1 && ent->client->pers.guardian_mode == 0 && ent->client->pers.universe_quest_objective_control == 3 && ent->client->pers.universe_quest_timer < level.time && ent->client->pers.universe_quest_messages < 8 && !(ent->client->pers.universe_quest_counter & (1 << 8)))
						{
							gentity_t *npc_ent = NULL;
							if (ent->client->pers.universe_quest_messages == 0)
								trap->SendServerCommand( -1, va("chat \"%s^7: I sense the presence of an artifact here.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 1)
								trap->SendServerCommand( -1, va("chat \"^3Spooky voice^7: Indeed you do, %s^7...\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 2)
								trap->SendServerCommand( -1, va("chat \"%s^7: What is going on here?\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 3)
								trap->SendServerCommand( -1, va("chat \"^3Spooky voice^7: I know you are the legendary hero...I defeated the protector of the artifact here, he was serving the Master of Evil...\""));
							else if (ent->client->pers.universe_quest_messages == 4)
								trap->SendServerCommand( -1, va("chat \"%s^7: Well... could you give me the artifact pls?\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 5)
								trap->SendServerCommand( -1, va("chat \"^3Spooky voice^7: Find me, %s^7, and I will give you the artifact you seek...\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 6)
								trap->SendServerCommand( -1, va("chat \"%s^7: I can't believe it... :/\"", ent->client->pers.netname));
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

							trap->SendServerCommand( -1, va("chat \"%s^7: Thanks for the artifact! :)\"", ent->client->pers.netname));

							universe_quest_artifacts_checker(ent);

							quest_get_new_player(ent);
						}
						else if (ent->client->pers.universe_quest_artifact_holder_id > -1 && ent->client->pers.can_play_quest == 1 && ent->client->pers.guardian_mode == 0 && ent->client->ps.hasLookTarget == qtrue && ent->client->ps.lookTarget == ent->client->pers.universe_quest_artifact_holder_id)
						{ // zyk: found quest_ragnos npc, set artifact effect on player
							ent->client->ps.powerups[PW_FORCE_BOON] = level.time + 10000;
							ent->client->pers.universe_quest_timer = level.time + 5000;

							G_FreeEntity(&g_entities[ent->client->pers.universe_quest_artifact_holder_id]);

							ent->client->pers.universe_quest_artifact_holder_id = -3;

							trap->SendServerCommand( -1, va("chat \"^3Spooky voice^7: Nicely done...now, hero %s^7, receive this artifact...\"",ent->client->pers.netname));
						}
					}
					else if (level.quest_map == 11)
					{   
						if (ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS && !(ent->client->pers.defeated_guardians & (1 << 9)) && ent->client->pers.can_play_quest == 1 && ent->client->pers.guardian_mode == 0 && (int) ent->client->ps.origin[0] > 2642 && (int) ent->client->ps.origin[0] < 2842 && (int) ent->client->ps.origin[1] > -125 && (int) ent->client->ps.origin[1] < 75 && (int) ent->client->ps.origin[2] >= -3815 && (int) ent->client->ps.origin[2] <= -3807)
						{
							if (ent->client->pers.light_quest_timer < level.time)
							{
								if (ent->client->pers.light_quest_messages == 0)
									trap->SendServerCommand( -1, va("chat \"^1Guardian of Fire: ^7I am the Guardian of Fire, %s^7! Now you will feel my fire burning!\"",ent->client->pers.netname));
								else if (ent->client->pers.light_quest_messages == 1)
								{
									spawn_boss(ent,2742,-25,-3808,0,"guardian_boss_6",3059,-25,-3808,179,6);
								}
								ent->client->pers.light_quest_messages++;
								ent->client->pers.light_quest_timer = level.time + 3000;
							}
						}
					}
					else if (level.quest_map == 12)
					{
						if (ent->client->pers.universe_quest_progress == 8 && ent->client->pers.can_play_quest == 1 && !(ent->client->pers.universe_quest_counter & (1 << 1)) && ent->client->pers.universe_quest_timer < level.time)
						{ // zyk: eighth mission of Universe Quest. Guardians part
							gentity_t *npc_ent = NULL;

							// zyk: killing other npcs that may be near the guardians
							if (ent->client->pers.universe_quest_messages == 0)
								zyk_NPC_Kill_f("all");

							if (ent->client->pers.universe_quest_messages == 0)
								npc_ent = Zyk_NPC_SpawnType("guardian_boss_9",-1096,13190,-358,0);
							else if (ent->client->pers.universe_quest_messages == 1)
								npc_ent = Zyk_NPC_SpawnType("guardian_of_darkness",-896,13190,-358,179);
							else if (ent->client->pers.universe_quest_messages == 2)
								npc_ent = Zyk_NPC_SpawnType("guardian_of_eternity",-996,13290,-358,-90);
							else if (ent->client->pers.universe_quest_messages == 3)
								npc_ent = Zyk_NPC_SpawnType("guardian_of_universe",-996,13090,-358,90);

							if (npc_ent)
							{
								npc_ent->client->playerTeam = NPCTEAM_PLAYER;
								npc_ent->client->enemyTeam = NPCTEAM_ENEMY;
							}

							if (ent->client->pers.universe_quest_messages == 5)
								trap->SendServerCommand( -1, va("chat \"^2Guardian of Universe^7: Welcome, %s.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 6)
								trap->SendServerCommand( -1, va("chat \"%s^7: Guardians. I had a strange dream.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 7)
								trap->SendServerCommand( -1, va("chat \"%s^7: It is about a place which I believe it is the sacred t2_trip obelisk.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 8)
								trap->SendServerCommand( -1, va("chat \"^5Guardian of Light^7: Interesting. The place you are talking about...\""));
							else if (ent->client->pers.universe_quest_messages == 9)
								trap->SendServerCommand( -1, va("chat \"^1Guardian of Darkness^7: Is the Guardian of Time seal place!\""));
							else if (ent->client->pers.universe_quest_messages == 10)
								trap->SendServerCommand( -1, va("chat \"^3Guardian of Eternity^7: Then the time has come!\""));
							else if (ent->client->pers.universe_quest_messages == 11)
								trap->SendServerCommand( -1, va("chat \"%s^7: Please tell me everything you know about it.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 12)
								trap->SendServerCommand( -1, va("chat \"^2Guardian of Universe^7: There is an old prophecy called the Prophecy of Time.\""));
							else if (ent->client->pers.universe_quest_messages == 13)
								trap->SendServerCommand( -1, va("chat \"^2Guardian of Universe^7: It says the legendary hero will face...\""));
							else if (ent->client->pers.universe_quest_messages == 14)
								trap->SendServerCommand( -1, va("chat \"^2Guardian of Universe^7: The most evil being ever.\""));
							else if (ent->client->pers.universe_quest_messages == 15)
								trap->SendServerCommand( -1, va("chat \"^2Guardian of Universe^7: He is sealed in a place called the Sacred Dimension...\""));
							else if (ent->client->pers.universe_quest_messages == 16)
								trap->SendServerCommand( -1, va("chat \"^2Guardian of Universe^7: The only way to defeat him is going there.\""));
							else if (ent->client->pers.universe_quest_messages == 17)
								trap->SendServerCommand( -1, va("chat \"^2Guardian of Universe^7: But be aware that the Master of Evil is back.\""));
							else if (ent->client->pers.universe_quest_messages == 18)
								trap->SendServerCommand( -1, va("chat \"^2Guardian of Universe^7: He used my Resurrection Power to come back.\""));
							else if (ent->client->pers.universe_quest_messages == 19)
								trap->SendServerCommand( -1, va("chat \"%s^7: So I did not defeat him at all.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 20)
								trap->SendServerCommand( -1, va("chat \"^2Guardian of Universe^7: He is trying to find the sacred crystals.\""));
							else if (ent->client->pers.universe_quest_messages == 21)
								trap->SendServerCommand( -1, va("chat \"^2Guardian of Universe^7: these 3 crystals are a key to open the gate to the Sacred Dimension.\""));
							else if (ent->client->pers.universe_quest_messages == 22)
								trap->SendServerCommand( -1, va("chat \"^2Guardian of Universe^7: He wants to become a guardian to have the guardian powers.\""));
							else if (ent->client->pers.universe_quest_messages == 23)
								trap->SendServerCommand( -1, va("chat \"^2Guardian of Universe^7: You must find the crystals before him.\""));
							else if (ent->client->pers.universe_quest_messages == 24)
								trap->SendServerCommand( -1, va("chat \"%s^7: Yes. I will find the crystals. Thank you for the information.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 25)
							{
								trap->SendServerCommand( -1, va("chat \"^2Guardian of Universe^7: Thank you hero, now go.\""));

								ent->client->pers.universe_quest_counter |= (1 << 1);
								first_second_act_objective(ent);
								save_account(ent);
								quest_get_new_player(ent);
							}

							if (ent->client->pers.universe_quest_messages < 4)
							{
								ent->client->pers.universe_quest_messages++;
								ent->client->pers.universe_quest_timer = level.time + 1500;
							}
							else if (ent->client->pers.universe_quest_messages > 4)
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
									trap->SendServerCommand( -1, va("chat \"^2Guardian of Universe: ^7Welcome, legendary hero %s^7! You finally reached my challenge.\"",ent->client->pers.netname));
								else if (ent->client->pers.universe_quest_messages == 1)
									trap->SendServerCommand( -1, "chat \"^2Guardian of Universe: ^7This battle will grant you the Universe Power if you defeat me.\"");
								else if (ent->client->pers.universe_quest_messages == 2)
									trap->SendServerCommand( -1, va("chat \"^2Guardian of Universe: ^7Get ready, %s^7!\"",ent->client->pers.netname));
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
								trap->SendServerCommand( -1, va("chat \"^2Guardian of Universe: ^7Very well done, %s^7!\"",ent->client->pers.netname));
							}
							else if (ent->client->pers.universe_quest_messages == 6)
							{
								trap->SendServerCommand( -1, va("chat \"^2Guardian of Universe: ^7With your strength, you defeated the guardians of Light Quest.\""));
							}
							else if (ent->client->pers.universe_quest_messages == 7)
							{
								trap->SendServerCommand( -1, va("chat \"^2Guardian of Universe: ^7With your perception, you found all notes of Dark Quest.\""));
							}
							else if (ent->client->pers.universe_quest_messages == 8)
							{
								trap->SendServerCommand( -1, va("chat \"^2Guardian of Universe: ^7With your wisdom, you solved the riddles of Eternity Quest.\""));
							}
							else if (ent->client->pers.universe_quest_messages == 9)
							{
								trap->SendServerCommand( -1, va("chat \"^2Guardian of Universe: ^7With your courage and hope, you defeated the Master of Evil and brought balance to the Universe once again.\""));
							}
							else if (ent->client->pers.universe_quest_messages == 10)
							{
								trap->SendServerCommand( -1, va("chat \"^2Guardian of Universe: ^7Now you will have the full strength of Universe Power.\""));
							}
							else if (ent->client->pers.universe_quest_messages == 11)
							{
								trap->SendServerCommand( -1, va("chat \"^2Guardian of Universe: ^7Good. Now farewell... brave hero! May the power of the guardians guide you in your journey!\""));
								
								ent->client->pers.universe_quest_progress = 8;
								ent->client->pers.universe_quest_messages = 0;

								save_account(ent);

								set_max_health(ent);
								set_max_shield(ent);

								quest_get_new_player(ent);
							}

							if (ent->client->pers.universe_quest_messages > 4 && ent->client->pers.universe_quest_messages < 11)
							{
								ent->client->pers.universe_quest_messages++;
								ent->client->pers.universe_quest_timer = level.time + 5000;
							}
						}
					}
					else if (level.quest_map == 13)
					{
						if (ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS && !(ent->client->pers.defeated_guardians & (1 << 5)) && ent->client->pers.can_play_quest == 1 && ent->client->pers.guardian_mode == 0 && (int) ent->client->ps.origin[0] > -480 && (int) ent->client->ps.origin[0] < -280 && (int) ent->client->ps.origin[1] > 1478 && (int) ent->client->ps.origin[1] < 1678 && (int) ent->client->ps.origin[2] == 4760)
						{
							if (ent->client->pers.light_quest_timer < level.time)
							{
								if (ent->client->pers.light_quest_messages == 0)
									trap->SendServerCommand( -1, va("chat \"^3Guardian of Earth: ^7I am the Guardian of Earth, %s^7! Try to defeat my strength and power!\"",ent->client->pers.netname));
								else if (ent->client->pers.light_quest_messages == 1)
								{
									spawn_boss(ent,-380,1578,4761,-90,"guardian_boss_2",-380,1335,4761,90,2);
								}
								ent->client->pers.light_quest_messages++;
								ent->client->pers.light_quest_timer = level.time + 3000;
							}
						}

						if (ent->client->pers.universe_quest_progress == 2 && ent->client->pers.can_play_quest == 1 && ent->client->pers.universe_quest_objective_control == 3 && ent->client->pers.universe_quest_timer < level.time && ent->client->pers.universe_quest_messages < 1 && !(ent->client->pers.universe_quest_counter & (1 << 5)))
						{
							gentity_t *npc_ent = NULL;
							npc_ent = Zyk_NPC_SpawnType("quest_reborn_boss",-584,296,5977,0);
							if (npc_ent)
							{
								npc_ent->client->ps.powerups[PW_FORCE_BOON] = level.time + 5500;

								npc_ent->client->pers.universe_quest_artifact_holder_id = ent-g_entities;
								ent->client->pers.universe_quest_artifact_holder_id = npc_ent-g_entities;
							}
							trap->SendServerCommand( -1, va("chat \"%s^7: I sense the presence of an artifact here.\"", ent->client->pers.netname));
						
							ent->client->pers.universe_quest_messages++;
							ent->client->pers.universe_quest_timer = level.time + 5000;
						}

						if (ent->client->pers.universe_quest_artifact_holder_id == -2 && ent->client->pers.can_play_quest == 1 && ent->client->ps.powerups[PW_FORCE_BOON])
						{ // zyk: player got the artifact, save it to his account
							trap->SendServerCommand( -1, va("chat \"%s^7: This is one of the artifacts!\"", ent->client->pers.netname));
							ent->client->pers.universe_quest_artifact_holder_id = -1;
							ent->client->pers.universe_quest_counter |= (1 << 5);
							save_account(ent);

							universe_quest_artifacts_checker(ent);

							quest_get_new_player(ent);
						}
						else if (ent->client->pers.universe_quest_artifact_holder_id > -1 && ent->client->pers.can_play_quest == 1)
						{ // zyk: setting the force boon time in the artifact holder npc so it doesnt run out
							if (&g_entities[ent->client->pers.universe_quest_artifact_holder_id] && g_entities[ent->client->pers.universe_quest_artifact_holder_id].client && g_entities[ent->client->pers.universe_quest_artifact_holder_id].client->pers.universe_quest_artifact_holder_id != -1 && g_entities[ent->client->pers.universe_quest_artifact_holder_id].client->ps.powerups[PW_FORCE_BOON] < (level.time + 1000))
								g_entities[ent->client->pers.universe_quest_artifact_holder_id].client->ps.powerups[PW_FORCE_BOON] = level.time + 1000;
						}
					}
					else if (level.quest_map == 14)
					{
						if (ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS && !(ent->client->pers.defeated_guardians & (1 << 11)) && ent->client->pers.can_play_quest == 1 && ent->client->pers.guardian_mode == 0 && (int) ent->client->ps.origin[0] > -100 && (int) ent->client->ps.origin[0] < 100 && (int) ent->client->ps.origin[1] > 1035 && (int) ent->client->ps.origin[1] < 1235 && (int) ent->client->ps.origin[2] == 24)
						{
							if (ent->client->pers.light_quest_timer < level.time)
							{
								if (ent->client->pers.light_quest_messages == 0)
									trap->SendServerCommand( -1, va("chat \"^3Guardian of Resistance: ^7I am the Guardian of Resistance, %s^7! Your attacks are powerless!\"",ent->client->pers.netname));
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
						if (ent->client->pers.defeated_guardians != NUMBER_OF_GUARDIANS && !(ent->client->pers.defeated_guardians & (1 << 10)) && ent->client->pers.can_play_quest == 1 && ent->client->pers.guardian_mode == 0 && (int) ent->client->ps.origin[0] > 5125 && (int) ent->client->ps.origin[0] < 5325 && (int) ent->client->ps.origin[1] > 362 && (int) ent->client->ps.origin[1] < 562 && (int) ent->client->ps.origin[2] == 664)
						{
							if (ent->client->pers.light_quest_timer < level.time)
							{
								if (ent->client->pers.light_quest_messages == 0)
									trap->SendServerCommand( -1, va("chat \"^7Guardian of Wind: ^7I am the Guardian of Wind, %s^7! Try to reach me in the air if you can!\"",ent->client->pers.netname));
								else if (ent->client->pers.light_quest_messages == 1)
								{
									spawn_boss(ent,5225,462,665,179,"guardian_boss_7",5025,462,865,179,7);
								}
								ent->client->pers.light_quest_messages++;
								ent->client->pers.light_quest_timer = level.time + 3000;
							}
						}
					}
					else if (level.quest_map == 17)
					{
						if (ent->client->pers.universe_quest_progress == 8 && ent->client->pers.can_play_quest == 1 && !(ent->client->pers.universe_quest_counter & (1 << 2)) && ent->client->pers.universe_quest_timer < level.time && (int) ent->client->ps.origin[0] > -18584 && (int) ent->client->ps.origin[0] < -17585 && (int) ent->client->ps.origin[1] > 17752 && (int) ent->client->ps.origin[1] < 18681 && (int) ent->client->ps.origin[2] > 1505 && (int) ent->client->ps.origin[2] < 1550)
						{ // zyk: nineth Universe Quest mission. Guardian of Time part
							if (ent->client->pers.universe_quest_messages == 1)
								trap->SendServerCommand( -1, va("chat \"%s^7: This is the place of my dreams.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 2)
								trap->SendServerCommand( -1, va("chat \"^7Guardian of Time^7: Indeed, %s.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 3)
								trap->SendServerCommand( -1, va("chat \"%s^7: Wait a second...are you...\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 4)
								trap->SendServerCommand( -1, va("chat \"^7Guardian of Time^7: Yes. I am the Guardian of Time.\""));
							else if (ent->client->pers.universe_quest_messages == 5)
								trap->SendServerCommand( -1, va("chat \"^7Guardian of Time^7: I am sealed in this place.\""));
							else if (ent->client->pers.universe_quest_messages == 6)
								trap->SendServerCommand( -1, va("chat \"^7Guardian of Time^7: The Guardian of Chaos did it to me.\""));
							else if (ent->client->pers.universe_quest_messages == 7)
								trap->SendServerCommand( -1, va("chat \"^7Guardian of Time^7: He wanted to conquer the entire Universe with my power.\""));
							else if (ent->client->pers.universe_quest_messages == 8)
								trap->SendServerCommand( -1, va("chat \"^7Guardian of Time^7: But before he sealed me, I actually sealed him in the Sacred Dimension.\""));
							else if (ent->client->pers.universe_quest_messages == 9)
								trap->SendServerCommand( -1, va("chat \"^7Guardian of Time^7: Probably you are having a lot of questions in your mind.\""));
							else if (ent->client->pers.universe_quest_messages == 10)
								trap->SendServerCommand( -1, va("chat \"^7Guardian of Time^7: Well, I am the true Guardian.\""));
							else if (ent->client->pers.universe_quest_messages == 11)
								trap->SendServerCommand( -1, va("chat \"^7Guardian of Time^7: The other guardians are just beings who got the amulets\""));
							else if (ent->client->pers.universe_quest_messages == 12)
								trap->SendServerCommand( -1, va("chat \"^7Guardian of Time^7: Understand this, their amulets are in fact parts of one.\""));
							else if (ent->client->pers.universe_quest_messages == 13)
								trap->SendServerCommand( -1, va("chat \"^7Guardian of Time^7: The amulet of time.\""));
							else if (ent->client->pers.universe_quest_messages == 14)
								trap->SendServerCommand( -1, va("chat \"^7Guardian of Time^7: This item, together with the sacred crystals, will open the gate...\""));
							else if (ent->client->pers.universe_quest_messages == 15)
								trap->SendServerCommand( -1, va("chat \"^7Guardian of Time^7: to the Sacred Dimension.\""));
							else if (ent->client->pers.universe_quest_messages == 16)
								trap->SendServerCommand( -1, va("chat \"^7Guardian of Time^7: you are the chosen one to do that hero.\""));
							else if (ent->client->pers.universe_quest_messages == 17)
								trap->SendServerCommand( -1, va("chat \"^7Guardian of Time^7: find the crystals and bring the guardian amulets here.\""));
							else if (ent->client->pers.universe_quest_messages == 18)
								trap->SendServerCommand( -1, va("chat \"^7Guardian of Time^7: the crystals will set me free, and I will recreate the amulet of time.\""));
							else if (ent->client->pers.universe_quest_messages == 19)
								trap->SendServerCommand( -1, va("chat \"^7Guardian of Time^7: Now go hero, someone is already searching for them, for evil purposes.\""));
							else if (ent->client->pers.universe_quest_messages == 20)
							{
								trap->SendServerCommand( -1, va("chat \"%s^7: I will find the crystals and set you free!\"", ent->client->pers.netname));

								ent->client->pers.universe_quest_counter |= (1 << 2);
								first_second_act_objective(ent);
								save_account(ent);
								quest_get_new_player(ent);
							}

							ent->client->pers.universe_quest_messages++;
							ent->client->pers.universe_quest_timer = level.time + 5000;
						}

						if (ent->client->pers.universe_quest_progress == 10 && ent->client->pers.can_play_quest == 1 && ent->client->pers.universe_quest_timer < level.time && (int) ent->client->ps.origin[0] > -18584 && (int) ent->client->ps.origin[0] < -17585 && (int) ent->client->ps.origin[1] > 17752 && (int) ent->client->ps.origin[1] < 18681 && (int) ent->client->ps.origin[2] > 1505 && (int) ent->client->ps.origin[2] < 1550)
						{ // zyk: eleventh objective of Universe Quest. Setting Guardian of Time free
							gentity_t *npc_ent = NULL;

							if (ent->client->pers.universe_quest_messages == 1)
								trap->SendServerCommand( -1, va("chat \"%s^7: This is it. I will use the crystals.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 2)
								npc_ent = Zyk_NPC_SpawnType("guardian_of_time",-18084,17970,1658,-90);
							else if (ent->client->pers.universe_quest_messages == 3)
								trap->SendServerCommand( -1, va("chat \"^7Guardian of Time^7: I am finally free.\""));
							else if (ent->client->pers.universe_quest_messages == 4)
								trap->SendServerCommand( -1, va("chat \"^7Guardian of Time^7: Thank you, hero.\""));
							else if (ent->client->pers.universe_quest_messages == 5)
								trap->SendServerCommand( -1, va("chat \"^7Guardian of Time^7: Give me your amulets, I will recreate the Amulet of Time.\""));
							else if (ent->client->pers.universe_quest_messages == 6)
								trap->SendServerCommand( -1, va("chat \"^7Guardian of Time^7: It is recreated. Now it will be possible to open the gate.\""));
							else if (ent->client->pers.universe_quest_messages == 7)
								trap->SendServerCommand( -1, va("chat \"^7Guardian of Time^7: To do that we also need the other chosen people here.\""));
							else if (ent->client->pers.universe_quest_messages == 8)
								trap->SendServerCommand( -1, va("chat \"^7Guardian of Time^7: I will call the other guardians and the sages with telepathy.\""));
							else if (ent->client->pers.universe_quest_messages == 9)
								trap->SendServerCommand( -1, va("chat \"^7Guardian of Time^7: They are needed in this moment.\""));
							else if (ent->client->pers.universe_quest_messages == 10)
								trap->SendServerCommand( -1, va("chat \"^7Guardian of Time^7: As the Prophecy of Time says, the sages, the guardians, you and me...\""));
							else if (ent->client->pers.universe_quest_messages == 11)
								trap->SendServerCommand( -1, va("chat \"^7Guardian of Time^7: And even the Master of Evil are required...\""));
							else if (ent->client->pers.universe_quest_messages == 12)
								trap->SendServerCommand( -1, va("chat \"^7Guardian of Time^7: To open the gate to the Sacred Dimension.\""));
							else if (ent->client->pers.universe_quest_messages == 13)
								trap->SendServerCommand( -1, va("chat \"^7Guardian of Time^7: You will understand why later.\""));
							else if (ent->client->pers.universe_quest_messages == 14)
								trap->SendServerCommand( -1, va("chat \"^7Guardian of Time^7: We must go to the Temple of the Gate.\""));
							else if (ent->client->pers.universe_quest_messages == 15)
								trap->SendServerCommand( -1, va("chat \"^7Guardian of Time^7: Just continue the path from here and you will find the temple.\""));
							else if (ent->client->pers.universe_quest_messages == 16)
								trap->SendServerCommand( -1, va("chat \"^7Guardian of Time^7: Beware, Master of Evil soldiers are guarding it right now.\""));
							else if (ent->client->pers.universe_quest_messages == 17)
								trap->SendServerCommand( -1, va("chat \"^7Guardian of Time^7: You will need to fight them.\""));
							else if (ent->client->pers.universe_quest_messages == 18)
							{
								trap->SendServerCommand( -1, va("chat \"%s^7: I understand. I will meet you all there.\"", ent->client->pers.netname));

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
									trap->SendServerCommand( -1, "chat \"^7Guardian of Time: ^7Hero, we must defeat all of the soldiers.\"");
								}
								else if (ent->client->pers.universe_quest_messages == 1)
								{
									npc_ent = Zyk_NPC_SpawnType("sage_of_universe",9800,11904,1593,-52);
									trap->SendServerCommand( -1, "chat \"^2Sage of Universe: ^7You can count on us too.\"");
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
									trap->SendServerCommand( -1, "chat \"^2Guardian of Universe: ^7We will also help.\"");
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
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",10212,8033,1593,-179);
								else if (ent->client->pers.hunter_quest_messages == 6)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",10212,8164,1593,-179);
								else if (ent->client->pers.hunter_quest_messages == 7)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",9280,9246,1593,-179);
								else if (ent->client->pers.hunter_quest_messages == 8)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",9528,9425,1593,-179);
								else if (ent->client->pers.hunter_quest_messages == 9)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",9779,9426,1593,-179);
								else if (ent->client->pers.hunter_quest_messages == 10)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",10238,10031,1800,-179);
								else if (ent->client->pers.hunter_quest_messages == 11)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",9954,10016,1800,-179);
								else if (ent->client->pers.hunter_quest_messages == 12)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",9450,10524,1800,-179);
								else if (ent->client->pers.hunter_quest_messages == 13)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",9294,10583,1800,-179);
								else if (ent->client->pers.hunter_quest_messages == 15)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",9305,10418,1800,-179);
								else if (ent->client->pers.hunter_quest_messages == 16)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",9065,10415,1800,-179);
								else if (ent->client->pers.hunter_quest_messages == 17)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",9069,10244,1800,-179);
								else if (ent->client->pers.hunter_quest_messages == 18)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",10094,9199,1800,-179);
								else if (ent->client->pers.hunter_quest_messages == 19)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",10139,9282,1800,-179);
								else if (ent->client->pers.hunter_quest_messages == 20)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",10199,9392,1800,-179);
								else if (ent->client->pers.hunter_quest_messages == 21)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",10250,9486,1800,-179);
								else if (ent->client->pers.hunter_quest_messages == 22)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",10298,9574,1800,-179);
								else if (ent->client->pers.hunter_quest_messages == 23)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",10358,9686,1800,-179);
								else if (ent->client->pers.hunter_quest_messages == 24)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",10200,9777,1800,-179);
								else if (ent->client->pers.hunter_quest_messages == 25)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",10284,8511,1800,-179);
								else if (ent->client->pers.hunter_quest_messages == 26)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",10220,8239,1800,-179);
								else if (ent->client->pers.hunter_quest_messages == 27)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",10123,8343,1800,-179);
								else if (ent->client->pers.hunter_quest_messages == 28)
									npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",10026,8490,1800,-179);

								if (npc_ent)
									npc_ent->client->pers.universe_quest_objective_control = ent->s.number;

								if (ent->client->pers.hunter_quest_messages < 14)
								{
									ent->client->pers.hunter_quest_messages++;
									ent->client->pers.hunter_quest_timer = level.time + 1500;
								}
								else if (ent->client->pers.hunter_quest_messages > 14 && ent->client->pers.hunter_quest_messages < 29)
								{
									ent->client->pers.hunter_quest_messages++;
									ent->client->pers.hunter_quest_timer = level.time + 2500;
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
									trap->SendServerCommand( -1, "chat \"^7Guardian of Time: ^7Let's go inside the temple.\"");
								else if (ent->client->pers.universe_quest_messages == 2)
									trap->SendServerCommand( -1, "chat \"^1Master of Evil: ^7We meet again, hero.\"");
								else if (ent->client->pers.universe_quest_messages == 3)
									trap->SendServerCommand( -1, va("chat \"%s: ^7This time you will not resurrect.\"", ent->client->pers.netname));
								else if (ent->client->pers.universe_quest_messages == 4)
									trap->SendServerCommand( -1, "chat \"^1Master of Evil: ^7I can resurrect and you can beat me, so there is no point to fight.\"");
								else if (ent->client->pers.universe_quest_messages == 5)
									trap->SendServerCommand( -1, "chat \"^1Master of Evil: ^7I am here to show you the truth about your friends.\"");
								else if (ent->client->pers.universe_quest_messages == 6)
									trap->SendServerCommand( -1, va("chat \"%s: ^7Don't try to fool me.\"", ent->client->pers.netname));
								else if (ent->client->pers.universe_quest_messages == 7)
									trap->SendServerCommand( -1, "chat \"^1Master of Evil: ^7Listen to me! Your friends are fooling you.\"");
								else if (ent->client->pers.universe_quest_messages == 8)
									trap->SendServerCommand( -1, "chat \"^1Master of Evil: ^7I know about the Prophecy of Time, and with it I found out the truth.\"");
								else if (ent->client->pers.universe_quest_messages == 9)
									trap->SendServerCommand( -1, "chat \"^1Master of Evil: ^7Your friends helped you just to get access to the Sacred Dimension.\"");
								else if (ent->client->pers.universe_quest_messages == 10)
									trap->SendServerCommand( -1, "chat \"^1Master of Evil: ^7And only the hero can open the gate using the Amulet of Time and the Sacred Crystals.\"");
								else if (ent->client->pers.universe_quest_messages == 11)
									trap->SendServerCommand( -1, "chat \"^1Master of Evil: ^7The Prophecy also says that whoever kills the hero could also access it with these items.\"");
								else if (ent->client->pers.universe_quest_messages == 12)
									trap->SendServerCommand( -1, "chat \"^1Master of Evil: ^7That is why I tried to kill you. But you are too strong for me.\"");
								else if (ent->client->pers.universe_quest_messages == 13)
									trap->SendServerCommand( -1, "chat \"^1Master of Evil: ^7Ask the Guardian of Time. She is the True Guardian, she would not lie.\"");
								else if (ent->client->pers.universe_quest_messages == 14)
									trap->SendServerCommand( -1, "chat \"^7Guardian of Time: ^7Unfortunately, he is saying the truth.\"");
								else if (ent->client->pers.universe_quest_messages == 15)
									trap->SendServerCommand( -1, va("chat \"%s: ^7I can't believe it...so I was used by all of you...\"", ent->client->pers.netname));
								else if (ent->client->pers.universe_quest_messages == 16)
									trap->SendServerCommand( -1, "chat \"^7Guardian of Time: ^7All of us are the chosen people. We all have some powers of the Amulet of Time.\"");
								else if (ent->client->pers.universe_quest_messages == 17)
									trap->SendServerCommand( -1, "chat \"^7Guardian of Time: ^7We were necessary here so you can make the decisive choice.\"");
								else if (ent->client->pers.universe_quest_messages == 18)
									trap->SendServerCommand( -1, "chat \"^7Guardian of Time: ^7The sages have their own reasons to go to the Sacred Dimension.\"");
								else if (ent->client->pers.universe_quest_messages == 19)
									trap->SendServerCommand( -1, "chat \"^7Guardian of Time: ^7The guardians also have their own reasons to go to the Sacred Dimension.\"");
								else if (ent->client->pers.universe_quest_messages == 20)
									trap->SendServerCommand( -1, "chat \"^7Guardian of Time: ^7I also have my reasons.\"");
								else if (ent->client->pers.universe_quest_messages == 21)
									trap->SendServerCommand( -1, "chat \"^7Guardian of Time: ^7And the Master of Evil also has his reasons.\"");
								else if (ent->client->pers.universe_quest_messages == 22)
									trap->SendServerCommand( -1, "chat \"^7Guardian of Time: ^7You will have to choose between these 4 choices.\"");
								else if (ent->client->pers.universe_quest_messages == 23)
									trap->SendServerCommand( -1, "chat \"^7Guardian of Time: ^7Understand that, after choosing, the others will lose their powers.\"");
								else if (ent->client->pers.universe_quest_messages == 24)
									trap->SendServerCommand( -1, "chat \"^7Guardian of Time: ^7And these powers will be absorbed into the Amulet of Time.\"");
								else if (ent->client->pers.universe_quest_messages == 25)
									trap->SendServerCommand( -1, va("chat \"%s: ^7That is a difficult decision.\"", ent->client->pers.netname));
								else if (ent->client->pers.universe_quest_messages == 26)
									trap->SendServerCommand( -1, "chat \"^7Guardian of Time: ^7It is in the Prophecy of Time, there is no other way.\"");
								else if (ent->client->pers.universe_quest_messages == 27)
									trap->SendServerCommand( -1, "chat \"^7Guardian of Time: ^7Your decision will set the fate of the Universe.\"");
								else if (ent->client->pers.universe_quest_messages == 28)
								{
									ent->client->pers.universe_quest_progress = 13;
									save_account(ent);
									quest_get_new_player(ent);
								}

								if (ent->client->pers.universe_quest_progress == 12 && ent->client->pers.universe_quest_messages < 29)
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
									trap->SendServerCommand( -1, "chat \"^2Sage of Universe: ^7Hero, you have to choose us.\"");
								else if (ent->client->pers.universe_quest_messages == 2)
									trap->SendServerCommand( -1, "chat \"^5Sage of Light: ^7the True Guardians must be people who listen to other opinions.\"");
								else if (ent->client->pers.universe_quest_messages == 3)
									trap->SendServerCommand( -1, "chat \"^1Sage of Darkness: ^7We will be guardians who will keep balance to the Universe.\"");
								else if (ent->client->pers.universe_quest_messages == 4)
									trap->SendServerCommand( -1, "chat \"^3Sage of Eternity: ^7And by choosing us, we will give you a new power.\"");
								else if (ent->client->pers.universe_quest_messages == 5)
									trap->SendServerCommand( -1, "chat \"^2Sage of Universe: ^7It is the ^2Sleeping Flowers^7. Choose wisely, hero.\"");
								else if (ent->client->pers.universe_quest_messages == 6)
									trap->SendServerCommand( -1, "chat \"^2Guardian of Universe: ^7Hero, by choosing us...\"");
								else if (ent->client->pers.universe_quest_messages == 7)
									trap->SendServerCommand( -1, "chat \"^1Guardian of Darkness: ^7We will use our wisdom and power to keep the balance in the Universe.\"");
								else if (ent->client->pers.universe_quest_messages == 8)
									trap->SendServerCommand( -1, "chat \"^3Guardian of Eternity: ^7We are old beings, we have much wisdom.\"");
								else if (ent->client->pers.universe_quest_messages == 9)
									trap->SendServerCommand( -1, "chat \"^5Guardian of Light: ^7If you choose us, we will give you the ^3Elemental Power^7.\"");
								else if (ent->client->pers.universe_quest_messages == 10)
									trap->SendServerCommand( -1, "chat \"^2Guardian of Universe: ^7This power uses the four elements powers you have seen us using in the guardian battles.\"");
								else if (ent->client->pers.universe_quest_messages == 11)
									trap->SendServerCommand( -1, "chat \"^2Guardian of Universe: ^7Think about that when making your choice.\"");
								else if (ent->client->pers.universe_quest_messages == 12)
									trap->SendServerCommand( -1, "chat \"^1Master of Evil: ^7Don't be a fool, don't listen to these guys.\"");
								else if (ent->client->pers.universe_quest_messages == 13)
									trap->SendServerCommand( -1, "chat \"^1Master of Evil: ^7They used you just to get more power.\"");
								else if (ent->client->pers.universe_quest_messages == 14)
									trap->SendServerCommand( -1, "chat \"^1Master of Evil: ^7At least I am true in my ambitions.\"");
								else if (ent->client->pers.universe_quest_messages == 15)
									trap->SendServerCommand( -1, "chat \"^1Master of Evil: ^7Hero, only power matters in this life. The weak deserve to die!\"");
								else if (ent->client->pers.universe_quest_messages == 16)
									trap->SendServerCommand( -1, "chat \"^1Master of Evil: ^7There will never be peace in Universe.\"");
								else if (ent->client->pers.universe_quest_messages == 17)
									trap->SendServerCommand( -1, "chat \"^1Master of Evil: ^7Choose me and I will make you powerful.\"");
								else if (ent->client->pers.universe_quest_messages == 18)
									trap->SendServerCommand( -1, "chat \"^1Master of Evil: ^7I will give you the ^1Chaos Power^7. I read about it in the Prophecy of Time.\"");
								else if (ent->client->pers.universe_quest_messages == 19)
									trap->SendServerCommand( -1, "chat \"^1Master of Evil: ^7The Guardian of Chaos has it, but after you defeat him I will give it to you.\"");
								else if (ent->client->pers.universe_quest_messages == 20)
									trap->SendServerCommand( -1, "chat \"^1Master of Evil: ^7Choose me and you won't regret it.\"");
								else if (ent->client->pers.universe_quest_messages == 21)
									trap->SendServerCommand( -1, "chat \"^7Guardian of Time: ^7Hero, listen to me.\"");
								else if (ent->client->pers.universe_quest_messages == 22)
									trap->SendServerCommand( -1, "chat \"^7Guardian of Time: ^7I am one of the oldest beings in the Universe.\"");
								else if (ent->client->pers.universe_quest_messages == 23)
									trap->SendServerCommand( -1, "chat \"^7Guardian of Time: ^7Only I have the wisdom to be the True Guardian.\"");
								else if (ent->client->pers.universe_quest_messages == 24)
									trap->SendServerCommand( -1, "chat \"^7Guardian of Time: ^7Choose me, and I will give you the Time Power.\"");
								else if (ent->client->pers.universe_quest_messages == 25)
									trap->SendServerCommand( -1, "chat \"^7Guardian of Time: ^7It will allow you to paralyze enemies for a short time.\"");
								else if (ent->client->pers.universe_quest_messages == 26)
									trap->SendServerCommand( -1, "chat \"^7Guardian of Time: ^7Now, make your decision. After choosing someone...\"");
								else if (ent->client->pers.universe_quest_messages == 27)
									trap->SendServerCommand( -1, "chat \"^7Guardian of Time: ^7Go with him to the center of the water in the temple.\"");
								else if (ent->client->pers.universe_quest_messages == 30)
								{
									ent->client->pers.universe_quest_counter |= (1 << 0);
									ent->client->pers.universe_quest_messages = 29;
									trap->SendServerCommand( -1, "chat \"^2Sage of Universe: ^7Thank you for chosing us, hero.\"");
								}
								else if (ent->client->pers.universe_quest_messages == 31)
								{
									ent->client->pers.universe_quest_counter |= (1 << 1);
									ent->client->pers.universe_quest_messages = 29;
									trap->SendServerCommand( -1, "chat \"^2Guardian of Universe: ^7Hero, you chose well.\"");
								}
								else if (ent->client->pers.universe_quest_messages == 32)
								{
									ent->client->pers.universe_quest_counter |= (1 << 2);
									ent->client->pers.universe_quest_messages = 29;
									trap->SendServerCommand( -1, "chat \"^1Master of Evil: ^7That's it! Nicely done, hero. We will have the power now!\"");
								}
								else if (ent->client->pers.universe_quest_messages == 33)
								{
									ent->client->pers.universe_quest_counter |= (1 << 3);
									ent->client->pers.universe_quest_messages = 29;
									trap->SendServerCommand( -1, "chat \"^7Guardian of Time: ^7That is a wise choice, hero.\"");
								}
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
								else if (ent->client->pers.universe_quest_messages == 29)
								{
									ent->client->pers.universe_quest_timer = level.time + 2000;
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

							// zyk: player must take the chosen npc to the center of the temple
							if (ent->client->pers.universe_quest_objective_control != -1 && ent->client->pers.universe_quest_messages == 28)
							{
								gentity_t *npc_ent = &g_entities[ent->client->pers.universe_quest_objective_control];

								if ((int) npc_ent->client->ps.origin[0] > 12534 && (int) npc_ent->client->ps.origin[0] < 12834 && (int) npc_ent->client->ps.origin[1] > 8350 && (int) npc_ent->client->ps.origin[1] < 8650 && (int) npc_ent->client->ps.origin[2] == 1512)
								{ // zyk: settings universe_quest_messages based on the choice made by the player
									if (Q_stricmp( npc_ent->NPC_type, "sage_of_light" ) == 0 || Q_stricmp( npc_ent->NPC_type, "sage_of_darkness" ) == 0 || Q_stricmp( npc_ent->NPC_type, "sage_of_eternity" ) == 0 || Q_stricmp( npc_ent->NPC_type, "sage_of_universe" ) == 0)
									{
										ent->client->pers.universe_quest_messages = 30;
									}
									else if (Q_stricmp( npc_ent->NPC_type, "guardian_boss_9" ) == 0 || Q_stricmp( npc_ent->NPC_type, "guardian_of_darkness" ) == 0 || Q_stricmp( npc_ent->NPC_type, "guardian_of_eternity" ) == 0 || Q_stricmp( npc_ent->NPC_type, "guardian_of_universe" ) == 0)
									{
										ent->client->pers.universe_quest_messages = 31;
									}
									else if (Q_stricmp( npc_ent->NPC_type, "master_of_evil" ) == 0 )
									{
										ent->client->pers.universe_quest_messages = 32;
									}
									else if (Q_stricmp( npc_ent->NPC_type, "guardian_of_time" ) == 0 )
									{
										ent->client->pers.universe_quest_messages = 33;
									}
								}
							}
						}

						if (ent->client->pers.universe_quest_progress == 14 && ent->client->pers.can_play_quest == 1)
						{ // zyk: Universe Quest last mission
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
								}
								else if (ent->client->pers.universe_quest_messages == 9)
								{ // zyk: teleports the quest player to the Sacred Dimension
									if ((int) ent->client->ps.origin[0] > 12648 && (int) ent->client->ps.origin[0] < 12688 && (int) ent->client->ps.origin[1] > 8480 && (int) ent->client->ps.origin[1] < 8520 && (int) ent->client->ps.origin[2] > 1500 && (int) ent->client->ps.origin[2] < 1520)
									{
										vec3_t origin;
										vec3_t angles;
										int npc_iterator = 0;
										gentity_t *this_ent = NULL;

										// zyk: cleaning npcs that are not the quest ones
										for (npc_iterator = level.maxclients; npc_iterator < level.num_entities; npc_iterator++)
										{
											this_ent = &g_entities[npc_iterator];

											if (this_ent && this_ent->NPC && Q_stricmp( this_ent->NPC_type, "sage_of_light" ) != 0 && Q_stricmp( this_ent->NPC_type, "sage_of_darkness" ) != 0 && Q_stricmp( this_ent->NPC_type, "sage_of_eternity" ) != 0 && Q_stricmp( this_ent->NPC_type, "sage_of_universe" ) != 0 && Q_stricmp( this_ent->NPC_type, "guardian_of_time" ) != 0 && Q_stricmp( this_ent->NPC_type, "guardian_boss_9" ) != 0 && Q_stricmp( this_ent->NPC_type, "guardian_of_darkness" ) != 0 && Q_stricmp( this_ent->NPC_type, "guardian_of_eternity" ) != 0 && Q_stricmp( this_ent->NPC_type, "guardian_of_universe" ) != 0 && Q_stricmp( this_ent->NPC_type, "master_of_evil" ) != 0)
												G_FreeEntity(this_ent);
										}

										origin[0] = -1915.0f;
										origin[1] = -26945.0f;
										origin[2] = 200.0f;
										angles[0] = 0.0f;
										angles[1] = -179.0f;
										angles[2] = 0.0f;

										zyk_TeleportPlayer(ent,origin,angles);
										ent->client->pers.universe_quest_messages = 10;
									}
								}
								else if (ent->client->pers.universe_quest_messages == 11)
								{
									trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7So, the legendary hero is finally here.\"");
								}
								else if (ent->client->pers.universe_quest_messages == 12)
								{
									trap->SendServerCommand( -1, va("chat \"%s^7: ^7I'm here to defeat you and restore the balance to the Universe.\"", ent->client->pers.netname));
								}
								else if (ent->client->pers.universe_quest_messages == 13)
								{
									trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7The Guardian of Time could not defeat me. She was weak.\"");
								}
								else if (ent->client->pers.universe_quest_messages == 14)
								{
									trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7Nothing can stop me! Not even you.\"");
								}
								else if (ent->client->pers.universe_quest_messages == 15)
								{
									trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7The time of the Prophecy of Time has come. It will be my return.\"");
								}
								else if (ent->client->pers.universe_quest_messages == 16)
								{
									trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7The Universe will be mine! I will spread Chaos over it!\"");
								}
								else if (ent->client->pers.universe_quest_messages == 17)
								{
									trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7Hero, you will now...die!\"");
								}
								else if (ent->client->pers.universe_quest_messages == 18)
								{ // zyk: here starts the battle against the Guardian of Chaos
									int npc_iterator = 0;
									gentity_t *this_ent = NULL;

									// zyk: cleaning npcs that are not the quest ones
									for (npc_iterator = MAX_CLIENTS; npc_iterator < level.num_entities; npc_iterator++)
									{
										this_ent = &g_entities[npc_iterator];

										if (this_ent && this_ent->NPC && Q_stricmp( this_ent->NPC_type, "sage_of_light" ) != 0 && Q_stricmp( this_ent->NPC_type, "sage_of_darkness" ) != 0 && Q_stricmp( this_ent->NPC_type, "sage_of_eternity" ) != 0 && Q_stricmp( this_ent->NPC_type, "sage_of_universe" ) != 0 && Q_stricmp( this_ent->NPC_type, "guardian_of_time" ) != 0 && Q_stricmp( this_ent->NPC_type, "guardian_boss_9" ) != 0 && Q_stricmp( this_ent->NPC_type, "guardian_of_darkness" ) != 0 && Q_stricmp( this_ent->NPC_type, "guardian_of_eternity" ) != 0 && Q_stricmp( this_ent->NPC_type, "guardian_of_universe" ) != 0 && Q_stricmp( this_ent->NPC_type, "master_of_evil" ) != 0)
											G_FreeEntity(this_ent);
									}

									spawn_boss(ent,0,0,0,0,"guardian_of_chaos",-4228,-26946,393,0,14);
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

									trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7How is this possible! I should be unbeatable...\"");
								}
								else if (ent->client->pers.universe_quest_messages == 21)
								{
									if (ent->client->pers.universe_quest_counter & (1 << 0))
										trap->SendServerCommand( -1, "chat \"^2Sage of Universe: ^7Amazing! You beat the Guardian of Chaos! Well done!\"");
									else if (ent->client->pers.universe_quest_counter & (1 << 1))
										trap->SendServerCommand( -1, "chat \"^2Guardian of Universe: ^7Thank you, hero! You defeated him!\"");
									else if (ent->client->pers.universe_quest_counter & (1 << 2))
										trap->SendServerCommand( -1, "chat \"^1Master of Evil: ^7Muahahahaha! That's really nice what you have done here!\"");
									else if (ent->client->pers.universe_quest_counter & (1 << 3))
										trap->SendServerCommand( -1, "chat \"^7Guardian of Time: ^7Well done, hero. You finished what I started a long time ago.\"");
								}
								else if (ent->client->pers.universe_quest_messages == 22)
								{
									if (ent->client->pers.universe_quest_counter & (1 << 0))
										trap->SendServerCommand( -1, "chat \"^2Sage of Universe: ^7I will now receive the full power of the Amulet of Time.\"");
									else if (ent->client->pers.universe_quest_counter & (1 << 1))
										trap->SendServerCommand( -1, "chat \"^2Guardian of Universe: ^7Now the Amulet of Time will have its full power.\"");
									else if (ent->client->pers.universe_quest_counter & (1 << 2))
										trap->SendServerCommand( -1, "chat \"^1Master of Evil: ^7Power! That's it! The Amulet of Time, fully powered, and it is all mine!\"");
									else if (ent->client->pers.universe_quest_counter & (1 << 3))
										trap->SendServerCommand( -1, "chat \"^7Guardian of Time: ^7The Amulet of Time will again receive its full power.\"");
								}
								else if (ent->client->pers.universe_quest_messages == 23)
								{
									if (ent->client->pers.universe_quest_counter & (1 << 0))
										trap->SendServerCommand( -1, "chat \"^2Sage of Universe: ^7I am the True Guardian now. We sages will keep balance to the Universe.\"");
									else if (ent->client->pers.universe_quest_counter & (1 << 1))
										trap->SendServerCommand( -1, "chat \"^2Guardian of Universe: ^7I have become the True Guardian. With this power, the guardians will keep balance to the Universe.\"");
									else if (ent->client->pers.universe_quest_counter & (1 << 2))
										trap->SendServerCommand( -1, "chat \"^1Master of Evil: ^7I am the True Guardian! With this power I can now be unstopable!\"");
									else if (ent->client->pers.universe_quest_counter & (1 << 3))
										trap->SendServerCommand( -1, "chat \"^7Guardian of Time: ^7Once again, I am the True Guardian. This time, the Universe will truly be in balance.\"");
								}
								else if (ent->client->pers.universe_quest_messages == 24)
								{
									if (ent->client->pers.universe_quest_counter & (1 << 0))
										trap->SendServerCommand( -1, "chat \"^2Sage of Universe: ^7Receive the ^2Sleeping Flowers ^7now. This will really be useful to you.\"");
									else if (ent->client->pers.universe_quest_counter & (1 << 1))
										trap->SendServerCommand( -1, "chat \"^2Guardian of Universe: ^7Now I will give you the ^3Elemental Power. ^7Use it when necessary.\"");
									else if (ent->client->pers.universe_quest_counter & (1 << 2))
										trap->SendServerCommand( -1, "chat \"^1Master of Evil: ^7Now, you will become my instrument of conquest. Get this ^1Chaos Power^7. Destroy all the weak!\"");
									else if (ent->client->pers.universe_quest_counter & (1 << 3))
										trap->SendServerCommand( -1, "chat \"^7Guardian of Time: ^7As I told you, you will now receive the Time Power. Use it wisely, hero.\"");
								}
								else if (ent->client->pers.universe_quest_messages == 25)
								{
									if (ent->client->pers.universe_quest_counter & (1 << 0))
										trap->SendServerCommand( -1, "chat \"^2Sage of Universe: ^7Go now and enjoy your life, my friend.\"");
									else if (ent->client->pers.universe_quest_counter & (1 << 1))
										trap->SendServerCommand( -1, "chat \"^2Guardian of Universe: ^7You have to go now hero. Be the Keeper as I told you before.\"");
									else if (ent->client->pers.universe_quest_counter & (1 << 2))
										trap->SendServerCommand( -1, "chat \"^1Master of Evil: ^7Now go and conquer everything. Destroy all the weak!\"");
									else if (ent->client->pers.universe_quest_counter & (1 << 3))
										trap->SendServerCommand( -1, "chat \"^7Guardian of Time: ^7It is time for you to go. I wish you well in your life.\"");
								}
								else if (ent->client->pers.universe_quest_messages == 26)
								{ // zyk: end of the Universe Quest
									vec3_t origin;
									vec3_t angles;

									origin[0] = 12658.0f;
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
									ent->client->pers.universe_quest_timer = level.time + 3000;
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
									for (effect_iterator = 0; effect_iterator < level.num_entities; effect_iterator++)
									{
										this_ent = &g_entities[effect_iterator];

										if (Q_stricmp(this_ent->targetname, "zyk_quest_models") == 0)
											G_FreeEntity(this_ent);
										else if (this_ent && this_ent->NPC) // zyk: this will clean the npc and vehicles in the Sacred Dimension area
											G_FreeEntity(this_ent);
									}

									if (ent->client->pers.universe_quest_counter & (1 << 3))
									{
										npc_ent = Zyk_NPC_SpawnType("guardian_of_time",12834,8500,1700,179);
										trap->SendServerCommand( -1, "chat \"^7Guardian of Time: ^7I will use the Amulet of Time and the Sacred Crystals to open the gate to the Sacred Dimension.\"");
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
										trap->SendServerCommand( -1, "chat \"^7Guardian of Time: ^7Master of Evil, use the Amulet of Time and the Sacred Crystals to open the gate to the Sacred Dimension.\"");
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
										trap->SendServerCommand( -1, "chat \"^7Guardian of Time: ^7Guardian of Universe, use the Amulet of Time and the Sacred Crystals to open the gate to the Sacred Dimension.\"");
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
										trap->SendServerCommand( -1, "chat \"^7Guardian of Time: ^7Sage of Universe, use the Amulet of Time and the Sacred Crystals to open the gate to the Sacred Dimension.\"");
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
									trap->SendServerCommand( -1, "chat \"^7Guardian of Time: ^7Remember hero. If you are in the void, go straight forward...\"");
									if (npc_ent)
										npc_ent->client->pers.universe_quest_messages = -2000;
								}

								if (ent->client->pers.hunter_quest_messages < 10)
								{
									ent->client->pers.hunter_quest_messages++;
									ent->client->pers.hunter_quest_timer = level.time + 1500;
								}
							}
						}
					}
					else if (level.quest_map == 18)
					{ 
						if (ent->client->pers.hunter_quest_progress != NUMBER_OF_OBJECTIVES && !(ent->client->pers.hunter_quest_progress & (1 << 11)) && ent->client->pers.can_play_quest == 1 && (int) ent->client->ps.origin[0] > -1168 && (int) ent->client->ps.origin[0] < -1128 && (int) ent->client->ps.origin[1] > -1478 && (int) ent->client->ps.origin[1] < -1438 && (int) ent->client->ps.origin[2] > 610 && (int) ent->client->ps.origin[2] < 620)
						{
							trap->SendServerCommand( -1, "chat \"^3Quest System: ^7Found an ancient note.\"");
							ent->client->pers.hunter_quest_progress |= (1 << 11);
							clean_note_model();
							save_account(ent);
							quest_get_new_player(ent);
						}

						if (ent->client->pers.universe_quest_progress == 9 && ent->client->pers.can_play_quest == 1 && !(ent->client->pers.universe_quest_counter & (1 << 0)) && ent->client->pers.universe_quest_timer < level.time)
						{ // zyk: Crystal of Destiny
							gentity_t *npc_ent = NULL;

							if (ent->client->pers.universe_quest_messages == 0)
								npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",-4208,-1462,888,-51);
							else if (ent->client->pers.universe_quest_messages == 1)
								npc_ent = Zyk_NPC_SpawnType("quest_reborn_boss",-3119,-1300,1004,-124);
							else if (ent->client->pers.universe_quest_messages == 2)
								npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",-2862,-1871,1102,-137);
							else if (ent->client->pers.universe_quest_messages == 3)
								npc_ent = Zyk_NPC_SpawnType("quest_reborn_boss",-3080,-2367,1103,-159);
							else if (ent->client->pers.universe_quest_messages == 4)
								npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",190,-1801,714,-141);
							else if (ent->client->pers.universe_quest_messages == 5)
								npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",353,-757,835,-177);
							else if (ent->client->pers.universe_quest_messages == 6)
								npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",442,-1360,608,179);
							else if (ent->client->pers.universe_quest_messages == 7)
								npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",475,-1502,596,170);
							else if (ent->client->pers.universe_quest_messages == 8)
								npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",-343,-2760,918,-177);
							else if (ent->client->pers.universe_quest_messages == 9)
								npc_ent = Zyk_NPC_SpawnType("quest_reborn_boss",-1846,-2360,912,-178);
							else if (ent->client->pers.universe_quest_messages == 10)
							{
								if (level.quest_model_id == -1)
									load_crystal_model(-35,-1542,608,179,0);
							}

							if (ent->client->pers.universe_quest_messages < 11)
							{
								ent->client->pers.universe_quest_messages++;
								ent->client->pers.universe_quest_timer = level.time + 1500;
							}
							else if (ent->client->pers.universe_quest_messages == 11 && (int) ent->client->ps.origin[0] > -65 && (int) ent->client->ps.origin[0] < -5 && (int) ent->client->ps.origin[1] > -1572 && (int) ent->client->ps.origin[1] < -1512 && (int) ent->client->ps.origin[2] > 588 && (int) ent->client->ps.origin[2] < 638)
							{
								ent->client->pers.universe_quest_messages = 12;
							}
							else if (ent->client->pers.universe_quest_messages > 11)
							{
								ent->client->pers.universe_quest_counter |= (1 << 0);
								trap->SendServerCommand( -1, "chat \"^3Quest System^7: Got the ^4Crystal of Destiny^7.\"");
								universe_crystals_check(ent);
								save_account(ent);
								clean_crystal_model();
								quest_get_new_player(ent);
							}
						}

						// zyk: Universe Quest artifact
						if (ent->client->pers.universe_quest_progress == 2 && ent->client->pers.can_play_quest == 1 && ent->client->pers.universe_quest_objective_control == 3 && !(ent->client->pers.universe_quest_counter & (1 << 4)) && ent->client->pers.universe_quest_timer < level.time)
						{
							gentity_t *npc_ent = NULL;
							if (ent->client->pers.universe_quest_messages == 0)
							{
								trap->SendServerCommand( -1, va("chat \"%s^7: I sense the presence of an artifact here.\"", ent->client->pers.netname));
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
								npc_ent = Zyk_NPC_SpawnType("quest_reborn_boss",-120,-1630,857,179);
								if (npc_ent)
								{
									npc_ent->client->ps.powerups[PW_FORCE_BOON] = level.time + 5500;

									npc_ent->client->pers.universe_quest_artifact_holder_id = ent-g_entities;
									ent->client->pers.universe_quest_artifact_holder_id = npc_ent-g_entities;
								}
							}

							if (ent->client->pers.universe_quest_messages < 6)
							{
								ent->client->pers.universe_quest_messages++;
								ent->client->pers.universe_quest_timer = level.time + 5000;
							}
						}

						if (ent->client->pers.universe_quest_artifact_holder_id == -2 && ent->client->pers.can_play_quest == 1 && ent->client->ps.powerups[PW_FORCE_BOON])
						{ // zyk: player got the artifact, save it to his account
							trap->SendServerCommand( -1, va("chat \"%s^7: This is one of the artifacts!\"", ent->client->pers.netname));
							ent->client->pers.universe_quest_artifact_holder_id = -1;
							ent->client->pers.universe_quest_counter |= (1 << 4);
							save_account(ent);

							universe_quest_artifacts_checker(ent);

							quest_get_new_player(ent);
						}
						else if (ent->client->pers.universe_quest_artifact_holder_id > -1 && ent->client->pers.can_play_quest == 1)
						{ // zyk: setting the force boon time in the artifact holder npc so it doesnt run out
							if (&g_entities[ent->client->pers.universe_quest_artifact_holder_id] && g_entities[ent->client->pers.universe_quest_artifact_holder_id].client && g_entities[ent->client->pers.universe_quest_artifact_holder_id].client->pers.universe_quest_artifact_holder_id != -1 && g_entities[ent->client->pers.universe_quest_artifact_holder_id].client->ps.powerups[PW_FORCE_BOON] < (level.time + 1000))
								g_entities[ent->client->pers.universe_quest_artifact_holder_id].client->ps.powerups[PW_FORCE_BOON] = level.time + 1000;
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
									trap->SendServerCommand( -1, va("chat \"^6Guardian of Agility: ^7I am the Guardian of Agility, %s^7! Im too fast for you, snail!\"",ent->client->pers.netname));
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
								trap->SendServerCommand( -1, va("chat \"%s^7: I sense the presence of an artifact here.\"", ent->client->pers.netname));
								npc_ent = Zyk_NPC_SpawnType("quest_reborn_boss",8480,-1084,-90,90);
								if (npc_ent)
								{
									npc_ent->client->ps.powerups[PW_FORCE_BOON] = level.time + 5500;

									npc_ent->client->pers.universe_quest_artifact_holder_id = ent-g_entities;
									ent->client->pers.universe_quest_artifact_holder_id = npc_ent-g_entities;
								}
							}

							if (ent->client->pers.universe_quest_messages < 1)
							{
								ent->client->pers.universe_quest_messages++;
								ent->client->pers.universe_quest_timer = level.time + 5000;
							}
						}

						if (ent->client->pers.universe_quest_artifact_holder_id == -2 && ent->client->pers.can_play_quest == 1 && ent->client->ps.powerups[PW_FORCE_BOON])
						{ // zyk: player got the artifact, save it to his account
							trap->SendServerCommand( -1, va("chat \"%s^7: This is one of the artifacts!\"", ent->client->pers.netname));
							ent->client->pers.universe_quest_artifact_holder_id = -1;
							ent->client->pers.universe_quest_counter |= (1 << 7);
							save_account(ent);

							universe_quest_artifacts_checker(ent);

							quest_get_new_player(ent);
						}
						else if (ent->client->pers.universe_quest_artifact_holder_id > -1 && ent->client->pers.can_play_quest == 1)
						{ // zyk: setting the force boon time in the artifact holder npc so it doesnt run out
							if (&g_entities[ent->client->pers.universe_quest_artifact_holder_id] && g_entities[ent->client->pers.universe_quest_artifact_holder_id].client && g_entities[ent->client->pers.universe_quest_artifact_holder_id].client->pers.universe_quest_artifact_holder_id != -1 && g_entities[ent->client->pers.universe_quest_artifact_holder_id].client->ps.powerups[PW_FORCE_BOON] < (level.time + 1000))
								g_entities[ent->client->pers.universe_quest_artifact_holder_id].client->ps.powerups[PW_FORCE_BOON] = level.time + 1000;
						}
					}
					else if (level.quest_map == 22)
					{ // zyk: Universe Quest artifact
						if (ent->client->pers.universe_quest_progress == 2 && ent->client->pers.can_play_quest == 1 && ent->client->pers.universe_quest_objective_control == 3 && !(ent->client->pers.universe_quest_counter & (1 << 9)) && ent->client->pers.universe_quest_timer < level.time)
						{
							gentity_t *npc_ent = NULL;
							if (ent->client->pers.universe_quest_messages == 0)
							{
								trap->SendServerCommand( -1, va("chat \"%s^7: I sense the presence of an artifact here.\"", ent->client->pers.netname));
								npc_ent = Zyk_NPC_SpawnType("quest_reborn_boss",3075,-3964,-102,-90);
								if (npc_ent)
								{
									npc_ent->client->ps.powerups[PW_FORCE_BOON] = level.time + 5500;

									npc_ent->client->pers.universe_quest_artifact_holder_id = ent-g_entities;
									ent->client->pers.universe_quest_artifact_holder_id = npc_ent-g_entities;
								}
							}

							if (ent->client->pers.universe_quest_messages < 1)
							{
								ent->client->pers.universe_quest_messages++;
								ent->client->pers.universe_quest_timer = level.time + 5000;
							}
						}

						if (ent->client->pers.universe_quest_artifact_holder_id == -2 && ent->client->pers.can_play_quest == 1 && ent->client->ps.powerups[PW_FORCE_BOON])
						{ // zyk: player got the artifact, save it to his account
							trap->SendServerCommand( -1, va("chat \"%s^7: This is one of the artifacts!\"", ent->client->pers.netname));
							ent->client->pers.universe_quest_artifact_holder_id = -1;
							ent->client->pers.universe_quest_counter |= (1 << 9);
							save_account(ent);

							universe_quest_artifacts_checker(ent);

							quest_get_new_player(ent);
						}
						else if (ent->client->pers.universe_quest_artifact_holder_id > -1 && ent->client->pers.can_play_quest == 1)
						{ // zyk: setting the force boon time in the artifact holder npc so it doesnt run out
							if (&g_entities[ent->client->pers.universe_quest_artifact_holder_id] && g_entities[ent->client->pers.universe_quest_artifact_holder_id].client && g_entities[ent->client->pers.universe_quest_artifact_holder_id].client->pers.universe_quest_artifact_holder_id != -1 && g_entities[ent->client->pers.universe_quest_artifact_holder_id].client->ps.powerups[PW_FORCE_BOON] < (level.time + 1000))
								g_entities[ent->client->pers.universe_quest_artifact_holder_id].client->ps.powerups[PW_FORCE_BOON] = level.time + 1000;
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
								trap->SendServerCommand( -1, "chat \"^2Green Sand Raider^7: We are the sand raiders tribe! Surrender or die!\"");
							}
							else if (ent->client->pers.universe_quest_messages == 61)
							{
								npc_ent = Zyk_NPC_SpawnType("quest_sand_raider_brown",12173,-225,-486,-179);
								trap->SendServerCommand( -1, "chat \"^3Brown Sand Raider^7: We will get the treasures of the city, and nothing can stop us!\"");
							}
							else if (ent->client->pers.universe_quest_messages == 62)
							{
								npc_ent = Zyk_NPC_SpawnType("quest_sand_raider_blue",12173,-137,-486,-179);
								trap->SendServerCommand( -1, "chat \"^4Blue Sand Raider^7: Anyone who resists will be dead so don't stand on our way!\"");
							}
							else if (ent->client->pers.universe_quest_messages == 63)
							{
								npc_ent = Zyk_NPC_SpawnType("quest_sand_raider_red",12173,-41,-486,-179);
								trap->SendServerCommand( -1, "chat \"^1Red Sand Raider^7: Ok, brothers! Let's strike!\"");
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
								trap->SendServerCommand( -1, "chat \"^3Sage of Eternity: ^7Hero. Come quickly to the Mayor's house. We need to talk to you.\"");
							}
							else if (ent->client->pers.universe_quest_messages == 201)
							{
								npc_ent = Zyk_NPC_SpawnType("sage_of_darkness",-7867,-1759,-358,90);
								trap->SendServerCommand( -1, va("chat \"%s^7: The sages! They are here! Something really serious must have happened in taspir1.\"", ent->client->pers.netname));
							}
							else if (ent->client->pers.universe_quest_messages == 202)
							{
								npc_ent = Zyk_NPC_SpawnType("sage_of_eternity",-7746,-1782,-358,90);
								trap->SendServerCommand( -1, va("chat \"%s^7: I will go to the mayor's house and find out.\"", ent->client->pers.netname));
							}
							else if (ent->client->pers.universe_quest_messages == 203)
							{
								npc_ent = Zyk_NPC_SpawnType("sage_of_universe",-7775,-1492,-358,-90);
							}
							else if (ent->client->pers.universe_quest_messages == 205)
								trap->SendServerCommand( -1, va("chat \"^5Sage of Light: ^7Now, %s^7, we will reveal you everything.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 206)
								trap->SendServerCommand( -1, va("chat \"^1Sage of Darkness: ^7%s^7, since the beginning we knew you were the chosen one.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 207)
								trap->SendServerCommand( -1, va("chat \"^3Sage of Eternity: ^7The guardians told us about the arrival of a hero here that would be able to save the Guardian of Universe.\""));
							else if (ent->client->pers.universe_quest_messages == 208)
								trap->SendServerCommand( -1, va("chat \"^2Sage of Universe: ^7It is there in the Prophecy of Universe.\""));
							else if (ent->client->pers.universe_quest_messages == 209)
								trap->SendServerCommand( -1, va("chat \"%s^7: It is more complicated than I thought.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 210)
								trap->SendServerCommand( -1, va("chat \"^1Sage of Darkness: ^7Yes %s^7, but it is the truth.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 211)
								trap->SendServerCommand( -1, va("chat \"^5Sage of Light: ^7The prophecy says that if the hero fails to defeat the Master of Evil, the entire Universe is doomed!\""));
							else if (ent->client->pers.universe_quest_messages == 212)
								trap->SendServerCommand( -1, va("chat \"^3Sage of Eternity: ^7But if the hero succeeds, then the Universe will be in balance once again.\""));
							else if (ent->client->pers.universe_quest_messages == 213)
								trap->SendServerCommand( -1, va("chat \"^2Sage of Universe: ^7%s^7, You must defeat the Master of Evil, or everything will be taken over by him!\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 214)
								trap->SendServerCommand( -1, va("chat \"^3Samir: ^7What a complicated story! But it looks like you don't have much choice!\""));
							else if (ent->client->pers.universe_quest_messages == 215)
								trap->SendServerCommand( -1, va("chat \"%s^7: I understand. But, sages, why did you all came back from taspir1? What happened there?\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 216)
								trap->SendServerCommand( -1, va("chat \"^3Sage of Eternity: ^7The Master of Evil felt our presence there and sent his soldiers to find us.\""));
							else if (ent->client->pers.universe_quest_messages == 217)
								trap->SendServerCommand( -1, va("chat \"^5Sage of Light: ^7We had to flee from there, but now he is aware of our quest to stop him.\""));
							else if (ent->client->pers.universe_quest_messages == 218)
								trap->SendServerCommand( -1, va("chat \"^1Sage of Darkness: ^7Then, we had the idea of searching for you here.\""));
							else if (ent->client->pers.universe_quest_messages == 219)
								trap->SendServerCommand( -1, va("chat \"^2Sage of Universe: ^7%s^7, you collected all artifacts and amulets. Well done.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 220)
								trap->SendServerCommand( -1, va("chat \"^2Sage of Universe: ^7Now you must go to ^3taspir1 ^7and defeat the Master of Evil.\""));
							else if (ent->client->pers.universe_quest_messages == 221)
								trap->SendServerCommand( -1, va("chat \"^2Sage of Universe: ^7The power of the amulets will merge all artifacts into his life force and make him vulnarable again.\""));
							else if (ent->client->pers.universe_quest_messages == 222)
								trap->SendServerCommand( -1, va("chat \"%s^7: Thanks for your help. Now I will go there to put an end to this.\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 223)
							{
								trap->SendServerCommand( -1, va("chat \"^2Sage of Universe: ^7You must be aware that his soldiers are there and you will need to fight them. Now go, hero.\""));

								ent->client->pers.universe_quest_progress = 6;
								ent->client->pers.universe_quest_counter = 0;
								ent->client->pers.universe_quest_objective_control = -1;

								save_account(ent);

								quest_get_new_player(ent);
							}

							if (ent->client->pers.universe_quest_messages < 40 && npc_ent)
							{ // zyk: tests npc_ent so if for some reason the npc dont get spawned, the server tries to spawn it again
								ent->client->pers.universe_quest_messages++;
								ent->client->pers.universe_quest_timer = level.time + 1500;

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
								ent->client->pers.universe_quest_timer = level.time + 2000;
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
								trap->SendServerCommand( -1, va("chat \"%s^7: The time has come. I must defeat the Master of Evil to save the Guardian of Universe.\"", ent->client->pers.netname));
								npc_ent = Zyk_NPC_SpawnType("quest_reborn_boss",1800,-2900,2785,90);
							}
							else if (ent->client->pers.universe_quest_messages == 2)
								trap->SendServerCommand( -1, va("chat \"^1Master of Evil: ^7Nice one, %s^7! You finally reached me!\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 3)
								trap->SendServerCommand( -1, va("chat \"%s: ^7Master of Evil? You won't escape from me!\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 4)
								trap->SendServerCommand( -1, va("chat \"^1Master of Evil: ^7You are wrong, %s^7. YOU who won't escape from my power!\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 5)
								trap->SendServerCommand( -1, va("chat \"^1Master of Evil: ^7I know you have got all amulets and artifacts. I can feel myself vulnerable again...\""));
							else if (ent->client->pers.universe_quest_messages == 6)
								trap->SendServerCommand( -1, va("chat \"^1Master of Evil: ^7But I am still the most powerful being in existence!\""));
							else if (ent->client->pers.universe_quest_messages == 7)
								trap->SendServerCommand( -1, va("chat \"^1Master of Evil: ^7You can't stop me, %s^7! I will rule the Universe!\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 8)
								trap->SendServerCommand( -1, va("chat \"%s: ^7We will see about that!\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 9)
								trap->SendServerCommand( -1, va("chat \"^1Master of Evil: ^7Now, %s^7. Prepare yourself to die!\"", ent->client->pers.netname));
							else if (ent->client->pers.universe_quest_messages == 10)
							{
								spawn_boss(ent,2135,-2857,2620,-90,"master_of_evil",0,0,0,0,12);

								npc_ent = NULL;
							}
							else if (ent->client->pers.universe_quest_messages == 12)
							{ // zyk: defeated Master of Evil
								zyk_NPC_Kill_f("all"); // zyk: killing the guardian spawns
								trap->SendServerCommand( -1, va("chat \"^1Master of Evil: ^7It can't be! It's not possible! I... am... gone...\""));
							}
							else if (ent->client->pers.universe_quest_messages == 13)
							{ // zyk: defeated Master of Evil
								trap->SendServerCommand( -1, va("chat \"^2Guardian of Universe: ^7Greetings, hero. I am the Guardian of Universe.\""));
							}
							else if (ent->client->pers.universe_quest_messages == 14)
							{ // zyk: defeated Master of Evil
								trap->SendServerCommand( -1, va("chat \"^2Guardian of Universe: ^7You saved me, %s^7.\"", ent->client->pers.netname));
							}
							else if (ent->client->pers.universe_quest_messages == 15)
							{ // zyk: defeated Master of Evil
								trap->SendServerCommand( -1, va("chat \"^2Guardian of Universe: ^7Thanks to your efforts, the Universe will be in balance once again.\""));
							}
							else if (ent->client->pers.universe_quest_messages == 16)
							{ // zyk: defeated Master of Evil
								trap->SendServerCommand( -1, va("chat \"^2Guardian of Universe: ^7I will meet the other guardians in mp/siege_korriban. Farewell, %s^7.\"", ent->client->pers.netname));
							
								ent->client->pers.universe_quest_progress = 7;
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
					else if (level.quest_map == 26)
					{ // zyk: Universe Quest Sacred Crystal
						if (ent->client->pers.universe_quest_progress == 9 && ent->client->pers.can_play_quest == 1 && !(ent->client->pers.universe_quest_counter & (1 << 1)) && ent->client->pers.universe_quest_timer < level.time)
						{ // zyk: Crystal of Truth
							gentity_t *npc_ent = NULL;

							if (ent->client->pers.universe_quest_messages == 0)
								npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",1175,-899,564,-159);
							else if (ent->client->pers.universe_quest_messages == 1)
								npc_ent = Zyk_NPC_SpawnType("quest_reborn_boss",967,-577,589,-96);
							else if (ent->client->pers.universe_quest_messages == 2)
								npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",1015,-1000,527,-140);
							else if (ent->client->pers.universe_quest_messages == 3)
								npc_ent = Zyk_NPC_SpawnType("quest_reborn_boss",949,-1402,487,177);
							else if (ent->client->pers.universe_quest_messages == 4)
								npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",605,-1149,479,-175);
							else if (ent->client->pers.universe_quest_messages == 5)
								npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",-1527,-1278,479,81);
							else if (ent->client->pers.universe_quest_messages == 6)
								npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",-2255,-1903,441,58);
							else if (ent->client->pers.universe_quest_messages == 7)
								npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",-2899,-1531,491,-9);
							else if (ent->client->pers.universe_quest_messages == 8)
								npc_ent = Zyk_NPC_SpawnType("quest_super_soldier",-1342,-294,480,105);
							else if (ent->client->pers.universe_quest_messages == 9)
								npc_ent = Zyk_NPC_SpawnType("quest_reborn_boss",-700,-434,593,109);
							else if (ent->client->pers.universe_quest_messages == 10)
							{
								if (level.quest_model_id == -1)
									load_crystal_model(1238,-326,457,-104,1);
							}

							if (ent->client->pers.universe_quest_messages < 11)
							{
								ent->client->pers.universe_quest_messages++;
								ent->client->pers.universe_quest_timer = level.time + 1500;
							}
							else if (ent->client->pers.universe_quest_messages == 11 && (int) ent->client->ps.origin[0] > 1208 && (int) ent->client->ps.origin[0] < 1268 && (int) ent->client->ps.origin[1] > -356 && (int) ent->client->ps.origin[1] < -296 && (int) ent->client->ps.origin[2] > 437 && (int) ent->client->ps.origin[2] < 487)
							{
								ent->client->pers.universe_quest_messages = 12;
							}
							else if (ent->client->pers.universe_quest_messages > 11)
							{
								ent->client->pers.universe_quest_counter |= (1 << 1);
								trap->SendServerCommand( -1, "chat \"^3Quest System^7: Got the ^1Crystal of Truth^7.\"");
								universe_crystals_check(ent);
								save_account(ent);
								clean_crystal_model();
								quest_get_new_player(ent);
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

			ultimate_power_events(ent);

			// zyk: quest guardians special abilities
			if (ent->client->pers.guardian_invoked_by_id != -1 && ent->health > 0)
			{
				if (ent->client->pers.guardian_mode == 1)
				{ // zyk: Guardian of Water
					if (ent->client->pers.guardian_timer < level.time)
					{ // zyk: healing ability. If guardian is at water, heals more hp
						if ((int)ent->client->ps.origin[2] < 321)
						{
							healing_water(ent,500);
							trap->SendServerCommand( -1, "chat \"^4Guardian of Water: ^7Ultra Healing Water!\"");
						}
						else
						{
							healing_water(ent,100);
							trap->SendServerCommand( -1, "chat \"^4Guardian of Water: ^7Healing Water!\"");
						}

						ent->client->pers.guardian_timer = level.time + 15000;
					}
				}
				else if (ent->client->pers.guardian_mode == 2)
				{ // zyk: Guardian of Earth
					ent->client->ps.stats[STAT_WEAPONS] = ent->client->pers.guardian_weapons_backup;

					if (ent->client->pers.guardian_timer < level.time)
					{ // zyk: uses earthquake ability
						earthquake(ent,2000,((ent->client->ps.stats[STAT_MAX_HEALTH] - ent->health)/20),800);
						ent->client->pers.guardian_timer = level.time + 3000 + ent->health;
						trap->SendServerCommand( -1, "chat \"^3Guardian of Earth: ^7Earthquake!\"");
					}
				}
				else if (ent->client->pers.guardian_mode == 3)
				{ // zyk: Guardian of Forest
					if (ent->client->pers.guardian_timer < level.time)
					{ // zyk: uses sleeping flowers ability
						sleeping_flowers(ent,4000,700);
						trap->SendServerCommand( -1, "chat \"^2Guardian of Forest: ^7Sleeping flowers!\"");
						ent->client->pers.guardian_timer = level.time + 15000;
					}
				}
				else if (ent->client->pers.guardian_mode == 4)
				{ // zyk: Guardian of Intelligence
					ent->client->ps.stats[STAT_WEAPONS] = ent->client->pers.guardian_weapons_backup;
					if (ent->client->pers.guardian_timer < level.time)
					{ // zyk: uses cloaking ability
						if (!ent->client->ps.powerups[PW_CLOAKED])
						{
							Jedi_Cloak(ent);
							trap->SendServerCommand( -1, "chat \"^5Guardian of Intelligence: ^7Cloaking.\"");
						}
						ent->client->pers.guardian_timer = level.time + 10000;
					}
				}
				else if (ent->client->pers.guardian_mode == 5)
				{ // zyk: Guardian of Agility
					ent->client->ps.stats[STAT_WEAPONS] = ent->client->pers.guardian_weapons_backup;

					if (ent->client->pers.hunter_quest_messages == 0 && ent->health < (ent->client->ps.stats[STAT_MAX_HEALTH]/2))
					{ // zyk: after losing half HP, uses his special ability
						ent->client->pers.hunter_quest_messages = 1;
						ent->NPC->stats.walkSpeed *= 2;
						ent->NPC->stats.runSpeed *= 2;
						trap->SendServerCommand( -1, "chat \"^6Guardian of Agility: ^7Ultra Speed!\"");
					}
				}
				else if (ent->client->pers.guardian_mode == 6)
				{ // zyk: Guardian of Fire
					ent->client->ps.stats[STAT_WEAPONS] = ent->client->pers.guardian_weapons_backup;
					if (ent->client->pers.guardian_timer < level.time)
					{ // zyk: fire ability
						if (ent->client->pers.flame_thrower < level.time)
						{
							ent->client->pers.flame_thrower = level.time + 5000;
							trap->SendServerCommand( -1, "chat \"^1Guardian of Fire: ^7Flame Burst!\"");
						}

						Player_FireFlameThrower(ent);

						if (ent->client->pers.flame_thrower < (level.time + 100))
							ent->client->pers.guardian_timer = level.time + 15000;
					}
				}
				else if (ent->client->pers.guardian_mode == 7)
				{ // zyk: Guardian of Wind
					int blowing_wind_duration = ((ent->client->ps.stats[STAT_MAX_HEALTH] - ent->health)/2) + 1000;

					if (ent->client->pers.guardian_timer < (level.time - blowing_wind_duration))
					{
						ent->client->pers.hunter_quest_messages = 0;
						ent->client->pers.guardian_timer = level.time + ent->client->ps.stats[STAT_MAX_HEALTH];
					}
					else if (ent->client->pers.guardian_timer < level.time)
					{ // zyk: uses blowing wind ability
						if (ent->client->pers.hunter_quest_messages == 0)
						{
							int player_iterator = 0;

							for ( player_iterator = 0; player_iterator < level.maxclients; player_iterator++)
							{
								gentity_t *player_ent = &g_entities[player_iterator];
							
								G_Sound(player_ent, CHAN_AUTO, G_SoundIndex("sound/effects/vacuum.mp3"));
							}

							ent->client->pers.hunter_quest_messages = 1;
							trap->SendServerCommand( -1, "chat \"^7Guardian of Wind: ^7Blowing Wind!\"");
						}
						else
						{
							int player_iterator = 0;
							static	vec3_t	forward;
							vec3_t dir;

							AngleVectors( ent->r.currentAngles, forward, NULL, NULL );

							VectorNormalize(forward);

							for ( player_iterator = 0; player_iterator < level.maxclients; player_iterator++)
							{
								gentity_t *player_ent = &g_entities[player_iterator];

								// zyk: player on floor needs more scale
								if (player_ent->client->ps.groundEntityNum != ENTITYNUM_NONE)
									VectorScale(forward,80.0,dir);
								else
									VectorScale(forward,25.0,dir);

								VectorAdd(player_ent->client->ps.velocity, dir, player_ent->client->ps.velocity);
							}
						}
					}
				}
				else if (ent->client->pers.guardian_mode == 8)
				{ // zyk: Guardian of Light
					if (ent->client->pers.hunter_quest_messages == 0 && ent->health < (ent->client->ps.stats[STAT_MAX_HEALTH]/2))
					{ // zyk: after losing half HP, uses his special ability
						ent->client->pers.hunter_quest_messages = 1;
						trap->SendServerCommand( -1, "chat \"^5Guardian of Light: ^7Light Power!\"");
					}
					else if (ent->client->pers.hunter_quest_messages == 1 && ent->client->pers.guardian_timer < level.time)
					{
						if ((ent->health + 2) < ent->client->ps.stats[STAT_MAX_HEALTH])
							ent->health += 2;
						else
							ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];

						ent->client->pers.guardian_timer = level.time + 1000;
					}
				}
				else if (ent->client->pers.guardian_mode == 9)
				{ // zyk: Guardian of Darkness
					if (ent->client->pers.hunter_quest_messages == 0 && ent->health < (ent->client->ps.stats[STAT_MAX_HEALTH]/2))
					{ // zyk: after losing half HP, uses his special ability
						ent->client->pers.hunter_quest_messages = 1;
						trap->SendServerCommand( -1, "chat \"^1Guardian of Darkness: ^7Dark Power!\"");
					}
				}
				else if (ent->client->pers.guardian_mode == 10)
				{ // zyk: Guardian of Eternity
					if (ent->client->pers.hunter_quest_messages == 0 && ent->health < (ent->client->ps.stats[STAT_MAX_HEALTH]/2))
					{ // zyk: after losing half HP, uses his special ability
						ent->client->pers.hunter_quest_messages = 1;
						trap->SendServerCommand( -1, "chat \"^3Guardian of Eternity: ^7Eternity Power!\"");
					}
				}
				else if (ent->client->pers.guardian_mode == 11)
				{ // zyk: Guardian of Resistance
					ent->client->ps.stats[STAT_WEAPONS] = ent->client->pers.guardian_weapons_backup;

					if (ent->client->pers.hunter_quest_messages == 0 && ent->health < (ent->client->ps.stats[STAT_MAX_HEALTH]/2))
					{ // zyk: after losing half HP, uses his special ability
						ent->client->pers.hunter_quest_messages = 1;
						ent->flags |= FL_SHIELDED;
						trap->SendServerCommand( -1, "chat \"^3Guardian of Resistance: ^7Ultra Resistance!\"");
					}
				}
				else if (ent->client->pers.guardian_mode == 12)
				{ // zyk: Master of Evil
					ent->client->ps.stats[STAT_WEAPONS] = ent->client->pers.guardian_weapons_backup;

					// zyk: take him back if he falls
					if (ent->client->ps.origin[0] < 500 || ent->client->ps.origin[0] > 3500 || ent->client->ps.origin[1] < -4350 || ent->client->ps.origin[1] > -1350 || ent->client->ps.origin[2] < 2500 || ent->client->ps.origin[2] > 4000)
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
						
						if (ent->health < (ent->client->ps.stats[STAT_MAX_HEALTH]/4))
							Zyk_NPC_SpawnType("quest_reborn_boss",2135,-2857,2800,90);
						else if (ent->health < (ent->client->ps.stats[STAT_MAX_HEALTH]/2))
							Zyk_NPC_SpawnType("quest_reborn_red",2135,-2857,2800,90);
						else if (ent->health < ((ent->client->ps.stats[STAT_MAX_HEALTH]/4) * 3))
							Zyk_NPC_SpawnType("quest_reborn_blue",2135,-2857,2800,90);
						else if (ent->health < ent->client->ps.stats[STAT_MAX_HEALTH])
							Zyk_NPC_SpawnType("quest_reborn",2135,-2857,2800,-90);

						if (!ent->client->ps.powerups[PW_CLOAKED])
							Jedi_Cloak(ent);

						ent->client->pers.guardian_timer = level.time + 25000;
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

						if (ent->client->ps.powerups[PW_CLOAKED])
							Jedi_Decloak(ent);
						else
							Jedi_Cloak(ent);

						ent->client->pers.guardian_timer = level.time + 40000;
					}
				}
				else if (ent->client->pers.guardian_mode == 14)
				{ // zyk: Guardian of Chaos
					if (ent->client->pers.guardian_timer < level.time)
					{
						if (ent->client->pers.hunter_quest_messages == 0)
						{
							trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7Cloaking!\"");
							if (!ent->client->ps.powerups[PW_CLOAKED])
								Jedi_Cloak(ent);

							ent->client->pers.hunter_quest_messages++;
							ent->client->pers.guardian_timer = level.time + 3000;
						}
						else if (ent->client->pers.hunter_quest_messages == 1)
						{
							if (ent->client->pers.guardian_timer < (level.time - 5000))
							{
								ent->client->pers.hunter_quest_messages++;
								ent->client->pers.universe_quest_messages = 0;
								ent->client->pers.guardian_timer = level.time + 2000;
							}
							else if (ent->client->pers.universe_quest_messages == 0)
							{
								gentity_t *player_ent = &g_entities[ent->client->pers.guardian_invoked_by_id];

								G_Sound(player_ent, CHAN_AUTO, G_SoundIndex("sound/effects/vacuum.mp3"));

								ent->client->pers.universe_quest_messages = 1;
								trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7Blowing Wind!\"");
							}
							else
							{
								static	vec3_t	forward;
								vec3_t dir;
								gentity_t *player_ent = &g_entities[ent->client->pers.guardian_invoked_by_id];

								AngleVectors( ent->r.currentAngles, forward, NULL, NULL );

								VectorNormalize(forward);

								if (player_ent->client->ps.groundEntityNum != ENTITYNUM_NONE)
									VectorScale(forward,90.0,dir);
								else
									VectorScale(forward,25.0,dir);

								VectorAdd(player_ent->client->ps.velocity, dir, player_ent->client->ps.velocity);
							}
						}
						else if (ent->client->pers.hunter_quest_messages == 2)
						{
							trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7Healing Water!\"");

							healing_water(ent,200);

							ent->client->pers.hunter_quest_messages++;
							ent->client->pers.guardian_timer = level.time + 3000;
						}
						else if (ent->client->pers.hunter_quest_messages == 3)
						{
							vec3_t origin, angles;
							gentity_t *player_ent = &g_entities[ent->client->pers.guardian_invoked_by_id];

							origin[0] = player_ent->client->ps.origin[0];
							origin[1] = player_ent->client->ps.origin[1];
							origin[2] = player_ent->client->ps.origin[2] + 200;
							angles[0] = 0.0f;
							angles[1] = 0.0f;
							angles[2] = 0.0f;

							trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7Teleport!\"");
							zyk_TeleportPlayer(ent,origin,angles);

							ent->client->pers.hunter_quest_messages++;
							ent->client->pers.guardian_timer = level.time + 5000;
						}
						else if (ent->client->pers.hunter_quest_messages == 4)
						{
							sleeping_flowers(ent,4000,1000);
							trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7Sleeping Flowers!\"");
							ent->client->pers.hunter_quest_messages++;
							ent->client->pers.guardian_timer = level.time + 3000;
						}
						else if (ent->client->pers.hunter_quest_messages == 5)
						{
							gentity_t *player_ent = &g_entities[ent->client->pers.guardian_invoked_by_id];
							int distance = (int)Distance(ent->client->ps.origin,player_ent->client->ps.origin);

							trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7Slap of Death!\"");
							
							if (distance < 1200)
							{
								if (player_ent->client->jetPackOn)
								{
									Jetpack_Off(player_ent);
								}

								G_Damage(player_ent,NULL,NULL,NULL,NULL,300,0,MOD_UNKNOWN);

								player_ent->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
								player_ent->client->ps.forceHandExtendTime = level.time + 3000;
								player_ent->client->ps.velocity[2] += 900;
								player_ent->client->ps.forceDodgeAnim = 0;
								player_ent->client->ps.quickerGetup = qtrue;
							}

							ent->client->pers.hunter_quest_messages++;
							ent->client->pers.guardian_timer = level.time + 3000;
						}
						else if (ent->client->pers.hunter_quest_messages == 6)
						{
							gentity_t *player_ent = &g_entities[ent->client->pers.guardian_invoked_by_id];
							int distance = (int)Distance(ent->client->ps.origin,player_ent->client->ps.origin);

							if (ent->client->pers.flame_thrower < level.time)
							{
								ent->client->pers.flame_thrower = level.time + 5000;
								trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7Flame Burst!\"");
							}

							ent->client->pers.hunter_quest_messages++;
							ent->client->pers.guardian_timer = level.time + 3000;
						}
						else if (ent->client->pers.hunter_quest_messages == 7)
						{ // zyk: Outer Area Damage ability damages the player if he is far from a certain distance
							gentity_t *player_ent = &g_entities[ent->client->pers.guardian_invoked_by_id];
							int distance = (int)Distance(ent->client->ps.origin,player_ent->client->ps.origin);

							if (distance > 500)
							{
								G_Damage(player_ent,NULL,NULL,NULL,NULL,250,0,MOD_UNKNOWN);
							}

							ent->client->pers.hunter_quest_messages++;
							ent->client->pers.guardian_timer = level.time + 3000;

							trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7Outer Area Damage!\"");
						}
						else if (ent->client->pers.hunter_quest_messages == 8)
						{
							earthquake(ent,2000,800,3000);
							ent->client->pers.hunter_quest_messages++;
							ent->client->pers.guardian_timer = level.time + 5000;

							trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7Earthquake!\"");
						}
						else if (ent->client->pers.hunter_quest_messages == 9)
						{
							gentity_t *player_ent = &g_entities[ent->client->pers.guardian_invoked_by_id];
							int distance = (int)Distance(ent->client->ps.origin,player_ent->client->ps.origin);

							if (distance < 1600)
							{
								// zyk: set the player as being affected by Time Power
								player_ent->client->pers.ultimate_power_target = 3;
								player_ent->client->pers.ultimate_power_target_timer = level.time + 6000;
							}

							ent->client->pers.hunter_quest_messages++;
							ent->client->pers.guardian_timer = level.time + 5000;

							trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7Time Power!\"");
						}
						else if (ent->client->pers.hunter_quest_messages == 10)
						{
							vec3_t origin, angles;

							origin[0] = -3136.0f;
							origin[1] = -26946.0f;
							origin[2] = 549.0f;
							angles[0] = 0.0f;
							angles[1] = 0.0f;
							angles[2] = 0.0f;

							trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7Teleport!\"");
							zyk_TeleportPlayer(ent,origin,angles);

							ent->client->pers.hunter_quest_messages++;
							ent->client->pers.guardian_timer = level.time + 2000;
						}
						else if (ent->client->pers.hunter_quest_messages == 11)
						{
							if (ent->health < (ent->client->ps.stats[STAT_MAX_HEALTH]/2))
							{ // zyk: if guardian has less than half hp, uses The Void ability
								gentity_t *player_ent = &g_entities[ent->client->pers.guardian_invoked_by_id];
								vec3_t origin, angles;

								origin[0] = -1915.0f;
								origin[1] = -26945.0f;
								origin[2] = -1000.0f;
								angles[0] = 0.0f;
								angles[1] = -179.0f;
								angles[2] = 0.0f;

								zyk_TeleportPlayer(player_ent,origin,angles);

								G_Sound(player_ent, CHAN_AUTO, G_SoundIndex("sound/chars/ragnos/misc/confuse3.mp3"));

								trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7The Void!\"");
								ent->client->pers.hunter_quest_messages++;
								ent->client->pers.guardian_timer = level.time + 1000;
								ent->client->pers.light_quest_messages = 1;
							}
							else
							{
								ent->client->pers.hunter_quest_messages++;
								ent->client->pers.guardian_timer = level.time + 3000;
							}
						}
						else if (ent->client->pers.hunter_quest_messages == 12)
						{
							if (ent->client->pers.light_quest_messages == 1)
							{ // zyk: player must escape from The Void so Guardian of Chaos can use Chaos Power ability
								gentity_t *player_ent = &g_entities[ent->client->pers.guardian_invoked_by_id];

								if ((int) player_ent->client->ps.origin[0] < -5000)
								{ // zyk: player escaped from the void, return to the battle arena
									vec3_t origin, angles;

									origin[0] = -1915.0f;
									origin[1] = -26945.0f;
									origin[2] = 200.0f;
									angles[0] = 0.0f;
									angles[1] = -179.0f;
									angles[2] = 0.0f;

									zyk_TeleportPlayer(player_ent,origin,angles);

									ent->client->pers.light_quest_messages = 0;
									ent->client->pers.guardian_timer = level.time + 5000;
								}
								else if (ent->health > 0)
								{ // zyk: while player is in the void, takes damage
									G_Damage(player_ent,NULL,NULL,NULL,NULL,((int)ceil((ent->client->ps.stats[STAT_MAX_HEALTH]*1.5)/ent->health)),0,MOD_UNKNOWN);
									ent->client->pers.guardian_timer = level.time + 1000;
								}
							}
							else
							{
								gentity_t *player_ent = &g_entities[ent->client->pers.guardian_invoked_by_id];
								int distance = (int)Distance(ent->client->ps.origin,player_ent->client->ps.origin);

								if (distance < 1600)
								{
									if (player_ent->client->jetPackOn)
									{
										Jetpack_Off(player_ent);
									}

									// zyk: set the player as being affected by Chaos Power
									player_ent->client->pers.ultimate_power_target = 2;
									player_ent->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
									player_ent->client->ps.forceHandExtendTime = level.time + 5000;
									player_ent->client->ps.velocity[2] += 150;
									player_ent->client->ps.forceDodgeAnim = 0;
									player_ent->client->ps.quickerGetup = qtrue;
									player_ent->client->ps.electrifyTime = level.time + 5000;

									G_Damage(player_ent,NULL,NULL,NULL,NULL,200,0,MOD_UNKNOWN);
								}

								trap->SendServerCommand( -1, "chat \"^1Guardian of Chaos: ^7Chaos Power!\"");

								ent->client->pers.hunter_quest_messages++;
								ent->client->pers.guardian_timer = level.time + 1500;
							}
						}
						else if (ent->client->pers.hunter_quest_messages == 13)
						{ // zyk: second part of chaos power
							gentity_t *player_ent = &g_entities[ent->client->pers.guardian_invoked_by_id];

							if (player_ent->client->pers.ultimate_power_target == 2)
							{
								G_Damage(player_ent,NULL,NULL,NULL,NULL,200,0,MOD_UNKNOWN);
							}
							ent->client->pers.hunter_quest_messages++;
							ent->client->pers.guardian_timer = level.time + 1500;
						}
						else if (ent->client->pers.hunter_quest_messages == 14)
						{ // zyk: last part of chaos power
							gentity_t *player_ent = &g_entities[ent->client->pers.guardian_invoked_by_id];

							if (player_ent->client->pers.ultimate_power_target == 2)
							{
								G_Damage(player_ent,NULL,NULL,NULL,NULL,200,0,MOD_UNKNOWN);

								player_ent->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
								player_ent->client->ps.forceHandExtendTime = level.time + 3000;
								player_ent->client->ps.velocity[2] += 900;
								player_ent->client->ps.forceDodgeAnim = 0;
								player_ent->client->ps.quickerGetup = qtrue;
								player_ent->client->pers.ultimate_power_target = 0;
							}
							ent->client->pers.hunter_quest_messages = 0;
							ent->client->pers.guardian_timer = level.time + 3000;
						}
					}

					if (ent->client->pers.flame_thrower > level.time)
						Player_FireFlameThrower(ent);
				}
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
