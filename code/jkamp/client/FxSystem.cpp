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

#include "client.h"
#include "cl_cgameapi.h"
#include "FxScheduler.h"
#include "ghoul2/G2.h"

cvar_t	*fx_debug;
#ifdef _DEBUG
cvar_t	*fx_freeze;
#endif
cvar_t	*fx_countScale;
cvar_t	*fx_nearCull;

#define DEFAULT_EXPLOSION_RADIUS	512

// Stuff for the FxHelper
//------------------------------------------------------
SFxHelper::SFxHelper() :
	mTime(0),
	mOldTime(0),
	mFrameTime(0),
	mTimeFrozen(false),
	refdef(0)
{
}

void SFxHelper::ReInit(refdef_t* pRefdef)
{
	mTime = 0;
	mOldTime = 0;
	mFrameTime = 0;
	mTimeFrozen = false;
	refdef = pRefdef;
}

//------------------------------------------------------
void SFxHelper::Print( const char *msg, ... )
{
	va_list		argptr;
	char		text[1024];

	va_start( argptr, msg );
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end( argptr );

	Com_DPrintf( text );
}

//------------------------------------------------------
void SFxHelper::AdjustTime( int frametime )
{
#ifdef _DEBUG
	if ( fx_freeze->integer || ( frametime <= 0 ))
#else
	if ( frametime <= 0 )
#endif
	{
		// Allow no time progression when we are paused.
		mFrameTime = 0;
		mRealTime = 0.0f;
	}
	else
	{
		mOldTime = mTime;
		mTime = frametime;
		mFrameTime = mTime - mOldTime;

		mRealTime = mFrameTime * 0.001f;


/*		mFrameTime = frametime;
		mTime += mFrameTime;
		mRealTime = mFrameTime * 0.001f;*/

//		mHalfRealTimeSq = mRealTime * mRealTime * 0.5f;
	}
}

//------------------------------------------------------
void SFxHelper::CameraShake( vec3_t origin, float intensity, int radius, int time )
{
	TCGCameraShake	*data = (TCGCameraShake *)cl.mSharedMemory;

	VectorCopy(origin, data->mOrigin);
	data->mIntensity = intensity;
	data->mRadius = radius;
	data->mTime = time;

	CGVM_CameraShake();
}

//------------------------------------------------------
qboolean SFxHelper::GetOriginAxisFromBolt(CGhoul2Info_v *pGhoul2, int mEntNum, int modelNum, int boltNum, vec3_t /*out*/origin, vec3_t /*out*/axis[3])
{
	qboolean doesBoltExist;
	mdxaBone_t 		boltMatrix;
	TCGGetBoltData	*data = (TCGGetBoltData*)cl.mSharedMemory;
	data->mEntityNum = mEntNum;
	CGVM_GetLerpData();//this func will zero out pitch and roll for players, and ridable vehicles

	//Fixme: optimize these VM calls away by storing

	// go away and get me the bolt position for this frame please
	doesBoltExist = re->G2API_GetBoltMatrix(*pGhoul2, modelNum, boltNum,
		&boltMatrix, data->mAngles, data->mOrigin, theFxHelper.mOldTime, 0, data->mScale);

	if (doesBoltExist)
	{	// set up the axis and origin we need for the actual effect spawning
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
	}
	return doesBoltExist;
}
