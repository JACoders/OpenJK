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
#define NPCAI_CUSTOM_GRAVITY	0x00200000	//Don't use g_gravity, I fly!

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

typedef struct gNPCstats_e
{//Stats, loaded in, and can be set by scripts
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
} gNPCstats_t;

// NOTE!!!  If you add any ptr fields into this structure could you please tell me so I can update the load/save code?
//	so far the only things I've got to cope with are a bunch of gentity_t*'s, but tell me if any more get added -slc
//

#define	MAX_ENEMY_POS_LAG	2400
#define	ENEMY_POS_LAG_INTERVAL	100
#define	ENEMY_POS_LAG_STEPS	(MAX_ENEMY_POS_LAG/ENEMY_POS_LAG_INTERVAL)
typedef struct
{
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
} gNPC_t;

void G_SquadPathsInit(void);
void NPC_InitGame( void );
void G_LoadBoltOns( void );
void Svcmd_NPC_f( void );
void NAV_DebugShowWaypoints (void);
void NAV_DebugShowBoxes (void);
void NAV_DebugShowSquadPaths (void);
