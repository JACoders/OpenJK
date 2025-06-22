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

#include "common_headers.h"

#if !defined(FX_SCHEDULER_H_INC)
	#include "FxScheduler.h"
#endif

#include "cg_media.h"	//for cgs.model_draw for G2

extern vmCvar_t	fx_debug;
extern vmCvar_t	fx_freeze;

extern void CG_ExplosionEffects( vec3_t origin, float intensity, int radius, int time );

// Stuff for the FxHelper
//------------------------------------------------------
void SFxHelper::Init()
{
	mTime = 0;
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
			if ( frameTime > 300 ) // hack for returning from paused and time bursts
			{
				frameTime = 300;
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
void SFxHelper::PlaySound( const vec3_t org, int entityNum, int entchannel, int sfxHandle )
{
	cgi_S_StartSound( org, entityNum, entchannel, sfxHandle );
}

//------------------------------------------------------
void SFxHelper::PlayLocalSound( int sfxHandle, int channelNum )
{
	cgi_S_StartLocalSound(sfxHandle, channelNum);
}

//------------------------------------------------------
void SFxHelper::Trace( trace_t *tr, vec3_t start, vec3_t min, vec3_t max,
						vec3_t end, int skipEntNum, int flags )
{
	CG_Trace( tr, start, min, max, end, skipEntNum, flags );
}

void SFxHelper::G2Trace( trace_t *tr, vec3_t start, vec3_t min, vec3_t max,
						vec3_t end, int skipEntNum, int flags )
{
	//CG_Trace( tr, start, min, max, end, skipEntNum, flags, G2_COLLIDE );
	gi.trace(tr, start, NULL, NULL, end, skipEntNum, flags, G2_COLLIDE, 0);
}

//------------------------------------------------------
void SFxHelper::AddFxToScene( refEntity_t *ent )
{
	cgi_R_AddRefEntityToScene( ent );
}

//------------------------------------------------------
int SFxHelper::RegisterShader( const gsl::cstring_span& shader )
{
	// TODO: it would be nice to change the ABI here to allow for passing of string views
	return cgi_R_RegisterShader( std::string( shader.begin(), shader.end() ).c_str() );
}

//------------------------------------------------------
int SFxHelper::RegisterSound( const gsl::cstring_span& sound )
{
	// TODO: it would be nice to change the ABI here to allow for passing of string views
	return cgi_S_RegisterSound( std::string( sound.begin(), sound.end() ).c_str() );
}

//------------------------------------------------------
int SFxHelper::RegisterModel( const gsl::cstring_span& model )
{
	return cgi_R_RegisterModel( std::string( model.begin(), model.end() ).c_str() );
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

//------------------------------------------------------
int SFxHelper::GetOriginAxisFromBolt(const centity_t &cent, int modelNum, int boltNum, vec3_t /*out*/origin, vec3_t /*out*/axis[3])
{
	if ((cg.time-cent.snapShotTime) > 200)
	{ //you were added more than 200ms ago, so I say you are no longer valid/in our snapshot.
		return 0;
	}

	int doesBoltExist;
	mdxaBone_t 	boltMatrix;
	vec3_t	G2Angles = {cent.lerpAngles[0] , cent.lerpAngles[1], cent.lerpAngles[2]};
	if ( cent.currentState.eType == ET_PLAYER )
	{//players use cent.renderAngles
		VectorCopy( cent.renderAngles, G2Angles );

		if ( cent.gent //has a game entity
			&& cent.gent->s.m_iVehicleNum != 0 //in a vehicle
			&& cent.gent->m_pVehicle //have a valid vehicle pointer
			&& cent.gent->m_pVehicle->m_pVehicleInfo->type != VH_FIGHTER //it's not a fighter
			&& cent.gent->m_pVehicle->m_pVehicleInfo->type != VH_SPEEDER //not a speeder
			)
		{
			G2Angles[PITCH]=0;
			G2Angles[ROLL] =0;
		}
	}

	// go away and get me the bolt position for this frame please
	doesBoltExist = gi.G2API_GetBoltMatrix(cent.gent->ghoul2, modelNum,
		boltNum, &boltMatrix, G2Angles,
		cent.lerpOrigin, cg.time, cgs.model_draw,
		cent.currentState.modelScale);
	// set up the axis and origin we need for the actual effect spawning
	origin[0] = boltMatrix.matrix[0][3];
	origin[1] = boltMatrix.matrix[1][3];
	origin[2] = boltMatrix.matrix[2][3];

	axis[1][0] = boltMatrix.matrix[0][0];
	axis[1][1] = boltMatrix.matrix[1][0];
	axis[1][2] = boltMatrix.matrix[2][0];

	axis[0][0] = boltMatrix.matrix[0][1];
	axis[0][1] = boltMatrix.matrix[1][1];
	axis[0][2] = boltMatrix.matrix[2][1];

	axis[2][0] = boltMatrix.matrix[0][2];
	axis[2][1] = boltMatrix.matrix[1][2];
	axis[2][2] = boltMatrix.matrix[2][2];
	return doesBoltExist;
}