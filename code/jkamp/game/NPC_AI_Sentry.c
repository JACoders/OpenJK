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

extern gitem_t	*BG_FindItemForAmmo( ammo_t ammo );
extern void G_SoundOnEnt( gentity_t *ent, soundChannel_t channel, const char *soundPath );

#define MIN_DISTANCE		256
#define MIN_DISTANCE_SQR	( MIN_DISTANCE * MIN_DISTANCE )

#define SENTRY_FORWARD_BASE_SPEED	10
#define SENTRY_FORWARD_MULTIPLIER	5

#define SENTRY_VELOCITY_DECAY	0.85f
#define SENTRY_STRAFE_VEL		256
#define SENTRY_STRAFE_DIS		200
#define SENTRY_UPWARD_PUSH		32
#define SENTRY_HOVER_HEIGHT		24

//Local state enums
enum
{
	LSTATE_NONE = 0,
	LSTATE_ASLEEP,
	LSTATE_WAKEUP,
	LSTATE_ACTIVE,
	LSTATE_POWERING_UP,
	LSTATE_ATTACKING,
};

/*
-------------------------
NPC_Sentry_Precache
-------------------------
*/
void NPC_Sentry_Precache(void)
{
	int i;

	G_SoundIndex( "sound/chars/sentry/misc/sentry_explo" );
	G_SoundIndex( "sound/chars/sentry/misc/sentry_pain" );
	G_SoundIndex( "sound/chars/sentry/misc/sentry_shield_open" );
	G_SoundIndex( "sound/chars/sentry/misc/sentry_shield_close" );
	G_SoundIndex( "sound/chars/sentry/misc/sentry_hover_1_lp" );
	G_SoundIndex( "sound/chars/sentry/misc/sentry_hover_2_lp" );

	for ( i = 1; i < 4; i++)
	{
		G_SoundIndex( va( "sound/chars/sentry/misc/talk%d", i ) );
	}

	G_EffectIndex( "bryar/muzzle_flash");
	G_EffectIndex( "env/med_explode");

	RegisterItem( BG_FindItemForAmmo( AMMO_BLASTER ));
}

/*
================
sentry_use
================
*/
void sentry_use( gentity_t *self, gentity_t *other, gentity_t *activator)
{
	G_ActivateBehavior(self,BSET_USE);

	self->flags &= ~FL_SHIELDED;
	NPC_SetAnim( self, SETANIM_BOTH, BOTH_POWERUP1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
//	self->NPC->localState = LSTATE_WAKEUP;
	self->NPC->localState = LSTATE_ACTIVE;
}

/*
-------------------------
NPC_Sentry_Pain
-------------------------
*/
void NPC_Sentry_Pain(gentity_t *self, gentity_t *attacker, int damage)
{
	int mod = gPainMOD;

	NPC_Pain( self, attacker, damage );

	if ( mod == MOD_DEMP2 || mod == MOD_DEMP2_ALT )
	{
		self->NPC->burstCount = 0;
		TIMER_Set( self, "attackDelay", Q_irand( 9000, 12000) );
		self->flags |= FL_SHIELDED;
		NPC_SetAnim( self, SETANIM_BOTH, BOTH_FLY_SHIELDED, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		G_Sound( self, CHAN_AUTO, G_SoundIndex("sound/chars/sentry/misc/sentry_pain") );

		self->NPC->localState = LSTATE_ACTIVE;
	}

	// You got hit, go after the enemy
//	if (self->NPC->localState == LSTATE_ASLEEP)
//	{
//		G_Sound( self, G_SoundIndex("sound/chars/sentry/misc/shieldsopen.wav"));
//
//		self->flags &= ~FL_SHIELDED;
//		NPC_SetAnim( self, SETANIM_BOTH, BOTH_POWERUP1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
//		self->NPC->localState = LSTATE_WAKEUP;
//	}
}

/*
-------------------------
Sentry_Fire
-------------------------
*/
void Sentry_Fire (void)
{
	vec3_t	muzzle;
	static	vec3_t	forward, vright, up;
	gentity_t	*missile;
	mdxaBone_t	boltMatrix;
	int			bolt, which;

	NPCS.NPC->flags &= ~FL_SHIELDED;

	if ( NPCS.NPCInfo->localState == LSTATE_POWERING_UP )
	{
		if ( TIMER_Done( NPCS.NPC, "powerup" ))
		{
			NPCS.NPCInfo->localState = LSTATE_ATTACKING;
			NPC_SetAnim( NPCS.NPC, SETANIM_BOTH, BOTH_ATTACK1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		}
		else
		{
			// can't do anything right now
			return;
		}
	}
	else if ( NPCS.NPCInfo->localState == LSTATE_ACTIVE )
	{
		NPCS.NPCInfo->localState = LSTATE_POWERING_UP;

		G_Sound( NPCS.NPC, CHAN_AUTO, G_SoundIndex("sound/chars/sentry/misc/sentry_shield_open") );
		NPC_SetAnim( NPCS.NPC, SETANIM_BOTH, BOTH_POWERUP1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		TIMER_Set( NPCS.NPC, "powerup", 250 );
		return;
	}
	else if ( NPCS.NPCInfo->localState != LSTATE_ATTACKING )
	{
		// bad because we are uninitialized
		NPCS.NPCInfo->localState = LSTATE_ACTIVE;
		return;
	}

	// Which muzzle to fire from?
	which = NPCS.NPCInfo->burstCount % 3;
	switch( which )
	{
	case 0:
		bolt = trap->G2API_AddBolt(NPCS.NPC->ghoul2, 0, "*flash1");
		break;
	case 1:
		bolt = trap->G2API_AddBolt(NPCS.NPC->ghoul2, 0, "*flash2");
		break;
	case 2:
	default:
		bolt = trap->G2API_AddBolt(NPCS.NPC->ghoul2, 0, "*flash03");
	}

	trap->G2API_GetBoltMatrix( NPCS.NPC->ghoul2, 0,
				bolt,
				&boltMatrix, NPCS.NPC->r.currentAngles, NPCS.NPC->r.currentOrigin, level.time,
				NULL, NPCS.NPC->modelScale );

	BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, muzzle );

	AngleVectors( NPCS.NPC->r.currentAngles, forward, vright, up );
//	G_Sound( NPC, G_SoundIndex("sound/chars/sentry/misc/shoot.wav"));

	G_PlayEffectID( G_EffectIndex("bryar/muzzle_flash"), muzzle, forward );

	missile = CreateMissile( muzzle, forward, 1600, 10000, NPCS.NPC, qfalse );

	missile->classname = "bryar_proj";
	missile->s.weapon = WP_BRYAR_PISTOL;

	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_BRYAR_PISTOL;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	NPCS.NPCInfo->burstCount++;
	NPCS.NPC->attackDebounceTime = level.time + 50;
	missile->damage = 5;

	// now scale for difficulty
	if ( g_npcspskill.integer == 0 )
	{
		NPCS.NPC->attackDebounceTime += 200;
		missile->damage = 1;
	}
	else if ( g_npcspskill.integer == 1 )
	{
		NPCS.NPC->attackDebounceTime += 100;
		missile->damage = 3;
	}
}

/*
-------------------------
Sentry_MaintainHeight
-------------------------
*/
void Sentry_MaintainHeight( void )
{
	float	dif;

	NPCS.NPC->s.loopSound = G_SoundIndex( "sound/chars/sentry/misc/sentry_hover_1_lp" );

	// Update our angles regardless
	NPC_UpdateAngles( qtrue, qtrue );

	// If we have an enemy, we should try to hover at about enemy eye level
	if ( NPCS.NPC->enemy )
	{
		// Find the height difference
		dif = (NPCS.NPC->enemy->r.currentOrigin[2]+NPCS.NPC->enemy->r.maxs[2]) - NPCS.NPC->r.currentOrigin[2];

		// cap to prevent dramatic height shifts
		if ( fabs( dif ) > 8 )
		{
			if ( fabs( dif ) > SENTRY_HOVER_HEIGHT )
			{
				dif = ( dif < 0 ? -24 : 24 );
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

		if (goal)
		{
			dif = goal->r.currentOrigin[2] - NPCS.NPC->r.currentOrigin[2];

			if ( fabs( dif ) > SENTRY_HOVER_HEIGHT )
			{
				NPCS.ucmd.upmove = ( NPCS.ucmd.upmove < 0 ? -4 : 4 );
			}
			else
			{
				if ( NPCS.NPC->client->ps.velocity[2] )
				{
					NPCS.NPC->client->ps.velocity[2] *= SENTRY_VELOCITY_DECAY;

					if ( fabs( NPCS.NPC->client->ps.velocity[2] ) < 2 )
					{
						NPCS.NPC->client->ps.velocity[2] = 0;
					}
				}
			}
		}
		// Apply friction to Z
		else if ( NPCS.NPC->client->ps.velocity[2] )
		{
			NPCS.NPC->client->ps.velocity[2] *= SENTRY_VELOCITY_DECAY;

			if ( fabs( NPCS.NPC->client->ps.velocity[2] ) < 1 )
			{
				NPCS.NPC->client->ps.velocity[2] = 0;
			}
		}
	}

	// Apply friction
	if ( NPCS.NPC->client->ps.velocity[0] )
	{
		NPCS.NPC->client->ps.velocity[0] *= SENTRY_VELOCITY_DECAY;

		if ( fabs( NPCS.NPC->client->ps.velocity[0] ) < 1 )
		{
			NPCS.NPC->client->ps.velocity[0] = 0;
		}
	}

	if ( NPCS.NPC->client->ps.velocity[1] )
	{
		NPCS.NPC->client->ps.velocity[1] *= SENTRY_VELOCITY_DECAY;

		if ( fabs( NPCS.NPC->client->ps.velocity[1] ) < 1 )
		{
			NPCS.NPC->client->ps.velocity[1] = 0;
		}
	}

	NPC_FaceEnemy( qtrue );
}

/*
-------------------------
Sentry_Idle
-------------------------
*/
void Sentry_Idle( void )
{
	Sentry_MaintainHeight();

	// Is he waking up?
	if (NPCS.NPCInfo->localState == LSTATE_WAKEUP)
	{
		if (NPCS.NPC->client->ps.torsoTimer<=0)
		{
			NPCS.NPCInfo->scriptFlags |= SCF_LOOK_FOR_ENEMIES;
			NPCS.NPCInfo->burstCount = 0;
		}
	}
	else
	{
		NPC_SetAnim( NPCS.NPC, SETANIM_BOTH, BOTH_SLEEP1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		NPCS.NPC->flags |= FL_SHIELDED;

		NPC_BSIdle();
	}
}

/*
-------------------------
Sentry_Strafe
-------------------------
*/
void Sentry_Strafe( void )
{
	int		dir;
	vec3_t	end, right;
	trace_t	tr;

	AngleVectors( NPCS.NPC->client->renderInfo.eyeAngles, NULL, right, NULL );

	// Pick a random strafe direction, then check to see if doing a strafe would be
	//	reasonable valid
	dir = ( rand() & 1 ) ? -1 : 1;
	VectorMA( NPCS.NPC->r.currentOrigin, SENTRY_STRAFE_DIS * dir, right, end );

	trap->Trace( &tr, NPCS.NPC->r.currentOrigin, NULL, NULL, end, NPCS.NPC->s.number, MASK_SOLID, qfalse, 0, 0 );

	// Close enough
	if ( tr.fraction > 0.9f )
	{
		VectorMA( NPCS.NPC->client->ps.velocity, SENTRY_STRAFE_VEL * dir, right, NPCS.NPC->client->ps.velocity );

		// Add a slight upward push
		NPCS.NPC->client->ps.velocity[2] += SENTRY_UPWARD_PUSH;

		// Set the strafe start time so we can do a controlled roll
	//	NPC->fx_time = level.time;
		NPCS.NPCInfo->standTime = level.time + 3000 + Q_flrand(0.0f, 1.0f) * 500;
	}
}

/*
-------------------------
Sentry_Hunt
-------------------------
*/
void Sentry_Hunt( qboolean visible, qboolean advance )
{
	float	distance, speed;
	vec3_t	forward;

	//If we're not supposed to stand still, pursue the player
	if ( NPCS.NPCInfo->standTime < level.time )
	{
		// Only strafe when we can see the player
		if ( visible )
		{
			Sentry_Strafe();
			return;
		}
	}

	//If we don't want to advance, stop here
	if ( !advance && visible )
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

	speed = SENTRY_FORWARD_BASE_SPEED + SENTRY_FORWARD_MULTIPLIER * g_npcspskill.integer;
	VectorMA( NPCS.NPC->client->ps.velocity, speed, forward, NPCS.NPC->client->ps.velocity );
}

/*
-------------------------
Sentry_RangedAttack
-------------------------
*/
void Sentry_RangedAttack( qboolean visible, qboolean advance )
{
	if ( TIMER_Done( NPCS.NPC, "attackDelay" ) && NPCS.NPC->attackDebounceTime < level.time && visible )	// Attack?
	{
		if ( NPCS.NPCInfo->burstCount > 6 )
		{
			if ( !NPCS.NPC->fly_sound_debounce_time )
			{//delay closing down to give the player an opening
				NPCS.NPC->fly_sound_debounce_time = level.time + Q_irand( 500, 2000 );
			}
			else if ( NPCS.NPC->fly_sound_debounce_time < level.time )
			{
				NPCS.NPCInfo->localState = LSTATE_ACTIVE;
				NPCS.NPC->fly_sound_debounce_time = NPCS.NPCInfo->burstCount = 0;
				TIMER_Set( NPCS.NPC, "attackDelay", Q_irand( 2000, 3500) );
				NPCS.NPC->flags |= FL_SHIELDED;
				NPC_SetAnim( NPCS.NPC, SETANIM_BOTH, BOTH_FLY_SHIELDED, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				G_SoundOnEnt( NPCS.NPC, CHAN_AUTO, "sound/chars/sentry/misc/sentry_shield_close" );
			}
		}
		else
		{
			Sentry_Fire();
		}
	}

	if ( NPCS.NPCInfo->scriptFlags & SCF_CHASE_ENEMIES )
	{
		Sentry_Hunt( visible, advance );
	}
}

/*
-------------------------
Sentry_AttackDecision
-------------------------
*/
void Sentry_AttackDecision( void )
{
	float		distance;
	qboolean	visible, advance;

	// Always keep a good height off the ground
	Sentry_MaintainHeight();

	NPCS.NPC->s.loopSound = G_SoundIndex( "sound/chars/sentry/misc/sentry_hover_2_lp" );

	//randomly talk
	if ( TIMER_Done(NPCS.NPC,"patrolNoise") )
	{
		if (TIMER_Done(NPCS.NPC,"angerNoise"))
		{
			G_SoundOnEnt( NPCS.NPC, CHAN_AUTO, va("sound/chars/sentry/misc/talk%d", Q_irand(1, 3)) );

			TIMER_Set( NPCS.NPC, "patrolNoise", Q_irand( 4000, 10000 ) );
		}
	}

	// He's dead.
	if (NPCS.NPC->enemy->health<1)
	{
		NPCS.NPC->enemy = NULL;
		Sentry_Idle();
		return;
	}

	// If we don't have an enemy, just idle
	if ( NPC_CheckEnemyExt(qfalse) == qfalse )
	{
		Sentry_Idle();
		return;
	}

	// Rate our distance to the target and visibilty
	distance	= (int) DistanceHorizontalSquared( NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin );
	visible		= NPC_ClearLOS4( NPCS.NPC->enemy );
	advance		= (qboolean)(distance > MIN_DISTANCE_SQR);

	// If we cannot see our target, move to see it
	if ( visible == qfalse )
	{
		if ( NPCS.NPCInfo->scriptFlags & SCF_CHASE_ENEMIES )
		{
			Sentry_Hunt( visible, advance );
			return;
		}
	}

	NPC_FaceEnemy( qtrue );

	Sentry_RangedAttack( visible, advance );
}

qboolean NPC_CheckPlayerTeamStealth( void );

/*
-------------------------
NPC_Sentry_Patrol
-------------------------
*/
void NPC_Sentry_Patrol( void )
{
	Sentry_MaintainHeight();

	//If we have somewhere to go, then do that
	if (!NPCS.NPC->enemy)
	{
		if ( NPC_CheckPlayerTeamStealth() )
		{
			//NPC_AngerSound();
			NPC_UpdateAngles( qtrue, qtrue );
			return;
		}

		if ( UpdateGoal() )
		{
			//start loop sound once we move
			NPCS.ucmd.buttons |= BUTTON_WALKING;
			NPC_MoveToGoal( qtrue );
		}

		//randomly talk
		if (TIMER_Done(NPCS.NPC,"patrolNoise"))
		{
			G_SoundOnEnt( NPCS.NPC, CHAN_AUTO, va("sound/chars/sentry/misc/talk%d", Q_irand(1, 3)) );

			TIMER_Set( NPCS.NPC, "patrolNoise", Q_irand( 2000, 4000 ) );
		}
	}

	NPC_UpdateAngles( qtrue, qtrue );
}

/*
-------------------------
NPC_BSSentry_Default
-------------------------
*/
void NPC_BSSentry_Default( void )
{
	if ( NPCS.NPC->targetname )
	{
		NPCS.NPC->use = sentry_use;
	}

	if (( NPCS.NPC->enemy ) && (NPCS.NPCInfo->localState != LSTATE_WAKEUP))
	{
		// Don't attack if waking up or if no enemy
		Sentry_AttackDecision();
	}
	else if ( NPCS.NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES )
	{
		NPC_Sentry_Patrol();
	}
	else
	{
		Sentry_Idle();
	}
}
