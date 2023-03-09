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

#ifndef __G_LOCAL_H__
#define __G_LOCAL_H__
// g_local.h -- local definitions for game module

// define GAME_INCLUDE so that g_public.h does not define the
// short, server-visible gclient_t and gentity_t structures,
// because we define the full size ones in this file
#define	GAME_INCLUDE
#include "../ui/gameinfo.h"
#include "g_shared.h"
#include "anims.h"
#include "dmstates.h"

//==================================================================

// the "gameversion" client command will print this plus compile date
#define	GAMEVERSION	"OpenJK"

#define BODY_QUEUE_SIZE		8

#define Q3_INFINITE			16777216

#define	FRAMETIME			100					// msec
#define	EVENT_VALID_MSEC	300
#define	CARNAGE_REWARD_TIME	3000

#define	INTERMISSION_DELAY_TIME	1000

#define	START_TIME_LINK_ENTS		FRAMETIME*1 // time-delay after map start at which all ents have been spawned, so can link them
#define	START_TIME_FIND_LINKS		FRAMETIME*2 // time-delay after map start at which you can find linked entities
#define	START_TIME_MOVERS_SPAWNED	FRAMETIME*2 // time-delay after map start at which all movers should be spawned
#define	START_TIME_REMOVE_ENTS		FRAMETIME*3 // time-delay after map start to remove temporary ents
#define	START_TIME_NAV_CALC			FRAMETIME*4 // time-delay after map start to connect waypoints and calc routes
#define	START_TIME_FIND_WAYPOINT	FRAMETIME*5 // time-delay after map start after which it's okay to try to find your best waypoint

// gentity->flags
#define	FL_SHIELDED				0x00000001	// protected from all damage except lightsabers
#define FL_DMG_BY_HEAVY_WEAP_ONLY	0x00000002	// protected from all damage except heavy weap class missiles
#define	FL_DMG_BY_SABER_ONLY	0x00000004	//protected from all damage except saber damage
#define	FL_GODMODE				0x00000010
#define	FL_NOTARGET				0x00000020
#define	FL_TEAMSLAVE			0x00000400	// not the first on the team
#define FL_NO_KNOCKBACK			0x00000800
#define FL_DROPPED_ITEM			0x00001000
#define	FL_DONT_SHOOT			0x00002000	// Can target him, but not shoot him
#define	FL_UNDYING				0x00004000	// Takes damage down to 1 point, but does not die
//#define FL_OVERCHARGED			0x00008000	// weapon shot is an overcharged version....probably a lame place to be putting this flag...
#define FL_LOCK_PLAYER_WEAPONS	0x00010000	// player can't switch weapons... ask james if there's a better spot for this
#define FL_DISINTEGRATED		0x00020000	// marks that the corpse has already been disintegrated
#define FL_FORCE_PULLABLE_ONLY	0x00040000	// cannot be force pushed
#define FL_NO_IMPACT_DMG		0x00080000	// Will not take impact damage
#define FL_OVERCHARGED_HEALTH	0x00100000	// Reduce health back to max
#define FL_NO_ANGLES			0x00200000	// No bone angle overrides, no pitch or roll in full angles
#define FL_RED_CROSSHAIR		0x00400000	// Crosshair red on me


//Pointer safety utilities
#define VALID( a )		( a != NULL )
#define	VALIDATE( a )	( assert( a ) )

#define	VALIDATEV( a )	if ( a == NULL ) {	assert(0);	return;			}
#define	VALIDATEB( a )	if ( a == NULL ) {	assert(0);	return qfalse;	}
#define VALIDATEP( a )	if ( a == NULL ) {	assert(0);	return NULL;	}

#define VALIDSTRING( a )	( ( a != NULL ) && ( a[0] != '\0' ) )

//animations
class animFileSet_t
{
public:
	char			filename[MAX_QPATH];
	animation_t		animations[MAX_ANIMATIONS];
	animevent_t		torsoAnimEvents[MAX_ANIM_EVENTS];
	animevent_t		legsAnimEvents[MAX_ANIM_EVENTS];
	unsigned char	torsoAnimEventCount;
	unsigned char	legsAnimEventCount;


	void sg_export(
		ojk::SavedGameHelper& saved_game) const
	{
		saved_game.write<int8_t>(filename);
		saved_game.write<>(animations);
		saved_game.write<>(torsoAnimEvents);
		saved_game.write<>(legsAnimEvents);
		saved_game.write<uint8_t>(torsoAnimEventCount);
		saved_game.write<uint8_t>(legsAnimEventCount);
		saved_game.skip(2);
	}

	void sg_import(
		ojk::SavedGameHelper& saved_game)
	{
		saved_game.read<int8_t>(filename);
		saved_game.read<>(animations);
		saved_game.read<>(torsoAnimEvents);
		saved_game.read<>(legsAnimEvents);
		saved_game.read<uint8_t>(torsoAnimEventCount);
		saved_game.read<uint8_t>(legsAnimEventCount);
		saved_game.skip(2);
	}
}; // animFileSet_t

extern stringID_table_t animTable [MAX_ANIMATIONS+1];

//Interest points

#define MAX_INTEREST_POINTS		64

typedef struct
{
	vec3_t		origin;
	char		*target;
} interestPoint_t;

//Combat points

#define MAX_COMBAT_POINTS		512

typedef struct
{
	vec3_t		origin;
	int			flags;
//	char		*NPC_targetname;
//	team_t		team;
	qboolean	occupied;
	int			waypoint;
	int			dangerTime;
} combatPoint_t;

// Alert events

#define	MAX_ALERT_EVENTS	32

enum alertEventType_e
{
	AET_SIGHT,
	AET_SOUND,
};

enum alertEventLevel_e
{
	AEL_MINOR,			//Enemy responds to the sound, but only by looking
	AEL_SUSPICIOUS,		//Enemy looks at the sound, and will also investigate it
	AEL_DISCOVERED,		//Enemy knows the player is around, and will actively hunt
	AEL_DANGER,			//Enemy should try to find cover
	AEL_DANGER_GREAT,	//Enemy should run like hell!
};

// !!!!!!!!! LOADSAVE-affecting struct !!!!!!!!!!
class alertEvent_t
{
public:
	vec3_t				position;	//Where the event is located
	float				radius;		//Consideration radius
	alertEventLevel_e	level;		//Priority level of the event
	alertEventType_e	type;		//Event type (sound,sight)
	gentity_t			*owner;		//Who made the sound
	float				light;		//ambient light level at point
	float				addLight;	//additional light- makes it more noticable, even in darkness
	int					ID;			//unique... if get a ridiculous number, this will repeat, but should not be a problem as it's just comparing it to your lastAlertID
	int					timestamp;	//when it was created
	qboolean			onGround;	//alert is on the ground (only used for sounds)


	void sg_export(
		ojk::SavedGameHelper& saved_game) const
	{
		saved_game.write<float>(position);
		saved_game.write<float>(radius);
		saved_game.write<int32_t>(level);
		saved_game.write<int32_t>(type);
		saved_game.write<int32_t>(owner);
		saved_game.write<float>(light);
		saved_game.write<float>(addLight);
		saved_game.write<int32_t>(ID);
		saved_game.write<int32_t>(timestamp);
		saved_game.write<int32_t>(onGround);
	}

	void sg_import(
		ojk::SavedGameHelper& saved_game)
	{
		saved_game.read<float>(position);
		saved_game.read<float>(radius);
		saved_game.read<int32_t>(level);
		saved_game.read<int32_t>(type);
		saved_game.read<int32_t>(owner);
		saved_game.read<float>(light);
		saved_game.read<float>(addLight);
		saved_game.read<int32_t>(ID);
		saved_game.read<int32_t>(timestamp);
		saved_game.read<int32_t>(onGround);
	}
}; // alertEvent_t

//
// this structure is cleared as each map is entered
//

#define	MAX_SPAWN_VARS			64
#define	MAX_SPAWN_VARS_CHARS	2048

typedef struct
{
	char	targetname[MAX_QPATH];
	char	target[MAX_QPATH];
	char	target2[MAX_QPATH];
	char	target3[MAX_QPATH];
	char	target4[MAX_QPATH];
	int		nodeID;
} waypointData_t;

#define	WF_RAINING		0x00000001	// raining
#define	WF_SNOWING		0x00000002	// snowing
#define	WF_PUFFING		0x00000004	// puffing something

// !!!!!!!!!! LOADSAVE-affecting structure !!!!!!!!!!
class level_locals_t
{
public:
	gclient_t	*clients;		// [maxclients]

	// store latched cvars here that we want to get at often
	int			maxclients;

	int			framenum;
	int			time;					// in msec
	int			previousTime;			// so movers can back up when blocked

	int			globalTime;				// global time at level initialization

	char		mapname[MAX_QPATH];		// the server name (base1, etc)

	qboolean	locationLinked;			// target_locations get linked
	gentity_t	*locationHead;			// head of the location list

	alertEvent_t	alertEvents[ MAX_ALERT_EVENTS ];
	int				numAlertEvents;
	int				curAlertID;

	AIGroupInfo_t	groups[MAX_FRAME_GROUPS];

	animFileSet_t	knownAnimFileSets[MAX_ANIM_FILES];
	int				numKnownAnimFileSets;

	int				worldFlags;

	int				dmState;		//actually, we do want save/load the dynamic music state
// =====================================
//
// NOTE!!!!!!   The only things beyond this point in the structure should be the ones you do NOT wish to be
//					affected by loading saved-games. Since loading a game first starts the map and then loads
//					over things like entities etc then these fields are usually the ones setup by the map loader.
//					If they ever get modified in-game let me know and I'll include them in the save. -Ste
//
#define LEVEL_LOCALS_T_SAVESTOP logFile	// name of whichever field is next below this in the source

	fileHandle_t	logFile;

	//Interest points- squadmates automatically look at these if standing around and close to them
	interestPoint_t	interestPoints[MAX_INTEREST_POINTS];
	int			numInterestPoints;

	//Combat points- NPCs in bState BS_COMBAT_POINT will find their closest empty combat_point
	combatPoint_t	combatPoints[MAX_COMBAT_POINTS];
	int			numCombatPoints;
	char		spawntarget[MAX_QPATH];		// the targetname of the spawnpoint you want the player to start at

	int			dmDebounceTime;
	int			dmBeatTime;

	int			mNumBSPInstances;
	int			mBSPInstanceDepth;
	vec3_t		mOriginAdjust;
	float		mRotationAdjust;
	char		*mTargetAdjust;
	qboolean	hasBspInstances;


	void sg_export(
		ojk::SavedGameHelper& saved_game) const
	{
		saved_game.write<int32_t>(clients);
		saved_game.write<int32_t>(maxclients);
		saved_game.write<int32_t>(framenum);
		saved_game.write<int32_t>(time);
		saved_game.write<int32_t>(previousTime);
		saved_game.write<int32_t>(globalTime);
		saved_game.write<int8_t>(mapname);
		saved_game.write<int32_t>(locationLinked);
		saved_game.write<int32_t>(locationHead);
		saved_game.write<>(alertEvents);
		saved_game.write<int32_t>(numAlertEvents);
		saved_game.write<int32_t>(curAlertID);
		saved_game.write<>(groups);
		saved_game.write<>(knownAnimFileSets);
		saved_game.write<int32_t>(numKnownAnimFileSets);
		saved_game.write<int32_t>(worldFlags);
		saved_game.write<int32_t>(dmState);
	}

	void sg_import(
		ojk::SavedGameHelper& saved_game)
	{
		saved_game.read<int32_t>(clients);
		saved_game.read<int32_t>(maxclients);
		saved_game.read<int32_t>(framenum);
		saved_game.read<int32_t>(time);
		saved_game.read<int32_t>(previousTime);
		saved_game.read<int32_t>(globalTime);
		saved_game.read<int8_t>(mapname);
		saved_game.read<int32_t>(locationLinked);
		saved_game.read<int32_t>(locationHead);
		saved_game.read<>(alertEvents);
		saved_game.read<int32_t>(numAlertEvents);
		saved_game.read<int32_t>(curAlertID);
		saved_game.read<>(groups);
		saved_game.read<>(knownAnimFileSets);
		saved_game.read<int32_t>(numKnownAnimFileSets);
		saved_game.read<int32_t>(worldFlags);
		saved_game.read<int32_t>(dmState);
	}
}; // level_locals_t

extern	level_locals_t	level;
extern	game_export_t	globals;

extern	cvar_t	*g_gravity;
extern	cvar_t	*g_speed;
extern	cvar_t	*g_cheats;
extern	cvar_t	*g_developer;
extern	cvar_t	*g_knockback;
extern	cvar_t	*g_inactivity;
extern	cvar_t	*g_debugMove;
extern	cvar_t	*g_subtitles;
extern	cvar_t	*g_removeDoors;

extern	cvar_t	*g_ICARUSDebug;

extern cvar_t	*g_npcdebug;

extern	cvar_t	*g_allowBunnyhopping;
extern gentity_t *player;
//
// g_spawn.c
//
qboolean	G_SpawnField( unsigned int uiField, char **ppKey, char **ppValue );
qboolean	G_SpawnString( const char *key, const char *defaultString, char **out );
// spawn string returns a temporary reference, you must CopyString() if you want to keep it
qboolean	G_SpawnFloat( const char *key, const char *defaultString, float *out );
qboolean	G_SpawnInt( const char *key, const char *defaultString, int *out );
qboolean	G_SpawnVector( const char *key, const char *defaultString, float *out );
qboolean	G_SpawnVector4( const char *key, const char *defaultString, float *out );
qboolean	G_SpawnAngleHack( const char *key, const char *defaultString, float *out );
void		G_SpawnEntitiesFromString( const char *entities );

//
// g_cmds.c
//
void Cmd_Score_f (gentity_t *ent);

//
// g_items.c
//
void G_RunItem( gentity_t *ent );
void RespawnItem( gentity_t *ent );
gentity_t *Drop_Item( gentity_t *ent, gitem_t *item, float angle, qboolean copytarget );
void G_SpawnItem (gentity_t *ent, gitem_t *item);
void FinishSpawningItem( gentity_t *ent );
void	Add_Ammo (gentity_t *ent, int weapon, int count);
void Touch_Item (gentity_t *ent, gentity_t *other, trace_t *trace);

void ClearRegisteredItems( void );
void RegisterItem( gitem_t *item );
void SaveRegisteredItems( void );

//
// g_utils.c
//
int G_ModelIndex( const char *name );
int	G_SoundIndex( const char *name );
/*
Ghoul2 Insert Start
*/
int G_SkinIndex( const char *name );
void	G_SetBoltSurfaceRemoval( const int entNum, const int modelIndex, const int boltIndex, const int surfaceIndex = -1, float duration = 5000 );
/*
Ghoul2 Insert End
*/

int G_EffectIndex( const char *name );
void G_PlayEffect( const char *name, const vec3_t origin );
void G_PlayEffect( const char *name, int clientNum );
void G_PlayEffect( const char *name, const vec3_t origin, const vec3_t fwd );
void G_PlayEffect( const char *name, const vec3_t origin, const vec3_t axis[3] );
void G_PlayEffect( int fxID, const vec3_t origin );
void G_PlayEffect( int fxID, const vec3_t origin, const vec3_t fwd );
void G_PlayEffect( int fxID, const vec3_t origin, const vec3_t axis[3] );
void G_PlayEffect( int fxID, const int modelIndex, const int boltIndex, const int entNum, const vec3_t origin, int iLoopTime = qfalse, qboolean isRelative = qfalse );//iLoopTime 0 = not looping, 1 for infinite, else duration
void G_PlayEffect( int fxID, int entNum, const vec3_t fwd );
void G_StopEffect( int fxID, const int modelIndex, const int boltIndex, const int entNum );
void G_StopEffect(const char *name, const int modelIndex, const int boltIndex, const int entNum );

int G_BSPIndex( char *name );

void	G_KillBox (gentity_t *ent);
gentity_t *G_Find (gentity_t *from, int fieldofs, const char *match);
int		G_RadiusList ( vec3_t origin, float radius,	gentity_t *ignore, qboolean takeDamage, gentity_t *ent_list[MAX_GENTITIES]);
gentity_t *G_PickTarget (char *targetname);
void	G_UseTargets (gentity_t *ent, gentity_t *activator);
void	G_UseTargets2 (gentity_t *ent, gentity_t *activator, const char *string);
void	G_SetMovedir ( vec3_t angles, vec3_t movedir);

void	G_InitGentity( gentity_t *e, qboolean bFreeG2 );
gentity_t	*G_Spawn (void);
gentity_t *G_TempEntity( const vec3_t origin, int event );
void	G_Sound( gentity_t *ent, int soundIndex );
void	G_FreeEntity( gentity_t *e );

void	G_TouchTriggers (gentity_t *ent);
void	G_TouchTeamClients (gentity_t *ent);
void	G_TouchSolids (gentity_t *ent);

char	*vtos( const vec3_t v );

float vectoyaw( const vec3_t vec );

void G_AddEvent( gentity_t *ent, int event, int eventParm );
void G_SetOrigin( gentity_t *ent, const vec3_t origin );
void G_SetAngles( gentity_t *ent, const vec3_t angles );

void	G_DebugLine(vec3_t A, vec3_t B, int duration, int color, qboolean deleteornot);

//
// g_combat.c
//
qboolean CanDamage (gentity_t *targ, const vec3_t origin);
void G_Damage( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, const vec3_t dir, const vec3_t point, int damage, int dflags, int mod, int hitLoc=HL_NONE );
void G_RadiusDamage (const vec3_t origin, gentity_t *attacker, float damage, float radius, gentity_t *ignore, int mod);
gentity_t *TossClientItems( gentity_t *self );
void ExplodeDeath_Wait( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath,int dFlags,int hitLoc );
void ExplodeDeath( gentity_t *self );
void GoExplodeDeath( gentity_t *self, gentity_t *other, gentity_t *activator);
void G_ApplyKnockback( gentity_t *targ, const vec3_t newDir, float knockback );
void G_Throw( gentity_t *targ, const vec3_t newDir, float push );

// damage flags
#define DAMAGE_RADIUS			0x00000001	// damage was indirect
#define DAMAGE_NO_ARMOR			0x00000002	// armour does not protect from this damage
#define DAMAGE_NO_KNOCKBACK		0x00000008	// do not affect velocity, just view angles
#define DAMAGE_NO_HIT_LOC		0x00000010	// do not modify damage by hit loc
#define DAMAGE_NO_PROTECTION	0x00000020  // armor, shields, invulnerability, and godmode have no effect
#define DAMAGE_EXTRA_KNOCKBACK	0x00000040	// add extra knockback to this damage
#define DAMAGE_DEATH_KNOCKBACK	0x00000080	// only does knockback on death of target
#define DAMAGE_IGNORE_TEAM		0x00000100	// damage is always done, regardless of teams
#define DAMAGE_NO_DAMAGE		0x00000200	// do no actual damage but react as if damage was taken
#define DAMAGE_DISMEMBER		0x00000400	// do dismemberment
#define DAMAGE_NO_KILL			0x00000800	// do damage, but don't kill them
#define DAMAGE_HEAVY_WEAP_CLASS	0x00001000	// doing heavy weapon type damage, certain objects may only take damage by missiles containing this flag
#define DAMAGE_CUSTOM_HUD		0x00002000	// really dumb, but....
#define	DAMAGE_IMPACT_DIE		0x00004000	// if a vehicle hits a wall it should instantly die
#define	DAMAGE_DIE_ON_IMPACT	0x00008000	// ignores force-power based protection
#define	DAMAGE_SABER_KNOCKBACK1 0x00010000	// scale knockback based on saber1's knockbackScale value
#define	DAMAGE_SABER_KNOCKBACK2 0x00020000	// scale knockback based on saber2's knockbackScale value
#define	DAMAGE_SABER_KNOCKBACK1_B2 0x00040000	// scale knockback based on saber1's knockbackScale2 value
#define	DAMAGE_SABER_KNOCKBACK2_B2 0x00080000	// scale knockback based on saber2's knockbackScale2 value
//
// g_missile.c
//
void G_RunMissile( gentity_t *ent );

//
// g_mover.c
//
#define MOVER_START_ON		1
#define MOVER_FORCE_ACTIVATE	2
#define MOVER_CRUSHER		4
#define MOVER_TOGGLE		8
#define MOVER_LOCKED		16
#define MOVER_GOODIE		32
#define MOVER_PLAYER_USE	64
#define MOVER_INACTIVE		128
void G_RunMover( gentity_t *ent );


//
// g_misc.c
//
void TeleportPlayer( gentity_t *player, vec3_t origin, vec3_t angles );


//
// g_weapon.c
//
void WP_LoadWeaponParms (void);

void IT_LoadItemParms( void );

//
// g_client.c
//
void SetClientViewAngle( gentity_t *ent, vec3_t angle );
gentity_t *SelectSpawnPoint ( vec3_t avoidPoint, team_t team, vec3_t origin, vec3_t angles );
void respawn (gentity_t *ent);
qboolean ClientSpawn( gentity_t *ent, SavedGameJustLoaded_e eSavedGameJustLoaded );
void player_die (gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod, int dFlags, int hitLoc);
void AddScore( gentity_t *ent, int score );
qboolean SpotWouldTelefrag( gentity_t *spot, team_t checkteam );
void G_RemoveWeaponModels( gentity_t *ent );

//
// g_svcmds.c
//
qboolean	ConsoleCommand( void );

//
// g_weapon.c
//
void FireWeapon( gentity_t *ent, qboolean alt_fire );

//
// p_hud.c
//
void MoveClientToIntermission (gentity_t *client);
void G_SetStats (gentity_t *ent);
void DeathmatchScoreboardMessage (gentity_t *client);

//
// g_cmds.c
//
void G_SayTo( gentity_t *ent, gentity_t *other, int mode, int color, const char *name, const char *message );

//
// g_pweapon.c
//


//
// g_main.c
//
void G_RunThink (gentity_t *ent);
void NORETURN QDECL G_Error( const char *fmt, ... );
void SetInUse(gentity_t *ent);
void ClearInUse(gentity_t *ent);
qboolean PInUse(unsigned int entNum);
qboolean PInUse2(gentity_t *ent);
void WriteInUseBits(void);
void ReadInUseBits(void);

//
// g_nav.cpp
//
void Svcmd_Nav_f (void);

//
// g_squad.cpp
//
void Svcmd_Comm_f (void);
void Svcmd_Hail_f (void);
void Svcmd_Form_f (void);


//
// g_utils.cpp
//
void Svcmd_Use_f (void);
extern void G_SoundOnEnt( gentity_t *ent, soundChannel_t channel, const char *soundPath );
extern void G_SoundIndexOnEnt( gentity_t *ent, soundChannel_t channel, int index );

//
// g_weapons.cpp
//

//
// g_client.c
//
char *ClientConnect( int clientNum, qboolean firstTime, SavedGameJustLoaded_e eSavedGameJustLoaded );
void ClientUserinfoChanged( int clientNum );
void ClientDisconnect( int clientNum );
void ClientBegin( int clientNum, usercmd_t *cmd, SavedGameJustLoaded_e eSavedGameJustLoaded );
void ClientCommand( int clientNum );

//
// g_active.c
//
void ClientThink( int clientNum, usercmd_t *cmd );
void ClientEndFrame (gentity_t *ent);

//
// g_inventory.cpp
//
extern qboolean INV_GoodieKeyGive( gentity_t *target );
extern qboolean INV_GoodieKeyTake( gentity_t *target );
extern int INV_GoodieKeyCheck( gentity_t *target );
extern qboolean INV_SecurityKeyGive( gentity_t *target, const char *keyname );
extern void INV_SecurityKeyTake( gentity_t *target, const char *keyname );
extern qboolean INV_SecurityKeyCheck( gentity_t *target, const char *keyname );

//
// g_team.c
//
qboolean OnSameTeam( gentity_t *ent1, gentity_t *ent2 );


//
// g_mem.c
//
void *G_Alloc( int size );
void G_InitMemory( void );
void Svcmd_GameMem_f( void );

//
// g_session.c
//
void G_ReadSessionData( gclient_t *client );
void G_InitSessionData( gclient_t *client, char *userinfo );

void G_InitWorldSession( void );
void G_WriteSessionData( void );


//
// NPC_senses.cpp
//
extern void AddSightEvent( gentity_t *owner, vec3_t position, float radius, alertEventLevel_e alertLevel, float addLight=0.0f );
extern void AddSoundEvent( gentity_t *owner, vec3_t position, float radius, alertEventLevel_e alertLevel, qboolean needLOS = qfalse, qboolean onGround = qfalse );
extern qboolean G_CheckForDanger( gentity_t *self, int alertEvent );
extern int G_CheckAlertEvents( gentity_t *self, qboolean checkSight, qboolean checkSound, float maxSeeDist, float maxHearDist, int ignoreAlert = -1, qboolean mustHaveOwner = qfalse, int minAlertLevel = AEL_MINOR, qboolean onGroundOnly = qfalse );
extern qboolean G_CheckForDanger( gentity_t *self, int alertEvent );
extern qboolean G_ClearLOS( gentity_t *self, const vec3_t start, const vec3_t end );
extern qboolean G_ClearLOS( gentity_t *self, gentity_t *ent, const vec3_t end );
extern qboolean G_ClearLOS( gentity_t *self, const vec3_t start, gentity_t *ent );
extern qboolean G_ClearLOS( gentity_t *self, gentity_t *ent );
extern qboolean G_ClearLOS( gentity_t *self, const vec3_t end );

//============================================================================

//Tags

// Reference tags

#define MAX_REFTAGS	128		// Probably more than we'll ever need
#define MAX_REFNAME	32

#define	RTF_NONE	0
#define	RTF_NAVGOAL	0x00000001

typedef struct reference_tag_s
{
	char	name[MAX_REFNAME];
	vec3_t	origin;
	vec3_t	angles;
	int		flags;	//Just in case
	int		radius;	//For nav goals
} reference_tag_t;

extern	void TAG_Init( void );
extern	reference_tag_t	*TAG_Add( const char *name, const char *owner, vec3_t origin, vec3_t angles, int radius, int flags );

extern	int	TAG_GetOrigin( const char *owner, const char *name, vec3_t origin );
extern	int	TAG_GetAngles( const char *owner, const char *name, vec3_t angles );
extern	int TAG_GetRadius( const char *owner, const char *name );
extern	int TAG_GetFlags( const char *owner, const char *name );

void TAG_ShowTags( int flags );

// Reference tags END

extern char *G_NewString( const char *string );

// some stuff for savegames...
//
void WriteLevel(qboolean qbAutosave);
void ReadLevel(qboolean qbAutosave, qboolean qbLoadTransition);
qboolean GameAllowedToSaveHere(void);

extern qboolean G_ActivateBehavior( gentity_t *ent, int bset );

//Timing information
void		TIMER_Clear( void );
void		TIMER_Clear( int idx );
void		TIMER_Save( void );
void		TIMER_Load( void );
void		TIMER_Set( gentity_t *ent, const char *identifier, int duration );
int			TIMER_Get( gentity_t *ent, const char *identifier );
qboolean	TIMER_Done( gentity_t *ent, const char *identifier );
qboolean	TIMER_Start( gentity_t *self, const char *identifier, int duration );
qboolean	TIMER_Done2( gentity_t *ent, const char *identifier, qboolean remove = qfalse );
qboolean	TIMER_Exists( gentity_t *ent, const char *identifier );
void		TIMER_Remove( gentity_t *ent, const char *identifier );

float NPC_GetHFOVPercentage( vec3_t spot, vec3_t from, vec3_t facing, float hFOV );
float NPC_GetVFOVPercentage( vec3_t spot, vec3_t from, vec3_t facing, float vFOV );

#endif//#ifndef __G_LOCAL_H__
