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

#include "tr_weather.h"
#include "tr_local.h"
#include <utility>
#include <vector>
#include <cmath>


struct weatherSystem_t
{
	VBO_t *lastVBO;
	VBO_t *vbo;
	unsigned vboLastUpdateFrame;
	int numVertices;

	srfWeather_t weatherSurface;
	vertexAttribute_t attribsTemplate[2];
};

namespace
{
	const float CHUNK_SIZE = 2000.0f;
	const float HALF_CHUNK_SIZE = CHUNK_SIZE * 0.5f;
	const int CHUNK_COUNT = 9;  // in 3x3 arrangement
	const int RAIN_VERTEX_COUNT = 50000;

	struct rainVertex_t
	{
		vec3_t position;
		vec3_t velocity;
	};

	void GenerateRainModel( weatherSystem_t& ws )
	{
		std::vector<rainVertex_t> rainVertices(RAIN_VERTEX_COUNT * CHUNK_COUNT);

		for ( int i = 0; i < rainVertices.size(); ++i )
		{
			rainVertex_t& vertex = rainVertices[i];
			vertex.position[0] = Q_flrand(-HALF_CHUNK_SIZE, HALF_CHUNK_SIZE);
			vertex.position[1] = Q_flrand(-HALF_CHUNK_SIZE, HALF_CHUNK_SIZE);
			vertex.position[2] = Q_flrand(-HALF_CHUNK_SIZE, HALF_CHUNK_SIZE);
			vertex.velocity[0] = Q_flrand(-2.0f, 2.0f);
			vertex.velocity[1] = Q_flrand(-2.0f, 2.0f);
			vertex.velocity[2] = Q_flrand(-20.0f, 0.0f);
		}

		ws.lastVBO = R_CreateVBO(
			nullptr,
			sizeof(rainVertex_t) * rainVertices.size(),
			VBO_USAGE_XFB);
		ws.vbo = R_CreateVBO(
			(byte *)rainVertices.data(),
			sizeof(rainVertex_t) * rainVertices.size(),
			VBO_USAGE_XFB);
		ws.vboLastUpdateFrame = 0;

		ws.attribsTemplate[0].index = ATTR_INDEX_POSITION;
		ws.attribsTemplate[0].numComponents = 3;
		ws.attribsTemplate[0].offset = offsetof(rainVertex_t, position);
		ws.attribsTemplate[0].stride = sizeof(rainVertex_t);
		ws.attribsTemplate[0].type = GL_FLOAT;
		ws.attribsTemplate[0].vbo = nullptr;

		ws.attribsTemplate[1].index = ATTR_INDEX_COLOR;
		ws.attribsTemplate[1].numComponents = 3;
		ws.attribsTemplate[1].offset = offsetof(rainVertex_t, velocity);
		ws.attribsTemplate[1].stride = sizeof(rainVertex_t);
		ws.attribsTemplate[1].type = GL_FLOAT;
		ws.attribsTemplate[1].vbo = nullptr;
	}

	void GenerateDepthMap()
	{
		vec3_t mapSize;
		VectorSubtract(
			tr.world->bmodels[0].bounds[0], 
			tr.world->bmodels[0].bounds[1],
			mapSize);
		mapSize[2] = 0.0f;

		const vec3_t left = {0.0f, 1.0f, 0.0f};
		const vec3_t up = {0.0f, 0.0f, 1.0f};
		const vec3_t forward = {1.0f, 0.0f, 0.0f};

		matrix3_t viewAxes;
		vec3_t viewOrigin;
		VectorMA(tr.world->bmodels[0].bounds[0], 0.5f, mapSize, viewOrigin);

		orientationr_t orientation;
		R_SetOrientationOriginAndAxis(orientation, viewOrigin, left, forward, up);

		refdef_t refdef = {};
		RE_BeginScene(&refdef);
		RE_ClearScene();

		R_SetupViewParmsForOrthoRendering(
			tr.weatherDepthFbo->width,
			tr.weatherDepthFbo->height,
			tr.weatherDepthFbo,
			VPF_DEPTHCLAMP | VPF_DEPTHSHADOW | VPF_ORTHOGRAPHIC | VPF_NOVIEWMODEL,
			orientation,
			tr.world->bmodels[0].bounds);

		const int firstDrawSurf = tr.refdef.numDrawSurfs;

		R_GenerateDrawSurfs(&tr.viewParms, &tr.refdef);
		R_SortAndSubmitDrawSurfs(
			tr.refdef.drawSurfs + firstDrawSurf,
			tr.refdef.numDrawSurfs - firstDrawSurf);
		R_IssuePendingRenderCommands();
		R_InitNextFrame();
		RE_EndScene();
	}

	void RB_SimulateWeather(weatherSystem_t& ws)
	{
		if (ws.vboLastUpdateFrame == backEndData->realFrameNumber)
		{
			// Already simulated for this frame
			return;
		}

		// Intentionally switched. Previous frame's VBO would be in ws.vbo and
		// this frame's VBO would be ws.lastVBO.
		VBO_t *lastRainVBO = ws.vbo;
		VBO_t *rainVBO = ws.lastVBO;

		DrawItem item = {};
		item.renderState.transformFeedback = true;
		item.transformFeedbackBuffer = {rainVBO, 0, rainVBO->vertexesSize};
		item.program = &tr.weatherUpdateShader;

		const size_t numAttribs = ARRAY_LEN(ws.attribsTemplate);
		item.numAttributes = numAttribs;
		item.attributes = ojkAllocArray<vertexAttribute_t>(
			*backEndData->perFrameMemory, numAttribs);
		memcpy(
			item.attributes,
			ws.attribsTemplate,
			sizeof(*item.attributes) * numAttribs);
		item.attributes[0].vbo = lastRainVBO;
		item.attributes[1].vbo = lastRainVBO;

		UniformDataWriter uniformDataWriter;
		uniformDataWriter.Start(&tr.weatherUpdateShader);

		const vec2_t mapZExtents = {
			-1000.0f, 1000.0f
			//tr.world->bmodels[0].bounds[0][2],
			//tr.world->bmodels[0].bounds[1][2]
		};
		uniformDataWriter.SetUniformVec2(UNIFORM_MAPZEXTENTS, mapZExtents);
		uniformDataWriter.SetUniformFloat(UNIFORM_TIME, backEnd.refdef.floatTime);
		item.uniformData = uniformDataWriter.Finish(*backEndData->perFrameMemory);

		item.draw.type = DRAW_COMMAND_ARRAYS;
		item.draw.numInstances = 1;
		item.draw.primitiveType = GL_POINTS;
		item.draw.params.arrays.numVertices = RAIN_VERTEX_COUNT * CHUNK_COUNT;

		// This is a bit dodgy. Push this towards the front of the queue so we
		// guarantee this happens before the actual drawing
		const uint32_t key = RB_CreateSortKey(item, 0, SS_OPAQUE);
		RB_AddDrawItem(backEndData->currentPass, key, item);

		ws.vboLastUpdateFrame = backEndData->realFrameNumber;
		std::swap(ws.lastVBO, ws.vbo);
	}
}

void R_InitWeatherForMap()
{
	GenerateRainModel(*tr.weatherSystem);
	GenerateDepthMap();
}

void R_InitWeatherSystem()
{
	Com_Printf("Initializing weather system\n");
	tr.weatherSystem =
		(weatherSystem_t *)Z_Malloc(sizeof(*tr.weatherSystem), TAG_R_TERRAIN, qtrue);
	tr.weatherSystem->weatherSurface.surfaceType = SF_WEATHER;
}

void R_ShutdownWeatherSystem()
{
	if (tr.weatherSystem != nullptr)
	{
		Com_Printf("Shutting down weather system\n");

		Z_Free(tr.weatherSystem);
		tr.weatherSystem = nullptr;
	}
	else
	{
		ri.Printf(PRINT_DEVELOPER,
			"Weather system shutdown requested, but it is already shut down.\n");
	}
}

void R_AddWeatherSurfaces()
{
	assert(tr.weatherSystem);

	if (r_debugWeather->integer)
	{
		R_AddDrawSurf(
			(surfaceType_t *)&tr.weatherSystem->weatherSurface,
			REFENTITYNUM_WORLD,
			tr.weatherInternalShader,
			0, /* fogIndex */
			qfalse, /* dlightMap */
			qfalse, /* postRender */
			0 /* cubemapIndex */
		);
	}
}

void RB_SurfaceWeather( srfWeather_t *surf )
{
	assert(tr.weatherSystem);

	weatherSystem_t& ws = *tr.weatherSystem;
	assert(surf == &ws.weatherSurface);

	RB_EndSurface();

	RB_SimulateWeather(ws);

	DrawItem item = {};

	item.renderState.stateBits =
		GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	item.renderState.cullType = CT_FRONT_SIDED;
	item.renderState.depthRange = { 0.0f, 1.0f };
	item.program = &tr.weatherShader;

	const size_t numAttribs = ARRAY_LEN(ws.attribsTemplate);
	item.numAttributes = numAttribs;
	item.attributes = ojkAllocArray<vertexAttribute_t>(
		*backEndData->perFrameMemory, numAttribs);
	memcpy(
		item.attributes,
		ws.attribsTemplate,
		sizeof(*item.attributes) * numAttribs);
	item.attributes[0].vbo = ws.vbo;
	item.attributes[1].vbo = ws.vbo;

	item.draw.type = DRAW_COMMAND_ARRAYS;
	item.draw.numInstances = 1;
	item.draw.primitiveType = GL_POINTS;
	item.draw.params.arrays.numVertices = RAIN_VERTEX_COUNT;

	const float *viewOrigin = backEnd.viewParms.ori.origin;
	float centerZoneOffsetX =
		std::floor((viewOrigin[0] / CHUNK_SIZE) + 0.5f) * CHUNK_SIZE;
	float centerZoneOffsetY =
		std::floor((viewOrigin[1] / CHUNK_SIZE) + 0.5f) * CHUNK_SIZE;
	int chunkIndex = 0;
	for (int y = -1; y <= 1; ++y)
	{
		for (int x = -1; x <= 1; ++x, ++chunkIndex)
		{
			UniformDataWriter uniformDataWriter;
			uniformDataWriter.Start(&tr.weatherShader);
			uniformDataWriter.SetUniformMatrix4x4(
				UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
			uniformDataWriter.SetUniformVec3(
				UNIFORM_VIEWORIGIN, backEnd.viewParms.ori.origin);
			uniformDataWriter.SetUniformVec2(
				UNIFORM_ZONEOFFSET,
				centerZoneOffsetX + x * CHUNK_SIZE,
				centerZoneOffsetY + y * CHUNK_SIZE);
			uniformDataWriter.SetUniformFloat(UNIFORM_TIME, backEnd.refdef.floatTime);
			item.uniformData = uniformDataWriter.Finish(*backEndData->perFrameMemory);

			item.draw.params.arrays.firstVertex = RAIN_VERTEX_COUNT * chunkIndex;

			uint32_t key = RB_CreateSortKey(item, 15, SS_SEE_THROUGH);
			RB_AddDrawItem(backEndData->currentPass, key, item);
		}
	}
}
