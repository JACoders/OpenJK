
#ifndef FX_SYSTEM_H_INC
#define FX_SYSTEM_H_INC

#if !defined(G2_H_INC)
	#include "ghoul2/G2.h"
#endif

extern cvar_t	*fx_debug;

#ifdef _SOF2DEV_
extern cvar_t	*fx_freeze;
#endif

extern cvar_t	*fx_countScale;
extern cvar_t	*fx_nearCull;
extern cvar_t	*fx_flashRadius;

inline void Vector2Clear(vec2_t a)
{
	a[0] = 0.0f;
	a[1] = 0.0f;
}

inline void Vector2Set(vec2_t a,float b,float c)
{
	a[0] = b;
	a[1] = c;
}

inline void Vector2Copy(vec2_t src,vec2_t dst)
{
	dst[0] = src[0];
	dst[1] = src[1];
}

inline void Vector2MA(vec2_t src, float m, vec2_t v, vec2_t dst)
{
	dst[0] = src[0] + (m*v[0]);
	dst[1] = src[1] + (m*v[1]);
}

inline void Vector2Scale(vec2_t src,float b,vec2_t dst)
{
	dst[0] = src[0] * b;
	dst[1] = src[1] * b;
}

class SFxHelper
{
public:
	int		mTime;
	int		mOldTime;
	int		mFrameTime;
	bool	mTimeFrozen;
	float	mRealTime;
	refdef_t*	refdef;
#ifdef _DEBUG
	int		mMainRefs;
	int		mMiniRefs;
#endif

public:
	SFxHelper(void);

	inline	int	GetTime(void) { return mTime; }
	inline	int	GetFrameTime(void) { return mFrameTime; }

	void	ReInit(refdef_t* pRefdef);
	void	AdjustTime( int time );

	// These functions are wrapped and used by the fx system in case it makes things a bit more portable
	void	Print( const char *msg, ... );

	// File handling
	inline	int		OpenFile( const char *path, fileHandle_t *fh, int mode )
	{
		return FS_FOpenFileByMode( path, fh, FS_READ );
	}
	inline	int		ReadFile( void *data, int len, fileHandle_t fh )
	{
		FS_Read2( data, len, fh );
		return 1;
	}
	inline	void	CloseFile( fileHandle_t fh )
	{
		FS_FCloseFile( fh );
	}

	// Sound
	inline	void	PlaySound( vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfxHandle, int volume, int radius )
	{
		//S_StartSound( origin, ENTITYNUM_NONE, CHAN_AUTO, sfxHandle, volume, radius );
		S_StartSound( origin, ENTITYNUM_NONE, CHAN_AUTO, sfxHandle );
	}
	inline	void	PlayLocalSound(sfxHandle_t sfxHandle, int entchannel)
	{
		//S_StartSound( origin, ENTITYNUM_NONE, CHAN_AUTO, sfxHandle, volume, radius );
		S_StartLocalSound(sfxHandle, entchannel);
	}
	inline	int		RegisterSound( const char *sound )
	{
		return S_RegisterSound( sound );
	}

	// Physics/collision
	inline	void	Trace( trace_t &tr, vec3_t start, vec3_t min, vec3_t max, vec3_t end, int skipEntNum, int flags )
	{
		TCGTrace		*td = (TCGTrace *)cl.mSharedMemory;

		if ( !min )
		{
			min = vec3_origin;
		}

		if ( !max )
		{
			max = vec3_origin;
		}

		memset(td, sizeof(*td), 0);
		VectorCopy(start, td->mStart);
		VectorCopy(min, td->mMins);
		VectorCopy(max, td->mMaxs);
		VectorCopy(end, td->mEnd);
		td->mSkipNumber = skipEntNum;
		td->mMask = flags;

		VM_Call( cgvm, CG_TRACE );

		tr = td->mResult;
	}

	inline	void	G2Trace( trace_t &tr, vec3_t start, vec3_t min, vec3_t max, vec3_t end, int skipEntNum, int flags )
	{
		TCGTrace		*td = (TCGTrace *)cl.mSharedMemory;

		if ( !min )
		{
			min = vec3_origin;
		}

		if ( !max )
		{
			max = vec3_origin;
		}

		memset(td, sizeof(*td), 0);
		VectorCopy(start, td->mStart);
		VectorCopy(min, td->mMins);
		VectorCopy(max, td->mMaxs);
		VectorCopy(end, td->mEnd);
		td->mSkipNumber = skipEntNum;
		td->mMask = flags;

		VM_Call( cgvm, CG_G2TRACE );

		tr = td->mResult;
	}

	inline	void	AddGhoul2Decal(int shader, vec3_t start, vec3_t dir, float size)
	{
		TCGG2Mark		*td = (TCGG2Mark *)cl.mSharedMemory;

		td->size = size;
		td->shader = shader;
		VectorCopy(start, td->start);
		VectorCopy(dir, td->dir);

		VM_Call(cgvm, CG_G2MARK);
	}

	inline	void	AddFxToScene( refEntity_t *ent )
	{
#ifdef _DEBUG
		mMainRefs++;

		assert(!ent || ent->renderfx >= 0);
#endif
		re.AddRefEntityToScene( ent );
	}
	inline	void	AddFxToScene( miniRefEntity_t *ent )
	{
#ifdef _DEBUG
		mMiniRefs++;

		assert(!ent || ent->renderfx >= 0);
#endif
		re.AddMiniRefEntityToScene( ent );
	}
#ifndef VV_LIGHTING
	inline	void	AddLightToScene( vec3_t org, float radius, float red, float green, float blue )
	{
		re.AddLightToScene(	org, radius, red, green, blue );
	}
#endif

	inline	int		RegisterShader( const char *shader )
	{
		return re.RegisterShader( shader );
	}
	inline	int		RegisterModel( const char *model )
	{
		return re.RegisterModel( model );
	}

	inline	void	AddPolyToScene( int shader, int count, polyVert_t *verts )
	{
		re.AddPolyToScene( shader, count, verts, 1 );
	}

	inline void AddDecalToScene ( qhandle_t shader, const vec3_t origin, const vec3_t dir, float orientation, float r, float g, float b, float a, qboolean alphaFade, float radius, qboolean temporary )
	{
		re.AddDecalToScene ( shader, origin, dir, orientation, r, g, b, a, alphaFade, radius, temporary );
	}

	void	CameraShake( vec3_t origin, float intensity, int radius, int time );
	qboolean SFxHelper::GetOriginAxisFromBolt(CGhoul2Info_v *pGhoul2, int mEntNum, int modelNum, int boltNum, vec3_t /*out*/origin, vec3_t /*out*/axis[3]);
};

extern SFxHelper	theFxHelper;

#endif // FX_SYSTEM_H_INC
