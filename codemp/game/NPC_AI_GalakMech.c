#include "b_local.h"
#include "g_nav.h"
#include "anims.h"
#include "w_saber.h"

extern void G_AddVoiceEvent( gentity_t *self, int event, int speakDebounceTime );
extern void NPC_AimAdjust( int change );
extern qboolean WP_LobFire( gentity_t *self, vec3_t start, vec3_t target, vec3_t mins, vec3_t maxs, int clipmask, 
				vec3_t velocity, qboolean tracePath, int ignoreEntNum, int enemyNum,
				float minSpeed, float maxSpeed, float idealSpeed, qboolean mustHit );
extern void G_SoundOnEnt (gentity_t *ent, soundChannel_t channel, const char *soundPath);

#include "../namespace_begin.h"
extern qboolean BG_CrouchAnim( int anim );
#include "../namespace_end.h"

//extern void NPC_Mark1_Part_Explode(gentity_t *self,int bolt);

#define MELEE_DIST_SQUARED 6400//80*80
#define MIN_LOB_DIST_SQUARED 65536//256*256
#define MAX_LOB_DIST_SQUARED 200704//448*448
#define REPEATER_ALT_SIZE				3	// half of bbox size
#define	GENERATOR_HEALTH	25
#define TURN_ON				0x00000000
#define TURN_OFF			0x00000100
#define GALAK_SHIELD_HEALTH	500

static vec3_t shieldMins = {-60, -60, -24 };
static vec3_t shieldMaxs = {60, 60, 80};

extern qboolean NPC_CheckPlayerTeamStealth( void );

static qboolean enemyLOS4;
static qboolean enemyCS4;
static qboolean hitAlly4;
static qboolean faceEnemy4;
static qboolean move4;
static qboolean shoot4;
static float	enemyDist4;
static vec3_t	impactPos4;

void NPC_GalakMech_Precache( void )
{
	G_SoundIndex( "sound/weapons/galak/skewerhit.wav" );
	G_SoundIndex( "sound/weapons/galak/lasercharge.wav" );
	G_SoundIndex( "sound/weapons/galak/lasercutting.wav" );
	G_SoundIndex( "sound/weapons/galak/laserdamage.wav" );

	G_EffectIndex( "galak/trace_beam" );
	G_EffectIndex( "galak/beam_warmup" );
//	G_EffectIndex( "small_chunks");
	G_EffectIndex( "env/med_explode2");
	G_EffectIndex( "env/small_explode2");
	G_EffectIndex( "galak/explode");
	G_EffectIndex( "blaster/smoke_bolton");
//	G_EffectIndex( "env/exp_trail_comp");
}

void NPC_GalakMech_Init( gentity_t *ent )
{
	if (ent->NPC->behaviorState != BS_CINEMATIC)
	{
		ent->client->ps.stats[STAT_ARMOR] = GALAK_SHIELD_HEALTH;
		ent->NPC->investigateCount = ent->NPC->investigateDebounceTime = 0;
		ent->flags |= FL_SHIELDED;//reflect normal shots
		//rwwFIXMEFIXME: Support PW_GALAK_SHIELD
		//ent->client->ps.powerups[PW_GALAK_SHIELD] = Q3_INFINITE;//temp, for effect
		//ent->fx_time = level.time;
		VectorSet( ent->r.mins, -60, -60, -24 );
		VectorSet( ent->r.maxs, 60, 60, 80 );
		ent->flags |= FL_NO_KNOCKBACK;//don't get pushed
		TIMER_Set( ent, "attackDelay", 0 );	//FIXME: Slant for difficulty levels
		TIMER_Set( ent, "flee", 0 );
		TIMER_Set( ent, "smackTime", 0 );
		TIMER_Set( ent, "beamDelay", 0 );
		TIMER_Set( ent, "noLob", 0 );
		TIMER_Set( ent, "noRapid", 0 );
		TIMER_Set( ent, "talkDebounce", 0 );

		NPC_SetSurfaceOnOff( ent, "torso_shield", TURN_ON );
		NPC_SetSurfaceOnOff( ent, "torso_galakface", TURN_OFF );
		NPC_SetSurfaceOnOff( ent, "torso_galakhead", TURN_OFF );
		NPC_SetSurfaceOnOff( ent, "torso_eyes_mouth", TURN_OFF );
		NPC_SetSurfaceOnOff( ent, "torso_collar", TURN_OFF );
		NPC_SetSurfaceOnOff( ent, "torso_galaktorso", TURN_OFF );
	}
	else
	{
//		NPC_SetSurfaceOnOff( ent, "helmet", TURN_OFF );
		NPC_SetSurfaceOnOff( ent, "torso_shield", TURN_OFF );
		NPC_SetSurfaceOnOff( ent, "torso_galakface", TURN_ON );
		NPC_SetSurfaceOnOff( ent, "torso_galakhead", TURN_ON );
		NPC_SetSurfaceOnOff( ent, "torso_eyes_mouth", TURN_ON );
		NPC_SetSurfaceOnOff( ent, "torso_collar", TURN_ON );
		NPC_SetSurfaceOnOff( ent, "torso_galaktorso", TURN_ON );
	}

}

//-----------------------------------------------------------------
static void GM_CreateExplosion( gentity_t *self, const int boltID, qboolean doSmall ) //doSmall = qfalse
{
	if ( boltID >=0 )
	{
		mdxaBone_t	boltMatrix;
		vec3_t		org, dir;

		trap_G2API_GetBoltMatrix( self->ghoul2, 0, 
					boltID,
					&boltMatrix, self->r.currentAngles, self->r.currentOrigin, level.time,
					NULL, self->modelScale );

		BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, org );
		BG_GiveMeVectorFromMatrix( &boltMatrix, NEGATIVE_Y, dir );

		if ( doSmall )
		{
			G_PlayEffectID( G_EffectIndex("env/small_explode2"), org, dir );
		}
		else
		{
			G_PlayEffectID( G_EffectIndex("env/med_explode2"), org, dir );
		}
	}
}

/*
-------------------------
GM_Dying
-------------------------
*/

void GM_Dying( gentity_t *self )
{
	if ( level.time - self->s.time < 4000 )
	{//FIXME: need a real effect
		//self->s.powerups |= ( 1 << PW_SHOCKED );
		//self->client->ps.powerups[PW_SHOCKED] = level.time + 1000;
		self->client->ps.electrifyTime = level.time + 1000;
		if ( TIMER_Done( self, "dyingExplosion" ) )
		{
			int	newBolt;
			switch ( Q_irand( 1, 14 ) )
			{
			// Find place to generate explosion
			case 1:
				if (!trap_G2API_GetSurfaceRenderStatus( self->ghoul2, 0, "r_hand" ))
				{//r_hand still there
					GM_CreateExplosion( self, trap_G2API_AddBolt(self->ghoul2, 0, "*flasha"), qtrue );
					NPC_SetSurfaceOnOff( self, "r_hand", TURN_OFF );
				}
				else if (!trap_G2API_GetSurfaceRenderStatus( self->ghoul2, 0, "r_arm_middle" ))
				{//r_arm_middle still there
					newBolt = trap_G2API_AddBolt( self->ghoul2, 0, "*r_arm_elbow" );
					NPC_SetSurfaceOnOff( self, "r_arm_middle", TURN_OFF );
				}
				break;
			case 2:
				//FIXME: do only once?
				if (!trap_G2API_GetSurfaceRenderStatus( self->ghoul2, 0, "l_hand" ))
				{//l_hand still there
					GM_CreateExplosion( self, trap_G2API_AddBolt(self->ghoul2, 0, "*flashc"), qfalse );
					NPC_SetSurfaceOnOff( self, "l_hand", TURN_OFF );
				}
				else if (!trap_G2API_GetSurfaceRenderStatus( self->ghoul2, 0, "l_arm_wrist" ))
				{//l_arm_wrist still there
					newBolt = trap_G2API_AddBolt( self->ghoul2, 0, "*l_arm_cap_l_hand" );
					NPC_SetSurfaceOnOff( self, "l_arm_wrist", TURN_OFF );
				}
				else if (!trap_G2API_GetSurfaceRenderStatus( self->ghoul2, 0, "l_arm_middle" ))
				{//l_arm_middle still there
					newBolt = trap_G2API_AddBolt( self->ghoul2, 0, "*l_arm_cap_l_hand" );
					NPC_SetSurfaceOnOff( self, "l_arm_middle", TURN_OFF );
				}
				else if (!trap_G2API_GetSurfaceRenderStatus( self->ghoul2, 0, "l_arm_augment" ))
				{//l_arm_augment still there
					newBolt = trap_G2API_AddBolt( self->ghoul2, 0, "*l_arm_elbow" );
					NPC_SetSurfaceOnOff( self, "l_arm_augment", TURN_OFF );
				}
				break;
			case 3:
			case 4:
				newBolt = trap_G2API_AddBolt( self->ghoul2, 0, "*hip_fr" );
				GM_CreateExplosion( self, newBolt, qfalse );
				break;
			case 5:
			case 6:
				newBolt = trap_G2API_AddBolt( self->ghoul2, 0, "*shldr_l" );
				GM_CreateExplosion( self, newBolt, qfalse );
				break;
			case 7:
			case 8:
				newBolt = trap_G2API_AddBolt( self->ghoul2, 0, "*uchest_r" );
				GM_CreateExplosion( self, newBolt, qfalse );
				break;
			case 9:
			case 10:
				GM_CreateExplosion( self, self->client->renderInfo.headBolt, qfalse );
				break;
			case 11:
				newBolt = trap_G2API_AddBolt( self->ghoul2, 0, "*l_leg_knee" );
				GM_CreateExplosion( self, newBolt, qtrue );
				break;
			case 12:
				newBolt = trap_G2API_AddBolt( self->ghoul2, 0, "*r_leg_knee" );
				GM_CreateExplosion( self, newBolt, qtrue );
				break;
			case 13:
				newBolt = trap_G2API_AddBolt( self->ghoul2, 0, "*l_leg_foot" );
				GM_CreateExplosion( self, newBolt, qtrue );
				break;
			case 14:
				newBolt = trap_G2API_AddBolt( self->ghoul2, 0, "*r_leg_foot" );
				GM_CreateExplosion( self, newBolt, qtrue );
				break;
			}

			TIMER_Set( self, "dyingExplosion", Q_irand( 300, 1100 ) );
		}
	}
	else
	{//one final, huge explosion
		G_PlayEffectID( G_EffectIndex("galak/explode"), self->r.currentOrigin, vec3_origin );
//		G_PlayEffect( "small_chunks", self->r.currentOrigin );
//		G_PlayEffect( "env/exp_trail_comp", self->r.currentOrigin, self->currentAngles );
		self->nextthink = level.time + FRAMETIME;
		self->think = G_FreeEntity;
	}
}

/*
-------------------------
NPC_GM_Pain
-------------------------
*/

extern void NPC_SetPainEvent( gentity_t *self );
void NPC_GM_Pain(gentity_t *self, gentity_t *attacker, int damage)
{
	vec3_t point;
	gentity_t *inflictor = attacker;
	int hitLoc = 1;
	int mod = gPainMOD;

	VectorCopy(gPainPoint, point);

	//if ( self->client->ps.powerups[PW_GALAK_SHIELD] == 0 )
	if (0) //rwwFIXMEFIXME: do all of this
	{//shield is currently down
		//FIXME: allow for radius damage?
		/*
		if ( (hitLoc==HL_GENERIC1) && (self->locationDamage[HL_GENERIC1] > GENERATOR_HEALTH) )
		{
			int newBolt = gi.G2API_AddBolt( &self->ghoul2[self->playerModel], "*antenna_base" );
			if ( newBolt != -1 )
			{
				GM_CreateExplosion( self, newBolt, qfalse );
			}

			NPC_SetSurfaceOnOff( self, "torso_shield", TURN_OFF );
			NPC_SetSurfaceOnOff( self, "torso_antenna", TURN_OFF );
			NPC_SetSurfaceOnOff( self, "torso_antenna_base_cap", TURN_ON );
			self->client->ps.powerups[PW_GALAK_SHIELD] = 0;//temp, for effect
			self->client->ps.stats[STAT_ARMOR] = 0;//no more armor
			self->NPC->investigateDebounceTime = 0;//stop recharging

			NPC_SetAnim( self, SETANIM_BOTH, BOTH_ALERT1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			TIMER_Set( self, "attackDelay", self->client->ps.torsoTimer );
			G_AddEvent( self, Q_irand( EV_DEATH1, EV_DEATH3 ), self->health );
		}
		*/
	}
	else
	{//store the point for shield impact
		if ( point )
		{
		//	VectorCopy( point, self->pos4 );
		//	self->client->poisonTime = level.time;
			//rwwFIXMEFIXME: ..do this is as well.
		}
	}

	if ( !self->lockCount && self->client->ps.torsoTimer <= 0 )
	{//don't interrupt laser sweep attack or other special attacks/moves
		if ( self->count < 4 && self->health > 100 && hitLoc != HL_GENERIC1 )
		{
			if ( self->delay < level.time )
			{
				int speech;
				switch( self->count )
				{
				default:
				case 0:
					speech = EV_PUSHED1;
					break;
				case 1:
					speech = EV_PUSHED2;
					break;
				case 2:
					speech = EV_PUSHED3;
					break;
				case 3:
					speech = EV_DETECTED1;
					break;
				}
				self->count++;
				self->NPC->blockedSpeechDebounceTime = 0;
				G_AddVoiceEvent( self, speech, Q_irand( 3000, 5000 ) );
				self->delay = level.time + Q_irand( 5000, 7000 );
			}
		}
		else
		{
			NPC_Pain(self, attacker, damage);
		}
	}
	else if ( hitLoc == HL_GENERIC1 )
	{
		NPC_SetPainEvent( self );
		//self->s.powerups |= ( 1 << PW_SHOCKED );
		//self->client->ps.powerups[PW_SHOCKED] = level.time + Q_irand( 500, 2500 );
		self->client->ps.electrifyTime = level.time + Q_irand(500, 2500);
	}

	if ( inflictor && inflictor->lastEnemy == self )
	{//He force-pushed my own lobfires back at me
		if ( mod == MOD_REPEATER_ALT && !Q_irand( 0, 2 ) )
		{
			if ( TIMER_Done( self, "noRapid" ) )
			{
				self->NPC->scriptFlags &= ~SCF_ALT_FIRE;
				self->alt_fire = qfalse;
				TIMER_Set( self, "noLob", Q_irand( 2000, 6000 ) );
			}
			else
			{//hopefully this will make us fire the laser
				TIMER_Set( self, "noLob", Q_irand( 1000, 2000 ) );
			}
		}
		else if ( mod == MOD_REPEATER && !Q_irand( 0, 5 ) )
		{
			if ( TIMER_Done( self, "noLob" ) )
			{
				self->NPC->scriptFlags |= SCF_ALT_FIRE;
				self->alt_fire = qtrue;
				TIMER_Set( self, "noRapid", Q_irand( 2000, 6000 ) );
			}
			else
			{//hopefully this will make us fire the laser
				TIMER_Set( self, "noRapid", Q_irand( 1000, 2000 ) );
			}
		}
	}
}

/*
-------------------------
GM_HoldPosition
-------------------------
*/

static void GM_HoldPosition( void )
{
	NPC_FreeCombatPoint( NPCInfo->combatPoint, qtrue );
	if ( !trap_ICARUS_TaskIDPending( NPC, TID_MOVE_NAV ) )
	{//don't have a script waiting for me to get to my point, okay to stop trying and stand
		NPCInfo->goalEntity = NULL;
	}
}

/*
-------------------------
GM_Move
-------------------------
*/
static qboolean GM_Move( void )
{
	qboolean moved;
	navInfo_t info;

	NPCInfo->combatMove = qtrue;//always move straight toward our goal

	moved = NPC_MoveToGoal( qtrue );
	
	//Get the move info
	NAV_GetLastMove( &info );

	//FIXME: if we bump into another one of our guys and can't get around him, just stop!
	//If we hit our target, then stop and fire!
	if ( info.flags & NIF_COLLISION ) 
	{
		if ( info.blocker == NPC->enemy )
		{
			GM_HoldPosition();
		}
	}

	//If our move failed, then reset
	if ( moved == qfalse )
	{//FIXME: if we're going to a combat point, need to pick a different one
		if ( !trap_ICARUS_TaskIDPending( NPC, TID_MOVE_NAV ) )
		{//can't transfer movegoal or stop when a script we're running is waiting to complete
			GM_HoldPosition();
		}
	}

	return moved;
}

/*
-------------------------
NPC_BSGM_Patrol
-------------------------
*/

void NPC_BSGM_Patrol( void )
{
	if ( NPC_CheckPlayerTeamStealth() )
	{
		NPC_UpdateAngles( qtrue, qtrue );
		return;
	}

	//If we have somewhere to go, then do that
	if ( UpdateGoal() )
	{
		ucmd.buttons |= BUTTON_WALKING;
		NPC_MoveToGoal( qtrue );
	}

	NPC_UpdateAngles( qtrue, qtrue );
}

/*
-------------------------
GM_CheckMoveState
-------------------------
*/

static void GM_CheckMoveState( void )
{
	if ( trap_ICARUS_TaskIDPending( NPC, TID_MOVE_NAV ) )
	{//moving toward a goal that a script is waiting on, so don't stop for anything!
		move4 = qtrue;
	}

	//See if we're moving towards a goal, not the enemy
	if ( ( NPCInfo->goalEntity != NPC->enemy ) && ( NPCInfo->goalEntity != NULL ) )
	{
		//Did we make it?
		if ( NAV_HitNavGoal( NPC->r.currentOrigin, NPC->r.mins, NPC->r.maxs, NPCInfo->goalEntity->r.currentOrigin, 16, qfalse ) || 
			( !trap_ICARUS_TaskIDPending( NPC, TID_MOVE_NAV ) && enemyLOS4 && enemyDist4 <= 10000 ) )
		{//either hit our navgoal or our navgoal was not a crucial (scripted) one (maybe a combat point) and we're scouting and found our enemy
			NPC_ReachedGoal();
			//don't attack right away
			TIMER_Set( NPC, "attackDelay", Q_irand( 250, 500 ) );	//FIXME: Slant for difficulty levels
			return;
		}
	}
}

/*
-------------------------
GM_CheckFireState
-------------------------
*/

static void GM_CheckFireState( void )
{
	if ( enemyCS4 )
	{//if have a clear shot, always try
		return;
	}

	if ( !VectorCompare( NPC->client->ps.velocity, vec3_origin ) )
	{//if moving at all, don't do this
		return;
	}

	//See if we should continue to fire on their last position
	if ( !hitAlly4 && NPCInfo->enemyLastSeenTime > 0 )
	{
		if ( level.time - NPCInfo->enemyLastSeenTime < 10000 )
		{
			if ( !Q_irand( 0, 10 ) )
			{
				//Fire on the last known position
				vec3_t	muzzle, dir, angles;
				qboolean tooClose = qfalse;
				qboolean tooFar = qfalse;
				float distThreshold;
				float dist;

				CalcEntitySpot( NPC, SPOT_HEAD, muzzle );
				if ( VectorCompare( impactPos4, vec3_origin ) )
				{//never checked ShotEntity this frame, so must do a trace...
					trace_t tr;
					//vec3_t	mins = {-2,-2,-2}, maxs = {2,2,2};
					vec3_t	forward, end;
					AngleVectors( NPC->client->ps.viewangles, forward, NULL, NULL );
					VectorMA( muzzle, 8192, forward, end );
					trap_Trace( &tr, muzzle, vec3_origin, vec3_origin, end, NPC->s.number, MASK_SHOT );
					VectorCopy( tr.endpos, impactPos4 );
				}

				//see if impact would be too close to me
				distThreshold = 16384/*128*128*/;//default
				if ( NPC->s.weapon == WP_REPEATER )
				{
					if ( NPCInfo->scriptFlags&SCF_ALT_FIRE )
					{
						distThreshold = 65536/*256*256*/;
					}
				}

				dist = DistanceSquared( impactPos4, muzzle );

				if ( dist < distThreshold )
				{//impact would be too close to me
					tooClose = qtrue;
				}
				else if ( level.time - NPCInfo->enemyLastSeenTime > 5000 )
				{//we've haven't seen them in the last 5 seconds
					//see if it's too far from where he is
					distThreshold = 65536/*256*256*/;//default
					if ( NPC->s.weapon == WP_REPEATER )
					{
						if ( NPCInfo->scriptFlags&SCF_ALT_FIRE )
						{
							distThreshold = 262144/*512*512*/;
						}
					}
					dist = DistanceSquared( impactPos4, NPCInfo->enemyLastSeenLocation );
					if ( dist > distThreshold )
					{//impact would be too far from enemy
						tooFar = qtrue;
					}
				}

				if ( !tooClose && !tooFar )
				{//okay too shoot at last pos
					VectorSubtract( NPCInfo->enemyLastSeenLocation, muzzle, dir );
					VectorNormalize( dir );
					vectoangles( dir, angles );

					NPCInfo->desiredYaw		= angles[YAW];
					NPCInfo->desiredPitch	= angles[PITCH];

					shoot4 = qtrue;
					faceEnemy4 = qfalse;
					return;
				}
			}
		}
	}
}

void NPC_GM_StartLaser( void )
{
	if ( !NPC->lockCount )
	{//haven't already started a laser attack
		//warm up for the beam attack
#if 0
		NPC_SetAnim( NPC, SETANIM_TORSO, TORSO_RAISEWEAP2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
#endif
		TIMER_Set( NPC, "beamDelay", NPC->client->ps.torsoTimer );
		TIMER_Set( NPC, "attackDelay", NPC->client->ps.torsoTimer+3000 );
		NPC->lockCount = 1;
		//turn on warmup effect
		G_PlayEffectID( G_EffectIndex("galak/beam_warmup"), NPC->r.currentOrigin, vec3_origin );
		G_SoundOnEnt( NPC, CHAN_AUTO, "sound/weapons/galak/lasercharge.wav" );
	}
}

void GM_StartGloat( void )
{
	NPC->wait = 0;
	NPC_SetSurfaceOnOff( NPC, "torso_galakface", TURN_ON );
	NPC_SetSurfaceOnOff( NPC, "torso_galakhead", TURN_ON );
	NPC_SetSurfaceOnOff( NPC, "torso_eyes_mouth", TURN_ON );
	NPC_SetSurfaceOnOff( NPC, "torso_collar", TURN_ON );
	NPC_SetSurfaceOnOff( NPC, "torso_galaktorso", TURN_ON );

	NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_STAND2TO1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	NPC->client->ps.legsTimer += 500;
	NPC->client->ps.torsoTimer += 500;
}
/*
-------------------------
NPC_BSGM_Attack
-------------------------
*/

void NPC_BSGM_Attack( void )
{
	//Don't do anything if we're hurt
	if ( NPC->painDebounceTime > level.time )
	{
		NPC_UpdateAngles( qtrue, qtrue );
		return;
	}

#if 0
	//FIXME: if killed enemy, use victory anim
	if ( NPC->enemy && NPC->enemy->health <= 0 
		&& !NPC->enemy->s.number )
	{//my enemy is dead
		if ( NPC->client->ps.torsoAnim == BOTH_STAND2TO1 )
		{
			if ( NPC->client->ps.torsoTimer <= 500 )
			{
				G_AddVoiceEvent( NPC, Q_irand( EV_VICTORY1, EV_VICTORY3 ), 3000 );
				NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_TRIUMPHANT1START, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				NPC->client->ps.legsTimer += 500;
				NPC->client->ps.torsoTimer += 500;
			}
		}
		else if ( NPC->client->ps.torsoAnim == BOTH_TRIUMPHANT1START )
		{
			if ( NPC->client->ps.torsoTimer <= 500 )
			{
				NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_TRIUMPHANT1STARTGESTURE, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				NPC->client->ps.legsTimer += 500;
				NPC->client->ps.torsoTimer += 500;
			}
		}
		else if ( NPC->client->ps.torsoAnim == BOTH_TRIUMPHANT1STARTGESTURE )
		{
			if ( NPC->client->ps.torsoTimer <= 500 )
			{
				NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_TRIUMPHANT1STOP, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				NPC->client->ps.legsTimer += 500;
				NPC->client->ps.torsoTimer += 500;
			}
		}
		else if ( NPC->client->ps.torsoAnim == BOTH_TRIUMPHANT1STOP )
		{
			if ( NPC->client->ps.torsoTimer <= 500 )
			{
				NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_STAND1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				NPC->client->ps.legsTimer = -1;
				NPC->client->ps.torsoTimer = -1;
			}
		}
		else if ( NPC->wait )
		{
			if ( TIMER_Done( NPC, "gloatTime" ) )
			{
				GM_StartGloat();
			}
			else if ( DistanceHorizontalSquared( NPC->client->renderInfo.eyePoint, NPC->enemy->r.currentOrigin ) > 4096 && (NPCInfo->scriptFlags&SCF_CHASE_ENEMIES) )//64 squared
			{
				NPCInfo->goalEntity = NPC->enemy;
				GM_Move();
			}
			else
			{//got there
				GM_StartGloat();
			}
		}
		NPC_FaceEnemy( qtrue );
		NPC_UpdateAngles( qtrue, qtrue );
		return;
	}
#endif

	//If we don't have an enemy, just idle
	if ( NPC_CheckEnemyExt(qfalse) == qfalse || !NPC->enemy )
	{
		NPC->enemy = NULL;
		NPC_BSGM_Patrol();
		return;
	}

	enemyLOS4 = enemyCS4 = qfalse;
	move4 = qtrue;
	faceEnemy4 = qfalse;
	shoot4 = qfalse;
	hitAlly4 = qfalse;
	VectorClear( impactPos4 );
	enemyDist4 = DistanceSquared( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin );

	//if ( NPC->client->ps.torsoAnim == BOTH_ATTACK4 ||
	//	NPC->client->ps.torsoAnim == BOTH_ATTACK5 )
	if (0)
	{
		shoot4 = qfalse;
		if ( TIMER_Done( NPC, "smackTime" ) && !NPCInfo->blockedDebounceTime )
		{//time to smack
			//recheck enemyDist4 and InFront
			if ( enemyDist4 < MELEE_DIST_SQUARED && InFront( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin, NPC->client->ps.viewangles, 0.3f ) )
			{
				vec3_t	smackDir;
				VectorSubtract( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin, smackDir );
				smackDir[2] += 30;
				VectorNormalize( smackDir );
				//hurt them
				G_Sound( NPC->enemy, CHAN_AUTO, G_SoundIndex( "sound/weapons/galak/skewerhit.wav" ) );
				G_Damage( NPC->enemy, NPC, NPC, smackDir, NPC->r.currentOrigin, (g_spskill.integer+1)*Q_irand( 5, 10), DAMAGE_NO_ARMOR|DAMAGE_NO_KNOCKBACK, MOD_CRUSH ); 
				if ( NPC->client->ps.torsoAnim == BOTH_ATTACK4 )
				{//smackdown
					int knockAnim = BOTH_KNOCKDOWN1;
					if ( BG_CrouchAnim( NPC->enemy->client->ps.legsAnim ) )
					{//knockdown from crouch
						knockAnim = BOTH_KNOCKDOWN4;
					}
					//throw them
					smackDir[2] = 1;
					VectorNormalize( smackDir );
					G_Throw( NPC->enemy, smackDir, 50 );
					NPC_SetAnim( NPC->enemy, SETANIM_BOTH, knockAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				}
				else
				{//uppercut
					//throw them
					G_Throw( NPC->enemy, smackDir, 100 );
					//make them backflip
					NPC_SetAnim( NPC->enemy, SETANIM_BOTH, BOTH_KNOCKDOWN5, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				}
				//done with the damage
				NPCInfo->blockedDebounceTime = 1;
			}
		}
	}
	else if ( NPC->lockCount ) //already shooting laser
	{//sometimes use the laser beam attack, but only after he's taken down our generator
		shoot4 = qfalse;
		if ( NPC->lockCount == 1 )
		{//charging up
			if ( TIMER_Done( NPC, "beamDelay" ) )
			{//time to start the beam
				int laserAnim;
				//if ( Q_irand( 0, 1 ) )
				if (1)
				{
					laserAnim = BOTH_ATTACK2;
				}
				/*
				else
				{
					laserAnim = BOTH_ATTACK7;
				}
				*/
				NPC_SetAnim( NPC, SETANIM_BOTH, laserAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				TIMER_Set( NPC, "attackDelay", NPC->client->ps.torsoTimer + Q_irand( 1000, 3000 ) );
				//turn on beam effect
				NPC->lockCount = 2;
				G_PlayEffectID( G_EffectIndex("galak/trace_beam"), NPC->r.currentOrigin, vec3_origin );
				NPC->s.loopSound = G_SoundIndex( "sound/weapons/galak/lasercutting.wav" );
				if ( !NPCInfo->coverTarg )
				{//for moving looping sound at end of trace
					NPCInfo->coverTarg = G_Spawn();
					if ( NPCInfo->coverTarg )
					{
						G_SetOrigin( NPCInfo->coverTarg, NPC->client->renderInfo.muzzlePoint );
						NPCInfo->coverTarg->r.svFlags |= SVF_BROADCAST;
						NPCInfo->coverTarg->s.loopSound = G_SoundIndex( "sound/weapons/galak/lasercutting.wav" );
					}
				}
			}
		}
		else
		{//in the actual attack now
			if ( NPC->client->ps.torsoTimer <= 0 )
			{//attack done!
				NPC->lockCount = 0;
				G_FreeEntity( NPCInfo->coverTarg );
				NPC->s.loopSound = 0;
#if 0
				NPC_SetAnim( NPC, SETANIM_TORSO, TORSO_DROPWEAP2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
#endif
				TIMER_Set( NPC, "attackDelay", NPC->client->ps.torsoTimer );
			}
			else
			{//attack still going
				//do the trace and damage
				trace_t	trace;
				vec3_t	end, mins={-3,-3,-3}, maxs={3,3,3};
				VectorMA( NPC->client->renderInfo.muzzlePoint, 1024, NPC->client->renderInfo.muzzleDir, end );
				trap_Trace( &trace, NPC->client->renderInfo.muzzlePoint, mins, maxs, end, NPC->s.number, MASK_SHOT );
				if ( trace.allsolid || trace.startsolid )
				{//oops, in a wall
					if ( NPCInfo->coverTarg )
					{
						G_SetOrigin( NPCInfo->coverTarg, NPC->client->renderInfo.muzzlePoint );
					}
				}
				else
				{//clear
					if ( trace.fraction < 1.0f )
					{//hit something
						gentity_t *traceEnt = &g_entities[trace.entityNum];
						if ( traceEnt && traceEnt->takedamage )
						{//damage it
							G_SoundAtLoc( trace.endpos, CHAN_AUTO, G_SoundIndex( "sound/weapons/galak/laserdamage.wav" ) );
							G_Damage( traceEnt, NPC, NPC, NPC->client->renderInfo.muzzleDir, trace.endpos, 10, 0, MOD_UNKNOWN );
						}
					}
					if ( NPCInfo->coverTarg )
					{
						G_SetOrigin( NPCInfo->coverTarg, trace.endpos );
					}
					if ( !Q_irand( 0, 5 ) )
					{
						G_SoundAtLoc( trace.endpos, CHAN_AUTO, G_SoundIndex( "sound/weapons/galak/laserdamage.wav" ) );
					}
				}
			}
		}
	}
	else 
	{//Okay, we're not in a special attack, see if we should switch weapons or start a special attack
		/*
		if ( NPC->s.weapon == WP_REPEATER 
			&& !(NPCInfo->scriptFlags & SCF_ALT_FIRE)//using rapid-fire
			&& NPC->enemy->s.weapon == WP_SABER //enemy using saber
			&& NPC->client && (NPC->client->ps.saberEventFlags&SEF_DEFLECTED)
			&& !Q_irand( 0, 50 ) )
		{//he's deflecting my shots, switch to the laser or the lob fire for a while
			TIMER_Set( NPC, "noRapid", Q_irand( 2000, 6000 ) );
			NPCInfo->scriptFlags |= SCF_ALT_FIRE;
			NPC->alt_fire = qtrue;
			if ( NPC->locationDamage[HL_GENERIC1] > GENERATOR_HEALTH && (Q_irand( 0, 1 )||enemyDist4 < MAX_LOB_DIST_SQUARED) )
			{//shield down, use laser
				NPC_GM_StartLaser();
			}
		}
		else*/
		if (// !NPC->client->ps.powerups[PW_GALAK_SHIELD] 
			1 //rwwFIXMEFIXME: just act like the shield is down til the effects and stuff are done
			&& enemyDist4 < MELEE_DIST_SQUARED 
			&& InFront( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin, NPC->client->ps.viewangles, 0.3f ) 
			&& NPC->enemy->localAnimIndex <= 1 )//within 80 and in front
		{//our shield is down, and enemy within 80, if very close, use melee attack to slap away
			if ( TIMER_Done( NPC, "attackDelay" ) )
			{
				//animate me
				int swingAnim = BOTH_ATTACK1;
#if 0
				if ( NPC->locationDamage[HL_GENERIC1] > GENERATOR_HEALTH )
				{//generator down, use random melee
					swingAnim = Q_irand( BOTH_ATTACK4, BOTH_ATTACK5 );//smackdown or uppercut
				}
				else
				{//always knock-away
					swingAnim = BOTH_ATTACK5;//uppercut
				}
#endif
				//FIXME: swing sound
				NPC_SetAnim( NPC, SETANIM_BOTH, swingAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				TIMER_Set( NPC, "attackDelay", NPC->client->ps.torsoTimer + Q_irand( 1000, 3000 ) );
				//delay the hurt until the proper point in the anim
				TIMER_Set( NPC, "smackTime", 600 );
				NPCInfo->blockedDebounceTime = 0;
				//FIXME: say something?
			}
		}
		else if ( !NPC->lockCount && NPC->locationDamage[HL_GENERIC1] > GENERATOR_HEALTH
			&& TIMER_Done( NPC, "attackDelay" )
			&& InFront( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin, NPC->client->ps.viewangles, 0.3f )
			&& ((!Q_irand( 0, 10*(2-g_spskill.integer))&& enemyDist4 > MIN_LOB_DIST_SQUARED&& enemyDist4 < MAX_LOB_DIST_SQUARED)
				||(!TIMER_Done( NPC, "noLob" )&&!TIMER_Done( NPC, "noRapid" ))) 
			&& NPC->enemy->s.weapon != WP_TURRET )
		{//sometimes use the laser beam attack, but only after he's taken down our generator
			shoot4 = qfalse;
			NPC_GM_StartLaser();
		}
		else if ( enemyDist4 < MIN_LOB_DIST_SQUARED 
			&& (NPC->enemy->s.weapon != WP_TURRET || Q_stricmp( "PAS", NPC->enemy->classname ))
			&& TIMER_Done( NPC, "noRapid" ) )//256
		{//enemy within 256
			if ( (NPC->client->ps.weapon == WP_REPEATER) && (NPCInfo->scriptFlags & SCF_ALT_FIRE) )
			{//shooting an explosive, but enemy too close, switch to primary fire
				NPCInfo->scriptFlags &= ~SCF_ALT_FIRE;
				NPC->alt_fire = qfalse;
				//FIXME: use weap raise & lower anims
				NPC_ChangeWeapon( WP_REPEATER );
			}
		}
		else if ( (enemyDist4 > MAX_LOB_DIST_SQUARED || (NPC->enemy->s.weapon == WP_TURRET && !Q_stricmp( "PAS", NPC->enemy->classname )))
			&& TIMER_Done( NPC, "noLob" ) )//448
		{//enemy more than 448 away and we are ready to try lob fire again
			if ( (NPC->client->ps.weapon == WP_REPEATER) && !(NPCInfo->scriptFlags & SCF_ALT_FIRE) )
			{//enemy far enough away to use lobby explosives
				NPCInfo->scriptFlags |= SCF_ALT_FIRE;
				NPC->alt_fire = qtrue;
				//FIXME: use weap raise & lower anims
				NPC_ChangeWeapon( WP_REPEATER );
			}
		}
	}

	//can we see our target?
	if ( NPC_ClearLOS4( NPC->enemy ) )
	{
		NPCInfo->enemyLastSeenTime = level.time;//used here for aim debouncing, not always a clear LOS
		enemyLOS4 = qtrue;

		if ( NPC->client->ps.weapon == WP_NONE )
		{
			enemyCS4 = qfalse;//not true, but should stop us from firing
			NPC_AimAdjust( -1 );//adjust aim worse longer we have no weapon
		}
		else
		{//can we shoot our target?
			if ( ((NPC->client->ps.weapon == WP_REPEATER && (NPCInfo->scriptFlags&SCF_ALT_FIRE))) && enemyDist4 < MIN_LOB_DIST_SQUARED )//256
			{
				enemyCS4 = qfalse;//not true, but should stop us from firing
				hitAlly4 = qtrue;//us!
				//FIXME: if too close, run away!
			}
			else
			{
				int hit = NPC_ShotEntity( NPC->enemy, impactPos4 );
				gentity_t *hitEnt = &g_entities[hit];
				if ( hit == NPC->enemy->s.number 
					|| ( hitEnt && hitEnt->client && hitEnt->client->playerTeam == NPC->client->enemyTeam )
					|| ( hitEnt && hitEnt->takedamage ) )
				{//can hit enemy or will hit glass or other breakable, so shoot anyway
					enemyCS4 = qtrue;
					NPC_AimAdjust( 2 );//adjust aim better longer we have clear shot at enemy
					VectorCopy( NPC->enemy->r.currentOrigin, NPCInfo->enemyLastSeenLocation );
				}
				else
				{//Hmm, have to get around this bastard
					NPC_AimAdjust( 1 );//adjust aim better longer we can see enemy
					if ( hitEnt && hitEnt->client && hitEnt->client->playerTeam == NPC->client->playerTeam )
					{//would hit an ally, don't fire!!!
						hitAlly4 = qtrue;
					}
					else
					{//Check and see where our shot *would* hit... if it's not close to the enemy (within 256?), then don't fire
					}
				}
			}
		}
	}
	else if ( trap_InPVS( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin ) )
	{
		int hit;
		gentity_t *hitEnt;

		if ( TIMER_Done( NPC, "talkDebounce" ) && !Q_irand( 0, 10 ) )
		{
			if ( NPCInfo->enemyCheckDebounceTime < 8 )
			{
				int speech = -1;
				switch( NPCInfo->enemyCheckDebounceTime )
				{
				case 0:
				case 1:
				case 2:
					speech = EV_CHASE1 + NPCInfo->enemyCheckDebounceTime;
					break;
				case 3:
				case 4:
				case 5:
					speech = EV_COVER1 + NPCInfo->enemyCheckDebounceTime-3;
					break;
				case 6:
				case 7:
					speech = EV_ESCAPING1 + NPCInfo->enemyCheckDebounceTime-6;
					break;
				}
				NPCInfo->enemyCheckDebounceTime++;
				if ( speech != -1 )
				{
					G_AddVoiceEvent( NPC, speech, Q_irand( 3000, 5000 ) );
					TIMER_Set( NPC, "talkDebounce", Q_irand( 5000, 7000 ) );
				}
			}
		}

		NPCInfo->enemyLastSeenTime = level.time;

		hit = NPC_ShotEntity( NPC->enemy, impactPos4 );
		hitEnt = &g_entities[hit];
		if ( hit == NPC->enemy->s.number 
			|| ( hitEnt && hitEnt->client && hitEnt->client->playerTeam == NPC->client->enemyTeam )
			|| ( hitEnt && hitEnt->takedamage ) )
		{//can hit enemy or will hit glass or other breakable, so shoot anyway
			enemyCS4 = qtrue;
		}
		else
		{
			faceEnemy4 = qtrue;
			NPC_AimAdjust( -1 );//adjust aim worse longer we cannot see enemy
		}
	}

	if ( enemyLOS4 )
	{
		faceEnemy4 = qtrue;
	}
	else
	{
		if ( !NPCInfo->goalEntity )
		{
			NPCInfo->goalEntity = NPC->enemy;
		}
		if ( NPCInfo->goalEntity == NPC->enemy )
		{//for now, always chase the enemy
			move4 = qtrue;
		}
	}
	if ( enemyCS4 )
	{
		shoot4 = qtrue;
		//NPCInfo->enemyCheckDebounceTime = level.time;//actually used here as a last actual LOS
	}
	else
	{
		if ( !NPCInfo->goalEntity )
		{
			NPCInfo->goalEntity = NPC->enemy;
		}
		if ( NPCInfo->goalEntity == NPC->enemy )
		{//for now, always chase the enemy
			move4 = qtrue;
		}
	}

	//Check for movement to take care of
	GM_CheckMoveState();

	//See if we should override shooting decision with any special considerations
	GM_CheckFireState();

	if ( NPC->client->ps.weapon == WP_REPEATER && (NPCInfo->scriptFlags&SCF_ALT_FIRE) && shoot4 && TIMER_Done( NPC, "attackDelay" ) )
	{
		vec3_t	muzzle;
		vec3_t	angles;
		vec3_t	target;
		vec3_t velocity = {0,0,0};
		vec3_t mins = {-REPEATER_ALT_SIZE,-REPEATER_ALT_SIZE,-REPEATER_ALT_SIZE}, maxs = {REPEATER_ALT_SIZE,REPEATER_ALT_SIZE,REPEATER_ALT_SIZE};
		qboolean clearshot;

		CalcEntitySpot( NPC, SPOT_WEAPON, muzzle );
		
		VectorCopy( NPC->enemy->r.currentOrigin, target );

		target[0] += flrand( -5, 5 )+(crandom()*(6-NPCInfo->currentAim)*2);
		target[1] += flrand( -5, 5 )+(crandom()*(6-NPCInfo->currentAim)*2);
		target[2] += flrand( -5, 5 )+(crandom()*(6-NPCInfo->currentAim)*2);

		//Find the desired angles
		clearshot = WP_LobFire( NPC, muzzle, target, mins, maxs, MASK_SHOT|CONTENTS_LIGHTSABER, 
			velocity, qtrue, NPC->s.number, NPC->enemy->s.number,
			300, 1100, 1500, qtrue );
		if ( VectorCompare( vec3_origin, velocity ) || (!clearshot&&enemyLOS4&&enemyCS4)  )
		{//no clear lob shot and no lob shot that will hit something breakable
			if ( enemyLOS4 && enemyCS4 && TIMER_Done( NPC, "noRapid" ) )
			{//have a clear straight shot, so switch to primary
				NPCInfo->scriptFlags &= ~SCF_ALT_FIRE;
				NPC->alt_fire = qfalse;
				NPC_ChangeWeapon( WP_REPEATER );
				//keep this weap for a bit
				TIMER_Set( NPC, "noLob", Q_irand( 500, 1000 ) );
			}
			else
			{
				shoot4 = qfalse;
			}
		}
		else
		{
			vectoangles( velocity, angles );

			NPCInfo->desiredYaw		= AngleNormalize360( angles[YAW] );
			NPCInfo->desiredPitch	= AngleNormalize360( angles[PITCH] );

			VectorCopy( velocity, NPC->client->hiddenDir );
			NPC->client->hiddenDist = VectorNormalize ( NPC->client->hiddenDir );
		}
	}
	else if ( faceEnemy4 )
	{//face the enemy
		NPC_FaceEnemy( qtrue );
	}

	if ( !TIMER_Done( NPC, "standTime" ) )
	{
		move4 = qfalse;
	}
	if ( !(NPCInfo->scriptFlags&SCF_CHASE_ENEMIES) )
	{//not supposed to chase my enemies
		if ( NPCInfo->goalEntity == NPC->enemy )
		{//goal is my entity, so don't move
			move4 = qfalse;
		}
	}

	if ( move4 && !NPC->lockCount )
	{//move toward goal
		if ( NPCInfo->goalEntity 
			/*&& NPC->client->ps.legsAnim != BOTH_ALERT1
			&& NPC->client->ps.legsAnim != BOTH_ATTACK2 
			&& NPC->client->ps.legsAnim != BOTH_ATTACK4
			&& NPC->client->ps.legsAnim != BOTH_ATTACK5 
			&& NPC->client->ps.legsAnim != BOTH_ATTACK7*/ )
		{
			move4 = GM_Move();
		}
		else
		{
			move4 = qfalse;
		}
	}

	if ( !TIMER_Done( NPC, "flee" ) )
	{//running away
		faceEnemy4 = qfalse;
	}

	//FIXME: check scf_face_move_dir here?

	if ( !faceEnemy4 )
	{//we want to face in the dir we're running
		if ( !move4 )
		{//if we haven't moved, we should look in the direction we last looked?
			VectorCopy( NPC->client->ps.viewangles, NPCInfo->lastPathAngles );
		}
		if ( move4 )
		{//don't run away and shoot
			NPCInfo->desiredYaw = NPCInfo->lastPathAngles[YAW];
			NPCInfo->desiredPitch = 0;
			shoot4 = qfalse;
		}
	}
	NPC_UpdateAngles( qtrue, qtrue );

	if ( NPCInfo->scriptFlags & SCF_DONT_FIRE )
	{
		shoot4 = qfalse;
	}

	if ( NPC->enemy && NPC->enemy->enemy )
	{
		if ( NPC->enemy->s.weapon == WP_SABER && NPC->enemy->enemy->s.weapon == WP_SABER )
		{//don't shoot at an enemy jedi who is fighting another jedi, for fear of injuring one or causing rogue blaster deflections (a la Obi Wan/Vader duel at end of ANH)
			shoot4 = qfalse;
		}
	}
	//FIXME: don't shoot right away!
	if ( shoot4 )
	{//try to shoot if it's time
		if ( TIMER_Done( NPC, "attackDelay" ) )
		{
			if( !(NPCInfo->scriptFlags & SCF_FIRE_WEAPON) ) // we've already fired, no need to do it again here
			{
				WeaponThink( qtrue );
			}
		}
	}

	//also:
	if ( NPC->enemy->s.weapon == WP_TURRET && !Q_stricmp( "PAS", NPC->enemy->classname ) )
	{//crush turrets
		if ( G_BoundsOverlap( NPC->r.absmin, NPC->r.absmax, NPC->enemy->r.absmin, NPC->enemy->r.absmax ) )
		{//have to do this test because placed turrets are not solid to NPCs (so they don't obstruct navigation)
			//if ( NPC->client->ps.powerups[PW_GALAK_SHIELD] > 0 )
			if (0)
			{
				NPC->client->ps.powerups[PW_BATTLESUIT] = level.time + ARMOR_EFFECT_TIME;
				G_Damage( NPC->enemy, NPC, NPC, NULL, NPC->r.currentOrigin, 100, DAMAGE_NO_KNOCKBACK, MOD_UNKNOWN ); 
			}
			else
			{
				G_Damage( NPC->enemy, NPC, NPC, NULL, NPC->r.currentOrigin, 100, DAMAGE_NO_KNOCKBACK, MOD_CRUSH ); 
			}
		}
	}
	else if ( NPCInfo->touchedByPlayer != NULL && NPCInfo->touchedByPlayer == NPC->enemy )
	{//touched enemy
		//if ( NPC->client->ps.powerups[PW_GALAK_SHIELD] > 0 )
		if (0)
		{//zap him!
			vec3_t	smackDir;

			//animate me
#if 0
			NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_ATTACK6, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
#endif
			TIMER_Set( NPC, "attackDelay", NPC->client->ps.torsoTimer );
			TIMER_Set( NPC, "standTime", NPC->client->ps.legsTimer );
			//FIXME: debounce this?
			NPCInfo->touchedByPlayer = NULL;
			//FIXME: some shield effect?
			NPC->client->ps.powerups[PW_BATTLESUIT] = level.time + ARMOR_EFFECT_TIME;

			VectorSubtract( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin, smackDir );
			smackDir[2] += 30;
			VectorNormalize( smackDir );
			G_Damage( NPC->enemy, NPC, NPC, smackDir, NPC->r.currentOrigin, (g_spskill.integer+1)*Q_irand( 5, 10), DAMAGE_NO_KNOCKBACK, MOD_UNKNOWN ); 
			//throw them
			G_Throw( NPC->enemy, smackDir, 100 );
			//NPC->enemy->s.powerups |= ( 1 << PW_SHOCKED );
			if ( NPC->enemy->client )
			{
			//	NPC->enemy->client->ps.powerups[PW_SHOCKED] = level.time + 1000;
				NPC->enemy->client->ps.electrifyTime = level.time + 1000;
			}
			//stop any attacks
			ucmd.buttons = 0;
		}
	}

	if ( NPCInfo->movementSpeech < 3 && NPCInfo->blockedSpeechDebounceTime <= level.time )
	{
		if ( NPC->enemy && NPC->enemy->health > 0 && NPC->enemy->painDebounceTime > level.time )
		{
			if ( NPC->enemy->health < 50 && NPCInfo->movementSpeech == 2 )
			{
				G_AddVoiceEvent( NPC, EV_ANGER2, Q_irand( 2000, 4000 ) );
				NPCInfo->movementSpeech = 3;
			}
			else if ( NPC->enemy->health < 75 && NPCInfo->movementSpeech == 1 )
			{
				G_AddVoiceEvent( NPC, EV_ANGER1, Q_irand( 2000, 4000 ) );
				NPCInfo->movementSpeech = 2;
			}
			else if ( NPC->enemy->health < 100 && NPCInfo->movementSpeech == 0 )
			{
				G_AddVoiceEvent( NPC, EV_ANGER3, Q_irand( 2000, 4000 ) );
				NPCInfo->movementSpeech = 1;
			}
		}
	}
}

void NPC_BSGM_Default( void )
{
	if( NPCInfo->scriptFlags & SCF_FIRE_WEAPON )
	{
		WeaponThink( qtrue );
	}
	
	if ( NPC->client->ps.stats[STAT_ARMOR] <= 0 )
	{//armor gone
	//	if ( !NPCInfo->investigateDebounceTime )
		if (0)
		{//start regenerating the armor
			NPC_SetSurfaceOnOff( NPC, "torso_shield", TURN_OFF );
			NPC->flags &= ~FL_SHIELDED;//no more reflections
			VectorSet( NPC->r.mins, -20, -20, -24 );
			VectorSet( NPC->r.maxs, 20, 20, 64 );
			NPC->client->ps.crouchheight = NPC->client->ps.standheight = 64;
			if ( NPC->locationDamage[HL_GENERIC1] < GENERATOR_HEALTH )
			{//still have the generator bolt-on
				if ( NPCInfo->investigateCount < 12 )
				{
					NPCInfo->investigateCount++;
				}
				NPCInfo->investigateDebounceTime = level.time + (NPCInfo->investigateCount * 5000);
			}
		}
		else if ( NPCInfo->investigateDebounceTime < level.time )
		{//armor regenerated, turn shield back on
			//do a trace and make sure we can turn this back on?
			trace_t	tr;
			trap_Trace( &tr, NPC->r.currentOrigin, shieldMins, shieldMaxs, NPC->r.currentOrigin, NPC->s.number, NPC->clipmask );
			if ( !tr.startsolid )
			{
				VectorCopy( shieldMins, NPC->r.mins );
				VectorCopy( shieldMaxs, NPC->r.maxs );
				NPC->client->ps.crouchheight = NPC->client->ps.standheight = shieldMaxs[2];
				NPC->client->ps.stats[STAT_ARMOR] = GALAK_SHIELD_HEALTH;
				NPCInfo->investigateDebounceTime = 0;
				NPC->flags |= FL_SHIELDED;//reflect normal shots
			//	NPC->fx_time = level.time;
				NPC_SetSurfaceOnOff( NPC, "torso_shield", TURN_ON );
			}
		}
	}
	/*
	if ( NPC->client->ps.stats[STAT_ARMOR] > 0 )
	{//armor present
		NPC->client->ps.powerups[PW_GALAK_SHIELD] = Q3_INFINITE;//temp, for effect
		NPC_SetSurfaceOnOff( NPC, "torso_shield", TURN_ON );
	}
	else
	{
		NPC_SetSurfaceOnOff( NPC, "torso_shield", TURN_OFF );
	}
	*/
	//rwwFIXMEFIXME: Allow this stuff, and again, going to have to let the client know about it.
	//Maybe a surface-off bitflag of some sort in the entity state?

	if( !NPC->enemy )
	{//don't have an enemy, look for one
		NPC_BSGM_Patrol();
	}
	else //if ( NPC->enemy )
	{//have an enemy
		NPC_BSGM_Attack();
	}
}
