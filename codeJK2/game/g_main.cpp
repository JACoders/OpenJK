/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
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

#include "g_headers.h"

#include "g_local.h"
#include "g_functions.h"
#include "Q3_Interface.h"
#include "g_nav.h"
#include "g_roff.h"
#include "g_navigator.h"
#include "b_local.h"
#include "anims.h"
#include "g_icarus.h"
#include "objectives.h"
#include "../cgame/cg_local.h"	// yeah I know this is naughty, but we're shipping soon...
#include "time.h"

extern CNavigator		navigator;

static int				navCalcPathTime = 0;
int		eventClearTime = 0;

#define	STEPSIZE		18

level_locals_t	level;
game_import_t	gi;
game_export_t	globals;
gentity_t		g_entities[MAX_GENTITIES];
unsigned int	g_entityInUseBits[MAX_GENTITIES/32];

void G_ASPreCacheFree(void);

void ClearAllInUse(void)
{
	memset(g_entityInUseBits,0,sizeof(g_entityInUseBits));
}

void SetInUse(gentity_t *ent)
{
	assert(((uintptr_t)ent)>=(uintptr_t)g_entities);
	assert(((uintptr_t)ent)<=(uintptr_t)(g_entities+MAX_GENTITIES-1));
	unsigned int entNum=ent-g_entities;
	g_entityInUseBits[entNum/32]|=((unsigned int)1)<<(entNum&0x1f);
}

void ClearInUse(gentity_t *ent)
{
	assert(((uintptr_t)ent)>=(uintptr_t)g_entities);
	assert(((uintptr_t)ent)<=(uintptr_t)(g_entities+MAX_GENTITIES-1));
	unsigned int entNum=ent-g_entities;
	g_entityInUseBits[entNum/32]&=~(((unsigned int)1)<<(entNum&0x1f));
}

qboolean PInUse(unsigned int entNum)
{
	assert(entNum>=0);
	assert(entNum<MAX_GENTITIES);
	return((g_entityInUseBits[entNum/32]&(((unsigned int)1)<<(entNum&0x1f)))!=0);
}

qboolean PInUse2(gentity_t *ent)
{
	assert(((uintptr_t)ent)>=(uintptr_t)g_entities);
	assert(((uintptr_t)ent)<=(uintptr_t)(g_entities+MAX_GENTITIES-1));
	unsigned int entNum=ent-g_entities;
	return((g_entityInUseBits[entNum/32]&(((unsigned int)1)<<(entNum&0x1f)))!=0);
}

void WriteInUseBits(void)
{
	gi.AppendToSaveGame(INT_ID('I','N','U','S'), &g_entityInUseBits, sizeof(g_entityInUseBits) );
}

void ReadInUseBits(void)
{
	gi.ReadFromSaveGame(INT_ID('I','N','U','S'), &g_entityInUseBits, sizeof(g_entityInUseBits), NULL);
	// This is only temporary. Once I have converted all the ent->inuse refs,
	// it won;t be needed -MW.
	for(int i=0;i<MAX_GENTITIES;i++)
	{
		g_entities[i].inuse=PInUse(i);
	}
}

void ValidateInUseBits(void)
{
	for(int i=0;i<MAX_GENTITIES;i++)
	{
		assert(g_entities[i].inuse==PInUse(i));
	}
}

class CGEntCleaner
{
public:
	~CGEntCleaner()
	{
		for (int i=0; i<MAX_GENTITIES; i++)
		{
			gi.G2API_CleanGhoul2Models(g_entities[i].ghoul2);
		}
	}
};

// CGEntCleaner TheGEntCleaner; I don't think we want this

gentity_t		*player;

cvar_t	*g_speed;
cvar_t	*g_gravity;
cvar_t	*g_sex;
cvar_t	*g_spskill;
cvar_t	*g_cheats;
cvar_t	*g_developer;
cvar_t	*g_timescale;
cvar_t	*g_knockback;
cvar_t	*g_dismemberment;
cvar_t	*g_dismemberProbabilities;
cvar_t	*g_synchSplitAnims;

cvar_t	*g_inactivity;
cvar_t	*g_debugMove;
cvar_t	*g_debugDamage;
cvar_t	*g_weaponRespawn;
cvar_t	*g_subtitles;
cvar_t	*g_ICARUSDebug;
cvar_t	*com_buildScript;
cvar_t	*g_skippingcin;
cvar_t	*g_AIsurrender;
cvar_t	*g_numEntities;
cvar_t	*g_iscensored;

cvar_t	*g_saberAutoBlocking;
cvar_t	*g_saberRealisticCombat;
cvar_t	*g_saberMoveSpeed;
cvar_t	*g_saberAnimSpeed;
cvar_t	*g_saberAutoAim;

qboolean	stop_icarus = qfalse;

extern char *G_GetLocationForEnt( gentity_t *ent );
extern void pitch_roll_for_slope( gentity_t *forwhom, vec3_t pass_slope );
extern void CP_FindCombatPointWaypoints( void );
extern qboolean InFront( vec3_t spot, vec3_t from, vec3_t fromAngles, float threshHold = 0.0f );

void G_RunFrame (int levelTime);
void PrintEntClassname( int gentNum );
void CG_LoadInterface (void);
void ClearNPCGlobals( void );
extern void AI_UpdateGroups( void );

void ClearPlayerAlertEvents( void );
extern void NPC_ShowDebugInfo (void);
extern int killPlayerTimer;
extern	cvar_t		*d_altRoutes;

/*
static void G_DynamicMusicUpdate( usercmd_t *ucmd )

  FIXME: can we merge any of this with the G_ChooseLookEnemy stuff?
*/
static void G_DynamicMusicUpdate( void )
{
	gentity_t	*ent;
	gentity_t	*entityList[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		mins, maxs;
	int			i, e;
	int			distSq, radius = 2048;
	vec3_t		center;
	int			danger = 0;
	int			battle = 0;
	int			entTeam;
	qboolean	LOScalced = qfalse, clearLOS = qfalse;

	//FIXME: intro and/or other cues? (one-shot music sounds)

	//loops

	//player-based
	if ( !player )
	{//WTF?
		player = &g_entities[0];
		return;
	}

	if ( !player->client
		|| player->client->pers.teamState.state != TEAM_ACTIVE
		|| level.time - player->client->pers.enterTime < 100 )
	{//player hasn't spawned yet
		return;
	}

	if ( player->health <= 0 && player->max_health > 0 )
	{//defeat music
		if ( level.dmState != DM_DEATH )
		{
			level.dmState = DM_DEATH;
		}
	}

	if ( level.dmState == DM_DEATH )
	{
		gi.SetConfigstring( CS_DYNAMIC_MUSIC_STATE, "death" );
		return;
	}

	if ( level.dmState == DM_BOSS )
	{
		gi.SetConfigstring( CS_DYNAMIC_MUSIC_STATE, "boss" );
		return;
	}

	if ( level.dmState == DM_SILENCE )
	{
		gi.SetConfigstring( CS_DYNAMIC_MUSIC_STATE, "silence" );
		return;
	}

	if ( level.dmBeatTime > level.time )
	{//not on a beat
		return;
	}

	level.dmBeatTime = level.time + 1000;//1 second beats

	if ( player->health <= 20 )
	{
		danger = 1;
	}

	//enemy-based
	VectorCopy( player->currentOrigin, center );
	for ( i = 0 ; i < 3 ; i++ )
	{
		mins[i] = center[i] - radius;
		maxs[i] = center[i] + radius;
	}

	numListedEntities = gi.EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );
	for ( e = 0 ; e < numListedEntities ; e++ )
	{
		ent = entityList[ e ];
		if ( !ent || !ent->inuse )
		{
			continue;
		}

		if ( !ent->client || !ent->NPC )
		{
			if ( ent->classname && (!Q_stricmp( "PAS", ent->classname )||!Q_stricmp( "misc_turret", ent->classname )) )
			{//a turret
				entTeam = ent->noDamageTeam;
			}
			else
			{
				continue;
			}
		}
		else
		{//an NPC
			entTeam = ent->client->playerTeam;
		}

		if ( entTeam == player->client->playerTeam )
		{//ally
			continue;
		}

		if ( entTeam == TEAM_NEUTRAL && (!ent->enemy || !ent->enemy->client || ent->enemy->client->playerTeam != player->client->playerTeam) )
		{//a droid that is not mad at me or my allies
			continue;
		}

		if ( !gi.inPVS( player->currentOrigin, ent->currentOrigin ) )
		{//not potentially visible
			continue;
		}

		if ( ent->client && ent->s.weapon == WP_NONE )
		{//they don't have a weapon... FIXME: only do this for droids?
			continue;
		}

		LOScalced = clearLOS = qfalse;
		if ( (ent->enemy==player&&(!ent->NPC||ent->NPC->confusionTime<level.time)) || (ent->client&&ent->client->ps.weaponTime) || (!ent->client&&ent->attackDebounceTime>level.time))
		{//mad
			if ( ent->health > 0 )
			{//alive
				//FIXME: do I really need this check?
				if ( ent->s.weapon == WP_SABER && ent->client && !ent->client->ps.saberActive && ent->enemy != player )
				{//a Jedi who has not yet gotten made at me
					continue;
				}
				if ( ent->NPC && ent->NPC->behaviorState == BS_CINEMATIC )
				{//they're not actually going to do anything about being mad at me...
					continue;
				}
				//okay, they're in my PVS, but how close are they?  Are they actively attacking me?
				if ( !ent->client && ent->s.weapon == WP_TURRET && ent->fly_sound_debounce_time && ent->fly_sound_debounce_time - level.time < 10000 )
				{//a turret that shot at me less than ten seconds ago
				}
				else if ( ent->client && ent->client->ps.lastShotTime && ent->client->ps.lastShotTime - level.time < 10000 )
				{//an NPC that shot at me less than ten seconds ago
				}
				else
				{//not actively attacking me lately, see how far away they are
					distSq = DistanceSquared( ent->currentOrigin, player->currentOrigin );
					if ( distSq > 4194304/*2048*2048*/ )
					{//> 2048 away
						continue;
					}
					else if ( distSq > 1048576/*1024*1024*/ )
					{//> 1024 away
						clearLOS = G_ClearLOS( player, player->client->renderInfo.eyePoint, ent );
						LOScalced = qtrue;
						if ( clearLOS == qfalse )
						{//No LOS
							continue;
						}
					}
				}
				battle++;
			}
		}

		if ( level.dmState == DM_EXPLORE )
		{//only do these visibility checks if you're still in exploration mode
			if ( !InFront( ent->currentOrigin, player->currentOrigin, player->client->ps.viewangles, 0.0f) )
			{//not in front
				continue;
			}

			if ( !LOScalced )
			{
				clearLOS = G_ClearLOS( player, player->client->renderInfo.eyePoint, ent );
			}
			if ( !clearLOS )
			{//can't see them directly
				continue;
			}
		}

		if ( ent->health <= 0 )
		{//dead
			if ( !ent->client || level.time - ent->s.time > 10000 )
			{//corpse has been dead for more than 10 seconds
				//FIXME: coming across corpses should cause danger sounds too?
				continue;
			}
		}
		//we see enemies and/or corpses
		danger++;
	}

	if ( !battle )
	{//no active enemies, but look for missiles, shot impacts, etc...
		int alert = G_CheckAlertEvents( player, qtrue, qtrue, 1024, 1024, -1, qfalse, AEL_SUSPICIOUS );
		if ( alert != -1 )
		{//FIXME: maybe tripwires and other FIXED things need their own sound, some kind of danger/caution theme
			if ( G_CheckForDanger( player, alert ) )
			{//found danger near by
				danger++;
				battle = 1;
			}
			else if ( level.alertEvents[alert].owner && (level.alertEvents[alert].owner == player->enemy || (level.alertEvents[alert].owner->client && level.alertEvents[alert].owner->client->playerTeam == player->client->enemyTeam) ) )
			{//NPC on enemy team of player made some noise
				switch ( level.alertEvents[alert].level )
				{
				case AEL_DISCOVERED:
					//dangerNear = qtrue;
					break;
				case AEL_SUSPICIOUS:
					//suspicious = qtrue;
					break;
				case AEL_MINOR:
					//distraction = qtrue;
					break;
				default:
					break;
				}
			}
		}
	}

	if ( battle )
	{//battle - this can interrupt level.dmDebounceTime of lower intensity levels
		//play battle
		if ( level.dmState != DM_ACTION )
		{
			gi.SetConfigstring( CS_DYNAMIC_MUSIC_STATE, "action" );
		}
		level.dmState = DM_ACTION;
		if ( battle > 5 )
		{
			//level.dmDebounceTime = level.time + 8000;//don't change again for 5 seconds
		}
		else
		{
			//level.dmDebounceTime = level.time + 3000 + 1000*battle;
		}
	}
	else
	{
		if ( level.dmDebounceTime > level.time )
		{//not ready to switch yet
			return;
		}
		else
		{//at least 1 second (for beats)
			//level.dmDebounceTime = level.time + 1000;//FIXME: define beat time?
		}
		/*
		if ( danger || dangerNear )
		{//danger
			//stay on whatever we were on, action or exploration
			if ( !danger )
			{//minimum
				danger = 1;
			}
			if ( danger > 3 )
			{
				level.dmDebounceTime = level.time + 5000;
			}
			else
			{
				level.dmDebounceTime = level.time + 2000 + 1000*danger;
			}
		}
		else
		*/
		{//still nothing dangerous going on
			if ( level.dmState != DM_EXPLORE )
			{//just went to explore, hold it for a couple seconds at least
				//level.dmDebounceTime = level.time + 2000;
				gi.SetConfigstring( CS_DYNAMIC_MUSIC_STATE, "explore" );
			}
			level.dmState = DM_EXPLORE;
			//FIXME: look for interest points and play "mysterious" music instead of exploration?
			//FIXME: suspicious and distraction sounds should play some cue or change music in a subtle way?
			//play exploration
		}
		//FIXME: when do we go to silence?
	}
}

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
//	for ( i=1, e=g_entities,i ; i < globals.num_entities ; i++,e++ )
	for ( i=1 ; i < globals.num_entities ; i++ )
	{
//		if (!e->inuse)
//			continue;
		if(!PInUse(i))
			continue;
		e=&g_entities[i];

		if (!e->team)
			continue;
		if (e->flags & FL_TEAMSLAVE)
			continue;
		e->teammaster = e;
		c++;
		c2++;
//		for (j=i+1, e2=e+1 ; j < globals.num_entities ; j++,e2++)
		for (j=i+1; j < globals.num_entities ; j++)
		{
//			if (!e2->inuse)
//				continue;
			if(!PInUse(j))
				continue;

			e2=&g_entities[j];
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

	gi.Printf ("%i teams with %i entities\n", c, c2);
}


/*
============
G_InitCvars

============
*/
void G_InitCvars( void ) {
	// don't override the cheat state set by the system
	g_cheats = gi.cvar ("helpUsObi", "", 0);
	g_developer = gi.cvar ("developer", "", 0);

	// noset vars
	gi.cvar( "gamename", GAMEVERSION , CVAR_SERVERINFO | CVAR_ROM );
	gi.cvar( "gamedate", __DATE__ , CVAR_ROM );
	g_skippingcin = gi.cvar ("skippingCinematic", "0", CVAR_ROM);

	// latched vars

	// change anytime vars
	g_speed = gi.cvar( "g_speed", "250", CVAR_CHEAT );
	g_gravity = gi.cvar( "g_gravity", "800", CVAR_SAVEGAME|CVAR_ROM );
	g_sex = gi.cvar ("sex", "male", CVAR_USERINFO | CVAR_ARCHIVE|CVAR_SAVEGAME|CVAR_NORESTART );
	g_spskill = gi.cvar ("g_spskill", "0", CVAR_ARCHIVE | CVAR_SAVEGAME|CVAR_NORESTART);
	g_knockback = gi.cvar( "g_knockback", "1000", CVAR_CHEAT );
	g_dismemberment = gi.cvar ( "g_dismemberment", "3", CVAR_ARCHIVE );//0 = none, 1 = arms and hands, 2 = legs, 3 = waist and head, 4 = mega dismemberment
	g_dismemberProbabilities = gi.cvar ( "g_dismemberProbabilities", "1", CVAR_ARCHIVE );//0 = ignore probabilities, 1 = use probabilities
	g_synchSplitAnims = gi.cvar ( "g_synchSplitAnims", "1", 0 );

	g_inactivity = gi.cvar ("g_inactivity", "0", 0);
	g_debugMove = gi.cvar ("g_debugMove", "0", CVAR_CHEAT );
	g_debugDamage = gi.cvar ("g_debugDamage", "0", CVAR_CHEAT );
	g_ICARUSDebug = gi.cvar( "g_ICARUSDebug", "0", CVAR_CHEAT );
	g_timescale = gi.cvar( "timescale", "1", 0 );

	g_subtitles = gi.cvar( "g_subtitles", "2", CVAR_ARCHIVE );
	com_buildScript = gi.cvar ("com_buildscript", "0", 0);

	g_saberAutoBlocking = gi.cvar( "g_saberAutoBlocking", "1", CVAR_ARCHIVE|CVAR_CHEAT );//must press +block button to do any blocking
	g_saberRealisticCombat = gi.cvar( "g_saberRealisticCombat", "0", CVAR_ARCHIVE|CVAR_CHEAT );//makes collision more precise, increases damage
	g_saberMoveSpeed = gi.cvar( "g_saberMoveSpeed", "1", CVAR_ARCHIVE|CVAR_CHEAT );//how fast you run while attacking with a saber
	g_saberAnimSpeed = gi.cvar( "g_saberAnimSpeed", "1", CVAR_ARCHIVE|CVAR_CHEAT );//how fast saber animations run
	g_saberAutoAim = gi.cvar( "g_saberAutoAim", "1", CVAR_ARCHIVE|CVAR_CHEAT );//auto-aims at enemies when not moving or when just running forward

	g_AIsurrender = gi.cvar( "g_AIsurrender", "0", CVAR_CHEAT );
	g_numEntities = gi.cvar( "g_numEntities", "0", CVAR_CHEAT );

	gi.cvar( "newTotalSecrets", "0", CVAR_ROM );
	gi.cvar_set("newTotalSecrets", "0");//used to carry over the count from SP_target_secret to ClientBegin
	g_iscensored = gi.cvar( "ui_iscensored", "0", CVAR_ARCHIVE|CVAR_ROM|CVAR_INIT|CVAR_CHEAT|CVAR_NORESTART );
}

/*
============
InitGame

============
*/

// I'm just declaring a global here which I need to get at in NAV_GenerateSquadPaths for deciding if pre-calc'd
//	data is valid, and this saves changing the proto of G_SpawnEntitiesFromString() to include a checksum param which
//	may get changed anyway if a new nav system is ever used. This way saves messing with g_local.h each time -slc
int giMapChecksum;
SavedGameJustLoaded_e g_eSavedGameJustLoaded;
qboolean g_qbLoadTransition = qfalse;
#ifndef FINAL_BUILD
extern int fatalErrors;
#endif
void InitGame(  const char *mapname, const char *spawntarget, int checkSum, const char *entities, int levelTime, int randomSeed, int globalTime, SavedGameJustLoaded_e eSavedGameJustLoaded, qboolean qbLoadTransition )
{
	giMapChecksum = checkSum;
	g_eSavedGameJustLoaded = eSavedGameJustLoaded;
	g_qbLoadTransition = qbLoadTransition;

	gi.Printf ("------- Game Initialization -------\n");
	gi.Printf ("gamename: %s\n", GAMEVERSION);
	gi.Printf ("gamedate: %s\n", __DATE__);

	srand( randomSeed );

	G_InitCvars();

	G_InitMemory();

	// set some level globals
	memset( &level, 0, sizeof( level ) );
	level.time = levelTime;
	level.globalTime = globalTime;
	Q_strncpyz( level.mapname, mapname, sizeof(level.mapname) );
	if ( spawntarget != NULL && spawntarget[0] )
	{
		Q_strncpyz( level.spawntarget, spawntarget, sizeof(level.spawntarget) );
	}
	else
	{
		level.spawntarget[0] = 0;
	}


	G_InitWorldSession();

	// initialize all entities for this game
	memset( g_entities, 0, MAX_GENTITIES * sizeof(g_entities[0]) );
	globals.gentities = g_entities;
	ClearAllInUse();
	// initialize all clients for this game
	level.maxclients = 1;
	level.clients = (struct gclient_s *) G_Alloc( level.maxclients * sizeof(level.clients[0]) );
	memset(level.clients, 0, level.maxclients * sizeof(level.clients[0]));

	// set client fields on player
	g_entities[0].client = level.clients;

	// always leave room for the max number of clients,
	// even if they aren't all used, so numbers inside that
	// range are NEVER anything but clients
	globals.num_entities = MAX_CLIENTS;

	//Set up NPC init data
	NPC_InitGame();

	TIMER_Clear();

	//
	//ICARUS INIT START

	gi.Printf("------ ICARUS Initialization ------\n");
	gi.Printf("ICARUS version : %1.2f\n", ICARUS_VERSION);

	Interface_Init( &interface_export );
	ICARUS_Init();

	gi.Printf ("-----------------------------------\n");

	//ICARUS INIT END
	//

	IT_LoadItemParms ();

	ClearRegisteredItems();

	//FIXME: if this is from a loadgame, it needs to be sure to write this out whenever you do a savegame since the edges and routes are dynamic...
	navCalculatePaths	= ( navigator.Load( mapname, checkSum ) == qfalse );

	// parse the key/value pairs and spawn gentities
	G_SpawnEntitiesFromString( entities );

	// general initialization
	G_FindTeams();

//	SaveRegisteredItems();

	gi.Printf ("-----------------------------------\n");

	if ( navCalculatePaths )
	{//not loaded - need to calc paths
		navCalcPathTime = level.time + START_TIME_NAV_CALC;//make sure all ents are in and linked
	}
	else
	{//loaded
		//FIXME: if this is from a loadgame, it needs to be sure to write this
		//out whenever you do a savegame since the edges and routes are dynamic...
		//OR: always do a navigator.CheckBlockedEdges() on map startup after nav-load/calc-paths
		navigator.pathsCalculated = qtrue;//just to be safe?  Does this get saved out?  No... assumed
		//need to do this, because combatpoint waypoints aren't saved out...?
		CP_FindCombatPointWaypoints();
		navCalcPathTime = 0;

		if ( g_eSavedGameJustLoaded == eNO )
		{//clear all the failed edges unless we just loaded the game (which would include failed edges)
			navigator.ClearAllFailedEdges();
		}
	}

	player = &g_entities[0];

	//Init dynamic music
	level.dmState = DM_EXPLORE;
	level.dmDebounceTime = 0;
	level.dmBeatTime = 0;

	level.curAlertID = 1;//0 is default for lastAlertEvent, so...
	eventClearTime = 0;
}

/*
=================
ShutdownGame
=================
*/
void ShutdownGame( void ) {
	gi.Printf ("==== ShutdownGame ====\n");

	gi.Printf ("... ICARUS_Shutdown\n");
	ICARUS_Shutdown ();	//Shut ICARUS down

	gi.Printf ("... Reference Tags Cleared\n");
	TAG_Init();	//Clear the reference tags

	gi.Printf ("... Navigation Data Cleared\n");
	NAV_Shutdown();

	// write all the client session data so we can get it back
	G_WriteSessionData();

/*
Ghoul2 Insert Start
*/
	gi.Printf ("... Ghoul2 Models Shutdown\n");
	for (int i=0; i<MAX_GENTITIES; i++)
	{
		gi.G2API_CleanGhoul2Models(g_entities[i].ghoul2);
	}
/*
Ghoul2 Insert End
*/
	G_ASPreCacheFree();
}



//===================================================================

static void G_Cvar_Create( const char *var_name, const char *var_value, int flags ) {
	gi.cvar( var_name, var_value, flags );
}

/*
=================
GetGameAPI

Returns a pointer to the structure with all entry points
and global variables
=================
*/
extern int PM_ValidateAnimRange( int startFrame, int endFrame, float animSpeed );
extern "C" Q_EXPORT game_export_t* QDECL GetGameAPI( game_import_t *import ) {
	gameinfo_import_t	gameinfo_import;

	gi = *import;

	globals.apiversion = GAME_API_VERSION;
	globals.Init = InitGame;
	globals.Shutdown = ShutdownGame;

	globals.WriteLevel = WriteLevel;
	globals.ReadLevel = ReadLevel;
	globals.GameAllowedToSaveHere = GameAllowedToSaveHere;

	globals.ClientThink = ClientThink;
	globals.ClientConnect = ClientConnect;
	globals.ClientUserinfoChanged = ClientUserinfoChanged;
	globals.ClientDisconnect = ClientDisconnect;
	globals.ClientBegin = ClientBegin;
	globals.ClientCommand = ClientCommand;

	globals.RunFrame = G_RunFrame;

	globals.ConsoleCommand = ConsoleCommand;
//	globals.PrintEntClassname = PrintEntClassname;

//	globals.ValidateAnimRange = PM_ValidateAnimRange;

	globals.gentitySize = sizeof(gentity_t);

	gameinfo_import.FS_FOpenFile = gi.FS_FOpenFile;
	gameinfo_import.FS_Read = gi.FS_Read;
	gameinfo_import.FS_FCloseFile = gi.FS_FCloseFile;
	gameinfo_import.Cvar_Set = gi.cvar_set;
	gameinfo_import.Cvar_VariableStringBuffer = gi.Cvar_VariableStringBuffer;
	gameinfo_import.Cvar_Create = G_Cvar_Create;

	GI_Init( &gameinfo_import );

	return &globals;
}

void QDECL G_Error( const char *fmt, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, fmt);
	Q_vsnprintf (text, sizeof(text), fmt, argptr);
	va_end (argptr);

	gi.Error( ERR_DROP, "%s", text);
}

/*
-------------------------
Com_Error
-------------------------
*/

void Com_Error ( int level, const char *error, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	Q_vsnprintf (text, sizeof(text), error, argptr);
	va_end (argptr);

	gi.Error( level, "%s", text);
}

/*
-------------------------
Com_Printf
-------------------------
*/

void Com_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	gi.Printf ("%s", text);
}

/*
========================================================================

MAP CHANGING

========================================================================
*/


/*
========================================================================

FUNCTIONS CALLED EVERY FRAME

========================================================================
*/

static void G_CheckTasksCompleted (gentity_t *ent)
{
	if ( Q3_TaskIDPending( ent, TID_CHAN_VOICE ) )
	{
		if ( !gi.VoiceVolume[ent->s.number] )
		{//not playing a voice sound
			//return task_complete
			Q3_TaskIDComplete( ent, TID_CHAN_VOICE );
		}
	}

	if ( Q3_TaskIDPending( ent, TID_LOCATION ) )
	{
		char	*currentLoc = G_GetLocationForEnt( ent );

		if ( currentLoc && currentLoc[0] && Q_stricmp( ent->message, currentLoc ) == 0 )
		{//we're in the desired location
			Q3_TaskIDComplete( ent, TID_LOCATION );
		}
		//FIXME: else see if were in other trigger_locations?
	}
}

static void G_CheckSpecialPersistentEvents( gentity_t *ent )
{//special-case alerts that would be a pain in the ass to have the ent's think funcs generate
	if ( ent == NULL )
	{
		return;
	}
	if ( ent->s.eType == ET_MISSILE && ent->s.weapon == WP_THERMAL && ent->s.pos.trType == TR_STATIONARY )
	{
		if ( eventClearTime == level.time + ALERT_CLEAR_TIME )
		{//events were just cleared out so add me again
			AddSoundEvent( ent->owner, ent->currentOrigin, ent->splashRadius*2, AEL_DANGER );
			AddSightEvent( ent->owner, ent->currentOrigin, ent->splashRadius*2, AEL_DANGER );
		}
	}
	if ( ent->forcePushTime >= level.time )
	{//being pushed
		if ( eventClearTime == level.time + ALERT_CLEAR_TIME )
		{//events were just cleared out so add me again
			//NOTE: presumes the player did the pushing, this is not always true, but shouldn't really matter?
			if ( ent->item && ent->item->giTag == INV_SECURITY_KEY )
			{
				AddSightEvent( player, ent->currentOrigin, 128, AEL_DISCOVERED );//security keys are more important
			}
			else
			{
				AddSightEvent( player, ent->currentOrigin, 128, AEL_SUSPICIOUS );//hmm... or should this always be discovered?
			}
		}
	}
	if ( ent->contents == CONTENTS_LIGHTSABER && !Q_stricmp( "lightsaber", ent->classname ) )
	{//lightsaber
		if( ent->owner && ent->owner->client )
		{
			if ( ent->owner->client->ps.saberLength > 0 )
			{//it's on
				//sight event
				AddSightEvent( ent->owner, ent->currentOrigin, 512, AEL_DISCOVERED );
			}
		}
	}
}
/*
=============
G_RunThink

Runs thinking code for this frame if necessary
=============
*/
void G_RunThink (gentity_t *ent)
{
	float	thinktime;

	/*
	if ( ent->NPC == NULL )
	{
		if ( ent->taskManager && !stop_icarus )
		{
			ent->taskManager->Update( );
		}
	}
	*/

	thinktime = ent->nextthink;
	if ( thinktime <= 0 )
	{
		goto runicarus;
	}

	if ( thinktime > level.time )
	{
		goto runicarus;
	}

	ent->nextthink = 0;
	if ( ent->e_ThinkFunc == thinkF_NULL )	// actually you don't need this if I check for it in the next function -slc
	{
		//gi.Error ( "NULL ent->think");
		goto runicarus;
	}

	GEntity_ThinkFunc( ent );	// ent->think (ent);

runicarus:
	if ( ent->inuse )	// GEntity_ThinkFunc( ent ) can have freed up this ent if it was a type flier_child (stasis1 crash)
	{
		if ( ent->NPC == NULL )
		{
			if ( ent->taskManager && !stop_icarus )
			{
				ent->taskManager->Update( );
			}
		}
	}
}

/*
-------------------------
G_Animate
-------------------------
*/

void G_Animate ( gentity_t *self )
{
	if ( self->s.eFlags & EF_SHADER_ANIM )
	{
		return;
	}
	if ( self->s.frame == self->endFrame )
	{
		if ( self->svFlags & SVF_ANIMATING )
		{
			// ghoul2 requires some extra checks to see if the animation is done since it doesn't set the current frame directly
			if ( self->ghoul2.size() )
			{
				float frame, junk2;
				int junk;

				// I guess query ghoul2 to find out what the current frame is and see if we are done.
				gi.G2API_GetBoneAnimIndex( &self->ghoul2[self->playerModel], self->rootBone,
									(cg.time?cg.time:level.time), &frame, &junk, &junk, &junk, &junk2, NULL );

				// It NEVER seems to get to what you'd think the last frame would be, so I'm doing this to try and catch when the animation has stopped
				if ( frame + 1 >= self->endFrame )
				{
					self->svFlags &= ~SVF_ANIMATING;
					Q3_TaskIDComplete( self, TID_ANIM_BOTH );
				}
			}
			else // not ghoul2
			{
				if ( self->loopAnim )
				{
					self->s.frame = self->startFrame;
				}
				else
				{
					self->svFlags &= ~SVF_ANIMATING;
				}

				//Finished sequence - FIXME: only do this once even on looping anims?
				Q3_TaskIDComplete( self, TID_ANIM_BOTH );
			}
		}
		return;
	}

	self->svFlags |= SVF_ANIMATING;

	// With ghoul2, we'll just set the desired start and end frame and let it do it's thing.
	if ( self->ghoul2.size())
	{
		self->s.frame = self->endFrame;

		gi.G2API_SetBoneAnimIndex( &self->ghoul2[self->playerModel], self->rootBone,
									self->startFrame, self->endFrame, BONE_ANIM_OVERRIDE_FREEZE, 1.0f, cg.time, -1, -1 );
		return;
	}

	if ( self->startFrame < self->endFrame )
	{
		if ( self->s.frame < self->startFrame || self->s.frame > self->endFrame )
		{
			self->s.frame = self->startFrame;
		}
		else
		{
			self->s.frame++;
		}
	}
	else if ( self->startFrame > self->endFrame )
	{
		if ( self->s.frame > self->startFrame || self->s.frame < self->endFrame )
		{
			self->s.frame = self->startFrame;
		}
		else
		{
			self->s.frame--;
		}
	}
	else
	{
		self->s.frame = self->endFrame;
	}
}

/*
-------------------------
ResetTeamCounters
-------------------------
*/

/*
void ResetTeamCounters( void )
{
	//clear team enemy counters
	for ( int team = TEAM_FREE; team < TEAM_NUM_TEAMS; team++ )
	{
		teamEnemyCount[team] = 0;
		teamCount[team] = 0;
	}
}
*/

/*
-------------------------
UpdateTeamCounters
-------------------------
*/
/*
void UpdateTeamCounters( gentity_t *ent )
{
	if ( !ent->NPC )
	{
		return;
	}
	if ( !ent->client )
	{
		return;
	}
	if ( ent->health <= 0 )
	{
		return;
	}
	if ( (ent->s.eFlags&EF_NODRAW) )
	{
		return;
	}
	if ( ent->client->playerTeam == TEAM_FREE )
	{
		return;
	}
	//this is an NPC who is alive and visible and is on a specific team

	teamCount[ent->client->playerTeam]++;
	if ( !ent->enemy )
	{
		return;
	}

	//ent has an enemy
	if ( !ent->enemy->client )
	{//enemy is a normal ent
		if ( ent->noDamageTeam == ent->client->playerTeam )
		{//it's on my team, don't count it as an enemy
			return;
		}
	}
	else
	{//enemy is another NPC/player
		if ( ent->enemy->client->playerTeam == ent->client->playerTeam)
		{//enemy is on the same team, don't count it as an enemy
			return;
		}
	}

	//ent's enemy is not on the same team
	teamLastEnemyTime[ent->client->playerTeam] = level.time;
	teamEnemyCount[ent->client->playerTeam]++;
}
*/
extern void G_SoundOnEnt( gentity_t *ent, soundChannel_t channel, const char *soundPath );
void G_PlayerGuiltDeath( void )
{
	if ( player && player->client )
	{//simulate death
		player->client->ps.stats[STAT_HEALTH] = 0;
		//turn off saber
		if ( player->client->ps.weapon == WP_SABER && player->client->ps.saberActive )
		{
			G_SoundOnEnt( player, CHAN_WEAPON, "sound/weapons/saber/saberoff.wav" );
			player->client->ps.saberActive = qfalse;
		}
		//play the "what have I done?!" anim
		NPC_SetAnim( player, SETANIM_BOTH, BOTH_FORCEHEAL_START, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		player->client->ps.legsAnimTimer = player->client->ps.torsoAnimTimer = -1;
		//look at yourself
		player->client->ps.stats[STAT_DEAD_YAW] = player->client->ps.viewangles[YAW]+180;
	}
}
extern void NPC_SetAnim(gentity_t	*ent,int type,int anim,int priority);
extern void G_MakeTeamVulnerable( void );
int killPlayerTimer = 0;
void G_CheckEndLevelTimers( gentity_t *ent )
{
	if ( killPlayerTimer && level.time > killPlayerTimer )
	{
		killPlayerTimer = 0;
		ent->health = 0;
		if ( ent->client && ent->client->ps.stats[STAT_HEALTH] > 0 )
		{
			G_PlayerGuiltDeath();
			//cg.missionStatusShow = qtrue;
			statusTextIndex = MAX_MISSIONFAILED;
			//debounce respawn time
			ent->client->respawnTime = level.time + 2000;
			//stop all scripts
			stop_icarus = qtrue;
			//make the team killable
			G_MakeTeamVulnerable();
		}
	}
}

void NAV_CheckCalcPaths( void )
{
	if ( navCalcPathTime && navCalcPathTime < level.time )
	{//first time we've ever loaded this map...
		//clear all the failed edges
		navigator.ClearAllFailedEdges();

		//Calculate all paths
		NAV_CalculatePaths( level.mapname, giMapChecksum );

		navigator.CalculatePaths();

#ifndef FINAL_BUILD
		if ( fatalErrors )
		{
			gi.Printf( S_COLOR_RED"Not saving .nav file due to fatal nav errors\n" );
		}
		else
#endif
		if ( navigator.Save( level.mapname, giMapChecksum ) == qfalse )
		{
			gi.Printf("Unable to save navigations data for map \"%s\" (checksum:%d)\n", level.mapname, giMapChecksum );
		}
		navCalcPathTime = 0;
	}
}

/*
================
G_RunFrame

Advances the non-player objects in the world
================
*/
#if	AI_TIMERS
int AITime = 0;
int navTime = 0;
#endif//	AI_TIMERS
#ifndef FINAL_BUILD
extern int delayedShutDown;
#endif
void G_RunFrame( int levelTime ) {
	int			i;
	gentity_t	*ent;
	int			ents_inuse=0; // someone's gonna be pissed I put this here...
#if	AI_TIMERS
	AITime = 0;
	navTime = 0;
#endif//	AI_TIMERS

	level.framenum++;
	level.previousTime = level.time;
	level.time = levelTime;
	//msec = level.time - level.previousTime;

	NAV_CheckCalcPaths();
	//ResetTeamCounters();

	AI_UpdateGroups();

	if ( d_altRoutes->integer )
	{
		navigator.CheckAllFailedEdges();
	}
	navigator.ClearCheckedNodes();

	//remember last waypoint, clear current one
//	for ( i = 0, ent = &g_entities[0]; i < globals.num_entities ; i++, ent++)
	for ( i = 0; i < globals.num_entities ; i++)
	{
//		if ( !ent->inuse )
//			continue;

		if(!PInUse(i))
			continue;

		ent = &g_entities[i];

		if ( ent->waypoint != WAYPOINT_NONE
			&& ent->noWaypointTime < level.time )
		{
			ent->lastWaypoint = ent->waypoint;
			ent->waypoint = WAYPOINT_NONE;
		}
		if ( d_altRoutes->integer )
		{
			navigator.CheckFailedNodes( ent );
		}
	}

	//Look to clear out old events
	ClearPlayerAlertEvents();

	//Run the frame for all entities
//	for ( i = 0, ent = &g_entities[0]; i < globals.num_entities ; i++, ent++)
	for ( i = 0; i < globals.num_entities ; i++)
	{
//		if ( !ent->inuse )
//			continue;

		if(!PInUse(i))
			continue;
		ents_inuse++;
		ent = &g_entities[i];

		// clear events that are too old
		if ( level.time - ent->eventTime > EVENT_VALID_MSEC ) {
			if ( ent->s.event ) {
				ent->s.event = 0;	// &= EV_EVENT_BITS;
				if ( ent->client ) {
					ent->client->ps.externalEvent = 0;
				}
			}
			if ( ent->freeAfterEvent ) {
				// tempEntities or dropped items completely go away after their event
				G_FreeEntity( ent );
				continue;
			} else if ( ent->unlinkAfterEvent ) {
				// items that will respawn will hide themselves after their pickup event
				ent->unlinkAfterEvent = qfalse;
				gi.unlinkentity( ent );
			}
		}

		// temporary entities don't think
		if ( ent->freeAfterEvent )
			continue;

		G_CheckTasksCompleted(ent);

		G_Roff( ent );

		if( !ent->client )
		{
			if ( !(ent->svFlags & SVF_SELF_ANIMATING) )
			{//FIXME: make sure this is done only for models with frames?
				//Or just flag as animating?
				if ( ent->s.eFlags & EF_ANIM_ONCE )
				{
					ent->s.frame++;
				}
				else if ( !(ent->s.eFlags & EF_ANIM_ALLFAST) )
				{
					G_Animate( ent );
				}
			}
		}
		G_CheckSpecialPersistentEvents( ent );

		if ( ent->s.eType == ET_MISSILE )
		{
			G_RunMissile( ent );
			continue;
		}

		if ( ent->s.eType == ET_ITEM )
		{
			G_RunItem( ent );
			continue;
		}

		if ( ent->s.eType == ET_MOVER )
		{
			if ( ent->model && Q_stricmp( "models/test/mikeg/tie_fighter.md3", ent->model ) == 0 )
			{
				TieFighterThink( ent );
			}
			G_RunMover( ent );
			continue;
		}

		//The player
		if ( i == 0 )
		{
			// decay batteries if the goggles are active
			if ( cg.zoomMode == 1 && ent->client->ps.batteryCharge > 0 )
			{
				ent->client->ps.batteryCharge--;
			}
			else if ( cg.zoomMode == 3 && ent->client->ps.batteryCharge > 0 )
			{
				ent->client->ps.batteryCharge -= 2;

				if ( ent->client->ps.batteryCharge < 0 )
				{
					ent->client->ps.batteryCharge = 0;
				}
			}

			G_CheckEndLevelTimers( ent );
			//Recalculate the nearest waypoint for the coming NPC updates
			NAV_FindPlayerWaypoint();

			if( ent->taskManager && !stop_icarus )
			{
				ent->taskManager->Update();
			}
			//dead
			if ( ent->health <= 0 )
			{
				if ( ent->client->ps.groundEntityNum != ENTITYNUM_NONE )
				{//on the ground
					pitch_roll_for_slope( ent, NULL );
				}
			}

			continue;	// players are ucmd driven
		}

		G_RunThink( ent );	// be aware that ent may be free after returning from here, at least one func frees them
		ClearNPCGlobals();			//	but these 2 funcs are ok
		//UpdateTeamCounters( ent );	//	   to call anyway on a freed ent.
	}

	// perform final fixups on the player
	ent = &g_entities[0];
	if ( ent->inuse )
	{
		ClientEndFrame( ent );
	}
	if( g_numEntities->integer )
	{
		gi.Printf( S_COLOR_WHITE"Number of Entities in use : %d\n", ents_inuse );
	}
	//DEBUG STUFF
	NAV_ShowDebugInfo();
	NPC_ShowDebugInfo();

	G_DynamicMusicUpdate();

#if	AI_TIMERS
	AITime -= navTime;
	if ( AITime > 20 )
	{
		gi.Printf( S_COLOR_RED"ERROR: total AI time: %d\n", AITime );
	}
	else if ( AITime > 10 )
	{
		gi.Printf( S_COLOR_YELLOW"WARNING: total AI time: %d\n", AITime );
	}
	else if ( AITime > 2 )
	{
		gi.Printf( S_COLOR_GREEN"total AI time: %d\n", AITime );
	}
	if ( navTime > 20 )
	{
		gi.Printf( S_COLOR_RED"ERROR: total nav time: %d\n", navTime );
	}
	else if ( navTime > 10 )
	{
		gi.Printf( S_COLOR_YELLOW"WARNING: total nav time: %d\n", navTime );
	}
	else if ( navTime > 2 )
	{
		gi.Printf( S_COLOR_GREEN"total nav time: %d\n", navTime );
	}
#endif//	AI_TIMERS

#ifndef FINAL_BUILD
	if ( delayedShutDown != 0 && delayedShutDown < level.time )
	{
		G_Error( "Game Errors. Scroll up the console to read them." );
	}
#endif

#ifdef _DEBUG
	if(!(level.framenum&0xff))
	{
		ValidateInUseBits();
	}
#endif
}



extern qboolean player_locked;

void G_LoadSave_WriteMiscData(void)
{
	gi.AppendToSaveGame(INT_ID('L','C','K','D'), &player_locked, sizeof(player_locked));
}



void G_LoadSave_ReadMiscData(void)
{
	gi.ReadFromSaveGame(INT_ID('L','C','K','D'), &player_locked, sizeof(player_locked), NULL);
}


void PrintEntClassname( int gentNum )
{
	Com_Printf( "%d: %s in snapshot\n", gentNum, g_entities[gentNum].classname );
}

IGhoul2InfoArray &TheGameGhoul2InfoArray()
{
	return gi.TheGhoul2InfoArray();
}
