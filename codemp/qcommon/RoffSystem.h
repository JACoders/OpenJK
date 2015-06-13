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

#include "qcommon/q_shared.h"	//needs to be in here for entityState_t
#include "server/server.h"

#include <vector>
#include <map>

// ROFF Defines
//-------------------
#define ROFF_VERSION				1
#define ROFF_NEW_VERSION			2
#define ROFF_STRING					"ROFF"
#define ROFF_SAMPLE_RATE			10	// 10hz
#define ROFF_AUTO_FIX_BAD_ANGLES	// exporter can mess up angles,
									//	defining this attempts to detect and fix these problems


// The CROFFSystem object provides all of the functionality of ROFF
//	caching, playback, and clean-up, plus some useful debug features.
//--------------------------------------
class CROFFSystem
//--------------------------------------
{
private:
//------

	// forward declarations
	class			CROFF;
	struct			SROFFEntity;

	typedef	std::map	<int, CROFF *> TROFFList;
	typedef std::vector	<SROFFEntity *> TROFFEntList;

	TROFFList		mROFFList;				// List of cached roffs
	int				mID;					// unique ID generator for new roff objects

	TROFFEntList	mROFFEntList;			// List of roffing entities

	// ROFF Header file definition, nothing else needs to see this
	typedef struct tROFFHeader
	//-------------------------------
	{
		char	mHeader[4];				// should match roff_string defined above
		long	mVersion;				// version num, supported version defined above
		float	mCount;					// I think this is a float because of a limitation of the roff exporter

	} TROFFHeader;

	// ROFF Entry, nothing else needs to see this
	typedef struct tROFFEntry
	//-------------------------------
	{
		float		mOriginOffset[3];
		float		mRotateOffset[3];
	} TROFFEntry;

	typedef struct tROFF2Header
	//-------------------------------
	{
		char	mHeader[4];				// should match roff_string defined above
		long	mVersion;				// version num, supported version defined above
		int		mCount;					// I think this is a float because of a limitation of the roff exporter
		int		mFrameRate;				// Frame rate the roff should be played at
		int		mNumNotes;				// number of notes (null terminated strings) after the roff data

	} TROFF2Header;

	// ROFF Entry, nothing else needs to see this
	typedef struct tROFF2Entry
	//-------------------------------
	{
		float		mOriginOffset[3];
		float		mRotateOffset[3];
		int			mStartNote, mNumNotes;		// note track info
	} TROFF2Entry;

	// An individual ROFF object,
	//	contains actual rotation/offset information
	//--------------------------------------
	class CROFF
	//--------------------------------------
	{
	public:
	//------

		int			mID;						// id for this roff file
		char		mROFFFilePath[MAX_QPATH];	// roff file path
		int			mROFFEntries;				// count of move/rotate commands
		int			mFrameTime;					// frame rate
		int			mLerp;						// Lerp rate (FPS)
		TROFF2Entry	*mMoveRotateList;			// move rotate/command list
		int			mNumNoteTracks;
		char		**mNoteTrackIndexes;
		qboolean	mUsedByClient;
		qboolean	mUsedByServer;

		CROFF()
		{
			mUsedByClient = mUsedByServer = qfalse;
		}
		CROFF( const char *file, int id );
		~CROFF();

	}; // class CROFF


	// The roff system tracks entities that are
	//	roffing, so this is the internal structure
	//	that represents these objects.
	//--------------------------------------
	struct SROFFEntity
	//--------------------------------------
	{
		int			mEntID;			// the entity that is currently roffing

		int			mROFFID;		// the roff to be applied to that entity
		int			mNextROFFTime;	// next time we should roff
		int			mROFFFrame;		// current roff frame we are applying

		qboolean		mKill;			// flag to kill a roffing ent
		qboolean		mSignal;		// TODO:  Need to implement some sort of signal to Icarus when roff is done.
		qboolean	mTranslated;	// should this roff be "rotated" to fit the entity's initial position?
		qboolean	mIsClient;
		vec3_t		mStartAngles;	// initial angle of the entity
	}; // struct SROFFEntity


	qboolean	IsROFF( byte *file );				// Makes sure the file is a valid roff file
	qboolean	InitROFF( byte *file, CROFF *obj );	// Handles stashing raw roff data into the roff object
	qboolean	InitROFF2( byte *file, CROFF *obj );	// Handles stashing raw roff data into the roff object
	void	FixBadAngles(CROFF *obj);
	int		NewID() { return ++mID; }			// Increment before return so we can use zero as failed return val
	qboolean	ApplyROFF( SROFFEntity *roff_ent,
					CROFFSystem::CROFF *roff );	// True = success; False = roff complete

	void	ProcessNote(SROFFEntity *roff_ent, char *note);

	void	SetLerp( trajectory_t *tr,
					trType_t, vec3_t origin,
					vec3_t delta, int time, int rate );

	qboolean	ClearLerp( SROFFEntity *roff_ent );				// Clears out the angular and position lerp fields

public:
//------

	CROFFSystem()	{	mID = 0; mROFFEntList.clear();	}
	~CROFFSystem()	{	Restart();	}


	qboolean		Restart();						// Free up all system resources and reset the ID counter

	int			Cache( const char *file, qboolean isClient );			// roffs should be precached at the start of each level
	int			GetID( const char *file );			// find the roff id by filename
	qboolean		Unload( int id );				// when a roff is done, it can be removed to free up resources
	qboolean	Clean(qboolean isClient);					// should be called when level is done, frees all roff resources
	void		List(void);						// dumps a list of all cached roff files to the console
	qboolean		List( int id );					// dumps the contents of the specified roff to the console

	qboolean	Play( int entID, int roffID, qboolean doTranslation, qboolean isClient);	// TODO: implement signal on playback completion.
	void		ListEnts();						// List the entities that are currently roffing
	qboolean	PurgeEnt( int entID, qboolean isClient );			// Purge the specified entity from the entity list by id
	qboolean		PurgeEnt( char *file );			// Purge the specified entity from the entity list by name
	void		UpdateEntities(qboolean isClient);			// applys roff data to roffing entities.

}; // class CROFFSystem


extern CROFFSystem theROFFSystem;
