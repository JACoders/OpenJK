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
	int             dataSize;
	int             offset;

	ri->Printf(PRINT_ALL, "------- R_InitVBOs -------\n");

	// glGenBuffers only allocates the IDs for these buffers. The 'buffer object' is
	// actually created on first bind.
	qglGenBuffers(MAX_IBOS, tr.iboNames);
	qglGenBuffers(MAX_VBOS, tr.vboNames);

	tr.numVBOs = 0;
	tr.numIBOs = 0;

	dataSize  = 12 * 1024 * 1024;
	tess.vbo = R_CreateVBO(NULL, dataSize, VBO_USAGE_DYNAMIC);

	offset = 0;

	tess.vbo->offsets[ATTR_INDEX_POSITION]       = offset; offset += sizeof(tess.xyz[0])              * SHADER_MAX_VERTEXES;
	tess.vbo->offsets[ATTR_INDEX_NORMAL]         = offset; offset += sizeof(tess.normal[0])           * SHADER_MAX_VERTEXES;
	// these next two are actually interleaved
	tess.vbo->offsets[ATTR_INDEX_TEXCOORD0]      = offset; offset += sizeof(tess.texCoords[0][0]) * 2 * SHADER_MAX_VERTEXES;
	tess.vbo->offsets[ATTR_INDEX_TANGENT]        = offset; offset += sizeof(tess.tangent[0])          * SHADER_MAX_VERTEXES;

	tess.vbo->offsets[ATTR_INDEX_COLOR]          = offset; offset += sizeof(tess.vertexColors[0])     * SHADER_MAX_VERTEXES;
	tess.vbo->offsets[ATTR_INDEX_LIGHTDIRECTION] = offset;

	tess.vbo->strides[ATTR_INDEX_POSITION]       = sizeof(tess.xyz[0]);
	tess.vbo->strides[ATTR_INDEX_NORMAL]         = sizeof(tess.normal[0]);
	tess.vbo->strides[ATTR_INDEX_TANGENT]        = sizeof(tess.tangent[0]);
	tess.vbo->strides[ATTR_INDEX_COLOR]          = sizeof(tess.vertexColors[0]);
	tess.vbo->strides[ATTR_INDEX_TEXCOORD0]      = sizeof(tess.texCoords[0][0]) * 2;
	tess.vbo->strides[ATTR_INDEX_LIGHTDIRECTION] = sizeof(tess.lightdir[0]);

	tess.vbo->sizes[ATTR_INDEX_POSITION]         = sizeof(tess.xyz[0]);
	tess.vbo->sizes[ATTR_INDEX_NORMAL]           = sizeof(tess.normal[0]);
	tess.vbo->sizes[ATTR_INDEX_TANGENT]          = sizeof(tess.tangent[0]);
	tess.vbo->sizes[ATTR_INDEX_COLOR]            = sizeof(tess.vertexColors[0]);
	tess.vbo->sizes[ATTR_INDEX_TEXCOORD0]        = sizeof(tess.texCoords[0][0]) * 2;
	tess.vbo->sizes[ATTR_INDEX_LIGHTDIRECTION]   = sizeof(tess.lightdir[0]);

	dataSize = 4 * 1024 * 1024;
	tess.ibo = R_CreateIBO(NULL, dataSize, VBO_USAGE_DYNAMIC);

	if ( glRefConfig.immutableBuffers )
	{
		R_BindVBO(tess.vbo);
		GL_CheckErrors();

		R_BindIBO(tess.ibo);
		GL_CheckErrors();

		tess.vboData = qglMapBufferRange(GL_ARRAY_BUFFER, 0, tess.vbo->vertexesSize, GL_MAP_WRITE_BIT | GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT);
		GL_CheckErrors();

		tess.iboData = qglMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, tess.ibo->indexesSize, GL_MAP_WRITE_BIT | GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT);
		GL_CheckErrors();

	}

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

	if ( glRefConfig.immutableBuffers )
	{
		R_BindVBO(tess.vbo);
		R_BindIBO(tess.ibo);
		qglUnmapBuffer(GL_ARRAY_BUFFER);
		qglUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
	}

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
	GLimp_LogComment("--- RB_UpdateVBOs ---\n");

	backEnd.pc.c_dynamicVboDraws++;

	// update the default VBO
	if(tess.numVertexes > 0 && tess.numVertexes <= SHADER_MAX_VERTEXES)
	{
		GLbitfield mapFlags = GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT;
		VertexArraysProperties vertexArrays = {};
		CalculateVertexArraysProperties(attribBits, &vertexArrays);

		int totalVertexDataSize = tess.numVertexes * vertexArrays.vertexDataSize;
		backEnd.pc.c_dynamicVboTotalSize += totalVertexDataSize;

		if ( (tess.internalVBOWriteOffset + totalVertexDataSize) > tess.vbo->vertexesSize )
		{
			tess.internalVBOCommitOffset = 0;
			tess.internalVBOWriteOffset = 0;
			mapFlags |= GL_MAP_INVALIDATE_BUFFER_BIT;
		}

		R_BindVBO(tess.vbo);

		// orphan old buffer so we don't stall on it
		void *dstPtr;
		if ( glRefConfig.immutableBuffers )
		{
			dstPtr = (byte *)tess.vboData + tess.internalVBOWriteOffset;
		}
		else
		{
			dstPtr = qglMapBufferRange(GL_ARRAY_BUFFER, tess.internalVBOWriteOffset, totalVertexDataSize, mapFlags);
		}

		// Interleave the data
		void *writePtr = dstPtr;
		for ( int i = 0; i < tess.numVertexes; i++ )
		{
			for ( int j = 0; j < vertexArrays.numVertexArrays; j++ )
			{
				int attributeIndex = vertexArrays.enabledAttributes[j];

				memcpy(writePtr, (byte *)vertexArrays.streams[attributeIndex] + i * vertexArrays.sizes[attributeIndex], vertexArrays.sizes[attributeIndex]);
				writePtr = (byte *)writePtr + vertexArrays.sizes[attributeIndex];
			}
		}

		if ( !glRefConfig.immutableBuffers )
		{
			qglUnmapBuffer(GL_ARRAY_BUFFER);
		}

		tess.internalVBOWriteOffset += totalVertexDataSize;
	}

	// update the default IBO
	if(tess.numIndexes > 0 && tess.numIndexes <= SHADER_MAX_INDEXES)
	{
		GLbitfield mapFlags = GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT;
		int totalIndexDataSize = tess.numIndexes * sizeof(tess.indexes[0]);

		R_BindIBO(tess.ibo);

		if ( (tess.internalIBOWriteOffset + totalIndexDataSize) > tess.ibo->indexesSize )
		{
			tess.internalIBOCommitOffset = 0;
			tess.internalIBOWriteOffset = 0;
			mapFlags |= GL_MAP_INVALIDATE_BUFFER_BIT;
		}

		void *dst;
		if ( glRefConfig.immutableBuffers )
		{
			dst = (byte *)tess.iboData + tess.internalIBOWriteOffset;
		}
		else
		{
			dst = qglMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, tess.internalIBOCommitOffset, totalIndexDataSize, mapFlags);
		}

		memcpy(dst, tess.indexes, totalIndexDataSize);

		if ( !glRefConfig.immutableBuffers )
		{
			qglUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
		}

		tess.internalIBOWriteOffset += totalIndexDataSize;
	}
}

void RB_CommitInternalBufferData()
{
	tess.internalIBOCommitOffset = tess.internalIBOWriteOffset;
	tess.internalVBOCommitOffset = tess.internalVBOWriteOffset;
}

void RB_UpdateUniformBlock(uniformBlock_t block, void *data)
{
	const uniformBlockInfo_t *blockInfo = uniformBlocksInfo + block;
	gpuFrame_t *thisFrame = backEndData->currentFrame;

	qglBufferSubData(GL_UNIFORM_BUFFER,
			thisFrame->uboWriteOffset, blockInfo->size, data);
	qglBindBufferRange(GL_UNIFORM_BUFFER, blockInfo->slot,
			thisFrame->ubo, thisFrame->uboWriteOffset, blockInfo->size);

	// FIXME: Use actual ubo alignment
	size_t alignedBlockSize = (blockInfo->size + 255) & ~255;
	thisFrame->uboWriteOffset += alignedBlockSize;
}
