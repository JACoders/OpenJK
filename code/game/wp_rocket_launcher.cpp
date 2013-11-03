/*
This file is part of OpenJK.

    OpenJK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    OpenJK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenJK.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2013 OpenJK

#include "g_local.h"
#include "b_local.h"
#include "g_functions.h"
#include "wp_saber.h"
#include "w_local.h"

//-----------------------
//	Rocket Launcher
//-----------------------

//---------------------------------------------------------
void rocketThink( gentity_t *ent )
//---------------------------------------------------------
{
	vec3_t newdir, targetdir, 
			up={0,0,1}, right; 
	vec3_t	org;
	float dot, dot2;

	if ( ent->disconnectDebounceTime && ent->disconnectDebounceTime < level.time )
	{//time's up, we're done, remove us
		if ( ent->lockCount )
		{//explode when die
			WP_ExplosiveDie( ent, ent->owner, ent->owner, 0, MOD_UNKNOWN, 0, HL_NONE );
		}
		else
		{//just remove when die
			G_FreeEntity( ent );
		}
		return;
	}
	if ( ent->enemy && ent->enemy->inuse )
	{
		float vel = (ent->spawnflags&1)?ent->speed:ROCKET_VELOCITY;
		float newDirMult = ent->angle?ent->angle*2.0f:1.0f;
		float oldDirMult = ent->angle?(1.0f-ent->angle)*2.0f:1.0f;

		if ( (ent->spawnflags&1) )
		{//vehicle rocket
			if ( ent->enemy->client && ent->enemy->client->NPC_class == CLASS_VEHICLE )
			{//tracking another vehicle
				if ( ent->enemy->client->ps.speed+ent->speed > vel )
				{
					vel = ent->enemy->client->ps.speed+ent->speed;
				}
			}
		}

		VectorCopy( ent->enemy->currentOrigin, org );
		org[2] += (ent->enemy->mins[2] + ent->enemy->maxs[2]) * 0.5f;

		if ( ent->enemy->client )
		{
			switch( ent->enemy->client->NPC_class )
			{
			case CLASS_ATST:
				org[2] += 80;
				break;
			case CLASS_MARK1:
				org[2] += 40;
				break;
			case CLASS_PROBE:
				org[2] += 60;
				break;
			default:
				break;
			}
			if ( !TIMER_Done( ent->enemy, "flee" ) )
			{
				TIMER_Set( ent->enemy, "rocketChasing", 500 );
			}
		}

		VectorSubtract( org, ent->currentOrigin, targetdir );
		VectorNormalize( targetdir );

		// Now the rocket can't do a 180 in space, so we'll limit the turn to about 45 degrees.
		dot = DotProduct( targetdir, ent->movedir );

		// a dot of 1.0 means right-on-target.
		if ( dot < 0.0f )
		{	
			// Go in the direction opposite, start a 180.
			CrossProduct( ent->movedir, up, right );
			dot2 = DotProduct( targetdir, right );

			if ( dot2 > 0 )
			{	
				// Turn 45 degrees right.
				VectorMA( ent->movedir, 0.3f*newDirMult, right, newdir );
			}
			else
			{	
				// Turn 45 degrees left.
				VectorMA(ent->movedir, -0.3f*newDirMult, right, newdir);
			}

			// Yeah we've adjusted horizontally, but let's split the difference vertically, so we kinda try to move towards it.
			newdir[2] = ( (targetdir[2]*newDirMult) + (ent->movedir[2]*oldDirMult) ) * 0.5;

			// slowing down coupled with fairly tight turns can lead us to orbit an enemy..looks bad so don't do it!
//			vel *= 0.5f;
		}
		else if ( dot < 0.70f )
		{	
			// Still a bit off, so we turn a bit softer
			VectorMA( ent->movedir, 0.5f*newDirMult, targetdir, newdir );
		}
		else
		{	
			// getting close, so turn a bit harder
			VectorMA( ent->movedir, 0.9f*newDirMult, targetdir, newdir );
		}

		// add crazy drunkenness
		for ( int i = 0; i < 3; i++ )
		{
			newdir[i] += crandom() * ent->random * 0.25f;
		}

		// decay the randomness
		ent->random *= 0.9f;

		if ( ent->enemy->client
			&& ent->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE )
		{//tracking a client who's on the ground, aim at the floor...?
			// Try to crash into the ground if we get close enough to do splash damage
			float dis = Distance( ent->currentOrigin, org );

			if ( dis < 128 )
			{
				// the closer we get, the more we push the rocket down, heh heh.
				newdir[2] -= (1.0f - (dis / 128.0f)) * 0.6f;
			}
		}

		VectorNormalize( newdir );

		VectorScale( newdir, vel * 0.5f, ent->s.pos.trDelta );
		VectorCopy( newdir, ent->movedir );
		SnapVector( ent->s.pos.trDelta );			// save net bandwidth
		VectorCopy( ent->currentOrigin, ent->s.pos.trBase );
		ent->s.pos.trTime = level.time;
	}

	ent->nextthink = level.time + ROCKET_ALT_THINK_TIME;	// Nothing at all spectacular happened, continue.
	return;
}

//---------------------------------------------------------
void WP_FireRocket( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	vec3_t	start;
	int		damage	= weaponData[WP_ROCKET_LAUNCHER].damage;
	float	vel = ROCKET_VELOCITY;

	if ( alt_fire )
	{
		vel *= 0.5f;
	}

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	gentity_t *missile = CreateMissile( start, forwardVec, vel, 10000, ent, alt_fire );

	missile->classname = "rocket_proj";
	missile->s.weapon = WP_ROCKET_LAUNCHER;
	missile->mass = 10;

	// Do the damages
	if ( ent->s.number != 0 )
	{
		if ( g_spskill->integer == 0 )
		{
			damage = ROCKET_NPC_DAMAGE_EASY;
		}
		else if ( g_spskill->integer == 1 )
		{
			damage = ROCKET_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = ROCKET_NPC_DAMAGE_HARD;
		}
		if (ent->client && ent->client->NPC_class==CLASS_BOBAFETT)
		{
			damage = damage/2;
		}
	}

	if ( alt_fire )
	{
		int	lockEntNum, lockTime;
		if ( ent->NPC && ent->enemy )
		{
			lockEntNum = ent->enemy->s.number;
			lockTime = Q_irand( 600, 1200 );
		}
		else
		{
			lockEntNum = g_rocketLockEntNum;
			lockTime = g_rocketLockTime;
		}
		// we'll consider attempting to lock this little poochie onto some baddie.
		if ( (lockEntNum > 0 || (ent->NPC && lockEntNum >= 0)) && lockEntNum < ENTITYNUM_WORLD && lockTime > 0 )
		{
			// take our current lock time and divide that by 8 wedge slices to get the current lock amount
			int dif = ( level.time - lockTime ) / ( 1200.0f / 8.0f );

			if ( dif < 0 )
			{
				dif = 0;
			}
			else if ( dif > 8 )
			{
				dif = 8;
			}

			// if we are fully locked, always take on the enemy.  
			//	Also give a slight advantage to higher, but not quite full charges.  
			//	Finally, just give any amount of charge a very slight random chance of locking.
			if ( dif == 8 || random() * dif > 2 || random() > 0.97f )
			{
				missile->enemy = &g_entities[lockEntNum];

				if ( missile->enemy 
					&& missile->enemy->inuse )//&& DistanceSquared( missile->currentOrigin, missile->enemy->currentOrigin ) < 262144 && InFOV( missile->currentOrigin, missile->enemy->currentOrigin, missile->enemy->client->ps.viewangles, 45, 45 ) )
				{
					if ( missile->enemy->client
						&& (missile->enemy->client->ps.forcePowersKnown&(1<<FP_PUSH))
						&& missile->enemy->client->ps.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_0 )
					{//have force push, don't flee from homing rockets
					}
					else
					{
						vec3_t dir, dir2;

						AngleVectors( missile->enemy->currentAngles, dir, NULL, NULL );
						AngleVectors( ent->client->renderInfo.eyeAngles, dir2, NULL, NULL );

						if ( DotProduct( dir, dir2 ) < 0.0f )
						{
							G_StartFlee( missile->enemy, ent, missile->enemy->currentOrigin, AEL_DANGER_GREAT, 3000, 5000 );
							if ( !TIMER_Done( missile->enemy, "flee" ) )
							{
								TIMER_Set( missile->enemy, "rocketChasing", 500 );
							}
						}
					}
				}
			}
		}

		VectorCopy( forwardVec, missile->movedir );

		missile->e_ThinkFunc = thinkF_rocketThink;
		missile->random = 1.0f;
		missile->nextthink = level.time + ROCKET_ALT_THINK_TIME;
	}

	// Make it easier to hit things
	VectorSet( missile->maxs, ROCKET_SIZE, ROCKET_SIZE, ROCKET_SIZE );
	VectorScale( missile->maxs, -1, missile->mins );

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;

	if ( alt_fire )
	{
		missile->methodOfDeath = MOD_ROCKET_ALT;
		missile->splashMethodOfDeath = MOD_ROCKET_ALT;// ?SPLASH;
	}
	else
	{
		missile->methodOfDeath = MOD_ROCKET;
		missile->splashMethodOfDeath = MOD_ROCKET;// ?SPLASH;
	}

	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
	missile->splashDamage = weaponData[WP_ROCKET_LAUNCHER].splashDamage;
	missile->splashRadius = weaponData[WP_ROCKET_LAUNCHER].splashRadius;

	// we don't want it to ever bounce
	missile->bounceCount = 0;
}
