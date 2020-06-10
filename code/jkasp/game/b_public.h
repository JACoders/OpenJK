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
#define NPCAI_SUBBOSS_CHARACTER	0x00000080	//Alora, tough reborn
#define NPCAI_OFF_PATH			0x00000100
#define NPCAI_IN_SQUADPOINT		0x00000200
#define NPCAI_STRAIGHT_TO_DESTPOS	0x00000400
#define NPCAI_HEAVY_MELEE		0x00000800	//4x melee damage, dismemberment
#define NPCAI_NO_SLOWDOWN		0x00001000
#define NPCAI_LOST				0x00002000	//Can't nav to his goal
#define NPCAI_SHIELDS			0x00004000	//Has shields, borg can adapt
#define NPCAI_GREET_ALLIES		0x00008000	//Say hi to nearby allies
#define NPCAI_FORM_TELE_NAV		0x00010000	//Tells formation people to use nav info to get to
#define NPCAI_ENROUTE_TO_HOMEWP 0x00020000	//Lets us know to run our lostenemyscript when we get to homeWp
#define NPCAI_MATCHPLAYERWEAPON 0x00040000	//Match the player's weapon except when it changes during cinematics
#define NPCAI_DIE_ON_IMPACT		0x00100000	//Next time you crashland, die!
#define NPCAI_WALKING			0x00200000
#define NPCAI_STOP_AT_LOS		0x00400000	//Stop Running When We Hit LOS
#define NPCAI_NAV_THROUGH_BREAKABLES	0x00800000	//Navigation allows connections through breakable (func_glass, func_breakable or misc_model_breakable)
#define NPCAI_KNEEL				0x01000000  //Kneel befor Zod
#define NPCAI_FLY				0x02000000	//Fly, My Pretty!
#define NPCAI_FLAMETHROW		0x04000000
#define NPCAI_ROSH				0x08000000	//I am Rosh, when I'm hurt, drop to one knee and wait for Vil or Dasariah to heal me
#define NPCAI_HEAL_ROSH			0x10000000	//Constantly look for NPC with NPC_type of rosh_dark, follow him, heal him if needbe
#define NPCAI_JUMP				0x20000000	//Jump Now
#define NPCAI_BOSS_CHARACTER	0x40000000	//Boss NPC flag for certain immunities/defenses
#define NPCAI_NO_JEDI_DELAY		0x80000000	//Reborn/Jedi don't taunt enemy before attacking

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
#define SCF_NAV_CAN_FLY		0x04000000	//Navigation allows connections through air
#define SCF_FLY_WITH_JET	0x08000000	//Must Fly With A Jet
#define	SCF_PILOT			0x10000000	//Can pilot a vehicle
#define	SCF_NAV_CAN_JUMP	0x20000000	//Can attempt to jump when blocked
#define	SCF_FIRE_WEAPON_NO_ANIM	0x40000000	//Fire weapon but don't play weapon firing anim
#define	SCF_SAFE_REMOVE		0x80000000	//Remove NPC when it's safe (when player isn't looking)


//#ifdef __DEBUG

//Debug flag definitions

#define	AID_IDLE		0x00000000	//Nothing is happening
#define AID_ACQUIRED	0x00000001	//A target has been found
#define AID_LOST		0x00000002	//Alert, but no target is in sight
#define AID_CONFUSED	0x00000004	//Is unable to come up with a course of action
#define AID_LOSTPATH	0x00000008	//Cannot make a valid movement due to lack of connections

//#endif //__DEBUG

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

typedef enum
{
	SEX_NEUTRAL = 0,
	SEX_MALE,
	SEX_FEMALE,
	SEX_SHEMALE//what the Hell, ya never know...
} sexType_t;

// !!!!!!!!!! LOADSAVE-affecting structure !!!!!!!!!!
class gNPCstats_t
{//Stats, loaded in, and can be set by scripts
public:
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
	int		runSpeed;
	int		walkSpeed;
	float	yawSpeed;			// 1 - whatever, default is 50
	int		health;
	int		acceleration;
	//sex
	sexType_t	sex;			//male, female, etc.


	void sg_export(
		ojk::SavedGameHelper& saved_game) const
	{
		saved_game.write<int32_t>(aggression);
		saved_game.write<int32_t>(aim);
		saved_game.write<float>(earshot);
		saved_game.write<int32_t>(evasion);
		saved_game.write<int32_t>(hfov);
		saved_game.write<int32_t>(intelligence);
		saved_game.write<int32_t>(move);
		saved_game.write<int32_t>(reactions);
		saved_game.write<float>(shootDistance);
		saved_game.write<int32_t>(vfov);
		saved_game.write<float>(vigilance);
		saved_game.write<float>(visrange);
		saved_game.write<int32_t>(runSpeed);
		saved_game.write<int32_t>(walkSpeed);
		saved_game.write<float>(yawSpeed);
		saved_game.write<int32_t>(health);
		saved_game.write<int32_t>(acceleration);
		saved_game.write<int32_t>(sex);
	}

	void sg_import(
		ojk::SavedGameHelper& saved_game)
	{
		saved_game.read<int32_t>(aggression);
		saved_game.read<int32_t>(aim);
		saved_game.read<float>(earshot);
		saved_game.read<int32_t>(evasion);
		saved_game.read<int32_t>(hfov);
		saved_game.read<int32_t>(intelligence);
		saved_game.read<int32_t>(move);
		saved_game.read<int32_t>(reactions);
		saved_game.read<float>(shootDistance);
		saved_game.read<int32_t>(vfov);
		saved_game.read<float>(vigilance);
		saved_game.read<float>(visrange);
		saved_game.read<int32_t>(runSpeed);
		saved_game.read<int32_t>(walkSpeed);
		saved_game.read<float>(yawSpeed);
		saved_game.read<int32_t>(health);
		saved_game.read<int32_t>(acceleration);
		saved_game.read<int32_t>(sex);
	}
}; // gNPCstats_t


#define	MAX_ENEMY_POS_LAG	2400
#define	ENEMY_POS_LAG_INTERVAL	100
#define	ENEMY_POS_LAG_STEPS	(MAX_ENEMY_POS_LAG/ENEMY_POS_LAG_INTERVAL)

// !!!!!!!!!! LOADSAVE-affecting structure !!!!!!!!!!
class gNPC_t
{
public:
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
#ifdef BASE_SAVE_COMPAT
	int			burstMean;			// keeping for original save compatibility
#endif
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

	int			homeWp;
	int			avoidSide;
	int			leaderAvoidSide;
	int			lastAvoidSteerSide;
	int			lastAvoidSteerSideDebouncer;
	AIGroupInfo_t	*group;
	int			troop;

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
	int			shoveCount;

	int			blockedDebounceTime;
	gentity_t*	blockedEntity;				// The entity That Causes The Current Blockage

	vec3_t		blockedTargetPosition;		// Where the actor was trying to get TO before blocked
	gentity_t*	blockedTargetEntity;		// Where the actor was trying to get TO before blocked


	//jump info
	vec3_t		jumpDest;					// Where The Actor Is Trying To Jump TO
	gentity_t*	jumpTarget;					// What Entity The Actor Is Trying To Jump TO
	float		jumpMaxXYDist;				// The Minimal Delta On The XY Plane Allowed To Jump To The Dest
	float		jumpMazZDist;
	int			jumpSide;					// Which Side The Last Jump Occured On
	int			jumpTime;					// When The Last Jump Started
	int			jumpBackupTime;				// If Active, Then The Guy Should Backup Before Jumping
	int			jumpNextCheckTime;			// The Minimal Next Time To Check For A Jump

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
	int			kneelTime;		//kneeling (for troopers)

	//Lagging enemy position - FIXME: seems awful wasteful...
	vec3_t		enemyLaggedPos[ENEMY_POS_LAG_STEPS];

	gentity_t	*watchTarget;	//for BS_CINEMATIC, keeps facing this ent

	int			ffireCount;		//sigh... you'd think I'd be able to find a way to do this without having to use 3 int fields, but...
	int			ffireDebounce;
	int			ffireFadeDebounce;


	void sg_export(
		ojk::SavedGameHelper& saved_game) const
	{
		saved_game.write<int32_t>(timeOfDeath);
		saved_game.write<int32_t>(touchedByPlayer);
		saved_game.write<int32_t>(enemyLastVisibility);
		saved_game.write<int32_t>(aimTime);
		saved_game.write<float>(desiredYaw);
		saved_game.write<float>(desiredPitch);
		saved_game.write<float>(lockedDesiredYaw);
		saved_game.write<float>(lockedDesiredPitch);
		saved_game.write<int32_t>(aimingBeam);
		saved_game.write<float>(enemyLastSeenLocation);
		saved_game.write<int32_t>(enemyLastSeenTime);
		saved_game.write<float>(enemyLastHeardLocation);
		saved_game.write<int32_t>(enemyLastHeardTime);
		saved_game.write<int32_t>(lastAlertID);
		saved_game.write<int32_t>(eFlags);
		saved_game.write<int32_t>(aiFlags);
		saved_game.write<int32_t>(currentAmmo);
		saved_game.write<int32_t>(shotTime);
		saved_game.write<int32_t>(burstCount);
		saved_game.write<int32_t>(burstMin);

#ifdef BASE_SAVE_COMPAT
		saved_game.write<int32_t>(burstMean);
#endif // BASE_SAVE_COMPAT

		saved_game.write<int32_t>(burstMax);
		saved_game.write<int32_t>(burstSpacing);
		saved_game.write<int32_t>(attackHold);
		saved_game.write<int32_t>(attackHoldTime);
		saved_game.write<float>(shootAngles);
		saved_game.write<int32_t>(rank);
		saved_game.write<int32_t>(behaviorState);
		saved_game.write<int32_t>(defaultBehavior);
		saved_game.write<int32_t>(tempBehavior);
		saved_game.write<int32_t>(ignorePain);
		saved_game.write<int32_t>(duckDebounceTime);
		saved_game.write<int32_t>(walkDebounceTime);
		saved_game.write<int32_t>(enemyCheckDebounceTime);
		saved_game.write<int32_t>(investigateDebounceTime);
		saved_game.write<int32_t>(investigateCount);
		saved_game.write<float>(investigateGoal);
		saved_game.write<int32_t>(investigateSoundDebounceTime);
		saved_game.write<int32_t>(greetingDebounceTime);
		saved_game.write<int32_t>(eventOwner);
		saved_game.write<int32_t>(coverTarg);
		saved_game.write<int32_t>(jumpState);
		saved_game.write<float>(followDist);
		saved_game.write<int32_t>(tempGoal);
		saved_game.write<int32_t>(goalEntity);
		saved_game.write<int32_t>(lastGoalEntity);
		saved_game.write<int32_t>(eventualGoal);
		saved_game.write<int32_t>(captureGoal);
		saved_game.write<int32_t>(defendEnt);
		saved_game.write<int32_t>(greetEnt);
		saved_game.write<int32_t>(goalTime);
		saved_game.write<int32_t>(straightToGoal);
		saved_game.write<float>(distToGoal);
		saved_game.write<int32_t>(navTime);
		saved_game.write<int32_t>(blockingEntNum);
		saved_game.write<int32_t>(blockedSpeechDebounceTime);
		saved_game.write<int32_t>(homeWp);
		saved_game.write<int32_t>(avoidSide);
		saved_game.write<int32_t>(leaderAvoidSide);
		saved_game.write<int32_t>(lastAvoidSteerSide);
		saved_game.write<int32_t>(lastAvoidSteerSideDebouncer);
		saved_game.write<int32_t>(group);
		saved_game.write<int32_t>(troop);
		saved_game.write<float>(lastPathAngles);
		saved_game.write<>(stats);
		saved_game.write<int32_t>(aimErrorDebounceTime);
		saved_game.write<float>(lastAimErrorYaw);
		saved_game.write<float>(lastAimErrorPitch);
		saved_game.write<float>(aimOfs);
		saved_game.write<int32_t>(currentAim);
		saved_game.write<int32_t>(currentAggression);
		saved_game.write<int32_t>(scriptFlags);
		saved_game.write<int32_t>(desiredSpeed);
		saved_game.write<int32_t>(currentSpeed);
		saved_game.write<int8_t>(last_forwardmove);
		saved_game.write<int8_t>(last_rightmove);
		saved_game.skip(2);
		saved_game.write<float>(lastClearOrigin);
		saved_game.write<int32_t>(shoveCount);
		saved_game.write<int32_t>(blockedDebounceTime);
		saved_game.write<int32_t>(blockedEntity);
		saved_game.write<float>(blockedTargetPosition);
		saved_game.write<int32_t>(blockedTargetEntity);
		saved_game.write<float>(jumpDest);
		saved_game.write<int32_t>(jumpTarget);
		saved_game.write<float>(jumpMaxXYDist);
		saved_game.write<float>(jumpMazZDist);
		saved_game.write<int32_t>(jumpSide);
		saved_game.write<int32_t>(jumpTime);
		saved_game.write<int32_t>(jumpBackupTime);
		saved_game.write<int32_t>(jumpNextCheckTime);
		saved_game.write<int32_t>(combatPoint);
		saved_game.write<int32_t>(lastFailedCombatPoint);
		saved_game.write<int32_t>(movementSpeech);
		saved_game.write<float>(movementSpeechChance);
		saved_game.write<int32_t>(nextBStateThink);
		saved_game.write<>(last_ucmd);
		saved_game.write<int32_t>(combatMove);
		saved_game.write<int32_t>(goalRadius);
		saved_game.write<int32_t>(pauseTime);
		saved_game.write<int32_t>(standTime);
		saved_game.write<int32_t>(localState);
		saved_game.write<int32_t>(squadState);
		saved_game.write<int32_t>(confusionTime);
		saved_game.write<int32_t>(charmedTime);
		saved_game.write<int32_t>(controlledTime);
		saved_game.write<int32_t>(surrenderTime);
		saved_game.write<int32_t>(kneelTime);
		saved_game.write<float>(enemyLaggedPos);
		saved_game.write<int32_t>(watchTarget);
		saved_game.write<int32_t>(ffireCount);
		saved_game.write<int32_t>(ffireDebounce);
		saved_game.write<int32_t>(ffireFadeDebounce);
	}

	void sg_import(
		ojk::SavedGameHelper& saved_game)
	{
		saved_game.read<int32_t>(timeOfDeath);
		saved_game.read<int32_t>(touchedByPlayer);
		saved_game.read<int32_t>(enemyLastVisibility);
		saved_game.read<int32_t>(aimTime);
		saved_game.read<float>(desiredYaw);
		saved_game.read<float>(desiredPitch);
		saved_game.read<float>(lockedDesiredYaw);
		saved_game.read<float>(lockedDesiredPitch);
		saved_game.read<int32_t>(aimingBeam);
		saved_game.read<float>(enemyLastSeenLocation);
		saved_game.read<int32_t>(enemyLastSeenTime);
		saved_game.read<float>(enemyLastHeardLocation);
		saved_game.read<int32_t>(enemyLastHeardTime);
		saved_game.read<int32_t>(lastAlertID);
		saved_game.read<int32_t>(eFlags);
		saved_game.read<int32_t>(aiFlags);
		saved_game.read<int32_t>(currentAmmo);
		saved_game.read<int32_t>(shotTime);
		saved_game.read<int32_t>(burstCount);
		saved_game.read<int32_t>(burstMin);

#ifdef BASE_SAVE_COMPAT
		saved_game.read<int32_t>(burstMean);
#endif // BASE_SAVE_COMPAT

		saved_game.read<int32_t>(burstMax);
		saved_game.read<int32_t>(burstSpacing);
		saved_game.read<int32_t>(attackHold);
		saved_game.read<int32_t>(attackHoldTime);
		saved_game.read<float>(shootAngles);
		saved_game.read<int32_t>(rank);
		saved_game.read<int32_t>(behaviorState);
		saved_game.read<int32_t>(defaultBehavior);
		saved_game.read<int32_t>(tempBehavior);
		saved_game.read<int32_t>(ignorePain);
		saved_game.read<int32_t>(duckDebounceTime);
		saved_game.read<int32_t>(walkDebounceTime);
		saved_game.read<int32_t>(enemyCheckDebounceTime);
		saved_game.read<int32_t>(investigateDebounceTime);
		saved_game.read<int32_t>(investigateCount);
		saved_game.read<float>(investigateGoal);
		saved_game.read<int32_t>(investigateSoundDebounceTime);
		saved_game.read<int32_t>(greetingDebounceTime);
		saved_game.read<int32_t>(eventOwner);
		saved_game.read<int32_t>(coverTarg);
		saved_game.read<int32_t>(jumpState);
		saved_game.read<float>(followDist);
		saved_game.read<int32_t>(tempGoal);
		saved_game.read<int32_t>(goalEntity);
		saved_game.read<int32_t>(lastGoalEntity);
		saved_game.read<int32_t>(eventualGoal);
		saved_game.read<int32_t>(captureGoal);
		saved_game.read<int32_t>(defendEnt);
		saved_game.read<int32_t>(greetEnt);
		saved_game.read<int32_t>(goalTime);
		saved_game.read<int32_t>(straightToGoal);
		saved_game.read<float>(distToGoal);
		saved_game.read<int32_t>(navTime);
		saved_game.read<int32_t>(blockingEntNum);
		saved_game.read<int32_t>(blockedSpeechDebounceTime);
		saved_game.read<int32_t>(homeWp);
		saved_game.read<int32_t>(avoidSide);
		saved_game.read<int32_t>(leaderAvoidSide);
		saved_game.read<int32_t>(lastAvoidSteerSide);
		saved_game.read<int32_t>(lastAvoidSteerSideDebouncer);
		saved_game.read<int32_t>(group);
		saved_game.read<int32_t>(troop);
		saved_game.read<float>(lastPathAngles);
		saved_game.read<>(stats);
		saved_game.read<int32_t>(aimErrorDebounceTime);
		saved_game.read<float>(lastAimErrorYaw);
		saved_game.read<float>(lastAimErrorPitch);
		saved_game.read<float>(aimOfs);
		saved_game.read<int32_t>(currentAim);
		saved_game.read<int32_t>(currentAggression);
		saved_game.read<int32_t>(scriptFlags);
		saved_game.read<int32_t>(desiredSpeed);
		saved_game.read<int32_t>(currentSpeed);
		saved_game.read<int8_t>(last_forwardmove);
		saved_game.read<int8_t>(last_rightmove);
		saved_game.skip(2);
		saved_game.read<float>(lastClearOrigin);
		saved_game.read<int32_t>(shoveCount);
		saved_game.read<int32_t>(blockedDebounceTime);
		saved_game.read<int32_t>(blockedEntity);
		saved_game.read<float>(blockedTargetPosition);
		saved_game.read<int32_t>(blockedTargetEntity);
		saved_game.read<float>(jumpDest);
		saved_game.read<int32_t>(jumpTarget);
		saved_game.read<float>(jumpMaxXYDist);
		saved_game.read<float>(jumpMazZDist);
		saved_game.read<int32_t>(jumpSide);
		saved_game.read<int32_t>(jumpTime);
		saved_game.read<int32_t>(jumpBackupTime);
		saved_game.read<int32_t>(jumpNextCheckTime);
		saved_game.read<int32_t>(combatPoint);
		saved_game.read<int32_t>(lastFailedCombatPoint);
		saved_game.read<int32_t>(movementSpeech);
		saved_game.read<float>(movementSpeechChance);
		saved_game.read<int32_t>(nextBStateThink);
		saved_game.read<>(last_ucmd);
		saved_game.read<int32_t>(combatMove);
		saved_game.read<int32_t>(goalRadius);
		saved_game.read<int32_t>(pauseTime);
		saved_game.read<int32_t>(standTime);
		saved_game.read<int32_t>(localState);
		saved_game.read<int32_t>(squadState);
		saved_game.read<int32_t>(confusionTime);
		saved_game.read<int32_t>(charmedTime);
		saved_game.read<int32_t>(controlledTime);
		saved_game.read<int32_t>(surrenderTime);
		saved_game.read<int32_t>(kneelTime);
		saved_game.read<float>(enemyLaggedPos);
		saved_game.read<int32_t>(watchTarget);
		saved_game.read<int32_t>(ffireCount);
		saved_game.read<int32_t>(ffireDebounce);
		saved_game.read<int32_t>(ffireFadeDebounce);
	}
}; // gNPC_t


void G_SquadPathsInit(void);
void NPC_InitGame( void );
void G_LoadBoltOns( void );
void Svcmd_NPC_f( void );
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
