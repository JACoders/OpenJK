/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
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

// cg_snapshot.c -- things that happen on snapshot transition,
// not necessarily every single frame
#include "cg_local.h"

/*
==================
CG_ResetEntity
==================
*/
void CG_ResetEntity( centity_t *cent ) {
	// if an event is set, assume it is new enough to use
	// if the event had timed out, it would have been cleared
	cent->previousEvent = 0;

	cent->trailTime = cg.snap->serverTime;

	VectorCopy (cent->currentState.origin, cent->lerpOrigin);
	VectorCopy (cent->currentState.angles, cent->lerpAngles);
	if ( cent->currentState.eType == ET_PLAYER ) {
		CG_ResetPlayerEntity( cent );
	}
}

/*
===============
CG_TransitionEntity

cent->nextState is moved to cent->currentState and events are fired
===============
*/
void CG_TransitionEntity( centity_t *cent ) {
	cent->currentState = cent->nextState;
	cent->currentValid = qtrue;

	// reset if the entity wasn't in the last frame or was teleported
	if ( !cent->interpolate ) {
		CG_ResetEntity( cent );
	}

	// clear the next state.  if will be set by the next CG_SetNextSnap
	cent->interpolate = qfalse;

	// check for events
	CG_CheckEvents( cent );
}


/*
==================
CG_SetInitialSnapshot

This will only happen on the very first snapshot, or
on tourney restarts.  All other times will use
CG_TransitionSnapshot instead.
==================
*/
void CG_SetInitialSnapshot( snapshot_t *snap ) {
	int				i;
	centity_t		*cent;
	entityState_t	*state;

	cg.snap = snap;

	// sort out solid entities
	//CG_BuildSolidList();

	CG_ExecuteNewServerCommands( snap->serverCommandSequence );

	// set our local weapon selection pointer to
	// what the server has indicated the current weapon is
	CG_Respawn();

	for ( i = 0 ; i < cg.snap->numEntities ; i++ ) {
		state = &cg.snap->entities[ i ];
		cent = &cg_entities[ state->number ];

		cent->currentState = *state;
		cent->interpolate = qfalse;
		cent->currentValid = qtrue;

		CG_ResetEntity( cent );

		// check for events
		CG_CheckEvents( cent );
	}
}


/*
===================
CG_TransitionSnapshot

The transition point from snap to nextSnap has passed
===================
*/
void CG_TransitionSnapshot( void ) {
	centity_t			*cent;
	snapshot_t			*oldFrame;
	int					i;

	if ( !cg.snap ) {
		CG_Error( "CG_TransitionSnapshot: NULL cg.snap" );
	}
	if ( !cg.nextSnap ) {
		CG_Error( "CG_TransitionSnapshot: NULL cg.nextSnap" );
	}

	// execute any server string commands before transitioning entities
	CG_ExecuteNewServerCommands( cg.nextSnap->serverCommandSequence );

	// clear the currentValid flag for all entities in the existing snapshot
	for ( i = 0 ; i < cg.snap->numEntities ; i++ ) {
		cent = &cg_entities[ cg.snap->entities[ i ].number ];
		cent->currentValid = qfalse;
	}

	// move nextSnap to snap and do the transitions
	oldFrame = cg.snap;
	cg.snap = cg.nextSnap;

	// sort out solid entities
	//CG_BuildSolidList();

	for ( i = 0 ; i < cg.snap->numEntities ; i++ )
	{
		if ( 1 )//cg.snap->entities[ i ].number != 0 ) // I guess the player adds his/her events elsewhere, so doing this also gives us double events for the player!
		{
			cent = &cg_entities[ cg.snap->entities[ i ].number ];
			CG_TransitionEntity( cent );
		}
	}

	cg.nextSnap = NULL;

	// check for playerstate transition events
	if ( oldFrame ) {
		// if we are not doing client side movement prediction for any
		// reason, then the client events and view changes will be issued now
		if ( cg_timescale.value >= 1.0f )
		{
			CG_TransitionPlayerState( &cg.snap->ps, &oldFrame->ps );
		}
	}

}


/*
===============
CG_SetEntityNextState

Determine if the entity can be interpolated between the states
present in cg.snap and cg,nextSnap
===============
*/
void CG_SetEntityNextState( centity_t *cent, entityState_t *state ) {
	cent->nextState = *state;

	// since we can't interpolate ghoul2 stuff from one frame to another, I'm just going to copy the ghoul2 info directly into the current state now
//	CGhoul2Info *currentModel = &state->ghoul2[1];
//	cent->gent->ghoul2 = state->ghoul2;
//	CGhoul2Info *newModel = &cent->gent->ghoul2[1];


	// if this frame is a teleport, or the entity wasn't in the
	// previous frame, don't interpolate
	if ( !cent->currentValid || ( ( cent->currentState.eFlags ^ state->eFlags ) & EF_TELEPORT_BIT )  ) {
		cent->interpolate = qfalse;
	} else {
		cent->interpolate = qtrue;
	}
}


/*
===================
CG_SetNextSnap

A new snapshot has just been read in from the client system.
===================
*/
void CG_SetNextSnap( snapshot_t *snap ) {
	int					num;
	entityState_t		*es;
	centity_t			*cent;

	cg.nextSnap = snap;

	// check for extrapolation errors
	for ( num = 0 ; num < snap->numEntities ; num++ ) {
		es = &snap->entities[num];
		cent = &cg_entities[ es->number ];
		CG_SetEntityNextState( cent, es );
	}

	// if the next frame is a teleport for the playerstate,
	if ( cg.snap && ( ( snap->ps.eFlags ^ cg.snap->ps.eFlags ) & EF_TELEPORT_BIT ) ) {
		cg.nextFrameTeleport = qtrue;
	} else {
		cg.nextFrameTeleport = qfalse;
	}
}


/*
========================
CG_ReadNextSnapshot

This is the only place new snapshots are requested
This may increment cg.processedSnapshotNum multiple
times if the client system fails to return a
valid snapshot.
========================
*/
snapshot_t *CG_ReadNextSnapshot( void ) {
	qboolean	r;
	snapshot_t	*dest;

	while ( cg.processedSnapshotNum < cg.latestSnapshotNum ) {
		// decide which of the two slots to load it into
		if ( cg.snap == &cg.activeSnapshots[0] ) {
			dest = &cg.activeSnapshots[1];
		} else {
			dest = &cg.activeSnapshots[0];
		}

		// try to read the snapshot from the client system
		cg.processedSnapshotNum++;
		r = cgi_GetSnapshot( cg.processedSnapshotNum, dest );

		// if it succeeded, return
		if ( r ) {
			return dest;
		}

		// a GetSnapshot will return failure if the snapshot
		// never arrived, or  is so old that its entities
		// have been shoved off the end of the circular
		// buffer in the client system.

		// record as a dropped packet
//		CG_AddLagometerSnapshotInfo( NULL );

		// If there are additional snapshots, continue trying to
		// read them.
	}

	// nothing left to read
	return NULL;
}

/*
=================
CG_RestartLevel

A tournement restart will clear everything, but doesn't
require a reload of all the media
=================
*/
extern void CG_LinkCentsToGents(void);
void CG_RestartLevel( void ) {
	int		snapshotNum;
	int		r;

	snapshotNum = cg.processedSnapshotNum;

/*
Ghoul2 Insert Start
*/

//	memset( cg_entities, 0, sizeof( cg_entities ) );
	CG_Init_CGents();
// this is a No-No now we have stl vector classes in here.
//	memset( &cg, 0, sizeof( cg ) );
	CG_Init_CG();
/*
Ghoul2 Insert End
*/


	CG_LinkCentsToGents();
	CG_InitLocalEntities();
	CG_InitMarkPolys();

	// regrab the first snapshot of the restart

	cg.processedSnapshotNum = snapshotNum;
	r = cgi_GetSnapshot( cg.processedSnapshotNum, &cg.activeSnapshots[0] );
	if ( !r ) {
		CG_Error( "cgi_GetSnapshot failed on restart" );
	}

	CG_SetInitialSnapshot( &cg.activeSnapshots[0] );
	cg.time = cg.snap->serverTime;
}


/*
============
CG_ProcessSnapshots

We are trying to set up a renderable view, so determine
what the simulated time is, and try to get snapshots
both before and after that time if available.

If we don't have a valid cg.snap after exiting this function,
then a 3D game view cannot be rendered.  This should only happen
right after the initial connection.  After cg.snap has been valid
once, it will never turn invalid.

Even if cg.snap is valid, cg.nextSnap may not be, if the snapshot
hasn't arrived yet (it becomes an extrapolating situation instead
of an interpolating one)

============
*/
void CG_ProcessSnapshots( void ) {
	snapshot_t		*snap;
	int				n;

	// see what the latest snapshot the client system has is
	cgi_GetCurrentSnapshotNumber( &n, &cg.latestSnapshotTime );
	if ( n != cg.latestSnapshotNum ) {
		if ( n < cg.latestSnapshotNum ) {
			// this should never happen
			CG_Error( "CG_ProcessSnapshots: n < cg.latestSnapshotNum" );
		}
		cg.latestSnapshotNum = n;
	}

	// If we have yet to receive a snapshot, check for it.
	// Once we have gotten the first snapshot, cg.snap will
	// always have valid data for the rest of the game
	if ( !cg.snap ) {
		snap = CG_ReadNextSnapshot();
		if ( !snap ) {
			// we can't continue until we get a snapshot
			return;
		}

		// set our weapon selection to what
		// the playerstate is currently using
		CG_SetInitialSnapshot( snap );
	}

	// loop until we either have a valid nextSnap with a serverTime
	// greater than cg.time to interpolate towards, or we run
	// out of available snapshots
	do {
		// if we don't have a nextframe, try and read a new one in
		if ( !cg.nextSnap ) {
			snap = CG_ReadNextSnapshot();

			// if we still don't have a nextframe, we will just have to
			// extrapolate
			if ( !snap ) {
				break;
			}

			CG_SetNextSnap( snap );

			// if time went backwards, we have a level restart
			if ( cg.nextSnap->serverTime < cg.snap->serverTime ) {
				// restart the level
				CG_RestartLevel();
				continue;	// we might also get a nextsnap
			}
		}

		// if our time is < nextFrame's, we have a nice interpolating state
		if ( cg.time < cg.nextSnap->serverTime ) {
			break;
		}

		// we have passed the transition from nextFrame to frame
		CG_TransitionSnapshot();
	} while ( 1 );

	if ( cg.snap->serverTime > cg.time )
	{
		cg.time=cg.snap->serverTime;
#if _DEBUG
		Com_Printf("CG_ProcessSnapshots: cg.snap->serverTime > cg.time");
#endif

	}
	if ( cg.nextSnap != NULL && cg.nextSnap->serverTime <= cg.time )
	{
		cg.time=cg.nextSnap->serverTime-1;
#if _DEBUG
		Com_Printf("CG_ProcessSnapshots: cg.nextSnap->serverTime <= cg.time");
#endif
	}
	// assert our valid conditions upon exiting
	if ( cg.snap == NULL ) {
		CG_Error( "CG_ProcessSnapshots: cg.snap == NULL" );
	}
	if ( cg.snap->serverTime > cg.time ) {
		CG_Error( "CG_ProcessSnapshots: cg.snap->serverTime > cg.time" );
	}
	if ( cg.nextSnap != NULL && cg.nextSnap->serverTime <= cg.time ) {
		CG_Error( "CG_ProcessSnapshots: cg.nextSnap->serverTime <= cg.time" );
	}
}

