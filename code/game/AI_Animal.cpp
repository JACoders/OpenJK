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
	    
#include "b_local.h"
#include "g_navigator.h"

#if !defined(RAVL_VEC_INC)
#include "../Ravl/CVec.h"
#endif
#include "../Ratl/vector_vs.h"

#define MAX_PACKS			10

#define	LEAVE_PACK_DISTANCE	1000
#define	JOIN_PACK_DISTANCE	800
#define	WANDER_RANGE		1000
#define	FRIGHTEN_DISTANCE	300

extern qboolean G_PlayerSpawned( void );

ratl::vector_vs<gentity_t*, MAX_PACKS>	mPacks;


////////////////////////////////////////////////////////////////////////////////////////
// Update The Packs, Delete Dead Leaders, Join / Split Packs, Find MY Leader
////////////////////////////////////////////////////////////////////////////////////////
gentity_t* NPC_AnimalUpdateLeader(void)
{
	// Find The Closest Pack Leader, Not Counting Myself
	//---------------------------------------------------
	gentity_t*	closestLeader = 0;
	float		closestDist = 0;
	int			myLeaderNum = 0;
	
	for (int i=0; i<mPacks.size(); i++)
	{
		// Dump Dead Leaders
		//-------------------
		if (mPacks[i]==0 || mPacks[i]->health<=0)
		{
			if (mPacks[i]==NPC->client->leader)
			{
				NPC->client->leader = 0;
			}

			mPacks.erase_swap(i);

			if (i>=mPacks.size())
			{
				closestLeader = 0;
				break;
			}
		}

		// Don't Count Self
		//------------------
		if (mPacks[i]==NPC)
		{
			myLeaderNum = i;
			continue;
		}

		float	Dist = Distance(mPacks[i]->currentOrigin, NPC->currentOrigin);
		if (!closestLeader || Dist<closestDist)
		{
			closestDist = Dist;
			closestLeader = mPacks[i];
		}
	}

	// In Joining Distance?
	//----------------------
	if (closestLeader && closestDist<JOIN_PACK_DISTANCE)
	{
		// Am I Already A Leader?
		//------------------------
		if (NPC->client->leader==NPC)
		{
			mPacks.erase_swap(myLeaderNum);		// Erase Myself From The Leader List
		}

		// Join The Pack!
		//----------------
		NPC->client->leader = closestLeader;
	}


	// Do I Have A Leader?
	//---------------------
	if (NPC->client->leader)
	{
		// AM I A Leader?
		//----------------
		if (NPC->client->leader!=NPC)
		{
			// If Our Leader Is Dead, Clear Him Out

			if ( NPC->client->leader->health<=0 || NPC->client->leader->inuse == 0)
			{
				NPC->client->leader = 0;
			}
			
			// If My Leader Isn't His Own Leader, Then, Use His Leader
			//---------------------------------------------------------
			else if (NPC->client->leader->client->leader!=NPC->client->leader)
			{
				// Eh.  Can this get more confusing?
				NPC->client->leader = NPC->client->leader->client->leader;
			}

			// If Our Leader Is Too Far Away, Clear Him Out
			//------------------------------------------------------
			else if ( Distance(NPC->client->leader->currentOrigin, NPC->currentOrigin)>LEAVE_PACK_DISTANCE)
			{
				NPC->client->leader = 0;
			}
		}

	}

	// If We Couldn't Find A Leader, Then Become One
	//-----------------------------------------------
	else if (!mPacks.full())
	{
		NPC->client->leader = NPC;
		mPacks.push_back(NPC);
	}
	return NPC->client->leader;
}




/*
-------------------------
NPC_BSAnimal_Default
-------------------------
*/
void NPC_BSAnimal_Default( void )
{
	if (!NPC || !NPC->client)
	{
		return;
	}

	// Update Some Positions
	//-----------------------
	CVec3	CurrentLocation(NPC->currentOrigin);


	// Update The Leader
	//-------------------
	gentity_t*	leader = NPC_AnimalUpdateLeader();


	// Select Closest Threat Location
	//--------------------------------
	CVec3	ThreatLocation(0,0,0);
	qboolean PlayerSpawned = G_PlayerSpawned();
	if ( PlayerSpawned )
	{//player is actually in the level now
		ThreatLocation = player->currentOrigin;
	}
	int	alertEvent = NPC_CheckAlertEvents(qtrue, qtrue, -1, qfalse, AEL_MINOR, qfalse);
	if ( alertEvent >= 0 )
	{
		alertEvent_t *event = &level.alertEvents[alertEvent];
		if (event->owner!=NPC  &&  Distance(event->position, CurrentLocation.v)<event->radius)
		{
			ThreatLocation = event->position;
		}
	}



//	float	DistToThreat	= CurrentLocation.Dist(ThreatLocation);
//	float	DistFromHome	= CurrentLocation.Dist(mHome);



	bool	EvadeThreat		= (level.time<NPCInfo->investigateSoundDebounceTime);
	bool	CharmedDocile	= (level.time<NPCInfo->confusionTime);
	bool	CharmedApproach = (level.time<NPCInfo->charmedTime);



	// If Not Already Evading, Test To See If We Should "Know" About The Threat
	//--------------------------------------------------------------------------
/*	if (false && !EvadeThreat && PlayerSpawned && (DistToThreat<FRIGHTEN_DISTANCE))
	{
		CVec3	LookAim(NPC->currentAngles);
		LookAim.AngToVec();
		CVec3	MyPos(CurrentLocation);
		MyPos -= ThreatLocation;
		MyPos.SafeNorm();

		float	DirectionSimilarity = MyPos.Dot(LookAim);

		if (fabsf(DirectionSimilarity)<0.8f)
		{
			EvadeThreat = true;
			NPCInfo->investigateSoundDebounceTime = level.time + Q_irand(0, 1000);
			VectorCopy(ThreatLocation.v, NPCInfo->investigateGoal);
		}
	}*/





	STEER::Activate(NPC);
	{
		// Charmed Approach - Walk TOWARD The Threat Location
		//----------------------------------------------------
		if (CharmedApproach)
		{
			NAV::GoTo(NPC, NPCInfo->investigateGoal);
		}

		// Charmed Docile - Stay Put
		//---------------------------
		else if (CharmedDocile)
		{
			NAV::ClearPath(NPC);
			STEER::Stop(NPC);
		}

		// Run Away From This Threat
		//---------------------------
		else if (EvadeThreat)
		{
			NAV::ClearPath(NPC);
			STEER::Flee(NPC, NPCInfo->investigateGoal);
		}

		// Normal Behavior
		//-----------------
		else
		{
			// Follow Our Pack Leader!
			//-------------------------
			if (leader && leader!=NPC)
			{
				float	followDist	= 100.0f;
				float	curDist		= Distance(NPC->currentOrigin, leader->followPos);


				// Update The Leader's Follow Position
				//-------------------------------------
				STEER::FollowLeader(NPC, leader, followDist);

				bool	inSeekRange = (curDist<followDist*10.0f);
				bool	onNbrPoints = (NAV::OnNeighboringPoints(NAV::GetNearestNode(NPC), leader->followPosWaypoint));
				bool	leaderStop	= ((level.time - leader->lastMoveTime)>500);

				// If Close Enough, Dump Any Existing Path
				//-----------------------------------------
				if (inSeekRange || onNbrPoints)
				{
					NAV::ClearPath(NPC);

					// If The Leader Isn't Moving, Stop
					//----------------------------------
					if (leaderStop)
					{
						STEER::Stop(NPC);
					}

					// Otherwise, Try To Get To The Follow Position
					//----------------------------------------------
					else
					{
						STEER::Seek(NPC, leader->followPos, fabsf(followDist)/2.0f/*slowing distance*/, 1.0f/*wight*/, leader->resultspeed);
					}
				}

				// Otherwise, Get A Path To The Follow Position
				//----------------------------------------------
				else
				{
					NAV::GoTo(NPC, leader->followPosWaypoint);
				}
				STEER::Separation(NPC, 4.0f);
				STEER::AvoidCollisions(NPC, leader);
			}

			// Leader AI - Basically Wander
			//------------------------------
			else
			{
				// Are We Doing A Path?
				//----------------------
				bool	HasPath = NAV::HasPath(NPC);
				if (HasPath)
				{
					HasPath = NAV::UpdatePath(NPC);
					if (HasPath)
					{
						STEER::Path(NPC);	// Follow The Path
						STEER::AvoidCollisions(NPC);
					}
				}

				if (!HasPath)
				{
					// If Debounce Time Has Expired, Choose A New Sub State
					//------------------------------------------------------
					if (NPCInfo->investigateDebounceTime<level.time)
					{
						// Clear Out Flags From The Previous Substate
						//--------------------------------------------
						NPCInfo->aiFlags	&= ~NPCAI_OFF_PATH;
						NPCInfo->aiFlags	&= ~NPCAI_WALKING;


						// Pick Another Spot
						//-------------------
						int		NEXTSUBSTATE = Q_irand(0, 10);

						bool	RandomPathNode = (NEXTSUBSTATE<8); //(NEXTSUBSTATE<9);  
						bool	PathlessWander = (NEXTSUBSTATE<9); //false;				

						

						// Random Path Node
						//------------------
						if (RandomPathNode)
						{
							// Sometimes, Walk
							//-----------------
							if (Q_irand(0, 1)==0)
							{
								NPCInfo->aiFlags	|= NPCAI_WALKING;
							}

							NPCInfo->investigateDebounceTime = level.time + Q_irand(3000, 10000);
							NAV::FindPath(NPC, NAV::ChooseRandomNeighbor(NAV::GetNearestNode(NPC)));//, mHome.v, WANDER_RANGE));
						}

						// Pathless Wandering
						//--------------------
						else if (PathlessWander)
						{
							// Sometimes, Walk
							//-----------------
							if (Q_irand(0, 1)==0)
							{
								NPCInfo->aiFlags	|= NPCAI_WALKING;
							}

							NPCInfo->investigateDebounceTime = level.time + Q_irand(3000, 10000);
							NPCInfo->aiFlags |= NPCAI_OFF_PATH;
						}

						// Just Stand Here
						//-----------------
						else
						{
							NPCInfo->investigateDebounceTime = level.time + Q_irand(2000, 6000);
							//NPC_SetAnim(NPC, SETANIM_BOTH, ((Q_irand(0, 1)==0)?(BOTH_GUARD_LOOKAROUND1):(BOTH_GUARD_IDLE1)), SETANIM_FLAG_NORMAL);
						}
					}

					// Ok, So We Don't Have A Path, And Debounce Time Is Still Active, So We Are Either Wandering Or Looking Around
					//--------------------------------------------------------------------------------------------------------------
					else
					{
					//	if (DistFromHome>(WANDER_RANGE))
					//	{
					//		STEER::Seek(NPC, mHome);
					//	}
					//	else
						{
							if (NPCInfo->aiFlags & NPCAI_OFF_PATH)
							{
								STEER::Wander(NPC);
								STEER::AvoidCollisions(NPC);
							}
							else
							{
								STEER::Stop(NPC);
							}
						}
					}
				}
			}
		}
	}
	STEER::DeActivate(NPC, &ucmd); 

	NPC_UpdateAngles( qtrue, qtrue );
}

