/*
===========================================================================
Copyright (C) 2006 Kirk Barnes
Copyright (C) 2006-2008 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of XreaL source code.

XreaL source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

XreaL source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with XreaL source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_fbo.c
#include "tr_local.h"

/*
=============
R_CheckFBO
=============
*/
qboolean R_CheckFBO(const FBO_t * fbo)
{
	int             code;
	int             id;

	qglGetIntegerv(GL_FRAMEBUFFER_BINDING, &id);
	qglBindFramebuffer(GL_FRAMEBUFFER, fbo->frameBuffer);

	code = qglCheckFramebufferStatus(GL_FRAMEBUFFER);

	qglBindFramebuffer(GL_FRAMEBUFFER, id);

	if(code == GL_FRAMEBUFFER_COMPLETE)
	{
		return qtrue;
	}

	// an error occured
	switch (code)
	{
		case GL_FRAMEBUFFER_COMPLETE:
			break;

		case GL_FRAMEBUFFER_UNSUPPORTED:
			ri.Printf(PRINT_WARNING, "R_CheckFBO: (%s) Unsupported framebuffer format\n", fbo->name);
			break;

		case GL_FRAMEBUFFER_UNDEFINED:
			ri.Printf(PRINT_WARNING, "R_CheckFBO: (%s) Default framebuffer was checked, but does not exist\n", fbo->name);
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			ri.Printf(PRINT_WARNING, "R_CheckFBO: (%s) Framebuffer incomplete attachment\n", fbo->name);
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			ri.Printf(PRINT_WARNING, "R_CheckFBO: (%s) Framebuffer incomplete, no attachments attached\n", fbo->name);
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
			ri.Printf(PRINT_WARNING, "R_CheckFBO: (%s) Framebuffer incomplete, mismatched multisampling values\n", fbo->name);
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
			ri.Printf(PRINT_WARNING, "R_CheckFBO: (%s) Framebuffer incomplete, mismatched layer targets\n",
					  fbo->name);
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			ri.Printf(PRINT_WARNING, "R_CheckFBO: (%s) Framebuffer incomplete, missing draw buffer\n", fbo->name);
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			ri.Printf(PRINT_WARNING, "R_CheckFBO: (%s) Framebuffer incomplete, missing read buffer\n", fbo->name);
			break;

		default:
			ri.Printf(PRINT_WARNING, "R_CheckFBO: (%s) unknown error 0x%X\n", fbo->name, code);
			//ri.Error(ERR_FATAL, "R_CheckFBO: (%s) unknown error 0x%X", fbo->name, code);
			//assert(0);
			break;
	}

	return qfalse;
}

/*
============
FBO_Create
============
*/
FBO_t          *FBO_Create(const char *name, int width, int height)
{
	FBO_t          *fbo;

	if(strlen(name) >= MAX_QPATH)
	{
		ri.Error(ERR_DROP, "FBO_Create: \"%s\" is too long", name);
	}

	if(width <= 0 || width > glRefConfig.maxRenderbufferSize)
	{
		ri.Error(ERR_DROP, "FBO_Create: bad width %i", width);
	}

	if(height <= 0 || height > glRefConfig.maxRenderbufferSize)
	{
		ri.Error(ERR_DROP, "FBO_Create: bad height %i", height);
	}

	if(tr.numFBOs == MAX_FBOS)
	{
		ri.Error(ERR_DROP, "FBO_Create: MAX_FBOS hit");
	}

	fbo = tr.fbos[tr.numFBOs] = (FBO_t *)ri.Hunk_Alloc(sizeof(*fbo), h_low);
	Q_strncpyz(fbo->name, name, sizeof(fbo->name));
	fbo->index = tr.numFBOs++;
	fbo->width = width;
	fbo->height = height;

	qglGenFramebuffers(1, &fbo->frameBuffer);

	return fbo;
}

void FBO_CreateBuffer(FBO_t *fbo, int format, int index, int multisample)
{
	uint32_t *pRenderBuffer;
	GLenum attachment;
	qboolean absent;

	switch(format)
	{
		case GL_RGB:
		case GL_RGBA:
		case GL_RGB8:
		case GL_RGBA8:
		case GL_RGB16F:
		case GL_RGBA16F:
		case GL_RGB32F:
		case GL_RGBA32F:
			fbo->colorFormat = format;
			pRenderBuffer = &fbo->colorBuffers[index];
			attachment = GL_COLOR_ATTACHMENT0 + index;
			break;

		case GL_DEPTH_COMPONENT:
		case GL_DEPTH_COMPONENT16:
		case GL_DEPTH_COMPONENT24:
		case GL_DEPTH_COMPONENT32:
			fbo->depthFormat = format;
			pRenderBuffer = &fbo->depthBuffer;
			attachment = GL_DEPTH_ATTACHMENT;
			break;

		case GL_STENCIL_INDEX:
		case GL_STENCIL_INDEX1:
		case GL_STENCIL_INDEX4:
		case GL_STENCIL_INDEX8:
		case GL_STENCIL_INDEX16:
			fbo->stencilFormat = format;
			pRenderBuffer = &fbo->stencilBuffer;
			attachment = GL_STENCIL_ATTACHMENT;
			break;

		case GL_DEPTH_STENCIL:
		case GL_DEPTH24_STENCIL8:
			fbo->packedDepthStencilFormat = format;
			pRenderBuffer = &fbo->packedDepthStencilBuffer;
			attachment = 0; // special for stencil and depth
			break;

		default:
			ri.Printf(PRINT_WARNING, "FBO_CreateBuffer: invalid format %d\n", format);
			return;
	}

	absent = (qboolean)(*pRenderBuffer == 0);
	if (absent)
		qglGenRenderbuffers(1, pRenderBuffer);

	qglBindRenderbuffer(GL_RENDERBUFFER, *pRenderBuffer);
	if (multisample)
	{
		qglRenderbufferStorageMultisample(GL_RENDERBUFFER, multisample, format, fbo->width, fbo->height);
	}
	else
	{
		qglRenderbufferStorage(GL_RENDERBUFFER, format, fbo->width, fbo->height);
	}

	if(absent)
	{
		if (attachment == 0)
		{
			qglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,   GL_RENDERBUFFER, *pRenderBuffer);
			qglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, *pRenderBuffer);
		}
		else
			qglFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, *pRenderBuffer);
	}
}


/*
=================
R_AttachFBOTexture1D
=================
*/
void R_AttachFBOTexture1D(int texId, int index)
{
	if(index < 0 || index >= glRefConfig.maxColorAttachments)
	{
		ri.Printf(PRINT_WARNING, "R_AttachFBOTexture1D: invalid attachment index %i\n", index);
		return;
	}

	qglFramebufferTexture1D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_1D, texId, 0);
}

/*
=================
R_AttachFBOTexture2D
=================
*/
void R_AttachFBOTexture2D(int target, int texId, int index)
{
	if (target != GL_TEXTURE_2D &&
		(target < GL_TEXTURE_CUBE_MAP_POSITIVE_X ||
		 target > GL_TEXTURE_CUBE_MAP_NEGATIVE_Z))
	{
		ri.Printf(PRINT_WARNING, "R_AttachFBOTexture2D: invalid target %i\n", target);
		return;
	}

	if (index < 0 || index >= glRefConfig.maxColorAttachments)
	{
		ri.Printf(PRINT_WARNING, "R_AttachFBOTexture2D: invalid attachment index %i\n", index);
		return;
	}

	qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, target, texId, 0);
}

/*
=================
R_AttachFBOTexture3D
=================
*/
void R_AttachFBOTexture3D(int texId, int index, int zOffset)
{
	if(index < 0 || index >= glRefConfig.maxColorAttachments)
	{
		ri.Printf(PRINT_WARNING, "R_AttachFBOTexture3D: invalid attachment index %i\n", index);
		return;
	}

	qglFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_3D, texId, 0, zOffset);
}

/*
=================
R_AttachFBOTextureDepth
=================
*/
void R_AttachFBOTextureDepth(int texId)
{
	qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texId, 0);
}

/*
=================
R_AttachFBOTexturePackedDepthStencil
=================
*/
void R_AttachFBOTexturePackedDepthStencil(int texId)
{
	qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texId, 0);
}

void FBO_AttachTextureImage(image_t *img, int index)
{
	if (!glState.currentFBO)
	{
		ri.Printf(PRINT_WARNING, "FBO: attempted to attach a texture image with no FBO bound!\n");
		return;
	}

	R_AttachFBOTexture2D(GL_TEXTURE_2D, img->texnum, index);

	glState.currentFBO->colorImage[index] = img;
	glState.currentFBO->colorBuffers[index] = img->texnum;
}

static void FBO_SetupDrawBuffers()
{
	if (!glState.currentFBO)
	{
		ri.Printf(PRINT_WARNING, "FBO: attempted to attach a texture image with no FBO bound!\n");
		return;
	}

	FBO_t *currentFBO = glState.currentFBO;
	int numBuffers = 0;
	GLenum bufs[8];

	while ( currentFBO->colorBuffers[numBuffers] != 0 )
	{
		numBuffers++;
	}

	if ( numBuffers == 0 )
	{
		qglDrawBuffer (GL_NONE);
	}
	else
	{
		for ( int i = 0; i < numBuffers; i++ )
		{
			bufs[i] = GL_COLOR_ATTACHMENT0 + i;
		}

		qglDrawBuffers (numBuffers, bufs);
	}
}

/*
============
FBO_Bind
============
*/
void FBO_Bind(FBO_t * fbo)
{
	if (glState.currentFBO == fbo)
		return;

	if (r_logFile->integer)
	{
		// don't just call LogComment, or we will get a call to va() every frame!
		if (fbo)
			GLimp_LogComment(va("--- FBO_Bind( %s ) ---\n", fbo->name));
		else
			GLimp_LogComment("--- FBO_Bind ( NULL ) ---\n");
	}

	if (!fbo)
	{
		qglBindFramebuffer(GL_FRAMEBUFFER, 0);
		glState.currentFBO = NULL;
	}
	else
	{
		qglBindFramebuffer(GL_FRAMEBUFFER, fbo->frameBuffer);
		glState.currentFBO = fbo;
	}
}

/*
============
FBO_Init
============
*/
void FBO_Init(void)
{
	int             i;
	int             hdrFormat, multisample;

	ri.Printf(PRINT_ALL, "------- FBO_Init -------\n");

	tr.numFBOs = 0;

	GL_CheckErrors();

	R_IssuePendingRenderCommands();

/*	if(glRefConfig.textureNonPowerOfTwo)
	{
		width = glConfig.vidWidth;
		height = glConfig.vidHeight;
	}
	else
	{
		width = NextPowerOfTwo(glConfig.vidWidth);
		height = NextPowerOfTwo(glConfig.vidHeight);
	} */

	hdrFormat = GL_RGBA8;
	if (r_hdr->integer)
	{
		hdrFormat = GL_RGBA16F;
	}

	qglGetIntegerv(GL_MAX_SAMPLES, &multisample);

	if (r_ext_framebuffer_multisample->integer < multisample)
	{
		multisample = r_ext_framebuffer_multisample->integer;
	}

	if (multisample < 2)
		multisample = 0;

	if (multisample != r_ext_framebuffer_multisample->integer)
	{
		ri.Cvar_SetValue("r_ext_framebuffer_multisample", (float)multisample);
	}

	// only create a render FBO if we need to resolve MSAA or do HDR
	// otherwise just render straight to the screen (tr.renderFbo = NULL)
	if (multisample)
	{
		tr.renderFbo = FBO_Create(
			"_render", tr.renderDepthImage->width,
			tr.renderDepthImage->height);

		FBO_Bind(tr.renderFbo);
		FBO_CreateBuffer(tr.renderFbo, hdrFormat, 0, multisample);
		FBO_CreateBuffer(tr.renderFbo, hdrFormat, 1, multisample);
		FBO_CreateBuffer(tr.renderFbo, GL_DEPTH24_STENCIL8, 0, multisample);
		FBO_SetupDrawBuffers();

		R_CheckFBO(tr.renderFbo);

		tr.msaaResolveFbo = FBO_Create(
			"_msaaResolve", tr.renderDepthImage->width,
			tr.renderDepthImage->height);

		FBO_Bind(tr.msaaResolveFbo);
		FBO_AttachTextureImage(tr.renderImage, 0);
		FBO_AttachTextureImage(tr.glowImage, 1);
		R_AttachFBOTexturePackedDepthStencil(tr.renderDepthImage->texnum);
		FBO_SetupDrawBuffers();

		R_CheckFBO(tr.msaaResolveFbo);
	}
	else
	{
		tr.renderFbo = FBO_Create(
			"_render", tr.renderDepthImage->width,
			tr.renderDepthImage->height);

		FBO_Bind(tr.renderFbo);
		FBO_AttachTextureImage(tr.renderImage, 0);
		FBO_AttachTextureImage(tr.glowImage, 1);
		R_AttachFBOTexturePackedDepthStencil(tr.renderDepthImage->texnum);
		FBO_SetupDrawBuffers();

		R_CheckFBO(tr.renderFbo);
	}

	// clear render buffer
	// this fixes the corrupt screen bug with r_hdr 1 on older hardware
	FBO_Bind(tr.renderFbo);
	qglClearColor( 0.f, 0.f, 0.f, 1 );
	qglClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// glow buffers
	{
		for ( int i = 0; i < ARRAY_LEN(tr.glowImageScaled); i++ )
		{
			tr.glowFboScaled[i] = FBO_Create(
				va("*glowScaled%d", i), tr.glowImageScaled[i]->width,
				tr.glowImageScaled[i]->height);

			FBO_Bind (tr.glowFboScaled[i]);
			FBO_AttachTextureImage (tr.glowImageScaled[i], 0);
			FBO_SetupDrawBuffers();

			R_CheckFBO (tr.glowFboScaled[i]);
		}
	}

	if (r_drawSunRays->integer)
	{
		tr.sunRaysFbo = FBO_Create(
			"_sunRays", tr.renderDepthImage->width,
			tr.renderDepthImage->height);

		FBO_Bind(tr.sunRaysFbo);
		FBO_AttachTextureImage(tr.sunRaysImage, 0);
		R_AttachFBOTextureDepth(tr.renderDepthImage->texnum);
		FBO_SetupDrawBuffers();

		R_CheckFBO(tr.sunRaysFbo);
	}

#if MAX_DRAWN_PSHADOWS > 0
	if (tr.pshadowArrayImage != NULL)
	{
		for( i = 0; i < MAX_DRAWN_PSHADOWS; i++)
		{
			tr.pshadowFbos[i] = FBO_Create(
				va("_shadowmap%i", i), tr.pshadowArrayImage->width,
				tr.pshadowArrayImage->height);

			FBO_Bind(tr.pshadowFbos[i]);
			qglFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, tr.pshadowArrayImage->texnum, 0, i);
			qglDrawBuffer(GL_NONE);
			qglReadBuffer(GL_NONE);
			R_CheckFBO(tr.pshadowFbos[i]);
		}

	}
#endif

	if (r_dlightMode->integer >= 2)
	{
		for (i = 0; i < MAX_DLIGHTS * 6; i++)
		{
			tr.shadowCubeFbo[i] = FBO_Create(va("_shadowCubeFbo_%i", i), PSHADOW_MAP_SIZE, PSHADOW_MAP_SIZE);
			FBO_Bind(tr.shadowCubeFbo[i]);
			qglFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, tr.pointShadowArrayImage->texnum, 0, i);
			qglDrawBuffer(GL_NONE);
			qglReadBuffer(GL_NONE);
			R_CheckFBO(tr.shadowCubeFbo[i]);
		}
	}

	if (tr.sunShadowArrayImage != NULL)
	{
		for ( i = 0; i < 3; i++)
		{
			tr.sunShadowFbo[i] = FBO_Create(
				va("_sunshadowmap%i", i),
				tr.sunShadowArrayImage->width,
				tr.sunShadowArrayImage->height);

			FBO_Bind(tr.sunShadowFbo[i]);
			qglFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, tr.sunShadowArrayImage->texnum, 0, i);
			qglDrawBuffer(GL_NONE);
			qglReadBuffer(GL_NONE);

			R_CheckFBO(tr.sunShadowFbo[i]);
		}

		tr.screenShadowFbo = FBO_Create(
			"_screenshadow", tr.screenShadowImage->width,
			tr.screenShadowImage->height);

		FBO_Bind(tr.screenShadowFbo);
		FBO_AttachTextureImage(tr.screenShadowImage, 0);
		FBO_SetupDrawBuffers();

		R_CheckFBO(tr.screenShadowFbo);
	}

	for (i = 0; i < 2; i++)
	{
		tr.textureScratchFbo[i] = FBO_Create(
			va("_texturescratch%d", i), tr.textureScratchImage[i]->width,
			tr.textureScratchImage[i]->height);

		FBO_Bind(tr.textureScratchFbo[i]);
		FBO_AttachTextureImage(tr.textureScratchImage[i], 0);
		FBO_SetupDrawBuffers();

		R_CheckFBO(tr.textureScratchFbo[i]);
	}

	{
		tr.calcLevelsFbo = FBO_Create(
			"_calclevels", tr.calcLevelsImage->width,
			tr.calcLevelsImage->height);

		FBO_Bind(tr.calcLevelsFbo);
		FBO_AttachTextureImage(tr.calcLevelsImage, 0);
		FBO_SetupDrawBuffers();

		R_CheckFBO(tr.calcLevelsFbo);
	}

	{
		tr.targetLevelsFbo = FBO_Create(
			"_targetlevels", tr.targetLevelsImage->width,
			tr.targetLevelsImage->height);

		FBO_Bind(tr.targetLevelsFbo);
		FBO_AttachTextureImage(tr.targetLevelsImage, 0);
		FBO_SetupDrawBuffers();

		R_CheckFBO(tr.targetLevelsFbo);
	}

	for (i = 0; i < 2; i++)
	{
		tr.quarterFbo[i] = FBO_Create(
			va("_quarter%d", i), tr.quarterImage[i]->width,
			tr.quarterImage[i]->height);

		FBO_Bind(tr.quarterFbo[i]);
		FBO_AttachTextureImage(tr.quarterImage[i], 0);
		FBO_SetupDrawBuffers();

		R_CheckFBO(tr.quarterFbo[i]);
	}

	if (r_ssao->integer)
	{
		tr.hdrDepthFbo = FBO_Create(
			"_hdrDepth", tr.hdrDepthImage->width, tr.hdrDepthImage->height);

		FBO_Bind(tr.hdrDepthFbo);
		FBO_AttachTextureImage(tr.hdrDepthImage, 0);
		FBO_SetupDrawBuffers();

		R_CheckFBO(tr.hdrDepthFbo);

		tr.screenSsaoFbo = FBO_Create(
			"_screenssao", tr.screenSsaoImage->width,
			tr.screenSsaoImage->height);

		FBO_Bind(tr.screenSsaoFbo);
		FBO_AttachTextureImage(tr.screenSsaoImage, 0);
		FBO_SetupDrawBuffers();

		R_CheckFBO(tr.screenSsaoFbo);
	}

	if (tr.renderCubeImage != NULL)
	{
		for (i = 0; i < 6; i++)
		{
			tr.renderCubeFbo[i] = FBO_Create(
				"_renderCubeFbo", tr.renderCubeImage->width,
				tr.renderCubeImage->height);
			FBO_Bind(tr.renderCubeFbo[i]);
			R_AttachFBOTexture2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, tr.renderCubeImage->texnum, 0);
			glState.currentFBO->colorImage[0] = tr.renderCubeImage;
			glState.currentFBO->colorBuffers[0] = tr.renderCubeImage->texnum;
			R_AttachFBOTextureDepth(tr.renderCubeDepthImage->texnum);

			FBO_SetupDrawBuffers();
			R_CheckFBO(tr.renderCubeFbo[i]);
		}

		tr.filterCubeFbo = FBO_Create(
			"_filterCubeFbo", tr.renderCubeImage->width,
			tr.renderCubeImage->height);
		FBO_Bind(tr.filterCubeFbo);
		qglFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tr.renderCubeImage->texnum, 0);
		glState.currentFBO->colorImage[0] = tr.renderCubeImage;
		glState.currentFBO->colorBuffers[0] = tr.renderCubeImage->texnum;
		FBO_SetupDrawBuffers();
		R_CheckFBO(tr.filterCubeFbo);
	}

	if (tr.weatherDepthImage != nullptr)
	{
		tr.weatherDepthFbo = FBO_Create(
			"_weatherDepthFbo",
			tr.weatherDepthImage->width,
			tr.weatherDepthImage->height);

		FBO_Bind(tr.weatherDepthFbo);
		R_AttachFBOTextureDepth(tr.weatherDepthImage->texnum);
		FBO_SetupDrawBuffers();

		R_CheckFBO(tr.weatherDepthFbo);
	}

	GL_CheckErrors();

	FBO_Bind(NULL);
}

/*
============
FBO_Shutdown
============
*/
void FBO_Shutdown(void)
{
	int             i, j;
	FBO_t          *fbo;

	ri.Printf(PRINT_ALL, "------- FBO_Shutdown -------\n");

	FBO_Bind(NULL);

	for(i = 0; i < tr.numFBOs; i++)
	{
		fbo = tr.fbos[i];

		for(j = 0; j < glRefConfig.maxColorAttachments; j++)
		{
			if(fbo->colorBuffers[j])
				qglDeleteRenderbuffers(1, &fbo->colorBuffers[j]);
		}

		if(fbo->depthBuffer)
			qglDeleteRenderbuffers(1, &fbo->depthBuffer);

		if(fbo->stencilBuffer)
			qglDeleteRenderbuffers(1, &fbo->stencilBuffer);

		if(fbo->frameBuffer)
			qglDeleteFramebuffers(1, &fbo->frameBuffer);
	}
}

/*
============
R_FBOList_f
============
*/
void R_FBOList_f(void)
{
	int             i;
	FBO_t          *fbo;

	ri.Printf(PRINT_ALL, "             size       name\n");
	ri.Printf(PRINT_ALL, "----------------------------------------------------------\n");

	for(i = 0; i < tr.numFBOs; i++)
	{
		fbo = tr.fbos[i];

		ri.Printf(PRINT_ALL, "  %4i: %4i %4i %s\n", i, fbo->width, fbo->height, fbo->name);
	}

	ri.Printf(PRINT_ALL, " %i FBOs\n", tr.numFBOs);
}

void FBO_BlitFromTexture(struct image_s *src, vec4i_t inSrcBox, vec2_t inSrcTexScale, FBO_t *dst, vec4i_t inDstBox, struct shaderProgram_s *shaderProgram, vec4_t inColor, int blend)
{
	vec4i_t dstBox, srcBox;
	vec2_t srcTexScale;
	vec4_t color;
	vec4_t quadVerts[4];
	vec2_t texCoords[4];
	vec2_t invTexRes;
	FBO_t *oldFbo = glState.currentFBO;
	matrix_t projection;
	int width, height;

	if (!src)
		return;

	if (inSrcBox)
	{
		VectorSet4(srcBox, inSrcBox[0], inSrcBox[1], inSrcBox[0] + inSrcBox[2],  inSrcBox[1] + inSrcBox[3]);
	}
	else
	{
		VectorSet4(srcBox, 0, 0, src->width, src->height);
	}

	// framebuffers are 0 bottom, Y up.
	if (inDstBox)
	{
		if (dst)
		{
			dstBox[0] = inDstBox[0];
			dstBox[1] = dst->height - inDstBox[1] - inDstBox[3];
			dstBox[2] = inDstBox[0] + inDstBox[2];
			dstBox[3] = dst->height - inDstBox[1];
		}
		else
		{
			dstBox[0] = inDstBox[0];
			dstBox[1] = glConfig.vidHeight - inDstBox[1] - inDstBox[3];
			dstBox[2] = inDstBox[0] + inDstBox[2];
			dstBox[3] = glConfig.vidHeight - inDstBox[1];
		}
	}
	else if (dst)
	{
		VectorSet4(dstBox, 0, dst->height, dst->width, 0);
	}
	else
	{
		VectorSet4(dstBox, 0, glConfig.vidHeight, glConfig.vidWidth, 0);
	}

	if (inSrcTexScale)
	{
		VectorCopy2(inSrcTexScale, srcTexScale);
	}
	else
	{
		srcTexScale[0] = srcTexScale[1] = 1.0f;
	}

	if (inColor)
	{
		VectorCopy4(inColor, color);
	}
	else
	{
		VectorCopy4(colorWhite, color);
	}

	if (!shaderProgram)
	{
		shaderProgram = &tr.textureColorShader;
	}

	FBO_Bind(dst);

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

	qglViewport( 0, 0, width, height );
	qglScissor( 0, 0, width, height );

	Matrix16Ortho(0, width, height, 0, 0, 1, projection);

	GL_Cull(CT_TWO_SIDED);

	GL_BindToTMU(src, TB_COLORMAP);

	VectorSet4(quadVerts[0], dstBox[0], dstBox[1], 0, 1);
	VectorSet4(quadVerts[1], dstBox[2], dstBox[1], 0, 1);
	VectorSet4(quadVerts[2], dstBox[2], dstBox[3], 0, 1);
	VectorSet4(quadVerts[3], dstBox[0], dstBox[3], 0, 1);

	texCoords[0][0] = srcBox[0] / (float)src->width; texCoords[0][1] = 1.0f - srcBox[1] / (float)src->height;
	texCoords[1][0] = srcBox[2] / (float)src->width; texCoords[1][1] = 1.0f - srcBox[1] / (float)src->height;
	texCoords[2][0] = srcBox[2] / (float)src->width; texCoords[2][1] = 1.0f - srcBox[3] / (float)src->height;
	texCoords[3][0] = srcBox[0] / (float)src->width; texCoords[3][1] = 1.0f - srcBox[3] / (float)src->height;

	invTexRes[0] = 1.0f / src->width  * srcTexScale[0];
	invTexRes[1] = 1.0f / src->height * srcTexScale[1];

	GL_State( blend );

	GLSL_BindProgram(shaderProgram);

	GLSL_SetUniformMatrix4x4(shaderProgram, UNIFORM_MODELVIEWPROJECTIONMATRIX, projection);
	GLSL_SetUniformVec4(shaderProgram, UNIFORM_COLOR, color);
	GLSL_SetUniformVec2(shaderProgram, UNIFORM_INVTEXRES, invTexRes);
	GLSL_SetUniformVec2(shaderProgram, UNIFORM_AUTOEXPOSUREMINMAX, tr.refdef.autoExposureMinMax);
	GLSL_SetUniformVec3(shaderProgram, UNIFORM_TONEMINAVGMAXLINEAR, tr.refdef.toneMinAvgMaxLinear);

	RB_InstantQuad2(quadVerts, texCoords); //, color, shaderProgram, invTexRes);

	FBO_Bind(oldFbo);
}

void FBO_Blit(FBO_t *src, vec4i_t inSrcBox, vec2_t srcTexScale, FBO_t *dst, vec4i_t dstBox, struct shaderProgram_s *shaderProgram, vec4_t color, int blend)
{
	vec4i_t srcBox;

	if (!src)
	{
		ri.Printf(PRINT_WARNING, "Tried to blit from a NULL FBO!\n");
		return;
	}

	// framebuffers are 0 bottom, Y up.
	if (inSrcBox)
	{
		srcBox[0] = inSrcBox[0];
		srcBox[1] = src->height - inSrcBox[1] - inSrcBox[3];
		srcBox[2] = inSrcBox[2];
		srcBox[3] = inSrcBox[3];
	}
	else
	{
		VectorSet4(srcBox, 0, src->height, src->width, -src->height);
	}

	FBO_BlitFromTexture(src->colorImage[0], srcBox, srcTexScale, dst, dstBox, shaderProgram, color, blend | GLS_DEPTHTEST_DISABLE);
}

void FBO_FastBlit(FBO_t *src, vec4i_t srcBox, FBO_t *dst, vec4i_t dstBox, int buffers, int filter)
{
	vec4i_t srcBoxFinal, dstBoxFinal;
	GLuint srcFb, dstFb;

	// get to a neutral state first
	//FBO_Bind(NULL);

	srcFb = src ? src->frameBuffer : 0;
	dstFb = dst ? dst->frameBuffer : 0;

	if (!srcBox)
	{
		if (src)
		{
			VectorSet4(srcBoxFinal, 0, 0, src->width, src->height);
		}
		else
		{
			VectorSet4(srcBoxFinal, 0, 0, glConfig.vidWidth, glConfig.vidHeight);
		}
	}
	else
	{
		VectorSet4(srcBoxFinal, srcBox[0], srcBox[1], srcBox[0] + srcBox[2], srcBox[1] + srcBox[3]);
	}

	if (!dstBox)
	{
		if (dst)
		{
			VectorSet4(dstBoxFinal, 0, 0, dst->width, dst->height);
		}
		else
		{
			VectorSet4(dstBoxFinal, 0, 0, glConfig.vidWidth, glConfig.vidHeight);
		}
	}
	else
	{
		VectorSet4(dstBoxFinal, dstBox[0], dstBox[1], dstBox[0] + dstBox[2], dstBox[1] + dstBox[3]);
	}

	qglBindFramebuffer(GL_READ_FRAMEBUFFER, srcFb);
	qglBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstFb);

	qglBlitFramebuffer(srcBoxFinal[0], srcBoxFinal[1], srcBoxFinal[2], srcBoxFinal[3],
	                      dstBoxFinal[0], dstBoxFinal[1], dstBoxFinal[2], dstBoxFinal[3],
						  buffers, filter);

	qglBindFramebuffer(GL_FRAMEBUFFER, 0);
	glState.currentFBO = NULL;
}

void FBO_FastBlitIndexed(FBO_t *src, FBO_t *dst, int srcReadBuffer, int dstDrawBuffer, int buffers, int filter)
{
	assert (src != NULL);
	assert (dst != NULL);

	qglBindFramebuffer(GL_READ_FRAMEBUFFER, src->frameBuffer);
	qglReadBuffer (GL_COLOR_ATTACHMENT0 + srcReadBuffer);

	qglBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst->frameBuffer);
	qglDrawBuffer (GL_COLOR_ATTACHMENT0 + dstDrawBuffer);

	qglBlitFramebuffer(0, 0, src->width, src->height,
	                      0, 0, dst->width, dst->height,
						  buffers, filter);

	qglReadBuffer (GL_COLOR_ATTACHMENT0);

	glState.currentFBO = dst;
	FBO_SetupDrawBuffers();

	qglBindFramebuffer(GL_FRAMEBUFFER, 0);
	glState.currentFBO = NULL;
}
