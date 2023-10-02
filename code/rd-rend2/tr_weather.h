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
#pragma once

#include "qcommon/qcommon.h"
#include "tr_local.h"

#define MAX_WINDOBJECTS 10
#define MAX_WEATHER_ZONES 100

enum weatherType_t
{
	WEATHER_RAIN,
	WEATHER_SNOW,
	WEATHER_SPACEDUST,
	WEATHER_SAND,
	WEATHER_FOG,

	NUM_WEATHER_TYPES
};

const int maxWeatherTypeParticles[NUM_WEATHER_TYPES] = {
	30000,
	10000,
	5000,
	1000,
	1000
};

struct weatherObject_t
{
	VBO_t *lastVBO;
	VBO_t *vbo;
	unsigned vboLastUpdateFrame;
	vertexAttribute_t attribsTemplate[2];

	bool active;

	float	gravity;
	float	fadeDistance;
	float	velocityOrientationScale;
	int		particleCount;
	image_t *drawImage;
	vec4_t  color;
	vec2_t	size;
};

struct windObject_t
{
	vec3_t currentVelocity;
	vec3_t targetVelocity;
	vec3_t maxVelocity;
	vec3_t minVelocity;
	float chanceOfDeadTime;
	vec2_t deadTimeMinMax;
	int targetVelocityTimeRemaining;
};

struct weatherBrushes_t
{
	uint8_t	numPlanes;
	vec4_t	planes[64];

};

enum weatherBrushType_t
{
	WEATHER_BRUSHES_NONE,
	WEATHER_BRUSHES_OUTSIDE,
	WEATHER_BRUSHES_INSIDE,

	NUM_WEATHER_BRUSH_TYPES
};

struct weatherSystem_t
{
	weatherObject_t weatherSlots[NUM_WEATHER_TYPES];
	windObject_t windSlots[MAX_WINDOBJECTS];
	weatherBrushes_t weatherBrushes[MAX_WEATHER_ZONES * 2];
	weatherBrushType_t weatherBrushType = WEATHER_BRUSHES_NONE;

	int activeWeatherTypes = 0;
	int activeWindObjects = 0;
	int numWeatherBrushes = 0;
	bool frozen;

	srfWeather_t weatherSurface;

	vec3_t		constWindDirection;
	vec3_t		windDirection;
	float		windSpeed;

	float		weatherMVP[16];
};
struct srfWeather_t;

void R_InitWeatherSystem();
void R_InitWeatherForMap();
void R_AddWeatherSurfaces();
void R_AddWeatherBrush(uint8_t numPlanes, vec4_t *planes);
void R_ShutdownWeatherSystem();
void RB_SurfaceWeather( srfWeather_t *surfaceType );

void R_WorldEffect_f(void);
