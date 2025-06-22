/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

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
R_InitNextFrame

====================
*/
void R_InitNextFrame( void ) {
	backEndData->commands.used = 0;

	tr.numTimedBlocks = 0;

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
void R_AddPolygonSurfaces( const trRefdef_t *refdef ) {
	srfPoly_t *poly;
	int	fogMask = -((refdef->rdflags & RDF_NOFOG) == 0);

	int i;
	for ( i = 0, poly = refdef->polys; i < refdef->numPolys ; i++, poly++ ) {
		shader_t *sh = R_GetShaderByHandle( poly->hShader );
		R_AddDrawSurf(
			(surfaceType_t *)poly,
			REFENTITYNUM_WORLD,
			sh,
			poly->fogIndex & fogMask,
			qfalse,
			qfalse,
			0 /* cubemapIndex */ );
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

	for ( j = 0; j < numPolys; j++ ) {
		if ( (r_numpolyverts + numVerts) > max_polyverts || r_numpolys >= max_polys ) {
			ri.Printf(
				PRINT_DEVELOPER,
				S_COLOR_YELLOW "WARNING: RE_AddPolyToScene: r_max_polys or r_max_polyverts reached\n");
			return;
		}

		poly = &backEndData->polys[r_numpolys];
		poly->surfaceType = SF_POLY;
		poly->hShader = hShader;
		poly->numVerts = numVerts;
		poly->verts = &backEndData->polyVerts[r_numpolyverts];

		Com_Memcpy( poly->verts, &verts[numVerts*j], numVerts * sizeof( *verts ) );

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
	vec3_t cross;

	if ( !tr.registered ) {
		return;
	}
	if ( r_numentities >= MAX_REFENTITIES ) {
		ri.Printf(PRINT_DEVELOPER, "RE_AddRefEntityToScene: Dropping refEntity, reached MAX_REFENTITIES\n");
		return;
	}
	if ( Q_isnan(ent->origin[0]) || Q_isnan(ent->origin[1]) || Q_isnan(ent->origin[2]) ) {
		static qboolean firstTime = qtrue;
		if (firstTime) {
			firstTime = qfalse;
			ri.Printf( PRINT_WARNING, "RE_AddRefEntityToScene passed a refEntity which has an origin with a NaN component\n");
		}
		return;
	}
	if ( (int)ent->reType < 0 || ent->reType >= RT_MAX_REF_ENTITY_TYPE ) {
		ri.Error( ERR_DROP, "RE_AddRefEntityToScene: bad reType %i", ent->reType );
	}

	backEndData->entities[r_numentities].e = *ent;
	backEndData->entities[r_numentities].lightingCalculated = qfalse;

	CrossProduct(ent->axis[0], ent->axis[1], cross);
	backEndData->entities[r_numentities].mirrored = (qboolean)(DotProduct(ent->axis[2], cross) < 0.f);

	r_numentities++;
}

/*
=====================
RE_AddMiniRefEntityToScene

1:1 with how vanilla does it --eez
=====================
*/
void RE_AddMiniRefEntityToScene( const miniRefEntity_t *miniRefEnt ) {
	refEntity_t entity;
	if(!tr.registered)
		return;
	if(!miniRefEnt)
		return;
	memset(&entity, 0, sizeof(entity));
	memcpy(&entity, miniRefEnt, sizeof(*miniRefEnt));
	RE_AddRefEntityToScene(&entity);
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

	if (r_hdr->integer)
	{
		float maxValue = MAX(r, MAX(g, b));
		if (maxValue > 1.0f)
		{
			VectorScale(dl->color, 1.0f / maxValue, dl->color);
			dl->radius *= maxValue;
		}
		dl->radius = MIN(dl->radius, 65535.0f);
	}

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

void RE_BeginScene(const refdef_t *fd)
{
	Com_Memcpy( tr.refdef.text, fd->text, sizeof( tr.refdef.text ) );

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
	tr.refdef.rdflags = fd->rdflags;
	tr.refdef.frameTime = fd->time - tr.refdef.lastTime;

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

	tr.refdef.sunDir[3] = 0.0f;
	tr.refdef.sunCol[3] = 1.0f;
	tr.refdef.sunAmbCol[3] = 1.0f;

	VectorCopy(tr.sunDirection, tr.refdef.sunDir);
	if ( (fd->rdflags & RDF_NOWORLDMODEL) || !(r_depthPrepass->value) ){
		tr.refdef.colorScale = 1.0f;
		VectorSet(tr.refdef.sunCol, 0, 0, 0);
		VectorSet(tr.refdef.sunAmbCol, 0, 0, 0);
	}
	else
	{
		tr.refdef.colorScale = r_forceSun->integer ? r_forceSunMapLightScale->value : tr.mapLightScale;

		if (r_sunlightMode->integer == 1)
		{
			tr.refdef.sunCol[0] =
			tr.refdef.sunCol[1] =
			tr.refdef.sunCol[2] = 1.0f;

			tr.refdef.sunAmbCol[0] =
			tr.refdef.sunAmbCol[1] =
			tr.refdef.sunAmbCol[2] = r_forceSun->integer ? r_forceSunAmbientScale->value : tr.sunShadowScale;
		}
		else
		{
			float scale = pow(2.0f, r_mapOverBrightBits->integer - tr.overbrightBits - 8);
			if (r_forceSun->integer)
			{
				VectorScale(tr.sunLight, scale * r_forceSunLightScale->value,   tr.refdef.sunCol);
				VectorScale(tr.sunLight, scale * r_forceSunAmbientScale->value, tr.refdef.sunAmbCol);
			}
			else
			{
				VectorScale(tr.sunLight, scale,                     tr.refdef.sunCol);
				VectorScale(tr.sunLight, scale * tr.sunShadowScale, tr.refdef.sunAmbCol);
			}
		}

		if (r_forceSun->integer == 2)
		{
			vec4_t lightDir, lightCol;
			int scale = 32768;
			float angle = (fd->time % scale) / (float)scale * M_PI;
			lightDir[0] = cos(angle);
			lightDir[1] = sin(35.0f * M_PI / 180.0f);
			lightDir[2] = sin(angle) * cos(35.0f * M_PI / 180.0f);
			lightDir[3] = 0.0f;

			lightCol[0] =
				lightCol[1] =
				lightCol[2] = CLAMP(sin(angle) * 2.0f, 0.0f, 1.0f) * 2.0f;
			lightCol[3] = 1.0f;

			VectorCopy4(lightDir, tr.refdef.sunDir);
			VectorCopy4(lightCol, tr.refdef.sunCol);
			VectorScale4(lightCol, 0.2f, tr.refdef.sunAmbCol);
		}
	}

	if (r_forceAutoExposure->integer)
	{
		tr.refdef.autoExposureMinMax[0] = r_forceAutoExposureMin->value;
		tr.refdef.autoExposureMinMax[1] = r_forceAutoExposureMax->value;
	}
	else
	{
		tr.refdef.autoExposureMinMax[0] = tr.autoExposureMinMax[0];
		tr.refdef.autoExposureMinMax[1] = tr.autoExposureMinMax[1];
	}

	if (r_forceToneMap->integer)
	{
		tr.refdef.toneMinAvgMaxLinear[0] = pow(2, r_forceToneMapMin->value);
		tr.refdef.toneMinAvgMaxLinear[1] = pow(2, r_forceToneMapAvg->value);
		tr.refdef.toneMinAvgMaxLinear[2] = pow(2, r_forceToneMapMax->value);
	}
	else
	{
		tr.refdef.toneMinAvgMaxLinear[0] = pow(2, tr.toneMinAvgMaxLevel[0]);
		tr.refdef.toneMinAvgMaxLinear[1] = pow(2, tr.toneMinAvgMaxLevel[1]);
		tr.refdef.toneMinAvgMaxLinear[2] = pow(2, tr.toneMinAvgMaxLevel[2]);
	}

	// Makro - copy exta info if present
	if (fd->rdflags & RDF_EXTRA) {
		const refdefex_t* extra = (const refdefex_t*) (fd+1);

		tr.refdef.blurFactor = extra->blurFactor;

		if (fd->rdflags & RDF_SUNLIGHT)
		{
			VectorCopy(extra->sunDir,    tr.refdef.sunDir);
			VectorCopy(extra->sunCol,    tr.refdef.sunCol);
			VectorCopy(extra->sunAmbCol, tr.refdef.sunAmbCol);
		}
	}
	else
	{
		tr.refdef.blurFactor = 0.0f;
	}

	// derived info

	tr.refdef.floatTime = tr.refdef.time * 0.001f;

	tr.refdef.numDrawSurfs = r_firstSceneDrawSurf;
	tr.refdef.drawSurfs = backEndData->drawSurfs;

	tr.refdef.num_entities = r_numentities - r_firstSceneEntity;
	tr.refdef.entities = &backEndData->entities[r_firstSceneEntity];

	tr.refdef.num_dlights = r_numdlights - r_firstSceneDlight;
	tr.refdef.dlights = &backEndData->dlights[r_firstSceneDlight];

	// Add the decals here because decals add polys and we need to ensure
	// that the polys are added before the the renderer is prepared
	if ( !(fd->rdflags & RDF_NOWORLDMODEL) )
		R_AddDecals();

	tr.refdef.numPolys = r_numpolys - r_firstScenePoly;
	tr.refdef.polys = &backEndData->polys[r_firstScenePoly];

	tr.refdef.num_pshadows = 0;
	tr.refdef.pshadows = &backEndData->pshadows[0];

	// turn off dynamic lighting globally by clearing all the
	// dlights if it needs to be disabled or if vertex lighting is enabled
	if ( r_dynamiclight->integer == 0 ||
		 r_vertexLight->integer == 1 ) {
		tr.refdef.num_dlights = 0;
	}

	if (fd->rdflags & RDF_SKYBOXPORTAL)
	{
		tr.world->skyboxportal = 1;

		// Don't update constants yet. Store everything and render everything next scene
		return;
	}
	else
	{
		// pasted this from SP
		// cdr - only change last time for the real render, not the portal
		tr.refdef.lastTime = fd->time;
	}

	// a single frame may have multiple scenes draw inside it --
	// a 3D game view, 3D status bar renderings, 3D menus, etc.
	// They need to be distinguished by the light flare code, because
	// the visibility state for a given surface may be different in
	// each scene / view.
	tr.frameSceneNum++;
	tr.sceneCount++;

	//ri.Printf(PRINT_ALL, "RE_BeginScene Frame: %i, skyportal: %i, entities: %i\n", backEndData->realFrameNumber, int(tr.world->skyboxportal && (tr.refdef.rdflags & RDF_SKYBOXPORTAL)), tr.refdef.num_entities);
	R_GatherFrameViews(&tr.refdef);
	RB_UpdateConstants(&tr.refdef);
}

void RE_EndScene()
{
	// the next scene rendered in this frame will tack on after this one
	r_firstSceneDrawSurf = tr.refdef.numDrawSurfs;
	r_firstSceneEntity = r_numentities;
	r_firstSceneDlight = r_numdlights;
	r_firstScenePoly = r_numpolys;
	tr.skyPortalEntities = 0;
	tr.numCachedViewParms = 0;
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
void RE_RenderScene( const refdef_t *fd )
{
	int				startTime;

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

	RE_BeginScene(fd);

	// Store skyportal info and don't render yet
	if (tr.refdef.rdflags & RDF_SKYBOXPORTAL)
	{
		viewParms_t		parms;
		Com_Memset(&parms, 0, sizeof(parms));
		parms.viewportX = fd->x;
		parms.viewportY = glConfig.vidHeight - (fd->y + fd->height);
		parms.viewportWidth = fd->width;
		parms.viewportHeight = fd->height;
		parms.isPortal = qfalse;
		parms.isSkyPortal = qfalse;
		parms.zNear = r_znear->value;

		parms.fovX = fd->fov_x;
		parms.fovY = fd->fov_y;

		parms.stereoFrame = tr.refdef.stereoFrame;

		VectorCopy(fd->vieworg, parms.ori.origin);
		VectorCopy(fd->viewaxis[0], parms.ori.axis[0]);
		VectorCopy(fd->viewaxis[1], parms.ori.axis[1]);
		VectorCopy(fd->viewaxis[2], parms.ori.axis[2]);

		VectorCopy(fd->vieworg, parms.pvsOrigin);

		parms.isSkyPortal = qtrue;
		Com_Memcpy(&tr.skyPortalParms, &parms, sizeof(viewParms_t));
		Com_Memcpy(tr.skyPortalAreaMask, tr.refdef.areamask, sizeof(tr.refdef.areamask));
		tr.skyPortalEntities = tr.refdef.num_entities;
		return;
	}

	// Render all the passes
	for (int i = 0; i < tr.numCachedViewParms; i++)
	{
		qhandle_t timer = R_BeginTimedBlockCmd(va("Render Pass %i", i));
		tr.refdef.numDrawSurfs = 0;
		R_RenderView(&tr.cachedViewParms[i]);
		R_IssuePendingRenderCommands();
		R_EndTimedBlockCmd(timer);
	}

	if(!( fd->rdflags & RDF_NOWORLDMODEL ))
	{
		qhandle_t timer = R_BeginTimedBlockCmd( "Post processing" );
		R_AddPostProcessCmd();
		R_EndTimedBlockCmd( timer );
	}

	R_IssuePendingRenderCommands();

	RE_EndScene();

	tr.frontEndMsec += ri.Milliseconds() - startTime;
}
