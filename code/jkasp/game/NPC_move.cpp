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

//
// NPC_move.cpp
//

#include "b_local.h"
#include "g_nav.h"
#include "anims.h"
#include "g_navigator.h"

extern qboolean NPC_ClearPathToGoal( vec3_t dir, gentity_t *goal );
extern qboolean NAV_MoveDirSafe( gentity_t *self, usercmd_t *cmd, float distScale = 1.0f );

qboolean G_BoundsOverlap(const vec3_t mins1, const vec3_t maxs1, const vec3_t mins2, const vec3_t maxs2);
extern int GetTime ( int lastTime );

navInfo_t	frameNavInfo;
extern qboolean FlyingCreature( gentity_t *ent );
extern qboolean PM_InKnockDown( playerState_t *ps );

extern cvar_t	*g_navSafetyChecks;

extern qboolean Boba_Flying( gentity_t *self );
extern qboolean PM_InRoll( playerState_t *ps );

#define	APEX_HEIGHT		200.0f
#define	PARA_WIDTH		(sqrt(APEX_HEIGHT)+sqrt(APEX_HEIGHT))
#define	JUMP_SPEED		200.0f




static qboolean NPC_TryJump();




static qboolean NPC_Jump( vec3_t dest, int goalEntNum )
{//FIXME: if land on enemy, knock him down & jump off again
	float	targetDist, travelTime, impactDist, bestImpactDist = Q3_INFINITE;//fireSpeed,
	float originalShotSpeed, shotSpeed, speedStep = 50.0f, minShotSpeed = 30.0f, maxShotSpeed = 500.0f;
	qboolean belowBlocked = qfalse, aboveBlocked = qfalse;
	vec3_t	targetDir, shotVel, failCase;
	trace_t	trace;
	trajectory_t	tr;
	qboolean	blocked;
	int		elapsedTime, timeStep = 250, hitCount = 0, aboveTries = 0, belowTries = 0, maxHits = 10;
	vec3_t	lastPos, testPos, bottom;

	VectorSubtract( dest, NPC->currentOrigin, targetDir );
	targetDist = VectorNormalize( targetDir );
	//make our shotSpeed reliant on the distance
	originalShotSpeed = targetDist;//DistanceHorizontal( dest, NPC->currentOrigin )/2.0f;
	if ( originalShotSpeed > maxShotSpeed )
	{
		originalShotSpeed = maxShotSpeed;
	}
	else if ( originalShotSpeed < minShotSpeed )
	{
		originalShotSpeed = minShotSpeed;
	}
	shotSpeed = originalShotSpeed;

	while ( hitCount < maxHits )
	{
		VectorScale( targetDir, shotSpeed, shotVel );
		travelTime = targetDist/shotSpeed;
		shotVel[2] += travelTime * 0.5 * NPC->client->ps.gravity;

		if ( !hitCount )
		{//save the first one as the worst case scenario
			VectorCopy( shotVel, failCase );
		}

		if ( 1 )//tracePath )
		{//do a rough trace of the path
			blocked = qfalse;

			VectorCopy( NPC->currentOrigin, tr.trBase );
			VectorCopy( shotVel, tr.trDelta );
			tr.trType = TR_GRAVITY;
			tr.trTime = level.time;
			travelTime *= 1000.0f;
			VectorCopy( NPC->currentOrigin, lastPos );

			//This may be kind of wasteful, especially on long throws... use larger steps?  Divide the travelTime into a certain hard number of slices?  Trace just to apex and down?
			for ( elapsedTime = timeStep; elapsedTime < floor(travelTime)+timeStep; elapsedTime += timeStep )
			{
				if ( (float)elapsedTime > travelTime )
				{//cap it
					elapsedTime = floor( travelTime );
				}
				EvaluateTrajectory( &tr, level.time + elapsedTime, testPos );
				//FUCK IT, always check for do not enter...
				gi.trace( &trace, lastPos, NPC->mins, NPC->maxs, testPos, NPC->s.number, NPC->clipmask|CONTENTS_BOTCLIP, (EG2_Collision)0, 0 );
				/*
				if ( testPos[2] < lastPos[2]
					&& elapsedTime < floor( travelTime ) )
				{//going down, haven't reached end, ignore botclip
					gi.trace( &trace, lastPos, NPC->mins, NPC->maxs, testPos, NPC->s.number, NPC->clipmask );
				}
				else
				{//going up, check for botclip
					gi.trace( &trace, lastPos, NPC->mins, NPC->maxs, testPos, NPC->s.number, NPC->clipmask|CONTENTS_BOTCLIP );
				}
				*/

				if ( trace.allsolid || trace.startsolid )
				{//started in solid
					if ( NAVDEBUG_showCollision )
					{
						CG_DrawEdge( lastPos, trace.endpos, EDGE_RED_TWOSECOND );
					}
					return qfalse;//you're hosed, dude
				}
				if ( trace.fraction < 1.0f )
				{//hit something
					if ( NAVDEBUG_showCollision )
					{
						CG_DrawEdge( lastPos, trace.endpos, EDGE_RED_TWOSECOND );	// TryJump
					}
					if ( trace.entityNum == goalEntNum )
					{//hit the enemy, that's bad!
						blocked = qtrue;
						/*
						if ( g_entities[goalEntNum].client && g_entities[goalEntNum].client->ps.groundEntityNum == ENTITYNUM_NONE )
						{//bah, would collide in mid-air, no good
							blocked = qtrue;
						}
						else
						{//he's on the ground, good enough, I guess
							//Hmm, don't want to land on him, though...?
						}
						*/
						break;
					}
					else
					{
						if ( trace.contents & CONTENTS_BOTCLIP )
						{//hit a do-not-enter brush
							blocked = qtrue;
							break;
						}
						if ( trace.plane.normal[2] > 0.7 && DistanceSquared( trace.endpos, dest ) < 4096 )//hit within 64 of desired location, should be okay
						{//close enough!
							break;
						}
						else
						{//FIXME: maybe find the extents of this brush and go above or below it on next try somehow?
							impactDist = DistanceSquared( trace.endpos, dest );
							if ( impactDist < bestImpactDist )
							{
								bestImpactDist = impactDist;
								VectorCopy( shotVel, failCase );
							}
							blocked = qtrue;
							break;
						}
					}
				}
				else
				{
					if ( NAVDEBUG_showCollision )
					{
						CG_DrawEdge( lastPos, testPos, EDGE_WHITE_TWOSECOND );	// TryJump
					}
				}
				if ( elapsedTime == floor( travelTime ) )
				{//reached end, all clear
					if ( trace.fraction >= 1.0f )
					{//hmm, make sure we'll land on the ground...
						//FIXME: do we care how far below ourselves or our dest we'll land?
						VectorCopy( trace.endpos, bottom );
						bottom[2] -= 128;
						gi.trace( &trace, trace.endpos, NPC->mins, NPC->maxs, bottom, NPC->s.number, NPC->clipmask, (EG2_Collision)0, 0 );
						if ( trace.fraction >= 1.0f )
						{//would fall too far
							blocked = qtrue;
						}
					}
					break;
				}
				else
				{
					//all clear, try next slice
					VectorCopy( testPos, lastPos );
				}
			}
			if ( blocked )
			{//hit something, adjust speed (which will change arc)
				hitCount++;
				//alternate back and forth between trying an arc slightly above or below the ideal
				if ( (hitCount%2) && !belowBlocked )
				{//odd
					belowTries++;
					shotSpeed = originalShotSpeed - (belowTries*speedStep);
				}
				else if ( !aboveBlocked )
				{//even
					aboveTries++;
					shotSpeed = originalShotSpeed + (aboveTries*speedStep);
				}
				else
				{//can't go any higher or lower
					hitCount = maxHits;
					break;
				}
				if ( shotSpeed > maxShotSpeed )
				{
					shotSpeed = maxShotSpeed;
					aboveBlocked = qtrue;
				}
				else if ( shotSpeed < minShotSpeed )
				{
					shotSpeed = minShotSpeed;
					belowBlocked = qtrue;
				}
			}
			else
			{//made it!
				break;
			}
		}
		else
		{//no need to check the path, go with first calc
			break;
		}
	}

	if ( hitCount >= maxHits )
	{//NOTE: worst case scenario, use the one that impacted closest to the target (or just use the first try...?)
		return qfalse;
		//NOTE: or try failcase?
		//VectorCopy( failCase, NPC->client->ps.velocity );
		//return qtrue;
	}
	VectorCopy( shotVel, NPC->client->ps.velocity );
	return qtrue;
}






#define NPC_JUMP_PREP_BACKUP_DIST 34.0f

trace_t		mJumpTrace;



qboolean NPC_CanTryJump()
{
	if (!(NPCInfo->scriptFlags&SCF_NAV_CAN_JUMP)	||		// Can't Jump
		(NPCInfo->scriptFlags&SCF_NO_ACROBATICS)	||		// If Can't Jump At All
		(level.time<NPCInfo->jumpBackupTime)		||		// If Backing Up, Don't Try The Jump Again
		(level.time<NPCInfo->jumpNextCheckTime)		||		// Don't Even Try To Jump Again For This Amount Of Time
		(NPCInfo->jumpTime)							||		// Don't Jump If Already Going
		(PM_InKnockDown(&NPC->client->ps))			||		// Don't Jump If In Knockdown
		(PM_InRoll(&NPC->client->ps))				||		// ... Or Roll
		(NPC->client->ps.groundEntityNum==ENTITYNUM_NONE)	// ... Or In The Air
		)
	{
		return qfalse;
	}
	return qtrue;
}

qboolean NPC_TryJump(const vec3_t& pos,	float max_xy_dist, float max_z_diff)
{
	if (NPC_CanTryJump())
	{
		NPCInfo->jumpNextCheckTime	= level.time + Q_irand(1000, 2000);

		VectorCopy(pos, NPCInfo->jumpDest);

		// Can't Try To Jump At A Point In The Air
		//-----------------------------------------
		{
			vec3_t	groundTest;
			VectorCopy(pos, groundTest);
			groundTest[2]	+= (NPC->mins[2]*3);
			gi.trace(&mJumpTrace, NPCInfo->jumpDest, vec3_origin, vec3_origin, groundTest, NPC->s.number, NPC->clipmask, (EG2_Collision)0, 0 );
			if (mJumpTrace.fraction >= 1.0f)
			{
				return qfalse;	//no ground = no jump
			}
		}
		NPCInfo->jumpTarget			= 0;
		NPCInfo->jumpMaxXYDist		= (max_xy_dist)?(max_xy_dist):((NPC->client->NPC_class==CLASS_ROCKETTROOPER)?1200:750);
		NPCInfo->jumpMazZDist		= (max_z_diff)?(max_z_diff):((NPC->client->NPC_class==CLASS_ROCKETTROOPER)?-1000:-450);
		NPCInfo->jumpTime			= 0;
		NPCInfo->jumpBackupTime		= 0;
		return NPC_TryJump();
	}
	return qfalse;
}

qboolean NPC_TryJump(gentity_t *goal,	float max_xy_dist, float max_z_diff)
{
	if (NPC_CanTryJump())
	{
		NPCInfo->jumpNextCheckTime	= level.time + Q_irand(1000, 3000);

		// Can't Jump At Targets In The Air
		//---------------------------------
		if (goal->client && goal->client->ps.groundEntityNum==ENTITYNUM_NONE)
		{
			return qfalse;
		}
		VectorCopy(goal->currentOrigin, NPCInfo->jumpDest);
		NPCInfo->jumpTarget			= goal;
		NPCInfo->jumpMaxXYDist		= (max_xy_dist)?(max_xy_dist):((NPC->client->NPC_class==CLASS_ROCKETTROOPER)?1200:750);
		NPCInfo->jumpMazZDist		= (max_z_diff)?(max_z_diff):((NPC->client->NPC_class==CLASS_ROCKETTROOPER)?-1000:-400);
		NPCInfo->jumpTime			= 0;
		NPCInfo->jumpBackupTime		= 0;
		return NPC_TryJump();
	}
	return qfalse;
}

void	 NPC_JumpAnimation()
{
	int	jumpAnim = BOTH_JUMP1;

	if ( NPC->client->NPC_class == CLASS_BOBAFETT
		|| (NPC->client->NPC_class == CLASS_REBORN && NPC->s.weapon != WP_SABER)
		|| NPC->client->NPC_class == CLASS_ROCKETTROOPER
		||( NPCInfo->rank != RANK_CREWMAN && NPCInfo->rank <= RANK_LT_JG ) )
	{//can't do acrobatics
		jumpAnim = BOTH_FORCEJUMP1;
	}
	else if (NPC->client->NPC_class != CLASS_HOWLER)
	{
		if ( NPC->client->NPC_class == CLASS_ALORA && Q_irand( 0, 3 ) )
		{
			jumpAnim = Q_irand( BOTH_ALORA_FLIP_1, BOTH_ALORA_FLIP_3 );
		}
		else
		{
			jumpAnim = BOTH_FLIP_F;
		}
	}
	NPC_SetAnim( NPC, SETANIM_BOTH, jumpAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
}

extern void JET_FlyStart(gentity_t* actor);

void	 NPC_JumpSound()
{
	if ( NPC->client->NPC_class == CLASS_HOWLER )
	{
		//FIXME: can I delay the actual jump so that it matches the anim...?
	}
	else if ( NPC->client->NPC_class == CLASS_BOBAFETT
		|| NPC->client->NPC_class == CLASS_ROCKETTROOPER )
	{
		// does this really need to be here?
		JET_FlyStart(NPC);
	}
	else
	{
		G_SoundOnEnt( NPC, CHAN_BODY, "sound/weapons/force/jump.wav" );
	}
}

qboolean NPC_TryJump()
{
	vec3_t	targetDirection;
	float	targetDistanceXY;
	float	targetDistanceZ;

	// Get The Direction And Distances To The Target
	//-----------------------------------------------
	VectorSubtract(NPCInfo->jumpDest, NPC->currentOrigin, targetDirection);
	targetDirection[2]	= 0.0f;
	targetDistanceXY	= VectorNormalize(targetDirection);
	targetDistanceZ		= NPCInfo->jumpDest[2] - NPC->currentOrigin[2];

	if ((targetDistanceXY>NPCInfo->jumpMaxXYDist) ||
		(targetDistanceZ<NPCInfo->jumpMazZDist))
	{
		return qfalse;
	}


	// Test To See If There Is A Wall Directly In Front Of Actor, If So, Backup Some
	//-------------------------------------------------------------------------------
	if (TIMER_Done(NPC, "jumpBackupDebounce"))
	{
		vec3_t	actorProjectedTowardTarget;
		VectorMA(NPC->currentOrigin, NPC_JUMP_PREP_BACKUP_DIST, targetDirection, actorProjectedTowardTarget);
		gi.trace(&mJumpTrace, NPC->currentOrigin, vec3_origin, vec3_origin, actorProjectedTowardTarget, NPC->s.number, NPC->clipmask, (EG2_Collision)0, 0);
		if ((mJumpTrace.fraction < 1.0f) ||
			(mJumpTrace.allsolid) ||
			(mJumpTrace.startsolid))
		{
			if (NAVDEBUG_showCollision)
			{
				CG_DrawEdge(NPC->currentOrigin, actorProjectedTowardTarget, EDGE_RED_TWOSECOND);	// TryJump
			}

			// TODO: We may want to test to see if it is safe to back up here?
			NPCInfo->jumpBackupTime = level.time + 1000;
			TIMER_Set(NPC, "jumpBackupDebounce", 5000);
			return qtrue;
		}
	}


//	bool	Wounded					= (NPC->health < 150);
//	bool	OnLowerLedge			= ((targetDistanceZ<-80.0f) && (targetDistanceZ>-200.0f));
//	bool	WithinNormalJumpRange	= ((targetDistanceZ<32.0f)  && (targetDistanceXY<200.0f));
	bool	WithinForceJumpRange	= ((fabsf(targetDistanceZ)>0) || (targetDistanceXY>128));

/*	if (Wounded && OnLowerLedge)
	{
		ucmd.forwardmove	= 127;
		VectorClear(NPC->client->ps.moveDir);
		TIMER_Set(NPC, "duck", -level.time);
		return qtrue;
	}

	if (WithinNormalJumpRange)
	{
		ucmd.upmove			= 127;
		ucmd.forwardmove	= 127;
		VectorClear(NPC->client->ps.moveDir);
		TIMER_Set(NPC, "duck", -level.time);
		return qtrue;
	}
*/

	if (!WithinForceJumpRange)
	{
		return qfalse;
	}



	// If There Is Any Chance That This Jump Will Land On An Enemy, Try 8 Different Traces Around The Target
	//-------------------------------------------------------------------------------------------------------
	if (NPCInfo->jumpTarget)
	{
		float	minSafeRadius	= (NPC->maxs[0]*1.5f) + (NPCInfo->jumpTarget->maxs[0]*1.5f);
		float	minSafeRadiusSq	= (minSafeRadius * minSafeRadius);

		if (DistanceSquared(NPCInfo->jumpDest, NPCInfo->jumpTarget->currentOrigin)<minSafeRadiusSq)
		{
			vec3_t	startPos;
			vec3_t	floorPos;
			VectorCopy(NPCInfo->jumpDest, startPos);

			floorPos[2] = NPCInfo->jumpDest[2] + (NPC->mins[2]-32);

			for (int sideTryCount=0; sideTryCount<8; sideTryCount++)
			{
				NPCInfo->jumpSide++;
				if ( NPCInfo->jumpSide > 7 )
				{
					NPCInfo->jumpSide = 0;
				}

				switch ( NPCInfo->jumpSide )
				{
				case 0:
					NPCInfo->jumpDest[0] = startPos[0] + minSafeRadius;
					NPCInfo->jumpDest[1] = startPos[1];
					break;
				case 1:
					NPCInfo->jumpDest[0] = startPos[0] + minSafeRadius;
					NPCInfo->jumpDest[1] = startPos[1] + minSafeRadius;
					break;
				case 2:
					NPCInfo->jumpDest[0] = startPos[0];
					NPCInfo->jumpDest[1] = startPos[1] + minSafeRadius;
					break;
				case 3:
					NPCInfo->jumpDest[0] = startPos[0] - minSafeRadius;
					NPCInfo->jumpDest[1] = startPos[1] + minSafeRadius;
					break;
				case 4:
					NPCInfo->jumpDest[0] = startPos[0] - minSafeRadius;
					NPCInfo->jumpDest[1] = startPos[1];
					break;
				case 5:
					NPCInfo->jumpDest[0] = startPos[0] - minSafeRadius;
					NPCInfo->jumpDest[1] = startPos[1] - minSafeRadius;
					break;
				case 6:
					NPCInfo->jumpDest[0] = startPos[0];
					NPCInfo->jumpDest[1] = startPos[1] - minSafeRadius;
					break;
				case 7:
					NPCInfo->jumpDest[0] = startPos[0] + minSafeRadius;
					NPCInfo->jumpDest[1] = startPos[1] -=minSafeRadius;
					break;
				}

				floorPos[0] = NPCInfo->jumpDest[0];
				floorPos[1] = NPCInfo->jumpDest[1];

				gi.trace(&mJumpTrace, NPCInfo->jumpDest, NPC->mins, NPC->maxs, floorPos, (NPCInfo->jumpTarget)?(NPCInfo->jumpTarget->s.number):(NPC->s.number), (NPC->clipmask|CONTENTS_BOTCLIP), (EG2_Collision)0, 0);
				if ((mJumpTrace.fraction<1.0f) &&
					(!mJumpTrace.allsolid) &&
					(!mJumpTrace.startsolid))
				{
					break;
				}

				if ( NAVDEBUG_showCollision )
				{
					CG_DrawEdge( NPCInfo->jumpDest, floorPos, EDGE_RED_TWOSECOND );
				}
			}

			// If All Traces Failed, Just Try Going Right Back At The Target Location
			//------------------------------------------------------------------------
			if ((mJumpTrace.fraction>=1.0f) ||
				(mJumpTrace.allsolid) ||
				(mJumpTrace.startsolid))
			{
				VectorCopy(startPos, NPCInfo->jumpDest);
			}
		}
	}

	// Now, Actually Try The Jump To The Dest Target
	//-----------------------------------------------
	if (NPC_Jump(NPCInfo->jumpDest, (NPCInfo->jumpTarget)?(NPCInfo->jumpTarget->s.number):(NPC->s.number)))
	{
		// We Made IT!
		//-------------
		NPC_JumpAnimation();
		NPC_JumpSound();

		NPC->client->ps.forceJumpZStart		 = NPC->currentOrigin[2];
		NPC->client->ps.pm_flags			|= PMF_JUMPING;
		NPC->client->ps.weaponTime			 = NPC->client->ps.torsoAnimTimer;
		NPC->client->ps.forcePowersActive	|= ( 1 << FP_LEVITATION );
		ucmd.forwardmove					 = 0;
		NPCInfo->jumpTime					 = 1;

		VectorClear(NPC->client->ps.moveDir);
		TIMER_Set(NPC, "duck", -level.time);

		return qtrue;
	}
	return qfalse;
}

qboolean NPC_Jumping()
{
	if ( NPCInfo->jumpTime )
	{
		if ( !(NPC->client->ps.pm_flags & PMF_JUMPING )//forceJumpZStart )
			&& !(NPC->client->ps.pm_flags&PMF_TRIGGER_PUSHED))
		{//landed
			NPCInfo->jumpTime = 0;
		}
		else
		{
		//	if (NPCInfo->jumpTarget)
		//	{
		//		NPC_FaceEntity(NPCInfo->jumpTarget, qtrue);
		//	}
		//	else
			{
				NPC_FacePosition(NPCInfo->jumpDest, qtrue);
			}
			return qtrue;
		}
	}
	return qfalse;
}

qboolean NPC_JumpBackingUp()
{
	if (NPCInfo->jumpBackupTime)
	{
		if (level.time<NPCInfo->jumpBackupTime)
		{
			STEER::Activate(NPC);
			STEER::Flee(NPC, NPCInfo->jumpDest);
			STEER::DeActivate(NPC, &ucmd);
			NPC_FacePosition(NPCInfo->jumpDest, qtrue);
			NPC_UpdateAngles( qfalse, qtrue );
			return qtrue;
		}

		NPCInfo->jumpBackupTime = 0;
		return NPC_TryJump();
	}
	return qfalse;
}




/*
-------------------------
NPC_CheckCombatMove
-------------------------
*/

inline qboolean NPC_CheckCombatMove( void )
{
	//return NPCInfo->combatMove;
	if ( ( NPCInfo->goalEntity && NPC->enemy && NPCInfo->goalEntity == NPC->enemy ) || ( NPCInfo->combatMove ) )
	{
		return qtrue;
	}

	if ( NPCInfo->goalEntity && NPCInfo->watchTarget )
	{
		if ( NPCInfo->goalEntity != NPCInfo->watchTarget )
		{
			return qtrue;
		}
	}

	return qfalse;
}

/*
-------------------------
NPC_LadderMove
-------------------------
*/

static void NPC_LadderMove( vec3_t dir )
{
	//FIXME: this doesn't guarantee we're facing ladder
	//ALSO: Need to be able to get off at top
	//ALSO: Need to play an anim
	//ALSO: Need transitionary anims?

	if ( ( dir[2] > 0 ) || ( dir[2] < 0 && NPC->client->ps.groundEntityNum == ENTITYNUM_NONE ) )
	{
		//Set our movement direction
		ucmd.upmove = (dir[2] > 0) ? 127 : -127;

		//Don't move around on XY
		ucmd.forwardmove = ucmd.rightmove = 0;
	}
}

/*
-------------------------
NPC_GetMoveInformation
-------------------------
*/

inline qboolean NPC_GetMoveInformation( vec3_t dir, float *distance )
{
	//NOTENOTE: Use path stacks!

	//Make sure we have somewhere to go
	if ( NPCInfo->goalEntity == NULL )
		return qfalse;

	//Get our move info
	VectorSubtract( NPCInfo->goalEntity->currentOrigin, NPC->currentOrigin, dir );
	*distance = VectorNormalize( dir );

	VectorCopy( NPCInfo->goalEntity->currentOrigin, NPCInfo->blockedTargetPosition );

	return qtrue;
}

/*
-------------------------
NAV_GetLastMove
-------------------------
*/

void NAV_GetLastMove( navInfo_t &info )
{
	info = frameNavInfo;
}


void G_UcmdMoveForDir( gentity_t *self, usercmd_t *cmd, vec3_t dir )
{
	vec3_t	forward, right;

	AngleVectors( self->currentAngles, forward, right, NULL );

	dir[2] = 0;
	VectorNormalize( dir );
	//NPCs cheat and store this directly because converting movement into a ucmd loses precision
	VectorCopy( dir, self->client->ps.moveDir );

	float fDot = DotProduct( forward, dir ) * 127.0f;
	float rDot = DotProduct( right, dir ) * 127.0f;
	//Must clamp this because DotProduct is not guaranteed to return a number within -1 to 1, and that would be bad when we're shoving this into a signed byte
	if ( fDot > 127.0f )
	{
		fDot = 127.0f;
	}
	if ( fDot < -127.0f )
	{
		fDot = -127.0f;
	}
	if ( rDot > 127.0f )
	{
		rDot = 127.0f;
	}
	if ( rDot < -127.0f )
	{
		rDot = -127.0f;
	}
	cmd->forwardmove = floor(fDot);
	cmd->rightmove = floor(rDot);

	/*
	vec3_t	wishvel;
	for ( int i = 0 ; i < 3 ; i++ )
	{
		wishvel[i] = forward[i]*cmd->forwardmove + right[i]*cmd->rightmove;
	}
	VectorNormalize( wishvel );
	if ( !VectorCompare( wishvel, dir ) )
	{
		Com_Printf( "PRECISION LOSS: %s != %s\n", vtos(wishvel), vtos(dir) );
	}
	*/
}

/*
-------------------------
NPC_MoveToGoal

  Now assumes goal is goalEntity, was no reason for it to be otherwise
-------------------------
*/

#if	AI_TIMERS
extern int navTime;
#endif//	AI_TIMERS
qboolean NPC_MoveToGoal( qboolean tryStraight ) //FIXME: tryStraight not even used!  Stop passing it
{
#if	AI_TIMERS
	int	startTime = GetTime(0);
#endif//	AI_TIMERS

	if ( PM_InKnockDown( &NPC->client->ps ) || ( ( NPC->client->ps.legsAnim >= BOTH_PAIN1 ) && ( NPC->client->ps.legsAnim <= BOTH_PAIN18 ) && NPC->client->ps.legsAnimTimer > 0 ) )
	{//If taking full body pain, don't move
		return qtrue;
	}

	if( NPC->s.eFlags & EF_LOCKED_TO_WEAPON )
	{//If in an emplaced gun, never try to navigate!
		return qtrue;
	}

	if( NPC->s.eFlags & EF_HELD_BY_RANCOR )
	{//If in a rancor's hand, never try to navigate!
		return qtrue;
	}
	if( NPC->s.eFlags & EF_HELD_BY_WAMPA )
	{//If in a wampa's hand, never try to navigate!
		return qtrue;
	}
	if( NPC->s.eFlags & EF_HELD_BY_SAND_CREATURE )
	{//If in a worm's mouth, never try to navigate!
		return qtrue;
	}

	if ( NPC->watertype & CONTENTS_LADDER )
	{//Do we still want to do this?
		vec3_t	dir;
		VectorSubtract( NPCInfo->goalEntity->currentOrigin, NPC->currentOrigin, dir );
		VectorNormalize( dir );
		NPC_LadderMove( dir );
	}


	bool	moveSuccess = true;
	STEER::Activate(NPC);
	{
		// Attempt To Steer Directly To Our Goal
		//---------------------------------------
		moveSuccess = STEER::GoTo(NPC,  NPCInfo->goalEntity, NPCInfo->goalRadius);

		// Perhaps Not Close Enough?  Try To Use The Navigation Grid
		//-----------------------------------------------------------
		if (!moveSuccess)
		{
			moveSuccess = NAV::GoTo(NPC, NPCInfo->goalEntity);
			if (!moveSuccess)
			{
				STEER::Stop(NPC);
			}
		}
	}
	STEER::DeActivate(NPC, &ucmd);


	#if	AI_TIMERS
		navTime += GetTime( startTime );
	#endif//	AI_TIMERS
	return (qboolean)moveSuccess;
}

/*
-------------------------
void NPC_SlideMoveToGoal( void )

  Now assumes goal is goalEntity, if want to use tempGoal, you set that before calling the func
-------------------------
*/
qboolean NPC_SlideMoveToGoal( void )
{
	float	saveYaw = NPC->client->ps.viewangles[YAW];

	NPCInfo->combatMove = qtrue;

	qboolean ret = NPC_MoveToGoal( qtrue );

	NPCInfo->desiredYaw	= saveYaw;

	return ret;
}


/*
-------------------------
NPC_ApplyRoff
-------------------------
*/

void NPC_ApplyRoff(void)
{
	PlayerStateToEntityState( &NPC->client->ps, &NPC->s );
	VectorCopy ( NPC->currentOrigin, NPC->lastOrigin );

	// use the precise origin for linking
	gi.linkentity(NPC);
}

