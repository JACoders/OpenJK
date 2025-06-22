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

//Client camera controls for cinematics

#include "cg_headers.h"

#include "cg_media.h"

#include "../game/g_roff.h"

bool		in_camera = false;
camera_t	client_camera={};
extern qboolean	player_locked;

extern gentity_t *G_Find (gentity_t *from, int fieldofs, const char *match);
extern void G_UseTargets (gentity_t *ent, gentity_t *activator);
void CGCam_FollowDisable( void );
void CGCam_TrackDisable( void );
void CGCam_Distance( float distance, qboolean initLerp );
void CGCam_DistanceDisable( void );
extern qboolean CG_CalcFOVFromX( float fov_x );
extern void WP_SaberCatch( gentity_t *self, gentity_t *saber, qboolean switchToSaber );

/*
TODO:
CloseUp, FullShot & Longshot commands:

  camera( CLOSEUP, <entity targetname>, angles(pitch yaw roll) )
  Will find the ent, apply angle offset to their head forward(minus pitch),
  get a preset distance away and set the FOV.  Trace to point, if less than
  1.0, put it there and open up FOV accordingly.
  Be sure to frame it so that eyespot and tag_head are positioned at proper
  places in the frame - ie: eyespot not in center, but not closer than 1/4
  screen width to the top...?
*/
/*
-------------------------
CGCam_Init
-------------------------
*/

void CGCam_Init( void )
{
	extern qboolean qbVidRestartOccured;
	if (!qbVidRestartOccured)
	{
		memset( &client_camera, 0, sizeof ( camera_t ) );
	}
}

/*
-------------------------
CGCam_Enable
-------------------------
*/
extern void CG_CalcVrect(void);
void CGCam_Enable( void )
{
	client_camera.bar_alpha = 0.0f;
	client_camera.bar_time = cg.time;

	client_camera.bar_alpha_source = 0.0f;
	client_camera.bar_alpha_dest = 1.0f;

	client_camera.bar_height_source = 0.0f;
	client_camera.bar_height_dest = 480/10;
	client_camera.bar_height = 0.0f;

	client_camera.info_state |= CAMERA_BAR_FADING;

	client_camera.FOV	= CAMERA_DEFAULT_FOV;
	client_camera.FOV2	= CAMERA_DEFAULT_FOV;

	in_camera = true;

	client_camera.next_roff_time = 0;

	if ( g_entities[0].inuse && g_entities[0].client )
	{
		//Player zero not allowed to do anything
		VectorClear( g_entities[0].client->ps.velocity );
		g_entities[0].contents = 0;

		if ( cg.zoomMode )
		{
			// need to shut off some form of zooming
			cg.zoomMode = 0;
		}

		if ( g_entities[0].client->ps.saberInFlight && g_entities[0].client->ps.saber[0].Active() )
		{//saber is out
			gentity_t *saberent = &g_entities[g_entities[0].client->ps.saberEntityNum];
			if ( saberent )
			{
				WP_SaberCatch( &g_entities[0], saberent, qfalse );
			}
		}

		for ( int i = 0; i < NUM_FORCE_POWERS; i++ )
		{//deactivate any active force powers
			g_entities[0].client->ps.forcePowerDuration[i] = 0;
extern void WP_ForcePowerStop( gentity_t *self, forcePowers_t forcePower );
			if ( g_entities[0].client->ps.forcePowerDuration[i] || (g_entities[0].client->ps.forcePowersActive&( 1 << i )) )
			{
				WP_ForcePowerStop( &g_entities[0], (forcePowers_t)i );
			}
		}
	}
}
/*
-------------------------
CGCam_Disable
-------------------------
*/

void CGCam_Disable( void )
{
	in_camera = false;

	client_camera.bar_alpha = 1.0f;
	client_camera.bar_time = cg.time;

	client_camera.bar_alpha_source = 1.0f;
	client_camera.bar_alpha_dest = 0.0f;

	client_camera.bar_height_source = 480/10;
	client_camera.bar_height_dest = 0.0f;

	client_camera.info_state |= CAMERA_BAR_FADING;

	if ( g_entities[0].inuse && g_entities[0].client )
	{
		g_entities[0].contents = CONTENTS_BODY;//MASK_PLAYERSOLID;
	}

	gi.SendServerCommand( 0, "cts");

	//if ( cg_skippingcin.integer )
	{//We're skipping the cinematic and it's over now
		gi.cvar_set("timescale", "1");
		gi.cvar_set("skippingCinematic", "0");
	}

	//we just came out of camera, so update cg.refdef.vieworg out of the camera's origin so the snapshot will know our new ori
	VectorCopy( g_entities[0].currentOrigin, cg.refdef.vieworg);
	VectorCopy( g_entities[0].client->ps.viewangles, cg.refdefViewAngles );
}

/*
-------------------------
CGCam_SetPosition
-------------------------
*/

void CGCam_SetPosition( vec3_t org )
{
	VectorCopy( org, client_camera.origin );
	VectorCopy( client_camera.origin, cg.refdef.vieworg );
}

/*
-------------------------
CGCam_Move
-------------------------
*/

void CGCam_Move( vec3_t dest, float duration )
{
	if ( client_camera.info_state & CAMERA_ROFFING )
	{
		client_camera.info_state &= ~CAMERA_ROFFING;
	}

	CGCam_TrackDisable();
	CGCam_DistanceDisable();

	if ( !duration )
	{
		client_camera.info_state &= ~CAMERA_MOVING;
		CGCam_SetPosition( dest );
		return;
	}

	client_camera.info_state |= CAMERA_MOVING;

	VectorCopy( dest, client_camera.origin2 );

	client_camera.move_duration = duration;
	client_camera.move_time = cg.time;
}

/*
-------------------------
CGCam_SetAngles
-------------------------
*/

void CGCam_SetAngles( vec3_t ang )
{
	VectorCopy( ang, client_camera.angles );
	VectorCopy(client_camera.angles, cg.refdefViewAngles );
}

/*
-------------------------
CGCam_Pan
-------------------------
*/

void CGCam_Pan( vec3_t dest, vec3_t panDirection, float duration )
{
	//vec3_t	panDirection = {0, 0, 0};
	int		i;
	float	delta1 , delta2;

	CGCam_FollowDisable();
	CGCam_DistanceDisable();

	if ( !duration )
	{
		CGCam_SetAngles( dest );
		client_camera.info_state &= ~CAMERA_PANNING;
		return;
	}

	//FIXME: make the dest an absolute value, and pass in a
	//panDirection as well.  If a panDirection's axis value is
	//zero, find the shortest difference for that axis.
	//Store the delta in client_camera.angles2.
	for( i = 0; i < 3; i++ )
	{
		dest[i] = AngleNormalize360( dest[i] );
		delta1 = dest[i] - AngleNormalize360( client_camera.angles[i] );
		if ( delta1 < 0 )
		{
			delta2 = delta1 + 360;
		}
		else
		{
			delta2 = delta1 - 360;
		}
		if ( !panDirection[i] )
		{//Didn't specify a direction, pick shortest
			if( Q_fabs(delta1) < Q_fabs(delta2) )
			{
				client_camera.angles2[i] = delta1;
			}
			else
			{
				client_camera.angles2[i] = delta2;
			}
		}
		else if ( panDirection[i] < 0 )
		{
			if( delta1 < 0 )
			{
				client_camera.angles2[i] = delta1;
			}
			else if( delta1 > 0 )
			{
				client_camera.angles2[i] = delta2;
			}
			else
			{//exact
				client_camera.angles2[i] = 0;
			}
		}
		else if ( panDirection[i] > 0 )
		{
			if( delta1 > 0 )
			{
				client_camera.angles2[i] = delta1;
			}
			else if( delta1 < 0 )
			{
				client_camera.angles2[i] = delta2;
			}
			else
			{//exact
				client_camera.angles2[i] = 0;
			}
		}
	}
	//VectorCopy( dest, client_camera.angles2 );

	client_camera.info_state |= CAMERA_PANNING;

	client_camera.pan_duration = duration;
	client_camera.pan_time = cg.time;
}

/*
-------------------------
CGCam_SetRoll
-------------------------
*/

void CGCam_SetRoll( float roll )
{
	client_camera.angles[2] = roll;
}

/*
-------------------------
CGCam_Roll
-------------------------
*/

void CGCam_Roll( float	dest, float duration )
{
	if ( !duration )
	{
		CGCam_SetRoll( dest );
		return;
	}

	//FIXME/NOTE: this will override current panning!!!
	client_camera.info_state |= CAMERA_PANNING;

	VectorCopy( client_camera.angles, client_camera.angles2 );
	client_camera.angles2[2] = AngleDelta( dest, client_camera.angles[2] );

	client_camera.pan_duration = duration;
	client_camera.pan_time = cg.time;
}

/*
-------------------------
CGCam_SetFOV
-------------------------
*/

void CGCam_SetFOV( float FOV )
{
	client_camera.FOV = FOV;
}

/*
-------------------------
CGCam_Zoom
-------------------------
*/

void CGCam_Zoom( float FOV, float duration )
{
	if ( !duration )
	{
		CGCam_SetFOV( FOV );
		return;
	}
	client_camera.info_state |= CAMERA_ZOOMING;

	client_camera.FOV_time	= cg.time;
	client_camera.FOV2		= FOV;

	client_camera.FOV_duration = duration;
}

void CGCam_Zoom2( float FOV, float FOV2, float duration )
{
	if ( !duration )
	{
		CGCam_SetFOV( FOV2 );
		return;
	}
	client_camera.info_state |= CAMERA_ZOOMING;

	client_camera.FOV_time	= cg.time;
	client_camera.FOV = FOV;
	client_camera.FOV2		= FOV2;

	client_camera.FOV_duration = duration;
}

void CGCam_ZoomAccel( float initialFOV, float fovVelocity, float fovAccel, float duration)
{
	if ( !duration )
	{
		return;
	}
	client_camera.info_state |= CAMERA_ACCEL;

	client_camera.FOV_time	= cg.time;
	client_camera.FOV2 = initialFOV;
	client_camera.FOV_vel = fovVelocity;
	client_camera.FOV_acc = fovAccel;

	client_camera.FOV_duration = duration;
}

/*
-------------------------
CGCam_Fade
-------------------------
*/

void CGCam_SetFade( vec4_t dest )
{//Instant completion
	client_camera.info_state &= ~CAMERA_FADING;
	client_camera.fade_duration = 0;
	VectorCopy4( dest, client_camera.fade_source );
	VectorCopy4( dest, client_camera.fade_color );
}

/*
-------------------------
CGCam_Fade
-------------------------
*/

void CGCam_Fade( vec4_t source, vec4_t dest, float duration )
{
	if ( !duration )
	{
		CGCam_SetFade( dest );
		return;
	}

	VectorCopy4( source, client_camera.fade_source );
	VectorCopy4( dest, client_camera.fade_dest );

	client_camera.fade_duration = duration;
	client_camera.fade_time = cg.time;

	client_camera.info_state |= CAMERA_FADING;
}

void CGCam_FollowDisable( void )
{
	client_camera.info_state &= ~CAMERA_FOLLOWING;
	client_camera.cameraGroup[0] = 0;
	client_camera.cameraGroupZOfs = 0;
	client_camera.cameraGroupTag[0] = 0;
}

void CGCam_TrackDisable( void )
{
	client_camera.info_state &= ~CAMERA_TRACKING;
	client_camera.trackEntNum = ENTITYNUM_WORLD;
}

void CGCam_DistanceDisable( void )
{
	client_camera.distance = 0;
}
/*
-------------------------
CGCam_Follow
-------------------------
*/

void CGCam_Follow( const char *cameraGroup, float speed, float initLerp )
{
	//Clear any previous
	CGCam_FollowDisable();

	if(!cameraGroup || !cameraGroup[0])
	{
		return;
	}

	if ( Q_stricmp("none", (char *)cameraGroup) == 0 )
	{//Turn off all aiming
		return;
	}

	if ( Q_stricmp("NULL", (char *)cameraGroup) == 0 )
	{//Turn off all aiming
		return;
	}

	//NOTE: if this interrupts a pan before it's done, need to copy the cg.refdef.viewAngles to the camera.angles!
	client_camera.info_state |= CAMERA_FOLLOWING;
	client_camera.info_state &= ~CAMERA_PANNING;

	//NULL terminate last char in case they type a name too long
	Q_strncpyz( client_camera.cameraGroup, cameraGroup, sizeof(client_camera.cameraGroup) );

	if ( speed )
	{
		client_camera.followSpeed = speed;
	}
	else
	{
		client_camera.followSpeed = 100.0f;
	}

	if ( initLerp )
	{
		client_camera.followInitLerp = qtrue;
	}
	else
	{
		client_camera.followInitLerp = qfalse;
	}
}

/*
-------------------------
Q3_CameraAutoAim

  Keeps camera pointed at an entity, usually will be a misc_camera_focus
  misc_camera_focus can be on a track that stays closest to it's subjects on that
  path (like Q3_CameraAutoTrack) or is can simply always put itself between it's subjects.
  misc_camera_focus can also set FOV/camera dist needed to keep the subjects in frame
-------------------------
*/

void CG_CameraAutoAim( const char *name )
{
	/*
	gentity_t *aimEnt = NULL;

	//Clear any previous
	CGCam_FollowDisable();

	if(Q_stricmp("none", (char *)name) == 0)
	{//Turn off all aiming
		return;
	}

	aimEnt = G_Find(NULL, FOFS(targetname), (char *)name);

	if(!aimEnt)
	{
		gi.Printf(S_COLOR_RED"ERROR: %s camera aim target not found\n", name);
		return;
	}

	//Lerp time...
	//aimEnt->aimDebounceTime = level.time;//FIXME: over time
	client_camera.aimEntNum = aimEnt->s.number;
	CGCam_Follow( aimEnt->cameraGroup, aimEnt->speed, aimEnt->spawnflags&1 );
	*/
}

/*
-------------------------
CGCam_Track
-------------------------
*/
void CGCam_Track( const char *trackName, float speed, float initLerp )
{
	gentity_t	*trackEnt = NULL;

	CGCam_TrackDisable();

	if(Q_stricmp("none", (char *)trackName) == 0)
	{//turn off tracking
		return;
	}

	//NOTE: if this interrupts a move before it's done, need to copy the cg.refdef.vieworg to the camera.origin!
	//This will find a path_corner now, not a misc_camera_track
	trackEnt = G_Find(NULL, FOFS(targetname), (char *)trackName);

	if ( !trackEnt )
	{
		gi.Printf(S_COLOR_RED"ERROR: %s camera track target not found\n", trackName);
		return;
	}

	client_camera.info_state |= CAMERA_TRACKING;
	client_camera.info_state &= ~CAMERA_MOVING;

	client_camera.trackEntNum = trackEnt->s.number;
	client_camera.initSpeed = speed/10.0f;
	client_camera.speed = speed;
	client_camera.nextTrackEntUpdateTime = cg.time;

	if ( initLerp )
	{
		client_camera.trackInitLerp = qtrue;
	}
	else
	{
		client_camera.trackInitLerp = qfalse;
	}
	/*
	if ( client_camera.info_state & CAMERA_FOLLOWING )
	{//Used to snap angles?  Do what...?
	}
	*/

	//Set a moveDir
	VectorSubtract( trackEnt->currentOrigin, client_camera.origin, client_camera.moveDir );

	if ( !client_camera.trackInitLerp )
	{//want to snap to first position
		//Snap to trackEnt's origin
		VectorCopy( trackEnt->currentOrigin, client_camera.origin );

		//Set new moveDir if trackEnt has a next path_corner
		//Possible that track has no next point, in which case we won't be moving anyway
		if ( trackEnt->target && trackEnt->target[0] )
		{
			gentity_t *newTrackEnt = G_Find( NULL, FOFS(targetname), trackEnt->target );
			if ( newTrackEnt )
			{
				VectorSubtract( newTrackEnt->currentOrigin, client_camera.origin, client_camera.moveDir );
			}
		}
	}

	VectorNormalize( client_camera.moveDir );
}

/*
-------------------------
Q3_CameraAutoTrack

  Keeps camera a certain distance from target entity but on the specified CameraPath
  The distance can be set in a script or derived from a misc_camera_focus.
  Dist will interpolate when changed, can also set acceleration/deceleration values.
  FOV will also interpolate.

  CameraPath might be a MAX path or perhaps a series of path_corners on the map itself
-------------------------
*/

void CG_CameraAutoTrack( const char *name )
{
	/*
	gentity_t *trackEnt = NULL;

	CGCam_TrackDisable();

	if(Q_stricmp("none", (char *)name) == 0)
	{//turn off tracking
		return;
	}

	//This will find a path_corner now, not a misc_camera_track
	trackEnt = G_Find(NULL, FOFS(targetname), (char *)name);

	if(!trackEnt)
	{
		gi.Printf(S_COLOR_RED"ERROR: %s camera track target not found\n", name);
		return;
	}

	//FIXME: last arg will be passed in
	CGCam_Track( trackEnt->s.number, trackEnt->speed, qfalse );
	//FIXME: this will be a seperate call
	CGCam_Distance( trackEnt->radius, qtrue);
	*/
}

/*
-------------------------
CGCam_Distance
-------------------------
*/

void CGCam_Distance( float distance, float initLerp )
{
	client_camera.distance = distance;

	if ( initLerp )
	{
		client_camera.distanceInitLerp = qtrue;
	}
	else
	{
		client_camera.distanceInitLerp = qfalse;
	}
}

//========================================================================================


void CGCam_FollowUpdate ( void )
{
	vec3_t		center, dir, cameraAngles, vec, focus[MAX_CAMERA_GROUP_SUBJECTS];//No more than 16 subjects in a cameraGroup
	gentity_t	*from = NULL;
	centity_t	*fromCent = NULL;
	int			num_subjects = 0, i;
	qboolean	focused = qfalse;

	if ( client_camera.cameraGroup[0] )
	{
		//Stay centered in my cameraGroup, if I have one
		while( NULL != (from = G_Find(from, FOFS(cameraGroup), client_camera.cameraGroup)))
		{
			/*
			if ( from->s.number == client_camera.aimEntNum )
			{//This is the misc_camera_focus, we'll be removing this ent altogether eventually
				continue;
			}
			*/

			if ( num_subjects >= MAX_CAMERA_GROUP_SUBJECTS )
			{
				gi.Printf(S_COLOR_RED"ERROR: Too many subjects in shot composition %s", client_camera.cameraGroup);
				break;
			}

			fromCent = &cg_entities[from->s.number];
			if ( !fromCent )
			{
				continue;
			}

			focused = qfalse;
			if ( from->client && client_camera.cameraGroupTag[0] && fromCent->gent->ghoul2.size() )
			{
				int newBolt = gi.G2API_AddBolt( &fromCent->gent->ghoul2[from->playerModel], client_camera.cameraGroupTag );
				if ( newBolt != -1 )
				{
					mdxaBone_t	boltMatrix;
					vec3_t	fromAngles = {0,from->client->ps.legsYaw,0};

					gi.G2API_GetBoltMatrix( fromCent->gent->ghoul2, from->playerModel, newBolt, &boltMatrix, fromAngles, fromCent->lerpOrigin, cg.time, cgs.model_draw, fromCent->currentState.modelScale );
					gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, focus[num_subjects] );

					focused = qtrue;
				}
			}
			if ( !focused )
			{
				if ( from->s.pos.trType != TR_STATIONARY )
//				if ( from->s.pos.trType == TR_INTERPOLATE )
				{//use interpolated origin?
					if ( !VectorCompare( vec3_origin, fromCent->lerpOrigin ) )
					{//hunh?  Somehow we've never seen this gentity on the client, so there is no lerpOrigin, so cheat over to the game and use the currentOrigin
						VectorCopy( from->currentOrigin, focus[num_subjects] );
					}
					else
					{
						VectorCopy( fromCent->lerpOrigin, focus[num_subjects] );
					}
				}
				else
				{
					VectorCopy(from->currentOrigin, focus[num_subjects]);
				}
				//FIXME: make a list here of their s.numbers instead so we can do other stuff with the list below
				if ( from->client )
				{//Track to their eyes - FIXME: maybe go off a tag?
					//FIXME:
					//Based on FOV and distance to subject from camera, pick the point that
					//keeps eyes 3/4 up from bottom of screen... what about bars?
					focus[num_subjects][2] += from->client->ps.viewheight;
				}
			}
			if ( client_camera.cameraGroupZOfs )
			{
				focus[num_subjects][2] += client_camera.cameraGroupZOfs;
			}
			num_subjects++;
		}

		if ( !num_subjects )	// Bad cameragroup
		{
#ifndef FINAL_BUILD
			gi.Printf(S_COLOR_RED"ERROR: Camera Focus unable to locate cameragroup: %s\n", client_camera.cameraGroup);
#endif
			return;
		}

		//Now average all points
		VectorCopy( focus[0], center );
		for( i = 1; i < num_subjects; i++ )
		{
			VectorAdd( focus[i], center, center );
		}
		VectorScale( center, 1.0f/((float)num_subjects), center );
	}
	else
	{
		return;
	}

	//Need to set a speed to keep a distance from
	//the subject- fixme: only do this if have a distance
	//set
	VectorSubtract( client_camera.subjectPos, center, vec );
	client_camera.subjectSpeed = VectorLengthSquared( vec ) * 100.0f / cg.frametime;

	/*
	if ( !cg_skippingcin.integer )
	{
		Com_Printf( S_COLOR_RED"org: %s\n", vtos(center) );
	}
	*/
	VectorCopy( center, client_camera.subjectPos );

	VectorSubtract( center, cg.refdef.vieworg, dir );//can't use client_camera.origin because it's not updated until the end of the move.

	//Get desired angle
	vectoangles(dir, cameraAngles);

	if ( client_camera.followInitLerp )
	{//Lerping
		float frac = cg.frametime/100.0f * client_camera.followSpeed/100.f;
		for( i = 0; i < 3; i++ )
		{
			cameraAngles[i] = AngleNormalize180( cameraAngles[i] );
			cameraAngles[i] = AngleNormalize180( client_camera.angles[i] + frac * AngleNormalize180(cameraAngles[i] - client_camera.angles[i]) );
			cameraAngles[i] = AngleNormalize180( cameraAngles[i] );
		}
#if 0
		Com_Printf( "%s\n", vtos(cameraAngles) );
#endif
	}
	else
	{//Snapping, should do this first time if follow_lerp_to_start_duration is zero
		//will lerp from this point on
		client_camera.followInitLerp = qtrue;
		for( i = 0; i < 3; i++ )
		{//normalize so that when we start lerping, it doesn't freak out
			cameraAngles[i] = AngleNormalize180( cameraAngles[i] );
		}
		//So tracker doesn't move right away thinking the first angle change
		//is the subject moving... FIXME: shouldn't set this until lerp done OR snapped?
		client_camera.subjectSpeed = 0;
	}

	//Point camera to lerp angles
	/*
	if ( !cg_skippingcin.integer )
	{
		Com_Printf( "ang: %s\n", vtos(cameraAngles) );
	}
	*/
	VectorCopy( cameraAngles, client_camera.angles );
}

void CGCam_TrackEntUpdate ( void )
{//FIXME: only do every 100 ms
	gentity_t	*trackEnt = NULL;
	gentity_t	*newTrackEnt = NULL;
	qboolean	reached = qfalse;
	vec3_t		vec;
	float		dist;

	if ( client_camera.trackEntNum >= 0 && client_camera.trackEntNum < ENTITYNUM_WORLD )
	{//We're already heading to a path_corner
		trackEnt = &g_entities[client_camera.trackEntNum];
		VectorSubtract( trackEnt->currentOrigin, client_camera.origin, vec );
		dist = VectorLengthSquared( vec );
		if ( dist < 256 )//16 squared
		{//FIXME: who should be doing the using here?
			G_UseTargets( trackEnt, trackEnt );
			reached = qtrue;
		}
	}

	if ( trackEnt && reached )
	{

		if ( trackEnt->target && trackEnt->target[0] )
		{//Find our next path_corner
			newTrackEnt = G_Find( NULL, FOFS(targetname), trackEnt->target );
			if ( newTrackEnt )
			{
				if ( newTrackEnt->radius < 0 )
				{//Don't bother trying to maintain a radius
					client_camera.distance = 0;
					client_camera.speed = client_camera.initSpeed;
				}
				else if ( newTrackEnt->radius > 0 )
				{
					client_camera.distance = newTrackEnt->radius;
				}

				if ( newTrackEnt->speed < 0 )
				{//go back to our default speed
					client_camera.speed = client_camera.initSpeed;
				}
				else if ( newTrackEnt->speed > 0 )
				{
					client_camera.speed = newTrackEnt->speed/10.0f;
				}
			}
		}
		else
		{//stop thinking if this is the last one
			CGCam_TrackDisable();
		}
	}

	if ( newTrackEnt )
	{//Update will lerp this
		client_camera.info_state |= CAMERA_TRACKING;
		client_camera.trackEntNum = newTrackEnt->s.number;
		VectorCopy( newTrackEnt->currentOrigin, client_camera.trackToOrg );
	}

	client_camera.nextTrackEntUpdateTime = cg.time + 100;
}

void CGCam_TrackUpdate ( void )
{
	vec3_t		goalVec, curVec, trackPos, vec;
	float		goalDist, dist;
	//qboolean	slowDown = qfalse;

	if ( client_camera.nextTrackEntUpdateTime <= cg.time )
	{
		CGCam_TrackEntUpdate();
	}

	VectorSubtract( client_camera.trackToOrg, client_camera.origin, goalVec );
	goalDist = VectorNormalize( goalVec );
	if ( goalDist > 100 )
	{
		goalDist = 100;
	}
	else if ( goalDist < 10 )
	{
		goalDist = 10;
	}

	if ( client_camera.distance && client_camera.info_state & CAMERA_FOLLOWING )
	{
		float	adjust = 0.0f, desiredSpeed = 0.0f;
		float	dot;

		if ( !client_camera.distanceInitLerp )
		{
			VectorSubtract( client_camera.origin, client_camera.subjectPos, vec );
			VectorNormalize( vec );
			//FIXME: use client_camera.moveDir here?
			VectorMA( client_camera.subjectPos, client_camera.distance, vec, client_camera.origin );
			//Snap to first time only
			client_camera.distanceInitLerp = qtrue;
			return;
		}
		else if ( client_camera.subjectSpeed > 0.05f )
		{//Don't start moving until subject moves
			VectorSubtract( client_camera.subjectPos, client_camera.origin, vec );
			dist = VectorNormalize(vec);
			dot = DotProduct(goalVec, vec);

			if ( dist > client_camera.distance )
			{//too far away
				if ( dot > 0 )
				{//Camera is moving toward the subject
					adjust = (dist - client_camera.distance);//Speed up
				}
				else if ( dot < 0 )
				{//Camera is moving away from the subject
					adjust = (dist - client_camera.distance) * -1.0f;//Slow down
				}
			}
			else if ( dist < client_camera.distance )
			{//too close
				if(dot > 0)
				{//Camera is moving toward the subject
					adjust = (client_camera.distance - dist) * -1.0f;//Slow down
				}
				else if(dot < 0)
				{//Camera is moving away from the subject
					adjust = (client_camera.distance - dist);//Speed up
				}
			}

			//Speed of the focus + our error
			//desiredSpeed = aimCent->gent->speed + (adjust * cg.frametime/100.0f);//cg.frameInterpolation);
			desiredSpeed = (adjust);// * cg.frametime/100.0f);//cg.frameInterpolation);

			//self->moveInfo.speed = desiredSpeed;

			//Don't change speeds faster than 10 every 10th of a second
			float	max_allowed_accel = MAX_ACCEL_PER_FRAME * (cg.frametime/100.0f);

			if ( !client_camera.subjectSpeed )
			{//full stop
				client_camera.speed = desiredSpeed;
			}
			else if ( client_camera.speed - desiredSpeed > max_allowed_accel )
			{//new speed much slower, slow down at max accel
				client_camera.speed -= max_allowed_accel;
			}
			else if ( desiredSpeed - client_camera.speed > max_allowed_accel )
			{//new speed much faster, speed up at max accel
				client_camera.speed += max_allowed_accel;
			}
			else
			{//remember this speed
				client_camera.speed = desiredSpeed;
			}

			//Com_Printf("Speed: %4.2f (%4.2f)\n", self->moveInfo.speed, aimCent->gent->speed);
		}
	}
	else
	{
		//slowDown = qtrue;
	}


	//FIXME: this probably isn't right, round it out more
	VectorScale( goalVec, cg.frametime/100.0f, goalVec );
	VectorScale( client_camera.moveDir, (100.0f - cg.frametime)/100.0f, curVec );
	VectorAdd( goalVec, curVec, client_camera.moveDir );
	VectorNormalize( client_camera.moveDir );
	/*if(slowDown)
	{
		VectorMA( client_camera.origin, client_camera.speed * goalDist/100.0f * cg.frametime/100.0f, client_camera.moveDir, trackPos );
	}
	else*/
	{
		VectorMA( client_camera.origin, client_camera.speed * cg.frametime/100.0f , client_camera.moveDir, trackPos );
	}

	//FIXME: Implement
	//Need to find point on camera's path that is closest to the desired distance from subject
	//OR: Need to intelligently pick this desired distance based on framing...
	VectorCopy( trackPos, client_camera.origin );
}

//=========================================================================================

/*
-------------------------
CGCam_UpdateBarFade
-------------------------
*/

void CGCam_UpdateBarFade( void )
{
	if ( client_camera.bar_time + BAR_DURATION < cg.time )
	{
		client_camera.bar_alpha = client_camera.bar_alpha_dest;
		client_camera.info_state &= ~CAMERA_BAR_FADING;
		client_camera.bar_height = client_camera.bar_height_dest;
	}
	else
	{
		client_camera.bar_alpha  = client_camera.bar_alpha_source + ( ( client_camera.bar_alpha_dest - client_camera.bar_alpha_source ) / BAR_DURATION ) * ( cg.time - client_camera.bar_time );;
		client_camera.bar_height = client_camera.bar_height_source + ( ( client_camera.bar_height_dest - client_camera.bar_height_source ) / BAR_DURATION ) * ( cg.time - client_camera.bar_time );;
	}
}

/*
-------------------------
CGCam_UpdateFade
-------------------------
*/

void CGCam_UpdateFade( void )
{
	if ( client_camera.info_state & CAMERA_FADING )
	{
		if ( client_camera.fade_time + client_camera.fade_duration < cg.time )
		{
			VectorCopy4( client_camera.fade_dest, client_camera.fade_color );
			client_camera.info_state &= ~CAMERA_FADING;
		}
		else
		{
			for ( int i = 0; i < 4; i++ )
			{
				client_camera.fade_color[i] = client_camera.fade_source[i] + (( ( client_camera.fade_dest[i] - client_camera.fade_source[i] ) ) / client_camera.fade_duration ) * ( cg.time - client_camera.fade_time );
			}
		}
	}
}
/*
-------------------------
CGCam_Update
-------------------------
*/
static void CGCam_Roff( void );

void CGCam_Update( void )
{
	int	i;
	qboolean	checkFollow = qfalse;
	qboolean	checkTrack = qfalse;

	// Apply new roff data to the camera as needed
	if ( client_camera.info_state & CAMERA_ROFFING )
	{
		CGCam_Roff();
	}

	//Check for a zoom
	if (client_camera.info_state & CAMERA_ACCEL)
	{
		// x = x0 + vt + 0.5*a*t*t
		float	actualFOV_X = client_camera.FOV;
		float	sanityMin = 1, sanityMax = 180;
		float	t = (cg.time - client_camera.FOV_time)*0.001; // mult by 0.001 cuz otherwise t is too darned big
		float	fovDuration = client_camera.FOV_duration;

#ifndef FINAL_BUILD
		if (cg_roffval4.integer)
		{
			fovDuration = cg_roffval4.integer;
		}
#endif
		if ( client_camera.FOV_time + fovDuration < cg.time )
		{
			client_camera.info_state &= ~CAMERA_ACCEL;
		}
		else
		{
			float	initialPosVal = client_camera.FOV2;
			float	velVal = client_camera.FOV_vel;
			float	accVal = client_camera.FOV_acc;

#ifndef FINAL_BUILD
			if (cg_roffdebug.integer)
			{
				if (fabs(cg_roffval1.value) > 0.001f)
				{
					initialPosVal = cg_roffval1.value;
				}
				if (fabs(cg_roffval2.value) > 0.001f)
				{
					velVal = cg_roffval2.value;
				}
				if (fabs(cg_roffval3.value) > 0.001f)
				{
					accVal = cg_roffval3.value;
				}
			}
#endif
			float	initialPos = initialPosVal;
			float	vel = velVal*t;
			float	acc = 0.5*accVal*t*t;

			actualFOV_X = initialPos + vel + acc;
			if (cg_roffdebug.integer)
			{
				Com_Printf("%d: fovaccel from %2.1f using vel = %2.4f, acc = %2.4f (current fov calc = %5.6f)\n",
					cg.time, initialPosVal, velVal, accVal, actualFOV_X);
			}

			if (actualFOV_X < sanityMin)
			{
				actualFOV_X = sanityMin;
			}
			else if (actualFOV_X > sanityMax)
			{
				actualFOV_X = sanityMax;
			}
			client_camera.FOV = actualFOV_X;
		}
		CG_CalcFOVFromX( actualFOV_X );
	}
	else if ( client_camera.info_state & CAMERA_ZOOMING )
	{
		float	actualFOV_X;

		if ( client_camera.FOV_time + client_camera.FOV_duration < cg.time )
		{
			actualFOV_X = client_camera.FOV = client_camera.FOV2;
			client_camera.info_state &= ~CAMERA_ZOOMING;
		}
		else
		{
			actualFOV_X = client_camera.FOV + (( ( client_camera.FOV2 - client_camera.FOV ) ) / client_camera.FOV_duration ) * ( cg.time - client_camera.FOV_time );
		}
		CG_CalcFOVFromX( actualFOV_X );
	}
	else
	{
		CG_CalcFOVFromX( client_camera.FOV );
	}

	//Check for roffing angles
	if ( (client_camera.info_state & CAMERA_ROFFING) && !(client_camera.info_state & CAMERA_FOLLOWING) )
	{
		if (client_camera.info_state & CAMERA_CUT)
		{
			// we're doing a cut, so just go to the new angles. none of this hifalutin lerping business.
			for ( i = 0; i < 3; i++ )
			{
				cg.refdefViewAngles[i] = AngleNormalize360( ( client_camera.angles[i] + client_camera.angles2[i] ) );
			}
		}
		else
		{
			for ( i = 0; i < 3; i++ )
			{
				cg.refdefViewAngles[i] =  client_camera.angles[i] + ( client_camera.angles2[i] / client_camera.pan_duration ) * ( cg.time - client_camera.pan_time );
			}
		}
	}
	else if ( client_camera.info_state & CAMERA_PANNING )
	{
		if (client_camera.info_state & CAMERA_CUT)
		{
			// we're doing a cut, so just go to the new angles. none of this hifalutin lerping business.
			for ( i = 0; i < 3; i++ )
			{
				cg.refdefViewAngles[i] = AngleNormalize360( ( client_camera.angles[i] + client_camera.angles2[i] ) );
			}
		}
		else
		{
			//Note: does not actually change the camera's angles until the pan time is done!
			if ( client_camera.pan_time + client_camera.pan_duration < cg.time )
			{//finished panning
				for ( i = 0; i < 3; i++ )
				{
					client_camera.angles[i] = AngleNormalize360( ( client_camera.angles[i] + client_camera.angles2[i] ) );
				}

				client_camera.info_state &= ~CAMERA_PANNING;
				VectorCopy(client_camera.angles, cg.refdefViewAngles );
			}
			else
			{//still panning
				for ( i = 0; i < 3; i++ )
				{
					//NOTE: does not store the resultant angle in client_camera.angles until pan is done
					cg.refdefViewAngles[i] = client_camera.angles[i] + ( client_camera.angles2[i] / client_camera.pan_duration ) * ( cg.time - client_camera.pan_time );
				}
			}
		}
	}
	else
	{
		checkFollow = qtrue;
	}

	//Check for movement
	if ( client_camera.info_state & CAMERA_MOVING )
	{
		//NOTE: does not actually move the camera until the movement time is done!
		if ( client_camera.move_time + client_camera.move_duration < cg.time )
		{
			VectorCopy( client_camera.origin2, client_camera.origin );
			client_camera.info_state &= ~CAMERA_MOVING;
			VectorCopy( client_camera.origin, cg.refdef.vieworg );
		}
		else
		{
			if (client_camera.info_state & CAMERA_CUT)
			{
				// we're doing a cut, so just go to the new origin. none of this fancypants lerping stuff.
				for ( i = 0; i < 3; i++ )
				{
					cg.refdef.vieworg[i] = client_camera.origin2[i];
				}
			}
			else
			{
				for ( i = 0; i < 3; i++ )
				{
					cg.refdef.vieworg[i] = client_camera.origin[i] + (( ( client_camera.origin2[i] - client_camera.origin[i] ) ) / client_camera.move_duration ) * ( cg.time - client_camera.move_time );
				}
			}
		}
	}
	else
	{
		checkTrack = qtrue;
	}

	if ( checkFollow )
	{
		if ( client_camera.info_state & CAMERA_FOLLOWING )
		{//This needs to be done after camera movement
			CGCam_FollowUpdate();
		}
		VectorCopy(client_camera.angles, cg.refdefViewAngles );
	}

	if ( checkTrack )
	{
		if ( client_camera.info_state & CAMERA_TRACKING )
		{//This has to run AFTER Follow if the camera is following a cameraGroup
			CGCam_TrackUpdate();
		}

		VectorCopy( client_camera.origin, cg.refdef.vieworg );
	}

	//Bar fading
	if ( client_camera.info_state & CAMERA_BAR_FADING )
	{
		CGCam_UpdateBarFade();
	}

	//Normal fading - separate call because can finish after camera is disabled
	CGCam_UpdateFade();

	//Update shaking if there's any
	//CGCam_UpdateSmooth( cg.refdef.vieworg, cg.refdefViewAngles );
	CGCam_UpdateShake( cg.refdef.vieworg, cg.refdefViewAngles );
	AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );
}

/*
-------------------------
CGCam_DrawWideScreen
-------------------------
*/

void CGCam_DrawWideScreen( void )
{
	vec4_t	modulate;

	//Only draw if visible
	if ( client_camera.bar_alpha )
	{
		CGCam_UpdateBarFade();

		modulate[0] = modulate[1] = modulate[2] = 0.0f;
		modulate[3] = client_camera.bar_alpha;

		CG_FillRect( cg.refdef.x, cg.refdef.y, 640, client_camera.bar_height, modulate  );
		CG_FillRect( cg.refdef.x, cg.refdef.y + 480 - client_camera.bar_height, 640, client_camera.bar_height, modulate  );
	}

	//NOTENOTE: Camera always draws the fades unless the alpha is 0
	if ( client_camera.fade_color[3] == 0.0f )
		return;

	CG_FillRect( cg.refdef.x, cg.refdef.y, 640, 480, client_camera.fade_color );
}

/*
-------------------------
CGCam_RenderScene
-------------------------
*/
void CGCam_RenderScene( void )
{
	CGCam_Update();
	CG_CalcVrect();
}

/*
-------------------------
CGCam_Shake
-------------------------
*/

void CGCam_Shake( float intensity, int duration )
{
	if ( intensity > MAX_SHAKE_INTENSITY )
		intensity = MAX_SHAKE_INTENSITY;

	client_camera.shake_intensity = intensity;
	client_camera.shake_duration = duration;
	client_camera.shake_start = cg.time;
}

/*
-------------------------
CGCam_UpdateShake

This doesn't actually affect the camera's info, but passed information instead
-------------------------
*/

void CGCam_UpdateShake( vec3_t origin, vec3_t angles )
{
	vec3_t	moveDir;
	float	intensity_scale, intensity;

	if ( client_camera.shake_duration <= 0 )
		return;

	if ( cg.time > ( client_camera.shake_start + client_camera.shake_duration ) )
	{
		client_camera.shake_intensity = 0;
		client_camera.shake_duration = 0;
		client_camera.shake_start = 0;
		return;
	}

	//intensity_scale now also takes into account FOV with 90.0 as normal
	intensity_scale = 1.0f - ( (float) ( cg.time - client_camera.shake_start ) / (float) client_camera.shake_duration ) * (((client_camera.FOV+client_camera.FOV2)/2.0f)/90.0f);

	intensity = client_camera.shake_intensity * intensity_scale;

	for ( int i = 0; i < 3; i++ )
	{
		moveDir[i] = ( Q_flrand(-1.0f, 1.0f) * intensity );
	}

	//FIXME: Lerp

	//Move the camera
	VectorAdd( origin, moveDir, origin );

	for ( int i = 0; i < 2; i++ ) // Don't do ROLL
		moveDir[i] = ( Q_flrand(-1.0f, 1.0f) * intensity );

	//FIXME: Lerp

	//Move the angles
	VectorAdd( angles, moveDir, angles );
}

void CGCam_Smooth( float intensity, int duration )
{
	client_camera.smooth_active=false; // means smooth_origin and angles are valid
	if ( intensity>1.0f||intensity==0.0f||duration<1)
	{
		client_camera.info_state &= ~CAMERA_SMOOTHING;
		return;
	}
	client_camera.info_state |= CAMERA_SMOOTHING;
	client_camera.smooth_intensity = intensity;
	client_camera.smooth_duration = duration;
	client_camera.smooth_start = cg.time;
}

void CGCam_UpdateSmooth( vec3_t origin, vec3_t angles )
{
	if (!(client_camera.info_state&CAMERA_SMOOTHING)||cg.time > ( client_camera.smooth_start + client_camera.smooth_duration ))
	{
		client_camera.info_state &= ~CAMERA_SMOOTHING;
		return;
	}
	if (!client_camera.smooth_active)
	{
		client_camera.smooth_active=true;
		VectorCopy(origin,client_camera.smooth_origin);
		return;
	}
	float factor=client_camera.smooth_intensity;
	if (client_camera.smooth_duration>200&&cg.time > ( client_camera.smooth_start + client_camera.smooth_duration-100 ))
	{
		factor+=(1.0f-client_camera.smooth_intensity)*
			(100.0f-(client_camera.smooth_start + client_camera.smooth_duration-cg.time))/100.0f;
	}
	int i;
	for (i=0;i<3;i++)
	{
		client_camera.smooth_origin[i]*=(1.0f-factor);
		client_camera.smooth_origin[i]+=factor*origin[i];
		origin[i]=client_camera.smooth_origin[i];
	}
}

void CGCam_NotetrackProcessFov(const char *addlArg)
{
	int a = 0;
	char t[64];

	if (!addlArg || !addlArg[0])
	{
		Com_Printf("camera roff 'fov' notetrack missing fov argument\n", addlArg);
		return;
	}
	if (isdigit(addlArg[a]))
	{
		// "fov <new fov>"
		int d = 0, tsize = 64;

		memset(t, 0, tsize*sizeof(char));
		while (addlArg[a] && d < tsize)
		{
			t[d++] = addlArg[a++];
		}
		// now the contents of t represent our desired fov
		float newFov = atof(t);
#ifndef FINAL_BUILD
		if (cg_roffdebug.integer)
		{
			if (fabs(cg_roffval1.value) > 0.001f)
			{
				newFov = cg_roffval1.value;
			}
		}
#endif
		if (cg_roffdebug.integer)
		{
			Com_Printf("notetrack: 'fov %2.2f' on frame %d\n", newFov, client_camera.roff_frame);
		}
		CGCam_Zoom(newFov, 0);
	}
}

void CGCam_NotetrackProcessFovZoom(const char *addlArg)
{
	int		a = 0;
	float	beginFOV = 0, endFOV = 0, fovTime = 0;

	if (!addlArg || !addlArg[0])
	{
		Com_Printf("camera roff 'fovzoom' notetrack missing arguments\n", addlArg);
		return;
	}
	//
	// "fovzoom <begin fov> <end fov> <time>"
	//
	char t[64];
	int d = 0, tsize = 64;

	memset(t, 0, tsize*sizeof(char));
	while (addlArg[a] && !isspace(addlArg[a]) && d < tsize)
	{
		t[d++] = addlArg[a++];
	}
	if (!isdigit(t[0]))
	{
		// assume a non-number here means we should start from our current fov
		beginFOV = client_camera.FOV;
	}
	else
	{
		// now the contents of t represent our beginning fov
		beginFOV = atof(t);
	}

	// eat leading whitespace
	while (addlArg[a] && addlArg[a] == ' ')
	{
		a++;
	}
	if (addlArg[a])
	{
		d = 0;
		memset(t, 0, tsize*sizeof(char));
		while (addlArg[a] && !isspace(addlArg[a]) && d < tsize)
		{
			t[d++] = addlArg[a++];
		}
		// now the contents of t represent our end fov
		endFOV = atof(t);

		// eat leading whitespace
		while (addlArg[a] && addlArg[a] == ' ')
		{
			a++;
		}
		if (addlArg[a])
		{
			d = 0;
			memset(t, 0, tsize*sizeof(char));
			while (addlArg[a] && !isspace(addlArg[a]) && d < tsize)
			{
				t[d++] = addlArg[a++];
			}
			// now the contents of t represent our time
			fovTime = atof(t);
		}
		else
		{
			Com_Printf("camera roff 'fovzoom' notetrack missing 'time' argument\n", addlArg);
			return;
		}
#ifndef FINAL_BUILD
		if (cg_roffdebug.integer)
		{
			if (fabs(cg_roffval1.value) > 0.001f)
			{
				beginFOV = cg_roffval1.value;
			}
			if (fabs(cg_roffval2.value) > 0.001f)
			{
				endFOV = cg_roffval2.value;
			}
			if (fabs(cg_roffval3.value) > 0.001f)
			{
				fovTime = cg_roffval3.value;
			}
		}
#endif
		if (cg_roffdebug.integer)
		{
			Com_Printf("notetrack: 'fovzoom %2.2f %2.2f %5.1f' on frame %d\n", beginFOV, endFOV, fovTime, client_camera.roff_frame);
		}
		CGCam_Zoom2(beginFOV, endFOV, fovTime);
	}
	else
	{
		Com_Printf("camera roff 'fovzoom' notetrack missing 'end fov' argument\n", addlArg);
		return;
	}
}

void CGCam_NotetrackProcessFovAccel(const char *addlArg)
{
	int		a = 0;
	float	beginFOV = 0, fovDelta = 0, fovDelta2 = 0, fovTime = 0;

	if (!addlArg || !addlArg[0])
	{
		Com_Printf("camera roff 'fovaccel' notetrack missing arguments\n", addlArg);
		return;
	}
	//
	// "fovaccel <begin fov> <fov delta> <fov delta2> <time>"
	//
	// where 'begin fov' is initial position, 'fov delta' is velocity, and 'fov delta2' is acceleration.
	char t[64];
	int d = 0, tsize = 64;

	memset(t, 0, tsize*sizeof(char));
	while (addlArg[a] && !isspace(addlArg[a]) && d < tsize)
	{
		t[d++] = addlArg[a++];
	}
	if (!isdigit(t[0]))
	{
		// assume a non-number here means we should start from our current fov
		beginFOV = client_camera.FOV;
	}
	else
	{
		// now the contents of t represent our beginning fov
		beginFOV = atof(t);
	}

	// eat leading whitespace
	while (addlArg[a] && addlArg[a] == ' ')
	{
		a++;
	}
	if (addlArg[a])
	{
		d = 0;
		memset(t, 0, tsize*sizeof(char));
		while (addlArg[a] && !isspace(addlArg[a]) && d < tsize)
		{
			t[d++] = addlArg[a++];
		}
		// now the contents of t represent our delta
		fovDelta = atof(t);

		// eat leading whitespace
		while (addlArg[a] && addlArg[a] == ' ')
		{
			a++;
		}
		if (addlArg[a])
		{
			d = 0;
			memset(t, 0, tsize*sizeof(char));
			while (addlArg[a] && !isspace(addlArg[a]) && d < tsize)
			{
				t[d++] = addlArg[a++];
			}
			// now the contents of t represent our fovDelta2
			fovDelta2 = atof(t);

			// eat leading whitespace
			while (addlArg[a] && addlArg[a] == ' ')
			{
				a++;
			}
			if (addlArg[a])
			{
				d = 0;
				memset(t, 0, tsize*sizeof(char));
				while (addlArg[a] && !isspace(addlArg[a]) && d < tsize)
				{
					t[d++] = addlArg[a++];
				}
				// now the contents of t represent our time
				fovTime = atof(t);
			}
			else
			{
				Com_Printf("camera roff 'fovaccel' notetrack missing 'time' argument\n", addlArg);
				return;
			}
			if (cg_roffdebug.integer)
			{
				Com_Printf("notetrack: 'fovaccel %2.2f %3.5f %3.5f %d' on frame %d\n", beginFOV, fovDelta, fovDelta2, fovTime, client_camera.roff_frame);
			}
			CGCam_ZoomAccel(beginFOV, fovDelta, fovDelta2, fovTime);
		}
		else
		{
			Com_Printf("camera roff 'fovaccel' notetrack missing 'delta2' argument\n", addlArg);
			return;
		}
	}
	else
	{
		Com_Printf("camera roff 'fovaccel' notetrack missing 'delta' argument\n", addlArg);
		return;
	}
}

// 3/18/03 kef -- blatantly thieved from G_RoffNotetrackCallback
static void CG_RoffNotetrackCallback(const char *notetrack)
{
	int i = 0, r = 0;
	char type[256];
//	char argument[512];
	char addlArg[512];
	int addlArgs = 0;

	if (!notetrack)
	{
		return;
	}

	//notetrack = "fov 65";

	while (notetrack[i] && notetrack[i] != ' ')
	{
		type[i] = notetrack[i];
		i++;
	}

	type[i] = '\0';

	//if (notetrack[i] != ' ')
	//{ //didn't pass in a valid notetrack type, or forgot the argument for it
	//	return;
	//}

/*	i++;

	while (notetrack[i] && notetrack[i] != ' ')
	{
		if (notetrack[i] != '\n' && notetrack[i] != '\r')
		{ //don't read line ends for an argument
			argument[r] = notetrack[i];
			r++;
		}
		i++;
	}
	argument[r] = '\0';
	if (!r)
	{
		return;
	}
*/

	if (notetrack[i] == ' ')
	{ //additional arguments...
		addlArgs = 1;

		i++;
		r = 0;
		while (notetrack[i])
		{
			addlArg[r] = notetrack[i];
			r++;
			i++;
		}
		addlArg[r] = '\0';
	}

	if (strcmp(type, "cut") == 0)
	{
		client_camera.info_state |= CAMERA_CUT;
		if (cg_roffdebug.integer)
		{
			Com_Printf("notetrack: 'cut' on frame %d\n", client_camera.roff_frame);
		}

		// this is just a really hacky way of getting a cut and a fov command on the same frame
		if (addlArgs)
		{
			CG_RoffNotetrackCallback(addlArg);
		}
	}
	else if (strcmp(type, "fov") == 0)
	{
		if (addlArgs)
		{
			CGCam_NotetrackProcessFov(addlArg);
			return;
		}
		Com_Printf("camera roff 'fov' notetrack missing fov argument\n", addlArg);
	}
	else if (strcmp(type, "fovzoom") == 0)
	{
		if (addlArgs)
		{
			CGCam_NotetrackProcessFovZoom(addlArg);
			return;
		}
		Com_Printf("camera roff 'fovzoom' notetrack missing 'begin fov' argument\n", addlArg);
	}
	else if (strcmp(type, "fovaccel") == 0)
	{
		if (addlArgs)
		{
			CGCam_NotetrackProcessFovAccel(addlArg);
			return;
		}
		Com_Printf("camera roff 'fovaccel' notetrack missing 'begin fov' argument\n", addlArg);
	}
}

/*
-------------------------
CGCam_StartRoff

Sets up the camera to use
a rof file
-------------------------
*/

void CGCam_StartRoff( char *roff )
{
	CGCam_FollowDisable();
	CGCam_TrackDisable();

	// Set up the roff state info..we'll hijack the moving and panning code until told otherwise
	//	...CAMERA_FOLLOWING would be a case that could override this..
	client_camera.info_state |= CAMERA_MOVING;
	client_camera.info_state |= CAMERA_PANNING;

	if ( !G_LoadRoff( roff ) )
	{
		// The load failed so don't turn on the roff playback...
		Com_Printf( S_COLOR_RED"ROFF camera playback failed\n" );
		return;
	};

	client_camera.info_state |= CAMERA_ROFFING;

	Q_strncpyz(client_camera.sRoff,roff,sizeof(client_camera.sRoff));
	client_camera.roff_frame = 0;
	client_camera.next_roff_time = cg.time;	// I can work right away
}

/*
-------------------------
CGCam_StopRoff

Stops camera rof
-------------------------
*/

static void CGCam_StopRoff( void )
{
	// Clear the roff flag
	client_camera.info_state &= ~CAMERA_ROFFING;
	client_camera.info_state &= ~CAMERA_MOVING;
}

/*
------------------------------------------------------
CGCam_Roff

Applies the sampled roff data to the camera and does
the lerping itself...this is done because the current
camera interpolation doesn't seem to work all that
great when you are adjusting the camera org and angles
so often...or maybe I'm just on crack.
------------------------------------------------------
*/

static void CGCam_Roff( void )
{
 while ( client_camera.next_roff_time <= cg.time )
 {
	// Make sure that the roff is cached
	const int roff_id = G_LoadRoff( client_camera.sRoff );

	if ( !roff_id )
	{
		return;
	}

	// The ID is one higher than the array index
	const roff_list_t	*roff	= &roffs[ roff_id - 1 ];
	vec3_t org, ang;

	if ( roff->type == 2 )
	{
		move_rotate2_t	*data	= &((move_rotate2_t *)roff->data)[ client_camera.roff_frame ];
		VectorCopy( data->origin_delta, org );
		VectorCopy( data->rotate_delta, ang );

		// since we just hit a new frame, clear our CUT flag
		client_camera.info_state &= ~CAMERA_CUT;

		if (data->mStartNote != -1 || data->mNumNotes)
		{
			CG_RoffNotetrackCallback(roffs[roff_id - 1].mNoteTrackIndexes[data->mStartNote]);
		}
	}
	else
	{
		move_rotate_t	*data	= &((move_rotate_t *)roff->data)[ client_camera.roff_frame ];
		VectorCopy( data->origin_delta, org );
		VectorCopy( data->rotate_delta, ang );
	}

	// Yeah, um, I guess this just has to be negated?
	//ang[PITCH]	=- ang[PITCH];
	ang[ROLL]	= -ang[ROLL];
	// might need to to yaw as well.  need a test...

	if ( cg_developer.integer )
	{
		Com_Printf( S_COLOR_GREEN"CamROFF: frame: %d o:<%.2f %.2f %.2f> a:<%.2f %.2f %.2f>\n",
					client_camera.roff_frame,
					org[0], org[1], org[2],
					ang[0], ang[1], ang[2] );
	}

	if ( client_camera.roff_frame )
	{
		// Don't mess with angles if we are following
		if ( !(client_camera.info_state & CAMERA_FOLLOWING) )
		{
			VectorAdd( client_camera.angles, client_camera.angles2, client_camera.angles );
		}

		VectorCopy( client_camera.origin2, client_camera.origin );
	}

	// Don't mess with angles if we are following
	if ( !(client_camera.info_state & CAMERA_FOLLOWING) )
	{
		VectorCopy( ang, client_camera.angles2 );
		client_camera.pan_time = cg.time;
		client_camera.pan_duration = roff->mFrameTime;
	}

	VectorAdd( client_camera.origin, org, client_camera.origin2 );

	client_camera.move_time = cg.time;
	client_camera.move_duration = roff->mFrameTime;

	if ( ++client_camera.roff_frame >= roff->frames )
	{
		CGCam_StopRoff();
		return;
	}

	// Check back in frameTime to get the next roff entry
	client_camera.next_roff_time += roff->mFrameTime;
 }
}

void CMD_CGCam_Disable( void )
{
	vec4_t	fade = {0, 0, 0, 0};

	CGCam_Disable();
	CGCam_SetFade( fade );
	player_locked = qfalse;
}
