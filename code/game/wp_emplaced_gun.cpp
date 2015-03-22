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

#include "g_local.h"
#include "b_local.h"
#include "g_functions.h"
#include "wp_saber.h"
#include "w_local.h"

//---------------------------------------------------------
void WP_FireTurboLaserMissile( gentity_t *ent, vec3_t start, vec3_t dir )
//---------------------------------------------------------
{
	int velocity	= ent->mass; //FIXME: externalize
	gentity_t *missile;

	missile = CreateMissile( start, dir, velocity, 10000, ent, qfalse );
	
	//use a custom shot effect
	//missile->s.otherEntityNum2 = G_EffectIndex( "turret/turb_shot" );
	//use a custom impact effect
	//missile->s.emplacedOwner = G_EffectIndex( "turret/turb_impact" );

	missile->classname = "turbo_proj";
	missile->s.weapon = WP_TIE_FIGHTER;

	missile->damage = ent->damage;		//FIXME: externalize
	missile->splashDamage = ent->splashDamage;	//FIXME: externalize
	missile->splashRadius = ent->splashRadius;	//FIXME: externalize
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_EMPLACED; //MOD_TURBLAST; //count as a heavy weap
	missile->splashMethodOfDeath = MOD_EMPLACED; //MOD_TURBLAST;// ?SPLASH;
	missile->clipmask = MASK_SHOT;

	// we don't want it to bounce forever
	missile->bounceCount = 8;

	//set veh as cgame side owner for purpose of fx overrides
	//missile->s.owner = ent->s.number;

	//don't let them last forever
	missile->e_ThinkFunc = thinkF_G_FreeEntity;
	missile->nextthink = level.time + 10000;//at 20000 speed, that should be more than enough
}

// Emplaced Gun
//---------------------------------------------------------
void WP_EmplacedFire( gentity_t *ent )
//---------------------------------------------------------
{
	float damage = weaponData[WP_EMPLACED_GUN].damage * ( ent->NPC ? 0.1f : 1.0f );
	float vel = EMPLACED_VEL * ( ent->NPC ? 0.4f : 1.0f );

	WP_MissileTargetHint(ent, muzzle, forwardVec);

	gentity_t	*missile = CreateMissile( muzzle, forwardVec, vel, 10000, ent );

	missile->classname = "emplaced_proj";
	missile->s.weapon = WP_EMPLACED_GUN;

	missile->damage = damage; 
	missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_HEAVY_WEAP_CLASS;
	missile->methodOfDeath = MOD_EMPLACED;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// do some weird switchery on who the real owner is, we do this so the projectiles don't hit the gun object
	if ( ent && ent->client && !(ent->client->ps.eFlags&EF_LOCKED_TO_WEAPON) )
	{
		missile->owner = ent;
	}
	else
	{
		missile->owner = ent->owner;
	}

	if ( missile->owner->e_UseFunc == useF_eweb_use )
	{
		missile->alt_fire = qtrue;
	}

	VectorSet( missile->maxs, EMPLACED_SIZE, EMPLACED_SIZE, EMPLACED_SIZE );
	VectorScale( missile->maxs, -1, missile->mins );

	// alternate muzzles
	ent->fxID = !ent->fxID;
}