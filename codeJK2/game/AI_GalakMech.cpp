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
#include "g_headers.h"

#include "b_local.h"
#include "g_nav.h"
#include "anims.h"
#include "wp_saber.h"

extern qboolean G_StandardHumanoid( const char *modelName );
extern void G_AddVoiceEvent( gentity_t *self, int event, int speakDebounceTime );
extern qboolean Q3_TaskIDPending( gentity_t *ent, taskID_t taskType );
extern void NPC_AimAdjust( int change );
extern qboolean WP_LobFire( gentity_t *self, vec3_t start, vec3_t target, vec3_t mins, vec3_t maxs, int clipmask,
				vec3_t velocity, qboolean tracePath, int ignoreEntNum, int enemyNum,
				float minSpeed, float maxSpeed, float idealSpeed, qboolean mustHit );
extern qboolean InFront( vec3_t spot, vec3_t from, vec3_t fromAngles, float threshHold = 0.0f );
extern void G_SoundAtSpot( vec3_t org, int soundIndex );
extern void G_SoundOnEnt (gentity_t *ent, soundChannel_t channel, const char *soundPath);
extern qboolean PM_CrouchAnim( int anim );
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

static qboolean enemyLOS;
static qboolean enemyCS;
static qboolean hitAlly;
static qboolean faceEnemy;
static qboolean AImove;
static qboolean shoot;
static float	enemyDist;
static vec3_t	impactPos;

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
		ent->client->ps.powerups[PW_GALAK_SHIELD] = Q3_INFINITE;//temp, for effect
		ent->fx_time = level.time;
		VectorSet( ent->mins, -60, -60, -24 );
		VectorSet( ent->maxs, 60, 60, 80 );
		ent->flags |= FL_NO_KNOCKBACK;//don't get pushed
		TIMER_Set( ent, "attackDelay", 0 );	//FIXME: Slant for difficulty levels
		TIMER_Set( ent, "flee", 0 );
		TIMER_Set( ent, "smackTime", 0 );
		TIMER_Set( ent, "beamDelay", 0 );
		TIMER_Set( ent, "noLob", 0 );
		TIMER_Set( ent, "noRapid", 0 );
		TIMER_Set( ent, "talkDebounce", 0 );
		gi.G2API_SetSurfaceOnOff( &ent->ghoul2[ent->playerModel], "torso_shield_off", TURN_ON );
		gi.G2API_SetSurfaceOnOff( &ent->ghoul2[ent->playerModel], "torso_galakface_off", TURN_OFF );
		gi.G2API_SetSurfaceOnOff( &ent->ghoul2[ent->playerModel], "torso_galakhead_off", TURN_OFF );
		gi.G2API_SetSurfaceOnOff( &ent->ghoul2[ent->playerModel], "torso_eyes_mouth_off", TURN_OFF );
		gi.G2API_SetSurfaceOnOff( &ent->ghoul2[ent->playerModel], "torso_collar_off", TURN_OFF );
		gi.G2API_SetSurfaceOnOff( &ent->ghoul2[ent->playerModel], "torso_galaktorso_off", TURN_OFF );
	}
	else
	{
//		gi.G2API_SetSurfaceOnOff( &ent->ghoul2[ent->playerModel], "helmet", TURN_OFF );
		gi.G2API_SetSurfaceOnOff( &ent->ghoul2[ent->playerModel], "torso_shield_off", TURN_OFF );
		gi.G2API_SetSurfaceOnOff( &ent->ghoul2[ent->playerModel], "torso_galakface_off", TURN_ON );
		gi.G2API_SetSurfaceOnOff( &ent->ghoul2[ent->playerModel], "torso_galakhead_off", TURN_ON );
		gi.G2API_SetSurfaceOnOff( &ent->ghoul2[ent->playerModel], "torso_eyes_mouth_off", TURN_ON );
		gi.G2API_SetSurfaceOnOff( &ent->ghoul2[ent->playerModel], "torso_collar_off", TURN_ON );
		gi.G2API_SetSurfaceOnOff( &ent->ghoul2[ent->playerModel], "torso_galaktorso_off", TURN_ON );
	}

}

//-----------------------------------------------------------------
static void GM_CreateExplosion( gentity_t *self, const int boltID, qboolean doSmall = qfalse )
{
	if ( boltID >=0 )
	{
		mdxaBone_t	boltMatrix;
		vec3_t		org, dir;

		gi.G2API_GetBoltMatrix( self->ghoul2, self->playerModel,
					boltID,
					&boltMatrix, self->currentAngles, self->currentOrigin, (cg.time?cg.time:level.time),
					NULL, self->s.modelScale );

		gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, org );
		gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_Y, dir );

		if ( doSmall )
		{
			G_PlayEffect( "env/small_explode2", org, dir );
		}
		else
		{
			G_PlayEffect( "env/med_explode2", org, dir );
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
		self->s.powerups |= ( 1 << PW_SHOCKED );
		self->client->ps.powerups[PW_SHOCKED] = level.time + 1000;
		if ( TIMER_Done( self, "dyingExplosion" ) )
		{
			int	newBolt;
			switch ( Q_irand( 1, 14 ) )
			{
			// Find place to generate explosion
			case 1:
				if (!gi.G2API_GetSurfaceRenderStatus( &self->ghoul2[self->playerModel], "r_hand" ))
				{//r_hand still there
					GM_CreateExplosion( self, self->handRBolt, qtrue );
					gi.G2API_SetSurfaceOnOff( &self->ghoul2[self->playerModel], "r_hand", TURN_OFF );
				}
				else if (!gi.G2API_GetSurfaceRenderStatus( &self->ghoul2[self->playerModel], "r_arm_middle" ))
				{//r_arm_middle still there
					newBolt = gi.G2API_AddBolt( &self->ghoul2[self->playerModel], "*r_arm_elbow" );
					gi.G2API_SetSurfaceOnOff( &self->ghoul2[self->playerModel], "r_arm_middle", TURN_OFF );
				}
				break;
			case 2:
				//FIXME: do only once?
				if (!gi.G2API_GetSurfaceRenderStatus( &self->ghoul2[self->playerModel], "l_hand" ))
				{//l_hand still there
					GM_CreateExplosion( self, self->handLBolt );
					gi.G2API_SetSurfaceOnOff( &self->ghoul2[self->playerModel], "l_hand", TURN_OFF );
				}
				else if (!gi.G2API_GetSurfaceRenderStatus( &self->ghoul2[self->playerModel], "l_arm_wrist" ))
				{//l_arm_wrist still there
					newBolt = gi.G2API_AddBolt( &self->ghoul2[self->playerModel], "*l_arm_cap_l_hand" );
					gi.G2API_SetSurfaceOnOff( &self->ghoul2[self->playerModel], "l_arm_wrist", TURN_OFF );
				}
				else if (!gi.G2API_GetSurfaceRenderStatus( &self->ghoul2[self->playerModel], "l_arm_middle" ))
				{//l_arm_middle still there
					newBolt = gi.G2API_AddBolt( &self->ghoul2[self->playerModel], "*l_arm_cap_l_hand" );
					gi.G2API_SetSurfaceOnOff( &self->ghoul2[self->playerModel], "l_arm_middle", TURN_OFF );
				}
				else if (!gi.G2API_GetSurfaceRenderStatus( &self->ghoul2[self->playerModel], "l_arm_augment" ))
				{//l_arm_augment still there
					newBolt = gi.G2API_AddBolt( &self->ghoul2[self->playerModel], "*l_arm_elbow" );
					gi.G2API_SetSurfaceOnOff( &self->ghoul2[self->playerModel], "l_arm_augment", TURN_OFF );
				}
				break;
			case 3:
			case 4:
				newBolt = gi.G2API_AddBolt( &self->ghoul2[self->playerModel], "*hip_fr" );
				GM_CreateExplosion( self, newBolt );
				break;
			case 5:
			case 6:
				newBolt = gi.G2API_AddBolt( &self->ghoul2[self->playerModel], "*shldr_l" );
				GM_CreateExplosion( self, newBolt );
				break;
			case 7:
			case 8:
				newBolt = gi.G2API_AddBolt( &self->ghoul2[self->playerModel], "*uchest_r" );
				GM_CreateExplosion( self, newBolt );
				break;
			case 9:
			case 10:
				GM_CreateExplosion( self, self->headBolt );
				break;
			case 11:
				newBolt = gi.G2API_AddBolt( &self->ghoul2[self->playerModel], "*l_leg_knee" );
				GM_CreateExplosion( self, newBolt, qtrue );
				break;
			case 12:
				newBolt = gi.G2API_AddBolt( &self->ghoul2[self->playerModel], "*r_leg_knee" );
				GM_CreateExplosion( self, newBolt, qtrue );
				break;
			case 13:
				newBolt = gi.G2API_AddBolt( &self->ghoul2[self->playerModel], "*l_leg_foot" );
				GM_CreateExplosion( self, newBolt, qtrue );
				break;
			case 14:
				newBolt = gi.G2API_AddBolt( &self->ghoul2[self->playerModel], "*r_leg_foot" );
				GM_CreateExplosion( self, newBolt, qtrue );
				break;
			}

			TIMER_Set( self, "dyingExplosion", Q_irand( 300, 1100 ) );
		}
	}
	else
	{//one final, huge explosion
		G_PlayEffect( "galak/explode", self->currentOrigin );
//		G_PlayEffect( "small_chunks", self->currentOrigin );
//		G_PlayEffect( "env/exp_trail_comp", self->currentOrigin, self->currentAngles );
		self->nextthink = level.time + FRAMETIME;
		self->e_ThinkFunc = thinkF_G_FreeEntity;
	}
}

/*
-------------------------
NPC_GM_Pain
-------------------------
*/

extern void NPC_SetPainEvent( gentity_t *self );
void NPC_GM_Pain( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, vec3_t point, int damage, int mod,int hitLoc )
{
	if ( self->client->ps.powerups[PW_GALAK_SHIELD] == 0 )
	{//shield is currently down
		//FIXME: allow for radius damage?
		if ( (hitLoc==HL_GENERIC1) && (self->locationDamage[HL_GENERIC1] > GENERATOR_HEALTH) )
		{
			int newBolt = gi.G2API_AddBolt( &self->ghoul2[self->playerModel], "*antenna_base" );
			if ( newBolt != -1 )
			{
				GM_CreateExplosion( self, newBolt );
			}

			gi.G2API_SetSurfaceOnOff( &self->ghoul2[self->playerModel], "torso_shield_off", TURN_OFF );
			gi.G2API_SetSurfaceOnOff( &self->ghoul2[self->playerModel], "torso_antenna", TURN_OFF );
			gi.G2API_SetSurfaceOnOff( &self->ghoul2[self->playerModel], "torso_antenna_base_cap_off", TURN_ON );
			self->client->ps.powerups[PW_GALAK_SHIELD] = 0;//temp, for effect
			self->client->ps.stats[STAT_ARMOR] = 0;//no more armor
			self->NPC->investigateDebounceTime = 0;//stop recharging

			NPC_SetAnim( self, SETANIM_BOTH, BOTH_ALERT1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			TIMER_Set( self, "attackDelay", self->client->ps.torsoAnimTimer );
			G_AddEvent( self, Q_irand( EV_DEATH1, EV_DEATH3 ), self->health );
		}
	}
	else
	{//store the point for shield impact
		if ( point )
		{
			VectorCopy( point, self->pos4 );
			self->client->poisonTime = level.time;
		}
	}

	if ( !self->lockCount && !self->client->ps.torsoAnimTimer )
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
			NPC_Pain( self, inflictor, attacker, point, damage, mod, hitLoc );
		}
	}
	else if ( hitLoc == HL_GENERIC1 )
	{
		NPC_SetPainEvent( self );
		self->s.powerups |= ( 1 << PW_SHOCKED );
		self->client->ps.powerups[PW_SHOCKED] = level.time + Q_irand( 500, 2500 );
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
	if ( !Q3_TaskIDPending( NPC, TID_MOVE_NAV ) )
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
	NPCInfo->combatMove = qtrue;//always move straight toward our goal

	qboolean	moved = NPC_MoveToGoal( qtrue );
	navInfo_t	info;

	//Get the move info
	NAV_GetLastMove( info );

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
		if ( !Q3_TaskIDPending( NPC, TID_MOVE_NAV ) )
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
	if ( Q3_TaskIDPending( NPC, TID_MOVE_NAV ) )
	{//moving toward a goal that a script is waiting on, so don't stop for anything!
		AImove = qtrue;
	}

	//See if we're moving towards a goal, not the enemy
	if ( ( NPCInfo->goalEntity != NPC->enemy ) && ( NPCInfo->goalEntity != NULL ) )
	{
		//Did we make it?
		if ( NAV_HitNavGoal( NPC->currentOrigin, NPC->mins, NPC->maxs, NPCInfo->goalEntity->currentOrigin, 16, qfalse ) ||
			( !Q3_TaskIDPending( NPC, TID_MOVE_NAV ) && enemyLOS && enemyDist <= 10000 ) )
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
	if ( enemyCS )
	{//if have a clear shot, always try
		return;
	}

	if ( !VectorCompare( NPC->client->ps.velocity, vec3_origin ) )
	{//if moving at all, don't do this
		return;
	}

	//See if we should continue to fire on their last position
	if ( !hitAlly && NPCInfo->enemyLastSeenTime > 0 )
	{
		if ( level.time - NPCInfo->enemyLastSeenTime < 10000 )
		{
			if ( !Q_irand( 0, 10 ) )
			{
				//Fire on the last known position
				vec3_t	muzzle, dir, angles;
				qboolean tooClose = qfalse;
				qboolean tooFar = qfalse;

				CalcEntitySpot( NPC, SPOT_HEAD, muzzle );
				if ( VectorCompare( impactPos, vec3_origin ) )
				{//never checked ShotEntity this frame, so must do a trace...
					trace_t tr;
					//vec3_t	mins = {-2,-2,-2}, maxs = {2,2,2};
					vec3_t	forward, end;
					AngleVectors( NPC->client->ps.viewangles, forward, NULL, NULL );
					VectorMA( muzzle, 8192, forward, end );
					gi.trace( &tr, muzzle, vec3_origin, vec3_origin, end, NPC->s.number, MASK_SHOT, G2_NOCOLLIDE, 0 );
					VectorCopy( tr.endpos, impactPos );
				}

				//see if impact would be too close to me
				float distThreshold = 16384/*128*128*/;//default
				if ( NPC->s.weapon == WP_REPEATER )
				{
					if ( NPCInfo->scriptFlags&SCF_ALT_FIRE )
					{
						distThreshold = 65536/*256*256*/;
					}
				}

				float dist = DistanceSquared( impactPos, muzzle );

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
					dist = DistanceSquared( impactPos, NPCInfo->enemyLastSeenLocation );
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

					shoot = qtrue;
					faceEnemy = qfalse;
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
		NPC_SetAnim( NPC, SETANIM_TORSO, TORSO_RAISEWEAP2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		TIMER_Set( NPC, "beamDelay", NPC->client->ps.torsoAnimTimer );
		TIMER_Set( NPC, "attackDelay", NPC->client->ps.torsoAnimTimer+3000 );
		NPC->lockCount = 1;
		//turn on warmup effect
		G_PlayEffect( "galak/beam_warmup", NPC->s.number );
		G_SoundOnEnt( NPC, CHAN_AUTO, "sound/weapons/galak/lasercharge.wav" );
	}
}

void GM_StartGloat( void )
{
	NPC->wait = 0;
	gi.G2API_SetSurfaceOnOff( &NPC->ghoul2[NPC->playerModel], "torso_galakface_off", TURN_ON );
	gi.G2API_SetSurfaceOnOff( &NPC->ghoul2[NPC->playerModel], "torso_galakhead_off", TURN_ON );
	gi.G2API_SetSurfaceOnOff( &NPC->ghoul2[NPC->playerModel], "torso_eyes_mouth_off", TURN_ON );
	gi.G2API_SetSurfaceOnOff( &NPC->ghoul2[NPC->playerModel], "torso_collar_off", TURN_ON );
	gi.G2API_SetSurfaceOnOff( &NPC->ghoul2[NPC->playerModel], "torso_galaktorso_off", TURN_ON );
	NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_STAND2TO1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	NPC->client->ps.legsAnimTimer += 500;
	NPC->client->ps.torsoAnimTimer += 500;
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

	//FIXME: if killed enemy, use victory anim
	if ( NPC->enemy && NPC->enemy->health <= 0
		&& !NPC->enemy->s.number )
	{//my enemy is dead
		if ( NPC->client->ps.torsoAnim == BOTH_STAND2TO1 )
		{
			if ( NPC->client->ps.torsoAnimTimer <= 500 )
			{
				G_AddVoiceEvent( NPC, Q_irand( EV_VICTORY1, EV_VICTORY3 ), 3000 );
				NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_TRIUMPHANT1START, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				NPC->client->ps.legsAnimTimer += 500;
				NPC->client->ps.torsoAnimTimer += 500;
			}
		}
		else if ( NPC->client->ps.torsoAnim == BOTH_TRIUMPHANT1START )
		{
			if ( NPC->client->ps.torsoAnimTimer <= 500 )
			{
				NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_TRIUMPHANT1STARTGESTURE, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				NPC->client->ps.legsAnimTimer += 500;
				NPC->client->ps.torsoAnimTimer += 500;
			}
		}
		else if ( NPC->client->ps.torsoAnim == BOTH_TRIUMPHANT1STARTGESTURE )
		{
			if ( NPC->client->ps.torsoAnimTimer <= 500 )
			{
				NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_TRIUMPHANT1STOP, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				NPC->client->ps.legsAnimTimer += 500;
				NPC->client->ps.torsoAnimTimer += 500;
			}
		}
		else if ( NPC->client->ps.torsoAnim == BOTH_TRIUMPHANT1STOP )
		{
			if ( NPC->client->ps.torsoAnimTimer <= 500 )
			{
				NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_STAND1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				NPC->client->ps.legsAnimTimer = -1;
				NPC->client->ps.torsoAnimTimer = -1;
			}
		}
		else if ( NPC->wait )
		{
			if ( TIMER_Done( NPC, "gloatTime" ) )
			{
				GM_StartGloat();
			}
			else if ( DistanceHorizontalSquared( NPC->client->renderInfo.eyePoint, NPC->enemy->currentOrigin ) > 4096 && (NPCInfo->scriptFlags&SCF_CHASE_ENEMIES) )//64 squared
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
	//If we don't have an enemy, just idle
	if ( NPC_CheckEnemyExt() == qfalse || !NPC->enemy )
	{
		NPC->enemy = NULL;
		NPC_BSGM_Patrol();
		return;
	}

	enemyLOS = enemyCS = qfalse;
	AImove = qtrue;
	faceEnemy = qfalse;
	shoot = qfalse;
	hitAlly = qfalse;
	VectorClear( impactPos );
	enemyDist = DistanceSquared( NPC->currentOrigin, NPC->enemy->currentOrigin );

	if ( NPC->client->ps.torsoAnim == BOTH_ATTACK4 ||
		NPC->client->ps.torsoAnim == BOTH_ATTACK5 )
	{
		shoot = qfalse;
		if ( TIMER_Done( NPC, "smackTime" ) && !NPCInfo->blockedDebounceTime )
		{//time to smack
			//recheck enemyDist and InFront
			if ( enemyDist < MELEE_DIST_SQUARED && InFront( NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f ) )
			{
				vec3_t	smackDir;
				VectorSubtract( NPC->enemy->currentOrigin, NPC->currentOrigin, smackDir );
				smackDir[2] += 30;
				VectorNormalize( smackDir );
				//hurt them
				G_Sound( NPC->enemy, G_SoundIndex( "sound/weapons/galak/skewerhit.wav" ) );
				G_Damage( NPC->enemy, NPC, NPC, smackDir, NPC->currentOrigin, (g_spskill->integer+1)*Q_irand( 5, 10), DAMAGE_NO_ARMOR|DAMAGE_NO_KNOCKBACK, MOD_CRUSH );
				if ( NPC->client->ps.torsoAnim == BOTH_ATTACK4 )
				{//smackdown
					int knockAnim = BOTH_KNOCKDOWN1;
					if ( PM_CrouchAnim( NPC->enemy->client->ps.legsAnim ) )
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
		shoot = qfalse;
		if ( NPC->lockCount == 1 )
		{//charging up
			if ( TIMER_Done( NPC, "beamDelay" ) )
			{//time to start the beam
				int laserAnim;
				if ( Q_irand( 0, 1 ) )
				{
					laserAnim = BOTH_ATTACK2;
				}
				else
				{
					laserAnim = BOTH_ATTACK7;
				}
				NPC_SetAnim( NPC, SETANIM_BOTH, laserAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				TIMER_Set( NPC, "attackDelay", NPC->client->ps.torsoAnimTimer + Q_irand( 1000, 3000 ) );
				//turn on beam effect
				NPC->lockCount = 2;
				G_PlayEffect( "galak/trace_beam", NPC->s.number );
				NPC->s.loopSound = G_SoundIndex( "sound/weapons/galak/lasercutting.wav" );
				if ( !NPCInfo->coverTarg )
				{//for moving looping sound at end of trace
					NPCInfo->coverTarg = G_Spawn();
					if ( NPCInfo->coverTarg )
					{
						G_SetOrigin( NPCInfo->coverTarg, NPC->client->renderInfo.muzzlePoint );
						NPCInfo->coverTarg->svFlags |= SVF_BROADCAST;
						NPCInfo->coverTarg->s.loopSound = G_SoundIndex( "sound/weapons/galak/lasercutting.wav" );
					}
				}
			}
		}
		else
		{//in the actual attack now
			if ( !NPC->client->ps.torsoAnimTimer )
			{//attack done!
				NPC->lockCount = 0;
				G_FreeEntity( NPCInfo->coverTarg );
				NPC->s.loopSound = 0;
				NPC_SetAnim( NPC, SETANIM_TORSO, TORSO_DROPWEAP2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				TIMER_Set( NPC, "attackDelay", NPC->client->ps.torsoAnimTimer );
			}
			else
			{//attack still going
				//do the trace and damage
				trace_t	trace;
				vec3_t	end, mins={-3,-3,-3}, maxs={3,3,3};
				VectorMA( NPC->client->renderInfo.muzzlePoint, 1024, NPC->client->renderInfo.muzzleDir, end );
				gi.trace( &trace, NPC->client->renderInfo.muzzlePoint, mins, maxs, end, NPC->s.number, MASK_SHOT, G2_NOCOLLIDE, 0 );
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
							G_SoundAtSpot( trace.endpos, G_SoundIndex( "sound/weapons/galak/laserdamage.wav" ) );
							G_Damage( traceEnt, NPC, NPC, NPC->client->renderInfo.muzzleDir, trace.endpos, 10, 0, MOD_ENERGY );
						}
					}
					if ( NPCInfo->coverTarg )
					{
						G_SetOrigin( NPCInfo->coverTarg, trace.endpos );
					}
					if ( !Q_irand( 0, 5 ) )
					{
						G_SoundAtSpot( trace.endpos, G_SoundIndex( "sound/weapons/galak/laserdamage.wav" ) );
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
			if ( NPC->locationDamage[HL_GENERIC1] > GENERATOR_HEALTH && (Q_irand( 0, 1 )||enemyDist < MAX_LOB_DIST_SQUARED) )
			{//shield down, use laser
				NPC_GM_StartLaser();
			}
		}
		else*/
		if ( !NPC->client->ps.powerups[PW_GALAK_SHIELD]
			&& enemyDist < MELEE_DIST_SQUARED
			&& InFront( NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f )
			&& G_StandardHumanoid( NPC->enemy->NPC_type ) )//within 80 and in front
		{//our shield is down, and enemy within 80, if very close, use melee attack to slap away
			if ( TIMER_Done( NPC, "attackDelay" ) )
			{
				//animate me
				int swingAnim;
				if ( NPC->locationDamage[HL_GENERIC1] > GENERATOR_HEALTH )
				{//generator down, use random melee
					swingAnim = Q_irand( BOTH_ATTACK4, BOTH_ATTACK5 );//smackdown or uppercut
				}
				else
				{//always knock-away
					swingAnim = BOTH_ATTACK5;//uppercut
				}
				//FIXME: swing sound
				NPC_SetAnim( NPC, SETANIM_BOTH, swingAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				TIMER_Set( NPC, "attackDelay", NPC->client->ps.torsoAnimTimer + Q_irand( 1000, 3000 ) );
				//delay the hurt until the proper point in the anim
				TIMER_Set( NPC, "smackTime", 600 );
				NPCInfo->blockedDebounceTime = 0;
				//FIXME: say something?
			}
		}
		else if ( !NPC->lockCount && NPC->locationDamage[HL_GENERIC1] > GENERATOR_HEALTH
			&& TIMER_Done( NPC, "attackDelay" )
			&& InFront( NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f )
			&& ((!Q_irand( 0, 10*(2-g_spskill->integer))&& enemyDist > MIN_LOB_DIST_SQUARED&& enemyDist < MAX_LOB_DIST_SQUARED)
				||(!TIMER_Done( NPC, "noLob" )&&!TIMER_Done( NPC, "noRapid" )))
			&& NPC->enemy->s.weapon != WP_TURRET )
		{//sometimes use the laser beam attack, but only after he's taken down our generator
			shoot = qfalse;
			NPC_GM_StartLaser();
		}
		else if ( enemyDist < MIN_LOB_DIST_SQUARED
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
		else if ( (enemyDist > MAX_LOB_DIST_SQUARED || (NPC->enemy->s.weapon == WP_TURRET && !Q_stricmp( "PAS", NPC->enemy->classname )))
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
	if ( NPC_ClearLOS( NPC->enemy ) )
	{
		NPCInfo->enemyLastSeenTime = level.time;//used here for aim debouncing, not always a clear LOS
		enemyLOS = qtrue;

		if ( NPC->client->ps.weapon == WP_NONE )
		{
			enemyCS = qfalse;//not true, but should stop us from firing
			NPC_AimAdjust( -1 );//adjust aim worse longer we have no weapon
		}
		else
		{//can we shoot our target?
			if ( ((NPC->client->ps.weapon == WP_REPEATER && (NPCInfo->scriptFlags&SCF_ALT_FIRE))) && enemyDist < MIN_LOB_DIST_SQUARED )//256
			{
				enemyCS = qfalse;//not true, but should stop us from firing
				hitAlly = qtrue;//us!
				//FIXME: if too close, run away!
			}
			else
			{
				int hit = NPC_ShotEntity( NPC->enemy, impactPos );
				gentity_t *hitEnt = &g_entities[hit];
				if ( hit == NPC->enemy->s.number
					|| ( hitEnt && hitEnt->client && hitEnt->client->playerTeam == NPC->client->enemyTeam )
					|| ( hitEnt && hitEnt->takedamage ) )
				{//can hit enemy or will hit glass or other breakable, so shoot anyway
					enemyCS = qtrue;
					NPC_AimAdjust( 2 );//adjust aim better longer we have clear shot at enemy
					VectorCopy( NPC->enemy->currentOrigin, NPCInfo->enemyLastSeenLocation );
				}
				else
				{//Hmm, have to get around this bastard
					NPC_AimAdjust( 1 );//adjust aim better longer we can see enemy
					if ( hitEnt && hitEnt->client && hitEnt->client->playerTeam == NPC->client->playerTeam )
					{//would hit an ally, don't fire!!!
						hitAlly = qtrue;
					}
					else
					{//Check and see where our shot *would* hit... if it's not close to the enemy (within 256?), then don't fire
					}
				}
			}
		}
	}
	else if ( gi.inPVS( NPC->enemy->currentOrigin, NPC->currentOrigin ) )
	{
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

		int hit = NPC_ShotEntity( NPC->enemy, impactPos );
		gentity_t *hitEnt = &g_entities[hit];
		if ( hit == NPC->enemy->s.number
			|| ( hitEnt && hitEnt->client && hitEnt->client->playerTeam == NPC->client->enemyTeam )
			|| ( hitEnt && hitEnt->takedamage ) )
		{//can hit enemy or will hit glass or other breakable, so shoot anyway
			enemyCS = qtrue;
		}
		else
		{
			faceEnemy = qtrue;
			NPC_AimAdjust( -1 );//adjust aim worse longer we cannot see enemy
		}
	}

	if ( enemyLOS )
	{
		faceEnemy = qtrue;
	}
	else
	{
		if ( !NPCInfo->goalEntity )
		{
			NPCInfo->goalEntity = NPC->enemy;
		}
		if ( NPCInfo->goalEntity == NPC->enemy )
		{//for now, always chase the enemy
			AImove = qtrue;
		}
	}
	if ( enemyCS )
	{
		shoot = qtrue;
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
			AImove = qtrue;
		}
	}

	//Check for movement to take care of
	GM_CheckMoveState();

	//See if we should override shooting decision with any special considerations
	GM_CheckFireState();

	if ( NPC->client->ps.weapon == WP_REPEATER && (NPCInfo->scriptFlags&SCF_ALT_FIRE) && shoot && TIMER_Done( NPC, "attackDelay" ) )
	{
		vec3_t	muzzle;
		vec3_t	angles;
		vec3_t	target;
		vec3_t velocity = {0,0,0};
		vec3_t mins = {-REPEATER_ALT_SIZE,-REPEATER_ALT_SIZE,-REPEATER_ALT_SIZE}, maxs = {REPEATER_ALT_SIZE,REPEATER_ALT_SIZE,REPEATER_ALT_SIZE};

		CalcEntitySpot( NPC, SPOT_WEAPON, muzzle );

		VectorCopy( NPC->enemy->currentOrigin, target );

		target[0] += Q_flrand( -5, 5 )+(Q_flrand(-1.0f, 1.0f)*(6-NPCInfo->currentAim)*2);
		target[1] += Q_flrand( -5, 5 )+(Q_flrand(-1.0f, 1.0f)*(6-NPCInfo->currentAim)*2);
		target[2] += Q_flrand( -5, 5 )+(Q_flrand(-1.0f, 1.0f)*(6-NPCInfo->currentAim)*2);

		//Find the desired angles
		qboolean clearshot = WP_LobFire( NPC, muzzle, target, mins, maxs, MASK_SHOT|CONTENTS_LIGHTSABER,
			velocity, qtrue, NPC->s.number, NPC->enemy->s.number,
			300, 1100, 1500, qtrue );
		if ( VectorCompare( vec3_origin, velocity ) || (!clearshot&&enemyLOS&&enemyCS)  )
		{//no clear lob shot and no lob shot that will hit something breakable
			if ( enemyLOS && enemyCS && TIMER_Done( NPC, "noRapid" ) )
			{//have a clear straight shot, so switch to primary
				NPCInfo->scriptFlags &= ~SCF_ALT_FIRE;
				NPC->alt_fire = qfalse;
				NPC_ChangeWeapon( WP_REPEATER );
				//keep this weap for a bit
				TIMER_Set( NPC, "noLob", Q_irand( 500, 1000 ) );
			}
			else
			{
				shoot = qfalse;
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
	else if ( faceEnemy )
	{//face the enemy
		NPC_FaceEnemy( qtrue );
	}

	if ( !TIMER_Done( NPC, "standTime" ) )
	{
		AImove = qfalse;
	}
	if ( !(NPCInfo->scriptFlags&SCF_CHASE_ENEMIES) )
	{//not supposed to chase my enemies
		if ( NPCInfo->goalEntity == NPC->enemy )
		{//goal is my entity, so don't move
			AImove = qfalse;
		}
	}

	if ( AImove && !NPC->lockCount )
	{//move toward goal
		if ( NPCInfo->goalEntity
			&& NPC->client->ps.legsAnim != BOTH_ALERT1
			&& NPC->client->ps.legsAnim != BOTH_ATTACK2
			&& NPC->client->ps.legsAnim != BOTH_ATTACK4
			&& NPC->client->ps.legsAnim != BOTH_ATTACK5
			&& NPC->client->ps.legsAnim != BOTH_ATTACK7 )
		{
			AImove = GM_Move();
		}
		else
		{
			AImove = qfalse;
		}
	}

	if ( !TIMER_Done( NPC, "flee" ) )
	{//running away
		faceEnemy = qfalse;
	}

	//FIXME: check scf_face_move_dir here?

	if ( !faceEnemy )
	{//we want to face in the dir we're running
		if ( !AImove )
		{//if we haven't moved, we should look in the direction we last looked?
			VectorCopy( NPC->client->ps.viewangles, NPCInfo->lastPathAngles );
		}
		if ( AImove )
		{//don't run away and shoot
			NPCInfo->desiredYaw = NPCInfo->lastPathAngles[YAW];
			NPCInfo->desiredPitch = 0;
			shoot = qfalse;
		}
	}
	NPC_UpdateAngles( qtrue, qtrue );

	if ( NPCInfo->scriptFlags & SCF_DONT_FIRE )
	{
		shoot = qfalse;
	}

	if ( NPC->enemy && NPC->enemy->enemy )
	{
		if ( NPC->enemy->s.weapon == WP_SABER && NPC->enemy->enemy->s.weapon == WP_SABER )
		{//don't shoot at an enemy jedi who is fighting another jedi, for fear of injuring one or causing rogue blaster deflections (a la Obi Wan/Vader duel at end of ANH)
			shoot = qfalse;
		}
	}
	//FIXME: don't shoot right away!
	if ( shoot )
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
		if ( G_BoundsOverlap( NPC->absmin, NPC->absmax, NPC->enemy->absmin, NPC->enemy->absmax ) )
		{//have to do this test because placed turrets are not solid to NPCs (so they don't obstruct navigation)
			if ( NPC->client->ps.powerups[PW_GALAK_SHIELD] > 0 )
			{
				NPC->client->ps.powerups[PW_BATTLESUIT] = level.time + ARMOR_EFFECT_TIME;
				G_Damage( NPC->enemy, NPC, NPC, NULL, NPC->currentOrigin, 100, DAMAGE_NO_KNOCKBACK, MOD_ELECTROCUTE );
			}
			else
			{
				G_Damage( NPC->enemy, NPC, NPC, NULL, NPC->currentOrigin, 100, DAMAGE_NO_KNOCKBACK, MOD_CRUSH );
			}
		}
	}
	else if ( NPCInfo->touchedByPlayer != NULL && NPCInfo->touchedByPlayer == NPC->enemy )
	{//touched enemy
		if ( NPC->client->ps.powerups[PW_GALAK_SHIELD] > 0 )
		{//zap him!
			//animate me
			NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_ATTACK6, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			TIMER_Set( NPC, "attackDelay", NPC->client->ps.torsoAnimTimer );
			TIMER_Set( NPC, "standTime", NPC->client->ps.legsAnimTimer );
			//FIXME: debounce this?
			NPCInfo->touchedByPlayer = NULL;
			//FIXME: some shield effect?
			NPC->client->ps.powerups[PW_BATTLESUIT] = level.time + ARMOR_EFFECT_TIME;
			vec3_t	smackDir;
			VectorSubtract( NPC->enemy->currentOrigin, NPC->currentOrigin, smackDir );
			smackDir[2] += 30;
			VectorNormalize( smackDir );
			G_Damage( NPC->enemy, NPC, NPC, smackDir, NPC->currentOrigin, (g_spskill->integer+1)*Q_irand( 5, 10), DAMAGE_NO_KNOCKBACK, MOD_ELECTROCUTE );
			//throw them
			G_Throw( NPC->enemy, smackDir, 100 );
			NPC->enemy->s.powerups |= ( 1 << PW_SHOCKED );
			if ( NPC->enemy->client )
			{
				NPC->enemy->client->ps.powerups[PW_SHOCKED] = level.time + 1000;
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
		if ( !NPCInfo->investigateDebounceTime )
		{//start regenerating the armor
			gi.G2API_SetSurfaceOnOff( &NPC->ghoul2[NPC->playerModel], "torso_shield_off", TURN_OFF );
			NPC->flags &= ~FL_SHIELDED;//no more reflections
			VectorSet( NPC->mins, -20, -20, -24 );
			VectorSet( NPC->maxs, 20, 20, 64 );
			NPC->client->crouchheight = NPC->client->standheight = 64;
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
			gi.trace( &tr, NPC->currentOrigin, shieldMins, shieldMaxs, NPC->currentOrigin, NPC->s.number, NPC->clipmask, G2_NOCOLLIDE, 0 );
			if ( !tr.startsolid )
			{
				VectorCopy( shieldMins, NPC->mins );
				VectorCopy( shieldMaxs, NPC->maxs );
				NPC->client->crouchheight = NPC->client->standheight = shieldMaxs[2];
				NPC->client->ps.stats[STAT_ARMOR] = GALAK_SHIELD_HEALTH;
				NPCInfo->investigateDebounceTime = 0;
				NPC->flags |= FL_SHIELDED;//reflect normal shots
				NPC->fx_time = level.time;
				gi.G2API_SetSurfaceOnOff( &NPC->ghoul2[NPC->playerModel], "torso_shield_off", TURN_ON );
			}
		}
	}
	if ( NPC->client->ps.stats[STAT_ARMOR] > 0 )
	{//armor present
		NPC->client->ps.powerups[PW_GALAK_SHIELD] = Q3_INFINITE;//temp, for effect
		gi.G2API_SetSurfaceOnOff( &NPC->ghoul2[NPC->playerModel], "torso_shield_off", TURN_ON );
	}
	else
	{
		gi.G2API_SetSurfaceOnOff( &NPC->ghoul2[NPC->playerModel], "torso_shield_off", TURN_OFF );
	}

	if( !NPC->enemy )
	{//don't have an enemy, look for one
		NPC_BSGM_Patrol();
	}
	else //if ( NPC->enemy )
	{//have an enemy
		NPC_BSGM_Attack();
	}
}
