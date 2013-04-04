// leave this line at the top of all AI_xxxx.cpp files for PCH reasons...
#include "g_headers.h"

	    
#include "b_local.h"

extern void G_GetBoltPosition( gentity_t *self, int boltIndex, vec3_t pos, int modelIndex );

// These define the working combat range for these suckers
#define MIN_DISTANCE		128
#define MIN_DISTANCE_SQR	( MIN_DISTANCE * MIN_DISTANCE )

#define MAX_DISTANCE		1024
#define MAX_DISTANCE_SQR	( MAX_DISTANCE * MAX_DISTANCE )

#define LSTATE_CLEAR		0
#define LSTATE_WAITING		1

void Rancor_SetBolts( gentity_t *self )
{
	if ( self && self->client )
	{
		renderInfo_t *ri = &self->client->renderInfo;
		ri->handRBolt = trap_G2API_AddBolt( self->ghoul2, 0, "*r_hand" );
		ri->handLBolt = trap_G2API_AddBolt( self->ghoul2, 0, "*l_hand" );
		ri->headBolt = trap_G2API_AddBolt( self->ghoul2, 0, "*head_eyes" );
		ri->torsoBolt = trap_G2API_AddBolt( self->ghoul2, 0, "jaw_bone" );
	}
}

/*
-------------------------
NPC_Rancor_Precache
-------------------------
*/
void NPC_Rancor_Precache( void )
{
	int i;
	for ( i = 1; i < 3; i ++ )
	{
		G_SoundIndex( va("sound/chars/rancor/snort_%d.wav", i) );
	}
	G_SoundIndex( "sound/chars/rancor/swipehit.wav" );
	G_SoundIndex( "sound/chars/rancor/chomp.wav" );
}


/*
-------------------------
Rancor_Idle
-------------------------
*/
void Rancor_Idle( void )
{
	NPCInfo->localState = LSTATE_CLEAR;

	//If we have somewhere to go, then do that
	if ( UpdateGoal() )
	{
		ucmd.buttons &= ~BUTTON_WALKING;
		NPC_MoveToGoal( qtrue );
	}
}


qboolean Rancor_CheckRoar( gentity_t *self )
{
	if ( !self->wait )
	{//haven't ever gotten mad yet
		self->wait = 1;//do this only once
		self->client->ps.eFlags2 |= EF2_ALERTED;
		NPC_SetAnim( self, SETANIM_BOTH, BOTH_STAND1TO2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
		TIMER_Set( self, "rageTime", self->client->ps.legsTimer );
		return qtrue;
	}
	return qfalse;
}
/*
-------------------------
Rancor_Patrol
-------------------------
*/
void Rancor_Patrol( void )
{
	NPCInfo->localState = LSTATE_CLEAR;

	//If we have somewhere to go, then do that
	if ( UpdateGoal() )
	{
		ucmd.buttons &= ~BUTTON_WALKING;
		NPC_MoveToGoal( qtrue );
	}
	else
	{
		if ( TIMER_Done( NPC, "patrolTime" ))
		{
			TIMER_Set( NPC, "patrolTime", crandom() * 5000 + 5000 );
		}
	}

	if ( NPC_CheckEnemyExt( qtrue ) == qfalse )
	{
		Rancor_Idle();
		return;
	}
	Rancor_CheckRoar( NPC );
	TIMER_Set( NPC, "lookForNewEnemy", Q_irand( 5000, 15000 ) );
}
 
/*
-------------------------
Rancor_Move
-------------------------
*/
void Rancor_Move( qboolean visible )
{
	if ( NPCInfo->localState != LSTATE_WAITING )
	{
		NPCInfo->goalEntity = NPC->enemy;
		if ( !NPC_MoveToGoal( qtrue ) )
		{
			NPCInfo->consecutiveBlockedMoves++;
		}
		else
		{
			NPCInfo->consecutiveBlockedMoves = 0;
		}
		NPCInfo->goalRadius = MAX_DISTANCE;	// just get us within combat range
	}
}

//---------------------------------------------------------
//extern void G_Knockdown( gentity_t *self, gentity_t *attacker, const vec3_t pushDir, float strength, qboolean breakSaberLock );
extern void G_Knockdown( gentity_t *victim );
extern void G_Dismember( gentity_t *ent, gentity_t *enemy, vec3_t point, int limbType, float limbRollBase, float limbPitchBase, int deathAnim, qboolean postDeath );
//extern qboolean G_DoDismemberment( gentity_t *self, vec3_t point, int mod, int damage, int hitLoc, qboolean force );
extern float NPC_EntRangeFromBolt( gentity_t *targEnt, int boltIndex );
extern int NPC_GetEntsNearBolt( int *radiusEnts, float radius, int boltIndex, vec3_t boltOrg );

void Rancor_DropVictim( gentity_t *self )
{
	//FIXME: if Rancor dies, it should drop its victim.
	//FIXME: if Rancor is removed, it must remove its victim.
	if ( self->activator )
	{
		if ( self->activator->client )
		{
			self->activator->client->ps.eFlags2 &= ~EF2_HELD_BY_MONSTER;
			self->activator->client->ps.hasLookTarget = qfalse;
			self->activator->client->ps.lookTarget = ENTITYNUM_NONE;
			self->activator->client->ps.viewangles[ROLL] = 0;
			SetClientViewAngle( self->activator, self->activator->client->ps.viewangles );
			self->activator->r.currentAngles[PITCH] = self->activator->r.currentAngles[ROLL] = 0;
			G_SetAngles( self->activator, self->activator->r.currentAngles );
		}
		if ( self->activator->health <= 0 )
		{
			//if ( self->activator->s.number )
			{//never free player
				if ( self->count == 1 )
				{//in my hand, just drop them
					if ( self->activator->client )
					{
						self->activator->client->ps.legsTimer = self->activator->client->ps.torsoTimer = 0;
						//FIXME: ragdoll?
					}
				}
				else
				{
					if ( self->activator->client )
					{
						self->activator->client->ps.eFlags |= EF_NODRAW;//so his corpse doesn't drop out of me...
					}
					//G_FreeEntity( self->activator );
				}
			}
		}
		else
		{
			if ( self->activator->NPC )
			{//start thinking again
				self->activator->NPC->nextBStateThink = level.time;
			}
			//clear their anim and let them fall
			self->activator->client->ps.legsTimer = self->activator->client->ps.torsoTimer = 0;
		}
		if ( self->enemy == self->activator )
		{
			self->enemy = NULL;
		}
		self->activator = NULL;
	}
	self->count = 0;//drop him
}

void Rancor_Swing( qboolean tryGrab )
{
	int			radiusEntNums[128];
	int			numEnts;
	const float	radius = 88;
	const float	radiusSquared = (radius*radius);
	int			i;
	vec3_t		boltOrg;

	numEnts = NPC_GetEntsNearBolt( radiusEntNums, radius, NPC->client->renderInfo.handRBolt, boltOrg );

	for ( i = 0; i < numEnts; i++ )
	{
		gentity_t *radiusEnt = &g_entities[radiusEntNums[i]];
		if ( !radiusEnt->inuse )
		{
			continue;
		}
		
		if ( radiusEnt == NPC )
		{//Skip the rancor ent
			continue;
		}
		
		if ( radiusEnt->client == NULL )
		{//must be a client
			continue;
		}

		if ( (radiusEnt->client->ps.eFlags2&EF2_HELD_BY_MONSTER) )
		{//can't be one already being held
			continue;
		}
		
		if ( DistanceSquared( radiusEnt->r.currentOrigin, boltOrg ) <= radiusSquared )
		{
			if ( tryGrab 
				&& NPC->count != 1 //don't have one in hand or in mouth already - FIXME: allow one in hand and any number in mouth!
				&& radiusEnt->client->NPC_class != CLASS_RANCOR
				&& radiusEnt->client->NPC_class != CLASS_GALAKMECH
				&& radiusEnt->client->NPC_class != CLASS_ATST
				&& radiusEnt->client->NPC_class != CLASS_GONK
				&& radiusEnt->client->NPC_class != CLASS_R2D2
				&& radiusEnt->client->NPC_class != CLASS_R5D2
				&& radiusEnt->client->NPC_class != CLASS_MARK1
				&& radiusEnt->client->NPC_class != CLASS_MARK2
				&& radiusEnt->client->NPC_class != CLASS_MOUSE
				&& radiusEnt->client->NPC_class != CLASS_PROBE
				&& radiusEnt->client->NPC_class != CLASS_SEEKER
				&& radiusEnt->client->NPC_class != CLASS_REMOTE
				&& radiusEnt->client->NPC_class != CLASS_SENTRY
				&& radiusEnt->client->NPC_class != CLASS_INTERROGATOR
				&& radiusEnt->client->NPC_class != CLASS_VEHICLE )
			{//grab
				if ( NPC->count == 2 )
				{//have one in my mouth, remove him
					TIMER_Remove( NPC, "clearGrabbed" );
					Rancor_DropVictim( NPC );
				}
				NPC->enemy = radiusEnt;//make him my new best friend
				radiusEnt->client->ps.eFlags2 |= EF2_HELD_BY_MONSTER;
				//FIXME: this makes it so that the victim can't hit us with shots!  Just use activator or something
				radiusEnt->client->ps.hasLookTarget = qtrue;
				radiusEnt->client->ps.lookTarget = NPC->s.number;
				NPC->activator = radiusEnt;//remember him
				NPC->count = 1;//in my hand
				//wait to attack
				TIMER_Set( NPC, "attacking", NPC->client->ps.legsTimer + Q_irand(500, 2500) );
				if ( radiusEnt->health > 0 && radiusEnt->pain )
				{//do pain on enemy
					radiusEnt->pain( radiusEnt, NPC, 100 );
					//GEntity_PainFunc( radiusEnt, NPC, NPC, radiusEnt->r.currentOrigin, 0, MOD_CRUSH );
				}
				else if ( radiusEnt->client )
				{
					radiusEnt->client->ps.forceHandExtend = HANDEXTEND_NONE;
					radiusEnt->client->ps.forceHandExtendTime = 0;
					NPC_SetAnim( radiusEnt, SETANIM_BOTH, BOTH_SWIM_IDLE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
				}
			}
			else
			{//smack
				vec3_t pushDir;
				vec3_t angs;

				G_Sound( radiusEnt, CHAN_AUTO, G_SoundIndex( "sound/chars/rancor/swipehit.wav" ) );
				//actually push the enemy
				/*
				//VectorSubtract( radiusEnt->r.currentOrigin, boltOrg, pushDir );
				VectorSubtract( radiusEnt->r.currentOrigin, NPC->r.currentOrigin, pushDir );
				pushDir[2] = Q_flrand( 100, 200 );
				VectorNormalize( pushDir );
				*/
				VectorCopy( NPC->client->ps.viewangles, angs );
				angs[YAW] += flrand( 25, 50 );
				angs[PITCH] = flrand( -25, -15 );
				AngleVectors( angs, pushDir, NULL, NULL );
				if ( radiusEnt->client->NPC_class != CLASS_RANCOR
					&& radiusEnt->client->NPC_class != CLASS_ATST )
				{
					G_Damage( radiusEnt, NPC, NPC, vec3_origin, radiusEnt->r.currentOrigin, Q_irand( 25, 40 ), DAMAGE_NO_ARMOR|DAMAGE_NO_KNOCKBACK, MOD_MELEE );
					G_Throw( radiusEnt, pushDir, 250 );
					if ( radiusEnt->health > 0 )
					{//do pain on enemy
						G_Knockdown( radiusEnt );//, NPC, pushDir, 100, qtrue );
					}
				}
			}
		}
	}
}

void Rancor_Smash( void )
{
	int			radiusEntNums[128];
	int			numEnts;
	const float	radius = 128;
	const float	halfRadSquared = ((radius/2)*(radius/2));
	const float	radiusSquared = (radius*radius);
	float		distSq;
	int			i;
	vec3_t		boltOrg;

	AddSoundEvent( NPC, NPC->r.currentOrigin, 512, AEL_DANGER, qfalse );//, qtrue );

	numEnts = NPC_GetEntsNearBolt( radiusEntNums, radius, NPC->client->renderInfo.handLBolt, boltOrg );

	for ( i = 0; i < numEnts; i++ )
	{
		gentity_t *radiusEnt = &g_entities[radiusEntNums[i]];
		if ( !radiusEnt->inuse )
		{
			continue;
		}
		
		if ( radiusEnt == NPC )
		{//Skip the rancor ent
			continue;
		}
		
		if ( radiusEnt->client == NULL )
		{//must be a client
			continue;
		}

		if ( (radiusEnt->client->ps.eFlags2&EF2_HELD_BY_MONSTER) )
		{//can't be one being held
			continue;
		}
		
		distSq = DistanceSquared( radiusEnt->r.currentOrigin, boltOrg );
		if ( distSq <= radiusSquared )
		{
			G_Sound( radiusEnt, CHAN_AUTO, G_SoundIndex( "sound/chars/rancor/swipehit.wav" ) );
			if ( distSq < halfRadSquared )
			{//close enough to do damage, too
				G_Damage( radiusEnt, NPC, NPC, vec3_origin, radiusEnt->r.currentOrigin, Q_irand( 10, 25 ), DAMAGE_NO_ARMOR|DAMAGE_NO_KNOCKBACK, MOD_MELEE );
			}
			if ( radiusEnt->health > 0 
				&& radiusEnt->client
				&& radiusEnt->client->NPC_class != CLASS_RANCOR
				&& radiusEnt->client->NPC_class != CLASS_ATST )
			{
				if ( distSq < halfRadSquared 
					|| radiusEnt->client->ps.groundEntityNum != ENTITYNUM_NONE )
				{//within range of my fist or withing ground-shaking range and not in the air
					G_Knockdown( radiusEnt );//, NPC, vec3_origin, 100, qtrue );
				}
			}
		}
	}
}

void Rancor_Bite( void )
{
	int			radiusEntNums[128];
	int			numEnts;
	const float	radius = 100;
	const float	radiusSquared = (radius*radius);
	int			i;
	vec3_t		boltOrg;

	numEnts = NPC_GetEntsNearBolt( radiusEntNums, radius, NPC->client->renderInfo.crotchBolt, boltOrg );//was gutBolt?

	for ( i = 0; i < numEnts; i++ )
	{
		gentity_t *radiusEnt = &g_entities[radiusEntNums[i]];
		if ( !radiusEnt->inuse )
		{
			continue;
		}
		
		if ( radiusEnt == NPC )
		{//Skip the rancor ent
			continue;
		}
		
		if ( radiusEnt->client == NULL )
		{//must be a client
			continue;
		}

		if ( (radiusEnt->client->ps.eFlags2&EF2_HELD_BY_MONSTER) )
		{//can't be one already being held
			continue;
		}
		
		if ( DistanceSquared( radiusEnt->r.currentOrigin, boltOrg ) <= radiusSquared )
		{
			G_Damage( radiusEnt, NPC, NPC, vec3_origin, radiusEnt->r.currentOrigin, Q_irand( 15, 30 ), DAMAGE_NO_ARMOR|DAMAGE_NO_KNOCKBACK, MOD_MELEE );
			if ( radiusEnt->health <= 0 && radiusEnt->client )
			{//killed them, chance of dismembering
				if ( !Q_irand( 0, 1 ) )
				{//bite something off
					int hitLoc = Q_irand( G2_MODELPART_HEAD, G2_MODELPART_RLEG );
					if ( hitLoc == G2_MODELPART_HEAD )
					{
						NPC_SetAnim( radiusEnt, SETANIM_BOTH, BOTH_DEATH17, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
					}
					else if ( hitLoc == G2_MODELPART_WAIST )
					{
						NPC_SetAnim( radiusEnt, SETANIM_BOTH, BOTH_DEATHBACKWARD2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
					}
					//radiusEnt->client->dismembered = qfalse;
					//FIXME: the limb should just disappear, cuz I ate it
					G_Dismember( radiusEnt, NPC, radiusEnt->r.currentOrigin, hitLoc, 90, 0, radiusEnt->client->ps.torsoAnim, qtrue);
					//G_DoDismemberment( radiusEnt, radiusEnt->r.currentOrigin, MOD_SABER, 1000, hitLoc, qtrue );
				}
			}
			G_Sound( radiusEnt, CHAN_AUTO, G_SoundIndex( "sound/chars/rancor/chomp.wav" ) );
		}
	}
}
//------------------------------
extern void TossClientItems( gentity_t *self );
void Rancor_Attack( float distance, qboolean doCharge )
{
	if ( !TIMER_Exists( NPC, "attacking" ) )
	{
		if ( NPC->count == 2 && NPC->activator )
		{
		}
		else if ( NPC->count == 1 && NPC->activator )
		{//holding enemy
			if ( NPC->activator->health > 0 && Q_irand( 0, 1 ) )
			{//quick bite
				NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_ATTACK1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
				TIMER_Set( NPC, "attack_dmg", 450 );
			}
			else
			{//full eat
				NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_ATTACK3, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
				TIMER_Set( NPC, "attack_dmg", 900 );
				//Make victim scream in fright
				if ( NPC->activator->health > 0 && NPC->activator->client )
				{
					G_AddEvent( NPC->activator, Q_irand(EV_DEATH1, EV_DEATH3), 0 );
					NPC_SetAnim( NPC->activator, SETANIM_TORSO, BOTH_FALLDEATH1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
					if ( NPC->activator->NPC )
					{//no more thinking for you
						TossClientItems( NPC );
						NPC->activator->NPC->nextBStateThink = Q3_INFINITE;
					}
				}
			}
		}
		else if ( NPC->enemy->health > 0 && doCharge )
		{//charge
			vec3_t	fwd, yawAng;
			VectorSet( yawAng, 0, NPC->client->ps.viewangles[YAW], 0 );
			AngleVectors( yawAng, fwd, NULL, NULL );
			VectorScale( fwd, distance*1.5f, NPC->client->ps.velocity );
			NPC->client->ps.velocity[2] = 150;
			NPC->client->ps.groundEntityNum = ENTITYNUM_NONE;

			NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_MELEE2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
			TIMER_Set( NPC, "attack_dmg", 1250 );
		}
		else if ( !Q_irand(0, 1) )
		{//smash
			NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_MELEE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
			TIMER_Set( NPC, "attack_dmg", 1000 );
		}
		else
		{//try to grab
			NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_ATTACK2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
			TIMER_Set( NPC, "attack_dmg", 1000 );
		}

		TIMER_Set( NPC, "attacking", NPC->client->ps.legsTimer + random() * 200 );
	}

	// Need to do delayed damage since the attack animations encapsulate multiple mini-attacks

	if ( TIMER_Done2( NPC, "attack_dmg", qtrue ) )
	{
		vec3_t shakePos;
		switch ( NPC->client->ps.legsAnim )
		{
		case BOTH_MELEE1:
			Rancor_Smash();
			G_GetBoltPosition( NPC, NPC->client->renderInfo.handLBolt, shakePos, 0 );
			G_ScreenShake( shakePos, NULL, 4.0f, 1000, qfalse );
			//CGCam_Shake( 1.0f*playerDist/128.0f, 1000 );
			break;
		case BOTH_MELEE2:
			Rancor_Bite();
			TIMER_Set( NPC, "attack_dmg2", 450 );
			break;
		case BOTH_ATTACK1:
			if ( NPC->count == 1 && NPC->activator )
			{
				G_Damage( NPC->activator, NPC, NPC, vec3_origin, NPC->activator->r.currentOrigin, Q_irand( 25, 40 ), DAMAGE_NO_ARMOR|DAMAGE_NO_KNOCKBACK, MOD_MELEE );
				if ( NPC->activator->health <= 0 )
				{//killed him
					//make it look like we bit his head off
					//NPC->activator->client->dismembered = qfalse;
					G_Dismember( NPC->activator, NPC, NPC->activator->r.currentOrigin, G2_MODELPART_HEAD, 90, 0, NPC->activator->client->ps.torsoAnim, qtrue);
					//G_DoDismemberment( NPC->activator, NPC->activator->r.currentOrigin, MOD_SABER, 1000, HL_HEAD, qtrue );
					NPC->activator->client->ps.forceHandExtend = HANDEXTEND_NONE;
					NPC->activator->client->ps.forceHandExtendTime = 0;
					NPC_SetAnim( NPC->activator, SETANIM_BOTH, BOTH_SWIM_IDLE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
				}
				G_Sound( NPC->activator, CHAN_AUTO, G_SoundIndex( "sound/chars/rancor/chomp.wav" ) );
			}
			break;
		case BOTH_ATTACK2:
			//try to grab
			Rancor_Swing( qtrue );
			break;
		case BOTH_ATTACK3:
			if ( NPC->count == 1 && NPC->activator )
			{
				//cut in half
				if ( NPC->activator->client )
				{
					//NPC->activator->client->dismembered = qfalse;
					G_Dismember( NPC->activator, NPC, NPC->activator->r.currentOrigin, G2_MODELPART_WAIST, 90, 0, NPC->activator->client->ps.torsoAnim, qtrue);
					//G_DoDismemberment( NPC->activator, NPC->enemy->r.currentOrigin, MOD_SABER, 1000, HL_WAIST, qtrue );
				}
				//KILL
				G_Damage( NPC->activator, NPC, NPC, vec3_origin, NPC->activator->r.currentOrigin, NPC->enemy->health+10, DAMAGE_NO_PROTECTION|DAMAGE_NO_ARMOR|DAMAGE_NO_KNOCKBACK|DAMAGE_NO_HIT_LOC, MOD_MELEE );//, HL_NONE );//
				if ( NPC->activator->client )
				{
					NPC->activator->client->ps.forceHandExtend = HANDEXTEND_NONE;
					NPC->activator->client->ps.forceHandExtendTime = 0;
					NPC_SetAnim( NPC->activator, SETANIM_BOTH, BOTH_SWIM_IDLE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
				}
				TIMER_Set( NPC, "attack_dmg2", 1350 );
				G_Sound( NPC->activator, CHAN_AUTO, G_SoundIndex( "sound/chars/rancor/swipehit.wav" ) );
				G_AddEvent( NPC->activator, EV_JUMP, NPC->activator->health );
			}
			break;
		}
	}
	else if ( TIMER_Done2( NPC, "attack_dmg2", qtrue ) )
	{
		switch ( NPC->client->ps.legsAnim )
		{
		case BOTH_MELEE1:
			break;
		case BOTH_MELEE2:
			Rancor_Bite();
			break;
		case BOTH_ATTACK1:
			break;
		case BOTH_ATTACK2:
			break;
		case BOTH_ATTACK3:
			if ( NPC->count == 1 && NPC->activator )
			{//swallow victim
				G_Sound( NPC->activator, CHAN_AUTO, G_SoundIndex( "sound/chars/rancor/chomp.wav" ) );
				//FIXME: sometimes end up with a live one in our mouths?
				//just make sure they're dead
				if ( NPC->activator->health > 0 )
				{
					//cut in half
					//NPC->activator->client->dismembered = qfalse;
					G_Dismember( NPC->activator, NPC, NPC->activator->r.currentOrigin, G2_MODELPART_WAIST, 90, 0, NPC->activator->client->ps.torsoAnim, qtrue);
					//G_DoDismemberment( NPC->activator, NPC->enemy->r.currentOrigin, MOD_SABER, 1000, HL_WAIST, qtrue );
					//KILL
					G_Damage( NPC->activator, NPC, NPC, vec3_origin, NPC->activator->r.currentOrigin, NPC->enemy->health+10, DAMAGE_NO_PROTECTION|DAMAGE_NO_ARMOR|DAMAGE_NO_KNOCKBACK|DAMAGE_NO_HIT_LOC, MOD_MELEE );//, HL_NONE );
					NPC->activator->client->ps.forceHandExtend = HANDEXTEND_NONE;
					NPC->activator->client->ps.forceHandExtendTime = 0;
					NPC_SetAnim( NPC->activator, SETANIM_BOTH, BOTH_SWIM_IDLE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
					G_AddEvent( NPC->activator, EV_JUMP, NPC->activator->health );
				}
				if ( NPC->activator->client )
				{//*sigh*, can't get tags right, just remove them?
					NPC->activator->client->ps.eFlags |= EF_NODRAW;
				}
				NPC->count = 2;
				TIMER_Set( NPC, "clearGrabbed", 2600 );
			}
			break;
		}
	}
	else if ( NPC->client->ps.legsAnim == BOTH_ATTACK2 )
	{
		if ( NPC->client->ps.legsTimer >= 1200 && NPC->client->ps.legsTimer <= 1350 )
		{
			if ( Q_irand( 0, 2 ) )
			{
				Rancor_Swing( qfalse );
			}
			else
			{
				Rancor_Swing( qtrue );
			}
		}
		else if ( NPC->client->ps.legsTimer >= 1100 && NPC->client->ps.legsTimer <= 1550 )
		{
			Rancor_Swing( qtrue );
		}
	}

	// Just using this to remove the attacking flag at the right time
	TIMER_Done2( NPC, "attacking", qtrue );
}

//----------------------------------
void Rancor_Combat( void )
{
	if ( NPC->count )
	{//holding my enemy
		if ( TIMER_Done2( NPC, "takingPain", qtrue ))
		{
			NPCInfo->localState = LSTATE_CLEAR;
		}
		else
		{
			Rancor_Attack( 0, qfalse );
		}
		NPC_UpdateAngles( qtrue, qtrue );
		return;
	}
	// If we cannot see our target or we have somewhere to go, then do that
	if ( !NPC_ClearLOS4( NPC->enemy ) )//|| UpdateGoal( ))
	{
		NPCInfo->combatMove = qtrue;
		NPCInfo->goalEntity = NPC->enemy;
		NPCInfo->goalRadius = MIN_DISTANCE;//MAX_DISTANCE;	// just get us within combat range

		if ( !NPC_MoveToGoal( qtrue ) )
		{//couldn't go after him?  Look for a new one
			TIMER_Set( NPC, "lookForNewEnemy", 0 );
			NPCInfo->consecutiveBlockedMoves++;
		}
		else 
		{
			NPCInfo->consecutiveBlockedMoves = 0;
		}
		return;
	}

	// Sometimes I have problems with facing the enemy I'm attacking, so force the issue so I don't look dumb
	NPC_FaceEnemy( qtrue );

	{
		float	distance;
		qboolean	advance;
		qboolean	doCharge;

		distance	= Distance( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin );	
		advance = (qboolean)( distance > (NPC->r.maxs[0]+MIN_DISTANCE) ? qtrue : qfalse  );
		doCharge = qfalse;

		if ( advance )
		{//have to get closer
			vec3_t	yawOnlyAngles;
			VectorSet( yawOnlyAngles, 0, NPC->r.currentAngles[YAW], 0 );
			if ( NPC->enemy->health > 0
				&& fabs(distance-250) <= 80 
				&& InFOV3( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin, yawOnlyAngles, 30, 30 ) )
			{
				if ( !Q_irand( 0, 9 ) )
				{//go for the charge
					doCharge = qtrue;
					advance = qfalse;
				}
			}
		}

		if (( advance /*|| NPCInfo->localState == LSTATE_WAITING*/ ) && TIMER_Done( NPC, "attacking" )) // waiting monsters can't attack
		{
			if ( TIMER_Done2( NPC, "takingPain", qtrue ))
			{
				NPCInfo->localState = LSTATE_CLEAR;
			}
			else
			{
				Rancor_Move( 1 );
			}
		}
		else
		{
			Rancor_Attack( distance, doCharge );
		}
	}
}

/*
-------------------------
NPC_Rancor_Pain
-------------------------
*/
//void NPC_Rancor_Pain( gentity_t *self, gentity_t *inflictor, gentity_t *other, const vec3_t point, int damage, int mod,int hitLoc ) 
void NPC_Rancor_Pain( gentity_t *self, gentity_t *attacker, int damage ) 
{
	qboolean hitByRancor = qfalse;
	if ( attacker&&attacker->client&&attacker->client->NPC_class==CLASS_RANCOR )
	{
		hitByRancor = qtrue;
	}
	if ( attacker 
		&& attacker->inuse 
		&& attacker != self->enemy
		&& !(attacker->flags&FL_NOTARGET) )
	{
		if ( !self->count )
		{
			if ( (!attacker->s.number&&!Q_irand(0,3))
				|| !self->enemy
				|| self->enemy->health == 0
				|| (self->enemy->client&&self->enemy->client->NPC_class == CLASS_RANCOR)
				|| (self->NPC && self->NPC->consecutiveBlockedMoves>=10 && DistanceSquared( attacker->r.currentOrigin, self->r.currentOrigin ) < DistanceSquared( self->enemy->r.currentOrigin, self->r.currentOrigin )) ) 
			{//if my enemy is dead (or attacked by player) and I'm not still holding/eating someone, turn on the attacker
				//FIXME: if can't nav to my enemy, take this guy if I can nav to him
				G_SetEnemy( self, attacker );
				TIMER_Set( self, "lookForNewEnemy", Q_irand( 5000, 15000 ) );
				if ( hitByRancor )
				{//stay mad at this Rancor for 2-5 secs before looking for attacker enemies
					TIMER_Set( self, "rancorInfight", Q_irand( 2000, 5000 ) );
				}

			}
		}
	}
	if ( (hitByRancor|| (self->count==1&&self->activator&&!Q_irand(0,4)) || Q_irand( 0, 200 ) < damage )//hit by rancor, hit while holding live victim, or took a lot of damage
		&& self->client->ps.legsAnim != BOTH_STAND1TO2
		&& TIMER_Done( self, "takingPain" ) )
	{
		if ( !Rancor_CheckRoar( self ) )
		{
			if ( self->client->ps.legsAnim != BOTH_MELEE1
				&& self->client->ps.legsAnim != BOTH_MELEE2
				&& self->client->ps.legsAnim != BOTH_ATTACK2 )
			{//cant interrupt one of the big attack anims
				/*
				if ( self->count != 1 
					|| attacker == self->activator
					|| (self->client->ps.legsAnim != BOTH_ATTACK1&&self->client->ps.legsAnim != BOTH_ATTACK3) )
				*/
				{//if going to bite our victim, only victim can interrupt that anim
					if ( self->health > 100 || hitByRancor )
					{
						TIMER_Remove( self, "attacking" );

						VectorCopy( self->NPC->lastPathAngles, self->s.angles );

						if ( self->count == 1 )
						{
							NPC_SetAnim( self, SETANIM_BOTH, BOTH_PAIN2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
						}
						else
						{
							NPC_SetAnim( self, SETANIM_BOTH, BOTH_PAIN1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
						}
						TIMER_Set( self, "takingPain", self->client->ps.legsTimer+Q_irand(0, 500) );

						if ( self->NPC )
						{
							self->NPC->localState = LSTATE_WAITING;
						}
					}
				}
			}
		}
		//let go
		/*
		if ( !Q_irand( 0, 3 ) && self->count == 1 )
		{
			Rancor_DropVictim( self );
		}
		*/
	}
}

void Rancor_CheckDropVictim( void )
{
	vec3_t mins;
	vec3_t maxs;
	vec3_t start; 
	vec3_t end; 
	trace_t	trace;

	VectorSet( mins, NPC->activator->r.mins[0]-1, NPC->activator->r.mins[1]-1, 0 );
	VectorSet( maxs, NPC->activator->r.maxs[0]+1, NPC->activator->r.maxs[1]+1, 1 );
	VectorSet( start, NPC->activator->r.currentOrigin[0], NPC->activator->r.currentOrigin[1], NPC->activator->r.absmin[2] ); 
	VectorSet( end, NPC->activator->r.currentOrigin[0], NPC->activator->r.currentOrigin[1], NPC->activator->r.absmax[2]-1 ); 

	trap_Trace( &trace, start, mins, maxs, end, NPC->activator->s.number, NPC->activator->clipmask );
	if ( !trace.allsolid && !trace.startsolid && trace.fraction >= 1.0f )
	{
		Rancor_DropVictim( NPC );
	}
}

//if he's stepping on things then crush them -rww
void Rancor_Crush(void)
{
	gentity_t *crush;

	if (!NPC ||
		!NPC->client ||
		NPC->client->ps.groundEntityNum >= ENTITYNUM_WORLD)
	{ //nothing to crush
		return;
	}

	crush = &g_entities[NPC->client->ps.groundEntityNum];
	if (crush->inuse && crush->client && !crush->localAnimIndex)
	{ //a humanoid, smash them good.
		G_Damage(crush, NPC, NPC, NULL, NPC->r.currentOrigin, 200, 0, MOD_CRUSH);
	}
}

/*
-------------------------
NPC_BSRancor_Default
-------------------------
*/
void NPC_BSRancor_Default( void )
{
	AddSightEvent( NPC, NPC->r.currentOrigin, 1024, AEL_DANGER_GREAT, 50 );

	Rancor_Crush();

	NPC->client->ps.eFlags2 &= ~(EF2_USE_ALT_ANIM|EF2_GENERIC_NPC_FLAG);
	if ( NPC->count )
	{//holding someone
		NPC->client->ps.eFlags2 |= EF2_USE_ALT_ANIM;
		if ( NPC->count == 2 )
		{//in my mouth
			NPC->client->ps.eFlags2 |= EF2_GENERIC_NPC_FLAG;
		}
	}
	else
	{
		NPC->client->ps.eFlags2 &= ~(EF2_USE_ALT_ANIM|EF2_GENERIC_NPC_FLAG);
	}

	if ( TIMER_Done2( NPC, "clearGrabbed", qtrue ) )
	{
		Rancor_DropVictim( NPC );
	}
	else if ( NPC->client->ps.legsAnim == BOTH_PAIN2 
		&& NPC->count == 1 
		&& NPC->activator )
	{
		if ( !Q_irand( 0, 3 ) )
		{
			Rancor_CheckDropVictim();
		}
	}
	if ( !TIMER_Done( NPC, "rageTime" ) )
	{//do nothing but roar first time we see an enemy
		AddSoundEvent( NPC, NPC->r.currentOrigin, 1024, AEL_DANGER_GREAT, qfalse );//, qfalse );
		NPC_FaceEnemy( qtrue );
		return;
	}
	if ( NPC->enemy )
	{
		/*
		if ( NPC->enemy->client //enemy is a client
			&& (NPC->enemy->client->NPC_class == CLASS_UGNAUGHT || NPC->enemy->client->NPC_class == CLASS_JAWA )//enemy is a lowly jawa or ugnaught
			&& NPC->enemy->enemy != NPC//enemy's enemy is not me
			&& (!NPC->enemy->enemy || !NPC->enemy->enemy->client || NPC->enemy->enemy->client->NPC_class!=CLASS_RANCOR) )//enemy's enemy is not a client or is not a rancor (which is as scary as me anyway)
		{//they should be scared of ME and no-one else
			G_SetEnemy( NPC->enemy, NPC );
		}
		*/
		if ( TIMER_Done(NPC,"angrynoise") )
		{
			G_Sound( NPC, CHAN_AUTO, G_SoundIndex( va("sound/chars/rancor/misc/anger%d.wav", Q_irand(1, 3))) );

			TIMER_Set( NPC, "angrynoise", Q_irand( 5000, 10000 ) );
		}
		else
		{
			AddSoundEvent( NPC, NPC->r.currentOrigin, 512, AEL_DANGER_GREAT, qfalse );//, qfalse );
		}
		if ( NPC->count == 2 && NPC->client->ps.legsAnim == BOTH_ATTACK3 )
		{//we're still chewing our enemy up
			NPC_UpdateAngles( qtrue, qtrue );
			return;
		}
		//else, if he's in our hand, we eat, else if he's on the ground, we keep attacking his dead body for a while
		if( NPC->enemy->client && NPC->enemy->client->NPC_class == CLASS_RANCOR )
		{//got mad at another Rancor, look for a valid enemy
			if ( TIMER_Done( NPC, "rancorInfight" ) )
			{
				NPC_CheckEnemyExt( qtrue );
			}
		}
		else if ( !NPC->count )
		{
			if ( ValidEnemy( NPC->enemy ) == qfalse )
			{
				TIMER_Remove( NPC, "lookForNewEnemy" );//make them look again right now
				if ( !NPC->enemy->inuse || level.time - NPC->enemy->s.time > Q_irand( 10000, 15000 ) )
				{//it's been a while since the enemy died, or enemy is completely gone, get bored with him
					NPC->enemy = NULL;
					Rancor_Patrol();
					NPC_UpdateAngles( qtrue, qtrue );
					return;
				}
			}
			if ( TIMER_Done( NPC, "lookForNewEnemy" ) )
			{
				gentity_t *newEnemy, *sav_enemy = NPC->enemy;//FIXME: what about NPC->lastEnemy?
				NPC->enemy = NULL;
				newEnemy = NPC_CheckEnemy( NPCInfo->confusionTime < level.time, qfalse, qfalse );
				NPC->enemy = sav_enemy;
				if ( newEnemy && newEnemy != sav_enemy )
				{//picked up a new enemy!
					NPC->lastEnemy = NPC->enemy;
					G_SetEnemy( NPC, newEnemy );
					//hold this one for at least 5-15 seconds
					TIMER_Set( NPC, "lookForNewEnemy", Q_irand( 5000, 15000 ) );
				}
				else
				{//look again in 2-5 secs
					TIMER_Set( NPC, "lookForNewEnemy", Q_irand( 2000, 5000 ) );
				}
			}
		}
		Rancor_Combat();
	}
	else 
	{
		if ( TIMER_Done(NPC,"idlenoise") )
		{
			G_Sound( NPC, CHAN_AUTO, G_SoundIndex( va("sound/chars/rancor/snort_%d.wav", Q_irand(1, 2))) );

			TIMER_Set( NPC, "idlenoise", Q_irand( 2000, 4000 ) );
			AddSoundEvent( NPC, NPC->r.currentOrigin, 384, AEL_DANGER, qfalse );//, qfalse );
		}
		if ( NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES )
		{
			Rancor_Patrol();
		}
		else
		{
			Rancor_Idle();
		}
	}

	NPC_UpdateAngles( qtrue, qtrue );
}
