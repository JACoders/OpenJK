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

struct weatherSystem_t
{
	VBO_t *vbo;
	int numVertices;

	srfWeather_t weatherSurface;
};

namespace
{
	struct rainVertex_t
	{
		vec3_t position;
		vec3_t seed;
	};

	void GenerateRainModel( weatherSystem_t& ws )
	{
		static const int MAX_RAIN_VERTICES = 1000;
		rainVertex_t rainVertices[MAX_RAIN_VERTICES];

		for ( int i = 0; i < MAX_RAIN_VERTICES; ++i )
		{
			rainVertex_t& vertex = rainVertices[i];
			vertex.position[0] = Q_flrand(-500.0f, 500.0f);
			vertex.position[1] = Q_flrand(-500.0f, 500.0f);
			vertex.position[2] = Q_flrand(-500.0f, 500.0f);
			vertex.seed[0] = Q_flrand(0.0f, 1.0f);
			vertex.seed[1] = Q_flrand(0.0f, 1.0f);
			vertex.seed[2] = Q_flrand(0.0f, 1.0f);
		}

		ws.vbo = R_CreateVBO((byte *)rainVertices, sizeof(rainVertices), VBO_USAGE_STATIC);
		ws.numVertices = MAX_RAIN_VERTICES;
	}
}

void R_InitWeatherSystem()
{
	Com_Printf("Initializing weather system\n");
	tr.weatherSystem = (weatherSystem_t *)Z_Malloc(sizeof(*tr.weatherSystem), TAG_R_TERRAIN, qtrue);
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
		ri->Printf(PRINT_DEVELOPER,
			"Weather system shutdown requested, but it is already shut down.\n");
	}
}

void R_AddWeatherSurfaces()
{
	assert(tr.weatherSystem);

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

void RB_SurfaceWeather( srfWeather_t *surf )
{
	assert(tr.weatherSystem);

	const weatherSystem_t& ws = *tr.weatherSystem;
	assert(surf == &ws.weatherSurface);

	RB_EndSurface();

	DrawItem item = {};

	SamplerBindingsWriter samplerBindingsWriter;
	// FIXME: This is a bit ugly with the casting
	item.samplerBindings = samplerBindingsWriter.Finish(
		*backEndData->perFrameMemory, (int *)&item.numSamplerBindings);

	UniformDataWriter uniformDataWriter;
	uniformDataWriter.Start(&tr.weatherShader);
	uniformDataWriter.SetUniformMatrix4x4(
			UNIFORM_MODELVIEWPROJECTIONMATRIX,
			glState.modelviewProjection);
	uniformDataWriter.SetUniformVec3(
			UNIFORM_VIEWORIGIN,
			backEnd.viewParms.ori.origin);
	item.uniformData = uniformDataWriter.Finish(*backEndData->perFrameMemory);

	item.stateBits = GLS_DEPTHFUNC_LESS;
	item.cullType = CT_FRONT_SIDED;
	item.program = &tr.weatherShader;
	item.depthRange = { 0.0f, 1.0f };

	vertexAttribute_t attribs[2] = {};
	attribs[0].index = ATTR_INDEX_POSITION;
	attribs[0].numComponents = 3;
	attribs[0].offset = offsetof(rainVertex_t, position);
	attribs[0].stride = sizeof(rainVertex_t);
	attribs[0].type = GL_FLOAT;
	attribs[0].vbo = ws.vbo;
	attribs[1].index = 1;
	attribs[1].numComponents = 3;
	attribs[1].offset = offsetof(rainVertex_t, seed);
	attribs[1].stride = sizeof(rainVertex_t);
	attribs[1].type = GL_FLOAT;
	attribs[1].vbo = ws.vbo;

	const size_t numAttribs = ARRAY_LEN(attribs);
	item.numAttributes = numAttribs;
	item.attributes = ojkAllocArray<vertexAttribute_t>(
		*backEndData->perFrameMemory, numAttribs);
	memcpy(item.attributes, attribs, sizeof(*item.attributes) * numAttribs);

	item.draw.type = DRAW_COMMAND_ARRAYS;
	item.draw.numInstances = 100;
	item.draw.primitiveType = GL_POINTS;
	item.draw.params.arrays.numVertices = ws.numVertices;

	uint32_t key = RB_CreateSortKey(item, 15, SS_SEE_THROUGH);
	RB_AddDrawItem(backEndData->currentPass, key, item);
}
