// Bowcaster Weapon

#include "cg_local.h"

/*
---------------------------
FX_BowcasterProjectileThink
---------------------------
*/

void FX_BowcasterProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	trap_FX_PlayEffectID( cgs.effects.bowcasterShotEffect, cent->lerpOrigin, forward, -1, -1 );
}

/*
---------------------------
FX_BowcasterHitWall
---------------------------
*/

void FX_BowcasterHitWall( vec3_t origin, vec3_t normal )
{
	trap_FX_PlayEffectID( cgs.effects.bowcasterImpactEffect, origin, normal, -1, -1 );
}

/*
---------------------------
FX_BowcasterHitPlayer
---------------------------
*/

void FX_BowcasterHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid )
{
	trap_FX_PlayEffectID( cgs.effects.bowcasterImpactEffect, origin, normal, -1, -1 );
}

/*
------------------------------
FX_BowcasterAltProjectileThink
------------------------------
*/

void FX_BowcasterAltProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	trap_FX_PlayEffectID( cgs.effects.bowcasterShotEffect, cent->lerpOrigin, forward, -1, -1 );
}

