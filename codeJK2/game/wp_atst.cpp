/*
This file is part of OpenJK.

    OpenJK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    OpenJK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenJK.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2013 OpenJK

#include "g_headers.h"

#include "b_local.h"
#include "g_local.h"
#include "wp_saber.h"
#include "w_local.h"
#include "g_functions.h"

// ATST Main
//---------------------------------------------------------
void WP_ATSTMainFire( gentity_t *ent )
//---------------------------------------------------------
{
	float vel = ATST_MAIN_VEL;

//	if ( ent->client && (ent->client->ps.eFlags & EF_IN_ATST ))
//	{
//		vel = 4500.0f;
//	}

	if ( !ent->s.number )
	{
		// player shoots faster
		vel *= 1.6f;
	}

	gentity_t	*missile = CreateMissile( wpMuzzle, wpFwd, vel, 10000, ent );

	missile->classname = "atst_main_proj";
	missile->s.weapon = WP_ATST_MAIN;

	missile->damage = weaponData[WP_ATST_MAIN].damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK|DAMAGE_HEAVY_WEAP_CLASS;
	missile->methodOfDeath = MOD_ENERGY;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	missile->owner = ent;

	VectorSet( missile->maxs, ATST_MAIN_SIZE, ATST_MAIN_SIZE, ATST_MAIN_SIZE );
	VectorScale( missile->maxs, -1, missile->mins );

}

// ATST Alt Side
//---------------------------------------------------------
void WP_ATSTSideAltFire( gentity_t *ent )
//---------------------------------------------------------
{
	int	damage	= weaponData[WP_ATST_SIDE].altDamage;
	float	vel = ATST_SIDE_ALT_NPC_VELOCITY;

	if ( ent->client && (ent->client->ps.eFlags & EF_IN_ATST ))
	{
		vel = ATST_SIDE_ALT_VELOCITY;
	}

	gentity_t *missile = CreateMissile( wpMuzzle, wpFwd, vel, 10000, ent, qtrue );

	missile->classname = "atst_rocket";
	missile->s.weapon = WP_ATST_SIDE;

	missile->mass = 10;

	// Do the damages
	if ( ent->s.number != 0 )
	{
		if ( g_spskill->integer == 0 )
		{
			damage = ATST_SIDE_ROCKET_NPC_DAMAGE_EASY;
		}
		else if ( g_spskill->integer == 1 )
		{
			damage = ATST_SIDE_ROCKET_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = ATST_SIDE_ROCKET_NPC_DAMAGE_HARD;
		}
	}

	VectorCopy( wpFwd, missile->movedir );

	// Make it easier to hit things
	VectorSet( missile->maxs, ATST_SIDE_ALT_ROCKET_SIZE, ATST_SIDE_ALT_ROCKET_SIZE, ATST_SIDE_ALT_ROCKET_SIZE );
	VectorScale( missile->maxs, -1, missile->mins );

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_HEAVY_WEAP_CLASS;
	missile->methodOfDeath = MOD_EXPLOSIVE;
	missile->splashMethodOfDeath = MOD_EXPLOSIVE_SPLASH;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// Scale damage down a bit if it is coming from an NPC
	missile->splashDamage = weaponData[WP_ATST_SIDE].altSplashDamage * ( ent->s.number == 0 ? 1.0f : ATST_SIDE_ALT_ROCKET_SPLASH_SCALE );
	missile->splashRadius = weaponData[WP_ATST_SIDE].altSplashRadius;

	// we don't want it to ever bounce
	missile->bounceCount = 0;
}

// ATST Side
//---------------------------------------------------------
void WP_ATSTSideFire( gentity_t *ent )
//---------------------------------------------------------
{
	int	damage	= weaponData[WP_ATST_SIDE].damage;

	gentity_t *missile = CreateMissile( wpMuzzle, wpFwd, ATST_SIDE_MAIN_VELOCITY, 10000, ent, qfalse );

	missile->classname = "atst_side_proj";
	missile->s.weapon = WP_ATST_SIDE;

	// Do the damages
	if ( ent->s.number != 0 )
	{
		if ( g_spskill->integer == 0 )
		{
			damage = ATST_SIDE_MAIN_NPC_DAMAGE_EASY;
		}
		else if ( g_spskill->integer == 1 )
		{
			damage = ATST_SIDE_MAIN_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = ATST_SIDE_MAIN_NPC_DAMAGE_HARD;
		}
	}

	VectorSet( missile->maxs, ATST_SIDE_MAIN_SIZE, ATST_SIDE_MAIN_SIZE, ATST_SIDE_MAIN_SIZE );
	VectorScale( missile->maxs, -1, missile->mins );

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK|DAMAGE_HEAVY_WEAP_CLASS;
	missile->methodOfDeath = MOD_ENERGY;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	missile->splashDamage = weaponData[WP_ATST_SIDE].splashDamage * ( ent->s.number == 0 ? 1.0f : 0.6f );
	missile->splashRadius = weaponData[WP_ATST_SIDE].splashRadius;

	// we don't want it to bounce
	missile->bounceCount = 0;
}