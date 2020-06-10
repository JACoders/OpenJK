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
#include "FxScheduler.h"

//------------------------------------------------------
// CPrimitiveTemplate
//	Set up our minimal default values
//
// Input:
//	none
//
// Return:
//	none
//------------------------------------------------------
CPrimitiveTemplate::CPrimitiveTemplate()
{
	// We never start out as a copy or with a name
	mCopy = false;
	mName[0] = 0;
	mCullRange = 0; // no distance culling

	mFlags = mSpawnFlags = 0;
	mElasticity.SetRange(0.1f, 0.1f);

	mSoundVolume = -1;
	mSoundRadius = -1;

	mMatImpactFX = MATIMPACTFX_NONE;

	mLife.SetRange( 50.0f, 50.0f );
	mSpawnCount.SetRange( 1.0f, 1.0f );
	mRadius.SetRange( 10.0f, 10.0f );
	mHeight.SetRange( 10.0f, 10.0f );
	mWindModifier.SetRange( 1.0f, 1.0f );

	VectorSet( mMin, 0.0f, 0.0f, 0.0f );
	VectorSet( mMax, 0.0f, 0.0f, 0.0f );

	mRedStart.SetRange( 1.0f, 1.0f );
	mGreenStart.SetRange( 1.0f, 1.0f );
	mBlueStart.SetRange( 1.0f, 1.0f );

	mRedEnd.SetRange( 1.0f, 1.0f );
	mGreenEnd.SetRange( 1.0f, 1.0f );
	mBlueEnd.SetRange( 1.0f, 1.0f );

	mAlphaStart.SetRange( 1.0f, 1.0f );
	mAlphaEnd.SetRange( 1.0f, 1.0f );

	mSizeStart.SetRange( 1.0f, 1.0f );
	mSizeEnd.SetRange( 1.0f, 1.0f );

	mSize2Start.SetRange( 1.0f, 1.0f );
	mSize2End.SetRange( 1.0f, 1.0f );

	mLengthStart.SetRange( 1.0f, 1.0f );
	mLengthEnd.SetRange( 1.0f, 1.0f );

	mTexCoordS.SetRange( 1.0f, 1.0f );
	mTexCoordT.SetRange( 1.0f, 1.0f );

	mVariance.SetRange( 1.0f, 1.0f );
	mDensity.SetRange( 10.0f, 10.0f );// default this high so it doesn't do bad things
}

//-----------------------------------------------------------
CPrimitiveTemplate &CPrimitiveTemplate::operator=(const CPrimitiveTemplate &that)
{
	// I'm assuming that doing a memcpy wouldn't work here
	// If you are looking at this and know a better way to do this, please tell me.
	strcpy( mName, that.mName );

	mType				= that.mType;

	mSpawnDelay			= that.mSpawnDelay;
	mSpawnCount			= that.mSpawnCount;
	mLife				= that.mLife;
	mCullRange			= that.mCullRange;

	mMediaHandles		= that.mMediaHandles;
	mImpactFxHandles	= that.mImpactFxHandles;
	mDeathFxHandles		= that.mDeathFxHandles;
	mEmitterFxHandles	= that.mEmitterFxHandles;
	mPlayFxHandles		= that.mPlayFxHandles;

	mFlags				= that.mFlags;
	mSpawnFlags			= that.mSpawnFlags;

	VectorCopy( that.mMin, mMin );
	VectorCopy( that.mMax, mMax );

	mOrigin1X			= that.mOrigin1X;
	mOrigin1Y			= that.mOrigin1Y;
	mOrigin1Z			= that.mOrigin1Z;

	mOrigin2X			= that.mOrigin2X;
	mOrigin2Y			= that.mOrigin2Y;
	mOrigin2Z			= that.mOrigin2Z;

	mRadius				= that.mRadius;
	mHeight				= that.mHeight;
	mWindModifier		= that.mWindModifier;

	mRotation			= that.mRotation;
	mRotationDelta		= that.mRotationDelta;

	mAngle1				= that.mAngle1;
	mAngle2				= that.mAngle2;
	mAngle3				= that.mAngle3;

	mAngle1Delta		= that.mAngle1Delta;
	mAngle2Delta		= that.mAngle2Delta;
	mAngle3Delta		= that.mAngle3Delta;

	mVelX				= that.mVelX;
	mVelY				= that.mVelY;
	mVelZ				= that.mVelZ;

	mAccelX				= that.mAccelX;
	mAccelY				= that.mAccelY;
	mAccelZ				= that.mAccelZ;

	mGravity			= that.mGravity;

	mDensity			= that.mDensity;
	mVariance			= that.mVariance;

	mRedStart			= that.mRedStart;
	mGreenStart			= that.mGreenStart;
	mBlueStart			= that.mBlueStart;

	mRedEnd				= that.mRedEnd;
	mGreenEnd			= that.mGreenEnd;
	mBlueEnd			= that.mBlueEnd;

	mRGBParm			= that.mRGBParm;

	mAlphaStart			= that.mAlphaStart;
	mAlphaEnd			= that.mAlphaEnd;
	mAlphaParm			= that.mAlphaParm;

	mSizeStart			= that.mSizeStart;
	mSizeEnd			= that.mSizeEnd;
	mSizeParm			= that.mSizeParm;

	mSize2Start			= that.mSize2Start;
	mSize2End			= that.mSize2End;
	mSize2Parm			= that.mSize2Parm;

	mLengthStart		= that.mLengthStart;
	mLengthEnd			= that.mLengthEnd;
	mLengthParm			= that.mLengthParm;

	mTexCoordS			= that.mTexCoordS;
	mTexCoordT			= that.mTexCoordT;

	mElasticity			= that.mElasticity;

	mSoundRadius		= that.mSoundRadius;
	mSoundVolume		= that.mSoundVolume;

	return *this;
}

//------------------------------------------------------
// ParseFloat
//	Removes up to two values from a passed in string and
//	sets these values into the passed in min and max
//	fields.  if no max is present, min is copied into it.
//
// input:
//	string that contains up to two float values
//  min & max are used to return the parse values
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseFloat( const char *val, float *min, float *max )
{
	// We don't allow passing in a null for either of the fields
	if ( min == 0 || max == 0 )
	{ // failue
		return false;
	}

	// attempt to read out the values
	int v = sscanf( val, "%f %f", min, max );

	if ( v == 0 )
	{ // nothing was there, failure
		return false;
	}
	else if ( v == 1 )
	{ // only one field entered, this is ok, but we should copy min into max
		*max = *min;
	}

	return true;
}


//------------------------------------------------------
// ParseVector
//	Removes up to six values from a passed in string and
//	sets these values into the passed in min and max vector
//	fields. if no max is present, min is copied into it.
//
// input:
//	string that contains up to six float values
//  min & max are used to return the parse values
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseVector( const char *val, vec3_t min, vec3_t max )
{
	// we don't allow passing in a null
	if ( min == 0 || max == 0 )
	{
		return false;
	}

	// attempt to read out our values
	int v = sscanf( val, "%f %f %f   %f %f %f", &min[0], &min[1], &min[2], &max[0], &max[1], &max[2] );

	// Check for completeness
	if ( v < 3 || v == 4 || v == 5 )
	{ // not a complete value
		return false;
	}
	else if ( v == 3 )
	{ // only a min was entered, so copy the result into max
		VectorCopy( min, max );
	}

	return true;
}

//------------------------------------------------------
// ParseGroupFlags
//	Group flags are generic in nature, so we can easily
//	use a generic function to parse them in, then the
//	caller can shift them into the appropriate range.
//
// input:
//	string that contains the flag strings
//  *flags returns the set bit flags
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseGroupFlags( const char *val, int *flags )
{
	// Must pass in a non-null pointer
	if ( flags == 0 )
	{
		return false;
	}

	char	flag[][32] = {"\0","\0","\0","0"};
	bool	ok = true;

	// For a sub group, really you probably only have one or two flags set
	int v = sscanf( val, "%s %s %s %s", flag[0], flag[1], flag[2], flag[3] );

	// Clear out the flags field, then convert the flag string to an actual value ( use generic flags )
	*flags = 0;

	for ( int i = 0; i < 4; i++ )
	{
		if ( i + 1 > v )
		{
			return true;
		}

		if ( !Q_stricmp( flag[i], "linear" ) )
			*flags |= FX_LINEAR;
		else if ( !Q_stricmp( flag[i], "nonlinear" ) )
			*flags |= FX_NONLINEAR;
		else if ( !Q_stricmp( flag[i], "wave" ) )
			*flags |= FX_WAVE;
		else if ( !Q_stricmp( flag[i], "random" ) )
			*flags |= FX_RAND;
		else if ( !Q_stricmp( flag[i], "clamp" ) )
			*flags |= FX_CLAMP;

		else
		{ // we have badness going on, but continue on in case there are any valid fields in here
			ok = false;
		}
	}

	return ok;
}

//------------------------------------------------------
// ParseMin
//	Reads in a min bounding box field in vector format
//
// input:
//	string that contains three float values
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseMin( const char *val )
{
	vec3_t min;

	if ( ParseVector( val, min, min ) == true )
	{
		VectorCopy( min, mMin );

		// We assume that if a min is being set that we are using physics and a bounding box
		mFlags |= (FX_USE_BBOX | FX_APPLY_PHYSICS);
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseMax
//	Reads in a max bounding box field in vector format
//
// input:
//	string that contains three float values
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseMax( const char *val )
{
	vec3_t max;

	if ( ParseVector( val, max, max ) == true )
	{
		VectorCopy( max, mMax );

		// We assume that if a max is being set that we are using physics and a bounding box
		mFlags |= (FX_USE_BBOX | FX_APPLY_PHYSICS);
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseLife
//	Reads in a ranged life value
//
// input:
//	string that contains a float range ( two vals )
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseLife( const char *val )
{
	float min, max;

	if ( ParseFloat( val, &min, &max ) == true )
	{
		mLife.SetRange( min, max );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseDelay
//	Reads in a ranged delay value
//
// input:
//	string that contains a float range ( two vals )
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseDelay( const char *val )
{
	float min, max;

	if ( ParseFloat( val, &min, &max ) == true )
	{
		mSpawnDelay.SetRange( min, max );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseCount
//	Reads in a ranged count value
//
// input:
//	string that contains a float range ( two vals )
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseCount( const char *val )
{
	float min, max;

	if ( ParseFloat( val, &min, &max ) == true )
	{
		mSpawnCount.SetRange( min, max );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseElasticity
//	Reads in a ranged elasticity value
//
// input:
//	string that contains a float range ( two vals )
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseElasticity( const char *val )
{
	float min, max;

	if ( ParseFloat( val, &min, &max ) == true )
	{
		mElasticity.SetRange( min, max );

		// We assume that if elasticity is set that we are using physics, but don't assume we are
		//	using a bounding box unless a min/max are explicitly set
		mFlags |= FX_APPLY_PHYSICS;
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseOrigin1
//	Reads in an origin field in vector format
//
// input:
//	string that contains three float values
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseOrigin1( const char *val )
{
	vec3_t min, max;

	if ( ParseVector( val, min, max ) == true )
	{
		mOrigin1X.SetRange( min[0], max[0] );
		mOrigin1Y.SetRange( min[1], max[1] );
		mOrigin1Z.SetRange( min[2], max[2] );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseOrigin2
//	Reads in an origin field in vector format
//
// input:
//	string that contains three float values
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseOrigin2( const char *val )
{
	vec3_t min, max;

	if ( ParseVector( val, min, max ) == true )
	{
		mOrigin2X.SetRange( min[0], max[0] );
		mOrigin2Y.SetRange( min[1], max[1] );
		mOrigin2Z.SetRange( min[2], max[2] );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseRadius
//	Reads in a ranged radius value
//
// input:
//	string that contains one or two floats
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseRadius( const char *val )
{
	float min, max;

	if ( ParseFloat( val, &min, &max ) == true )
	{
		mRadius.SetRange( min, max );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseHeight
//	Reads in a ranged height value
//
// input:
//	string that contains one or two floats
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseHeight( const char *val )
{
	float min, max;

	if ( ParseFloat( val, &min, &max ) == true )
	{
		mHeight.SetRange( min, max );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseWindModifier
//	Reads in a ranged wind modifier value
//
// input:
//	string that contains one or two floats
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseWindModifier( const char *val )
{
	float min, max;

	if ( ParseFloat( val, &min, &max ) == true )
	{
		mWindModifier.SetRange( min, max );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseRotation
//	Reads in a ranged rotation value
//
// input:
//	string that contains one or two floats
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseRotation( const char *val )
{
	float min, max;

	if ( ParseFloat( val, &min, &max ) == qtrue )
	{
		mRotation.SetRange( min, max );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseRotationDelta
//	Reads in a ranged rotationDelta value
//
// input:
//	string that contains one or two floats
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseRotationDelta( const char *val )
{
	float min, max;

	if ( ParseFloat( val, &min, &max ) == qtrue )
	{
		mRotationDelta.SetRange( min, max );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseAngle
//	Reads in a ranged angle field in vector format
//
// input:
//	string that contains one or two vectors
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseAngle( const char *val )
{
	vec3_t min, max;

	if ( ParseVector( val, min, max ) == true )
	{
		mAngle1.SetRange( min[0], max[0] );
		mAngle2.SetRange( min[1], max[1] );
		mAngle3.SetRange( min[2], max[2] );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseAngleDelta
//	Reads in a ranged angleDelta field in vector format
//
// input:
//	string that contains one or two vectors
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseAngleDelta( const char *val )
{
	vec3_t min, max;

	if ( ParseVector( val, min, max ) == true )
	{
		mAngle1Delta.SetRange( min[0], max[0] );
		mAngle2Delta.SetRange( min[1], max[1] );
		mAngle3Delta.SetRange( min[2], max[2] );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseVelocity
//	Reads in a ranged velocity field in vector format
//
// input:
//	string that contains one or two vectors
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseVelocity( const char *val )
{
	vec3_t min, max;

	if ( ParseVector( val, min, max ) == true )
	{
		mVelX.SetRange( min[0], max[0] );
		mVelY.SetRange( min[1], max[1] );
		mVelZ.SetRange( min[2], max[2] );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseFlags
//	These are flags that are not specific to a group,
//	rather, they are specific to the whole primitive.
//
// input:
//	string that contains the flag strings
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseFlags( const char *val )
{
	char	flag[][32] = {"\0","\0","\0","\0","\0","\0","\0"};
	bool	ok = true;

	// For a primitive, really you probably only have two or less flags set
	int v = sscanf( val, "%s %s %s %s %s %s %s", flag[0], flag[1], flag[2], flag[3], flag[4], flag[5], flag[6] );

	for ( int i = 0; i < 7; i++ )
	{
		if ( i + 1 > v )
		{
			return true;
		}

			 if ( !Q_stricmp( flag[i], "useModel" ))
			mFlags |= FX_ATTACHED_MODEL;
		else if ( !Q_stricmp( flag[i], "useBBox" ))
			mFlags |= FX_USE_BBOX;
		else if ( !Q_stricmp( flag[i], "usePhysics" ))
			mFlags |= FX_APPLY_PHYSICS;
		else if ( !Q_stricmp( flag[i], "expensivePhysics" ))
			mFlags |= FX_EXPENSIVE_PHYSICS;
		//rww - begin g2 stuff
		else if ( !Q_stricmp( flag[i], "ghoul2Collision" ))
			mFlags |= (FX_GHOUL2_TRACE|FX_APPLY_PHYSICS|FX_EXPENSIVE_PHYSICS);
		else if ( !Q_stricmp( flag[i], "ghoul2Decals" ))
			mFlags |= FX_GHOUL2_DECALS;
		//rww - end
		else if ( !Q_stricmp( flag[i], "impactKills" ))
			mFlags |= FX_KILL_ON_IMPACT;
		else if ( !Q_stricmp( flag[i], "impactFx" ))
			mFlags |= FX_IMPACT_RUNS_FX;
		else if ( !Q_stricmp( flag[i], "deathFx" ))
			mFlags |= FX_DEATH_RUNS_FX;
		else if ( !Q_stricmp( flag[i], "useAlpha" ))
			mFlags |= FX_USE_ALPHA;
		else if ( !Q_stricmp( flag[i], "emitFx" ))
			mFlags |= FX_EMIT_FX;
		else if ( !Q_stricmp( flag[i], "depthHack" ))
			mFlags |= FX_DEPTH_HACK;
		else if ( !Q_stricmp( flag[i], "relative" ))
			mFlags |= FX_RELATIVE;
		else if ( !Q_stricmp( flag[i], "setShaderTime" ))
			mFlags |= FX_SET_SHADER_TIME;
		else if ( !Q_stricmp( flag[i], "paperPhysics" ))
			mFlags |= FX_PAPER_PHYSICS; //warning! shared flag.  You use this with a cylinder and you can expect evilness to ensue
		else if ( !Q_stricmp( flag[i], "localizedFlash" ))
			mFlags |= FX_LOCALIZED_FLASH; //warning! shared flag.  You use this with a cylinder and you can expect evilness to ensue
		else if ( !Q_stricmp( flag[i], "playerView" ))
			mFlags |= FX_PLAYER_VIEW; //warning! shared flag.  You use this with a cylinder and you can expect evilness to ensue
		else
		{ // we have badness going on, but continue on in case there are any valid fields in here
			ok = false;
		}
	}

	return ok;
}

//------------------------------------------------------
// ParseSpawnFlags
//	These kinds of flags control how things spawn.  They
//	never get passed on to a primitive.
//
// input:
//	string that contains the flag strings
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseSpawnFlags( const char *val )
{
	char	flag[][32] = {"\0","\0","\0","\0","\0","\0","\0"};
	bool	ok = true;

	// For a primitive, really you probably only have two or less flags set
	int v = sscanf( val, "%s %s %s %s %s %s %s", flag[0], flag[1], flag[2], flag[3], flag[4], flag[5], flag[6] );

	for ( int i = 0; i < 7; i++ )
	{
		if ( i + 1 > v )
		{
			return true;
		}

			 if ( !Q_stricmp( flag[i], "org2fromTrace" ) )
			mSpawnFlags |= FX_ORG2_FROM_TRACE;
		else if ( !Q_stricmp( flag[i], "traceImpactFx" ) )
			mSpawnFlags |= FX_TRACE_IMPACT_FX;
		else if ( !Q_stricmp( flag[i], "org2isOffset" ) )
			mSpawnFlags |= FX_ORG2_IS_OFFSET;
		else if ( !Q_stricmp( flag[i], "cheapOrgCalc" ) )
			mSpawnFlags |= FX_CHEAP_ORG_CALC;
		else if ( !Q_stricmp( flag[i], "cheapOrg2Calc" ) )
			mSpawnFlags |= FX_CHEAP_ORG2_CALC;
		else if ( !Q_stricmp( flag[i], "absoluteVel" ) )
			mSpawnFlags |= FX_VEL_IS_ABSOLUTE;
		else if ( !Q_stricmp( flag[i], "absoluteAccel" ) )
			mSpawnFlags |= FX_ACCEL_IS_ABSOLUTE;
		else if ( !Q_stricmp( flag[i], "orgOnSphere" ) ) // sphere/ellipsoid
			mSpawnFlags |= FX_ORG_ON_SPHERE;
		else if ( !Q_stricmp( flag[i], "orgOnCylinder" ) ) // cylinder/disk
			mSpawnFlags |= FX_ORG_ON_CYLINDER;
		else if ( !Q_stricmp( flag[i], "axisFromSphere" ) )
			mSpawnFlags |= FX_AXIS_FROM_SPHERE;
		else if ( !Q_stricmp( flag[i], "randrotaroundfwd" ) )
			mSpawnFlags |= FX_RAND_ROT_AROUND_FWD;
		else if ( !Q_stricmp( flag[i], "evenDistribution" ) )
			mSpawnFlags |= FX_EVEN_DISTRIBUTION;
		else if ( !Q_stricmp( flag[i], "rgbComponentInterpolation" ) )
			mSpawnFlags |= FX_RGB_COMPONENT_INTERP;
		else if ( !Q_stricmp( flag[i], "affectedByWind" ) )
			mSpawnFlags |= FX_AFFECTED_BY_WIND;
		else
		{ // we have badness going on, but continue on in case there are any valid fields in here
			ok = false;
		}
	}

	return ok;
}



bool CPrimitiveTemplate::ParseMaterialImpact(const char *val)
{
	if (!Q_stricmp(val, "shellsound"))
	{
		mMatImpactFX = MATIMPACTFX_SHELLSOUND;
	}
	else
	{
		mMatImpactFX = MATIMPACTFX_NONE;
		theFxHelper.Print( "CPrimitiveTemplate::ParseMaterialImpact -- unknown materialImpact type!\n" );
		return false;
	}
	return true;
}


//------------------------------------------------------
// ParseAcceleration
//	Reads in a ranged acceleration field in vector format
//
// input:
//	string that contains one or two vectors
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseAcceleration( const char *val )
{
	vec3_t min, max;

	if ( ParseVector( val, min, max ) == true )
	{
		mAccelX.SetRange( min[0], max[0] );
		mAccelY.SetRange( min[1], max[1] );
		mAccelZ.SetRange( min[2], max[2] );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseGravity
//	Reads in a ranged gravity value
//
// input:
//	string that contains one or two floats
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseGravity( const char *val )
{
	float min, max;

	if ( ParseFloat( val, &min, &max ) == true )
	{
		mGravity.SetRange( min, max );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseDensity
//	Reads in a ranged density value.  Density is only
//	for emitters that are calling effects...it basically
//	specifies how often the emitter should emit fx.
//
// input:
//	string that contains one or two floats
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseDensity( const char *val )
{
	float min, max;

	if ( ParseFloat( val, &min, &max ) == true )
	{
		mDensity.SetRange( min, max );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseVariance
//	Reads in a ranged variance value.  Variance is only
//	valid for emitters that are calling effects...
//	it basically determines the amount of slop in the
//	density calculations
//
// input:
//	string that contains one or two floats
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseVariance( const char *val )
{
	float min, max;

	if ( ParseFloat( val, &min, &max ) == true )
	{
		mVariance.SetRange( min, max );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseRGBStart
//	Reads in a ranged rgbStart field in vector format
//
// input:
//	string that contains one or two vectors
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseRGBStart( const char *val )
{
	vec3_t min, max;

	if ( ParseVector( val, min, max ) == true )
	{
		mRedStart.SetRange( min[0], max[0] );
		mGreenStart.SetRange( min[1], max[1] );
		mBlueStart.SetRange( min[2], max[2] );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseRGBEnd
//	Reads in a ranged rgbEnd field in vector format
//
// input:
//	string that contains one or two vectors
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseRGBEnd( const char *val )
{
	vec3_t min, max;

	if ( ParseVector( val, min, max ) == true )
	{
		mRedEnd.SetRange( min[0], max[0] );
		mGreenEnd.SetRange( min[1], max[1] );
		mBlueEnd.SetRange( min[2], max[2] );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseRGBParm
//	Reads in a ranged rgbParm field in float format
//
// input:
//	string that contains one or two floats
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseRGBParm( const char *val )
{
	float min, max;

	if ( ParseFloat( val, &min, &max ) == true )
	{
		mRGBParm.SetRange( min, max );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseRGBFlags
//	Reads in a set of rgbFlags in string format
//
// input:
//	string that contains the flag strings
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseRGBFlags( const char *val )
{
	int flags;

	if ( ParseGroupFlags( val, &flags ) == true )
	{
		// Convert our generic flag values into type specific ones
		mFlags |= ( flags << FX_RGB_SHIFT );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseAlphaStart
//	Reads in a ranged alphaStart field in float format
//
// input:
//	string that contains one or two floats
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseAlphaStart( const char *val )
{
	float min, max;

	if ( ParseFloat( val, &min, &max ) == true )
	{
		mAlphaStart.SetRange( min, max );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseAlphaEnd
//	Reads in a ranged alphaEnd field in float format
//
// input:
//	string that contains one or two floats
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseAlphaEnd( const char *val )
{
	float min, max;

	if ( ParseFloat( val, &min, &max ) == true )
	{
		mAlphaEnd.SetRange( min, max );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseAlphaParm
//	Reads in a ranged alphaParm field in float format
//
// input:
//	string that contains one or two floats
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseAlphaParm( const char *val )
{
	float min, max;

	if ( ParseFloat( val, &min, &max ) == true )
	{
		mAlphaParm.SetRange( min, max );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseAlphaFlags
//	Reads in a set of alphaFlags in string format
//
// input:
//	string that contains the flag strings
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseAlphaFlags( const char *val )
{
	int flags;

	if ( ParseGroupFlags( val, &flags ) == true )
	{
		// Convert our generic flag values into type specific ones
		mFlags |= ( flags << FX_ALPHA_SHIFT );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseSizeStart
//	Reads in a ranged sizeStart field in float format
//
// input:
//	string that contains one or two floats
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseSizeStart( const char *val )
{
	float min, max;

	if ( ParseFloat( val, &min, &max ) == true )
	{
		mSizeStart.SetRange( min, max );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseSizeEnd
//	Reads in a ranged sizeEnd field in float format
//
// input:
//	string that contains one or two floats
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseSizeEnd( const char *val )
{
	float min, max;

	if ( ParseFloat( val, &min, &max ) == true )
	{
		mSizeEnd.SetRange( min, max );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseSizeParm
//	Reads in a ranged sizeParm field in float format
//
// input:
//	string that contains one or two floats
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseSizeParm( const char *val )
{
	float min, max;

	if ( ParseFloat( val, &min, &max ) == true )
	{
		mSizeParm.SetRange( min, max );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseSizeFlags
//	Reads in a set of sizeFlags in string format
//
// input:
//	string that contains the flag strings
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseSizeFlags( const char *val )
{
	int flags;

	if ( ParseGroupFlags( val, &flags ) == true )
	{
		// Convert our generic flag values into type specific ones
		mFlags |= ( flags << FX_SIZE_SHIFT );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseSize2Start
//	Reads in a ranged Size2Start field in float format
//
// input:
//	string that contains one or two floats
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseSize2Start( const char *val )
{
	float min, max;

	if ( ParseFloat( val, &min, &max ) == true )
	{
		mSize2Start.SetRange( min, max );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseSize2End
//	Reads in a ranged Size2End field in float format
//
// input:
//	string that contains one or two floats
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseSize2End( const char *val )
{
	float min, max;

	if ( ParseFloat( val, &min, &max ) == true )
	{
		mSize2End.SetRange( min, max );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseSize2Parm
//	Reads in a ranged Size2Parm field in float format
//
// input:
//	string that contains one or two floats
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseSize2Parm( const char *val )
{
	float min, max;

	if ( ParseFloat( val, &min, &max ) == true )
	{
		mSize2Parm.SetRange( min, max );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseSize2Flags
//	Reads in a set of Size2Flags in string format
//
// input:
//	string that contains the flag strings
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseSize2Flags( const char *val )
{
	int flags;

	if ( ParseGroupFlags( val, &flags ) == true )
	{
		// Convert our generic flag values into type specific ones
		mFlags |= ( flags << FX_SIZE2_SHIFT );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseLengthStart
//	Reads in a ranged lengthStart field in float format
//
// input:
//	string that contains one or two floats
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseLengthStart( const char *val )
{
	float min, max;

	if ( ParseFloat( val, &min, &max ) == true )
	{
		mLengthStart.SetRange( min, max );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseLengthEnd
//	Reads in a ranged lengthEnd field in float format
//
// input:
//	string that contains one or two floats
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseLengthEnd( const char *val )
{
	float min, max;

	if ( ParseFloat( val, &min, &max ) == true )
	{
		mLengthEnd.SetRange( min, max );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseLengthParm
//	Reads in a ranged lengthParm field in float format
//
// input:
//	string that contains one or two floats
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseLengthParm( const char *val )
{
	float min, max;

	if ( ParseFloat( val, &min, &max ) == true )
	{
		mLengthParm.SetRange( min, max );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseLengthFlags
//	Reads in a set of lengthFlags in string format
//
// input:
//	string that contains the flag strings
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseLengthFlags( const char *val )
{
	int flags;

	if ( ParseGroupFlags( val, &flags ) == true )
	{
		// Convert our generic flag values into type specific ones
		mFlags |= ( flags << FX_LENGTH_SHIFT );
		return true;
	}

	return false;
}

//------------------------------------------------------
// ParseShaders
//	Reads in a group of shaders and registers them
//
// input:
//	Parse group that contains the list of shaders to parse
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseShaders( CGPValue *grp )
{
	const char	*val;
	int			handle;

	if ( grp->IsList() )
	{
		// If we are a list we have to do separate processing
		CGPObject *list = grp->GetList();

		while ( list )
		{
			// name is actually the value contained in the list
			val = list->GetName();

			handle = theFxHelper.RegisterShader( val );
			mMediaHandles.AddHandle( handle );

			list = (CGPValue *)list->GetNext();
		}
	}
	else
	{
		// Let's get a value
		val = grp->GetTopValue();

		if ( val )
		{
			handle = theFxHelper.RegisterShader( val );
			mMediaHandles.AddHandle( handle );
		}
		else
		{
			// empty "list"
			theFxHelper.Print( "CPrimitiveTemplate::ParseShaders called with an empty list!\n" );
			return false;
		}
	}

	return true;
}

//------------------------------------------------------
// ParseSounds
//	Reads in a group of sounds and registers them
//
// input:
//	Parse group that contains the list of sounds to parse
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseSounds( CGPValue *grp )
{
	const char	*val;
	int			handle;

	if ( grp->IsList() )
	{
		// If we are a list we have to do separate processing
		CGPObject *list = grp->GetList();

		while ( list )
		{
			// name is actually the value contained in the list
			val = list->GetName();

			handle = theFxHelper.RegisterSound( val );
			mMediaHandles.AddHandle( handle );

			list = (CGPValue *)list->GetNext();
		}
	}
	else
	{
		// Let's get a value
		val = grp->GetTopValue();

		if ( val )
		{
			handle = theFxHelper.RegisterSound( val );
			mMediaHandles.AddHandle( handle );
		}
		else
		{
			// empty "list"
			theFxHelper.Print( "CPrimitiveTemplate::ParseSounds called with an empty list!\n" );
			return false;
		}
	}

	return true;
}

//------------------------------------------------------
// ParseModels
//	Reads in a group of models and registers them
//
// input:
//	Parse group that contains the list of models to parse
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseModels( CGPValue *grp )
{
	const char	*val;
	int			handle;

	if ( grp->IsList() )
	{
		// If we are a list we have to do separate processing
		CGPObject *list = grp->GetList();

		while ( list )
		{
			// name is actually the value contained in the list
			val = list->GetName();

			handle = theFxHelper.RegisterModel( val );
			mMediaHandles.AddHandle( handle );

			list = (CGPValue *)list->GetNext();
		}
	}
	else
	{
		// Let's get a value
		val = grp->GetTopValue();

		if ( val )
		{
			handle = theFxHelper.RegisterModel( val );
			mMediaHandles.AddHandle( handle );
		}
		else
		{
			// empty "list"
			theFxHelper.Print( "CPrimitiveTemplate::ParseModels called with an empty list!\n" );
			return false;
		}
	}

	mFlags |= FX_ATTACHED_MODEL;

	return true;
}

//------------------------------------------------------
// ParseImpactFxStrings
//	Reads in a group of fx file names and registers them
//
// input:
//	Parse group that contains the list of fx to parse
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseImpactFxStrings( CGPValue *grp )
{
	const char	*val;
	int			handle;

	if ( grp->IsList() )
	{
		// If we are a list we have to do separate processing
		CGPObject *list = grp->GetList();

		while ( list )
		{
			// name is actually the value contained in the list
			val = list->GetName();
			handle = theFxScheduler.RegisterEffect( val );

			if ( handle )
			{
				mImpactFxHandles.AddHandle( handle );
			}
			else
			{
				theFxHelper.Print( "FxTemplate: Impact effect file not found.\n" );
				return false;
			}

			list = (CGPValue *)list->GetNext();
		}
	}
	else
	{
		// Let's get a value
		val = grp->GetTopValue();

		if ( val )
		{
			handle = theFxScheduler.RegisterEffect( val );

			if ( handle )
			{
				mImpactFxHandles.AddHandle( handle );
			}
			else
			{
				theFxHelper.Print( "FxTemplate: Impact effect file not found.\n" );
				return false;
			}
		}
		else
		{
			// empty "list"
			theFxHelper.Print( "CPrimitiveTemplate::ParseImpactFxStrings called with an empty list!\n" );
			return false;
		}
	}

	mFlags |= FX_IMPACT_RUNS_FX | FX_APPLY_PHYSICS;

	return true;
}

//------------------------------------------------------
// ParseDeathFxStrings
//	Reads in a group of fx file names and registers them
//
// input:
//	Parse group that contains the list of fx to parse
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseDeathFxStrings( CGPValue *grp )
{
	const char	*val;
	int			handle;

	if ( grp->IsList() )
	{
		// If we are a list we have to do separate processing
		CGPObject *list = grp->GetList();

		while ( list )
		{
			// name is actually the value contained in the list
			val = list->GetName();
			handle = theFxScheduler.RegisterEffect( val );

			if ( handle )
			{
				mDeathFxHandles.AddHandle( handle );
			}
			else
			{
				theFxHelper.Print( "FxTemplate: Death effect file not found.\n" );
				return false;
			}

			list = (CGPValue *)list->GetNext();
		}
	}
	else
	{
		// Let's get a value
		val = grp->GetTopValue();

		if ( val )
		{
			handle = theFxScheduler.RegisterEffect( val );

			if ( handle )
			{
				mDeathFxHandles.AddHandle( handle );
			}
			else
			{
				theFxHelper.Print( "FxTemplate: Death effect file not found.\n" );
				return false;
			}
		}
		else
		{
			// empty "list"
			theFxHelper.Print( "CPrimitiveTemplate::ParseDeathFxStrings called with an empty list!\n" );
			return false;
		}
	}

	mFlags |= FX_DEATH_RUNS_FX;

	return true;
}

//------------------------------------------------------
// ParseEmitterFxStrings
//	Reads in a group of fx file names and registers them
//
// input:
//	Parse group that contains the list of fx to parse
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseEmitterFxStrings( CGPValue *grp )
{
	const char	*val;
	int			handle;

	if ( grp->IsList() )
	{
		// If we are a list we have to do separate processing
		CGPObject *list = grp->GetList();

		while ( list )
		{
			// name is actually the value contained in the list
			val = list->GetName();
			handle = theFxScheduler.RegisterEffect( val );

			if ( handle )
			{
				mEmitterFxHandles.AddHandle( handle );
			}
			else
			{
				theFxHelper.Print( "FxTemplate: Emitter effect file not found.\n" );
				return false;
			}

			list = (CGPValue *)list->GetNext();
		}
	}
	else
	{
		// Let's get a value
		val = grp->GetTopValue();

		if ( val )
		{
			handle = theFxScheduler.RegisterEffect( val );

			if ( handle )
			{
				mEmitterFxHandles.AddHandle( handle );
			}
			else
			{
				theFxHelper.Print( "FxTemplate: Emitter effect file not found.\n" );
				return false;
			}
		}
		else
		{
			// empty "list"
			theFxHelper.Print( "CPrimitiveTemplate::ParseEmitterFxStrings called with an empty list!\n" );
			return false;
		}
	}

	mFlags |= FX_EMIT_FX;

	return true;
}

//------------------------------------------------------
// ParsePlayFxStrings
//	Reads in a group of fx file names and registers them
//
// input:
//	Parse group that contains the list of fx to parse
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParsePlayFxStrings( CGPValue *grp )
{
	const char	*val;
	int			handle;

	if ( grp->IsList() )
	{
		// If we are a list we have to do separate processing
		CGPObject *list = grp->GetList();

		while ( list )
		{
			// name is actually the value contained in the list
			val = list->GetName();
			handle = theFxScheduler.RegisterEffect( val );

			if ( handle )
			{
				mPlayFxHandles.AddHandle( handle );
			}
			else
			{
				theFxHelper.Print( "FxTemplate: Effect file not found.\n" );
				return false;
			}

			list = (CGPValue *)list->GetNext();
		}
	}
	else
	{
		// Let's get a value
		val = grp->GetTopValue();

		if ( val )
		{
			handle = theFxScheduler.RegisterEffect( val );

			if ( handle )
			{
				mPlayFxHandles.AddHandle( handle );
			}
			else
			{
				theFxHelper.Print( "FxTemplate: Effect file not found.\n" );
				return false;
			}
		}
		else
		{
			// empty "list"
			theFxHelper.Print( "CPrimitiveTemplate::ParsePlayFxStrings called with an empty list!\n" );
			return false;
		}
	}

	return true;
}

//------------------------------------------------------
// ParseRGB
//	Takes an RGB group and chomps out any pairs contained
//	in it.
//
// input:
//	the parse group to process
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseRGB( CGPGroup *grp )
{
	CGPValue	*pairs;
	const char	*key;
	const char	*val;

	// Inside of the group, we should have a series of pairs
	pairs = grp->GetPairs();

	while( pairs )
	{
		// Let's get the key field
		key = pairs->GetName();
		val = pairs->GetTopValue();

		// Huge Q_stricmp lists suxor
			 if ( !Q_stricmp( key, "start" ) )
			ParseRGBStart( val );
		else if ( !Q_stricmp( key, "end" ) )
			ParseRGBEnd( val );
		else if ( !Q_stricmp( key, "parm" ) || !Q_stricmp( key, "parms" ) )
			ParseRGBParm( val );
		else if ( !Q_stricmp( key, "flags" ) || !Q_stricmp( key, "flag" ) )
			ParseRGBFlags( val );
		else
			theFxHelper.Print( "Unknown key parsing an RGB group: %s\n", key );

		pairs = (CGPValue *)pairs->GetNext();
	}

	return true;
}

//------------------------------------------------------
// ParseAlpha
//	Takes an alpha group and chomps out any pairs contained
//	in it.
//
// input:
//	the parse group to process
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseAlpha( CGPGroup *grp )
{
	CGPValue	*pairs;
	const char	*key;
	const char	*val;

	// Inside of the group, we should have a series of pairs
	pairs = grp->GetPairs();

	while( pairs )
	{
		// Let's get the key field
		key = pairs->GetName();
		val = pairs->GetTopValue();

		// Huge Q_stricmp lists suxor
			 if ( !Q_stricmp( key, "start" ) )
			ParseAlphaStart( val );
		else if ( !Q_stricmp( key, "end" ) )
			ParseAlphaEnd( val );
		else if ( !Q_stricmp( key, "parm" ) || !Q_stricmp( key, "parms" ) )
			ParseAlphaParm( val );
		else if ( !Q_stricmp( key, "flags" ) || !Q_stricmp( key, "flag" ) )
			ParseAlphaFlags( val );
		else
			theFxHelper.Print( "Unknown key parsing an Alpha group: %s\n", key );

		pairs = (CGPValue *)pairs->GetNext();
	}

	return true;
}

//------------------------------------------------------
// ParseSize
//	Takes a size group and chomps out any pairs contained
//	in it.
//
// input:
//	the parse group to process
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseSize( CGPGroup *grp )
{
	CGPValue	*pairs;
	const char	*key;
	const char	*val;

	// Inside of the group, we should have a series of pairs
	pairs = grp->GetPairs();

	while( pairs )
	{
		// Let's get the key field
		key = pairs->GetName();
		val = pairs->GetTopValue();

		// Huge Q_stricmp lists suxor
			 if ( !Q_stricmp( key, "start" ) )
			ParseSizeStart( val );
		else if ( !Q_stricmp( key, "end" ) )
			ParseSizeEnd( val );
		else if ( !Q_stricmp( key, "parm" ) || !Q_stricmp( key, "parms" ) )
			ParseSizeParm( val );
		else if ( !Q_stricmp( key, "flags" ) || !Q_stricmp( key, "flag" ) )
			ParseSizeFlags( val );
		else
			theFxHelper.Print( "Unknown key parsing a Size group: %s\n", key );

		pairs = (CGPValue *)pairs->GetNext();
	}

	return true;
}

//------------------------------------------------------
// ParseSize2
//	Takes a Size2 group and chomps out any pairs contained
//	in it.
//
// input:
//	the parse group to process
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseSize2( CGPGroup *grp )
{
	CGPValue	*pairs;
	const char	*key;
	const char	*val;

	// Inside of the group, we should have a series of pairs
	pairs = grp->GetPairs();

	while( pairs )
	{
		// Let's get the key field
		key = pairs->GetName();
		val = pairs->GetTopValue();

		// Huge Q_stricmp lists suxor
			 if ( !Q_stricmp( key, "start" ) )
			ParseSize2Start( val );
		else if ( !Q_stricmp( key, "end" ) )
			ParseSize2End( val );
		else if ( !Q_stricmp( key, "parm" ) || !Q_stricmp( key, "parms" ) )
			ParseSize2Parm( val );
		else if ( !Q_stricmp( key, "flags" ) || !Q_stricmp( key, "flag" ) )
			ParseSize2Flags( val );
		else
			theFxHelper.Print( "Unknown key parsing a Size2 group: %s\n", key );

		pairs = (CGPValue *)pairs->GetNext();
	}

	return true;
}

//------------------------------------------------------
// ParseLength
//	Takes a length group and chomps out any pairs contained
//	in it.
//
// input:
//	the parse group to process
//
// return:
//	success of parse operation.
//------------------------------------------------------
bool CPrimitiveTemplate::ParseLength( CGPGroup *grp )
{
	CGPValue	*pairs;
	const char	*key;
	const char	*val;

	// Inside of the group, we should have a series of pairs
	pairs = grp->GetPairs();

	while( pairs )
	{
		// Let's get the key field
		key = pairs->GetName();
		val = pairs->GetTopValue();

		// Huge Q_stricmp lists suxor
			 if ( !Q_stricmp( key, "start" ))
			ParseLengthStart( val );
		else if ( !Q_stricmp( key, "end" ))
			ParseLengthEnd( val );
		else if ( !Q_stricmp( key, "parm" ) || !Q_stricmp( key, "parms" ))
			ParseLengthParm( val );
		else if ( !Q_stricmp( key, "flags" ) || !Q_stricmp( key, "flag" ))
			ParseLengthFlags( val );
		else
			theFxHelper.Print( "Unknown key parsing a Length group: %s\n", key );

		pairs = (CGPValue *)pairs->GetNext();
	}

	return true;
}

// Parse a primitive, apply defaults first, grab any base level
//	key pairs, then process any sub groups we may contain.
//------------------------------------------------------
bool CPrimitiveTemplate::ParsePrimitive( CGPGroup *grp )
{
	CGPGroup	*subGrp;
	CGPValue	*pairs;
	const char	*key;
	const char	*val;

	// Lets work with the pairs first
	pairs = grp->GetPairs();

	while( pairs )
	{
		// the fields
		key = pairs->GetName();
		val = pairs->GetTopValue();

		// Huge Q_stricmp lists suxor
			 if ( !Q_stricmp( key, "count" ) )
			ParseCount( val );
		else if ( !Q_stricmp( key, "shaders" ) || !Q_stricmp( key, "shader" ) )
			ParseShaders( pairs );
		else if ( !Q_stricmp( key, "models" ) || !Q_stricmp( key, "model" ) )
			ParseModels( pairs );
		else if ( !Q_stricmp( key, "sounds" ) || !Q_stricmp( key, "sound" ) )
			ParseSounds( pairs );
		else if ( !Q_stricmp( key, "impactfx" ) )
			ParseImpactFxStrings( pairs );
		else if ( !Q_stricmp( key, "deathfx" ) )
			ParseDeathFxStrings( pairs );
		else if ( !Q_stricmp( key, "emitfx" ) )
			ParseEmitterFxStrings( pairs );
		else if ( !Q_stricmp( key, "playfx" ) )
			ParsePlayFxStrings( pairs );
		else if ( !Q_stricmp( key, "life" ) )
			ParseLife( val );
		else if ( !Q_stricmp( key, "delay" ) )
			ParseDelay( val );
		else if ( !Q_stricmp( key, "cullrange" ) ) {
//			mCullRange = atoi( val );
//			mCullRange *= mCullRange; // square it now so we don't have to square every time we compare
		}
		else if ( !Q_stricmp( key, "bounce" ) || !Q_stricmp( key, "intensity" ) ) // me==bad for reusing this...but it shouldn't hurt anything)
			ParseElasticity( val );
		else if ( !Q_stricmp( key, "min" ) )
			ParseMin( val );
		else if ( !Q_stricmp( key, "max" ) )
			ParseMax( val );
		else if ( !Q_stricmp( key, "angle" ) || !Q_stricmp( key, "angles" ) )
			ParseAngle( val );
		else if ( !Q_stricmp( key, "angleDelta" ) )
			ParseAngleDelta( val );
		else if ( !Q_stricmp( key, "velocity" ) || !Q_stricmp( key, "vel" ) )
			ParseVelocity( val );
		else if ( !Q_stricmp( key, "acceleration" ) || !Q_stricmp( key, "accel" ) )
			ParseAcceleration( val );
		else if ( !Q_stricmp( key, "gravity" ) )
			ParseGravity( val );
		else if ( !Q_stricmp( key, "density" ) )
			ParseDensity( val );
		else if ( !Q_stricmp( key, "variance" ) )
			ParseVariance( val );
		else if ( !Q_stricmp( key, "origin" ) )
			ParseOrigin1( val );
		else if ( !Q_stricmp( key, "origin2" ) )
			ParseOrigin2( val );
		else if ( !Q_stricmp( key, "radius" ) ) // part of ellipse/cylinder calcs.
			ParseRadius( val );
		else if ( !Q_stricmp( key, "height" ) ) // part of ellipse/cylinder calcs.
			ParseHeight( val );
		else if ( !Q_stricmp( key, "wind" ) )
			ParseWindModifier( val );
		else if ( !Q_stricmp( key, "rotation" ) )
			ParseRotation( val );
		else if ( !Q_stricmp( key, "rotationDelta" ) )
			ParseRotationDelta( val );
		// these need to get passed on to the primitive
		else if ( !Q_stricmp( key, "flags" ) || !Q_stricmp( key, "flag" ) )
			ParseFlags( val );
		// these are used to spawn things in cool ways, but don't ever get passed on to prims.
		else if ( !Q_stricmp( key, "spawnFlags" ) || !Q_stricmp( key, "spawnFlag" ) )
			ParseSpawnFlags( val );
		else if ( !Q_stricmp( key, "name" ) ) {
			if ( val ) // just stash the descriptive name of the primitive
				strcpy( mName, val );
		}
		else if ( !Q_stricmp( key, "materialImpact" ) )
			ParseMaterialImpact( val );
		else
			theFxHelper.Print( "Unknown key parsing an effect primitive: %s\n", key );

		pairs = (CGPValue *)pairs->GetNext();
	}

	subGrp = grp->GetSubGroups();

	// Lets chomp on the groups now
	while ( subGrp )
	{
		key = subGrp->GetName();

			 if ( !Q_stricmp( key, "rgb" ) )
			ParseRGB( subGrp );
		else if ( !Q_stricmp( key, "alpha" ) )
			ParseAlpha( subGrp );
		else if ( !Q_stricmp( key, "size" ) || !Q_stricmp( key, "width" ) )
			ParseSize( subGrp );
		else if ( !Q_stricmp( key, "size2" ) || !Q_stricmp( key, "width2" ) )
			ParseSize2( subGrp );
		else if ( !Q_stricmp( key, "length" ) || !Q_stricmp( key, "height" ) )
			ParseLength( subGrp );
		else
			theFxHelper.Print( "Unknown group key parsing a particle: %s\n", key );

		subGrp = (CGPGroup *)subGrp->GetNext();
	}

	return true;
}
