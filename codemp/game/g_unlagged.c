//NT - unlagged - new time-shift client functions

#include "g_local.h"
#include "g_unlagged.h"

static int frametime = 25;
static int historytime = 250;
static int numTrails = 10; //historytime/frametime; 
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
		VectorCopy( ent->r.currentAngles, ent->client->unlagged.trail[i].currentAngles );
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
		newtime = level.previousTime + trap->Milliseconds() - level.frameStartTime; //Loda fixme, this is never set?
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
	VectorCopy( ent->r.currentAngles, ent->client->unlagged.trail[head].currentAngles );
	ent->client->unlagged.trail[head].leveltime = level.time;
	ent->client->unlagged.trail[head].time = newtime;

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


/*
=================
G_TimeShiftClient

Move a client back to where he was at the specified "time"
=================
*/
void G_TimeShiftClient( gentity_t *ent, int time ) {
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
			VectorCopy( ent->r.currentAngles, ent->client->unlagged.saved.currentAngles );
			ent->client->unlagged.saved.leveltime = level.time;
		}

		// if we haven't wrapped back to the head, we've sandwiched, so
		// we shift the client's position back to where he was at "time"
		if ( j != ent->client->unlagged.trailHead )
		{
			float	frac = (float)(ent->client->unlagged.trail[k].time - time) / (float)(ent->client->unlagged.trail[k].time - ent->client->unlagged.trail[j].time);

			// FOR TESTING ONLY
			//Com_Printf( "level time: %d, fire time: %d, j time: %d, k time: %d\n", level.time, time, ent->client->unlagged.trail[j].time, ent->client->unlagged.trail[k].time );

			// interpolate between the two origins to give position at time index "time"
			TimeShiftLerp( frac, ent->client->unlagged.trail[k].currentOrigin, ent->client->unlagged.trail[j].currentOrigin, ent->r.currentOrigin );
			ent->r.currentAngles[YAW] = LerpAngle( ent->client->unlagged.trail[k].currentAngles[YAW], ent->r.currentAngles[YAW], frac );

			// lerp these too, just for fun (and ducking)
			TimeShiftLerp( frac, ent->client->unlagged.trail[k].mins, ent->client->unlagged.trail[j].mins, ent->r.mins );
			TimeShiftLerp( frac, ent->client->unlagged.trail[k].maxs, ent->client->unlagged.trail[j].maxs, ent->r.maxs );

			// this will recalculate absmin and absmax
			trap->LinkEntity( (sharedEntity_t *)ent );
		} else {
			// we wrapped, so grab the earliest
			VectorCopy( ent->client->unlagged.trail[k].currentAngles, ent->r.currentAngles );
			VectorCopy( ent->client->unlagged.trail[k].currentOrigin, ent->r.currentOrigin );
			VectorCopy( ent->client->unlagged.trail[k].mins, ent->r.mins );
			VectorCopy( ent->client->unlagged.trail[k].maxs, ent->r.maxs );

			// this will recalculate absmin and absmax
			trap->LinkEntity( (sharedEntity_t *)ent );
		}
	}
}


/*
=====================
G_TimeShiftAllClients

Move ALL clients back to where they were at the specified "time",
except for "skip"
=====================
*/
void G_TimeShiftAllClients( int time, gentity_t *skip ) {
	int			i;
	gentity_t	*ent;

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
			G_TimeShiftClient( ent, time );
		}
	}
}


/*
===================
G_UnTimeShiftClient

Move a client back to where he was before the time shift
===================
*/
void G_UnTimeShiftClient( gentity_t *ent ) {
	// if it was saved
	if ( ent->client->unlagged.saved.leveltime == level.time ) {
		// move it back
		VectorCopy( ent->client->unlagged.saved.mins, ent->r.mins );
		VectorCopy( ent->client->unlagged.saved.maxs, ent->r.maxs );
		VectorCopy( ent->client->unlagged.saved.currentOrigin, ent->r.currentOrigin );
		VectorCopy( ent->client->unlagged.saved.currentAngles, ent->r.currentAngles );
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
void G_UnTimeShiftAllClients( gentity_t *skip ) {
	int			i;
	gentity_t	*ent;

	ent = &g_entities[0];
	for ( i = 0; i < MAX_CLIENTS; i++, ent++) {
		if ( ent->client && ent->inuse && ent->client->sess.sessionTeam < TEAM_SPECTATOR && ent != skip ) {
			G_UnTimeShiftClient( ent );
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
		trap->Trace( &trace, origin, ent->r.mins, ent->r.maxs, end, ent->s.number, ent->clipmask, qfalse, 0, 0 );

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

	VectorCopy (ent->s.pos.trBase, start_o);
	VectorCopy (ent->s.pos.trDelta, start_v);

	if ( !G_PredictPlayerSlideMove( ent, frametime ) ) {
		// not clipped, so forget stepping
		return;
	}

	VectorCopy( ent->s.pos.trBase, down_o);
	VectorCopy( ent->s.pos.trDelta, down_v);

	VectorCopy (start_o, up);
	up[2] += STEPSIZE;

	// test the player position if they were a stepheight higher
	trap->Trace( &trace, start_o, ent->r.mins, ent->r.maxs, up, ent->s.number, ent->clipmask, qfalse, 0, 0);
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
	trap->Trace( &trace, ent->s.pos.trBase, ent->r.mins, ent->r.maxs, down, ent->s.number, ent->clipmask, qfalse, 0, 0 );
	if ( !trace.allsolid ) {
		VectorCopy( trace.endpos, ent->s.pos.trBase );
	}
	if ( trace.fraction < 1.0 ) {
		G_PredictPlayerClipVelocity( ent->s.pos.trDelta, trace.plane.normal, ent->s.pos.trDelta );
	}
}
