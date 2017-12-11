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
#include "../cgame/cg_local.h"
#include "g_functions.h"
#include "g_navigator.h"
#include "g_nav.h"

// These define the working combat range for these suckers
#define MIN_DISTANCE		128
#define MIN_DISTANCE_SQR	( MIN_DISTANCE * MIN_DISTANCE )

#define MAX_DISTANCE		1024
#define MAX_DISTANCE_SQR	( MAX_DISTANCE * MAX_DISTANCE )

#define LSTATE_CLEAR		0
#define LSTATE_WAITING		1

#define SPF_RANCOR_MUTANT	1
#define SPF_RANCOR_FASTKILL	2

extern qboolean G_EntIsBreakable( int entityNum, gentity_t *breaker );
extern cvar_t	*g_dismemberment;
extern cvar_t	*g_bobaDebug;

void Rancor_Attack( float distance, qboolean doCharge, qboolean aimAtBlockedEntity );
/*
-------------------------
NPC_Rancor_Precache
-------------------------
*/
void NPC_Rancor_Precache( void )
{
	int i;
	for ( i = 1; i < 5; i ++ )
	{
		G_SoundIndex( va("sound/chars/rancor/snort_%d.wav", i) );
	}
	G_SoundIndex( "sound/chars/rancor/swipehit.wav" );
	G_SoundIndex( "sound/chars/rancor/chomp.wav" );
}

void NPC_MutantRancor_Precache( void )
{
	G_SoundIndex( "sound/chars/rancor/breath_start.wav" );
	G_SoundIndex( "sound/chars/rancor/breath_loop.wav" );
	G_EffectIndex( "mrancor/breath" );
}
//FIXME: initialize all my timers

qboolean Rancor_CheckAhead( vec3_t end )
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

	if ( trace.entityNum < ENTITYNUM_WORLD
		&& G_EntIsBreakable( trace.entityNum, NPC ) )
	{//breakable brush in our way, break it
	//	NPCInfo->blockedEntity = &g_entities[trace.entityNum];
		return qtrue;
	}

	//Aw screw it, always try to go straight at him if we can at all
	if ( trace.fraction >= 0.25f )
		return qtrue;

	//FIXME: if something in the way that's not the world, set blocked ent
	return qfalse;
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
		NPC_SetAnim( self, SETANIM_BOTH, BOTH_STAND1TO2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
		TIMER_Set( self, "rageTime", self->client->ps.legsAnimTimer );
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
		NPCInfo->goalRadius = NPC->maxs[0]+(MIN_DISTANCE*NPC->s.modelScale[0]);	// just get us within combat range
		//FIXME: for some reason, if NPC_MoveToGoal fails, it sets my angles to my lastPathAngles, which I don't want
		float savYaw = NPCInfo->desiredYaw;
		bool	savWalking = !!(ucmd.buttons&BUTTON_WALKING);
		if ( !NPC_MoveToGoal( qtrue ) )
		{//can't macro-nav, just head right for him
			//FIXME: if something in the way that's not the world, set blocked ent
			vec3_t dest;
			VectorCopy( NPCInfo->goalEntity->currentOrigin, dest );
			if ( Rancor_CheckAhead( dest ) )
			{//use our temp move straight to goal check
				if (!savWalking)
				{
					ucmd.buttons &= ~BUTTON_WALKING;				// Unset from MoveToGoal()
				}
				STEER::Activate(NPC);
				STEER::Seek(NPC, dest);
				STEER::AvoidCollisions(NPC);
				STEER::DeActivate(NPC, &ucmd);
				/*
				VectorSubtract( dest, NPC->currentOrigin, NPC->client->ps.moveDir );
				NPC->client->ps.speed = VectorNormalize( NPC->client->ps.moveDir );
				NPCInfo->desiredYaw = vectoyaw( NPC->client->ps.moveDir );
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
				*/
			}
			else
			{//all else fails, look at him
			//	gi.Printf("Fail\n");
				NPCInfo->lockedDesiredYaw = NPCInfo->desiredYaw = savYaw;
			/*	if ( !NPCInfo->blockedEntity )
				{//not already trying to break a breakable somewhere
					if ( NPC->enemy && NPC->enemy->client && NPC->enemy->client->ps.groundEntityNum < ENTITYNUM_WORLD )
					{//hmm, maybe he's on a breakable brush?
						if ( G_EntIsBreakable( NPC->enemy->client->ps.groundEntityNum, NPC ) )
						{//break it!
							gentity_t *breakable = &g_entities[NPC->enemy->client->ps.groundEntityNum];
							int		sanityCheck = 0;
							//FiXME: and if he's on a stack of 3 breakables?
							//FIXME: See if the breakable has a targetname, if so see if the thing targeting it is a breakable, if so, etc...
							while ( sanityCheck < 20 && breakable && breakable->targetname )
							{
								gentity_t *breakableNext = NULL;
								while ( sanityCheck < 20 && (breakableNext = G_Find( breakableNext, FOFS(target), breakable->targetname )) != NULL )
								{
									if ( breakableNext && G_EntIsBreakable( breakableNext->s.number, NPC ) )
									{
										breakable = breakableNext;
										break;
									}
									else
									{
										sanityCheck++;
									}
								}
								if ( !breakableNext )
								{//not targetted by another breakable that we can break
									break;
								}
								else
								{
									sanityCheck++;
								}
							}
							NPCInfo->blockedEntity = breakable;
						}
					}
				}*/
				if ( !NPCInfo->blockedEntity && NPC->enemy && gi.inPVS(NPC->currentOrigin, NPC->enemy->currentOrigin))
				{//nothing to destroy?  just go straight at goal dest
					qboolean horzClose = qfalse;
					if (!savWalking)
					{
						ucmd.buttons &= ~BUTTON_WALKING;				// Unset from MoveToGoal()
					}

					if ( DistanceHorizontal( NPC->enemy->currentOrigin, NPC->currentOrigin ) < (NPC->maxs[0]+(MIN_DISTANCE*NPC->s.modelScale[0])) )
					{//close, just look at him
						horzClose = qtrue;
						NPC_FaceEnemy( qtrue );
					}
					else
					{//try to move  towards him
						STEER::Activate(NPC);
						STEER::Seek(NPC, dest);
						STEER::AvoidCollisions(NPC);
						STEER::DeActivate(NPC, &ucmd);
					}
					//let him know he should attack at random out of frustration?
					if ( NPCInfo->goalEntity == NPC->enemy )
					{
						if ( TIMER_Done( NPC, "attacking" )
							&& TIMER_Done( NPC, "frustrationAttack" ) )
						{
							float enemyDist = Distance( dest, NPC->currentOrigin );
							if ( (!horzClose||!Q_irand(0,5))
								&& Q_irand( 0, 1 ) )
							{
								Rancor_Attack( enemyDist, qtrue, qfalse );
							}
							else
							{
								Rancor_Attack( enemyDist, qfalse, qfalse );
							}
							if ( horzClose )
							{
								TIMER_Set( NPC, "frustrationAttack", Q_irand( 2000, 5000 ) );
							}
							else
							{
								TIMER_Set( NPC, "frustrationAttack", Q_irand( 5000, 15000 ) );
							}
						}
					}
				}
			}
		}
	}
}

//---------------------------------------------------------
extern void G_Knockdown( gentity_t *self, gentity_t *attacker, const vec3_t pushDir, float strength, qboolean breakSaberLock );
extern qboolean G_DoDismemberment( gentity_t *self, vec3_t point, int mod, int damage, int hitLoc, qboolean force = qfalse );
extern float NPC_EntRangeFromBolt( gentity_t *targEnt, int boltIndex );
extern int NPC_GetEntsNearBolt( gentity_t **radiusEnts, float radius, int boltIndex, vec3_t boltOrg );

void Rancor_DropVictim( gentity_t *self )
{
	//FIXME: if Rancor dies, it should drop its victim.
	//FIXME: if Rancor is removed, it must remove its victim.
	//FIXME: if in BOTH_HOLD_DROP, throw them a little, too?
	if ( self->activator )
	{
		if ( self->activator->client )
		{
			self->activator->client->ps.eFlags &= ~EF_HELD_BY_RANCOR;
		}
		self->activator->activator = NULL;
		if ( self->activator->health <= 0 )
		{
			if ( self->activator->s.number )
			{//never free player
				if ( self->count == 1 )
				{//in my hand, just drop them
					if ( self->activator->client )
					{
						self->activator->client->ps.legsAnimTimer = self->activator->client->ps.torsoAnimTimer = 0;
						//FIXME: ragdoll?
					}
				}
				else
				{
					G_FreeEntity( self->activator );
				}
			}
			else
			{
				self->activator->s.eFlags |= EF_NODRAW;
				if ( self->activator->client )
				{
					self->activator->client->ps.eFlags |= EF_NODRAW;
				}
				self->activator->clipmask &= ~CONTENTS_BODY;
			}
		}
		else
		{
			if ( self->activator->NPC )
			{//start thinking again
				self->activator->NPC->nextBStateThink = level.time;
			}
			//clear their anim and let them fall
			self->activator->client->ps.legsAnimTimer = self->activator->client->ps.torsoAnimTimer = 0;
		}
		if ( self->enemy == self->activator )
		{
			self->enemy = NULL;
		}
		if ( self->activator->s.number == 0 )
		{//don't attack the player again for a bit
			TIMER_Set( self, "attackDebounce", Q_irand( 2000, 4000+((2-g_spskill->integer)*2000) ) );
		}
		self->activator = NULL;
	}
	self->count = 0;//drop him
}

void Rancor_Swing( int boltIndex, qboolean tryGrab )
{
	gentity_t	*radiusEnts[ 128 ];
	int			numEnts;
	const float	radius = (NPC->spawnflags&SPF_RANCOR_MUTANT)?200:88;
	const float	radiusSquared = (radius*radius);
	int			i;
	vec3_t		boltOrg;
	vec3_t		originUp;

	VectorCopy(NPC->currentOrigin, originUp);
	originUp[2] += (NPC->maxs[2]*0.75f);

	numEnts = NPC_GetEntsNearBolt( radiusEnts, radius, boltIndex, boltOrg );

	//if ( NPCInfo->blockedEntity && G_EntIsBreakable( NPCInfo->blockedEntity->s.number, NPC ) )
	{//attacking a breakable brush
		//HMM... maybe always do this?
		//if boltOrg inside a breakable brush, damage it
		trace_t trace;
		gi.trace( &trace, NPC->pos3, vec3_origin, vec3_origin, boltOrg, NPC->s.number, CONTENTS_SOLID|CONTENTS_BODY, (EG2_Collision)0, 0 );
#ifndef FINAL_BUILD
		if ( g_bobaDebug->integer > 0 )
		{
			G_DebugLine(NPC->pos3, boltOrg, 1000, 0x000000ff, qtrue);
		}
#endif
		//remember pos3 for the trace from last hand pos to current hand pos next time
		VectorCopy( boltOrg, NPC->pos3 );
		//FIXME: also do a trace TO the bolt from where we are...?
		if ( G_EntIsBreakable( trace.entityNum, NPC ) )
		{
			G_Damage( &g_entities[trace.entityNum], NPC, NPC, vec3_origin, boltOrg, 100, 0, MOD_MELEE );
		}
		else
		{//fuck, do an actual line trace, I guess...
			gi.trace( &trace, originUp, vec3_origin, vec3_origin, boltOrg, NPC->s.number, CONTENTS_SOLID|CONTENTS_BODY, (EG2_Collision)0, 0 );
#ifndef FINAL_BUILD
			if ( g_bobaDebug->integer > 0 )
			{
				G_DebugLine(originUp, boltOrg, 1000, 0x000000ff, qtrue);
			}
#endif
			if ( G_EntIsBreakable( trace.entityNum, NPC ) )
			{
				G_Damage( &g_entities[trace.entityNum], NPC, NPC, vec3_origin, boltOrg, 200, 0, MOD_MELEE );
			}
		}
	}

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
			continue;
		}

		if ( (radiusEnts[i]->client->ps.eFlags&EF_HELD_BY_RANCOR)
			||(radiusEnts[i]->client->ps.eFlags&EF_HELD_BY_WAMPA) )
		{//can't be one already being held
			continue;
		}

		if ( (radiusEnts[i]->s.eFlags&EF_NODRAW) )
		{//not if invisible
			continue;
		}
		/*
		if ( !radiusEnts[i]->contents )
		{//not if non-solid
			continue;
		}
		*/

		if ( DistanceSquared( radiusEnts[i]->currentOrigin, boltOrg ) <= radiusSquared )
		{
			if ( !gi.inPVS( radiusEnts[i]->currentOrigin, NPC->currentOrigin ) )
			{//don't grab anything that's in another PVS
				continue;
			}
			/*
			qboolean skipGrab = qfalse;
			if ( tryGrab//want to grab
				&& (NPC->spawnflags&SPF_RANCOR_FASTKILL)//mutant rancor
				&& radiusEnts[i]->s.number >= MAX_CLIENTS //not the player
				&& Q_irand( 0, 1 ) )//50% chance
			{//don't grab them, just smack them away
				skipGrab = qtrue;
			}
			*/
			if ( tryGrab
				//&& !skipGrab
				&& NPC->count != 1 //don't have one in hand or in mouth already - FIXME: allow one in hand and any number in mouth!
				&& radiusEnts[i]->client->NPC_class != CLASS_RANCOR
				&& radiusEnts[i]->client->NPC_class != CLASS_GALAKMECH
				&& radiusEnts[i]->client->NPC_class != CLASS_ATST
				&& radiusEnts[i]->client->NPC_class != CLASS_GONK
				&& radiusEnts[i]->client->NPC_class != CLASS_R2D2
				&& radiusEnts[i]->client->NPC_class != CLASS_R5D2
				&& radiusEnts[i]->client->NPC_class != CLASS_MARK1
				&& radiusEnts[i]->client->NPC_class != CLASS_MARK2
				&& radiusEnts[i]->client->NPC_class != CLASS_MOUSE
				&& radiusEnts[i]->client->NPC_class != CLASS_PROBE
				&& radiusEnts[i]->client->NPC_class != CLASS_SEEKER
				&& radiusEnts[i]->client->NPC_class != CLASS_REMOTE
				&& radiusEnts[i]->client->NPC_class != CLASS_SENTRY
				&& radiusEnts[i]->client->NPC_class != CLASS_INTERROGATOR
				&& radiusEnts[i]->client->NPC_class != CLASS_VEHICLE )
			{//grab
				if ( NPC->count == 2 )
				{//have one in my mouth, remove him
					TIMER_Remove( NPC, "clearGrabbed" );
					Rancor_DropVictim( NPC );
				}
				NPC->enemy = radiusEnts[i];//make him my new best friend
				radiusEnts[i]->client->ps.eFlags |= EF_HELD_BY_RANCOR;
				//FIXME: this makes it so that the victim can't hit us with shots!  Just use activator or something
				radiusEnts[i]->activator = NPC; // kind of dumb, but when we are locked to the Rancor, we are owned by it.
				NPC->activator = radiusEnts[i];//remember him
				NPC->count = 1;//in my hand
				//wait to attack
				TIMER_Set( NPC, "attacking", NPC->client->ps.legsAnimTimer + Q_irand(500, 2500) );
				if ( radiusEnts[i]->health > 0 )
				{//do pain on enemy
					GEntity_PainFunc( radiusEnts[i], NPC, NPC, radiusEnts[i]->currentOrigin, 0, MOD_CRUSH );
				}
				else if ( radiusEnts[i]->client )
				{
					NPC_SetAnim( radiusEnts[i], SETANIM_BOTH, BOTH_SWIM_IDLE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
				}
			}
			else
			{//smack
				G_Sound( radiusEnts[i], G_SoundIndex( "sound/chars/rancor/swipehit.wav" ) );
				//actually push the enemy
				vec3_t pushDir;
				/*
				//VectorSubtract( radiusEnts[i]->currentOrigin, boltOrg, pushDir );
				VectorSubtract( radiusEnts[i]->currentOrigin, NPC->currentOrigin, pushDir );
				pushDir[2] = Q_flrand( 100, 200 );
				VectorNormalize( pushDir );
				*/
				if ( (NPC->spawnflags&SPF_RANCOR_FASTKILL)
					&& radiusEnts[i]->s.number >= MAX_CLIENTS )
				{
					G_Damage( radiusEnts[i], NPC, NPC, vec3_origin, boltOrg, radiusEnts[i]->health+1000, (DAMAGE_NO_KNOCKBACK|DAMAGE_NO_PROTECTION), MOD_MELEE );
				}
				vec3_t angs;
				VectorCopy( NPC->client->ps.viewangles, angs );
				angs[YAW] += Q_flrand( 25, 50 );
				angs[PITCH] = Q_flrand( -25, -15 );
				AngleVectors( angs, pushDir, NULL, NULL );
				if ( radiusEnts[i]->client->NPC_class != CLASS_RANCOR
					&& radiusEnts[i]->client->NPC_class != CLASS_ATST
					&& !(radiusEnts[i]->flags&FL_NO_KNOCKBACK) )
				{
					G_Throw( radiusEnts[i], pushDir, 250 );
					if ( radiusEnts[i]->health > 0 )
					{//do pain on enemy
						G_Knockdown( radiusEnts[i], NPC, pushDir, 100, qtrue );
					}
				}
			}
		}
	}
}

void Rancor_Smash( void )
{
	gentity_t	*radiusEnts[ 128 ];
	int			numEnts;
	const float	radius = (NPC->spawnflags&SPF_RANCOR_MUTANT)?256:128;
	const float	halfRadSquared = ((radius/2)*(radius/2));
	const float	radiusSquared = (radius*radius);
	float		distSq;
	int			i;
	vec3_t		boltOrg;

	AddSoundEvent( NPC, NPC->currentOrigin, 512, AEL_DANGER, qfalse, qtrue );

	numEnts = NPC_GetEntsNearBolt( radiusEnts, radius, NPC->handLBolt, boltOrg );

	//if ( NPCInfo->blockedEntity && G_EntIsBreakable( NPCInfo->blockedEntity->s.number, NPC ) )
	{//attacking a breakable brush
		//HMM... maybe always do this?
		//if boltOrg inside a breakable brush, damage it
		trace_t trace;
		gi.trace( &trace, boltOrg, vec3_origin, vec3_origin, NPC->pos3, NPC->s.number, CONTENTS_SOLID|CONTENTS_BODY, (EG2_Collision)0, 0 );
#ifndef FINAL_BUILD
		if ( g_bobaDebug->integer > 0 )
		{
			G_DebugLine(NPC->pos3, boltOrg, 1000, 0x000000ff, qtrue);
		}
#endif
		//remember pos3 for the trace from last hand pos to current hand pos next time
		VectorCopy( boltOrg, NPC->pos3 );
		//FIXME: also do a trace TO the bolt from where we are...?
		if ( G_EntIsBreakable( trace.entityNum, NPC ) )
		{
			G_Damage( &g_entities[trace.entityNum], NPC, NPC, vec3_origin, boltOrg, 200, 0, MOD_MELEE );
		}
		else
		{//fuck, do an actual line trace, I guess...
			gi.trace( &trace, NPC->currentOrigin, vec3_origin, vec3_origin, boltOrg, NPC->s.number, CONTENTS_SOLID|CONTENTS_BODY, (EG2_Collision)0, 0 );
#ifndef FINAL_BUILD
			if ( g_bobaDebug->integer > 0 )
			{
				G_DebugLine(NPC->currentOrigin, boltOrg, 1000, 0x000000ff, qtrue);
			}
#endif
			if ( G_EntIsBreakable( trace.entityNum, NPC ) )
			{
				G_Damage( &g_entities[trace.entityNum], NPC, NPC, vec3_origin, boltOrg, 200, 0, MOD_MELEE );
			}
		}
	}

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
			if ( G_EntIsBreakable( radiusEnts[i]->s.number, NPC ) )
			{//damage breakables within range, but not as much
				if ( !Q_irand( 0, 1 ) )
				{
					G_Damage( radiusEnts[i], NPC, NPC, vec3_origin, radiusEnts[i]->currentOrigin, 100, 0, MOD_MELEE );
				}
			}
			continue;
		}

		if ( (radiusEnts[i]->client->ps.eFlags&EF_HELD_BY_RANCOR) )
		{//can't be one being held
			continue;
		}

		if ( (radiusEnts[i]->s.eFlags&EF_NODRAW) )
		{//not if invisible
			continue;
		}

		distSq = DistanceSquared( radiusEnts[i]->currentOrigin, boltOrg );
		if ( distSq <= radiusSquared )
		{
			if ( distSq < halfRadSquared )
			{//close enough to do damage, too
				G_Sound( radiusEnts[i], G_SoundIndex( "sound/chars/rancor/swipehit.wav" ) );
				if ( (NPC->spawnflags&SPF_RANCOR_FASTKILL)
					&& radiusEnts[i]->s.number >= MAX_CLIENTS )
				{
					G_Damage( radiusEnts[i], NPC, NPC, vec3_origin, boltOrg, radiusEnts[i]->health+1000, (DAMAGE_NO_KNOCKBACK|DAMAGE_NO_PROTECTION), MOD_MELEE );
				}
				else if ( (NPC->spawnflags&SPF_RANCOR_MUTANT) )//FIXME: a flag or something would be better
				{//more damage
					G_Damage( radiusEnts[i], NPC, NPC, vec3_origin, radiusEnts[i]->currentOrigin, Q_irand( 40, 55 ), DAMAGE_NO_KNOCKBACK, MOD_MELEE );
				}
				else
				{
					G_Damage( radiusEnts[i], NPC, NPC, vec3_origin, radiusEnts[i]->currentOrigin, Q_irand( 10, 25 ), DAMAGE_NO_KNOCKBACK, MOD_MELEE );
				}
			}
			if ( radiusEnts[i]->health > 0
				&& radiusEnts[i]->client
				&& radiusEnts[i]->client->NPC_class != CLASS_RANCOR
				&& radiusEnts[i]->client->NPC_class != CLASS_ATST )
			{
				if ( distSq < halfRadSquared
					|| radiusEnts[i]->client->ps.groundEntityNum != ENTITYNUM_NONE )
				{//within range of my fist or withing ground-shaking range and not in the air
					if ( (NPC->spawnflags&SPF_RANCOR_MUTANT) )
					{
						G_Knockdown( radiusEnts[i], NPC, vec3_origin, 500, qtrue );
					}
					else
					{
						G_Knockdown( radiusEnts[i], NPC, vec3_origin, Q_irand( 200, 350), qtrue );
					}
				}
			}
		}
	}
}

void Rancor_Bite( void )
{
	gentity_t	*radiusEnts[ 128 ];
	int			numEnts;
	const float	radius = 100;
	const float	radiusSquared = (radius*radius);
	int			i;
	vec3_t		boltOrg;

	numEnts = NPC_GetEntsNearBolt( radiusEnts, radius, NPC->gutBolt, boltOrg );

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
			continue;
		}

		if ( (radiusEnts[i]->client->ps.eFlags&EF_HELD_BY_RANCOR) )
		{//can't be one already being held
			continue;
		}

		if ( (radiusEnts[i]->s.eFlags&EF_NODRAW) )
		{//not if invisible
			continue;
		}

		if ( DistanceSquared( radiusEnts[i]->currentOrigin, boltOrg ) <= radiusSquared )
		{
			if ( (NPC->spawnflags&SPF_RANCOR_FASTKILL)
				&& radiusEnts[i]->s.number >= MAX_CLIENTS )
			{
				G_Damage( radiusEnts[i], NPC, NPC, vec3_origin, radiusEnts[i]->currentOrigin, radiusEnts[i]->health+1000, (DAMAGE_NO_KNOCKBACK|DAMAGE_NO_PROTECTION), MOD_MELEE );
			}
			else if ( (NPC->spawnflags&SPF_RANCOR_MUTANT) )
			{//more damage
				G_Damage( radiusEnts[i], NPC, NPC, vec3_origin, radiusEnts[i]->currentOrigin, Q_irand( 35, 50 ), DAMAGE_NO_KNOCKBACK, MOD_MELEE );
			}
			else
			{
				G_Damage( radiusEnts[i], NPC, NPC, vec3_origin, radiusEnts[i]->currentOrigin, Q_irand( 15, 30 ), DAMAGE_NO_KNOCKBACK, MOD_MELEE );
			}
			if ( radiusEnts[i]->health <= 0 && radiusEnts[i]->client )
			{//killed them, chance of dismembering
				if ( !Q_irand( 0, 1 ) )
				{//bite something off
					int hitLoc = HL_WAIST;
					if ( g_dismemberment->integer < 3 )
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
			G_Sound( radiusEnts[i], G_SoundIndex( "sound/chars/rancor/chomp.wav" ) );
		}
	}
}
//------------------------------
extern gentity_t *TossClientItems( gentity_t *self );
void Rancor_Attack( float distance, qboolean doCharge, qboolean aimAtBlockedEntity )
{
	if ( !TIMER_Exists( NPC, "attacking" )
		&& TIMER_Done( NPC, "attackDebounce" ) )
	{
		if ( NPC->count == 2 && NPC->activator )
		{
		}
		else if ( NPC->count == 1 && NPC->activator )
		{//holding enemy
			if ( (!(NPC->spawnflags&SPF_RANCOR_FASTKILL) ||NPC->activator->s.number<MAX_CLIENTS)
				&& NPC->activator->health > 0
				&& Q_irand( 0, 1 ) )
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
			if ( !Q_irand( 0, 3 ) )
			{
				NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_ATTACK5, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
				TIMER_Set( NPC, "attack_dmg", 1250 );
				if ( NPC->enemy && NPC->enemy->s.number == 0 )
				{//don't attack the player again for a bit
					TIMER_Set( NPC, "attackDebounce", NPC->client->ps.legsAnimTimer + Q_irand( 2000, 4000+((2-g_spskill->integer)*2000) ) );
				}
			}
			else if ( (NPC->spawnflags&SPF_RANCOR_MUTANT) )
			{//breath attack
				int breathAnim = BOTH_ATTACK4;
				gentity_t *checkEnt = NULL;
				vec3_t	center;
				if ( NPC->enemy && NPC->enemy->inuse )
				{
					checkEnt = NPC->enemy;
					VectorCopy( NPC->enemy->currentOrigin, center );
				}
				else if ( NPCInfo->blockedEntity && NPCInfo->blockedEntity->inuse )
				{
					checkEnt = NPCInfo->blockedEntity;
					//if it has an origin brush, use it...
					if ( VectorCompare( NPCInfo->blockedEntity->s.origin, vec3_origin ) )
					{//no origin brush, calc center
						VectorAdd( NPCInfo->blockedEntity->mins, NPCInfo->blockedEntity->maxs, center );
						VectorScale( center, 0.5f, center );
					}
					else
					{//use origin brush as center
						VectorCopy( NPCInfo->blockedEntity->s.origin, center );
					}
				}
				if ( checkEnt )
				{
					float zHeightRelative = center[2]-NPC->currentOrigin[2];
					if ( zHeightRelative >= (128.0f*NPC->s.modelScale[2]) )
					{
						breathAnim = BOTH_ATTACK7;
					}
					else if ( zHeightRelative >= (64.0f*NPC->s.modelScale[2]) )
					{
						breathAnim = BOTH_ATTACK6;
					}
				}
				NPC_SetAnim( NPC, SETANIM_BOTH, breathAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
				//start effect here
				G_PlayEffect( G_EffectIndex( "mrancor/breath" ), NPC->playerModel, NPC->gutBolt, NPC->s.number, NPC->currentOrigin, (NPC->client->ps.legsAnimTimer-500), qfalse );
				TIMER_Set( NPC, "breathAttack", NPC->client->ps.legsAnimTimer-500 );
				G_SoundOnEnt( NPC, CHAN_WEAPON, "sound/chars/rancor/breath_start.wav" );
				NPC->s.loopSound = G_SoundIndex( "sound/chars/rancor/breath_loop.wav" );
				if ( NPC->enemy && NPC->enemy->s.number == 0 )
				{//don't attack the player again for a bit
					TIMER_Set( NPC, "attackDebounce", NPC->client->ps.legsAnimTimer + Q_irand( 2000, 4000+((2-g_spskill->integer)*2000) ) );
				}
			}
			else
			{
				NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_MELEE2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
				TIMER_Set( NPC, "attack_dmg", 1250 );
				vec3_t	fwd, yawAng ={0, NPC->client->ps.viewangles[YAW], 0};
				AngleVectors( yawAng, fwd, NULL, NULL );
				VectorScale( fwd, distance*1.5f, NPC->client->ps.velocity );
				NPC->client->ps.velocity[2] = 150;
				NPC->client->ps.groundEntityNum = ENTITYNUM_NONE;
				if ( NPC->enemy && NPC->enemy->s.number == 0 )
				{//don't attack the player again for a bit
					TIMER_Set( NPC, "attackDebounce", NPC->client->ps.legsAnimTimer + Q_irand( 2000, 4000+((2-g_spskill->integer)*2000) ) );
				}
			}
		}
		else if ( !Q_irand(0, 1)
			/*&& (NPC->spawnflags&SPF_RANCOR_MUTANT)*/ )
		{//mutant rancor can smash
			NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_MELEE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
			TIMER_Set( NPC, "attack_dmg", 900 );
			//init pos3 for the trace from last hand pos to current hand pos
			VectorCopy( NPC->currentOrigin, NPC->pos3 );
		}
		else if ( (NPC->spawnflags&SPF_RANCOR_MUTANT)
			|| distance >= NPC->maxs[0]+(MIN_DISTANCE*NPC->s.modelScale[0])-64.0f )
		{//try to grab
			int grabAnim = BOTH_ATTACK2;
			gentity_t *checkEnt = NULL;
			vec3_t	center;
			if ( (!aimAtBlockedEntity||!NPCInfo->blockedEntity) && NPC->enemy && NPC->enemy->inuse )
			{
				checkEnt = NPC->enemy;
				VectorCopy( NPC->enemy->currentOrigin, center );
			}
			else if ( NPCInfo->blockedEntity && NPCInfo->blockedEntity->inuse )
			{
				checkEnt = NPCInfo->blockedEntity;
				//if it has an origin brush, use it...
				if ( VectorCompare( NPCInfo->blockedEntity->s.origin, vec3_origin ) )
				{//no origin brush, calc center
					VectorAdd( NPCInfo->blockedEntity->mins, NPCInfo->blockedEntity->maxs, center );
					VectorScale( center, 0.5f, center );
				}
				else
				{//use origin brush as center
					VectorCopy( NPCInfo->blockedEntity->s.origin, center );
				}
			}
			if ( checkEnt )
			{
				float zHeightRelative = center[2]-NPC->currentOrigin[2];
				if ( zHeightRelative >= (128.0f*NPC->s.modelScale[2]) )
				{
					grabAnim = BOTH_ATTACK11;
				}
				else if ( zHeightRelative >= (64.0f*NPC->s.modelScale[2]) )
				{
					grabAnim = BOTH_ATTACK10;
				}
			}
			NPC_SetAnim( NPC, SETANIM_BOTH, grabAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
			TIMER_Set( NPC, "attack_dmg", 800 );
			if ( NPC->enemy && NPC->enemy->s.number == 0 )
			{//don't attack the player again for a bit
				TIMER_Set( NPC, "attackDebounce", NPC->client->ps.legsAnimTimer + Q_irand( 2000, 4000+((2-g_spskill->integer)*2000) ) );
			}
			//init pos3 for the trace from last hand pos to current hand pos
			VectorCopy( NPC->currentOrigin, NPC->pos3 );
		}
		else
		{
			//FIXME: back up?
			ucmd.forwardmove = -64;
			//FIXME: check for walls/ledges?
			return;
		}

		TIMER_Set( NPC, "attacking", NPC->client->ps.legsAnimTimer + Q_flrand(0.0f, 1.0f) * 200 );
	}

	// Need to do delayed damage since the attack animations encapsulate multiple mini-attacks
	float playerDist;

	if ( TIMER_Done2( NPC, "attack_dmg", qtrue ) )
	{
		switch ( NPC->client->ps.legsAnim )
		{
		case BOTH_MELEE1:
			Rancor_Smash();
			playerDist = NPC_EntRangeFromBolt( player, NPC->handLBolt );
			if ( (NPC->spawnflags&SPF_RANCOR_MUTANT) )
			{
				if ( playerDist < 512 )
				{
					CGCam_Shake( 1.0f*playerDist/256, 1000 );
				}
			}
			else
			{
				if ( playerDist < 256 )
				{
					CGCam_Shake( 1.0f*playerDist/128.0f, 1000 );
				}
			}
			break;
		case BOTH_MELEE2:
			Rancor_Bite();
			TIMER_Set( NPC, "attack_dmg2", 450 );
			break;
		case BOTH_ATTACK1:
			if ( NPC->count == 1 && NPC->activator )
			{
				if ( (NPC->spawnflags&SPF_RANCOR_FASTKILL)
					&& NPC->activator->s.number >= MAX_CLIENTS )
				{
					G_Damage( NPC->activator, NPC, NPC, vec3_origin, NPC->activator->currentOrigin, NPC->activator->health+1000, (DAMAGE_NO_KNOCKBACK|DAMAGE_NO_PROTECTION), MOD_MELEE );
				}
				else if ( (NPC->spawnflags&SPF_RANCOR_MUTANT) )//FIXME: a flag or something would be better
				{//more damage
					G_Damage( NPC->activator, NPC, NPC, vec3_origin, NPC->activator->currentOrigin, Q_irand( 55, 70 ), DAMAGE_NO_KNOCKBACK, MOD_MELEE );
				}
				else
				{
					G_Damage( NPC->activator, NPC, NPC, vec3_origin, NPC->activator->currentOrigin, Q_irand( 25, 40 ), DAMAGE_NO_KNOCKBACK, MOD_MELEE );
				}
				if ( NPC->activator->health <= 0 )
				{//killed him
					if ( g_dismemberment->integer >= 3 )
					{//make it look like we bit his head off
						NPC->activator->client->dismembered = false;
						G_DoDismemberment( NPC->activator, NPC->activator->currentOrigin, MOD_SABER, 1000, HL_HEAD, qtrue );
					}
					NPC_SetAnim( NPC->activator, SETANIM_BOTH, BOTH_SWIM_IDLE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
				}
				G_Sound( NPC->activator, G_SoundIndex( "sound/chars/rancor/chomp.wav" ) );
			}
			break;
		case BOTH_ATTACK2:
		case BOTH_ATTACK10:
		case BOTH_ATTACK11:
			//try to grab
			Rancor_Swing( NPC->handRBolt, qtrue );
			break;
		case BOTH_ATTACK3:
			if ( NPC->count == 1 && NPC->activator )
			{
				//cut in half
				if ( NPC->activator->client )
				{
					NPC->activator->client->dismembered = false;
					G_DoDismemberment( NPC->activator, NPC->enemy->currentOrigin, MOD_SABER, 1000, HL_WAIST, qtrue );
				}
				//KILL
				G_Damage( NPC->activator, NPC, NPC, vec3_origin, NPC->activator->currentOrigin, NPC->enemy->health+1000, DAMAGE_NO_PROTECTION|DAMAGE_NO_ARMOR|DAMAGE_NO_KNOCKBACK|DAMAGE_NO_HIT_LOC, MOD_MELEE, HL_NONE );
				if ( NPC->activator->client )
				{
					NPC_SetAnim( NPC->activator, SETANIM_BOTH, BOTH_SWIM_IDLE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
				}
				TIMER_Set( NPC, "attack_dmg2", 1350 );
				G_Sound( NPC->activator, G_SoundIndex( "sound/chars/rancor/swipehit.wav" ) );
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
				G_Sound( NPC->activator, G_SoundIndex( "sound/chars/rancor/chomp.wav" ) );
				//FIXME: sometimes end up with a live one in our mouths?
				//just make sure they're dead
				if ( NPC->activator->health > 0 )
				{
					//cut in half
					NPC->activator->client->dismembered = false;
					G_DoDismemberment( NPC->activator, NPC->enemy->currentOrigin, MOD_SABER, 1000, HL_WAIST, qtrue );
					//KILL
					G_Damage( NPC->activator, NPC, NPC, vec3_origin, NPC->activator->currentOrigin, NPC->enemy->health+1000, DAMAGE_NO_PROTECTION|DAMAGE_NO_ARMOR|DAMAGE_NO_KNOCKBACK|DAMAGE_NO_HIT_LOC, MOD_MELEE, HL_NONE );
					NPC_SetAnim( NPC->activator, SETANIM_BOTH, BOTH_SWIM_IDLE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
					G_AddEvent( NPC->activator, EV_JUMP, NPC->activator->health );
				}
				NPC->count = 2;
				TIMER_Set( NPC, "clearGrabbed", 2600 );
			}
			break;
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
		NPCInfo->enemyLastSeenTime = level.time;
		if ( TIMER_Done2( NPC, "takingPain", qtrue ))
		{
			NPCInfo->localState = LSTATE_CLEAR;
		}
		else if ( (NPC->spawnflags&SPF_RANCOR_FASTKILL)
			&& NPC->activator
			&& NPC->activator->s.number >= MAX_CLIENTS )
		{
			Rancor_Attack( 0, qfalse, qfalse );
		}
		else if ( NPC->useDebounceTime >= level.time
			&& NPC->activator )
		{//just sniffing the guy
			if ( NPC->useDebounceTime <= level.time + 100
				&& NPC->client->ps.legsAnim != BOTH_HOLD_DROP)
			{//just about done, drop him
				NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_HOLD_DROP, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				TIMER_Set( NPC, "attacking", NPC->client->ps.legsAnimTimer+(Q_irand(500,1000)*(3-g_spskill->integer)) );
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
				Rancor_Attack( 0, qfalse, qfalse );
			}
		}
		NPC_UpdateAngles( qtrue, qtrue );
		return;
	}

	NPCInfo->goalRadius = NPC->maxs[0]+(MAX_DISTANCE*NPC->s.modelScale[0]);	// just get us within combat range

	// If we cannot see our target or we have somewhere to go, then do that
	if ( !NPC_ClearLOS( NPC->enemy ) || UpdateGoal( ))
	{
		NPCInfo->combatMove = qtrue;
		NPCInfo->goalEntity = NPC->enemy;

		Rancor_Move( qfalse );
		return;
	}

	NPCInfo->enemyLastSeenTime = level.time;
	// Sometimes I have problems with facing the enemy I'm attacking, so force the issue so I don't look dumb
	NPC_FaceEnemy( qtrue );

	float	distance	= Distance( NPC->currentOrigin, NPC->enemy->currentOrigin );

	qboolean	advance = (qboolean)( distance > (NPC->maxs[0]+(MIN_DISTANCE*NPC->s.modelScale[0])) ? qtrue : qfalse  );
	qboolean	doCharge = qfalse;

	if ( advance )
	{//have to get closer
		if ( (NPC->spawnflags&SPF_RANCOR_MUTANT)
			&& (!NPC->enemy||!NPC->enemy->client) )
		{//don't do breath attack vs. bbrushes
		}
		else
		{
			vec3_t	yawOnlyAngles = {0, NPC->currentAngles[YAW], 0};
			if ( NPC->enemy->health > 0
				&& fabs(distance-(250.0f*NPC->s.modelScale[0])) <= (80.0f*NPC->s.modelScale[0])
				&& InFOV( NPC->enemy->currentOrigin, NPC->currentOrigin, yawOnlyAngles, 30, 30 ) )
			{
				int chance = 9;
				if ( (NPC->spawnflags&SPF_RANCOR_MUTANT) )
				{//higher chance of doing breath attack
					chance = 5-g_spskill->integer;
				}
				if ( !Q_irand( 0, chance ) )
				{//go for the charge
					doCharge = qtrue;
					advance = qfalse;
				}
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
			Rancor_Move( qtrue );
		}
	}
	else
	{
		Rancor_Attack( distance, doCharge, qfalse );
	}
}

/*
-------------------------
NPC_Rancor_Pain
-------------------------
*/
void NPC_Rancor_Pain( gentity_t *self, gentity_t *inflictor, gentity_t *other, const vec3_t point, int damage, int mod,int hitLoc )
{
	qboolean hitByRancor = qfalse;

	if ( self->NPC && self->NPC->ignorePain )
	{
		return;
	}
	if ( !TIMER_Done( self, "breathAttack" ) )
	{//nothing interrupts breath attack
		return;
	}

	TIMER_Remove( self, "confusionTime" );

	if ( other&&other->client&&other->client->NPC_class==CLASS_RANCOR )
	{
		hitByRancor = qtrue;
	}
	if ( other
		&& other->inuse
		&& other != self->enemy
		&& !(other->flags&FL_NOTARGET) )
	{
		if ( !self->count )
		{
			if ( (!other->s.number&&!Q_irand(0,3))
				|| !self->enemy
				|| self->enemy->health == 0
				|| (self->enemy->client&&self->enemy->client->NPC_class == CLASS_RANCOR)
				|| (!Q_irand(0, 4 ) && DistanceSquared( other->currentOrigin, self->currentOrigin ) < DistanceSquared( self->enemy->currentOrigin, self->currentOrigin )) )
			{//if my enemy is dead (or attacked by player) and I'm not still holding/eating someone, turn on the attacker
				//FIXME: if can't nav to my enemy, take this guy if I can nav to him
				self->lastEnemy = self->enemy;
				G_SetEnemy( self, other );
				if ( self->enemy != self->lastEnemy )
				{//clear this so that we only sniff the player the first time we pick them up
					self->useDebounceTime = 0;
				}
				TIMER_Set( self, "lookForNewEnemy", Q_irand( 5000, 15000 ) );
				if ( hitByRancor )
				{//stay mad at this Rancor for 2-5 secs before looking for other enemies
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
				&& self->client->ps.legsAnim != BOTH_ATTACK2
				&& self->client->ps.legsAnim != BOTH_ATTACK10
				&& self->client->ps.legsAnim != BOTH_ATTACK11 )
			{//cant interrupt one of the big attack anims
				/*
				if ( self->count != 1
					|| other == self->activator
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
						TIMER_Set( self, "takingPain", self->client->ps.legsAnimTimer+Q_irand(0, 500*(2-g_spskill->integer)) );

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
	if ( (NPC->spawnflags&SPF_RANCOR_FASTKILL)
		&& NPC->activator->s.number >= MAX_CLIENTS )
	{
		return;
	}
	vec3_t mins={NPC->activator->mins[0]-1,NPC->activator->mins[1]-1,0};
	vec3_t maxs={NPC->activator->maxs[0]+1,NPC->activator->maxs[1]+1,1};
	vec3_t start={NPC->activator->currentOrigin[0],NPC->activator->currentOrigin[1],NPC->activator->absmin[2]};
	vec3_t end={NPC->activator->currentOrigin[0],NPC->activator->currentOrigin[1],NPC->activator->absmax[2]-1};
	trace_t	trace;
	gi.trace( &trace, start, mins, maxs, end, NPC->activator->s.number, NPC->activator->clipmask, (EG2_Collision)0, 0 );
	if ( !trace.allsolid && !trace.startsolid && trace.fraction >= 1.0f )
	{
		Rancor_DropVictim( NPC );
	}
}

qboolean Rancor_AttackBBrush( void )
{
	trace_t	trace;
	vec3_t center;
	vec3_t dir2Brush, end;
	float	checkDist = 64.0f;//32.0f;

	if ( VectorCompare( NPCInfo->blockedEntity->s.origin, vec3_origin ) )
	{//no origin brush, calc center
		VectorAdd( NPCInfo->blockedEntity->mins, NPCInfo->blockedEntity->maxs, center );
		VectorScale( center, 0.5f, center );
	}
	else
	{
		VectorCopy( NPCInfo->blockedEntity->s.origin, center );
	}
	if ( NAVDEBUG_showCollision )
	{
		CG_DrawEdge( NPC->currentOrigin, center, EDGE_IMPACT_POSSIBLE );
	}
	center[2] = NPC->currentOrigin[2];//we can't fly, so let's ignore z diff
	NPC_FacePosition( center, qfalse );
	//see if we're close to it
	VectorSubtract( center, NPC->currentOrigin, dir2Brush );
	float brushSize = ((NPCInfo->blockedEntity->maxs[0] - NPCInfo->blockedEntity->mins[0])*0.5f+(NPCInfo->blockedEntity->maxs[1] - NPCInfo->blockedEntity->mins[1])*0.5f) * 0.5f;
	float dist2Brush = VectorNormalize( dir2Brush )-(NPC->maxs[0])-brushSize;
	if ( dist2Brush < (MIN_DISTANCE*NPC->s.modelScale[0]) )
	{//close enough to just hit it
		trace.fraction = 0.0f;
		trace.entityNum = NPCInfo->blockedEntity->s.number;
	}
	else
	{
		VectorMA( NPC->currentOrigin, checkDist, dir2Brush, end );
		gi.trace( &trace, NPC->currentOrigin, NPC->mins, NPC->maxs, end, NPC->s.number, NPC->clipmask, (EG2_Collision)0, 0 );
		if ( trace.allsolid || trace.startsolid )
		{//wtf?
			NPCInfo->blockedEntity = NULL;
			return qfalse;
		}
	}
	if ( trace.fraction >= 1.0f //too far away
		|| trace.entityNum != NPCInfo->blockedEntity->s.number )//OR blocked by something else
	{//keep moving towards it
		ucmd.buttons &= ~BUTTON_WALKING;				// Unset from MoveToGoal()
		STEER::Activate(NPC);
		STEER::Seek(NPC, center);
		STEER::AvoidCollisions(NPC);
		STEER::DeActivate(NPC, &ucmd);
		/*
		VectorCopy( dir2Brush, NPC->client->ps.moveDir );
		if ( NPC->client->ps.speed < NPCInfo->stats.walkSpeed )
		{
			NPC->client->ps.speed = NPCInfo->stats.walkSpeed;
			ucmd.buttons |= BUTTON_WALKING;
		}
		*/
		//NPCInfo->enemyLastSeenTime = level.time;
		//let the function that called us know that we called NAV ourselves
	}
 	else if ( trace.entityNum == NPCInfo->blockedEntity->s.number )
	{//close enough, smash it!
		Rancor_Attack( (trace.fraction*checkDist), qfalse, qtrue );//FIXME: maybe charge at it, smash through?
		TIMER_Remove( NPC, "attackDebounce" );//don't wait on these
		NPCInfo->enemyLastSeenTime = level.time;
	}
	else
	{
		//Com_Printf( S_COLOR_RED"RANCOR cannot reach intended breakable %s, blocked by %s\n", NPC->blockedEntity->targetname, g_entities[trace.entityNum].classname );
		if ( G_EntIsBreakable( trace.entityNum, NPC ) )
		{//oh, well, smash that, then
			//G_SetEnemy( NPC, &g_entities[trace.entityNum] );
			gentity_t*	prevblockedEnt = NPCInfo->blockedEntity;
			NPCInfo->blockedEntity = &g_entities[trace.entityNum];
			Rancor_Attack( (trace.fraction*checkDist), qfalse, qtrue );//FIXME: maybe charge at it, smash through?
			TIMER_Remove( NPC, "attackDebounce" );//don't wait on these
			NPCInfo->enemyLastSeenTime = level.time;
			NPCInfo->blockedEntity = prevblockedEnt;
		}
		else
		{
			NPCInfo->blockedEntity = NULL;
			return qfalse;
		}
	}
	return qtrue;
}

void Rancor_FireBreathAttack( void )
{
	int		damage	= Q_irand( 10, 15 );
	trace_t		tr;
	gentity_t	*traceEnt = NULL;
	mdxaBone_t	boltMatrix;
	vec3_t		start, end, dir, traceMins = {-4, -4, -4}, traceMaxs = {4, 4, 4};
	vec3_t		rancAngles = {0,NPC->client->ps.viewangles[YAW],0};

	gi.G2API_GetBoltMatrix( NPC->ghoul2, NPC->playerModel, NPC->gutBolt,
			&boltMatrix, rancAngles, NPC->currentOrigin, (cg.time?cg.time:level.time),
			NULL, NPC->s.modelScale );

	gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, start );
	gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_Z, dir );
	VectorMA( start, 512, dir, end );

	gi.trace( &tr, start, traceMins, traceMaxs, end, NPC->s.number, MASK_SHOT, (EG2_Collision)0, 0 );

	traceEnt = &g_entities[tr.entityNum];
	if ( tr.entityNum < ENTITYNUM_WORLD
		&& traceEnt->takedamage
		&& traceEnt->client )
	{//breath attack only does damage to living things
		G_Damage( traceEnt, NPC, NPC, dir, tr.endpos, damage*2, DAMAGE_NO_ARMOR|DAMAGE_NO_KNOCKBACK|DAMAGE_NO_HIT_LOC|DAMAGE_IGNORE_TEAM, MOD_LAVA, HL_NONE );
	}
	if ( tr.fraction < 1.0f )
	{//hit something, do radius damage
		G_RadiusDamage( tr.endpos, NPC, damage, 250, NPC, MOD_LAVA );
	}
}

void Rancor_CheckAnimDamage( void )
{
	if ( NPC->client->ps.legsAnim == BOTH_ATTACK2
		|| NPC->client->ps.legsAnim == BOTH_ATTACK10
		|| NPC->client->ps.legsAnim == BOTH_ATTACK11 )
	{
		if ( NPC->client->ps.legsAnimTimer >= 1200 && NPC->client->ps.legsAnimTimer <= 1350 )
		{
			if ( Q_irand( 0, 2 ) )
			{
				Rancor_Swing( NPC->handRBolt, qfalse );
			}
			else
			{
				Rancor_Swing( NPC->handRBolt, qtrue );
			}
		}
		else if ( NPC->client->ps.legsAnimTimer >= 1100 && NPC->client->ps.legsAnimTimer <= 1550 )
		{
			Rancor_Swing( NPC->handRBolt, qtrue );
		}
	}
	else if ( NPC->client->ps.legsAnim == BOTH_ATTACK5 )
	{
		if ( NPC->client->ps.legsAnimTimer >= 750 && NPC->client->ps.legsAnimTimer <= 1300 )
		{
			Rancor_Swing( NPC->handLBolt, qfalse );
		}
		else if ( NPC->client->ps.legsAnimTimer >= 1700 && NPC->client->ps.legsAnimTimer <= 2300 )
		{
			Rancor_Swing( NPC->handRBolt, qfalse );
		}
	}
}
/*
-------------------------
NPC_BSRancor_Default
-------------------------
*/
void NPC_BSRancor_Default( void )
{
	AddSightEvent( NPC, NPC->currentOrigin, 1024, AEL_DANGER_GREAT, 50 );

	if (NPCInfo->blockedEntity && TIMER_Done(NPC, "blockedEntityIgnore"))
	{
		if (!TIMER_Exists(NPC, "blockedEntityTimeOut"))
		{
			TIMER_Set(NPC, "blockedEntityTimeOut", 5000);
		}
		else if (TIMER_Done(NPC, "blockedEntityTimeOut"))
		{
			TIMER_Remove(NPC, "blockedEntityTimeOut");
			TIMER_Set(NPC, "blockedEntityIgnore", 25000);
			NPCInfo->blockedEntity = NULL;
		}
	}
	else
	{
		TIMER_Remove(NPC, "blockedEntityTimeOut");
		TIMER_Remove(NPC, "blockedEntityIgnore");
	}

	Rancor_CheckAnimDamage();

	if ( !TIMER_Done( NPC, "breathAttack" ) )
	{//doing breath attack, just do damage
		Rancor_FireBreathAttack();
		NPC_UpdateAngles( qtrue, qtrue );
		return;
	}
	else if ( NPC->client->ps.legsAnim == BOTH_ATTACK4
		|| NPC->client->ps.legsAnim == BOTH_ATTACK6
		|| NPC->client->ps.legsAnim == BOTH_ATTACK7 )
	{
		G_StopEffect( G_EffectIndex( "mrancor/breath" ), NPC->playerModel, NPC->gutBolt, NPC->s.number );
		NPC->s.loopSound = 0;
	}

	if ( TIMER_Done2( NPC, "clearGrabbed", qtrue ) )
	{
		Rancor_DropVictim( NPC );
	}
	else if ( (NPC->client->ps.legsAnim == BOTH_PAIN2 || NPC->client->ps.legsAnim == BOTH_HOLD_DROP )
		&& NPC->count == 1
		&& NPC->activator )
	{
		Rancor_CheckDropVictim();
	}
	if ( !TIMER_Done( NPC, "rageTime" ) )
	{//do nothing but roar first time we see an enemy
		AddSoundEvent( NPC, NPC->currentOrigin, 1024, AEL_DANGER_GREAT, qfalse, qfalse );
		NPC_FaceEnemy( qtrue );
		return;
	}

	if ( NPCInfo->localState == LSTATE_WAITING
		&& TIMER_Done2( NPC, "takingPain", qtrue ) )
	{//was not doing anything because we were taking pain, but pain is done now, so clear it...
		NPCInfo->localState = LSTATE_CLEAR;
	}

	if ( !TIMER_Done( NPC, "confusionTime" ) )
	{
		NPC_UpdateAngles( qtrue, qtrue );
		return;
	}

	if ( NPC->enemy )
	{
		if ( NPC->enemy->client //enemy is a client
			&& (NPC->enemy->client->NPC_class == CLASS_UGNAUGHT || NPC->enemy->client->NPC_class == CLASS_JAWA )//enemy is a lowly jawa or ugnaught
			&& NPC->enemy->enemy != NPC//enemy's enemy is not me
			&& (!NPC->enemy->enemy || !NPC->enemy->enemy->client || NPC->enemy->enemy->client->NPC_class!=CLASS_RANCOR) )//enemy's enemy is not a client or is not a rancor (which is as scary as me anyway)
		{//they should be scared of ME and no-one else
			G_SetEnemy( NPC->enemy, NPC );
		}
		if ( TIMER_Done(NPC,"angrynoise") )
		{
			G_SoundOnEnt( NPC, CHAN_AUTO, va("sound/chars/rancor/anger%d.wav", Q_irand(1, 3)) );

			TIMER_Set( NPC, "angrynoise", Q_irand( 5000, 10000 ) );
		}
		else
		{
			AddSoundEvent( NPC, NPC->currentOrigin, 512, AEL_DANGER_GREAT, qfalse, qfalse );
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
			if ( NPCInfo->blockedEntity )
			{//something in our way
				if ( !NPCInfo->blockedEntity->inuse )
				{//was destroyed
					NPCInfo->blockedEntity = NULL;
				}
				else
				{
					//a breakable?
					if ( G_EntIsBreakable( NPCInfo->blockedEntity->s.number, NPC ) )
					{//breakable brush
						if ( !Rancor_AttackBBrush() )
						{//didn't move inside that func, so call move here...?
							Rancor_Move( qtrue );
						}
						NPC_UpdateAngles( qtrue, qtrue );
						return;
					}
					else
					{//if it's a client and in our way, get mad at it!
						if ( NPCInfo->blockedEntity != NPC->enemy
							&& NPCInfo->blockedEntity->client
							&& NPC_ValidEnemy( NPCInfo->blockedEntity )
							&& !Q_irand( 0, 9 ) )
						{
							G_SetEnemy( NPC, NPCInfo->blockedEntity );
							//look again in 2-5 secs
							TIMER_Set( NPC, "lookForNewEnemy", Q_irand( 2000, 5000 ) );
							NPCInfo->blockedEntity = NULL;
						}
					}
				}
			}
			if ( NPC_ValidEnemy( NPC->enemy ) == qfalse )
			{
				TIMER_Remove( NPC, "lookForNewEnemy" );//make them look again right now
				if ( !NPC->enemy->inuse
					|| level.time - NPC->enemy->s.time > Q_irand( 10000, 15000 )
					|| (NPC->spawnflags&SPF_RANCOR_FASTKILL) )//don't linger on dead bodies
				{//it's been a while since the enemy died, or enemy is completely gone, get bored with him
					if ( (NPC->spawnflags&SPF_RANCOR_MUTANT)
						&& player && player->health >= 0 )
					{//all else failing, always go after the player
						NPC->lastEnemy = NPC->enemy;
						G_SetEnemy( NPC, player );
						if ( NPC->enemy != NPC->lastEnemy )
						{//clear this so that we only sniff the player the first time we pick them up
							NPC->useDebounceTime = 0;
						}
					}
					else
					{
						NPC->enemy = NULL;
						Rancor_Patrol();
						NPC_UpdateAngles( qtrue, qtrue );
						return;
					}
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
		Rancor_Combat();
		if ( TIMER_Done( NPC, "attacking" )
			&& TIMER_Done( NPC, "takingpain" )
			&& TIMER_Done( NPC, "confusionDebounce" )
			&& NPCInfo->localState == LSTATE_CLEAR
			&& !NPC->count )
		{//not busy
			if ( !ucmd.forwardmove
				&& !ucmd.rightmove
				&& VectorCompare( NPC->client->ps.moveDir, vec3_origin ) )
			{//not moving
				if ( level.time - NPCInfo->enemyLastSeenTime > 5000 )
				{//haven't seen an enemy in a while
					if ( !Q_irand( 0, 20 ) )
					{
						if ( Q_irand( 0, 1 ) )
						{
							NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_GUARD_IDLE1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
						}
						else
						{
							NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_GUARD_LOOKAROUND1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
						}
						TIMER_Set( NPC, "confusionTime", NPC->client->ps.legsAnimTimer );
						TIMER_Set( NPC, "confusionDebounce", NPC->client->ps.legsAnimTimer+Q_irand( 4000, 8000 ) );
					}
				}
			}
		}
	}
	else
	{
		if ( TIMER_Done(NPC,"idlenoise") )
		{
			G_SoundOnEnt( NPC, CHAN_AUTO, va("sound/chars/rancor/snort_%d.wav", Q_irand(1, 4)) );

			TIMER_Set( NPC, "idlenoise", Q_irand( 2000, 4000 ) );
			AddSoundEvent( NPC, NPC->currentOrigin, 384, AEL_DANGER, qfalse, qfalse );
		}
		if ( NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES )
		{
			Rancor_Patrol();
			if ( !NPC->enemy && NPC->wait )
			{//we've been mad before and can't find an enemy
				if ( (NPC->spawnflags&SPF_RANCOR_MUTANT)
					&& player && player->health >= 0 )
				{//all else failing, always go after the player
					NPC->lastEnemy = NPC->enemy;
					G_SetEnemy( NPC, player );
					if ( NPC->enemy != NPC->lastEnemy )
					{//clear this so that we only sniff the player the first time we pick them up
						NPC->useDebounceTime = 0;
					}
				}
			}
		}
		else
		{
			Rancor_Idle();
		}
	}

	NPC_UpdateAngles( qtrue, qtrue );
}
