/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
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

// cg_localents.c -- every frame, generate renderer commands for locally
// processed entities, like smoke puffs, gibs, shells, etc.
#include "cg_local.h"
#include "cg_media.h"

#define	MAX_LOCAL_ENTITIES	512
localEntity_t	cg_localEntities[MAX_LOCAL_ENTITIES];
localEntity_t	cg_activeLocalEntities;		// double linked list
localEntity_t	*cg_freeLocalEntities;		// single linked list

/*
===================
CG_InitLocalEntities

This is called at startup and for tournement restarts
===================
*/

void	CG_InitLocalEntities( void ) {
	int		i;

	memset( cg_localEntities, 0, sizeof( cg_localEntities ) );
	cg_activeLocalEntities.next = &cg_activeLocalEntities;
	cg_activeLocalEntities.prev = &cg_activeLocalEntities;
	cg_freeLocalEntities = cg_localEntities;
	for ( i = 0 ; i < MAX_LOCAL_ENTITIES - 1 ; i++ ) {
		cg_localEntities[i].next = &cg_localEntities[i+1];
	}
}


/*
==================
CG_FreeLocalEntity
==================
*/
void CG_FreeLocalEntity( localEntity_t *le ) {
	if ( !le->prev ) {
		CG_Error( "CG_FreeLocalEntity: not active" );
	}

	// remove from the doubly linked active list
	le->prev->next = le->next;
	le->next->prev = le->prev;

	// the free list is only singly linked
	le->next = cg_freeLocalEntities;
	cg_freeLocalEntities = le;
}

/*
===================
CG_AllocLocalEntity

Will allways succeed, even if it requires freeing an old active entity
===================
*/
localEntity_t	*CG_AllocLocalEntity( void ) {
	localEntity_t	*le;

	if ( !cg_freeLocalEntities ) {
		// no free entities, so free the one at the end of the chain
		// remove the oldest active entity
		CG_FreeLocalEntity( cg_activeLocalEntities.prev );
	}

	le = cg_freeLocalEntities;
	cg_freeLocalEntities = cg_freeLocalEntities->next;

	memset( le, 0, sizeof( *le ) );

	// link into the active list
	le->next = cg_activeLocalEntities.next;
	le->prev = &cg_activeLocalEntities;
	cg_activeLocalEntities.next->prev = le;
	cg_activeLocalEntities.next = le;
	le->ownerGentNum = -1;
	return le;
}


/*
====================================================================================

FRAGMENT PROCESSING

A fragment localentity interacts with the environment in some way (hitting walls),
or generates more localentities along a trail.

====================================================================================
*/


/*
================
CG_FragmentBounceSound
================
*/
void CG_FragmentBounceSound( localEntity_t *le, trace_t *trace )
{
	// half the fragments will make a bounce sounds
	if ( rand() & 1 )
	{
		sfxHandle_t	s = 0;

		switch( le->leBounceSoundType )
		{
		case LEBS_ROCK:
			s = cgs.media.rockBounceSound[Q_irand(0,1)];
			break;
		case LEBS_METAL:
			s = cgs.media.metalBounceSound[Q_irand(0,1)];// FIXME: make sure that this sound is registered properly...might still be rock bounce sound....
			break;
		default:
			break;
		}

		if ( s )
		{
			cgi_S_StartSound( trace->endpos, ENTITYNUM_WORLD, CHAN_AUTO, s );
		}

		// bouncers only make the sound once...
		// FIXME: arbitrary...change if it bugs you
		le->leBounceSoundType = LEBS_NONE;
	}
	else if ( rand() & 1 )
	{
		// we may end up bouncing again, but each bounce reduces the chance of playing the sound again or they may make a lot of noise when they settle
		// FIXME: maybe just always do this??
		le->leBounceSoundType = LEBS_NONE;
	}
}


/*
================
CG_ReflectVelocity
================
*/
void CG_ReflectVelocity( localEntity_t *le, trace_t *trace )
{
	vec3_t	velocity;
	float	dot;
	int		hitTime;

	// reflect the velocity on the trace plane
	hitTime = cg.time - cg.frametime + cg.frametime * trace->fraction;
	EvaluateTrajectoryDelta( &le->pos, hitTime, velocity );
	dot = DotProduct( velocity, trace->plane.normal );
	VectorMA( velocity, -2*dot, trace->plane.normal, le->pos.trDelta );

	VectorScale( le->pos.trDelta, le->bounceFactor, le->pos.trDelta );

	VectorCopy( trace->endpos, le->pos.trBase );
	le->pos.trTime = cg.time;

	// check for stop, making sure that even on low FPS systems it doesn't bobble
	if ( trace->allsolid ||
		( trace->plane.normal[2] > 0 &&
		( le->pos.trDelta[2] < 40 || le->pos.trDelta[2] < -cg.frametime * le->pos.trDelta[2] ) ) )
	{
		le->pos.trType = TR_STATIONARY;
	}
}

/*
================
CG_AddFragment
================
*/
void CG_AddFragment( localEntity_t *le )
{
	vec3_t	newOrigin;
	trace_t	trace;
	// used to sink into the ground, but it looks better to maybe just fade them out
	int		t;

	t = le->endTime - cg.time;

	if ( t < FRAG_FADE_TIME )
	{
		le->refEntity.renderfx |= RF_ALPHA_FADE;
		le->refEntity.shaderRGBA[0] = le->refEntity.shaderRGBA[1] = le->refEntity.shaderRGBA[2] = 255;
		le->refEntity.shaderRGBA[3] = ((float)t / FRAG_FADE_TIME) * 255.0f;
	}

	if ( le->pos.trType == TR_STATIONARY )
	{
		if ( !(cgi_CM_PointContents( le->refEntity.origin, 0 ) & CONTENTS_SOLID ))
		{
			// thing is no longer in solid, so let gravity take it back
			VectorCopy( le->refEntity.origin, le->pos.trBase );
			VectorClear( le->pos.trDelta );
			le->pos.trTime = cg.time;
			le->pos.trType = TR_GRAVITY;
		}

		cgi_R_AddRefEntityToScene( &le->refEntity );

		return;
	}

	// calculate new position
	EvaluateTrajectory( &le->pos, cg.time, newOrigin );

	le->refEntity.renderfx |= RF_LIGHTING_ORIGIN;
	VectorCopy( newOrigin, le->refEntity.lightingOrigin );

	// trace a line from previous position to new position
	CG_Trace( &trace, le->refEntity.origin, NULL, NULL, newOrigin, le->ownerGentNum, CONTENTS_SOLID );
	if ( trace.fraction == 1.0 ) {
		// still in free fall
		VectorCopy( newOrigin, le->refEntity.origin );

		if ( le->leFlags & LEF_TUMBLE ) {
			vec3_t angles;

			EvaluateTrajectory( &le->angles, cg.time, angles );
			AnglesToAxis( angles, le->refEntity.axis );
			for(int k = 0; k < 3; k++)
			{
				VectorScale(le->refEntity.axis[k], le->radius, le->refEntity.axis[k]);
			}

		}

		cgi_R_AddRefEntityToScene( &le->refEntity );

		return;
	}

	// if it is in a nodrop zone, remove it
	// this keeps gibs from waiting at the bottom of pits of death
	// and floating levels
	if ( cgi_CM_PointContents( trace.endpos, 0 ) & CONTENTS_NODROP )
	{
		CG_FreeLocalEntity( le );
		return;
	}

	// do a bouncy sound
	CG_FragmentBounceSound( le, &trace );

	// reflect the velocity on the trace plane
	CG_ReflectVelocity( le, &trace );
	//FIXME: if LEF_TUMBLE, change avelocity too?

	cgi_R_AddRefEntityToScene( &le->refEntity );
}

/*
=====================================================================

TRIVIAL LOCAL ENTITIES

These only do simple scaling or modulation before passing to the renderer
=====================================================================
*/

/*
** CG_AddTeleporterEffect
*/
void CG_AddTeleporterEffect( localEntity_t *le ) {
	refEntity_t *re;
	float c;

	re = &le->refEntity;

	c = ( le->endTime - cg.time ) / ( float ) ( le->endTime - le->startTime );

	re->shaderRGBA[0] =
	re->shaderRGBA[1] =
	re->shaderRGBA[2] =
	re->shaderRGBA[3] = 0xff * c;

	cgi_R_AddRefEntityToScene( re );
}

/*
** CG_AddFadeRGB
*/
void CG_AddFadeRGB( localEntity_t *le ) {
	refEntity_t *re;
	float c;

	re = &le->refEntity;

	c = ( le->endTime - cg.time ) * le->lifeRate;
	c *= 0xff;

	re->shaderRGBA[0] = le->color[0] * c;
	re->shaderRGBA[1] = le->color[1] * c;
	re->shaderRGBA[2] = le->color[2] * c;
	re->shaderRGBA[3] = le->color[3] * c;

	cgi_R_AddRefEntityToScene( re );
}

/*
==================
CG_AddPuff
==================
*/
static void CG_AddPuff( localEntity_t *le ) {
	refEntity_t	*re;
	float		c;
	vec3_t		delta;
	float		len;

	re = &le->refEntity;

	// fade / grow time
	c = ( le->endTime - cg.time ) / (float)( le->endTime - le->startTime );

	re->shaderRGBA[0] = le->color[0] * c;
	re->shaderRGBA[1] = le->color[1] * c;
	re->shaderRGBA[2] = le->color[2] * c;

	if ( !( le->leFlags & LEF_PUFF_DONT_SCALE ) ) {
		re->radius = le->radius * ( 1.0 - c ) + 8;
	}

	EvaluateTrajectory( &le->pos, cg.time, re->origin );

	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	VectorSubtract( re->origin, cg.refdef.vieworg, delta );
	len = VectorLength( delta );
	if ( len < le->radius ) {
		CG_FreeLocalEntity( le );
		return;
	}

	cgi_R_AddRefEntityToScene( re );
}

/*
================
CG_AddLocalLight
================
*/
static void CG_AddLocalLight( localEntity_t *le )
{
	// There should be a light if this is being used, but hey...
	if ( le->light )
	{
		float		light;

		light = (float)( cg.time - le->startTime ) / ( le->endTime - le->startTime );

		if ( light < 0.5 )
		{
			light = 1.0;
		}
		else
		{
			light = 1.0 - ( light - 0.5 ) * 2;
		}

		light = le->light * light;

		cgi_R_AddLightToScene( le->refEntity.origin, light, le->lightColor[0], le->lightColor[1], le->lightColor[2] );
	}
}

//---------------------------------------------------
static void CG_AddFadeModel( localEntity_t *le )
{
	refEntity_t	*ent = &le->refEntity;

	if ( cg.time < le->startTime )
	{
		CG_FreeLocalEntity( le );
		return;
	}

	float frac = 1.0f - ((float)( cg.time - le->startTime )/(float)( le->endTime - le->startTime ));

	ent->shaderRGBA[0] = le->color[0] * frac;
	ent->shaderRGBA[1] = le->color[1] * frac;
	ent->shaderRGBA[2] = le->color[2] * frac;
	ent->shaderRGBA[3] = le->color[3] * frac;

	EvaluateTrajectory( &le->pos, cg.time, ent->origin );

	// add the entity
	cgi_R_AddRefEntityToScene( ent );
}

// NOTE: this is 100% for the demp2 alt-fire effect, so changes to the visual effect will affect game side demp2 code
//---------------------------------------------------
static void CG_AddFadeScaleModel( localEntity_t *le )
{
	refEntity_t	*ent = &le->refEntity;

	float frac = ( cg.time - le->startTime )/((float)( le->endTime - le->startTime ));

	frac *= frac * frac; // yes, this is completely ridiculous...but it causes the shell to grow slowly then "explode" at the end

	ent->nonNormalizedAxes = qtrue;

	AxisCopy( axisDefault, ent->axis );

	VectorScale( ent->axis[0], le->radius * frac, ent->axis[0] );
	VectorScale( ent->axis[1], le->radius * frac, ent->axis[1] );
	VectorScale( ent->axis[2], le->radius * 0.5f * frac, ent->axis[2] );

	frac = 1.0f - frac;

	ent->shaderRGBA[0] = le->color[0] * frac;
	ent->shaderRGBA[1] = le->color[1] * frac;
	ent->shaderRGBA[2] = le->color[2] * frac;
	ent->shaderRGBA[3] = le->color[3] * frac;

	// add the entity
	cgi_R_AddRefEntityToScene( ent );
}

// create a quad that doesn't use a refEnt.  Currently only for use with the DebugNav drawing so it doesn't have to use fx
//------------------------------------------
static void CG_AddQuad( localEntity_t *le )
{
	polyVert_t	verts[4];

	VectorCopy( le->refEntity.origin, verts[0].xyz );
	verts[0].xyz[0] -= le->radius;
	verts[0].xyz[1] -= le->radius;
	verts[0].st[0] = 0;
	verts[0].st[1] = 0;

	for ( int i = 0; i < 4; i++ )
	{
		verts[i].modulate[0] = le->color[0];
		verts[i].modulate[1] = le->color[1];
		verts[i].modulate[2] = le->color[2];
		verts[i].modulate[3] = le->color[3];
	}

	VectorCopy( le->refEntity.origin, verts[1].xyz );
	verts[1].xyz[0] -= le->radius;
	verts[1].xyz[1] += le->radius;
	verts[1].st[0] = 0;
	verts[1].st[1] = 1;

	VectorCopy( le->refEntity.origin, verts[2].xyz );
	verts[2].xyz[0] += le->radius;
	verts[2].xyz[1] += le->radius;
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;

	VectorCopy( le->refEntity.origin, verts[3].xyz );
	verts[3].xyz[0] += le->radius;
	verts[3].xyz[1] -= le->radius;
	verts[3].st[0] = 1;
	verts[3].st[1] = 0;

	cgi_R_AddPolyToScene( le->refEntity.customShader, 4, verts );
}

// create a sprite that doesn't use a refEnt.  Currently only for use with the DebugNav drawing so it doesn't have to use fx
//------------------------------------------
static void CG_AddSprite( localEntity_t *le )
{
	polyVert_t	verts[4];

	VectorCopy( le->refEntity.origin, verts[0].xyz );
	VectorMA( verts[0].xyz, -le->radius, cg.refdef.viewaxis[2], verts[0].xyz );
	VectorMA( verts[0].xyz, -le->radius, cg.refdef.viewaxis[1], verts[0].xyz );
	verts[0].st[0] = 0;
	verts[0].st[1] = 0;

	for ( int i = 0; i < 4; i++ )
	{
		verts[i].modulate[0] = le->color[0];
		verts[i].modulate[1] = le->color[1];
		verts[i].modulate[2] = le->color[2];
		verts[i].modulate[3] = le->color[3];
	}

	VectorCopy( le->refEntity.origin, verts[1].xyz );
	VectorMA( verts[1].xyz, -le->radius, cg.refdef.viewaxis[2], verts[1].xyz );
	VectorMA( verts[1].xyz, le->radius, cg.refdef.viewaxis[1], verts[1].xyz );
	verts[1].st[0] = 0;
	verts[1].st[1] = 1;

	VectorCopy( le->refEntity.origin, verts[2].xyz );
	VectorMA( verts[2].xyz, le->radius, cg.refdef.viewaxis[2], verts[2].xyz );
	VectorMA( verts[2].xyz, le->radius, cg.refdef.viewaxis[1], verts[2].xyz );
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;

	VectorCopy( le->refEntity.origin, verts[3].xyz );
	VectorMA( verts[3].xyz, le->radius, cg.refdef.viewaxis[2], verts[3].xyz );
	VectorMA( verts[3].xyz, -le->radius, cg.refdef.viewaxis[1], verts[3].xyz );
	verts[3].st[0] = 1;
	verts[3].st[1] = 0;

	cgi_R_AddPolyToScene( le->refEntity.customShader, 4, verts );
}

/*
===================
CG_AddLine

for beams and the like.
===================
*/
void CG_AddLine( localEntity_t *le )
{
	refEntity_t	*re;

	re = &le->refEntity;

	re->reType = RT_LINE;

	cgi_R_AddRefEntityToScene( re );
}

//==============================================================================

/*
===================
CG_AddLocalEntities

===================
*/
void CG_AddLocalEntities( void )
{
	localEntity_t	*le, *next;

	// walk the list backwards, so any new local entities generated
	// (trails, marks, etc) will be present this frame
	le = cg_activeLocalEntities.prev;
	for ( ; le != &cg_activeLocalEntities ; le = next ) {
		// grab next now, so if the local entity is freed we
		// still have it
		next = le->prev;

		if ( cg.time >= le->endTime ) {
			CG_FreeLocalEntity( le );
			continue;
		}
		switch ( le->leType ) {
		default:
			CG_Error( "Bad leType: %i", le->leType );
			break;

		case LE_MARK:
			break;

		case LE_FADE_MODEL:
			CG_AddFadeModel( le );
			break;

		case LE_FADE_SCALE_MODEL:
			CG_AddFadeScaleModel( le );
			break;

		case LE_FRAGMENT:
			CG_AddFragment( le );
			break;

		case LE_PUFF:
			CG_AddPuff( le );
			break;

		case LE_FADE_RGB:		// teleporters, railtrails
			CG_AddFadeRGB( le );
			break;

		case LE_LIGHT:
			CG_AddLocalLight( le );
			break;

		case LE_LINE:					// oriented lines for FX
			CG_AddLine( le );
			break;

		// Use for debug only
		case LE_QUAD:
			CG_AddQuad( le );
			break;

		case LE_SPRITE:
			CG_AddSprite( le );
		}
	}
}




