//Anything above this #include will be ignored by the compiler
#include "qcommon/exe_headers.h"

#include "client.h"
#include "FxScheduler.h"

//#define __FXCHECKER

#ifdef __FXCHECKER
	#include <float.h>
#endif // __FXCHECKER

int	FX_RegisterEffect(const char *file)
{
	return theFxScheduler.RegisterEffect(file, true);
}

void FX_PlayEffect( const char *file, vec3_t org, vec3_t fwd, int vol, int rad )
{
#ifdef __FXCHECKER
	if (_isnan(org[0]) || _isnan(org[1]) || _isnan(org[2]))
	{
		assert(0);
	}
	if (_isnan(fwd[0]) || _isnan(fwd[1]) || _isnan(fwd[2]))
	{
		assert(0);
	}
	if (fabs(fwd[0]) < 0.1 && fabs(fwd[1]) < 0.1 && fabs(fwd[2]) < 0.1)
	{
		assert(0);
	}
#endif // __FXCHECKER

	theFxScheduler.PlayEffect(file, org, fwd, vol, rad);
}

void FX_PlayEffectID( int id, vec3_t org, vec3_t fwd, int vol, int rad, qboolean isPortal )
{
#ifdef __FXCHECKER
	if (_isnan(org[0]) || _isnan(org[1]) || _isnan(org[2]))
	{
		assert(0);
	}
	if (_isnan(fwd[0]) || _isnan(fwd[1]) || _isnan(fwd[2]))
	{
		assert(0);
	}
	if (fabs(fwd[0]) < 0.1 && fabs(fwd[1]) < 0.1 && fabs(fwd[2]) < 0.1)
	{
		assert(0);
	}
#endif // __FXCHECKER

	theFxScheduler.PlayEffect(id, org, fwd, vol, rad, !!isPortal );
}

void FX_PlayBoltedEffectID( int id, vec3_t org,
						   const int boltInfo, CGhoul2Info_v *ghoul2, int iLooptime, qboolean isRelative )
{
	theFxScheduler.PlayEffect(id, org, 0, boltInfo, ghoul2, -1, -1, -1, qfalse, iLooptime, !!isRelative  );
}

void FX_PlayEntityEffectID( int id, vec3_t org,
						matrix3_t axis, const int boltInfo, const int entNum, int vol, int rad )
{
#ifdef __FXCHECKER
	if (_isnan(org[0]) || _isnan(org[1]) || _isnan(org[2]))
	{
		assert(0);
	}
#endif // __FXCHECKER

	theFxScheduler.PlayEffect(id, org, axis, boltInfo, 0, -1, vol, rad );
}

void FX_AddScheduledEffects( qboolean portal )
{
	theFxScheduler.AddScheduledEffects(!!portal);
}

void FX_Draw2DEffects( float screenXScale, float screenYScale )
{
	theFxScheduler.Draw2DEffects( screenXScale, screenYScale );
}

int FX_InitSystem( refdef_t* refdef )
{
	return FX_Init( refdef );
}

void FX_SetRefDefFromCGame( refdef_t* refdef )
{
	FX_SetRefDef( refdef );
}

qboolean FX_FreeSystem( void )
{
	return (qboolean)FX_Free( true );
}

void FX_AdjustTime( int time )
{
	theFxHelper.AdjustTime(time);
}
