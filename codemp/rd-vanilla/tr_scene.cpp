//Anything above this #include will be ignored by the compiler
#include "qcommon/exe_headers.h"

#include "tr_local.h"

#if !defined(G2_H_INC)
	#include "ghoul2/G2.h"
#endif
#include "G2_local.h"
#include "qcommon/matcomp.h"

#ifdef VV_LIGHTING
#include "tr_lightmanager.h"
#endif

#pragma warning (disable: 4512)	//default assignment operator could not be gened
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
R_ToggleSmpFrame

====================
*/
void R_ToggleSmpFrame( void ) {
	backEndData->commands.used = 0;

	r_firstSceneDrawSurf = 0;

#ifdef VV_LIGHTING
	VVLightMan.num_dlights = 0;
#endif
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

	tr.currentEntityNum = TR_WORLDENT;
	tr.shiftedEntityNum = tr.currentEntityNum << QSORT_ENTITYNUM_SHIFT;

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
		Com_Printf (S_COLOR_YELLOW  "WARNING: RE_AddPolyToScene: NULL poly shader\n");
		return;
	}

	for ( j = 0; j < numPolys; j++ ) {
		if ( r_numpolyverts + numVerts > max_polyverts || r_numpolys >= max_polys ) {
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

	if ( r_numentities >= TR_WORLDENT )
	{
#ifndef FINAL_BUILD
		Com_Printf( "WARNING: RE_AddRefEntityToScene: too many entities\n");
#endif
		return;
	}
	if ( ent->reType < 0 || ent->reType >= RT_MAX_REF_ENTITY_TYPE ) {
		Com_Error( ERR_DROP, "RE_AddRefEntityToScene: bad reType %i", ent->reType );
	}

	backEndData->entities[r_numentities].e = *ent;
	backEndData->entities[r_numentities].lightingCalculated = qfalse;

	if (ent->ghoul2)
	{
		CGhoul2Info_v	&ghoul2 = *((CGhoul2Info_v *)ent->ghoul2);

		if (!ghoul2[0].mModel)
		{
#ifdef _DEBUG
			CGhoul2Info &g2 = ghoul2[0];
#endif
			//DebugBreak();
			Com_Printf("Your ghoul2 instance has no model!\n");
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
#ifndef VV_LIGHTING
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
#endif

/*
=====================
RE_AddLightToScene

=====================
*/
#ifndef VV_LIGHTING
void RE_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b ) {
	RE_AddDynamicLightToScene( org, intensity, r, g, b, qfalse );
}
#endif

/*
=====================
RE_AddAdditiveLightToScene

=====================
*/
#ifndef VV_LIGHTING
void RE_AddAdditiveLightToScene( const vec3_t org, float intensity, float r, float g, float b ) {
	RE_AddDynamicLightToScene( org, intensity, r, g, b, qtrue );
}
#endif


enum
{
	DECALPOLY_TYPE_NORMAL,
	DECALPOLY_TYPE_FADE,
	DECALPOLY_TYPE_MAX
};

#define		DECAL_FADE_TIME		1000

decalPoly_t*		RE_AllocDecal		( int type );

static decalPoly_t	re_decalPolys[DECALPOLY_TYPE_MAX][MAX_DECAL_POLYS];

static int			re_decalPolyHead[DECALPOLY_TYPE_MAX];
static int			re_decalPolyTotal[DECALPOLY_TYPE_MAX];

/*
===================
RE_ClearDecals

This is called to remove all decals from the world
===================
*/

void RE_ClearDecals ( void ) 
{
	memset( re_decalPolys, 0, sizeof(re_decalPolys) );
	memset( re_decalPolyHead, 0, sizeof(re_decalPolyHead) );
	memset( re_decalPolyTotal, 0, sizeof(re_decalPolyTotal) );
}

void R_InitDecals ( void )
{
	RE_ClearDecals ( );
}

void RE_FreeDecal ( int type, int index )
{
	if ( !re_decalPolys[type][index].time )
	{
		return;
	}
	
	if ( type == DECALPOLY_TYPE_NORMAL )
	{
		decalPoly_t* fade;

		fade = RE_AllocDecal ( DECALPOLY_TYPE_FADE );

		memcpy ( fade, &re_decalPolys[type][index], sizeof(decalPoly_t) );

		fade->time = tr.refdef.time;
		fade->fadetime = tr.refdef.time + DECAL_FADE_TIME;
	}

	re_decalPolys[type][index].time = 0;

	re_decalPolyTotal[type]--;
}

/*
===================
RE_AllocDecal

Will allways succeed, even if it requires freeing an old active mark
===================
*/
decalPoly_t* RE_AllocDecal( int type ) 
{
	decalPoly_t	*le;
	
	// See if the cvar changed
	if ( re_decalPolyTotal[type] > r_markcount->integer )
	{
		RE_ClearDecals ( );
	}

	le = &re_decalPolys[type][re_decalPolyHead[type]];

	// If it has no time its the first occasion its been used
	if ( le->time )
	{
		if ( le->time != tr.refdef.time ) 
		{
			int i = re_decalPolyHead[type];		

			// since we are killing one that existed before, make sure we 
			// kill all the other marks that belong to the group
			do
			{
				i++;
				if ( i >= r_markcount->integer )
				{
					i = 0;
				}

				// Break out on the first one thats not part of the group
				if ( re_decalPolys[type][i].time != le->time )
				{
					break;
				}

				RE_FreeDecal ( type, i );
			}
			while ( i != re_decalPolyHead[type] );			

			RE_FreeDecal ( type, re_decalPolyHead[type] );
		}
		else
		{
			RE_FreeDecal ( type, re_decalPolyHead[type] );
		}
	}

	memset ( le, 0, sizeof(decalPoly_t) );
	le->time = tr.refdef.time;

	re_decalPolyTotal[type]++;

	// Move on to the next decal poly and wrap around if need be
	re_decalPolyHead[type]++;
	if ( re_decalPolyHead[type] >= r_markcount->integer )
	{
		re_decalPolyHead[type] = 0;
	}

	return le;
}


/*
=================
RE_AddDecalToScene

origin should be a point within a unit of the plane
dir should be the plane normal

temporary marks will not be stored or randomly oriented, but immediately
passed to the renderer.
=================
*/
#define	MAX_DECAL_FRAGMENTS	128
#define	MAX_DECAL_POINTS		384

void RE_AddDecalToScene ( qhandle_t decalShader, const vec3_t origin, const vec3_t dir, float orientation, float red, float green, float blue, float alpha, qboolean alphaFade, float radius, qboolean temporary )
{
	vec3_t			axis[3];
	float			texCoordScale;
	vec3_t			originalPoints[4];
	byte			colors[4];
	int				i, j;
	int				numFragments;
	markFragment_t	markFragments[MAX_DECAL_FRAGMENTS], *mf;
	vec3_t			markPoints[MAX_DECAL_POINTS];
	vec3_t			projection;

	assert(decalShader);

	if ( r_markcount->integer <= 0 && !temporary )
	{
		return;
	}

	if ( radius <= 0 ) 
	{
		Com_Error( ERR_FATAL, "RE_AddDecalToScene:  called with <= 0 radius" );
	}

	// create the texture axis
	VectorNormalize2( dir, axis[0] );
	PerpendicularVector( axis[1], axis[0] );
	RotatePointAroundVector( axis[2], axis[0], axis[1], orientation );
	CrossProduct( axis[0], axis[2], axis[1] );

	texCoordScale = 0.5 * 1.0 / radius;

	// create the full polygon
	for ( i = 0 ; i < 3 ; i++ ) 
	{
		originalPoints[0][i] = origin[i] - radius * axis[1][i] - radius * axis[2][i];
		originalPoints[1][i] = origin[i] + radius * axis[1][i] - radius * axis[2][i];
		originalPoints[2][i] = origin[i] + radius * axis[1][i] + radius * axis[2][i];
		originalPoints[3][i] = origin[i] - radius * axis[1][i] + radius * axis[2][i];
	}

	// get the fragments
	VectorScale( dir, -20, projection );
	numFragments = R_MarkFragments( 4, (const vec3_t*)originalPoints,
					projection, MAX_DECAL_POINTS, markPoints[0],
					MAX_DECAL_FRAGMENTS, markFragments );

	colors[0] = red * 255;
	colors[1] = green * 255;
	colors[2] = blue * 255;
	colors[3] = alpha * 255;

	for ( i = 0, mf = markFragments ; i < numFragments ; i++, mf++ ) 
	{
		polyVert_t	*v;
		polyVert_t	verts[MAX_VERTS_ON_DECAL_POLY];
		decalPoly_t	*decal;

		// we have an upper limit on the complexity of polygons
		// that we store persistantly
		if ( mf->numPoints > MAX_VERTS_ON_DECAL_POLY ) 
		{
			mf->numPoints = MAX_VERTS_ON_DECAL_POLY;
		}

		for ( j = 0, v = verts ; j < mf->numPoints ; j++, v++ ) 
		{
			vec3_t		delta;

			VectorCopy( markPoints[mf->firstPoint + j], v->xyz );

			VectorSubtract( v->xyz, origin, delta );
			v->st[0] = 0.5 + DotProduct( delta, axis[1] ) * texCoordScale;
			v->st[1] = 0.5 + DotProduct( delta, axis[2] ) * texCoordScale;

			*(int *)v->modulate = *(int *)colors;
		}

		// if it is a temporary (shadow) mark, add it immediately and forget about it
		if ( temporary ) 
		{
			RE_AddPolyToScene( decalShader, mf->numPoints, verts, 1 );
			continue;
		}

		// otherwise save it persistantly
		decal = RE_AllocDecal( DECALPOLY_TYPE_NORMAL );
		decal->time = tr.refdef.time;
		decal->shader = decalShader;
		decal->poly.numVerts = mf->numPoints;
		decal->color[0] = red;
		decal->color[1] = green;
		decal->color[2] = blue;
		decal->color[3] = alpha;
		memcpy( decal->verts, verts, mf->numPoints * sizeof( verts[0] ) );
	}
}

/*
===============
R_AddDecals
===============
*/
static inline void R_AddDecals ( void ) 
{
	int			decalPoly;
	int			type;
	static int  lastMarkCount = -1;

	if ( r_markcount->integer != lastMarkCount )
	{
		if ( lastMarkCount != -1 )
		{
			RE_ClearDecals ( );
		}

		lastMarkCount = r_markcount->integer;
	}

	if ( r_markcount->integer <= 0 )
	{
		return;
	}

	for ( type = DECALPOLY_TYPE_NORMAL; type < DECALPOLY_TYPE_MAX; type ++ )
	{
		decalPoly = re_decalPolyHead[type];

		do
		{
			decalPoly_t* p = &re_decalPolys[type][decalPoly];

			if ( p->time )
			{				
				if ( p->fadetime )
				{
					int t;

					// fade all marks out with time
					t = tr.refdef.time - p->time;
					if ( t < DECAL_FADE_TIME ) 
					{
						float fade;
						int	  j;

						fade = 255.0f * (1.0f - ((float)t / DECAL_FADE_TIME));
						
						for ( j = 0 ; j < p->poly.numVerts ; j++ ) 
						{
							p->verts[j].modulate[3] = fade;
						}

						RE_AddPolyToScene( p->shader, p->poly.numVerts, p->verts, 1 );
					}
					else
					{
						RE_FreeDecal ( type, decalPoly );
					}
				}
				else
				{
					RE_AddPolyToScene( p->shader, p->poly.numVerts, p->verts, 1 );
				}
			}

			decalPoly++;
			if ( decalPoly >= r_markcount->integer )
			{
				decalPoly = 0;
			}
		}
		while ( decalPoly != re_decalPolyHead[type] );
	}
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
	lastTime = fd->time;

	if (fd->rdflags & RDF_SKYBOXPORTAL)
	{
		skyboxportal = 1;
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

#ifndef VV_LIGHTING
	tr.refdef.num_dlights = r_numdlights - r_firstSceneDlight;
	tr.refdef.dlights = &backEndData->dlights[r_firstSceneDlight];
#endif

	// Add the decals here because decals add polys and we need to ensure
	// that the polys are added before the the renderer is prepared
	if ( !(tr.refdef.rdflags & RDF_NOWORLDMODEL) ) 
	{
		R_AddDecals ( );
	}

	tr.refdef.numPolys = r_numpolys - r_firstScenePoly;
	tr.refdef.polys = &backEndData->polys[r_firstScenePoly];

	// turn off dynamic lighting globally by clearing all the
	// dlights if it needs to be disabled or if vertex lighting is enabled
#ifndef VV_LIGHTING
	if ( r_dynamiclight->integer == 0 ||
		 r_vertexLight->integer == 1 ) {
		tr.refdef.num_dlights = 0;
	}
#endif

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
