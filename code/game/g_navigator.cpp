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
//
//
//
//
////////////////////////////////////////////////////////////////////////////////////////

#include "../cgame/cg_local.h"
#include "g_shared.h"
#include "g_nav.h"

////////////////////////////////////////////////////////////////////////////////////////
// HFile Bindings
////////////////////////////////////////////////////////////////////////////////////////
bool	HFILEopen_read(int& handle,	const char* filepath)		{gi.FS_FOpenFile(filepath, &handle, FS_READ);	return (handle!=0);}
bool	HFILEopen_write(int& handle, const char* filepath)		{gi.FS_FOpenFile(filepath, &handle, FS_WRITE);	return (handle!=0);}
bool	HFILEread(int& handle,		void*		data, int size)	{return (gi.FS_Read(data, size, handle)!=0);}
bool	HFILEwrite(int& handle,		const void* data, int size)	{return (gi.FS_Write(data, size, handle)!=0);}
bool	HFILEclose(int& handle)									{gi.FS_FCloseFile(handle); return true;}



////////////////////////////////////////////////////////////////////////////////////////
// Externs
////////////////////////////////////////////////////////////////////////////////////////
extern gentity_t*	G_FindDoorTrigger( gentity_t *ent );
extern qboolean		G_EntIsBreakable( int entityNum, gentity_t *breaker );
extern qboolean		G_CheckInSolidTeleport (const vec3_t& teleportPos, gentity_t *self);

extern cvar_t*		g_nav1;
extern cvar_t*		g_nav2;
extern cvar_t*		g_developer;
extern int			delayedShutDown;
extern vec3_t		playerMinsStep;
extern vec3_t		playerMaxs;

////////////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////////////
#include "b_local.h"
#include "g_navigator.h"
#if !defined(RAGL_GRAPH_VS_INC)
	#include "../Ragl/graph_vs.h"
#endif
#if !defined(RATL_GRAPH_REGION_INC)
	#include "../Ragl/graph_region.h"
#endif
#if !defined(RATL_VECTOR_VS_INC)
	#include "../Ratl/vector_vs.h"
#endif
#if !defined(RUFL_HSTRING_INC)
	#include "../Rufl/hstring.h"
#endif
#if !defined(RUFL_HFILE_INC)
	#include "../Rufl/hfile.h"
#endif
#if !defined(RAVL_BOUNDS_INC)
	#include "../Ravl/CBounds.h"
#endif




////////////////////////////////////////////////////////////////////////////////////////
// Defines
////////////////////////////////////////////////////////////////////////////////////////
#define		NAV_VERSION						1.3f
#define		NEIGHBORING_DIST				200.0f
#define		SAFE_NEIGHBORINGPOINT_DIST		400.0f
#define		SAFE_AT_NAV_DIST_SQ				6400.0f			//80*80
#define		SAFE_GOTO_DIST_SQ				19600.0f		//140*140

namespace NAV
{
	enum
	{
		NUM_NODES			= 1024,
		// now 5 bytes each
		NUM_EDGES			= 3*NUM_NODES,
		NUM_EDGES_PER_NODE	= 20,


		NUM_REGIONS			= NUM_NODES/3,	// Had to raise this up for bounty
		NUM_CELLS			= 32,	// should be the square root of NUM_NODES
		NUM_NODES_PER_CELL	= 60,	// had to raise this for t3_bounty
		NUM_TARGETS			= 5,	// max number of outgoing edges from a given node

		CELL_RANGE			= 1000,
		VIEW_RANGE			= 550,

		BIAS_NONWAYPOINT	= 500,
		BIAS_DANGER			= 8000,
		BIAS_TOOSMALL		= 10000,

		NULL_PATH_USER_INDEX= -1,
		MAX_PATH_USERS		= 100,
		MAX_PATH_SIZE		= NUM_NODES/7,

		Z_CULL_OFFSET		= 60,

		MAX_NODES_PER_NAME	= 30,
		MAX_EDGE_SEG_LEN	= 100,
		MAX_EDGE_FLOOR_DIST	= 60,
		MAX_EDGE_AUTO_LEN   = 500,

		MAX_EDGES_PER_ENT	= 10,
		MAX_BLOCKING_ENTS	= 100,
		MAX_ALERTS_PER_AGENT= 10,
		MAX_ALERT_TIME		= 10000,

		MIN_WAY_NEIGHBORS	= 4,
		MAX_NONWP_NEIGHBORS = 1,


		// Human Sized
		//-------------
		SC_MEDIUM_RADIUS	= 20,
		SC_MEDIUM_HEIGHT	= 60,

		// Rancor Sized
		//--------------
		SC_LARGE_RADIUS		= 60,
		SC_LARGE_HEIGHT		= 120,


		SAVE_LOAD			= 0,

		CHECK_JUMP			= 0,
		CHECK_START_OPEN	= 1,
		CHECK_START_SOLID	= 1,
	};
}

namespace STEER
{
	enum
	{
		NULL_STEER_USER_INDEX= -1,
		MAX_NEIGHBORS		 = 20,
		Z_CULL_OFFSET		 = 60,
		SIDE_LOCKED_TIMER	 = 2000,
		NEIGHBOR_RANGE		 = 60,
	};
}



////////////////////////////////////////////////////////////////////////////////////////
// Total Memory - 11 Bytes (can save 5 bytes by removing Name and Targets)
////////////////////////////////////////////////////////////////////////////////////////
class	CWayNode
{
public:
	CVec3			mPoint;
	float			mRadius;
	NAV::EPointType	mType;
	hstring			mName;						// TODO OPTIMIZATION: Remove This?
	hstring			mTargets[NAV::NUM_TARGETS];	// TODO OPTIMIZATION: Remove This
	enum EWayNodeFlags
	{
		WN_NONE = 0,
		WN_ISLAND,
		WN_FLOATING,
		WN_DROPTOFLOOR,
		WN_NOAUTOCONNECT,
		WN_MAX
	};
	ratl::bits_vs<WN_MAX>	mFlags;

	////////////////////////////////////////////////////////////////////////////////////
	// Access Operator (For Cells)(For Triangulation)
	////////////////////////////////////////////////////////////////////////////////////
	float			operator[](int dimension)
	{
		return mPoint[dimension];
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Left Right Test (For Triangulation)
	////////////////////////////////////////////////////////////////////////////////////
	virtual		ESide	LRTest(const CWayNode& A, const CWayNode& B) const
	{
		return (mPoint.LRTest(A.mPoint, B.mPoint));
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Point In Circle (For Triangulation)
	////////////////////////////////////////////////////////////////////////////////////
	virtual		bool	InCircle(const CWayNode& A, const CWayNode& B, const CWayNode& C) const
	{
		return (mPoint.PtInCircle(A.mPoint, B.mPoint, C.mPoint));
	}
};
const CWayNode&		GetNode(int Handle);

////////////////////////////////////////////////////////////////////////////////////////
// Total Memory - 5 bytes
////////////////////////////////////////////////////////////////////////////////////////
class	CWayEdge
{
public:
	int				mNodeA;			// DO NOT REMOVE THIS: Handles are full ints because upper bits are used
	int				mNodeB;			// DO NOT REMOVE THIS: Handles are full ints because upper bits are used
	float			mDistance;		// DO NOT REMOVE THIS: It's a serious runtime optimization for A*

	unsigned short	mOwnerNum;		// Converted to short.  Largest entity number is 1024
	unsigned short	mEntityNum;		// Converted to short.  Largest entity number is 1024
	enum EWayEdgeFlags
	{
		WE_NONE = 0,

		WE_SIZE_MEDIUM,
		WE_SIZE_LARGE,

		WE_BLOCKING_DOOR,
		WE_BLOCKING_WALL,
		WE_BLOCKING_BREAK,

		WE_VALID,
		WE_ONHULL,
		WE_FLYING,
		WE_JUMPING,
		WE_CANBEINVAL,
		WE_DESIGNERPLACED,

		WE_MAX
	};
	ratl::bits_vs<WE_MAX>	mFlags;		// Should be only one int


	////////////////////////////////////////////////////////////////////////////////////
	// Size Function
	////////////////////////////////////////////////////////////////////////////////////
	inline int		Blocking()
	{
		if (mFlags.get_bit(WE_BLOCKING_BREAK))
		{
			return WE_BLOCKING_BREAK;
		}
		if (mFlags.get_bit(WE_BLOCKING_WALL))
		{
			return WE_BLOCKING_WALL;
		}
		if (mFlags.get_bit(WE_BLOCKING_DOOR))
		{
			return WE_BLOCKING_DOOR;
		}
		return 0;
	}
	inline	bool	BlockingBreakable()		{return (mFlags.get_bit(WE_BLOCKING_BREAK));}
	inline	bool	BlockingWall()			{return (mFlags.get_bit(WE_BLOCKING_WALL));}
	inline	bool	BlockingDoor()			{return (mFlags.get_bit(WE_BLOCKING_DOOR));}


	////////////////////////////////////////////////////////////////////////////////////
	// Size Function
	////////////////////////////////////////////////////////////////////////////////////
	inline int		Size() const
	{
		return (mFlags.get_bit(WE_SIZE_MEDIUM)?(WE_SIZE_MEDIUM):(WE_SIZE_LARGE));
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Access Operator (For Cells)(For Triangulation)
	////////////////////////////////////////////////////////////////////////////////////
	float			operator[](int dimension) const
	{
		CVec3	Half(GetNode(mNodeA).mPoint + GetNode(mNodeB).mPoint);
		return (Half[dimension] * 0.5f);
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Point - returns the center of the edge
	////////////////////////////////////////////////////////////////////////////////////
	void			Point(CVec3& Half) const
	{
		Half = GetNode(mNodeA).mPoint;
		Half += GetNode(mNodeB).mPoint;
		Half *= 0.5f;

	}

	////////////////////////////////////////////////////////////////////////////////////
	// GetPoint A
	////////////////////////////////////////////////////////////////////////////////////
	const CVec3&	PointA() const
	{
		return GetNode(mNodeA).mPoint;
	}

	////////////////////////////////////////////////////////////////////////////////////
	// GetPoint B
	////////////////////////////////////////////////////////////////////////////////////
	const CVec3&	PointB() const
	{
		return GetNode(mNodeB).mPoint;
	}

};
const CWayEdge&	GetEdge(int Handle);


struct	SNodeSort
{
	NAV::TNodeHandle	mHandle;
	float				mDistance;
	bool				mInRadius;


	bool	operator < (const SNodeSort& other) const
	{
		return (mDistance<other.mDistance);
	}
};

struct SEntSize
{
	float	mRadius;
	float	mHeight;
};

struct SDangerAlert
{
	int		mHandle;
	float	mDanger;
};




////////////////////////////////////////////////////////////////////////////////////////
// Defines
////////////////////////////////////////////////////////////////////////////////////////
typedef		ragl::graph_vs		<CWayNode, NAV::NUM_NODES, CWayEdge, NAV::NUM_EDGES, NAV::NUM_EDGES_PER_NODE>				TGraph;
typedef		ragl::graph_region	<CWayNode, NAV::NUM_NODES, CWayEdge, NAV::NUM_EDGES, NAV::NUM_EDGES_PER_NODE, NAV::NUM_REGIONS, NAV::NUM_REGIONS>	TGraphRegion;
typedef		TGraph::cells		<NAV::NUM_NODES_PER_CELL, NAV::NUM_CELLS, NAV::NUM_CELLS>									TGraphCells;
typedef		ratl::vector_vs		<SNodeSort, NAV::NUM_NODES_PER_CELL>														TNearestNavSort;

typedef		ratl::array_vs		<SDangerAlert, NAV::MAX_ALERTS_PER_AGENT>													TAlertList;
typedef		ratl::array_vs		<TAlertList, MAX_GENTITIES>																	TEntityAlertList;
typedef		ratl::vector_vs		<NAV::TNodeHandle, NAV::MAX_NODES_PER_NAME>													TNamedNodeList;
typedef		ratl::map_vs		<hstring, TNamedNodeList, NAV::NUM_NODES>													TNameToNodeMap;

typedef		ratl::vector_vs		<NAV::TEdgeHandle, NAV::MAX_EDGES_PER_ENT>													TEdgesPerEnt;
typedef		ratl::map_vs		<int, TEdgesPerEnt, NAV::MAX_BLOCKING_ENTS>													TEntEdgeMap;


////////////////////////////////////////////////////////////////////////////////////////
// Path Point
//
// This is actual vector and speed location (as well as node handle of that the agent
// has planned to go to.
////////////////////////////////////////////////////////////////////////////////////////
struct	SPathPoint
{
	CVec3				mPoint;
	float				mSpeed;
	float				mSlowingRadius;
	float				mReachedRadius;
	float				mDist;
	float				mETA;
	NAV::TNodeHandle	mNode;
};
typedef		ratl::vector_vs		<SPathPoint, NAV::MAX_PATH_SIZE>															TPath;


////////////////////////////////////////////////////////////////////////////////////////
// Path User
//
// This is the cached path for a given actor
////////////////////////////////////////////////////////////////////////////////////////
struct	SPathUser
{
	int			mEnd;
	bool		mSuccess;
	int			mLastUseTime;
	int			mLastAStarTime;
	TPath		mPath;
};
typedef		ratl::pool_vs<SPathUser, NAV::MAX_PATH_USERS>																	TPathUsers;
typedef		ratl::array_vs<int, MAX_GENTITIES>																				TPathUserIndex;


typedef		ratl::vector_vs<gentity_t*, STEER::MAX_NEIGHBORS>																TNeighbors;

////////////////////////////////////////////////////////////////////////////////////////
// Steer User
//
// This is the cached steering data for a given actor
////////////////////////////////////////////////////////////////////////////////////////
struct	SSteerUser
{
	// Constant Values In Entity
	//---------------------------
	float		mMaxForce;
	float		mMaxSpeed;
	float		mRadius;
	float		mMass;


	// Current Values
	//----------------
	TNeighbors	mNeighbors;

	CVec3		mOrientation;
	CVec3		mPosition;

	CVec3		mVelocity;
	float		mSpeed;


	// Values Projected From Current Values
	//--------------------------------------
	CVec3		mProjectFwd;
	CVec3		mProjectSide;
	CVec3		mProjectPath;


	// Temporary Values
	//------------------
	CVec3		mDesiredVelocity;
	float		mDesiredSpeed;
	float		mDistance;
	CVec3		mSeekLocation;

	int			mIgnoreEntity;

	bool		mBlocked;
	int			mBlockedTgtEntity;
	CVec3		mBlockedTgtPosition;

	// Steering
	//----------
	CVec3		mSteering;
	float		mNewtons;
};

typedef		ratl::pool_vs<SSteerUser, 4>																					TSteerUsers;
typedef		ratl::array_vs<int, MAX_GENTITIES>																				TSteerUserIndex;
typedef		ratl::bits_vs<MAX_GENTITIES>																					TEntBits;


TAlertList&			GetAlerts(gentity_t* actor);
TGraph&				GetGraph();
int					GetAirRegion();
int					GetIslandRegion();



////////////////////////////////////////////////////////////////////////////////////////
// The Graph User
//
// Here we define our own user class, which can invalidate edges and generate unique
// costs for node traversal based on the nodes, and possibly the actor attempting to
// cross the nodes at any given time.
//
////////////////////////////////////////////////////////////////////////////////////////
class		CGraphUser : public TGraph::user
{
private:
	gentity_t*			mActor;
	int					mActorSize;

	CVec3				mDangerSpot;
	float				mDangerSpotRadiusSq;


public:
	////////////////////////////////////////////////////////////////////////////////////
	//
	////////////////////////////////////////////////////////////////////////////////////
	void	SetActor(gentity_t* actor)
	{
		mActor			= actor;
		mActorSize		= NAV::ClassifyEntSize(actor);
		mDangerSpotRadiusSq	= 0;
	}

	////////////////////////////////////////////////////////////////////////////////////
	//
	////////////////////////////////////////////////////////////////////////////////////
	void	ClearActor()
	{
		mActor = 0;
		mActorSize = 0;
		mDangerSpotRadiusSq = 0;
	}

	gentity_t*	GetActor()
	{
		return mActor;
	}

	void	SetDangerSpot(const CVec3& Spot, float RadiusSq)
	{
		mDangerSpot = Spot;
		mDangerSpotRadiusSq = RadiusSq;
	}
	void	ClearDangerSpot()
	{
		mDangerSpotRadiusSq = 0;
	}




public:
	////////////////////////////////////////////////////////////////////////////////////
	//
	////////////////////////////////////////////////////////////////////////////////////
	virtual		bool	can_be_invalid(const CWayEdge& Edge) const
	{
		return (Edge.mFlags.get_bit(CWayEdge::WE_CANBEINVAL));
	}


	////////////////////////////////////////////////////////////////////////////////////
	//
	////////////////////////////////////////////////////////////////////////////////////
	virtual		bool	is_valid(CWayEdge& Edge, int EndPoint=0) const
	{
		// If The Actor Can't Fly, But This Is A Flying Edge, It's Invalid
		//-----------------------------------------------------------------
		if (mActor && Edge.mFlags.get_bit(CWayEdge::WE_FLYING) && mActor->NPC && !(mActor->NPC->scriptFlags&SCF_NAV_CAN_FLY))
		{
			return false;
		}

		// If The Actor Can't Fly, But This Is A Flying Edge, It's Invalid
		//-----------------------------------------------------------------
		if (mActor && Edge.mFlags.get_bit(CWayEdge::WE_JUMPING) && mActor->NPC && !(mActor->NPC->scriptFlags&SCF_NAV_CAN_JUMP))
		{
			return false;
		}


		// If The Actor Is Too Big, This Is Not A Valid Edge For Him
		//-----------------------------------------------------------
		if (mActor && Edge.Size()<mActorSize &&
 			(EndPoint!=-1) /*&&									// Don't count the last edge on the path as invalid because of size
			(!EndPoint || (EndPoint!=Edge.mNodeA && EndPoint!=Edge.mNodeB))*/)
		{
			return false;
		}

		if (Edge.mEntityNum!=ENTITYNUM_NONE)
		{
			gentity_t*	ent		= &g_entities[Edge.mEntityNum];
			if (ent)
			{
				// Can The Actor Navigate Through The Breakable Entity?
				//------------------------------------------------------
				if ((mActor) &&
					(mActor->NPC) &&
					(mActor->NPC->aiFlags&NPCAI_NAV_THROUGH_BREAKABLES) &&
					(Edge.BlockingBreakable()) &&
					(G_EntIsBreakable(Edge.mEntityNum, mActor))
					)
				{
					return true;
				}

				// Is This A Door?
				//-----------------
				if (Edge.BlockingDoor())
				{
					bool	StartOpen	= (ent->spawnflags & 1);
					bool	Closed		= (StartOpen)?(ent->moverState==MOVER_POS2):(ent->moverState==MOVER_POS1);

					// If It Is Closed, We Want To Check If It Will Auto Open For Us
					//---------------------------------------------------------------
					if (Closed)
					{
						gentity_t*	owner	= &g_entities[Edge.mOwnerNum];
						if (owner)
						{
							// Check To See If The Owner Is Inactive Or Locked, Or Unavailable To The NPC
							//----------------------------------------------------------------------------
							if ((owner->svFlags & SVF_INACTIVE) ||
								(owner==ent && (owner->spawnflags & (MOVER_PLAYER_USE|MOVER_FORCE_ACTIVATE|MOVER_LOCKED))) ||
								(owner!=ent && (owner->spawnflags & (1 /*PLAYERONLY*/|4 /*USE_BOTTON*/))))
							{
								return false;
							}


							// Look For A Key
							//----------------
							if (mActor!=0 && (owner->spawnflags & MOVER_GOODIE))
							{
								int key = INV_GoodieKeyCheck(mActor);
								if (!key)
								{
									return false;
								}
							}
						}

						// No Owner?  This Must Be A Scripted Door Or Other Contraption
						//--------------------------------------------------------------
						else
						{
							return false;
						}
					}
					return true;
				}

				// If This Is A Wall, Check If It Has Contents Now
				//-------------------------------------------------
				else if (Edge.BlockingWall())
				{
					return !(ent->contents&CONTENTS_SOLID);
				}
			}
		}
		else if ( Edge.BlockingBreakable())
		{//we had a breakable in our way, now it's gone, see if there is anything else in the way
			if ( NAV::TestEdge( Edge.mNodeA, Edge.mNodeB, false ) )
			{//clear it
				Edge.mFlags.clear_bit(CWayEdge::WE_BLOCKING_BREAK);
			}
			//NOTE: if this fails with the SC_LARGE size
		}

		return (Edge.mFlags.get_bit(CWayEdge::WE_VALID));
	}

	////////////////////////////////////////////////////////////////////////////////////
	// This is the cost estimate from any node to any other node (usually the goal)
	////////////////////////////////////////////////////////////////////////////////////
	virtual		float	cost(const CWayNode& A, const CWayNode& B) const
	{
		return (A.mPoint.Dist(B.mPoint));
	}

	////////////////////////////////////////////////////////////////////////////////////
	// This is the cost estimate for traversing a particular edge
	////////////////////////////////////////////////////////////////////////////////////
	virtual		float	cost(const CWayEdge& Edge, const CWayNode& B) const
	{
		float	DangerBias = 0.0f;
		if (mActor)
		{
			int			eHandle = GetGraph().edge_index(Edge);
			TAlertList& al = GetAlerts(mActor);
			for (int alIndex=0; alIndex<TAlertList::CAPACITY; alIndex++)
			{
				if (al[alIndex].mHandle==eHandle && al[alIndex].mDanger>0.0f)
				{
					DangerBias += (al[alIndex].mDanger*NAV::BIAS_DANGER);
				}
			}

			// If The Actor Is Too Big, Bias This Edge For Him
			//-------------------------------------------------
			if (Edge.Size()<mActorSize)
			{
				//DangerBias += NAV::BIAS_TOOSMALL;
			}
		}

		if (mDangerSpotRadiusSq > mDangerSpot.DistToLine2(Edge.PointA(), Edge.PointB()))
		{
			DangerBias += NAV::BIAS_DANGER;
		}


		if (B.mType==NAV::PT_WAYNODE)
		{
            return (Edge.mDistance + DangerBias);
		}
		return ((Edge.mDistance + DangerBias) + NAV::BIAS_NONWAYPOINT);
	}

	////////////////////////////////////////////////////////////////////////////////////
	//
	////////////////////////////////////////////////////////////////////////////////////
	virtual		bool	on_same_floor(const CWayNode& A, const CWayNode& B) const
	{
		return (fabsf(A.mPoint[2] - B.mPoint[2])<100.0f);
	}


	////////////////////////////////////////////////////////////////////////////////////
	// setup the edge (For Triangulation)
	//
	// This function is here because it evaluates the base cost from NodeA to NodeB, which
	//
	////////////////////////////////////////////////////////////////////////////////////
	virtual		void	setup_edge(CWayEdge& Edge, int A, int B, bool OnHull, const CWayNode& NodeA, const CWayNode& NodeB, bool CanBeInvalid=false)
	{
		Edge.mNodeA			= A;
		Edge.mNodeB			= B;
		Edge.mDistance		= NodeA.mPoint.Dist(NodeB.mPoint);
		Edge.mEntityNum		= ENTITYNUM_NONE;
		Edge.mOwnerNum		= ENTITYNUM_NONE;
		Edge.mFlags.clear();
		Edge.mFlags.set_bit(CWayEdge::WE_VALID);
		if (CanBeInvalid)
		{
			Edge.mFlags.set_bit(CWayEdge::WE_CANBEINVAL);
		}
		if (OnHull)
		{
			Edge.mFlags.set_bit(CWayEdge::WE_ONHULL);
		}
	}
};



////////////////////////////////////////////////////////////////////////////////////////
// The Global Public Objects
////////////////////////////////////////////////////////////////////////////////////////
TGraph				mGraph;
TGraphRegion		mRegion(mGraph);
TGraphCells			mCells(mGraph);

TGraph::search		mSearch;
CGraphUser			mUser;


TNameToNodeMap		mNodeNames;
TEntEdgeMap			mEntEdgeMap;

TNearestNavSort		mNearestNavSort;

TPathUsers			mPathUsers;
TPathUserIndex		mPathUserIndex;
SPathUser			mPathUserMaster;

TSteerUsers			mSteerUsers;
TSteerUserIndex		mSteerUserIndex;

TEntityAlertList	mEntityAlertList;

vec3_t				mZeroVec;
trace_t				mMoveTrace;
trace_t				mViewTrace;

int					mMoveTraceCount = 0;
int					mViewTraceCount = 0;
int					mConnectTraceCount = 0;
int					mConnectTime = 0;
int					mIslandCount = 0;
int					mIslandRegion = 0;
int					mAirRegion = 0;
char				mLocStringA[256] = {0};
char				mLocStringB[256] = {0};



////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
TAlertList&			GetAlerts(gentity_t* actor)
{
	return mEntityAlertList[actor->s.number];
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
TGraph&				GetGraph()
{
	return mGraph;
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
const CWayNode&		GetNode(int Handle)
{
	return mGraph.get_node(Handle);
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
const CWayEdge&		GetEdge(int Handle)
{
	return mGraph.get_edge(Handle);
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
int					GetAirRegion()
{
	return mAirRegion;
}
int					GetIslandRegion()
{
	return mIslandRegion;
}


////////////////////////////////////////////////////////////////////////////////////////
// Helper Function : View Trace
////////////////////////////////////////////////////////////////////////////////////////
bool		ViewTrace(const CVec3& a, const CVec3& b)
{
	int contents	= (CONTENTS_SOLID|CONTENTS_TERRAIN|CONTENTS_MONSTERCLIP);

	mViewTraceCount++;
	gi.trace(&mViewTrace, a.v, 0, 0, b.v, ENTITYNUM_NONE, contents, (EG2_Collision)0, 0);

	if ((mViewTrace.allsolid==qfalse) && (mViewTrace.startsolid==qfalse ) && (mViewTrace.fraction==1.0f))
	{
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////
// Helper Function : View Trace
////////////////////////////////////////////////////////////////////////////////////////
bool		ViewNavTrace(const CVec3& a, const CVec3& b)
{
	int contents	= (CONTENTS_SOLID|CONTENTS_TERRAIN|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP);

	mViewTraceCount++;
	gi.trace(&mViewTrace, a.v, 0, 0, b.v, ENTITYNUM_NONE, contents, (EG2_Collision)0, 0);

	if ((mViewTrace.allsolid==qfalse) && (mViewTrace.startsolid==qfalse ) && (mViewTrace.fraction==1.0f))
	{
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////
// Helper Function : Move Trace
////////////////////////////////////////////////////////////////////////////////////////
bool		MoveTrace(const CVec3& Start, const CVec3& Stop, const CVec3& Mins, const CVec3& Maxs,
					  int IgnoreEnt=0,
					  bool CheckForDoNotEnter=false,
					  bool RetryIfStartInDoNotEnter=true,
					  bool IgnoreAllEnts=false,
					  int OverrideContents=0)
{
	int contents	= (MASK_NPCSOLID);
	if (OverrideContents)
	{
		contents	= OverrideContents;
	}
	if (CheckForDoNotEnter)
	{
		contents |= CONTENTS_BOTCLIP;
	}
	if (IgnoreAllEnts)
	{
		contents &= ~CONTENTS_BODY;
	}


	// Run The Trace
	//---------------
	mMoveTraceCount++;
	gi.trace(&mMoveTrace, Start.v, Mins.v, Maxs.v, Stop.v, IgnoreEnt, contents, (EG2_Collision)0, 0);


	// Did It Make It?
	//-----------------
	if ((mMoveTrace.allsolid==qfalse) && (mMoveTrace.startsolid==qfalse ) && (mMoveTrace.fraction==1.0f))
	{
		return true;
	}

	// If We Started In Solid, Try Removing The "Do Not Enter" Contents Type, And Trace Again
	//----------------------------------------------------------------------------------------
	if (CheckForDoNotEnter && RetryIfStartInDoNotEnter && ((mMoveTrace.allsolid==qtrue) || (mMoveTrace.startsolid==qtrue)))
	{
		contents &= ~CONTENTS_BOTCLIP;

		// Run The Trace
		//---------------
		mMoveTraceCount++;
		gi.trace(&mMoveTrace, Start.v, Mins.v, Maxs.v, Stop.v, IgnoreEnt, contents, (EG2_Collision)0, 0);

		// Did It Make It?
		//-----------------
		if ((mMoveTrace.allsolid==qfalse) && (mMoveTrace.startsolid==qfalse ) && (mMoveTrace.fraction==1.0f))
		{
			return true;
		}
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////
// Helper Function : Move Trace (with actor)
////////////////////////////////////////////////////////////////////////////////////////
bool		MoveTrace(gentity_t* actor, const CVec3& goalPosition, bool IgnoreAllEnts=false)
{
	assert(actor!=0);
	CVec3	Mins(actor->mins);
	CVec3	Maxs(actor->maxs);

	Mins[2] += (STEPSIZE*1);

	return MoveTrace(actor->currentOrigin, goalPosition, Mins, Maxs, actor->s.number, true, true, IgnoreAllEnts/*, actor->contents*/);
}




////////////////////////////////////////////////////////////////////////////////////////
// GoTo
//
// This Function serves as a master control for finding, updating, and following a path
////////////////////////////////////////////////////////////////////////////////////////
bool			NAV::GoTo(gentity_t* actor, TNodeHandle target, float MaxDangerLevel)
{
	assert(actor!=0 && actor->client!=0);

	// Check If We Already Have A Path
	//---------------------------------
	bool	HasPath = NAV::HasPath(actor);

	// If So Update It
	//-----------------
	if (HasPath)
	{
		HasPath = NAV::UpdatePath(actor, target, MaxDangerLevel);
	}

	// If No Path, Try To Find One
	//-----------------------------
	if (!HasPath)
	{
		HasPath = NAV::FindPath(actor, target, MaxDangerLevel);
	}

	// If We Have A Path, Now Try To Follow It
	//-----------------------------------------
	if (HasPath)
	{
		HasPath = (STEER::Path(actor)!=0.0f);
		if (HasPath)
		{
			if (STEER::AvoidCollisions(actor, actor->client->leader))
			{
				STEER::Blocked(actor, NAV::NextPosition(actor));
			}
		}
		else
		{
			STEER::Blocked(actor, NAV::GetNodePosition(target));
		}
	}

	// Nope, No Path At All...  Bad
	//------------------------------
	else
	{
		STEER::Blocked(actor, NAV::GetNodePosition(target));
	}

	return HasPath;
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
bool			NAV::GoTo(gentity_t* actor, gentity_t* target, float MaxDangerLevel)
{
	assert(actor!=0 && actor->client!=0);

	bool		HasPath		= false;
	TNodeHandle	targetNode	= GetNearestNode(target, true);

	// If The Target Has No Nearest Nav Point, Go To His Last Valid Waypoint Instead
	//-------------------------------------------------------------------------------
	if (targetNode==0)
	{
		targetNode = target->lastWaypoint;
	}

	// Has He EVER Had A Valid Waypoint?
	//-----------------------------------
	if (targetNode!=0)
	{
		// If On An Edge, Pick The Safest Of The Two Points
		//--------------------------------------------------
		if (targetNode<0)
		{
			targetNode = (Q_irand(0,1)==0)?(mGraph.get_edge(abs(targetNode)).mNodeA):(mGraph.get_edge(abs(targetNode)).mNodeB);
		}

		// Check If We Already Have A Path
		//---------------------------------
		HasPath = NAV::HasPath(actor);

		// If So Update It
		//-----------------
		if (HasPath)
		{
			HasPath = NAV::UpdatePath(actor, targetNode, MaxDangerLevel);
		}

		// If No Path, Try To Find One
		//-----------------------------
		if (!HasPath)
		{
			HasPath = NAV::FindPath(actor, targetNode, MaxDangerLevel);
		}

		// If We Have A Path, Now Try To Follow It
		//-----------------------------------------
		if (HasPath)
		{
			HasPath = (STEER::Path(actor)!=0.0f);
			if (HasPath)
			{
				// Attempt To Avoid Collisions Along The Path
				//--------------------------------------------
				if (STEER::AvoidCollisions(actor, actor->client->leader))
				{
					// Have A Path, Currently Blocked By Something
					//---------------------------------------------
					STEER::Blocked(actor, NAV::NextPosition(actor));
				}
			}
			else
			{
				STEER::Blocked(actor, target);
			}
		}

		// Nope, No Path At All...  Bad
		//------------------------------
		else
		{
			STEER::Blocked(actor, target);
		}
	}

	// No Waypoint Near
	//------------------
	else
	{
		STEER::Blocked(actor, target);
	}

	return HasPath;
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
bool			NAV::GoTo(gentity_t* actor, const vec3_t& position, float MaxDangerLevel)
{
	assert(actor!=0 && actor->client!=0);

	bool		HasPath		= false;
	TNodeHandle	targetNode	= GetNearestNode(position);
	if (targetNode!=0)
	{
		// If On An Edge, Pick The Safest Of The Two Points
		//--------------------------------------------------
		if (targetNode<0)
		{
			targetNode = (Q_irand(0,1)==0)?(mGraph.get_edge(abs(targetNode)).mNodeA):(mGraph.get_edge(abs(targetNode)).mNodeB);
		}

		// Check If We Already Have A Path
		//---------------------------------
		HasPath = NAV::HasPath(actor);

		// If So Update It
		//-----------------
		if (HasPath)
		{
			HasPath = NAV::UpdatePath(actor, targetNode, MaxDangerLevel);
		}

		// If No Path, Try To Find One
		//-----------------------------
		if (!HasPath)
		{
			HasPath = NAV::FindPath(actor, targetNode, MaxDangerLevel);
		}

		// If We Have A Path, Now Try To Follow It
		//-----------------------------------------
		if (HasPath)
		{
			HasPath = (STEER::Path(actor)!=0.0f);
			if (HasPath)
			{
				// Attempt To Avoid Collisions Along The Path
				//--------------------------------------------
				if (STEER::AvoidCollisions(actor, actor->client->leader))
				{
					// Have A Path, Currently Blocked By Something
					//---------------------------------------------
					STEER::Blocked(actor, NAV::NextPosition(actor));
				}
			}
			else
			{
				STEER::Blocked(actor, NAV::NextPosition(actor));
			}
		}

		// Nope, No Path At All...  Bad
		//------------------------------
		else
		{
			STEER::Blocked(actor, position);
		}
	}

	// No Waypoint Near
	//------------------
	else
	{
		STEER::Blocked(actor, position);
	}

	return HasPath;
}


////////////////////////////////////////////////////////////////////////////////////////
// This function exists as a wrapper so that the graph can write to the gi.Printf()
////////////////////////////////////////////////////////////////////////////////////////
void	stupid_print(const char* data)
{
	gi.Printf(data);
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
bool			NAV::LoadFromFile(const char *filename, int checksum)
{
	mZeroVec[0] = 0;
	mZeroVec[1] = 0;
	mZeroVec[2] = 0;

	mPathUserIndex.fill(NULL_PATH_USER_INDEX);
	mSteerUserIndex.fill(STEER::NULL_STEER_USER_INDEX);

	mMoveTraceCount = 0;
	mViewTraceCount = 0;
	mConnectTraceCount = 0;
	mConnectTime = 0;
	mIslandCount = 0;
	mIslandRegion = 0;
	mAirRegion = 0;

	memset(&mEntityAlertList, 0, sizeof(mEntityAlertList));

#if !defined(FINAL_BUILD)
	ratl::ratl_base::OutputPrint = stupid_print;
#endif

	mGraph.clear();
	mRegion.clear();
	mCells.clear();
	mNodeNames.clear();
	mNearestNavSort.clear();

	if (SAVE_LOAD)
	{
		hfile	navFile(va("maps/%s.navNEW"));
		if (!navFile.open_read(NAV_VERSION, checksum))
		{
			return false;
		}
		navFile.load(&mGraph, sizeof(mGraph));
		navFile.load(&mRegion, sizeof(mRegion));
		navFile.load(&mCells, sizeof(mCells));
		navFile.close();
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
bool			NAV::TestEdge( TNodeHandle NodeA, TNodeHandle NodeB, qboolean IsDebugEdge )
{
	int			atHandle = mGraph.get_edge_across( NodeA, NodeB );
	CWayEdge&	at = mGraph.get_edge(atHandle);
	CWayNode&	a = mGraph.get_node(at.mNodeA);
	CWayNode&	b = mGraph.get_node(at.mNodeB);
	CVec3		Mins(-15.0f, -15.0f, 0.0f);		// These were the old "sizeless" defaults
	CVec3		Maxs(15.0f, 15.0f, 40.0f);
	bool		CanGo			= false;
	bool		HitCharacter	= false;
	int			EntHit			= ENTITYNUM_NONE;
	int			i				= (int)(at.Size());

	a.mPoint.ToStr(mLocStringA);
	b.mPoint.ToStr(mLocStringB);

	const char*	aName = (a.mName.empty())?(mLocStringA):(a.mName.c_str());
	const char*	bName = (b.mName.empty())?(mLocStringB):(b.mName.c_str());

	float		radius = (at.Size()==CWayEdge::WE_SIZE_LARGE)?(SC_LARGE_RADIUS):(SC_MEDIUM_RADIUS);
	float		height = (at.Size()==CWayEdge::WE_SIZE_LARGE)?(SC_LARGE_HEIGHT):(SC_MEDIUM_HEIGHT);

	Mins[0] = Mins[1] = (radius) * -1.0f;
	Maxs[0] = Maxs[1] = (radius);
	Maxs[2] =			(height);


	// If Either Start Or End Points Are Too Small, Don' Bother At This Size
	//-----------------------------------------------------------------------
	if ((a.mType==PT_WAYNODE && a.mRadius<radius) ||
		(b.mType==PT_WAYNODE && b.mRadius<radius))
	{
		if (IsDebugEdge)
		{
			gi.Printf("Nav(%s)<->(%s): Size Too Big\n", aName, bName, i);
		}
		CanGo = false;
		return CanGo;
	}


	// Try It
	//--------
	CanGo	= MoveTrace(a.mPoint, b.mPoint, Mins, Maxs, 0, true, false);
	EntHit	= mMoveTrace.entityNum;

	// Check For A Flying Edge
	//-------------------------
	if (a.mFlags.get_bit(CWayNode::WN_FLOATING) || b.mFlags.get_bit(CWayNode::WN_FLOATING))
	{
		at.mFlags.set_bit(CWayEdge::WE_FLYING);
		if (!a.mFlags.get_bit(CWayNode::WN_FLOATING) || !b.mFlags.get_bit(CWayNode::WN_FLOATING))
		{
			at.mFlags.set_bit(CWayEdge::WE_CANBEINVAL);
		}
	}


	// Well, It' Can't Go, But Possibly If We Hit An Entity And Remove That Entity, We Can Go?
	//-----------------------------------------------------------------------------------------
	if (!CanGo &&
		!mMoveTrace.startsolid &&
		EntHit!=ENTITYNUM_WORLD &&
		EntHit!=ENTITYNUM_NONE &&
		(&g_entities[EntHit])!=0)
	{
		gentity_t*	ent		= &g_entities[EntHit];

		if (IsDebugEdge)
		{
			gi.Printf("Nav(%s)<->(%s): Hit Entity Type (%s), TargetName (%s)\n", aName, bName, ent->classname, ent->targetname);
		}


		// Find Out What Type Of Entity This Is
		//--------------------------------------
		if (!Q_stricmp("func_door", ent->classname))
		{
			at.mFlags.set_bit(CWayEdge::WE_BLOCKING_DOOR);
		}
		else if (
			!Q_stricmp("func_wall", ent->classname) ||
			!Q_stricmp("func_static", ent->classname) ||
			!Q_stricmp("func_usable", ent->classname))
		{
			at.mFlags.set_bit(CWayEdge::WE_BLOCKING_WALL);
		}
		else if (
			!Q_stricmp("func_glass", ent->classname) ||
			!Q_stricmp("func_breakable", ent->classname) ||
			!Q_stricmp("misc_model_breakable", ent->classname))
		{
			at.mFlags.set_bit(CWayEdge::WE_BLOCKING_BREAK);
		}
		else if (ent->NPC || ent->s.number==0)
		{
			HitCharacter = true;
		}

		// Don't Care About Any Other Entity Types
		//-----------------------------------------
		else
		{
			if (IsDebugEdge)
			{
				gi.Printf("Nav(%s)<->(%s): Unable To Ignore Ent, Going A Size Down\n", aName, bName);
			}
			return CanGo;	// Go To Next Size Down
		}


		// If It Is A Door, Try Opening The Door To See If We Can Get In
		//---------------------------------------------------------------
		if (at.BlockingDoor())
		{
			// Find The Master
			//-----------------
			gentity_t *master = ent;
			while (master && master->teammaster && (master->flags&FL_TEAMSLAVE))
			{
				master = master->teammaster;
			}
			bool DoorIsStartOpen = master->spawnflags&1;

			// Open The Chain
			//----------------
			gentity_t *slave = master;
			while (slave)
			{
				VectorCopy((DoorIsStartOpen)?(slave->pos1):(slave->pos2), slave->currentOrigin);
				gi.linkentity(slave);
				slave = slave->teamchain;
			}

			// Try The Trace
			//---------------
			CanGo = MoveTrace(a.mPoint, b.mPoint, Mins, Maxs, 0, true, false);
			if (CanGo)
			{
				ent		= master;
				EntHit	= master->s.number;
			}
			else
			{
				if (IsDebugEdge)
				{
					gi.Printf("Nav(%s)<->(%s): Unable Pass Through Door Even When Open, Going A Size Down\n", aName, bName);
				}
			}

			// Close The Door
			//----------------
			slave = master;
			while (slave)
			{
				VectorCopy((DoorIsStartOpen)?(slave->pos2):(slave->pos1), slave->currentOrigin);
				gi.linkentity(slave);
				slave = slave->teamchain;
			}
		}

		// Assume Breakable Walls Will Be Clear Later
		//-----------------------------------------------------
		else if (at.BlockingBreakable())
		{//we'll do the trace again later if this ent gets broken
			CanGo = true;
		}

		// Otherwise, Try It, Pretending The Ent is Not There
		//----------------------------------------------------
		else
		{
			CanGo = MoveTrace(a.mPoint, b.mPoint, Mins, Maxs, EntHit, true, false);
			if (IsDebugEdge)
			{
				gi.Printf("Nav(%s)<->(%s): Unable Pass Through Even If Entity Was Gone, Going A Size Down\n", aName, bName);
			}
		}



		// If We Can Now Go, Ignoring The Entity, Remember That (But Don't Remember Characters - They Won't Stay Anyway)
		//---------------------------------------------------------------------------------------------------------------
		if (CanGo && !HitCharacter)
		{
			ent->wayedge		= atHandle;
			at.mEntityNum		= EntHit;
			at.mFlags.set_bit(CWayEdge::WE_CANBEINVAL);

			// Add It To The Edge Map
			//------------------------
			TEntEdgeMap::iterator eemiter = mEntEdgeMap.find(EntHit);
			if (eemiter==mEntEdgeMap.end())
			{
				TEdgesPerEnt	EdgesPerEnt;
				EdgesPerEnt.push_back(atHandle);
				mEntEdgeMap.insert(EntHit, EdgesPerEnt);
			}
			else
			{
				if (!eemiter->full())
				{
					eemiter->push_back(atHandle);
				}
				else
				{
#ifndef FINAL_BUILD
					assert("Max Edges Perh Handle Reached, Unable To Add To Edge Map!"==0);
					gi.Printf("WARNING: Too many nav edges pass through entity %d (%s)\n", EntHit, g_entities[EntHit].targetname);
#endif
				}
			}

			// Check For Special Conditions For The Different Types
			//-------------------------------------------------------
			if (at.BlockingDoor())
			{
				// Doors Need To Know Their "owner" Entity - The Thing That Controlls
				// When The Door Is Open (or can "auto open")
				//
				// We'll start by assuming the door is it's own master
				//-----------------------------------------------------
				at.mOwnerNum = ent->s.number;
				gentity_t*	owner = 0;

				// If There Is A Target Name, See If This Thing Is Controlled From A Switch Or Something
				//---------------------------------------------------------------------------------------
				if (ent->targetname)
				{
					// Try Target
					//------------
					owner = G_Find(owner, FOFS(target), ent->targetname);
					if (owner &&
						(!Q_stricmp("trigger_multiple", owner->classname) || !Q_stricmp("trigger_once", owner->classname)))
					{
						at.mOwnerNum = owner->s.number;
					}
					else
					{
						// Try Target2
						//-------------
						owner = G_Find(owner, FOFS(target2), ent->targetname);
						if (owner &&
							(!Q_stricmp("trigger_multiple", owner->classname) || !Q_stricmp("trigger_once", owner->classname)))
						{
							at.mOwnerNum = owner->s.number;
						}
					}
				}

				// Otherwise, See If There Is An Auto Door Opener Trigger
				//--------------------------------------------------------
				else
				{
					owner = G_FindDoorTrigger(ent);
					if (owner)
					{
						at.mOwnerNum = owner->s.number;
					}
				}
			}

			// Breakable Walls Are Not Valid Until Broken.  Period
			//-----------------------------------------------------
			else if (at.BlockingBreakable())
			{//we'll do the trace again later if this ent gets broken
				at.mFlags.clear_bit(CWayEdge::WE_VALID);
			}
		}
	}

	// Now Search For Any Holes In The Ground
	//----------------------------------------
	if (CHECK_JUMP && CanGo)
	{
		CVec3	Mins(-15.0f, -15.0f, 0.0f);		// These were the old "sizeless" defaults
		CVec3	Maxs(15.0f, 15.0f, 40.0f);

		CVec3	AtoB(b.mPoint - a.mPoint);
		float	AtoBDist = AtoB.SafeNorm();
		int		AtoBSegs = (AtoBDist / MAX_EDGE_SEG_LEN);

		AtoB *= MAX_EDGE_SEG_LEN;
		CVec3	Start(a.mPoint);
		CVec3	Stop;

		for (int curSeg=1; (curSeg<AtoBSegs && CanGo); curSeg++)
		{
			Start	+= AtoB;
			Stop	= Start;
			Stop[2] -= MAX_EDGE_FLOOR_DIST;

			CanGo = !MoveTrace(Start, Stop, Mins, Maxs, EntHit, true, false);
		}
	}

	return CanGo;
}
////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
bool			NAV::LoadFromEntitiesAndSaveToFile(const char *filename, int checksum)
{
	if (mGraph.size_nodes()<=3)
	{
		return true;
	}
	mUser.ClearActor();

	mMoveTraceCount = 0;
	mViewTraceCount = 0;
	mConnectTraceCount = 0;
	mConnectTime = gi.Milliseconds();

	// PHASE 0: SCAN ALL ENTITIES AND TEMPORARLY CLOSE / TURN THEM ON
	//================================================================
	TEntBits				Doors;
	TEntBits				Walls;
	TEntBits				NPCs;
	if (CHECK_START_OPEN)
	{
		for (int curEnt=0; curEnt<MAX_GENTITIES; curEnt++)
		{
			gentity_t*	ent		= &g_entities[curEnt];

			if (!ent || !ent->inuse)
			{
				continue;
			}

			// Is It An NPC or Vehicle?
			//--------------------------------------
			if (ent->NPC  || (ent->NPC_type && Q_stricmp(ent->NPC_type, "atst")==0))
			{
				NPCs.set_bit(curEnt);
				ent->lastMoveTime = ent->contents;
				ent->contents = 0;
				gi.linkentity(ent);
			}



			// Is This Ent "Start Off" or "Start Open"?
			//------------------------------------------
			if (!(ent->spawnflags&1))
			{
				continue;
			}


			// Is It A Door?  Then Close It
			//--------------------------------------
			if (!Q_stricmp("func_door", ent->classname))
			{
				Doors.set_bit(curEnt);
				VectorCopy(ent->pos2, ent->currentOrigin);
				gi.linkentity(ent);
			}

			// Is It A Wall?  Then Turn It On
			//--------------------------------------
			else if (!Q_stricmp("func_wall", ent->classname) ||	!Q_stricmp("func_usable", ent->classname))
			{
				Walls.set_bit(curEnt);
				ent->contents = ent->spawnContents;
				gi.linkentity(ent);
			}
		}
	}


	// PHASE I: TRIANGULATE ALL NODES IN THE GRAPH
	//=============================================
/*	TGraphTriang	*Triang = new TGraphTriang(mGraph);
	Triang->clear();
	Triang->insertion_hull();
	Triang->delaunay_edge_flip();
	Triang->floor_shape(mUser, 1000.0f);
	Triang->alpha_shape(mUser, 1000.0f);
	Triang->finish(mUser);
	delete Triang;
*/
	mCells.fill_cells_nodes(NAV::CELL_RANGE);






	// PHASE II: SCAN THROUGH EXISTING NODES AND GENERATE EDGES TO OTHER NODES WAY POINTS
	//====================================================================================
	CWayNode*		at;
	int				atHandle;
#ifdef _DEBUG
	const char*		atNameStr;
#endif // _DEBUG
	int				tgtNum;
	int				tgtHandle;
	hstring			tgtName;
#ifdef _DEBUG
	const char*		tgtNameStr;
#endif // _DEBUG

	CWayEdge		atToTgt;
	CVec3			atFloor;
	CVec3			atRoof;
	bool			atOnFloor;


	TNameToNodeMap::iterator	nameFinder;
	TGraph::TNodes::iterator	nodeIter;
	TGraph::TEdges::iterator	edgeIter;

	ratl::ratl_compare			closestNbrs[MIN_WAY_NEIGHBORS];

	// Drop To Floor And Mark Floating
	//---------------------------------
	for (nodeIter=mGraph.nodes_begin(); nodeIter!=mGraph.nodes_end(); nodeIter++)
	{
		at				= &(*nodeIter);
		atRoof			= at->mPoint;
		atFloor			= at->mPoint;
		if (at->mFlags.get_bit(CWayNode::WN_DROPTOFLOOR))
		{
			atFloor[2]	-= MAX_EDGE_FLOOR_DIST;
		}
		atFloor[2]		-= (MAX_EDGE_FLOOR_DIST * 1.5f);
		atOnFloor		= !ViewTrace(atRoof, atFloor);
		if (at->mFlags.get_bit(CWayNode::WN_DROPTOFLOOR))
		{
			at->mPoint		= mViewTrace.endpos;
			at->mPoint[2]	+= 5.0f;
		}
		else if (!atOnFloor && (at->mType==PT_WAYNODE || at->mType==PT_GOALNODE))
		{
			at->mFlags.set_bit(CWayNode::WN_FLOATING);
		}
	}


	for (nodeIter=mGraph.nodes_begin(); nodeIter!=mGraph.nodes_end(); nodeIter++)
	{
		nodeIter->mPoint.ToStr(mLocStringA);

		at				= &(*nodeIter);
		atHandle		= (nodeIter.index());
#ifdef _DEBUG
		atNameStr		= (at->mName.empty())?(mLocStringA):(at->mName.c_str());
#endif // _DEBUG

		// Connect To Hand Designed Targets
		//----------------------------------
		for (tgtNum=0; tgtNum<NAV::NUM_TARGETS; tgtNum++)
		{
			// Does This Target Name Exist (or is it "")?
			//--------------------------------------------
			tgtName		= at->mTargets[tgtNum];
			if (!tgtName || tgtName.empty())
			{
				continue;
			}
#ifdef _DEBUG
			tgtNameStr	= tgtName.c_str();
#endif // _DEBUG

			// Clear The Name In The Array, So Save Is Not Corrupted
			//-------------------------------------------------------
			at->mTargets[tgtNum] = 0;


			// Try To Find The Node This Target Name Refers To
			//-------------------------------------------------
			nameFinder	= mNodeNames.find(tgtName);
			if (nameFinder==mNodeNames.end())
			{
#ifdef _DEBUG
				gi.Printf( S_COLOR_YELLOW "WARNING: nav unable to locate target (%s) from node (%s)\n", tgtNameStr,
					atNameStr );
#endif // _DEBUG
				continue;
			}

			// For Each One
			//--------------
			for (int tgtNameIndex=0; tgtNameIndex<(*nameFinder).size(); tgtNameIndex++)
			{
				// Connect The Two Nodes In The Graph
				//------------------------------------
				tgtHandle	= (*nameFinder)[tgtNameIndex];

				// Only If The Target Is NOT The Same As The Source
				//--------------------------------------------------
				if (atHandle!=tgtHandle)
				{
					int edge = mGraph.get_edge_across(atHandle, tgtHandle);

					// If The Edge Already Exists, Just Make Sure To Add Any Flags
					//-------------------------------------------------------------
					if (edge)
					{
						// If It Is The Jump Edge (Last Target), Mark It
						//-----------------------------------------------
						if (tgtNum == (NAV::NUM_TARGETS-1))
						{
							mGraph.get_edge(edge).mFlags.set_bit(CWayEdge::WE_JUMPING);
						}
						mGraph.get_edge(edge).mFlags.set_bit(CWayEdge::WE_DESIGNERPLACED);
						continue;
					}

					// Setup The Edge
					//----------------
					mUser.setup_edge(atToTgt, atHandle, tgtHandle, false, mGraph.get_node(atHandle), mGraph.get_node(tgtHandle), false);
					atToTgt.mFlags.set_bit(CWayEdge::WE_DESIGNERPLACED);

					// If It Is The Jump Edge (Last Target), Mark It
					//-----------------------------------------------
					if (tgtNum == (NAV::NUM_TARGETS-1))
					{
						atToTgt.mFlags.set_bit(CWayEdge::WE_JUMPING);
					}

					// Now Tell The Graph Which Edge Index To Store Between The Two Points
					//---------------------------------------------------------------------
					mGraph.connect_node(atToTgt, atHandle, tgtHandle);
				}
			}
		}


		// If It Is A Combat Or Goal Nav, Try To "Auto Connect" To A Few Nearby Way Points
		//---------------------------------------------------------------------------------
		if (!at->mFlags.get_bit(CWayNode::WN_NOAUTOCONNECT) &&
			(at->mType==NAV::PT_COMBATNODE || at->mType==NAV::PT_GOALNODE))
		{
			// Get The List Of Nodes For This Cell Of The Map
			//------------------------------------------------
			TGraphCells::SCell&			Cell = mCells.get_cell(at->mPoint[0], at->mPoint[1]);

			// Create A Closest Neighbors Array And Initialize It Empty
			//----------------------------------------------------------
			for (int i=0; i<MIN_WAY_NEIGHBORS; i++)
			{
				closestNbrs[i].mHandle = 0;
				closestNbrs[i].mCost = 0;
			}
			int highestCost=0;
			int nonWPCount=0;

			for (int cellNode=0; cellNode<Cell.mNodes.size(); cellNode++)
			{
				tgtHandle = Cell.mNodes[cellNode];

				// If It Is The Same Node, Or We Already Connect To It, Ignore
				//-------------------------------------------------------------
				if (tgtHandle==atHandle || mGraph.get_edge_across(atHandle, tgtHandle))
				{
					continue;
				}

				// If The Target Is Another Combat Point Or Goal Node, Ignore It
				//--------------------------------------------------------------
				CWayNode&	node = mGraph.get_node(tgtHandle);


				// Ignore Ones That Are A Floor Above Or Below
				//---------------------------------------------
				if (fabsf(node.mPoint[2] - at->mPoint[2])>NAV::MAX_EDGE_FLOOR_DIST)
				{
					continue;
				}

				// Ignore Ones That Are Too Far
				//------------------------------
				float cost = node.mPoint.Dist(at->mPoint);
				if (cost>NAV::MAX_EDGE_AUTO_LEN)
				{
					continue;
				}

				// Connecting To Another Combat Point Or Goal Node It Must Be Half The Max Connect Distance
				//------------------------------------------------------------------------------------------
				if (node.mType==NAV::PT_COMBATNODE || node.mType==NAV::PT_GOALNODE)
				{
					nonWPCount++;
					if (nonWPCount>NAV::MAX_NONWP_NEIGHBORS)
					{
						continue;
					}

					if (cost>(NAV::MAX_EDGE_AUTO_LEN/2.0f))
					{
						continue;
					}
				}

				// If We Already Have Points, Ignore Anything Farther Than The Current Farthest
				//------------------------------------------------------------------------------
				if (closestNbrs[highestCost].mHandle!=0 && closestNbrs[highestCost].mCost<cost)
				{
					continue;
				}

				// Ignore Anything Not In The PVS
				//--------------------------------
				if (!(gi.inPVS(node.mPoint.v, at->mPoint.v)))
				{
					continue;
				}


				// Now Record This Point Over The One With The Highest Cost
				//----------------------------------------------------------
				closestNbrs[highestCost].mHandle	= tgtHandle;
				closestNbrs[highestCost].mCost		= node.mPoint.Dist(at->mPoint);

				// Find The New Highest Cost
				//---------------------------
				for (int i=0; i<MIN_WAY_NEIGHBORS; i++)
				{
					if (closestNbrs[i].mHandle==0)
					{
						highestCost = i;
						break;
					}

					if (closestNbrs[i].mCost>closestNbrs[highestCost].mCost)
					{
						highestCost = i;
					}
				}
			}

			// Now Connect All The Closest Neighbors
			//---------------------------------------
			for (int i=0; i<MIN_WAY_NEIGHBORS; i++)
			{
				if (closestNbrs[i].mHandle)
				{
					mUser.setup_edge(atToTgt, atHandle, closestNbrs[i].mHandle, false, (*nodeIter), mGraph.get_node(closestNbrs[i].mHandle), false);
					mGraph.connect_node(atToTgt, atHandle, closestNbrs[i].mHandle);
				}
			}
		}
	}





	// PHASE III: SCAN EDGES AND RUN TRACES FOR VALID CONNECTIONS & DOORS
	//==================================================================
	ratl::vector_vs<int, NAV::NUM_EDGES>	*ToBeRemoved = new ratl::vector_vs<int, NAV::NUM_EDGES>;
	for (edgeIter=mGraph.edges_begin(); edgeIter!=mGraph.edges_end(); edgeIter++)
	{
		CWayEdge& at		= (*edgeIter);
		int		  atHandle	= edgeIter.index();
		CWayNode& a			= mGraph.get_node(at.mNodeA);
		CWayNode& b			= mGraph.get_node(at.mNodeB);

		mGraph.get_node(at.mNodeA).mPoint.ToStr(mLocStringA);
		mGraph.get_node(at.mNodeB).mPoint.ToStr(mLocStringB);

		const char*	aName = (a.mName.empty())?(mLocStringA):(a.mName.c_str());
		const char*	bName = (b.mName.empty())?(mLocStringB):(b.mName.c_str());


		if (at.mFlags.get_bit(CWayEdge::WE_JUMPING))
		{
			at.mFlags.set_bit(CWayEdge::WE_SIZE_LARGE);
			at.mFlags.set_bit(CWayEdge::WE_CANBEINVAL);
			at.mFlags.set_bit(CWayEdge::WE_DESIGNERPLACED);
			continue;
		}


		// Cycle through the different sizes, starting with the largest
		//--------------------------------------------------------------
		bool	CanGo		= false;
		bool	IsDebugEdge =
			(g_nav1->string[0] && g_nav2->string[0] &&
			(!Q_stricmp(*(a.mName), g_nav1->string) || !Q_stricmp(*(b.mName), g_nav1->string)) &&
			(!Q_stricmp(*(a.mName), g_nav2->string) || !Q_stricmp(*(b.mName), g_nav2->string)));

		// For debugging a connection between two known points:
		//------------------------------------------------------
		if (IsDebugEdge)
		{
			gi.Printf("===============================\n");
			gi.Printf("Nav(%s)<->(%s): DEBUGGING START\n", aName, bName);
			assert(0);	// Break Here
		}

		// Try Large
		//-----------
		at.mFlags.set_bit(CWayEdge::WE_SIZE_LARGE);
		if (IsDebugEdge)
		{
			gi.Printf("Nav(%s)<->(%s): Attempting Size Large...\n", aName, bName);
		}

		// Try Medium
		//------------
		CanGo = TestEdge( at.mNodeA, at.mNodeB, IsDebugEdge );
		if (!CanGo)
		{
			at.mFlags.clear_bit(CWayEdge::WE_SIZE_LARGE);
			at.mFlags.set_bit(CWayEdge::WE_SIZE_MEDIUM);
			if (IsDebugEdge)
			{
				gi.Printf("Nav(%s)<->(%s): Attempting Size Medium...\n", aName, bName);
			}
			CanGo = TestEdge( at.mNodeA, at.mNodeB, IsDebugEdge );
		}

		// If This Edge Can't Go At Any Size, Dump It
		//--------------------------------------------
		if (!CanGo)
		{
			ToBeRemoved->push_back(atHandle);
			if (IsDebugEdge)
			{
				CVec3	ContactNormal(mMoveTrace.plane.normal);
				CVec3	ContactPoint( mMoveTrace.endpos);

				char	cpointstr[256] = {0};
				char	cnormstr[256] = {0};

				ContactNormal.ToStr(cnormstr);
				ContactPoint.ToStr(cpointstr);

				gi.Printf("Nav(%s)<->(%s): FAILED, NO SMALLER SIZE POSSIBLE\n", aName, bName);
				gi.Printf("Nav(%s)<->(%s): The last trace hit:\n", aName, bName);
				gi.Printf("Nav(%s)<->(%s):     at %s,\n", aName, bName, cpointstr);
				gi.Printf("Nav(%s)<->(%s):     normal %s\n", aName, bName, cnormstr);
				if (mMoveTrace.entityNum!=ENTITYNUM_WORLD)
				{
					gentity_t*	ent		= &g_entities[mMoveTrace.entityNum];
					gi.Printf("Nav(%s)<->(%s):     on entity Type (%s), TargetName (%s)\n", aName, bName, ent->classname, ent->targetname);
				}
				if ((mMoveTrace.contents)&CONTENTS_MONSTERCLIP)
				{
					gi.Printf("Nav(%s)<->(%s):     with contents BLOCKNPC\n", aName, bName);
				}
				else if ((mMoveTrace.contents)&CONTENTS_BOTCLIP)
				{
					gi.Printf("Nav(%s)<->(%s):     with contents DONOTENTER\n", aName, bName);
				}
				else if ((mMoveTrace.contents)&CONTENTS_SOLID)
				{
					gi.Printf("Nav(%s)<->(%s):     with contents SOLID\n", aName, bName);
				}
				else if ((mMoveTrace.contents)&CONTENTS_WATER)
				{
					gi.Printf("Nav(%s)<->(%s):     with contents WATER\n", aName, bName);
				}
			}
		}
		else
		{
			if (IsDebugEdge)
			{
				gi.Printf("Nav(%s)<->(%s): Success!\n", aName, bName);
			}
		}

		if (IsDebugEdge)
		{
			gi.Printf("Nav(%s)<->(%s): DEBUGGING END\n", aName, bName);
			gi.Printf("===============================\n");
		}
	}

	// Now Go Ahead And Remove Dead Edges
	//------------------------------------
	for (int RemIndex=0;RemIndex<ToBeRemoved->size(); RemIndex++)
	{
		CWayEdge& at = mGraph.get_edge((*ToBeRemoved)[RemIndex]);
		if (at.mFlags.get_bit(CWayEdge::WE_DESIGNERPLACED))
		{
#ifdef _DEBUG
			hstring		aHstr = mGraph.get_node(at.mNodeA).mName;
			hstring		bHstr = mGraph.get_node(at.mNodeB).mName;
#endif // _DEBUG

			mGraph.get_node(at.mNodeA).mPoint.ToStr(mLocStringA);
			mGraph.get_node(at.mNodeB).mPoint.ToStr(mLocStringB);

#ifdef _DEBUG
			gi.Printf( S_COLOR_RED "ERROR: Nav connect failed: %s@%s <-> %s@%s\n", aHstr.c_str(), mLocStringA,
				bHstr.c_str(), mLocStringB );
#endif // _DEBUG
			delayedShutDown = level.time + 100;
		}
		mGraph.remove_edge(at.mNodeA, at.mNodeB);
	}

	delete ToBeRemoved;

	// Detect Point Islands
	//----------------------
	for (nodeIter=mGraph.nodes_begin(); nodeIter!=mGraph.nodes_end(); nodeIter++)
	{
		at				= &(*nodeIter);
		atHandle		= nodeIter.index();

		if (!mGraph.node_has_neighbors(atHandle))
		{
			at->mPoint.ToStr(mLocStringA);

			at->mFlags.set_bit(CWayNode::WN_ISLAND);
			mIslandCount++;
			if (at->mType==NAV::PT_COMBATNODE)
			{
#ifndef FINAL_BUILD
				gi.Printf( S_COLOR_RED"ERROR: Combat Point %s@%s Is Not Connected To Anything\n", at->mName.c_str(), mLocStringA);
				delayedShutDown = level.time + 100;
#endif
			}
			if (at->mType==NAV::PT_GOALNODE)
			{
				// Try To Trace Down, If We Don't Hit Any Ground, Assume This Is An "Air Point"
				//------------------------------------------------------------------------------
				CVec3 Down(at->mPoint);
				Down[2] -= 100;
				if (!ViewTrace(at->mPoint, Down))
				{
#ifndef FINAL_BUILD
					gi.Printf( S_COLOR_RED"ERROR: Nav Goal %s@%s Is Not Connected To Anything\n", at->mName.c_str(), mLocStringA);
					delayedShutDown = level.time + 100;
#endif
				}
			}
		}
	}



	// PHASE IV: SCAN EDGES FOR REGIONS
	//==================================
	mRegion.clear();

	mIslandRegion	= mRegion.reserve();
//	mAirRegion		= mRegion.reserve();
	for (nodeIter=mGraph.nodes_begin(); nodeIter!=mGraph.nodes_end(); nodeIter++)
	{
		at				= &(*nodeIter);
		if (at->mFlags.get_bit(CWayNode::WN_ISLAND))
		{
			mRegion.assign_region(nodeIter.index(), mIslandRegion);
		}
	//	else if (at->mFlags.get_bit(CWayNode::WN_FLOATING))
	//	{
	//		mRegion.assign_region(nodeIter.index(), mAirRegion);
	//	}
	}
	if (!mRegion.find_regions(mUser))
	{
#ifndef FINAL_BUILD
		gi.Printf( S_COLOR_RED"ERROR: Too Many Regions!\n");
		delayedShutDown = level.time + 100;
#endif
	}
	if (!mRegion.find_region_edges())
	{
#ifndef FINAL_BUILD
		gi.Printf( S_COLOR_RED"ERROR: Too Many Region Edges!\n");
		delayedShutDown = level.time + 100;
#endif
	}


	// PHASE V: SCAN NODES AND FILL CELLS
	//===================================
	mCells.fill_cells_edges(NAV::CELL_RANGE);


	// PHASE VI: SCAN ALL ENTITIES AND RE OPEN / TURN THEM OFF
	//=========================================================
	if (CHECK_START_OPEN)
	{
		for (int curEnt=0; curEnt<MAX_GENTITIES; curEnt++)
		{
			// Is This Ent "Start Off" or "Start Open"?
			//------------------------------------------
			gentity_t*	ent		= &g_entities[curEnt];
			if (!ent || !ent->inuse)
			{
				continue;
			}


			// Is It A Door?
			//--------------------------------------
			if (Doors.get_bit(curEnt))
			{
				VectorCopy(ent->pos1, ent->currentOrigin);
				gi.linkentity(ent);
			}

			// Is It A Wall?
			//--------------------------------------
			else if (Walls.get_bit(curEnt))
			{
				ent->contents = 0;
				gi.linkentity(ent);
			}

			// Is It An NPC?
			//--------------------------------------
			else if (NPCs.get_bit(curEnt))
			{
				ent->contents = ent->lastMoveTime;
				ent->lastMoveTime = 0;
				gi.linkentity(ent);
			}


		}
	}
	mConnectTraceCount = mMoveTraceCount;
	mMoveTraceCount = 0;

	mConnectTime = gi.Milliseconds() - mConnectTime;


	// PHASE VI: SAVE TO FILE
	//========================
	if (SAVE_LOAD)
	{
		hfile	navFile(va("maps/%s.navNEW"));
		if (!navFile.open_write(NAV_VERSION, checksum))
		{
			return false;
		}
		navFile.save(&mGraph, sizeof(mGraph));
		navFile.save(&mRegion, sizeof(mRegion));
		navFile.save(&mCells, sizeof(mCells));
		navFile.close();
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
void			NAV::DecayDangerSenses()
{
	float	PerFrameDecay = (50.0f) / (float)(NAV::MAX_ALERT_TIME);
	for (int entIndex=0; entIndex<TEntityAlertList::CAPACITY; entIndex++)
	{
		TAlertList&	ae = mEntityAlertList[entIndex];
		for (int alertIndex=0; alertIndex<TAlertList::CAPACITY; alertIndex++)
		{
			if (ae[alertIndex].mHandle!=0)
			{
				ae[alertIndex].mDanger -= PerFrameDecay;

				// If It Just Decayed To Nothing, Clear It
				//-----------------------------------------
				if (ae[alertIndex].mDanger<=0.0f)
				{
					ae[alertIndex].mHandle = 0;
					ae[alertIndex].mDanger = 0;
				}
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
void			NAV::RegisterDangerSense(gentity_t* actor, int alertEventIndex)
{
	if (actor==0 || alertEventIndex<0)
	{
		assert(0);
		return;
	}

	//If there are no waypoints, don't bother trying to register danger on them
	if (mGraph.size_nodes()<1)
	{
		return;
	}

	// Get The Alert List For This Ent And The Alert Itself
	//------------------------------------------------------
	TAlertList&		al = mEntityAlertList[actor->s.number];
	alertEvent_t&	ae = level.alertEvents[alertEventIndex];

	if (ae.radius<=0.0f)
	{
		return;
	}

	// DEBUG GRAPHICS
	//=====================================================
	if (NAVDEBUG_showRadius)
	{
		CG_DrawRadius(ae.position, ae.radius, NODE_GOAL);
	}
	//=====================================================


	CVec3	DangerPoint(ae.position);

	// Look Through The Nearby Edges And Record Any That Are Affected
	//----------------------------------------------------------------
	TGraphCells::TCellNodes& cellEdges = mCells.get_cell(DangerPoint[0], DangerPoint[1]).mEdges;
	for (int cellEdgeIndex=0; cellEdgeIndex<cellEdges.size(); cellEdgeIndex++)
	{
		int			edgeHandle	= cellEdges[cellEdgeIndex];
		CWayEdge&	edge		= mGraph.get_edge(edgeHandle);
		float		edgeDanger	= ((ae.radius - DangerPoint.DistToLine(edge.PointA(), edge.PointB()))/(ae.radius));
		if (edgeDanger>0.0f)
		{

			// Record The Square, So That Danger Drops Off Quadradically Rather Than Linearly
			//--------------------------------------------------------------------------------
			edgeDanger *= edgeDanger;


			// Now Find The Index We Will "Replace" With This New Information
			//----------------------------------------------------------------
			int	replaceIndex = -1;
			for (int alIndex=0; alIndex<TAlertList::CAPACITY; alIndex++)
			{

				// If It's The Same Edge Handle, Then Replace This One!
				//------------------------------------------------------
				if (al[alIndex].mHandle==edgeHandle || al[alIndex].mHandle==0)
				{
					replaceIndex = alIndex;
					break;
				}

				// Otherwise, Remember The Least Dangerous One
				//---------------------------------------------
				if (replaceIndex==-1 || al[alIndex].mDanger<al[replaceIndex].mDanger)
				{
					replaceIndex = alIndex;
				}
			}

			// Now Go Ahead And Record It
			//----------------------------
			al[replaceIndex].mHandle = edgeHandle;
			al[replaceIndex].mDanger = edgeDanger;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
void			NAV::WayEdgesNowClear(gentity_t* ent)
{
	if (ent)
	{
		ent->wayedge = 0;

		int EdgeHandle;
		int	EntNum = ent->s.number;

		TEntEdgeMap::iterator finder = mEntEdgeMap.find(EntNum);
		if (finder!=mEntEdgeMap.end())
		{
			for (int i=0; i<finder->size(); i++)
			{
				EdgeHandle = (*finder)[i];
				if (EdgeHandle!=0)
				{
					CWayEdge& edge = mGraph.get_edge(EdgeHandle);
					edge.mFlags.set_bit(CWayEdge::WE_VALID);
					edge.mEntityNum = ENTITYNUM_NONE;
					edge.mOwnerNum = ENTITYNUM_NONE;
				}
			}
			mEntEdgeMap.erase(EntNum);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
void			NAV::SpawnedPoint(gentity_t* ent, NAV::EPointType type)
{
	if (mGraph.size_nodes()>=NUM_NODES)
	{
#ifndef FINAL_BUILD
		gi.Printf( "SpawnedPoint: Max Nav points reached (%d)!\n",NUM_NODES );
#endif
        return;
	}

	CVec3 Mins;
	CVec3 Maxs;


	Mins[0] = Mins[1] = (SC_MEDIUM_RADIUS) * -1.0f;
	Maxs[0] = Maxs[1] = (SC_MEDIUM_RADIUS);
	Mins[2] = 0.0f;
	Maxs[2] = SC_MEDIUM_HEIGHT;


	CVec3 Start(ent->currentOrigin);
	CVec3 Stop(ent->currentOrigin);
	Stop[2] += 5.0f;

	Start.ToStr(mLocStringA);
	const char* pointName = (ent->targetname && ent->targetname[0])?(ent->targetname):"?";

	if (CHECK_START_SOLID)
	{
		// Try It
		//--------
		if (!MoveTrace(Start, Stop, Mins, Maxs, 0, true, false))
		{
			assert("ERROR: Nav in solid!"==0);
			gi.Printf( S_COLOR_RED"ERROR: Nav(%d) in solid: %s@%s\n", type, pointName, mLocStringA);
			delayedShutDown = level.time + 100;
			return;
		}
	}

	CWayNode	node;

	node.mPoint			= ent->currentOrigin;
	node.mRadius		= ent->radius;
	node.mType			= type;
	node.mFlags.clear();
	if (type==NAV::PT_WAYNODE && (ent->spawnflags & 2))
	{
		node.mFlags.set_bit(CWayNode::WN_DROPTOFLOOR);
	}
	if (ent->spawnflags & 4)
	{
		node.mFlags.set_bit(CWayNode::WN_NOAUTOCONNECT);
	}

	// TO AVOID PROBLEMS WITH THE TRIANGULATION, WE MOVE THE POINTS AROUND JUST A BIT
	//================================================================================
	//node.mPoint		+= CVec3(Q_flrand(-RANDOM_PERMUTE, RANDOM_PERMUTE), Q_flrand(-RANDOM_PERMUTE, RANDOM_PERMUTE), 0.0f);
	//================================================================================

	// Validate That The New Location Is Still Safe
	//----------------------------------------------
	if (false && CHECK_START_SOLID)
	{
		Start = (node.mPoint);
		Stop = (node.mPoint);
		Stop[2] += 5.0f;

		// Try It Again
		//--------------
		if (!MoveTrace(Start, Stop, Mins, Maxs, 0, true, false))
		{
			gi.Printf( S_COLOR_YELLOW"WARNING: Nav Moved To Solid, Resetting: (%s)\n", pointName);
			assert("WARNING: Nav Moved To Solid, Resetting!"==0);
			node.mPoint		= ent->currentOrigin;
		}
	}

	node.mTargets[0]	= ent->target;
	node.mTargets[1]	= ent->target2;
	node.mTargets[2]	= ent->target3;
	node.mTargets[3]	= ent->target4;
	node.mTargets[4]	= ent->targetJump;
	node.mName			= ent->targetname;

	int		NodeHandle	= mGraph.insert_node(node);

	ent->waypoint		= NodeHandle;

	mCells.expand_bounds(NodeHandle);

	if (!node.mName.empty())
	{
		TNameToNodeMap::iterator		nameFinder	= mNodeNames.find(node.mName);

		if (nameFinder==mNodeNames.end())
		{
			TNamedNodeList	list;
			list.clear();
			list.push_back(NodeHandle);
			mNodeNames.insert(node.mName, list);
		}
		else
		{
			(*nameFinder).push_back(NodeHandle);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
NAV::TNodeHandle		NAV::GetNearestNode(gentity_t* ent, bool forceRecalcNow, NAV::TNodeHandle goal)
{
	if (!ent)
	{
		return 0;
	}

	if (ent->waypoint==WAYPOINT_NONE || forceRecalcNow || (level.time>ent->noWaypointTime))
	{
		if (ent->waypoint)
		{
			ent->lastWaypoint	= ent->waypoint;
		}
		ent->waypoint		=
			GetNearestNode(
			ent->currentOrigin,
			ent->waypoint,
			goal,
			ent->s.number,
			(ent->client && ent->client->moveType==MT_FLYSWIM));
		ent->noWaypointTime = level.time + 1000;	// Don't Erase This Result For 5 Seconds
	}

	return ent->waypoint;
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
NAV::TNodeHandle		NAV::GetNearestNode(const vec3_t& position, NAV::TNodeHandle previous, NAV::TNodeHandle goal, int ignoreEnt, bool allowZOffset)
{
	if (mGraph.size_edges()>0)
	{
		// Get The List Of Nodes For This Cell Of The Map
		//------------------------------------------------
		TGraphCells::SCell&			Cell = mCells.get_cell(position[0], position[1]);
		if (Cell.mNodes.empty() && Cell.mEdges.empty())
		{
#ifndef FINAL_BUILD
			if (g_developer->value)
			{
				gi.Printf("WARNING: Failure To Find A Node Here, Examine Cell Layout\n");
			}
#endif
			return WAYPOINT_NONE;
		}

		CVec3		Pos(position);
		SNodeSort	NodeSort;

		// PHASE I - TEST NAV POINTS
		//===========================
		{
			mNearestNavSort.clear();
			for (int i=0; i<Cell.mNodes.size() && !mNearestNavSort.full(); i++)
			{
				CWayNode&	node = mGraph.get_node(Cell.mNodes[i]);

				NodeSort.mHandle	= Cell.mNodes[i];
				NodeSort.mDistance	= node.mPoint.Dist2(Pos);
				NodeSort.mInRadius	= (NodeSort.mDistance<(node.mRadius*node.mRadius));

				// Severly Bias Points That Are Not On The Same Z Height As The Pos
				//------------------------------------------------------------------
				if (!allowZOffset)
				{
					float	ZOff = fabsf(node.mPoint[2] - Pos[2]);
					if (ZOff>(VIEW_RANGE / 4))
					{
						continue;
					}
					if (ZOff>30.0f)
					{
						NodeSort.mDistance += (ZOff*ZOff);
					}
				}

				// Ignore Points That Are Too Far
				//--------------------------------
				if (NodeSort.mDistance>(NAV::VIEW_RANGE*NAV::VIEW_RANGE))
				{
					continue;
				}

				// Bias Points That Are Not Connected To Anything
				//------------------------------------------------
				if (node.mFlags.get_bit(CWayNode::WN_ISLAND))
				{
					NodeSort.mDistance *= 3.0f;
				}

				if (previous && previous!=NodeSort.mHandle && !NAV::InSameRegion(previous, NodeSort.mHandle))
				{
					NodeSort.mDistance += (100.0f*100.0f);
				}
				if (previous>0 && previous!=NodeSort.mHandle && !mGraph.get_edge_across(previous, NodeSort.mHandle))
				{
					NodeSort.mDistance += (200.0f*200.0f);
				}
				if (goal && goal!=NodeSort.mHandle && !NAV::InSameRegion(goal, NodeSort.mHandle))
				{
					NodeSort.mDistance += (300.0f*300.0f);
				}


				// Bias Combat And Goal Nodes Some
				//---------------------------------
			//	if (node.mType==NAV::PT_COMBATNODE || node.mType==NAV::PT_GOALNODE)
			//	{
			//		NodeSort.mDistance += 50.0f;
			//	}

				mNearestNavSort.push_back(NodeSort);
			}

			// Sort Them By Distance
			//-----------------------
			mNearestNavSort.sort();

			// Now , Run Through Each Of The Sorted Nodes, Starting With The Closest One
			//---------------------------------------------------------------------------
			for (int j=0; j<mNearestNavSort.size(); j++)
			{
				// If In The Radius Of This Point, It Is Safe To Return It
				//---------------------------------------------------------
				if (mNearestNavSort[j].mInRadius)
				{
					return mNearestNavSort[j].mHandle;
				}

				// Otherwise, We Need To Trace To It
				//-----------------------------------
				else if (ViewNavTrace(Pos, mGraph.get_node(mNearestNavSort[j].mHandle).mPoint))
				{
					return mNearestNavSort[j].mHandle;
				}
			}
		}

		// PHASE II: TEST NAV EDGES
		//==========================
		{
			CVec3	Point;
			CVec3	PointOnEdge;
			float	PointOnEdgeRange;
			mNearestNavSort.clear();
			for (int i=0; i<Cell.mEdges.size() && !mNearestNavSort.full(); i++)
			{
				CWayEdge&	edge = mGraph.get_edge(Cell.mEdges[i]);

				edge.Point(Point);

				NodeSort.mHandle	= Cell.mEdges[i];
				NodeSort.mDistance	= Point.Dist2(Pos);

				// Severly Bias Points That Are Not On The Same Z Height As The Pos
				//------------------------------------------------------------------
				if (!allowZOffset)
				{
					float	ZOff = fabsf(Point[2] - Pos[2]);
					if (ZOff>(VIEW_RANGE / 4))
					{
						continue;
					}
					if (ZOff>30.0f)
					{
						NodeSort.mDistance += (ZOff*ZOff);
					}
				}

				// Ignore Points That Are Too Far
				//--------------------------------
				if (NodeSort.mDistance>(NAV::VIEW_RANGE*NAV::VIEW_RANGE))
				{
					continue;
				}
				mNearestNavSort.push_back(NodeSort);
			}

			// Sort Them By Distance
			//-----------------------
			mNearestNavSort.sort();

			// Now , Run Through Each Of The Sorted Edges, Starting With The Closest One
			//---------------------------------------------------------------------------
			for (int j=0; j<mNearestNavSort.size(); j++)
			{
				CWayEdge&	edge = mGraph.get_edge(mNearestNavSort[j].mHandle);
				PointOnEdge = Pos;

				PointOnEdgeRange = PointOnEdge.ProjectToLine(edge.PointA(), edge.PointB());

				if (PointOnEdgeRange>0.0f && PointOnEdgeRange<1.0f)
				{
					// Otherwise, We Need To Trace To It
					//-----------------------------------
					if (ViewNavTrace(Pos, PointOnEdge))
					{
						return (mNearestNavSort[j].mHandle * -1);	// "Edges" have negative IDs
					}
				}
			}
		}
	}
	return WAYPOINT_NONE;
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
NAV::TNodeHandle		NAV::ChooseRandomNeighbor(NAV::TNodeHandle NodeHandle)
{
	if (NodeHandle!=WAYPOINT_NONE && NodeHandle>0)
	{
		TGraph::TNodeNeighbors&	neighbors = mGraph.get_node_neighbors(NodeHandle);
		if (neighbors.size()>0)
		{
			return (neighbors[Q_irand(0, neighbors.size()-1)].mNode);
		}
	}
	return WAYPOINT_NONE;
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
NAV::TNodeHandle		NAV::ChooseRandomNeighbor(TNodeHandle NodeHandle, const vec3_t& position, float maxDistance)
{
	if (NodeHandle!=WAYPOINT_NONE && NodeHandle>0)
	{
		CVec3 Pos(position);

		TGraph::TNodeNeighbors&	neighbors = mGraph.get_node_neighbors(NodeHandle);

		// Remove All Neighbors That Are Too Far
		//---------------------------------------
		for (int i=0; i<neighbors.size(); i++)
		{
			if (mGraph.get_node(neighbors[i].mNode).mPoint.Dist(Pos)>maxDistance)
			{
				neighbors.erase_swap(i);
				i--;
				if (neighbors.empty())
				{
					return WAYPOINT_NONE;
				}
			}
		}

		// Now, Randomly Pick From What Is Left
		//--------------------------------------
		if (neighbors.size()>0)
		{
			return (neighbors[Q_irand(0, neighbors.size()-1)].mNode);
		}
	}
	return WAYPOINT_NONE;
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
NAV::TNodeHandle		NAV::ChooseClosestNeighbor(NAV::TNodeHandle NodeHandle, const vec3_t& position)
{
	if (NodeHandle!=WAYPOINT_NONE && NodeHandle>0)
	{
		CVec3					pos(position);
		TGraph::TNodeNeighbors&	neighbors = mGraph.get_node_neighbors(NodeHandle);

		NAV::TNodeHandle	Cur			= WAYPOINT_NONE;
		float				CurDist		= 0.0f;
		NAV::TNodeHandle	Best		= NodeHandle;
		float				BestDist	= mGraph.get_node(Cur).mPoint.Dist2(pos);

		for (int i=0; i<neighbors.size(); i++)
		{
			Cur		= neighbors[i].mNode;
			CurDist = mGraph.get_node(Cur).mPoint.Dist2(pos);
			if (Best==WAYPOINT_NONE || BestDist<CurDist)
			{
				Best		= Cur;
				BestDist	= CurDist;
			}
		}
		return Best;
	}
	return WAYPOINT_NONE;
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
NAV::TNodeHandle		NAV::ChooseFarthestNeighbor(NAV::TNodeHandle NodeHandle, const vec3_t& position)
{
	if (NodeHandle!=WAYPOINT_NONE && NodeHandle>0)
	{
		CVec3					pos(position);
		TGraph::TNodeNeighbors&	neighbors = mGraph.get_node_neighbors(NodeHandle);

		NAV::TNodeHandle	Cur			= WAYPOINT_NONE;
		float				CurDist		= 0.0f;
		NAV::TNodeHandle	Best		= NodeHandle;
		float				BestDist	= mGraph.get_node(Cur).mPoint.Dist2(pos);

		for (int i=0; i<neighbors.size(); i++)
		{
			Cur		= neighbors[i].mNode;
			CurDist = mGraph.get_node(Cur).mPoint.Dist2(pos);
			if (Best==WAYPOINT_NONE || BestDist>CurDist)
			{
				Best		= Cur;
				BestDist	= CurDist;
			}
		}
		return Best;
	}
	return WAYPOINT_NONE;
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
NAV::TNodeHandle		NAV::ChooseFarthestNeighbor(gentity_t* actor, const vec3_t& target, float maxSafeDot)
{
	CVec3					actorPos(actor->currentOrigin);
	CVec3					targetPos(target);
	CVec3					actorToTgt(targetPos - actorPos);
	float					actorToTgtDist = actorToTgt.Norm();

	NAV::TNodeHandle		cur			= GetNearestNode(actor);

	// If Not Anywhere, Give Up
	//--------------------------
	if (cur==WAYPOINT_NONE)
	{
		return WAYPOINT_NONE;
	}

	// If On An Edge, Pick The Safest Of The Two Points
	//--------------------------------------------------
	if (cur<0)
	{
		CWayEdge& edge = mGraph.get_edge(abs(cur));
		if (edge.PointA().Dist2(targetPos)>edge.PointA().Dist2(actorPos))
		{
			return edge.mNodeA;
		}
		return edge.mNodeB;
	}

	CVec3					curPos(mGraph.get_node(cur).mPoint);
	CVec3					curToTgt(targetPos - curPos);
	float					curDist		= curToTgt.SafeNorm();
//	float					curDot		= curToTgt.Dot(actorToTgt);
	NAV::TNodeHandle		best		= WAYPOINT_NONE;
	float					bestDist	= 0.0f;
	TGraph::TNodeNeighbors&	neighbors	= mGraph.get_node_neighbors(cur);


	// If The Actor's Current Point Is Valid, Initialize The Best One To That
	//------------------------------------------------------------------------
	if (curDist>actorToTgtDist && actorPos.Dist(curPos)>300.0f)//curDot<maxSafeDot
	{
		best		= cur;
		bestDist	= curDist;
	}


	for (int i=0; i<neighbors.size(); i++)
	{
		cur			= neighbors[i].mNode;
		curPos		= (mGraph.get_node(cur).mPoint);
		curToTgt	= (targetPos - curPos);
		curDist		= curToTgt.SafeNorm();
//		curDot		= curToTgt.Dot(actorToTgt);

		if (curDist>bestDist && curDist>actorToTgtDist)//curDot<maxSafeDot
		{
			best		= cur;
			bestDist	= curDist;
		}
	}
	return best;
}



////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
bool	NAV::FindPath(gentity_t* actor, NAV::TNodeHandle target, float MaxDangerLevel)
{
	mUser.ClearActor();

	// If Either Start Or End Is Invalid, We Can't Do Any Pathing
	//------------------------------------------------------------
	if (target==WAYPOINT_NONE)
	{
		return false;
	}

	NAV::TNodeHandle	start = GetNearestNode(actor, true, target);
	if (start==WAYPOINT_NONE)
	{
		return false;
	}

	// Convert Edges To Points
	//------------------------
	if (start<0)
	{
		start = (Q_irand(0,1)==0)?(mGraph.get_edge(abs(start)).mNodeA):(mGraph.get_edge(abs(start)).mNodeB);
	}
	if (target<0)
	{
		target = (Q_irand(0,1)==0)?(mGraph.get_edge(abs(target)).mNodeA):(mGraph.get_edge(abs(target)).mNodeB);
	}

	mUser.SetActor(actor);

	// First Step: Find The Actor And Make Sure He Has A Path User Struct
	//--------------------------------------------------------------------
	int pathUserNum = mPathUserIndex[actor->s.number];
	if (pathUserNum==NULL_PATH_USER_INDEX)
	{
		if (mPathUsers.full())
		{
			assert("NAV: No more unused path users, possibly change MAX_PATH_USERS"==0);
			return false;
		}

		pathUserNum = mPathUsers.alloc();
		mPathUsers[pathUserNum].mEnd			= WAYPOINT_NONE;
		mPathUsers[pathUserNum].mSuccess		= false;
		mPathUsers[pathUserNum].mLastAStarTime	= 0;
		mPathUserIndex[actor->s.number]			= pathUserNum;
	}
	SPathUser&	puser = mPathUsers[pathUserNum];
	puser.mLastUseTime = level.time;


	// Now, Check To See If He Already Has Found A Path To This Target
	//-----------------------------------------------------------------
	if (puser.mEnd==target && level.time<puser.mLastAStarTime)
	{
		return puser.mSuccess;
	}



	// Setup The Search
	//------------------
	mSearch.mStart	= start;
	mSearch.mEnd	= target;
	puser.mEnd		= target;


	// First Check The Region
	//------------------------
	if (mRegion.size()>0 && !mRegion.has_valid_edge(mSearch.mStart, mSearch.mEnd, mUser))
	{
		puser.mSuccess = false;
		return puser.mSuccess;
	}



	// Now, Run A*
	//-------------
	if (actor->enemy && actor->enemy->client)
	{
		if (actor->enemy->client->ps.weapon==WP_SABER)
		{
			mUser.SetDangerSpot(actor->enemy->currentOrigin, 200.0f);
		}
		else if (
			actor->enemy->client->NPC_class==CLASS_RANCOR ||
			actor->enemy->client->NPC_class==CLASS_WAMPA)
		{
			mUser.SetDangerSpot(actor->enemy->currentOrigin, 400.0f);
		}
	}
	mGraph.astar(mSearch, mUser);
	mUser.ClearDangerSpot();

	puser.mLastAStarTime = level.time + Q_irand(3000, 6000);
	puser.mSuccess = mSearch.success();
	if (!puser.mSuccess)
	{
		return puser.mSuccess;
	}


	// Grab A Couple "Current Conditions"
	//------------------------------------
	CVec3	At(actor->currentOrigin);
	float	AtTime	= level.time;
	float	AtSpeed = actor->NPC->stats.runSpeed;
	if (!(actor->NPC->scriptFlags&SCF_RUNNING) &&
		((actor->NPC->scriptFlags&SCF_WALKING) ||
		 (actor->NPC->aiFlags&NPCAI_WALKING) ||
		 (ucmd.buttons&BUTTON_WALKING)
		))
	{
		AtSpeed	= actor->NPC->stats.walkSpeed;
	}

	AtSpeed *= 0.001f;		// Convert units/sec to units/millisec for comparison against level.time
	AtSpeed *= 0.25;		// Cut the speed in half to account for accel & decel & some slop



	// Get The Size Of This Actor
	//----------------------------
	float minRadius = Min(actor->mins[0], actor->mins[1]);
	float maxRadius = Max(actor->maxs[0], actor->maxs[1]);
	float radius = Max(fabsf(minRadius), maxRadius);

	if (radius>20.0f)
	{
		radius = 20.0f;
	}


	// Copy The Search Results Into The Path
	//---------------------------------------
	{
		SPathPoint PPoint = {};
		puser.mPath.clear();
		for (mSearch.path_begin(); !mSearch.path_end() && !puser.mPath.full(); mSearch.path_inc())
		{
			if (puser.mPath.full())
			{
				// NAV_TODO: If the path is longer than the max length, we want to store the first valid ones, instead of returning false
				assert("This Is A Test To See If We Hit This Condition Anymore...  It Should Be Handled Properly"==0);
				mPathUsers.free(pathUserNum);
				mPathUserIndex[actor->s.number]		= NULL_PATH_USER_INDEX;
				return false;
			}

			PPoint.mNode				= mSearch.path_at();
			PPoint.mPoint				= mGraph.get_node(PPoint.mNode).mPoint;
			PPoint.mSpeed				= AtSpeed;
			PPoint.mSlowingRadius		= 0.0f;
			PPoint.mReachedRadius		= Max(radius*3.0f, (mGraph.get_node(PPoint.mNode).mRadius * 0.40f));
			if (mGraph.get_node(PPoint.mNode).mFlags.get_bit(CWayNode::WN_FLOATING))
			{
				PPoint.mReachedRadius	= 20.0f;
			}
			PPoint.mReachedRadius		*= PPoint.mReachedRadius;	// squared, for faster checks later
			PPoint.mDist				= 0.0f;
			PPoint.mETA					= 0.0f;

			puser.mPath.push_back(PPoint);
		}
		assert(puser.mPath.size()>0);


		// Last Point On The Path Always Gets A Slowing Radius
		//-----------------------------------------------------
		puser.mPath[0].mSlowingRadius	= Max(70.0f, (mGraph.get_node(PPoint.mNode).mRadius * 0.75f));
		puser.mPath[0].mReachedRadius	= Max(radius, (mGraph.get_node(PPoint.mNode).mRadius * 0.15f));
		puser.mPath[0].mReachedRadius	*= puser.mPath[0].mReachedRadius;	// squared, for faster checks later
	}

	int numEdges = (puser.mPath.size()-1);


	// Trim Out Backtracking Edges
	//-----------------------------
	for (int edge=0; edge<numEdges; edge++)
	{
		CVec3	PointA(puser.mPath[edge].mPoint);
		CVec3	PointB(puser.mPath[edge+1].mPoint);

		CVec3	AtOnEdge(At);
		float	AtOnEdgeScale = AtOnEdge.ProjectToLineSeg(PointA, PointB);
		float	AtDistToEdge = AtOnEdge.Dist(At);

		if (AtOnEdgeScale>0.1f && AtOnEdgeScale<0.9f)
		{
			if (AtDistToEdge<(radius) || (AtDistToEdge<(radius*20.0f) && MoveTrace(At, AtOnEdge, actor->mins, actor->maxs, actor->s.number, true, true, false)))
			{
				puser.mPath.resize(edge+2);			// +2 because every edge needs at least 2 points
				puser.mPath[edge+1].mPoint = AtOnEdge;
				break;
			}
		}
	}



	// For All Points On The Path, Compute ETA, And Check For Sharp Corners
	//----------------------------------------------------------------------
	CVec3	AtToNext;
	CVec3	NextToBeyond;
	float	NextToBeyondDistance;
	float	NextToBeyondDot;

	for (int i=puser.mPath.size()-1; i>-1; i--)
	{
		SPathPoint&		PPoint	= puser.mPath[i];					// For Debugging And A Tad Speed Improvement, Get A Ref Directly

		AtToNext		= (PPoint.mPoint - At);
		if (fabsf(AtToNext[2])>Z_CULL_OFFSET)
		{
			AtToNext[2] = 0.0f;
		}

		PPoint.mDist	=  AtToNext.Norm();					// Get The Distance And Norm The Direction
		PPoint.mETA		=  (PPoint.mDist / PPoint.mSpeed);	// Estimate Our Eta By Distance/Speed
		PPoint.mETA		+= AtTime;


		// Check To See If This Is The Apex Of A Sharp Turn
		//--------------------------------------------------
		if (i!=0 &&			//is there a next point?
			!mGraph.get_node(PPoint.mNode).mFlags.get_bit(CWayNode::WN_FLOATING)
			)
		{
			NextToBeyond = (puser.mPath[i-1].mPoint - PPoint.mPoint);
			if (fabsf(NextToBeyond[2])>Z_CULL_OFFSET)
			{
				NextToBeyond[2] = 0.0f;
			}
			NextToBeyondDistance = NextToBeyond.Norm();
			NextToBeyondDot = NextToBeyond.Dot(AtToNext);

			if ((NextToBeyondDistance>150.0f && PPoint.mDist>150.0f && NextToBeyondDot<0.64f) ||
				(NextToBeyondDistance>30.0f && NextToBeyondDot<0.5f))
			{
				PPoint.mSlowingRadius = Max(40.0f, mGraph.get_node(PPoint.mNode).mRadius);		// Force A Stop Here
			}
		}

		// Update Our Time And Location For The Next Point
		//-------------------------------------------------
		AtTime	= PPoint.mETA;
		At		= PPoint.mPoint;
	}

	// Failed To Find An Acceptibly Safe Path
	//----------------------------------------
	if (MaxDangerLevel!=1.0f && NAV::PathDangerLevel(NPC)>MaxDangerLevel)
	{
		puser.mSuccess = false;
	}


	assert(puser.mPath.size()>0);
	return puser.mSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
bool			NAV::SafePathExists(const CVec3& startVec, const CVec3& stopVec, const CVec3& danger, float dangerDistSq)
{
	mUser.ClearActor();


	// If Either Start Or End Is Invalid, We Can't Do Any Pathing
	//------------------------------------------------------------
	NAV::TNodeHandle	target = GetNearestNode(stopVec.v, 0, 0, 0, true);
	if (target==WAYPOINT_NONE)
	{
		return false;
	}

	NAV::TNodeHandle	start = GetNearestNode(startVec.v, 0, target, 0, true);
	if (start==WAYPOINT_NONE)
	{
		return false;
	}

	// Convert Edges To Points
	//------------------------
	if (start<0)
	{
		start = mGraph.get_edge(abs(start)).mNodeA;
	}
	if (target<0)
	{
		target = mGraph.get_edge(abs(target)).mNodeA;
	}
	if (start==target)
	{
		return true;
	}


	// First Step: Find The Actor And Make Sure He Has A Path User Struct
	//--------------------------------------------------------------------
	SPathUser&	puser = mPathUserMaster;
	puser.mLastUseTime = level.time;


	// Now, Check To See If He Already Has Found A Path To This Target
	//-----------------------------------------------------------------
	if (puser.mEnd==target && level.time<puser.mLastAStarTime)
	{
		return puser.mSuccess;
	}



	// Setup The Search
	//------------------
	mSearch.mStart	= start;
	mSearch.mEnd	= target;
	puser.mEnd		= target;


	// First Check The Region
	//------------------------
 	if (mRegion.size()>0 && !mRegion.has_valid_edge(mSearch.mStart, mSearch.mEnd, mUser))
	{
		puser.mSuccess = false;
		return puser.mSuccess;
	}

	// Now, Run A*
	//-------------
//	mUser.SetDangerSpot(danger, dangerDistSq);
	mGraph.astar(mSearch, mUser);
//	mUser.ClearDangerSpot();

	puser.mLastAStarTime = level.time + Q_irand(3000, 6000);;
	puser.mSuccess = mSearch.success();
	if (!puser.mSuccess)
	{
		return puser.mSuccess;
	}


	// Failed To Find An Acceptibly Safe Path
	//----------------------------------------
	CVec3	Prev(stopVec);
	CVec3	Next;
	for (mSearch.path_begin(); !mSearch.path_end(); mSearch.path_inc())
	{
		Next = mGraph.get_node(mSearch.path_at()).mPoint;
		if (dangerDistSq > danger.DistToLine2(Next, Prev))
		{
			puser.mSuccess = false;
			break;
		}
		Prev = Next;
	}
	if (puser.mSuccess)
	{
		Next = startVec;
		if (dangerDistSq > danger.DistToLine2(Prev, Next))
		{
			puser.mSuccess = false;
		}
	}

	return puser.mSuccess;
}




////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
float			NAV::EstimateCostToGoal(const vec3_t& position, TNodeHandle Goal)
{
	if (Goal!=0 && Goal!=WAYPOINT_NONE)
	{
		return	(Distance(position, GetNodePosition(Goal)));
	}
	return 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
float			NAV::EstimateCostToGoal(TNodeHandle Start, TNodeHandle Goal)
{
	mUser.ClearActor();
	if (Goal!=0 && Goal!=WAYPOINT_NONE && Start!=0 && Start!=WAYPOINT_NONE)
	{
		return (Distance(GetNodePosition(Start), GetNodePosition(Goal)));
	}
	return 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
bool			NAV::OnSamePoint(gentity_t* actor, gentity_t* target)
{
	return (GetNearestNode(actor)==GetNearestNode(target));
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
bool			NAV::InSameRegion(gentity_t* actor, gentity_t* target)
{
	mUser.ClearActor();
	if (mRegion.size()>0)
	{
		NAV::TNodeHandle	actNode = GetNearestNode(actor);
		NAV::TNodeHandle	tgtNode = GetNearestNode(target);
		if (actNode==WAYPOINT_NONE || tgtNode==WAYPOINT_NONE)
		{
			return false;
		}
		if (actNode==tgtNode)
		{
			return true;
		}

		if (actNode<0)
		{
			actNode = mGraph.get_edge(abs(actNode)).mNodeA;
		}
		if (tgtNode<0)
		{
			tgtNode = mGraph.get_edge(abs(tgtNode)).mNodeA;
		}

		mUser.SetActor(actor);

		return (mRegion.has_valid_edge(actNode, tgtNode, mUser));
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
bool			NAV::InSameRegion(gentity_t* actor, const vec3_t& position)
{
	mUser.ClearActor();
	if (mRegion.size()>0)
	{
		NAV::TNodeHandle	actNode = GetNearestNode(actor);
		NAV::TNodeHandle	tgtNode = GetNearestNode(position);

		if (actNode==WAYPOINT_NONE || tgtNode==WAYPOINT_NONE)
		{
			return false;
		}
		if (actNode==tgtNode)
		{
			return true;
		}

		if (actNode<0)
		{
			actNode = mGraph.get_edge(abs(actNode)).mNodeA;
		}
		if (tgtNode<0)
		{
			tgtNode = mGraph.get_edge(abs(tgtNode)).mNodeA;
		}

		mUser.SetActor(actor);

		return (mRegion.has_valid_edge(actNode, tgtNode, mUser));
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
bool			NAV::InSameRegion(NAV::TNodeHandle A, NAV::TNodeHandle B)
{
	if (mRegion.size()>0)
	{
		NAV::TNodeHandle	actNode = A;
		NAV::TNodeHandle	tgtNode = B;

		if (actNode==WAYPOINT_NONE || tgtNode==WAYPOINT_NONE)
		{
			return false;
		}
		if (actNode==tgtNode)
		{
			return true;
		}

		if (actNode<0)
		{
			actNode = mGraph.get_edge(abs(actNode)).mNodeA;
		}
		if (tgtNode<0)
		{
			tgtNode = mGraph.get_edge(abs(tgtNode)).mNodeA;
		}

		gentity_t*	tempActor = mUser.GetActor();
		mUser.ClearActor();
		bool	hasEdge = mRegion.has_valid_edge(actNode, tgtNode, mUser);
		if (tempActor)
		{
			mUser.SetActor(tempActor);
		}
		return hasEdge;
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
bool			NAV::OnNeighboringPoints(TNodeHandle A, TNodeHandle B)
{
	if (A==B)
	{
		return true;
	}

	if (A<=0 || B<=0)
	{
		return false;
	}
	int edgeNum = mGraph.get_edge_across(A, B);
	if (edgeNum &&
		!mGraph.get_edge(edgeNum).mFlags.get_bit(CWayEdge::WE_JUMPING) &&
		!mGraph.get_edge(edgeNum).mFlags.get_bit(CWayEdge::WE_FLYING) &&
		 mGraph.get_edge(edgeNum).mDistance<SAFE_NEIGHBORINGPOINT_DIST)
	{
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
bool			NAV::OnNeighboringPoints(gentity_t* actor, gentity_t* target)
{
	if (OnNeighboringPoints(GetNearestNode(actor), GetNearestNode(target)))
	{
		if (Distance(actor->currentOrigin, target->currentOrigin)<NEIGHBORING_DIST)
		{
//			if (ViewNavTrace(actor->currentOrigin, target->currentOrigin))
			{
				return true;
			}
		}
	}
	return false;
}


////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
bool			NAV::OnNeighboringPoints(gentity_t* actor, const vec3_t& position)
{
	if (OnNeighboringPoints(GetNearestNode(actor), GetNearestNode(position)))
	{
		if (Distance(actor->currentOrigin, position)<NEIGHBORING_DIST)
		{
//			if (ViewNavTrace(actor->currentOrigin, position))
			{
				return true;
			}
		}
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////
// Is The Position (at) within the safe radius of the atNode, targetNode, or the edge
// between?
////////////////////////////////////////////////////////////////////////////////////////
bool			NAV::InSafeRadius(CVec3 at, TNodeHandle atNode, TNodeHandle targetNode)
{
	// Uh, No
	//--------
	if (atNode<=0)
	{
		return false;
	}

	// If In The Radius Of Nearest Nav Point
	//---------------------------------------
	if (Distance(at.v, GetNodePosition(atNode))<mGraph.get_node(atNode).mRadius)
	{
		return true;
	}

	// If We Have A Target Node, We May Be On The Edge That Connects Between The Two
	//-------------------------------------------------------------------------------
	if (targetNode>0 && atNode!=targetNode)
	{
		// If In The Radius Of Target Nav Point
		//---------------------------------------
		if (Distance(at.v, GetNodePosition(targetNode))<mGraph.get_node(targetNode).mRadius)
		{
			return true;
		}

		// Does The Edge Exist?
		//----------------------
		int	atToTargetEdgeIndex = mGraph.get_edge_across(atNode, targetNode);
		if (atToTargetEdgeIndex!=0)
		{
			CWayEdge&	atToTargetEdge	= mGraph.get_edge(atToTargetEdgeIndex);

			// Is The Edge Valid?
			//--------------------
			if (!atToTargetEdge.mFlags.get_bit(CWayEdge::WE_FLYING) &&
				!atToTargetEdge.mFlags.get_bit(CWayEdge::WE_JUMPING) &&
				mUser.is_valid(atToTargetEdge, targetNode))
			{
				float	atDistToEdge = at.DistToLine(atToTargetEdge.PointA(), atToTargetEdge.PointB());

				// Now Are We Close Enough To The Edge
				//-------------------------------------
				if (atToTargetEdge.mFlags.get_bit(CWayEdge::WE_SIZE_LARGE))
				{
					return (atDistToEdge<SC_LARGE_RADIUS);
				}
				return (atDistToEdge<SC_MEDIUM_RADIUS);
			}// not valid edge
		}// no edge
	}// no valid target
	return false;
}



////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
bool			NAV::FindPath(gentity_t* actor, gentity_t* target, float MaxDangerLevel)
{
	assert(target!=0 && actor!=0);
	if (target!=0 && actor!=0)
	{
		if (target->waypoint==WAYPOINT_NONE)
		{
			GetNearestNode(target);
		}
		if (target->waypoint!=WAYPOINT_NONE)
		{
			return FindPath(actor, target->waypoint, MaxDangerLevel);
		}
		else if (target->lastWaypoint!=WAYPOINT_NONE)
		{
			return FindPath(actor, target->lastWaypoint, MaxDangerLevel);
		}
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
bool			NAV::FindPath(gentity_t* actor, const vec3_t& position, float MaxDangerLevel)
{
	return FindPath(actor, GetNearestNode(position), MaxDangerLevel);
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
const vec3_t&	NAV::NextPosition(gentity_t* actor)
{
	assert(HasPath(actor));
	TPath&	path = mPathUsers[mPathUserIndex[actor->s.number]].mPath;

	return (path[path.size()-1].mPoint.v);
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
bool			NAV::NextPosition(gentity_t* actor, CVec3& Position)
{
	assert(HasPath(actor));
	TPath&	path = mPathUsers[mPathUserIndex[actor->s.number]].mPath;
	Position		= path[path.size()-1].mPoint;
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
bool			NAV::NextPosition(gentity_t* actor, CVec3& Position, float& SlowingRadius, bool& Fly, bool& Jump)
{
	assert(HasPath(actor));
	TPath&			path = mPathUsers[mPathUserIndex[actor->s.number]].mPath;
	SPathPoint&		next = path[path.size()-1];
	CWayNode&		node = mGraph.get_node(next.mNode);
	int				curNodeIndex	= NAV::GetNearestNode(actor);
	int				edgeIndex		= (curNodeIndex>0)?(mGraph.get_edge_across(curNodeIndex, next.mNode)):(abs(curNodeIndex));

	SlowingRadius	= next.mSlowingRadius;
	Position		= next.mPoint;
	Fly				= node.mFlags.get_bit(CWayNode::WN_FLOATING);

	if (edgeIndex)
	{
		Jump		= mGraph.get_edge(edgeIndex).mFlags.get_bit(CWayEdge::WE_JUMPING);
	}

	return true;
}
////////////////////////////////////////////////////////////////////////////////////////
// This function completely removes any pathfinding information related to this agent
// and frees up this path user structure for someone else to use.
////////////////////////////////////////////////////////////////////////////////////////
void			NAV::ClearPath(gentity_t* actor)
{
	int pathUserNum = mPathUserIndex[actor->s.number];
	if (pathUserNum==NULL_PATH_USER_INDEX)
	{
		return;
	}

	mPathUsers.free(pathUserNum);
	mPathUserIndex[actor->s.number] = NULL_PATH_USER_INDEX;
}


////////////////////////////////////////////////////////////////////////////////////////
// Update Path
//
// Removes points that have been reached, and frees the user if no points remain.
//
////////////////////////////////////////////////////////////////////////////////////////
bool			NAV::UpdatePath(gentity_t* actor, TNodeHandle target, float MaxDangerLevel)
{
	int pathUserNum = mPathUserIndex[actor->s.number];
	if (pathUserNum==NULL_PATH_USER_INDEX)
	{
		return false;
	}
	if (!mPathUsers[pathUserNum].mSuccess)
	{
		return false;
	}
	if (!mPathUsers[pathUserNum].mPath.size())
	{
		return false;
	}


	// Remove Any Points We Have Reached
	//-----------------------------------
	CVec3		At(actor->currentOrigin);
	TPath&		path = mPathUsers[pathUserNum].mPath;
	bool		InReachedRadius = false;
	bool		ReachedAnything = false;
	assert(path.size()>0);

	do
	{
		SPathPoint&	PPoint	= path[path.size()-1];
		CVec3	Dir(PPoint.mPoint - At);
		if (fabsf(At[2] - PPoint.mPoint[2])<Z_CULL_OFFSET)
		{
			Dir[2] = 0.0f;
		}

		InReachedRadius = (Dir.Len2()<PPoint.mReachedRadius);
		if (InReachedRadius)
		{
			ReachedAnything = true;
			path.pop_back();
		}
	} while (InReachedRadius && path.size()>0);


	// If We've Reached The End Of The Path, Return With no path!
	//------------------------------------------------------------
	if (path.empty())
	{
		return false;
	}

	if (ReachedAnything)
	{
		if (target!=PT_NONE && mPathUsers[pathUserNum].mEnd!=target)
		{
			return false;
		}
	}

	// If Not, Time To Double Check To See If The Path Is Still Valid
	//----------------------------------------------------------------
	if (path[path.size()-1].mETA<level.time || (MaxDangerLevel!=1.0f && PathDangerLevel(NPC)>MaxDangerLevel))
	{
		// Hmmm.  Should Have Reached This Point By Now, Or Too Dangerous.  Try To Recompute The Path
		//--------------------------------------------------------------------------------------------
		NAV::TNodeHandle target = mPathUsers[pathUserNum].mEnd;			// Remember The End As Our Target
		if (target==WAYPOINT_NONE)
		{
			ClearPath(actor);
			return false;
		}
		mPathUsers[pathUserNum].mEnd = WAYPOINT_NONE;					// Clear Out The Old End
		if (!FindPath(actor, target, MaxDangerLevel))
		{
			mPathUsers[pathUserNum].mEnd = target;
			return false;
		}
		return true;
	}


	// Ok, We Have A Path, And Are On-Route
	//--------------------------------------
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
bool			NAV::HasPath(gentity_t* actor, TNodeHandle target)
{
	int pathUserNum = mPathUserIndex[actor->s.number];
	if (pathUserNum==NULL_PATH_USER_INDEX)
	{
		return false;
	}
	if (!mPathUsers[pathUserNum].mSuccess)
	{
		return false;
	}
	if (!mPathUsers[pathUserNum].mPath.size())
	{
		return false;
	}
	if (target!=PT_NONE && mPathUsers[pathUserNum].mEnd!=target)
	{
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
float			NAV::PathDangerLevel(gentity_t* actor)
{
	if (!actor)
	{
		assert("No Actor!!!"==0);
		return 0.0f;
	}
	int pathUserNum = mPathUserIndex[actor->s.number];
	if (pathUserNum==NULL_PATH_USER_INDEX)
	{
		return 0.0f;
	}
	TPath&	Path = mPathUsers[pathUserNum].mPath;

	// If it only has one point, let's say it's not dangerous
	//--------------------------------------------------------
	if (Path.size()<2)
	{
		return 0.0f;
	}

	float		DangerLevel			= 0.0f;
	CVec3		enemyPos;
	float		enemySafeDist		= 0.0f;
	float		enemyDangerLevel	= 0.0f;
	TEdgeHandle	curEdge				= 0;
	int			curPathAt			= Path.size()-1;
	TAlertList& al					= mEntityAlertList[actor->s.number];
	int			alIndex				= 0;
	TNodeHandle	prevNode			= (GetNearestNode(actor));
	CVec3		prevPoint(actor->currentOrigin);


	// Some Special Enemies Always Cause Persistant Danger
	//-----------------------------------------------------
	if (actor->enemy && actor->enemy->client)
	{
		if (actor->enemy->client->ps.weapon==WP_SABER ||
			actor->enemy->client->NPC_class==CLASS_RANCOR ||
			actor->enemy->client->NPC_class==CLASS_WAMPA)
		{
			enemyPos = (actor->enemy->currentOrigin);
			enemySafeDist = actor->enemy->radius * 10.0f;
		}
	}


	// Go Through All Remaining Points On The Path
	//---------------------------------------------
	for (; curPathAt>=0; curPathAt--)
	{
		SPathPoint&	PPoint	= Path[curPathAt];

		// If Any Edges On The Path Have Been Registered As Dangerous
		//-------------------------------------------------------------
		if (prevNode<0 || mGraph.get_edge_across(prevNode, PPoint.mNode))
		{
			if (prevNode<0)
			{
				curEdge = prevNode;
			}
			else
			{
				curEdge = mGraph.get_edge_across(prevNode, PPoint.mNode);
			}
			for (alIndex=0; alIndex<TAlertList::CAPACITY; alIndex++)
			{
				if (al[alIndex].mHandle==curEdge && al[alIndex].mDanger>DangerLevel)
				{
					DangerLevel = al[alIndex].mDanger;
				}
			}
		}

		// Check For Enemy Position Proximity To This Next Edge (using actual Edge Locations)
		//------------------------------------------------------------------------------------
		if (enemySafeDist!=0.0f)
		{
			enemyDangerLevel = enemyPos.DistToLine(prevPoint, PPoint.mPoint)/enemySafeDist;
			if (enemyDangerLevel>DangerLevel)
			{
				DangerLevel = enemyDangerLevel;
			}
		}

		prevNode	= PPoint.mNode;
		prevPoint	= PPoint.mPoint;
	}
	return DangerLevel;
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
int				NAV::PathNodesRemaining(gentity_t* actor)
{
	int pathUserNum = mPathUserIndex[actor->s.number];
	if (pathUserNum==NULL_PATH_USER_INDEX)
	{
		return false;
	}
	return mPathUsers[pathUserNum].mPath.size();

}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
const vec3_t&	NAV::GetNodePosition(TNodeHandle NodeHandle)
{
	if (NodeHandle!=0)
	{
		if (NodeHandle>0)
		{
			return (mGraph.get_node(NodeHandle).mPoint.v);
		}
		else
		{
			return (mGraph.get_edge(abs(NodeHandle)).PointA().v);
		}
	}
	return mZeroVec;
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
void			NAV::GetNodePosition(TNodeHandle NodeHandle, vec3_t& position)
{
	if (NodeHandle!=0)
	{
		if (NodeHandle>0)
		{
			VectorCopy(mGraph.get_node(NodeHandle).mPoint.v, position);
		}
		else
		{
			VectorCopy(mGraph.get_edge(abs(NodeHandle)).PointA().v, position);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// Call This function to get the size classification for a given entity
////////////////////////////////////////////////////////////////////////////////////////
unsigned int		NAV::ClassifyEntSize(gentity_t* ent)
{
	if (ent)
	{
		float minRadius = Min(ent->mins[0], ent->mins[1]);
		float maxRadius = Max(ent->maxs[0], ent->maxs[1]);
		float radius = Max(fabsf(minRadius), maxRadius);
		float height = ent->maxs[2];

		if ((radius > SC_MEDIUM_RADIUS) || height > (SC_MEDIUM_HEIGHT))
		{
			return CWayEdge::WE_SIZE_LARGE;
		}
		return CWayEdge::WE_SIZE_MEDIUM;
	}
	return 0;
}


////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
void			NAV::ShowDebugInfo(const vec3_t& PlayerPosition, int PlayerWaypoint)
{
	mUser.ClearActor();
	CVec3	atEnd;

	// Show Nodes
	//------------
	if (NAVDEBUG_showNodes || NAVDEBUG_showCombatPoints || NAVDEBUG_showNavGoals)
	{
		for (TGraph::TNodes::iterator atIter=mGraph.nodes_begin(); atIter!=mGraph.nodes_end(); atIter++)
		{
			CWayNode& at	= (*atIter);
			atEnd			= at.mPoint;
			atEnd[2]		+= 30.0f;


			if (gi.inPVS(PlayerPosition, at.mPoint.v))
			{
				if (at.mType==NAV::PT_WAYNODE && NAVDEBUG_showNodes)
				{
					if (NAVDEBUG_showPointLines)
					{
						if (at.mFlags.get_bit(CWayNode::WN_FLOATING))
						{
							CG_DrawEdge(at.mPoint.v, atEnd.v, EDGE_NODE_FLOATING );
						}
						else
						{
							CG_DrawEdge(at.mPoint.v, atEnd.v, EDGE_NODE_NORMAL );
						}
					}
					else
					{
						if (at.mFlags.get_bit(CWayNode::WN_FLOATING))
						{
							CG_DrawNode(at.mPoint.v, NODE_FLOATING );
						}
						else
						{
							CG_DrawNode(at.mPoint.v, NODE_NORMAL );
						}
					}
					if (NAVDEBUG_showRadius && at.mPoint.Dist2(PlayerPosition)<(at.mRadius*at.mRadius))
					{
						if (at.mFlags.get_bit(CWayNode::WN_FLOATING))
						{
							CG_DrawRadius(at.mPoint.v, at.mRadius, NODE_FLOATING );
						}
						else
						{
							CG_DrawRadius(at.mPoint.v, at.mRadius, NODE_NORMAL );
						}
					}
				}
				else if (at.mType==NAV::PT_COMBATNODE && NAVDEBUG_showCombatPoints)
				{
					if (NAVDEBUG_showPointLines)
					{
						CG_DrawEdge(at.mPoint.v, atEnd.v, EDGE_NODE_COMBAT );
					}
					else
					{
						CG_DrawCombatPoint(at.mPoint.v, 0);
					}
				}
				else if (at.mType==NAV::PT_GOALNODE && NAVDEBUG_showNavGoals)
				{
					if (NAVDEBUG_showPointLines)
					{
						CG_DrawEdge(at.mPoint.v, atEnd.v, EDGE_NODE_GOAL );
					}
					else
					{
						CG_DrawNode(at.mPoint.v, NODE_NAVGOAL);
					}
				}
			}
		}
	}


	// Show Edges
	//------------
	if (NAVDEBUG_showEdges)
	{
		for (TGraph::TEdges::iterator atIter=mGraph.edges_begin(); atIter!=mGraph.edges_end(); atIter++)
		{
			CWayEdge& at	= (*atIter);
			CWayNode& a		= mGraph.get_node(at.mNodeA);
			CWayNode& b		= mGraph.get_node(at.mNodeB);
			CVec3	AvePos	= a.mPoint + b.mPoint;
			AvePos *= 0.5f;

			if (AvePos.Dist2(PlayerPosition)<(500.0f*500.0f) && gi.inPVS(PlayerPosition, AvePos.v))
			{
				if (mUser.is_valid(at))
				{
					if (at.mFlags.get_bit(CWayEdge::WE_JUMPING))
					{
						CG_DrawEdge(a.mPoint.v, b.mPoint.v, EDGE_JUMP);
					}
					else if (at.mFlags.get_bit(CWayEdge::WE_FLYING))
					{
						CG_DrawEdge(a.mPoint.v, b.mPoint.v, EDGE_FLY);
					}
					else if (at.Size()==CWayEdge::WE_SIZE_LARGE)
					{
						CG_DrawEdge(a.mPoint.v, b.mPoint.v, EDGE_LARGE);
					}
					else
					{
						CG_DrawEdge(a.mPoint.v, b.mPoint.v, EDGE_NORMAL);
					}
				}
				else
				{
					CG_DrawEdge(a.mPoint.v, b.mPoint.v, EDGE_BLOCKED);
				}
			}
		}
	}
	if (NAVDEBUG_showGrid)
	{
		float x1, y1;
		float x2, y2;
		float z = 0.0f;

		for (int x=0; x<TGraphCells::SIZEX; x++)
		{
			for (int y=0; y<TGraphCells::SIZEY; y++)
			{
				TGraphCells::TCellNodes& nodes = mCells.get_cell(x, y).mNodes;
				mCells.get_cell_upperleft(x,y, x1, y1);
				mCells.get_cell_lowerright(x,y, x2, y2);

				int type = EDGE_CELL_EMPTY;
				if (nodes.size())
				{
					type = EDGE_CELL;
					z = mGraph.get_node(nodes[0])[2] - 10.0f;
				}


				CVec3	upleft(x1, y1, z);
				CVec3	upright(x2, y1, z);
				CVec3	downright(x2, y2, z);
				CVec3	downleft(x1, y2, z);
				CVec3	center = (upleft + downright) * 0.5f;
				if (center.Dist(PlayerPosition)<10000.0f)
				{
					CG_DrawEdge(upleft.v, upright.v, type);
					CG_DrawEdge(upright.v, downright.v, type);
				//	CG_DrawEdge(downright.v, downleft.v, type);
				//	CG_DrawEdge(downleft.v, upleft.v, type);

				}
			}
		}
	}

	if ( NAVDEBUG_showTestPath )
	{
		// NAV_TODO: Allow Test Paths
	}

	if ( NAVDEBUG_showNearest && player && (player->waypoint!=0 || player->lastWaypoint!=0))
	{
		PlayerWaypoint = (player->waypoint)?(player->waypoint):(player->lastWaypoint);
		CVec3	PPos(PlayerPosition);
		if (PlayerWaypoint>0)
		{
			CWayNode& node	= mGraph.get_node(PlayerWaypoint);

			CG_DrawEdge(PPos.v, node.mPoint.v, (player->waypoint)?(EDGE_NEARESTVALID):(EDGE_NEARESTINVALID));
		}
		else
		{
			CWayEdge& edge	= mGraph.get_edge(abs(PlayerWaypoint));
			CVec3 PosOnLine(PlayerPosition);
			PosOnLine.ProjectToLineSeg(edge.PointA(), edge.PointB());

			CG_DrawEdge(PPos.v, PosOnLine.v, (player->waypoint)?(EDGE_NEARESTVALID):(EDGE_NEARESTINVALID));
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////
// Show Stats
////////////////////////////////////////////////////////////////////////////////////
void			NAV::ShowStats()
{
#if !defined(FINAL_BUILD)
	mGraph.ProfileSpew();
	mRegion.ProfileSpew();

	mGraph.ProfilePrint("Point Islands: (%d)", mIslandCount);
	mGraph.ProfilePrint("");
	mGraph.ProfilePrint("");
	mGraph.ProfilePrint("--------------------------------------------------------");
	mGraph.ProfilePrint("              Star Wars - Jedi Academy                  ");
	mGraph.ProfilePrint("               Additional Statistics                    ");
	mGraph.ProfilePrint("--------------------------------------------------------");
	mGraph.ProfilePrint("");
	mGraph.ProfilePrint("MEMORY CONSUMPTION (In Bytes)");
	mGraph.ProfilePrint("Cells  : (%d)", (sizeof(mCells)));
	mGraph.ProfilePrint("Path   : (%d)", (sizeof(mPathUsers)+sizeof(mPathUserIndex)));
	mGraph.ProfilePrint("Steer  : (%d)", (sizeof(mSteerUsers)+sizeof(mSteerUserIndex)));
	mGraph.ProfilePrint("Alerts : (%d)", (sizeof(mEntityAlertList)));
	float totalBytes = (
		sizeof(mCells)+
		sizeof(mGraph)+
		sizeof(mRegion)+
		sizeof(mPathUsers)+
		sizeof(mPathUserIndex)+
		sizeof(mSteerUsers)+
		sizeof(mSteerUserIndex)+
		sizeof(mEntityAlertList));

	mGraph.ProfilePrint("TOTAL :  (KiloBytes): (%5.3f)  MeggaBytes(%3.3f)",
			((float)(totalBytes)/1024.0f),
			((float)(totalBytes)/1048576.0f)
			);
	mGraph.ProfilePrint("");


	mGraph.ProfilePrint("Connect Stats: Milliseconds(%d) Traces(%d)", mConnectTime, mConnectTraceCount);
	mGraph.ProfilePrint("");
	mGraph.ProfilePrint("Move Trace: Count(%d) PerFrame(%f)", mMoveTraceCount, (float)(mMoveTraceCount)/(float)(level.time));
	mGraph.ProfilePrint("View Trace: Count(%d) PerFrame(%f)", mViewTraceCount, (float)(mViewTraceCount)/(float)(level.time));

#endif
}

////////////////////////////////////////////////////////////////////////////////////
// TeleportTo
////////////////////////////////////////////////////////////////////////////////////
void			NAV::TeleportTo(gentity_t* actor, const char* pointName)
{
	assert(actor!=0);
	hstring	nName(pointName);
	TNameToNodeMap::iterator	nameFinder= mNodeNames.find(nName);
	if (nameFinder!=mNodeNames.end())
	{
		if ((*nameFinder).size()>1)
		{
			gi.Printf("WARNING: More than one point named (%s).  Going to first one./n", pointName);
		}
		TeleportPlayer(actor, mGraph.get_node((*nameFinder)[0]).mPoint.v, actor->currentAngles);
		return;
	}
	gi.Printf("Unable To Locate Point (%s)\n", pointName);
}

////////////////////////////////////////////////////////////////////////////////////
// TeleportTo
////////////////////////////////////////////////////////////////////////////////////
void			NAV::TeleportTo(gentity_t* actor, int pointNum)
{
	assert(actor!=0);
	TeleportPlayer(actor, mGraph.get_node(pointNum).mPoint.v, actor->currentAngles);
	return;
}




////////////////////////////////////////////////////////////////////////////////////
// Activate
////////////////////////////////////////////////////////////////////////////////////
void			STEER::Activate(gentity_t* actor)
{
	assert(!Active(actor) && actor && actor->client && actor->NPC);	// Can't Activate If Already Active


// PHASE I - ACTIVATE THE STEER USER FOR THIS ACTOR
//==================================================
	if (mSteerUsers.full())
	{
		assert("STEER: No more unused steer users, possibly change size"==0);
		return;
	}

	// Get A Steer User From The Pool
	//--------------------------------
	int steerUserNum = mSteerUsers.alloc();
	mSteerUserIndex[actor->s.number] = steerUserNum;
	SSteerUser& suser = mSteerUsers[steerUserNum];


// PHASE II - Copy Data For This Actor Into The SUser
//====================================================
	suser.mPosition		= actor->currentOrigin;
	suser.mOrientation	= actor->currentAngles;
	suser.mVelocity		= actor->client->ps.velocity;
	suser.mSpeed		= suser.mVelocity.Len();

	suser.mBlocked		= false;

	suser.mMaxSpeed		= actor->NPC->stats.runSpeed;
	suser.mRadius		= RadiusFromBounds(actor->mins, actor->maxs);
	suser.mMaxForce		= 150.0f;		//STEER_TODO: Get From actor Somehow
	suser.mMass			= 1.0f;			//STEER_TODO: Get From actor Somehow
	if (!(actor->NPC->scriptFlags&SCF_RUNNING) &&
		((actor->NPC->scriptFlags&SCF_WALKING) ||
		 (actor->NPC->aiFlags&NPCAI_WALKING) ||
		 (ucmd.buttons&BUTTON_WALKING)
		))
	{
		suser.mMaxSpeed	= actor->NPC->stats.walkSpeed;
	}

#ifdef _DEBUG
	assert(suser.mPosition.IsFinite());
	assert(suser.mOrientation.IsFinite());
	assert(suser.mVelocity.IsFinite());
#endif


	// Find Our Neighbors
	//--------------------
	suser.mNeighbors.clear();
	float	RangeSize = suser.mRadius + STEER::NEIGHBOR_RANGE;

	CVec3	Range(RangeSize, RangeSize, (actor->client->moveType==MT_FLYSWIM)?(RangeSize):(suser.mRadius*2.0f));
	CVec3	Mins(suser.mPosition - Range);
	CVec3	Maxs(suser.mPosition + Range);

	gentity_t*	EntityList[MAX_GENTITIES];
	gentity_t*	neighbor = 0;

	int	numFound = gi.EntitiesInBox(Mins.v, Maxs.v, EntityList, MAX_GENTITIES);
	for (int i=0; i<numFound; i++)
	{
		neighbor = EntityList[i];
		assert(neighbor!=0);

		if (neighbor->s.number==actor->s.number || neighbor==actor->enemy || !neighbor->client || neighbor->health<=0 || !neighbor->inuse)
		{
			continue;
		}

		suser.mNeighbors.push_back(neighbor);
	}


	// Clear Out Steering, So If No STEER Operations Are Called, Net Effect Is Zero
	//------------------------------------------------------------------------------
	suser.mSteering.Clear();
	suser.mNewtons		= 0.0f;

	VectorClear(actor->client->ps.moveDir);
				actor->client->ps.speed = 0;


// PHASE III - Project The Current Velocity Forward, To The Side, And Onto The Path
//==================================================================================
	suser.mProjectFwd	= suser.mPosition + (suser.mVelocity * 1.0f);
	suser.mProjectSide	=                   (suser.mVelocity * 0.3f);
	suser.mProjectSide.Reposition(suser.mPosition, (actor->NPC->avoidSide==Side_Left)?(40.0f):(-40.0f));


	// STEER_TODO: Project The Point The Path (If The character has one)
	//-------------------------------------------------------------------
	//suser.mProjectPath	= ;

}

//////////////////////////////////////////////////////////////////////
// DeActivate
//
// This function first scales back the composite steering vector to
// the max force range, and if there is any steering left, it
// applies the steering to the velocity.  Velocity is also truncated
// to be less than Max Speed.
//
// Finaly, the results are copied onto the entity and the steer user
// struct is freed for use by another entity.
//////////////////////////////////////////////////////////////////////
void			STEER::DeActivate(gentity_t* actor, usercmd_t* ucmd)
{
	assert(Active(actor) && actor && actor->client && actor->NPC);		// Can't Deactivate If Never Activated
	SSteerUser& suser = mSteerUsers[mSteerUserIndex[actor->s.number]];


#ifdef _DEBUG
	assert(suser.mPosition.IsFinite());
	assert(suser.mOrientation.IsFinite());
	assert(suser.mSteering.IsFinite());
	assert(suser.mMass!=0.0f);
#endif



// PHASE I - TRUNCATE STEERING AND APPLY TO VELOCITY
//===================================================
	suser.mNewtons	 = suser.mSteering.Truncate(suser.mMaxForce);
	if (suser.mNewtons>1E-10)
	{
		suser.mSteering /= suser.mMass;
		suser.mVelocity += suser.mSteering;
		suser.mSpeed	 = suser.mVelocity.Truncate(suser.mMaxSpeed);

		// DEBUG GRAPHICS
		//=================================================================
		if (NAVDEBUG_showCollision)
		{
			CVec3 EndThrust(suser.mPosition+suser.mSteering);
			CVec3 EndVelocity(suser.mPosition+suser.mVelocity);

			CG_DrawEdge(suser.mPosition.v, EndThrust.v,		EDGE_THRUST);
			CG_DrawEdge(suser.mPosition.v, EndVelocity.v,	EDGE_VELOCITY);
		}
		//=================================================================
	}
	if (suser.mSpeed<10.0f)
	{
		suser.mSpeed = 0.0f;
	}







// PHASE II - CONVERT VELOCITY TO MOVE DIRECTION & ANGLES
//========================================================
	if (!NPC_Jumping())
	{
		CVec3	MoveDir(suser.mVelocity);
		CVec3	Angles(actor->NPC->lastPathAngles);

		if (suser.mSpeed>0.0f && MoveDir!=CVec3::mZero)
		{
			MoveDir.Norm();
			CVec3	NewAngles(suser.mVelocity);
			NewAngles.VecToAng();

			Angles = NewAngles;//((Angles + NewAngles)*0.75f);
		}
#ifdef _DEBUG
		assert(MoveDir.IsFinite());
		assert(Angles.IsFinite());
#endif



// PHASE III - ASSIGN ALL THIS DATA TO THE ACTOR ENTITY
//======================================================
		actor->NPC->aiFlags |= NPCAI_NO_SLOWDOWN;

		VectorCopy(MoveDir.v,
			actor->client->ps.moveDir);
			actor->client->ps.speed		= suser.mSpeed;

		VectorCopy(Angles.v,
			actor->NPC->lastPathAngles);
			actor->NPC->desiredPitch	= 0.0f;
			actor->NPC->desiredYaw		= AngleNormalize360(Angles[YAW]);


		// Convert Movement To User Command
		//----------------------------------
		if (suser.mSpeed > 0.0f)
		{
			vec3_t	forward, right, up;
			AngleVectors(actor->currentAngles,	forward, right, up);	// Use Current Angles

			float fDot = Com_Clamp(-127.0f, 127.0f, DotProduct(forward, MoveDir.v)*127.0f);
			float rDot = Com_Clamp(-127.0f, 127.0f, DotProduct(right,   MoveDir.v)*127.0f);

			ucmd->forwardmove	= floor(fDot);
			ucmd->rightmove		= floor(rDot);
			ucmd->upmove		= 0.0f;

			if (suser.mSpeed<(actor->NPC->stats.walkSpeed + 5.0f))
			{
				ucmd->buttons |= BUTTON_WALKING;
			}
			else
			{
				ucmd->buttons &= ~BUTTON_WALKING;
			}

			// Handle Fly Swim Movement
			//--------------------------
			if (actor->client->moveType==MT_FLYSWIM)
			{
				ucmd->forwardmove	= 0.0f;
				ucmd->rightmove		= 0.0f;

				VectorCopy(suser.mVelocity.v, actor->client->ps.velocity);
			}
		}
		else
		{
			ucmd->forwardmove	= 0.0f;
			ucmd->rightmove		= 0.0f;
			ucmd->upmove		= 0.0f;

			// Handle Fly Swim Movement
			//--------------------------
			if (actor->client->moveType==MT_FLYSWIM)
			{
				VectorClear(actor->client->ps.velocity);
			}
		}

		// Slow Down If Going Backwards
		//------------------------------
		if (ucmd->forwardmove<0.0f)
		{
			client->ps.speed	*= 0.75f;
			suser.mSpeed		*= 0.75f;
		}


// PHASE IV - UPDATE BLOCKING INFORMATION
//========================================
		if		(suser.mBlocked)
		{
			// If Not Previously Blocked, Record The Start Time And Any Entites Involved
			//---------------------------------------------------------------------------
			if (!(actor->NPC->aiFlags&NPCAI_BLOCKED))
			{
				actor->NPC->aiFlags				|= NPCAI_BLOCKED;
				actor->NPC->blockedDebounceTime  = level.time;
			}

			// If Navigation Or Steering Had A Target Entity (Trying To Go To), Record That Here
			//-----------------------------------------------------------------------------------
			actor->NPC->blockedTargetEntity  = 0;
			if (suser.mBlockedTgtEntity!=ENTITYNUM_NONE)
			{
				actor->NPC->blockedTargetEntity	 = &g_entities[suser.mBlockedTgtEntity];
			}
			VectorCopy(suser.mBlockedTgtPosition.v, actor->NPC->blockedTargetPosition);
		}
		else
		{
			// Nothing Blocking, So Turn Off The Blocked Stuff If It Is There
			//----------------------------------------------------------------
			if (actor->NPC->aiFlags&NPCAI_BLOCKED)
			{
				actor->NPC->aiFlags				&= ~NPCAI_BLOCKED;
				actor->NPC->blockedDebounceTime  = 0;
				actor->NPC->blockedTargetEntity	 = 0;
			}
		}

		// If We've Been Blocked For More Than 2 Seconds, May Want To Take Corrective Action
		//-----------------------------------------------------------------------------------
		if (STEER::HasBeenBlockedFor(actor, 2000))
		{
			if (NAVDEBUG_showEnemyPath)
			{
				CG_DrawEdge(actor->currentOrigin, actor->NPC->blockedTargetPosition, EDGE_PATHBLOCKED);
				if (actor->waypoint)
				{
					CVec3	Dumb(NAV::GetNodePosition(actor->waypoint));
					CG_DrawEdge(actor->currentOrigin, Dumb.v, EDGE_BLOCKED);
				}
			}

			// If He Can Fly Or Jump, Try That
			//---------------------------------
			if ((actor->NPC->scriptFlags&SCF_NAV_CAN_FLY) ||
				(actor->NPC->scriptFlags&SCF_NAV_CAN_JUMP)
				)
			{
				// Ok, Well, How About Jumping To The Last Waypoint We Had That Was Valid
				//------------------------------------------------------------------------
				if ((STEER::HasBeenBlockedFor(actor, 8000)) &&
					(!actor->waypoint || Distance(NAV::GetNodePosition(actor->waypoint), actor->currentOrigin)>150.0f) &&
					(actor->lastWaypoint))
				{
					if (player &&
						(STEER::HasBeenBlockedFor(actor, 15000)) &&
						!gi.inPVS(player->currentOrigin, NAV::GetNodePosition(actor->lastWaypoint)) &&
						!gi.inPVS(player->currentOrigin, actor->currentOrigin) &&
						!G_CheckInSolidTeleport(NAV::GetNodePosition(actor->lastWaypoint), actor)
						)
					{
						G_SetOrigin(actor, NAV::GetNodePosition(actor->lastWaypoint));
						G_SoundOnEnt( NPC, CHAN_BODY, "sound/weapons/force/jump.wav" );
					}
					else
					{
						NPC_TryJump(NAV::GetNodePosition(actor->lastWaypoint));
					}
				}

				// First, Try Jumping Directly To Our Target Desired Location
				//------------------------------------------------------------
				else
				{
					// If We Had A Target Entity, Try Jumping There
					//----------------------------------------------
					if (NPCInfo->blockedTargetEntity)
					{
						NPC_TryJump(NPCInfo->blockedTargetEntity);
					}

					// Otherwise Try Jumping To The Target Position
					//----------------------------------------------
					else
					{
						NPC_TryJump(NPCInfo->blockedTargetPosition);
					}
				}
			}
		}
	}

// PHASE V - FREE UP THE STEER USER
//===================================
	mSteerUsers.free(mSteerUserIndex[actor->s.number]);
	mSteerUserIndex[actor->s.number] = NULL_STEER_USER_INDEX;
}

//////////////////////////////////////////////////////////////////////
// Active?
//
// Simple way for external systemm to check if this object is active
// already.
//
//////////////////////////////////////////////////////////////////////
bool			STEER::Active(gentity_t* actor)
{
	return (mSteerUserIndex[actor->s.number]!=NULL_STEER_USER_INDEX);
}

////////////////////////////////////////////////////////////////////////////////////
// SafeToGoTo - returns true if it is safe for the actor to steer toward the target
// position
////////////////////////////////////////////////////////////////////////////////////
bool			STEER::SafeToGoTo(gentity_t* actor, const vec3_t& targetPosition, int targetNode)
{
	int		actorNode				= NAV::GetNearestNode(actor, true, targetNode);
	float	actorToTargetDistance	= Distance(actor->currentOrigin,  targetPosition);


	// Are They Close Enough To Just Go There
	//----------------------------------------
	if (actorToTargetDistance<110.0f && fabsf(targetPosition[2]-actor->currentOrigin[2])<50.0f)
	{
		return true;
	}

	// Are They Both Within The Radius Of Their Nearest Nav Point?
	//-------------------------------------------------------------
	if (actorToTargetDistance<500.0f)
	{
		// Are Both Actor And Target In Safe Radius?
		//-------------------------------------------
		if (NAV::OnNeighboringPoints(actorNode, targetNode) &&
			NAV::InSafeRadius(actor->currentOrigin, actorNode,  targetNode) &&
			NAV::InSafeRadius(targetPosition,		targetNode, actorNode))
		{
			return true;
		}

		// If Close Enough, We May Be Able To Go Anyway, So Try A Trace
		//--------------------------------------------------------------
		if (actorToTargetDistance<400.0f)//250.0f)
		{
			if (!TIMER_Done(actor, "SafeToGoToDURATION"))
			{
				return true;
			}

			if (TIMER_Done(actor, "SafeToGoToCHECK"))
			{
				TIMER_Set( actor, "SafeToGoToCHECK", 1500);			// Check Every 1.5 Seconds
				if (MoveTrace(actor, targetPosition, true))
				{
					TIMER_Set(actor, "SafeToGoToDURATION", 2000);	// Safe For 2 Seconds

					if (NAVDEBUG_showCollision)
					{
						CVec3 Dumb(targetPosition);
						CG_DrawEdge(actor->currentOrigin, Dumb.v, EDGE_WHITE_TWOSECOND);
					}
				}
				else
				{
					if (NAVDEBUG_showCollision)
					{
						CVec3 Dumb(targetPosition);
						CG_DrawEdge(actor->currentOrigin, Dumb.v, EDGE_RED_TWOSECOND);
					}
				}
			}
		}
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////
// Master Functions
////////////////////////////////////////////////////////////////////////////////////
bool			STEER::GoTo(gentity_t* actor,  gentity_t* target, float reachedRadius, bool avoidCollisions)
{
	// Can't Steer To A Guy In The Air
	//---------------------------------
	if (!target)
	{
		NAV::ClearPath(actor);
		STEER::Stop(actor);
		return true;
	}

	// If Within Reached Radius, Just Stop Right Here
	//------------------------------------------------
	if (STEER::Reached(actor, target->currentOrigin, reachedRadius, (actor->client && actor->client->moveType==MT_FLYSWIM)))
	{
		NAV::ClearPath(actor);
		STEER::Stop(actor);
		return true;
	}

	// Check To See If It Is Safe To Attempt Steering Toward The Target Position
	//---------------------------------------------------------------------------
	if (
		//(target->client && target->client->ps.groundEntityNum==ENTITYNUM_NONE) ||
		!STEER::SafeToGoTo(actor, target->currentOrigin, NAV::GetNearestNode(target))
 		)
	{
		return false;
	}

	// Ok, It Is, So Clear The Path, And Go Toward Our Target
	//--------------------------------------------------------
	NAV::ClearPath(actor);
	STEER::Persue(actor, target, reachedRadius*4.0f);
	if (avoidCollisions)
	{
		if (STEER::AvoidCollisions(actor, actor->client->leader)!=0.0f)
		{
			STEER::Blocked(actor, target);					// Collision Detected
		}
	}

	if (NAVDEBUG_showEnemyPath)
	{
		CG_DrawEdge(actor->currentOrigin, target->currentOrigin, EDGE_FOLLOWPOS);
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////////
// Master Function- GoTo
////////////////////////////////////////////////////////////////////////////////////
bool			STEER::GoTo(gentity_t* actor, const vec3_t& position, float reachedRadius, bool avoidCollisions)
{
	// If Within Reached Radius, Just Stop Right Here
	//------------------------------------------------
	if (STEER::Reached(actor, position, reachedRadius, (actor->client && actor->client->moveType==MT_FLYSWIM)))
	{
		NAV::ClearPath(actor);
		STEER::Stop(actor);
		return true;
	}

	// Check To See If It Is Safe To Attempt Steering Toward The Target Position
	//---------------------------------------------------------------------------
	if (!STEER::SafeToGoTo(actor, position, NAV::GetNearestNode(position)))
	{
		return false;
	}

	// Ok, It Is, So Clear The Path, And Go Toward Our Target
	//--------------------------------------------------------
	NAV::ClearPath(actor);
	STEER::Seek(actor, position, reachedRadius*2.0f);
	if (avoidCollisions)
	{
		if (STEER::AvoidCollisions(actor, actor->client->leader)!=0.0f)
		{
			STEER::Blocked(actor, position);					// Collision Detected
		}
	}

	if (NAVDEBUG_showEnemyPath)
	{
		CVec3	Dumb(position);
		CG_DrawEdge(actor->currentOrigin, Dumb.v, EDGE_FOLLOWPOS);
	}
	return true;
}


////////////////////////////////////////////////////////////////////////////////////
// Blocked Recorder
////////////////////////////////////////////////////////////////////////////////////
void			STEER::Blocked(gentity_t* actor, gentity_t* target)
{
	assert(Active(actor));
	SSteerUser& suser = mSteerUsers[mSteerUserIndex[actor->s.number]];

	suser.mBlocked				= true;
	suser.mBlockedTgtEntity		= target->s.number;
	suser.mBlockedTgtPosition	= target->currentOrigin;
}

////////////////////////////////////////////////////////////////////////////////////
// Blocked Recorder
////////////////////////////////////////////////////////////////////////////////////
void			STEER::Blocked(gentity_t* actor, const vec3_t& target)
{
	assert(Active(actor));
	SSteerUser& suser = mSteerUsers[mSteerUserIndex[actor->s.number]];

	suser.mBlocked				= true;
	suser.mBlockedTgtEntity		= ENTITYNUM_NONE;
	suser.mBlockedTgtPosition	= target;
}

////////////////////////////////////////////////////////////////////////////////////
// Blocked Recorder
////////////////////////////////////////////////////////////////////////////////////
bool			STEER::HasBeenBlockedFor(gentity_t* actor, int duration)
{
	return (
		(actor->NPC->aiFlags&NPCAI_BLOCKED) &&
		(level.time - actor->NPC->blockedDebounceTime)>duration
		);
}

//////////////////////////////////////////////////////////////////////
// Stop
//
// Just Decelerate.
//
//////////////////////////////////////////////////////////////////////
float			STEER::Stop(gentity_t* actor, float weight)
{
	assert(Active(actor));
	SSteerUser& suser = mSteerUsers[mSteerUserIndex[actor->s.number]];

	suser.mDesiredVelocity.Clear();
	suser.mDistance			=  0.0f;
	suser.mDesiredSpeed		=  0.0f;
	suser.mSteering			+= ((suser.mDesiredVelocity - suser.mVelocity) * weight);

	if (actor->NPC->aiFlags&NPCAI_FLY)
	{
		int nearestNode = NAV::GetNearestNode(actor);
		if (nearestNode>0 && !mGraph.get_node(nearestNode).mFlags.get_bit(CWayNode::WN_FLOATING))
		{
			actor->NPC->aiFlags &= ~NPCAI_FLY;
		}
	}

#ifdef _DEBUG
	assert(suser.mSteering.IsFinite());
#endif

	return 0.0f;
}

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
float			STEER::MatchSpeed(gentity_t* actor, float speed, float weight)
{
	assert(Active(actor));
	SSteerUser& suser = mSteerUsers[mSteerUserIndex[actor->s.number]];

	suser.mDesiredVelocity	= suser.mVelocity;
	suser.mDesiredVelocity.Truncate(speed);
	suser.mDistance			=  0.0f;
	suser.mDesiredSpeed		=  0.0f;
	suser.mSteering			+= ((suser.mDesiredVelocity - suser.mVelocity) * weight);

#ifdef _DEBUG
	assert(suser.mSteering.IsFinite());
#endif

	return 0.0f;

}


////////////////////////////////////////////////////////////////////////////////////
// Seek
//
// The most basic of all steering operations, this function applies a change to
// the steering vector
//
////////////////////////////////////////////////////////////////////////////////////
float			STEER::Seek(gentity_t* actor, const CVec3& pos, float slowingDistance, float weight, float desiredSpeed)
{
	assert(Active(actor));
	SSteerUser& suser = mSteerUsers[mSteerUserIndex[actor->s.number]];

	suser.mSeekLocation		= pos;
	suser.mDesiredVelocity	= suser.mSeekLocation - suser.mPosition;

	//If The Difference In Height Is Small Enough, Just Kill It
	//----------------------------------------------------------
	if (fabsf(suser.mDesiredVelocity[2]) < 10.0f)
	{
		suser.mDesiredVelocity[2] = 0.0f;
	}


	suser.mDistance			=  suser.mDesiredVelocity.SafeNorm();
	if (suser.mDistance>0.0f)
	{
		suser.mDesiredSpeed		=  (desiredSpeed!=0.0f)?(desiredSpeed):(suser.mMaxSpeed);
		if (slowingDistance!=0.0f && suser.mDistance < slowingDistance)
		{
			suser.mDesiredSpeed	*= (suser.mDistance/slowingDistance);
		}
		suser.mDesiredVelocity	*= suser.mDesiredSpeed;
	}
	else
	{
		suser.mDesiredSpeed = 0.0f;
		suser.mDesiredVelocity.Clear();
	}

	suser.mSteering			+= ((suser.mDesiredVelocity - suser.mVelocity) * weight);

#ifdef _DEBUG
	assert(suser.mSteering.IsFinite());
#endif

	return suser.mDistance;
}

////////////////////////////////////////////////////////////////////////////////////
// Flee
//
// Similar to seek, except there is no concept of a slowing distance.  Adds the
// position vectors instead of subtracting them to obtain a desired velocity away
// from the target.
//
////////////////////////////////////////////////////////////////////////////////////
float			STEER::Flee(gentity_t* actor,		const CVec3& pos, float weight)
{
	assert(Active(actor));
	SSteerUser& suser = mSteerUsers[mSteerUserIndex[actor->s.number]];

	suser.mDesiredVelocity	=  suser.mPosition - pos;
	suser.mDistance			=  suser.mDesiredVelocity.SafeNorm();
	suser.mDesiredSpeed		=  suser.mMaxSpeed;
	suser.mDesiredVelocity	*= suser.mDesiredSpeed;
	suser.mSteering			+= ((suser.mDesiredVelocity - suser.mVelocity) * weight);
	suser.mSeekLocation		= pos + suser.mDesiredVelocity;


#ifdef _DEBUG
	assert(suser.mSteering.IsFinite());
#endif

	return suser.mDistance;
}


////////////////////////////////////////////////////////////////////////////////////
// Persue
//
// Predict the target's position and seek that.
//
////////////////////////////////////////////////////////////////////////////////////
float			STEER::Persue(gentity_t* actor,	gentity_t* target, float slowingDistance)
{
	assert(Active(actor) && target);
	SSteerUser& suser = mSteerUsers[mSteerUserIndex[actor->s.number]];

	CVec3	ProjectedTargetPosition(target->currentOrigin);

	if (target->client)
	{
		float	DistToTarget = ProjectedTargetPosition.Dist(suser.mPosition);

		CVec3	TargetVelocity(target->client->ps.velocity);
		float	TargetSpeed = TargetVelocity.SafeNorm();
		if (TargetSpeed>0.0f)
		{
			TargetVelocity *= (DistToTarget + 5.0f);
			ProjectedTargetPosition += TargetVelocity;
		}
	}

	return Seek(actor, ProjectedTargetPosition, slowingDistance);
}


////////////////////////////////////////////////////////////////////////////////////
// Persue
//
// Predict the target's position and seek that.
//
////////////////////////////////////////////////////////////////////////////////////
float			STEER::Persue(gentity_t* actor,	gentity_t* target, float slowingDistance, float offsetForward, float offsetRight, float offsetUp, bool relativeToTargetFacing)
{
	assert(Active(actor) && target);
	SSteerUser& suser = mSteerUsers[mSteerUserIndex[actor->s.number]];

	CVec3	ProjectedTargetPosition(target->currentOrigin);

	// If Target Is A Client, Take His Velocity Into Account And Project His Position
	//--------------------------------------------------------------------------------
	if (target->client)
	{
		float	DistToTarget = ProjectedTargetPosition.Dist(suser.mPosition);

		CVec3	TargetVelocity(target->client->ps.velocity);
		float	TargetSpeed = TargetVelocity.SafeNorm();
	  	if (TargetSpeed>0.0f)
		{
			TargetVelocity[2] *= 0.1f;
			TargetVelocity *= (DistToTarget + 5.0f);
			ProjectedTargetPosition += TargetVelocity;
		}
	}

	// Get The Direction Toward The Target
	//-------------------------------------
	CVec3	DirectionToTarget(ProjectedTargetPosition);
			DirectionToTarget -= suser.mPosition;
			DirectionToTarget.SafeNorm();


	CVec3	ProjectForward(DirectionToTarget);
	CVec3	ProjectRight;
	CVec3	ProjectUp;

	if (relativeToTargetFacing)
	{
		AngleVectors(target->currentAngles, ProjectForward.v, ProjectRight.v, ProjectUp.v);
		if (ProjectRight.Dot(DirectionToTarget)>0.0f)
		{
			ProjectRight *= -1.0f;		// If Going In Same Direction As Target Right, Project Toward Target Left
		}
	}
	else
	{
		MakeNormalVectors(ProjectForward.v, ProjectRight.v, ProjectUp.v);
	}

	ProjectedTargetPosition.ScaleAdd(ProjectForward, offsetForward);
	ProjectedTargetPosition.ScaleAdd(ProjectRight,	 offsetRight);
	ProjectedTargetPosition.ScaleAdd(ProjectUp,		 offsetUp);

	return Seek(actor, ProjectedTargetPosition, slowingDistance);
}


////////////////////////////////////////////////////////////////////////////////////
// Evade
//
// Predict the target's position and flee that.
//
////////////////////////////////////////////////////////////////////////////////////
float			STEER::Evade(gentity_t* actor,		gentity_t* target)
{
	assert(Active(actor) && target);
	SSteerUser& suser = mSteerUsers[mSteerUserIndex[actor->s.number]];

	CVec3	ProjectedTargetPosition(target->currentOrigin);

	if (target->client)
	{
		float	DistToTarget = ProjectedTargetPosition.Dist(suser.mPosition);


		CVec3	TargetVelocity(target->client->ps.velocity);
		float	TargetSpeed = TargetVelocity.SafeNorm();
		if (TargetSpeed>0.0f)
		{
			TargetVelocity *= (DistToTarget + 5.0f);
			ProjectedTargetPosition += TargetVelocity;
		}
	}

	return Flee(actor, ProjectedTargetPosition);
}


////////////////////////////////////////////////////////////////////////////////////
// Separation
////////////////////////////////////////////////////////////////////////////////////
float			STEER::Separation(gentity_t* actor, float Scale)
{
	assert(Active(actor) && actor);
	SSteerUser& suser = mSteerUsers[mSteerUserIndex[actor->s.number]];

	if (!suser.mNeighbors.empty())
	{
		for (int i=0; i<suser.mNeighbors.size(); i++)
		{
			if (suser.mNeighbors[i]->s.number>actor->s.number)
			{
				CVec3	NbrPos(suser.mNeighbors[i]->currentOrigin);
				CVec3	NbrToAct(suser.mPosition - NbrPos);
				float	NbrToActDist = NbrToAct.Len2();
				if (NbrToActDist>1.0f)
				{
					NbrToActDist = (1.0f/NbrToActDist);
					NbrToAct *= NbrToActDist * (suser.mMaxSpeed * 10.0f) * Scale;
					suser.mSteering	+= NbrToAct;

					if (NAVDEBUG_showCollision)
					{
						CVec3	Prj(suser.mPosition + NbrToAct);
						CG_DrawEdge(suser.mPosition.v,	Prj.v, EDGE_IMPACT_POSSIBLE);	// Separation
					}
				}
			}
		}
	}

#ifdef _DEBUG
	assert(suser.mSteering.IsFinite());
#endif

	return 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////
// Alignment
////////////////////////////////////////////////////////////////////////////////////
float			STEER::Alignment(gentity_t* actor, float Scale)
{
	return 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////
// Cohesion
////////////////////////////////////////////////////////////////////////////////////
float			STEER::Cohesion(gentity_t* actor, float Scale)
{
	assert(Active(actor) && actor);
	SSteerUser& suser = mSteerUsers[mSteerUserIndex[actor->s.number]];

	if (!suser.mNeighbors.empty())
	{
		CVec3 AvePosition( 0.0f, 0.0f, 0.0f );
		for (int i=0; i<suser.mNeighbors.size(); i++)
		{
			AvePosition += CVec3(suser.mNeighbors[i]->currentOrigin);
		}
		AvePosition *= 1.0f/suser.mNeighbors.size();
		return Seek(actor, AvePosition);
	}
	return 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////
// Find Nearest Leader
////////////////////////////////////////////////////////////////////////////////////
gentity_t*		STEER::SelectLeader(gentity_t* actor)
{
	assert(Active(actor) && actor);
	SSteerUser& suser = mSteerUsers[mSteerUserIndex[actor->s.number]];

	for (int i=0; i<suser.mNeighbors.size(); i++)
	{
		if (suser.mNeighbors[i]->s.number>actor->s.number && !Q_stricmp(suser.mNeighbors[i]->NPC_type, actor->NPC_type ))
		{
			return suser.mNeighbors[i];
		}

	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////
// Path - Seek to the next position on the path (or jump)
////////////////////////////////////////////////////////////////////////////////////
float			STEER::Path(gentity_t* actor)
{
	if (NAV::HasPath(actor))
	{
		CVec3	NextPosition;
		float	NextSlowingRadius;
		bool	Fly = false;
		bool	Jump = false;

		if (!NAV::NextPosition(actor, NextPosition, NextSlowingRadius, Fly, Jump))
		{
			assert("STEER: Unable to obtain the next path position"==0);
			return 0.0f;
		}

		// Start Flying If Next Point Is In The Air
		//------------------------------------------
		if (Fly)
		{
			actor->NPC->aiFlags |= NPCAI_FLY;
		}

		// Otherwise, If Next Point Is On The Ground, No Need To Fly Any Longer
		//----------------------------------------------------------------------
		else if (actor->NPC->aiFlags&NPCAI_FLY)
		{
			actor->NPC->aiFlags &= ~NPCAI_FLY;
		}

		// Start Jumping If Next Point Is A Jump
		//---------------------------------------
		if (Jump)
		{
			if (NPC_TryJump(NextPosition.v))
			{
				actor->NPC->aiFlags |= NPCAI_JUMP;
				return 1.0f;
			}
		}
		actor->NPC->aiFlags &=~NPCAI_JUMP;


		// Preview His Path
		//------------------
		if (NAVDEBUG_showEnemyPath)
		{
			CVec3	Prev(actor->currentOrigin);
			TPath&	path = mPathUsers[mPathUserIndex[actor->s.number]].mPath;
			for (int i=path.size()-1; i>=0; i--)
			{
				CG_DrawEdge(Prev.v, path[i].mPoint.v, EDGE_PATH);
				Prev = path[i].mPoint;
			}
		}

		// If Jump Was On, But We Reached This Point, It Must Have Failed
		//----------------------------------------------------------------
		if (Jump)
		{
			Stop(actor);
			return 0.0f;	// We've Failed!
		}

		// Otherwise, Go!
		//----------------
		return Seek(actor, NextPosition, NextSlowingRadius);
	}
	return 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////
// Wander
////////////////////////////////////////////////////////////////////////////////////
float			STEER::Wander(gentity_t* actor)
{
	assert(Active(actor) && actor);
	SSteerUser& suser = mSteerUsers[mSteerUserIndex[actor->s.number]];


	CVec3 Direction(CVec3::mX);

	if (suser.mSpeed>0.1f)
	{
		Direction = suser.mVelocity;
		Direction.VecToAng();
		Direction[2] += Q_irand(-5, 5);
		Direction.AngToVec();
	}

	Direction *= 70.0f;
	Seek(actor, suser.mPosition+Direction);

	return 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////
// Follow A Leader Entity
////////////////////////////////////////////////////////////////////////////////////
float			STEER::FollowLeader(gentity_t* actor, gentity_t* leader, float dist)
{
	assert(Active(actor) && actor && leader && leader->client);
	SSteerUser& suser = mSteerUsers[mSteerUserIndex[actor->s.number]];


	float	LeaderSpeed		= leader->resultspeed;
	int		TimeRemaining	= (leader->followPosRecalcTime - level.time);

	if (TimeRemaining<0 || (LeaderSpeed>0.0f && TimeRemaining>1000))
	{
		CVec3	LeaderPosition(leader->currentOrigin);
		CVec3	LeaderDirection(leader->currentAngles);
		LeaderDirection.pitch() = 0;
		LeaderDirection.AngToVec();

		if (!actor->enemy && !leader->enemy)
		{
			LeaderDirection = (LeaderPosition - suser.mPosition);
			LeaderDirection.Norm();
		}

		CVec3	FollowPosition(LeaderDirection);
		FollowPosition *= (-1.0f * (fabsf(dist)+suser.mRadius));
		FollowPosition += LeaderPosition;

		MoveTrace(leader, FollowPosition, true);
		if (mMoveTrace.fraction>0.1)
		{
			FollowPosition = mMoveTrace.endpos;
			FollowPosition += (LeaderDirection * suser.mRadius);

			VectorCopy(FollowPosition.v, leader->followPos);
			leader->followPosWaypoint = NAV::GetNearestNode(leader->followPos, leader->waypoint, 0, leader->s.number);
		}

		float	MaxSpeed	= (g_speed->value);
		if (LeaderSpeed>MaxSpeed)
		{
			MaxSpeed = LeaderSpeed;
		}
		float	SpeedScale	= (1.0f - (LeaderSpeed / MaxSpeed));

		leader->followPosRecalcTime =
			(level.time) +
			(Q_irand(50, 500)) +
			(SpeedScale * Q_irand(3000, 8000)) +
			((!actor->enemy && !leader->enemy)?(Q_irand(8000, 15000)):(0));
	}

	if (NAVDEBUG_showEnemyPath)
	{
		CG_DrawEdge(leader->currentOrigin, leader->followPos, EDGE_FOLLOWPOS);
	}

	return 0.0;
}


//////////////////////////////////////////////////////////////////////
// Test For A Collision
//
// This is a helper function used by collision avoidance.
//
// Returns true when a collision is detected (not safe)
//
//////////////////////////////////////////////////////////////////////
bool		TestCollision(gentity_t* actor, SSteerUser& suser, const CVec3& ProjectVelocity, float ProjectSpeed, ESide Side, float weight=1.0f)
{
	// Test To See If The Projected Position Is Safe
	//-----------------------------------------------
	bool Safe = (Side==Side_None)?(MoveTrace(actor, suser.mProjectFwd)):(MoveTrace(actor, suser.mProjectSide));
	if (mMoveTrace.entityNum!=ENTITYNUM_NONE && mMoveTrace.entityNum!=ENTITYNUM_WORLD)
	{
		// The Ignore Entity Is Safe
		//---------------------------
		if (mMoveTrace.entityNum==suser.mIgnoreEntity)
		{
			Safe = true;
		}

		// Doors Are Always Safe
		//-----------------------
		if (g_entities[mMoveTrace.entityNum].classname &&
			Q_stricmp(g_entities[mMoveTrace.entityNum].classname, "func_door")==0)
		{
			Safe = true;
		}

		// If It's Breakable And We Can Go Through It, Then That's Safe Too
		//------------------------------------------------------------------
		if ((actor->NPC->aiFlags&NPCAI_NAV_THROUGH_BREAKABLES) &&
			G_EntIsBreakable(mMoveTrace.entityNum, actor))
		{
			Safe = true;
		}

		// TODO: Put Other Ignore Cases Below
		//------------------------------------
	}

	// Need These Vectors To Draw The Lines Below
	//--------------------------------------------
	CVec3	ContactNormal(mMoveTrace.plane.normal);
	CVec3	ContactPoint( mMoveTrace.endpos);
	int		ContactNum =  mMoveTrace.entityNum;


	if (!Safe && Side==Side_None)
	{
		// Did We Hit A Client?
		//----------------------
		if (ContactNum!=ENTITYNUM_WORLD && ContactNum!=ENTITYNUM_NONE && g_entities[ContactNum].client!=0)
		{
			gentity_t*		Contact			= &g_entities[ContactNum];
			//bool			ContactIsPlayer = (Contact->client->ps.clientNum==0);
			CVec3			ContactVelocity   (Contact->client->ps.velocity);
			CVec3			ContactPosition   (Contact->currentOrigin);
			float			ContactSpeed	= (ContactVelocity.Len());

			// If He Is Moving, We Might Be Able To Just Slow Down Some And Stay Behind Him
			//------------------------------------------------------------------------------
			if (ContactSpeed>0.01f)
			{
				if (ContactSpeed<ProjectSpeed)
				{
					CVec3	MyDirection(ProjectVelocity);
					CVec3	ContactDirection(ContactVelocity);

					ContactDirection.Norm();
					MyDirection.Norm();

					float	DirectionSimilarity = fabsf(MyDirection.Dot(ContactDirection));
					if (DirectionSimilarity>0.5)
					{
						// Match Speed
						//-------------
						suser.mDesiredVelocity	= suser.mVelocity;
						suser.mDesiredVelocity.Truncate(ContactSpeed);
						suser.mSteering			+= ((suser.mDesiredVelocity - ProjectVelocity) * DirectionSimilarity);
						suser.mIgnoreEntity		= ContactNum;	// So The Side Trace Does Not Care About This Guy
#ifdef _DEBUG
						assert(suser.mSteering.IsFinite());
#endif
						Safe = true;	// We'll Say It's Safe For Now
					}
				}
			}

			// Ok, He's Just Standing There...
			//---------------------------------
			else
			{
				CVec3	Next(suser.mSeekLocation);
				if (NAV::HasPath(actor))
				{
					Next = CVec3(NAV::NextPosition(actor));
				}

				CVec3 AbsMin(Contact->absmin);
				CVec3 AbsMax(Contact->absmax);

				// Is The Contact Standing Over Our Next Position?
				//-------------------------------------------------
				if (Next>AbsMin && Next<AbsMax)
				{
					// Ok, Just Give Up And Stop For Now
					//-----------------------------------
					suser.mSteering			-= ProjectVelocity;
					suser.mIgnoreEntity		= ContactNum;
#ifdef _DEBUG
					assert(suser.mSteering.IsFinite());
#endif

					Safe = true;	// We say it is "Safe" because We Don't Want To Try And Steer Around
				}
			}
		}

		// Ignore Shallow Slopes
		//-----------------------
		if (!Safe && ContactNormal[2]>0.0f && ContactNormal[2]<MIN_WALK_NORMAL)
		{
			Safe = true;
		}

		// If Still Not Safe
		//-------------------
		if (!Safe)
		{
			// Normalize Projected Velocity And Dot It With The Contact Normal
			//-----------------------------------------------------------------
			CVec3	ProjectDirection(ProjectVelocity);
			ProjectDirection.Norm();

			// Cross The Contact Normal With Z In Order To Get A Parallel Vector
			//-------------------------------------------------------------------
			ContactNormal.Cross(CVec3::mZ);

			// Only Force A Particular Steering Side For A Max Of 2 Seconds, Then Retest Dot Product
			//---------------------------------------------------------------------------------------
			if (actor->NPC->lastAvoidSteerSide!=Side_None && actor->NPC->lastAvoidSteerSideDebouncer<level.time)
			{
				actor->NPC->lastAvoidSteerSide				= Side_None;
				actor->NPC->lastAvoidSteerSideDebouncer		= level.time + Q_irand(500, STEER::SIDE_LOCKED_TIMER);
			}

			// Make Sure The Normal Is Going The Same Way As The Velocity
			//-----------------------------------------------------------
			if (((ESide)(actor->NPC->lastAvoidSteerSide)==Side_Right) ||
				((ESide)(actor->NPC->lastAvoidSteerSide)==Side_None && ContactNormal.Dot(ProjectDirection)<0.0f))
			{
				ContactNormal *= -1.0f;
				actor->NPC->lastAvoidSteerSide  = Side_Right;
			}
			else
			{
				actor->NPC->lastAvoidSteerSide  = Side_Left;
			}
			ContactNormal[2] = 0.0f;

			// Scale Up The Normal A Bit And Translate The Contact Point Out
			//---------------------------------------------------------------
			ContactNormal *= (ProjectSpeed * weight * 0.5);
			ContactPoint += ContactNormal;

			// Move Toward The Contact Point
			//-------------------------------
			STEER::Seek(actor, ContactPoint, weight);
		}

		// If It Was Safe, Reset Our Avoid Side Data
		//-------------------------------------------
		else if (Side==Side_None)
		{
			actor->NPC->lastAvoidSteerSide = Side_None;
		}
	}





	// DEBUG GRAPHICS
	//=================================================================
	if (NAVDEBUG_showCollision)
	{
		CVec3	Prj((Side==Side_None)?(suser.mProjectFwd):(suser.mProjectSide));

		if (Safe)
		{
			CG_DrawEdge(suser.mPosition.v,	Prj.v,				EDGE_IMPACT_SAFE);		// WHITE LINE
		}
		else
		{
			CG_DrawEdge(suser.mPosition.v,	mMoveTrace.endpos,	EDGE_IMPACT_POSSIBLE);	// RED LINE
			CG_DrawEdge(mMoveTrace.endpos,	ContactPoint.v,		EDGE_IMPACT_POSSIBLE);	// RED LINE
		}
	}
	//=================================================================

	return !Safe;
}

////////////////////////////////////////////////////////////////////////////////////
// Collision Avoidance
//
// Usually the last steering operation to call before finialization, this operation
// attempts to avoid collisions with nearby entities and architecture by thrusing
// away from them.
////////////////////////////////////////////////////////////////////////////////////
float			STEER::AvoidCollisions(gentity_t* actor, gentity_t* leader)
{
	assert(Active(actor) && actor && actor->client);
	SSteerUser& suser = mSteerUsers[mSteerUserIndex[actor->s.number]];

	suser.mIgnoreEntity = -5;


	// Simulate The Results Of Any Current Steering To The Velocity
	//--------------------------------------------------------------
	CVec3	ProjectedSteering(suser.mSteering);
	CVec3	ProjectedVelocity(suser.mVelocity);
	float	ProjectedSpeed = suser.mSpeed;
	float	Newtons;

	Newtons	 = ProjectedSteering.Truncate(suser.mMaxForce);
	if (Newtons>1E-10)
	{
		ProjectedSteering	/= suser.mMass;
		ProjectedVelocity	+= ProjectedSteering;
		ProjectedSpeed		=  ProjectedVelocity.Truncate(suser.mMaxSpeed);
	}


	// Select An Ignore Entity
	//-------------------------
	if (actor->NPC->behaviorState!=BS_CINEMATIC)
	{
		if (actor->NPC->greetEnt && actor->NPC->greetEnt->owner==NPC)
		{
			suser.mIgnoreEntity = actor->NPC->greetEnt->s.clientNum;
		}
		else if (actor->enemy)
		{
			suser.mIgnoreEntity = actor->enemy->s.clientNum;
		}
		else if (leader)
		{
			suser.mIgnoreEntity = leader->s.clientNum;
		}
	}


	// If Moving
	//-----------
	if (ProjectedSpeed>0.01f)
	{
		CVec3	ProjectVelocitySide(ProjectedVelocity);
		ProjectVelocitySide.Reposition(CVec3::mZero, (actor->NPC->avoidSide==Side_Left)?(40.0f):(-40.0f));

		// Project Based On Our ProjectedVelocity
		//-----------------------------------------
		suser.mProjectFwd	= suser.mPosition + (ProjectedVelocity * 1.0f);
		suser.mProjectSide	= suser.mPosition + (ProjectVelocitySide * 0.3f);

		// Test For Collisions In The Front And On The Sides
		//---------------------------------------------------
		bool	HitFront	= TestCollision(actor, suser, ProjectedVelocity, ProjectedSpeed, Side_None, 1.0f);
		bool	HitSide		= TestCollision(actor, suser, ProjectedVelocity, ProjectedSpeed, (ESide)actor->NPC->avoidSide, 0.5f);
		if (!HitSide)
		{
			// If The Side Is Clear, Try The Other Side
			//------------------------------------------
			actor->NPC->avoidSide = (actor->NPC->avoidSide==Side_Left)?(Side_Right):(Side_Left);
		}

		// Hit Something!
		//----------------
		if (HitFront || HitSide)
		{
			return ProjectedSpeed;
		}
	}
	return 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////
// Reached
////////////////////////////////////////////////////////////////////////////////////
bool	STEER::Reached(gentity_t* actor, gentity_t* target, float targetRadius, bool flying)
{
	if (!actor || !target)
	{
		return false;
	}

	CVec3	ActorPos(actor->currentOrigin);
	CVec3	ActorMins(actor->absmin);
	CVec3	ActorMaxs(actor->absmax);

	CVec3	TargetPos(target->currentOrigin);

	if (TargetPos.Dist2(ActorPos)<(targetRadius*targetRadius) || (TargetPos>ActorMins && TargetPos<ActorMaxs))
	{
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////
// Reached
////////////////////////////////////////////////////////////////////////////////////
bool	STEER::Reached(gentity_t* actor, NAV::TNodeHandle target, float targetRadius, bool flying)
{
	if (!actor || !target)
	{
		return false;
	}

	CVec3	ActorPos(actor->currentOrigin);
	CVec3	ActorMins(actor->absmin);
	CVec3	ActorMaxs(actor->absmax);

	CVec3	TargetPos(NAV::GetNodePosition(target));

	if (TargetPos.Dist2(ActorPos)<(targetRadius*targetRadius) || (TargetPos>ActorMins && TargetPos<ActorMaxs))
	{
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////
// Reached
////////////////////////////////////////////////////////////////////////////////////
bool	STEER::Reached(gentity_t* actor, const vec3_t& target, float targetRadius, bool flying)
{
	if (!actor || !target)
	{
		return false;
	}

	CVec3	ActorPos(actor->currentOrigin);
	CVec3	ActorMins(actor->absmin);
	CVec3	ActorMaxs(actor->absmax);

	CVec3	TargetPos(target);

	if (TargetPos.Dist2(ActorPos)<(targetRadius*targetRadius) || (TargetPos>ActorMins && TargetPos<ActorMaxs))
	{
		return true;
	}

//	if (target->client && target->client->ps.groundEntityNum == ENTITYNUM_NONE)
//	{
//		TargetPos -= ActorPos;
//		if (fabsf(TargetPos[2]<(targetRadius*8)))
//		{
//			TargetPos[2] = 0.0f;
//			if (TargetPos.Len2()<((targetRadius*2.0f)*(targetRadius*2.0f)))
//			{
//				return true;
//			}
//		}
//	}

	return false;
}



// Clean up all of the krufty structures that only grow, never shrink, eventually
// causing asserts and subsequent memory trashing.
void	ClearAllNavStructures(void)
{
	TEntEdgeMap::iterator i = mEntEdgeMap.begin();
	for ( ; i != mEntEdgeMap.end(); i++)
	{
		i->clear();
	}
	mEntEdgeMap.clear();
}







