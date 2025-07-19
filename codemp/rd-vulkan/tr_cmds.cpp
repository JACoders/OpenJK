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

/*
=====================
R_PerformanceCounters
=====================
*/
void R_PerformanceCounters( void ) {
	if ( !r_speeds->integer ) {
		// clear the counters even if we aren't printing
		memset( &tr.pc, 0, sizeof( tr.pc ) );
		memset( &backEnd.pc, 0, sizeof( backEnd.pc ) );
		return;
	}

	if (r_speeds->integer == 1) {
		const float texSize = R_SumOfUsedImages( qfalse )/(8*1048576.0f)*(r_texturebits->integer?r_texturebits->integer:glConfig.colorBits);
		ri.Printf( PRINT_ALL,  "%i/%i shdrs/srfs %i leafs %i vrts %i/%i tris %.2fMB tex\n",
			backEnd.pc.c_shaders, backEnd.pc.c_surfaces, tr.pc.c_leafs, backEnd.pc.c_vertexes,
			backEnd.pc.c_indexes/3, backEnd.pc.c_totalIndexes/3,
			texSize );
	} else if (r_speeds->integer == 2) {
		ri.Printf( PRINT_ALL,  "(patch) %i sin %i sclip  %i sout %i bin %i bclip %i bout\n",
			tr.pc.c_sphere_cull_patch_in, tr.pc.c_sphere_cull_patch_clip, tr.pc.c_sphere_cull_patch_out,
			tr.pc.c_box_cull_patch_in, tr.pc.c_box_cull_patch_clip, tr.pc.c_box_cull_patch_out );
		ri.Printf( PRINT_ALL,  "(md3) %i sin %i sclip  %i sout %i bin %i bclip %i bout\n",
			tr.pc.c_sphere_cull_md3_in, tr.pc.c_sphere_cull_md3_clip, tr.pc.c_sphere_cull_md3_out,
			tr.pc.c_box_cull_md3_in, tr.pc.c_box_cull_md3_clip, tr.pc.c_box_cull_md3_out );
	} else if (r_speeds->integer == 3) {
		ri.Printf( PRINT_ALL,  "viewcluster: %i\n", tr.viewCluster );
	} else if (r_speeds->integer == 4) {
		if ( backEnd.pc.c_dlightVertexes ) {
			ri.Printf( PRINT_ALL,  "dlight srf:%i  culled:%i  verts:%i  tris:%i\n",
				tr.pc.c_dlightSurfaces, tr.pc.c_dlightSurfacesCulled,
				backEnd.pc.c_dlightVertexes, backEnd.pc.c_dlightIndexes / 3 );
		}
	}
	else if (r_speeds->integer == 5 )
	{
		vk_debug("zFar: %.0f\n", tr.viewParms.zFar );
	}
	else if (r_speeds->integer == 6 )
	{
		vk_debug("flare adds:%i tests:%i renders:%i\n",
			backEnd.pc.c_flareAdds, backEnd.pc.c_flareTests, backEnd.pc.c_flareRenders );
	}
	else if (r_speeds->integer == 7) {
		const float texSize = R_SumOfUsedImages(qtrue) / (1048576.0f);
		const float backBuff= glConfig.vidWidth * glConfig.vidHeight * glConfig.colorBits / (8.0f * 1024*1024);
		const float depthBuff= glConfig.vidWidth * glConfig.vidHeight * glConfig.depthBits / (8.0f * 1024*1024);
		const float stencilBuff= glConfig.vidWidth * glConfig.vidHeight * glConfig.stencilBits / (8.0f * 1024*1024);
		ri.Printf( PRINT_ALL,  "Tex MB %.2f + buffers %.2f MB = Total %.2fMB\n",
			texSize, backBuff*2+depthBuff+stencilBuff, texSize+backBuff*2+depthBuff+stencilBuff);
	}

	memset( &tr.pc, 0, sizeof( tr.pc ) );
	memset( &backEnd.pc, 0, sizeof( backEnd.pc ) );
}

/*
====================
R_IssueRenderCommands
====================
*/
void R_IssueRenderCommands( qboolean runPerformanceCounters ) {
	renderCommandList_t	*cmdList;

	cmdList = &backEndData->commands;
	assert(cmdList);
	// add an end-of-list command
	byteAlias_t *ba = (byteAlias_t *)&cmdList->cmds[cmdList->used];
	ba->ui = RC_END_OF_LIST;


	// clear it out, in case this is a sync and not a buffer flip
	cmdList->used = 0;

	if (backEnd.screenshotMask == 0) {
		if (ri.VK_IsMinimized())
			return; // skip backend when minimized
		//if (backEnd.throttle)
		//	return; // or throttled on demand
	}
	else {
		if (ri.VK_IsMinimized() && !R_CanMinimize()) {
			backEnd.screenshotMask = 0;
			return;
		}
	}

	// actually start the commands going
	if ( !r_skipBackEnd->integer ) {
		// let it start on the new batch
		RB_ExecuteRenderCommands( cmdList->cmds );
	}

	// at this point, the back end thread is idle, so it is ok
	// to look at it's performance counters
	if ( runPerformanceCounters ) {
		R_PerformanceCounters();
	}
}

/*
============
R_GetCommandBufferReserved

make sure there is enough command space
============
*/
static void* R_GetCommandBufferReserved( int bytes, int reservedBytes ) {
	renderCommandList_t* cmdList;

	cmdList = &backEndData->commands;
	bytes = PAD(bytes, sizeof(void*));

	// always leave room for the end of list command
	if (cmdList->used + bytes + sizeof(int) + reservedBytes > MAX_RENDER_COMMANDS) {
		if (bytes > MAX_RENDER_COMMANDS - sizeof(int)) {
			ri.Error(ERR_FATAL, "R_GetCommandBuffer: bad size %i", bytes);
		}
		// if we run out of room, just start dropping commands
		return NULL;
	}

	cmdList->used += bytes;

	return cmdList->cmds + cmdList->used - bytes;

}
/*
============
R_GetCommandBuffer

make sure there is enough command space
============
*/
void *R_GetCommandBuffer( int bytes ) {
	tr.lastRenderCommand = RC_END_OF_LIST;

	return R_GetCommandBufferReserved(bytes, PAD(sizeof(swapBuffersCommand_t), sizeof(void*)));
}

/*
=============
R_AddDrawSurfCmd

=============
*/
void	R_AddDrawSurfCmd( drawSurf_t *drawSurfs, int numDrawSurfs ) {
	drawSurfsCommand_t	*cmd;

	cmd = (drawSurfsCommand_t *) R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_DRAW_SURFS;

	cmd->drawSurfs = drawSurfs;
	cmd->numDrawSurfs = numDrawSurfs;

	cmd->refdef = tr.refdef;
	cmd->viewParms = tr.viewParms;

	tr.numDrawSurfCmds++;
	if (tr.drawSurfCmd == NULL) {
		tr.drawSurfCmd = cmd;
	}
}

/*
=============
RE_SetColor

Passing NULL will set the color to white
=============
*/
void RE_SetColor( const float *rgba ) {
	setColorCommand_t	*cmd;

	if ( !tr.registered ) {
		return;
	}
	cmd = (setColorCommand_t *) R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_SET_COLOR;
	if ( !rgba ) {
		static float colorWhite[4] = { 1, 1, 1, 1 };

		rgba = colorWhite;
	}

	cmd->color[0] = rgba[0];
	cmd->color[1] = rgba[1];
	cmd->color[2] = rgba[2];
	cmd->color[3] = rgba[3];
}

/*
=============
RE_StretchPic
=============
*/

/*static Matrix4f orthographic(float left, float right, float bottom, float top, float near, float far) {
	Matrix4f ortho = new Matrix4f();

	float tx = -(right + left) / (right - left);
	float ty = -(top + bottom) / (top - bottom);
	float tz = -(far + near) / (far - near);

	ortho.m00 = 2f / (right - left);
	ortho.m11 = 2f / (top - bottom);
	ortho.m22 = -2f / (far - near);
	ortho.m03 = tx;
	ortho.m13 = ty;
	ortho.m23 = tz;

	return ortho;
}*/

void RE_StretchPic ( float x, float y, float w, float h,
					  float s1, float t1, float s2, float t2, qhandle_t hShader ) {
	stretchPicCommand_t	*cmd;

	cmd = (stretchPicCommand_t *) R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}

	cmd->commandId = RC_STRETCH_PIC;
	cmd->shader = R_GetShaderByHandle( hShader );
	cmd->x = x;
	cmd->y = y;
	cmd->w = w;
	cmd->h = h;
	cmd->s1 = s1;
	cmd->t1 = t1;
	cmd->s2 = s2;
	cmd->t2 = t2;
}

/*
=============
RE_RotatePic
=============
*/
void RE_RotatePic ( float x, float y, float w, float h,
					  float s1, float t1, float s2, float t2,float a, qhandle_t hShader ) {
	rotatePicCommand_t	*cmd;

	cmd = (rotatePicCommand_t *) R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}

	cmd->commandId = RC_ROTATE_PIC;
	cmd->shader = R_GetShaderByHandle( hShader );
	cmd->x = x;
	cmd->y = y;
	cmd->w = w;
	cmd->h = h;
	cmd->s1 = s1;
	cmd->t1 = t1;
	cmd->s2 = s2;
	cmd->t2 = t2;
	cmd->a = a;
}

/*
=============
RE_RotatePic2
=============
*/
void RE_RotatePic2 ( float x, float y, float w, float h,
					  float s1, float t1, float s2, float t2,float a, qhandle_t hShader ) {
	rotatePicCommand_t	*cmd;

	cmd = (rotatePicCommand_t *) R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}

	cmd->commandId = RC_ROTATE_PIC2;
	cmd->shader = R_GetShaderByHandle( hShader );
	cmd->x = x;
	cmd->y = y;
	cmd->w = w;
	cmd->h = h;
	cmd->s1 = s1;
	cmd->t1 = t1;
	cmd->s2 = s2;
	cmd->t2 = t2;
	cmd->a = a;
}

void RE_RenderWorldEffects( void )
{
	drawBufferCommand_t	*cmd;

	cmd = (drawBufferCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_WORLD_EFFECTS;
}

void RE_RenderAutoMap( void )
{
	drawBufferCommand_t	*cmd;

	cmd = (drawBufferCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd )
	{
		return;
	}
	cmd->commandId = RC_AUTO_MAP;
}

/*
====================
RE_BeginFrame

If running in stereo, RE_BeginFrame will be called twice
for each RE_EndFrame
====================
*/
void RE_BeginFrame( stereoFrame_t stereoFrame ) {
	drawBufferCommand_t	*cmd;

	if ( !tr.registered ) {
		return;
	}
	
	glState.finishCalled = qfalse;

	ResetGhoul2RenderableSurfaceHeap();

	backEnd.doneBloom = qfalse;

	tr.frameCount++;
	tr.frameSceneNum = 0;

	//
	// texturemode stuff
	//
	if ( r_textureMode->modified ) {
		vk_texture_mode( r_textureMode->string, qfalse );
		r_textureMode->modified = qfalse;
		vk_update_post_process_pipelines();
	}

	//
	// gamma stuff
	//
	if ( r_gamma->modified || r_greyscale->modified || r_dither->modified ) {
		r_gamma->modified = qfalse;
		r_greyscale->modified = qfalse;
		r_dither->modified = qfalse;

		R_SetColorMappings();
		vk_update_post_process_pipelines();
	}

	if ( cl_ratioFix->modified ) {
		R_Set2DRatio();
		cl_ratioFix->modified = qfalse;
	}

#ifndef USE_BUFFER_CLEAR
	if ( r_fastsky->modified && vk.clearAttachment ) {
#else
	if ( r_fastsky->modified ) {
#endif
		vk_set_clearcolor();
		r_fastsky->modified = qfalse;
	}

	//
	// draw buffer stuff
	//
	cmd = (drawBufferCommand_t *) R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_DRAW_BUFFER;
	tr.lastRenderCommand = RC_DRAW_BUFFER;

	cmd->buffer = 0;

#ifndef USE_BUFFER_CLEAR
	if ( vk.clearAttachment && ( r_fastsky->integer || ( tr.world && tr.world->globalFog != -1 ) ) ) {
		clearColorCommand_t *clrcmd;
		if ( ( clrcmd = (clearColorCommand_t*)R_GetCommandBuffer( sizeof( *clrcmd ) ) ) == nullptr )
			return;
		clrcmd->commandId = RC_CLEARCOLOR;
	}
#endif // USE_BUFFER_CLEAR
}

/*
=============
RE_EndFrame

Returns the number of msec spent in the back end
=============
*/
void RE_EndFrame( int *frontEndMsec, int *backEndMsec ) {
	swapBuffersCommand_t	*cmd;

	if ( !tr.registered ) {
		return;
	}
	cmd = (swapBuffersCommand_t *) R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_SWAP_BUFFERS;

	R_IssueRenderCommands( qtrue );

	// use the other buffers next frame, because another CPU
	// may still be rendering into the current ones
	R_InitNextFrame();

	if ( frontEndMsec ) {
		*frontEndMsec = tr.frontEndMsec;
	}
	tr.frontEndMsec = 0;
	if ( backEndMsec ) {
		*backEndMsec = backEnd.pc.msec;
	}
	backEnd.pc.msec = 0;
}

/*
=============
RE_TakeVideoFrame
=============
*/
void RE_TakeVideoFrame( int width, int height, byte *captureBuffer, byte *encodeBuffer, qboolean motionJpeg )
{
	videoFrameCommand_t *cmd;

	if ( !tr.registered )
		return;
#if 0
	cmd = (videoFrameCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd )
		return;

	cmd->commandId = RC_VIDEOFRAME;
#endif

	backEnd.screenshotMask |= SCREENSHOT_AVI;
	cmd = &backEnd.vcmd;

	cmd->width = width;
	cmd->height = height;
	cmd->captureBuffer = captureBuffer;
	cmd->encodeBuffer = encodeBuffer;
	cmd->motionJpeg = motionJpeg;
}

qboolean R_CanMinimize( void )
{
	if (vk.fboActive || vk.offscreenRender)
		return qtrue;

	return qfalse;
}