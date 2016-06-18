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

#ifndef __B_PUBLIC_H__
#define __B_PUBLIC_H__

#include "bstate.h"
#include "ai.h"

#define NPCAI_CHECK_WEAPON		0x00000001
#define NPCAI_BURST_WEAPON		0x00000002
#define NPCAI_MOVING			0x00000004
#define NPCAI_TOUCHED_GOAL		0x00000008
#define NPCAI_PUSHED			0x00000010
#define NPCAI_NO_COLL_AVOID		0x00000020
#define NPCAI_BLOCKED			0x00000040
#define NPCAI_OFF_PATH			0x00000100
#define NPCAI_IN_SQUADPOINT		0x00000200
#define NPCAI_STRAIGHT_TO_DESTPOS	0x00000400
#define NPCAI_NO_SLOWDOWN		0x00001000
#define NPCAI_LOST				0x00002000	//Can't nav to his goal
#define NPCAI_SHIELDS			0x00004000	//Has shields, borg can adapt
#define NPCAI_GREET_ALLIES		0x00008000	//Say hi to nearby allies
#define NPCAI_FORM_TELE_NAV		0x00010000	//Tells formation people to use nav info to get to 
#define NPCAI_ENROUTE_TO_HOMEWP 0x00020000	//Lets us know to run our lostenemyscript when we get to homeWp
#define NPCAI_MATCHPLAYERWEAPON 0x00040000	//Match the player's weapon except when it changes during cinematics
#define NPCAI_DIE_ON_IMPACT		0x00100000	//Next time you crashland, die!

//Script flags
#define	SCF_CROUCHED		0x00000001	//Force ucmd.upmove to be -127
#define	SCF_WALKING			0x00000002	//Force BUTTON_WALKING to be pressed
#define	SCF_MORELIGHT		0x00000004	//NPC will have a minlight of 96
#define	SCF_LEAN_RIGHT		0x00000008	//Force rightmove+BUTTON_USE
#define	SCF_LEAN_LEFT		0x00000010	//Force leftmove+BUTTON_USE
#define	SCF_RUNNING			0x00000020	//Takes off walking button, overrides SCF_WALKING
#define	SCF_ALT_FIRE		0x00000040	//Force to use alt-fire when firing
#define	SCF_NO_RESPONSE		0x00000080	//NPC will not do generic responses to being used
#define	SCF_FFDEATH			0x00000100	//Just tells player_die to run the friendly fire deathscript
#define	SCF_NO_COMBAT_TALK	0x00000200	//NPC will not use their generic combat chatter stuff
#define	SCF_CHASE_ENEMIES	0x00000400	//NPC chase enemies - FIXME: right now this is synonymous with using combat points... should it be?
#define	SCF_LOOK_FOR_ENEMIES	0x00000800	//NPC be on the lookout for enemies
#define	SCF_FACE_MOVE_DIR	0x00001000	//NPC face direction it's moving - FIXME: not really implemented right now
#define	SCF_IGNORE_ALERTS	0x00002000	//NPC ignore alert events
#define SCF_DONT_FIRE		0x00004000	//NPC won't shoot
#define	SCF_DONT_FLEE		0x00008000	//NPC never flees
#define	SCF_FORCED_MARCH	0x00010000	//NPC that the player must aim at to make him walk
#define	SCF_NO_GROUPS		0x00020000	//NPC cannot alert groups or be part of a group
#define	SCF_FIRE_WEAPON		0x00040000	//NPC will fire his (her) weapon
#define	SCF_NO_MIND_TRICK	0x00080000	//Not succeptible to mind tricks
#define	SCF_USE_CP_NEAREST	0x00100000	//Will use combat point close to it, not next to player or try and flank player
#define	SCF_NO_FORCE		0x00200000	//Not succeptible to force powers
#define	SCF_NO_FALLTODEATH	0x00400000	//NPC will not scream and tumble and fall to hit death over large drops
#define	SCF_NO_ACROBATICS	0x00800000	//Jedi won't jump, roll or cartwheel
#define	SCF_USE_SUBTITLES	0x01000000	//Regardless of subtitle setting, this NPC will display subtitles when it speaks lines
#define	SCF_NO_ALERT_TALK	0x02000000	//Will not say alert sounds, but still can be woken up by alerts

//#ifdef __DEBUG

//Debug flag definitions

#define	AID_IDLE		0x00000000	//Nothing is happening
#define AID_ACQUIRED	0x00000001	//A target has been found
#define AID_LOST		0x00000002	//Alert, but no target is in sight
#define AID_CONFUSED	0x00000004	//Is unable to come up with a course of action
#define AID_LOSTPATH	0x00000008	//Cannot make a valid movement due to lack of connections

//#endif //__DEBUG

//extern qboolean showWaypoints;

typedef enum {VIS_UNKNOWN, VIS_NOT, VIS_PVS, VIS_360, VIS_FOV, VIS_SHOOT} visibility_t;
typedef enum {SPOT_ORIGIN, SPOT_CHEST, SPOT_HEAD, SPOT_HEAD_LEAN, SPOT_WEAPON, SPOT_LEGS, SPOT_GROUND} spot_t;

typedef enum //# lookMode_e
{
	LM_ENT = 0,
	LM_INTEREST
} lookMode_t;

typedef enum //# jumpState_e
{
	JS_WAITING = 0,
	JS_FACING,
	JS_CROUCHING,
	JS_JUMPING,
	JS_LANDING
} jumpState_t;

typedef enum //# movetype_e
{
	MT_STATIC = 0,
	MT_WALK,
	MT_RUNJUMP,
	MT_FLYSWIM,
	NUM_MOVETYPES
} movetype_t;

#pragma pack(push, 4)
class SgGNpcStats
{
public:
    int32_t aggression;
    int32_t aim;
    float earshot;
    int32_t evasion;
    int32_t hfov;
    int32_t intelligence;
    int32_t move;
    int32_t reactions;
    float shootDistance;
    int32_t vfov;
    float vigilance;
    float visrange;
    int32_t moveType;
    int32_t runSpeed;
    int32_t walkSpeed;
    float yawSpeed;
    int32_t health;
    int32_t acceleration;
}; // SgGNpcStats
#pragma pack(pop)

typedef struct gNPCstats_e
{//Stats, loaded in, and can be set by scripts
    using SgType = SgGNpcStats;


	//AI
	int		aggression;			//			"
	int		aim;				//			"
	float	earshot;			//			"
	int		evasion;			//			"
	int		hfov;				// horizontal field of view
	int		intelligence;		//			"
	int		move;				//			"
	int		reactions;			// 1-5, higher is better
	float	shootDistance;		//Maximum range- overrides range set for weapon if nonzero
	int		vfov;				// vertical field of view
	float	vigilance;			//			"
	float	visrange;			//			"
	//Movement
	movetype_t	moveType;
	int		runSpeed;
	int		walkSpeed;
	float	yawSpeed;			// 1 - whatever, default is 50
	int		health;
	int		acceleration;


    void sg_export(
        SgType& dst) const
    {
        ::sg_export(aggression, dst.aggression);
        ::sg_export(aim, dst.aim);
        ::sg_export(earshot, dst.earshot);
        ::sg_export(evasion, dst.evasion);
        ::sg_export(hfov, dst.hfov);
        ::sg_export(intelligence, dst.intelligence);
        ::sg_export(move, dst.move);
        ::sg_export(reactions, dst.reactions);
        ::sg_export(shootDistance, dst.shootDistance);
        ::sg_export(vfov, dst.vfov);
        ::sg_export(vigilance, dst.vigilance);
        ::sg_export(visrange, dst.visrange);
        ::sg_export(moveType, dst.moveType);
        ::sg_export(runSpeed, dst.runSpeed);
        ::sg_export(walkSpeed, dst.walkSpeed);
        ::sg_export(yawSpeed, dst.yawSpeed);
        ::sg_export(health, dst.health);
        ::sg_export(acceleration, dst.acceleration);
    }

    void sg_import(
        const SgType& src)
    {
        ::sg_import(src.aggression, aggression);
        ::sg_import(src.aim, aim);
        ::sg_import(src.earshot, earshot);
        ::sg_import(src.evasion, evasion);
        ::sg_import(src.hfov, hfov);
        ::sg_import(src.intelligence, intelligence);
        ::sg_import(src.move, move);
        ::sg_import(src.reactions, reactions);
        ::sg_import(src.shootDistance, shootDistance);
        ::sg_import(src.vfov, vfov);
        ::sg_import(src.vigilance, vigilance);
        ::sg_import(src.visrange, visrange);
        ::sg_import(src.moveType, moveType);
        ::sg_import(src.runSpeed, runSpeed);
        ::sg_import(src.walkSpeed, walkSpeed);
        ::sg_import(src.yawSpeed, yawSpeed);
        ::sg_import(src.health, health);
        ::sg_import(src.acceleration, acceleration);
    }
} gNPCstats_t;

// NOTE!!!  If you add any ptr fields into this structure could you please tell me so I can update the load/save code?
//	so far the only things I've got to cope with are a bunch of gentity_t*'s, but tell me if any more get added -slc
//

#define	MAX_ENEMY_POS_LAG	2400
#define	ENEMY_POS_LAG_INTERVAL	100
#define	ENEMY_POS_LAG_STEPS	(MAX_ENEMY_POS_LAG/ENEMY_POS_LAG_INTERVAL)

#pragma pack(push, 4)
class SgGNpc
{
public:
    int32_t timeOfDeath;
    int32_t touchedByPlayer;
    int32_t enemyLastVisibility;
    int32_t aimTime;
    float desiredYaw;
    float desiredPitch;
    float lockedDesiredYaw;
    float lockedDesiredPitch;
    int32_t aimingBeam;
    SgVec3 enemyLastSeenLocation;
    int32_t enemyLastSeenTime;
    SgVec3 enemyLastHeardLocation;
    int32_t enemyLastHeardTime;
    int32_t lastAlertID;
    int32_t eFlags;
    int32_t aiFlags;
    int32_t currentAmmo;
    int32_t shotTime;
    int32_t burstCount;
    int32_t burstMin;
    int32_t burstMean;
    int32_t burstMax;
    int32_t burstSpacing;
    int32_t attackHold;
    int32_t attackHoldTime;
    SgVec3 shootAngles;
    int32_t rank;
    int32_t behaviorState;
    int32_t defaultBehavior;
    int32_t tempBehavior;
    int32_t ignorePain;
    int32_t duckDebounceTime;
    int32_t walkDebounceTime;
    int32_t enemyCheckDebounceTime;
    int32_t investigateDebounceTime;
    int32_t investigateCount;
    SgVec3 investigateGoal;
    int32_t investigateSoundDebounceTime;
    int32_t greetingDebounceTime;
    int32_t eventOwner;
    int32_t coverTarg;
    int32_t jumpState;
    float followDist;
    int32_t tempGoal;
    int32_t goalEntity;
    int32_t lastGoalEntity;
    int32_t eventualGoal;
    int32_t captureGoal;
    int32_t defendEnt;
    int32_t greetEnt;
    int32_t goalTime;
    int32_t straightToGoal;
    float distToGoal;
    int32_t navTime;
    int32_t blockingEntNum;
    int32_t blockedSpeechDebounceTime;
    int32_t lastSideStepSide;
    int32_t sideStepHoldTime;
    int32_t homeWp;
    int32_t group;
    SgVec3 lastPathAngles;
    SgGNpcStats stats;
    int32_t aimErrorDebounceTime;
    float lastAimErrorYaw;
    float lastAimErrorPitch;
    SgVec3 aimOfs;
    int32_t currentAim;
    int32_t currentAggression;
    int32_t scriptFlags;
    int32_t desiredSpeed;
    int32_t currentSpeed;
    int8_t last_forwardmove;
    int8_t last_rightmove;
    SgVec3 lastClearOrigin;
    int32_t consecutiveBlockedMoves;
    int32_t blockedDebounceTime;
    int32_t shoveCount;
    SgVec3 blockedDest;
    int32_t combatPoint;
    int32_t lastFailedCombatPoint;
    int32_t movementSpeech;
    float movementSpeechChance;
    int32_t nextBStateThink;
    SgUserCmd last_ucmd;
    int32_t combatMove;
    int32_t goalRadius;
    int32_t pauseTime;
    int32_t standTime;
    int32_t localState;
    int32_t squadState;
    int32_t confusionTime;
    int32_t charmedTime;
    int32_t controlledTime;
    int32_t surrenderTime;
    SgArray<SgVec3, ENEMY_POS_LAG_STEPS> enemyLaggedPos;
    int32_t watchTarget;
    int32_t ffireCount;
    int32_t ffireDebounce; 
    int32_t ffireFadeDebounce;
}; // SgGNpc
#pragma pack(pop)

typedef struct 
{
    using SgType = SgGNpc;


	//FIXME: Put in playerInfo or something
	int			timeOfDeath;			//FIXME do we really need both of these
	gentity_t	*touchedByPlayer;

	visibility_t	enemyLastVisibility;

	int			aimTime;
	float		desiredYaw;
	float		desiredPitch;
	float		lockedDesiredYaw;
	float		lockedDesiredPitch;
	gentity_t	*aimingBeam;		// debugging aid

	vec3_t		enemyLastSeenLocation;
	int			enemyLastSeenTime;
	vec3_t		enemyLastHeardLocation;
	int			enemyLastHeardTime;
	int			lastAlertID;		//unique ID

	int			eFlags;
	int			aiFlags;

	int			currentAmmo;		// this sucks, need to find a better way
	int			shotTime;
	int			burstCount;
	int			burstMin;
	int			burstMean;
	int			burstMax;
	int			burstSpacing;
	int			attackHold;
	int			attackHoldTime;
	vec3_t		shootAngles;		//Angles to where bot is shooting - fixme: make he torso turn to reflect these

	//extra character info
	rank_t		rank;		//for pips

	//Behavior state info
	bState_t	behaviorState;	//determines what actions he should be doing
	bState_t	defaultBehavior;//State bot will default to if none other set
	bState_t	tempBehavior;//While valid, overrides other behavior

	qboolean	ignorePain;		//only play pain scripts when take pain

	int			duckDebounceTime;//Keeps them ducked for a certain time
	int			walkDebounceTime;
	int			enemyCheckDebounceTime;
	int			investigateDebounceTime;
	int			investigateCount;
	vec3_t		investigateGoal;
	int			investigateSoundDebounceTime;
	int			greetingDebounceTime;//when we can greet someone next
	gentity_t	*eventOwner;
	
	//bState-specific fields
	gentity_t	*coverTarg;
	jumpState_t	jumpState;
	float		followDist;

	// goal, navigation & pathfinding
	gentity_t	*tempGoal;			// used for locational goals (player's last seen/heard position)
	gentity_t	*goalEntity;
	gentity_t	*lastGoalEntity;
	gentity_t	*eventualGoal;
	gentity_t	*captureGoal;		//Where we should try to capture
	gentity_t	*defendEnt;			//Who we're trying to protect
	gentity_t	*greetEnt;			//Who we're greeting
	int			goalTime;	//FIXME: This is never actually used
	qboolean	straightToGoal;	//move straight at navgoals
	float		distToGoal;
	int			navTime;
	int			blockingEntNum;
	int			blockedSpeechDebounceTime;
	int			lastSideStepSide;
	int			sideStepHoldTime;
	int			homeWp;
	AIGroupInfo_t	*group;

	vec3_t		lastPathAngles;		//So we know which way to face generally when we stop

	//stats
	gNPCstats_t	stats;
	int			aimErrorDebounceTime;
	float		lastAimErrorYaw;
	float		lastAimErrorPitch;
	vec3_t		aimOfs;
	int			currentAim;
	int			currentAggression;

	//scriptflags
	int			scriptFlags;//in b_local.h

	//moveInfo
	int			desiredSpeed;
	int			currentSpeed;
	char		last_forwardmove;
	char		last_rightmove;
	vec3_t		lastClearOrigin;
	int			consecutiveBlockedMoves;
	int			blockedDebounceTime;
	int			shoveCount;
	vec3_t		blockedDest;

	//
	int			combatPoint;//NPCs in bState BS_COMBAT_POINT will find their closest empty combat_point
	int			lastFailedCombatPoint;//NPCs in bState BS_COMBAT_POINT will find their closest empty combat_point
	int			movementSpeech;	//what to say when you first successfully move
	float		movementSpeechChance;//how likely you are to say it

	//Testing physics at 20fps
	int			nextBStateThink;
	usercmd_t	last_ucmd;

	//
	//JWEIER ADDITIONS START

	qboolean	combatMove;
	int			goalRadius;

	//FIXME: These may be redundant
	
	/*
	int			weaponTime;		//Time until refire is valid
	int			jumpTime;
	*/
	int			pauseTime;		//Time to stand still
	int			standTime;

	int			localState;		//Tracking information local to entity
	int			squadState;		//Tracking information for team level interaction

	//JWEIER ADDITIONS END
	//

	int			confusionTime;	//Doesn't respond to alerts or pick up enemies (unless shot) until this time is up
	int			charmedTime;	//charmed to enemy team
	int			controlledTime;	//controlled by player
	int			surrenderTime;	//Hands up

	//Lagging enemy position - FIXME: seems awful wasteful...
	vec3_t		enemyLaggedPos[ENEMY_POS_LAG_STEPS];

	gentity_t	*watchTarget;	//for BS_CINEMATIC, keeps facing this ent

	int			ffireCount;		//sigh... you'd think I'd be able to find a way to do this without having to use 3 int fields, but...
	int			ffireDebounce;	
	int			ffireFadeDebounce;


    void sg_export(
        SgType& dst) const
    {
        ::sg_export(timeOfDeath, dst.timeOfDeath);
        ::sg_export(touchedByPlayer, dst.touchedByPlayer);
        ::sg_export(enemyLastVisibility, dst.enemyLastVisibility);
        ::sg_export(aimTime, dst.aimTime);
        ::sg_export(desiredYaw, dst.desiredYaw);
        ::sg_export(desiredPitch, dst.desiredPitch);
        ::sg_export(lockedDesiredYaw, dst.lockedDesiredYaw);
        ::sg_export(lockedDesiredPitch, dst.lockedDesiredPitch);
        ::sg_export(aimingBeam, dst.aimingBeam);
        ::sg_export(enemyLastSeenLocation, dst.enemyLastSeenLocation);
        ::sg_export(enemyLastSeenTime, dst.enemyLastSeenTime);
        ::sg_export(enemyLastHeardLocation, dst.enemyLastHeardLocation);
        ::sg_export(enemyLastHeardTime, dst.enemyLastHeardTime);
        ::sg_export(lastAlertID, dst.lastAlertID);
        ::sg_export(eFlags, dst.eFlags);
        ::sg_export(aiFlags, dst.aiFlags);
        ::sg_export(currentAmmo, dst.currentAmmo);
        ::sg_export(shotTime, dst.shotTime);
        ::sg_export(burstCount, dst.burstCount);
        ::sg_export(burstMin, dst.burstMin);
        ::sg_export(burstMean, dst.burstMean);
        ::sg_export(burstMax, dst.burstMax);
        ::sg_export(burstSpacing, dst.burstSpacing);
        ::sg_export(attackHold, dst.attackHold);
        ::sg_export(attackHoldTime, dst.attackHoldTime);
        ::sg_export(shootAngles, dst.shootAngles);
        ::sg_export(rank, dst.rank);
        ::sg_export(behaviorState, dst.behaviorState);
        ::sg_export(defaultBehavior, dst.defaultBehavior);
        ::sg_export(tempBehavior, dst.tempBehavior);
        ::sg_export(ignorePain, dst.ignorePain);
        ::sg_export(duckDebounceTime, dst.duckDebounceTime);
        ::sg_export(walkDebounceTime, dst.walkDebounceTime);
        ::sg_export(enemyCheckDebounceTime, dst.enemyCheckDebounceTime);
        ::sg_export(investigateDebounceTime, dst.investigateDebounceTime);
        ::sg_export(investigateCount, dst.investigateCount);
        ::sg_export(investigateGoal, dst.investigateGoal);
        ::sg_export(investigateSoundDebounceTime, dst.investigateSoundDebounceTime);
        ::sg_export(greetingDebounceTime, dst.greetingDebounceTime);
        ::sg_export(eventOwner, dst.eventOwner);
        ::sg_export(coverTarg, dst.coverTarg);
        ::sg_export(jumpState, dst.jumpState);
        ::sg_export(followDist, dst.followDist);
        ::sg_export(tempGoal, dst.tempGoal);
        ::sg_export(goalEntity, dst.goalEntity);
        ::sg_export(lastGoalEntity, dst.lastGoalEntity);
        ::sg_export(eventualGoal, dst.eventualGoal);
        ::sg_export(captureGoal, dst.captureGoal);
        ::sg_export(defendEnt, dst.defendEnt);
        ::sg_export(greetEnt, dst.greetEnt);
        ::sg_export(goalTime, dst.goalTime);
        ::sg_export(straightToGoal, dst.straightToGoal);
        ::sg_export(distToGoal, dst.distToGoal);
        ::sg_export(navTime, dst.navTime);
        ::sg_export(blockingEntNum, dst.blockingEntNum);
        ::sg_export(blockedSpeechDebounceTime, dst.blockedSpeechDebounceTime);
        ::sg_export(lastSideStepSide, dst.lastSideStepSide);
        ::sg_export(sideStepHoldTime, dst.sideStepHoldTime);
        ::sg_export(homeWp, dst.homeWp);
        ::sg_export(group, dst.group);
        ::sg_export(lastPathAngles, dst.lastPathAngles);
        ::sg_export(stats, dst.stats);
        ::sg_export(aimErrorDebounceTime, dst.aimErrorDebounceTime);
        ::sg_export(lastAimErrorYaw, dst.lastAimErrorYaw);
        ::sg_export(lastAimErrorPitch, dst.lastAimErrorPitch);
        ::sg_export(aimOfs, dst.aimOfs);
        ::sg_export(currentAim, dst.currentAim);
        ::sg_export(currentAggression, dst.currentAggression);
        ::sg_export(scriptFlags, dst.scriptFlags);
        ::sg_export(desiredSpeed, dst.desiredSpeed);
        ::sg_export(currentSpeed, dst.currentSpeed);
        ::sg_export(last_forwardmove, dst.last_forwardmove);
        ::sg_export(last_rightmove, dst.last_rightmove);
        ::sg_export(lastClearOrigin, dst.lastClearOrigin);
        ::sg_export(consecutiveBlockedMoves, dst.consecutiveBlockedMoves);
        ::sg_export(blockedDebounceTime, dst.blockedDebounceTime);
        ::sg_export(shoveCount, dst.shoveCount);
        ::sg_export(blockedDest, dst.blockedDest);
        ::sg_export(combatPoint, dst.combatPoint);
        ::sg_export(lastFailedCombatPoint, dst.lastFailedCombatPoint);
        ::sg_export(movementSpeech, dst.movementSpeech);
        ::sg_export(movementSpeechChance, dst.movementSpeechChance);
        ::sg_export(nextBStateThink, dst.nextBStateThink);
        ::sg_export(last_ucmd, dst.last_ucmd);
        ::sg_export(combatMove, dst.combatMove);
        ::sg_export(goalRadius, dst.goalRadius);
        ::sg_export(pauseTime, dst.pauseTime);
        ::sg_export(standTime, dst.standTime);
        ::sg_export(localState, dst.localState);
        ::sg_export(squadState, dst.squadState);
        ::sg_export(confusionTime, dst.confusionTime);
        ::sg_export(charmedTime, dst.charmedTime);
        ::sg_export(controlledTime, dst.controlledTime);
        ::sg_export(surrenderTime, dst.surrenderTime);
        ::sg_export(enemyLaggedPos, dst.enemyLaggedPos);
        ::sg_export(watchTarget, dst.watchTarget);
        ::sg_export(ffireCount, dst.ffireCount);
        ::sg_export(ffireDebounce, dst.ffireDebounce); 
        ::sg_export(ffireFadeDebounce, dst.ffireFadeDebounce);
    }

    void sg_import(
        const SgType& src)
    {
        ::sg_import(src.timeOfDeath, timeOfDeath);
        ::sg_import(src.touchedByPlayer, touchedByPlayer);
        ::sg_import(src.enemyLastVisibility, enemyLastVisibility);
        ::sg_import(src.aimTime, aimTime);
        ::sg_import(src.desiredYaw, desiredYaw);
        ::sg_import(src.desiredPitch, desiredPitch);
        ::sg_import(src.lockedDesiredYaw, lockedDesiredYaw);
        ::sg_import(src.lockedDesiredPitch, lockedDesiredPitch);
        ::sg_import(src.aimingBeam, aimingBeam);
        ::sg_import(src.enemyLastSeenLocation, enemyLastSeenLocation);
        ::sg_import(src.enemyLastSeenTime, enemyLastSeenTime);
        ::sg_import(src.enemyLastHeardLocation, enemyLastHeardLocation);
        ::sg_import(src.enemyLastHeardTime, enemyLastHeardTime);
        ::sg_import(src.lastAlertID, lastAlertID);
        ::sg_import(src.eFlags, eFlags);
        ::sg_import(src.aiFlags, aiFlags);
        ::sg_import(src.currentAmmo, currentAmmo);
        ::sg_import(src.shotTime, shotTime);
        ::sg_import(src.burstCount, burstCount);
        ::sg_import(src.burstMin, burstMin);
        ::sg_import(src.burstMean, burstMean);
        ::sg_import(src.burstMax, burstMax);
        ::sg_import(src.burstSpacing, burstSpacing);
        ::sg_import(src.attackHold, attackHold);
        ::sg_import(src.attackHoldTime, attackHoldTime);
        ::sg_import(src.shootAngles, shootAngles);
        ::sg_import(src.rank, rank);
        ::sg_import(src.behaviorState, behaviorState);
        ::sg_import(src.defaultBehavior, defaultBehavior);
        ::sg_import(src.tempBehavior, tempBehavior);
        ::sg_import(src.ignorePain, ignorePain);
        ::sg_import(src.duckDebounceTime, duckDebounceTime);
        ::sg_import(src.walkDebounceTime, walkDebounceTime);
        ::sg_import(src.enemyCheckDebounceTime, enemyCheckDebounceTime);
        ::sg_import(src.investigateDebounceTime, investigateDebounceTime);
        ::sg_import(src.investigateCount, investigateCount);
        ::sg_import(src.investigateGoal, investigateGoal);
        ::sg_import(src.investigateSoundDebounceTime, investigateSoundDebounceTime);
        ::sg_import(src.greetingDebounceTime, greetingDebounceTime);
        ::sg_import(src.eventOwner, eventOwner);
        ::sg_import(src.coverTarg, coverTarg);
        ::sg_import(src.jumpState, jumpState);
        ::sg_import(src.followDist, followDist);
        ::sg_import(src.tempGoal, tempGoal);
        ::sg_import(src.goalEntity, goalEntity);
        ::sg_import(src.lastGoalEntity, lastGoalEntity);
        ::sg_import(src.eventualGoal, eventualGoal);
        ::sg_import(src.captureGoal, captureGoal);
        ::sg_import(src.defendEnt, defendEnt);
        ::sg_import(src.greetEnt, greetEnt);
        ::sg_import(src.goalTime, goalTime);
        ::sg_import(src.straightToGoal, straightToGoal);
        ::sg_import(src.distToGoal, distToGoal);
        ::sg_import(src.navTime, navTime);
        ::sg_import(src.blockingEntNum, blockingEntNum);
        ::sg_import(src.blockedSpeechDebounceTime, blockedSpeechDebounceTime);
        ::sg_import(src.lastSideStepSide, lastSideStepSide);
        ::sg_import(src.sideStepHoldTime, sideStepHoldTime);
        ::sg_import(src.homeWp, homeWp);
        ::sg_import(src.group, group);
        ::sg_import(src.lastPathAngles, lastPathAngles);
        ::sg_import(src.stats, stats);
        ::sg_import(src.aimErrorDebounceTime, aimErrorDebounceTime);
        ::sg_import(src.lastAimErrorYaw, lastAimErrorYaw);
        ::sg_import(src.lastAimErrorPitch, lastAimErrorPitch);
        ::sg_import(src.aimOfs, aimOfs);
        ::sg_import(src.currentAim, currentAim);
        ::sg_import(src.currentAggression, currentAggression);
        ::sg_import(src.scriptFlags, scriptFlags);
        ::sg_import(src.desiredSpeed, desiredSpeed);
        ::sg_import(src.currentSpeed, currentSpeed);
        ::sg_import(src.last_forwardmove, last_forwardmove);
        ::sg_import(src.last_rightmove, last_rightmove);
        ::sg_import(src.lastClearOrigin, lastClearOrigin);
        ::sg_import(src.consecutiveBlockedMoves, consecutiveBlockedMoves);
        ::sg_import(src.blockedDebounceTime, blockedDebounceTime);
        ::sg_import(src.shoveCount, shoveCount);
        ::sg_import(src.blockedDest, blockedDest);
        ::sg_import(src.combatPoint, combatPoint);
        ::sg_import(src.lastFailedCombatPoint, lastFailedCombatPoint);
        ::sg_import(src.movementSpeech, movementSpeech);
        ::sg_import(src.movementSpeechChance, movementSpeechChance);
        ::sg_import(src.nextBStateThink, nextBStateThink);
        ::sg_import(src.last_ucmd, last_ucmd);
        ::sg_import(src.combatMove, combatMove);
        ::sg_import(src.goalRadius, goalRadius);
        ::sg_import(src.pauseTime, pauseTime);
        ::sg_import(src.standTime, standTime);
        ::sg_import(src.localState, localState);
        ::sg_import(src.squadState, squadState);
        ::sg_import(src.confusionTime, confusionTime);
        ::sg_import(src.charmedTime, charmedTime);
        ::sg_import(src.controlledTime, controlledTime);
        ::sg_import(src.surrenderTime, surrenderTime);
        ::sg_import(src.enemyLaggedPos, enemyLaggedPos);
        ::sg_import(src.watchTarget, watchTarget);
        ::sg_import(src.ffireCount, ffireCount);
        ::sg_import(src.ffireDebounce, ffireDebounce); 
        ::sg_import(src.ffireFadeDebounce, ffireFadeDebounce);
    }
} gNPC_t;

void G_SquadPathsInit(void);
void NPC_InitGame( void );
void G_LoadBoltOns( void );
void Svcmd_NPC_f( void );
void NAV_DebugShowWaypoints (void);
void NAV_DebugShowBoxes (void);
void NAV_DebugShowSquadPaths (void);
/*
void Bot_InitGame( void );
void Bot_InitPreSpawn( void );
void Bot_InitPostSpawn( void );
void Bot_Shutdown( void );
void Bot_Think( gentity_t *ent, int msec );
void Bot_Connect( gentity_t *bot, char *botName );
void Bot_Begin( gentity_t *bot );
void Bot_Disconnect( gentity_t *bot );
void Svcmd_Bot_f( void );
void Nav_ItemSpawn( gentity_t *ent, int remaining );
*/

//
// This section should be moved to QFILES.H
//
/*
#define NAVFILE_ID			(('I')+('N'<<8)+('A'<<16)+('V'<<24))
#define NAVFILE_VERSION		6

typedef struct {
	unsigned	id;
	unsigned	version;
	unsigned	checksum;
	unsigned	surfaceCount;
	unsigned	edgeCount;
} navheader_t;


#define MAX_SURFACES	4096

#define NSF_PUSH			0x00000001
#define NSF_WATERLEVEL1		0x00000002
#define NSF_WATERLEVEL2		0x00000004
#define NSF_WATER_NOAIR		0x00000008
#define NSF_DUCK			0x00000010
#define NSF_PAIN			0x00000020
#define NSF_TELEPORTER		0x00000040
#define NSF_PLATHIGH		0x00000080
#define NSF_PLATLOW			0x00000100
#define NSF_DOOR_FLOOR		0x00000200
#define NSF_DOOR_SHOOT		0x00000400
#define NSF_DOOR_BUTTON		0x00000800
#define NSF_BUTTON			0x00001000

typedef struct {
	vec3_t		origin;
	vec2_t		absmin;
	vec2_t		absmax;
	int			parm;
	unsigned	flags;
	unsigned	edgeCount;
	unsigned	edgeIndex;
} nsurface_t;


#define NEF_DUCK			0x00000001
#define NEF_JUMP			0x00000002
#define NEF_HOLD			0x00000004
#define NEF_WALK			0x00000008
#define NEF_RUN				0x00000010
#define NEF_NOAIRMOVE		0x00000020
#define NEF_LEFTGROUND		0x00000040
#define NEF_PLAT			0x00000080
#define NEF_FALL1			0x00000100
#define NEF_FALL2			0x00000200
#define NEF_DOOR_SHOOT		0x00000400
#define NEF_DOOR_BUTTON		0x00000800
#define NEF_BUTTON			0x00001000

typedef struct {
	vec3_t		origin;
	vec2_t		absmin;		// region within this surface that is the portal to the other surface
	vec2_t		absmax;
	int			surfaceNum;
	unsigned	flags;		// jump, prerequisite button, will take falling damage, etc...
	float		cost;
	int			dirIndex;
	vec3_t		endSpot;
	int			parm;
} nedge_t;
*/
#endif
