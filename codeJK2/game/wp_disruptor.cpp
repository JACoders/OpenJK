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
//	Tenloss Disruptor
//---------------------
extern qboolean G_GetHitLocFromSurfName( gentity_t *ent, const char *surfName, int *hitLoc, vec3_t point, vec3_t dir, vec3_t bladeDir, int mod );
int G_GetHitLocFromTrace( trace_t *trace, int mod )
{
	int hitLoc = HL_NONE;
	for (int i=0; i < MAX_G2_COLLISIONS; i++)
	{
		if ( trace->G2CollisionMap[i].mEntityNum == -1 )
		{
			break;
		}

		CCollisionRecord &coll = trace->G2CollisionMap[i];
		if ( (coll.mFlags & G2_FRONTFACE) )
		{
			G_GetHitLocFromSurfName( &g_entities[coll.mEntityNum], gi.G2API_GetSurfaceName( &g_entities[coll.mEntityNum].ghoul2[coll.mModelIndex], coll.mSurfaceIndex ), &hitLoc, coll.mCollisionPosition, NULL, NULL, mod );
			//we only want the first "entrance wound", so break
			break;
		}
	}
	return hitLoc;
}

//---------------------------------------------------------
static void WP_DisruptorMainFire( gentity_t *ent )
//---------------------------------------------------------
{
	int			damage = weaponData[WP_DISRUPTOR].damage;
	qboolean	render_impact = qtrue;
	vec3_t		start, end, spot;
	trace_t		tr;
	gentity_t	*traceEnt = NULL, *tent;
	float		dist, shotDist, shotRange = 8192;

	if ( ent->NPC )
	{
		switch ( g_spskill->integer )
		{
		case 0:
			damage = DISRUPTOR_NPC_MAIN_DAMAGE_EASY;
			break;
		case 1:
			damage = DISRUPTOR_NPC_MAIN_DAMAGE_MEDIUM;
			break;
		case 2:
		default:
			damage = DISRUPTOR_NPC_MAIN_DAMAGE_HARD;
			break;
		}
	}

	VectorCopy( wpMuzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );

//	if ( ent->client && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > 0 && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > cg.time )
//	{
//		// in overcharge mode, so doing double damage
//		damage *= 2;
//	}

	VectorMA( start, shotRange, wpFwd, end );

	int ignore = ent->s.number;
	int traces = 0;
	while ( traces < 10 )
	{//need to loop this in case we hit a Jedi who dodges the shot
		gi.trace( &tr, start, NULL, NULL, end, ignore, MASK_SHOT, G2_RETURNONHIT, 0 );

		traceEnt = &g_entities[tr.entityNum];
		if ( traceEnt && traceEnt->s.weapon == WP_SABER )//&& traceEnt->NPC
		{//FIXME: need a more reliable way to know we hit a jedi?
			if ( Jedi_DodgeEvasion( traceEnt, ent, &tr, HL_NONE ) )
			{//act like we didn't even hit him
				VectorCopy( tr.endpos, start );
				ignore = tr.entityNum;
				traces++;
				continue;
			}
		}
		//a Jedi is not dodging this shot
		break;
	}

	if ( tr.surfaceFlags & SURF_NOIMPACT )
	{
		render_impact = qfalse;
	}

	// always render a shot beam, doing this the old way because I don't much feel like overriding the effect.
	tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_MAIN_SHOT );
	tent->svFlags |= SVF_BROADCAST;
	VectorCopy( wpMuzzle, tent->s.origin2 );

	if ( render_impact )
	{
		if ( tr.entityNum < ENTITYNUM_WORLD && traceEnt->takedamage )
		{
			// Create a simple impact type mark that doesn't last long in the world
			G_PlayEffect( G_EffectIndex( "disruptor/flesh_impact" ), tr.endpos, tr.plane.normal );

			if ( traceEnt->client && LogAccuracyHit( traceEnt, ent ))
			{
				ent->client->ps.persistant[PERS_ACCURACY_HITS]++;
			}

			int hitLoc = G_GetHitLocFromTrace( &tr, MOD_DISRUPTOR );
			if ( traceEnt && traceEnt->client && traceEnt->client->NPC_class == CLASS_GALAKMECH )
			{//hehe
				G_Damage( traceEnt, ent, ent, wpFwd, tr.endpos, 3, DAMAGE_DEATH_KNOCKBACK, MOD_DISRUPTOR, hitLoc );
			}
			else
			{
				G_Damage( traceEnt, ent, ent, wpFwd, tr.endpos, damage, DAMAGE_DEATH_KNOCKBACK, MOD_DISRUPTOR, hitLoc );
			}
		}
		else
		{
			G_PlayEffect( G_EffectIndex( "disruptor/wall_impact" ), tr.endpos, tr.plane.normal );
		}
	}

	shotDist = shotRange * tr.fraction;

	for ( dist = 0; dist < shotDist; dist += 64 )
	{
		//FIXME: on a really long shot, this could make a LOT of alerts in one frame...
		VectorMA( start, dist, wpFwd, spot );
		AddSightEvent( ent, spot, 256, AEL_DISCOVERED, 50 );
	}
	VectorMA( start, shotDist-4, wpFwd, spot );
	AddSightEvent( ent, spot, 256, AEL_DISCOVERED, 50 );
}

//---------------------------------------------------------
void WP_DisruptorAltFire( gentity_t *ent )
//---------------------------------------------------------
{
	int			damage = weaponData[WP_DISRUPTOR].altDamage, skip, traces = DISRUPTOR_ALT_TRACES;
	qboolean	render_impact = qtrue;
	vec3_t		start, end;
	vec3_t		muzzle2, spot, dir;
	trace_t		tr;
	gentity_t	*traceEnt, *tent;
	float		dist, shotDist, shotRange = 8192;
	qboolean	hitDodged = qfalse, fullCharge = qfalse;

	VectorCopy( wpMuzzle, muzzle2 ); // making a backup copy

	// The trace start will originate at the eye so we can ensure that it hits the crosshair.
	if ( ent->NPC )
	{
		switch ( g_spskill->integer )
		{
		case 0:
			damage = DISRUPTOR_NPC_ALT_DAMAGE_EASY;
			break;
		case 1:
			damage = DISRUPTOR_NPC_ALT_DAMAGE_MEDIUM;
			break;
		case 2:
		default:
			damage = DISRUPTOR_NPC_ALT_DAMAGE_HARD;
			break;
		}
		VectorCopy( wpMuzzle, start );

		fullCharge = qtrue;
	}
	else
	{
		VectorCopy( ent->client->renderInfo.eyePoint, start );
		AngleVectors( ent->client->renderInfo.eyeAngles, wpFwd, NULL, NULL );

		// don't let NPC's do charging
		int count = ( level.time - ent->client->ps.weaponChargeTime - 50 ) / DISRUPTOR_CHARGE_UNIT;

		if ( count < 1 )
		{
			count = 1;
		}
		else if ( count >= 10 )
		{
			count = 10;
			fullCharge = qtrue;
		}

		// more powerful charges go through more things
		if ( count < 3 )
		{
			traces = 1;
		}
		else if ( count < 6 )
		{
			traces = 2;
		}
		//else do full traces

		damage = damage * count + weaponData[WP_DISRUPTOR].damage * 0.5f; // give a boost to low charge shots
	}

	skip = ent->s.number;

//	if ( ent->client && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > 0 && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > cg.time )
//	{
//		// in overcharge mode, so doing double damage
//		damage *= 2;
//	}

	for ( int i = 0; i < traces; i++ )
	{
		VectorMA( start, shotRange, wpFwd, end );

		//NOTE: if you want to be able to hit guys in emplaced guns, use "G2_COLLIDE, 10" instead of "G2_RETURNONHIT, 0"
		//alternately, if you end up hitting an emplaced_gun that has a sitter, just redo this one trace with the "G2_COLLIDE, 10" to see if we it the sitter
		gi.trace( &tr, start, NULL, NULL, end, skip, MASK_SHOT, G2_COLLIDE, 10 );//G2_RETURNONHIT, 0 );

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
			gi.Printf( "BAD! Disruptor gun shot somehow traced back and hit the owner!\n" );
#endif
			continue;
		}

		// always render a shot beam, doing this the old way because I don't much feel like overriding the effect.
		tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_SNIPER_SHOT );
		tent->svFlags |= SVF_BROADCAST;
		tent->alt_fire = fullCharge; // mark us so we can alter the effect

		VectorCopy( muzzle2, tent->s.origin2 );

		if ( tr.fraction >= 1.0f )
		{
			// draw the beam but don't do anything else
			break;
		}

		traceEnt = &g_entities[tr.entityNum];

		if ( traceEnt && traceEnt->s.weapon == WP_SABER )//&& traceEnt->NPC
		{//FIXME: need a more reliable way to know we hit a jedi?
			hitDodged = Jedi_DodgeEvasion( traceEnt, ent, &tr, HL_NONE );
			//acts like we didn't even hit him
		}
		if ( !hitDodged )
		{
			if ( render_impact )
			{
				if (( tr.entityNum < ENTITYNUM_WORLD && traceEnt->takedamage ) || !Q_stricmp( traceEnt->classname, "misc_model_breakable" )
					|| traceEnt->s.eType == ET_MOVER )
				{
					// Create a simple impact type mark that doesn't last long in the world
					G_PlayEffect( G_EffectIndex( "disruptor/alt_hit" ), tr.endpos, tr.plane.normal );

					if ( traceEnt->client && LogAccuracyHit( traceEnt, ent ))
					{//NOTE: hitting multiple ents can still get you over 100% accuracy
						ent->client->ps.persistant[PERS_ACCURACY_HITS]++;
					}

					int hitLoc = G_GetHitLocFromTrace( &tr, MOD_DISRUPTOR );
					if ( traceEnt && traceEnt->client && traceEnt->client->NPC_class == CLASS_GALAKMECH )
					{//hehe
						G_Damage( traceEnt, ent, ent, wpFwd, tr.endpos, 10, DAMAGE_NO_KNOCKBACK|DAMAGE_NO_HIT_LOC, fullCharge ? MOD_SNIPER : MOD_DISRUPTOR, hitLoc );
						break;
					}
					G_Damage( traceEnt, ent, ent, wpFwd, tr.endpos, damage, DAMAGE_NO_KNOCKBACK|DAMAGE_NO_HIT_LOC, fullCharge ? MOD_SNIPER : MOD_DISRUPTOR, hitLoc );
				}
				else
				{
					 // we only make this mark on things that can't break or move
					tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_SNIPER_MISS );
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

	// now go along the trail and make sight events
	VectorSubtract( tr.endpos, wpMuzzle, dir );

	shotDist = VectorNormalize( dir );

	//FIXME: if shoot *really* close to someone, the alert could be way out of their FOV
	for ( dist = 0; dist < shotDist; dist += 64 )
	{
		//FIXME: on a really long shot, this could make a LOT of alerts in one frame...
		VectorMA( wpMuzzle, dist, dir, spot );
		AddSightEvent( ent, spot, 256, AEL_DISCOVERED, 50 );
	}
	//FIXME: spawn a temp ent that continuously spawns sight alerts here?  And 1 sound alert to draw their attention?
	VectorMA( start, shotDist-4, wpFwd, spot );
	AddSightEvent( ent, spot, 256, AEL_DISCOVERED, 50 );
}

//---------------------------------------------------------
void WP_FireDisruptor( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	if ( alt_fire )
	{
		WP_DisruptorAltFire( ent );
	}
	else
	{
		WP_DisruptorMainFire( ent );
	}

	G_PlayEffect( G_EffectIndex( "disruptor/line_cap" ), wpMuzzle, wpFwd );
}