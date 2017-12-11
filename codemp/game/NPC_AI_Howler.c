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

// These define the working combat range for these suckers
#define MIN_DISTANCE		54
#define MIN_DISTANCE_SQR	( MIN_DISTANCE * MIN_DISTANCE )

#define MAX_DISTANCE		128
#define MAX_DISTANCE_SQR	( MAX_DISTANCE * MAX_DISTANCE )

#define LSTATE_CLEAR		0
#define LSTATE_WAITING		1

/*
-------------------------
NPC_Howler_Precache
-------------------------
*/
void NPC_Howler_Precache( void )
{
}


/*
-------------------------
Howler_Idle
-------------------------
*/
void Howler_Idle( void ) {
}


/*
-------------------------
Howler_Patrol
-------------------------
*/
void Howler_Patrol( void )
{
	vec3_t dif;

	NPCS.NPCInfo->localState = LSTATE_CLEAR;

	//If we have somewhere to go, then do that
	if ( UpdateGoal() )
	{
		NPCS.ucmd.buttons &= ~BUTTON_WALKING;
		NPC_MoveToGoal( qtrue );
	}
	else
	{
		if ( TIMER_Done( NPCS.NPC, "patrolTime" ))
		{
			TIMER_Set( NPCS.NPC, "patrolTime", Q_flrand(-1.0f, 1.0f) * 5000 + 5000 );
		}
	}

	//rwwFIXMEFIXME: Care about all clients, not just client 0
	//OJK: clientnum 0
	VectorSubtract( g_entities[0].r.currentOrigin, NPCS.NPC->r.currentOrigin, dif );

	if ( VectorLengthSquared( dif ) < 256 * 256 )
	{
		G_SetEnemy( NPCS.NPC, &g_entities[0] );
	}

	if ( NPC_CheckEnemyExt( qtrue ) == qfalse )
	{
		Howler_Idle();
		return;
	}
}

/*
-------------------------
Howler_Move
-------------------------
*/
void Howler_Move( qboolean visible )
{
	if ( NPCS.NPCInfo->localState != LSTATE_WAITING )
	{
		NPCS.NPCInfo->goalEntity = NPCS.NPC->enemy;
		NPC_MoveToGoal( qtrue );
		NPCS.NPCInfo->goalRadius = MAX_DISTANCE;	// just get us within combat range
	}
}

//---------------------------------------------------------
void Howler_TryDamage( gentity_t *enemy, int damage )
{
	vec3_t	end, dir;
	trace_t	tr;

	if ( !enemy )
	{
		return;
	}

	AngleVectors( NPCS.NPC->client->ps.viewangles, dir, NULL, NULL );
	VectorMA( NPCS.NPC->r.currentOrigin, MIN_DISTANCE, dir, end );

	// Should probably trace from the mouth, but, ah well.
	trap->Trace( &tr, NPCS.NPC->r.currentOrigin, vec3_origin, vec3_origin, end, NPCS.NPC->s.number, MASK_SHOT, qfalse, 0, 0 );

	if ( tr.entityNum != ENTITYNUM_WORLD )
	{
		G_Damage( &g_entities[tr.entityNum], NPCS.NPC, NPCS.NPC, dir, tr.endpos, damage, DAMAGE_NO_KNOCKBACK, MOD_MELEE );
	}
}

//------------------------------
void Howler_Attack( void )
{
	if ( !TIMER_Exists( NPCS.NPC, "attacking" ))
	{
		// Going to do ATTACK1
		TIMER_Set( NPCS.NPC, "attacking", 1700 + Q_flrand(0.0f, 1.0f) * 200 );
		NPC_SetAnim( NPCS.NPC, SETANIM_BOTH, BOTH_ATTACK1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );

		TIMER_Set( NPCS.NPC, "attack_dmg", 200 ); // level two damage
	}

	// Need to do delayed damage since the attack animations encapsulate multiple mini-attacks
	if ( TIMER_Done2( NPCS.NPC, "attack_dmg", qtrue ))
	{
		Howler_TryDamage( NPCS.NPC->enemy, 5 );
	}

	// Just using this to remove the attacking flag at the right time
	TIMER_Done2( NPCS.NPC, "attacking", qtrue );
}

//----------------------------------
void Howler_Combat( void )
{
	float distance;
	qboolean advance;

	// If we cannot see our target or we have somewhere to go, then do that
	if ( !NPC_ClearLOS4( NPCS.NPC->enemy ) || UpdateGoal( ))
	{
		NPCS.NPCInfo->combatMove = qtrue;
		NPCS.NPCInfo->goalEntity = NPCS.NPC->enemy;
		NPCS.NPCInfo->goalRadius = MAX_DISTANCE;	// just get us within combat range

		NPC_MoveToGoal( qtrue );
		return;
	}

	// Sometimes I have problems with facing the enemy I'm attacking, so force the issue so I don't look dumb
	NPC_FaceEnemy( qtrue );

	distance	= DistanceHorizontalSquared( NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin );
	advance = (qboolean)( distance > MIN_DISTANCE_SQR ? qtrue : qfalse  );

	if (( advance || NPCS.NPCInfo->localState == LSTATE_WAITING ) && TIMER_Done( NPCS.NPC, "attacking" )) // waiting monsters can't attack
	{
		if ( TIMER_Done2( NPCS.NPC, "takingPain", qtrue ))
		{
			NPCS.NPCInfo->localState = LSTATE_CLEAR;
		}
		else
		{
			Howler_Move( qtrue );
		}
	}
	else
	{
		Howler_Attack();
	}
}

/*
-------------------------
NPC_Howler_Pain
-------------------------
*/
void NPC_Howler_Pain( gentity_t *self, gentity_t *attacker, int damage )
{
	if ( damage >= 10 )
	{
		TIMER_Remove( self, "attacking" );
		TIMER_Set( self, "takingPain", 2900 );

		VectorCopy( self->NPC->lastPathAngles, self->s.angles );

		NPC_SetAnim( self, SETANIM_BOTH, BOTH_PAIN1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );

		if ( self->NPC )
		{
			self->NPC->localState = LSTATE_WAITING;
		}
	}
}


/*
-------------------------
NPC_BSHowler_Default
-------------------------
*/
void NPC_BSHowler_Default( void )
{
	if ( NPCS.NPC->enemy )
		Howler_Combat();
	else if ( NPCS.NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES )
		Howler_Patrol();
	else
		Howler_Idle();

	NPC_UpdateAngles( qtrue, qtrue );
}
