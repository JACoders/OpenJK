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

// Rocket Launcher Weapon

#include "cg_local.h"
#include "cg_media.h"
#include "FxScheduler.h"

/*
---------------------------
FX_RocketProjectileThink
---------------------------
*/

void FX_RocketProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	theFxScheduler.PlayEffect( "rocket/shot", cent->lerpOrigin, forward );
}

/*
---------------------------
FX_RocketHitWall
---------------------------
*/

void FX_RocketHitWall( vec3_t origin, vec3_t normal )
{
	theFxScheduler.PlayEffect( "rocket/explosion", origin, normal );
}

/*
---------------------------
FX_RocketHitPlayer
---------------------------
*/

void FX_RocketHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid )
{
	theFxScheduler.PlayEffect( "rocket/explosion", origin, normal );
}

/*
---------------------------
FX_RocketAltProjectileThink
---------------------------
*/

void FX_RocketAltProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	theFxScheduler.PlayEffect( "rocket/shot", cent->lerpOrigin, forward );
}
