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

#pragma once

#include "client/cl_cgameapi.h"
#include "ghoul2/G2.h"

extern cvar_t	*fx_debug;

#ifdef _DEBUG
extern cvar_t	*fx_freeze;
#endif

extern cvar_t	*fx_countScale;
extern cvar_t	*fx_nearCull;

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
	SFxHelper();

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
		FS_Read( data, len, fh );
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

		memset(td, 0, sizeof(*td));
		VectorCopy(start, td->mStart);
		VectorCopy(min, td->mMins);
		VectorCopy(max, td->mMaxs);
		VectorCopy(end, td->mEnd);
		td->mSkipNumber = skipEntNum;
		td->mMask = flags;

		CGVM_Trace();

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

		memset(td, 0, sizeof(*td));
		VectorCopy(start, td->mStart);
		VectorCopy(min, td->mMins);
		VectorCopy(max, td->mMaxs);
		VectorCopy(end, td->mEnd);
		td->mSkipNumber = skipEntNum;
		td->mMask = flags;

		CGVM_G2Trace();

		tr = td->mResult;
	}

	inline	void	AddGhoul2Decal(int shader, vec3_t start, vec3_t dir, float size)
	{
		TCGG2Mark		*td = (TCGG2Mark *)cl.mSharedMemory;

		td->size = size;
		td->shader = shader;
		VectorCopy(start, td->start);
		VectorCopy(dir, td->dir);

		CGVM_G2Mark();
	}

	inline	void	AddFxToScene( refEntity_t *ent )
	{
#ifdef _DEBUG
		mMainRefs++;

		assert(!ent || ent->renderfx >= 0);
#endif
		re->AddRefEntityToScene( ent );
	}
	inline	void	AddFxToScene( miniRefEntity_t *ent )
	{
#ifdef _DEBUG
		mMiniRefs++;

		assert(!ent || ent->renderfx >= 0);
#endif
		re->AddMiniRefEntityToScene( ent );
	}
	inline	void	AddLightToScene( vec3_t org, float radius, float red, float green, float blue )
	{
		re->AddLightToScene(	org, radius, red, green, blue );
	}

	inline	int		RegisterShader( const char *shader )
	{
		return re->RegisterShader( shader );
	}
	inline	int		RegisterModel( const char *model )
	{
		return re->RegisterModel( model );
	}

	inline	void	AddPolyToScene( int shader, int count, polyVert_t *verts )
	{
		re->AddPolyToScene( shader, count, verts, 1 );
	}

	inline void AddDecalToScene ( qhandle_t shader, const vec3_t origin, const vec3_t dir, float orientation, float r, float g, float b, float a, qboolean alphaFade, float radius, qboolean temporary )
	{
		re->AddDecalToScene ( shader, origin, dir, orientation, r, g, b, a, alphaFade, radius, temporary );
	}

	void	CameraShake( vec3_t origin, float intensity, int radius, int time );
	qboolean GetOriginAxisFromBolt(CGhoul2Info_v *pGhoul2, int mEntNum, int modelNum, int boltNum, vec3_t /*out*/origin, vec3_t /*out*/axis[3]);
};

extern SFxHelper	theFxHelper;
