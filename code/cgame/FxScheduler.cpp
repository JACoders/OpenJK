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

#if !defined(GHOUL2_SHARED_H_INC)
	#include "../game/ghoul2_shared.h"	//for CGhoul2Info_v
#endif

#if !defined(G2_H_INC)
	#include "../ghoul2/G2.h"
#endif

#if !defined(__Q_SHARED_H)
	#include "../qcommon/q_shared.h"
#endif

#include <cmath>


CFxScheduler	theFxScheduler;

// don't even ask,. it's to do with loadsave...
//
std::vector < sstring_t > g_vstrEffectsNeededPerSlot;
SLoopedEffect	gLoopedEffectArray[MAX_LOOPED_FX];	// must be in sync with CFxScheduler::mLoopedEffectArray
void CFxScheduler::FX_CopeWithAnyLoadedSaveGames(void)
{
	if ( !g_vstrEffectsNeededPerSlot.empty() )
	{
		memcpy( mLoopedEffectArray, gLoopedEffectArray, sizeof(mLoopedEffectArray) );
		assert( g_vstrEffectsNeededPerSlot.size() == MAX_LOOPED_FX );

		for (size_t iFX = 0; iFX < g_vstrEffectsNeededPerSlot.size(); iFX++)
		{
			const char *psFX_Filename = g_vstrEffectsNeededPerSlot[iFX].c_str();
			if ( psFX_Filename[0] )
			{
				// register it...
				//
				mLoopedEffectArray[ iFX ].mId = RegisterEffect( psFX_Filename );
				//
				// cope with any relative stop time...
				//
				if ( mLoopedEffectArray[ iFX ].mLoopStopTime )
				{
					 mLoopedEffectArray[ iFX ].mLoopStopTime -= mLoopedEffectArray[ iFX ].mNextTime;
				} 
				//
				// and finally reset the time to be the newly-zeroed game time...
				//
				mLoopedEffectArray[ iFX ].mNextTime = 0;	// otherwise it won't process until game time catches up
			}
			else
			{
				mLoopedEffectArray[ iFX ].mId = 0;
			}
		}

		g_vstrEffectsNeededPerSlot.clear();
	}
}

void FX_CopeWithAnyLoadedSaveGames(void)
{
	theFxScheduler.FX_CopeWithAnyLoadedSaveGames();
}

// for loadsave...
//
void FX_Read( void )
{
	theFxScheduler.LoadSave_Read();
}

// for loadsave...
//
void FX_Write( void )
{
	theFxScheduler.LoadSave_Write();
}

void CFxScheduler::LoadSave_Read()
{		
	Clean();	// need to get rid of old pre-cache handles, or it thinks it has some older effects when it doesn't	
	g_vstrEffectsNeededPerSlot.clear();	// jic
	gi.ReadFromSaveGame(INT_ID('F','X','L','E'), (void *) &gLoopedEffectArray, sizeof(gLoopedEffectArray), NULL);
	//
	// now read in and re-register the effects we need for those structs...
	//
	for (int iFX = 0; iFX < MAX_LOOPED_FX; iFX++)
	{
		char sFX_Filename[MAX_QPATH];
		gi.ReadFromSaveGame(INT_ID('F','X','F','N'), sFX_Filename, sizeof(sFX_Filename), NULL);
		g_vstrEffectsNeededPerSlot.push_back( sFX_Filename );
	}
}

void CFxScheduler::LoadSave_Write()
{
	// bsave the data we need...
	//
	gi.AppendToSaveGame(INT_ID('F','X','L','E'), mLoopedEffectArray, sizeof(mLoopedEffectArray));
	//
	// then cope with the fact that the mID field in each struct of the array we've just saved will not 
	//	necessarily point at the same thing when reloading, so save out the actual fx filename strings they
	//	need for re-registration...
	//
	// since this is only for savegames, and I've got < 2 hours to finish this and test it I'm going to be lazy
	//	with the ondisk data... (besides, the RLE compression will kill most of this anyway)
	//
	for (int iFX = 0; iFX < MAX_LOOPED_FX; iFX++)
	{
		char sFX_Filename[MAX_QPATH];
		memset(sFX_Filename,0,sizeof(sFX_Filename));	// instead of "sFX_Filename[0]=0;" so RLE will squash whole array to nothing, not just stop at '\0' then have old crap after it to compress

		int &iID = mLoopedEffectArray[ iFX ].mId;
		if ( iID )
		{
			// now we need to look up what string this represents, unfortunately the existing
			//	lookup table is backwards (keywise) for our needs, so parse the whole thing...
			//
			for (TEffectID::iterator it = mEffectIDs.begin(); it != mEffectIDs.end(); ++it)
			{
				if ( (*it).second == iID )
				{
					Q_strncpyz( sFX_Filename, (*it).first.c_str(), sizeof(sFX_Filename) );
					break;
				}
			}
		}

		// write out this string...
		//
		gi.AppendToSaveGame(INT_ID('F','X','F','N'), sFX_Filename, sizeof(sFX_Filename));
	}
}


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
	memset( &mLoopedEffectArray, 0, sizeof( mLoopedEffectArray ));
}

int CFxScheduler::ScheduleLoopedEffect( int id, int boltInfo, bool isPortal, int iLoopTime, bool isRelative )
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
//			theFxHelper.Print( "CFxScheduler::ScheduleLoopedEffect- updating %s\n", mEffectTemplates[id].mEffectName);
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
	COM_StripExtension( file, sfile, sizeof(sfile) );
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
#ifdef _DEBUG_FX
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
			if ( cg_entities[entNum].gent->inuse )
			{// only play the looped effect when the ent is still inUse....
				PlayEffect( mLoopedEffectArray[i].mId, cg_entities[entNum].lerpOrigin, 0, mLoopedEffectArray[i].mBoltInfo, -1, mLoopedEffectArray[i].mPortalEffect, false,  mLoopedEffectArray[i].mIsRelative );	//very important to send FALSE looptime to not recursively add me!
				mLoopedEffectArray[i].mNextTime = theFxHelper.mTime + mEffectTemplates[mLoopedEffectArray[i].mId].mRepeatDelay;
			}
			else
			{
				theFxHelper.Print( "CFxScheduler::AddLoopedEffects- entity was removed without stopping any looping fx it owned." );
				memset( &mLoopedEffectArray[i], 0, sizeof(mLoopedEffectArray[i]) );
				continue;
			}
			if ( mLoopedEffectArray[i].mLoopStopTime && mLoopedEffectArray[i].mLoopStopTime < theFxHelper.mTime )	//time's up
			{//kill this entry
				memset( &mLoopedEffectArray[i], 0, sizeof(mLoopedEffectArray[i]) );
			}
		}
	}

}

//-----------------------------------------------------------
void SEffectTemplate::operator=(const SEffectTemplate &that)
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
	char			*data;
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
		Com_sprintf( temp, sizeof(temp), "%s/%s.efx", FX_FILE_PATH, sfile );
		pfile = temp;
	}

	len = theFxHelper.OpenFile( pfile, &fh, FS_READ );

	if ( len < 0 )
	{
		theFxHelper.Print( "RegisterEffect: failed to load: %s\n", pfile );
		return 0;
	}

	if (len == 0)
	{
		theFxHelper.Print( "RegisterEffect: INVALID file: %s\n", pfile );
		theFxHelper.CloseFile( fh );
		return 0;
	}

	// Allocate enough space to hold the file
	// This should be flagged temp, but it seems ok as is.
	data = new char[len+1];

	// Get the goods and ensure Null termination
	theFxHelper.ReadFile( data, len, fh );
	data[len] = '\0';
	bufParse = data;

	// Let the generic parser process the whole file
	parser.Parse( &bufParse );

	theFxHelper.CloseFile( fh );

	// Delete our temp copy of the file
	delete [] data;

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
void CFxScheduler::PlayEffect( int id, vec3_t origin, bool isPortal )
{
	vec3_t	axis[3];

	VectorSet( axis[0], 0, 0, 1 );
	VectorSet( axis[1], 1, 0, 0 );
	VectorSet( axis[2], 0, 1, 0 );

	PlayEffect( id, origin, axis, -1, -1, isPortal );
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
void CFxScheduler::PlayEffect( int id, vec3_t origin, vec3_t forward, bool isPortal )
{
	vec3_t	axis[3];

	// Take the forward vector and create two arbitrary but perpendicular vectors
	VectorCopy( forward, axis[0] );
	MakeNormalVectors( forward, axis[1], axis[2] );

	PlayEffect( id, origin, axis, -1, -1, isPortal );
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
void CFxScheduler::PlayEffect( const char *file, vec3_t origin, vec3_t axis[3], const int boltInfo, const int entNum, bool isPortal, int iLoopTime, bool isRelative )
{
	char	sfile[MAX_QPATH];

	// Get an extenstion stripped version of the file
	COM_StripExtension( file, sfile, sizeof(sfile) );

	// This is a horribly dumb thing to have to do, but QuakeIII might not have calc'd the lerpOrigin
	//	for the entity we may be trying to bolt onto.  We like having the correct origin, so we are
	//	forced to call this function....
	if ( entNum > -1 )
	{
		CG_CalcEntityLerpPositions( &cg_entities[entNum] );
	}

#ifndef FINAL_BUILD
	if ( mEffectIDs[sfile] == 0 )
	{
		theFxHelper.Print( "CFxScheduler::PlayEffect unregistered/non-existent effect: %s\n", sfile );		
	}
#endif

	PlayEffect( mEffectIDs[sfile], origin, axis, boltInfo, entNum, isPortal, iLoopTime, isRelative );
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
void CFxScheduler::PlayEffect( const char *file, int clientID, bool isPortal )
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
			if ( delay < 1 && !isPortal )
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
				sfx->mClientID	= clientID;

				if (isPortal)
				{
					sfx->mPortalEffect = true;
				}
				else
				{
					sfx->mPortalEffect = false;
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

bool gEffectsInPortal = false; //this is just because I don't want to have to add an mPortalEffect field to every actual effect.

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
	vec3_t	org,org2;
	int		flags = 0;

	// Origin calculations -- completely ignores most things
	//-------------------------------------
	VectorSet( org,  fx->mOrigin1X.GetVal(), fx->mOrigin1Y.GetVal(), fx->mOrigin1Z.GetVal() );
	VectorSet( org2, fx->mOrigin2X.GetVal(), fx->mOrigin2Y.GetVal(), fx->mOrigin2Z.GetVal() );

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
						fx->mMin, fx->mMax, fx->mElasticity.GetVal(),
						fx->mDeathFxHandles.GetHandle(), fx->mImpactFxHandles.GetHandle(),
						fx->mLife.GetVal(), fx->mMediaHandles.GetHandle(), flags );
		break;

	//---------
	case Line:
	//---------

		FX_AddLine( clientID, org, org2,
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

		if (gEffectsInPortal)
		{ //could orient this anyway for panning, but eh. It's going to appear to the player in the sky the same place no matter what, so just make it a local sound.
			theFxHelper.PlayLocalSound( fx->mMediaHandles.GetHandle(), CHAN_AUTO );
		}
		else
		{
			// bolted sounds actually play on the client....
			theFxHelper.PlaySound( NULL, clientID, CHAN_WEAPON, fx->mMediaHandles.GetHandle() );
		}
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
void CFxScheduler::PlayEffect( int id, vec3_t origin, vec3_t axis[3], const int boltInfo, const int entNum, bool isPortal, int iLoopTime, bool isRelative )
{
	SEffectTemplate			*fx;
	CPrimitiveTemplate		*prim;
	int						i = 0;
	int						count = 0, delay = 0;
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

	int	modelNum = 0, boltNum = -1;
	int	entityNum = entNum;

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
			ScheduleLoopedEffect(id, boltInfo, isPortal, iLoopTime, isRelative);
		}
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
			if ( delay < 1 && !forceScheduling && !isPortal )
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
				SScheduledEffect	*sfx;
				sfx = new SScheduledEffect;
				sfx->mStartTime = theFxHelper.mTime + delay;
				sfx->mpTemplate = prim;
				sfx->mClientID = -1;
				sfx->mIsRelative = isRelative;
				sfx->mEntNum = entityNum;	//ent if bolted, else -1 for none, or -2 for _Immersion client 0

				sfx->mPortalEffect = isPortal;

				if ( boltInfo == -1 )
				{
					if ( entNum == -1 )
					{
						// we aren't bolting, so make sure the spawn system knows this by putting -1's in these fields
						sfx->mBoltNum = -1;
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
						sfx->mModelNum = 0;

						AxisCopy( axis, sfx->mAxis );
					}
				}
				else
				{
					// we are bolting, so store the extra info
					sfx->mBoltNum = boltNum;
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
void CFxScheduler::PlayEffect( const char *file, vec3_t origin, bool isPortal )
{
	char	sfile[MAX_QPATH];

	// Get an extenstion stripped version of the file
	COM_StripExtension( file, sfile, sizeof(sfile) );

	PlayEffect( mEffectIDs[sfile], origin, isPortal );

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
void CFxScheduler::PlayEffect( const char *file, vec3_t origin, vec3_t forward, bool isPortal )
{
	char	sfile[MAX_QPATH];

	// Get an extenstion stripped version of the file
	COM_StripExtension( file, sfile, sizeof(sfile) );

	PlayEffect( mEffectIDs[sfile], origin, forward, isPortal );

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
//	boolean portal (true when adding effects to be drawn in the skyportal)
//
// Return:
//	none
//------------------------------------------------------
void CFxScheduler::AddScheduledEffects( bool portal )
{
	TScheduledEffect::iterator	itr, next;
	vec3_t						origin;
	vec3_t						axis[3];
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

	itr = mFxSchedule.begin();

	while ( itr != mFxSchedule.end() )
	{
		next = itr;
		next++;

		if (portal == (*itr)->mPortalEffect)
		{
			if ( *(*itr) <= theFxHelper.mTime )
			{
				if ( (*itr)->mClientID >= 0 )
				{
					CreateEffect( (*itr)->mpTemplate, (*itr)->mClientID, 
									theFxHelper.mTime - (*itr)->mStartTime );
				}
				else if ((*itr)->mBoltNum == -1)
				{// normal effect
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
				{	//bolted on effect				
					// do we need to go and re-get the bolt matrix again? Since it takes time lets try to do it only once
					if (((*itr)->mModelNum != oldModelNum) || ((*itr)->mEntNum != oldEntNum) || ((*itr)->mBoltNum != oldBoltIndex))
					{
						const centity_t &cent = cg_entities[(*itr)->mEntNum];
						if (cent.gent->ghoul2.IsValid())
						{
							if ((*itr)->mModelNum>=0&&(*itr)->mModelNum<cent.gent->ghoul2.size())
							{
								if (cent.gent->ghoul2[(*itr)->mModelNum].mModelindex>=0)
								{
									doesBoltExist = theFxHelper.GetOriginAxisFromBolt(cent, (*itr)->mModelNum, (*itr)->mBoltNum, origin, axis);
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
						if ((*itr)->mIsRelative )
						{
							CreateEffect( (*itr)->mpTemplate, 
										vec3_origin, axis, 
										0, (*itr)->mEntNum, (*itr)->mModelNum, (*itr)->mBoltNum );
						}
						else
						{
							CreateEffect( (*itr)->mpTemplate, 
										origin, axis, 
										theFxHelper.mTime - (*itr)->mStartTime );
						}
					}
				}

				// Get 'em out of there.
				delete *itr;
				mFxSchedule.erase(itr);
			}
		}

		itr = next;
	}

	// Add all active effects into the scene
	FX_Add(portal);

	gEffectsInPortal = false;
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
void CFxScheduler::CreateEffect( CPrimitiveTemplate *fx, const vec3_t origin, vec3_t axis[3], int lateTime, int clientID, int modelNum, int boltNum )
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
	if (clientID>=0 && modelNum>=0 && boltNum>=0)
	{//since you passed in these values, mark as relative to use them
		flags |= FX_RELATIVE;
	}

	if( fx->mSpawnFlags & FX_RAND_ROT_AROUND_FWD )
	{
		RotatePointAroundVector( ax[1], ax[0], axis[1], random()*360.0f );
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
			if ( fx->mSpawnFlags & FX_CHEAP_ORG2_CALC || flags & FX_RELATIVE )
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

		FX_AddParticle( clientID, org, vel, accel, fx->mGravity.GetVal(),
						fx->mSizeStart.GetVal(), fx->mSizeEnd.GetVal(), fx->mSizeParm.GetVal(),
						fx->mAlphaStart.GetVal(), fx->mAlphaEnd.GetVal(), fx->mAlphaParm.GetVal(),
						sRGB, eRGB, fx->mRGBParm.GetVal(),
						fx->mRotation.GetVal(), fx->mRotationDelta.GetVal(),
						fx->mMin, fx->mMax, fx->mElasticity.GetVal(), 
						fx->mDeathFxHandles.GetHandle(), fx->mImpactFxHandles.GetHandle(),
						fx->mLife.GetVal(), fx->mMediaHandles.GetHandle(), flags, modelNum, boltNum );
		break;

	//---------
	case Line:
	//---------

		FX_AddLine( clientID, org, org2, 
						fx->mSizeStart.GetVal(), fx->mSizeEnd.GetVal(), fx->mSizeParm.GetVal(),
						fx->mAlphaStart.GetVal(), fx->mAlphaEnd.GetVal(), fx->mAlphaParm.GetVal(),
						sRGB, eRGB, fx->mRGBParm.GetVal(),
						fx->mLife.GetVal(), fx->mMediaHandles.GetHandle(), fx->mImpactFxHandles.GetHandle(), flags, modelNum, boltNum );
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
						fx->mLife.GetVal(), fx->mMediaHandles.GetHandle(), flags, modelNum, boltNum );
		break;

	//----------------
	case Electricity:
	//----------------

		FX_AddElectricity( clientID, org, org2, 
						fx->mSizeStart.GetVal(), fx->mSizeEnd.GetVal(), fx->mSizeParm.GetVal(),
						fx->mAlphaStart.GetVal(), fx->mAlphaEnd.GetVal(), fx->mAlphaParm.GetVal(),
						sRGB, eRGB, fx->mRGBParm.GetVal(),
						fx->mElasticity.GetVal(), fx->mLife.GetVal(), fx->mMediaHandles.GetHandle(), flags, modelNum, boltNum );
		break;

	//---------
	case Cylinder:
	//---------

		FX_AddCylinder( clientID, org, ax[0],
						fx->mSizeStart.GetVal(), fx->mSizeEnd.GetVal(), fx->mSizeParm.GetVal(),
						fx->mSize2Start.GetVal(), fx->mSize2End.GetVal(), fx->mSize2Parm.GetVal(),
						fx->mLengthStart.GetVal(), fx->mLengthEnd.GetVal(), fx->mLengthParm.GetVal(),
						fx->mAlphaStart.GetVal(), fx->mAlphaEnd.GetVal(), fx->mAlphaParm.GetVal(),
						sRGB, eRGB, fx->mRGBParm.GetVal(),
						fx->mLife.GetVal(), fx->mMediaHandles.GetHandle(), flags, modelNum, boltNum );
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
						fx->mLife.GetVal(), emitterModel, flags );
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

		if (fx->mFlags & FX_GHOUL2_DECALS)
		{
			trace_t	tr;
			vec3_t	end;

			VectorMA(org, 64, ax[0], end);

			theFxHelper.G2Trace(&tr, org, NULL, NULL, end, ENTITYNUM_NONE, MASK_PLAYERSOLID);

			if (tr.entityNum < ENTITYNUM_WORLD &&
				g_entities[tr.entityNum].ghoul2.size())
			{
				gentity_t *ent = &g_entities[tr.entityNum];

				if ( ent != NULL )
				{
					vec3_t entOrg, hitDir;
					float entYaw;
					float firstModel = 0;
					if ( !(ent->s.eFlags&EF_NODRAW) )
					{//not drawn, no marks
						if ( ent->client )
						{
							VectorCopy( ent->client->ps.origin, entOrg );
						}
						else
						{
							VectorCopy( ent->currentOrigin, entOrg );
						}
						if ( ent->client )
						{
							entYaw = ent->client->ps.viewangles[YAW];
						}
						else
						{
							entYaw = ent->currentAngles[YAW];
						}
						//if ( VectorCompare( tr.plane.normal, vec3_origin ) )
						{//hunh, no plane?  Use trace dir
							VectorCopy( ax[0], hitDir );
						}
						/*
						else
						{
							VectorCopy( tr.plane.normal, hitDir );
						}
						*/

						CG_AddGhoul2Mark(fx->mMediaHandles.GetHandle(), fx->mSizeStart.GetVal(), tr.endpos, tr.plane.normal,
							tr.entityNum, entOrg, entYaw,
							ent->ghoul2, ent->s.modelScale, Q_irand(40000, 60000), firstModel);
					}
				}
			}
		}
		break;

	//-------------------
	case OrientedParticle:
	//-------------------

		FX_AddOrientedParticle( clientID, org, ax[0], vel, accel,
					fx->mSizeStart.GetVal(), fx->mSizeEnd.GetVal(), fx->mSizeParm.GetVal(),
					fx->mAlphaStart.GetVal(), fx->mAlphaEnd.GetVal(), fx->mAlphaParm.GetVal(),
					sRGB, eRGB, fx->mRGBParm.GetVal(),
					fx->mRotation.GetVal(), fx->mRotationDelta.GetVal(),
					fx->mMin, fx->mMax, fx->mElasticity.GetVal(),
					fx->mDeathFxHandles.GetHandle(), fx->mImpactFxHandles.GetHandle(),
					fx->mLife.GetVal(), fx->mMediaHandles.GetHandle(), flags, modelNum, boltNum );
		break;

	//---------
	case Sound:
	//---------
		if (gEffectsInPortal)
		{ //could orient this anyway for panning, but eh. It's going to appear to the player in the sky the same place no matter what, so just make it a local sound.
			theFxHelper.PlayLocalSound( fx->mMediaHandles.GetHandle(), CHAN_AUTO );
		}
		else if ( fx->mSpawnFlags & FX_SND_LESS_ATTENUATION )
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
