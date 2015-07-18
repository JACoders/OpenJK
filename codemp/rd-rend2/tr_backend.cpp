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
#include "glext.h"

backEndData_t	*backEndData;
backEndState_t	backEnd;


static float	s_flipMatrix[16] = {
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
		ri->Printf( PRINT_WARNING, "GL_Bind: NULL image\n" );
		texnum = tr.defaultImage->texnum;
	} else {
		texnum = image->texnum;
	}

	if ( r_nobind->integer && tr.dlightImage ) {		// performance evaluation option
		texnum = tr.dlightImage->texnum;
	}

	if ( glState.currenttextures[glState.currenttmu] != texnum ) {
		if ( image ) {
			image->frameUsed = tr.frameCount;
		}
		glState.currenttextures[glState.currenttmu] = texnum;
		if (image && image->flags & IMGFLAG_CUBEMAP)
			qglBindTexture( GL_TEXTURE_CUBE_MAP, texnum );
		else
			qglBindTexture( GL_TEXTURE_2D, texnum );
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

	if (!(unit >= 0 && unit <= 31))
		ri->Error( ERR_DROP, "GL_SelectTexture: unit = %i", unit );

	qglActiveTexture( GL_TEXTURE0 + unit );

	glState.currenttmu = unit;
}

/*
** GL_BindToTMU
*/
void GL_BindToTMU( image_t *image, int tmu )
{
	int		texnum;
	int     oldtmu = glState.currenttmu;

	if (!image)
		texnum = 0;
	else
		texnum = image->texnum;

	if ( glState.currenttextures[tmu] != texnum ) {
		GL_SelectTexture( tmu );
		if (image)
			image->frameUsed = tr.frameCount;
		glState.currenttextures[tmu] = texnum;

		if (image && (image->flags & IMGFLAG_CUBEMAP))
			qglBindTexture( GL_TEXTURE_CUBE_MAP, texnum );
		else
			qglBindTexture( GL_TEXTURE_2D, texnum );
		GL_SelectTexture( oldtmu );
	}
}


/*
** GL_Cull
*/
void GL_Cull( int cullType ) {
	if ( glState.faceCulling == cullType ) {
		return;
	}

	glState.faceCulling = cullType;

	if ( backEnd.projection2D )
	{
		return;
	}

	if ( cullType == CT_TWO_SIDED ) 
	{
		qglDisable( GL_CULL_FACE );
	} 
	else 
	{
		qboolean cullFront;
		qglEnable( GL_CULL_FACE );

		cullFront = (qboolean)(cullType == CT_FRONT_SIDED);
		if ( backEnd.viewParms.isMirror )
		{
			cullFront = (qboolean)(!cullFront);
		}

		if ( backEnd.currentEntity && backEnd.currentEntity->mirrored )
		{
			cullFront = (qboolean)(!cullFront);
		}

		qglCullFace( cullFront ? GL_FRONT : GL_BACK );
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
		ri->Error( ERR_DROP, "GL_TexEnv: invalid env '%d' passed", env );
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
	if ( diff & GLS_DEPTHFUNC_BITS )
	{
		if ( stateBits & GLS_DEPTHFUNC_EQUAL )
		{
			qglDepthFunc( GL_EQUAL );
		}
		else if ( stateBits & GLS_DEPTHFUNC_GREATER)
		{
			qglDepthFunc( GL_GREATER );
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
		GLenum srcFactor = GL_ONE, dstFactor = GL_ONE;

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
				ri->Error( ERR_DROP, "GL_State: invalid src blend state bits" );
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
				ri->Error( ERR_DROP, "GL_State: invalid dst blend state bits" );
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
		case GLS_ATEST_LT_128:
			qglEnable( GL_ALPHA_TEST );
			qglAlphaFunc( GL_LESS, 0.5f );
			break;
		case GLS_ATEST_GE_128:
			qglEnable( GL_ALPHA_TEST );
			qglAlphaFunc( GL_GEQUAL, 0.5f );
			break;
		case GLS_ATEST_GE_192:
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


void GL_SetProjectionMatrix(matrix_t matrix)
{
	Matrix16Copy(matrix, glState.projection);
	Matrix16Multiply(glState.projection, glState.modelview, glState.modelviewProjection);	
}


void GL_SetModelviewMatrix(matrix_t matrix)
{
	Matrix16Copy(matrix, glState.modelview);
	Matrix16Multiply(glState.projection, glState.modelview, glState.modelviewProjection);	
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


static void SetViewportAndScissor( void ) {
	GL_SetProjectionMatrix( backEnd.viewParms.projectionMatrix );

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
void RB_BeginDrawingView (void) {
	int clearBits = 0;

	// we will need to change the projection matrix before drawing
	// 2D images again
	backEnd.projection2D = qfalse;

	// FIXME: HUGE HACK: render to the screen fbo if we've already postprocessed the frame and aren't drawing more world
	// drawing more world check is in case of double renders, such as skyportals
	if (backEnd.viewParms.targetFbo == NULL)
	{
		if (!tr.renderFbo || (backEnd.framePostProcessed && (backEnd.refdef.rdflags & RDF_NOWORLDMODEL)))
		{
			FBO_Bind(NULL);
		}
		else
		{
			FBO_Bind(tr.renderFbo);
		}
	}
	else
	{
		FBO_Bind(backEnd.viewParms.targetFbo);

		// FIXME: hack for cubemap testing
		if (tr.renderCubeFbo != NULL && backEnd.viewParms.targetFbo == tr.renderCubeFbo)
		{
			//qglFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + backEnd.viewParms.targetFboLayer, backEnd.viewParms.targetFbo->colorImage[0]->texnum, 0);
			qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + backEnd.viewParms.targetFboLayer, tr.cubemaps[backEnd.viewParms.targetFboCubemapIndex]->texnum, 0);
		}
	}

	//
	// set the modelview matrix for the viewer
	//
	SetViewportAndScissor();

	// ensures that depth writes are enabled for the depth clear
	GL_State( GLS_DEFAULT );
	// clear relevant buffers
	clearBits = GL_DEPTH_BUFFER_BIT;

	if ( r_measureOverdraw->integer || r_shadows->integer == 2 )
	{
		clearBits |= GL_STENCIL_BUFFER_BIT;
	}

	if ( r_fastsky->integer && !( backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) )
	{
		clearBits |= GL_COLOR_BUFFER_BIT;	// FIXME: only if sky shaders have been used
#ifdef _DEBUG
		qglClearColor( 0.8f, 0.7f, 0.4f, 1.0f );	// FIXME: get color of sky
#else
		qglClearColor( 0.0f, 0.0f, 0.0f, 1.0f );	// FIXME: get color of sky
#endif
	}

	// clear to white for shadow maps
	if (backEnd.viewParms.flags & VPF_SHADOWMAP)
	{
		clearBits |= GL_COLOR_BUFFER_BIT;
		qglClearColor( 1.0f, 1.0f, 1.0f, 1.0f );
	}

	// clear to black for cube maps
	if (tr.renderCubeFbo != NULL && backEnd.viewParms.targetFbo == tr.renderCubeFbo)
	{
		clearBits |= GL_COLOR_BUFFER_BIT;
		qglClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
	}

	qglClear( clearBits );

	if (backEnd.viewParms.targetFbo == NULL)
	{
		// Clear the glow target
		float black[] = {0.0f, 0.0f, 0.0f, 1.0f};
		qglClearBufferfv (GL_COLOR, 1, black);
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
#if 0
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
#endif
		GL_SetModelviewMatrix( s_flipMatrix );
	}
}


#define	MAC_EVENT_PUMP_MSEC		5

/*
==================
RB_RenderDrawSurfList
==================
*/
static void RB_RenderDrawSurfList( drawSurf_t *drawSurfs, int numDrawSurfs ) {
	shader_t		*shader, *oldShader;
	int				fogNum, oldFogNum;
	int				entityNum, oldEntityNum;
	int				dlighted, oldDlighted;
	int				postRender, oldPostRender;
	int             cubemapIndex, oldCubemapIndex;
	int		depthRange, oldDepthRange;
	int				i;
	drawSurf_t		*drawSurf;
	int				oldSort;
	float			originalTime;
	FBO_t*			fbo = NULL;
	qboolean		inQuery = qfalse;

	float			depth[2];


	// save original time for entity shader offsets
	originalTime = backEnd.refdef.floatTime;

	fbo = glState.currentFBO;

	// draw everything
	oldEntityNum = -1;
	backEnd.currentEntity = &tr.worldEntity;
	oldShader = NULL;
	oldFogNum = -1;
	oldDepthRange = 0;
	oldDlighted = 0;
	oldPostRender = 0;
	oldCubemapIndex = -1;
	oldSort = -1;

	depth[0] = 0.f;
	depth[1] = 1.f;

	backEnd.pc.c_surfaces += numDrawSurfs;

	for (i = 0, drawSurf = drawSurfs ; i < numDrawSurfs ; i++, drawSurf++) {
		if ( drawSurf->sort == oldSort && drawSurf->cubemapIndex == oldCubemapIndex) {
			if (backEnd.depthFill && shader && shader->sort != SS_OPAQUE)
				continue;

			// fast path, same as previous sort
			rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );
			continue;
		}
		oldSort = drawSurf->sort;
		R_DecomposeSort( drawSurf->sort, &entityNum, &shader, &fogNum, &dlighted, &postRender );
		cubemapIndex = drawSurf->cubemapIndex;

		//
		// change the tess parameters if needed
		// a "entityMergable" shader is a shader that can have surfaces from seperate
		// entities merged into a single batch, like smoke and blood puff sprites
		if ( shader != NULL && ( shader != oldShader || fogNum != oldFogNum || dlighted != oldDlighted || postRender != oldPostRender || cubemapIndex != oldCubemapIndex
			|| ( entityNum != oldEntityNum && !shader->entityMergable ) ) ) {
			if (oldShader != NULL) {
				RB_EndSurface();
			}
			RB_BeginSurface( shader, fogNum, cubemapIndex );
			backEnd.pc.c_surfBatches++;
			oldShader = shader;
			oldFogNum = fogNum;
			oldDlighted = dlighted;
			oldPostRender = postRender;
			oldCubemapIndex = cubemapIndex;
		}

		if (backEnd.depthFill && shader && shader->sort != SS_OPAQUE)
			continue;

		//
		// change the modelview matrix if needed
		//
		if ( entityNum != oldEntityNum ) {
			qboolean sunflare = qfalse;
			depthRange = 0;

			if ( entityNum != REFENTITYNUM_WORLD ) {
				backEnd.currentEntity = &backEnd.refdef.entities[entityNum];
				backEnd.refdef.floatTime = originalTime - backEnd.currentEntity->e.shaderTime;
				// we have to reset the shaderTime as well otherwise image animations start
				// from the wrong frame
				tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;

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
					depthRange = 1;
				}
			} else {
				backEnd.currentEntity = &tr.worldEntity;
				backEnd.refdef.floatTime = originalTime;
				backEnd.ori = backEnd.viewParms.world;
				// we have to reset the shaderTime as well otherwise image animations on
				// the world (like water) continue with the wrong frame
				tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;
				R_TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.ori );
			}

			GL_SetModelviewMatrix( backEnd.ori.modelMatrix );

			//
			// change depthrange. Also change projection matrix so first person weapon does not look like coming
			// out of the screen.
			//
			if (oldDepthRange != depthRange)
			{
				switch ( depthRange ) {
					default:
					case 0:
						if(backEnd.viewParms.stereoFrame != STEREO_CENTER)
						{
							GL_SetProjectionMatrix( backEnd.viewParms.projectionMatrix );
						}

						if (!sunflare)
							qglDepthRange (0, 1);

						depth[0] = 0;
						depth[1] = 1;
						break;

					case 1:
						if(backEnd.viewParms.stereoFrame != STEREO_CENTER)
						{
							viewParms_t temp = backEnd.viewParms;

							R_SetupProjection(&temp, r_znear->value, 0, qfalse);

							GL_SetProjectionMatrix( temp.projectionMatrix );
						}

 						if(!oldDepthRange)
						{
							depth[0] = 0;
							depth[1] = 0.3f;
 							qglDepthRange (depth[0], depth[1]);
	 					}
						break;

					case 2:
						if(backEnd.viewParms.stereoFrame != STEREO_CENTER)
						{
							viewParms_t temp = backEnd.viewParms;

							R_SetupProjection(&temp, r_znear->value, 0, qfalse);

							GL_SetProjectionMatrix( temp.projectionMatrix );
						}

 						if(!oldDepthRange)
						{
							depth[0] = 0.0f;
							depth[1] = 0.0f;
 							qglDepthRange (depth[0], depth[1]);
	 					}
						break;
				}

				oldDepthRange = depthRange;
			}

			oldEntityNum = entityNum;
		}

		// add the triangles for this surface
		rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );
	}

	backEnd.refdef.floatTime = originalTime;

	// draw the contents of the last shader batch
	if (oldShader != NULL) {
		RB_EndSurface();
	}

	if (inQuery) {
		qglEndQuery(GL_SAMPLES_PASSED);
	}

	FBO_Bind(fbo);

	// go back to the world modelview matrix

	GL_SetModelviewMatrix( backEnd.viewParms.world.modelMatrix );

	//qglDepthRange (0, 1);
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
	matrix_t matrix;
	int width, height;

	if (backEnd.projection2D && backEnd.last2DFBO == glState.currentFBO)
		return;

	backEnd.projection2D = qtrue;
	backEnd.last2DFBO = glState.currentFBO;

	if (glState.currentFBO)
	{
		width = glState.currentFBO->width;
		height = glState.currentFBO->height;
	}
	else
	{
		width = glConfig.vidWidth;
		height = glConfig.vidHeight;
	}

	// set 2D virtual screen size
	qglViewport( 0, 0, width, height );
	qglScissor( 0, 0, width, height );

	Matrix16Ortho(0, 640, 480, 0, 0, 1, matrix);
	GL_SetProjectionMatrix(matrix);
	Matrix16Identity(matrix);
	GL_SetModelviewMatrix(matrix);

	GL_State( GLS_DEPTHTEST_DISABLE |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

	qglDisable( GL_CULL_FACE );
	qglDisable( GL_CLIP_PLANE0 );

	// set time for 2D shaders
	backEnd.refdef.time = ri->Milliseconds();
	backEnd.refdef.floatTime = backEnd.refdef.time * 0.001f;

	// reset color scaling
	backEnd.refdef.colorScale = 1.0f;
}


/*
=============
RE_StretchRaw

FIXME: not exactly backend
Stretches a raw 32 bit power of 2 bitmap image over the given screen rectangle.
Used for cinematics.
=============
*/
void RE_StretchRaw (int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty) {
	int			i, j;
	int			start, end;
	vec4_t quadVerts[4];
	vec2_t texCoords[4];

	if ( !tr.registered ) {
		return;
	}
	R_IssuePendingRenderCommands();

	if ( tess.numIndexes ) {
		RB_EndSurface();
	}

	// we definately want to sync every frame for the cinematics
	qglFinish();

	start = 0;
	if ( r_speeds->integer ) {
		start = ri->Milliseconds();
	}

	// make sure rows and cols are powers of 2
	for ( i = 0 ; ( 1 << i ) < cols ; i++ ) {
	}
	for ( j = 0 ; ( 1 << j ) < rows ; j++ ) {
	}
	if ( ( 1 << i ) != cols || ( 1 << j ) != rows) {
		ri->Error (ERR_DROP, "Draw_StretchRaw: size not a power of 2: %i by %i", cols, rows);
	}

	RE_UploadCinematic (cols, rows, data, client, dirty);

	if ( r_speeds->integer ) {
		end = ri->Milliseconds();
		ri->Printf( PRINT_ALL, "qglTexSubImage2D %i, %i: %i msec\n", cols, rows, end - start );
	}

	// FIXME: HUGE hack
	if (!tr.renderFbo || backEnd.framePostProcessed)
	{
		FBO_Bind(NULL);
	}
	else
	{
		FBO_Bind(tr.renderFbo);
	}

	RB_SetGL2D();

	VectorSet4(quadVerts[0], x,     y,     0.0f, 1.0f);
	VectorSet4(quadVerts[1], x + w, y,     0.0f, 1.0f);
	VectorSet4(quadVerts[2], x + w, y + h, 0.0f, 1.0f);
	VectorSet4(quadVerts[3], x,     y + h, 0.0f, 1.0f);

	VectorSet2(texCoords[0], 0.5f / cols,          0.5f / rows);
	VectorSet2(texCoords[1], (cols - 0.5f) / cols, 0.5f / rows);
	VectorSet2(texCoords[2], (cols - 0.5f) / cols, (rows - 0.5f) / rows);
	VectorSet2(texCoords[3], 0.5f / cols,          (rows - 0.5f) / rows);

	GLSL_BindProgram(&tr.textureColorShader);
	
	GLSL_SetUniformMatrix16(&tr.textureColorShader, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
	GLSL_SetUniformVec4(&tr.textureColorShader, UNIFORM_COLOR, colorWhite);

	RB_InstantQuad2(quadVerts, texCoords);
}

void RE_UploadCinematic (int cols, int rows, const byte *data, int client, qboolean dirty) {

	GL_Bind( tr.scratchImage[client] );

	// if the scratchImage isn't in the format we want, specify it as a new texture
	if ( cols != tr.scratchImage[client]->width || rows != tr.scratchImage[client]->height ) {
		tr.scratchImage[client]->width = tr.scratchImage[client]->uploadWidth = cols;
		tr.scratchImage[client]->height = tr.scratchImage[client]->uploadHeight = rows;
		qglTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );	
	} else {
		if (dirty) {
			// otherwise, just subimage upload it so that drivers can tell we are going to be changing
			// it and don't try and do a texture compression
			qglTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, data );
		}
	}
}


/*
=============
RB_SetColor

=============
*/
static const void	*RB_SetColor( const void *data ) {
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
static const void *RB_StretchPic ( const void *data ) {
	const stretchPicCommand_t	*cmd;
	shader_t *shader;
	int		numVerts, numIndexes;

	cmd = (const stretchPicCommand_t *)data;

	// FIXME: HUGE hack
	if (!tr.renderFbo || backEnd.framePostProcessed)
	{
		FBO_Bind(NULL);
	}
	else
	{
		FBO_Bind(tr.renderFbo);
	}

	RB_SetGL2D();

	shader = cmd->shader;
	if ( shader != tess.shader ) {
		if ( tess.numIndexes ) {
			RB_EndSurface();
		}
		backEnd.currentEntity = &backEnd.entity2D;
		RB_BeginSurface( shader, 0, 0 );
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

	{
		vec4_t color;

		VectorScale4(backEnd.color2D, 1.0f / 255.0f, color);

		VectorCopy4(color, tess.vertexColors[ numVerts ]);
		VectorCopy4(color, tess.vertexColors[ numVerts + 1]);
		VectorCopy4(color, tess.vertexColors[ numVerts + 2]);
		VectorCopy4(color, tess.vertexColors[ numVerts + 3 ]);
	}

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
RB_DrawRotatePic
=============
*/
static const void *RB_RotatePic ( const void *data ) 
{
	const rotatePicCommand_t	*cmd;
	image_t *image;
	shader_t *shader;
	int numVerts;
	int numIndexes;
	vec3_t point, rotatedPoint;
	vec3_t axis = { 0.0f, 0.0f, 1.0f };
	vec3_t xlat;

	cmd = (const rotatePicCommand_t *)data;

	// FIXME: HUGE hack
	if (!tr.renderFbo || backEnd.framePostProcessed)
	{
		FBO_Bind(NULL);
	}
	else
	{
		FBO_Bind(tr.renderFbo);
	}

	RB_SetGL2D();

	shader = cmd->shader;
	image = shader->stages[0]->bundle[0].image[0];
	if ( shader != tess.shader ) {
		if ( tess.numIndexes ) {
			RB_EndSurface();
		}
		backEnd.currentEntity = &backEnd.entity2D;
		RB_BeginSurface( shader, 0, 0 );
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

	{
		vec4_t color;

		VectorScale4(backEnd.color2D, 1.0f / 255.0f, color);

		VectorCopy4(color, tess.vertexColors[ numVerts ]);
		VectorCopy4(color, tess.vertexColors[ numVerts + 1]);
		VectorCopy4(color, tess.vertexColors[ numVerts + 2]);
		VectorCopy4(color, tess.vertexColors[ numVerts + 3 ]);
	}

	VectorSet (xlat, cmd->x, cmd->y, 0.0f);

	VectorSet (point, -cmd->w, 0.0f, 0.0f);
	RotatePointAroundVector (rotatedPoint, axis, point, cmd->a);
	VectorAdd (rotatedPoint, xlat, tess.xyz[numVerts]);

	tess.texCoords[ numVerts ][0][0] = cmd->s1;
	tess.texCoords[ numVerts ][0][1] = cmd->t1;

	VectorSet (point, 0.0f, 0.0f, 0.0f);
	RotatePointAroundVector (rotatedPoint, axis, point, cmd->a);
	VectorAdd (rotatedPoint, xlat, tess.xyz[numVerts + 1]);

	tess.texCoords[ numVerts + 1 ][0][0] = cmd->s2;
	tess.texCoords[ numVerts + 1 ][0][1] = cmd->t1;

	VectorSet (point, 0.0f, cmd->h, 0.0f);
	RotatePointAroundVector (rotatedPoint, axis, point, cmd->a);
	VectorAdd (rotatedPoint, xlat, tess.xyz[numVerts + 2]);

	tess.texCoords[ numVerts + 2 ][0][0] = cmd->s2;
	tess.texCoords[ numVerts + 2 ][0][1] = cmd->t2;

	VectorSet (point, -cmd->w, cmd->h, 0.0f);
	RotatePointAroundVector (rotatedPoint, axis, point, cmd->a);
	VectorAdd (rotatedPoint, xlat, tess.xyz[numVerts + 3]);

	tess.texCoords[ numVerts + 3 ][0][0] = cmd->s1;
	tess.texCoords[ numVerts + 3 ][0][1] = cmd->t2;

	return (const void *)(cmd + 1);
}

/*
=============
RB_DrawRotatePic2
=============
*/
static const void *RB_RotatePic2 ( const void *data ) 
{
	const rotatePicCommand_t	*cmd;
	image_t *image;
	shader_t *shader;
	int numVerts;
	int numIndexes;
	vec3_t point, rotatedPoint;
	vec3_t axis = { 0.0f, 0.0f, 1.0f };
	vec3_t xlat;

	cmd = (const rotatePicCommand_t *)data;

	// FIXME: HUGE hack
	if (!tr.renderFbo || backEnd.framePostProcessed)
	{
		FBO_Bind(NULL);
	}
	else
	{
		FBO_Bind(tr.renderFbo);
	}

	RB_SetGL2D();

	shader = cmd->shader;
	image = shader->stages[0]->bundle[0].image[0];
	if ( shader != tess.shader ) {
		if ( tess.numIndexes ) {
			RB_EndSurface();
		}
		backEnd.currentEntity = &backEnd.entity2D;
		RB_BeginSurface( shader, 0, 0 );
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

	{
		vec4_t color;

		VectorScale4(backEnd.color2D, 1.0f / 255.0f, color);

		VectorCopy4(color, tess.vertexColors[ numVerts ]);
		VectorCopy4(color, tess.vertexColors[ numVerts + 1]);
		VectorCopy4(color, tess.vertexColors[ numVerts + 2]);
		VectorCopy4(color, tess.vertexColors[ numVerts + 3 ]);
	}

	VectorSet (xlat, cmd->x, cmd->y, 0.0f);

	VectorSet (point, -cmd->w * 0.5f, -cmd->h * 0.5f, 0.0f);
	RotatePointAroundVector (rotatedPoint, axis, point, -cmd->a);
	VectorAdd (rotatedPoint, xlat, tess.xyz[numVerts]);

	tess.texCoords[ numVerts ][0][0] = cmd->s1;
	tess.texCoords[ numVerts ][0][1] = cmd->t1;

	VectorSet (point, cmd->w * 0.5f, -cmd->h * 0.5f, 0.0f);
	RotatePointAroundVector (rotatedPoint, axis, point, -cmd->a);
	VectorAdd (rotatedPoint, xlat, tess.xyz[numVerts + 1]);

	tess.texCoords[ numVerts + 1 ][0][0] = cmd->s2;
	tess.texCoords[ numVerts + 1 ][0][1] = cmd->t1;

	VectorSet (point, cmd->w * 0.5f, cmd->h * 0.5f, 0.0f);
	RotatePointAroundVector (rotatedPoint, axis, point, -cmd->a);
	VectorAdd (rotatedPoint, xlat, tess.xyz[numVerts + 2]);

	tess.texCoords[ numVerts + 2 ][0][0] = cmd->s2;
	tess.texCoords[ numVerts + 2 ][0][1] = cmd->t2;

	VectorSet (point, -cmd->w * 0.5f, cmd->h * 0.5f, 0.0f);
	RotatePointAroundVector (rotatedPoint, axis, point, -cmd->a);
	VectorAdd (rotatedPoint, xlat, tess.xyz[numVerts + 3]);

	tess.texCoords[ numVerts + 3 ][0][0] = cmd->s1;
	tess.texCoords[ numVerts + 3 ][0][1] = cmd->t2;

	return (const void *)(cmd + 1);
}


/*
=============
RB_DrawSurfs

=============
*/
static const void	*RB_DrawSurfs( const void *data ) {
	const drawSurfsCommand_t	*cmd;

	// finish any 2D drawing if needed
	if ( tess.numIndexes ) {
		RB_EndSurface();
	}

	cmd = (const drawSurfsCommand_t *)data;

	backEnd.refdef = cmd->refdef;
	backEnd.viewParms = cmd->viewParms;

	// clear the z buffer, set the modelview, etc
	RB_BeginDrawingView ();

	if (backEnd.viewParms.flags & VPF_DEPTHCLAMP)
	{
		qglEnable(GL_DEPTH_CLAMP);
	}

	if (!(backEnd.refdef.rdflags & RDF_NOWORLDMODEL) && (r_depthPrepass->integer || (backEnd.viewParms.flags & VPF_DEPTHSHADOW)))
	{
		FBO_t *oldFbo = glState.currentFBO;

		backEnd.depthFill = qtrue;
		qglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		RB_RenderDrawSurfList( cmd->drawSurfs, cmd->numDrawSurfs );
		qglColorMask(!backEnd.colorMask[0], !backEnd.colorMask[1], !backEnd.colorMask[2], !backEnd.colorMask[3]);
		backEnd.depthFill = qfalse;

		if (tr.msaaResolveFbo)
		{
			// If we're using multisampling, resolve the depth first
			FBO_FastBlit(tr.renderFbo, NULL, tr.msaaResolveFbo, NULL, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		}
		else if (tr.renderFbo == NULL)
		{
			// If we're rendering directly to the screen, copy the depth to a texture
			GL_BindToTMU(tr.renderDepthImage, 0);
			qglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 0, 0, glConfig.vidWidth, glConfig.vidHeight, 0);
		}

		if (r_ssao->integer)
		{
			// need the depth in a texture we can do GL_LINEAR sampling on, so copy it to an HDR image
			FBO_BlitFromTexture(tr.renderDepthImage, NULL, NULL, tr.hdrDepthFbo, NULL, NULL, NULL, 0);
		}

		if (r_sunlightMode->integer && backEnd.viewParms.flags & VPF_USESUNLIGHT)
		{
			vec4_t quadVerts[4];
			vec2_t texCoords[4];
			vec4_t box;

			FBO_Bind(tr.screenShadowFbo);

			box[0] = backEnd.viewParms.viewportX      * tr.screenShadowFbo->width  / (float)glConfig.vidWidth;
			box[1] = backEnd.viewParms.viewportY      * tr.screenShadowFbo->height / (float)glConfig.vidHeight;
			box[2] = backEnd.viewParms.viewportWidth  * tr.screenShadowFbo->width  / (float)glConfig.vidWidth;
			box[3] = backEnd.viewParms.viewportHeight * tr.screenShadowFbo->height / (float)glConfig.vidHeight;

			qglViewport(box[0], box[1], box[2], box[3]);
			qglScissor(box[0], box[1], box[2], box[3]);

			box[0] = backEnd.viewParms.viewportX               / (float)glConfig.vidWidth;
			box[1] = backEnd.viewParms.viewportY               / (float)glConfig.vidHeight;
			box[2] = box[0] + backEnd.viewParms.viewportWidth  / (float)glConfig.vidWidth;
			box[3] = box[1] + backEnd.viewParms.viewportHeight / (float)glConfig.vidHeight;

			texCoords[0][0] = box[0]; texCoords[0][1] = box[3];
			texCoords[1][0] = box[2]; texCoords[1][1] = box[3];
			texCoords[2][0] = box[2]; texCoords[2][1] = box[1];
			texCoords[3][0] = box[0]; texCoords[3][1] = box[1];

			box[0] = -1.0f;
			box[1] = -1.0f;
			box[2] =  1.0f;
			box[3] =  1.0f;

			VectorSet4(quadVerts[0], box[0], box[3], 0, 1);
			VectorSet4(quadVerts[1], box[2], box[3], 0, 1);
			VectorSet4(quadVerts[2], box[2], box[1], 0, 1);
			VectorSet4(quadVerts[3], box[0], box[1], 0, 1);

			GL_State( GLS_DEPTHTEST_DISABLE );

			GLSL_BindProgram(&tr.shadowmaskShader);

			GL_BindToTMU(tr.renderDepthImage, TB_COLORMAP);
			GL_BindToTMU(tr.sunShadowDepthImage[0], TB_SHADOWMAP);
			GL_BindToTMU(tr.sunShadowDepthImage[1], TB_SHADOWMAP2);
			GL_BindToTMU(tr.sunShadowDepthImage[2], TB_SHADOWMAP3);

			GLSL_SetUniformMatrix16(&tr.shadowmaskShader, UNIFORM_SHADOWMVP,  backEnd.refdef.sunShadowMvp[0]);
			GLSL_SetUniformMatrix16(&tr.shadowmaskShader, UNIFORM_SHADOWMVP2, backEnd.refdef.sunShadowMvp[1]);
			GLSL_SetUniformMatrix16(&tr.shadowmaskShader, UNIFORM_SHADOWMVP3, backEnd.refdef.sunShadowMvp[2]);
			
			GLSL_SetUniformVec3(&tr.shadowmaskShader, UNIFORM_VIEWORIGIN,  backEnd.refdef.vieworg);
			{
				vec4_t viewInfo;
				vec3_t viewVector;

				float zmax = backEnd.viewParms.zFar;
				float ymax = zmax * tan(backEnd.viewParms.fovY * M_PI / 360.0f);
				float xmax = zmax * tan(backEnd.viewParms.fovX * M_PI / 360.0f);

				float zmin = r_znear->value;

				VectorScale(backEnd.refdef.viewaxis[0], zmax, viewVector);
				GLSL_SetUniformVec3(&tr.shadowmaskShader, UNIFORM_VIEWFORWARD, viewVector);
				VectorScale(backEnd.refdef.viewaxis[1], xmax, viewVector);
				GLSL_SetUniformVec3(&tr.shadowmaskShader, UNIFORM_VIEWLEFT,    viewVector);
				VectorScale(backEnd.refdef.viewaxis[2], ymax, viewVector);
				GLSL_SetUniformVec3(&tr.shadowmaskShader, UNIFORM_VIEWUP,      viewVector);

				VectorSet4(viewInfo, zmax / zmin, zmax, 0.0, 0.0);

				GLSL_SetUniformVec4(&tr.shadowmaskShader, UNIFORM_VIEWINFO, viewInfo);
			}


			RB_InstantQuad2(quadVerts, texCoords); //, color, shaderProgram, invTexRes);
		}

		if (r_ssao->integer)
		{
			vec4_t quadVerts[4];
			vec2_t texCoords[4];

			FBO_Bind(tr.quarterFbo[0]);

			qglViewport(0, 0, tr.quarterFbo[0]->width, tr.quarterFbo[0]->height);
			qglScissor(0, 0, tr.quarterFbo[0]->width, tr.quarterFbo[0]->height);

			VectorSet4(quadVerts[0], -1,  1, 0, 1);
			VectorSet4(quadVerts[1],  1,  1, 0, 1);
			VectorSet4(quadVerts[2],  1, -1, 0, 1);
			VectorSet4(quadVerts[3], -1, -1, 0, 1);

			texCoords[0][0] = 0; texCoords[0][1] = 1;
			texCoords[1][0] = 1; texCoords[1][1] = 1;
			texCoords[2][0] = 1; texCoords[2][1] = 0;
			texCoords[3][0] = 0; texCoords[3][1] = 0;

			GL_State( GLS_DEPTHTEST_DISABLE );

			GLSL_BindProgram(&tr.ssaoShader);

			GL_BindToTMU(tr.hdrDepthImage, TB_COLORMAP);

			{
				vec4_t viewInfo;

				float zmax = backEnd.viewParms.zFar;
				float zmin = r_znear->value;

				VectorSet4(viewInfo, zmax / zmin, zmax, 0.0, 0.0);

				GLSL_SetUniformVec4(&tr.ssaoShader, UNIFORM_VIEWINFO, viewInfo);
			}

			RB_InstantQuad2(quadVerts, texCoords); //, color, shaderProgram, invTexRes);


			FBO_Bind(tr.quarterFbo[1]);

			qglViewport(0, 0, tr.quarterFbo[1]->width, tr.quarterFbo[1]->height);
			qglScissor(0, 0, tr.quarterFbo[1]->width, tr.quarterFbo[1]->height);

			GLSL_BindProgram(&tr.depthBlurShader[0]);

			GL_BindToTMU(tr.quarterImage[0],  TB_COLORMAP);
			GL_BindToTMU(tr.hdrDepthImage, TB_LIGHTMAP);

			{
				vec4_t viewInfo;

				float zmax = backEnd.viewParms.zFar;
				float zmin = r_znear->value;

				VectorSet4(viewInfo, zmax / zmin, zmax, 0.0, 0.0);

				GLSL_SetUniformVec4(&tr.depthBlurShader[0], UNIFORM_VIEWINFO, viewInfo);
			}

			RB_InstantQuad2(quadVerts, texCoords); //, color, shaderProgram, invTexRes);


			FBO_Bind(tr.screenSsaoFbo);

			qglViewport(0, 0, tr.screenSsaoFbo->width, tr.screenSsaoFbo->height);
			qglScissor(0, 0, tr.screenSsaoFbo->width, tr.screenSsaoFbo->height);

			GLSL_BindProgram(&tr.depthBlurShader[1]);

			GL_BindToTMU(tr.quarterImage[1],  TB_COLORMAP);
			GL_BindToTMU(tr.hdrDepthImage, TB_LIGHTMAP);

			{
				vec4_t viewInfo;

				float zmax = backEnd.viewParms.zFar;
				float zmin = r_znear->value;

				VectorSet4(viewInfo, zmax / zmin, zmax, 0.0, 0.0);

				GLSL_SetUniformVec4(&tr.depthBlurShader[1], UNIFORM_VIEWINFO, viewInfo);
			}


			RB_InstantQuad2(quadVerts, texCoords); //, color, shaderProgram, invTexRes);
		}

		// reset viewport and scissor
		FBO_Bind(oldFbo);
		SetViewportAndScissor();
	}

	if (backEnd.viewParms.flags & VPF_DEPTHCLAMP)
	{
		qglDisable(GL_DEPTH_CLAMP);
	}

	if (!(backEnd.viewParms.flags & VPF_DEPTHSHADOW))
	{
		RB_RenderDrawSurfList( cmd->drawSurfs, cmd->numDrawSurfs );

		if (r_drawSun->integer)
		{
			RB_DrawSun(0.1, tr.sunShader);
		}

		if (r_drawSunRays->integer)
		{
			FBO_t *oldFbo = glState.currentFBO;
			FBO_Bind(tr.sunRaysFbo);
			
			qglClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
			qglClear( GL_COLOR_BUFFER_BIT );

			tr.sunFlareQueryActive[tr.sunFlareQueryIndex] = qtrue;
			qglBeginQuery(GL_SAMPLES_PASSED, tr.sunFlareQuery[tr.sunFlareQueryIndex]);

			RB_DrawSun(0.3, tr.sunFlareShader);

			qglEndQuery(GL_SAMPLES_PASSED);

			FBO_Bind(oldFbo);
		}

		// darken down any stencil shadows
		RB_ShadowFinish();		

		// add light flares on lights that aren't obscured
		RB_RenderFlares();
	}

	if (tr.renderCubeFbo != NULL && backEnd.viewParms.targetFbo == tr.renderCubeFbo)
	{
		FBO_Bind(NULL);
		GL_SelectTexture(TB_CUBEMAP);
		GL_BindToTMU(tr.cubemaps[backEnd.viewParms.targetFboCubemapIndex], TB_CUBEMAP);
		qglGenerateMipmap(GL_TEXTURE_CUBE_MAP);
		GL_SelectTexture(0);
	}

	return (const void *)(cmd + 1);
}


/*
=============
RB_DrawBuffer

=============
*/
static const void	*RB_DrawBuffer( const void *data ) {
	const drawBufferCommand_t	*cmd;

	cmd = (const drawBufferCommand_t *)data;

	// finish any 2D drawing if needed
	if(tess.numIndexes)
		RB_EndSurface();

	FBO_Bind(NULL);

	qglDrawBuffer( cmd->buffer );

	// clear screen for debugging
	if ( r_clear->integer ) {
		qglClearColor( 1, 0, 0.5, 1 );
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
	int		i;
	image_t	*image;
	float	x, y, w, h;
	int		start, end;

	RB_SetGL2D();

	qglClear( GL_COLOR_BUFFER_BIT );

	qglFinish();

	start = ri->Milliseconds();

	for ( i=0 ; i<tr.numImages ; i++ ) {
		image = tr.images[i];

		w = glConfig.vidWidth / 20;
		h = glConfig.vidHeight / 15;
		x = i % 20 * w;
		y = i / 20 * h;

		// show in proportional size in mode 2
		if ( r_showImages->integer == 2 ) {
			w *= image->uploadWidth / 512.0f;
			h *= image->uploadHeight / 512.0f;
		}

		{
			vec4_t quadVerts[4];

			GL_Bind(image);

			VectorSet4(quadVerts[0], x, y, 0, 1);
			VectorSet4(quadVerts[1], x + w, y, 0, 1);
			VectorSet4(quadVerts[2], x + w, y + h, 0, 1);
			VectorSet4(quadVerts[3], x, y + h, 0, 1);

			RB_InstantQuad(quadVerts);
		}
	}

	qglFinish();

	end = ri->Milliseconds();
	ri->Printf( PRINT_ALL, "%i msec to draw all images\n", end - start );

}

/*
=============
RB_ColorMask

=============
*/
static const void *RB_ColorMask(const void *data)
{
	const colorMaskCommand_t *cmd = (colorMaskCommand_t *)data;

	// finish any 2D drawing if needed
	if(tess.numIndexes)
		RB_EndSurface();

	// reverse color mask, so 0 0 0 0 is the default
	backEnd.colorMask[0] = (qboolean)(!cmd->rgba[0]);
	backEnd.colorMask[1] = (qboolean)(!cmd->rgba[1]);
	backEnd.colorMask[2] = (qboolean)(!cmd->rgba[2]);
	backEnd.colorMask[3] = (qboolean)(!cmd->rgba[3]);

	qglColorMask(cmd->rgba[0], cmd->rgba[1], cmd->rgba[2], cmd->rgba[3]);
	
	return (const void *)(cmd + 1);
}

/*
=============
RB_ClearDepth

=============
*/
static const void *RB_ClearDepth(const void *data)
{
	const clearDepthCommand_t *cmd = (clearDepthCommand_t *)data;
	
	// finish any 2D drawing if needed
	if(tess.numIndexes)
		RB_EndSurface();

	// texture swapping test
	if (r_showImages->integer)
		RB_ShowImages();

	if (!tr.renderFbo || backEnd.framePostProcessed)
	{
		FBO_Bind(NULL);
	}
	else
	{
		FBO_Bind(tr.renderFbo);
	}

	qglClear(GL_DEPTH_BUFFER_BIT);

	// if we're doing MSAA, clear the depth texture for the resolve buffer
	if (tr.msaaResolveFbo)
	{
		FBO_Bind(tr.msaaResolveFbo);
		qglClear(GL_DEPTH_BUFFER_BIT);
	}

	
	return (const void *)(cmd + 1);
}


/*
=============
RB_SwapBuffers

=============
*/
static const void	*RB_SwapBuffers( const void *data ) {
	const swapBuffersCommand_t	*cmd;

	// finish any 2D drawing if needed
	if ( tess.numIndexes ) {
		RB_EndSurface();
	}

	ResetGhoul2RenderableSurfaceHeap();

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

		stencilReadback = (unsigned char *)ri->Hunk_AllocateTempMemory( glConfig.vidWidth * glConfig.vidHeight );
		qglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencilReadback );

		for ( i = 0; i < glConfig.vidWidth * glConfig.vidHeight; i++ ) {
			sum += stencilReadback[i];
		}

		backEnd.pc.c_overDraw += sum;
		ri->Hunk_FreeTempMemory( stencilReadback );
	}

	if (!backEnd.framePostProcessed)
	{
		if (tr.msaaResolveFbo && r_hdr->integer)
		{
			// Resolving an RGB16F MSAA FBO to the screen messes with the brightness, so resolve to an RGB16F FBO first
			FBO_FastBlit(tr.renderFbo, NULL, tr.msaaResolveFbo, NULL, GL_COLOR_BUFFER_BIT, GL_NEAREST);
			FBO_FastBlit(tr.msaaResolveFbo, NULL, NULL, NULL, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		}
		else if (tr.renderFbo)
		{
			FBO_FastBlit(tr.renderFbo, NULL, NULL, NULL, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		}
	}

	GLimp_LogComment( "***************** RB_SwapBuffers *****************\n\n\n" );

	ri->WIN_Present( &window );

	backEnd.framePostProcessed = qfalse;
	backEnd.projection2D = qfalse;

	return (const void *)(cmd + 1);
}

/*
=============
RB_CapShadowMap

=============
*/
static const void *RB_CaptureShadowMap(const void *data)
{
	const capShadowmapCommand_t *cmd = (const capShadowmapCommand_t *)data;

	// finish any 2D drawing if needed
	if(tess.numIndexes)
		RB_EndSurface();

	if (cmd->map != -1)
	{
		GL_SelectTexture(0);
		if (cmd->cubeSide != -1)
		{
			if (tr.shadowCubemaps[cmd->map] != NULL)
			{
				GL_Bind(tr.shadowCubemaps[cmd->map]);
				qglCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + cmd->cubeSide, 0, GL_RGBA8, backEnd.refdef.x, glConfig.vidHeight - ( backEnd.refdef.y + PSHADOW_MAP_SIZE ), PSHADOW_MAP_SIZE, PSHADOW_MAP_SIZE, 0);
			}
		}
		else
		{
			if (tr.pshadowMaps[cmd->map] != NULL)
			{
				GL_Bind(tr.pshadowMaps[cmd->map]);
				qglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, backEnd.refdef.x, glConfig.vidHeight - ( backEnd.refdef.y + PSHADOW_MAP_SIZE ), PSHADOW_MAP_SIZE, PSHADOW_MAP_SIZE, 0);
			}
		}
	}

	return (const void *)(cmd + 1);
}


/*
=============
RB_PostProcess

=============
*/
const void *RB_PostProcess(const void *data)
{
	const postProcessCommand_t *cmd = (const postProcessCommand_t *)data;
	FBO_t *srcFbo;
	vec4i_t srcBox, dstBox;
	qboolean autoExposure;

	// finish any 2D drawing if needed
	if(tess.numIndexes)
		RB_EndSurface();

	if (!r_postProcess->integer || (tr.viewParms.flags & VPF_NOPOSTPROCESS))
	{
		// do nothing
		return (const void *)(cmd + 1);
	}

	if (cmd)
	{
		backEnd.refdef = cmd->refdef;
		backEnd.viewParms = cmd->viewParms;
	}

	srcFbo = tr.renderFbo;
	if (tr.msaaResolveFbo)
	{
		// Resolve the MSAA before anything else
		// Can't resolve just part of the MSAA FBO, so multiple views will suffer a performance hit here
		FBO_FastBlit(tr.renderFbo, NULL, tr.msaaResolveFbo, NULL, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		srcFbo = tr.msaaResolveFbo;

		if ( r_dynamicGlow->integer )
		{
			FBO_FastBlitIndexed(tr.renderFbo, tr.msaaResolveFbo, 1, 1, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		}
	}

	dstBox[0] = backEnd.viewParms.viewportX;
	dstBox[1] = backEnd.viewParms.viewportY;
	dstBox[2] = backEnd.viewParms.viewportWidth;
	dstBox[3] = backEnd.viewParms.viewportHeight;

	if (r_ssao->integer)
	{
		srcBox[0] = backEnd.viewParms.viewportX      * tr.screenSsaoImage->width  / (float)glConfig.vidWidth;
		srcBox[1] = backEnd.viewParms.viewportY      * tr.screenSsaoImage->height / (float)glConfig.vidHeight;
		srcBox[2] = backEnd.viewParms.viewportWidth  * tr.screenSsaoImage->width  / (float)glConfig.vidWidth;
		srcBox[3] = backEnd.viewParms.viewportHeight * tr.screenSsaoImage->height / (float)glConfig.vidHeight;

		//FBO_BlitFromTexture(tr.screenSsaoImage, srcBox, NULL, srcFbo, dstBox, NULL, NULL, GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO);
		srcBox[1] = tr.screenSsaoImage->height - srcBox[1];
		srcBox[3] = -srcBox[3];

		FBO_Blit(tr.screenSsaoFbo, srcBox, NULL, srcFbo, dstBox, NULL, NULL, GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO);
	}

	if (r_dynamicGlow->integer)
	{
		// Downscale 8x
		FBO_BlitFromTexture (tr.glowImage, NULL, NULL, tr.glowFboScaled[0], NULL, &tr.textureColorShader, NULL, GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO);
		FBO_FastBlit (tr.glowFboScaled[0], NULL, tr.glowFboScaled[1], NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		FBO_FastBlit (tr.glowFboScaled[1], NULL, tr.glowFboScaled[2], NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR);

		// Blur a few times
		float spread = 1.0f;
		for ( int i = 0, numPasses = r_dynamicGlowPasses->integer; i < numPasses; i++ )
		{
			RB_GaussianBlur (tr.glowFboScaled[2], tr.glowFboScaled[3], tr.glowFboScaled[2], spread);

			spread += r_dynamicGlowDelta->value * 0.25f;
		}

		// Upscale 4x
		qglScissor (0, 0, tr.glowFboScaled[1]->width, tr.glowFboScaled[1]->height);
		FBO_FastBlit (tr.glowFboScaled[2], NULL, tr.glowFboScaled[1], NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR);

		qglScissor (0, 0, tr.glowFboScaled[0]->width, tr.glowFboScaled[0]->height);
		FBO_FastBlit (tr.glowFboScaled[1], NULL, tr.glowFboScaled[0], NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR);

		// Restore scissor rect
		qglScissor (0, 0, glConfig.vidWidth, glConfig.vidHeight);
	}
	srcBox[0] = backEnd.viewParms.viewportX;
	srcBox[1] = backEnd.viewParms.viewportY;
	srcBox[2] = backEnd.viewParms.viewportWidth;
	srcBox[3] = backEnd.viewParms.viewportHeight;

	if (srcFbo)
	{
		if (r_hdr->integer && (r_toneMap->integer || r_forceToneMap->integer))
		{
			autoExposure = (qboolean)(r_autoExposure->integer || r_forceAutoExposure->integer);
			RB_ToneMap(srcFbo, srcBox, NULL, dstBox, autoExposure);
		}
		else if (r_cameraExposure->value == 0.0f)
		{
			FBO_FastBlit(srcFbo, srcBox, NULL, dstBox, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		}
		else
		{
			vec4_t color;

			color[0] =
			color[1] =
			color[2] = pow(2, r_cameraExposure->value); //exp2(r_cameraExposure->value);
			color[3] = 1.0f;

			FBO_Blit(srcFbo, srcBox, NULL, NULL, dstBox, NULL, color, 0);
		}
	}

	if (r_drawSunRays->integer)
		RB_SunRays(NULL, srcBox, NULL, dstBox);

	if (1)
		RB_BokehBlur(NULL, srcBox, NULL, dstBox, backEnd.refdef.blurFactor);

	if (0 && r_sunlightMode->integer)
	{
		vec4i_t dstBox;
		VectorSet4(dstBox, 0, 0, 128, 128);
		FBO_BlitFromTexture(tr.sunShadowDepthImage[0], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
		VectorSet4(dstBox, 128, 0, 128, 128);
		FBO_BlitFromTexture(tr.sunShadowDepthImage[1], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
		VectorSet4(dstBox, 256, 0, 128, 128);
		FBO_BlitFromTexture(tr.sunShadowDepthImage[2], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
	}

	if (0)
	{
		vec4i_t dstBox;
		VectorSet4(dstBox, 256, glConfig.vidHeight - 256, 256, 256);
		FBO_BlitFromTexture(tr.renderDepthImage, NULL, NULL, NULL, dstBox, NULL, NULL, 0);
		VectorSet4(dstBox, 512, glConfig.vidHeight - 256, 256, 256);
		FBO_BlitFromTexture(tr.screenShadowImage, NULL, NULL, NULL, dstBox, NULL, NULL, 0);
	}

	if (0)
	{
		vec4i_t dstBox;
		VectorSet4(dstBox, 256, glConfig.vidHeight - 256, 256, 256);
		FBO_BlitFromTexture(tr.sunRaysImage, NULL, NULL, NULL, dstBox, NULL, NULL, 0);
	}

#if 0
	if (r_cubeMapping->integer && tr.numCubemaps)
	{
		vec4i_t dstBox;
		int cubemapIndex = R_CubemapForPoint( backEnd.viewParms.ori.origin );

		if (cubemapIndex)
		{
			VectorSet4(dstBox, 0, glConfig.vidHeight - 256, 256, 256);
			//FBO_BlitFromTexture(tr.renderCubeImage, NULL, NULL, NULL, dstBox, &tr.testcubeShader, NULL, 0);
			FBO_BlitFromTexture(tr.cubemaps[cubemapIndex - 1], NULL, NULL, NULL, dstBox, &tr.testcubeShader, NULL, 0);
		}
	}
#endif

	if (r_dynamicGlow->integer != 0)
	{
		// Composite the glow/bloom texture
		int blendFunc = 0;
		vec4_t color = { 1.0f, 1.0f, 1.0f, 1.0f };

		if ( r_dynamicGlow->integer == 2 )
		{
			// Debug output
			blendFunc = GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO;
		}
		else if ( r_dynamicGlowSoft->integer )
		{
			blendFunc = GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE_MINUS_SRC_COLOR;
			color[0] = color[1] = color[2] = r_dynamicGlowIntensity->value;
		}
		else
		{
			blendFunc = GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE;
			color[0] = color[1] = color[2] = r_dynamicGlowIntensity->value;
		}

		FBO_BlitFromTexture (tr.glowFboScaled[0]->colorImage[0], NULL, NULL, NULL, NULL, NULL, color, blendFunc);
	}

	backEnd.framePostProcessed = qtrue;

	return (const void *)(cmd + 1);
}


/*
====================
RB_ExecuteRenderCommands
====================
*/
void RB_ExecuteRenderCommands( const void *data ) {
	int		t1, t2;

	t1 = ri->Milliseconds ();

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
		case RC_DRAW_SURFS:
			data = RB_DrawSurfs( data );
			break;
		case RC_DRAW_BUFFER:
			data = RB_DrawBuffer( data );
			break;
		case RC_SWAP_BUFFERS:
			data = RB_SwapBuffers( data );
			break;
		case RC_SCREENSHOT:
			data = RB_TakeScreenshotCmd( data );
			break;
		case RC_VIDEOFRAME:
			data = RB_TakeVideoFrameCmd( data );
			break;
		case RC_COLORMASK:
			data = RB_ColorMask(data);
			break;
		case RC_CLEARDEPTH:
			data = RB_ClearDepth(data);
			break;
		case RC_CAPSHADOWMAP:
			data = RB_CaptureShadowMap(data);
			break;
		case RC_POSTPROCESS:
			data = RB_PostProcess(data);
			break;
		case RC_END_OF_LIST:
		default:
			// finish any 2D drawing if needed
			if(tess.numIndexes)
				RB_EndSurface();

			// stop rendering
			t2 = ri->Milliseconds ();
			backEnd.pc.msec = t2 - t1;
			return;
		}
	}

}