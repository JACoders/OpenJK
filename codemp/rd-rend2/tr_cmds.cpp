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
#include "tr_allocator.h"

/*
=====================
R_PerformanceCounters
=====================
*/
void R_PerformanceCounters( void ) {
	gpuFrame_t *currentFrame = backEndData->frames + (backEndData->realFrameNumber % MAX_FRAMES);

	if ( !r_speeds->integer ) {
		// clear the counters even if we aren't printing
		Com_Memset( &tr.pc, 0, sizeof( tr.pc ) );
		Com_Memset( &backEnd.pc, 0, sizeof( backEnd.pc ) );
		currentFrame->numTimedBlocks = 0;
		currentFrame->numTimers = 0;
		return;
	}

	if (r_speeds->integer == 1) {
		ri.Printf (PRINT_ALL, "%i/%i/%i shaders/batches/surfs %i leafs %i verts %i/%i tris %.2f mtex %.2f dc\n",
			backEnd.pc.c_shaders, backEnd.pc.c_surfBatches, backEnd.pc.c_surfaces, tr.pc.c_leafs, backEnd.pc.c_vertexes,
			backEnd.pc.c_indexes/3, backEnd.pc.c_totalIndexes/3,
			R_SumOfUsedImages()/(1000000.0f), backEnd.pc.c_overDraw / (float)(glConfig.vidWidth * glConfig.vidHeight) );
	} else if (r_speeds->integer == 2) {
		ri.Printf (PRINT_ALL, "(patch) %i sin %i sclip  %i sout %i bin %i bclip %i bout\n",
			tr.pc.c_sphere_cull_patch_in, tr.pc.c_sphere_cull_patch_clip, tr.pc.c_sphere_cull_patch_out,
			tr.pc.c_box_cull_patch_in, tr.pc.c_box_cull_patch_clip, tr.pc.c_box_cull_patch_out );
		ri.Printf (PRINT_ALL, "(md3) %i sin %i sclip  %i sout %i bin %i bclip %i bout\n",
			tr.pc.c_sphere_cull_md3_in, tr.pc.c_sphere_cull_md3_clip, tr.pc.c_sphere_cull_md3_out,
			tr.pc.c_box_cull_md3_in, tr.pc.c_box_cull_md3_clip, tr.pc.c_box_cull_md3_out );
	} else if (r_speeds->integer == 3) {
		ri.Printf (PRINT_ALL, "viewcluster: %i\n", tr.viewCluster );
	} else if (r_speeds->integer == 4) {
		if ( backEnd.pc.c_dlightVertexes ) {
			ri.Printf (PRINT_ALL, "dlight srf:%i  culled:%i  verts:%i  tris:%i\n",
				tr.pc.c_dlightSurfaces, tr.pc.c_dlightSurfacesCulled,
				backEnd.pc.c_dlightVertexes, backEnd.pc.c_dlightIndexes / 3 );
		}
	}
	else if (r_speeds->integer == 5 )
	{
		ri.Printf( PRINT_ALL, "zFar: %.0f\n", tr.viewParms.zFar );
	}
	else if (r_speeds->integer == 6 )
	{
		ri.Printf( PRINT_ALL, "flare adds:%i tests:%i renders:%i\n",
			backEnd.pc.c_flareAdds, backEnd.pc.c_flareTests, backEnd.pc.c_flareRenders );
	}
	else if (r_speeds->integer == 7 )
	{
		ri.Printf( PRINT_ALL, "VBO draws: static %i dynamic %i (%.2fKB)\nMultidraws: %i merged %i\n",
			backEnd.pc.c_staticVboDraws, backEnd.pc.c_dynamicVboDraws, backEnd.pc.c_dynamicVboTotalSize / (1024.0f),
			backEnd.pc.c_multidraws, backEnd.pc.c_multidrawsMerged );
		ri.Printf( PRINT_ALL, "GLSL binds: %i  draws: gen %i light %i fog %i dlight %i\n",
			backEnd.pc.c_glslShaderBinds, backEnd.pc.c_genericDraws, backEnd.pc.c_lightallDraws, backEnd.pc.c_fogDraws, backEnd.pc.c_dlightDraws);
	}
	else if (r_speeds->integer == 8)
	{
		ri.Printf( PRINT_ALL, "0-19: %d 20-49: %d 50-99: %d 100-299: %d\n",
			backEnd.pc.c_triangleCountBins[TRI_BIN_0_19],
			backEnd.pc.c_triangleCountBins[TRI_BIN_20_49],
			backEnd.pc.c_triangleCountBins[TRI_BIN_50_99],
			backEnd.pc.c_triangleCountBins[TRI_BIN_100_299]);

		ri.Printf( PRINT_ALL, "300-599: %d 600-999: %d 1000-1499: %d 1500-1999: %d\n",
			backEnd.pc.c_triangleCountBins[TRI_BIN_300_599],
			backEnd.pc.c_triangleCountBins[TRI_BIN_600_999],
			backEnd.pc.c_triangleCountBins[TRI_BIN_1000_1499],
			backEnd.pc.c_triangleCountBins[TRI_BIN_1500_1999]);
		ri.Printf( PRINT_ALL, "2000-2999: %d 3000+: %d\n",
			backEnd.pc.c_triangleCountBins[TRI_BIN_2000_2999],
			backEnd.pc.c_triangleCountBins[TRI_BIN_3000_PLUS]);
	}
	else if ( r_speeds->integer == 100 )
	{
		gpuFrame_t *frame = backEndData->frames + (backEndData->realFrameNumber % MAX_FRAMES);

		// TODO: We want to draw this as text on the screen...
		// Print to console for now

		int numTimedBlocks = frame->numTimedBlocks;
		for ( int i = 0; i < numTimedBlocks; i++ )
		{
			gpuTimedBlock_t *timedBlock = frame->timedBlocks + i;
			GLuint64 startTime, endTime, diffInNs;
			float diffInMs;

			qglGetQueryObjectui64v( timedBlock->beginTimer, GL_QUERY_RESULT, &startTime);
			qglGetQueryObjectui64v( timedBlock->endTimer, GL_QUERY_RESULT, &endTime);

			diffInNs = endTime - startTime;
			diffInMs = diffInNs / 1e6f;

			ri.Printf( PRINT_ALL, "%s: %.3fms ", timedBlock->name, diffInMs );

			if ( (i % 7) == 6 )
			{
				ri.Printf( PRINT_ALL, "\n" );
			}
		}

		ri.Printf( PRINT_ALL, "\n" );
	}

	Com_Memset( &tr.pc, 0, sizeof( tr.pc ) );
	Com_Memset( &backEnd.pc, 0, sizeof( backEnd.pc ) );
	currentFrame->numTimedBlocks = 0;
	currentFrame->numTimers = 0;
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
	*(int *)(cmdList->cmds + cmdList->used) = RC_END_OF_LIST;

	// clear it out, in case this is a sync and not a buffer flip
	cmdList->used = 0;

	if ( runPerformanceCounters ) {
		R_PerformanceCounters();
	}

	// actually start the commands going
	if ( !r_skipBackEnd->integer ) {
		// let it start on the new batch
		RB_ExecuteRenderCommands( cmdList->cmds );
	}
}


/*
====================
R_IssuePendingRenderCommands

Issue any pending commands and wait for them to complete.
====================
*/
void R_IssuePendingRenderCommands( void ) {
	if ( !tr.registered ) {
		return;
	}
	R_IssueRenderCommands( qfalse );
}

/*
============
R_GetCommandBuffer

make sure there is enough command space
============
*/
static void *R_GetCommandBufferReserved( int bytes, int reservedBytes ) {
	renderCommandList_t	*cmdList;

	cmdList = &backEndData->commands;
	bytes = PAD(bytes, sizeof(void *));

	// always leave room for the end of list command
	if ( cmdList->used + bytes + sizeof(int) + reservedBytes > MAX_RENDER_COMMANDS ) {
		if ( bytes > MAX_RENDER_COMMANDS - (int)sizeof(int)) {
			ri.Error( ERR_FATAL, "R_GetCommandBuffer: bad size %i", bytes );
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
void *R_GetCommandBuffer(int bytes) {
	return R_GetCommandBufferReserved(bytes, PAD(sizeof(swapBuffersCommand_t), sizeof(void *)));
}


/*
=============
R_AddDrawSurfCmd

=============
*/
void	R_AddDrawSurfCmd( drawSurf_t *drawSurfs, int numDrawSurfs ) {
	drawSurfsCommand_t	*cmd;

	if (!tr.registered) {
		return;
	}
	cmd = (drawSurfsCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_DRAW_SURFS;

	cmd->drawSurfs = drawSurfs;
	cmd->numDrawSurfs = numDrawSurfs;

	cmd->refdef = tr.refdef;
	cmd->viewParms = tr.viewParms;
}

/*
=============
R_AddConvolveCubemapsCmd

=============
*/
void	R_AddConvolveCubemapCmd( cubemap_t *cubemap , int cubemapId ) {
	convolveCubemapCommand_t	*cmd;

	if (!tr.registered) {
		return;
	}
	cmd = (convolveCubemapCommand_t *)R_GetCommandBuffer( sizeof( *cmd ));
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_CONVOLVECUBEMAP;

	cmd->cubemap = cubemap;
	cmd->cubemapId = cubemapId;
}

/*
=============
R_PostProcessingCmd

=============
*/
void	R_AddPostProcessCmd( ) {
	postProcessCommand_t	*cmd;

	if (!tr.registered) {
		return;
	}
	cmd = (postProcessCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_POSTPROCESS;

	cmd->refdef = tr.refdef;
	cmd->viewParms = tr.viewParms;
}

qhandle_t R_BeginTimedBlockCmd( const char *name )
{
	beginTimedBlockCommand_t *cmd;

	if (!tr.registered) {
		return (qhandle_t)-1;
	}
	cmd = (beginTimedBlockCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd )
	{
		return (qhandle_t)-1;
	}

	if ( tr.numTimedBlocks >= (MAX_GPU_TIMERS / 2) )
	{
		return (qhandle_t)-1;
	}

	cmd->commandId = RC_BEGIN_TIMED_BLOCK;
	cmd->name = name;
	cmd->timerHandle = tr.numTimedBlocks++;

	return (qhandle_t)cmd->timerHandle;
}

void R_EndTimedBlockCmd( qhandle_t timerHandle )
{
	endTimedBlockCommand_t *cmd;

	if (!tr.registered) {
		return;
	}
	cmd = (endTimedBlockCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd )
	{
		return;
	}

	if ( cmd->timerHandle == -1 )
	{
		return;
	}

	cmd->commandId = RC_END_TIMED_BLOCK;
	cmd->timerHandle = timerHandle;
}

/*
=============
RE_SetColor

Passing NULL will set the color to white
=============
*/
void	RE_SetColor( const float *rgba ) {
	setColorCommand_t	*cmd;

	if ( !tr.registered ) {
		return;
	}
	cmd = (setColorCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
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
RE_RotatePic
=============
*/
void RE_RotatePic ( float x, float y, float w, float h,
					  float s1, float t1, float s2, float t2,float a, qhandle_t hShader ) {
	rotatePicCommand_t	*cmd;

	if (!tr.registered) {
		return;
	}
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

	if (!tr.registered) {
		return;
	}
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

/*
=============
RE_StretchPic
=============
*/
void RE_StretchPic ( float x, float y, float w, float h,
					  float s1, float t1, float s2, float t2, qhandle_t hShader ) {
	stretchPicCommand_t	*cmd;

	if ( !tr.registered ) {
		return;
	}
	cmd = (stretchPicCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
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

#define MODE_RED_CYAN	1
#define MODE_RED_BLUE	2
#define MODE_RED_GREEN	3
#define MODE_GREEN_MAGENTA 4
#define MODE_MAX	MODE_GREEN_MAGENTA

void R_SetColorMode(GLboolean *rgba, stereoFrame_t stereoFrame, int colormode)
{
	rgba[0] = rgba[1] = rgba[2] = rgba[3] = GL_TRUE;

	if(colormode > MODE_MAX)
	{
		if(stereoFrame == STEREO_LEFT)
			stereoFrame = STEREO_RIGHT;
		else if(stereoFrame == STEREO_RIGHT)
			stereoFrame = STEREO_LEFT;

		colormode -= MODE_MAX;
	}

	if(colormode == MODE_GREEN_MAGENTA)
	{
		if(stereoFrame == STEREO_LEFT)
			rgba[0] = rgba[2] = GL_FALSE;
		else if(stereoFrame == STEREO_RIGHT)
			rgba[1] = GL_FALSE;
	}
	else
	{
		if(stereoFrame == STEREO_LEFT)
			rgba[1] = rgba[2] = GL_FALSE;
		else if(stereoFrame == STEREO_RIGHT)
		{
			rgba[0] = GL_FALSE;

			if(colormode == MODE_RED_BLUE)
				rgba[1] = GL_FALSE;
			else if(colormode == MODE_RED_GREEN)
				rgba[2] = GL_FALSE;
		}
	}
}


/*
====================
RE_BeginFrame

If running in stereo, RE_BeginFrame will be called twice
for each RE_EndFrame
====================
*/
void RE_BeginFrame( stereoFrame_t stereoFrame ) {
	drawBufferCommand_t	*cmd = NULL;
	colorMaskCommand_t *colcmd = NULL;

	if ( !tr.registered ) {
		return;
	}

	int frameNumber = backEndData->realFrameNumber;
	gpuFrame_t *thisFrame = &backEndData->frames[frameNumber % MAX_FRAMES];
	backEndData->currentFrame = thisFrame;
	if ( thisFrame->sync )
	{
		GLsync sync = thisFrame->sync;
		GLenum result = qglClientWaitSync( sync, 0, 0 );
		if ( result != GL_ALREADY_SIGNALED )
		{
			ri.Printf( PRINT_DEVELOPER, "OpenGL: GPU is more than %d frames behind! Waiting for this frame to finish...\n", MAX_FRAMES );

			static const GLuint64 HALF_SECOND = 500 * 1000 * 1000;
			do
			{
				result = qglClientWaitSync( sync, GL_SYNC_FLUSH_COMMANDS_BIT, HALF_SECOND);
				if ( result == GL_WAIT_FAILED )
				{
					// This indicates that opengl context was lost, is there a way to recover?
					qglDeleteSync( sync );
					thisFrame->sync = NULL;

					thisFrame->uboWriteOffset = 0;

					thisFrame->dynamicIboCommitOffset = 0;
					thisFrame->dynamicIboWriteOffset = 0;

					thisFrame->dynamicVboCommitOffset = 0;
					thisFrame->dynamicVboWriteOffset = 0;

					backEndData->perFrameMemory->Reset();

					ri.Error(ERR_DROP, "OpenGL: Failed to wait for fence. Context lost. (0x%x)\n", qglGetError());
					return;
				}
			}
			while ( result != GL_ALREADY_SIGNALED && result != GL_CONDITION_SATISFIED );
		}
		qglDeleteSync( sync );
		thisFrame->sync = NULL;

		// Perform readback operations
		if (thisFrame->screenshotReadback.pbo > 0)
			R_SaveScreenshot(&thisFrame->screenshotReadback);

		// Resets resources
		qglBindBuffer(GL_UNIFORM_BUFFER, thisFrame->ubo);
		glState.currentGlobalUBO = thisFrame->ubo;
		thisFrame->uboWriteOffset = 0;

		thisFrame->dynamicIboCommitOffset = 0;
		thisFrame->dynamicIboWriteOffset = 0;

		thisFrame->dynamicVboCommitOffset = 0;
		thisFrame->dynamicVboWriteOffset = 0;

		backEndData->perFrameMemory->Reset();
	}

	tr.frameCount++;
	tr.frameSceneNum = 0;

	tr.fogsUboOffset = -1;
	tr.lightsUboOffset = -1;
	tr.sceneUboOffset = -1;

	//
	// do overdraw measurement
	//
	if ( r_measureOverdraw->integer )
	{
		if ( glConfig.stencilBits < 4 )
		{
			ri.Printf( PRINT_ALL, "Warning: not enough stencil bits to measure overdraw: %d\n", glConfig.stencilBits );
			ri.Cvar_Set( "r_measureOverdraw", "0" );
			r_measureOverdraw->modified = qfalse;
		}
		else if ( r_shadows->integer == 2 )
		{
			ri.Printf( PRINT_ALL, "Warning: stencil shadows and overdraw measurement are mutually exclusive\n" );
			ri.Cvar_Set( "r_measureOverdraw", "0" );
			r_measureOverdraw->modified = qfalse;
		}
		else
		{
			R_IssuePendingRenderCommands();
			qglEnable( GL_STENCIL_TEST );
			qglStencilMask( ~0U );
			qglClearStencil( 0U );
			qglStencilFunc( GL_ALWAYS, 0U, ~0U );
			qglStencilOp( GL_KEEP, GL_INCR, GL_INCR );
		}
		r_measureOverdraw->modified = qfalse;
	}
	else
	{
		// this is only reached if it was on and is now off
		if ( r_measureOverdraw->modified ) {
			R_IssuePendingRenderCommands();
			qglDisable( GL_STENCIL_TEST );
		}
		r_measureOverdraw->modified = qfalse;
	}

	//
	// texturemode stuff
	//
	if ( r_textureMode->modified || r_ext_texture_filter_anisotropic->modified ) {
		R_IssuePendingRenderCommands();
		GL_TextureMode( r_textureMode->string );
		r_textureMode->modified = qfalse;
		r_ext_texture_filter_anisotropic->modified = qfalse;
	}

	//
	// gamma stuff
	//
	if ( r_gamma->modified ) {
		r_gamma->modified = qfalse;

		R_IssuePendingRenderCommands();
		R_SetColorMappings();
	}

	// check for errors
	if ( !r_ignoreGLErrors->integer )
	{
		R_IssuePendingRenderCommands();

		GLenum err = qglGetError();
		if ( err != GL_NO_ERROR )
			Com_Error( ERR_FATAL, "RE_BeginFrame() - glGetError() failed (0x%x)!\n", err );
	}

	if (glConfig.stereoEnabled) {
		if( !(cmd = (drawBufferCommand_t *)R_GetCommandBuffer(sizeof(*cmd))) )
			return;

		cmd->commandId = RC_DRAW_BUFFER;

		if ( stereoFrame == STEREO_LEFT ) {
			cmd->buffer = (int)GL_BACK_LEFT;
		} else if ( stereoFrame == STEREO_RIGHT ) {
			cmd->buffer = (int)GL_BACK_RIGHT;
		} else {
			ri.Error( ERR_FATAL, "RE_BeginFrame: Stereo is enabled, but stereoFrame was %i", stereoFrame );
		}
	}
	else
	{
		if(r_anaglyphMode->integer)
		{
			if(r_anaglyphMode->modified)
			{
				// clear both, front and backbuffer.
				qglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
				backEnd.colorMask[0] = qfalse;
				backEnd.colorMask[1] = qfalse;
				backEnd.colorMask[2] = qfalse;
				backEnd.colorMask[3] = qfalse;
				qglClearColor(0.0f, 0.0f, 0.0f, 1.0f);

				// clear all framebuffers
				if (tr.msaaResolveFbo)
				{
					FBO_Bind(tr.msaaResolveFbo);
					qglClear(GL_COLOR_BUFFER_BIT);
				}

				if (tr.renderFbo)
				{
					FBO_Bind(tr.renderFbo);
					qglClear(GL_COLOR_BUFFER_BIT);
				}

				FBO_Bind(NULL);

				qglDrawBuffer(GL_FRONT);
				qglClear(GL_COLOR_BUFFER_BIT);
				qglDrawBuffer(GL_BACK);
				qglClear(GL_COLOR_BUFFER_BIT);

				r_anaglyphMode->modified = qfalse;
			}

			if(stereoFrame == STEREO_LEFT)
			{
				if( !(cmd = (drawBufferCommand_t *)R_GetCommandBuffer(sizeof(*cmd))) )
					return;

				if( !(colcmd = (colorMaskCommand_t *)R_GetCommandBuffer(sizeof(*colcmd))) )
					return;
			}
			else if(stereoFrame == STEREO_RIGHT)
			{
				clearDepthCommand_t *cldcmd;

				if( !(cldcmd = (clearDepthCommand_t *)R_GetCommandBuffer(sizeof(*cldcmd))) )
					return;

				cldcmd->commandId = RC_CLEARDEPTH;

				if( !(colcmd = (colorMaskCommand_t *)R_GetCommandBuffer(sizeof(*colcmd))) )
					return;
			}
			else
				ri.Error( ERR_FATAL, "RE_BeginFrame: Stereo is enabled, but stereoFrame was %i", stereoFrame );

			R_SetColorMode(colcmd->rgba, stereoFrame, r_anaglyphMode->integer);
			colcmd->commandId = RC_COLORMASK;
		}
		else
		{
			if(stereoFrame != STEREO_CENTER)
				ri.Error( ERR_FATAL, "RE_BeginFrame: Stereo is disabled, but stereoFrame was %i", stereoFrame );

			if( !(cmd = (drawBufferCommand_t *)R_GetCommandBuffer(sizeof(*cmd))) )
				return;
		}

		if(cmd)
		{
			cmd->commandId = RC_DRAW_BUFFER;

			if(r_anaglyphMode->modified)
			{
				qglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
				backEnd.colorMask[0] = qfalse;
				backEnd.colorMask[1] = qfalse;
				backEnd.colorMask[2] = qfalse;
				backEnd.colorMask[3] = qfalse;
				r_anaglyphMode->modified = qfalse;
			}

			if (!Q_stricmp(r_drawBuffer->string, "GL_FRONT"))
				cmd->buffer = (int)GL_FRONT;
			else
				cmd->buffer = (int)GL_BACK;
		}
	}

	tr.refdef.stereoFrame = stereoFrame;
}

void R_NewFrameSync()
{
	gpuFrame_t *currentFrame = backEndData->currentFrame;

	assert(!currentFrame->sync);
	currentFrame->sync = qglFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

	backEndData->realFrameNumber++;
	backEnd.framePostProcessed = qfalse;
	backEnd.projection2D = qfalse;
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
	cmd = (swapBuffersCommand_t *)R_GetCommandBufferReserved( sizeof( *cmd ), 0 );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_SWAP_BUFFERS;

	R_IssueRenderCommands( qtrue );

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
void RE_TakeVideoFrame( int width, int height,
		byte *captureBuffer, byte *encodeBuffer, qboolean motionJpeg )
{
	videoFrameCommand_t	*cmd;

	if( !tr.registered ) {
		return;
	}

	cmd = (videoFrameCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	if( !cmd ) {
		return;
	}

	cmd->commandId = RC_VIDEOFRAME;

	cmd->width = width;
	cmd->height = height;
	cmd->captureBuffer = captureBuffer;
	cmd->encodeBuffer = encodeBuffer;
	cmd->motionJpeg = motionJpeg;
}
