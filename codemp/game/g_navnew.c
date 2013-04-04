#include "b_local.h"
#include "g_nav.h"

qboolean NAV_CheckAhead( gentity_t *self, vec3_t end, trace_t *trace, int clipmask );
qboolean NAV_TestForBlocked( gentity_t *self, gentity_t *goal, gentity_t *blocker, float distance, int *flags );

void G_Line( vec3_t start, vec3_t end, vec3_t color, float alpha );
void G_Cube( vec3_t mins, vec3_t maxs, vec3_t color, float alpha );
void G_CubeOutline( vec3_t mins, vec3_t maxs, int time, unsigned int color, float alpha );
void G_DrawEdge( vec3_t start, vec3_t end, int type );
void G_DrawNode( vec3_t origin, int type );
void G_DrawCombatPoint( vec3_t origin, int type );
void TAG_ShowTags( int flags );

qboolean NAV_CheckNodeFailedForEnt( gentity_t *ent, int nodeNum )
{
	int j;

	//FIXME: must be a better way to do this
	for ( j = 0; j < MAX_FAILED_NODES; j++ )
	{
		if ( ent->failedWaypoints[j] == nodeNum+1 )//+1 because 0 is a valid nodeNum, but also the default
		{//we failed against this node
			return qtrue;
		}
	}
	return qfalse;
}
/*
-------------------------
NPC_UnBlocked
-------------------------
*/
void NPC_ClearBlocked( gentity_t *self )
{
	if ( self->NPC == NULL )
		return;

	//self->NPC->aiFlags &= ~NPCAI_BLOCKED;
	self->NPC->blockingEntNum = ENTITYNUM_NONE;
}

void NPC_SetBlocked( gentity_t *self, gentity_t *blocker )
{
	if ( self->NPC == NULL )
		return;

	//self->NPC->aiFlags |= NPCAI_BLOCKED;
	self->NPC->blockedSpeechDebounceTime = level.time + MIN_BLOCKED_SPEECH_TIME + ( random() * 4000 );
	self->NPC->blockingEntNum = blocker->s.number;
}

/*
-------------------------
NAVNEW_ClearPathBetweenPoints
-------------------------
*/
int NAVNEW_ClearPathBetweenPoints(vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, int ignore, int clipmask)
{
	trace_t	trace;

	//Test if they're even conceivably close to one another
	if ( !trap_InPVS( start, end ) )
	{
		return ENTITYNUM_WORLD;
	}

	trap_Trace( &trace, start, mins, maxs, end, ignore, clipmask );

	//if( ( ( trace.startsolid == false ) && ( trace.allsolid == false ) ) && ( trace.fraction < 1.0f ) )
	//{//FIXME: check for drops?
	//FIXME: if startsolid or allsolid, then the path isn't clear... but returning ENTITYNUM_NONE indicates to CheckFailedEdge that is is clear...?
		return trace.entityNum;
	//}

	//return ENTITYNUM_NONE;
}

/*
-------------------------
NAVNEW_PushBlocker
-------------------------
*/
void NAVNEW_PushBlocker( gentity_t *self, gentity_t *blocker, vec3_t right, qboolean setBlockedInfo )
{//try pushing blocker to one side
	trace_t	tr;
	vec3_t	mins, end;
	float	rightSucc, leftSucc, moveamt;

	if ( self->NPC->shoveCount > 30 )
	{//don't push for more than 3 seconds;
		return;
	}

	if ( !blocker->s.number )
	{//never push the player
		return;
	}

	if ( !blocker->client || !VectorCompare( blocker->client->pushVec, vec3_origin ) )
	{//someone else is pushing him, wait until they give up?
		return;
	}

	VectorCopy( blocker->r.mins, mins );
	mins[2] += STEPSIZE;

	moveamt = (self->r.maxs[1] + blocker->r.maxs[1]) * 1.2;//yes, magic number

	VectorMA( blocker->r.currentOrigin, -moveamt, right, end );
	trap_Trace( &tr, blocker->r.currentOrigin, mins, blocker->r.maxs, end, blocker->s.number, blocker->clipmask|CONTENTS_BOTCLIP);
	if ( !tr.startsolid && !tr.allsolid )
	{
		leftSucc = tr.fraction;
	}
	else
	{
		leftSucc = 0.0f;
	}
	
	if ( leftSucc >= 1.0f )
	{//it's clear, shove him that way
		VectorScale( right, -moveamt, blocker->client->pushVec );
		blocker->client->pushVecTime = level.time + 2000;
	}
	else
	{
		VectorMA( blocker->r.currentOrigin, moveamt, right, end );
		trap_Trace( &tr, blocker->r.currentOrigin, mins, blocker->r.maxs, end, blocker->s.number, blocker->clipmask|CONTENTS_BOTCLIP );
		if ( !tr.startsolid && !tr.allsolid )
		{
			rightSucc = tr.fraction;
		}
		else
		{
			rightSucc = 0.0f;
		}
		
		if ( leftSucc == 0.0f && rightSucc == 0.0f )
		{//both sides failed
			if ( d_patched.integer )
			{//use patch-style navigation
				blocker->client->pushVecTime = 0;
			}
			return;
		}

		if ( rightSucc >= 1.0f )
		{//it's clear, shove him that way
			VectorScale( right, moveamt, blocker->client->pushVec );
			blocker->client->pushVecTime = level.time + 2000;
		}
		//if neither are enough, we probably can't get around him, but keep trying
		else if ( leftSucc >= rightSucc )
		{//favor the left, all things being equal
			VectorScale( right, -moveamt, blocker->client->pushVec );
			blocker->client->pushVecTime = level.time + 2000;
		}
		else
		{
			VectorScale( right, moveamt, blocker->client->pushVec );
			blocker->client->pushVecTime = level.time + 2000;
		}
	}

	if ( setBlockedInfo )
	{
		//we tried pushing
		self->NPC->shoveCount++;
	}
}

/*
-------------------------
NAVNEW_DanceWithBlocker
-------------------------
*/
qboolean NAVNEW_DanceWithBlocker( gentity_t *self, gentity_t *blocker, vec3_t movedir, vec3_t right )
{//sees if blocker has any lateral movement
	if ( blocker->client && !VectorCompare( blocker->client->ps.velocity, vec3_origin ) )
	{
		vec3_t blocker_movedir;
		float dot;

		VectorCopy( blocker->client->ps.velocity, blocker_movedir );
		blocker_movedir[2] = 0;//cancel any vertical motion
		dot = DotProduct( blocker_movedir, right );
		if ( dot > 50.0f )
		{//he's moving to the right of me at a relatively good speed
			//go to my left
			VectorMA( movedir, -1, right, movedir );
			VectorNormalize( movedir );
			return qtrue;
		}
		else if ( dot > -50.0f )
		{//he's moving to the left of me at a relatively good speed
			//go to my right
			VectorAdd( right, movedir, movedir );
			VectorNormalize( movedir );
			return qtrue;
		}
		/*
		vec3_t	block_pos;
		trace_t	tr;
		VectorScale( blocker_movedir, -1, blocker_movedir );
		VectorMA( self->r.currentOrigin, blocked_dist, blocker_movedir, block_pos );
		if ( NAVNEW_CheckAhead( self, block_pos, tr, ( self->clipmask & ~CONTENTS_BODY )|CONTENTS_BOTCLIP ) )
		{
			VectorCopy( blocker_movedir, movedir );
			return qtrue;
		}
		*/
	}
	return qfalse;
}

/*
-------------------------
NAVNEW_SidestepBlocker
-------------------------
*/
qboolean NAVNEW_SidestepBlocker( gentity_t *self, gentity_t *blocker, vec3_t blocked_dir, float blocked_dist, vec3_t movedir, vec3_t right )
{//trace to sides of blocker and see if either is clear
	trace_t	tr;
	vec3_t	avoidAngles;
	vec3_t	avoidRight_dir, avoidLeft_dir, block_pos, mins;
	float	rightSucc, leftSucc, yaw, avoidRadius, arcAngle;

	VectorCopy( self->r.mins, mins );
	mins[2] += STEPSIZE;

	//Get the blocked direction
	yaw = vectoyaw( blocked_dir );

	//Get the avoid radius
	avoidRadius = sqrt( ( blocker->r.maxs[0] * blocker->r.maxs[0] ) + ( blocker->r.maxs[1] * blocker->r.maxs[1] ) ) + 
						sqrt( ( self->r.maxs[0] * self->r.maxs[0] ) + ( self->r.maxs[1] * self->r.maxs[1] ) );

	//See if we're inside our avoidance radius
	arcAngle = ( blocked_dist <= avoidRadius ) ? 135 : ( ( avoidRadius / blocked_dist ) * 90 );

	/*
	float dot = DotProduct( blocked_dir, right );

	//Go right on the first try if that works better
	if ( dot < 0.0f )
		arcAngle *= -1;
	*/

	VectorClear( avoidAngles );

	//need to stop it from ping-ponging, so we have a bit of a debounce time on which side you try
	if ( self->NPC->sideStepHoldTime > level.time )
	{
		if ( self->NPC->lastSideStepSide == -1 )//left
		{
			arcAngle *= -1;
		}//else right
		avoidAngles[YAW] = AngleNormalize360( yaw + arcAngle );
		AngleVectors( avoidAngles, movedir, NULL, NULL );
		VectorMA( self->r.currentOrigin, blocked_dist, movedir, block_pos );
		trap_Trace( &tr, self->r.currentOrigin, mins, self->r.maxs, block_pos, self->s.number, self->clipmask|CONTENTS_BOTCLIP );
		return (tr.fraction==1.0&&!tr.allsolid&&!tr.startsolid);
	}

	//test right
	avoidAngles[YAW] = AngleNormalize360( yaw + arcAngle );
	AngleVectors( avoidAngles, avoidRight_dir, NULL, NULL );

	VectorMA( self->r.currentOrigin, blocked_dist, avoidRight_dir, block_pos );
		
	trap_Trace( &tr, self->r.currentOrigin, mins, self->r.maxs, block_pos, self->s.number, self->clipmask|CONTENTS_BOTCLIP );

	if ( !tr.allsolid && !tr.startsolid )
	{
		if ( tr.fraction >= 1.0f )
		{//all clear, go for it (favor the right if both are equal)
			VectorCopy( avoidRight_dir, movedir );
			self->NPC->lastSideStepSide = 1;
			self->NPC->sideStepHoldTime = level.time + 2000;
			return qtrue;
		}
		rightSucc = tr.fraction;
	}
	else
	{
		rightSucc = 0.0f;
	}

	//now test left
	arcAngle *= -1;

	avoidAngles[YAW] = AngleNormalize360( yaw + arcAngle );
	AngleVectors( avoidAngles, avoidLeft_dir, NULL, NULL );

	VectorMA( self->r.currentOrigin, blocked_dist, avoidLeft_dir, block_pos );
		
	trap_Trace( &tr, self->r.currentOrigin, mins, self->r.maxs, block_pos, self->s.number, self->clipmask|CONTENTS_BOTCLIP );

	if ( !tr.allsolid && !tr.startsolid )
	{
		if ( tr.fraction >= 1.0f )
		{//all clear, go for it (right side would have already succeeded if as good as this)
			VectorCopy( avoidLeft_dir, movedir );
			self->NPC->lastSideStepSide = -1;
			self->NPC->sideStepHoldTime = level.time + 2000;
			return qtrue;
		}
		leftSucc = tr.fraction;
	}
	else
	{
		leftSucc = 0.0f;
	}

	if ( leftSucc == 0.0f && rightSucc == 0.0f )
	{//both sides failed
		return qfalse;
	}

	if ( rightSucc*blocked_dist >= avoidRadius || leftSucc*blocked_dist >= avoidRadius ) 
	{//the traces hit something, but got a relatively good distance
		if ( rightSucc >= leftSucc )
		{//favor the right, all things being equal
			VectorCopy( avoidRight_dir, movedir );
			self->NPC->lastSideStepSide = 1;
			self->NPC->sideStepHoldTime = level.time + 2000;
		}
		else
		{
			VectorCopy( avoidLeft_dir, movedir );
			self->NPC->lastSideStepSide = -1;
			self->NPC->sideStepHoldTime = level.time + 2000;
		}
		return qtrue;
	}

	//if neither are enough, we probably can't get around him
	return qfalse;
}

/*
-------------------------
NAVNEW_Bypass
-------------------------
*/
qboolean NAVNEW_Bypass( gentity_t *self, gentity_t *blocker, vec3_t blocked_dir, float blocked_dist, vec3_t movedir, qboolean setBlockedInfo ) 
{
	vec3_t	moveangles, right;

	//Draw debug info if requested
	if ( NAVDEBUG_showCollision )
	{
		G_DrawEdge( self->r.currentOrigin, blocker->r.currentOrigin, EDGE_NORMAL );
	}

	vectoangles( movedir, moveangles );
	moveangles[2] = 0;
	AngleVectors( moveangles, NULL, right, NULL );

	//Check to see what dir the other guy is moving in (if any) and pick the opposite dir
	if ( NAVNEW_DanceWithBlocker( self, blocker, movedir, right ) )
	{
		return qtrue;
	}

	//Okay, so he's not moving to my side, see which side of him is most clear
	if ( NAVNEW_SidestepBlocker( self, blocker, blocked_dir, blocked_dist, movedir, right ) )
	{
		return qtrue;
	}

	//Neither side is clear, tell him to step aside
	NAVNEW_PushBlocker( self, blocker, right, setBlockedInfo );

	return qfalse;
}

/*
-------------------------
NAVNEW_CheckDoubleBlock
-------------------------
*/
qboolean NAVNEW_CheckDoubleBlock( gentity_t *self, gentity_t *blocker, vec3_t blocked_dir )
{
	//Stop double waiting
	if ( ( blocker->NPC ) && ( blocker->NPC->blockingEntNum == self->s.number ) )
		return qtrue;

	return qfalse;
}

/*
-------------------------
NAVNEW_ResolveEntityCollision
-------------------------
*/
extern void CalcTeamDoorCenter ( gentity_t *ent, vec3_t center );
qboolean NAVNEW_ResolveEntityCollision( gentity_t *self, gentity_t *blocker, vec3_t movedir, vec3_t pathDir, qboolean setBlockedInfo )
{
	vec3_t	blocked_dir;
	float blocked_dist;

	//Doors are ignored
	if ( Q_stricmp( blocker->classname, "func_door" ) == 0 )
	{
		vec3_t center;
		CalcTeamDoorCenter ( blocker, center );
		if ( DistanceSquared( self->r.currentOrigin, center ) > MIN_DOOR_BLOCK_DIST_SQR )
			return qtrue;
	}

	VectorSubtract( blocker->r.currentOrigin, self->r.currentOrigin, blocked_dir );
	blocked_dist = VectorNormalize( blocked_dir );

	//Make sure an actual collision is going to happen
//	if ( NAVNEW_PredictCollision( self, blocker, movedir, blocked_dir ) == qfalse )
//		return qtrue;
	
	//First, attempt to walk around the blocker or shove him out of the way
	if ( NAVNEW_Bypass( self, blocker, blocked_dir, blocked_dist, movedir, setBlockedInfo ) )
		return qtrue;

	//Can't get around him... see if I'm blocking him too... if so, I need to just keep moving?
	if ( NAVNEW_CheckDoubleBlock( self, blocker, blocked_dir ) )
		return qtrue;

	if ( setBlockedInfo )
	{
		//Complain about it if we can
		NPC_SetBlocked( self, blocker );
	}

	return qfalse;
}

/*
-------------------------
NAVNEW_AvoidCollision
-------------------------
*/
qboolean NAVNEW_AvoidCollision( gentity_t *self, gentity_t *goal, navInfo_t *info, qboolean setBlockedInfo, int blockedMovesLimit )
{
	vec3_t	movedir;
	vec3_t	movepos;

	//Cap our distance
	if ( info->distance > MAX_COLL_AVOID_DIST )
	{
		info->distance = MAX_COLL_AVOID_DIST;
	}

	//Get an end position
	VectorMA( self->r.currentOrigin, info->distance, info->direction, movepos );
	VectorCopy( info->direction, movedir );

	//Now test against entities
	if ( NAV_CheckAhead( self, movepos, &info->trace, CONTENTS_BODY ) == qfalse )
	{
		//Get the blocker
		info->blocker = &g_entities[ info->trace.entityNum ];
		info->flags |= NIF_COLLISION;

		//Ok to hit our goal entity
		if ( goal == info->blocker )
			return qtrue;

		if ( setBlockedInfo )
		{
			if ( self->NPC->consecutiveBlockedMoves > blockedMovesLimit  )
			{
				if ( d_patched.integer )
				{//use patch-style navigation
					self->NPC->consecutiveBlockedMoves++;
				}
				NPC_SetBlocked( self, info->blocker );
				return qfalse;
			}
			self->NPC->consecutiveBlockedMoves++;
		}
		//See if we're moving along with them
		//if ( NAVNEW_TrueCollision( self, info->blocker, movedir, info->direction ) == qfalse )
		//	return qtrue;

		//Test for blocking by standing on goal
		if ( NAV_TestForBlocked( self, goal, info->blocker, info->distance, &info->flags ) == qtrue )
			return qfalse;

		//If the above function said we're blocked, don't do the extra checks
		/*
		if ( info->flags & NIF_BLOCKED )
			return qtrue;
		*/

		//See if we can get that entity to move out of our way
		if ( NAVNEW_ResolveEntityCollision( self, info->blocker, movedir, info->pathDirection, setBlockedInfo ) == qfalse )
			return qfalse;

		VectorCopy( movedir, info->direction );
		
		return qtrue;
	}
	else
	{
		if ( setBlockedInfo )
		{
			self->NPC->consecutiveBlockedMoves = 0;
		}
	}

	//Our path is clear, just move there
	if ( NAVDEBUG_showCollision )
	{
		G_DrawEdge( self->r.currentOrigin, movepos, EDGE_MOVEDIR );
	}

	return qtrue;
}

qboolean NAVNEW_TestNodeConnectionBlocked( int wp1, int wp2, gentity_t *ignoreEnt, int goalEntNum, qboolean checkWorld, qboolean checkEnts )
{//see if the direct path between 2 nodes is blocked by architecture or an ent
	vec3_t	pos1, pos2, mins, maxs;
	trace_t	trace;
	int		clipmask = MASK_NPCSOLID|CONTENTS_BOTCLIP;
	int ignoreEntNum;
	vec3_t playerMins, playerMaxs;

	if ( !checkWorld && !checkEnts )
	{//duh, nothing to trace against
		return qfalse;
	}
	VectorSet(playerMins, -15, -15, DEFAULT_MINS_2);
	VectorSet(playerMaxs, 15, 15, DEFAULT_MAXS_2);

	trap_Nav_GetNodePosition( wp1, pos1 );
	trap_Nav_GetNodePosition( wp2, pos2 );

	if ( !checkWorld )
	{
		clipmask &= ~(CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP);
	}
	if ( !checkEnts )
	{
		clipmask &= ~CONTENTS_BODY;
	}
	if ( ignoreEnt )
	{
		VectorCopy( ignoreEnt->r.mins, mins );
		VectorCopy( ignoreEnt->r.maxs, maxs );
		ignoreEntNum = ignoreEnt->s.number;
	}
	else
	{
		VectorCopy( playerMins, mins );
		VectorCopy( playerMaxs, mins );
		ignoreEntNum = ENTITYNUM_NONE;
	}
	mins[2] += STEPSIZE;
	//don't let box get inverted
	if ( mins[2] > maxs[2] )
	{	
		mins[2] = maxs[2];
	}

	trap_Trace( &trace, pos1, mins, maxs, pos2, ignoreEntNum, clipmask );
	if ( trace.fraction >= 1.0f || trace.entityNum == goalEntNum )
	{//clear or hit goal
		return qfalse;
	}
	//hit something we weren't supposed to
	return qtrue;
}
/*
-------------------------
NAVNEW_MoveToGoal
-------------------------
*/
int	NAVNEW_MoveToGoal( gentity_t *self, navInfo_t *info )
{
	int			bestNode = WAYPOINT_NONE;
	qboolean	foundClearPath = qfalse;
	vec3_t		origin;
	navInfo_t	tempInfo;
	qboolean	setBlockedInfo = qtrue;
	qboolean	inBestWP, inGoalWP, goalWPFailed = qfalse;
	int			numTries = 0;

	memcpy( &tempInfo, info, sizeof( tempInfo ) );

	//Must have a goal entity to move there
	if( self->NPC->goalEntity == NULL )
		return WAYPOINT_NONE;

	if ( self->waypoint == WAYPOINT_NONE && self->noWaypointTime > level.time )
	{//didn't have a valid one in about the past second, don't look again just yet
		return WAYPOINT_NONE;
	}
	if ( self->NPC->goalEntity->waypoint == WAYPOINT_NONE && self->NPC->goalEntity->noWaypointTime > level.time )
	{//didn't have a valid one in about the past second, don't look again just yet
		return WAYPOINT_NONE;
	}
	if ( self->noWaypointTime > level.time &&
		self->NPC->goalEntity->noWaypointTime > level.time )
	{//just use current waypoints
		bestNode = trap_Nav_GetBestNodeAltRoute2( self->waypoint, self->NPC->goalEntity->waypoint, bestNode );
	}
	//FIXME!!!!: this is making them wiggle back and forth between waypoints
	else if ( (bestNode = trap_Nav_GetBestPathBetweenEnts( self, self->NPC->goalEntity, NF_CLEAR_PATH )) == NODE_NONE )//!NAVNEW_GetWaypoints( self, qtrue ) )
	{//one of us didn't have a valid waypoint!
		if ( self->waypoint == NODE_NONE )
		{//don't even try to find one again for a bit
			self->noWaypointTime = level.time + Q_irand( 500, 1500 );
		}
		if ( self->NPC->goalEntity->waypoint == NODE_NONE )
		{//don't even try to find one again for a bit
			self->NPC->goalEntity->noWaypointTime = level.time + Q_irand( 500, 1500 );
		}
		return WAYPOINT_NONE;
	}
	else
	{
		if ( self->NPC->goalEntity->noWaypointTime < level.time )
		{
			self->NPC->goalEntity->noWaypointTime = level.time + Q_irand( 500, 1500 );
		}
	}

	while( !foundClearPath )
	{
		inBestWP = inGoalWP = qfalse;
		/*
		bestNode = trap_Nav_GetBestNodeAltRoute( self->waypoint, self->NPC->goalEntity->waypoint, bestNode );
		*/

		if ( bestNode == WAYPOINT_NONE )
		{
			goto failed;
		}

		//see if we can get directly to the next node off bestNode en route to goal's node...
		//NOTE: shouldn't be necc. now
		/*
		int oldBestNode = bestNode;
		bestNode = NAV_TestBestNode( self, self->waypoint, bestNode, qtrue );//, self->NPC->goalEntity->waypoint );// 
		//NOTE: Guaranteed to return something
		if ( bestNode != oldBestNode )
		{//we were blocked somehow
			if ( setBlockedInfo )
			{
				self->NPC->aiFlags |= NPCAI_BLOCKED;
				trap_Nav_GetNodePosition( oldBestNode, NPCInfo->blockedDest );
			}
		}
		*/
		trap_Nav_GetNodePosition( bestNode, origin );
		/*
		if ( !goalWPFailed )
		{//we haven't already tried to go straight to goal or goal's wp
			if ( bestNode == self->NPC->goalEntity->waypoint )
			{//our bestNode is the goal's wp
				if ( NAV_HitNavGoal( self->r.currentOrigin, self->r.mins, self->r.maxs, origin, trap_Nav_GetNodeRadius( bestNode ), FlyingCreature( self ) ) )
				{//we're in the goal's wp
					inGoalWP = qtrue;
					//we're in the goalEntity's waypoint already
					//so head for the goalEntity since we know it's clear of architecture
					//FIXME: this is pretty stupid because the NPCs try to go straight
					//		towards their goal before then even try macro_nav...
					VectorCopy( self->NPC->goalEntity->r.currentOrigin, origin );
				}
			}
		}
		*/
		if ( !inGoalWP )
		{//not heading straight for goal
			if ( bestNode == self->waypoint )
			{//we know it's clear or architecture
				//trap_Nav_GetNodePosition( self->waypoint, origin );
				/*
				if ( NAV_HitNavGoal( self->r.currentOrigin, self->r.mins, self->r.maxs, origin, trap_Nav_GetNodeRadius( bestNode ), FlyingCreature( self ) ) )
				{//we're in the wp we're heading for already
					inBestWP = qtrue;
				}
				*/
			}
			else
			{//heading to an edge off our confirmed clear waypoint... make sure it's clear
				//it it's not, bestNode will fall back to our waypoint
				int oldBestNode = bestNode;
				bestNode = NAV_TestBestNode( self, self->waypoint, bestNode, qtrue );
				if ( bestNode == self->waypoint )
				{//we fell back to our waypoint, reset the origin
					self->NPC->aiFlags |= NPCAI_BLOCKED;
					trap_Nav_GetNodePosition( oldBestNode, NPCInfo->blockedDest );
					trap_Nav_GetNodePosition( bestNode, origin );
				}
			}
		}
		//Com_Printf( "goalwp = %d, mywp = %d, node = %d, origin = %s\n", self->NPC->goalEntity->waypoint, self->waypoint, bestNode, vtos(origin) );

		memcpy( &tempInfo, info, sizeof( tempInfo ) );
		VectorSubtract( origin, self->r.currentOrigin, tempInfo.direction );
		VectorNormalize( tempInfo.direction );

		//NOTE: One very important thing NAVNEW_AvoidCollision does is
		//		it actually CHANGES the value of "direction" - it changes it to
		//		whatever dir you need to go in to avoid the obstacle...
		foundClearPath = NAVNEW_AvoidCollision( self, self->NPC->goalEntity, &tempInfo, setBlockedInfo, 5 );

		if ( !foundClearPath )
		{//blocked by an ent
			if ( inGoalWP )
			{//we were heading straight for the goal, head for the goal's wp instead
				trap_Nav_GetNodePosition( bestNode, origin );
				foundClearPath = NAVNEW_AvoidCollision( self, self->NPC->goalEntity, &tempInfo, setBlockedInfo, 5 );
			}
		}

		if ( foundClearPath )
		{//clear!
			//If we got set to blocked, clear it
			NPC_ClearBlocked( self );
			//Take the dir
			memcpy( info, &tempInfo, sizeof( *info ) );
			if ( self->s.weapon == WP_SABER )
			{//jedi
				if ( info->direction[2] * info->distance > 64 )
				{
					self->NPC->aiFlags |= NPCAI_BLOCKED;
					VectorCopy( origin, NPCInfo->blockedDest );
					goto failed;
				}
			}
		}
		else
		{//blocked by ent!
			if ( setBlockedInfo )
			{
				self->NPC->aiFlags |= NPCAI_BLOCKED;
				trap_Nav_GetNodePosition( bestNode, NPCInfo->blockedDest );
			}
			//Only set blocked info first time
			setBlockedInfo = qfalse;

			if ( inGoalWP )
			{//we headed for our goal and failed and our goal's WP and failed
				if ( self->waypoint == self->NPC->goalEntity->waypoint )
				{//our waypoint is our goal's waypoint, nothing we can do
					//remember that this node is blocked
					trap_Nav_AddFailedNode( self, self->waypoint );
					goto failed;
				}
				else
				{//try going for our waypoint this time
					goalWPFailed = qtrue;
					inGoalWP = qfalse;
				}
			}
			else if ( bestNode != self->waypoint )
			{//we headed toward our next waypoint (instead of our waypoint) and failed
				if ( d_altRoutes.integer )
				{//mark this edge failed and try our waypoint
					//NOTE: don't assume there is something blocking the direct path
					//			between my waypoint and the bestNode... I could be off
					//			that path because of collision avoidance...
					if ( d_patched.integer &&//use patch-style navigation
 						( !trap_Nav_NodesAreNeighbors( self->waypoint, bestNode )
						|| NAVNEW_TestNodeConnectionBlocked( self->waypoint, bestNode, self, self->NPC->goalEntity->s.number, qfalse, qtrue ) ) )
					{//the direct path between these 2 nodes is blocked by an ent
						trap_Nav_AddFailedEdge( self->s.number, self->waypoint, bestNode );
					}
					bestNode = self->waypoint;
				}
				else
				{
					//we should stop
					goto failed;
				}
			}
			else 
			{//we headed for *our* waypoint and couldn't get to it
				if ( d_altRoutes.integer )
				{
					//remember that this node is blocked
					trap_Nav_AddFailedNode( self, self->waypoint );
					//Now we should get our waypoints again
					//FIXME: cache the trace-data for subsequent calls as only the route info would have changed
					//if ( (bestNode = trap_Nav_GetBestPathBetweenEnts( self, self->NPC->goalEntity, NF_CLEAR_PATH )) == NODE_NONE )//!NAVNEW_GetWaypoints( self, qfalse ) )
					{//one of our waypoints is WAYPOINT_NONE now
						goto failed;
					}
				}
				else
				{
					//we should stop
					goto failed;
				}
			}

			if ( ++numTries >= 10 )
			{
				goto failed;
			}
		}
	}

//finish:
	//Draw any debug info, if requested
	if ( NAVDEBUG_showEnemyPath )
	{
		vec3_t	dest, start;

		//Get the positions
		trap_Nav_GetNodePosition( self->NPC->goalEntity->waypoint, dest );
		trap_Nav_GetNodePosition( bestNode, start );

		//Draw the route
		G_DrawNode( start, NODE_START );
		if ( bestNode != self->waypoint )
		{
			vec3_t	wpPos;
			trap_Nav_GetNodePosition( self->waypoint, wpPos );
			G_DrawNode( wpPos, NODE_NAVGOAL );
		}
		G_DrawNode( dest, NODE_GOAL );
		G_DrawEdge( dest, self->NPC->goalEntity->r.currentOrigin, EDGE_PATH );
		G_DrawNode( self->NPC->goalEntity->r.currentOrigin, NODE_GOAL );
		trap_Nav_ShowPath( bestNode, self->NPC->goalEntity->waypoint );
	}

	self->NPC->shoveCount = 0;

	//let me keep this waypoint for a while
	if ( self->noWaypointTime < level.time )
	{
		self->noWaypointTime = level.time + Q_irand( 500, 1500 );
	}
	return bestNode;

failed:
	//FIXME: What we should really do here is have a list of the goal's and our
	//		closest clearpath waypoints, ranked.  If the first set fails, try the rest
	//		until there are no alternatives.

	trap_Nav_GetNodePosition( self->waypoint, origin );

	//do this to avoid ping-ponging?
	return WAYPOINT_NONE;
	/*
	//this was causing ping-ponging
	if ( DistanceSquared( origin, self->r.currentOrigin ) < 16 )//woo, magic number
	{//We're right up on our waypoint, so that won't help, return none
		//Or maybe find the nextbest here?
		return WAYPOINT_NONE;
	}
	else
	{//Try going to our waypoint
		bestNode = self->waypoint;

		VectorSubtract( origin, self->r.currentOrigin, info.direction );
		VectorNormalize( info.direction );
	}
	
	goto finish;
	*/
}
