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

// cg_view.c -- setup all the parameters (position, angle, etc)
// for a 3D rendering

// this line must stay at top so the whole PCH thing works...
#include "cg_headers.h"

#include "cg_media.h"
#include "FxScheduler.h"
#include "../game/wp_saber.h"
#include "../game/g_vehicles.h"

#define MASK_CAMERACLIP (MASK_SOLID)
#define CAMERA_SIZE	4

float cg_zoomFov;

//#define CG_CAM_ABOVE	2
extern qboolean CG_OnMovingPlat( playerState_t *ps );
extern Vehicle_t *G_IsRidingVehicle( gentity_t *ent );

extern int g_crosshairSameEntTime;
extern int g_crosshairEntNum;

/*
=============================================================================

  MODEL TESTING

The viewthing and gun positioning tools from Q2 have been integrated and
enhanced into a single model testing facility.

Model viewing can begin with either "testmodel <modelname>" or "testgun <modelname>".

The names must be the full pathname after the basedir, like
"models/weapons/v_launch/tris.md3" or "players/male/tris.md3"

Testmodel will create a fake entity 100 units in front of the current view
position, directly facing the viewer.  It will remain immobile, so you can
move around it to view it from different angles.

Testgun will cause the model to follow the player around and supress the real
view weapon model.  The default frame 0 of most guns is completely off screen,
so you will probably have to cycle a couple frames to see it.

"nextframe", "prevframe", "nextskin", and "prevskin" commands will change the
frame or skin of the testmodel.  These are bound to F5, F6, F7, and F8 in
q3default.cfg.

If a gun is being tested, the "gun_x", "gun_y", and "gun_z" variables will let
you adjust the positioning.

Note that none of the model testing features update while the game is paused, so
it may be convenient to test with deathmatch set to 1 so that bringing down the
console doesn't pause the game.

=============================================================================
*/

/*
Ghoul2 Insert Start
*/

/*
=================
CG_TestModel_f

Creates an entity in front of the current position, which
can then be moved around
=================
*/
void CG_TestG2Model_f (void) {
	vec3_t		angles;
	CGhoul2Info_v *ghoul2;

	memset( &cg.testModelEntity, 0, sizeof(cg.testModelEntity) );
	ghoul2 = new CGhoul2Info_v;
	cg.testModelEntity.ghoul2 = ghoul2;
	if ( cgi_Argc() < 2 ) {
		return;
	}

	Q_strncpyz (cg.testModelName, CG_Argv( 1 ), MAX_QPATH );
	cg.testModelEntity.hModel = cgi_R_RegisterModel( cg.testModelName );

	cg.testModel = gi.G2API_InitGhoul2Model(*((CGhoul2Info_v *)cg.testModelEntity.ghoul2), cg.testModelName, cg.testModelEntity.hModel, NULL_HANDLE, NULL_HANDLE,0,0);
	cg.testModelEntity.radius = 100.0f;

	if ( cgi_Argc() == 3 ) {
		cg.testModelEntity.backlerp = atof( CG_Argv( 2 ) );
		cg.testModelEntity.frame = 1;
		cg.testModelEntity.oldframe = 0;
	}
	if (! cg.testModelEntity.hModel ) {
		CG_Printf( "Can't register model\n" );
		return;
	}

	VectorMA( cg.refdef.vieworg, 100, cg.refdef.viewaxis[0], cg.testModelEntity.origin );

	angles[PITCH] = 0;
	angles[YAW] = 180 + cg.refdefViewAngles[1];
	angles[ROLL] = 0;

	AnglesToAxis( angles, cg.testModelEntity.axis );
}

void CG_ListModelSurfaces_f (void)
{
	CGhoul2Info_v	&ghoul2 = *((CGhoul2Info_v *)cg.testModelEntity.ghoul2);

 	gi.G2API_ListSurfaces(&ghoul2[cg.testModel]);
}


void CG_ListModelBones_f (void)
{
  	// test to see if we got enough args
	if ( cgi_Argc() < 2 )
	{
		return;
	}
	CGhoul2Info_v	&ghoul2 = *((CGhoul2Info_v *)cg.testModelEntity.ghoul2);

	gi.G2API_ListBones(&ghoul2[cg.testModel], atoi(CG_Argv(1)));
}

void CG_TestModelSurfaceOnOff_f(void)
{
	// test to see if we got enough args
	if ( cgi_Argc() < 3 )
	{
		return;
	}
	CGhoul2Info_v	&ghoul2 = *((CGhoul2Info_v *)cg.testModelEntity.ghoul2);

	gi.G2API_SetSurfaceOnOff(&ghoul2[cg.testModel], CG_Argv(1), atoi(CG_Argv(2)));
}

void CG_TestModelSetAnglespre_f(void)
{
	vec3_t	angles;

	if ( cgi_Argc() < 3 )
	{
		return;
	}
	CGhoul2Info_v	&ghoul2 = *((CGhoul2Info_v *)cg.testModelEntity.ghoul2);

	angles[0] = atof(CG_Argv(2));
	angles[1] = atof(CG_Argv(3));
	angles[2] = atof(CG_Argv(4));
	gi.G2API_SetBoneAngles(&ghoul2[cg.testModel], CG_Argv(1), angles, BONE_ANGLES_PREMULT, POSITIVE_X, POSITIVE_Z, POSITIVE_Y, NULL, 0, 0);
}

void CG_TestModelSetAnglespost_f(void)
{
	vec3_t	angles;

	if ( cgi_Argc() < 3 )
	{
		return;
	}
	CGhoul2Info_v	&ghoul2 = *((CGhoul2Info_v *)cg.testModelEntity.ghoul2);

	angles[0] = atof(CG_Argv(2));
	angles[1] = atof(CG_Argv(3));
	angles[2] = atof(CG_Argv(4));
	gi.G2API_SetBoneAngles(&ghoul2[cg.testModel], CG_Argv(1), angles, BONE_ANGLES_POSTMULT, POSITIVE_X, POSITIVE_Z, POSITIVE_Y, NULL, 0, 0);
}

void CG_TestModelAnimate_f(void)
{
	char	boneName[100];
	CGhoul2Info_v	&ghoul2 = *((CGhoul2Info_v *)cg.testModelEntity.ghoul2);

	strcpy(boneName, CG_Argv(1));
	gi.G2API_SetBoneAnim(&ghoul2[cg.testModel], boneName, atoi(CG_Argv(2)), atoi(CG_Argv(3)), BONE_ANIM_OVERRIDE_LOOP, atof(CG_Argv(4)), cg.time, -1, -1);

}

/*
Ghoul2 Insert End
*/


/*
=================
CG_TestModel_f

Creates an entity in front of the current position, which
can then be moved around
=================
*/
void CG_TestModel_f (void) {
	vec3_t		angles;

	memset( &cg.testModelEntity, 0, sizeof(cg.testModelEntity) );
	if ( cgi_Argc() < 2 ) {
		return;
	}

	Q_strncpyz (cg.testModelName, CG_Argv( 1 ), MAX_QPATH );
	cg.testModelEntity.hModel = cgi_R_RegisterModel( cg.testModelName );

	if ( cgi_Argc() == 3 ) {
		cg.testModelEntity.backlerp = atof( CG_Argv( 2 ) );
		cg.testModelEntity.frame = 1;
		cg.testModelEntity.oldframe = 0;
	}
	if (! cg.testModelEntity.hModel ) {
		CG_Printf( "Can't register model\n" );
		return;
	}

	VectorMA( cg.refdef.vieworg, 100, cg.refdef.viewaxis[0], cg.testModelEntity.origin );

	angles[PITCH] = 0;
	angles[YAW] = 180 + cg.refdefViewAngles[1];
	angles[ROLL] = 0;

	AnglesToAxis( angles, cg.testModelEntity.axis );
}


void CG_TestModelNextFrame_f (void) {
	cg.testModelEntity.frame++;
	CG_Printf( "frame %i\n", cg.testModelEntity.frame );
}

void CG_TestModelPrevFrame_f (void) {
	cg.testModelEntity.frame--;
	if ( cg.testModelEntity.frame < 0 ) {
		cg.testModelEntity.frame = 0;
	}
	CG_Printf( "frame %i\n", cg.testModelEntity.frame );
}

void CG_TestModelNextSkin_f (void) {
	cg.testModelEntity.skinNum++;
	CG_Printf( "skin %i\n", cg.testModelEntity.skinNum );
}

void CG_TestModelPrevSkin_f (void) {
	cg.testModelEntity.skinNum--;
	if ( cg.testModelEntity.skinNum < 0 ) {
		cg.testModelEntity.skinNum = 0;
	}
	CG_Printf( "skin %i\n", cg.testModelEntity.skinNum );
}

static void CG_AddTestModel (void) {
	// re-register the model, because the level may have changed
/*	cg.testModelEntity.hModel = cgi_R_RegisterModel( cg.testModelName );
	if (! cg.testModelEntity.hModel ) {
		CG_Printf ("Can't register model\n");
		return;
	}
*/
	cgi_R_AddRefEntityToScene( &cg.testModelEntity );
}



//============================================================================


/*
=================
CG_CalcVrect

Sets the coordinates of the rendered window
=================
*/
void CG_CalcVrect (void) {
	const int		size = 100;

	cg.refdef.width = cgs.glconfig.vidWidth * size * 0.01;
	cg.refdef.width &= ~1;

	cg.refdef.height = cgs.glconfig.vidHeight * size * 0.01;
	cg.refdef.height &= ~1;

	cg.refdef.x = (cgs.glconfig.vidWidth - cg.refdef.width) * 0.5;
	cg.refdef.y = (cgs.glconfig.vidHeight - cg.refdef.height) * 0.5;
}

//==============================================================================
//==============================================================================
#define CAMERA_DAMP_INTERVAL	50

#define CAMERA_CROUCH_NUDGE		6

static vec3_t	cameramins = { -CAMERA_SIZE, -CAMERA_SIZE, -CAMERA_SIZE };
static vec3_t	cameramaxs = { CAMERA_SIZE, CAMERA_SIZE, CAMERA_SIZE };
vec3_t	camerafwd, cameraup, camerahorizdir;

vec3_t	cameraFocusAngles,			cameraFocusLoc;
vec3_t	cameraIdealTarget,			cameraIdealLoc;
vec3_t	cameraCurTarget={0,0,0},	cameraCurLoc={0,0,0};
vec3_t	cameraOldLoc={0,0,0},		cameraNewLoc={0,0,0};
int		cameraLastFrame=0;

float	cameraLastYaw=0;
float	cameraStiffFactor=0.0f;

/*
===============
Notes on the camera viewpoint in and out...

cg.refdef.vieworg
--at the start of the function holds the player actor's origin (center of player model).
--it is set to the final view location of the camera at the end of the camera code.
cg.refdefViewAngles
--at the start holds the client's view angles
--it is set to the final view angle of the camera at the end of the camera code.

===============
*/

/*
===============
CG_CalcIdealThirdPersonViewTarget

===============
*/
static void CG_CalcIdealThirdPersonViewTarget(void)
{
	// Initialize IdealTarget
	qboolean usesViewEntity = (qboolean)(cg.snap->ps.viewEntity && cg.snap->ps.viewEntity < ENTITYNUM_WORLD);
	VectorCopy(cg.refdef.vieworg, cameraFocusLoc);

	if ( usesViewEntity )
	{

		gentity_t *gent = &g_entities[cg.snap->ps.viewEntity];
		if ( gent->client && (gent->client->NPC_class == CLASS_GONK
			|| gent->client->NPC_class == CLASS_INTERROGATOR
			|| gent->client->NPC_class == CLASS_SENTRY
			|| gent->client->NPC_class == CLASS_PROBE
			|| gent->client->NPC_class == CLASS_MOUSE
			|| gent->client->NPC_class == CLASS_R2D2
			|| gent->client->NPC_class == CLASS_R5D2) )
		{	// Droids use a generic offset
			cameraFocusLoc[2] += 4;
			VectorCopy( cameraFocusLoc, cameraIdealTarget );
			return;
		}

		if( gent->client->ps.pm_flags & PMF_DUCKED )
		{	// sort of a nasty hack in order to get this to work. Don't tell Ensiform, or I'll have to kill him. --eez
			cameraFocusLoc[2] -= CAMERA_CROUCH_NUDGE*4;
		}
	}

	// Add in the new viewheight
	cameraFocusLoc[2] += cg.predicted_player_state.viewheight;
	if ( cg.snap
		&& (cg.snap->ps.eFlags&EF_HELD_BY_SAND_CREATURE) )
	{
		VectorCopy( cameraFocusLoc, cameraIdealTarget );
		cameraIdealTarget[2] += 192;
	}
	else if ( cg.snap
		&& (cg.snap->ps.eFlags&EF_HELD_BY_WAMPA) )
	{
		VectorCopy( cameraFocusLoc, cameraIdealTarget );
		cameraIdealTarget[2] -= 48;
	}
	else if ( cg.overrides.active & CG_OVERRIDE_3RD_PERSON_VOF )
	{
		// Add in a vertical offset from the viewpoint, which puts the actual target above the head, regardless of angle.
		VectorCopy( cameraFocusLoc, cameraIdealTarget );
		cameraIdealTarget[2] += cg.overrides.thirdPersonVertOffset;
		//VectorMA(cameraFocusLoc, cg.overrides.thirdPersonVertOffset, cameraup, cameraIdealTarget);
	}
	else
	{
		// Add in a vertical offset from the viewpoint, which puts the actual target above the head, regardless of angle.
		VectorCopy( cameraFocusLoc, cameraIdealTarget );
		cameraIdealTarget[2] += cg_thirdPersonVertOffset.value;
		//VectorMA(cameraFocusLoc, cg_thirdPersonVertOffset.value, cameraup, cameraIdealTarget);
	}

	// Now, if the player is crouching, do a little special tweak.  The problem is that the player's head is way out of his bbox.
	if (cg.predicted_player_state.pm_flags & PMF_DUCKED)
	{ // Nudge to focus location up a tad.
		vec3_t nudgepos;
		trace_t trace;

		VectorCopy(cameraFocusLoc, nudgepos);
		nudgepos[2]+=CAMERA_CROUCH_NUDGE;
		CG_Trace(&trace, cameraFocusLoc, cameramins, cameramaxs, nudgepos,
			( usesViewEntity ) ? cg.snap->ps.viewEntity : cg.predicted_player_state.clientNum, MASK_CAMERACLIP);
		if (trace.fraction < 1.0)
		{
			VectorCopy(trace.endpos, cameraFocusLoc);
		}
		else
		{
			VectorCopy(nudgepos, cameraFocusLoc);
		}
	}
}



/*
===============
CG_CalcIdealThirdPersonViewLocation

===============
*/
static void CG_CalcIdealThirdPersonViewLocation(void)
{
	if ( cg.overrides.active & CG_OVERRIDE_3RD_PERSON_RNG )
	{
		VectorMA(cameraIdealTarget, -(cg.overrides.thirdPersonRange), camerafwd, cameraIdealLoc);
	}
	else if ( cg.snap
		&& (cg.snap->ps.eFlags&EF_HELD_BY_RANCOR)
		&& cg_entities[cg.snap->ps.clientNum].gent->activator )
	{//stay back
		VectorMA(cameraIdealTarget, -180.0f*cg_entities[cg.snap->ps.clientNum].gent->activator->s.modelScale[0], camerafwd, cameraIdealLoc);
	}
	else if ( cg.snap
		&& (cg.snap->ps.eFlags&EF_HELD_BY_WAMPA)
		&& cg_entities[cg.snap->ps.clientNum].gent->activator
		&& cg_entities[cg.snap->ps.clientNum].gent->activator->inuse )
	{//stay back
		VectorMA(cameraIdealTarget, -120.0f*cg_entities[cg.snap->ps.clientNum].gent->activator->s.modelScale[0], camerafwd, cameraIdealLoc);
	}
	else if ( cg.snap
		&& (cg.snap->ps.eFlags&EF_HELD_BY_SAND_CREATURE)
		&& cg_entities[cg.snap->ps.clientNum].gent->activator )
	{//stay back
		VectorMA(cg_entities[cg_entities[cg.snap->ps.clientNum].gent->activator->s.number].lerpOrigin, -180.0f, camerafwd, cameraIdealLoc);
	}
	else
	{
		VectorMA(cameraIdealTarget, -(cg_thirdPersonRange.value), camerafwd, cameraIdealLoc);
	}

	if ( cg.renderingThirdPerson && (cg.snap->ps.forcePowersActive&(1<<FP_SPEED)) && player->client->ps.forcePowerDuration[FP_SPEED] )
	{
		float timeLeft = player->client->ps.forcePowerDuration[FP_SPEED] - cg.time;
		float length = FORCE_SPEED_DURATION*forceSpeedValue[player->client->ps.forcePowerLevel[FP_SPEED]];
		float amt = forceSpeedRangeMod[player->client->ps.forcePowerLevel[FP_SPEED]];
		if ( timeLeft < 500 )
		{//start going back
			VectorMA(cameraIdealLoc, (timeLeft)/500*amt, camerafwd, cameraIdealLoc);
		}
		else if ( length - timeLeft < 1000 )
		{//start zooming in
			VectorMA(cameraIdealLoc, (length - timeLeft)/1000*amt, camerafwd, cameraIdealLoc);
		}
		else
		{
			VectorMA(cameraIdealLoc, amt, camerafwd, cameraIdealLoc);
		}
	}
}



static void CG_ResetThirdPersonViewDamp(void)
{
	trace_t trace;

	// Cap the pitch within reasonable limits
	if (cameraFocusAngles[PITCH] > 89.0)
	{
		cameraFocusAngles[PITCH] = 89.0;
	}
	else if (cameraFocusAngles[PITCH] < -89.0)
	{
		cameraFocusAngles[PITCH] = -89.0;
	}

	AngleVectors(cameraFocusAngles, camerafwd, NULL, cameraup);

	// Set the cameraIdealTarget
	CG_CalcIdealThirdPersonViewTarget();

	// Set the cameraIdealLoc
	CG_CalcIdealThirdPersonViewLocation();

	// Now, we just set everything to the new positions.
	VectorCopy(cameraIdealLoc, cameraCurLoc);
	VectorCopy(cameraIdealTarget, cameraCurTarget);

	// First thing we do is trace from the first person viewpoint out to the new target location.
	CG_Trace(&trace, cameraFocusLoc, cameramins, cameramaxs, cameraCurTarget, cg.predicted_player_state.clientNum, MASK_CAMERACLIP);
	if (trace.fraction <= 1.0)
	{
		VectorCopy(trace.endpos, cameraCurTarget);
	}

	// Now we trace from the new target location to the new view location, to make sure there is nothing in the way.
	CG_Trace(&trace, cameraCurTarget, cameramins, cameramaxs, cameraCurLoc, cg.predicted_player_state.clientNum, MASK_CAMERACLIP);
	if (trace.fraction <= 1.0)
	{
		VectorCopy(trace.endpos, cameraCurLoc);
	}

	cameraLastFrame = cg.time;
	cameraLastYaw = cameraFocusAngles[YAW];
	cameraStiffFactor = 0.0f;
}

// This is called every frame.
static void CG_UpdateThirdPersonTargetDamp(void)
{
	trace_t trace;
	vec3_t	targetdiff;
	float	dampfactor, dtime, ratio;

	// Set the cameraIdealTarget
	// Automatically get the ideal target, to avoid jittering.
	CG_CalcIdealThirdPersonViewTarget();

	if ( CG_OnMovingPlat( &cg.snap->ps ) )
	{//if moving on a plat, camera is *tight*
		VectorCopy(cameraIdealTarget, cameraCurTarget);
	}
	else if (cg_thirdPersonTargetDamp.value>=1.0)//||cg.thisFrameTeleport)
	{	// No damping.
		VectorCopy(cameraIdealTarget, cameraCurTarget);
	}
	else if (cg_thirdPersonTargetDamp.value>=0.0)
	{
		// Calculate the difference from the current position to the new one.
		VectorSubtract(cameraIdealTarget, cameraCurTarget, targetdiff);

		// Now we calculate how much of the difference we cover in the time allotted.
		// The equation is (Damp)^(time)
		dampfactor = 1.0-cg_thirdPersonTargetDamp.value;	// We must exponent the amount LEFT rather than the amount bled off
		dtime = (float)(cg.time-cameraLastFrame) * (1.0/cg_timescale.value) * (1.0/(float)CAMERA_DAMP_INTERVAL);	// Our dampfactor is geared towards a time interval equal to "1".

		// Note that since there are a finite number of "practical" delta millisecond values possible,
		// the ratio should be initialized into a chart ultimately.
		if ( cg_smoothCamera.integer )
			ratio = powf( dampfactor, dtime );
		else
			ratio = Q_powf( dampfactor, dtime );

		// This value is how much distance is "left" from the ideal.
		VectorMA(cameraIdealTarget, -ratio, targetdiff, cameraCurTarget);
		/////////////////////////////////////////////////////////////////////////////////////////////////////////
	}

	// Now we trace to see if the new location is cool or not.

	// First thing we do is trace from the first person viewpoint out to the new target location.
	if ( cg.snap
		&& (cg.snap->ps.eFlags&EF_HELD_BY_SAND_CREATURE)
		&& cg_entities[cg.snap->ps.clientNum].gent->activator )
	{//if being held by a sand creature, trace from his actual origin, since we could be underground or otherwise in solid once he eats us
		CG_Trace(&trace, cg_entities[cg_entities[cg.snap->ps.clientNum].gent->activator->s.number].lerpOrigin, cameramins, cameramaxs, cameraCurTarget, cg.predicted_player_state.clientNum, MASK_CAMERACLIP);
	}
	else
	{
		CG_Trace(&trace, cameraFocusLoc, cameramins, cameramaxs, cameraCurTarget, cg.predicted_player_state.clientNum, MASK_CAMERACLIP);
	}
	if (trace.fraction < 1.0)
	{
		VectorCopy(trace.endpos, cameraCurTarget);
	}

	// Note that previously there was an upper limit to the number of physics traces that are done through the world
	// for the sake of camera collision, since it wasn't calced per frame.  Now it is calculated every frame.
	// This has the benefit that the camera is a lot smoother now (before it lerped between tested points),
	// however two full volume traces each frame is a bit scary to think about.
}

// This can be called every interval, at the user's discretion.
static int camWaterAdjust = 0;
static void CG_UpdateThirdPersonCameraDamp(void)
{
	trace_t trace;
	vec3_t	locdiff;
	float dampfactor, dtime, ratio;

	// Set the cameraIdealLoc
	CG_CalcIdealThirdPersonViewLocation();


	// First thing we do is calculate the appropriate damping factor for the camera.
	dampfactor=0.0f;
	if ( CG_OnMovingPlat( &cg.snap->ps ) )
	{//if moving on a plat, camera is *tight*
		dampfactor=1.0f;
	}
	else if ( cg.overrides.active & CG_OVERRIDE_3RD_PERSON_CDP )
	{
		if ( cg.overrides.thirdPersonCameraDamp != 0.0f )
		{
			float pitch;

			// Note that the camera pitch has already been capped off to 89.
			pitch = Q_fabs(cameraFocusAngles[PITCH]);

			// The higher the pitch, the larger the factor, so as you look up, it damps a lot less.
			pitch /=115.0f;
			dampfactor = (1.0-cg.overrides.thirdPersonCameraDamp)*(pitch*pitch);

			dampfactor += cg.overrides.thirdPersonCameraDamp;
		}
	}
	else if ( cg_thirdPersonCameraDamp.value != 0.0f )
	{
		float pitch;

		// Note that the camera pitch has already been capped off to 89.
		pitch = Q_fabs(cameraFocusAngles[PITCH]);

		// The higher the pitch, the larger the factor, so as you look up, it damps a lot less.
		pitch /= 115.0f;
		dampfactor = (1.0-cg_thirdPersonCameraDamp.value)*(pitch*pitch);

		dampfactor += cg_thirdPersonCameraDamp.value;

		// Now we also multiply in the stiff factor, so that faster yaw changes are stiffer.
		if (cameraStiffFactor > 0.0f)
		{	// The cameraStiffFactor is how much of the remaining damp below 1 should be shaved off, i.e. approach 1 as stiffening increases.
			dampfactor += (1.0-dampfactor)*cameraStiffFactor;
		}
	}

	if (dampfactor>=1.0)//||cg.thisFrameTeleport)
	{	// No damping.
		VectorCopy(cameraIdealLoc, cameraCurLoc);
	}
	else if (dampfactor>=0.0)
	{
		// Calculate the difference from the current position to the new one.
		VectorSubtract(cameraIdealLoc, cameraCurLoc, locdiff);

		// Now we calculate how much of the difference we cover in the time allotted.
		// The equation is (Damp)^(time)
		dampfactor = 1.0-dampfactor;	// We must exponent the amount LEFT rather than the amount bled off
		dtime = (float)(cg.time-cameraLastFrame) * (1.0/cg_timescale.value) * (1.0/(float)CAMERA_DAMP_INTERVAL);	// Our dampfactor is geared towards a time interval equal to "1".

		// Note that since there are a finite number of "practical" delta millisecond values possible,
		// the ratio should be initialized into a chart ultimately.
		if ( cg_smoothCamera.integer )
			ratio = powf( dampfactor, dtime );
		else
			ratio = Q_powf( dampfactor, dtime );

		// This value is how much distance is "left" from the ideal.
		VectorMA(cameraIdealLoc, -ratio, locdiff, cameraCurLoc);
		/////////////////////////////////////////////////////////////////////////////////////////////////////////
	}

	// Now we trace from the first person viewpoint to the new view location, to make sure there is nothing in the way between the user and the camera...
//	CG_Trace(&trace, cameraFocusLoc, cameramins, cameramaxs, cameraCurLoc, cg.predicted_player_state.clientNum, MASK_CAMERACLIP);
	// (OLD) Now we trace from the new target location to the new view location, to make sure there is nothing in the way.
	if ( cg.snap
		&& (cg.snap->ps.eFlags&EF_HELD_BY_SAND_CREATURE)
		&& cg_entities[cg.snap->ps.clientNum].gent->activator )
	{//if being held by a sand creature, trace from his actual origin, since we could be underground or otherwise in solid once he eats us
		CG_Trace( &trace, cg_entities[cg_entities[cg.snap->ps.clientNum].gent->activator->s.number].lerpOrigin, cameramins, cameramaxs, cameraCurLoc, cg.predicted_player_state.clientNum, MASK_CAMERACLIP);
	}
	else
	{
		CG_Trace( &trace, cameraCurTarget, cameramins, cameramaxs, cameraCurLoc, cg.predicted_player_state.clientNum, MASK_CAMERACLIP);
	}
	if ( trace.fraction < 1.0f )
	{
		VectorCopy( trace.endpos, cameraCurLoc );

		// We didn't trace all the way back, so push down the target accordingly.
//		VectorSubtract(cameraCurTarget, cameraFocusLoc, locdiff);
//		VectorMA(cameraFocusLoc, trace.fraction, locdiff, cameraCurTarget);

		//FIXME: when the trace hits movers, it gets very very jaggy... ?
		/*
		//this doesn't actually help any
		if ( trace.entityNum != ENTITYNUM_WORLD )
		{
			centity_t *cent = &cg_entities[trace.entityNum];
			gentity_t *gent = &g_entities[trace.entityNum];
			if ( cent != NULL && gent != NULL )
			{
				if ( cent->currentState.pos.trType == TR_LINEAR || cent->currentState.pos.trType == TR_LINEAR_STOP )
				{
					vec3_t	diff;
					VectorSubtract( cent->lerpOrigin, gent->currentOrigin, diff );
					VectorAdd( cameraCurLoc, diff, cameraCurLoc );
				}
			}
		}
		*/
	}

	// Note that previously there was an upper limit to the number of physics traces that are done through the world
	// for the sake of camera collision, since it wasn't calced per frame.  Now it is calculated every frame.
	// This has the benefit that the camera is a lot smoother now (before it lerped between tested points),
	// however two full volume traces each frame is a bit scary to think about.
}




/*
===============
CG_OffsetThirdPersonView

===============
*/
extern qboolean	MatrixMode;
static void CG_OffsetThirdPersonView( void )
{
	vec3_t diff;
	float deltayaw;

	camWaterAdjust = 0;
	cameraStiffFactor = 0.0;

	// Set camera viewing direction.
	VectorCopy( cg.refdefViewAngles, cameraFocusAngles );

	if ( cg.snap
		&& (cg.snap->ps.eFlags&EF_HELD_BY_RANCOR)
		&& cg_entities[cg.snap->ps.clientNum].gent->activator )
	{
		centity_t	*monster = &cg_entities[cg_entities[cg.snap->ps.clientNum].gent->activator->s.number];
		VectorSet( cameraFocusAngles, 0, AngleNormalize180(monster->lerpAngles[YAW]+180), 0 );
	}
	else if ( cg.snap && (cg.snap->ps.eFlags&EF_HELD_BY_SAND_CREATURE) )
	{
		centity_t	*monster = &cg_entities[cg_entities[cg.snap->ps.clientNum].gent->activator->s.number];
		VectorSet( cameraFocusAngles, 0, AngleNormalize180(monster->lerpAngles[YAW]+180), 0 );
		cameraFocusAngles[PITCH] = 0.0f;//flatten it out
	}
	else if ( G_IsRidingVehicle( &g_entities[0] ) )
	{
		cameraFocusAngles[YAW] = cg_entities[g_entities[0].owner->s.number].lerpAngles[YAW];
		if ( cg.overrides.active & CG_OVERRIDE_3RD_PERSON_ANG )
		{
			cameraFocusAngles[YAW] += cg.overrides.thirdPersonAngle;
		}
		else
		{
			cameraFocusAngles[YAW] += cg_thirdPersonAngle.value;
		}
	}
	else if ( cg.predicted_player_state.stats[STAT_HEALTH] <= 0 )
	{// if dead, look at killer
		if ( MatrixMode )
		{
			if ( cg.overrides.active & CG_OVERRIDE_3RD_PERSON_ANG )
			{
				cameraFocusAngles[YAW] += cg.overrides.thirdPersonAngle;
			}
			else
			{
				cameraFocusAngles[YAW] = cg.predicted_player_state.stats[STAT_DEAD_YAW];
				cameraFocusAngles[YAW] += cg_thirdPersonAngle.value;
			}
		}
		else
		{
			cameraFocusAngles[YAW] = cg.predicted_player_state.stats[STAT_DEAD_YAW];
		}
	}
	else
	{	// Add in the third Person Angle.
		if ( cg.overrides.active & CG_OVERRIDE_3RD_PERSON_ANG )
		{
			cameraFocusAngles[YAW] += cg.overrides.thirdPersonAngle;
		}
		else
		{
			cameraFocusAngles[YAW] += cg_thirdPersonAngle.value;
		}
		if ( cg.overrides.active & CG_OVERRIDE_3RD_PERSON_POF )
		{
			cameraFocusAngles[PITCH] += cg.overrides.thirdPersonPitchOffset;
		}
		else
		{
			cameraFocusAngles[PITCH] += cg_thirdPersonPitchOffset.value;
		}
	}

	if ( !cg.renderingThirdPerson && (cg.snap->ps.weapon == WP_SABER||cg.snap->ps.weapon == WP_MELEE) )
	{// First person saber
		// FIXME: use something network-friendly
		vec3_t	org, viewDir;
		VectorCopy( cg_entities[0].gent->client->renderInfo.eyePoint, org );
		float blend = 1.0f - fabs(cg.refdefViewAngles[PITCH])/90.0f;
		AngleVectors( cg.refdefViewAngles, viewDir, NULL, NULL );
		VectorMA( org, -8, viewDir, org );
		VectorScale( org, 1.0f - blend, org );
		VectorMA( org, blend, cg.refdef.vieworg, cg.refdef.vieworg );
		return;
	}
	// The next thing to do is to see if we need to calculate a new camera target location.

	// If we went back in time for some reason, or if we just started, reset the sample.
	if (cameraLastFrame == 0 || cameraLastFrame > cg.time)
	{
		CG_ResetThirdPersonViewDamp();
	}
	else
	{
		// Cap the pitch within reasonable limits
		if (cameraFocusAngles[PITCH] > 89.0)
		{
			cameraFocusAngles[PITCH] = 89.0;
		}
		else if (cameraFocusAngles[PITCH] < -89.0)
		{
			cameraFocusAngles[PITCH] = -89.0;
		}

		AngleVectors(cameraFocusAngles, camerafwd, NULL, cameraup);

		deltayaw = fabs(cameraFocusAngles[YAW] - cameraLastYaw);
		if (deltayaw > 180.0f)
		{ // Normalize this angle so that it is between 0 and 180.
			deltayaw = fabs(deltayaw - 360.0f);
		}
		cameraStiffFactor = deltayaw / (float)(cg.time-cameraLastFrame);
		if (cameraStiffFactor < 1.0)
		{
			cameraStiffFactor = 0.0;
		}
		else if (cameraStiffFactor > 2.5)
		{
			cameraStiffFactor = 0.75;
		}
		else
		{	// 1 to 2 scales from 0.0 to 0.5
			cameraStiffFactor = (cameraStiffFactor-1.0f)*0.5f;
		}
		cameraLastYaw = cameraFocusAngles[YAW];

		// Move the target to the new location.
		CG_UpdateThirdPersonTargetDamp();
		CG_UpdateThirdPersonCameraDamp();
	}

	// Now interestingly, the Quake method is to calculate a target focus point above the player, and point the camera at it.
	// We won't do that for now.

	// We must now take the angle taken from the camera target and location.
	VectorSubtract(cameraCurTarget, cameraCurLoc, diff);
	//Com_Printf( "%s\n", vtos(diff) );
	float dist = VectorNormalize(diff);
	if ( dist < 1.0f )
	{//must be hitting something, need some value to calc angles, so use cam forward
		VectorCopy( camerafwd, diff );
	}
	vectoangles(diff, cg.refdefViewAngles);

	// Temp: just move the camera to the side a bit
	extern vmCvar_t cg_thirdPersonHorzOffset;
	if ( cg_thirdPersonHorzOffset.value != 0.0f )
	{
		AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );
		VectorMA( cameraCurLoc, cg_thirdPersonHorzOffset.value, cg.refdef.viewaxis[1], cameraCurLoc );
	}

	// ...and of course we should copy the new view location to the proper spot too.
	VectorCopy(cameraCurLoc, cg.refdef.vieworg);

	//if we hit the water, do a last-minute adjustment
	if ( camWaterAdjust )
	{
		cg.refdef.vieworg[2] += camWaterAdjust;
	}
	cameraLastFrame=cg.time;
}



/*
===============
CG_OffsetThirdPersonView

===============
*/
/*
#define	FOCUS_DISTANCE	512
static void CG_OffsetThirdPersonView( void ) {
	vec3_t		forward, right, up;
	vec3_t		view;
	vec3_t		focusAngles;
	trace_t		trace;
	static vec3_t	mins = { -4, -4, -4 };
	static vec3_t	maxs = { 4, 4, 4 };
	vec3_t		focusPoint;
	float		focusDist;
	float		forwardScale, sideScale;

	cg.refdef.vieworg[2] += cg.predicted_player_state.viewheight;

	VectorCopy( cg.refdefViewAngles, focusAngles );

	// if dead, look at killer
	if ( cg.predicted_player_state.stats[STAT_HEALTH] <= 0 ) {
		focusAngles[YAW] = cg.predicted_player_state.stats[STAT_DEAD_YAW];
		cg.refdefViewAngles[YAW] = cg.predicted_player_state.stats[STAT_DEAD_YAW];
	}

	if ( focusAngles[PITCH] > 45 ) {
		focusAngles[PITCH] = 45;		// don't go too far overhead
	}
	AngleVectors( focusAngles, forward, NULL, NULL );

	VectorMA( cg.refdef.vieworg, FOCUS_DISTANCE, forward, focusPoint );

	VectorCopy( cg.refdef.vieworg, view );

	view[2] += 8;

	cg.refdefViewAngles[PITCH] *= 0.5;

	AngleVectors( cg.refdefViewAngles, forward, right, up );

	float tpAngle = cg.overrides.thirdPersonAngle ? cg.overrides.thirdPersonAngle : cg_thirdPersonAngle.value;
	forwardScale = cos( tpAngle / 180 * M_PI );
	sideScale = sin( tpAngle / 180 * M_PI );
	VectorMA( view, -tpAngle * forwardScale, forward, view );
	VectorMA( view, -tpAngle * sideScale, right, view );

	// trace a ray from the origin to the viewpoint to make sure the view isn't
	// in a solid block.  Use an 8 by 8 block to prevent the view from near clipping anything

	CG_Trace( &trace, cg.refdef.vieworg, mins, maxs, view, cg.predicted_player_state.clientNum, MASK_CAMERACLIP );

	if ( trace.fraction != 1.0 ) {
		VectorCopy( trace.endpos, view );
		view[2] += (1.0 - trace.fraction) * 32;
		// try another trace to this position, because a tunnel may have the ceiling
		// close enogh that this is poking out

		CG_Trace( &trace, cg.refdef.vieworg, mins, maxs, view, cg.predicted_player_state.clientNum, MASK_CAMERACLIP );
		VectorCopy( trace.endpos, view );
	}


	VectorCopy( view, cg.refdef.vieworg );

	// select pitch to look at focus point from vieword
	VectorSubtract( focusPoint, cg.refdef.vieworg, focusPoint );
	focusDist = sqrt( focusPoint[0] * focusPoint[0] + focusPoint[1] * focusPoint[1] );
	if ( focusDist < 1 ) {
		focusDist = 1;	// should never happen
	}
	cg.refdefViewAngles[PITCH] = -180 / M_PI * atan2( focusPoint[2], focusDist );
	cg.refdefViewAngles[YAW] -= tpAngle;
}


#define MIN_CAMERA_HEIGHT	75
#define MIN_CAMERA_PITCH	40
#define MAX_CAMERA_PITCH	90

//----------------------------------------------
static void CG_OffsetThirdPersonOverheadView( void ) {
	vec3_t		view, angs;
	trace_t		trace;
	static vec3_t	mins = { -4, -4, -4 };
	static vec3_t	maxs = { 4, 4, 4 };

	VectorCopy( cg.refdef.vieworg, view );

	// Move straight up from the player, making sure to always go at least the min camera height,
	//	otherwise, the camera will clip into the head of the player.
	float tpRange = cg.overrides.thirdPersonRange ? cg.overrides.thirdPersonRange : cg_thirdPersonRange.value;
	if ( tpRange < MIN_CAMERA_HEIGHT )
	{
		view[2] += MIN_CAMERA_HEIGHT;
	}
	else
	{
		view[2] += tpRange;
	}

	// Now adjust the camera angles, but we shouldn't adjust the viewAngles...only the viewAxis
	VectorCopy( cg.refdefViewAngles, angs );
	angs[PITCH] = cg.overrides.thirdPersonAngle ? cg.overrides.thirdPersonAngle : cg_thirdPersonAngle.value;

	// Simple clamp to weed out any really obviously nasty angles
	if ( angs[PITCH] < MIN_CAMERA_PITCH )
	{
		angs[PITCH] = MIN_CAMERA_PITCH;
	}
	else if ( angs[PITCH] > MAX_CAMERA_PITCH )
	{
		angs[PITCH] = MAX_CAMERA_PITCH;
	}

	// Convert our new desired camera angles and store them where they will get used by the engine
	//	when setting up the actual camera view.
	AnglesToAxis( angs, cg.refdef.viewaxis );
	cg.refdefViewAngles[PITCH] = 0;
	g_entities[0].client->ps.delta_angles[PITCH] = 0;

	// Trace a ray from the origin to the viewpoint to make sure the view isn't
	//	in a solid block.
	CG_Trace( &trace, cg.refdef.vieworg, mins, maxs, view, cg.predicted_player_state.clientNum, MASK_CAMERACLIP);

	if ( trace.fraction != 1.0 )
	{
		VectorCopy( trace.endpos, cg.refdef.vieworg );
	}
	else
	{
		VectorCopy( view, cg.refdef.vieworg );
	}
}
*/
// this causes a compiler bug on mac MrC compiler
static void CG_StepOffset( void ) {
	int		timeDelta;

	// smooth out stair climbing
	timeDelta = cg.time - cg.stepTime;
	if ( timeDelta < STEP_TIME ) {
		cg.refdef.vieworg[2] -= cg.stepChange
			* (STEP_TIME - timeDelta) / STEP_TIME;
	}
}

/*
===============
CG_OffsetFirstPersonView

===============
*/
extern qboolean PM_InForceGetUp( playerState_t *ps );
extern qboolean PM_InGetUp( playerState_t *ps );
extern qboolean PM_InKnockDown( playerState_t *ps );
extern int PM_AnimLength( int index, animNumber_t anim );
static void CG_OffsetFirstPersonView( qboolean firstPersonSaber ) {
	float			*origin;
	float			*angles;
	float			bob;
	float			ratio;
	float			delta;
	float			speed;
	float			f;
	vec3_t			predictedVelocity;
	int				timeDelta;

	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		return;
	}

	origin = cg.refdef.vieworg;
	angles = cg.refdefViewAngles;

	// if dead, fix the angle and don't add any kick
	if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 )
	{
		angles[ROLL] = 40;
		angles[PITCH] = -15;
		angles[YAW] = cg.snap->ps.stats[STAT_DEAD_YAW];
		origin[2] += cg.predicted_player_state.viewheight;
		return;
	}

	if ( g_entities[0].client && PM_InKnockDown( &g_entities[0].client->ps ) )
	{
		float perc, animLen = (float)PM_AnimLength( g_entities[0].client->clientInfo.animFileIndex, (animNumber_t)g_entities[0].client->ps.legsAnim );
		if ( PM_InGetUp( &g_entities[0].client->ps ) || PM_InForceGetUp( &g_entities[0].client->ps ) )
		{//start righting the view
			perc = (float)g_entities[0].client->ps.legsAnimTimer/animLen*2;
		}
		else
		{//tilt the view
			perc = (animLen-g_entities[0].client->ps.legsAnimTimer)/animLen*2;
		}
		if ( perc > 1.0f )
		{
			perc = 1.0f;
		}
		angles[ROLL] = perc*40;
		angles[PITCH] = perc*-15;
	}

	// add angles based on weapon kick
	int kickTime = (cg.time - cg.kick_time);
	if ( kickTime < 800 )
	{//kicks are always 1 second long.  Deal with it.
		float kickPerc = 0.0f;
		if ( kickTime <= 200 )
		{//winding up
			kickPerc = kickTime/200.0f;
		}
		else
		{//returning to normal
			kickTime = 800 - kickTime;
			kickPerc = kickTime/600.0f;
		}
		VectorMA( angles, kickPerc, cg.kick_angles, angles );
	}

	// add angles based on damage kick
	if ( cg.damageTime ) {
		ratio = cg.time - cg.damageTime;
		if ( ratio < DAMAGE_DEFLECT_TIME ) {
			ratio /= DAMAGE_DEFLECT_TIME;
			angles[PITCH] += ratio * cg.v_dmg_pitch;
			angles[ROLL] += ratio * cg.v_dmg_roll;
		} else {
			ratio = 1.0 - ( ratio - DAMAGE_DEFLECT_TIME ) / DAMAGE_RETURN_TIME;
			if ( ratio > 0 ) {
				angles[PITCH] += ratio * cg.v_dmg_pitch;
				angles[ROLL] += ratio * cg.v_dmg_roll;
			}
		}
	}

	// add pitch based on fall kick
#if 0
	ratio = ( cg.time - cg.landTime) / FALL_TIME;
	if (ratio < 0)
		ratio = 0;
	angles[PITCH] += ratio * cg.fall_value;
#endif

	// add angles based on velocity
	VectorCopy( cg.predicted_player_state.velocity, predictedVelocity );

	delta = DotProduct ( predictedVelocity, cg.refdef.viewaxis[0]);
	angles[PITCH] += delta * cg_runpitch.value;

	delta = DotProduct ( predictedVelocity, cg.refdef.viewaxis[1]);
	angles[ROLL] -= delta * cg_runroll.value;

	// add angles based on bob

	// make sure the bob is visible even at low speeds
	speed = cg.xyspeed > 200 ? cg.xyspeed : 200;

	delta = cg.bobfracsin * cg_bobpitch.value * speed;
	if (cg.predicted_player_state.pm_flags & PMF_DUCKED)
		delta *= 3;		// crouching
	angles[PITCH] += delta;
	delta = cg.bobfracsin * cg_bobroll.value * speed;
	if (cg.predicted_player_state.pm_flags & PMF_DUCKED)
		delta *= 3;		// crouching accentuates roll
	if (cg.bobcycle & 1)
		delta = -delta;
	angles[ROLL] += delta;

//===================================

	if ( !firstPersonSaber )//First person saber
	{
		// add view height
		if ( cg.snap->ps.viewEntity > 0 && cg.snap->ps.viewEntity < ENTITYNUM_WORLD )
		{
			if ( &g_entities[cg.snap->ps.viewEntity] &&
				g_entities[cg.snap->ps.viewEntity].client &&
				g_entities[cg.snap->ps.viewEntity].client->ps.viewheight )
			{
				origin[2] += g_entities[cg.snap->ps.viewEntity].client->ps.viewheight;
			}
			else
			{
				origin[2] += 4;//???
			}
		}
		else
		{
			origin[2] += cg.predicted_player_state.viewheight;
		}
	}

	// smooth out duck height changes
	timeDelta = cg.time - cg.duckTime;
	if ( timeDelta < DUCK_TIME) {
		cg.refdef.vieworg[2] -= cg.duckChange * (DUCK_TIME - timeDelta) / DUCK_TIME;
	}

	// add bob height
	bob = cg.bobfracsin * cg.xyspeed * cg_bobup.value;
	if (bob > 6) {
		bob = 6;
	}

	origin[2] += bob;


	// add fall height
	delta = cg.time - cg.landTime;
	if ( delta < LAND_DEFLECT_TIME ) {
		f = delta / LAND_DEFLECT_TIME;
		cg.refdef.vieworg[2] += cg.landChange * f;
	} else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME ) {
		delta -= LAND_DEFLECT_TIME;
		f = 1.0 - ( delta / LAND_RETURN_TIME );
		cg.refdef.vieworg[2] += cg.landChange * f;
	}

	// add step offset
	CG_StepOffset();

	if(cg.snap->ps.leanofs != 0)
	{
		vec3_t	right;
		//add leaning offset
		//FIXME: when crouching, this bounces up and down?!
		cg.refdefViewAngles[2] += (float)cg.snap->ps.leanofs/2;
		AngleVectors(cg.refdefViewAngles, NULL, right, NULL);
		VectorMA(cg.refdef.vieworg, (float)cg.snap->ps.leanofs, right, cg.refdef.vieworg);
	}

	// pivot the eye based on a neck length
#if 0
	{
#define	NECK_LENGTH		8
	vec3_t			forward, up;

	cg.refdef.vieworg[2] -= NECK_LENGTH;
	AngleVectors( cg.refdefViewAngles, forward, NULL, up );
	VectorMA( cg.refdef.vieworg, 3, forward, cg.refdef.vieworg );
	VectorMA( cg.refdef.vieworg, NECK_LENGTH, up, cg.refdef.vieworg );
	}
#endif
}


/*
====================
CG_CalcFovFromX

Calcs Y FOV from given X FOV
====================
*/
qboolean CG_CalcFOVFromX( float fov_x )
{
	float	x;
	float	fov_y;
	qboolean	inwater;

	if ( cg_fovAspectAdjust.integer ) {
		// Based on LordHavoc's code for Darkplaces
		// http://www.quakeworld.nu/forum/topic/53/what-does-your-qw-look-like/page/30
		const float baseAspect = 0.75f; // 3/4
		const float aspect = (float)cgs.glconfig.vidWidth/(float)cgs.glconfig.vidHeight;
		const float desiredFov = fov_x;

		fov_x = atan( tan( desiredFov*M_PI / 360.0f ) * baseAspect*aspect )*360.0f / M_PI;
	}

	x = cg.refdef.width / tan( fov_x / 360 * M_PI );
	fov_y = atan2( cg.refdef.height, x );
	fov_y = fov_y * 360 / M_PI;

	// there's a problem with this, it only takes the leafbrushes into account, not the entity brushes,
	//	so if you give slime/water etc properties to a func_door area brush in order to move the whole water
	//	level up/down this doesn't take into account the door position, so warps the view the whole time
	//	whether the water is up or not.
	// warp if underwater
	float	phase;
	float	v;
	cg.refdef.viewContents = 0;
	if (gi.totalMapContents() & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ))
	{
		cg.refdef.viewContents = CG_PointContents( cg.refdef.vieworg, -1 );
	}
	if ( cg.refdef.viewContents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) )
	{
		phase = cg.time / 1000.0 * WAVE_FREQUENCY * M_PI * 2;
		v = WAVE_AMPLITUDE * sin( phase );
		fov_x += v;
		fov_y -= v;
		inwater = qtrue;
	}
	else
	{
		inwater = qfalse;
	}

	// see if we are drugged by an interrogator.  We muck with the FOV here, a bit later, after viewangles are calc'ed, I muck with those too.
	if ( cg.wonkyTime > 0 && cg.wonkyTime > cg.time )
	{
		float perc = (float)(cg.wonkyTime - cg.time) / 10000.0f; // goes for 10 seconds

		fov_x += ( 25.0f * perc );
		fov_y -= ( cos( cg.time * 0.0008f ) * 5.0f * perc );
	}

	// set it
	cg.refdef.fov_x = fov_x;
	cg.refdef.fov_y = fov_y;

	return (inwater);
}

float CG_ForceSpeedFOV( void )
{
	float fov;
	float timeLeft = player->client->ps.forcePowerDuration[FP_SPEED] - cg.time;
	float length = FORCE_SPEED_DURATION*forceSpeedValue[player->client->ps.forcePowerLevel[FP_SPEED]];
	float amt = forceSpeedFOVMod[player->client->ps.forcePowerLevel[FP_SPEED]];
	if ( timeLeft < 500 )
	{//start going back
		fov = cg_fov.value + (timeLeft)/500*amt;
	}
	else if ( length - timeLeft < 1000 )
	{//start zooming in
		fov = cg_fov.value + (length - timeLeft)/1000*amt;
	}
	else
	{//stay at this FOV
		fov = cg_fov.value+amt;
	}
	return fov;
}
/*
====================
CG_CalcFov

Fixed fov at intermissions, otherwise account for fov variable and zooms.
====================
*/
static qboolean	CG_CalcFov( void ) {
	float	fov_x;
	float	f;

	if ( cg.predicted_player_state.pm_type == PM_INTERMISSION ) {
		// if in intermission, use a fixed value
		fov_x = 80;
	}
	else if ( cg.snap
		&& cg.snap->ps.viewEntity > 0
		&& cg.snap->ps.viewEntity < ENTITYNUM_WORLD
		&& (!cg.renderingThirdPerson || g_entities[cg.snap->ps.viewEntity].e_DieFunc == dieF_camera_die) )
	{
		// if in entity camera view, use a special FOV
		if ( &g_entities[cg.snap->ps.viewEntity] &&
			g_entities[cg.snap->ps.viewEntity].NPC )
		{//FIXME: looks bad when take over a jedi... but never really do that, do we?
			fov_x = g_entities[cg.snap->ps.viewEntity].NPC->stats.hfov;
			//sanity-cap?
			if ( fov_x > 120 )
			{
				fov_x = 120;
			}
			else if ( fov_x < 10 )
			{
				fov_x = 10;
			}
		}
		else
		{
			if ( cg.overrides.active & CG_OVERRIDE_FOV )
			{
				fov_x = cg.overrides.fov;
			}
			else
			{
				fov_x = 120;//FIXME: read from the NPC's fov stats?
			}
		}
	}
	else if ( (!cg.zoomMode || cg.zoomMode > 2) && (cg.snap->ps.forcePowersActive&(1<<FP_SPEED)) && player->client->ps.forcePowerDuration[FP_SPEED] )//cg.renderingThirdPerson &&
	{
		fov_x = CG_ForceSpeedFOV();
	} else {
		// user selectable
		if ( cg.overrides.active & CG_OVERRIDE_FOV )
		{
			fov_x = cg.overrides.fov;
		}
		else
		{
			fov_x = cg_fov.value;
		}
		if ( fov_x < 1 ) {
			fov_x = 1;
		} else if ( fov_x > 160 ) {
			fov_x = 160;
		}

		// Disable zooming when in third person
		if ( cg.zoomMode && cg.zoomMode < 3 )//&& !cg.renderingThirdPerson ) // light amp goggles do none of the zoom silliness
		{
			if ( !cg.zoomLocked )
			{
				if ( cg.zoomMode == 1 )
				{
					// binoculars zooming either in or out
					cg_zoomFov += cg.zoomDir * cg.frametime * 0.05f;
				}
				else
				{
					// disruptor zooming in faster
					cg_zoomFov -= cg.frametime * 0.075f;
				}

				// Clamp zoomFov
				float actualFOV = (cg.overrides.active&CG_OVERRIDE_FOV) ? cg.overrides.fov : cg_fov.value;
				if ( cg_zoomFov < MAX_ZOOM_FOV )
				{
					cg_zoomFov = MAX_ZOOM_FOV;
				}
				else if ( cg_zoomFov > actualFOV )
				{
					cg_zoomFov = actualFOV;
				}
				else
				{//still zooming
					static int zoomSoundTime = 0;

					if ( zoomSoundTime < cg.time )
					{
						sfxHandle_t snd;

						if ( cg.zoomMode == 1 )
						{
							snd = cgs.media.zoomLoop;
						}
						else
						{
							snd = cgs.media.disruptorZoomLoop;
						}

						// huh?  This could probably just be added as a looping sound??
						cgi_S_StartSound( cg.refdef.vieworg, ENTITYNUM_WORLD, CHAN_LOCAL, snd );
						zoomSoundTime = cg.time + 150;
					}
				}
			}

			fov_x = cg_zoomFov;
		} else {
			f = ( cg.time - cg.zoomTime ) / ZOOM_OUT_TIME;
			if ( f <= 1.0 ) {
				fov_x = cg_zoomFov + f * ( fov_x - cg_zoomFov );
			}
		}
	}

//	g_fov = fov_x;
	return ( CG_CalcFOVFromX( fov_x ) );
}



/*
===============
CG_DamageBlendBlob

===============
*/
static void CG_DamageBlendBlob( void )
{
	int			t;
	int			maxTime;
	refEntity_t		ent;

	if ( !cg.damageValue ) {
		return;
	}

	maxTime = DAMAGE_TIME;
	t = cg.time - cg.damageTime;
	if ( t <= 0 || t >= maxTime ) {
		return;
	}

	memset( &ent, 0, sizeof( ent ) );
	ent.reType = RT_SPRITE;
	ent.renderfx = RF_FIRST_PERSON;

	VectorMA( cg.refdef.vieworg, 8, cg.refdef.viewaxis[0], ent.origin );
	VectorMA( ent.origin, cg.damageX * -8, cg.refdef.viewaxis[1], ent.origin );
	VectorMA( ent.origin, cg.damageY * 8, cg.refdef.viewaxis[2], ent.origin );

	ent.radius = cg.damageValue * 3 * ( 1.0 - ((float)t / maxTime) );
	ent.customShader = cgs.media.damageBlendBlobShader;
	ent.shaderRGBA[0] = 180 * ( 1.0 - ((float)t / maxTime) );
	ent.shaderRGBA[1] = 50 * ( 1.0 - ((float)t / maxTime) );
	ent.shaderRGBA[2] = 50 * ( 1.0 - ((float)t / maxTime) );
	ent.shaderRGBA[3] = 255;

	cgi_R_AddRefEntityToScene( &ent );
}

/*
====================
CG_SaberClashFlare
====================
*/
extern int g_saberFlashTime;
extern vec3_t g_saberFlashPos;
extern qboolean CG_WorldCoordToScreenCoord(vec3_t worldCoord, int *x, int *y);
void CG_SaberClashFlare( void )
{
	int				t, maxTime = 150;

	t = cg.time - g_saberFlashTime;

	if ( t <= 0 || t >= maxTime )
	{
		return;
	}

	vec3_t dif;

	// Don't do clashes for things that are behind us
	VectorSubtract( g_saberFlashPos, cg.refdef.vieworg, dif );

	if ( DotProduct( dif, cg.refdef.viewaxis[0] ) < 0.2 )
	{
		return;
	}

	trace_t tr;

	CG_Trace( &tr, cg.refdef.vieworg, NULL, NULL, g_saberFlashPos, -1, CONTENTS_SOLID );

	if ( tr.fraction < 1.0f )
	{
		return;
	}

	vec3_t color;
	int x,y;
	float v, len = VectorNormalize( dif );

	// clamp to a known range
	if ( len > 800 )
	{
		len = 800;
	}

	v = ( 1.0f - ((float)t / maxTime )) * ((1.0f - ( len / 800.0f )) * 2.0f + 0.35f);

	CG_WorldCoordToScreenCoord( g_saberFlashPos, &x, &y );

	VectorSet( color, 0.8f, 0.8f, 0.8f );
	cgi_R_SetColor( color );

	CG_DrawPic( x - ( v * 300 ), y - ( v * 300 ),
				v * 600, v * 600,
				cgi_R_RegisterShader( "gfx/effects/saberFlare" ));
}

/*
===============
CG_CalcViewValues

Sets cg.refdef view values
===============
*/
static qboolean CG_CalcViewValues( void ) {
	playerState_t	*ps;
	//qboolean		viewEntIsHumanoid = qfalse;
	qboolean		viewEntIsCam = qfalse;

	memset( &cg.refdef, 0, sizeof( cg.refdef ) );

	// calculate size of 3D view
	CG_CalcVrect();

	if( cg.snap->ps.viewEntity != 0 && cg.snap->ps.viewEntity < ENTITYNUM_WORLD &&
		g_entities[cg.snap->ps.viewEntity].client)
	{
		ps = &g_entities[cg.snap->ps.viewEntity].client->ps;
		//viewEntIsHumanoid = qtrue;
	}
	else
	{
		ps = &cg.predicted_player_state;
	}
#ifndef FINAL_BUILD
	trap_Com_SetOrgAngles(ps->origin,ps->viewangles);
#endif
	// intermission view
	if ( ps->pm_type == PM_INTERMISSION ) {
		VectorCopy( ps->origin, cg.refdef.vieworg );
		VectorCopy( ps->viewangles, cg.refdefViewAngles );
		AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );
		return CG_CalcFov();
	}

	cg.bobcycle = ( ps->bobCycle & 128 ) >> 7;
	cg.bobfracsin = fabs( sin( ( ps->bobCycle & 127 ) / 127.0 * M_PI ) );
	cg.xyspeed = sqrt( ps->velocity[0] * ps->velocity[0] +
		ps->velocity[1] * ps->velocity[1] );

	if ( G_IsRidingVehicle( &g_entities[0] ) )
	{
		VectorCopy( ps->origin, cg.refdef.vieworg );
		VectorCopy( cg_entities[g_entities[0].owner->s.number].lerpAngles, cg.refdefViewAngles );
		if ( !(ps->eFlags&EF_NODRAW) )
		{//riding it, not *inside* it
			//let us look up & down
			cg.refdefViewAngles[PITCH] = cg_entities[ps->clientNum].lerpAngles[PITCH] * 0.2f;
		}
	}
	else if ( cg.snap->ps.viewEntity > 0 && cg.snap->ps.viewEntity < ENTITYNUM_WORLD )
	{//in an entity camera view
		/*
		if ( g_entities[cg.snap->ps.viewEntity].client && cg.renderingThirdPerson )
		{
			VectorCopy( g_entities[cg.snap->ps.viewEntity].client->renderInfo.eyePoint, cg.refdef.vieworg );
		}
		else
		*/
		{
			VectorCopy( cg_entities[cg.snap->ps.viewEntity].lerpOrigin, cg.refdef.vieworg );
		}
		VectorCopy( cg_entities[cg.snap->ps.viewEntity].lerpAngles, cg.refdefViewAngles );
		if ( !Q_stricmp( "misc_camera", g_entities[cg.snap->ps.viewEntity].classname ) || g_entities[cg.snap->ps.viewEntity].s.weapon == WP_TURRET )
		{
			viewEntIsCam = qtrue;
		}
	}
	else if ( cg.renderingThirdPerson && !cg.zoomMode && (cg.overrides.active&CG_OVERRIDE_3RD_PERSON_ENT) )
	{//different center, same angle
		VectorCopy( cg_entities[cg.overrides.thirdPersonEntity].lerpOrigin, cg.refdef.vieworg );
		VectorCopy( ps->viewangles, cg.refdefViewAngles );
	}
	else
	{//player's center and angles
		VectorCopy( ps->origin, cg.refdef.vieworg );
		VectorCopy( ps->viewangles, cg.refdefViewAngles );
	}

	// add error decay
	if ( cg_errorDecay.value > 0 ) {
		int		t;
		float	f;

		t = cg.time - cg.predictedErrorTime;
		f = ( cg_errorDecay.value - t ) / cg_errorDecay.value;
		if ( f > 0 && f < 1 ) {
			VectorMA( cg.refdef.vieworg, f, cg.predictedError, cg.refdef.vieworg );
		} else {
			cg.predictedErrorTime = 0;
		}
	}

	if ( (cg.renderingThirdPerson||cg.snap->ps.weapon == WP_SABER||cg.snap->ps.weapon == WP_MELEE)
		&& !cg.zoomMode
		&& !viewEntIsCam )
	{
		// back away from character
//		if ( cg_thirdPerson.integer == CG_CAM_ABOVE)
//		{			`
//			CG_OffsetThirdPersonOverheadView();
//		}
//		else
//		{
		// First person saber
		if ( !cg.renderingThirdPerson )
		{
			if ( cg.snap->ps.weapon == WP_SABER||cg.snap->ps.weapon == WP_MELEE )
			{
				vec3_t dir;
				CG_OffsetFirstPersonView( qtrue );
				cg.refdef.vieworg[2] += 32;
				AngleVectors( cg.refdefViewAngles, dir, NULL, NULL );
				VectorMA( cg.refdef.vieworg, -2, dir, cg.refdef.vieworg );
			}
		}
		CG_OffsetThirdPersonView();
//		}
	}
	else
	{
		// offset for local bobbing and kicks
		CG_OffsetFirstPersonView( qfalse );
		centity_t	*playerCent = &cg_entities[0];
		if ( playerCent && playerCent->gent && playerCent->gent->client )
		{
			VectorCopy( cg.refdef.vieworg, playerCent->gent->client->renderInfo.eyePoint );
			VectorCopy( cg.refdefViewAngles, playerCent->gent->client->renderInfo.eyeAngles );
			if ( cg.snap->ps.viewEntity > 0 && cg.snap->ps.viewEntity < ENTITYNUM_WORLD )
			{//in an entity camera view
				if ( cg_entities[cg.snap->ps.viewEntity].gent->client )
				{//looking through a client's eyes
					VectorCopy( cg.refdef.vieworg, cg_entities[cg.snap->ps.viewEntity].gent->client->renderInfo.eyePoint );
					VectorCopy( cg.refdefViewAngles, cg_entities[cg.snap->ps.viewEntity].gent->client->renderInfo.eyeAngles );
				}
				else
				{//looking through a regular ent's eyes
					VectorCopy( cg.refdef.vieworg, cg_entities[cg.snap->ps.viewEntity].lerpOrigin );
					VectorCopy( cg.refdefViewAngles, cg_entities[cg.snap->ps.viewEntity].lerpAngles );
				}
			}
			VectorCopy( playerCent->gent->client->renderInfo.eyePoint, playerCent->gent->client->renderInfo.headPoint );
			if ( cg.snap->ps.viewEntity <= 0 || cg.snap->ps.viewEntity >= ENTITYNUM_WORLD )
			{//not in entity cam
				playerCent->gent->client->renderInfo.headPoint[2] -= 8;
			}
		}
	}

	//VectorCopy( cg.refdef.vieworg, cgRefdefVieworg );
	// shake the camera if necessary
	CGCam_UpdateSmooth( cg.refdef.vieworg, cg.refdefViewAngles );
	CGCam_UpdateShake( cg.refdef.vieworg, cg.refdefViewAngles );

	/*
	if ( in_camera )
	{
		Com_Printf( "%s %s\n", vtos(client_camera.origin), vtos(cg.refdef.vieworg) );
	}
	*/

	// see if we are drugged by an interrogator.  We muck with the angles here, just a bit earlier, we mucked with the FOV
	if ( cg.wonkyTime > 0 && cg.wonkyTime > cg.time )
	{
		float perc = (float)(cg.wonkyTime - cg.time) / 10000.0f; // goes for 10 seconds

		cg.refdefViewAngles[ROLL] += ( sin( cg.time * 0.0004f )  * 7.0f * perc );
		cg.refdefViewAngles[PITCH] += ( 26.0f * perc + sin( cg.time * 0.0011f ) * 3.0f * perc );
	}

	AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );

	if ( cg.hyperspace )
	{
		cg.refdef.rdflags |= RDF_NOWORLDMODEL | RDF_HYPERSPACE;
	}

	// field of view
	return CG_CalcFov();
}


/*
=====================
CG_PowerupTimerSounds
=====================
*/
static void CG_PowerupTimerSounds( void )
{
	int	i, time;

	// powerup timers going away
	for ( i = 0 ; i < MAX_POWERUPS ; i++ )
	{
		time = cg.snap->ps.powerups[i];

		if ( time > 0 && time < cg.time )
		{
			// add any special powerup expiration sounds here
//			switch( i )
//			{
//			case PW_WEAPON_OVERCHARGE:
//				cgi_S_StartSound( NULL, cg.snap->ps.clientNum, CHAN_ITEM, cgs.media.overchargeEndSound );
//				break;
//			}
		}
	}
}

/*
==============
CG_DrawSkyBoxPortal
==============
*/
extern void cgi_CM_SnapPVS(vec3_t origin,byte *buffer);

static void CG_DrawSkyBoxPortal(void)
{
	refdef_t backuprefdef;
	const char *cstr;
	char *token;

	cstr = CG_ConfigString(CS_SKYBOXORG);

	if (!cstr || !strlen(cstr))
	{
		// no skybox in this map
		return;
	}

	backuprefdef = cg.refdef;

	// asdf --eez
	COM_BeginParseSession();
	token = COM_ParseExt(&cstr, qfalse);
	if (!token || !token[0])
	{
		CG_Error( "CG_DrawSkyBoxPortal: error parsing skybox configstring\n");
	}
	cg.refdef.vieworg[0] = atof(token);

	token = COM_ParseExt(&cstr, qfalse);
	if (!token || !token[0])
	{
		CG_Error( "CG_DrawSkyBoxPortal: error parsing skybox configstring\n");
	}
	cg.refdef.vieworg[1] = atof(token);

	token = COM_ParseExt(&cstr, qfalse);
	if (!token || !token[0])
	{
		CG_Error( "CG_DrawSkyBoxPortal: error parsing skybox configstring\n");
	}
	cg.refdef.vieworg[2] = atof(token);

	// setup fog the first time, ignore this part of the configstring after that
	token = COM_ParseExt(&cstr, qfalse);
	if (!token || !token[0])
	{
		CG_Error( "CG_DrawSkyBoxPortal: error parsing skybox configstring.  No fog state\n");
	}
	else
	{
		if ( atoi( token ) ) {
			// this camera has fog
			token = COM_ParseExt( &cstr, qfalse );
			if ( !VALIDSTRING( token ) ) {
				CG_Error( "CG_DrawSkyBoxPortal: error parsing skybox configstring.  No fog[0]\n");
			}

			token = COM_ParseExt(&cstr, qfalse);
			if ( !VALIDSTRING( token ) ) {
				CG_Error( "CG_DrawSkyBoxPortal: error parsing skybox configstring.  No fog[1]\n");
			}

			token = COM_ParseExt(&cstr, qfalse);
			if ( !VALIDSTRING( token ) ) {
				CG_Error( "CG_DrawSkyBoxPortal: error parsing skybox configstring.  No fog[2]\n");
			}

			COM_ParseExt( &cstr, qfalse );
			COM_ParseExt( &cstr, qfalse );
		}
	}

	COM_EndParseSession();
/*
	static float lastfov = cg_zoomFov;	// for transitions back from zoomed in modes
	float fov_x;
	float fov_y;
	float x;
	float f = 0;

	if ( cg.predicted_player_state.pm_type == PM_INTERMISSION )
	{
		// if in intermission, use a fixed value
		fov_x = (cg.overrides.active&CG_OVERRIDE_FOV) ? cg.overrides.fov : cg_fov.value;
	}
	else
	{
		if (cg.zoomMode)
		{
			fov_x = cg_zoomFov;
		}
		else
		{
			fov_x = (cg.overrides.active&CG_OVERRIDE_FOV) ? cg.overrides.fov : cg_fov.value;
			if ( fov_x < 1 )
			{
				fov_x = 1;
			}
			else if ( fov_x > 160 )
			{
				fov_x = 160;
			}
		}

		// do smooth transitions for zooming
		if (cg.zoomMode)
		{ //zoomed/zooming in
			f = ( cg.time - cg.zoomTime ) / (float)ZOOM_OUT_TIME;
			if ( f > 1.0 ) {
				fov_x = cg_zoomFov;
			} else {
				fov_x = fov_x + f * ( cg_zoomFov - fov_x );
			}
			lastfov = fov_x;
		}
		else
		{ //zooming out
			f = ( cg.time - cg.zoomTime ) / (float)ZOOM_OUT_TIME;
			if ( f > 1.0 ) {
				fov_x = fov_x;
			} else {
				fov_x = cg_zoomFov + f * ( fov_x - cg_zoomFov);
			}
		}
	}

	x = cg.refdef.width / tan( fov_x / 360 * M_PI );
	fov_y = atan2( cg.refdef.height, x );
	fov_y = fov_y * 360 / M_PI;

	cg.refdef.fov_x = fov_x;
	cg.refdef.fov_y = fov_y;
*/
	//inherit fov and axis from whatever the player is doing (regular, camera overrides or zoomed, whatever)
	if ( !cg.hyperspace )
	{
		CG_AddPacketEntities(qtrue); //rww - There was no proper way to put real entities inside the portal view before.
									 //This will put specially flagged entities in the render.
		//Add effects flagged to play only in portals
		theFxScheduler.AddScheduledEffects( true );
	}

	cg.refdef.rdflags |= RDF_SKYBOXPORTAL;	//mark portal scene specialness
	cg.refdef.rdflags |= RDF_DRAWSKYBOX;	//drawk portal skies

	cgi_CM_SnapPVS( cg.refdef.vieworg, cg.refdef.areamask );	//fill in my areamask for this view origin
	// draw the skybox
	cgi_R_RenderScene( &cg.refdef );

	cg.refdef = backuprefdef;
}

//----------------------------
void CG_RunEmplacedWeapon()
{
	gentity_t	*player = &g_entities[0],
				*gun = player->owner;

	// Override the camera when we are locked onto the gun.
	if ( player
		&& gun
		&& !gun->bounceCount//not an eweb
		&& ( player->s.eFlags & EF_LOCKED_TO_WEAPON ))
	{
//		float dist = -1; // default distance behind gun

		// don't let the player try and change this
		cg.renderingThirdPerson = qtrue;

//		cg.refdefViewAngles[PITCH] += cg.overrides.thirdPersonPitchOffset? cg.overrides.thirdPersonPitchOffset: cg_thirdPersonPitchOffset.value;
//		cg.refdefViewAngles[YAW] += cg.overrides.thirdPersonAngle ? cg.overrides.thirdPersonAngle : cg_thirdPersonAngle.value;;

		AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );

		// Slide in behind the gun.
//		if ( gun->delay + 500 > cg.time )
//		{
//			dist -= (( gun->delay + 500 ) - cg.time ) * 0.02f;
//		}

		VectorCopy( gun->pos2, cg.refdef.vieworg );
		VectorMA( cg.refdef.vieworg, -20.0f, gun->pos3, cg.refdef.vieworg );
		if ( cg.snap->ps.viewEntity <= 0 || cg.snap->ps.viewEntity >= ENTITYNUM_WORLD )
		{
			VectorMA( cg.refdef.vieworg, 35.0f, gun->pos4, cg.refdef.vieworg );
		}

	}
}

//=========================================================================

/*
=================
CG_DrawActiveFrame

Generates and draws a game scene and status information at the given time.
=================
*/
extern void CG_BuildSolidList( void );
extern void CG_ClearHealthBarEnts( void );
extern vec3_t	serverViewOrg;
static qboolean cg_rangedFogging = qfalse; //so we know if we should go back to normal fog
void CG_DrawActiveFrame( int serverTime, stereoFrame_t stereoView ) {
	qboolean	inwater = qfalse;

	cg.time = serverTime;

	// update cvars
	CG_UpdateCvars();

	// if we are only updating the screen as a loading
	// pacifier, don't even try to read snapshots
	if ( cg.infoScreenText[0] != 0 ) {
		CG_DrawInformation();
		return;
	}

	CG_ClearHealthBarEnts();

	CG_RunLightStyles();

	// any looped sounds will be respecified as entities
	// are added to the render list
	cgi_S_ClearLoopingSounds();

	// clear all the render lists
	cgi_R_ClearScene();

	CG_BuildSolidList();

	// set up cg.snap and possibly cg.nextSnap
	CG_ProcessSnapshots();
	// if we haven't received any snapshots yet, all
	// we can draw is the information screen
	if ( !cg.snap ) {
		//CG_DrawInformation();
		return;
	}

	// make sure the lagometerSample and frame timing isn't done twice when in stereo
	if ( stereoView != STEREO_RIGHT ) {
		cg.frametime = cg.time - cg.oldTime;
		cg.oldTime = cg.time;
	}
	// Make sure the helper has the updated time
	theFxHelper.AdjustTime( cg.frametime );

	// let the client system know what our weapon and zoom settings are
	//FIXME: should really send forcePowersActive over network onto cg.snap->ps...
	const int fpActive = cg_entities[0].gent->client->ps.forcePowersActive;
	const bool matrixMode = !!(fpActive & ((1 << FP_SPEED) | (1 << FP_RAGE)));
	float speed = cg.refdef.fov_y / 75.0 * (matrixMode ? 1.0f : cg_timescale.value);

//FIXME: junk code, BUG:168

	static bool wasForceSpeed=false;
	bool isForceSpeed=cg_entities[0].gent->client->ps.forcePowersActive&(1<<FP_SPEED)?true:false;
	if (isForceSpeed&&!wasForceSpeed)
	{
		CGCam_Smooth(0.75f,5000);
	}
	wasForceSpeed=isForceSpeed;

//

	float mPitchOverride = 0.0f;
	float mYawOverride = 0.0f;
	if ( cg.snap->ps.clientNum == 0 )
	{//pointless check, but..
		if ( cg_entities[0].gent->s.eFlags & EF_LOCKED_TO_WEAPON )
		{
			speed *= 0.25f;
		}
		Vehicle_t *pVeh = NULL;

		// Mouse turns slower.
		if ( ( pVeh = G_IsRidingVehicle( &g_entities[0] ) ) != NULL )
		{
			if ( pVeh->m_pVehicleInfo->mousePitch )
			{
				mPitchOverride = pVeh->m_pVehicleInfo->mousePitch;
			}
			if ( pVeh->m_pVehicleInfo->mouseYaw )
			{
				mYawOverride = pVeh->m_pVehicleInfo->mouseYaw;
			}
		}
	}
	cgi_SetUserCmdValue( cg.weaponSelect, speed, mPitchOverride, mYawOverride );

	// this counter will be bumped for every valid scene we generate
	cg.clientFrame++;

	// update cg.predicted_player_state
	CG_PredictPlayerState();

	if (cg.snap->ps.eFlags&EF_HELD_BY_SAND_CREATURE)
	{
		cg.zoomMode = 0;
	}
	// decide on third person view
	cg.renderingThirdPerson = cg_thirdPerson.integer
								|| (cg.snap->ps.stats[STAT_HEALTH] <= 0)
								|| (cg.snap->ps.eFlags&EF_HELD_BY_SAND_CREATURE)
								|| ((g_entities[0].client&&g_entities[0].client->NPC_class==CLASS_ATST)
								|| (cg.snap->ps.weapon == WP_SABER || cg.snap->ps.weapon == WP_MELEE) );

	if ( cg.zoomMode )
	{
		// zoomed characters should never do third person stuff??
		cg.renderingThirdPerson = qfalse;
	}

	if ( in_camera )
	{
		// The camera takes over the view
		CGCam_RenderScene();
	}
	else
	{
		//Finish any fading that was happening
		CGCam_UpdateFade();
		// build cg.refdef
		inwater = CG_CalcViewValues();
	}

	if (cg.zoomMode)
	{ //zooming with binoculars or sniper, set the fog range based on the zoom level -rww
		cg_rangedFogging = qtrue;
		//smaller the fov the less fog we have between the view and cull dist
		cgi_R_SetRangeFog(cg.refdef.fov_x*64.0f);
	}
	else if (cg_rangedFogging)
	{ //disable it
		cg_rangedFogging = qfalse;
		cgi_R_SetRangeFog(0.0f);
	}

	cg.refdef.time = cg.time;

	CG_DrawSkyBoxPortal();

	// NOTE: this may completely override the camera
	CG_RunEmplacedWeapon();

	// first person blend blobs, done after AnglesToAxis
	if ( !cg.renderingThirdPerson ) {
		CG_DamageBlendBlob();
	}

	// build the render lists
	if ( !cg.hyperspace ) {
		CG_AddPacketEntities(qfalse);			// adter calcViewValues, so predicted player state is correct
		CG_AddMarks();
		CG_AddLocalEntities();
		CG_DrawMiscEnts();
	}

	//check for opaque water
	// this game does not have opaque water
	if ( 0 )
	{
		vec3_t	camTest;
		VectorCopy( cg.refdef.vieworg, camTest );
		camTest[2] += 6;
		if ( !(CG_PointContents( camTest, 0 )&CONTENTS_SOLID) && !gi.inPVS( cg.refdef.vieworg, camTest ) )
		{//crossed visible line into another room
			cg.refdef.vieworg[2] -= 6;
			//cgi_CM_SnapPVS(cg.refdef.vieworg,cg.snap->areamask);
		}
		else
		{
			VectorCopy( cg.refdef.vieworg, camTest );
			camTest[2] -= 6;
			if ( !(CG_PointContents( camTest, 0 )&CONTENTS_SOLID) && !gi.inPVS( cg.refdef.vieworg, camTest ) )
			{
				cg.refdef.vieworg[2] += 6;
				//cgi_CM_SnapPVS(cg.refdef.vieworg,cg.snap->areamask);
			}
			else //if ( inwater )
			{//extra-special hack... sometimes when crouched in water with first person lightsaber, your PVS is wrong???
				/*
				if ( !cg.renderingThirdPerson && (cg.snap->ps.weapon == WP_SABER||cg.snap->ps.weapon == WP_MELEE) )
				{//pseudo first-person for saber and fists
					cgi_CM_SnapPVS(cg.refdef.vieworg,cg.snap->areamask);
				}
				*/
			}
		}
	}
	//FIXME: first person crouch-uncrouch STILL FUCKS UP THE AREAMASK!!!
	//if ( !VectorCompare2( cg.refdef.vieworg, cg.snap->ps.serverViewOrg ) && !gi.inPVS( cg.refdef.vieworg, cg.snap->ps.serverViewOrg ) )
	{//actual view org and server's view org don't match and aren't same PVS, rebuild the areamask
		//Com_Printf( S_COLOR_RED"%s != %s\n", vtos(cg.refdef.vieworg), vtos(cg.snap->ps.serverViewOrg) );
		cgi_CM_SnapPVS( cg.refdef.vieworg, cg.snap->areamask );
	}

	// Don't draw the in-view weapon when in camera mode
	if ( !in_camera
		&& !cg_pano.integer
		&& cg.snap->ps.weapon != WP_SABER
		&& ( cg.snap->ps.viewEntity == 0 || cg.snap->ps.viewEntity >= ENTITYNUM_WORLD ) )
	{
		CG_AddViewWeapon( &cg.predicted_player_state );
	}
	else if( cg.snap->ps.viewEntity != 0 && cg.snap->ps.viewEntity < ENTITYNUM_WORLD )
	{
		if( g_entities[cg.snap->ps.viewEntity].client && g_entities[cg.snap->ps.viewEntity].NPC )
		{
			CG_AddViewWeapon( &g_entities[cg.snap->ps.viewEntity ].client->ps );	// HAX - because I wanted to --eez
		}
	}

	if ( !cg.hyperspace && fx_freeze.integer<2 )
	{
		//Add all effects
		theFxScheduler.AddScheduledEffects( false );
	}

	// finish up the rest of the refdef
	if ( cg.testModelEntity.hModel ) {
		CG_AddTestModel();
	}

	memcpy( cg.refdef.areamask, cg.snap->areamask, sizeof( cg.refdef.areamask ) );

	// update audio positions
	//This is done from the vieworg to get origin for non-attenuated sounds
	cgi_S_UpdateAmbientSet( CG_ConfigString( CS_AMBIENT_SET ), cg.refdef.vieworg );
	//NOTE: if we want to make you be able to hear far away sounds with electrobinoculars, add the hacked-in positional offset here (base on fov)
	/*
	vec3_t listener_origin;
	VectorCopy( cg.refdef.vieworg, listener_origin );
	if ( cg.snap->ps.forcePowersActive&(1<<FP_SEE)
		&& cg.snap->ps.forcePowerLevel[FP_SEE] >= FORCE_LEVEL_2 )
	{//FIXME: if I can see through walls, why not actually move the listener_position through the wall too?
		if ( g_crosshairEntNum < ENTITYNUM_WORLD
			&& g_crosshairSameEntTime >= 1000 )
		{//been looking at this guy for at least a full second
			gentity_t *crossEnt = &g_entities[g_crosshairEntNum];
			if ( crossEnt->client && crossEnt->health >= 0 )
			{//a living client, let's listen in...?
				//FIXME: lerp our listener_origin to their position over a second or two?
				VectorCopy( cg_entities[g_crosshairEntNum].lerpOrigin, listener_origin );
				inwater = qtrue;//do cool reverby effect...?
			}
		}
	}
	cgi_S_Respatialize( cg.snap->ps.clientNum, listener_origin, cg.refdef.viewaxis, inwater );
	*/
	cgi_S_Respatialize( cg.snap->ps.clientNum, cg.refdef.vieworg, cg.refdef.viewaxis, inwater );

	// warning sounds when powerup is wearing off
	CG_PowerupTimerSounds();

	if ( cg_pano.integer ) {	// let's grab a panorama!
		cg.levelShot = qtrue;  //hide the 2d
		VectorClear(cg.refdefViewAngles);
		cg.refdefViewAngles[YAW] = -360 * cg_pano.integer/cg_panoNumShots.integer;	//choose angle
		AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );
		CG_DrawActive( stereoView );
		cg.levelShot = qfalse;
	} 	else {
		// actually issue the rendering calls
		CG_DrawActive( stereoView );
	}
	/*
	if ( in_camera && !cg_skippingcin.integer )
	{
		Com_Printf( S_COLOR_GREEN"ang: %s\n", vtos(cg.refdefViewAngles) );
	}
	*/
}

