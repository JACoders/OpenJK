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
// tr_shade.c

#include "tr_local.h"
#include "tr_allocator.h"

/*

  THIS ENTIRE FILE IS BACK END

  This file deals with applying shaders to surface data in the tess struct.
*/

color4ub_t	styleColors[MAX_LIGHT_STYLES];

void RB_BinTriangleCounts( void );


/*
==================
R_DrawElements

==================
*/

void R_DrawElementsVBO( int numIndexes, glIndex_t firstIndex, glIndex_t minIndex, glIndex_t maxIndex )
{
	int offset = firstIndex * sizeof(glIndex_t) +
		(tess.useInternalVBO ? backEndData->currentFrame->dynamicIboCommitOffset : 0);

	GL_DrawIndexed(GL_TRIANGLES, numIndexes, GL_INDEX_TYPE, offset, 1, 0);
}


static void R_DrawMultiElementsVBO( int multiDrawPrimitives, glIndex_t *multiDrawMinIndex, glIndex_t *multiDrawMaxIndex,
	GLsizei *multiDrawNumIndexes, glIndex_t **multiDrawFirstIndex)
{
	GL_MultiDrawIndexed(
			GL_TRIANGLES,
			multiDrawNumIndexes,
			multiDrawFirstIndex,
			multiDrawPrimitives);
}


/*
=============================================================

SURFACE SHADERS

=============================================================
*/

shaderCommands_t	tess;


/*
=================
R_BindAnimatedImageToTMU

=================
*/
void R_BindAnimatedImageToTMU( textureBundle_t *bundle, int tmu ) {
	int		index;

	if ( bundle->isVideoMap ) {
		int oldtmu = glState.currenttmu;
		GL_SelectTexture(tmu);
		ri.CIN_RunCinematic(bundle->videoMapHandle);
		ri.CIN_UploadCinematic(bundle->videoMapHandle);
		GL_SelectTexture(oldtmu);
		return;
	}

	if ( bundle->numImageAnimations <= 1 ) {
		GL_BindToTMU( bundle->image[0], tmu);
		return;
	}

	if (backEnd.currentEntity->e.renderfx & RF_SETANIMINDEX )
	{
		index = backEnd.currentEntity->e.skinNum;
	}
	else
	{
		// it is necessary to do this messy calc to make sure animations line up
		// exactly with waveforms of the same frequency
		index = Q_ftol( tess.shaderTime * bundle->imageAnimationSpeed * FUNCTABLE_SIZE );
		index >>= FUNCTABLE_SIZE2;

		if ( index < 0 ) {
			index = 0;	// may happen with shader time offsets
		}
	}

	if ( bundle->oneShotAnimMap )
	{
		if ( index >= bundle->numImageAnimations )
		{
			// stick on last frame
			index = bundle->numImageAnimations - 1;
		}
	}
	else
	{
		// loop
		index %= bundle->numImageAnimations;
	}

	GL_BindToTMU( bundle->image[ index ], tmu );
}

/*
==============
RB_BeginSurface

We must set some things up before beginning any tesselation,
because a surface may be forced to perform a RB_End due
to overflow.
==============
*/
void RB_BeginSurface( shader_t *shader, int fogNum, int cubemapIndex ) {

	shader_t *state = (shader->remappedShader) ? shader->remappedShader : shader;

	tess.numIndexes = 0;
	tess.firstIndex = 0;
	tess.numVertexes = 0;
	tess.multiDrawPrimitives = 0;
	tess.shader = state;
	tess.fogNum = fogNum;
	tess.cubemapIndex = cubemapIndex;
	tess.dlightBits = 0;		// will be OR'd in by surface functions
	tess.pshadowBits = 0;       // will be OR'd in by surface functions
	tess.xstages = state->stages;
	tess.numPasses = state->numUnfoggedPasses;
	tess.currentStageIteratorFunc = state->optimalStageIteratorFunc;
	tess.externalIBO = nullptr;
	tess.useInternalVBO = qtrue;

	tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;
	if (tess.shader->clampTime && tess.shaderTime >= tess.shader->clampTime) {
		tess.shaderTime = tess.shader->clampTime;
	}

	if (backEnd.viewParms.flags & VPF_DEPTHSHADOW)
	{
		tess.currentStageIteratorFunc = RB_StageIteratorGeneric;
	}
}



extern float EvalWaveForm( const waveForm_t *wf );
extern float EvalWaveFormClamped( const waveForm_t *wf );


static void ComputeTexMods( shaderStage_t *pStage, int bundleNum, float *outMatrix, float *outOffTurb)
{
	int tm;
	float matrix[6], currentmatrix[6];
	textureBundle_t *bundle = &pStage->bundle[bundleNum];

	matrix[0] = 1.0f; matrix[2] = 0.0f; matrix[4] = 0.0f;
	matrix[1] = 0.0f; matrix[3] = 1.0f; matrix[5] = 0.0f;

	currentmatrix[0] = 1.0f; currentmatrix[2] = 0.0f; currentmatrix[4] = 0.0f;
	currentmatrix[1] = 0.0f; currentmatrix[3] = 1.0f; currentmatrix[5] = 0.0f;

	outMatrix[0] = 1.0f; outMatrix[2] = 0.0f;
	outMatrix[1] = 0.0f; outMatrix[3] = 1.0f;

	outOffTurb[0] = 0.0f; outOffTurb[1] = 0.0f; outOffTurb[2] = 0.0f; outOffTurb[3] = 0.0f;

	for ( tm = 0; tm < bundle->numTexMods ; tm++ ) {
		switch ( bundle->texMods[tm].type )
		{

		case TMOD_NONE:
			tm = TR_MAX_TEXMODS;		// break out of for loop
			break;

		case TMOD_TURBULENT:
			RB_CalcTurbulentFactors(&bundle->texMods[tm].wave, &outOffTurb[2], &outOffTurb[3]);
			break;

		case TMOD_ENTITY_TRANSLATE:
			RB_CalcScrollTexMatrix( backEnd.currentEntity->e.shaderTexCoord, matrix );
			break;

		case TMOD_SCROLL:
			RB_CalcScrollTexMatrix( bundle->texMods[tm].scroll,
									 matrix );
			break;

		case TMOD_SCALE:
			RB_CalcScaleTexMatrix( bundle->texMods[tm].scale,
								  matrix );
			break;

		case TMOD_STRETCH:
			RB_CalcStretchTexMatrix( &bundle->texMods[tm].wave,
								   matrix );
			break;

		case TMOD_TRANSFORM:
			RB_CalcTransformTexMatrix( &bundle->texMods[tm],
									 matrix );
			break;

		case TMOD_ROTATE:
			RB_CalcRotateTexMatrix( bundle->texMods[tm].rotateSpeed,
									matrix );
			break;

		default:
			ri.Error( ERR_DROP, "ERROR: unknown texmod '%d' in shader '%s'", bundle->texMods[tm].type, tess.shader->name );
			break;
		}

		switch ( bundle->texMods[tm].type )
		{
		case TMOD_NONE:
		case TMOD_TURBULENT:
		default:
			break;

		case TMOD_ENTITY_TRANSLATE:
		case TMOD_SCROLL:
		case TMOD_SCALE:
		case TMOD_STRETCH:
		case TMOD_TRANSFORM:
		case TMOD_ROTATE:
			outMatrix[0] = matrix[0] * currentmatrix[0] + matrix[2] * currentmatrix[1];
			outMatrix[1] = matrix[1] * currentmatrix[0] + matrix[3] * currentmatrix[1];

			outMatrix[2] = matrix[0] * currentmatrix[2] + matrix[2] * currentmatrix[3];
			outMatrix[3] = matrix[1] * currentmatrix[2] + matrix[3] * currentmatrix[3];

			outOffTurb[0] = matrix[0] * currentmatrix[4] + matrix[2] * currentmatrix[5] + matrix[4];
			outOffTurb[1] = matrix[1] * currentmatrix[4] + matrix[3] * currentmatrix[5] + matrix[5];

			currentmatrix[0] = outMatrix[0];
			currentmatrix[1] = outMatrix[1];
			currentmatrix[2] = outMatrix[2];
			currentmatrix[3] = outMatrix[3];
			currentmatrix[4] = outOffTurb[0];
			currentmatrix[5] = outOffTurb[1];
			break;
		}
	}
}

static void ComputeShaderColors( shaderStage_t *pStage, vec4_t baseColor, vec4_t vertColor, int blend, colorGen_t *forceRGBGen, alphaGen_t *forceAlphaGen )
{
	colorGen_t rgbGen = pStage->rgbGen;
	alphaGen_t alphaGen = pStage->alphaGen;

	baseColor[0] =
   	baseColor[1] =
   	baseColor[2] =
   	baseColor[3] = 1.0f;

   	vertColor[0] =
   	vertColor[1] =
   	vertColor[2] =
   	vertColor[3] = 0.0f;

	if ( forceRGBGen != NULL && *forceRGBGen != CGEN_BAD )
	{
		rgbGen = *forceRGBGen;
	}

	if ( forceAlphaGen != NULL && *forceAlphaGen != AGEN_IDENTITY )
	{
		alphaGen = *forceAlphaGen;
	}

	switch ( rgbGen )
	{
		case CGEN_IDENTITY_LIGHTING:
			baseColor[0] =
			baseColor[1] =
			baseColor[2] = tr.identityLight;
			break;
		case CGEN_EXACT_VERTEX:
		case CGEN_EXACT_VERTEX_LIT:
			baseColor[0] =
			baseColor[1] =
			baseColor[2] =
			baseColor[3] = 0.0f;

			vertColor[0] =
			vertColor[1] =
			vertColor[2] =
			vertColor[3] = 1.0f;
			break;
		case CGEN_CONST:
			baseColor[0] = pStage->constantColor[0];
			baseColor[1] = pStage->constantColor[1];
			baseColor[2] = pStage->constantColor[2];
			baseColor[3] = pStage->constantColor[3];
			break;
		case CGEN_VERTEX:
			baseColor[0] =
			baseColor[1] =
			baseColor[2] =
			baseColor[3] = 0.0f;

			vertColor[0] =
			vertColor[1] =
			vertColor[2] = tr.identityLight;
			vertColor[3] = 1.0f;
			break;
		case CGEN_VERTEX_LIT:
			baseColor[0] =
			baseColor[1] =
			baseColor[2] =
			baseColor[3] = 0.0f;

			vertColor[0] =
			vertColor[1] =
			vertColor[2] =
			vertColor[3] = tr.identityLight;
			break;
		case CGEN_ONE_MINUS_VERTEX:
			baseColor[0] =
			baseColor[1] =
			baseColor[2] = tr.identityLight;

			vertColor[0] =
			vertColor[1] =
			vertColor[2] = -tr.identityLight;
			break;
		case CGEN_FOG:
			{
				fog_t *fog = tr.world->fogs + tess.fogNum;
				VectorCopy4(fog->color, baseColor);
			}
			break;
		case CGEN_WAVEFORM:
			baseColor[0] =
			baseColor[1] =
			baseColor[2] = RB_CalcWaveColorSingle( &pStage->rgbWave );
			break;
		case CGEN_ENTITY:
		case CGEN_LIGHTING_DIFFUSE_ENTITY:
			if (backEnd.currentEntity)
			{
				baseColor[0] = ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[0] / 255.0f;
				baseColor[1] = ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[1] / 255.0f;
				baseColor[2] = ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[2] / 255.0f;
				baseColor[3] = ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[3] / 255.0f;

				if ( alphaGen == AGEN_IDENTITY &&
					backEnd.currentEntity->e.shaderRGBA[3] == 255 )
				{
					alphaGen = AGEN_SKIP;
				}
			}
			break;
		case CGEN_ONE_MINUS_ENTITY:
			if (backEnd.currentEntity)
			{
				baseColor[0] = 1.0f - ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[0] / 255.0f;
				baseColor[1] = 1.0f - ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[1] / 255.0f;
				baseColor[2] = 1.0f - ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[2] / 255.0f;
				baseColor[3] = 1.0f - ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[3] / 255.0f;
			}
			break;
		case CGEN_LIGHTMAPSTYLE:
			VectorScale4 (styleColors[pStage->lightmapStyle], 1.0f / 255.0f, baseColor);
			break;
		case CGEN_IDENTITY:
		case CGEN_LIGHTING_DIFFUSE:
		case CGEN_BAD:
			break;
		default:
			break;
	}

	//
	// alphaGen
	//
	switch ( alphaGen )
	{
		case AGEN_SKIP:
			break;
		case AGEN_CONST:
			if ( rgbGen != CGEN_CONST ) {
				baseColor[3] = pStage->constantColor[3];
				vertColor[3] = 0.0f;
			}
			break;
		case AGEN_WAVEFORM:
			baseColor[3] = RB_CalcWaveAlphaSingle( &pStage->alphaWave );
			vertColor[3] = 0.0f;
			break;
		case AGEN_ENTITY:
			if (backEnd.currentEntity)
			{
				baseColor[3] = ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[3] / 255.0f;
			}
			vertColor[3] = 0.0f;
			break;
		case AGEN_ONE_MINUS_ENTITY:
			if (backEnd.currentEntity)
			{
				baseColor[3] = 1.0f - ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[3] / 255.0f;
			}
			vertColor[3] = 0.0f;
			break;
		case AGEN_VERTEX:
			if ( rgbGen != CGEN_VERTEX ) {
				baseColor[3] = 0.0f;
				vertColor[3] = 1.0f;
			}
			break;
		case AGEN_ONE_MINUS_VERTEX:
			baseColor[3] = 1.0f;
			vertColor[3] = -1.0f;
			break;
		case AGEN_IDENTITY:
		case AGEN_LIGHTING_SPECULAR:
		case AGEN_PORTAL:
			// Done entirely in vertex program
			baseColor[3] = 1.0f;
			vertColor[3] = 0.0f;
			break;
		default:
			break;
	}

	if ( forceAlphaGen != NULL )
	{
		*forceAlphaGen = alphaGen;
	}

	if ( forceRGBGen != NULL )
	{
		*forceRGBGen = rgbGen;
	}

	// multiply color by overbrightbits if this isn't a blend
	if (tr.overbrightBits
	 && !((blend & GLS_SRCBLEND_BITS) == GLS_SRCBLEND_DST_COLOR)
	 && !((blend & GLS_SRCBLEND_BITS) == GLS_SRCBLEND_ONE_MINUS_DST_COLOR)
	 && !((blend & GLS_DSTBLEND_BITS) == GLS_DSTBLEND_SRC_COLOR)
	 && !((blend & GLS_DSTBLEND_BITS) == GLS_DSTBLEND_ONE_MINUS_SRC_COLOR))
	{
		float scale = 1 << tr.overbrightBits;

		baseColor[0] *= scale;
		baseColor[1] *= scale;
		baseColor[2] *= scale;
		vertColor[0] *= scale;
		vertColor[1] *= scale;
		vertColor[2] *= scale;
	}

	// FIXME: find some way to implement this.
#if 0
	// if in greyscale rendering mode turn all color values into greyscale.
	if(r_greyscale->integer)
	{
		int scale;

		for(i = 0; i < tess.numVertexes; i++)
		{
			scale = (tess.svars.colors[i][0] + tess.svars.colors[i][1] + tess.svars.colors[i][2]) / 3;
			tess.svars.colors[i][0] = tess.svars.colors[i][1] = tess.svars.colors[i][2] = scale;
		}
	}
#endif
}

static void ComputeFogColorMask( shaderStage_t *pStage, vec4_t fogColorMask )
{
	switch(pStage->adjustColorsForFog)
	{
		case ACFF_MODULATE_RGB:
			fogColorMask[0] =
			fogColorMask[1] =
			fogColorMask[2] = 1.0f;
			fogColorMask[3] = 0.0f;
			break;
		case ACFF_MODULATE_ALPHA:
			fogColorMask[0] =
			fogColorMask[1] =
			fogColorMask[2] = 0.0f;
			fogColorMask[3] = 1.0f;
			break;
		case ACFF_MODULATE_RGBA:
			fogColorMask[0] =
			fogColorMask[1] =
			fogColorMask[2] =
			fogColorMask[3] = 1.0f;
			break;
		default:
			fogColorMask[0] =
			fogColorMask[1] =
			fogColorMask[2] =
			fogColorMask[3] = 0.0f;
			break;
	}
}

static void CaptureDrawData(const shaderCommands_t *input, shaderStage_t *stage, int glslShaderIndex, int stageIndex )
{
	if ( !tr.numFramesToCapture )
		return;

	if ( input->multiDrawPrimitives )
	{
		int numIndexes = 0;
		for ( int i = 0; i < input->multiDrawPrimitives; i++ )
			numIndexes += input->multiDrawNumIndexes[i];

		const char *data = va("%d,%d,%s,%d,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,Y\n",
				tr.frameCount,
				backEnd.currentEntity == &tr.worldEntity ? -1 : (backEnd.currentEntity - tr.refdef.entities),
				stage->glslShaderGroup ? "lightall" : "generic", glslShaderIndex,
				input->shader->name, stageIndex, input->shader->sortedIndex, (int)input->shader->sort,
				input->fogNum,
				input->cubemapIndex,
				glState.vertexAttribsState,
				glState.glStateBits,
				glState.currentVBO->vertexesVBO,
				glState.currentIBO->indexesVBO,
				numIndexes / 3);
		ri.FS_Write(data, strlen(data), tr.debugFile);
	}
	else
	{
		const char *data = va("%d,%d,%s,%d,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,N\n",
				tr.frameCount,
				backEnd.currentEntity == &tr.worldEntity ? -1 : (backEnd.currentEntity - tr.refdef.entities),
				stage->glslShaderGroup ? "lightall" : "generic", glslShaderIndex,
				input->shader->name, stageIndex, input->shader->sortedIndex, (int)input->shader->sort,
				input->fogNum,
				input->cubemapIndex,
				glState.vertexAttribsState,
				glState.glStateBits,
				glState.currentVBO->vertexesVBO,
				glState.currentIBO->indexesVBO,
				input->numIndexes / 3);
		ri.FS_Write(data, strlen(data), tr.debugFile);
	}
}

uint32_t RB_CreateSortKey( const DrawItem& item, int stage, int layer )
{
	uint32_t key = 0;
	uintptr_t shaderProgram = (uintptr_t)item.program;

	assert(stage < 16);
	layer = Q_min(layer, 15);

	key |= (layer & 0xf) << 28;
	key |= (stage & 0xf) << 24;
	key |= shaderProgram & 0x00ffffff;
	return key;
}

static cullType_t RB_GetCullType( const viewParms_t *viewParms, const trRefEntity_t *refEntity, cullType_t shaderCullType )
{
	assert(refEntity);

	cullType_t cullType = CT_TWO_SIDED;
	if ( !backEnd.projection2D )
	{
		if ( shaderCullType != CT_TWO_SIDED )
		{
			bool cullFront = (shaderCullType == CT_FRONT_SIDED);
			if ( viewParms->isMirror )
				cullFront = !cullFront;

			if ( refEntity->mirrored )
				cullFront = !cullFront;

			if ( viewParms->flags & VPF_DEPTHSHADOW )
				cullFront = !cullFront;

			cullType = (cullFront ? CT_FRONT_SIDED : CT_BACK_SIDED);
		}
	}

	return cullType;
}

DepthRange RB_GetDepthRange( const trRefEntity_t *re, const shader_t *shader )
{
	DepthRange range = {0.0f, 1.0f};
	if ( shader->isSky )
	{
		// r_showsky will let all the sky blocks be drawn in
		// front of everything to allow developers to see how
		// much sky is getting sucked in
		if ( !r_showsky->integer )
		{
			range.minDepth = 1.0f;
			range.maxDepth = 1.0f;
		}
		else
		{
			range.maxDepth = 0.0f;
		}
	}
	else if ( re->e.renderfx & RF_NODEPTH )
	{
		range.maxDepth = 0.0f;
	}
	else if ( re->e.renderfx & RF_DEPTHHACK )
	{
		range.maxDepth = 0.3f;
	}

	return range;
}

void RB_FillDrawCommand(
	DrawCommand& drawCmd,
	GLenum primitiveType,
	int numInstances,
	const shaderCommands_t *input
)
{
	drawCmd.primitiveType = primitiveType;
	drawCmd.numInstances = numInstances;

	if ( input->multiDrawPrimitives )
	{
		if ( input->multiDrawPrimitives == 1 )
		{
			drawCmd.type = DRAW_COMMAND_INDEXED;
			drawCmd.params.indexed.indexType = GL_INDEX_TYPE;
			drawCmd.params.indexed.firstIndex = (glIndex_t)(size_t)(input->multiDrawFirstIndex[0]);
			drawCmd.params.indexed.numIndices = input->multiDrawNumIndexes[0];
			drawCmd.params.indexed.baseVertex = 0;
		}
		else
		{
			drawCmd.type = DRAW_COMMAND_MULTI_INDEXED;
			drawCmd.params.multiIndexed.numDraws = input->multiDrawPrimitives;

			drawCmd.params.multiIndexed.firstIndices =
				ojkAllocArray<glIndex_t *>(*backEndData->perFrameMemory, input->multiDrawPrimitives);
			memcpy(drawCmd.params.multiIndexed.firstIndices,
				input->multiDrawFirstIndex,
				sizeof(glIndex_t *) * input->multiDrawPrimitives);

			drawCmd.params.multiIndexed.numIndices =
				ojkAllocArray<GLsizei>(*backEndData->perFrameMemory, input->multiDrawPrimitives);
			memcpy(drawCmd.params.multiIndexed.numIndices,
				input->multiDrawNumIndexes,
				sizeof(GLsizei *) * input->multiDrawPrimitives);
		}
	}
	else
	{
		int offset = input->firstIndex * sizeof(glIndex_t) +
			(input->useInternalVBO ? backEndData->currentFrame->dynamicIboCommitOffset : 0);

		drawCmd.type = DRAW_COMMAND_INDEXED;
		drawCmd.params.indexed.indexType = GL_INDEX_TYPE;
		drawCmd.params.indexed.firstIndex = offset;
		drawCmd.params.indexed.numIndices = input->numIndexes;
		drawCmd.params.indexed.baseVertex = 0;
	}
}

static UniformBlockBinding GetCameraBlockUniformBinding(
	const trRefEntity_t *refEntity)
{
	const GLuint currentFrameUbo = backEndData->currentFrame->ubo;
	UniformBlockBinding binding = {};
	binding.block = UNIFORM_BLOCK_CAMERA;

	if (refEntity == &backEnd.entity2D)
	{
		binding.ubo = tr.staticUbo;
		binding.offset = tr.camera2DUboOffset;
	}
	else if (refEntity == &backEnd.entityFlare)
	{
		binding.ubo = tr.staticUbo;
		binding.offset = tr.cameraFlareUboOffset;
	}
	else
	{
		binding.ubo = currentFrameUbo;
		binding.offset = tr.cameraUboOffsets[backEnd.viewParms.currentViewParm];
	}
	return binding;
}

static UniformBlockBinding GetLightsBlockUniformBinding()
{
	const GLuint currentFrameUbo = backEndData->currentFrame->ubo;
	UniformBlockBinding binding = {};
	binding.block = UNIFORM_BLOCK_LIGHTS;

	if (tr.lightsUboOffset == -1)
	{
		binding.ubo = tr.staticUbo;
		binding.offset = tr.defaultLightsUboOffset;
	}
	else
	{
		binding.ubo = currentFrameUbo;
		binding.offset = tr.lightsUboOffset;
	}
	return binding;
}

static UniformBlockBinding GetSceneBlockUniformBinding()
{
	const GLuint currentFrameUbo = backEndData->currentFrame->ubo;
	UniformBlockBinding binding = {};
	binding.block = UNIFORM_BLOCK_SCENE;

	if (tr.sceneUboOffset == -1)
	{
		binding.ubo = tr.staticUbo;
		binding.offset = tr.defaultSceneUboOffset;
	}
	else
	{
		binding.ubo = currentFrameUbo;
		binding.offset = tr.sceneUboOffset;
	}
	return binding;
}

static UniformBlockBinding GetFogsBlockUniformBinding()
{
	const GLuint currentFrameUbo = backEndData->currentFrame->ubo;
	UniformBlockBinding binding = {};
	binding.block = UNIFORM_BLOCK_FOGS;

	if (tr.fogsUboOffset == -1)
	{
		binding.ubo = tr.staticUbo;
		binding.offset = tr.defaultFogsUboOffset;
	}
	else
	{
		binding.ubo = currentFrameUbo;
		binding.offset = tr.fogsUboOffset;
	}
	return binding;
}

static UniformBlockBinding GetEntityBlockUniformBinding(
	const trRefEntity_t *refEntity)
{
	const GLuint currentFrameUbo = backEndData->currentFrame->ubo;
	UniformBlockBinding binding = {};
	binding.block = UNIFORM_BLOCK_ENTITY;

	if (refEntity == &backEnd.entity2D)
	{
		binding.ubo = tr.staticUbo;
		binding.offset = tr.entity2DUboOffset;
	}
	else if (refEntity == &backEnd.entityFlare)
	{
		binding.ubo = tr.staticUbo;
		binding.offset = tr.entityFlareUboOffset;
	}
	else
	{
		binding.ubo = currentFrameUbo;
		if (!refEntity || refEntity == &tr.worldEntity)
		{
			binding.offset = tr.entityUboOffsets[REFENTITYNUM_WORLD];
		}
		else
		{
			const int refEntityNum = refEntity - backEnd.refdef.entities;
			binding.offset = tr.entityUboOffsets[refEntityNum];
		}
	}

	return binding;
}

static UniformBlockBinding GetBonesBlockUniformBinding(
	const trRefEntity_t *refEntity)
{
	const GLuint currentFrameUbo = backEndData->currentFrame->ubo;
	UniformBlockBinding binding = {};
	binding.ubo = currentFrameUbo;
	binding.block = UNIFORM_BLOCK_BONES;

	if (refEntity == &tr.worldEntity)
		binding.offset = 0;
	else if (refEntity == &backEnd.entity2D)
		binding.offset = 0;
	else
	{
		binding.offset = tr.animationBoneUboOffset;
	}

	return binding;
}

static UniformBlockBinding GetShaderInstanceBlockUniformBinding(
	const trRefEntity_t *refEntity, const shader_t *shader)
{
	const GLuint currentFrameUbo = backEndData->currentFrame->ubo;
	UniformBlockBinding binding = {};
	binding.ubo = tr.shaderInstanceUbo;
	binding.block = UNIFORM_BLOCK_SHADER_INSTANCE;

	if (shader->ShaderInstanceUboOffset == -1)
	{
		binding.ubo = tr.staticUbo;
		binding.offset = tr.defaultShaderInstanceUboOffset;
		return binding;
	}

	binding.offset = shader->ShaderInstanceUboOffset;

	return binding;
}

/*
================
DrawTris

Draws triangle outlines for debugging
================
*/
static void DrawTris(shaderCommands_t *input, const VertexArraysProperties *vertexArrays) {

	Allocator& frameAllocator = *backEndData->perFrameMemory;
	vertexAttribute_t attribs[ATTR_INDEX_MAX] = {};
	GL_VertexArraysToAttribs(attribs, ARRAY_LEN(attribs), vertexArrays);
	{
		int index = 0;

		if (input->shader->numDeforms && !ShaderRequiresCPUDeforms(input->shader))
		{
			index |= GENERICDEF_USE_DEFORM_VERTEXES;
		}
#ifdef REND2_SP
		if (glState.vertexAnimation)
		{
			index |= GENERICDEF_USE_VERTEX_ANIMATION;
		}
#endif // REND2_SP
		else if (glState.skeletalAnimation)
		{
			index |= GENERICDEF_USE_SKELETAL_ANIMATION;
		}

		shaderProgram_t *sp = &tr.genericShader[index];
		assert(sp);
		UniformDataWriter uniformDataWriter;
		SamplerBindingsWriter samplerBindingsWriter;
		uniformDataWriter.Start(sp);

		const UniformBlockBinding uniformBlockBindings[] = {
			GetCameraBlockUniformBinding(backEnd.currentEntity),
			GetSceneBlockUniformBinding(),
			GetEntityBlockUniformBinding(backEnd.currentEntity),
			GetShaderInstanceBlockUniformBinding(
				backEnd.currentEntity, input->shader),
			GetBonesBlockUniformBinding(backEnd.currentEntity)
		};

		samplerBindingsWriter.AddStaticImage(tr.whiteImage, TB_DIFFUSEMAP);
		const vec4_t baseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		const vec4_t vertColor = { 0.0f, 0.0f, 0.0f, 0.0f };
		uniformDataWriter.SetUniformVec4(UNIFORM_BASECOLOR, baseColor);
		uniformDataWriter.SetUniformVec4(UNIFORM_VERTCOLOR, vertColor);

		DrawItem item = {};
		item.renderState.stateBits = GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE | GLS_POLYGON_OFFSET_FILL;
		item.renderState.cullType = RB_GetCullType(&backEnd.viewParms, backEnd.currentEntity, input->shader->cullType);
		item.renderState.depthRange = RB_GetDepthRange(backEnd.currentEntity, input->shader);
		item.program = sp;
		item.ibo = input->externalIBO ? input->externalIBO : backEndData->currentFrame->dynamicIbo;
		item.uniformData = uniformDataWriter.Finish(frameAllocator);
		item.samplerBindings = samplerBindingsWriter.Finish(
			frameAllocator, &item.numSamplerBindings);

		DrawItemSetVertexAttributes(
			item, attribs, vertexArrays->numVertexArrays, frameAllocator);
		DrawItemSetUniformBlockBindings(
			item, uniformBlockBindings, frameAllocator);

		RB_FillDrawCommand(item.draw, GL_TRIANGLES, 1, input);

		uint32_t key = RB_CreateSortKey(item, 15, 15);

		RB_AddDrawItem(backEndData->currentPass, key, item);
	}
}

/*
================
DrawNormals

Draws vertex normals for debugging
================
*/
static void DrawNormals(shaderCommands_t *input) {
	//FIXME: implement this
}

static void ProjectPshadowVBOGLSL( const shaderCommands_t *input, const VertexArraysProperties *vertexArrays ) {
	int		l;
	vec3_t	origin;
	float	radius;

	if ( !backEnd.refdef.num_pshadows ) {
		return;
	}

	cullType_t cullType = RB_GetCullType(&backEnd.viewParms, backEnd.currentEntity, input->shader->cullType);

	UniformDataWriter uniformDataWriter;
	SamplerBindingsWriter samplerBindingsWriter;
	shaderStage_t *pStage = tess.xstages[0];

	vertexAttribute_t attribs[ATTR_INDEX_MAX] = {};
	GL_VertexArraysToAttribs(attribs, ARRAY_LEN(attribs), vertexArrays);

	for ( l = 0; l < backEnd.refdef.num_pshadows; l++ ) {
		pshadow_t	*ps;
		shaderProgram_t *sp;
		vec4_t vector;

		if ( !( tess.pshadowBits & ( 1 << l ) ) ) {
			continue;	// this surface definately doesn't have any of this shadow
		}

		ps = &backEnd.refdef.pshadows[l];
		VectorCopy( ps->lightOrigin, origin );
		radius = ps->lightRadius;

		sp = &tr.pshadowShader;

		uniformDataWriter.Start(sp);

		uniformDataWriter.SetUniformMatrix4x4(UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);

		VectorCopy(origin, vector);
		vector[3] = (float)l;
		uniformDataWriter.SetUniformVec4(UNIFORM_LIGHTORIGIN, vector);

		VectorScale(ps->lightViewAxis[0], 1.0f, vector);
		uniformDataWriter.SetUniformVec3(UNIFORM_LIGHTFORWARD, vector);

		VectorScale(ps->lightViewAxis[1], 1.0f / ps->viewRadius, vector);
		uniformDataWriter.SetUniformVec3(UNIFORM_LIGHTRIGHT, vector);

		VectorScale(ps->lightViewAxis[2], 1.0f / ps->viewRadius, vector);
		uniformDataWriter.SetUniformVec3(UNIFORM_LIGHTUP, vector);

		uniformDataWriter.SetUniformFloat(UNIFORM_LIGHTRADIUS, radius);

		// include GLS_DEPTHFUNC_EQUAL so alpha tested surfaces don't add light
		// where they aren't rendered
		uint32_t stateBits = 0;
		stateBits = GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL;

		samplerBindingsWriter.AddStaticImage(tr.pshadowArrayImage, TB_DIFFUSEMAP);

		CaptureDrawData(input, pStage, 0, 0);

		DrawItem item = {};
		item.renderState.stateBits = stateBits;
		item.renderState.cullType = cullType;
		item.program = sp;
		item.renderState.depthRange = RB_GetDepthRange(backEnd.currentEntity, input->shader);
		item.ibo = input->externalIBO ? input->externalIBO : backEndData->currentFrame->dynamicIbo;

		item.numAttributes = vertexArrays->numVertexArrays;
		item.attributes = ojkAllocArray<vertexAttribute_t>(
			*backEndData->perFrameMemory, vertexArrays->numVertexArrays);
		memcpy(item.attributes, attribs, sizeof(*item.attributes)*vertexArrays->numVertexArrays);

		item.uniformData = uniformDataWriter.Finish(*backEndData->perFrameMemory);
		// FIXME: This is a bit ugly with the casting
		item.samplerBindings = samplerBindingsWriter.Finish(
			*backEndData->perFrameMemory, &item.numSamplerBindings);

		RB_FillDrawCommand(item.draw, GL_TRIANGLES, 1, input);

		uint32_t key = RB_CreateSortKey(item, 13, input->shader->sort);
		RB_AddDrawItem(backEndData->currentPass, key, item);

		backEnd.pc.c_totalIndexes += tess.numIndexes;

		RB_BinTriangleCounts();
	}
}

/*
===================
RB_FogPass

Blends a fog texture on top of everything else
===================
*/
static void RB_FogPass( shaderCommands_t *input, const VertexArraysProperties *vertexArrays )
{
	cullType_t cullType = RB_GetCullType(&backEnd.viewParms, backEnd.currentEntity, input->shader->cullType);

	vertexAttribute_t attribs[ATTR_INDEX_MAX] = {};
	GL_VertexArraysToAttribs(attribs, ARRAY_LEN(attribs), vertexArrays);

	int shaderBits = 0;

	if (input->shader->numDeforms && !ShaderRequiresCPUDeforms(input->shader))
		shaderBits |= FOGDEF_USE_DEFORM_VERTEXES;
#ifdef REND2_SP
	if (glState.vertexAnimation)
		shaderBits |= FOGDEF_USE_VERTEX_ANIMATION;
#endif // REND2_SP
	else if (glState.skeletalAnimation)
		shaderBits |= FOGDEF_USE_SKELETAL_ANIMATION;

	if (tr.world && tr.world->globalFog &&
		input->fogNum != tr.world->globalFogIndex &&
		input->shader->sort != SS_FOG)
		shaderBits |= FOGDEF_USE_FALLBACK_GLOBAL_FOG;

	if (input->numPasses > 0)
		if (input->xstages[0]->alphaTestType != ALPHA_TEST_NONE)
			shaderBits |= FOGDEF_USE_ALPHA_TEST;

	shaderProgram_t *sp = tr.fogShader + shaderBits;

	backEnd.pc.c_fogDraws++;

	UniformDataWriter uniformDataWriter;
	uniformDataWriter.Start(sp);
	uniformDataWriter.SetUniformInt(UNIFORM_FOGINDEX, input->fogNum - 1);
	if (input->numPasses > 0)
		uniformDataWriter.SetUniformInt(UNIFORM_ALPHA_TEST_TYPE, input->xstages[0]->alphaTestType);

	uint32_t stateBits = GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	if ( tess.shader->fogPass == FP_EQUAL )
		stateBits |= GLS_DEPTHFUNC_EQUAL;

	if (input->shader->polygonOffset == qtrue)
		stateBits |= GLS_POLYGON_OFFSET_FILL;

	const UniformBlockBinding uniformBlockBindings[] = {
		GetCameraBlockUniformBinding(backEnd.currentEntity),
		GetFogsBlockUniformBinding(),
		GetEntityBlockUniformBinding(backEnd.currentEntity),
		GetShaderInstanceBlockUniformBinding(
			backEnd.currentEntity, input->shader),
		GetBonesBlockUniformBinding(backEnd.currentEntity)
	};

	SamplerBindingsWriter samplerBindingsWriter;
	if (input->numPasses > 0)
		if (input->xstages[0]->alphaTestType != ALPHA_TEST_NONE)
			samplerBindingsWriter.AddStaticImage(input->xstages[0]->bundle[0].image[0], 0);

	Allocator& frameAllocator = *backEndData->perFrameMemory;
	DrawItem item = {};
	item.renderState.stateBits = stateBits;
	item.renderState.cullType = cullType;
	item.renderState.depthRange = RB_GetDepthRange(backEnd.currentEntity, input->shader);
	item.program = sp;
	item.uniformData = uniformDataWriter.Finish(frameAllocator);
	item.ibo = input->externalIBO ? input->externalIBO : backEndData->currentFrame->dynamicIbo;
	item.samplerBindings = samplerBindingsWriter.Finish(
		frameAllocator, &item.numSamplerBindings);

	DrawItemSetVertexAttributes(
		item, attribs, vertexArrays->numVertexArrays, frameAllocator);
	DrawItemSetUniformBlockBindings(
		item, uniformBlockBindings, frameAllocator);

	RB_FillDrawCommand(item.draw, GL_TRIANGLES, 1, input);

	const uint32_t key = RB_CreateSortKey(item, 14, input->shader->sort);
	RB_AddDrawItem(backEndData->currentPass, key, item);

	// invert fog planes and render global fog into them
	if (input->fogNum != tr.world->globalFogIndex && tr.world->globalFogIndex != -1)
	{
		// only invert render fog planes
		if (input->shader->sort != SS_FOG)
			return;
		if (backEnd.currentEntity && backEnd.currentEntity != &tr.worldEntity)
			return;
		// well, no idea how to handle this case, it's actually wrong
		if (cullType == CT_TWO_SIDED)
			return;
		if (cullType == CT_FRONT_SIDED)
			cullType = CT_BACK_SIDED;
		else
			cullType = CT_FRONT_SIDED;
		UniformDataWriter uniformDataWriterBack;
		uniformDataWriterBack.Start(sp);
		uniformDataWriterBack.SetUniformInt(UNIFORM_FOGINDEX, tr.world->globalFogIndex - 1);
		if (input->numPasses > 0)
		{
			uniformDataWriterBack.SetUniformInt(UNIFORM_ALPHA_TEST_TYPE, input->xstages[0]->alphaTestType);
			SamplerBindingsWriter samplerBindingsWriter;
			if (input->xstages[0]->alphaTestType != ALPHA_TEST_NONE)
				samplerBindingsWriter.AddStaticImage(input->xstages[0]->bundle[0].image[0], 0);
		}

		DrawItem backItem = {};
		memcpy(&backItem, &item, sizeof(item));
		backItem.renderState.cullType = cullType;
		backItem.uniformData = uniformDataWriterBack.Finish(frameAllocator);
		backItem.samplerBindings = samplerBindingsWriter.Finish(
			frameAllocator, &item.numSamplerBindings);

		DrawItemSetVertexAttributes(
			backItem, attribs, vertexArrays->numVertexArrays, frameAllocator);
		DrawItemSetUniformBlockBindings(
			backItem, uniformBlockBindings, frameAllocator);

		RB_FillDrawCommand(backItem.draw, GL_TRIANGLES, 1, input);

		const uint32_t key = RB_CreateSortKey(backItem, 14, input->shader->sort);
		RB_AddDrawItem(backEndData->currentPass, key, backItem);
	}
}

static unsigned int RB_CalcShaderVertexAttribs( const shader_t *shader )
{
	unsigned int vertexAttribs = shader->vertexAttribs;
#ifdef REND2_SP
	if(glState.vertexAnimation)
	{
		//vertexAttribs &= ~ATTR_COLOR;
		vertexAttribs |= ATTR_POSITION2;
		if (vertexAttribs & ATTR_NORMAL)
		{
			vertexAttribs |= ATTR_NORMAL2;
			vertexAttribs |= ATTR_TANGENT2;
		}
	}
#endif // REND2_SP
	if (glState.skeletalAnimation)
	{
		vertexAttribs |= ATTR_BONE_WEIGHTS;
		vertexAttribs |= ATTR_BONE_INDEXES;
	}

	return vertexAttribs;
}

static shaderProgram_t *SelectShaderProgram( int stageIndex, shaderStage_t *stage, shaderProgram_t *glslShaderGroup, bool useAlphaTestGE192, bool forceRefraction )
{
	uint32_t index;
	shaderProgram_t *result = nullptr;

	if (forceRefraction)
	{
		index = 0;
		if (tess.shader->numDeforms && !ShaderRequiresCPUDeforms(tess.shader))
		{
			index |= REFRACTIONDEF_USE_DEFORM_VERTEXES;
		}
#ifdef REND2_SP
		if (glState.vertexAnimation)
		{
			index |= REFRACTIONDEF_USE_VERTEX_ANIMATION;
		}
#endif // REND2_SP
		else if (glState.skeletalAnimation)
		{
			index |= REFRACTIONDEF_USE_SKELETAL_ANIMATION;
		}

		if (stage->bundle[0].tcGen != TCGEN_TEXTURE || (stage->bundle[0].numTexMods))
			index |= REFRACTIONDEF_USE_TCGEN_AND_TCMOD;

		if (!useAlphaTestGE192)
		{
			if (stage->alphaTestType != ALPHA_TEST_NONE)
				index |= REFRACTIONDEF_USE_TCGEN_AND_TCMOD | REFRACTIONDEF_USE_ALPHA_TEST;
		}
		else
		{
			index |= REFRACTIONDEF_USE_ALPHA_TEST;
		}

		if (tr.hdrLighting == qtrue)
			index |= REFRACTIONDEF_USE_SRGB_TRANSFORM;

		return &tr.refractionShader[index];
	}

	if (backEnd.depthFill)
	{
		if (glslShaderGroup == tr.lightallShader)
		{
			index = 0;

			if (backEnd.currentEntity && backEnd.currentEntity != &tr.worldEntity)
			{
#ifdef REND2_SP
				if (glState.vertexAnimation)
				{
					index |= LIGHTDEF_USE_VERTEX_ANIMATION;
				}
				else
#endif // REND2_SP
				if (glState.skeletalAnimation)
				{
					index |= LIGHTDEF_USE_SKELETAL_ANIMATION;
				}
			}

			if ( !useAlphaTestGE192 )
			{
				if (stage->alphaTestType != ALPHA_TEST_NONE)
					index |= LIGHTDEF_USE_ALPHA_TEST;
			}
			else
			{
				index |= LIGHTDEF_USE_ALPHA_TEST;
			}

			result = &stage->glslShaderGroup[index];
			backEnd.pc.c_lightallDraws++;
		}
		else
		{
			index = 0;

			if (tess.shader->numDeforms && !ShaderRequiresCPUDeforms(tess.shader))
			{
				index |= GENERICDEF_USE_DEFORM_VERTEXES;
			}
#ifdef REND2_SP
			if (glState.vertexAnimation)
			{
				index |= GENERICDEF_USE_VERTEX_ANIMATION;
			}
#endif // REND2_SP
			else if (glState.skeletalAnimation)
			{
				index |= GENERICDEF_USE_SKELETAL_ANIMATION;
			}

			if ( !useAlphaTestGE192 )
			{
				if (stage->alphaTestType != ALPHA_TEST_NONE)
					index |= GENERICDEF_USE_TCGEN_AND_TCMOD | GENERICDEF_USE_ALPHA_TEST;
			}
			else
			{
				index |= GENERICDEF_USE_ALPHA_TEST;
			}

			if (backEnd.currentEntity->e.renderfx & (RF_DISINTEGRATE1 | RF_DISINTEGRATE2))
				index |= GENERICDEF_USE_RGBAGEN;

			if (backEnd.currentEntity->e.renderfx & RF_DISINTEGRATE2)
				index |= GENERICDEF_USE_DEFORM_VERTEXES;

			result = &tr.genericShader[index];
			backEnd.pc.c_genericDraws++;
		}
	}
	else if (stage->glslShaderGroup == tr.lightallShader)
	{
		index = stage->glslShaderIndex;

		if (r_lightmap->integer && (index & LIGHTDEF_USE_LIGHTMAP) && !(index & LIGHTDEF_USE_LIGHT_VERTEX))
		{
			index = LIGHTDEF_USE_LIGHTMAP;
		}
		else
		{
			if (backEnd.currentEntity && backEnd.currentEntity != &tr.worldEntity)
			{
#ifdef REND2_SP
				if (glState.vertexAnimation)
				{
					index |= LIGHTDEF_USE_VERTEX_ANIMATION;
				}
#endif // REND2_SP
				if (glState.skeletalAnimation)
				{
					index |= LIGHTDEF_USE_SKELETAL_ANIMATION;
				}
			}

			if ( !useAlphaTestGE192 )
			{
				if (stage->alphaTestType != ALPHA_TEST_NONE)
					index |= LIGHTDEF_USE_TCGEN_AND_TCMOD | LIGHTDEF_USE_ALPHA_TEST;
			}
			else
			{
				index |= LIGHTDEF_USE_ALPHA_TEST;
			}
		}

		result = &stage->glslShaderGroup[index];
		backEnd.pc.c_lightallDraws++;
	}
	else
	{
		result = GLSL_GetGenericShaderProgram(stageIndex);
		backEnd.pc.c_genericDraws++;
	}

	return result;
}

/*
=================
RB_ShadowTessEnd

=================
*/
void RB_ShadowTessEnd(shaderCommands_t *input, const VertexArraysProperties *vertexArrays) {
	if (glConfig.stencilBits < 4) {
		ri.Printf(PRINT_ALL, "no stencil bits for stencil writing\n");
		return;
	}

	if (!input->numVertexes || !input->numIndexes || input->useInternalVBO)
	{
		return;
	}

	vertexAttribute_t attribs[ATTR_INDEX_MAX] = {};
	GL_VertexArraysToAttribs(attribs, ARRAY_LEN(attribs), vertexArrays);
	GL_VertexAttribPointers(vertexArrays->numVertexArrays, attribs);

	Allocator& frameAllocator = *backEndData->perFrameMemory;

	cullType_t cullType = CT_TWO_SIDED;

	int stateBits = GLS_DEPTHFUNC_LESS | GLS_STENCILTEST_ENABLE | GLS_COLORMASK_BITS;

	const UniformBlockBinding uniformBlockBindings[] = {
		GetCameraBlockUniformBinding(backEnd.currentEntity),
		GetEntityBlockUniformBinding(backEnd.currentEntity),
		GetBonesBlockUniformBinding(backEnd.currentEntity)
	};

	DrawItem item = {};
	item.renderState.stateBits = stateBits;
	item.renderState.cullType = cullType;
	DepthRange range = { 0.0f, 1.0f };
	item.renderState.depthRange = range;
	item.program = &tr.volumeShadowShader;
	item.ibo = input->externalIBO ? input->externalIBO : backEndData->currentFrame->dynamicIbo;

	DrawItemSetVertexAttributes(
		item, attribs, vertexArrays->numVertexArrays, frameAllocator);
	DrawItemSetUniformBlockBindings(
		item, uniformBlockBindings, frameAllocator);

	RB_FillDrawCommand(item.draw, GL_TRIANGLES, 1, input);

	const uint32_t key = RB_CreateSortKey(item, 13, 15);
	RB_AddDrawItem(backEndData->currentPass, key, item);
}

static void RB_IterateStagesGeneric( shaderCommands_t *input, const VertexArraysProperties *vertexArrays )
{
	Allocator& frameAllocator = *backEndData->perFrameMemory;
	cullType_t cullType = RB_GetCullType(&backEnd.viewParms, backEnd.currentEntity, input->shader->cullType);

	vertexAttribute_t attribs[ATTR_INDEX_MAX] = {};
	GL_VertexArraysToAttribs(attribs, ARRAY_LEN(attribs), vertexArrays);

	UniformDataWriter uniformDataWriter;
	SamplerBindingsWriter samplerBindingsWriter;

	for ( int stage = 0; stage < MAX_SHADER_STAGES; stage++ )
	{
		shaderStage_t *pStage = input->xstages[stage];
		shaderProgram_t *sp;
		vec4_t texMatrix;
		vec4_t texOffTurb;
		int stateBits;
		colorGen_t forceRGBGen = CGEN_BAD;
		alphaGen_t forceAlphaGen = AGEN_IDENTITY;
		int index = 0;
		bool useAlphaTestGE192 = false;
		bool forceRefraction = false;
		vec4_t disintegrationInfo;

		if ( !pStage )
		{
			break;
		}

		if ( pStage->ss )
		{
			continue;
		}

		stateBits = pStage->stateBits;

		if (backEnd.currentEntity)
		{
			assert(backEnd.currentEntity->e.renderfx >= 0);

			if ( backEnd.currentEntity->e.renderfx & ( RF_DISINTEGRATE1 | RF_DISINTEGRATE2 ))
			{
				if ( backEnd.currentEntity->e.renderfx & RF_DISINTEGRATE1 )
				{
					// we want to be able to rip a hole in the thing being
					// disintegrated, and by doing the depth-testing it avoids some
					// kinds of artefacts, but will probably introduce others?
					stateBits = GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHMASK_TRUE;
					forceRGBGen = CGEN_DISINTEGRATION_1;
					useAlphaTestGE192 = true;
				}
				else
					forceRGBGen = CGEN_DISINTEGRATION_2;

				disintegrationInfo[0] = backEnd.currentEntity->e.oldorigin[0];
				disintegrationInfo[1] = backEnd.currentEntity->e.oldorigin[1];
				disintegrationInfo[2] = backEnd.currentEntity->e.oldorigin[2];
				disintegrationInfo[3] = (backEnd.refdef.time - backEnd.currentEntity->e.endTime) * 0.045f;
				disintegrationInfo[3] *= disintegrationInfo[3];
			}
			else if ( backEnd.currentEntity->e.renderfx & RF_RGB_TINT )
			{//want to use RGBGen from ent
				forceRGBGen = CGEN_ENTITY;
			}

			// only force blend on the internal distortion shader
			if (input->shader == tr.distortionShader)
				stateBits = GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;

			if (input->shader->useDistortion == qtrue || backEnd.currentEntity->e.renderfx & RF_DISTORTION)
				forceRefraction = true;

			if ( backEnd.currentEntity->e.renderfx & RF_FORCE_ENT_ALPHA )
			{
				stateBits = GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
				if ( backEnd.currentEntity->e.renderfx & RF_ALPHA_DEPTH )
				{
					// depth write, so faces through the model will be stomped
					// over by nearer ones. this works because we draw
					// RF_FORCE_ENT_ALPHA stuff after everything else, including
					// standard alpha surfs.
					stateBits |= GLS_DEPTHMASK_TRUE;
				}
			}
		}

		if (backEnd.viewParms.flags & VPF_POINTSHADOW)
			stateBits |= GLS_POLYGON_OFFSET_FILL;

		sp = SelectShaderProgram(stage, pStage, pStage->glslShaderGroup, useAlphaTestGE192, forceRefraction);
		assert(sp);

		uniformDataWriter.Start(sp);

		if ( input->fogNum ) {
			vec4_t fogColorMask;
			ComputeFogColorMask(pStage, fogColorMask);
			uniformDataWriter.SetUniformVec4(UNIFORM_FOGCOLORMASK, fogColorMask);
			uniformDataWriter.SetUniformInt(UNIFORM_FOGINDEX, input->fogNum - 1);
		}

		float volumetricBaseValue = -1.0f;
		if ( backEnd.currentEntity->e.renderfx & RF_VOLUMETRIC )
		{
			volumetricBaseValue = backEnd.currentEntity->e.shaderRGBA[0] / 255.0f;
		}
		else
		{
			vec4_t baseColor;
			vec4_t vertColor;

#ifdef REND2_SP_MAYBE
			// Fade will be set true when rendering some gore surfaces
			if (input->fade)
			{
				VectorCopy4(input->svars.colors[input->firstIndex], baseColor);
				VectorScale4(baseColor, 1.0f / 255.0f, baseColor);
				VectorSet4(vertColor, 0.0f, 0.0f, 0.0f, 0.0f);
			}
			else
#endif
				ComputeShaderColors(pStage, baseColor, vertColor, stateBits, &forceRGBGen, &forceAlphaGen);

			if ((backEnd.refdef.colorScale != 1.0f) && !(backEnd.refdef.rdflags & RDF_NOWORLDMODEL))
			{
				// use VectorScale to only scale first three values, not alpha
				VectorScale(baseColor, backEnd.refdef.colorScale, baseColor);
				VectorScale(vertColor, backEnd.refdef.colorScale, vertColor);
			}

			if (backEnd.currentEntity->e.renderfx & RF_FORCE_ENT_ALPHA)
			{
				baseColor[3] = backEnd.currentEntity->e.shaderRGBA[3] / 255.0f;
				vertColor[3] = 0.0f;
			}

			if (backEnd.currentEntity->e.hModel != NULL)
			{
				model_t *model = R_GetModelByHandle(backEnd.currentEntity->e.hModel);
				if (model->type != MOD_BRUSH)
				{
					switch (forceRGBGen)
					{
					case CGEN_EXACT_VERTEX:
					case CGEN_EXACT_VERTEX_LIT:
					case CGEN_VERTEX:
					case CGEN_VERTEX_LIT:
						baseColor[0] =
							baseColor[1] =
							baseColor[2] =
							baseColor[3] = 0.0f;

						vertColor[0] =
							vertColor[1] =
							vertColor[2] =
							vertColor[3] = tr.identityLight;
						break;
					default:
						break;
					}
				}
			}

			uniformDataWriter.SetUniformVec4(UNIFORM_BASECOLOR, baseColor);
			uniformDataWriter.SetUniformVec4(UNIFORM_VERTCOLOR, vertColor);
		}

#if 0 // TODO: Revisit this, isn't it just a alphaGen?
		if (pStage->alphaGen == AGEN_PORTAL)
		{
			uniformDataWriter.SetUniformFloat(UNIFORM_PORTALRANGE, tess.shader->portalRange);
		}
#endif
		uniformDataWriter.SetUniformInt(UNIFORM_COLORGEN, forceRGBGen);
		uniformDataWriter.SetUniformInt(UNIFORM_ALPHAGEN, forceAlphaGen);

		if (backEnd.currentEntity->e.renderfx & (RF_DISINTEGRATE1 | RF_DISINTEGRATE2))
			uniformDataWriter.SetUniformVec4(UNIFORM_DISINTEGRATION, disintegrationInfo);

		if (forceRefraction)
		{
			vec4_t color;
			color[0] =
			color[1] =
			color[2] = powf(2.0f, r_cameraExposure->value);
			color[3] = 10.0f;
			uniformDataWriter.SetUniformVec4(UNIFORM_COLOR, color);
			uniformDataWriter.SetUniformVec2(UNIFORM_AUTOEXPOSUREMINMAX, tr.refdef.autoExposureMinMax);
			uniformDataWriter.SetUniformVec3(UNIFORM_TONEMINAVGMAXLINEAR, tr.refdef.toneMinAvgMaxLinear);
		}

#ifdef REND2_SP_MAYBE
		// tess scale will be set true only when theres scaled gore
		if (!input->scale)
			texMatrix[0] = texMatrix[3] = input->texCoords[input->firstIndex][0][0];
		else
#endif
			ComputeTexMods(pStage, TB_DIFFUSEMAP, texMatrix, texOffTurb);

		uniformDataWriter.SetUniformVec4(UNIFORM_DIFFUSETEXMATRIX, texMatrix);
		uniformDataWriter.SetUniformVec4(UNIFORM_DIFFUSETEXOFFTURB, texOffTurb);

		uniformDataWriter.SetUniformInt(UNIFORM_TCGEN0, pStage->bundle[0].tcGen);
		uniformDataWriter.SetUniformInt(UNIFORM_TCGEN1, pStage->bundle[1].tcGen);
		if (pStage->bundle[0].tcGen == TCGEN_VECTOR)
		{
			uniformDataWriter.SetUniformVec3(UNIFORM_TCGEN0VECTOR0, pStage->bundle[0].tcGenVectors[0]);
			uniformDataWriter.SetUniformVec3(UNIFORM_TCGEN0VECTOR1, pStage->bundle[0].tcGenVectors[1]);
		}

		vec4_t normalScale = {
			pStage->normalScale[0],
			pStage->normalScale[1],
			backEnd.viewParms.isPortal ? -1.0f : 1.0f,
			pStage->normalScale[3]
		};

		uniformDataWriter.SetUniformVec4(UNIFORM_NORMALSCALE, normalScale);
		uniformDataWriter.SetUniformVec4(UNIFORM_SPECULARSCALE, pStage->specularScale);

		const float parallaxBias = r_forceParallaxBias->value > 0.0f ? r_forceParallaxBias->value : pStage->parallaxBias;
		uniformDataWriter.SetUniformFloat(UNIFORM_PARALLAXBIAS, parallaxBias);

		const AlphaTestType alphaTestType =
			useAlphaTestGE192 ? ALPHA_TEST_GE192 : pStage->alphaTestType;
		uniformDataWriter.SetUniformInt(UNIFORM_ALPHA_TEST_TYPE, alphaTestType);

		//
		// do multitexture
		//
		bool enableCubeMaps = (	r_cubeMapping->integer
								&& !(tr.viewParms.flags & VPF_NOCUBEMAPS)
								&& input->cubemapIndex > 0
								&& pStage->rgbGen != CGEN_LIGHTMAPSTYLE );
		bool enableDLights = (	tess.dlightBits
								&& tess.shader->sort <= SS_OPAQUE
								&& !(tess.shader->surfaceFlags & (SURF_NODLIGHT | SURF_SKY))
								&& pStage->rgbGen != CGEN_LIGHTMAPSTYLE );

		if (forceRefraction)
		{
			FBO_t *srcFbo = tr.renderFbo;
			if (tr.msaaResolveFbo)
				srcFbo = tr.msaaResolveFbo;

			samplerBindingsWriter.AddStaticImage(srcFbo->colorImage[0], TB_COLORMAP);
			samplerBindingsWriter.AddStaticImage(tr.renderDepthImage, TB_SHADOWMAP);
			qboolean autoExposure = (qboolean)(r_autoExposure->integer || r_forceAutoExposure->integer);

			if (autoExposure)
				samplerBindingsWriter.AddStaticImage(tr.calcLevelsImage, TB_LEVELSMAP);
			else
				samplerBindingsWriter.AddStaticImage(tr.fixedLevelsImage, TB_LEVELSMAP);
		}
		else if ( backEnd.depthFill )
		{
			if (pStage->alphaTestType == ALPHA_TEST_NONE)
				samplerBindingsWriter.AddStaticImage(tr.whiteImage, 0);
			else if ( pStage->bundle[TB_COLORMAP].image[0] != 0 )
				samplerBindingsWriter.AddAnimatedImage(&pStage->bundle[TB_COLORMAP], TB_COLORMAP);
		}
		else if ( pStage->glslShaderGroup == tr.lightallShader )
		{
			int i;
			vec4_t enableTextures = {};

			if (r_sunlightMode->integer &&
					(backEnd.viewParms.flags & VPF_USESUNLIGHT) &&
					(pStage->glslShaderIndex & LIGHTDEF_LIGHTTYPE_MASK))
			{
				samplerBindingsWriter.AddStaticImage(tr.sunShadowArrayImage, TB_SHADOWMAP);
				enableTextures[2] = 1.0f;
			}
			else
				samplerBindingsWriter.AddStaticImage(tr.whiteImage, TB_SHADOWMAP);

			if ((r_lightmap->integer == 1 || r_lightmap->integer == 2) &&
				(pStage->bundle[0].isLightmap || pStage->bundle[1].isLightmap))
			{
				for (i = 0; i < NUM_TEXTURE_BUNDLES; i++)
				{
					if (i == TB_DIFFUSEMAP)
						samplerBindingsWriter.AddStaticImage(tr.whiteImage, i);
					else
						samplerBindingsWriter.AddAnimatedImage(&pStage->bundle[i], i);
				}
			}
			else if (r_lightmap->integer == 3 && pStage->bundle[TB_DELUXEMAP].image[0])
			{
				for (i = 0; i < NUM_TEXTURE_BUNDLES; i++)
				{
					if (i == TB_LIGHTMAP)
						samplerBindingsWriter.AddAnimatedImage(&pStage->bundle[TB_DELUXEMAP], i);
					else if (i == TB_DIFFUSEMAP)
						samplerBindingsWriter.AddStaticImage(tr.whiteImage, i);
					else
						samplerBindingsWriter.AddAnimatedImage(&pStage->bundle[i], i);
				}
			}
			else
			{
				qboolean light = (qboolean)((pStage->glslShaderIndex & LIGHTDEF_LIGHTTYPE_MASK) != 0);
				qboolean allowVertexLighting = (qboolean)!(r_normalMapping->integer || r_specularMapping->integer);

				if (pStage->bundle[TB_DIFFUSEMAP].image[0])
					samplerBindingsWriter.AddAnimatedImage(&pStage->bundle[TB_DIFFUSEMAP], TB_DIFFUSEMAP);

				if (pStage->bundle[TB_LIGHTMAP].image[0])
					samplerBindingsWriter.AddAnimatedImage(&pStage->bundle[TB_LIGHTMAP], TB_LIGHTMAP);

				// bind textures that are sampled and used in the glsl shader, and
				// bind whiteImage to textures that are sampled but zeroed in the glsl shader
				//
				// alternatives:
				//  - use the last bound texture
				//     -> costs more to sample a higher res texture then throw out the result
				//  - disable texture sampling in glsl shader with #ifdefs, as before
				//     -> increases the number of shaders that must be compiled
				//
				if (light && !allowVertexLighting)
				{
					if (pStage->bundle[TB_NORMALMAP].image[0])
					{
						samplerBindingsWriter.AddAnimatedImage(&pStage->bundle[TB_NORMALMAP], TB_NORMALMAP);
						enableTextures[0] = 1.0f;
					}
					else if (r_normalMapping->integer)
					{
						samplerBindingsWriter.AddStaticImage(tr.whiteImage, TB_NORMALMAP);
					}

					if (pStage->bundle[TB_DELUXEMAP].image[0])
					{
						samplerBindingsWriter.AddAnimatedImage(&pStage->bundle[TB_DELUXEMAP], TB_DELUXEMAP);
						enableTextures[1] = 1.0f;
					}
					else if (r_deluxeMapping->integer)
					{
						samplerBindingsWriter.AddStaticImage(tr.whiteImage, TB_DELUXEMAP);
					}

					if (pStage->bundle[TB_SPECULARMAP].image[0])
					{
						samplerBindingsWriter.AddAnimatedImage(&pStage->bundle[TB_SPECULARMAP], TB_SPECULARMAP);
					}
					else if (r_specularMapping->integer)
					{
						samplerBindingsWriter.AddStaticImage(tr.whiteImage, TB_SPECULARMAP);
					}

					if (r_ssao->integer && tr.world && backEnd.framePostProcessed == qfalse)
						samplerBindingsWriter.AddStaticImage(tr.screenSsaoImage, TB_SSAOMAP);
					else if (r_ssao->integer)
						samplerBindingsWriter.AddStaticImage(tr.whiteImage, TB_SSAOMAP);

				}

				if ( enableCubeMaps )
				{
					enableTextures[3] =  1.0f;
				}
			}

			uniformDataWriter.SetUniformVec4(UNIFORM_ENABLETEXTURES, enableTextures);
		}
		else if ( pStage->bundle[1].image[0] != 0 )
		{
			samplerBindingsWriter.AddAnimatedImage(&pStage->bundle[0], 0);
			samplerBindingsWriter.AddAnimatedImage(&pStage->bundle[1], 1);
		}
		else
		{
			//
			// set state
			//
			samplerBindingsWriter.AddAnimatedImage(&pStage->bundle[0], 0);
		}

		//
		// testing cube map
		//
		if ( enableCubeMaps )
		{
			vec4_t vec;
			cubemap_t *cubemap = &tr.cubemaps[input->cubemapIndex - 1];

			samplerBindingsWriter.AddStaticImage(cubemap->image, TB_CUBEMAP);
			samplerBindingsWriter.AddStaticImage(tr.envBrdfImage, TB_ENVBRDFMAP);

			VectorSubtract(cubemap->origin, backEnd.viewParms.ori.origin, vec);
			vec[3] = 1.0f;

			VectorScale4(vec, 1.0f / cubemap->parallaxRadius, vec);

			uniformDataWriter.SetUniformVec4(UNIFORM_CUBEMAPINFO, vec);
		}

		if (enableDLights)
		{
			uniformDataWriter.SetUniformInt(UNIFORM_LIGHTMASK, tess.dlightBits);
			if (r_dlightMode->integer > 1)
				samplerBindingsWriter.AddStaticImage(tr.pointShadowArrayImage, TB_SHADOWMAPARRAY);
		}
		else
			uniformDataWriter.SetUniformInt(UNIFORM_LIGHTMASK, 0);

		CaptureDrawData(input, pStage, index, stage);

		const UniformBlockBinding uniformBlockBindings[] = {
			GetCameraBlockUniformBinding(backEnd.currentEntity),
			GetLightsBlockUniformBinding(),
			GetSceneBlockUniformBinding(),
			GetFogsBlockUniformBinding(),
			GetEntityBlockUniformBinding(backEnd.currentEntity),
			GetShaderInstanceBlockUniformBinding(
				backEnd.currentEntity, input->shader),
			GetBonesBlockUniformBinding(backEnd.currentEntity)
		};

		DrawItem item = {};
		item.renderState.stateBits = stateBits;
		item.renderState.cullType = forceRefraction ? CT_TWO_SIDED : cullType;
		item.renderState.depthRange = RB_GetDepthRange(backEnd.currentEntity, input->shader);
		item.program = sp;
		item.ibo = input->externalIBO ? input->externalIBO : backEndData->currentFrame->dynamicIbo;
		item.uniformData = uniformDataWriter.Finish(frameAllocator);
		item.samplerBindings = samplerBindingsWriter.Finish(
			frameAllocator, &item.numSamplerBindings);

		DrawItemSetVertexAttributes(
			item, attribs, vertexArrays->numVertexArrays, frameAllocator);
		DrawItemSetUniformBlockBindings(
			item, uniformBlockBindings, frameAllocator);

		RB_FillDrawCommand(item.draw, GL_TRIANGLES, 1, input);

		uint32_t key = RB_CreateSortKey(item, stage, input->shader->sort);

		RB_AddDrawItem(backEndData->currentPass, key, item);

		// allow skipping out to show just lightmaps during development
		if ( r_lightmap->integer && ( pStage->bundle[0].isLightmap || pStage->bundle[1].isLightmap ) )
		{
			break;
		}

		if (backEnd.depthFill)
			break;
	}
}

/*
** RB_StageIteratorGeneric
*/
void RB_StageIteratorGeneric( void )
{
	shaderCommands_t *input = &tess;
	if (!input->numVertexes || !input->numIndexes)
	{
		return;
	}

	//
	// log this call
	//
	if ( r_logFile->integer )
	{
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		GLimp_LogComment( va("--- RB_StageIteratorGeneric( %s ) ---\n", tess.shader->name) );
	}

	//
	// update vertex buffer data
	//
	uint32_t vertexAttribs = RB_CalcShaderVertexAttribs( input->shader );
	if (tess.useInternalVBO)
	{
		RB_DeformTessGeometry();
		RB_UpdateVBOs(vertexAttribs);
	}
	else
	{
		backEnd.pc.c_staticVboDraws++;
	}

	//
	// vertex arrays
	//
	VertexArraysProperties vertexArrays;
	if ( tess.useInternalVBO )
	{
		CalculateVertexArraysProperties(vertexAttribs, &vertexArrays);
		for ( int i = 0; i < vertexArrays.numVertexArrays; i++ )
		{
			int attributeIndex = vertexArrays.enabledAttributes[i];
			vertexArrays.offsets[attributeIndex] += backEndData->currentFrame->dynamicVboCommitOffset;
		}
	}
	else
	{
		CalculateVertexArraysFromVBO(vertexAttribs, glState.currentVBO, &vertexArrays);
	}

	if ( backEnd.depthFill )
	{
		RB_IterateStagesGeneric( input, &vertexArrays );
	}
	else
	{
		RB_IterateStagesGeneric( input, &vertexArrays );

		//
		// pshadows!
		//
		if (r_shadows->integer == 4 &&
				tess.pshadowBits &&
				tess.shader->sort <= SS_OPAQUE &&
				!(tess.shader->surfaceFlags & (SURF_NODLIGHT | SURF_SKY)))
		{
			ProjectPshadowVBOGLSL( input, &vertexArrays );
		}

		//
		// volumeshadows!
		//
		if (glState.genShadows && r_shadows->integer == 2)
		{
			RB_ShadowTessEnd( input, &vertexArrays );
		}

		//
		// now do fog
		//
		const fog_t *fog = nullptr;
		if ( tr.world && input->fogNum != 0)
		{
			fog = tr.world->fogs + input->fogNum;
		}
		if (fog && tess.shader->fogPass && r_drawfog->integer)
			RB_FogPass(input, &vertexArrays);

		//
		// draw debugging stuff
		//
		if ( r_showtris->integer ) {
			DrawTris( input, &vertexArrays );
		}
		if ( r_shownormals->integer ) {
			DrawNormals( input );
		}
	}

	RB_CommitInternalBufferData();
}

void RB_BinTriangleCounts( void )
{
	int numTriangles = tess.numIndexes / 3;
	if ( numTriangles < 20 )
		backEnd.pc.c_triangleCountBins[TRI_BIN_0_19]++;
	else if ( numTriangles < 50 )
		backEnd.pc.c_triangleCountBins[TRI_BIN_20_49]++;
	else if ( numTriangles < 100 )
		backEnd.pc.c_triangleCountBins[TRI_BIN_50_99]++;
	else if ( numTriangles < 300 )
		backEnd.pc.c_triangleCountBins[TRI_BIN_100_299]++;
	else if ( numTriangles < 600 )
		backEnd.pc.c_triangleCountBins[TRI_BIN_300_599]++;
	else if ( numTriangles < 1000 )
		backEnd.pc.c_triangleCountBins[TRI_BIN_1000_1499]++;
	else if ( numTriangles < 1500 )
		backEnd.pc.c_triangleCountBins[TRI_BIN_1500_1999]++;
	else if ( numTriangles < 2000 )
		backEnd.pc.c_triangleCountBins[TRI_BIN_2000_2999]++;
	else
		backEnd.pc.c_triangleCountBins[TRI_BIN_3000_PLUS]++;
}

/*
** RB_EndSurface
*/
void RB_EndSurface( void ) {
	shaderCommands_t *input;

	input = &tess;

	if (input->numIndexes == 0 || input->numVertexes == 0) {
		return;
	}

	if (input->indexes[SHADER_MAX_INDEXES-1] != 0) {
		ri.Error (ERR_DROP, "RB_EndSurface() - SHADER_MAX_INDEXES hit");
	}
	if (input->xyz[SHADER_MAX_VERTEXES-1][0] != 0) {
		ri.Error (ERR_DROP, "RB_EndSurface() - SHADER_MAX_VERTEXES hit");
	}

	// for debugging of sort order issues, stop rendering after a given sort value
	if ( r_debugSort->integer && r_debugSort->integer < tess.shader->sort ) {
		return;
	}

	if (tr.world && !backEnd.framePostProcessed) {
		if (tr.world->skyboxportal)
		{
			// world
			if (!(tr.viewParms.isSkyPortal) && (tess.currentStageIteratorFunc == RB_StageIteratorSky))
			{	// don't process these tris at all
				return;
			}
			// portal sky
			else if (!(backEnd.refdef.rdflags & RDF_DRAWSKYBOX) && (tess.currentStageIteratorFunc != RB_StageIteratorSky))
			{	// /only/ process sky tris
				return;
			}
		}
	}

	//
	// update performance counters
	//
	backEnd.pc.c_shaders++;
	backEnd.pc.c_vertexes += tess.numVertexes;
	backEnd.pc.c_indexes += tess.numIndexes;
	backEnd.pc.c_totalIndexes += tess.numIndexes * tess.numPasses;

	RB_BinTriangleCounts();

	//
	// call off to shader specific tess end function
	//
	tess.currentStageIteratorFunc();

	// clear shader so we can tell we don't have any unclosed surfaces
	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.firstIndex = 0;
	tess.multiDrawPrimitives = 0;
	tess.externalIBO = nullptr;
	glState.vertexAnimation = qfalse;
	glState.skeletalAnimation = qfalse;
	glState.genShadows = qfalse;

	GLimp_LogComment( "----------\n" );
}
