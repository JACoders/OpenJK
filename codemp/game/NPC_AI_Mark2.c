#include "b_local.h"
#include "g_nav.h"

//#define AMMO_POD_HEALTH				40
#define AMMO_POD_HEALTH				1
#define TURN_OFF					0x00000100

#define VELOCITY_DECAY		0.25
#define MAX_DISTANCE		256
#define MAX_DISTANCE_SQR	( MAX_DISTANCE * MAX_DISTANCE )
#define MIN_DISTANCE		24
#define MIN_DISTANCE_SQR	( MIN_DISTANCE * MIN_DISTANCE )

#include "../namespace_begin.h"
extern gitem_t	*BG_FindItemForAmmo( ammo_t ammo );
#include "../namespace_end.h"

//Local state enums
enum
{
	LSTATE_NONE = 0,
	LSTATE_DROPPINGDOWN,
	LSTATE_DOWN,
	LSTATE_RISINGUP,
};

void NPC_Mark2_Precache( void )
{
	G_SoundIndex( "sound/chars/mark2/misc/mark2_explo" );// blows up on death
	G_SoundIndex( "sound/chars/mark2/misc/mark2_pain" );
	G_SoundIndex( "sound/chars/mark2/misc/mark2_fire" );
	G_SoundIndex( "sound/chars/mark2/misc/mark2_move_lp" );

	G_EffectIndex( "explosions/droidexplosion1" );
	G_EffectIndex( "env/med_explode2" );
	G_EffectIndex( "blaster/smoke_bolton" );
	G_EffectIndex( "bryar/muzzle_flash" );

	RegisterItem( BG_FindItemForWeapon( WP_BRYAR_PISTOL ));
	RegisterItem( BG_FindItemForAmmo( 	AMMO_METAL_BOLTS));
	RegisterItem( BG_FindItemForAmmo( AMMO_POWERCELL ));
	RegisterItem( BG_FindItemForAmmo( AMMO_BLASTER ));
}

/*
-------------------------
NPC_Mark2_Part_Explode
-------------------------
*/
void NPC_Mark2_Part_Explode( gentity_t *self, int bolt )
{
	if ( bolt >=0 )
	{
		mdxaBone_t	boltMatrix;
		vec3_t		org, dir;

		trap_G2API_GetBoltMatrix( self->ghoul2, 0, 
					bolt,
					&boltMatrix, self->r.currentAngles, self->r.currentOrigin, level.time,
					NULL, self->modelScale );

		BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, org );
		BG_GiveMeVectorFromMatrix( &boltMatrix, NEGATIVE_Y, dir );

		G_PlayEffectID( G_EffectIndex("env/med_explode2"), org, dir );
		G_PlayEffectID( G_EffectIndex("blaster/smoke_bolton"), org, dir );
	}

	//G_PlayEffectID( G_EffectIndex("blaster/smoke_bolton"), self->playerModel, bolt, self->s.number);

	self->count++;	// Count of pods blown off
}

/*
-------------------------
NPC_Mark2_Pain
- look at what was hit and see if it should be removed from the model.
-------------------------
*/
void NPC_Mark2_Pain(gentity_t *self, gentity_t *attacker, int damage)
{
	int newBolt,i;
	int hitLoc = gPainHitLoc;
	
	NPC_Pain( self, attacker, damage );

	for (i=0;i<3;i++)
	{
		if ((hitLoc==HL_GENERIC1+i) && (self->locationDamage[HL_GENERIC1+i] > AMMO_POD_HEALTH))	// Blow it up?
		{
			if (self->locationDamage[hitLoc] >= AMMO_POD_HEALTH)
			{			
				newBolt = trap_G2API_AddBolt( self->ghoul2, 0, va("torso_canister%d",(i+1)) );
				if ( newBolt != -1 )
				{
					NPC_Mark2_Part_Explode(self,newBolt);
				}
				NPC_SetSurfaceOnOff( self, va("torso_canister%d",(i+1)), TURN_OFF );
				break;
			}
		}
	}

	G_Sound( self, CHAN_AUTO, G_SoundIndex( "sound/chars/mark2/misc/mark2_pain" ));

	// If any pods were blown off, kill him
	if (self->count > 0)
	{
		G_Damage( self, NULL, NULL, NULL, NULL, self->health, DAMAGE_NO_PROTECTION, MOD_UNKNOWN );
	}
}

/*
-------------------------
Mark2_Hunt
-------------------------
*/
void Mark2_Hunt(void)
{
	if ( NPCInfo->goalEntity == NULL )
	{
		NPCInfo->goalEntity = NPC->enemy;
	}

	// Turn toward him before moving towards him.
	NPC_FaceEnemy( qtrue );

	NPCInfo->combatMove = qtrue;
	NPC_MoveToGoal( qtrue );
}

/*
-------------------------
Mark2_FireBlaster
-------------------------
*/
void Mark2_FireBlaster(qboolean advance)
{
	vec3_t	muzzle1,enemy_org1,delta1,angleToEnemy1;
	static	vec3_t	forward, vright, up;
	static	vec3_t	muzzle;
	gentity_t	*missile;
	mdxaBone_t	boltMatrix;
	int bolt = trap_G2API_AddBolt(NPC->ghoul2, 0, "*flash");

	trap_G2API_GetBoltMatrix( NPC->ghoul2, 0, 
				bolt,
				&boltMatrix, NPC->r.currentAngles, NPC->r.currentOrigin, level.time,
				NULL, NPC->modelScale );

	BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, muzzle1 );

	if (NPC->health)
	{
		CalcEntitySpot( NPC->enemy, SPOT_HEAD, enemy_org1 );
		VectorSubtract (enemy_org1, muzzle1, delta1);
		vectoangles ( delta1, angleToEnemy1 );
		AngleVectors (angleToEnemy1, forward, vright, up);
	}
	else
	{
		AngleVectors (NPC->r.currentAngles, forward, vright, up);
	}

	G_PlayEffectID( G_EffectIndex("bryar/muzzle_flash"), muzzle1, forward );

	G_Sound( NPC, CHAN_AUTO, G_SoundIndex("sound/chars/mark2/misc/mark2_fire"));

	missile = CreateMissile( muzzle1, forward, 1600, 10000, NPC, qfalse );

	missile->classname = "bryar_proj";
	missile->s.weapon = WP_BRYAR_PISTOL;

	missile->damage = 1;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_BRYAR_PISTOL;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

}

/*
-------------------------
Mark2_BlasterAttack
-------------------------
*/
void Mark2_BlasterAttack(qboolean advance)
{
	if ( TIMER_Done( NPC, "attackDelay" ) )	// Attack?
	{
		if (NPCInfo->localState == LSTATE_NONE)	// He's up so shoot less often.
		{
			TIMER_Set( NPC, "attackDelay", Q_irand( 500, 2000) );
		}
		else
		{
			TIMER_Set( NPC, "attackDelay", Q_irand( 100, 500) );
		}
		Mark2_FireBlaster(advance);
		return;
	}
	else if (advance)
	{
		Mark2_Hunt();
	}
}

/*
-------------------------
Mark2_AttackDecision
-------------------------
*/
void Mark2_AttackDecision( void )
{
	float		distance;
	qboolean	visible;
	qboolean	advance;

	NPC_FaceEnemy( qtrue );

	distance	= (int) DistanceHorizontalSquared( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin );	
	visible		= NPC_ClearLOS4( NPC->enemy );
	advance		= (qboolean)(distance > MIN_DISTANCE_SQR);

	// He's been ordered to get up
	if (NPCInfo->localState == LSTATE_RISINGUP)
	{
		NPC->flags &= ~FL_SHIELDED;
		NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_RUN1START, SETANIM_FLAG_HOLD|SETANIM_FLAG_OVERRIDE );
		if ((NPC->client->ps.legsTimer<=0) && 
			NPC->client->ps.torsoAnim == BOTH_RUN1START )
		{
			NPCInfo->localState = LSTATE_NONE;	// He's up again.
		}
		return;
	}

	// If we cannot see our target, move to see it
	if ((!visible) || (!NPC_FaceEnemy(qtrue)))
	{
		// If he's going down or is down, make him get up
		if ((NPCInfo->localState == LSTATE_DOWN) || (NPCInfo->localState == LSTATE_DROPPINGDOWN))
		{
			if ( TIMER_Done( NPC, "downTime" ) )	// Down being down?? (The delay is so he doesn't pop up and down when the player goes in and out of range)
			{
				NPCInfo->localState = LSTATE_RISINGUP;
				NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_RUN1STOP, SETANIM_FLAG_HOLD|SETANIM_FLAG_OVERRIDE );
				TIMER_Set( NPC, "runTime", Q_irand( 3000, 8000) );	// So he runs for a while before testing to see if he should drop down.
			}
		}
		else
		{
			Mark2_Hunt();
		}
		return;
	}

	// He's down but he could advance if he wants to.
	if ((advance) && (TIMER_Done( NPC, "downTime" )) && (NPCInfo->localState == LSTATE_DOWN))
	{
		NPCInfo->localState = LSTATE_RISINGUP;
		NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_RUN1STOP, SETANIM_FLAG_HOLD|SETANIM_FLAG_OVERRIDE );
		TIMER_Set( NPC, "runTime", Q_irand( 3000, 8000) );	// So he runs for a while before testing to see if he should drop down.
	}

	NPC_FaceEnemy( qtrue );

	// Dropping down to shoot
	if (NPCInfo->localState == LSTATE_DROPPINGDOWN)
	{
		NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_RUN1STOP, SETANIM_FLAG_HOLD|SETANIM_FLAG_OVERRIDE );
		TIMER_Set( NPC, "downTime", Q_irand( 3000, 9000) );

		if ((NPC->client->ps.legsTimer<=0) && NPC->client->ps.torsoAnim == BOTH_RUN1STOP )
		{
			NPC->flags |= FL_SHIELDED;
			NPCInfo->localState = LSTATE_DOWN;
		}
	}
	// He's down and shooting
	else if (NPCInfo->localState == LSTATE_DOWN)
	{
		NPC->flags |= FL_SHIELDED;//only damagable by lightsabers and missiles

		Mark2_BlasterAttack(qfalse);
	}
	else if (TIMER_Done( NPC, "runTime" ))	// Lowering down to attack. But only if he's done running at you.
	{
		NPCInfo->localState = LSTATE_DROPPINGDOWN;
	}
	else if (advance)
	{
		// We can see enemy so shoot him if timer lets you.
		Mark2_BlasterAttack(advance);
	}
}


/*
-------------------------
Mark2_Patrol
-------------------------
*/
void Mark2_Patrol( void )
{
	if ( NPC_CheckPlayerTeamStealth() )
	{
//		G_Sound( NPC, G_SoundIndex("sound/chars/mark1/misc/anger.wav"));
		NPC_UpdateAngles( qtrue, qtrue );
		return;
	}

	//If we have somewhere to go, then do that
	if (!NPC->enemy)
	{
		if ( UpdateGoal() )
		{
			ucmd.buttons |= BUTTON_WALKING;
			NPC_MoveToGoal( qtrue );
			NPC_UpdateAngles( qtrue, qtrue );
		}

		//randomly talk
		if (TIMER_Done(NPC,"patrolNoise"))
		{
//			G_Sound( NPC, G_SoundIndex(va("sound/chars/mark1/misc/talk%d.wav",	Q_irand(1, 4))));

			TIMER_Set( NPC, "patrolNoise", Q_irand( 2000, 4000 ) );
		}
	}
}

/*
-------------------------
Mark2_Idle
-------------------------
*/
void Mark2_Idle( void )
{
	NPC_BSIdle();
}

/*
-------------------------
NPC_BSMark2_Default
-------------------------
*/
void NPC_BSMark2_Default( void )
{
	if ( NPC->enemy )
	{
		NPCInfo->goalEntity = NPC->enemy;
		Mark2_AttackDecision();
	}
	else if ( NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES )
	{
		Mark2_Patrol();
	}
	else
	{
		Mark2_Idle();
	}
}
