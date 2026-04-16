/*
===========================================================================
Copyright (C) 2016, OpenJK contributors

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
#include "MikkTSpace/mikktspace.h"


/*
================
R_FixMikktVertIndex

Swaps second index to third and the other way around.
When baking normal maps for models, the mesh importer will swap face indices to change the face orientation.
To generate exactly the same tangents, we have to swap indices to the same order.
Change when mesh importers swaps different indices!
(modelling tool standard is backface culling vs frontface culling in id tech 3)
================
*/
int R_FixMikktVertIndex(const int index)
{
	switch (index % 3)
	{
	case 2: return 1;
	case 1: return 2;
	default: return index;
	}
	return index;
}

struct BspMeshData
{
	int numSurfaces;
	packedVertex_t *vertices;
	glIndex_t *indices;
};

int R_GetNumFaces(const SMikkTSpaceContext * pContext)
{
	BspMeshData *meshData = (BspMeshData *)pContext->m_pUserData;
	return meshData->numSurfaces;
}

int R_GetNumVertices(const SMikkTSpaceContext * pContext, const int iFace)
{
	return 3;
}

void R_GetBSPPosition(const SMikkTSpaceContext * pContext, float *fvPosOut, const int iFace, const int iVert)
{
	BspMeshData *meshData = (BspMeshData *)pContext->m_pUserData;
	const int vert_index = R_FixMikktVertIndex(iVert);
	glIndex_t index = meshData->indices[iFace * 3 + vert_index];
	packedVertex_t& vertex = meshData->vertices[index];
	fvPosOut[0] = vertex.position[0];
	fvPosOut[1] = vertex.position[1];
	fvPosOut[2] = vertex.position[2];
}

void R_GetNormalBSPSurface(const SMikkTSpaceContext * pContext, float *fvNormOut, const int iFace, const int iVert)
{
	BspMeshData *meshData = (BspMeshData *)pContext->m_pUserData;
	const int vert_index = R_FixMikktVertIndex(iVert);
	glIndex_t index = meshData->indices[iFace * 3 + vert_index];
	packedVertex_t& vertex = meshData->vertices[index];

	fvNormOut[0] = ((vertex.normal) & 0x3ff) * 1.0f / 511.5f - 1.0f;
	fvNormOut[1] = ((vertex.normal >> 10) & 0x3ff) * 1.0f / 511.5f - 1.0f;
	fvNormOut[2] = ((vertex.normal >> 20) & 0x3ff) * 1.0f / 511.5f - 1.0f;
}

void R_GetBSPTexCoord(const SMikkTSpaceContext * pContext, float *fvTexcOut, const int iFace, const int iVert)
{
	BspMeshData *meshData = (BspMeshData *)pContext->m_pUserData;
	const int vert_index = R_FixMikktVertIndex(iVert);
	glIndex_t index = meshData->indices[iFace * 3 + vert_index];
	packedVertex_t& vertex = meshData->vertices[index];
	fvTexcOut[0] = vertex.texcoords[0][0];
	fvTexcOut[1] = vertex.texcoords[0][1];
}

void R_SetBSPTSpaceBasic(const SMikkTSpaceContext * pContext, const float *fvTangent, const float fSign, const int iFace, const int iVert)
{
	BspMeshData *meshData = (BspMeshData *)pContext->m_pUserData;
	const int vert_index = R_FixMikktVertIndex(iVert);
	glIndex_t index = meshData->indices[iFace * 3 + vert_index];
	packedVertex_t& vertex = meshData->vertices[index];
	if (vertex.tangent == 0u)
		vertex.tangent = (((uint32_t)(fSign * 1.5f + 2.0f)) << 30)
		| (((uint32_t)(fvTangent[2] * 511.5f + 512.0f)) << 20)
		| (((uint32_t)(fvTangent[1] * 511.5f + 512.0f)) << 10)
		| (((uint32_t)(fvTangent[0] * 511.5f + 512.0f)));
}

void R_CalcMikkTSpaceBSPSurface(int numSurfaces, packedVertex_t *vertices, glIndex_t *indices)
{
	SMikkTSpaceInterface tangentSpaceInterface;

	tangentSpaceInterface.m_getNumFaces = R_GetNumFaces;
	tangentSpaceInterface.m_getNumVerticesOfFace = R_GetNumVertices;
	tangentSpaceInterface.m_getPosition = R_GetBSPPosition;
	tangentSpaceInterface.m_getNormal = R_GetNormalBSPSurface;
	tangentSpaceInterface.m_getTexCoord = R_GetBSPTexCoord;
	tangentSpaceInterface.m_setTSpaceBasic = R_SetBSPTSpaceBasic;
	tangentSpaceInterface.m_setTSpace = NULL;

	BspMeshData meshData;
	meshData.numSurfaces = numSurfaces;
	meshData.vertices = vertices;
	meshData.indices = indices;

	SMikkTSpaceContext modelContext;
	modelContext.m_pUserData = &meshData;
	modelContext.m_pInterface = &tangentSpaceInterface;

	genTangSpaceDefault(&modelContext);
}

struct ModelMeshData
{
	int numSurfaces;
	mdvVertex_t *verts;
	uint32_t *tangents;
	mdvSt_t *texcoords;
	glIndex_t *indices;
};

void R_GetModelPosition(const SMikkTSpaceContext * pContext, float *fvPosOut, const int iFace, const int iVert)
{
	ModelMeshData *meshData = (ModelMeshData *)pContext->m_pUserData;
	const int vert_index = R_FixMikktVertIndex(iVert);
	glIndex_t index = meshData->indices[iFace * 3 + vert_index];
	fvPosOut[0] = meshData->verts[index].xyz[0];
	fvPosOut[1] = meshData->verts[index].xyz[1];
	fvPosOut[2] = meshData->verts[index].xyz[2];
}

void R_GetNormalModelSurface(const SMikkTSpaceContext * pContext, float *fvNormOut, const int iFace, const int iVert)
{
	ModelMeshData *meshData = (ModelMeshData *)pContext->m_pUserData;
	const int vert_index = R_FixMikktVertIndex(iVert);
	glIndex_t index = meshData->indices[iFace * 3 + vert_index];

	fvNormOut[0] = meshData->verts[index].normal[0];
	fvNormOut[1] = meshData->verts[index].normal[1];
	fvNormOut[2] = meshData->verts[index].normal[2];
}

void R_GetModelTexCoord(const SMikkTSpaceContext * pContext, float *fvTexcOut, const int iFace, const int iVert)
{
	ModelMeshData *meshData = (ModelMeshData *)pContext->m_pUserData;
	const int vert_index = R_FixMikktVertIndex(iVert);
	glIndex_t index = meshData->indices[iFace * 3 + vert_index];
	fvTexcOut[0] = meshData->texcoords[index].st[0];
	fvTexcOut[1] = meshData->texcoords[index].st[1];
}

void R_SetModelTSpaceBasic(const SMikkTSpaceContext * pContext, const float *fvTangent, const float fSign, const int iFace, const int iVert)
{
	ModelMeshData *meshData = (ModelMeshData *)pContext->m_pUserData;
	const int vert_index = R_FixMikktVertIndex(iVert);
	glIndex_t index = meshData->indices[iFace * 3 + vert_index];

	uint32_t& tangent = meshData->tangents[index];
	tangent = (((uint32_t)(fSign * 1.5f + 2.0f)) << 30)
		| (((uint32_t)(fvTangent[2] * 511.5f + 512.0f)) << 20)
		| (((uint32_t)(fvTangent[1] * 511.5f + 512.0f)) << 10)
		| (((uint32_t)(fvTangent[0] * 511.5f + 512.0f)));

}

void R_CalcMikkTSpaceMD3Surface(int numSurfaces, mdvVertex_t *verts, uint32_t *tangents, mdvSt_t *texcoords, glIndex_t *indices)
{
	SMikkTSpaceInterface tangentSpaceInterface;
	tangentSpaceInterface.m_getNumFaces = R_GetNumFaces;
	tangentSpaceInterface.m_getNumVerticesOfFace = R_GetNumVertices;
	tangentSpaceInterface.m_getPosition = R_GetModelPosition;
	tangentSpaceInterface.m_getNormal = R_GetNormalModelSurface;
	tangentSpaceInterface.m_getTexCoord = R_GetModelTexCoord;
	tangentSpaceInterface.m_setTSpaceBasic = R_SetModelTSpaceBasic;
	tangentSpaceInterface.m_setTSpace = NULL;

	ModelMeshData meshData;
	meshData.numSurfaces = numSurfaces;
	meshData.verts = verts;
	meshData.tangents = tangents;
	meshData.texcoords = texcoords;
	meshData.indices = indices;

	SMikkTSpaceContext modelContext;
	modelContext.m_pUserData = &meshData;
	modelContext.m_pInterface = &tangentSpaceInterface;
	genTangSpaceDefault(&modelContext);
}

struct GlmMeshData
{
	int numSurfaces;
	mdxmVertex_t *vertices;
	mdxmVertexTexCoord_t *tcs;
	uint32_t *tangents;
	glIndex_t *indices;
};

void R_GetGlmPosition(const SMikkTSpaceContext * pContext, float *fvPosOut, const int iFace, const int iVert)
{
	GlmMeshData *meshData = (GlmMeshData *)pContext->m_pUserData;
	const int vert_index = R_FixMikktVertIndex(iVert);
	glIndex_t index = meshData->indices[iFace * 3 + vert_index];
	mdxmVertex_t& vertex = meshData->vertices[index];
	fvPosOut[0] = vertex.vertCoords[0];
	fvPosOut[1] = vertex.vertCoords[1];
	fvPosOut[2] = vertex.vertCoords[2];
}

void R_GetNormalGlmSurface(const SMikkTSpaceContext * pContext, float *fvNormOut, const int iFace, const int iVert)
{
	GlmMeshData *meshData = (GlmMeshData *)pContext->m_pUserData;
	const int vert_index = R_FixMikktVertIndex(iVert);
	glIndex_t index = meshData->indices[iFace * 3 + vert_index];
	mdxmVertex_t& vertex = meshData->vertices[index];

	fvNormOut[0] = vertex.normal[0];
	fvNormOut[1] = vertex.normal[1];
	fvNormOut[2] = vertex.normal[2];
}

void R_GetGlmTexCoord(const SMikkTSpaceContext * pContext, float *fvTexcOut, const int iFace, const int iVert)
{
	GlmMeshData *meshData = (GlmMeshData *)pContext->m_pUserData;
	const int vert_index = R_FixMikktVertIndex(iVert);
	glIndex_t index = meshData->indices[iFace * 3 + vert_index];
	mdxmVertexTexCoord_t& tcs = meshData->tcs[index];
	fvTexcOut[0] = tcs.texCoords[0];
	fvTexcOut[1] = tcs.texCoords[1];
}

void R_SetGlmTSpaceBasic(const SMikkTSpaceContext * pContext, const float *fvTangent, const float fSign, const int iFace, const int iVert)
{
	GlmMeshData *meshData = (GlmMeshData *)pContext->m_pUserData;
	const int vert_index = R_FixMikktVertIndex(iVert);
	glIndex_t index = meshData->indices[iFace * 3 + vert_index];
	uint32_t& tangent = meshData->tangents[index];

	tangent = (((uint32_t)(fSign * 1.5f + 2.0f)) << 30)
		| (((uint32_t)(fvTangent[2] * 511.5f + 512.0f)) << 20)
		| (((uint32_t)(fvTangent[1] * 511.5f + 512.0f)) << 10)
		| (((uint32_t)(fvTangent[0] * 511.5f + 512.0f)));
}

void R_CalcMikkTSpaceGlmSurface(int numSurfaces, mdxmVertex_t *vertices, mdxmVertexTexCoord_t *textureCoordinates, uint32_t *tangents, glIndex_t *indices)
{
	SMikkTSpaceInterface tangentSpaceInterface;

	tangentSpaceInterface.m_getNumFaces = R_GetNumFaces;
	tangentSpaceInterface.m_getNumVerticesOfFace = R_GetNumVertices;
	tangentSpaceInterface.m_getPosition = R_GetGlmPosition;
	tangentSpaceInterface.m_getNormal = R_GetNormalGlmSurface;
	tangentSpaceInterface.m_getTexCoord = R_GetGlmTexCoord;
	tangentSpaceInterface.m_setTSpaceBasic = R_SetGlmTSpaceBasic;
	tangentSpaceInterface.m_setTSpace = NULL;

	GlmMeshData meshData;
	meshData.numSurfaces = numSurfaces;
	meshData.vertices = vertices;
	meshData.tcs = textureCoordinates;
	meshData.tangents = tangents;
	meshData.indices = indices;

	SMikkTSpaceContext modelContext;
	modelContext.m_pUserData = &meshData;
	modelContext.m_pInterface = &tangentSpaceInterface;

	genTangSpaceDefault(&modelContext);
}