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
#include "g_nav.h"

void Remote_Strafe( void );

#define VELOCITY_DECAY	0.85f


//Local state enums
enum
{
	LSTATE_NONE = 0,
};

void Remote_Idle( void );

void NPC_Remote_Precache(void)
{
	G_SoundIndex("sound/chars/remote/misc/fire.wav");
	G_SoundIndex( "sound/chars/remote/misc/hiss.wav");
	G_EffectIndex( "env/small_explode");
}

/*
-------------------------
NPC_Remote_Pain
-------------------------
*/
void NPC_Remote_Pain(gentity_t *self, gentity_t *attacker, int damage)
{
	SaveNPCGlobals();
	SetNPCGlobals( self );
	Remote_Strafe();
	RestoreNPCGlobals();

	NPC_Pain( self, attacker, damage );
}

/*
-------------------------
Remote_MaintainHeight
-------------------------
*/
void Remote_MaintainHeight( void )
{
	float	dif;

	// Update our angles regardless
	NPC_UpdateAngles( qtrue, qtrue );

	if ( NPCS.NPC->client->ps.velocity[2] )
	{
		NPCS.NPC->client->ps.velocity[2] *= VELOCITY_DECAY;

		if ( fabs( NPCS.NPC->client->ps.velocity[2] ) < 2 )
		{
			NPCS.NPC->client->ps.velocity[2] = 0;
		}
	}
	// If we have an enemy, we should try to hover at or a little below enemy eye level
	if ( NPCS.NPC->enemy )
	{
		if (TIMER_Done( NPCS.NPC, "heightChange"))
		{
			TIMER_Set( NPCS.NPC,"heightChange",Q_irand( 1000, 3000 ));

			// Find the height difference
			dif = (NPCS.NPC->enemy->r.currentOrigin[2] +  Q_irand( 0, NPCS.NPC->enemy->r.maxs[2]+8 )) - NPCS.NPC->r.currentOrigin[2];

			// cap to prevent dramatic height shifts
			if ( fabs( dif ) > 2 )
			{
				if ( fabs( dif ) > 24 )
				{
					dif = ( dif < 0 ? -24 : 24 );
				}
				dif *= 10;
				NPCS.NPC->client->ps.velocity[2] = (NPCS.NPC->client->ps.velocity[2]+dif)/2;
			//	NPC->fx_time = level.time;
				G_Sound( NPCS.NPC, CHAN_AUTO, G_SoundIndex("sound/chars/remote/misc/hiss.wav"));
			}
		}
	}
	else
	{
		gentity_t *goal = NULL;

		if ( NPCS.NPCInfo->goalEntity )	// Is there a goal?
		{
			goal = NPCS.NPCInfo->goalEntity;
		}
		else
		{
			goal = NPCS.NPCInfo->lastGoalEntity;
		}
		if ( goal )
		{
			dif = goal->r.currentOrigin[2] - NPCS.NPC->r.currentOrigin[2];

			if ( fabs( dif ) > 24 )
			{
				dif = ( dif < 0 ? -24 : 24 );
				NPCS.NPC->client->ps.velocity[2] = (NPCS.NPC->client->ps.velocity[2]+dif)/2;
			}
		}
	}

	// Apply friction
	if ( NPCS.NPC->client->ps.velocity[0] )
	{
		NPCS.NPC->client->ps.velocity[0] *= VELOCITY_DECAY;

		if ( fabs( NPCS.NPC->client->ps.velocity[0] ) < 1 )
		{
			NPCS.NPC->client->ps.velocity[0] = 0;
		}
	}

	if ( NPCS.NPC->client->ps.velocity[1] )
	{
		NPCS.NPC->client->ps.velocity[1] *= VELOCITY_DECAY;

		if ( fabs( NPCS.NPC->client->ps.velocity[1] ) < 1 )
		{
			NPCS.NPC->client->ps.velocity[1] = 0;
		}
	}
}

#define REMOTE_STRAFE_VEL	256
#define REMOTE_STRAFE_DIS	200
#define REMOTE_UPWARD_PUSH	32

/*
-------------------------
Remote_Strafe
-------------------------
*/
void Remote_Strafe( void )
{
	int		dir;
	vec3_t	end, right;
	trace_t	tr;

	AngleVectors( NPCS.NPC->client->renderInfo.eyeAngles, NULL, right, NULL );

	// Pick a random strafe direction, then check to see if doing a strafe would be
	//	reasonable valid
	dir = ( rand() & 1 ) ? -1 : 1;
	VectorMA( NPCS.NPC->r.currentOrigin, REMOTE_STRAFE_DIS * dir, right, end );

	trap->Trace( &tr, NPCS.NPC->r.currentOrigin, NULL, NULL, end, NPCS.NPC->s.number, MASK_SOLID, qfalse, 0, 0 );

	// Close enough
	if ( tr.fraction > 0.9f )
	{
		VectorMA( NPCS.NPC->client->ps.velocity, REMOTE_STRAFE_VEL * dir, right, NPCS.NPC->client->ps.velocity );

		G_Sound( NPCS.NPC, CHAN_AUTO, G_SoundIndex("sound/chars/remote/misc/hiss.wav"));

		// Add a slight upward push
		NPCS.NPC->client->ps.velocity[2] += REMOTE_UPWARD_PUSH;

		// Set the strafe start time so we can do a controlled roll
	//	NPC->fx_time = level.time;
		NPCS.NPCInfo->standTime = level.time + 3000 + Q_flrand(0.0f, 1.0f) * 500;
	}
}

#define REMOTE_FORWARD_BASE_SPEED	10
#define REMOTE_FORWARD_MULTIPLIER	5

/*
-------------------------
Remote_Hunt
-------------------------
*/
void Remote_Hunt( qboolean visible, qboolean advance, qboolean retreat )
{
	float	distance, speed;
	vec3_t	forward;

	//If we're not supposed to stand still, pursue the player
	if ( NPCS.NPCInfo->standTime < level.time )
	{
		// Only strafe when we can see the player
		if ( visible )
		{
			Remote_Strafe();
			return;
		}
	}

	//If we don't want to advance, stop here
	if ( advance == qfalse && visible == qtrue )
		return;

	//Only try and navigate if the player is visible
	if ( visible == qfalse )
	{
		// Move towards our goal
		NPCS.NPCInfo->goalEntity = NPCS.NPC->enemy;
		NPCS.NPCInfo->goalRadius = 12;

		//Get our direction from the navigator if we can't see our target
		if ( NPC_GetMoveDirection( forward, &distance ) == qfalse )
			return;
	}
	else
	{
		VectorSubtract( NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin, forward );
		/*distance = */VectorNormalize( forward );
	}

	speed = REMOTE_FORWARD_BASE_SPEED + REMOTE_FORWARD_MULTIPLIER * g_npcspskill.integer;
	if ( retreat == qtrue )
	{
		speed *= -1;
	}
	VectorMA( NPCS.NPC->client->ps.velocity, speed, forward, NPCS.NPC->client->ps.velocity );
}


/*
-------------------------
Remote_Fire
-------------------------
*/
void Remote_Fire (void)
{
	vec3_t	delta1, enemy_org1, muzzle1;
	vec3_t	angleToEnemy1;
	static	vec3_t	forward, vright, up;
//	static	vec3_t	muzzle;
	gentity_t	*missile;

	CalcEntitySpot( NPCS.NPC->enemy, SPOT_HEAD, enemy_org1 );
	VectorCopy( NPCS.NPC->r.currentOrigin, muzzle1 );

	VectorSubtract (enemy_org1, muzzle1, delta1);

	vectoangles ( delta1, angleToEnemy1 );
	AngleVectors (angleToEnemy1, forward, vright, up);

	missile = CreateMissile( NPCS.NPC->r.currentOrigin, forward, 1000, 10000, NPCS.NPC, qfalse );

	G_PlayEffectID( G_EffectIndex("bryar/muzzle_flash"), NPCS.NPC->r.currentOrigin, forward );

	missile->classname = "briar";
	missile->s.weapon = WP_BRYAR_PISTOL;

	missile->damage = 10;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_BRYAR_PISTOL;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

}

/*
-------------------------
Remote_Ranged
-------------------------
*/
void Remote_Ranged( qboolean visible, qboolean advance, qboolean retreat )
{
	if ( TIMER_Done( NPCS.NPC, "attackDelay" ) )	// Attack?
	{
		TIMER_Set( NPCS.NPC, "attackDelay", Q_irand( 500, 3000 ) );
		Remote_Fire();
	}

	if ( NPCS.NPCInfo->scriptFlags & SCF_CHASE_ENEMIES )
	{
		Remote_Hunt( visible, advance, retreat );
	}
}

#define	MIN_MELEE_RANGE		320
#define	MIN_MELEE_RANGE_SQR	( MIN_MELEE_RANGE * MIN_MELEE_RANGE )

#define MIN_DISTANCE		80
#define MIN_DISTANCE_SQR	( MIN_DISTANCE * MIN_DISTANCE )

/*
-------------------------
Remote_Attack
-------------------------
*/
void Remote_Attack( void )
{
	float		distance;
	qboolean	visible;
	float		idealDist;
	qboolean	advance, retreat;

	if ( TIMER_Done(NPCS.NPC,"spin") )
	{
		TIMER_Set( NPCS.NPC, "spin", Q_irand( 250, 1500 ) );
		NPCS.NPCInfo->desiredYaw += Q_irand( -200, 200 );
	}
	// Always keep a good height off the ground
	Remote_MaintainHeight();

	// If we don't have an enemy, just idle
	if ( NPC_CheckEnemyExt(qfalse) == qfalse )
	{
		Remote_Idle();
		return;
	}

	// Rate our distance to the target, and our visibilty
	distance	= (int) DistanceHorizontalSquared( NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin );
	visible		= NPC_ClearLOS4( NPCS.NPC->enemy );
	idealDist	= MIN_DISTANCE_SQR+(MIN_DISTANCE_SQR*flrand( 0, 1 ));
	advance		= (qboolean)(distance > idealDist*1.25);
	retreat		= (qboolean)(distance < idealDist*0.75);

	// If we cannot see our target, move to see it
	if ( visible == qfalse )
	{
		if ( NPCS.NPCInfo->scriptFlags & SCF_CHASE_ENEMIES )
		{
			Remote_Hunt( visible, advance, retreat );
			return;
		}
	}

	Remote_Ranged( visible, advance, retreat );

}

/*
-------------------------
Remote_Idle
-------------------------
*/
void Remote_Idle( void )
{
	Remote_MaintainHeight();

	NPC_BSIdle();
}

/*
-------------------------
Remote_Patrol
-------------------------
*/
void Remote_Patrol( void )
{
	Remote_MaintainHeight();

	//If we have somewhere to go, then do that
	if (!NPCS.NPC->enemy)
	{
		if ( UpdateGoal() )
		{
			//start loop sound once we move
			NPCS.ucmd.buttons |= BUTTON_WALKING;
			NPC_MoveToGoal( qtrue );
		}
	}

	NPC_UpdateAngles( qtrue, qtrue );
}


/*
-------------------------
NPC_BSRemote_Default
-------------------------
*/
void NPC_BSRemote_Default( void )
{
	if ( NPCS.NPC->enemy )
		Remote_Attack();
	else if ( NPCS.NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES )
		Remote_Patrol();
	else
		Remote_Idle();
}
