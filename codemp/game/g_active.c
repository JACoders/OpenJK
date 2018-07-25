// Copyright (C) 1999-2000 Id Software, Inc.
//

#include "g_local.h"
#include "bg_saga.h"
#include "g_unlagged.h"

extern void Jedi_Cloak( gentity_t *self );
extern void Jedi_Decloak( gentity_t *self );
extern void G_TestLine(vec3_t start, vec3_t end, int color, int time);

static int frametime = 25;
static int historytime = 250;
static int numTrails = 10; //historytime/frametime; 

qboolean PM_SaberInTransition( int move );
qboolean PM_SaberInStart( int move );
qboolean PM_SaberInReturn( int move );
qboolean WP_SaberStyleValidForSaber( saberInfo_t *saber1, saberInfo_t *saber2, int saberHolstered, int saberAnimLevel );
qboolean saberCheckKnockdown_DuelLoss(gentity_t *saberent, gentity_t *saberOwner, gentity_t *other);
qboolean BG_CanJetpack(playerState_t *ps);

void P_SetTwitchInfo(gclient_t	*client)
{
	client->ps.painTime = level.time;
	client->ps.painDirection ^= 1;
}

/*
============
G_ResetTrail

Clear out the given client's origin trails (should be called from ClientBegin and when
the teleport bit is toggled)
============
*/
void G_ResetTrail( gentity_t *ent ) {
	int		i, time;

	// fill up the origin trails with data (assume the current position for the last 1/2 second or so)
	ent->client->unlagged.trailHead = numTrails - 1;
	for ( i = ent->client->unlagged.trailHead, time = level.time; i >= 0; i--, time -= frametime ) {
		VectorCopy( ent->r.mins, ent->client->unlagged.trail[i].mins );
		VectorCopy( ent->r.maxs, ent->client->unlagged.trail[i].maxs );
		VectorCopy( ent->r.currentOrigin, ent->client->unlagged.trail[i].currentOrigin );
		//VectorCopy( ent->r.currentAngles, ent->client->unlagged.trail[i].currentAngles );

		ent->client->unlagged.trail[i].torsoAnim = ent->client->ps.torsoAnim;
		ent->client->unlagged.trail[i].torsoTimer = ent->client->ps.torsoTimer;
		ent->client->unlagged.trail[i].legsAnim = ent->client->ps.legsAnim;
		ent->client->unlagged.trail[i].legsTimer = ent->client->ps.legsTimer;
		ent->client->unlagged.trail[i].realAngle = ent->s.apos.trBase[YAW];

		ent->client->unlagged.trail[i].leveltime = time;
		ent->client->unlagged.trail[i].time = time;
	}
}


/*
============
G_StoreTrail

Keep track of where the client's been (usually called every ClientThink)
============
*/
void G_StoreTrail( gentity_t *ent ) {
	int		head, newtime;

	head = ent->client->unlagged.trailHead;

	// if we're on a new frame
	if ( ent->client->unlagged.trail[head].leveltime < level.time ) {
		// snap the last head up to the end of frame time
		ent->client->unlagged.trail[head].time = level.previousTime;

		// increment the head
		ent->client->unlagged.trailHead++;
		if ( ent->client->unlagged.trailHead >= numTrails ) {
			ent->client->unlagged.trailHead = 0;
		}
		head = ent->client->unlagged.trailHead;
	}

	if ( ent->r.svFlags & SVF_BOT ) {
		// bots move only once per frame
		newtime = level.time;
	} else {
		// calculate the actual server time
		// (we set level.frameStartTime every G_RunFrame)
		newtime = level.previousTime + trap->Milliseconds() - level.frameStartTime;
		if ( newtime > level.time ) {
			newtime = level.time;
		} else if ( newtime <= level.previousTime ) {
			newtime = level.previousTime + 1;
		}
	}

	// store all the collision-detection info and the time
	VectorCopy( ent->r.mins, ent->client->unlagged.trail[head].mins );
	VectorCopy( ent->r.maxs, ent->client->unlagged.trail[head].maxs );
	VectorCopy( ent->r.currentOrigin, ent->client->unlagged.trail[head].currentOrigin );
	//VectorCopy( ent->r.currentAngles, ent->client->unlagged.trail[head].currentAngles );

	ent->client->unlagged.trail[head].torsoAnim = ent->client->ps.torsoAnim;
	ent->client->unlagged.trail[head].torsoTimer = ent->client->ps.torsoTimer;
	ent->client->unlagged.trail[head].legsAnim = ent->client->ps.legsAnim;
	ent->client->unlagged.trail[head].legsTimer = ent->client->ps.legsTimer;
	ent->client->unlagged.trail[head].realAngle = ent->s.apos.trBase[YAW];

	//if (ent->client->unlagged.trail[head].currentAngles[0] || ent->client->unlagged.trail[head].currentAngles[1] || ent->client->unlagged.trail[head].currentAngles[2])
		//Com_Printf("Current angles are %.2f %.2f %.2f\n", ent->client->unlagged.trail[head].currentAngles[0], ent->client->unlagged.trail[head].currentAngles[1], ent->client->unlagged.trail[head].currentAngles[2]);

	ent->client->unlagged.trail[head].leveltime = level.time;
	ent->client->unlagged.trail[head].time = newtime;

	//Also store their anim info? Since with ghoul2 collision that matters..

	// FOR TESTING ONLY
	//Com_Printf("level.previousTime: %d, level.time: %d, newtime: %d\n", level.previousTime, level.time, newtime);
}


/*
=============
TimeShiftLerp

Used below to interpolate between two previous vectors
Returns a vector "frac" times the distance between "start" and "end"
=============
*/
static void TimeShiftLerp( float frac, vec3_t start, vec3_t end, vec3_t result ) {
	float	comp = 1.0f - frac;

	result[0] = frac * start[0] + comp * end[0];
	result[1] = frac * start[1] + comp * end[1];
	result[2] = frac * start[2] + comp * end[2];
}

static void TimeShiftAnimLerp( float frac, int anim1, int anim2, int time1, int time2, int *outTime ) {
	if (anim1 == anim2 && time2 > time1) {//Only lerp if both anims are same and time2 is after time1.
		*outTime = time1 + (time2 - time1)*frac;
	}
	else
		*outTime = time2;

	//Com_Printf("Timeshift anim lerping: time1 is %i, time 2 is %i, lerped is %i\n", time1, time2, outTime);
}


static void G_DrawPlayerStick(gentity_t *ent, int color, int duration, int time) {
	//Lets draw a visual of the unlagged hitbox difference?
	//Not sure how to get bounding boxes of g2 parts, so maybe just two stick figures? (lagged stick figure = red, unlagged stick figure = green) and make them both appear for like 5 seconds?
	vec3_t headPos, torsoPos, rHandPos, lHandPos, rArmPos, lArmPos, rKneePos, lKneePos, rFootPos, lFootPos, G2Angles;
	mdxaBone_t	boltMatrix;
	int handLBolt, handRBolt, armRBolt, armLBolt, kneeLBolt, kneeRBolt, footLBolt, footRBolt;

	if (!ent->client)
		return;
	if (ent->localAnimIndex > 1) //Not humanoid
		return;

	//Com_Printf("Drawing player stick model for %s\n", ent->client->pers.netname);

	handLBolt = trap->G2API_AddBolt(ent->ghoul2, 0, "*l_hand");
	handRBolt = trap->G2API_AddBolt(ent->ghoul2, 0, "*r_hand");
	armLBolt = trap->G2API_AddBolt(ent->ghoul2, 0, "*l_arm_elbow");
	armRBolt = trap->G2API_AddBolt(ent->ghoul2, 0, "*r_arm_elbow");
	kneeLBolt = trap->G2API_AddBolt(ent->ghoul2, 0, "*hips_l_knee");
	kneeRBolt = trap->G2API_AddBolt(ent->ghoul2, 0, "*hips_r_knee");
	footLBolt = trap->G2API_AddBolt(ent->ghoul2, 0, "*l_leg_foot");
	footRBolt = trap->G2API_AddBolt(ent->ghoul2, 0, "*r_leg_foot");  //Shouldnt these always be the same numbers? can just make them constants?.. or pm->g2Bolts_RFoot etc?

	VectorSet(G2Angles, 0, ent->s.apos.trBase[YAW], 0); //Ok so r.currentAngles isnt even used for players ??
	VectorCopy(ent->r.currentOrigin, torsoPos);

	VectorCopy(torsoPos, headPos);
	headPos[2] += ent->r.maxs[2]; //E?
	torsoPos[2] += 8;//idk man
	G_TestLine(headPos, torsoPos, color, duration); //Head -> Torso

	trap->G2API_GetBoltMatrix( ent->ghoul2, 0, armRBolt, &boltMatrix, G2Angles, ent->r.currentOrigin, time, NULL, ent->modelScale ); //ent->cmd.serverTime ?.. why does this need time?
	rArmPos[0] = boltMatrix.matrix[0][3];
	rArmPos[1] = boltMatrix.matrix[1][3];
	rArmPos[2] = boltMatrix.matrix[2][3];
	G_TestLine(torsoPos, rArmPos, color, duration); //Torso -> R Arm

	trap->G2API_GetBoltMatrix( ent->ghoul2, 0, armLBolt, &boltMatrix, G2Angles, ent->r.currentOrigin, time, NULL, ent->modelScale );
	lArmPos[0] = boltMatrix.matrix[0][3];
	lArmPos[1] = boltMatrix.matrix[1][3];
	lArmPos[2] = boltMatrix.matrix[2][3];
	G_TestLine(torsoPos, lArmPos, color, duration); //Torso -> L Arm

	trap->G2API_GetBoltMatrix( ent->ghoul2, 0, handRBolt, &boltMatrix, G2Angles, ent->r.currentOrigin, time, NULL, ent->modelScale ); 
	rHandPos[0] = boltMatrix.matrix[0][3];
	rHandPos[1] = boltMatrix.matrix[1][3];
	rHandPos[2] = boltMatrix.matrix[2][3];
	G_TestLine(rArmPos, rHandPos, color, duration); //R Arm  -> R Hand

	trap->G2API_GetBoltMatrix( ent->ghoul2, 0, handLBolt, &boltMatrix, G2Angles, ent->r.currentOrigin, time, NULL, ent->modelScale );
	lHandPos[0] = boltMatrix.matrix[0][3];
	lHandPos[1] = boltMatrix.matrix[1][3];
	lHandPos[2] = boltMatrix.matrix[2][3];
	G_TestLine(lArmPos, lHandPos, color, duration); //L Arm -> L Hand

	trap->G2API_GetBoltMatrix( ent->ghoul2, 0, kneeRBolt, &boltMatrix, G2Angles, ent->r.currentOrigin, time, NULL, ent->modelScale );
	rKneePos[0] = boltMatrix.matrix[0][3];
	rKneePos[1] = boltMatrix.matrix[1][3];
	rKneePos[2] = boltMatrix.matrix[2][3];
	G_TestLine(torsoPos, rKneePos, color, duration); //Torso -> R Knee

	trap->G2API_GetBoltMatrix( ent->ghoul2, 0, kneeLBolt, &boltMatrix, G2Angles, ent->r.currentOrigin, time, NULL, ent->modelScale );
	lKneePos[0] = boltMatrix.matrix[0][3];
	lKneePos[1] = boltMatrix.matrix[1][3];
	lKneePos[2] = boltMatrix.matrix[2][3];
	G_TestLine(torsoPos, lKneePos, color, duration); //Torso -> L Knee

	trap->G2API_GetBoltMatrix( ent->ghoul2, 0, footRBolt, &boltMatrix, G2Angles, ent->r.currentOrigin, time, NULL, ent->modelScale );
	rFootPos[0] = boltMatrix.matrix[0][3];
	rFootPos[1] = boltMatrix.matrix[1][3];
	rFootPos[2] = boltMatrix.matrix[2][3];
	G_TestLine(rKneePos, rFootPos, color, duration); //R Knee -> R Foot

	trap->G2API_GetBoltMatrix( ent->ghoul2, 0, footLBolt, &boltMatrix, G2Angles, ent->r.currentOrigin, time, NULL, ent->modelScale );
	lFootPos[0] = boltMatrix.matrix[0][3];
	lFootPos[1] = boltMatrix.matrix[1][3];
	lFootPos[2] = boltMatrix.matrix[2][3];
	G_TestLine(lKneePos, lFootPos, color, duration); //L Knee -> L Foot
}


/*
=================
G_TimeShiftClient

Move a client back to where he was at the specified "time"
=================
*/
void G_TimeShiftClient( gentity_t *ent, int time, qboolean timeshiftAnims ) {
	int		j, k;

	if ( time > level.time ) {
		time = level.time;
	}

	// find two entries in the origin trail whose times sandwich "time"
	// assumes no two adjacent trail records have the same timestamp
	j = k = ent->client->unlagged.trailHead;
	do {
		if ( ent->client->unlagged.trail[j].time <= time )
			break;

		k = j;
		j--;
		if ( j < 0 ) {
			j = numTrails - 1;
		}
	}
	while ( j != ent->client->unlagged.trailHead );

	// if we got past the first iteration above, we've sandwiched (or wrapped)
	if ( j != k ) {
		// make sure it doesn't get re-saved
		if ( ent->client->unlagged.saved.leveltime != level.time ) {
			// save the current origin and bounding box
			VectorCopy( ent->r.mins, ent->client->unlagged.saved.mins );
			VectorCopy( ent->r.maxs, ent->client->unlagged.saved.maxs );
			VectorCopy( ent->r.currentOrigin, ent->client->unlagged.saved.currentOrigin );
			//VectorCopy( ent->r.currentAngles, ent->client->unlagged.saved.currentAngles );

			if (timeshiftAnims) {
				ent->client->unlagged.saved.torsoAnim = ent->client->ps.torsoAnim;
				ent->client->unlagged.saved.torsoTimer = ent->client->ps.torsoTimer;
				ent->client->unlagged.saved.legsAnim = ent->client->ps.legsAnim;
				ent->client->unlagged.saved.legsTimer = ent->client->ps.legsTimer;
				ent->client->unlagged.saved.realAngle = ent->s.apos.trBase[YAW];
			}

			ent->client->unlagged.saved.leveltime = level.time;
		}

#if 1
		if (g_unlagged.integer & (1<<3)) {
			G_DrawPlayerStick(ent, 0x0000ff, 5000, level.time);
			//Com_Printf("pre angle is %.2f\n", ent->s.apos.trBase[YAW]);
		}
#endif

		// if we haven't wrapped back to the head, we've sandwiched, so
		// we shift the client's position back to where he was at "time"
		if ( j != ent->client->unlagged.trailHead )
		{
			float	frac = (float)(ent->client->unlagged.trail[k].time - time) / (float)(ent->client->unlagged.trail[k].time - ent->client->unlagged.trail[j].time);

			// FOR TESTING ONLY
			//Com_Printf( "level time: %d, fire time: %d, j time: %d, k time: %d\n", level.time, time, ent->client->unlagged.trail[j].time, ent->client->unlagged.trail[k].time );

			// interpolate between the two origins to give position at time index "time"
			TimeShiftLerp( frac, ent->client->unlagged.trail[k].currentOrigin, ent->client->unlagged.trail[j].currentOrigin, ent->r.currentOrigin );
			//ent->r.currentAngles[YAW] = LerpAngle( ent->client->unlagged.trail[k].currentAngles[YAW], ent->r.currentAngles[YAW], frac );

			// lerp these too, just for fun (and ducking)
			TimeShiftLerp( frac, ent->client->unlagged.trail[k].mins, ent->client->unlagged.trail[j].mins, ent->r.mins );
			TimeShiftLerp( frac, ent->client->unlagged.trail[k].maxs, ent->client->unlagged.trail[j].maxs, ent->r.maxs );
			
			//Lerp this somehow?
			if (timeshiftAnims) {
				/*
				Com_Printf("Lerp timeshifting client %s. Old anim = %i %i (times %i %i).  New Anim = %i %i (times %i %i).\n", 
					ent->client->pers.netname,
					ent->client->ps.torsoAnim, ent->client->ps.legsAnim, ent->client->ps.torsoTimer, ent->client->ps.legsTimer, 
					ent->client->unlagged.trail[k].torsoAnim, ent->client->unlagged.trail[k].legsAnim, ent->client->unlagged.trail[k].torsoTimer, ent->client->unlagged.trail[k].legsTimer);
				*/

				ent->client->ps.torsoAnim = ent->client->unlagged.trail[k].torsoAnim;
				ent->client->ps.legsAnim = ent->client->unlagged.trail[k].legsAnim;
				TimeShiftAnimLerp(frac, ent->client->unlagged.trail[j].torsoAnim, ent->client->unlagged.trail[k].torsoAnim, ent->client->unlagged.trail[j].torsoTimer, ent->client->unlagged.trail[k].torsoTimer,  &ent->client->ps.torsoTimer);
				TimeShiftAnimLerp(frac, ent->client->unlagged.trail[j].legsAnim, ent->client->unlagged.trail[k].legsAnim, ent->client->unlagged.trail[j].legsTimer, ent->client->unlagged.trail[k].legsTimer, &ent->client->ps.legsTimer);
				//ent->s.apos.trBase[YAW] = LerpAngle( ent->client->unlagged.trail[k].realAngle, ent->s.apos.trBase[YAW], frac ); //Shouldnt this be lerping between k and j instead of k and trbase ?
				ent->s.apos.trBase[YAW] = LerpAngle( ent->client->unlagged.trail[k].realAngle, ent->client->unlagged.trail[j].realAngle, frac ); //Shouldnt this be lerping between k and j instead of k and trbase ?

				//Com_Printf("j angle is %.2f k angle is %.2f frac is %.2f\n", ent->client->unlagged.trail[j].realAngle, ent->client->unlagged.trail[k].realAngle, frac);
				//Com_Printf("interp angle is %.2f\n", LerpAngle( ent->client->unlagged.trail[j].realAngle, ent->client->unlagged.trail[k].realAngle, frac ));
			}

			// this will recalculate absmin and absmax
			trap->LinkEntity( (sharedEntity_t *)ent );
		} else {
			// we wrapped, so grab the earliest
			//VectorCopy( ent->client->unlagged.trail[k].currentAngles, ent->r.currentAngles );
			VectorCopy( ent->client->unlagged.trail[k].currentOrigin, ent->r.currentOrigin );
			VectorCopy( ent->client->unlagged.trail[k].mins, ent->r.mins );
			VectorCopy( ent->client->unlagged.trail[k].maxs, ent->r.maxs );

			if (timeshiftAnims) {
				/*
				Com_Printf("Timeshifting client %s. Old anim = %i %i (times %i %i).  New Anim = %i %i (times %i %i).\n", 
					ent->client->pers.netname,
					ent->client->ps.torsoAnim, ent->client->ps.legsAnim, ent->client->ps.torsoTimer, ent->client->ps.legsTimer, 
					ent->client->unlagged.trail[k].torsoAnim, ent->client->unlagged.trail[k].legsAnim, ent->client->unlagged.trail[k].torsoTimer, ent->client->unlagged.trail[k].legsTimer);
				*/

				ent->client->ps.torsoAnim = ent->client->unlagged.trail[k].torsoAnim;
				ent->client->ps.torsoTimer = ent->client->unlagged.trail[k].torsoTimer;
				ent->client->ps.legsAnim = ent->client->unlagged.trail[k].legsAnim;
				ent->client->ps.legsTimer = ent->client->unlagged.trail[k].legsTimer;
				ent->s.apos.trBase[YAW] = ent->client->unlagged.trail[k].realAngle;
			}

			// this will recalculate absmin and absmax
			trap->LinkEntity( (sharedEntity_t *)ent );
		}

#if 1
		if (g_unlagged.integer & (1<<3)) {
			G_DrawPlayerStick(ent, 0x00ff00, 5000, level.time);
			//Com_Printf("post angle is %.2f\n", ent->s.apos.trBase[YAW]);
		}
#endif


	}
}

/*
=====================
G_TimeShiftAllClients

Move ALL clients back to where they were at the specified "time",
except for "skip"
=====================
*/
void G_TimeShiftAllClients( int time, gentity_t *skip, qboolean timeshiftAnims ) {
	int			i;
	gentity_t	*ent;

	if (!skip->client)
		return;
	if (skip->r.svFlags & SVF_BOT)
		return;
	if (skip->s.eType == ET_NPC)
		return;

	if ( time > level.time ) {
		time = level.time;
	}

	//loda - test offset here so no more "reverse leading"
	//if (g_unlaggedOffset.integer)
		//time += g_unlaggedOffset.integer;

	// for every client
	ent = &g_entities[0];
	for ( i = 0; i < MAX_CLIENTS; i++, ent++ ) {
		if ( ent->client && ent->inuse && ent->client->sess.sessionTeam < TEAM_SPECTATOR && ent != skip ) {
			G_TimeShiftClient( ent, time, timeshiftAnims );
		}
	}
}


/*
===================
G_UnTimeShiftClient

Move a client back to where he was before the time shift
===================
*/
void G_UnTimeShiftClient( gentity_t *ent, qboolean timeshiftAnims ) {
	// if it was saved
	if ( ent->client->unlagged.saved.leveltime == level.time ) {
		// move it back
		VectorCopy( ent->client->unlagged.saved.mins, ent->r.mins );
		VectorCopy( ent->client->unlagged.saved.maxs, ent->r.maxs );
		VectorCopy( ent->client->unlagged.saved.currentOrigin, ent->r.currentOrigin );
		//VectorCopy( ent->client->unlagged.saved.currentAngles, ent->r.currentAngles );

		if (timeshiftAnims) {
			ent->client->ps.torsoAnim = ent->client->unlagged.saved.torsoAnim;
			ent->client->ps.torsoTimer = ent->client->unlagged.saved.torsoTimer;
			ent->client->ps.legsAnim = ent->client->unlagged.saved.legsAnim;
			ent->client->ps.legsTimer = ent->client->unlagged.saved.legsTimer;
			ent->s.apos.trBase[YAW] = ent->client->unlagged.saved.realAngle;
		}

		ent->client->unlagged.saved.leveltime = 0;

		// this will recalculate absmin and absmax
		trap->LinkEntity( (sharedEntity_t *)ent );
	}
}

/*
=======================
G_UnTimeShiftAllClients

Move ALL the clients back to where they were before the time shift,
except for "skip"
=======================
*/
void G_UnTimeShiftAllClients( gentity_t *skip, qboolean timeshiftAnims ) {
	int			i;
	gentity_t	*ent;

	if (!skip->client)
		return;
	if (skip->r.svFlags & SVF_BOT)
		return;
	if (skip->s.eType == ET_NPC)
		return;

	ent = &g_entities[0];
	for ( i = 0; i < MAX_CLIENTS; i++, ent++) {
		if ( ent->client && ent->inuse && ent->client->sess.sessionTeam < TEAM_SPECTATOR && ent != skip ) {
			G_UnTimeShiftClient( ent, timeshiftAnims );
		}
	}
}

void G_UpdateTrailData( void ) {
	frametime = level.time - level.previousTime;
	historytime = g_unlagged.integer;
	numTrails = NUM_CLIENT_TRAILS;
}


/*
===========================
G_PredictPlayerClipVelocity

Slide on the impacting surface
===========================
*/

#define	OVERCLIP		1.001f

void G_PredictPlayerClipVelocity( vec3_t in, vec3_t normal, vec3_t out ) {
	float	backoff;

	// find the magnitude of the vector "in" along "normal"
	backoff = DotProduct (in, normal);

	// tilt the plane a bit to avoid floating-point error issues
	if ( backoff < 0 ) {
		backoff *= OVERCLIP;
	} else {
		backoff /= OVERCLIP;
	}

	// slide along
	VectorMA( in, -backoff, normal, out );
}

/*
========================
G_PredictPlayerSlideMove

Advance the given entity frametime seconds, sliding as appropriate
========================
*/
#define	MAX_CLIP_PLANES	5

qboolean G_PredictPlayerSlideMove( gentity_t *ent, float frametime ) {
	int			bumpcount, numbumps;
	vec3_t		dir;
	float		d;
	int			numplanes;
	vec3_t		planes[MAX_CLIP_PLANES];
	vec3_t		primal_velocity, velocity, origin;
	vec3_t		clipVelocity;
	int			i, j, k;
	trace_t	trace;
	vec3_t		end;
	float		time_left;
	float		into;
	vec3_t		endVelocity;
	vec3_t		endClipVelocity;
	//vec3_t		worldUp = { 0.0f, 0.0f, 1.0f };
	
	numbumps = 4;

	VectorCopy( ent->s.pos.trDelta, primal_velocity );
	VectorCopy( primal_velocity, velocity );
	VectorCopy( ent->s.pos.trBase, origin );

	VectorCopy( velocity, endVelocity );

	time_left = frametime;

	numplanes = 0;

	for ( bumpcount = 0; bumpcount < numbumps; bumpcount++ ) {

		// calculate position we are trying to move to
		VectorMA( origin, time_left, velocity, end );

		// see if we can make it there
		JP_Trace( &trace, origin, ent->r.mins, ent->r.maxs, end, ent->s.number, ent->clipmask, qfalse, 0, 0 );

		if (trace.allsolid) {
			// entity is completely trapped in another solid
			VectorClear( velocity );
			VectorCopy( origin, ent->s.pos.trBase );
			return qtrue;
		}

		if (trace.fraction > 0) {
			// actually covered some distance
			VectorCopy( trace.endpos, origin );
		}

		if (trace.fraction == 1) {
			break;		// moved the entire distance
		}

		time_left -= time_left * trace.fraction;

		if ( numplanes >= MAX_CLIP_PLANES ) {
			// this shouldn't really happen
			VectorClear( velocity );
			VectorCopy( origin, ent->s.pos.trBase );
			return qtrue;
		}

		//
		// if this is the same plane we hit before, nudge velocity
		// out along it, which fixes some epsilon issues with
		// non-axial planes
		//
		for ( i = 0; i < numplanes; i++ ) {
			if ( DotProduct( trace.plane.normal, planes[i] ) > 0.99 ) {
				VectorAdd( trace.plane.normal, velocity, velocity );
				break;
			}
		}

		if ( i < numplanes ) {
			continue;
		}

		VectorCopy( trace.plane.normal, planes[numplanes] );
		numplanes++;

		//
		// modify velocity so it parallels all of the clip planes
		//

		// find a plane that it enters
		for ( i = 0; i < numplanes; i++ ) {
			into = DotProduct( velocity, planes[i] );
			if ( into >= 0.1 ) {
				continue;		// move doesn't interact with the plane
			}

			// slide along the plane
			G_PredictPlayerClipVelocity( velocity, planes[i], clipVelocity );

			// slide along the plane
			G_PredictPlayerClipVelocity( endVelocity, planes[i], endClipVelocity );

			// see if there is a second plane that the new move enters
			for ( j = 0; j < numplanes; j++ ) {
				if ( j == i ) {
					continue;
				}

				if ( DotProduct( clipVelocity, planes[j] ) >= 0.1 ) {
					continue;		// move doesn't interact with the plane
				}

				// try clipping the move to the plane
				G_PredictPlayerClipVelocity( clipVelocity, planes[j], clipVelocity );
				G_PredictPlayerClipVelocity( endClipVelocity, planes[j], endClipVelocity );

				// see if it goes back into the first clip plane
				if ( DotProduct( clipVelocity, planes[i] ) >= 0 ) {
					continue;
				}

				// slide the original velocity along the crease
				CrossProduct( planes[i], planes[j], dir );
				VectorNormalize( dir );
				d = DotProduct( dir, velocity );
				VectorScale( dir, d, clipVelocity );

				CrossProduct( planes[i], planes[j], dir );
				VectorNormalize( dir );
				d = DotProduct( dir, endVelocity );
				VectorScale( dir, d, endClipVelocity );

				// see if there is a third plane the the new move enters
				for ( k = 0; k < numplanes; k++ ) {
					if ( k == i || k == j ) {
						continue;
					}

					if ( DotProduct( clipVelocity, planes[k] ) >= 0.1 ) {
						continue;		// move doesn't interact with the plane
					}

					// stop dead at a tripple plane interaction
					VectorClear( velocity );
					VectorCopy( origin, ent->s.pos.trBase );
					return qtrue;
				}
			}

			// if we have fixed all interactions, try another move
			VectorCopy( clipVelocity, velocity );
			VectorCopy( endClipVelocity, endVelocity );
			break;
		}
	}

	VectorCopy( endVelocity, velocity );
	VectorCopy( origin, ent->s.pos.trBase );

	return (bumpcount != 0);
}

/*
============================
G_PredictPlayerStepSlideMove

Advance the given entity frametime seconds, stepping and sliding as appropriate
============================
*/
#define	STEPSIZE 18

void G_PredictPlayerStepSlideMove( gentity_t *ent, float frametime ) {
	vec3_t start_o, start_v, down_o, down_v;
	vec3_t down, up;
	trace_t trace;
	float stepSize;
	int NEW_STEPSIZE = STEPSIZE;

	if (ent->client && (ent->client->sess.movementStyle == 3 || ent->client->sess.movementStyle == 4 || ent->client->sess.movementStyle == 6 || ent->client->sess.movementStyle == 7 || ent->client->sess.movementStyle == 8)) {
		if (ent->client->ps.velocity[2] > 0 && ent->client->pers.cmd.upmove > 0) {
			int jumpHeight = ent->client->ps.origin[2] - ent->client->ps.fd.forceJumpZStart;
			//NEW_STEPSIZE = 46;

			if (jumpHeight > 48)
				jumpHeight = 48;
			else if (jumpHeight < 22)
				jumpHeight = 22;

			NEW_STEPSIZE = 48 - jumpHeight + 22;
		}
		else 
			NEW_STEPSIZE = 22;
	}

	VectorCopy (ent->s.pos.trBase, start_o);
	VectorCopy (ent->s.pos.trDelta, start_v);

	if ( !G_PredictPlayerSlideMove( ent, frametime ) ) {
		// not clipped, so forget stepping
		return;
	}

	VectorCopy( ent->s.pos.trBase, down_o);
	VectorCopy( ent->s.pos.trDelta, down_v);

	VectorCopy (start_o, up);
	up[2] += NEW_STEPSIZE;

	// test the player position if they were a stepheight higher
	JP_Trace( &trace, start_o, ent->r.mins, ent->r.maxs, up, ent->s.number, ent->clipmask, qfalse, 0, 0);
	if ( trace.allsolid ) {
		return;		// can't step up
	}

	stepSize = trace.endpos[2] - start_o[2];

	// try slidemove from this position
	VectorCopy( trace.endpos, ent->s.pos.trBase );
	VectorCopy( start_v, ent->s.pos.trDelta );

	G_PredictPlayerSlideMove( ent, frametime );

	// push down the final amount
	VectorCopy( ent->s.pos.trBase, down );
	down[2] -= stepSize;
	JP_Trace( &trace, ent->s.pos.trBase, ent->r.mins, ent->r.maxs, down, ent->s.number, ent->clipmask, qfalse, 0, 0 );
	if ( !trace.allsolid ) {
		VectorCopy( trace.endpos, ent->s.pos.trBase );
	}
	if ( trace.fraction < 1.0 ) {
		G_PredictPlayerClipVelocity( ent->s.pos.trDelta, trace.plane.normal, ent->s.pos.trDelta );
	}
}











/*
===============
G_DamageFeedback

Called just before a snapshot is sent to the given player.
Totals up all damage and generates both the playerState_t
damage values to that client for pain blends and kicks, and
global pain sound events for all clients.
===============
*/
void P_DamageFeedback( gentity_t *player ) {
	gclient_t	*client;
	float	count;
	vec3_t	angles;

	client = player->client;
	if ( client->ps.pm_type == PM_DEAD ) {
		return;
	}

	// total points of damage shot at the player this frame
	count = client->damage_blood + client->damage_armor;
	if ( count == 0 ) {
		return;		// didn't take any damage
	}

	if ( count > 255 ) {
		count = 255;
	}

	// send the information to the client

	// world damage (falling, slime, etc) uses a special code
	// to make the blend blob centered instead of positional
	if ( client->damage_fromWorld ) {
		client->ps.damagePitch = 255;
		client->ps.damageYaw = 255;

		client->damage_fromWorld = qfalse;
	} else {
		vectoangles( client->damage_from, angles );
		client->ps.damagePitch = angles[PITCH]/360.0 * 256;
		client->ps.damageYaw = angles[YAW]/360.0 * 256;

		//cap them since we can't send negative values in here across the net
		if (client->ps.damagePitch < 0)
		{
			client->ps.damagePitch = 0;
		}
		if (client->ps.damageYaw < 0)
		{
			client->ps.damageYaw = 0;
		}
	}

	// play an appropriate pain sound
	if ( (level.time > player->pain_debounce_time) && !(player->flags & FL_GODMODE) && !(player->s.eFlags & EF_DEAD) ) {

		// don't do more than two pain sounds a second
		// nmckenzie: also don't make him loud and whiny if he's only getting nicked.
		if ( level.time - client->ps.painTime < 500 || count < 10) {
			return;
		}
		P_SetTwitchInfo(client);
		player->pain_debounce_time = level.time + 700;
		
		if (g_stopHealthESP.integer == 1)
			G_AddEvent( player, EV_PAIN, 50 ); //anti ESP here?
		else if (g_stopHealthESP.integer > 1) 
		{
			char* pain;

			if (player->health <= 25)
				pain = "*pain25.wav";
			else if (player->health <= 50)
				pain = "*pain50.wav";
			else if (player->health <= 75)
				pain = "*pain75.wav";
			else
				pain = "*pain100.wav";
			
			G_EntitySound(player, CHAN_VOICE, G_SoundIndex(pain));
		}
		else
			G_AddEvent( player, EV_PAIN, player->health ); //anti ESP here?
		client->ps.damageEvent++;

		if (client->damage_armor && !client->damage_blood)
		{
			client->ps.damageType = 1; //pure shields
		}
		else if (client->damage_armor)
		{
			client->ps.damageType = 2; //shields and health
		}
		else
		{
			client->ps.damageType = 0; //pure health
		}
	}


	client->ps.damageCount = count;

	//
	// clear totals
	//
	client->damage_blood = 0;
	client->damage_armor = 0;
	client->damage_knockback = 0;
}



/*
=============
P_WorldEffects

Check for lava / slime contents and drowning
=============
*/
void P_WorldEffects( gentity_t *ent ) {
#ifdef BASE_COMPAT
	qboolean	envirosuit = qfalse;
#endif
	int			waterlevel;

	if ( ent->client->noclip ) {
		ent->client->airOutTime = level.time + 12000;	// don't need air
		return;
	}

	waterlevel = ent->waterlevel;

	#ifdef BASE_COMPAT
		envirosuit = ent->client->ps.powerups[PW_BATTLESUIT] > level.time;
	#endif // BASE_COMPAT

	//
	// check for drowning
	//
	if ( waterlevel == 3 ) {
		#ifdef BASE_COMPAT
			// envirosuit give air
			if ( envirosuit )
				ent->client->airOutTime = level.time + 10000;
		#endif // BASE_COMPAT

		// if out of air, start drowning
		if ( ent->client->airOutTime < level.time) {
			// drown!
			ent->client->airOutTime += 1000;
			if ( ent->health > 0 ) {
				// take more damage the longer underwater
				ent->damage += 2;
				if (ent->damage > 15)
					ent->damage = 15;

				// play a gurp sound instead of a normal pain sound
				if (ent->health <= ent->damage) {
					G_Sound(ent, CHAN_VOICE, G_SoundIndex(/*"*drown.wav"*/"sound/player/gurp1.wav"));
				} else if (rand()&1) {
					G_Sound(ent, CHAN_VOICE, G_SoundIndex("sound/player/gurp1.wav"));
				} else {
					G_Sound(ent, CHAN_VOICE, G_SoundIndex("sound/player/gurp2.wav"));
				}

				// don't play a normal pain sound
				ent->pain_debounce_time = level.time + 200;

				G_Damage (ent, NULL, NULL, NULL, NULL, 
					ent->damage, DAMAGE_NO_ARMOR, MOD_WATER);
			}
		}
	} else {
		ent->client->airOutTime = level.time + 12000;
		ent->damage = 2;
	}

	//
	// check for sizzle damage (move to pmove?)
	//
	if ( waterlevel && (ent->watertype & (CONTENTS_LAVA|CONTENTS_SLIME)) )
	{
		if ( ent->health > 0 && ent->pain_debounce_time <= level.time )
		{
		#ifdef BASE_COMPAT
			if ( envirosuit )
				G_AddEvent( ent, EV_POWERUP_BATTLESUIT, 0 );
			else
		#endif
			{
				if ( ent->watertype & CONTENTS_LAVA )
					G_Damage( ent, NULL, NULL, NULL, NULL, 30*waterlevel, 0, MOD_LAVA );

				if ( ent->watertype & CONTENTS_SLIME )
					G_Damage( ent, NULL, NULL, NULL, NULL, 10*waterlevel, 0, MOD_SLIME );
			}
		}
	}
}





//==============================================================
extern void G_ApplyKnockback( gentity_t *targ, vec3_t newDir, float knockback );
void DoImpact( gentity_t *self, gentity_t *other, qboolean damageSelf )
{
	float magnitude, my_mass;
	vec3_t	velocity;
	int cont;
	qboolean easyBreakBrush = qtrue;

	if( self->client )
	{
		VectorCopy( self->client->ps.velocity, velocity );
		if( !self->mass )
		{
			my_mass = 10;
		}
		else
		{
			my_mass = self->mass;
		}
	}
	else 
	{
		VectorCopy( self->s.pos.trDelta, velocity );
		if ( self->s.pos.trType == TR_GRAVITY )
		{
			velocity[2] -= 0.25f * g_gravity.value;
		}
		if( !self->mass )
		{
			my_mass = 1;
		}
		else if ( self->mass <= 10 )
		{
			my_mass = 10;
		}
		else
		{
			my_mass = self->mass;///10;
		}
	}

	magnitude = VectorLength( velocity ) * my_mass / 10;

	/*
	if(pointcontents(self.absmax)==CONTENT_WATER)//FIXME: or other watertypes
		magnitude/=3;							//water absorbs 2/3 velocity

	if(self.classname=="barrel"&&self.aflag)//rolling barrels are made for impacts!
		magnitude*=3;

	if(self.frozen>0&&magnitude<300&&self.flags&FL_ONGROUND&&loser==world&&self.velocity_z<-20&&self.last_onground+0.3<time)
		magnitude=300;
	*/
	if ( other->material == MAT_GLASS 
		|| other->material == MAT_GLASS_METAL 
		|| other->material == MAT_GRATE1
		|| ((other->flags&FL_BBRUSH)&&(other->spawnflags&8/*THIN*/))
		|| (other->r.svFlags&SVF_GLASS_BRUSH) )
	{
		easyBreakBrush = qtrue;
	}

	if ( !self->client || self->client->ps.lastOnGround+300<level.time || ( self->client->ps.lastOnGround+100 < level.time && easyBreakBrush ) )
	{
		vec3_t dir1, dir2;
		float force = 0, dot;

		if ( easyBreakBrush )
			magnitude *= 2;

		//damage them
		if ( magnitude >= 100 && other->s.number < ENTITYNUM_WORLD )
		{
			VectorCopy( velocity, dir1 );
			VectorNormalize( dir1 );
			if( VectorCompare( other->r.currentOrigin, vec3_origin ) )
			{//a brush with no origin
				VectorCopy ( dir1, dir2 );
			}
			else
			{
				VectorSubtract( other->r.currentOrigin, self->r.currentOrigin, dir2 );
				VectorNormalize( dir2 );
			}

			dot = DotProduct( dir1, dir2 );

			if ( dot >= 0.2 )
			{
				force = dot;
			}
			else
			{
				force = 0;
			}

			force *= (magnitude/50);

			cont = trap->PointContents( other->r.absmax, other->s.number );
			if( (cont&CONTENTS_WATER) )//|| (self.classname=="barrel"&&self.aflag))//FIXME: or other watertypes
			{
				force /= 3;							//water absorbs 2/3 velocity
			}

			/*
			if(self.frozen>0&&force>10)
				force=10;
			*/

			if( ( force >= 1 && other->s.number >= MAX_CLIENTS ) || force >= 10)
			{
	/*			
				dprint("Damage other (");
				dprint(loser.classname);
				dprint("): ");
				dprint(ftos(force));
				dprint("\n");
	*/
				if ( other->r.svFlags & SVF_GLASS_BRUSH )
				{
					other->splashRadius = (float)(self->r.maxs[0] - self->r.mins[0])/4.0f;
				}
				if ( other->takedamage )
				{
					G_Damage( other, self, self, velocity, self->r.currentOrigin, force, DAMAGE_NO_ARMOR, MOD_CRUSH);//FIXME: MOD_IMPACT
				}
				else
				{
					G_ApplyKnockback( other, dir2, force );
				}
			}
		}

		if ( damageSelf && self->takedamage )
		{
			//Now damage me
			//FIXME: more lenient falling damage, especially for when driving a vehicle
			if ( self->client && self->client->ps.fd.forceJumpZStart )
			{//we were force-jumping
				if ( self->r.currentOrigin[2] >= self->client->ps.fd.forceJumpZStart )
				{//we landed at same height or higher than we landed
					magnitude = 0;
				}
				else
				{//FIXME: take off some of it, at least?
					magnitude = (self->client->ps.fd.forceJumpZStart-self->r.currentOrigin[2])/3;
				}
			}
			//if(self.classname!="monster_mezzoman"&&self.netname!="spider")//Cats always land on their feet
				if( ( magnitude >= 100 + self->health && self->s.number >= MAX_CLIENTS && self->s.weapon != WP_SABER ) || ( magnitude >= 700 ) )//&& self.safe_time < level.time ))//health here is used to simulate structural integrity
				{
					if ( (self->s.weapon == WP_SABER) && self->client && self->client->ps.groundEntityNum < ENTITYNUM_NONE && magnitude < 1000 )
					{//players and jedi take less impact damage
						//allow for some lenience on high falls
						magnitude /= 2;
						/*
						if ( self.absorb_time >= time )//crouching on impact absorbs 1/2 the damage
						{
							magnitude/=2;
						}
						*/
					}
					magnitude /= 40;
					magnitude = magnitude - force/2;//If damage other, subtract half of that damage off of own injury
					if ( magnitude >= 1 )
					{
		//FIXME: Put in a thingtype impact sound function
		/*					
						dprint("Damage self (");
						dprint(self.classname);
						dprint("): ");
						dprint(ftos(magnitude));
						dprint("\n");
		*/
						/*
						if ( self.classname=="player_sheep "&& self.flags&FL_ONGROUND && self.velocity_z > -50 )
							return;
						*/
						G_Damage( self, NULL, NULL, NULL, self->r.currentOrigin, magnitude/2, DAMAGE_NO_ARMOR, MOD_FALLING );//FIXME: MOD_IMPACT
					}
				}
		}

		//FIXME: slow my velocity some?

		// NOTENOTE We don't use lastimpact as of yet
//		self->lastImpact = level.time;

		/*
		if(self.flags&FL_ONGROUND)
			self.last_onground=time;
		*/
	}
}

void Client_CheckImpactBBrush( gentity_t *self, gentity_t *other )
{
	if ( !other || !other->inuse )
	{
		return;
	}
	if (!self || !self->inuse || !self->client ||
		self->client->tempSpectate >= level.time ||
		self->client->sess.sessionTeam == TEAM_SPECTATOR)
	{ //hmm.. let's not let spectators ram into breakables.
		return;
	}

	/*
	if (BG_InSpecialJump(self->client->ps.legsAnim))
	{ //don't do this either, qa says it creates "balance issues"
		return;
	}
	*/

	if ( other->material == MAT_GLASS 
		|| other->material == MAT_GLASS_METAL 
		|| other->material == MAT_GRATE1
		|| ((other->flags&FL_BBRUSH)&&(other->spawnflags&8/*THIN*/))
		|| ((other->flags&FL_BBRUSH)&&(other->health<=10))
		|| (other->r.svFlags&SVF_GLASS_BRUSH) )
	{//clients only do impact damage against easy-break breakables
		DoImpact( self, other, qfalse );
	}
}


/*
===============
G_SetClientSound
===============
*/
void G_SetClientSound( gentity_t *ent ) {
	if (ent->client && ent->client->isHacking)
	{ //loop hacking sound
		ent->client->ps.loopSound = level.snd_hack;
		ent->s.loopIsSoundset = qfalse;
	}
	else if (ent->client && ent->client->isMedHealed > level.time)
	{ //loop healing sound
		ent->client->ps.loopSound = level.snd_medHealed;
		ent->s.loopIsSoundset = qfalse;
	}
	else if (ent->client && ent->client->isMedSupplied > level.time)
	{ //loop supplying sound
		ent->client->ps.loopSound = level.snd_medSupplied;
		ent->s.loopIsSoundset = qfalse;
	}
	else if (ent->client && ent->waterlevel && (ent->watertype&(CONTENTS_LAVA|CONTENTS_SLIME)) ) {
		ent->client->ps.loopSound = level.snd_fry;
		ent->s.loopIsSoundset = qfalse;
	} else if (ent->client) {
		ent->client->ps.loopSound = 0;
		ent->s.loopIsSoundset = qfalse;
	} else {
		ent->s.loopSound = 0;
		ent->s.loopIsSoundset = qfalse;
	}
}



//==============================================================

/*
==============
ClientImpacts
==============
*/
void ClientImpacts( gentity_t *ent, pmove_t *pmove ) {
	int		i, j;
	trace_t	trace;
	gentity_t	*other;

	memset( &trace, 0, sizeof( trace ) );
	for (i=0 ; i<pmove->numtouch ; i++) {
		for (j=0 ; j<i ; j++) {
			if (pmove->touchents[j] == pmove->touchents[i] ) {
				break;
			}
		}
		if (j != i) {
			continue;	// duplicated
		}
		other = &g_entities[ pmove->touchents[i] ];

		if ( ( ent->r.svFlags & SVF_BOT ) && ( ent->touch ) ) {
			ent->touch( ent, other, &trace );
		}

		if ( !other->touch ) {
			continue;
		}

		other->touch( other, ent, &trace );
	}

}

/*
============
G_TouchTriggersLerped

Find all trigger entities that ent's current position touches.
Spectators will only interact with teleporters.

This version checks at 6 unit steps between last and current origins
============
*/
/*
extern void G_TestLine(vec3_t start, vec3_t end, int color, int time);
void G_TouchTriggersLerped( gentity_t *ent ) {
	int			i, num, touch[MAX_GENTITIES];
	float		dist, curDist = 0;
	gentity_t	*hit;
	trace_t		trace;
	vec3_t		end, mins, maxs, diff;
	const vec3_t	range = { 40, 40, 52 };
	qboolean	touched[MAX_GENTITIES];

	//trap->Print("Checking lerped trigger\n");

	if ((ent->client->oldFlags ^ ent->client->ps.eFlags ) & EF_TELEPORT_BIT)
		return;
	if (ent->client->ps.pm_type != PM_NORMAL && ent->client->ps.pm_type != PM_JETPACK && ent->client->ps.pm_type != PM_FLOAT)
		return;

	VectorSubtract( ent->client->ps.origin, ent->client->oldOrigin, diff );
	dist = VectorNormalize( diff );

	if (dist > 1024)
		return;
	memset (touched, qfalse, sizeof(touched));

	for (curDist = 0; curDist  < dist; curDist += 24.0f) {
		VectorMA( ent->client->oldOrigin, curDist, diff, end );
	
		{
			vec3_t		end2;
			VectorCopy(end, end2);
			end2[2] += 128;
			G_TestLine(end, end2, 0x00000ff, 8000);
		}
			
		VectorSubtract( end, range, mins ); //tha fuck is this
		VectorAdd( end, range, maxs );

		num = trap->EntitiesInBox(mins, maxs, touch, MAX_GENTITIES);

		VectorAdd( end, ent->r.mins, mins ); //tha fuck is this
		VectorAdd( end, ent->r.maxs, maxs );

		for ( i=0 ; i<num ; i++ ) {
			hit = &g_entities[touch[i]];

			if ((!hit->touch) && (!ent->touch)) //why ent->touch ?
				continue;
			if (!(hit->r.contents & CONTENTS_TRIGGER))
				continue;
			if (touched[i] == qtrue) //already touched this move
				continue;
			if (Q_stricmp(hit->classname, "trigger_teleport") && Q_stricmp(hit->classname, "trigger_multiple") && Q_stricmp(hit->classname, "df_trigger_checkpoint"))//Not teleport, or multiple trigger, or checkpoint
				continue;
			if (!trap->EntityContact( mins, maxs, (sharedEntity_t *)hit, qfalse))
				continue;

			touched[i] = qtrue;

			memset( &trace, 0, sizeof(trace) );

			if (hit->touch) {
				hit->touch (hit, ent, &trace);
				//trap->Print("Using a lerped trigger!\n");
				//trap->SendServerCommand( -1, "print \"Using lerped trigger\n\"");
			}
		}
	}

}
*/

//extern void G_TestLine(vec3_t start, vec3_t end, int color, int time);
void G_TouchTriggersWithTrace( gentity_t *ent ) {
	static vec3_t	playerMins = {-15, -15, DEFAULT_MINS_2};
	static vec3_t	playerMaxs = {15, 15, DEFAULT_MAXS_2};
	vec3_t			diff;
	trace_t		trace;

	if ((ent->client->oldFlags ^ ent->client->ps.eFlags ) & EF_TELEPORT_BIT)
		return;
	if (ent->client->ps.pm_type != PM_NORMAL && ent->client->ps.pm_type != PM_JETPACK && ent->client->ps.pm_type != PM_FLOAT)
		return;

	VectorSubtract( ent->client->ps.origin, ent->client->oldOrigin, diff );
	if (VectorLengthSquared(diff) > 512*512) //sanity check i guess
		return;

	trap->Trace( &trace, ent->client->ps.origin, playerMins, playerMaxs, ent->client->oldOrigin, ent->client->ps.clientNum, CONTENTS_TRIGGER, qfalse, 0, 0 );
	//G_TestLine(ent->client->ps.origin, ent->client->oldOrigin, 0x00000ff, 5000);
		
	if (trace.allsolid) //We are actually inside the trigger, so lets assume we already checked it..
		return;
	if (trace.fraction == 1) //Did the entire trace without hitting any triggers
		 return;

	if (trace.fraction > 0) {
		gentity_t	*hit;

		hit = &g_entities[trace.entityNum];

		if (!hit->touch) //No point if the trigger doesnt do anything if we touch it.
			return;
		/*
		if (Q_stricmp(hit->classname, "trigger_teleport") && 
			Q_stricmp(hit->classname, "trigger_multiple") && 
			Q_stricmp(hit->classname, "df_trigger_checkpoint") && 
			Q_stricmp(hit->classname, "df_trigger_start") && 
			Q_stricmp(hit->classname, "df_trigger_finish"))//Not teleport, or multiple trigger, or checkpoint
			return;
		*/

		//trap->Print("Trace trigger touch! time: %i\n", trap->Milliseconds());

		//memset( &trace2, 0, sizeof(trace2) );
		hit->touch (hit, ent, NULL);
	}
}


/*
============
G_TouchTriggers

Find all trigger entities that ent's current position touches.
Spectators will only interact with teleporters.
============
*/
void	G_TouchTriggers( gentity_t *ent ) {
	int			i, num;
	int			touch[MAX_GENTITIES];
	gentity_t	*hit;
	trace_t		trace;
	vec3_t		mins, maxs;
	static vec3_t	range = { 40, 40, 52 };

	if ( !ent->client ) {
		return;
	}

	// dead clients don't activate triggers!
	if ( ent->client->ps.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	VectorSubtract( ent->client->ps.origin, range, mins );
	VectorAdd( ent->client->ps.origin, range, maxs );

	num = trap->EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	// can't use ent->r.absmin, because that has a one unit pad
	VectorAdd( ent->client->ps.origin, ent->r.mins, mins );
	VectorAdd( ent->client->ps.origin, ent->r.maxs, maxs );

	for ( i=0 ; i<num ; i++ ) {
		hit = &g_entities[touch[i]];

		if ( !hit->touch && !ent->touch ) {
			continue;
		}
		if ( !( hit->r.contents & CONTENTS_TRIGGER ) ) {
			continue;
		}

		// ignore most entities if a spectator
		if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
			if ( hit->s.eType != ET_TELEPORT_TRIGGER &&
				// this is ugly but adding a new ET_? type will
				// most likely cause network incompatibilities
				hit->touch != Touch_DoorTrigger) {
				continue;
			}
		}

		// use seperate code for determining if an item is picked up
		// so you don't have to actually contact its bounding box
		if ( hit->s.eType == ET_ITEM ) {
			if ( !BG_PlayerTouchesItem( &ent->client->ps, &hit->s, level.time ) ) {
				continue;
			}
		} else {
			if ( !trap->EntityContact( mins, maxs, (sharedEntity_t *)hit, qfalse ) ) {
				continue;
			}
		}

		memset( &trace, 0, sizeof(trace) );

		if ( hit->touch ) {
			hit->touch (hit, ent, &trace);
		}

		if ( ( ent->r.svFlags & SVF_BOT ) && ( ent->touch ) ) {
			ent->touch( ent, hit, &trace );
		}
	}

	// if we didn't touch a jump pad this pmove frame
	if ( ent->client->ps.jumppad_frame != ent->client->ps.pmove_framecount ) {
		ent->client->ps.jumppad_frame = 0;
		ent->client->ps.jumppad_ent = 0;
	}
}


/*
============
G_MoverTouchTriggers

Find all trigger entities that ent's current position touches.
Spectators will only interact with teleporters.
============
*/
void G_MoverTouchPushTriggers( gentity_t *ent, vec3_t oldOrg ) 
{
	int			i, num;
	float		step, stepSize, dist;
	int			touch[MAX_GENTITIES];
	gentity_t	*hit;
	trace_t		trace;
	vec3_t		mins, maxs, dir, size, checkSpot;
	const vec3_t	range = { 40, 40, 52 };

	// non-moving movers don't hit triggers!
	if ( !VectorLengthSquared( ent->s.pos.trDelta ) ) 
	{
		return;
	}

	VectorSubtract( ent->r.mins, ent->r.maxs, size );
	stepSize = VectorLength( size );
	if ( stepSize < 1 )
	{
		stepSize = 1;
	}

	VectorSubtract( ent->r.currentOrigin, oldOrg, dir );
	dist = VectorNormalize( dir );
	for ( step = 0; step <= dist; step += stepSize )
	{
		VectorMA( ent->r.currentOrigin, step, dir, checkSpot );
		VectorSubtract( checkSpot, range, mins );
		VectorAdd( checkSpot, range, maxs );

		num = trap->EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

		// can't use ent->r.absmin, because that has a one unit pad
		VectorAdd( checkSpot, ent->r.mins, mins );
		VectorAdd( checkSpot, ent->r.maxs, maxs );

		for ( i=0 ; i<num ; i++ ) 
		{
			hit = &g_entities[touch[i]];

			if ( hit->s.eType != ET_PUSH_TRIGGER )
			{
				continue;
			}

			if ( hit->touch == NULL ) 
			{
				continue;
			}

			if ( !( hit->r.contents & CONTENTS_TRIGGER ) ) 
			{
				continue;
			}


			if ( !trap->EntityContact( mins, maxs, (sharedEntity_t *)hit, qfalse ) ) 
			{
				continue;
			}

			memset( &trace, 0, sizeof(trace) );

			if ( hit->touch != NULL ) 
			{
				hit->touch(hit, ent, &trace);
			}
		}
	}
}

static void SV_PMTrace( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask ) {
	JP_Trace( results, start, mins, maxs, end, passEntityNum, contentMask, qfalse, 0, 10 ); //JAPRO loda, change this to ghoul2 for g2 player collision hitbox, could be useful for hockey maybe?
}

int SpectatorFind(gentity_t *self)
{
	int i;

	for (i = 0; i < level.numConnectedClients; i ++) {
		gentity_t *ent = &g_entities[level.sortedClients[i]];
		float	  dist;
		vec3_t	  angles;
		trace_t		tr;

		if (ent == self)
			continue;
		if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
			continue;

		//VectorSubtract( ent->client->ps.origin, self->client->ps.origin, angles ); //Changed because only r.curentOrigin is unlagged
		VectorSubtract( ent->r.currentOrigin, self->client->ps.origin, angles );
		dist = VectorLengthSquared ( angles );
		vectoangles ( angles, angles );

		if (dist > 8192*8192) // Out of range
			continue;
		if (!InFieldOfVision(self->client->ps.viewangles, 30, angles)) // Not in our FOV
			continue;

		//JP_Trace( &tr, self->client->ps.origin, NULL, NULL, ent->client->ps.origin, self->s.number, MASK_SOLID, qfalse, 0, 0 );
		JP_Trace( &tr, self->client->ps.origin, NULL, NULL, ent->r.currentOrigin, self->s.number, MASK_SOLID, qfalse, 0, 0 );
		if (tr.fraction != 1.0) //if not line of sight
			continue;

		// Return the first guy that fits the requirements
		return level.sortedClients[i];
	}
	return -1;
}

/*
=================
SpectatorThink
=================
*/
void SpectatorThink( gentity_t *ent, usercmd_t *ucmd ) {
	pmove_t	pmove;
	gclient_t	*client;
	int match;

	client = ent->client;

	if ( client->sess.spectatorState != SPECTATOR_FOLLOW ) {
		client->ps.pm_type = PM_SPECTATOR;
		client->ps.speed = 400;	// faster than normal
		client->ps.basespeed = 400;

		////OSP: pause
		//if ( level.pause.state != PAUSE_NONE ) //actually why should spectators be frozen in pause?
		//	client->ps.pm_type = PM_FREEZE;

		//hmm, shouldn't have an anim if you're a spectator, make sure
		//it gets cleared.
		client->ps.legsAnim = 0;
		client->ps.legsTimer = 0;
		client->ps.torsoAnim = 0;
		client->ps.torsoTimer = 0;

		//client->ps.fd.forcePowersActive |= ( 1 << FP_SEE ); //spectator force sight wallhack
		//client->ps.fd.forcePowerLevel[FP_SEE] = 2;

		// set up for pmove
		memset (&pmove, 0, sizeof(pmove));
		pmove.ps = &client->ps;
		pmove.cmd = *ucmd;
		pmove.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;	// spectators can fly through bodies
		pmove.trace = SV_PMTrace;
		pmove.pointcontents = trap->PointContents;

		pmove.noSpecMove = g_noSpecMove.integer;

		pmove.animations = NULL;
		pmove.nonHumanoid = qfalse;

		//Set up bg entity data
		pmove.baseEnt = (bgEntity_t *)g_entities;
		pmove.entSize = sizeof(gentity_t);

		// perform a pmove
		Pmove (&pmove);
		// save results of pmove
		VectorCopy( client->ps.origin, ent->s.origin );

		if (ent->client->tempSpectate < level.time)
		{
			G_TouchTriggers( ent );
		}
		trap->UnlinkEntity( (sharedEntity_t *)ent );
	}

	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;

	if (client->tempSpectate < level.time)
	{
		if ( client->sess.spectatorState == SPECTATOR_FOLLOW && (client->buttons & BUTTON_ATTACK) && !(client->oldbuttons & BUTTON_ATTACK) )
			Cmd_FollowCycle_f( ent, 1 ); // Clicked while following a guy -> go to next guy
		else if ( (client->buttons & BUTTON_ATTACK) && !(client->oldbuttons & BUTTON_ATTACK) ) {
			G_TimeShiftAllClients( ent->client->pers.cmd.serverTime, ent, qfalse );
			match = SpectatorFind(ent);
			G_UnTimeShiftAllClients( ent, qfalse );
			if (match == -1) // Clicked while not following a guy -> Follow guy you are aiming at.
				Cmd_FollowCycle_f( ent, 1 );
			else {
				client->sess.spectatorClient = match;
				client->sess.spectatorState = SPECTATOR_FOLLOW;
			}
		}
		else if ( client->sess.spectatorState == SPECTATOR_FOLLOW && (client->buttons & BUTTON_ALT_ATTACK) && !(client->oldbuttons & BUTTON_ALT_ATTACK) )
			Cmd_FollowCycle_f( ent, -1 );

		/*
		if (client->sess.spectatorState == SPECTATOR_FOLLOW && (ucmd->upmove > 0))
		{ //jump now removes you from follow mode
			StopFollowing(ent);
		}
		*/

		if (client->sess.spectatorState == SPECTATOR_FOLLOW) {
			if (ucmd->upmove > 0)
				StopFollowing(ent);
			else {
				gentity_t *other;
				other = &g_entities[client->sess.spectatorClient];
				if (other && other->client) {
					if (g_allowNoFollow.integer && other->client->pers.noFollow) {
						if (ent->client->sess.fullAdmin) {
							if (!(g_fullAdminLevel.integer & (1 << A_SEEHIDDEN)))
								StopFollowing(ent);
						}
						else if (ent->client->sess.juniorAdmin) {
							if (!(g_juniorAdminLevel.integer & (1 << A_SEEHIDDEN)))
								StopFollowing(ent);
						}
						else
							StopFollowing(ent);
					}
				}
				else 
					StopFollowing(ent);
			}
		}
	}
}

/*
=================
ClientInactivityTimer

Returns qfalse if the client is dropped
=================
*/
qboolean ClientInactivityTimer( gclient_t *client ) {
	if ( ! g_inactivity.integer ) {
		// give everyone some time, so if the operator sets g_inactivity during
		// gameplay, everyone isn't kicked
		client->inactivityTime = level.time + 60 * 1000;
		client->inactivityWarning = qfalse;
	} else if ( client->pers.cmd.forwardmove || 
		client->pers.cmd.rightmove || 
		client->pers.cmd.upmove ||
		(client->pers.cmd.buttons & (BUTTON_ATTACK|BUTTON_ALT_ATTACK)) ) {
		client->inactivityTime = level.time + g_inactivity.integer * 1000;
		client->inactivityWarning = qfalse;
	} else if ( !client->pers.localClient ) {
		if ( level.time > client->inactivityTime ) {
			trap->DropClient( client - level.clients, "Dropped due to inactivity" );
			return qfalse;
		}
		if ( level.time > client->inactivityTime - 10000 && !client->inactivityWarning ) {
			client->inactivityWarning = qtrue;
			trap->SendServerCommand( client - level.clients, "cp \"Ten seconds until inactivity drop!\n\"" );
		}
	}
	return qtrue;
}

/*
==================
ClientTimerActions

Actions that happen once a second
==================
*/
void ClientTimerActions( gentity_t *ent, int msec ) {
	gclient_t	*client;

	client = ent->client;
	client->timeResidual += msec;

	while ( client->timeResidual >= 1000 ) 
	{
		client->timeResidual -= 1000;

		// count down health when over max
		if ( ent->health > client->ps.stats[STAT_MAX_HEALTH] ) {
			ent->health--;
		}

		// count down armor when over max
		if ( client->ps.stats[STAT_ARMOR] > client->ps.stats[STAT_MAX_HEALTH] ) {
			client->ps.stats[STAT_ARMOR]--;
		}

		if (client->csTimeLeft) {//japro send message and count down a second
			trap->SendServerCommand( ent-g_entities, va("cp \"^7%s\n\"", ent->client->csMessage ));
			client->csTimeLeft--;
		}
	}

	client->timeResidualBig += msec;

	while ( client->timeResidualBig >= 5000 ) 
	{
		client->timeResidualBig -= 5000;

		if ((g_rabbit.integer == 3) && client->ps.powerups[PW_NEUTRALFLAG]) {
			AddScore( ent, ent->r.currentOrigin, 1 );
		}
	}
}

/*
====================
ClientIntermissionThink
====================
*/
void ClientIntermissionThink( gclient_t *client ) {
	client->ps.eFlags &= ~EF_TALK;
	client->ps.eFlags &= ~EF_FIRING;

	// the level will exit when everyone wants to or after timeouts

	// swap and latch button actions
	client->oldbuttons = client->buttons;
	client->buttons = client->pers.cmd.buttons;
	if ( client->buttons & ( BUTTON_ATTACK | BUTTON_USE_HOLDABLE ) & ( client->oldbuttons ^ client->buttons ) ) {
		// this used to be an ^1 but once a player says ready, it should stick
		client->readyToExit = qtrue;
	}
}

extern void NPC_SetAnim(gentity_t	*ent,int setAnimParts,int anim,int setAnimFlags);
void G_VehicleAttachDroidUnit( gentity_t *vehEnt )
{
	if ( vehEnt && vehEnt->m_pVehicle && vehEnt->m_pVehicle->m_pDroidUnit != NULL )
	{
		gentity_t *droidEnt = (gentity_t *)vehEnt->m_pVehicle->m_pDroidUnit;
		mdxaBone_t boltMatrix;
		vec3_t	fwd;

		trap->G2API_GetBoltMatrix(vehEnt->ghoul2, 0, vehEnt->m_pVehicle->m_iDroidUnitTag, &boltMatrix, vehEnt->r.currentAngles, vehEnt->r.currentOrigin, level.time,
			NULL, vehEnt->modelScale);
		BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, droidEnt->r.currentOrigin);
		BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, fwd);
		vectoangles( fwd, droidEnt->r.currentAngles );
		
		if ( droidEnt->client )
		{
			VectorCopy( droidEnt->r.currentAngles, droidEnt->client->ps.viewangles );
			VectorCopy( droidEnt->r.currentOrigin, droidEnt->client->ps.origin );
		}

		G_SetOrigin( droidEnt, droidEnt->r.currentOrigin );
		trap->LinkEntity( (sharedEntity_t *)droidEnt );
		
		if ( droidEnt->NPC )
		{
			NPC_SetAnim( droidEnt, SETANIM_BOTH, BOTH_STAND2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		}
	}
}

//called gameside only from pmove code (convenience)
extern qboolean BG_SabersOff( playerState_t *ps );
void G_CheapWeaponFire(int entNum, int ev)
{
	gentity_t *ent = &g_entities[entNum];
	
	if (!ent->inuse || !ent->client)
	{
		return;
	}

	switch (ev)
	{
		case EV_FIRE_WEAPON:
			if (ent->m_pVehicle && ent->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER &&
				ent->client && ent->client->ps.m_iVehicleNum)
			{ //a speeder with a pilot
				gentity_t *rider = &g_entities[ent->client->ps.m_iVehicleNum-1];
				if (rider->inuse && rider->client)
				{ //pilot is valid...
                    if (rider->client->ps.weapon != WP_MELEE &&
						(rider->client->ps.weapon != WP_SABER || !BG_SabersOff(&rider->client->ps)))
					{ //can only attack on speeder when using melee or when saber is holstered
						break;
					}
				}
			}

			FireWeapon( ent, qfalse );
			ent->client->dangerTime = level.time;
			ent->client->ps.eFlags &= ~EF_INVULNERABLE;
			ent->client->invulnerableTimer = 0;
			break;
		case EV_ALT_FIRE:
			FireWeapon( ent, qtrue );
			ent->client->dangerTime = level.time;
			ent->client->ps.eFlags &= ~EF_INVULNERABLE;
			ent->client->invulnerableTimer = 0;
			break;
	}
}

/*
================
ClientEvents

Events will be passed on to the clients for presentation,
but any server game effects are handled here
================
*/
qboolean BG_InKnockDownOnly( int anim );

void ClientEvents( gentity_t *ent, int oldEventSequence ) {
	int		i;//, j;
	int		event;
	gclient_t *client;
	int		damage;
	vec3_t	dir;
//	vec3_t	origin, angles;
//	qboolean	fired;
//	gitem_t *item;
//	gentity_t *drop;

	client = ent->client;

	if ( oldEventSequence < client->ps.eventSequence - MAX_PS_EVENTS ) {
		oldEventSequence = client->ps.eventSequence - MAX_PS_EVENTS;
	}
	for ( i = oldEventSequence ; i < client->ps.eventSequence ; i++ ) {
		event = client->ps.events[ i & (MAX_PS_EVENTS-1) ];

		switch ( event ) {
		case EV_FALL:
		case EV_ROLL:
			{
				int delta = client->ps.eventParms[ i & (MAX_PS_EVENTS-1) ];
				qboolean knockDownage = qfalse;

				if (ent->client && ent->client->ps.fallingToDeath)
				{
					break;
				}

				if (ent->client && ent->client->sess.raceMode)
					break;

				if ( ent->s.eType != ET_PLAYER )
				{
					break;		// not in the player model
				}
				
				if ( dmflags.integer & DF_NO_FALLING )
				{
					break;
				}

				if (BG_InKnockDownOnly(ent->client->ps.legsAnim))
				{
					if (delta <= 14)
					{
						break;
					}
					knockDownage = qtrue;
				}
				else
				{
					if (delta <= 44)
					{
						break;
					}
				}

//[JAPRO - Serverside - Fall damage Splat cap - Start]
				if (knockDownage) {
					if (g_maxFallDmg.integer && delta >= g_maxFallDmg.integer)
						damage = g_maxFallDmg.value; //knockdown splat damage gets capped
					else damage = delta*1; 
				}
				else {
					if (level.gametype == GT_SIEGE && delta > 60)
						damage = delta*1; //good enough for now, I guess
					else {
						if (g_maxFallDmg.integer && delta * 0.16 >= g_maxFallDmg.integer)
							damage = g_maxFallDmg.integer;
						else damage = delta*0.16; //good enough for now, I guess
					}
				}
//[JAPRO - Serverside - Fall damage cap - End]

				VectorSet (dir, 0, 0, 1);
				ent->pain_debounce_time = level.time + 200;	// no normal pain sound
				G_Damage (ent, NULL, NULL, NULL, NULL, damage, DAMAGE_NO_ARMOR, MOD_FALLING);

				if (ent->health < 1)
				{
					G_Sound(ent, CHAN_AUTO, G_SoundIndex( "sound/player/fallsplat.wav" ));
				}
			}
			break;
		case EV_FIRE_WEAPON:
			FireWeapon( ent, qfalse );
			ent->client->dangerTime = level.time;
			ent->client->ps.eFlags &= ~EF_INVULNERABLE;
			ent->client->invulnerableTimer = 0;
			break;

		case EV_ALT_FIRE:
			FireWeapon( ent, qtrue );
			ent->client->dangerTime = level.time;
			ent->client->ps.eFlags &= ~EF_INVULNERABLE;
			ent->client->invulnerableTimer = 0;
			break;

		case EV_SABER_ATTACK:
			ent->client->dangerTime = level.time;
			ent->client->ps.eFlags &= ~EF_INVULNERABLE;
			ent->client->invulnerableTimer = 0;
			break;

		//rww - Note that these must be in the same order (ITEM#-wise) as they are in holdable_t
		case EV_USE_ITEM1: //seeker droid
			ItemUse_Seeker(ent);
			break;
		case EV_USE_ITEM2: //shield
			ItemUse_Shield(ent);
			break;
		case EV_USE_ITEM3: //medpack
			ItemUse_MedPack(ent);
			break;
		case EV_USE_ITEM4: //big medpack
			ItemUse_MedPack_Big(ent);
			break;
		case EV_USE_ITEM5: //binoculars
			ItemUse_Binoculars(ent);
			break;
		case EV_USE_ITEM6: //sentry gun
			ItemUse_Sentry(ent);
			break;
		case EV_USE_ITEM7: //jetpack
			if (!g_tweakJetpack.integer && !ent->client->sess.raceMode)
				ItemUse_Jetpack(ent);
			break;
		case EV_USE_ITEM8: //health disp
			//ItemUse_UseDisp(ent, HI_HEALTHDISP);
			break;
		case EV_USE_ITEM9: //ammo disp
			//ItemUse_UseDisp(ent, HI_AMMODISP);
			break;
		case EV_USE_ITEM10: //eweb
			ItemUse_UseEWeb(ent);
			break;
		case EV_USE_ITEM11: //cloak
			ItemUse_UseCloak(ent);
			break;
		default:
			break;
		}
	}

}

/*
==============
SendPendingPredictableEvents
==============
*/
void SendPendingPredictableEvents( playerState_t *ps ) {
	gentity_t *t;
	int event, seq;
	int extEvent, number;

	// if there are still events pending
	if ( ps->entityEventSequence < ps->eventSequence ) {
		// create a temporary entity for this event which is sent to everyone
		// except the client who generated the event
		seq = ps->entityEventSequence & (MAX_PS_EVENTS-1);
		event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
		// set external event to zero before calling BG_PlayerStateToEntityState
		extEvent = ps->externalEvent;
		ps->externalEvent = 0;
		// create temporary entity for event
		t = G_TempEntity( ps->origin, event );
		number = t->s.number;
		BG_PlayerStateToEntityState( ps, &t->s, qtrue );
		t->s.number = number;
		t->s.eType = ET_EVENTS + event;
		t->s.eFlags |= EF_PLAYER_EVENT;
		t->s.otherEntityNum = ps->clientNum;
		// send to everyone except the client who generated the event
		t->r.svFlags |= SVF_NOTSINGLECLIENT;
		t->r.singleClient = ps->clientNum;
		// set back external event
		ps->externalEvent = extEvent;
	}
}

/*
==================
G_UpdateClientBroadcasts

Determines whether this client should be broadcast to any other clients.  
A client is broadcast when another client is using force sight or is
==================
*/
#define MAX_JEDIMASTER_DISTANCE	2500
#define MAX_JEDIMASTER_FOV		100

#define MAX_SIGHT_DISTANCE		1500
#define MAX_SIGHT_FOV			100

static void G_UpdateForceSightBroadcasts ( gentity_t *self )
{
	int i;

	// Any clients with force sight on should see this client
	for ( i = 0; i < level.numConnectedClients; i ++ )
	{
		gentity_t *ent = &g_entities[level.sortedClients[i]];
		float	  dist;
		vec3_t	  angles;
	
		if ( ent == self )
			continue;

		if (!g_removeSpectatorPortals.integer || ent->client->sess.sessionTeam != TEAM_SPECTATOR) // Setting is off, or they are not in spec. (this is only skipped if setting is on and they are in spec)
		{
			// Not using force sight so we shouldnt broadcast to this one
			if ( !(ent->client->ps.fd.forcePowersActive & (1<<FP_SEE) ) )
				continue;

			VectorSubtract( self->client->ps.origin, ent->client->ps.origin, angles );
			dist = VectorLengthSquared ( angles );
			vectoangles ( angles, angles );

			// Too far away then just forget it
			if ( dist > MAX_SIGHT_DISTANCE * MAX_SIGHT_DISTANCE )
				continue;
		
			// If not within the field of view then forget it
			if ( !InFieldOfVision ( ent->client->ps.viewangles, MAX_SIGHT_FOV, angles ) )
				continue;
				//break; //why is this break and not continue?
		}
		// Turn on the broadcast bit for the master and since there is only one
		// master we are done
		self->r.broadcastClients[ent->s.clientNum/32] |= (1 << (ent->s.clientNum%32));
		//break; //wait what, this isnt master this is force sight, there could be way more than one user why does it break
	}
}

static void G_UpdateJediMasterBroadcasts ( gentity_t *self )
{
	int i;

	// Not jedi master mode then nothing to do
	if ( level.gametype != GT_JEDIMASTER )
	{
		return;
	}

	// This client isnt the jedi master so it shouldnt broadcast
	if ( !self->client->ps.isJediMaster )
	{
		return;
	}

	// Broadcast ourself to all clients within range
	for ( i = 0; i < level.numConnectedClients; i ++ )
	{
		gentity_t *ent = &g_entities[level.sortedClients[i]];
		float	  dist;
		vec3_t	  angles;

		if ( ent == self )
		{
			continue;
		}

		VectorSubtract( self->client->ps.origin, ent->client->ps.origin, angles );
		dist = VectorLengthSquared ( angles );
		vectoangles ( angles, angles );

		// Too far away then just forget it
		if ( dist > MAX_JEDIMASTER_DISTANCE * MAX_JEDIMASTER_DISTANCE )
		{
			continue;
		}
		
		// If not within the field of view then forget it
		if ( !InFieldOfVision ( ent->client->ps.viewangles, MAX_JEDIMASTER_FOV, angles ) )
		{
			continue;
		}

		// Turn on the broadcast bit for the master and since there is only one
		// master we are done
		self->r.broadcastClients[ent->s.clientNum/32] |= (1 << (ent->s.clientNum%32));
		//why not break here, since there actually is only one master?... the fuck is this
		break;
	}
}

#if _ANTIWALLHACK

/*
static float VectorAngle( const vec3_t a, const vec3_t b ) {
	const float lA = VectorLength( a );
	const float lB = VectorLength( b );
	const float lAB = lA * lB;

	if ( lAB == 0.0f ) {
		return 0.0f;
	}
	else {
		return (float)(acosf( DotProduct( a, b ) / lAB ) * (180.f / M_PI));
	}
}

static void MakeVector( const vec3_t ain, vec3_t vout ) {
	float pitch, yaw, tmp;

	pitch = (float)(ain[PITCH] * M_PI / 180.0f);
	yaw = (float)(ain[YAW] * M_PI / 180.0f);
	tmp = (float)cosf( pitch );

	vout[1] = (float)(-tmp * -cosf( yaw ));
	vout[2] = (float)(sinf( yaw )*tmp);
	vout[3] = (float)-sinf( pitch );
}
*/

//extern void G_TestLine(vec3_t start, vec3_t end, int color, int time);
static qboolean SE_RenderIsVisible( const gentity_t *self, const vec3_t startPos, const vec3_t testOrigin,
	qboolean reversedCheck )
{
	trace_t results;

	trap->Trace( &results, startPos, NULL, NULL, testOrigin, self - g_entities, MASK_SOLID, qfalse, 0, 0 );

	if ( results.fraction < 1.0f ) {
		if ( (results.surfaceFlags & SURF_FORCEFIELD)
			|| (results.surfaceFlags & MATERIAL_MASK) == MATERIAL_GLASS
			|| (results.surfaceFlags & MATERIAL_MASK) == MATERIAL_SHATTERGLASS )
		{
			//FIXME: This is a quick hack to render people and things through glass and force fields, but will also take
			//	effect even if there is another wall between them (and double glass) - which is bad, of course, but
			//	nothing i can prevent right now.
			if ( reversedCheck || SE_RenderIsVisible( self, testOrigin, startPos, qtrue ) ) {
				return qtrue;
			}
		}

		return qfalse;
	}

	return qtrue;
}

static qboolean SE_RenderPlayerChecks( const gentity_t *self, const vec3_t playerOrigin, vec3_t playerPoints[9] ) {
	trace_t results;
	int i;

	for ( i = 0; i < 9; i++ ) {
		if ( trap->PointContents( playerPoints[i], self - g_entities ) == CONTENTS_SOLID ) {
			trap->Trace( &results, playerOrigin, NULL, NULL, playerPoints[i], self - g_entities, MASK_SOLID, qfalse, 0,
				0 );
			VectorCopy( results.endpos, playerPoints[i] );
		}
	}

	return qtrue;
}


static qboolean SE_IsPlayerCrouching( const gentity_t *ent ) {
	const playerState_t *ps = &ent->client->ps;

	// FIXME: This is no proper way to determine if a client is actually in a crouch position, we want to do this in
	//	order to properly hide a client from the enemy when he is crouching behind an obstacle and could not possibly
	//	be seen.

	if ( !ent->inuse || !ps ) {
		return qfalse;
	}

	if ( ps->forceHandExtend == HANDEXTEND_KNOCKDOWN ) {
		return qtrue;
	}

	if ( ps->pm_flags & PMF_DUCKED ) {
		return qtrue;
	}

	switch ( ps->legsAnim ) {
	case BOTH_GETUP1:
	case BOTH_GETUP2:
	case BOTH_GETUP3:
	case BOTH_GETUP4:
	case BOTH_GETUP5:
	case BOTH_FORCE_GETUP_F1:
	case BOTH_FORCE_GETUP_F2:
	case BOTH_FORCE_GETUP_B1:
	case BOTH_FORCE_GETUP_B2:
	case BOTH_FORCE_GETUP_B3:
	case BOTH_FORCE_GETUP_B4:
	case BOTH_FORCE_GETUP_B5:
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_BROLL_L:
	case BOTH_GETUP_BROLL_R:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
	case BOTH_GETUP_FROLL_L:
	case BOTH_GETUP_FROLL_R:
		return qtrue;
	default:
		break;
	}

	switch ( ps->torsoAnim ) {
	case BOTH_GETUP1:
	case BOTH_GETUP2:
	case BOTH_GETUP3:
	case BOTH_GETUP4:
	case BOTH_GETUP5:
	case BOTH_FORCE_GETUP_F1:
	case BOTH_FORCE_GETUP_F2:
	case BOTH_FORCE_GETUP_B1:
	case BOTH_FORCE_GETUP_B2:
	case BOTH_FORCE_GETUP_B3:
	case BOTH_FORCE_GETUP_B4:
	case BOTH_FORCE_GETUP_B5:
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_BROLL_L:
	case BOTH_GETUP_BROLL_R:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
	case BOTH_GETUP_FROLL_L:
	case BOTH_GETUP_FROLL_R:
		return qtrue;
	default:
		break;
	}

	return qfalse;
}

static qboolean SE_RenderPlayerPoints( qboolean isCrouching, const vec3_t playerAngles, const vec3_t playerOrigin,
	vec3_t playerPoints[9] )
{
	int isHeight = isCrouching ? 32 : 56;
	vec3_t	forward, right, up;

	AngleVectors( playerAngles, forward, right, up );

	VectorMA( playerOrigin, 32.0f, up, playerPoints[0] );
	VectorMA( playerOrigin, 64.0f, forward, playerPoints[1] );
	VectorMA( playerPoints[1], 64.0f, right, playerPoints[1] );
	VectorMA( playerOrigin, 64.0f, forward, playerPoints[2] );
	VectorMA( playerPoints[2], -64.0f, right, playerPoints[2] );
	VectorMA( playerOrigin, -64.0f, forward, playerPoints[3] );
	VectorMA( playerPoints[3], 64.0f, right, playerPoints[3] );
	VectorMA( playerOrigin, -64.0f, forward, playerPoints[4] );
	VectorMA( playerPoints[4], -64.0f, right, playerPoints[4] );

	VectorMA( playerPoints[1], isHeight, up, playerPoints[5] );
	VectorMA( playerPoints[2], isHeight, up, playerPoints[6] );
	VectorMA( playerPoints[3], isHeight, up, playerPoints[7] );
	VectorMA( playerPoints[4], isHeight, up, playerPoints[8] );

	return qtrue;
}

static void GetCameraPosition(const gentity_t *self, vec3_t cameraOrigin) {
	vec3_t forward;
	int thirdPerson = 1, thirdPersonRange = 80, thirdPersonVertOffset = 16;

	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
	VectorNormalize( forward );

	//Lets see if they have japro, then get the thirdpersonvertoffset and thirdpersonrange.  otherwise just use defaults of 16 and 80.
	if (self->client->pers.isJAPRO) {
		thirdPerson = self->client->pers.thirdPerson;
		thirdPersonRange = self->client->pers.thirdPersonRange;
		thirdPersonVertOffset = self->client->pers.thirdPersonVertOffset;
	}

	//Get third person position.  
	VectorCopy( self->client->ps.origin, cameraOrigin );
	VectorMA( cameraOrigin, -thirdPersonRange, forward, cameraOrigin );
	cameraOrigin[2] += 24 + thirdPersonVertOffset;

	if (SE_IsPlayerCrouching(self))
		cameraOrigin[2] -= 32;
}

static qboolean SE_NetworkPlayer( const gentity_t *self, const gentity_t *other ) {
	int i;
	vec3_t firstPersonPos, thirdPersonPos, targPos[9];
	uint32_t contents;

	GetCameraPosition(self, thirdPersonPos);

	VectorCopy( self->client->ps.origin, firstPersonPos );
	firstPersonPos[2] += 24;
	if (SE_IsPlayerCrouching(self))
		firstPersonPos[2] -= 32;

	contents = trap->PointContents( firstPersonPos, self - g_entities );

	// translucent, we should probably just network them anyways
	if ( contents & (CONTENTS_WATER | CONTENTS_LAVA | CONTENTS_SLIME) ) {
		return qtrue;
	}

	// entirely in an opaque surface, no point networking them.
	if ( contents & (CONTENTS_SOLID | CONTENTS_TERRAIN | CONTENTS_OPAQUE) ) {
#ifdef _DEBUG
		if ( self->s.number == 0 ) {
			trap->Print( "WALLHACK[%i]: inside opaque surface\n", level.time );
		}
#endif // _DEBUG
		return qfalse;
	}

	// plot their bbox pointer into targPos[]
	SE_RenderPlayerPoints( SE_IsPlayerCrouching( other ), other->client->ps.viewangles, other->client->ps.origin,
		targPos );
	SE_RenderPlayerChecks( self, other->client->ps.origin, targPos );

	for ( i = 0; i < 9; i++ ) {

		if (g_antiWallhack.integer > 1) {
			int offset = g_antiWallhack.integer - 2;

			if (offset < 0)
				offset = 0;
			if (offset > 8)
				offset = 8;

			G_TestLine(thirdPersonPos, targPos[offset], 0x0000ff, 200); //check trace.fraction? ehh trace.startsolid or whatever?
		}

		if ( SE_RenderIsVisible( self, thirdPersonPos, targPos[i], qfalse ) ) {
			return qtrue;
		}
		if ( SE_RenderIsVisible( self, firstPersonPos, targPos[i], qfalse ) ) {
			return qtrue;
		}
	}

#ifdef _DEBUG
	if ( self->s.number == 0 ) {
		trap->Print( "WALLHACK[%i]: not visible\n", level.time );
	}
#endif // _DEBUG

	return qfalse;
}

/*
static qboolean SE_RenderInFOV( const gentity_t *self, const vec3_t testOrigin ) {
	const float fov = 110.0f;
	vec3_t	tmp, aim, view;

	VectorCopy( self->client->ps.origin, tmp );
	VectorSubtract( testOrigin, tmp, aim );
	MakeVector( self->client->ps.viewangles, view );

	// don't network if they're not in our field of view
	//TODO: only skip if they haven't been in our field of view for ~500ms to avoid flickering
	//TODO: also check distance, factoring in delta angle
	if ( VectorAngle( view, aim ) > (fov / 1.2f) ) {
#ifdef _DEBUG
		if ( self->s.number == 0 ) {
			trap->Print( "WALLHACK[%i]: not in field of view\n", level.time );
		}
#endif // _DEBUG
		return qfalse;
	}

	return qtrue;
}
*/

// Tracing non-players seems to have a bad effect, we know players are limited to 32 per frame, however other gentities
//	that are being added are not! It's stupid to actually add traces for it, even with a limited form i used before of 2
//	traces per object. There are to many too track and simply networking them takes less FPS either way
qboolean G_EntityOccluded( const gentity_t *self, const gentity_t *other ) {
	// This is a non-player object, just send it (see above).
	if ( !other->inuse || other->s.number >= level.maxclients ) {
		return qtrue;
	}

	// If this player is me, or my spectee, we will always draw and don't trace.
	if ( self == other ) {
		return qtrue;
	}

	if ( self->client->ps.zoomMode ) { // 0.0
		return qtrue;
	}

	/*
	// Not rendering; this player is not in our FOV.
	if ( !SE_RenderInFOV( self, other->client->ps.origin ) ) {
		Com_Printf("NOT FOV");
		return qtrue;
	}
	*/

	// Not rendering; this player's traces did not appear in my screen.
	if ( !SE_NetworkPlayer( self, other ) ) {
		return qtrue;
	}

	return qfalse;
}

static const float maxJediMasterDistance = (float)(2500 * 2500); // x^2, optimisation
static const float maxJediMasterFOV = 100.0f;
static const float maxForceSightDistance = (float)(1500 * 1500); // x^2, optimisation
static const float maxForceSightFOV = 100.0f;
void G_UpdateClientBroadcasts( gentity_t *self ) {
	int i;
	gentity_t *other;

	// we are always sent to ourselves
	// we are always sent to other clients if we are in their PVS
	// if we are not in their PVS, we must set the broadcastClients bit field
	// if we do not wish to be sent to any particular entity, we must set the broadcastClients bit field and the
	//	SVF_BROADCASTCLIENTS bit flag
	self->r.broadcastClients[0] = 0; //wats this
	self->r.broadcastClients[1] = 0;


	/* //the fuck is this
	if ( japp_antiWallhack.integer ) {
		self->r.svFlags |= SVF_BROADCASTCLIENTS;
	}
	else {
		self->r.svFlags &= ~SVF_BROADCASTCLIENTS;
	}

	*/

	for ( i = 0, other = g_entities; i < MAX_CLIENTS; i++, other++ ) {
		qboolean send = qfalse;
		float dist;
		vec3_t angles;

		if ( !other->inuse || other->client->pers.connected != CON_CONNECTED ) {
			// no need to compute visibility for non-connected clients
			continue;
		}

		if ( other == self ) {
			// we are always sent to ourselves anyway, this is purely an optimisation
			continue;
		}

		if (g_removeSpectatorPortals.integer && other->client->sess.sessionTeam == TEAM_SPECTATOR) {
			send = qtrue;
		}
		else if ( g_antiWallhack.integer && other->client->ps.duelInProgress && self->client->ps.duelInProgress && other->client->ps.duelIndex == self->client->ps.clientNum && self->client->ps.duelIndex == other->client->ps.clientNum) {
			if ( G_EntityOccluded( self, other ) ) {
				other->r.svFlags |= SVF_NOTSINGLECLIENT;
				other->r.singleClient = self->client->ps.clientNum;
				continue;
			}
			else {
				other->r.svFlags &= ~SVF_NOTSINGLECLIENT;
				send = qtrue;
			}
		}
		else {
			VectorSubtract( self->client->ps.origin, other->client->ps.origin, angles );
			dist = VectorLengthSquared( angles );
			vectoangles( angles, angles );

			// broadcast jedi master to everyone if we are in distance/field of view
			if ( level.gametype == GT_JEDIMASTER && self->client->ps.isJediMaster ) {
				if ( dist < maxJediMasterDistance
					&& InFieldOfVision( other->client->ps.viewangles, maxJediMasterFOV, angles ) )
				{
					send = qtrue;
				}
			}

			// broadcast this client to everyone using force sight if we are in distance/field of view
			if ( (other->client->ps.fd.forcePowersActive & (1 << FP_SEE)) ) {
				if ( dist < maxForceSightDistance
					&& InFieldOfVision( other->client->ps.viewangles, maxForceSightFOV, angles ) )
				{
					send = qtrue;
				}
			}
		}

		if ( send ) {
			self->r.broadcastClients[i / 32] |= (1 << (i % 32));
		}
	}

	trap->LinkEntity( (sharedEntity_t *)self );
}
#else
void G_UpdateClientBroadcasts ( gentity_t *self )
{
	// Clear all the broadcast bits for this client
	memset ( self->r.broadcastClients, 0, sizeof ( self->r.broadcastClients ) );

	// The jedi master is broadcast to everyone in range
	G_UpdateJediMasterBroadcasts ( self );

	// Anyone with force sight on should see this client
	G_UpdateForceSightBroadcasts ( self );
}
#endif

void G_AddPushVecToUcmd( gentity_t *self, usercmd_t *ucmd )
{
	vec3_t	forward, right, moveDir;
	float	pushSpeed, fMove, rMove;

	if ( !self->client )
	{
		return;
	}
	pushSpeed = VectorLengthSquared(self->client->pushVec);
	if(!pushSpeed)
	{//not being pushed
		return;
	}

	AngleVectors(self->client->ps.viewangles, forward, right, NULL);
	VectorScale(forward, ucmd->forwardmove/127.0f * self->client->ps.speed, moveDir);
	VectorMA(moveDir, ucmd->rightmove/127.0f * self->client->ps.speed, right, moveDir);
	//moveDir is now our intended move velocity

	VectorAdd(moveDir, self->client->pushVec, moveDir);
	self->client->ps.speed = VectorNormalize(moveDir);
	//moveDir is now our intended move velocity plus our push Vector

	fMove = 127.0 * DotProduct(forward, moveDir);
	rMove = 127.0 * DotProduct(right, moveDir);
	ucmd->forwardmove = floor(fMove);//If in the same dir , will be positive
	ucmd->rightmove = floor(rMove);//If in the same dir , will be positive

	if ( self->client->pushVecTime < level.time )
	{
		VectorClear( self->client->pushVec );
	}
}

qboolean G_StandingAnim( int anim )
{//NOTE: does not check idles or special (cinematic) stands
	switch ( anim )
	{
	case BOTH_STAND1:
	case BOTH_STAND2:
	case BOTH_STAND3:
	case BOTH_STAND4:
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean G_ActionButtonPressed(int buttons)
{
	if (buttons & BUTTON_ATTACK)
	{
		return qtrue;
	}
	else if (buttons & BUTTON_USE_HOLDABLE)
	{
		return qtrue;
	}
	else if (buttons & BUTTON_GESTURE)
	{
		return qtrue;
	}
	else if (buttons & BUTTON_USE)
	{
		return qtrue;
	}
	else if (buttons & BUTTON_FORCEGRIP)
	{
		return qtrue;
	}
	else if (buttons & BUTTON_ALT_ATTACK)
	{
		return qtrue;
	}
	else if (buttons & BUTTON_FORCEPOWER)
	{
		return qtrue;
	}
	else if (buttons & BUTTON_FORCE_LIGHTNING)
	{
		return qtrue;
	}
	else if (buttons & BUTTON_FORCE_DRAIN)
	{
		return qtrue;
	}
	else if (buttons & BUTTON_DASH)
	{
		return qtrue;
	}
	else if (buttons & BUTTON_TARGET)
	{
		return qtrue;
	}

	return qfalse;
}

void G_CheckClientIdle( gentity_t *ent, usercmd_t *ucmd ) 
{
	vec3_t viewChange;
	qboolean actionPressed;
	int buttons;

	if ( !ent || !ent->client || ent->health <= 0 || ent->client->ps.stats[STAT_HEALTH] <= 0 ||
		ent->client->sess.sessionTeam == TEAM_SPECTATOR || (ent->client->ps.pm_flags & PMF_FOLLOW))
	{
		return;
	}

	buttons = ucmd->buttons;

	if (ent->r.svFlags & SVF_BOT)
	{ //they press use all the time..
		buttons &= ~BUTTON_USE;
	}
	actionPressed = G_ActionButtonPressed(buttons);

	VectorSubtract(ent->client->ps.viewangles, ent->client->idleViewAngles, viewChange);
	if ( !VectorCompare( vec3_origin, ent->client->ps.velocity ) 
		|| actionPressed || ucmd->forwardmove || ucmd->rightmove || ucmd->upmove 
		|| !G_StandingAnim( ent->client->ps.legsAnim ) 
		|| (ent->health+ent->client->ps.stats[STAT_ARMOR]) != ent->client->idleHealth
		|| VectorLength(viewChange) > 10
		|| ent->client->ps.legsTimer > 0
		|| ent->client->ps.torsoTimer > 0
		|| ent->client->ps.weaponTime > 0
		|| ent->client->ps.weaponstate == WEAPON_CHARGING
		|| ent->client->ps.weaponstate == WEAPON_CHARGING_ALT
		|| ent->client->ps.zoomMode
		|| (ent->client->ps.weaponstate != WEAPON_READY && ent->client->ps.weapon != WP_SABER)
		|| ent->client->ps.forceHandExtend != HANDEXTEND_NONE
		|| ent->client->ps.saberBlocked != BLOCKED_NONE
		|| ent->client->ps.saberBlocking >= level.time
		|| ent->client->ps.weapon == WP_MELEE
		|| (ent->client->ps.weapon != ent->client->pers.cmd.weapon && ent->s.eType != ET_NPC))
	{//FIXME: also check for turning?
		qboolean brokeOut = qfalse;

		if ( !VectorCompare( vec3_origin, ent->client->ps.velocity ) 
			|| actionPressed || ucmd->forwardmove || ucmd->rightmove || ucmd->upmove 
			|| (ent->health+ent->client->ps.stats[STAT_ARMOR]) != ent->client->idleHealth
			|| ent->client->ps.zoomMode
			|| (ent->client->ps.weaponstate != WEAPON_READY && ent->client->ps.weapon != WP_SABER)
			|| (ent->client->ps.weaponTime > 0 && ent->client->ps.weapon == WP_SABER)
			|| ent->client->ps.weaponstate == WEAPON_CHARGING
			|| ent->client->ps.weaponstate == WEAPON_CHARGING_ALT
			|| ent->client->ps.forceHandExtend != HANDEXTEND_NONE
			|| ent->client->ps.saberBlocked != BLOCKED_NONE
			|| ent->client->ps.saberBlocking >= level.time
			|| ent->client->ps.weapon == WP_MELEE
			|| (ent->client->ps.weapon != ent->client->pers.cmd.weapon && ent->s.eType != ET_NPC))
		{
			//if in an idle, break out
			switch ( ent->client->ps.legsAnim )
			{
			case BOTH_STAND1IDLE1:
			case BOTH_STAND2IDLE1:
			case BOTH_STAND2IDLE2:
			case BOTH_STAND3IDLE1:
			case BOTH_STAND5IDLE1:
				ent->client->ps.legsTimer = 0;
				brokeOut = qtrue;
				break;
			}
			switch ( ent->client->ps.torsoAnim )
			{
			case BOTH_STAND1IDLE1:
			case BOTH_STAND2IDLE1:
			case BOTH_STAND2IDLE2:
			case BOTH_STAND3IDLE1:
			case BOTH_STAND5IDLE1:
				ent->client->ps.torsoTimer = 0;
				ent->client->ps.weaponTime = 0;
				ent->client->ps.saberMove = LS_READY;
				brokeOut = qtrue;
				break;
			}
		}
		//
		ent->client->idleHealth = (ent->health+ent->client->ps.stats[STAT_ARMOR]);
		VectorCopy(ent->client->ps.viewangles, ent->client->idleViewAngles);
		if ( ent->client->idleTime < level.time )
		{
			ent->client->idleTime = level.time;
		}

		if (brokeOut &&
			(ent->client->ps.weaponstate == WEAPON_CHARGING || ent->client->ps.weaponstate == WEAPON_CHARGING_ALT))
		{
			ent->client->ps.torsoAnim = TORSO_RAISEWEAP1;
		}
	}
	/*
	else if (ent->s.eType == ET_PLAYER && !ent->client->ps.duelInProgress && level.time - ent->client->idleTime > g_inactivityProtectTime.integer * 1000) {
		ent->client->pers.protect = qtrue;
	}
	*/
	else if ( level.time - ent->client->idleTime > 5000 )
	{//been idle for 5 seconds
		int	idleAnim = -1;
		switch ( ent->client->ps.legsAnim )
		{
		case BOTH_STAND1:
			idleAnim = BOTH_STAND1IDLE1;
			break;
		case BOTH_STAND2:
			idleAnim = BOTH_STAND2IDLE1;//Q_irand(BOTH_STAND2IDLE1,BOTH_STAND2IDLE2);
			break;
		case BOTH_STAND3:
			idleAnim = BOTH_STAND3IDLE1;
			break;
		case BOTH_STAND5:
			idleAnim = BOTH_STAND5IDLE1;
			break;
		}

		if (idleAnim == BOTH_STAND2IDLE1 && Q_irand(1, 10) <= 5)
		{
			idleAnim = BOTH_STAND2IDLE2;
		}

		if ( /*PM_HasAnimation( ent, idleAnim )*/idleAnim > 0 && idleAnim < MAX_ANIMATIONS )
		{
			G_SetAnim(ent, ucmd, SETANIM_BOTH, idleAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);

			//don't idle again after this anim for a while
			//ent->client->idleTime = level.time + PM_AnimLength( ent->client->clientInfo.animFileIndex, (animNumber_t)idleAnim ) + Q_irand( 0, 2000 );
			ent->client->idleTime = level.time + ent->client->ps.legsTimer + Q_irand( 0, 2000 );
		}
	}
}

void NPC_Accelerate( gentity_t *ent, qboolean fullWalkAcc, qboolean fullRunAcc )
{
	if ( !ent->client || !ent->NPC )
	{
		return;
	}

	if ( !ent->NPC->stats.acceleration )
	{//No acceleration means just start and stop
		ent->NPC->currentSpeed = ent->NPC->desiredSpeed;
	}
	//FIXME:  in cinematics always accel/decel?
	else if ( ent->NPC->desiredSpeed <= ent->NPC->stats.walkSpeed )
	{//Only accelerate if at walkSpeeds
		if ( ent->NPC->desiredSpeed > ent->NPC->currentSpeed + ent->NPC->stats.acceleration )
		{
			//ent->client->ps.friction = 0;
			ent->NPC->currentSpeed += ent->NPC->stats.acceleration;
		}
		else if ( ent->NPC->desiredSpeed > ent->NPC->currentSpeed )
		{
			//ent->client->ps.friction = 0;
			ent->NPC->currentSpeed = ent->NPC->desiredSpeed;
		}
		else if ( fullWalkAcc && ent->NPC->desiredSpeed < ent->NPC->currentSpeed - ent->NPC->stats.acceleration )
		{//decelerate even when walking
			ent->NPC->currentSpeed -= ent->NPC->stats.acceleration;
		}
		else if ( ent->NPC->desiredSpeed < ent->NPC->currentSpeed )
		{//stop on a dime
			ent->NPC->currentSpeed = ent->NPC->desiredSpeed;
		}
	}
	else//  if ( ent->NPC->desiredSpeed > ent->NPC->stats.walkSpeed )
	{//Only decelerate if at runSpeeds
		if ( fullRunAcc && ent->NPC->desiredSpeed > ent->NPC->currentSpeed + ent->NPC->stats.acceleration )
		{//Accelerate to runspeed
			//ent->client->ps.friction = 0;
			ent->NPC->currentSpeed += ent->NPC->stats.acceleration;
		}
		else if ( ent->NPC->desiredSpeed > ent->NPC->currentSpeed )
		{//accelerate instantly
			//ent->client->ps.friction = 0;
			ent->NPC->currentSpeed = ent->NPC->desiredSpeed;
		}
		else if ( fullRunAcc && ent->NPC->desiredSpeed < ent->NPC->currentSpeed - ent->NPC->stats.acceleration )
		{
			ent->NPC->currentSpeed -= ent->NPC->stats.acceleration;
		}
		else if ( ent->NPC->desiredSpeed < ent->NPC->currentSpeed )
		{
			ent->NPC->currentSpeed = ent->NPC->desiredSpeed;
		}
	}
}

/*
-------------------------
NPC_GetWalkSpeed
-------------------------
*/

static int NPC_GetWalkSpeed( gentity_t *ent )
{
	int	walkSpeed = 0;

	if ( ( ent->client == NULL ) || ( ent->NPC == NULL ) )
		return 0;

	switch ( ent->client->playerTeam )
	{
	case NPCTEAM_PLAYER:	//To shutup compiler, will add entries later (this is stub code)
	default:
		walkSpeed = ent->NPC->stats.walkSpeed;
		break;
	}

	return walkSpeed;
}

/*
-------------------------
NPC_GetRunSpeed
-------------------------
*/
static int NPC_GetRunSpeed( gentity_t *ent )
{
	int	runSpeed = 0;

	if ( ( ent->client == NULL ) || ( ent->NPC == NULL ) )
		return 0;
/*
	switch ( ent->client->playerTeam )
	{
	case TEAM_BORG:
		runSpeed = ent->NPC->stats.runSpeed;
		runSpeed += BORG_RUN_INCR * (g_npcspskill->integer%3);
		break;

	case TEAM_8472:
		runSpeed = ent->NPC->stats.runSpeed;
		runSpeed += SPECIES_RUN_INCR * (g_npcspskill->integer%3);
		break;

	case TEAM_STASIS:
		runSpeed = ent->NPC->stats.runSpeed;
		runSpeed += STASIS_RUN_INCR * (g_npcspskill->integer%3);
		break;

	case TEAM_BOTS:
		runSpeed = ent->NPC->stats.runSpeed;
		break;

	default:
		runSpeed = ent->NPC->stats.runSpeed;
		break;
	}
*/
	// team no longer indicates species/race.  Use NPC_class to adjust speed for specific npc types
	switch( ent->client->NPC_class)
	{
	case CLASS_PROBE:	// droid cases here to shut-up compiler
	case CLASS_GONK:
	case CLASS_R2D2:
	case CLASS_R5D2:
	case CLASS_MARK1:
	case CLASS_MARK2:
	case CLASS_PROTOCOL:
	case CLASS_ATST: // hmm, not really your average droid
	case CLASS_MOUSE:
	case CLASS_SEEKER:
	case CLASS_REMOTE:
		runSpeed = ent->NPC->stats.runSpeed;
		break;

	default:
		runSpeed = ent->NPC->stats.runSpeed*1.3f; //rww - seems to slow in MP for some reason.
		break;
	}

	return runSpeed;
}

//Seems like a slightly less than ideal method for this, could it be done on the client?
extern qboolean FlyingCreature( gentity_t *ent );
void G_CheckMovingLoopingSounds( gentity_t *ent, usercmd_t *ucmd )
{
	if ( ent->client )
	{
		if ( (ent->NPC&&!VectorCompare( vec3_origin, ent->client->ps.moveDir ))//moving using moveDir
			|| ucmd->forwardmove || ucmd->rightmove//moving using ucmds
			|| (ucmd->upmove&&FlyingCreature( ent ))//flier using ucmds to move
			|| (FlyingCreature( ent )&&!VectorCompare( vec3_origin, ent->client->ps.velocity )&&ent->health>0))//flier using velocity to move
		{
			switch( ent->client->NPC_class )
			{
			case CLASS_R2D2:
				ent->s.loopSound = G_SoundIndex( "sound/chars/r2d2/misc/r2_move_lp.wav" );
				break;
			case CLASS_R5D2:
				ent->s.loopSound = G_SoundIndex( "sound/chars/r2d2/misc/r2_move_lp2.wav" );
				break;
			case CLASS_MARK2:
				ent->s.loopSound = G_SoundIndex( "sound/chars/mark2/misc/mark2_move_lp" );
				break;
			case CLASS_MOUSE:
				ent->s.loopSound = G_SoundIndex( "sound/chars/mouse/misc/mouse_lp" );
				break;
			case CLASS_PROBE:
				ent->s.loopSound = G_SoundIndex( "sound/chars/probe/misc/probedroidloop" );
			default:
				break;
			}
		}
		else
		{//not moving under your own control, stop loopSound
			if ( ent->client->NPC_class == CLASS_R2D2 || ent->client->NPC_class == CLASS_R5D2 
					|| ent->client->NPC_class == CLASS_MARK2 || ent->client->NPC_class == CLASS_MOUSE 
					|| ent->client->NPC_class == CLASS_PROBE )
			{
				ent->s.loopSound = 0;
			}
		}
	}
}

void G_HeldByMonster( gentity_t *ent, usercmd_t *ucmd )
{
	if ( ent 
		&& ent->client
		&& ent->client->ps.hasLookTarget )//NOTE: lookTarget is an entity number, so this presumes that client 0 is NOT a Rancor...
	{
		gentity_t *monster = &g_entities[ent->client->ps.lookTarget];
		if ( monster && monster->client )
		{
			//take the monster's waypoint as your own
			ent->waypoint = monster->waypoint;
			if ( monster->s.NPC_class == CLASS_RANCOR )
			{//only possibility right now, may add Wampa and Sand Creature later
				BG_AttachToRancor( monster->ghoul2, //ghoul2 info
					monster->r.currentAngles[YAW],
					monster->r.currentOrigin,
					level.time,
					NULL,
					monster->modelScale,
					(monster->client->ps.eFlags2&EF2_GENERIC_NPC_FLAG),
					ent->client->ps.origin,
					ent->client->ps.viewangles,
					NULL );
			}
			VectorClear( ent->client->ps.velocity );
			G_SetOrigin( ent, ent->client->ps.origin );
			SetClientViewAngle( ent, ent->client->ps.viewangles );
			G_SetAngles( ent, ent->client->ps.viewangles );
			trap->LinkEntity( (sharedEntity_t *)ent );//redundant?
		}
	}
	// don't allow movement, weapon switching, and most kinds of button presses
	ucmd->forwardmove = 0;
	ucmd->rightmove = 0;
	ucmd->upmove = 0;
}

typedef enum tauntTypes_e
{
	TAUNT_TAUNT = 0,
	TAUNT_BOW,
	TAUNT_MEDITATE,
	TAUNT_FLOURISH,
	TAUNT_GLOAT
} tauntTypes_t;

void G_SetTauntAnim( gentity_t *ent, int taunt )
{

#if 1 //ass
	if (g_emotesDisable.integer & (1 << E_BASEDUEL)) { //block flourish/gloat/bow/meditate outside of duelmode or while moving
		if (ent->client->pers.cmd.upmove ||
			ent->client->pers.cmd.forwardmove ||
			ent->client->pers.cmd.rightmove)
		{ //hack, don't do while moving
			return;
		}
		if ( taunt != TAUNT_TAUNT )
		{//normal taunt always allowed
			if ( level.gametype != GT_DUEL && level.gametype != GT_POWERDUEL )
			{//no taunts unless in Duel
				return;
			}
		}
	}
#endif

	BG_ClearRocketLock(&ent->client->ps);// fix: rocket lock bug

	if ( ent->client->ps.torsoTimer < 1 
		&& ent->client->ps.forceHandExtend == HANDEXTEND_NONE 
		&& ent->client->ps.legsTimer < 1 
		&& ent->client->ps.weaponTime < 1 
		&& ent->client->ps.saberLockTime < level.time )
	{
		int anim = -1;
		switch ( taunt )
		{
		case TAUNT_TAUNT:
			if ( ent->client->ps.weapon != WP_SABER )
			{
				anim = BOTH_ENGAGETAUNT;
			}
			else if ( ent->client->saber[0].tauntAnim != -1 )
			{
				anim = ent->client->saber[0].tauntAnim;
			}
			else if ( ent->client->saber[1].model 
					&& ent->client->saber[1].model[0]
					&& ent->client->saber[1].tauntAnim != -1 )
			{
				anim = ent->client->saber[1].tauntAnim;
			}
			else
			{
				switch ( ent->client->ps.fd.saberAnimLevel )
				{
				case SS_FAST:
				case SS_TAVION:
					if ( ent->client->ps.saberHolstered == 1 
						&& ent->client->saber[1].model 
						&& ent->client->saber[1].model[0] )
					{//turn off second saber
						G_Sound( ent, CHAN_WEAPON, ent->client->saber[1].soundOff );
					}
					else if ( ent->client->ps.saberHolstered == 0 )
					{//turn off first
						G_Sound( ent, CHAN_WEAPON, ent->client->saber[0].soundOff );
					}
					ent->client->ps.saberHolstered = 2;
					anim = BOTH_GESTURE1;
					break;
				case SS_MEDIUM:
				case SS_STRONG:
				case SS_DESANN:
					anim = BOTH_ENGAGETAUNT;
					break;
				case SS_DUAL:
					if ( ent->client->ps.saberHolstered == 1 
						&& ent->client->saber[1].model 
						&& ent->client->saber[1].model[0] )
					{//turn on second saber
						G_Sound( ent, CHAN_WEAPON, ent->client->saber[1].soundOn );
					}
					else if ( ent->client->ps.saberHolstered == 2 )
					{//turn on first
						G_Sound( ent, CHAN_WEAPON, ent->client->saber[0].soundOn );
					}
					ent->client->ps.saberHolstered = 0;
					anim = BOTH_DUAL_TAUNT;
					break;
				case SS_STAFF:
					if ( ent->client->ps.saberHolstered > 0 )
					{//turn on all blades
						G_Sound( ent, CHAN_WEAPON, ent->client->saber[0].soundOn );
					}
					ent->client->ps.saberHolstered = 0;
					anim = BOTH_STAFF_TAUNT;
					break;
				}
			}
			break;
		case TAUNT_BOW:
			if ( ent->client->saber[0].bowAnim != -1 )
			{
				anim = ent->client->saber[0].bowAnim;
			}
			else if ( ent->client->saber[1].model 
					&& ent->client->saber[1].model[0]
					&& ent->client->saber[1].bowAnim != -1 )
			{
				anim = ent->client->saber[1].bowAnim;
			}
			else
			{
				anim = BOTH_BOW;
			}
			if (ent->client->ps.weapon == WP_SABER) //JAPRO - Serverside - Bow saber sound fix
			{
				if ( ent->client->ps.saberHolstered == 1 && ent->client->saber[1].model && ent->client->saber[1].model[0] )
				{//turn off second saber
					G_Sound( ent, CHAN_WEAPON, ent->client->saber[1].soundOff );
				}
				else if ( ent->client->ps.saberHolstered == 0 )
				{//turn off first
					G_Sound( ent, CHAN_WEAPON, ent->client->saber[0].soundOff );
				}
			}
			ent->client->ps.saberHolstered = 2;
			break;
		case TAUNT_MEDITATE:
			if ( ent->client->saber[0].meditateAnim != -1 )
			{
				anim = ent->client->saber[0].meditateAnim;
			}
			else if ( ent->client->saber[1].model 
					&& ent->client->saber[1].model[0]
					&& ent->client->saber[1].meditateAnim != -1 )
			{
				anim = ent->client->saber[1].meditateAnim;
			}
			else
			{
				anim = BOTH_MEDITATE;
			}
			if (ent->client->ps.weapon == WP_SABER) //JAPRO - Serverside - Saber bow sound fix
			{
				if ( ent->client->ps.saberHolstered == 1 && ent->client->saber[1].model && ent->client->saber[1].model[0] )
				{//turn off second saber
					G_Sound( ent, CHAN_WEAPON, ent->client->saber[1].soundOff );
				}
				else if ( ent->client->ps.saberHolstered == 0 )
				{//turn off first
					G_Sound( ent, CHAN_WEAPON, ent->client->saber[0].soundOff );
				}
			}
			ent->client->ps.saberHolstered = 2;
			break;
		case TAUNT_FLOURISH:
			if ( ent->client->ps.weapon == WP_SABER )
			{
				if ( ent->client->ps.saberHolstered == 1 
					&& ent->client->saber[1].model 
					&& ent->client->saber[1].model[0] )
				{//turn on second saber
					G_Sound( ent, CHAN_WEAPON, ent->client->saber[1].soundOn );
				}
				else if ( ent->client->ps.saberHolstered == 2 )
				{//turn on first
					G_Sound( ent, CHAN_WEAPON, ent->client->saber[0].soundOn );
				}
				ent->client->ps.saberHolstered = 0;
				if ( ent->client->saber[0].flourishAnim != -1 )
				{
					anim = ent->client->saber[0].flourishAnim;
				}
				else if ( ent->client->saber[1].model 
					&& ent->client->saber[1].model[0]
					&& ent->client->saber[1].flourishAnim != -1 )
				{
					anim = ent->client->saber[1].flourishAnim;
				}
				else
				{
					switch ( ent->client->ps.fd.saberAnimLevel )
					{
					case SS_FAST:
					case SS_TAVION:
						anim = BOTH_SHOWOFF_FAST;
						break;
					case SS_MEDIUM:
						anim = BOTH_SHOWOFF_MEDIUM;
						break;
					case SS_STRONG:
					case SS_DESANN:
						anim = BOTH_SHOWOFF_STRONG;
						break;
					case SS_DUAL:
						anim = BOTH_SHOWOFF_DUAL;
						break;
					case SS_STAFF:
						anim = BOTH_SHOWOFF_STAFF;
						break;
					}
				}
			}
			break;
		case TAUNT_GLOAT:
			if ( ent->client->ps.weapon == WP_SABER )//JAPRO - Serverside - Fix Gloat saber sound
			{
				if ( ent->client->saber[0].gloatAnim != -1 )
				{
					anim = ent->client->saber[0].gloatAnim;
				}
				else if ( ent->client->saber[1].model 
						&& ent->client->saber[1].model[0]
						&& ent->client->saber[1].gloatAnim != -1 )
				{
					anim = ent->client->saber[1].gloatAnim;
				}
				else
				{
					switch ( ent->client->ps.fd.saberAnimLevel )
					{
					case SS_FAST:
					case SS_TAVION:
						anim = BOTH_VICTORY_FAST;
						break;
					case SS_MEDIUM:
						anim = BOTH_VICTORY_MEDIUM;
						break;
					case SS_STRONG:
					case SS_DESANN:
						if ( ent->client->ps.saberHolstered )
						{//turn on first
							G_Sound( ent, CHAN_WEAPON, ent->client->saber[0].soundOn );
						}
						ent->client->ps.saberHolstered = 0;
						anim = BOTH_VICTORY_STRONG;
						break;
					case SS_DUAL:
						if ( ent->client->ps.saberHolstered == 1 
							&& ent->client->saber[1].model 
							&& ent->client->saber[1].model[0] )
						{//turn on second saber
							G_Sound( ent, CHAN_WEAPON, ent->client->saber[1].soundOn );
						}
						else if ( ent->client->ps.saberHolstered == 2 )
						{//turn on first
							G_Sound( ent, CHAN_WEAPON, ent->client->saber[0].soundOn );
						}
						ent->client->ps.saberHolstered = 0;
						anim = BOTH_VICTORY_DUAL;
						break;
					case SS_STAFF:
						if ( ent->client->ps.saberHolstered )
						{//turn on first
							G_Sound( ent, CHAN_WEAPON, ent->client->saber[0].soundOn );
						}
						ent->client->ps.saberHolstered = 0;
						anim = BOTH_VICTORY_STAFF;
						break;
					}
				}
				break;
			}
		}
		if ( anim != -1 )
		{
			if ( ent->client->ps.groundEntityNum != ENTITYNUM_NONE ) 
			{
				ent->client->ps.forceHandExtend = HANDEXTEND_TAUNT;
				ent->client->ps.forceDodgeAnim = anim;
				ent->client->ps.forceHandExtendTime = level.time + BG_AnimLength(ent->localAnimIndex, (animNumber_t)anim);
			}
			if ( taunt != TAUNT_MEDITATE 
				&& taunt != TAUNT_BOW )
			{//no sound for meditate or bow
				G_AddEvent( ent, EV_TAUNT, taunt );
			}
		}
	}
}

extern void G_TestLine(vec3_t start, vec3_t end, int color, int time);
void TryTargettingLaser( gentity_t *ent ) {
	//BUTTON_TARGET
	//Only gets called if we are ingame i think
	//Check if we are alive
	//This should really be a clientside FX based on players viewangles, idk.

	vec3_t start, end, view;
	trace_t trace;

	if (!ent || !ent->client)
		return;
	if (ent->health <= 0)
		return;
	if (ent->client->lastTargetLaserTime > level.time)
		return;

	ent->client->lastTargetLaserTime = level.time + 250;

	AngleVectors( ent->client->ps.viewangles, view, NULL, NULL );

	VectorCopy( ent->client->ps.origin, start );
	start[2] += ent->client->ps.viewheight;
	VectorMA( start, 16, view, start ); //znear
	VectorMA( start, 8192, view, end ); //max distance

	trap->Trace(&trace, start, NULL, NULL, end, ent->s.number, CONTENTS_SOLID, qfalse, 0, 0);

	G_TestLine(start, trace.endpos, 0x00ff00, 250); //check trace.fraction? ehh trace.startsolid or whatever?
}

void G_AddDuel(char *winner, char *loser, int start_time, int type, int winner_hp, int winner_shield);
void GiveClientWeapons(gclient_t *client);
/*
==============
ClientThink

This will be called once for each client frame, which will
usually be a couple times for each server frame on fast clients.

If "g_synchronousClients 1" is set, this will be called exactly
once for each server frame, which makes for smooth demo recording.
==============
*/
qboolean G_SetSaber(gentity_t *ent, int saberNum, char *saberName, qboolean siegeOverride);
qboolean G_SaberModelSetup(gentity_t *ent);
#if _GRAPPLE
void Weapon_GrapplingHook_Fire (gentity_t *ent);
void Weapon_HookFree (gentity_t *ent);
void Weapon_HookThink (gentity_t *ent);
#endif

qboolean CanGrapple( gentity_t *ent ) {
	if (!ent || !ent->client)
		return qfalse;
	if (!g_allowGrapple.integer && !ent->client->sess.raceMode)
		return qfalse;
	if (ent->client->sess.raceMode && ent->client->sess.movementStyle != MV_JETPACK)
		return qfalse;
	if (ent->client->ps.duelInProgress)
		return qfalse;
	if (!BG_SaberInIdle(ent->client->ps.saberMove))
		return qfalse;
	if (BG_InRoll(&ent->client->ps, ent->client->ps.legsAnim))
		return qfalse;
	if (BG_InSpecialJump(ent->client->ps.legsAnim))
		return qfalse;	
	return qtrue;
}

qboolean CanFireGrapple( gentity_t *ent ) { // Adapt for new hold-to-use jetpack?
	if (!ent || !ent->client)
		return qfalse;
	if (!g_allowGrapple.integer && !ent->client->sess.raceMode)
		return qfalse;
	if (ent->client->sess.raceMode && ent->client->sess.movementStyle != MV_JETPACK)
		return qfalse;
	if (ent->client->ps.duelInProgress)
		return qfalse;
	if (ent->client->jetPackOn)
		return qfalse;
	if (!BG_SaberInIdle(ent->client->ps.saberMove))
		return qfalse;
	if (BG_InRoll(&ent->client->ps, ent->client->ps.legsAnim))
		return qfalse;
	if (BG_InSpecialJump(ent->client->ps.legsAnim))
		return qfalse;	
	return qtrue;
}

void ClientThink_real( gentity_t *ent ) {
	gclient_t	*client;
	pmove_t		pmove;
	int			oldEventSequence;
	int			msec;
	usercmd_t	*ucmd;
	qboolean	isNPC = qfalse;
	qboolean	controlledByPlayer = qfalse;
	qboolean	killJetFlags = qtrue;
	qboolean	isFollowing;

	client = ent->client;

	if (ent->s.eType == ET_NPC)
	{
		isNPC = qtrue;
	}

	// don't think if the client is not yet connected (and thus not yet spawned in)
	if (client->pers.connected != CON_CONNECTED && !isNPC) {
		return;
	}

	// This code was moved here from clientThink to fix a problem with g_synchronousClients 
	// being set to 1 when in vehicles. 
	if ( ent->s.number < MAX_CLIENTS && ent->client->ps.m_iVehicleNum )
	{//driving a vehicle
		if (g_entities[ent->client->ps.m_iVehicleNum].client)
		{
			gentity_t *veh = &g_entities[ent->client->ps.m_iVehicleNum];

			if (veh->m_pVehicle &&
				veh->m_pVehicle->m_pPilot == (bgEntity_t *)ent)
			{ //only take input from the pilot...
				veh->client->ps.commandTime = ent->client->ps.commandTime;
				memcpy(&veh->m_pVehicle->m_ucmd, &ent->client->pers.cmd, sizeof(usercmd_t));
				if ( veh->m_pVehicle->m_ucmd.buttons & BUTTON_TALK )
				{ //forced input if "chat bubble" is up
					veh->m_pVehicle->m_ucmd.buttons = BUTTON_TALK;
					veh->m_pVehicle->m_ucmd.forwardmove = 0;
					veh->m_pVehicle->m_ucmd.rightmove = 0;
					veh->m_pVehicle->m_ucmd.upmove = 0;
				}
			}
		}
	}

	isFollowing = (client->ps.pm_flags & PMF_FOLLOW) ? qtrue : qfalse;

	if (!isFollowing)
	{
		qboolean forceSingle = qfalse;
		qboolean changeStyle = qfalse;
		if (g_saberDisable.integer && (ent->s.weapon == WP_SABER) && (ent->s.eType == ET_PLAYER) && !client->sess.raceMode && (client->sess.sessionTeam != TEAM_SPECTATOR)) { //single style

			//trap->Print("AnimLevel: %i, DrawLevel: %i, Baselevel: %i\n", ent->client->ps.fd.saberAnimLevel, ent->client->ps.fd.saberDrawAnimLevel, ent->client->ps.fd.saberAnimLevelBase);

			if (g_saberDisable.integer & SABERSTYLE_DESANN) { //Force Desann
				client->ps.fd.saberAnimLevel = client->ps.fd.saberDrawAnimLevel = client->ps.fd.saberAnimLevelBase = SS_DESANN;
			}
			else if (g_saberDisable.integer & SABERSTYLE_TAVION) { //Force Tavion
				client->ps.fd.saberAnimLevel = client->ps.fd.saberDrawAnimLevel = client->ps.fd.saberAnimLevelBase = SS_TAVION;
			}
			else if (client->ps.fd.saberAnimLevel == SS_FAST) {
				if (g_saberDisable.integer & SABERSTYLE_BLUE) //No blue
					changeStyle = qtrue;
			}
			else if (client->ps.fd.saberAnimLevel == SS_MEDIUM) {
				if (g_saberDisable.integer & SABERSTYLE_YELLOW) //No yellow
					changeStyle = qtrue;
			}
			else if (client->ps.fd.saberAnimLevel == SS_STRONG) {
				if (g_saberDisable.integer & SABERSTYLE_RED) //No red
					changeStyle = qtrue;
			}
			else if (client->ps.fd.saberAnimLevel == SS_DUAL) {
				if (g_saberDisable.integer & SABERSTYLE_DUAL) //No duals
					forceSingle = qtrue;
			}
			else if (client->ps.fd.saberAnimLevel == SS_STAFF) {
				if (g_saberDisable.integer & SABERSTYLE_STAFF) //No staff
					forceSingle = qtrue;
			}

			if ((g_saberDisable.integer & SABERSTYLE_STAFF) && (g_saberDisable.integer & SABERSTYLE_DUAL) && (Q_stricmp(client->pers.saber1, "kyle") || Q_stricmp(client->pers.saber2, "none"))) {//No staff or duel.. force single kyle saber if not already.
				forceSingle = qtrue;
			}

			if (forceSingle) {
				char userinfo[MAX_INFO_STRING] = {0};

				trap->GetUserinfo(ent-g_entities, userinfo, sizeof(userinfo));
				G_SetSaber(ent, 0, "kyle", qfalse);
				G_SetSaber(ent, 1, "none", qfalse);
				trap->SetUserinfo(ent-g_entities, userinfo);
				ClientUserinfoChanged(ent-g_entities);
				G_SaberModelSetup(ent);

				if (g_saberDisable.integer & SABERSTYLE_DESANN)
					client->ps.fd.saberAnimLevel = client->ps.fd.saberDrawAnimLevel = client->ps.fd.saberAnimLevelBase = SS_DESANN;
				else if (g_saberDisable.integer & SABERSTYLE_TAVION)
					client->ps.fd.saberAnimLevel = client->ps.fd.saberDrawAnimLevel = client->ps.fd.saberAnimLevelBase = SS_TAVION;
				else if (client->ps.fd.saberAnimLevel == SS_DUAL)
					client->ps.fd.saberAnimLevel = client->ps.fd.saberDrawAnimLevel = client->ps.fd.saberAnimLevelBase = SS_FAST;
				else if (client->ps.fd.saberAnimLevel == SS_STAFF)
					client->ps.fd.saberAnimLevel = client->ps.fd.saberDrawAnimLevel = client->ps.fd.saberAnimLevelBase = SS_MEDIUM;

				if ((g_saberDisable.integer & SABERSTYLE_RED) && (client->ps.fd.saberAnimLevel == SS_STRONG))
					Cmd_SaberAttackCycle_f(ent);
				else if ((g_saberDisable.integer & SABERSTYLE_YELLOW) && (client->ps.fd.saberAnimLevel == SS_MEDIUM))
					Cmd_SaberAttackCycle_f(ent);
				else if ((g_saberDisable.integer & SABERSTYLE_BLUE) && (client->ps.fd.saberAnimLevel == SS_FAST))
					Cmd_SaberAttackCycle_f(ent);
			}
			else if (changeStyle) {
				Cmd_SaberAttackCycle_f(ent); //meh, could still be cleaned up but w/e .. only buggs out a lil bit at spawn when changing from duals/staff -> single and a single style is disabled?
			}	
		}
		
		if (!forceSingle && !changeStyle &&
			level.gametype == GT_SIEGE &&
			client->siegeClass != -1 &&
			bgSiegeClasses[client->siegeClass].saberStance)
		{ //the class says we have to use this stance set.
			if (!(bgSiegeClasses[client->siegeClass].saberStance & (1 << client->ps.fd.saberAnimLevel)))
			{ //the current stance is not in the bitmask, so find the first one that is.
				int i = SS_FAST;

				while (i < SS_NUM_SABER_STYLES)
				{
					if (bgSiegeClasses[client->siegeClass].saberStance & (1 << i))
					{
						if (i == SS_DUAL 
							&& client->ps.saberHolstered == 1 )
						{//one saber should be off, adjust saberAnimLevel accordinly
							client->ps.fd.saberAnimLevelBase = i;
							client->ps.fd.saberAnimLevel = SS_FAST;
							client->ps.fd.saberDrawAnimLevel = client->ps.fd.saberAnimLevel;
						}
						else if ( i == SS_STAFF
							&& client->ps.saberHolstered == 1 
							&& client->saber[0].singleBladeStyle != SS_NONE)
						{//one saber or blade should be off, adjust saberAnimLevel accordinly
							client->ps.fd.saberAnimLevelBase = i;
							client->ps.fd.saberAnimLevel = client->saber[0].singleBladeStyle;
							client->ps.fd.saberDrawAnimLevel = client->ps.fd.saberAnimLevel;
						}
						else
						{
							client->ps.fd.saberAnimLevelBase = client->ps.fd.saberAnimLevel = i;
							client->ps.fd.saberDrawAnimLevel = i;
						}
						break;
					}

					i++;
				}
			}
		}
		else if (client->saber[0].model[0] && client->saber[1].model[0])
		{ //with two sabs always use akimbo style
			if ( client->ps.saberHolstered == 1 )
			{//one saber should be off, adjust saberAnimLevel accordinly
				client->ps.fd.saberAnimLevelBase = SS_DUAL;
				client->ps.fd.saberAnimLevel = SS_FAST;
				client->ps.fd.saberDrawAnimLevel = client->ps.fd.saberAnimLevel;
			}
			else
			{
				if ( !WP_SaberStyleValidForSaber( &client->saber[0], &client->saber[1], client->ps.saberHolstered, client->ps.fd.saberAnimLevel ) )
				{//only use dual style if the style we're trying to use isn't valid
					client->ps.fd.saberAnimLevelBase = client->ps.fd.saberAnimLevel = SS_DUAL;
				}
				client->ps.fd.saberDrawAnimLevel = client->ps.fd.saberAnimLevel;
			}
		}
		else
		{
			if (client->saber[0].stylesLearned == (1<<SS_STAFF) )
			{ //then *always* use the staff style
				client->ps.fd.saberAnimLevelBase = SS_STAFF;
			}
			if ( client->ps.fd.saberAnimLevelBase == SS_STAFF )
			{//using staff style
				if ( client->ps.saberHolstered == 1 
					&& client->saber[0].singleBladeStyle != SS_NONE)
				{//one blade should be off, adjust saberAnimLevel accordinly
					client->ps.fd.saberAnimLevel = client->saber[0].singleBladeStyle;
					client->ps.fd.saberDrawAnimLevel = client->ps.fd.saberAnimLevel;
				}
				else
				{
					client->ps.fd.saberAnimLevel = SS_STAFF;
					client->ps.fd.saberDrawAnimLevel = client->ps.fd.saberAnimLevel;
				}
			}
		}
	}

	// mark the time, so the connection sprite can be removed
	ucmd = &ent->client->pers.cmd;

	if ( client && !isFollowing && (client->ps.eFlags2&EF2_HELD_BY_MONSTER) )
	{
		G_HeldByMonster( ent, ucmd );
	}

	// sanity check the command time to prevent speedup cheating
	if ( ucmd->serverTime > level.time + 200 ) {
		ucmd->serverTime = level.time + 200;
//		trap->Print("serverTime <<<<<\n" );
	}
	else if ( ucmd->serverTime < level.time - 1000 ) {
		ucmd->serverTime = level.time - 1000;
//		trap->Print("serverTime >>>>>\n" );
	} 

#if 0
//
	if (!isNPC && client && client->sess.sessionTeam == TEAM_FREE && client->pers.raceMode) {
		//trap->Print("k: %i\n", client->pers.cmd.angles[YAW] - client->pers.lastCmd.angles[YAW]);
		if ((client->pers.cmd.angles[YAW] != client->pers.lastCmd.angles[YAW])) { //Clients aim has changed this frame, so run the tests
			const int delta = client->pers.cmd.angles[YAW] - client->pers.lastCmd.angles[YAW];
			const int AIM_SAMPLES = 64;
			int i, total = 0, average, varianceTotal = 0;

			if (delta < 6500 && delta > -6500) { //who knows

				client->pers.aimSamples[client->pers.aimCount % AIM_SAMPLES] = delta; //fuckign broken thing
				client->pers.aimCount++;

				for (i=0; i<AIM_SAMPLES; i++) {
					total += client->pers.aimSamples[i];
				}

				average = total / AIM_SAMPLES;

				for (i=0; i<AIM_SAMPLES;i++) {
					varianceTotal += (client->pers.aimSamples[i] - average) * (client->pers.aimSamples[i] - average);
				}

				if (varianceTotal < 500) {
					trap->SendServerCommand( ent-g_entities, "chat \"Yawspeed detected\n\"");
				}

				trap->Print("Variance: %i, average: %i, delta %i\n", varianceTotal, average, delta);

			}

			//Sometimes change is +-61536.. why?

			//Goal: Detect a change in sensitivity by analyzing delta history
			//If the lowest factor of delta changes, the sensitivity has changed?  But it does not guarnetee to find every sens change..

			//well, mouse accel completely fucks this over... hmm?


		}

		client->pers.lastCmd = client->pers.cmd;
	}


#endif

#if 0
	//Set sad hack of userinput 
	if (ucmd->forwardmove > 0) //w
		client->ps.userInt1 = 1;
	if (ucmd->rightmove < 0) //a
		client->ps.userInt2 = 1;
	if (ucmd->forwardmove < 0) //s
		client->ps.userInt3 = 1;
	if (ucmd->rightmove > 0) //d
		client->ps.userFloat1 = 1;
	if (ucmd->upmove > 0) //up
		client->ps.userFloat2 = 1;
	if (ucmd->upmove < 0) //down
		client->ps.userFloat3 = 1;
	if (ucmd->buttons & BUTTON_WALKING)
		client->ps.userVec1[0] = 1;
#endif

	if (isNPC && (ucmd->serverTime - client->ps.commandTime) < 1)
	{
		ucmd->serverTime = client->ps.commandTime + 100;
	}

	client->lastUpdateFrame = level.framenum; //Unlagged

	msec = ucmd->serverTime - client->ps.commandTime;
	// following others may result in bad times, but we still want
	// to check for follow toggles
	if ( msec < 1 && client->sess.spectatorState != SPECTATOR_FOLLOW ) {
		return;
	}

	if ( msec > 200 ) {
		msec = 200;
	}

	if ( pmove_msec.integer <= 1 ) {
		trap->Cvar_Set("pmove_msec", "1");
	}
	else if (pmove_msec.integer > 66) {
		trap->Cvar_Set("pmove_msec", "66");
	}

	if (!isNPC && !(ent->r.svFlags & SVF_BOT) && client->sess.sessionTeam != TEAM_SPECTATOR && g_forceLogin.integer && !client->pers.userName[0]) {
		SetTeam ( ent, "spectator", qtrue );
		trap->SendServerCommand( ent-g_entities, "print \"^1You must login to join the game\n\"");
	}

	if (!isNPC && client->sess.sessionTeam == TEAM_FREE && !g_raceMode.integer) {
		if (client->ps.stats[STAT_RACEMODE] || level.gametype >= GT_TEAM) {
			SetTeam ( ent, "spectator", qtrue );
			client->sess.raceMode = qfalse;
			client->ps.stats[STAT_RACEMODE] = qfalse;
		}
	}
	
	/*if (client->ps.stats[STAT_RACEMODE] && client->ps.stats[STAT_MOVEMENTSTYLE] != MV_SWOOP)//Is this really needed..
		ucmd->serverTime = ((ucmd->serverTime + 7) / 8) * 8;//Integer math was making this bad, but is this even really needed? I guess for 125fps bhop height it is?
	else*/if (pmove_fixed.integer || client->pers.pmoveFixed)
		ucmd->serverTime = ((ucmd->serverTime + pmove_msec.integer-1) / pmove_msec.integer) * pmove_msec.integer;

	if ((client->sess.sessionTeam != TEAM_SPECTATOR) && !client->ps.stats[STAT_RACEMODE] && (g_movementStyle.integer >= MV_SIEGE && g_movementStyle.integer <= MV_WSW) || g_movementStyle.integer == MV_SP || g_movementStyle.integer == MV_SLICK) { //Ok,, this should be like every frame, right??
		client->sess.movementStyle = g_movementStyle.integer;
	}
	client->ps.stats[STAT_MOVEMENTSTYLE] = client->sess.movementStyle;

	if (g_rabbit.integer && client->ps.powerups[PW_NEUTRALFLAG]) {
		if (client->ps.fd.forcePowerLevel[FP_LEVITATION] > 1) {
			client->savedJumpLevel = client->ps.fd.forcePowerLevel[FP_LEVITATION];
			client->ps.fd.forcePowerLevel[FP_LEVITATION] = 1;
		}
	}
	else if (client->savedJumpLevel) {
		client->ps.fd.forcePowerLevel[FP_LEVITATION] = client->savedJumpLevel;
	}
	if (client->ps.stats[STAT_RACEMODE]) {
			client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] = 3; //make sure its allowed on server? or?
		if (client->ps.stats[STAT_ONLYBHOP])
			client->ps.fd.forcePowerLevel[FP_LEVITATION] = 3;
	}

	//
	// check for exiting intermission
	//
	if ( level.intermissiontime ) 
	{
		if ( ent->s.number < MAX_CLIENTS
			|| client->NPC_class == CLASS_VEHICLE )
		{//players and vehicles do nothing in intermissions
			ClientIntermissionThink( client );
			return;
		}
	}

	// spectators don't do much
	if ( client->sess.sessionTeam == TEAM_SPECTATOR || client->tempSpectate > level.time ) {
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD ) {
			return;
		}
		SpectatorThink( ent, ucmd );
		return;
	}

	if (ent && ent->client && (ent->client->ps.eFlags & EF_INVULNERABLE))
	{
		if (ent->client->invulnerableTimer <= level.time)
		{
			ent->client->ps.eFlags &= ~EF_INVULNERABLE;
		}
	}

	if (ent && ent->client && level.gametype == GT_FFA && !ent->client->ps.powerups[PW_NEUTRALFLAG] && ent->client->sess.sessionTeam == TEAM_FREE) {//JAPRO - Serverside - God chat :(
		if ((ent->client->ps.eFlags & EF_TALK) && !ent->client->pers.chatting) {
			ent->client->pers.chatting = qtrue;
			ent->client->pers.lastChatTime = level.time;
		}
		else if (!(ent->client->ps.eFlags & EF_TALK)) {
			ent->client->pers.chatting = qfalse;
			if (!sv_cheats.integer)
				ent->flags &= ~FL_GODMODE;
			ent->takedamage = qtrue;
		}	
		else if (g_godChat.integer && level.gametype == GT_FFA && (ent->client->ps.eFlags & EF_TALK) && (ent->client->pers.lastChatTime + 3000) < level.time) { //Only god chat them 3 seconds after their chatbox goes up to prevent some abuse :/
			if (!ent->client->ps.duelInProgress) {
				if (!sv_cheats.integer)
					ent->flags |= FL_GODMODE;
				ent->takedamage = qfalse;
			}
		}
	} // Godchat end

	if (ent && ent->client && ent->client->sess.raceMode) {
		const int movementStyle = ent->client->sess.movementStyle;
		if (movementStyle == MV_RJCPM || movementStyle == MV_RJQ3) {
			ent->client->ps.stats[STAT_WEAPONS] = (1 << WP_MELEE) + (1 << WP_SABER) + (1 << WP_ROCKET_LAUNCHER);
			ent->client->ps.ammo[AMMO_ROCKETS] = 2;
			if (ent->health > 0)
				ent->client->ps.stats[STAT_ARMOR] = ent->client->ps.stats[STAT_HEALTH] = ent->health = 100;
		}
		else {
			client->ps.ammo[AMMO_POWERCELL] = 300;

			if (movementStyle == MV_SIEGE || movementStyle == MV_JKA || movementStyle == MV_QW || movementStyle == MV_PJK || movementStyle == MV_SP || movementStyle == MV_SPEED || movementStyle == MV_JETPACK) {
				ent->client->ps.stats[STAT_WEAPONS] = (1 << WP_MELEE) + (1 << WP_SABER) + (1 << WP_DISRUPTOR) + (1 << WP_STUN_BATON);
			}
			else {
				ent->client->ps.stats[STAT_WEAPONS] = (1 << WP_MELEE) + (1 << WP_SABER) + (1 << WP_DISRUPTOR);
			}
		}

		if (movementStyle == MV_JETPACK) //always give jetpack style a jetpack, and non jetpack styles no jetpack, maybe this should just be in clientspawn ?
			ent->client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << HI_JETPACK);
		else
			ent->client->ps.stats[STAT_HOLDABLE_ITEMS] &= ~(1 << HI_JETPACK); 
	}


	if (ent->s.eType != ET_NPC)
	{
		// check for inactivity timer, but never drop the local client of a non-dedicated server
		if ( !ClientInactivityTimer( client ) ) {
			return;
		}
	}

	//Check if we should have a fullbody push effect around the player
	if (client->pushEffectTime > level.time)
	{
		client->ps.eFlags |= EF_BODYPUSH;
	}
	else if (client->pushEffectTime)
	{
		client->pushEffectTime = 0;
		client->ps.eFlags &= ~EF_BODYPUSH;
	}

	if (client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK))
	{
		client->ps.eFlags |= EF_JETPACK;
	}
	else
	{
		client->ps.eFlags &= ~EF_JETPACK;
	}

	//OSP: pause
	if ( level.pause.state != PAUSE_NONE && !client->sess.raceMode) { //Only pause non racers?
		ucmd->buttons = ucmd->generic_cmd = 0;
		ucmd->forwardmove = ucmd->rightmove = ucmd->upmove = 0;
		client->ps.pm_type = PM_FREEZE;
	}
	else  if ( client->noclip ) {
		client->ps.pm_type = PM_NOCLIP;
	} else if ( client->ps.eFlags & EF_DISINTEGRATION ) {
		client->ps.pm_type = PM_NOCLIP;
	} else if ( client->ps.stats[STAT_HEALTH] <= 0 ) {
		client->ps.pm_type = PM_DEAD;
	} else {
		if (client->ps.forceGripChangeMovetype)
		{
			client->ps.pm_type = client->ps.forceGripChangeMovetype;
		}
		else//Start
		{

			/*
			if (client->pers.protect)
			{
				if (client->pers.cmd.forwardmove || client->pers.cmd.rightmove || client->pers.cmd.upmove || client->pers.cmd.buttons & BUTTON_ALT_ATTACK ||
					client->pers.cmd.buttons & BUTTON_ATTACK || client->ps.eFlags2 == EF2_HELD_BY_MONSTER || client->ps.duelInProgress) {
					ent->takedamage = qtrue;
					client->ps.eFlags &= ~EF_INVULNERABLE;
					client->pers.protect = qfalse;
				}
				else {
					ent->takedamage = qfalse;
					client->ps.eFlags |= EF_INVULNERABLE;
				}
			}
			*/

			if (client->pers.amfreeze)
			{
				if (!client->ps.duelInProgress)
				{
					client->ps.pm_type = PM_FREEZE;
					client->ps.saberAttackWound = 0;
					client->ps.saberIdleWound = 0;
					client->ps.saberCanThrow = qfalse;
					client->ps.forceRestricted = qtrue;
					ent->takedamage = qfalse;
					if (client->ps.weapon == WP_SABER)
					{ //make sure their saber is shut off
						if ( client->ps.saberHolstered < 2 )
						{
							client->ps.saberHolstered = 2;
							client->ps.weaponTime = 400;
						}
					}
				}
			}
			else if (client->emote_freeze)
			{ //unfreeze if we are being gripped i guess rite
				if (client->ps.fd.forceGripCripple || client->pers.cmd.forwardmove || client->pers.cmd.rightmove || client->pers.cmd.upmove || client->ps.eFlags2 == EF2_HELD_BY_MONSTER || client->buttons & BUTTON_ATTACK || client->buttons & BUTTON_ALT_ATTACK || client->buttons & BUTTON_DASH)
				{
					client->emote_freeze = qfalse;
					client->ps.saberCanThrow = qtrue;
					client->ps.saberBlocked = 1;
					client->ps.saberBlocking = 1;
					client->ps.forceRestricted = qfalse;
				}
				else 
				{
					client->ps.pm_type = PM_FREEZE;
					client->ps.forceRestricted = qtrue;
					if (ent->client->ps.weapon == WP_SABER && ent->client->ps.saberHolstered < 2)
					{
						if (ent->client->saber[0].soundOff)
						{
							G_Sound(ent, CHAN_AUTO, ent->client->saber[0].soundOff);
						}
						if (ent->client->saber[1].soundOff && ent->client->saber[1].model[0])
						{
							G_Sound(ent, CHAN_AUTO, ent->client->saber[1].soundOff);
						}
						ent->client->ps.saberHolstered = 2;
						ent->client->ps.weaponTime = 400;
					}
				}
			}
			//End
			else if (client->jetPackOn)
			{
				client->ps.pm_type = PM_JETPACK; //terrible to set this here where it cant be predicted?
				client->ps.eFlags |= EF_JETPACK_ACTIVE;
				killJetFlags = qfalse;
			}

//JAPRO - Serverside - jetpack - Effects - Start - Do this in pmove now..
			/*
			else if ((g_tweakJetpack.integer || ent->client->sess.raceMode) && ent->client->pers.cmd.buttons & BUTTON_JETPACK && BG_CanJetpack(&client->ps))
			{
				client->ps.eFlags |= EF_JETPACK_ACTIVE;
				killJetFlags = qfalse;
			}
			*/
			else if ((g_tweakJetpack.integer || client->sess.raceMode) && client->ps.eFlags & EF_JETPACK_ACTIVE) {
				killJetFlags = qfalse;
			}
//JAPRO - Serverside - jetpack - Effects - End

			else
			{
				client->ps.pm_type = PM_NORMAL;
			}
		}
	}

	if (killJetFlags)
	{
		client->ps.eFlags &= ~EF_JETPACK_ACTIVE;
		client->ps.eFlags &= ~EF_JETPACK_FLAMING;
	}

#define	SLOWDOWN_DIST	128.0f
#define	MIN_NPC_SPEED	16.0f

	if (client->bodyGrabIndex != ENTITYNUM_NONE)
	{
		gentity_t *grabbed = &g_entities[client->bodyGrabIndex];

		if (!grabbed->inuse || grabbed->s.eType != ET_BODY ||
			(grabbed->s.eFlags & EF_DISINTEGRATION) ||
			(grabbed->s.eFlags & EF_NODRAW))
		{
			if (grabbed->inuse && grabbed->s.eType == ET_BODY)
			{
				grabbed->s.ragAttach = 0;
			}
			client->bodyGrabIndex = ENTITYNUM_NONE;
		}
		else
		{
			mdxaBone_t rhMat;
			vec3_t rhOrg, tAng;
			vec3_t bodyDir;
			float bodyDist;

			ent->client->ps.forceHandExtend = HANDEXTEND_DRAGGING;

			if (ent->client->ps.forceHandExtendTime < level.time + 500)
			{
				ent->client->ps.forceHandExtendTime = level.time + 1000;
			}

			VectorSet(tAng, 0, ent->client->ps.viewangles[YAW], 0);
			trap->G2API_GetBoltMatrix(ent->ghoul2, 0, 0, &rhMat, tAng, ent->client->ps.origin, level.time,
				NULL, ent->modelScale); //0 is always going to be right hand bolt
			BG_GiveMeVectorFromMatrix(&rhMat, ORIGIN, rhOrg);

			VectorSubtract(rhOrg, grabbed->r.currentOrigin, bodyDir);
			bodyDist = VectorLength(bodyDir);

			if (bodyDist > 40.0f)
			{ //can no longer reach
				grabbed->s.ragAttach = 0;
				client->bodyGrabIndex = ENTITYNUM_NONE;
			}
			else if (bodyDist > 24.0f)
			{
				bodyDir[2] = 0; //don't want it floating
				//VectorScale(bodyDir, 0.1f, bodyDir);
				VectorAdd(grabbed->epVelocity, bodyDir, grabbed->epVelocity);
				G_Sound(grabbed, CHAN_AUTO, G_SoundIndex("sound/player/roll1.wav"));
			}
		}
	}
	else if (ent->client->ps.forceHandExtend == HANDEXTEND_DRAGGING)
	{
		ent->client->ps.forceHandExtend = HANDEXTEND_WEAPONREADY;
	}

	if (ent->NPC && ent->s.NPC_class != CLASS_VEHICLE) //vehicles manage their own speed
	{
		//FIXME: swoop should keep turning (and moving forward?) for a little bit?
		if ( ent->NPC->combatMove == qfalse )
		{
			//if ( !(ucmd->buttons & BUTTON_USE) )
			if (1)
			{//Not leaning
				qboolean Flying = (ucmd->upmove && (ent->client->ps.eFlags2&EF2_FLYING));//ent->client->moveType == MT_FLYSWIM);
				qboolean Climbing = (ucmd->upmove && ent->watertype&CONTENTS_LADDER );

				//client->ps.friction = 6;

				if ( ucmd->forwardmove || ucmd->rightmove || Flying )
				{
					//if ( ent->NPC->behaviorState != BS_FORMATION )
					{//In - Formation NPCs set thier desiredSpeed themselves
						if ( ucmd->buttons & BUTTON_WALKING )
						{
							ent->NPC->desiredSpeed = NPC_GetWalkSpeed( ent );//ent->NPC->stats.walkSpeed;
						}
						else//running
						{
							ent->NPC->desiredSpeed = NPC_GetRunSpeed( ent );//ent->NPC->stats.runSpeed;
						}

						if ( ent->NPC->currentSpeed >= 80 && !controlledByPlayer )
						{//At higher speeds, need to slow down close to stuff
							//Slow down as you approach your goal
						//	if ( ent->NPC->distToGoal < SLOWDOWN_DIST && client->race != RACE_BORG && !(ent->NPC->aiFlags&NPCAI_NO_SLOWDOWN) )//128
							if ( ent->NPC->distToGoal < SLOWDOWN_DIST && !(ent->NPC->aiFlags&NPCAI_NO_SLOWDOWN) )//128
							{
								if ( ent->NPC->desiredSpeed > MIN_NPC_SPEED )
								{
									float slowdownSpeed = ((float)ent->NPC->desiredSpeed) * ent->NPC->distToGoal / SLOWDOWN_DIST;

									ent->NPC->desiredSpeed = ceil(slowdownSpeed);
									if ( ent->NPC->desiredSpeed < MIN_NPC_SPEED )
									{//don't slow down too much
										ent->NPC->desiredSpeed = MIN_NPC_SPEED;
									}
								}
							}
						}
					}
				}
				else if ( Climbing )
				{
					ent->NPC->desiredSpeed = ent->NPC->stats.walkSpeed;
				}
				else
				{//We want to stop
					ent->NPC->desiredSpeed = 0;
				}

				NPC_Accelerate( ent, qfalse, qfalse );

				if ( ent->NPC->currentSpeed <= 24 && ent->NPC->desiredSpeed < ent->NPC->currentSpeed )
				{//No-one walks this slow
					client->ps.speed = ent->NPC->currentSpeed = 0;//Full stop
					ucmd->forwardmove = 0;
					ucmd->rightmove = 0;
				}
				else
				{
					if ( ent->NPC->currentSpeed <= ent->NPC->stats.walkSpeed )
					{//Play the walkanim
						ucmd->buttons |= BUTTON_WALKING;
					}
					else
					{
						ucmd->buttons &= ~BUTTON_WALKING;
					}

					if ( ent->NPC->currentSpeed > 0 )
					{//We should be moving
						if ( Climbing || Flying )
						{
							if ( !ucmd->upmove )
							{//We need to force them to take a couple more steps until stopped
								ucmd->upmove = ent->NPC->last_ucmd.upmove;//was last_upmove;
							}
						}
						else if ( !ucmd->forwardmove && !ucmd->rightmove )
						{//We need to force them to take a couple more steps until stopped
							ucmd->forwardmove = ent->NPC->last_ucmd.forwardmove;//was last_forwardmove;
							ucmd->rightmove = ent->NPC->last_ucmd.rightmove;//was last_rightmove;
						}
					}

					client->ps.speed = ent->NPC->currentSpeed;
				//	if ( player && player->client && player->client->ps.viewEntity == ent->s.number )
				//	{
				//	}
				//	else
					//rwwFIXMEFIXME: do this and also check for all real client
					if (1)
					{
						//Slow down on turns - don't orbit!!!
						float turndelta = 0; 
						// if the NPC is locked into a Yaw, we want to check the lockedDesiredYaw...otherwise the NPC can't walk backwards, because it always thinks it trying to turn according to desiredYaw
						//if( client->renderInfo.renderFlags & RF_LOCKEDANGLE ) // yeah I know the RF_ flag is a pretty ugly hack...
						if (0) //rwwFIXMEFIXME: ...
						{	
							turndelta = (180 - fabs( AngleDelta( ent->r.currentAngles[YAW], ent->NPC->lockedDesiredYaw ) ))/180;
						}
						else
						{
							turndelta = (180 - fabs( AngleDelta( ent->r.currentAngles[YAW], ent->NPC->desiredYaw ) ))/180;
						}
												
						if ( turndelta < 0.75f )
						{
							client->ps.speed = 0;
						}
						else if ( ent->NPC->distToGoal < 100 && turndelta < 1.0 )
						{//Turn is greater than 45 degrees or closer than 100 to goal
							client->ps.speed = floor(((float)(client->ps.speed))*turndelta);
						}
					}
				}
			}
		}
		else
		{	
			ent->NPC->desiredSpeed = ( ucmd->buttons & BUTTON_WALKING ) ? NPC_GetWalkSpeed( ent ) : NPC_GetRunSpeed( ent );

			client->ps.speed = ent->NPC->desiredSpeed;
		}

		if (ucmd->buttons & BUTTON_WALKING)
		{ //sort of a hack I guess since MP handles walking differently from SP (has some proxy cheat prevention methods)
			/*
			if (ent->client->ps.speed > 64)
			{
				ent->client->ps.speed = 64;
			}
			*/

			if (ucmd->forwardmove > 64)
			{
				ucmd->forwardmove = 64;	
			}
			else if (ucmd->forwardmove < -64)
			{
				ucmd->forwardmove = -64;
			}
			
			if (ucmd->rightmove > 64)
			{
				ucmd->rightmove = 64;
			}
			else if ( ucmd->rightmove < -64)
			{
				ucmd->rightmove = -64;
			}

			//ent->client->ps.speed = ent->client->ps.basespeed = NPC_GetRunSpeed( ent );
		}
		client->ps.basespeed = client->ps.speed;
	}
	else if (!client->ps.m_iVehicleNum &&
		(!ent->NPC || ent->s.NPC_class != CLASS_VEHICLE)) //if riding a vehicle it will manage our speed and such
	{
		// set speed

		client->ps.speed = g_speed.value;
		if (client->sess.raceMode || client->ps.stats[STAT_RACEMODE])
			client->ps.speed = 250.0f;
		if (client->ps.stats[STAT_MOVEMENTSTYLE] == MV_QW || client->ps.stats[STAT_MOVEMENTSTYLE] == MV_CPM || client->ps.stats[STAT_MOVEMENTSTYLE] == MV_Q3 || client->ps.stats[STAT_MOVEMENTSTYLE] == MV_WSW || client->ps.stats[STAT_MOVEMENTSTYLE] == MV_RJQ3 || client->ps.stats[STAT_MOVEMENTSTYLE] == MV_RJCPM || client->ps.stats[STAT_MOVEMENTSTYLE] == MV_BOTCPM) {//qw is 320 too
			if (client->sess.movementStyle == MV_QW || client->sess.movementStyle == MV_CPM || client->sess.movementStyle == MV_Q3 || client->sess.movementStyle == MV_WSW || client->sess.movementStyle == MV_RJQ3 || client->sess.movementStyle == MV_RJCPM || client->sess.movementStyle == MV_BOTCPM) {  //loda double check idk...
				client->ps.speed *= 1.28f;//bring it up to 320 on g_speed 250 for vq3/wsw physics mode
				if (client->pers.haste)
					client->ps.speed *= 1.3f;
			}
		}
		else if (client->ps.stats[STAT_MOVEMENTSTYLE] == MV_SPEED && client->sess.movementStyle == MV_SPEED) {
			client->ps.speed *= 1.7f;
			if (client->ps.fd.forcePower > 50)
				client->ps.fd.forcePower = 50;
		}

		//Check for a siege class speed multiplier
		if (level.gametype == GT_SIEGE &&
			client->siegeClass != -1)
		{
			client->ps.speed *= bgSiegeClasses[client->siegeClass].speed;
		}

		if (client->bodyGrabIndex != ENTITYNUM_NONE)
		{ //can't go nearly as fast when dragging a body around
			client->ps.speed *= 0.2f;
		}

		client->ps.basespeed = client->ps.speed;
	}

	if ( !ent->NPC || !(ent->NPC->aiFlags&NPCAI_CUSTOM_GRAVITY) )
	{//use global gravity
		if (ent->NPC && ent->s.NPC_class == CLASS_VEHICLE &&
			ent->m_pVehicle && ent->m_pVehicle->m_pVehicleInfo->gravity)
		{ //use custom veh gravity
			client->ps.gravity = ent->m_pVehicle->m_pVehicleInfo->gravity;
		}
		else
		{
			if (ent->client->inSpaceIndex && ent->client->inSpaceIndex != ENTITYNUM_NONE)
			{ //in space, so no gravity...
				client->ps.gravity = 1.0f;
				if (ent->s.number < MAX_CLIENTS)
				{
					VectorScale(client->ps.velocity, 0.8f, client->ps.velocity);
				}
			}
			else
			{
				if (client->ps.eFlags2 & EF2_SHIP_DEATH)
				{ //float there
					VectorClear(client->ps.velocity);
					client->ps.gravity = 1.0f;
				}
				else
				{
					client->ps.gravity = g_gravity.value;
					if (client->sess.raceMode || client->ps.stats[STAT_RACEMODE])
						client->ps.gravity = 750; //Match 125fps gravity here since we are using decimal precision for Zvel now
				}
			}
		}
	}

	if (ent->client->ps.duelInProgress)
	{
		gentity_t *duelAgainst = &g_entities[ent->client->ps.duelIndex];

		//Keep the time updated, so once this duel ends this player can't engage in a duel for another
		//10 seconds. This will give other people a chance to engage in duels in case this player wants
		//to engage again right after he's done fighting and someone else is waiting.
		ent->client->ps.fd.privateDuelTime = level.time + 10000;   //oh?

		if (ent->client->ps.duelTime < level.time)
		{
			//Bring out the sabers
			if (ent->client->ps.weapon == WP_SABER 
				&& ent->client->ps.saberHolstered 
				&& ent->client->ps.duelTime )
			{
				ent->client->ps.saberHolstered = 0;

				if (ent->client->saber[0].soundOn)
				{
					G_Sound(ent, CHAN_AUTO, ent->client->saber[0].soundOn);
				}
				if (ent->client->saber[1].soundOn)
				{
					G_Sound(ent, CHAN_AUTO, ent->client->saber[1].soundOn);
				}

				//G_AddEvent(ent, EV_PRIVATE_DUEL, 2); //what the fuck why 2?

				ent->client->ps.duelTime = 0;
			}

			if (duelAgainst 
				&& duelAgainst->client 
				&& duelAgainst->inuse 
				&& duelAgainst->client->ps.weapon == WP_SABER 
				&& duelAgainst->client->ps.saberHolstered 
				&& duelAgainst->client->ps.duelTime)
			{
				duelAgainst->client->ps.saberHolstered = 0;

				if (duelAgainst->client->saber[0].soundOn)
				{
					G_Sound(duelAgainst, CHAN_AUTO, duelAgainst->client->saber[0].soundOn);
				}
				if (duelAgainst->client->saber[1].soundOn)
				{
					G_Sound(duelAgainst, CHAN_AUTO, duelAgainst->client->saber[1].soundOn);
				}

				//G_AddEvent(duelAgainst, EV_PRIVATE_DUEL, 2); //what?

				duelAgainst->client->ps.duelTime = 0;
			}
		}
		else //loda fixme, how to predict this
		{
			client->ps.speed = 0;
			client->ps.basespeed = 0;
			ucmd->forwardmove = 0;
			ucmd->rightmove = 0;
			ucmd->upmove = 0;
		}

		if (!duelAgainst || !duelAgainst->client || !duelAgainst->inuse ||
			duelAgainst->client->ps.duelIndex != ent->s.number)
		{
			ent->client->ps.duelInProgress = qfalse;
			G_AddEvent(ent, EV_PRIVATE_DUEL, 0);
		}
		else if (duelAgainst->health < 1 || duelAgainst->client->ps.stats[STAT_HEALTH] < 1)
		{
			ent->client->ps.duelInProgress = qfalse;
			duelAgainst->client->ps.duelInProgress = qfalse;

			G_AddEvent(ent, EV_PRIVATE_DUEL, 0);
			G_AddEvent(duelAgainst, EV_PRIVATE_DUEL, 0);

			/*
			trap->SendServerCommand( ent-g_entities, va("print \"%s %s\n\"", ent->client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLDUELWINNER")) );
			trap->SendServerCommand( duelAgainst-g_entities, va("print \"%s %s\n\"", ent->client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLDUELWINNER")) );
			*/
//[JAPRO - Serverside - Duel - Improve/fix duel end print - Start]
			//Show ranked, elo change? kms
			if (ent->health > 0 && ent->client->ps.stats[STAT_HEALTH] > 0)
			{
				if (dueltypes[ent->client->ps.clientNum] == 0) {//Saber
					trap->SendServerCommand(-1, va("print \"%s^7 %s %s^7! (^1%i^7/^2%i^7) (Saber)\n\"", 
						ent->client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLDUELWINNER"), duelAgainst->client->pers.netname, ent->client->ps.stats[STAT_HEALTH], ent->client->ps.stats[STAT_ARMOR]));
				}
				else if (dueltypes[ent->client->ps.clientNum] == 1) {//Force
					trap->SendServerCommand(-1, va("print \"%s^7 %s %s^7! (^1%i^7/^2%i/^4%i^7/^5%i^7/^3%i^7) (Force)\n\"", 
						ent->client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLDUELWINNER"), duelAgainst->client->pers.netname, ent->client->ps.stats[STAT_HEALTH], ent->client->ps.stats[STAT_ARMOR], ent->client->ps.fd.forcePower, ent->client->pers.stats.duelDamageGiven, duelAgainst->client->pers.stats.duelDamageGiven));
				}
				else {
					trap->SendServerCommand(-1, va("print \"%s^7 %s %s^7! (^1%i^7/^2%i^7) (Gun)\n\"", 
						ent->client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLDUELWINNER"), duelAgainst->client->pers.netname, ent->client->ps.stats[STAT_HEALTH], ent->client->ps.stats[STAT_ARMOR]));			
					if (dueltypes[ent->client->ps.clientNum] > 2) {
						int weapon = dueltypes[ent->client->ps.clientNum] - 2;
						if (weapon == LAST_USEABLE_WEAPON + 2) { //All weapons
							GiveClientWeapons(ent->client);
							GiveClientWeapons(duelAgainst->client);
						}
						else if (weapon >= WP_BLASTER && weapon <= WP_BRYAR_OLD) { //loda fixme..
							if (weapon == WP_ROCKET_LAUNCHER)
								ent->client->ps.ammo[weaponData[weapon].ammoIndex] = 3;
							else
								ent->client->ps.ammo[weaponData[weapon].ammoIndex] = (int)(ammoData[weaponData[weapon].ammoIndex].max * 0.5); //gun duel ammo.. fix this
						}
					}			
				}
				if (ent->client->pers.lastUserName && ent->client->pers.lastUserName[0] && duelAgainst->client->pers.lastUserName && duelAgainst->client->pers.lastUserName[0]) //loda
					G_AddDuel(ent->client->pers.lastUserName, duelAgainst->client->pers.lastUserName, ent->client->pers.duelStartTime, dueltypes[ent->client->ps.clientNum], ent->client->ps.stats[STAT_HEALTH], ent->client->ps.stats[STAT_ARMOR]);
				if (ent->health < ent->client->ps.stats[STAT_MAX_HEALTH])
					ent->client->ps.stats[STAT_HEALTH] = ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
				ent->client->ps.stats[STAT_ARMOR] = 25;//JAPRO
				if (g_spawnInvulnerability.integer) {
					ent->client->ps.eFlags |= EF_INVULNERABLE;
					ent->client->invulnerableTimer = level.time + g_spawnInvulnerability.integer;
				}
				G_LogPrintf("Duel end: %s^7 defeated %s^7 in type %i\n", ent->client->pers.netname,  duelAgainst->client->pers.netname, dueltypes[ent->client->ps.clientNum]);
			}
			else
			{ //it was a draw, because we both managed to die in the same frame
				if (dueltypes[ent->client->ps.clientNum] == 0) {//Saber
					trap->SendServerCommand(-1, va("print \"%s^7 %s %s^7! (Saber)\n\"", 
						ent->client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLDUELTIE"), duelAgainst->client->pers.netname));
				}
				else if (dueltypes[ent->client->ps.clientNum] == 1) {//Force
					trap->SendServerCommand(-1, va("print \"%s^7 %s %s^7! (^4%i^7/^5%i^7/^3%i^7) (Force)\n\"", 
						ent->client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLDUELTIE"), duelAgainst->client->pers.netname, ent->client->ps.fd.forcePower, ent->client->pers.stats.duelDamageGiven, duelAgainst->client->pers.stats.duelDamageGiven));
				}
				else {
					trap->SendServerCommand(-1, va("print \"%s^7 %s %s^7! (Gun)\n\"",
						ent->client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLDUELTIE"), duelAgainst->client->pers.netname));
				}
				G_LogPrintf("Duel end: %s^7 tied %s^7 in type %i\n", ent->client->pers.netname,  duelAgainst->client->pers.netname, dueltypes[ent->client->ps.clientNum]);
			}
			ent->client->pers.stats.duelDamageGiven = 0;
			duelAgainst->client->pers.stats.duelDamageGiven = 0;
//[JAPRO - Serverside - Duel - Improve/fix duel end print - End]
		}
		else
		{
			vec3_t vSub;
			float subLen = 0;

			VectorSubtract(ent->client->ps.origin, duelAgainst->client->ps.origin, vSub);
			subLen = VectorLength(vSub);

			if (subLen >= 1024 && g_duelDistanceLimit.integer)//[JAPRO - Serverside - Duel - Remove duel distance limit]
			{
				ent->client->ps.duelInProgress = 0;
				duelAgainst->client->ps.duelInProgress = 0;

				G_AddEvent(ent, EV_PRIVATE_DUEL, 0);
				G_AddEvent(duelAgainst, EV_PRIVATE_DUEL, 0);

				trap->SendServerCommand( -1, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PLDUELSTOP")) );
			}
		}
	}

	if (ent->client->doingThrow > level.time)
	{
		gentity_t *throwee = &g_entities[ent->client->throwingIndex];

		if (!throwee->inuse || !throwee->client || throwee->health < 1 ||
			throwee->client->sess.sessionTeam == TEAM_SPECTATOR ||
			(throwee->client->ps.pm_flags & PMF_FOLLOW) ||
			throwee->client->throwingIndex != ent->s.number)
		{
			ent->client->doingThrow = 0;
			ent->client->ps.forceHandExtend = HANDEXTEND_NONE;

			if (throwee->inuse && throwee->client)
			{
				throwee->client->ps.heldByClient = 0;
				throwee->client->beingThrown = 0;

				if (throwee->client->ps.forceHandExtend != HANDEXTEND_POSTTHROWN)
				{
					throwee->client->ps.forceHandExtend = HANDEXTEND_NONE;
				}
			}
		}
	}

	if (ent->client->beingThrown > level.time)
	{
		gentity_t *thrower = &g_entities[ent->client->throwingIndex];

		if (!thrower->inuse || !thrower->client || thrower->health < 1 ||
			thrower->client->sess.sessionTeam == TEAM_SPECTATOR ||
			(thrower->client->ps.pm_flags & PMF_FOLLOW) ||
			thrower->client->throwingIndex != ent->s.number)
		{
			ent->client->ps.heldByClient = 0;
			ent->client->beingThrown = 0;

			if (ent->client->ps.forceHandExtend != HANDEXTEND_POSTTHROWN)
			{
				ent->client->ps.forceHandExtend = HANDEXTEND_NONE;
			}

			if (thrower->inuse && thrower->client)
			{
				thrower->client->doingThrow = 0;
				thrower->client->ps.forceHandExtend = HANDEXTEND_NONE;
			}
		}
		else if (thrower->inuse && thrower->client && thrower->ghoul2 &&
			trap->G2API_HaveWeGhoul2Models(thrower->ghoul2))
		{
#if 0
			int lHandBolt = trap->G2API_AddBolt(thrower->ghoul2, 0, "*l_hand");
			int pelBolt = trap->G2API_AddBolt(thrower->ghoul2, 0, "pelvis");


			if (lHandBolt != -1 && pelBolt != -1)
#endif
			{
				float pDif = 40.0f;
				vec3_t boltOrg, pBoltOrg;
				vec3_t tAngles;
				vec3_t vDif;
				vec3_t entDir, otherAngles;
				vec3_t fwd, right;

				//Always look at the thrower.
				VectorSubtract( thrower->client->ps.origin, ent->client->ps.origin, entDir );
				VectorCopy( ent->client->ps.viewangles, otherAngles );
				otherAngles[YAW] = vectoyaw( entDir );
				SetClientViewAngle( ent, otherAngles );

				VectorCopy(thrower->client->ps.viewangles, tAngles);
				tAngles[PITCH] = tAngles[ROLL] = 0;

				//Get the direction between the pelvis and position of the hand
#if 0
				mdxaBone_t boltMatrix, pBoltMatrix;

				trap->G2API_GetBoltMatrix(thrower->ghoul2, 0, lHandBolt, &boltMatrix, tAngles, thrower->client->ps.origin, level.time, 0, thrower->modelScale);
				boltOrg[0] = boltMatrix.matrix[0][3];
				boltOrg[1] = boltMatrix.matrix[1][3];
				boltOrg[2] = boltMatrix.matrix[2][3];

				trap->G2API_GetBoltMatrix(thrower->ghoul2, 0, pelBolt, &pBoltMatrix, tAngles, thrower->client->ps.origin, level.time, 0, thrower->modelScale);
				pBoltOrg[0] = pBoltMatrix.matrix[0][3];
				pBoltOrg[1] = pBoltMatrix.matrix[1][3];
				pBoltOrg[2] = pBoltMatrix.matrix[2][3];
#else //above tends to not work once in a while, for various reasons I suppose.
				VectorCopy(thrower->client->ps.origin, pBoltOrg);
				AngleVectors(tAngles, fwd, right, 0);
				boltOrg[0] = pBoltOrg[0] + fwd[0]*8 + right[0]*pDif;
				boltOrg[1] = pBoltOrg[1] + fwd[1]*8 + right[1]*pDif;
				boltOrg[2] = pBoltOrg[2];
#endif
				//G_TestLine(boltOrg, pBoltOrg, 0x0000ff, 50);

				VectorSubtract(ent->client->ps.origin, boltOrg, vDif);
				if (VectorLength(vDif) > 32.0f && (thrower->client->doingThrow - level.time) < 4500)
				{ //the hand is too far away, and can no longer hold onto us, so escape.
					ent->client->ps.heldByClient = 0;
					ent->client->beingThrown = 0;
					thrower->client->doingThrow = 0;

					thrower->client->ps.forceHandExtend = HANDEXTEND_NONE;
					G_EntitySound( thrower, CHAN_VOICE, G_SoundIndex("*pain25.wav") );

					ent->client->ps.forceDodgeAnim = 2;
					ent->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
					ent->client->ps.forceHandExtendTime = level.time + 500;
					ent->client->ps.velocity[2] = 400;
					G_PreDefSound(ent->client->ps.origin, PDSOUND_FORCEJUMP);
				}
				else if ((client->beingThrown - level.time) < 4000)
				{ //step into the next part of the throw, and go flying back
					float vScale = 400.0f;
					ent->client->ps.forceHandExtend = HANDEXTEND_POSTTHROWN;
					ent->client->ps.forceHandExtendTime = level.time + 1200;
					ent->client->ps.forceDodgeAnim = 0;

					thrower->client->ps.forceHandExtend = HANDEXTEND_POSTTHROW;
					thrower->client->ps.forceHandExtendTime = level.time + 200;

					ent->client->ps.heldByClient = 0;

					ent->client->ps.heldByClient = 0;
					ent->client->beingThrown = 0;
					thrower->client->doingThrow = 0;

					AngleVectors(thrower->client->ps.viewangles, vDif, 0, 0);
					ent->client->ps.velocity[0] = vDif[0]*vScale;
					ent->client->ps.velocity[1] = vDif[1]*vScale;
					ent->client->ps.velocity[2] = 400;

					G_EntitySound( ent, CHAN_VOICE, G_SoundIndex("*pain100.wav") );
					G_EntitySound( thrower, CHAN_VOICE, G_SoundIndex("*jump1.wav") );

					//Set the thrower as the "other killer", so if we die from fall/impact damage he is credited.
					ent->client->ps.otherKiller = thrower->s.number;
//JAPRO - Serverside - Change otherkiller time if fixkillcredit is on - Start
					if (!g_fixKillCredit.integer)
						ent->client->ps.otherKillerTime = level.time + 8000;
					else
						ent->client->ps.otherKillerTime = level.time + 2000;
//JAPRO - Serverside - Change otherkiller time if fixkillcredit is on - End
					ent->client->ps.otherKillerDebounceTime = level.time + 100;
				}
				else
				{ //see if we can move to be next to the hand.. if it's not clear, break the throw.
					vec3_t intendedOrigin;
					trace_t tr;
					trace_t tr2;

					VectorSubtract(boltOrg, pBoltOrg, vDif);
					VectorNormalize(vDif);

					VectorClear(ent->client->ps.velocity);
					intendedOrigin[0] = pBoltOrg[0] + vDif[0]*pDif;
					intendedOrigin[1] = pBoltOrg[1] + vDif[1]*pDif;
					intendedOrigin[2] = thrower->client->ps.origin[2];

					JP_Trace(&tr, intendedOrigin, ent->r.mins, ent->r.maxs, intendedOrigin, ent->s.number, ent->clipmask, qfalse, 0, 0);
					JP_Trace(&tr2, ent->client->ps.origin, ent->r.mins, ent->r.maxs, intendedOrigin, ent->s.number, CONTENTS_SOLID, qfalse, 0, 0);

					if (tr.fraction == 1.0 && !tr.startsolid && tr2.fraction == 1.0 && !tr2.startsolid)
					{
						VectorCopy(intendedOrigin, ent->client->ps.origin);
						
						if ((client->beingThrown - level.time) < 4800)
						{
							ent->client->ps.heldByClient = thrower->s.number+1;
						}
					}
					else
					{ //if the guy can't be put here then it's time to break the throw off.
						ent->client->ps.heldByClient = 0;
						ent->client->beingThrown = 0;
						thrower->client->doingThrow = 0;

						thrower->client->ps.forceHandExtend = HANDEXTEND_NONE;
						G_EntitySound( thrower, CHAN_VOICE, G_SoundIndex("*pain25.wav") );

						ent->client->ps.forceDodgeAnim = 2;
						ent->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
						ent->client->ps.forceHandExtendTime = level.time + 500;
						ent->client->ps.velocity[2] = 400;
						G_PreDefSound(ent->client->ps.origin, PDSOUND_FORCEJUMP);
					}
				}
			}
		}
	}
	else if (ent->client->ps.heldByClient)
	{
		ent->client->ps.heldByClient = 0;
	}


#if _GRAPPLE
	//CHUNK 1

		// sanity check, deals with falling etc;
	if ( ent->client->hook && ent->client->hook->think == Weapon_HookThink && CanGrapple(ent)) {
			ent->client->ps.pm_flags |= PMF_GRAPPLE;
	} else {
		//Com_Printf("Unsetting grapple pmf\n");
		ent->client->ps.pm_flags &= ~( PMF_GRAPPLE );
	}
#endif

	/*
	if ( client->ps.powerups[PW_HASTE] ) {
		client->ps.speed *= 1.3;
	}
	*/

	//Will probably never need this again, since we have g2 properly serverside now.
	//But just in case.
	/*
	if (client->ps.usingATST && ent->health > 0)
	{ //we have special shot clip boxes as an ATST
		ent->r.contents |= CONTENTS_NOSHOT;
		ATST_ManageDamageBoxes(ent);
	}
	else
	{
		ent->r.contents &= ~CONTENTS_NOSHOT;
		client->damageBoxHandle_Head = 0;
		client->damageBoxHandle_RLeg = 0;
		client->damageBoxHandle_LLeg = 0;
	}
	*/

	//rww - moved this stuff into the pmove code so that it's predicted properly
	//BG_AdjustClientSpeed(&client->ps, &client->pers.cmd, level.time);

	// set up for pmove
	oldEventSequence = client->ps.eventSequence;

	memset (&pmove, 0, sizeof(pmove));

	if ( ent->flags & FL_FORCE_GESTURE ) {
		ent->flags &= ~FL_FORCE_GESTURE;
		ent->client->pers.cmd.buttons |= BUTTON_GESTURE;
	}

	if (ent->client && ent->client->ps.fallingToDeath &&
		(level.time - FALL_FADE_TIME) > ent->client->ps.fallingToDeath)
	{ //die!
		if (ent->health > 0)
		{
			gentity_t *otherKiller = ent;
			if (ent->client->ps.otherKillerTime > level.time &&
				ent->client->ps.otherKiller != ENTITYNUM_NONE)
			{
				otherKiller = &g_entities[ent->client->ps.otherKiller];

				if (!otherKiller->inuse)
				{
					otherKiller = ent;
				}
			}
			G_Damage(ent, otherKiller, otherKiller, NULL, ent->client->ps.origin, 9999, DAMAGE_NO_PROTECTION, MOD_FALLING);
			//player_die(ent, ent, ent, 100000, MOD_FALLING);
	//		if (!ent->NPC)
	//		{
	//			ClientRespawn(ent);
	//		}
	//		ent->client->ps.fallingToDeath = 0;

			G_MuteSound(ent->s.number, CHAN_VOICE); //stop screaming, because you are dead!
		}
	}

//JAPRO - Fixkillcredit stuff , im not sure actually - Start
	if (!g_fixKillCredit.integer)
	{
		if (ent->client->ps.otherKillerTime > level.time &&
			ent->client->ps.groundEntityNum != ENTITYNUM_NONE &&
			ent->client->ps.otherKillerDebounceTime < level.time)
		{
			ent->client->ps.otherKillerTime = 0;
			ent->client->ps.otherKiller = ENTITYNUM_NONE;
		}
		else if (ent->client->ps.otherKillerTime > level.time &&
			ent->client->ps.groundEntityNum == ENTITYNUM_NONE)
		{
			if (ent->client->ps.otherKillerDebounceTime < (level.time + 100))
			{
				ent->client->ps.otherKillerDebounceTime = level.time + 100;
			}
		}
	}
	else if (ent->client->ps.otherKiller != ENTITYNUM_NONE)
	{
		if (ent->client->ps.otherKillerTime < level.time)
				ent->client->ps.otherKiller = ENTITYNUM_NONE;
		else if (ent->client->ps.groundEntityNum == ENTITYNUM_NONE)
		{
			if (ent->client->ps.otherKillerTime < level.time + 100)
				ent->client->ps.otherKillerTime = level.time + 100;
			if (ent->client->ps.otherKillerDebounceTime < level.time + 100)
				ent->client->ps.otherKillerDebounceTime = level.time + 100;
		}
	}
//JAPRO - Fixkillcredit stuff - End

//	WP_ForcePowersUpdate( ent, msec, ucmd); //update any active force powers
//	WP_SaberPositionUpdate(ent, ucmd); //check the server-side saber point, do apprioriate server-side actions (effects are cs-only)

	//NOTE: can't put USE here *before* PMove!!
	if ( ent->client->ps.useDelay > level.time 
		&& ent->client->ps.m_iVehicleNum )
	{//when in a vehicle, debounce the use...
		ucmd->buttons &= ~BUTTON_USE;
	}

	//FIXME: need to do this before check to avoid walls and cliffs (or just cliffs?)
	G_AddPushVecToUcmd( ent, ucmd );

	//play/stop any looping sounds tied to controlled movement
	G_CheckMovingLoopingSounds( ent, ucmd );

	pmove.ps = &client->ps;
	pmove.cmd = *ucmd;
	if ( pmove.ps->pm_type == PM_DEAD ) {
		pmove.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;
	}
	else if ( ent->r.svFlags & SVF_BOT ) {
		pmove.tracemask = MASK_PLAYERSOLID | CONTENTS_MONSTERCLIP;
	}
	else {
		pmove.tracemask = MASK_PLAYERSOLID;
	}
	pmove.trace = SV_PMTrace;
	pmove.pointcontents = trap->PointContents;
	pmove.debugLevel = g_debugMove.integer;
	pmove.noFootsteps = (dmflags.integer & DF_NO_FOOTSTEPS) > 0;

	pmove.pmove_fixed = pmove_fixed.integer | client->pers.pmoveFixed;
	pmove.pmove_msec = pmove_msec.integer;
	pmove.pmove_float = pmove_float.integer;

	pmove.animations = bgAllAnims[ent->localAnimIndex].anims;//NULL;

	//rww - bgghoul2
	pmove.ghoul2 = NULL;

#ifdef _DEBUG
	if (g_disableServerG2.integer)
	{

	}
	else
#endif
	if (ent->ghoul2)
	{
		if (ent->localAnimIndex > 1)
		{ //if it isn't humanoid then we will be having none of this.
			pmove.ghoul2 = NULL;
		}
		else
		{
			pmove.ghoul2 = ent->ghoul2;
			pmove.g2Bolts_LFoot = trap->G2API_AddBolt(ent->ghoul2, 0, "*l_leg_foot");
			pmove.g2Bolts_RFoot = trap->G2API_AddBolt(ent->ghoul2, 0, "*r_leg_foot");
		}
	}

	//point the saber data to the right place
#if 0
	k = 0;
	while (k < MAX_SABERS)
	{
		if (ent->client->saber[k].model[0])
		{
			pm.saber[k] = &ent->client->saber[k];
		}
		else
		{
			pm.saber[k] = NULL;
		}
		k++;
	}
#endif

	//I'll just do this every frame in case the scale changes in realtime (don't need to update the g2 inst for that)
	VectorCopy(ent->modelScale, pmove.modelScale);
	//rww end bgghoul2

	pmove.gametype = level.gametype;
	pmove.debugMelee = g_debugMelee.integer;
	pmove.stepSlideFix = g_stepSlideFix.integer;

	pmove.noSpecMove = g_noSpecMove.integer;

	pmove.nonHumanoid = (ent->localAnimIndex > 0);

	/*
	if (level.intermissionQueued != 0 && g_singlePlayer.integer) {
		if ( level.time - level.intermissionQueued >= 1000  ) {
			pm.cmd.buttons = 0;
			pm.cmd.forwardmove = 0;
			pm.cmd.rightmove = 0;
			pm.cmd.upmove = 0;
			if ( level.time - level.intermissionQueued >= 2000 && level.time - level.intermissionQueued <= 2500 ) {
				trap->SendConsoleCommand( EXEC_APPEND, "centerview\n");
			}
			ent->client->ps.pm_type = PM_SPINTERMISSION;
		}
	}
	*/

	//Set up bg entity data
	pmove.baseEnt = (bgEntity_t *)g_entities;
	pmove.entSize = sizeof(gentity_t);

	if (ent->client->ps.saberLockTime > level.time)
	{
		gentity_t *blockOpp = &g_entities[ent->client->ps.saberLockEnemy];

		if (blockOpp && blockOpp->inuse && blockOpp->client)
		{
			vec3_t lockDir, lockAng;

			//VectorClear( ent->client->ps.velocity );
			VectorSubtract( blockOpp->r.currentOrigin, ent->r.currentOrigin, lockDir );
			//lockAng[YAW] = vectoyaw( defDir );
			vectoangles(lockDir, lockAng);
			SetClientViewAngle( ent, lockAng );
		}

		if ( ent->client->ps.saberLockHitCheckTime < level.time )
		{//have moved to next frame since last lock push
			ent->client->ps.saberLockHitCheckTime = level.time;//so we don't push more than once per server frame
			if ( ( ent->client->buttons & BUTTON_ATTACK ) && ! ( ent->client->oldbuttons & BUTTON_ATTACK ) )
			{
				if ( ent->client->ps.saberLockHitIncrementTime < level.time )
				{//have moved to next frame since last saberlock attack button press
					int lockHits = 0;
					ent->client->ps.saberLockHitIncrementTime = level.time;//so we don't register an attack key press more than once per server frame
					//NOTE: FP_SABER_OFFENSE level already taken into account in PM_SaberLocked
					if ( (ent->client->ps.fd.forcePowersActive&(1<<FP_RAGE)) )
					{//raging: push harder
						lockHits = 1+ent->client->ps.fd.forcePowerLevel[FP_RAGE];
					}
					else
					{//normal attack
						switch ( ent->client->ps.fd.saberAnimLevel )
						{
						case SS_FAST:
							lockHits = 1;
							break;
						case SS_MEDIUM:
						case SS_TAVION:
						case SS_DUAL:
						case SS_STAFF:
							lockHits = 2;
							break;
						case SS_STRONG:
						case SS_DESANN:
							lockHits = 3;
							break;
						}
					}
					if ( ent->client->ps.fd.forceRageRecoveryTime > level.time 
						&& Q_irand( 0, 1 ) )
					{//finished raging: weak
						lockHits -= 1;
					}
					lockHits += ent->client->saber[0].lockBonus;
					if ( ent->client->saber[1].model
						&& ent->client->saber[1].model[0]
						&& !ent->client->ps.saberHolstered )
					{
						lockHits += ent->client->saber[1].lockBonus;
					}
					ent->client->ps.saberLockHits += lockHits;
					if ( g_saberLockRandomNess.integer )
					{
						ent->client->ps.saberLockHits += Q_irand( 0, g_saberLockRandomNess.integer );
						if ( ent->client->ps.saberLockHits < 0 )
						{
							ent->client->ps.saberLockHits = 0;
						}
					}
				}
			}
			if ( ent->client->ps.saberLockHits > 0 )
			{
				if ( !ent->client->ps.saberLockAdvance )
				{
					ent->client->ps.saberLockHits--;
				}
				ent->client->ps.saberLockAdvance = qtrue;
			}
		}
	}
	else
	{
		ent->client->ps.saberLockFrame = 0;
		//check for taunt
		if ( (pmove.cmd.generic_cmd == GENCMD_ENGAGE_DUEL) && (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL) )
		{//already in a duel, make it a taunt command
			pmove.cmd.buttons |= BUTTON_GESTURE;
		}
	}


#if _GRAPPLE
	//CHUNK 2
	//Com_Printf("Chunk 2 start!\n");
	if (pm && ent->client)
	{

		if ( (pmove.cmd.buttons & BUTTON_GRAPPLE) &&
				ent->client->ps.pm_type != PM_DEAD &&
				!ent->client->hookHasBeenFired &&
				(ent->client->hookFireTime < level.time - g_hookFloodProtect.integer) &&
				CanFireGrapple(ent))
		{
			Weapon_GrapplingHook_Fire( ent );
			ent->client->hookHasBeenFired = qtrue;
			ent->client->hookFireTime = level.time;
		}

		if ( !(pmove.cmd.buttons & BUTTON_GRAPPLE)  &&
				ent->client->ps.pm_type != PM_DEAD &&
				ent->client->hookHasBeenFired &&
				ent->client->fireHeld )
		{
			ent->client->fireHeld = qfalse;
			ent->client->hookHasBeenFired = qfalse;
			client->ps.pm_flags &= ~( PMF_GRAPPLE );
		}

		if ( client->hook && client->fireHeld == qfalse ) {
			Weapon_HookFree(client->hook);
		}
	}
#endif


	if (ent->s.number >= MAX_CLIENTS)
	{
		VectorCopy(ent->r.mins, pmove.mins);
		VectorCopy(ent->r.maxs, pmove.maxs);
#if 1
		if (ent->s.NPC_class == CLASS_VEHICLE &&
			ent->m_pVehicle )
		{
			if ( ent->m_pVehicle->m_pPilot)
			{ //vehicles want to use their last pilot ucmd I guess
				if ((level.time - ent->m_pVehicle->m_ucmd.serverTime) > 2000)
				{ //Previous owner disconnected, maybe
					ent->m_pVehicle->m_ucmd.serverTime = level.time;
					ent->client->ps.commandTime = level.time-100;
					msec = 100;
				}
				else if ( ent->m_pVehicle->m_ucmd.serverTime > level.time + 200 ) { //stop speedup cheating for vehicles japro
					ent->m_pVehicle->m_ucmd.serverTime = level.time + 200;
					//trap->Print("serverTime <<<<<\n" );
				}
				else if ( ent->m_pVehicle->m_ucmd.serverTime < level.time - 1000 ) {
					ent->m_pVehicle->m_ucmd.serverTime = level.time - 1000;
					//trap->Print("serverTime >>>>>\n" );
				}

				memcpy(&pmove.cmd, &ent->m_pVehicle->m_ucmd, sizeof(usercmd_t));
				
				//no veh can strafe
				pmove.cmd.rightmove = 0;
				//no crouching or jumping!
				pmove.cmd.upmove = 0;

				//NOTE: button presses were getting lost!
				assert(g_entities[ent->m_pVehicle->m_pPilot->s.number].client);
				pmove.cmd.buttons = (g_entities[ent->m_pVehicle->m_pPilot->s.number].client->pers.cmd.buttons&(BUTTON_ATTACK|BUTTON_ALT_ATTACK));
			}
			if ( ent->m_pVehicle->m_pVehicleInfo->type == VH_WALKER )
			{
				if ( ent->client->ps.groundEntityNum != ENTITYNUM_NONE )
				{//ATST crushes anything underneath it
					gentity_t	*under = &g_entities[ent->client->ps.groundEntityNum];
					if ( under && under->health && under->takedamage )
					{
						vec3_t	down = {0,0,-1};
						//FIXME: we'll be doing traces down from each foot, so we'll have a real impact origin
						G_Damage( under, ent, ent, down, under->r.currentOrigin, 100, 0, MOD_CRUSH );
					}
				}
			}
		}
#endif
	}

	Pmove (&pmove);

	if (ent->client->solidHack)
	{
		if (ent->client->solidHack > level.time)
		{ //whee!
			ent->r.contents = 0;
		}
		else
		{
			ent->r.contents = CONTENTS_BODY;
			ent->client->solidHack = 0;
		}
	}
	
	if ( ent->NPC )
	{
		VectorCopy( ent->client->ps.viewangles, ent->r.currentAngles );
	}

	if (pmove.checkDuelLoss)
	{
		if (pmove.checkDuelLoss > 0 && (pmove.checkDuelLoss <= MAX_CLIENTS || (pmove.checkDuelLoss < (MAX_GENTITIES-1) && g_entities[pmove.checkDuelLoss-1].s.eType == ET_NPC) ) )
		{
			gentity_t *clientLost = &g_entities[pmove.checkDuelLoss-1];

			if (clientLost && clientLost->inuse && clientLost->client && Q_irand(0, 40) > clientLost->health)
			{
				vec3_t attDir;
				VectorSubtract(ent->client->ps.origin, clientLost->client->ps.origin, attDir);
				VectorNormalize(attDir);

				VectorClear(clientLost->client->ps.velocity);
				clientLost->client->ps.forceHandExtend = HANDEXTEND_NONE;
				clientLost->client->ps.forceHandExtendTime = 0;

				gGAvoidDismember = 1;
				G_Damage(clientLost, ent, ent, attDir, clientLost->client->ps.origin, 9999, DAMAGE_NO_PROTECTION, MOD_SABER);

				if (clientLost->health < 1)
				{
					gGAvoidDismember = 2;
					G_CheckForDismemberment(clientLost, ent, clientLost->client->ps.origin, 999, (clientLost->client->ps.legsAnim), qfalse);
				}

				gGAvoidDismember = 0;
			}
			else if (clientLost && clientLost->inuse && clientLost->client &&
				clientLost->client->ps.forceHandExtend != HANDEXTEND_KNOCKDOWN && clientLost->client->ps.saberEntityNum)
			{ //if we didn't knock down it was a circle lock. So as punishment, make them lose their saber and go into a proper anim
				saberCheckKnockdown_DuelLoss(&g_entities[clientLost->client->ps.saberEntityNum], clientLost, ent);
			}
		}

		pmove.checkDuelLoss = 0;
	}

	//if (pmove.cmd.generic_cmd && (pmove.cmd.generic_cmd != ent->client->lastGenCmd || ent->client->lastGenCmdTime < level.time))
	if (pmove.cmd.generic_cmd)
	{
		//ent->client->lastGenCmd = pmove.cmd.generic_cmd;
		//if (pmove.cmd.generic_cmd != GENCMD_FORCE_THROW && pmove.cmd.generic_cmd != GENCMD_FORCE_PULL)
		//{ //these are the only two where you wouldn't care about a delay between
		//	ent->client->lastGenCmdTime = level.time + 300; //default 100ms debounce between issuing the same command.
		//}

		switch(pmove.cmd.generic_cmd)
		{
		case 0:
			break;
		case GENCMD_SABERSWITCH:
			if (ent->client->genCmdDebounce[GENCMD_DELAY_SABER] > level.time - 300)
				break;
			ent->client->genCmdDebounce[GENCMD_DELAY_SABER] = level.time;
			Cmd_ToggleSaber_f(ent);
			break;
		case GENCMD_ENGAGE_DUEL:
			if (ent->client->genCmdDebounce[GENCMD_DELAY_DUEL] > level.time - 100)
				break;
			ent->client->genCmdDebounce[GENCMD_DELAY_DUEL] = level.time;
			if ( level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL )
			{//already in a duel, made it a taunt command
			}
			else
			{
				Cmd_EngageDuel_f(ent, 0);
			}
			break;
		case GENCMD_FORCE_HEAL:
			if (ent->client->genCmdDebounce[GENCMD_DELAY_HEAL] > level.time - 300)
				break;
			ent->client->genCmdDebounce[GENCMD_DELAY_HEAL] = level.time;
			ForceHeal(ent);
			break;
		case GENCMD_FORCE_SPEED:
			if (ent->client->genCmdDebounce[GENCMD_DELAY_SPEED] > level.time - 300)
				break;
			ent->client->genCmdDebounce[GENCMD_DELAY_SPEED] = level.time;
			ForceSpeed(ent, 0);
			break;
		case GENCMD_FORCE_THROW:
			ForceThrow(ent, qfalse);
			break;
		case GENCMD_FORCE_PULL:
			ForceThrow(ent, qtrue);
			break;
		case GENCMD_FORCE_DISTRACT:
			if (ent->client->genCmdDebounce[GENCMD_DELAY_TRICK] > level.time - 300)
				break;
			ent->client->genCmdDebounce[GENCMD_DELAY_TRICK] = level.time;
			ForceTelepathy(ent);
			break;
		case GENCMD_FORCE_RAGE:
			if (ent->client->genCmdDebounce[GENCMD_DELAY_RAGE] > level.time - 300)
				break;
			ent->client->genCmdDebounce[GENCMD_DELAY_RAGE] = level.time;
			ForceRage(ent);
			break;
		case GENCMD_FORCE_PROTECT:
			if (ent->client->genCmdDebounce[GENCMD_DELAY_PROTECT] > level.time - 300)
				break;
			ent->client->genCmdDebounce[GENCMD_DELAY_PROTECT] = level.time;
			ForceProtect(ent);
			break;
		case GENCMD_FORCE_ABSORB:
			if (ent->client->genCmdDebounce[GENCMD_DELAY_ABSORB] > level.time - 300)
				break;
			ent->client->genCmdDebounce[GENCMD_DELAY_ABSORB] = level.time;
			ForceAbsorb(ent);
			break;
		case GENCMD_FORCE_HEALOTHER:
			ForceTeamHeal(ent); //Only need to debounce powers that you can toggle i guess, since these are debounced when you 'activate' them
			break;
		case GENCMD_FORCE_FORCEPOWEROTHER:
			ForceTeamForceReplenish(ent);
			break;
		case GENCMD_FORCE_SEEING:
			if (ent->client->genCmdDebounce[GENCMD_DELAY_SEEING] > level.time - 300)
				break;
			ent->client->genCmdDebounce[GENCMD_DELAY_SEEING] = level.time;
			ForceSeeing(ent);
			break;
		case GENCMD_USE_SEEKER:
			if ( (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_SEEKER)) &&
				G_ItemUsable(&ent->client->ps, HI_SEEKER) )
			{
				ItemUse_Seeker(ent);
				G_AddEvent(ent, EV_USE_ITEM0+HI_SEEKER, 0);
				ent->client->ps.stats[STAT_HOLDABLE_ITEMS] &= ~(1 << HI_SEEKER);
			}
			break;
		case GENCMD_USE_FIELD:
			if ( (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_SHIELD)) &&
				G_ItemUsable(&ent->client->ps, HI_SHIELD) )
			{
				ItemUse_Shield(ent);
				G_AddEvent(ent, EV_USE_ITEM0+HI_SHIELD, 0);
				ent->client->ps.stats[STAT_HOLDABLE_ITEMS] &= ~(1 << HI_SHIELD);
			}
			break;
		case GENCMD_USE_BACTA:
			if ( (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_MEDPAC)) &&
				G_ItemUsable(&ent->client->ps, HI_MEDPAC) )
			{
				ItemUse_MedPack(ent);
				G_AddEvent(ent, EV_USE_ITEM0+HI_MEDPAC, 0);
				ent->client->ps.stats[STAT_HOLDABLE_ITEMS] &= ~(1 << HI_MEDPAC);
			}
			break;
		case GENCMD_USE_BACTABIG:
			if ( (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_MEDPAC_BIG)) &&
				G_ItemUsable(&ent->client->ps, HI_MEDPAC_BIG) )
			{
				ItemUse_MedPack_Big(ent);
				G_AddEvent(ent, EV_USE_ITEM0+HI_MEDPAC_BIG, 0);
				ent->client->ps.stats[STAT_HOLDABLE_ITEMS] &= ~(1 << HI_MEDPAC_BIG);
			}
			break;
		case GENCMD_USE_ELECTROBINOCULARS:
			if (ent->client->genCmdDebounce[GENCMD_DELAY_BINOCS] > level.time - 300)
				break;
			ent->client->genCmdDebounce[GENCMD_DELAY_BINOCS] = level.time;
			if ( (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_BINOCULARS)) &&
				G_ItemUsable(&ent->client->ps, HI_BINOCULARS) )
			{
				ItemUse_Binoculars(ent);
				if (ent->client->ps.zoomMode == 0)
				{
					G_AddEvent(ent, EV_USE_ITEM0+HI_BINOCULARS, 1);
				}
				else
				{
					G_AddEvent(ent, EV_USE_ITEM0+HI_BINOCULARS, 2);
				}
			}
			break;
		case GENCMD_ZOOM:
			if (ent->client->genCmdDebounce[GENCMD_DELAY_ZOOM] > level.time - 300)
				break;
			ent->client->genCmdDebounce[GENCMD_DELAY_ZOOM] = level.time;
			if ( (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_BINOCULARS)) &&
				G_ItemUsable(&ent->client->ps, HI_BINOCULARS) )
			{
				ItemUse_Binoculars(ent);
				if (ent->client->ps.zoomMode == 0)
				{
					G_AddEvent(ent, EV_USE_ITEM0+HI_BINOCULARS, 1);
				}
				else
				{
					G_AddEvent(ent, EV_USE_ITEM0+HI_BINOCULARS, 2);
				}
			}
			break;
		case GENCMD_USE_SENTRY:
			if ( (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_SENTRY_GUN)) &&
				G_ItemUsable(&ent->client->ps, HI_SENTRY_GUN) )
			{
				ItemUse_Sentry(ent);
				G_AddEvent(ent, EV_USE_ITEM0+HI_SENTRY_GUN, 0);
				ent->client->ps.stats[STAT_HOLDABLE_ITEMS] &= ~(1 << HI_SENTRY_GUN);
			}
			break;
		case GENCMD_USE_JETPACK:
			if (ent->client->genCmdDebounce[GENCMD_DELAY_JETPACK] > level.time - 300)
				break;
			ent->client->genCmdDebounce[GENCMD_DELAY_JETPACK] = level.time;
			if ( (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK)) &&
				G_ItemUsable(&ent->client->ps, HI_JETPACK) )
			{
				if (!g_tweakJetpack.integer && !ent->client->sess.raceMode) {//JAPRO - Remove old jetpack
					ItemUse_Jetpack(ent);
					G_AddEvent(ent, EV_USE_ITEM0+HI_JETPACK, 0);
				}
				/*
				if (ent->client->ps.zoomMode == 0)
				{
					G_AddEvent(ent, EV_USE_ITEM0+HI_BINOCULARS, 1);
				}
				else
				{
					G_AddEvent(ent, EV_USE_ITEM0+HI_BINOCULARS, 2);
				}
				*/
			}
			break;
		case GENCMD_USE_HEALTHDISP:
			if ( (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_HEALTHDISP)) &&
				G_ItemUsable(&ent->client->ps, HI_HEALTHDISP) )
			{
				//ItemUse_UseDisp(ent, HI_HEALTHDISP);
				G_AddEvent(ent, EV_USE_ITEM0+HI_HEALTHDISP, 0);
			}
			break;
		case GENCMD_USE_AMMODISP:
			if ( (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_AMMODISP)) &&
				G_ItemUsable(&ent->client->ps, HI_AMMODISP) )
			{
				//ItemUse_UseDisp(ent, HI_AMMODISP);
				G_AddEvent(ent, EV_USE_ITEM0+HI_AMMODISP, 0);
			}
			break;
		case GENCMD_USE_EWEB:
			if (ent->client->genCmdDebounce[GENCMD_DELAY_EWEB] > level.time - 300)
				break;
			ent->client->genCmdDebounce[GENCMD_DELAY_EWEB] = level.time;
			if ( (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_EWEB)) &&
				G_ItemUsable(&ent->client->ps, HI_EWEB) )
			{
				ItemUse_UseEWeb(ent);
				G_AddEvent(ent, EV_USE_ITEM0+HI_EWEB, 0);
			}
			break;
		case GENCMD_USE_CLOAK:
			if (ent->client->genCmdDebounce[GENCMD_DELAY_CLOAK] > level.time - 300)
				break;
			ent->client->genCmdDebounce[GENCMD_DELAY_CLOAK] = level.time;
			if ( (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_CLOAK)) &&
				G_ItemUsable(&ent->client->ps, HI_CLOAK) )
			{
				if ( ent->client->ps.powerups[PW_CLOAKED] )
				{//decloak
					Jedi_Decloak( ent );
				}
				else
				{//cloak
					Jedi_Cloak( ent );
				}
			}
			break;
		case GENCMD_SABERATTACKCYCLE:
			{
				int delay = 300;
				if (g_tweakSaber.integer & ST_FASTCYCLE) {
					if (!(ent->client->saber[0].singleBladeStyle || (ent->client->saber[1].model && ent->client->saber[1].model[0])))//Single
						delay = 100;
				}
				if (ent->client->genCmdDebounce[GENCMD_DELAY_SABERSWITCH] > level.time - delay) //Not sure what this should be.. on baseJK you can bypass any delay, though it seems clearly intended to be 300ms delay..
					break; //Cant really make this delay super low, since then people who use keyboard binds for saberswitch have trouble only switching once i guess :s
			}
			ent->client->genCmdDebounce[GENCMD_DELAY_SABERSWITCH] = level.time;
			Cmd_SaberAttackCycle_f(ent);
			break;
		case GENCMD_TAUNT:
			if (ent->client->genCmdDebounce[GENCMD_DELAY_TAUNT] > level.time - 1000)
				break;
			ent->client->genCmdDebounce[GENCMD_DELAY_TAUNT] = level.time;
			G_SetTauntAnim( ent, TAUNT_TAUNT );
			break;
		case GENCMD_BOW:
			if (ent->client->genCmdDebounce[GENCMD_DELAY_EMOTE] > level.time - 1000)
				break;
			ent->client->genCmdDebounce[GENCMD_DELAY_EMOTE] = level.time;
			G_SetTauntAnim( ent, TAUNT_BOW );
			break;
		case GENCMD_MEDITATE:
			if (ent->client->genCmdDebounce[GENCMD_DELAY_EMOTE] > level.time - 1000)
				break;
			ent->client->genCmdDebounce[GENCMD_DELAY_EMOTE] = level.time;
			G_SetTauntAnim( ent, TAUNT_MEDITATE );
			break;
		case GENCMD_FLOURISH:
			if (ent->client->genCmdDebounce[GENCMD_DELAY_TAUNT] > level.time - 1000)
				break;
			ent->client->genCmdDebounce[GENCMD_DELAY_TAUNT] = level.time;
			G_SetTauntAnim( ent, TAUNT_FLOURISH );
			break;
		case GENCMD_GLOAT:
			if (ent->client->genCmdDebounce[GENCMD_DELAY_TAUNT] > level.time - 1000)
				break;
			ent->client->genCmdDebounce[GENCMD_DELAY_TAUNT] = level.time;
			G_SetTauntAnim( ent, TAUNT_GLOAT );
			break;
		default:
			break;
		}
	}

	// save results of pmove
	if ( ent->client->ps.eventSequence != oldEventSequence ) {
		ent->eventTime = level.time;
	}
	/*
	if (g_smoothClients.integer) {
		BG_PlayerStateToEntityStateExtraPolate( &ent->client->ps, &ent->s, ent->client->ps.commandTime, qfalse );
		//rww - 12-03-02 - Don't snap the origin of players! It screws prediction all up.
	}
	else {
	*/
		BG_PlayerStateToEntityState( &ent->client->ps, &ent->s, qfalse );
	//}

	if (isNPC)
	{
		ent->s.eType = ET_NPC;
	}

	SendPendingPredictableEvents( &ent->client->ps );

	//if ( !( ent->client->ps.eFlags & EF_FIRING ) ) {
		//client->fireHeld = qfalse;		// for grapple
	//}

	// use the snapped origin for linking so it matches client predicted versions
	VectorCopy( ent->s.pos.trBase, ent->r.currentOrigin );

	if (ent->s.eType != ET_NPC ||
		ent->s.NPC_class != CLASS_VEHICLE ||
		!ent->m_pVehicle ||
		!ent->m_pVehicle->m_iRemovedSurfaces)
	{ //let vehicles that are getting broken apart do their own crazy sizing stuff
		VectorCopy (pmove.mins, ent->r.mins);
		VectorCopy (pmove.maxs, ent->r.maxs);
	}

	ent->waterlevel = pmove.waterlevel;
	ent->watertype = pmove.watertype;

	// execute client events
	//OSP: pause
	if ( level.pause.state == PAUSE_NONE || ent->client->sess.raceMode )
		ClientEvents( ent, oldEventSequence );



	if ( pmove.useEvent )
	{
		//TODO: Use
//		TryUse( ent );
	}
	if ((ent->client->pers.cmd.buttons & BUTTON_USE) && ent->client->ps.useDelay < level.time)
	{
		TryUse(ent);
		ent->client->ps.useDelay = level.time + 100;
	}
	if (g_allowTargetLaser.integer && ent->client->pers.cmd.buttons & BUTTON_TARGET) { //Add cvar to enable/disable
		TryTargettingLaser(ent);
	}

	// link entity now, after any personal teleporters have been used
	trap->LinkEntity ((sharedEntity_t *)ent);
	if ( !ent->client->noclip ) {
		G_TouchTriggers( ent );
	}

	// NOTE: now copy the exact origin over otherwise clients can be snapped into solid
	VectorCopy( ent->client->ps.origin, ent->r.currentOrigin );

	//test for solid areas in the AAS file
//	BotTestAAS(ent->r.currentOrigin);

	// touch other objects
	ClientImpacts( ent, &pmove );

	// save results of triggers and client events
	if (ent->client->ps.eventSequence != oldEventSequence) {
		ent->eventTime = level.time;
	}

	// swap and latch button actions
	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;
	client->latched_buttons |= client->buttons & ~client->oldbuttons;

//	G_VehicleAttachDroidUnit( ent );

		// Did we kick someone in our pmove sequence?
	if (client->ps.forceKickFlip && !client->sess.raceMode)//Saber)
	{
		gentity_t *faceKicked = &g_entities[client->ps.forceKickFlip-1];

		if (faceKicked && faceKicked->client && (!OnSameTeam(ent, faceKicked) || g_friendlyFire.integer) &&
			(!faceKicked->client->ps.duelInProgress || faceKicked->client->ps.duelIndex == ent->s.number) &&
			(!ent->client->ps.duelInProgress || ent->client->ps.duelIndex == faceKicked->s.number)
			&& ((!ent->client->didGlitchKick || !ent->client->ps.fd.forceGripCripple) || g_glitchKickDamage.integer < 0))
		{
			if (faceKicked && faceKicked->client && faceKicked->health && faceKicked->takedamage && !faceKicked->client->sess.raceMode && !faceKicked->client->noclip)
			{//push them away and do pain
				vec3_t oppDir;
				int strength = (int)VectorNormalize2( client->ps.velocity, oppDir );
				int glitchKickBonus = 0;

				strength *= 0.05;

				VectorScale( oppDir, -1, oppDir );

				if (faceKicked->client->sess.movementStyle != MV_WSW) //gross hack to use dashtime as lastKickedByTime for jka (flipkick) physics
					faceKicked->client->ps.stats[STAT_DASHTIME] = 200;

				if (ent->client->ps.fd.forceGripCripple && g_glitchKickDamage.integer >= 0) {
					ent->client->didGlitchKick = qtrue;
					glitchKickBonus = g_glitchKickDamage.integer;
				}
				
//JAPRO - Serverside - New flipkick damage options - Start
				if (g_flipKick.integer < 2 && g_flipKickDamageScale.value)
					G_Damage( faceKicked, ent, ent, oppDir, client->ps.origin, ((strength + glitchKickBonus) * g_flipKickDamageScale.value), DAMAGE_NO_ARMOR, MOD_MELEE );//default flipkick dmg
				else if (g_flipKick.integer == 2 && g_flipKickDamageScale.value)
				{
					//int damageStrength = strength; //Revert this and use damageStrength here if we want to have flipkick knockback strength "random" i.e. give slight advantage to wait 2 kick scripters..
					if (strength > 10)
					{
						strength = 20 + glitchKickBonus;
					}
					G_Damage( faceKicked, ent, ent, 0, 0, (strength * g_flipKickDamageScale.value), DAMAGE_NO_ARMOR, MOD_MELEE ); //new japro flipkick dmg
				}
				else if (g_flipKickDamageScale.value)
					G_Damage( faceKicked, ent, ent, 0, 0, 20 + glitchKickBonus, DAMAGE_NO_ARMOR, MOD_MELEE ); //new japro flipkick dmg (20)

				if (g_fixKillCredit.integer) {//JAPRO - Serverside - fix flipkick not giving killcredit..?
					faceKicked->client->ps.otherKillerTime = level.time + 2000;
					faceKicked->client->ps.otherKiller = ent->s.number;
					faceKicked->client->ps.otherKillerDebounceTime = level.time + 100;
				}

//JAPRO - Serverside - New flipkick damage options - End

				if ( faceKicked->client->ps.weapon != WP_SABER ||
					 faceKicked->client->ps.fd.saberAnimLevel != FORCE_LEVEL_3 ||
					 (!BG_SaberInAttack(faceKicked->client->ps.saberMove) && !PM_SaberInStart(faceKicked->client->ps.saberMove) && !PM_SaberInReturn(faceKicked->client->ps.saberMove) && !PM_SaberInTransition(faceKicked->client->ps.saberMove)) )
				{
					if (faceKicked->health > 0 &&
						faceKicked->client->ps.stats[STAT_HEALTH] > 0 &&
						faceKicked->client->ps.forceHandExtend != HANDEXTEND_KNOCKDOWN)
					{

						if (BG_KnockDownable(&faceKicked->client->ps)) {
							if (g_nonRandomKnockdown.integer < 1) { //Default, random knockdowns
								if (Q_irand(1, 10) <= 3) {
									faceKicked->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
									faceKicked->client->ps.forceHandExtendTime = level.time + 1100;
									faceKicked->client->ps.forceDodgeAnim = 0; //this toggles between 1 and 0, when it's 1 we should play the get up anim
								}
							}
							else if (g_nonRandomKnockdown.integer == 1) { //forceDrainTime was unused technically, so hijack it for this.  forceHealTime is not accurate, its already 1000 ahead 
								if ((faceKicked->client->ps.fd.forceDrainTime > level.time - 2000) || (faceKicked->client->ps.fd.forceHealTime > level.time - 1000)) { //drained/ healed recently?? Fixme
									faceKicked->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
									faceKicked->client->ps.forceHandExtendTime = level.time + 1100;
									faceKicked->client->ps.forceDodgeAnim = 0;
								}
							}
							else if (g_nonRandomKnockdown.integer == 2) { //semi random... where u cant get 2 in a row.. and kd chance is increased for each kick u do thats not a KD
								if ((Q_irand(1, 10) * faceKicked->client->noKnockdownStreak) > 9) { //Average one knockdown every ~3 kicks.. same as before..?  but with less variance?
									faceKicked->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
									faceKicked->client->ps.forceHandExtendTime = level.time + 1100;
									faceKicked->client->ps.forceDodgeAnim = 0;
									faceKicked->client->noKnockdownStreak = 0;

									//Streak:   Chance new        Chance old:
									//0			0				  30% (always)
									//1			10%
									//2			60%
									//3			70%
									//4			80%
									//5			90%
									//6			90%
									//7			90%
									//8			90%
									//9			90%
									//10		100%
								}
								else
									faceKicked->client->noKnockdownStreak++;
							}
							else if (g_nonRandomKnockdown.integer == 3) {
									vec3_t diffOrigin;
									float diffAngle;

									//this sortof breaks getting sidekicked in that kickbomb gk.. maybe only have it knockdown if diffOrigin[pitch] is small or something.

									VectorSubtract(ent->client->ps.origin, faceKicked->client->ps.origin, diffOrigin);
									vectoangles(diffOrigin, diffOrigin);
									diffAngle = fabs(AngleDelta(faceKicked->client->ps.viewangles[YAW], diffOrigin[YAW]));

									//debug
									//trap->SendServerCommand( ent-g_entities, va("cp \"Kick angle (given): %.1f\n\n\n\n\n\n\n\n\n\n\n\n\"", diffAngle));
									//trap->SendServerCommand( faceKicked-g_entities, va("cp \"Kick angle (recieved): %.1f\n\n\n\n\n\n\n\n\n\n\n\n\"", diffAngle));

									//Higher diffangle = Higher chance of KD.. or..?
									if (diffAngle > 60.0f) {
										faceKicked->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
										faceKicked->client->ps.forceHandExtendTime = level.time + 1100;
										faceKicked->client->ps.forceDodgeAnim = 0;
									}
							}
							else if (g_nonRandomKnockdown.integer == 4) {
									vec3_t diffOrigin;
									float diffAngleYaw, anglePitch, pitchScale;

									//this sortof breaks getting sidekicked in that kickbomb gk.. maybe only have it knockdown if diffOrigin[pitch] is small or something.

									VectorSubtract(ent->client->ps.origin, faceKicked->client->ps.origin, diffOrigin);
									vectoangles(diffOrigin, diffOrigin);
									diffAngleYaw = fabs(AngleDelta(faceKicked->client->ps.viewangles[YAW], diffOrigin[YAW]));

									anglePitch = AngleNormalize180(diffOrigin[PITCH]); //maybe?

									
									pitchScale = fabs(anglePitch / 90.0f);
									//pitchScale *= pitchScale;

									if (pitchScale < 0.9) //bit of a sad hack to stop headkicks from doing knockdown so much since they dont really have any yaw component, but also not giving advantage to fast doubletap scripts
										pitchScale = 0;


									//trap->Print("anglepitch: %.2f, pitchscale: %.1f, new pitchscale: %.1f", anglePitch, pitchScale, 1 - pitchScale);

									pitchScale = 1 - pitchScale;

							

									//debug
									//trap->SendServerCommand( ent-g_entities, va("cp \"Kick angle (given): %.1f, chance: %.1f\n\n\n\n\n\n\n\n\n\n\n\n\"", diffAngleYaw, (diffAngleYaw / 180.0f) * pitchScale * 100.0f));
									//trap->SendServerCommand( faceKicked-g_entities, va("cp \"Kick angle (recieved): %.1f, chance: %.1f\n\n\n\n\n\n\n\n\n\n\n\n\"", diffAngleYaw, (diffAngleYaw / 180.0f) * pitchScale * 100.0f));

									if (Q_irand(1, 180) <= (diffAngleYaw * pitchScale)) { //0 percent chance of KD aimed directly at them, 100 percent aimed completely away... whatever
										faceKicked->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
										faceKicked->client->ps.forceHandExtendTime = level.time + 1100;
										faceKicked->client->ps.forceDodgeAnim = 0; 
									}
							}
							else if (g_nonRandomKnockdown.integer > 4) { //no KDs
							}						
						}

//JAPRO - Serverside - Fix killcredit stuff - Start
						if (!g_fixKillCredit.integer)
						{
							faceKicked->client->ps.otherKiller = ent->s.number;
							faceKicked->client->ps.otherKillerTime = level.time + 5000;
							faceKicked->client->ps.otherKillerDebounceTime = level.time + 100;
						}
//JAPRO - Serverside - Fix killcredit stuff - End

						faceKicked->client->ps.velocity[0] = oppDir[0]*(strength*40);
						faceKicked->client->ps.velocity[1] = oppDir[1]*(strength*40);
						faceKicked->client->ps.velocity[2] = 200; //something here might be different than ja+? how tell..
					}
				}
				G_Sound( faceKicked, CHAN_AUTO, G_SoundIndex( va("sound/weapons/melee/punch%d", Q_irand(1, 4)) ) );
			}
		}

		client->ps.forceKickFlip = 0;
	}

	// check for respawning
	if ( client->ps.stats[STAT_HEALTH] <= 0 
		&& !(client->ps.eFlags2&EF2_HELD_BY_MONSTER)//can't respawn while being eaten
		&& ent->s.eType != ET_NPC ) {
		// wait for the attack button to be pressed
		if ( level.time > client->respawnTime && !gDoSlowMoDuel ) {
			// forcerespawn is to prevent users from waiting out powerups
			int forceRes = g_forceRespawn.integer;

			if (level.gametype == GT_POWERDUEL)
			{
				forceRes = 1;
			}
			else if (level.gametype == GT_SIEGE && g_siegeRespawn.integer)
			{ //wave respawning on
				forceRes = 1;
			}

			if ( forceRes > 0 && 
				( level.time - client->respawnTime ) > forceRes * 1000 ) {
				ClientRespawn( ent );
				return;
			}
		
			// pressing attack or use is the normal respawn method
			if ( ucmd->buttons & ( BUTTON_ATTACK | BUTTON_USE_HOLDABLE ) ) {
				ClientRespawn( ent );
			}
		}
		else if (gDoSlowMoDuel)
		{
			client->respawnTime = level.time + 1000;
		}
		return;
	}

	
#if 0//_GRAPPLE
	//CHUNK 3
	// sanity check, deals with falling etc;

	//Com_Printf("Chunk 3 start\n");


	if ( ent->client->hook && ent->client->hook->think == Weapon_HookThink && CanGrapple(ent)) {
		ent->client->ps.pm_flags |= PMF_GRAPPLE;
	} else {
		ent->client->ps.pm_flags &= ~( PMF_GRAPPLE );
	}

	//if (ent->client && ent->s.eType != ET_NPC)
	//Com_Printf("Flags: %i, hasbeenfired: %i, fireheld: %i\n", ent->client->ps.pm_flags, ent->client->hookHasBeenFired, ent->client->fireHeld);
#endif
	

	//Copy current velocity to lastvelocity
	VectorCopy(ent->client->ps.velocity, ent->client->lastVelocity);

	// perform once-a-second actions
	//OSP: pause
	if ( level.pause.state == PAUSE_NONE || ent->client->sess.raceMode )
		ClientTimerActions( ent, msec );

	G_UpdateClientBroadcasts ( ent );

	//try some idle anims on ent if getting no input and not moving for some time
	G_CheckClientIdle( ent, ucmd );
	// This code was moved here from clientThink to fix a problem with g_synchronousClients 
	// being set to 1 when in vehicles. 
	if ( ent->s.number < MAX_CLIENTS && ent->client->ps.m_iVehicleNum )
	{//driving a vehicle
		//run it
		if (g_entities[ent->client->ps.m_iVehicleNum].inuse && g_entities[ent->client->ps.m_iVehicleNum].client)
		{
			ClientThink(ent->client->ps.m_iVehicleNum, &g_entities[ent->client->ps.m_iVehicleNum].m_pVehicle->m_ucmd);
		}
		else
		{ //vehicle no longer valid?
			ent->client->ps.m_iVehicleNum = 0;
		}
	}
}

/*
==================
G_CheckClientTimeouts

Checks whether a client has exceded any timeouts and act accordingly
==================
*/
void G_CheckClientTimeouts ( gentity_t *ent )
{
	// Only timeout supported right now is the timeout to spectator mode
	if ( !g_timeouttospec.integer )
	{
		return;
	}

	// Already a spectator, no need to boot them to spectator
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR )
	{
		return;
	}

	// See how long its been since a command was received by the client and if its 
	// longer than the timeout to spectator then force this client into spectator mode
	if ( level.time - ent->client->pers.cmd.serverTime > g_timeouttospec.integer * 1000 )
	{
		SetTeam ( ent, "spectator", qfalse );
	}
}

/*
==================
ClientThink

A new command has arrived from the client
==================
*/
void ClientThink( int clientNum, usercmd_t *ucmd ) {
	gentity_t *ent;
	ent = g_entities + clientNum;
	if (clientNum < MAX_CLIENTS)
	{
		trap->GetUsercmd( clientNum, &ent->client->pers.cmd );
	}

	// mark the time we got info, so we can display the
	// phone jack if they don't get any for a while
	ent->client->lastCmdTime = level.time;

	if (ucmd)
	{
		ent->client->pers.cmd = *ucmd; //Somehow this crashes the server if you try to board a vehicle without it having spawned yet...? somtimes?
	}

/* 	This was moved to clientthink_real, but since its sort of a risky change i left it here for 
    now as a more concrete reference - BSD
  
	if ( clientNum < MAX_CLIENTS
		&& ent->client->ps.m_iVehicleNum )
	{//driving a vehicle
		if (g_entities[ent->client->ps.m_iVehicleNum].client)
		{
			gentity_t *veh = &g_entities[ent->client->ps.m_iVehicleNum];

			if (veh->m_pVehicle &&
				veh->m_pVehicle->m_pPilot == (bgEntity_t *)ent)
			{ //only take input from the pilot...
				veh->client->ps.commandTime = ent->client->ps.commandTime;
				memcpy(&veh->m_pVehicle->m_ucmd, &ent->client->pers.cmd, sizeof(usercmd_t));
				if ( veh->m_pVehicle->m_ucmd.buttons & BUTTON_TALK )
				{ //forced input if "chat bubble" is up
					veh->m_pVehicle->m_ucmd.buttons = BUTTON_TALK;
					veh->m_pVehicle->m_ucmd.forwardmove = 0;
					veh->m_pVehicle->m_ucmd.rightmove = 0;
					veh->m_pVehicle->m_ucmd.upmove = 0;
				}
			}
		}
	}
*/
	if ( !(ent->r.svFlags & SVF_BOT) && !g_synchronousClients.integer ) {
		ClientThink_real( ent );
	}
	// vehicles are clients and when running synchronous they still need to think here
	// so special case them.
	else if ( clientNum >= MAX_CLIENTS ) {
		ClientThink_real( ent );
	}

/*	This was moved to clientthink_real, but since its sort of a risky change i left it here for 
    now as a more concrete reference - BSD
    
	if ( clientNum < MAX_CLIENTS
		&& ent->client->ps.m_iVehicleNum )
	{//driving a vehicle
		//run it
		if (g_entities[ent->client->ps.m_iVehicleNum].inuse &&
			g_entities[ent->client->ps.m_iVehicleNum].client)
		{
			ClientThink(ent->client->ps.m_iVehicleNum, &g_entities[ent->client->ps.m_iVehicleNum].m_pVehicle->m_ucmd);
		}
		else
		{ //vehicle no longer valid?
			ent->client->ps.m_iVehicleNum = 0;
		}
	}
*/
}


static void ForceClientUpdate(gentity_t *ent) {
	trap->GetUsercmd( ent-g_entities, &ent->client->pers.cmd );

	ent->client->lastCmdTime = level.time;

	// fill with seemingly valid data
	ent->client->pers.cmd.serverTime = level.time;

	//ent->client->pers.cmd.buttons = 0;
	//ent->client->pers.cmd.forwardmove = ent->client->pers.cmd.rightmove = ent->client->pers.cmd.upmove = 0;

	ent->client->pers.cmd.buttons = ent->client->pers.lastCmd.buttons;
	ent->client->pers.cmd.forwardmove = ent->client->pers.lastCmd.forwardmove;
	ent->client->pers.cmd.rightmove = ent->client->pers.lastCmd.rightmove;
	ent->client->pers.cmd.upmove = ent->client->pers.lastCmd.upmove;

	//Get rid of deadstops caused by lag (set their upmove to their last upmove) ..apparently this was really annoying for some people with bad net
	//Still fixes the problem of lagging through triggers, or freezing midair etc..

	ClientThink_real( ent );
}

void G_RunClient( gentity_t *ent ) {
	qboolean forceUpdateRate = qfalse;

	//If racemode , do forceclientupdaterate hardcoded at like 4/5 hz ?

	// force client updates if they're not sending packets at roughly 4hz

	if (ent->client->pers.recordingDemo) { //(ent->client->ps.pm_flags & PMF_FOLLOW) ?
		if (ent->client->pers.noFollow || ent->client->pers.practice || sv_cheats.integer || !ent->client->pers.userName[0] || !ent->client->sess.raceMode || !ent->client->pers.stats.startTime || (ent->client->sess.sessionTeam == TEAM_SPECTATOR) ||
			((ent->client->lastHereTime < level.time - 30000) && (level.time - ent->client->pers.demoStoppedTime > 10000)) ||
			(trap->Milliseconds() - ent->client->pers.stats.startTime > 240*60*1000)) // just give up on races longer than 4 hours lmao
		{
			//Their demo is bad, dont keep telling game to keep it
		}
		else 
			ent->client->pers.stopRecordingTime = level.time + 5000; //Their demo is good! tell game not to delete it yet
	}

	if (ent->client->pers.recordingDemo && (ent->client->pers.stopRecordingTime < level.time)) {
		ent->client->pers.recordingDemo = qfalse;
		ent->client->pers.demoStoppedTime = level.time;

		if (ent->client->pers.keepDemo) {
			//trap->SendServerCommand( ent-g_entities, "chat \"RECORDING STOPPED (timeout), HIGHSCORE\"");
			trap->SendConsoleCommand( EXEC_APPEND, va("svstoprecord %i;wait 10;svrenamedemo temp/%s races/%s\n", ent->s.number, ent->client->pers.oldDemoName, ent->client->pers.demoName));
		}
		else {
			//trap->SendServerCommand( ent-g_entities, va("chat \"RECORDING STOPPED for client %i\"", ent->client->ps.clientNum));
			trap->SendConsoleCommand( EXEC_APPEND, va("svstoprecord %i\n", ent->s.number));
		}
	}

	if (ent->client->pers.cmd.forwardmove || ent->client->pers.cmd.rightmove || ent->client->pers.cmd.upmove || (ent->client->pers.cmd.buttons & (BUTTON_ATTACK|BUTTON_ALT_ATTACK))) {
		ent->client->lastHereTime = level.time;
	}

	if (ent->client->sess.sessionTeam != TEAM_SPECTATOR && !(ent->r.svFlags & SVF_BOT)) {
		if (ent->client->sess.raceMode) {
			if (ent->client->lastCmdTime < (level.time - 250)) { //Force 250ms updaterate for racers?
				forceUpdateRate = qtrue;
			}
			if (ent->client->lastCmdTime < (level.time - (1000/sv_fps.integer))) {
				G_TouchTriggers( ent ); //They have bad FPS, so also check if they are in trigger here.
			}
			G_TouchTriggersWithTrace( ent ); //

			//Do AFKTime here, subtract it from racetime and clear it when racetime is added.
			if (level.time - ent->client->lastHereTime > 10000) { //They have been AFK for more than 10 seconds
				ent->client->afkDuration += 1000/sv_fps.integer;
			}

			/*
			ok, is it safe to do lerp/trace checks for timer triggers?
			we are only checking to see if we touch a trigger between our last spot, and our current spot.
			ignoring teles/warps/noclip/swoop boarding, that seems fine... 
			//This happens every sv_fps. NOT every client frame... so its possible that this could hit something that a client wouldnt have hit normally?... but really it would not be noticable.
			//to account for tele/warp/noclip/boarding, store previous teleport bit state, check if it was toggled, dont trace if so?
			//where do we store previous teleport bit state, i guess here in runclient?
			//if we are lagging bad, we do forceclientupdaterate and return, that could skip setting oldflags, oldorigin??

			//for hitting timer triggers, i dont see how this can be abused really... worth a try.

			*/

		}
		else {
			if (g_forceClientUpdateRate.integer && (ent->client->lastCmdTime < (level.time - g_forceClientUpdateRate.integer))) {
				forceUpdateRate = qtrue;
			}
		}
	}

	VectorCopy( ent->client->ps.origin, ent->client->oldOrigin );
	ent->client->oldFlags = ent->client->ps.eFlags; //fuck, this should be in runframe?

	if (forceUpdateRate) {
		ForceClientUpdate(ent);
		return;
	}

	ent->client->pers.lastCmd = ent->client->pers.cmd; //this should be after force update rate .... because..?


	if ( !(ent->r.svFlags & SVF_BOT) && !g_synchronousClients.integer ) {
		return;
	}
	ent->client->pers.cmd.serverTime = level.time;
	ClientThink_real( ent );
}


/*
==================
SpectatorClientEndFrame

==================
*/
void SpectatorClientEndFrame( gentity_t *ent ) {
	gclient_t	*cl;

	if (ent->s.eType == ET_NPC)
	{
		assert(0);
		return;
	}

	// if we are doing a chase cam or a remote view, grab the latest info
	if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
		int		clientNum;//, flags;

		clientNum = ent->client->sess.spectatorClient;

		// team follow1 and team follow2 go to whatever clients are playing
		if ( clientNum == -1 ) {
			clientNum = level.follow1;
		} else if ( clientNum == -2 ) {
			clientNum = level.follow2;
		}
		if ( clientNum >= 0 ) {
			cl = &level.clients[ clientNum ];
			if ( cl->pers.connected == CON_CONNECTED && cl->sess.sessionTeam != TEAM_SPECTATOR ) {
				//flags = (cl->mGameFlags & ~(PSG_VOTED | PSG_TEAMVOTED)) | (ent->client->mGameFlags & (PSG_VOTED | PSG_TEAMVOTED));
				//ent->client->mGameFlags = flags;
				ent->client->ps.eFlags = cl->ps.eFlags;
				ent->client->ps = cl->ps;
				ent->client->ps.pm_flags |= PMF_FOLLOW;

				if (g_blockDuelHealthSpec.integer && ent->client->ps.duelInProgress) {
					ent->client->ps.stats[STAT_ARMOR] = 0;
					ent->client->ps.stats[STAT_HEALTH] = 0;
				}

				//ent->client->ps.fd.forcePowersActive |= ( 1 << FP_SEE ); //spectator force sight wallhack
				//ent->client->ps.fd.forcePowerLevel[FP_SEE] = 2;

				return;
			} else {
				// drop them to free spectators unless they are dedicated camera followers
				if ( ent->client->sess.spectatorClient >= 0 ) {
					ent->client->sess.spectatorState = SPECTATOR_FREE;
					ClientBegin( ent->client - level.clients, qtrue );
				}
			}
		}
	}

	if ( ent->client->sess.spectatorState == SPECTATOR_SCOREBOARD ) {
		ent->client->ps.pm_flags |= PMF_SCOREBOARD;
	} else {
		ent->client->ps.pm_flags &= ~PMF_SCOREBOARD;
	}
}

/*
==============
ClientEndFrame

Called at the end of each server frame for each connected client
A fast client will have multiple ClientThink for each ClientEdFrame,
while a slow client may have multiple ClientEndFrame between ClientThink.
==============
*/
void ClientEndFrame( gentity_t *ent ) {
	int			i;
	qboolean isNPC = qfalse;
	int frames; //japro smoothclients

	if (ent->s.eType == ET_NPC)
	{
		isNPC = qtrue;
	}

	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		SpectatorClientEndFrame( ent );
		return;
	}

	// turn off any expired powerups
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {

		//OSP: pause
		//	If we're paused, update powerup timers accordingly.
		//	Make sure we dont let stuff like CTF flags expire.
		if ( ent->client->ps.powerups[i] == 0 ) //loda- whats this do?
			continue;

		if ( level.pause.state != PAUSE_NONE && ent->client->ps.powerups[i] != INT_MAX && !ent->client->sess.raceMode )
			ent->client->ps.powerups[i] += level.time - level.previousTime;

		if ( ent->client->ps.powerups[ i ] < level.time ) {
			ent->client->ps.powerups[ i ] = 0;
		}

	}


	//OSP: pause
	//	If we're paused, make sure other timers stay in sync
	if ( level.pause.state != PAUSE_NONE && !ent->client->sess.raceMode) {
		int time_delta = level.time - level.previousTime;

		ent->client->airOutTime += time_delta;
		ent->client->inactivityTime += time_delta;
		ent->client->pers.connectTime += time_delta;
		ent->client->pers.enterTime += time_delta;
		ent->client->pers.teamState.lastreturnedflag += time_delta;
		ent->client->pers.teamState.lasthurtcarrier += time_delta;
		ent->client->pers.teamState.lastfraggedcarrier += time_delta;
		ent->client->respawnTime += time_delta;
		ent->pain_debounce_time += time_delta;

		ent->client->force.drainDebounce += time_delta;
		ent->client->force.lightningDebounce += time_delta;
		//ent->client->forceDebounce.drain += time_delta;
		//ent->client->forceDebounce.lightning += time_delta;
		ent->client->ps.fd.forcePowerRegenDebounceTime += time_delta;
	}


	//
	// If the end of unit layout is displayed, don't give
	// the player any normal movement attributes
	//
	if ( level.intermissiontime ) {
		if ( ent->s.number < MAX_CLIENTS
			|| ent->client->NPC_class == CLASS_VEHICLE )
		{//players and vehicles do nothing in intermissions
			return;
		}
	}

	// burn from lava, etc
	P_WorldEffects (ent);

	// apply all the damage taken this frame
	P_DamageFeedback (ent);

	// add the EF_CONNECTION flag if we haven't gotten commands recently
	if ( level.time - ent->client->lastCmdTime > 1000 ) {
		if (g_lagIcon.integer) //Loda fixme should be up here ^ ?
			ent->client->ps.eFlags |= EF_CONNECTION;
	}
	else
		ent->client->ps.eFlags &= ~EF_CONNECTION;

	ent->client->ps.stats[STAT_HEALTH] = ent->health;	// FIXME: get rid of ent->health...

	G_SetClientSound (ent);

	// set the latest infor
	/*
	if (g_smoothClients.integer) {
		BG_PlayerStateToEntityStateExtraPolate( &ent->client->ps, &ent->s, ent->client->ps.commandTime, qfalse );
		//rww - 12-03-02 - Don't snap the origin of players! It screws prediction all up.
	}
	else {
	*/
		BG_PlayerStateToEntityState( &ent->client->ps, &ent->s, qfalse );
	//}

	if (isNPC)
	{
		ent->s.eType = ET_NPC;
	}

	SendPendingPredictableEvents( &ent->client->ps );

//unlagged - smooth clients #1 - japro
	// mark as not missing updates initially
	ent->client->ps.eFlags &= ~EF_CONNECTION;

	// see how many frames the client has missed
	frames = level.framenum - ent->client->lastUpdateFrame - 1;

	// don't extrapolate more than two frames
	if ( frames > 2 ) {
		frames = 2;

		// if they missed more than two in a row, show the phone jack
		if (g_lagIcon.integer) {
			ent->client->ps.eFlags |= EF_CONNECTION;
			ent->s.eFlags |= EF_CONNECTION;
		}
	}

	// did the client miss any frames?
	if ( frames > 0 && g_smoothClients.integer && VectorLength(ent->client->ps.velocity) >= 90 && !(ent->r.svFlags & SVF_BOT)) { // loda - sad hack fix this
		// yep, missed one or more, so extrapolate the player's movement
		//G_PredictPlayerMove( ent, (float)frames / sv_fps.integer );
		G_PredictPlayerStepSlideMove( ent, (float)frames / sv_fps.integer );
		// save network bandwidth
		SnapVector( ent->s.pos.trBase ); //is this wallbug???
	}
//unlagged - smooth clients #1 - japro


	G_StoreTrail( ent );
	// set the bit for the reachability area the client is currently in
//	i = trap->AAS_PointReachabilityAreaIndex( ent->client->ps.origin );
//	ent->client->areabits[i >> 3] |= 1 << (i & 7);
}