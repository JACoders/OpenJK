/*
This file is part of Jedi Knight 2.

    Jedi Knight 2 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Jedi Knight 2 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Knight 2.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

// Bowcaster Weapon

#include "cg_local.h"
#include "cg_media.h"
#include "FxScheduler.h"

/*
---------------------------
FX_BowcasterProjectileThink
---------------------------
*/

void FX_BowcasterProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->gent->s.pos.trDelta, forward ) == 0.0f )
	{
		if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
		{
			forward[2] = 1.0f;
		}
	}

	// hack the scale of the forward vector if we were just fired or bounced...this will shorten up the tail for a split second so tails don't clip so harshly
	int dif = cg.time - cent->gent->s.pos.trTime;

	if ( dif < 75 )
	{
		if ( dif < 0 )
		{
			dif = 0;
		}

		float scale = ( dif / 75.0f ) * 0.95f + 0.05f;

		VectorScale( forward, scale, forward );
	}

	theFxScheduler.PlayEffect( cgs.effects.bowcasterShotEffect, cent->lerpOrigin, forward );
}

/*
---------------------------
FX_BowcasterHitWall
---------------------------
*/

void FX_BowcasterHitWall( vec3_t origin, vec3_t normal )
{
	theFxScheduler.PlayEffect( cgs.effects.bowcasterImpactEffect, origin, normal );
}

/*
---------------------------
FX_BowcasterHitPlayer
---------------------------
*/

void FX_BowcasterHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid )
{
	theFxScheduler.PlayEffect( cgs.effects.bowcasterImpactEffect, origin, normal );
}