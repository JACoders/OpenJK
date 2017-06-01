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
#include "g_navigator.h"

// These define the working combat range for these suckers
#define MIN_DISTANCE		48
#define MIN_DISTANCE_SQR	( MIN_DISTANCE * MIN_DISTANCE )

#define MAX_DISTANCE		1024
#define MAX_DISTANCE_SQR	( MAX_DISTANCE * MAX_DISTANCE )

#define LSTATE_CLEAR		0
#define LSTATE_WAITING		1

float enemyDist = 0;
extern qboolean NAV_CheckAhead( gentity_t *self, vec3_t end, trace_t &trace, int clipmask );
extern int PM_AnimLength( int index, animNumber_t anim );
extern cvar_t	*g_dismemberment;
/*
-------------------------
NPC_Wampa_Precache
-------------------------
*/
void NPC_Wampa_Precache( void )
{
	/*
	int i;
	for ( i = 1; i < 4; i ++ )
	{
		G_SoundIndex( va("sound/chars/wampa/growl%d.wav", i) );
	}
	for ( i = 1; i < 3; i ++ )
	{
		G_SoundIndex( va("sound/chars/wampa/snort%d.wav", i) );
	}
	*/
	G_SoundIndex( "sound/chars/rancor/swipehit.wav" );
	//G_SoundIndex( "sound/chars/wampa/chomp.wav" );
}


/*
-------------------------
Wampa_Idle
-------------------------
*/
void Wampa_Idle( void )
{
	NPCInfo->localState = LSTATE_CLEAR;

	//If we have somewhere to go, then do that
	if ( UpdateGoal() )
	{
		ucmd.buttons &= ~BUTTON_WALKING;
		NPC_MoveToGoal( qtrue );
	}
}

qboolean Wampa_CheckRoar( gentity_t *self )
{
	if ( self->wait < level.time )
	{
		self->wait = level.time + Q_irand( 5000, 20000 );
		NPC_SetAnim( self, SETANIM_BOTH, Q_irand(BOTH_GESTURE1,BOTH_GESTURE2), (SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD) );
		TIMER_Set( self, "rageTime", self->client->ps.legsAnimTimer );
		return qtrue;
	}
	return qfalse;
}
/*
-------------------------
Wampa_Patrol
-------------------------
*/
void Wampa_Patrol( void )
{
	NPCInfo->localState = LSTATE_CLEAR;

	//If we have somewhere to go, then do that
	if ( UpdateGoal() )
	{
		ucmd.buttons |= BUTTON_WALKING;
		NPC_MoveToGoal( qtrue );
	}

	if ( NPC_CheckEnemyExt( qtrue ) == qfalse )
	{
		Wampa_Idle();
		return;
	}
	Wampa_CheckRoar( NPC );
	TIMER_Set( NPC, "lookForNewEnemy", Q_irand( 5000, 15000 ) );
}

/*
-------------------------
Wampa_Move
-------------------------
*/
void Wampa_Move( qboolean visible )
{
	if ( NPCInfo->localState != LSTATE_WAITING )
	{
		NPCInfo->goalEntity = NPC->enemy;

		trace_t	trace;
		if ( !NAV_CheckAhead( NPC, NPCInfo->goalEntity->currentOrigin, trace, (NPC->clipmask|CONTENTS_BOTCLIP) ) )
		{
			if ( !NPC_MoveToGoal( qfalse ) )
			{
				STEER::Activate(NPC);
				STEER::Seek(NPC, NPCInfo->goalEntity->currentOrigin);
				STEER::AvoidCollisions(NPC);
				STEER::DeActivate(NPC, &ucmd);
			}
		}
		NPCInfo->goalRadius = MIN_DISTANCE;//MAX_DISTANCE;	// just get us within combat range

		if ( NPC->enemy )
		{//pick correct movement speed and anim
			//run by default
			ucmd.buttons &= ~BUTTON_WALKING;
			if ( !TIMER_Done( NPC, "runfar" )
				|| !TIMER_Done( NPC, "runclose" ) )
			{//keep running with this anim & speed for a bit
			}
			else if ( !TIMER_Done( NPC, "walk" ) )
			{//keep walking for a bit
				ucmd.buttons |= BUTTON_WALKING;
			}
			else if ( visible && enemyDist > 350 && NPCInfo->stats.runSpeed == 200 )//180 )
			{//fast run, all fours
				//BOTH_RUN1
				NPCInfo->stats.runSpeed = 300;
				TIMER_Set( NPC, "runfar", Q_irand( 4000, 8000 ) );
				if ( NPC->client->ps.legsAnim == BOTH_RUN2 )
				{
					NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_RUN2TORUN1, SETANIM_FLAG_HOLD );
				}
			}
			else if ( enemyDist > 200 && NPCInfo->stats.runSpeed == 300 )
			{//slow run, upright
				//BOTH_RUN2
				NPCInfo->stats.runSpeed = 200;//180;
				TIMER_Set( NPC, "runclose", Q_irand( 5000, 10000 ) );
				if ( NPC->client->ps.legsAnim == BOTH_RUN1 )
				{
					NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_RUN1TORUN2, SETANIM_FLAG_HOLD );
				}
			}
			else if ( enemyDist < 100 )
			{//walk
				NPCInfo->stats.runSpeed = 200;//180;
				ucmd.buttons |= BUTTON_WALKING;
				TIMER_Set( NPC, "walk", Q_irand( 6000, 12000 ) );
			}
		}
	}
}

//---------------------------------------------------------
extern void G_Knockdown( gentity_t *self, gentity_t *attacker, const vec3_t pushDir, float strength, qboolean breakSaberLock );
extern qboolean G_DoDismemberment( gentity_t *self, vec3_t point, int mod, int damage, int hitLoc, qboolean force = qfalse );
extern int NPC_GetEntsNearBolt( gentity_t **radiusEnts, float radius, int boltIndex, vec3_t boltOrg );

void Wampa_Slash( int boltIndex, qboolean backhand )
{
	gentity_t	*radiusEnts[ 128 ];
	int			numEnts;
	const float	radius = 88;
	const float	radiusSquared = (radius*radius);
	int			i;
	vec3_t		boltOrg;
	int			damage = (backhand)?Q_irand(10,15):Q_irand(20,30);

	numEnts = NPC_GetEntsNearBolt( radiusEnts, radius, boltIndex, boltOrg );

	for ( i = 0; i < numEnts; i++ )
	{
		if ( !radiusEnts[i]->inuse )
		{
			continue;
		}

		if ( radiusEnts[i] == NPC )
		{//Skip the wampa ent
			continue;
		}

		if ( radiusEnts[i]->client == NULL )
		{//must be a client
			continue;
		}

		if ( DistanceSquared( radiusEnts[i]->currentOrigin, boltOrg ) <= radiusSquared )
		{
			//smack
			G_Damage( radiusEnts[i], NPC, NPC, vec3_origin, radiusEnts[i]->currentOrigin, damage, ((backhand)?0:DAMAGE_NO_KNOCKBACK), MOD_MELEE );
			if ( backhand )
			{
				//actually push the enemy
				vec3_t pushDir;
				vec3_t angs;
				VectorCopy( NPC->client->ps.viewangles, angs );
				angs[YAW] += Q_flrand( 25, 50 );
				angs[PITCH] = Q_flrand( -25, -15 );
				AngleVectors( angs, pushDir, NULL, NULL );
				if ( radiusEnts[i]->client->NPC_class != CLASS_WAMPA
					&& radiusEnts[i]->client->NPC_class != CLASS_RANCOR
					&& radiusEnts[i]->client->NPC_class != CLASS_ATST
					&& !(radiusEnts[i]->flags&FL_NO_KNOCKBACK) )
				{
					G_Throw( radiusEnts[i], pushDir, 65 );
					if ( radiusEnts[i]->health > 0 && Q_irand( 0, 1 ) )
					{//do pain on enemy
						G_Knockdown( radiusEnts[i], NPC, pushDir, 300, qtrue );
					}
				}
			}
			else if ( radiusEnts[i]->health <= 0 && radiusEnts[i]->client )
			{//killed them, chance of dismembering
				if ( !Q_irand( 0, 1 ) )
				{//bite something off
					int hitLoc = HL_WAIST;
					if ( g_dismemberment->integer < 4 )
					{
						hitLoc = Q_irand( HL_BACK_RT, HL_HAND_LT );
					}
					else
					{
						hitLoc = Q_irand( HL_WAIST, HL_HEAD );
					}
					if ( hitLoc == HL_HEAD )
					{
						NPC_SetAnim( radiusEnts[i], SETANIM_BOTH, BOTH_DEATH17, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
					}
					else if ( hitLoc == HL_WAIST )
					{
						NPC_SetAnim( radiusEnts[i], SETANIM_BOTH, BOTH_DEATHBACKWARD2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
					}
					radiusEnts[i]->client->dismembered = false;
					//FIXME: the limb should just disappear, cuz I ate it
					G_DoDismemberment( radiusEnts[i], radiusEnts[i]->currentOrigin, MOD_SABER, 1000, hitLoc, qtrue );
				}
			}
			else if ( !Q_irand( 0, 3 ) && radiusEnts[i]->health > 0 )
			{//one out of every 4 normal hits does a knockdown, too
				vec3_t pushDir;
				vec3_t angs;
				VectorCopy( NPC->client->ps.viewangles, angs );
				angs[YAW] += Q_flrand( 25, 50 );
				angs[PITCH] = Q_flrand( -25, -15 );
				AngleVectors( angs, pushDir, NULL, NULL );
				G_Knockdown( radiusEnts[i], NPC, pushDir, 35, qtrue );
			}
			G_Sound( radiusEnts[i], G_SoundIndex( "sound/chars/rancor/swipehit.wav" ) );
		}
	}
}

//------------------------------
void Wampa_Attack( float distance, qboolean doCharge )
{
	if ( !TIMER_Exists( NPC, "attacking" ) )
	{
		if ( !Q_irand(0, 3) && !doCharge )
		{//double slash
			NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_ATTACK1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
			TIMER_Set( NPC, "attack_dmg", 750 );
		}
		else if ( doCharge || (distance > 270 && distance < 430 && !Q_irand(0, 1)) )
		{//leap
			NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_ATTACK2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
			TIMER_Set( NPC, "attack_dmg", 500 );
			vec3_t	fwd, yawAng ={0, NPC->client->ps.viewangles[YAW], 0};
			AngleVectors( yawAng, fwd, NULL, NULL );
			VectorScale( fwd, distance*1.5f, NPC->client->ps.velocity );
			NPC->client->ps.velocity[2] = 150;
			NPC->client->ps.groundEntityNum = ENTITYNUM_NONE;
		}
		else if ( distance < 100 )//&& !Q_irand( 0, 4 ) )
		{//grab
			NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_HOLD_START, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
			NPC->client->ps.legsAnimTimer += 200;
			TIMER_Set( NPC, "attack_dmg", 250 );
		}
		else
		{//backhand
			NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_ATTACK3, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
			TIMER_Set( NPC, "attack_dmg", 250 );
		}

		TIMER_Set( NPC, "attacking", NPC->client->ps.legsAnimTimer + Q_flrand(0.0f, 1.0f) * 200 );
		//allow us to re-evaluate our running speed/anim
		TIMER_Set( NPC, "runfar", -1 );
		TIMER_Set( NPC, "runclose", -1 );
		TIMER_Set( NPC, "walk", -1 );
	}

	// Need to do delayed damage since the attack animations encapsulate multiple mini-attacks

	if ( TIMER_Done2( NPC, "attack_dmg", qtrue ) )
	{
		switch ( NPC->client->ps.legsAnim )
		{
		case BOTH_ATTACK1:
			Wampa_Slash( NPC->handRBolt, qfalse );
			//do second hit
			TIMER_Set( NPC, "attack_dmg2", 100 );
			break;
		case BOTH_ATTACK2:
			Wampa_Slash( NPC->handRBolt, qfalse );
			TIMER_Set( NPC, "attack_dmg2", 100 );
			break;
		case BOTH_ATTACK3:
			Wampa_Slash( NPC->handLBolt, qtrue );
			break;
		}
	}
	else if ( TIMER_Done2( NPC, "attack_dmg2", qtrue ) )
	{
		switch ( NPC->client->ps.legsAnim )
		{
		case BOTH_ATTACK1:
			Wampa_Slash( NPC->handLBolt, qfalse );
			break;
		case BOTH_ATTACK2:
			Wampa_Slash( NPC->handLBolt, qfalse );
			break;
		}
	}

	// Just using this to remove the attacking flag at the right time
	TIMER_Done2( NPC, "attacking", qtrue );

	if ( NPC->client->ps.legsAnim == BOTH_ATTACK1 && distance > (NPC->maxs[0]+MIN_DISTANCE) )
	{//okay to keep moving
		ucmd.buttons |= BUTTON_WALKING;
		Wampa_Move( qtrue );
	}
}

//----------------------------------
void Wampa_Combat( void )
{
	// If we cannot see our target or we have somewhere to go, then do that
	if ( !NPC_ClearLOS( NPC->enemy ) )
	{
		if ( !Q_irand( 0, 10 ) )
		{
			if ( Wampa_CheckRoar( NPC ) )
			{
				return;
			}
		}
		NPCInfo->combatMove = qtrue;
		NPCInfo->goalEntity = NPC->enemy;
		NPCInfo->goalRadius = MIN_DISTANCE;//MAX_DISTANCE;	// just get us within combat range

		Wampa_Move( qfalse );
		return;
	}
	/*
	else if ( UpdateGoal() )
	{
		NPCInfo->combatMove = qtrue;
		NPCInfo->goalEntity = NPC->enemy;
		NPCInfo->goalRadius = MIN_DISTANCE;//MAX_DISTANCE;	// just get us within combat range

		Wampa_Move( 1 );
		return;
	}*/

	// Sometimes I have problems with facing the enemy I'm attacking, so force the issue so I don't look dumb
	//FIXME: always seems to face off to the left or right?!!!!
	NPC_FaceEnemy( qtrue );

	float distance	= enemyDist = Distance( NPC->currentOrigin, NPC->enemy->currentOrigin );

	qboolean	advance = (qboolean)( distance > (NPC->maxs[0]+MIN_DISTANCE) ? qtrue : qfalse  );
	qboolean	doCharge = qfalse;

	if ( advance )
	{//have to get closer
		vec3_t	yawOnlyAngles = {0, NPC->currentAngles[YAW], 0};
		if ( NPC->enemy->health > 0//enemy still alive
			&& fabs(distance-350) <= 80 //enemy anywhere from 270 to 430 away
			&& InFOV( NPC->enemy->currentOrigin, NPC->currentOrigin, yawOnlyAngles, 20, 20 ) )//enemy generally in front
		{//10% chance of doing charge anim
			if ( !Q_irand( 0, 6 ) )
			{//go for the charge
				doCharge = qtrue;
				advance = qfalse;
			}
		}
	}

	if (( advance || NPCInfo->localState == LSTATE_WAITING ) && TIMER_Done( NPC, "attacking" )) // waiting monsters can't attack
	{
		if ( TIMER_Done2( NPC, "takingPain", qtrue ))
		{
			NPCInfo->localState = LSTATE_CLEAR;
		}
		else
		{
			Wampa_Move( qtrue );
		}
	}
	else
	{
		if ( !Q_irand( 0, 15 ) )
		{//FIXME: only do this if we just damaged them or vice-versa?
			if ( Wampa_CheckRoar( NPC ) )
			{
				return;
			}
		}
		Wampa_Attack( distance, doCharge );
	}
}

/*
-------------------------
NPC_Wampa_Pain
-------------------------
*/
void NPC_Wampa_Pain( gentity_t *self, gentity_t *inflictor, gentity_t *other, const vec3_t point, int damage, int mod,int hitLoc )
{
	qboolean hitByWampa = qfalse;
	if ( self->count )
	{//FIXME: need pain anim
		NPC_SetAnim( self, SETANIM_BOTH, BOTH_STAND2TO1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
		TIMER_Set( self, "takingPain", self->client->ps.legsAnimTimer );
		TIMER_Set(self,"attacking",-level.time);
		return;
	}
	if ( other&&other->client&&other->client->NPC_class==CLASS_WAMPA )
	{
		hitByWampa = qtrue;
	}
	if ( other
		&& other->inuse
		&& other != self->enemy
		&& !(other->flags&FL_NOTARGET) )
	{
		if ( (!other->s.number&&!Q_irand(0,3))
			|| !self->enemy
			|| self->enemy->health == 0
			|| (self->enemy->client&&self->enemy->client->NPC_class == CLASS_WAMPA)
			|| (!Q_irand(0, 4 ) && DistanceSquared( other->currentOrigin, self->currentOrigin ) < DistanceSquared( self->enemy->currentOrigin, self->currentOrigin )) )
		{//if my enemy is dead (or attacked by player) and I'm not still holding/eating someone, turn on the attacker
			//FIXME: if can't nav to my enemy, take this guy if I can nav to him
			self->lastEnemy = other;
			G_SetEnemy( self, other );
			if ( self->enemy != self->lastEnemy )
			{//clear this so that we only sniff the player the first time we pick them up
				self->useDebounceTime = 0;
			}
			TIMER_Set( self, "lookForNewEnemy", Q_irand( 5000, 15000 ) );
			if ( hitByWampa )
			{//stay mad at this Wampa for 2-5 secs before looking for other enemies
				TIMER_Set( self, "wampaInfight", Q_irand( 2000, 5000 ) );
			}
		}
	}
	if ( (hitByWampa|| Q_irand( 0, 100 ) < damage )//hit by wampa, hit while holding live victim, or took a lot of damage
		&& self->client->ps.legsAnim != BOTH_GESTURE1
		&& self->client->ps.legsAnim != BOTH_GESTURE2
		&& TIMER_Done( self, "takingPain" ) )
	{
		if ( !Wampa_CheckRoar( self ) )
		{
			if ( self->client->ps.legsAnim != BOTH_ATTACK1
				&& self->client->ps.legsAnim != BOTH_ATTACK2
				&& self->client->ps.legsAnim != BOTH_ATTACK3 )
			{//cant interrupt one of the big attack anims
				if ( self->health > 100 || hitByWampa )
				{
					TIMER_Remove( self, "attacking" );

					VectorCopy( self->NPC->lastPathAngles, self->s.angles );

					if ( !Q_irand( 0, 1 ) )
					{
						NPC_SetAnim( self, SETANIM_BOTH, BOTH_PAIN2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
					}
					else
					{
						NPC_SetAnim( self, SETANIM_BOTH, BOTH_PAIN1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
					}
					TIMER_Set( self, "takingPain", self->client->ps.legsAnimTimer+Q_irand(0, 500*(2-g_spskill->integer)) );
					TIMER_Set(self,"attacking",-level.time);
					//allow us to re-evaluate our running speed/anim
					TIMER_Set( self, "runfar", -1 );
					TIMER_Set( self, "runclose", -1 );
					TIMER_Set( self, "walk", -1 );

					if ( self->NPC )
					{
						self->NPC->localState = LSTATE_WAITING;
					}
				}
			}
		}
	}
}

void Wampa_DropVictim( gentity_t *self )
{
	//FIXME: if Wampa dies, it should drop its victim.
	//FIXME: if Wampa is removed, it must remove its victim.
	//FIXME: if in BOTH_HOLD_DROP, throw them a little, too?
	if ( self->health > 0 )
	{
		NPC_SetAnim( self, SETANIM_BOTH, BOTH_STAND2TO1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	}
	TIMER_Set(self,"attacking",-level.time);
	if ( self->activator )
	{
		if ( self->activator->client )
		{
			self->activator->client->ps.eFlags &= ~EF_HELD_BY_WAMPA;
		}
		self->activator->activator = NULL;
		NPC_SetAnim( self->activator, SETANIM_BOTH, BOTH_RELEASED, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		self->activator->client->ps.legsAnimTimer += 500;
		self->activator->client->ps.weaponTime = self->activator->client->ps.torsoAnimTimer = self->activator->client->ps.legsAnimTimer;
		if ( self->activator->health > 0 )
		{
			if ( self->activator->NPC )
			{//start thinking again
				self->activator->NPC->nextBStateThink = level.time;
			}
			if ( self->activator->client && self->activator->s.number < MAX_CLIENTS )
			{
				vec3_t vicAngles = {30,AngleNormalize180(self->client->ps.viewangles[YAW]+180),0};
				SetClientViewAngle( self->activator, vicAngles );
			}
		}
		else
		{
			if ( self->enemy == self->activator )
			{
				self->enemy = NULL;
			}
			self->activator->clipmask &= ~CONTENTS_BODY;
		}
		self->activator = NULL;
	}
	self->count = 0;//drop him
}

qboolean Wampa_CheckDropVictim( gentity_t *self, qboolean excludeMe )
{
	if ( !self
		|| !self->activator )
	{
		return qtrue;
	}
	vec3_t mins={self->activator->mins[0]-1,self->activator->mins[1]-1,0};
	vec3_t maxs={self->activator->maxs[0]+1,self->activator->maxs[1]+1,1};
	vec3_t start={self->activator->currentOrigin[0],self->activator->currentOrigin[1],self->activator->absmin[2]};
	vec3_t end={self->activator->currentOrigin[0],self->activator->currentOrigin[1],self->activator->absmax[2]-1};
	trace_t	trace;
	if ( excludeMe )
	{
		gi.unlinkentity( self );
	}
	gi.trace( &trace, start, mins, maxs, end, self->activator->s.number, self->activator->clipmask, (EG2_Collision)0, 0 );
	if ( excludeMe )
	{
		gi.linkentity( self );
	}
	if ( !trace.allsolid && !trace.startsolid && trace.fraction >= 1.0f )
	{
		Wampa_DropVictim( self );
		return qtrue;
	}
	if ( excludeMe )
	{//victim stuck in wall
		if ( self->NPC )
		{//turn
			self->NPC->desiredYaw += Q_irand( -30, 30 );
			self->NPC->lockedDesiredYaw = self->NPC->desiredYaw;
		}
	}
	return qfalse;
}

extern float NPC_EnemyRangeFromBolt( int boltIndex );
qboolean Wampa_TryGrab( void )
{
	const float	radius = 64.0f;

	if ( !NPC->enemy
		|| !NPC->enemy->client
		|| NPC->enemy->health <= 0 )
	{
		return qfalse;
	}

	float enemyDist = NPC_EnemyRangeFromBolt( NPC->handRBolt );
	if ( enemyDist <= radius
		&& !NPC->count //don't have one in hand already
		&& NPC->enemy->client->NPC_class != CLASS_RANCOR
		&& NPC->enemy->client->NPC_class != CLASS_GALAKMECH
		&& NPC->enemy->client->NPC_class != CLASS_ATST
		&& NPC->enemy->client->NPC_class != CLASS_GONK
		&& NPC->enemy->client->NPC_class != CLASS_R2D2
		&& NPC->enemy->client->NPC_class != CLASS_R5D2
		&& NPC->enemy->client->NPC_class != CLASS_MARK1
		&& NPC->enemy->client->NPC_class != CLASS_MARK2
		&& NPC->enemy->client->NPC_class != CLASS_MOUSE
		&& NPC->enemy->client->NPC_class != CLASS_PROBE
		&& NPC->enemy->client->NPC_class != CLASS_SEEKER
		&& NPC->enemy->client->NPC_class != CLASS_REMOTE
		&& NPC->enemy->client->NPC_class != CLASS_SENTRY
		&& NPC->enemy->client->NPC_class != CLASS_INTERROGATOR
		&& NPC->enemy->client->NPC_class != CLASS_VEHICLE )
	{//grab
		NPC->enemy = NPC->enemy;//make him my new best friend
		NPC->enemy->client->ps.eFlags |= EF_HELD_BY_WAMPA;
		//FIXME: this makes it so that the victim can't hit us with shots!  Just use activator or something
		NPC->enemy->activator = NPC; // kind of dumb, but when we are locked to the Rancor, we are owned by it.
		NPC->activator = NPC->enemy;//remember him
		NPC->count = 1;//in my hand
		//wait to attack
		TIMER_Set( NPC, "attacking", NPC->client->ps.legsAnimTimer + Q_irand(500, 2500) );
		NPC_SetAnim( NPC->enemy, SETANIM_BOTH, BOTH_GRABBED, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_HOLD_END, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		TIMER_Set( NPC, "takingPain", -level.time );
		return qtrue;
	}
	else if ( enemyDist < radius*2.0f )
	{//smack
		G_Sound( NPC->enemy, G_SoundIndex( "sound/chars/rancor/swipehit.wav" ) );
		//actually push the enemy
		vec3_t pushDir;
		vec3_t angs;
		VectorCopy( NPC->client->ps.viewangles, angs );
		angs[YAW] += Q_flrand( 25, 50 );
		angs[PITCH] = Q_flrand( -25, -15 );
		AngleVectors( angs, pushDir, NULL, NULL );
		if ( NPC->enemy->client->NPC_class != CLASS_RANCOR
			&& NPC->enemy->client->NPC_class != CLASS_ATST
			&& !(NPC->enemy->flags&FL_NO_KNOCKBACK) )
		{
			G_Throw( NPC->enemy, pushDir, Q_irand( 30, 70 ) );
			if ( NPC->enemy->health > 0 )
			{//do pain on enemy
				G_Knockdown( NPC->enemy, NPC, pushDir, 300, qtrue );
			}
		}
	}
	return qfalse;
}

/*
-------------------------
NPC_BSWampa_Default
-------------------------
*/
void NPC_BSWampa_Default( void )
{
	//NORMAL ANIMS
	//	stand1 = normal stand
	//	walk1 = normal, non-angry walk
	//	walk2 = injured
	//	run1 = far away run
	//	run2 = close run
	//VICTIM ANIMS
	//	grabswipe = melee1 - sweep out and grab
	//	stand2 attack = attack4 - while holding victim, swipe at him
	//	walk3_drag = walk5 - walk with drag
	//	stand2 = hold victim
	//	stand2to1 = drop victim
	if ( NPC->client->ps.legsAnim == BOTH_HOLD_START )
	{
		NPC_FaceEnemy( qtrue );
		if ( NPC->client->ps.legsAnimTimer < 200 )
		{//see if he's there to grab
			if ( !Wampa_TryGrab() )
			{
				NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_HOLD_MISS, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			}
		}
		return;
	}


	if ( NPC->count )
	{
		if ( !NPC->activator
			|| !NPC->activator->client )
		{//wtf?
			NPC->count = 0;
			NPC->activator = NULL;
		}
		else
		{
			if ( NPC->client->ps.legsAnim == BOTH_HOLD_DROP )
			{
				if ( NPC->client->ps.legsAnimTimer < PM_AnimLength(NPC->client->clientInfo.animFileIndex, (animNumber_t)NPC->client->ps.legsAnim)-500 )
				{//at least half a second into the anim
					if ( Wampa_CheckDropVictim( NPC, qfalse ) )
					{
						TIMER_Set( NPC, "attacking", 1000+(Q_irand(500,1000)*(3-g_spskill->integer)) );
					}
				}
			}
			else if ( !TIMER_Done( NPC, "takingPain" ) )
			{
				Wampa_CheckDropVictim( NPC, qfalse );
			}
			else if ( NPC->activator->health <= 0 )
			{
				if ( TIMER_Done(NPC,"sniffCorpse") )
				{
					Wampa_CheckDropVictim( NPC, qfalse );
				}
			}
			else if ( NPC->useDebounceTime >= level.time
				&& NPC->activator )
			{//just sniffing the guy
				if ( NPC->useDebounceTime <= level.time + 100
					&& NPC->client->ps.legsAnim != BOTH_HOLD_DROP)
				{//just about done, drop him
					NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_HOLD_DROP, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
					TIMER_Set( NPC, "attacking", NPC->client->ps.legsAnimTimer+500 );
				}
			}
			else
			{
				if ( !NPC->useDebounceTime
					&& NPC->activator
					&& NPC->activator->s.number < MAX_CLIENTS )
				{//first time I pick the player, just sniff them
					if ( TIMER_Done(NPC,"attacking") )
					{//ready to attack
						NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_HOLD_SNIFF, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
						NPC->useDebounceTime = level.time + NPC->client->ps.legsAnimTimer + Q_irand( 500, 2000 );
					}
				}
				else
				{
					if ( TIMER_Done(NPC,"attacking") )
					{//ready to attack
						NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_HOLD_ATTACK/*BOTH_ATTACK4*/, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
						TIMER_Set(NPC,"grabAttackDamage",1400);
						TIMER_Set(NPC,"attacking",NPC->client->ps.legsAnimTimer+Q_irand(3000,10000));
					}

					if ( NPC->client->ps.legsAnim == BOTH_HOLD_ATTACK )
					{
						if ( NPC->client->ps.legsAnimTimer )
						{
							if ( TIMER_Done2(NPC,"grabAttackDamage",qtrue) )
							{
								G_Sound( NPC->activator, G_SoundIndex( "sound/chars/rancor/swipehit.wav" ) );
								G_Damage( NPC->activator, NPC, NPC, vec3_origin, NPC->activator->currentOrigin, Q_irand( 25, 40 ), (DAMAGE_NO_KNOCKBACK|DAMAGE_NO_ARMOR), MOD_MELEE );
								if ( NPC->activator->health <= 0 )
								{//killed them, chance of dismembering
									int hitLoc = HL_WAIST;
									if ( g_dismemberment->integer < 4 )
									{
										hitLoc = Q_irand( HL_BACK_RT, HL_HAND_LT );
									}
									else
									{
										hitLoc = Q_irand( HL_WAIST, HL_HEAD );
									}
									NPC->activator->client->dismembered = false;
									//FIXME: the limb should just disappear, cuz I ate it
									G_DoDismemberment( NPC->activator, NPC->activator->currentOrigin, MOD_SABER, 1000, hitLoc, qtrue );
									TIMER_Set( NPC, "sniffCorpse", Q_irand( 2000, 5000 ) );
								}
								NPC_SetAnim( NPC->activator, SETANIM_BOTH, BOTH_HANG_PAIN, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
							}
						}
						else
						{
							NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_HOLD_IDLE/*BOTH_ATTACK4*/, SETANIM_FLAG_NORMAL );
						}
					}
					else if ( NPC->client->ps.legsAnim == BOTH_STAND2TO1
						&& !NPC->client->ps.legsAnimTimer )
					{
						NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_HOLD_IDLE, SETANIM_FLAG_NORMAL );
					}
				}
			}
		}

		NPC_UpdateAngles( qtrue, qtrue );
		return;
	}

	if ( NPCInfo->localState == LSTATE_WAITING
		&& TIMER_Done2( NPC, "takingPain", qtrue ) )
	{//was not doing anything because we were taking pain, but pain is done now, so clear it...
		NPCInfo->localState = LSTATE_CLEAR;
	}

	if ( !TIMER_Done( NPC, "rageTime" ) )
	{//do nothing but roar first time we see an enemy
		NPC_FaceEnemy( qtrue );
		return;
	}
	if ( NPC->enemy )
	{
		if ( NPC->enemy->client //enemy is a client
			&& (NPC->enemy->client->NPC_class == CLASS_UGNAUGHT || NPC->enemy->client->NPC_class == CLASS_JAWA )//enemy is a lowly jawa or ugnaught
			&& NPC->enemy->enemy != NPC//enemy's enemy is not me
			&& (!NPC->enemy->enemy || !NPC->enemy->enemy->client || NPC->enemy->enemy->client->NPC_class!=CLASS_RANCOR) )//enemy's enemy is not a client or is not a rancor (which is scarier than me)
		{//they should be scared of ME and no-one else
			G_SetEnemy( NPC->enemy, NPC );
		}
		if ( !TIMER_Done(NPC,"attacking") )
		{//in middle of attack
			//face enemy
			NPC_FaceEnemy( qtrue );
			//continue attack logic
			enemyDist = Distance( NPC->currentOrigin, NPC->enemy->currentOrigin );
			Wampa_Attack( enemyDist, qfalse );
			return;
		}
		else
		{
			if ( TIMER_Done(NPC,"angrynoise") )
			{
				G_SoundOnEnt( NPC, CHAN_AUTO, va("sound/chars/wampa/misc/anger%d.wav", Q_irand(1, 2)) );

				TIMER_Set( NPC, "angrynoise", Q_irand( 5000, 10000 ) );
			}
			//else, if he's in our hand, we eat, else if he's on the ground, we keep attacking his dead body for a while
			if( NPC->enemy->client && NPC->enemy->client->NPC_class == CLASS_WAMPA )
			{//got mad at another Wampa, look for a valid enemy
				if ( TIMER_Done( NPC, "wampaInfight" ) )
				{
					NPC_CheckEnemyExt( qtrue );
				}
			}
			else
			{
				if ( NPC_ValidEnemy( NPC->enemy ) == qfalse )
				{
					TIMER_Remove( NPC, "lookForNewEnemy" );//make them look again right now
					if ( !NPC->enemy->inuse || level.time - NPC->enemy->s.time > Q_irand( 10000, 15000 ) )
					{//it's been a while since the enemy died, or enemy is completely gone, get bored with him
						NPC->enemy = NULL;
						Wampa_Patrol();
						NPC_UpdateAngles( qtrue, qtrue );
						return;
					}
				}
				if ( TIMER_Done( NPC, "lookForNewEnemy" ) )
				{
					gentity_t *sav_enemy = NPC->enemy;//FIXME: what about NPC->lastEnemy?
					NPC->enemy = NULL;
					gentity_t *newEnemy = NPC_CheckEnemy( (qboolean)(NPCInfo->confusionTime < level.time), qfalse, qfalse );
					NPC->enemy = sav_enemy;
					if ( newEnemy && newEnemy != sav_enemy )
					{//picked up a new enemy!
						NPC->lastEnemy = NPC->enemy;
						G_SetEnemy( NPC, newEnemy );
						if ( NPC->enemy != NPC->lastEnemy )
						{//clear this so that we only sniff the player the first time we pick them up
							NPC->useDebounceTime = 0;
						}
						//hold this one for at least 5-15 seconds
						TIMER_Set( NPC, "lookForNewEnemy", Q_irand( 5000, 15000 ) );
					}
					else
					{//look again in 2-5 secs
						TIMER_Set( NPC, "lookForNewEnemy", Q_irand( 2000, 5000 ) );
					}
				}
			}
			Wampa_Combat();
			return;
		}
	}
	else
	{
		if ( TIMER_Done(NPC,"idlenoise") )
		{
			G_SoundOnEnt( NPC, CHAN_AUTO, "sound/chars/wampa/misc/anger3.wav" );

			TIMER_Set( NPC, "idlenoise", Q_irand( 2000, 4000 ) );
		}
		if ( NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES )
		{
			Wampa_Patrol();
		}
		else
		{
			Wampa_Idle();
		}
	}

	NPC_UpdateAngles( qtrue, qtrue );
}
