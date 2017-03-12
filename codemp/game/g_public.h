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

// g_public.h -- game module information visible to server

#pragma once

#include "qcommon/q_shared.h"

#define Q3_INFINITE			16777216

#define	GAME_API_VERSION	1

// entity->svFlags
// the server does not know how to interpret most of the values
// in entityStates (level eType), so the game must explicitly flag
// special server behaviors
#define	SVF_NOCLIENT			0x00000001	// don't send entity to clients, even if it has effects
#define SVF_BROADCASTCLIENTS	0x00000002	// only broadcast to clients specified in r.broadcastClients[clientNum/32]
#define SVF_BOT					0x00000008	// set if the entity is a bot
#define SVF_PLAYER_USABLE		0x00000010	// player can use this with the use button
#define	SVF_BROADCAST			0x00000020	// send to all connected clients
#define	SVF_PORTAL				0x00000040	// merge a second pvs at origin2 into snapshots
#define	SVF_USE_CURRENT_ORIGIN	0x00000080	// entity->r.currentOrigin instead of entity->s.origin
											// for link position (missiles and movers)
#define SVF_SINGLECLIENT		0x00000100	// only send to a single client (entityShared_t->singleClient)
#define SVF_NOSERVERINFO		0x00000200	// don't send CS_SERVERINFO updates to this client
											// so that it can be updated for ping tools without
											// lagging clients
#define SVF_CAPSULE				0x00000400	// use capsule for collision detection instead of bbox
#define SVF_NOTSINGLECLIENT		0x00000800	// send entity to everyone but one client
											// (entityShared_t->singleClient)

#define SVF_OWNERNOTSHARED		0x00001000	// If it's owned by something and another thing owned by that something
											// hits it, it will still touch

#define	SVF_ICARUS_FREEZE		0x00008000	// NPCs are frozen, ents don't execute ICARUS commands

#define SVF_GLASS_BRUSH			0x08000000	// Ent is a glass brush

#define SVF_NO_BASIC_SOUNDS		0x10000000	// No basic sounds
#define SVF_NO_COMBAT_SOUNDS	0x20000000	// No combat sounds
#define SVF_NO_EXTRA_SOUNDS		0x40000000	// No extra or jedi sounds

//rww - ghoul2 trace flags
#define G2TRFLAG_DOGHOULTRACE	0x00000001 //do the ghoul2 trace
#define G2TRFLAG_HITCORPSES		0x00000002 //will try g2 collision on the ent even if it's EF_DEAD
#define G2TRFLAG_GETSURFINDEX	0x00000004 //will replace surfaceFlags with the ghoul2 surface index that was hit, if any.
#define G2TRFLAG_THICK			0x00000008 //assures that the trace radius will be significantly large regardless of the trace box size.

//===============================================================

//this structure is shared by gameside and in-engine NPC nav routines.
typedef struct failedEdge_e
{
	int	startID;
	int	endID;
	int checkTime;
	int	entID;
} failedEdge_t;

typedef struct entityShared_s {
	qboolean	linked;				// qfalse if not in any good cluster
	int			linkcount;

	int			svFlags;			// SVF_NOCLIENT, SVF_BROADCAST, etc
	int			singleClient;		// only send to this client when SVF_SINGLECLIENT is set

	qboolean	bmodel;				// if false, assume an explicit mins / maxs bounding box
									// only set by trap_SetBrushModel
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
	qboolean	mIsRoffing;			// set to qtrue when the entity is being roffed

	// when a trace call is made and passEntityNum != ENTITYNUM_NONE,
	// an ent will be excluded from testing if:
	// ent->s.number == passEntityNum	(don't interact with self)
	// ent->s.ownerNum = passEntityNum	(don't interact with your own missiles)
	// entity[ent->s.ownerNum].ownerNum = passEntityNum	(don't interact with other missiles from owner)
	int			ownerNum;

	// mask of clients that this entity should be broadcast to
	// the first 32 clients are represented by the first array index and the latter 32 clients are represented by the
	//	second array index.
	uint32_t	broadcastClients[2];
} entityShared_t;

//bstate.h
typedef enum //# bState_e
{//These take over only if script allows them to be autonomous
	BS_DEFAULT = 0,//# default behavior for that NPC
	BS_ADVANCE_FIGHT,//# Advance to captureGoal and shoot enemies if you can
	BS_SLEEP,//# Play awake script when startled by sound
	BS_FOLLOW_LEADER,//# Follow your leader and shoot any enemies you come across
	BS_JUMP,//# Face navgoal and jump to it.
	BS_SEARCH,//# Using current waypoint as a base, search the immediate branches of waypoints for enemies
	BS_WANDER,//# Wander down random waypoint paths
	BS_NOCLIP,//# Moves through walls, etc.
	BS_REMOVE,//# Waits for player to leave PVS then removes itself
	BS_CINEMATIC,//# Does nothing but face it's angles and move to a goal if it has one
	//# #eol
	//internal bStates only
	BS_WAIT,//# Does nothing but face it's angles
	BS_STAND_GUARD,
	BS_PATROL,
	BS_INVESTIGATE,//# head towards temp goal and look for enemies and listen for sounds
	BS_STAND_AND_SHOOT,
	BS_HUNT_AND_KILL,
	BS_FLEE,//# Run away!
	NUM_BSTATES
} bState_t;

enum
{
	EDGE_NORMAL,
	EDGE_PATH,
	EDGE_BLOCKED,
	EDGE_FAILED,
	EDGE_MOVEDIR
};

enum
{
	NODE_NORMAL,
	NODE_START,
	NODE_GOAL,
	NODE_NAVGOAL,
};

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

typedef enum //# bSet_e
{//This should check to matching a behavior state name first, then look for a script
	BSET_INVALID = -1,
	BSET_FIRST = 0,
	BSET_SPAWN = 0,//# script to use when first spawned
	BSET_USE,//# script to use when used
	BSET_AWAKE,//# script to use when awoken/startled
	BSET_ANGER,//# script to use when aquire an enemy
	BSET_ATTACK,//# script to run when you attack
	BSET_VICTORY,//# script to run when you kill someone
	BSET_LOSTENEMY,//# script to run when you can't find your enemy
	BSET_PAIN,//# script to use when take pain
	BSET_FLEE,//# script to use when take pain below 50% of health
	BSET_DEATH,//# script to use when killed
	BSET_DELAYED,//# script to run when self->delayScriptTime is reached
	BSET_BLOCKED,//# script to run when blocked by a friendly NPC or player
	BSET_BUMPED,//# script to run when bumped into a friendly NPC or player (can set bumpRadius)
	BSET_STUCK,//# script to run when blocked by a wall
	BSET_FFIRE,//# script to run when player shoots their own teammates
	BSET_FFDEATH,//# script to run when player kills a teammate
	BSET_MINDTRICK,//# script to run when player does a mind trick on this NPC

	NUM_BSETS
} bSet_t;

#define	MAX_PARMS	16
#define	MAX_PARM_STRING_LENGTH	MAX_QPATH//was 16, had to lengthen it so they could take a valid file path
typedef struct parms_s {
	char	parm[MAX_PARMS][MAX_PARM_STRING_LENGTH];
} parms_t;

#define MAX_FAILED_NODES 8

#if (!defined(MACOS_X) && !defined(__GCC__) && !defined(__GNUC__))
typedef struct Vehicle_s Vehicle_t;
#endif

// the server looks at a sharedEntity, which is the start of the game's gentity_t structure
//mod authors should not touch this struct
typedef struct sharedEntity_s {
	entityState_t	s;				// communicated by server to clients
	playerState_t	*playerState;	//needs to be in the gentity for bg entity access
									//if you want to actually see the contents I guess
									//you will have to be sure to VMA it first.
#if (!defined(MACOS_X) && !defined(__GCC__) && !defined(__GNUC__))
	Vehicle_t		*m_pVehicle; //vehicle data
#else
	struct Vehicle_s		*m_pVehicle; //vehicle data
#endif
	void			*ghoul2; //g2 instance
	int				localAnimIndex; //index locally (game/cgame) to anim data for this skel
	vec3_t			modelScale; //needed for g2 collision

	//from here up must also be unified with bgEntity/centity

	entityShared_t	r;				// shared by both the server system and game

	//Script/ICARUS-related fields
	int				taskID[NUM_TIDS];
	parms_t			*parms;
	char			*behaviorSet[NUM_BSETS];
	char			*script_targetname;
	int				delayScriptTime;
	char			*fullName;

	//rww - targetname and classname are now shared as well. ICARUS needs access to them.
	char			*targetname;
	char			*classname;			// set in QuakeEd

	//rww - and yet more things to share. This is because the nav code is in the exe because it's all C++.
	int				waypoint;			//Set once per frame, if you've moved, and if someone asks
	int				lastWaypoint;		//To make sure you don't double-back
	int				lastValidWaypoint;	//ALWAYS valid -used for tracking someone you lost
	int				noWaypointTime;		//Debouncer - so don't keep checking every waypoint in existance every frame that you can't find one
	int				combatPoint;
	int				failedWaypoints[MAX_FAILED_NODES];
	int				failedWaypointCheckTime;

	int				next_roff_time; //rww - npc's need to know when they're getting roff'd
} sharedEntity_t;

#if !defined(_GAME) && defined(__cplusplus)
class CSequencer;
class CTaskManager;

//I suppose this could be in another in-engine header or something. But we never want to
//include an icarus file before sharedentity_t is declared.
extern CSequencer	*gSequencers[MAX_GENTITIES];
extern CTaskManager	*gTaskManagers[MAX_GENTITIES];

#include "icarus/icarus.h"
#include "icarus/sequencer.h"
#include "icarus/taskmanager.h"
#endif

typedef struct T_G_ICARUS_PLAYSOUND_s {
	int taskID;
	int entID;
	char name[2048];
	char channel[2048];
} T_G_ICARUS_PLAYSOUND;


typedef struct T_G_ICARUS_SET_s {
	int taskID;
	int entID;
	char type_name[2048];
	char data[2048];
} T_G_ICARUS_SET;

typedef struct T_G_ICARUS_LERP2POS_s {
	int taskID;
	int entID;
	vec3_t origin;
	vec3_t angles;
	float duration;
	qboolean nullAngles; //special case
} T_G_ICARUS_LERP2POS;

typedef struct T_G_ICARUS_LERP2ORIGIN_s {
	int taskID;
	int entID;
	vec3_t origin;
	float duration;
} T_G_ICARUS_LERP2ORIGIN;

typedef struct T_G_ICARUS_LERP2ANGLES_s {
	int taskID;
	int entID;
	vec3_t angles;
	float duration;
} T_G_ICARUS_LERP2ANGLES;

typedef struct T_G_ICARUS_GETTAG_s {
	int entID;
	char name[2048];
	int lookup;
	vec3_t info;
} T_G_ICARUS_GETTAG;

typedef struct T_G_ICARUS_LERP2START_s {
	int entID;
	int taskID;
	float duration;
} T_G_ICARUS_LERP2START;

typedef struct T_G_ICARUS_LERP2END_s {
	int entID;
	int taskID;
	float duration;
} T_G_ICARUS_LERP2END;

typedef struct T_G_ICARUS_USE_s {
	int entID;
	char target[2048];
} T_G_ICARUS_USE;

typedef struct T_G_ICARUS_KILL_s {
	int entID;
	char name[2048];
} T_G_ICARUS_KILL;

typedef struct T_G_ICARUS_REMOVE_s {
	int entID;
	char name[2048];
} T_G_ICARUS_REMOVE;

typedef struct T_G_ICARUS_PLAY_s {
	int taskID;
	int entID;
	char type[2048];
	char name[2048];
} T_G_ICARUS_PLAY;

typedef struct T_G_ICARUS_GETFLOAT_s {
	int entID;
	int type;
	char name[2048];
	float value;
} T_G_ICARUS_GETFLOAT;

typedef struct T_G_ICARUS_GETVECTOR_s {
	int entID;
	int type;
	char name[2048];
	vec3_t value;
} T_G_ICARUS_GETVECTOR;

typedef struct T_G_ICARUS_GETSTRING_s {
	int entID;
	int type;
	char name[2048];
	char value[2048];
} T_G_ICARUS_GETSTRING;

typedef struct T_G_ICARUS_SOUNDINDEX_s {
	char filename[2048];
} T_G_ICARUS_SOUNDINDEX;
typedef struct T_G_ICARUS_GETSETIDFORSTRING_s {
	char string[2048];
} T_G_ICARUS_GETSETIDFORSTRING;

typedef enum gameImportLegacy_e {
	G_PRINT,
	G_ERROR,
	G_MILLISECONDS,
	G_PRECISIONTIMER_START,
	G_PRECISIONTIMER_END,
	G_CVAR_REGISTER,
	G_CVAR_UPDATE,
	G_CVAR_SET,
	G_CVAR_VARIABLE_INTEGER_VALUE,
	G_CVAR_VARIABLE_STRING_BUFFER,
	G_ARGC,
	G_ARGV,
	G_FS_FOPEN_FILE,
	G_FS_READ,
	G_FS_WRITE,
	G_FS_FCLOSE_FILE,
	G_SEND_CONSOLE_COMMAND,
	G_LOCATE_GAME_DATA,
	G_DROP_CLIENT,
	G_SEND_SERVER_COMMAND,
	G_SET_CONFIGSTRING,
	G_GET_CONFIGSTRING,
	G_GET_USERINFO,
	G_SET_USERINFO,
	G_GET_SERVERINFO,
	G_SET_SERVER_CULL,
	G_SET_BRUSH_MODEL,
	G_TRACE,
	G_G2TRACE,
	G_POINT_CONTENTS,
	G_IN_PVS,
	G_IN_PVS_IGNORE_PORTALS,
	G_ADJUST_AREA_PORTAL_STATE,
	G_AREAS_CONNECTED,
	G_LINKENTITY,
	G_UNLINKENTITY,
	G_ENTITIES_IN_BOX,
	G_ENTITY_CONTACT,
	G_BOT_ALLOCATE_CLIENT,
	G_BOT_FREE_CLIENT,
	G_GET_USERCMD,
	G_GET_ENTITY_TOKEN,
	G_SIEGEPERSSET,
	G_SIEGEPERSGET,
	G_FS_GETFILELIST,
	G_DEBUG_POLYGON_CREATE,
	G_DEBUG_POLYGON_DELETE,
	G_REAL_TIME,
	G_SNAPVECTOR,
	G_TRACECAPSULE,
	G_ENTITY_CONTACTCAPSULE,
	SP_GETSTRINGTEXTSTRING,
	G_ROFF_CLEAN,
	G_ROFF_UPDATE_ENTITIES,
	G_ROFF_CACHE,
	G_ROFF_PLAY,
	G_ROFF_PURGE_ENT,
	G_TRUEMALLOC,
	G_TRUEFREE,
	G_ICARUS_RUNSCRIPT,
	G_ICARUS_REGISTERSCRIPT,
	G_ICARUS_INIT,
	G_ICARUS_VALIDENT,
	G_ICARUS_ISINITIALIZED,
	G_ICARUS_MAINTAINTASKMANAGER,
	G_ICARUS_ISRUNNING,
	G_ICARUS_TASKIDPENDING,
	G_ICARUS_INITENT,
	G_ICARUS_FREEENT,
	G_ICARUS_ASSOCIATEENT,
	G_ICARUS_SHUTDOWN,
	G_ICARUS_TASKIDSET,
	G_ICARUS_TASKIDCOMPLETE,
	G_ICARUS_SETVAR,
	G_ICARUS_VARIABLEDECLARED,
	G_ICARUS_GETFLOATVARIABLE,
	G_ICARUS_GETSTRINGVARIABLE,
	G_ICARUS_GETVECTORVARIABLE,
	G_SET_SHARED_BUFFER,

	G_MEMSET = 100,
	G_MEMCPY,
	G_STRNCPY,
	G_SIN,
	G_COS,
	G_ATAN2,
	G_SQRT,
	G_MATRIXMULTIPLY,
	G_ANGLEVECTORS,
	G_PERPENDICULARVECTOR,
	G_FLOOR,
	G_CEIL,
	G_TESTPRINTINT,
	G_TESTPRINTFLOAT,
	G_ACOS,
	G_ASIN,

	G_NAV_INIT = 200,
	G_NAV_FREE,
	G_NAV_LOAD,
	G_NAV_SAVE,
	G_NAV_ADDRAWPOINT,
	G_NAV_CALCULATEPATHS,
	G_NAV_HARDCONNECT,
	G_NAV_SHOWNODES,
	G_NAV_SHOWEDGES,
	G_NAV_SHOWPATH,
	G_NAV_GETNEARESTNODE,
	G_NAV_GETBESTNODE,
	G_NAV_GETNODEPOSITION,
	G_NAV_GETNODENUMEDGES,
	G_NAV_GETNODEEDGE,
	G_NAV_GETNUMNODES,
	G_NAV_CONNECTED,
	G_NAV_GETPATHCOST,
	G_NAV_GETEDGECOST,
	G_NAV_GETPROJECTEDNODE,
	G_NAV_CHECKFAILEDNODES,
	G_NAV_ADDFAILEDNODE,
	G_NAV_NODEFAILED,
	G_NAV_NODESARENEIGHBORS,
	G_NAV_CLEARFAILEDEDGE,
	G_NAV_CLEARALLFAILEDEDGES,
	G_NAV_EDGEFAILED,
	G_NAV_ADDFAILEDEDGE,
	G_NAV_CHECKFAILEDEDGE,
	G_NAV_CHECKALLFAILEDEDGES,
	G_NAV_ROUTEBLOCKED,
	G_NAV_GETBESTNODEALTROUTE,
	G_NAV_GETBESTNODEALT2,
	G_NAV_GETBESTPATHBETWEENENTS,
	G_NAV_GETNODERADIUS,
	G_NAV_CHECKBLOCKEDEDGES,
	G_NAV_CLEARCHECKEDNODES,
	G_NAV_CHECKEDNODE,
	G_NAV_SETCHECKEDNODE,
	G_NAV_FLAGALLNODES,
	G_NAV_GETPATHSCALCULATED,
	G_NAV_SETPATHSCALCULATED,

	BOTLIB_SETUP = 250,
	BOTLIB_SHUTDOWN,
	BOTLIB_LIBVAR_SET,
	BOTLIB_LIBVAR_GET,
	BOTLIB_PC_ADD_GLOBAL_DEFINE,
	BOTLIB_START_FRAME,
	BOTLIB_LOAD_MAP,
	BOTLIB_UPDATENTITY,
	BOTLIB_TEST,
	BOTLIB_GET_SNAPSHOT_ENTITY,
	BOTLIB_GET_CONSOLE_MESSAGE,
	BOTLIB_USER_COMMAND,

	BOTLIB_AAS_ENABLE_ROUTING_AREA = 300,
	BOTLIB_AAS_BBOX_AREAS,
	BOTLIB_AAS_AREA_INFO,
	BOTLIB_AAS_ENTITY_INFO,
	BOTLIB_AAS_INITIALIZED,
	BOTLIB_AAS_PRESENCE_TYPE_BOUNDING_BOX,
	BOTLIB_AAS_TIME,
	BOTLIB_AAS_POINT_AREA_NUM,
	BOTLIB_AAS_TRACE_AREAS,
	BOTLIB_AAS_POINT_CONTENTS,
	BOTLIB_AAS_NEXT_BSP_ENTITY,
	BOTLIB_AAS_VALUE_FOR_BSP_EPAIR_KEY,
	BOTLIB_AAS_VECTOR_FOR_BSP_EPAIR_KEY,
	BOTLIB_AAS_FLOAT_FOR_BSP_EPAIR_KEY,
	BOTLIB_AAS_INT_FOR_BSP_EPAIR_KEY,
	BOTLIB_AAS_AREA_REACHABILITY,
	BOTLIB_AAS_AREA_TRAVEL_TIME_TO_GOAL_AREA,
	BOTLIB_AAS_SWIMMING,
	BOTLIB_AAS_PREDICT_CLIENT_MOVEMENT,

	BOTLIB_EA_SAY = 400,
	BOTLIB_EA_SAY_TEAM,
	BOTLIB_EA_COMMAND,
	BOTLIB_EA_ACTION,
	BOTLIB_EA_GESTURE,
	BOTLIB_EA_TALK,
	BOTLIB_EA_ATTACK,
	BOTLIB_EA_ALT_ATTACK,
	BOTLIB_EA_FORCEPOWER,
	BOTLIB_EA_USE,
	BOTLIB_EA_RESPAWN,
	BOTLIB_EA_CROUCH,
	BOTLIB_EA_MOVE_UP,
	BOTLIB_EA_MOVE_DOWN,
	BOTLIB_EA_MOVE_FORWARD,
	BOTLIB_EA_MOVE_BACK,
	BOTLIB_EA_MOVE_LEFT,
	BOTLIB_EA_MOVE_RIGHT,
	BOTLIB_EA_SELECT_WEAPON,
	BOTLIB_EA_JUMP,
	BOTLIB_EA_DELAYED_JUMP,
	BOTLIB_EA_MOVE,
	BOTLIB_EA_VIEW,
	BOTLIB_EA_END_REGULAR,
	BOTLIB_EA_GET_INPUT,
	BOTLIB_EA_RESET_INPUT,

	BOTLIB_AI_LOAD_CHARACTER = 500,
	BOTLIB_AI_FREE_CHARACTER,
	BOTLIB_AI_CHARACTERISTIC_FLOAT,
	BOTLIB_AI_CHARACTERISTIC_BFLOAT,
	BOTLIB_AI_CHARACTERISTIC_INTEGER,
	BOTLIB_AI_CHARACTERISTIC_BINTEGER,
	BOTLIB_AI_CHARACTERISTIC_STRING,
	BOTLIB_AI_ALLOC_CHAT_STATE,
	BOTLIB_AI_FREE_CHAT_STATE,
	BOTLIB_AI_QUEUE_CONSOLE_MESSAGE,
	BOTLIB_AI_REMOVE_CONSOLE_MESSAGE,
	BOTLIB_AI_NEXT_CONSOLE_MESSAGE,
	BOTLIB_AI_NUM_CONSOLE_MESSAGE,
	BOTLIB_AI_INITIAL_CHAT,
	BOTLIB_AI_REPLY_CHAT,
	BOTLIB_AI_CHAT_LENGTH,
	BOTLIB_AI_ENTER_CHAT,
	BOTLIB_AI_STRING_CONTAINS,
	BOTLIB_AI_FIND_MATCH,
	BOTLIB_AI_MATCH_VARIABLE,
	BOTLIB_AI_UNIFY_WHITE_SPACES,
	BOTLIB_AI_REPLACE_SYNONYMS,
	BOTLIB_AI_LOAD_CHAT_FILE,
	BOTLIB_AI_SET_CHAT_GENDER,
	BOTLIB_AI_SET_CHAT_NAME,
	BOTLIB_AI_RESET_GOAL_STATE,
	BOTLIB_AI_RESET_AVOID_GOALS,
	BOTLIB_AI_PUSH_GOAL,
	BOTLIB_AI_POP_GOAL,
	BOTLIB_AI_EMPTY_GOAL_STACK,
	BOTLIB_AI_DUMP_AVOID_GOALS,
	BOTLIB_AI_DUMP_GOAL_STACK,
	BOTLIB_AI_GOAL_NAME,
	BOTLIB_AI_GET_TOP_GOAL,
	BOTLIB_AI_GET_SECOND_GOAL,
	BOTLIB_AI_CHOOSE_LTG_ITEM,
	BOTLIB_AI_CHOOSE_NBG_ITEM,
	BOTLIB_AI_TOUCHING_GOAL,
	BOTLIB_AI_ITEM_GOAL_IN_VIS_BUT_NOT_VISIBLE,
	BOTLIB_AI_GET_LEVEL_ITEM_GOAL,
	BOTLIB_AI_AVOID_GOAL_TIME,
	BOTLIB_AI_INIT_LEVEL_ITEMS,
	BOTLIB_AI_UPDATE_ENTITY_ITEMS,
	BOTLIB_AI_LOAD_ITEM_WEIGHTS,
	BOTLIB_AI_FREE_ITEM_WEIGHTS,
	BOTLIB_AI_SAVE_GOAL_FUZZY_LOGIC,
	BOTLIB_AI_ALLOC_GOAL_STATE,
	BOTLIB_AI_FREE_GOAL_STATE,
	BOTLIB_AI_RESET_MOVE_STATE,
	BOTLIB_AI_MOVE_TO_GOAL,
	BOTLIB_AI_MOVE_IN_DIRECTION,
	BOTLIB_AI_RESET_AVOID_REACH,
	BOTLIB_AI_RESET_LAST_AVOID_REACH,
	BOTLIB_AI_REACHABILITY_AREA,
	BOTLIB_AI_MOVEMENT_VIEW_TARGET,
	BOTLIB_AI_ALLOC_MOVE_STATE,
	BOTLIB_AI_FREE_MOVE_STATE,
	BOTLIB_AI_INIT_MOVE_STATE,
	BOTLIB_AI_CHOOSE_BEST_FIGHT_WEAPON,
	BOTLIB_AI_GET_WEAPON_INFO,
	BOTLIB_AI_LOAD_WEAPON_WEIGHTS,
	BOTLIB_AI_ALLOC_WEAPON_STATE,
	BOTLIB_AI_FREE_WEAPON_STATE,
	BOTLIB_AI_RESET_WEAPON_STATE,
	BOTLIB_AI_GENETIC_PARENTS_AND_CHILD_SELECTION,
	BOTLIB_AI_INTERBREED_GOAL_FUZZY_LOGIC,
	BOTLIB_AI_MUTATE_GOAL_FUZZY_LOGIC,
	BOTLIB_AI_GET_NEXT_CAMP_SPOT_GOAL,
	BOTLIB_AI_GET_MAP_LOCATION_GOAL,
	BOTLIB_AI_NUM_INITIAL_CHATS,
	BOTLIB_AI_GET_CHAT_MESSAGE,
	BOTLIB_AI_REMOVE_FROM_AVOID_GOALS,
	BOTLIB_AI_PREDICT_VISIBLE_POSITION,
	BOTLIB_AI_SET_AVOID_GOAL_TIME,
	BOTLIB_AI_ADD_AVOID_SPOT,
	BOTLIB_AAS_ALTERNATIVE_ROUTE_GOAL,
	BOTLIB_AAS_PREDICT_ROUTE,
	BOTLIB_AAS_POINT_REACHABILITY_AREA_INDEX,
	BOTLIB_PC_LOAD_SOURCE,
	BOTLIB_PC_FREE_SOURCE,
	BOTLIB_PC_READ_TOKEN,
	BOTLIB_PC_SOURCE_FILE_AND_LINE,

	G_R_REGISTERSKIN,
	G_G2_LISTBONES,
	G_G2_LISTSURFACES,
	G_G2_HAVEWEGHOULMODELS,
	G_G2_SETMODELS,
	G_G2_GETBOLT,
	G_G2_GETBOLT_NOREC,
	G_G2_GETBOLT_NOREC_NOROT,
	G_G2_INITGHOUL2MODEL,
	G_G2_SETSKIN,
	G_G2_SIZE,
	G_G2_ADDBOLT,
	G_G2_SETBOLTINFO,
	G_G2_ANGLEOVERRIDE,
	G_G2_PLAYANIM,
	G_G2_GETBONEANIM,
	G_G2_GETGLANAME,
	G_G2_COPYGHOUL2INSTANCE,
	G_G2_COPYSPECIFICGHOUL2MODEL,
	G_G2_DUPLICATEGHOUL2INSTANCE,
	G_G2_HASGHOUL2MODELONINDEX,
	G_G2_REMOVEGHOUL2MODEL,
	G_G2_REMOVEGHOUL2MODELS,
	G_G2_CLEANMODELS,
	G_G2_COLLISIONDETECT,
	G_G2_COLLISIONDETECTCACHE,
	G_G2_SETROOTSURFACE,
	G_G2_SETSURFACEONOFF,
	G_G2_SETNEWORIGIN,
	G_G2_DOESBONEEXIST,
	G_G2_GETSURFACERENDERSTATUS,
	G_G2_ABSURDSMOOTHING,
	G_G2_SETRAGDOLL,
	G_G2_ANIMATEG2MODELS,
	G_G2_RAGPCJCONSTRAINT,
	G_G2_RAGPCJGRADIENTSPEED,
	G_G2_RAGEFFECTORGOAL,
	G_G2_GETRAGBONEPOS,
	G_G2_RAGEFFECTORKICK,
	G_G2_RAGFORCESOLVE,
	G_G2_SETBONEIKSTATE,
	G_G2_IKMOVE,
	G_G2_REMOVEBONE,
	G_G2_ATTACHINSTANCETOENTNUM,
	G_G2_CLEARATTACHEDINSTANCE,
	G_G2_CLEANENTATTACHMENTS,
	G_G2_OVERRIDESERVER,
	G_G2_GETSURFACENAME,
	G_SET_ACTIVE_SUBBSP,
	G_CM_REGISTER_TERRAIN,
	G_RMG_INIT,
	G_BOT_UPDATEWAYPOINTS,
	G_BOT_CALCULATEPATHS
} gameImportLegacy_t;

typedef enum gameExportLegacy_e {
	GAME_INIT,
	GAME_SHUTDOWN,
	GAME_CLIENT_CONNECT,
	GAME_CLIENT_BEGIN,
	GAME_CLIENT_USERINFO_CHANGED,
	GAME_CLIENT_DISCONNECT,
	GAME_CLIENT_COMMAND,
	GAME_CLIENT_THINK,
	GAME_RUN_FRAME,
	GAME_CONSOLE_COMMAND,
	BOTAI_START_FRAME,
	GAME_ROFF_NOTETRACK_CALLBACK,
	GAME_SPAWN_RMG_ENTITY,
	GAME_ICARUS_PLAYSOUND,
	GAME_ICARUS_SET,
	GAME_ICARUS_LERP2POS,
	GAME_ICARUS_LERP2ORIGIN,
	GAME_ICARUS_LERP2ANGLES,
	GAME_ICARUS_GETTAG,
	GAME_ICARUS_LERP2START,
	GAME_ICARUS_LERP2END,
	GAME_ICARUS_USE,
	GAME_ICARUS_KILL,
	GAME_ICARUS_REMOVE,
	GAME_ICARUS_PLAY,
	GAME_ICARUS_GETFLOAT,
	GAME_ICARUS_GETVECTOR,
	GAME_ICARUS_GETSTRING,
	GAME_ICARUS_SOUNDINDEX,
	GAME_ICARUS_GETSETIDFORSTRING,
	GAME_NAV_CLEARPATHTOPOINT,
	GAME_NAV_CLEARLOS,
	GAME_NAV_CLEARPATHBETWEENPOINTS,
	GAME_NAV_CHECKNODEFAILEDFORENT,
	GAME_NAV_ENTISUNLOCKEDDOOR,
	GAME_NAV_ENTISDOOR,
	GAME_NAV_ENTISBREAKABLE,
	GAME_NAV_ENTISREMOVABLEUSABLE,
	GAME_NAV_FINDCOMBATPOINTWAYPOINTS,
	GAME_GETITEMINDEXBYTAG
} gameExportLegacy_t;

typedef struct gameImport_s {
	// misc
	void		(*Print)								( const char *msg, ... );
	NORETURN_PTR void (*Error)( int level, const char *fmt, ... );
	int			(*Milliseconds)							( void );
	void		(*PrecisionTimerStart)					( void **timer );
	int			(*PrecisionTimerEnd)					( void *timer );
	void		(*SV_RegisterSharedMemory)				( char *memory );
	int			(*RealTime)								( qtime_t *qtime );
	void		(*TrueMalloc)							( void **ptr, int size );
	void		(*TrueFree)								( void **ptr );
	void		(*SnapVector)							( float *v );

	// cvar
	void		(*Cvar_Register)						( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, uint32_t flags );
	void		(*Cvar_Set)								( const char *var_name, const char *value );
	void		(*Cvar_Update)							( vmCvar_t *vmCvar );
	int			(*Cvar_VariableIntegerValue)			( const char *var_name );
	void		(*Cvar_VariableStringBuffer)			( const char *var_name, char *buffer, int bufsize );

	// cmd
	int			(*Argc)									( void );
	void		(*Argv)									( int n, char *buffer, int bufferLength );

	// filesystem
	void		(*FS_Close)								( fileHandle_t f );
	int			(*FS_GetFileList)						( const char *path, const char *extension, char *listbuf, int bufsize );
	int			(*FS_Open)								( const char *qpath, fileHandle_t *f, fsMode_t mode );
	int			(*FS_Read)								( void *buffer, int len, fileHandle_t f );
	int			(*FS_Write)								( const void *buffer, int len, fileHandle_t f );

	// server
	void		(*AdjustAreaPortalState)				( sharedEntity_t *ent, qboolean open );
	qboolean	(*AreasConnected)						( int area1, int area2 );
	int			(*DebugPolygonCreate)					( int color, int numPoints, vec3_t *points );
	void		(*DebugPolygonDelete)					( int id );
	void		(*DropClient)							( int clientNum, const char *reason );
	int			(*EntitiesInBox)						( const vec3_t mins, const vec3_t maxs, int *list, int maxcount );
	qboolean	(*EntityContact)						( const vec3_t mins, const vec3_t maxs, const sharedEntity_t *ent, int capsule );
	void		(*GetConfigstring)						( int num, char *buffer, int bufferSize );
	qboolean	(*GetEntityToken)						( char *buffer, int bufferSize );
	void		(*GetServerinfo)						( char *buffer, int bufferSize );
	void		(*GetUsercmd)							( int clientNum, usercmd_t *cmd );
	void		(*GetUserinfo)							( int num, char *buffer, int bufferSize );
	qboolean	(*InPVS)								( const vec3_t p1, const vec3_t p2 );
	qboolean	(*InPVSIgnorePortals)					( const vec3_t p1, const vec3_t p2 );
	void		(*LinkEntity)							( sharedEntity_t *ent );
	void		(*LocateGameData)						( sharedEntity_t *gEnts, int numGEntities, int sizeofGEntity_t, playerState_t *clients, int sizeofGClient );
	int			(*PointContents)						( const vec3_t point, int passEntityNum );
	void		(*SendConsoleCommand)					( int exec_when, const char *text );
	void		(*SendServerCommand)					( int clientNum, const char *text );
	void		(*SetBrushModel)						( sharedEntity_t *ent, const char *name );
	void		(*SetConfigstring)						( int num, const char *string );
	void		(*SetServerCull)						( float cullDistance );
	void		(*SetUserinfo)							( int num, const char *buffer );
	void		(*SiegePersSet)							( siegePers_t *pers );
	void		(*SiegePersGet)							( siegePers_t *pers );
	void		(*Trace)								( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask, int capsule, int traceFlags, int useLod );
	void		(*UnlinkEntity)							( sharedEntity_t *ent );

	// ROFF
	qboolean	(*ROFF_Clean)							( void );
	void		(*ROFF_UpdateEntities)					( void );
	int			(*ROFF_Cache)							( char *file );
	qboolean	(*ROFF_Play)							( int entID, int roffID, qboolean doTranslation );
	qboolean	(*ROFF_Purge_Ent)						( int entID );

	// ICARUS
	int			(*ICARUS_RunScript)						( sharedEntity_t *ent, const char *name );
	qboolean	(*ICARUS_RegisterScript)				( const char *name, qboolean bCalledDuringInterrogate );
	void		(*ICARUS_Init)							( void );
	qboolean	(*ICARUS_ValidEnt)						( sharedEntity_t *ent );
	qboolean	(*ICARUS_IsInitialized)					( int entID );
	qboolean	(*ICARUS_MaintainTaskManager)			( int entID );
	qboolean	(*ICARUS_IsRunning)						( int entID );
	qboolean	(*ICARUS_TaskIDPending)					( sharedEntity_t *ent, int taskID );
	void		(*ICARUS_InitEnt)						( sharedEntity_t *ent );
	void		(*ICARUS_FreeEnt)						( sharedEntity_t *ent );
	void		(*ICARUS_AssociateEnt)					( sharedEntity_t *ent );
	void		(*ICARUS_Shutdown)						( void );
	void		(*ICARUS_TaskIDSet)						( sharedEntity_t *ent, int taskType, int taskID );
	void		(*ICARUS_TaskIDComplete)				( sharedEntity_t *ent, int taskType );
	void		(*ICARUS_SetVar)						( int taskID, int entID, const char *type_name, const char *data );
	int			(*ICARUS_VariableDeclared)				( const char *type_name );
	int			(*ICARUS_GetFloatVariable)				( const char *name, float *value );
	int			(*ICARUS_GetStringVariable)				( const char *name, const char *value );
	int			(*ICARUS_GetVectorVariable)				( const char *name, const vec3_t value );

	// navigation
	void		(*Nav_Init)								( void );
	void		(*Nav_Free)								( void );
	qboolean	(*Nav_Load)								( const char *filename, int checksum );
	qboolean	(*Nav_Save)								( const char *filename, int checksum );
	int			(*Nav_AddRawPoint)						( vec3_t point, int flags, int radius );
	void		(*Nav_CalculatePaths)					( qboolean recalc );
	void		(*Nav_HardConnect)						( int first, int second );
	void		(*Nav_ShowNodes)						( void );
	void		(*Nav_ShowEdges)						( void );
	void		(*Nav_ShowPath)							( int start, int end );
	int			(*Nav_GetNearestNode)					( sharedEntity_t *ent, int lastID, int flags, int targetID );
	int			(*Nav_GetBestNode)						( int startID, int endID, int rejectID );
	int			(*Nav_GetNodePosition)					( int nodeID, vec3_t out );
	int			(*Nav_GetNodeNumEdges)					( int nodeID );
	int			(*Nav_GetNodeEdge)						( int nodeID, int edge );
	int			(*Nav_GetNumNodes)						( void );
	qboolean	(*Nav_Connected)						( int startID, int endID );
	int			(*Nav_GetPathCost)						( int startID, int endID );
	int			(*Nav_GetEdgeCost)						( int startID, int endID );
	int			(*Nav_GetProjectedNode)					( vec3_t origin, int nodeID );
	void		(*Nav_CheckFailedNodes)					( sharedEntity_t *ent );
	void		(*Nav_AddFailedNode)					( sharedEntity_t *ent, int nodeID );
	qboolean	(*Nav_NodeFailed)						( sharedEntity_t *ent, int nodeID );
	qboolean	(*Nav_NodesAreNeighbors)				( int startID, int endID );
	void		(*Nav_ClearFailedEdge)					( failedEdge_t *failedEdge );
	void		(*Nav_ClearAllFailedEdges)				( void );
	int			(*Nav_EdgeFailed)						( int startID, int endID );
	void		(*Nav_AddFailedEdge)					( int entID, int startID, int endID );
	qboolean	(*Nav_CheckFailedEdge)					( failedEdge_t *failedEdge );
	void		(*Nav_CheckAllFailedEdges)				( void );
	qboolean	(*Nav_RouteBlocked)						( int startID, int testEdgeID, int endID, int rejectRank );
	int			(*Nav_GetBestNodeAltRoute)				( int startID, int endID, int *pathCost, int rejectID );
	int			(*Nav_GetBestNodeAltRoute2)				( int startID, int endID, int rejectID );
	int			(*Nav_GetBestPathBetweenEnts)			( sharedEntity_t *ent, sharedEntity_t *goal, int flags );
	int			(*Nav_GetNodeRadius)					( int nodeID );
	void		(*Nav_CheckBlockedEdges)				( void );
	void		(*Nav_ClearCheckedNodes)				( void );
	int			(*Nav_CheckedNode)						( int wayPoint, int ent );
	void		(*Nav_SetCheckedNode)					( int wayPoint, int ent, int value );
	void		(*Nav_FlagAllNodes)						( int newFlag );
	qboolean	(*Nav_GetPathsCalculated)				( void );
	void		(*Nav_SetPathsCalculated)				( qboolean newVal );

	// botlib
	int			(*BotAllocateClient)					( void );
	void		(*BotFreeClient)						( int clientNum );
	int			(*BotLoadCharacter)						( char *charfile, float skill );
	void		(*BotFreeCharacter)						( int character );
	float		(*Characteristic_Float)					( int character, int index );
	float		(*Characteristic_BFloat)				( int character, int index, float min, float max );
	int			(*Characteristic_Integer)				( int character, int index );
	int			(*Characteristic_BInteger)				( int character, int index, int min, int max );
	void		(*Characteristic_String)				( int character, int index, char *buf, int size );
	int			(*BotAllocChatState)					( void );
	void		(*BotFreeChatState)						( int handle );
	void		(*BotQueueConsoleMessage)				( int chatstate, int type, char *message );
	void		(*BotRemoveConsoleMessage)				( int chatstate, int handle );
	int			(*BotNextConsoleMessage)				( int chatstate, void *cm );
	int			(*BotNumConsoleMessages)				( int chatstate );
	void		(*BotInitialChat)						( int chatstate, char *type, int mcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7 );
	int			(*BotReplyChat)							( int chatstate, char *message, int mcontext, int vcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7 );
	int			(*BotChatLength)						( int chatstate );
	void		(*BotEnterChat)							( int chatstate, int client, int sendto );
	int			(*StringContains)						( char *str1, char *str2, int casesensitive );
	int			(*BotFindMatch)							( char *str, void *match, unsigned long int context );
	void		(*BotMatchVariable)						( void *match, int variable, char *buf, int size );
	void		(*UnifyWhiteSpaces)						( char *string );
	void		(*BotReplaceSynonyms)					( char *string, unsigned long int context );
	int			(*BotLoadChatFile)						( int chatstate, char *chatfile, char *chatname );
	void		(*BotSetChatGender)						( int chatstate, int gender );
	void		(*BotSetChatName)						( int chatstate, char *name, int client );
	void		(*BotResetGoalState)					( int goalstate );
	void		(*BotResetAvoidGoals)					( int goalstate );
	void		(*BotPushGoal)							( int goalstate, void *goal );
	void		(*BotPopGoal)							( int goalstate );
	void		(*BotEmptyGoalStack)					( int goalstate );
	void		(*BotDumpAvoidGoals)					( int goalstate );
	void		(*BotDumpGoalStack)						( int goalstate );
	void		(*BotGoalName)							( int number, char *name, int size );
	int			(*BotGetTopGoal)						( int goalstate, void *goal );
	int			(*BotGetSecondGoal)						( int goalstate, void *goal );
	int			(*BotChooseLTGItem)						( int goalstate, vec3_t origin, int *inventory, int travelflags );
	int			(*BotChooseNBGItem)						( int goalstate, vec3_t origin, int *inventory, int travelflags, void *ltg, float maxtime );
	int			(*BotTouchingGoal)						( vec3_t origin, void *goal );
	int			(*BotItemGoalInVisButNotVisible)		( int viewer, vec3_t eye, vec3_t viewangles, void *goal );
	int			(*BotGetLevelItemGoal)					( int index, char *classname, void *goal );
	float		(*BotAvoidGoalTime)						( int goalstate, int number );
	void		(*BotInitLevelItems)					( void );
	void		(*BotUpdateEntityItems)					( void );
	int			(*BotLoadItemWeights)					( int goalstate, char *filename );
	void		(*BotFreeItemWeights)					( int goalstate );
	void		(*BotSaveGoalFuzzyLogic)				( int goalstate, char *filename );
	int			(*BotAllocGoalState)					( int state );
	void		(*BotFreeGoalState)						( int handle );
	void		(*BotResetMoveState)					( int movestate );
	void		(*BotMoveToGoal)						( void *result, int movestate, void *goal, int travelflags );
	int			(*BotMoveInDirection)					( int movestate, vec3_t dir, float speed, int type );
	void		(*BotResetAvoidReach)					( int movestate );
	void		(*BotResetLastAvoidReach)				( int movestate );
	int			(*BotReachabilityArea)					( vec3_t origin, int testground );
	int			(*BotMovementViewTarget)				( int movestate, void *goal, int travelflags, float lookahead, vec3_t target );
	int			(*BotAllocMoveState)					( void );
	void		(*BotFreeMoveState)						( int handle );
	void		(*BotInitMoveState)						( int handle, void *initmove );
	int			(*BotChooseBestFightWeapon)				( int weaponstate, int *inventory );
	void		(*BotGetWeaponInfo)						( int weaponstate, int weapon, void *weaponinfo );
	int			(*BotLoadWeaponWeights)					( int weaponstate, char *filename );
	int			(*BotAllocWeaponState)					( void );
	void		(*BotFreeWeaponState)					( int weaponstate );
	void		(*BotResetWeaponState)					( int weaponstate );
	int			(*GeneticParentsAndChildSelection)		( int numranks, float *ranks, int *parent1, int *parent2, int *child );
	void		(*BotInterbreedGoalFuzzyLogic)			( int parent1, int parent2, int child );
	void		(*BotMutateGoalFuzzyLogic)				( int goalstate, float range );
	int			(*BotGetNextCampSpotGoal)				( int num, void *goal );
	int			(*BotGetMapLocationGoal)				( char *name, void *goal );
	int			(*BotNumInitialChats)					( int chatstate, char *type );
	void		(*BotGetChatMessage)					( int chatstate, char *buf, int size );
	void		(*BotRemoveFromAvoidGoals)				( int goalstate, int number );
	int			(*BotPredictVisiblePosition)			( vec3_t origin, int areanum, void *goal, int travelflags, vec3_t target );
	void		(*BotSetAvoidGoalTime)					( int goalstate, int number, float avoidtime );
	void		(*BotAddAvoidSpot)						( int movestate, vec3_t origin, float radius, int type );
	int			(*BotLibSetup)							( void );
	int			(*BotLibShutdown)						( void );
	int			(*BotLibVarSet)							( char *var_name, char *value );
	int			(*BotLibVarGet)							( char *var_name, char *value, int size );
	int			(*BotLibDefine)							( char *string );
	int			(*BotLibStartFrame)						( float time );
	int			(*BotLibLoadMap)						( const char *mapname );
	int			(*BotLibUpdateEntity)					( int ent, void *bue );
	int			(*BotLibTest)							( int parm0, char *parm1, vec3_t parm2, vec3_t parm3 );
	int			(*BotGetSnapshotEntity)					( int clientNum, int sequence );
	int			(*BotGetServerCommand)					( int clientNum, char *message, int size );
	void		(*BotUserCommand)						( int clientNum, usercmd_t *ucmd );
	void		(*BotUpdateWaypoints)					( int wpnum, wpobject_t **wps );
	void		(*BotCalculatePaths)					( int rmg );

	// area awareness system
	int			(*AAS_EnableRoutingArea)				( int areanum, int enable );
	int			(*AAS_BBoxAreas)						( vec3_t absmins, vec3_t absmaxs, int *areas, int maxareas );
	int			(*AAS_AreaInfo)							( int areanum, void *info );
	void		(*AAS_EntityInfo)						( int entnum, void *info );
	int			(*AAS_Initialized)						( void );
	void		(*AAS_PresenceTypeBoundingBox)			( int presencetype, vec3_t mins, vec3_t maxs );
	float		(*AAS_Time)								( void );
	int			(*AAS_PointAreaNum)						( vec3_t point );
	int			(*AAS_TraceAreas)						( vec3_t start, vec3_t end, int *areas, vec3_t *points, int maxareas );
	int			(*AAS_PointContents)					( vec3_t point );
	int			(*AAS_NextBSPEntity)					( int ent );
	int			(*AAS_ValueForBSPEpairKey)				( int ent, char *key, char *value, int size );
	int			(*AAS_VectorForBSPEpairKey)				( int ent, char *key, vec3_t v );
	int			(*AAS_FloatForBSPEpairKey)				( int ent, char *key, float *value );
	int			(*AAS_IntForBSPEpairKey)				( int ent, char *key, int *value );
	int			(*AAS_AreaReachability)					( int areanum );
	int			(*AAS_AreaTravelTimeToGoalArea)			( int areanum, vec3_t origin, int goalareanum, int travelflags );
	int			(*AAS_Swimming)							( vec3_t origin );
	int			(*AAS_PredictClientMovement)			( void *move, int entnum, vec3_t origin, int presencetype, int onground, vec3_t velocity, vec3_t cmdmove, int cmdframes, int maxframes, float frametime, int stopevent, int stopareanum, int visualize );
	int			(*AAS_AlternativeRouteGoals)			( vec3_t start, int startareanum, vec3_t goal, int goalareanum, int travelflags, void *altroutegoals, int maxaltroutegoals, int type );
	int			(*AAS_PredictRoute)						( void *route, int areanum, vec3_t origin, int goalareanum, int travelflags, int maxareas, int maxtime, int stopevent, int stopcontents, int stoptfl, int stopareanum );
	int			(*AAS_PointReachabilityAreaIndex)		( vec3_t point );

	// elementary action
	void		(*EA_Say)								( int client, char *str );
	void		(*EA_SayTeam)							( int client, char *str );
	void		(*EA_Command)							( int client, char *command );
	void		(*EA_Action)							( int client, int action );
	void		(*EA_Gesture)							( int client );
	void		(*EA_Talk)								( int client );
	void		(*EA_Attack)							( int client );
	void		(*EA_Alt_Attack)						( int client );
	void		(*EA_ForcePower)						( int client );
	void		(*EA_Use)								( int client );
	void		(*EA_Respawn)							( int client );
	void		(*EA_Crouch)							( int client );
	void		(*EA_MoveUp)							( int client );
	void		(*EA_MoveDown)							( int client );
	void		(*EA_MoveForward)						( int client );
	void		(*EA_MoveBack)							( int client );
	void		(*EA_MoveLeft)							( int client );
	void		(*EA_MoveRight)							( int client );
	void		(*EA_SelectWeapon)						( int client, int weapon );
	void		(*EA_Jump)								( int client );
	void		(*EA_DelayedJump)						( int client );
	void		(*EA_Move)								( int client, vec3_t dir, float speed );
	void		(*EA_View)								( int client, vec3_t viewangles );
	void		(*EA_EndRegular)						( int client, float thinktime );
	void		(*EA_GetInput)							( int client, float thinktime, void *input );
	void		(*EA_ResetInput)						( int client );

	// botlib preprocessor
	int			(*PC_LoadSource)						( const char *filename );
	int			(*PC_FreeSource)						( int handle );
	int			(*PC_ReadToken)							( int handle, pc_token_t *pc_token );
	int			(*PC_SourceFileAndLine)					( int handle, char *filename, int *line );

	// renderer, terrain
	qhandle_t	(*R_RegisterSkin)						( const char *name );
	const char *(*SetActiveSubBSP)						( int index );
	int			(*CM_RegisterTerrain)					( const char *config );
	void		(*RMG_Init)								( void );

	void		(*G2API_ListModelBones)					( void *ghlInfo, int frame );
	void		(*G2API_ListModelSurfaces)				( void *ghlInfo );
	qboolean	(*G2API_HaveWeGhoul2Models)				( void *ghoul2 );
	void		(*G2API_SetGhoul2ModelIndexes)			( void *ghoul2, qhandle_t *modelList, qhandle_t *skinList );
	qboolean	(*G2API_GetBoltMatrix)					( void *ghoul2, const int modelIndex, const int boltIndex, mdxaBone_t *matrix, const vec3_t angles, const vec3_t position, const int frameNum, qhandle_t *modelList, vec3_t scale );
	qboolean	(*G2API_GetBoltMatrix_NoReconstruct)	( void *ghoul2, const int modelIndex, const int boltIndex, mdxaBone_t *matrix, const vec3_t angles, const vec3_t position, const int frameNum, qhandle_t *modelList, vec3_t scale );
	qboolean	(*G2API_GetBoltMatrix_NoRecNoRot)		( void *ghoul2, const int modelIndex, const int boltIndex, mdxaBone_t *matrix, const vec3_t angles, const vec3_t position, const int frameNum, qhandle_t *modelList, vec3_t scale );
	int			(*G2API_InitGhoul2Model)				( void **ghoul2Ptr, const char *fileName, int modelIndex, qhandle_t customSkin, qhandle_t customShader, int modelFlags, int lodBias );
	qboolean	(*G2API_SetSkin)						( void *ghoul2, int modelIndex, qhandle_t customSkin, qhandle_t renderSkin );
	int			(*G2API_Ghoul2Size)						( void *ghlInfo );
	int			(*G2API_AddBolt)						( void *ghoul2, int modelIndex, const char *boneName );
	void		(*G2API_SetBoltInfo)					( void *ghoul2, int modelIndex, int boltInfo );
	qboolean	(*G2API_SetBoneAngles)					( void *ghoul2, int modelIndex, const char *boneName, const vec3_t angles, const int flags, const int up, const int right, const int forward, qhandle_t *modelList, int blendTime , int currentTime );
	qboolean	(*G2API_SetBoneAnim)					( void *ghoul2, const int modelIndex, const char *boneName, const int startFrame, const int endFrame, const int flags, const float animSpeed, const int currentTime, const float setFrame, const int blendTime );
	qboolean	(*G2API_GetBoneAnim)					( void *ghoul2, const char *boneName, const int currentTime, float *currentFrame, int *startFrame, int *endFrame, int *flags, float *animSpeed, int *modelList, const int modelIndex );
	void		(*G2API_GetGLAName)						( void *ghoul2, int modelIndex, char *fillBuf );
	int			(*G2API_CopyGhoul2Instance)				( void *g2From, void *g2To, int modelIndex );
	void		(*G2API_CopySpecificGhoul2Model)		( void *g2From, int modelFrom, void *g2To, int modelTo );
	void		(*G2API_DuplicateGhoul2Instance)		( void *g2From, void **g2To );
	qboolean	(*G2API_HasGhoul2ModelOnIndex)			( void *ghlInfo, int modelIndex );
	qboolean	(*G2API_RemoveGhoul2Model)				( void *ghlInfo, int modelIndex );
	qboolean	(*G2API_RemoveGhoul2Models)				( void *ghlInfo );
	void		(*G2API_CleanGhoul2Models)				( void **ghoul2Ptr );
	void		(*G2API_CollisionDetect)				( CollisionRecord_t *collRecMap, void *ghoul2, const vec3_t angles, const vec3_t position, int frameNumber, int entNum, vec3_t rayStart, vec3_t rayEnd, vec3_t scale, int traceFlags, int useLod, float fRadius );
	void		(*G2API_CollisionDetectCache)			( CollisionRecord_t *collRecMap, void* ghoul2, const vec3_t angles, const vec3_t position, int frameNumber, int entNum, vec3_t rayStart, vec3_t rayEnd, vec3_t scale, int traceFlags, int useLod, float fRadius );
	qboolean	(*G2API_SetRootSurface)					( void *ghoul2, const int modelIndex, const char *surfaceName );
	qboolean	(*G2API_SetSurfaceOnOff)				( void *ghoul2, const char *surfaceName, const int flags );
	qboolean	(*G2API_SetNewOrigin)					( void *ghoul2, const int boltIndex );
	qboolean	(*G2API_DoesBoneExist)					( void *ghoul2, int modelIndex, const char *boneName );
	int			(*G2API_GetSurfaceRenderStatus)			( void *ghoul2, const int modelIndex, const char *surfaceName );
	void		(*G2API_AbsurdSmoothing)				( void *ghoul2, qboolean status );
	void		(*G2API_SetRagDoll)						( void *ghoul2, sharedRagDollParams_t *params );
	void		(*G2API_AnimateG2Models)				( void *ghoul2, int time, sharedRagDollUpdateParams_t *params );
	qboolean	(*G2API_RagPCJConstraint)				( void *ghoul2, const char *boneName, vec3_t min, vec3_t max );
	qboolean	(*G2API_RagPCJGradientSpeed)			( void *ghoul2, const char *boneName, const float speed );
	qboolean	(*G2API_RagEffectorGoal)				( void *ghoul2, const char *boneName, vec3_t pos );
	qboolean	(*G2API_GetRagBonePos)					( void *ghoul2, const char *boneName, vec3_t pos, vec3_t entAngles, vec3_t entPos, vec3_t entScale );
	qboolean	(*G2API_RagEffectorKick)				( void *ghoul2, const char *boneName, vec3_t velocity );
	qboolean	(*G2API_RagForceSolve)					( void *ghoul2, qboolean force );
	qboolean	(*G2API_SetBoneIKState)					( void *ghoul2, int time, const char *boneName, int ikState, sharedSetBoneIKStateParams_t *params );
	qboolean	(*G2API_IKMove)							( void *ghoul2, int time, sharedIKMoveParams_t *params );
	qboolean	(*G2API_RemoveBone)						( void *ghoul2, const char *boneName, int modelIndex );
	void		(*G2API_AttachInstanceToEntNum)			( void *ghoul2, int entityNum, qboolean server );
	void		(*G2API_ClearAttachedInstance)			( int entityNum );
	void		(*G2API_CleanEntAttachments)			( void );
	qboolean	(*G2API_OverrideServer)					( void *serverInstance );
	void		(*G2API_GetSurfaceName)					( void *ghoul2, int surfNumber, int modelIndex, char *fillBuf );
} gameImport_t;

typedef struct gameExport_s {
	void		(*InitGame)							( int levelTime, int randomSeed, int restart );
	void		(*ShutdownGame)						( int restart );
	char *		(*ClientConnect)					( int clientNum, qboolean firstTime, qboolean isBot );
	void		(*ClientBegin)						( int clientNum, qboolean allowTeamReset );
	qboolean	(*ClientUserinfoChanged)			( int clientNum );
	void		(*ClientDisconnect)					( int clientNum );
	void		(*ClientCommand)					( int clientNum );
	void		(*ClientThink)						( int clientNum, usercmd_t *ucmd );
	void		(*RunFrame)							( int levelTime );
	qboolean	(*ConsoleCommand)					( void );
	int			(*BotAIStartFrame)					( int time );
	void		(*ROFF_NotetrackCallback)			( int entID, const char *notetrack );
	void		(*SpawnRMGEntity)					( void );
	int			(*ICARUS_PlaySound)					( void );
	qboolean	(*ICARUS_Set)						( void );
	void		(*ICARUS_Lerp2Pos)					( void );
	void		(*ICARUS_Lerp2Origin)				( void );
	void		(*ICARUS_Lerp2Angles)				( void );
	int			(*ICARUS_GetTag)					( void );
	void		(*ICARUS_Lerp2Start)				( void );
	void		(*ICARUS_Lerp2End)					( void );
	void		(*ICARUS_Use)						( void );
	void		(*ICARUS_Kill)						( void );
	void		(*ICARUS_Remove)					( void );
	void		(*ICARUS_Play)						( void );
	int			(*ICARUS_GetFloat)					( void );
	int			(*ICARUS_GetVector)					( void );
	int			(*ICARUS_GetString)					( void );
	void		(*ICARUS_SoundIndex)				( void );
	int			(*ICARUS_GetSetIDForString)			( void );
	qboolean	(*NAV_ClearPathToPoint)				( int entID, vec3_t pmins, vec3_t pmaxs, vec3_t point, int clipmask, int okToHitEnt );
	qboolean	(*NPC_ClearLOS2)					( int entID, const vec3_t end );
	int			(*NAVNEW_ClearPathBetweenPoints)	( vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, int ignore, int clipmask );
	qboolean	(*NAV_CheckNodeFailedForEnt)		( int entID, int nodeNum );
	qboolean	(*NAV_EntIsUnlockedDoor)			( int entityNum );
	qboolean	(*NAV_EntIsDoor)					( int entityNum );
	qboolean	(*NAV_EntIsBreakable)				( int entityNum );
	qboolean	(*NAV_EntIsRemovableUsable)			( int entNum );
	void		(*NAV_FindCombatPointWaypoints)		( void );
	int			(*BG_GetItemIndexByTag)				( int tag, int type );
} gameExport_t;


//linking of game library
typedef gameExport_t* (QDECL *GetGameAPI_t)( int apiVersion, gameImport_t *import );
