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

// Rocket Launcher Weapon

#include "cg_local.h"

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

	trap->FX_PlayEffectID( cgs.effects.rocketShotEffect, cent->lerpOrigin, forward, -1, -1, qfalse );
}

/*
---------------------------
FX_RocketHitWall
---------------------------
*/

void FX_RocketHitWall( vec3_t origin, vec3_t normal )
{
	trap->FX_PlayEffectID( cgs.effects.rocketExplosionEffect, origin, normal, -1, -1, qfalse );
}

/*
---------------------------
FX_RocketHitPlayer
---------------------------
*/

void FX_RocketHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid )
{
	trap->FX_PlayEffectID( cgs.effects.rocketExplosionEffect, origin, normal, -1, -1, qfalse );
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

	trap->FX_PlayEffectID( cgs.effects.rocketShotEffect, cent->lerpOrigin, forward, -1, -1, qfalse );
}
