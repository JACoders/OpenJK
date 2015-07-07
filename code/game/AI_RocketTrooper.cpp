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
extern qboolean PM_FlippingAnim( int anim );
extern void NPC_BSST_Patrol( void );

extern void RT_FlyStart( gentity_t *self );
extern qboolean Q3_TaskIDPending( gentity_t *ent, taskID_t taskType );

#define VELOCITY_DECAY		0.7f

#define RT_FLYING_STRAFE_VEL	60
#define RT_FLYING_STRAFE_DIS	200
#define RT_FLYING_UPWARD_PUSH	150

#define RT_FLYING_FORWARD_BASE_SPEED	50
#define RT_FLYING_FORWARD_MULTIPLIER	10

void RT_Precache( void )
{
	G_SoundIndex( "sound/chars/boba/bf_blast-off.wav" );
	G_SoundIndex( "sound/chars/boba/bf_jetpack_lp.wav" );
	G_SoundIndex( "sound/chars/boba/bf_land.wav" );
	G_EffectIndex( "rockettrooper/flameNEW" );
	G_EffectIndex( "rockettrooper/light_cone" );//extern this?  At least use a different one
}

extern void NPC_BehaviorSet_Stormtrooper( int bState );
void RT_RunStormtrooperAI( void )
{
	int bState;
	//Execute our bState
	if(NPCInfo->tempBehavior)
	{//Overrides normal behavior until cleared
		bState = NPCInfo->tempBehavior;
	}
	else
	{
		if(!NPCInfo->behaviorState)
			NPCInfo->behaviorState = NPCInfo->defaultBehavior;

		bState = NPCInfo->behaviorState;
	}
	NPC_BehaviorSet_Stormtrooper( bState );
}

void RT_FireDecide( void )
{
	qboolean enemyLOS = qfalse;
	qboolean enemyCS = qfalse;
	qboolean enemyInFOV = qfalse;
	//qboolean move = qtrue;
	qboolean shoot = qfalse;
	qboolean hitAlly = qfalse;
	vec3_t	impactPos;
	float	enemyDist;

	if ( NPC->client->ps.groundEntityNum == ENTITYNUM_NONE 
		&& NPC->client->ps.forceJumpZStart
		&& !PM_FlippingAnim( NPC->client->ps.legsAnim )
		&& !Q_irand( 0, 10 ) )
	{//take off
		RT_FlyStart( NPC );
	}

	if ( !NPC->enemy )
	{
		return;
	}

	VectorClear( impactPos );
	enemyDist = DistanceSquared( NPC->currentOrigin, NPC->enemy->currentOrigin );

	vec3_t	enemyDir, shootDir;
	VectorSubtract( NPC->enemy->currentOrigin, NPC->currentOrigin, enemyDir );
	VectorNormalize( enemyDir );
	AngleVectors( NPC->client->ps.viewangles, shootDir, NULL, NULL );
	float dot = DotProduct( enemyDir, shootDir );
	if ( dot > 0.5f ||( enemyDist * (1.0f-dot)) < 10000 )
	{//enemy is in front of me or they're very close and not behind me
		enemyInFOV = qtrue;
	}

	if ( enemyDist < MIN_ROCKET_DIST_SQUARED )//128
	{//enemy within 128
		if ( (NPC->client->ps.weapon == WP_FLECHETTE || NPC->client->ps.weapon == WP_REPEATER) && 
			(NPCInfo->scriptFlags & SCF_ALT_FIRE) )
		{//shooting an explosive, but enemy too close, switch to primary fire
			NPCInfo->scriptFlags &= ~SCF_ALT_FIRE;
			//FIXME: we can never go back to alt-fire this way since, after this, we don't know if we were initially supposed to use alt-fire or not...
		}
	}

	//can we see our target?
	if ( TIMER_Done( NPC, "nextAttackDelay" ) && TIMER_Done( NPC, "flameTime" ) )
	{
		if ( NPC_ClearLOS( NPC->enemy ) )
		{
			NPCInfo->enemyLastSeenTime = level.time;
			enemyLOS = qtrue;

			if ( NPC->client->ps.weapon == WP_NONE )
			{
				enemyCS = qfalse;//not true, but should stop us from firing
			}
			else
			{//can we shoot our target?
				if ( (NPC->client->ps.weapon == WP_ROCKET_LAUNCHER 
					|| (NPC->client->ps.weapon == WP_CONCUSSION && !(NPCInfo->scriptFlags&SCF_ALT_FIRE))
					|| (NPC->client->ps.weapon == WP_FLECHETTE && (NPCInfo->scriptFlags&SCF_ALT_FIRE))) && enemyDist < MIN_ROCKET_DIST_SQUARED )//128*128
				{
					enemyCS = qfalse;//not true, but should stop us from firing
					hitAlly = qtrue;//us!
					//FIXME: if too close, run away!
				}
				else if ( enemyInFOV )
				{//if enemy is FOV, go ahead and check for shooting
					int hit = NPC_ShotEntity( NPC->enemy, impactPos );
					gentity_t *hitEnt = &g_entities[hit];

					if ( hit == NPC->enemy->s.number 
						|| ( hitEnt && hitEnt->client && hitEnt->client->playerTeam == NPC->client->enemyTeam )
						|| ( hitEnt && hitEnt->takedamage && ((hitEnt->svFlags&SVF_GLASS_BRUSH)||hitEnt->health < 40||NPC->s.weapon == WP_EMPLACED_GUN) ) )
					{//can hit enemy or enemy ally or will hit glass or other minor breakable (or in emplaced gun), so shoot anyway
						enemyCS = qtrue;
						//NPC_AimAdjust( 2 );//adjust aim better longer we have clear shot at enemy
						VectorCopy( NPC->enemy->currentOrigin, NPCInfo->enemyLastSeenLocation );
					}
					else
					{//Hmm, have to get around this bastard
						//NPC_AimAdjust( 1 );//adjust aim better longer we can see enemy
						if ( hitEnt && hitEnt->client && hitEnt->client->playerTeam == NPC->client->playerTeam )
						{//would hit an ally, don't fire!!!
							hitAlly = qtrue;
						}
						else
						{//Check and see where our shot *would* hit... if it's not close to the enemy (within 256?), then don't fire
						}
					}
				}
				else
				{
					enemyCS = qfalse;//not true, but should stop us from firing
				}
			}
		}
		else if ( gi.inPVS( NPC->enemy->currentOrigin, NPC->currentOrigin ) )
		{
			NPCInfo->enemyLastSeenTime = level.time;
			//NPC_AimAdjust( -1 );//adjust aim worse longer we cannot see enemy
		}

		if ( NPC->client->ps.weapon == WP_NONE )
		{
			shoot = qfalse;
		}
		else
		{
			if ( enemyCS )
			{
				shoot = qtrue;
			}
		}

		if ( !enemyCS )
		{//if have a clear shot, always try
			//See if we should continue to fire on their last position
			//!TIMER_Done( NPC, "stick" ) || 
			if ( !hitAlly //we're not going to hit an ally
				&& enemyInFOV //enemy is in our FOV //FIXME: or we don't have a clear LOS?
				&& NPCInfo->enemyLastSeenTime > 0 )//we've seen the enemy
			{
				if ( level.time - NPCInfo->enemyLastSeenTime < 10000 )//we have seem the enemy in the last 10 seconds
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
							gi.trace( &tr, muzzle, vec3_origin, vec3_origin, end, NPC->s.number, MASK_SHOT, (EG2_Collision)0, 0 );
							VectorCopy( tr.endpos, impactPos );
						}

						//see if impact would be too close to me
						float distThreshold = 16384/*128*128*/;//default
						switch ( NPC->s.weapon )
						{
						case WP_ROCKET_LAUNCHER:
						case WP_FLECHETTE:
						case WP_THERMAL:
						case WP_TRIP_MINE:
						case WP_DET_PACK:
							distThreshold = 65536/*256*256*/;
							break;
						case WP_REPEATER:
							if ( NPCInfo->scriptFlags&SCF_ALT_FIRE )
							{
								distThreshold = 65536/*256*256*/;
							}
							break;
						case WP_CONCUSSION:
							if ( !(NPCInfo->scriptFlags&SCF_ALT_FIRE) )
							{
								distThreshold = 65536/*256*256*/;
							}
							break;
						default:
							break;
						}

						float dist = DistanceSquared( impactPos, muzzle );

						if ( dist < distThreshold )
						{//impact would be too close to me
							tooClose = qtrue;
						}
						else if ( level.time - NPCInfo->enemyLastSeenTime > 5000 ||
							(NPCInfo->group && level.time - NPCInfo->group->lastSeenEnemyTime > 5000 ))
						{//we've haven't seen them in the last 5 seconds
							//see if it's too far from where he is
							distThreshold = 65536/*256*256*/;//default
							switch ( NPC->s.weapon )
							{
							case WP_ROCKET_LAUNCHER:
							case WP_FLECHETTE:
							case WP_THERMAL:
							case WP_TRIP_MINE:
							case WP_DET_PACK:
								distThreshold = 262144/*512*512*/;
								break;
							case WP_REPEATER:
								if ( NPCInfo->scriptFlags&SCF_ALT_FIRE )
								{
									distThreshold = 262144/*512*512*/;
								}
								break;
							case WP_CONCUSSION:
								if ( !(NPCInfo->scriptFlags&SCF_ALT_FIRE) )
								{
									distThreshold = 262144/*512*512*/;
								}
								break;
							default:
								break;
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
						}
					}
				}
			}
		}

		//FIXME: don't shoot right away!
		if ( NPC->client->fireDelay )
		{
			if ( NPC->s.weapon == WP_ROCKET_LAUNCHER
				|| (NPC->s.weapon == WP_CONCUSSION&&!(NPCInfo->scriptFlags&SCF_ALT_FIRE)) )
			{
				if ( !enemyLOS || !enemyCS )
				{//cancel it
					NPC->client->fireDelay = 0;
				}
				else
				{//delay our next attempt
					TIMER_Set( NPC, "nextAttackDelay", Q_irand( 1000, 3000 ) );//FIXME: base on g_spskill
				}
			}
		}
		else if ( shoot )
		{//try to shoot if it's time
			if ( TIMER_Done( NPC, "nextAttackDelay" ) )
			{
				if( !(NPCInfo->scriptFlags & SCF_FIRE_WEAPON) ) // we've already fired, no need to do it again here
				{
					WeaponThink( qtrue );
				}
				//NASTY
				int altChance = 6;//FIXME: base on g_spskill
				if ( NPC->s.weapon == WP_ROCKET_LAUNCHER )
				{
					if ( (ucmd.buttons&BUTTON_ATTACK) 
						&& !Q_irand( 0, altChance ) )
					{//every now and then, shoot a homing rocket
						ucmd.buttons &= ~BUTTON_ATTACK;
						ucmd.buttons |= BUTTON_ALT_ATTACK;
						NPC->client->fireDelay = Q_irand( 1000, 3000 );//FIXME: base on g_spskill
					}
				}
				else if ( NPC->s.weapon == WP_CONCUSSION )
				{
					if ( (ucmd.buttons&BUTTON_ATTACK) 
						&& Q_irand( 0, altChance*5 ) )
					{//fire the beam shot
						ucmd.buttons &= ~BUTTON_ATTACK;
						ucmd.buttons |= BUTTON_ALT_ATTACK;
						TIMER_Set( NPC, "nextAttackDelay", Q_irand( 1500, 2500 ) );//FIXME: base on g_spskill
					}
					else
					{//fire the rocket-like shot
						TIMER_Set( NPC, "nextAttackDelay", Q_irand( 3000, 5000 ) );//FIXME: base on g_spskill
					}
				}
			}
		}
	}
}

//=====================================================================================
//FLYING behavior 
//=====================================================================================
qboolean RT_Flying( gentity_t *self )
{
	return ((qboolean)(self->client->moveType==MT_FLYSWIM));
}

void RT_FlyStart( gentity_t *self )
{//switch to seeker AI for a while
	if ( TIMER_Done( self, "jetRecharge" ) 
		&& !RT_Flying( self ) )
	{
		self->client->ps.gravity = 0;
		self->svFlags |= SVF_CUSTOM_GRAVITY;
		self->client->moveType = MT_FLYSWIM;
		//Inform NPC_HandleAIFlags we want to fly
		
		if (self->NPC){
			self->NPC->aiFlags |= NPCAI_FLY;
			self->lastInAirTime = level.time;
		}
		
		//start jet effect
		self->client->jetPackTime = Q3_INFINITE;
		if ( self->genericBolt1 != -1 )
		{
			G_PlayEffect( G_EffectIndex( "rockettrooper/flameNEW" ), self->playerModel, self->genericBolt1, self->s.number, self->currentOrigin, qtrue, qtrue );
		}
		if ( self->genericBolt2 != -1 )
		{
			G_PlayEffect( G_EffectIndex( "rockettrooper/flameNEW" ), self->playerModel, self->genericBolt2, self->s.number, self->currentOrigin, qtrue, qtrue );
		}

		//take-off sound
		G_SoundOnEnt( self, CHAN_ITEM, "sound/chars/boba/bf_blast-off.wav" );
		//jet loop sound
		self->s.loopSound = G_SoundIndex( "sound/chars/boba/bf_jetpack_lp.wav"  );
		if ( self->NPC )
		{
			self->count = Q3_INFINITE; // SEEKER shot ammo count
		}
	}
}

void RT_FlyStop( gentity_t *self )
{
	self->client->ps.gravity = g_gravity->value;
	self->svFlags &= ~SVF_CUSTOM_GRAVITY;
	self->client->moveType = MT_RUNJUMP;
	//Stop the effect
	self->client->jetPackTime = 0;
	if ( self->genericBolt1 != -1 )
	{
		G_StopEffect("rockettrooper/flameNEW", self->playerModel, self->genericBolt1, self->s.number );
	}
	if ( self->genericBolt2 != -1 )
	{
		G_StopEffect("rockettrooper/flameNEW", self->playerModel, self->genericBolt2, self->s.number );
	}
	//stop jet loop sound
	self->s.loopSound = 0;
	G_SoundOnEnt( self, CHAN_ITEM, "sound/chars/boba/bf_land.wav" );

	if ( self->NPC )
	{
		self->count = 0; // SEEKER shot ammo count
		TIMER_Set( self, "jetRecharge", Q_irand( 1000, 5000 ) );
		TIMER_Set( self, "jumpChaseDebounce", Q_irand( 500, 2000 ) );
	}
}

void RT_JetPackEffect( int duration )
{
	if ( NPC->genericBolt1 != -1 )
	{
		G_PlayEffect( G_EffectIndex( "rockettrooper/flameNEW" ), NPC->playerModel, NPC->genericBolt1, NPC->s.number, NPC->currentOrigin, duration, qtrue );
	}
	if ( NPC->genericBolt2 != -1 )
	{
		G_PlayEffect( G_EffectIndex( "rockettrooper/flameNEW" ), NPC->playerModel, NPC->genericBolt2, NPC->s.number, NPC->currentOrigin, duration, qtrue );
	}

	//take-off sound
	G_SoundOnEnt( NPC, CHAN_ITEM, "sound/chars/boba/bf_blast-off.wav" );
}

void RT_Flying_ApplyFriction( float frictionScale )
{
	if ( NPC->client->ps.velocity[0] )
	{
		NPC->client->ps.velocity[0] *= VELOCITY_DECAY;///frictionScale;

		if ( fabs( NPC->client->ps.velocity[0] ) < 1 )
		{
			NPC->client->ps.velocity[0] = 0;
		}
	}

	if ( NPC->client->ps.velocity[1] )
	{
		NPC->client->ps.velocity[1] *= VELOCITY_DECAY;///frictionScale;

		if ( fabs( NPC->client->ps.velocity[1] ) < 1 )
		{
			NPC->client->ps.velocity[1] = 0;
		}
	}
}

void RT_Flying_MaintainHeight( void )
{	
	float	dif = 0;

	// Update our angles regardless
	NPC_UpdateAngles( qtrue, qtrue );

	if ( NPC->forcePushTime > level.time )
	{//if being pushed, we don't have control over our movement
		return;
	}

	if ( (NPC->client->ps.pm_flags&PMF_TIME_KNOCKBACK) )
	{//don't slow down for a bit
		if ( NPC->client->ps.pm_time > 0 )
		{
			VectorScale( NPC->client->ps.velocity, 0.9f, NPC->client->ps.velocity );
			return;
		}
	}

	/*
	if ( (NPC->client->ps.eFlags&EF_FORCE_GRIPPED) )
	{
		RT_Flying_ApplyFriction( 3.0f );
		return;
	}
	*/
	// If we have an enemy, we should try to hover at or a little below enemy eye level
	if ( NPC->enemy 
		&& (!Q3_TaskIDPending( NPC, TID_MOVE_NAV ) || !NPCInfo->goalEntity ) )
	{
		if (TIMER_Done( NPC, "heightChange" ))
		{
			TIMER_Set( NPC,"heightChange",Q_irand( 1000, 3000 ));
			
			float enemyZHeight = NPC->enemy->currentOrigin[2];
			if ( NPC->enemy->client 
				&& NPC->enemy->client->ps.groundEntityNum == ENTITYNUM_NONE
				&& (NPC->enemy->client->ps.forcePowersActive&(1<<FP_LEVITATION)) )
			{//so we don't go up when they force jump up at us
				enemyZHeight = NPC->enemy->client->ps.forceJumpZStart;
			}

			// Find the height difference
			dif = (enemyZHeight +  Q_flrand( NPC->enemy->maxs[2]/2, NPC->enemy->maxs[2]+8 )) - NPC->currentOrigin[2]; 

			float	difFactor = 10.0f;

			// cap to prevent dramatic height shifts
			if ( fabs( dif ) > 2*difFactor )
			{
				if ( fabs( dif ) > 20*difFactor )
				{
					dif = ( dif < 0 ? -20*difFactor : 20*difFactor );
				}

				NPC->client->ps.velocity[2] = (NPC->client->ps.velocity[2]+dif)/2;
			}
			NPC->client->ps.velocity[2] *= Q_flrand( 0.85f, 1.25f );
		}
		else
		{//don't get too far away from height of enemy...
			float enemyZHeight = NPC->enemy->currentOrigin[2];
			if ( NPC->enemy->client 
				&& NPC->enemy->client->ps.groundEntityNum == ENTITYNUM_NONE
				&& (NPC->enemy->client->ps.forcePowersActive&(1<<FP_LEVITATION)) )
			{//so we don't go up when they force jump up at us
				enemyZHeight = NPC->enemy->client->ps.forceJumpZStart;
			}
			dif = NPC->currentOrigin[2] - (enemyZHeight+64);
			float maxHeight = 200;
			float hDist = DistanceHorizontal( NPC->enemy->currentOrigin, NPC->currentOrigin );
			if ( hDist < 512 )
			{
				maxHeight *= hDist/512;
			}
			if ( dif > maxHeight )
			{
				if ( NPC->client->ps.velocity[2] > 0 )//FIXME: or: we can't see him anymore
				{//slow down
					if ( NPC->client->ps.velocity[2] )
					{
						NPC->client->ps.velocity[2] *= VELOCITY_DECAY;

						if ( fabs( NPC->client->ps.velocity[2] ) < 2 )
						{
							NPC->client->ps.velocity[2] = 0;
						}
					}
				}
				else
				{//start coming back down
					NPC->client->ps.velocity[2] -= 4;
				}
			}
			else if ( dif < -200 && NPC->client->ps.velocity[2] < 0 )//we're way below him
			{
				if ( NPC->client->ps.velocity[2] < 0 )//FIXME: or: we can't see him anymore
				{//slow down
					if ( NPC->client->ps.velocity[2] )
					{
						NPC->client->ps.velocity[2] *= VELOCITY_DECAY;

						if ( fabs( NPC->client->ps.velocity[2] ) > -2 )
						{
							NPC->client->ps.velocity[2] = 0;
						}
					}
				}
				else
				{//start going back up
					NPC->client->ps.velocity[2] += 4;
				}
			}
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
			dif = goal->currentOrigin[2] - NPC->currentOrigin[2];
		}
		else if ( VectorCompare( NPC->pos1, vec3_origin ) )
		{//have a starting position as a reference point
			dif = NPC->pos1[2] - NPC->currentOrigin[2];
		}

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
	RT_Flying_ApplyFriction( 1.0f );
}

void RT_Flying_Strafe( void )
{
	int		side;
	vec3_t	end, right, dir;
	trace_t	tr;

	if ( random() > 0.7f 
		|| !NPC->enemy 
		|| !NPC->enemy->client )
	{
		// Do a regular style strafe
		AngleVectors( NPC->client->renderInfo.eyeAngles, NULL, right, NULL );

		// Pick a random strafe direction, then check to see if doing a strafe would be
		//	reasonably valid
		side = ( rand() & 1 ) ? -1 : 1;
		VectorMA( NPC->currentOrigin, RT_FLYING_STRAFE_DIS * side, right, end );

		gi.trace( &tr, NPC->currentOrigin, NULL, NULL, end, NPC->s.number, MASK_SOLID, (EG2_Collision)0, 0 );

		// Close enough
		if ( tr.fraction > 0.9f )
		{
			float vel = RT_FLYING_STRAFE_VEL+Q_flrand(-20,20);
			VectorMA( NPC->client->ps.velocity, vel*side, right, NPC->client->ps.velocity );
			if ( !Q_irand( 0, 3 ) )
			{
				// Add a slight upward push
				float upPush = RT_FLYING_UPWARD_PUSH;
				if ( NPC->client->ps.velocity[2] < 300 )
				{
					if ( NPC->client->ps.velocity[2] < 300+upPush )
					{
						NPC->client->ps.velocity[2] += upPush;
					}
					else
					{
						NPC->client->ps.velocity[2] = 300;
					}
				}
			}

			NPCInfo->standTime = level.time + 1000 + random() * 500;
		}
	}
	else
	{
		// Do a strafe to try and keep on the side of their enemy
		AngleVectors( NPC->enemy->client->renderInfo.eyeAngles, dir, right, NULL );

		// Pick a random side
		side = ( rand() & 1 ) ? -1 : 1;
		float	stDis = RT_FLYING_STRAFE_DIS*2.0f;
		VectorMA( NPC->enemy->currentOrigin, stDis * side, right, end );

		// then add a very small bit of random in front of/behind the player action
		VectorMA( end, crandom() * 25, dir, end );

		gi.trace( &tr, NPC->currentOrigin, NULL, NULL, end, NPC->s.number, MASK_SOLID, (EG2_Collision)0, 0 );

		// Close enough
		if ( tr.fraction > 0.9f )
		{
			float vel = (RT_FLYING_STRAFE_VEL*4)+Q_flrand(-20,20);
			VectorSubtract( tr.endpos, NPC->currentOrigin, dir );
			dir[2] *= 0.25; // do less upward change
			float dis = VectorNormalize( dir );
			if ( dis > vel )
			{
				dis = vel;
			}
			// Try to move the desired enemy side
			VectorMA( NPC->client->ps.velocity, dis, dir, NPC->client->ps.velocity );

			if ( !Q_irand( 0, 3 ) )
			{
				float upPush = RT_FLYING_UPWARD_PUSH;
				// Add a slight upward push
				if ( NPC->client->ps.velocity[2] < 300 )
				{
					if ( NPC->client->ps.velocity[2] < 300+upPush )
					{
						NPC->client->ps.velocity[2] += upPush;
					}
					else
					{
						NPC->client->ps.velocity[2] = 300;
					}
				}
				else if ( NPC->client->ps.velocity[2] > 300 )
				{
					NPC->client->ps.velocity[2] = 300;
				}
			}

			NPCInfo->standTime = level.time + 2500 + random() * 500;
		}
	}
}

void RT_Flying_Hunt( qboolean visible, qboolean advance )
{
	float	distance, speed;
	vec3_t	forward;

	if ( NPC->forcePushTime >= level.time )
		//|| (NPC->client->ps.eFlags&EF_FORCE_GRIPPED) )
	{//if being pushed, we don't have control over our movement
		NPC->delay = 0;
		return;
	}
	NPC_FaceEnemy( qtrue );

	// If we're not supposed to stand still, pursue the player
	if ( NPCInfo->standTime < level.time )
	{
		// Only strafe when we can see the player
		if ( visible )
		{
			NPC->delay = 0;
			RT_Flying_Strafe();
			return;
		}
	}

	// If we don't want to advance, stop here
	if ( advance  )
	{
		// Only try and navigate if the player is visible
		if ( visible == qfalse )
		{
			// Move towards our goal
			NPCInfo->goalEntity = NPC->enemy;
			NPCInfo->goalRadius = 24;

			NPC->delay = 0;
			NPC_MoveToGoal(qtrue);
			return;

		}
	}
	//else move straight at/away from him
	VectorSubtract( NPC->enemy->currentOrigin, NPC->currentOrigin, forward );
	forward[2] *= 0.1f;
	distance = VectorNormalize( forward );

	speed = RT_FLYING_FORWARD_BASE_SPEED + RT_FLYING_FORWARD_MULTIPLIER * g_spskill->integer;
	if ( advance && distance < Q_flrand( 256, 3096 ) )
	{
		NPC->delay = 0;
		VectorMA( NPC->client->ps.velocity, speed, forward, NPC->client->ps.velocity );
	}
	else if ( distance < Q_flrand( 0, 128 ) )
	{
		if ( NPC->health <= 50 )
		{//always back off
			NPC->delay = 0;
		}
		else if ( !TIMER_Done( NPC, "backoffTime" ) )
		{//still backing off from end of last delay
			NPC->delay = 0;
		}
		else if ( !NPC->delay )
		{//start a new delay
			NPC->delay = Q_irand( 0, 10+(20*(2-g_spskill->integer)) );
		}
		else
		{//continue the current delay
			NPC->delay--;
		}
		if ( !NPC->delay )
		{//delay done, now back off for a few seconds!
			TIMER_Set( NPC, "backoffTime", Q_irand( 2000, 5000 ) );
			VectorMA( NPC->client->ps.velocity, speed*-2, forward, NPC->client->ps.velocity );
		}
	}
	else
	{
		NPC->delay = 0;
	}
}

void RT_Flying_Ranged( qboolean visible, qboolean advance )
{
	if ( NPCInfo->scriptFlags & SCF_CHASE_ENEMIES )
	{
		RT_Flying_Hunt( visible, advance );
	}
}

void RT_Flying_Attack( void )
{
	// Always keep a good height off the ground
	RT_Flying_MaintainHeight();

	// Rate our distance to the target, and our visibilty
	float		distance	= DistanceHorizontalSquared( NPC->currentOrigin, NPC->enemy->currentOrigin );	
	qboolean	visible		= NPC_ClearLOS( NPC->enemy );
	qboolean	advance		= (qboolean)(distance>(256.0f*256.0f));

	// If we cannot see our target, move to see it
	if ( visible == qfalse )
	{
		if ( NPCInfo->scriptFlags & SCF_CHASE_ENEMIES )
		{
			RT_Flying_Hunt( visible, advance );
			return;
		}
	}

	RT_Flying_Ranged( visible, advance );
}

void RT_Flying_Think( void )
{
	if ( Q3_TaskIDPending( NPC, TID_MOVE_NAV ) 
		&& UpdateGoal() )
	{//being scripted to go to a certain spot, don't maintain height
		if ( NPC_MoveToGoal( qtrue ) )
		{//we could macro-nav to our goal
			if ( NPC->enemy && NPC->enemy->health && NPC->enemy->inuse )
			{
				NPC_FaceEnemy( qtrue );
				RT_FireDecide();
			}
		}
		else
		{//frick, no where to nav to, keep us in the air!
			RT_Flying_MaintainHeight();
		}
		return;
	}

	if ( NPC->random == 0.0f )
	{
		// used to offset seekers around a circle so they don't occupy the same spot.  This is not a fool-proof method.
		NPC->random = random() * 6.3f; // roughly 2pi
	}

	if ( NPC->enemy && NPC->enemy->health && NPC->enemy->inuse )
	{
		RT_Flying_Attack();
		RT_FireDecide();
		return;
	}
	else
	{
		RT_Flying_MaintainHeight();
		RT_RunStormtrooperAI();
		return;
	}
}


//=====================================================================================
//ON GROUND WITH ENEMY behavior
//=====================================================================================


//=====================================================================================
//DEFAULT behavior
//=====================================================================================
extern void RT_CheckJump( void );
void NPC_BSRT_Default( void )
{
	//FIXME: custom pain and death funcs:
		//pain3 is in air
		//die in air is both_falldeath1
		//attack1 is on ground, attack2 is in air

	//FIXME: this doesn't belong here
	if ( NPC->client->ps.groundEntityNum != ENTITYNUM_NONE )
	{
		if ( NPCInfo->rank >= RANK_LT )//&& !Q_irand( 0, 50 ) )
		{//officers always stay in the air
			NPC->client->ps.velocity[2] = Q_irand( 50, 125 );
			NPC->NPC->aiFlags |= NPCAI_FLY;	//fixme also, Inform NPC_HandleAIFlags we want to fly
		}
	}

	if ( RT_Flying( NPC ) )
	{//FIXME: only officers need do this, right?
		RT_Flying_Think();
	}
	else if ( NPC->enemy != NULL )
	{//rocketrooper on ground with enemy
		UpdateGoal();
		RT_RunStormtrooperAI();
		RT_CheckJump();
		//NPC_BSST_Default();//FIXME: add missile avoidance
		//RT_Hunt();//NPC_BehaviorSet_Jedi( bState );
	}
	else
	{//shouldn't have gotten in here
		RT_RunStormtrooperAI();
		//NPC_BSST_Patrol();
	}
}