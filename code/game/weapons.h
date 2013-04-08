/*
This file is part of Jedi Academy.

    Jedi Academy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Jedi Academy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Academy.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

// Filename:-	weapons.h
//
// Note that this is now included from both server and game modules, so don't include any other header files
//	within this one that might break stuff...

#ifndef __WEAPONS_H__
#define __WEAPONS_H__

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
#ifndef _USRDLL
	void	*func;
	void	*altfunc;

	char	mMuzzleEffect[64];
	int		mMuzzleEffectID;
	char	mAltMuzzleEffect[64];
	int		mAltMuzzleEffectID;

#endif

} weaponData_t;


typedef struct ammoData_s
{
	char	icon[32];	// Name of ammo icon file
	int		max;		// Max amount player can hold of ammo
} ammoData_t;


#endif//#ifndef __WEAPONS_H__
