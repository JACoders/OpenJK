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

		default:
			ri->Error (ERR_FATAL, "bad vboUsage_t given: %i", usage);
			return GL_INVALID_OPERATION;
	}
}

#if 0
struct Attribute
{
	int index;
	GLenum type;
	int numElements;
	qboolean normalised;
	int offset;
	int stride;
	int stream;
};

const int MAX_ATTRIBUTES = 8;
struct VertexFormat
{
	Attribute attributes[MAX_ATTRIBUTES];
	int numAttributes;
};

const int MAX_VERTEX_STREAMS = 2;
struct VertexArrayObject
{
	GLuint vao;
	IBO_t *ibo;
	VBO_t *vbos[MAX_VERTEX_STREAMS];
	int numStreams;
	VertexFormat format;
};

VertexArrayObject *R_GetVertexArrayObject( const VertexFormat& format )
{
	return nullptr;
}
#endif

/*
============
R_CreateVBO
============
*/
VBO_t *R_CreateVBO(byte * vertexes, int vertexesSize, vboUsage_t usage)
{
	VBO_t          *vbo;

	if ( tr.numVBOs == MAX_VBOS ) {
		ri->Error( ERR_DROP, "R_CreateVBO: MAX_VBOS hit");
	}

	R_IssuePendingRenderCommands();

	vbo = tr.vbos[tr.numVBOs] = (VBO_t *)ri->Hunk_Alloc(sizeof(*vbo), h_low);

	memset(vbo, 0, sizeof(*vbo));

	vbo->vertexesSize = vertexesSize;
	vbo->vertexesVBO = tr.vboNames[tr.numVBOs];
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
		ri->Error( ERR_DROP, "R_CreateIBO: MAX_IBOS hit");
	}

	R_IssuePendingRenderCommands();

	ibo = tr.ibos[tr.numIBOs] = (IBO_t *)ri->Hunk_Alloc(sizeof(*ibo), h_low);

	ibo->indexesSize = indexesSize;
	ibo->indexesVBO = tr.iboNames[tr.numIBOs];
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
		ri->Error(ERR_DROP, "R_BindNullVBO: NULL vbo");
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
		ri->Error(ERR_DROP, "R_BindIBO: NULL ibo");
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
R_InitVBOs
============
*/
void R_InitVBOs(void)
{
	ri->Printf(PRINT_ALL, "------- R_InitVBOs -------\n");

	// glGenBuffers only allocates the IDs for these buffers. The 'buffer object' is
	// actually created on first bind.
	qglGenBuffers(MAX_IBOS, tr.iboNames);
	qglGenBuffers(MAX_VBOS, tr.vboNames);

	tr.numVBOs = 0;
	tr.numIBOs = 0;

	R_BindNullVBO();
	R_BindNullIBO();

	GL_CheckErrors();
}

/*
============
R_ShutdownVBOs
============
*/
void R_ShutdownVBOs(void)
{
	ri->Printf(PRINT_ALL, "------- R_ShutdownVBOs -------\n");

	R_BindNullVBO();
	R_BindNullIBO();

	qglDeleteBuffers(MAX_IBOS, tr.iboNames);
	qglDeleteBuffers(MAX_VBOS, tr.vboNames);

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

	ri->Printf (PRINT_ALL, " vertex buffers\n");
	ri->Printf (PRINT_ALL, "----------------\n\n");

	ri->Printf(PRINT_ALL, " id   size (MB)\n");
	ri->Printf(PRINT_ALL, "---------------\n");

	for(i = 0; i < tr.numVBOs; i++)
	{
		vbo = tr.vbos[i];

		ri->Printf(PRINT_ALL, " %4i %4.2f\n", i, vbo->vertexesSize / (1024.0f * 1024.0f));

		vertexesSize += vbo->vertexesSize;
	}

	ri->Printf(PRINT_ALL, " %d total buffers\n", tr.numVBOs);
	ri->Printf(PRINT_ALL, " %.2f MB in total\n\n", vertexesSize / (1024.0f * 1024.0f));


	ri->Printf (PRINT_ALL, " index buffers\n");
	ri->Printf (PRINT_ALL, "---------------\n\n");

	ri->Printf(PRINT_ALL, " id   size (MB)\n");
	ri->Printf(PRINT_ALL, "---------------\n");

	for(i = 0; i < tr.numIBOs; i++)
	{
		ibo = tr.ibos[i];

		ri->Printf(PRINT_ALL, " %4i %4.2f\n", i, ibo->indexesSize / (1024.0f * 1024.0f));

		indexesSize += ibo->indexesSize;
	}

	ri->Printf(PRINT_ALL, " %d total buffers\n", tr.numIBOs);
	ri->Printf(PRINT_ALL, " %.2f MB in total\n\n", indexesSize / (1024.0f * 1024.0f));
}

void AddVertexArray(VertexArraysProperties *properties, int attributeIndex, size_t size, int stride, int offset, void *stream )
{
	properties->enabledAttributes[properties->numVertexArrays]  = attributeIndex;
	properties->offsets[attributeIndex]                         = offset;
	properties->vertexDataSize                                 += size;
	properties->sizes[attributeIndex]                           = size;
	properties->strides[attributeIndex]                         = stride;
	properties->streams[attributeIndex]                         = stream;

	properties->numVertexArrays++;
}

void CalculateVertexArraysProperties(uint32_t attributes, VertexArraysProperties *properties)
{
	properties->vertexDataSize = 0;
	properties->numVertexArrays = 0;

	if(attributes & ATTR_BITS)
	{
		if (attributes & ATTR_POSITION)
			AddVertexArray(properties, ATTR_INDEX_POSITION, sizeof(tess.xyz[0]), 0, properties->vertexDataSize, tess.xyz);

		if (attributes & ATTR_TEXCOORD0)
			AddVertexArray(properties, ATTR_INDEX_TEXCOORD0, sizeof(tess.texCoords[0][0]) * 2, 0, properties->vertexDataSize, tess.texCoords[0][0]);

		if (attributes & ATTR_TEXCOORD1)
			AddVertexArray(properties, ATTR_INDEX_TEXCOORD1, sizeof(tess.texCoords[0][1]) * 2, 0, properties->vertexDataSize, tess.texCoords[0][1]);

		if (attributes & ATTR_NORMAL)
			AddVertexArray(properties, ATTR_INDEX_NORMAL, sizeof(tess.normal[0]), 0, properties->vertexDataSize, tess.normal);

		if (attributes & ATTR_TANGENT)
			AddVertexArray(properties, ATTR_INDEX_TANGENT, sizeof(tess.tangent[0]), 0, properties->vertexDataSize, tess.tangent);

		if (attributes & ATTR_COLOR)
			AddVertexArray(properties, ATTR_INDEX_COLOR, sizeof(tess.vertexColors[0]), 0, properties->vertexDataSize, tess.vertexColors);

		if (attributes & ATTR_LIGHTDIRECTION)
			AddVertexArray(properties, ATTR_INDEX_LIGHTDIRECTION, sizeof(tess.lightdir[0]), 0, properties->vertexDataSize, tess.lightdir);
	}
	else
	{
		AddVertexArray(properties, ATTR_INDEX_POSITION, sizeof(tess.xyz[0]), 0, properties->vertexDataSize, tess.xyz);
		AddVertexArray(properties, ATTR_INDEX_TEXCOORD0, sizeof(tess.texCoords[0][0]) * 2, 0, properties->vertexDataSize, tess.texCoords[0][0]);
		AddVertexArray(properties, ATTR_INDEX_TEXCOORD1, sizeof(tess.texCoords[0][1]) * 2, 0, properties->vertexDataSize, tess.texCoords[0][1]);
		AddVertexArray(properties, ATTR_INDEX_NORMAL, sizeof(tess.normal[0]), 0, properties->vertexDataSize, tess.normal);
		AddVertexArray(properties, ATTR_INDEX_TANGENT, sizeof(tess.tangent[0]), 0, properties->vertexDataSize, tess.tangent);
		AddVertexArray(properties, ATTR_INDEX_COLOR, sizeof(tess.vertexColors[0]), 0, properties->vertexDataSize, tess.vertexColors);
		AddVertexArray(properties, ATTR_INDEX_LIGHTDIRECTION, sizeof(tess.lightdir[0]), 0, properties->vertexDataSize, tess.lightdir);
	}

	for ( int i = 0; i < properties->numVertexArrays; i++ )
		properties->strides[properties->enabledAttributes[i]] = properties->vertexDataSize;
}

void CalculateVertexArraysFromVBO(uint32_t attributes, const VBO_t *vbo, VertexArraysProperties *properties)
{
	properties->vertexDataSize = 0;
	properties->numVertexArrays = 0;

	for ( int i = 0, j = 1; i < ATTR_INDEX_MAX; i++, j <<= 1 )
	{
		if ( attributes & j )
			AddVertexArray(properties, i, vbo->sizes[i], vbo->strides[i], vbo->offsets[i], NULL);
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
				int attributeIndex = vertexArrays.enabledAttributes[j];
				void *stream = vertexArrays.streams[attributeIndex];
				size_t vertexSize = vertexArrays.sizes[attributeIndex];

				memcpy(writePtr, (byte *)stream + i * vertexSize, vertexArrays.sizes[attributeIndex]);
				writePtr = (byte *)writePtr + vertexArrays.sizes[attributeIndex];
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

void RB_CommitInternalBufferData()
{
	gpuFrame_t *currentFrame = backEndData->currentFrame;

	currentFrame->dynamicIboCommitOffset = currentFrame->dynamicIboWriteOffset;
	currentFrame->dynamicVboCommitOffset = currentFrame->dynamicVboWriteOffset;
}

void RB_BindAndUpdateUniformBlock(uniformBlock_t block, void *data)
{
	const uniformBlockInfo_t *blockInfo = uniformBlocksInfo + block;
	gpuFrame_t *thisFrame = backEndData->currentFrame;

	RB_BindUniformBlock(block);

	qglBufferSubData(GL_UNIFORM_BUFFER,
			thisFrame->uboWriteOffset, blockInfo->size, data);

	// FIXME: Use actual ubo alignment
	const size_t alignedBlockSize = (blockInfo->size + 255) & ~255;
	thisFrame->uboWriteOffset += alignedBlockSize;
}

void RB_BindUniformBlock(uniformBlock_t block)
{
	const uniformBlockInfo_t *blockInfo = uniformBlocksInfo + block;
	gpuFrame_t *thisFrame = backEndData->currentFrame;

	qglBindBufferRange(GL_UNIFORM_BUFFER, blockInfo->slot,
			thisFrame->ubo, thisFrame->uboWriteOffset, blockInfo->size);
}
