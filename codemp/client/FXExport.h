#pragma once

int	FX_RegisterEffect(const char *file);

void FX_PlayEffect( const char *file, vec3_t org, vec3_t fwd, int vol, int rad );		// builds arbitrary perp. right vector, does a cross product to define up

void FX_PlayEffectID( int id, vec3_t org, vec3_t fwd, int vol, int rad, qboolean isPortal = qfalse );		// builds arbitrary perp. right vector, does a cross product to define up
void FX_PlayEntityEffectID( int id, vec3_t org, matrix3_t axis, const int boltInfo, const int entNum, int vol, int rad );
void FX_PlayBoltedEffectID( int id, vec3_t org, const int boltInfo, CGhoul2Info_v *ghoul2, int iLooptime, qboolean isRelative );

void FX_AddScheduledEffects( qboolean portal );
void		FX_Draw2DEffects ( float screenXScale, float screenYScale );

int			FX_InitSystem( refdef_t* refdef );	// called in CG_Init to purge the fx system.
void		FX_SetRefDefFromCGame( refdef_t* refdef );
qboolean	FX_FreeSystem( void );	// ditches all active effects;
void		FX_AdjustTime( int time );
