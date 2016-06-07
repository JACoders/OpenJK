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

// define GAME_INCLUDE so that g_public.h does not define the
// short, server-visible gclient_t and gentity_t structures,
// because we define the full size ones in this file
#define GAME_INCLUDE
#include "../../code/qcommon/q_shared.h"
#include "../cgame/cg_local.h"
#include "b_local.h"
#include "g_local.h"
#include "g_shared.h"
#include "bg_local.h"
#include "anims.h"
#include "wp_saber.h"

extern qboolean PM_InAnimForSaberMove( int anim, int saberMove );
extern qboolean PM_InForceGetUp( playerState_t *ps );
extern qboolean PM_InKnockDown( playerState_t *ps );

extern qboolean		player_locked;
extern pmove_t		*pm;
extern pml_t		pml;

void BG_G2SetBoneAngles( centity_t *cent, gentity_t *gent, int boneIndex, const vec3_t angles, const int flags,
							 const Eorientations up, const Eorientations left, const Eorientations forward, qhandle_t *modelList )
{
	if (boneIndex!=-1)
	{
		gi.G2API_SetBoneAnglesIndex( &cent->gent->ghoul2[0], boneIndex, angles, flags, up, left, forward, modelList, 0, 0 ); 
	}
}

#define	MAX_YAWSPEED_X_WING		1
#define	MAX_PITCHSPEED_X_WING	1
void PM_ScaleUcmd( playerState_t *ps, usercmd_t *cmd, gentity_t *gent )
{
	if ( ps->vehicleModel != 0 )
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
qboolean PM_AdjustAnglesToGripper( gentity_t *ent, usercmd_t *ucmd )
{//FIXME: make this more generic and have it actually *tell* the client what cmd angles it should be locked at?
	if ( (ent->client->ps.eFlags&EF_FORCE_GRIPPED) && ent->enemy )
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

qboolean PM_AdjustAngleForWallRun( gentity_t *ent, usercmd_t *ucmd, qboolean doMove )
{
	if (( ent->client->ps.legsAnim == BOTH_WALL_RUN_RIGHT || ent->client->ps.legsAnim == BOTH_WALL_RUN_LEFT ) && ent->client->ps.legsAnimTimer > 500 )
	{//wall-running and not at end of anim
		//stick to wall, if there is one
		vec3_t	rt, traceTo, mins = {ent->mins[0],ent->mins[1],0}, maxs = {ent->maxs[0],ent->maxs[1],24}, fwdAngles = {0, ent->client->ps.viewangles[YAW], 0};		
		trace_t	trace;
		float	dist, yawAdjust;

		AngleVectors( fwdAngles, NULL, rt, NULL );
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
		gi.trace( &trace, ent->currentOrigin, mins, maxs, traceTo, ent->s.number, ent->clipmask, G2_NOCOLLIDE, 0  );
		if ( trace.fraction < 1.0f && trace.plane.normal[2] == 0.0f )
		{//still a vertical wall there
			//FIXME: don't pull around 90 turns
			//FIXME: simulate stepping up steps here, somehow?
			if ( ent->s.number || !player_locked )
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
			if ( ent->s.number || !player_locked )
			{
				if ( doMove )
				{
					//push me forward
					vec3_t	fwd;
					float	zVel = ent->client->ps.velocity[2];
					if ( zVel > forceJumpStrength[FORCE_LEVEL_2]/2.0f )
					{
						zVel = forceJumpStrength[FORCE_LEVEL_2]/2.0f;
					}
					if ( ent->client->ps.legsAnimTimer > 500 )
					{//not at end of anim yet
						fwdAngles[YAW] = ent->client->ps.viewangles[YAW];
						AngleVectors( fwdAngles, fwd, NULL, NULL );
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
						VectorScale( fwd, speed, ent->client->ps.velocity );
					}
					ent->client->ps.velocity[2] = zVel;//preserve z velocity
					VectorMA( ent->client->ps.velocity, -128, trace.plane.normal, ent->client->ps.velocity );
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
			if ( !ent->s.number )
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
		if ( ent->s.number || !player_locked )
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
	if ( !ent->s.number )
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
	if ( ent->s.number )
	{
		return qfalse;
	}
	if ( ( ent->client->ps.saberMove == LS_A_BACK || ent->client->ps.saberMove == LS_A_BACK_CR || ent->client->ps.saberMove == LS_A_BACKSTAB )
		&& PM_InAnimForSaberMove( ent->client->ps.torsoAnim, ent->client->ps.saberMove ) )
	{
		if ( ent->client->ps.saberMove != LS_A_BACKSTAB || !ent->enemy || ent->s.number )
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

qboolean PM_AdjustAnglesForKnockdown( gentity_t *ent, usercmd_t *ucmd, qboolean angleClampOnly )
{
	if ( PM_InKnockDown( &ent->client->ps ) )
	{//being knocked down or getting up, can't do anything!
		if ( !angleClampOnly )
		{
			ucmd->forwardmove = 0;
			ucmd->rightmove = 0;
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
	float		pitchMin=-75, pitchMax=75, yawMin=0, yawMax=0;	//just to shut up warnings
	int			i;
	vec3_t		start, end, tmins, tmaxs, right;
	trace_t		trace;
	qboolean	lockedYaw = qfalse;

	if ( ps->pm_type == PM_INTERMISSION ) 
	{
		return;		// no view changes at all
	}

	if ( ps->pm_type != PM_SPECTATOR && ps->stats[STAT_HEALTH] <= 0 ) 
	{
		return;		// no view changes at all
	}

//	if ( player_locked )
//	{//can't turn
//		return;
//	}
	if ( ps->eFlags & EF_NPC && gent != NULL && gent->client != NULL )
	{
		if(gent->client->renderInfo.renderFlags & RF_LOCKEDANGLE)
		{
			pitchMin = 0 - gent->client->renderInfo.headPitchRangeUp - gent->client->renderInfo.torsoPitchRangeUp;
			pitchMax = gent->client->renderInfo.headPitchRangeDown + gent->client->renderInfo.torsoPitchRangeDown;
			
			yawMin = 0 - gent->client->renderInfo.headYawRangeLeft - gent->client->renderInfo.torsoYawRangeLeft;
			yawMax = gent->client->renderInfo.headYawRangeRight + gent->client->renderInfo.torsoYawRangeRight;

			lockedYaw = qtrue;
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
		pitchMin = -35;
		pitchMax = 30;
	}

	const short pitchClampMin = ANGLE2SHORT(pitchMin);
	const short pitchClampMax = ANGLE2SHORT(pitchMax);

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
			} 
			else if ( temp < pitchClampMin ) 
			{
				ps->delta_angles[i] = (pitchClampMin - cmd->angles[i]) & 0xffff;	//& clamp to short
				temp = pitchClampMin;
			}
		}
		if ( i == ROLL && ps->vehicleModel != 0 ) 
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
		//FIXME: Are we losing precision here?  Is this why it jitters?
		ps->viewangles[i] = SHORT2ANGLE(temp);

		if ( i == YAW && lockedYaw) 
		{
			// don't let the player look left or right more than the clamp, if any
			if ( AngleSubtract(ps->viewangles[i], gent->client->renderInfo.lockYaw) > yawMax ) 
			{
				ps->viewangles[i] = yawMax;
			} 
			else if ( AngleSubtract(ps->viewangles[i], gent->client->renderInfo.lockYaw) < yawMin ) 
			{
				ps->viewangles[i] = yawMin;
			}
		}
	}

	if ( (!cg.renderingThirdPerson||cg.zoomMode) && (cmd->buttons & BUTTON_USE) && cmd->rightmove != 0 && !cmd->forwardmove && cmd->upmove <= 0 )
	{//Only lean if holding use button, strafing and not moving forward or back and not jumping
		if ( gent )
		{
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
			gi.trace( &trace, start, tmins, tmaxs, end, gent->s.number, MASK_PLAYERSOLID, G2_NOCOLLIDE, 0 );

			ps->leanofs = floor((float)leanofs * trace.fraction);

			ps->leanStopDebounceTime = 20;
		}
	}
	else
	{
		if ( gent && (cmd->forwardmove || cmd->upmove > 0) )
		{
			if( ( pm->ps->legsAnim == LEGS_LEAN_RIGHT1) ||
				( pm->ps->legsAnim == LEGS_LEAN_LEFT1) )
			{
				pm->ps->legsAnimTimer = 0;//Force it to stop the anim
			}
		}

		if ( ps->leanofs > 0 )
		{
			//FIXME: play lean anim backwards?
			ps->leanofs-=4;
			if ( ps->leanofs < 0 )
			{
				ps->leanofs = 0;
			}
		}
		else if ( ps->leanofs < 0 )
		{
			//FIXME: play lean anim backwards?
			ps->leanofs+=4;
			if ( ps->leanofs > 0 )
			{
				ps->leanofs = 0;
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

