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

#include "../server/exe_headers.h"

#include "tr_local.h"

backEndData_t	*backEndData;
backEndState_t	backEnd;

bool tr_stencilled = false;
extern qboolean tr_distortionPrePost; //tr_shadows.cpp
extern qboolean tr_distortionNegate; //tr_shadows.cpp
extern void RB_CaptureScreenImage(void); //tr_shadows.cpp
extern void RB_DistortionFill(void); //tr_shadows.cpp
static void RB_DrawGlowOverlay();
static void RB_BlurGlowTexture();

// Whether we are currently rendering only glowing objects or not.
bool g_bRenderGlowingObjects = false;

// Whether the current hardware supports dynamic glows/flares.
bool g_bDynamicGlowSupported = false;

static const float s_flipMatrix[16] = {
	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	0, 0, -1, 0,
	-1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 0, 1
};


/*
** GL_Bind
*/
void GL_Bind( image_t *image ) {
	int texnum;

	if ( !image ) {
		ri.Printf( PRINT_WARNING, "GL_Bind: NULL image\n" );
		texnum = tr.defaultImage->texnum;
	} else {
		texnum = image->texnum;
	}

	if ( r_nobind->integer && tr.dlightImage ) {		// performance evaluation option
		texnum = tr.dlightImage->texnum;
	}

	if ( glState.currenttextures[glState.currenttmu] != texnum ) {
		image->frameUsed = tr.frameCount;
		glState.currenttextures[glState.currenttmu] = texnum;
		qglBindTexture (GL_TEXTURE_2D, texnum);
	}
}

/*
** GL_SelectTexture
*/
void GL_SelectTexture( int unit )
{
	if ( glState.currenttmu == unit )
	{
		return;
	}

	if ( unit == 0 )
	{
		qglActiveTextureARB( GL_TEXTURE0_ARB );
		GLimp_LogComment( "glActiveTextureARB( GL_TEXTURE0_ARB )\n" );
		qglClientActiveTextureARB( GL_TEXTURE0_ARB );
		GLimp_LogComment( "glClientActiveTextureARB( GL_TEXTURE0_ARB )\n" );
	}
	else if ( unit == 1 )
	{
		qglActiveTextureARB( GL_TEXTURE1_ARB );
		GLimp_LogComment( "glActiveTextureARB( GL_TEXTURE1_ARB )\n" );
		qglClientActiveTextureARB( GL_TEXTURE1_ARB );
		GLimp_LogComment( "glClientActiveTextureARB( GL_TEXTURE1_ARB )\n" );
	}
	else if ( unit == 2 )
	{
		qglActiveTextureARB( GL_TEXTURE2_ARB );
		GLimp_LogComment( "glActiveTextureARB( GL_TEXTURE2_ARB )\n" );
		qglClientActiveTextureARB( GL_TEXTURE2_ARB );
		GLimp_LogComment( "glClientActiveTextureARB( GL_TEXTURE2_ARB )\n" );
	}
	else if ( unit == 3 )
	{
		qglActiveTextureARB( GL_TEXTURE3_ARB );
		GLimp_LogComment( "glActiveTextureARB( GL_TEXTURE3_ARB )\n" );
		qglClientActiveTextureARB( GL_TEXTURE3_ARB );
		GLimp_LogComment( "glClientActiveTextureARB( GL_TEXTURE3_ARB )\n" );
	}
	else {
		Com_Error( ERR_DROP, "GL_SelectTexture: unit = %i", unit );
	}

	glState.currenttmu = unit;
}


/*
** GL_Cull
*/
void GL_Cull( int cullType ) {
	if ( glState.faceCulling == cullType ) {
		return;
	}
	glState.faceCulling = cullType;
	if (backEnd.projection2D){	//don't care, we're in 2d when it's always disabled
		return;
	}

	if ( cullType == CT_TWO_SIDED )
	{
		qglDisable( GL_CULL_FACE );
	}
	else
	{
		qglEnable( GL_CULL_FACE );

		if ( cullType == CT_BACK_SIDED )
		{
			if ( backEnd.viewParms.isMirror )
			{
				qglCullFace( GL_FRONT );
			}
			else
			{
				qglCullFace( GL_BACK );
			}
		}
		else
		{
			if ( backEnd.viewParms.isMirror )
			{
				qglCullFace( GL_BACK );
			}
			else
			{
				qglCullFace( GL_FRONT );
			}
		}
	}
}

/*
** GL_TexEnv
*/
void GL_TexEnv( int env )
{
	if ( env == glState.texEnv[glState.currenttmu] )
	{
		return;
	}

	glState.texEnv[glState.currenttmu] = env;


	switch ( env )
	{
	case GL_MODULATE:
		qglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		break;
	case GL_REPLACE:
		qglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
		break;
	case GL_DECAL:
		qglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );
		break;
	case GL_ADD:
		qglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD );
		break;
	default:
		Com_Error( ERR_DROP, "GL_TexEnv: invalid env '%d' passed\n", env );
		break;
	}
}

/*
** GL_State
**
** This routine is responsible for setting the most commonly changed state
** in Q3.
*/
void GL_State( uint32_t stateBits )
{
	uint32_t diff = stateBits ^ glState.glStateBits;

	if ( !diff )
	{
		return;
	}

	//
	// check depthFunc bits
	//
	if ( diff & GLS_DEPTHFUNC_EQUAL )
	{
		if ( stateBits & GLS_DEPTHFUNC_EQUAL )
		{
			qglDepthFunc( GL_EQUAL );
		}
		else
		{
			qglDepthFunc( GL_LEQUAL );
		}
	}

	//
	// check blend bits
	//
	if ( diff & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) )
	{
		GLenum srcFactor, dstFactor;

		if ( stateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) )
		{
			switch ( stateBits & GLS_SRCBLEND_BITS )
			{
			case GLS_SRCBLEND_ZERO:
				srcFactor = GL_ZERO;
				break;
			case GLS_SRCBLEND_ONE:
				srcFactor = GL_ONE;
				break;
			case GLS_SRCBLEND_DST_COLOR:
				srcFactor = GL_DST_COLOR;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
				srcFactor = GL_ONE_MINUS_DST_COLOR;
				break;
			case GLS_SRCBLEND_SRC_ALPHA:
				srcFactor = GL_SRC_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
				srcFactor = GL_ONE_MINUS_SRC_ALPHA;
				break;
			case GLS_SRCBLEND_DST_ALPHA:
				srcFactor = GL_DST_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
				srcFactor = GL_ONE_MINUS_DST_ALPHA;
				break;
			case GLS_SRCBLEND_ALPHA_SATURATE:
				srcFactor = GL_SRC_ALPHA_SATURATE;
				break;
			default:
				srcFactor = GL_ONE;		// to get warning to shut up
				Com_Error( ERR_DROP, "GL_State: invalid src blend state bits\n" );
				break;
			}

			switch ( stateBits & GLS_DSTBLEND_BITS )
			{
			case GLS_DSTBLEND_ZERO:
				dstFactor = GL_ZERO;
				break;
			case GLS_DSTBLEND_ONE:
				dstFactor = GL_ONE;
				break;
			case GLS_DSTBLEND_SRC_COLOR:
				dstFactor = GL_SRC_COLOR;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
				dstFactor = GL_ONE_MINUS_SRC_COLOR;
				break;
			case GLS_DSTBLEND_SRC_ALPHA:
				dstFactor = GL_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
				dstFactor = GL_ONE_MINUS_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_DST_ALPHA:
				dstFactor = GL_DST_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
				dstFactor = GL_ONE_MINUS_DST_ALPHA;
				break;
			default:
				dstFactor = GL_ONE;		// to get warning to shut up
				Com_Error( ERR_DROP, "GL_State: invalid dst blend state bits\n" );
				break;
			}

			qglEnable( GL_BLEND );
			qglBlendFunc( srcFactor, dstFactor );
		}
		else
		{
			qglDisable( GL_BLEND );
		}
	}

	//
	// check depthmask
	//
	if ( diff & GLS_DEPTHMASK_TRUE )
	{
		if ( stateBits & GLS_DEPTHMASK_TRUE )
		{
			qglDepthMask( GL_TRUE );
		}
		else
		{
			qglDepthMask( GL_FALSE );
		}
	}

	//
	// fill/line mode
	//
	if ( diff & GLS_POLYMODE_LINE )
	{
		if ( stateBits & GLS_POLYMODE_LINE )
		{
			qglPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		}
		else
		{
			qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		}
	}

	//
	// depthtest
	//
	if ( diff & GLS_DEPTHTEST_DISABLE )
	{
		if ( stateBits & GLS_DEPTHTEST_DISABLE )
		{
			qglDisable( GL_DEPTH_TEST );
		}
		else
		{
			qglEnable( GL_DEPTH_TEST );
		}
	}

	//
	// alpha test
	//
	if ( diff & GLS_ATEST_BITS )
	{
		switch ( stateBits & GLS_ATEST_BITS )
		{
		case 0:
			qglDisable( GL_ALPHA_TEST );
			break;
		case GLS_ATEST_GT_0:
			qglEnable( GL_ALPHA_TEST );
			qglAlphaFunc( GL_GREATER, 0.0f );
			break;
		case GLS_ATEST_LT_80:
			qglEnable( GL_ALPHA_TEST );
			qglAlphaFunc( GL_LESS, 0.5f );
			break;
		case GLS_ATEST_GE_80:
			qglEnable( GL_ALPHA_TEST );
			qglAlphaFunc( GL_GEQUAL, 0.5f );
			break;
		case GLS_ATEST_GE_C0:
			qglEnable( GL_ALPHA_TEST );
			qglAlphaFunc( GL_GEQUAL, 0.75f );
			break;
		default:
			assert( 0 );
			break;
		}
	}

	glState.glStateBits = stateBits;
}



/*
================
RB_Hyperspace

A player has predicted a teleport, but hasn't arrived yet
================
*/
static void RB_Hyperspace( void ) {
	float		c;

	if ( !backEnd.isHyperspace ) {
		// do initialization shit
	}

	c = ( backEnd.refdef.time & 255 ) / 255.0f;
	qglClearColor( c, c, c, 1 );
	qglClear( GL_COLOR_BUFFER_BIT );

	backEnd.isHyperspace = qtrue;
}


void SetViewportAndScissor( void ) {
	qglMatrixMode(GL_PROJECTION);
	qglLoadMatrixf( backEnd.viewParms.projectionMatrix );
	qglMatrixMode(GL_MODELVIEW);

	// set the window clipping
	qglViewport( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
		backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );
	qglScissor( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
		backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );
}

/*
=================
RB_BeginDrawingView

Any mirrored or portaled views have already been drawn, so prepare
to actually render the visible surfaces for this view
=================
*/
static void RB_BeginDrawingView (void) {
	int clearBits = GL_DEPTH_BUFFER_BIT;

	// sync with gl if needed
	if ( r_finish->integer == 1 && !glState.finishCalled ) {
		qglFinish ();
		glState.finishCalled = qtrue;
	}
	if ( r_finish->integer == 0 ) {
		glState.finishCalled = qtrue;
	}

	// we will need to change the projection matrix before drawing
	// 2D images again
	backEnd.projection2D = qfalse;

	//
	// set the modelview matrix for the viewer
	//
	SetViewportAndScissor();

	// ensures that depth writes are enabled for the depth clear
	GL_State( GLS_DEFAULT );

	// clear relevant buffers
	if ( r_measureOverdraw->integer || r_shadows->integer == 2 || tr_stencilled )
	{
		clearBits |= GL_STENCIL_BUFFER_BIT;
		tr_stencilled = false;
	}

	if (skyboxportal)
	{
		if ( backEnd.refdef.rdflags & RDF_SKYBOXPORTAL )
		{	// portal scene, clear whatever is necessary
			if (r_fastsky->integer || (backEnd.refdef.rdflags & RDF_NOWORLDMODEL) )
			{	// fastsky: clear color
				// try clearing first with the portal sky fog color, then the world fog color, then finally a default
				clearBits |= GL_COLOR_BUFFER_BIT;
				if (tr.world && tr.world->globalFog != -1)
				{
					const fog_t		*fog = &tr.world->fogs[tr.world->globalFog];
					qglClearColor(fog->parms.color[0],  fog->parms.color[1], fog->parms.color[2], 1.0f );
				}
				else
				{
					qglClearColor ( 0.3f, 0.3f, 0.3f, 1.0 );
				}
			}
		}
	}
	else
	{
		if ( r_fastsky->integer && !( backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) && !g_bRenderGlowingObjects )
		{
			if (tr.world && tr.world->globalFog != -1)
			{
				const fog_t		*fog = &tr.world->fogs[tr.world->globalFog];
				qglClearColor(fog->parms.color[0],  fog->parms.color[1], fog->parms.color[2], 1.0f );
			}
			else
			{
				qglClearColor( 0.3f, 0.3f, 0.3f, 1 );	// FIXME: get color of sky
			}
			clearBits |= GL_COLOR_BUFFER_BIT;	// FIXME: only if sky shaders have been used
		}
	}

	if ( !( backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) && ( r_DynamicGlow->integer && !g_bRenderGlowingObjects ) )
	{
		if (tr.world && tr.world->globalFog != -1)
		{ //this is because of a bug in multiple scenes I think, it needs to clear for the second scene but it doesn't normally.
			const fog_t		*fog = &tr.world->fogs[tr.world->globalFog];

			qglClearColor(fog->parms.color[0],  fog->parms.color[1], fog->parms.color[2], 1.0f );
			clearBits |= GL_COLOR_BUFFER_BIT;
		}
	}
	// If this pass is to just render the glowing objects, don't clear the depth buffer since
	// we're sharing it with the main scene (since the main scene has already been rendered). -AReis
	if ( g_bRenderGlowingObjects )
	{
		clearBits &= ~GL_DEPTH_BUFFER_BIT;
	}

	if (clearBits)
	{
		qglClear( clearBits );
	}

	if ( ( backEnd.refdef.rdflags & RDF_HYPERSPACE ) )
	{
		RB_Hyperspace();
		return;
	}
	else
	{
		backEnd.isHyperspace = qfalse;
	}

	glState.faceCulling = -1;		// force face culling to set next time

	// we will only draw a sun if there was sky rendered in this view
	backEnd.skyRenderedThisView = qfalse;

	// clip to the plane of the portal
	if ( backEnd.viewParms.isPortal ) {
		float	plane[4];
		double	plane2[4];

		plane[0] = backEnd.viewParms.portalPlane.normal[0];
		plane[1] = backEnd.viewParms.portalPlane.normal[1];
		plane[2] = backEnd.viewParms.portalPlane.normal[2];
		plane[3] = backEnd.viewParms.portalPlane.dist;

		plane2[0] = DotProduct (backEnd.viewParms.ori.axis[0], plane);
		plane2[1] = DotProduct (backEnd.viewParms.ori.axis[1], plane);
		plane2[2] = DotProduct (backEnd.viewParms.ori.axis[2], plane);
		plane2[3] = DotProduct (plane, backEnd.viewParms.ori.origin) - plane[3];

		qglLoadMatrixf( s_flipMatrix );
		qglClipPlane (GL_CLIP_PLANE0, plane2);
		qglEnable (GL_CLIP_PLANE0);
	} else {
		qglDisable (GL_CLIP_PLANE0);
	}
}

#define	MAC_EVENT_PUMP_MSEC		5

//used by RF_DISTORTION
static inline bool R_WorldCoordToScreenCoordFloat(vec3_t worldCoord, float *x, float *y)
{
	int	xcenter, ycenter;
	vec3_t	local, transformed;
	vec3_t	vfwd;
	vec3_t	vright;
	vec3_t	vup;
	float xzi;
	float yzi;

	xcenter = glConfig.vidWidth / 2;
	ycenter = glConfig.vidHeight / 2;

	//AngleVectors (tr.refdef.viewangles, vfwd, vright, vup);
	VectorCopy(tr.refdef.viewaxis[0], vfwd);
	VectorCopy(tr.refdef.viewaxis[1], vright);
	VectorCopy(tr.refdef.viewaxis[2], vup);

	VectorSubtract (worldCoord, tr.refdef.vieworg, local);

	transformed[0] = DotProduct(local,vright);
	transformed[1] = DotProduct(local,vup);
	transformed[2] = DotProduct(local,vfwd);

	// Make sure Z is not negative.
	if(transformed[2] < 0.01)
	{
		return false;
	}

	xzi = xcenter / transformed[2] * (90.0/tr.refdef.fov_x);
	yzi = ycenter / transformed[2] * (90.0/tr.refdef.fov_y);

	*x = xcenter + xzi * transformed[0];
	*y = ycenter - yzi * transformed[1];

	return true;
}

//used by RF_DISTORTION
static inline bool R_WorldCoordToScreenCoord( vec3_t worldCoord, int *x, int *y )
{
	float	xF, yF;
	bool retVal = R_WorldCoordToScreenCoordFloat( worldCoord, &xF, &yF );
	*x = (int)xF;
	*y = (int)yF;
	return retVal;
}

/*
==================
RB_RenderDrawSurfList
==================
*/
//number of possible surfs we can postrender.
//note that postrenders lack much of the optimization that the standard sort-render crap does,
//so it's slower.
#define MAX_POST_RENDERS	128

typedef struct
{
	int			fogNum;
	int			entNum;
	int			dlighted;
	int			depthRange;
	drawSurf_t	*drawSurf;
	shader_t	*shader;
} postRender_t;

static postRender_t g_postRenders[MAX_POST_RENDERS];
static int g_numPostRenders = 0;

void RB_RenderDrawSurfList( drawSurf_t *drawSurfs, int numDrawSurfs ) {
	shader_t		*shader, *oldShader;
	int				fogNum, oldFogNum;
	int				entityNum, oldEntityNum;
	int				dlighted, oldDlighted;
	int				depthRange, oldDepthRange;
	int				i;
	drawSurf_t		*drawSurf;
	unsigned int	oldSort;
	float			originalTime;
	trRefEntity_t	*curEnt;
	postRender_t	*pRender;
	bool			didShadowPass = false;

	if (g_bRenderGlowingObjects)
	{ //only shadow on initial passes
		didShadowPass = true;
	}

	// save original time for entity shader offsets
	originalTime = backEnd.refdef.floatTime;

	// clear the z buffer, set the modelview, etc
	RB_BeginDrawingView ();

	// draw everything
	oldEntityNum = -1;
	backEnd.currentEntity = &tr.worldEntity;
	oldShader = NULL;
	oldFogNum = -1;
	oldDepthRange = qfalse;
	oldDlighted = qfalse;
	oldSort = (unsigned int) -1;
	depthRange = qfalse;

	backEnd.pc.c_surfaces += numDrawSurfs;

	for (i = 0, drawSurf = drawSurfs ; i < numDrawSurfs ; i++, drawSurf++) {
		if ( drawSurf->sort == oldSort ) {
			// fast path, same as previous sort
			rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );
			continue;
		}
		R_DecomposeSort( drawSurf->sort, &entityNum, &shader, &fogNum, &dlighted );

		// If we're rendering glowing objects, but this shader has no stages with glow, skip it!
		if ( g_bRenderGlowingObjects && !shader->hasGlow )
		{
			shader = oldShader;
			entityNum = oldEntityNum;
			fogNum = oldFogNum;
			dlighted = oldDlighted;
			continue;
		}

		oldSort = drawSurf->sort;

		//
		// change the tess parameters if needed
		// a "entityMergable" shader is a shader that can have surfaces from seperate
		// entities merged into a single batch, like smoke and blood puff sprites
		if (entityNum != REFENTITYNUM_WORLD &&
			g_numPostRenders < MAX_POST_RENDERS)
		{
			if ( (backEnd.refdef.entities[entityNum].e.renderfx & RF_DISTORTION)/* ||
				(backEnd.refdef.entities[entityNum].e.renderfx & RF_FORCE_ENT_ALPHA)*/)
				//not sure if we need this alpha fix for sp or not, leaving it out for now -rww
			{ //must render last
				curEnt = &backEnd.refdef.entities[entityNum];
				pRender = &g_postRenders[g_numPostRenders];

				g_numPostRenders++;

				depthRange = 0;
				//figure this stuff out now and store it
				if ( curEnt->e.renderfx & RF_NODEPTH )
				{
					depthRange = 2;
				}
				else if ( curEnt->e.renderfx & RF_DEPTHHACK )
				{
					depthRange = 1;
				}
				pRender->depthRange = depthRange;

				//It is not necessary to update the old* values because
				//we are not updating now with the current values.
				depthRange = oldDepthRange;

				//store off the ent num
				pRender->entNum = entityNum;

				//remember the other values necessary for rendering this surf
				pRender->drawSurf = drawSurf;
				pRender->dlighted = dlighted;
				pRender->fogNum = fogNum;
				pRender->shader = shader;

				//assure the info is back to the last set state
				shader = oldShader;
				entityNum = oldEntityNum;
				fogNum = oldFogNum;
				dlighted = oldDlighted;

				oldSort = (unsigned int)-1; //invalidate this thing, cause we may want to postrender more surfs of the same sort

				//continue without bothering to begin a draw surf
				continue;
			}
		}

		if (shader != oldShader || fogNum != oldFogNum || dlighted != oldDlighted
			|| ( entityNum != oldEntityNum && !shader->entityMergable ) )
		{
			if (oldShader != NULL) {
				RB_EndSurface();

				if (!didShadowPass && shader && shader->sort > SS_BANNER)
				{
					RB_ShadowFinish();
					didShadowPass = true;
				}
			}
			RB_BeginSurface( shader, fogNum );
			oldShader = shader;
			oldFogNum = fogNum;
			oldDlighted = dlighted;
		}

		//
		// change the modelview matrix if needed
		//
		if ( entityNum != oldEntityNum ) {
			depthRange = qfalse;

			if ( entityNum != REFENTITYNUM_WORLD ) {
				backEnd.currentEntity = &backEnd.refdef.entities[entityNum];
				backEnd.refdef.floatTime = originalTime - backEnd.currentEntity->e.shaderTime;

				// set up the transformation matrix
				R_RotateForEntity( backEnd.currentEntity, &backEnd.viewParms, &backEnd.ori );

				// set up the dynamic lighting if needed
				if ( backEnd.currentEntity->needDlights ) {
					R_TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.ori );
				}

				if ( backEnd.currentEntity->e.renderfx & RF_NODEPTH ) {
					// No depth at all, very rare but some things for seeing through walls
					depthRange = 2;
				}
				else if ( backEnd.currentEntity->e.renderfx & RF_DEPTHHACK ) {
					// hack the depth range to prevent view model from poking into walls
					depthRange = qtrue;
				}
			} else {
				backEnd.currentEntity = &tr.worldEntity;
				backEnd.refdef.floatTime = originalTime;
				backEnd.ori = backEnd.viewParms.world;
				R_TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.ori );
			}

			qglLoadMatrixf( backEnd.ori.modelMatrix );

			//
			// change depthrange if needed
			//
			if ( oldDepthRange != depthRange ) {
				switch ( depthRange ) {
					default:
					case 0:
						qglDepthRange (0, 1);
						break;

					case 1:
						qglDepthRange (0, .3);
						break;

					case 2:
						qglDepthRange (0, 0);
						break;
				}

				oldDepthRange = depthRange;
			}

			oldEntityNum = entityNum;
		}

		// add the triangles for this surface
		rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );
	}

	// draw the contents of the last shader batch
	if (oldShader != NULL) {
		RB_EndSurface();
	}

	if (tr_stencilled && tr_distortionPrePost)
	{ //ok, cap it now
		RB_CaptureScreenImage();
		RB_DistortionFill();
	}

	//render distortion surfs (or anything else that needs to be post-rendered)
	if (g_numPostRenders > 0)
	{
		int lastPostEnt = -1;

		while (g_numPostRenders > 0)
		{
			g_numPostRenders--;
			pRender = &g_postRenders[g_numPostRenders];

			RB_BeginSurface( pRender->shader, pRender->fogNum );

			backEnd.currentEntity = &backEnd.refdef.entities[pRender->entNum];

			backEnd.refdef.floatTime = originalTime - backEnd.currentEntity->e.shaderTime;

			// set up the transformation matrix
			R_RotateForEntity( backEnd.currentEntity, &backEnd.viewParms, &backEnd.ori );

			// set up the dynamic lighting if needed
			if ( backEnd.currentEntity->needDlights )
			{
				R_TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.ori );
			}

			qglLoadMatrixf( backEnd.ori.modelMatrix );

			depthRange = pRender->depthRange;
			switch ( depthRange )
			{
				default:
				case 0:
					qglDepthRange (0, 1);
					break;

				case 1:
					qglDepthRange (0, .3);
					break;

				case 2:
					qglDepthRange (0, 0);
					break;
			}

			if ((backEnd.currentEntity->e.renderfx & RF_DISTORTION) &&
				lastPostEnt != pRender->entNum)
			{ //do the capture now, we only need to do it once per ent
				int x, y;
				int rad = backEnd.currentEntity->e.radius;
				//We are going to just bind this, and then the CopyTexImage is going to
				//stomp over this texture num in texture memory.
				GL_Bind( tr.screenImage );

				if (R_WorldCoordToScreenCoord( backEnd.currentEntity->e.origin, &x, &y ))
				{
					int cX, cY;
					cX = glConfig.vidWidth-x-(rad/2);
					cY = glConfig.vidHeight-y-(rad/2);

					if (cX+rad > glConfig.vidWidth)
					{ //would it go off screen?
						cX = glConfig.vidWidth-rad;
					}
					else if (cX < 0)
					{ //cap it off at 0
						cX = 0;
					}

					if (cY+rad > glConfig.vidHeight)
					{ //would it go off screen?
						cY = glConfig.vidHeight-rad;
					}
					else if (cY < 0)
					{ //cap it off at 0
						cY = 0;
					}

					//now copy a portion of the screen to this texture
					qglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, cX, cY, rad, rad, 0);

					lastPostEnt = pRender->entNum;
				}
			}

			rb_surfaceTable[ *pRender->drawSurf->surface ]( pRender->drawSurf->surface );
			RB_EndSurface();
		}
	}

	// go back to the world modelview matrix
	qglLoadMatrixf( backEnd.viewParms.world.modelMatrix );
	if ( depthRange ) {
		qglDepthRange (0, 1);
	}

#if 0
	RB_DrawSun();
#endif
	if (tr_stencilled && !tr_distortionPrePost)
	{ //draw in the stencil buffer's cutout
		RB_DistortionFill();
	}
	if (!didShadowPass)
	{
		// darken down any stencil shadows
		RB_ShadowFinish();
		didShadowPass = true;
	}

// add light flares on lights that aren't obscured
//	RB_RenderFlares();
}


/*
============================================================================

RENDER BACK END FUNCTIONS

============================================================================
*/

/*
================
RB_SetGL2D

================
*/
void	RB_SetGL2D (void) {
	backEnd.projection2D = qtrue;

	// set 2D virtual screen size
	qglViewport( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	qglScissor( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	qglMatrixMode(GL_PROJECTION);
    qglLoadIdentity ();
	qglOrtho (0, 640, 480, 0, 0, 1);
	qglMatrixMode(GL_MODELVIEW);
    qglLoadIdentity ();

	GL_State( GLS_DEPTHTEST_DISABLE |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

	qglDisable( GL_CULL_FACE );
	qglDisable( GL_CLIP_PLANE0 );

	// set time for 2D shaders
	backEnd.refdef.time = ri.Milliseconds();
	backEnd.refdef.floatTime = backEnd.refdef.time * 0.001f;
}


/*
=============
RB_SetColor

=============
*/
const void	*RB_SetColor( const void *data ) {
	const setColorCommand_t	*cmd;

	cmd = (const setColorCommand_t *)data;

	backEnd.color2D[0] = cmd->color[0] * 255;
	backEnd.color2D[1] = cmd->color[1] * 255;
	backEnd.color2D[2] = cmd->color[2] * 255;
	backEnd.color2D[3] = cmd->color[3] * 255;

	return (const void *)(cmd + 1);
}

/*
=============
RB_StretchPic
=============
*/
const void *RB_StretchPic ( const void *data ) {
	const stretchPicCommand_t	*cmd;
	shader_t *shader;
	int		numVerts, numIndexes;

	cmd = (const stretchPicCommand_t *)data;

	if ( !backEnd.projection2D ) {
		RB_SetGL2D();
	}

	shader = cmd->shader;
	if ( shader != tess.shader ) {
		if ( tess.numIndexes ) {
			RB_EndSurface();
		}
		backEnd.currentEntity = &backEnd.entity2D;
		RB_BeginSurface( shader, 0 );
	}

	RB_CHECKOVERFLOW( 4, 6 );
	numVerts = tess.numVertexes;
	numIndexes = tess.numIndexes;

	tess.numVertexes += 4;
	tess.numIndexes += 6;

	tess.indexes[ numIndexes ] = numVerts + 3;
	tess.indexes[ numIndexes + 1 ] = numVerts + 0;
	tess.indexes[ numIndexes + 2 ] = numVerts + 2;
	tess.indexes[ numIndexes + 3 ] = numVerts + 2;
	tess.indexes[ numIndexes + 4 ] = numVerts + 0;
	tess.indexes[ numIndexes + 5 ] = numVerts + 1;

	byteAlias_t *baDest = NULL, *baSource = (byteAlias_t *)&backEnd.color2D;
	baDest = (byteAlias_t *)&tess.vertexColors[numVerts + 0]; baDest->ui = baSource->ui;
	baDest = (byteAlias_t *)&tess.vertexColors[numVerts + 1]; baDest->ui = baSource->ui;
	baDest = (byteAlias_t *)&tess.vertexColors[numVerts + 2]; baDest->ui = baSource->ui;
	baDest = (byteAlias_t *)&tess.vertexColors[numVerts + 3]; baDest->ui = baSource->ui;

	tess.xyz[ numVerts ][0] = cmd->x;
	tess.xyz[ numVerts ][1] = cmd->y;
	tess.xyz[ numVerts ][2] = 0;

	tess.texCoords[ numVerts ][0][0] = cmd->s1;
	tess.texCoords[ numVerts ][0][1] = cmd->t1;

	tess.xyz[ numVerts + 1 ][0] = cmd->x + cmd->w;
	tess.xyz[ numVerts + 1 ][1] = cmd->y;
	tess.xyz[ numVerts + 1 ][2] = 0;

	tess.texCoords[ numVerts + 1 ][0][0] = cmd->s2;
	tess.texCoords[ numVerts + 1 ][0][1] = cmd->t1;

	tess.xyz[ numVerts + 2 ][0] = cmd->x + cmd->w;
	tess.xyz[ numVerts + 2 ][1] = cmd->y + cmd->h;
	tess.xyz[ numVerts + 2 ][2] = 0;

	tess.texCoords[ numVerts + 2 ][0][0] = cmd->s2;
	tess.texCoords[ numVerts + 2 ][0][1] = cmd->t2;

	tess.xyz[ numVerts + 3 ][0] = cmd->x;
	tess.xyz[ numVerts + 3 ][1] = cmd->y + cmd->h;
	tess.xyz[ numVerts + 3 ][2] = 0;

	tess.texCoords[ numVerts + 3 ][0][0] = cmd->s1;
	tess.texCoords[ numVerts + 3 ][0][1] = cmd->t2;

	return (const void *)(cmd + 1);
}


/*
=============
RB_RotatePic
=============
*/
const void *RB_RotatePic ( const void *data )
{
	const rotatePicCommand_t	*cmd;
	shader_t *shader;

	cmd = (const rotatePicCommand_t *)data;

	shader = cmd->shader;

	if ( !backEnd.projection2D ) {
		RB_SetGL2D();
	}

	shader = cmd->shader;
	if ( shader != tess.shader ) {
		if ( tess.numIndexes ) {
			RB_EndSurface();
		}
		backEnd.currentEntity = &backEnd.entity2D;
		RB_BeginSurface( shader, 0 );
	}

	RB_CHECKOVERFLOW( 4, 6 );
	int numVerts = tess.numVertexes;
	int numIndexes = tess.numIndexes;

	float angle = DEG2RAD( cmd-> a );
	float s = sinf( angle );
	float c = cosf( angle );

	matrix3_t m = {
		{ c, s, 0.0f },
		{ -s, c, 0.0f },
		{ cmd->x + cmd->w, cmd->y, 1.0f }
	};

	tess.numVertexes += 4;
	tess.numIndexes += 6;

	tess.indexes[ numIndexes ] = numVerts + 3;
	tess.indexes[ numIndexes + 1 ] = numVerts + 0;
	tess.indexes[ numIndexes + 2 ] = numVerts + 2;
	tess.indexes[ numIndexes + 3 ] = numVerts + 2;
	tess.indexes[ numIndexes + 4 ] = numVerts + 0;
	tess.indexes[ numIndexes + 5 ] = numVerts + 1;

	byteAlias_t *baDest = NULL, *baSource = (byteAlias_t *)&backEnd.color2D;
	baDest = (byteAlias_t *)&tess.vertexColors[numVerts + 0]; baDest->ui = baSource->ui;
	baDest = (byteAlias_t *)&tess.vertexColors[numVerts + 1]; baDest->ui = baSource->ui;
	baDest = (byteAlias_t *)&tess.vertexColors[numVerts + 2]; baDest->ui = baSource->ui;
	baDest = (byteAlias_t *)&tess.vertexColors[numVerts + 3]; baDest->ui = baSource->ui;

	tess.xyz[ numVerts ][0] = m[0][0] * (-cmd->w) + m[2][0];
	tess.xyz[ numVerts ][1] = m[0][1] * (-cmd->w) + m[2][1];
	tess.xyz[ numVerts ][2] = 0;

	tess.texCoords[ numVerts ][0][0] = cmd->s1;
	tess.texCoords[ numVerts ][0][1] = cmd->t1;

	tess.xyz[ numVerts + 1 ][0] = m[2][0];
	tess.xyz[ numVerts + 1 ][1] = m[2][1];
	tess.xyz[ numVerts + 1 ][2] = 0;

	tess.texCoords[ numVerts + 1 ][0][0] = cmd->s2;
	tess.texCoords[ numVerts + 1 ][0][1] = cmd->t1;

	tess.xyz[ numVerts + 2 ][0] = m[1][0] * (cmd->h) + m[2][0];
	tess.xyz[ numVerts + 2 ][1] = m[1][1] * (cmd->h) + m[2][1];
	tess.xyz[ numVerts + 2 ][2] = 0;

	tess.texCoords[ numVerts + 2 ][0][0] = cmd->s2;
	tess.texCoords[ numVerts + 2 ][0][1] = cmd->t2;

	tess.xyz[ numVerts + 3 ][0] = m[0][0] * (-cmd->w) + m[1][0] * (cmd->h) + m[2][0];
	tess.xyz[ numVerts + 3 ][1] = m[0][1] * (-cmd->w) + m[1][1] * (cmd->h) + m[2][1];
	tess.xyz[ numVerts + 3 ][2] = 0;

	tess.texCoords[ numVerts + 3 ][0][0] = cmd->s1;
	tess.texCoords[ numVerts + 3 ][0][1] = cmd->t2;

	return (const void *)(cmd + 1);
}

/*
=============
RB_RotatePic2
=============
*/
const void *RB_RotatePic2 ( const void *data )
{
	const rotatePicCommand_t	*cmd;
	shader_t *shader;

	cmd = (const rotatePicCommand_t *)data;

	shader = cmd->shader;

	// FIXME is this needed
	if ( shader->numUnfoggedPasses )
	{
		if ( !backEnd.projection2D ) {
			RB_SetGL2D();
		}

		shader = cmd->shader;
		if ( shader != tess.shader ) {
			if ( tess.numIndexes ) {
				RB_EndSurface();
			}
			backEnd.currentEntity = &backEnd.entity2D;
			RB_BeginSurface( shader, 0 );
		}

		RB_CHECKOVERFLOW( 4, 6 );
		int numVerts = tess.numVertexes;
		int numIndexes = tess.numIndexes;

		float angle = DEG2RAD( cmd-> a );
		float s = sinf( angle );
		float c = cosf( angle );

		matrix3_t m = {
			{ c, s, 0.0f },
			{ -s, c, 0.0f },
			{ cmd->x, cmd->y, 1.0f }
		};

		tess.numVertexes += 4;
		tess.numIndexes += 6;

		tess.indexes[ numIndexes ] = numVerts + 3;
		tess.indexes[ numIndexes + 1 ] = numVerts + 0;
		tess.indexes[ numIndexes + 2 ] = numVerts + 2;
		tess.indexes[ numIndexes + 3 ] = numVerts + 2;
		tess.indexes[ numIndexes + 4 ] = numVerts + 0;
		tess.indexes[ numIndexes + 5 ] = numVerts + 1;

		byteAlias_t *baDest = NULL, *baSource = (byteAlias_t *)&backEnd.color2D;
		baDest = (byteAlias_t *)&tess.vertexColors[numVerts + 0]; baDest->ui = baSource->ui;
		baDest = (byteAlias_t *)&tess.vertexColors[numVerts + 1]; baDest->ui = baSource->ui;
		baDest = (byteAlias_t *)&tess.vertexColors[numVerts + 2]; baDest->ui = baSource->ui;
		baDest = (byteAlias_t *)&tess.vertexColors[numVerts + 3]; baDest->ui = baSource->ui;

		tess.xyz[ numVerts ][0] = m[0][0] * (-cmd->w * 0.5f) + m[1][0] * (-cmd->h * 0.5f) + m[2][0];
		tess.xyz[ numVerts ][1] = m[0][1] * (-cmd->w * 0.5f) + m[1][1] * (-cmd->h * 0.5f) + m[2][1];
		tess.xyz[ numVerts ][2] = 0;

		tess.texCoords[ numVerts ][0][0] = cmd->s1;
		tess.texCoords[ numVerts ][0][1] = cmd->t1;

		tess.xyz[ numVerts + 1 ][0] = m[0][0] * (cmd->w * 0.5f) + m[1][0] * (-cmd->h * 0.5f) + m[2][0];
		tess.xyz[ numVerts + 1 ][1] = m[0][1] * (cmd->w * 0.5f) + m[1][1] * (-cmd->h * 0.5f) + m[2][1];
		tess.xyz[ numVerts + 1 ][2] = 0;

		tess.texCoords[ numVerts + 1 ][0][0] = cmd->s2;
		tess.texCoords[ numVerts + 1 ][0][1] = cmd->t1;

		tess.xyz[ numVerts + 2 ][0] = m[0][0] * (cmd->w * 0.5f) + m[1][0] * (cmd->h * 0.5f) + m[2][0];
		tess.xyz[ numVerts + 2 ][1] = m[0][1] * (cmd->w * 0.5f) + m[1][1] * (cmd->h * 0.5f) + m[2][1];
		tess.xyz[ numVerts + 2 ][2] = 0;

		tess.texCoords[ numVerts + 2 ][0][0] = cmd->s2;
		tess.texCoords[ numVerts + 2 ][0][1] = cmd->t2;

		tess.xyz[ numVerts + 3 ][0] = m[0][0] * (-cmd->w * 0.5f) + m[1][0] * (cmd->h * 0.5f) + m[2][0];
		tess.xyz[ numVerts + 3 ][1] = m[0][1] * (-cmd->w * 0.5f) + m[1][1] * (cmd->h * 0.5f) + m[2][1];
		tess.xyz[ numVerts + 3 ][2] = 0;

		tess.texCoords[ numVerts + 3 ][0][0] = cmd->s1;
		tess.texCoords[ numVerts + 3 ][0][1] = cmd->t2;
	}

	return (const void *)(cmd + 1);
}

/*
=============
RB_ScissorPic
=============
*/
const void *RB_Scissor ( const void *data )
{
	const scissorCommand_t	*cmd;

	cmd = (const scissorCommand_t *)data;

	if ( !backEnd.projection2D )
	{
		RB_SetGL2D();
	}

	if (cmd->x >= 0)
	{
		qglScissor( cmd->x,(glConfig.vidHeight - cmd->y - cmd->h),cmd->w,cmd->h);
	}
	else
	{
		qglScissor( 0, 0, glConfig.vidWidth, glConfig.vidHeight);
	}

	return (const void *)(cmd + 1);
}

/*
=============
RB_DrawSurfs

=============
*/
const void	*RB_DrawSurfs( const void *data ) {
	const drawSurfsCommand_t	*cmd;

	// finish any 2D drawing if needed
	if ( tess.numIndexes ) {
		RB_EndSurface();
	}

	cmd = (const drawSurfsCommand_t *)data;

	backEnd.refdef = cmd->refdef;
	backEnd.viewParms = cmd->viewParms;

	RB_RenderDrawSurfList( cmd->drawSurfs, cmd->numDrawSurfs );

	// Dynamic Glow/Flares:
	/*
		The basic idea is to render the glowing parts of the scene to an offscreen buffer, then take
		that buffer and blur it. After it is sufficiently blurred, re-apply that image back to
		the normal screen using a additive blending. To blur the scene I use a vertex program to supply
		four texture coordinate offsets that allow 'peeking' into adjacent pixels. In the register
		combiner (pixel shader), I combine the adjacent pixels using a weighting factor. - Aurelio
	*/

	// Render dynamic glowing/flaring objects.
	if ( !(backEnd.refdef.rdflags & RDF_NOWORLDMODEL) && g_bDynamicGlowSupported && r_DynamicGlow->integer )
	{
		// Copy the normal scene to texture.
		qglDisable( GL_TEXTURE_2D );
		qglEnable( GL_TEXTURE_RECTANGLE_ARB );
		qglBindTexture( GL_TEXTURE_RECTANGLE_ARB, tr.sceneImage );
		qglCopyTexSubImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, backEnd.viewParms.viewportX, backEnd.viewParms.viewportY, backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);
		qglDisable( GL_TEXTURE_RECTANGLE_ARB );
		qglEnable( GL_TEXTURE_2D );

		// Just clear colors, but leave the depth buffer intact so we can 'share' it.
		qglClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
		qglClear( GL_COLOR_BUFFER_BIT );

		// Render the glowing objects.
		g_bRenderGlowingObjects = true;
		RB_RenderDrawSurfList( cmd->drawSurfs, cmd->numDrawSurfs );
		g_bRenderGlowingObjects = false;
		qglFinish();

		// Copy the glow scene to texture.
		qglDisable( GL_TEXTURE_2D );
		qglEnable( GL_TEXTURE_RECTANGLE_ARB );
		qglBindTexture( GL_TEXTURE_RECTANGLE_ARB, tr.screenGlow );
		qglCopyTexSubImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0,  backEnd.viewParms.viewportX, backEnd.viewParms.viewportY, backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );
		qglDisable( GL_TEXTURE_RECTANGLE_ARB );
		qglEnable( GL_TEXTURE_2D );

		// Resize the viewport to the blur texture size.
		const int oldViewWidth = backEnd.viewParms.viewportWidth;
		const int oldViewHeight = backEnd.viewParms.viewportHeight;
		backEnd.viewParms.viewportWidth = r_DynamicGlowWidth->integer;
		backEnd.viewParms.viewportHeight = r_DynamicGlowHeight->integer;
		SetViewportAndScissor();

		// Blur the scene.
		RB_BlurGlowTexture();

		// Copy the finished glow scene back to texture.
		qglDisable( GL_TEXTURE_2D );
		qglEnable( GL_TEXTURE_RECTANGLE_ARB );
		qglBindTexture( GL_TEXTURE_RECTANGLE_ARB, tr.blurImage );
		qglCopyTexSubImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, 0, 0, backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );
		qglDisable( GL_TEXTURE_RECTANGLE_ARB );
		qglEnable( GL_TEXTURE_2D );

		// Set the viewport back to normal.
		backEnd.viewParms.viewportWidth = oldViewWidth;
		backEnd.viewParms.viewportHeight = oldViewHeight;
		SetViewportAndScissor();
		qglClear( GL_COLOR_BUFFER_BIT );

		// Draw the glow additively over the screen.
		RB_DrawGlowOverlay();
	}

	return (const void *)(cmd + 1);
}


/*
=============
RB_DrawBuffer

=============
*/
const void	*RB_DrawBuffer( const void *data ) {
	const drawBufferCommand_t	*cmd;

	cmd = (const drawBufferCommand_t *)data;

	qglDrawBuffer( cmd->buffer );

		// clear screen for debugging
	if (!( backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) && tr.world && tr.refdef.rdflags & RDF_doLAGoggles)
	{
		const fog_t		*fog = &tr.world->fogs[tr.world->numfogs];

		qglClearColor(fog->parms.color[0],  fog->parms.color[1], fog->parms.color[2], 1.0f );
		qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
	else if (!( backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) && tr.world && tr.world->globalFog != -1 && tr.sceneCount)//don't clear during menus, wait for real scene
	{
		const fog_t		*fog = &tr.world->fogs[tr.world->globalFog];

		qglClearColor(fog->parms.color[0],  fog->parms.color[1], fog->parms.color[2], 1.0f );
		qglClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	}
	else if ( r_clear->integer )
	{	// clear screen for debugging
		int i = r_clear->integer;
		if (i == 42) {
			i = Q_irand(0,8);
		}
		switch (i)
		{
		default:
			qglClearColor( 1, 0, 0.5, 1 );
			break;
		case 1:
			qglClearColor( 1.0, 0.0, 0.0, 1.0); //red
			break;
		case 2:
			qglClearColor( 0.0, 1.0, 0.0, 1.0); //green
			break;
		case 3:
			qglClearColor( 1.0, 1.0, 0.0, 1.0); //yellow
			break;
		case 4:
			qglClearColor( 0.0, 0.0, 1.0, 1.0); //blue
			break;
		case 5:
			qglClearColor( 0.0, 1.0, 1.0, 1.0); //cyan
			break;
		case 6:
			qglClearColor( 1.0, 0.0, 1.0, 1.0); //magenta
			break;
		case 7:
			qglClearColor( 1.0, 1.0, 1.0, 1.0); //white
			break;
		case 8:
			qglClearColor( 0.0, 0.0, 0.0, 1.0); //black
			break;
		}
		qglClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	}

	return (const void *)(cmd + 1);
}

/*
===============
RB_ShowImages

Draw all the images to the screen, on top of whatever
was there.  This is used to test for texture thrashing.

Also called by RE_EndRegistration
===============
*/
void RB_ShowImages( void ) {
	image_t	*image;
	float	x, y, w, h;
	//int		start, end;

	if ( !backEnd.projection2D ) {
		RB_SetGL2D();
	}

	qglFinish();

	//start = ri.Milliseconds();

	int i=0;
//	int iNumImages =
	   				 R_Images_StartIteration();
	while ( (image = R_Images_GetNextIteration()) != NULL)
	{
		w = glConfig.vidWidth / 20;
		h = glConfig.vidHeight / 15;
		x = i % 20 * w;
		y = i / 20 * h;

		// show in proportional size in mode 2
		if ( r_showImages->integer == 2 ) {
			w *= image->width / 512.0;
			h *= image->height / 512.0;
		}

		GL_Bind( image );
		qglBegin (GL_QUADS);
			qglTexCoord2f( 0, 0 );
			qglVertex2f( x, y );
			qglTexCoord2f( 1, 0 );
			qglVertex2f( x + w, y );
			qglTexCoord2f( 1, 1 );
			qglVertex2f( x + w, y + h );
			qglTexCoord2f( 0, 1 );
			qglVertex2f( x, y + h );
		qglEnd();
		i++;
	}

	qglFinish();

	//end = ri.Milliseconds();
	//ri.Printf( PRINT_ALL, "%i msec to draw all images\n", end - start );
}


/*
=============
RB_SwapBuffers

=============
*/
extern void RB_RenderWorldEffects( void );
const void	*RB_SwapBuffers( const void *data ) {
	const swapBuffersCommand_t	*cmd;

	// finish any 2D drawing if needed
	if ( tess.numIndexes ) {
		RB_EndSurface();
	}

	// texture swapping test
	if ( r_showImages->integer ) {
		RB_ShowImages();
	}

	cmd = (const swapBuffersCommand_t *)data;

	// we measure overdraw by reading back the stencil buffer and
	// counting up the number of increments that have happened
	if ( r_measureOverdraw->integer ) {
		int i;
		long sum = 0;
		unsigned char *stencilReadback;

		stencilReadback = (unsigned char *) Z_Malloc( glConfig.vidWidth * glConfig.vidHeight, TAG_TEMP_WORKSPACE, qfalse );
		qglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencilReadback );

		for ( i = 0; i < glConfig.vidWidth * glConfig.vidHeight; i++ ) {
			sum += stencilReadback[i];
		}

		backEnd.pc.c_overDraw += sum;
		Z_Free( stencilReadback );
	}

    if ( !glState.finishCalled ) {
        qglFinish();
	}

    GLimp_LogComment( "***************** RB_SwapBuffers *****************\n\n\n" );

	ri.WIN_Present(&window);

	backEnd.projection2D = qfalse;

	return (const void *)(cmd + 1);
}

const void	*RB_WorldEffects( const void *data )
{
	const setModeCommand_t	*cmd;

	cmd = (const setModeCommand_t *)data;

	// Always flush the tess buffer
	if ( tess.shader && tess.numIndexes )
	{
		RB_EndSurface();
	}
	RB_RenderWorldEffects();

	if(tess.shader)
	{
		RB_BeginSurface( tess.shader, tess.fogNum );
	}

	return (const void *)(cmd + 1);
}

/*
====================
RB_ExecuteRenderCommands
====================
*/
void RB_ExecuteRenderCommands( const void *data ) {
	int		t1, t2;

	t1 = ri.Milliseconds ();

	while ( 1 ) {
		data = PADP(data, sizeof(void *));

		switch ( *(const int *)data ) {
		case RC_SET_COLOR:
			data = RB_SetColor( data );
			break;
		case RC_STRETCH_PIC:
			data = RB_StretchPic( data );
			break;
		case RC_ROTATE_PIC:
			data = RB_RotatePic( data );
			break;
		case RC_ROTATE_PIC2:
			data = RB_RotatePic2( data );
			break;
		case RC_SCISSOR:
			data = RB_Scissor( data );
			break;
		case RC_DRAW_SURFS:
			data = RB_DrawSurfs( data );
			break;
		case RC_DRAW_BUFFER:
			data = RB_DrawBuffer( data );
			break;
		case RC_SWAP_BUFFERS:
			data = RB_SwapBuffers( data );
			break;
		case RC_WORLD_EFFECTS:
			data = RB_WorldEffects( data );
			break;
		case RC_END_OF_LIST:
		default:
			// stop rendering
			t2 = ri.Milliseconds ();
			backEnd.pc.msec = t2 - t1;
			return;
		}
	}

}

// What Pixel Shader type is currently active (regcoms or fragment programs).
GLuint g_uiCurrentPixelShaderType = 0x0;

// Begin using a Pixel Shader.
void BeginPixelShader( GLuint uiType, GLuint uiID )
{
	switch ( uiType )
	{
		// Using Register Combiners, so call the Display List that stores it.
		case GL_REGISTER_COMBINERS_NV:
		{
			// Just in case...
			if ( !qglCombinerParameterfvNV )
				return;

			// Call the list with the regcom in it.
			qglEnable( GL_REGISTER_COMBINERS_NV );
			qglCallList( uiID );

			g_uiCurrentPixelShaderType = GL_REGISTER_COMBINERS_NV;
		}
		return;

		// Using Fragment Programs, so call the program.
		case GL_FRAGMENT_PROGRAM_ARB:
		{
			// Just in case...
			if ( !qglGenProgramsARB )
				return;

			qglEnable( GL_FRAGMENT_PROGRAM_ARB );
			qglBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, uiID );

			g_uiCurrentPixelShaderType = GL_FRAGMENT_PROGRAM_ARB;
		}
		return;
	}
}

// Stop using a Pixel Shader and return states to normal.
void EndPixelShader()
{
	if ( g_uiCurrentPixelShaderType == 0x0 )
		return;

	qglDisable( g_uiCurrentPixelShaderType );
}

// Hack variable for deciding which kind of texture rectangle thing to do (for some
// reason it acts different on radeon! It's against the spec!).
extern bool g_bTextureRectangleHack;

static inline void RB_BlurGlowTexture()
{
	qglDisable (GL_CLIP_PLANE0);
	GL_Cull( CT_TWO_SIDED );

	// Go into orthographic 2d mode.
	qglMatrixMode(GL_PROJECTION);
	qglPushMatrix();
	qglLoadIdentity();
	qglOrtho(0, backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight, 0, -1, 1);
	qglMatrixMode(GL_MODELVIEW);
	qglPushMatrix();
	qglLoadIdentity();

	GL_State(GLS_DEPTHTEST_DISABLE);

	/////////////////////////////////////////////////////////
	// Setup vertex and pixel programs.
	/////////////////////////////////////////////////////////

	// NOTE: The 0.25 is because we're blending 4 textures (so = 1.0) and we want a relatively normalized pixel
	// intensity distribution, but this won't happen anyways if intensity is higher than 1.0.
	float fBlurDistribution = r_DynamicGlowIntensity->value * 0.25f;
	float fBlurWeight[4] = { fBlurDistribution, fBlurDistribution, fBlurDistribution, 1.0f };

	// Enable and set the Vertex Program.
	qglEnable( GL_VERTEX_PROGRAM_ARB );
	qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, tr.glowVShader );

	// Apply Pixel Shaders.
	if ( qglCombinerParameterfvNV )
	{
		BeginPixelShader( GL_REGISTER_COMBINERS_NV, tr.glowPShader );

		// Pass the blur weight to the regcom.
		qglCombinerParameterfvNV( GL_CONSTANT_COLOR0_NV, (float*)&fBlurWeight );
	}
	else if ( qglProgramEnvParameter4fARB )
	{
		BeginPixelShader( GL_FRAGMENT_PROGRAM_ARB, tr.glowPShader );

		// Pass the blur weight to the Fragment Program.
		qglProgramEnvParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, 0, fBlurWeight[0], fBlurWeight[1], fBlurWeight[2], fBlurWeight[3] );
	}

	/////////////////////////////////////////////////////////
	// Set the blur texture to the 4 texture stages.
	/////////////////////////////////////////////////////////

	// How much to offset each texel by.
	float fTexelWidthOffset = 0.1f, fTexelHeightOffset = 0.1f;

	GLuint uiTex = tr.screenGlow;

	qglActiveTextureARB( GL_TEXTURE3_ARB );
	qglEnable( GL_TEXTURE_RECTANGLE_ARB );
	qglBindTexture( GL_TEXTURE_RECTANGLE_ARB, uiTex );

	qglActiveTextureARB( GL_TEXTURE2_ARB );
	qglEnable( GL_TEXTURE_RECTANGLE_ARB );
	qglBindTexture( GL_TEXTURE_RECTANGLE_ARB, uiTex );

	qglActiveTextureARB( GL_TEXTURE1_ARB );
	qglEnable( GL_TEXTURE_RECTANGLE_ARB );
	qglBindTexture( GL_TEXTURE_RECTANGLE_ARB, uiTex );

	qglActiveTextureARB(GL_TEXTURE0_ARB );
	qglDisable( GL_TEXTURE_2D );
	qglEnable( GL_TEXTURE_RECTANGLE_ARB );
	qglBindTexture( GL_TEXTURE_RECTANGLE_ARB, uiTex );

	/////////////////////////////////////////////////////////
	// Draw the blur passes (each pass blurs it more, increasing the blur radius ).
	/////////////////////////////////////////////////////////

	//int iTexWidth = backEnd.viewParms.viewportWidth, iTexHeight = backEnd.viewParms.viewportHeight;
	int iTexWidth = glConfig.vidWidth, iTexHeight = glConfig.vidHeight;

	for ( int iNumBlurPasses = 0; iNumBlurPasses < r_DynamicGlowPasses->integer; iNumBlurPasses++ )
	{
		// Load the Texel Offsets into the Vertex Program.
		qglProgramEnvParameter4fARB( GL_VERTEX_PROGRAM_ARB, 0, -fTexelWidthOffset, -fTexelWidthOffset, 0.0f, 0.0f );
		qglProgramEnvParameter4fARB( GL_VERTEX_PROGRAM_ARB, 1, -fTexelWidthOffset, fTexelWidthOffset, 0.0f, 0.0f );
		qglProgramEnvParameter4fARB( GL_VERTEX_PROGRAM_ARB, 2, fTexelWidthOffset, -fTexelWidthOffset, 0.0f, 0.0f );
		qglProgramEnvParameter4fARB( GL_VERTEX_PROGRAM_ARB, 3, fTexelWidthOffset, fTexelWidthOffset, 0.0f, 0.0f );

		// After first pass put the tex coords to the viewport size.
		if ( iNumBlurPasses == 1 )
		{
			if ( !g_bTextureRectangleHack )
			{
				iTexWidth = backEnd.viewParms.viewportWidth;
				iTexHeight = backEnd.viewParms.viewportHeight;
			}

			uiTex = tr.blurImage;
			qglActiveTextureARB( GL_TEXTURE3_ARB );
			qglDisable( GL_TEXTURE_2D );
			qglEnable( GL_TEXTURE_RECTANGLE_ARB );
			qglBindTexture( GL_TEXTURE_RECTANGLE_ARB, uiTex );
			qglActiveTextureARB( GL_TEXTURE2_ARB );
			qglDisable( GL_TEXTURE_2D );
			qglEnable( GL_TEXTURE_RECTANGLE_ARB );
			qglBindTexture( GL_TEXTURE_RECTANGLE_ARB, uiTex );
			qglActiveTextureARB( GL_TEXTURE1_ARB );
			qglDisable( GL_TEXTURE_2D );
			qglEnable( GL_TEXTURE_RECTANGLE_ARB );
			qglBindTexture( GL_TEXTURE_RECTANGLE_ARB, uiTex );
			qglActiveTextureARB(GL_TEXTURE0_ARB );
			qglDisable( GL_TEXTURE_2D );
			qglEnable( GL_TEXTURE_RECTANGLE_ARB );
			qglBindTexture( GL_TEXTURE_RECTANGLE_ARB, uiTex );

			// Copy the current image over.
			qglBindTexture( GL_TEXTURE_RECTANGLE_ARB, uiTex );
			qglCopyTexSubImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, 0, 0, backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );
		}

		// Draw the fullscreen quad.
		qglBegin( GL_QUADS );
			qglMultiTexCoord2fARB( GL_TEXTURE0_ARB, 0, iTexHeight );
			qglVertex2f( 0, 0 );

			qglMultiTexCoord2fARB( GL_TEXTURE0_ARB, 0, 0 );
			qglVertex2f( 0, backEnd.viewParms.viewportHeight );

			qglMultiTexCoord2fARB( GL_TEXTURE0_ARB, iTexWidth, 0 );
			qglVertex2f( backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );

			qglMultiTexCoord2fARB( GL_TEXTURE0_ARB, iTexWidth, iTexHeight );
			qglVertex2f( backEnd.viewParms.viewportWidth, 0 );
		qglEnd();

		qglBindTexture( GL_TEXTURE_RECTANGLE_ARB, tr.blurImage );
		qglCopyTexSubImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, 0, 0, backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );

		// Increase the texel offsets.
		// NOTE: This is possibly the most important input to the effect. Even by using an exponential function I've been able to
		// make it look better (at a much higher cost of course). This is cheap though and still looks pretty great. In the future
		// I might want to use an actual gaussian equation to correctly calculate the pixel coefficients and attenuates, texel
		// offsets, gaussian amplitude and radius...
		fTexelWidthOffset += r_DynamicGlowDelta->value;
		fTexelHeightOffset += r_DynamicGlowDelta->value;
	}

	// Disable multi-texturing.
	qglActiveTextureARB( GL_TEXTURE3_ARB );
	qglDisable( GL_TEXTURE_RECTANGLE_ARB );

	qglActiveTextureARB( GL_TEXTURE2_ARB );
	qglDisable( GL_TEXTURE_RECTANGLE_ARB );

	qglActiveTextureARB( GL_TEXTURE1_ARB );
	qglDisable( GL_TEXTURE_RECTANGLE_ARB );

	qglActiveTextureARB(GL_TEXTURE0_ARB );
	qglDisable( GL_TEXTURE_RECTANGLE_ARB );
	qglEnable( GL_TEXTURE_2D );

	qglDisable( GL_VERTEX_PROGRAM_ARB );
	EndPixelShader();

	qglMatrixMode(GL_PROJECTION);
	qglPopMatrix();
	qglMatrixMode(GL_MODELVIEW);
	qglPopMatrix();

	qglDisable( GL_BLEND );
	glState.currenttmu = 0;	//this matches the last one we activated
}

// Draw the glow blur over the screen additively.
static inline void RB_DrawGlowOverlay()
{
	qglDisable (GL_CLIP_PLANE0);
	GL_Cull( CT_TWO_SIDED );

	// Go into orthographic 2d mode.
	qglMatrixMode(GL_PROJECTION);
	qglPushMatrix();
	qglLoadIdentity();
	qglOrtho(0, glConfig.vidWidth, glConfig.vidHeight, 0, -1, 1);
	qglMatrixMode(GL_MODELVIEW);
	qglPushMatrix();
	qglLoadIdentity();

	GL_State(GLS_DEPTHTEST_DISABLE);

	qglDisable( GL_TEXTURE_2D );
	qglEnable( GL_TEXTURE_RECTANGLE_ARB );

	// For debug purposes.
	if ( r_DynamicGlow->integer != 2 )
	{
		// Render the normal scene texture.
		qglBindTexture( GL_TEXTURE_RECTANGLE_ARB, tr.sceneImage );
		qglBegin(GL_QUADS);
			qglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
			qglTexCoord2f( 0, glConfig.vidHeight );
			qglVertex2f( 0, 0 );

			qglTexCoord2f( 0, 0 );
			qglVertex2f( 0, glConfig.vidHeight );

			qglTexCoord2f( glConfig.vidWidth, 0 );
			qglVertex2f( glConfig.vidWidth, glConfig.vidHeight );

			qglTexCoord2f( glConfig.vidWidth, glConfig.vidHeight );
			qglVertex2f( glConfig.vidWidth, 0 );
		qglEnd();
	}

	// One and Inverse Src Color give a very soft addition, while one one is a bit stronger. With one one we can
	// use additive blending through multitexture though.
	if ( r_DynamicGlowSoft->integer )
	{
		qglBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_COLOR );
	}
	else
	{
		qglBlendFunc( GL_ONE, GL_ONE );
	}
	qglEnable( GL_BLEND );

	// Now additively render the glow texture.
	qglBindTexture( GL_TEXTURE_RECTANGLE_ARB, tr.blurImage );
	qglBegin(GL_QUADS);
		qglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
		qglTexCoord2f( 0, r_DynamicGlowHeight->integer );
		qglVertex2f( 0, 0 );

		qglTexCoord2f( 0, 0 );
		qglVertex2f( 0, glConfig.vidHeight );

		qglTexCoord2f( r_DynamicGlowWidth->integer, 0 );
		qglVertex2f( glConfig.vidWidth, glConfig.vidHeight );

		qglTexCoord2f( r_DynamicGlowWidth->integer, r_DynamicGlowHeight->integer );
		qglVertex2f( glConfig.vidWidth, 0 );
	qglEnd();

	qglDisable( GL_TEXTURE_RECTANGLE_ARB );
	qglEnable( GL_TEXTURE_2D );
	qglBlendFunc( GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR );
	qglDisable( GL_BLEND );

	qglMatrixMode(GL_PROJECTION);
	qglPopMatrix();
	qglMatrixMode(GL_MODELVIEW);
	qglPopMatrix();
}
