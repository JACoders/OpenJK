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
#ifdef USE_PMLIGHT
static int			r_firstSceneLitSurf;
#endif

int					r_numdlights;
static	int			r_firstSceneDlight;

static	int			r_numentities;
static	int			r_firstSceneEntity;

static	int			r_numpolys;
static	int			r_firstScenePoly;

static	int			r_numpolyverts;

int					skyboxportal;
int					drawskyboxportal;
//static int	r_frameCount;	// incremented every frame
/*
====================
R_InitNextFrame

====================
*/
void R_InitNextFrame( void ) {
	backEndData->commands.used = 0;

	r_firstSceneDrawSurf = 0;
#ifdef USE_PMLIGHT
	r_firstSceneLitSurf = 0;
#endif

	r_numdlights = 0;
	r_firstSceneDlight = 0;

	r_numentities = 0;
	r_firstSceneEntity = 0;

	r_numpolys = 0;
	r_firstScenePoly = 0;

	r_numpolyverts = 0;

	//r_frameCount++;
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
	int				i;
	shader_t		*sh;
	const srfPoly_t	*poly;

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
	srfPoly_t		*poly;
	int				i, j;
	int				fogIndex;
	const fog_t		*fog;
	vec3_t			bounds[2];

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

	// not adding these flags yet
	// if ( (int)ent->reType < 0 || ent->reType >= RT_MAX_SP_REF_ENTITY_TYPE || ent->reType == RT_MAX_MP_REF_ENTITY_TYPE ) {

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
		return;
	}

	refEntity_t		tempEnt;

	memcpy(&tempEnt, ent, sizeof(*ent));
	memset(((char *)&tempEnt)+sizeof(*ent), 0, sizeof(tempEnt) - sizeof(*ent));
	RE_AddRefEntityToScene(&tempEnt);
}

/*
=====================
RE_AddDynamicLightToScene

=====================
*/
static void RE_AddDynamicLightToScene( const vec3_t org, float intensity, float r, float g, float b, int additive ) {
	dlight_t	*dl;

	if ( !tr.registered ) {
		return;
	}
	//if ( r_numdlights >= MAX_DLIGHTS ) {
	if (r_numdlights >= ARRAY_LEN(backEndData->dlights)) {
		return;
	}
	if ( intensity <= 0 ) {
		return;
	}
#ifdef USE_PMLIGHT
	{
		r *= r_dlightIntensity->value;
		g *= r_dlightIntensity->value;
		b *= r_dlightIntensity->value;
		intensity *= r_dlightScale->value;
	}
#endif

	if (r_dlightSaturation->value != 1.0)
	{
		float luminance = LUMA(r, g, b);
		r = LERP(luminance, r, r_dlightSaturation->value);
		g = LERP(luminance, g, r_dlightSaturation->value);
		b = LERP(luminance, b, r_dlightSaturation->value);
	}

	dl = &backEndData->dlights[r_numdlights++];
	VectorCopy (org, dl->origin);
	dl->radius = intensity;
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;
	dl->additive = additive;
	dl->linear = qfalse;
}

/*
=====================
RE_AddLinearLightToScene
=====================
*/
void RE_AddLinearLightToScene( const vec3_t start, const vec3_t end, float intensity, float r, float g, float b ) {
	dlight_t	*dl;
	if (VectorCompare(start, end)) {
		RE_AddDynamicLightToScene(start, intensity, r, g, b, 0);
		return;
	}
	if (!tr.registered) {
		return;
	}
	if (r_numdlights >= ARRAY_LEN(backEndData->dlights)) {
		return;
	}
	if (intensity <= 0) {
		return;
	}
#ifdef USE_PMLIGHT
	{
		r *= r_dlightIntensity->value;
		g *= r_dlightIntensity->value;
		b *= r_dlightIntensity->value;
		intensity *= r_dlightScale->value;
	}
#endif

	if (r_dlightSaturation->value != 1.0)
	{
		float luminance = LUMA(r, g, b);
		r = LERP(luminance, r, r_mapGreyScale->value);
		g = LERP(luminance, g, r_mapGreyScale->value);
		b = LERP(luminance, b, r_mapGreyScale->value);
	}

	dl = &backEndData->dlights[r_numdlights++];
	VectorCopy(start, dl->origin);
	VectorCopy(end, dl->origin2);
	dl->radius = intensity;
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;
	dl->additive = 0;
	dl->linear = qtrue;
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
void RE_RenderWorldEffects( void );
void RE_RenderAutoMap( void );
void RE_RenderScene( const refdef_t *fd ) {
	renderCommand_t	lastRenderCommand;
	viewParms_t		parms;
	int				startTime;
	static	int		lastTime = 0;

	if (!tr.registered) {
		return;
	}

	if (r_norefresh->integer) {
		return;
	}

	startTime = ri.Milliseconds() * ri.Cvar_VariableValue("timescale");

	if (!tr.world && !(fd->rdflags & RDF_NOWORLDMODEL)) {
		Com_Error(ERR_DROP, "R_RenderScene: NULL worldmodel");
	}

	memcpy(tr.refdef.text, fd->text, sizeof(tr.refdef.text));

	tr.refdef.x = fd->x;
	tr.refdef.y = fd->y;
	tr.refdef.width = fd->width;
	tr.refdef.height = fd->height;
	tr.refdef.fov_x = fd->fov_x;
	tr.refdef.fov_y = fd->fov_y;

	VectorCopy(fd->vieworg, tr.refdef.vieworg);
	VectorCopy(fd->viewaxis[0], tr.refdef.viewaxis[0]);
	VectorCopy(fd->viewaxis[1], tr.refdef.viewaxis[1]);
	VectorCopy(fd->viewaxis[2], tr.refdef.viewaxis[2]);

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

	// copy the areamask data over and note if it has changed, which
	// will force a reset of the visible leafs even if the view hasn't moved
	tr.refdef.areamaskModified = qfalse;
	if (!(tr.refdef.rdflags & RDF_NOWORLDMODEL)) {
		int		areaDiff;
		int		i;

		// compare the area bits
		areaDiff = 0;
		for (i = 0; i < MAX_MAP_AREA_BYTES / sizeof(int); i++) {
			areaDiff |= ((int*)tr.refdef.areamask)[i] ^ ((int*)fd->areamask)[i];
			((int*)tr.refdef.areamask)[i] = ((int*)fd->areamask)[i];
		}

		if (areaDiff) {
			// a door just opened or something
			tr.refdef.areamaskModified = qtrue;
		}
	}


	// derived info

	tr.refdef.floatTime = tr.refdef.time * 0.001f;

	tr.refdef.numDrawSurfs = r_firstSceneDrawSurf;
	tr.refdef.drawSurfs = backEndData->drawSurfs;

#ifdef USE_PMLIGHT
	tr.refdef.numLitSurfs = r_firstSceneLitSurf;
	tr.refdef.litSurfs = backEndData->litSurfs;
#endif

	tr.refdef.num_entities = r_numentities - r_firstSceneEntity;
	tr.refdef.entities = &backEndData->entities[r_firstSceneEntity];

	tr.refdef.num_dlights = r_numdlights - r_firstSceneDlight;
	tr.refdef.dlights = &backEndData->dlights[r_firstSceneDlight];

	// Add the decals here because decals add polys and we need to ensure
	// that the polys are added before the the renderer is prepared
	if (!(tr.refdef.rdflags & RDF_NOWORLDMODEL))
	{
		R_AddDecals();
	}

	tr.refdef.numPolys = r_numpolys - r_firstScenePoly;
	tr.refdef.polys = &backEndData->polys[r_firstScenePoly];

	// turn off dynamic lighting globally by clearing all the
	// dlights if it needs to be disabled or if vertex lighting is enabled
	if (r_dynamiclight->integer == 0 || r_vertexLight->integer == 1) {
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
	memset(&parms, 0, sizeof(parms));
	parms.viewportX = tr.refdef.x;
	parms.viewportY = glConfig.vidHeight - (tr.refdef.y + tr.refdef.height);
	parms.viewportWidth = tr.refdef.width;
	parms.viewportHeight = tr.refdef.height;

	parms.scissorX = parms.viewportX;
	parms.scissorY = parms.viewportY;
	parms.scissorWidth = parms.viewportWidth;
	parms.scissorHeight = parms.viewportHeight;

	parms.portalView = PV_NONE;

#ifdef USE_PMLIGHT
	parms.dlights = tr.refdef.dlights;
	parms.num_dlights = tr.refdef.num_dlights;
#endif

	parms.zNear = r_znear->value;

	parms.fovX = tr.refdef.fov_x;
	parms.fovY = tr.refdef.fov_y;

	VectorCopy(fd->vieworg, parms.ori.origin);
	VectorCopy(fd->viewaxis[0], parms.ori.axis[0]);
	VectorCopy(fd->viewaxis[1], parms.ori.axis[1]);
	VectorCopy(fd->viewaxis[2], parms.ori.axis[2]);

	VectorCopy(fd->vieworg, parms.pvsOrigin);

	lastRenderCommand = (renderCommand_t)tr.lastRenderCommand;
	tr.drawSurfCmd = NULL;
	tr.numDrawSurfCmds = 0;

	R_RenderView(&parms);
	if (tr.needScreenMap)
	{
		if (lastRenderCommand == RC_DRAW_BUFFER) {
			// duplicate all views, including portals
			drawSurfsCommand_t *cmd, *src = NULL;
			int i;

			for (i = 0; i < tr.numDrawSurfCmds; i++)
			{
				cmd = (drawSurfsCommand_t*)R_GetCommandBuffer(sizeof(*cmd));
				src = tr.drawSurfCmd + i;
				*cmd = *src;
			}

			if (src)
			{
				// first drawsurface
				tr.drawSurfCmd[0].refdef.needScreenMap = qtrue;
				// last drawsurface
				src->refdef.switchRenderPass = qtrue;
			}
		}

		tr.needScreenMap = 0;
	}

	// the next scene rendered in this frame will tack on after this one
	r_firstSceneDrawSurf = tr.refdef.numDrawSurfs;
#ifdef USE_PMLIGHT
	r_firstSceneLitSurf = tr.refdef.numLitSurfs;
#endif
	r_firstSceneEntity = r_numentities;
	r_firstSceneDlight = r_numdlights;
	r_firstScenePoly = r_numpolys;

	tr.frontEndMsec += ri.Milliseconds()*ri.Cvar_VariableValue( "timescale" ) - startTime;

	RE_RenderWorldEffects();

#ifdef RDF_AUTOMAP
	if ( tr.refdef.rdflags & RDF_AUTOMAP )
	{
		RE_RenderAutoMap();
	}
#endif
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
