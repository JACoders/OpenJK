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
#include "g_nav.h"

extern void Boba_FireDecide( void );

void Seeker_Strafe( void );

#define VELOCITY_DECAY		0.7f

#define	MIN_MELEE_RANGE		320
#define	MIN_MELEE_RANGE_SQR	( MIN_MELEE_RANGE * MIN_MELEE_RANGE )

#define MIN_DISTANCE		80
#define MIN_DISTANCE_SQR	( MIN_DISTANCE * MIN_DISTANCE )

#define SEEKER_STRAFE_VEL	100
#define SEEKER_STRAFE_DIS	200
#define SEEKER_UPWARD_PUSH	32

#define SEEKER_FORWARD_BASE_SPEED	10
#define SEEKER_FORWARD_MULTIPLIER	2

#define SEEKER_SEEK_RADIUS			1024

//------------------------------------
void NPC_Seeker_Precache(void)
{
	G_SoundIndex("sound/chars/seeker/misc/fire.wav");
	G_SoundIndex( "sound/chars/seeker/misc/hiss.wav");
	G_EffectIndex( "env/small_explode");
}

//------------------------------------
void NPC_Seeker_Pain(gentity_t *self, gentity_t *attacker, int damage)
{
	if ( !(self->NPC->aiFlags&NPCAI_CUSTOM_GRAVITY ))
	{
		G_Damage( self, NULL, NULL, (float*)vec3_origin, (float*)vec3_origin, 999, 0, MOD_FALLING );
	}

	SaveNPCGlobals();
	SetNPCGlobals( self );
	Seeker_Strafe();
	RestoreNPCGlobals();
	NPC_Pain( self, attacker, damage );
}

//------------------------------------
void Seeker_MaintainHeight( void )
{
	float	dif;

	// Update our angles regardless
	NPC_UpdateAngles( qtrue, qtrue );

	// If we have an enemy, we should try to hover at or a little below enemy eye level
	if ( NPCS.NPC->enemy )
	{
		if (TIMER_Done( NPCS.NPC, "heightChange" ))
		{
			float difFactor;

			TIMER_Set( NPCS.NPC,"heightChange",Q_irand( 1000, 3000 ));

			// Find the height difference
			dif = (NPCS.NPC->enemy->r.currentOrigin[2] +  flrand( NPCS.NPC->enemy->r.maxs[2]/2, NPCS.NPC->enemy->r.maxs[2]+8 )) - NPCS.NPC->r.currentOrigin[2];

			difFactor = 1.0f;
			if ( NPCS.NPC->client->NPC_class == CLASS_BOBAFETT )
			{
				if ( TIMER_Done( NPCS.NPC, "flameTime" ) )
				{
					difFactor = 10.0f;
				}
			}

			// cap to prevent dramatic height shifts
			if ( fabs( dif ) > 2*difFactor )
			{
				if ( fabs( dif ) > 24*difFactor )
				{
					dif = ( dif < 0 ? -24*difFactor : 24*difFactor );
				}

				NPCS.NPC->client->ps.velocity[2] = (NPCS.NPC->client->ps.velocity[2]+dif)/2;
			}
			if ( NPCS.NPC->client->NPC_class == CLASS_BOBAFETT )
			{
				NPCS.NPC->client->ps.velocity[2] *= flrand( 0.85f, 3.0f );
			}
		}
	}
	else
	{
		gentity_t *goal = NULL;

		if ( NPCS.NPCInfo->goalEntity )	// Is there a goal?
		{
			goal = NPCS.NPCInfo->goalEntity;
		}
		else
		{
			goal = NPCS.NPCInfo->lastGoalEntity;
		}
		if ( goal )
		{
			dif = goal->r.currentOrigin[2] - NPCS.NPC->r.currentOrigin[2];

			if ( fabs( dif ) > 24 )
			{
				NPCS.ucmd.upmove = ( NPCS.ucmd.upmove < 0 ? -4 : 4 );
			}
			else
			{
				if ( NPCS.NPC->client->ps.velocity[2] )
				{
					NPCS.NPC->client->ps.velocity[2] *= VELOCITY_DECAY;

					if ( fabs( NPCS.NPC->client->ps.velocity[2] ) < 2 )
					{
						NPCS.NPC->client->ps.velocity[2] = 0;
					}
				}
			}
		}
	}

	// Apply friction
	if ( NPCS.NPC->client->ps.velocity[0] )
	{
		NPCS.NPC->client->ps.velocity[0] *= VELOCITY_DECAY;

		if ( fabs( NPCS.NPC->client->ps.velocity[0] ) < 1 )
		{
			NPCS.NPC->client->ps.velocity[0] = 0;
		}
	}

	if ( NPCS.NPC->client->ps.velocity[1] )
	{
		NPCS.NPC->client->ps.velocity[1] *= VELOCITY_DECAY;

		if ( fabs( NPCS.NPC->client->ps.velocity[1] ) < 1 )
		{
			NPCS.NPC->client->ps.velocity[1] = 0;
		}
	}
}

//------------------------------------
void Seeker_Strafe( void )
{
	int		side;
	vec3_t	end, right, dir;
	trace_t	tr;

	if ( Q_flrand(0.0f, 1.0f) > 0.7f || !NPCS.NPC->enemy || !NPCS.NPC->enemy->client )
	{
		// Do a regular style strafe
		AngleVectors( NPCS.NPC->client->renderInfo.eyeAngles, NULL, right, NULL );

		// Pick a random strafe direction, then check to see if doing a strafe would be
		//	reasonably valid
		side = ( rand() & 1 ) ? -1 : 1;
		VectorMA( NPCS.NPC->r.currentOrigin, SEEKER_STRAFE_DIS * side, right, end );

		trap->Trace( &tr, NPCS.NPC->r.currentOrigin, NULL, NULL, end, NPCS.NPC->s.number, MASK_SOLID, qfalse, 0, 0 );

		// Close enough
		if ( tr.fraction > 0.9f )
		{
			float vel = SEEKER_STRAFE_VEL;
			float upPush = SEEKER_UPWARD_PUSH;
			if ( NPCS.NPC->client->NPC_class != CLASS_BOBAFETT )
			{
				G_Sound( NPCS.NPC, CHAN_AUTO, G_SoundIndex( "sound/chars/seeker/misc/hiss" ));
			}
			else
			{
				vel *= 3.0f;
				upPush *= 4.0f;
			}
			VectorMA( NPCS.NPC->client->ps.velocity, vel*side, right, NPCS.NPC->client->ps.velocity );
			// Add a slight upward push
			NPCS.NPC->client->ps.velocity[2] += upPush;

			NPCS.NPCInfo->standTime = level.time + 1000 + Q_flrand(0.0f, 1.0f) * 500;
		}
	}
	else
	{
		float stDis;

		// Do a strafe to try and keep on the side of their enemy
		AngleVectors( NPCS.NPC->enemy->client->renderInfo.eyeAngles, dir, right, NULL );

		// Pick a random side
		side = ( rand() & 1 ) ? -1 : 1;
		stDis = SEEKER_STRAFE_DIS;
		if ( NPCS.NPC->client->NPC_class == CLASS_BOBAFETT )
		{
			stDis *= 2.0f;
		}
		VectorMA( NPCS.NPC->enemy->r.currentOrigin, stDis * side, right, end );

		// then add a very small bit of random in front of/behind the player action
		VectorMA( end, Q_flrand(-1.0f, 1.0f) * 25, dir, end );

		trap->Trace( &tr, NPCS.NPC->r.currentOrigin, NULL, NULL, end, NPCS.NPC->s.number, MASK_SOLID, qfalse, 0, 0 );

		// Close enough
		if ( tr.fraction > 0.9f )
		{
			float dis, upPush;

			VectorSubtract( tr.endpos, NPCS.NPC->r.currentOrigin, dir );
			dir[2] *= 0.25; // do less upward change
			dis = VectorNormalize( dir );

			// Try to move the desired enemy side
			VectorMA( NPCS.NPC->client->ps.velocity, dis, dir, NPCS.NPC->client->ps.velocity );

			upPush = SEEKER_UPWARD_PUSH;
			if ( NPCS.NPC->client->NPC_class != CLASS_BOBAFETT )
			{
				G_Sound( NPCS.NPC, CHAN_AUTO, G_SoundIndex( "sound/chars/seeker/misc/hiss" ));
			}
			else
			{
				upPush *= 4.0f;
			}

			// Add a slight upward push
			NPCS.NPC->client->ps.velocity[2] += upPush;

			NPCS.NPCInfo->standTime = level.time + 2500 + Q_flrand(0.0f, 1.0f) * 500;
		}
	}
}

//------------------------------------
void Seeker_Hunt( qboolean visible, qboolean advance )
{
	float	distance, speed;
	vec3_t	forward;

	NPC_FaceEnemy( qtrue );

	// If we're not supposed to stand still, pursue the player
	if ( NPCS.NPCInfo->standTime < level.time )
	{
		// Only strafe when we can see the player
		if ( visible )
		{
			Seeker_Strafe();
			return;
		}
	}

	// If we don't want to advance, stop here
	if ( advance == qfalse )
	{
		return;
	}

	// Only try and navigate if the player is visible
	if ( visible == qfalse )
	{
		// Move towards our goal
		NPCS.NPCInfo->goalEntity = NPCS.NPC->enemy;
		NPCS.NPCInfo->goalRadius = 24;

		// Get our direction from the navigator if we can't see our target
		if ( NPC_GetMoveDirection( forward, &distance ) == qfalse )
		{
			return;
		}
	}
	else
	{
		VectorSubtract( NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin, forward );
		/*distance = */VectorNormalize( forward );
	}

	speed = SEEKER_FORWARD_BASE_SPEED + SEEKER_FORWARD_MULTIPLIER * g_npcspskill.integer;
	VectorMA( NPCS.NPC->client->ps.velocity, speed, forward, NPCS.NPC->client->ps.velocity );
}

//------------------------------------
void Seeker_Fire( void )
{
	vec3_t		dir, enemy_org, muzzle;
	gentity_t	*missile;

	CalcEntitySpot( NPCS.NPC->enemy, SPOT_HEAD, enemy_org );
	VectorSubtract( enemy_org, NPCS.NPC->r.currentOrigin, dir );
	VectorNormalize( dir );

	// move a bit forward in the direction we shall shoot in so that the bolt doesn't poke out the other side of the seeker
	VectorMA( NPCS.NPC->r.currentOrigin, 15, dir, muzzle );

	missile = CreateMissile( muzzle, dir, 1000, 10000, NPCS.NPC, qfalse );

	G_PlayEffectID( G_EffectIndex("blaster/muzzle_flash"), NPCS.NPC->r.currentOrigin, dir );

	missile->classname = "blaster";
	missile->s.weapon = WP_BLASTER;

	missile->damage = 5;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_BLASTER;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
	if ( NPCS.NPC->r.ownerNum < ENTITYNUM_NONE )
	{
		missile->r.ownerNum = NPCS.NPC->r.ownerNum;
	}
}

//------------------------------------
void Seeker_Ranged( qboolean visible, qboolean advance )
{
	if ( NPCS.NPC->client->NPC_class != CLASS_BOBAFETT )
	{
		if ( NPCS.NPC->count > 0 )
		{
			if ( TIMER_Done( NPCS.NPC, "attackDelay" ))	// Attack?
			{
				TIMER_Set( NPCS.NPC, "attackDelay", Q_irand( 250, 2500 ));
				Seeker_Fire();
				NPCS.NPC->count--;
			}
		}
		else
		{
			// out of ammo, so let it die...give it a push up so it can fall more and blow up on impact
	//		NPC->client->ps.gravity = 900;
	//		NPC->svFlags &= ~SVF_CUSTOM_GRAVITY;
	//		NPC->client->ps.velocity[2] += 16;
			G_Damage( NPCS.NPC, NPCS.NPC, NPCS.NPC, NULL, NULL, 999, 0, MOD_UNKNOWN );
		}
	}

	if ( NPCS.NPCInfo->scriptFlags & SCF_CHASE_ENEMIES )
	{
		Seeker_Hunt( visible, advance );
	}
}

//------------------------------------
void Seeker_Attack( void )
{
	float		distance;
	qboolean	visible, advance;

	// Always keep a good height off the ground
	Seeker_MaintainHeight();

	// Rate our distance to the target, and our visibilty
	distance	= DistanceHorizontalSquared( NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin );
	visible		= NPC_ClearLOS4( NPCS.NPC->enemy );
	advance		= (qboolean)(distance > MIN_DISTANCE_SQR);

	if ( NPCS.NPC->client->NPC_class == CLASS_BOBAFETT )
	{
		advance = (qboolean)(distance>(200.0f*200.0f));
	}

	// If we cannot see our target, move to see it
	if ( visible == qfalse )
	{
		if ( NPCS.NPCInfo->scriptFlags & SCF_CHASE_ENEMIES )
		{
			Seeker_Hunt( visible, advance );
			return;
		}
	}

	Seeker_Ranged( visible, advance );
}

//------------------------------------
void Seeker_FindEnemy( void )
{
	int			numFound;
	float		dis, bestDis = SEEKER_SEEK_RADIUS * SEEKER_SEEK_RADIUS + 1;
	vec3_t		mins, maxs;
	int			entityList[MAX_GENTITIES];
	gentity_t	*ent, *best = NULL;
	int			i;

	VectorSet( maxs, SEEKER_SEEK_RADIUS, SEEKER_SEEK_RADIUS, SEEKER_SEEK_RADIUS );
	VectorScale( maxs, -1, mins );

	numFound = trap->EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( i = 0 ; i < numFound ; i++ )
	{
		ent = &g_entities[entityList[i]];

		if ( ent->s.number == NPCS.NPC->s.number
			|| !ent->client //&& || !ent->NPC
			|| ent->health <= 0
			|| !ent->inuse )
		{
			continue;
		}

		if ( ent->client->playerTeam == NPCS.NPC->client->playerTeam || ent->client->playerTeam == NPCTEAM_NEUTRAL ) // don't attack same team or bots
		{
			continue;
		}

		// try to find the closest visible one
		if ( !NPC_ClearLOS4( ent ))
		{
			continue;
		}

		dis = DistanceHorizontalSquared( NPCS.NPC->r.currentOrigin, ent->r.currentOrigin );

		if ( dis <= bestDis )
		{
			bestDis = dis;
			best = ent;
		}
	}

	if ( best )
	{
		// used to offset seekers around a circle so they don't occupy the same spot.  This is not a fool-proof method.
		NPCS.NPC->random = Q_flrand(0.0f, 1.0f) * 6.3f; // roughly 2pi

		NPCS.NPC->enemy = best;
	}
}

//------------------------------------
void Seeker_FollowOwner( void )
{
	float	dis, minDistSqr;
	vec3_t	pt, dir;
	gentity_t	*owner = &g_entities[NPCS.NPC->s.owner];

	Seeker_MaintainHeight();

	if ( NPCS.NPC->client->NPC_class == CLASS_BOBAFETT )
	{
		owner = NPCS.NPC->enemy;
	}
	if ( !owner || owner == NPCS.NPC || !owner->client )
	{
		return;
	}
	//rwwFIXMEFIXME: Care about all clients not just 0
	dis	= DistanceHorizontalSquared( NPCS.NPC->r.currentOrigin, owner->r.currentOrigin );

	minDistSqr = MIN_DISTANCE_SQR;

	if ( NPCS.NPC->client->NPC_class == CLASS_BOBAFETT )
	{
		if ( TIMER_Done( NPCS.NPC, "flameTime" ) )
		{
			minDistSqr = 200*200;
		}
	}

	if ( dis < minDistSqr )
	{
		// generally circle the player closely till we take an enemy..this is our target point
		if ( NPCS.NPC->client->NPC_class == CLASS_BOBAFETT )
		{
			pt[0] = owner->r.currentOrigin[0] + cos( level.time * 0.001f + NPCS.NPC->random ) * 250;
			pt[1] = owner->r.currentOrigin[1] + sin( level.time * 0.001f + NPCS.NPC->random ) * 250;
			if ( NPCS.NPC->client->jetPackTime < level.time )
			{
				pt[2] = NPCS.NPC->r.currentOrigin[2] - 64;
			}
			else
			{
				pt[2] = owner->r.currentOrigin[2] + 200;
			}
		}
		else
		{
			pt[0] = owner->r.currentOrigin[0] + cos( level.time * 0.001f + NPCS.NPC->random ) * 56;
			pt[1] = owner->r.currentOrigin[1] + sin( level.time * 0.001f + NPCS.NPC->random ) * 56;
			pt[2] = owner->r.currentOrigin[2] + 40;
		}

		VectorSubtract( pt, NPCS.NPC->r.currentOrigin, dir );
		VectorMA( NPCS.NPC->client->ps.velocity, 0.8f, dir, NPCS.NPC->client->ps.velocity );
	}
	else
	{
		if ( NPCS.NPC->client->NPC_class != CLASS_BOBAFETT )
		{
			if ( TIMER_Done( NPCS.NPC, "seekerhiss" ))
			{
				TIMER_Set( NPCS.NPC, "seekerhiss", 1000 + Q_flrand(0.0f, 1.0f) * 1000 );
				G_Sound( NPCS.NPC, CHAN_AUTO, G_SoundIndex( "sound/chars/seeker/misc/hiss" ));
			}
		}

		// Hey come back!
		NPCS.NPCInfo->goalEntity = owner;
		NPCS.NPCInfo->goalRadius = 32;
		NPC_MoveToGoal( qtrue );
		NPCS.NPC->parent = owner;
	}

	if ( NPCS.NPCInfo->enemyCheckDebounceTime < level.time )
	{
		// check twice a second to find a new enemy
		Seeker_FindEnemy();
		NPCS.NPCInfo->enemyCheckDebounceTime = level.time + 500;
	}

	NPC_UpdateAngles( qtrue, qtrue );
}

//------------------------------------
void NPC_BSSeeker_Default( void )
{
	/*
	if ( in_camera )
	{
		if ( NPC->client->NPC_class != CLASS_BOBAFETT )
		{
			// cameras make me commit suicide....
			G_Damage( NPC, NPC, NPC, NULL, NULL, 999, 0, MOD_UNKNOWN );
		}
	}
	*/
	//N/A for MP.
	if ( NPCS.NPC->r.ownerNum < ENTITYNUM_NONE )
	{
		//OJKFIXME: clientnum 0
		gentity_t *owner = &g_entities[0];
		if ( owner->health <= 0
			|| (owner->client && owner->client->pers.connected == CON_DISCONNECTED) )
		{//owner is dead or gone
			//remove me
			G_Damage( NPCS.NPC, NULL, NULL, NULL, NULL, 10000, DAMAGE_NO_PROTECTION, MOD_TELEFRAG );
			return;
		}
	}

	if ( NPCS.NPC->random == 0.0f )
	{
		// used to offset seekers around a circle so they don't occupy the same spot.  This is not a fool-proof method.
		NPCS.NPC->random = Q_flrand(0.0f, 1.0f) * 6.3f; // roughly 2pi
	}

	if ( NPCS.NPC->enemy && NPCS.NPC->enemy->health && NPCS.NPC->enemy->inuse )
	{
		//OJKFIXME: clientnum 0
		if ( NPCS.NPC->client->NPC_class != CLASS_BOBAFETT && ( NPCS.NPC->enemy->s.number == 0 || ( NPCS.NPC->enemy->client && NPCS.NPC->enemy->client->NPC_class == CLASS_SEEKER )) )
		{
			//hacked to never take the player as an enemy, even if the player shoots at it
			NPCS.NPC->enemy = NULL;
		}
		else
		{
			Seeker_Attack();
			if ( NPCS.NPC->client->NPC_class == CLASS_BOBAFETT )
			{
				Boba_FireDecide();
			}
			return;
		}
	}

	// In all other cases, follow the player and look for enemies to take on
	Seeker_FollowOwner();
}
