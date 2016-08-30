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

void Interrogator_Idle( void );
void DeathFX( gentity_t *ent );
extern void G_SoundOnEnt( gentity_t *ent, soundChannel_t channel, const char *soundPath );

enum
{
LSTATE_BLADESTOP=0,
LSTATE_BLADEUP,
LSTATE_BLADEDOWN,
};

/*
-------------------------
NPC_Interrogator_Precache
-------------------------
*/
void NPC_Interrogator_Precache(gentity_t *self)
{
	G_SoundIndex( "sound/chars/interrogator/misc/torture_droid_lp" );
	G_SoundIndex("sound/chars/mark1/misc/anger.wav");
	G_SoundIndex( "sound/chars/probe/misc/talk");
	G_SoundIndex( "sound/chars/interrogator/misc/torture_droid_inject" );
	G_SoundIndex( "sound/chars/interrogator/misc/int_droid_explo" );
	G_EffectIndex( "explosions/droidexplosion1" );
}
/*
-------------------------
Interrogator_die
-------------------------
*/
void Interrogator_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod,int dFlags,int hitLoc )
{
	self->client->ps.velocity[2] = -100;
	/*
	self->locationDamage[HL_NONE] += damage;
	if (self->locationDamage[HL_NONE] > 40)
	{
		DeathFX(self);
		self->client->ps.eFlags |= EF_NODRAW;
		self->contents = CONTENTS_CORPSE;
	}
	else
	*/
	{
		self->client->ps.eFlags2 &= ~EF2_FLYING;//moveType = MT_WALK;
		self->client->ps.velocity[0] = Q_irand( -20, -10 );
		self->client->ps.velocity[1] = Q_irand( -20, -10 );
		self->client->ps.velocity[2] = -100;
	}
	//self->takedamage = qfalse;
	//self->client->ps.eFlags |= EF_NODRAW;
	//self->contents = 0;
	return;
}

/*
-------------------------
Interrogator_PartsMove
-------------------------
*/
void Interrogator_PartsMove(void)
{
	// Syringe
	if ( TIMER_Done(NPCS.NPC,"syringeDelay") )
	{
		NPCS.NPC->pos1[1] = AngleNormalize360( NPCS.NPC->pos1[1]);

		if ((NPCS.NPC->pos1[1] < 60) || (NPCS.NPC->pos1[1] > 300))
		{
			NPCS.NPC->pos1[1]+=Q_irand( -20, 20 );	// Pitch
		}
		else if (NPCS.NPC->pos1[1] > 180)
		{
			NPCS.NPC->pos1[1]=Q_irand( 300, 360 );	// Pitch
		}
		else
		{
			NPCS.NPC->pos1[1]=Q_irand( 0, 60 );	// Pitch
		}

	//	trap->G2API_SetBoneAnglesIndex( &NPC->ghoul2[NPC->playerModel], NPC->genericBone1, NPC->pos1, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL );
		NPC_SetBoneAngles(NPCS.NPC, "left_arm", NPCS.NPC->pos1);

		TIMER_Set( NPCS.NPC, "syringeDelay", Q_irand( 100, 1000 ) );
	}

	// Scalpel
	if ( TIMER_Done(NPCS.NPC,"scalpelDelay") )
	{
		// Change pitch
		if ( NPCS.NPCInfo->localState == LSTATE_BLADEDOWN )	// Blade is moving down
		{
			NPCS.NPC->pos2[0]-= 30;
			if (NPCS.NPC->pos2[0] < 180)
			{
				NPCS.NPC->pos2[0] = 180;
				NPCS.NPCInfo->localState = LSTATE_BLADEUP;	// Make it move up
			}
		}
		else											// Blade is coming back up
		{
			NPCS.NPC->pos2[0]+= 30;
			if (NPCS.NPC->pos2[0] >= 360)
			{
				NPCS.NPC->pos2[0] = 360;
				NPCS.NPCInfo->localState = LSTATE_BLADEDOWN;	// Make it move down
				TIMER_Set( NPCS.NPC, "scalpelDelay", Q_irand( 100, 1000 ) );
			}
		}

		NPCS.NPC->pos2[0] = AngleNormalize360( NPCS.NPC->pos2[0]);
	//	trap->G2API_SetBoneAnglesIndex( &NPC->ghoul2[NPC->playerModel], NPC->genericBone2, NPC->pos2, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL );

		NPC_SetBoneAngles(NPCS.NPC, "right_arm", NPCS.NPC->pos2);
	}

	// Claw
	NPCS.NPC->pos3[1] += Q_irand( 10, 30 );
	NPCS.NPC->pos3[1] = AngleNormalize360( NPCS.NPC->pos3[1]);
	//trap->G2API_SetBoneAnglesIndex( &NPC->ghoul2[NPC->playerModel], NPC->genericBone3, NPC->pos3, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL );

	NPC_SetBoneAngles(NPCS.NPC, "claw", NPCS.NPC->pos3);

}

#define VELOCITY_DECAY	0.85f
#define HUNTER_UPWARD_PUSH	2

/*
-------------------------
Interrogator_MaintainHeight
-------------------------
*/
void Interrogator_MaintainHeight( void )
{
	float	dif;
//	vec3_t	endPos;
//	trace_t	trace;

	NPCS.NPC->s.loopSound = G_SoundIndex( "sound/chars/interrogator/misc/torture_droid_lp" );
	// Update our angles regardless
	NPC_UpdateAngles( qtrue, qtrue );

	// If we have an enemy, we should try to hover at about enemy eye level
	if ( NPCS.NPC->enemy )
	{
		// Find the height difference
		dif = (NPCS.NPC->enemy->r.currentOrigin[2] + NPCS.NPC->enemy->r.maxs[2]) - NPCS.NPC->r.currentOrigin[2];

		// cap to prevent dramatic height shifts
		if ( fabs( dif ) > 2 )
		{
			if ( fabs( dif ) > 16 )
			{
				dif = ( dif < 0 ? -16 : 16 );
			}

			NPCS.NPC->client->ps.velocity[2] = (NPCS.NPC->client->ps.velocity[2]+dif)/2;
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
				NPCS.ucmd.upmove = ( NPCS.ucmd.upmove < 0 ? -4 : 4 );
			}
			else
			{
				if ( NPCS.NPC->client->ps.velocity[2] )
				{
					NPCS.NPC->client->ps.velocity[2] *= VELOCITY_DECAY;

					if ( fabs( NPCS.NPC->client->ps.velocity[2] ) < 2 )
					{
						NPCS.NPC->client->ps.velocity[2] = 0;
					}
				}
			}
		}
		// Apply friction
		else if ( NPCS.NPC->client->ps.velocity[2] )
		{
			NPCS.NPC->client->ps.velocity[2] *= VELOCITY_DECAY;

			if ( fabs( NPCS.NPC->client->ps.velocity[2] ) < 1 )
			{
				NPCS.NPC->client->ps.velocity[2] = 0;
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

#define HUNTER_STRAFE_VEL	32
#define HUNTER_STRAFE_DIS	200
/*
-------------------------
Interrogator_Strafe
-------------------------
*/
void Interrogator_Strafe( void )
{
	int		dir;
	vec3_t	end, right;
	trace_t	tr;
	float	dif;

	AngleVectors( NPCS.NPC->client->renderInfo.eyeAngles, NULL, right, NULL );

	// Pick a random strafe direction, then check to see if doing a strafe would be
	//	reasonable valid
	dir = ( rand() & 1 ) ? -1 : 1;
	VectorMA( NPCS.NPC->r.currentOrigin, HUNTER_STRAFE_DIS * dir, right, end );

	trap->Trace( &tr, NPCS.NPC->r.currentOrigin, NULL, NULL, end, NPCS.NPC->s.number, MASK_SOLID, qfalse, 0, 0 );

	// Close enough
	if ( tr.fraction > 0.9f )
	{
		VectorMA( NPCS.NPC->client->ps.velocity, HUNTER_STRAFE_VEL * dir, right, NPCS.NPC->client->ps.velocity );

		// Add a slight upward push
		if ( NPCS.NPC->enemy )
		{
			// Find the height difference
			dif = (NPCS.NPC->enemy->r.currentOrigin[2] + 32) - NPCS.NPC->r.currentOrigin[2];

			// cap to prevent dramatic height shifts
			if ( fabs( dif ) > 8 )
			{
				dif = ( dif < 0 ? -HUNTER_UPWARD_PUSH : HUNTER_UPWARD_PUSH );
			}

			NPCS.NPC->client->ps.velocity[2] += dif;

		}

		// Set the strafe start time
		//NPCS.NPC->fx_time = level.time;
		NPCS.NPCInfo->standTime = level.time + 3000 + Q_flrand(0.0f, 1.0f) * 500;
	}
}

/*
-------------------------
Interrogator_Hunt
-------------------------`
*/

#define HUNTER_FORWARD_BASE_SPEED	10
#define HUNTER_FORWARD_MULTIPLIER	2

void Interrogator_Hunt( qboolean visible, qboolean advance )
{
	float	distance, speed;
	vec3_t	forward;

	Interrogator_PartsMove();

	NPC_FaceEnemy(qfalse);

	//If we're not supposed to stand still, pursue the player
	if ( NPCS.NPCInfo->standTime < level.time )
	{
		// Only strafe when we can see the player
		if ( visible )
		{
			Interrogator_Strafe();
			if ( NPCS.NPCInfo->standTime > level.time )
			{//successfully strafed
				return;
			}
		}
	}

	//If we don't want to advance, stop here
	if ( advance == qfalse )
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

	speed = HUNTER_FORWARD_BASE_SPEED + HUNTER_FORWARD_MULTIPLIER * g_npcspskill.integer;
	VectorMA( NPCS.NPC->client->ps.velocity, speed, forward, NPCS.NPC->client->ps.velocity );
}

#define MIN_DISTANCE		64

/*
-------------------------
Interrogator_Melee
-------------------------
*/
void Interrogator_Melee( qboolean visible, qboolean advance )
{
	if ( TIMER_Done( NPCS.NPC, "attackDelay" ) )	// Attack?
	{
		// Make sure that we are within the height range before we allow any damage to happen
		if ( NPCS.NPC->r.currentOrigin[2] >= NPCS.NPC->enemy->r.currentOrigin[2]+NPCS.NPC->enemy->r.mins[2] && NPCS.NPC->r.currentOrigin[2]+NPCS.NPC->r.mins[2]+8 < NPCS.NPC->enemy->r.currentOrigin[2]+NPCS.NPC->enemy->r.maxs[2] )
		{
			//gentity_t *tent;

			TIMER_Set( NPCS.NPC, "attackDelay", Q_irand( 500, 3000 ) );
			G_Damage( NPCS.NPC->enemy, NPCS.NPC, NPCS.NPC, 0, 0, 2, DAMAGE_NO_KNOCKBACK, MOD_MELEE );

		//	NPC->enemy->client->poisonDamage = 18;
		//	NPC->enemy->client->poisonTime = level.time + 1000;

			// Drug our enemy up and do the wonky vision thing
//			tent = G_TempEntity( NPC->enemy->r.currentOrigin, EV_DRUGGED );
//			tent->owner = NPC->enemy;

			//rwwFIXMEFIXME: poison damage

			G_Sound( NPCS.NPC, CHAN_AUTO, G_SoundIndex( "sound/chars/interrogator/misc/torture_droid_inject.mp3" ));
		}
	}

	if ( NPCS.NPCInfo->scriptFlags & SCF_CHASE_ENEMIES )
	{
		Interrogator_Hunt( visible, advance );
	}
}

/*
-------------------------
Interrogator_Attack
-------------------------
*/
void Interrogator_Attack( void )
{
	float		distance;
	qboolean	visible;
	qboolean	advance;

	// Always keep a good height off the ground
	Interrogator_MaintainHeight();

	//randomly talk
	if ( TIMER_Done(NPCS.NPC,"patrolNoise") )
	{
		if (TIMER_Done(NPCS.NPC,"angerNoise"))
		{
			G_SoundOnEnt( NPCS.NPC, CHAN_AUTO, va("sound/chars/probe/misc/talk.wav",	Q_irand(1, 3)) );

			TIMER_Set( NPCS.NPC, "patrolNoise", Q_irand( 4000, 10000 ) );
		}
	}

	// If we don't have an enemy, just idle
	if ( NPC_CheckEnemyExt(qfalse) == qfalse )
	{
		Interrogator_Idle();
		return;
	}

	// Rate our distance to the target, and our visibilty
	distance	= (int) DistanceHorizontalSquared( NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin );
	visible		= NPC_ClearLOS4( NPCS.NPC->enemy );
	advance		= (qboolean)(distance > MIN_DISTANCE*MIN_DISTANCE );

	if ( !visible )
	{
		advance = qtrue;
	}
	if ( NPCS.NPCInfo->scriptFlags & SCF_CHASE_ENEMIES )
	{
		Interrogator_Hunt( visible, advance );
	}

	NPC_FaceEnemy( qtrue );

	if (!advance)
	{
		Interrogator_Melee( visible, advance );
	}
}

/*
-------------------------
Interrogator_Idle
-------------------------
*/
void Interrogator_Idle( void )
{
	if ( NPC_CheckPlayerTeamStealth() )
	{
		G_SoundOnEnt( NPCS.NPC, CHAN_AUTO, "sound/chars/mark1/misc/anger.wav" );
		NPC_UpdateAngles( qtrue, qtrue );
		return;
	}

	Interrogator_MaintainHeight();

	NPC_BSIdle();
}

/*
-------------------------
NPC_BSInterrogator_Default
-------------------------
*/
void NPC_BSInterrogator_Default( void )
{
	//NPC->e_DieFunc = dieF_Interrogator_die;

	if ( NPCS.NPC->enemy )
	{
		Interrogator_Attack();
	}
	else
	{
		Interrogator_Idle();
	}

}
