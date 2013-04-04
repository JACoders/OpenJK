// Copyright (C) 1999-2000 Id Software, Inc.
//
#ifndef G_PUBLIC_H

// g_public.h -- game module information visible to server

#define G_PUBLIC_H

#define Q3_INFINITE			16777216 

#define	GAME_API_VERSION	8

// entity->svFlags
// the server does not know how to interpret most of the values
// in entityStates (level eType), so the game must explicitly flag
// special server behaviors
#define	SVF_NOCLIENT			0x00000001	// don't send entity to clients, even if it has effects
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

typedef struct {
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

	// mask of clients that this entity should be broadcast too.  The first 32 clients
	// are represented by the first array index and the latter 32 clients are represented
	// by the second array index.
	int			broadcastClients[2];

} entityShared_t;

//===============================================================

//
// system traps provided by the main engine
//
typedef enum {
	//============== general Quake services ==================

	G_PRINT,		// ( const char *string );
	// print message on the local console

	G_ERROR,		// ( const char *string );
	// abort the game

	G_MILLISECONDS,	// ( void );
	// get current time for profiling reasons
	// this should NOT be used for any game related tasks,
	// because it is not journaled

	//Also for profiling.. do not use for game related tasks.
	G_PRECISIONTIMER_START,
	G_PRECISIONTIMER_END,

	// console variable interaction
	G_CVAR_REGISTER,	// ( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags );
	G_CVAR_UPDATE,	// ( vmCvar_t *vmCvar );
	G_CVAR_SET,		// ( const char *var_name, const char *value );
	G_CVAR_VARIABLE_INTEGER_VALUE,	// ( const char *var_name );

	G_CVAR_VARIABLE_STRING_BUFFER,	// ( const char *var_name, char *buffer, int bufsize );

	G_ARGC,			// ( void );
	// ClientCommand and ServerCommand parameter access

	G_ARGV,			// ( int n, char *buffer, int bufferLength );

	G_FS_FOPEN_FILE,	// ( const char *qpath, fileHandle_t *file, fsMode_t mode );
	G_FS_READ,		// ( void *buffer, int len, fileHandle_t f );
	G_FS_WRITE,		// ( const void *buffer, int len, fileHandle_t f );
	G_FS_FCLOSE_FILE,		// ( fileHandle_t f );

	G_SEND_CONSOLE_COMMAND,	// ( const char *text );
	// add commands to the console as if they were typed in
	// for map changing, etc


	//=========== server specific functionality =============

	G_LOCATE_GAME_DATA,		// ( gentity_t *gEnts, int numGEntities, int sizeofGEntity_t,
	//							playerState_t *clients, int sizeofGameClient );
	// the game needs to let the server system know where and how big the gentities
	// are, so it can look at them directly without going through an interface

	G_DROP_CLIENT,		// ( int clientNum, const char *reason );
	// kick a client off the server with a message

	G_SEND_SERVER_COMMAND,	// ( int clientNum, const char *fmt, ... );
	// reliably sends a command string to be interpreted by the given
	// client.  If clientNum is -1, it will be sent to all clients

	G_SET_CONFIGSTRING,	// ( int num, const char *string );
	// config strings hold all the index strings, and various other information
	// that is reliably communicated to all clients
	// All of the current configstrings are sent to clients when
	// they connect, and changes are sent to all connected clients.
	// All confgstrings are cleared at each level start.

	G_GET_CONFIGSTRING,	// ( int num, char *buffer, int bufferSize );

	G_GET_USERINFO,		// ( int num, char *buffer, int bufferSize );
	// userinfo strings are maintained by the server system, so they
	// are persistant across level loads, while all other game visible
	// data is completely reset

	G_SET_USERINFO,		// ( int num, const char *buffer );

	G_GET_SERVERINFO,	// ( char *buffer, int bufferSize );
	// the serverinfo info string has all the cvars visible to server browsers

	G_SET_SERVER_CULL,
	//server culling to reduce traffic on open maps -rww

	G_SET_BRUSH_MODEL,	// ( gentity_t *ent, const char *name );
	// sets mins and maxs based on the brushmodel name

	G_TRACE,	// ( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
	// collision detection against all linked entities

	G_G2TRACE,	// ( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
	// collision detection against all linked entities with ghoul2 check

	G_POINT_CONTENTS,	// ( const vec3_t point, int passEntityNum );
	// point contents against all linked entities

	G_IN_PVS,			// ( const vec3_t p1, const vec3_t p2 );

	G_IN_PVS_IGNORE_PORTALS,	// ( const vec3_t p1, const vec3_t p2 );

	G_ADJUST_AREA_PORTAL_STATE,	// ( gentity_t *ent, qboolean open );

	G_AREAS_CONNECTED,	// ( int area1, int area2 );

	G_LINKENTITY,		// ( gentity_t *ent );
	// an entity will never be sent to a client or used for collision
	// if it is not passed to linkentity.  If the size, position, or
	// solidity changes, it must be relinked.

	G_UNLINKENTITY,		// ( gentity_t *ent );		
	// call before removing an interactive entity

	G_ENTITIES_IN_BOX,	// ( const vec3_t mins, const vec3_t maxs, gentity_t **list, int maxcount );
	// EntitiesInBox will return brush models based on their bounding box,
	// so exact determination must still be done with EntityContact

	G_ENTITY_CONTACT,	// ( const vec3_t mins, const vec3_t maxs, const gentity_t *ent );
	// perform an exact check against inline brush models of non-square shape

	// access for bots to get and free a server client (FIXME?)
	G_BOT_ALLOCATE_CLIENT,	// ( void );

	G_BOT_FREE_CLIENT,	// ( int clientNum );

	G_GET_USERCMD,	// ( int clientNum, usercmd_t *cmd )

	G_GET_ENTITY_TOKEN,	// qboolean ( char *buffer, int bufferSize )
	// Retrieves the next string token from the entity spawn text, returning
	// false when all tokens have been parsed.
	// This should only be done at GAME_INIT time.

	G_SIEGEPERSSET,
	G_SIEGEPERSGET,

	G_FS_GETFILELIST,
	G_DEBUG_POLYGON_CREATE,
	G_DEBUG_POLYGON_DELETE,
	G_REAL_TIME,
	G_SNAPVECTOR,

	G_TRACECAPSULE,	// ( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
	G_ENTITY_CONTACTCAPSULE,	// ( const vec3_t mins, const vec3_t maxs, const gentity_t *ent );

//	SP_REGISTER_SERVER_CMD,
	SP_GETSTRINGTEXTSTRING,

	G_ROFF_CLEAN,				// qboolean	ROFF_Clean(void);
	G_ROFF_UPDATE_ENTITIES,		// void		ROFF_UpdateEntities(void);
	G_ROFF_CACHE,				// int		ROFF_Cache(char *file);
	G_ROFF_PLAY,				// qboolean	ROFF_Play(int entID, int roffID, qboolean doTranslation);
	G_ROFF_PURGE_ENT,			// qboolean ROFF_PurgeEnt( int entID )

	//rww - dynamic vm memory allocation!
	G_TRUEMALLOC,
	G_TRUEFREE,

	//rww - icarus traps
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

	//BEGIN VM STUFF
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

	//END VM STUFF

	//rww - BEGIN NPC NAV TRAPS
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
	//rww - END NPC NAV TRAPS

	BOTLIB_SETUP = 250,				// ( void );
	BOTLIB_SHUTDOWN,				// ( void );
	BOTLIB_LIBVAR_SET,
	BOTLIB_LIBVAR_GET,
	BOTLIB_PC_ADD_GLOBAL_DEFINE,
	BOTLIB_START_FRAME,
	BOTLIB_LOAD_MAP,
	BOTLIB_UPDATENTITY,
	BOTLIB_TEST,

	BOTLIB_GET_SNAPSHOT_ENTITY,		// ( int client, int ent );
	BOTLIB_GET_CONSOLE_MESSAGE,		// ( int client, char *message, int size );
	BOTLIB_USER_COMMAND,			// ( int client, usercmd_t *ucmd );

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

	/*
Ghoul2 Insert Start
*/
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

/*
	//rww - RAGDOLL_BEGIN
*/
	G_G2_SETRAGDOLL,
	G_G2_ANIMATEG2MODELS,
/*
	//rww - RAGDOLL_END
*/
	//additional ragdoll options -rww
	G_G2_RAGPCJCONSTRAINT,
	G_G2_RAGPCJGRADIENTSPEED,
	G_G2_RAGEFFECTORGOAL,
	G_G2_GETRAGBONEPOS,
	G_G2_RAGEFFECTORKICK,
	G_G2_RAGFORCESOLVE,

	//rww - ik move method, allows you to specify a bone and move it to a world point (within joint constraints)
	//by using the majority of gil's existing bone angling stuff from the ragdoll code.
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
/*
Ghoul2 Insert End
*/

} gameImport_t;

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
typedef struct
{	
	char	parm[MAX_PARMS][MAX_PARM_STRING_LENGTH];
} parms_t;

#define MAX_FAILED_NODES 8

typedef struct Vehicle_s Vehicle_t;

// the server looks at a sharedEntity, which is the start of the game's gentity_t structure
//mod authors should not touch this struct
typedef struct {
	entityState_t	s;				// communicated by server to clients
	playerState_t	*playerState;	//needs to be in the gentity for bg entity access
									//if you want to actually see the contents I guess
									//you will have to be sure to VMA it first.
	Vehicle_t		*m_pVehicle; //vehicle data
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

#ifdef __cplusplus
class CSequencer;
class CTaskManager;

//I suppose this could be in another in-engine header or something. But we never want to
//include an icarus file before sharedentity_t is declared.
extern CSequencer	*gSequencers[MAX_GENTITIES];
extern CTaskManager	*gTaskManagers[MAX_GENTITIES];

#include "../icarus/icarus.h"
#include "../icarus/sequencer.h"
#include "../icarus/taskmanager.h"
#endif

//
// functions exported by the game subsystem
//
typedef enum {
	GAME_INIT,	// ( int levelTime, int randomSeed, int restart );
	// init and shutdown will be called every single level
	// The game should call G_GET_ENTITY_TOKEN to parse through all the
	// entity configuration text and spawn gentities.

	GAME_SHUTDOWN,	// (void);

	GAME_CLIENT_CONNECT,	// ( int clientNum, qboolean firstTime, qboolean isBot );
	// return NULL if the client is allowed to connect, otherwise return
	// a text string with the reason for denial

	GAME_CLIENT_BEGIN,				// ( int clientNum );

	GAME_CLIENT_USERINFO_CHANGED,	// ( int clientNum );

	GAME_CLIENT_DISCONNECT,			// ( int clientNum );

	GAME_CLIENT_COMMAND,			// ( int clientNum );

	GAME_CLIENT_THINK,				// ( int clientNum );

	GAME_RUN_FRAME,					// ( int levelTime );

	GAME_CONSOLE_COMMAND,			// ( void );
	// ConsoleCommand will be called when a command has been issued
	// that is not recognized as a builtin function.
	// The game can issue trap_argc() / trap_argv() commands to get the command
	// and parameters.  Return qfalse if the game doesn't recognize it as a command.

	BOTAI_START_FRAME,				// ( int time );

	GAME_ROFF_NOTETRACK_CALLBACK,	// int entnum, char *notetrack

	GAME_SPAWN_RMG_ENTITY, //rwwRMG - added

	//rww - icarus callbacks
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
} gameExport_t;

typedef struct
{
	int taskID;
	int entID;
	char name[2048];
	char channel[2048];
} T_G_ICARUS_PLAYSOUND;


typedef struct
{
	int taskID;
	int entID;
	char type_name[2048];
	char data[2048];
} T_G_ICARUS_SET;

typedef struct
{
	int taskID;
	int entID; 
	vec3_t origin;
	vec3_t angles;
	float duration;
	qboolean nullAngles; //special case
} T_G_ICARUS_LERP2POS;

typedef struct
{
	int taskID;
	int entID;
	vec3_t origin;
	float duration;
} T_G_ICARUS_LERP2ORIGIN;

typedef struct
{
	int taskID;
	int entID;
	vec3_t angles;
	float duration;
} T_G_ICARUS_LERP2ANGLES;

typedef struct
{
	int entID;
	char name[2048];
	int lookup;
	vec3_t info;
} T_G_ICARUS_GETTAG;

typedef struct
{
	int entID;
	int taskID;
	float duration;
} T_G_ICARUS_LERP2START;

typedef struct
{
	int entID;
	int taskID;
	float duration;
} T_G_ICARUS_LERP2END;

typedef struct
{
	int entID;
	char target[2048];
} T_G_ICARUS_USE;

typedef struct
{
	int entID;
	char name[2048];
} T_G_ICARUS_KILL;

typedef struct
{
	int entID;
	char name[2048];
} T_G_ICARUS_REMOVE;

typedef struct
{
	int taskID;
	int entID;
	char type[2048];
	char name[2048];
} T_G_ICARUS_PLAY;

typedef struct
{
	int entID;
	int type;
	char name[2048];
	float value;
} T_G_ICARUS_GETFLOAT;

typedef struct
{
	int entID;
	int type;
	char name[2048];
	vec3_t value;
} T_G_ICARUS_GETVECTOR;

typedef struct
{
	int entID;
	int type;
	char name[2048];
	char value[2048];
} T_G_ICARUS_GETSTRING;

typedef struct
{
	char filename[2048];
} T_G_ICARUS_SOUNDINDEX;
typedef struct
{
	char string[2048];
} T_G_ICARUS_GETSETIDFORSTRING;

#endif //G_PUBLIC_H
