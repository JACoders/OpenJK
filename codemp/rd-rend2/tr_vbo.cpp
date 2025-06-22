/*
===========================================================================
Copyright (C) 2007-2009 Robert Beckebans <trebor_7@users.sourceforge.net>

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
// tr_vbo.c
#include "tr_local.h"

#ifdef _G2_GORE
#include "G2_gore_r2.h"
#endif


uint32_t R_VboPackTangent(vec4_t v)
{
	return (((uint32_t)(v[3] * 1.5f   + 2.0f  )) << 30)
		    | (((uint32_t)(v[2] * 511.5f + 512.0f)) << 20)
		    | (((uint32_t)(v[1] * 511.5f + 512.0f)) << 10)
		    | (((uint32_t)(v[0] * 511.5f + 512.0f)));
}

uint32_t R_VboPackNormal(vec3_t v)
{
	return (((uint32_t)(v[2] * 511.5f + 512.0f)) << 20)
		    | (((uint32_t)(v[1] * 511.5f + 512.0f)) << 10)
		    | (((uint32_t)(v[0] * 511.5f + 512.0f)));
}

void R_VboUnpackTangent(vec4_t v, uint32_t b)
{
	v[0] = ((b)       & 0x3ff) * 1.0f/511.5f - 1.0f;
	v[1] = ((b >> 10) & 0x3ff) * 1.0f/511.5f - 1.0f;
	v[2] = ((b >> 20) & 0x3ff) * 1.0f/511.5f - 1.0f;
	v[3] = ((b >> 30) & 0x3)   * 1.0f/1.5f   - 1.0f;
}

void R_VboUnpackNormal(vec3_t v, uint32_t b)
{
	v[0] = ((b)       & 0x3ff) * 1.0f/511.5f - 1.0f;
	v[1] = ((b >> 10) & 0x3ff) * 1.0f/511.5f - 1.0f;
	v[2] = ((b >> 20) & 0x3ff) * 1.0f/511.5f - 1.0f;
}

static GLenum GetGLBufferUsage ( vboUsage_t usage )
{
	switch (usage)
	{
		case VBO_USAGE_STATIC:
			return GL_STATIC_DRAW;

		case VBO_USAGE_DYNAMIC:
			return GL_STREAM_DRAW;

		case VBO_USAGE_XFB:
			return GL_STREAM_COPY;

		default:
			ri.Error (ERR_FATAL, "bad vboUsage_t given: %i", usage);
			return GL_INVALID_OPERATION;
	}
}

/*
============
R_CreateVBO
============
*/
VBO_t *R_CreateVBO(byte * vertexes, int vertexesSize, vboUsage_t usage)
{
	VBO_t          *vbo;

	if ( tr.numVBOs == MAX_VBOS ) {
		ri.Error( ERR_DROP, "R_CreateVBO: MAX_VBOS hit");
	}

	R_IssuePendingRenderCommands();

	vbo = tr.vbos[tr.numVBOs] = (VBO_t *)ri.Hunk_Alloc(sizeof(*vbo), h_low);

	memset(vbo, 0, sizeof(*vbo));

	vbo->vertexesSize = vertexesSize;
	qglGenBuffers(1, &vbo->vertexesVBO);
	tr.numVBOs++;

	qglBindBuffer(GL_ARRAY_BUFFER, vbo->vertexesVBO);
	if ( glRefConfig.immutableBuffers )
	{
		GLbitfield creationFlags = 0;
		if ( usage == VBO_USAGE_DYNAMIC )
		{
			creationFlags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
		}

		qglBufferStorage(GL_ARRAY_BUFFER, vertexesSize, vertexes, creationFlags);
	}
	else
	{
		int glUsage = GetGLBufferUsage (usage);
		qglBufferData(GL_ARRAY_BUFFER, vertexesSize, vertexes, glUsage);
	}

	qglBindBuffer(GL_ARRAY_BUFFER, 0);

	glState.currentVBO = NULL;

	GL_CheckErrors();

	return vbo;
}

/*
============
R_CreateIBO
============
*/
IBO_t *R_CreateIBO(byte * indexes, int indexesSize, vboUsage_t usage)
{
	IBO_t          *ibo;

	if ( tr.numIBOs == MAX_IBOS ) {
		ri.Error( ERR_DROP, "R_CreateIBO: MAX_IBOS hit");
	}

	R_IssuePendingRenderCommands();

	ibo = tr.ibos[tr.numIBOs] = (IBO_t *)ri.Hunk_Alloc(sizeof(*ibo), h_low);

	ibo->indexesSize = indexesSize;
	qglGenBuffers(1, &ibo->indexesVBO);
	tr.numIBOs++;

	qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo->indexesVBO);
	if ( glRefConfig.immutableBuffers )
	{
		GLbitfield creationFlags = 0;
		if ( usage == VBO_USAGE_DYNAMIC )
		{
			creationFlags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
		}

		qglBufferStorage(GL_ELEMENT_ARRAY_BUFFER, indexesSize, indexes, creationFlags);
		GL_CheckErrors();
	}
	else
	{
		int glUsage = GetGLBufferUsage (usage);
		qglBufferData(GL_ELEMENT_ARRAY_BUFFER, indexesSize, indexes, glUsage);
	}

	qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glState.currentIBO = NULL;

	GL_CheckErrors();

	return ibo;
}

/*
============
R_BindVBO
============
*/
void R_BindVBO(VBO_t * vbo)
{
	if(!vbo)
	{
		//R_BindNullVBO();
		ri.Error(ERR_DROP, "R_BindVBO: NULL vbo");
		return;
	}

	if(r_logFile->integer)
	{
		GLimp_LogComment("--- R_BindVBO() ---\n");
	}

	if(glState.currentVBO != vbo)
	{
		glState.currentVBO = vbo;

		glState.vertexAttribsInterpolation = 0;
		glState.vertexAttribsOldFrame = 0;
		glState.vertexAttribsNewFrame = 0;
		glState.vertexAttribsTexCoordOffset[0] = 0;
		glState.vertexAttribsTexCoordOffset[1] = 1;
		glState.vertexAnimation = qfalse;
		glState.skeletalAnimation = qfalse;

		qglBindBuffer(GL_ARRAY_BUFFER, vbo->vertexesVBO);

		backEnd.pc.c_vboVertexBuffers++;
	}
}

/*
============
R_BindNullVBO
============
*/
void R_BindNullVBO(void)
{
	GLimp_LogComment("--- R_BindNullVBO ---\n");

	if(glState.currentVBO)
	{
		qglBindBuffer(GL_ARRAY_BUFFER, 0);
		glState.currentVBO = NULL;
	}

	GL_CheckErrors();
}

/*
============
R_BindIBO
============
*/
void R_BindIBO(IBO_t * ibo)
{
	if(!ibo)
	{
		//R_BindNullIBO();
		ri.Error(ERR_DROP, "R_BindIBO: NULL ibo");
		return;
	}

	if(r_logFile->integer)
	{
		GLimp_LogComment("--- R_BindIBO() ---\n");
	}

	if(glState.currentIBO != ibo)
	{
		qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo->indexesVBO);

		glState.currentIBO = ibo;

		backEnd.pc.c_vboIndexBuffers++;
	}
}

/*
============
R_BindNullIBO
============
*/
void R_BindNullIBO(void)
{
	GLimp_LogComment("--- R_BindNullIBO ---\n");

	if(glState.currentIBO)
	{
		qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glState.currentIBO = NULL;
	}
}

/*
============
R_InitGPUBuffers
============
*/
void R_InitGPUBuffers(void)
{
	ri.Printf(PRINT_ALL, "------- R_InitGPUBuffers -------\n");

	// glGenBuffers only allocates the IDs for these buffers. The 'buffer object' is
	// actually created on first bind.
	qglGenBuffers(1, &tr.staticUbo);
	qglGenBuffers(MAX_SUB_BSP + 1, tr.spriteUbos);
	qglGenBuffers(1, &tr.shaderInstanceUbo);

	// Allocate/create ShaderInstanceBlock ubo
	const int alignment = glRefConfig.uniformBufferOffsetAlignment - 1;
	const size_t alignedBlockSize = (sizeof(ShaderInstanceBlock) + alignment) & ~alignment;

	qglBindBuffer(GL_UNIFORM_BUFFER, tr.shaderInstanceUbo);
	glState.currentGlobalUBO = tr.shaderInstanceUbo;
	qglBufferData(
		GL_UNIFORM_BUFFER,
		MAX_SHADERS * alignedBlockSize,
		nullptr,
		GL_STATIC_DRAW);

	tr.numVBOs = 0;
	tr.numIBOs = 0;

	R_BindNullVBO();
	R_BindNullIBO();

	GL_CheckErrors();
}

/*
============
R_DestroyGPUBuffers
============
*/
void R_DestroyGPUBuffers(void)
{
	ri.Printf(PRINT_ALL, "------- R_DestroyGPUBuffers -------\n");

	R_BindNullVBO();
	R_BindNullIBO();

	qglDeleteBuffers(1, &tr.staticUbo);
	qglDeleteBuffers(MAX_SUB_BSP + 1, tr.spriteUbos);
	qglDeleteBuffers(1, &tr.shaderInstanceUbo);

	for (int i = 0; i < tr.numVBOs; i++)
	{
		VBO_t *vbo = tr.vbos[i];

		if (vbo->vertexesVBO)
		{
			qglDeleteBuffers(1, &vbo->vertexesVBO);
		}
	}

	for (int i = 0; i < tr.numIBOs; i++)
	{
		IBO_t *ibo = tr.ibos[i];

		if (ibo->indexesVBO)
		{
			qglDeleteBuffers(1, &ibo->indexesVBO);
		}
	}

	tr.numVBOs = 0;
	tr.numIBOs = 0;
}

/*
============
R_VBOList_f
============
*/
void R_VBOList_f(void)
{
	int             i;
	VBO_t          *vbo;
	IBO_t          *ibo;
	int             vertexesSize = 0;
	int             indexesSize = 0;

	ri.Printf (PRINT_ALL, " vertex buffers\n");
	ri.Printf (PRINT_ALL, "----------------\n\n");

	ri.Printf(PRINT_ALL, " id   size (MB)\n");
	ri.Printf(PRINT_ALL, "---------------\n");

	for(i = 0; i < tr.numVBOs; i++)
	{
		vbo = tr.vbos[i];

		ri.Printf(PRINT_ALL, " %4i %4.2f\n", i, vbo->vertexesSize / (1024.0f * 1024.0f));

		vertexesSize += vbo->vertexesSize;
	}

	ri.Printf(PRINT_ALL, " %d total buffers\n", tr.numVBOs);
	ri.Printf(PRINT_ALL, " %.2f MB in total\n\n", vertexesSize / (1024.0f * 1024.0f));


	ri.Printf (PRINT_ALL, " index buffers\n");
	ri.Printf (PRINT_ALL, "---------------\n\n");

	ri.Printf(PRINT_ALL, " id   size (MB)\n");
	ri.Printf(PRINT_ALL, "---------------\n");

	for(i = 0; i < tr.numIBOs; i++)
	{
		ibo = tr.ibos[i];

		ri.Printf(PRINT_ALL, " %4i %4.2f\n", i, ibo->indexesSize / (1024.0f * 1024.0f));

		indexesSize += ibo->indexesSize;
	}

	ri.Printf(PRINT_ALL, " %d total buffers\n", tr.numIBOs);
	ri.Printf(PRINT_ALL, " %.2f MB in total\n\n", indexesSize / (1024.0f * 1024.0f));
}

void AddVertexArray(
	VertexArraysProperties *properties,
	int attributeIndex,
	size_t size,
	int stride,
	int offset,
	void *stream,
	int streamStride)
{
	properties->enabledAttributes[properties->numVertexArrays]  = attributeIndex;
	properties->offsets[attributeIndex]                         = offset;
	properties->vertexDataSize                                 += size;
	properties->sizes[attributeIndex]                           = size;
	properties->strides[attributeIndex]                         = stride;
	properties->streams[attributeIndex]                         = stream;
	properties->streamStrides[attributeIndex]					= streamStride;

	properties->numVertexArrays++;
}

void CalculateVertexArraysProperties(uint32_t attributes, VertexArraysProperties *properties)
{
	properties->vertexDataSize = 0;
	properties->numVertexArrays = 0;

	if (!attributes)
	{
		attributes =
			ATTR_POSITION |
			ATTR_TEXCOORD0 |
			ATTR_TEXCOORD1 |
			ATTR_NORMAL |
			ATTR_TANGENT |
			ATTR_COLOR |
			ATTR_LIGHTDIRECTION;
	}

	if (attributes & ATTR_BITS)
	{
		if (attributes & ATTR_POSITION)
			AddVertexArray(
				properties,
				ATTR_INDEX_POSITION,
				sizeof(tess.xyz[0]),
				0,
				properties->vertexDataSize,
				tess.xyz,
				sizeof(tess.xyz[0]));

		if (attributes & ATTR_TEXCOORD0)
			AddVertexArray(
				properties,
				ATTR_INDEX_TEXCOORD0,
				sizeof(tess.texCoords[0][0]),
				0,
				properties->vertexDataSize,
				tess.texCoords[0][0],
				sizeof(tess.texCoords[0][0]) * NUM_TESS_TEXCOORDS);

		if (attributes & ATTR_TEXCOORD1)
			AddVertexArray(
				properties,
				ATTR_INDEX_TEXCOORD1,
				sizeof(tess.texCoords[0][1]),
				0,
				properties->vertexDataSize,
				tess.texCoords[0][1],
				sizeof(tess.texCoords[0][0]) * NUM_TESS_TEXCOORDS);

		if (attributes & ATTR_TEXCOORD2)
			AddVertexArray(
				properties,
				ATTR_INDEX_TEXCOORD2,
				sizeof(tess.texCoords[0][2]) * 2,
				0,
				properties->vertexDataSize,
				tess.texCoords[0][2],
				sizeof(tess.texCoords[0][0]) * NUM_TESS_TEXCOORDS);
;

		if (attributes & ATTR_TEXCOORD3)
			AddVertexArray(
				properties,
				ATTR_INDEX_TEXCOORD3,
				sizeof(tess.texCoords[0][3]) * 2,
				0,
				properties->vertexDataSize,
				tess.texCoords[0][3],
				sizeof(tess.texCoords[0][0]) * NUM_TESS_TEXCOORDS);


		if (attributes & ATTR_TEXCOORD4)
			AddVertexArray(
				properties,
				ATTR_INDEX_TEXCOORD4,
				sizeof(tess.texCoords[0][4]) * 2,
				0,
				properties->vertexDataSize,
				tess.texCoords[0][4],
				sizeof(tess.texCoords[0][0]) * NUM_TESS_TEXCOORDS);

		if (attributes & ATTR_NORMAL)
			AddVertexArray(
				properties,
				ATTR_INDEX_NORMAL,
				sizeof(tess.normal[0]),
				0,
				properties->vertexDataSize,
				tess.normal,
				sizeof(tess.normal[0]));

		if (attributes & ATTR_TANGENT)
			AddVertexArray(
				properties,
				ATTR_INDEX_TANGENT,
				sizeof(tess.tangent[0]),
				0,
				properties->vertexDataSize,
				tess.tangent,
				sizeof(tess.tangent[0]));

		if (attributes & ATTR_COLOR)
			AddVertexArray(
				properties,
				ATTR_INDEX_COLOR,
				sizeof(tess.vertexColors[0]),
				0,
				properties->vertexDataSize,
				tess.vertexColors,
				sizeof(tess.vertexColors[0]));

		if (attributes & ATTR_LIGHTDIRECTION)
			AddVertexArray(
				properties,
				ATTR_INDEX_LIGHTDIRECTION,
				sizeof(tess.lightdir[0]),
				0,
				properties->vertexDataSize,
				tess.lightdir,
				sizeof(tess.lightdir[0]));
	}

	for ( int i = 0; i < properties->numVertexArrays; i++ )
		properties->strides[properties->enabledAttributes[i]] = properties->vertexDataSize;
}

void CalculateVertexArraysFromVBO(
	uint32_t attributes,
	const VBO_t *vbo,
	VertexArraysProperties *properties)
{
	properties->vertexDataSize = 0;
	properties->numVertexArrays = 0;

	for (int i = 0, j = 1; i < ATTR_INDEX_MAX; i++, j <<= 1)
	{
		if (vbo->sizes[i] == 0)
			continue;
		if (attributes & j)
			AddVertexArray(
				properties,
				i,
				vbo->sizes[i],
				vbo->strides[i],
				vbo->offsets[i],
				NULL,
				0);
	}
}

/*
==============
RB_UpdateVBOs

Adapted from Tess_UpdateVBOs from xreal

Update the default VBO to replace the client side vertex arrays
==============
*/
void RB_UpdateVBOs(unsigned int attribBits)
{
	gpuFrame_t *currentFrame = backEndData->currentFrame;

	GLimp_LogComment("--- RB_UpdateVBOs ---\n");

	backEnd.pc.c_dynamicVboDraws++;

	// update the default VBO
	if (tess.numVertexes > 0 && tess.numVertexes <= SHADER_MAX_VERTEXES)
	{
		VBO_t *frameVbo = currentFrame->dynamicVbo;
		GLbitfield mapFlags = GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT;
		VertexArraysProperties vertexArrays = {};
		CalculateVertexArraysProperties(attribBits, &vertexArrays);

		int totalVertexDataSize = tess.numVertexes * vertexArrays.vertexDataSize;
		backEnd.pc.c_dynamicVboTotalSize += totalVertexDataSize;

		if ( (currentFrame->dynamicVboWriteOffset + totalVertexDataSize) > frameVbo->vertexesSize )
		{
			// TODO: Eh...resize?
			assert(!"This shouldn't happen");
			return;
		}

		R_BindVBO(frameVbo);

		void *dstPtr;
		if ( glRefConfig.immutableBuffers )
		{
			dstPtr = (byte *)currentFrame->dynamicVboMemory + currentFrame->dynamicVboWriteOffset;
		}
		else
		{
			dstPtr = qglMapBufferRange(GL_ARRAY_BUFFER, currentFrame->dynamicVboWriteOffset,
				totalVertexDataSize, mapFlags);
		}

		// Interleave the data
		void *writePtr = dstPtr;
		for ( int i = 0; i < tess.numVertexes; i++ )
		{
			for ( int j = 0; j < vertexArrays.numVertexArrays; j++ )
			{
				const int attributeIndex = vertexArrays.enabledAttributes[j];
				const size_t attribSize = vertexArrays.sizes[attributeIndex];
				const int streamStride = vertexArrays.streamStrides[attributeIndex];
				void *stream = vertexArrays.streams[attributeIndex];

				memcpy(writePtr, (byte *)stream + i * streamStride, attribSize);
				writePtr = (byte *)writePtr + attribSize;
			}
		}

		if ( !glRefConfig.immutableBuffers )
		{
			qglUnmapBuffer(GL_ARRAY_BUFFER);
		}

		currentFrame->dynamicVboWriteOffset += totalVertexDataSize;
	}

	// update the default IBO
	if(tess.numIndexes > 0 && tess.numIndexes <= SHADER_MAX_INDEXES)
	{
		IBO_t *frameIbo = currentFrame->dynamicIbo;
		GLbitfield mapFlags = GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT;
		int totalIndexDataSize = tess.numIndexes * sizeof(tess.indexes[0]);

		R_BindIBO(frameIbo);

		if ( (currentFrame->dynamicIboWriteOffset + totalIndexDataSize) > frameIbo->indexesSize )
		{
			// TODO: Resize the buffer?
			assert(!"This shouldn't happen");
			return;
		}

		void *dst;
		if ( glRefConfig.immutableBuffers )
		{
			dst = (byte *)currentFrame->dynamicIboMemory + currentFrame->dynamicIboWriteOffset;
		}
		else
		{
			dst = qglMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, currentFrame->dynamicIboWriteOffset,
				totalIndexDataSize, mapFlags);
		}

		memcpy(dst, tess.indexes, totalIndexDataSize);

		if ( !glRefConfig.immutableBuffers )
		{
			qglUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
		}

		currentFrame->dynamicIboWriteOffset += totalIndexDataSize;
	}
}

#ifdef _G2_GORE
void RB_UpdateGoreVBO(srfG2GoreSurface_t *goreSurface)
{
	goreSurface->firstVert = tr.goreVBOCurrentIndex;
	goreSurface->firstIndex = tr.goreIBOCurrentIndex;

	if (tr.goreVBOCurrentIndex + goreSurface->numVerts >= (MAX_LODS * MAX_GORE_RECORDS * MAX_GORE_VERTS * MAX_FRAMES))
		tr.goreVBOCurrentIndex = 0;

	R_BindVBO(tr.goreVBO);
	qglBufferSubData(
		GL_ARRAY_BUFFER,
		sizeof(g2GoreVert_t) * tr.goreVBOCurrentIndex,
		sizeof(g2GoreVert_t) * goreSurface->numVerts,
		goreSurface->verts
	);
	tr.goreVBOCurrentIndex += goreSurface->numVerts;

	if (tr.goreIBOCurrentIndex + goreSurface->numVerts >= (MAX_LODS * MAX_GORE_RECORDS * MAX_GORE_INDECIES * MAX_FRAMES))
		tr.goreIBOCurrentIndex = 0;

	R_BindIBO(tr.goreIBO);
	qglBufferSubData(
		GL_ELEMENT_ARRAY_BUFFER,
		sizeof(glIndex_t) * tr.goreIBOCurrentIndex,
		sizeof(glIndex_t) * goreSurface->numIndexes,
		goreSurface->indexes
	);
	tr.goreIBOCurrentIndex += goreSurface->numIndexes;
}
#endif

void RB_CommitInternalBufferData()
{
	gpuFrame_t *currentFrame = backEndData->currentFrame;

	currentFrame->dynamicIboCommitOffset = currentFrame->dynamicIboWriteOffset;
	currentFrame->dynamicVboCommitOffset = currentFrame->dynamicVboWriteOffset;
}

void RB_BindUniformBlock(GLuint ubo, uniformBlock_t block, int offset)
{
	const uniformBlockInfo_t *blockInfo = uniformBlocksInfo + block;

	assert(blockInfo->slot < MAX_UBO_BINDINGS);

	bufferBinding_t *currentBinding = glState.currentUBOs + blockInfo->slot;
	if (currentBinding->buffer != ubo ||
		currentBinding->offset != offset ||
		currentBinding->size != blockInfo->size)
	{
		qglBindBufferRange(
			GL_UNIFORM_BUFFER, blockInfo->slot, ubo, offset, blockInfo->size);
		glState.currentGlobalUBO = ubo;

		currentBinding->buffer = ubo;
		currentBinding->offset = offset;
		currentBinding->size = blockInfo->size;
	}
}

int RB_BindAndUpdateFrameUniformBlock(uniformBlock_t block, void *data)
{
	const uniformBlockInfo_t *blockInfo = uniformBlocksInfo + block;
	gpuFrame_t *thisFrame = backEndData->currentFrame;
	const int offset = thisFrame->uboWriteOffset;

	RB_BindUniformBlock(thisFrame->ubo, block, offset);

	qglBufferSubData(GL_UNIFORM_BUFFER,
			thisFrame->uboWriteOffset, blockInfo->size, data);

	const int alignment = glRefConfig.uniformBufferOffsetAlignment - 1;
	const size_t alignedBlockSize = (blockInfo->size + alignment) & ~alignment;
	thisFrame->uboWriteOffset += alignedBlockSize;

	return offset;
}

int RB_AddShaderInstanceBlock(void *data)
{
	if (glState.currentGlobalUBO != tr.shaderInstanceUbo)
	{
		qglBindBuffer(GL_UNIFORM_BUFFER, tr.shaderInstanceUbo);
		glState.currentGlobalUBO = tr.shaderInstanceUbo;
	}
	const size_t writeOffset = tr.shaderInstanceUboWriteOffset;

	qglBufferSubData(GL_UNIFORM_BUFFER,
		tr.shaderInstanceUboWriteOffset, sizeof(ShaderInstanceBlock), data);

	const int alignment = glRefConfig.uniformBufferOffsetAlignment - 1;
	const size_t alignedBlockSize = (sizeof(ShaderInstanceBlock) + alignment) & ~alignment;
	tr.shaderInstanceUboWriteOffset += alignedBlockSize;

	return writeOffset;
}

void RB_BeginConstantsUpdate(gpuFrame_t *frame)
{
	if (glState.currentGlobalUBO != frame->ubo)
	{
		qglBindBuffer(GL_UNIFORM_BUFFER, frame->ubo);
		glState.currentGlobalUBO = frame->ubo;
	}

	const GLbitfield mapFlags =
		GL_MAP_WRITE_BIT |
		GL_MAP_UNSYNCHRONIZED_BIT |
		GL_MAP_FLUSH_EXPLICIT_BIT;

	frame->uboMapBase = frame->uboWriteOffset;
	frame->uboMemory = qglMapBufferRange(
		GL_UNIFORM_BUFFER,
		frame->uboWriteOffset,
		frame->uboSize - frame->uboWriteOffset,
		mapFlags);
}

int RB_AppendConstantsData(
	gpuFrame_t *frame, const void *data, size_t dataSize)
{
	const size_t writeOffset = frame->uboWriteOffset;
	const size_t relativeOffset = writeOffset - frame->uboMapBase;

	memcpy((char *)frame->uboMemory + relativeOffset, data, dataSize);

	const int alignment = glRefConfig.uniformBufferOffsetAlignment - 1;
	const size_t alignedBlockSize = (dataSize + alignment) & ~alignment;

	frame->uboWriteOffset += alignedBlockSize;
	assert(frame->uboWriteOffset > 0);
	return writeOffset;
}

void RB_EndConstantsUpdate(const gpuFrame_t *frame)
{
	qglFlushMappedBufferRange(
		GL_UNIFORM_BUFFER,
		frame->uboMapBase,
		frame->uboWriteOffset - frame->uboMapBase);
	qglUnmapBuffer(GL_UNIFORM_BUFFER);
}