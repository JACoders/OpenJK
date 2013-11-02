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

// Golan Arms Flechette Weapon

// this line must stay at top so the whole PCH thing works...
#include "cg_headers.h"

#include "cg_media.h"
#include "FxScheduler.h"

/*
-------------------------
FX_FlechetteProjectileThink
-------------------------
*/

void FX_FlechetteProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	EvaluateTrajectoryDelta( &cent->gent->s.pos, cg.time, forward );

	if ( VectorNormalize( forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	theFxScheduler.PlayEffect( cgs.effects.flechetteShotEffect, cent->lerpOrigin, forward );
}

/*
-------------------------
FX_FlechetteWeaponHitWall
-------------------------
*/
void FX_FlechetteWeaponHitWall( vec3_t origin, vec3_t normal )
{
	theFxScheduler.PlayEffect( cgs.effects.flechetteShotDeathEffect, origin, normal );
}

/*
-------------------------
FX_BlasterWeaponHitPlayer
-------------------------
*/
void FX_FlechetteWeaponHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid )
{
//	if ( humanoid )
//	{
		theFxScheduler.PlayEffect( cgs.effects.flechetteFleshImpactEffect, origin, normal );
//	}
//	else
//	{
//		theFxScheduler.PlayEffect( "blaster/droid_impact", origin, normal );
//	}
}

/*
-------------------------
FX_FlechetteProjectileThink
-------------------------
*/

void FX_FlechetteAltProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	theFxScheduler.PlayEffect( cgs.effects.flechetteAltShotEffect, cent->lerpOrigin, forward );
}