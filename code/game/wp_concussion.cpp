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

#include "g_local.h"
#include "b_local.h"
#include "g_functions.h"
#include "wp_saber.h"
#include "w_local.h"
#include "../cgame/cg_local.h"

static void WP_FireConcussionAlt( gentity_t *ent )
{//a rail-gun-like beam
	int			damage = weaponData[WP_CONCUSSION].altDamage, skip, traces = DISRUPTOR_ALT_TRACES;
	qboolean	render_impact = qtrue;
	vec3_t		start, end;
	vec3_t		muzzle2, spot, dir;
	trace_t		tr;
	gentity_t	*traceEnt, *tent;
	float		dist, shotDist, shotRange = 8192;
	qboolean	hitDodged = qfalse;

	if (ent->s.number >= MAX_CLIENTS)
	{
		vec3_t angles;
		vectoangles(forwardVec, angles);
		angles[PITCH] += ( crandom() * (CONC_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f));//was 0.5f
		angles[YAW]	  += ( crandom() * (CONC_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f));//was 0.5f
		AngleVectors(angles, forwardVec, vrightVec, up);
	}

	//Shove us backwards for half a second
	VectorMA( ent->client->ps.velocity, -200, forwardVec, ent->client->ps.velocity );
	ent->client->ps.groundEntityNum = ENTITYNUM_NONE;
	if ( (ent->client->ps.pm_flags&PMF_DUCKED) )
	{//hunkered down
		ent->client->ps.pm_time = 100;
	}
	else
	{
		ent->client->ps.pm_time = 250;
	}
	ent->client->ps.pm_flags |= PMF_TIME_KNOCKBACK|PMF_TIME_NOFRICTION;
	//FIXME: only if on ground?  So no "rocket jump"?  Or: (see next FIXME)
	//FIXME: instead, set a forced ucmd backmove instead of this sliding

	VectorCopy( muzzle, muzzle2 ); // making a backup copy

	// The trace start will originate at the eye so we can ensure that it hits the crosshair.
	if ( ent->NPC )
	{
		switch ( g_spskill->integer )
		{
		case 0:
			damage = CONC_ALT_NPC_DAMAGE_EASY;
			break;
		case 1:
			damage = CONC_ALT_NPC_DAMAGE_MEDIUM;
			break;
		case 2:
		default:
			damage = CONC_ALT_NPC_DAMAGE_HARD;
			break;
		}
	}
	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );

	skip = ent->s.number;

//	if ( ent->client && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > 0 && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > cg.time )
//	{
//		// in overcharge mode, so doing double damage
//		damage *= 2;
//	}
	
	//Make it a little easier to hit guys at long range
	vec3_t shot_mins, shot_maxs;
	VectorSet( shot_mins, -1, -1, -1 );
	VectorSet( shot_maxs, 1, 1, 1 );

	for ( int i = 0; i < traces; i++ )
	{
		VectorMA( start, shotRange, forwardVec, end );

		//NOTE: if you want to be able to hit guys in emplaced guns, use "G2_COLLIDE, 10" instead of "G2_RETURNONHIT, 0"
		//alternately, if you end up hitting an emplaced_gun that has a sitter, just redo this one trace with the "G2_COLLIDE, 10" to see if we it the sitter
		//gi.trace( &tr, start, NULL, NULL, end, skip, MASK_SHOT, G2_COLLIDE, 10 );//G2_RETURNONHIT, 0 );
		gi.trace( &tr, start, shot_mins, shot_maxs, end, skip, MASK_SHOT, G2_COLLIDE, 10 );//G2_RETURNONHIT, 0 );

		if ( tr.surfaceFlags & SURF_NOIMPACT ) 
		{
			render_impact = qfalse;
		}

		if ( tr.entityNum == ent->s.number )
		{
			// should never happen, but basically we don't want to consider a hit to ourselves?
			// Get ready for an attempt to trace through another person
			VectorCopy( tr.endpos, muzzle2 );
			VectorCopy( tr.endpos, start );
			skip = tr.entityNum;
#ifdef _DEBUG
			gi.Printf( "BAD! Concussion gun shot somehow traced back and hit the owner!\n" );			
#endif
			continue;
		}

		// always render a shot beam, doing this the old way because I don't much feel like overriding the effect.
		//NOTE: let's just draw one beam at the end
		//tent = G_TempEntity( tr.endpos, EV_CONC_ALT_SHOT );
		//tent->svFlags |= SVF_BROADCAST;

		//VectorCopy( muzzle2, tent->s.origin2 );

		if ( tr.fraction >= 1.0f )
		{
			// draw the beam but don't do anything else
			break;
		}

		traceEnt = &g_entities[tr.entityNum];

		if ( traceEnt //&& traceEnt->NPC 
			&& ( traceEnt->s.weapon == WP_SABER || (traceEnt->client && (traceEnt->client->NPC_class == CLASS_BOBAFETT||traceEnt->client->NPC_class == CLASS_REBORN) ) ) )
		{//FIXME: need a more reliable way to know we hit a jedi?
			hitDodged = Jedi_DodgeEvasion( traceEnt, ent, &tr, HL_NONE );
			//acts like we didn't even hit him
		}
		if ( !hitDodged )
		{
			if ( render_impact )
			{
				if (( tr.entityNum < ENTITYNUM_WORLD && traceEnt->takedamage ) 
					|| !Q_stricmp( traceEnt->classname, "misc_model_breakable" ) 
					|| traceEnt->s.eType == ET_MOVER )
				{
					// Create a simple impact type mark that doesn't last long in the world
					G_PlayEffect( G_EffectIndex( "concussion/alt_hit" ), tr.endpos, tr.plane.normal );

					if ( traceEnt->client && LogAccuracyHit( traceEnt, ent )) 
					{//NOTE: hitting multiple ents can still get you over 100% accuracy
						ent->client->ps.persistant[PERS_ACCURACY_HITS]++;
					} 

					int hitLoc = G_GetHitLocFromTrace( &tr, MOD_CONC_ALT );
					qboolean noKnockBack = (traceEnt->flags&FL_NO_KNOCKBACK);//will be set if they die, I want to know if it was on *before* they died
					if ( traceEnt && traceEnt->client && traceEnt->client->NPC_class == CLASS_GALAKMECH )
					{//hehe
						G_Damage( traceEnt, ent, ent, forwardVec, tr.endpos, 10, DAMAGE_NO_KNOCKBACK|DAMAGE_NO_HIT_LOC, MOD_CONC_ALT, hitLoc );
						break;
					}
					G_Damage( traceEnt, ent, ent, forwardVec, tr.endpos, damage, DAMAGE_NO_KNOCKBACK|DAMAGE_NO_HIT_LOC, MOD_CONC_ALT, hitLoc );

					//do knockback and knockdown manually
					if ( traceEnt->client )
					{//only if we hit a client
						vec3_t pushDir;
						VectorCopy( forwardVec, pushDir );
						if ( pushDir[2] < 0.2f )
						{
							pushDir[2] = 0.2f;
						}//hmm, re-normalize?  nah...
						//if ( traceEnt->NPC || Q_irand(0,g_spskill->integer+1) )
						{
							if ( !noKnockBack )
							{//knock-backable
								G_Throw( traceEnt, pushDir, 200 );
								if ( traceEnt->client->NPC_class == CLASS_ROCKETTROOPER )
								{
									traceEnt->client->ps.pm_time = Q_irand( 1500, 3000 );
								}
							}
							if ( traceEnt->health > 0 )
							{//alive
								if ( G_HasKnockdownAnims( traceEnt ) )
								{//knock-downable
									G_Knockdown( traceEnt, ent, pushDir, 400, qtrue );
								}
							}
						}
					}

					if ( traceEnt->s.eType == ET_MOVER )
					{//stop the traces on any mover
						break;
					}
				}
				else 
				{
					 // we only make this mark on things that can't break or move
					tent = G_TempEntity( tr.endpos, EV_CONC_ALT_MISS );
					tent->svFlags |= SVF_BROADCAST;
					VectorCopy( tr.plane.normal, tent->pos1 );
					break; // hit solid, but doesn't take damage, so stop the shot...we _could_ allow it to shoot through walls, might be cool?
				}
			}
			else // not rendering impact, must be a skybox or other similar thing?
			{
				break; // don't try anymore traces
			}
		}
		// Get ready for an attempt to trace through another person
		VectorCopy( tr.endpos, muzzle2 );
		VectorCopy( tr.endpos, start );
		skip = tr.entityNum;
		hitDodged = qfalse;
	}
	//just draw one beam all the way to the end
	tent = G_TempEntity( tr.endpos, EV_CONC_ALT_SHOT );
	tent->svFlags |= SVF_BROADCAST;
	VectorCopy( muzzle, tent->s.origin2 );

	// now go along the trail and make sight events
	VectorSubtract( tr.endpos, muzzle, dir );

	shotDist = VectorNormalize( dir );

	//FIXME: if shoot *really* close to someone, the alert could be way out of their FOV
	for ( dist = 0; dist < shotDist; dist += 64 )
	{
		//FIXME: on a really long shot, this could make a LOT of alerts in one frame...
		VectorMA( muzzle, dist, dir, spot );
		AddSightEvent( ent, spot, 256, AEL_DISCOVERED, 50 );
		//FIXME: creates *way* too many effects, make it one effect somehow?
		G_PlayEffect( G_EffectIndex( "concussion/alt_ring" ), spot, forwardVec );
	}
	//FIXME: spawn a temp ent that continuously spawns sight alerts here?  And 1 sound alert to draw their attention?
	VectorMA( start, shotDist-4, forwardVec, spot );
	AddSightEvent( ent, spot, 256, AEL_DISCOVERED, 50 );

	G_PlayEffect( G_EffectIndex( "concussion/altmuzzle_flash" ), muzzle, forwardVec );
}

static void WP_FireConcussion( gentity_t *ent )
{//a fast rocket-like projectile
	vec3_t	start;
	int		damage	= weaponData[WP_CONCUSSION].damage;
	float	vel = CONC_VELOCITY;

	if (ent->s.number >= MAX_CLIENTS)
	{
		vec3_t angles;
		vectoangles(forwardVec, angles);
		angles[PITCH] += ( crandom() * (CONC_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f));//was 0.5f
		angles[YAW]	  += ( crandom() * (CONC_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f));//was 0.5f
		AngleVectors(angles, forwardVec, vrightVec, up);
	}

	//hold us still for a bit
	ent->client->ps.pm_time = 300;
	ent->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	//add viewkick
	if ( ent->s.number < MAX_CLIENTS//player only
		&& !cg.renderingThirdPerson )//gives an advantage to being in 3rd person, but would look silly otherwise
	{//kick the view back
		cg.kick_angles[PITCH] = Q_flrand( -10, -15 );
		cg.kick_time = level.time;
	}

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	gentity_t *missile = CreateMissile( start, forwardVec, vel, 10000, ent, qfalse );

	missile->classname = "conc_proj";
	missile->s.weapon = WP_CONCUSSION;
	missile->mass = 10;

	// Do the damages
	if ( ent->s.number != 0 )
	{
		if ( g_spskill->integer == 0 )
		{
			damage = CONC_NPC_DAMAGE_EASY;
		}
		else if ( g_spskill->integer == 1 )
		{
			damage = CONC_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = CONC_NPC_DAMAGE_HARD;
		}
	}

	// Make it easier to hit things
	VectorSet( missile->maxs, ROCKET_SIZE, ROCKET_SIZE, ROCKET_SIZE );
	VectorScale( missile->maxs, -1, missile->mins );

	missile->damage = damage;
	missile->dflags = DAMAGE_EXTRA_KNOCKBACK;

	missile->methodOfDeath = MOD_CONC;
	missile->splashMethodOfDeath = MOD_CONC;

	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
	missile->splashDamage = weaponData[WP_CONCUSSION].splashDamage;
	missile->splashRadius = weaponData[WP_CONCUSSION].splashRadius;

	// we don't want it to ever bounce
	missile->bounceCount = 0;
}

void WP_Concussion( gentity_t *ent, qboolean alt_fire )
{
	if(alt_fire)
	{
		WP_FireConcussionAlt(ent);
	}
	else
	{
		WP_FireConcussion(ent);
	}
}