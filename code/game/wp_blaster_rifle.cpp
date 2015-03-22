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


//---------------
//	Blaster
//---------------

//---------------------------------------------------------
void WP_FireBlasterMissile( gentity_t *ent, vec3_t start, vec3_t dir, qboolean altFire )
//---------------------------------------------------------
{
	int velocity	= BLASTER_VELOCITY;
	int	damage		= altFire ? weaponData[WP_BLASTER].altDamage : weaponData[WP_BLASTER].damage;

	if ( ent && ent->client && ent->client->NPC_class == CLASS_VEHICLE )
	{
		damage *= 3;
		velocity = ATST_MAIN_VEL + ent->client->ps.speed;
	}
	else
	{
		// If an enemy is shooting at us, lower the velocity so you have a chance to evade
		if ( ent->client && ent->client->ps.clientNum != 0 && ent->client->NPC_class != CLASS_BOBAFETT )
		{
			if ( g_spskill->integer < 2 )
			{		
				velocity *= BLASTER_NPC_VEL_CUT;
			}
			else
			{
				velocity *= BLASTER_NPC_HARD_VEL_CUT;
			}
		}
	}

	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	WP_MissileTargetHint(ent, start, dir);

	gentity_t *missile = CreateMissile( start, dir, velocity, 10000, ent, altFire );

	missile->classname = "blaster_proj";
	missile->s.weapon = WP_BLASTER;

	// Do the damages
	if ( ent->s.number != 0 && ent->client->NPC_class != CLASS_BOBAFETT )
	{
		if ( g_spskill->integer == 0 )
		{
			damage = BLASTER_NPC_DAMAGE_EASY;
		}
		else if ( g_spskill->integer == 1 )
		{
			damage = BLASTER_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = BLASTER_NPC_DAMAGE_HARD;
		}
	}

//	if ( ent->client )
//	{
//		if ( ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > 0 && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > cg.time )
//		{
//			// in overcharge mode, so doing double damage
//			missile->flags |= FL_OVERCHARGED;
//			damage *= 2;
//		}
//	}

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	if ( altFire )
	{
		missile->methodOfDeath = MOD_BLASTER_ALT;
	}
	else
	{
		missile->methodOfDeath = MOD_BLASTER;
	}
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

//---------------------------------------------------------
void WP_FireBlaster( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	vec3_t	dir, angs;

	vectoangles( forwardVec, angs );

	if ( ent->client && ent->client->NPC_class == CLASS_VEHICLE )
	{//no inherent aim screw up
	}
	else if ( !(ent->client->ps.forcePowersActive&(1<<FP_SEE))
		|| ent->client->ps.forcePowerLevel[FP_SEE] < FORCE_LEVEL_2 )
	{//force sight 2+ gives perfect aim
		//FIXME: maybe force sight level 3 autoaims some?
		if ( alt_fire )
		{
			// add some slop to the alt-fire direction
			angs[PITCH] += crandom() * BLASTER_ALT_SPREAD;
			angs[YAW]	+= crandom() * BLASTER_ALT_SPREAD;
		}
		else
		{
			// Troopers use their aim values as well as the gun's inherent inaccuracy
			// so check for all classes of stormtroopers and anyone else that has aim error
			if ( ent->client && ent->NPC &&
				( ent->client->NPC_class == CLASS_STORMTROOPER ||
				ent->client->NPC_class == CLASS_SWAMPTROOPER ) )
			{
				angs[PITCH] += ( crandom() * (BLASTER_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f));//was 0.5f
				angs[YAW]	+= ( crandom() * (BLASTER_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f));//was 0.5f
			}
			else
			{
				// add some slop to the main-fire direction
				angs[PITCH] += crandom() * BLASTER_MAIN_SPREAD;
				angs[YAW]	+= crandom() * BLASTER_MAIN_SPREAD;
			}
		}
	}

	AngleVectors( angs, dir, NULL, NULL );

	// FIXME: if temp_org does not have clear trace to inside the bbox, don't shoot!
	WP_FireBlasterMissile( ent, muzzle, dir, alt_fire );
}