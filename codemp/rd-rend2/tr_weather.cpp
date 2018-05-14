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

struct weatherSystem_t
{
	VBO_t *lastVBO;
	VBO_t *vbo;
	unsigned vboLastUpdateFrame;
	int numVertices;

	srfWeather_t weatherSurface;
};

namespace
{
	struct rainVertex_t
	{
		vec3_t position;
		vec3_t velocity;
	};

	void GenerateRainModel( weatherSystem_t& ws )
	{
		static const int MAX_RAIN_VERTICES = 5000;
		rainVertex_t rainVertices[MAX_RAIN_VERTICES];

		for ( int i = 0; i < MAX_RAIN_VERTICES; ++i )
		{
			rainVertex_t& vertex = rainVertices[i];
			vertex.position[0] = Q_flrand(-1000.0f, 3000.0f);
			vertex.position[1] = Q_flrand(-1000.0f, 3000.0f);
			vertex.position[2] = Q_flrand(-1000.0f, 3000.0f);
			vertex.velocity[0] = Q_flrand(-2.0f, 2.0f);
			vertex.velocity[1] = Q_flrand(-2.0f, 2.0f);
			vertex.velocity[2] = Q_flrand(-20.0f, 0.0f);
		}

		ws.lastVBO = R_CreateVBO(nullptr, sizeof(rainVertices), VBO_USAGE_XFB);
		ws.vbo = R_CreateVBO((byte *)rainVertices, sizeof(rainVertices), VBO_USAGE_XFB);
		ws.numVertices = MAX_RAIN_VERTICES;
		ws.vboLastUpdateFrame = 0;
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

		vertexAttribute_t attribs[2] = {};
		attribs[0].index = ATTR_INDEX_POSITION;
		attribs[0].numComponents = 3;
		attribs[0].offset = offsetof(rainVertex_t, position);
		attribs[0].stride = sizeof(rainVertex_t);
		attribs[0].type = GL_FLOAT;
		attribs[0].vbo = lastRainVBO;
		attribs[1].index = ATTR_INDEX_COLOR;
		attribs[1].numComponents = 3;
		attribs[1].offset = offsetof(rainVertex_t, velocity);
		attribs[1].stride = sizeof(rainVertex_t);
		attribs[1].type = GL_FLOAT;
		attribs[1].vbo = lastRainVBO;

		const size_t numAttribs = ARRAY_LEN(attribs);

		DrawItem item = {};
		item.renderState.transformFeedback = true;
		item.transformFeedbackBuffer = rainVBO;
		item.program = &tr.weatherUpdateShader;
		item.numAttributes = numAttribs;
		item.attributes = ojkAllocArray<vertexAttribute_t>(
			*backEndData->perFrameMemory, numAttribs);
		memcpy(item.attributes, attribs, sizeof(*item.attributes) * numAttribs);

		UniformDataWriter uniformDataWriter;
		uniformDataWriter.Start(&tr.weatherUpdateShader);

		const vec2_t mapZExtents = {
			tr.world->bmodels[0].bounds[0][2],
			tr.world->bmodels[0].bounds[1][2]
		};
		uniformDataWriter.SetUniformVec2(UNIFORM_MAPZEXTENTS, mapZExtents);
		uniformDataWriter.SetUniformFloat(UNIFORM_TIME, backEnd.refdef.floatTime);
		item.uniformData = uniformDataWriter.Finish(*backEndData->perFrameMemory);

		item.draw.type = DRAW_COMMAND_ARRAYS;
		item.draw.numInstances = 1;
		item.draw.primitiveType = GL_POINTS;
		item.draw.params.arrays.numVertices = ws.numVertices;

		// This is a bit dodgy. Push this towards the front of the queue so we
		// guarantee this happens before the actual drawing
		const uint32_t key = RB_CreateSortKey(item, 0, SS_OPAQUE);
		RB_AddDrawItem(backEndData->currentPass, key, item);

		ws.vboLastUpdateFrame = backEndData->realFrameNumber;
		std::swap(ws.lastVBO, ws.vbo);
	}
}

void R_InitWeatherSystem()
{
	Com_Printf("Initializing weather system\n");
	tr.weatherSystem =
		(weatherSystem_t *)Z_Malloc(sizeof(*tr.weatherSystem), TAG_R_TERRAIN, qtrue);
	GenerateRainModel(*tr.weatherSystem);
	tr.weatherSystem->weatherSurface.surfaceType = SF_WEATHER;
}

void R_ShutdownWeatherSystem()
{
	if ( tr.weatherSystem )
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

	if (true)
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

	SamplerBindingsWriter samplerBindingsWriter;
	item.samplerBindings = samplerBindingsWriter.Finish(
		*backEndData->perFrameMemory, (int *)&item.numSamplerBindings);

	UniformDataWriter uniformDataWriter;
	uniformDataWriter.Start(&tr.weatherShader);
	uniformDataWriter.SetUniformMatrix4x4(
		UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
	uniformDataWriter.SetUniformVec3(
		UNIFORM_VIEWORIGIN, backEnd.viewParms.ori.origin);
	const vec2_t mapZExtents = { -3000.0, 9000.0 };
	uniformDataWriter.SetUniformVec2(UNIFORM_MAPZEXTENTS, mapZExtents);
	uniformDataWriter.SetUniformFloat(UNIFORM_TIME, backEnd.refdef.floatTime);
	item.uniformData = uniformDataWriter.Finish(*backEndData->perFrameMemory);

	item.renderState.stateBits =
		GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	item.renderState.cullType = CT_FRONT_SIDED;
	item.renderState.depthRange = { 0.0f, 1.0f };
	item.program = &tr.weatherShader;

	vertexAttribute_t attribs[2] = {};
	attribs[0].index = ATTR_INDEX_POSITION;
	attribs[0].numComponents = 3;
	attribs[0].offset = offsetof(rainVertex_t, position);
	attribs[0].stride = sizeof(rainVertex_t);
	attribs[0].type = GL_FLOAT;
	attribs[0].vbo = ws.vbo;

	attribs[1].index = ATTR_INDEX_COLOR;
	attribs[1].numComponents = 3;
	attribs[1].offset = offsetof(rainVertex_t, velocity);
	attribs[1].stride = sizeof(rainVertex_t);
	attribs[1].type = GL_FLOAT;
	attribs[1].vbo = ws.vbo;

	const size_t numAttribs = ARRAY_LEN(attribs);
	item.numAttributes = numAttribs;
	item.attributes = ojkAllocArray<vertexAttribute_t>(
		*backEndData->perFrameMemory, numAttribs);
	memcpy(item.attributes, attribs, sizeof(*item.attributes) * numAttribs);

	item.draw.type = DRAW_COMMAND_ARRAYS;
	item.draw.numInstances = 1;
	item.draw.primitiveType = GL_POINTS;
	item.draw.params.arrays.numVertices = ws.numVertices;

	uint32_t key = RB_CreateSortKey(item, 15, SS_SEE_THROUGH);
	RB_AddDrawItem(backEndData->currentPass, key, item);
}
