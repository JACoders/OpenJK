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

#if !defined(CG_LOCAL_H_INC)
	#include "cg_local.h"
#endif

#ifndef FX_SYSTEM_H_INC
#define FX_SYSTEM_H_INC

#include "qcommon/safe/gsl.h"


#define irand	Q_irand
#define flrand	Q_flrand

extern vmCvar_t	fx_debug;
extern vmCvar_t	fx_freeze;

extern void	CG_CalcEntityLerpPositions( centity_t * );	

struct SFxHelper
{
	int		mTime;
	int		mOldTime;
	int		mFrameTime;
	float	mFloatFrameTime;
	bool	mTimeFrozen;

	void	Init();
	void	AdjustTime( int time );

	// These functions are wrapped and used by the fx system in case it makes things a bit more portable
	void	Print( const char *msg, ... );

	// File handling
	int		OpenFile( const char *path, fileHandle_t *fh, int mode );
	int		ReadFile( void *data, int len, fileHandle_t fh );
	void	CloseFile( fileHandle_t fh );

	// Sound
	void	PlaySound( vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx );
	int		RegisterSound( const gsl::cstring_view& sound );

	// Physics/collision
	void	Trace( trace_t *tr, vec3_t start, vec3_t min, vec3_t max, vec3_t end, int skipEntNum, int flags );

	void	AddFxToScene( refEntity_t *ent );
	void	AddLightToScene( vec3_t org, float radius, float red, float green, float blue );

	int		RegisterShader( const gsl::cstring_view& shader );
	int		RegisterModel( const gsl::cstring_view& model );

	void	AddPolyToScene( int shader, int count, polyVert_t *verts );

	void	CameraShake( vec3_t origin, float intensity, int radius, int time );
};

extern SFxHelper	theFxHelper;

#endif // FX_SYSTEM_H_INC
