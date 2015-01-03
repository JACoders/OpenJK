/*
This file is part of Jedi Academy.

    Jedi Academy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Jedi Academy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Academy.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

// tr_light.c

// leave this as first line for PCH reasons...
//
#include "../server/exe_headers.h"


#include "tr_local.h"

#define	DLIGHT_AT_RADIUS		16
// at the edge of a dlight's influence, this amount of light will be added

#define	DLIGHT_MINIMUM_RADIUS	16		
// never calculate a range less than this to prevent huge light numbers


/*
===============
R_TransformDlights

Transforms the origins of an array of dlights.
Used by both the front end (for DlightBmodel) and
the back end (before doing the lighting calculation)
===============
*/
void R_TransformDlights( int count, dlight_t *dl, orientationr_t *ori) {
	int		i;
	vec3_t	temp;

	for ( i = 0 ; i < count ; i++, dl++ ) {
		VectorSubtract( dl->origin, ori->origin, temp );
		dl->transformed[0] = DotProduct( temp, ori->axis[0] );
		dl->transformed[1] = DotProduct( temp, ori->axis[1] );
		dl->transformed[2] = DotProduct( temp, ori->axis[2] );
	}
}

/*
=============
R_DlightBmodel

Determine which dynamic lights may effect this bmodel
=============
*/
void R_DlightBmodel( bmodel_t *bmodel, qboolean NoLight ) {
	int			i, j;
	dlight_t	*dl;
	int			mask;
	msurface_t	*surf;

	// transform all the lights
	R_TransformDlights( tr.refdef.num_dlights, tr.refdef.dlights, &tr.ori );

	mask = 0;
	if (!NoLight)
	{
		for ( i=0 ; i<tr.refdef.num_dlights ; i++ ) {
			dl = &tr.refdef.dlights[i];

			// see if the point is close enough to the bounds to matter
			for ( j = 0 ; j < 3 ; j++ ) {
				if ( dl->transformed[j] - bmodel->bounds[1][j] > dl->radius ) {
					break;
				}
				if ( bmodel->bounds[0][j] - dl->transformed[j] > dl->radius ) {
					break;
				}
			}
			if ( j < 3 ) {
				continue;
			}

			// we need to check this light
			mask |= 1 << i;
		}
	}

	tr.currentEntity->needDlights = (mask != 0);
	tr.currentEntity->dlightBits = mask;


	// set the dlight bits in all the surfaces
	for ( i = 0 ; i < bmodel->numSurfaces ; i++ ) {
		surf = bmodel->firstSurface + i;

		if ( *surf->data == SF_FACE ) {
			((srfSurfaceFace_t *)surf->data)->dlightBits = mask;
		} else if ( *surf->data == SF_GRID ) {
			((srfGridMesh_t *)surf->data)->dlightBits = mask;
		} else if ( *surf->data == SF_TRIANGLES ) {
			((srfTriangles_t *)surf->data)->dlightBits = mask;
		}
	}
}

/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

extern	cvar_t	*r_ambientScale;
extern	cvar_t	*r_directedScale;
extern	cvar_t	*r_debugLight;

/*
=================
R_SetupEntityLightingGrid

=================
*/
static void R_SetupEntityLightingGrid( trRefEntity_t *ent ) {
	vec3_t			lightOrigin;
	int				pos[3];
	int				i, j;
	float			frac[3];
	int				gridStep[3];
	vec3_t			direction;
	float			totalFactor;
	unsigned short	*startGridPos;
	
	if (r_fullbright->integer || (tr.refdef.rdflags & RDF_doLAGoggles) )
	{
		ent->ambientLight[0] = ent->ambientLight[1] = ent->ambientLight[2] = 255.0;
		ent->directedLight[0] = ent->directedLight[1] = ent->directedLight[2] = 255.0;
		VectorCopy( tr.sunDirection, ent->lightDir );
		return;
	}

	if ( ent->e.renderfx & RF_LIGHTING_ORIGIN ) {
		// seperate lightOrigins are needed so an object that is
		// sinking into the ground can still be lit, and so
		// multi-part models can be lit identically
		VectorCopy( ent->e.lightingOrigin, lightOrigin );
	} else {
		VectorCopy( ent->e.origin, lightOrigin );
	}
#define ACCURATE_LIGHTGRID_SAMPLING 1
#if ACCURATE_LIGHTGRID_SAMPLING
	vec3_t	startLightOrigin;
	VectorCopy( lightOrigin, startLightOrigin );
#endif

	VectorSubtract( lightOrigin, tr.world->lightGridOrigin, lightOrigin );
	for ( i = 0 ; i < 3 ; i++ ) {
		float	v;

		v = lightOrigin[i]*tr.world->lightGridInverseSize[i];
		pos[i] = floor( v );
		frac[i] = v - pos[i];
		if ( pos[i] < 0 ) {
			pos[i] = 0;
		} else if ( pos[i] >= tr.world->lightGridBounds[i] - 1 ) {
			pos[i] = tr.world->lightGridBounds[i] - 1;
		}
	}

	VectorClear( ent->ambientLight );
	VectorClear( ent->directedLight );
	VectorClear( direction );

	// trilerp the light value
	gridStep[0] = 1;
	gridStep[1] = tr.world->lightGridBounds[0];
	gridStep[2] = tr.world->lightGridBounds[0] * tr.world->lightGridBounds[1];
	startGridPos = tr.world->lightGridArray	+ pos[0] * gridStep[0] 
					+ pos[1] * gridStep[1] 	+ pos[2] * gridStep[2];
#if ACCURATE_LIGHTGRID_SAMPLING
	vec3_t	startGridOrg;
	VectorCopy( tr.world->lightGridOrigin, startGridOrg );
	startGridOrg[0] += pos[0] * tr.world->lightGridSize[0];
	startGridOrg[1] += pos[1] * tr.world->lightGridSize[1];
	startGridOrg[2] += pos[2] * tr.world->lightGridSize[2];
#endif
	totalFactor = 0;
	for ( i = 0 ; i < 8 ; i++ ) {
		float			factor;
		mgrid_t			*data;
		unsigned short	*gridPos;
		int				lat, lng;
		vec3_t			normal;
#if ACCURATE_LIGHTGRID_SAMPLING
		vec3_t			gridOrg;
		VectorCopy( startGridOrg, gridOrg );
#endif

		factor = 1.0;
		gridPos = startGridPos;
		for ( j = 0 ; j < 3 ; j++ ) {
			if ( i & (1<<j) ) {
				factor *= frac[j];
				gridPos += gridStep[j];
#if ACCURATE_LIGHTGRID_SAMPLING
				gridOrg[j] += tr.world->lightGridSize[j];
#endif
			} else {
				factor *= (1.0 - frac[j]);
			}
		}

		if (gridPos >= tr.world->lightGridArray + tr.world->numGridArrayElements)
		{//we've gone off the array somehow
			continue;
		}
		data = tr.world->lightGridData + *gridPos;

		if ( data->styles[0] == LS_NONE ) 
		{
			continue;	// ignore samples in walls
		}

#if 0
		if ( !SV_inPVS( startLightOrigin, gridOrg ) )
		{
			continue;
		}
#endif

		totalFactor += factor;

		for(j=0;j<MAXLIGHTMAPS;j++)
		{
			if (data->styles[j] != LS_NONE)
			{
				const byte	style= data->styles[j];

				ent->ambientLight[0] += factor * data->ambientLight[j][0] * styleColors[style][0] / 255.0f;
				ent->ambientLight[1] += factor * data->ambientLight[j][1] * styleColors[style][1] / 255.0f;
				ent->ambientLight[2] += factor * data->ambientLight[j][2] * styleColors[style][2] / 255.0f;

				ent->directedLight[0] += factor * data->directLight[j][0] * styleColors[style][0] / 255.0f;
				ent->directedLight[1] += factor * data->directLight[j][1] * styleColors[style][1] / 255.0f;
				ent->directedLight[2] += factor * data->directLight[j][2] * styleColors[style][2] / 255.0f;
			}
			else
			{
				break;
			}
		}

		lat = data->latLong[1];
		lng = data->latLong[0];
		lat *= (FUNCTABLE_SIZE/256);
		lng *= (FUNCTABLE_SIZE/256);

		// decode X as cos( lat ) * sin( long )
		// decode Y as sin( lat ) * sin( long )
		// decode Z as cos( long )

		normal[0] = tr.sinTable[(lat+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK] * tr.sinTable[lng];
		normal[1] = tr.sinTable[lat] * tr.sinTable[lng];
		normal[2] = tr.sinTable[(lng+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK];

		VectorMA( direction, factor, normal, direction );

#if ACCURATE_LIGHTGRID_SAMPLING
		if ( r_debugLight->integer && ent->e.hModel == -1 )
		{
			//draw 	
			refEntity_t refEnt;
			refEnt.hModel = 0;
			refEnt.ghoul2 = NULL;
			refEnt.renderfx = 0;
			VectorCopy( gridOrg, refEnt.origin );
			vectoangles( normal, refEnt.angles );
			AnglesToAxis( refEnt.angles, refEnt.axis );
			refEnt.reType = RT_MODEL;
			RE_AddRefEntityToScene( &refEnt );

			refEnt.renderfx = RF_DEPTHHACK;
			refEnt.reType = RT_SPRITE;
			refEnt.customShader = RE_RegisterShader( "gfx/misc/debugAmbient" );
			refEnt.shaderRGBA[0] = data->ambientLight[0][0];
			refEnt.shaderRGBA[1] = data->ambientLight[0][1];
			refEnt.shaderRGBA[2] = data->ambientLight[0][2];
			refEnt.shaderRGBA[3] = 255;
			refEnt.radius = factor*50+2.0f; // maybe always give it a minimum size?
			refEnt.rotation = 0;			// don't let the sprite wobble around
			RE_AddRefEntityToScene( &refEnt );

			refEnt.reType = RT_LINE;
			refEnt.customShader = RE_RegisterShader( "gfx/misc/debugArrow" );
			refEnt.shaderRGBA[0] = data->directLight[0][0];
			refEnt.shaderRGBA[1] = data->directLight[0][1];
			refEnt.shaderRGBA[2] = data->directLight[0][2];
			refEnt.shaderRGBA[3] = 255;
			VectorCopy( refEnt.origin, refEnt.oldorigin );
			VectorMA( gridOrg, (factor*-255) - 2.0f, normal, refEnt.origin ); // maybe always give it a minimum length
			refEnt.radius = 1.5f;
			RE_AddRefEntityToScene( &refEnt );
		}
#endif
	}

	if ( totalFactor > 0 && totalFactor < 0.99 ) 
	{
		totalFactor = 1.0 / totalFactor;
		VectorScale( ent->ambientLight, totalFactor, ent->ambientLight );
		VectorScale( ent->directedLight, totalFactor, ent->directedLight );
	}

	VectorScale( ent->ambientLight, r_ambientScale->value, ent->ambientLight );
	VectorScale( ent->directedLight, r_directedScale->value, ent->directedLight );

	VectorNormalize2( direction, ent->lightDir );
}


/*
===============
LogLight
===============
*/
static void LogLight( trRefEntity_t *ent ) {
	int	max1, max2;

	/*
	if ( !(ent->e.renderfx & RF_FIRST_PERSON ) ) {
		return;
	}
	*/

	max1 = VectorLength( ent->ambientLight );
	/*
	max1 = ent->ambientLight[0];
	if ( ent->ambientLight[1] > max1 ) {
		max1 = ent->ambientLight[1];
	} else if ( ent->ambientLight[2] > max1 ) {
		max1 = ent->ambientLight[2];
	}
	*/

	max2 = VectorLength( ent->directedLight );
	/*
	max2 = ent->directedLight[0];
	if ( ent->directedLight[1] > max2 ) {
		max2 = ent->directedLight[1];
	} else if ( ent->directedLight[2] > max2 ) {
		max2 = ent->directedLight[2];
	}
	*/

	ri.Printf( PRINT_ALL, "amb:%i  dir:%i  direction: (%4.2f, %4.2f, %4.2f)\n", max1, max2, ent->lightDir[0], ent->lightDir[1], ent->lightDir[2] );
}

/*
=================
R_SetupEntityLighting

Calculates all the lighting values that will be used
by the Calc_* functions
=================
*/
void R_SetupEntityLighting( const trRefdef_t *refdef, trRefEntity_t *ent ) {
	int				i;
	dlight_t		*dl;
	float			power;
	vec3_t			dir;
	float			d;
	vec3_t			lightDir;
	vec3_t			lightOrigin;

	// lighting calculations 
	if ( ent->lightingCalculated ) {
		return;
	}
	ent->lightingCalculated = qtrue;

	//
	// trace a sample point down to find ambient light
	//
	if ( ent->e.renderfx & RF_LIGHTING_ORIGIN ) {
		// seperate lightOrigins are needed so an object that is
		// sinking into the ground can still be lit, and so
		// multi-part models can be lit identically
		VectorCopy( ent->e.lightingOrigin, lightOrigin );
	} else {
		VectorCopy( ent->e.origin, lightOrigin );
	}

	// if NOWORLDMODEL, only use dynamic lights (menu system, etc)
	if ( !(refdef->rdflags & RDF_NOWORLDMODEL ) 
		&& tr.world->lightGridData ) {
		R_SetupEntityLightingGrid( ent );
	} else {
		ent->ambientLight[0] = ent->ambientLight[1] = 
			ent->ambientLight[2] = tr.identityLight * 150;
		ent->directedLight[0] = ent->directedLight[1] = 
			ent->directedLight[2] = tr.identityLight * 150;
		VectorCopy( tr.sunDirection, ent->lightDir );
	}

	// bonus items and view weapons have a fixed minimum add
	if (  ent->e.renderfx & RF_MORELIGHT  ) {
		ent->ambientLight[0] += tr.identityLight * 96;
		ent->ambientLight[1] += tr.identityLight * 96;
		ent->ambientLight[2] += tr.identityLight * 96;
	}
	else {
		// give everything a minimum light add
		ent->ambientLight[0] += tr.identityLight * 32;
		ent->ambientLight[1] += tr.identityLight * 32;
		ent->ambientLight[2] += tr.identityLight * 32;
	}

	//
	// modify the light by dynamic lights
	//
	d = VectorLength( ent->directedLight );
	VectorScale( ent->lightDir, d, lightDir );

	for ( i = 0 ; i < refdef->num_dlights ; i++ ) {
		dl = &refdef->dlights[i];
		VectorSubtract( dl->origin, lightOrigin, dir );
		d = VectorNormalize( dir );

		power = DLIGHT_AT_RADIUS * ( dl->radius * dl->radius );
		if ( d < DLIGHT_MINIMUM_RADIUS ) {
			d = DLIGHT_MINIMUM_RADIUS;
		}
		d = power / ( d * d );

		VectorMA( ent->directedLight, d, dl->color, ent->directedLight );
		VectorMA( lightDir, d, dir, lightDir );
	}

	// clamp ambient
	for ( i = 0 ; i < 3 ; i++ ) {
		if ( ent->ambientLight[i] > tr.identityLightByte ) {
			ent->ambientLight[i] = tr.identityLightByte;
		}
	}

	if ( r_debugLight->integer ) {
		LogLight( ent );
	}

	// save out the byte packet version
	((byte *)&ent->ambientLightInt)[0] = Q_ftol( ent->ambientLight[0] );
	((byte *)&ent->ambientLightInt)[1] = Q_ftol( ent->ambientLight[1] );
	((byte *)&ent->ambientLightInt)[2] = Q_ftol( ent->ambientLight[2] );
	((byte *)&ent->ambientLightInt)[3] = 0xff;
	
	// transform the direction to local space
	VectorNormalize( lightDir );
	ent->lightDir[0] = DotProduct( lightDir, ent->e.axis[0] );
	ent->lightDir[1] = DotProduct( lightDir, ent->e.axis[1] );
	ent->lightDir[2] = DotProduct( lightDir, ent->e.axis[2] );
}

//pass in origin
qboolean RE_GetLighting( const vec3_t origin, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir) {
	trRefEntity_t tr_ent;

	if ( !tr.world || !tr.world->lightGridData) {
		ambientLight[0] = ambientLight[1] = ambientLight[2] = 255.0;
		directedLight[0] = directedLight[1] = directedLight[2] = 255.0;
		VectorCopy( tr.sunDirection, lightDir );
		return qfalse;
	}
	memset (&tr_ent, 0, sizeof(tr_ent) );

	if ( ambientLight[0] == 666 )
	{//HAX0R
		tr_ent.e.hModel = -1;
	}

	VectorCopy (origin, tr_ent.e.origin);
	R_SetupEntityLightingGrid( &tr_ent );
	VectorCopy ( tr_ent.ambientLight,	ambientLight);
	VectorCopy ( tr_ent.directedLight,	directedLight);
	VectorCopy ( tr_ent.lightDir,		lightDir);
	return qtrue;
}

/*
=================
R_LightForPoint
=================
*/
int R_LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir )
{
	trRefEntity_t ent;
	
	// bk010103 - this segfaults with -nolight maps
	if ( tr.world->lightGridData == NULL )
	  return qfalse;

	memset(&ent, 0, sizeof(ent));
	VectorCopy( point, ent.e.origin );
	R_SetupEntityLightingGrid( &ent );
	VectorCopy(ent.ambientLight, ambientLight);
	VectorCopy(ent.directedLight, directedLight);
	VectorCopy(ent.lightDir, lightDir);

	return qtrue;
}
