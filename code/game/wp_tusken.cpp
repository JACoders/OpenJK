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

#include "g_local.h"
#include "b_local.h"
#include "g_functions.h"
#include "wp_saber.h"
#include "w_local.h"

//---------------------------------------------------------
void WP_FireTuskenRifle( gentity_t *ent )
//---------------------------------------------------------
{
	vec3_t	start;

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	if ( !(ent->client->ps.forcePowersActive&(1<<FP_SEE))
		|| ent->client->ps.forcePowerLevel[FP_SEE] < FORCE_LEVEL_2 )
	{//force sight 2+ gives perfect aim
		//FIXME: maybe force sight level 3 autoaims some?
		if ( ent->NPC && ent->NPC->currentAim < 5 )
		{
			vec3_t	angs;

			vectoangles( forwardVec, angs );

			if ( ent->client->NPC_class == CLASS_IMPWORKER )
			{//*sigh*, hack to make impworkers less accurate without affecteing imperial officer accuracy
				angs[PITCH] += ( crandom() * (BLASTER_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f));//was 0.5f
				angs[YAW]	+= ( crandom() * (BLASTER_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f));//was 0.5f
			}
			else
			{
				angs[PITCH] += ( crandom() * ((5-ent->NPC->currentAim)*0.25f) );
				angs[YAW]	+= ( crandom() * ((5-ent->NPC->currentAim)*0.25f) );
			}

			AngleVectors( angs, forwardVec, NULL, NULL );
		}
	}

	WP_MissileTargetHint(ent, start, forwardVec);

	gentity_t	*missile = CreateMissile( start, forwardVec, TUSKEN_RIFLE_VEL, 10000, ent, qfalse );

	missile->classname = "trifle_proj";
	missile->s.weapon = WP_TUSKEN_RIFLE;

	if ( ent->s.number < MAX_CLIENTS || g_spskill->integer >= 2 )
	{
		missile->damage = TUSKEN_RIFLE_DAMAGE_HARD;
	}
	else if ( g_spskill->integer > 0 )
	{
		missile->damage = TUSKEN_RIFLE_DAMAGE_MEDIUM;
	}
	else
	{
		missile->damage = TUSKEN_RIFLE_DAMAGE_EASY;
	}
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;

	missile->methodOfDeath = MOD_BRYAR;//???

	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}