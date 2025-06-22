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

#ifndef __BG_PUBLIC_H__
#define __BG_PUBLIC_H__
// bg_public.h -- definitions shared by both the server game and client game modules
#include "weapons.h"
#include "g_items.h"
#include "teams.h"
#include "statindex.h"

#define	DEFAULT_GRAVITY		800
#define	GIB_HEALTH			-40
#define	ARMOR_PROTECTION	0.40

#define	MAX_ITEMS			256

#define	RANK_TIED_FLAG		0x4000

#define DEFAULT_SHOTGUN_SPREAD	700
#define DEFAULT_SHOTGUN_COUNT	11

#define	ITEM_RADIUS			15		// item sizes are needed for client side pickup detection

//Player sizes
extern float DEFAULT_MINS_0;
extern float DEFAULT_MINS_1;
extern float DEFAULT_MAXS_0;
extern float DEFAULT_MAXS_1;
extern float DEFAULT_PLAYER_RADIUS;
#define DEFAULT_MINS_2		-24
#define DEFAULT_MAXS_2		40// was 32, but too short for player
#define CROUCH_MAXS_2		16

#define ATST_MINS0			-40
#define ATST_MINS1			-40
#define ATST_MINS2			-24
#define ATST_MAXS0			40
#define ATST_MAXS1			40
#define ATST_MAXS2			248

//Player viewheights
#define	STANDARD_VIEWHEIGHT_OFFSET -4
//#define	RAVEN_VIEWHEIGHT_ADJ 2
//#define	DEFAULT_VIEWHEIGHT	(26+RAVEN_VIEWHEIGHT_ADJ)
//#define CROUCH_VIEWHEIGHT	12
#define	DEAD_VIEWHEIGHT		-16
//Player movement values
#define	MIN_WALK_NORMAL		0.7		// can't walk on very steep slopes
#define	JUMP_VELOCITY		225		// 270
#define	STEPSIZE			18



/*
===================================================================================

PMOVE MODULE

The pmove code takes a player_state_t and a usercmd_t and generates a new player_state_t
and some other output data.  Used for local prediction on the client game and true
movement on the server game.
===================================================================================
*/

typedef enum {
	PM_NORMAL,		// can accelerate and turn
	PM_NOCLIP,		// noclip movement
	PM_SPECTATOR,	// still run into walls
	PM_DEAD,		// no acceleration or turning, but free falling
	PM_FREEZE,		// stuck in place with no control
	PM_INTERMISSION	// no movement or status bar
} pmtype_t;

typedef enum {
	WEAPON_READY,
	WEAPON_RAISING,
	WEAPON_DROPPING,
	WEAPON_FIRING,
	WEAPON_CHARGING,
	WEAPON_CHARGING_ALT,
	WEAPON_IDLE, //lowered
} weaponstate_t;

// pmove->pm_flags
#define	PMF_DUCKED			1
#define	PMF_JUMP_HELD		2
#define	PMF_JUMPING			4		// yes, I really am in a jump -- Mike, you may want to come up with something better here since this is really a temp fix.
#define	PMF_BACKWARDS_JUMP	8		// go into backwards land
#define	PMF_BACKWARDS_RUN	16		// coast down to backwards run
#define	PMF_TIME_LAND		32		// pm_time is time before rejump
#define	PMF_TIME_KNOCKBACK	64		// pm_time is an air-accelerate only time
#define	PMF_TIME_NOFRICTION	128		// pm_time is a no-friction time
//#define	PMF_BLOCKED			128		// Bumped into something you can't step up - NOTHING WAS EVER CHECKING THIS
#define	PMF_TIME_WATERJUMP	256		// pm_time is waterjump
#define	PMF_RESPAWNED		512		// clear after attack and jump buttons come up
#define	PMF_USE_FORCE		1024	// for debouncing the button
#define PMF_JUMP_DUCKED		2048	// viewheight changes in mid-air
#define PMF_TRIGGER_PUSHED	4096	// pushed by a trigger_push or other such thing - cannot force jump and will not take impact damage
//#define PMF_FOLLOW			4096	// spectate following another player
#define PMF_SCOREBOARD		8192	// spectate as a scoreboard
//#define	PMF_BUMPED			16384	// Bumped into something and slid to the side - NOTHING WAS EVER CHECKING THIS
#define PMF_SLOW_MO_FALL	16384	// Fall slower until hit ground
#define	PMF_ATTACK_HELD		32768	// Holding down the attack button
#define	PMF_ALT_ATTACK_HELD	65536	// Holding down the alt-attack button

#define	PMF_ALL_TIMES	(PMF_TIME_WATERJUMP|PMF_TIME_LAND|PMF_TIME_KNOCKBACK|PMF_TIME_NOFRICTION)

#define	MAXTOUCH	32
typedef struct gentity_s gentity_t;
typedef struct {
	// state (in / out)
	playerState_t	*ps;

	// command (in)
	usercmd_t	cmd;
	int			tracemask;			// collide against these types of surfaces
	int			debugLevel;			// if set, diagnostic output will be printed
	qboolean	noFootsteps;		// if the game is setup for no footsteps by the server

	// results (out)
	int			numtouch;
	int			touchents[MAXTOUCH];

	int			useEvent;

	vec3_t		mins, maxs;			// bounding box size

	int			watertype;
	int			waterlevel;

	float		xyspeed;
	gentity_s	*gent;				// Pointer to entity in g_entities[]

	// callbacks to test the world
	// these will be different functions during game and cgame
	void		(*trace)( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
						const int passEntityNum, const int contentMask, const EG2_Collision eG2TraceType/* = (EG2_Collision)0*/, const int useLod/* = 0 */);
	int			(*pointcontents)( const vec3_t point, int passEntityNum );
} pmove_t;

// if a full pmove isn't done on the client, you can just update the angles
void PM_UpdateViewAngles( playerState_t *ps, usercmd_t *cmd, gentity_t *gent );
void Pmove( pmove_t *pmove );


#define SETANIM_TORSO 1
#define SETANIM_LEGS  2
#define SETANIM_BOTH  (SETANIM_TORSO|SETANIM_LEGS)//3

#define SETANIM_FLAG_NORMAL		0//Only set if timer is 0
#define SETANIM_FLAG_OVERRIDE	1//Override previous
#define SETANIM_FLAG_HOLD		2//Set the new timer
#define SETANIM_FLAG_RESTART	4//Allow restarting the anim if playing the same one (weapon fires)
#define SETANIM_FLAG_HOLDLESS	8//Set the new timer

void PM_SetAnim(pmove_t	*pm,int setAnimParts,int anim,int setAnimFlags, int blendTime=100);
void PM_SetAnimFinal(int *torsoAnim,int *legsAnim,int type,int anim,int priority,int *torsoAnimTimer,int *legsAnimTimer,gentity_t *gent,int blendTime=100);
void PM_SetSaberMove(short newMove);

//===================================================================================


// player_state->persistant[] indexes
// these fields are the only part of player_state that isn't
// cleared on respawn
//
//  NOTE!!! Even though this is an enum, the array that contains these uses #define MAX_PERSISTANT 16 in q_shared.h,
//		so be careful how many you add since it'll just overflow without telling you -slc
//
typedef enum {
	PERS_SCORE,						// !!! MUST NOT CHANGE, SERVER AND GAME BOTH REFERENCE !!!
	PERS_HITS,						// total points damage inflicted so damage beeps can sound on change
	PERS_TEAM,
	PERS_SPAWN_COUNT,				// incremented every respawn
//	PERS_REWARD_COUNT,				// incremented for each reward sound
	PERS_ATTACKER,					// clientnum of last damage inflicter
	PERS_KILLED,					// count of the number of times you died

	PERS_ACCURACY_SHOTS,			// scoreboard - number of player shots
	PERS_ACCURACY_HITS,				// scoreboard - number of player shots that hit an enemy
	PERS_ENEMIES_KILLED,			// scoreboard - number of enemies player killed
	PERS_TEAMMATES_KILLED			// scoreboard - number of teammates killed
} persEnum_t;


// entityState_t->eFlags
#define	EF_DEAD					0x00000001	// don't draw a foe marker over players with EF_DEAD
#define	EF_NPC					0x00000002	// An NPC
#define	EF_TELEPORT_BIT			0x00000004	// toggled every time the origin abruptly changes
#define	EF_SHADER_ANIM			0x00000008	// Animating shader (by s.frame)
#define	EF_BOUNCE				0x00000010	// for missiles
#define	EF_BOUNCE_HALF			0x00000020	// for missiles
#define EF_MISSILE_STICK		0x00000040	// missiles that stick to the wall.
#define	EF_NODRAW				0x00000080	// may have an event, but no model (unspawned items)
#define	EF_FIRING				0x00000100	// for lightning gun
#define EF_ALT_FIRING			0x00000200	// for alt-fires, mostly for lightning guns though
#define	EF_NO_TED				0x00000400	// won't show up on TED unless actively scan with tricorder
#define	EF_AUTO_SIZE			0x00000800	// CG_Ents will create the mins & max itself based on model bounds
#define	EF_BOUNCE_SHRAPNEL		0x00001000	// special shrapnel flag
#define	EF_CONNECTION			0x00002000	// draw a connection trouble sprite
#define EF_ANIM_ALLFAST			0x00004000	// automatically cycle through all frames at 10hz
#define EF_ANIM_ONCE			0x00008000	// cycle through all frames just once then stop
#define EF_PLANTED_CHARGE		0x00010000	// For detpack charge
#define EF_PROX_TRIP			0x00020000	// Proximity trip mine has been activated
#define EF_LOCKED_TO_WEAPON		0x00040000	// When we use an emplaced weapon, we turn this on to lock us to that weapon
//#define EF_use_me				0x00080000	// Not used
//#define EF_use_me				0x00100000	// Not used
//#define EF_use_me				0x00200000	// Not used
//#define EF_use_me				0x00400000	// Not used
//#define EF_use_me				0x00800000	// Not used
#define EF_IN_ATST				0x01000000	// Driving an ATST
#define EF_DISINTEGRATION		0x02000000	// Disruptor effect
//#define EF_use_me				0x04000000	// Not used
//#define EF_use_me				0x08000000	// Not used
#define EF_DISABLE_SHADER_ANIM	0x10000000	// Normally shader animation chugs along, but movers can force shader animation to be on frame 1
#define EF_FORCE_GRIPPED		0x20000000	// Force gripped effect
//#define EF_use_me				0x40000000	// Not used
#define EF_BLOCKED_MOVER		0x80000000	// for movers that are blocked - shared with previous

typedef enum {
	PW_NONE,
	PW_QUAD,// This can go away
	PW_BATTLESUIT,
	PW_HASTE,// This can go away
	PW_CLOAKED,
	PW_UNCLOAKING,
	PW_DISRUPTION,
	PW_GALAK_SHIELD,
//	PW_WEAPON_OVERCHARGE,
	PW_SEEKER,
	PW_SHOCKED,//electricity effect
	PW_DISINT_2,//ghost
	PW_INVINCIBLE,
	PW_FORCE_PUSH,

	PW_NUM_POWERUPS
} powerup_t;

#define PW_REMOVE_AT_DEATH ((1<<PW_QUAD)|(1<<PW_BATTLESUIT)|(1<<PW_HASTE)|(1<<PW_CLOAKED)|(1<<PW_UNCLOAKING)|(1<<PW_UNCLOAKING)|(1<<PW_GALAK_SHIELD)|(1<<PW_DISINT_2)|(1<<PW_INVINCIBLE)|(1<<PW_SEEKER))
// entityState_t->event values
// entity events are for effects that take place relative
// to an existing entities origin.  Very network efficient.

// two bits at the top of the entityState->event field
// will be incremented with each change in the event so
// that an identical event started twice in a row can
// be distinguished.  And off the value with ~EV_EVENT_BITS
// to retrieve the actual event number
#define	EV_EVENT_BIT1		0x00000100
#define	EV_EVENT_BIT2		0x00000200
#define	EV_EVENT_BITS		(EV_EVENT_BIT1|EV_EVENT_BIT2)

typedef enum {
	EV_NONE,

	EV_FOOTSTEP,
	EV_FOOTSTEP_METAL,
	EV_FOOTSPLASH,
	EV_FOOTWADE,
	EV_SWIM,

	EV_STEP_4,
	EV_STEP_8,
	EV_STEP_12,
	EV_STEP_16,

	EV_FALL_SHORT,
	EV_FALL_MEDIUM,
	EV_FALL_FAR,

	EV_JUMP,
	EV_ROLL,
	EV_WATER_TOUCH,	// foot touches
	EV_WATER_LEAVE,	// foot leaves
	EV_WATER_UNDER,	// head touches
	EV_WATER_CLEAR,	// head leaves
	EV_WATER_GURP1,	// need air 1
	EV_WATER_GURP2,	// need air 2
	EV_WATER_DROWN,	// drowned

	EV_ITEM_PICKUP,

	EV_NOAMMO,
	EV_CHANGE_WEAPON,
	EV_FIRE_WEAPON,
	EV_ALT_FIRE,
	EV_POWERUP_SEEKER_FIRE,
	EV_POWERUP_BATTLESUIT,
	EV_USE,

	EV_REPLICATOR,

	EV_BATTERIES_CHARGED,

	EV_GRENADE_BOUNCE,		// eventParm will be the soundindex
	EV_MISSILE_STICK,		// eventParm will be the soundindex

	EV_BMODEL_SOUND,
	EV_GENERAL_SOUND,
	EV_GLOBAL_SOUND,		// no attenuation

	EV_PLAY_EFFECT,
	EV_PLAY_MUZZLE_EFFECT,

	EV_TARGET_BEAM_DRAW,

	EV_DISRUPTOR_MAIN_SHOT,
	EV_DISRUPTOR_SNIPER_SHOT,
	EV_DISRUPTOR_SNIPER_MISS,

	EV_DEMP2_ALT_IMPACT,

	EV_PAIN,
	EV_DEATH1,
	EV_DEATH2,
	EV_DEATH3,

	EV_MISSILE_HIT,
	EV_MISSILE_MISS,

	EV_DISINTEGRATION,

	EV_ANGER1,	//Say when acquire an enemy when didn't have one before
	EV_ANGER2,
	EV_ANGER3,

	EV_VICTORY1,	//Say when killed an enemy
	EV_VICTORY2,
	EV_VICTORY3,

	EV_CONFUSE1,	//Say when confused
	EV_CONFUSE2,
	EV_CONFUSE3,

	EV_PUSHED1,		//Say when pushed
	EV_PUSHED2,
	EV_PUSHED3,

	EV_CHOKE1,		//Say when choking
	EV_CHOKE2,
	EV_CHOKE3,

	EV_FFWARN,		//ffire founds
	EV_FFTURN,
	//extra sounds for ST
	EV_CHASE1,
	EV_CHASE2,
	EV_CHASE3,
	EV_COVER1,
	EV_COVER2,
	EV_COVER3,
	EV_COVER4,
	EV_COVER5,
	EV_DETECTED1,
	EV_DETECTED2,
	EV_DETECTED3,
	EV_DETECTED4,
	EV_DETECTED5,
	EV_LOST1,
	EV_OUTFLANK1,
	EV_OUTFLANK2,
	EV_ESCAPING1,
	EV_ESCAPING2,
	EV_ESCAPING3,
	EV_GIVEUP1,
	EV_GIVEUP2,
	EV_GIVEUP3,
	EV_GIVEUP4,
	EV_LOOK1,
	EV_LOOK2,
	EV_SIGHT1,
	EV_SIGHT2,
	EV_SIGHT3,
	EV_SOUND1,
	EV_SOUND2,
	EV_SOUND3,
	EV_SUSPICIOUS1,
	EV_SUSPICIOUS2,
	EV_SUSPICIOUS3,
	EV_SUSPICIOUS4,
	EV_SUSPICIOUS5,
	//extra sounds for Jedi
	EV_COMBAT1,
	EV_COMBAT2,
	EV_COMBAT3,
	EV_JDETECTED1,
	EV_JDETECTED2,
	EV_JDETECTED3,
	EV_TAUNT1,
	EV_TAUNT2,
	EV_TAUNT3,
	EV_JCHASE1,
	EV_JCHASE2,
	EV_JCHASE3,
	EV_JLOST1,
	EV_JLOST2,
	EV_JLOST3,
	EV_DEFLECT1,
	EV_DEFLECT2,
	EV_DEFLECT3,
	EV_GLOAT1,
	EV_GLOAT2,
	EV_GLOAT3,
	EV_PUSHFAIL,

	EV_USE_ITEM,

	EV_USE_INV_BINOCULARS,
	EV_USE_INV_BACTA,
	EV_USE_INV_SEEKER,
	EV_USE_INV_LIGHTAMP_GOGGLES,
	EV_USE_INV_SENTRY,

	EV_USE_FORCE,

	EV_DRUGGED,		// hit by an interrogator

	EV_DEBUG_LINE,

	EV_NUM_ENTITY_EVENTS
} entity_event_t;

class animation_t
{
public:
	int		firstFrame;
	int		numFrames;
	int		loopFrames;			// 0 to numFrames, -1 = no loop
	int		frameLerp;			// msec between frames
	int		initialLerp;		// msec to get to first frame


	void sg_export(
		ojk::SavedGameHelper& saved_game) const
	{
		saved_game.write<int32_t>(firstFrame);
		saved_game.write<int32_t>(numFrames);
		saved_game.write<int32_t>(loopFrames);
		saved_game.write<int32_t>(frameLerp);
		saved_game.write<int32_t>(initialLerp);
	}

	void sg_import(
		ojk::SavedGameHelper& saved_game)
	{
		saved_game.read<int32_t>(firstFrame);
		saved_game.read<int32_t>(numFrames);
		saved_game.read<int32_t>(loopFrames);
		saved_game.read<int32_t>(frameLerp);
		saved_game.read<int32_t>(initialLerp);
	}
}; // animation_t

#define	MAX_RANDOM_ANIMSOUNDS	8

typedef struct animsounds_s
{
	int		keyFrame;			//Frame to play sound on
	int		soundIndex[MAX_RANDOM_ANIMSOUNDS];			//sound file to play - FIXME: should be an index, handle random some other way?
	int		numRandomAnimSounds;		//string variable min for va("...%d.wav", Q_irand(lowestVa, highestVa))
	int		probability;		//chance sound will play, zero value will not run this test (0 = 100% basically)


	void sg_export(
		ojk::SavedGameHelper& saved_game) const
	{
		saved_game.write<int32_t>(keyFrame);
		saved_game.write<int32_t>(soundIndex);
		saved_game.write<int32_t>(numRandomAnimSounds);
		saved_game.write<int32_t>(probability);
	}

	void sg_import(
		ojk::SavedGameHelper& saved_game)
	{
		saved_game.read<int32_t>(keyFrame);
		saved_game.read<int32_t>(soundIndex);
		saved_game.read<int32_t>(numRandomAnimSounds);
		saved_game.read<int32_t>(probability);
	}
} animsounds_t;

// means of death
typedef enum {

	MOD_UNKNOWN,

// weapons
	MOD_SABER,
	MOD_BRYAR,
	MOD_BRYAR_ALT,
	MOD_BLASTER,
	MOD_BLASTER_ALT,
	MOD_DISRUPTOR,
	MOD_SNIPER,
	MOD_BOWCASTER,
	MOD_BOWCASTER_ALT,
	MOD_REPEATER,
	MOD_REPEATER_ALT,
	MOD_DEMP2,
	MOD_DEMP2_ALT,
	MOD_FLECHETTE,
	MOD_FLECHETTE_ALT,
	MOD_ROCKET,
	MOD_ROCKET_ALT,
	MOD_THERMAL,
	MOD_THERMAL_ALT,
	MOD_DETPACK,
	MOD_LASERTRIP,
	MOD_LASERTRIP_ALT,
	MOD_MELEE,
	MOD_SEEKER,
	MOD_FORCE_GRIP,
	MOD_EMPLACED,

// world / generic
	MOD_ELECTROCUTE,
	MOD_EXPLOSIVE,
	MOD_EXPLOSIVE_SPLASH,
	MOD_KNOCKOUT,
	MOD_ENERGY,
	MOD_ENERGY_SPLASH,
	MOD_WATER,
	MOD_SLIME,
	MOD_LAVA,
	MOD_CRUSH,
	MOD_IMPACT,
	MOD_FALLING,
	MOD_SUICIDE,
	MOD_TRIGGER_HURT,

	NUM_MODS,

} meansOfDeath_t;


//---------------------------------------------------------

// gitem_t->type
typedef enum
{
	IT_BAD,
	IT_WEAPON,
	IT_AMMO,
	IT_ARMOR,
	IT_HEALTH,
	IT_HOLDABLE,
	IT_BATTERY,
	IT_HOLOCRON,

} itemType_t;



typedef struct gitem_s {
	char		*classname;	// spawning name
	char		*pickup_sound;
	char		*world_model;

	char		*icon;

	int			quantity;		// for ammo how much, or duration of powerup
	itemType_t  giType;			// IT_* flags

	int			giTag;

	char		*precaches;		// string of all models and images this item will use
	char		*sounds;		// string of all sounds this item will use
	vec3_t		mins;			// Bbox
	vec3_t		maxs;			// Bbox
} gitem_t;

// included in both the game dll and the client
extern	gitem_t	bg_itemlist[];
extern	const int		bg_numItems;


//==============================================================================

/*
typedef struct ginfoitem_s
{
	char				*infoString;// Text message
	vec3_t				color;		// Text color

} ginfoitem_t;
*/

//==============================================================================

extern weaponData_t weaponData[WP_NUM_WEAPONS];

//==============================================================================
extern ammoData_t ammoData[AMMO_MAX];

//==============================================================================

gitem_t	*FindItem( const char *className );
gitem_t	*FindItemForWeapon( weapon_t weapon );
gitem_t	*FindItemForInventory( int inv );

#define	ITEM_INDEX(x) ((x)-bg_itemlist)

qboolean	BG_CanItemBeGrabbed( const entityState_t *ent, const playerState_t *ps );


// content masks
#define	MASK_ALL				(-1)
#define	MASK_SOLID				(CONTENTS_SOLID)
#define	MASK_PLAYERSOLID		(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_BODY)
#define	MASK_NPCSOLID			(CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_BODY)
#define	MASK_DEADSOLID			(CONTENTS_SOLID|CONTENTS_PLAYERCLIP)
#define	MASK_WATER				(CONTENTS_WATER|CONTENTS_LAVA|CONTENTS_SLIME)
#define	MASK_OPAQUE				(CONTENTS_OPAQUE|CONTENTS_SLIME|CONTENTS_LAVA)//was CONTENTS_SOLID, not CONTENTS_OPAQUE...?
/*
Ghoul2 Insert Start
*/
#define	MASK_SHOT				(CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_CORPSE|CONTENTS_SHOTCLIP)
/*
Ghoul2 Insert End
*/

//
// entityState_t->eType
//
typedef enum {
	ET_GENERAL,
	ET_PLAYER,
	ET_ITEM,
	ET_MISSILE,
	ET_MOVER,
	ET_BEAM,
	ET_PORTAL,
	ET_SPEAKER,
	ET_PUSH_TRIGGER,
	ET_TELEPORT_TRIGGER,
	ET_INVISIBLE,
	ET_THINKER,
	ET_CLOUD, // dumb

	ET_EVENTS				// any of the EV_* events can be added freestanding
							// by setting eType to ET_EVENTS + eventNum
							// this avoids having to set eFlags and eventNum
} entityType_t;



void	EvaluateTrajectory( const trajectory_t *tr, int atTime, vec3_t result );
void	EvaluateTrajectoryDelta( const trajectory_t *tr, int atTime, vec3_t result );

void AddEventToPlayerstate( int newEvent, int eventParm, playerState_t *ps );
int	CurrentPlayerstateEvent( playerState_t *ps );

void PlayerStateToEntityState( playerState_t *ps, entityState_t *s );

qboolean	BG_PlayerTouchesItem( playerState_t *ps, entityState_t *item, int atTime );

#endif//#ifndef __BG_PUBLIC_H__