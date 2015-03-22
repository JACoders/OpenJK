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

#include "g_local.h"
#include "g_functions.h"
#include "Q3_Interface.h"
#include "g_nav.h"
#include "g_roff.h"
#include "g_navigator.h"
#include "b_local.h"
#include "anims.h"
#include "objectives.h"
#include "../cgame/cg_local.h"	// yeah I know this is naughty, but we're shipping soon...

//rww - RAGDOLL_BEGIN
#include "../ghoul2/ghoul2_gore.h"
//rww - RAGDOLL_END

extern void WP_SaberLoadParms( void );
extern qboolean G_PlayerSpawned( void );

extern void Rail_Initialize( void );
extern void Rail_Update( void );
extern void	Rail_Reset(void);

extern void Troop_Initialize( void );
extern void Troop_Update( void );
extern void	Troop_Reset(void);

extern void	Pilot_Reset(void);
extern void	Pilot_Update(void);

extern void G_ASPreCacheFree(void);


int		eventClearTime = 0;

extern qboolean g_bCollidableRoffs;


#define	STEPSIZE		18

level_locals_t	level;
game_import_t	gi;
game_export_t	globals;
gentity_t		g_entities[MAX_GENTITIES];
unsigned int	g_entityInUseBits[MAX_GENTITIES/32];

static void ClearAllInUse(void)
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

/*qboolean PInUse2(gentity_t *ent)
{
	assert(((unsigned int)ent)>=(unsigned int)g_entities);
	assert(((unsigned int)ent)<=(unsigned int)(g_entities+MAX_GENTITIES-1));
	unsigned int entNum=ent-g_entities;
	return((g_entityInUseBits[entNum/32]&(((unsigned int)1)<<(entNum&0x1f)))!=0);
}
*/

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

#ifdef _DEBUG
static void ValidateInUseBits(void)
{
	for(int i=0;i<MAX_GENTITIES;i++)
	{
		assert(g_entities[i].inuse==PInUse(i));
	}
}
#endif

gentity_t		*player;

cvar_t	*g_speed;
cvar_t	*g_gravity;
cvar_t	*g_stepSlideFix;

cvar_t	*g_sex;
cvar_t	*g_spskill;
cvar_t	*g_cheats;
cvar_t	*g_developer;
cvar_t	*g_timescale;
cvar_t	*g_knockback;
cvar_t	*g_dismemberment;
cvar_t	*g_corpseRemovalTime;

cvar_t	*g_synchSplitAnims;
#ifndef FINAL_BUILD
cvar_t	*g_AnimWarning;
#endif
cvar_t	*g_noFootSlide;
cvar_t	*g_noFootSlideRunScale;
cvar_t	*g_noFootSlideWalkScale;

cvar_t	*g_nav1;
cvar_t	*g_nav2;
cvar_t	*g_bobaDebug;

cvar_t	*g_delayedShutdown;

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
//cvar_t	*g_iscensored;

cvar_t	*g_saberAutoBlocking;
cvar_t	*g_saberRealisticCombat;
cvar_t	*debug_subdivision;
cvar_t	*g_saberDamageCapping;
cvar_t	*g_saberMoveSpeed;
cvar_t	*g_saberAnimSpeed;
cvar_t	*g_saberAutoAim;
cvar_t	*g_saberNewControlScheme;
cvar_t	*g_debugSaberLock;
cvar_t	*g_saberLockRandomNess;
cvar_t	*g_debugMelee;
cvar_t	*g_saberRestrictForce;
cvar_t	*g_saberPickuppableDroppedSabers;
cvar_t	*g_dismemberProbabilities;

cvar_t	*g_speederControlScheme;

cvar_t	*g_char_model;
cvar_t	*g_char_skin_head;
cvar_t	*g_char_skin_torso;
cvar_t	*g_char_skin_legs;
cvar_t	*g_char_color_red;
cvar_t	*g_char_color_green;
cvar_t	*g_char_color_blue;
cvar_t	*g_saber;
cvar_t	*g_saber2;
cvar_t	*g_saber_color;
cvar_t	*g_saber2_color;
cvar_t	*g_saberDarkSideSaberColor;

// kef -- used with DebugTraceForNPC
cvar_t	*g_npcdebug;

// mcg -- testing: make NPCs obey do not enter brushes better?
cvar_t	*g_navSafetyChecks;

cvar_t	*g_broadsword;

qboolean	stop_icarus = qfalse;

extern char *G_GetLocationForEnt( gentity_t *ent );
extern void CP_FindCombatPointWaypoints( void );
extern qboolean InFront( vec3_t spot, vec3_t from, vec3_t fromAngles, float threshHold = 0.0f );

void G_RunFrame (int levelTime);
void PrintEntClassname( int gentNum );
void ClearNPCGlobals( void );
extern void AI_UpdateGroups( void );

void ClearPlayerAlertEvents( void );
extern void NPC_ShowDebugInfo (void);
extern int killPlayerTimer;

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
	//qboolean	dangerNear = qfalse;
	//qboolean	suspicious = qfalse;
	qboolean	LOScalced = qfalse, clearLOS = qfalse;

	//FIXME: intro and/or other cues? (one-shot music sounds)

	//loops

	//player-based
	if ( !player )
	{//WTF?
		player = &g_entities[0];
		return;
	}

	if ( !G_PlayerSpawned() )
	{//player hasn't spawned yet!
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
				if ( ent->s.weapon == WP_SABER && ent->client && !ent->client->ps.SaberActive() && ent->enemy != player )
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

void G_ConnectNavs( const char *mapname, int checkSum )
{
	NAV::LoadFromEntitiesAndSaveToFile(mapname, checkSum);
	CP_FindCombatPointWaypoints();
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
	for ( i=MAX_CLIENTS ; i < globals.num_entities ; i++ )
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
					e->targetname = G_NewString(e2->targetname);
					e2->targetname = NULL;
				}
			}
		}
	}

	//gi.Printf ("%i teams with %i entities\n", c, c2);
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
	g_stepSlideFix = gi.cvar( "g_stepSlideFix", "1", CVAR_ARCHIVE );
	g_sex = gi.cvar ("sex", "f", CVAR_USERINFO | CVAR_ARCHIVE|CVAR_SAVEGAME|CVAR_NORESTART );
	g_spskill = gi.cvar ("g_spskill", "0", CVAR_ARCHIVE | CVAR_SAVEGAME|CVAR_NORESTART);
	g_knockback = gi.cvar( "g_knockback", "1000", CVAR_CHEAT );
	g_dismemberment = gi.cvar ( "g_dismemberment", "3", CVAR_ARCHIVE );//0 = none, 1 = arms and hands, 2 = legs, 3 = waist and head
	// for now I'm making default 10 seconds
	g_corpseRemovalTime = gi.cvar ( "g_corpseRemovalTime", "10", CVAR_ARCHIVE );//number of seconds bodies stick around for, at least... 0 = never go away
	g_synchSplitAnims = gi.cvar ( "g_synchSplitAnims", "1", 0 );
#ifndef FINAL_BUILD
	g_AnimWarning = gi.cvar ( "g_AnimWarning", "1", 0 );
#endif
	g_noFootSlide = gi.cvar ( "g_noFootSlide", "1", 0 );
	g_noFootSlideRunScale = gi.cvar ( "g_noFootSlideRunScale", "150.0", 0 );
	g_noFootSlideWalkScale = gi.cvar ( "g_noFootSlideWalkScale", "50.0", 0 );

	g_nav1 = gi.cvar ( "g_nav1", "", 0 );
	g_nav2 = gi.cvar ( "g_nav2", "", 0 );

	g_bobaDebug = gi.cvar ( "g_bobaDebug", "", 0 );

	g_delayedShutdown = gi.cvar ( "g_delayedShutdown", "0", 0 );

	g_inactivity = gi.cvar ("g_inactivity", "0", 0);
	g_debugMove = gi.cvar ("g_debugMove", "0", CVAR_CHEAT );
	g_debugDamage = gi.cvar ("g_debugDamage", "0", CVAR_CHEAT );
	g_ICARUSDebug = gi.cvar( "g_ICARUSDebug", "0", CVAR_CHEAT );
	g_timescale = gi.cvar( "timescale", "1", 0 );
	g_npcdebug = gi.cvar( "g_npcdebug", "0", 0 );
	g_navSafetyChecks = gi.cvar( "g_navSafetyChecks", "0", 0 );
	// NOTE : I also create this is UI_Init()
	g_subtitles = gi.cvar( "g_subtitles", "0", CVAR_ARCHIVE );
	com_buildScript = gi.cvar ("com_buildscript", "0", 0);

	g_saberAutoBlocking = gi.cvar( "g_saberAutoBlocking", "1", CVAR_CHEAT );//must press +block button to do any blocking
	g_saberRealisticCombat = gi.cvar( "g_saberMoreRealistic", "0", CVAR_CHEAT );//makes collision more precise, increases damage
	debug_subdivision = gi.cvar( "debug_subdivision", "0", CVAR_CHEAT|CVAR_INIT );//debug for dismemberment
	g_dismemberProbabilities = gi.cvar ( "g_dismemberProbabilities", "1", CVAR_CHEAT );//0 = ignore probabilities, 1 = use probabilities
	g_saberDamageCapping = gi.cvar( "g_saberDamageCapping", "1", CVAR_CHEAT );//caps damage of sabers vs players and NPC who use sabers
	g_saberMoveSpeed = gi.cvar( "g_saberMoveSpeed", "1", CVAR_CHEAT );//how fast you run while attacking with a saber
	g_saberAnimSpeed = gi.cvar( "g_saberAnimSpeed", "1", CVAR_CHEAT );//how fast saber animations run
	g_saberAutoAim = gi.cvar( "g_saberAutoAim", "1", CVAR_CHEAT );//auto-aims at enemies when not moving or when just running forward
	g_saberNewControlScheme = gi.cvar( "g_saberNewControlScheme", "0", CVAR_ARCHIVE );//use +forcefocus to pull off all the special moves
	g_debugSaberLock = gi.cvar( "g_debugSaberLock", "0", CVAR_CHEAT );//just for debugging/development, makes saberlocks happen all the time
	g_saberLockRandomNess = gi.cvar( "g_saberLockRandomNess", "2", CVAR_ARCHIVE );//just for debugging/development, controls frequency of saberlocks
	g_debugMelee = gi.cvar( "g_debugMelee", "0", CVAR_CHEAT );//just for debugging/development, test kicks and grabs
	g_saberRestrictForce = gi.cvar( "g_saberRestrictForce", "0", CVAR_ARCHIVE );//restricts certain force powers when using a 2-handed saber or 2 sabers
	g_saberPickuppableDroppedSabers = gi.cvar( "g_saberPickuppableDroppedSabers", "0", CVAR_CHEAT );//lets you pick up sabers that are dropped

	g_AIsurrender = gi.cvar( "g_AIsurrender", "0", CVAR_CHEAT );
	g_numEntities = gi.cvar( "g_numEntities", "0", 0 );

	gi.cvar( "newTotalSecrets", "0", CVAR_ROM );
	gi.cvar_set("newTotalSecrets", "0");//used to carry over the count from SP_target_secret to ClientBegin
	//g_iscensored = gi.cvar( "ui_iscensored", "0", CVAR_ARCHIVE|CVAR_ROM|CVAR_INIT|CVAR_CHEAT|CVAR_NORESTART );

	g_speederControlScheme = gi.cvar( "g_speederControlScheme", "2", CVAR_ARCHIVE );//2 is default, 1 is alternate

	g_char_model = gi.cvar( "g_char_model", "jedi_tf", CVAR_ARCHIVE|CVAR_SAVEGAME|CVAR_NORESTART );
	g_char_skin_head = gi.cvar( "g_char_skin_head", "head_a1", CVAR_ARCHIVE|CVAR_SAVEGAME|CVAR_NORESTART );
	g_char_skin_torso = gi.cvar( "g_char_skin_torso", "torso_a1", CVAR_ARCHIVE|CVAR_SAVEGAME|CVAR_NORESTART );
	g_char_skin_legs = gi.cvar( "g_char_skin_legs", "lower_a1", CVAR_ARCHIVE|CVAR_SAVEGAME|CVAR_NORESTART );
	g_char_color_red = gi.cvar( "g_char_color_red", "255", CVAR_ARCHIVE|CVAR_SAVEGAME|CVAR_NORESTART );
	g_char_color_green = gi.cvar( "g_char_color_green", "255", CVAR_ARCHIVE|CVAR_SAVEGAME|CVAR_NORESTART );
	g_char_color_blue = gi.cvar( "g_char_color_blue", "255", CVAR_ARCHIVE|CVAR_SAVEGAME|CVAR_NORESTART );
	g_saber = gi.cvar( "g_saber", "single_1", CVAR_ARCHIVE|CVAR_SAVEGAME|CVAR_NORESTART );
	g_saber2 = gi.cvar( "g_saber2", "", CVAR_ARCHIVE|CVAR_SAVEGAME|CVAR_NORESTART );
	g_saber_color = gi.cvar( "g_saber_color", "yellow", CVAR_ARCHIVE|CVAR_SAVEGAME|CVAR_NORESTART );
	g_saber2_color = gi.cvar( "g_saber2_color", "yellow", CVAR_ARCHIVE|CVAR_SAVEGAME|CVAR_NORESTART );
	g_saberDarkSideSaberColor = gi.cvar( "g_saberDarkSideSaberColor", "0", CVAR_ARCHIVE );	//when you turn evil, it turns your saber red!

	g_broadsword = gi.cvar( "broadsword", "1", 0);

	gi.cvar( "tier_storyinfo", "0", CVAR_ROM|CVAR_SAVEGAME|CVAR_NORESTART);
	gi.cvar( "tiers_complete", "", CVAR_ROM|CVAR_SAVEGAME|CVAR_NORESTART);

	gi.cvar( "ui_prisonerobj_currtotal", "0", CVAR_ROM|CVAR_SAVEGAME|CVAR_NORESTART);
	gi.cvar( "ui_prisonerobj_maxtotal", "0", CVAR_ROM|CVAR_SAVEGAME|CVAR_NORESTART);

	gi.cvar( "g_clearstats", "1", CVAR_ROM|CVAR_NORESTART);

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
void InitGame(  const char *mapname, const char *spawntarget, int checkSum, const char *entities, int levelTime, int randomSeed, int globalTime, SavedGameJustLoaded_e eSavedGameJustLoaded, qboolean qbLoadTransition )
{
	//rww - default this to 0, we will auto-set it to 1 if we run into a terrain ent
	gi.cvar_set("RMG", "0");

	g_bCollidableRoffs = false;

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

	//Load sabers.cfg data
	WP_SaberLoadParms();
	//Set up NPC init data
	NPC_InitGame();

	TIMER_Clear();
	Rail_Reset();
	Troop_Reset();
	Pilot_Reset();

	IT_LoadItemParms ();

	ClearRegisteredItems();

	// clear out old nav info, attempt to load from file
	NAV::LoadFromFile(level.mapname, giMapChecksum);

	// parse the key/value pairs and spawn gentities
	G_SpawnEntitiesFromString( entities );

	// general initialization
	G_FindTeams();

//	SaveRegisteredItems();

	gi.Printf ("-----------------------------------\n");

	Rail_Initialize();
	Troop_Initialize();


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
void ShutdownGame( void )
{
	// write all the client session data so we can get it back
	G_WriteSessionData();

	// Destroy the Game Interface.
	IGameInterface::Destroy();

	// Shut ICARUS down.
	IIcarusInterface::DestroyIcarus();

	// Destroy the Game Interface again.  Only way to really free everything.
	IGameInterface::Destroy();

	TAG_Init();	//Clear the reference tags
/*
Ghoul2 Insert Start
*/
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

//BEGIN GAMESIDE RMG
qboolean G_ParseSpawnVars( const char **data );
void G_SpawnGEntityFromSpawnVars( void );

void G_GameSpawnRMGEntity(char *s)
{
	if (G_ParseSpawnVars((const char **)&s))
	{
		G_SpawnGEntityFromSpawnVars();
	}
}
//END GAMESIDE RMG

/*
=================
GetGameAPI

Returns a pointer to the structure with all entry points
and global variables
=================
*/
extern int PM_ValidateAnimRange( const int startFrame, const int endFrame, const float animSpeed );
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
	globals.ConnectNavs = G_ConnectNavs;

	globals.ConsoleCommand = ConsoleCommand;
	//globals.PrintEntClassname = PrintEntClassname;

	//globals.ValidateAnimRange = PM_ValidateAnimRange;

	globals.GameSpawnRMGEntity = G_GameSpawnRMGEntity;

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
			AddSoundEvent( ent->owner, ent->currentOrigin, ent->splashRadius*2, AEL_DANGER, qfalse, qtrue );
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
			if ( ent->owner->client->ps.SaberLength() > 0 )
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
	if ( (ent->nextthink <= 0) || (ent->nextthink > level.time) )
	{
		goto runicarus;
	}

	ent->nextthink = 0;
	if ( ent->e_ThinkFunc == thinkF_NULL )	// actually you don't need this since I check for it in the next function -slc
	{
		goto runicarus;
	}

	GEntity_ThinkFunc( ent );

runicarus:
	if ( ent->inuse )	// GEntity_ThinkFunc( ent ) can have freed up this ent if it was a type flier_child (stasis1 crash)
	{
		if ( ent->NPC == NULL )
		{
			if ( ent->m_iIcarusID != IIcarusInterface::ICARUS_INVALID && !stop_icarus )
			{
				IIcarusInterface::GetIcarus()->Update( ent->m_iIcarusID );
			}
		}
	}
}

/*
-------------------------
G_Animate
-------------------------
*/

static void G_Animate ( gentity_t *self )
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
void G_PlayerGuiltDeath( void )
{
	if ( player && player->client )
	{//simulate death
		player->client->ps.stats[STAT_HEALTH] = 0;
		//turn off saber
		if ( player->client->ps.weapon == WP_SABER && player->client->ps.SaberActive() )
		{
			G_SoundIndexOnEnt( player, CHAN_WEAPON, player->client->ps.saber[0].soundOff );
			player->client->ps.SaberDeactivate();
		}
		//play the "what have I done?!" anim
		NPC_SetAnim( player, SETANIM_BOTH, BOTH_FORCEHEAL_START, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		player->client->ps.legsAnimTimer = player->client->ps.torsoAnimTimer = -1;
		//look at yourself
		player->client->ps.stats[STAT_DEAD_YAW] = player->client->ps.viewangles[YAW]+180;
	}
}
extern void NPC_SetAnim(gentity_t	*ent,int setAnimParts,int anim,int setAnimFlags, int iBlend);
extern void G_MakeTeamVulnerable( void );
int killPlayerTimer = 0;
static void G_CheckEndLevelTimers( gentity_t *ent )
{
	if ( killPlayerTimer && level.time > killPlayerTimer )
	{
		killPlayerTimer = 0;
		ent->health = 0;
		if ( ent->client && ent->client->ps.stats[STAT_HEALTH] > 0 )
		{
			G_PlayerGuiltDeath();
			//cg.missionStatusShow = qtrue;
			statusTextIndex = MISSIONFAILED_TURNED;
			//debounce respawn time
			ent->client->respawnTime = level.time + 2000;
			//stop all scripts
			stop_icarus = qtrue;
			//make the team killable
			G_MakeTeamVulnerable();
		}
	}
}



//rww - RAGDOLL_BEGIN
class CGameRagDollUpdateParams : public CRagDollUpdateParams
{
	void EffectorCollision(const SRagDollEffectorCollision &data)
	{
		//Com_Printf("Effector Collision at (%f %f %f)\n",data.effectorPosition[0],data.effectorPosition[1],data.effectorPosition[2]);
		vec3_t effectorPosDif;

		if (data.useTracePlane)
		{
			float magicFactor42=64.0f;
			VectorScale(data.tr.plane.normal,magicFactor42,effectorPosDif);
		}
		else
		{
			gentity_t *thisguy = &g_entities[me];

			if (thisguy && thisguy->client)
			{
				VectorSubtract(thisguy->client->ps.origin, data.effectorPosition, effectorPosDif);
			}
			else
			{
				return;
			}
		}

		VectorAdd(effectorTotal, effectorPosDif, effectorTotal);

		hasEffectorData = qtrue;
		return;
	}
	void RagDollBegin()
	{
		return;
	}
	virtual void RagDollSettled()
	{
		return;
	}
	void Collision()   // we had a collision, please stop animating and (sometime soon) call SetRagDoll RP_DEATH_COLLISION
	{
		return;
	}

#ifdef _DEBUG
	void DebugLine(vec3_t p1,vec3_t p2,int color,bool bbox)
	{
		if (!bbox)
		{
			CG_TestLine(p1, p2, 50, color, 1);
		}
		return;
	}
#endif
public:
	vec3_t effectorTotal;
	qboolean hasEffectorData;
};

//list of valid ragdoll effectors
static const char *g_effectorStringTable[] =
{ //commented out the ones I don't want dragging to affect
//	"thoracic",
//	"rhand",
	"lhand",
	"rtibia",
	"ltibia",
	"rtalus",
	"ltalus",
//	"rradiusX",
	"lradiusX",
	"rfemurX",
	"lfemurX",
//	"ceyebrow",
	NULL //always terminate
};

extern qboolean G_StandardHumanoid( gentity_t *self );
extern void PM_SetTorsoAnimTimer( gentity_t *ent, int *torsoAnimTimer, int time );
extern void PM_SetLegsAnimTimer( gentity_t *ent, int *legsAnimTimer, int time );
extern qboolean G_ReleaseEntity( gentity_t *grabber );

static void G_BodyDragUpdate(gentity_t *ent, gentity_t *dragger)
{
	vec3_t handVec;
	float handDist;

	assert(ent && ent->inuse && ent->client && ent->ghoul2.size() &&
		dragger && dragger->inuse && dragger->client && dragger->ghoul2.size());

	VectorSubtract(dragger->client->renderInfo.handRPoint, ent->client->renderInfo.torsoPoint, handVec);
	handDist = VectorLength(handVec);

	if (handDist > 64.0f)
	{
		G_ReleaseEntity(dragger);
	}
	else if (handDist > 12.0f)
	{
		VectorNormalize(handVec);
		VectorScale(handVec, 256.0f, handVec);
		handVec[2] = 0;

		//VectorAdd(ent->client->ps.velocity, handVec, ent->client->ps.velocity);
		//VectorCopy(handVec, ent->client->ps.velocity);
		ent->client->ps.velocity[0] = handVec[0];
		ent->client->ps.velocity[1] = handVec[1];
	}
}

//we want to see which way the pelvis is facing to get a relatively oriented base settling frame
//this is to avoid the arms stretching in opposite directions on the body trying to reach the base
//pose if the pelvis is flipped opposite of the base pose or something -rww
static int G_RagAnimForPositioning(gentity_t *ent)
{
	vec3_t dir;
	mdxaBone_t matrix;
	assert(ent->client);
	vec3_t G2Angles = {0, ent->client->ps.viewangles[YAW], 0};

	assert(ent->ghoul2.size() > 0);
	assert(ent->crotchBolt > -1);

	gi.G2API_GetBoltMatrix(ent->ghoul2, ent->playerModel, ent->crotchBolt, &matrix, G2Angles, ent->client->ps.origin,
		(cg.time?cg.time:level.time), NULL, ent->s.modelScale);
	gi.G2API_GiveMeVectorFromMatrix(matrix, NEGATIVE_Z, dir);

	if (dir[2] > 0.1f)
	{ //facing up
		return BOTH_DEADFLOP2;
	}
	else
	{ //facing down
		return BOTH_DEADFLOP1;
	}
}

static inline qboolean G_RagWantsHumanoidsOnly( CGhoul2Info *ghlInfo )
{
	char	*GLAName;
	GLAName = gi.G2API_GetGLAName( ghlInfo );
	assert(GLAName);

	if ( !Q_stricmp( "models/players/_humanoid/_humanoid", GLAName ) )
	{//only _humanoid skeleton is expected to have these
		return qtrue;
	}

	return qfalse;
}

//rww - game interface for the ragdoll stuff.
//Returns qtrue if the entity is now in a ragdoll state, otherwise qfalse.
//(ported from MP's CG version)

qboolean G_RagDoll(gentity_t *ent, vec3_t forcedAngles)
{
	vec3_t G2Angles;
	vec3_t usedOrg;
	qboolean inSomething = qfalse;
	int ragAnim;
	//int ragVar = gi.Cvar_VariableIntegerValue("broadsword");
	int ragVar = g_broadsword->integer;

	if (!ragVar)
	{
		return qfalse;
	}

	if (!ent ||
		!ent->inuse ||
		!ent->client ||
		ent->health > 0 ||
		ent->client->noRagTime >= level.time ||
		ent->client->noRagTime==-1 ||
		(ent->s.powerups & (1 << PW_DISRUPTION)) ||
		!ent->e_DieFunc ||
 		ent->playerModel < 0 ||
		!ent->ghoul2.size() ||
		!G_RagWantsHumanoidsOnly(&ent->ghoul2[ent->playerModel])
		)
	{
		return qfalse;
	}

	VectorCopy(forcedAngles, G2Angles);
	//forcedAngles[0] = forcedAngles[2] = 0;

	if (ent->client->ps.heldByClient <= ENTITYNUM_WORLD)
	{
		gentity_t *grabbedBy = &g_entities[ent->client->ps.heldByClient];

		if (grabbedBy->inuse && grabbedBy->client &&
			grabbedBy->ghoul2.size())
		{
			G_BodyDragUpdate(ent, grabbedBy);
		}
	}

	//--FIXME: do not go into ragdoll if in a spinning death anim or something, it really messes things up
	//rww 12/17/02 - should be ok now actually with my new stuff

	VectorCopy(ent->client->ps.origin, usedOrg);

	if (!ent->client->isRagging)
	{ //If we're not in a ragdoll state, perform the checks.
		if (ent->client->ps.heldByClient <= ENTITYNUM_WORLD)
		{ //want to rag no matter what then
			inSomething = qtrue;
		}
		else if (ent->client->ps.groundEntityNum == ENTITYNUM_NONE)
		{
			vec3_t cVel;

			VectorCopy(ent->client->ps.velocity, cVel);

			if (VectorNormalize(cVel) > 400)
			{ //if he's flying through the air at a good enough speed, switch into ragdoll
				inSomething = qtrue;
			}
		}

		if (ragVar > 1)
		{ //go into rag instantly upon death
			inSomething = qtrue;

			//also shove them a tad so they don't just collapse onto the floor
			//VectorScale(ent->client->ps.velocity, 1.3f, ent->client->ps.velocity);
			ent->client->ps.velocity[2] += 32;
		}

		if (!inSomething)
		{
			int i = 0;
			int boltChecks[5];
			vec3_t boltPoints[5];
			vec3_t trStart, trEnd;
			vec3_t tAng;
			//qboolean deathDone = qfalse;
			trace_t tr;
			mdxaBone_t boltMatrix;

			VectorSet( tAng, 0, ent->client->ps.viewangles[YAW], 0 );

			if (ent->client->ps.legsAnimTimer <= 0)
			{ //Looks like the death anim is done playing
				//deathDone = qtrue;
			}

			//if (deathDone)
			if (1)
			{ //only trace from the hands if the death anim is already done.
				boltChecks[0] = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "rhand");
				boltChecks[1] = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "lhand");
			}
			else
			{ //otherwise start the trace loop at the cranium.
				i = 2;
			}
			boltChecks[2] = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "cranium");
			boltChecks[3] = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "rtalus");
			boltChecks[4] = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "ltalus");

			//Do the head first, because the hands reference it anyway.
			gi.G2API_GetBoltMatrix(ent->ghoul2, ent->playerModel, boltChecks[2], &boltMatrix, tAng, ent->client->ps.origin, (cg.time?cg.time:level.time), NULL, ent->s.modelScale);
			gi.G2API_GiveMeVectorFromMatrix(boltMatrix, ORIGIN, boltPoints[2]);

			while (i < 5)
			{
				if (i < 2)
				{ //when doing hands, trace to the head instead of origin
					gi.G2API_GetBoltMatrix(ent->ghoul2, ent->playerModel, boltChecks[i], &boltMatrix, tAng, ent->client->ps.origin, (cg.time?cg.time:level.time), NULL, ent->s.modelScale);
					gi.G2API_GiveMeVectorFromMatrix(boltMatrix, ORIGIN, boltPoints[i]);
					VectorCopy(boltPoints[i], trStart);
					VectorCopy(boltPoints[2], trEnd);
				}
				else
				{
					if (i > 2)
					{ //2 is the head, which already has the bolt point.
						gi.G2API_GetBoltMatrix(ent->ghoul2, ent->playerModel, boltChecks[i], &boltMatrix, tAng, ent->client->ps.origin, (cg.time?cg.time:level.time), NULL, ent->s.modelScale);
						gi.G2API_GiveMeVectorFromMatrix(boltMatrix, ORIGIN, boltPoints[i]);
					}
					VectorCopy(boltPoints[i], trStart);
					VectorCopy(ent->client->ps.origin, trEnd);
				}

				//Now that we have all that sorted out, trace between the two points we desire.
				gi.trace(&tr, trStart, NULL, NULL, trEnd, ent->s.number, MASK_SOLID, (EG2_Collision)0, 0);

				if (tr.fraction != 1.0 || tr.startsolid || tr.allsolid)
				{ //Hit something or start in solid, so flag it and break.
					//This is a slight hack, but if we aren't done with the death anim, we don't really want to
					//go into ragdoll unless our body has a relatively "flat" pitch.
#if 0
					vec3_t vSub;

					//Check the pitch from the head to the right foot (should be reasonable)
					VectorSubtract(boltPoints[2], boltPoints[3], vSub);
					VectorNormalize(vSub);
					vectoangles(vSub, vSub);

					if (deathDone || (vSub[PITCH] < 50 && vSub[PITCH] > -50))
					{
						inSomething = qtrue;
					}
#else
					inSomething = qtrue;
#endif
					break;
				}

				i++;
			}
		}

		if (inSomething)
		{
			/*
			PM_SetTorsoAnimTimer( ent, &ent->client->ps.torsoAnimTimer, 0 );
			PM_SetLegsAnimTimer( ent, &ent->client->ps.legsAnimTimer, 0 );

			PM_SetAnimFinal(&ent->client->ps.torsoAnim,&ent->client->ps.legsAnim,SETANIM_BOTH,ragAnim,SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD,
				&ent->client->ps.torsoAnimTimer,&ent->client->ps.legsAnimTimer,ent,500);
			*/
			ent->client->isRagging = qtrue;
		}
	}

	if (ent->client->isRagging)
	{ //We're in a ragdoll state, so make the call to keep our positions updated and whatnot.
		CRagDollParams tParms;
		CGameRagDollUpdateParams tuParms;

		ragAnim = G_RagAnimForPositioning(ent);

		/*
		if (ent->ikStatus)
		{ //ik must be reset before ragdoll is started, or you'll get some interesting results.
			trap_G2API_SetBoneIKState(cent->ghoul2, cg.time, NULL, IKS_NONE, NULL);
			cent->ikStatus = qfalse;
		}
		*/

		//these will be used as "base" frames for the ragoll settling.
		tParms.startFrame = level.knownAnimFileSets[ent->client->clientInfo.animFileIndex].animations[ragAnim].firstFrame;
		tParms.endFrame = level.knownAnimFileSets[ent->client->clientInfo.animFileIndex].animations[ragAnim].firstFrame+level.knownAnimFileSets[ent->client->clientInfo.animFileIndex].animations[ragAnim].numFrames;
#if 1
		{
			float currentFrame;
			int startFrame, endFrame;
			int flags;
			float animSpeed;

			if (gi.G2API_GetBoneAnim(&ent->ghoul2[0], "model_root", (cg.time?cg.time:level.time), &currentFrame, &startFrame, &endFrame, &flags, &animSpeed, NULL))
			{ //lock the anim on the current frame.
				int blendTime = 500;

				gi.G2API_SetBoneAnim(&ent->ghoul2[0], "lower_lumbar", currentFrame, currentFrame+1, flags, animSpeed,(cg.time?cg.time:level.time), currentFrame, blendTime);
				gi.G2API_SetBoneAnim(&ent->ghoul2[0], "model_root", currentFrame, currentFrame+1, flags, animSpeed, (cg.time?cg.time:level.time), currentFrame, blendTime);
				gi.G2API_SetBoneAnim(&ent->ghoul2[0], "Motion", currentFrame, currentFrame+1, flags, animSpeed, (cg.time?cg.time:level.time), currentFrame, blendTime);
			}
		}
#endif

		gi.G2API_SetBoneAngles( &ent->ghoul2[ent->playerModel], "upper_lumbar", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 100, (cg.time?cg.time:level.time) );
		gi.G2API_SetBoneAngles( &ent->ghoul2[ent->playerModel], "lower_lumbar", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 100, (cg.time?cg.time:level.time) );
		gi.G2API_SetBoneAngles( &ent->ghoul2[ent->playerModel], "thoracic", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 100, (cg.time?cg.time:level.time) );
		gi.G2API_SetBoneAngles( &ent->ghoul2[ent->playerModel], "cervical", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 100, (cg.time?cg.time:level.time) );

		VectorCopy(G2Angles, tParms.angles);
		VectorCopy(usedOrg, tParms.position);
		VectorCopy(ent->s.modelScale, tParms.scale);
		tParms.me = ent->s.number;
		tParms.groundEnt = ent->client->ps.groundEntityNum;

		tParms.collisionType = 1;
		tParms.RagPhase=CRagDollParams::RP_DEATH_COLLISION;
		tParms.fShotStrength = 4;

		gi.G2API_SetRagDoll(ent->ghoul2, &tParms);


		tuParms.hasEffectorData = qfalse;
		VectorClear(tuParms.effectorTotal);

		VectorCopy(G2Angles, tuParms.angles);
		VectorCopy(usedOrg, tuParms.position);
		VectorCopy(ent->s.modelScale, tuParms.scale);
		tuParms.me = ent->s.number;
		tuParms.settleFrame = tParms.endFrame-1;
		tuParms.groundEnt = ent->client->ps.groundEntityNum;

		if (ent->client->ps.groundEntityNum != ENTITYNUM_NONE)
		{
			VectorClear(tuParms.velocity);
		}
		else
		{
			VectorScale(ent->client->ps.velocity, 0.4f, tuParms.velocity);
		}

		gi.G2API_AnimateG2Models(ent->ghoul2, (cg.time?cg.time:level.time), &tuParms);

		if (ent->client->ps.heldByClient <= ENTITYNUM_WORLD)
		{
			gentity_t *grabEnt;

			grabEnt = &g_entities[ent->client->ps.heldByClient];

			if (grabEnt->client && grabEnt->ghoul2.size())
			{
				vec3_t bOrg;
				vec3_t thisHand;
				vec3_t hands;
				vec3_t pcjMin, pcjMax;
				vec3_t pDif;
				vec3_t thorPoint;
				float difLen;

				//Get the person who is holding our hand's hand location
				//gi.G2API_GetBoltMatrix(grabEnt->ghoul2, 0, grabEnt->gent->client->renderInfo.handRPoint, &matrix, grabEnt->turAngles, grabEnt->lerpOrigin,
				//	cg.time, cgs.gameModels, grabEnt->modelScale);
				//BG_GiveMeVectorFromMatrix(&matrix, ORIGIN, bOrg);
				VectorCopy(grabEnt->client->renderInfo.handRPoint, bOrg);

				//Get our hand's location
				//trap_G2API_GetBoltMatrix(cent->ghoul2, 0, 0, &matrix, cent->turAngles, cent->lerpOrigin,
				//	cg.time, cgs.gameModels, cent->modelScale);
				//BG_GiveMeVectorFromMatrix(&matrix, ORIGIN, thisHand);
				VectorCopy(ent->client->renderInfo.handRPoint, thisHand);

				//Get the position of the thoracic bone for hinting its velocity later on
				//thorBolt = trap_G2API_AddBolt(cent->ghoul2, 0, "thoracic");
				//trap_G2API_GetBoltMatrix(cent->ghoul2, 0, thorBolt, &matrix, cent->turAngles, cent->lerpOrigin,
				//	cg.time, cgs.gameModels, cent->modelScale);
				//BG_GiveMeVectorFromMatrix(&matrix, ORIGIN, thorPoint);
				VectorCopy(ent->client->renderInfo.torsoPoint, thorPoint);

				VectorSubtract(bOrg, thisHand, hands);

				if (VectorLength(hands) < 3.0f)
				{
					gi.G2API_RagForceSolve(ent->ghoul2, qfalse);
				}
				else
				{
					gi.G2API_RagForceSolve(ent->ghoul2, qtrue);
				}

				//got the hand pos of him, now we want to make our hand go to it
				gi.G2API_RagEffectorGoal(ent->ghoul2, "rhand", bOrg);
				gi.G2API_RagEffectorGoal(ent->ghoul2, "rradius", bOrg);
				gi.G2API_RagEffectorGoal(ent->ghoul2, "rradiusX", bOrg);
				gi.G2API_RagEffectorGoal(ent->ghoul2, "rhumerusX", bOrg);
				gi.G2API_RagEffectorGoal(ent->ghoul2, "rhumerus", bOrg);

				//Make these two solve quickly so we can update decently
				gi.G2API_RagPCJGradientSpeed(ent->ghoul2, "rhumerus", 1.5f);
				gi.G2API_RagPCJGradientSpeed(ent->ghoul2, "rradius", 1.5f);

				//Break the constraints on them I suppose
				VectorSet(pcjMin, -999, -999, -999);
				VectorSet(pcjMax, 999, 999, 999);
				gi.G2API_RagPCJConstraint(ent->ghoul2, "rhumerus", pcjMin, pcjMax);
				gi.G2API_RagPCJConstraint(ent->ghoul2, "rradius", pcjMin, pcjMax);

				ent->client->overridingBones = level.time + 2000;

				//hit the thoracic velocity to the hand point
				VectorSubtract(bOrg, thorPoint, hands);
				VectorNormalize(hands);
				VectorScale(hands, 2048.0f, hands);
				gi.G2API_RagEffectorKick(ent->ghoul2, "thoracic", hands);
				gi.G2API_RagEffectorKick(ent->ghoul2, "ceyebrow", hands);

				VectorSubtract(ent->client->ragLastOrigin, ent->client->ps.origin, pDif);
				VectorCopy(ent->client->ps.origin, ent->client->ragLastOrigin);

				if (ent->client->ragLastOriginTime >= level.time && ent->client->ps.groundEntityNum != ENTITYNUM_NONE)
				{ //make sure it's reasonably updated
					difLen = VectorLength(pDif);
					if (difLen > 0.0f)
					{ //if we're being dragged, then kick all the bones around a bit
						vec3_t dVel;
						vec3_t rVel;
						int i = 0;

						if (difLen < 12.0f)
						{
							VectorScale(pDif, 12.0f/difLen, pDif);
							difLen = 12.0f;
						}

						while (g_effectorStringTable[i])
						{
							VectorCopy(pDif, dVel);
							dVel[2] = 0;

							//Factor in a random velocity
							VectorSet(rVel, Q_flrand(-0.1f, 0.1f), Q_flrand(-0.1f, 0.1f), Q_flrand(0.1f, 0.5));
							VectorScale(rVel, 8.0f, rVel);

							VectorAdd(dVel, rVel, dVel);
							VectorScale(dVel, 10.0f, dVel);

							gi.G2API_RagEffectorKick(ent->ghoul2, g_effectorStringTable[i], dVel);

							i++;
						}
					}
				}
				ent->client->ragLastOriginTime = level.time + 1000;
			}
		}
		else if (ent->client->overridingBones)
		{ //reset things to their normal rag state
			vec3_t pcjMin, pcjMax;
			vec3_t dVel;

			//got the hand pos of him, now we want to make our hand go to it
			gi.G2API_RagEffectorGoal(ent->ghoul2, "rhand", NULL);
			gi.G2API_RagEffectorGoal(ent->ghoul2, "rradius", NULL);
			gi.G2API_RagEffectorGoal(ent->ghoul2, "rradiusX", NULL);
			gi.G2API_RagEffectorGoal(ent->ghoul2, "rhumerusX", NULL);
			gi.G2API_RagEffectorGoal(ent->ghoul2, "rhumerus", NULL);

			VectorSet(dVel, 0.0f, 0.0f, -64.0f);
			gi.G2API_RagEffectorKick(ent->ghoul2, "rhand", dVel);

			gi.G2API_RagPCJGradientSpeed(ent->ghoul2, "rhumerus", 0.0f);
			gi.G2API_RagPCJGradientSpeed(ent->ghoul2, "rradius", 0.0f);

			VectorSet(pcjMin,-100.0f,-40.0f,-15.0f);
			VectorSet(pcjMax,-15.0f,80.0f,15.0f);
			gi.G2API_RagPCJConstraint(ent->ghoul2, "rhumerus", pcjMin, pcjMax);

			VectorSet(pcjMin,-25.0f,-20.0f,-20.0f);
			VectorSet(pcjMax,90.0f,20.0f,-20.0f);
			gi.G2API_RagPCJConstraint(ent->ghoul2, "rradius", pcjMin, pcjMax);

			if (ent->client->overridingBones < level.time)
			{
				gi.G2API_RagForceSolve(ent->ghoul2, qfalse);
				ent->client->overridingBones = 0;
			}
			else
			{
				gi.G2API_RagForceSolve(ent->ghoul2, qtrue);
			}
		}

		if (tuParms.hasEffectorData)
		{
			/*
			vec3_t efDir;
			vec3_t existingVelocity;
			float evValue = 0;

			VectorCopy(tuParms.effectorTotal, efDir);
			VectorNormalize(efDir);
			VectorCopy(ent->client->ps.velocity, existingVelocity);
			evValue = VectorNormalize(existingVelocity);

			if (evValue < 32)
			{
				ent->client->ps.velocity[0] += efDir[0]*64;
				ent->client->ps.velocity[1] += efDir[1]*64;
				ent->client->ps.velocity[2] += efDir[2]*64;
			}
			*/

			VectorNormalize(tuParms.effectorTotal);
			VectorScale(tuParms.effectorTotal, 7.0f, tuParms.effectorTotal);

			VectorAdd(ent->client->ps.velocity, tuParms.effectorTotal, ent->client->ps.velocity);
		}

		return qtrue;
	}

	return qfalse;
}
//rww - RAGDOLL_END


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

	//ResetTeamCounters();
	NAV::DecayDangerSenses();
	Rail_Update();
	Troop_Update();
	Pilot_Update();


	if (player && gi.WE_IsShaking(player->currentOrigin))
	{
 	  	CGCam_Shake(0.45f, 100);
	}


	AI_UpdateGroups();




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
			}
			/*	// This is never set to true anywhere. Killing the field (BTO - VV)
			else if ( ent->unlinkAfterEvent ) {
				// items that will respawn will hide themselves after their pickup event
				ent->unlinkAfterEvent = qfalse;
				gi.unlinkentity( ent );
			}
			*/
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
			// FIXME string comparison in per-frame thinks wut???
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
			NAV::GetNearestNode( ent );


			if( ent->m_iIcarusID != IIcarusInterface::ICARUS_INVALID && !stop_icarus )
			{
				IIcarusInterface::GetIcarus()->Update( ent->m_iIcarusID );
			}
			//dead
			if ( ent->health <= 0 )
			{
				if ( ent->client->ps.groundEntityNum != ENTITYNUM_NONE )
				{//on the ground
					pitch_roll_for_slope( ent );
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
	NAV::ShowDebugInfo(ent->currentOrigin, ent->waypoint);
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

extern int delayedShutDown;
	if ( g_delayedShutdown->integer && delayedShutDown != 0 && delayedShutDown < level.time )
	{
		assert(0);
		G_Error( "Game Errors. Scroll up the console to read them.\n" );
	}

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


/*
void PrintEntClassname( int gentNum )
{
	Com_Printf( "%d: %s in snapshot\n", gentNum, g_entities[gentNum].classname );
}
*/
IGhoul2InfoArray &TheGameGhoul2InfoArray()
{
	return gi.TheGhoul2InfoArray();
}
