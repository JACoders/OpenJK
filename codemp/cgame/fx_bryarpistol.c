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

// Bryar Pistol Weapon Effects

#include "cg_local.h"
#include "fx_local.h"

/*
-------------------------

	MAIN FIRE

-------------------------
FX_BryarProjectileThink
-------------------------
*/
void FX_BryarProjectileThink(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	trap->FX_PlayEffectID( cgs.effects.bryarShotEffect, cent->lerpOrigin, forward, -1, -1, qfalse );
}

/*
-------------------------
FX_BryarHitWall
-------------------------
*/
void FX_BryarHitWall( vec3_t origin, vec3_t normal )
{
	trap->FX_PlayEffectID( cgs.effects.bryarWallImpactEffect, origin, normal, -1, -1, qfalse );
}

/*
-------------------------
FX_BryarHitPlayer
-------------------------
*/
void FX_BryarHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid )
{
	if ( humanoid )
	{
		trap->FX_PlayEffectID( cgs.effects.bryarFleshImpactEffect, origin, normal, -1, -1, qfalse );
	}
	else
	{
		trap->FX_PlayEffectID( cgs.effects.bryarDroidImpactEffect, origin, normal, -1, -1, qfalse );
	}
}


/*
-------------------------

	ALT FIRE

-------------------------
FX_BryarAltProjectileThink
-------------------------
*/
void FX_BryarAltProjectileThink(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;
	int t;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	// see if we have some sort of extra charge going on
	for (t = 1; t < cent->currentState.generic1; t++ )
	{
		// just add ourselves over, and over, and over when we are charged
		trap->FX_PlayEffectID( cgs.effects.bryarPowerupShotEffect, cent->lerpOrigin, forward, -1, -1, qfalse );
	}

	//	for ( int t = 1; t < cent->gent->count; t++ )	// The single player stores the charge in count, which isn't accessible on the client

	trap->FX_PlayEffectID( cgs.effects.bryarShotEffect, cent->lerpOrigin, forward, -1, -1, qfalse );
}

/*
-------------------------
FX_BryarAltHitWall
-------------------------
*/
void FX_BryarAltHitWall( vec3_t origin, vec3_t normal, int power )
{
	switch( power )
	{
	case 4:
	case 5:
		trap->FX_PlayEffectID( cgs.effects.bryarWallImpactEffect3, origin, normal, -1, -1, qfalse );
		break;

	case 2:
	case 3:
		trap->FX_PlayEffectID( cgs.effects.bryarWallImpactEffect2, origin, normal, -1, -1, qfalse );
		break;

	default:
		trap->FX_PlayEffectID( cgs.effects.bryarWallImpactEffect, origin, normal, -1, -1, qfalse );
		break;
	}
}

/*
-------------------------
FX_BryarAltHitPlayer
-------------------------
*/
void FX_BryarAltHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid )
{
	if ( humanoid )
	{
		trap->FX_PlayEffectID( cgs.effects.bryarFleshImpactEffect, origin, normal, -1, -1, qfalse );
	}
	else
	{
		trap->FX_PlayEffectID( cgs.effects.bryarDroidImpactEffect, origin, normal, -1, -1, qfalse );
	}
}


//TURRET
/*
-------------------------
FX_TurretProjectileThink
-------------------------
*/
void FX_TurretProjectileThink(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	trap->FX_PlayEffectID( cgs.effects.turretShotEffect, cent->lerpOrigin, forward, -1, -1, qfalse );
}

/*
-------------------------
FX_TurretHitWall
-------------------------
*/
void FX_TurretHitWall( vec3_t origin, vec3_t normal )
{
	trap->FX_PlayEffectID( cgs.effects.bryarWallImpactEffect, origin, normal, -1, -1, qfalse );
}

/*
-------------------------
FX_TurretHitPlayer
-------------------------
*/
void FX_TurretHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid )
{
	if ( humanoid )
	{
		trap->FX_PlayEffectID( cgs.effects.bryarFleshImpactEffect, origin, normal, -1, -1, qfalse );
	}
	else
	{
		trap->FX_PlayEffectID( cgs.effects.bryarDroidImpactEffect, origin, normal, -1, -1, qfalse );
	}
}



//CONCUSSION (yeah, should probably make a new file for this.. or maybe just move all these stupid semi-redundant fx_ functions into one file)
/*
-------------------------
FX_ConcussionHitWall
-------------------------
*/
void FX_ConcussionHitWall( vec3_t origin, vec3_t normal )
{
	trap->FX_PlayEffectID( cgs.effects.concussionImpactEffect, origin, normal, -1, -1, qfalse );
}

/*
-------------------------
FX_ConcussionHitPlayer
-------------------------
*/
void FX_ConcussionHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid )
{
	trap->FX_PlayEffectID( cgs.effects.concussionImpactEffect, origin, normal, -1, -1, qfalse );
}

/*
-------------------------
FX_ConcussionProjectileThink
-------------------------
*/
void FX_ConcussionProjectileThink(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	trap->FX_PlayEffectID( cgs.effects.concussionShotEffect, cent->lerpOrigin, forward, -1, -1, qfalse );
}

/*
---------------------------
FX_ConcAltShot
---------------------------
*/
static vec3_t WHITE	={1.0f,1.0f,1.0f};
static vec3_t BRIGHT={0.75f,0.5f,1.0f};

void FX_ConcAltShot( vec3_t start, vec3_t end )
{
	//"concussion/beam"
	trap->FX_AddLine( start, end, 0.1f, 10.0f, 0.0f,
							1.0f, 0.0f, 0.0f,
							WHITE, WHITE, 0.0f,
							175, trap->R_RegisterShader( "gfx/effects/blueLine" ),
							FX_SIZE_LINEAR | FX_ALPHA_LINEAR );

	// add some beef
	trap->FX_AddLine( start, end, 0.1f, 7.0f, 0.0f,
						1.0f, 0.0f, 0.0f,
						BRIGHT, BRIGHT, 0.0f,
						150, trap->R_RegisterShader( "gfx/misc/whiteline2" ),
						FX_SIZE_LINEAR | FX_ALPHA_LINEAR );
}
