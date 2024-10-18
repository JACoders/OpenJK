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

#include "cg_media.h"

extern int drawnFx;
extern int mParticles;
extern int mOParticles;
extern int mLines;
extern int mTails;

extern vmCvar_t	fx_expensivePhysics;

// Helper function
//-------------------------
void ClampVec( vec3_t dat, byte *res )
{
	int r;

	// clamp all vec values, then multiply the normalized values by 255 to maximize the result
	for ( int i = 0; i < 3; i++ )
	{
		r = Q_ftol(dat[i] * 255.0f);

		if ( r < 0 )
		{
			r = 0;
		}
		else if ( r > 255 )
		{
			r = 255;
		}

		res[i] = (unsigned char)r;
	}
}

void GetOrigin( int clientID, vec3_t org )
{
	if ( clientID >=0 )
	{
		centity_t *cent = &cg_entities[clientID];

		if ( cent && cent->gent && cent->gent->client )
		{
			VectorCopy( cent->gent->client->renderInfo.muzzlePoint, org );
		}
	}
}

void GetDir( int clientID, vec3_t org )
{
	if ( clientID >=0 )
	{
		centity_t *cent = &cg_entities[clientID];

		if ( cent && cent->gent && cent->gent->client )
		{
			VectorCopy( cent->gent->client->renderInfo.muzzleDir, org );
		}
	}
}

//--------------------------
//
// Base Effect Class
//
//--------------------------


//--------------------------
//
// Derived Particle Class
//
//--------------------------

void CParticle::Die()
{
	if ( mFlags & FX_DEATH_RUNS_FX && !(mFlags & FX_KILL_ON_IMPACT) )
	{
		vec3_t	norm;

		// Man, this just seems so, like, uncool and stuff...
		VectorSet( norm, Q_flrand(-1.0f, 1.0f), Q_flrand(-1.0f, 1.0f), Q_flrand(-1.0f, 1.0f));
		VectorNormalize( norm );

		theFxScheduler.PlayEffect( mDeathFxID, mOrigin1, norm );
	}
}

//----------------------------
bool CParticle::Cull()
{
	vec3_t	dir;

	// Get the direction to the view
	VectorSubtract( mOrigin1, cg.refdef.vieworg, dir );

	// Check if it's behind the viewer
	if ( (DotProduct( cg.refdef.viewaxis[0], dir )) < 0 )
	{
		return true;
	}

	float len = VectorLengthSquared( dir );

	// Can't be too close
	if ( len < 16 * 16 )
	{
		return true;
	}

	return false;
}

//----------------------------
void CParticle::Draw()
{
	if ( mFlags & FX_DEPTH_HACK )
	{
		// Not sure if first person needs to be set, but it can't hurt?
		mRefEnt.renderfx |= RF_DEPTHHACK;
	}

	// Add our refEntity to the scene
	VectorCopy( mOrigin1, mRefEnt.origin );
	theFxHelper.AddFxToScene( &mRefEnt );

	drawnFx++;
	mParticles++;
}

//----------------------------
// Update
//----------------------------
bool CParticle::Update()
{
	// Game pausing can cause dumb time things to happen, so kill the effect in this instance
	if ( mTimeStart > theFxHelper.mTime )
	{
		return false;
	}

	if ( mFlags & FX_RELATIVE )
	{
		if ( mClientID < 0 || mClientID >= ENTITYNUM_WORLD )
		{	// we are somehow not bolted even though the flag is on?
			return false;
		}

		vec3_t	org;
		vec3_t	ax[3];

		// Get our current position and direction
		if (mModelNum>=0 && mBoltNum>=0)	//bolt style
		{
			const centity_t &cent = cg_entities[mClientID];
			if (!cent.gent->ghoul2.IsValid())
			{
				return false;
			}

			if (!theFxHelper.GetOriginAxisFromBolt(cent, mModelNum, mBoltNum, org, ax))
			{	//could not get bolt
				return false;
			}
		}
		else
		{//fixme change this to bolt style...
			vec3_t dir, ang;

			GetOrigin( mClientID, org );
			GetDir( mClientID, dir );

			vectoangles( dir, ang );
			AngleVectors( ang, ax[0], ax[1], ax[2] );
		}
		vec3_t 	realVel, realAccel;

		VectorMA( org, mOrgOffset[0], ax[0], org );
		VectorMA( org, mOrgOffset[1], ax[1], org );
		VectorMA( org, mOrgOffset[2], ax[2], org );

		const float	time = (theFxHelper.mTime - mTimeStart) * 0.001f;
		// calc the real velocity and accel vectors
		VectorScale( ax[0], mVel[0], realVel );
		VectorMA( realVel, mVel[1], ax[1], realVel );
		VectorMA( realVel, mVel[2], ax[2], realVel );
		realVel[2] += 0.5f * mGravity * time;

		VectorScale( ax[0], mAccel[0], realAccel );
		VectorMA( realAccel, mAccel[1], ax[1], realAccel );
		VectorMA( realAccel, mAccel[2], ax[2], realAccel );

		// Get our real velocity at the current time, taking into account the effects of acceleartion.  NOTE: not sure if this is even 100% correct math-wise
		VectorMA( realVel, time, realAccel, realVel );

		// Now move us to where we should be at the given time
		VectorMA( org, time, realVel, mOrigin1 );

	}
	else if (( mTimeStart < theFxHelper.mTime ) && UpdateOrigin() == false )
	{
		// we are marked for death
		return false;
	}

	if ( !Cull())
	{
		UpdateSize();
		UpdateRGB();
		UpdateAlpha();
		UpdateRotation();

		Draw();
	}

	return true;
}

//----------------------------
// Update Origin
//----------------------------
bool CParticle::UpdateOrigin()
{
	vec3_t	new_origin;
//	float	ftime, time2;

	UpdateVelocity();

	// Calc the time differences
//	ftime = theFxHelper.mFrameTime * 0.001f;
	//time2 = ftime * ftime * 0.5f;
//	time2=0;

	// Predict the new position
	new_origin[0] = mOrigin1[0] + theFxHelper.mFloatFrameTime * mVel[0];// + time2 * mVel[0];
	new_origin[1] = mOrigin1[1] + theFxHelper.mFloatFrameTime * mVel[1];// + time2 * mVel[1];
	new_origin[2] = mOrigin1[2] + theFxHelper.mFloatFrameTime * mVel[2];// + time2 * mVel[2];

	// Only perform physics if this object is tagged to do so
	if ( (mFlags & FX_APPLY_PHYSICS) )
	{
		bool solid;

		if ( (mFlags&FX_EXPENSIVE_PHYSICS)
			&& fx_expensivePhysics.integer )
		{
			solid = true; // by setting this to true, we force a real trace to happen
		}
		else
		{
			// if this returns solid, we need to do a trace
			solid = !!(CG_PointContents( new_origin, ENTITYNUM_WORLD ) & ( MASK_SHOT | CONTENTS_WATER ));
		}

		if ( solid )
		{
			trace_t	trace;
			float	dot;

			if ( mFlags & FX_USE_BBOX )
			{
				if (mFlags & FX_GHOUL2_TRACE)
				{
					theFxHelper.G2Trace( &trace, mOrigin1, mMin, mMax, new_origin, ENTITYNUM_NONE, ( MASK_SHOT | CONTENTS_WATER ) );
				}
				else
				{
					theFxHelper.Trace( &trace, mOrigin1, mMin, mMax, new_origin, -1, ( MASK_SHOT | CONTENTS_WATER ) );
				}
			}
			else
			{
				if (mFlags & FX_GHOUL2_TRACE)
				{
					theFxHelper.G2Trace( &trace, mOrigin1, NULL, NULL, new_origin, ENTITYNUM_NONE, ( MASK_SHOT | CONTENTS_WATER ) );
				}
				else
				{
					theFxHelper.Trace( &trace, mOrigin1, NULL, NULL, new_origin, -1, ( MASK_SHOT | CONTENTS_WATER ) );
				}
			}

			if ( trace.startsolid || trace.allsolid || trace.fraction == 1.0)
			{
			}
			else
			{
				// Hit something
				if ( mFlags & FX_IMPACT_RUNS_FX && !(trace.surfaceFlags & SURF_NOIMPACT ))
				{
					theFxScheduler.PlayEffect( mImpactFxID, trace.endpos, trace.plane.normal );
				}

				if ( mFlags & FX_KILL_ON_IMPACT	)
				{
					// time to die
					return false;
				}

				VectorMA( mVel, theFxHelper.mFloatFrameTime * trace.fraction, mAccel, mVel );

				dot = DotProduct( mVel, trace.plane.normal );

				VectorMA( mVel, -2 * dot, trace.plane.normal, mVel );

				VectorScale( mVel, mElasticity, mVel );

				// If the velocity is too low, make it stop moving, rotating, and turn off physics to avoid
				//	doing expensive operations when they aren't needed
				if ( trace.plane.normal[2] > 0 && mVel[2] < 4 )
				{
					VectorClear( mVel );
					VectorClear( mAccel );

					mFlags &= ~(FX_APPLY_PHYSICS|FX_IMPACT_RUNS_FX);
				}

				// Set the origin to the exact impact point
				VectorCopy( trace.endpos, mOrigin1 );
				return true;
			}
		}
	}

	// No physics were done to this object, move it
	VectorCopy( new_origin, mOrigin1 );

	return true;
}

//----------------------------
// Update Size
//----------------------------
void CParticle::UpdateSize()
{
	// completely biased towards start if it doesn't get overridden
	float	perc1 = 1.0f, perc2 = 1.0f;

	if ( (mFlags & FX_SIZE_LINEAR) )
	{
		// calculate element biasing
		perc1 = 1.0f - (float)(theFxHelper.mTime - mTimeStart)
						/ (float)(mTimeEnd - mTimeStart);
	}

	// We can combine FX_LINEAR with _either_ FX_NONLINEAR, FX_WAVE, or FX_CLAMP
	if (( mFlags & FX_SIZE_PARM_MASK ) == FX_SIZE_NONLINEAR )
	{
		if ( theFxHelper.mTime > mSizeParm )
		{
			// get percent done, using parm as the start of the non-linear fade
			perc2 = 1.0f - (float)(theFxHelper.mTime - mSizeParm)
							/ (float)(mTimeEnd - mSizeParm);
		}

		if ( mFlags & FX_SIZE_LINEAR )
		{
			// do an even blend
			perc1 = perc1 * 0.5f + perc2 * 0.5f;
		}
		else
		{
			// just copy it over...sigh
			perc1 = perc2;
		}
	}
	else if (( mFlags & FX_SIZE_PARM_MASK ) == FX_SIZE_WAVE )
	{
		// wave gen, with parm being the frequency multiplier
		perc1 = perc1 * (float)cos( (theFxHelper.mTime - mTimeStart) * mSizeParm );
	}
	else if (( mFlags & FX_SIZE_PARM_MASK ) == FX_SIZE_CLAMP )
	{
		if ( theFxHelper.mTime < mSizeParm )
		{
			// get percent done, using parm as the start of the non-linear fade
			perc2 = (float)(mSizeParm - theFxHelper.mTime)
							/ (float)(mSizeParm - mTimeStart);
		}
		else
		{
			perc2 = 0.0f; // make it full size??
		}

		if ( (mFlags & FX_SIZE_LINEAR) )
		{
			// do an even blend
			perc1 = perc1 * 0.5f + perc2 * 0.5f;
		}
		else
		{
			// just copy it over...sigh
			perc1 = perc2;
		}
	}

	// If needed, RAND can coexist with linear and either non-linear or wave.
	if (( mFlags & FX_SIZE_RAND ))
	{
		// Random simply modulates the existing value
		perc1 = Q_flrand(0.0f, 1.0f) * perc1;
	}

	mRefEnt.radius = (mSizeStart * perc1) + (mSizeEnd * (1.0f - perc1));
}

//----------------------------
// Update RGB
//----------------------------
void CParticle::UpdateRGB()
{
	// completely biased towards start if it doesn't get overridden
	float	perc1 = 1.0f, perc2 = 1.0f;
	vec3_t	res;

	if ( (mFlags & FX_RGB_LINEAR) )
	{
		// calculate element biasing
		perc1 = 1.0f - (float)( theFxHelper.mTime - mTimeStart )
						/ (float)( mTimeEnd - mTimeStart );
	}

	// We can combine FX_LINEAR with _either_ FX_NONLINEAR, FX_WAVE, or FX_CLAMP
	if (( mFlags & FX_RGB_PARM_MASK ) == FX_RGB_NONLINEAR )
	{
		if ( theFxHelper.mTime > mRGBParm )
		{
			// get percent done, using parm as the start of the non-linear fade
			perc2 = 1.0f - (float)( theFxHelper.mTime - mRGBParm )
							/ (float)( mTimeEnd - mRGBParm );
		}

		if ( (mFlags & FX_RGB_LINEAR) )
		{
			// do an even blend
			perc1 = perc1 * 0.5f + perc2 * 0.5f;
		}
		else
		{
			// just copy it over...sigh
			perc1 = perc2;
		}
	}
	else if (( mFlags & FX_RGB_PARM_MASK ) == FX_RGB_WAVE )
	{
		// wave gen, with parm being the frequency multiplier
		perc1 = perc1 * (float)cos(( theFxHelper.mTime - mTimeStart ) * mRGBParm );
	}
	else if (( mFlags & FX_RGB_PARM_MASK ) == FX_RGB_CLAMP )
	{
		if ( theFxHelper.mTime < mRGBParm )
		{
			// get percent done, using parm as the start of the non-linear fade
			perc2 = (float)(mRGBParm - theFxHelper.mTime)
							/ (float)(mRGBParm - mTimeStart);
		}
		else
		{
			perc2 = 0.0f; // make it full size??
		}

		if (( mFlags & FX_RGB_LINEAR ))
		{
			// do an even blend
			perc1 = perc1 * 0.5f + perc2 * 0.5f;
		}
		else
		{
			// just copy it over...sigh
			perc1 = perc2;
		}
	}

	// If needed, RAND can coexist with linear and either non-linear or wave.
	if (( mFlags & FX_RGB_RAND ))
	{
		// Random simply modulates the existing value
		perc1 = Q_flrand(0.0f, 1.0f) * perc1;
	}

	// Now get the correct color
	VectorScale( mRGBStart, perc1, res );
	VectorMA( res, (1.0f - perc1), mRGBEnd, mRefEnt.angles ); // angles is a temp storage, will get clamped to a byte in the UpdateAlpha section
}


//----------------------------
// Update Alpha
//----------------------------
void CParticle::UpdateAlpha()
{
	// completely biased towards start if it doesn't get overridden
	float	perc1 = 1.0f, perc2 = 1.0f;

	if ( mFlags & FX_ALPHA_LINEAR )
	{
		// calculate element biasing
		perc1 = 1.0f - (float)(theFxHelper.mTime - mTimeStart)
						/ (float)(mTimeEnd - mTimeStart);
	}

	// We can combine FX_LINEAR with _either_ FX_NONLINEAR, FX_WAVE, or FX_CLAMP
	if (( mFlags & FX_ALPHA_PARM_MASK ) == FX_ALPHA_NONLINEAR )
	{
		if ( theFxHelper.mTime > mAlphaParm )
		{
			// get percent done, using parm as the start of the non-linear fade
			perc2 = 1.0f - (float)(theFxHelper.mTime - mAlphaParm)
							/ (float)(mTimeEnd - mAlphaParm);
		}

		if ( mFlags & FX_ALPHA_LINEAR )
		{
			// do an even blend
			perc1 = perc1 * 0.5f + perc2 * 0.5f;
		}
		else
		{
			// just copy it over...sigh
			perc1 = perc2;
		}
	}
	else if (( mFlags & FX_ALPHA_PARM_MASK ) == FX_ALPHA_WAVE )
	{
		// wave gen, with parm being the frequency multiplier
		perc1 = perc1 * (float)cos( (theFxHelper.mTime - mTimeStart) * mAlphaParm );
	}
	else if (( mFlags & FX_ALPHA_PARM_MASK ) == FX_ALPHA_CLAMP )
	{
		if ( theFxHelper.mTime < mAlphaParm )
		{
			// get percent done, using parm as the start of the non-linear fade
			perc2 = (float)(mAlphaParm - theFxHelper.mTime)
							/ (float)(mAlphaParm - mTimeStart);
		}
		else
		{
			perc2 = 0.0f; // make it full size??
		}

		if ( mFlags & FX_ALPHA_LINEAR )
		{
			// do an even blend
			perc1 = perc1 * 0.5f + perc2 * 0.5f;
		}
		else
		{
			// just copy it over...sigh
			perc1 = perc2;
		}
	}

	perc1 = (mAlphaStart * perc1) + (mAlphaEnd * (1.0f - perc1));

	// We should be in the right range, but clamp to ensure
	if ( perc1 < 0.0f )
	{
		perc1 = 0.0f;
	}
	else if ( perc1 > 1.0f )
	{
		perc1 = 1.0f;
	}

	// If needed, RAND can coexist with linear and either non-linear or wave.
	if ( (mFlags & FX_ALPHA_RAND) )
	{
		// Random simply modulates the existing value
		perc1 = Q_flrand(0.0f, 1.0f) * perc1;
	}

	if ( mFlags & FX_USE_ALPHA )
	{
		// should use this when using art that has an alpha channel
		 ClampVec( mRefEnt.angles, (byte*)(&mRefEnt.shaderRGBA) );
		 mRefEnt.shaderRGBA[3] = (byte)(perc1 * 0xff);
	}
	else
	{
		// Modulate the rgb fields by the alpha value to do the fade, works fine for additive blending
		VectorScale( mRefEnt.angles, perc1, mRefEnt.angles );
		ClampVec( mRefEnt.angles, (byte*)(&mRefEnt.shaderRGBA) );
	}
}

//--------------------------------
//
// Derived Oriented Particle Class
//
//--------------------------------


//----------------------------
bool COrientedParticle::Cull()
{
	vec3_t	dir;

	// Get the direction to the view
	VectorSubtract( mOrigin1, cg.refdef.vieworg, dir );

	// Check if it's behind the viewer
	if ( (DotProduct( cg.refdef.viewaxis[0], dir )) < 0 )
	{
		return true;
	}

	float len = VectorLengthSquared( dir );

	// Can't be too close
	if ( len < 24 * 24 )
	{
		return true;
	}

	return false;
}

//----------------------------
void COrientedParticle::Draw()
{
	if ( mFlags & FX_DEPTH_HACK )
	{
		// Not sure if first person needs to be set
		mRefEnt.renderfx |= RF_DEPTHHACK;
	}

	// Add our refEntity to the scene
	VectorCopy( mOrigin1, mRefEnt.origin );
	VectorCopy( mNormal, mRefEnt.axis[0] );
	theFxHelper.AddFxToScene( &mRefEnt );

	drawnFx++;
	mOParticles++;
}

//----------------------------
// Update
//----------------------------
bool COrientedParticle::Update()
{
	// Game pausing can cause dumb time things to happen, so kill the effect in this instance
	if ( mTimeStart > theFxHelper.mTime )
	{
		return false;
	}

	if ( mFlags & FX_RELATIVE )
	{
		if ( mClientID < 0 || mClientID >= ENTITYNUM_WORLD )
		{	// we are somehow not bolted even though the flag is on?
			return false;
		}

		vec3_t	org;
		vec3_t	ax[3];

		// Get our current position and direction
		if (mModelNum>=0 && mBoltNum>=0)	//bolt style
		{
			const centity_t &cent = cg_entities[mClientID];
			if (!cent.gent->ghoul2.IsValid())
			{
				return false;
			}

			if (!theFxHelper.GetOriginAxisFromBolt(cent, mModelNum, mBoltNum, org, ax))
			{	//could not get bolt
				return false;
			}
		}
		else
		{//fixme change this to bolt style...
			vec3_t dir, ang;

			GetOrigin( mClientID, org );
			GetDir( mClientID, dir );

			vectoangles( dir, ang );
			AngleVectors( ang, ax[0], ax[1], ax[2] );
		}
		vec3_t 	realVel, realAccel;

		VectorMA( org, mOrgOffset[0], ax[0], org );
		VectorMA( org, mOrgOffset[1], ax[1], org );
		VectorMA( org, mOrgOffset[2], ax[2], org );

		const float	time = (theFxHelper.mTime - mTimeStart) * 0.001f;
		// calc the real velocity and accel vectors
		VectorScale( ax[0], mVel[0], realVel );
		VectorMA( realVel, mVel[1], ax[1], realVel );
		VectorMA( realVel, mVel[2], ax[2], realVel );
		realVel[2] += 0.5f * mGravity * time;

		VectorScale( ax[0], mAccel[0], realAccel );
		VectorMA( realAccel, mAccel[1], ax[1], realAccel );
		VectorMA( realAccel, mAccel[2], ax[2], realAccel );

		// Get our real velocity at the current time, taking into account the effects of acceleartion.  NOTE: not sure if this is even 100% correct math-wise
		VectorMA( realVel, time, realAccel, realVel );

		// Now move us to where we should be at the given time
		VectorMA( org, time, realVel, mOrigin1 );

		//use the normalOffset and add that to the actual normal of the bolt
		//NOTE: not tested!!!
		vec3_t	boltAngles, offsetAngles, transformedAngles;
		vectoangles( ax[0], boltAngles );
		vectoangles( mNormalOffset, offsetAngles );
		VectorAdd( boltAngles, offsetAngles, transformedAngles );
		AngleVectors( transformedAngles, mNormal, NULL, NULL );
	}
	else if (( mTimeStart < theFxHelper.mTime ) && UpdateOrigin() == false )
	{
		// we are marked for death
		return false;
	}

	if ( !Cull())
	{
		UpdateSize();
		UpdateRGB();
		UpdateAlpha();
		UpdateRotation();

		Draw();
	}

	return true;
}


//----------------------------
//
// Derived Line Class
//
//----------------------------

//----------------------------
bool CLine::Cull( void )
{
	vec3_t	dir;

	VectorSubtract( mOrigin1, cg.refdef.vieworg, dir );

	//Check if it's in front of the viewer
	if ( (DotProduct( cg.refdef.viewaxis[0], dir )) >= 0 )
	{
		return false;	//don't cull
	}

	VectorSubtract( mOrigin2, cg.refdef.vieworg, dir );

	//Check if it's in front of the viewer
	if ( (DotProduct( cg.refdef.viewaxis[0], dir )) >= 0 )
	{
		return false;
	}

	return true; //all points behind viewer
}

//----------------------------
void CLine::Draw()
{
	if ( mFlags & FX_DEPTH_HACK )
	{
		// Not sure if first person needs to be set, but it can't hurt?
		mRefEnt.renderfx |= RF_DEPTHHACK;
	}

	VectorCopy( mOrigin1, mRefEnt.origin );
	VectorCopy( mOrigin2, mRefEnt.oldorigin );

	theFxHelper.AddFxToScene( &mRefEnt );

	drawnFx++;
	mLines++;
}

//----------------------------
bool CLine::Update()
{
	// Game pausing can cause dumb time things to happen, so kill the effect in this instance
	if ( mTimeStart > theFxHelper.mTime )
	{
		return false;
	}

	if ( mFlags & FX_RELATIVE )
	{
		if ( mClientID < 0 || mClientID >= ENTITYNUM_WORLD )
		{	// we are somehow not bolted even though the flag is on?
			return false;
		}

		vec3_t	ax[3] = {};
		// Get our current position and direction
		if (mModelNum>=0 && mBoltNum>=0)	//bolt style
		{
			const centity_t &cent = cg_entities[mClientID];
			if (!cent.gent->ghoul2.IsValid())
			{
				return false;
			}

			if (!theFxHelper.GetOriginAxisFromBolt(cent, mModelNum, mBoltNum, mOrigin1, ax))
			{	//could not get bolt
				return false;
			}
		}
		else
		{//fixme change this to bolt style...
			// Get our current position and direction
			GetOrigin( mClientID, mOrigin1 );
			GetDir( mClientID, ax[0] );
		}

		VectorAdd(mOrigin1, mOrgOffset, mOrigin1);	//add the offset to the bolt point

		vec3_t	end;
		trace_t	trace;
		if ( mFlags & FX_APPLY_PHYSICS )
		{
			VectorMA( mOrigin1, 2048, ax[0], end );

			theFxHelper.Trace( &trace, mOrigin1, NULL, NULL, end, mClientID, MASK_SHOT );

			VectorCopy( trace.endpos, mOrigin2 );

			if ( mImpactFxID > 0 )
			{
				theFxScheduler.PlayEffect( mImpactFxID, trace.endpos, trace.plane.normal );
			}
		}
		else
		{
			VectorMA( mOrigin1, mVel[0], ax[0], mOrigin2 );
			VectorMA( mOrigin2, mVel[1], ax[1], mOrigin2 );
			VectorMA( mOrigin2, mVel[2], ax[2], mOrigin2 );
		}
	}

	UpdateSize();
	UpdateRGB();
	UpdateAlpha();

	Draw();

	return true;
}


//----------------------------
//
// Derived Electricity Class
//
//----------------------------
void CElectricity::Initialize()
{
	mRefEnt.frame = Q_flrand(0.0f, 1.0f) * 1265536;
	mRefEnt.endTime = cg.time + (mTimeEnd - mTimeStart);

	if ( mFlags & FX_DEPTH_HACK )
	{
		mRefEnt.renderfx |= RF_DEPTHHACK;
	}

	if ( mFlags & FX_BRANCH )
	{
		mRefEnt.renderfx |= RF_FORKED;
	}

	if ( mFlags & FX_TAPER )
	{
		mRefEnt.renderfx |= RF_TAPERED;
	}

	if ( mFlags & FX_GROW )
	{
		mRefEnt.renderfx |= RF_GROW;
	}
}

//----------------------------
void CElectricity::Draw()
{
	VectorCopy( mOrigin1, mRefEnt.origin );
	VectorCopy( mOrigin2, mRefEnt.oldorigin );
	mRefEnt.angles[0] = mChaos;
	mRefEnt.angles[1] = mTimeEnd - mTimeStart;

	theFxHelper.AddFxToScene( &mRefEnt );

	drawnFx++;
	mLines++; // NOT REALLY A LINE!
}

//----------------------------
bool CElectricity::Update()
{
	// Game pausing can cause dumb time things to happen, so kill the effect in this instance
	if ( mTimeStart > theFxHelper.mTime )
	{
		return false;
	}

	//Handle Relative and Bolted Effects
	if ( mFlags & FX_RELATIVE )
	{//add mOrgOffset to bolt position and store in mOrigin1
		if ( mClientID < 0 || mClientID >= ENTITYNUM_WORLD )
		{	// we are somehow not bolted even though the flag is on?
			return false;
		}

		vec3_t	ax[3] = {};
		// Get our current position and direction
		if (mModelNum>=0 && mBoltNum>=0)	//bolt style
		{
			const centity_t &cent = cg_entities[mClientID];
			if (!cent.gent->ghoul2.IsValid())
			{
				return false;
			}

			if (!theFxHelper.GetOriginAxisFromBolt(cent, mModelNum, mBoltNum, mOrigin1, ax))
			{	//could not get bolt
				return false;
			}
		}
		else
		{//fixme change this to bolt style...
			// Get our current position and direction
			GetOrigin( mClientID, mOrigin1 );
			GetDir( mClientID, ax[0] );
		}

		//add the offset to the bolt point
		VectorAdd(mOrigin1, mOrgOffset, mOrigin1);

		//add the endpoint offset to the start to get the final offset
		VectorMA( mOrigin1, mVel[0], ax[0], mOrigin2 );
		VectorMA( mOrigin2, mVel[1], ax[1], mOrigin2 );
		VectorMA( mOrigin2, mVel[2], ax[2], mOrigin2 );
	}
	//else just uses the static origin1 & origin2 as start and end
	UpdateSize();
	UpdateRGB();
	UpdateAlpha();

	Draw();

	return true;
}


//----------------------------
//
// Derived Tail Class
//
//----------------------------
bool CTail::Cull()
{
	vec3_t	dir;

	// Get the direction to the view
	VectorSubtract( mOrigin1, cg.refdef.vieworg, dir );

	// Check if it's behind the viewer
	if ( (DotProduct( cg.refdef.viewaxis[0], dir )) < 0 )
	{
		return true;
	}

	return false;
}

//----------------------------
void CTail::Draw()
{
	if ( mFlags & FX_DEPTH_HACK )
	{
		// Not sure if first person needs to be set
		mRefEnt.renderfx |= RF_DEPTHHACK;
	}

	VectorCopy( mOrigin1, mRefEnt.origin );

	theFxHelper.AddFxToScene( &mRefEnt );

	drawnFx++;
	mTails++;
}

//----------------------------
bool CTail::Update()
{
	// Game pausing can cause dumb time things to happen, so kill the effect in this instance
	if ( mTimeStart > theFxHelper.mTime )
	{
		return false;
	}

	if ( !fx_freeze.integer )
	{
		VectorCopy( mOrigin1, mOldOrigin );
	}

	if ( mFlags & FX_RELATIVE )
	{
		if ( mClientID < 0 || mClientID >= ENTITYNUM_WORLD )
		{	// we are somehow not bolted even though the flag is on?
			return false;
		}

		vec3_t	org;
		vec3_t	ax[3];
		if (mModelNum>=0 && mBoltNum>=0)	//bolt style
		{
			const centity_t &cent = cg_entities[mClientID];
			if (!cent.gent->ghoul2.IsValid())
			{
				return false;
			}

			if (!theFxHelper.GetOriginAxisFromBolt(cent, mModelNum, mBoltNum, org, ax))
			{	//could not get bolt
				return false;
			}
		}
		else
		{
			vec3_t	dir;
			// Get our current position and direction
			GetOrigin( mClientID, org );
			GetDir( mClientID, dir );
			vec3_t ang;

			vectoangles( dir, ang );
			AngleVectors( ang, ax[0], ax[1], ax[2] );
		}

		vec3_t 	realVel, realAccel;

		VectorMA( org, mOrgOffset[0], ax[0], org );
		VectorMA( org, mOrgOffset[1], ax[1], org );
		VectorMA( org, mOrgOffset[2], ax[2], org );

		// calc the real velocity and accel vectors
		// FIXME: if you want right and up movement in addition to the forward movement, you'll have to convert dir into a set of perp. axes and do some extra work
		VectorScale( ax[0], mVel[0], realVel );
		VectorMA( realVel, mVel[1], ax[1], realVel );
		VectorMA( realVel, mVel[2], ax[2], realVel );

		VectorScale( ax[0], mAccel[0], realAccel );
		VectorMA( realAccel, mAccel[1], ax[1], realAccel );
		VectorMA( realAccel, mAccel[2], ax[2], realAccel );

		const float	time = (theFxHelper.mTime - mTimeStart) * 0.001f;

		// Get our real velocity at the current time, taking into account the effects of acceleration.  NOTE: not sure if this is even 100% correct math-wise
		VectorMA( realVel, time, realAccel, realVel );

		// Now move us to where we should be at the given time
		VectorMA( org, time, realVel, mOrigin1 );

		// Just calc an old point some time in the past, doesn't really matter when
		VectorMA( org, (time - 0.003f), realVel, mOldOrigin );
	}
	else if (( mTimeStart < theFxHelper.mTime ) && UpdateOrigin() == false )
	{
		// we are marked for death
		return false;
	}

	if ( !Cull() )
	{
		UpdateSize();
		UpdateLength();
		UpdateRGB();
		UpdateAlpha();

		CalcNewEndpoint();

		Draw();
	}

	return true;
}

//----------------------------
void CTail::UpdateLength()
{
	// completely biased towards start if it doesn't get overridden
	float	perc1 = 1.0f, perc2 = 1.0f;

	if ( mFlags & FX_LENGTH_LINEAR )
	{
		// calculate element biasing
		perc1 = 1.0f - (float)(theFxHelper.mTime - mTimeStart)
						/ (float)(mTimeEnd - mTimeStart);
	}

	// We can combine FX_LINEAR with _either_ FX_NONLINEAR or FX_WAVE
	if (( mFlags & FX_LENGTH_PARM_MASK ) == FX_LENGTH_NONLINEAR )
	{
		if ( theFxHelper.mTime > mLengthParm )
		{
			// get percent done, using parm as the start of the non-linear fade
			perc2 = 1.0f - (float)(theFxHelper.mTime - mLengthParm)
							/ (float)(mTimeEnd - mLengthParm);
		}

		if ( mFlags & FX_LENGTH_LINEAR )
		{
			// do an even blend
			perc1 = perc1 * 0.5f + perc2 * 0.5f;
		}
		else
		{
			// just copy it over...sigh
			perc1 = perc2;
		}
	}
	else if (( mFlags & FX_LENGTH_PARM_MASK ) == FX_LENGTH_WAVE )
	{
		// wave gen, with parm being the frequency multiplier
		perc1 = perc1 * (float)cos( (theFxHelper.mTime - mTimeStart) * mLengthParm );
	}
	else if (( mFlags & FX_LENGTH_PARM_MASK ) == FX_LENGTH_CLAMP )
	{
		if ( theFxHelper.mTime < mLengthParm )
		{
			// get percent done, using parm as the start of the non-linear fade
			perc2 = (float)(mLengthParm - theFxHelper.mTime)
							/ (float)(mLengthParm - mTimeStart);
		}
		else
		{
			perc2 = 0.0f; // make it full size??
		}

		if ( mFlags & FX_LENGTH_LINEAR )
		{
			// do an even blend
			perc1 = perc1 * 0.5f + perc2 * 0.5f;
		}
		else
		{
			// just copy it over...sigh
			perc1 = perc2;
		}
	}

	// If needed, RAND can coexist with linear and either non-linear or wave.
	if ( mFlags & FX_LENGTH_RAND )
	{
		// Random simply modulates the existing value
		perc1 = Q_flrand(0.0f, 1.0f) * perc1;
	}

	mLength = (mLengthStart * perc1) + (mLengthEnd * (1.0f - perc1));
}

//----------------------------
void CTail::CalcNewEndpoint()
{
	vec3_t temp;

	// FIXME:  Hmmm, this looks dumb when physics are on and a bounce happens
	VectorSubtract( mOldOrigin, mOrigin1, temp );

	// I wish we didn't have to do a VectorNormalize every frame.....
	VectorNormalize( temp );

	VectorMA( mOrigin1, mLength, temp, mRefEnt.oldorigin );
}


//----------------------------
//
// Derived Cylinder Class
//
//----------------------------
void CCylinder::Draw()
{
	if ( mFlags & FX_DEPTH_HACK )
	{
		// Not sure if first person needs to be set, but it can't hurt?
		mRefEnt.renderfx |= RF_DEPTHHACK;
	}

	VectorCopy( mOrigin1, mRefEnt.origin );
	VectorMA( mOrigin1, mLength, mRefEnt.axis[0], mRefEnt.oldorigin );

	theFxHelper.AddFxToScene( &mRefEnt );

	drawnFx++;
}

//----------------------------
// Update Size2
//----------------------------
void CCylinder::UpdateSize2()
{
	// completely biased towards start if it doesn't get overridden
	float	perc1 = 1.0f, perc2 = 1.0f;

	if ( mFlags & FX_SIZE2_LINEAR )
	{
		// calculate element biasing
		perc1 = 1.0f - (float)(theFxHelper.mTime - mTimeStart)
						/ (float)(mTimeEnd - mTimeStart);
	}

	// We can combine FX_LINEAR with _either_ FX_NONLINEAR or FX_WAVE
	if (( mFlags & FX_SIZE2_PARM_MASK ) == FX_SIZE2_NONLINEAR )
	{
		if ( theFxHelper.mTime > mSize2Parm )
		{
			// get percent done, using parm as the start of the non-linear fade
			perc2 = 1.0f - (float)(theFxHelper.mTime - mSize2Parm)
							/ (float)(mTimeEnd - mSize2Parm);
		}

		if ( (mFlags & FX_SIZE2_LINEAR) )
		{
			// do an even blend
			perc1 = perc1 * 0.5f + perc2 * 0.5f;
		}
		else
		{
			// just copy it over...sigh
			perc1 = perc2;
		}
	}
	else if (( mFlags & FX_SIZE2_PARM_MASK ) == FX_SIZE2_WAVE )
	{
		// wave gen, with parm being the frequency multiplier
		perc1 = perc1 * (float)cos( (theFxHelper.mTime - mTimeStart) * mSize2Parm );
	}
	else if (( mFlags & FX_SIZE2_PARM_MASK ) == FX_SIZE2_CLAMP )
	{
		if ( theFxHelper.mTime < mSize2Parm )
		{
			// get percent done, using parm as the start of the non-linear fade
			perc2 = (float)(mSize2Parm - theFxHelper.mTime)
							/ (float)(mSize2Parm - mTimeStart);
		}
		else
		{
			perc2 = 0.0f; // make it full size??
		}

		if ( mFlags & FX_SIZE2_LINEAR )
		{
			// do an even blend
			perc1 = perc1 * 0.5f + perc2 * 0.5f;
		}
		else
		{
			// just copy it over...sigh
			perc1 = perc2;
		}
	}

	// If needed, RAND can coexist with linear and either non-linear or wave.
	if ( mFlags & FX_SIZE2_RAND )
	{
		// Random simply modulates the existing value
		perc1 = Q_flrand(0.0f, 1.0f) * perc1;
	}

	mRefEnt.backlerp = (mSize2Start * perc1) + (mSize2End * (1.0f - perc1));
}

//----------------------------
bool CCylinder::Update()
{
	// Game pausing can cause dumb time things to happen, so kill the effect in this instance
	if ( mTimeStart > theFxHelper.mTime )
	{
		return false;
	}

	if ( mFlags & FX_RELATIVE )
	{
		if ( mClientID < 0 || mClientID >= ENTITYNUM_WORLD )
		{	// we are somehow not bolted even though the flag is on?
			return false;
		}

		vec3_t	ax[3] = {};
		// Get our current position and direction
		if (mModelNum>=0 && mBoltNum>=0)	//bolt style
		{
			const centity_t &cent = cg_entities[mClientID];
			if (!cent.gent->ghoul2.IsValid())
			{
				return false;
			}

			if (!theFxHelper.GetOriginAxisFromBolt(cent, mModelNum, mBoltNum, mOrigin1, ax))
			{	//could not get bolt
				return false;
			}
		}
		else
		{//fixme change this to bolt style...
			// Get our current position and direction
			GetOrigin( mClientID, mOrigin1 );
			GetDir( mClientID, ax[0] );
		}

		VectorAdd(mOrigin1, mOrgOffset, mOrigin1);	//add the offset to the bolt point

		VectorCopy( ax[0], mRefEnt.axis[0] );
		//FIXME: should mNormal be a modifier on the forward axis?
		/*
		VectorMA( mOrigin1, mNormal[0], ax[0], mOrigin2 );
		VectorMA( mOrigin2, mNormal[1], ax[1], mOrigin2 );
		VectorMA( mOrigin2, mNormal[2], ax[2], mOrigin2 );
		*/
	}

	UpdateSize();
	UpdateSize2();
	UpdateLength();
	UpdateRGB();
	UpdateAlpha();

	Draw();

	return true;
}


//----------------------------
//
// Derived Emitter Class
//
//----------------------------

//----------------------------
// Draw
//----------------------------
void CEmitter::Draw()
{
	// Emitters don't draw themselves, but they may need to add an attached model
	if ( mFlags & FX_ATTACHED_MODEL )
	{
		mRefEnt.nonNormalizedAxes = qtrue;

		VectorCopy( mOrigin1, mRefEnt.origin );

		// ensure that we are sized
		for ( int i = 0; i < 3; i++ )
		{
			VectorScale( mRefEnt.axis[i], mRefEnt.radius, mRefEnt.axis[i] );
		}

		theFxHelper.AddFxToScene( &mRefEnt );
	}

	// If we are emitting effects, we had better be careful because just calling it every cgame frame could
	//	either choke up the effects system on a fast machine, or look really nasty on a low end one.
	if ( mFlags & FX_EMIT_FX )
	{
		vec3_t	org, v;
		float	ftime, time2,
				step;
		int		i, t, dif;

#define TRAIL_RATE		8 // we "think" at about a 60hz rate

		// Pick a target step distance and square it
		step = mDensity + Q_flrand(-1.0f, 1.0f) * mVariance;
		step *= step;

		dif = 0;

		for ( t = mOldTime; t <= theFxHelper.mTime; t += TRAIL_RATE )
		{
			dif += TRAIL_RATE;

			// ?Not sure if it's better to update this before or after updating the origin
			VectorMA( mOldVelocity, dif * 0.001f, mAccel, v );

			// Calc the time differences
			ftime = dif * 0.001f;
			time2 = ftime * ftime * 0.5f;

			// Predict the new position
			for ( i = 0 ; i < 3 ; i++ )
			{
				org[i] = mOldOrigin[i] + ftime * v[i] + time2 * v[i];
			}

			// Only perform physics if this object is tagged to do so
			if ( (mFlags & FX_APPLY_PHYSICS) )
			{
				bool solid;

				if ( (mFlags&FX_EXPENSIVE_PHYSICS)
					&& fx_expensivePhysics.integer )
				{
					solid = true; // by setting this to true, we force a real trace to happen
				}
				else
				{
					// if this returns solid, we need to do a trace
					solid = !!(CG_PointContents( org, ENTITYNUM_WORLD ) & MASK_SHOT);
				}

				if ( solid )
				{
					trace_t	trace;

					if ( mFlags & FX_USE_BBOX )
					{
						theFxHelper.Trace( &trace, mOldOrigin, mMin, mMax, org, -1, MASK_SHOT );
					}
					else
					{
						theFxHelper.Trace( &trace, mOldOrigin, NULL, NULL, org, -1, MASK_SHOT );
					}

					// Hit something
					if ( trace.fraction < 1.0f || trace.startsolid || trace.allsolid )
					{
						return;
					}
				}
			}

			// Is it time to draw an effect?
			if ( DistanceSquared( org, mOldOrigin ) >= step )
			{
				// Pick a new target step distance and square it
				step = mDensity + Q_flrand(-1.0f, 1.0f) * mVariance;
				step *= step;

				// We met the step criteria so, we should add in the effect
				theFxScheduler.PlayEffect( mEmitterFxID, org, mRefEnt.axis );

				VectorCopy( org, mOldOrigin );
				VectorCopy( v, mOldVelocity );
				dif = 0;
				mOldTime = t;
			}
		}
	}

	drawnFx++;
}

//----------------------------
bool CEmitter::Update()
{
	// Game pausing can cause dumb time things to happen, so kill the effect in this instance
	if ( mTimeStart > theFxHelper.mTime )
	{
		return false;
	}

	//FIXME: Handle Relative and Bolted Effects
	/*
	if ( mFlags & FX_RELATIVE )
	{
		if ( mClientID < 0 || mClientID >= ENTITYNUM_WORLD )
		{	// we are somehow not bolted even though the flag is on?
			return false;
		}

		// Get our current position and direction
		if (mModelNum>=0 && mBoltNum>=0)	//bolt style
		{
		}
	}
	*/
	// Use this to track if we've stopped moving
	VectorCopy( mOrigin1, mOldOrigin );
	VectorCopy( mVel, mOldVelocity );

	if (( mTimeStart < theFxHelper.mTime ) && UpdateOrigin() == false )
	{	// we are marked for death
		return false;
	}

	// If the thing is no longer moving, kill the angle delta, but don't do it too quickly or it will
	//	look very artificial.  Don't do it too slowly or it will look like there is no friction.
	if ( VectorCompare( mOldOrigin, mOrigin1 ))
	{
		VectorScale( mAngleDelta, 0.6f, mAngleDelta );
	}

	UpdateAngles();
	UpdateSize();
//	UpdateRGB();	// had wanted to do something slick whereby an emitted effect could somehow inherit these
//	UpdateAlpha();	// values, but it's not a priority right now.

	Draw();

	return true;
}

//----------------------------
void CEmitter::UpdateAngles()
{
	VectorMA( mAngles, theFxHelper.mFrameTime * 0.01f, mAngleDelta, mAngles ); // was 0.001f, but then you really have to jack up the delta to even notice anything
	AnglesToAxis( mAngles, mRefEnt.axis );
}


//--------------------------
//
// Derived Light Class
//
//--------------------------
//----------------------------
// Update
//----------------------------
bool CLight::Update()
{
	// Game pausing can cause dumb time things to happen, so kill the effect in this instance
	if ( mTimeStart > theFxHelper.mTime )
	{
		return false;
	}

	//FIXME: Handle Relative and Bolted Effects
	/*
	if ( mFlags & FX_RELATIVE )
	{
		if ( mClientID < 0 || mClientID >= ENTITYNUM_WORLD )
		{	// we are somehow not bolted even though the flag is on?
			return false;
		}

		// Get our current position and direction
		if (mModelNum>=0 && mBoltNum>=0)	//bolt style
		{
		}
	}
	*/
	UpdateSize();
	UpdateRGB();

	Draw();

	return true;
}

//----------------------------
// Update Size
//----------------------------
void CLight::UpdateSize()
{
	// completely biased towards start if it doesn't get overridden
	float	perc1 = 1.0f, perc2 = 1.0f;

	if ( mFlags & FX_SIZE_LINEAR )
	{
		// calculate element biasing
		perc1 = 1.0f - (float)(theFxHelper.mTime - mTimeStart)
						/ (float)(mTimeEnd - mTimeStart);
	}

	// We can combine FX_LINEAR with _either_ FX_NONLINEAR or FX_WAVE
	if (( mFlags & FX_SIZE_PARM_MASK ) == FX_SIZE_NONLINEAR )
	{
		if ( theFxHelper.mTime > mSizeParm )
		{
			// get percent done, using parm as the start of the non-linear fade
			perc2 = 1.0f - (float)(theFxHelper.mTime - mSizeParm)
							/ (float)(mTimeEnd - mSizeParm);
		}

		if ( (mFlags & FX_SIZE_LINEAR) )
		{
			// do an even blend
			perc1 = perc1 * 0.5f + perc2 * 0.5f;
		}
		else
		{
			// just copy it over...sigh
			perc1 = perc2;
		}
	}
	else if (( mFlags & FX_SIZE_PARM_MASK ) == FX_SIZE_WAVE )
	{
		// wave gen, with parm being the frequency multiplier
		perc1 = perc1 * (float)cos( (theFxHelper.mTime - mTimeStart) * mSizeParm );
	}
	else if (( mFlags & FX_SIZE_PARM_MASK ) == FX_SIZE_CLAMP )
	{
		if ( theFxHelper.mTime < mSizeParm )
		{
			// get percent done, using parm as the start of the non-linear fade
			perc2 = (float)(mSizeParm - theFxHelper.mTime)
							/ (float)(mSizeParm - mTimeStart);
		}
		else
		{
			perc2 = 0.0f; // make it full size??
		}

		if ( mFlags & FX_SIZE_LINEAR )
		{
			// do an even blend
			perc1 = perc1 * 0.5f + perc2 * 0.5f;
		}
		else
		{
			// just copy it over...sigh
			perc1 = perc2;
		}
	}

	// If needed, RAND can coexist with linear and either non-linear or wave.
	if ( mFlags & FX_SIZE_RAND )
	{
		// Random simply modulates the existing value
		perc1 = Q_flrand(0.0f, 1.0f) * perc1;
	}

	mRefEnt.radius = (mSizeStart * perc1) + (mSizeEnd * (1.0f - perc1));
}

//----------------------------
// Update RGB
//----------------------------
void CLight::UpdateRGB()
{
	// completely biased towards start if it doesn't get overridden
	float	perc1 = 1.0f, perc2 = 1.0f;
	vec3_t	res;

	if ( mFlags & FX_RGB_LINEAR )
	{
		// calculate element biasing
		perc1 = 1.0f - (float)( theFxHelper.mTime - mTimeStart )
						/ (float)( mTimeEnd - mTimeStart );
	}

	// We can combine FX_LINEAR with _either_ FX_NONLINEAR or FX_WAVE
	if (( mFlags & FX_RGB_PARM_MASK ) == FX_RGB_NONLINEAR )
	{
		if ( theFxHelper.mTime > mRGBParm )
		{
			// get percent done, using parm as the start of the non-linear fade
			perc2 = 1.0f - (float)( theFxHelper.mTime - mRGBParm )
							/ (float)( mTimeEnd - mRGBParm );
		}

		if ( mFlags & FX_RGB_LINEAR )
		{
			// do an even blend
			perc1 = perc1 * 0.5f + perc2 * 0.5f;
		}
		else
		{
			// just copy it over...sigh
			perc1 = perc2;
		}
	}
	else if (( mFlags & FX_RGB_PARM_MASK ) == FX_RGB_WAVE )
	{
		// wave gen, with parm being the frequency multiplier
		perc1 = perc1 * (float)cos(( theFxHelper.mTime - mTimeStart ) * mRGBParm );
	}
	else if (( mFlags & FX_RGB_PARM_MASK ) == FX_RGB_CLAMP )
	{
		if ( theFxHelper.mTime < mRGBParm )
		{
			// get percent done, using parm as the start of the non-linear fade
			perc2 = (float)(mRGBParm - theFxHelper.mTime)
							/ (float)(mRGBParm - mTimeStart);
		}
		else
		{
			perc2 = 0.0f; // make it full size??
		}

		if ( mFlags & FX_RGB_LINEAR )
		{
			// do an even blend
			perc1 = perc1 * 0.5f + perc2 * 0.5f;
		}
		else
		{
			// just copy it over...sigh
			perc1 = perc2;
		}
	}

	// If needed, RAND can coexist with linear and either non-linear or wave.
	if ( mFlags & FX_RGB_RAND )
	{
		// Random simply modulates the existing value
		perc1 = Q_flrand(0.0f, 1.0f) * perc1;
	}

	// Now get the correct color
	VectorScale( mRGBStart, perc1, res );

	mRefEnt.lightingOrigin[0] = res[0] + ( 1.0f - perc1 ) * mRGBEnd[0];
	mRefEnt.lightingOrigin[1] = res[1] + ( 1.0f - perc1 ) * mRGBEnd[1];
	mRefEnt.lightingOrigin[2] = res[2] + ( 1.0f - perc1 ) * mRGBEnd[2];
}


//--------------------------
//
// Derived Trail Class
//
//--------------------------
#define NEW_MUZZLE	0
#define NEW_TIP		1
#define OLD_TIP		2
#define OLD_MUZZLE	3

//----------------------------
void CTrail::Draw()
{
	polyVert_t	verts[3];
//	vec3_t		color;

	// build the first tri out of the new muzzle...new tip...old muzzle
	VectorCopy( mVerts[NEW_MUZZLE].origin, verts[0].xyz );
	VectorCopy( mVerts[NEW_TIP].origin, verts[1].xyz );
	VectorCopy( mVerts[OLD_MUZZLE].origin, verts[2].xyz );

//	VectorScale( mVerts[NEW_MUZZLE].curRGB, mVerts[NEW_MUZZLE].curAlpha, color );
	verts[0].modulate[0] = mVerts[NEW_MUZZLE].rgb[0];
	verts[0].modulate[1] = mVerts[NEW_MUZZLE].rgb[1];
	verts[0].modulate[2] = mVerts[NEW_MUZZLE].rgb[2];
	verts[0].modulate[3] = mVerts[NEW_MUZZLE].alpha;

//	VectorScale( mVerts[NEW_TIP].curRGB, mVerts[NEW_TIP].curAlpha, color );
	verts[1].modulate[0] = mVerts[NEW_TIP].rgb[0];
	verts[1].modulate[1] = mVerts[NEW_TIP].rgb[1];
	verts[1].modulate[2] = mVerts[NEW_TIP].rgb[2];
	verts[1].modulate[3] = mVerts[NEW_TIP].alpha;

//	VectorScale( mVerts[OLD_MUZZLE].curRGB, mVerts[OLD_MUZZLE].curAlpha, color );
	verts[2].modulate[0] = mVerts[OLD_MUZZLE].rgb[0];
	verts[2].modulate[1] = mVerts[OLD_MUZZLE].rgb[1];
	verts[2].modulate[2] = mVerts[OLD_MUZZLE].rgb[2];
	verts[2].modulate[3] = mVerts[OLD_MUZZLE].alpha;

	verts[0].st[0] = mVerts[NEW_MUZZLE].curST[0];
	verts[0].st[1] = mVerts[NEW_MUZZLE].curST[1];
	verts[1].st[0] = mVerts[NEW_TIP].curST[0];
	verts[1].st[1] = mVerts[NEW_TIP].curST[1];
	verts[2].st[0] = mVerts[OLD_MUZZLE].curST[0];
	verts[2].st[1] = mVerts[OLD_MUZZLE].curST[1];

	// Add this tri
	theFxHelper.AddPolyToScene( mShader, 3, verts );

	// build the second tri out of the old muzzle...old tip...new tip
	VectorCopy( mVerts[OLD_MUZZLE].origin, verts[0].xyz );
	VectorCopy( mVerts[OLD_TIP].origin, verts[1].xyz );
	VectorCopy( mVerts[NEW_TIP].origin, verts[2].xyz );

//	VectorScale( mVerts[OLD_MUZZLE].curRGB, mVerts[OLD_MUZZLE].curAlpha, color );
	verts[0].modulate[0] = mVerts[OLD_MUZZLE].rgb[0];
	verts[0].modulate[1] = mVerts[OLD_MUZZLE].rgb[1];
	verts[0].modulate[2] = mVerts[OLD_MUZZLE].rgb[2];
	verts[0].modulate[3] = mVerts[OLD_MUZZLE].alpha;

//	VectorScale( mVerts[OLD_TIP].curRGB, mVerts[OLD_TIP].curAlpha, color );
	verts[1].modulate[0] = mVerts[OLD_TIP].rgb[0];
	verts[1].modulate[1] = mVerts[OLD_TIP].rgb[1];
	verts[1].modulate[2] = mVerts[OLD_TIP].rgb[2];
	verts[0].modulate[3] = mVerts[OLD_TIP].alpha;

//	VectorScale( mVerts[NEW_TIP].curRGB, mVerts[NEW_TIP].curAlpha, color );
	verts[2].modulate[0] = mVerts[NEW_TIP].rgb[0];
	verts[2].modulate[1] = mVerts[NEW_TIP].rgb[1];
	verts[2].modulate[2] = mVerts[NEW_TIP].rgb[2];
	verts[0].modulate[3] = mVerts[NEW_TIP].alpha;

	verts[0].st[0] = mVerts[OLD_MUZZLE].curST[0];
	verts[0].st[1] = mVerts[OLD_MUZZLE].curST[1];
	verts[1].st[0] = mVerts[OLD_TIP].curST[0];
	verts[1].st[1] = mVerts[OLD_TIP].curST[1];
	verts[2].st[0] = mVerts[NEW_TIP].curST[0];
	verts[2].st[1] = mVerts[NEW_TIP].curST[1];

	// Add this tri
	theFxHelper.AddPolyToScene( mShader, 3, verts );

	drawnFx++;
}

//----------------------------
// Update
//----------------------------
bool CTrail::Update()
{
	// Game pausing can cause dumb time things to happen, so kill the effect in this instance
	if ( mTimeStart > theFxHelper.mTime )
	{
		return false;
	}

	//FIXME: Handle Relative and Bolted Effects
	/*
	if ( mFlags & FX_RELATIVE )
	{
		if ( mClientID < 0 || mClientID >= ENTITYNUM_WORLD )
		{	// we are somehow not bolted even though the flag is on?
			return false;
		}

		// Get our current position and direction
		if (mModelNum>=0 && mBoltNum>=0)	//bolt style
		{
		}
	}
	*/
	float perc = (float)(mTimeEnd - theFxHelper.mTime) / (float)(mTimeEnd - mTimeStart);

	for ( int t = 0; t < 4; t++ )
	{
//		mVerts[t].curAlpha = mVerts[t].alpha * perc + mVerts[t].destAlpha * ( 1.0f - perc );
//		if ( mVerts[t].curAlpha < 0.0f )
//		{
//			mVerts[t].curAlpha = 0.0f;
//		}

//		VectorScale( mVerts[t].rgb, perc, mVerts[t].curRGB );
//		VectorMA( mVerts[t].curRGB, ( 1.0f - perc ), mVerts[t].destrgb, mVerts[t].curRGB );
		mVerts[t].curST[0] = mVerts[t].ST[0] * perc + mVerts[t].destST[0] * ( 1.0f - perc );
		if ( mVerts[t].curST[0] > 1.0f )
		{
			mVerts[t].curST[0] = 1.0f;
		}
		mVerts[t].curST[1] = mVerts[t].ST[1] * perc + mVerts[t].destST[1] * ( 1.0f - perc );
	}

	Draw();

	return true;
}


//--------------------------
//
// Derived Poly Class
//
//--------------------------
bool CPoly::Cull()
{
	vec3_t	dir;

	// Get the direction to the view
	VectorSubtract( mOrigin1, cg.refdef.vieworg, dir );

	// Check if it's behind the viewer
	if ( (DotProduct( cg.refdef.viewaxis[0], dir )) < 0 )
	{
		return true;
	}

	float len = VectorLengthSquared( dir );

	// Can't be too close
	if ( len < 24 * 24 )
	{
		return true;
	}

	return false;
}

//----------------------------
void CPoly::Draw()
{
	polyVert_t	verts[MAX_CPOLY_VERTS];

	for ( int i = 0; i < mCount; i++ )
	{
		// Add our midpoint and vert offset to get the actual vertex
		VectorAdd( mOrigin1, mOrg[i], verts[i].xyz );

		// Assign the same color to each vert
		for ( int k=0; k<4; k++ )
			verts[i].modulate[k] = mRefEnt.shaderRGBA[k];

		// Copy the ST coords
		VectorCopy2( mST[i], verts[i].st );
	}

	// Add this poly
	theFxHelper.AddPolyToScene( mRefEnt.customShader, mCount, verts );

	drawnFx++;
}

//----------------------------
void CPoly::CalcRotateMatrix()
{
	float	cosX, cosZ;
	float	sinX, sinZ;
	float	rad;

	// rotate around Z
	rad = DEG2RAD( mRotDelta[YAW] * theFxHelper.mFrameTime * 0.01f );
	cosZ = cos( rad );
	sinZ = sin( rad );
	// rotate around X
	rad = DEG2RAD( mRotDelta[PITCH] * theFxHelper.mFrameTime * 0.01f );
	cosX = cos( rad );
	sinX = sin( rad );

/*Pitch - aroundx  Yaw - around z
1 0  0			 c -s 0
0 c -s			 s  c 0
0 s  c			 0  0 1
*/
	mRot[0][0] = cosZ;
	mRot[1][0] = -sinZ;
	mRot[2][0] = 0;
	mRot[0][1] = cosX * sinZ;
	mRot[1][1] = cosX * cosZ;
	mRot[2][1] = -sinX;
	mRot[0][2] = sinX * sinZ;
	mRot[1][2] = sinX * cosZ;
	mRot[2][2] = cosX;
/*
// ROLL is not supported unless anyone complains, if it needs to be added, use this format
Roll

 c 0 s
 0 1 0
-s 0 c
*/
	mLastFrameTime = theFxHelper.mFrameTime;
}

//--------------------------------
void CPoly::Rotate()
{
	vec3_t	temp[MAX_CPOLY_VERTS];
	float	dif = abs(mLastFrameTime - theFxHelper.mFrameTime);

	// Very generous check with frameTimes
	if ( dif > 0.5f * mLastFrameTime )
	{
		CalcRotateMatrix();
	}

	// Multiply our rotation matrix by each of the offset verts to get their new position
	for ( int i = 0; i < mCount; i++ )
	{
		VectorRotate( mOrg[i], mRot, temp[i] );
		VectorCopy( temp[i], mOrg[i] );
	}
}

//----------------------------
// Update
//----------------------------
bool CPoly::Update()
{
	vec3_t mOldOrigin = { 0.0f };

	//FIXME: Handle Relative and Bolted Effects
	/*
	if ( mFlags & FX_RELATIVE )
	{
		if ( mClientID < 0 || mClientID >= ENTITYNUM_WORLD )
		{	// we are somehow not bolted even though the flag is on?
			return false;
		}

		// Get our current position and direction
		if (mModelNum>=0 && mBoltNum>=0)	//bolt style
		{
		}
	}
	*/
	// Game pausing can cause dumb time things to happen, so kill the effect in this instance
	if ( mTimeStart > theFxHelper.mTime )
	{
		return false;
	}

	// If our timestamp hasn't exired yet, we won't even consider doing any kind of motion
	if ( theFxHelper.mTime > mTimeStamp )
	{
		VectorCopy( mOrigin1, mOldOrigin );

		if (( mTimeStart < theFxHelper.mTime ) && UpdateOrigin() == false )
		{
			// we are marked for death
			return false;
		}
	}

	if ( !Cull() )
	{
		// only rotate when our start timestamp has expired
		if ( theFxHelper.mTime > mTimeStamp )
		{
			// Only rotate whilst moving
			if ( !VectorCompare( mOldOrigin, mOrigin1 ))
			{
				Rotate();
			}
		}

		UpdateRGB();
		UpdateAlpha();

		Draw();
	}

	return true;
}

//----------------------------
void CPoly::PolyInit()
{
	if ( mCount < 3 )
	{
		return;
	}

	int		i;
	vec3_t	org={0,0,0};

	// Find our midpoint
	for ( i = 0; i < mCount; i++ )
	{
		VectorAdd( org, mOrg[i], org );
	}

	VectorScale( org, (float)(1.0f / mCount), org );

	// now store our midpoint for physics purposes
	VectorCopy( org, mOrigin1 );

	// Now we process the passed in points and make it so that they aren't actually the point...
	//	rather, they are the offset from mOrigin1.
	for ( i = 0; i < mCount; i++ )
	{
		VectorSubtract( mOrg[i], mOrigin1, mOrg[i] );
	}

	CalcRotateMatrix();
}

/*
-------------------------
CBezier

Bezier curve line
-------------------------
*/
//----------------------------
bool CBezier::Update( void )
{
	float	ftime, time2;

	//FIXME: Handle Relative and Bolted Effects
	/*
	if ( mFlags & FX_RELATIVE )
	{
		if ( mClientID < 0 || mClientID >= ENTITYNUM_WORLD )
		{	// we are somehow not bolted even though the flag is on?
			return false;
		}

		// Get our current position and direction
		if (mModelNum>=0 && mBoltNum>=0)	//bolt style
		{
		}
	}
	*/
	ftime = cg.frametime * 0.001f;
	time2 = ftime * ftime * 0.5f;

	for ( int i = 0; i < 3; i++ )
	{
		mControl1[i] = mControl1[i] + ftime * mControl1Vel[i] + time2 * mControl1Vel[i];
		mControl2[i] = mControl2[i] + ftime * mControl2Vel[i] + time2 * mControl2Vel[i];
	}

	UpdateSize();
	UpdateRGB();
	UpdateAlpha();

	Draw();

	return true;
}

//----------------------------
void CBezier::DrawSegment( vec3_t start, vec3_t end, float texcoord1, float texcoord2 )
{
	vec3_t			lineDir, cross, viewDir;
	static vec3_t	lastEnd[2];
	polyVert_t		verts[4];
	float			scale;

	VectorSubtract( end, start, lineDir );
	VectorSubtract( end, cg.refdef.vieworg, viewDir );
	CrossProduct( lineDir, viewDir, cross );
	VectorNormalize( cross );

	scale = mRefEnt.radius * 0.5f;

	//Construct the oriented quad
	if ( mInit )
	{
		VectorCopy( lastEnd[0], verts[0].xyz );
		VectorCopy( lastEnd[1], verts[1].xyz );
	}
	else
	{
		VectorMA( start, -scale, cross, verts[0].xyz );
		VectorMA( start, scale, cross, verts[1].xyz );
	}

	verts[0].st[0] = 0.0f;
	verts[0].st[1] = texcoord1;

	verts[0].modulate[0] = mRefEnt.shaderRGBA[0] * ( 1.0f - texcoord1 );
	verts[0].modulate[1] = mRefEnt.shaderRGBA[1] * ( 1.0f - texcoord1 );
	verts[0].modulate[2] = mRefEnt.shaderRGBA[2] * ( 1.0f - texcoord1 );
	verts[0].modulate[3] = mRefEnt.shaderRGBA[3];

	verts[1].st[0] = 1.0f;
	verts[1].st[1] = texcoord1;
	verts[1].modulate[0] = mRefEnt.shaderRGBA[0] * ( 1.0f - texcoord1 );
	verts[1].modulate[1] = mRefEnt.shaderRGBA[1] * ( 1.0f - texcoord1 );
	verts[1].modulate[2] = mRefEnt.shaderRGBA[2] * ( 1.0f - texcoord1 );
	verts[1].modulate[3] = mRefEnt.shaderRGBA[3];

	if ( texcoord1 == 0.0f )
	{
		verts[0].modulate[0] = 0;
		verts[0].modulate[1] = 0;
		verts[0].modulate[2] = 0;
		verts[0].modulate[3] = 0;
		verts[1].modulate[0] = 0;
		verts[1].modulate[1] = 0;
		verts[1].modulate[2] = 0;
		verts[1].modulate[3] = 0;
	}

	VectorMA( end, scale, cross, verts[2].xyz );
	verts[2].st[0] = 1.0f;
	verts[2].st[1] = texcoord2;
	verts[2].modulate[0] = mRefEnt.shaderRGBA[0] * ( 1.0f - texcoord2 );
	verts[2].modulate[1] = mRefEnt.shaderRGBA[1] * ( 1.0f - texcoord2 );
	verts[2].modulate[2] = mRefEnt.shaderRGBA[2] * ( 1.0f - texcoord2 );
	verts[2].modulate[3] = mRefEnt.shaderRGBA[3];

	VectorMA( end, -scale, cross, verts[3].xyz );
	verts[3].st[0] = 0.0f;
	verts[3].st[1] = texcoord2;
	verts[3].modulate[0] = mRefEnt.shaderRGBA[0] * ( 1.0f - texcoord2 );
	verts[3].modulate[1] = mRefEnt.shaderRGBA[1] * ( 1.0f - texcoord2 );
	verts[3].modulate[2] = mRefEnt.shaderRGBA[2] * ( 1.0f - texcoord2 );
	verts[3].modulate[3] = mRefEnt.shaderRGBA[3];

	cgi_R_AddPolyToScene( mRefEnt.customShader, 4, verts );

	VectorCopy( verts[2].xyz, lastEnd[1] );
	VectorCopy( verts[3].xyz, lastEnd[0] );

	mInit = true;
}

const	float	BEZIER_RESOLUTION	= 16.0f;

//----------------------------
void CBezier::Draw( void )
{
	vec3_t	pos, old_pos;
    float	mu, mum1;
	float	incr = 1.0f / BEZIER_RESOLUTION, tex = 1.0f, tc1, tc2;
	int		i;

	VectorCopy( mOrigin1, old_pos );

	mInit = false;	//Signify a new batch for vert gluing

	// Calculate the texture coords so the texture can stretch along the whole bezier
//	if ( mFlags & FXF_WRAP )
//	{
//		tex = m_stScale / 1.0f;
//	}

	float mum13, mu3, group1, group2;

	tc1 = 0.0f;

	for ( mu = incr; mu <= 1.0f; mu += incr )
	{
		//Four point curve
		mum1	= 1 - mu;
		mum13	= mum1 * mum1 * mum1;
		mu3		= mu * mu * mu;
		group1	= 3 * mu * mum1 * mum1;
		group2	= 3 * mu * mu *mum1;

		for ( i = 0; i < 3; i++ )
		{
			pos[i] = mum13 * mOrigin1[i] + group1 * mControl1[i] + group2 * mControl2[i] + mu3 * mOrigin2[i];
		}

//		if ( m_flags & FXF_WRAP )
//		{
			tc2 = mu * tex;
//		}
//		else
//		{
//			// Texture will get mapped onto each segement
//			tc1 = 0.0f;
//			tc2 = 1.0f;
//		}

		//Draw it
		DrawSegment( old_pos, pos, tc1, tc2 );

		VectorCopy( pos, old_pos );
		tc1 = tc2;
	}

	drawnFx++;
	mLines++; // NOT REALLY A LINE
}

/*
-------------------------
CFlash

Full screen flash
-------------------------
*/

//----------------------------
bool CFlash::Update( void )
{
	UpdateRGB();
	Draw();

	return true;
}

//----------------------------
void CFlash::Init( void )
{
	vec3_t	dif;
	float	mod = 1.0f, dis;

	VectorSubtract( mOrigin1, cg.refdef.vieworg, dif );
	dis = VectorNormalize( dif );

	mod = DotProduct( dif, cg.refdef.viewaxis[0] );

	if ( dis > 600 || ( mod < 0.5f && dis > 100 ))
	{
		mod = 0.0f;
	}
	else if ( mod < 0.5f && dis <= 100 )
	{
		mod += 1.1f;
	}

	mod *= (1.0f - ((dis * dis) / (600.0f * 600.0f)));

	VectorScale( mRGBStart, mod, mRGBStart );
	VectorScale( mRGBEnd, mod, mRGBEnd );
}

//----------------------------
void CFlash::Draw( void )
{
    // Interestingly, if znear is set > than this, then the flash
    // doesn't appear at all.
    const float FLASH_DISTANCE_FROM_VIEWER = 8.0f;

	mRefEnt.reType = RT_SPRITE;

	for ( int i = 0; i < 3; i++ )
	{
		if ( mRefEnt.lightingOrigin[i] > 1.0f )
		{
			mRefEnt.lightingOrigin[i] = 1.0f;
		}
		else if ( mRefEnt.lightingOrigin[i] < 0.0f )
		{
			mRefEnt.lightingOrigin[i] = 0.0f;
		}
	}
	mRefEnt.shaderRGBA[0] = mRefEnt.lightingOrigin[0] * 255;
	mRefEnt.shaderRGBA[1] = mRefEnt.lightingOrigin[1] * 255;
	mRefEnt.shaderRGBA[2] = mRefEnt.lightingOrigin[2] * 255;
	mRefEnt.shaderRGBA[3] = 255;

	VectorCopy( cg.refdef.vieworg, mRefEnt.origin );
	VectorMA( mRefEnt.origin, FLASH_DISTANCE_FROM_VIEWER, cg.refdef.viewaxis[0], mRefEnt.origin );

    // This is assuming that the screen is wider than it is tall.
    mRefEnt.radius = FLASH_DISTANCE_FROM_VIEWER * tan (DEG2RAD (cg.refdef.fov_x * 0.5f));

	theFxHelper.AddFxToScene( &mRefEnt );

	drawnFx++;
}
