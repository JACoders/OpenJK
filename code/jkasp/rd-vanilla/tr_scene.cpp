/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2005 - 2015, ioquake3 contributors
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

#include "../server/exe_headers.h"

#include "tr_local.h"

int			r_firstSceneDrawSurf;

int			r_numdlights;
int			r_firstSceneDlight;

int			r_numentities;
int			r_firstSceneEntity;

int			r_numpolys;
int			r_firstScenePoly;

int			r_numpolyverts;

int			skyboxportal;
int			drawskyboxportal;

/*
====================
R_InitNextFrame

====================
*/
void R_InitNextFrame( void ) {

	backEndData->commands.used = 0;

	r_firstSceneDrawSurf = 0;

	r_numdlights = 0;
	r_firstSceneDlight = 0;

	r_numentities = 0;
	r_firstSceneEntity = 0;

	r_numpolys = 0;
	r_firstScenePoly = 0;

	r_numpolyverts = 0;
}


/*
====================
RE_ClearScene

====================
*/
void RE_ClearScene( void ) {
	r_firstSceneDlight = r_numdlights;
	r_firstSceneEntity = r_numentities;
	r_firstScenePoly = r_numpolys;
	tr.refdef.rdflags &= ~(RDF_doLAGoggles|RDF_doFullbright);	//probably not needed since it gets copied over in RE_RenderScene
	tr.refdef.doLAGoggles = qfalse;
}

/*
===========================================================================

DISCRETE POLYS

===========================================================================
*/

/*
=====================
R_AddPolygonSurfaces

Adds all the scene's polys into this view's drawsurf list
=====================
*/
void R_AddPolygonSurfaces( void ) {
	int			i;
	shader_t	*sh;
	srfPoly_t	*poly;

	tr.currentEntityNum = REFENTITYNUM_WORLD;
	tr.shiftedEntityNum = tr.currentEntityNum << QSORT_REFENTITYNUM_SHIFT;

	for ( i = 0, poly = tr.refdef.polys; i < tr.refdef.numPolys ; i++, poly++ ) {
		sh = R_GetShaderByHandle( poly->hShader );
		R_AddDrawSurf( ( surfaceType_t * )poly, sh, poly->fogIndex, qfalse );
	}
}

/*
=====================
RE_AddPolyToScene

=====================
*/
void RE_AddPolyToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts ) {
	srfPoly_t	*poly;
	int			i;
	int			fogIndex = 0;
	fog_t		*fog;
	vec3_t		bounds[2];

	if ( !tr.registered ) {
		return;
	}

	if ( !hShader ) {
#ifndef FINAL_BUILD
		Com_DPrintf( S_COLOR_YELLOW"WARNING: RE_AddPolyToScene: NULL poly shader\n");
#endif
		return;
	}

	if ( r_numpolyverts + numVerts >= MAX_POLYVERTS || r_numpolys >= MAX_POLYS ) {
      /*
      NOTE TTimo this was initially a PRINT_WARNING
      but it happens a lot with high fighting scenes and particles
      since we don't plan on changing the const and making for room for those effects
      simply cut this message to developer only
      */
		ri.Printf( PRINT_DEVELOPER, S_COLOR_YELLOW  "WARNING: RE_AddPolyToScene: r_max_polys or r_max_polyverts reached\n");
		return;
	}

	poly = &backEndData->polys[r_numpolys];
	poly->surfaceType = SF_POLY;
	poly->hShader = hShader;
	poly->numVerts = numVerts;
	poly->verts = &backEndData->polyVerts[r_numpolyverts];

	memcpy( poly->verts, verts, numVerts * sizeof( *verts ) );
	r_numpolys++;
	r_numpolyverts += numVerts;

	// see if it is in a fog volume
	if ( !tr.world || tr.world->numfogs == 1) {
		fogIndex = 0;
	} else {
		// find which fog volume the poly is in
		VectorCopy( poly->verts[0].xyz, bounds[0] );
		VectorCopy( poly->verts[0].xyz, bounds[1] );
		for ( i = 1 ; i < poly->numVerts ; i++ ) {
			AddPointToBounds( poly->verts[i].xyz, bounds[0], bounds[1] );
		}
		for ( int fI = 1 ; fI < tr.world->numfogs ; fI++ ) {
			fog = &tr.world->fogs[fI];
			if ( bounds[0][0] >= fog->bounds[0][0]
				&& bounds[0][1] >= fog->bounds[0][1]
				&& bounds[0][2] >= fog->bounds[0][2]
				&& bounds[1][0] <= fog->bounds[1][0]
				&& bounds[1][1] <= fog->bounds[1][1]
				&& bounds[1][2] <= fog->bounds[1][2] )
			{//completely in this one
				fogIndex = fI;
				break;
			}
			else if ( ( bounds[0][0] >= fog->bounds[0][0] && bounds[0][1] >= fog->bounds[0][1] && bounds[0][2] >= fog->bounds[0][2] &&
						bounds[0][0] <= fog->bounds[1][0] && bounds[0][1] <= fog->bounds[1][1] && bounds[0][2] <= fog->bounds[1][2]) ||
				( bounds[1][0] >= fog->bounds[0][0] && bounds[1][1] >= fog->bounds[0][1] && bounds[1][2] >= fog->bounds[0][2] &&
					bounds[1][0] <= fog->bounds[1][0] && bounds[1][1] <= fog->bounds[1][1] && bounds[1][2] <= fog->bounds[1][2] ) )
			{//partially in this one
				if ( tr.refdef.fogIndex == fI || R_FogParmsMatch( tr.refdef.fogIndex, fI ) )
				{//take new one only if it's the same one that the viewpoint is in
					fogIndex = fI;
					break;
				}
				else if ( !fogIndex )
				{//didn't find one yet, so use this one
					fogIndex = fI;
				}
			}
		}
	}
	poly->fogIndex = fogIndex;
}


//=================================================================================


/*
=====================
RE_AddRefEntityToScene

=====================
*/
void RE_AddRefEntityToScene( const refEntity_t *ent ) {
	if ( !tr.registered ) {
		return;
	}
	if ( r_numentities >= MAX_REFENTITIES ) {
#ifndef FINAL_BUILD
		ri.Printf( PRINT_WARNING, "WARNING: RE_AddRefEntityToScene: too many entities\n");
#endif
		return;
	}
	if ( ent->reType < 0 || ent->reType >= RT_MAX_REF_ENTITY_TYPE ) {
		Com_Error( ERR_DROP, "RE_AddRefEntityToScene: bad reType %i", ent->reType );
	}

	backEndData->entities[r_numentities].e = *ent;
	backEndData->entities[r_numentities].lightingCalculated = qfalse;

	r_numentities++;
}


/*
=====================
RE_AddLightToScene

=====================
*/
void RE_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b ) {
	dlight_t	*dl;

	if ( !tr.registered ) {
		return;
	}
	if ( r_numdlights >= MAX_DLIGHTS ) {
		return;
	}
	if ( intensity <= 0 ) {
		return;
	}
	dl = &backEndData->dlights[r_numdlights++];
	VectorCopy (org, dl->origin);
	dl->radius = intensity;
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;
}


/*
@@@@@@@@@@@@@@@@@@@@@
RE_RenderScene

Draw a 3D view into a part of the window, then return
to 2D drawing.

Rendering a scene may require multiple views to be rendered
to handle mirrors,
@@@@@@@@@@@@@@@@@@@@@
*/
extern int	recursivePortalCount;
void RE_RenderScene( const refdef_t *fd ) {
	viewParms_t		parms;
	int				startTime;
	static int		lastTime = 0;

	if ( !tr.registered ) {
		return;
	}
	GLimp_LogComment( "====== RE_RenderScene =====\n" );

	if ( r_norefresh->integer ) {
		return;
	}

	startTime = ri.Milliseconds();

	if (!tr.world && !( fd->rdflags & RDF_NOWORLDMODEL ) ) {
		Com_Error (ERR_DROP, "R_RenderScene: NULL worldmodel");
	}

//	memcpy( tr.refdef.text, fd->text, sizeof( tr.refdef.text ) );

	tr.refdef.x = fd->x;
	tr.refdef.y = fd->y;
	tr.refdef.width = fd->width;
	tr.refdef.height = fd->height;
	tr.refdef.fov_x = fd->fov_x;
	tr.refdef.fov_y = fd->fov_y;

	VectorCopy( fd->vieworg, tr.refdef.vieworg );
	VectorCopy( fd->viewaxis[0], tr.refdef.viewaxis[0] );
	VectorCopy( fd->viewaxis[1], tr.refdef.viewaxis[1] );
	VectorCopy( fd->viewaxis[2], tr.refdef.viewaxis[2] );

	tr.refdef.time = fd->time;
	tr.refdef.frametime = fd->time - lastTime;
	tr.refdef.rdflags = fd->rdflags;
	// Ignore my last there. This actually breaks the rest of the game as well, causing massive issues.
	// We need to figure out what's going on in fd->rdflags first :S .. I'm half-tempted to just revert
	// back to JK2 and use a qbool for it --eez

	if (fd->rdflags & RDF_SKYBOXPORTAL)
	{
		skyboxportal = 1;
	}
	else
	{
		// cdr - only change last time for the real render, not the portal
		lastTime = fd->time;
	}

	if (fd->rdflags & RDF_DRAWSKYBOX)
	{
		drawskyboxportal = 1;
	}
	else
	{
		drawskyboxportal = 0;
	}

	// copy the areamask data over and note if it has changed, which
	// will force a reset of the visible leafs even if the view hasn't moved
	tr.refdef.areamaskModified = qfalse;
	if ( ! (tr.refdef.rdflags & RDF_NOWORLDMODEL) ) {
		int		areaDiff;
		int		i;

		// compare the area bits
		areaDiff = 0;
		for (i = 0 ; i < MAX_MAP_AREA_BYTES/4 ; i++) {
			areaDiff |= ((int *)tr.refdef.areamask)[i] ^ ((int *)fd->areamask)[i];
			((int *)tr.refdef.areamask)[i] = ((int *)fd->areamask)[i];
		}

		if ( areaDiff ) {
			// a door just opened or something
			tr.refdef.areamaskModified = qtrue;
		}
	}


	// derived info

	tr.refdef.floatTime = tr.refdef.time * 0.001;

	tr.refdef.numDrawSurfs = r_firstSceneDrawSurf;
	tr.refdef.drawSurfs = backEndData->drawSurfs;

	tr.refdef.num_entities = r_numentities - r_firstSceneEntity;
	tr.refdef.entities = &backEndData->entities[r_firstSceneEntity];
	tr.refdef.num_dlights = r_numdlights - r_firstSceneDlight;
	tr.refdef.dlights = &backEndData->dlights[r_firstSceneDlight];

	tr.refdef.numPolys = r_numpolys - r_firstScenePoly;
	tr.refdef.polys = &backEndData->polys[r_firstScenePoly];

	// turn off dynamic lighting globally by clearing all the
	// dlights if it needs to be disabled or if vertex lighting is enabled
	if ( r_dynamiclight->integer == 0 ||
		 r_vertexLight->integer == 1 ) {
		tr.refdef.num_dlights = 0;
	}

	// a single frame may have multiple scenes draw inside it --
	// a 3D game view, 3D status bar renderings, 3D menus, etc.
	// They need to be distinguished by the light flare code, because
	// the visibility state for a given surface may be different in
	// each scene / view.
	tr.frameSceneNum++;
	tr.sceneCount++;

	// setup view parms for the initial view
	//
	// set up viewport
	// The refdef takes 0-at-the-top y coordinates, so
	// convert to GL's 0-at-the-bottom space
	//
	memset( &parms, 0, sizeof( parms ) );
	parms.viewportX = tr.refdef.x;
	parms.viewportY = glConfig.vidHeight - ( tr.refdef.y + tr.refdef.height );
	parms.viewportWidth = tr.refdef.width;
	parms.viewportHeight = tr.refdef.height;
	parms.isPortal = qfalse;

	parms.fovX = tr.refdef.fov_x;
	parms.fovY = tr.refdef.fov_y;

	VectorCopy( fd->vieworg, parms.ori.origin );
	VectorCopy( fd->viewaxis[0], parms.ori.axis[0] );
	VectorCopy( fd->viewaxis[1], parms.ori.axis[1] );
	VectorCopy( fd->viewaxis[2], parms.ori.axis[2] );

	VectorCopy( fd->vieworg, parms.pvsOrigin );

	recursivePortalCount = 0;
	R_RenderView( &parms );

	// the next scene rendered in this frame will tack on after this one
	r_firstSceneDrawSurf = tr.refdef.numDrawSurfs;
	r_firstSceneEntity = r_numentities;
	r_firstSceneDlight = r_numdlights;
	r_firstScenePoly = r_numpolys;

	tr.frontEndMsec += ri.Milliseconds() - startTime;
	RE_RenderWorldEffects();
}
