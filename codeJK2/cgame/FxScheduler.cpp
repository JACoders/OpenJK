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

#if !defined(GHOUL2_SHARED_H_INC)
	#include "../game/ghoul2_shared.h"	//for CGhoul2Info_v
#endif

#if !defined(G2_H_INC)
	#include "../../code/ghoul2/G2.h"
#endif

#if !defined(__Q_SHARED_H)
	#include "../game/q_shared.h"
#endif

#include "cg_media.h"

#include <cmath>


CFxScheduler	theFxScheduler;

//-----------------------------------------------------------
void CMediaHandles::operator=(const CMediaHandles &that )
{
	mMediaList.clear();

	for ( size_t i = 0; i < that.mMediaList.size(); i++ )
	{
		mMediaList.push_back( that.mMediaList[i] );
	}
}

//------------------------------------------------------
CFxScheduler::CFxScheduler()
{
	memset( &mEffectTemplates, 0, sizeof( mEffectTemplates ));
}

//-----------------------------------------------------------
void SEffectTemplate::operator=(const SEffectTemplate &that)
{
	mCopy = true;

	Q_strncpyz( mEffectName, that.mEffectName, sizeof(mEffectName) );

	mPrimitiveCount = that.mPrimitiveCount;

	for( int i = 0; i < mPrimitiveCount; i++ )
	{
		mPrimitives[i] = new CPrimitiveTemplate;
		*(mPrimitives[i]) = *(that.mPrimitives[i]);
		// Mark use as a copy so that we know that we should be chucked when used up
		mPrimitives[i]->mCopy = true;
	}
}

//------------------------------------------------------
// Clean
//	Free up any memory we've allocated so we aren't leaking memory
//
// Input:
//	Whether to clean everything or just stop the playing (active) effects
//
// Return:
//	None
//
//------------------------------------------------------
void CFxScheduler::Clean(bool bRemoveTemplates /*= true*/, int idToPreserve /*= 0*/)
{
	int								i, j;
	TScheduledEffect::iterator		itr, next;

	// Ditch any scheduled effects
	itr = mFxSchedule.begin();

	while ( itr != mFxSchedule.end() )
	{
		next = itr;
		next++;

		delete *itr;
		mFxSchedule.erase(itr);

		itr = next;
	}

	if (bRemoveTemplates)
	{
		// Ditch any effect templates
		for ( i = 1; i < FX_MAX_EFFECTS; i++ )
		{
			if ( i == idToPreserve)
			{
				continue;
			}

			if ( mEffectTemplates[i].mInUse )
			{
				// Ditch the primitives
				for (j = 0; j < mEffectTemplates[i].mPrimitiveCount; j++)
				{
					delete mEffectTemplates[i].mPrimitives[j];
				}
			}

			mEffectTemplates[i].mInUse = false;
		}

		if (idToPreserve == 0)
		{
			mEffectIDs.clear();
		}
		else
		{
			// Clear the effect names, but first get the name of the effect to preserve,
			// and restore it after clearing.
			fxString_t str;
			TEffectID::iterator iter;

			for (iter = mEffectIDs.begin(); iter != mEffectIDs.end(); ++iter)
			{
				if ((*iter).second == idToPreserve)
				{
					str = (*iter).first;
					break;
				}
			}

			mEffectIDs.clear();

			mEffectIDs[str] = idToPreserve;
		}
	}
}

//------------------------------------------------------
// RegisterEffect
//	Attempt to open the specified effect file, if
//	file read succeeds, parse the file.
//
// Input:
//	path or filename to open
//
// Return:
//	int handle to the effect
//------------------------------------------------------
int CFxScheduler::RegisterEffect( const char *file, bool bHasCorrectPath /*= false*/ )
{
	// Dealing with file names:
	// File names can come from two places - the editor, in which case we should use the given
	// path as is, and the effect file, in which case we should add the correct path and extension.
	// In either case we create a stripped file name to use for naming effects.
	//

	char sfile[MAX_QPATH];

	// Get an extension stripped version of the file
	if (bHasCorrectPath)
	{
		const char *last = file, *p = file;

		while (*p != '\0')
		{
			if ((*p == '/') || (*p == '\\'))
			{
				last = p + 1;
			}

			p++;
		}

		COM_StripExtension( last, sfile, sizeof(sfile) );
	}
	else
	{
		COM_StripExtension( file, sfile, sizeof(sfile) );
	}

	// see if the specified file is already registered.  If it is, just return the id of that file
	TEffectID::iterator itr;

	itr = mEffectIDs.find( sfile );

	if ( itr != mEffectIDs.end() )
	{
		return (*itr).second;
	}

	CGenericParser2	parser;
	int				len = 0;
	fileHandle_t	fh;
	char			data[65536];
	char			temp[MAX_QPATH];
	const char		*pfile;
	char			*bufParse = 0;

	if (bHasCorrectPath)
	{
		pfile = file;
	}
	else
	{
		// Add on our extension and prepend the file with the default path
		sprintf( temp, "%s/%s.efx", FX_FILE_PATH, sfile );
		pfile = temp;
	}

	len = theFxHelper.OpenFile( pfile, &fh, FS_READ );

	if ( len < 0 )
	{
		theFxHelper.Print( "Effect file load failed: %s\n", pfile );
		return 0;
	}

	if (len == 0)
	{
		theFxHelper.Print( "INVALID Effect file: %s\n", pfile );
		theFxHelper.CloseFile( fh );
		return 0;
	}

	// If we'll overflow our buffer, bail out--not a particularly elegant solution
	if (len >= (int)sizeof(data) - 1 )
	{
		theFxHelper.CloseFile( fh );
		return 0;
	}

	// Get the goods and ensure Null termination
	theFxHelper.ReadFile( data, len, fh );
	data[len] = '\0';
	bufParse = data;

	// Let the generic parser process the whole file
	parser.Parse( &bufParse );

	theFxHelper.CloseFile( fh );

	// Lets convert the effect file into something that we can work with
	return ParseEffect( sfile, parser.GetBaseParseGroup() );
}


//------------------------------------------------------
// ParseEffect
//	Starts at ground zero, using each group header to
//	determine which kind of effect we are working with.
//	Then we call the appropriate function to parse the
//	specified effect group.
//
// Input:
//	base group, essentially the whole files contents
//
// Return:
//	int handle of the effect
//------------------------------------------------------

struct primitiveType_s { const char *name; EPrimType type; } primitiveTypes[] = {
	{ "particle", Particle },
	{ "line", Line },
	{ "tail", Tail },
	{ "sound", Sound },
	{ "cylinder", Cylinder },
	{ "electricity", Electricity },
	{ "emitter", Emitter },
	{ "decal", Decal },
	{ "orientedparticle", OrientedParticle },
	{ "fxrunner", FxRunner },
	{ "light", Light },
	{ "cameraShake", CameraShake },
	{ "flash", ScreenFlash },
};
static const size_t numPrimitiveTypes = ARRAY_LEN( primitiveTypes );

int CFxScheduler::ParseEffect( const char *file, CGPGroup *base )
{
	CGPGroup			*primitiveGroup;
	CPrimitiveTemplate	*prim;
	const char			*grpName;
	SEffectTemplate		*effect = 0;
	EPrimType			type;
	int					handle;

	effect = GetNewEffectTemplate( &handle, file );

	if ( !handle || !effect )
	{
		// failure
		return 0;
	}

	primitiveGroup = base->GetSubGroups();

	while ( primitiveGroup )
	{
		grpName = primitiveGroup->GetName();

		type = None;
		for ( size_t i=0; i<numPrimitiveTypes; i++ ) {
			if ( !Q_stricmp( grpName, primitiveTypes[i].name ) ) {
				type = primitiveTypes[i].type;
				break;
			}
		}

		if ( type != None )
		{
			prim = new CPrimitiveTemplate;

			prim->mType = type;
			prim->ParsePrimitive( primitiveGroup );

			// Add our primitive template to the effect list
			AddPrimitiveToEffect( effect, prim );
		}

		primitiveGroup = (CGPGroup *)primitiveGroup->GetNext();
	}

	return handle;
}


//------------------------------------------------------
// AddPrimitiveToEffect
//	Takes a primitive and attaches it to the effect.
//
// Input:
//	Effect template that we tack the primitive on to
//	Primitive to add to the effect template
//
// Return:
//	None
//------------------------------------------------------
void CFxScheduler::AddPrimitiveToEffect( SEffectTemplate *fx, CPrimitiveTemplate *prim )
{
	int ct = fx->mPrimitiveCount;

	if ( ct >= FX_MAX_EFFECT_COMPONENTS )
	{
		theFxHelper.Print( "FxScheduler:  Error--too many primitives in an effect\n" );
	}
	else
	{
		fx->mPrimitives[ct] = prim;
		fx->mPrimitiveCount++;
	}
}

//------------------------------------------------------
// GetNewEffectTemplate
//	Finds an unused effect template and returns it to the
//	caller.
//
// Input:
//	pointer to an id that will be filled in,
//	file name-- should be NULL when requesting a copy
//
// Return:
//	the id of the added effect template
//------------------------------------------------------
SEffectTemplate *CFxScheduler::GetNewEffectTemplate( int *id, const char *file )
{
	SEffectTemplate *effect;

	// wanted zero to be a bogus effect ID, so we just skip it.
	for ( int i = 1; i < FX_MAX_EFFECTS; i++ )
	{
		effect = &mEffectTemplates[i];

		if ( !effect->mInUse )
		{
			*id = i;
			memset( effect, 0, sizeof( SEffectTemplate ));

			// If we are a copy, we really won't have a name that we care about saving for later
			if ( file )
			{
				mEffectIDs[file] = i;
				Q_strncpyz( effect->mEffectName, file, sizeof(effect->mEffectName) );
			}

			effect->mInUse = true;
			return effect;
		}
	}

	theFxHelper.Print( "FxScheduler:  Error--reached max effects\n" );
	*id = 0;
	return 0;
}

//------------------------------------------------------
// GetEffectCopy
//	Returns a copy of the desired effect so that it can
//	easily be modified run-time.
//
// Input:
//	file-- the name of the effect file that you want a copy of
//	newHandle-- will actually be the returned handle to the new effect
//				you have to hold onto this if you intend to call it again
//
// Return:
//	the pointer to the copy
//------------------------------------------------------
SEffectTemplate *CFxScheduler::GetEffectCopy( const char *file, int *newHandle )
{
	return ( GetEffectCopy( mEffectIDs[file], newHandle ) );
}

//------------------------------------------------------
// GetEffectCopy
//	Returns a copy of the desired effect so that it can
//	easily be modified run-time.
//
// Input:
//	fxHandle-- the handle to the effect that you want a copy of
//	newHandle-- will actually be the returned handle to the new effect
//				you have to hold onto this if you intend to call it again
//
// Return:
//	the pointer to the copy
//------------------------------------------------------
SEffectTemplate *CFxScheduler::GetEffectCopy( int fxHandle, int *newHandle )
{
	if ( fxHandle < 1 || fxHandle >= FX_MAX_EFFECTS || !mEffectTemplates[fxHandle].mInUse )
	{
		// Didn't even request a valid effect to copy!!!
		theFxHelper.Print( "FxScheduler: Bad effect file copy request\n" );

		*newHandle = 0;
		return 0;
	}

	// never get a copy when time is frozen
	if ( fx_freeze.integer )
	{
		return 0;
	}

	// Copies shouldn't have names, otherwise they could trash our stl map used for getting ID from name
	SEffectTemplate *copy = GetNewEffectTemplate( newHandle, NULL );

	if ( copy && *newHandle )
	{
		// do the effect copy and mark us as what we are
		*copy = mEffectTemplates[fxHandle];
		copy->mCopy = true;

		// the user had better hold onto this handle if they ever hope to call this effect.
		return copy;
	}

	// No space left to return an effect
	*newHandle = 0;
	return 0;
}

//------------------------------------------------------
// GetPrimitiveCopy
//	Helper function that returns a copy of the desired primitive
//
// Input:
//	fxHandle - the pointer to the effect copy you want to override
//	componentName - name of the component to find
//
// Return:
//	the pointer to the desired primitive
//------------------------------------------------------
CPrimitiveTemplate *CFxScheduler::GetPrimitiveCopy( SEffectTemplate *effectCopy, const char *componentName )
{
	if ( !effectCopy || !effectCopy->mInUse )
	{
		return NULL;
	}

	for ( int i = 0; i < effectCopy->mPrimitiveCount; i++ )
	{
		if ( !Q_stricmp( effectCopy->mPrimitives[i]->mName, componentName ))
		{
			// we found a match, so return it
			return effectCopy->mPrimitives[i];
		}
	}

	// bah, no good.
	return NULL;
}

//------------------------------------------------------
static void ReportPlayEffectError(int id)
{
#ifdef _DEBUG
		theFxHelper.Print( "CFxScheduler::PlayEffect called with invalid effect ID: %i\n", id );
#endif
}


//------------------------------------------------------
// PlayEffect
//	Handles scheduling an effect so all the components
//	happen at the specified time.  Applies a default up
//	axis.
//
// Input:
//	Effect file id and the origin
//
// Return:
//	none
//------------------------------------------------------
void CFxScheduler::PlayEffect( int id, vec3_t origin )
{
	vec3_t	axis[3];

	VectorSet( axis[0], 0, 0, 1 );
	VectorSet( axis[1], 1, 0, 0 );
	VectorSet( axis[2], 0, 1, 0 );

	PlayEffect( id, origin, axis );
}

//------------------------------------------------------
// PlayEffect
//	Handles scheduling an effect so all the components
//	happen at the specified time.  Takes a fwd vector
//	and builds a right and up vector
//
// Input:
//	Effect file id, the origin, and a fwd vector
//
// Return:
//	none
//------------------------------------------------------
void CFxScheduler::PlayEffect( int id, vec3_t origin, vec3_t forward )
{
	vec3_t	axis[3];

	// Take the forward vector and create two arbitrary but perpendicular vectors
	VectorCopy( forward, axis[0] );
	MakeNormalVectors( forward, axis[1], axis[2] );

	PlayEffect( id, origin, axis );
}

//------------------------------------------------------
// PlayEffect
//	Handles scheduling an effect so all the components
//	happen at the specified time.  Uses the specified axis
//
// Input:
//	Effect file name, the origin, and axis.
//	Optional boltInfo (defaults to -1)
//  Optional entity number to be used by a cheap entity origin bolt (defaults to -1)
//
// Return:
//	none
//------------------------------------------------------
void CFxScheduler::PlayEffect( const char *file, vec3_t origin, vec3_t axis[3], const int boltInfo, const int entNum )
{
	char	sfile[MAX_QPATH];

	// Get an extenstion stripped version of the file
	COM_StripExtension( file, sfile, sizeof(sfile) );

	// This is a horribly dumb thing to have to do, but QuakeIII might not have calc'd the lerpOrigin
	//	for the entity we may be trying to bolt onto.  We like having the correct origin, so we are
	//	forced to call this function....
	if ( entNum != -1 )
	{
		CG_CalcEntityLerpPositions( &cg_entities[entNum] );
	}

#ifndef FINAL_BUILD
	if ( mEffectIDs[sfile] == 0 )
	{
		theFxHelper.Print( "CFxScheduler::PlayEffect unregistered/non-existent effect: %s\n", sfile );
	}
#endif

	PlayEffect( mEffectIDs[sfile], origin, axis, boltInfo, entNum );
}

//------------------------------------------------------
// PlayEffect
//	Handles scheduling an effect so all the components
//	happen at the specified time.  Uses the specified axis
//
// Input:
//	Effect file name, the origin, and axis.
//	Optional boltInfo (defaults to -1)
//  Optional entity number to be used by a cheap entity origin bolt (defaults to -1)
//
// Return:
//	none
//------------------------------------------------------
void CFxScheduler::PlayEffect( const char *file, int clientID )
{
	char	sfile[MAX_QPATH];
	int		id;

	// Get an extenstion stripped version of the file
	COM_StripExtension( file, sfile, sizeof(sfile) );
	id = mEffectIDs[sfile];

#ifndef FINAL_BUILD
	if ( id == 0 )
	{
		theFxHelper.Print( "CFxScheduler::PlayEffect unregistered/non-existent effect: %s\n", file );
	}
#endif

	SEffectTemplate			*fx;
	CPrimitiveTemplate		*prim;
	int						i = 0;
	int						count = 0, delay = 0;
	SScheduledEffect		*sfx;
	float					factor = 0.0f;

	if ( id < 1 || id >= FX_MAX_EFFECTS || !mEffectTemplates[id].mInUse )
	{
		// Now you've done it!
		ReportPlayEffectError(id);
		return;
	}

	// Don't bother scheduling the effect if the system is currently frozen

	// Get the effect.
	fx = &mEffectTemplates[id];

	// Loop through the primitives and schedule each bit
	for ( i = 0; i < fx->mPrimitiveCount; i++ )
	{
		prim = fx->mPrimitives[i];

		count = prim->mSpawnCount.GetRoundedVal();

		if ( prim->mCopy )
		{
			// If we are a copy, we need to store a "how many references count" so that we
			//	can keep the primitive template around for the correct amount of time.
			prim->mRefCount = count;
		}

		if ( prim->mSpawnFlags & FX_EVEN_DISTRIBUTION )
		{
			factor = abs(prim->mSpawnDelay.GetMax() - prim->mSpawnDelay.GetMin()) / (float)count;
		}

		// Schedule the random number of bits
		for ( int t = 0; t < count; t++ )
		{
			if ( prim->mSpawnFlags & FX_EVEN_DISTRIBUTION )
			{
				delay = t * factor;
			}
			else
			{
				delay = prim->mSpawnDelay.GetVal();
			}

			// if the delay is so small, we may as well just create this bit right now
			if ( delay < 1 )
			{
				CreateEffect( prim, clientID, -delay );
			}
			else
			{
				// We have to create a new scheduled effect so that we can create it at a later point
				//	you should avoid this because it's much more expensive
				sfx = new SScheduledEffect;
				sfx->mStartTime = theFxHelper.mTime + delay;
				sfx->mpTemplate = prim;
				sfx->mClientID = clientID;

				mFxSchedule.push_front( sfx );
			}
		}
	}

	// We track effect templates and primitive templates separately.
	if ( fx->mCopy )
	{
		// We don't use dynamic memory allocation, so just mark us as dead
		fx->mInUse = false;
	}
}

//------------------------------------------------------
// CreateEffect
//	Creates the specified fx taking into account the
//	multitude of different ways it could be spawned.
//
// Input:
//	template used to build the effect, desired effect origin,
//	desired orientation and how late the effect is so that
//	it can be moved to the correct spot
//
// Return:
//	none
//------------------------------------------------------
void CFxScheduler::CreateEffect( CPrimitiveTemplate *fx, int clientID, int delay )
{
	vec3_t	sRGB, eRGB;
	vec3_t	vel, accel;
	vec3_t	org;
	int		flags = 0;

	// Origin calculations -- completely ignores most things
	//-------------------------------------
	VectorSet( org, fx->mOrigin1X.GetVal(), fx->mOrigin1Y.GetVal(), fx->mOrigin1Z.GetVal() );

	// handle RGB color
	if ( fx->mSpawnFlags & FX_RGB_COMPONENT_INTERP )
	{
		float perc = random();

		VectorSet( sRGB, fx->mRedStart.GetVal( perc ), fx->mGreenStart.GetVal( perc ), fx->mBlueStart.GetVal( perc ) );
		VectorSet( eRGB, fx->mRedEnd.GetVal( perc ), fx->mGreenEnd.GetVal( perc ), fx->mBlueEnd.GetVal( perc ) );
	}
	else
	{
		VectorSet( sRGB, fx->mRedStart.GetVal(), fx->mGreenStart.GetVal(), fx->mBlueStart.GetVal() );
		VectorSet( eRGB, fx->mRedEnd.GetVal(), fx->mGreenEnd.GetVal(), fx->mBlueEnd.GetVal() );
	}

	// NOTE: This completely disregards a few specialty flags.
	VectorSet( vel, fx->mVelX.GetVal( ), fx->mVelY.GetVal( ), fx->mVelZ.GetVal( ) );
	VectorSet( accel, fx->mAccelX.GetVal( ), fx->mAccelY.GetVal( ), fx->mAccelZ.GetVal( ) );

	// If depth hack ISN'T already on, then turn it on.  Otherwise, we treat a pre-existing depth_hack flag as NOT being depth_hack.
	//	This is done because muzzle flash fx files are shared amongst all shooters, but for the player we need to do depth hack in first person....
	if ( !( fx->mFlags & FX_DEPTH_HACK ) && !cg.renderingThirdPerson ) // hack!
	{
		flags = fx->mFlags | FX_RELATIVE | FX_DEPTH_HACK;
	}
	else
	{
		flags = (fx->mFlags | FX_RELATIVE) & ~FX_DEPTH_HACK;
	}

	// We only support particles for now
	//------------------------
	switch( fx->mType )
	{
	//---------
	case Particle:
	//---------

		FX_AddParticle( clientID, org, vel, accel, fx->mGravity.GetVal(),
					fx->mSizeStart.GetVal(), fx->mSizeEnd.GetVal(), fx->mSizeParm.GetVal(),
					fx->mAlphaStart.GetVal(), fx->mAlphaEnd.GetVal(), fx->mAlphaParm.GetVal(),
					sRGB, eRGB, fx->mRGBParm.GetVal(),
					fx->mRotation.GetVal(), fx->mRotationDelta.GetVal(),
					fx->mLife.GetVal(), fx->mMediaHandles.GetHandle(), flags );
		break;

	//---------
	case Line:
	//---------

		FX_AddLine( clientID, org,
						fx->mSizeStart.GetVal(), fx->mSizeEnd.GetVal(), fx->mSizeParm.GetVal(),
						fx->mAlphaStart.GetVal(), fx->mAlphaEnd.GetVal(), fx->mAlphaParm.GetVal(),
						sRGB, eRGB, fx->mRGBParm.GetVal(),
						fx->mLife.GetVal(), fx->mMediaHandles.GetHandle(), fx->mImpactFxHandles.GetHandle(), flags );
		break;

	//---------
	case Tail:
	//---------

		FX_AddTail( clientID, org, vel, accel,
						fx->mSizeStart.GetVal(), fx->mSizeEnd.GetVal(), fx->mSizeParm.GetVal(),
						fx->mLengthStart.GetVal(), fx->mLengthEnd.GetVal(), fx->mLengthParm.GetVal(),
						fx->mAlphaStart.GetVal(), fx->mAlphaEnd.GetVal(), fx->mAlphaParm.GetVal(),
						sRGB, eRGB, fx->mRGBParm.GetVal(),
						fx->mMin, fx->mMax, fx->mElasticity.GetVal(),
						fx->mDeathFxHandles.GetHandle(), fx->mImpactFxHandles.GetHandle(),
						fx->mLife.GetVal(), fx->mMediaHandles.GetHandle(), flags );
		break;


	//---------
	case Sound:
	//---------

		// bolted sounds actually play on the client....
		theFxHelper.PlaySound( NULL, clientID, CHAN_WEAPON, fx->mMediaHandles.GetHandle() );
		break;

	//---------
	case Light:
	//---------

		// don't much care if the light stays bolted...so just add it.
		if ( clientID >= 0 && clientID < ENTITYNUM_WORLD )
		{
			// ..um, ok.....
			centity_t *cent = &cg_entities[clientID];

			if ( cent && cent->gent && cent->gent->client )
			{
				FX_AddLight( cent->gent->client->renderInfo.muzzlePoint, fx->mSizeStart.GetVal(), fx->mSizeEnd.GetVal(), fx->mSizeParm.GetVal(),
						sRGB, eRGB, fx->mRGBParm.GetVal(),
						fx->mLife.GetVal(), fx->mFlags );
			}
		}
		break;

	//---------
	case CameraShake:
	//---------

		if ( clientID >= 0 && clientID < ENTITYNUM_WORLD )
		{
			// ..um, ok.....
			centity_t *cent = &cg_entities[clientID];

			if ( cent && cent->gent && cent->gent->client )
			{
				theFxHelper.CameraShake( cent->gent->currentOrigin, fx->mElasticity.GetVal(), fx->mRadius.GetVal(), fx->mLife.GetVal() );
			}
		}
		break;

	default:
		break;
	}

	// Track when we need to clean ourselves up if we are a copy
	if ( fx->mCopy )
	{
		fx->mRefCount--;

		if ( fx->mRefCount <= 0 )
		{
			delete fx;
		}
	}
}

//------------------------------------------------------
// PlayEffect
//	Handles scheduling an effect so all the components
//	happen at the specified time.  Uses the specified axis
//
// Input:
//	Effect id, the origin, and axis.
//	Optional boltInfo (defaults to -1)
//  Optional entity number to be used by a cheap entity origin bolt (defaults to -1)
//
// Return:
//	none
//------------------------------------------------------
void CFxScheduler::PlayEffect( int id, vec3_t origin, vec3_t axis[3], const int boltInfo, const int entNum )
{
	SEffectTemplate			*fx;
	CPrimitiveTemplate		*prim;
	int						i = 0;
	int						count = 0, delay = 0;
	SScheduledEffect		*sfx;
	float					factor = 0.0f;
	bool					forceScheduling = false;

	if ( id < 1 || id >= FX_MAX_EFFECTS || !mEffectTemplates[id].mInUse )
	{
		// Now you've done it!
		ReportPlayEffectError(id);
		return;
	}

	// Don't bother scheduling the effect if the system is currently frozen
	if ( fx_freeze.integer )
	{
		return;
	}

	int	modelNum = 0, boltNum = 0;
	int	entityNum = entNum;

	if ( boltInfo > 0 )
	{
		// extract the wraith ID from the bolt info
		modelNum	= ( boltInfo >> MODEL_SHIFT )	& MODEL_AND;
		boltNum		= ( boltInfo >> BOLT_SHIFT )	& BOLT_AND;
		entityNum	= ( boltInfo >> ENTITY_SHIFT )	& ENTITY_AND;

		// We always force ghoul bolted objects to be scheduled so that they don't play right away.
		forceScheduling = true;
	}

	// Get the effect.
	fx = &mEffectTemplates[id];

	// Loop through the primitives and schedule each bit
	for ( i = 0; i < fx->mPrimitiveCount; i++ )
	{
		prim = fx->mPrimitives[i];

		if ( prim->mCullRange )
		{
			if ( DistanceSquared( origin, cg.refdef.vieworg ) > prim->mCullRange )  // cull range has already been squared
			{
				// is too far away, so don't add this primitive group
				continue;
			}
		}

		count = prim->mSpawnCount.GetRoundedVal();

		if ( prim->mCopy )
		{
			// If we are a copy, we need to store a "how many references count" so that we
			//	can keep the primitive template around for the correct amount of time.
			prim->mRefCount = count;
		}

		if ( prim->mSpawnFlags & FX_EVEN_DISTRIBUTION )
		{
			factor = abs(prim->mSpawnDelay.GetMax() - prim->mSpawnDelay.GetMin()) / (float)count;
		}

		// Schedule the random number of bits
		for ( int t = 0; t < count; t++ )
		{
			if ( prim->mSpawnFlags & FX_EVEN_DISTRIBUTION )
			{
				delay = t * factor;
			}
			else
			{
				delay = prim->mSpawnDelay.GetVal();
			}

			// if the delay is so small, we may as well just create this bit right now
			if ( delay < 1 && !forceScheduling )
			{
				if ( boltInfo == -1 && entNum != -1 )
				{
					// Find out where the entity currently is
					CreateEffect( prim, cg_entities[entNum].lerpOrigin, axis, -delay );
				}
				else
				{
					CreateEffect( prim, origin, axis, -delay );
				}
			}
			else
			{
				// We have to create a new scheduled effect so that we can create it at a later point
				//	you should avoid this because it's much more expensive
				sfx = new SScheduledEffect;
				sfx->mStartTime = theFxHelper.mTime + delay;
				sfx->mpTemplate = prim;
				sfx->mClientID = -1;

				if ( boltInfo == -1 )
				{
					if ( entNum == -1 )
					{
						// we aren't bolting, so make sure the spawn system knows this by putting -1's in these fields
						sfx->mBoltNum = -1;
						sfx->mEntNum = -1;
						sfx->mModelNum = 0;

						if ( origin )
						{
							VectorCopy( origin, sfx->mOrigin );
						}
						else
						{
							VectorClear( sfx->mOrigin );
						}

						AxisCopy( axis, sfx->mAxis );
					}
					else
					{
						// we are doing bolting onto the origin of the entity, so use a cheaper method
						sfx->mBoltNum = -1;
						sfx->mEntNum = entityNum;
						sfx->mModelNum = 0;

						AxisCopy( axis, sfx->mAxis );
					}
				}
				else
				{
					// we are bolting, so store the extra info
					sfx->mBoltNum = boltNum;
					sfx->mEntNum = entityNum;
					sfx->mModelNum = modelNum;

					// Also, the ghoul bolt may not be around yet, so delay the creation one frame
					sfx->mStartTime++;
				}

				mFxSchedule.push_front( sfx );
			}
		}
	}

	// We track effect templates and primitive templates separately.
	if ( fx->mCopy )
	{
		// We don't use dynamic memory allocation, so just mark us as dead
		fx->mInUse = false;
	}
}

//------------------------------------------------------
// PlayEffect
//	Handles scheduling an effect so all the components
//	happen at the specified time.  Applies a default up
//	axis.
//
// Input:
//	Effect file name and the origin
//
// Return:
//	none
//------------------------------------------------------
void CFxScheduler::PlayEffect( const char *file, vec3_t origin )
{
	char	sfile[MAX_QPATH];

	// Get an extenstion stripped version of the file
	COM_StripExtension( file, sfile, sizeof(sfile) );

	PlayEffect( mEffectIDs[sfile], origin );

#ifndef FINAL_BUILD
	if ( mEffectIDs[sfile] == 0 )
	{
		theFxHelper.Print( "CFxScheduler::PlayEffect unregistered/non-existent effect: %s\n", file );
	}
#endif
}

//------------------------------------------------------
// PlayEffect
//	Handles scheduling an effect so all the components
//	happen at the specified time.  Takes a forward vector
//	and uses this to complete the axis field.
//
// Input:
//	Effect file name, the origin, and a forward vector
//
// Return:
//	none
//------------------------------------------------------
void CFxScheduler::PlayEffect( const char *file, vec3_t origin, vec3_t forward )
{
	char	sfile[MAX_QPATH];

	// Get an extenstion stripped version of the file
	COM_StripExtension( file, sfile, sizeof(sfile) );

	PlayEffect( mEffectIDs[sfile], origin, forward );

#ifndef FINAL_BUILD
	if ( mEffectIDs[sfile] == 0 )
	{
		theFxHelper.Print( "CFxScheduler::PlayEffect unregistered/non-existent effect: %s\n", file );
	}
#endif
}

//------------------------------------------------------
// AddScheduledEffects
//	Handles determining if a scheduled effect should
//	be created or not.  If it should it handles converting
//	the template effect into a real one.
//
// Input:
//	none
//
// Return:
//	none
//------------------------------------------------------
void CFxScheduler::AddScheduledEffects( void )
{
	TScheduledEffect::iterator	itr, next;
	vec3_t						origin;
	vec3_t						axis[3];
	int							oldEntNum = -1, oldBoltIndex = -1, oldModelNum = -1;
	qboolean					doesBoltExist  = false;

	itr = mFxSchedule.begin();

	while ( itr != mFxSchedule.end() )
	{
		next = itr;
		next++;

		if ( *(*itr) <= theFxHelper.mTime )
		{
			if ( (*itr)->mClientID >= 0 )
			{
				CreateEffect( (*itr)->mpTemplate, (*itr)->mClientID,
								theFxHelper.mTime - (*itr)->mStartTime );
			}
			else if ((*itr)->mBoltNum == -1)
			{// ok, are we spawning a bolt on effect or a normal one?
				if ( (*itr)->mEntNum != -1 )
				{
					// Find out where the entity currently is
					CreateEffect( (*itr)->mpTemplate,
								cg_entities[(*itr)->mEntNum].lerpOrigin, (*itr)->mAxis,
								theFxHelper.mTime - (*itr)->mStartTime );
				}
				else
				{
					CreateEffect( (*itr)->mpTemplate,
								(*itr)->mOrigin, (*itr)->mAxis,
								theFxHelper.mTime - (*itr)->mStartTime );
				}
			}
			else
			{

				mdxaBone_t 		boltMatrix;

				doesBoltExist=qfalse;
				// do we need to go and re-get the bolt matrix again? Since it takes time lets try and do it only once
				if (((*itr)->mModelNum != oldModelNum) || ((*itr)->mEntNum != oldEntNum) || ((*itr)->mBoltNum != oldBoltIndex))
				{
					if (cg_entities[(*itr)->mEntNum].gent->ghoul2.IsValid())
					{
						if ((*itr)->mModelNum>=0&&(*itr)->mModelNum<cg_entities[(*itr)->mEntNum].gent->ghoul2.size())
						{
							if (cg_entities[(*itr)->mEntNum].gent->ghoul2[(*itr)->mModelNum].mModelindex>=0)
							{

								// go away and get me the bolt position for this frame please
								doesBoltExist = gi.G2API_GetBoltMatrix(cg_entities[(*itr)->mEntNum].gent->ghoul2, (*itr)->mModelNum, (*itr)->mBoltNum, &boltMatrix, cg_entities[(*itr)->mEntNum].lerpAngles, cg_entities[(*itr)->mEntNum].lerpOrigin, cg.time, cgs.model_draw, cg_entities[(*itr)->mEntNum].currentState.modelScale);
								// set up the axis and origin we need for the actual effect spawning
	   							origin[0] = boltMatrix.matrix[0][3];
								origin[1] = boltMatrix.matrix[1][3];
								origin[2] = boltMatrix.matrix[2][3];

								axis[0][0] = boltMatrix.matrix[0][0];
								axis[0][1] = boltMatrix.matrix[1][0];
								axis[0][2] = boltMatrix.matrix[2][0];

								axis[1][0] = boltMatrix.matrix[0][1];
								axis[1][1] = boltMatrix.matrix[1][1];
								axis[1][2] = boltMatrix.matrix[2][1];

								axis[2][0] = boltMatrix.matrix[0][2];
								axis[2][1] = boltMatrix.matrix[1][2];
								axis[2][2] = boltMatrix.matrix[2][2];
							}
						}
					}

					oldModelNum = (*itr)->mModelNum;
					oldEntNum = (*itr)->mEntNum;
					oldBoltIndex = (*itr)->mBoltNum;
				}

				// only do this if we found the bolt
				if (doesBoltExist)
				{
					CreateEffect( (*itr)->mpTemplate,
									origin, axis,
									theFxHelper.mTime - (*itr)->mStartTime );
				}
			}

			// Get 'em out of there.
			delete *itr;
			mFxSchedule.erase(itr);
		}

		itr = next;
	}

	// Add all active effects into the scene
	FX_Add();
}

//------------------------------------------------------
// CreateEffect
//	Creates the specified fx taking into account the
//	multitude of different ways it could be spawned.
//
// Input:
//	template used to build the effect, desired effect origin,
//	desired orientation and how late the effect is so that
//	it can be moved to the correct spot
//
// Return:
//	none
//------------------------------------------------------
void CFxScheduler::CreateEffect( CPrimitiveTemplate *fx, vec3_t origin, vec3_t axis[3], int lateTime )
{
	vec3_t	org, org2, temp,
				vel, accel,
				sRGB, eRGB,
				ang, angDelta,
				ax[3];
	trace_t	tr;
	int		emitterModel;

	// We may modify the axis, so make a work copy
	AxisCopy( axis, ax );

	if( fx->mSpawnFlags & FX_RAND_ROT_AROUND_FWD )
	{
		RotatePointAroundVector( ax[1], ax[0], axis[1], random()*360.0f );
		CrossProduct( ax[0], ax[1], ax[2] );
	}

	// Origin calculations
	//-------------------------------------
	if ( fx->mSpawnFlags & FX_CHEAP_ORG_CALC )
	{ // let's take the easy way out
		VectorSet( org, fx->mOrigin1X.GetVal(), fx->mOrigin1Y.GetVal(), fx->mOrigin1Z.GetVal() );
	}
	else
	{ // time for some extra work
		VectorScale( ax[0], fx->mOrigin1X.GetVal(), org );
		VectorMA( org, fx->mOrigin1Y.GetVal(), ax[1], org );
		VectorMA( org, fx->mOrigin1Z.GetVal(), ax[2], org );
	}

	// We always add our calculated offset to the passed in origin...
	VectorAdd( org, origin, org );

	// Now, we may need to calc a point on a sphere/ellipsoid/cylinder/disk and add that to it
	//----------------------------------------------------------------
	if ( fx->mSpawnFlags & FX_ORG_ON_SPHERE )
	{
		float x, y;
		float width, height;

		x = DEG2RAD( random() * 360.0f );
		y = DEG2RAD( random() * 180.0f );

		width = fx->mRadius.GetVal();
		height = fx->mHeight.GetVal();

		// calculate point on ellipse
		VectorSet( temp, sin(x) * width * sin(y), cos(x) * width * sin(y), cos(y) * height ); // sinx * siny, cosx * siny, cosy
		VectorAdd( org, temp, org );

		if ( fx->mSpawnFlags & FX_AXIS_FROM_SPHERE )
		{
			// well, we will now override the axis at the users request
			VectorNormalize2( temp, ax[0] );
			MakeNormalVectors( ax[0], ax[1], ax[2] );
		}
	}
	else if ( fx->mSpawnFlags & FX_ORG_ON_CYLINDER )
	{
		vec3_t	pt;

		// set up our point, then rotate around the current direction to.  Make unrotated cylinder centered around 0,0,0
		VectorScale( ax[1], fx->mRadius.GetVal(), pt );
		VectorMA( pt, crandom() * 0.5f * fx->mHeight.GetVal(), ax[0], pt );
		RotatePointAroundVector( temp, ax[0], pt, random() * 360.0f );

		VectorAdd( org, temp, org );

		if ( fx->mSpawnFlags & FX_AXIS_FROM_SPHERE )
		{
			vec3_t up={0,0,1};

			// well, we will now override the axis at the users request
			VectorNormalize2( temp, ax[0] );

			if ( ax[0][2] == 1.0f )
			{
				// readjust up
				VectorSet( up, 0, 1, 0 );
			}

			CrossProduct( up, ax[0], ax[1] );
			CrossProduct( ax[0], ax[1], ax[2] );
		}
	}

	// There are only a few types that really use velocity and acceleration, so do extra work for those types
	//--------------------------------------------------------------------------------------------------------
	if ( fx->mType == Particle || fx->mType == OrientedParticle || fx->mType == Tail || fx->mType == Emitter )
	{
		// Velocity calculations
		//-------------------------------------
		if ( fx->mSpawnFlags & FX_VEL_IS_ABSOLUTE )
		{
			VectorSet( vel, fx->mVelX.GetVal(), fx->mVelY.GetVal(), fx->mVelZ.GetVal() );
		}
		else
		{ // bah, do some extra work to coerce it
			VectorScale( ax[0], fx->mVelX.GetVal(), vel );
			VectorMA( vel, fx->mVelY.GetVal(), ax[1], vel );
			VectorMA( vel, fx->mVelZ.GetVal(), ax[2], vel );
		}

		// Acceleration calculations
		//-------------------------------------
		if ( fx->mSpawnFlags & FX_ACCEL_IS_ABSOLUTE )
		{
			VectorSet( accel, fx->mAccelX.GetVal(), fx->mAccelY.GetVal(), fx->mAccelZ.GetVal() );
		}
		else
		{
			VectorScale( ax[0], fx->mAccelX.GetVal(), accel );
			VectorMA( accel, fx->mAccelY.GetVal(), ax[1], accel );
			VectorMA( accel, fx->mAccelZ.GetVal(), ax[2], accel );
		}

		// Gravity is completely decoupled from acceleration since it is __always__ absolute
		// NOTE: I only effect Z ( up/down in the Quake world )
		accel[2] += fx->mGravity.GetVal();

		// There may be a lag between when the effect should be created and when it actually gets created.
		//	Since we know what the discrepancy is, we can attempt to compensate...
		if ( lateTime > 0 )
		{
			// Calc the time differences
			float ftime = lateTime * 0.001f;
			float time2 = ftime * ftime * 0.5f;

			VectorMA( vel, ftime, accel, vel );

			// Predict the new position
			for ( int i = 0 ; i < 3 ; i++ )
			{
				org[i] = org[i] + ftime * vel[i] + time2 * vel[i];
			}
		}
	} // end moving types

	// Line type primitives work with an origin2, so do the extra work for them
	//--------------------------------------------------------------------------
	if ( fx->mType == Line || fx->mType == Electricity )
	{
		// We may have to do a trace to find our endpoint
		if ( fx->mSpawnFlags & FX_ORG2_FROM_TRACE )
		{
			VectorMA( org, FX_MAX_TRACE_DIST, ax[0], temp );

			if ( fx->mSpawnFlags & FX_ORG2_IS_OFFSET )
			{ // add a random flair to the endpoint...note: org2 will have to be pretty large to affect this much
				// we also do this pre-trace as opposed to post trace since we may have to render an impact effect
				//	and we will want the normal at the exact endpos...
				if ( fx->mSpawnFlags & FX_CHEAP_ORG2_CALC )
				{
					VectorSet( org2, fx->mOrigin2X.GetVal(), fx->mOrigin2Y.GetVal(), fx->mOrigin2Z.GetVal() );
					VectorAdd( org2, temp, temp );
				}
				else
				{ // I can only imagine a few cases where you might want to do this...
					VectorMA( temp, fx->mOrigin2X.GetVal(), ax[0], temp );
					VectorMA( temp, fx->mOrigin2Y.GetVal(), ax[1], temp );
					VectorMA( temp, fx->mOrigin2Z.GetVal(), ax[2], temp );
				}
			}

			theFxHelper.Trace( &tr, org, NULL, NULL, temp, -1, CONTENTS_SOLID | CONTENTS_SHOTCLIP );//MASK_SHOT );

			if ( tr.startsolid || tr.allsolid )
			{
				VectorCopy( org, org2 ); // this is not a very good solution
			}
			else
			{
				VectorCopy( tr.endpos, org2 );
			}

			if ( fx->mSpawnFlags & FX_TRACE_IMPACT_FX )
			{
				PlayEffect( fx->mImpactFxHandles.GetHandle(), org2, tr.plane.normal );
			}
		}
		else
		{
			if ( fx->mSpawnFlags & FX_CHEAP_ORG2_CALC )
			{
				VectorSet( org2, fx->mOrigin2X.GetVal(), fx->mOrigin2Y.GetVal(), fx->mOrigin2Z.GetVal() );
			}
			else
			{
				VectorScale( ax[0], fx->mOrigin2X.GetVal(), org2 );
				VectorMA( org2, fx->mOrigin2Y.GetVal(), ax[1], org2 );
				VectorMA( org2, fx->mOrigin2Z.GetVal(), ax[2], org2 );

				VectorAdd( org2, origin, org2 );
			}

		}
	} // end special org2 types

	// handle RGB color, but only for types that will use it
	//---------------------------------------------------------------------------
	if ( fx->mType != Sound && fx->mType != FxRunner && fx->mType != CameraShake )
	{
		if ( fx->mSpawnFlags & FX_RGB_COMPONENT_INTERP )
		{
			float perc = random();

			VectorSet( sRGB, fx->mRedStart.GetVal( perc ), fx->mGreenStart.GetVal( perc ), fx->mBlueStart.GetVal( perc ) );
			VectorSet( eRGB, fx->mRedEnd.GetVal( perc ), fx->mGreenEnd.GetVal( perc ), fx->mBlueEnd.GetVal( perc ) );
		}
		else
		{
			VectorSet( sRGB, fx->mRedStart.GetVal(), fx->mGreenStart.GetVal(), fx->mBlueStart.GetVal() );
			VectorSet( eRGB, fx->mRedEnd.GetVal(), fx->mGreenEnd.GetVal(), fx->mBlueEnd.GetVal() );
		}
	}

	// Now create the appropriate effect entity
	//------------------------
	switch( fx->mType )
	{
	//---------
	case Particle:
	//---------

		FX_AddParticle( org, vel, accel,
						fx->mSizeStart.GetVal(), fx->mSizeEnd.GetVal(), fx->mSizeParm.GetVal(),
						fx->mAlphaStart.GetVal(), fx->mAlphaEnd.GetVal(), fx->mAlphaParm.GetVal(),
						sRGB, eRGB, fx->mRGBParm.GetVal(),
						fx->mRotation.GetVal(), fx->mRotationDelta.GetVal(),
						fx->mMin, fx->mMax, fx->mElasticity.GetVal(),
						fx->mDeathFxHandles.GetHandle(), fx->mImpactFxHandles.GetHandle(),
						fx->mLife.GetVal(), fx->mMediaHandles.GetHandle(), fx->mFlags );
		break;

	//---------
	case Line:
	//---------

		FX_AddLine( org, org2,
						fx->mSizeStart.GetVal(), fx->mSizeEnd.GetVal(), fx->mSizeParm.GetVal(),
						fx->mAlphaStart.GetVal(), fx->mAlphaEnd.GetVal(), fx->mAlphaParm.GetVal(),
						sRGB, eRGB, fx->mRGBParm.GetVal(),
						fx->mLife.GetVal(), fx->mMediaHandles.GetHandle(), fx->mFlags );
		break;

	//---------
	case Tail:
	//---------

		FX_AddTail( org, vel, accel,
						fx->mSizeStart.GetVal(), fx->mSizeEnd.GetVal(), fx->mSizeParm.GetVal(),
						fx->mLengthStart.GetVal(), fx->mLengthEnd.GetVal(), fx->mLengthParm.GetVal(),
						fx->mAlphaStart.GetVal(), fx->mAlphaEnd.GetVal(), fx->mAlphaParm.GetVal(),
						sRGB, eRGB, fx->mRGBParm.GetVal(),
						fx->mMin, fx->mMax, fx->mElasticity.GetVal(),
						fx->mDeathFxHandles.GetHandle(), fx->mImpactFxHandles.GetHandle(),
						fx->mLife.GetVal(), fx->mMediaHandles.GetHandle(), fx->mFlags );
		break;

	//----------------
	case Electricity:
	//----------------

		FX_AddElectricity( org, org2,
						fx->mSizeStart.GetVal(), fx->mSizeEnd.GetVal(), fx->mSizeParm.GetVal(),
						fx->mAlphaStart.GetVal(), fx->mAlphaEnd.GetVal(), fx->mAlphaParm.GetVal(),
						sRGB, eRGB, fx->mRGBParm.GetVal(),
						fx->mElasticity.GetVal(), fx->mLife.GetVal(), fx->mMediaHandles.GetHandle(), fx->mFlags );
		break;

	//---------
	case Cylinder:
	//---------

		FX_AddCylinder( org, ax[0],
						fx->mSizeStart.GetVal(), fx->mSizeEnd.GetVal(), fx->mSizeParm.GetVal(),
						fx->mSize2Start.GetVal(), fx->mSize2End.GetVal(), fx->mSize2Parm.GetVal(),
						fx->mLengthStart.GetVal(), fx->mLengthEnd.GetVal(), fx->mLengthParm.GetVal(),
						fx->mAlphaStart.GetVal(), fx->mAlphaEnd.GetVal(), fx->mAlphaParm.GetVal(),
						sRGB, eRGB, fx->mRGBParm.GetVal(),
						fx->mLife.GetVal(), fx->mMediaHandles.GetHandle(), fx->mFlags );
		break;

	//---------
	case Emitter:
	//---------

		// for chunk angles, you don't really need much control over the end result...you just want variation..
		VectorSet( ang,
					fx->mAngle1.GetVal(),
					fx->mAngle2.GetVal(),
					fx->mAngle3.GetVal() );

		vectoangles( ax[0], temp );
		VectorAdd( ang, temp, ang );

		VectorSet( angDelta,
					fx->mAngle1Delta.GetVal(),
					fx->mAngle2Delta.GetVal(),
					fx->mAngle3Delta.GetVal() );

		emitterModel = fx->mMediaHandles.GetHandle();

		FX_AddEmitter( org, vel, accel,
						fx->mSizeStart.GetVal(), fx->mSizeEnd.GetVal(), fx->mSizeParm.GetVal(),
						fx->mAlphaStart.GetVal(), fx->mAlphaEnd.GetVal(), fx->mAlphaParm.GetVal(),
						sRGB, eRGB, fx->mRGBParm.GetVal(),
						ang, angDelta,
						fx->mMin, fx->mMax, fx->mElasticity.GetVal(),
						fx->mDeathFxHandles.GetHandle(), fx->mImpactFxHandles.GetHandle(),
						fx->mEmitterFxHandles.GetHandle(),
						fx->mDensity.GetVal(), fx->mVariance.GetVal(),
						fx->mLife.GetVal(), emitterModel, fx->mFlags );
		break;

	//---------
	case Decal:
	//---------

		// I'm calling this function ( at least for now ) because it handles projecting
		//	the decal mark onto the surfaces properly.  This is especially important for large marks.
		// The downside is that it's much less flexible....
		CG_ImpactMark( fx->mMediaHandles.GetHandle(), org, ax[0], fx->mRotation.GetVal(),
			sRGB[0], sRGB[1], sRGB[2], fx->mAlphaStart.GetVal(),
			qtrue, fx->mSizeStart.GetVal(), qfalse );
		break;

	//-------------------
	case OrientedParticle:
	//-------------------

		FX_AddOrientedParticle( org, ax[0], vel, accel,
					fx->mSizeStart.GetVal(), fx->mSizeEnd.GetVal(), fx->mSizeParm.GetVal(),
					fx->mAlphaStart.GetVal(), fx->mAlphaEnd.GetVal(), fx->mAlphaParm.GetVal(),
					sRGB, eRGB, fx->mRGBParm.GetVal(),
					fx->mRotation.GetVal(), fx->mRotationDelta.GetVal(),
					fx->mMin, fx->mMax, fx->mElasticity.GetVal(),
					fx->mDeathFxHandles.GetHandle(), fx->mImpactFxHandles.GetHandle(),
					fx->mLife.GetVal(), fx->mMediaHandles.GetHandle(), fx->mFlags );
		break;

	//---------
	case Sound:
	//---------

		if ( fx->mSpawnFlags & FX_SND_LESS_ATTENUATION )
		{
			theFxHelper.PlaySound( org, ENTITYNUM_NONE, CHAN_LESS_ATTEN, fx->mMediaHandles.GetHandle() );
		}
		else
		{
			theFxHelper.PlaySound( org, ENTITYNUM_NONE, CHAN_AUTO, fx->mMediaHandles.GetHandle() );
		}
		break;

	//---------
	case FxRunner:
	//---------

		PlayEffect( fx->mPlayFxHandles.GetHandle(), org, ax );
		break;

	//---------
	case Light:
	//---------

		FX_AddLight( org, fx->mSizeStart.GetVal(), fx->mSizeEnd.GetVal(), fx->mSizeParm.GetVal(),
						sRGB, eRGB, fx->mRGBParm.GetVal(),
						fx->mLife.GetVal(), fx->mFlags );
		break;

	//---------
	case CameraShake:
	//---------
		// It calculates how intense the shake should be based on how close you are to the origin you pass in here
		//	elasticity is actually the intensity...radius is the distance in which the shake will have some effect
		//	life is how long the effect lasts.
		theFxHelper.CameraShake( org, fx->mElasticity.GetVal(), fx->mRadius.GetVal(), fx->mLife.GetVal() );
		break;

	//--------------
	case ScreenFlash:
	//--------------

		FX_AddFlash( org,
					sRGB, eRGB, fx->mRGBParm.GetVal(),
					fx->mLife.GetVal(), fx->mMediaHandles.GetHandle(), fx->mFlags );
		break;

	default:
		break;
	}

	// Track when we need to clean ourselves up if we are a copy
	if ( fx->mCopy )
	{
		fx->mRefCount--;

		if ( fx->mRefCount <= 0 )
		{
			delete fx;
		}
	}
}
