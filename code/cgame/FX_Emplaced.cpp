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

	// If tie-fighter missle use green shot.
	if ( cent->currentState.weapon == WP_TIE_FIGHTER )
	{
		theFxScheduler.PlayEffect( "ships/imp_blastershot", cent->lerpOrigin, forward );
	}
	else
	{
		if ( cent->gent && cent->gent->owner && cent->gent->owner->activator && cent->gent->owner->activator->s.number > 0 )
		{
			// NPC's do short shot
			if ( cent->gent->alt_fire )
			{
				theFxScheduler.PlayEffect( "eweb/shotNPC", cent->lerpOrigin, forward );
			}
			else
			{
				theFxScheduler.PlayEffect( "emplaced/shotNPC", cent->lerpOrigin, forward );
			}
		}
		else
		{
			// players do long shot
			if ( cent->gent && cent->gent->alt_fire )
			{
				theFxScheduler.PlayEffect( "eweb/shotNPC", cent->lerpOrigin, forward );
			}
			else
			{
				theFxScheduler.PlayEffect( "emplaced/shot", cent->lerpOrigin, forward );
			}
		}
	}
}

/*
---------------------------
FX_EmplacedHitWall
---------------------------
*/

void FX_EmplacedHitWall( vec3_t origin, vec3_t normal, qboolean eweb )
{
	if ( eweb )
	{
		theFxScheduler.PlayEffect( "eweb/wall_impact", origin, normal );
	}
	else
	{
		theFxScheduler.PlayEffect( "emplaced/wall_impact", origin, normal );
	}
}

/*
---------------------------
FX_EmplacedHitPlayer
---------------------------
*/

void FX_EmplacedHitPlayer( vec3_t origin, vec3_t normal, qboolean eweb )
{
	if ( eweb )
	{
		theFxScheduler.PlayEffect( "eweb/flesh_impact", origin, normal );
	}
	else
	{
		theFxScheduler.PlayEffect( "emplaced/wall_impact", origin, normal );
	}
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