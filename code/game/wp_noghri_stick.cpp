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
#include "wp_saber.h"
#include "w_local.h"

//---------------------------------------------------------
void WP_FireNoghriStick( gentity_t *ent )
//---------------------------------------------------------
{
	vec3_t	dir, angs;

	vectoangles( forwardVec, angs );

	if ( !(ent->client->ps.forcePowersActive&(1<<FP_SEE))
		|| ent->client->ps.forcePowerLevel[FP_SEE] < FORCE_LEVEL_2 )
	{//force sight 2+ gives perfect aim
		//FIXME: maybe force sight level 3 autoaims some?
		// add some slop to the main-fire direction
		angs[PITCH] += ( Q_flrand(-1.0f, 1.0f) * (BLASTER_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f));//was 0.5f
		angs[YAW]	+= ( Q_flrand(-1.0f, 1.0f) * (BLASTER_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f));//was 0.5f
	}

	AngleVectors( angs, dir, NULL, NULL );

	// FIXME: if temp_org does not have clear trace to inside the bbox, don't shoot!
	int velocity	= 1200;

	WP_TraceSetStart( ent, muzzle, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	WP_MissileTargetHint(ent, muzzle, dir);

	gentity_t *missile = CreateMissile( muzzle, dir, velocity, 10000, ent, qfalse );

	missile->classname = "noghri_proj";
	missile->s.weapon = WP_NOGHRI_STICK;

	// Do the damages
	if ( ent->s.number != 0 )
	{
		if ( g_spskill->integer == 0 )
		{
			missile->damage = 1;
		}
		else if ( g_spskill->integer == 1 )
		{
			missile->damage = 5;
		}
		else
		{
			missile->damage = 10;
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

	missile->dflags = DAMAGE_NO_KNOCKBACK;
	missile->methodOfDeath = MOD_BLASTER;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
	missile->splashDamage = 0;
	missile->splashRadius = 100;
	missile->splashMethodOfDeath = MOD_GAS;

	//Hmm: maybe spew gas on impact?
}