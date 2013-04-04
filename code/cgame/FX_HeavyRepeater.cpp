// Heavy Repeater Weapon

// this line must stay at top so the whole PCH thing works...
#include "cg_headers.h"

//#include "cg_local.h"
#include "cg_media.h"
#include "FxScheduler.h"

/*
---------------------------
FX_RepeaterProjectileThink
---------------------------
*/

void FX_RepeaterProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	theFxScheduler.PlayEffect( "repeater/projectile", cent->lerpOrigin, forward );
}

/*
------------------------
FX_RepeaterHitWall
------------------------
*/

void FX_RepeaterHitWall( vec3_t origin, vec3_t normal )
{
	theFxScheduler.PlayEffect( "repeater/wall_impact", origin, normal );
}

/*
------------------------
FX_RepeaterHitPlayer
------------------------
*/

void FX_RepeaterHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid )
{
	theFxScheduler.PlayEffect( "repeater/wall_impact", origin, normal );
//	theFxScheduler.PlayEffect( "repeater/flesh_impact", origin, normal );
}

/*
------------------------------
FX_RepeaterAltProjectileThink
-----------------------------
*/

void FX_RepeaterAltProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	theFxScheduler.PlayEffect( "repeater/alt_projectile", cent->lerpOrigin, forward );
//	theFxScheduler.PlayEffect( "repeater/alt_projectile", cent->lerpOrigin, forward );
}

/*
------------------------
FX_RepeaterAltHitWall
------------------------
*/

void FX_RepeaterAltHitWall( vec3_t origin, vec3_t normal )
{
	theFxScheduler.PlayEffect( "repeater/concussion", origin, normal );
//	theFxScheduler.PlayEffect( "repeater/alt_wall_impact2", origin, normal );
}

/*
------------------------
FX_RepeaterAltHitPlayer
------------------------
*/

void FX_RepeaterAltHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid )
{
	theFxScheduler.PlayEffect( "repeater/concussion", origin );
//	theFxScheduler.PlayEffect( "repeater/alt_wall_impact2", origin, normal );
}