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

#if !defined(FX_SCHEDULER_H_INC)
	#include "FxScheduler.h"
#endif

extern vmCvar_t	fx_debug;
extern vmCvar_t	fx_freeze;

extern void CG_ExplosionEffects( vec3_t origin, float intensity, int radius, int time );

// Stuff for the FxHelper
//------------------------------------------------------
void SFxHelper::Init()
{
	mTime = 0;
	mOldTime = 0;
	mTimeFrozen = false;
}

//------------------------------------------------------
void SFxHelper::Print( const char *msg, ... )
{
#ifndef FINAL_BUILD

	va_list		argptr;
	char		text[1024];

	va_start( argptr, msg );
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end( argptr );
 
	gi.Printf( text );

#endif
}

//------------------------------------------------------
void SFxHelper::AdjustTime( int frameTime )
{
	if ( fx_freeze.integer || ( frameTime <= 0 ))
	{
		// Allow no time progression when we are paused.
		mFrameTime = 0;
		mFloatFrameTime = 0.0f;
	}
	else
	{
		if ( !cg_paused.integer )
		{
			if ( frameTime > 1500 ) // hack
			{
				frameTime = 200;
			}

			mFrameTime = frameTime;
			mFloatFrameTime = mFrameTime * 0.001f;
			mTime += mFrameTime;
		}
	}
}

//------------------------------------------------------
int SFxHelper::OpenFile( const char *file, fileHandle_t *fh, int mode )
{
//	char path[256];

//	sprintf( path, "%s/%s", FX_FILE_PATH, file );
	return cgi_FS_FOpenFile( file, fh, FS_READ );
}

//------------------------------------------------------
int SFxHelper::ReadFile( void *data, int len, fileHandle_t fh )
{
	return cgi_FS_Read( data, len, fh );
}

//------------------------------------------------------
void SFxHelper::CloseFile( fileHandle_t fh )
{
	cgi_FS_FCloseFile( fh );
}

//------------------------------------------------------
void SFxHelper::PlaySound( vec3_t org, int entityNum, int entchannel, int sfxHandle )
{
	cgi_S_StartSound( org, entityNum, entchannel, sfxHandle );
}

//------------------------------------------------------
void SFxHelper::Trace( trace_t *tr, vec3_t start, vec3_t min, vec3_t max, 
						vec3_t end, int skipEntNum, int flags )
{
	CG_Trace( tr, start, min, max, end, skipEntNum, flags );
}

//------------------------------------------------------
void SFxHelper::AddFxToScene( refEntity_t *ent )
{
	cgi_R_AddRefEntityToScene( ent );
}

//------------------------------------------------------
int SFxHelper::RegisterShader( const char *shader )
{
	return cgi_R_RegisterShader( shader );
}

//------------------------------------------------------
int SFxHelper::RegisterSound( const char *sound )
{
	return cgi_S_RegisterSound( sound );
}

//------------------------------------------------------
int SFxHelper::RegisterModel( const char *model )
{
	return cgi_R_RegisterModel( model );
}

//------------------------------------------------------
void SFxHelper::AddLightToScene( vec3_t org, float radius, float red, float green, float blue )
{
	cgi_R_AddLightToScene( org, radius, red, green, blue );
}

//------------------------------------------------------
void SFxHelper::AddPolyToScene( int shader, int count, polyVert_t *verts )
{
	cgi_R_AddPolyToScene( shader, count, verts );
}

//------------------------------------------------------
void SFxHelper::CameraShake( vec3_t origin, float intensity, int radius, int time )
{
	CG_ExplosionEffects( origin, intensity, radius, time );
}