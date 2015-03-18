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
#include "../cgame/cg_camera.h"
#include "../cgame/cg_local.h"
#include "g_functions.h"

extern void G_Knockdown( gentity_t *self, gentity_t *attacker, const vec3_t pushDir, float strength, qboolean breakSaberLock );

#define MIN_ATTACK_DIST_SQ	128
#define MIN_MISS_DIST		100
#define MIN_MISS_DIST_SQ	(MIN_MISS_DIST*MIN_MISS_DIST)
#define MAX_MISS_DIST		500
#define MAX_MISS_DIST_SQ	(MAX_MISS_DIST*MAX_MISS_DIST)
#define MIN_SCORE			-37500 //speed of (50*50) - dist of (200*200)

void SandCreature_Precache( void )
{
	int i;
	G_EffectIndex( "env/sand_dive" );
	G_EffectIndex( "env/sand_spray" );
	G_EffectIndex( "env/sand_move" );
	G_EffectIndex( "env/sand_move_breach" );
	//G_EffectIndex( "env/sand_attack_breach" );
	for ( i = 1; i < 4; i++ )
	{
		G_SoundIndex( va( "sound/chars/sand_creature/voice%d.mp3", i ) );
	}
	G_SoundIndex( "sound/chars/sand_creature/slither.wav" );
}

void SandCreature_ClearTimers( gentity_t *ent )
{
	TIMER_Set( NPC, "speaking", -level.time );
	TIMER_Set( NPC, "breaching", -level.time );
	TIMER_Set( NPC, "breachDebounce", -level.time );
	TIMER_Set( NPC, "pain", -level.time );
	TIMER_Set( NPC, "attacking", -level.time );
	TIMER_Set( NPC, "missDebounce", -level.time );
}

void NPC_SandCreature_Die( gentity_t *self, gentity_t *inflictor, gentity_t *other, const vec3_t point, int damage, int mod,int hitLoc )
{
	//FIXME: somehow make him solid when he dies?
}

void NPC_SandCreature_Pain( gentity_t *self, gentity_t *inflictor, gentity_t *other, const vec3_t point, int damage, int mod,int hitLoc )
{
	if ( TIMER_Done( self, "pain" ) )
	{
		//FIXME: effect and sound
		//FIXME: shootable during this anim?
		NPC_SetAnim( self, SETANIM_LEGS, Q_irand(BOTH_ATTACK1,BOTH_ATTACK2), SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_RESTART );
		G_AddEvent( self, EV_PAIN, Q_irand( 0, 100 ) );
		TIMER_Set( self, "pain", self->client->ps.legsAnimTimer + Q_irand( 500, 2000 ) );	
		float playerDist = Distance( player->currentOrigin, self->currentOrigin );
		if ( playerDist < 256 )
		{
			CGCam_Shake( 1.0f*playerDist/128.0f, self->client->ps.legsAnimTimer );
		}
	}
	self->enemy = self->NPC->goalEntity = NULL;
}

void SandCreature_MoveEffect( void )
{
	vec3_t	up = {0,0,1};
	vec3_t	org = {NPC->currentOrigin[0], NPC->currentOrigin[1], NPC->absmin[2]+2};

	float playerDist = Distance( player->currentOrigin, NPC->currentOrigin );
	if ( playerDist < 256 )
	{
		CGCam_Shake( 0.75f*playerDist/256.0f, 250 );
	}
	
	if ( level.time-NPC->client->ps.lastStationary > 2000 )
	{//first time moving for at least 2 seconds
		//clear speakingtime
		TIMER_Set( NPC, "speaking", -level.time );
	}

	if ( TIMER_Done( NPC, "breaching" ) 
		&& TIMER_Done( NPC, "breachDebounce" )
		&& TIMER_Done( NPC, "pain" )
		&& TIMER_Done( NPC, "attacking" )
		&& !Q_irand( 0, 10 ) )
	{//Breach!
		//FIXME: only do this while moving forward?
		trace_t	trace;
		//make him solid here so he can be hit/gets blocked on stuff. Check clear first.
		gi.trace( &trace, NPC->currentOrigin, NPC->mins, NPC->maxs, NPC->currentOrigin, NPC->s.number, MASK_NPCSOLID, (EG2_Collision)0, 0 );
		if ( !trace.allsolid && !trace.startsolid )
		{
			NPC->clipmask = MASK_NPCSOLID;//turn solid for a little bit
			NPC->contents = CONTENTS_BODY;
			//NPC->takedamage = qtrue;//can be shot?

			//FIXME: Breach sound?
			//FIXME: Breach effect?
			NPC_SetAnim( NPC, SETANIM_LEGS, BOTH_WALK2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_RESTART );
			TIMER_Set( NPC, "breaching", NPC->client->ps.legsAnimTimer );
			TIMER_Set( NPC, "breachDebounce", NPC->client->ps.legsAnimTimer+Q_irand( 0, 10000 ) );
		}
	}
	if ( !TIMER_Done( NPC, "breaching" ) )
	{//different effect when breaching
		//FIXME: make effect
		G_PlayEffect( G_EffectIndex( "env/sand_move_breach" ), org, up );
	}
	else
	{
		G_PlayEffect( G_EffectIndex( "env/sand_move" ), org, up );
	}
	NPC->s.loopSound = G_SoundIndex( "sound/chars/sand_creature/slither.wav" );
}

qboolean SandCreature_CheckAhead( vec3_t end )
{
	trace_t	trace;
	int clipmask = NPC->clipmask|CONTENTS_BOTCLIP;

	//make sure our goal isn't underground (else the trace will fail)
	vec3_t	bottom = {end[0],end[1],end[2]+NPC->mins[2]};
	gi.trace( &trace, end, vec3_origin, vec3_origin, bottom, NPC->s.number, NPC->clipmask, (EG2_Collision)0, 0 );
	if ( trace.fraction < 1.0f )
	{//in the ground, raise it up
		end[2] -= NPC->mins[2]*(1.0f-trace.fraction)-0.125f;
	}

	gi.trace( &trace, NPC->currentOrigin, NPC->mins, NPC->maxs, end, NPC->s.number, clipmask, (EG2_Collision)0, 0 );

	if ( trace.startsolid&&(trace.contents&CONTENTS_BOTCLIP) )
	{//started inside do not enter, so ignore them
		clipmask &= ~CONTENTS_BOTCLIP;
		gi.trace( &trace, NPC->currentOrigin, NPC->mins, NPC->maxs, end, NPC->s.number, clipmask, (EG2_Collision)0, 0 );
	}
	//Do a simple check
	if ( ( trace.allsolid == qfalse ) && ( trace.startsolid == qfalse ) && ( trace.fraction == 1.0f ) )
		return qtrue;

	if ( trace.plane.normal[2] >= MIN_WALK_NORMAL )
	{
		return qtrue;
	}

	//This is a work around
	float	radius = ( NPC->maxs[0] > NPC->maxs[1] ) ? NPC->maxs[0] : NPC->maxs[1];
	float	dist = Distance( NPC->currentOrigin, end );
	float	tFrac = 1.0f - ( radius / dist );

	if ( trace.fraction >= tFrac )
		return qtrue;

	return qfalse;
}

qboolean SandCreature_Move( void )
{
	qboolean moved = qfalse;
	//FIXME should ignore doors..?
	vec3_t dest;
	VectorCopy( NPCInfo->goalEntity->currentOrigin, dest );
	//Sand Creatures look silly using waypoints when they can go straight to the goal
	if ( SandCreature_CheckAhead( dest ) )
	{//use our temp move straight to goal check
		VectorSubtract( dest, NPC->currentOrigin, NPC->client->ps.moveDir );
		NPC->client->ps.speed = VectorNormalize( NPC->client->ps.moveDir );
		if ( (ucmd.buttons&BUTTON_WALKING) && NPC->client->ps.speed > NPCInfo->stats.walkSpeed )
		{
			NPC->client->ps.speed = NPCInfo->stats.walkSpeed;
		}
		else
		{
			if ( NPC->client->ps.speed < NPCInfo->stats.walkSpeed )
			{
				NPC->client->ps.speed = NPCInfo->stats.walkSpeed;
			}
			if ( !(ucmd.buttons&BUTTON_WALKING) && NPC->client->ps.speed < NPCInfo->stats.runSpeed )
			{
				NPC->client->ps.speed = NPCInfo->stats.runSpeed;
			}
			else if ( NPC->client->ps.speed > NPCInfo->stats.runSpeed )
			{
				NPC->client->ps.speed = NPCInfo->stats.runSpeed;
			}
		}
		moved = qtrue;
	}
	else
	{
		moved = NPC_MoveToGoal( qtrue );
	}
	if ( moved && NPC->radius )
	{
		vec3_t	newPos;
		float curTurfRange, newTurfRange;
		curTurfRange = DistanceHorizontal( NPC->currentOrigin, NPC->s.origin );
		VectorMA( NPC->currentOrigin, NPC->client->ps.speed/100.0f, NPC->client->ps.moveDir, newPos );
		newTurfRange = DistanceHorizontal( newPos, NPC->s.origin );
		if ( newTurfRange > NPC->radius && newTurfRange > curTurfRange )
		{//would leave our range
			//stop
			NPC->client->ps.speed = 0.0f;
			VectorClear( NPC->client->ps.moveDir );
			ucmd.forwardmove = ucmd.rightmove = 0;
			moved = qfalse;
		}
	}
	return (moved);
	//often erroneously returns false ???  something wrong with NAV...?
}

void SandCreature_Attack( qboolean miss )
{
	//FIXME: make it able to grab a thermal detonator, take it down, 
	//		then have it explode inside them, killing them 
	//		(or, do damage, making them stick half out of the ground and
	//		screech for a bit, giving you a chance to run for it!)

	//FIXME: effect and sound
	//FIXME: shootable during this anim?
	if ( !NPC->enemy->client )
	{
		NPC_SetAnim( NPC, SETANIM_LEGS, BOTH_ATTACK1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_RESTART );
	}
	else
	{
		NPC_SetAnim( NPC, SETANIM_LEGS, Q_irand( BOTH_ATTACK1, BOTH_ATTACK2 ), SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_RESTART );
	}
	//don't do anything else while in this anim
	TIMER_Set( NPC, "attacking", NPC->client->ps.legsAnimTimer );
	float playerDist = Distance( player->currentOrigin, NPC->currentOrigin );
	if ( playerDist < 256 )
	{
		//FIXME: tone this down
		CGCam_Shake( 0.75f*playerDist/128.0f, NPC->client->ps.legsAnimTimer );
	} 

	if ( miss )
	{//purposely missed him, chance of knocking him down
		//FIXME: if, during the attack anim, I do end up catching him close to my mouth, then snatch him anyway...
		if ( NPC->enemy && NPC->enemy->client )
		{
			vec3_t dir2Enemy;
			VectorSubtract( NPC->enemy->currentOrigin, NPC->currentOrigin, dir2Enemy );
			if ( dir2Enemy[2] < 30 )
			{
				dir2Enemy[2] = 30;
			}
			if ( g_spskill->integer > 0 )
			{
				float enemyDist = VectorNormalize( dir2Enemy );
				//FIXME: tone this down, smaller radius
				if ( enemyDist < 200 && NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE )
				{
					float throwStr = ((200-enemyDist)*0.4f)+20;
					if ( throwStr > 45 )
					{
						throwStr = 45;
					}
					G_Throw( NPC->enemy, dir2Enemy, throwStr );
					if ( g_spskill->integer > 1 )
					{//knock them down, too
						if ( NPC->enemy->health > 0 
							&& Q_flrand( 50, 150 ) > enemyDist )
						{//knock them down
							G_Knockdown( NPC->enemy, NPC, dir2Enemy, 300, qtrue );
							if ( NPC->enemy->s.number < MAX_CLIENTS )
							{//make the player look up at me
								vec3_t vAng;
								vectoangles( dir2Enemy, vAng );
								VectorSet( vAng, AngleNormalize180(vAng[PITCH])*-1, NPC->enemy->client->ps.viewangles[YAW], 0 );
								SetClientViewAngle( NPC->enemy, vAng );
							}
						}
					}
				}
			}
		}
	}
	else
	{
		NPC->enemy->activator = NPC; // kind of dumb, but when we are locked to the Rancor, we are owned by it.
		NPC->activator = NPC->enemy;//remember him
		//this guy isn't going anywhere anymore
		NPC->enemy->contents = 0;
		NPC->enemy->clipmask = 0;

		if ( NPC->activator->client )
		{
			NPC->activator->client->ps.SaberDeactivate();
			NPC->activator->client->ps.eFlags |= EF_HELD_BY_SAND_CREATURE;
			if ( NPC->activator->health > 0 && NPC->activator->client )
			{
				G_AddEvent( NPC->activator, Q_irand(EV_DEATH1, EV_DEATH3), 0 );
				NPC_SetAnim( NPC->activator, SETANIM_LEGS, BOTH_SWIM_IDLE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
				NPC_SetAnim( NPC->activator, SETANIM_TORSO, BOTH_FALLDEATH1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
				TossClientItems( NPC );
				if ( NPC->activator->NPC )
				{//no more thinking for you
					NPC->activator->NPC->nextBStateThink = Q3_INFINITE;
				}
			}
			/*
			if ( !NPC->activator->s.number )
			{
				cg.overrides.active |= (CG_OVERRIDE_3RD_PERSON_CDP|CG_OVERRIDE_3RD_PERSON_RNG);
				cg.overrides.thirdPersonCameraDamp = 0;
				cg.overrides.thirdPersonRange = 120;
			}
			*/
		}
		else
		{
			NPC->activator->s.eFlags |= EF_HELD_BY_SAND_CREATURE;
		}
	}
}

float SandCreature_EntScore( gentity_t *ent )
{
	float moveSpeed, dist;

	if ( ent->client )
	{
		moveSpeed = VectorLengthSquared( ent->client->ps.velocity );
	}
	else
	{
		moveSpeed = VectorLengthSquared( ent->s.pos.trDelta );
	}
	dist = DistanceSquared( NPC->currentOrigin, ent->currentOrigin );
	return (moveSpeed-dist);
}

void SandCreature_SeekEnt( gentity_t *bestEnt, float score )
{
	NPCInfo->enemyLastSeenTime = level.time;
	VectorCopy( bestEnt->currentOrigin, NPCInfo->enemyLastSeenLocation );
	NPC_SetMoveGoal( NPC, NPCInfo->enemyLastSeenLocation, 0, qfalse );
	if ( score > MIN_SCORE )
	{
		NPC->enemy = bestEnt;
	}
}

void SandCreature_CheckMovingEnts( void )
{
	gentity_t	*radiusEnts[ 128 ];
	int			numEnts;
	const float	radius = NPCInfo->stats.earshot;
	int			i;
	vec3_t		mins, maxs;

	for ( i = 0; i < 3; i++ )
	{
		mins[i] = NPC->currentOrigin[i] - radius;
		maxs[i] = NPC->currentOrigin[i] + radius;
	}

	numEnts = gi.EntitiesInBox( mins, maxs, radiusEnts, 128 );
	int bestEnt = -1;
	float bestScore = 0;
	float checkScore;

	for ( i = 0; i < numEnts; i++ )
	{
		if ( !radiusEnts[i]->inuse )
		{
			continue;
		}
		
		if ( radiusEnts[i] == NPC )
		{//Skip the rancor ent
			continue;
		}
		
		if ( radiusEnts[i]->client == NULL )
		{//must be a client
			if ( radiusEnts[i]->s.eType != ET_MISSILE
				|| radiusEnts[i]->s.weapon != WP_THERMAL )
			{//not a thermal detonator
				continue;
			}
		}
		else
		{
			if ( (radiusEnts[i]->client->ps.eFlags&EF_HELD_BY_RANCOR) )
			{//can't be one being held
				continue;
			}

			if ( (radiusEnts[i]->client->ps.eFlags&EF_HELD_BY_WAMPA) )
			{//can't be one being held
				continue;
			}

			if ( (radiusEnts[i]->client->ps.eFlags&EF_HELD_BY_SAND_CREATURE) )
			{//can't be one being held
				continue;
			}

			if ( (radiusEnts[i]->s.eFlags&EF_NODRAW) )
			{//not if invisible
				continue;
			}
	
			if ( radiusEnts[i]->client->ps.groundEntityNum != ENTITYNUM_WORLD )
			{//not on the ground
				continue;
			}

			if ( radiusEnts[i]->client->NPC_class == CLASS_SAND_CREATURE )
			{
				continue;
			}
		}

		if ( (radiusEnts[i]->flags&FL_NOTARGET) )
		{
			continue;
		}
		/*
		if ( radiusEnts[i]->client && (radiusEnts[i]->client->NPC_class == CLASS_RANCOR || radiusEnts[i]->client->NPC_class == CLASS_ATST ) )
		{//can't grab rancors or atst's
			continue;
		}
		*/
		checkScore = SandCreature_EntScore( radiusEnts[i] );
		//FIXME: take mass into account too?  What else?
		if ( checkScore > bestScore )
		{
			bestScore = checkScore;
			bestEnt = i;
		}
	}
	if ( bestEnt != -1 )
	{
		SandCreature_SeekEnt( radiusEnts[bestEnt], bestScore );
	}
}

void SandCreature_SeekAlert( int alertEvent )
{
	alertEvent_t *alert = &level.alertEvents[alertEvent];

	//FIXME: check for higher alert status or closer than last location?
	NPCInfo->enemyLastSeenTime = level.time;
	VectorCopy( alert->position, NPCInfo->enemyLastSeenLocation );
	NPC_SetMoveGoal( NPC, NPCInfo->enemyLastSeenLocation, 0, qfalse );
}

void SandCreature_CheckAlerts( void )
{
	if ( !(NPCInfo->scriptFlags&SCF_IGNORE_ALERTS) )
	{
		int alertEvent = NPC_CheckAlertEvents( qfalse, qtrue, NPCInfo->lastAlertID, qfalse, AEL_MINOR, qtrue );

		//There is an event to look at
		if ( alertEvent >= 0 )
		{
			//if ( level.alertEvents[alertEvent].ID != NPCInfo->lastAlertID )
			{
				SandCreature_SeekAlert( alertEvent );
			}
		}
	}
}

float SandCreature_DistSqToGoal( qboolean goalIsEnemy )
{
	float goalDistSq;
	if ( !NPCInfo->goalEntity || goalIsEnemy )
	{
		if ( !NPC->enemy )
		{
			return Q3_INFINITE;
		}
		NPCInfo->goalEntity = NPC->enemy;
	}

	if ( NPCInfo->goalEntity->client )
	{
		goalDistSq = DistanceSquared( NPC->currentOrigin, NPCInfo->goalEntity->currentOrigin );
	}
	else
	{
		vec3_t gOrg;
		VectorCopy( NPCInfo->goalEntity->currentOrigin, gOrg );
		gOrg[2] -= (NPC->mins[2]-NPCInfo->goalEntity->mins[2]);//moves the gOrg up/down to make it's origin seem at the proper height as if it had my mins
		goalDistSq = DistanceSquared( NPC->currentOrigin, gOrg );
	}
	return goalDistSq;
}

void SandCreature_Chase( void )
{
	if ( !NPC->enemy->inuse )
	{//freed
		NPC->enemy = NULL;
		return;
	}
	
	if ( (NPC->svFlags&SVF_LOCKEDENEMY) )
	{//always know where he is
		NPCInfo->enemyLastSeenTime = level.time;
	}

	if ( !(NPC->svFlags&SVF_LOCKEDENEMY) )
	{
		if ( level.time-NPCInfo->enemyLastSeenTime > 10000 )
		{
			NPC->enemy = NULL;
			return;
		}
	}

	if ( NPC->enemy->client )
	{
		if ( (NPC->enemy->client->ps.eFlags&EF_HELD_BY_SAND_CREATURE)
			|| (NPC->enemy->client->ps.eFlags&EF_HELD_BY_RANCOR)
			|| (NPC->enemy->client->ps.eFlags&EF_HELD_BY_WAMPA) )
		{//was picked up by another monster, forget about him
			NPC->enemy = NULL;
			NPC->svFlags &= ~SVF_LOCKEDENEMY;
			return;
		}
	}
	//chase the enemy
	if ( NPC->enemy->client 
		&& NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_WORLD 
		&& !(NPC->svFlags&SVF_LOCKEDENEMY) )
	{//off the ground!
		//FIXME: keep moving in the dir we were moving for a little bit...
	}
	else
	{
		float enemyScore = SandCreature_EntScore( NPC->enemy );
		if ( enemyScore < MIN_SCORE 
			&& !(NPC->svFlags&SVF_LOCKEDENEMY) )
		{//too slow or too far away
		}
		else
		{
			float moveSpeed;
			if ( NPC->enemy->client )
			{
				moveSpeed = VectorLengthSquared( NPC->enemy->client->ps.velocity );
			}
			else
			{
				moveSpeed = VectorLengthSquared( NPC->enemy->s.pos.trDelta );
			}
			if ( moveSpeed )
			{//he's still moving, update my goalEntity's origin
				SandCreature_SeekEnt( NPC->enemy, 0 );
				NPCInfo->enemyLastSeenTime = level.time;
			}
		}
	}

	if ( (level.time-NPCInfo->enemyLastSeenTime) > 5000 
		&& !(NPC->svFlags&SVF_LOCKEDENEMY) )
	{//enemy hasn't moved in about 5 seconds, see if there's anything else of interest
		SandCreature_CheckAlerts();
		SandCreature_CheckMovingEnts();
	}

	float enemyDistSq = SandCreature_DistSqToGoal( qtrue );

	//FIXME: keeps chasing goalEntity even when it's already reached it...?
	if ( enemyDistSq >= MIN_ATTACK_DIST_SQ//NPCInfo->goalEntity &&
		&& (level.time-NPCInfo->enemyLastSeenTime) <= 3000 )
	{//sensed enemy (or something) less than 3 seconds ago
		ucmd.buttons &= ~BUTTON_WALKING;
		if ( SandCreature_Move() )
		{
			SandCreature_MoveEffect();
		}
	}
	else if ( (level.time-NPCInfo->enemyLastSeenTime) <= 5000 
		&& !(NPC->svFlags&SVF_LOCKEDENEMY) )
	{//NOTE: this leaves a 2-second dead zone in which they'll just sit there unless their enemy moves
		//If there is an event we might be interested in if we weren't still interested in our enemy
		if ( NPC_CheckAlertEvents( qfalse, qtrue, NPCInfo->lastAlertID, qfalse, AEL_MINOR, qtrue ) >= 0 )
		{//just stir
			SandCreature_MoveEffect();
		}
	}

	if ( enemyDistSq < MIN_ATTACK_DIST_SQ )
	{
		if ( NPC->enemy->client )
		{
			NPC->client->ps.viewangles[YAW] = NPC->enemy->client->ps.viewangles[YAW];
		}
		if ( TIMER_Done( NPC, "breaching" ) )
		{
			//okay to attack
			SandCreature_Attack( qfalse );
		}
	}
	else if ( enemyDistSq < MAX_MISS_DIST_SQ 
		&& enemyDistSq > MIN_MISS_DIST_SQ
		&& NPC->enemy->client 
		&& TIMER_Done( NPC, "breaching" )
		&& TIMER_Done( NPC, "missDebounce" )
		&& !VectorCompare( NPC->pos1, NPC->currentOrigin ) //so we don't come up again in the same spot
		&& !Q_irand( 0, 10 ) )
	{
		if ( !(NPC->svFlags&SVF_LOCKEDENEMY) )
		{
			//miss them
			SandCreature_Attack( qtrue );
			VectorCopy( NPC->currentOrigin, NPC->pos1 );
			TIMER_Set( NPC, "missDebounce", Q_irand( 3000, 10000 ) );
		}
	}
}

void SandCreature_Hunt( void )
{
	SandCreature_CheckAlerts();
	SandCreature_CheckMovingEnts();
	//If we have somewhere to go, then do that
	//FIXME: keeps chasing goalEntity even when it's already reached it...?
	if ( NPCInfo->goalEntity 
		&& SandCreature_DistSqToGoal( qfalse ) >= MIN_ATTACK_DIST_SQ )
	{
		ucmd.buttons |= BUTTON_WALKING;
		if ( SandCreature_Move() )
		{
			SandCreature_MoveEffect();
		}
	}
	else
	{
		NPC_ReachedGoal();
	}
}

void SandCreature_Sleep( void )
{
	SandCreature_CheckAlerts();
	SandCreature_CheckMovingEnts();
	//FIXME: keeps chasing goalEntity even when it's already reached it!
	if ( NPCInfo->goalEntity 
		&& SandCreature_DistSqToGoal( qfalse ) >= MIN_ATTACK_DIST_SQ )
	{
		ucmd.buttons |= BUTTON_WALKING;
		if ( SandCreature_Move() )
		{
			SandCreature_MoveEffect();
		}
	}
	else
	{
		NPC_ReachedGoal();
	}
	/*
	if ( UpdateGoal() )
	{
		ucmd.buttons |= BUTTON_WALKING;
		//FIXME: Sand Creatures look silly using waypoints when they can go straight to the goal
		if ( SandCreature_Move() )
		{
			SandCreature_MoveEffect();
		}
	}
	*/
}

void	SandCreature_PushEnts()
{
  	int			numEnts;
	gentity_t*	radiusEnts[128];
	const float	radius = 70;
	vec3_t		mins, maxs;
	vec3_t		smackDir;
	float		smackDist;

	for (int i = 0; i < 3; i++ )
	{
		mins[i] = NPC->currentOrigin[i] - radius;
		maxs[i] = NPC->currentOrigin[i] + radius;
	}

	numEnts = gi.EntitiesInBox(mins, maxs, radiusEnts, 128);
	for (int entIndex=0; entIndex<numEnts; entIndex++)
	{
		// Only Clients
		//--------------
		if (!radiusEnts[entIndex] || !radiusEnts[entIndex]->client || radiusEnts[entIndex]==NPC)
		{
			continue;
		}

		// Do The Vector Distance Test
		//-----------------------------
		VectorSubtract(radiusEnts[entIndex]->currentOrigin, NPC->currentOrigin, smackDir);
		smackDist = VectorNormalize(smackDir);
		if (smackDist<radius)
		{
 			G_Throw(radiusEnts[entIndex], smackDir, 90);
		}
	}
}

void NPC_BSSandCreature_Default( void )
{
	qboolean visible = qfalse;
	
	//clear it every frame, will be set if we actually move this frame...
	NPC->s.loopSound = 0;

	if ( NPC->health > 0 && TIMER_Done( NPC, "breaching" ) )
	{//go back to non-solid mode
		if ( NPC->contents )
		{
			NPC->contents = 0;
		}
		if ( NPC->clipmask == MASK_NPCSOLID )
		{
			NPC->clipmask = CONTENTS_SOLID|CONTENTS_MONSTERCLIP;
		}
		if ( TIMER_Done( NPC, "speaking" ) )
		{
			G_SoundOnEnt( NPC, CHAN_VOICE, va( "sound/chars/sand_creature/voice%d.mp3", Q_irand( 1, 3 ) ) );
			TIMER_Set( NPC, "speaking", Q_irand( 3000, 10000 ) );
		}
	}
	else
	{//still in breaching anim
		visible = qtrue;
		//FIXME: maybe push things up/away and maybe knock people down when doing this?
		//FIXME: don't turn while breaching?
		//FIXME: move faster while breaching?
		//NOTE: shaking now done whenever he moves
	}

	//FIXME: when in start and end of attack/pain anims, need ground disturbance effect around him
	// NOTENOTE: someone stubbed this code in, so I figured I'd use it.  The timers are all weird, ie, magic numbers that sort of work, 
	//	but maybe I'll try and figure out real values later if I have time.
	if ( NPC->client->ps.legsAnim == BOTH_ATTACK1
		|| NPC->client->ps.legsAnim == BOTH_ATTACK2 ) 
	{//FIXME: get start and end frame numbers for this effect for each of these anims
		vec3_t	up={0,0,1};
		vec3_t org;
		VectorCopy( NPC->currentOrigin, org );
		org[2] -= 40; 
		if ( NPC->client->ps.legsAnimTimer > 3700 )
		{
//			G_PlayEffect( G_EffectIndex( "env/sand_dive"  ), NPC->currentOrigin, up );
			G_PlayEffect( G_EffectIndex( "env/sand_spray"  ), org, up );
		}
		else if ( NPC->client->ps.legsAnimTimer > 1600 	&& NPC->client->ps.legsAnimTimer < 1900 )
		{
			G_PlayEffect( G_EffectIndex( "env/sand_spray"  ), org, up );
		}
		//G_PlayEffect( G_EffectIndex( "env/sand_attack_breach" ), org, up );
	}
	

	if ( !TIMER_Done( NPC, "pain" ) )
	{
		visible = qtrue;
	}
	else if ( !TIMER_Done( NPC, "attacking" ) )
	{
		visible = qtrue;
	}
	else
	{
		if ( NPC->activator )
		{//kill and remove the guy we ate
			//FIXME: want to play ...?  What was I going to say?
			NPC->activator->health = 0;
			GEntity_DieFunc( NPC->activator, NPC, NPC, 1000, MOD_MELEE, 0, HL_NONE );
			if ( NPC->activator->s.number )
			{
				G_FreeEntity( NPC->activator );
			}
			else
			{//can't remove the player, just make him invisible
				NPC->client->ps.eFlags |= EF_NODRAW;
			}
			NPC->activator = NPC->enemy = NPCInfo->goalEntity = NULL;
		}

		if ( NPC->enemy )
		{
			SandCreature_Chase();
		}
		else if ( (level.time - NPCInfo->enemyLastSeenTime) < 5000 )//FIXME: should make this able to be variable
		{//we were alerted recently, move towards there and look for footsteps, etc.
			SandCreature_Hunt();
		}
		else
		{//no alerts, sleep and wake up only by alerts
			//FIXME: keeps chasing goalEntity even when it's already reached it!
			SandCreature_Sleep();
		}
	}
	NPC_UpdateAngles( qtrue, qtrue );
	if ( !visible )
	{
		NPC->client->ps.eFlags |= EF_NODRAW;
		NPC->s.eFlags |= EF_NODRAW;
	}
	else
	{
		NPC->client->ps.eFlags &= ~EF_NODRAW;
		NPC->s.eFlags &= ~EF_NODRAW;

		SandCreature_PushEnts();
	}
}

//FIXME: need pain behavior of sticking up through ground, writhing and screaming
//FIXME: need death anim like pain, but flopping aside and staying above ground...