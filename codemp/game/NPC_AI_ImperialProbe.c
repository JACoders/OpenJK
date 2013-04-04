#include "b_local.h"
#include "g_nav.h"

#include "../namespace_begin.h"
gitem_t	*BG_FindItemForAmmo( ammo_t ammo );
#include "../namespace_end.h"
extern void G_SoundOnEnt( gentity_t *ent, soundChannel_t channel, const char *soundPath );

//Local state enums
enum
{
	LSTATE_NONE = 0,
	LSTATE_BACKINGUP,
	LSTATE_SPINNING,
	LSTATE_PAIN,
	LSTATE_DROP
};

void ImperialProbe_Idle( void );

void NPC_Probe_Precache(void)
{
	int i;

	for ( i = 1; i < 4; i++)
	{
		G_SoundIndex( va( "sound/chars/probe/misc/probetalk%d", i ) );
	}
	G_SoundIndex( "sound/chars/probe/misc/probedroidloop" );
	G_SoundIndex("sound/chars/probe/misc/anger1");
	G_SoundIndex("sound/chars/probe/misc/fire");

	G_EffectIndex( "chunks/probehead" );
	G_EffectIndex( "env/med_explode2" );
	G_EffectIndex( "explosions/probeexplosion1");
	G_EffectIndex( "bryar/muzzle_flash" );

	RegisterItem( BG_FindItemForAmmo( AMMO_BLASTER ));
	RegisterItem( BG_FindItemForWeapon( WP_BRYAR_PISTOL ) );
}
/*
-------------------------
Hunter_MaintainHeight
-------------------------
*/

#define VELOCITY_DECAY	0.85f

void ImperialProbe_MaintainHeight( void )
{	
	float	dif;
//	vec3_t	endPos;
//	trace_t	trace;

	// Update our angles regardless
	NPC_UpdateAngles( qtrue, qtrue );

	// If we have an enemy, we should try to hover at about enemy eye level
	if ( NPC->enemy )
	{
		// Find the height difference
		dif = NPC->enemy->r.currentOrigin[2] - NPC->r.currentOrigin[2]; 

		// cap to prevent dramatic height shifts
		if ( fabs( dif ) > 8 )
		{
			if ( fabs( dif ) > 16 )
			{
				dif = ( dif < 0 ? -16 : 16 );
			}

			NPC->client->ps.velocity[2] = (NPC->client->ps.velocity[2]+dif)/2;
		}
	}
	else
	{
		gentity_t *goal = NULL;

		if ( NPCInfo->goalEntity )	// Is there a goal?
		{
			goal = NPCInfo->goalEntity;
		}
		else
		{
			goal = NPCInfo->lastGoalEntity;
		}
		if ( goal )
		{
			dif = goal->r.currentOrigin[2] - NPC->r.currentOrigin[2];

			if ( fabs( dif ) > 24 )
			{
				ucmd.upmove = ( ucmd.upmove < 0 ? -4 : 4 );
			}
			else
			{
				if ( NPC->client->ps.velocity[2] )
				{
					NPC->client->ps.velocity[2] *= VELOCITY_DECAY;

					if ( fabs( NPC->client->ps.velocity[2] ) < 2 )
					{
						NPC->client->ps.velocity[2] = 0;
					}
				}
			}
		}
		// Apply friction
		else if ( NPC->client->ps.velocity[2] )
		{
			NPC->client->ps.velocity[2] *= VELOCITY_DECAY;

			if ( fabs( NPC->client->ps.velocity[2] ) < 1 )
			{
				NPC->client->ps.velocity[2] = 0;
			}
		}

		// Stay at a given height until we take on an enemy
/*		VectorSet( endPos, NPC->r.currentOrigin[0], NPC->r.currentOrigin[1], NPC->r.currentOrigin[2] - 512 );
		trap_Trace( &trace, NPC->r.currentOrigin, NULL, NULL, endPos, NPC->s.number, MASK_SOLID );

		if ( trace.fraction != 1.0f )
		{
			float	length = ( trace.fraction * 512 );

			if ( length < 80 )
			{
				ucmd.upmove = 32;
			}
			else if ( length > 120 )
			{
				ucmd.upmove = -32;
			}
			else
			{ 
				if ( NPC->client->ps.velocity[2] )
				{
					NPC->client->ps.velocity[2] *= VELOCITY_DECAY;

					if ( fabs( NPC->client->ps.velocity[2] ) < 1 )
					{
						NPC->client->ps.velocity[2] = 0;
					}
				}
			}
		} */
	}

	// Apply friction
	if ( NPC->client->ps.velocity[0] )
	{
		NPC->client->ps.velocity[0] *= VELOCITY_DECAY;

		if ( fabs( NPC->client->ps.velocity[0] ) < 1 )
		{
			NPC->client->ps.velocity[0] = 0;
		}
	}

	if ( NPC->client->ps.velocity[1] )
	{
		NPC->client->ps.velocity[1] *= VELOCITY_DECAY;

		if ( fabs( NPC->client->ps.velocity[1] ) < 1 )
		{
			NPC->client->ps.velocity[1] = 0;
		}
	}
}

/*
-------------------------
ImperialProbe_Strafe
-------------------------
*/

#define HUNTER_STRAFE_VEL	256
#define HUNTER_STRAFE_DIS	200
#define HUNTER_UPWARD_PUSH	32

void ImperialProbe_Strafe( void )
{
	int		dir;
	vec3_t	end, right;
	trace_t	tr;

	AngleVectors( NPC->client->renderInfo.eyeAngles, NULL, right, NULL );

	// Pick a random strafe direction, then check to see if doing a strafe would be
	//	reasonable valid
	dir = ( rand() & 1 ) ? -1 : 1;
	VectorMA( NPC->r.currentOrigin, HUNTER_STRAFE_DIS * dir, right, end );

	trap_Trace( &tr, NPC->r.currentOrigin, NULL, NULL, end, NPC->s.number, MASK_SOLID );

	// Close enough
	if ( tr.fraction > 0.9f )
	{
		VectorMA( NPC->client->ps.velocity, HUNTER_STRAFE_VEL * dir, right, NPC->client->ps.velocity );

		// Add a slight upward push
		NPC->client->ps.velocity[2] += HUNTER_UPWARD_PUSH;

		// Set the strafe start time so we can do a controlled roll
		//NPC->fx_time = level.time;
		NPCInfo->standTime = level.time + 3000 + random() * 500;
	}
}

/*
-------------------------
ImperialProbe_Hunt
-------------------------`
*/

#define HUNTER_FORWARD_BASE_SPEED	10
#define HUNTER_FORWARD_MULTIPLIER	5

void ImperialProbe_Hunt( qboolean visible, qboolean advance )
{
	float	distance, speed;
	vec3_t	forward;

	NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_RUN1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );

	//If we're not supposed to stand still, pursue the player
	if ( NPCInfo->standTime < level.time )
	{
		// Only strafe when we can see the player
		if ( visible )
		{
			ImperialProbe_Strafe();
			return;
		}
	}

	//If we don't want to advance, stop here
	if ( advance == qfalse )
		return;

	//Only try and navigate if the player is visible
	if ( visible == qfalse )
	{
		// Move towards our goal
		NPCInfo->goalEntity = NPC->enemy;
		NPCInfo->goalRadius = 12;

		//Get our direction from the navigator if we can't see our target
		if ( NPC_GetMoveDirection( forward, &distance ) == qfalse )
			return;
	}
	else
	{
		VectorSubtract( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin, forward );
		distance = VectorNormalize( forward );
	}

	speed = HUNTER_FORWARD_BASE_SPEED + HUNTER_FORWARD_MULTIPLIER * g_spskill.integer;
	VectorMA( NPC->client->ps.velocity, speed, forward, NPC->client->ps.velocity );
}

/*
-------------------------
ImperialProbe_FireBlaster
-------------------------
*/
void ImperialProbe_FireBlaster(void)
{
	vec3_t	muzzle1,enemy_org1,delta1,angleToEnemy1;
	static	vec3_t	forward, vright, up;
	static	vec3_t	muzzle;
	int genBolt1;
	gentity_t	*missile;
	mdxaBone_t	boltMatrix;

	genBolt1 = trap_G2API_AddBolt(NPC->ghoul2, 0, "*flash");

	//FIXME: use {0, NPC->client->ps.legsYaw, 0}
	trap_G2API_GetBoltMatrix( NPC->ghoul2, 0, 
				genBolt1,
				&boltMatrix, NPC->r.currentAngles, NPC->r.currentOrigin, level.time,
				NULL, NPC->modelScale );

	BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, muzzle1 );

	G_PlayEffectID( G_EffectIndex("bryar/muzzle_flash"), muzzle1, vec3_origin );

	G_Sound( NPC, CHAN_AUTO, G_SoundIndex( "sound/chars/probe/misc/fire" ));

	if (NPC->health)
	{
		CalcEntitySpot( NPC->enemy, SPOT_CHEST, enemy_org1 );
		enemy_org1[0]+= Q_irand(0,10);
		enemy_org1[1]+= Q_irand(0,10);
		VectorSubtract (enemy_org1, muzzle1, delta1);
		vectoangles ( delta1, angleToEnemy1 );
		AngleVectors (angleToEnemy1, forward, vright, up);
	}
	else
	{
		AngleVectors (NPC->r.currentAngles, forward, vright, up);
	}

	missile = CreateMissile( muzzle1, forward, 1600, 10000, NPC, qfalse );

	missile->classname = "bryar_proj";
	missile->s.weapon = WP_BRYAR_PISTOL;

	if ( g_spskill.integer <= 1 )
	{
		missile->damage = 5;
	}
	else 
	{
		missile->damage = 10;
	}


	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_UNKNOWN;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

}

/*
-------------------------
ImperialProbe_Ranged
-------------------------
*/
void ImperialProbe_Ranged( qboolean visible, qboolean advance )
{
	int	delay_min,delay_max;

	if ( TIMER_Done( NPC, "attackDelay" ) )	// Attack?
	{

		if ( g_spskill.integer == 0 )
		{
			delay_min = 500;
			delay_max = 3000;
		}
		else if ( g_spskill.integer > 1 )
		{
			delay_min = 500;
			delay_max = 2000;
		}
		else
		{
			delay_min = 300;
			delay_max = 1500;
		}

		TIMER_Set( NPC, "attackDelay", Q_irand( 500, 3000 ) );
		ImperialProbe_FireBlaster();
//		ucmd.buttons |= BUTTON_ATTACK;
	}

	if ( NPCInfo->scriptFlags & SCF_CHASE_ENEMIES )
	{
		ImperialProbe_Hunt( visible, advance );
	}
}

/*
-------------------------
ImperialProbe_AttackDecision
-------------------------
*/

#define	MIN_MELEE_RANGE		320
#define	MIN_MELEE_RANGE_SQR	( MIN_MELEE_RANGE * MIN_MELEE_RANGE )

#define MIN_DISTANCE		128
#define MIN_DISTANCE_SQR	( MIN_DISTANCE * MIN_DISTANCE )

void ImperialProbe_AttackDecision( void )
{
	float		distance;	
	qboolean	visible;
	qboolean	advance;

	// Always keep a good height off the ground
	ImperialProbe_MaintainHeight();

	//randomly talk
	if ( TIMER_Done(NPC,"patrolNoise") )
	{
		if (TIMER_Done(NPC,"angerNoise"))
		{
			G_SoundOnEnt( NPC, CHAN_AUTO, va("sound/chars/probe/misc/probetalk%d", Q_irand(1, 3)) );

			TIMER_Set( NPC, "patrolNoise", Q_irand( 4000, 10000 ) );
		}
	}

	// If we don't have an enemy, just idle
	if ( NPC_CheckEnemyExt(qfalse) == qfalse )
	{
		ImperialProbe_Idle();
		return;
	}

	NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_RUN1, SETANIM_FLAG_NORMAL);

	// Rate our distance to the target, and our visibilty
	distance	= (int) DistanceHorizontalSquared( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin );	
	visible		= NPC_ClearLOS4( NPC->enemy );
	advance		= (qboolean)(distance > MIN_DISTANCE_SQR);

	// If we cannot see our target, move to see it
	if ( visible == qfalse )
	{
		if ( NPCInfo->scriptFlags & SCF_CHASE_ENEMIES )
		{
			ImperialProbe_Hunt( visible, advance );
			return;
		}
	}

	// Sometimes I have problems with facing the enemy I'm attacking, so force the issue so I don't look dumb
	NPC_FaceEnemy( qtrue );

	// Decide what type of attack to do
	ImperialProbe_Ranged( visible, advance );
}

/*
-------------------------
NPC_BSDroid_Pain
-------------------------
*/
void NPC_Probe_Pain(gentity_t *self, gentity_t *attacker, int damage)
{
	float	pain_chance;
	gentity_t *other = attacker;
	int mod = gPainMOD;
	
	VectorCopy( self->NPC->lastPathAngles, self->s.angles );

	if ( self->health < 30 || mod == MOD_DEMP2 || mod == MOD_DEMP2_ALT ) // demp2 always messes them up real good
	{
		vec3_t endPos;
		trace_t	trace;

		VectorSet( endPos, self->r.currentOrigin[0], self->r.currentOrigin[1], self->r.currentOrigin[2] - 128 );
		trap_Trace( &trace, self->r.currentOrigin, NULL, NULL, endPos, self->s.number, MASK_SOLID );

		if ( trace.fraction == 1.0f || mod == MOD_DEMP2 ) // demp2 always does this
		{
			/*
			if (self->client->clientInfo.headModel != 0)
			{
				vec3_t origin;

				VectorCopy(self->r.currentOrigin,origin);
				origin[2] +=50;
//				G_PlayEffect( "small_chunks", origin );
				G_PlayEffect( "chunks/probehead", origin );
				G_PlayEffect( "env/med_explode2", origin );
				self->client->clientInfo.headModel = 0;
				self->client->moveType = MT_RUNJUMP;
				self->client->ps.gravity = g_gravity->value*.1;
			}
			*/
			
			if ( (mod == MOD_DEMP2 || mod == MOD_DEMP2_ALT) && other )
			{
				vec3_t dir;

				NPC_SetAnim( self, SETANIM_BOTH, BOTH_PAIN1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);

				VectorSubtract( self->r.currentOrigin, other->r.currentOrigin, dir );
				VectorNormalize( dir );

				VectorMA( self->client->ps.velocity, 550, dir, self->client->ps.velocity );
				self->client->ps.velocity[2] -= 127;
			}

			//self->s.powerups |= ( 1 << PW_SHOCKED );
			//self->client->ps.powerups[PW_SHOCKED] = level.time + 3000;
			self->client->ps.electrifyTime = level.time + 3000;

			self->NPC->localState = LSTATE_DROP;
		} 
	}
	else
	{
		pain_chance = NPC_GetPainChance( self, damage );

		if ( random() < pain_chance )	// Spin around in pain?
		{
			NPC_SetAnim( self, SETANIM_BOTH, BOTH_PAIN1, SETANIM_FLAG_OVERRIDE);
		}	
	}

	NPC_Pain( self, attacker, damage );
}

/*
-------------------------
ImperialProbe_Idle
-------------------------
*/

void ImperialProbe_Idle( void )
{
	ImperialProbe_MaintainHeight();

	NPC_BSIdle();
}

/*
-------------------------
NPC_BSImperialProbe_Patrol
-------------------------
*/
void ImperialProbe_Patrol( void )
{
	ImperialProbe_MaintainHeight();

	if ( NPC_CheckPlayerTeamStealth() )
	{
		NPC_UpdateAngles( qtrue, qtrue );
		return;
	}

	//If we have somewhere to go, then do that
	if (!NPC->enemy)
	{
		NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_RUN1, SETANIM_FLAG_NORMAL );

		if ( UpdateGoal() )
		{
			//start loop sound once we move
			NPC->s.loopSound = G_SoundIndex( "sound/chars/probe/misc/probedroidloop" );
			ucmd.buttons |= BUTTON_WALKING;
			NPC_MoveToGoal( qtrue );
		}
		//randomly talk
		if (TIMER_Done(NPC,"patrolNoise"))
		{
			G_SoundOnEnt( NPC, CHAN_AUTO, va("sound/chars/probe/misc/probetalk%d", Q_irand(1, 3)) );

			TIMER_Set( NPC, "patrolNoise", Q_irand( 2000, 4000 ) );
		}
	}
	else	// He's got an enemy. Make him angry.
	{
		G_SoundOnEnt( NPC, CHAN_AUTO, "sound/chars/probe/misc/anger1" );
		TIMER_Set( NPC, "angerNoise", Q_irand( 2000, 4000 ) );
		//NPCInfo->behaviorState = BS_HUNT_AND_KILL;
	}

	NPC_UpdateAngles( qtrue, qtrue );
}

/*
-------------------------
ImperialProbe_Wait
-------------------------
*/
void ImperialProbe_Wait(void)
{
	if ( NPCInfo->localState == LSTATE_DROP )
	{
		vec3_t endPos;
		trace_t	trace;

		NPCInfo->desiredYaw = AngleNormalize360( NPCInfo->desiredYaw + 25 );

		VectorSet( endPos, NPC->r.currentOrigin[0], NPC->r.currentOrigin[1], NPC->r.currentOrigin[2] - 32 );
		trap_Trace( &trace, NPC->r.currentOrigin, NULL, NULL, endPos, NPC->s.number, MASK_SOLID );

		if ( trace.fraction != 1.0f )
		{
			G_Damage(NPC, NPC->enemy, NPC->enemy, NULL, NULL, 2000, 0,MOD_UNKNOWN); 
		} 
	}

	NPC_UpdateAngles( qtrue, qtrue );
}

/*
-------------------------
NPC_BSImperialProbe_Default
-------------------------
*/
void NPC_BSImperialProbe_Default( void )
{

	if ( NPC->enemy )
	{
		NPCInfo->goalEntity = NPC->enemy;
		ImperialProbe_AttackDecision();
	}
	else if ( NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES )
	{
		ImperialProbe_Patrol();
	}
	else if ( NPCInfo->localState == LSTATE_DROP )
	{
		ImperialProbe_Wait();
	}
	else
	{
		ImperialProbe_Idle();
	}
}
