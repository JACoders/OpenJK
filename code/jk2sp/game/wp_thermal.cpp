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
#include "g_local.h"
#include "wp_saber.h"
#include "w_local.h"
#include "g_functions.h"

//---------------------
//	Thermal Detonator
//---------------------

//---------------------------------------------------------
void thermalDetonatorExplode( gentity_t *ent )
//---------------------------------------------------------
{
	if ( !ent->count )
	{
		G_Sound( ent, G_SoundIndex( "sound/weapons/thermal/warning.wav" ) );
		ent->count = 1;
		ent->nextthink = level.time + 800;
		ent->svFlags |= SVF_BROADCAST;//so everyone hears/sees the explosion?
	}
	else
	{
		vec3_t	pos;

		VectorSet( pos, ent->currentOrigin[0], ent->currentOrigin[1], ent->currentOrigin[2] + 8 );

		ent->takedamage = qfalse; // don't allow double deaths!

		G_RadiusDamage( ent->currentOrigin, ent->owner, weaponData[WP_THERMAL].splashDamage, weaponData[WP_THERMAL].splashRadius, NULL, MOD_EXPLOSIVE_SPLASH );

		G_PlayEffect( "thermal/explosion", ent->currentOrigin );
		G_PlayEffect( "thermal/shockwave", ent->currentOrigin );

		G_FreeEntity( ent );
	}
}

//-------------------------------------------------------------------------------------------------------------
void thermal_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod, int dFlags, int hitLoc )
//-------------------------------------------------------------------------------------------------------------
{
	thermalDetonatorExplode( self );
}

//---------------------------------------------------------
qboolean WP_LobFire( gentity_t *self, vec3_t start, vec3_t target, vec3_t mins, vec3_t maxs, int clipmask,
				vec3_t velocity, qboolean tracePath, int ignoreEntNum, int enemyNum,
				float minSpeed, float maxSpeed, float idealSpeed, qboolean mustHit )
//---------------------------------------------------------
{
	float	targetDist, shotSpeed, speedInc = 100, travelTime, impactDist, bestImpactDist = Q3_INFINITE;//fireSpeed,
	vec3_t	targetDir, shotVel, failCase = { 0.0f };
	trace_t	trace;
	trajectory_t	tr;
	qboolean	blocked;
	int		elapsedTime, skipNum, timeStep = 500, hitCount = 0, maxHits = 7;
	vec3_t	lastPos, testPos;
	gentity_t	*traceEnt;

	if ( !idealSpeed )
	{
		idealSpeed = 300;
	}
	else if ( idealSpeed < speedInc )
	{
		idealSpeed = speedInc;
	}
	shotSpeed = idealSpeed;
	skipNum = (idealSpeed-speedInc)/speedInc;
	if ( !minSpeed )
	{
		minSpeed = 100;
	}
	if ( !maxSpeed )
	{
		maxSpeed = 900;
	}
	while ( hitCount < maxHits )
	{
		VectorSubtract( target, start, targetDir );
		targetDist = VectorNormalize( targetDir );

		VectorScale( targetDir, shotSpeed, shotVel );
		travelTime = targetDist/shotSpeed;
		shotVel[2] += travelTime * 0.5 * g_gravity->value;

		if ( !hitCount )
		{//save the first (ideal) one as the failCase (fallback value)
			if ( !mustHit )
			{//default is fine as a return value
				VectorCopy( shotVel, failCase );
			}
		}

		if ( tracePath )
		{//do a rough trace of the path
			blocked = qfalse;

			VectorCopy( start, tr.trBase );
			VectorCopy( shotVel, tr.trDelta );
			tr.trType = TR_GRAVITY;
			tr.trTime = level.time;
			travelTime *= 1000.0f;
			VectorCopy( start, lastPos );

			//This may be kind of wasteful, especially on long throws... use larger steps?  Divide the travelTime into a certain hard number of slices?  Trace just to apex and down?
			for ( elapsedTime = timeStep; elapsedTime < floor(travelTime)+timeStep; elapsedTime += timeStep )
			{
				if ( (float)elapsedTime > travelTime )
				{//cap it
					elapsedTime = floor( travelTime );
				}
				EvaluateTrajectory( &tr, level.time + elapsedTime, testPos );
				gi.trace( &trace, lastPos, mins, maxs, testPos, ignoreEntNum, clipmask, G2_NOCOLLIDE, 0 );

				if ( trace.allsolid || trace.startsolid )
				{
					blocked = qtrue;
					break;
				}
				if ( trace.fraction < 1.0f )
				{//hit something
					if ( trace.entityNum == enemyNum )
					{//hit the enemy, that's perfect!
						break;
					}
					else if ( trace.plane.normal[2] > 0.7 && DistanceSquared( trace.endpos, target ) < 4096 )//hit within 64 of desired location, should be okay
					{//close enough!
						break;
					}
					else
					{//FIXME: maybe find the extents of this brush and go above or below it on next try somehow?
						impactDist = DistanceSquared( trace.endpos, target );
						if ( impactDist < bestImpactDist )
						{
							bestImpactDist = impactDist;
							VectorCopy( shotVel, failCase );
						}
						blocked = qtrue;
						//see if we should store this as the failCase
						if ( trace.entityNum < ENTITYNUM_WORLD )
						{//hit an ent
							traceEnt = &g_entities[trace.entityNum];
							if ( traceEnt && traceEnt->takedamage && !OnSameTeam( self, traceEnt ) )
							{//hit something breakable, so that's okay
								//we haven't found a clear shot yet so use this as the failcase
								VectorCopy( shotVel, failCase );
							}
						}
						break;
					}
				}
				if ( elapsedTime == floor( travelTime ) )
				{//reached end, all clear
					break;
				}
				else
				{
					//all clear, try next slice
					VectorCopy( testPos, lastPos );
				}
			}
			if ( blocked )
			{//hit something, adjust speed (which will change arc)
				hitCount++;
				shotSpeed = idealSpeed + ((hitCount-skipNum) * speedInc);//from min to max (skipping ideal)
				if ( hitCount >= skipNum )
				{//skip ideal since that was the first value we tested
					shotSpeed += speedInc;
				}
			}
			else
			{//made it!
				break;
			}
		}
		else
		{//no need to check the path, go with first calc
			break;
		}
	}

	if ( hitCount >= maxHits )
	{//NOTE: worst case scenario, use the one that impacted closest to the target (or just use the first try...?)
		VectorCopy( failCase, velocity );
		return qfalse;
	}
	VectorCopy( shotVel, velocity );
	return qtrue;
}

//---------------------------------------------------------
void WP_ThermalThink( gentity_t *ent )
//---------------------------------------------------------
{
	int			count;
	qboolean	blow = qfalse;

	// Thermal detonators for the player do occasional radius checks and blow up if there are entities in the blast radius
	//	This is done so that the main fire is actually useful as an attack.  We explode anyway after delay expires.
	if ( ent->delay > level.time )
	{
		//	Finally, we force it to bounce at least once before doing the special checks, otherwise it's just too easy for the player?
		if ( ent->has_bounced )
		{
			count = G_RadiusList( ent->currentOrigin, TD_TEST_RAD, ent, qtrue, ent_list );

			for ( int i = 0; i < count; i++ )
			{
				if ( ent_list[i]->s.number == 0 )
				{
					// avoid deliberately blowing up next to the player, no matter how close any enemy is..
					//	...if the delay time expires though, there is no saving the player...muwhaaa haa ha
					blow = qfalse;
					break;
				}
				else if ( ent_list[i]->client && ent_list[i]->health > 0 )
				{
					// sometimes the ent_list order changes, so we should make sure that the player isn't anywhere in this list
					blow = qtrue;
				}
			}
		}
	}
	else
	{
		// our death time has arrived, even if nothing is near us
		blow = qtrue;
	}

	if ( blow )
	{
		ent->e_ThinkFunc = thinkF_thermalDetonatorExplode;
		ent->nextthink = level.time + 50;
	}
	else
	{
		// we probably don't need to do this thinking logic very often...maybe this is fast enough?
		ent->nextthink = level.time + TD_THINK_TIME;
	}
}

//---------------------------------------------------------
gentity_t *WP_FireThermalDetonator( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	gentity_t	*bolt;
	vec3_t		dir, start;
	float		damageScale = 1.0f;

	VectorCopy( wpFwd, dir );
	VectorCopy( wpMuzzle, start );

	bolt = G_Spawn();

	bolt->classname = "thermal_detonator";

	if ( ent->s.number != 0 )
	{
		// If not the player, cut the damage a bit so we don't get pounded on so much
		damageScale = TD_NPC_DAMAGE_CUT;
	}

	if ( !alt_fire && ent->s.number == 0 )
	{
		// Main fires for the players do a little bit of extra thinking
		bolt->e_ThinkFunc = thinkF_WP_ThermalThink;
		bolt->nextthink = level.time + TD_THINK_TIME;
		bolt->delay = level.time + TD_TIME; // How long 'til she blows
	}
	else
	{
		bolt->e_ThinkFunc = thinkF_thermalDetonatorExplode;
		bolt->nextthink = level.time + TD_TIME; // How long 'til she blows
	}

	bolt->mass = 10;

	// How 'bout we give this thing a size...
	VectorSet( bolt->mins, -4.0f, -4.0f, -4.0f );
	VectorSet( bolt->maxs, 4.0f, 4.0f, 4.0f );
	bolt->clipmask = MASK_SHOT;
	bolt->clipmask &= ~CONTENTS_CORPSE;
	bolt->contents = CONTENTS_SHOTCLIP;
	bolt->takedamage = qtrue;
	bolt->health = 15;
	bolt->e_DieFunc = dieF_thermal_die;

	WP_TraceSetStart( ent, start, bolt->mins, bolt->maxs );//make sure our start point isn't on the other side of a wall

	float chargeAmount = 1.0f; // default of full charge

	if ( ent->client )
	{
		chargeAmount = level.time - ent->client->ps.weaponChargeTime;
	}

	// get charge amount
	chargeAmount = chargeAmount / (float)TD_VELOCITY;

	if ( chargeAmount > 1.0f )
	{
		chargeAmount = 1.0f;
	}
	else if ( chargeAmount < TD_MIN_CHARGE )
	{
		chargeAmount = TD_MIN_CHARGE;
	}

	// normal ones bounce, alt ones explode on impact
	bolt->s.pos.trType = TR_GRAVITY;
	bolt->owner = ent;
	VectorScale( dir, TD_VELOCITY * chargeAmount, bolt->s.pos.trDelta );

	if ( ent->health > 0 )
	{
		bolt->s.pos.trDelta[2] += 120;

		if ( ent->NPC && ent->enemy )
		{//FIXME: we're assuming he's actually facing this direction...
			vec3_t	target;

			VectorCopy( ent->enemy->currentOrigin, target );
			if ( target[2] <= start[2] )
			{
				vec3_t	vec;
				VectorSubtract( target, start, vec );
				VectorNormalize( vec );
				VectorMA( target, Q_flrand( 0, -32 ), vec, target );//throw a little short
			}

			target[0] += Q_flrand( -5, 5 )+(Q_flrand(-1.0f, 1.0f)*(6-ent->NPC->currentAim)*2);
			target[1] += Q_flrand( -5, 5 )+(Q_flrand(-1.0f, 1.0f)*(6-ent->NPC->currentAim)*2);
			target[2] += Q_flrand( -5, 5 )+(Q_flrand(-1.0f, 1.0f)*(6-ent->NPC->currentAim)*2);

			WP_LobFire( ent, start, target, bolt->mins, bolt->maxs, bolt->clipmask, bolt->s.pos.trDelta, qtrue, ent->s.number, ent->enemy->s.number );
		}
	}

	if ( alt_fire )
	{
		bolt->alt_fire = qtrue;
	}
	else
	{
		bolt->s.eFlags |= EF_BOUNCE_HALF;
	}

	bolt->s.loopSound = G_SoundIndex( "sound/weapons/thermal/thermloop.wav" );

	bolt->damage = weaponData[WP_THERMAL].damage * damageScale;
	bolt->dflags = 0;
	bolt->splashDamage = weaponData[WP_THERMAL].splashDamage * damageScale;
	bolt->splashRadius = weaponData[WP_THERMAL].splashRadius;

	bolt->s.eType = ET_MISSILE;
	bolt->svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_THERMAL;

	if ( alt_fire )
	{
		bolt->methodOfDeath = MOD_THERMAL_ALT;
		bolt->splashMethodOfDeath = MOD_THERMAL_ALT;//? SPLASH;
	}
	else
	{
		bolt->methodOfDeath = MOD_THERMAL;
		bolt->splashMethodOfDeath = MOD_THERMAL;//? SPLASH;
	}

	bolt->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );

	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, bolt->currentOrigin);

	VectorCopy( start, bolt->pos2 );

	return bolt;
}

//---------------------------------------------------------
gentity_t *WP_DropThermal( gentity_t *ent )
//---------------------------------------------------------
{
	AngleVectors( ent->client->ps.viewangles, wpFwd, wpVright, wpUp );
	CalcEntitySpot( ent, SPOT_WEAPON, wpMuzzle );
	return (WP_FireThermalDetonator( ent, qfalse ));
}