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

//
// NPC.cpp - generic functions
//

#include "g_headers.h"

#include "b_local.h"
#include "anims.h"
#include "g_functions.h"
#include "say.h"
#include "Q3_Interface.h"

extern vec3_t playerMins;
extern vec3_t playerMaxs;
//extern void PM_SetAnimFinal(int *torsoAnim,int *legsAnim,int type,int anim,int priority,int *torsoAnimTimer,int *legsAnimTimer,gentity_t *gent);
extern void G_SoundOnEnt( gentity_t *ent, soundChannel_t channel, const char *soundPath );
extern void PM_SetTorsoAnimTimer( gentity_t *ent, int *torsoAnimTimer, int time );
extern void PM_SetLegsAnimTimer( gentity_t *ent, int *legsAnimTimer, int time );
extern void NPC_BSNoClip ( void );
extern void G_AddVoiceEvent( gentity_t *self, int event, int speakDebounceTime );
extern void NPC_ApplyRoff (void);
extern void NPC_TempLookTarget ( gentity_t *self, int lookEntNum, int minLookTime, int maxLookTime );
extern void NPC_CheckPlayerAim ( void );
extern void NPC_CheckAllClear ( void );
extern void G_AddVoiceEvent( gentity_t *self, int event, int speakDebounceTime );
extern qboolean NPC_CheckLookTarget( gentity_t *self );
extern void NPC_SetLookTarget( gentity_t *self, int entNum, int clearTime );
extern void Mark1_dying( gentity_t *self );
extern void NPC_BSCinematic( void );
extern int GetTime ( int lastTime );
extern void NPC_BSGM_Default( void );
extern void NPC_CheckCharmed( void );

extern cvar_t	*g_dismemberment;
extern cvar_t	*g_saberRealisticCombat;

//Local Variables
// ai debug cvars
cvar_t		*debugNPCAI;			// used to print out debug info about the bot AI
cvar_t		*debugNPCFreeze;		// set to disable bot ai and temporarily freeze them in place
cvar_t		*debugNPCAimingBeam;
cvar_t		*debugBreak;
cvar_t		*debugNoRoam;
cvar_t		*debugNPCName;
cvar_t		*d_saberCombat;
cvar_t		*d_JediAI;
cvar_t		*d_noGroupAI;
cvar_t		*d_asynchronousGroupAI;
cvar_t		*d_altRoutes;
cvar_t		*d_patched;
cvar_t		*d_slowmodeath;

extern qboolean	stop_icarus;

gentity_t		*NPC;
gNPC_t			*NPCInfo;
gclient_t		*client;
usercmd_t		ucmd;
visibility_t	enemyVisibility;

void NPC_SetAnim(gentity_t	*ent,int type,int anim,int priority);
void pitch_roll_for_slope( gentity_t *forwhom, vec3_t pass_slope );
extern void GM_Dying( gentity_t *self );

extern int eventClearTime;

void CorpsePhysics( gentity_t *self )
{
	// run the bot through the server like it was a real client
	memset( &ucmd, 0, sizeof( ucmd ) );
	ClientThink( self->s.number, &ucmd );
	VectorCopy( self->s.origin, self->s.origin2 );

	if ( self->client->NPC_class == CLASS_GALAKMECH )
	{
		GM_Dying( self );
	}
	//FIXME: match my pitch and roll for the slope of my groundPlane
	if ( self->client->ps.groundEntityNum != ENTITYNUM_NONE && !(self->flags&FL_DISINTEGRATED) )
	{//on the ground
		//FIXME: check 4 corners
		pitch_roll_for_slope( self, NULL );
	}

	if ( eventClearTime == level.time + ALERT_CLEAR_TIME )
	{//events were just cleared out so add me again
		if ( !(self->client->ps.eFlags&EF_NODRAW) )
		{
			AddSightEvent( self->enemy, self->currentOrigin, 384, AEL_DISCOVERED );
		}
	}

	if ( level.time - self->s.time > 3000 )
	{//been dead for 3 seconds
		if ( g_dismemberment->integer < 11381138 && !g_saberRealisticCombat->integer )
		{//can't be dismembered once dead
			if ( self->client->NPC_class != CLASS_PROTOCOL )
			{
				self->client->dismembered = qtrue;
			}
		}
	}

	if ( level.time - self->s.time > 500 )
	{//don't turn "nonsolid" until about 1 second after actual death

		if ((self->client->NPC_class != CLASS_MARK1) && (self->client->NPC_class != CLASS_INTERROGATOR))	// The Mark1 & Interrogator stays solid.
		{
			self->contents = CONTENTS_CORPSE;
		}

		if ( self->message )
		{
			self->contents |= CONTENTS_TRIGGER;
		}
	}
}

/*
----------------------------------------
NPC_RemoveBody

Determines when it's ok to ditch the corpse
----------------------------------------
*/
#define REMOVE_DISTANCE		128
#define REMOVE_DISTANCE_SQR (REMOVE_DISTANCE * REMOVE_DISTANCE)

void NPC_RemoveBody( gentity_t *self )
{
	CorpsePhysics( self );

	self->nextthink = level.time + FRAMETIME;

	if ( self->NPC->nextBStateThink <= level.time )
	{
		if( self->taskManager && !stop_icarus )
		{
			self->taskManager->Update( );
		}
	}
	self->NPC->nextBStateThink = level.time + FRAMETIME;

	if ( self->message )
	{//I still have a key
		return;
	}

	// I don't consider this a hack, it's creative coding . . .
	// I agree, very creative... need something like this for ATST and GALAKMECH too!
	if (self->client->NPC_class == CLASS_MARK1)
	{
		Mark1_dying( self );
	}

	// Since these blow up, remove the bounding box.
	if ( self->client->NPC_class == CLASS_REMOTE
		|| self->client->NPC_class == CLASS_SENTRY
		|| self->client->NPC_class == CLASS_PROBE
		|| self->client->NPC_class == CLASS_INTERROGATOR
		|| self->client->NPC_class == CLASS_PROBE
		|| self->client->NPC_class == CLASS_MARK2 )
	{
		if ( !self->taskManager || !self->taskManager->IsRunning() )
		{
			G_FreeEntity( self );
		}
		return;
	}

	//FIXME: don't ever inflate back up?
	self->maxs[2] = self->client->renderInfo.eyePoint[2] - self->currentOrigin[2] + 4;
	if ( self->maxs[2] < -8 )
	{
		self->maxs[2] = -8;
	}

	if ( self->client->NPC_class == CLASS_GALAKMECH )
	{//never disappears
		return;
	}
	if ( self->NPC && self->NPC->timeOfDeath <= level.time )
	{
		self->NPC->timeOfDeath = level.time + 1000;
		// Only do all of this nonsense for Scav boys ( and girls )
	///	if ( self->client->playerTeam == TEAM_SCAVENGERS || self->client->playerTeam == TEAM_KLINGON
	//		|| self->client->playerTeam == TEAM_HIROGEN || self->client->playerTeam == TEAM_MALON )
		// should I check NPC_class here instead of TEAM ? - dmv
		if( self->client->playerTeam == TEAM_ENEMY || self->client->NPC_class == CLASS_PROTOCOL )
		{
			self->nextthink = level.time + FRAMETIME; // try back in a second

			if ( DistanceSquared( g_entities[0].currentOrigin, self->currentOrigin ) <= REMOVE_DISTANCE_SQR )
			{
				return;
			}

			if ( (InFOV( self, &g_entities[0], 110, 90 )) ) // generous FOV check
			{
				if ( (NPC_ClearLOS( &g_entities[0], self->currentOrigin )) )
				{
					return;
				}
			}
		}

		//FIXME: there are some conditions - such as heavy combat - in which we want
		//			to remove the bodies... but in other cases it's just weird, like
		//			when they're right behind you in a closed room and when they've been
		//			placed as dead NPCs by a designer...
		//			For now we just assume that a corpse with no enemy was
		//			placed in the map as a corpse
		if ( self->enemy )
		{
			if ( !self->taskManager || !self->taskManager->IsRunning() )
			{
				if ( self->client && self->client->ps.saberEntityNum > 0 && self->client->ps.saberEntityNum < ENTITYNUM_WORLD )
				{
					gentity_t *saberent = &g_entities[self->client->ps.saberEntityNum];
					if ( saberent )
					{
						G_FreeEntity( saberent );
					}
				}
				G_FreeEntity( self );
			}
		}
	}
}

/*
----------------------------------------
NPC_RemoveBody

Determines when it's ok to ditch the corpse
----------------------------------------
*/

int BodyRemovalPadTime( gentity_t *ent )
{
	int	time;

	if ( !ent || !ent->client )
		return 0;

	// team no longer indicates species/race, so in this case we'd use NPC_class, but
	switch( ent->client->NPC_class )
	{
	case CLASS_MOUSE:
	case CLASS_GONK:
	case CLASS_R2D2:
	case CLASS_R5D2:
	//case CLASS_PROTOCOL:
	case CLASS_MARK1:
	case CLASS_MARK2:
	case CLASS_PROBE:
	case CLASS_SEEKER:
	case CLASS_REMOTE:
	case CLASS_SENTRY:
	case CLASS_INTERROGATOR:
		time = 0;
		break;
	default:
		// never go away
	//	time = Q3_INFINITE;
		// for now I'm making default 10000
		time = 10000;
		break;

	}


	return time;
}


/*
----------------------------------------
NPC_RemoveBodyEffect

Effect to be applied when ditching the corpse
----------------------------------------
*/

static void NPC_RemoveBodyEffect(void)
{
//	vec3_t		org;
//	gentity_t	*tent;

	if ( !NPC || !NPC->client || (NPC->s.eFlags & EF_NODRAW) )
		return;

	// team no longer indicates species/race, so in this case we'd use NPC_class, but

	// stub code
	switch(NPC->client->NPC_class)
	{
	case CLASS_PROBE:
	case CLASS_SEEKER:
	case CLASS_REMOTE:
	case CLASS_SENTRY:
	case CLASS_GONK:
	case CLASS_R2D2:
	case CLASS_R5D2:
	//case CLASS_PROTOCOL:
	case CLASS_MARK1:
	case CLASS_MARK2:
	case CLASS_INTERROGATOR:
	case CLASS_ATST: // yeah, this is a little weird, but for now I'm listing all droids
	//	VectorCopy( NPC->currentOrigin, org );
	//	org[2] -= 16;
	//	tent = G_TempEntity( org, EV_BOT_EXPLODE );
	//	tent->owner = NPC;
		break;
	default:
		break;
	}


}


/*
====================================================================
void pitch_roll_for_slope (edict_t *forwhom, vec3_t *slope)

MG

This will adjust the pitch and roll of a monster to match
a given slope - if a non-'0 0 0' slope is passed, it will
use that value, otherwise it will use the ground underneath
the monster.  If it doesn't find a surface, it does nothinh\g
and returns.
====================================================================
*/

void pitch_roll_for_slope( gentity_t *forwhom, vec3_t pass_slope )
{
	vec3_t	slope;
	vec3_t	nvf, ovf, ovr, startspot, endspot, new_angles = { 0, 0, 0 };
	float	pitch, mod, dot;

	//if we don't have a slope, get one
	if( !pass_slope || VectorCompare( vec3_origin, pass_slope ) )
	{
		trace_t trace;

		VectorCopy( forwhom->currentOrigin, startspot );
		startspot[2] += forwhom->mins[2] + 4;
		VectorCopy( startspot, endspot );
		endspot[2] -= 300;
		gi.trace( &trace, forwhom->currentOrigin, vec3_origin, vec3_origin, endspot, forwhom->s.number, MASK_SOLID, G2_NOCOLLIDE, 0 );
//		if(trace_fraction>0.05&&forwhom.movetype==MOVETYPE_STEP)
//			forwhom.flags(-)FL_ONGROUND;

		if ( trace.fraction >= 1.0 )
			return;

		if ( VectorCompare( vec3_origin, trace.plane.normal ) )
			return;

		VectorCopy( trace.plane.normal, slope );
	}
	else
	{
		VectorCopy( pass_slope, slope );
	}

	AngleVectors( forwhom->currentAngles, ovf, ovr, NULL );

	vectoangles( slope, new_angles );
	pitch = new_angles[PITCH] + 90;
	new_angles[ROLL] = new_angles[PITCH] = 0;

	AngleVectors( new_angles, nvf, NULL, NULL );

	mod = DotProduct( nvf, ovr );

	if ( mod<0 )
		mod = -1;
	else
		mod = 1;

	dot = DotProduct( nvf, ovf );

	if ( forwhom->client )
	{
		forwhom->client->ps.viewangles[PITCH] = dot * pitch;
		forwhom->client->ps.viewangles[ROLL] = ((1-Q_fabs(dot)) * pitch * mod);
		float oldmins2 = forwhom->mins[2];
		forwhom->mins[2] = -24 + 12 * fabs(forwhom->client->ps.viewangles[PITCH])/180.0f;
		//FIXME: if it gets bigger, move up
		if ( oldmins2 > forwhom->mins[2] )
		{//our mins is now lower, need to move up
			//FIXME: trace?
			forwhom->client->ps.origin[2] += (oldmins2 - forwhom->mins[2]);
			forwhom->currentOrigin[2] = forwhom->client->ps.origin[2];
			gi.linkentity( forwhom );
		}
	}
	else
	{
		forwhom->currentAngles[PITCH] = dot * pitch;
		forwhom->currentAngles[ROLL] = ((1-Q_fabs(dot)) * pitch * mod);
	}
}

/*
void NPC_PostDeathThink( void )
{
	float	mostdist;
	trace_t trace1, trace2, trace3, trace4, movetrace;
	vec3_t	org, endpos, startpos, forward, right;
	int		whichtrace = 0;
	float	cornerdist[4];
	qboolean	frontbackbothclear = false;
	qboolean	rightleftbothclear = false;

	if( NPC->client->ps.groundEntityNum == ENTITYNUM_NONE || !VectorCompare( vec3_origin, NPC->client->ps.velocity ) )
	{
		if ( NPC->client->ps.groundEntityNum != ENTITYNUM_NONE && NPC->client->ps.friction == 1.0 )//check avelocity?
		{
			pitch_roll_for_slope( NPC, NULL );
		}

		return;
	}

	cornerdist[FRONT] = cornerdist[BACK] = cornerdist[RIGHT] = cornerdist[LEFT] = 0.0f;

	mostdist = MIN_DROP_DIST;

	AngleVectors( NPC->currentAngles, forward, right, NULL );
	VectorCopy( NPC->currentOrigin, org );
	org[2] += NPC->mins[2];

	VectorMA( org, NPC->dead_size, forward, startpos );
	VectorCopy( startpos, endpos );
	endpos[2] -= 128;
	gi.trace( &trace1, startpos, vec3_origin, vec3_origin, endpos, NPC->s.number, MASK_SOLID, );
	if( !trace1.allsolid && !trace1.startsolid )
	{
		cornerdist[FRONT] = trace1.fraction;
		if ( trace1.fraction > mostdist )
		{
			mostdist = trace1.fraction;
			whichtrace = 1;
		}
	}

	VectorMA( org, -NPC->dead_size, forward, startpos );
	VectorCopy( startpos, endpos );
	endpos[2] -= 128;
	gi.trace( &trace2, startpos, vec3_origin, vec3_origin, endpos, NPC->s.number, MASK_SOLID );
	if( !trace2.allsolid && !trace2.startsolid )
	{
		cornerdist[BACK] = trace2.fraction;
		if ( trace2.fraction > mostdist )
		{
			mostdist = trace2.fraction;
			whichtrace = 2;
		}
	}

	VectorMA( org, NPC->dead_size/2, right, startpos );
	VectorCopy( startpos, endpos );
	endpos[2] -= 128;
	gi.trace( &trace3, startpos, vec3_origin, vec3_origin, endpos, NPC->s.number, MASK_SOLID );
	if ( !trace3.allsolid && !trace3.startsolid )
	{
		cornerdist[RIGHT] = trace3.fraction;
		if ( trace3.fraction>mostdist )
		{
			mostdist = trace3.fraction;
			whichtrace = 3;
		}
	}

	VectorMA( org, -NPC->dead_size/2, right, startpos );
	VectorCopy( startpos, endpos );
	endpos[2] -= 128;
	gi.trace( &trace4, startpos, vec3_origin, vec3_origin, endpos, NPC->s.number, MASK_SOLID );
	if ( !trace4.allsolid && !trace4.startsolid )
	{
		cornerdist[LEFT] = trace4.fraction;
		if ( trace4.fraction > mostdist )
		{
			mostdist = trace4.fraction;
			whichtrace = 4;
		}
	}

	//OK!  Now if two opposite sides are hanging, use a third if any, else, do nothing
	if ( cornerdist[FRONT] > MIN_DROP_DIST && cornerdist[BACK] > MIN_DROP_DIST )
		frontbackbothclear = true;

	if ( cornerdist[RIGHT] > MIN_DROP_DIST && cornerdist[LEFT] > MIN_DROP_DIST )
		rightleftbothclear = true;

	if ( frontbackbothclear && rightleftbothclear )
		return;

	if ( frontbackbothclear )
	{
		if ( cornerdist[RIGHT] > MIN_DROP_DIST )
			whichtrace = 3;
		else if ( cornerdist[LEFT] > MIN_DROP_DIST )
			whichtrace = 4;
		else
			return;
	}

	if ( rightleftbothclear )
	{
		if ( cornerdist[FRONT] > MIN_DROP_DIST )
			whichtrace = 1;
		else if ( cornerdist[BACK] > MIN_DROP_DIST )
			whichtrace = 2;
		else
			return;
	}

	switch ( whichtrace )
	{//check for stuck
	case 1:
		VectorMA( NPC->currentOrigin, NPC->maxs[0], forward, endpos );
		gi.trace( &movetrace, NPC->currentOrigin, NPC->mins, NPC->maxs, endpos, NPC->s.number, MASK_MONSTERSOLID );
		if ( movetrace.allsolid || movetrace.startsolid || movetrace.fraction < 1.0 )
			if ( canmove( movetrace.ent ) )
				whichtrace = -1;
			else
				whichtrace = 0;
		break;
	case 2:
		VectorMA( NPC->currentOrigin, -NPC->maxs[0], forward, endpos );
		gi.trace( &movetrace, NPC->currentOrigin, NPC->mins, NPC->maxs, endpos, NPC->s.number, MASK_MONSTERSOLID );
		if ( movetrace.allsolid || movetrace.startsolid || movetrace.fraction < 1.0 )
			if ( canmove( movetrace.ent ) )
				whichtrace = -1;
			else
				whichtrace = 0;
		break;
	case 3:
		VectorMA( NPC->currentOrigin, NPC->maxs[0], right, endpos );
		gi.trace( &movetrace, NPC->currentOrigin, NPC->mins, NPC->maxs, endpos, NPC->s.number, MASK_MONSTERSOLID );
		if ( movetrace.allsolid || movetrace.startsolid || movetrace.fraction < 1.0 )
			if ( canmove( movetrace.ent ) )
				whichtrace = -1;
			else
				whichtrace = 0;
		break;
	case 4:
		VectorMA( NPC->currentOrigin, -NPC->maxs[0], right, endpos );
		gi.trace( &movetrace, NPC->currentOrigin, NPC->mins, NPC->maxs, endpos, NPC->s.number, MASK_MONSTERSOLID );
		if (movetrace.allsolid || movetrace.startsolid || movetrace.fraction < 1.0 )
			if ( canmove( movetrace.ent ) )
				whichtrace = -1;
			else
				whichtrace = 0;
		break;
	}

	switch ( whichtrace )
	{
	case 1:
		VectorMA( NPC->client->ps.velocity, 200, forward, NPC->client->ps.velocity );
		if ( trace1.fraction >= 0.9 )
		{
//can't anymore, origin not in center of deathframe!
//			NPC->avelocity[PITCH] = -300;
			NPC->client->ps.friction = 1.0;
		}
		else
		{
			pitch_roll_for_slope( NPC, &trace1.plane.normal );
			NPC->client->ps.friction = trace1.plane.normal[2] * 0.1;
		}
		return;
		break;

	case 2:
		VectorMA( NPC->client->ps.velocity, -200, forward, NPC->client->ps.velocity );
		if(trace2.fraction >= 0.9)
		{
//can't anymore, origin not in center of deathframe!
//			NPC->avelocity[PITCH] = 300;
			NPC->client->ps.friction = 1.0;
		}
		else
		{
			pitch_roll_for_slope( NPC, &trace2.plane.normal );
			NPC->client->ps.friction = trace2.plane.normal[2] * 0.1;
		}
		return;
		break;

	case 3:
		VectorMA( NPC->client->ps.velocity, 200, right, NPC->client->ps.velocity );
		if ( trace3.fraction >= 0.9 )
		{
//can't anymore, origin not in center of deathframe!
//			NPC->avelocity[ROLL] = -300;
			NPC->client->ps.friction = 1.0;
		}
		else
		{
			pitch_roll_for_slope( NPC, &trace3.plane.normal );
			NPC->client->ps.friction = trace3.plane.normal[2] * 0.1;
		}
		return;
		break;

	case 4:
		VectorMA( NPC->client->ps.velocity, -200, right, NPC->client->ps.velocity );
		if ( trace4.fraction >= 0.9 )
		{
//can't anymore, origin not in center of deathframe!
//			NPC->avelocity[ROLL] = 300;
			NPC->client->ps.friction = 1.0;
		}
		else
		{
			pitch_roll_for_slope( NPC, &trace4.plane.normal );
			NPC->client->ps.friction = trace4.plane.normal[2] * 0.1;
		}
		return;
		break;
	}

	//on solid ground
	if ( whichtrace == -1 )
	{
		return;
	}
	NPC->client->ps.friction = 1.0;

	//VectorClear( NPC->avelocity );
	pitch_roll_for_slope( NPC, NULL );

	//gi.linkentity (NPC);
}
*/

/*
----------------------------------------
DeadThink
----------------------------------------
*/
static void DeadThink ( void )
{
	trace_t	trace;

	//HACKHACKHACKHACKHACK
	//We should really have a seperate G2 bounding box (seperate from the physics bbox) for G2 collisions only
	//FIXME: don't ever inflate back up?
	NPC->maxs[2] = NPC->client->renderInfo.eyePoint[2] - NPC->currentOrigin[2] + 4;
	if ( NPC->maxs[2] < -8 )
	{
		NPC->maxs[2] = -8;
	}
	if ( VectorCompare( NPC->client->ps.velocity, vec3_origin ) )
	{//not flying through the air
		if ( NPC->mins[0] > -32 )
		{
			NPC->mins[0] -= 1;
			gi.trace (&trace, NPC->currentOrigin, NPC->mins, NPC->maxs, NPC->currentOrigin, NPC->s.number, NPC->clipmask, G2_NOCOLLIDE, 0 );
			if ( trace.allsolid )
			{
				NPC->mins[0] += 1;
			}
		}
		if ( NPC->maxs[0] < 32 )
		{
			NPC->maxs[0] += 1;
			gi.trace (&trace, NPC->currentOrigin, NPC->mins, NPC->maxs, NPC->currentOrigin, NPC->s.number, NPC->clipmask, G2_NOCOLLIDE, 0 );
			if ( trace.allsolid )
			{
				NPC->maxs[0] -= 1;
			}
		}
		if ( NPC->mins[1] > -32 )
		{
			NPC->mins[1] -= 1;
			gi.trace (&trace, NPC->currentOrigin, NPC->mins, NPC->maxs, NPC->currentOrigin, NPC->s.number, NPC->clipmask, G2_NOCOLLIDE, 0 );
			if ( trace.allsolid )
			{
				NPC->mins[1] += 1;
			}
		}
		if ( NPC->maxs[1] < 32 )
		{
			NPC->maxs[1] += 1;
			gi.trace (&trace, NPC->currentOrigin, NPC->mins, NPC->maxs, NPC->currentOrigin, NPC->s.number, NPC->clipmask, G2_NOCOLLIDE, 0 );
			if ( trace.allsolid )
			{
				NPC->maxs[1] -= 1;
			}
		}
	}
	//HACKHACKHACKHACKHACK

	//FIXME: tilt and fall off of ledges?
	//NPC_PostDeathThink();

	/*
	if ( !NPCInfo->timeOfDeath && NPC->client != NULL && NPCInfo != NULL )
	{
		//haven't finished death anim yet and were NOT given a specific amount of time to wait before removal
		int				legsAnim	= NPC->client->ps.legsAnim;
		animation_t		*animations	= knownAnimFileSets[NPC->client->clientInfo.animFileIndex].animations;

		NPC->bounceCount = -1; // This is a cheap hack for optimizing the pointcontents check below

		//ghoul doesn't tell us this anymore
		//if ( NPC->client->renderInfo.legsFrame == animations[legsAnim].firstFrame + (animations[legsAnim].numFrames - 1) )
		{
			//reached the end of the death anim
			NPCInfo->timeOfDeath = level.time + BodyRemovalPadTime( NPC );
		}
	}
	else
	*/
	{
		//death anim done (or were given a specific amount of time to wait before removal), wait the requisite amount of time them remove
		if ( level.time >= NPCInfo->timeOfDeath + BodyRemovalPadTime( NPC ) )
		{
			if ( NPC->client->ps.eFlags & EF_NODRAW )
			{
				if ( !NPC->taskManager || !NPC->taskManager->IsRunning() )
				{
					NPC->e_ThinkFunc = thinkF_G_FreeEntity;
					NPC->nextthink = level.time + FRAMETIME;
				}
			}
			else
			{
				// Start the body effect first, then delay 400ms before ditching the corpse
				NPC_RemoveBodyEffect();

				//FIXME: keep it running through physics somehow?
				NPC->e_ThinkFunc = thinkF_NPC_RemoveBody;
				NPC->nextthink = level.time + FRAMETIME;
			//	if ( NPC->client->playerTeam == TEAM_FORGE )
			//		NPCInfo->timeOfDeath = level.time + FRAMETIME * 8;
			//	else if ( NPC->client->playerTeam == TEAM_BOTS )
				class_t	npc_class = NPC->client->NPC_class;
				// check for droids
				if ( npc_class == CLASS_SEEKER || npc_class == CLASS_REMOTE || npc_class == CLASS_PROBE || npc_class == CLASS_MOUSE ||
					 npc_class == CLASS_GONK || npc_class == CLASS_R2D2 || npc_class == CLASS_R5D2 ||
					 npc_class == CLASS_MARK2 || npc_class == CLASS_SENTRY )//npc_class == CLASS_PROTOCOL ||
				{
					NPC->client->ps.eFlags |= EF_NODRAW;
					NPCInfo->timeOfDeath = level.time + FRAMETIME * 8;
				}
				else
					NPCInfo->timeOfDeath = level.time + FRAMETIME * 4;
			}
			return;
		}
	}

	// If the player is on the ground and the resting position contents haven't been set yet...(BounceCount tracks the contents)
	if ( NPC->bounceCount < 0 && NPC->s.groundEntityNum >= 0 )
	{
		// if client is in a nodrop area, make him/her nodraw
		int contents = NPC->bounceCount = gi.pointcontents( NPC->currentOrigin, -1 );

		if ( ( contents & CONTENTS_NODROP ) )
		{
			NPC->client->ps.eFlags |= EF_NODRAW;
		}
	}

	CorpsePhysics( NPC );
}


/*
===============
SetNPCGlobals

local function to set globals used throughout the AI code
===============
*/
void SetNPCGlobals( gentity_t *ent )
{
	NPC = ent;
	NPCInfo = ent->NPC;
	client = ent->client;
	memset( &ucmd, 0, sizeof( usercmd_t ) );
}

gentity_t	*_saved_NPC;
gNPC_t		*_saved_NPCInfo;
gclient_t	*_saved_client;
usercmd_t	_saved_ucmd;

void SaveNPCGlobals()
{
	_saved_NPC = NPC;
	_saved_NPCInfo = NPCInfo;
	_saved_client = client;
	memcpy( &_saved_ucmd, &ucmd, sizeof( usercmd_t ) );
}

void RestoreNPCGlobals()
{
	NPC = _saved_NPC;
	NPCInfo = _saved_NPCInfo;
	client = _saved_client;
	memcpy( &ucmd, &_saved_ucmd, sizeof( usercmd_t ) );
}

//We MUST do this, other funcs were using NPC illegally when "self" wasn't the global NPC
void ClearNPCGlobals( void )
{
	NPC = NULL;
	NPCInfo = NULL;
	client = NULL;
}
//===============

extern	qboolean	showBBoxes;
vec3_t NPCDEBUG_RED = {1.0, 0.0, 0.0};
vec3_t NPCDEBUG_GREEN = {0.0, 1.0, 0.0};
vec3_t NPCDEBUG_BLUE = {0.0, 0.0, 1.0};
vec3_t NPCDEBUG_LIGHT_BLUE = {0.3f, 0.7f, 1.0};
extern void CG_Cube( vec3_t mins, vec3_t maxs, vec3_t color, float alpha );
extern void CG_Line( vec3_t start, vec3_t end, vec3_t color, float alpha );

void NPC_ShowDebugInfo (void)
{
	if ( showBBoxes )
	{
		gentity_t	*found = NULL;
		vec3_t		mins, maxs;

		while( (found = G_Find( found, FOFS(classname), "NPC" ) ) != NULL )
		{
			if ( gi.inPVS( found->currentOrigin, g_entities[0].currentOrigin ) )
			{
				VectorAdd( found->currentOrigin, found->mins, mins );
				VectorAdd( found->currentOrigin, found->maxs, maxs );
				CG_Cube( mins, maxs, NPCDEBUG_RED, 0.25 );
			}
		}
	}
}

void NPC_ApplyScriptFlags (void)
{
	if ( NPCInfo->scriptFlags & SCF_CROUCHED )
	{
		if ( NPCInfo->charmedTime > level.time && (ucmd.forwardmove || ucmd.rightmove) )
		{//ugh, if charmed and moving, ignore the crouched command
		}
		else
		{
			ucmd.upmove = -127;
		}
	}

	if(NPCInfo->scriptFlags & SCF_RUNNING)
	{
		ucmd.buttons &= ~BUTTON_WALKING;
	}
	else if(NPCInfo->scriptFlags & SCF_WALKING)
	{
		if ( NPCInfo->charmedTime > level.time && (ucmd.forwardmove || ucmd.rightmove) )
		{//ugh, if charmed and moving, ignore the walking command
		}
		else
		{
			ucmd.buttons |= BUTTON_WALKING;
		}
	}
/*
	if(NPCInfo->scriptFlags & SCF_CAREFUL)
	{
		ucmd.buttons |= BUTTON_CAREFUL;
	}
*/
	if(NPCInfo->scriptFlags & SCF_LEAN_RIGHT)
	{
		ucmd.buttons |= BUTTON_USE;
		ucmd.rightmove = 127;
		ucmd.forwardmove = 0;
		ucmd.upmove = 0;
	}
	else if(NPCInfo->scriptFlags & SCF_LEAN_LEFT)
	{
		ucmd.buttons |= BUTTON_USE;
		ucmd.rightmove = -127;
		ucmd.forwardmove = 0;
		ucmd.upmove = 0;
	}

	if ( (NPCInfo->scriptFlags & SCF_ALT_FIRE) && (ucmd.buttons & BUTTON_ATTACK) )
	{//Use altfire instead
		ucmd.buttons |= BUTTON_ALT_ATTACK;
	}
}

void Q3_DebugPrint( int level, const char *format, ... );
void NPC_HandleAIFlags (void)
{
	//FIXME: make these flags checks a function call like NPC_CheckAIFlagsAndTimers
	if ( NPCInfo->aiFlags & NPCAI_LOST )
	{//Print that you need help!
		//FIXME: shouldn't remove this just yet if cg_draw needs it
		NPCInfo->aiFlags &= ~NPCAI_LOST;

		/*
		if ( showWaypoints )
		{
			Q3_DebugPrint(WL_WARNING, "%s can't navigate to target %s (my wp: %d, goal wp: %d)\n", NPC->targetname, NPCInfo->goalEntity->targetname, NPC->waypoint, NPCInfo->goalEntity->waypoint );
		}
		*/

		if ( NPCInfo->goalEntity && NPCInfo->goalEntity == NPC->enemy )
		{//We can't nav to our enemy
			//Drop enemy and see if we should search for him
			NPC_LostEnemyDecideChase();
		}
	}

	//MRJ Request:
	/*
	if ( NPCInfo->aiFlags & NPCAI_GREET_ALLIES && !NPC->enemy )//what if "enemy" is the greetEnt?
	{//If no enemy, look for teammates to greet
		//FIXME: don't say hi to the same guy over and over again.
		if ( NPCInfo->greetingDebounceTime < level.time )
		{//Has been at least 2 seconds since we greeted last
			if ( !NPCInfo->greetEnt )
			{//Find a teammate whom I'm facing and who is facing me and within 128
				NPCInfo->greetEnt = NPC_PickAlly( qtrue, 128, qtrue, qtrue );
			}

			if ( NPCInfo->greetEnt && !Q_irand(0, 5) )
			{//Start greeting someone
				qboolean	greeted = qfalse;

				//TODO:  If have a greetscript, run that instead?

				//FIXME: make them greet back?
				if( !Q_irand( 0, 2 ) )
				{//Play gesture anim (press gesture button?)
					greeted = qtrue;
					NPC_SetAnim( NPC, SETANIM_TORSO, Q_irand( BOTH_GESTURE1, BOTH_GESTURE3 ), SETANIM_FLAG_NORMAL|SETANIM_FLAG_HOLD );
					//NOTE: play full-body gesture if not moving?
				}

				if( !Q_irand( 0, 2 ) )
				{//Play random voice greeting sound
					greeted = qtrue;
					//FIXME: need NPC sound sets

					//G_AddVoiceEvent( NPC, Q_irand(EV_GREET1, EV_GREET3), 2000 );
				}

				if( !Q_irand( 0, 1 ) )
				{//set looktarget to them for a second or two
					greeted = qtrue;
					NPC_TempLookTarget( NPC, NPCInfo->greetEnt->s.number, 1000, 3000 );
				}

				if ( greeted )
				{//Did at least one of the things above
					//Don't greet again for 2 - 4 seconds
					NPCInfo->greetingDebounceTime = level.time + Q_irand( 2000, 4000 );
					NPCInfo->greetEnt = NULL;
				}
			}
		}
	}
	*/
	//been told to play a victory sound after a delay
	if ( NPCInfo->greetingDebounceTime && NPCInfo->greetingDebounceTime < level.time )
	{
		G_AddVoiceEvent( NPC, Q_irand(EV_VICTORY1, EV_VICTORY3), Q_irand( 2000, 4000 ) );
		NPCInfo->greetingDebounceTime = 0;
	}

	if ( NPCInfo->ffireCount > 0 )
	{
		if ( NPCInfo->ffireFadeDebounce < level.time )
		{
			NPCInfo->ffireCount--;
			//Com_Printf( "drop: %d < %d\n", NPCInfo->ffireCount, 3+((2-g_spskill->integer)*2) );
			NPCInfo->ffireFadeDebounce = level.time + 3000;
		}
	}
	if ( d_patched->integer )
	{//use patch-style navigation
		if ( NPCInfo->consecutiveBlockedMoves > 20 )
		{//been stuck for a while, try again?
			NPCInfo->consecutiveBlockedMoves = 0;
		}
	}
}

void NPC_AvoidWallsAndCliffs (void)
{
/*
	vec3_t	forward, right, testPos, angles, mins;
	trace_t	trace;
	float	fwdDist, rtDist;
	//FIXME: set things like this forward dir once at the beginning
	//of a frame instead of over and over again
	if ( NPCInfo->aiFlags & NPCAI_NO_COLL_AVOID )
	{
		return;
	}

	if ( ucmd.upmove > 0 || NPC->client->ps.groundEntityNum == ENTITYNUM_NONE )
	{//Going to jump or in the air
		return;
	}

	if ( !ucmd.forwardmove && !ucmd.rightmove )
	{
		return;
	}

	if ( fabs( AngleDelta( NPC->currentAngles[YAW], NPCInfo->desiredYaw ) ) < 5.0 )//!ucmd.angles[YAW] )
	{//Not turning much, don't do this
		//NOTE: Should this not happen only if you're not turning AT ALL?
		//	You could be turning slowly but moving fast, so that would
		//	still let you walk right off a cliff...
		//NOTE: Or maybe it is a good idea to ALWAYS do this, regardless
		//	of whether ot not we're turning?  But why would we be walking
		//  straight into a wall or off	a cliff unless we really wanted to?
		return;
	}

	VectorCopy( NPC->mins, mins );
	mins[2] += STEPSIZE;
	angles[YAW] = NPC->client->ps.viewangles[YAW];//Add ucmd.angles[YAW]?
	AngleVectors( angles, forward, right, NULL );
	fwdDist = ((float)ucmd.forwardmove)/16.0f;
	rtDist = ((float)ucmd.rightmove)/16.0f;
	VectorMA( NPC->currentOrigin, fwdDist, forward, testPos );
	VectorMA( testPos, rtDist, right, testPos );
	gi.trace( &trace, NPC->currentOrigin, mins, NPC->maxs, testPos, NPC->s.number, NPC->clipmask );
	if ( trace.allsolid || trace.startsolid || trace.fraction < 1.0 )
	{//Going to bump into something, don't move, just turn
		ucmd.forwardmove = 0;
		ucmd.rightmove = 0;
		return;
	}

	VectorCopy(trace.endpos, testPos);
	testPos[2] -= 128;

	gi.trace( &trace, trace.endpos, mins, NPC->maxs, testPos, NPC->s.number, NPC->clipmask );
	if ( trace.allsolid || trace.startsolid || trace.fraction < 1.0 )
	{//Not going off a cliff
		return;
	}

	//going to fall at least 128, don't move, just turn... is this bad, though?  What if we want them to drop off?
	ucmd.forwardmove = 0;
	ucmd.rightmove = 0;
	return;
*/
}

void NPC_CheckAttackScript(void)
{
	if(!(ucmd.buttons & BUTTON_ATTACK))
	{
		return;
	}

	G_ActivateBehavior(NPC, BSET_ATTACK);
}

float NPC_MaxDistSquaredForWeapon (void);
void NPC_CheckAttackHold(void)
{
	vec3_t		vec;

	// If they don't have an enemy they shouldn't hold their attack anim.
	if ( !NPC->enemy )
	{
		NPCInfo->attackHoldTime = 0;
		return;
	}

/*	if ( ( NPC->client->ps.weapon == WP_BORG_ASSIMILATOR ) || ( NPC->client->ps.weapon == WP_BORG_DRILL ) )
	{//FIXME: don't keep holding this if can't hit enemy?

		// If they don't have shields ( been disabled) they shouldn't hold their attack anim.
		if ( !(NPC->NPC->aiFlags & NPCAI_SHIELDS) )
		{
			NPCInfo->attackHoldTime = 0;
			return;
		}

		VectorSubtract(NPC->enemy->currentOrigin, NPC->currentOrigin, vec);
		if( VectorLengthSquared(vec) > NPC_MaxDistSquaredForWeapon() )
		{
			NPCInfo->attackHoldTime = 0;
			PM_SetTorsoAnimTimer(NPC, &NPC->client->ps.torsoAnimTimer, 0);
		}
		else if( NPCInfo->attackHoldTime && NPCInfo->attackHoldTime > level.time )
		{
			ucmd.buttons |= BUTTON_ATTACK;
		}
		else if ( ( NPCInfo->attackHold ) && ( ucmd.buttons & BUTTON_ATTACK ) )
		{
			NPCInfo->attackHoldTime = level.time + NPCInfo->attackHold;
			PM_SetTorsoAnimTimer(NPC, &NPC->client->ps.torsoAnimTimer, NPCInfo->attackHold);
		}
		else
		{
			NPCInfo->attackHoldTime = 0;
			PM_SetTorsoAnimTimer(NPC, &NPC->client->ps.torsoAnimTimer, 0);
		}
	}
	else*/
	{//everyone else...?  FIXME: need to tie this into AI somehow?
		VectorSubtract(NPC->enemy->currentOrigin, NPC->currentOrigin, vec);
		if( VectorLengthSquared(vec) > NPC_MaxDistSquaredForWeapon() )
		{
			NPCInfo->attackHoldTime = 0;
		}
		else if( NPCInfo->attackHoldTime && NPCInfo->attackHoldTime > level.time )
		{
			ucmd.buttons |= BUTTON_ATTACK;
		}
		else if ( ( NPCInfo->attackHold ) && ( ucmd.buttons & BUTTON_ATTACK ) )
		{
			NPCInfo->attackHoldTime = level.time + NPCInfo->attackHold;
		}
		else
		{
			NPCInfo->attackHoldTime = 0;
		}
	}
}

/*
void NPC_KeepCurrentFacing(void)

Fills in a default ucmd to keep current angles facing
*/
void NPC_KeepCurrentFacing(void)
{
	if(!ucmd.angles[YAW])
	{
		ucmd.angles[YAW] = ANGLE2SHORT( client->ps.viewangles[YAW] ) - client->ps.delta_angles[YAW];
	}

	if(!ucmd.angles[PITCH])
	{
		ucmd.angles[PITCH] = ANGLE2SHORT( client->ps.viewangles[PITCH] ) - client->ps.delta_angles[PITCH];
	}
}

/*
-------------------------
NPC_BehaviorSet_Charmed
-------------------------
*/

void NPC_BehaviorSet_Charmed( int bState )
{
	switch( bState )
	{
	case BS_FOLLOW_LEADER://# 40: Follow your leader and shoot any enemies you come across
		NPC_BSFollowLeader();
		break;
	case BS_REMOVE:
		NPC_BSRemove();
		break;
	case BS_SEARCH:			//# 43: Using current waypoint as a base, search the immediate branches of waypoints for enemies
		NPC_BSSearch();
		break;
	case BS_WANDER:			//# 46: Wander down random waypoint paths
		NPC_BSWander();
		break;
	case BS_FLEE:
		NPC_BSFlee();
		break;
	default:
	case BS_DEFAULT://whatever
		NPC_BSDefault();
		break;
	}
}
/*
-------------------------
NPC_BehaviorSet_Default
-------------------------
*/

void NPC_BehaviorSet_Default( int bState )
{
	switch( bState )
	{
	case BS_ADVANCE_FIGHT://head toward captureGoal, shoot anything that gets in the way
		NPC_BSAdvanceFight ();
		break;
	case BS_SLEEP://Follow a path, looking for enemies
		NPC_BSSleep ();
		break;
	case BS_FOLLOW_LEADER://# 40: Follow your leader and shoot any enemies you come across
		NPC_BSFollowLeader();
		break;
	case BS_JUMP:			//41: Face navgoal and jump to it.
		NPC_BSJump();
		break;
	case BS_REMOVE:
		NPC_BSRemove();
		break;
	case BS_SEARCH:			//# 43: Using current waypoint as a base, search the immediate branches of waypoints for enemies
		NPC_BSSearch();
		break;
	case BS_NOCLIP:
		NPC_BSNoClip();
		break;
	case BS_WANDER:			//# 46: Wander down random waypoint paths
		NPC_BSWander();
		break;
	case BS_FLEE:
		NPC_BSFlee();
		break;
	case BS_WAIT:
		NPC_BSWait();
		break;
	case BS_CINEMATIC:
		NPC_BSCinematic();
		break;
	default:
	case BS_DEFAULT://whatever
		NPC_BSDefault();
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Interrogator
-------------------------
*/
void NPC_BehaviorSet_Interrogator( int bState )
{
	switch( bState )
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSInterrogator_Default();
		break;
	default:
		NPC_BehaviorSet_Default( bState );
		break;
	}
}

void NPC_BSImperialProbe_Attack( void );
void NPC_BSImperialProbe_Patrol( void );
void NPC_BSImperialProbe_Wait(void);

/*
-------------------------
NPC_BehaviorSet_ImperialProbe
-------------------------
*/
void NPC_BehaviorSet_ImperialProbe( int bState )
{
	switch( bState )
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSImperialProbe_Default();
		break;
	default:
		NPC_BehaviorSet_Default( bState );
		break;
	}
}


void NPC_BSSeeker_Default( void );

/*
-------------------------
NPC_BehaviorSet_Seeker
-------------------------
*/
void NPC_BehaviorSet_Seeker( int bState )
{
	switch( bState )
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSSeeker_Default();
		break;
	default:
		NPC_BehaviorSet_Default( bState );
		break;
	}
}

void NPC_BSRemote_Default( void );

/*
-------------------------
NPC_BehaviorSet_Remote
-------------------------
*/
void NPC_BehaviorSet_Remote( int bState )
{
	switch( bState )
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSRemote_Default();
		break;
	default:
		NPC_BehaviorSet_Default( bState );
		break;
	}
}

void NPC_BSSentry_Default( void );

/*
-------------------------
NPC_BehaviorSet_Sentry
-------------------------
*/
void NPC_BehaviorSet_Sentry( int bState )
{
	switch( bState )
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSSentry_Default();
		break;
	default:
		NPC_BehaviorSet_Default( bState );
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Grenadier
-------------------------
*/
void NPC_BehaviorSet_Grenadier( int bState )
{
	switch( bState )
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSGrenadier_Default();
		break;

	default:
		NPC_BehaviorSet_Default( bState );
		break;
	}
}
/*
-------------------------
NPC_BehaviorSet_Sniper
-------------------------
*/
void NPC_BehaviorSet_Sniper( int bState )
{
	switch( bState )
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSSniper_Default();
		break;

	default:
		NPC_BehaviorSet_Default( bState );
		break;
	}
}
/*
-------------------------
NPC_BehaviorSet_Stormtrooper
-------------------------
*/

void NPC_BehaviorSet_Stormtrooper( int bState )
{
	switch( bState )
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSST_Default();
		break;

	case BS_INVESTIGATE:
		NPC_BSST_Investigate();
		break;

	case BS_SLEEP:
		NPC_BSST_Sleep();
		break;

	default:
		NPC_BehaviorSet_Default( bState );
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Jedi
-------------------------
*/

void NPC_BehaviorSet_Jedi( int bState )
{
	switch( bState )
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSJedi_Default();
		break;

	case BS_FOLLOW_LEADER:
		NPC_BSJedi_FollowLeader();
		break;

	default:
		NPC_BehaviorSet_Default( bState );
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Droid
-------------------------
*/
void NPC_BehaviorSet_Droid( int bState )
{
	switch( bState )
	{
	case BS_DEFAULT:
	case BS_STAND_GUARD:
	case BS_PATROL:
		NPC_BSDroid_Default();
		break;
	default:
		NPC_BehaviorSet_Default( bState );
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Mark1
-------------------------
*/
void NPC_BehaviorSet_Mark1( int bState )
{
	switch( bState )
	{
	case BS_DEFAULT:
	case BS_STAND_GUARD:
	case BS_PATROL:
		NPC_BSMark1_Default();
		break;
	default:
		NPC_BehaviorSet_Default( bState );
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Mark2
-------------------------
*/
void NPC_BehaviorSet_Mark2( int bState )
{
	switch( bState )
	{
	case BS_DEFAULT:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
		NPC_BSMark2_Default();
		break;
	default:
		NPC_BehaviorSet_Default( bState );
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_ATST
-------------------------
*/
void NPC_BehaviorSet_ATST( int bState )
{
	switch( bState )
	{
	case BS_DEFAULT:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
		NPC_BSATST_Default();
		break;
	default:
		NPC_BehaviorSet_Default( bState );
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_MineMonster
-------------------------
*/
void NPC_BehaviorSet_MineMonster( int bState )
{
	switch( bState )
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSMineMonster_Default();
		break;
	default:
		NPC_BehaviorSet_Default( bState );
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Howler
-------------------------
*/
void NPC_BehaviorSet_Howler( int bState )
{
	switch( bState )
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSHowler_Default();
		break;
	default:
		NPC_BehaviorSet_Default( bState );
		break;
	}
}

/*
-------------------------
NPC_RunBehavior
-------------------------
*/
extern void NPC_BSEmplaced( void );
extern qboolean NPC_CheckSurrender( void );
void NPC_RunBehavior( int team, int bState )
{
	//qboolean dontSetAim = qfalse;

	if ( bState == BS_CINEMATIC )
	{
		NPC_BSCinematic();
	}
	else if ( NPC->client->ps.weapon == WP_EMPLACED_GUN )
	{
		NPC_BSEmplaced();
		NPC_CheckCharmed();
		return;
	}
	else if ( NPC->client->ps.weapon == WP_SABER )
	{//jedi
		NPC_BehaviorSet_Jedi( bState );
		//dontSetAim = qtrue;
	}
	else if ( NPCInfo->scriptFlags & SCF_FORCED_MARCH )
	{//being forced to march
		NPC_BSDefault();
	}
	else
	{
		switch( team )
		{

	//	case TEAM_SCAVENGERS:
	//	case TEAM_IMPERIAL:
	//	case TEAM_KLINGON:
	//	case TEAM_HIROGEN:
	//	case TEAM_MALON:
		// not sure if TEAM_ENEMY is appropriate here, I think I should be using NPC_class to check for behavior - dmv
		case TEAM_ENEMY:
			// special cases for enemy droids
			switch( NPC->client->NPC_class)
			{
			case CLASS_ATST:
				NPC_BehaviorSet_ATST( bState );
				return;
			case CLASS_PROBE:
				NPC_BehaviorSet_ImperialProbe(bState);
				return;
			case CLASS_REMOTE:
				NPC_BehaviorSet_Remote( bState );
				return;
			case CLASS_SENTRY:
				NPC_BehaviorSet_Sentry(bState);
				return;
			case CLASS_INTERROGATOR:
				NPC_BehaviorSet_Interrogator( bState );
				return;
			case CLASS_MINEMONSTER:
				NPC_BehaviorSet_MineMonster( bState );
				return;
			case CLASS_HOWLER:
				NPC_BehaviorSet_Howler( bState );
				return;
			case CLASS_MARK1:
				NPC_BehaviorSet_Mark1( bState );
				return;
			case CLASS_MARK2:
				NPC_BehaviorSet_Mark2( bState );
				return;
			case CLASS_GALAKMECH:
				NPC_BSGM_Default();
				return;
			default:
				break;
			}

			if ( NPC->enemy && NPC->s.weapon == WP_NONE && bState != BS_HUNT_AND_KILL && !Q3_TaskIDPending( NPC, TID_MOVE_NAV ) )
			{//if in battle and have no weapon, run away, fixme: when in BS_HUNT_AND_KILL, they just stand there
				if ( bState != BS_FLEE )
				{
					NPC_StartFlee( NPC->enemy, NPC->enemy->currentOrigin, AEL_DANGER_GREAT, 5000, 10000 );
				}
				else
				{
					NPC_BSFlee();
				}
				return;
			}
			if ( NPC->client->ps.weapon == WP_SABER )
			{//special melee exception
				NPC_BehaviorSet_Default( bState );
				return;
			}
			if ( NPC->client->ps.weapon == WP_DISRUPTOR && (NPCInfo->scriptFlags & SCF_ALT_FIRE) )
			{//a sniper
				NPC_BehaviorSet_Sniper( bState );
				return;
			}
			if ( NPC->client->ps.weapon == WP_THERMAL || NPC->client->ps.weapon == WP_MELEE )//FIXME: separate AI for melee fighters
			{//a grenadier
				NPC_BehaviorSet_Grenadier( bState );
				return;
			}
			if ( NPC_CheckSurrender() )
			{
				return;
			}
			NPC_BehaviorSet_Stormtrooper( bState );
			break;

		case TEAM_NEUTRAL:

			// special cases for enemy droids
			if ( NPC->client->NPC_class == CLASS_PROTOCOL || NPC->client->NPC_class == CLASS_UGNAUGHT)
			{
				NPC_BehaviorSet_Default(bState);
			}
			else
			{
				// Just one of the average droids
				NPC_BehaviorSet_Droid( bState );
			}
			break;

		default:
			if ( NPC->client->NPC_class == CLASS_SEEKER )
			{
				NPC_BehaviorSet_Seeker(bState);
			}
			else
			{
				if ( NPCInfo->charmedTime > level.time )
				{
					NPC_BehaviorSet_Charmed( bState );
				}
				else
				{
					NPC_BehaviorSet_Default( bState );
				}
				NPC_CheckCharmed();
				//dontSetAim = qtrue;
			}
			break;
		}
	}
}

/*
===============
NPC_ExecuteBState

  MCG

NPC Behavior state thinking

===============
*/
void NPC_ExecuteBState ( gentity_t *self)//, int msec )
{
	bState_t	bState;

	NPC_HandleAIFlags();

	//FIXME: these next three bits could be a function call, some sort of setup/cleanup func
	//Lookmode must be reset every think cycle
	if(NPC->delayScriptTime && NPC->delayScriptTime <= level.time)
	{
		G_ActivateBehavior( NPC, BSET_DELAYED);
		NPC->delayScriptTime = 0;
	}

	//Clear this and let bState set it itself, so it automatically handles changing bStates... but we need a set bState wrapper func
	NPCInfo->combatMove = qfalse;

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

	//Pick the proper bstate for us and run it
	NPC_RunBehavior( self->client->playerTeam, bState );


//	if(bState != BS_POINT_COMBAT && NPCInfo->combatPoint != -1)
//	{
		//level.combatPoints[NPCInfo->combatPoint].occupied = qfalse;
		//NPCInfo->combatPoint = -1;
//	}

	//Here we need to see what the scripted stuff told us to do
//Only process snapshot if independant and in combat mode- this would pick enemies and go after needed items
//	ProcessSnapshot();

//Ignore my needs if I'm under script control- this would set needs for items
//	CheckSelf();

	//Back to normal?  All decisions made?

	//FIXME: don't walk off ledges unless we can get to our goal faster that way, or that's our goal's surface
	//NPCPredict();

	if ( NPC->enemy )
	{
		if ( !NPC->enemy->inuse )
		{//just in case bState doesn't catch this
			G_ClearEnemy( NPC );
		}
	}

	if ( NPC->client->ps.saberLockTime && NPC->client->ps.saberLockEnemy != ENTITYNUM_NONE )
	{
		NPC_SetLookTarget( NPC, NPC->client->ps.saberLockEnemy, level.time+1000 );
	}
	else if ( !NPC_CheckLookTarget( NPC ) )
	{
		if ( NPC->enemy )
		{
			NPC_SetLookTarget( NPC, NPC->enemy->s.number, 0 );
		}
	}

	if ( NPC->enemy )
	{
		if(NPC->enemy->flags & FL_DONT_SHOOT)
		{
			ucmd.buttons &= ~BUTTON_ATTACK;
			ucmd.buttons &= ~BUTTON_ALT_ATTACK;
		}
		else if ( NPC->client->playerTeam != TEAM_ENEMY && NPC->enemy->NPC && (NPC->enemy->NPC->surrenderTime > level.time || (NPC->enemy->NPC->scriptFlags&SCF_FORCED_MARCH)) )
		{//don't shoot someone who's surrendering if you're a good guy
			ucmd.buttons &= ~BUTTON_ATTACK;
			ucmd.buttons &= ~BUTTON_ALT_ATTACK;
		}

		if(client->ps.weaponstate == WEAPON_IDLE)
		{
			client->ps.weaponstate = WEAPON_READY;
		}
	}
	else
	{
		if(client->ps.weaponstate == WEAPON_READY)
		{
			client->ps.weaponstate = WEAPON_IDLE;
		}
	}

	if(!(ucmd.buttons & BUTTON_ATTACK) && NPC->attackDebounceTime > level.time)
	{//We just shot but aren't still shooting, so hold the gun up for a while
		if(client->ps.weapon == WP_SABER )
		{//One-handed
			NPC_SetAnim(NPC,SETANIM_TORSO,TORSO_WEAPONREADY1,SETANIM_FLAG_NORMAL);
		}
		else if(client->ps.weapon == WP_BRYAR_PISTOL)
		{//Sniper pose
			NPC_SetAnim(NPC,SETANIM_TORSO,TORSO_WEAPONREADY3,SETANIM_FLAG_NORMAL);
		}
		/*//FIXME: What's the proper solution here?
		else
		{//heavy weapon
			NPC_SetAnim(NPC,SETANIM_TORSO,TORSO_WEAPONREADY3,SETANIM_FLAG_NORMAL);
		}
		*/
	}
	else if ( !NPC->enemy )//HACK!
	{
		if( NPC->s.torsoAnim == TORSO_WEAPONREADY1 || NPC->s.torsoAnim == TORSO_WEAPONREADY3 )
		{//we look ready for action, using one of the first 2 weapon, let's rest our weapon on our shoulder
			NPC_SetAnim(NPC,SETANIM_TORSO,TORSO_WEAPONIDLE1,SETANIM_FLAG_NORMAL);
		}
	}

	NPC_CheckAttackHold();
	NPC_ApplyScriptFlags();

	//cliff and wall avoidance
	NPC_AvoidWallsAndCliffs();

	// run the bot through the server like it was a real client
//=== Save the ucmd for the second no-think Pmove ============================
	ucmd.serverTime = level.time - 50;
	memcpy( &NPCInfo->last_ucmd, &ucmd, sizeof( usercmd_t ) );
	if ( !NPCInfo->attackHoldTime )
	{
		NPCInfo->last_ucmd.buttons &= ~(BUTTON_ATTACK|BUTTON_ALT_ATTACK);//so we don't fire twice in one think
	}
//============================================================================
	NPC_CheckAttackScript();
	NPC_KeepCurrentFacing();

	if ( !NPC->next_roff_time || NPC->next_roff_time < level.time )
	{//If we were following a roff, we don't do normal pmoves.
		ClientThink( NPC->s.number, &ucmd );
	}
	else
	{
		NPC_ApplyRoff();
	}

	// end of thinking cleanup
	NPCInfo->touchedByPlayer = NULL;

	NPC_CheckPlayerAim();
	NPC_CheckAllClear();

	/*if( ucmd.forwardmove || ucmd.rightmove )
	{
		int	i, la = -1, ta = -1;

		for(i = 0; i < MAX_ANIMATIONS; i++)
		{
			if( NPC->client->ps.legsAnim == i )
			{
				la = i;
			}

			if( NPC->client->ps.torsoAnim == i )
			{
				ta = i;
			}

			if(la != -1 && ta != -1)
			{
				break;
			}
		}

		if(la != -1 && ta != -1)
		{//FIXME: should never play same frame twice or restart an anim before finishing it
			gi.Printf("LegsAnim: %s(%d) TorsoAnim: %s(%d)\n", animTable[la].name, NPC->renderInfo.legsFrame, animTable[ta].name, NPC->client->renderInfo.torsoFrame);
		}
	}*/
}

void NPC_CheckInSolid(void)
{
	trace_t	trace;
	vec3_t	point;
	VectorCopy(NPC->currentOrigin, point);
	point[2] -= 0.25;

	gi.trace(&trace, NPC->currentOrigin, NPC->mins, NPC->maxs, point, NPC->s.number, NPC->clipmask, G2_NOCOLLIDE, 0);
	if(!trace.startsolid && !trace.allsolid)
	{
		VectorCopy(NPC->currentOrigin, NPCInfo->lastClearOrigin);
	}
	else
	{
		if(VectorLengthSquared(NPCInfo->lastClearOrigin))
		{
//			gi.Printf("%s stuck in solid at %s: fixing...\n", NPC->script_targetname, vtos(NPC->currentOrigin));
			G_SetOrigin(NPC, NPCInfo->lastClearOrigin);
			gi.linkentity(NPC);
		}
	}
}

/*
===============
NPC_Think

Main NPC AI - called once per frame
===============
*/
#if	AI_TIMERS
extern int AITime;
#endif//	AI_TIMERS
void NPC_Think ( gentity_t *self)//, int msec )
{
	vec3_t	oldMoveDir;

	self->nextthink = level.time + FRAMETIME;

	SetNPCGlobals( self );

	memset( &ucmd, 0, sizeof( ucmd ) );

	VectorCopy( self->client->ps.moveDir, oldMoveDir );
	VectorClear( self->client->ps.moveDir );
	// see if NPC ai is frozen
	if ( debugNPCFreeze->value || (NPC->svFlags&SVF_ICARUS_FREEZE) )
	{
		NPC_UpdateAngles( qtrue, qtrue );
		ClientThink(self->s.number, &ucmd);
		VectorCopy(self->s.origin, self->s.origin2 );
		return;
	}

	if(!self || !self->NPC || !self->client)
	{
		return;
	}

	// dead NPCs have a special think, don't run scripts (for now)
	//FIXME: this breaks deathscripts
	if ( self->health <= 0 )
	{
		DeadThink();
		if ( NPCInfo->nextBStateThink <= level.time )
		{
			if( self->taskManager && !stop_icarus )
			{
				self->taskManager->Update( );
			}
		}
		return;
	}

	self->nextthink = level.time + FRAMETIME/2;

	if ( player->client->ps.viewEntity == self->s.number )
	{//being controlled by player
		if ( self->client )
		{//make the noises
			if ( TIMER_Done( self, "patrolNoise" ) && !Q_irand( 0, 20 ) )
			{
				switch( self->client->NPC_class )
				{
				case CLASS_R2D2:				// droid
					G_SoundOnEnt(self, CHAN_AUTO, va("sound/chars/r2d2/misc/r2d2talk0%d.wav",Q_irand(1, 3)) );
					break;
				case CLASS_R5D2:				// droid
					G_SoundOnEnt(self, CHAN_AUTO, va("sound/chars/r5d2/misc/r5talk%d.wav",Q_irand(1, 4)) );
					break;
				case CLASS_PROBE:				// droid
					G_SoundOnEnt(self, CHAN_AUTO, va("sound/chars/probe/misc/probetalk%d.wav",Q_irand(1, 3)) );
					break;
				case CLASS_MOUSE:				// droid
					G_SoundOnEnt(self, CHAN_AUTO, va("sound/chars/mouse/misc/mousego%d.wav",Q_irand(1, 3)) );
					break;
				case CLASS_GONK:				// droid
					G_SoundOnEnt(self, CHAN_AUTO, va("sound/chars/gonk/misc/gonktalk%d.wav",Q_irand(1, 2)) );
					break;
				default:
					break;
				}
				TIMER_Set( self, "patrolNoise", Q_irand( 2000, 4000 ) );
			}
		}
		//FIXME: might want to at least make sounds or something?
		//NPC_UpdateAngles(qtrue, qtrue);
		//Which ucmd should we send?  Does it matter, since it gets overridden anyway?
		NPCInfo->last_ucmd.serverTime = level.time - 50;
		ClientThink( NPC->s.number, &ucmd );
		VectorCopy(self->s.origin, self->s.origin2 );
		return;
	}

	if ( NPCInfo->nextBStateThink <= level.time )
	{
#if	AI_TIMERS
		int	startTime = GetTime(0);
#endif//	AI_TIMERS
		if ( NPC->s.eType != ET_PLAYER )
		{//Something drastic happened in our script
			return;
		}

		if ( NPC->s.weapon == WP_SABER && g_spskill->integer >= 2 && NPCInfo->rank > RANK_LT_JG )
		{//Jedi think faster on hard difficulty, except low-rank (reborn)
			NPCInfo->nextBStateThink = level.time + FRAMETIME/2;
		}
		else
		{//Maybe even 200 ms?
			NPCInfo->nextBStateThink = level.time + FRAMETIME;
		}

		//nextthink is set before this so something in here can override it
		NPC_ExecuteBState( self );

#if	AI_TIMERS
		int addTime = GetTime( startTime );
		if ( addTime > 50 )
		{
			gi.Printf( S_COLOR_RED"ERROR: NPC number %d, %s %s at %s, weaponnum: %d, using %d of AI time!!!\n", NPC->s.number, NPC->NPC_type, NPC->targetname, vtos(NPC->currentOrigin), NPC->s.weapon, addTime );
		}
		AITime += addTime;
#endif//	AI_TIMERS
	}
	else
	{
		VectorCopy( oldMoveDir, self->client->ps.moveDir );
		//or use client->pers.lastCommand?
		NPCInfo->last_ucmd.serverTime = level.time - 50;
		if ( !NPC->next_roff_time || NPC->next_roff_time < level.time )
		{//If we were following a roff, we don't do normal pmoves.
			//FIXME: firing angles (no aim offset) or regular angles?
			NPC_UpdateAngles(qtrue, qtrue);
			memcpy( &ucmd, &NPCInfo->last_ucmd, sizeof( usercmd_t ) );
			ClientThink(NPC->s.number, &ucmd);
		}
		else
		{
			NPC_ApplyRoff();
		}
		VectorCopy(self->s.origin, self->s.origin2 );
	}
	//must update icarus *every* frame because of certain animation completions in the pmove stuff that can leave a 50ms gap between ICARUS animation commands
	if( self->taskManager && !stop_icarus )
	{
		self->taskManager->Update();
	}
}

void NPC_InitAI ( void )
{
	debugNoRoam = gi.cvar ( "d_noroam", "0", CVAR_CHEAT );
	debugNPCAimingBeam = gi.cvar ( "d_npcaiming", "0", CVAR_CHEAT );
	debugBreak = gi.cvar ( "d_break", "0", CVAR_CHEAT );

	debugNPCAI = gi.cvar ( "d_npcai", "0", CVAR_CHEAT );
	debugNPCFreeze = gi.cvar ( "d_npcfreeze", "0", CVAR_CHEAT);
	d_JediAI = gi.cvar ( "d_JediAI", "0", CVAR_CHEAT );
	d_noGroupAI = gi.cvar ( "d_noGroupAI", "0", CVAR_CHEAT );
	d_asynchronousGroupAI = gi.cvar ( "d_asynchronousGroupAI", "1", CVAR_CHEAT );
	d_altRoutes = gi.cvar ( "d_altRoutes", "1", CVAR_CHEAT );
	d_patched = gi.cvar ( "d_patched", "1", CVAR_CHEAT );

	//0 = never (BORING)
	//1 = kyle only
	//2 = kyle and last enemy jedi
	//3 = kyle and any enemy jedi
	//4 = kyle and last enemy in a group
	//5 = kyle and any enemy
	//6 = also when kyle takes pain or enemy jedi dodges player saber swing or does an acrobatic evasion
	d_slowmodeath = gi.cvar ( "d_slowmodeath", "3", CVAR_ARCHIVE );//save this setting

	d_saberCombat = gi.cvar ( "d_saberCombat", "0", CVAR_CHEAT );
}

/*
==================================
void NPC_InitAnimTable( void )

  Need to initialize this table.
  If someone tried to play an anim
  before table is filled in with
  values, causes tasks that wait for
  anim completion to never finish.
  (frameLerp of 0 * numFrames of 0 = 0)
==================================
*/
void NPC_InitAnimTable( void )
{
	for ( int i = 0; i < MAX_ANIM_FILES; i++ )
	{
		for ( int j = 0; j < MAX_ANIMATIONS; j++ )
		{
			level.knownAnimFileSets[i].animations[j].firstFrame = 0;
			level.knownAnimFileSets[i].animations[j].frameLerp = 100;
			level.knownAnimFileSets[i].animations[j].initialLerp = 100;
			level.knownAnimFileSets[i].animations[j].numFrames = 0;
		}
	}
}

void NPC_InitGame( void )
{
//	globals.NPCs = (gNPC_t *) gi.TagMalloc(game.maxclients * sizeof(game.bots[0]), TAG_GAME);
	debugNPCName = gi.cvar ( "d_npc", "", 0  );
	NPC_LoadParms();
	NPC_InitAI();
	NPC_InitAnimTable();
	/*
	ResetTeamCounters();
	for ( int team = TEAM_FREE; team < TEAM_NUM_TEAMS; team++ )
	{
		teamLastEnemyTime[team] = -10000;
	}
	*/
}

void NPC_SetAnim(gentity_t	*ent,int setAnimParts,int anim,int setAnimFlags)
{	// FIXME : once torsoAnim and legsAnim are in the same structure for NCP and Players
	// rename PM_SETAnimFinal to PM_SetAnim and have both NCP and Players call PM_SetAnim

	if(ent->client)
	{//Players, NPCs
		if (setAnimFlags&SETANIM_FLAG_OVERRIDE)
		{
			if (setAnimParts & SETANIM_TORSO)
			{
				if( (setAnimFlags & SETANIM_FLAG_RESTART) || ent->client->ps.torsoAnim != anim )
				{
					PM_SetTorsoAnimTimer( ent, &ent->client->ps.torsoAnimTimer, 0 );
				}
			}
			if (setAnimParts & SETANIM_LEGS)
			{
				if( (setAnimFlags & SETANIM_FLAG_RESTART) || ent->client->ps.legsAnim != anim )
				{
					PM_SetLegsAnimTimer( ent, &ent->client->ps.legsAnimTimer, 0 );
				}
			}
		}

		PM_SetAnimFinal(&ent->client->ps.torsoAnim,&ent->client->ps.legsAnim,setAnimParts,anim,setAnimFlags,
			&ent->client->ps.torsoAnimTimer,&ent->client->ps.legsAnimTimer,ent);
	}
	else
	{//bodies, etc.
		if (setAnimFlags&SETANIM_FLAG_OVERRIDE)
		{
			if (setAnimParts & SETANIM_TORSO)
			{
				if( (setAnimFlags & SETANIM_FLAG_RESTART) || ent->s.torsoAnim != anim )
				{
					PM_SetTorsoAnimTimer( ent, &ent->s.torsoAnimTimer, 0 );
				}
			}
			if (setAnimParts & SETANIM_LEGS)
			{
				if( (setAnimFlags & SETANIM_FLAG_RESTART) || ent->s.legsAnim != anim )
				{
					PM_SetLegsAnimTimer( ent, &ent->s.legsAnimTimer, 0 );
				}
			}
		}

		PM_SetAnimFinal(&ent->s.torsoAnim,&ent->s.legsAnim,setAnimParts,anim,setAnimFlags,
			&ent->s.torsoAnimTimer,&ent->s.legsAnimTimer,ent);
	}
}
