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
// NAVIGATOR
// ---------
// This file provides an interface to two actor related systems:
//  - Path Finding
//  - Steering
//
//
//
////////////////////////////////////////////////////////////////////////////////////////
#ifndef __G_NAVIGATOR__
#define __G_NAVIGATOR__


#define	USENEWNAVSYSTEM	1


#if !defined(RAVL_VEC_INC)
	#include "../Ravl/CVec.h"
#endif


////////////////////////////////////////////////////////////////////////////////////////
// The NAV Namespace
//
// This namespace provides the public interface to the NPC Navigation and Pathfinding
// system.  This system is a bidirectional graph of nodes and weighted edges.  Finding
// a path from one node to another is accomplished with A*, and cached internally for
// each actor who requests a path.
////////////////////////////////////////////////////////////////////////////////////////
namespace NAV
{
	typedef		int		TNodeHandle;
	typedef		int		TEdgeHandle;


	enum	EPointType
	{
		PT_NONE = 0,

		PT_WAYNODE,
		PT_COMBATNODE,
		PT_GOALNODE,

		PT_MAX
	};




	////////////////////////////////////////////////////////////////////////////////////
	// Save, Load, Construct
	////////////////////////////////////////////////////////////////////////////////////
	bool			LoadFromFile(const char *filename, int checksum);
	bool			TestEdge( TNodeHandle NodeA, TNodeHandle NodeB, qboolean IsDebugEdge );
	bool			LoadFromEntitiesAndSaveToFile(const char *filename, int checksum);
	void			SpawnedPoint(gentity_t* ent, EPointType type=PT_WAYNODE);


	////////////////////////////////////////////////////////////////////////////////////
	// Finding Nav Points
	////////////////////////////////////////////////////////////////////////////////////
	TNodeHandle		GetNearestNode(gentity_t* ent, bool forceRecalcNow=false, NAV::TNodeHandle goal=0);
	TNodeHandle		GetNearestNode(const vec3_t& position, TNodeHandle previous=0, NAV::TNodeHandle goal=0, int ignoreEnt=ENTITYNUM_NONE, bool allowZOffset=false);

	TNodeHandle		ChooseRandomNeighbor(TNodeHandle NodeHandle);
	TNodeHandle		ChooseRandomNeighbor(TNodeHandle NodeHandle, const vec3_t& position, float maxDistance);
	TNodeHandle		ChooseClosestNeighbor(TNodeHandle NodeHandle, const vec3_t& position);
	TNodeHandle		ChooseFarthestNeighbor(TNodeHandle NodeHandle, const vec3_t& position);
	TNodeHandle		ChooseFarthestNeighbor(gentity_t* actor, const vec3_t& target, float maxSafeDot);


	////////////////////////////////////////////////////////////////////////////////////
	// Get The Location Of A Given Node Handle
	////////////////////////////////////////////////////////////////////////////////////
	const vec3_t&	GetNodePosition(TNodeHandle NodeHandle);
	void			GetNodePosition(TNodeHandle NodeHandle, vec3_t& position);


	////////////////////////////////////////////////////////////////////////////////////
	// Testing Nearness
	////////////////////////////////////////////////////////////////////////////////////
	float			EstimateCostToGoal(const vec3_t& position, TNodeHandle Goal);
	float			EstimateCostToGoal(TNodeHandle Start, TNodeHandle Goal);

	bool			OnSamePoint(gentity_t* actor, gentity_t* target);
	bool			OnNeighboringPoints(TNodeHandle A, TNodeHandle B);
	bool			OnNeighboringPoints(gentity_t* actor, gentity_t* target);
	bool			OnNeighboringPoints(gentity_t* actor, const vec3_t& position);
	bool			InSameRegion(gentity_t* actor, gentity_t* target);
	bool			InSameRegion(gentity_t* actor, const vec3_t& position);
	bool			InSameRegion(TNodeHandle A, TNodeHandle B);

	bool			InSafeRadius(CVec3 at, TNodeHandle atNode, TNodeHandle targetNode=0);

	////////////////////////////////////////////////////////////////////////////////////
	// Finding A Path
	////////////////////////////////////////////////////////////////////////////////////
	bool			GoTo(gentity_t* actor, TNodeHandle target, float MaxDangerLevel=1.0f);
	bool			GoTo(gentity_t* actor, gentity_t* target, float MaxDangerLevel=1.0f);
	bool			GoTo(gentity_t* actor, const vec3_t& position, float MaxDangerLevel=1.0f);

	bool			FindPath(gentity_t* actor, TNodeHandle target, float MaxDangerLevel=1.0f);
	bool			FindPath(gentity_t* actor, gentity_t* target, float MaxDangerLevel=1.0f);
	bool			FindPath(gentity_t* actor, const vec3_t& position, float MaxDangerLevel=1.0f);

	bool			SafePathExists(const CVec3& start, const CVec3& stop, const CVec3& danger, float dangerDistSq);

	bool			HasPath(gentity_t* actor, TNodeHandle target=PT_NONE);
	void			ClearPath(gentity_t* actor);
	bool			UpdatePath(gentity_t* actor, TNodeHandle target=PT_NONE, float MaxDangerLevel=1.0f);
	float			PathDangerLevel(gentity_t* actor);
	int				PathNodesRemaining(gentity_t* actor);

	const vec3_t&	NextPosition(gentity_t* actor);
	bool			NextPosition(gentity_t* actor, CVec3& Position);
	bool			NextPosition(gentity_t* actor, CVec3& Position, float& SlowingRadius, bool& Fly, bool& Jump);


	////////////////////////////////////////////////////////////////////////////////////
	// Update One Or More Edges As A Result Of An Entity Getting Removed
	////////////////////////////////////////////////////////////////////////////////////
	void			WayEdgesNowClear(gentity_t* ent);


	////////////////////////////////////////////////////////////////////////////////////
	// How Big Is The Given Ent
	////////////////////////////////////////////////////////////////////////////////////
	unsigned int	ClassifyEntSize(gentity_t* ent);
	void			RegisterDangerSense(gentity_t* actor, int alertEventIndex);
	void			DecayDangerSenses();


	////////////////////////////////////////////////////////////////////////////////////
	// Debugging Information
	////////////////////////////////////////////////////////////////////////////////////
	void			ShowDebugInfo(const vec3_t& PlayerPosition, TNodeHandle PlayerWaypoint);
	void			ShowStats();

	void			TeleportTo(gentity_t* actor, const char* pointName);
	void			TeleportTo(gentity_t* actor, int pointNum);
}





////////////////////////////////////////////////////////////////////////////////////////
// The STEER Namespace
//
// These functions allow access to the steering system.
//
// The Reset() and Finalize() functions MUST be called before and after any other steering
// operations.  Beyond that, all other steering operations can be called in any order
// and any number of times.  Once Finalize() is called, the results of all these
// operations will be summed up and applied as accelleration to the actor's velocity.
////////////////////////////////////////////////////////////////////////////////////////
namespace STEER
{
	////////////////////////////////////////////////////////////////////////////////////
	// Reset & Finalize
	//
	// Call these two operations before and after all other STEER operations.  They
	// clear out and setup the thrust vector for use by the entity.
	////////////////////////////////////////////////////////////////////////////////////
	void			Activate(gentity_t* actor);
	void			DeActivate(gentity_t* actor, usercmd_t* ucmd);
	bool			Active(gentity_t* actor);


	////////////////////////////////////////////////////////////////////////////////////
	// Master Functions
	////////////////////////////////////////////////////////////////////////////////////
	bool			GoTo(gentity_t* actor, gentity_t* target, float reachedRadius, bool avoidCollisions=true);
	bool			GoTo(gentity_t* actor, const vec3_t& position, float reachedRadius, bool avoidCollisions=true);

	bool			SafeToGoTo(gentity_t* actor, const vec3_t& targetPosition, int targetNode);


	////////////////////////////////////////////////////////////////////////////////////
	// Stop
	//
	// Slow down and come to a stop.
	//
	////////////////////////////////////////////////////////////////////////////////////
	float			Stop(gentity_t* actor, float weight=1.0f);
	float			MatchSpeed(gentity_t* actor, float speed, float weight=1.0f);


	////////////////////////////////////////////////////////////////////////////////////
	// Seek & Flee
	//
	// These two operations form the root of all steering.  They do simple
	// vector operations and add to the thrust vector.
	////////////////////////////////////////////////////////////////////////////////////
	float			Seek(gentity_t* actor,		const CVec3& pos, float slowingDistance=0.0f,  float weight=1.0f, float desiredSpeed=0.0f);
	float			Flee(gentity_t* actor,		const CVec3& pos, float weight=1.0f);

	////////////////////////////////////////////////////////////////////////////////////
	// Persue & Evade
	//
	// Slightly more complicated than Seek & Flee, these operations predict the position
	// of the target entitiy.
	////////////////////////////////////////////////////////////////////////////////////
	float			Persue(gentity_t* actor,	gentity_t* target, float slowingDistance);
	float			Persue(gentity_t* actor,	gentity_t* target, float slowingDistance, float offsetForward, float offsetRight=0.0f, float offsetUp=0.0f, bool relativeToTargetFacing=false);
	float			Evade(gentity_t* actor,		gentity_t* target);

	////////////////////////////////////////////////////////////////////////////////////
	// Separation, Alignment, Cohesion
	//
	// These standard steering operations will apply thrust to achieve a group oriented
	// position or direction.
	////////////////////////////////////////////////////////////////////////////////////
	float			Separation(gentity_t* actor, float Scale=1.0f);
	float			Alignment(gentity_t* actor, float Scale=1.0f);
	float			Cohesion(gentity_t* actor, float Scale=1.0f);

	////////////////////////////////////////////////////////////////////////////////////
	// Wander & Path
	//
	// By far the most common way to alter a character's thrust, path maintaines motion
	// along a navigational path (see NAV namespace), and a random wander path.
	////////////////////////////////////////////////////////////////////////////////////
	float			Path(gentity_t* actor);
	float			Wander(gentity_t* actor);
	float			FollowLeader(gentity_t* actor, gentity_t* leader, float dist);


	////////////////////////////////////////////////////////////////////////////////////
	// Collision Avoidance
	//
	// Usually the last steering operation to call before finialization, this operation
	// attempts to avoid collisions with nearby entities and architecture by thrusing
	// away from them.
	////////////////////////////////////////////////////////////////////////////////////
	float			AvoidCollisions(gentity_t* actor, gentity_t* leader=0);
	gentity_t*		SelectLeader(gentity_t* actor);

	////////////////////////////////////////////////////////////////////////////////////
	// Blocked
	//
	// This function records whether AI is blocked while the steering is active
	////////////////////////////////////////////////////////////////////////////////////
	void			Blocked(gentity_t* actor, gentity_t* target);
	void			Blocked(gentity_t* actor, const vec3_t& target);
	bool			HasBeenBlockedFor(gentity_t* actor, int duration);


	////////////////////////////////////////////////////////////////////////////////////
	// Reached
	//
	// A quick function to see if a target location has been reached by an actor
	////////////////////////////////////////////////////////////////////////////////////
	bool			Reached(gentity_t* actor, gentity_t* target,  float targetRadius, bool flying=false);
	bool			Reached(gentity_t* actor, NAV::TNodeHandle target, float targetRadius, bool flying=false);
	bool			Reached(gentity_t* actor, const vec3_t& target, float targetRadius, bool flying=false);
}





#endif	//__G_NAVIGATOR__