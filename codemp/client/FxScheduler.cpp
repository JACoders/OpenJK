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
#include "qcommon/q_shared.h"

#include <algorithm>
#include <cmath>
#include <string>

CFxScheduler	theFxScheduler;

//-----------------------------------------------------------
CMediaHandles &CMediaHandles::operator=(const CMediaHandles &that )
{
	mMediaList.clear();

	for ( size_t i = 0; i < that.mMediaList.size(); i++ )
	{
		mMediaList.push_back( that.mMediaList[i] );
	}
	return *this;
}

//------------------------------------------------------
CFxScheduler::CFxScheduler()
{
	mNextFree2DEffect = 0;
	memset( &mEffectTemplates, 0, sizeof( mEffectTemplates ));
	memset( &mLoopedEffectArray, 0, sizeof( mLoopedEffectArray ));
}

int CFxScheduler::ScheduleLoopedEffect( int id, int boltInfo, CGhoul2Info_v *ghoul2, bool isPortal, int iLoopTime, bool isRelative  )
{
	int i;

	assert(id);
	assert(boltInfo!=-1);

	for (i=0;i<MAX_LOOPED_FX;i++)	//see if it's already playing so we can just update it
	{
		if (mLoopedEffectArray[i].mId == id &&
			mLoopedEffectArray[i].mBoltInfo == boltInfo &&
			mLoopedEffectArray[i].mPortalEffect == isPortal
			)
		{
#ifdef _DEBUG
			theFxHelper.Print( "CFxScheduler::ScheduleLoopedEffect- updating %s\n", mEffectTemplates[id].mEffectName);
#endif
			break;
		}
	}

	if(i==MAX_LOOPED_FX)	//didn't find it existing, so find a free spot
	{
		for (i=0;i<MAX_LOOPED_FX;i++)
		{
			if (!mLoopedEffectArray[i].mId)
			{
				break;
			}
		}
	}

	if(i==MAX_LOOPED_FX)
	{//bad
		assert(i!=MAX_LOOPED_FX);
		theFxHelper.Print( "CFxScheduler::AddLoopedEffect- No Free Slots available for %d\n", mEffectTemplates[id].mEffectName);
		return -1;
	}
	mLoopedEffectArray[i].mId = id;
	mLoopedEffectArray[i].mBoltInfo = boltInfo;
	mLoopedEffectArray[i].mGhoul2  = ghoul2;
	mLoopedEffectArray[i].mPortalEffect = isPortal;
	mLoopedEffectArray[i].mIsRelative = isRelative;
	mLoopedEffectArray[i].mNextTime = theFxHelper.mTime + mEffectTemplates[id].mRepeatDelay ;
	mLoopedEffectArray[i].mLoopStopTime = (iLoopTime==1) ? 0 : theFxHelper.mTime + iLoopTime;
	return i;
}

void CFxScheduler::StopEffect( const char *file, int boltInfo, bool isPortal )
{
	int i;
	char	sfile[MAX_QPATH];

	// Get an extenstion stripped version of the file
	COM_StripExtension( file, sfile, sizeof( sfile ) );
	const int id = mEffectIDs[sfile];
#ifndef FINAL_BUILD
	if ( id == 0 )
	{
		theFxHelper.Print( "CFxScheduler::StopEffect- unregistered/non-existent effect: %s\n", sfile );
		return;
	}
#endif

	for (i=0;i<MAX_LOOPED_FX;i++)
	{
		if (mLoopedEffectArray[i].mId == id &&
			mLoopedEffectArray[i].mBoltInfo == boltInfo &&
			mLoopedEffectArray[i].mPortalEffect == isPortal
			)
		{
			memset( &mLoopedEffectArray[i], 0, sizeof(mLoopedEffectArray[i]) );
			return;
		}
	}
#ifdef _DEBUG
	theFxHelper.Print( "CFxScheduler::StopEffect- (%s) is not looping!\n", file);
#endif
}

void CFxScheduler::AddLoopedEffects()
{
	int i;

	for (i=0;i<MAX_LOOPED_FX;i++)
	{
		if (mLoopedEffectArray[i].mId && mLoopedEffectArray[i].mNextTime < theFxHelper.mTime )
		{
			const int entNum = ( mLoopedEffectArray[i].mBoltInfo >> ENTITY_SHIFT )	& ENTITY_AND;
			// Find out where the entity currently is
			TCGVectorData	*data = (TCGVectorData*)cl.mSharedMemory;

			data->mEntityNum = entNum;
			CGVM_GetLerpOrigin();

			PlayEffect( mLoopedEffectArray[i].mId, data->mPoint, 0, mLoopedEffectArray[i].mBoltInfo, mLoopedEffectArray[i].mGhoul2, -1, mLoopedEffectArray[i].mPortalEffect, false, mLoopedEffectArray[i].mIsRelative );	//very important to send FALSE to not recursively add me!
			mLoopedEffectArray[i].mNextTime = theFxHelper.mTime + mEffectTemplates[mLoopedEffectArray[i].mId].mRepeatDelay;
			if (mLoopedEffectArray[i].mLoopStopTime && mLoopedEffectArray[i].mLoopStopTime < theFxHelper.mTime)	//time's up
			{//kill this entry
				memset( &mLoopedEffectArray[i], 0, sizeof(mLoopedEffectArray[i]) );
			}
		}
	}

}

//-----------------------------------------------------------
SEffectTemplate &SEffectTemplate::operator=(const SEffectTemplate &that)
{
	mCopy = true;

	strcpy( mEffectName, that.mEffectName );

	mPrimitiveCount = that.mPrimitiveCount;

	for( int i = 0; i < mPrimitiveCount; i++ )
	{
		mPrimitives[i] = new CPrimitiveTemplate;
		*(mPrimitives[i]) = *(that.mPrimitives[i]);
		// Mark use as a copy so that we know that we should be chucked when used up
		mPrimitives[i]->mCopy = true;
	}
	return *this;
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
		++next;

		mScheduledEffectsPool.Free (*itr);
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
			std::string str;
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

	COM_StripExtension( file, sfile, sizeof( sfile ) );

	std::string s = sfile;
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);

	Com_DPrintf("Registering effect : %s\n", sfile);

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
	char			*bufParse = 0;

	// if our file doesn't have an extension, add one
	std::string finalFilename = file;
	std::string effectsSubstr = finalFilename.substr(0, 7);

	if (finalFilename.find('.') == std::string::npos)
	{
		// didn't find an extension so add one
		finalFilename += ".efx";
	}

	// kef - grr. this angers me. every filename everywhere should start from the base dir
	if (effectsSubstr.compare("effects") != 0)
	{
		//theFxHelper.Print("Hey!!! '%s' should be pathed from the base directory!!!\n", finalFilename.c_str());
		std::string strTemp = finalFilename;
		finalFilename = "effects/";
		finalFilename += strTemp;
	}

	len = theFxHelper.OpenFile( finalFilename.c_str(), &fh, FS_READ );

	/*
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
	*/


	if ( len < 0 )
	{
		theFxHelper.Print( "Effect file load failed: %s\n", finalFilename.c_str() );
		return 0;
	}

	if (len == 0)
	{
		theFxHelper.Print( "INVALID Effect file: %s\n", finalFilename.c_str() );
		theFxHelper.CloseFile( fh );
		return 0;
	}

	// If we'll overflow our buffer, bail out--not a particularly elegant solution
	if ((unsigned)len >= sizeof(data) - 1 )
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
	CGPValue			*pair;

	effect = GetNewEffectTemplate( &handle, file );

	if ( !handle || !effect )
	{
		// failure
		return 0;
	}
	if ((pair = base->GetPairs())!=0)
	{
		grpName = pair->GetName();
		if ( !Q_stricmp( grpName, "repeatDelay" ))
		{
			effect->mRepeatDelay = atoi(pair->GetTopValue());
		}
		else
		{//unknown

		}
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
				strcpy( effect->mEffectName, file );
			}

			effect->mInUse = true;
			effect->mRepeatDelay = 300;
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
	if ( fxHandle < 1 || fxHandle >= FX_MAX_EFFECTS)
	{
		// Didn't even request a valid effect to copy!!!
		theFxHelper.Print( "FxScheduler: Bad effect file copy request: id = %d\n", fxHandle );

		*newHandle = 0;
		return 0;
	}

	if (!mEffectTemplates[fxHandle].mInUse )
	{
		// Didn't even request a valid effect to copy!!!
		theFxHelper.Print( "FxScheduler: Bad effect file copy request: id %d not inuse\n", fxHandle );

		*newHandle = 0;
		return 0;
	}

#ifdef _DEBUG
	// never get a copy when time is frozen
	if ( fx_freeze->integer )
	{
		return 0;
	}
#endif

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


void	CFxScheduler::MaterialImpact(trace_t *tr, CEffect *effect)
{
/*	EMatImpactEffect matImpactEffect = effect->GetMatImpactFX();
	int impactParm = effect->GetMatImpactParm();

	if (matImpactEffect == MATIMPACTFX_NONE)
	{
		return;
	}
	else if (matImpactEffect == MATIMPACTFX_SHELLSOUND)
	{
		// only want to play this for the first impact
		effect->SetMatImpactFX(MATIMPACTFX_NONE);

		int	material = tr->surfaceFlags & MATERIAL_MASK;
		const char *ammoName = CWeaponSystem::GetAmmoName(impactParm);

		if(ammoName && materials[material].HasShellSound(ammoName))
		{
			theFxHelper.PlaySound( tr->endpos, ENTITYNUM_NONE, CHAN_AUTO, materials[material].GetShellSoundHandle(ammoName) );
		}
	}*/
}


//------------------------------------------------------
static void ReportPlayEffectError(int id)
{
	theFxHelper.Print( "CFxScheduler::PlayEffect called with invalid effect ID: %i\n", id );
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
/*void CFxScheduler::PlayEffect( int id, vec3_t origin, int vol, int rad )
{
	matrix3_t	axis;

	VectorSet( axis[0], 0, 0, 1 );
	VectorSet( axis[1], 1, 0, 0 );
	VectorSet( axis[2], 0, 1, 0 );

	PlayEffect( id, origin, axis, vol, rad );
}
*/
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
void CFxScheduler::PlayEffect( int id, vec3_t origin, vec3_t forward, int vol, int rad, bool isPortal )
{
	matrix3_t	axis;

	// Take the forward vector and create two arbitrary but perpendicular vectors
	VectorCopy( forward, axis[0] );
	MakeNormalVectors( forward, axis[1], axis[2] );

	PlayEffect( id, origin, axis, -1, 0, -1, vol, rad, isPortal );
}


//------------------------------------------------------
// PlayEffect
//	Handles scheduling an effect so all the components
//	happen at the specified time.  Uses the specified axis
//
// Input:
//	Effect file name, the origin, and axis.
//	Optional boltInfo (defaults to -1)
//  and iGhoul2 used by boltInfo
//
// Return:
//	none
//------------------------------------------------------
void CFxScheduler::PlayEffect( const char *file, vec3_t origin, matrix3_t axis, const int boltInfo, CGhoul2Info_v *ghoul2,
							  int fxParm /*-1*/, int vol, int rad, int iLoopTime, bool isRelative )
{
	char	sfile[MAX_QPATH];

	// Get an extenstion stripped version of the file
	COM_StripExtension( file, sfile, sizeof( sfile ) );

#ifndef FINAL_BUILD
	if ( mEffectIDs[sfile] == 0 )
	{
		theFxHelper.Print( "CFxScheduler::PlayEffect unregistered/non-existent effect: %s\n", sfile );
		return;
	}
#endif

	PlayEffect( mEffectIDs[sfile], origin, axis, boltInfo, ghoul2, fxParm, vol, rad, qfalse, iLoopTime, isRelative );
}

int	totalPrimitives = 0;
int	totalEffects = 0;

void GetRGB_Colors( CPrimitiveTemplate *fx, vec3_t outStartRGB, vec3_t outEndRGB )
{
	float	percent;

	if ( fx->mSpawnFlags & FX_RGB_COMPONENT_INTERP )
	{
		percent = flrand(0.0f, 1.0f);

		VectorSet( outStartRGB, fx->mRedStart.GetVal(percent), fx->mGreenStart.GetVal(percent), fx->mBlueStart.GetVal(percent) );
		VectorSet( outEndRGB, fx->mRedEnd.GetVal(percent), fx->mGreenEnd.GetVal(percent), fx->mBlueEnd.GetVal(percent) );
	}
	else
	{
		VectorSet( outStartRGB, fx->mRedStart.GetVal(), fx->mGreenStart.GetVal(), fx->mBlueStart.GetVal() );
		VectorSet( outEndRGB, fx->mRedEnd.GetVal(), fx->mGreenEnd.GetVal(), fx->mBlueEnd.GetVal() );
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
void CFxScheduler::PlayEffect( int id, vec3_t origin, matrix3_t axis, const int boltInfo, CGhoul2Info_v *ghoul2, int fxParm /*-1*/, int vol, int rad, bool isPortal/*false*/, int iLoopTime/*0*/,  bool isRelative )
{
	SEffectTemplate			*fx;
	CPrimitiveTemplate		*prim;
	int						i = 0;
	int						count = 0, delay = 0;
	float					factor = 0.0f, fxscale;
	bool					forceScheduling = false;

	if ( id < 1 || id >= FX_MAX_EFFECTS || !mEffectTemplates[id].mInUse )
	{
		// Now you've done it!
		ReportPlayEffectError(id);
		return;
	}

#ifdef _DEBUG
	// Don't bother scheduling the effect if the system is currently frozen
	if ( fx_freeze->integer )
	{
		return;
	}
#endif

	int						modelNum = 0, boltNum = -1;
	int						entityNum = -1;

	if ( boltInfo > 0 )
	{
		// extract the wraith ID from the bolt info
		modelNum	= ( boltInfo >> MODEL_SHIFT )	& MODEL_AND;
		boltNum		= ( boltInfo >> BOLT_SHIFT )	& BOLT_AND;
		entityNum	= ( boltInfo >> ENTITY_SHIFT )	& ENTITY_AND;

		// We always force ghoul bolted objects to be scheduled so that they don't play right away.
		forceScheduling = true;

		if (iLoopTime)//0 = not looping, 1 for infinite, else duration
		{//store off the id to reschedule every frame
			ScheduleLoopedEffect(id, boltInfo, ghoul2, !!isPortal, iLoopTime, isRelative);
		}
	}

	// Get the effect.
	fx = &mEffectTemplates[id];

#ifndef FINAL_BUILD
	if ( fx_debug->integer == 2 )
	{
		Com_Printf( "> %s\n", fx->mEffectName);
	}
#endif

	// Loop through the primitives and schedule each bit
	for ( i = 0; i < fx->mPrimitiveCount; i++ )
	{
		totalPrimitives++;
		prim = fx->mPrimitives[i];

		prim->mSoundRadius = rad;
		prim->mSoundVolume = vol;

		if ( prim->mCullRange )
		{
			if ( DistanceSquared( origin, theFxHelper.refdef->vieworg ) > prim->mCullRange ) // cullrange gets squared on load
			{
				// is too far away
				continue;
			}
		}

		// Scale the particles based on the countscale factor.  Never, ever scale the particles upwards, however.
		fxscale = fx_countScale->value;
		if (fxscale > 1.0)
		{
			fxscale = 1.0;
		}
		// Only use scalability if there is a range
		// Temp fix until I have time to reweight all the scalability files
		if(fabsf(prim->mSpawnCount.GetMax() - prim->mSpawnCount.GetMin()) > 1.0f)
		{
			count = Round(prim->mSpawnCount.GetVal() * fxscale);
		}
		else
		{
			count = Round(prim->mSpawnCount.GetVal());
		}
		// Make sure we have at least one particle after scaling
		if(prim->mSpawnCount.GetMin() >= 1.0f && count < 1)
		{
			count = 1;
		}

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
			totalEffects++;
			if ( prim->mSpawnFlags & FX_EVEN_DISTRIBUTION )
			{
				delay = t * factor;
			}
			else
			{
				delay = prim->mSpawnDelay.GetVal();
			}

			// if the delay is so small, we may as well just create this bit right now
			if ( delay < 1 && !forceScheduling && !isPortal )
			{
				if ( boltInfo == -1 && entityNum != -1 )
				{
					// Find out where the entity currently is
					TCGVectorData	*data = (TCGVectorData*)cl.mSharedMemory;

					data->mEntityNum = entityNum;
					CGVM_GetLerpOrigin();
					CreateEffect( prim, data->mPoint, axis, -delay, fxParm );
				}
				else
				{
					CreateEffect( prim, origin, axis, -delay, fxParm );
				}
			}
			else
			{
				SScheduledEffect		*sfx = mScheduledEffectsPool.Alloc();

				if ( sfx == NULL )
				{
					Com_Error (ERR_DROP, "ERROR: Failed to allocate EFX from memory pool.");
					return;
				}

				sfx->mStartTime = theFxHelper.mTime + delay;
				sfx->mpTemplate = prim;
				sfx->mIsRelative = isRelative;
				sfx->mPortalEffect = isPortal;

				if ( boltInfo == -1 )
				{
					sfx->ghoul2 = NULL;
					if ( entityNum == -1 )
					{
						// we aren't bolting, so make sure the spawn system knows this by putting -1's in these fields
						sfx->mBoltNum = -1;
						sfx->mEntNum = ENTITYNUM_NONE;
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
					sfx->ghoul2 = ghoul2;

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
/*
void CFxScheduler::PlayEffect( const char *file, vec3_t origin, int vol, int rad )
{
	char	sfile[MAX_QPATH];

	// Get an extenstion stripped version of the file
	COM_StripExtension( file, sfile, sizeof(sfile) );

	PlayEffect( mEffectIDs[sfile], origin, vol, rad );
}
*/
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
void CFxScheduler::PlayEffect( const char *file, vec3_t origin, vec3_t forward, int vol, int rad )
{
	char	sfile[MAX_QPATH];

	// Get an extenstion stripped version of the file
	COM_StripExtension( file, sfile, sizeof( sfile ) );

	PlayEffect( mEffectIDs[sfile], origin, forward, vol, rad );
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
bool gEffectsInPortal = qfalse; //this is just because I don't want to have to add an mPortalEffect field to every actual effect.

void CFxScheduler::AddScheduledEffects( bool portal )
{
	TScheduledEffect::iterator	itr, next;
	vec3_t						origin;
	matrix3_t					axis;
	int							oldEntNum = -1, oldBoltIndex = -1, oldModelNum = -1;
	qboolean					doesBoltExist  = qfalse;

	if (portal)
	{
		gEffectsInPortal = true;
	}
	else
	{
		AddLoopedEffects();
	}

	for ( itr = mFxSchedule.begin(); itr != mFxSchedule.end(); /* do nothing */ )
	{
		SScheduledEffect *effect = *itr;

		if (portal == effect->mPortalEffect && effect->mStartTime <= theFxHelper.mTime )
		{ //only render portal fx on the skyportal pass and vice versa
			if (effect->mBoltNum == -1)
			{// ok, are we spawning a bolt on effect or a normal one?
				if ( effect->mEntNum != ENTITYNUM_NONE )
				{
					// Find out where the entity currently is
					TCGVectorData	*data = (TCGVectorData*)cl.mSharedMemory;

					data->mEntityNum = effect->mEntNum;
					CGVM_GetLerpOrigin();
					CreateEffect( effect->mpTemplate,
								data->mPoint, effect->mAxis,
								theFxHelper.mTime - effect->mStartTime );
				}
				else
				{
					CreateEffect( effect->mpTemplate,
								effect->mOrigin, effect->mAxis,
								theFxHelper.mTime - effect->mStartTime );
				}
			}
			else
			{	//bolted on effect
				// do we need to go and re-get the bolt matrix again? Since it takes time lets try to do it only once
				if ((effect->mModelNum != oldModelNum) ||
					(effect->mEntNum != oldEntNum) ||
					(effect->mBoltNum != oldBoltIndex))
				{
					oldModelNum = effect->mModelNum;
					oldEntNum = effect->mEntNum;
					oldBoltIndex = effect->mBoltNum;

					doesBoltExist = theFxHelper.GetOriginAxisFromBolt(effect->ghoul2, effect->mEntNum, effect->mModelNum, effect->mBoltNum, origin, axis);
				}

				// only do this if we found the bolt
				if (doesBoltExist)
				{
					if (effect->mIsRelative )
					{
						CreateEffect( effect->mpTemplate,
									origin, axis, 0, -1,
									effect->ghoul2, effect->mEntNum, effect->mModelNum, effect->mBoltNum );
					}
					else
					{
						CreateEffect( effect->mpTemplate,
									origin, axis,
									theFxHelper.mTime - effect->mStartTime );
					}
				}
			}

			mScheduledEffectsPool.Free (effect);
			itr = mFxSchedule.erase(itr);
		}
		else
		{
			++itr;
		}
	}

	// Add all active effects into the scene
	FX_Add( !!portal );

	gEffectsInPortal = false;
}

bool CFxScheduler::Add2DEffect(float x, float y, float w, float h, vec4_t color, qhandle_t shaderHandle)
{
	// need some sort of scale here because the effect was created using world units, not pixels
	float		fxScale2D = 10.0f;

	if (mNextFree2DEffect < FX_MAX_2DEFFECTS)
	{
		m2DEffects[mNextFree2DEffect].mScreenX = x;
		m2DEffects[mNextFree2DEffect].mScreenY = y;
		m2DEffects[mNextFree2DEffect].mWidth = w*fxScale2D;
		m2DEffects[mNextFree2DEffect].mHeight = h*fxScale2D;
		VectorCopy4(color, m2DEffects[mNextFree2DEffect].mColor);
		m2DEffects[mNextFree2DEffect].mShaderHandle = shaderHandle;

		mNextFree2DEffect++;
		return true;
	}
	return false;
}

void CFxScheduler::Draw2DEffects(float screenXScale, float screenYScale)
{
	float	x = 0, y = 0, w = 0, h = 0;

	for (int i = 0; i < mNextFree2DEffect; i++)
	{
		x = m2DEffects[i].mScreenX;
		y = m2DEffects[i].mScreenY;
		w = m2DEffects[i].mWidth;
		h = m2DEffects[i].mHeight;

		x *= screenXScale;
		w *= screenXScale;
		y *= screenYScale;
		h *= screenYScale;

		//allow 2d effect coloring?
		re->DrawStretchPic(x - (w*0.5f), y - (h*0.5f), w, h, 0, 0, 1, 1, /*m2DEffects[i].mColor,*/ m2DEffects[i].mShaderHandle);
	}
	// now that all 2D effects have been drawn we can consider the entire array to be free
	mNextFree2DEffect = 0;
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
void CFxScheduler::CreateEffect( CPrimitiveTemplate *fx, const vec3_t origin, matrix3_t axis, int lateTime, int fxParm /*-1*/, CGhoul2Info_v *ghoul2, int entNum, int modelNum, int boltNum  )
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

	int flags = fx->mFlags;
	if (ghoul2 != NULL && modelNum>=0 && boltNum>=0)
	{//since you passed in these values, mark as relative to use them if it is supported
		switch( fx->mType )
		{
		case Particle:
		case Line:
		case Tail:
		case Electricity:
		case Cylinder:
		case Emitter:
		case OrientedParticle:
		case Light:
			flags |= FX_RELATIVE;
			break;
		case Decal:
		case FxRunner:
		case ScreenFlash:
			//not supported yet
		case Sound:
		case CameraShake:
			//does not work bolted
			break;
		default:
			break;
		}
	}

	if( fx->mSpawnFlags & FX_RAND_ROT_AROUND_FWD )
	{
		RotatePointAroundVector( ax[1], ax[0], axis[1], flrand(0.0f, 360.0f) );
		CrossProduct( ax[0], ax[1], ax[2] );
	}

	// Origin calculations
	//-------------------------------------
	if ( fx->mSpawnFlags & FX_CHEAP_ORG_CALC || flags & FX_RELATIVE  )
	{ // let's take the easy way out
		VectorSet( org, fx->mOrigin1X.GetVal(), fx->mOrigin1Y.GetVal(), fx->mOrigin1Z.GetVal() );
	}
	else
	{ // time for some extra work
		VectorScale( ax[0], fx->mOrigin1X.GetVal(), org );
		VectorMA( org, fx->mOrigin1Y.GetVal(), ax[1], org );
		VectorMA( org, fx->mOrigin1Z.GetVal(), ax[2], org );
	}

	// We always add our calculated offset to the passed in origin, unless relative!
	if( !(flags & FX_RELATIVE) )
	{
		VectorAdd( org, origin, org );
	}
	// Now, we may need to calc a point on a sphere/ellipsoid/cylinder/disk and add that to it
	//----------------------------------------------------------------
	if ( fx->mSpawnFlags & FX_ORG_ON_SPHERE )
	{
		float x, y;
		float width, height;

		x = DEG2RAD( flrand(0.0f, 360.0f) );
		y = DEG2RAD( flrand(0.0f, 180.0f) );

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
		VectorMA( pt, flrand(-1.0f, 1.0f) * 0.5f * fx->mHeight.GetVal(), ax[0], pt );
		RotatePointAroundVector( temp, ax[0], pt, flrand(0.0f, 360.0f) );

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

	if ( fx->mType == OrientedParticle )
	{//bolted oriented particles use origin2 as an angular rotation offset...
		if ( flags & FX_RELATIVE )
		{
			VectorSet( ax[0], fx->mOrigin2X.GetVal(), fx->mOrigin2Y.GetVal(), fx->mOrigin2Z.GetVal() );
		}
	}

	// There are only a few types that really use velocity and acceleration, so do extra work for those types
	//--------------------------------------------------------------------------------------------------------
	if ( fx->mType == Particle || fx->mType == OrientedParticle || fx->mType == Tail || fx->mType == Emitter )
	{
		// Velocity calculations
		//-------------------------------------
		if ( fx->mSpawnFlags & FX_VEL_IS_ABSOLUTE || flags & FX_RELATIVE )
		{
			VectorSet( vel, fx->mVelX.GetVal(), fx->mVelY.GetVal(), fx->mVelZ.GetVal() );
		}
		else
		{ // bah, do some extra work to coerce it
			VectorScale( ax[0], fx->mVelX.GetVal(), vel );
			VectorMA( vel, fx->mVelY.GetVal(), ax[1], vel );
			VectorMA( vel, fx->mVelZ.GetVal(), ax[2], vel );
		}

		//-------------------------------------
		if ( fx->mSpawnFlags & FX_AFFECTED_BY_WIND )
		{
/*rjr			vec3_t wind;

			// wind is affecting us, so modify our initial velocity.  ideally, we would update throughout our lives, but this is easier
			CL_GetWindVector( wind );
			VectorMA( vel, fx->mWindModifier.GetVal() * 0.01f, wind, vel );
			*/
		}

		// Acceleration calculations
		//-------------------------------------
		if ( fx->mSpawnFlags & FX_ACCEL_IS_ABSOLUTE || flags & FX_RELATIVE )
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
				if ( fx->mSpawnFlags & FX_CHEAP_ORG2_CALC || flags & FX_RELATIVE )
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

			theFxHelper.Trace( tr, org, NULL, NULL, temp, -1, MASK_SOLID );

			VectorCopy( tr.endpos, org2 );

			if ( fx->mSpawnFlags & FX_TRACE_IMPACT_FX )
			{
				PlayEffect( fx->mImpactFxHandles.GetHandle(), org2, tr.plane.normal );
			}
		}
		else
		{
			if ( fx->mSpawnFlags & FX_CHEAP_ORG2_CALC || flags & FX_RELATIVE )
			{
				VectorSet( org2, fx->mOrigin2X.GetVal(), fx->mOrigin2Y.GetVal(), fx->mOrigin2Z.GetVal() );
			}
			else
			{
				VectorScale( ax[0], fx->mOrigin2X.GetVal(), org2 );
				VectorMA( org2, fx->mOrigin2Y.GetVal(), ax[1], org2 );
				VectorMA( org2, fx->mOrigin2Z.GetVal(), ax[2], org2 );
			}
			if( !(flags & FX_RELATIVE) )
			{
				VectorAdd( org2, origin, org2 );
			}
		}
	} // end special org2 types

	// handle RGB color, but only for types that will use it
	//---------------------------------------------------------------------------
	if ( fx->mType != Sound && fx->mType != FxRunner && fx->mType != CameraShake )
	{
		GetRGB_Colors( fx, sRGB, eRGB );
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
						fx->mLife.GetVal(), fx->mMediaHandles.GetHandle(), flags, fx->mMatImpactFX, fxParm,
						ghoul2, entNum, modelNum, boltNum );
		break;

	//---------
	case Line:
	//---------

		FX_AddLine( org, org2,
						fx->mSizeStart.GetVal(), fx->mSizeEnd.GetVal(), fx->mSizeParm.GetVal(),
						fx->mAlphaStart.GetVal(), fx->mAlphaEnd.GetVal(), fx->mAlphaParm.GetVal(),
						sRGB, eRGB, fx->mRGBParm.GetVal(),
						fx->mLife.GetVal(), fx->mMediaHandles.GetHandle(), flags, fx->mMatImpactFX, fxParm,
						ghoul2, entNum, modelNum, boltNum);
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
						fx->mLife.GetVal(), fx->mMediaHandles.GetHandle(), flags, fx->mMatImpactFX, fxParm,
						ghoul2, entNum, modelNum, boltNum);
		break;

	//----------------
	case Electricity:
	//----------------

		FX_AddElectricity( org, org2,
						fx->mSizeStart.GetVal(), fx->mSizeEnd.GetVal(), fx->mSizeParm.GetVal(),
						fx->mAlphaStart.GetVal(), fx->mAlphaEnd.GetVal(), fx->mAlphaParm.GetVal(),
						sRGB, eRGB, fx->mRGBParm.GetVal(),
						fx->mElasticity.GetVal(), fx->mLife.GetVal(), fx->mMediaHandles.GetHandle(), flags,
						fx->mMatImpactFX, fxParm,
						ghoul2, entNum, modelNum, boltNum);
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
						fx->mLife.GetVal(), fx->mMediaHandles.GetHandle(), flags, fx->mMatImpactFX, fxParm,
						ghoul2, entNum, modelNum, boltNum,
						(qboolean)( fx->mSpawnFlags & FX_ORG2_FROM_TRACE ));
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
						fx->mLife.GetVal(), emitterModel, flags, fx->mMatImpactFX, fxParm );
		break;

	//---------
	case Decal:
	//---------

/* rjr
		// This function is a somewhat higher-cost function for projecting a decal mark onto a surface.
		// We shouldn't always need this, only for big hits or when a decal is very close-up.

		// If the impact size is greater than 6, don't even try to use cheap sprites.  Big marks need real decals.
		if (fx->mSizeStart.GetVal()<6)
		{
			vec3_t dest, normal;
			int findmarkret;

			findmarkret = CG_FindMark(org, ax[0], dest, normal);

			if (findmarkret)
			{	// Legal to put down a mark.

				if ( findmarkret == FINDMARK_CHEAP ||		// If we can't put down a decal OR if and distance > 200
						DistanceSquared( dest, cg.refdef.vieworg ) > 40000)
				{	// Use cheap oriented particle decals.
					vec3_t zerovec={0,0,0};

					FX_AddOrientedParticle( effectCloud, org, ax[0],
							zerovec,	// velocity
							zerovec,	// acceleration
							fx->mSizeStart.GetVal(), fx->mSizeStart.GetVal(), 0,	// size params
							fx->mAlphaStart.GetVal(), 0, 0.75,						// alpha params (start fading at 75% life)
							sRGB, sRGB, 0,											// rgb params
							fx->mRotation.GetVal(), 0,								// rotation delta
							zerovec,	// min
							zerovec,	// max
							0,			// bounce
							0,			// deathID
							0,			// impactID
							20000,		// lifetime
							fx->mMediaHandles.GetHandle(),
							(fx->mFlags&~FX_ALPHA_PARM_MASK)|FX_ALPHA_NONLINEAR,
							MATIMPACTFX_NONE, -1);
				}
				else
				{	// Use the expensive kind.
					color.rgba.r = sRGB[0] * 0xff;
					color.rgba.g = sRGB[1] * 0xff;
					color.rgba.b = sRGB[2] * 0xff;
					color.rgba.a = fx->mAlphaStart.GetVal() * 0xff;

					CG_ImpactMark( fx->mMediaHandles.GetHandle(), org, ax[0],
								fx->mRotation.GetVal(), color.c, 12000, 8000,
								true, fx->mSizeStart.GetVal(), false );
				}
			}
		}
		else
		{	// Use the expensive kind.
			color.rgba.r = sRGB[0] * 0xff;
			color.rgba.g = sRGB[1] * 0xff;
			color.rgba.b = sRGB[2] * 0xff;
			color.rgba.a = fx->mAlphaStart.GetVal() * 0xff;

			CG_ImpactMark( fx->mMediaHandles.GetHandle(), org, ax[0],
						fx->mRotation.GetVal(), color.c, 12000, 8000,
						true, fx->mSizeStart.GetVal(), false );
		} */

		theFxHelper.AddDecalToScene ( fx->mMediaHandles.GetHandle(), org, ax[0], fx->mRotation.GetVal(), sRGB[0], sRGB[1], sRGB[2], fx->mAlphaStart.GetVal(), qtrue, fx->mSizeStart.GetVal(), qfalse );

		if (fx->mFlags & FX_GHOUL2_DECALS)
		{
			theFxHelper.AddGhoul2Decal(fx->mMediaHandles.GetHandle(), org, ax[0], fx->mSizeStart.GetVal());
		}

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
					fx->mLife.GetVal(), fx->mMediaHandles.GetHandle(), flags, fx->mMatImpactFX, fxParm,
					ghoul2, entNum, modelNum, boltNum);
		break;

	//---------
	case Sound:
	//---------
		if (gEffectsInPortal)
		{ //could orient this anyway for panning, but eh. It's going to appear to the player in the sky the same place no matter what, so just make it a local sound.
			theFxHelper.PlayLocalSound( fx->mMediaHandles.GetHandle(), CHAN_AUTO );
		}
		else
		{
			theFxHelper.PlaySound( org, ENTITYNUM_NONE, CHAN_AUTO, fx->mMediaHandles.GetHandle(), fx->mSoundVolume, fx->mSoundRadius );
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
						fx->mLife.GetVal(), flags, fx->mMatImpactFX, fxParm,
						ghoul2, entNum, modelNum, boltNum);
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

		FX_AddFlash( org, fx->mSizeStart.GetVal(), fx->mSizeEnd.GetVal(), fx->mSizeParm.GetVal(),
					fx->mAlphaStart.GetVal(), fx->mAlphaEnd.GetVal(), fx->mAlphaParm.GetVal(),
					sRGB, eRGB, fx->mRGBParm.GetVal(),
					fx->mLife.GetVal(), fx->mMediaHandles.GetHandle(), flags, fx->mMatImpactFX, fxParm );
		break;

	default:
		assert(0);
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
// CreateEffect
//	Creates the fx_runner
//
// Input:
//	template used to build the effect, and the scheduled effect we are based off of
//
// Return:
//	none
//------------------------------------------------------
void CFxScheduler::CreateEffect( CPrimitiveTemplate *fx, SScheduledEffect *scheduledFx )
{
	int boltInfo;

	// annoying bit....we have to pack the values back into an int before calling playEffect since there isn't the ideal overload we can already use.
	boltInfo    =  (( scheduledFx->mModelNum & MODEL_AND  ) << MODEL_SHIFT  );
	boltInfo    |= (( scheduledFx->mBoltNum  & BOLT_AND   ) << BOLT_SHIFT   );
	boltInfo    |= (( scheduledFx->mEntNum   & ENTITY_AND ) << ENTITY_SHIFT );

	PlayEffect( fx->mPlayFxHandles.GetHandle(), scheduledFx->mOrigin, scheduledFx->mAxis, boltInfo );
}

