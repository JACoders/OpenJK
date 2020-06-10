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

// DEMP2 Weapon

#include "cg_local.h"

/*
---------------------------
FX_DEMP2_ProjectileThink
---------------------------
*/

void FX_DEMP2_ProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	trap->FX_PlayEffectID( cgs.effects.demp2ProjectileEffect, cent->lerpOrigin, forward, -1, -1, qfalse );
}

/*
---------------------------
FX_DEMP2_HitWall
---------------------------
*/

void FX_DEMP2_HitWall( vec3_t origin, vec3_t normal )
{
	trap->FX_PlayEffectID( cgs.effects.demp2WallImpactEffect, origin, normal, -1, -1, qfalse );
}

/*
---------------------------
FX_DEMP2_HitPlayer
---------------------------
*/

void FX_DEMP2_HitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid )
{
	trap->FX_PlayEffectID( cgs.effects.demp2FleshImpactEffect, origin, normal, -1, -1, qfalse );
}

/*
---------------------------
FX_DEMP2_AltBeam
---------------------------
*/
void FX_DEMP2_AltBeam( vec3_t start, vec3_t end, vec3_t normal, //qboolean spark,
								vec3_t targ1, vec3_t targ2 )
{
//NOTENOTE Fix this after trap calls for all primitives are created.
/*
	vec3_t	dir, chaos,
			c1, c2,
			v1, v2;
	float	len,
			s1, s2, s3;

	VectorSubtract( end, start, dir );
	len = VectorNormalize( dir );

	// Get the base control points, we'll work from there
	VectorMA( start, 0.3333f * len, dir, c1 );
	VectorMA( start, 0.6666f * len, dir, c2 );

	// get some chaos values that really aren't very chaotic :)
	s1 = sin( cg.time * 0.005f ) * 2 + Q_flrand(-1.0f, 1.0f) * 0.2f;
	s2 = sin( cg.time * 0.001f );
	s3 = sin( cg.time * 0.011f );

	VectorSet( chaos, len * 0.01f * s1,
						len * 0.02f * s2,
						len * 0.04f * (s1 + s2 + s3));

	VectorAdd( c1, chaos, c1 );
	VectorScale( chaos, 4.0f, v1 );

	VectorSet( chaos, -len * 0.02f * s3,
						len * 0.01f * (s1 * s2),
						-len * 0.02f * (s1 + s2 * s3));

	VectorAdd( c2, chaos, c2 );
	VectorScale( chaos, 2.0f, v2 );

	VectorSet( chaos, 1.0f, 1.0f, 1.0f );

	FX_AddBezier( start, targ1,
						c1, v1, c2, v2,
						5.0f + s1 * 2, 8.0f, 0.0f,
						1.0f, 0.0f, 0.0f,
						chaos, chaos, 0.0f,
						1.0f, trap->R_RegisterShader( "gfx/misc/electric2" ), FX_ALPHA_LINEAR );

	FX_AddBezier( start, targ1,
						c2, v2, c1, v1,
						3.0f + s3, 3.0f, 0.0f,
						1.0f, 0.0f, 0.0f,
						chaos, chaos, 0.0f,
						1.0f, trap->R_RegisterShader( "gfx/misc/electric2" ), FX_ALPHA_LINEAR );

	s1 = sin( cg.time * 0.0005f ) + Q_flrand(-1.0f, 1.0f) * 0.1f;
	s2 = sin( cg.time * 0.0025f );
	float cc2 = cos( cg.time * 0.0025f );
	s3 = sin( cg.time * 0.01f ) + Q_flrand(-1.0f, 1.0f) * 0.1f;

	VectorSet( chaos, len * 0.08f * s2,
						len * 0.04f * cc2,//s1 * -s3,
						len * 0.06f * s3 );

	VectorAdd( c1, chaos, c1 );
	VectorScale( chaos, 4.0f, v1 );

	VectorSet( chaos, len * 0.02f * s1 * s3,
						len * 0.04f * s2,
						len * 0.03f * s1 * s2 );

	VectorAdd( c2, chaos, c2 );
	VectorScale( chaos, 3.0f, v2 );

	VectorSet( chaos, 1.0f, 1.0f, 1.0f );

	FX_AddBezier( start, targ1,
						c1, v1, c2, v2,
						4.0f + s3, 8.0f, 0.0f,
						1.0f, 0.0f, 0.0f,
						chaos, chaos, 0.0f,
						1.0f, trap->R_RegisterShader( "gfx/misc/electric2" ), FX_ALPHA_LINEAR );

	FX_AddBezier( start, targ1,
						c2, v1, c1, v2,
						5.0f + s1 * 2, 8.0f, 0.0f,
						1.0f, 0.0f, 0.0f,
						chaos, chaos, 0.0f,
						1.0f, trap->R_RegisterShader( "gfx/misc/electric2" ), FX_ALPHA_LINEAR );


	VectorMA( start, 14.0f, dir, c1 );

	FX_AddSprite( c1, NULL, NULL, 12.0f + Q_flrand(-1.0f, 1.0f) * 4, 0.0f, 1.0f, 1.0f, Q_flrand(0.0f, 1.0f) * 360, 0.0f, 1.0f,
						trap->R_RegisterShader( "gfx/misc/lightningFlash" ));
	FX_AddSprite( c1, NULL, NULL, 6.0f + Q_flrand(-1.0f, 1.0f) * 2, 0.0f, 1.0f, 1.0f, Q_flrand(0.0f, 1.0f) * 360, 0.0f, 1.0f,
						trap->R_RegisterShader( "gfx/misc/lightningFlash" ));

	FX_AddSprite( targ1, NULL, NULL, 4.0f + Q_flrand(-1.0f, 1.0f), 0.0f, 1.0f, 0.0f, chaos, chaos, Q_flrand(0.0f, 1.0f) * 360, 0.0f, 10,
						trap->R_RegisterShader( "gfx/misc/lightningFlash" ));
	FX_AddSprite( targ1, NULL, NULL, 8.0f + Q_flrand(-1.0f, 1.0f) * 2, 0.0f, 1.0f, 0.0f, chaos, chaos, Q_flrand(0.0f, 1.0f) * 360, 0.0f, 10,
						trap->R_RegisterShader( "gfx/misc/lightningFlash" ));


	//--------------------------------------------

	VectorSubtract( targ2, targ1, dir );
	len = VectorNormalize( dir );

	// Get the base control points, we'll work from there
	VectorMA( targ1, 0.3333f * len, dir, c1 );
	VectorMA( targ1, 0.6666f * len, dir, c2 );

	// get some chaos values that really aren't very chaotic :)
	s1 = sin( cg.time * 0.005f ) * 2 + Q_flrand(-1.0f, 1.0f) * 0.2f;
	s2 = sin( cg.time * 0.001f );
	s3 = sin( cg.time * 0.011f );

	VectorSet( chaos, len * 0.01f * s1,
						len * 0.02f * s2,
						len * 0.04f * (s1 + s2 + s3));

	VectorAdd( c1, chaos, c1 );
	VectorScale( chaos, 4.0f, v1 );

	VectorSet( chaos, -len * 0.02f * s3,
						len * 0.01f * (s1 * s2),
						-len * 0.02f * (s1 + s2 * s3));

	VectorAdd( c2, chaos, c2 );
	VectorScale( chaos, 2.0f, v2 );

	VectorSet( chaos, 1.0f, 1.0f, 1.0f );

	FX_AddBezier( targ1, targ2,
						c1, v1, c2, v2,
						5.0f + s1 * 2, 8.0f, 0.0f,
						1.0f, 0.0f, 0.0f,
						chaos, chaos, 0.0f,
						1.0f, trap->R_RegisterShader( "gfx/misc/electric2" ), FX_ALPHA_LINEAR );

	FX_AddBezier( targ1, targ2,
						c2, v2, c1, v1,
						3.0f + s3, 3.0f, 0.0f,
						1.0f, 0.0f, 0.0f,
						chaos, chaos, 0.0f,
						1.0f, trap->R_RegisterShader( "gfx/misc/electric2" ), FX_ALPHA_LINEAR );

	s1 = sin( cg.time * 0.0005f ) + Q_flrand(-1.0f, 1.0f) * 0.1f;
	s2 = sin( cg.time * 0.0025f );
	cc2 = cos( cg.time * 0.0025f );
	s3 = sin( cg.time * 0.01f ) + Q_flrand(-1.0f, 1.0f) * 0.1f;

	VectorSet( chaos, len * 0.08f * s2,
						len * 0.04f * cc2,//s1 * -s3,
						len * 0.06f * s3 );

	VectorAdd( c1, chaos, c1 );
	VectorScale( chaos, 4.0f, v1 );

	VectorSet( chaos, len * 0.02f * s1 * s3,
						len * 0.04f * s2,
						len * 0.03f * s1 * s2 );

	VectorAdd( c2, chaos, c2 );
	VectorScale( chaos, 3.0f, v2 );

	VectorSet( chaos, 1.0f, 1.0f, 1.0f );

	FX_AddBezier( targ1, targ2,
						c1, v1, c2, v2,
						4.0f + s3, 8.0f, 0.0f,
						1.0f, 0.0f, 0.0f,
						chaos, chaos, 0.0f,
						1.0f, trap->R_RegisterShader( "gfx/misc/electric2" ), FX_ALPHA_LINEAR );

	FX_AddBezier( targ1, targ2,
						c2, v1, c1, v2,
						5.0f + s1 * 2, 8.0f, 0.0f,
						1.0f, 0.0f, 0.0f,
						chaos, chaos, 0.0f,
						1.0f, trap->R_RegisterShader( "gfx/misc/electric2" ), FX_ALPHA_LINEAR );


	FX_AddSprite( targ2, NULL, NULL, 4.0f + Q_flrand(-1.0f, 1.0f), 0.0f, 1.0f, 0.0f, chaos, chaos, Q_flrand(0.0f, 1.0f) * 360, 0.0f, 10,
						trap->R_RegisterShader( "gfx/misc/lightningFlash" ));
	FX_AddSprite( targ2, NULL, NULL, 8.0f + Q_flrand(-1.0f, 1.0f) * 2, 0.0f, 1.0f, 0.0f, chaos, chaos, Q_flrand(0.0f, 1.0f) * 360, 0.0f, 10,
						trap->R_RegisterShader( "gfx/misc/lightningFlash" ));
*/
}

//---------------------------------------------
void FX_DEMP2_AltDetonate( vec3_t org, float size )
{
	localEntity_t	*ex;

	ex = CG_AllocLocalEntity();
	ex->leType = LE_FADE_SCALE_MODEL;
	memset( &ex->refEntity, 0, sizeof( refEntity_t ));

	ex->refEntity.renderfx |= RF_VOLUMETRIC;

	ex->startTime = cg.time;
	ex->endTime = ex->startTime + 800;//1600;

	ex->radius = size;
	ex->refEntity.customShader = cgs.media.demp2ShellShader;
	ex->refEntity.hModel = cgs.media.demp2Shell;
	VectorCopy( org, ex->refEntity.origin );

	ex->color[0] = ex->color[1] = ex->color[2] = 255.0f;
}
