// Copyright (C) 1999-2000 Id Software, Inc.
//
// bg_g2_utils.c -- both games misc functions, all completely stateless

#include "q_shared.h"
#include "bg_public.h"
#include "bg_strap.h"

#ifdef QAGAME
#include "g_local.h"
#endif

#ifdef UI_EXPORTS
#include "../ui/ui_local.h"
#endif

#ifndef UI_EXPORTS
#ifndef QAGAME
#include "../cgame/cg_local.h"
#endif
#endif

#include "../namespace_begin.h"

void BG_AttachToRancor( void *ghoul2,
					   float rancYaw,
					   vec3_t rancOrigin,
					   int time,
					   qhandle_t *modelList, 
					   vec3_t modelScale,
					   qboolean inMouth,
					   vec3_t out_origin,
					   vec3_t out_angles,
					   vec3_t out_axis[3] )
{
	mdxaBone_t	boltMatrix;
	int boltIndex;
	vec3_t rancAngles;
	vec3_t temp_angles;
	// Getting the bolt here
	if ( inMouth )
	{//in mouth
		boltIndex = trap_G2API_AddBolt(ghoul2, 0, "jaw_bone");
	}
	else
	{//in right hand
		boltIndex = trap_G2API_AddBolt(ghoul2, 0, "*r_hand");
	}
	VectorSet( rancAngles, 0, rancYaw, 0 );
	trap_G2API_GetBoltMatrix( ghoul2, 0, boltIndex, 
			&boltMatrix, rancAngles, rancOrigin, time,
			modelList, modelScale );
	// Storing ent position, bolt position, and bolt axis
	if ( out_origin )
	{
		BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, out_origin );
	}
	if ( out_axis )
	{
		if ( inMouth )
		{//in mouth
			BG_GiveMeVectorFromMatrix( &boltMatrix, POSITIVE_Z, out_axis[0] );
			BG_GiveMeVectorFromMatrix( &boltMatrix, NEGATIVE_Y, out_axis[1] );
			BG_GiveMeVectorFromMatrix( &boltMatrix, NEGATIVE_X, out_axis[2] );
		}
		else
		{//in hand
			BG_GiveMeVectorFromMatrix( &boltMatrix, NEGATIVE_Y, out_axis[0] );
			BG_GiveMeVectorFromMatrix( &boltMatrix, POSITIVE_X, out_axis[1] );
			BG_GiveMeVectorFromMatrix( &boltMatrix, POSITIVE_Z, out_axis[2] );
		}
		//FIXME: this is messing up our axis and turning us inside-out?
		if ( out_angles )
		{
			vectoangles( out_axis[0], out_angles );
			vectoangles( out_axis[2], temp_angles );
			out_angles[ROLL] = -temp_angles[PITCH];
		}
	}
	else if ( out_angles )
	{
		vec3_t temp_axis[3];
		if ( inMouth )
		{//in mouth
			BG_GiveMeVectorFromMatrix( &boltMatrix, POSITIVE_Z, temp_axis[0] );
			BG_GiveMeVectorFromMatrix( &boltMatrix, NEGATIVE_X, temp_axis[2] );
		}
		else
		{//in hand
			BG_GiveMeVectorFromMatrix( &boltMatrix, NEGATIVE_Y, temp_axis[0] );
			BG_GiveMeVectorFromMatrix( &boltMatrix, POSITIVE_Z, temp_axis[2] );
		}
		//FIXME: this is messing up our axis and turning us inside-out?
		vectoangles( temp_axis[0], out_angles );
		vectoangles( temp_axis[2], temp_angles );
		out_angles[ROLL] = -temp_angles[PITCH];
	}
}

#define	MAX_VARIANTS 8
qboolean BG_GetRootSurfNameWithVariant( void *ghoul2, const char *rootSurfName, char *returnSurfName, int returnSize )
{
	if ( !ghoul2 || !trap_G2API_GetSurfaceRenderStatus( ghoul2, 0, rootSurfName ) )
	{//see if the basic name without variants is on
		Q_strncpyz( returnSurfName, rootSurfName, returnSize );
		return qtrue;
	}
	else
	{//check variants
		int i;
		for ( i = 0; i < MAX_VARIANTS; i++ )
		{
			Com_sprintf( returnSurfName, returnSize, "%s%c", rootSurfName, 'a'+i );
			if ( !trap_G2API_GetSurfaceRenderStatus( ghoul2, 0, returnSurfName ) )
			{
				return qtrue;
			}
		}
	}
	Q_strncpyz( returnSurfName, rootSurfName, returnSize );
	return qfalse;
}

#include "../namespace_end.h"
