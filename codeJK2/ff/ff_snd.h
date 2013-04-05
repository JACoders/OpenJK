#ifndef FF_SND_H
#define FF_SND_H

#include "../ff/ff_public.h"

void FF_AddForce( ffHandle_t ff/*, int entNum, const vec3_t origin, float maxDistance, float minDistance*/ );
void FF_AddLoopingForce( ffHandle_t ff/*, int entNum, const vec3_t origin, float maxDistance, float minDistance*/ );
//void FF_Respatialize( int entNum, const vec3_t origin );
void FF_Update( void );

#endif // FF_SND_H