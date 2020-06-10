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

#ifndef __AI__
#define __AI__

//Distance ratings
enum distance_e
{
	DIST_MELEE,
	DIST_LONG,
};

//Attack types
enum attack_e
{
	ATTACK_MELEE,
	ATTACK_RANGE,
};

enum
{
	SQUAD_IDLE,					//No target found, waiting
	SQUAD_STAND_AND_SHOOT,		//Standing in position and shoot (no cover)
	SQUAD_RETREAT,				//Running away from combat
	SQUAD_COVER,				//Under protective cover
	SQUAD_TRANSITION,			//Moving between points, not firing
	SQUAD_POINT,				//On point, laying down suppressive fire
	SQUAD_SCOUT,				//Poking out to draw enemy
	NUM_SQUAD_STATES,
};

//sigh... had to move in here for groupInfo
typedef enum //# rank_e
{
	RANK_CIVILIAN,
	RANK_CREWMAN,
	RANK_ENSIGN,
	RANK_LT_JG,
	RANK_LT,
	RANK_LT_COMM,
	RANK_COMMANDER,
	RANK_CAPTAIN
} rank_t;

qboolean NPC_CheckPlayerTeamStealth( void );

//AI_GRENADIER
void NPC_BSGrenadier_Default( void );

//AI_SNIPER
void NPC_BSSniper_Default( void );

//AI_STORMTROOPER
void NPC_BSST_Investigate( void );
void NPC_BSST_Default( void );
void NPC_BSST_Sleep( void );

//AI_JEDI
void NPC_BSJedi_Investigate( void );
void NPC_BSJedi_Default( void );
void NPC_BSJedi_FollowLeader( void );

// AI_DROID
void NPC_BSDroid_Default( void );

// AI_ImperialProbe
void NPC_BSImperialProbe_Default( void );

// AI_atst
void NPC_BSATST_Default( void );

void NPC_BSInterrogator_Default( void );

// AI Mark 1
void NPC_BSMark1_Default( void );

// AI Mark 2
void NPC_BSMark2_Default( void );


void NPC_BSMineMonster_Default( void );
void NPC_BSHowler_Default( void );

//Utilities
//Group AI
#define	MAX_FRAME_GROUPS	32
// !!!!!!!!!! LOADSAVE-affecting structure !!!!!!!!!!
class AIGroupMember_t
{
public:
	int	number;
	int waypoint;
	int pathCostToEnemy;
	int	closestBuddy;


	void sg_export(
		ojk::SavedGameHelper& saved_game) const
	{
		saved_game.write<int32_t>(number);
		saved_game.write<int32_t>(waypoint);
		saved_game.write<int32_t>(pathCostToEnemy);
		saved_game.write<int32_t>(closestBuddy);
	}

	void sg_import(
		ojk::SavedGameHelper& saved_game)
	{
		saved_game.read<int32_t>(number);
		saved_game.read<int32_t>(waypoint);
		saved_game.read<int32_t>(pathCostToEnemy);
		saved_game.read<int32_t>(closestBuddy);
	}
}; // AIGroupMember_t

#define MAX_GROUP_MEMBERS 32
// !!!!!!!!!! LOADSAVE-affecting structure !!!!!!!!!!
class AIGroupInfo_t
{
public:
	int			numGroup;
	qboolean	processed;
	team_t		team;
	gentity_t	*enemy;
	int			enemyWP;
	int			speechDebounceTime;
	int			lastClearShotTime;
	int			lastSeenEnemyTime;
	int			morale;
	int			moraleAdjust;
	int			moraleDebounce;
	int			memberValidateTime;
	int			activeMemberNum;
	gentity_t	*commander;
	vec3_t		enemyLastSeenPos;
	int			numState[ NUM_SQUAD_STATES ];
	AIGroupMember_t member[ MAX_GROUP_MEMBERS ];


	void sg_export(
		ojk::SavedGameHelper& saved_game) const
	{
		saved_game.write<int32_t>(numGroup);
		saved_game.write<int32_t>(processed);
		saved_game.write<int32_t>(team);
		saved_game.write<int32_t>(enemy);
		saved_game.write<int32_t>(enemyWP);
		saved_game.write<int32_t>(speechDebounceTime);
		saved_game.write<int32_t>(lastClearShotTime);
		saved_game.write<int32_t>(lastSeenEnemyTime);
		saved_game.write<int32_t>(morale);
		saved_game.write<int32_t>(moraleAdjust);
		saved_game.write<int32_t>(moraleDebounce);
		saved_game.write<int32_t>(memberValidateTime);
		saved_game.write<int32_t>(activeMemberNum);
		saved_game.write<int32_t>(commander);
		saved_game.write<float>(enemyLastSeenPos);
		saved_game.write<int32_t>(numState);
		saved_game.write<>(member);
	}

	void sg_import(
		ojk::SavedGameHelper& saved_game)
	{
		saved_game.read<int32_t>(numGroup);
		saved_game.read<int32_t>(processed);
		saved_game.read<int32_t>(team);
		saved_game.read<int32_t>(enemy);
		saved_game.read<int32_t>(enemyWP);
		saved_game.read<int32_t>(speechDebounceTime);
		saved_game.read<int32_t>(lastClearShotTime);
		saved_game.read<int32_t>(lastSeenEnemyTime);
		saved_game.read<int32_t>(morale);
		saved_game.read<int32_t>(moraleAdjust);
		saved_game.read<int32_t>(moraleDebounce);
		saved_game.read<int32_t>(memberValidateTime);
		saved_game.read<int32_t>(activeMemberNum);
		saved_game.read<int32_t>(commander);
		saved_game.read<float>(enemyLastSeenPos);
		saved_game.read<int32_t>(numState);
		saved_game.read<>(member);
	}
}; // AIGroupInfo_t

int	AI_GetGroupSize( vec3_t origin, int radius, team_t playerTeam, gentity_t *avoid = NULL );
int AI_GetGroupSize( gentity_t *ent, int radius );

void AI_GetGroup( gentity_t *self );

qboolean AI_CheckEnemyCollision( gentity_t *ent, qboolean takeEnemy = qtrue );
gentity_t *AI_DistributeAttack( gentity_t *attacker, gentity_t *enemy, team_t team, int threshold );

#endif	//__AI__
