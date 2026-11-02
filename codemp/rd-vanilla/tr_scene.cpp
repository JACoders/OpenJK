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

#include "tr_local.h"

#include "ghoul2/G2.h"
#include "ghoul2/g2_local.h"
#include "qcommon/matcomp.h"
#include "qcommon/disablewarnings.h"

static	int			r_firstSceneDrawSurf;

static	int			r_numdlights;
static	int			r_firstSceneDlight;

static	int			r_numentities;
static	int			r_firstSceneEntity;
static	int			r_numminientities;
static	int			r_firstSceneMiniEntity;
static	int			refEntParent = -1;

static	int			r_numpolys;
static	int			r_firstScenePoly;

static	int			r_numpolyverts;

int					skyboxportal;
int					drawskyboxportal;

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
	refEntParent = -1;
	r_numminientities = 0;
	r_firstSceneMiniEntity = 0;

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
	refEntParent = -1;
	r_firstSceneMiniEntity = r_numminientities;
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
		R_AddDrawSurf( (surfaceType_t *)poly, sh, poly->fogIndex, qfalse );
	}
}

/*
=====================
RE_AddPolyToScene

=====================
*/
void RE_AddPolyToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys ) {
	srfPoly_t	*poly;
	int			i, j;
	int			fogIndex;
	fog_t		*fog;
	vec3_t		bounds[2];

	if ( !tr.registered ) {
		return;
	}

	if ( !hShader ) {
		ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: RE_AddPolyToScene: NULL poly shader\n");
		return;
	}

	for ( j = 0; j < numPolys; j++ ) {
		if ( r_numpolyverts + numVerts >= max_polyverts || r_numpolys >= max_polys ) {
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

		memcpy( poly->verts, &verts[numVerts*j], numVerts * sizeof( *verts ) );

		// done.
		r_numpolys++;
		r_numpolyverts += numVerts;

		// if no world is loaded
		if ( tr.world == NULL ) {
			fogIndex = 0;
		}
		// see if it is in a fog volume
		else if ( tr.world->numfogs == 1 ) {
			fogIndex = 0;
		} else {
			// find which fog volume the poly is in
			VectorCopy( poly->verts[0].xyz, bounds[0] );
			VectorCopy( poly->verts[0].xyz, bounds[1] );
			for ( i = 1 ; i < poly->numVerts ; i++ ) {
				AddPointToBounds( poly->verts[i].xyz, bounds[0], bounds[1] );
			}
			for ( fogIndex = 1 ; fogIndex < tr.world->numfogs ; fogIndex++ ) {
				fog = &tr.world->fogs[fogIndex];
				if ( bounds[1][0] >= fog->bounds[0][0]
					&& bounds[1][1] >= fog->bounds[0][1]
					&& bounds[1][2] >= fog->bounds[0][2]
					&& bounds[0][0] <= fog->bounds[1][0]
					&& bounds[0][1] <= fog->bounds[1][1]
					&& bounds[0][2] <= fog->bounds[1][2] ) {
					break;
				}
			}
			if ( fogIndex == tr.world->numfogs ) {
				fogIndex = 0;
			}
		}
		poly->fogIndex = fogIndex;
	}
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
		ri.Printf(PRINT_DEVELOPER, "RE_AddRefEntityToScene: Dropping refEntity, reached MAX_REFENTITIES\n");
		return;
	}

	/*if ( Q_isnan(ent->origin[0]) || Q_isnan(ent->origin[1]) || Q_isnan(ent->origin[2]) ) {
		static qboolean firstTime = qtrue;
		if (firstTime) {
			firstTime = qfalse;
			ri.Printf( PRINT_WARNING, "RE_AddRefEntityToScene passed a refEntity which has an origin with a NaN component\n");
		}
		return;
	}*/

	assert(!ent || ent->renderfx >= 0);

	if (ent->reType == RT_ENT_CHAIN)
	{ //minirefents must die.
		return;
	}

#ifdef _DEBUG
	if (ent->reType == RT_MODEL)
	{
		assert(ent->hModel || ent->ghoul2 || ent->customShader);
	}
#endif

	if ( (int)ent->reType < 0 || ent->reType >= RT_MAX_REF_ENTITY_TYPE ) {
		Com_Error( ERR_DROP, "RE_AddRefEntityToScene: bad reType %i", ent->reType );
	}

	backEndData->entities[r_numentities].e = *ent;
	backEndData->entities[r_numentities].lightingCalculated = qfalse;

	if (ent->ghoul2)
	{
		CGhoul2Info_v	&ghoul2 = *((CGhoul2Info_v *)ent->ghoul2);

		if (!ghoul2[0].mModel)
		{
			ri.Printf( PRINT_ALL, "Your ghoul2 instance has no model!\n");
		}
	}

	/*
	if (ent->reType == RT_ENT_CHAIN)
	{
		refEntParent = r_numentities;
		backEndData->entities[r_numentities].e.uRefEnt.uMini.miniStart = r_numminientities - r_firstSceneMiniEntity;
		backEndData->entities[r_numentities].e.uRefEnt.uMini.miniCount = 0;
	}
	else
	{
	*/
		refEntParent = -1;
	//}

	r_numentities++;
}


/************************************************************************************************
 * RE_AddMiniRefEntityToScene                                                                   *
 *    Adds a mini ref ent to the scene.  If the input parameter is null, it signifies the end   *
 *    of the chain.  Otherwise, if there is a valid chain parent, it will be added to that.     *
 *    If there is no parent, it will be added as a regular ref ent.                             *
 *                                                                                              *
 * Input                                                                                        *
 *    ent: the mini ref ent to be added                                                         *
 *                                                                                              *
 * Output / Return                                                                              *
 *    none                                                                                      *
 *                                                                                              *
 ************************************************************************************************/
void RE_AddMiniRefEntityToScene( const miniRefEntity_t *ent )
{
#if 0
	refEntity_t		*parent;
#endif

	if ( !tr.registered )
	{
		return;
	}
	if (!ent)
	{
		refEntParent = -1;
		return;
	}

#if 1 //i hate you minirefent!
	refEntity_t		tempEnt;

	memcpy(&tempEnt, ent, sizeof(*ent));
	memset(((char *)&tempEnt)+sizeof(*ent), 0, sizeof(tempEnt) - sizeof(*ent));
	RE_AddRefEntityToScene(&tempEnt);
#else

	if ( ent->reType < 0 || ent->reType >= RT_MAX_REF_ENTITY_TYPE )
	{
		Com_Error( ERR_DROP, "RE_AddMiniRefEntityToScene: bad reType %i", ent->reType );
	}

	if (!r_numentities || refEntParent == -1 || r_numminientities >= MAX_MINI_ENTITIES)
	{ //rww - add it as a refent also if we run out of minis
//		Com_Error( ERR_DROP, "RE_AddMiniRefEntityToScene: mini without parent ref ent");
		refEntity_t		tempEnt;

		memcpy(&tempEnt, ent, sizeof(*ent));
		memset(((char *)&tempEnt)+sizeof(*ent), 0, sizeof(tempEnt) - sizeof(*ent));
		RE_AddRefEntityToScene(&tempEnt);
		return;
	}

	parent = &backEndData->entities[refEntParent].e;
	parent->uRefEnt.uMini.miniCount++;

	backEndData->miniEntities[r_numminientities].e = *ent;
	r_numminientities++;
#endif
}

/*
=====================
RE_AddDynamicLightToScene

=====================
*/
void RE_AddDynamicLightToScene( const vec3_t org, float intensity, float r, float g, float b, int additive ) {
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
	dl->additive = additive;
}

/*
=====================
RE_AddLightToScene

=====================
*/
void RE_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b ) {
	RE_AddDynamicLightToScene( org, intensity, r, g, b, qfalse );
}

/*
=====================
RE_AddAdditiveLightToScene

=====================
*/
void RE_AddAdditiveLightToScene( const vec3_t org, float intensity, float r, float g, float b ) {
	RE_AddDynamicLightToScene( org, intensity, r, g, b, qtrue );
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
void RE_RenderWorldEffects(void);
void RE_RenderAutoMap(void);
void RE_SetDepthOfField( float focus, float range, float blur ) {
	tr.dofNext[0] = focus;
	tr.dofNext[1] = range > 1.0f ? range : 1.0f;
	tr.dofNext[2] = blur > 0.0f ? blur : 0.0f;
}

void RE_SetScreenFilter( const screenFilter_t *filter ) {
	tr.filterNextOn = (qboolean)( filter && filter->strength > 0.0f );
	if ( tr.filterNextOn )
		tr.filterNext = *filter;
}

int RE_GetFogs( vec3_t *colors, float *depths, vec3_t *mins, vec3_t *maxs, int *global, int max ) {
	int count;

	*global = -1;
	if ( !tr.world || !tr.world->fogs )
		return 0;
	count = tr.world->fogsSaved ? tr.world->numMapFogs : tr.world->numfogs;
	count = count < max ? count : max;
	for ( int i = 1; i < count; i++ ) {
		const fog_t *fog = &tr.world->fogs[i];
		const fogParms_t *parms = fog->changed ? &fog->original : &fog->parms;

		VectorCopy( parms->color, colors[i] );
		depths[i] = parms->depthForOpaque;
		VectorCopy( fog->bounds[0], mins[i] );
		VectorCopy( fog->bounds[1], maxs[i] );
	}
	if ( tr.world->globalFog > 0 && tr.world->globalFog < count )
		*global = tr.world->globalFog;
	return count;
}

void RE_SetFog( int fog, const float *color, float depth ) {
	fog_t *f;
	float d;

	if ( !tr.world || !tr.world->fogs || fog <= 0 || fog >= tr.world->numfogs )
		return;
	f = &tr.world->fogs[fog];
	if ( !f->changed ) {
		f->original = f->parms;
		f->changed = qtrue;
	}
	f->overridden = (qboolean)( color != NULL );
	if ( color ) {
		VectorCopy( color, f->parms.color );
		f->parms.depthForOpaque = depth > 1.0f ? depth : 1.0f;
	} else {
		f->parms = f->original;
	}
	f->colorInt = ColorBytes4( f->parms.color[0] * tr.identityLight, f->parms.color[1] * tr.identityLight,
		f->parms.color[2] * tr.identityLight, 1.0f );
	d = f->parms.depthForOpaque < 1 ? 1 : f->parms.depthForOpaque;
	f->tcScale = 1.0f / ( d * 8 );
}

// a world surface's box; qfalse for a kind that has none
static qboolean R_SurfaceBounds( const msurface_t *surf, vec3_t mins, vec3_t maxs ) {
	ClearBounds( mins, maxs );
	switch ( *surf->data ) {
	case SF_FACE: {
		const srfSurfaceFace_t *face = (const srfSurfaceFace_t *)surf->data;

		for ( int i = 0; i < face->numPoints; i++ )
			AddPointToBounds( face->points[i], mins, maxs );
		return (qboolean)( face->numPoints > 0 );
	}
	case SF_GRID:
		VectorCopy( ( (const srfGridMesh_t *)surf->data )->meshBounds[0], mins );
		VectorCopy( ( (const srfGridMesh_t *)surf->data )->meshBounds[1], maxs );
		return qtrue;
	case SF_TRIANGLES:
		VectorCopy( ( (const srfTriangles_t *)surf->data )->bounds[0], mins );
		VectorCopy( ( (const srfTriangles_t *)surf->data )->bounds[1], maxs );
		return qtrue;
	default:
		return qfalse;
	}
}

int RE_AddFog( const vec3_t mins, const vec3_t maxs, const float *color, float depth ) {
	world_t *w = tr.world;
	fog_t *f;
	int index;
	float d;

	if ( !w || !w->fogs || w->numfogs >= w->maxFogs - 1 )
		return 0;
	// the map's own kept, to go back to
	if ( !w->fogsSaved ) {
		w->numMapFogs = w->numfogs;
		for ( int i = 0; i < w->numsurfaces; i++ )
			w->surfaces[i].mapFogIndex = w->surfaces[i].fogIndex;
		w->fogsSaved = qtrue;
	}
	index = w->numfogs++;
	f = &w->fogs[index];
	memset( f, 0, sizeof( *f ) );
	f->originalBrushNumber = -2;
	VectorCopy( mins, f->bounds[0] );
	VectorCopy( maxs, f->bounds[1] );
	VectorCopy( color, f->parms.color );
	f->parms.depthForOpaque = depth > 1.0f ? depth : 1.0f;
	f->colorInt = ColorBytes4( color[0] * tr.identityLight, color[1] * tr.identityLight, color[2] * tr.identityLight, 1.0f );
	d = f->parms.depthForOpaque < 1 ? 1 : f->parms.depthForOpaque;
	f->tcScale = 1.0f / ( d * 8 );
	// its top is what it's seen through from outside
	f->hasSurface = qtrue;
	VectorSet( f->surface, 0.0f, 0.0f, -1.0f );
	f->surface[3] = -maxs[2];

	// the surfaces in it, but not in one of the map's volumes or one added before
	for ( int i = 0; i < w->numsurfaces; i++ ) {
		msurface_t *surf = &w->surfaces[i];
		vec3_t smins, smaxs;

		if ( ( surf->mapFogIndex && surf->mapFogIndex != w->globalFog ) || surf->fogIndex != surf->mapFogIndex )
			continue;
		if ( !R_SurfaceBounds( surf, smins, smaxs ) )
			continue;
		if ( smins[0] > maxs[0] || smaxs[0] < mins[0] || smins[1] > maxs[1] || smaxs[1] < mins[1] || smins[2] > maxs[2] || smaxs[2] < mins[2] )
			continue;
		surf->fogIndex = index;
	}
	return index;
}

void RE_ClearAddedFogs( void ) {
	world_t *w = tr.world;

	if ( !w || !w->fogsSaved )
		return;
	w->numfogs = w->numMapFogs;
	for ( int i = 0; i < w->numsurfaces; i++ )
		w->surfaces[i].fogIndex = w->surfaces[i].mapFogIndex;
}

void RE_SetSkyLook( const float *tint, const vec3_t angles, const vec3_t shift ) {
	tr.skyLookOn = (qboolean)( tint != NULL );
	if ( !tint )
		return;
	VectorCopy( tint, tr.skyLookTint );
	VectorCopy( angles, tr.skyLookAngles );
	VectorCopy( shift, tr.skyLookShift );
}

void RE_SetSkyZoom( float zoom ) {
	tr.skyZoom = Com_Clamp( 0.1f, 10.0f, zoom );
}

// the sky look on this scene: a 3D skybox's view turned the other way round its sky and moved,
// the sky shader turned in the other views
static void R_SkyLook( const refdef_t *fd ) {
	matrix3_t axis;

	// the zoom: a 3D skybox's view narrower or wider, the sky shader drawn so in the backend
	tr.refdef.skyZoom = tr.skyZoom > 0.0f ? tr.skyZoom : 1.0f;
	if ( ( fd->rdflags & RDF_SKYBOXPORTAL ) && !( fd->rdflags & RDF_NOWORLDMODEL ) && tr.refdef.skyZoom != 1.0f ) {
		tr.refdef.fov_x = RAD2DEG( 2.0f * atanf( tanf( DEG2RAD( fd->fov_x ) * 0.5f ) / tr.refdef.skyZoom ) );
		tr.refdef.fov_y = RAD2DEG( 2.0f * atanf( tanf( DEG2RAD( fd->fov_y ) * 0.5f ) / tr.refdef.skyZoom ) );
	}

	tr.refdef.skyTinted = tr.refdef.skyTurned = qfalse;
	if ( !tr.skyLookOn || ( fd->rdflags & RDF_NOWORLDMODEL ) )
		return;
	VectorCopy( tr.skyLookTint, tr.refdef.skyTint );
	tr.refdef.skyTinted = (qboolean)( tr.skyLookTint[0] != 1.0f || tr.skyLookTint[1] != 1.0f || tr.skyLookTint[2] != 1.0f );
	if ( VectorCompare( tr.skyLookAngles, vec3_origin ) && ( !( fd->rdflags & RDF_SKYBOXPORTAL ) || VectorCompare( tr.skyLookShift, vec3_origin ) ) )
		return;
	AnglesToAxis( tr.skyLookAngles, axis );
	if ( fd->rdflags & RDF_SKYBOXPORTAL ) {
		// what lies along a direction in the turned sky lies along it turned back in the skybox
		for ( int i = 0; i < 3; i++ ) {
			for ( int j = 0; j < 3; j++ )
				tr.refdef.viewaxis[i][j] = DotProduct( fd->viewaxis[i], axis[j] );
		}
		VectorAdd( fd->vieworg, tr.skyLookShift, tr.refdef.vieworg );
	} else {
		tr.refdef.skyTurned = qtrue;
		for ( int i = 0; i < 3; i++ )
			VectorCopy( axis[i], tr.refdef.skyAxis[i] );
	}
}

void RE_SetSkyPortal( const vec3_t origin ) {
	if ( !origin ) {
		// the world's sky surfaces were left out for the portal's view: drawn again. A map's own
		// 3D skybox turns this back on with its next view
		if ( tr.portalSkyOn )
			skyboxportal = 0;
		tr.portalSkyOn = qfalse;
		return;
	}
	tr.portalSkyOn = qtrue;
	VectorCopy( origin, tr.portalSkyOrg );
}

// the client's sky portal before a world scene: the same view from its spot, as a 3D skybox's,
// without the scene's entities, lights and polys, which are left for the world scene after it
static void R_RenderPortalSky( const refdef_t *fd ) {
	const int numEntities = r_numentities, numMini = r_numminientities, numDlights = r_numdlights, numPolys = r_numpolys;
	refdef_t sky = *fd;

	VectorCopy( tr.portalSkyOrg, sky.vieworg );
	sky.rdflags |= RDF_SKYBOXPORTAL | RDF_DRAWSKYBOX;
	sky.rdflags &= ~RDF_AUTOMAP;
	// the snapshot's areas are the player's; from the spot, every area may be seen
	memset( sky.areamask, 0, sizeof( sky.areamask ) );
	r_numentities = r_firstSceneEntity;
	r_numminientities = r_firstSceneMiniEntity;
	r_numdlights = r_firstSceneDlight;
	r_numpolys = r_firstScenePoly;
	tr.inPortalSky = qtrue;
	RE_RenderScene( &sky );
	tr.inPortalSky = qfalse;
	r_numentities = numEntities;
	r_numminientities = numMini;
	r_numdlights = numDlights;
	r_numpolys = numPolys;
}

void RE_RenderScene( const refdef_t *fd ) {
	viewParms_t		parms;
	int				startTime;
	static	int		lastTime = 0;

	if ( !tr.registered ) {
		return;
	}
	GLimp_LogComment( "====== RE_RenderScene =====\n" );

	if ( r_norefresh->integer ) {
		return;
	}

	startTime = ri.Milliseconds()*ri.Cvar_VariableValue( "timescale" );

	if (!tr.world && !( fd->rdflags & RDF_NOWORLDMODEL ) ) {
		Com_Error (ERR_DROP, "R_RenderScene: NULL worldmodel");
	}

	// the client's sky portal: a map's own 3D skybox moved to its spot, else one drawn before each world scene
	if ( !( fd->rdflags & RDF_NOWORLDMODEL ) && !tr.inPortalSky ) {
		if ( fd->rdflags & RDF_SKYBOXPORTAL ) {
			tr.portalSkyDrawn = qtrue;
			if ( tr.portalSkyOn ) {
				refdef_t sky = *fd;

				VectorCopy( tr.portalSkyOrg, sky.vieworg );
				memset( sky.areamask, 0, sizeof( sky.areamask ) );
				tr.inPortalSky = qtrue;
				RE_RenderScene( &sky );
				tr.inPortalSky = qfalse;
				return;
			}
		} else {
			if ( tr.portalSkyOn && !tr.portalSkyDrawn )
				R_RenderPortalSky( fd );
			tr.portalSkyDrawn = qfalse;
		}
	}

	memcpy( tr.refdef.text, fd->text, sizeof( tr.refdef.text ) );

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

	if (fd->rdflags & RDF_SKYBOXPORTAL)
	{
		skyboxportal = 1;
	}
	else
	{
		// pasted this from SP
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

	if (tr.refdef.frametime > 500)
	{
		tr.refdef.frametime = 500;
	}
	else if (tr.refdef.frametime < 0)
	{
		tr.refdef.frametime = 0;
	}
	tr.refdef.rdflags = fd->rdflags;

	// the depth of field the client asked for, on the world's main view only
	if ( !( fd->rdflags & ( RDF_NOWORLDMODEL | RDF_SKYBOXPORTAL ) ) ) {
		tr.refdef.dofFocus = tr.dofNext[0];
		tr.refdef.dofRange = tr.dofNext[1];
		tr.refdef.dofBlur = tr.dofNext[2];
		tr.refdef.filterOn = tr.filterNextOn;
		if ( tr.filterNextOn )
			tr.refdef.filter = tr.filterNext;
	} else {
		tr.refdef.dofBlur = 0.0f;
		tr.refdef.filterOn = qfalse;
	}
	R_SkyLook( fd );

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

	tr.refdef.floatTime = tr.refdef.time * 0.001f;

	tr.refdef.numDrawSurfs = r_firstSceneDrawSurf;
	tr.refdef.drawSurfs = backEndData->drawSurfs;

	tr.refdef.num_entities = r_numentities - r_firstSceneEntity;
	tr.refdef.entities = &backEndData->entities[r_firstSceneEntity];
	tr.refdef.miniEntities = &backEndData->miniEntities[r_firstSceneMiniEntity];

	tr.refdef.num_dlights = r_numdlights - r_firstSceneDlight;
	tr.refdef.dlights = &backEndData->dlights[r_firstSceneDlight];

	// Add the decals here because decals add polys and we need to ensure
	// that the polys are added before the the renderer is prepared
	if ( !(tr.refdef.rdflags & RDF_NOWORLDMODEL) && !tr.inPortalSky )
	{
		R_AddDecals ( );
	}

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

	// the refdef's view: a 3D skybox's may be turned and moved by the sky look
	VectorCopy( tr.refdef.vieworg, parms.ori.origin );
	VectorCopy( tr.refdef.viewaxis[0], parms.ori.axis[0] );
	VectorCopy( tr.refdef.viewaxis[1], parms.ori.axis[1] );
	VectorCopy( tr.refdef.viewaxis[2], parms.ori.axis[2] );

	VectorCopy( tr.refdef.vieworg, parms.pvsOrigin );

	R_RenderView( &parms );

	// the next scene rendered in this frame will tack on after this one
	r_firstSceneDrawSurf = tr.refdef.numDrawSurfs;
	r_firstSceneEntity = r_numentities;
	r_firstSceneMiniEntity = r_numminientities;
	r_firstSceneDlight = r_numdlights;
	r_firstScenePoly = r_numpolys;

	refEntParent = -1;

	tr.frontEndMsec += ri.Milliseconds()*ri.Cvar_VariableValue( "timescale" ) - startTime;

	RE_RenderWorldEffects();

	if (tr.refdef.rdflags & RDF_AUTOMAP)
	{
		RE_RenderAutoMap();
	}
}

#if 0 //rwwFIXMEFIXME: Disable this before release!!!!!! I am just trying to find a crash bug.
int R_GetRNumEntities(void)
{
	return r_numentities;
}

void R_SetRNumEntities(int num)
{
	r_numentities = num;
}
#endif
