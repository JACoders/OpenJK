#pragma once

void R_InitWorldEffects(void);
void R_ShutdownWorldEffects(void);
void RB_RenderWorldEffects(void);

void RE_WorldEffectCommand(const char *command);
void R_WorldEffect_f(void);

bool R_GetWindVector(vec3_t windVector);
bool R_GetWindSpeed(float &windSpeed);

bool R_IsRaining();
//bool R_IsSnowing();
bool R_IsPuffing();
void RE_AddWeatherZone(vec3_t mins, vec3_t maxs);
