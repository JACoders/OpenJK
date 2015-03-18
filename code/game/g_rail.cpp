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

////////////////////////////////////////////////////////////////////////////////////////
// RAVEN SOFTWARE - STAR WARS: JK II
//  (c) 2002 Activision
//
// Rail System
//
// The rail system is intended to provide a means for generating moving entities along
// tracks of varying speed and direction.  The entities are pulled from the map based
// upon their targets and recycled in random positions and order
//
////////////////////////////////////////////////////////////////////////////////////////
#include "../qcommon/q_shared.h"
#include "bg_public.h"
#include "../cgame/cg_local.h"
#include "g_functions.h"
////////////////////////////////////////////////////////////////////////////////////////
// Externs & Fwd Decl.
////////////////////////////////////////////////////////////////////////////////////////
extern void		G_SoundAtSpot( vec3_t org, int soundIndex, qboolean broadcast );
extern void CG_DrawEdge( vec3_t start, vec3_t end, int type );

class	CRailTrack;
class	CRailLane;
class	CRailMover;


////////////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////////////
#include "b_local.h"
#if !defined(RATL_ARRAY_VS_INC)
	#include "../Ratl/array_vs.h"
#endif
#if !defined(RATL_VECTOR_VS_INC)
	#include "../Ratl/vector_vs.h"
#endif
#if !defined(RAVL_VEC_INC)
	#include "../Ravl/CVec.h"
#endif
#if !defined(RUFL_HSTRING_INC)
	#include "../Rufl/hstring.h"
#endif
#if !defined(RATL_GRID_VS_INC)
	#include "../Ratl/grid_vs.h"
#endif
#if !defined(RATL_POOL_VS_INC)
	#include "../Ratl/pool_vs.h"
#endif

////////////////////////////////////////////////////////////////////////////////////////
// Constants
////////////////////////////////////////////////////////////////////////////////////////
#define		MAX_TRACKS			4
#define		MAX_LANES			8
#define		MAX_MOVERS			150
#define		MAX_MOVER_ENTS		10
#define		MAX_MOVERS_TRACK	80
#define		MAX_COLS			32
#define		MAX_ROWS			96
#define		MAX_ROW_HISTORY		10

#define		WOOSH_DEBUG			0
#define		WOOSH_ALL_RANGE		1500.0f
#define		WOOSH_SUPPORT_RANGE	2500.0f
#define		WOOSH_TUNNEL_RANGE	3000.0f


bool		mRailSystemActive = false;


////////////////////////////////////////////////////////////////////////////////////////
// The Rail Track
//
// Tracks are the central component to the rails system.  They provide the master
// repositry of all movers and maintain the list of available movers as well as
//
////////////////////////////////////////////////////////////////////////////////////////
class	CRailTrack
{
public:
	void	Setup(gentity_t *ent)
	{
		mName						= ent->targetname;
		mSpeedGridCellsPerSecond	= ent->speed;
		mNumMoversPerRow			= ent->count;
		mMins						= ent->mins;
		mMaxs						= ent->maxs;
		mStartTime					= ent->delay + level.time;
		mGridCellSize				= (ent->radius!=0.0f)?(ent->radius):(1.0f);
		mVertical					= (ent->s.angles[1]==90.0f || ent->s.angles[1]==270.0f);	
		mNegative					= (ent->s.angles[1]==180.0f || ent->s.angles[1]==270.0f);	// From Maxs To Mins
		mWAxis						= (mVertical)?(0):(1);
		mHAxis						= (mVertical)?(1):(0);
		mTravelDistanceUnits		= ent->maxs[mHAxis] - ent->mins[mHAxis];

		mRow						= 0;
		mNextUpdateTime				= 0;
	
		mCenterLocked				= false;

		SnapVectorToGrid(mMins);
		SnapVectorToGrid(mMaxs);

		// Calculate Number Of Rows And Columns
		//--------------------------------------
		mRows						= ((mMaxs[mHAxis] - mMins[mHAxis]) / mGridCellSize);
		mCols						= ((mMaxs[mWAxis] - mMins[mWAxis]) / mGridCellSize);

		// Calculate Grid Center
		//-----------------------
		mGridCenter = ((mMins+mMaxs)*0.5f);
		SnapVectorToGrid(mGridCenter);

		// Calculate Speed & Velocity
		//----------------------------
		mSpeedUnitsPerMillisecond	= mSpeedGridCellsPerSecond * mGridCellSize / 1000.0f;
		mTravelTimeMilliseconds		= mTravelDistanceUnits / mSpeedUnitsPerMillisecond;

		AngleVectors(ent->s.angles, mDirection.v, 0, 0);
		mDirection.SafeNorm();
		mVelocity					= mDirection;
		mVelocity					*= (mSpeedGridCellsPerSecond * mGridCellSize);

		mNextUpdateDelay			= 1000.0f / (float)(mSpeedGridCellsPerSecond);


		// Calculate Bottom Left Corner
		//------------------------------
		mGridBottomLeftCorner = ent->mins;
		if (ent->s.angles[1]==180.0f)
		{
			mGridBottomLeftCorner[0] = mMaxs[0];
		}
		else if (ent->s.angles[1]==270.0f)
		{
			mGridBottomLeftCorner[1] = mMaxs[1];
		}
		SnapVectorToGrid(mGridBottomLeftCorner);


		mCells.set_size(mCols/*xSize*/, mRows/*ySize*/);
		mCells.init(0);

		mMovers.clear();


		if (!mNumMoversPerRow)
		{
			mNumMoversPerRow = 3;
		}

		// Safe Clamp Number Of Rows & Cols
		//----------------------------------
		if (mRows>(MAX_ROWS-1))
		{
			mRows = (MAX_ROWS-1);
			assert(0);
		}
		if (mCols>(MAX_COLS-1))
		{
			mCols = (MAX_COLS-1);
			assert(0);
		}
	}

	void	SnapVectorToGrid(CVec3&	Vec)
	{
		SnapFloatToGrid(Vec[0]);
		SnapFloatToGrid(Vec[1]);
	}

	void	SnapFloatToGrid(float& f)
	{
		f = (int)(f);

		bool	fNeg		= (f<0);
		if (fNeg)
		{
			f *= -1;		// Temporarly make it positive
		}

		int		Offset		= ((int)(f) % (int)(mGridCellSize));
		int		OffsetAbs	= abs(Offset);
		if (OffsetAbs>(mGridCellSize/2))
		{
			Offset = (mGridCellSize - OffsetAbs) * -1;
		}

		f -= Offset;

		if (fNeg)
		{
			f *= -1;		// Put It Back To Negative
		}

		f = (int)(f);

		assert(((int)(f)%(int)(mGridCellSize)) == 0);
	}





	void	Update();
	bool	TestMoverInCells(CRailMover* mover, int atCol);
	void	InsertMoverInCells(CRailMover* mover, int atCol);
	void	RandomizeTestCols(int startCol, int stopCol);







public:
	hstring		mName;

	int			mRow;
	int			mNumMoversPerRow;
	
	int			mNextUpdateTime;
	int			mNextUpdateDelay;
	int			mStartTime;

	int			mRows;
	int			mCols;

	bool		mVertical;
	bool		mNegative;
	int			mHAxis;
	int			mWAxis;

	int			mSpeedGridCellsPerSecond;
	float		mSpeedUnitsPerMillisecond;
	int			mTravelTimeMilliseconds;
	float		mTravelDistanceUnits;
	CVec3		mDirection;
	CVec3		mVelocity;

	CVec3		mMins;
	CVec3		mMaxs;

	CVec3		mGridBottomLeftCorner;
	CVec3		mGridCenter;
	float		mGridCellSize;

	bool		mCenterLocked;

	ratl::grid2_vs<CRailMover*, MAX_COLS, MAX_ROWS>	mCells;
	ratl::vector_vs<CRailMover*, MAX_MOVERS_TRACK>	mMovers;
	ratl::vector_vs<int, MAX_ROWS>					mTestCols;
};
ratl::vector_vs<CRailTrack, MAX_TRACKS>				mRailTracks;

////////////////////////////////////////////////////////////////////////////////////////
/*QUAKED rail_track (0 .5 .8) ? x x x x x x x x
A rail track determines what location and direction rail_mover entities go.  Don't bother with any origin brushes.  Make sure to set:

"radius"     Number of units to break down into grid size
"speed"      Number of grid sized units per second rail_movers will go at
"angle"      The direction rail_movers will go
"count"		 The number of mover ents the track will try to add per row
"delay"		 How long the ent will wait from the start of the level before placing movers
*/
////////////////////////////////////////////////////////////////////////////////////////
void SP_rail_track(gentity_t *ent)
{
	gi.SetBrushModel(ent, ent->model);
	G_SpawnInt("delay", "0", &ent->delay);
	mRailTracks.push_back().Setup(ent);
	G_FreeEntity(ent);
	mRailSystemActive = true;
}





////////////////////////////////////////////////////////////////////////////////////////
// The Rail Lane
//
//
//
////////////////////////////////////////////////////////////////////////////////////////
class	CRailLane
{
public:
	////////////////////////////////////////////////////////////////////////////////////
	// From Entity Setup Spawn
	////////////////////////////////////////////////////////////////////////////////////
	void		Setup(gentity_t* ent)
	{
		mName		= ent->targetname;
		mNameTrack	= ent->target;
		mMins		= ent->mins;
		mMaxs		= ent->maxs;
		mStartTime	= ent->delay + level.time;
	}

	hstring		mName;
	hstring		mNameTrack;
	CVec3		mMins;
	CVec3		mMaxs;
	int			mStartTime;

public:
	////////////////////////////////////////////////////////////////////////////////////
	// Initialize
	//
	// This function scans through the list of tracks and hooks itself up with the
	// track
	////////////////////////////////////////////////////////////////////////////////////
	void		Initialize()
	{
		mTrack		= 0;
		mMinCol		= 0;
		mMaxCol		= 0;

//		int		dummy;
		for (int i=0; i<mRailTracks.size(); i++)
		{
			if (mRailTracks[i].mName==mNameTrack)
			{
				mTrack	= &(mRailTracks[i]);
				mTrack->SnapVectorToGrid(mMins);
				mTrack->SnapVectorToGrid(mMaxs);

				mMinCol = (int)((mMins[mTrack->mWAxis] - mTrack->mMins[mTrack->mWAxis])/mTrack->mGridCellSize);
				mMaxCol = (int)((mMaxs[mTrack->mWAxis] - mTrack->mMins[mTrack->mWAxis] - (mTrack->mGridCellSize/2.0f))/mTrack->mGridCellSize);

				//if (mTrack->mNegative)
				//{
				//	mMinCol = (mTrack->mCols - mMinCol - 1);
				//	mMaxCol = (mTrack->mCols - mMaxCol - 1);
				//}


//				mTrack->mCells.get_cell_coords(mMins[mTrack->mWAxis], 0, mMinCol, dummy);
//				mTrack->mCells.get_cell_coords((mMaxs[mTrack->mWAxis]-10.0f), 0, mMaxCol, dummy);
				break;
			}
		}
		assert(mTrack!=0);
	}

	CRailTrack*	mTrack;
	int			mMinCol;
	int			mMaxCol;
};
ratl::vector_vs<CRailLane, MAX_LANES>		mRailLanes;

////////////////////////////////////////////////////////////////////////////////////////
/*QUAKED rail_lane (0 .5 .8) ? x x x x x x x x
Use rail lanes to split up tracks.  Just target it to a track that you want to break up into pieces

"delay"		 How long the ent will wait from the start of the level before placing movers
*/
////////////////////////////////////////////////////////////////////////////////////////
void SP_rail_lane(gentity_t *ent)
{
 	gi.SetBrushModel(ent, ent->model);
	G_SpawnInt("delay", "0", &ent->delay);
	mRailLanes.push_back().Setup(ent);
	G_FreeEntity(ent);
}





////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
class	CRailMover
{
public:
	////////////////////////////////////////////////////////////////////////////////////
	// From Entity Setup Spawn
	////////////////////////////////////////////////////////////////////////////////////
	void		Setup(gentity_t* ent)
	{
		mEnt					= ent;
		mCenter					= (ent->spawnflags&1);
		mSoundPlayed			= false;
		mOriginOffset			= ent->mins;
		mOriginOffset			+= ent->maxs;
		mOriginOffset			*= 0.5f;
		mOriginOffset[2]		= 0;//((ent->maxs[2] - ent->mins[2]) * 0.5);

		ent->e_ReachedFunc		= reachedF_NULL;
		ent->moverState			= MOVER_POS1;
		ent->svFlags			= SVF_USE_CURRENT_ORIGIN;
		ent->s.eType			= ET_MOVER;
		ent->s.eFlags			|= EF_NODRAW;
		ent->contents			= 0;
		ent->clipmask			= 0;

		ent->s.pos.trType		= TR_STATIONARY;
		ent->s.pos.trDuration	= 0;
		ent->s.pos.trTime		= 0;

		VectorCopy( ent->pos1, ent->currentOrigin );
		VectorCopy( ent->pos1, ent->s.pos.trBase );


		gi.linkentity(ent);
	}
	gentity_t*	mEnt;
	bool		mCenter;
	CVec3		mOriginOffset;
	bool		mSoundPlayed;


	bool		Active()
	{
		assert(mEnt!=0);
		return (level.time<(mEnt->s.pos.trDuration + mEnt->s.pos.trTime));
	}


public:
	////////////////////////////////////////////////////////////////////////////////////
	// Initialize
	//
	// This function scans through the list of tracks and hooks itself up with the
	// track (and possibly lane)
	////////////////////////////////////////////////////////////////////////////////////
	void		Initialize()
	{
		mTrack	= 0;
		mLane	= 0;
		mCols	= 0;
		mRows	= 0;

		hstring	target = mEnt->target;
		for (int track=0; track<mRailTracks.size(); track++)
		{
			if (mRailTracks[track].mName==target)
			{
				mTrack = &(mRailTracks[track]);
				break;
			}
		}
		if (mTrack==0)
		{
			for (int lane=0; lane<mRailLanes.size(); lane++)
			{
				if (mRailLanes[lane].mName==target)
				{
					mLane	= &(mRailLanes[lane]);
					mTrack	= mLane->mTrack;
					break;
				}
			}
		}
		assert(mTrack!=0);
		if (mTrack)
		{
			mTrack->mMovers.push_back(this);
			mCols	= (int)((mEnt->maxs[mTrack->mWAxis] - mEnt->mins[mTrack->mWAxis]) / mTrack->mGridCellSize) + 1;
			mRows	= (int)((mEnt->maxs[mTrack->mHAxis] - mEnt->mins[mTrack->mHAxis]) / mTrack->mGridCellSize) + 1;

			// Make Sure The Mover Fits In The Track And Lane
			//------------------------------------------------
			if (mRows>mTrack->mRows)
			{
//				assert(0);
				mRows = mTrack->mRows;
			}
			if (mCols>mTrack->mCols)
			{
//				assert(0);
				mCols = mTrack->mCols;
			}
			if (mLane && mCols>(mLane->mMaxCol - mLane->mMinCol + 1))
			{
//				assert(0);
				mCols = (mLane->mMaxCol - mLane->mMinCol + 1);
			}
		}
	}

	CRailTrack*	mTrack;
	CRailLane*	mLane;
	int			mCols;
	int			mRows;
};
ratl::vector_vs<CRailMover, MAX_MOVERS>		mRailMovers;

////////////////////////////////////////////////////////////////////////////////////////
/*QUAKED rail_mover (0 .5 .8) ? CENTER x x x x x x x
Rail Mover will go along the track and lane of your choice.  Just target it to either a track or a lane.  Don't bother with any origin brushes.

CENTER       Will force this mover to attempt to center in the track or lane

"target"     The track or lane you want this entity to move through
"model"      A model you wish to use, not necessary - can be just a brush
"angle"      Random angle rotation allowable on this thing
*/
////////////////////////////////////////////////////////////////////////////////////////
void SP_rail_mover(gentity_t *ent)
{
	gi.SetBrushModel(ent, ent->model);
	mRailMovers.push_back().Setup(ent);
}


ratl::vector_vs<int, 20>				mWooshSml;	// Small Building
ratl::vector_vs<int, 20>				mWooshMed;	// Medium Building
ratl::vector_vs<int, 10>				mWooshLar;	// Large Building
ratl::vector_vs<int, 10>				mWooshSup;	// Track Support
ratl::vector_vs<int, 3>					mWooshTun;	// Tunnel




////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
void	Rail_Reset()
{
	mRailSystemActive = false;
	mRailTracks.clear();
	mRailLanes.clear();
	mRailMovers.clear();

	mWooshSml.clear();
	mWooshMed.clear();
	mWooshLar.clear();
	mWooshSup.clear();
	mWooshTun.clear();
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
void	Rail_Initialize()
{
	for (int lane=0; lane<mRailLanes.size(); lane++)
	{
		mRailLanes[lane].Initialize();
	}

	for (int mover=0; mover<mRailMovers.size(); mover++)
	{
		mRailMovers[mover].Initialize();
	}

	// Precache All The Woosh Sounds
	//-------------------------------
	if (!mRailMovers.empty())
	{
		mWooshMed.push_back(G_SoundIndex("sound/effects/woosh1"));
		mWooshSml.push_back(G_SoundIndex("sound/effects/woosh2"));
		mWooshMed.push_back(G_SoundIndex("sound/effects/woosh3"));
		mWooshSml.push_back(G_SoundIndex("sound/effects/woosh4"));
		mWooshLar.push_back(G_SoundIndex("sound/effects/woosh5"));
		mWooshSml.push_back(G_SoundIndex("sound/effects/woosh6"));
		mWooshSup.push_back(G_SoundIndex("sound/effects/woosh7"));
		mWooshSup.push_back(G_SoundIndex("sound/effects/woosh8"));
		mWooshSup.push_back(G_SoundIndex("sound/effects/woosh9"));
		mWooshLar.push_back(G_SoundIndex("sound/effects/woosh10"));
		mWooshLar.push_back(G_SoundIndex("sound/effects/woosh11"));
		mWooshLar.push_back(G_SoundIndex("sound/effects/woosh12"));
		mWooshSml.push_back(G_SoundIndex("sound/effects/woosh13"));
		mWooshMed.push_back(G_SoundIndex("sound/effects/woosh14"));
		mWooshMed.push_back(G_SoundIndex("sound/effects/woosh15"));
		mWooshMed.push_back(G_SoundIndex("sound/effects/woosh16"));
		mWooshSml.push_back(G_SoundIndex("sound/effects/woosh17"));
		mWooshMed.push_back(G_SoundIndex("sound/effects/woosh18"));
		mWooshMed.push_back(G_SoundIndex("sound/effects/woosh19"));
		mWooshMed.push_back(G_SoundIndex("sound/effects/woosh20"));
		mWooshMed.push_back(G_SoundIndex("sound/effects/woosh21"));
		mWooshLar.push_back(G_SoundIndex("sound/effects/woosh22"));
		mWooshLar.push_back(G_SoundIndex("sound/effects/woosh23"));
		mWooshSup.push_back(G_SoundIndex("sound/effects/woosh24"));
		mWooshSup.push_back(G_SoundIndex("sound/effects/woosh25"));
		mWooshMed.push_back(G_SoundIndex("sound/effects/woosh26"));
		mWooshMed.push_back(G_SoundIndex("sound/effects/woosh27"));
		mWooshMed.push_back(G_SoundIndex("sound/effects/woosh28"));
		mWooshLar.push_back(G_SoundIndex("sound/effects/woosh29"));
		mWooshTun.push_back(G_SoundIndex("sound/effects/whoosh_tunnel"));
	}
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
void	Rail_Update()
{
	if (mRailSystemActive)// && false)
	{
		for (int track=0; track<mRailTracks.size(); track++)
		{
			if (level.time>mRailTracks[track].mNextUpdateTime && !mRailTracks[track].mMovers.empty())
			{
				mRailTracks[track].Update();
			}
		}

		// Is The Player Outside?
		//------------------------
 		if (player && gi.WE_IsOutside(player->currentOrigin))
		{
			int		wooshSound;
			vec3_t	wooshSoundPos;
			vec3_t	moverOrigin;
			vec3_t	playerToMover;
			float	playerToMoverDistance;
			float	playerToMoverDistanceFraction;

			// Iterate Over All The Movers
			//-----------------------------
 			for (int moverIndex=0; moverIndex<mRailMovers.size(); moverIndex++)
			{
 				CRailMover&	mover = mRailMovers[moverIndex];

				// Is It Active, And Has The Sound Already Played On It?
				//--------------------------------------------------------
   			 	if (mover.Active() && !mover.mSoundPlayed)
				{ 
 					VectorAdd(mover.mEnt->currentOrigin, mover.mOriginOffset.v, moverOrigin);
 					VectorSubtract(moverOrigin, player->currentOrigin, playerToMover);
					playerToMover[2]		= 0.0f;
 					playerToMoverDistance	= VectorNormalize(playerToMover);


					// Is It Close Enough?
					//---------------------
					if ((( mover.mLane || !mover.mCenter) &&								// Not Center Track
						 (playerToMoverDistance<WOOSH_ALL_RANGE) && 						//  And Close Enough
						 (DotProduct(playerToMover, mover.mTrack->mDirection.v)>-0.45f))	//  And On The Side
						||																	//OR
						((!mover.mLane &&  mover.mCenter) &&								// Is Center Track
						  (playerToMoverDistance<WOOSH_SUPPORT_RANGE ||						//  And Close Enough for Support
						  (playerToMoverDistance<WOOSH_TUNNEL_RANGE && mover.mRows>10))		//   Or Close Enough For Tunnel
						 ))
					{
						mover.mSoundPlayed = true;
						wooshSound = 0;

						// The Centered Entities Play Right On The Player's Head For Full Volume
						//-----------------------------------------------------------------------
						if (mover.mCenter && !mover.mLane)
						{
 							VectorCopy(player->currentOrigin, wooshSoundPos);
 							wooshSoundPos[2] += 50;

							// If It Is Very Long, Play The Tunnel Sound
							//-------------------------------------------
							if (mover.mRows>10)
							{
								wooshSound = mWooshTun[Q_irand(0, mWooshTun.size()-1)];
							}

							// Otherwise It Is A Support
							//---------------------------
							else 
							{ 
								wooshSound = mWooshSup[Q_irand(0, mWooshSup.size()-1)];
							}
						}

						// All Other Entities Play At A Fraction Of Their Normal Range
						//-------------------------------------------------------------
 						else 
						{
							// Scale The Play Pos By The Square Of The Distance
							//--------------------------------------------------
							playerToMoverDistanceFraction = playerToMoverDistance/WOOSH_ALL_RANGE;
 							playerToMoverDistanceFraction *= playerToMoverDistanceFraction;
							playerToMoverDistanceFraction *= 0.6f;
							playerToMoverDistance *= playerToMoverDistanceFraction;
 							VectorMA(player->currentOrigin, playerToMoverDistance, playerToMover, wooshSoundPos);

							// Large Building
							//----------------
							if (mover.mRows>4)
							{
								wooshSound = mWooshLar[Q_irand(0, mWooshLar.size()-1)];
							}

							// Medium Building
							//-----------------
							else if (mover.mRows>2)
							{
								wooshSound = mWooshMed[Q_irand(0, mWooshMed.size()-1)];
							}

							// Small Building
							//----------------
							else 
							{ 
								wooshSound = mWooshSml[Q_irand(0, mWooshSml.size()-1)];
							}
						}

						// If A Woosh Sound Was Selected, Play It Now
						//--------------------------------------------
						if (wooshSound)
						{
							G_SoundAtSpot(wooshSoundPos, wooshSound, qfalse);
							if (WOOSH_DEBUG)
							{
								CG_DrawEdge(player->currentOrigin, wooshSoundPos, EDGE_WHITE_TWOSECOND);
							}
						}
					}
				}
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
void	Rail_LockCenterOfTrack(const char* trackName)
{
	hstring	name = trackName;
	for (int track=0; track<mRailTracks.size(); track++)
	{
		if (mRailTracks[track].mName==name)
		{
			mRailTracks[track].mCenterLocked = true;
			return;
		}
	}
	assert(0);
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
void	Rail_UnLockCenterOfTrack(const char* trackName)
{
	hstring	name = trackName;
	for (int track=0; track<mRailTracks.size(); track++)
	{
		if (mRailTracks[track].mName==name)
		{
			mRailTracks[track].mCenterLocked = false;
			return;
		}
	}
	assert(0);
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
void	CRailTrack::Update()
{
	mNextUpdateTime = level.time + mNextUpdateDelay;


	// Now, Attempt To Add A Number Of Movers To The Track
	//-----------------------------------------------------
	int		attempt;
	int		startCol;
	int		stopCol;
	int		atCol;
	int		testColIndex;

	for (attempt=0; attempt<mNumMoversPerRow; attempt++)
	{
		// Randomly Select A Mover And Test To See If It Is Active
		//---------------------------------------------------------
		CRailMover*	mover = mMovers[Q_irand(0, mMovers.size()-1)];
		if (mover->Active())
		{
			continue;
		}

		// Don't Spawn Until Start Time Has Expired
		//------------------------------------------
		if (level.time < ((mover->mLane)?(mover->mLane->mStartTime):(mStartTime)))
		{
			continue;
		}

		// If Center Locked, Stop Spawning Center Track Movers
		//-----------------------------------------------------
		if (mover->mCenter && mCenterLocked)
		{
			continue;
		}
	

		// Restrict It To A Lane
		//-----------------------
		if (mover->mLane)
		{
			startCol	= mover->mLane->mMinCol;
			stopCol		= mover->mLane->mMaxCol+1;
		}

		// Or Let It Go Anywhere On The Track
		//------------------------------------
		else
		{
			startCol	= 0;
			stopCol		= mCols;
		}
		stopCol -= (mover->mCols-1);


		// If The Mover Is Too Big To Fit In The Lane, Go On To Next Attempt
		//-------------------------------------------------------------------
		if (stopCol<=startCol)
		{
			assert(0);	// Should Not Happen
			continue;
		}

		// Force It To Center
		//--------------------
		if (mover->mCenter && stopCol!=(startCol+1))
		{
			startCol	= ((mCols/2) - (mover->mCols/2));
			stopCol		= startCol+1;
		}


		// Construct A List Of Columns To Test For Insertion
		//---------------------------------------------------
		mTestCols.clear();
		for (int i=startCol; i<stopCol; i++)
		{
			mTestCols.push_back(i);
		}

		// Now Try All The Cols To See If The Building Can Fit
		//-----------------------------------------------------
		while (!mTestCols.empty())
		{
			// Randomly Pick A Column, Then Remove It From The Vector
			//--------------------------------------------------------
			testColIndex = Q_irand(0, mTestCols.size()-1);
			atCol = mTestCols[testColIndex];
			mTestCols.erase_swap(testColIndex);

			if (TestMoverInCells(mover, atCol))
			{
				// Ok, We've Found A Safe Column To Insert This Mover
				//----------------------------------------------------
				InsertMoverInCells(mover, atCol);

				// Now Transport The Actual Mover Entity Into Position, Link It & Send It Off
				//----------------------------------------------------------------------------
				CVec3	StartPos(mGridBottomLeftCorner);
				StartPos[mWAxis] += ((atCol * mGridCellSize) + ((mover->mCols/2.0f) * mGridCellSize));
				StartPos[mHAxis] += (((mover->mRows/2.0f) * mGridCellSize) * ((mNegative)?(1):(-1)));
				StartPos[2] = 0;

				// If Centered, Actually Put It At EXACTLY The Right Position On The Width Axis
				//------------------------------------------------------------------------------
				if (mover->mCenter)
				{
					StartPos[mWAxis] = mGridCenter[mWAxis];
					float	deltaOffset = mGridCenter[mWAxis] - mover->mOriginOffset[mWAxis];
					if (deltaOffset<(mGridCellSize*0.5f) )
					{
						StartPos[mWAxis] -= deltaOffset;
					}
				}

				StartPos -= mover->mOriginOffset;
				G_SetOrigin(mover->mEnt, StartPos.v);

				// Start It Moving
				//-----------------
				VectorCopy(StartPos.v, mover->mEnt->s.pos.trBase);
				VectorCopy(mVelocity.v, mover->mEnt->s.pos.trDelta);
				mover->mEnt->s.pos.trTime		= level.time;
				mover->mEnt->s.pos.trDuration	= mTravelTimeMilliseconds + (mNextUpdateDelay*mover->mRows);
				mover->mEnt->s.pos.trType		= TR_LINEAR_STOP;
				mover->mEnt->s.eFlags			&= ~EF_NODRAW;

				mover->mSoundPlayed				= false;


				// Successfully Inserted This Mover.  Now Move On To The Next Mover
				//------------------------------------------------------------------
				break;
			}
		}
	}

	// Incriment The Current Row
	//---------------------------
	mRow++;
	if (mRow>=mRows)
	{
		mRow = 0;
	}

	// Erase The Erase Row
	//---------------------
	int	EraseRow = mRow - MAX_ROW_HISTORY;
	if (EraseRow<0)
	{
		EraseRow += mRows;
	}
	for (int col=0; col<mCols; col++)
	{
		mCells.get(col, EraseRow) = 0;
	}
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
void	CRailTrack::RandomizeTestCols(int startCol, int stopCol)
{
	int numCols = (stopCol - startCol);
	int swapA;
	int	swapB;



	for (int swapNum=0; swapNum<numCols; swapNum++)
	{
		swapA = Q_irand(0, numCols-1);
		swapB = Q_irand(0, numCols-1);
		if (swapA!=swapB)
		{
			mTestCols.swap(swapA, swapB);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
bool	CRailTrack::TestMoverInCells(CRailMover* mover, int atCol)
{
	//for (int moverRow=0; (moverRow<mover->mRows); moverRow++)
	//{
		for (int moverCol=0; (moverCol<mover->mCols); moverCol++)
		{
			if (mCells.get(atCol+moverCol, mRow/*+moverRow*/)!=0)
			{
				return false;
			}
		}
	//}
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
void	CRailTrack::InsertMoverInCells(CRailMover* mover, int atCol)
{
	for (int moverCol=0; (moverCol<mover->mCols); moverCol++)
	{
		int col = atCol+moverCol;
		for (int moverRow=0; (moverRow<mover->mRows); moverRow++)
		{
			int row = mRow+moverRow;
			if (row>=mRows)
			{
				row -= mRows;
			}
			assert(mCells.get(col, row)==0);
			mCells.get(col, row) = mover;
		}
	}
}

