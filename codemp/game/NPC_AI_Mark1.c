#include "b_local.h"
#include "g_nav.h"

#define	MIN_MELEE_RANGE				320
#define	MIN_MELEE_RANGE_SQR			( MIN_MELEE_RANGE * MIN_MELEE_RANGE )

#define MIN_DISTANCE				128
#define MIN_DISTANCE_SQR			( MIN_DISTANCE * MIN_DISTANCE )

#define TURN_OFF					0x00000100

#define LEFT_ARM_HEALTH				40
#define RIGHT_ARM_HEALTH			40
#define AMMO_POD_HEALTH				40

#define	BOWCASTER_VELOCITY			1300
#define	BOWCASTER_NPC_DAMAGE_EASY	12
#define	BOWCASTER_NPC_DAMAGE_NORMAL	24
#define	BOWCASTER_NPC_DAMAGE_HARD	36
#define BOWCASTER_SIZE				2
#define BOWCASTER_SPLASH_DAMAGE		0
#define BOWCASTER_SPLASH_RADIUS		0

//Local state enums
enum
{
	LSTATE_NONE = 0,
	LSTATE_ASLEEP,
	LSTATE_WAKEUP,
	LSTATE_FIRED0,
	LSTATE_FIRED1,
	LSTATE_FIRED2,
	LSTATE_FIRED3,
	LSTATE_FIRED4,
};

qboolean NPC_CheckPlayerTeamStealth( void );
void Mark1_BlasterAttack(qboolean advance);
void DeathFX( gentity_t *ent );

#include "../namespace_begin.h"
extern gitem_t *BG_FindItemForAmmo( ammo_t ammo );
#include "../namespace_end.h"

/*
-------------------------
NPC_Mark1_Precache
-------------------------
*/
void NPC_Mark1_Precache(void)
{
	G_SoundIndex( "sound/chars/mark1/misc/mark1_wakeup");
	G_SoundIndex( "sound/chars/mark1/misc/shutdown");
	G_SoundIndex( "sound/chars/mark1/misc/walk");
	G_SoundIndex( "sound/chars/mark1/misc/run");
	G_SoundIndex( "sound/chars/mark1/misc/death1");
	G_SoundIndex( "sound/chars/mark1/misc/death2");
	G_SoundIndex( "sound/chars/mark1/misc/anger");
	G_SoundIndex( "sound/chars/mark1/misc/mark1_fire");
	G_SoundIndex( "sound/chars/mark1/misc/mark1_pain");
	G_SoundIndex( "sound/chars/mark1/misc/mark1_explo");

//	G_EffectIndex( "small_chunks");
	G_EffectIndex( "env/med_explode2");
	G_EffectIndex( "explosions/probeexplosion1");
	G_EffectIndex( "blaster/smoke_bolton");
	G_EffectIndex( "bryar/muzzle_flash");
	G_EffectIndex( "explosions/droidexplosion1" );

	RegisterItem( BG_FindItemForAmmo( 	AMMO_METAL_BOLTS));
	RegisterItem( BG_FindItemForAmmo( AMMO_BLASTER ));
	RegisterItem( BG_FindItemForWeapon( WP_BOWCASTER ));
	RegisterItem( BG_FindItemForWeapon( WP_BRYAR_PISTOL ));
}

/*
-------------------------
NPC_Mark1_Part_Explode
-------------------------
*/
void NPC_Mark1_Part_Explode( gentity_t *self, int bolt )
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

	//G_PlayEffectID( G_EffectIndex("blaster/smoke_bolton"), self->playerModel, bolt, self->s.number );
}

/*
-------------------------
Mark1_Idle
-------------------------
*/
void Mark1_Idle( void )
{

	NPC_BSIdle();

	NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_SLEEP1, SETANIM_FLAG_NORMAL );
}

/*
-------------------------
Mark1Dead_FireRocket
- Shoot the left weapon, the multi-blaster
-------------------------
*/
void Mark1Dead_FireRocket (void)
{
	mdxaBone_t	boltMatrix;
	vec3_t	muzzle1,muzzle_dir;
	gentity_t *missile;

	int	damage	= 50;
	int bolt = trap_G2API_AddBolt(NPC->ghoul2, 0, "*flash5");

	trap_G2API_GetBoltMatrix( NPC->ghoul2, 0, 
				bolt,
				&boltMatrix, NPC->r.currentAngles, NPC->r.currentOrigin, level.time,
				NULL, NPC->modelScale );

	BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, muzzle1 );
	BG_GiveMeVectorFromMatrix( &boltMatrix, NEGATIVE_Y, muzzle_dir );

	G_PlayEffectID( G_EffectIndex("bryar/muzzle_flash"), muzzle1, muzzle_dir );

	G_Sound( NPC, CHAN_AUTO, G_SoundIndex("sound/chars/mark1/misc/mark1_fire"));

	missile = CreateMissile( muzzle1, muzzle_dir, BOWCASTER_VELOCITY, 10000, NPC, qfalse );

	missile->classname = "bowcaster_proj";
	missile->s.weapon = WP_BOWCASTER;

	VectorSet( missile->r.maxs, BOWCASTER_SIZE, BOWCASTER_SIZE, BOWCASTER_SIZE );
	VectorScale( missile->r.maxs, -1, missile->r.mins );

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	//missile->methodOfDeath = MOD_ENERGY;
	missile->methodOfDeath = MOD_ROCKET;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
	missile->splashDamage = BOWCASTER_SPLASH_DAMAGE;
	missile->splashRadius = BOWCASTER_SPLASH_RADIUS;

	// we don't want it to bounce
	missile->bounceCount = 0;

}

/*
-------------------------
Mark1Dead_FireBlaster
- Shoot the left weapon, the multi-blaster
-------------------------
*/
void Mark1Dead_FireBlaster (void)
{
	vec3_t	muzzle1,muzzle_dir;
	gentity_t	*missile;
	mdxaBone_t	boltMatrix;
	int			bolt;

	bolt = trap_G2API_AddBolt(NPC->ghoul2, 0, "*flash1"); 

	trap_G2API_GetBoltMatrix( NPC->ghoul2, 0, 
				bolt,
				&boltMatrix, NPC->r.currentAngles, NPC->r.currentOrigin, level.time,
				NULL, NPC->modelScale );

	BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, muzzle1 );
	BG_GiveMeVectorFromMatrix( &boltMatrix, NEGATIVE_Y, muzzle_dir );

	G_PlayEffectID( G_EffectIndex("bryar/muzzle_flash"), muzzle1, muzzle_dir );

	missile = CreateMissile( muzzle1, muzzle_dir, 1600, 10000, NPC, qfalse );

	G_Sound( NPC, CHAN_AUTO, G_SoundIndex("sound/chars/mark1/misc/mark1_fire"));

	missile->classname = "bryar_proj";
	missile->s.weapon = WP_BRYAR_PISTOL;

	missile->damage = 1;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_BRYAR_PISTOL;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

}

/*
-------------------------
Mark1_die
-------------------------
*/
void Mark1_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod,int dFlags,int hitLoc )
{
	/*
	int	anim;

	// Is he dead already?
	anim = self->client->ps.legsAnim;
	if (((anim==BOTH_DEATH1) || (anim==BOTH_DEATH2)) && (self->client->ps.torsoTimer<=0)) 
	{	// This is because self->health keeps getting zeroed out. HL_NONE acts as health in this case.
		self->locationDamage[HL_NONE] += damage;
		if (self->locationDamage[HL_NONE] > 50)
		{
			DeathFX(self);
			self->client->ps.eFlags |= EF_NODRAW;
			self->contents = CONTENTS_CORPSE;
			// G_FreeEntity( self ); // Is this safe?  I can't see why we'd mark it nodraw and then just leave it around??
			self->e_ThinkFunc = thinkF_G_FreeEntity;
			self->nextthink = level.time + FRAMETIME;
		}
		return;
	}
	*/

	G_Sound( self, CHAN_AUTO, G_SoundIndex(va("sound/chars/mark1/misc/death%d.wav",Q_irand( 1, 2))));

	// Choose a death anim
	if (Q_irand( 1, 10) > 5)
	{
		NPC_SetAnim( self, SETANIM_BOTH, BOTH_DEATH2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	}
	else
	{
		NPC_SetAnim( self, SETANIM_BOTH, BOTH_DEATH1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	}
}

/*
-------------------------
Mark1_dying
-------------------------
*/
void Mark1_dying( gentity_t *self )
{
	int	num,newBolt;

	if (self->client->ps.torsoTimer>0)
	{
		if (TIMER_Done(self,"dyingExplosion"))
		{
			num = Q_irand( 1, 3);

			// Find place to generate explosion
			if (num == 1)
			{
				num = Q_irand( 8, 10);
				newBolt = trap_G2API_AddBolt( self->ghoul2, 0, va("*flash%d",num) );
				NPC_Mark1_Part_Explode(self,newBolt);
			}
			else
			{
				num = Q_irand( 1, 6);
				newBolt = trap_G2API_AddBolt( self->ghoul2, 0, va("*torso_tube%d",num) );
				NPC_Mark1_Part_Explode(self,newBolt);
				NPC_SetSurfaceOnOff( self, va("torso_tube%d",num), TURN_OFF );
			}

			TIMER_Set( self, "dyingExplosion", Q_irand( 300, 1000 ) );
		}


//		int		dir;
//		vec3_t	right;

		// Shove to the side
//		AngleVectors( self->client->renderInfo.eyeAngles, NULL, right, NULL );
//		VectorMA( self->client->ps.velocity, -80, right, self->client->ps.velocity );

		// See which weapons are there
		// Randomly fire blaster
		if (!trap_G2API_GetSurfaceRenderStatus( self->ghoul2, 0, "l_arm" ))	// Is the blaster still on the model?
		{
			if (Q_irand( 1, 5) == 1)
			{
				SaveNPCGlobals();
				SetNPCGlobals( self );
				Mark1Dead_FireBlaster();
				RestoreNPCGlobals();
			}
		}

		// Randomly fire rocket
		if (!trap_G2API_GetSurfaceRenderStatus( self->ghoul2, 0, "r_arm" ))	// Is the rocket still on the model?
		{
			if (Q_irand( 1, 10) == 1)
			{
				SaveNPCGlobals();
				SetNPCGlobals( self );
				Mark1Dead_FireRocket();
				RestoreNPCGlobals();
			}
		}
	}

}

/*
-------------------------
NPC_Mark1_Pain
- look at what was hit and see if it should be removed from the model.
-------------------------
*/
void NPC_Mark1_Pain(gentity_t *self, gentity_t *attacker, int damage)
{
	int newBolt,i,chance;
	int hitLoc = gPainHitLoc;
	
	NPC_Pain( self, attacker, damage );

	G_Sound( self, CHAN_AUTO, G_SoundIndex("sound/chars/mark1/misc/mark1_pain"));

	// Hit in the CHEST???
	if (hitLoc==HL_CHEST)
	{
		chance = Q_irand( 1, 4);
	
		if ((chance == 1) && (damage > 5))
		{
			NPC_SetAnim( self, SETANIM_BOTH, BOTH_PAIN1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		}
	}
	// Hit in the left arm?
	else if ((hitLoc==HL_ARM_LT) && (self->locationDamage[HL_ARM_LT] > LEFT_ARM_HEALTH))
	{
		if (self->locationDamage[hitLoc] >= LEFT_ARM_HEALTH)	// Blow it up?
		{
			newBolt = trap_G2API_AddBolt( self->ghoul2, 0, "*flash3" );
			if ( newBolt != -1 )
			{
				NPC_Mark1_Part_Explode(self,newBolt);
			}

			NPC_SetSurfaceOnOff( self, "l_arm", TURN_OFF );
		}
	}
	// Hit in the right arm?
	else if ((hitLoc==HL_ARM_RT) && (self->locationDamage[HL_ARM_RT] > RIGHT_ARM_HEALTH))	// Blow it up?
	{
		if (self->locationDamage[hitLoc] >= RIGHT_ARM_HEALTH)
		{			
			newBolt = trap_G2API_AddBolt( self->ghoul2, 0, "*flash4" );
			if ( newBolt != -1 )
			{
//				G_PlayEffect( "small_chunks", self->playerModel, self->genericBolt2, self->s.number);
				NPC_Mark1_Part_Explode( self, newBolt );
			}

			NPC_SetSurfaceOnOff( self, "r_arm", TURN_OFF );
		}
	}
	// Check ammo pods
	else
	{
		for (i=0;i<6;i++)
		{
			if ((hitLoc==HL_GENERIC1+i) && (self->locationDamage[HL_GENERIC1+i] > AMMO_POD_HEALTH))	// Blow it up?
			{
				if (self->locationDamage[hitLoc] >= AMMO_POD_HEALTH)
				{			
					newBolt = trap_G2API_AddBolt( self->ghoul2, 0, va("*torso_tube%d",(i+1)) );
					if ( newBolt != -1 )
					{
						NPC_Mark1_Part_Explode(self,newBolt);
					}
					NPC_SetSurfaceOnOff( self, va("torso_tube%d",(i+1)), TURN_OFF );
					NPC_SetAnim( self, SETANIM_BOTH, BOTH_PAIN1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
					break;
				}
			}
		}
	}

	// Are both guns shot off?
	if ((trap_G2API_GetSurfaceRenderStatus( self->ghoul2, 0, "l_arm" )>0) &&
		(trap_G2API_GetSurfaceRenderStatus( self->ghoul2, 0, "r_arm" )>0))
	{
		G_Damage(self,NULL,NULL,NULL,NULL,self->health,0,MOD_UNKNOWN);
	}
}

/*
-------------------------
Mark1_Hunt
- look for enemy.
-------------------------`
*/
void Mark1_Hunt(void)
{

	if ( NPCInfo->goalEntity == NULL )
	{
		NPCInfo->goalEntity = NPC->enemy;
	}

	NPC_FaceEnemy( qtrue );

	NPCInfo->combatMove = qtrue;
	NPC_MoveToGoal( qtrue );
}

/*
-------------------------
Mark1_FireBlaster
- Shoot the left weapon, the multi-blaster
-------------------------
*/
void Mark1_FireBlaster(void)
{
	vec3_t	muzzle1,enemy_org1,delta1,angleToEnemy1;
	static	vec3_t	forward, vright, up;
	static	vec3_t	muzzle;
	gentity_t	*missile;
	mdxaBone_t	boltMatrix;
	int			bolt;

	// Which muzzle to fire from?
	if ((NPCInfo->localState <= LSTATE_FIRED0) || (NPCInfo->localState == LSTATE_FIRED4)) 
	{
		NPCInfo->localState = LSTATE_FIRED1;
		bolt = trap_G2API_AddBolt(NPC->ghoul2, 0, "*flash1"); 
	}
	else if (NPCInfo->localState == LSTATE_FIRED1)
	{
		NPCInfo->localState = LSTATE_FIRED2;
		bolt = trap_G2API_AddBolt(NPC->ghoul2, 0, "*flash2"); 
	}
	else if (NPCInfo->localState == LSTATE_FIRED2)
	{
		NPCInfo->localState = LSTATE_FIRED3;
		bolt = trap_G2API_AddBolt(NPC->ghoul2, 0, "*flash3"); 
	}
	else
	{
		NPCInfo->localState = LSTATE_FIRED4;
		bolt = trap_G2API_AddBolt(NPC->ghoul2, 0, "*flash4"); 
	}

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

	G_Sound( NPC, CHAN_AUTO, G_SoundIndex("sound/chars/mark1/misc/mark1_fire"));

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
Mark1_BlasterAttack
-------------------------
*/
void Mark1_BlasterAttack(qboolean advance )
{
	int chance;

	if ( TIMER_Done( NPC, "attackDelay" ) )	// Attack?
	{
		chance = Q_irand( 1, 5);
	
		NPCInfo->burstCount++;

		if (NPCInfo->burstCount<3)	// Too few shots this burst?
		{
			chance = 2;				// Force it to keep firing.
		}
		else if (NPCInfo->burstCount>12)	// Too many shots fired this burst?
		{
			NPCInfo->burstCount = 0;
			chance = 1;				// Force it to stop firing.
		}

		// Stop firing.
		if (chance == 1)
		{
			NPCInfo->burstCount = 0;
			TIMER_Set( NPC, "attackDelay", Q_irand( 1000, 3000) );
			NPC->client->ps.torsoTimer=0;						// Just in case the firing anim is running.
		}
		else
		{
			if (TIMER_Done( NPC, "attackDelay2" ))	// Can't be shooting every frame.
			{
				TIMER_Set( NPC, "attackDelay2", Q_irand( 50, 50) );
				Mark1_FireBlaster();
 				NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_ATTACK1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			}
			return;
		}
	}
	else if (advance)
	{
		if ( NPC->client->ps.torsoAnim == BOTH_ATTACK1 )
		{
			NPC->client->ps.torsoTimer=0;						// Just in case the firing anim is running.
		}
		Mark1_Hunt();
	}
	else	// Make sure he's not firing.
	{
		if ( NPC->client->ps.torsoAnim == BOTH_ATTACK1 )
		{
			NPC->client->ps.torsoTimer=0;						// Just in case the firing anim is running.
		}
	}
}

/*
-------------------------
Mark1_FireRocket
-------------------------
*/
void Mark1_FireRocket(void)
{
	mdxaBone_t	boltMatrix;
	vec3_t	muzzle1,enemy_org1,delta1,angleToEnemy1;
	static	vec3_t	forward, vright, up;
	int bolt = trap_G2API_AddBolt(NPC->ghoul2, 0, "*flash5");
	gentity_t *missile;

	int	damage	= 50;

	trap_G2API_GetBoltMatrix( NPC->ghoul2, 0, 
				bolt,
				&boltMatrix, NPC->r.currentAngles, NPC->r.currentOrigin, level.time,
				NULL, NPC->modelScale );

	BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, muzzle1 );

//	G_PlayEffect( "blaster/muzzle_flash", muzzle1 );

	CalcEntitySpot( NPC->enemy, SPOT_HEAD, enemy_org1 );
	VectorSubtract (enemy_org1, muzzle1, delta1);
	vectoangles ( delta1, angleToEnemy1 );
	AngleVectors (angleToEnemy1, forward, vright, up);

	G_Sound( NPC, CHAN_AUTO, G_SoundIndex("sound/chars/mark1/misc/mark1_fire" ));

	missile = CreateMissile( muzzle1, forward, BOWCASTER_VELOCITY, 10000, NPC, qfalse );

	missile->classname = "bowcaster_proj";
	missile->s.weapon = WP_BOWCASTER;

	VectorSet( missile->r.maxs, BOWCASTER_SIZE, BOWCASTER_SIZE, BOWCASTER_SIZE );
	VectorScale( missile->r.maxs, -1, missile->r.mins );

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_ROCKET;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
	missile->splashDamage = BOWCASTER_SPLASH_DAMAGE;
	missile->splashRadius = BOWCASTER_SPLASH_RADIUS;

	// we don't want it to bounce
	missile->bounceCount = 0;

}

/*
-------------------------
Mark1_RocketAttack
-------------------------
*/
void Mark1_RocketAttack( qboolean advance )
{
	if ( TIMER_Done( NPC, "attackDelay" ) )	// Attack?
	{
		TIMER_Set( NPC, "attackDelay", Q_irand( 1000, 3000) );
 		NPC_SetAnim( NPC, SETANIM_TORSO, BOTH_ATTACK2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		Mark1_FireRocket();
	}
	else if (advance)
	{
		Mark1_Hunt();
	}
}

/*
-------------------------
Mark1_AttackDecision
-------------------------
*/
void Mark1_AttackDecision( void )
{
	int blasterTest,rocketTest;
	float		distance;
	distance_e	distRate;
	qboolean	visible;
	qboolean	advance;

	//randomly talk
	if ( TIMER_Done(NPC,"patrolNoise") )
	{
		if (TIMER_Done(NPC,"angerNoise"))
		{
//			G_Sound( NPC, G_SoundIndex(va("sound/chars/mark1/misc/talk%d.wav",	Q_irand(1, 4))));
			TIMER_Set( NPC, "patrolNoise", Q_irand( 4000, 10000 ) );
		}
	}

	// Enemy is dead or he has no enemy.
	if ((NPC->enemy->health<1) || ( NPC_CheckEnemyExt(qfalse) == qfalse ))
	{
		NPC->enemy = NULL;
		return;
	}

	// Rate our distance to the target and visibility
	distance	= (int) DistanceHorizontalSquared( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin );	
	distRate	= ( distance > MIN_MELEE_RANGE_SQR ) ? DIST_LONG : DIST_MELEE;
	visible		= NPC_ClearLOS4( NPC->enemy );
	advance		= (qboolean)(distance > MIN_DISTANCE_SQR);

	// If we cannot see our target, move to see it
	if ((!visible) || (!NPC_FaceEnemy(qtrue)))
	{
		Mark1_Hunt();
		return;
	}

	// See if the side weapons are there
	blasterTest = trap_G2API_GetSurfaceRenderStatus( NPC->ghoul2, 0, "l_arm" );
	rocketTest = trap_G2API_GetSurfaceRenderStatus( NPC->ghoul2, 0, "r_arm" );

	// It has both side weapons
	if (!blasterTest  && !rocketTest)
	{
		;	// So do nothing.
	}
	else if (blasterTest!=-1
		&&blasterTest)
	{
		distRate = DIST_LONG;
	}
	else if (rocketTest!=-1
		&&rocketTest)
	{
		distRate = DIST_MELEE;
	}
	else	// It should never get here, but just in case
	{ 
		NPC->health = 0;
		NPC->client->ps.stats[STAT_HEALTH] = 0;
		//GEntity_DieFunc(NPC, NPC, NPC, 100, MOD_UNKNOWN);
		if (NPC->die)
		{
			NPC->die(NPC, NPC, NPC, 100, MOD_UNKNOWN);
		}
	}

	// We can see enemy so shoot him if timers let you.
	NPC_FaceEnemy( qtrue );

	if (distRate == DIST_MELEE)
	{
		Mark1_BlasterAttack(advance);
	}
	else if (distRate == DIST_LONG)
	{
		Mark1_RocketAttack(advance);
	}
}

/*
-------------------------
Mark1_Patrol
-------------------------
*/
void Mark1_Patrol( void )
{
	if ( NPC_CheckPlayerTeamStealth() )
	{
		G_Sound( NPC, CHAN_AUTO, G_SoundIndex("sound/chars/mark1/misc/mark1_wakeup"));
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
//		if (TIMER_Done(NPC,"patrolNoise"))
//		{
//			G_Sound( NPC, G_SoundIndex(va("sound/chars/mark1/misc/talk%d.wav",	Q_irand(1, 4))));
//
//			TIMER_Set( NPC, "patrolNoise", Q_irand( 2000, 4000 ) );
//		}
	}

}


/*
-------------------------
NPC_BSMark1_Default
-------------------------
*/
void NPC_BSMark1_Default( void )
{
	//NPC->e_DieFunc = dieF_Mark1_die;

	if ( NPC->enemy )
	{
		NPCInfo->goalEntity = NPC->enemy;
		Mark1_AttackDecision();
	}
	else if ( NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES )
	{
		Mark1_Patrol();
	}
	else
	{
		Mark1_Idle();
	}
}
