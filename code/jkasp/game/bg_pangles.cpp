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

#include "common_headers.h"

// define GAME_INCLUDE so that g_public.h does not define the
// short, server-visible gclient_t and gentity_t structures,
// because we define the full size ones in this file
#define GAME_INCLUDE
#include "../qcommon/q_shared.h"
#include "g_shared.h"
#include "bg_local.h"
#include "anims.h"
#include "wp_saber.h"
#include "g_vehicles.h"
#include "../ghoul2/ghoul2_gore.h"

extern void CG_SetClientViewAngles( vec3_t angles, qboolean overrideViewEnt );
extern qboolean PM_InAnimForSaberMove( int anim, int saberMove );
extern qboolean PM_InForceGetUp( playerState_t *ps );
extern qboolean PM_InKnockDown( playerState_t *ps );
extern qboolean PM_InReboundJump( int anim );
extern qboolean PM_StabDownAnim( int anim );
extern qboolean PM_DodgeAnim( int anim );
extern qboolean PM_DodgeHoldAnim( int anim );
extern qboolean PM_InReboundHold( int anim );
extern qboolean PM_InKnockDownNoGetup( playerState_t *ps );
extern qboolean PM_InGetUpNoRoll( playerState_t *ps );
extern Vehicle_t *G_IsRidingVehicle( gentity_t *ent );
extern void WP_ForcePowerDrain( gentity_t *self, forcePowers_t forcePower, int overrideAmt );
extern qboolean G_ControlledByPlayer( gentity_t *self );

extern qboolean cg_usingInFrontOf;
extern qboolean		player_locked;
extern pmove_t		*pm;
extern pml_t		pml;

extern cvar_t	*g_debugMelee;


void BG_IK_MoveLimb( CGhoul2Info_v &ghoul2, int boltIndex, char *animBone, char *firstBone, char *secondBone,
				   int time, entityState_t *ent, int animFileIndex, int basePose,
				   vec3_t desiredPos, qboolean *ikInProgress, vec3_t origin,
				   vec3_t angles, vec3_t scale, int blendTime, qboolean forceHalt )
{
	mdxaBone_t holdPointMatrix;
	vec3_t holdPoint;
	vec3_t torg;
	float distToDest;
	animation_t	*anim = &level.knownAnimFileSets[animFileIndex].animations[basePose];

	assert( ghoul2.size() );

	assert( anim->firstFrame > 0 );//FIXME: umm...?

	if ( !*ikInProgress && !forceHalt )
	{
		sharedSetBoneIKStateParams_t ikP;

		//restrict the shoulder joint
		//VectorSet(ikP.pcjMins,-50.0f,-80.0f,-15.0f);
		//VectorSet(ikP.pcjMaxs,15.0f,40.0f,15.0f);

		//for now, leaving it unrestricted, but restricting elbow joint.
		//This lets us break the arm however we want in order to fling people
		//in throws, and doesn't look bad.
		VectorSet( ikP.pcjMins, 0, 0, 0 );
		VectorSet( ikP.pcjMaxs, 0, 0, 0 );

		//give the info on our entity.
		ikP.blendTime = blendTime;
		VectorCopy( origin, ikP.origin );
		VectorCopy( angles, ikP.angles );
		ikP.angles[PITCH] = 0;
		ikP.pcjOverrides = 0;
		ikP.radius = 10.0f;
		VectorCopy( scale, ikP.scale );

		//base pose frames for the limb
		ikP.startFrame = anim->firstFrame + anim->numFrames;
		ikP.endFrame = anim->firstFrame + anim->numFrames;

		//ikP.forceAnimOnBone = qfalse; //let it use existing anim if it's the same as this one.

		//we want to call with a null bone name first. This will init all of the
		//ik system stuff on the g2 instance, because we need ragdoll effectors
		//in order for our pcj's to know how to angle properly.
		if ( !gi.G2API_SetBoneIKState( ghoul2, time, NULL, IKS_DYNAMIC, &ikP ) )
		{
			assert( !"Failed to init IK system for g2 instance!" );
		}

		//Now, create our IK bone state.
		if ( gi.G2API_SetBoneIKState( ghoul2, time, "lower_lumbar", IKS_DYNAMIC, &ikP ) )
		{
			//restrict the elbow joint
			VectorSet( ikP.pcjMins, -90.0f, -20.0f, -20.0f );
			VectorSet( ikP.pcjMaxs, 30.0f, 20.0f, -20.0f );
			if ( gi.G2API_SetBoneIKState( ghoul2, time, "upper_lumbar", IKS_DYNAMIC, &ikP ) )
			{
				//restrict the elbow joint
				VectorSet( ikP.pcjMins, -90.0f, -20.0f, -20.0f );
				VectorSet( ikP.pcjMaxs, 30.0f, 20.0f, -20.0f );
				if ( gi.G2API_SetBoneIKState( ghoul2, time, "thoracic", IKS_DYNAMIC, &ikP ) )
				{
					//restrict the elbow joint
					VectorSet( ikP.pcjMins, -90.0f, -20.0f, -20.0f );
					VectorSet( ikP.pcjMaxs, 30.0f, 20.0f, -20.0f );
					if ( gi.G2API_SetBoneIKState( ghoul2, time, secondBone, IKS_DYNAMIC, &ikP ) )
					{
						//restrict the elbow joint
						VectorSet( ikP.pcjMins, -90.0f, -20.0f, -20.0f );
						VectorSet( ikP.pcjMaxs, 30.0f, 20.0f, -20.0f );

						if ( gi.G2API_SetBoneIKState( ghoul2, time, firstBone, IKS_DYNAMIC, &ikP ) )
						{ //everything went alright.
							*ikInProgress = qtrue;
						}
					}
				}
			}
		}
	}

	if ( *ikInProgress && !forceHalt )
	{ //actively update our ik state.
		sharedIKMoveParams_t ikM;
		CRagDollUpdateParams tuParms;
		vec3_t tAngles;

		//set the argument struct up
		VectorCopy( desiredPos, ikM.desiredOrigin ); //we want the bone to move here.. if possible

		VectorCopy( angles, tAngles );
		tAngles[PITCH] = tAngles[ROLL] = 0;

		gi.G2API_GetBoltMatrix( ghoul2, 0, boltIndex, &holdPointMatrix, tAngles, origin, time, 0, scale );
		//Get the point position from the matrix.
		holdPoint[0] = holdPointMatrix.matrix[0][3];
		holdPoint[1] = holdPointMatrix.matrix[1][3];
		holdPoint[2] = holdPointMatrix.matrix[2][3];

		VectorSubtract( holdPoint, desiredPos, torg );
		distToDest = VectorLength( torg );

		//closer we are, more we want to keep updated.
		//if we're far away we don't want to be too fast or we'll start twitching all over.
		if ( distToDest < 2 )
		{ //however if we're this close we want very precise movement
			ikM.movementSpeed = 0.4f;
		}
		else if ( distToDest < 16 )
		{
			ikM.movementSpeed = 0.9f;//8.0f;
		}
		else if ( distToDest < 32 )
		{
			ikM.movementSpeed = 0.8f;//4.0f;
		}
		else if ( distToDest < 64 )
		{
			ikM.movementSpeed = 0.7f;//2.0f;
		}
		else
		{
			ikM.movementSpeed = 0.6f;
		}
		VectorCopy( origin, ikM.origin ); //our position in the world.

		ikM.boneName[0] = 0;
		if ( gi.G2API_IKMove( ghoul2, time, &ikM ) )
		{
			//now do the standard model animate stuff with ragdoll update params.
			VectorCopy( angles, tuParms.angles );
			tuParms.angles[PITCH] = 0;

			VectorCopy( origin, tuParms.position );
			VectorCopy( scale, tuParms.scale );

			tuParms.me = ent->number;
			VectorClear( tuParms.velocity );

			gi.G2API_AnimateG2Models( ghoul2, time, &tuParms );
		}
		else
		{
			*ikInProgress = qfalse;
		}
	}
	else if ( *ikInProgress )
	{ //kill it
		float cFrame, animSpeed;
		int sFrame, eFrame, flags;

		gi.G2API_SetBoneIKState( ghoul2, time, "lower_lumbar", IKS_NONE, NULL );
		gi.G2API_SetBoneIKState( ghoul2, time, "upper_lumbar", IKS_NONE, NULL );
		gi.G2API_SetBoneIKState( ghoul2, time, "thoracic", IKS_NONE, NULL );
		gi.G2API_SetBoneIKState( ghoul2, time, secondBone, IKS_NONE, NULL );
		gi.G2API_SetBoneIKState( ghoul2, time, firstBone, IKS_NONE, NULL );

		//then reset the angles/anims on these PCJs
		gi.G2API_SetBoneAngles( &ghoul2[0], "lower_lumbar", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, time );
		gi.G2API_SetBoneAngles( &ghoul2[0], "upper_lumbar", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, time );
		gi.G2API_SetBoneAngles( &ghoul2[0], "thoracic", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, time );
		gi.G2API_SetBoneAngles( &ghoul2[0], secondBone, vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, time );
		gi.G2API_SetBoneAngles( &ghoul2[0], firstBone, vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, time );

		//Get the anim/frames that the pelvis is on exactly, and match the left arm back up with them again.
		gi.G2API_GetBoneAnim( &ghoul2[0], animBone, (const int)time, &cFrame, &sFrame, &eFrame, &flags, &animSpeed, 0 );
		gi.G2API_SetBoneAnim( &ghoul2[0], "lower_lumbar", sFrame, eFrame, flags, animSpeed, time, sFrame, 300 );
		gi.G2API_SetBoneAnim( &ghoul2[0], "upper_lumbar", sFrame, eFrame, flags, animSpeed, time, sFrame, 300 );
		gi.G2API_SetBoneAnim( &ghoul2[0], "thoracic", sFrame, eFrame, flags, animSpeed, time, sFrame, 300 );
		gi.G2API_SetBoneAnim( &ghoul2[0], secondBone, sFrame, eFrame, flags, animSpeed, time, sFrame, 300 );
		gi.G2API_SetBoneAnim( &ghoul2[0], firstBone, sFrame, eFrame, flags, animSpeed, time, sFrame, 300 );

		//And finally, get rid of all the ik state effector data by calling with null bone name (similar to how we init it).
		gi.G2API_SetBoneIKState( ghoul2, time, NULL, IKS_NONE, NULL );

		*ikInProgress = qfalse;
	}
}

void PM_IKUpdate( gentity_t *ent )
{
	//The bone we're holding them by and the next bone after that
	char *animBone = "lower_lumbar";
	char *firstBone = "lradius";
	char *secondBone = "lhumerus";
	char *defaultBoltName = "*r_hand";

	if ( !ent->client )
	{
		return;
	}
	if ( ent->client->ps.heldByClient <= ENTITYNUM_WORLD )
	{ //then put our arm in this client's hand
		gentity_t *holder = &g_entities[ent->client->ps.heldByClient];

		if ( holder && holder->inuse && holder->client && holder->ghoul2.size())
		{
			if ( !ent->client->ps.heldByBolt )
			{//bolt wan't set
				ent->client->ps.heldByBolt = gi.G2API_AddBolt( &holder->ghoul2[0], defaultBoltName );
			}
		}
		else
		{ //they're gone, stop holding me
			ent->client->ps.heldByClient = 0;
			return;
		}

		if ( ent->client->ps.heldByBolt )
		{
			mdxaBone_t boltMatrix;
			vec3_t boltOrg;
			vec3_t tAngles;

			VectorCopy( holder->client->ps.viewangles, tAngles );
			tAngles[PITCH] = tAngles[ROLL] = 0;

			gi.G2API_GetBoltMatrix( holder->ghoul2, 0, ent->client->ps.heldByBolt, &boltMatrix, tAngles, holder->client->ps.origin, level.time, 0, holder->s.modelScale );
			gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, boltOrg );

			int grabbedByBolt = gi.G2API_AddBolt( &ent->ghoul2[0], firstBone );
			if ( grabbedByBolt )
			{
				//point the limb
				BG_IK_MoveLimb( ent->ghoul2, grabbedByBolt, animBone, firstBone, secondBone,
					level.time, &ent->s, ent->client->clientInfo.animFileIndex,
					ent->client->ps.torsoAnim/*BOTH_DEAD1*/, boltOrg, &ent->client->ps.ikStatus,
					ent->client->ps.origin, ent->client->ps.viewangles, ent->s.modelScale,
					500, qfalse );

				//now see if we need to be turned and/or pulled
				vec3_t grabDiff, grabbedByOrg;

				VectorCopy( ent->client->ps.viewangles, tAngles );
				tAngles[PITCH] = tAngles[ROLL] = 0;

				gi.G2API_GetBoltMatrix( ent->ghoul2, 0, grabbedByBolt, &boltMatrix, tAngles, ent->client->ps.origin, level.time, 0, ent->s.modelScale );
				gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, grabbedByOrg );

				//check for turn
				vec3_t org2Targ, org2Bolt;
				VectorSubtract( boltOrg, ent->currentOrigin, org2Targ );
				float org2TargYaw = vectoyaw( org2Targ );
				VectorSubtract( grabbedByOrg, ent->currentOrigin, org2Bolt );
				float org2BoltYaw = vectoyaw( org2Bolt );
				if ( org2TargYaw-1.0f > org2BoltYaw )
				{
					ent->currentAngles[YAW]++;
					G_SetAngles( ent, ent->currentAngles );
				}
				else if ( org2TargYaw+1.0f < org2BoltYaw )
				{
					ent->currentAngles[YAW]--;
					G_SetAngles( ent, ent->currentAngles );
				}

				//check for pull
				VectorSubtract( boltOrg, grabbedByOrg, grabDiff );
				if ( VectorLength( grabDiff ) > 128.0f )
				{//too far, release me
					ent->client->ps.heldByClient = holder->client->ps.heldClient = ENTITYNUM_NONE;
				}
				else if ( 1 )
				{//pull me along
					trace_t trace;
					vec3_t destOrg;
					VectorAdd( ent->currentOrigin, grabDiff, destOrg );
					gi.trace( &trace, ent->currentOrigin, ent->mins, ent->maxs, destOrg, ent->s.number, (ent->clipmask&~holder->contents), (EG2_Collision)0, 0 );
					G_SetOrigin( ent, trace.endpos );
					//FIXME: better yet: do an actual slidemove to the new pos?
					//FIXME: if I'm alive, just tell me to walk some?
				}
				//FIXME: if I need to turn to keep my bone facing him, do so...
			}
			//don't let us fall?
			VectorClear( ent->client->ps.velocity );
			//FIXME: also make the holder point his holding limb at you?
		}
	}
	else if ( ent->client->ps.ikStatus )
	{ //make sure we aren't IKing if we don't have anyone to hold onto us.
		if ( ent && ent->inuse && ent->client && ent->ghoul2.size() )
		{
			if ( !ent->client->ps.heldByBolt )
			{
				ent->client->ps.heldByBolt = gi.G2API_AddBolt( &ent->ghoul2[0], defaultBoltName );
			}
		}
		else
		{ //This shouldn't happen, but just in case it does, we'll have a failsafe.
			ent->client->ps.heldByBolt = 0;
			ent->client->ps.ikStatus = qfalse;
		}

		if ( ent->client->ps.heldByBolt )
		{
			BG_IK_MoveLimb( ent->ghoul2, ent->client->ps.heldByBolt, animBone, firstBone, secondBone,
				level.time, &ent->s, ent->client->clientInfo.animFileIndex,
				ent->client->ps.torsoAnim/*BOTH_DEAD1*/, (float *)vec3_origin,
				&ent->client->ps.ikStatus, ent->client->ps.origin,
				ent->client->ps.viewangles, ent->s.modelScale, 500, qtrue );
		}
	}
}


void BG_G2SetBoneAngles( centity_t *cent, gentity_t *gent, int boneIndex, const vec3_t angles, const int flags,
							 const Eorientations up, const Eorientations right, const Eorientations forward, qhandle_t *modelList )
{
	if (boneIndex!=-1)
	{
		gi.G2API_SetBoneAnglesIndex( &cent->gent->ghoul2[0], boneIndex, angles, flags, up, right, forward, modelList, 0, 0 );
	}
}

#define	MAX_YAWSPEED_X_WING		1
#define	MAX_PITCHSPEED_X_WING	1
void PM_ScaleUcmd( playerState_t *ps, usercmd_t *cmd, gentity_t *gent )
{
	if ( G_IsRidingVehicle( gent ) )
	{//driving a vehicle
		//clamp the turn rate
		int maxPitchSpeed = MAX_PITCHSPEED_X_WING;//switch, eventually?  Or read from file?
		int diff = AngleNormalize180(SHORT2ANGLE((cmd->angles[PITCH]+ps->delta_angles[PITCH]))) - floor(ps->viewangles[PITCH]);

		if ( diff > maxPitchSpeed )
		{
			cmd->angles[PITCH] = ANGLE2SHORT( ps->viewangles[PITCH] + maxPitchSpeed ) - ps->delta_angles[PITCH];
		}
		else if ( diff < -maxPitchSpeed )
		{
			cmd->angles[PITCH] = ANGLE2SHORT( ps->viewangles[PITCH] - maxPitchSpeed ) - ps->delta_angles[PITCH];
		}

		//Um, WTF?  When I turn in a certain direction, I start going backwards?  Or strafing?
		int maxYawSpeed = MAX_YAWSPEED_X_WING;//switch, eventually?  Or read from file?
		diff = AngleNormalize180(SHORT2ANGLE(cmd->angles[YAW]+ps->delta_angles[YAW]) - floor(ps->viewangles[YAW]));

		//clamp the turn rate
		if ( diff > maxYawSpeed )
		{
			cmd->angles[YAW] = ANGLE2SHORT( ps->viewangles[YAW] + maxYawSpeed ) - ps->delta_angles[YAW];
		}
		else if ( diff < -maxYawSpeed )
		{
			cmd->angles[YAW] = ANGLE2SHORT( ps->viewangles[YAW] - maxYawSpeed ) - ps->delta_angles[YAW];
		}
	}
}

extern void SetClientViewAngle( gentity_t *ent, vec3_t angle );
qboolean PM_LockAngles( gentity_t *ent, usercmd_t *ucmd )
{
	if ( ent->client->ps.viewEntity <= 0 || ent->client->ps.viewEntity >= ENTITYNUM_WORLD )
	{//don't clamp angles when looking through a viewEntity
		SetClientViewAngle( ent, ent->client->ps.viewangles );
	}
	ucmd->angles[PITCH] = ANGLE2SHORT( ent->client->ps.viewangles[PITCH] ) - ent->client->ps.delta_angles[PITCH];
	ucmd->angles[YAW] = ANGLE2SHORT( ent->client->ps.viewangles[YAW] ) - ent->client->ps.delta_angles[YAW];
	return qtrue;
}

qboolean PM_AdjustAnglesToGripper( gentity_t *ent, usercmd_t *ucmd )
{//FIXME: make this more generic and have it actually *tell* the client what cmd angles it should be locked at?
	if ( ((ent->client->ps.eFlags&EF_FORCE_GRIPPED)||(ent->client->ps.eFlags&EF_FORCE_DRAINED)) && ent->enemy )
	{
		vec3_t dir, angles;

		VectorSubtract( ent->enemy->currentOrigin, ent->currentOrigin, dir );
		vectoangles( dir, angles );
		angles[PITCH] = AngleNormalize180( angles[PITCH] );
		angles[YAW] = AngleNormalize180( angles[YAW] );
		if ( ent->client->ps.viewEntity <= 0 || ent->client->ps.viewEntity >= ENTITYNUM_WORLD )
		{//don't clamp angles when looking through a viewEntity
			SetClientViewAngle( ent, angles );
		}
		ucmd->angles[PITCH] = ANGLE2SHORT(angles[PITCH]) - ent->client->ps.delta_angles[PITCH];
		ucmd->angles[YAW] = ANGLE2SHORT(angles[YAW]) - ent->client->ps.delta_angles[YAW];
		return qtrue;
	}
	return qfalse;
}

qboolean PM_AdjustAnglesToPuller( gentity_t *ent, gentity_t *puller, usercmd_t *ucmd, qboolean faceAway )
{//FIXME: make this more generic and have it actually *tell* the client what cmd angles it should be locked at?
	vec3_t dir, angles;

	VectorSubtract( puller->currentOrigin, ent->currentOrigin, dir );
	vectoangles( dir, angles );
	angles[PITCH] = AngleNormalize180( angles[PITCH] );
	if ( faceAway )
	{
		angles[YAW] += 180;
	}
	angles[YAW] = AngleNormalize180( angles[YAW] );
	if ( ent->client->ps.viewEntity <= 0 || ent->client->ps.viewEntity >= ENTITYNUM_WORLD )
	{//don't clamp angles when looking through a viewEntity
		SetClientViewAngle( ent, angles );
	}
	ucmd->angles[PITCH] = ANGLE2SHORT(angles[PITCH]) - ent->client->ps.delta_angles[PITCH];
	ucmd->angles[YAW] = ANGLE2SHORT(angles[YAW]) - ent->client->ps.delta_angles[YAW];
	return qtrue;
}

qboolean PM_AdjustAngleForWallRun( gentity_t *ent, usercmd_t *ucmd, qboolean doMove )
{
	if (( ent->client->ps.legsAnim == BOTH_WALL_RUN_RIGHT || ent->client->ps.legsAnim == BOTH_WALL_RUN_LEFT ) && ent->client->ps.legsAnimTimer > 500 )
	{//wall-running and not at end of anim
		//stick to wall, if there is one
		vec3_t	fwd, rt, traceTo, mins = {ent->mins[0],ent->mins[1],0}, maxs = {ent->maxs[0],ent->maxs[1],24}, fwdAngles = {0, ent->client->ps.viewangles[YAW], 0};
		trace_t	trace;
		float	dist, yawAdjust=0.0f;

		AngleVectors( fwdAngles, fwd, rt, NULL );

		if ( ent->client->ps.legsAnim == BOTH_WALL_RUN_RIGHT )
		{
			dist = 128;
			yawAdjust = -90;
		}
		else
		{
			dist = -128;
			yawAdjust = 90;
		}
		VectorMA( ent->currentOrigin, dist, rt, traceTo );
		gi.trace( &trace, ent->currentOrigin, mins, maxs, traceTo, ent->s.number, ent->clipmask, (EG2_Collision)0, 0 );
		if ( trace.fraction < 1.0f
			&& (trace.plane.normal[2] >= 0.0f && trace.plane.normal[2] <= 0.4f) )//&& ent->client->ps.groundEntityNum == ENTITYNUM_NONE )
		{
			trace_t	trace2;
			vec3_t traceTo2;
			vec3_t	wallRunFwd, wallRunAngles = {0};

			wallRunAngles[YAW] = vectoyaw( trace.plane.normal )+yawAdjust;
			AngleVectors( wallRunAngles, wallRunFwd, NULL, NULL );

			VectorMA( ent->currentOrigin, 32, wallRunFwd, traceTo2 );
			gi.trace( &trace2, ent->currentOrigin, mins, maxs, traceTo2, ent->s.number, ent->clipmask, (EG2_Collision)0, 0 );
			if ( trace2.fraction < 1.0f && DotProduct( trace2.plane.normal, wallRunFwd ) <= -0.999f )
			{//wall we can't run on in front of us
				trace.fraction = 1.0f;//just a way to get it to kick us off the wall below
			}
		}
		if ( trace.fraction < 1.0f
			&& (trace.plane.normal[2] >= 0.0f && trace.plane.normal[2] <= 0.4f) )//&& ent->client->ps.groundEntityNum == ENTITYNUM_NONE )
		{//still a vertical wall there
			//FIXME: don't pull around 90 turns
			//FIXME: simulate stepping up steps here, somehow?
			if ( (ent->s.number>=MAX_CLIENTS&&!G_ControlledByPlayer(ent)) || !player_locked )
			{
				if ( ent->client->ps.legsAnim == BOTH_WALL_RUN_RIGHT )
				{
					ucmd->rightmove = 127;
				}
				else
				{
					ucmd->rightmove = -127;
				}
			}
			if ( ucmd->upmove < 0 )
			{
				ucmd->upmove = 0;
			}
			if ( ent->NPC )
			{//invalid now
				VectorClear( ent->client->ps.moveDir );
			}
			//make me face perpendicular to the wall
			ent->client->ps.viewangles[YAW] = vectoyaw( trace.plane.normal )+yawAdjust;
			if ( ent->client->ps.viewEntity <= 0 || ent->client->ps.viewEntity >= ENTITYNUM_WORLD )
			{//don't clamp angles when looking through a viewEntity
				SetClientViewAngle( ent, ent->client->ps.viewangles );
			}
			ucmd->angles[YAW] = ANGLE2SHORT( ent->client->ps.viewangles[YAW] ) - ent->client->ps.delta_angles[YAW];
			if ( (ent->s.number&&!G_ControlledByPlayer(ent)) || !player_locked )
			{
				if ( doMove )
				{
					//push me forward
					float	zVel = ent->client->ps.velocity[2];
					if ( zVel > forceJumpStrength[FORCE_LEVEL_2]/2.0f )
					{
						zVel = forceJumpStrength[FORCE_LEVEL_2]/2.0f;
					}
					//pull me toward the wall
					VectorScale( trace.plane.normal, -128, ent->client->ps.velocity );
					if ( ent->client->ps.legsAnimTimer > 500 )
					{//not at end of anim yet, pushing forward
						//FIXME: or MA?
						float speed = 175;
						if ( ucmd->forwardmove < 0 )
						{//slower
							speed = 100;
						}
						else if ( ucmd->forwardmove > 0 )
						{
							speed = 250;//running speed
						}
						VectorMA( ent->client->ps.velocity, speed, fwd, ent->client->ps.velocity );
					}
					ent->client->ps.velocity[2] = zVel;//preserve z velocity
					//VectorMA( ent->client->ps.velocity, -128, trace.plane.normal, ent->client->ps.velocity );
					//pull me toward the wall, too
					//VectorMA( ent->client->ps.velocity, dist, rt, ent->client->ps.velocity );
				}
			}
			ucmd->forwardmove = 0;
			return qtrue;
		}
		else if ( doMove )
		{//stop it
			if ( ent->client->ps.legsAnim == BOTH_WALL_RUN_RIGHT )
			{
				NPC_SetAnim( ent, SETANIM_BOTH, BOTH_WALL_RUN_RIGHT_STOP, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			}
			else if ( ent->client->ps.legsAnim == BOTH_WALL_RUN_LEFT )
			{
				NPC_SetAnim( ent, SETANIM_BOTH, BOTH_WALL_RUN_LEFT_STOP, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			}
		}
	}
	return qfalse;
}

extern int PM_AnimLength( int index, animNumber_t anim );
qboolean PM_AdjustAnglesForSpinningFlip( gentity_t *ent, usercmd_t *ucmd, qboolean anglesOnly )
{
	vec3_t	newAngles;
	float	animLength, spinStart, spinEnd, spinAmt, spinLength;
	animNumber_t	spinAnim;

	if ( ent->client->ps.legsAnim == BOTH_JUMPFLIPSTABDOWN )
	{
		spinAnim = BOTH_JUMPFLIPSTABDOWN;
		spinStart = 300.0f;//700.0f;
		spinEnd = 1400.0f;
		spinAmt = 180.0f;
	}
	else if ( ent->client->ps.legsAnim == BOTH_JUMPFLIPSLASHDOWN1 )
	{
		spinAnim = BOTH_JUMPFLIPSLASHDOWN1;
		spinStart = 300.0f;//700.0f;//1500.0f;
		spinEnd = 1400.0f;//2300.0f;
		spinAmt = 180.0f;
	}
	else
	{
		if ( !anglesOnly )
		{
			if ( (ent->s.number<MAX_CLIENTS||G_ControlledByPlayer(ent)) )
			{
				cg.overrides.active &= ~CG_OVERRIDE_3RD_PERSON_VOF;
				cg.overrides.thirdPersonVertOffset = 0;
			}
		}
		return qfalse;
	}
	animLength = PM_AnimLength( ent->client->clientInfo.animFileIndex, spinAnim );
	float elapsedTime = (float)(animLength-ent->client->ps.legsAnimTimer);
	//face me
	if ( elapsedTime >= spinStart && elapsedTime <= spinEnd )
	{
		spinLength = spinEnd - spinStart;
		VectorCopy( ent->client->ps.viewangles, newAngles );
		newAngles[YAW] = ent->angle + (spinAmt * (elapsedTime-spinStart) / spinLength);
		if ( ent->client->ps.viewEntity <= 0 || ent->client->ps.viewEntity >= ENTITYNUM_WORLD )
		{//don't clamp angles when looking through a viewEntity
			SetClientViewAngle( ent, newAngles );
		}
		ucmd->angles[PITCH] = ANGLE2SHORT( ent->client->ps.viewangles[PITCH] ) - ent->client->ps.delta_angles[PITCH];
		ucmd->angles[YAW] = ANGLE2SHORT( ent->client->ps.viewangles[YAW] ) - ent->client->ps.delta_angles[YAW];
		if ( anglesOnly )
		{
			return qtrue;
		}
	}
	else if ( anglesOnly )
	{
		return qfalse;
	}
	//push me
	if ( ent->client->ps.legsAnimTimer > 300 )//&& ent->client->ps.groundEntityNum == ENTITYNUM_NONE )
	{//haven't landed or reached end of anim yet
		if ( (ent->s.number>=MAX_CLIENTS&&!G_ControlledByPlayer(ent)) || !player_locked )
		{
			vec3_t pushDir, pushAngles = {0,ent->angle,0};
			AngleVectors( pushAngles, pushDir, NULL, NULL );
			if ( DotProduct( ent->client->ps.velocity, pushDir ) < 100 )
			{
				VectorMA( ent->client->ps.velocity, 10, pushDir, ent->client->ps.velocity );
			}
		}
	}
	//do a dip in the view
	if ( (ent->s.number<MAX_CLIENTS||G_ControlledByPlayer(ent)) )
	{
		float viewDip = 0;
		if ( elapsedTime < animLength/2.0f )
		{//starting anim
			viewDip = (elapsedTime/animLength)*-120.0f;
		}
		else
		{//ending anim
			viewDip = ((animLength-elapsedTime)/animLength)*-120.0f;
		}
		cg.overrides.active |= CG_OVERRIDE_3RD_PERSON_VOF;
		cg.overrides.thirdPersonVertOffset = cg_thirdPersonVertOffset.value+viewDip;
		//pm->ps->viewheight = standheight + viewDip;
	}
	return qtrue;
}

qboolean PM_AdjustAnglesForBackAttack( gentity_t *ent, usercmd_t *ucmd )
{
	if ( (ent->s.number>=MAX_CLIENTS&&!G_ControlledByPlayer(ent)) )
	{
		return qfalse;
	}
	if ( ( ent->client->ps.saberMove == LS_A_BACK || ent->client->ps.saberMove == LS_A_BACK_CR || ent->client->ps.saberMove == LS_A_BACKSTAB )
		&& PM_InAnimForSaberMove( ent->client->ps.torsoAnim, ent->client->ps.saberMove ) )
	{
		if ( ent->client->ps.saberMove != LS_A_BACKSTAB || !ent->enemy || (ent->s.number>=MAX_CLIENTS&&!G_ControlledByPlayer(ent)) )
		{
			if ( ent->client->ps.viewEntity <= 0 || ent->client->ps.viewEntity >= ENTITYNUM_WORLD )
			{//don't clamp angles when looking through a viewEntity
				SetClientViewAngle( ent, ent->client->ps.viewangles );
			}
			ucmd->angles[PITCH] = ANGLE2SHORT( ent->client->ps.viewangles[PITCH] ) - ent->client->ps.delta_angles[PITCH];
			ucmd->angles[YAW] = ANGLE2SHORT( ent->client->ps.viewangles[YAW] ) - ent->client->ps.delta_angles[YAW];
		}
		else
		{//keep player facing away from their enemy
			vec3_t enemyBehindDir;
			VectorSubtract( ent->currentOrigin, ent->enemy->currentOrigin, enemyBehindDir );
			float enemyBehindYaw = AngleNormalize180( vectoyaw( enemyBehindDir ) );
			float yawError = AngleNormalize180( enemyBehindYaw - AngleNormalize180( ent->client->ps.viewangles[YAW] ) );
			if ( yawError > 1 )
			{
				yawError = 1;
			}
			else if ( yawError < -1 )
			{
				yawError = -1;
			}
			ucmd->angles[YAW] = ANGLE2SHORT( AngleNormalize180( ent->client->ps.viewangles[YAW] + yawError ) ) - ent->client->ps.delta_angles[YAW];
			ucmd->angles[PITCH] = ANGLE2SHORT( ent->client->ps.viewangles[PITCH] ) - ent->client->ps.delta_angles[PITCH];
		}
		return qtrue;
	}
	return qfalse;
}

qboolean PM_AdjustAnglesForSaberLock( gentity_t *ent, usercmd_t *ucmd )
{
	if ( ent->client->ps.saberLockTime > level.time )
	{
		if ( ent->client->ps.viewEntity <= 0 || ent->client->ps.viewEntity >= ENTITYNUM_WORLD )
		{//don't clamp angles when looking through a viewEntity
			SetClientViewAngle( ent, ent->client->ps.viewangles );
		}
		ucmd->angles[PITCH] = ANGLE2SHORT( ent->client->ps.viewangles[PITCH] ) - ent->client->ps.delta_angles[PITCH];
		ucmd->angles[YAW] = ANGLE2SHORT( ent->client->ps.viewangles[YAW] ) - ent->client->ps.delta_angles[YAW];
		return qtrue;
	}
	return qfalse;
}

int G_MinGetUpTime( gentity_t *ent )
{
	if ( ent
		&& ent->client
		&& ( ent->client->ps.legsAnim == BOTH_PLAYER_PA_3_FLY
			|| ent->client->ps.legsAnim == BOTH_LK_DL_ST_T_SB_1_L
			|| ent->client->ps.legsAnim == BOTH_RELEASED ) )
	{//special cases
		return 200;
	}
	else if ( ent && ent->client && ent->client->NPC_class == CLASS_ALORA )
	{//alora springs up very quickly from knockdowns!
		return 1000;
	}
	else if ( (ent->s.clientNum < MAX_CLIENTS||G_ControlledByPlayer(ent)) )
	{//player can get up faster based on his/her force jump skill
		int getUpTime = PLAYER_KNOCKDOWN_HOLD_EXTRA_TIME;
		if ( ent->client->ps.forcePowerLevel[FP_LEVITATION] >= FORCE_LEVEL_3 )
		{
			return (getUpTime+400);//750
		}
		else if ( ent->client->ps.forcePowerLevel[FP_LEVITATION] == FORCE_LEVEL_2 )
		{
			return (getUpTime+200);//500
		}
		else if ( ent->client->ps.forcePowerLevel[FP_LEVITATION] == FORCE_LEVEL_1 )
		{
			return (getUpTime+100);//250
		}
		else
		{
			return getUpTime;
		}
	}
	return 200;
}

qboolean PM_AdjustAnglesForKnockdown( gentity_t *ent, usercmd_t *ucmd, qboolean angleClampOnly )
{
	if ( PM_InKnockDown( &ent->client->ps ) )
	{//being knocked down or getting up, can't do anything!
		if ( !angleClampOnly )
		{
			if ( ent->client->ps.legsAnimTimer > G_MinGetUpTime( ent )
				|| (ent->s.number >= MAX_CLIENTS&&!G_ControlledByPlayer(ent)) )
			{//can't get up yet
				ucmd->forwardmove = 0;
				ucmd->rightmove = 0;
			}
			if ( ent->NPC )
			{
				VectorClear( ent->client->ps.moveDir );
			}
			//you can jump up out of a knockdown and you get get up into a crouch from a knockdown
			//ucmd->upmove = 0;
			//if ( !PM_InForceGetUp( &ent->client->ps ) || ent->client->ps.torsoAnimTimer > 800 || ent->s.weapon != WP_SABER )
			if ( ent->health > 0 )
			{//can only attack if you've started a force-getup and are using the saber
				ucmd->buttons = 0;
			}
		}
		if ( !PM_InForceGetUp( &ent->client->ps ) )
		{//can't turn unless in a force getup
			if ( ent->client->ps.viewEntity <= 0 || ent->client->ps.viewEntity >= ENTITYNUM_WORLD )
			{//don't clamp angles when looking through a viewEntity
				SetClientViewAngle( ent, ent->client->ps.viewangles );
			}
			ucmd->angles[PITCH] = ANGLE2SHORT( ent->client->ps.viewangles[PITCH] ) - ent->client->ps.delta_angles[PITCH];
			ucmd->angles[YAW] = ANGLE2SHORT( ent->client->ps.viewangles[YAW] ) - ent->client->ps.delta_angles[YAW];
			return qtrue;
		}
	}
	return qfalse;
}

qboolean PM_AdjustAnglesForDualJumpAttack( gentity_t *ent, usercmd_t *ucmd )
{
	if ( ent->client->ps.viewEntity <= 0 || ent->client->ps.viewEntity >= ENTITYNUM_WORLD )
	{//don't clamp angles when looking through a viewEntity
		SetClientViewAngle( ent, ent->client->ps.viewangles );
	}
	ucmd->angles[PITCH] = ANGLE2SHORT( ent->client->ps.viewangles[PITCH] ) - ent->client->ps.delta_angles[PITCH];
	ucmd->angles[YAW] = ANGLE2SHORT( ent->client->ps.viewangles[YAW] ) - ent->client->ps.delta_angles[YAW];
	return qtrue;
}

qboolean PM_AdjustAnglesForLongJump( gentity_t *ent, usercmd_t *ucmd )
{
	if ( ent->client->ps.viewEntity <= 0 || ent->client->ps.viewEntity >= ENTITYNUM_WORLD )
	{//don't clamp angles when looking through a viewEntity
		SetClientViewAngle( ent, ent->client->ps.viewangles );
	}
	ucmd->angles[PITCH] = ANGLE2SHORT( ent->client->ps.viewangles[PITCH] ) - ent->client->ps.delta_angles[PITCH];
	ucmd->angles[YAW] = ANGLE2SHORT( ent->client->ps.viewangles[YAW] ) - ent->client->ps.delta_angles[YAW];
	return qtrue;
}

qboolean PM_AdjustAnglesForGrapple( gentity_t *ent, usercmd_t *ucmd )
{
	if ( ent->client->ps.viewEntity <= 0 || ent->client->ps.viewEntity >= ENTITYNUM_WORLD )
	{//don't clamp angles when looking through a viewEntity
		SetClientViewAngle( ent, ent->client->ps.viewangles );
	}
	ucmd->angles[PITCH] = ANGLE2SHORT( ent->client->ps.viewangles[PITCH] ) - ent->client->ps.delta_angles[PITCH];
	ucmd->angles[YAW] = ANGLE2SHORT( ent->client->ps.viewangles[YAW] ) - ent->client->ps.delta_angles[YAW];
	return qtrue;
}

qboolean PM_AdjustAngleForWallRunUp( gentity_t *ent, usercmd_t *ucmd, qboolean doMove )
{
	if ( ent->client->ps.legsAnim == BOTH_FORCEWALLRUNFLIP_START )
	{//wall-running up
		//stick to wall, if there is one
		vec3_t	fwd, traceTo, mins = {ent->mins[0],ent->mins[1],0}, maxs = {ent->maxs[0],ent->maxs[1],24}, fwdAngles = {0, ent->client->ps.viewangles[YAW], 0};
		trace_t	trace;
		float	dist = 128;

		AngleVectors( fwdAngles, fwd, NULL, NULL );
		VectorMA( ent->currentOrigin, dist, fwd, traceTo );
		gi.trace( &trace, ent->currentOrigin, mins, maxs, traceTo, ent->s.number, ent->clipmask, (EG2_Collision)0, 0 );
		if ( trace.fraction > 0.5f )
		{//hmm, some room, see if there's a floor right here
			trace_t	trace2;
			vec3_t	top, bottom;
			VectorCopy( trace.endpos, top );
			top[2] += (ent->mins[2]*-1) + 4.0f;
			VectorCopy( top, bottom );
			bottom[2] -= 64.0f;//was 32.0f
			gi.trace( &trace2, top, ent->mins, ent->maxs, bottom, ent->s.number, ent->clipmask, (EG2_Collision)0, 0 );
			if ( !trace2.allsolid
				&& !trace2.startsolid
				&& trace2.fraction < 1.0f
				&& trace2.plane.normal[2] > 0.7f )//slope we can stand on
			{//cool, do the alt-flip and land on whetever it is we just scaled up
				VectorScale( fwd, 100, ent->client->ps.velocity );
				ent->client->ps.velocity[2] += 200;
				NPC_SetAnim( ent, SETANIM_BOTH, BOTH_FORCEWALLRUNFLIP_ALT, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				ent->client->ps.pm_flags |= PMF_JUMP_HELD;
				ent->client->ps.pm_flags |= PMF_JUMPING|PMF_SLOW_MO_FALL;
				ent->client->ps.forcePowersActive |= (1<<FP_LEVITATION);
				G_AddEvent( ent, EV_JUMP, 0 );
				ucmd->upmove = 0;
				return qfalse;
			}
		}
		if ( //ucmd->upmove <= 0 &&
			ent->client->ps.legsAnimTimer > 0
			&& ucmd->forwardmove > 0
			&& trace.fraction < 1.0f
			&& (trace.plane.normal[2] >= 0.0f && trace.plane.normal[2] <= MAX_WALL_RUN_Z_NORMAL) )
		{//still a vertical wall there
			//make sure there's not a ceiling above us!
			trace_t	trace2;
			VectorCopy( ent->currentOrigin, traceTo );
			traceTo[2] += 64;
			gi.trace( &trace2, ent->currentOrigin, mins, maxs, traceTo, ent->s.number, ent->clipmask, (EG2_Collision)0, 0 );
			if ( trace2.fraction < 1.0f )
			{//will hit a ceiling, so force jump-off right now
				//NOTE: hits any entity or clip brush in the way, too, not just architecture!
			}
			else
			{//all clear, keep going
				//FIXME: don't pull around 90 turns
				//FIXME: simulate stepping up steps here, somehow?
				if ( (ent->s.number>=MAX_CLIENTS&&!G_ControlledByPlayer(ent)) || !player_locked )
				{
					ucmd->forwardmove = 127;
				}
				if ( ucmd->upmove < 0 )
				{
					ucmd->upmove = 0;
				}
				if ( ent->NPC )
				{//invalid now
					VectorClear( ent->client->ps.moveDir );
				}
				//make me face the wall
				ent->client->ps.viewangles[YAW] = vectoyaw( trace.plane.normal )+180;
				if ( ent->client->ps.viewEntity <= 0 || ent->client->ps.viewEntity >= ENTITYNUM_WORLD )
				{//don't clamp angles when looking through a viewEntity
					SetClientViewAngle( ent, ent->client->ps.viewangles );
				}
				ucmd->angles[YAW] = ANGLE2SHORT( ent->client->ps.viewangles[YAW] ) - ent->client->ps.delta_angles[YAW];
				if ( (ent->s.number>=MAX_CLIENTS&&!G_ControlledByPlayer(ent)) || !player_locked )
				{
					if ( doMove )
					{
						//pull me toward the wall
						VectorScale( trace.plane.normal, -128, ent->client->ps.velocity );
						//push me up
						if ( ent->client->ps.legsAnimTimer > 200 )
						{//not at end of anim yet
							float speed = 300;
							/*
							if ( ucmd->forwardmove < 0 )
							{//slower
								speed = 100;
							}
							else if ( ucmd->forwardmove > 0 )
							{
								speed = 250;//running speed
							}
							*/
							ent->client->ps.velocity[2] = speed;//preserve z velocity
						}
						//pull me toward the wall
						//VectorMA( ent->client->ps.velocity, -128, trace.plane.normal, ent->client->ps.velocity );
					}
				}
				ucmd->forwardmove = 0;
				return qtrue;
			}
		}
		//failed!
		if ( doMove )
		{//stop it
			VectorScale( fwd, WALL_RUN_UP_BACKFLIP_SPEED, ent->client->ps.velocity );
			ent->client->ps.velocity[2] += 200;
			NPC_SetAnim( ent, SETANIM_BOTH, BOTH_FORCEWALLRUNFLIP_END, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			ent->client->ps.pm_flags |= PMF_JUMP_HELD;
			ent->client->ps.pm_flags |= PMF_JUMPING|PMF_SLOW_MO_FALL;
			ent->client->ps.forcePowersActive |= (1<<FP_LEVITATION);
			G_AddEvent( ent, EV_JUMP, 0 );
			ucmd->upmove = 0;
			//return qtrue;
		}
	}
	return qfalse;
}

float G_ForceWallJumpStrength( void )
{
	return (forceJumpStrength[FORCE_LEVEL_3]/2.5f);
}

qboolean PM_AdjustAngleForWallJump( gentity_t *ent, usercmd_t *ucmd, qboolean doMove )
{
	if ( PM_InReboundJump( ent->client->ps.legsAnim )
		|| PM_InReboundHold( ent->client->ps.legsAnim ) )
	{//hugging wall, getting ready to jump off
		//stick to wall, if there is one
		vec3_t	checkDir, traceTo, mins = {ent->mins[0],ent->mins[1],0}, maxs = {ent->maxs[0],ent->maxs[1],24}, fwdAngles = {0, ent->client->ps.viewangles[YAW], 0};
		trace_t	trace;
		float	dist = 128, yawAdjust;
		switch ( ent->client->ps.legsAnim )
		{
		case BOTH_FORCEWALLREBOUND_RIGHT:
		case BOTH_FORCEWALLHOLD_RIGHT:
			AngleVectors( fwdAngles, NULL, checkDir, NULL );
			yawAdjust = -90;
			break;
		case BOTH_FORCEWALLREBOUND_LEFT:
		case BOTH_FORCEWALLHOLD_LEFT:
			AngleVectors( fwdAngles, NULL, checkDir, NULL );
			VectorScale( checkDir, -1, checkDir );
			yawAdjust = 90;
			break;
		case BOTH_FORCEWALLREBOUND_FORWARD:
		case BOTH_FORCEWALLHOLD_FORWARD:
			AngleVectors( fwdAngles, checkDir, NULL, NULL );
			yawAdjust = 180;
			break;
		case BOTH_FORCEWALLREBOUND_BACK:
		case BOTH_FORCEWALLHOLD_BACK:
			AngleVectors( fwdAngles, checkDir, NULL, NULL );
			VectorScale( checkDir, -1, checkDir );
			yawAdjust = 0;
			break;
		default:
			//WTF???
			return qfalse;
			break;
		}
		if ( g_debugMelee->integer )
		{//uber-skillz
			if ( ucmd->upmove > 0 )
			{//hold on until you let go manually
				if ( PM_InReboundHold( ent->client->ps.legsAnim ) )
				{//keep holding
					if ( ent->client->ps.legsAnimTimer < 150 )
					{
						ent->client->ps.legsAnimTimer = 150;
					}
				}
				else
				{//if got to hold part of anim, play hold anim
					if ( ent->client->ps.legsAnimTimer <= 300 )
					{
						ent->client->ps.SaberDeactivate();
						NPC_SetAnim( ent, SETANIM_BOTH, BOTH_FORCEWALLRELEASE_FORWARD+(ent->client->ps.legsAnim-BOTH_FORCEWALLHOLD_FORWARD), SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
						ent->client->ps.legsAnimTimer = ent->client->ps.torsoAnimTimer = 150;
					}
				}
			}
		}
		VectorMA( ent->currentOrigin, dist, checkDir, traceTo );
		gi.trace( &trace, ent->currentOrigin, mins, maxs, traceTo, ent->s.number, ent->clipmask, (EG2_Collision)0, 0 );
		if ( //ucmd->upmove <= 0 &&
			ent->client->ps.legsAnimTimer > 100 &&
			trace.fraction < 1.0f && fabs(trace.plane.normal[2]) <= MAX_WALL_GRAB_SLOPE )
		{//still a vertical wall there
			//FIXME: don't pull around 90 turns
			/*
			if ( (ent->s.number>=MAX_CLIENTS&&!G_ControlledByPlayer(ent)) || !player_locked )
			{
				ucmd->forwardmove = 127;
			}
			*/
			if ( ucmd->upmove < 0 )
			{
				ucmd->upmove = 0;
			}
			if ( ent->NPC )
			{//invalid now
				VectorClear( ent->client->ps.moveDir );
			}
			//align me to the wall
			ent->client->ps.viewangles[YAW] = vectoyaw( trace.plane.normal )+yawAdjust;
			if ( ent->client->ps.viewEntity <= 0 || ent->client->ps.viewEntity >= ENTITYNUM_WORLD )
			{//don't clamp angles when looking through a viewEntity
				SetClientViewAngle( ent, ent->client->ps.viewangles );
			}
			ucmd->angles[YAW] = ANGLE2SHORT( ent->client->ps.viewangles[YAW] ) - ent->client->ps.delta_angles[YAW];
			if ( (ent->s.number>=MAX_CLIENTS&&!G_ControlledByPlayer(ent)) || !player_locked )
			{
				if ( doMove )
				{
					//pull me toward the wall
					VectorScale( trace.plane.normal, -128, ent->client->ps.velocity );
				}
			}
			ucmd->upmove = 0;
			ent->client->ps.pm_flags |= PMF_STUCK_TO_WALL;
			return qtrue;
		}
		else if ( doMove
			&& (ent->client->ps.pm_flags&PMF_STUCK_TO_WALL))
		{//jump off
			//push off of it!
			ent->client->ps.pm_flags &= ~PMF_STUCK_TO_WALL;
			ent->client->ps.velocity[0] = ent->client->ps.velocity[1] = 0;
			VectorScale( checkDir, -JUMP_OFF_WALL_SPEED, ent->client->ps.velocity );
			ent->client->ps.velocity[2] = G_ForceWallJumpStrength();
			ent->client->ps.pm_flags |= PMF_JUMPING|PMF_JUMP_HELD;
			G_SoundOnEnt( ent, CHAN_BODY, "sound/weapons/force/jump.wav" );
			ent->client->ps.forcePowersActive |= (1<<FP_LEVITATION);
			WP_ForcePowerDrain( ent, FP_LEVITATION, 10 );
			if ( PM_InReboundHold( ent->client->ps.legsAnim ) )
			{//if was in hold pose, release now
				NPC_SetAnim( ent, SETANIM_BOTH, BOTH_FORCEWALLRELEASE_FORWARD+(ent->client->ps.legsAnim-BOTH_FORCEWALLHOLD_FORWARD), SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			}
			//no control for half a second
			ent->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
			ent->client->ps.pm_time = 500;
			ucmd->forwardmove = 0;
			ucmd->rightmove = 0;
			ucmd->upmove = 0;
			//return qtrue;
		}
	}
	ent->client->ps.pm_flags &= ~PMF_STUCK_TO_WALL;
	return qfalse;
}

qboolean PM_AdjustAnglesForBFKick( gentity_t *self, usercmd_t *ucmd, vec3_t fwdAngs, qboolean aimFront )
{
	//Auto-aim the player at the ent in front/back of them
	//FIXME: camera angle should always be in front/behind me for the 2 kicks
	//			(to hide how far away the two entities really are)
	//FIXME: don't let the people we're auto-aiming at move?
	gentity_t	*ent;
	gentity_t	*entityList[MAX_GENTITIES];
	vec3_t		mins, maxs;
	int			i, e;
	int			radius = ((self->maxs[0]*1.5f)+(self->maxs[0]*1.5f)+STAFF_KICK_RANGE+24.0f);//a little wide on purpose
	vec3_t		center, vec2Ent, v_fwd;
	float		distToEnt, bestDist = Q3_INFINITE;
	float		dot, bestDot = -1.1f;
	float		bestYaw = Q3_INFINITE;

	AngleVectors( fwdAngs, v_fwd, NULL, NULL );

	VectorCopy( self->currentOrigin, center );

	for ( i = 0 ; i < 3 ; i++ )
	{
		mins[i] = center[i] - radius;
		maxs[i] = center[i] + radius;
	}

	int numListedEntities = gi.EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( e = 0 ; e < numListedEntities ; e++ )
	{
		ent = entityList[ e ];

		if (ent == self)
			continue;
		if (ent->owner == self)
			continue;
		if ( !(ent->inuse) )
			continue;
		//not a client?
		if ( !ent->client )
			continue;
		//ally?
		if ( ent->client->playerTeam == self->client->playerTeam )
			continue;
		//on the ground
		if ( PM_InKnockDown( &ent->client->ps ) )
			continue;
		//dead?
		if ( ent->health <= 0 )
		{
			if ( level.time - ent->s.time > 2000 )
			{//died more than 2 seconds ago, forget him
				continue;
			}
		}
		//too far?
		VectorSubtract( ent->currentOrigin, center, vec2Ent );
		distToEnt = VectorNormalize( vec2Ent );
		if ( distToEnt > radius )
			continue;

		if ( !aimFront )
		{//aim away from them
			VectorScale( vec2Ent, -1, vec2Ent );
		}
		dot = DotProduct( vec2Ent, v_fwd );
		if ( dot < 0.0f )
		{//never turn all the way around
			continue;
		}
		if ( dot > bestDot || ((bestDot-dot<0.25f)&&distToEnt-bestDist>8.0f) )
		{//more in front... OR: still relatively close to in front and significantly closer
			bestDot = dot;
			bestDist = distToEnt;
			bestYaw = vectoyaw( vec2Ent );
		}
	}
	if ( bestYaw != Q3_INFINITE && bestYaw != fwdAngs[YAW] )
	{//aim us at them
		AngleNormalize180( bestYaw );
		AngleNormalize180( fwdAngs[YAW] );
		float angDiff = AngleSubtract( bestYaw, fwdAngs[YAW] );
		AngleNormalize180( angDiff );
		if ( fabs(angDiff) <= 3.0f )
		{
			self->client->ps.viewangles[YAW] = bestYaw;
		}
		else if ( angDiff > 0.0f )
		{//more than 3 degrees higher
			self->client->ps.viewangles[YAW] += 3.0f;
		}
		else
		{//must be more than 3 less than
			self->client->ps.viewangles[YAW] -= 3.0f;
		}
		if ( self->client->ps.viewEntity <= 0 || self->client->ps.viewEntity >= ENTITYNUM_WORLD )
		{//don't clamp angles when looking through a viewEntity
			SetClientViewAngle( self, self->client->ps.viewangles );
		}
		ucmd->angles[YAW] = ANGLE2SHORT( self->client->ps.viewangles[YAW] ) - self->client->ps.delta_angles[YAW];
		return qtrue;
	}
	else
	{//lock these angles
		if ( self->client->ps.viewEntity <= 0 || self->client->ps.viewEntity >= ENTITYNUM_WORLD )
		{//don't clamp angles when looking through a viewEntity
			SetClientViewAngle( self, self->client->ps.viewangles );
		}
		ucmd->angles[YAW] = ANGLE2SHORT( self->client->ps.viewangles[YAW] ) - self->client->ps.delta_angles[YAW];
	}
	return qtrue;
}

qboolean PM_AdjustAnglesForStabDown( gentity_t *ent, usercmd_t *ucmd )
{
	if ( PM_StabDownAnim( ent->client->ps.torsoAnim )
		&& ent->client->ps.torsoAnimTimer )
	{//lock our angles
		ucmd->forwardmove = ucmd->rightmove = ucmd->upmove = 0;
		float elapsedTime = PM_AnimLength( ent->client->clientInfo.animFileIndex, (animNumber_t)ent->client->ps.torsoAnim ) - ent->client->ps.torsoAnimTimer;
		//FIXME: scale forwardmove by dist from enemy - none if right next to him (so we don't slide off!)
		if ( ent->enemy )
		{
			float dist2Enemy = DistanceHorizontal( ent->enemy->currentOrigin, ent->currentOrigin );
			if ( dist2Enemy > (ent->enemy->maxs[0]*1.5f)+(ent->maxs[0]*1.5f) )
			{
				ent->client->ps.speed = dist2Enemy*2.0f;
			}
			else
			{
				ent->client->ps.speed = 0;
			}
		}
		else
		{
			ent->client->ps.speed = 150;
		}
		switch ( ent->client->ps.legsAnim )
		{
		case BOTH_STABDOWN:
			if ( elapsedTime >= 300 && elapsedTime < 900 )
			{//push forward?
				//FIXME: speed!
				ucmd->forwardmove = 127;
			}
			break;
		case BOTH_STABDOWN_STAFF:
			if ( elapsedTime > 400 && elapsedTime < 950 )
			{//push forward?
				//FIXME: speed!
				ucmd->forwardmove = 127;
			}
			break;
		case BOTH_STABDOWN_DUAL:
			if ( elapsedTime >= 300 && elapsedTime < 900 )
			{//push forward?
				//FIXME: speed!
				ucmd->forwardmove = 127;
			}
			break;
		}
		VectorClear( ent->client->ps.moveDir );

		if ( ent->enemy//still have a valid enemy
			&& ent->enemy->client//who is a client
			&& (PM_InKnockDownNoGetup( &ent->enemy->client->ps ) //enemy still on ground
				|| PM_InGetUpNoRoll( &ent->enemy->client->ps ) ) )//or getting straight up
		{//aim at the enemy
			vec3_t enemyDir;
			VectorSubtract( ent->enemy->currentOrigin, ent->currentOrigin, enemyDir );
			float enemyYaw = AngleNormalize180( vectoyaw( enemyDir ) );
			float yawError = AngleNormalize180( enemyYaw - AngleNormalize180( ent->client->ps.viewangles[YAW] ) );
			if ( yawError > 1 )
			{
				yawError = 1;
			}
			else if ( yawError < -1 )
			{
				yawError = -1;
			}
			ucmd->angles[YAW] = ANGLE2SHORT( AngleNormalize180( ent->client->ps.viewangles[YAW] + yawError ) ) - ent->client->ps.delta_angles[YAW];
			ucmd->angles[PITCH] = ANGLE2SHORT( ent->client->ps.viewangles[PITCH] ) - ent->client->ps.delta_angles[PITCH];
		}
		else
		{//can't turn
			if ( ent->client->ps.viewEntity <= 0 || ent->client->ps.viewEntity >= ENTITYNUM_WORLD )
			{//don't clamp angles when looking through a viewEntity
				SetClientViewAngle( ent, ent->client->ps.viewangles );
			}
			ucmd->angles[PITCH] = ANGLE2SHORT( ent->client->ps.viewangles[PITCH] ) - ent->client->ps.delta_angles[PITCH];
			ucmd->angles[YAW] = ANGLE2SHORT( ent->client->ps.viewangles[YAW] ) - ent->client->ps.delta_angles[YAW];
		}
		return qtrue;
	}
	return qfalse;
}

qboolean PM_AdjustAnglesForSpinProtect( gentity_t *ent, usercmd_t *ucmd )
{
	if ( ent->client->ps.torsoAnim == BOTH_A6_SABERPROTECT )
	{//in the dual spin thing
		if ( ent->client->ps.torsoAnimTimer )
		{//flatten and lock our angles, pull back camera
			//FIXME: lerp this
			ent->client->ps.viewangles[PITCH] = 0;
			if ( ent->client->ps.viewEntity <= 0 || ent->client->ps.viewEntity >= ENTITYNUM_WORLD )
			{//don't clamp angles when looking through a viewEntity
				SetClientViewAngle( ent, ent->client->ps.viewangles );
			}
			ucmd->angles[PITCH] = ANGLE2SHORT( ent->client->ps.viewangles[PITCH] ) - ent->client->ps.delta_angles[PITCH];
			ucmd->angles[YAW] = ANGLE2SHORT( ent->client->ps.viewangles[YAW] ) - ent->client->ps.delta_angles[YAW];
			return qtrue;
		}
	}
	return qfalse;
}

qboolean PM_AdjustAnglesForWallRunUpFlipAlt( gentity_t *ent, usercmd_t *ucmd )
{
	if ( ent->client->ps.viewEntity <= 0 || ent->client->ps.viewEntity >= ENTITYNUM_WORLD )
	{//don't clamp angles when looking through a viewEntity
		SetClientViewAngle( ent, ent->client->ps.viewangles );
	}
	ucmd->angles[PITCH] = ANGLE2SHORT( ent->client->ps.viewangles[PITCH] ) - ent->client->ps.delta_angles[PITCH];
	ucmd->angles[YAW] = ANGLE2SHORT( ent->client->ps.viewangles[YAW] ) - ent->client->ps.delta_angles[YAW];
	return qtrue;
}

qboolean PM_AdjustAnglesForHeldByMonster( gentity_t *ent, gentity_t *monster, usercmd_t *ucmd )
{
	vec3_t	newViewAngles;
	if ( !monster || !monster->client )
	{
		return qfalse;
	}
	VectorScale( monster->client->ps.viewangles, -1, newViewAngles );
	if ( ent->client->ps.viewEntity <= 0 || ent->client->ps.viewEntity >= ENTITYNUM_WORLD )
	{//don't clamp angles when looking through a viewEntity
		SetClientViewAngle( ent, newViewAngles );
	}
	ucmd->angles[PITCH] = ANGLE2SHORT( newViewAngles[PITCH] ) - ent->client->ps.delta_angles[PITCH];
	ucmd->angles[YAW] = ANGLE2SHORT( newViewAngles[YAW] ) - ent->client->ps.delta_angles[YAW];
	return qtrue;
}

qboolean G_OkayToLean( playerState_t *ps, usercmd_t *cmd, qboolean interruptOkay )
{
	if ( (ps->clientNum < MAX_CLIENTS||G_ControlledByPlayer(&g_entities[ps->clientNum]))//player
		&& ps->groundEntityNum != ENTITYNUM_NONE//on ground
		&& ( (interruptOkay//okay to interrupt a lean
				&& PM_DodgeAnim( ps->torsoAnim ) )//already leaning
			||
			 (!ps->weaponTime//not attacking or being prevented from attacking
				&& !ps->legsAnimTimer//not in any held legs anim
				&& !ps->torsoAnimTimer) //not in any held torso anim
			)
		&& !(cmd->buttons&(BUTTON_ATTACK|BUTTON_ALT_ATTACK|BUTTON_FORCE_LIGHTNING|BUTTON_USE_FORCE|BUTTON_FORCE_DRAIN|BUTTON_FORCEGRIP))//not trying to attack
		//&& (ps->forcePowersActive&(1<<FP_SPEED))
		&& VectorCompare( ps->velocity, vec3_origin )//not moving
		&& !cg_usingInFrontOf )//use button wouldn't be used for anything else
	{
		return qtrue;
	}
	return qfalse;
}
/*
================
PM_UpdateViewAngles

This can be used as another entry point when only the viewangles
are being updated isntead of a full move

//FIXME: Now that they pmove twice per think, they snap-look really fast
================
*/
void PM_UpdateViewAngles( playerState_t *ps, usercmd_t *cmd, gentity_t *gent )
{
	short		temp;
	float		rootPitch = 0, pitchMin=-75, pitchMax=75, yawMin=0, yawMax=0, lockedYawValue = 0;	//just to shut up warnings
	int			i;
	vec3_t		start, end, tmins, tmaxs, right;
	trace_t		trace;
	qboolean	lockedYaw = qfalse/*, clamped = qfalse*/;

	if ( ps->pm_type == PM_INTERMISSION )
	{
		return;		// no view changes at all
	}

	//TEMP
#if 0 //rww 12/23/02 - I'm disabling this for now, I'm going to try to make it work with my new rag stuff
	if ( gent != NULL )
	{
		PM_IKUpdate( gent );
	}
#endif

	if ( ps->pm_type != PM_SPECTATOR && ps->stats[STAT_HEALTH] <= 0 )
	{
		return;		// no view changes at all
	}

//	if ( player_locked )
//	{//can't turn
//		return;
//	}
	if ( ps->clientNum != 0 && gent != NULL && gent->client != NULL )
	{
		if(gent->client->renderInfo.renderFlags & RF_LOCKEDANGLE)
		{
			pitchMin = 0 - gent->client->renderInfo.headPitchRangeUp - gent->client->renderInfo.torsoPitchRangeUp;
			pitchMax = gent->client->renderInfo.headPitchRangeDown + gent->client->renderInfo.torsoPitchRangeDown;

			yawMin = 0 - gent->client->renderInfo.headYawRangeLeft - gent->client->renderInfo.torsoYawRangeLeft;
			yawMax = gent->client->renderInfo.headYawRangeRight + gent->client->renderInfo.torsoYawRangeRight;

			lockedYaw = qtrue;
			lockedYawValue = gent->client->renderInfo.lockYaw;
		}
		else
		{
			pitchMin = -gent->client->renderInfo.headPitchRangeUp-gent->client->renderInfo.torsoPitchRangeUp;
			pitchMax = gent->client->renderInfo.headPitchRangeDown+gent->client->renderInfo.torsoPitchRangeDown;
		}
	}

	if ( ps->eFlags & EF_LOCKED_TO_WEAPON )
	{
		// Emplaced guns have different pitch capabilities
		if ( gent && gent->owner && gent->owner->e_UseFunc == useF_eweb_use )
		{
			pitchMin = -15;
			pitchMax = 10;
			/*
			lockedYawValue = gent->owner->s.angles[YAW];
			yawMax = gent->owner->s.origin2[0];
			yawMin = yawMax *-1;
			lockedYaw = qtrue;
			*/
		}
		else
		{
			pitchMin = -35;
			pitchMax = 30;
		}
	}

	Vehicle_t *pVeh = NULL;

	// If we're a vehicle, or we're riding a vehicle...?
	if ( gent && gent->client && gent->client->NPC_class == CLASS_VEHICLE && gent->m_pVehicle)
	{
		pVeh = gent->m_pVehicle;
		// If we're a vehicle...
		if (pVeh->m_pVehicleInfo->Inhabited(pVeh) || pVeh->m_iBoarding!=0 || pVeh->m_pVehicleInfo->type!=VH_ANIMAL)
		{
			lockedYawValue = pVeh->m_vOrientation[YAW];
			yawMax = yawMin = 0;
			rootPitch = pVeh->m_vOrientation[PITCH];//???  what if goes over 90 when add the min/max?
			pitchMax = 0.0f;//pVeh->m_pVehicleInfo->pitchLimit;
			pitchMin = 0.0f;//-pitchMax;
			lockedYaw = qtrue;
		}
		// If we're riding a vehicle...
		else if ( (pVeh = G_IsRidingVehicle( gent ) ) != NULL )
		{
			if ( pVeh->m_pVehicleInfo->type != VH_ANIMAL )
			{//animals just turn normally, no clamping
				lockedYawValue = 0;//gent->owner->client->ps.vehicleAngles[YAW];
				lockedYaw = qtrue;
				yawMax = pVeh->m_pVehicleInfo->lookYaw;
				yawMin = -yawMax;
				if ( pVeh->m_pVehicleInfo->type == VH_FIGHTER )
				{
					rootPitch = pVeh->m_vOrientation[PITCH];//gent->owner->client->ps.vehicleAngles[PITCH];//???  what if goes over 90 when add the min/max?
					pitchMax = pVeh->m_pVehicleInfo->pitchLimit;
					pitchMin = -pitchMax;
				}
				else
				{
					rootPitch = 0;//gent->owner->client->ps.vehicleAngles[PITCH];//???  what if goes over 90 when add the min/max?
					pitchMax = pVeh->m_pVehicleInfo->lookPitch;
					pitchMin = -pitchMax;
				}
			}
		}

		/*if ( vehIndex != -1 )
		{
			//FIXME: read from NPCs.cfg or vehicles.cfg?
			lockedYaw = qtrue;
		}*/
	}

	const short pitchClampMin = ANGLE2SHORT(rootPitch+pitchMin);
	const short pitchClampMax = ANGLE2SHORT(rootPitch+pitchMax);
	const short yawClampMin = ANGLE2SHORT(lockedYawValue+yawMin);
	const short yawClampMax = ANGLE2SHORT(lockedYawValue+yawMax);

	// circularly clamp the angles with deltas
	for (i=0 ; i<3 ; i++)
	{
		temp = cmd->angles[i] + ps->delta_angles[i];
		if ( i == PITCH )
		{
			//FIXME get this limit from the NPCs stats?
			// don't let the player look up or down more than 90 degrees
			if ( temp > pitchClampMax )
			{
				ps->delta_angles[i] = (pitchClampMax - cmd->angles[i]) & 0xffff;	//& clamp to short
				temp = pitchClampMax;
				//clamped = qtrue;
			}
			else if ( temp < pitchClampMin )
			{
				ps->delta_angles[i] = (pitchClampMin - cmd->angles[i]) & 0xffff;	//& clamp to short
				temp = pitchClampMin;
				//clamped = qtrue;
			}
		}
		/*
		if ( i == ROLL && ps->vehicleIndex != VEHICLE_NONE )
		{
			if ( temp > pitchClampMax )
			{
				ps->delta_angles[i] = (pitchClampMax - cmd->angles[i]) & 0xffff;
				temp = pitchClampMax;
			}
			else if ( temp < pitchClampMin )
			{
				ps->delta_angles[i] = (pitchClampMin - cmd->angles[i]) & 0xffff;
				temp = pitchClampMin;
			}
		}
		*/
		//FIXME: Are we losing precision here?  Is this why it jitters?
		/*
		if ( i == YAW && lockedYaw )
		{
			float multiplier = 1.0f;
			float newYaw = SHORT2ANGLE(temp);
			lockedYawValue = AngleNormalize180( lockedYawValue );
			float yawDiff = AngleNormalize180( newYaw ) - lockedYawValue;
			if ( yawDiff > 180 )
			{
				multiplier = -1.0f;
				yawDiff = 360 - yawDiff;
			}
			else if ( yawDiff < -180 )
			{
				//multiplier = -1.0f;
				yawDiff = 360 + yawDiff;
			}
			// don't let the player look left or right more than the clamp, if any
			if ( yawDiff > yawMax )
			{
				//clamped = qtrue;
				ps->viewangles[i] = AngleNormalize180( lockedYawValue+((yawMax-2)*multiplier) );
			}
			else if ( yawDiff < yawMin )
			{
				//clamped = qtrue;
				ps->viewangles[i] = AngleNormalize180( lockedYawValue+((yawMin+2)*multiplier) );
			}
			else
			{
				ps->viewangles[i] = newYaw;
			}
		}
		*/
		if ( i == YAW && lockedYaw )
		{
			//FIXME get this limit from the NPCs stats?
			// don't let the player look up or down more than 90 degrees
			if ( temp > yawClampMax )
			{
				ps->delta_angles[i] = (yawClampMax - cmd->angles[i]) & 0xffff;	//& clamp to short
				temp = yawClampMax;
				//clamped = qtrue;
			}
			else if ( temp < yawClampMin )
			{
				ps->delta_angles[i] = (yawClampMin - cmd->angles[i]) & 0xffff;	//& clamp to short
				temp = yawClampMin;
				//clamped = qtrue;
			}
			ps->viewangles[i] = SHORT2ANGLE(temp);
		}
		else
		{
			ps->viewangles[i] = SHORT2ANGLE(temp);
		}
	}

	if ( gent )
	{	//only in the real pmove
		if ( (cmd->buttons & BUTTON_USE) )
		{//check leaning
			if ( cg.renderingThirdPerson )
			{//third person lean
				if ( G_OkayToLean( ps, cmd, qtrue )
					&& (cmd->rightmove || (cmd->forwardmove && g_debugMelee->integer ) ) )//pushing a direction
				{
					int anim = -1;
					if ( cmd->rightmove > 0 )
					{//lean right
						if ( cmd->forwardmove > 0 )
						{//lean forward right
							if ( ps->torsoAnim == BOTH_DODGE_HOLD_FR )
							{
								anim = ps->torsoAnim;
							}
							else
							{
								anim = BOTH_DODGE_FR;
							}
						}
						else if ( cmd->forwardmove < 0 )
						{//lean backward right
							if ( ps->torsoAnim == BOTH_DODGE_HOLD_BR )
							{
								anim = ps->torsoAnim;
							}
							else
							{
								anim = BOTH_DODGE_BR;
							}
						}
						else
						{//lean right
							if ( ps->torsoAnim == BOTH_DODGE_HOLD_R )
							{
								anim = ps->torsoAnim;
							}
							else
							{
								anim = BOTH_DODGE_R;
							}
						}
					}
					else if ( cmd->rightmove < 0 )
					{//lean left
						if ( cmd->forwardmove > 0 )
						{//lean forward left
							if ( ps->torsoAnim == BOTH_DODGE_HOLD_FL )
							{
								anim = ps->torsoAnim;
							}
							else
							{
								anim = BOTH_DODGE_FL;
							}
						}
						else if ( cmd->forwardmove < 0 )
						{//lean backward left
							if ( ps->torsoAnim == BOTH_DODGE_HOLD_BL )
							{
								anim = ps->torsoAnim;
							}
							else
							{
								anim = BOTH_DODGE_BL;
							}
						}
						else
						{//lean left
							if ( ps->torsoAnim == BOTH_DODGE_HOLD_L )
							{
								anim = ps->torsoAnim;
							}
							else
							{
								anim = BOTH_DODGE_L;
							}
						}
					}
					else
					{//not pressing either side
						if ( cmd->forwardmove > 0 )
						{//lean forward
							if ( PM_DodgeAnim( ps->torsoAnim ) )
							{
								anim = ps->torsoAnim;
							}
							else if ( Q_irand( 0, 1 ) )
							{
								anim = BOTH_DODGE_FL;
							}
							else
							{
								anim = BOTH_DODGE_FR;
							}
						}
						else if ( cmd->forwardmove < 0 )
						{//lean backward
							if ( PM_DodgeAnim( ps->torsoAnim ) )
							{
								anim = ps->torsoAnim;
							}
							else if ( Q_irand( 0, 1 ) )
							{
								anim = BOTH_DODGE_BL;
							}
							else
							{
								anim = BOTH_DODGE_BR;
							}
						}
					}
					if ( anim != -1 )
					{
						int extraHoldTime = 0;
						if ( PM_DodgeAnim( ps->torsoAnim )
							&& !PM_DodgeHoldAnim( ps->torsoAnim ) )
						{//already in a dodge
							//use the hold pose, don't start it all over again
							anim = BOTH_DODGE_HOLD_FL+(anim-BOTH_DODGE_FL);
							extraHoldTime = 200;
						}
						if ( anim == pm->ps->torsoAnim )
						{
							if ( pm->ps->torsoAnimTimer < 100 )
							{
								pm->ps->torsoAnimTimer = 100;
							}
						}
						else
						{
							NPC_SetAnim( gent, SETANIM_TORSO, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
						}
						if ( extraHoldTime && ps->torsoAnimTimer < extraHoldTime )
						{
							ps->torsoAnimTimer += extraHoldTime;
						}
						if ( ps->groundEntityNum != ENTITYNUM_NONE
							&& !cmd->upmove )
						{
							NPC_SetAnim( gent, SETANIM_LEGS, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
							ps->legsAnimTimer = ps->torsoAnimTimer;
						}
						else
						{
							NPC_SetAnim( gent, SETANIM_LEGS, anim, SETANIM_FLAG_NORMAL );
						}
						ps->weaponTime = ps->torsoAnimTimer;
						ps->leanStopDebounceTime = ceil((float)ps->torsoAnimTimer/50.0f);//20;
					}
				}
			}
			else if ( (!cg.renderingThirdPerson||cg.zoomMode) && cmd->rightmove != 0 && !cmd->forwardmove && cmd->upmove <= 0 )
			{//Only lean if holding use button, strafing and not moving forward or back and not jumping
				int leanofs = 0;
				vec3_t	viewangles;

				if ( cmd->rightmove > 0 )
				{
					/*
					if( pm->ps->legsAnim != LEGS_LEAN_RIGHT1)
					{
					PM_SetAnim(pm, SETANIM_LEGS, LEGS_LEAN_RIGHT1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
					}
					pm->ps->legsAnimTimer = 500;//Force it to hold the anim for at least half a sec
					*/

					if ( ps->leanofs <= 28 )
					{
						leanofs = ps->leanofs + 4;
					}
					else
					{
						leanofs = 32;
					}
				}
				else
				{
					/*
					if ( pm->ps->legsAnim != LEGS_LEAN_LEFT1 )
					{
					PM_SetAnim(pm, SETANIM_LEGS, LEGS_LEAN_LEFT1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
					}
					pm->ps->legsAnimTimer = 500;//Force it to hold the anim for at least half a sec
					*/

					if ( ps->leanofs >= -28 )
					{
						leanofs = ps->leanofs - 4;
					}
					else
					{
						leanofs = -32;
					}
				}

				VectorCopy( ps->origin, start );
				start[2] += ps->viewheight;
				VectorCopy( ps->viewangles, viewangles );
				viewangles[ROLL] = 0;
				AngleVectors( ps->viewangles, NULL, right, NULL );
				VectorNormalize( right );
				right[2] = (leanofs<0)?0.25:-0.25;
				VectorMA( start, leanofs, right, end );
				VectorSet( tmins, -8, -8, -4 );
				VectorSet( tmaxs, 8, 8, 4 );
				//if we don't trace EVERY frame, can TURN while leaning and
				//end up leaning into solid architecture (sigh)
				gi.trace( &trace, start, tmins, tmaxs, end, gent->s.number, MASK_PLAYERSOLID, (EG2_Collision)0, 0 );

				ps->leanofs = floor((float)leanofs * trace.fraction);

				ps->leanStopDebounceTime = 20;
			}
			else
			{
				if ( (cmd->forwardmove || cmd->upmove > 0) )
				{
					if( ( pm->ps->legsAnim == LEGS_LEAN_RIGHT1) ||
						( pm->ps->legsAnim == LEGS_LEAN_LEFT1) )
					{
						pm->ps->legsAnimTimer = 0;//Force it to stop the anim
					}

					if ( ps->leanofs > 0 )
					{
						ps->leanofs-=4;
						if ( ps->leanofs < 0 )
						{
							ps->leanofs = 0;
						}
					}
					else if ( ps->leanofs < 0 )
					{
						ps->leanofs+=4;
						if ( ps->leanofs > 0 )
						{
							ps->leanofs = 0;
						}
					}
				}

			}
		}
		else//BUTTON_USE
		{
			if ( ps->leanofs > 0 )
			{
				ps->leanofs-=4;
				if ( ps->leanofs < 0 )
				{
					ps->leanofs = 0;
				}
			}
			else if ( ps->leanofs < 0 )
			{
				ps->leanofs+=4;
				if ( ps->leanofs > 0 )
				{
					ps->leanofs = 0;
				}
			}
		}
	}

	if ( ps->leanStopDebounceTime )
	{
		ps->leanStopDebounceTime -= 1;
		cmd->rightmove = 0;
		cmd->buttons &= ~BUTTON_USE;
	}
}
