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

// These utilities are meant for strictly non-player, non-team NPCs.
// These functions are in their own file because they are only intended
// for use with NPCs who's logic has been overriden from the original
// AI code, and who's code resides in files with the AI_ prefix.

#include "b_local.h"
#include "g_nav.h"

#define	MAX_RADIUS_ENTS		128
#define	DEFAULT_RADIUS		45

qboolean AI_ValidateGroupMember( AIGroupInfo_t *group, gentity_t *member );

extern void G_TestLine(vec3_t start, vec3_t end, int color, int time);

/*
-------------------------
AI_GetGroupSize
-------------------------
*/

int	AI_GetGroupSize( vec3_t origin, int radius, team_t playerTeam, gentity_t *avoid )
{
	int			radiusEnts[ MAX_RADIUS_ENTS ];
	gentity_t	*check;
	vec3_t		mins, maxs;
	int			numEnts, realCount = 0;
	int			i;
	int			j;

	//Setup the bbox to search in
	for ( i = 0; i < 3; i++ )
	{
		mins[i] = origin[i] - radius;
		maxs[i] = origin[i] + radius;
	}

	//Get the number of entities in a given space
	numEnts = trap->EntitiesInBox( mins, maxs, radiusEnts, MAX_RADIUS_ENTS );

	//Cull this list
	for ( j = 0; j < numEnts; j++ )
	{
		check = &g_entities[radiusEnts[j]];

		//Validate clients
		if ( check->client == NULL )
			continue;

		//Skip the requested avoid ent if present
		if ( ( avoid != NULL ) && ( check == avoid ) )
			continue;

		//Must be on the same team
		if ( check->client->playerTeam != (npcteam_t)playerTeam )
			continue;

		//Must be alive
		if ( check->health <= 0 )
			continue;

		realCount++;
	}

	return realCount;
}

//Overload

int AI_GetGroupSize2( gentity_t *ent, int radius )
{
	if ( ( ent == NULL ) || ( ent->client == NULL ) )
		return -1;

	return AI_GetGroupSize( ent->r.currentOrigin, radius, (team_t)ent->client->playerTeam, ent );
}

void AI_SetClosestBuddy( AIGroupInfo_t *group )
{
	int	i, j;
	int	dist, bestDist;

	for ( i = 0; i < group->numGroup; i++ )
	{
		group->member[i].closestBuddy = ENTITYNUM_NONE;

		bestDist = Q3_INFINITE;
		for ( j = 0; j < group->numGroup; j++ )
		{
			dist = DistanceSquared( g_entities[group->member[i].number].r.currentOrigin, g_entities[group->member[j].number].r.currentOrigin );
			if ( dist < bestDist )
			{
				bestDist = dist;
				group->member[i].closestBuddy = group->member[j].number;
			}
		}
	}
}

void AI_SortGroupByPathCostToEnemy( AIGroupInfo_t *group )
{
	AIGroupMember_t bestMembers[MAX_GROUP_MEMBERS];
	int				i, j, k;
	qboolean		sort = qfalse;

	if ( group->enemy != NULL )
	{//FIXME: just use enemy->waypoint?
		group->enemyWP = NAV_FindClosestWaypointForEnt( group->enemy, WAYPOINT_NONE );
	}
	else
	{
		group->enemyWP = WAYPOINT_NONE;
	}

	for ( i = 0; i < group->numGroup; i++ )
	{
		if ( group->enemyWP == WAYPOINT_NONE )
		{//FIXME: just use member->waypoint?
			group->member[i].waypoint = WAYPOINT_NONE;
			group->member[i].pathCostToEnemy = Q3_INFINITE;
		}
		else
		{//FIXME: just use member->waypoint?
			group->member[i].waypoint = NAV_FindClosestWaypointForEnt( group->enemy, WAYPOINT_NONE );
			if ( group->member[i].waypoint != WAYPOINT_NONE )
			{
				group->member[i].pathCostToEnemy = trap->Nav_GetPathCost( group->member[i].waypoint, group->enemyWP );
				//at least one of us has a path, so do sorting
				sort = qtrue;
			}
			else
			{
				group->member[i].pathCostToEnemy = Q3_INFINITE;
			}
		}
	}
	//Now sort
	if ( sort )
	{
		//initialize bestMembers data
		for ( j = 0; j < group->numGroup; j++ )
		{
			bestMembers[j].number = ENTITYNUM_NONE;
		}

		for ( i = 0; i < group->numGroup; i++ )
		{
			for ( j = 0; j < group->numGroup; j++ )
			{
				if ( bestMembers[j].number != ENTITYNUM_NONE )
				{//slot occupied
					if ( group->member[i].pathCostToEnemy < bestMembers[j].pathCostToEnemy )
					{//this guy has a shorter path than the one currenly in this spot, bump him and put myself in here
						for ( k = group->numGroup; k < j; k++ )
						{
							memcpy( &bestMembers[k], &bestMembers[k-1], sizeof( bestMembers[k] ) );
						}
						memcpy( &bestMembers[j], &group->member[i], sizeof( bestMembers[j] ) );
						break;
					}
				}
				else
				{//slot unoccupied, reached end of list, throw self in here
					memcpy( &bestMembers[j], &group->member[i], sizeof( bestMembers[j] ) );
					break;
				}
			}
		}

		//Okay, now bestMembers is a sorted list, just copy it into group->members
		for ( i = 0; i < group->numGroup; i++ )
		{
			memcpy( &group->member[i], &bestMembers[i], sizeof( group->member[i] ) );
		}
	}
}

qboolean AI_FindSelfInPreviousGroup( gentity_t *self )
{//go through other groups made this frame and see if any of those contain me already
	int	i, j;
	for ( i = 0; i < MAX_FRAME_GROUPS; i++ )
	{
		if ( level.groups[i].numGroup )//&& level.groups[i].enemy != NULL )
		{//check this one
			for ( j = 0; j < level.groups[i].numGroup; j++ )
			{
				if ( level.groups[i].member[j].number == self->s.number )
				{
					self->NPC->group = &level.groups[i];
					return qtrue;
				}
			}
		}
	}
	return qfalse;
}

void AI_InsertGroupMember( AIGroupInfo_t *group, gentity_t *member )
{
	int i;

	//okay, you know what?  Check this damn group and make sure we're not already in here!
	for ( i = 0; i < group->numGroup; i++ )
	{
		if ( group->member[i].number == member->s.number )
		{//already in here
			break;
		}
	}
	if ( i < group->numGroup )
	{//found him in group already
	}
	else
	{//add him in
		group->member[group->numGroup++].number = member->s.number;
		group->numState[member->NPC->squadState]++;
	}
	if ( !group->commander || (member->NPC->rank > group->commander->NPC->rank) )
	{//keep track of highest rank
		group->commander = member;
	}
	member->NPC->group = group;
}

qboolean AI_TryJoinPreviousGroup( gentity_t *self )
{//go through other groups made this frame and see if any of those have the same enemy as me... if so, add me in!
	int	i;
	for ( i = 0; i < MAX_FRAME_GROUPS; i++ )
	{
		if ( level.groups[i].numGroup
			&& level.groups[i].numGroup < (MAX_GROUP_MEMBERS - 1)
			//&& level.groups[i].enemy != NULL
			&& level.groups[i].enemy == self->enemy )
		{//has members, not full and has my enemy
			if ( AI_ValidateGroupMember( &level.groups[i], self ) )
			{//I am a valid member for this group
				AI_InsertGroupMember( &level.groups[i], self );
				return qtrue;
			}
		}
	}
	return qfalse;
}

qboolean AI_GetNextEmptyGroup( gentity_t *self )
{
	int i;

	if ( AI_FindSelfInPreviousGroup( self ) )
	{//already in one, no need to make a new one
		return qfalse;
	}

	if ( AI_TryJoinPreviousGroup( self ) )
	{//try to just put us in one that already exists
		return qfalse;
	}

	//okay, make a whole new one, then
	for ( i = 0; i < MAX_FRAME_GROUPS; i++ )
	{
		if ( !level.groups[i].numGroup )
		{//make a new one
			self->NPC->group = &level.groups[i];
			return qtrue;
		}
	}

	//if ( i >= MAX_FRAME_GROUPS )
	{//WTF?  Out of groups!
		self->NPC->group = NULL;
		return qfalse;
	}
}

qboolean AI_ValidateNoEnemyGroupMember( AIGroupInfo_t *group, gentity_t *member )
{
	vec3_t center;

	if ( !group )
	{
		return qfalse;
	}
	if ( group->commander )
	{
		VectorCopy( group->commander->r.currentOrigin, center );
	}
	else
	{//hmm, just pick the first member
		if ( group->member[0].number < 0 || group->member[0].number >= ENTITYNUM_WORLD )
		{
			return qfalse;
		}
		VectorCopy( g_entities[group->member[0].number].r.currentOrigin, center );
	}
	//FIXME: maybe it should be based on the center of the mass of the group, not the commander?
	if ( DistanceSquared( center, member->r.currentOrigin ) > 147456/*384*384*/ )
	{
		return qfalse;
	}
	if ( !trap->InPVS( member->r.currentOrigin, center ) )
	{//not within PVS of the group enemy
		return qfalse;
	}
	return qtrue;
}

qboolean AI_ValidateGroupMember( AIGroupInfo_t *group, gentity_t *member )
{
	//Validate ents
	if ( member == NULL )
		return qfalse;

	//Validate clients
	if ( member->client == NULL )
		return qfalse;

	//Validate NPCs
	if ( member->NPC == NULL )
		return qfalse;

	//must be aware
	if ( member->NPC->confusionTime > level.time )
		return qfalse;

	//must be allowed to join groups
	if ( member->NPC->scriptFlags&SCF_NO_GROUPS )
		return qfalse;

	//Must not be in another group
	if ( member->NPC->group != NULL && member->NPC->group != group )
	{//FIXME: if that group's enemy is mine, why not absorb that group into mine?
		return qfalse;
	}

	//Must be alive
	if ( member->health <= 0 )
		return qfalse;

	//can't be in an emplaced gun
//	if( member->s.eFlags & EF_LOCKED_TO_WEAPON )
//		return qfalse;
	//rwwFIXMEFIXME: support this flag

	//Must be on the same team
	if ( member->client->playerTeam != (npcteam_t)group->team )
		return qfalse;

	if ( member->client->ps.weapon == WP_SABER ||//!= self->s.weapon )
		member->client->ps.weapon == WP_THERMAL ||
		member->client->ps.weapon == WP_DISRUPTOR ||
		member->client->ps.weapon == WP_EMPLACED_GUN ||
//		member->client->ps.weapon == WP_BOT_LASER ||		// Probe droid	- Laser blast
		member->client->ps.weapon == WP_STUN_BATON ||
		member->client->ps.weapon == WP_TURRET /*||			// turret guns
		member->client->ps.weapon == WP_ATST_MAIN ||
		member->client->ps.weapon == WP_ATST_SIDE ||
		member->client->ps.weapon == WP_TIE_FIGHTER*/ )
	{//not really a squad-type guy
		return qfalse;
	}

	if ( member->client->NPC_class == CLASS_ATST ||
		member->client->NPC_class == CLASS_PROBE ||
		member->client->NPC_class == CLASS_SEEKER ||
		member->client->NPC_class == CLASS_REMOTE ||
		member->client->NPC_class == CLASS_SENTRY ||
		member->client->NPC_class == CLASS_INTERROGATOR ||
		member->client->NPC_class == CLASS_MINEMONSTER ||
		member->client->NPC_class == CLASS_HOWLER ||
		member->client->NPC_class == CLASS_MARK1 ||
		member->client->NPC_class == CLASS_MARK2 )
	{//these kinds of enemies don't actually use this group AI
		return qfalse;
	}

	//should have same enemy
	if ( member->enemy != group->enemy )
	{
		if ( member->enemy != NULL )
		{//he's fighting someone else, leave him out
			return qfalse;
		}
		if ( !trap->InPVS( member->r.currentOrigin, group->enemy->r.currentOrigin ) )
		{//not within PVS of the group enemy
			return qfalse;
		}
	}
	else if ( group->enemy == NULL )
	{//if the group is a patrol group, only take those within the room and radius
		if ( !AI_ValidateNoEnemyGroupMember( group, member ) )
		{
			return qfalse;
		}
	}
	//must be actually in combat mode
	if ( !TIMER_Done( member, "interrogating" ) )
		return qfalse;
	//FIXME: need to have a route to enemy and/or clear shot?
	return qtrue;
}

/*
-------------------------
AI_GetGroup
-------------------------
*/
void AI_GetGroup( gentity_t *self )
{
	int	i;
	gentity_t	*member;//, *waiter;
	//int	waiters[MAX_WAITERS];

	if ( !self || !self->NPC )
	{
		return;
	}

	if ( d_noGroupAI.integer )
	{
		self->NPC->group = NULL;
		return;
	}

	if ( !self->client )
	{
		self->NPC->group = NULL;
		return;
	}

	if ( self->NPC->scriptFlags&SCF_NO_GROUPS )
	{
		self->NPC->group = NULL;
		return;
	}

	if ( self->enemy && (!self->enemy->client || (level.time - self->NPC->enemyLastSeenTime > 7000 )))
	{
		self->NPC->group = NULL;
		return;
	}

	if ( !AI_GetNextEmptyGroup( self ) )
	{//either no more groups left or we're already in a group built earlier
		return;
	}

	//create a new one
	memset( self->NPC->group, 0, sizeof( AIGroupInfo_t ) );

	self->NPC->group->enemy = self->enemy;
	self->NPC->group->team = (team_t)self->client->playerTeam;
	self->NPC->group->processed = qfalse;
	self->NPC->group->commander = self;
	self->NPC->group->memberValidateTime = level.time + 2000;
	self->NPC->group->activeMemberNum = 0;

	if ( self->NPC->group->enemy )
	{
		self->NPC->group->lastSeenEnemyTime = level.time;
		self->NPC->group->lastClearShotTime = level.time;
		VectorCopy( self->NPC->group->enemy->r.currentOrigin, self->NPC->group->enemyLastSeenPos );
	}

//	for ( i = 0, member = &g_entities[0]; i < globals.num_entities ; i++, member++)
	for ( i = 0; i < level.num_entities ; i++)
	{
		member = &g_entities[i];

		if (!member->inuse)
		{
			continue;
		}

		if ( !AI_ValidateGroupMember( self->NPC->group, member ) )
		{//FIXME: keep track of those who aren't angry yet and see if we should wake them after we assemble the core group
			continue;
		}

		//store it
		AI_InsertGroupMember( self->NPC->group, member );

		if ( self->NPC->group->numGroup >= (MAX_GROUP_MEMBERS - 1) )
		{//full
			break;
		}
	}

	/*
	//now go through waiters and see if any should join the group
	//NOTE:  Some should hang back and probably not attack, so we can ambush
	//NOTE: only do this if calling for reinforcements?
	for ( i = 0; i < numWaiters; i++ )
	{
		waiter = &g_entities[waiters[i]];

		for ( j = 0; j < self->NPC->group->numGroup; j++ )
		{
			member = &g_entities[self->NPC->group->member[j];

			if ( trap->InPVS( waiter->r.currentOrigin, member->r.currentOrigin ) )
			{//this waiter is within PVS of a current member
			}
		}
	}
	*/

	if ( self->NPC->group->numGroup <= 0 )
	{//none in group
		self->NPC->group = NULL;
		return;
	}

	AI_SortGroupByPathCostToEnemy( self->NPC->group );
	AI_SetClosestBuddy( self->NPC->group );
}

void AI_SetNewGroupCommander( AIGroupInfo_t *group )
{
	gentity_t *member = NULL;
	int i;

	group->commander = NULL;
	for ( i = 0; i < group->numGroup; i++ )
	{
		member = &g_entities[group->member[i].number];

		if ( !group->commander || (member && member->NPC && group->commander->NPC && member->NPC->rank > group->commander->NPC->rank) )
		{//keep track of highest rank
			group->commander = member;
		}
	}
}

void AI_DeleteGroupMember( AIGroupInfo_t *group, int memberNum )
{
	int i;

	if ( group->commander && group->commander->s.number == group->member[memberNum].number )
	{
		group->commander = NULL;
	}
	if ( g_entities[group->member[memberNum].number].NPC )
	{
		g_entities[group->member[memberNum].number].NPC->group = NULL;
	}
	for ( i = memberNum; i < (group->numGroup-1); i++ )
	{
		memcpy( &group->member[i], &group->member[i+1], sizeof( group->member[i] ) );
	}
	if ( memberNum < group->activeMemberNum )
	{
		group->activeMemberNum--;
		if ( group->activeMemberNum < 0 )
		{
			group->activeMemberNum = 0;
		}
	}
	group->numGroup--;
	if ( group->numGroup < 0 )
	{
		group->numGroup = 0;
	}
	AI_SetNewGroupCommander( group );
}

void AI_DeleteSelfFromGroup( gentity_t *self )
{
	int i;

	//FIXME: if killed, keep track of how many in group killed?  To affect morale?
	for ( i = 0; i < self->NPC->group->numGroup; i++ )
	{
		if ( self->NPC->group->member[i].number == self->s.number )
		{
			AI_DeleteGroupMember( self->NPC->group, i );
			return;
		}
	}
}

extern void ST_AggressionAdjust( gentity_t *self, int change );
extern void ST_MarkToCover( gentity_t *self );
extern void ST_StartFlee( gentity_t *self, gentity_t *enemy, vec3_t dangerPoint, int dangerLevel, int minTime, int maxTime );
void AI_GroupMemberKilled( gentity_t *self )
{
	AIGroupInfo_t *group = self->NPC->group;
	gentity_t	*member;
	qboolean	noflee = qfalse;
	int			i;

	if ( !group )
	{//what group?
		return;
	}
	if ( !self || !self->NPC || self->NPC->rank < RANK_ENSIGN )
	{//I'm not an officer, let's not really care for now
		return;
	}
	//temporarily drop group morale for a few seconds
	group->moraleAdjust -= self->NPC->rank;
	//go through and drop aggression on my teammates (more cover, worse aim)
	for ( i = 0; i < group->numGroup; i++ )
	{
		member = &g_entities[group->member[i].number];
		if ( member == self )
		{
			continue;
		}
		if ( member->NPC->rank > RANK_ENSIGN )
		{//officers do not panic
			noflee = qtrue;
		}
		else
		{
			ST_AggressionAdjust( member, -1 );
			member->NPC->currentAim -= Q_irand( 0, 10 );//Q_irand( 0, 2);//drop their aim accuracy
		}
	}
	//okay, if I'm the group commander, make everyone else flee
	if ( group->commander != self )
	{//I'm not the commander... hmm, should maybe a couple flee... maybe those near me?
		return;
	}
	//now see if there is another of sufficient rank to keep them from fleeing
	if ( !noflee )
	{
		self->NPC->group->speechDebounceTime = 0;
		for ( i = 0; i < group->numGroup; i++ )
		{
			member = &g_entities[group->member[i].number];
			if ( member == self )
			{
				continue;
			}
			if ( member->NPC->rank < RANK_ENSIGN )
			{//grunt
				if ( group->enemy && DistanceSquared( member->r.currentOrigin, group->enemy->r.currentOrigin ) < 65536/*256*256*/ )
				{//those close to enemy run away!
					ST_StartFlee( member, group->enemy, member->r.currentOrigin, AEL_DANGER_GREAT, 3000, 5000 );
				}
				else if ( DistanceSquared( member->r.currentOrigin, self->r.currentOrigin ) < 65536/*256*256*/ )
				{//those close to me run away!
					ST_StartFlee( member, group->enemy, member->r.currentOrigin, AEL_DANGER_GREAT, 3000, 5000 );
				}
				else
				{//else, maybe just a random chance
					if ( Q_irand( 0, self->NPC->rank ) > member->NPC->rank )
					{//lower rank they are, higher rank I am, more likely they are to flee
						ST_StartFlee( member, group->enemy, member->r.currentOrigin, AEL_DANGER_GREAT, 3000, 5000 );
					}
					else
					{
						ST_MarkToCover( member );
					}
				}
				member->NPC->currentAim -= Q_irand( 1, 15 ); //Q_irand( 1, 3 );//drop their aim accuracy even more
			}
			member->NPC->currentAim -= Q_irand( 1, 15 ); //Q_irand( 1, 3 );//drop their aim accuracy even more
		}
	}
}

void AI_GroupUpdateEnemyLastSeen( AIGroupInfo_t *group, vec3_t spot )
{
	if ( !group )
	{
		return;
	}
	group->lastSeenEnemyTime = level.time;
	VectorCopy( spot, group->enemyLastSeenPos );
}

void AI_GroupUpdateClearShotTime( AIGroupInfo_t *group )
{
	if ( !group )
	{
		return;
	}
	group->lastClearShotTime = level.time;
}

void AI_GroupUpdateSquadstates( AIGroupInfo_t *group, gentity_t *member, int newSquadState )
{
	int i;

	if ( !group )
	{
		member->NPC->squadState = newSquadState;
		return;
	}

	for ( i = 0; i < group->numGroup; i++ )
	{
		if ( group->member[i].number == member->s.number )
		{
			group->numState[member->NPC->squadState]--;
			member->NPC->squadState = newSquadState;
			group->numState[member->NPC->squadState]++;
			return;
		}
	}
}

qboolean AI_RefreshGroup( AIGroupInfo_t *group )
{
	gentity_t	*member;
	int			i;//, j;

	//see if we should merge with another group
	for ( i = 0; i < MAX_FRAME_GROUPS; i++ )
	{
		if ( &level.groups[i] == group )
		{
			break;
		}
		else
		{
			if ( level.groups[i].enemy == group->enemy )
			{//2 groups with same enemy
				if ( level.groups[i].numGroup+group->numGroup < (MAX_GROUP_MEMBERS - 1) )
				{//combining the members would fit in one group
					qboolean deleteWhenDone = qtrue;
					int j;

					//combine the members of mine into theirs
					for ( j = 0; j < group->numGroup; j++ )
					{
						member = &g_entities[group->member[j].number];
						if ( level.groups[i].enemy == NULL )
						{//special case for groups without enemies, must be in range
							if ( !AI_ValidateNoEnemyGroupMember( &level.groups[i], member ) )
							{
								deleteWhenDone = qfalse;
								continue;
							}
						}
						//remove this member from this group
						AI_DeleteGroupMember( group, j );
						//keep marker at same place since we deleted this guy and shifted everyone up one
						j--;
						//add them to the earlier group
						AI_InsertGroupMember( &level.groups[i], member );
					}
					//return and delete this group
					if ( deleteWhenDone )
					{
						return qfalse;
					}
				}
			}
		}
	}
	//clear numStates
	for ( i = 0; i < NUM_SQUAD_STATES; i++ )
	{
		group->numState[i] = 0;
	}

	//go through group and validate each membership
	group->commander = NULL;
	for ( i = 0; i < group->numGroup; i++ )
	{
		/*
		//this checks for duplicate copies of one member in a group
		for ( j = 0; j < group->numGroup; j++ )
		{
			if ( i != j )
			{
				if ( group->member[i].number == group->member[j].number )
				{
					break;
				}
			}
		}
		if ( j < group->numGroup )
		{//found a dupe!
			trap->Printf( S_COLOR_RED"ERROR: member %s(%d) a duplicate group member!!!\n", g_entities[group->member[i].number].targetname, group->member[i].number );
			AI_DeleteGroupMember( group, i );
			i--;
			continue;
		}
		*/
		member = &g_entities[group->member[i].number];

		//Must be alive
		if ( member->health <= 0 )
		{
			AI_DeleteGroupMember( group, i );
			//keep marker at same place since we deleted this guy and shifted everyone up one
			i--;
		}
		else if ( group->memberValidateTime < level.time && !AI_ValidateGroupMember( group, member ) )
		{
			//remove this one from the group
			AI_DeleteGroupMember( group, i );
			//keep marker at same place since we deleted this guy and shifted everyone up one
			i--;
		}
		else
		{//membership is valid
			//keep track of squadStates
			group->numState[member->NPC->squadState]++;
			if ( !group->commander || member->NPC->rank > group->commander->NPC->rank )
			{//keep track of highest rank
				group->commander = member;
			}
		}
	}
	if ( group->memberValidateTime < level.time )
	{
		group->memberValidateTime = level.time + Q_irand( 500, 2500 );
	}
	//Now add any new guys as long as we're not full
	/*
	for ( i = 0, member = &g_entities[0]; i < globals.num_entities && group->numGroup < (MAX_GROUP_MEMBERS - 1); i++, member++)
	{
		if ( !AI_ValidateGroupMember( group, member ) )
		{//FIXME: keep track of those who aren't angry yet and see if we should wake them after we assemble the core group
			continue;
		}
		if ( member->NPC->group == group )
		{//DOH, already in our group
			continue;
		}

		//store it
		AI_InsertGroupMember( group, member );
	}
	*/

	//calc the morale of this group
	group->morale = group->moraleAdjust;
	for ( i = 0; i < group->numGroup; i++ )
	{
		member = &g_entities[group->member[i].number];
		if ( member->NPC->rank < RANK_ENSIGN )
		{//grunts
			group->morale++;
		}
		else
		{
			group->morale += member->NPC->rank;
		}
		if ( group->commander && d_npcai.integer )
		{
			//G_DebugLine( group->commander->r.currentOrigin, member->r.currentOrigin, FRAMETIME, 0x00ff00ff, qtrue );
			G_TestLine(group->commander->r.currentOrigin, member->r.currentOrigin, 0x00000ff, FRAMETIME);
		}
	}
	if ( group->enemy )
	{//modify morale based on enemy health and weapon
		if ( group->enemy->health < 10 )
		{
			group->morale += 10;
		}
		else if ( group->enemy->health < 25 )
		{
			group->morale += 5;
		}
		else if ( group->enemy->health < 50 )
		{
			group->morale += 2;
		}
		switch( group->enemy->s.weapon )
		{
		case WP_SABER:
			group->morale -= 5;
			break;
		case WP_BRYAR_PISTOL:
			group->morale += 3;
			break;
		case WP_DISRUPTOR:
			group->morale += 2;
			break;
		case WP_REPEATER:
			group->morale -= 1;
			break;
		case WP_FLECHETTE:
			group->morale -= 2;
			break;
		case WP_ROCKET_LAUNCHER:
			group->morale -= 10;
			break;
		case WP_THERMAL:
			group->morale -= 5;
			break;
		case WP_TRIP_MINE:
			group->morale -= 3;
			break;
		case WP_DET_PACK:
			group->morale -= 10;
			break;
//		case WP_MELEE:			// Any ol' melee attack
//			group->morale += 20;
//			break;
		case WP_STUN_BATON:
			group->morale += 10;
			break;
		case WP_EMPLACED_GUN:
			group->morale -= 8;
			break;
//		case WP_ATST_MAIN:
//			group->morale -= 8;
//			break;
//		case WP_ATST_SIDE:
//			group->morale -= 20;
//			break;
		}
	}
	if ( group->moraleDebounce < level.time )
	{//slowly degrade whatever moraleAdjusters we may have
		if ( group->moraleAdjust > 0 )
		{
			group->moraleAdjust--;
		}
		else if ( group->moraleAdjust < 0 )
		{
			group->moraleAdjust++;
		}
		group->moraleDebounce = level.time + 1000;//FIXME: define?
	}
	//mark this group as not having been run this frame
	group->processed = qfalse;

	return (group->numGroup>0);
}

void AI_UpdateGroups( void )
{
	int i;

	if ( d_noGroupAI.integer )
	{
		return;
	}
	//Clear all Groups
	for ( i = 0; i < MAX_FRAME_GROUPS; i++ )
	{
		if ( !level.groups[i].numGroup || AI_RefreshGroup( &level.groups[i] ) == qfalse )//level.groups[i].enemy == NULL ||
		{
			memset( &level.groups[i], 0, sizeof( level.groups[i] ) );
		}
	}
}

qboolean AI_GroupContainsEntNum( AIGroupInfo_t *group, int entNum )
{
	int i;

	if ( !group )
	{
		return qfalse;
	}
	for ( i = 0; i < group->numGroup; i++ )
	{
		if ( group->member[i].number == entNum )
		{
			return qtrue;
		}
	}
	return qfalse;
}
//Overload

/*
void AI_GetGroup( AIGroupInfo_t &group, gentity_t *ent, int radius )
{
	if ( ent->client == NULL )
		return;

	vec3_t	temp, angles;

	//FIXME: This is specialized code.. move?
	if ( ent->enemy )
	{
		VectorSubtract( ent->enemy->r.currentOrigin, ent->r.currentOrigin, temp );
		VectorNormalize( temp );	//FIXME: Needed?
		vectoangles( temp, angles );
	}
	else
	{
		VectorCopy( ent->currentAngles, angles );
	}

	AI_GetGroup( group, ent->r.currentOrigin, ent->currentAngles, DEFAULT_RADIUS, radius, ent->client->playerTeam, ent, ent->enemy );
}
*/
/*
-------------------------
AI_CheckEnemyCollision
-------------------------
*/

qboolean AI_CheckEnemyCollision( gentity_t *ent, qboolean takeEnemy )
{
	navInfo_t	info;

	if ( ent == NULL )
		return qfalse;

//	if ( ent->svFlags & SVF_LOCKEDENEMY )
//		return qfalse;

	NAV_GetLastMove( &info );

	//See if we've hit something
	if ( ( info.blocker ) && ( info.blocker != ent->enemy ) )
	{
		if ( ( info.blocker->client ) && ( info.blocker->client->playerTeam == ent->client->enemyTeam ) )
		{
			if ( takeEnemy )
				G_SetEnemy( ent, info.blocker );

			return qtrue;
		}
	}

	return qfalse;
}

/*
-------------------------
AI_DistributeAttack
-------------------------
*/

#define	MAX_RADIUS_ENTS		128

gentity_t *AI_DistributeAttack( gentity_t *attacker, gentity_t *enemy, team_t team, int threshold )
{
	int			radiusEnts[ MAX_RADIUS_ENTS ];
	gentity_t	*check;
	int			numEnts;
	int			numSurrounding;
	int			i;
	int			j;
	vec3_t		mins, maxs;

	//Don't take new targets
//	if ( NPC->svFlags & SVF_LOCKEDENEMY )
//		return enemy;

	numSurrounding = AI_GetGroupSize( enemy->r.currentOrigin, 48, team, attacker );

	//First, see if we should look for the player
	if ( enemy != &g_entities[0] )
	{
		//rwwFIXMEFIXME: care about all clients not just 0
		int	aroundPlayer = AI_GetGroupSize( g_entities[0].r.currentOrigin, 48, team, attacker );

		//See if we're above our threshold
		if ( aroundPlayer < threshold )
		{
			return &g_entities[0];
		}
	}

	//See if our current enemy is still ok
	if ( numSurrounding < threshold )
		return enemy;

	//Otherwise we need to take a new enemy if possible

	//Setup the bbox to search in
	for ( i = 0; i < 3; i++ )
	{
		mins[i] = enemy->r.currentOrigin[i] - 512;
		maxs[i] = enemy->r.currentOrigin[i] + 512;
	}

	//Get the number of entities in a given space
	numEnts = trap->EntitiesInBox( mins, maxs, radiusEnts, MAX_RADIUS_ENTS );

	//Cull this list
	for ( j = 0; j < numEnts; j++ )
	{
		check = &g_entities[radiusEnts[j]];

		//Validate clients
		if ( check->client == NULL )
			continue;

		//Skip the requested avoid ent if present
		if ( check == enemy )
			continue;

		//Must be on the same team
		if ( check->client->playerTeam != enemy->client->playerTeam )
			continue;

		//Must be alive
		if ( check->health <= 0 )
			continue;

		//Must not be overwhelmed
		if ( AI_GetGroupSize( check->r.currentOrigin, 48, team, attacker ) > threshold )
			continue;

		return check;
	}

	return NULL;
}
