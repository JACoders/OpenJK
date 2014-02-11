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

// Bowcaster Weapon

// this line must stay at top so the whole PCH thing works...
#include "cg_headers.h"

#include "cg_media.h"
#include "FxScheduler.h"


/*
---------------------------
FX_ATSTMainProjectileThink
---------------------------
*/
void FX_ATSTMainProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon )
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

	if ( dif < 30 )
	{
		if ( dif < 0 )
		{
			dif = 0;
		}

		float scale = ( dif / 30.0f ) * 0.95f + 0.05f;

		VectorScale( forward, scale, forward );
	}

	theFxScheduler.PlayEffect( "atst/shot", cent->lerpOrigin, forward );
}

/*
---------------------------
FX_ATSTMainHitWall
---------------------------
*/
void FX_ATSTMainHitWall( vec3_t origin, vec3_t normal )
{
	theFxScheduler.PlayEffect( "atst/wall_impact", origin, normal );
}

/*
---------------------------
FX_ATSTMainHitPlayer
---------------------------
*/
void FX_ATSTMainHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid )
{
	if ( humanoid )
	{
		theFxScheduler.PlayEffect( "atst/flesh_impact", origin, normal );
	}
	else
	{
		theFxScheduler.PlayEffect( "atst/droid_impact", origin, normal );
	}
}

/*
---------------------------
FX_ATSTSideAltProjectileThink
---------------------------
*/
void FX_ATSTSideAltProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	theFxScheduler.PlayEffect( "atst/side_alt_shot", cent->lerpOrigin, forward );
}

/*
---------------------------
FX_ATSTSideMainProjectileThink
---------------------------
*/
void FX_ATSTSideMainProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	theFxScheduler.PlayEffect( "atst/side_main_shot", cent->lerpOrigin, forward );
}
