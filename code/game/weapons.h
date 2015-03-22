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

// Filename:-	weapons.h
//
// Note that this is now included from both server and game modules, so don't include any other header files
//	within this one that might break stuff...

#ifndef __WEAPONS_H__
#define __WEAPONS_H__

#include "../qcommon/q_shared.h"

typedef enum //# weapon_e
{
	WP_NONE,

	// Player weapons
	WP_SABER,			// player and NPC weapon
	WP_BLASTER_PISTOL,	// player and NPC weapon
	WP_BLASTER,			// player and NPC weapon
	WP_DISRUPTOR,		// player and NPC weapon
	WP_BOWCASTER,		// NPC weapon - player can pick this up, but never starts with them
	WP_REPEATER,		// NPC weapon - player can pick this up, but never starts with them
	WP_DEMP2,			// NPC weapon - player can pick this up, but never starts with them
	WP_FLECHETTE,		// NPC weapon - player can pick this up, but never starts with them
	WP_ROCKET_LAUNCHER,	// NPC weapon - player can pick this up, but never starts with them
	WP_THERMAL,			// player and NPC weapon
	WP_TRIP_MINE,		// NPC weapon - player can pick this up, but never starts with them
	WP_DET_PACK,		// NPC weapon - player can pick this up, but never starts with them
	WP_CONCUSSION,		// NPC weapon - player can pick this up, but never starts with them

	//extras
	WP_MELEE,			// player and NPC weapon - Any ol' melee attack

	//when in atst
	WP_ATST_MAIN,
	WP_ATST_SIDE,

	// These can never be gotten directly by the player
	WP_STUN_BATON,		// stupid weapon, should remove
	
	//NPC weapons
	WP_BRYAR_PISTOL,	// NPC weapon - player can pick this up, but never starts with them

	WP_EMPLACED_GUN,

	WP_BOT_LASER,		// Probe droid	- Laser blast

	WP_TURRET,			// turret guns 

	WP_TIE_FIGHTER,

	WP_RAPID_FIRE_CONC,

	WP_JAWA,
	WP_TUSKEN_RIFLE,
	WP_TUSKEN_STAFF,
	WP_SCEPTER,
	WP_NOGHRI_STICK,

	//# #eol
	WP_NUM_WEAPONS
} weapon_t;

#define FIRST_WEAPON		WP_SABER		// this is the first weapon for next and prev weapon switching
#define MAX_PLAYER_WEAPONS	WP_STUN_BATON	// this is the max you can switch to and get with the give all. - FIXME: it's actually this one *minus* one... why?

// AMMO_NONE must be first and AMMO_MAX must be last, cause weapon load validates based off of these vals
typedef enum //# ammo_e
{
	AMMO_NONE,
	AMMO_FORCE,		// AMMO_PHASER
	AMMO_BLASTER,	// AMMO_STARFLEET,
	AMMO_POWERCELL,	// AMMO_ALIEN,
	AMMO_METAL_BOLTS,
	AMMO_ROCKETS,
	AMMO_EMPLACED,
	AMMO_THERMAL,
	AMMO_TRIPMINE,
	AMMO_DETPACK,
	AMMO_MAX
} ammo_t;


typedef struct weaponData_s
{
	char	classname[32];		// Spawning name
	char	weaponMdl[64];		// Weapon Model
	char	firingSnd[64];		// Sound made when fired
	char	altFiringSnd[64];	// Sound made when alt-fired
//	char	flashSnd[64];		// Sound made by flash
//	char	altFlashSnd[64];	// Sound made by an alt-flash
	char	stopSnd[64];		// Sound made when weapon stops firing
	char	chargeSnd[64];		// sound to start when the weapon initiates the charging sequence
	char	altChargeSnd[64];	// alt sound to start when the weapon initiates the charging sequence
	char	selectSnd[64];		// the sound to play when this weapon gets selected

	int		ammoIndex;			// Index to proper ammo slot
	int		ammoLow;			// Count when ammo is low

	int		energyPerShot;		// Amount of energy used per shot
	int		fireTime;			// Amount of time between firings
	int		range;				// Range of weapon
	
	int		altEnergyPerShot;	// Amount of energy used for alt-fire
	int		altFireTime;		// Amount of time between alt-firings
	int		altRange;			// Range of alt-fire

	char	weaponIcon[64];		// Name of weapon icon file
	int		numBarrels;			// how many barrels should we expect for this weapon?

	char	missileMdl[64];		// Missile Model
	char	missileSound[64];	// Missile flight sound
	float  	missileDlight;		// what is says
	vec3_t 	missileDlightColor;	// ditto

	char	alt_missileMdl[64];		// Missile Model
	char	alt_missileSound[64];	// Missile sound
	float  	alt_missileDlight;		// what is says
	vec3_t 	alt_missileDlightColor;	// ditto

	char	missileHitSound[64];	// Missile impact sound
	char	altmissileHitSound[64];	// alt Missile impact sound
	void	*func;
	void	*altfunc;

	char	mMuzzleEffect[64];
	int		mMuzzleEffectID;
	char	mAltMuzzleEffect[64];
	int		mAltMuzzleEffectID;

	int		damage;
	int		altDamage;
	int		splashDamage;
	int		altSplashDamage;
	float	splashRadius;
	float	altSplashRadius;

} weaponData_t;


typedef struct ammoData_s
{
	char	icon[64];	// Name of ammo icon file
	int		max;		// Max amount player can hold of ammo
} ammoData_t;

// Bryar Pistol
//--------
#define BRYAR_PISTOL_VEL			1800
#define BRYAR_PISTOL_DAMAGE			14
#define BRYAR_CHARGE_UNIT			200.0f	// bryar charging gives us one more unit every 200ms--if you change this, you'll have to do the same in bg_pmove

// E11 Blaster
//---------
#define BLASTER_MAIN_SPREAD			0.5f
#define BLASTER_ALT_SPREAD			1.5f
#define BLASTER_NPC_SPREAD			0.5f
#define BLASTER_VELOCITY			2300
#define BLASTER_NPC_VEL_CUT			0.5f
#define BLASTER_NPC_HARD_VEL_CUT	0.7f
#define BLASTER_DAMAGE				20
#define	BLASTER_NPC_DAMAGE_EASY		6
#define	BLASTER_NPC_DAMAGE_NORMAL	12 // 14
#define	BLASTER_NPC_DAMAGE_HARD		16 // 18

// Tenloss Disruptor
//----------
#define DISRUPTOR_MAIN_DAMAGE			14
#define DISRUPTOR_NPC_MAIN_DAMAGE_EASY	5
#define DISRUPTOR_NPC_MAIN_DAMAGE_MEDIUM	10
#define DISRUPTOR_NPC_MAIN_DAMAGE_HARD	15

#define DISRUPTOR_ALT_DAMAGE			12
#define DISRUPTOR_NPC_ALT_DAMAGE_EASY	15
#define DISRUPTOR_NPC_ALT_DAMAGE_MEDIUM	25
#define DISRUPTOR_NPC_ALT_DAMAGE_HARD	30
#define DISRUPTOR_ALT_TRACES			3		// can go through a max of 3 entities
#define DISRUPTOR_CHARGE_UNIT			150.0f	// distruptor charging gives us one more unit every 150ms--if you change this, you'll have to do the same in bg_pmove

// Wookie Bowcaster
//----------
#define	BOWCASTER_DAMAGE			45
#define	BOWCASTER_VELOCITY			1300
#define	BOWCASTER_NPC_DAMAGE_EASY	12
#define	BOWCASTER_NPC_DAMAGE_NORMAL	24
#define	BOWCASTER_NPC_DAMAGE_HARD	36
#define BOWCASTER_SPLASH_DAMAGE		0
#define BOWCASTER_SPLASH_RADIUS		0
#define BOWCASTER_SIZE				2

#define BOWCASTER_ALT_SPREAD		5.0f
#define BOWCASTER_VEL_RANGE			0.3f
#define BOWCASTER_CHARGE_UNIT		200.0f	// bowcaster charging gives us one more unit every 200ms--if you change this, you'll have to do the same in bg_pmove

// Heavy Repeater
//----------
#define REPEATER_SPREAD				1.4f
#define REPEATER_NPC_SPREAD			0.7f
#define	REPEATER_DAMAGE				8
#define	REPEATER_VELOCITY			1600
#define	REPEATER_NPC_DAMAGE_EASY	2
#define	REPEATER_NPC_DAMAGE_NORMAL	4
#define	REPEATER_NPC_DAMAGE_HARD	6

#define REPEATER_ALT_SIZE				3	// half of bbox size
#define	REPEATER_ALT_DAMAGE				60
#define REPEATER_ALT_SPLASH_DAMAGE		60
#define REPEATER_ALT_SPLASH_RADIUS		128
#define	REPEATER_ALT_VELOCITY			1100
#define	REPEATER_ALT_NPC_DAMAGE_EASY	15
#define	REPEATER_ALT_NPC_DAMAGE_NORMAL	30
#define	REPEATER_ALT_NPC_DAMAGE_HARD	45

// DEMP2
//----------
#define	DEMP2_DAMAGE				15
#define	DEMP2_VELOCITY				1800
#define	DEMP2_NPC_DAMAGE_EASY		6
#define	DEMP2_NPC_DAMAGE_NORMAL		12
#define	DEMP2_NPC_DAMAGE_HARD		18
#define	DEMP2_SIZE					2		// half of bbox size

#define DEMP2_ALT_DAMAGE			15
#define DEMP2_CHARGE_UNIT			500.0f	// demp2 charging gives us one more unit every 500ms--if you change this, you'll have to do the same in bg_pmove
#define DEMP2_ALT_RANGE				4096
#define DEMP2_ALT_SPLASHRADIUS		256

// Golan Arms Flechette
//---------
#define FLECHETTE_SHOTS				6
#define FLECHETTE_SPREAD			4.0f
#define FLECHETTE_DAMAGE			15
#define FLECHETTE_VEL				3500
#define FLECHETTE_SIZE				1

#define FLECHETTE_ALT_DAMAGE		20
#define FLECHETTE_ALT_SPLASH_DAM	20
#define FLECHETTE_ALT_SPLASH_RAD	128

// NOT CURRENTLY USED
#define FLECHETTE_MINE_RADIUS_CHECK		200
#define FLECHETTE_MINE_VEL				1000
#define FLECHETTE_MINE_DAMAGE			100
#define FLECHETTE_MINE_SPLASH_DAMAGE	200
#define FLECHETTE_MINE_SPLASH_RADIUS	200

// Personal Rocket Launcher
//---------
#define	ROCKET_VELOCITY				900
#define	ROCKET_DAMAGE				100
#define	ROCKET_SPLASH_DAMAGE		100
#define	ROCKET_SPLASH_RADIUS		160
#define ROCKET_NPC_DAMAGE_EASY		20
#define ROCKET_NPC_DAMAGE_NORMAL	40
#define ROCKET_NPC_DAMAGE_HARD		60
#define ROCKET_SIZE					3

#define	ROCKET_ALT_VELOCITY			(ROCKET_VELOCITY*0.5)
#define ROCKET_ALT_THINK_TIME		100

// Concussion Rifle
//---------
//primary
#define	CONC_VELOCITY				3000
#define	CONC_DAMAGE					150
#define CONC_NPC_SPREAD				0.7f
#define	CONC_NPC_DAMAGE_EASY		15
#define	CONC_NPC_DAMAGE_NORMAL		30
#define	CONC_NPC_DAMAGE_HARD		50
#define	CONC_SPLASH_DAMAGE			50
#define	CONC_SPLASH_RADIUS			300
//alt
#define CONC_ALT_DAMAGE				225//100
#define CONC_ALT_NPC_DAMAGE_EASY	10
#define CONC_ALT_NPC_DAMAGE_MEDIUM	20
#define CONC_ALT_NPC_DAMAGE_HARD	30

// Emplaced Gun
//--------------
#define EMPLACED_VEL				6000	// very fast
#define EMPLACED_DAMAGE				150		// and very damaging
#define EMPLACED_SIZE				5		// make it easier to hit things

// ATST Main Gun
//--------------
#define ATST_MAIN_VEL				4000	// 
#define ATST_MAIN_DAMAGE			25		// 
#define ATST_MAIN_SIZE				3		// make it easier to hit things

// ATST Side Gun
//---------------
#define ATST_SIDE_MAIN_DAMAGE				75
#define ATST_SIDE_MAIN_VELOCITY				1300
#define ATST_SIDE_MAIN_NPC_DAMAGE_EASY		30
#define ATST_SIDE_MAIN_NPC_DAMAGE_NORMAL	40
#define ATST_SIDE_MAIN_NPC_DAMAGE_HARD		50
#define ATST_SIDE_MAIN_SIZE					4
#define ATST_SIDE_MAIN_SPLASH_DAMAGE		10	// yeah, pretty small, either zero out or make it worth having?
#define ATST_SIDE_MAIN_SPLASH_RADIUS		16	// yeah, pretty small, either zero out or make it worth having?

#define ATST_SIDE_ALT_VELOCITY				1100
#define ATST_SIDE_ALT_NPC_VELOCITY			600
#define ATST_SIDE_ALT_DAMAGE				130

#define ATST_SIDE_ROCKET_NPC_DAMAGE_EASY	30
#define ATST_SIDE_ROCKET_NPC_DAMAGE_NORMAL	50
#define ATST_SIDE_ROCKET_NPC_DAMAGE_HARD	90

#define	ATST_SIDE_ALT_SPLASH_DAMAGE			130
#define	ATST_SIDE_ALT_SPLASH_RADIUS			200
#define ATST_SIDE_ALT_ROCKET_SIZE			5
#define ATST_SIDE_ALT_ROCKET_SPLASH_SCALE	0.5f	// scales splash for NPC's

// Stun Baton
//--------------
#define STUN_BATON_DAMAGE			22
#define STUN_BATON_ALT_DAMAGE		22
#define STUN_BATON_RANGE			25

// Laser Trip Mine
//--------------
#define LT_DAMAGE			150
#define LT_SPLASH_RAD		256.0f
#define LT_SPLASH_DAM		90

#define LT_VELOCITY			250.0f
#define LT_ALT_VELOCITY		1000.0f

#define PROX_MINE_RADIUS_CHECK		190

#define LT_SIZE				3.0f
#define LT_ALT_TIME			2000
#define	LT_ACTIVATION_DELAY	1000
#define	LT_DELAY_TIME		50

// Thermal Detonator
//--------------
#define TD_DAMAGE			100
#define TD_NPC_DAMAGE_CUT	0.6f	// NPC thrown dets deliver only 60% of the damage that a player thrown one does
#define TD_SPLASH_RAD		128
#define TD_SPLASH_DAM		90
#define TD_VELOCITY			900
#define TD_MIN_CHARGE		0.15f
#define TD_TIME				4000
#define TD_THINK_TIME		300		// don't think too often?
#define TD_TEST_RAD			(TD_SPLASH_RAD * 0.8f) // no sense in auto-blowing up if exactly on the radius edge--it would hardly do any damage
#define TD_ALT_TIME			3000

#define TD_ALT_DAMAGE		100
#define TD_ALT_SPLASH_RAD	128
#define TD_ALT_SPLASH_DAM	90
#define TD_ALT_VELOCITY		600
#define TD_ALT_MIN_CHARGE	0.15f
#define TD_ALT_TIME			3000

// Tusken Rifle Shot
//--------------
#define TUSKEN_RIFLE_VEL			3000	// fast
#define TUSKEN_RIFLE_DAMAGE_EASY	20		// damaging
#define TUSKEN_RIFLE_DAMAGE_MEDIUM	30		// very damaging
#define TUSKEN_RIFLE_DAMAGE_HARD	50		// extremely damaging


#endif//#ifndef __WEAPONS_H__
