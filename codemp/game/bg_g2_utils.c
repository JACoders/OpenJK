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

// bg_g2_utils.c -- both games misc functions, all completely stateless
// only in game and cgame, NOT ui

#include "qcommon/q_shared.h"
#include "bg_public.h"

#if defined(_GAME)
	#include "g_local.h"
#elif defined(_CGAME)
	#include "cgame/cg_local.h"
#endif

void BG_AttachToRancor( void *ghoul2, float rancYaw, vec3_t rancOrigin, int time, qhandle_t *modelList, vec3_t modelScale, qboolean inMouth, vec3_t out_origin, vec3_t out_angles, matrix3_t out_axis )
{
	mdxaBone_t	boltMatrix;
	int boltIndex;
	vec3_t rancAngles;
	vec3_t temp_angles;
	// Getting the bolt here
	if ( inMouth )
	{//in mouth
	#if defined(_GAME)
		boltIndex = trap->G2API_AddBolt(ghoul2, 0, "jaw_bone");
	#elif defined(_CGAME)
		boltIndex = trap->G2API_AddBolt(ghoul2, 0, "jaw_bone");
	#endif
	}
	else
	{//in right hand
	#if defined(_GAME)
		boltIndex = trap->G2API_AddBolt(ghoul2, 0, "*r_hand");
	#elif defined(_CGAME)
		boltIndex = trap->G2API_AddBolt(ghoul2, 0, "*r_hand");
	#endif
	}
	VectorSet( rancAngles, 0, rancYaw, 0 );
#if defined(_GAME)
	trap->G2API_GetBoltMatrix( ghoul2, 0, boltIndex, &boltMatrix, rancAngles, rancOrigin, time, modelList, modelScale );
#elif defined(_CGAME)
	trap->G2API_GetBoltMatrix( ghoul2, 0, boltIndex, &boltMatrix, rancAngles, rancOrigin, time, modelList, modelScale );
#endif
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
		matrix3_t temp_axis;
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
#if defined(_GAME)
	if ( !ghoul2 || !trap->G2API_GetSurfaceRenderStatus( ghoul2, 0, rootSurfName ) )
#elif defined(_CGAME)
	if ( !ghoul2 || !trap->G2API_GetSurfaceRenderStatus( ghoul2, 0, rootSurfName ) )
#endif
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
		#if defined(_GAME)
			if ( !trap->G2API_GetSurfaceRenderStatus( ghoul2, 0, returnSurfName ) )
		#elif defined(_CGAME)
			if ( !trap->G2API_GetSurfaceRenderStatus( ghoul2, 0, returnSurfName ) )
		#endif
			{
				return qtrue;
			}
		}
	}
	Q_strncpyz( returnSurfName, rootSurfName, returnSize );
	return qfalse;
}

