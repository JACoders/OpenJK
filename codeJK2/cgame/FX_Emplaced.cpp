// Emplaced Weapon

// this line must stay at top so the whole PCH thing works...
#include "cg_headers.h"

//#include "cg_local.h"
#include "cg_media.h"
#include "FxScheduler.h"

/*
---------------------------
FX_EmplacedProjectileThink
---------------------------
*/

void FX_EmplacedProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon )
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

	if ( cent->gent && cent->gent->owner && cent->gent->owner->activator && cent->gent->owner->activator->s.number > 0 )
	{
		// NPC's do short shot
		theFxScheduler.PlayEffect( "emplaced/shotNPC", cent->lerpOrigin, forward );
	}
	else
	{
		// players do long shot
		theFxScheduler.PlayEffect( "emplaced/shot", cent->lerpOrigin, forward );
	}
}

/*
---------------------------
FX_EmplacedHitWall
---------------------------
*/

void FX_EmplacedHitWall( vec3_t origin, vec3_t normal )
{
	theFxScheduler.PlayEffect( "emplaced/wall_impact", origin, normal );
}

/*
---------------------------
FX_TurretProjectileThink
---------------------------
*/

void FX_TurretProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon )
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

	theFxScheduler.PlayEffect( "turret/shot", cent->lerpOrigin, forward );
}