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

#ifndef __G_SHARED_H__
#define __G_SHARED_H__

#include "bg_public.h"
#include "g_public.h"
#include "b_public.h"
#include "../icarus/StdAfx.h"	//need stl
#include "../rd-common/tr_types.h"
#include "../cgame/cg_public.h"
#include "g_vehicles.h"
#include "hitlocs.h"
#include "bset.h"

#define	FOFS(x) offsetof(gentity_t, x)

typedef enum //# taskID_e
{
	TID_CHAN_VOICE = 0,	// Waiting for a voice sound to complete
	TID_ANIM_UPPER,		// Waiting to finish a lower anim holdtime
	TID_ANIM_LOWER,		// Waiting to finish a lower anim holdtime
	TID_ANIM_BOTH,		// Waiting to finish lower and upper anim holdtimes or normal md3 animating
	TID_MOVE_NAV,		// Trying to get to a navgoal or For ET_MOVERS
	TID_ANGLE_FACE,		// Turning to an angle or facing
	TID_BSTATE,			// Waiting for a certain bState to finish
	TID_LOCATION,		// Waiting for ent to enter a specific trigger_location
//	TID_MISSIONSTATUS,	// Waiting for player to finish reading MISSION STATUS SCREEN
	TID_RESIZE,			// Waiting for clear bbox to inflate size
	TID_SHOOT,			// Waiting for fire event
	NUM_TIDS,			// for def of taskID array
} taskID_t;


typedef enum //# material_e
{
	MAT_METAL = 0,	// scorched blue-grey metal
	MAT_GLASS,		// not a real chunk type, just plays an effect with glass sprites
	MAT_ELECTRICAL,	// sparks only
	MAT_ELEC_METAL,	// sparks/electrical type metal
	MAT_DRK_STONE,	// brown
	MAT_LT_STONE,	// tan
	MAT_GLASS_METAL,// glass sprites and METAl chunk
	MAT_METAL2,		// electrical metal type
	MAT_NONE,		// no chunks
	MAT_GREY_STONE,	// grey
	MAT_METAL3,		// METAL and METAL2 chunks
	MAT_CRATE1,		// yellow multi-colored crate chunks
	MAT_GRATE1,		// grate chunks
	MAT_ROPE,		// for yavin trial...no chunks, just wispy bits
	MAT_CRATE2,		// read multi-colored crate chunks
	MAT_WHITE_METAL,// white angular chunks

	NUM_MATERIALS

} material_t;

//===From cg_local.h================================================
#define	DEFAULT_HEADMODEL	""
#define	DEFAULT_TORSOMODEL	""
#define	DEFAULT_LEGSMODEL	"mouse"

// each client has an associated clientInfo_t
// that contains media references necessary to present the
// client model and other color coded effects
// this is regenerated each time a userinfo configstring changes

#define	MAX_CUSTOM_BASIC_SOUNDS	14
#define	MAX_CUSTOM_COMBAT_SOUNDS	17
#define	MAX_CUSTOM_EXTRA_SOUNDS	36
#define	MAX_CUSTOM_JEDI_SOUNDS	22
#define	MAX_CUSTOM_SOUNDS	(MAX_CUSTOM_JEDI_SOUNDS + MAX_CUSTOM_EXTRA_SOUNDS + MAX_CUSTOM_COMBAT_SOUNDS + MAX_CUSTOM_BASIC_SOUNDS)
// !!!!!!!!!! LOADSAVE-affecting structure !!!!!!!!!!
#pragma pack(push, 4)
class SgClientInfo
{
public:
    int32_t infoValid;
    SgArray<int8_t, MAX_QPATH> name;
    int32_t team;
    int32_t score;
    int32_t handicap;
    int32_t legsModel;
    int32_t legsSkin;
    int32_t torsoModel;
    int32_t torsoSkin;
    int32_t headModel;
    int32_t headSkin;
    int32_t animFileIndex;
    SgArray<int32_t, MAX_CUSTOM_SOUNDS> sounds;
    int32_t customBasicSoundDir;
    int32_t customCombatSoundDir;
    int32_t customExtraSoundDir;
    int32_t customJediSoundDir;
}; // SgClientInfo
#pragma pack(pop)

typedef struct {
    using SgType = SgClientInfo;


	qboolean		infoValid;

	char			name[MAX_QPATH];
	team_t			team;

	int				score;			// updated by score servercmds

	int				handicap;

	qhandle_t		legsModel;
	qhandle_t		legsSkin;

	qhandle_t		torsoModel;
	qhandle_t		torsoSkin;

	qhandle_t		headModel;
	qhandle_t		headSkin;

	int				animFileIndex;

	sfxHandle_t		sounds[MAX_CUSTOM_SOUNDS];

	char			*customBasicSoundDir;
	char			*customCombatSoundDir;
	char			*customExtraSoundDir;
	char			*customJediSoundDir;


    void sg_export(
        SgType& dst) const;

    void sg_import(
        const SgType& src);
} clientInfo_t;


//==================================================================
typedef enum
{
	MOVER_POS1,
	MOVER_POS2,
	MOVER_1TO2,
	MOVER_2TO1
} moverState_t;

// Rendering information structure


typedef enum
{
	MODEL_LEGS = 0,
	MODEL_TORSO,
	MODEL_HEAD,
	MODEL_WEAPON1,
	MODEL_WEAPON2,
	MODEL_WEAPON3,
	MODEL_EXTRA1,
	MODEL_EXTRA2,
	NUM_TARGET_MODELS
} targetModel_t;

//renderFlags
#define	RF_LOCKEDANGLE	1

// !!!!!!!!!! LOADSAVE-affecting structure !!!!!!!!!!
#pragma pack(push, 4)
class SgRenderInfo
{
public:
    SgArray<int8_t, 32> legsModelName;
    SgArray<int8_t, 32> torsoModelName;
    SgArray<int8_t, 32> headModelName;
    int32_t headYawRangeLeft;
    int32_t headYawRangeRight;
    int32_t headPitchRangeUp;
    int32_t headPitchRangeDown;
    int32_t torsoYawRangeLeft;
    int32_t torsoYawRangeRight;
    int32_t torsoPitchRangeUp;
    int32_t torsoPitchRangeDown;
    int32_t legsFrame;
    int32_t torsoFrame;
    float legsFpsMod;
    float torsoFpsMod;
    SgArray<uint8_t, 4> customRGBA;
    int32_t boneIndex1;
    int32_t boneIndex2;
    int32_t boneIndex3;
    int32_t boneIndex4;
    int32_t boneOrient;
    SgVec3 boneAngles1;
    SgVec3 boneAngles2;
    SgVec3 boneAngles3;
    SgVec3 boneAngles4;
    int32_t renderFlags;
    SgVec3 muzzlePoint;
    SgVec3 muzzleDir;
    SgVec3 muzzlePointOld;
    SgVec3 muzzleDirOld;
    int32_t mPCalcTime;
    float lockYaw;
    SgVec3 headPoint;
    SgVec3 headAngles;
    SgVec3 handRPoint;
    SgVec3 handLPoint;
    SgVec3 crotchPoint;
    SgVec3 footRPoint;
    SgVec3 footLPoint;
    SgVec3 torsoPoint;
    SgVec3 torsoAngles;
    SgVec3 eyePoint;
    SgVec3 eyeAngles;
    int32_t lookTarget;
    int32_t lookMode;
    int32_t lookTargetClearTime;
    int32_t lastVoiceVolume;
    SgVec3 lastHeadAngles;
    SgVec3 headBobAngles;
    SgVec3 targetHeadBobAngles;
    int32_t lookingDebounceTime;
    float legsYaw;
}; // SgRenderInfo
#pragma pack(pop)

typedef struct renderInfo_s
{
    using SgType = SgRenderInfo;


	// Legs model, or full model on one piece entities

	union
	{
		char	legsModelName[32];		// -slc[]
		char	modelName[32];			// -slc[]
	};

	char	torsoModelName[32];			// -slc[]
	char	headModelName[32];			// -slc[]

	//In whole degrees, How far to let the different model parts yaw and pitch
	int		headYawRangeLeft;
	int		headYawRangeRight;
	int		headPitchRangeUp;
	int		headPitchRangeDown;

	int		torsoYawRangeLeft;
	int		torsoYawRangeRight;
	int		torsoPitchRangeUp;
	int		torsoPitchRangeDown;

	int		legsFrame;
	int		torsoFrame;

	float	legsFpsMod;
	float	torsoFpsMod;

	//Fields to apply to entire model set, individual model's equivalents will modify this value
	byte	customRGBA[4];//Red Green Blue, 0 = don't apply

	//Allow up to 4 PCJ lookup values to be stored here.
	//The resolve to configstrings which contain the name of the
	//desired bone.
	int		boneIndex1;
	int		boneIndex2;
	int		boneIndex3;
	int		boneIndex4;

	//packed with x, y, z orientations for bone angles
	int		boneOrient;

	//I.. feel bad for doing this, but NPCs really just need to
	//be able to control this sort of thing from the server sometimes.
	//At least it's at the end so this stuff is never going to get sent
	//over for anything that isn't an NPC.
	vec3_t	boneAngles1; //angles of boneIndex1
	vec3_t	boneAngles2; //angles of boneIndex2
	vec3_t	boneAngles3; //angles of boneIndex3
	vec3_t	boneAngles4; //angles of boneIndex4

	//RF?
	int			renderFlags;

	//
	vec3_t		muzzlePoint;
	vec3_t		muzzleDir;
	vec3_t		muzzlePointOld;
	vec3_t		muzzleDirOld;
	//vec3_t		muzzlePointNext;	// Muzzle point one server frame in the future!
	//vec3_t		muzzleDirNext;
	int			mPCalcTime;//Last time muzzle point was calced

	//
	float		lockYaw;//

	//
	vec3_t		headPoint;//Where your tag_head is
	vec3_t		headAngles;//where the tag_head in the torso is pointing
	vec3_t		handRPoint;//where your right hand is
	vec3_t		handLPoint;//where your left hand is
	vec3_t		crotchPoint;//Where your crotch is
	vec3_t		footRPoint;//where your right hand is
	vec3_t		footLPoint;//where your left hand is
	vec3_t		torsoPoint;//Where your chest is
	vec3_t		torsoAngles;//Where the chest is pointing
	vec3_t		eyePoint;//Where your eyes are
	vec3_t		eyeAngles;//Where your eyes face
	int			lookTarget;//Which ent to look at with lookAngles
	lookMode_t	lookMode;
	int			lookTargetClearTime;//Time to clear the lookTarget
	int			lastVoiceVolume;//Last frame's voice volume
	vec3_t		lastHeadAngles;//Last headAngles, NOT actual facing of head model
	vec3_t		headBobAngles;//headAngle offsets
	vec3_t		targetHeadBobAngles;//head bob angles will try to get to targetHeadBobAngles
	int			lookingDebounceTime;//When we can stop using head looking angle behavior
	float		legsYaw;//yaw angle your legs are actually rendering at


    void sg_export(
        SgType& dst) const;

    void sg_import(
        const SgType& src);
} renderInfo_t;

// Movement information structure

/*
typedef struct moveInfo_s	// !!!!!!!!!! LOADSAVE-affecting struct !!!!!!!!
{
	vec3_t	desiredAngles;	// Desired facing angles
	float	speed;	// Speed of movement
	float	aspeed;	// Speed of angular movement
	vec3_t	moveDir;	// Direction of movement
	vec3_t	velocity;	// movement velocity
	int		flags;			// Special state flags
} moveInfo_t;
*/

typedef enum {
	CON_DISCONNECTED,
	CON_CONNECTING,
	CON_CONNECTED
} clientConnected_t;

typedef enum {
	TEAM_BEGIN,		// Beginning a team game, spawn at base
	TEAM_ACTIVE		// Now actively playing
} playerTeamStateState_t;

// !!!!!!!!!! LOADSAVE-affecting structure !!!!!!!!!!
#pragma pack(push, 4)
class SgPlayerTeamState
{
public:
    int32_t state;
    int32_t captures;
    int32_t basedefense;
    int32_t carrierdefense;
    int32_t flagrecovery;
    int32_t fragcarrier;
    int32_t assists;
    float lasthurtcarrier;
    float lastreturnedflag;
    float flagsince;
    float lastfraggedcarrier;
}; // SgPlayerTeamState
#pragma pack(pop)

typedef struct {
    using SgType = SgPlayerTeamState;


	playerTeamStateState_t	state;

	int			captures;
	int			basedefense;
	int			carrierdefense;
	int			flagrecovery;
	int			fragcarrier;
	int			assists;

	float		lasthurtcarrier;
	float		lastreturnedflag;
	float		flagsince;
	float		lastfraggedcarrier;


    void sg_export(
        SgType& dst) const;

    void sg_import(
        const SgType& src);
} playerTeamState_t;

// !!!!!!!!!! LOADSAVE-affecting structure !!!!!!!!!!
#pragma pack(push, 4)
class SgObjectives
{
public:
    int32_t display;
    int32_t status;
}; // SgObjectives
#pragma pack(pop)

typedef struct objectives_s
{
    using SgType = SgObjectives;


	qboolean	display;	// A displayable objective?
	int			status;	// Succeed or fail or pending


    void sg_export(
        SgType& dst) const;

    void sg_import(
        const SgType& src);
} objectives_t;
// NOTE: This is an arbitrary number greater than our current number of objectives with
// some fluff just in case we add more in the future.
#define MAX_MISSION_OBJ 100

// !!!!!!!!!! LOADSAVE-affecting structure !!!!!!!!!!
#pragma pack(push, 4)
class SgMissionStats
{
public:
    int32_t secretsFound;
    int32_t totalSecrets;
    int32_t shotsFired;
    int32_t hits;
    int32_t enemiesSpawned;
    int32_t enemiesKilled;
    int32_t saberThrownCnt;
    int32_t saberBlocksCnt;
    int32_t legAttacksCnt;
    int32_t armAttacksCnt;
    int32_t torsoAttacksCnt;
    int32_t otherAttacksCnt;
    SgArray<int32_t, NUM_FORCE_POWERS> forceUsed;
    SgArray<int32_t, WP_NUM_WEAPONS> weaponUsed;
}; // SgMissionStats
#pragma pack(pop)

typedef struct missionStats_s
{
    using SgType = SgMissionStats;


	int				secretsFound;					// # of secret areas found
	int				totalSecrets;					// # of secret areas that could have been found
	int				shotsFired;						// total number of shots fired
	int				hits;							// Shots that did damage
	int				enemiesSpawned;					// # of enemies spawned
	int				enemiesKilled;					// # of enemies killed
	int				saberThrownCnt;					// # of times saber was thrown
	int				saberBlocksCnt;					// # of times saber was used to block
	int				legAttacksCnt;					// # of times legs were hit with saber
	int				armAttacksCnt;					// # of times arm were hit with saber
	int				torsoAttacksCnt;				// # of times torso was hit with saber
	int				otherAttacksCnt;				// # of times anything else on a monster was hit with saber
	int				forceUsed[NUM_FORCE_POWERS];	// # of times each force power was used
	int				weaponUsed[WP_NUM_WEAPONS];		// # of times each weapon was used


    void sg_export(
        SgType& dst) const;

    void sg_import(
        const SgType& src);
} missionStats_t;

// the auto following clients don't follow a specific client
// number, but instead follow the first two active players
#define	FOLLOW_ACTIVE1	-1
#define	FOLLOW_ACTIVE2	-2

// client data that stays across multiple levels or tournament restarts
// this is achieved by writing all the data to cvar strings at game shutdown
// time and reading them back at connection time.  Anything added here
// MUST be dealt with in G_InitSessionData() / G_ReadSessionData() / G_WriteSessionData()
//
// !!!!!!!!!! LOADSAVE-affecting structure !!!!!!!!!!
#pragma pack(push, 4)
class SgClientSession
{
public:
    int32_t missionObjectivesShown;
    int32_t sessionTeam;
    SgArray<SgObjectives, MAX_MISSION_OBJ> mission_objectives;
    SgMissionStats missionStats;
}; // SgClientSession
#pragma pack(pop)

typedef struct {
    using SgType = SgClientSession;


	int				missionObjectivesShown;	// Number of times mission objectives have been updated
	team_t			sessionTeam;
	objectives_t	mission_objectives[MAX_MISSION_OBJ];
	missionStats_t	missionStats;			// Various totals while on a mission


    void sg_export(
        SgType& dst) const;

    void sg_import(
        const SgType& src);
} clientSession_t;

// client data that stays across multiple respawns, but is cleared
// on each level change or team change at ClientBegin()
// !!!!!!!!!! LOADSAVE-affecting structure !!!!!!!!!!
#pragma pack(push, 4)
class SgClientPersistant
{
public:
    int32_t connected;
    SgUserCmd lastCommand;
    SgArray<int8_t, 34> netname;
    int32_t maxHealth;
    int32_t enterTime;
    SgArray<int16_t, 3> cmd_angles;
    SgPlayerTeamState teamState;
}; // SgClientPersistant
#pragma pack(pop)

typedef struct {
    using SgType = SgClientPersistant;


	clientConnected_t	connected;
	usercmd_t	lastCommand;
	char		netname[34];
	int			maxHealth;			// for handicapping
	int			enterTime;			// level.time the client entered the game
	short		cmd_angles[3];		// angles sent over in the last command

	playerTeamState_t teamState;	// status in teamplay games


    void sg_export(
        SgType& dst) const;

    void sg_import(
        const SgType& src);
} clientPersistant_t;

typedef enum {
	BLK_NO,
	BLK_TIGHT,		// Block only attacks and shots around the saber itself, a bbox of around 12x12x12
	BLK_WIDE		// Block all attacks in an area around the player in a rough arc of 180 degrees
} saberBlockType_t;

typedef enum {
	BLOCKED_NONE,
	BLOCKED_PARRY_BROKEN,
	BLOCKED_ATK_BOUNCE,
	BLOCKED_UPPER_RIGHT,
	BLOCKED_UPPER_LEFT,
	BLOCKED_LOWER_RIGHT,
	BLOCKED_LOWER_LEFT,
	BLOCKED_TOP,
	BLOCKED_UPPER_RIGHT_PROJ,
	BLOCKED_UPPER_LEFT_PROJ,
	BLOCKED_LOWER_RIGHT_PROJ,
	BLOCKED_LOWER_LEFT_PROJ,
	BLOCKED_TOP_PROJ
} saberBlockedType_t;

typedef enum //# movetype_e
{
	MT_STATIC = 0,
	MT_WALK,
	MT_RUNJUMP,
	MT_FLYSWIM,
	NUM_MOVETYPES
} movetype_t;

// !!!!!!!!!! LOADSAVE-affecting structure !!!!!!!!!!

// this structure is cleared on each ClientSpawn(),
// except for 'client->pers' and 'client->sess'
#pragma pack(push, 4)
class SgGClient
{
public:
    SgPlayerState ps;
    SgClientPersistant pers;
    SgClientSession sess;
    int32_t lastCmdTime;
    SgUserCmd usercmd;
    int32_t buttons;
    int32_t oldbuttons;
    int32_t latched_buttons;
    int32_t damage_armor;
    int32_t damage_blood;
    SgVec3 damage_from;
    int8_t damage_fromWorld;
    int8_t noclip;
    int8_t forced_forwardmove;
    int8_t forced_rightmove;
    int32_t respawnTime;
    int32_t idleTime;
    int32_t airOutTime;
    int32_t timeResidual;
    float facial_blink;
    float facial_timer;
    int32_t facial_anim;
    SgClientInfo clientInfo;
    int32_t moveType;
    int32_t jetPackTime;
    int32_t fireDelay;
    int32_t breathPuffTime;
    int32_t playerTeam;
    int32_t enemyTeam;
    int32_t leader;
    int32_t NPC_class;
    float hiddenDist;
    SgVec3 hiddenDir;
    SgRenderInfo renderInfo;
    int8_t dismembered;
    int8_t dismemberProbLegs;
    int8_t dismemberProbHead;
    int8_t dismemberProbArms;
    int8_t dismemberProbHands;
    int8_t dismemberProbWaist;
    int32_t standheight;
    int32_t crouchheight;
    int32_t poisonDamage;
    int32_t poisonTime;
    int32_t slopeRecalcTime;
    SgVec3 pushVec;
    int32_t pushVecTime;
    int32_t noRagTime;
    int32_t isRagging;
    int32_t overridingBones;
    SgVec3 ragLastOrigin;
    int32_t ragLastOriginTime;
    int32_t pushEffectFadeTime;
    SgVec3 pushEffectOrigin;
    int32_t rocketLockIndex;
    float rocketLastValidTime;
    float rocketLockTime;
    float rocketTargetTime;
    int32_t inSpaceSuffocation;
    int32_t inSpaceIndex;
}; // SgGClient
#pragma pack(pop)

struct gclient_s {
    using SgType = SgGClient;


	// ps MUST be the first element, because the server expects it
	playerState_t	ps;				// communicated by server to clients

	// private to game
	clientPersistant_t	pers;
	clientSession_t		sess;

	int			lastCmdTime;		// level.time of last usercmd_t, for EF_CONNECTION

	usercmd_t	usercmd;			// most recent usercmd

	int			buttons;
	int			oldbuttons;
	int			latched_buttons;

	// sum up damage over an entire frame, so
	// shotgun blasts give a single big kick
	int			damage_armor;		// damage absorbed by armor
	int			damage_blood;		// damage taken out of health
	vec3_t		damage_from;		// origin for vector calculation
	bool		damage_fromWorld;	// if true, don't use the damage_from vector
	bool		noclip;
//icarus forced moving.  is this still used?
	signed char		forced_forwardmove;
	signed char		forced_rightmove;

	// timers
	int			respawnTime;		// can respawn when time > this, force after g_forcerespwan
	int			idleTime;			// for playing idleAnims

	int			airOutTime;

	// timeResidual is used to handle events that happen every second
	// like health / armor countdowns and regeneration
	int			timeResidual;

	// Facial Expression Timers

	float		facial_blink;		// time before next blink. If a minus value, we are in blink mode
	float		facial_timer;		// time before next alert, frown or smile. If a minus value, we are in anim mode
	int			facial_anim;		// anim to show in anim mode

	//Client info - updated when ClientInfoChanged is called, instead of using configstrings
	clientInfo_t	clientInfo;
	movetype_t		moveType;
	int				jetPackTime;
	int				fireDelay;		//msec to delay calling G_FireWeapon after EV_FIREWEAPON event is called

	// The time at which a breath should be triggered. -Aurelio
	int				breathPuffTime;

	//Used to be in gentity_t, now here.. mostly formation stuff
	team_t		playerTeam;
	team_t		enemyTeam;
	gentity_t	*leader;
	class_t		NPC_class;

	//FIXME: could combine these
	float		hiddenDist;//How close ents have to be to pick you up as an enemy
	vec3_t		hiddenDir;//Normalized direction in which NPCs can't see you (you are hidden)

	renderInfo_t	renderInfo;

	//dismember tracker
	bool		dismembered;
	char		dismemberProbLegs;	// probability of the legs being dismembered (located in NPC.cfg, 0 = never, 100 = always)
	char		dismemberProbHead;  // probability of the head being dismembered (located in NPC.cfg, 0 = never, 100 = always)
	char		dismemberProbArms;  // probability of the arms being dismembered (located in NPC.cfg, 0 = never, 100 = always)
	char		dismemberProbHands; // probability of the hands being dismembered (located in NPC.cfg, 0 = never, 100 = always)
	char		dismemberProbWaist; // probability of the waist being dismembered (located in NPC.cfg, 0 = never, 100 = always)

	int			standheight;
	int			crouchheight;
	int			poisonDamage;				// Amount of poison damage to be given
	int			poisonTime;					// When to apply poison damage
	int			slopeRecalcTime;			// debouncer for slope-foot-height-diff calcing

	vec3_t		pushVec;
	int			pushVecTime;

	int			noRagTime; //don't do ragdoll stuff if > level.time
	qboolean	isRagging;
	int			overridingBones; //dragging body or doing something else to override one or more ragdoll effector's/pcj's

	vec3_t		ragLastOrigin; //keeping track of positions between rags while dragging corpses
	int			ragLastOriginTime;

	//push refraction effect vars
	int			pushEffectFadeTime;
	vec3_t		pushEffectOrigin;

	//Rocket locking vars for non-player clients (only Vehicles use these right now...)
	int			rocketLockIndex;
	float		rocketLastValidTime;
	float		rocketLockTime;
	float		rocketTargetTime;

	//for trigger_space brushes
	int			inSpaceSuffocation;
	int			inSpaceIndex;


    void sg_export(
        SgType& dst) const;

    void sg_import(
        const SgType& src);
};

#define	MAX_PARMS	16
#define	MAX_PARM_STRING_LENGTH	MAX_QPATH//was 16, had to lengthen it so they could take a valid file path

#pragma pack(push, 4)
class SgParms
{
public:
    SgArray2d<char, MAX_PARMS, MAX_PARM_STRING_LENGTH> parm;
}; // SgParms
#pragma pack(pop)

typedef struct
{
    using SgType = SgParms;


	char	parm[MAX_PARMS][MAX_PARM_STRING_LENGTH];


    void sg_export(
        SgType& dst) const;

    void sg_import(
        const SgType& src);
} parms_t;

#ifdef GAME_INCLUDE
//these hold the place for the enums in functions.h so i don't have to recompile everytime it changes
#define thinkFunc_t int
#define clThinkFunc_t int
#define reachedFunc_t int
#define blockedFunc_t int
#define touchFunc_t int
#define useFunc_t int
#define painFunc_t int
#define dieFunc_t int

#define MAX_FAILED_NODES 8
#define MAX_INHAND_WEAPONS	2


typedef struct centity_s centity_t;
// !!!!!!!!!!! LOADSAVE-affecting struct !!!!!!!!!!!!!
#pragma pack(push, 4)
class SgGEntity
{
public:
    SgEntityState s;
    int32_t client;
    int32_t inuse;
    int32_t linked;
    int32_t svFlags;
    int32_t bmodel;
    SgVec3 mins;
    SgVec3 maxs;
    int32_t contents;
    SgVec3 absmin;
    SgVec3 absmax;
    SgVec3 currentOrigin;
    SgVec3 currentAngles;
    int32_t owner;
    SgCGhoul2InfoV ghoul2;
    SgVec3 modelScale;
    int32_t classname;
    int32_t spawnflags;
    int32_t flags;
    int32_t model;
    int32_t model2;
    int32_t freetime;
    int32_t eventTime;
    int32_t freeAfterEvent;
    float physicsBounce;
    int32_t clipmask;
    float speed;
    float resultspeed;
    int32_t lastMoveTime;
    SgVec3 movedir;
    SgVec3 lastOrigin;
    SgVec3 lastAngles;
    float mass;
    int32_t lastImpact;
    int32_t watertype;
    int32_t waterlevel;
    int16_t wupdate;
    int16_t prev_waterlevel;
    float angle;
    int32_t target;
    int32_t target2;
    int32_t target3;
    int32_t target4;
    int32_t targetJump;
    int32_t targetname;
    int32_t team;
    int32_t roff;
    int32_t roff_ctr;
    int32_t next_roff_time;
    int32_t fx_time;
    int32_t nextthink;
    int32_t e_ThinkFunc;
    int32_t e_clThinkFunc;
    int32_t e_ReachedFunc;
    int32_t e_BlockedFunc;
    int32_t e_TouchFunc;
    int32_t e_UseFunc;
    int32_t e_PainFunc;
    int32_t e_DieFunc;
    int32_t health;
    int32_t max_health;
    int32_t takedamage;
    int32_t material;
    int32_t damage;
    int32_t dflags;
    int32_t splashDamage;
    int32_t splashRadius;
    int32_t methodOfDeath;
    int32_t splashMethodOfDeath;
    SgArray<int32_t, HL_MAX> locationDamage;
    int32_t chain;
    int32_t enemy;
    int32_t activator;
    int32_t teamchain;
    int32_t teammaster;
    int32_t lastEnemy;
    float wait;
    float random;
    int32_t delay;
    int32_t alt_fire;
    int32_t count;
    int32_t bounceCount;
    int32_t fly_sound_debounce_time;
    int32_t painDebounceTime;
    int32_t disconnectDebounceTime;
    int32_t attackDebounceTime;
    int32_t pushDebounceTime;
    int32_t aimDebounceTime;
    int32_t useDebounceTime;
    int32_t trigger_formation;
    int32_t spawnContents;
    int32_t waypoint;
    int32_t wayedge;
    int32_t lastWaypoint;
    int32_t lastInAirTime;
    int32_t noWaypointTime;
    int32_t combatPoint;
    SgVec3 followPos;
    int32_t followPosRecalcTime;
    int32_t followPosWaypoint;
    int32_t loopAnim;
    int32_t startFrame;
    int32_t endFrame;
    int32_t m_iIcarusID;
    SgArray<int32_t, NUM_TIDS> taskID;
    int32_t parms;
    SgArray<int32_t, NUM_BSETS> behaviorSet;
    int32_t script_targetname;
    int32_t delayScriptTime;
    int32_t soundSet;
    int32_t setTime;
    int32_t cameraGroup;
    int32_t noDamageTeam;
    int16_t playerModel;
    SgArray<int16_t, MAX_INHAND_WEAPONS> weaponModel;
    int16_t handRBolt;
    int16_t handLBolt;
    int16_t headBolt;
    int16_t cervicalBolt;
    int16_t chestBolt;
    int16_t gutBolt;
    int16_t torsoBolt;
    int16_t crotchBolt;
    int16_t motionBolt;
    int16_t kneeLBolt;
    int16_t kneeRBolt;
    int16_t elbowLBolt;
    int16_t elbowRBolt;
    int16_t footLBolt;
    int16_t footRBolt;
    int16_t faceBone;
    int16_t craniumBone;
    int16_t cervicalBone;
    int16_t thoracicBone;
    int16_t upperLumbarBone;
    int16_t lowerLumbarBone;
    int16_t hipsBone;
    int16_t motionBone;
    int16_t rootBone;
    int16_t footLBone;
    int16_t footRBone;
    int16_t humerusRBone;
    int16_t genericBone1;
    int16_t genericBone2;
    int16_t genericBone3;
    int16_t genericBolt1;
    int16_t genericBolt2;
    int16_t genericBolt3;
    int16_t genericBolt4;
    int16_t genericBolt5;
    int32_t cinematicModel;
    int32_t m_pVehicle;
    int32_t NPC;
    int32_t ownername;
    int32_t cantHitEnemyCounter;
    int32_t NPC_type;
    int32_t NPC_targetname;
    int32_t NPC_target;
    int32_t moverState;
    int32_t soundPos1;
    int32_t sound1to2;
    int32_t sound2to1;
    int32_t soundPos2;
    int32_t soundLoop;
    int32_t nextTrain;
    int32_t prevTrain;
    SgVec3 pos1;
    SgVec3 pos2;
    SgVec3 pos3;
    int32_t sounds;
    int32_t closetarget;
    int32_t opentarget;
    int32_t paintarget;
    int32_t lockCount;
    float radius;
    int32_t wpIndex;
    int32_t noise_index;
    SgVec4 startRGBA;
    SgVec4 finalRGBA;
    int32_t item;
    int32_t message;
    float lightLevel;
    int32_t forcePushTime;
    int32_t forcePuller;
}; // SgGEntity
#pragma pack(pop)

struct gentity_s {
    using SgType = SgGEntity;


	entityState_t	s;				// communicated by server to clients
	struct gclient_s	*client;	// NULL if not a player (unless it's NPC ( if (this->NPC != NULL)  )  <sigh>... -slc)
	qboolean	inuse;
	qboolean	linked;				// qfalse if not in any good cluster

	int			svFlags;			// SVF_NOCLIENT, SVF_BROADCAST, etc

	qboolean	bmodel;				// if false, assume an explicit mins / maxs bounding box
									// only set by gi.SetBrushModel
	vec3_t		mins, maxs;
	int			contents;			// CONTENTS_TRIGGER, CONTENTS_SOLID, CONTENTS_BODY, etc
									// a non-solid entity should set to 0

	vec3_t		absmin, absmax;		// derived from mins/maxs and origin + rotation

	// currentOrigin will be used for all collision detection and world linking.
	// it will not necessarily be the same as the trajectory evaluation for the current
	// time, because each entity must be moved one at a time after time is advanced
	// to avoid simultanious collision issues
	vec3_t		currentOrigin;
	vec3_t		currentAngles;

	gentity_t	*owner;				// objects never interact with their owners, to
									// prevent player missiles from immediately
									// colliding with their owner
/*
Ghoul2 Insert Start
*/
	// this marker thing of Jake's is used for memcpy() length calcs, so don't put any ordinary fields (like above)
	//	below this point or they won't work, and will mess up all sorts of stuff.
	//
	CGhoul2Info_v	ghoul2;

	vec3_t			modelScale; //needed for g2 collision
/*
Ghoul2 Insert End
*/

	// DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER
	// EXPECTS THE FIELDS IN THAT ORDER!

//==========================================================================================

//Essential entity fields
	// note: all the char* fields from here on should be left as ptrs, not declared, because of the way that ent-parsing
	//	works by forcing field offset ptrs as char* and using G_NewString()!! (see G_ParseField() in gmae/g_spawn.cpp -slc
	//
	char		*classname;			// set in QuakeEd
	int			spawnflags;			// set in QuakeEd

	int			flags;				// FL_* variables

	char		*model;				// Normal model, or legs model on tri-models
	char		*model2;			// Torso model

	int			freetime;			// sv.time when the object was freed

	int			eventTime;			// events will be cleared EVENT_VALID_MSEC after set
	qboolean	freeAfterEvent;
//	qboolean	unlinkAfterEvent;

//Physics and movement fields
	float		physicsBounce;		// 1.0 = continuous bounce, 0.0 = no bounce
	int			clipmask;			// brushes with this content value will be collided against
									// when moving.  items and corpses do not collide against
									// players, for instance
//	moveInfo_t	moveInfo;		//FIXME: use this more?
	float		speed;
	float		resultspeed;
	int			lastMoveTime;
	vec3_t		movedir;
	vec3_t		lastOrigin;			//Where you were last frame
	vec3_t		lastAngles;			//Where you were looking last frame
	float		mass;				//How heavy you are
	int			lastImpact;			//Last time you impacted something

//Variables reflecting environment
	int			watertype;
	int			waterlevel;
	short		wupdate;
	short		prev_waterlevel;

//Targeting/linking fields
	float		angle;			// set in editor, -1 = up, -2 = down
	char		*target;
	char		*target2;		//For multiple targets, not used for firing/triggering/using, though, only for path branches
	char		*target3;		//For multiple targets, not used for firing/triggering/using, though, only for path branches
	char		*target4;		//For multiple targets, not used for firing/triggering/using, though, only for path branches
	char		*targetJump;
	char		*targetname;
	char		*team;

	union
	{
		char      *roff;                  // the roff file to use, if there is one
		char	*fxFile;		// name of the external effect file
	};

	int		roff_ctr;		// current roff frame we are playing

	int			next_roff_time;
	int			fx_time;		// timer for beam in/out effects.

//Think Functions
	int			nextthink;//Used to determine if it's time to call e_ThinkFunc again
	thinkFunc_t		e_ThinkFunc;//Called once every game frame for every ent
	clThinkFunc_t	e_clThinkFunc;//Think func for equivalent centity
	reachedFunc_t	e_ReachedFunc;// movers call this when hitting endpoint
	blockedFunc_t	e_BlockedFunc;
	touchFunc_t		e_TouchFunc;
	useFunc_t		e_UseFunc;	//Called by G_UseTargets
	painFunc_t		e_PainFunc;	//Called by G_Damage when damage is taken
	dieFunc_t		e_DieFunc;	//Called by G_Damage when health reaches <= 0

//Health and damage fields
	int			health;
	int			max_health;
	qboolean	takedamage;
	material_t	material;
	int			damage;
	int			dflags;
	//explosives, breakable brushes
	int			splashDamage;	// quad will increase this without increasing radius
	int			splashRadius;
	int			methodOfDeath;
	int			splashMethodOfDeath;
	//int			hitLoc;//where you were last hit
	int			locationDamage[HL_MAX];		// Damage accumulated on different body locations

//Entity pointers
	gentity_t	*chain;
	gentity_t	*enemy;
	gentity_t	*activator;
	gentity_t	*teamchain;		// next entity in team
	gentity_t	*teammaster;	// master of the team
	gentity_t	*lastEnemy;

//Timing variables, counters and debounce times
	float		wait;
	float		random;
	int			delay;
	qboolean	alt_fire;
	int			count;
	int			bounceCount;
	int			fly_sound_debounce_time;	// wind tunnel
	int			painDebounceTime;
	int			disconnectDebounceTime;
	int			attackDebounceTime;
	int			pushDebounceTime;
	int			aimDebounceTime;
	int			useDebounceTime;

//Unions for miscellaneous fields used under very specific circumstances
	union
	{
		qboolean	trigger_formation;
		qboolean	misc_dlight_active;
		qboolean	has_bounced;	// for thermal Det.  we force at least one bounce to happen before it can do proximity checks
	};

//Navigation
	int			spawnContents;		// store contents of ents on spawn so nav system can restore them
	int			waypoint;			//Set once per frame, if you've moved, and if someone asks
	int			wayedge;			//Used by doors and breakable things to know what edge goes through them
	int			lastWaypoint;		//To make sure you don't double-back
	int			lastInAirTime;
	int			noWaypointTime;		//Debouncer - so don't keep checking every waypoint in existance every frame that you can't find one
	int			combatPoint;
	vec3_t		followPos;
	int			followPosRecalcTime;
	int			followPosWaypoint;

//Animation
	qboolean	loopAnim;
	int		startFrame;
	int		endFrame;

//Script/ICARUS-related fields
	int				m_iIcarusID;
	int				taskID[NUM_TIDS];
	parms_t			*parms;
	char		*behaviorSet[NUM_BSETS];
	char		*script_targetname;
	int			delayScriptTime;

// Ambient sound info
	char			*soundSet;	//Only used for local sets
	int				setTime;

//Used by cameras to locate subjects
	char			*cameraGroup;

//For damage
	team_t		noDamageTeam;

// Ghoul2 Animation info
	short			playerModel;
	short			weaponModel[MAX_INHAND_WEAPONS];
	short			handRBolt;
	short			handLBolt;
	short			headBolt;
	short			cervicalBolt;
	short			chestBolt;
	short			gutBolt;
	short			torsoBolt;
	short			crotchBolt;
	short			motionBolt;
	short			kneeLBolt;
	short			kneeRBolt;
	short			elbowLBolt;
	short			elbowRBolt;
	short			footLBolt;
	short			footRBolt;
	short			faceBone;
	short			craniumBone;
	short			cervicalBone;
	short			thoracicBone;
	short			upperLumbarBone;
	short			lowerLumbarBone;
	short			hipsBone;
	short			motionBone;
	short			rootBone;
	short			footLBone;
	short			footRBone;
	short			humerusRBone;

	short			genericBone1;		// For bones special to an entity
	short			genericBone2;
	short			genericBone3;

	short			genericBolt1;		// For bolts special to an entity
	short			genericBolt2;
	short			genericBolt3;
	short			genericBolt4;
	short			genericBolt5;

	qhandle_t		cinematicModel;

//==========================================================================================

//FIELDS USED EXCLUSIVELY BY SPECIFIC CLASSES OF ENTITIES
	// Vehicle information.
	// The vehicle object.
	Vehicle_t *m_pVehicle;

	//NPC/Player entity fields
	//FIXME: Make these client only?
	gNPC_t		*NPC;//Only allocated if the entity becomes an NPC

	//Other NPC/Player-related entity fields
	char		*ownername;//Used by squadpaths to locate owning NPC

//FIXME: Only used by NPCs, move it to gNPC_t
	int			cantHitEnemyCounter;//HACK - Makes them look for another enemy on the same team if the one they're after can't be hit

//Only used by NPC_spawners
	char		*NPC_type;
	char		*NPC_targetname;
	char		*NPC_target;

//Variables used by movers (most likely exclusively by them)
	moverState_t moverState;
	int			soundPos1;
	int			sound1to2;
	int			sound2to1;
	int			soundPos2;
	int			soundLoop;
	gentity_t	*nextTrain;
	gentity_t	*prevTrain;
	vec3_t		pos1, pos2;
	vec3_t		pos3;
	int			sounds;
	char		*closetarget;
	char		*opentarget;
	char		*paintarget;
	int			lockCount;	//for maglocks- actually get put on the trigger for the door

//Variables used only by waypoints (for the most part)
	float		radius;

	union
	{
		int		wpIndex;
		int		fxID;			// id of the external effect file
	};

	int			noise_index;

	vec4_t		startRGBA;

	union
	{
		vec4_t		finalRGBA;
		vec3_t		pos4;
		vec3_t		modelAngles;	//for brush entities with an attached md3 model, as an offset to the brush's angles
	};

//FIXME: Are these being used anymore?
	gitem_t		*item;			// for bonus items -
	char		*message;		//Used by triggers to print a message when activated

	float		lightLevel;

	//FIXME: can these be removed/condensed/absorbed?
	//Rendering info
	//int			color;

	//Force effects
	int			forcePushTime;
	int			forcePuller;	//who force-pulled me (so we don't damage them if we hit them)


    void sg_export(
        SgType& dst) const;

    void sg_import(
        const SgType& src);
};
#endif //#ifdef GAME_INCLUDE

extern	gentity_t		g_entities[MAX_GENTITIES];
extern	game_import_t	gi;

// each WP_* weapon enum has an associated weaponInfo_t
// that contains media references necessary to present the
// weapon and its effects
typedef struct weaponInfo_s {
	qboolean		registered;
	gitem_t			*item;

	qhandle_t		handsModel;			// the hands don't actually draw, they just position the weapon
	qhandle_t		weaponModel;		//for in view
	qhandle_t		weaponWorldModel;	//for in their hands
	qhandle_t		barrelModel[4];

	vec3_t			weaponMidpoint;		// so it will rotate centered instead of by tag

	qhandle_t		weaponIcon;			// The version of the icon with a glowy background
	qhandle_t		weaponIconNoAmmo;	// The version of the icon with no ammo warning
	qhandle_t		ammoIcon;

	qhandle_t		ammoModel;

	qhandle_t		missileModel;
	sfxHandle_t		missileSound;
	void			(*missileTrailFunc)( centity_t *, const struct weaponInfo_s *wi );

	qhandle_t		alt_missileModel;
	sfxHandle_t		alt_missileSound;
	void			(*alt_missileTrailFunc)( centity_t *, const struct weaponInfo_s *wi );

//	sfxHandle_t		flashSound;
//	sfxHandle_t		altFlashSound;

	sfxHandle_t		firingSound;
	sfxHandle_t		altFiringSound;

	sfxHandle_t		stopSound;

	sfxHandle_t		missileHitSound;
	sfxHandle_t		altmissileHitSound;

	sfxHandle_t		chargeSound;
	sfxHandle_t		altChargeSound;

	sfxHandle_t		selectSound;	// sound played when weapon is selected
} weaponInfo_t;

extern sfxHandle_t CAS_GetBModelSound( const char *name, int stage );

enum
{
	EDGE_NORMAL,
	EDGE_PATH,
	EDGE_BLOCKED,
	EDGE_FAILED,
	EDGE_FLY,
	EDGE_JUMP,
	EDGE_LARGE,
	EDGE_PATHBLOCKED,
	EDGE_NEARESTVALID,
	EDGE_NEARESTINVALID,

	EDGE_NODE_FLOATING,
	EDGE_NODE_NORMAL,
	EDGE_NODE_GOAL,
	EDGE_NODE_COMBAT,

	EDGE_CELL,
	EDGE_CELL_EMPTY,
	EDGE_IMPACT_SAFE,
	EDGE_IMPACT_POSSIBLE,
	EDGE_THRUST,
	EDGE_VELOCITY,

	EDGE_FOLLOWPOS,

	EDGE_WHITE_ONESECOND,
	EDGE_WHITE_TWOSECOND,
	EDGE_RED_ONESECOND,
	EDGE_RED_TWOSECOND,
};

enum
{
	NODE_NORMAL,
	NODE_FLOATING,
	NODE_GOAL,
	NODE_NAVGOAL,
};

#endif // #ifndef __G_SHARED_H__
