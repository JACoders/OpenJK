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
#pragma pack(push, 4)
class SgAiGroupMember
{
public:
    int32_t number;
    int32_t waypoint;
    int32_t pathCostToEnemy;
    int32_t closestBuddy;
}; // SgAiGroupMember
#pragma pack(pop)

typedef struct AIGroupMember_s
{
    using SgType = SgAiGroupMember;


	int	number;
	int waypoint;
	int pathCostToEnemy;
	int	closestBuddy;


    void sg_export(
        SgType& dst) const;

    void sg_import(
        const SgType& src);
} AIGroupMember_t;

#define MAX_GROUP_MEMBERS 32
// !!!!!!!!!! LOADSAVE-affecting structure !!!!!!!!!!
#pragma pack(push, 4)
class SgAiGroupInfo
{
public:
    int32_t numGroup;
    int32_t processed;
    int32_t team;
    int32_t enemy;
    int32_t enemyWP;
    int32_t speechDebounceTime;
    int32_t lastClearShotTime;
    int32_t lastSeenEnemyTime;
    int32_t morale;
    int32_t moraleAdjust;
    int32_t moraleDebounce;
    int32_t memberValidateTime;
    int32_t activeMemberNum;
    int32_t commander;
    SgVec3 enemyLastSeenPos;
    SgArray<int32_t, NUM_SQUAD_STATES> numState;
    SgArray<SgAiGroupMember, MAX_GROUP_MEMBERS> member;
}; // SgAiGroupInfo
#pragma pack(pop)

typedef struct AIGroupInfo_s
{
    using SgType = SgAiGroupInfo;


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
        SgType& dst) const;

    void sg_import(
        const SgType& src);
} AIGroupInfo_t;

int	AI_GetGroupSize( vec3_t origin, int radius, team_t playerTeam, gentity_t *avoid = NULL );
int AI_GetGroupSize( gentity_t *ent, int radius );

void AI_GetGroup( gentity_t *self );

qboolean AI_CheckEnemyCollision( gentity_t *ent, qboolean takeEnemy = qtrue );
gentity_t *AI_DistributeAttack( gentity_t *attacker, gentity_t *enemy, team_t team, int threshold );

#endif	//__AI__
