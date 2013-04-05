// leave this as first line for PCH reasons...
//
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


/*
====================
R_ToggleSmpFrame

====================
*/
void R_ToggleSmpFrame( void ) {
	if ( r_smp->integer ) {
		// use the other buffers next frame, because another CPU
		// may still be rendering into the current ones
		tr.smpFrame^= 1;
	} else {
		tr.smpFrame = 0;
	}

	backEndData[tr.smpFrame]->commands.used = 0;

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
	tr.refdef.doLAGoggles = qfalse;
	tr.refdef.doFullbright = qfalse;
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

	tr.currentEntityNum = ENTITYNUM_WORLD;
	tr.shiftedEntityNum = tr.currentEntityNum << QSORT_ENTITYNUM_SHIFT;

	for ( i = 0, poly = tr.refdef.polys; i < tr.refdef.numPolys ; i++, poly++ ) {
		sh = R_GetShaderByHandle( poly->hShader );
#ifdef _NPATCH
		R_AddDrawSurf( ( surfaceType_t * )poly, sh, poly->fogIndex, qfalse, 0 );
#else
		R_AddDrawSurf( ( surfaceType_t * )poly, sh, poly->fogIndex, qfalse );
#endif // _NPATCH
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
		ri.Printf( PRINT_WARNING, "WARNING: RE_AddPolyToScene: NULL poly shader\n");
		return;
	}

	if ( r_numpolyverts + numVerts > MAX_POLYVERTS || r_numpolys >= MAX_POLYS ) {
		return;
	}

	poly = &backEndData[tr.smpFrame]->polys[r_numpolys];
	poly->surfaceType = SF_POLY;
	poly->hShader = hShader;
	poly->numVerts = numVerts;
	poly->verts = &backEndData[tr.smpFrame]->polyVerts[r_numpolyverts];
	
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
	if ( r_numentities >= ENTITYNUM_WORLD ) {
#ifndef FINAL_BUILD
		ri.Printf( PRINT_WARNING, "WARNING: RE_AddRefEntityToScene: too many entities\n");
#endif
		return;
	}
	if ( ent->reType < 0 || ent->reType >= RT_MAX_REF_ENTITY_TYPE ) {
		ri.Error( ERR_DROP, "RE_AddRefEntityToScene: bad reType %i", ent->reType );
	}

	backEndData[tr.smpFrame]->entities[r_numentities].e = *ent;
	backEndData[tr.smpFrame]->entities[r_numentities].lightingCalculated = qfalse;

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
	dl = &backEndData[tr.smpFrame]->dlights[r_numdlights++];
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
		ri.Error (ERR_DROP, "R_RenderScene: NULL worldmodel");
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
	lastTime = fd->time;

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
	tr.refdef.drawSurfs = backEndData[tr.smpFrame]->drawSurfs;

	tr.refdef.num_entities = r_numentities - r_firstSceneEntity;
	tr.refdef.entities = &backEndData[tr.smpFrame]->entities[r_firstSceneEntity];

	tr.refdef.num_dlights = r_numdlights - r_firstSceneDlight;
	tr.refdef.dlights = &backEndData[tr.smpFrame]->dlights[r_firstSceneDlight];

	tr.refdef.numPolys = r_numpolys - r_firstScenePoly;
	tr.refdef.polys = &backEndData[tr.smpFrame]->polys[r_firstScenePoly];

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

	VectorCopy( fd->vieworg, parms.or.origin );
	VectorCopy( fd->viewaxis[0], parms.or.axis[0] );
	VectorCopy( fd->viewaxis[1], parms.or.axis[1] );
	VectorCopy( fd->viewaxis[2], parms.or.axis[2] );

	VectorCopy( fd->vieworg, parms.pvsOrigin );

#ifdef _NPATCH
	RE_NPatchLevel(r_npatches->integer);
#endif // _NPATCH
	
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
