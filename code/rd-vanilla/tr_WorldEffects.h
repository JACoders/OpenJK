/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

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

////////////////////////////////////////////////////////////////////////////////////////
// RAVEN SOFTWARE - STAR WARS: JK II
//  (c) 2002 Activision
//
// World Effects
//
////////////////////////////////////////////////////////////////////////////////////////
#pragma once
#if !defined __TR_WORLDEFFECTS_H
#define __TR_WORLDEFFECTS_H

#include "../qcommon/q_shared.h"		// For Vec3_t


////////////////////////////////////////////////////////////////////////////////////////
// Supported Commands
////////////////////////////////////////////////////////////////////////////////////////
void R_AddWeatherZone(vec3_t mins, vec3_t maxs);

void R_InitWorldEffects(void);
void R_ShutdownWorldEffects(void);
void RB_RenderWorldEffects(void);

void R_WorldEffectCommand(const char *command);
void R_WorldEffect_f(void);

////////////////////////////////////////////////////////////////////////////////////////
// Exported Functionality
////////////////////////////////////////////////////////////////////////////////////////
bool R_GetWindVector(vec3_t windVector, vec3_t atpoint);
bool R_GetWindSpeed(float &windSpeed, vec3_t atpoint);
bool R_GetWindGusting(vec3_t atpoint);
bool R_IsOutside(vec3_t pos);
float R_IsOutsideCausingPain(vec3_t pos);
float R_GetChanceOfSaberFizz();
bool R_IsShaking(vec3_t pos);
bool R_SetTempGlobalFogColor(vec3_t color);

bool R_IsRaining();
bool R_IsPuffing();


#endif // __TR_WORLDEFFECTS_H
