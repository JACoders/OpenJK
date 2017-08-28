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
// Troopers
//
// TODO
// ----
//
//
//
//

////////////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////////////
#include "b_local.h"
#include "g_navigator.h"
#if !defined(RAVL_VEC_INC)
	#include "../Ravl/CVec.h"
#endif
#if !defined(RATL_ARRAY_VS_INC)
	#include "../Ratl/array_vs.h"
#endif
#if !defined(RATL_VECTOR_VS_INC)
	#include "../Ratl/vector_vs.h"
#endif
#if !defined(RATL_HANDLE_POOL_VS_INC)
	#include "../Ratl/handle_pool_vs.h"
#endif
#if !defined(RUFL_HSTRING_INC)
	#include "../Rufl/hstring.h"
#endif


////////////////////////////////////////////////////////////////////////////////////////
// Defines
////////////////////////////////////////////////////////////////////////////////////////
#define		MAX_TROOPS				100
#define		MAX_ENTS_PER_TROOP		7
#define		MAX_TROOP_JOIN_DIST2	1000000  //1000 units
#define		MAX_TROOP_MERGE_DIST2	250000   //500 units
#define		TARGET_POS_VISITED		10000    //100 units


bool NPC_IsTrooper(gentity_t* actor);

enum
{
	SPEECH_CHASE,
	SPEECH_CONFUSED,
	SPEECH_COVER,
	SPEECH_DETECTED,
	SPEECH_GIVEUP,
	SPEECH_LOOK,
	SPEECH_LOST,
	SPEECH_OUTFLANK,
	SPEECH_ESCAPING,
	SPEECH_SIGHT,
	SPEECH_SOUND,
	SPEECH_SUSPICIOUS,
	SPEECH_YELL,
	SPEECH_PUSHED
};
extern void G_AddVoiceEvent( gentity_t *self, int event, int speakDebounceTime );
extern void CG_DrawEdge( vec3_t start, vec3_t end, int type );
static void HT_Speech( gentity_t *self, int speechType, float failChance )
{
	if ( Q_flrand(0.0f, 1.0f) < failChance )
	{
		return;
	}

	if ( failChance >= 0 )
	{//a negative failChance makes it always talk
		if ( self->NPC->group )
		{//group AI speech debounce timer
			if ( self->NPC->group->speechDebounceTime > level.time )
			{
				return;
			}
			/*
			else if ( !self->NPC->group->enemy )
			{
				if ( groupSpeechDebounceTime[self->client->playerTeam] > level.time )
				{
					return;
				}
			}
			*/
		}
		else if ( !TIMER_Done( self, "chatter" ) )
		{//personal timer
			return;
		}
	}

	TIMER_Set( self, "chatter", Q_irand( 2000, 4000 ) );

	if ( self->NPC->blockedSpeechDebounceTime > level.time )
	{
		return;
	}

	switch( speechType )
	{
	case SPEECH_CHASE:
		G_AddVoiceEvent( self, Q_irand(EV_CHASE1, EV_CHASE3), 2000 );
		break;
	case SPEECH_CONFUSED:
		G_AddVoiceEvent( self, Q_irand(EV_CONFUSE1, EV_CONFUSE3), 2000 );
		break;
	case SPEECH_COVER:
		G_AddVoiceEvent( self, Q_irand(EV_COVER1, EV_COVER5), 2000 );
		break;
	case SPEECH_DETECTED:
		G_AddVoiceEvent( self, Q_irand(EV_DETECTED1, EV_DETECTED5), 2000 );
		break;
	case SPEECH_GIVEUP:
		G_AddVoiceEvent( self, Q_irand(EV_GIVEUP1, EV_GIVEUP4), 2000 );
		break;
	case SPEECH_LOOK:
		G_AddVoiceEvent( self, Q_irand(EV_LOOK1, EV_LOOK2), 2000 );
		break;
	case SPEECH_LOST:
		G_AddVoiceEvent( self, EV_LOST1, 2000 );
		break;
	case SPEECH_OUTFLANK:
		G_AddVoiceEvent( self, Q_irand(EV_OUTFLANK1, EV_OUTFLANK2), 2000 );
		break;
	case SPEECH_ESCAPING:
		G_AddVoiceEvent( self, Q_irand(EV_ESCAPING1, EV_ESCAPING3), 2000 );
		break;
	case SPEECH_SIGHT:
		G_AddVoiceEvent( self, Q_irand(EV_SIGHT1, EV_SIGHT3), 2000 );
		break;
	case SPEECH_SOUND:
		G_AddVoiceEvent( self, Q_irand(EV_SOUND1, EV_SOUND3), 2000 );
		break;
	case SPEECH_SUSPICIOUS:
		G_AddVoiceEvent( self, Q_irand(EV_SUSPICIOUS1, EV_SUSPICIOUS5), 2000 );
		break;
	case SPEECH_YELL:
		G_AddVoiceEvent( self, Q_irand( EV_ANGER1, EV_ANGER3 ), 2000 );
		break;
	case SPEECH_PUSHED:
		G_AddVoiceEvent( self, Q_irand( EV_PUSHED1, EV_PUSHED3 ), 2000 );
		break;
	default:
		break;
	}

	self->NPC->blockedSpeechDebounceTime = level.time + 2000;
}




////////////////////////////////////////////////////////////////////////////////////////
// The Troop
//
// Troopers primarly derive their behavior from cooperation as a collective group of
// individuals.  They join Troops, each of which has a leader responsible for direcing
// the movement of the rest of the group.
//
////////////////////////////////////////////////////////////////////////////////////////
class	CTroop
{
	////////////////////////////////////////////////////////////////////////////////////
	// Various Troop Wide Data
	////////////////////////////////////////////////////////////////////////////////////
	int				mTroopHandle;
	int				mTroopTeam;
	bool			mTroopReform;

	float			mFormSpacingFwd;
	float			mFormSpacingRight;

public:
	bool	Empty()					{return mActors.empty();}
	int		Team()					{return mTroopTeam;}
	int		Handle()				{return mTroopHandle;}

	////////////////////////////////////////////////////////////////////////////////////
	// Initialize - Clear out all data, all actors, reset all variables
	////////////////////////////////////////////////////////////////////////////////////
	void	Initialize(int TroopHandle=0)
	{
		mActors.clear();
		mTarget			= 0;
		mState			= TS_NONE;
		mTroopHandle	= TroopHandle;
		mTroopTeam		= 0;
		mTroopReform	= false;
	}
	////////////////////////////////////////////////////////////////////////////////////
	// DistanceSq - Quick Operation to see how far an ent is from the rest of the troop
	////////////////////////////////////////////////////////////////////////////////////
	float	DistanceSq(gentity_t* ent)
	{
		if (mActors.size())
		{
			return DistanceSquared(ent->currentOrigin, mActors[0]->currentOrigin);
		}
		return 0.0f;
	}










private:
	////////////////////////////////////////////////////////////////////////////////////
	// The Actors
	//
	// Actors are all the troopers who belong to the group, their positions in this
	// vector affect their positions in the troop, whith the first actor as the leader
	////////////////////////////////////////////////////////////////////////////////////
	ratl::vector_vs<gentity_t*, MAX_ENTS_PER_TROOP>	mActors;

	////////////////////////////////////////////////////////////////////////////////////
	// MakeActorLeader - Move A Given Index To A Leader Position
	////////////////////////////////////////////////////////////////////////////////////
	void		MakeActorLeader(int index)
	{
		if (index!=0)
		{
			mActors[0]->client->leader = 0;
			mActors.swap(index, 0);
		}
		mActors[0]->client->leader = mActors[0];
		if (mActors[0])
		{
			if (mActors[0]->client->NPC_class==CLASS_HAZARD_TROOPER)
			{
				mFormSpacingFwd		= 75.0f;
				mFormSpacingRight	= 50.0f;
			}
			else
			{
				mFormSpacingFwd		= 75.0f;
				mFormSpacingRight	= 20.0f;
			}
		}
	}

public:
	////////////////////////////////////////////////////////////////////////////////////
	// AddActor - Adds a new actor to the troop & automatically promote to leader
	////////////////////////////////////////////////////////////////////////////////////
	void	AddActor(gentity_t* actor)
	{
		assert(actor->NPC->troop==0 && !mActors.full());
		actor->NPC->troop = mTroopHandle;
		mActors.push_back(actor);
		mTroopReform = true;
		if ((mActors.size()==1) || (actor->NPC->rank > mActors[0]->NPC->rank))
		{
			MakeActorLeader(mActors.size()-1);
		}
		if (!mTroopTeam)
		{
			mTroopTeam = actor->client->playerTeam;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////
	// RemoveActor - Removes an actor from the troop & automatically promote leader
	////////////////////////////////////////////////////////////////////////////////////
	void	RemoveActor(gentity_t* actor)
	{
		assert(actor->NPC->troop==mTroopHandle);
		int		bestNewLeader=-1;
		int		numEnts = mActors.size();
		//bool	found = false;
		mTroopReform = true;

		// Find The Actor
		//----------------
		for (int i=0; i<numEnts; i++)
		{
			if (mActors[i]==actor)
			{
				//found = true;
				mActors.erase_swap(i);
				numEnts --;
				if (i==0 && !mActors.empty())
				{
					bestNewLeader = 0;
				}
			}

			if (bestNewLeader>=0 && (mActors[i]->NPC->rank > mActors[bestNewLeader]->NPC->rank))
			{
				bestNewLeader = i;
			}
		}
		if (!mActors.empty() && bestNewLeader>=0)
		{
			MakeActorLeader(bestNewLeader);
		}

		//assert(found);
		actor->NPC->troop = 0;
	}

private:
	////////////////////////////////////////////////////////////////////////////////////
	// Enemy
	//
	// The troop has a collective enemy that it knows about, which is updated by all
	// the members of the group;
	////////////////////////////////////////////////////////////////////////////////////
	gentity_t*		mTarget;
	bool			mTargetVisable;
	int				mTargetVisableStartTime;
	int				mTargetVisableStopTime;
	CVec3			mTargetVisablePosition;
	int				mTargetIndex;
	int				mTargetLastKnownTime;
	CVec3			mTargetLastKnownPosition;
	bool			mTargetLastKnownPositionVisited;

	////////////////////////////////////////////////////////////////////////////////////
	// RegisterTarget - Records That the target is seen, when and where
	////////////////////////////////////////////////////////////////////////////////////
	void			RegisterTarget(gentity_t* target, int index, bool visable)
	{
		if (!mTarget)
		{
			HT_Speech(mActors[0], SPEECH_DETECTED, 0);
		}
		else if ((level.time - mTargetLastKnownTime)>8000)
		{
			HT_Speech(mActors[0], SPEECH_SIGHT, 0);
		}

		if (visable)
		{
			mTargetVisableStopTime = level.time;
			if (!mTargetVisable)
			{
				mTargetVisableStartTime = level.time;
			}

 			CalcEntitySpot(target, SPOT_HEAD, mTargetVisablePosition.v);
			mTargetVisablePosition[2] -= 10.0f;
		}

		mTarget							= target;
		mTargetVisable					= visable;
		mTargetIndex					= index;
		mTargetLastKnownTime			= level.time;
		mTargetLastKnownPosition		= target->currentOrigin;
		mTargetLastKnownPositionVisited = false;
	}

	////////////////////////////////////////////////////////////////////////////////////
	// RegisterTarget - Records That the target is seen, when and where
	////////////////////////////////////////////////////////////////////////////////////
	bool			TargetLastKnownPositionVisited()
	{
		if (!mTargetLastKnownPositionVisited)
		{
			float dist = DistanceSquared(mTargetLastKnownPosition.v, mActors[0]->currentOrigin);
			mTargetLastKnownPositionVisited = (dist<TARGET_POS_VISITED);
		}
		return mTargetLastKnownPositionVisited;
	}

	float			ClampScale(float val)
	{
		if (val>1.0f)
		{
			val = 1.0f;
		}
		if (val<0.0f)
		{
			val = 0.0f;
		}
		return val;
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Target Visibility
	//
	// Compute all factors that can add visibility to a target
	////////////////////////////////////////////////////////////////////////////////////
	float			TargetVisibility(gentity_t*	target)
	{
		float	Scale = 0.8f;
		if (target->client && target->client->ps.weapon==WP_SABER && target->client->ps.SaberActive())
		{
			Scale += 0.1f;
		}
		return ClampScale(Scale);
	}

	////////////////////////////////////////////////////////////////////////////////////
	//
	////////////////////////////////////////////////////////////////////////////////////
	float			TargetNoiseLevel(gentity_t*	target)
	{
		float	Scale = 0.1f;
		Scale	+= target->resultspeed / (float)g_speed->integer;
		if (target->client && target->client->ps.weapon==WP_SABER && target->client->ps.SaberActive())
		{
			Scale += 0.2f;
		}
		return ClampScale(Scale);
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Scan For Enemies
	////////////////////////////////////////////////////////////////////////////////////
	void			ScanForTarget(int scannerIndex)
	{
		gentity_t*		target;
		int				targetIndex=0;
		int				targetStop=ENTITYNUM_WORLD;
		CVec3			targetPos;
		CVec3			targetDirection;
		float			targetDistance;
		float			targetVisibility;
		float			targetNoiseLevel;

		gentity_t*		scanner					= mActors[scannerIndex];
		gNPCstats_t*	scannerStats			= &(scanner->NPC->stats);
		float			scannerMaxViewDist		= scannerStats->visrange;
		float			scannerMinVisability	= 0.1f;//1.0f - scannerStats->vigilance;
		float			scannerMaxHearDist		= scannerStats->earshot;
		float			scannerMinNoiseLevel	= 0.3f;//1.0f - scannerStats->vigilance;
		CVec3			scannerPos(scanner->currentOrigin);
		CVec3			scannerFwd(scanner->currentAngles);
		scannerFwd.AngToVec();

		// If Existing Target, Only Check It
		//-----------------------------------
		if (mTarget)
		{
			targetIndex		= mTargetIndex;
			targetStop		= mTargetIndex+1;
		}

		SaveNPCGlobals();
		SetNPCGlobals(scanner);


		for (; targetIndex<targetStop; targetIndex++)
		{
			target				= &g_entities[targetIndex];
			if (!NPC_ValidEnemy(target))
			{
				continue;
			}

			targetPos			= target->currentOrigin;
			if (target->client && target->client->ps.leanofs)
			{
				targetPos		= target->client->renderInfo.eyePoint;
			}

			targetDirection		= (targetPos - scannerPos);
			targetDistance		= targetDirection.SafeNorm();

			// Can The Scanner SEE The Target?
			//---------------------------------
			if (targetDistance<scannerMaxViewDist)
			{
				targetVisibility	= TargetVisibility(target);
				targetVisibility	*= targetDirection.Dot(scannerFwd);
				if (targetVisibility>scannerMinVisability)
				{
					if (NPC_ClearLOS(targetPos.v))
					{
						RegisterTarget(target, targetIndex, true);
						RestoreNPCGlobals();
						return;
					}
				}
			}

			// Can The Scanner HEAR The Target?
			//----------------------------------
			if (targetDistance<scannerMaxHearDist)
			{
				targetNoiseLevel = TargetNoiseLevel(target);
				targetNoiseLevel *= (1.0f - (targetDistance/scannerMaxHearDist));	// scale by distance
				if (targetNoiseLevel>scannerMinNoiseLevel)
				{
					RegisterTarget(target, targetIndex, false);
					RestoreNPCGlobals();
					return;
				}
			}
		}
		RestoreNPCGlobals();
	}








private:
	////////////////////////////////////////////////////////////////////////////////////
	// Troop State
	//
	// The troop as a whole can be acting under a number of different "behavior states"
	////////////////////////////////////////////////////////////////////////////////////
	enum ETroopState
	{
		TS_NONE = 0,				// No troop wide activity active

		TS_ADVANCE,		// CHOOSE A NEW ADVANCE TACTIC
		TS_ADVANCE_REGROUP,			// All ents move into squad position
		TS_ADVANCE_SEARCH,			// Slow advance, looking left to right, in formation
		TS_ADVANCE_COVER,			// One at a time moves forward, goes off path, provides cover
		TS_ADVANCE_FORMATION,		// In formation jog to goal location

		TS_ATTACK,		// CHOOSE A NEW ATTACK TACTIC
		TS_ATTACK_LINE,				// Form 2 lines, front kneel, back stand
		TS_ATTACK_FLANK,			// Same As Line, except scouting group attemts to get around other side of target
		TS_ATTACK_SURROUND,			// Get on all sides of target
		TS_ATTACK_COVER,			//

		TS_MAX
	};
	ETroopState		mState;

	CVec3			mFormHead;
	CVec3			mFormFwd;
	CVec3			mFormRight;


	////////////////////////////////////////////////////////////////////////////////////
	// TroopInFormation - A quick check to see if the troop is currently in formation
	////////////////////////////////////////////////////////////////////////////////////
	bool	TroopInFormation()
	{
		float	maxActorRangeSq = ((mActors.size()/2) + 2) * mFormSpacingFwd;
		maxActorRangeSq *= maxActorRangeSq;
		for (int actorIndex=1; actorIndex<mActors.size(); actorIndex++)
		{
			if (DistanceSq(mActors[actorIndex])>maxActorRangeSq)
			{
				return false;
			}
		}
		return true;
	}

	////////////////////////////////////////////////////////////////////////////////////
	// SActorOrder
	////////////////////////////////////////////////////////////////////////////////////
	struct	SActorOrder
	{
		CVec3	mPosition;
		int		mCombatPoint;
		bool	mKneelAndShoot;
	};
	ratl::array_vs<SActorOrder, MAX_ENTS_PER_TROOP>		mOrders;


	////////////////////////////////////////////////////////////////////////////////////
	// LeaderIssueAndUpdateOrders - Tell Everyone Where To Go
	////////////////////////////////////////////////////////////////////////////////////
	void	LeaderIssueAndUpdateOrders(ETroopState NextState)
	{
		int		actorIndex;
		int		actorCount = mActors.size();

		// Always Put Guys Closest To The Order Locations In Those Locations
		//-------------------------------------------------------------------
 		for (int orderIndex=1; orderIndex<actorCount; orderIndex++)
		{
			// Don't re-assign points combat point related orders
			//----------------------------------------------------
			if (mOrders[orderIndex].mCombatPoint==-1)
			{
				int		closestActorIndex		= orderIndex;
				float	closestActorDistance	= DistanceSquared(mOrders[orderIndex].mPosition.v, mActors[orderIndex]->currentOrigin);
				float	currentDistance			= closestActorDistance;
				for (actorIndex=orderIndex+1; actorIndex<actorCount; actorIndex++)
				{
					currentDistance = DistanceSquared(mOrders[orderIndex].mPosition.v, mActors[actorIndex]->currentOrigin);
					if (currentDistance<closestActorDistance)
					{
						closestActorDistance = currentDistance;
						closestActorIndex = actorIndex;
					}
				}
				if (orderIndex!=closestActorIndex)
				{
					mActors.swap(orderIndex, closestActorIndex);
				}
			}
		}

		// Now Copy The Orders Out To The Actors
		//---------------------------------------
		for (actorIndex=1; actorIndex<actorCount; actorIndex++)
		{
			VectorCopy(mOrders[actorIndex].mPosition.v, mActors[actorIndex]->pos1);
		}

// PHASE I - VOICE COMMANDS & ANIMATIONS
//=======================================
		gentity_t*	leader	= mActors[0];

		if (NextState!=mState)
		{
			if (mActors.size()>0)
			{
				switch (NextState)
				{
				case (TS_ADVANCE_REGROUP) :
					{
						break;
					}
				case (TS_ADVANCE_SEARCH) :
					{
						HT_Speech(leader, SPEECH_LOOK, 0);
						break;
					}
				case (TS_ADVANCE_COVER) :
					{
						HT_Speech(leader, SPEECH_COVER, 0);
						NPC_SetAnim(leader, SETANIM_TORSO, TORSO_HANDSIGNAL4, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLDLESS);
						break;
					}
				case (TS_ADVANCE_FORMATION) :
					{
						HT_Speech(leader, SPEECH_ESCAPING, 0);
						break;
					}


				case (TS_ATTACK_LINE) :
					{
						HT_Speech(leader, SPEECH_CHASE, 0);
						NPC_SetAnim(leader, SETANIM_TORSO, TORSO_HANDSIGNAL1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLDLESS);
						break;
					}
				case (TS_ATTACK_FLANK) :
					{
						HT_Speech(leader, SPEECH_OUTFLANK, 0);
						NPC_SetAnim(leader, SETANIM_TORSO, TORSO_HANDSIGNAL3, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLDLESS);
						break;
					}
				case (TS_ATTACK_SURROUND) :
					{
						HT_Speech(leader, SPEECH_GIVEUP, 0);
						NPC_SetAnim(leader, SETANIM_TORSO, TORSO_HANDSIGNAL2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLDLESS);
						break;
					}
				case (TS_ATTACK_COVER) :
					{
						HT_Speech(leader, SPEECH_COVER, 0);
						break;
					}
				default:
					{
					}
				}
			}
		}

		// If Attacking, And Not Forced To Reform, Don't Recalculate Orders
		//------------------------------------------------------------------
		else if (NextState>TS_ATTACK && !mTroopReform)
		{
			return;
		}


// PHASE II - COMPUTE THE NEW FORMATION HEAD, FORWARD, AND RIGHT VECTORS
//=======================================================================
		mFormHead	= leader->currentOrigin;
		mFormFwd	= (NAV::HasPath(leader))?(NAV::NextPosition(leader)):(mTargetLastKnownPosition);
		mFormFwd	-= mFormHead;
		mFormFwd[2] = 0;
		mFormFwd	*= -1.0f;				// Form Forward Goes Behind The Leader
		mFormFwd.Norm();

		mFormRight	= mFormFwd;
		mFormRight.Cross(CVec3::mZ);


		// Scale Vectors By Spacing Distances
		//------------------------------------
		mFormFwd	*= mFormSpacingFwd;
		mFormRight	*= mFormSpacingRight;

		// If Attacking, Move Head Forward Some To Center On Target
		//----------------------------------------------------------
 		if (NextState>TS_ATTACK)
		{
 			if (!mTroopReform)
			{
				int	FwdNum = ((actorCount/2)+1);
				for (int i=0; i<FwdNum; i++)
				{
					mFormHead -= mFormFwd;
				}
			}
			trace_t	trace;

			mOrders[0].mPosition = mFormHead;

			gi.trace(&trace,
					mActors[0]->currentOrigin,
					mActors[0]->mins,
					mActors[0]->maxs,
					mOrders[0].mPosition.v,
					mActors[0]->s.number,
					mActors[0]->clipmask,
					(EG2_Collision)0,
					0
					);

			if (trace.fraction<1.0f)
			{
				mOrders[0].mPosition = trace.endpos;
			}
		}
		else
		{
			mOrders[0].mPosition = mTargetLastKnownPosition;
		}

		VectorCopy(mOrders[0].mPosition.v, mActors[0]->pos1);

		CVec3		FormTgtToHead(mFormHead);
					FormTgtToHead -= mTargetLastKnownPosition;
		/*float		FormTgtToHeadDist = */FormTgtToHead.SafeNorm();

		CVec3		BaseAngleToHead(FormTgtToHead);
					BaseAngleToHead.VecToAng();

//		int			NumPerSide = mActors.size()/2;
//		float		WidestAngle = FORMATION_SURROUND_FAN * (NumPerSide+1);



// PHASE III - USE FORMATION VECTORS TO COMPUTE ORDERS FOR ALL ACTORS
//====================================================================
		for (actorIndex=1; actorIndex<actorCount; actorIndex++)
		{
			SaveNPCGlobals();
			SetNPCGlobals(mActors[actorIndex]);

			SActorOrder&	Order = mOrders[actorIndex];
			float			FwdScale = (float)((int)((actorIndex+1)/2));
			float			SideScale = ((actorIndex%2)==0)?(-1.0f):(1.0f);

			if (mActors[actorIndex]->NPC->combatPoint!=-1)
			{
				NPC_FreeCombatPoint(mActors[actorIndex]->NPC->combatPoint, qfalse);
				mActors[actorIndex]->NPC->combatPoint = -1;
			}


			Order.mPosition		= mFormHead;
			Order.mCombatPoint	= -1;
			Order.mKneelAndShoot= false;


			// Advance Orders
			//----------------
			if (NextState<TS_ATTACK)
			{
				if ((NextState==TS_ADVANCE_REGROUP) ||	(NextState==TS_ADVANCE_SEARCH) || (NextState==TS_ADVANCE_FORMATION))
				{
					Order.mPosition.ScaleAdd(mFormFwd,		FwdScale);
					Order.mPosition.ScaleAdd(mFormRight,	SideScale);
				}
				else if (NextState==TS_ADVANCE_COVER)
				{
					// TODO: Take Turns Switching Who Is In Front
					Order.mPosition.ScaleAdd(mFormFwd,		FwdScale);
					Order.mPosition.ScaleAdd(mFormRight,	SideScale);
				}
			}

			// Setup Initial Attack Orders
			//-----------------------------
			else
			{
				if (NextState==TS_ATTACK_LINE || (NextState==TS_ATTACK_FLANK && actorIndex<4))
				{
					Order.mPosition.ScaleAdd(mFormFwd,		FwdScale);
					Order.mPosition.ScaleAdd(mFormRight,		SideScale);
				}
				else if (NextState==TS_ATTACK_FLANK && actorIndex>=4)
				{
					int		cpFlags = (CP_HAS_ROUTE|CP_AVOID_ENEMY|CP_CLEAR|CP_COVER|CP_FLANK|CP_APPROACH_ENEMY);
					float	avoidDist = 128.0f;

					Order.mCombatPoint = NPC_FindCombatPointRetry(
											mActors[actorIndex]->currentOrigin,
											mActors[actorIndex]->currentOrigin,
											mActors[actorIndex]->currentOrigin,
											&cpFlags,
											avoidDist,
											0);

					if (Order.mCombatPoint!=-1 && (cpFlags&CP_CLEAR))
					{
						Order.mPosition = level.combatPoints[Order.mCombatPoint].origin;
						NPC_SetCombatPoint(Order.mCombatPoint);
					}
					else
					{
						Order.mPosition.ScaleAdd(mFormFwd,		FwdScale);
						Order.mPosition.ScaleAdd(mFormRight,		SideScale);
					}
				}
				else if (NextState==TS_ATTACK_SURROUND)
				{
					Order.mPosition.ScaleAdd(mFormFwd,		FwdScale);
					Order.mPosition.ScaleAdd(mFormRight,		SideScale);

/*					CVec3	FanAngles = BaseAngleToHead;
					FanAngles[YAW] += (SideScale * (WidestAngle-(FwdScale*FORMATION_SURROUND_FAN)));
					FanAngles.AngToVec();

					Order.mPosition = mTargetLastKnownPosition;
					Order.mPosition.ScaleAdd(FanAngles,		FormTgtToHeadDist);
*/
				}
				else if (NextState==TS_ATTACK_COVER)
				{
					Order.mPosition.ScaleAdd(mFormFwd,		FwdScale);
					Order.mPosition.ScaleAdd(mFormRight,		SideScale);
				}
			}

			if (NextState>=TS_ATTACK)
			{
				trace_t	trace;
				CVec3	OrderUp(Order.mPosition);
				OrderUp[2] += 10.0f;

				gi.trace(&trace,
					Order.mPosition.v,
					mActors[actorIndex]->mins,
					mActors[actorIndex]->maxs,
					OrderUp.v,
					mActors[actorIndex]->s.number,
					CONTENTS_SOLID|CONTENTS_TERRAIN|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP,
					(EG2_Collision)0,
					0);

				if (trace.startsolid || trace.allsolid)
				{
					int		cpFlags = (CP_HAS_ROUTE|CP_AVOID_ENEMY|CP_CLEAR|CP_COVER|CP_FLANK|CP_APPROACH_ENEMY);
					float	avoidDist = 128.0f;

					Order.mCombatPoint = NPC_FindCombatPointRetry(
											mActors[actorIndex]->currentOrigin,
											mActors[actorIndex]->currentOrigin,
											mActors[actorIndex]->currentOrigin,
											&cpFlags,
											avoidDist,
											0);

					if (Order.mCombatPoint!=-1)
					{
						Order.mPosition = level.combatPoints[Order.mCombatPoint].origin;
						NPC_SetCombatPoint(Order.mCombatPoint);
					}
					else
					{
						Order.mPosition = mOrders[0].mPosition;
					}
				}
			}
			RestoreNPCGlobals();
		}

		mTroopReform = false;
		mState = NextState;
 	}

	////////////////////////////////////////////////////////////////////////////////////
	// SufficientCoverNearby - Look at nearby combat points, see if there is enough
	////////////////////////////////////////////////////////////////////////////////////
	bool	SufficientCoverNearby()
	{
		// TODO: Evaluate Available Combat Points
		return false;
	}








public:
	////////////////////////////////////////////////////////////////////////////////////
	// Update - This is the primary "think" function from the troop
	////////////////////////////////////////////////////////////////////////////////////
	void	Update()
	{
		if (mActors.empty())
		{
			return;
		}
		ScanForTarget(0 /*Q_irand(0, (mActors.size()-1))*/);
		if (mTarget)
		{
			ETroopState NextState			= mState;
			int			TimeSinceLastSeen	= (level.time - mTargetVisableStopTime);
		//	int			TimeVisable			= (mTargetVisableStopTime - mTargetVisableStartTime);
			bool		Attack				= (TimeSinceLastSeen<2000);

			if (Attack)
			{
				// If Not Currently Attacking, Or We Want To Pick A New Attack Tactic
				//--------------------------------------------------------------------
				if (mState<TS_ATTACK /*|| TODO: Timer To Pick New Tactic */)
				{
					if (TroopInFormation())
					{
						NextState = (mActors.size()>4)?(TS_ATTACK_FLANK):(TS_ATTACK_LINE);
					}
					else
					{
						NextState = (SufficientCoverNearby())?(TS_ATTACK_COVER):(TS_ATTACK_SURROUND);
					}
				}
			}
			else
			{
				if (!TroopInFormation())
				{
					NextState = TS_ADVANCE_REGROUP;
				}
				else
				{
					if (TargetLastKnownPositionVisited())
					{
						NextState = TS_ADVANCE_SEARCH;
					}
					else
					{
						NextState = (TimeSinceLastSeen<10000)?(TS_ADVANCE_COVER):(TS_ADVANCE_FORMATION);
					}
				}
			}
			LeaderIssueAndUpdateOrders(NextState);

		}
	}

	////////////////////////////////////////////////////////////////////////////////////
	// MergeInto - Merges all actors into anther troop
	////////////////////////////////////////////////////////////////////////////////////
	void	MergeInto(CTroop& Other)
	{
		int	numEnts = mActors.size();
		for (int i=0; i<numEnts; i++)
		{
			mActors[i]->client->leader = 0;
			mActors[i]->NPC->troop = 0;
			Other.AddActor(mActors[i]);
		}
		mActors.clear();

		if (!Other.mTarget && mTarget)
		{
			Other.mTarget							= mTarget;
			Other.mTargetIndex						= mTargetIndex;
			Other.mTargetLastKnownPosition			= mTargetLastKnownPosition;
			Other.mTargetLastKnownPositionVisited	= mTargetLastKnownPositionVisited;
			Other.mTargetLastKnownTime				= mTargetLastKnownTime;
			Other.mTargetVisableStartTime			= mTargetVisableStartTime;
			Other.mTargetVisableStopTime			= mTargetVisableStopTime;
			Other.mTargetVisable					= mTargetVisable;
			Other.mTargetVisablePosition			= mTargetVisablePosition;
			Other.LeaderIssueAndUpdateOrders(mState);
		}
	}

	////////////////////////////////////////////////////////////////////////////////////
	//
	////////////////////////////////////////////////////////////////////////////////////
	gentity_t*	TrackingTarget()
	{
		return mTarget;
	}

	////////////////////////////////////////////////////////////////////////////////////
	//
	////////////////////////////////////////////////////////////////////////////////////
	gentity_t*	TroopLeader()
	{
		return  mActors[0];
	}

	////////////////////////////////////////////////////////////////////////////////////
	//
	////////////////////////////////////////////////////////////////////////////////////
	int			TimeSinceSeenTarget()
	{
		return (level.time - mTargetVisableStopTime);
	}

	////////////////////////////////////////////////////////////////////////////////////
	//
	////////////////////////////////////////////////////////////////////////////////////
	CVec3&		TargetVisablePosition()
	{
		return mTargetVisablePosition;
	}


	////////////////////////////////////////////////////////////////////////////////////
	//
	////////////////////////////////////////////////////////////////////////////////////
	float		FormSpacingFwd()
	{
		return mFormSpacingFwd;
	}

	////////////////////////////////////////////////////////////////////////////////////
	//
	////////////////////////////////////////////////////////////////////////////////////
	gentity_t*	TooCloseToTroopMember(gentity_t* actor)
	{
		for (int i=0; i<mActors.size(); i++)
		{
			// Only avoid guys ahead of us in the formation
			//----------------------------------------------
			if (actor==mActors[i])
			{
				return 0;
			}
		//	if (mActors[i]->resultspeed<10.0f)
		//	{
		//		continue;
		//	}

			if (i==0)
			{
				if (Distance(actor->currentOrigin, mActors[i]->currentOrigin)<(mFormSpacingFwd*0.5f))
				{
					return mActors[i];
				}
			}
			else
			{
				if (Distance(actor->currentOrigin, mActors[i]->currentOrigin)<(mFormSpacingFwd*0.5f))
				{
					return mActors[i];
				}
			}
		}
		assert("Somehow this actor is not actually in the troop..."==0);
		return 0;
	}
};
typedef		ratl::handle_pool_vs<CTroop, MAX_TROOPS>		TTroopPool;
TTroopPool	mTroops;






////////////////////////////////////////////////////////////////////////////////////////
// Erase All Data, Set To Default Vals Before Entities Spawn
////////////////////////////////////////////////////////////////////////////////////////
void		Troop_Reset()
{
	mTroops.clear();
}

////////////////////////////////////////////////////////////////////////////////////////
// Entities Have Just Spawned, Initialize
////////////////////////////////////////////////////////////////////////////////////////
void		Troop_Initialize()
{
}

////////////////////////////////////////////////////////////////////////////////////////
// Global Update Of All Troops
////////////////////////////////////////////////////////////////////////////////////////
void		Troop_Update()
{
	for (TTroopPool::iterator i=mTroops.begin(); i!=mTroops.end(); ++i)
	{
		i->Update();
	}
}


////////////////////////////////////////////////////////////////////////////////////////
// Erase All Data, Set To Default Vals Before Entities Spawn
////////////////////////////////////////////////////////////////////////////////////////
void		Trooper_UpdateTroop(gentity_t* actor)
{
	// Try To Join A Troop
	//---------------------
	if (!actor->NPC->troop)
	{
		float					curDist = 0;
		float					closestDist = 0;
		TTroopPool::iterator	closestTroop = mTroops.end();
		trace_t					trace;

		for (TTroopPool::iterator iTroop=mTroops.begin(); iTroop!=mTroops.end(); ++iTroop)
		{
			if (iTroop->Team()==actor->client->playerTeam)
			{
				curDist = iTroop->DistanceSq(actor);
				if (curDist<MAX_TROOP_JOIN_DIST2 && (!closestDist || curDist<closestDist))
				{
					// Only Join A Troop If You Can See The Leader
					//---------------------------------------------
					gi.trace(&trace,
						actor->currentOrigin,
						actor->mins,
						actor->maxs,
						iTroop->TroopLeader()->currentOrigin,
						actor->s.number,
						CONTENTS_SOLID|CONTENTS_TERRAIN|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP,
						(EG2_Collision)0,
						0);

					if (!trace.allsolid &&
						!trace.startsolid &&
						(trace.fraction>=1.0f || trace.entityNum==iTroop->TroopLeader()->s.number))
					{
						closestDist = curDist;
						closestTroop = iTroop;
					}
				}
			}
		}

		// If Found, Add The Actor To It
		//--------------------------------
		if (closestTroop!=mTroops.end())
		{
			closestTroop->AddActor(actor);
		}

		// If We Couldn't Find One, Create A New Troop
		//---------------------------------------------
		else if (!mTroops.full())
		{
			int	nTroopHandle = mTroops.alloc();
			mTroops[nTroopHandle].Initialize(nTroopHandle);
			mTroops[nTroopHandle].AddActor(actor);
		}
	}

	// If This Is A Leader, Then He Is Responsible For Merging Troops
	//----------------------------------------------------------------
	else if (actor->client->leader==actor)
	{
		float					curDist = 0;
		float					closestDist = 0;
		TTroopPool::iterator	closestTroop = mTroops.end();

		for (TTroopPool::iterator iTroop=mTroops.begin(); iTroop!=mTroops.end(); ++iTroop)
		{
			curDist = iTroop->DistanceSq(actor);
			if ((curDist<MAX_TROOP_MERGE_DIST2) &&
				(!closestDist || curDist<closestDist) &&
				(mTroops.index_to_handle(iTroop.index())!=actor->NPC->troop))
			{
				closestDist = curDist;
				closestTroop = iTroop;
			}
		}

		if (closestTroop!=mTroops.end())
		{
			int		oldTroopNum = actor->NPC->troop;
			mTroops[oldTroopNum].MergeInto(*closestTroop);
			mTroops.free(oldTroopNum);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
bool		Trooper_UpdateSmackAway(gentity_t* actor, gentity_t* target)
{
 	if (actor->client->ps.legsAnim==BOTH_MELEE1)
	{
		if (TIMER_Done(actor, "Trooper_SmackAway"))
		{
			CVec3	ActorPos(actor->currentOrigin);
			CVec3	ActorToTgt(target->currentOrigin);
					ActorToTgt -= ActorPos;
			float	ActorToTgtDist = ActorToTgt.SafeNorm();

			if (ActorToTgtDist<100.0f)
			{
				G_Throw(target, ActorToTgt.v, 200.0f);
			}
		}
		return true;
	}
	return false;
}


////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
void		Trooper_SmackAway(gentity_t* actor, gentity_t* target)
{
	assert(actor && actor->NPC);
 	if (actor->client->ps.legsAnim!=BOTH_MELEE1)
	{
		NPC_SetAnim(actor, SETANIM_BOTH, BOTH_MELEE1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
		TIMER_Set(actor, "Trooper_SmackAway",			actor->client->ps.torsoAnimTimer/4.0f);
	}
}


////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
bool		Trooper_Kneeling(gentity_t* actor)
{
	return (actor->NPC->aiFlags&NPCAI_KNEEL || actor->client->ps.legsAnim==BOTH_STAND_TO_KNEEL);
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
void		Trooper_KneelDown(gentity_t* actor)
{
	assert(actor && actor->NPC);
	if (!Trooper_Kneeling(actor) && level.time>actor->NPC->kneelTime)
	{
		NPC_SetAnim(actor, SETANIM_BOTH, BOTH_STAND_TO_KNEEL, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
		actor->NPC->aiFlags |=  NPCAI_KNEEL;
		actor->NPC->kneelTime = level.time + Q_irand(3000, 6000);
	}
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
void		Trooper_StandUp(gentity_t* actor, bool always=false)
{
	assert(actor && actor->NPC);
	if (Trooper_Kneeling(actor) && (always || level.time>actor->NPC->kneelTime))
	{
		actor->NPC->aiFlags &= ~NPCAI_KNEEL;
		NPC_SetAnim(actor, SETANIM_BOTH, BOTH_KNEEL_TO_STAND, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
		actor->NPC->kneelTime = level.time + Q_irand(3000, 6000);
	}
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
int			Trooper_CanHitTarget(gentity_t* actor, gentity_t* target, CTroop& troop, float& MuzzleToTargetDistance, CVec3& MuzzleToTarget)
{
	trace_t	tr;
	CVec3	MuzzlePoint(actor->currentOrigin);
	 		CalcEntitySpot(actor, SPOT_WEAPON, MuzzlePoint.v);

			MuzzleToTarget = troop.TargetVisablePosition();
			MuzzleToTarget			-= MuzzlePoint;
			MuzzleToTargetDistance	= MuzzleToTarget.SafeNorm();


	CVec3	MuzzleDirection(actor->currentAngles);
			MuzzleDirection.AngToVec();

	// Aiming In The Right Direction?
	//--------------------------------
	if (MuzzleDirection.Dot(MuzzleToTarget)>0.95)
	{
		// Clear Line Of Sight To Target?
		//--------------------------------
		gi.trace(&tr, MuzzlePoint.v, NULL, NULL, troop.TargetVisablePosition().v, actor->s.number, MASK_SHOT, (EG2_Collision)0, 0);
		if (tr.startsolid || tr.allsolid)
		{
			return ENTITYNUM_NONE;
		}
		if (tr.entityNum==target->s.number || tr.fraction>0.9f)
		{
			return target->s.number;
		}
		return tr.entityNum;
	}
	return ENTITYNUM_NONE;
}


////////////////////////////////////////////////////////////////////////////////////////
// Run The Per Trooper Update
////////////////////////////////////////////////////////////////////////////////////////
void		Trooper_Think(gentity_t* actor)
{
	gentity_t* target = (actor->NPC->troop)?(mTroops[actor->NPC->troop].TrackingTarget()):(0);
	if (target)
	{
		G_SetEnemy(actor, target);

		CTroop&		troop		= mTroops[actor->NPC->troop];
		bool		AtPos		= STEER::Reached(actor, actor->pos1, 10.0f);
		int			traceTgt	= ENTITYNUM_NONE;
		bool		traced		= false;
		bool		inSmackAway	= false;

		float		MuzzleToTargetDistance = 0.0f;
		CVec3		MuzzleToTarget;

		if (actor->NPC->combatPoint!=-1)
		{
			traceTgt	= Trooper_CanHitTarget(actor, target, troop, MuzzleToTargetDistance, MuzzleToTarget);
			traced		= true;
			if (traceTgt==target->s.number)
			{
				AtPos = true;
			}
		}


		// Smack!
		//-------
		if (Trooper_UpdateSmackAway(actor, target))
		{
			traced		= true;
			AtPos		= true;
			inSmackAway = true;
		}


		if (false)
		{
			CG_DrawEdge(actor->currentOrigin, actor->pos1, EDGE_IMPACT_SAFE);
		}

		// If There, Stop Moving
		//-----------------------
		STEER::Activate(actor);
		{
	 		gentity_t*	fleeFrom = troop.TooCloseToTroopMember(actor);

			// If Too Close To The Leader, Get Out Of His Way
			//------------------------------------------------
			if (fleeFrom)
			{
				STEER::Flee(actor, fleeFrom->currentOrigin, 1.0f);
				AtPos = false;
			}


			// If In Position, Stop Moving
			//-----------------------------
			if (AtPos)
			{
 				NAV::ClearPath(actor);
				STEER::Stop(actor);
			}

			// Otherwise, Try To Get To Position
			//-----------------------------------
			else
			{
				Trooper_StandUp(actor, true);

				// If Close Enough, Persue Our Target Directly
				//---------------------------------------------
				bool moveSuccess = STEER::GoTo(NPC,  actor->pos1, 10.0f, false);

				// Otherwise
				//-----------
				if (!moveSuccess)
				{
					moveSuccess = NAV::GoTo(NPC, actor->pos1);
				}

				// If No Way To Get To Position, Stay Here
				//-----------------------------------------
				if (!moveSuccess || (level.time - actor->lastMoveTime)>4000)
				{
					AtPos = true;
				}
			}
		}
		STEER::DeActivate(actor, &ucmd);




		// If There And Target Was Recently Visable
		//------------------------------------------
		if (AtPos && (troop.TimeSinceSeenTarget()<1500))
		{
			if (!traced)
			{
				traceTgt = Trooper_CanHitTarget(actor, target, troop, MuzzleToTargetDistance, MuzzleToTarget);
			}

			// Shoot!
			//--------
			if (traceTgt==target->s.number)
			{
				WeaponThink(qtrue);
			}
			else if (!inSmackAway)
			{
				// Otherwise, If Kneeling, Get Up!
				//---------------------------------
				if (Trooper_Kneeling(actor))
				{
					Trooper_StandUp(actor);
				}

				// If The Enemy Is Close Enough, Smack Him Away
				//----------------------------------------------
				else if (MuzzleToTargetDistance<40.0f)
				{
					Trooper_SmackAway(actor, target);
				}

				// If We Would Have It A Friend, Ask Him To Kneel
				//------------------------------------------------
				else if (traceTgt!=ENTITYNUM_NONE &&
							traceTgt!=ENTITYNUM_WORLD &&
							g_entities[traceTgt].client &&
							g_entities[traceTgt].NPC &&
							g_entities[traceTgt].client->playerTeam==actor->client->playerTeam &&
							NPC_IsTrooper(&g_entities[traceTgt]) &&
							g_entities[traceTgt].resultspeed<1.0f &&
							!(g_entities[traceTgt].NPC->aiFlags & NPCAI_KNEEL))
				{
					Trooper_KneelDown(&g_entities[traceTgt]);
				}
			}


			// Convert To Angles And Set That As Our Desired Look Direction
			//--------------------------------------------------------------
 			if (MuzzleToTargetDistance>100)
			{
 				MuzzleToTarget.VecToAng();

				NPCInfo->desiredYaw		= MuzzleToTarget[YAW];
				NPCInfo->desiredPitch	= MuzzleToTarget[PITCH];
			}
			else
			{
				MuzzleToTarget  = troop.TargetVisablePosition();
				MuzzleToTarget.v[2] -= 20.0f;					// Aim Lower
				MuzzleToTarget	-= actor->currentOrigin;
				MuzzleToTarget.SafeNorm();
 				MuzzleToTarget.VecToAng();

				NPCInfo->desiredYaw		= MuzzleToTarget[YAW];
				NPCInfo->desiredPitch	= MuzzleToTarget[PITCH];
			}
		}

		NPC_UpdateFiringAngles( qtrue, qtrue );
		NPC_UpdateAngles( qtrue, qtrue );

		if (Trooper_Kneeling(actor))
		{
			ucmd.upmove = -127;			// Set Crouch Height
		}
	}




	else
	{
		NPC_BSST_Default();
	}
}



////////////////////////////////////////////////////////////////////////////////////////
/*
-------------------------
NPC_BehaviorSet_Trooper
-------------------------
*/
////////////////////////////////////////////////////////////////////////////////////////
void NPC_BehaviorSet_Trooper( int bState )
{
	Trooper_UpdateTroop(NPC);
	switch( bState )
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		Trooper_Think(NPC);
		break;

	case BS_INVESTIGATE:
		NPC_BSST_Investigate();
		break;

	case BS_SLEEP:
		NPC_BSST_Sleep();
		break;

	default:
		Trooper_Think(NPC);
		break;
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// IsTrooper - return true if you want a given actor to use trooper AI
////////////////////////////////////////////////////////////////////////////////////////
bool NPC_IsTrooper(gentity_t* actor)
{
	return (
		actor &&
		actor->NPC &&
		actor->s.weapon &&
		!!(actor->NPC->scriptFlags&SCF_NO_GROUPS)// &&
//		 !(actor->NPC->scriptFlags&SCF_CHASE_ENEMIES)
		 );
}

void NPC_LeaveTroop(gentity_t* actor)
{
	assert(actor->NPC->troop);
	int wasInTroop = actor->NPC->troop;
	mTroops[actor->NPC->troop].RemoveActor(actor);
	if (mTroops[wasInTroop].Empty())
	{
		mTroops.free(wasInTroop);
	}
}


