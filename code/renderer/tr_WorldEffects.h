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

#include "../game/q_shared.h"		// For Vec3_t


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
