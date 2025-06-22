/*
===========================================================================
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

// ICARUS Engine Interface File
//
//	This file is the only section of the ICARUS systems that
//	is not directly portable from engine to engine.
//
//	-- jweier

#include "g_headers.h"

#include "g_local.h"
#include "g_functions.h"
#include "Q3_Interface.h"
#include "Q3_Registers.h"
#include "anims.h"
#include "b_local.h"
#include "events.h"
#include "g_nav.h"
#include "../cgame/cg_camera.h"
#include "../game/objectives.h"
#include "g_roff.h"
#include "../cgame/cg_local.h"
#include "g_icarus.h"
#include "wp_saber.h"

#ifndef _WIN32
#include <cstdlib>
#endif

extern int ICARUS_LinkEntity( int entID, CSequencer *sequencer, CTaskManager *taskManager );

extern void InitMover( gentity_t *ent );
extern void MatchTeam( gentity_t *teamLeader, int moverState, int time );
//extern void SetMoverState( gentity_t *ent, moverState_t moverState, int time );
extern void ChangeWeapon( gentity_t *ent, int newWeapon );
extern char *G_GetLocationForEnt( gentity_t *ent );
extern void NPC_BSSearchStart( int homeWp, bState_t bState );
extern void InitMoverTrData( gentity_t *ent );
extern qboolean SpotWouldTelefrag2( gentity_t *mover, vec3_t dest );
extern cvar_t *g_sex;
extern cvar_t *g_timescale;
extern void G_SetEnemy( gentity_t *self, gentity_t *enemy );
//extern void FX_BorgTeleport( vec3_t org );
static void Q3_SetWeapon (int entID, const char *wp_name);
static void Q3_SetItem (int entID, const char *item_name);
extern void CG_ChangeWeapon( int num );
extern int	TAG_GetOrigin2( const char *owner, const char *name, vec3_t origin );
extern void G_TeamRetaliation ( gentity_t *targ, gentity_t *attacker, qboolean stopIcarus );
extern void G_PlayDoorLoopSound( gentity_t *ent );
extern void G_PlayDoorSound( gentity_t *ent, int type );
extern void NPC_SetLookTarget( gentity_t *self, int entNum, int clearTime );
extern void NPC_ClearLookTarget( gentity_t *self );
qboolean PM_HasAnimation( gentity_t *ent, int animation );
extern int	BMS_START;
extern int	BMS_MID;
extern int	BMS_END;
extern cvar_t	*g_skippingcin;

extern qboolean	stop_icarus;

#define stringIDExpand(str, strEnum)	{ str, strEnum }, ENUM2STRING(strEnum)
//#define stringIDExpand(str, strEnum)	str,strEnum

/*
stringID_table_t tagsTable [] =
{
}
*/

stringID_table_t BSTable[] =
{
	ENUM2STRING(BS_DEFAULT),//# default behavior for that NPC
	ENUM2STRING(BS_ADVANCE_FIGHT),//# Advance to captureGoal and shoot enemies if you can
	ENUM2STRING(BS_SLEEP),//# Play awake script when startled by sound
	ENUM2STRING(BS_FOLLOW_LEADER),//# Follow your leader and shoot any enemies you come across
	ENUM2STRING(BS_JUMP),//# Face navgoal and jump to it.
	ENUM2STRING(BS_SEARCH),//# Using current waypoint as a base), search the immediate branches of waypoints for enemies
	ENUM2STRING(BS_WANDER),//# Wander down random waypoint paths
	ENUM2STRING(BS_NOCLIP),//# Moves through walls), etc.
	ENUM2STRING(BS_REMOVE),//# Waits for player to leave PVS then removes itself
	ENUM2STRING(BS_CINEMATIC),//# Does nothing but face it's angles and move to a goal if it has one
	//the rest are internal only
	{"",				-1},
};



stringID_table_t BSETTable[] =
{
	ENUM2STRING(BSET_SPAWN),//# script to use when first spawned
	ENUM2STRING(BSET_USE),//# script to use when used
	ENUM2STRING(BSET_AWAKE),//# script to use when awoken/startled
	ENUM2STRING(BSET_ANGER),//# script to use when aquire an enemy
	ENUM2STRING(BSET_ATTACK),//# script to run when you attack
	ENUM2STRING(BSET_VICTORY),//# script to run when you kill someone
	ENUM2STRING(BSET_LOSTENEMY),//# script to run when you can't find your enemy
	ENUM2STRING(BSET_PAIN),//# script to use when take pain
	ENUM2STRING(BSET_FLEE),//# script to use when take pain below 50% of health
	ENUM2STRING(BSET_DEATH),//# script to use when killed
	ENUM2STRING(BSET_DELAYED),//# script to run when self->delayScriptTime is reached
	ENUM2STRING(BSET_BLOCKED),//# script to run when blocked by a friendly NPC or player
	ENUM2STRING(BSET_BUMPED),//# script to run when bumped into a friendly NPC or player (can set bumpRadius)
	ENUM2STRING(BSET_STUCK),//# script to run when blocked by a wall
	ENUM2STRING(BSET_FFIRE),//# script to run when player shoots their own teammates
	ENUM2STRING(BSET_FFDEATH),//# script to run when player kills a teammate
	stringIDExpand("", BSET_INVALID),
	{"",				-1},
};

stringID_table_t WPTable[] =
{
	{"NULL", WP_NONE},
	ENUM2STRING(WP_NONE),
	// Player weapons
	ENUM2STRING(WP_SABER),				 // NOTE: lots of code assumes this is the first weapon (... which is crap) so be careful -Ste.
	ENUM2STRING(WP_BRYAR_PISTOL),
	ENUM2STRING(WP_BLASTER),
	ENUM2STRING(WP_DISRUPTOR),
	ENUM2STRING(WP_BOWCASTER),
	ENUM2STRING(WP_REPEATER),
	ENUM2STRING(WP_DEMP2),
	ENUM2STRING(WP_FLECHETTE),
	ENUM2STRING(WP_ROCKET_LAUNCHER),
	ENUM2STRING(WP_THERMAL),
	ENUM2STRING(WP_TRIP_MINE),
	ENUM2STRING(WP_DET_PACK),
	ENUM2STRING(WP_STUN_BATON),
	//NOTE: player can only have up to 16 weapons), anything after that is enemy only
	ENUM2STRING(WP_MELEE),			// Any ol' melee attack
	// NPC enemy weapons
	ENUM2STRING(WP_EMPLACED_GUN),
	ENUM2STRING(WP_BOT_LASER),		// Probe droid	- Laser blast
	ENUM2STRING(WP_TURRET),			// turret guns
	ENUM2STRING(WP_ATST_MAIN),
	ENUM2STRING(WP_ATST_SIDE),
	ENUM2STRING(WP_TIE_FIGHTER),
	ENUM2STRING(WP_RAPID_FIRE_CONC),
	ENUM2STRING(WP_BLASTER_PISTOL),	// apparently some enemy only version of the blaster
	{"", 0}
};

stringID_table_t INVTable[] =
{
	ENUM2STRING(INV_ELECTROBINOCULARS),
	ENUM2STRING(INV_BACTA_CANISTER),
	ENUM2STRING(INV_SEEKER),
	ENUM2STRING(INV_LIGHTAMP_GOGGLES),
	ENUM2STRING(INV_SENTRY),
	{"", 0}
};

stringID_table_t eventTable[] =
{
	//BOTH_h
	//END
	{"",				EV_BAD},
};

stringID_table_t DMSTable[] =
{
	{"NULL",-1},
	ENUM2STRING(DM_AUTO),	//# let the game determine the dynamic music as normal
	ENUM2STRING(DM_SILENCE),	//# stop the music
	ENUM2STRING(DM_EXPLORE),	//# force the exploration music to play
	ENUM2STRING(DM_ACTION),	//# force the action music to play
	ENUM2STRING(DM_BOSS),	//# force the boss battle music to play (if there is any)
	ENUM2STRING(DM_DEATH),	//# force the "player dead" music to play
	{"", -1}
};

stringID_table_t setTable[] =
{
	ENUM2STRING(SET_SPAWNSCRIPT),//0
	ENUM2STRING(SET_USESCRIPT),
	ENUM2STRING(SET_AWAKESCRIPT),
	ENUM2STRING(SET_ANGERSCRIPT),
	ENUM2STRING(SET_ATTACKSCRIPT),
	ENUM2STRING(SET_VICTORYSCRIPT),
	ENUM2STRING(SET_PAINSCRIPT),
	ENUM2STRING(SET_FLEESCRIPT),
	ENUM2STRING(SET_DEATHSCRIPT),
	ENUM2STRING(SET_DELAYEDSCRIPT),
	ENUM2STRING(SET_BLOCKEDSCRIPT),
	ENUM2STRING(SET_FFIRESCRIPT),
	ENUM2STRING(SET_FFDEATHSCRIPT),
	ENUM2STRING(SET_MINDTRICKSCRIPT),
	ENUM2STRING(SET_NO_MINDTRICK),
	ENUM2STRING(SET_ORIGIN),
	ENUM2STRING(SET_TELEPORT_DEST),
	ENUM2STRING(SET_ANGLES),
	ENUM2STRING(SET_XVELOCITY),
	ENUM2STRING(SET_YVELOCITY),
	ENUM2STRING(SET_ZVELOCITY),
	ENUM2STRING(SET_Z_OFFSET),
	ENUM2STRING(SET_ENEMY),
	ENUM2STRING(SET_LEADER),
	ENUM2STRING(SET_NAVGOAL),
	ENUM2STRING(SET_ANIM_UPPER),
	ENUM2STRING(SET_ANIM_LOWER),
	ENUM2STRING(SET_ANIM_BOTH),
	ENUM2STRING(SET_ANIM_HOLDTIME_LOWER),
	ENUM2STRING(SET_ANIM_HOLDTIME_UPPER),
	ENUM2STRING(SET_ANIM_HOLDTIME_BOTH),
	ENUM2STRING(SET_PLAYER_TEAM),
	ENUM2STRING(SET_ENEMY_TEAM),
	ENUM2STRING(SET_BEHAVIOR_STATE),
	ENUM2STRING(SET_BEHAVIOR_STATE),
	ENUM2STRING(SET_HEALTH),
	ENUM2STRING(SET_ARMOR),
	ENUM2STRING(SET_DEFAULT_BSTATE),
	ENUM2STRING(SET_CAPTURE),
	ENUM2STRING(SET_DPITCH),
	ENUM2STRING(SET_DYAW),
	ENUM2STRING(SET_EVENT),
	ENUM2STRING(SET_TEMP_BSTATE),
	ENUM2STRING(SET_COPY_ORIGIN),
	ENUM2STRING(SET_VIEWTARGET),
	ENUM2STRING(SET_WEAPON),
	ENUM2STRING(SET_ITEM),
	ENUM2STRING(SET_WALKSPEED),
	ENUM2STRING(SET_RUNSPEED),
	ENUM2STRING(SET_YAWSPEED),
	ENUM2STRING(SET_AGGRESSION),
	ENUM2STRING(SET_AIM),
	ENUM2STRING(SET_FRICTION),
	ENUM2STRING(SET_GRAVITY),
	ENUM2STRING(SET_IGNOREPAIN),
	ENUM2STRING(SET_IGNOREENEMIES),
	ENUM2STRING(SET_IGNOREALERTS),
	ENUM2STRING(SET_DONTSHOOT),
	ENUM2STRING(SET_DONTFIRE),
	ENUM2STRING(SET_LOCKED_ENEMY),
	ENUM2STRING(SET_NOTARGET),
	ENUM2STRING(SET_LEAN),
	ENUM2STRING(SET_CROUCHED),
	ENUM2STRING(SET_WALKING),
	ENUM2STRING(SET_RUNNING),
	ENUM2STRING(SET_CHASE_ENEMIES),
	ENUM2STRING(SET_LOOK_FOR_ENEMIES),
	ENUM2STRING(SET_FACE_MOVE_DIR),
	ENUM2STRING(SET_ALT_FIRE),
	ENUM2STRING(SET_DONT_FLEE),
	ENUM2STRING(SET_FORCED_MARCH),
	ENUM2STRING(SET_NO_RESPONSE),
	ENUM2STRING(SET_NO_COMBAT_TALK),
	ENUM2STRING(SET_NO_ALERT_TALK),
	ENUM2STRING(SET_UNDYING),
	ENUM2STRING(SET_TREASONED),
	ENUM2STRING(SET_DISABLE_SHADER_ANIM),
	ENUM2STRING(SET_SHADER_ANIM),
	ENUM2STRING(SET_INVINCIBLE),
	ENUM2STRING(SET_NOAVOID),
	ENUM2STRING(SET_SHOOTDIST),
	ENUM2STRING(SET_TARGETNAME),
	ENUM2STRING(SET_TARGET),
	ENUM2STRING(SET_TARGET2),
	ENUM2STRING(SET_LOCATION),
	ENUM2STRING(SET_PAINTARGET),
	ENUM2STRING(SET_TIMESCALE),
	ENUM2STRING(SET_VISRANGE),
	ENUM2STRING(SET_EARSHOT),
	ENUM2STRING(SET_VIGILANCE),
	ENUM2STRING(SET_HFOV),
	ENUM2STRING(SET_VFOV),
	ENUM2STRING(SET_DELAYSCRIPTTIME),
	ENUM2STRING(SET_FORWARDMOVE),
	ENUM2STRING(SET_RIGHTMOVE),
	ENUM2STRING(SET_LOCKYAW),
	ENUM2STRING(SET_SOLID),
	ENUM2STRING(SET_CAMERA_GROUP),
	ENUM2STRING(SET_CAMERA_GROUP_Z_OFS),
	ENUM2STRING(SET_CAMERA_GROUP_TAG),
	ENUM2STRING(SET_LOOK_TARGET),
	ENUM2STRING(SET_ADDRHANDBOLT_MODEL),
	ENUM2STRING(SET_REMOVERHANDBOLT_MODEL),
	ENUM2STRING(SET_ADDLHANDBOLT_MODEL),
	ENUM2STRING(SET_REMOVELHANDBOLT_MODEL),
	ENUM2STRING(SET_FACEAUX),
	ENUM2STRING(SET_FACEBLINK),
	ENUM2STRING(SET_FACEBLINKFROWN),
	ENUM2STRING(SET_FACEFROWN),
	ENUM2STRING(SET_FACENORMAL),
	ENUM2STRING(SET_FACEEYESCLOSED),
	ENUM2STRING(SET_FACEEYESOPENED),
	ENUM2STRING(SET_SCROLLTEXT),
	ENUM2STRING(SET_LCARSTEXT),
	ENUM2STRING(SET_SCROLLTEXTCOLOR),
	ENUM2STRING(SET_CAPTIONTEXTCOLOR),
	ENUM2STRING(SET_CENTERTEXTCOLOR),
	ENUM2STRING(SET_PLAYER_USABLE),
	ENUM2STRING(SET_STARTFRAME),
	ENUM2STRING(SET_ENDFRAME),
	ENUM2STRING(SET_ANIMFRAME),
	ENUM2STRING(SET_LOOP_ANIM),
	ENUM2STRING(SET_INTERFACE),
	ENUM2STRING(SET_SHIELDS),
	ENUM2STRING(SET_NO_KNOCKBACK),
	ENUM2STRING(SET_INVISIBLE),
	ENUM2STRING(SET_VAMPIRE),
	ENUM2STRING(SET_FORCE_INVINCIBLE),
	ENUM2STRING(SET_GREET_ALLIES),
	ENUM2STRING(SET_PLAYER_LOCKED),
	ENUM2STRING(SET_LOCK_PLAYER_WEAPONS),
	ENUM2STRING(SET_NO_IMPACT_DAMAGE),
	ENUM2STRING(SET_PARM1),
	ENUM2STRING(SET_PARM2),
	ENUM2STRING(SET_PARM3),
	ENUM2STRING(SET_PARM4),
	ENUM2STRING(SET_PARM5),
	ENUM2STRING(SET_PARM6),
	ENUM2STRING(SET_PARM7),
	ENUM2STRING(SET_PARM8),
	ENUM2STRING(SET_PARM9),
	ENUM2STRING(SET_PARM10),
	ENUM2STRING(SET_PARM11),
	ENUM2STRING(SET_PARM12),
	ENUM2STRING(SET_PARM13),
	ENUM2STRING(SET_PARM14),
	ENUM2STRING(SET_PARM15),
	ENUM2STRING(SET_PARM16),
	ENUM2STRING(SET_DEFEND_TARGET),
	ENUM2STRING(SET_WAIT),
	ENUM2STRING(SET_COUNT),
	ENUM2STRING(SET_SHOT_SPACING),
	ENUM2STRING(SET_VIDEO_PLAY),
	ENUM2STRING(SET_VIDEO_FADE_IN),
	ENUM2STRING(SET_VIDEO_FADE_OUT),
	ENUM2STRING(SET_REMOVE_TARGET),
	ENUM2STRING(SET_LOADGAME),
	ENUM2STRING(SET_MENU_SCREEN),
	ENUM2STRING(SET_OBJECTIVE_SHOW),
	ENUM2STRING(SET_OBJECTIVE_HIDE),
	ENUM2STRING(SET_OBJECTIVE_SUCCEEDED),
	ENUM2STRING(SET_OBJECTIVE_FAILED),
	ENUM2STRING(SET_MISSIONFAILED),
	ENUM2STRING(SET_TACTICAL_SHOW),
	ENUM2STRING(SET_TACTICAL_HIDE),
	ENUM2STRING(SET_FOLLOWDIST),
	ENUM2STRING(SET_SCALE),
	ENUM2STRING(SET_OBJECTIVE_CLEARALL),
	ENUM2STRING(SET_MISSIONSTATUSTEXT),
	ENUM2STRING(SET_WIDTH),
	ENUM2STRING(SET_CLOSINGCREDITS),
	ENUM2STRING(SET_SKILL),
	ENUM2STRING(SET_MISSIONSTATUSTIME),
	ENUM2STRING(SET_FULLNAME),
	ENUM2STRING(SET_FORCE_HEAL_LEVEL),
	ENUM2STRING(SET_FORCE_JUMP_LEVEL),
	ENUM2STRING(SET_FORCE_SPEED_LEVEL),
	ENUM2STRING(SET_FORCE_PUSH_LEVEL),
	ENUM2STRING(SET_FORCE_PULL_LEVEL),
	ENUM2STRING(SET_FORCE_MINDTRICK_LEVEL),
	ENUM2STRING(SET_FORCE_GRIP_LEVEL),
	ENUM2STRING(SET_FORCE_LIGHTNING_LEVEL),
	ENUM2STRING(SET_SABER_THROW),
	ENUM2STRING(SET_SABER_DEFENSE),
	ENUM2STRING(SET_SABER_OFFENSE),
	ENUM2STRING(SET_VIEWENTITY),
	ENUM2STRING(SET_WATCHTARGET),
	ENUM2STRING(SET_SABERACTIVE),
	ENUM2STRING(SET_ADJUST_AREA_PORTALS),
	ENUM2STRING(SET_DMG_BY_HEAVY_WEAP_ONLY),
	ENUM2STRING(SET_SHIELDED),
	ENUM2STRING(SET_NO_GROUPS),
	ENUM2STRING(SET_FIRE_WEAPON),
	ENUM2STRING(SET_INACTIVE),
	ENUM2STRING(SET_FUNC_USABLE_VISIBLE),
	ENUM2STRING(SET_MISSION_STATUS_SCREEN),
	ENUM2STRING(SET_END_SCREENDISSOLVE),
	ENUM2STRING(SET_LOOPSOUND),
	ENUM2STRING(SET_ICARUS_FREEZE),
	ENUM2STRING(SET_ICARUS_UNFREEZE),
	ENUM2STRING(SET_USE_CP_NEAREST),
	ENUM2STRING(SET_MORELIGHT),
	ENUM2STRING(SET_CINEMATIC_SKIPSCRIPT),
	ENUM2STRING(SET_NO_FORCE),
	ENUM2STRING(SET_NO_FALLTODEATH),
	ENUM2STRING(SET_DISMEMBERABLE),
	ENUM2STRING(SET_NO_ACROBATICS),
	ENUM2STRING(SET_MUSIC_STATE),
	ENUM2STRING(SET_USE_SUBTITLES),
	ENUM2STRING(SET_CLEAN_DAMAGING_ENTS),
	ENUM2STRING(SET_HUD),

//FIXME: add BOTH_ attributes here too
	{"",	SET_},
};

qboolean COM_ParseString( char **data, char **s );

//=======================================================================

interface_export_t	interface_export;


vec4_t textcolor_caption;
vec4_t textcolor_center;
vec4_t textcolor_scroll;


/*
============
Q3_ReadScript
  Description	: Reads in a file and attaches the script directory properly
  Return type	: static int
  Argument		: const char *name
  Argument		: void **buf
============
*/
extern int ICARUS_GetScript( const char *name, char **buf );	//g_icarus.cpp
static int Q3_ReadScript( const char *name, void **buf )
{
	return ICARUS_GetScript( va( "%s/%s", Q3_SCRIPT_DIR, name ), (char**)buf );	//get a (hopefully) cached file
}

/*
============
Q3_CenterPrint
  Description	: Prints a message in the center of the screen
  Return type	: static void
  Argument		:  const char *format
  Argument		: ...
============
*/
static void Q3_CenterPrint ( const char *format, ... )
{

	va_list		argptr;
	char		text[1024];

	va_start (argptr, format);
	Q_vsnprintf (text, sizeof(text), format, argptr);
	va_end (argptr);

	// FIXME: added '!' so you can print something that's hasn't been precached, '@' searches only for precache text
	// this is just a TEMPORARY placeholder until objectives are in!!!  -- dmv 11/26/01

	if ((text[0] == '@') || text[0] == '!')	// It's a key
	{
		if( text[0] == '!')
		{
			gi.SendServerCommand( 0, "cp \"%s\"", (text+1) );
			return;
		}

		gi.SendServerCommand( 0, "cp \"%s\"", text );
	}

	Q3_DebugPrint( WL_VERBOSE, "%s\n", text); 	// Just a developers note

	return;
}

/*
-------------------------
SetTextColor
-------------------------
*/

static void SetTextColor ( vec4_t textcolor,const char *color)
{

	if (Q_stricmp(color,"BLACK") == 0)
	{
		VectorCopy4( colorTable[CT_BLACK], textcolor );
	}
	else if (Q_stricmp(color,"RED") == 0)
	{
		VectorCopy4( colorTable[CT_RED], textcolor );
	}
	else if (Q_stricmp(color,"GREEN") == 0)
	{
		VectorCopy4( colorTable[CT_GREEN], textcolor );
	}
	else if (Q_stricmp(color,"YELLOW") == 0)
	{
		VectorCopy4( colorTable[CT_YELLOW], textcolor );
	}
	else if (Q_stricmp(color,"BLUE") == 0)
	{
		VectorCopy4( colorTable[CT_BLUE], textcolor );
	}
	else if (Q_stricmp(color,"CYAN") == 0)
	{
		VectorCopy4( colorTable[CT_CYAN], textcolor );
	}
	else if (Q_stricmp(color,"MAGENTA") == 0)
	{
		VectorCopy4( colorTable[CT_MAGENTA], textcolor );
	}
	else if (Q_stricmp(color,"WHITE") == 0)
	{
		VectorCopy4( colorTable[CT_WHITE], textcolor );
	}
	else
	{
		VectorCopy4( colorTable[CT_WHITE], textcolor );
	}

	return;
}

/*
-------------------------
void Q3_ClearTaskID( int *taskID )

WARNING: Clearing a taskID will make that task never finish unless you intend to
			return the same taskID from somewhere else.
-------------------------
*/
void Q3_TaskIDClear( int *taskID )
{
	*taskID = -1;
}

/*
-------------------------
qboolean Q3_TaskIDPending( gentity_t *ent, taskID_t taskType )
-------------------------
*/
qboolean Q3_TaskIDPending( gentity_t *ent, taskID_t taskType )
{
	if ( !ent->sequencer || !ent->taskManager )
	{
		return qfalse;
	}

	if ( taskType < TID_CHAN_VOICE || taskType >= NUM_TIDS )
	{
		return qfalse;
	}

	if ( ent->taskID[taskType] >= 0 )//-1 is none
	{
		return qtrue;
	}

	return qfalse;
}

/*
-------------------------
void Q3_TaskIDComplete( gentity_t *ent, taskID_t taskType )
-------------------------
*/
void Q3_TaskIDComplete( gentity_t *ent, taskID_t taskType )
{
	if ( taskType < TID_CHAN_VOICE || taskType >= NUM_TIDS )
	{
		return;
	}

	if ( ent->taskManager && Q3_TaskIDPending( ent, taskType ) )
	{//Complete it
		ent->taskManager->Completed( ent->taskID[taskType] );

		//See if any other tasks have the name number and clear them so we don't complete more than once
		int	clearTask = ent->taskID[taskType];
		for ( int tid = 0; tid < NUM_TIDS; tid++ )
		{
			if ( ent->taskID[tid] == clearTask )
			{
				Q3_TaskIDClear( &ent->taskID[tid] );
			}
		}

		//clear it - should be cleared in for loop above
		//Q3_TaskIDClear( &ent->taskID[taskType] );
	}
	//otherwise, wasn't waiting for a task to complete anyway
}

/*
-------------------------
void Q3_SetTaskID( gentity_t *ent, taskID_t taskType, int taskID )
-------------------------
*/

static void Q3_TaskIDSet( gentity_t *ent, taskID_t taskType, int taskID )
{
	if ( taskType < TID_CHAN_VOICE || taskType >= NUM_TIDS )
	{
		return;
	}

	//Might be stomping an old task, so complete and clear previous task if there was one
	Q3_TaskIDComplete( ent, taskType );

	ent->taskID[taskType] = taskID;
}


/*
============
Q3_CheckStringCounterIncrement
  Description	:
  Return type	: static float
  Argument		: const char *string
============
*/
static float Q3_CheckStringCounterIncrement( const char *string )
{
	char	*numString;
	float	val = 0.0f;

	if ( string[0] == '+' )
	{//We want to increment whatever the value is by whatever follows the +
		if ( string[1] )
		{
			numString = (char *)&string[1];
			val = atof( numString );
		}
	}
	else if ( string[0] == '-' )
	{//we want to decrement
		if ( string[1] )
		{
			numString = (char *)&string[1];
			val = atof( numString ) * -1;
		}
	}

	return val;
}

/*
-------------------------
Q3_GetAnimLower
-------------------------
*/
static char *Q3_GetAnimLower( gentity_t *ent )
{
	if ( ent->client == NULL )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_GetAnimLower: attempted to read animation state off non-client!\n" );
		return NULL;
	}

	int anim = ent->client->ps.legsAnim;

	return (char *) GetStringForID( animTable, anim );
}

/*
-------------------------
Q3_GetAnimUpper
-------------------------
*/
static char *Q3_GetAnimUpper( gentity_t *ent )
{
	if ( ent->client == NULL )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_GetAnimUpper: attempted to read animation state off non-client!\n" );
		return NULL;
	}

	int anim = ent->client->ps.torsoAnim;

	return (char *) GetStringForID( animTable, anim );
}

/*
-------------------------
Q3_GetAnimBoth
-------------------------
*/
static char *Q3_GetAnimBoth( gentity_t *ent )
{
 	char	*lowerName, *upperName;

	lowerName = Q3_GetAnimLower( ent );
	upperName = Q3_GetAnimUpper( ent );

	if ( VALIDSTRING( lowerName ) == false )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_GetAnimBoth: NULL legs animation string found!\n" );
		return NULL;
	}

	if ( VALIDSTRING( upperName ) == false )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_GetAnimBoth: NULL torso animation string found!\n" );
		return NULL;
	}

	if ( Q_stricmp( lowerName, upperName ) )
	{
#ifdef _DEBUG	// sigh, cut down on tester reports that aren't important
		Q3_DebugPrint( WL_WARNING, "Q3_GetAnimBoth: legs and torso animations did not match : returning legs\n" );
#endif
	}

	return lowerName;
}

/*
-------------------------
Q3_SetObjective
-------------------------
*/
static void Q3_SetObjective(const char *ObjEnum, int status)
{
	int objectiveID;
	gclient_t	*client;
	objectives_t	*objective;
	int				*objectivesShown;

	client = &level.clients[0];

	objectiveID = GetIDForString( objectiveTable, ObjEnum );
	objective = &client->sess.mission_objectives[objectiveID];
	objectivesShown = &client->sess.missionObjectivesShown;


	switch (status)
	{
	case SET_OBJ_HIDE :
		objective->display = OBJECTIVE_HIDE;
		break;
	case SET_OBJ_SHOW :
		objective->display = OBJECTIVE_SHOW;
		objectivesShown++;
		missionInfo_Updated = qtrue;	// Activate flashing text
		break;
	case SET_OBJ_PENDING :
		objective->status = OBJECTIVE_STAT_PENDING;
		if (objective->display != OBJECTIVE_HIDE)
		{
			objectivesShown++;
			missionInfo_Updated = qtrue;	// Activate flashing text
		}
		break;
	case SET_OBJ_SUCCEEDED :
		objective->status = OBJECTIVE_STAT_SUCCEEDED;
		if (objective->display != OBJECTIVE_HIDE)
		{
			objectivesShown++;
			missionInfo_Updated = qtrue;	// Activate flashing text
		}
		break;
	case SET_OBJ_FAILED :
		objective->status = OBJECTIVE_STAT_FAILED;
		if (objective->display != OBJECTIVE_HIDE)
		{
			objectivesShown++;
			missionInfo_Updated = qtrue;	// Activate flashing text
		}
		break;
	}
}


/*
-------------------------
Q3_SetMissionFailed
-------------------------
*/
extern void G_PlayerGuiltDeath( void );
static void	Q3_SetMissionFailed(const char *TextEnum)
{
	gentity_t	*ent = &g_entities[0];

	if ( ent->health >= 0 )
	{
		G_PlayerGuiltDeath();
	}
	ent->health = 0;
	//FIXME: what about other NPCs?  Scripts?

	// statusTextIndex is looked at on the client side.
	statusTextIndex = GetIDForString( missionFailedTable, TextEnum );
	cg.missionStatusShow = qtrue;
	if ( ent->client )
	{//hold this screen up for at least 2 seconds
		ent->client->respawnTime = level.time + 2000;
	}
}

/*
-------------------------
Q3_SetStatusText
-------------------------
*/
static void Q3_SetStatusText(const char *StatusTextEnum)
{
	int statusTextID;

	statusTextID = GetIDForString( statusTextTable, StatusTextEnum );

	switch (statusTextID)
	{
	case STAT_INSUBORDINATION:
	case STAT_YOUCAUSEDDEATHOFTEAMMATE:
	case STAT_DIDNTPROTECTTECH:
	case STAT_DIDNTPROTECT7OF9:
	case STAT_NOTSTEALTHYENOUGH:
	case STAT_STEALTHTACTICSNECESSARY:
	case STAT_WATCHYOURSTEP:
	case STAT_JUDGEMENTMUCHDESIRED:
		statusTextIndex = statusTextID;
		break;
	default:
		assert(0);
		break;
	}
}


/*
-------------------------
Q3_ObjectiveClearAll
-------------------------
*/
static void Q3_ObjectiveClearAll(void)
{
	client = &level.clients[0];
	memset(client->sess.mission_objectives,0,sizeof(client->sess.mission_objectives));
}

/*
=============
Q3_SetCaptionTextColor

Change color text prints in
=============
*/
static void Q3_SetCaptionTextColor ( const char *color)
{
	SetTextColor(textcolor_caption,color);
}

/*
=============
Q3_SetCenterTextColor

Change color text prints in
=============
*/
static void Q3_SetCenterTextColor ( const char *color)
{
	SetTextColor(textcolor_center,color);
}

/*
=============
Q3_SetScrollTextColor

Change color text prints in
=============
*/
static void Q3_SetScrollTextColor ( const char *color)
{
	SetTextColor(textcolor_scroll,color);
}

/*
=============
Q3_ScrollText

Prints a message in the center of the screen
=============
*/
static void Q3_ScrollText ( const char *id)
{
	gi.SendServerCommand( 0, "st \"%s\"", id);

	return;
}

/*
=============
Q3_LCARSText

Prints a message in the center of the screen giving it an LCARS frame around it
=============
*/
static void Q3_LCARSText ( const char *id)
{
	gi.SendServerCommand( 0, "lt \"%s\"", id);

	return;
}

/*
=============
Q3_GetEntityByName

Returns the sequencer of the entity by the given name
=============
*/
static gentity_t *Q3_GetEntityByName( const char *name )
{
	gentity_t				*ent;
	entlist_t::iterator		ei;
	char					temp[1024];

	if ( name == NULL || name[0] == '\0' )
		return NULL;

	strncpy( (char *) temp, name, sizeof(temp) );
	temp[sizeof(temp)-1] = 0;

	ei = ICARUS_EntList.find( Q_strupr( (char *) temp ) );

	if ( ei == ICARUS_EntList.end() )
		return NULL;

	ent = &g_entities[(*ei).second];

	return ent;
	// this now returns the ent instead of the sequencer -- dmv 06/27/01
//	if (ent == NULL)
//		return NULL;
//	return ent->sequencer;
}

/*
=============
Q3_GetTime

Get the current game time
=============
*/
static unsigned int Q3_GetTime( void )
{
	return level.time;
}

/*
=============
G_AddSexToMunroString

Take any string, look for "kyle/" replace with "kyla/" based on "sex"
And: Take any string, look for "/mr_" replace with "/ms_" based on "sex"
returns qtrue if changed to ms
=============
*/
/*
static qboolean G_AddSexToMunroString ( char *string, qboolean qDoBoth )
{
	char *start;

	if VALIDSTRING( string ) {
		if ( g_sex->string[0] == 'f' ) {
			start = strstr( string, "kyle/" );
			if ( start != NULL ) {
				strncpy( start, "kyla", 5 );
				return qtrue;
			} else {
				start = strrchr( string, '/' );		//get the last slash before the wav
				if (start != NULL) {
					if (!strncmp( start, "/mr_", 4) ) {
						if (qDoBoth) {	//we want to change mr to ms
							start[2] = 's';	//change mr to ms
							return qtrue;
						} else {	//IF qDoBoth
							return qfalse;	//don't want this one
						}
					}
				}	//IF found slash
			}
		}	//IF Female
		else {	//i'm male
			start = strrchr( string, '/' );		//get the last slash before the wav
			if (start != NULL) {
				if (!strncmp( start, "/ms_", 4) ) {
					return qfalse;	//don't want this one
				}
			}	//IF found slash
		}
	}	//if VALIDSTRING
	return qtrue;
}
*/

/*
=============
Q3_PlaySound

Plays a sound from an entity
=============
*/
extern void G_SoundOnEnt (gentity_t *ent, soundChannel_t channel, const char *soundPath);
extern void G_SoundBroadcast( gentity_t *ent, int soundIndex );
static int Q3_PlaySound( int taskID, int entID, const char *name, const char *channel )
{
	gentity_t		*ent = &g_entities[entID];
	char			finalName[MAX_QPATH];
	soundChannel_t	voice_chan = CHAN_VOICE; // set a default so the compiler doesn't bitch
	qboolean		type_voice = qfalse;

	Q_strncpyz( finalName, name, MAX_QPATH );
	Q_strupr(finalName);
	//G_AddSexToMunroString( finalName, qtrue );

	COM_StripExtension( (const char *)finalName, finalName, sizeof(finalName) );

	int soundHandle = G_SoundIndex( (char *) finalName );
	bool bBroadcast = false;

	if ( ( Q_stricmp( channel, "CHAN_ANNOUNCER" ) == 0 ) || (ent->classname && Q_stricmp("target_scriptrunner", ent->classname ) == 0) ) {
		bBroadcast = true;
	}


	// moved here from further down so I can easily check channel-type without code dup...
	//
	if ( Q_stricmp( channel, "CHAN_VOICE" ) == 0 )
	{
		voice_chan = CHAN_VOICE;
		type_voice = qtrue;
	}
	else if ( Q_stricmp( channel, "CHAN_VOICE_ATTEN" ) == 0 )
	{
		voice_chan = CHAN_VOICE_ATTEN;
		type_voice = qtrue;
	}
	else if ( Q_stricmp( channel, "CHAN_VOICE_GLOBAL" ) == 0 ) // this should broadcast to everyone, put only casue animation on G_SoundOnEnt...
	{
		voice_chan = CHAN_VOICE_GLOBAL;
		type_voice = qtrue;
		bBroadcast = true;
	}

	// if we're in-camera, check for skipping cinematic and ifso, no subtitle print (since screen is not being
	//	updated anyway during skipping). This stops leftover subtitles being left onscreen after unskipping.
	//
	if (!in_camera ||
		(!g_skippingcin || !g_skippingcin->integer)
		)	// paranoia towards project end <g>
	{
		// Text on
		// certain NPC's we always want to use subtitles regardless of subtitle setting
		if (g_subtitles->integer == 1 || (ent->NPC && (ent->NPC->scriptFlags & SCF_USE_SUBTITLES) ) ) // Show all text
		{
			if ( in_camera)	// Cinematic
			{
				gi.SendServerCommand( 0, "ct \"%s\" %i", finalName, soundHandle );
			}
			else //if (precacheWav[i].speaker==SP_NONE)	//  lower screen text
			{
				gentity_t		*ent2 = &g_entities[0];
				// the numbers in here were either the original ones Bob entered (350), or one arrived at from checking the distance Chell stands at in stasis2 by the computer core that was submitted as a bug report...
				//
				if (bBroadcast || (DistanceSquared(ent->currentOrigin, ent2->currentOrigin) < ((voice_chan == CHAN_VOICE_ATTEN)?(350 * 350):(1200 * 1200)) ) )
				{
					gi.SendServerCommand( 0, "ct \"%s\" %i", finalName, soundHandle );
				}
			}
		}
		// Cinematic only
		else if (g_subtitles->integer == 2) // Show only talking head text and CINEMATIC
		{
			if ( in_camera)	// Cinematic text
			{
				gi.SendServerCommand( 0, "ct \"%s\" %i", finalName, soundHandle);
			}
		}

	}

	if ( type_voice )
	{
		if ( g_timescale->value > 1.0f )
		{//Skip the damn sound!
			return qtrue;
		}
		else
		{
			//This the voice channel
			G_SoundOnEnt( ent, voice_chan, ((char *) finalName) );
		}
		//Remember we're waiting for this
		Q3_TaskIDSet( ent, TID_CHAN_VOICE, taskID );
		//do not task_return complete
	//	if( voice_chan == CHAN_VOICE_GLOBAL )
	//	{
	//		G_SoundBroadcast( ent, soundHandle );
	//	}
		return qfalse;
	}

	if ( bBroadcast )
	{//Broadcast the sound
		G_SoundBroadcast( ent, soundHandle );
	}
	else
	{
		G_Sound( ent, soundHandle );
	}

	return qtrue;
}


/*
=============
Q3_SetAngles

Sets the angles of an entity directly
=============
*/
static void Q3_SetDYaw( int entID, float data );
static void Q3_SetAngles( int entID, vec3_t angles )
{
	gentity_t	*ent = &g_entities[entID];


	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetAngles: bad ent %d\n", entID);
		return;
	}

	if (ent->client)
	{
		SetClientViewAngle( ent, angles );
		if ( ent->NPC )
		{
			Q3_SetDYaw( entID, angles[YAW] );
		}
	}
	else
	{
		VectorCopy( angles, ent->s.angles );
	}
	gi.linkentity( ent );
}

/*
=============
Q3_SetOrigin

Sets the origin of an entity directly
=============
*/
static void Q3_SetOrigin( int entID, vec3_t origin )
{
	gentity_t	*ent = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetOrigin: bad ent %d\n", entID);
		return;
	}

	gi.unlinkentity (ent);

	if(ent->client)
	{
		VectorCopy(origin, ent->client->ps.origin);
		VectorCopy(origin, ent->currentOrigin);
		ent->client->ps.origin[2] += 1;

		VectorClear (ent->client->ps.velocity);
		ent->client->ps.pm_time = 160;		// hold time
		ent->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;

		ent->client->ps.eFlags ^= EF_TELEPORT_BIT;

//		G_KillBox (ent);
	}
	else
	{
		G_SetOrigin( ent, origin );
	}

	gi.linkentity( ent );
}


/*
============
MoveOwner
  Description	:
  Return type	: void
  Argument		: gentity_t *self
============
*/
void MoveOwner( gentity_t *self )
{
	self->nextthink = level.time + FRAMETIME;
	self->e_ThinkFunc = thinkF_G_FreeEntity;

	if ( !self->owner || !self->owner->inuse )
	{
		return;
	}

	if ( SpotWouldTelefrag2( self->owner, self->currentOrigin ) )
	{
		self->e_ThinkFunc = thinkF_MoveOwner;
	}
	else
	{
		G_SetOrigin( self->owner, self->currentOrigin );
		Q3_TaskIDComplete( self->owner, TID_MOVE_NAV );
	}
}


/*
=============
Q3_SetTeleportDest

Copies passed origin to ent running script once there is nothing there blocking the spot
=============
*/
static qboolean Q3_SetTeleportDest( int entID, vec3_t org )
{
	gentity_t	*teleEnt = &g_entities[entID];

	if ( teleEnt )
	{
		if ( SpotWouldTelefrag2( teleEnt, org ) )
		{
			gentity_t *teleporter = G_Spawn();

			G_SetOrigin( teleporter, org );
			teleporter->owner = teleEnt;

			teleporter->e_ThinkFunc = thinkF_MoveOwner;
			teleporter->nextthink = level.time + FRAMETIME;

			return qfalse;
		}
		else
		{
			G_SetOrigin( teleEnt, org );
		}
	}

	return qtrue;
}

/*
=============
Q3_SetCopyOrigin

Copies origin of found ent into ent running script
=============`
*/
static void Q3_SetCopyOrigin( int entID, const char *name )
{
	gentity_t	*found = G_Find( NULL, FOFS(targetname), (char *) name);

	if(found)
	{
		Q3_SetOrigin( entID, found->currentOrigin );
		SetClientViewAngle( &g_entities[entID], found->s.angles );
	}
	else
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetCopyOrigin: ent %s not found!\n", name);
	}
}

/*
=============
Q3_SetVelocity

Set the velocity of an entity directly
=============
*/
static void Q3_SetVelocity( int entID, int axis, float speed )
{
	gentity_t	*found = &g_entities[entID];
	//FIXME: Not supported
	if(!found)
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetVelocity invalid entID %d\n", entID);
		return;
	}

	if(!found->client)
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetVelocity: not a client %d\n", entID);
		return;
	}

	//FIXME: add or set?
	found->client->ps.velocity[axis] += speed;

	found->client->ps.pm_time = 500;
	found->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
}

/*
============
Q3_SetAdjustAreaPortals
  Description	:
  Return type	: void
  Argument		:  int entID
  Argument		: qboolean shields
============
*/
static void Q3_SetAdjustAreaPortals( int entID, qboolean adjust )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetAdjustAreaPortals: invalid entID %d\n", entID);
		return;
	}

	ent->svFlags = (adjust) ? (ent->svFlags|SVF_MOVER_ADJ_AREA_PORTALS) : (ent->svFlags&~SVF_MOVER_ADJ_AREA_PORTALS);
}

/*
============
Q3_SetDmgByHeavyWeapOnly
  Description	:
  Return type	: void
  Argument		:  int entID
  Argument		: qboolean dmg
============
*/
static void Q3_SetDmgByHeavyWeapOnly( int entID, qboolean dmg )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetDmgByHeavyWeapOnly: invalid entID %d\n", entID);
		return;
	}

	ent->flags = (dmg) ? (ent->flags|FL_DMG_BY_HEAVY_WEAP_ONLY) : (ent->flags&~FL_DMG_BY_HEAVY_WEAP_ONLY);
}

/*
============
Q3_SetShielded
  Description	:
  Return type	: void
  Argument		:  int entID
  Argument		: qboolean dmg
============
*/
static void Q3_SetShielded( int entID, qboolean dmg )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetShielded: invalid entID %d\n", entID);
		return;
	}

	ent->flags = (dmg) ? (ent->flags|FL_SHIELDED) : (ent->flags&~FL_SHIELDED);
}

/*
============
Q3_SetNoGroups
  Description	:
  Return type	: void
  Argument		:  int entID
  Argument		: qboolean dmg
============
*/
static void Q3_SetNoGroups( int entID, qboolean noGroups )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetNoGroups: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetNoGroups: ent %s is not an NPC!\n", ent->targetname );
		return;
	}

	ent->NPC->scriptFlags = noGroups ? (ent->NPC->scriptFlags|SCF_NO_GROUPS) : (ent->NPC->scriptFlags&~SCF_NO_GROUPS);
}

/*
=============
moverCallback

Utility function
=============
*/
extern void misc_model_breakable_gravity_init( gentity_t *ent, qboolean dropToFloor );
void moverCallback( gentity_t *ent )
{
	//complete the task
	Q3_TaskIDComplete( ent, TID_MOVE_NAV );

	// play sound
	ent->s.loopSound = 0;//stop looping sound
	G_PlayDoorSound( ent, BMS_END );//play end sound

	if ( ent->moverState == MOVER_1TO2 )
	{//reached open
		// reached pos2
		MatchTeam( ent, MOVER_POS2, level.time );
		//SetMoverState( ent, MOVER_POS2, level.time );
	}
	else if ( ent->moverState == MOVER_2TO1 )
	{//reached closed
		MatchTeam( ent, MOVER_POS1, level.time );
		//SetMoverState( ent, MOVER_POS1, level.time );
		//close the portal
		if ( ent->svFlags & SVF_MOVER_ADJ_AREA_PORTALS )
		{
			gi.AdjustAreaPortalState( ent, qfalse );
		}
	}

	if ( ent->e_BlockedFunc == blockedF_Blocked_Mover )
	{
		ent->e_BlockedFunc = blockedF_NULL;
	}

	if ( !Q_stricmp( "misc_model_breakable", ent->classname ) && ent->physicsBounce )
	{//a gravity-affected model
		misc_model_breakable_gravity_init( ent, qfalse );
	}
}

/*
=============
anglerCallback

Utility function
=============
*/
void anglerCallback( gentity_t *ent )
{
	//Complete the task
	Q3_TaskIDComplete( ent, TID_ANGLE_FACE );

	//Set the currentAngles, clear all movement
	VectorMA( ent->s.apos.trBase, (ent->s.apos.trDuration*0.001f), ent->s.apos.trDelta, ent->currentAngles );
	VectorCopy( ent->currentAngles, ent->s.apos.trBase );
	VectorClear( ent->s.apos.trDelta );
	ent->s.apos.trDuration = 1;
	ent->s.apos.trType = TR_STATIONARY;
	ent->s.apos.trTime = level.time;

	//Stop thinking
	ent->e_ReachedFunc = reachedF_NULL;
	if ( ent->e_ThinkFunc == thinkF_anglerCallback )
	{
		ent->e_ThinkFunc = thinkF_NULL;
	}

	//link
	gi.linkentity( ent );
}

/*
=============
moveAndRotateCallback

Utility function
=============
*/
void moveAndRotateCallback( gentity_t *ent )
{
	//stop turning
	anglerCallback( ent );
	//stop moving
	moverCallback( ent );
}

void Blocked_Mover( gentity_t *ent, gentity_t *other ) {
	// remove anything other than a client -- no longer the case

	// don't remove security keys or goodie keys
	if ( (other->s.eType == ET_ITEM) && (other->item->giTag >= INV_GOODIE_KEY && other->item->giTag <= INV_SECURITY_KEY) )
	{
		// should we be doing anything special if a key blocks it... move it somehow..?
	}
	// if your not a client, or your a dead client remove yourself...
	else if ( other->s.number && (!other->client || (other->client && other->health <= 0 && other->contents == CONTENTS_CORPSE && !other->message)) )
	{
		if ( !other->taskManager || !other->taskManager->IsRunning() )
		{
			// if an item or weapon can we do a little explosion..?
			G_FreeEntity( other );
			return;
		}
	}

	if ( ent->damage ) {
		G_Damage( other, ent, ent, NULL, NULL, ent->damage, 0, MOD_CRUSH );
	}
}

/*
=============
Q3_Lerp2Start

Lerps the origin of an entity to its starting position
=============
*/
static void Q3_Lerp2Start( int entID, int taskID, float duration )
{
	gentity_t	*ent = &g_entities[entID];

	if(!ent)
	{
		Q3_DebugPrint( WL_WARNING, "Q3_Lerp2Start: invalid entID %d\n", entID);
		return;
	}

	if ( ent->client || ent->NPC || Q_stricmp(ent->classname, "target_scriptrunner") == 0 )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_Lerp2Start: ent %d is NOT a mover!\n", entID);
		return;
	}

	if ( ent->s.eType != ET_MOVER )
	{
		ent->s.eType = ET_MOVER;
	}

	//FIXME: set up correctly!!!
	ent->moverState = MOVER_2TO1;
	ent->s.eType = ET_MOVER;
	ent->e_ReachedFunc = reachedF_moverCallback;		//Callsback the the completion of the move
	if ( ent->damage )
	{
		ent->e_BlockedFunc = blockedF_Blocked_Mover;
	}

	ent->s.pos.trDuration = duration * 10;	//In seconds
	ent->s.pos.trTime = level.time;

	Q3_TaskIDSet( ent, TID_MOVE_NAV, taskID );
	// starting sound
	G_PlayDoorLoopSound( ent );
	G_PlayDoorSound( ent, BMS_START );	//??

	gi.linkentity( ent );
}

/*
=============
Q3_Lerp2End

Lerps the origin of an entity to its ending position
=============
*/
static void Q3_Lerp2End( int entID, int taskID, float duration )
{
	gentity_t	*ent = &g_entities[entID];

	if(!ent)
	{
		Q3_DebugPrint( WL_WARNING, "Q3_Lerp2End: invalid entID %d\n", entID);
		return;
	}

	if ( ent->client || ent->NPC || Q_stricmp(ent->classname, "target_scriptrunner") == 0 )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_Lerp2End: ent %d is NOT a mover!\n", entID);
		return;
	}

	if ( ent->s.eType != ET_MOVER )
	{
		ent->s.eType = ET_MOVER;
	}

	if ( ent->moverState == MOVER_POS1 )
	{//open the portal
		if ( ent->svFlags & SVF_MOVER_ADJ_AREA_PORTALS )
		{
			gi.AdjustAreaPortalState( ent, qtrue );
		}
	}

	//FIXME: set up correctly!!!
	ent->moverState = MOVER_1TO2;
	ent->s.eType = ET_MOVER;
	ent->e_ReachedFunc = reachedF_moverCallback;		//Callsback the the completion of the move
	if ( ent->damage )
	{
		ent->e_BlockedFunc = blockedF_Blocked_Mover;
	}

	ent->s.pos.trDuration = duration * 10;	//In seconds
	ent->s.time = level.time;

	Q3_TaskIDSet( ent, TID_MOVE_NAV, taskID );
	// starting sound
	G_PlayDoorLoopSound( ent );
	G_PlayDoorSound( ent, BMS_START );	//??

	gi.linkentity( ent );
}

/*
=============
Q3_Lerp2Pos

Lerps the origin and angles of an entity to the destination values

=============
*/
static void Q3_Lerp2Pos( int taskID, int entID, vec3_t origin, vec3_t angles, float duration )
{
	gentity_t	*ent = &g_entities[entID];
	vec3_t		ang;
	int			i;

	if(!ent)
	{
		Q3_DebugPrint( WL_WARNING, "Q3_Lerp2Pos: invalid entID %d\n", entID);
		return;
	}

	if ( ent->client || ent->NPC || Q_stricmp(ent->classname, "target_scriptrunner") == 0 )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_Lerp2Pos: ent %d is NOT a mover!\n", entID);
		return;
	}

	if ( ent->s.eType != ET_MOVER )
	{
		ent->s.eType = ET_MOVER;
	}

	//Don't allow a zero duration
	if ( duration == 0 )
		duration = 1;

	//
	// Movement

	moverState_t moverState = ent->moverState;

	if ( moverState == MOVER_POS1 || moverState == MOVER_2TO1 )
	{
		VectorCopy( ent->currentOrigin, ent->pos1 );
		VectorCopy( origin, ent->pos2 );

		if ( moverState == MOVER_POS1 )
		{//open the portal
			if ( ent->svFlags & SVF_MOVER_ADJ_AREA_PORTALS )
			{
				gi.AdjustAreaPortalState( ent, qtrue );
			}
		}

		moverState = MOVER_1TO2;
	}
	else /*if ( moverState == MOVER_POS2 || moverState == MOVER_1TO2 )*/
	{
		VectorCopy( ent->currentOrigin, ent->pos2 );
		VectorCopy( origin, ent->pos1 );

		moverState = MOVER_2TO1;
	}

	InitMoverTrData( ent );

	ent->s.pos.trDuration = duration;

	// start it going
	MatchTeam( ent, moverState, level.time );
	//SetMoverState( ent, moverState, level.time );

	//Only do the angles if specified
	if ( angles != NULL )
	{
		//
		// Rotation

		for ( i = 0; i < 3; i++ )
		{
			ang[i] = AngleDelta( angles[i], ent->currentAngles[i] );
			ent->s.apos.trDelta[i] = ( ang[i] / ( duration * 0.001f ) );
		}

		VectorCopy( ent->currentAngles, ent->s.apos.trBase );

		if ( ent->alt_fire )
		{
			ent->s.apos.trType = TR_LINEAR_STOP;
		}
		else
		{
			ent->s.apos.trType = TR_NONLINEAR_STOP;
		}
		ent->s.apos.trDuration = duration;

		ent->s.apos.trTime = level.time;

		ent->e_ReachedFunc = reachedF_moveAndRotateCallback;
		Q3_TaskIDSet( ent, TID_ANGLE_FACE, taskID );
	}
	else
	{
		//Setup the last bits of information
		ent->e_ReachedFunc  = reachedF_moverCallback;
	}

	if ( ent->damage )
	{
		ent->e_BlockedFunc = blockedF_Blocked_Mover;
	}

	Q3_TaskIDSet( ent, TID_MOVE_NAV, taskID );
	// starting sound
	G_PlayDoorLoopSound( ent );
	G_PlayDoorSound( ent, BMS_START );	//??

	gi.linkentity( ent );

}

/*
=============
Q3_Lerp2Origin

Lerps the origin to the destination value
=============
*/
static void Q3_Lerp2Origin( int taskID, int entID, vec3_t origin, float duration )
{
	gentity_t	*ent = &g_entities[entID];

	if(!ent)
	{
		Q3_DebugPrint( WL_WARNING, "Q3_Lerp2Origin: invalid entID %d\n", entID);
		return;
	}

	if ( ent->client || ent->NPC || Q_stricmp(ent->classname, "target_scriptrunner") == 0 )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_Lerp2Origin: ent %d is NOT a mover!\n", entID);
		return;
	}

	if ( ent->s.eType != ET_MOVER )
	{
		ent->s.eType = ET_MOVER;
	}

	moverState_t moverState = ent->moverState;

	if ( moverState == MOVER_POS1 || moverState == MOVER_2TO1 )
	{
		VectorCopy( ent->currentOrigin, ent->pos1 );
		VectorCopy( origin, ent->pos2 );

		if ( moverState == MOVER_POS1 )
		{//open the portal
			if ( ent->svFlags & SVF_MOVER_ADJ_AREA_PORTALS )
			{
				gi.AdjustAreaPortalState( ent, qtrue );
			}
		}

		moverState = MOVER_1TO2;
	}
	else if ( moverState == MOVER_POS2 || moverState == MOVER_1TO2 )
	{
		VectorCopy( ent->currentOrigin, ent->pos2 );
		VectorCopy( origin, ent->pos1 );

		moverState = MOVER_2TO1;
	}

	InitMoverTrData( ent );	//FIXME: This will probably break normal things that are being moved...

	ent->s.pos.trDuration = duration;

	// start it going
	MatchTeam( ent, moverState, level.time );
	//SetMoverState( ent, moverState, level.time );

	ent->e_ReachedFunc  = reachedF_moverCallback;
	if ( ent->damage )
	{
		ent->e_BlockedFunc = blockedF_Blocked_Mover;
	}
	if ( taskID != -1 )
	{
		Q3_TaskIDSet( ent, TID_MOVE_NAV, taskID );
	}
	// starting sound
	G_PlayDoorLoopSound( ent );//start looping sound
	G_PlayDoorSound( ent, BMS_START );	//play start sound

	gi.linkentity( ent );
}

static void Q3_SetOriginOffset( int entID, int axis, float offset )
{
	gentity_t	*ent = &g_entities[entID];

	if(!ent)
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetOriginOffset: invalid entID %d\n", entID);
		return;
	}

	if ( ent->client || ent->NPC || Q_stricmp(ent->classname, "target_scriptrunner") == 0 )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetOriginOffset: ent %d is NOT a mover!\n", entID);
		return;
	}

	vec3_t origin;
	VectorCopy( ent->s.origin, origin );
	origin[axis] += offset;
	float duration = 0;
	if ( ent->speed )
	{
		duration = fabs(offset)/fabs(ent->speed)*1000.0f;
	}
	Q3_Lerp2Origin( -1, entID, origin, duration );
}
/*
=============
Q3_LerpAngles

Lerps the angles to the destination value
=============
*/
static void Q3_Lerp2Angles( int taskID, int entID, vec3_t angles, float duration )
{
	gentity_t	*ent = &g_entities[entID];
	vec3_t		ang;
	int			i;

	if(!ent)
	{
		Q3_DebugPrint( WL_WARNING, "Q3_Lerp2Angles: invalid entID %d\n", entID);
		return;
	}

	if ( ent->client || ent->NPC || Q_stricmp(ent->classname, "target_scriptrunner") == 0 )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_Lerp2Angles: ent %d is NOT a mover!\n", entID);
		return;
	}

	//If we want an instant move, don't send 0...
	ent->s.apos.trDuration = (duration>0) ? duration : 1;

	for ( i = 0; i < 3; i++ )
	{
		ang [i] = AngleSubtract( angles[i], ent->currentAngles[i]);
		ent->s.apos.trDelta[i] = ( ang[i] / ( ent->s.apos.trDuration * 0.001f ) );
	}

	VectorCopy( ent->currentAngles, ent->s.apos.trBase );

	if ( ent->alt_fire )
	{
		ent->s.apos.trType = TR_LINEAR_STOP;
	}
	else
	{
		ent->s.apos.trType = TR_NONLINEAR_STOP;
	}

	ent->s.apos.trTime = level.time;

	Q3_TaskIDSet( ent, TID_ANGLE_FACE, taskID );

	//ent->e_ReachedFunc = reachedF_NULL;
	ent->e_ThinkFunc = thinkF_anglerCallback;
	ent->nextthink = level.time + duration;

	gi.linkentity( ent );
}

/*
=============
Q3_GetTag

Gets the value of a tag by the give name
=============
*/
static int	Q3_GetTag( int entID, const char *name, int lookup, vec3_t info )
{
	gentity_t	*ent = &g_entities[ entID ];

	VALIDATEB( ent );

	switch ( lookup )
	{
	case TYPE_ORIGIN:
		//return TAG_GetOrigin( ent->targetname, name, info );
		return TAG_GetOrigin( ent->ownername, name, info );
		break;

	case TYPE_ANGLES:
		//return TAG_GetAngles( ent->targetname, name, info );
		return TAG_GetAngles( ent->ownername, name, info );
		break;
	}

	return false;
}

/*
=============
Q3_SetNavGoal

Sets the navigational goal of an entity
=============
*/
static void Q3_SetNavGoal( int entID, const char *name )
{
	gentity_t	*ent  = &g_entities[ entID ];
	vec3_t		goalPos;

	if ( !ent->health )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetNavGoal: tried to set a navgoal (\"%s\") on a corpse! \"%s\"\n", name, ent->script_targetname );
		return;
	}
	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetNavGoal: tried to set a navgoal (\"%s\") on a non-NPC: \"%s\"\n", name, ent->script_targetname );
		return;
	}
	if ( !ent->NPC->tempGoal )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetNavGoal: tried to set a navgoal (\"%s\") on a dead NPC: \"%s\"\n", name, ent->script_targetname );
		return;
	}
	if ( !ent->NPC->tempGoal->inuse )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetNavGoal: NPC's (\"%s\") navgoal is freed: \"%s\"\n", name, ent->script_targetname );
		return;
	}
	if( Q_stricmp( "null", name) == 0 )
	{
		ent->NPC->goalEntity = NULL;
		Q3_TaskIDComplete( ent, TID_MOVE_NAV );
	}
	else
	{
		//Get the position of the goal
		if ( TAG_GetOrigin2( NULL, name, goalPos ) == false )
		{
			gentity_t	*targ = G_Find(NULL, FOFS(targetname), (char*)name);
			if ( !targ )
			{
				Q3_DebugPrint( WL_ERROR, "Q3_SetNavGoal: can't find NAVGOAL \"%s\"\n", name );
				return;
			}
			else
			{
				ent->NPC->goalEntity = targ;
				ent->NPC->goalRadius = sqrt(ent->maxs[0]+ent->maxs[0]) + sqrt(targ->maxs[0]+targ->maxs[0]);
				ent->NPC->aiFlags &= ~NPCAI_TOUCHED_GOAL;
			}
		}
		else
		{
			int	goalRadius = TAG_GetRadius( NULL, name );
			NPC_SetMoveGoal( ent, goalPos, goalRadius, qtrue );
			//We know we want to clear the lastWaypoint here
			ent->NPC->goalEntity->lastWaypoint = WAYPOINT_NONE;
			ent->NPC->aiFlags &= ~NPCAI_TOUCHED_GOAL;
	#ifdef _DEBUG
			//this is *only* for debugging navigation
			ent->NPC->tempGoal->target = G_NewString( name );
	#endif// _DEBUG
		}
	}
}

//-----------------------------------------------

/*
============
SetLowerAnim
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: int animID
============
*/
static void SetLowerAnim( int entID, int animID)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "SetLowerAnim: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->client )
	{
		Q3_DebugPrint( WL_ERROR, "SetLowerAnim: ent %d is NOT a player or NPC!\n", entID);
		return;
	}

	NPC_SetAnim(ent,SETANIM_LEGS,animID,SETANIM_FLAG_RESTART|SETANIM_FLAG_HOLD|SETANIM_FLAG_OVERRIDE);
}


/*
============
SetUpperAnim
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: int animID
============
*/
static void SetUpperAnim ( int entID, int animID)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "SetUpperAnim: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->client )
	{
		Q3_DebugPrint( WL_ERROR, "SetLowerAnim: ent %d is NOT a player or NPC!\n", entID);
		return;
	}

	NPC_SetAnim(ent,SETANIM_TORSO,animID,SETANIM_FLAG_RESTART|SETANIM_FLAG_HOLD|SETANIM_FLAG_OVERRIDE);
}

//-----------------------------------------------

/*
=============
Q3_SetAnimUpper

Sets the upper animation of an entity
=============
*/
static qboolean Q3_SetAnimUpper( int entID, const char *anim_name )
{
	int			animID = 0;

	animID = GetIDForString( animTable, anim_name );

	if( animID == -1 )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetAnimUpper: unknown animation sequence '%s'\n", anim_name );
		return qfalse;
	}

	if ( !PM_HasAnimation( &g_entities[entID], animID ) )
	{
		return qfalse;
	}

	SetUpperAnim( entID, animID );
	return qtrue;
}


/*
=============
Q3_SetAnimLower

Sets the lower animation of an entity
=============
*/
static qboolean Q3_SetAnimLower( int entID, const char *anim_name )
{
	int			animID = 0;

	//FIXME: Setting duck anim does not actually duck!

	animID = GetIDForString( animTable, anim_name );

	if( animID == -1 )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetAnimLower: unknown animation sequence '%s'\n", anim_name );
		return qfalse;
	}

	if ( !PM_HasAnimation( &g_entities[entID], animID ) )
	{
		return qfalse;
	}

	SetLowerAnim( entID, animID );
	return qtrue;
}

/*
============
Q3_SetAnimHoldTime
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: int int_data
  Argument		: qboolean lower
============
*/
extern void PM_SetTorsoAnimTimer( gentity_t *ent, int *torsoAnimTimer, int time );
extern void PM_SetLegsAnimTimer( gentity_t *ent, int *legsAnimTimer, int time );
static void Q3_SetAnimHoldTime( int entID, int int_data, qboolean lower )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetAnimHoldTime: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->client )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetAnimHoldTime: ent %d is NOT a player or NPC!\n", entID);
		return;
	}

	if(lower)
	{
		PM_SetLegsAnimTimer( ent, &ent->client->ps.legsAnimTimer, int_data );
	}
	else
	{
		PM_SetTorsoAnimTimer( ent, &ent->client->ps.torsoAnimTimer, int_data );
	}
}

/*
=============
Q3_SetEnemy

Sets the enemy of an entity
=============
*/
static void Q3_SetEnemy( int entID, const char *name )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetEnemy: invalid entID %d\n", entID);
		return;
	}

	if( !Q_stricmp("NONE", name) || !Q_stricmp("NULL", name))
	{
		if(ent->NPC)
		{
			G_ClearEnemy(ent);
		}
		else
		{
			ent->enemy = NULL;
		}
	}
	else
	{
		gentity_t	*enemy = G_Find( NULL, FOFS(targetname), (char *) name);

		if(enemy == NULL)
		{
			Q3_DebugPrint( WL_ERROR, "Q3_SetEnemy: no such enemy: '%s'\n", name );
			return;
		}
		/*else if(enemy->health <= 0)
		{
			//Q3_DebugPrint( WL_ERROR, "Q3_SetEnemy: ERROR - desired enemy has health %d\n", enemy->health );
			return;
		}*/
		else
		{
			if(ent->NPC)
			{
				G_SetEnemy( ent, enemy );
				ent->cantHitEnemyCounter = 0;
			}
			else
			{
				G_SetEnemy(ent, enemy);
			}
		}
	}
}

/*
=============
Q3_SetEnemy

Sets the enemy of an entity
=============
*/
/*
static void Q3_SetDefendTarget( int entID, const char *name )
{
	gentity_t	*ent  = &g_entities[entID];
	gentity_t	*defendEnt  = NULL;

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetDefendTarget: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetDefendTarget: ent %d is NOT an NPC!\n", entID);
		return;
	}

	defendEnt = G_Find(NULL, FOFS(targetname), (char *)name);
	if ( defendEnt != NULL )
	{
		ent->NPC->defendEnt = defendEnt;
	}
	else
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetDefendTarget: %s NOT found!\n", name);
	}
}
*/

/*
=============
Q3_SetLeader

Sets the leader of an NPC
=============
*/
static void Q3_SetLeader( int entID, const char *name )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetLeader: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->client )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetLeader: ent %d is NOT a player or NPC!\n", entID);
		return;
	}

	if( !Q_stricmp("NONE", name) || !Q_stricmp("NULL", name))
	{
		ent->client->leader = NULL;
	}
	else
	{
		gentity_t	*leader = G_Find( NULL, FOFS(targetname), (char *) name);

		if(leader == NULL)
		{
			//Q3_DebugPrint( WL_ERROR,"Q3_SetEnemy: unable to locate enemy: '%s'\n", name );
			return;
		}
		else if(leader->health <= 0)
		{
			//Q3_DebugPrint( WL_ERROR,"Q3_SetEnemy: ERROR - desired enemy has health %d\n", enemy->health );
			return;
		}
		else
		{
			ent->client->leader = leader;
		}
	}
}

stringID_table_t teamTable [] =
{
	ENUM2STRING(TEAM_FREE),
	ENUM2STRING(TEAM_PLAYER),
	ENUM2STRING(TEAM_ENEMY),
	ENUM2STRING(TEAM_NEUTRAL),
	{"", TEAM_FREE},
};


/*
============
Q3_SetPlayerTeam
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: const char *teamName
============
*/
static void Q3_SetPlayerTeam( int entID, const char *teamName )
{
	gentity_t	*ent  = &g_entities[entID];
	team_t		newTeam;

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetPlayerTeam: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->client )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetPlayerTeam: ent %d is NOT a player or NPC!\n", entID);
		return;
	}

	newTeam = (team_t)GetIDForString( teamTable, teamName );
	ent->client->playerTeam = newTeam;
}


/*
============
Q3_SetEnemyTeam
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: const char *teamName
============
*/
static void Q3_SetEnemyTeam( int entID, const char *teamName )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetEnemyTeam: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->client )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetEnemyTeam: ent %d is NOT a player or NPC!\n", entID);
		return;
	}

	ent->client->enemyTeam = (team_t)GetIDForString( teamTable, teamName );
}


/*
============
Q3_SetHealth
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: int data
============
*/
static void Q3_SetHealth( int entID, int data )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetHealth: invalid entID %d\n", entID);
		return;
	}

	if ( data < 0 )
	{
		data = 0;
	}

	ent->health = data;

	// should adjust max if new health is higher than max
	if ( ent->health > ent->max_health )
	{
		ent->max_health = ent->health;
	}

	if(!ent->client)
	{
		return;
	}

	ent->client->ps.stats[STAT_HEALTH] = data;
	if ( ent->s.number == 0 )
	{//clamp health to max
		if ( ent->client->ps.stats[STAT_HEALTH] > ent->client->ps.stats[STAT_MAX_HEALTH] )
		{
			ent->health = ent->client->ps.stats[STAT_HEALTH] = ent->client->ps.stats[STAT_MAX_HEALTH];
		}
		if ( data == 0 )
		{//artificially "killing" the player", don't let him respawn right away
			ent->client->ps.pm_type = PM_DEAD;
			//delay respawn for 2 seconds
			ent->client->respawnTime = level.time + 2000;
			//stop all scripts
			stop_icarus = qtrue;
			//make the team killable
			//G_MakeTeamVulnerable();
		}
	}
}

/*
============
Q3_SetArmor
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: int data
============
*/
static void Q3_SetArmor( int entID, int data )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetArmor: invalid entID %d\n", entID);
		return;
	}

	if(!ent->client)
	{
		return;
	}

	ent->client->ps.stats[STAT_ARMOR] = data;
	if ( ent->s.number == 0 )
	{//clamp armor to max_health
		if ( ent->client->ps.stats[STAT_ARMOR] > ent->client->ps.stats[STAT_MAX_HEALTH] )
		{
			ent->client->ps.stats[STAT_ARMOR] = ent->client->ps.stats[STAT_MAX_HEALTH];
		}
	}
}

/*
============
Q3_SetBState
  Description	:
  Return type	: static qboolean
  Argument		:  int entID
  Argument		: const char *bs_name
FIXME: this should be a general NPC wrapper function
	that is called ANY time	a bState is changed...
============
*/
static qboolean Q3_SetBState( int entID, const char *bs_name )
{
	gentity_t	*ent  = &g_entities[entID];
	bState_t	bSID;

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetBState: invalid entID %d\n", entID);
		return qtrue;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetBState: '%s' is not an NPC\n", ent->targetname );
		return qtrue;//ok to complete
	}

	bSID = (bState_t)(GetIDForString( BSTable, bs_name ));
	if ( bSID != (bState_t)-1 )
	{
		if ( bSID == BS_SEARCH || bSID == BS_WANDER )
		{
			//FIXME: Reimplement

			if( ent->waypoint != WAYPOINT_NONE )
			{
				NPC_BSSearchStart( ent->waypoint, bSID );
			}
			else
			{
				ent->waypoint = NAV_FindClosestWaypointForEnt( ent, WAYPOINT_NONE );

				if( ent->waypoint != WAYPOINT_NONE )
				{
					NPC_BSSearchStart( ent->waypoint, bSID );
				}
				/*else if( ent->lastWaypoint >=0 && ent->lastWaypoint < num_waypoints )
				{
					NPC_BSSearchStart( ent->lastWaypoint, bSID );
				}
				else if( ent->lastValidWaypoint >=0 && ent->lastValidWaypoint < num_waypoints )
				{
					NPC_BSSearchStart( ent->lastValidWaypoint, bSID );
				}*/
				else
				{
					Q3_DebugPrint( WL_ERROR, "Q3_SetBState: '%s' is not in a valid waypoint to search from!\n", ent->targetname );
					return qtrue;
				}
			}
		}


		ent->NPC->tempBehavior = BS_DEFAULT;//need to clear any temp behaviour
		if ( ent->NPC->behaviorState == BS_NOCLIP && bSID != BS_NOCLIP )
		{//need to rise up out of the floor after noclipping
			ent->currentOrigin[2] += 0.125;
			G_SetOrigin( ent, ent->currentOrigin );
		}
		ent->NPC->behaviorState = bSID;
		if ( bSID == BS_DEFAULT )
		{
			ent->NPC->defaultBehavior = bSID;
		}
	}

	ent->NPC->aiFlags &= ~NPCAI_TOUCHED_GOAL;

//	if ( bSID == BS_FLY )
//	{//FIXME: need a set bState wrapper
//		ent->NPC->stats.moveType = MT_FLYSWIM;
//	}
//	else
	{
		//FIXME: these are presumptions!
		//Q3_SetGravity( entID, g_gravity->value );
		//ent->NPC->stats.moveType = MT_RUNJUMP;
	}

	if ( bSID == BS_NOCLIP )
	{
		ent->client->noclip = qtrue;
	}
	else
	{
		ent->client->noclip = qfalse;
	}

/*
	if ( bSID == BS_FACE || bSID == BS_POINT_AND_SHOOT || bSID == BS_FACE_ENEMY )
	{
		ent->NPC->aimTime = level.time + 5 * 1000;//try for 5 seconds
		return qfalse;//need to wait for task complete message
	}
*/

//	if ( bSID == BS_SNIPER || bSID == BS_ADVANCE_FIGHT )
	if ( bSID == BS_ADVANCE_FIGHT )
	{
		return qfalse;//need to wait for task complete message
	}

/*
	if ( bSID == BS_SHOOT || bSID == BS_POINT_AND_SHOOT )
	{//Let them shoot right NOW
		ent->NPC->shotTime = ent->attackDebounceTime = level.time;
	}
*/
	if ( bSID == BS_JUMP )
	{
		ent->NPC->jumpState = JS_FACING;
	}

	return qtrue;//ok to complete
}


/*
============
Q3_SetTempBState
  Description	:
  Return type	: static qboolean
  Argument		:  int entID
  Argument		: const char *bs_name
============
*/
static qboolean Q3_SetTempBState( int entID, const char *bs_name )
{
	gentity_t	*ent  = &g_entities[entID];
	bState_t	bSID;

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetTempBState: invalid entID %d\n", entID);
		return qtrue;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetTempBState: '%s' is not an NPC\n", ent->targetname );
		return qtrue;//ok to complete
	}

	bSID = (bState_t)(GetIDForString( BSTable, bs_name ));
	if ( bSID != (bState_t)-1 )
	{
		ent->NPC->tempBehavior = bSID;
	}

/*
	if ( bSID == BS_FACE || bSID == BS_POINT_AND_SHOOT || bSID == BS_FACE_ENEMY )
	{
		ent->NPC->aimTime = level.time + 5 * 1000;//try for 5 seconds
		return qfalse;//need to wait for task complete message
	}
*/

/*
	if ( bSID == BS_SHOOT || bSID == BS_POINT_AND_SHOOT )
	{//Let them shoot right NOW
		ent->NPC->shotTime = ent->attackDebounceTime = level.time;
	}
*/
	return qtrue;//ok to complete
}


/*
============
Q3_SetDefaultBState
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: const char *bs_name
============
*/
static void Q3_SetDefaultBState( int entID, const char *bs_name )
{
	gentity_t	*ent  = &g_entities[entID];
	bState_t	bSID;

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetDefaultBState: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetDefaultBState: '%s' is not an NPC\n", ent->targetname );
		return;
	}

	bSID = (bState_t)(GetIDForString( BSTable, bs_name ));
	if ( bSID != (bState_t)-1 )
	{
		ent->NPC->defaultBehavior = bSID;
	}
}


/*
============
Q3_SetDPitch
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: float data
============
*/
static void Q3_SetDPitch( int entID, float data )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetDPitch: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC || !ent->client )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetDPitch: '%s' is not an NPC\n", ent->targetname );
		return;
	}

	int pitchMin = -ent->client->renderInfo.headPitchRangeUp + 1;
	int pitchMax = ent->client->renderInfo.headPitchRangeDown - 1;

	//clamp angle to -180 -> 180
	data = AngleNormalize180( data );

	//Clamp it to my valid range
	if ( data < -1 )
	{
		if ( data < pitchMin )
		{
			data = pitchMin;
		}
	}
	else if ( data > 1 )
	{
		if ( data > pitchMax )
		{
			data = pitchMax;
		}
	}

	ent->NPC->lockedDesiredPitch = ent->NPC->desiredPitch = data;
}


/*
============
Q3_SetDYaw
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: float data
============
*/
static void Q3_SetDYaw( int entID, float data )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetDYaw: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetDYaw: '%s' is not an NPC\n", ent->targetname );
		return;
	}

	if(!ent->enemy)
	{//don't mess with this if they're aiming at someone
		ent->NPC->lockedDesiredYaw = ent->NPC->desiredYaw = ent->s.angles[1] = data;
	}
	else
	{
		Q3_DebugPrint( WL_WARNING, "Could not set DYAW: '%s' has an enemy (%s)!\n", ent->targetname, ent->enemy->targetname );
	}
}


/*
============
Q3_SetShootDist
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: float data
============
*/
static void Q3_SetShootDist( int entID, float data )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetShootDist: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetShootDist: '%s' is not an NPC\n", ent->targetname );
		return;
	}

	ent->NPC->stats.shootDistance = data;
}


/*
============
Q3_SetVisrange
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: float data
============
*/
static void Q3_SetVisrange( int entID, float data )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetVisrange: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetVisrange: '%s' is not an NPC\n", ent->targetname );
		return;
	}

	ent->NPC->stats.visrange = data;
}


/*
============
Q3_SetEarshot
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: float data
============
*/
static void Q3_SetEarshot( int entID, float data )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetEarshot: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetEarshot: '%s' is not an NPC\n", ent->targetname );
		return;
	}

	ent->NPC->stats.earshot = data;
}


/*
============
Q3_SetVigilance
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: float data
============
*/
static void Q3_SetVigilance( int entID, float data )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetVigilance: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetVigilance: '%s' is not an NPC\n", ent->targetname );
		return;
	}

	ent->NPC->stats.vigilance = data;
}


/*
============
Q3_SetVFOV
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: int data
============
*/
static void Q3_SetVFOV( int entID, int data )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetVFOV: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetVFOV: '%s' is not an NPC\n", ent->targetname );
		return;
	}

	ent->NPC->stats.vfov = data;
}


/*
============
Q3_SetHFOV
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: int data
============
*/
static void Q3_SetHFOV( int entID, int data )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetHFOV: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetHFOV: '%s' is not an NPC\n", ent->targetname );
		return;
	}

	ent->NPC->stats.hfov = data;
}


/*
============
Q3_SetWidth
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: float data
============
*/
static void Q3_SetWidth( int entID, int data )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetWidth: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetWidth: '%s' is not an NPC\n", ent->targetname );
		return;
	}

	ent->maxs[0] = ent->maxs[1] =  data;
	ent->mins[0] = ent->mins[1] = -data;
}

/*
============
Q3_GetTimeScale
  Description	:
  Return type	: static unsigned int
  Argument		: void
============
*/
static unsigned int Q3_GetTimeScale( void )
{
	//return	Q3_TIME_SCALE;
	return g_timescale->value;
}


/*
============
Q3_SetTimeScale
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: const char *data
============
*/
static void Q3_SetTimeScale( int entID, const char *data )
{
	gi.cvar_set("timescale", data);
}


/*
============
Q3_SetInvisible
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: qboolean invisible
============
*/
static void Q3_SetInvisible( int entID, qboolean invisible )
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetInvisible: invalid entID %d\n", entID);
		return;
	}

	if ( invisible )
	{
		self->s.eFlags |= EF_NODRAW;
		if ( self->client )
		{
			self->client->ps.eFlags |= EF_NODRAW;
		}
		self->contents = 0;
	}
	else
	{
		self->s.eFlags &= ~EF_NODRAW;
		if ( self->client )
		{
			self->client->ps.eFlags &= ~EF_NODRAW;
		}
	}
}

/*
============
Q3_SetVampire
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: qboolean vampire
============
*/
static void Q3_SetVampire( int entID, qboolean vampire )
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self || !self->client )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetVampire: entID %d not a client\n", entID);
		return;
	}

	if ( vampire )
	{
		self->client->ps.powerups[PW_DISINT_2] = Q3_INFINITE;
	}
	else
	{
		self->client->ps.powerups[PW_DISINT_2] = 0;
	}
}
/*
============
Q3_SetGreetAllies
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: qboolean greet
============
*/
static void Q3_SetGreetAllies( int entID, qboolean greet )
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetGreetAllies: invalid entID %d\n", entID);
		return;
	}

	if ( !self->NPC )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetGreetAllies: ent %s is not an NPC!\n", self->targetname );
		return;
	}

	if ( greet )
	{
		self->NPC->aiFlags |= NPCAI_GREET_ALLIES;
	}
	else
	{
		self->NPC->aiFlags &= ~NPCAI_GREET_ALLIES;
	}
}


/*
============
Q3_SetViewTarget
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: const char *name
============
*/
static void Q3_SetViewTarget (int entID, const char *name)
{
	gentity_t	*self  = &g_entities[entID];
	gentity_t	*viewtarget = G_Find( NULL, FOFS(targetname), (char *) name);
	vec3_t		viewspot, selfspot, viewvec, viewangles;

	if ( !self )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetViewTarget: invalid entID %d\n", entID);
		return;
	}

	if ( !self->client )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetViewTarget: '%s' is not a player/NPC!\n", self->targetname );
		return;
	}

	//FIXME: Exception handle here
	if (viewtarget == NULL)
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetViewTarget: can't find ViewTarget: '%s'\n", name );
		return;
	}

	//FIXME: should we set behavior to BS_FACE and keep facing this ent as it moves
	//around for a script-specified length of time...?
	VectorCopy ( self->currentOrigin, selfspot );
	selfspot[2] += self->client->ps.viewheight;

	if ( viewtarget->client && (!g_skippingcin || !g_skippingcin->integer ) )
	{
		VectorCopy ( viewtarget->client->renderInfo.eyePoint, viewspot );
	}
	else
	{
		VectorCopy ( viewtarget->currentOrigin, viewspot );
	}

	VectorSubtract( viewspot, selfspot, viewvec );

	vectoangles( viewvec, viewangles );

	Q3_SetDYaw( entID, viewangles[YAW] );
	if ( !g_skippingcin || !g_skippingcin->integer )
	{
		Q3_SetDPitch( entID, viewangles[PITCH] );
	}
}


/*
============
Q3_SetWatchTarget
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: const char *name
============
*/
static void Q3_SetWatchTarget (int entID, const char *name)
{
	gentity_t	*self  = &g_entities[entID];
	gentity_t	*watchTarget = NULL;

	if ( !self )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetWatchTarget: invalid entID %d\n", entID);
		return;
	}

	if ( !self->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetWatchTarget: '%s' is not an NPC!\n", self->targetname );
		return;
	}

	if ( Q_stricmp( "NULL", name ) == 0 || Q_stricmp( "NONE", name ) == 0 || ( self->targetname && (Q_stricmp( self->targetname, name ) == 0) ) )
	{//clearing watchTarget
		self->NPC->watchTarget = NULL;
	}

	watchTarget = G_Find( NULL, FOFS(targetname), (char *) name);
	if ( watchTarget == NULL )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetWatchTarget: can't find WatchTarget: '%s'\n", name );
		return;
	}

	self->NPC->watchTarget = watchTarget;
}

void Q3_SetLoopSound(int entID, const char *name)
{
	sfxHandle_t	index;
	gentity_t	*self  = &g_entities[entID];

	if ( Q_stricmp( "NULL", name ) == 0 || Q_stricmp( "NONE", name )==0)
	{
		self->s.loopSound = 0;
		return;
	}

	if ( self->s.eType == ET_MOVER )
	{
		index = cgi_S_RegisterSound( name );
	}
	else
	{
		index = G_SoundIndex( name );
	}

	if (index)
	{
		self->s.loopSound = index;
	}
	else
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetLoopSound: can't find sound file: '%s'\n", name );
	}
}

void Q3_SetICARUSFreeze( int entID, const char *name, qboolean freeze )
{
	gentity_t	*self  = G_Find( NULL, FOFS(targetname), name );
	if ( !self )
	{//hmm, targetname failed, try script_targetname?
		self = G_Find( NULL, FOFS(script_targetname), name );
	}

	if ( !self )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetICARUSFreeze: invalid ent %s\n", name);
		return;
	}

	if ( freeze )
	{
		self->svFlags |= SVF_ICARUS_FREEZE;
	}
	else
	{
		self->svFlags &= ~SVF_ICARUS_FREEZE;
	}
}

/*
============
Q3_SetViewEntity
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: const char *name
============
*/
extern qboolean G_ClearViewEntity( gentity_t *ent );
extern void G_SetViewEntity( gentity_t *self, gentity_t *viewEntity );
void Q3_SetViewEntity(int entID, const char *name)
{
	gentity_t	*self  = &g_entities[entID];
	gentity_t	*viewtarget = G_Find( NULL, FOFS(targetname), (char *) name);

	if ( entID != 0 )
	{//only valid on player
		Q3_DebugPrint( WL_ERROR, "Q3_SetViewEntity: only valid on player\n", entID);
		return;
	}

	if ( !self )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetViewEntity: invalid entID %d\n", entID);
		return;
	}

	if ( !self->client )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetViewEntity: '%s' is not a player!\n", self->targetname );
		return;
	}

	if ( !name )
	{
		G_ClearViewEntity( self );
		return;
	}

	if ( viewtarget == NULL )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetViewEntity: can't find ViewEntity: '%s'\n", name );
		return;
	}

	G_SetViewEntity( self, viewtarget );
}

/*
============
Q3_SetWeapon
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: const char *wp_name
============
*/
extern gentity_t *TossClientItems( gentity_t *self );
extern void G_CreateG2AttachedWeaponModel( gentity_t *ent, const char *weaponModel );
extern void WP_SaberInitBladeData( gentity_t *ent );
static void Q3_SetWeapon (int entID, const char *wp_name)
{
	gentity_t	*self  = &g_entities[entID];
	int		wp;
	qboolean	hadWeapon = qfalse;

	if ( !self )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetWeapon: invalid entID %d\n", entID);
		return;
	}

	if ( !self->client )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetWeapon: '%s' is not a player/NPC!\n", self->targetname );
		return;
	}

	if ( self->NPC )
	{//since a script sets a weapon, we presume we don't want to auto-match the player's weapon anymore
		self->NPC->aiFlags &= ~NPCAI_MATCHPLAYERWEAPON;
	}

	if(!Q_stricmp("drop", wp_name))
	{//no weapon, drop it
		TossClientItems( self );
		self->client->ps.weapon = WP_NONE;
		if ( self->weaponModel >= 0 )
		{
			gi.G2API_RemoveGhoul2Model( self->ghoul2, self->weaponModel );
			self->weaponModel = -1;
		}
		return;
	}

	wp = GetIDForString( WPTable, wp_name );
	if(wp == WP_NONE)
	{//no weapon
		self->client->ps.weapon = WP_NONE;
		if ( self->weaponModel >= 0 )
		{
			gi.G2API_RemoveGhoul2Model( self->ghoul2, self->weaponModel );
			self->weaponModel = -1;
		}
		return;
	}

	gitem_t *item = FindItemForWeapon( (weapon_t) wp);
	RegisterItem( item );	//make sure the weapon is cached in case this runs at startup

	if ( self->client->ps.stats[STAT_WEAPONS]&( 1 << wp ) )
	{
		hadWeapon = qtrue;
	}
	if ( self->NPC )
	{//Should NPCs have only 1 weapon at a time?
		self->client->ps.stats[STAT_WEAPONS] = ( 1 << wp );
		self->client->ps.ammo[weaponData[wp].ammoIndex] = 999;

		ChangeWeapon( self, wp );
		self->client->ps.weapon = wp;
		self->client->ps.weaponstate = WEAPON_READY;//WEAPON_RAISING;
		G_AddEvent( self, EV_GENERAL_SOUND, G_SoundIndex( "sound/weapons/change.wav" ));
	}
	else
	{
		self->client->ps.stats[STAT_WEAPONS] |= ( 1 << wp );
		self->client->ps.ammo[weaponData[wp].ammoIndex] = ammoData[weaponData[wp].ammoIndex].max;

		G_AddEvent( self, EV_ITEM_PICKUP, (item - bg_itemlist) );
		//force it to change
		CG_ChangeWeapon( wp );
		G_AddEvent( self, EV_GENERAL_SOUND, G_SoundIndex( "sound/weapons/change.wav" ));
	}
	if ( self->weaponModel >= 0 )
	{
		gi.G2API_RemoveGhoul2Model( self->ghoul2, self->weaponModel );
	}

	if ( wp == WP_SABER )
	{
		if ( !hadWeapon )
		{
			WP_SaberInitBladeData( self );
		}
		G_CreateG2AttachedWeaponModel( self, self->client->ps.saberModel );
	}
	else
	{
		G_CreateG2AttachedWeaponModel( self, weaponData[wp].weaponMdl );
	}
}

/*
============
Q3_SetItem
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: const char *wp_name
============
*/
static void Q3_SetItem (int entID, const char *item_name)
{
	gentity_t	*self  = &g_entities[entID];
	int		inv;

	if ( !self )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetWeapon: invalid entID %d\n", entID);
		return;
	}

	if ( !self->client )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetWeapon: '%s' is not a player/NPC!\n", self->targetname );
		return;
	}

	inv = GetIDForString( INVTable, item_name );


	gitem_t *item = FindItemForInventory(inv);
	RegisterItem( item );	//make sure the item is cached in case this runs at startup

//		G_AddEvent( self, EV_ITEM_PICKUP, (item - bg_itemlist) );
//		G_AddEvent( self, EV_GENERAL_SOUND, G_SoundIndex( "sound/weapons/change.wav" ));

	self->client->ps.stats[STAT_ITEMS] |= (1<<item->giTag);

	if( (inv == INV_ELECTROBINOCULARS) || (inv == INV_LIGHTAMP_GOGGLES) )
	{
		self->client->ps.inventory[inv] = 1;
		return;
	}
	// else Bacta, seeker, sentry
	if( self->client->ps.inventory[inv] < 5 )
	{
		self->client->ps.inventory[inv]++;
	}
}



/*
============
Q3_SetWalkSpeed
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: int int_data
============
*/
static void Q3_SetWalkSpeed (int entID, int int_data)
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetWalkSpeed: invalid entID %d\n", entID);
		return;
	}

	if ( !self->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetWalkSpeed: '%s' is not an NPC!\n", self->targetname );
		return;
	}

	if(int_data == 0)
	{
		self->NPC->stats.walkSpeed = self->client->ps.speed = 1;
	}

	self->NPC->stats.walkSpeed = self->client->ps.speed = int_data;
}


/*
============
Q3_SetRunSpeed
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: int int_data
============
*/
static void Q3_SetRunSpeed (int entID, int int_data)
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetRunSpeed: invalid entID %d\n", entID);
		return;
	}

	if ( !self->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetRunSpeed: '%s' is not an NPC!\n", self->targetname );
		return;
	}

	if(int_data == 0)
	{
		self->NPC->stats.runSpeed = self->client->ps.speed = 1;
	}

	self->NPC->stats.runSpeed = self->client->ps.speed = int_data;
}


/*
============
Q3_SetYawSpeed
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: float float_data
============
*/
static void Q3_SetYawSpeed (int entID, float float_data)
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetYawSpeed: invalid entID %d\n", entID);
		return;
	}

	if ( !self->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetYawSpeed: '%s' is not an NPC!\n", self->targetname );
		return;
	}

	self->NPC->stats.yawSpeed = float_data;
}


/*
============
Q3_SetAggression
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: int int_data
============
*/
static void Q3_SetAggression(int entID, int int_data)
{
	gentity_t	*self  = &g_entities[entID];


	if ( !self )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetAggression: invalid entID %d\n", entID);
		return;
	}

	if ( !self->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetAggression: '%s' is not an NPC!\n", self->targetname );
		return;
	}

	if(int_data < 1 || int_data > 5)
		return;

	self->NPC->stats.aggression = int_data;
}


/*
============
Q3_SetAim
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: int int_data
============
*/
static void Q3_SetAim(int entID, int int_data)
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetAim: invalid entID %d\n", entID);
		return;
	}

	if ( !self->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetAim: '%s' is not an NPC!\n", self->targetname );
		return;
	}

	if(int_data < 1 || int_data > 5)
		return;

	self->NPC->stats.aim = int_data;
}


/*
============
Q3_SetFriction
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: int int_data
============
*/
static void Q3_SetFriction(int entID, int int_data)
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetFriction: invalid entID %d\n", entID);
		return;
	}

	if ( !self->client )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetFriction: '%s' is not an NPC/player!\n", self->targetname );
		return;
	}

	self->client->ps.friction = int_data;
}


/*
============
Q3_SetGravity
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: float float_data
============
*/
static void Q3_SetGravity(int entID, float float_data)
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetGravity: invalid entID %d\n", entID);
		return;
	}

	if ( !self->client )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetGravity: '%s' is not an NPC/player!\n", self->targetname );
		return;
	}

	//FIXME: what if we want to return them to normal global gravity?
	self->svFlags |= SVF_CUSTOM_GRAVITY;
	self->client->ps.gravity = float_data;
}


/*
============
Q3_SetWait
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: float float_data
============
*/
static void Q3_SetWait(int entID, float float_data)
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetWait: invalid entID %d\n", entID);
		return;
	}

	self->wait = float_data;
}


static void Q3_SetShotSpacing(int entID, int int_data)
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetShotSpacing: invalid entID %d\n", entID);
		return;
	}

	if ( !self->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetShotSpacing: '%s' is not an NPC!\n", self->targetname );
		return;
	}

	self->NPC->aiFlags &= ~NPCAI_BURST_WEAPON;
	self->NPC->burstSpacing = int_data;
}

/*
============
Q3_SetFollowDist
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: float float_data
============
*/
static void Q3_SetFollowDist(int entID, float float_data)
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetFollowDist: invalid entID %d\n", entID);
		return;
	}

	if ( !self->client || !self->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetFollowDist: '%s' is not an NPC!\n", self->targetname );
		return;
	}

	self->NPC->followDist = float_data;
}


/*
============
Q3_SetScale
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: float float_data
============
*/
static void Q3_SetScale(int entID, float float_data)
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetScale: invalid entID %d\n", entID);
		return;
	}

	self->s.scale = float_data;
}


/*
============
Q3_SetCount
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: const char *data
============
*/
static void Q3_SetCount(int entID, const char *data)
{
	gentity_t	*self  = &g_entities[entID];
	float		val = 0.0f;

	//FIXME: use FOFS() stuff here to make a generic entity field setting?
	if ( !self )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetCount: invalid entID %d\n", entID);
		return;
	}

	if ( (val = Q3_CheckStringCounterIncrement( data )) )
	{
		self->count += (int)(val);
	}
	else
	{
		self->count = atoi((char *) data);
	}
}


/*
============
Q3_SetSquadName
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: const char *squadname
============
*/
/*
static void Q3_SetSquadName (int entID, const char *squadname)
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetSquadName: invalid entID %d\n", entID);
		return;
	}

	if ( !self->client )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetSquadName: '%s' is not an NPC/player!\n", self->targetname );
		return;
	}

	if(!Q_stricmp("NULL", ((char *)squadname)))
	{
		self->client->squadname = NULL;
	}
	else
	{
		self->client->squadname = G_NewString(squadname);
	}
}
*/

/*
============
Q3_SetTargetName
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: const char *targetname
============
*/
static void Q3_SetTargetName (int entID, const char *targetname)
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetTargetName: invalid entID %d\n", entID);
		return;
	}

	if(!Q_stricmp("NULL", ((char *)targetname)))
	{
		self->targetname = NULL;
	}
	else
	{
		self->targetname = G_NewString( targetname );
	}

	/*
	if ( self->NPC )
	{//uhh, this will may screw up ICARUS' associated ent list
		self->script_targetname = self->targetname;
	}
	*/
}


/*
============
Q3_SetTarget
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: const char *target
============
*/
static void Q3_SetTarget (int entID, const char *target)
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetTarget: invalid entID %d\n", entID);
		return;
	}

	if(!Q_stricmp("NULL", ((char *)target)))
	{
		self->target = NULL;
	}
	else
	{
		self->target = G_NewString( target );
	}
}

/*
============
Q3_SetTarget2
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: const char *target
============
*/
static void Q3_SetTarget2 (int entID, const char *target2)
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetTarget2: invalid entID %d\n", entID);
		return;
	}

	if(!Q_stricmp("NULL", ((char *)target2)))
	{
		self->target2 = NULL;
	}
	else
	{
		self->target2 = G_NewString( target2 );
	}
}
/*
============
Q3_SetRemoveTarget
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: const char *target
============
*/
static void Q3_SetRemoveTarget (int entID, const char *target)
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetRemoveTarget: invalid entID %d\n", entID);
		return;
	}

	if ( !self->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetRemoveTarget: '%s' is not an NPC!\n", self->targetname );
		return;
	}

	if( !Q_stricmp("NULL", ((char *)target)) )
	{
		self->target3 = NULL;
	}
	else
	{
		self->target3 = G_NewString( target );
	}
}


/*
============
Q3_SetPainTarget
  Description	:
  Return type	: void
  Argument		: int entID
  Argument		: const char *targetname
============
*/
static void Q3_SetPainTarget (int entID, const char *targetname)
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetPainTarget: invalid entID %d\n", entID);
		return;
	}

	if(Q_stricmp("NULL", ((char *)targetname)) == 0)
	{
		self->paintarget = NULL;
	}
	else
	{
		self->paintarget = G_NewString((char *)targetname);
	}
}

/*
============
Q3_SetFullName
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: const char *fullName
============
*/
static void Q3_SetFullName (int entID, const char *fullName)
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetFullName: invalid entID %d\n", entID);
		return;
	}

	if(!Q_stricmp("NULL", ((char *)fullName)))
	{
		self->fullName = NULL;
	}
	else
	{
		self->fullName = G_NewString( fullName );
	}
}

static void Q3_SetMusicState( const char *dms )
{
	int newDMS = GetIDForString( DMSTable, dms );
	if ( newDMS != -1 )
	{
		level.dmState = newDMS;
	}
}

static void Q3_SetForcePowerLevel ( int entID, int forcePower, int forceLevel )
{
	if ( forcePower < FP_FIRST || forceLevel >= NUM_FORCE_POWERS )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetForcePowerLevel: Force Power index %d out of range (%d-%d)\n", forcePower, FP_FIRST, (NUM_FORCE_POWERS-1) );
		return;
	}

	if ( forceLevel < 0 || forceLevel >= NUM_FORCE_POWER_LEVELS )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetForcePowerLevel: Force power setting %d out of range (0-3)\n", forceLevel );
		return;
	}

	gentity_t	*self  = &g_entities[entID];

	if ( !self )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetForcePowerLevel: invalid entID %d\n", entID);
		return;
	}

	if ( !self->client )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetForcePowerLevel: ent %s is not a player or NPC\n", self->targetname );
		return;
	}

	if ((forceLevel > self->client->ps.forcePowerLevel[forcePower]) && (entID==0) && (forceLevel > 0))
	{
		if (!cg_updatedDataPadForcePower1.integer)
		{
			missionInfo_Updated = qtrue;	// Activate flashing text
			gi.cvar_set("cg_updatedDataPadForcePower1", va("%d",forcePower+1)); // The +1 is offset in the print routine. It ain't pretty, I know.
			cg_updatedDataPadForcePower1.integer = forcePower+1;
		}
		else if (!cg_updatedDataPadForcePower2.integer)
		{
			missionInfo_Updated = qtrue;	// Activate flashing text
			gi.cvar_set("cg_updatedDataPadForcePower2", va("%d",forcePower+1)); // The +1 is offset in the print routine. It ain't pretty, I know.
			cg_updatedDataPadForcePower2.integer = forcePower+1;
		}
		else if (!cg_updatedDataPadForcePower3.integer)
		{
			missionInfo_Updated = qtrue;	// Activate flashing text
			gi.cvar_set("cg_updatedDataPadForcePower3", va("%d",forcePower+1)); // The +1 is offset in the print routine. It ain't pretty, I know.
			cg_updatedDataPadForcePower3.integer = forcePower+1;
		}
	}

	self->client->ps.forcePowerLevel[forcePower] = forceLevel;
	if ( forceLevel )
	{
		self->client->ps.forcePowersKnown |= ( 1 << forcePower );
	}
	else
	{
		self->client->ps.forcePowersKnown &= ~( 1 << forcePower );
	}
}
/*
============
Q3_SetParm
  Description	:
  Return type	: void
  Argument		: int entID
  Argument		: int parmNum
  Argument		: const char *parmValue
============
*/
void Q3_SetParm (int entID, int parmNum, const char *parmValue)
{
	gentity_t	*ent = &g_entities[entID];
	float		val;

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetParm: invalid entID %d\n", entID);
		return;
	}

	if ( parmNum < 0 || parmNum >= MAX_PARMS )
	{
		Q3_DebugPrint( WL_WARNING, "SET_PARM: parmNum %d out of range!\n", parmNum );
		return;
	}

	if( !ent->parms )
	{
		ent->parms = (parms_t *)G_Alloc( sizeof(parms_t) );
		memset( ent->parms, 0, sizeof(parms_t) );
	}

	if ( (val = Q3_CheckStringCounterIncrement( parmValue )) )
	{
		val += atof( ent->parms->parm[parmNum] );
		sprintf( ent->parms->parm[parmNum], "%f", val );
	}
	else
	{//Just copy the string
		//copy only 16 characters
		strncpy( ent->parms->parm[parmNum], parmValue, sizeof(ent->parms->parm[0]) );
		//set the last charcter to null in case we had to truncate their passed string
		if ( ent->parms->parm[parmNum][sizeof(ent->parms->parm[0]) - 1] != 0 )
		{//Tried to set a string that is too long
			ent->parms->parm[parmNum][sizeof(ent->parms->parm[0]) - 1] = 0;
			Q3_DebugPrint( WL_WARNING, "SET_PARM: parm%d string too long, truncated to '%s'!\n", parmNum, ent->parms->parm[parmNum] );
		}
	}
}



/*
=============
Q3_SetCaptureGoal

Sets the capture spot goal of an entity
=============
*/
static void Q3_SetCaptureGoal( int entID, const char *name )
{
	gentity_t	*ent  = &g_entities[entID];
	gentity_t	*goal = G_Find( NULL, FOFS(targetname), (char *) name);

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetCaptureGoal: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetCaptureGoal: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	//FIXME: Exception handle here
	if (goal == NULL)
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetCaptureGoal: can't find CaptureGoal target: '%s'\n", name );
		return;
	}

	if(ent->NPC)
	{
		ent->NPC->captureGoal = goal;
		ent->NPC->goalEntity = goal;
		ent->NPC->goalTime = level.time + 100000;
//		NAV_ClearLastRoute(ent);
	}
}

/*
=============
Q3_SetEvent

?
=============
*/
static void Q3_SetEvent( int entID, const char *event_name )
{
	gentity_t	*ent  = &g_entities[entID];
//	gentity_t	*tent  = NULL;
	int			event;
//	vec3_t		spot;

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetEvent: invalid entID %d\n", entID);
		return;
	}

	event = GetIDForString( eventTable, event_name );
	switch( event )
	{
/*
	case EV_DISINTEGRATE:
		if( VectorCompare( ent->currentOrigin, vec3_origin ) )
		{//Brush with no origin
			VectorSubtract( ent->absmax, ent->absmin, spot );
			VectorMA( ent->absmin, 0.5, spot, spot );
		}
		else
		{
			VectorCopy( ent->currentOrigin, spot );
			spot[2] += ent->maxs[2]/2;
		}
		tent = G_TempEntity( spot, EV_DISINTEGRATION );
		tent->s.eventParm = PW_REGEN;
		tent->owner = ent;
		break;

*/
	case EV_BAD:
	default:
		//Q3_DebugPrint( WL_ERROR,"Q3_SetEvent: Invalid Event %d\n", event );
		return;
		break;
	}
}

/*
============
Q3_Use

Uses an entity
============
*/
static void Q3_Use( int entID, const char *target )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_Use: invalid entID %d\n", entID);
		return;
	}

	if( !target || !target[0] )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_Use: string is NULL!\n" );
		return;
	}

	G_UseTargets2(ent, ent, target);
}


/*
============
Q3_SetBehaviorSet

?
============
*/
static qboolean Q3_SetBehaviorSet( int entID, int toSet, const char *scriptname)
{
	gentity_t	*ent  = &g_entities[entID];
	bSet_t		bSet = BSET_INVALID;

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetBehaviorSet: invalid entID %d\n", entID);
		return qfalse;
	}

	switch(toSet)
	{
	case SET_SPAWNSCRIPT:
		bSet = BSET_SPAWN;
		break;
	case SET_USESCRIPT:
		bSet = BSET_USE;
		break;
	case SET_AWAKESCRIPT:
		bSet = BSET_AWAKE;
		break;
	case SET_ANGERSCRIPT:
		bSet = BSET_ANGER;
		break;
	case SET_ATTACKSCRIPT:
		bSet = BSET_ATTACK;
		break;
	case SET_VICTORYSCRIPT:
		bSet = BSET_VICTORY;
		break;
	case SET_LOSTENEMYSCRIPT:
		bSet = BSET_LOSTENEMY;
		break;
	case SET_PAINSCRIPT:
		bSet = BSET_PAIN;
		break;
	case SET_FLEESCRIPT:
		bSet = BSET_FLEE;
		break;
	case SET_DEATHSCRIPT:
		bSet = BSET_DEATH;
		break;
	case SET_DELAYEDSCRIPT:
		bSet = BSET_DELAYED;
		break;
	case SET_BLOCKEDSCRIPT:
		bSet = BSET_BLOCKED;
		break;
	case SET_FFIRESCRIPT:
		bSet = BSET_FFIRE;
		break;
	case SET_FFDEATHSCRIPT:
		bSet = BSET_FFDEATH;
		break;
	case SET_MINDTRICKSCRIPT:
		bSet = BSET_MINDTRICK;
		break;
	}

	if(bSet < BSET_SPAWN || bSet >= NUM_BSETS)
	{
		return qfalse;
	}

	if(!Q_stricmp("NULL", scriptname))
	{
		if ( ent->behaviorSet[bSet] != NULL )
		{
//			gi.TagFree( ent->behaviorSet[bSet] );
		}

		ent->behaviorSet[bSet] = NULL;
		//memset( &ent->behaviorSet[bSet], 0, sizeof(ent->behaviorSet[bSet]) );
	}
	else
	{
		if ( scriptname )
		{
			if ( ent->behaviorSet[bSet] != NULL )
			{
//				gi.TagFree( ent->behaviorSet[bSet] );
			}

			ent->behaviorSet[bSet] = G_NewString( (char *) scriptname );	//FIXME: This really isn't good...
		}

		//ent->behaviorSet[bSet] = scriptname;
		//strncpy( (char *) &ent->behaviorSet[bSet], scriptname, MAX_BSET_LENGTH );
	}
	return qtrue;
}

/*
============
Q3_SetDelayScriptTime

?
============
*/
static void Q3_SetDelayScriptTime(int entID, int delayTime)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetDelayScriptTime: invalid entID %d\n", entID);
		return;
	}

	ent->delayScriptTime = level.time + delayTime;
}


/*
============
Q3_SetIgnorePain

?
============
*/
static void Q3_SetIgnorePain( int entID, qboolean data)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetIgnorePain: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetIgnorePain: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	ent->NPC->ignorePain = data;
}

/*
============
Q3_SetIgnoreEnemies

?
============
*/
static void Q3_SetIgnoreEnemies( int entID, qboolean data)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetIgnoreEnemies: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetIgnoreEnemies: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if(data)
	{
		ent->svFlags |= SVF_IGNORE_ENEMIES;
	}
	else
	{
		ent->svFlags &= ~SVF_IGNORE_ENEMIES;
	}
}

/*
============
Q3_SetIgnoreAlerts

?
============
*/
static void Q3_SetIgnoreAlerts( int entID, qboolean data)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetIgnoreAlerts: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetIgnoreAlerts: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if(data)
	{
		ent->NPC->scriptFlags |= SCF_IGNORE_ALERTS;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_IGNORE_ALERTS;
	}
}


/*
============
Q3_SetNoTarget

?
============
*/
static void Q3_SetNoTarget( int entID, qboolean data)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetNoTarget: invalid entID %d\n", entID);
		return;
	}

	if(data)
		ent->flags |= FL_NOTARGET;
	else
		ent->flags &= ~FL_NOTARGET;
}

/*
============
Q3_SetDontShoot

?
============
*/
static void Q3_SetDontShoot( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetDontShoot: invalid entID %d\n", entID);
		return;
	}

	if(add)
	{
		ent->flags |= FL_DONT_SHOOT;
	}
	else
	{
		ent->flags &= ~FL_DONT_SHOOT;
	}
}

/*
============
Q3_SetDontFire

?
============
*/
static void Q3_SetDontFire( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetDontFire: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetDontFire: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if(add)
	{
		ent->NPC->scriptFlags |= SCF_DONT_FIRE;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_DONT_FIRE;
	}
}

/*
============
Q3_SetFireWeapon

?
============
*/
static void Q3_SetFireWeapon(int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_FireWeapon: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetFireWeapon: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if(add)
	{
		ent->NPC->scriptFlags |= SCF_FIRE_WEAPON;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_FIRE_WEAPON;
	}
}


/*
============
Q3_SetInactive

?
============
*/
static void Q3_SetInactive(int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetInactive: invalid entID %d\n", entID);
		return;
	}

	if(add)
	{
		ent->svFlags |= SVF_INACTIVE;
	}
	else
	{
		ent->svFlags &= ~SVF_INACTIVE;
	}
}

/*
============
Q3_SetFuncUsableVisible

?
============
*/
static void Q3_SetFuncUsableVisible(int entID, qboolean visible )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetFuncUsableVisible: invalid entID %d\n", entID);
		return;
	}

	// Yeah, I know that this doesn't even do half of what the func_usable use code does, but if I've got two things on top of each other...and only
	//	one is visible at a time....and neither can ever be used......and finally, the shader on it has the shader_anim stuff going on....It doesn't seem
	//	like I can easily use the other version without nasty side effects.
	if( visible )
	{
		ent->svFlags &= ~SVF_NOCLIENT;
		ent->s.eFlags &= ~EF_NODRAW;
	}
	else
	{
		ent->svFlags |= SVF_NOCLIENT;
		ent->s.eFlags |= EF_NODRAW;
	}
}

/*
============
Q3_SetLockedEnemy

?
============
*/
static void Q3_SetLockedEnemy ( int entID, qboolean locked)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetLockedEnemy: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetLockedEnemy: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	//FIXME: make an NPCAI_FLAG
	if(locked)
	{
		ent->svFlags |= SVF_LOCKEDENEMY;
	}
	else
	{
		ent->svFlags &= ~SVF_LOCKEDENEMY;
	}
}

char cinematicSkipScript[1024];

/*
============
Q3_SetCinematicSkipScript

============
*/
static void Q3_SetCinematicSkipScript( char *scriptname )
{

	if(Q_stricmp("none", scriptname) == 0 || Q_stricmp("NULL", scriptname) == 0)
	{
		memset( cinematicSkipScript, 0, sizeof( cinematicSkipScript ) );
	}
	else
	{
		Q_strncpyz(cinematicSkipScript,scriptname,sizeof(cinematicSkipScript));
	}

}

/*
============
Q3_SetNoMindTrick

?
============
*/
static void Q3_SetNoMindTrick( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetNoMindTrick: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetNoMindTrick: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if(add)
	{
		ent->NPC->scriptFlags |= SCF_NO_MIND_TRICK;
		ent->NPC->confusionTime = 0;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_NO_MIND_TRICK;
	}
}

/*
============
Q3_SetCrouched

?
============
*/
static void Q3_SetCrouched( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetCrouched: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetCrouched: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if(add)
	{
		ent->NPC->scriptFlags |= SCF_CROUCHED;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_CROUCHED;
	}
}

/*
============
Q3_SetWalking

?
============
*/
static void Q3_SetWalking( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetWalking: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetWalking: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if(add)
	{
		ent->NPC->scriptFlags |= SCF_WALKING;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_WALKING;
	}
}

/*
============
Q3_SetRunning

?
============
*/
static void Q3_SetRunning( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetRunning: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetRunning: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if(add)
	{
		ent->NPC->scriptFlags |= SCF_RUNNING;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_RUNNING;
	}
}

/*
============
Q3_SetForcedMarch

?
============
*/
static void Q3_SetForcedMarch( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetForcedMarch: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetForcedMarch: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if(add)
	{
		ent->NPC->scriptFlags |= SCF_FORCED_MARCH;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_FORCED_MARCH;
	}
}
/*
============
Q3_SetChaseEnemies

indicates whether the npc should chase after an enemy
============
*/
static void Q3_SetChaseEnemies( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetChaseEnemies: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetChaseEnemies: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if(add)
	{
		ent->NPC->scriptFlags |= SCF_CHASE_ENEMIES;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_CHASE_ENEMIES;
	}
}

/*
============
Q3_SetLookForEnemies

if set npc will be on the look out for potential enemies
if not set, npc will ignore enemies
============
*/
static void Q3_SetLookForEnemies( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetLookForEnemies: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetLookForEnemies: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if(add)
	{
		ent->NPC->scriptFlags |= SCF_LOOK_FOR_ENEMIES;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_LOOK_FOR_ENEMIES;
	}
}

/*
============
Q3_SetFaceMoveDir

============
*/
static void Q3_SetFaceMoveDir( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetFaceMoveDir: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetFaceMoveDir: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if(add)
	{
		ent->NPC->scriptFlags |= SCF_FACE_MOVE_DIR;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_FACE_MOVE_DIR;
	}
}

/*
============
Q3_SetAltFire

?
============
*/
static void Q3_SetAltFire( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetAltFire: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetAltFire: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if(add)
	{
		ent->NPC->scriptFlags |= SCF_ALT_FIRE;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_ALT_FIRE;
	}

	ChangeWeapon( ent, 	ent->client->ps.weapon );

}

/*
============
Q3_SetDontFlee

?
============
*/
static void Q3_SetDontFlee( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetDontFlee: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetDontFlee: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if(add)
	{
		ent->NPC->scriptFlags |= SCF_DONT_FLEE;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_DONT_FLEE;
	}
}

/*
============
Q3_SetNoResponse

?
============
*/
static void Q3_SetNoResponse( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetNoResponse: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetNoResponse: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if(add)
	{
		ent->NPC->scriptFlags |= SCF_NO_RESPONSE;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_NO_RESPONSE;
	}
}

/*
============
Q3_SetCombatTalk

?
============
*/
static void Q3_SetCombatTalk( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetCombatTalk: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetCombatTalk: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if ( add )
	{
		ent->NPC->scriptFlags |= SCF_NO_COMBAT_TALK;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_NO_COMBAT_TALK;
	}
}

/*
============
Q3_SetAlertTalk

?
============
*/
static void Q3_SetAlertTalk( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetAlertTalk: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetAlertTalk: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if ( add )
	{
		ent->NPC->scriptFlags |= SCF_NO_ALERT_TALK;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_NO_ALERT_TALK;
	}
}

/*
============
Q3_SetUseCpNearest

?
============
*/
static void Q3_SetUseCpNearest( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetUseCpNearest: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetUseCpNearest: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if ( add )
	{
		ent->NPC->scriptFlags |= SCF_USE_CP_NEAREST;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_USE_CP_NEAREST;
	}
}

/*
============
Q3_SetNoForce

?
============
*/
static void Q3_SetNoForce( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetNoForce: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetNoForce: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if ( add )
	{
		ent->NPC->scriptFlags |= SCF_NO_FORCE;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_NO_FORCE;
	}
}

/*
============
Q3_SetNoAcrobatics

?
============
*/
static void Q3_SetNoAcrobatics( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetNoAcrobatics: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetNoAcrobatics: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if ( add )
	{
		ent->NPC->scriptFlags |= SCF_NO_ACROBATICS;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_NO_ACROBATICS;
	}
}

/*
============
Q3_SetUseSubtitles

?
============
*/
static void Q3_SetUseSubtitles( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetUseSubtitles: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetUseSubtitles: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if ( add )
	{
		ent->NPC->scriptFlags |= SCF_USE_SUBTITLES;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_USE_SUBTITLES;
	}
}

/*
============
Q3_SetNoFallToDeath

?
============
*/
static void Q3_SetNoFallToDeath( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetNoFallToDeath: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetNoFallToDeath: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if ( add )
	{
		ent->NPC->scriptFlags |= SCF_NO_FALLTODEATH;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_NO_FALLTODEATH;
	}
}

/*
============
Q3_SetDismemberable

?
============
*/
static void Q3_SetDismemberable( int entID, qboolean dismemberable)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetDismemberable: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->client )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetDismemberable: '%s' is not an client!\n", ent->targetname );
		return;
	}

	ent->client->dismembered = (qboolean)(!dismemberable);
}


/*
============
Q3_SetMoreLight

?
============
*/
static void Q3_SetMoreLight( int entID, qboolean add )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetMoreLight: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetMoreLight: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if ( add )
	{
		ent->NPC->scriptFlags |= SCF_MORELIGHT;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_MORELIGHT;
	}
}

/*
============
Q3_SetUndying

?
============
*/
static void Q3_SetUndying( int entID, qboolean undying)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetUndying: invalid entID %d\n", entID);
		return;
	}

	if(undying)
	{
		ent->flags |= FL_UNDYING;
	}
	else
	{
		ent->flags &= ~FL_UNDYING;
	}
}

/*
============
Q3_SetInvincible

?
============
*/
static void Q3_SetInvincible( int entID, qboolean invincible)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetInvincible: invalid entID %d\n", entID);
		return;
	}

	if ( !Q_stricmp( "func_breakable", ent->classname ) )
	{
		if ( invincible )
		{
			ent->spawnflags |= 1;
		}
		else
		{
			ent->spawnflags &= ~1;
		}
		return;
	}

	if ( invincible )
	{
		ent->flags |= FL_GODMODE;
	}
	else
	{
		ent->flags &= ~FL_GODMODE;
	}
}
/*
============
Q3_SetForceInvincible
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: qboolean forceInv
============
*/
static void Q3_SetForceInvincible( int entID, qboolean forceInv )
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self || !self->client )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetForceInvincible: entID %d not a client\n", entID);
		return;
	}

	Q3_SetInvincible( entID, forceInv );
	if ( forceInv )
	{
		self->client->ps.powerups[PW_INVINCIBLE] = Q3_INFINITE;
	}
	else
	{
		self->client->ps.powerups[PW_INVINCIBLE] = 0;
	}
}

/*
============
Q3_SetNoAvoid

?
============
*/
static void Q3_SetNoAvoid( int entID, qboolean noAvoid)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetNoAvoid: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetNoAvoid: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if(noAvoid)
	{
		ent->NPC->aiFlags |= NPCAI_NO_COLL_AVOID;
	}
	else
	{
		ent->NPC->aiFlags &= ~NPCAI_NO_COLL_AVOID;
	}
}

/*
============
SolidifyOwner
  Description	:
  Return type	: void
  Argument		: gentity_t *self
============
*/
void SolidifyOwner( gentity_t *self )
{
	self->nextthink = level.time + FRAMETIME;
	self->e_ThinkFunc = thinkF_G_FreeEntity;

	if ( !self->owner || !self->owner->inuse )
	{
		return;
	}

	int oldContents = self->owner->contents;
	self->owner->contents = CONTENTS_BODY;
	if ( SpotWouldTelefrag2( self->owner, self->owner->currentOrigin ) )
	{
		self->owner->contents = oldContents;
		self->e_ThinkFunc = thinkF_SolidifyOwner;
	}
	else
	{
		if ( self->owner->NPC && !(self->owner->spawnflags & SFB_NOTSOLID) )
		{
			self->owner->clipmask |= CONTENTS_BODY;
		}
		Q3_TaskIDComplete( self->owner, TID_RESIZE );
	}
}


/*
============
Q3_SetSolid

?
============
*/
static qboolean Q3_SetSolid( int entID, qboolean solid)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetSolid: invalid entID %d\n", entID);
		return qtrue;
	}

	if ( solid )
	{//FIXME: Presumption
		int oldContents = ent->contents;
		ent->contents = CONTENTS_BODY;
		if ( SpotWouldTelefrag2( ent, ent->currentOrigin ) )
		{
			gentity_t *solidifier = G_Spawn();

			solidifier->owner = ent;

			solidifier->e_ThinkFunc = thinkF_SolidifyOwner;
			solidifier->nextthink = level.time + FRAMETIME;

			ent->contents = oldContents;
			return qfalse;
		}
		ent->clipmask |= CONTENTS_BODY;
	}
	else
	{//FIXME: Presumption
		if ( ent->s.eFlags & EF_NODRAW )
		{//We're invisible too, so set contents to none
			ent->contents = 0;
		}
		else
		{
			ent->contents = CONTENTS_CORPSE;
		}
		if ( ent->NPC )
		{
			if(!(ent->spawnflags & SFB_NOTSOLID))
			{
				ent->clipmask &= ~CONTENTS_BODY;
			}
		}
	}
	return qtrue;
}
/*
============
Q3_SetLean

?
============
*/
#define LEAN_NONE	0
#define LEAN_RIGHT	1
#define LEAN_LEFT	2
static void Q3_SetLean( int entID, int lean)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetLean: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetLean: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if(lean == LEAN_RIGHT)
	{
		ent->NPC->scriptFlags |= SCF_LEAN_RIGHT;
		ent->NPC->scriptFlags &= ~SCF_LEAN_LEFT;
	}
	else if(lean == LEAN_LEFT)
	{
		ent->NPC->scriptFlags |= SCF_LEAN_LEFT;
		ent->NPC->scriptFlags &= ~SCF_LEAN_RIGHT;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_LEAN_LEFT;
		ent->NPC->scriptFlags &= ~SCF_LEAN_RIGHT;
	}
}

/*
============
Q3_SetForwardMove

?
============
*/
static void Q3_SetForwardMove( int entID, int fmoveVal)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetForwardMove: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->client )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetForwardMove: '%s' is not an NPC/player!\n", ent->targetname );
		return;
	}

	ent->client->forced_forwardmove = fmoveVal;
}

/*
============
Q3_SetRightMove

?
============
*/
static void Q3_SetRightMove( int entID, int rmoveVal)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetRightMove: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->client )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetRightMove: '%s' is not an NPC/player!\n", ent->targetname );
		return;
	}

	ent->client->forced_rightmove = rmoveVal;
}

/*
============
Q3_SetLockAngle

?
============
*/
static void Q3_SetLockAngle( int entID, const char *lockAngle)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetLockAngle: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->client )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetLockAngle: '%s' is not an NPC/player!\n", ent->targetname );
		return;
	}

	if(Q_stricmp("off", lockAngle) == 0)
	{//free it
		ent->client->renderInfo.renderFlags &= ~RF_LOCKEDANGLE;
	}
	else
	{
		ent->client->renderInfo.renderFlags |= RF_LOCKEDANGLE;


		if(Q_stricmp("auto", lockAngle) == 0)
		{//use current yaw
			if( ent->NPC )	// I need this to work on NPCs, so their locked value
			{
				ent->NPC->lockedDesiredYaw =  NPC->client->ps.viewangles[YAW]; // could also set s.angles[1] and desiredYaw to this value...
			}
			else
			{
				ent->client->renderInfo.lockYaw = ent->client->ps.viewangles[YAW];
			}
		}
		else
		{//specified yaw
			if( ent->NPC ) // I need this to work on NPCs, so their locked value
			{
				ent->NPC->lockedDesiredYaw = atof((char *)lockAngle); // could also set s.angles[1] and desiredYaw to this value...
			}
			else
			{
				ent->client->renderInfo.lockYaw = atof((char *)lockAngle);
			}
		}
	}
}


/*
============
Q3_CameraGroup

?
============
*/
static void Q3_CameraGroup( int entID, char *camG)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_CameraGroup: invalid entID %d\n", entID);
		return;
	}

	ent->cameraGroup = G_NewString(camG);
}

extern camera_t	client_camera;
/*
============
Q3_CameraGroupZOfs

?
============
*/
static void Q3_CameraGroupZOfs( float camGZOfs )
{
	client_camera.cameraGroupZOfs = camGZOfs;
}
/*
============
Q3_CameraGroup

?
============
*/
static void Q3_CameraGroupTag( char *camGTag )
{
	Q_strncpyz( client_camera.cameraGroupTag, camGTag, sizeof(client_camera.cameraGroupTag) );
}

/*
============
Q3_RemoveRHandModel
============
*/
static void Q3_RemoveRHandModel( int entID, char *addModel)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( ent->cinematicModel >= 0 )
	{
		gi.G2API_RemoveGhoul2Model(ent->ghoul2,ent->cinematicModel);
	}
}

/*
============
Q3_AddRHandModel
============
*/
static void Q3_AddRHandModel( int entID, char *addModel)
{
	gentity_t	*ent  = &g_entities[entID];

	ent->cinematicModel = gi.G2API_InitGhoul2Model(ent->ghoul2, addModel, G_ModelIndex( addModel ), NULL_HANDLE, NULL_HANDLE, 0, 0);
	if ( ent->cinematicModel != -1 )
	{
		// attach it to the hand
		gi.G2API_AttachG2Model(&ent->ghoul2[ent->cinematicModel], &ent->ghoul2[ent->playerModel],
					ent->handRBolt, ent->playerModel);
	}
}

/*
============
Q3_AddLHandModel
============
*/
static void Q3_AddLHandModel( int entID, char *addModel)
{
	gentity_t	*ent  = &g_entities[entID];

	ent->cinematicModel = gi.G2API_InitGhoul2Model(ent->ghoul2, addModel, G_ModelIndex( addModel ), NULL_HANDLE, NULL_HANDLE, 0, 0);
	if ( ent->cinematicModel != -1 )
	{
		// attach it to the hand
		gi.G2API_AttachG2Model(&ent->ghoul2[ent->cinematicModel], &ent->ghoul2[ent->playerModel],
					ent->handLBolt, ent->playerModel);
	}
}

/*
============
Q3_RemoveLHandModel
============
*/
static void Q3_RemoveLHandModel( int entID, char *addModel)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( ent->cinematicModel >= 0 )
	{
		gi.G2API_RemoveGhoul2Model(ent->ghoul2, ent->cinematicModel);
	}
}

/*
============
Q3_LookTarget

?
============
*/
static void Q3_LookTarget( int entID, char *targetName)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_LookTarget: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->client )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_LookTarget: '%s' is not an NPC/player!\n", ent->targetname );
		return;
	}

	if(Q_stricmp("none", targetName) == 0 || Q_stricmp("NULL", targetName) == 0)
	{//clearing look target
		NPC_ClearLookTarget( ent );
		return;
	}

	gentity_t	*targ  = G_Find(NULL, FOFS(targetname), targetName);
	if(!targ)
	{
		Q3_DebugPrint( WL_ERROR, "Q3_LookTarget: Can't find ent %s\n", targetName );
		return;
	}

	NPC_SetLookTarget( ent, targ->s.number, 0 );
}

/*
============
Q3_Face

?
============
*/
static void Q3_Face( int entID,int expression, float holdtime)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_Face: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->client )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_Face: '%s' is not an NPC/player!\n", ent->targetname );
		return;
	}

	//FIXME: change to milliseconds to be consistant!
	holdtime *= 1000;

	switch(expression)
	{
	case SET_FACEEYESCLOSED:
		ent->client->facial_blink = 1;
		break;
	case SET_FACEEYESOPENED:
		ent->client->facial_blink = -1;
		break;
	case SET_FACEAUX:
		ent->client->facial_aux = -(level.time + holdtime);

		ent->client->facial_blink = level.time + holdtime + Q_flrand(3000.0, 5000.0);
		ent->client->facial_frown = level.time + holdtime + Q_flrand(6000.0, 10000.0);
		break;
	case SET_FACEBLINK:
		ent->client->facial_blink = -(level.time + holdtime);

		ent->client->facial_aux = level.time + holdtime + Q_flrand(6000.0, 10000.0);
		ent->client->facial_frown = level.time + holdtime + Q_flrand(6000.0, 10000.0);
		break;

	case SET_FACEBLINKFROWN:
		ent->client->facial_blink = -(level.time + holdtime);
		ent->client->facial_frown = -(level.time + holdtime);

		ent->client->facial_aux = level.time + holdtime + Q_flrand(6000.0, 10000.0);
		break;

	case SET_FACEFROWN:
		ent->client->facial_frown = -(level.time + holdtime);

		ent->client->facial_aux = level.time + holdtime + Q_flrand(6000.0, 10000.0);
		ent->client->facial_blink = level.time + holdtime + Q_flrand(3000.0, 5000.0);
		break;

	case SET_FACENORMAL:
		ent->client->facial_aux = level.time + Q_flrand(6000.0, 10000.0);
		ent->client->facial_blink = level.time + Q_flrand(3000.0, 5000.0);
		ent->client->facial_frown = level.time + holdtime + Q_flrand(6000.0, 10000.0);
		break;
	}

}


/*
============
Q3_SetPlayerUsable
  Description	:
  Return type	: void
  Argument		:  int entID
  Argument		: qboolean usable
============
*/
static void Q3_SetPlayerUsable( int entID, qboolean usable )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetPlayerUsable: invalid entID %d\n", entID);
		return;
	}

	if(usable)
	{
		ent->svFlags |= SVF_PLAYER_USABLE;
	}
	else
	{
		ent->svFlags &= ~SVF_PLAYER_USABLE;
	}
}

/*
============
Q3_SetDisableShaderAnims
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: int disabled
============
*/
static void Q3_SetDisableShaderAnims( int entID, int disabled )
{
	gentity_t *ent = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetDisableShaderAnims: invalid entID %d\n", entID);
		return;
	}

	if ( disabled )
	{
		ent->s.eFlags |= EF_DISABLE_SHADER_ANIM;
	}
	else
	{
		ent->s.eFlags &= ~EF_DISABLE_SHADER_ANIM;
	}
}

/*
============
Q3_SetShaderAnim
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: int disabled
============
*/
static void Q3_SetShaderAnim( int entID, int disabled )
{
	gentity_t *ent = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetShaderAnim: invalid entID %d\n", entID);
		return;
	}

	if ( disabled )
	{
		ent->s.eFlags |= EF_SHADER_ANIM;
	}
	else
	{
		ent->s.eFlags &= ~EF_SHADER_ANIM;
	}
}

/*
============
Q3_SetStartFrame
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: int startFrame
============
*/
static void Q3_SetStartFrame( int entID, int startFrame )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetStartFrame: invalid entID %d\n", entID);
		return;
	}

	if ( ent->client )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetLoopAnim: command not valid on players/NPCs!\n" );
		return;
	}

	if ( startFrame >= 0 )
	{
		ent->s.frame = startFrame;
		ent->startFrame = startFrame;
	}
}


/*
============
Q3_SetEndFrame
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: int endFrame
============
*/
static void Q3_SetEndFrame( int entID, int endFrame )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetEndFrame: invalid entID %d\n", entID);
		return;
	}

	if ( ent->client )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetEndFrame: command not valid on players/NPCs!\n" );
		return;
	}

	if ( endFrame >= 0 )
	{
		ent->endFrame = endFrame;
	}
}

/*
============
Q3_SetAnimFrame
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: int startFrame
============
*/
static void Q3_SetAnimFrame( int entID, int animFrame )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetAnimFrame: invalid entID %d\n", entID);
		return;
	}

	if ( ent->client )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetAnimFrame: command not valid on players/NPCs!\n" );
		return;
	}

	if ( animFrame >= ent->endFrame )
	{
		ent->s.frame = ent->endFrame;
	}
	else if ( animFrame >= ent->startFrame )
	{
		ent->s.frame = animFrame;
	}
	else
	{
		// FIXME/NOTE: Set s.frame anyway??
		Q3_DebugPrint( WL_WARNING, "Q3_SetAnimFrame: value must be valid number between StartFrame and EndFrame.\n" );
		return;
	}
}

void InflateOwner( gentity_t *self )
{
	self->nextthink = level.time + FRAMETIME;
	self->e_ThinkFunc = thinkF_G_FreeEntity;

	if ( !self->owner || !self->owner->inuse )
	{
		return;
	}

	trace_t	trace;

	gi.trace( &trace, self->currentOrigin, self->mins, self->maxs, self->currentOrigin, self->owner->s.number, self->owner->clipmask&~(CONTENTS_SOLID|CONTENTS_MONSTERCLIP), G2_NOCOLLIDE, 0 );
	if ( trace.allsolid || trace.startsolid )
	{
		self->e_ThinkFunc = thinkF_InflateOwner;
		return;
	}

	if ( Q3_TaskIDPending( self->owner, TID_RESIZE ) )
	{
		Q3_TaskIDComplete( self->owner, TID_RESIZE );

		VectorCopy( self->mins, self->owner->mins );
		VectorCopy( self->maxs, self->owner->maxs );
		gi.linkentity( self->owner );
	}
}


/*
============
Q3_SetLoopAnim
  Description	:
  Return type	: void
  Argument		:  int entID
  Argument		: qboolean loopAnim
============
*/
static void Q3_SetLoopAnim( int entID, qboolean loopAnim )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetLoopAnim: invalid entID %d\n", entID);
		return;
	}

	if ( ent->client )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetLoopAnim: command not valid on players/NPCs!\n" );
		return;
	}

	ent->loopAnim = loopAnim;
}


/*
============
Q3_SetShields
  Description	:
  Return type	: void
  Argument		:  int entID
  Argument		: qboolean shields
============
*/
static void Q3_SetShields( int entID, qboolean shields )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetShields: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetShields: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if ( shields )
	{
		ent->NPC->aiFlags |= NPCAI_SHIELDS;
	}
	else
	{
		ent->NPC->aiFlags &= ~NPCAI_SHIELDS;
	}
}

/*
============
Q3_SetSaberActive
  Description	:
  Return type	: void
  Argument		:  int entID
  Argument		: qboolean shields
============
*/
static void Q3_SetSaberActive( int entID, qboolean active )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetSaberActive: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->client )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetSaberActive: '%s' is not an player/NPC!\n", ent->targetname );
		return;
	}

	if ( ent->client->ps.weapon != WP_SABER )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_SetSaberActive: '%s' is not using a saber!\n", ent->targetname );
		return;
	}
	//FIXME: need an externalized call for the sound and any other internal changes?
	ent->client->ps.saberActive = active;
}

/*
============
Q3_SetNoKnockback
  Description	:
  Return type	: void
  Argument		:  int entID
  Argument		: qboolean noKnockback
============
*/
static void Q3_SetNoKnockback( int entID, qboolean noKnockback )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_SetNoKnockback: invalid entID %d\n", entID);
		return;
	}

	if ( noKnockback )
	{
		ent->flags |= FL_NO_KNOCKBACK;
	}
	else
	{
		ent->flags &= ~FL_NO_KNOCKBACK;
	}
}

/*
============
Q3_SetCleanDamagingEnts
  Description	:
  Return type	: void
============
*/
static void Q3_SetCleanDamagingEnts( void )
{
	gentity_t *ent = NULL;

	for ( int i = 0; i < ENTITYNUM_WORLD; i++ )
	{
		if ( !PInUse( i ))
		{
			continue;
		}

		ent = &g_entities[i];

		if ( ent )
		{
			if ( !ent->client && ( ent->s.weapon == WP_DET_PACK || ent->s.weapon == WP_TRIP_MINE || ent->s.weapon == WP_THERMAL ))
			{
				// check for a client, otherwise we could remove someone holding this weapon
				G_FreeEntity( ent );
			}
			else if ( ent->s.weapon == WP_TURRET && ent->activator && ent->activator->s.number == 0 && !Q_stricmp( "PAS", ent->classname ))
			{
				// is a player owner personal assault sentry gun.
				G_FreeEntity( ent );
			}
			else if ( ent->client && ent->client->NPC_class == CLASS_SEEKER )
			{
				// they blow up when they run out of ammo, so this may as well just do the same.
				G_Damage( ent, ent, ent, NULL, NULL, 999, 0, MOD_UNKNOWN );
			}
		}
	}
}


/*
============
Q3_SetInterface
  Description	:
  Return type	: void
  Argument		:  int entID
  Argument		: const char *data
============
*/
static void Q3_SetInterface( int entID, const char *data )
{
	gi.cvar_set("cg_drawStatus", data);
}

/*
============
Q3_SetLocation
  Description	:
  Return type	: qboolean
  Argument		:  int entID
  Argument		: const char *location
============
*/
static qboolean Q3_SetLocation( int entID, const char *location )
{
	gentity_t	*ent = &g_entities[entID];
	char		*currentLoc;

	if ( !ent )
	{
		return qtrue;
	}

	currentLoc = G_GetLocationForEnt( ent );
	if ( currentLoc && currentLoc[0] && Q_stricmp( location, currentLoc ) == 0 )
	{
		return qtrue;
	}

	ent->message = G_NewString( location );
	return qfalse;
}

/*
============
Q3_SetPlayerLocked
  Description	:
  Return type	: void
  Argument		:  int entID
  Argument		: qboolean locked
============
*/
qboolean	player_locked = qfalse;
static void Q3_SetPlayerLocked( int entID, qboolean locked )
{
	gentity_t	*ent = &g_entities[0];

	player_locked = locked;
	if ( ent && ent->client )
	{//stop him too
		VectorClear(ent->client->ps.velocity);
	}
}

/*
============
Q3_SetLockPlayerWeapons
  Description	:
  Return type	: void
  Argument		:  int entID
  Argument		: qboolean locked
============
*/
static void Q3_SetLockPlayerWeapons( int entID, qboolean locked )
{
	gentity_t	*ent = &g_entities[0];

	ent->flags &= ~FL_LOCK_PLAYER_WEAPONS;

	if( locked )
	{
		ent->flags |= FL_LOCK_PLAYER_WEAPONS;
	}

}


/*
============
Q3_SetNoImpactDamage
  Description	:
  Return type	: void
  Argument		:  int entID
  Argument		: qboolean locked
============
*/
static void Q3_SetNoImpactDamage( int entID, qboolean noImp )
{
	gentity_t	*ent = &g_entities[0];

	ent->flags &= ~FL_NO_IMPACT_DMG;

	if( noImp )
	{
		ent->flags |= FL_NO_IMPACT_DMG;
	}

}

extern void CG_CameraAutoAim( const char *name );
extern void CG_CameraAutoTrack( const char *name );

/*
============
Q3_SetVar
  Description	:
  Return type	: static void
  Argument		:  int taskID
  Argument		: int entID
  Argument		: const char *type_name
  Argument		: const char *data
============
*/
void Q3_SetVar( int taskID, int entID, const char *type_name, const char *data )
{
	int	vret = Q3_VariableDeclared( type_name ) ;
	float	float_data;
	float	val = 0.0f;


	if ( vret != VTYPE_NONE )
	{
		switch ( vret )
		{
		case VTYPE_FLOAT:
			//Check to see if increment command
			if ( (val = Q3_CheckStringCounterIncrement( data )) )
			{
				Q3_GetFloatVariable( type_name, &float_data );
				float_data += val;
			}
			else
			{
				float_data = atof((char *) data);
			}
			Q3_SetFloatVariable( type_name, float_data );
			break;

		case VTYPE_STRING:
			Q3_SetStringVariable( type_name, data );
			break;

		case VTYPE_VECTOR:
			Q3_SetVectorVariable( type_name, (char *) data );
			break;
		}

		return;
	}

	Q3_DebugPrint( WL_ERROR, "%s variable or field not found!\n", type_name );
}


/*
============
Q3_Set
  Description	:
  Return type	: void
  Argument		:  int taskID
  Argument		: int entID
  Argument		: const char *type_name
  Argument		: const char *data
============
*/
static void Q3_Set( int taskID, int entID, const char *type_name, const char *data )
{
	gentity_t	*ent = &g_entities[entID];
	float		float_data;
	int			int_data, toSet;
	vec3_t		vector_data;

	// eezstreet: Add support for cvars getting modified thru ICARUS script
	if(strlen(type_name) > 5 && !Q_stricmpn(type_name, "cvar_", 5))
	{
		gi.cvar_set(type_name+5, data);
		return;
	}

	//Set this for callbacks
	toSet = GetIDForString( setTable, type_name );

	//TODO: Throw in a showscript command that will list each command and what they're doing...
	//		maybe as simple as printing that line of the script to the console preceeded by the person's name?
	//		showscript can take any number of targetnames or "all"?  Groupname?
	switch ( toSet )
	{
	case SET_ORIGIN:
		sscanf( data, "%f %f %f", &vector_data[0], &vector_data[1], &vector_data[2] );
		G_SetOrigin( ent, vector_data );
		if ( strncmp( "NPC_", ent->classname, 4 ) == 0 )
		{//hack for moving spawners
			VectorCopy( vector_data, ent->s.origin);
		}
		break;

	case SET_TELEPORT_DEST:
		sscanf( data, "%f %f %f", &vector_data[0], &vector_data[1], &vector_data[2] );
		if ( !Q3_SetTeleportDest( entID, vector_data ) )
		{
			Q3_TaskIDSet( ent, TID_MOVE_NAV, taskID );
			return;
		}
		break;

	case SET_COPY_ORIGIN:
		Q3_SetCopyOrigin( entID, (char *) data );
		break;

	case SET_ANGLES:
		//Q3_SetAngles( entID, *(vec3_t *) data);
		sscanf( data, "%f %f %f", &vector_data[0], &vector_data[1], &vector_data[2] );
		Q3_SetAngles( entID, vector_data);
		break;

	case SET_XVELOCITY:
		float_data = atof((char *) data);
		Q3_SetVelocity( entID, 0, float_data);
		break;

	case SET_YVELOCITY:
		float_data = atof((char *) data);
		Q3_SetVelocity( entID, 1, float_data);
		break;

	case SET_ZVELOCITY:
		float_data = atof((char *) data);
		Q3_SetVelocity( entID, 2, float_data);
		break;

	case SET_Z_OFFSET:
		float_data = atof((char *) data);
		Q3_SetOriginOffset( entID, 2, float_data);
		break;

	case SET_ENEMY:
		Q3_SetEnemy( entID, (char *) data );
		break;

	case SET_LEADER:
		Q3_SetLeader( entID, (char *) data );
		break;

	case SET_NAVGOAL:
		Q3_SetNavGoal( entID, (char *) data );
		Q3_TaskIDSet( ent, TID_MOVE_NAV, taskID );
		return;	//Don't call it back
		break;

	case SET_ANIM_UPPER:
		if ( Q3_SetAnimUpper( entID, (char *) data ) )
		{
			Q3_TaskIDClear( &ent->taskID[TID_ANIM_BOTH] );//We only want to wait for the top
			Q3_TaskIDSet( ent, TID_ANIM_UPPER, taskID );
			return;	//Don't call it back
		}
		break;

	case SET_ANIM_LOWER:
		if ( Q3_SetAnimLower( entID, (char *) data ) )
		{
			Q3_TaskIDClear( &ent->taskID[TID_ANIM_BOTH] );//We only want to wait for the bottom
			Q3_TaskIDSet( ent, TID_ANIM_LOWER, taskID );
			return;	//Don't call it back
		}
		break;

	case SET_ANIM_BOTH:
		{
			int	both = 0;
			if ( Q3_SetAnimUpper( entID, (char *) data ) )
			{
				Q3_TaskIDSet( ent, TID_ANIM_UPPER, taskID );
				both++;
			}
			else
			{
				Q3_DebugPrint( WL_ERROR, "Q3_SetAnimUpper: %s does not have anim %s!\n", ent->targetname, (char *)data );
			}
			if ( Q3_SetAnimLower( entID, (char *) data ) )
			{
				Q3_TaskIDSet( ent, TID_ANIM_LOWER, taskID );
				both++;
			}
			else
			{
				Q3_DebugPrint( WL_ERROR, "Q3_SetAnimLower: %s does not have anim %s!\n", ent->targetname, (char *)data );
			}
			if ( both >= 2 )
			{
				Q3_TaskIDSet( ent, TID_ANIM_BOTH, taskID );
			}
			if ( both )
			{
				return;	//Don't call it back
			}
		}
		break;

	case SET_ANIM_HOLDTIME_LOWER:
		int_data = atoi((char *) data);
		Q3_SetAnimHoldTime( entID, int_data, qtrue );
		Q3_TaskIDClear( &ent->taskID[TID_ANIM_BOTH] );//We only want to wait for the bottom
		Q3_TaskIDSet( ent, TID_ANIM_LOWER, taskID );
		return;	//Don't call it back
		break;

	case SET_ANIM_HOLDTIME_UPPER:
		int_data = atoi((char *) data);
		Q3_SetAnimHoldTime( entID, int_data, qfalse );
		Q3_TaskIDClear( &ent->taskID[TID_ANIM_BOTH] );//We only want to wait for the top
		Q3_TaskIDSet( ent, TID_ANIM_UPPER, taskID );
		return;	//Don't call it back
		break;

	case SET_ANIM_HOLDTIME_BOTH:
		int_data = atoi((char *) data);
		Q3_SetAnimHoldTime( entID, int_data, qfalse );
		Q3_SetAnimHoldTime( entID, int_data, qtrue );
		Q3_TaskIDSet( ent, TID_ANIM_BOTH, taskID );
		Q3_TaskIDSet( ent, TID_ANIM_UPPER, taskID );
		Q3_TaskIDSet( ent, TID_ANIM_LOWER, taskID );
		return;	//Don't call it back
		break;

	case SET_PLAYER_TEAM:
		Q3_SetPlayerTeam( entID, (char *) data );
		break;

	case SET_ENEMY_TEAM:
		Q3_SetEnemyTeam( entID, (char *) data );
		break;

	case SET_HEALTH:
		int_data = atoi((char *) data);
		Q3_SetHealth( entID, int_data );
		break;

	case SET_ARMOR:
		int_data = atoi((char *) data);
		Q3_SetArmor( entID, int_data );
		break;

	case SET_BEHAVIOR_STATE:
		if( !Q3_SetBState( entID, (char *) data ) )
		{
			Q3_TaskIDSet( ent, TID_BSTATE, taskID );
			return;//don't complete
		}
		break;

	case SET_DEFAULT_BSTATE:
		Q3_SetDefaultBState( entID, (char *) data );
		break;

	case SET_TEMP_BSTATE:
		if( !Q3_SetTempBState( entID, (char *) data ) )
		{
			Q3_TaskIDSet( ent, TID_BSTATE, taskID );
			return;//don't complete
		}
		break;

	case SET_CAPTURE:
		Q3_SetCaptureGoal( entID, (char *) data );
		break;

	case SET_DPITCH://FIXME: make these set tempBehavior to BS_FACE and await completion?  Or set lockedDesiredPitch/Yaw and aimTime?
		float_data = atof((char *) data);
		Q3_SetDPitch( entID, float_data );
		Q3_TaskIDSet( ent, TID_ANGLE_FACE, taskID );
		return;
		break;

	case SET_DYAW:
		float_data = atof((char *) data);
		Q3_SetDYaw( entID, float_data );
		Q3_TaskIDSet( ent, TID_ANGLE_FACE, taskID );
		return;
		break;

	case SET_EVENT:
		Q3_SetEvent( entID, (char *) data );
		break;

	case SET_VIEWTARGET:
		Q3_SetViewTarget( entID, (char *) data );
		Q3_TaskIDSet( ent, TID_ANGLE_FACE, taskID );
		return;
		break;

	case SET_WATCHTARGET:
		Q3_SetWatchTarget( entID, (char *) data );
		break;

	case SET_VIEWENTITY:
		Q3_SetViewEntity( entID, (char *) data );
		break;

	case SET_LOOPSOUND:
		Q3_SetLoopSound( entID, (char *) data );
		break;

	case SET_ICARUS_FREEZE:
	case SET_ICARUS_UNFREEZE:
		Q3_SetICARUSFreeze( entID, (char *) data, (qboolean)(toSet == SET_ICARUS_FREEZE) );
		break;

	case SET_WEAPON:
		Q3_SetWeapon ( entID, (char *) data);
		break;

	case SET_ITEM:
		Q3_SetItem ( entID, (char *) data);
		break;

	case SET_WALKSPEED:
		int_data = atoi((char *) data);
		Q3_SetWalkSpeed ( entID, int_data);
		break;

	case SET_RUNSPEED:
		int_data = atoi((char *) data);
		Q3_SetRunSpeed ( entID, int_data);
		break;

	case SET_WIDTH:
		int_data = atoi((char *) data);
		Q3_SetWidth( entID, int_data );
		return;
		break;

	case SET_YAWSPEED:
		float_data = atof((char *) data);
		Q3_SetYawSpeed ( entID, float_data);
		break;

	case SET_AGGRESSION:
		int_data = atoi((char *) data);
		Q3_SetAggression ( entID, int_data);
		break;

	case SET_AIM:
		int_data = atoi((char *) data);
		Q3_SetAim ( entID, int_data);
		break;

	case SET_FRICTION:
		int_data = atoi((char *) data);
		Q3_SetFriction ( entID, int_data);
		break;

	case SET_GRAVITY:
		float_data = atof((char *) data);
		Q3_SetGravity ( entID, float_data);
		break;

	case SET_WAIT:
		float_data = atof((char *) data);
		Q3_SetWait( entID, float_data);
		break;

	case SET_FOLLOWDIST:
		float_data = atof((char *) data);
		Q3_SetFollowDist( entID, float_data);
		break;

	case SET_SCALE:
		float_data = atof((char *) data);
		Q3_SetScale( entID, float_data);
		break;

	case SET_COUNT:
		Q3_SetCount( entID, (char *) data);
		break;

	case SET_SHOT_SPACING:
		int_data = atoi((char *) data);
		Q3_SetShotSpacing( entID, int_data );
		break;

	case SET_IGNOREPAIN:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetIgnorePain( entID, qtrue);
		else if(!Q_stricmp("false", ((char *)data)))
			Q3_SetIgnorePain( entID, qfalse);
		break;

	case SET_IGNOREENEMIES:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetIgnoreEnemies( entID, qtrue);
		else if(!Q_stricmp("false", ((char *)data)))
			Q3_SetIgnoreEnemies( entID, qfalse);
		break;

	case SET_IGNOREALERTS:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetIgnoreAlerts( entID, qtrue);
		else if(!Q_stricmp("false", ((char *)data)))
			Q3_SetIgnoreAlerts( entID, qfalse);
		break;

	case SET_DONTSHOOT:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetDontShoot( entID, qtrue);
		else if(!Q_stricmp("false", ((char *)data)))
			Q3_SetDontShoot( entID, qfalse);
		break;

	case SET_DONTFIRE:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetDontFire( entID, qtrue);
		else if(!Q_stricmp("false", ((char *)data)))
			Q3_SetDontFire( entID, qfalse);
		break;

	case SET_LOCKED_ENEMY:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetLockedEnemy( entID, qtrue);
		else if(!Q_stricmp("false", ((char *)data)))
			Q3_SetLockedEnemy( entID, qfalse);
		break;

	case SET_NOTARGET:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetNoTarget( entID, qtrue);
		else if(!Q_stricmp("false", ((char *)data)))
			Q3_SetNoTarget( entID, qfalse);
		break;

	case SET_LEAN:
		if(!Q_stricmp("right", ((char *)data)))
			Q3_SetLean( entID, LEAN_RIGHT);
		else if(!Q_stricmp("left", ((char *)data)))
			Q3_SetLean( entID, LEAN_LEFT);
		else
			Q3_SetLean( entID, LEAN_NONE);
		break;

	case SET_SHOOTDIST:
		float_data = atof((char *) data);
		Q3_SetShootDist( entID, float_data );
		break;

	case SET_TIMESCALE:
		Q3_SetTimeScale( entID, (char *) data );
		break;

	case SET_VISRANGE:
		float_data = atof((char *) data);
		Q3_SetVisrange( entID, float_data );
		break;

	case SET_EARSHOT:
		float_data = atof((char *) data);
		Q3_SetEarshot( entID, float_data );
		break;

	case SET_VIGILANCE:
		float_data = atof((char *) data);
		Q3_SetVigilance( entID, float_data );
		break;

	case SET_VFOV:
		int_data = atoi((char *) data);
		Q3_SetVFOV( entID, int_data );
		break;

	case SET_HFOV:
		int_data = atoi((char *) data);
		Q3_SetHFOV( entID, int_data );
		break;

	case SET_TARGETNAME:
		Q3_SetTargetName( entID, (char *) data );
		break;

	case SET_TARGET:
		Q3_SetTarget( entID, (char *) data );
		break;

	case SET_TARGET2:
		Q3_SetTarget2( entID, (char *) data );
		break;

	case SET_LOCATION:
		if ( !Q3_SetLocation( entID, (char *) data ) )
		{
			Q3_TaskIDSet( ent, TID_LOCATION, taskID );
			return;
		}
		break;

	case SET_PAINTARGET:
		Q3_SetPainTarget( entID, (char *) data );
		break;

	case SET_DEFEND_TARGET:
		Q3_DebugPrint( WL_WARNING, "Q3_SetDefendTarget unimplemented\n", entID );
		//Q3_SetEnemy( entID, (char *) data);
		break;

	case SET_PARM1:
	case SET_PARM2:
	case SET_PARM3:
	case SET_PARM4:
	case SET_PARM5:
	case SET_PARM6:
	case SET_PARM7:
	case SET_PARM8:
	case SET_PARM9:
	case SET_PARM10:
	case SET_PARM11:
	case SET_PARM12:
	case SET_PARM13:
	case SET_PARM14:
	case SET_PARM15:
	case SET_PARM16:
		Q3_SetParm( entID, (toSet-SET_PARM1), (char *) data );
		break;

	case SET_SPAWNSCRIPT:
	case SET_USESCRIPT:
	case SET_AWAKESCRIPT:
	case SET_ANGERSCRIPT:
	case SET_ATTACKSCRIPT:
	case SET_VICTORYSCRIPT:
	case SET_PAINSCRIPT:
	case SET_FLEESCRIPT:
	case SET_DEATHSCRIPT:
	case SET_DELAYEDSCRIPT:
	case SET_BLOCKEDSCRIPT:
	case SET_FFIRESCRIPT:
	case SET_FFDEATHSCRIPT:
	case SET_MINDTRICKSCRIPT:
		if( !Q3_SetBehaviorSet(entID, toSet, (char *) data) )
			Q3_DebugPrint( WL_ERROR, "Q3_SetBehaviorSet: Invalid bSet %s\n", type_name );
		break;

	case SET_NO_MINDTRICK:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetNoMindTrick( entID, qtrue);
		else
			Q3_SetNoMindTrick( entID, qfalse);
		break;

	case SET_CINEMATIC_SKIPSCRIPT :
		Q3_SetCinematicSkipScript((char *) data);
		break;


	case SET_DELAYSCRIPTTIME:
		int_data = atoi((char *) data);
		Q3_SetDelayScriptTime( entID, int_data );
		break;

	case SET_CROUCHED:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetCrouched( entID, qtrue);
		else
			Q3_SetCrouched( entID, qfalse);
		break;

	case SET_WALKING:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetWalking( entID, qtrue);
		else
			Q3_SetWalking( entID, qfalse);
		break;

	case SET_RUNNING:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetRunning( entID, qtrue);
		else
			Q3_SetRunning( entID, qfalse);
		break;

	case SET_CHASE_ENEMIES:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetChaseEnemies( entID, qtrue);
		else
			Q3_SetChaseEnemies( entID, qfalse);
		break;

	case SET_LOOK_FOR_ENEMIES:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetLookForEnemies( entID, qtrue);
		else
			Q3_SetLookForEnemies( entID, qfalse);
		break;

	case SET_FACE_MOVE_DIR:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetFaceMoveDir( entID, qtrue);
		else
			Q3_SetFaceMoveDir( entID, qfalse);
		break;

	case SET_ALT_FIRE:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetAltFire( entID, qtrue);
		else
			Q3_SetAltFire( entID, qfalse);
		break;

	case SET_DONT_FLEE:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetDontFlee( entID, qtrue);
		else
			Q3_SetDontFlee( entID, qfalse);
		break;

	case SET_FORCED_MARCH:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetForcedMarch( entID, qtrue);
		else
			Q3_SetForcedMarch( entID, qfalse);
		break;

	case SET_NO_RESPONSE:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetNoResponse( entID, qtrue);
		else
			Q3_SetNoResponse( entID, qfalse);
		break;

	case SET_NO_COMBAT_TALK:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetCombatTalk( entID, qtrue);
		else
			Q3_SetCombatTalk( entID, qfalse);
		break;

	case SET_NO_ALERT_TALK:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetAlertTalk( entID, qtrue);
		else
			Q3_SetAlertTalk( entID, qfalse);
		break;

	case SET_USE_CP_NEAREST:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetUseCpNearest( entID, qtrue);
		else
			Q3_SetUseCpNearest( entID, qfalse);
		break;

	case SET_NO_FORCE:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetNoForce( entID, qtrue);
		else
			Q3_SetNoForce( entID, qfalse);
		break;

	case SET_NO_ACROBATICS:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetNoAcrobatics( entID, qtrue);
		else
			Q3_SetNoAcrobatics( entID, qfalse);
		break;

	case SET_USE_SUBTITLES:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetUseSubtitles( entID, qtrue);
		else
			Q3_SetUseSubtitles( entID, qfalse);
		break;

	case SET_NO_FALLTODEATH:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetNoFallToDeath( entID, qtrue);
		else
			Q3_SetNoFallToDeath( entID, qfalse);
		break;

	case SET_DISMEMBERABLE:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetDismemberable( entID, qtrue);
		else
			Q3_SetDismemberable( entID, qfalse);
		break;

	case SET_MORELIGHT:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetMoreLight( entID, qtrue);
		else
			Q3_SetMoreLight( entID, qfalse);
		break;


	case SET_TREASONED:
		Q3_DebugPrint( WL_VERBOSE, "SET_TREASONED is disabled, do not use\n" );
		/*
		G_TeamRetaliation( NULL, &g_entities[0], qfalse );
		ffireLevel = FFIRE_LEVEL_RETALIATION;
		*/
		break;

	case SET_UNDYING:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetUndying( entID, qtrue);
		else
			Q3_SetUndying( entID, qfalse);
		break;

	case SET_INVINCIBLE:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetInvincible( entID, qtrue);
		else
			Q3_SetInvincible( entID, qfalse);
		break;

	case SET_NOAVOID:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetNoAvoid( entID, qtrue);
		else
			Q3_SetNoAvoid( entID, qfalse);
		break;

	case SET_SOLID:
		if(!Q_stricmp("true", ((char *)data)))
		{
			if ( !Q3_SetSolid( entID, qtrue) )
			{
				Q3_TaskIDSet( ent, TID_RESIZE, taskID );
				return;
			}
		}
		else
		{
			Q3_SetSolid( entID, qfalse);
		}
		break;

	case SET_INVISIBLE:
		if( !Q_stricmp("true", ((char *)data)) )
			Q3_SetInvisible( entID, qtrue );
		else
			Q3_SetInvisible( entID, qfalse );
		break;

	case SET_VAMPIRE:
		if( !Q_stricmp("true", ((char *)data)) )
			Q3_SetVampire( entID, qtrue );
		else
			Q3_SetVampire( entID, qfalse );
		break;

	case SET_FORCE_INVINCIBLE:
		if( !Q_stricmp("true", ((char *)data)) )
			Q3_SetForceInvincible( entID, qtrue );
		else
			Q3_SetForceInvincible( entID, qfalse );
		break;

	case SET_GREET_ALLIES:
		if( !Q_stricmp("true", ((char *)data)) )
			Q3_SetGreetAllies( entID, qtrue );
		else
			Q3_SetGreetAllies( entID, qfalse );
		break;

	case SET_PLAYER_LOCKED:
		if( !Q_stricmp("true", ((char *)data)) )
			Q3_SetPlayerLocked( entID, qtrue );
		else
			Q3_SetPlayerLocked( entID, qfalse );
		break;

	case SET_LOCK_PLAYER_WEAPONS:
		if( !Q_stricmp("true", ((char *)data)) )
			Q3_SetLockPlayerWeapons( entID, qtrue );
		else
			Q3_SetLockPlayerWeapons( entID, qfalse );
		break;

	case SET_NO_IMPACT_DAMAGE:
		if( !Q_stricmp("true", ((char *)data)) )
			Q3_SetNoImpactDamage( entID, qtrue );
		else
			Q3_SetNoImpactDamage( entID, qfalse );
		break;

	case SET_FORWARDMOVE:
		int_data = atoi((char *) data);
		Q3_SetForwardMove( entID, int_data);
		break;

	case SET_RIGHTMOVE:
		int_data = atoi((char *) data);
		Q3_SetRightMove( entID, int_data);
		break;

	case SET_LOCKYAW:
		Q3_SetLockAngle( entID, data);
		break;

	case SET_CAMERA_GROUP:
		Q3_CameraGroup(entID, (char *)data);
		break;
	case SET_CAMERA_GROUP_Z_OFS:
		float_data = atof((char *) data);
		Q3_CameraGroupZOfs( float_data );
		break;
	case SET_CAMERA_GROUP_TAG:
		Q3_CameraGroupTag( (char *)data );
		break;

	//FIXME: put these into camera commands
	case SET_LOOK_TARGET:
		Q3_LookTarget(entID, (char *)data);
		break;

	case SET_ADDRHANDBOLT_MODEL:
		Q3_AddRHandModel(entID, (char *)data);
		break;

	case SET_REMOVERHANDBOLT_MODEL:
		Q3_RemoveRHandModel(entID, (char *)data);
		break;

	case SET_ADDLHANDBOLT_MODEL:
		Q3_AddLHandModel(entID, (char *)data);
		break;

	case SET_REMOVELHANDBOLT_MODEL:
		Q3_RemoveLHandModel(entID, (char *)data);
		break;

	case SET_FACEEYESCLOSED:
	case SET_FACEEYESOPENED:
	case SET_FACEAUX:
	case SET_FACEBLINK:
	case SET_FACEBLINKFROWN:
	case SET_FACEFROWN:
	case SET_FACENORMAL:
		float_data = atof((char *) data);
		Q3_Face(entID, toSet, float_data);
		break;

	case SET_SCROLLTEXT:
		Q3_ScrollText( (char *)data );
		break;

	case SET_LCARSTEXT:
		Q3_LCARSText( (char *)data );
		break;

	case SET_CAPTIONTEXTCOLOR:
		Q3_SetCaptionTextColor ( (char *)data );
		break;
	case SET_CENTERTEXTCOLOR:
		Q3_SetCenterTextColor ( (char *)data );
		break;
	case SET_SCROLLTEXTCOLOR:
		Q3_SetScrollTextColor ( (char *)data );
		break;

	case SET_PLAYER_USABLE:
		if(!Q_stricmp("true", ((char *)data)))
		{
			Q3_SetPlayerUsable(entID, qtrue);
		}
		else
		{
			Q3_SetPlayerUsable(entID, qfalse);
		}
		break;

	case SET_STARTFRAME:
		int_data = atoi((char *) data);
		Q3_SetStartFrame(entID, int_data);
		break;

	case SET_ENDFRAME:
		int_data = atoi((char *) data);
		Q3_SetEndFrame(entID, int_data);

		Q3_TaskIDSet( ent, TID_ANIM_BOTH, taskID );
		return;
		break;

	case SET_ANIMFRAME:
		int_data = atoi((char *) data);
		Q3_SetAnimFrame(entID, int_data);
		return;
		break;

	case SET_LOOP_ANIM:
		if(!Q_stricmp("true", ((char *)data)))
		{
			Q3_SetLoopAnim(entID, qtrue);
		}
		else
		{
			Q3_SetLoopAnim(entID, qfalse);
		}
		break;

	case SET_INTERFACE:
		if(!Q_stricmp("true", ((char *)data)))
		{
			Q3_SetInterface(entID, "1");
		}
		else
		{
			Q3_SetInterface(entID, "0");
		}

		break;

	case SET_SHIELDS:
		if(!Q_stricmp("true", ((char *)data)))
		{
			Q3_SetShields(entID, qtrue);
		}
		else
		{
			Q3_SetShields(entID, qfalse);
		}
		break;

	case SET_SABERACTIVE:
		if(!Q_stricmp("true", ((char *)data)))
		{
			Q3_SetSaberActive( entID, qtrue );
		}
		else
		{
			Q3_SetSaberActive( entID, qfalse );
		}
		break;

	case SET_ADJUST_AREA_PORTALS:
		if(!Q_stricmp("true", ((char *)data)))
		{
			Q3_SetAdjustAreaPortals( entID, qtrue );
		}
		else
		{
			Q3_SetAdjustAreaPortals( entID, qfalse );
		}
		break;

	case SET_DMG_BY_HEAVY_WEAP_ONLY:
		if(!Q_stricmp("true", ((char *)data)))
		{
			Q3_SetDmgByHeavyWeapOnly( entID, qtrue );
		}
		else
		{
			Q3_SetDmgByHeavyWeapOnly( entID, qfalse );
		}
		break;

	case SET_SHIELDED:
		if(!Q_stricmp("true", ((char *)data)))
		{
			Q3_SetShielded( entID, qtrue );
		}
		else
		{
			Q3_SetShielded( entID, qfalse );
		}
		break;

	case SET_NO_GROUPS:
		if(!Q_stricmp("true", ((char *)data)))
		{
			Q3_SetNoGroups( entID, qtrue );
		}
		else
		{
			Q3_SetNoGroups( entID, qfalse );
		}
		break;

	case SET_FIRE_WEAPON:
		if(!Q_stricmp("true", ((char *)data)))
		{
			Q3_SetFireWeapon( entID, qtrue);
		}
		else if(!Q_stricmp("false", ((char *)data)))
		{
			Q3_SetFireWeapon( entID, qfalse);
		}
		break;

	case SET_INACTIVE:
		if(!Q_stricmp("true", ((char *)data)))
		{
			Q3_SetInactive( entID, qtrue);
		}
		else if(!Q_stricmp("false", ((char *)data)))
		{
			Q3_SetInactive( entID, qfalse);
		}
		else if(!Q_stricmp("unlocked", ((char *)data)))
		{
extern void UnLockDoors(gentity_t *const ent);
			UnLockDoors(&g_entities[entID]);
		}
		else if(!Q_stricmp("locked", ((char *)data)))
		{
extern void LockDoors(gentity_t *const ent);
			LockDoors(&g_entities[entID]);
		}
		break;

	case SET_END_SCREENDISSOLVE:
		gi.SendConsoleCommand( "endscreendissolve\n");
		break;

	case SET_MISSION_STATUS_SCREEN:
		gi.cvar_set("cg_missionstatusscreen", "1");
		break;

	case SET_FUNC_USABLE_VISIBLE:
		if(!Q_stricmp("true", ((char *)data)))
		{
			Q3_SetFuncUsableVisible( entID, qtrue);
		}
		else if(!Q_stricmp("false", ((char *)data)))
		{
			Q3_SetFuncUsableVisible( entID, qfalse);
		}
		break;

	case SET_NO_KNOCKBACK:
		if(!Q_stricmp("true", ((char *)data)))
		{
			Q3_SetNoKnockback(entID, qtrue);
		}
		else
		{
			Q3_SetNoKnockback(entID, qfalse);
		}
		break;

	case SET_VIDEO_PLAY:
		// don't do this check now, James doesn't want a scripted cinematic to also skip any Video cinematics as well,
		//	the "timescale" and "skippingCinematic" cvars will be set back to normal in the Video code, so doing a
		//	skip will now only skip one section of a multiple-part story (eg VOY1 bridge sequence)
		//
//		if ( g_timescale->value <= 1.0f )
		{
			gi.SendConsoleCommand( va("inGameCinematic %s\n", (char *)data) );
		}
		break;

	case SET_VIDEO_FADE_IN:
		if(!Q_stricmp("true", ((char *)data)))
		{
			gi.cvar_set("cl_VidFadeUp", "1");
		}
		else
		{
			gi.cvar_set("cl_VidFadeUp", "0");
		}
		break;

	case SET_VIDEO_FADE_OUT:
		if(!Q_stricmp("true", ((char *)data)))
		{
			gi.cvar_set("cl_VidFadeDown", "1");
		}
		else
		{
			gi.cvar_set("cl_VidFadeDown", "0");
		}
		break;
	case SET_REMOVE_TARGET:
		Q3_SetRemoveTarget( entID, (const char *) data );
		break;

	case SET_LOADGAME:
		gi.SendConsoleCommand( va("load %s\n", (const char *) data ) );
		break;

	case SET_MENU_SCREEN:
		//UI_SetActiveMenu( (const char *) data );
		break;

	case SET_OBJECTIVE_SHOW:
		missionInfo_Updated = qtrue;	// Activate flashing text
		gi.cvar_set("cg_updatedDataPadObjective", "1");

		Q3_SetObjective((const char *) data ,SET_OBJ_SHOW);
		Q3_SetObjective((const char *) data ,SET_OBJ_PENDING);
		break;
	case SET_OBJECTIVE_HIDE:
		Q3_SetObjective((const char *) data ,SET_OBJ_HIDE);
		break;
	case SET_OBJECTIVE_SUCCEEDED:
		missionInfo_Updated = qtrue;	// Activate flashing text
		gi.cvar_set("cg_updatedDataPadObjective", "1");
		Q3_SetObjective((const char *) data ,SET_OBJ_SUCCEEDED);
		break;
	case SET_OBJECTIVE_FAILED:
		Q3_SetObjective((const char *) data ,SET_OBJ_FAILED);
		break;

	case SET_OBJECTIVE_CLEARALL:
		Q3_ObjectiveClearAll();
		break;

	case SET_MISSIONFAILED:
		Q3_SetMissionFailed((const char *) data);
		break;

	case SET_MISSIONSTATUSTEXT:
		Q3_SetStatusText((const char *) data);
		break;

	case SET_MISSIONSTATUSTIME:
		int_data = atoi((char *) data);
		cg.missionStatusDeadTime = level.time + int_data;
		break;

	case SET_CLOSINGCREDITS:
		gi.cvar_set("cg_endcredits", "1");
		break;

	case SET_SKILL:
//		//can never be set
		break;

	case SET_FULLNAME:
		Q3_SetFullName( entID, (char *) data );
		break;

	case SET_DISABLE_SHADER_ANIM:
		if(!Q_stricmp("true", ((char *)data)))
		{
			Q3_SetDisableShaderAnims( entID, qtrue);
		}
		else
		{
			Q3_SetDisableShaderAnims( entID, qfalse);
		}
		break;

	case SET_SHADER_ANIM:
		if(!Q_stricmp("true", ((char *)data)))
		{
			Q3_SetShaderAnim( entID, qtrue);
		}
		else
		{
			Q3_SetShaderAnim( entID, qfalse);
		}
		break;

	case SET_MUSIC_STATE:
		Q3_SetMusicState( (char *) data );
		break;

	case SET_CLEAN_DAMAGING_ENTS:
		Q3_SetCleanDamagingEnts();
		break;

	case SET_HUD:
		if(!Q_stricmp("true", ((char *)data)))
		{
			gi.cvar_set("cg_drawHUD", "1");
		}
		else
		{
			gi.cvar_set("cg_drawHUD", "0");
		}
		break;

	case SET_FORCE_HEAL_LEVEL:
	case SET_FORCE_JUMP_LEVEL:
	case SET_FORCE_SPEED_LEVEL:
	case SET_FORCE_PUSH_LEVEL:
	case SET_FORCE_PULL_LEVEL:
	case SET_FORCE_MINDTRICK_LEVEL:
	case SET_FORCE_GRIP_LEVEL:
	case SET_FORCE_LIGHTNING_LEVEL:
	case SET_SABER_THROW:
	case SET_SABER_DEFENSE:
	case SET_SABER_OFFENSE:
		int_data = atoi((char *) data);
		Q3_SetForcePowerLevel( entID, (toSet-SET_FORCE_HEAL_LEVEL), int_data );
		break;

	default:
		//Q3_DebugPrint( WL_ERROR, "Q3_Set: '%s' is not a valid set field\n", type_name );
		Q3_SetVar( taskID, entID, type_name, data );
		break;
	}

	ent->taskManager->Completed( taskID );
}



/*
============
Q3_Kill
  Description	:
  Return type	: void
  Argument		:  int entID
  Argument		: const char *name
============
*/
static void Q3_Kill( int entID, const char *name )
{
	gentity_t	*ent = &g_entities[entID];
	gentity_t	*victim = NULL;
	int			o_health;

	if( !Q_stricmp( name, "self") )
	{
		victim = ent;
	}
	else if( !Q_stricmp( name, "enemy" ) )
	{
		victim = ent->enemy;
	}
	else
	{
		victim = G_Find (NULL, FOFS(targetname), (char *) name );
	}

	if ( !victim )
	{
		Q3_DebugPrint( WL_WARNING, "Q3_Kill: can't find %s\n", name);
		return;
	}

	/*
	if ( victim == ent )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_Kill: entity %s trying to kill self - not allowed!\n", name);
		return;
	}
	*/
	if ( victim == ent )
	{//don't ICARUS_FreeEnt me, I'm in the middle of a script!  (FIXME: shouldn't ICARUS handle this internally?)
		victim->svFlags |= SVF_KILLED_SELF;
	}

	o_health = victim->health;
	victim->health = 0;
	if ( victim->client )
	{
		victim->flags |= FL_NO_KNOCKBACK;
	}
	//G_SetEnemy(victim, ent);
	if( victim->e_DieFunc != dieF_NULL )	// check can be omitted
	{
		GEntity_DieFunc( victim, NULL, NULL, o_health, MOD_UNKNOWN );
	}
}


/*
============
Q3_RemoveEnt
  Description	:
  Return type	: void
  Argument		: gentity_t *victim
============
*/
static void Q3_RemoveEnt( gentity_t *victim )
{
	if( victim->client )
	{
		//ClientDisconnect(ent);
		victim->s.eFlags |= EF_NODRAW;
		victim->s.eFlags &= ~EF_NPC;
		victim->svFlags &= ~SVF_NPC;
		victim->s.eType = ET_INVISIBLE;
		victim->contents = 0;
		victim->health = 0;
		victim->targetname = NULL;

		if ( victim->NPC && victim->NPC->tempGoal != NULL )
		{
			G_FreeEntity( victim->NPC->tempGoal );
			victim->NPC->tempGoal = NULL;
		}
		if ( victim->client->ps.saberEntityNum != ENTITYNUM_NONE && victim->client->ps.saberEntityNum > 0 )
		{
			if ( g_entities[victim->client->ps.saberEntityNum].inuse )
			{
				G_FreeEntity( &g_entities[victim->client->ps.saberEntityNum] );
			}
			victim->client->ps.saberEntityNum = ENTITYNUM_NONE;
		}
		//Disappear in half a second
		victim->e_ThinkFunc = thinkF_G_FreeEntity;
		victim->nextthink = level.time + 500;
		return;
	}
	else
	{
		victim->e_ThinkFunc = thinkF_G_FreeEntity;
		victim->nextthink = level.time + 100;
	}
}


/*
============
Q3_Remove
  Description	:
  Return type	: void
  Argument		:  int entID
  Argument		: const char *name
============
*/
static void Q3_Remove( int entID, const char *name )
{
	gentity_t *ent = &g_entities[entID];
	gentity_t	*victim = NULL;

	if( !Q_stricmp( "self", name ) )
	{
		victim = ent;
		if ( !victim )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_Remove: can't find %s\n", name );
			return;
		}
		Q3_RemoveEnt( victim );
	}
	else if( !Q_stricmp( "enemy", name ) )
	{
		victim = ent->enemy;
		if ( !victim )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_Remove: can't find %s\n", name );
			return;
		}
		Q3_RemoveEnt( victim );
	}
	else
	{
		victim = G_Find( NULL, FOFS(targetname), (char *) name );
		if ( !victim )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_Remove: can't find %s\n", name );
			return;
		}

		while ( victim )
		{
			Q3_RemoveEnt( victim );
			victim = G_Find( victim, FOFS(targetname), (char *) name );
		}
	}
}

/*
============
MakeOwnerInvis
  Description	:
  Return type	: void
  Argument		: gentity_t *self
============
*/
void MakeOwnerInvis(gentity_t *self)
{
	if(self->owner && self->owner->client)
	{
		self->owner->client->ps.powerups[PW_CLOAKED] = level.time + 500;
	}

	//HACKHGACLHACK!! - MCG
	self->e_ThinkFunc = thinkF_RemoveOwner;
	self->nextthink = level.time + 400;
}


/*
============
MakeOwnerEnergy
  Description	:
  Return type	: void
  Argument		: gentity_t *self
============
*/
void MakeOwnerEnergy(gentity_t *self)
{
	if(self->owner && self->owner->client)
	{
//		self->owner->client->ps.powerups[PW_QUAD] = level.time + 1000;
	}

	G_FreeEntity(self);
}

/*
============
RemoveOwner
  Description	:
  Return type	: void
  Argument		: gentity_t *self
============
*/
void RemoveOwner (gentity_t *self)
{
	if ( self->owner && self->owner->inuse )
	{//I have an owner and they heavn't been freed yet
		Q3_Remove( self->owner->s.number, "self" );
	}

	G_FreeEntity( self );
}

/*
=================================================

  Get / Set Functions

=================================================
*/

/*
============
Q3_GetFloat
  Description	:
  Return type	: int
  Argument		:  int entID
  Argument		: int type
  Argument		: const char *name
  Argument		: float *value
============
*/
static int Q3_GetFloat( int entID, int type, const char *name, float *value )
{
	gentity_t	*ent = &g_entities[entID];
//	gclient_t	*client;

	if ( !ent )
	{
		return false;
	}

	if( strlen(name) > 5 && !Q_stricmpn(name, "cvar_", 5) )
	{
		*value = (float)gi.Cvar_VariableIntegerValue(name+5);
		return true;
	}

	int toGet = GetIDForString( setTable, name );	//FIXME: May want to make a "getTable" as well
	//FIXME: I'm getting really sick of these huge switch statements!

	//NOTENOTE: return true if the value was correctly obtained
	switch ( toGet )
	{
	case SET_PARM1:
	case SET_PARM2:
	case SET_PARM3:
	case SET_PARM4:
	case SET_PARM5:
	case SET_PARM6:
	case SET_PARM7:
	case SET_PARM8:
	case SET_PARM9:
	case SET_PARM10:
	case SET_PARM11:
	case SET_PARM12:
	case SET_PARM13:
	case SET_PARM14:
	case SET_PARM15:
	case SET_PARM16:
		if (ent->parms == NULL)
		{
			Q3_DebugPrint( WL_ERROR, "GET_PARM: %s %s did not have any parms set!\n", ent->classname, ent->targetname );
			return false;	// would prefer qfalse, but I'm fitting in with what's here <sigh>
		}
		*value = atof( ent->parms->parm[toGet - SET_PARM1] );
		break;

	case SET_COUNT:
		*value = ent->count;
		break;

	case SET_HEALTH:
		*value = ent->health;
		break;

	case SET_SKILL:
		*value = g_spskill->integer;
		break;

	case SET_XVELOCITY://## %f="0.0" # Velocity along X axis
		if ( ent->client == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_XVELOCITY, %s not a client\n", ent->targetname );
			return false;
		}
		*value = ent->client->ps.velocity[0];
		break;

	case SET_YVELOCITY://## %f="0.0" # Velocity along Y axis
		if ( ent->client == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_YVELOCITY, %s not a client\n", ent->targetname );
			return false;
		}
		*value = ent->client->ps.velocity[1];
		break;

	case SET_ZVELOCITY://## %f="0.0" # Velocity along Z axis
		if ( ent->client == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_ZVELOCITY, %s not a client\n", ent->targetname );
			return false;
		}
		*value = ent->client->ps.velocity[2];
		break;

	case SET_Z_OFFSET:
		*value = ent->currentOrigin[2] - ent->s.origin[2];
		break;

	case SET_DPITCH://## %f="0.0" # Pitch for NPC to turn to
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_DPITCH, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = ent->NPC->desiredPitch;
		break;

	case SET_DYAW://## %f="0.0" # Yaw for NPC to turn to
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_DYAW, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = ent->NPC->desiredPitch;
		break;

	case SET_WIDTH://## %f="0.0" # Width of NPC bounding box
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_WIDTH, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = ent->mins[0];
		break;
	case SET_TIMESCALE://## %f="0.0" # Speed-up slow down game (0 - 1.0)
		*value = g_timescale->value;
		break;
	case SET_CAMERA_GROUP_Z_OFS://## %s="NULL" # all ents with this cameraGroup will be focused on
		return false;
		break;

	case SET_VISRANGE://## %f="0.0" # How far away NPC can see
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_VISRANGE, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = ent->NPC->stats.visrange;
		break;

	case SET_EARSHOT://## %f="0.0" # How far an NPC can hear
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_EARSHOT, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = ent->NPC->stats.earshot;
		break;

	case SET_VIGILANCE://## %f="0.0" # How often to look for enemies (0 - 1.0)
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_VIGILANCE, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = ent->NPC->stats.vigilance;
		break;

	case SET_GRAVITY://## %f="0.0" # Change this ent's gravity - 800 default
		if ( (ent->svFlags&SVF_CUSTOM_GRAVITY) && ent->client )
		{
			*value = ent->client->ps.gravity;
		}
		else
		{
			*value = g_gravity->value;
		}
		break;

	case SET_FACEEYESCLOSED:
	case SET_FACEEYESOPENED:
	case SET_FACEAUX:		//## %f="0.0" # Set face to Aux expression for number of seconds
	case SET_FACEBLINK:		//## %f="0.0" # Set face to Blink expression for number of seconds
	case SET_FACEBLINKFROWN:	//## %f="0.0" # Set face to Blinkfrown expression for number of seconds
	case SET_FACEFROWN:		//## %f="0.0" # Set face to Frown expression for number of seconds
	case SET_FACENORMAL:		//## %f="0.0" # Set face to Normal expression for number of seconds
		Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_FACE___ not implemented\n" );
		return false;
		break;
	case SET_WAIT:		//## %f="0.0" # Change an entity's wait field
		*value = ent->wait;
		break;
	case SET_FOLLOWDIST:		//## %f="0.0" # How far away to stay from leader in BS_FOLLOW_LEADER
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_FOLLOWDIST, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = ent->NPC->followDist;
		break;
	//# #sep ints
	case SET_ANIM_HOLDTIME_LOWER://## %d="0" # Hold lower anim for number of milliseconds
		if ( ent->client == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_ANIM_HOLDTIME_LOWER, %s not a client\n", ent->targetname );
			return false;
		}
		*value = ent->client->ps.legsAnimTimer;
		break;
	case SET_ANIM_HOLDTIME_UPPER://## %d="0" # Hold upper anim for number of milliseconds
		if ( ent->client == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_ANIM_HOLDTIME_UPPER, %s not a client\n", ent->targetname );
			return false;
		}
		*value = ent->client->ps.torsoAnimTimer;
		break;
	case SET_ANIM_HOLDTIME_BOTH://## %d="0" # Hold lower and upper anims for number of milliseconds
		Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_ANIM_HOLDTIME_BOTH not implemented\n" );
		return false;
		break;
	case SET_ARMOR://## %d="0" # Change armor
		if ( ent->client == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_ARMOR, %s not a client\n", ent->targetname );
			return false;
		}
		*value = ent->client->ps.stats[STAT_ARMOR];
		break;
	case SET_WALKSPEED://## %d="0" # Change walkSpeed
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_WALKSPEED, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = ent->NPC->stats.walkSpeed;
		break;
	case SET_RUNSPEED://## %d="0" # Change runSpeed
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_RUNSPEED, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = ent->NPC->stats.runSpeed;
		break;
	case SET_YAWSPEED://## %d="0" # Change yawSpeed
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_YAWSPEED, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = ent->NPC->stats.yawSpeed;
		break;
	case SET_AGGRESSION://## %d="0" # Change aggression 1-5
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_AGGRESSION, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = ent->NPC->stats.aggression;
		break;
	case SET_AIM://## %d="0" # Change aim 1-5
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_AIM, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = ent->NPC->stats.aim;
		break;
	case SET_FRICTION://## %d="0" # Change ent's friction - 6 default
		if ( ent->client == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_FRICTION, %s not a client\n", ent->targetname );
			return false;
		}
		*value = ent->client->ps.friction;
		break;
	case SET_SHOOTDIST://## %d="0" # How far the ent can shoot - 0 uses weapon
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_SHOOTDIST, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = ent->NPC->stats.shootDistance;
		break;
	case SET_HFOV://## %d="0" # Horizontal field of view
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_HFOV, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = ent->NPC->stats.hfov;
		break;
	case SET_VFOV://## %d="0" # Vertical field of view
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_VFOV, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = ent->NPC->stats.vfov;
		break;
	case SET_DELAYSCRIPTTIME://## %d="0" # How many seconds to wait before running delayscript
		*value = ent->delayScriptTime - level.time;
		break;
	case SET_FORWARDMOVE://## %d="0" # NPC move forward -127(back) to 127
		if ( ent->client == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_FORWARDMOVE, %s not a client\n", ent->targetname );
			return false;
		}
		*value = ent->client->forced_forwardmove;
		break;
	case SET_RIGHTMOVE://## %d="0" # NPC move right -127(left) to 127
		if ( ent->client == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_RIGHTMOVE, %s not a client\n", ent->targetname );
			return false;
		}
		*value = ent->client->forced_rightmove;
		break;
	case SET_STARTFRAME:	//## %d="0" # frame to start animation sequence on
		*value = ent->startFrame;
		break;
	case SET_ENDFRAME:	//## %d="0" # frame to end animation sequence on
		*value = ent->endFrame;
		break;
	case SET_ANIMFRAME:	//## %d="0" # of current frame
		*value = ent->s.frame;
		break;

	case SET_SHOT_SPACING://## %d="1000" # Time between shots for an NPC - reset to defaults when changes weapon
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_SHOT_SPACING, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = ent->NPC->burstSpacing;
		break;
	case SET_MISSIONSTATUSTIME://## %d="0" # Amount of time until Mission Status should be shown after death
		*value = cg.missionStatusDeadTime - level.time;
		break;
	//# #sep booleans
	case SET_IGNOREPAIN://## %t="BOOL_TYPES" # Do not react to pain
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_IGNOREPAIN, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = ent->NPC->ignorePain;
		break;
	case SET_IGNOREENEMIES://## %t="BOOL_TYPES" # Do not acquire enemies
		*value = (ent->svFlags&SVF_IGNORE_ENEMIES);
		break;
	case SET_IGNOREALERTS://## Do not get enemy set by allies in area(ambush)
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_IGNOREALERTS, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = (ent->NPC->scriptFlags&SCF_IGNORE_ALERTS);
		break;

	case SET_DONTSHOOT://## %t="BOOL_TYPES" # Others won't shoot you
		*value = (ent->flags&FL_DONT_SHOOT);
		break;
	case SET_NOTARGET://## %t="BOOL_TYPES" # Others won't pick you as enemy
		*value = (ent->flags&FL_NOTARGET);
		break;
	case SET_DONTFIRE://## %t="BOOL_TYPES" # Don't fire your weapon
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_DONTFIRE, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = (ent->NPC->scriptFlags&SCF_DONT_FIRE);
		break;

	case SET_LOCKED_ENEMY://## %t="BOOL_TYPES" # Keep current enemy until dead
		*value = (ent->svFlags&SVF_LOCKEDENEMY);
		break;
	case SET_CROUCHED://## %t="BOOL_TYPES" # Force NPC to crouch
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_CROUCHED, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = (ent->NPC->scriptFlags&SCF_CROUCHED);
		break;
	case SET_WALKING://## %t="BOOL_TYPES" # Force NPC to move at walkSpeed
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_WALKING, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = (ent->NPC->scriptFlags&SCF_WALKING);
		break;
	case SET_RUNNING://## %t="BOOL_TYPES" # Force NPC to move at runSpeed
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_RUNNING, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = (ent->NPC->scriptFlags&SCF_RUNNING);
		break;
	case SET_CHASE_ENEMIES://## %t="BOOL_TYPES" # NPC will chase after enemies
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_CHASE_ENEMIES, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = (ent->NPC->scriptFlags&SCF_CHASE_ENEMIES);
		break;
	case SET_LOOK_FOR_ENEMIES://## %t="BOOL_TYPES" # NPC will be on the lookout for enemies
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_LOOK_FOR_ENEMIES, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = (ent->NPC->scriptFlags&SCF_LOOK_FOR_ENEMIES);
		break;
	case SET_FACE_MOVE_DIR://## %t="BOOL_TYPES" # NPC will face in the direction it's moving
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_FACE_MOVE_DIR, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = (ent->NPC->scriptFlags&SCF_FACE_MOVE_DIR);
		break;
	case SET_FORCED_MARCH://## %t="BOOL_TYPES" # Force NPC to move at runSpeed
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_FORCED_MARCH, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = (ent->NPC->scriptFlags&SET_FORCED_MARCH);
		break;
	case SET_UNDYING://## %t="BOOL_TYPES" # Can take damage down to 1 but not die
		*value = (ent->flags&FL_UNDYING);
		break;
	case SET_NOAVOID://## %t="BOOL_TYPES" # Will not avoid other NPCs or architecture
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_NOAVOID, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = (ent->NPC->aiFlags&NPCAI_NO_COLL_AVOID);
		break;

	case SET_SOLID://## %t="BOOL_TYPES" # Make yourself notsolid or solid
		*value = ent->contents;
		break;
	case SET_PLAYER_USABLE://## %t="BOOL_TYPES" # Can be activateby the player's "use" button
		*value = (ent->svFlags&SVF_PLAYER_USABLE);
		break;
	case SET_LOOP_ANIM://## %t="BOOL_TYPES" # For non-NPCs: loop your animation sequence
		*value = ent->loopAnim;
		break;
	case SET_INTERFACE://## %t="BOOL_TYPES" # Player interface on/off
		Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_INTERFACE not implemented\n" );
		return false;
		break;
	case SET_SHIELDS://## %t="BOOL_TYPES" # NPC has no shields (Borg do not adapt)
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_SHIELDS, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = (ent->NPC->aiFlags&NPCAI_SHIELDS);
		break;
	case SET_INVISIBLE://## %t="BOOL_TYPES" # Makes an NPC not solid and not visible
		*value = (ent->s.eFlags&EF_NODRAW);
		break;
	case SET_VAMPIRE://## %t="BOOL_TYPES" # Makes an NPC not solid and not visible
		if ( !ent->client )
		{
			return false;
		}
		else
		{
			*value = (ent->client->ps.powerups[PW_DISINT_2]>level.time);
		}
		break;
	case SET_FORCE_INVINCIBLE://## %t="BOOL_TYPES" # Makes an NPC not solid and not visible
		if ( !ent->client )
		{
			return false;
		}
		else
		{
			*value = (ent->client->ps.powerups[PW_INVINCIBLE]>level.time);
		}
		break;
	case SET_GREET_ALLIES://## %t="BOOL_TYPES" # Makes an NPC greet teammates
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_GREET_ALLIES, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = (ent->NPC->aiFlags&NPCAI_GREET_ALLIES);
		break;
	case SET_VIDEO_FADE_IN://## %t="BOOL_TYPES" # Makes video playback fade in
		Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_VIDEO_FADE_IN not implemented\n" );
		return false;
		break;
	case SET_VIDEO_FADE_OUT://## %t="BOOL_TYPES" # Makes video playback fade out
		Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_VIDEO_FADE_OUT not implemented\n" );
		return false;
		break;
	case SET_PLAYER_LOCKED://## %t="BOOL_TYPES" # Makes it so player cannot move
		*value = player_locked;
		break;
	case SET_LOCK_PLAYER_WEAPONS://## %t="BOOL_TYPES" # Makes it so player cannot switch weapons
		*value = (ent->flags&FL_LOCK_PLAYER_WEAPONS);
		break;
	case SET_NO_IMPACT_DAMAGE://## %t="BOOL_TYPES" # Makes it so player cannot switch weapons
		*value = (ent->flags&FL_NO_IMPACT_DMG);
		break;
	case SET_NO_KNOCKBACK://## %t="BOOL_TYPES" # Stops this ent from taking knockback from weapons
		*value = (ent->flags&FL_NO_KNOCKBACK);
		break;
	case SET_ALT_FIRE://## %t="BOOL_TYPES" # Force NPC to use altfire when shooting
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_ALT_FIRE, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = (ent->NPC->scriptFlags&SCF_ALT_FIRE);
		break;
	case SET_NO_RESPONSE://## %t="BOOL_TYPES" # NPCs will do generic responses when this is on (usescripts override generic responses as well)
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_NO_RESPONSE, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = (ent->NPC->scriptFlags&SCF_NO_RESPONSE);
		break;
	case SET_INVINCIBLE://## %t="BOOL_TYPES" # Completely unkillable
		*value = (ent->flags&FL_GODMODE);
		break;
	case SET_MISSIONSTATUSACTIVE:	//# Turns on Mission Status Screen
		*value = cg.missionStatusShow;
		break;
	case SET_NO_COMBAT_TALK://## %t="BOOL_TYPES" # NPCs will not do their combat talking noises when this is on
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_NO_COMBAT_TALK, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = (ent->NPC->scriptFlags&SCF_NO_COMBAT_TALK);
		break;
	case SET_NO_ALERT_TALK://## %t="BOOL_TYPES" # NPCs will not do their combat talking noises when this is on
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_NO_ALERT_TALK, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = (ent->NPC->scriptFlags&SCF_NO_ALERT_TALK);
		break;
	case SET_USE_CP_NEAREST://## %t="BOOL_TYPES" # NPCs will use their closest combat points, not try and find ones next to the player, or flank player
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_USE_CP_NEAREST, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = (ent->NPC->scriptFlags&SCF_USE_CP_NEAREST);
		break;
	case SET_DISMEMBERABLE://## %t="BOOL_TYPES" # NPC will not be affected by force powers
		if ( ent->client == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_DISMEMBERABLE, %s not a client\n", ent->targetname );
			return false;
		}
		*value = !(ent->client->dismembered);
		break;
	case SET_NO_FORCE:
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_NO_FORCE, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = (ent->NPC->scriptFlags&SCF_NO_FORCE);
		break;
	case SET_NO_ACROBATICS:
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_NO_ACROBATICS, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = (ent->NPC->scriptFlags&SCF_NO_ACROBATICS);
		break;
	case SET_USE_SUBTITLES:
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_USE_SUBTITLES, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = (ent->NPC->scriptFlags&SCF_USE_SUBTITLES);
		break;
	case SET_NO_FALLTODEATH://## %t="BOOL_TYPES" # NPC will not be affected by force powers
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_NO_FALLTODEATH, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = (ent->NPC->scriptFlags&SCF_NO_FALLTODEATH);
		break;
	case SET_MORELIGHT://## %t="BOOL_TYPES" # NPCs will use their closest combat points, not try and find ones next to the player, or flank player
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_MORELIGHT, %s not an NPC\n", ent->targetname );
			return false;
		}
		*value = (ent->NPC->scriptFlags&SCF_MORELIGHT);
		break;
	case SET_TREASONED://## %t="BOOL_TYPES" # Player has turned on his own- scripts will stop: NPCs will turn on him and level changes load the brig
		Q3_DebugPrint( WL_VERBOSE, "SET_TREASONED is disabled, do not use\n" );
		*value = 0;//(ffireLevel>=FFIRE_LEVEL_RETALIATION);
		break;
	case SET_DISABLE_SHADER_ANIM:	//## %t="BOOL_TYPES" # Shaders won't animate
		*value = (ent->s.eFlags & EF_DISABLE_SHADER_ANIM);
		break;
	case SET_SHADER_ANIM:	//## %t="BOOL_TYPES" # Shader will be under frame control
		*value = (ent->s.eFlags & EF_SHADER_ANIM);
		break;

	default:
		if ( Q3_VariableDeclared( name ) != VTYPE_FLOAT )
			return false;

		return Q3_GetFloatVariable( name, value );
	}

	return true;
}


/*
============
Q3_GetVector
  Description	:
  Return type	: int
  Argument		:  int entID
  Argument		: int type
  Argument		: const char *name
  Argument		: vec3_t value
============
*/
static int Q3_GetVector( int entID, int type, const char *name, vec3_t value )
{
	gentity_t	*ent = &g_entities[entID];
	if ( !ent )
	{
		return false;
	}

	int toGet = GetIDForString( setTable, name );	//FIXME: May want to make a "getTable" as well
	//FIXME: I'm getting really sick of these huge switch statements!

	//NOTENOTE: return true if the value was correctly obtained
	switch ( toGet )
	{
	case SET_PARM1:
	case SET_PARM2:
	case SET_PARM3:
	case SET_PARM4:
	case SET_PARM5:
	case SET_PARM6:
	case SET_PARM7:
	case SET_PARM8:
	case SET_PARM9:
	case SET_PARM10:
	case SET_PARM11:
	case SET_PARM12:
	case SET_PARM13:
	case SET_PARM14:
	case SET_PARM15:
	case SET_PARM16:
		sscanf( ent->parms->parm[toGet - SET_PARM1], "%f %f %f", &value[0], &value[1], &value[2] );
		break;

	case SET_ORIGIN:
		VectorCopy(ent->currentOrigin, value);
		break;

	case SET_ANGLES:
		VectorCopy(ent->currentAngles, value);
		break;

	case SET_TELEPORT_DEST://## %v="0.0 0.0 0.0" # Set origin here as soon as the area is clear
		Q3_DebugPrint( WL_WARNING, "Q3_GetVector: SET_TELEPORT_DEST not implemented\n" );
		return false;
		break;

	default:

		if ( Q3_VariableDeclared( name ) != VTYPE_VECTOR )
			return false;

		return Q3_GetVectorVariable( name, value );
	}

	return true;
}

/*
============
Q3_GetString
  Description	:
  Return type	: int
  Argument		:  int entID
  Argument		: int type
  Argument		: const char *name
  Argument		: char **value
============
*/
static int Q3_GetString( int entID, int type, const char *name, char **value )
{
	gentity_t	*ent = &g_entities[entID];
	if ( !ent )
	{
		return false;
	}

	if( strlen(name) > 5 && !Q_stricmpn(name, "cvar_", 5) )
	{
		const char* cvar_name = name + 5;
		// by allocating and then re-using the same sufficiently large buffer,
		// we ensure that pointers to it never become invalid,
		// so we can support expressions using the same cvar twice,
		// e.g. if(get(cvar_x) == get(cvar_x))
		std::array<char, MAX_STRING_CHARS>& buf = ICARUS_CvarList[cvar_name];
		gi.Cvar_VariableStringBuffer(cvar_name, buf.data(), buf.size());
		*value = buf.data();
		return true;
	}

	int toGet = GetIDForString( setTable, name );	//FIXME: May want to make a "getTable" as well

	switch ( toGet )
	{
	case SET_ANIM_BOTH:
		*value = (char *) Q3_GetAnimBoth( ent );

		if ( VALIDSTRING( *value ) == false )
			return false;

		break;

	case SET_PARM1:
	case SET_PARM2:
	case SET_PARM3:
	case SET_PARM4:
	case SET_PARM5:
	case SET_PARM6:
	case SET_PARM7:
	case SET_PARM8:
	case SET_PARM9:
	case SET_PARM10:
	case SET_PARM11:
	case SET_PARM12:
	case SET_PARM13:
	case SET_PARM14:
	case SET_PARM15:
	case SET_PARM16:
		if ( ent->parms )
		{
			*value = (char *) ent->parms->parm[toGet - SET_PARM1];
		}
		else
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetString: invalid ent %s has no parms!\n", ent->targetname );
			return false;
		}
		break;

	case SET_TARGET:
		*value = (char *) ent->target;
		break;

	case SET_LOCATION:
		*value = G_GetLocationForEnt( ent );
		if ( !value || !value[0] )
		{
			return false;
		}
		break;

	//# #sep Scripts and other file paths
	case SET_SPAWNSCRIPT://## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when spawned //0 - do not change these, these are equal to BSET_SPAWN, etc
		*value = ent->behaviorSet[BSET_SPAWN];
		break;
	case SET_USESCRIPT://## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when used
		*value = ent->behaviorSet[BSET_USE];
		break;
	case SET_AWAKESCRIPT://## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when startled
		*value = ent->behaviorSet[BSET_AWAKE];
		break;
	case SET_ANGERSCRIPT://## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script run when find an enemy for the first time
		*value = ent->behaviorSet[BSET_ANGER];
		break;
	case SET_ATTACKSCRIPT://## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when you shoot
		*value = ent->behaviorSet[BSET_ATTACK];
		break;
	case SET_VICTORYSCRIPT://## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when killed someone
		*value = ent->behaviorSet[BSET_VICTORY];
		break;
	case SET_LOSTENEMYSCRIPT://## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when you can't find your enemy
		*value = ent->behaviorSet[BSET_LOSTENEMY];
		break;
	case SET_PAINSCRIPT://## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when hit
		*value = ent->behaviorSet[BSET_PAIN];
		break;
	case SET_FLEESCRIPT://## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when hit and low health
		*value = ent->behaviorSet[BSET_FLEE];
		break;
	case SET_DEATHSCRIPT://## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when killed
		*value = ent->behaviorSet[BSET_DEATH];
		break;
	case SET_DELAYEDSCRIPT://## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run after a delay
		*value = ent->behaviorSet[BSET_DELAYED];
		break;
	case SET_BLOCKEDSCRIPT://## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when blocked by teammate
		*value = ent->behaviorSet[BSET_BLOCKED];
		break;
	case SET_FFIRESCRIPT://## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when player has shot own team repeatedly
		*value = ent->behaviorSet[BSET_FFIRE];
		break;
	case SET_FFDEATHSCRIPT://## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when player kills a teammate
		*value = ent->behaviorSet[BSET_FFDEATH];
		break;

	//# #sep Standard strings
	case SET_ENEMY://## %s="NULL" # Set enemy by targetname
		if ( ent->enemy != NULL )
		{
			*value = ent->enemy->targetname;
		}
		else return false;
		break;
	case SET_LEADER://## %s="NULL" # Set for BS_FOLLOW_LEADER
		if ( ent->client == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetString: SET_LEADER, %s not a client\n", ent->targetname );
			return false;
		}
		else if ( ent->client->leader )
		{
			*value = ent->client->leader->targetname;
		}
		else return false;
		break;
	case SET_CAPTURE://## %s="NULL" # Set captureGoal by targetname
		if ( ent->NPC == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetString: SET_CAPTURE, %s not an NPC\n", ent->targetname );
			return false;
		}
		else if ( ent->NPC->captureGoal != NULL )
		{
			*value = ent->NPC->captureGoal->targetname;
		}
		else return false;
		break;

	case SET_TARGETNAME://## %s="NULL" # Set/change your targetname
		*value = ent->targetname;
		break;
	case SET_PAINTARGET://## %s="NULL" # Set/change what to use when hit
		*value = ent->paintarget;
		break;
	case SET_CAMERA_GROUP://## %s="NULL" # all ents with this cameraGroup will be focused on
		*value = ent->cameraGroup;
		break;
	case SET_CAMERA_GROUP_TAG://## %s="NULL" # all ents with this cameraGroup will be focused on
		return false;
		break;
	case SET_LOOK_TARGET://## %s="NULL" # object for NPC to look at
		if ( ent->client == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetString: SET_LOOK_TARGET, %s not a client\n", ent->targetname );
			return false;
		}
		else
		{
			gentity_t *lookTarg = &g_entities[ent->client->renderInfo.lookTarget];
			if ( lookTarg != NULL )
			{
				*value = lookTarg->targetname;
			}
			else return false;
		}
		break;
	case SET_TARGET2://## %s="NULL" # Set/change your target2: on NPC's: this fires when they're knocked out by the red hypo
		*value = ent->target2;
		break;

	case SET_REMOVE_TARGET://## %s="NULL" # Target that is fired when someone completes the BS_REMOVE behaviorState
		*value = ent->target3;
		break;
	case SET_WEAPON:
		if ( ent->client == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetString: SET_WEAPON, %s not a client\n", ent->targetname );
			return false;
		}
		else
		{
			*value = (char *)GetStringForID( WPTable, ent->client->ps.weapon );
		}
		break;

	case SET_ITEM:
		if ( ent->client == NULL )
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetString: SET_ITEM, %s not a client\n", ent->targetname );
			return false;
		}
		else
		{
		//	*value = (char *)GetStringForID( WPTable, ent->client->ps.weapon );
		}
		break;
	case SET_MUSIC_STATE:
		*value = (char *)GetStringForID( DMSTable, level.dmState );
		break;
	//The below cannot be gotten
	case SET_NAVGOAL://## %s="NULL" # *Move to this navgoal then continue script
		Q3_DebugPrint( WL_WARNING, "Q3_GetString: SET_NAVGOAL not implemented\n" );
		return false;
		break;
	case SET_VIEWTARGET://## %s="NULL" # Set angles toward ent by targetname
		Q3_DebugPrint( WL_WARNING, "Q3_GetString: SET_VIEWTARGET not implemented\n" );
		return false;
		break;
	case SET_WATCHTARGET://## %s="NULL" # Set angles toward ent by targetname
		if ( ent && ent->NPC && ent->NPC->watchTarget )
		{
			*value = ent->NPC->watchTarget->targetname;
		}
		else
		{
			Q3_DebugPrint( WL_WARNING, "Q3_GetString: SET_WATCHTARGET no watchTarget!\n" );
			return false;
		}
		break;
	case SET_VIEWENTITY:
		Q3_DebugPrint( WL_WARNING, "Q3_GetString: SET_VIEWENTITY not implemented\n" );
		return false;
		break;
	case SET_CAPTIONTEXTCOLOR:	//## %s=""  # Color of text RED:WHITE:BLUE: YELLOW
		Q3_DebugPrint( WL_WARNING, "Q3_GetString: SET_CAPTIONTEXTCOLOR not implemented\n" );
		return false;
		break;
	case SET_CENTERTEXTCOLOR:	//## %s=""  # Color of text RED:WHITE:BLUE: YELLOW
		Q3_DebugPrint( WL_WARNING, "Q3_GetString: SET_CENTERTEXTCOLOR not implemented\n" );
		return false;
		break;
	case SET_SCROLLTEXTCOLOR:	//## %s=""  # Color of text RED:WHITE:BLUE: YELLOW
		Q3_DebugPrint( WL_WARNING, "Q3_GetString: SET_SCROLLTEXTCOLOR not implemented\n" );
		return false;
		break;
	case SET_COPY_ORIGIN://## %s="targetname"  # Copy the origin of the ent with targetname to your origin
		Q3_DebugPrint( WL_WARNING, "Q3_GetString: SET_COPY_ORIGIN not implemented\n" );
		return false;
		break;
	case SET_DEFEND_TARGET://## %s="targetname"  # This NPC will attack the target NPC's enemies
		Q3_DebugPrint( WL_WARNING, "Q3_GetString: SET_COPY_ORIGIN not implemented\n" );
		return false;
		break;
	case SET_VIDEO_PLAY://## %s="filename" !!"W:\game\base\video\!!#*.roq" # Play a Video (inGame)
		Q3_DebugPrint( WL_WARNING, "Q3_GetString: SET_VIDEO_PLAY not implemented\n" );
		return false;
		break;
	case SET_LOADGAME://## %s="exitholodeck" # Load the savegame that was auto-saved when you started the holodeck
		Q3_DebugPrint( WL_WARNING, "Q3_GetString: SET_LOADGAME not implemented\n" );
		return false;
		break;
	case SET_LOCKYAW://## %s="off"  # Lock legs to a certain yaw angle (or "off" or "auto" uses current)
		Q3_DebugPrint( WL_WARNING, "Q3_GetString: SET_LOCKYAW not implemented\n" );
		return false;
		break;
	case SET_SCROLLTEXT:	//## %s="" # key of text string to print
		Q3_DebugPrint( WL_WARNING, "Q3_GetString: SET_SCROLLTEXT not implemented\n" );
		return false;
		break;
	case SET_LCARSTEXT:	//## %s="" # key of text string to print in LCARS frame
		Q3_DebugPrint( WL_WARNING, "Q3_GetString: SET_LCARSTEXT not implemented\n" );
		return false;
		break;

	case SET_FULLNAME://## %s="NULL" # Set/change your targetname
		*value = ent->fullName;
		break;
	default:

		if ( Q3_VariableDeclared( name ) != VTYPE_STRING )
			return false;

		return Q3_GetStringVariable( name, (const char **) value );
	}

	return true;
}

/*
============
Q3_Evaluate
  Description	:
  Return type	: int
  Argument		:  int p1Type
  Argument		: const char *p1
  Argument		: int p2Type
  Argument		: const char *p2
  Argument		: int operatorType
============
*/
static int Q3_Evaluate( int p1Type, const char *p1, int p2Type, const char *p2, int operatorType )
{
	float	f1=0, f2=0;
	vec3_t	v1, v2;
	char	*c1=0, *c2=0;
	int		i1=0, i2=0;

	//Always demote to int on float to integer comparisons
	if ( ( ( p1Type == TK_FLOAT ) && ( p2Type == TK_INT ) ) || ( ( p1Type == TK_INT ) && ( p2Type == TK_FLOAT ) ) )
	{
		p1Type = TK_INT;
		p2Type = TK_INT;
	}

	//Cannot compare two disimilar types
	if ( p1Type != p2Type )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_Evaluate comparing two disimilar types!\n");
		return false;
	}

	//Format the parameters
	switch ( p1Type )
	{
	case TK_FLOAT:
		sscanf( p1, "%f", &f1 );
		sscanf( p2, "%f", &f2 );
		break;

	case TK_INT:
		sscanf( p1, "%d", &i1 );
		sscanf( p2, "%d", &i2 );
		break;

	case TK_VECTOR:
		sscanf( p1, "%f %f %f", &v1[0], &v1[1], &v1[2] );
		sscanf( p2, "%f %f %f", &v2[0], &v2[1], &v2[2] );
		break;

	case TK_STRING:
	case TK_IDENTIFIER:
		c1 = (char *) p1;
		c2 = (char *) p2;
		break;

	default:
		Q3_DebugPrint( WL_WARNING, "Q3_Evaluate unknown type used!\n");
		return false;
	}

	//Compare them and return the result

	//FIXME: YUCK!!!  Better way to do this?

	switch ( operatorType )
	{

	//
	//	EQUAL TO
	//

	case TK_EQUALS:

		switch ( p1Type )
		{
		case TK_FLOAT:
			return (int) ( f1 == f2 );
			break;

		case TK_INT:
			return (int) ( i1 == i2 );
			break;

		case TK_VECTOR:
			return (int) VectorCompare( v1, v2 );
			break;

		case TK_STRING:
		case TK_IDENTIFIER:
			return (int) !Q_stricmp( c1, c2 );	//NOTENOTE: The script uses proper string comparison logic (ex. ( a == a ) == true )
			break;

		default:
			Q3_DebugPrint( WL_ERROR, "Q3_Evaluate unknown type used!\n");
			return false;
		}

		break;

	//
	//	GREATER THAN
	//

	case TK_GREATER_THAN:

		switch ( p1Type )
		{
		case TK_FLOAT:
			return (int) ( f1 > f2 );
			break;

		case TK_INT:
			return (int) ( i1 > i2 );
			break;

		case TK_VECTOR:
			Q3_DebugPrint( WL_ERROR, "Q3_Evaluate vector comparisons of type GREATER THAN cannot be performed!");
			return false;
			break;

		case TK_STRING:
		case TK_IDENTIFIER:
			Q3_DebugPrint( WL_ERROR, "Q3_Evaluate string comparisons of type GREATER THAN cannot be performed!");
			return false;
			break;

		default:
			Q3_DebugPrint( WL_ERROR, "Q3_Evaluate unknown type used!\n");
			return false;
		}

		break;

	//
	//	LESS THAN
	//

	case TK_LESS_THAN:

		switch ( p1Type )
		{
		case TK_FLOAT:
			return (int) ( f1 < f2 );
			break;

		case TK_INT:
			return (int) ( i1 < i2 );
			break;

		case TK_VECTOR:
			Q3_DebugPrint( WL_ERROR, "Q3_Evaluate vector comparisons of type LESS THAN cannot be performed!");
			return false;
			break;

		case TK_STRING:
		case TK_IDENTIFIER:
			Q3_DebugPrint( WL_ERROR, "Q3_Evaluate string comparisons of type LESS THAN cannot be performed!");
			return false;
			break;

		default:
			Q3_DebugPrint( WL_ERROR, "Q3_Evaluate unknown type used!\n");
			return false;
		}

		break;

	//
	//	NOT
	//

	case TK_NOT:	//NOTENOTE: Implied "NOT EQUAL TO"

		switch ( p1Type )
		{
		case TK_FLOAT:
			return (int) ( f1 != f2 );
			break;

		case TK_INT:
			return (int) ( i1 != i2 );
			break;

		case TK_VECTOR:
			return (int) !VectorCompare( v1, v2 );
			break;

		case TK_STRING:
		case TK_IDENTIFIER:
			return (int) Q_stricmp( c1, c2 );
			break;

		default:
			Q3_DebugPrint( WL_ERROR, "Q3_Evaluate unknown type used!\n");
			return false;
		}

		break;

	default:
		Q3_DebugPrint( WL_ERROR, "Q3_Evaluate unknown operator used!\n");
		break;
	}

	return false;
}

/*
-------------------------
Q3_CameraFade
-------------------------
*/
static void Q3_CameraFade( float sr, float sg, float sb, float sa, float dr, float dg, float db, float da, float duration )
{
	vec4_t	src, dst;

	src[0] = sr;
	src[1] = sg;
	src[2] = sb;
	src[3] = sa;

	dst[0] = dr;
	dst[1] = dg;
	dst[2] = db;
	dst[3] = da;

	CGCam_Fade( src, dst, duration );
}

/*
-------------------------
Q3_CameraPath
-------------------------
*/
static void Q3_CameraPath( const char *name )
{
	CGCam_StartRoff( G_NewString( name ) );
}

/*
-------------------------
Q3_DebugPrint
-------------------------
*/
void Q3_DebugPrint( int level, const char *format, ... )
{
	//Don't print messages they don't want to see
	if ( g_ICARUSDebug->integer < level )
		return;

	va_list		argptr;
	char		text[1024];

	va_start (argptr, format);
	Q_vsnprintf (text, sizeof(text), format, argptr);
	va_end (argptr);

	//Add the color formatting
	switch ( level )
	{
		case WL_ERROR:
			Com_Printf ( S_COLOR_RED"ERROR: %s", text );
			break;

		case WL_WARNING:
			Com_Printf ( S_COLOR_YELLOW"WARNING: %s", text );
			break;

		case WL_DEBUG:
			{
				int		entNum;
				char	*buffer;

				sscanf( text, "%d", &entNum );

				if ( ( ICARUS_entFilter >= 0 ) && ( ICARUS_entFilter != entNum ) )
					return;

				buffer = (char *) text;
				buffer += 5;

				if ( ( entNum < 0 ) || ( entNum >= MAX_GENTITIES ) )
					entNum = 0;

				Com_Printf ( S_COLOR_BLUE"DEBUG: %s(%d): %s\n", g_entities[entNum].script_targetname, entNum, buffer );
				break;
			}
		default:
		case WL_VERBOSE:
			Com_Printf ( S_COLOR_GREEN"INFO: %s", text );
			break;
	}
}

/*
-------------------------
Q3_Play
-------------------------
*/
static void Q3_Play( int taskID, int entID, const char *type, const char *name )
{
	gentity_t *ent = &g_entities[entID];

	if ( !Q_stricmp( type, "PLAY_ROFF" ) )
	{
		// Try to load the requested ROFF
		if ( G_LoadRoff( name ) )
		{
			ent->roff = G_NewString( name );

			// Start the roff from the beginning
			ent->roff_ctr = 0;

			//Save this off for later
			Q3_TaskIDSet( ent, TID_MOVE_NAV, taskID );

			// Let the ROFF playing start.
			ent->next_roff_time = level.time;

			// These need to be initialised up front...
			VectorCopy( ent->currentOrigin, ent->pos1 );
			VectorCopy( ent->currentAngles, ent->pos2 );

			gi.linkentity( ent );
		}
	}
}

/*
============
Interface_Init
  Description	: Inits the interface for the game
  Return type	: void
  Argument		: interface_export_t *pe
============
*/
void Interface_Init( interface_export_t *pe )
{
	//TODO: This is where you link up all your functions to the engine

	//General
	pe->I_LoadFile				=	Q3_ReadScript;
	pe->I_CenterPrint			=	Q3_CenterPrint;
	pe->I_DPrintf				=	Q3_DebugPrint;
	pe->I_GetEntityByName		=	Q3_GetEntityByName;
	pe->I_GetTime				=	Q3_GetTime;
	pe->I_GetTimeScale			=	Q3_GetTimeScale;
	pe->I_PlaySound				=	Q3_PlaySound;
	pe->I_Lerp2Pos				=	Q3_Lerp2Pos;
	pe->I_Lerp2Origin			=	Q3_Lerp2Origin;
	pe->I_Lerp2Angles			=	Q3_Lerp2Angles;
	pe->I_GetTag				=	Q3_GetTag;
	pe->I_Lerp2Start			=	Q3_Lerp2Start;
	pe->I_Lerp2End				=	Q3_Lerp2End;
	pe->I_Use					=	Q3_Use;
	pe->I_Kill					=	Q3_Kill;
	pe->I_Remove				=	Q3_Remove;
	pe->I_Set					=	Q3_Set;
	pe->I_Random				=	Q_flrand;
	pe->I_Play					=	Q3_Play;

	//Camera functions
	pe->I_CameraEnable			=	CGCam_Enable;
	pe->I_CameraDisable			=	CGCam_Disable;
	pe->I_CameraZoom			=	CGCam_Zoom;
	pe->I_CameraMove			=	CGCam_Move;
	pe->I_CameraPan				=	CGCam_Pan;
	pe->I_CameraRoll			=	CGCam_Roll;
	pe->I_CameraTrack			=	CGCam_Track;
	pe->I_CameraFollow			=	CGCam_Follow;
	pe->I_CameraDistance		=	CGCam_Distance;
	pe->I_CameraShake			=	CGCam_Shake;
	pe->I_CameraFade			=	Q3_CameraFade;
	pe->I_CameraPath			=	Q3_CameraPath;

	//Variable information
	pe->I_GetFloat				=	Q3_GetFloat;
	pe->I_GetVector				=	Q3_GetVector;
	pe->I_GetString				=	Q3_GetString;

	pe->I_Evaluate				=	Q3_Evaluate;

	pe->I_DeclareVariable		=	Q3_DeclareVariable;
	pe->I_FreeVariable			=	Q3_FreeVariable;

	//Save / Load functions
	pe->I_LinkEntity			=	ICARUS_LinkEntity;
	pe->saved_game = gi.saved_game;

	gclient_t	*client;
	client = &level.clients[0];
	memset(&client->sess,0,sizeof(client->sess));
}

// leave these two as standard mallocs for the moment, there's something weird happening in ICARUS...
//
void *ICARUS_Malloc(int iSize)
{
	//return gi.Malloc(iSize, TAG_ICARUS);
	return malloc(iSize);
}

void ICARUS_Free(void *pMem)
{
	//gi.Free(pMem);
	free(pMem);
}
