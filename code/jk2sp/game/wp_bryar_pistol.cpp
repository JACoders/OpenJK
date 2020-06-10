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

#include "g_headers.h"

#include "b_local.h"
#include "g_local.h"
#include "wp_saber.h"
#include "w_local.h"
#include "g_functions.h"

//---------------
//	Bryar Pistol
//---------------

//---------------------------------------------------------
void WP_FireBryarPistol( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	vec3_t	start;
	int		damage = !alt_fire ? weaponData[ent->s.weapon].damage : weaponData[ent->s.weapon].altDamage;

	VectorCopy( wpMuzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	if ( ent->NPC && ent->NPC->currentAim < 5 )
	{
		vec3_t	angs;

		vectoangles( wpFwd, angs );

		if ( ent->client->NPC_class == CLASS_IMPWORKER )
		{//*sigh*, hack to make impworkers less accurate without affecteing imperial officer accuracy
			angs[PITCH] += ( Q_flrand(-1.0f, 1.0f) * (BLASTER_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f));//was 0.5f
			angs[YAW]	+= ( Q_flrand(-1.0f, 1.0f) * (BLASTER_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f));//was 0.5f
		}
		else
		{
			angs[PITCH] += ( Q_flrand(-1.0f, 1.0f) * ((5-ent->NPC->currentAim)*0.25f) );
			angs[YAW]	+= ( Q_flrand(-1.0f, 1.0f) * ((5-ent->NPC->currentAim)*0.25f) );
		}

		AngleVectors( angs, wpFwd, NULL, NULL );
	}

	gentity_t	*missile = CreateMissile( start, wpFwd, BRYAR_PISTOL_VEL, 10000, ent, alt_fire );

	missile->classname = "bryar_proj";
	missile->s.weapon = WP_BRYAR_PISTOL;

	if ( alt_fire )
	{
		int count = ( level.time - ent->client->ps.weaponChargeTime ) / BRYAR_CHARGE_UNIT;

		if ( count < 1 )
		{
			count = 1;
		}
		else if ( count > 5 )
		{
			count = 5;
		}

		damage *= count;
		missile->count = count; // this will get used in the projectile rendering code to make a beefier effect
	}

//	if ( ent->client && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > 0 && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > cg.time )
//	{
//		// in overcharge mode, so doing double damage
//		missile->flags |= FL_OVERCHARGED;
//		damage *= 2;
//	}

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;

	if ( alt_fire )
	{
		missile->methodOfDeath = MOD_BRYAR_ALT;
	}
	else
	{
		missile->methodOfDeath = MOD_BRYAR;
	}

	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}