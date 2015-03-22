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

// cg_playerstate.c -- this file acts on changes in a new playerState_t
// With normal play, this will be done after local prediction, but when
// following another player or playing back a demo, it will be checked
// when the snapshot transitions like all the other entities

#include "cg_local.h"
#include "cg_media.h"

/*
==============
CG_CheckAmmo

If the ammo has gone low enough to generate the warning, play a sound
==============
*/
void CG_CheckAmmo( void ) 
{
//	int		i;
	int		total;
	int		previous;
//	int		weapons;

#if 0
		
	// see about how many seconds of ammo we have remaining
	weapons = cg.snap->ps.stats[ STAT_WEAPONS ];
	total = 0;

	for ( i = WP_STUN_BATON; i < WP_NUM_WEAPONS  i++ ) 
	{
		if ( ! ( weapons & ( 1 << i ) ) )
			continue;

		/*
		switch ( i ) 
		{
		case WP_ROCKET_LAUNCHER:
		case WP_GRENADE_LAUNCHER:
		case WP_RAILGUN:
		case WP_SHOTGUN:
			total += cg.snap->ps.ammo[i] * 1000;
			break;
		default:
			total += cg.snap->ps.ammo[i] * 200;
			break;
		}
		*/
		
		if ( total >= 5000 ) 
		{
			cg.lowAmmoWarning = 0;
			return;
		}
	}
#endif

	// Don't bother drawing the ammo warning when have no weapon selected
	if ( cg.weaponSelect == WP_NONE )
	{
		return;
	}

	total = cg.snap->ps.ammo[weaponData[cg.weaponSelect].ammoIndex];

	if (total > weaponData[cg.weaponSelect].ammoLow) // Low on ammo?
	{
		cg.lowAmmoWarning = 0;
		return;
	}


	previous = cg.lowAmmoWarning;

	if (!total)		// We're completely freak'in out!
	{
		cg.lowAmmoWarning = 2;
	} 
	else			// Got a little left
	{
		cg.lowAmmoWarning = 1;
	}

	// play a sound on transitions
	if ( cg.lowAmmoWarning != previous ) {
		cgi_S_StartLocalSound( cgs.media.noAmmoSound, CHAN_LOCAL_SOUND ); //"sound/weapons/noammo.wav"
	}
}

/*
==============
CG_DamageFeedback
==============
*/
void CG_DamageFeedback( int yawByte, int pitchByte, int damage ) {
	float		left, front, up;
	float		kick;
	int			health;
	float		scale;
	vec3_t		dir;
	vec3_t		angles;
	float		dist;
	float		yaw, pitch;

	//FIXME: Based on MOD, do different kinds of damage effects,
	//		for example, Borg damage could progressively tint screen green and raise FOV?

	// the lower on health you are, the greater the view kick will be
	health = cg.snap->ps.stats[STAT_HEALTH];
	if ( health < 40 ) {
		scale = 1;
	} else {
		scale = 40.0 / health;
	}
	kick = damage * scale;

	if (kick < 5)
		kick = 5;
	if (kick > 10)
		kick = 10;

	// if yaw and pitch are both 255, make the damage always centered (falling, etc)
	if ( yawByte == 255 && pitchByte == 255 ) {
		cg.damageX = 0;
		cg.damageY = 0;
		cg.v_dmg_roll = 0;
		cg.v_dmg_pitch = -kick;
	} else {
		// positional
		pitch = pitchByte / 255.0 * 360;
		yaw = yawByte / 255.0 * 360;

		angles[PITCH] = pitch;
		angles[YAW] = yaw;
		angles[ROLL] = 0;

		AngleVectors( angles, dir, NULL, NULL );
		VectorSubtract( vec3_origin, dir, dir );

		front = DotProduct (dir, cg.refdef.viewaxis[0] );
		left = DotProduct (dir, cg.refdef.viewaxis[1] );
		up = DotProduct (dir, cg.refdef.viewaxis[2] );

		dir[0] = front;
		dir[1] = left;
		dir[2] = 0;
		dist = VectorLength( dir );
		if ( dist < 0.1 ) {
			dist = 0.1f;
		}

		cg.v_dmg_roll = kick * left;
		
		cg.v_dmg_pitch = -kick * front;

		if ( front <= 0.1 ) {
			front = 0.1f;
		}
		cg.damageX = -left / front;
		cg.damageY = up / dist;
	}

	// clamp the position
	if ( cg.damageX > 1.0 ) {
		cg.damageX = 1.0;
	}
	if ( cg.damageX < - 1.0 ) {
		cg.damageX = -1.0;
	}

	if ( cg.damageY > 1.0 ) {
		cg.damageY = 1.0;
	}
	if ( cg.damageY < - 1.0 ) {
		cg.damageY = -1.0;
	}

	// don't let the screen flashes vary as much
	if ( kick > 10 ) {
		kick = 10;
	}
	cg.damageValue = kick;
	cg.v_dmg_time = cg.time + DAMAGE_TIME;
	cg.damageTime = cg.snap->serverTime;
}




/*
================
CG_Respawn

A respawn happened this snapshot
================
*/
void CG_Respawn( void ) {
	// no error decay on player movement
	cg.thisFrameTeleport = qtrue;

	// display weapons available
//	cg.weaponSelectTime = cg.time;
	SetWeaponSelectTime();

	// select the weapon the server says we are using
	cg.weaponSelect = cg.snap->ps.weapon;
}


/*
==============
CG_CheckPlayerstateEvents

==============
*/
void CG_CheckPlayerstateEvents( playerState_t *ps, playerState_t *ops ) {
	int			i;
	int			event;
	centity_t	*cent;

#if 0
	if ( ps->externalEvent && ps->externalEvent != ops->externalEvent ) {
		cent = &cg_entities[ ps->clientNum ];
		cent->currentState.event = ps->externalEvent;
		cent->currentState.eventParm = ps->externalEventParm;
		CG_EntityEvent( cent, cent->lerpOrigin );
	}
#endif

	for ( i = ps->eventSequence - MAX_PS_EVENTS ; i < ps->eventSequence ; i++ ) {
		if ( ps->events[i & (MAX_PS_EVENTS-1)] != ops->events[i & (MAX_PS_EVENTS-1)]
			|| i >= ops->eventSequence ) {
			event = ps->events[ i & (MAX_PS_EVENTS-1) ];

			cent = &cg_entities[ ps->clientNum ];
			cent->currentState.event = event;
			cent->currentState.eventParm = ps->eventParms[ i & (MAX_PS_EVENTS-1) ];
			CG_EntityEvent( cent, cent->lerpOrigin );
		}
	}
}

/*
==================
CG_CheckLocalSounds
==================
*/
/*
void CG_CheckLocalSounds( playerState_t *ps, playerState_t *ops ) {
	const char *s;

	// hit changes
	if ( ps->persistant[PERS_HITS] > ops->persistant[PERS_HITS] ) {
		cgi_S_StartLocalSound( "sound/feedback/hit.wav" );
	} else if ( ps->persistant[PERS_HITS] < ops->persistant[PERS_HITS] ) {
		cgi_S_StartLocalSound( "sound/feedback/hit_teammate.wav" );
	}

	// score up / down changes
	if ( ps->persistant[PERS_SCORE] > ops->persistant[PERS_SCORE] ) {
		cgi_S_StartLocalSound( "sound/feedback/scoreup.wav" );
	} else if ( ps->persistant[PERS_SCORE] < ops->persistant[PERS_SCORE] ) {
		cgi_S_StartLocalSound( "sound/feedback/scoredown.wav" );
	}

	// reward sounds
	if ( ps->persistant[PERS_REWARD_COUNT] > ops->persistant[PERS_REWARD_COUNT] ) {
		switch ( ps->persistant[PERS_REWARD] ) {
		case REWARD_IMPRESSIVE:
			cgi_S_StartLocalSound( "sound/feedback/impressive.wav" );
			break;
		case REWARD_EXCELLENT:
			cgi_S_StartLocalSound( "sound/feedback/excellent.wav" );
			break;
		case REWARD_DENIED:
			cgi_S_StartLocalSound( "sound/feedback/denied.wav" );
			break;

		default:
			CG_Error( "Bad reward_t" );
		}
	}

	// timelimit warnings
	if ( cgs.timelimit > 0 ) {
		if ( cgs.timelimit > 5 && !( cg.timelimitWarnings & 1 ) && cg.time > (cgs.timelimit - 5) * 60 * 1000 ) {
			cg.timelimitWarnings |= 1;
			cgi_S_StartLocalSound( "sound/feedback/5_minute.wav" );
		}
		if ( !( cg.timelimitWarnings & 2 ) && cg.time > (cgs.timelimit - 1) * 60 * 1000 ) {
			cg.timelimitWarnings |= 2;
			cgi_S_StartLocalSound( "sound/feedback/1_minute.wav" );
		}
		if ( !( cg.timelimitWarnings & 4 ) && cg.time > ( cgs.timelimit * 60 + 2 ) * 1000 ) {
			cg.timelimitWarnings |= 4;
			cgi_S_StartLocalSound( "sound/feedback/sudden_death.wav" );
		}
	}
}
*/

/*
===============
CG_TransitionPlayerState

===============
*/
void CG_TransitionPlayerState( playerState_t *ps, playerState_t *ops ) {
	// teleporting
	if ( ( ps->eFlags ^ ops->eFlags ) & EF_TELEPORT_BIT ) {
		cg.thisFrameTeleport = qtrue;
	} else {
		cg.thisFrameTeleport = qfalse;
	}

	// check for changing follow mode
	if ( ps->clientNum != ops->clientNum ) {
		cg.thisFrameTeleport = qtrue;
		// make sure we don't get any unwanted transition effects
		*ops = *ps;
	}

	// damage events (player is getting wounded)
	if ( ps->damageEvent != ops->damageEvent && ps->damageCount ) {
		CG_DamageFeedback( ps->damageYaw, ps->damagePitch, ps->damageCount );
	}

	// respawning
	if ( ps->persistant[PERS_SPAWN_COUNT] != ops->persistant[PERS_SPAWN_COUNT] ) {
		CG_Respawn();
	}

	// check for going low on ammo
	CG_CheckAmmo();

	// run events
	CG_CheckPlayerstateEvents( ps, ops );

	// smooth the ducking viewheight change
	if ( ps->viewheight != ops->viewheight ) 
	{
		if ( !cg.nextFrameTeleport )
		{//when we crouch/uncrouch in mid-air, our viewhieght doesn't actually change in
			//absolute world coordinates, just locally.
			cg.duckChange = ps->viewheight - ops->viewheight;
			cg.duckTime = cg.time;
		}
	}
}

