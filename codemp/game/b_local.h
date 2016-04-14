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

#pragma once

//B_local.h
//re-added by MCG

#include "g_local.h"
#include "b_public.h"
#include "say.h"

#include "ai.h"

#define	AI_TIMERS 0//turn on to see print-outs of AI/nav timing
//
// Navigation susbsystem
//

#define NAVF_DUCK			0x00000001
#define NAVF_JUMP			0x00000002
#define NAVF_HOLD			0x00000004
#define NAVF_SLOW			0x00000008

#define DEBUG_LEVEL_DETAIL	4
#define DEBUG_LEVEL_INFO	3
#define DEBUG_LEVEL_WARNING	2
#define DEBUG_LEVEL_ERROR	1
#define DEBUG_LEVEL_NONE	0

#define MAX_GOAL_REACHED_DIST_SQUARED	256//16 squared
#define MIN_ANGLE_ERROR 0.01f

#define MIN_ROCKET_DIST_SQUARED 16384//128*128
//
// NPC.cpp
//
// ai debug cvars
void SetNPCGlobals( gentity_t *ent );
void SaveNPCGlobals(void);
void RestoreNPCGlobals(void);
extern void NPC_Think ( gentity_t *self);

//NPC_reactions.cpp
extern void NPC_Pain(gentity_t *self, gentity_t *attacker, int damage);
extern void NPC_Touch( gentity_t *self, gentity_t *other, trace_t *trace );
extern void NPC_Use( gentity_t *self, gentity_t *other, gentity_t *activator );
extern float NPC_GetPainChance( gentity_t *self, int damage );

//
// NPC_misc.cpp
//
extern void Debug_Printf( vmCvar_t *cv, int level, char *fmt, ... );
extern void Debug_NPCPrintf( gentity_t *printNPC, vmCvar_t *cv, int debugLevel, char *fmt, ... );

//MCG - Begin============================================================
//NPC_ai variables - shared by NPC.cpp and the following modules
//OJKFIXME: Should probably construct these at the NPC entry points and pass as arguments to any function that needs them
typedef struct npcStatic_s {
	gentity_t		*NPC;
	gNPC_t			*NPCInfo;
	gclient_t		*client;
	usercmd_t		 ucmd;
	visibility_t	 enemyVisibility;
} npcStatic_t;
extern npcStatic_t NPCS;

//AI_Default
extern qboolean NPC_CheckInvestigate( int alertEventNum );
extern qboolean NPC_StandTrackAndShoot (gentity_t *NPC, qboolean canDuck);
extern void NPC_BSIdle( void );
extern void NPC_BSPointShoot(qboolean shoot);
extern void NPC_BSStandGuard (void);
extern void NPC_BSPatrol (void);
extern void NPC_BSHuntAndKill (void);
extern void NPC_BSStandAndShoot (void);
extern void NPC_BSRunAndShoot (void);
extern void NPC_BSWait( void );
extern void NPC_BSDefault( void );

//NPC_behavior
extern void NPC_BSAdvanceFight (void);
extern void NPC_BSInvestigate (void);
extern void NPC_BSSleep( void );
extern void NPC_BSFlee (void);
extern void NPC_BSFollowLeader (void);
extern void NPC_BSJump (void);
extern void NPC_BSRemove (void);
extern void NPC_BSSearch (void);
extern void NPC_BSSearchStart (int	homeWp, bState_t bState);
extern void NPC_BSWander (void);
extern void NPC_BSFlee( void );
extern void NPC_StartFlee( gentity_t *enemy, vec3_t dangerPoint, int dangerLevel, int fleeTimeMin, int fleeTimeMax );
extern void G_StartFlee( gentity_t *self, gentity_t *enemy, vec3_t dangerPoint, int dangerLevel, int fleeTimeMin, int fleeTimeMax );

//NPC_combat
extern int ChooseBestWeapon( void );
extern void NPC_ChangeWeapon( int newWeapon );
extern void ShootThink( void );
extern void WeaponThink( qboolean inCombat );
extern qboolean HaveWeapon( int weapon );
extern qboolean CanShoot ( gentity_t *ent, gentity_t *shooter );
extern void NPC_CheckPossibleEnemy( gentity_t *other, visibility_t vis );
extern gentity_t *NPC_PickEnemy (gentity_t *closestTo, int enemyTeam, qboolean checkVis, qboolean findPlayersFirst, qboolean findClosest);
extern gentity_t *NPC_CheckEnemy (qboolean findNew, qboolean tooFarOk, qboolean setEnemy ); //setEnemy = qtrue
extern qboolean NPC_CheckAttack (float scale);
extern qboolean NPC_CheckDefend (float scale);
extern qboolean NPC_CheckCanAttack (float attack_scale, qboolean stationary);
extern int NPC_AttackDebounceForWeapon (void);
extern qboolean EntIsGlass (gentity_t *check);
extern qboolean ShotThroughGlass (trace_t *tr, gentity_t *target, vec3_t spot, int mask);
extern qboolean ValidEnemy (gentity_t *ent);
extern void G_ClearEnemy (gentity_t *self);
extern gentity_t *NPC_PickAlly ( qboolean facingEachOther, float range, qboolean ignoreGroup, qboolean movingOnly );
extern void NPC_LostEnemyDecideChase(void);
extern float NPC_MaxDistSquaredForWeapon( void );
extern qboolean NPC_EvaluateShot( int hit, qboolean glassOK );
extern int NPC_ShotEntity( gentity_t *ent, vec3_t impactPos ); //impactedPos = NULL

//NPC_formation
extern qboolean NPC_SlideMoveToGoal (void);
extern float NPC_FindClosestTeammate (gentity_t *self);
extern void NPC_CalcClosestFormationSpot(gentity_t *self);
extern void G_MaintainFormations (gentity_t *self);
extern void NPC_BSFormation (void);
extern void NPC_CreateFormation (gentity_t *self);
extern void NPC_DropFormation (gentity_t *self);
extern void NPC_ReorderFormation (gentity_t *self);
extern void NPC_InsertIntoFormation (gentity_t *self);
extern void NPC_DeleteFromFormation (gentity_t *self);

#define COLLISION_RADIUS 32
#define NUM_POSITIONS 30

//NPC spawnflags
#define SFB_RIFLEMAN	2
#define SFB_PHASER		4

#define	SFB_CINEMATIC	32
#define	SFB_NOTSOLID	64
#define	SFB_STARTINSOLID 128

//NPC_goal
extern void SetGoal( gentity_t *goal, float rating );
extern void NPC_SetGoal( gentity_t *goal, float rating );
extern void NPC_ClearGoal( void );
extern void NPC_ReachedGoal( void );
extern qboolean ReachedGoal( gentity_t *goal );
extern gentity_t *UpdateGoal( void );
extern qboolean NPC_ClearPathToGoal(vec3_t dir, gentity_t *goal);
extern qboolean NPC_MoveToGoal( qboolean tryStraight );

//NPC_reactions

//NPC_senses
#define	ALERT_CLEAR_TIME	200
#define CHECK_PVS		1
#define CHECK_360		2
#define CHECK_FOV		4
#define CHECK_SHOOT		8
#define CHECK_VISRANGE	16
extern qboolean CanSee ( gentity_t *ent );
extern qboolean InFOV ( gentity_t *ent, gentity_t *from, int hFOV, int vFOV );
extern qboolean InFOV2( vec3_t origin, gentity_t *from, int hFOV, int vFOV );
extern qboolean InFOV3( vec3_t spot, vec3_t from, vec3_t fromAngles, int hFOV, int vFOV );
extern visibility_t NPC_CheckVisibility ( gentity_t *ent, int flags );
extern qboolean InVisrange ( gentity_t *ent );

//NPC_spawn
extern void NPC_Spawn ( gentity_t *ent, gentity_t *other, gentity_t *activator );

//NPC_stats
extern int NPC_ReactionTime ( void );
extern qboolean NPC_ParseParms( const char *NPCName, gentity_t *NPC );
extern void NPC_LoadParms( void );

//NPC_utils
extern int	teamNumbers[TEAM_NUM_TEAMS];
extern int	teamStrength[TEAM_NUM_TEAMS];
extern int	teamCounter[TEAM_NUM_TEAMS];
extern void CalcEntitySpot ( const gentity_t *ent, const spot_t spot, vec3_t point );
extern qboolean NPC_UpdateAngles ( qboolean doPitch, qboolean doYaw );
extern void NPC_UpdateShootAngles (vec3_t angles, qboolean doPitch, qboolean doYaw );
extern qboolean NPC_UpdateFiringAngles ( qboolean doPitch, qboolean doYaw );
extern void SetTeamNumbers (void);
extern qboolean G_ActivateBehavior (gentity_t *self, int bset );
extern void NPC_AimWiggle( vec3_t enemy_org );
extern void NPC_ClearLookTarget( gentity_t *self );
extern void NPC_SetLookTarget( gentity_t *self, int entNum, int clearTime );

//g_nav.cpp
extern int NAV_FindClosestWaypointForEnt (gentity_t *ent, int targWp);
extern qboolean NAV_CheckAhead( gentity_t *self, vec3_t end, trace_t *trace, int clipmask );

//NPC_combat
extern float IdealDistance ( gentity_t *self );

//g_squad
extern void NPC_SetSayState (gentity_t *self, gentity_t *to, int saying);

//g_utils
extern qboolean G_CheckInSolid (gentity_t *self, qboolean fix);

//MCG - End============================================================

// NPC.cpp
extern void NPC_SetAnim(gentity_t *ent, int type, int anim, int priority);
extern qboolean NPC_EnemyTooFar(gentity_t *enemy, float dist, qboolean toShoot);

// ==================================================================

//rww - special system for sync'ing bone angles between client and server.
void NPC_SetBoneAngles(gentity_t *ent, char *bone, vec3_t angles);

//rww - and another method of automatically managing surface status for the client and server at once
void NPC_SetSurfaceOnOff(gentity_t *ent, const char *surfaceName, int surfaceFlags);

extern qboolean NPC_ClearLOS( const vec3_t start, const vec3_t end );
extern qboolean NPC_ClearLOS5( const vec3_t end );
extern qboolean NPC_ClearLOS4( gentity_t *ent ) ;
extern qboolean NPC_ClearLOS3( const vec3_t start, gentity_t *ent );
extern qboolean NPC_ClearLOS2( gentity_t *ent, const vec3_t end );

extern qboolean NPC_ClearShot( gentity_t *ent );

extern int NPC_FindCombatPoint( const vec3_t position, const vec3_t avoidPosition, vec3_t enemyPosition, const int flags, const float avoidDist, const int ignorePoint ); //ignorePoint = -1


extern qboolean NPC_ReserveCombatPoint( int combatPointID );
extern qboolean NPC_FreeCombatPoint( int combatPointID, qboolean failed ); //failed = qfalse
extern qboolean NPC_SetCombatPoint( int combatPointID );

#define	CP_ANY			0			//No flags
#define	CP_COVER		0x00000001	//The enemy cannot currently shoot this position
#define CP_CLEAR		0x00000002	//This cover point has a clear shot to the enemy
#define CP_FLEE			0x00000004	//This cover point is marked as a flee point
#define CP_DUCK			0x00000008	//This cover point is marked as a duck point
#define CP_NEAREST		0x00000010	//Find the nearest combat point
#define CP_AVOID_ENEMY	0x00000020	//Avoid our enemy
#define CP_INVESTIGATE	0x00000040	//A special point worth enemy investigation if searching
#define	CP_SQUAD		0x00000080	//Squad path
#define	CP_AVOID		0x00000100	//Avoid supplied position
#define	CP_APPROACH_ENEMY 0x00000200	//Try to get closer to enemy
#define	CP_CLOSEST		0x00000400	//Take the closest combatPoint to the enemy that's available
#define	CP_FLANK		0x00000800	//Pick a combatPoint behind the enemy
#define	CP_HAS_ROUTE	0x00001000	//Pick a combatPoint that we have a route to
#define	CP_SNIPE		0x00002000	//Pick a combatPoint that is marked as a sniper spot
#define	CP_SAFE			0x00004000	//Pick a combatPoint that is not have dangerTime
#define	CP_HORZ_DIST_COLL 0x00008000	//Collect combat points within *horizontal* dist
#define	CP_NO_PVS		0x00010000	//A combat point out of the PVS of enemy pos
#define	CP_RETREAT		0x00020000	//Try to get farther from enemy

#define CPF_NONE		0
#define	CPF_DUCK		0x00000001
#define	CPF_FLEE		0x00000002
#define	CPF_INVESTIGATE	0x00000004
#define	CPF_SQUAD		0x00000008
#define	CPF_LEAN		0x00000010
#define	CPF_SNIPE		0x00000020

#define	MAX_COMBAT_POINT_CHECK	32

extern qboolean NPC_ValidEnemy( gentity_t *ent );
extern qboolean NPC_CheckEnemyExt( qboolean checkAlerts ); //checkAlerts = qfalse
extern qboolean NPC_FindPlayer( void );
extern qboolean NPC_CheckCanAttackExt( void );

extern int NPC_CheckAlertEvents( qboolean checkSight, qboolean checkSound, int ignoreAlert, qboolean mustHaveOwner, int minAlertLevel ); //ignoreAlert = -1, mustHaveOwner = qfalse, minAlertLevel = AEL_MINOR
extern qboolean NPC_CheckForDanger( int alertEvent );
extern void G_AlertTeam( gentity_t *victim, gentity_t *attacker, float radius, float soundDist );

extern int NPC_FindSquadPoint( vec3_t position );

extern void ClearPlayerAlertEvents( void );

extern qboolean G_BoundsOverlap(const vec3_t mins1, const vec3_t maxs1, const vec3_t mins2, const vec3_t maxs2);
extern qboolean NAV_HitNavGoal( vec3_t point, vec3_t mins, vec3_t maxs, vec3_t dest, int radius, qboolean flying );

extern void NPC_SetMoveGoal( gentity_t *ent, vec3_t point, int radius, qboolean isNavGoal, int combatPoint, gentity_t *targetEnt ); //isNavGoal = qfalse, combatPoint = -1, targetEnt = NULL

extern qboolean NAV_ClearPathToPoint(gentity_t *self, vec3_t pmins, vec3_t pmaxs, vec3_t point, int clipmask, int okToHitEnt );
extern void NPC_ApplyWeaponFireDelay(void);

//NPC_FaceXXX suite
extern qboolean NPC_FacePosition( vec3_t position, qboolean doPitch ); //doPitch = qtrue
extern qboolean NPC_FaceEntity( gentity_t *ent, qboolean doPitch ); //doPitch = qtrue
extern qboolean NPC_FaceEnemy( qboolean doPitch ); //doPitch = qtrue

//Skill level cvar
extern vmCvar_t	g_npcspskill;

#define	NIF_NONE		0x00000000
#define	NIF_FAILED		0x00000001	//failed to find a way to the goal
#define	NIF_MACRO_NAV	0x00000002	//using macro navigation
#define	NIF_COLLISION	0x00000004	//resolving collision with an entity
#define NIF_BLOCKED		0x00000008	//blocked from moving

/*
-------------------------
struct navInfo_s
-------------------------
*/

typedef struct navInfo_s
{
	gentity_t	*blocker;
	vec3_t		direction;
	vec3_t		pathDirection;
	float		distance;
	trace_t		trace;
	int			flags;
} navInfo_t;

extern int	NAV_MoveToGoal( gentity_t *self, navInfo_t *info );
extern void NAV_GetLastMove( navInfo_t *info );
extern qboolean NAV_AvoidCollision( gentity_t *self, gentity_t *goal, navInfo_t *info );
