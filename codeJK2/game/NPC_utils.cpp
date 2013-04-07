//NPC_utils.cpp

// leave this line at the top for all NPC_xxxx.cpp files...
#include "g_headers.h"




#include "b_local.h"
#include "g_icarus.h"
#include "Q3_Interface.h"

extern int	ICARUS_entFilter;
int	teamNumbers[TEAM_NUM_TEAMS];
int	teamStrength[TEAM_NUM_TEAMS];
int	teamCounter[TEAM_NUM_TEAMS];

#define	VALID_ATTACK_CONE	2.0f	//Degrees
void GetAnglesForDirection( const vec3_t p1, const vec3_t p2, vec3_t out );
extern void Q3_DebugPrint( int level, const char *format, ... );

/*
void CalcEntitySpot ( gentity_t *ent, spot_t spot, vec3_t point ) 

Added: Uses shootAngles if a NPC has them

*/
extern void ViewHeightFix(const gentity_t *const ent);
extern void AddLeanOfs(const gentity_t *const ent, vec3_t point);
extern void SubtractLeanOfs(const gentity_t *const ent, vec3_t point);
void CalcEntitySpot ( const gentity_t *ent, const spot_t spot, vec3_t point ) 
{
	vec3_t	forward, up, right;
	vec3_t	start, end;
	trace_t	tr;

	if ( !ent )
	{
		return;
	}
	ViewHeightFix(ent);
	switch ( spot ) 
	{
	case SPOT_ORIGIN:
		if(VectorCompare(ent->currentOrigin, vec3_origin))
		{//brush
			VectorSubtract(ent->absmax, ent->absmin, point);//size
			VectorMA(ent->absmin, 0.5, point, point);
		}
		else
		{
			VectorCopy ( ent->currentOrigin, point );
		}
		break;

	case SPOT_CHEST:
	case SPOT_HEAD:
		if ( ent->client && VectorLengthSquared( ent->client->renderInfo.eyePoint ) && (ent->client->ps.viewEntity <= 0 || ent->client->ps.viewEntity >= ENTITYNUM_WORLD) )
		{//Actual tag_head eyespot!
			//FIXME: Stasis aliens may have a problem here...
			VectorCopy( ent->client->renderInfo.eyePoint, point );
			if ( ent->client->NPC_class == CLASS_ATST )
			{//adjust up some
				point[2] += 28;//magic number :)
			}
			if ( ent->NPC )
			{//always aim from the center of my bbox, so we don't wiggle when we lean forward or backwards
				point[0] = ent->currentOrigin[0];
				point[1] = ent->currentOrigin[1];
			}
			else if ( !ent->s.number )
			{
				SubtractLeanOfs( ent, point );
			}
		}
		else
		{
			VectorCopy ( ent->currentOrigin, point );
			if ( ent->client ) 
			{
				point[2] += ent->client->ps.viewheight;
			}
		}
		if ( spot == SPOT_CHEST && ent->client )
		{
			if ( ent->client->NPC_class != CLASS_ATST )
			{//adjust up some
				point[2] -= ent->maxs[2]*0.2f;
			}
		}
		break;

	case SPOT_HEAD_LEAN:
		if ( ent->client && VectorLengthSquared( ent->client->renderInfo.eyePoint ) && (ent->client->ps.viewEntity <= 0 || ent->client->ps.viewEntity >= ENTITYNUM_WORLD) )
		{//Actual tag_head eyespot!
			//FIXME: Stasis aliens may have a problem here...
			VectorCopy( ent->client->renderInfo.eyePoint, point );
			if ( ent->client->NPC_class == CLASS_ATST )
			{//adjust up some
				point[2] += 28;//magic number :)
			}
			if ( ent->NPC )
			{//always aim from the center of my bbox, so we don't wiggle when we lean forward or backwards
				point[0] = ent->currentOrigin[0];
				point[1] = ent->currentOrigin[1];
			}
			else if ( !ent->s.number )
			{
				SubtractLeanOfs( ent, point );
			}
			//NOTE: automatically takes leaning into account!
		}
		else
		{
			VectorCopy ( ent->currentOrigin, point );
			if ( ent->client ) 
			{
				point[2] += ent->client->ps.viewheight;
			}
			//AddLeanOfs ( ent, point );
		}
		break;

	//FIXME: implement...
	//case SPOT_CHEST:
		//Returns point 3/4 from tag_torso to tag_head?
		//break;

	case SPOT_LEGS:
		VectorCopy ( ent->currentOrigin, point );
		point[2] += (ent->mins[2] * 0.5);
		break;

	case SPOT_WEAPON:
		if( ent->NPC && !VectorCompare( ent->NPC->shootAngles, vec3_origin ) && !VectorCompare( ent->NPC->shootAngles, ent->client->ps.viewangles ))
		{
			AngleVectors( ent->NPC->shootAngles, forward, right, up );
		}
		else
		{
			AngleVectors( ent->client->ps.viewangles, forward, right, up );
		}
		CalcMuzzlePoint( (gentity_t*)ent, forward, right, up, point, 0 );
		//NOTE: automatically takes leaning into account!
		break;

	case SPOT_GROUND:
		// if entity is on the ground, just use it's absmin
		if ( ent->s.groundEntityNum != -1 ) 
		{
			VectorCopy( ent->currentOrigin, point );
			point[2] = ent->absmin[2];
			break;
		}

		// if it is reasonably close to the ground, give the point underneath of it
		VectorCopy( ent->currentOrigin, start );
		start[2] = ent->absmin[2];
		VectorCopy( start, end );
		end[2] -= 64;
		gi.trace( &tr, start, ent->mins, ent->maxs, end, ent->s.number, MASK_PLAYERSOLID, G2_NOCOLLIDE, 0 );
		if ( tr.fraction < 1.0 ) 
		{
			VectorCopy( tr.endpos, point);
			break;
		}

		// otherwise just use the origin
		VectorCopy( ent->currentOrigin, point );
		break;

	default:
		VectorCopy ( ent->currentOrigin, point );
		break;
	}
}


//===================================================================================

/*
qboolean NPC_UpdateAngles ( qboolean doPitch, qboolean doYaw ) 

Added: option to do just pitch or just yaw

Does not include "aim" in it's calculations

FIXME: stop compressing angles into shorts!!!!
*/
extern cvar_t		*g_timescale;
qboolean NPC_UpdateAngles ( qboolean doPitch, qboolean doYaw ) 
{
#if 1

	float		error;
	float		decay;
	float		targetPitch = 0;
	float		targetYaw = 0;
	float		yawSpeed;
	qboolean	exact = qtrue;

	// if angle changes are locked; just keep the current angles
	// aimTime isn't even set anymore... so this code was never reached, but I need a way to lock NPC's yaw, so instead of making a new SCF_ flag, just use the existing render flag... - dmv
	if ( !NPC->enemy && ( (level.time < NPCInfo->aimTime) || NPC->client->renderInfo.renderFlags & RF_LOCKEDANGLE) ) 
	{
		if(doPitch)
			targetPitch = NPCInfo->lockedDesiredPitch;

		if(doYaw)
			targetYaw = NPCInfo->lockedDesiredYaw;
	}
	else 
	{
		// we're changing the lockedDesired Pitch/Yaw below so it's lost it's original meaning, get rid of the lock flag
		NPC->client->renderInfo.renderFlags &= ~RF_LOCKEDANGLE;

		if(doPitch)
		{
			targetPitch = NPCInfo->desiredPitch;
			NPCInfo->lockedDesiredPitch = NPCInfo->desiredPitch;
		}

		if(doYaw)
		{
			targetYaw = NPCInfo->desiredYaw;
			NPCInfo->lockedDesiredYaw = NPCInfo->desiredYaw;
		}			
	}

	if ( NPC->s.weapon == WP_EMPLACED_GUN )
	{
		// FIXME: this seems to do nothing, actually...
		yawSpeed = 20;
	}
	else
	{
		yawSpeed = NPCInfo->stats.yawSpeed;
	}

	if ( NPC->s.weapon == WP_SABER && NPC->client->ps.forcePowersActive&(1<<FP_SPEED) )
	{
		yawSpeed *= 1.0f/g_timescale->value;
	}
	
	if( doYaw )
	{
		// decay yaw error
		error = AngleDelta ( NPC->client->ps.viewangles[YAW], targetYaw );
		if( fabs(error) > MIN_ANGLE_ERROR )
		{
			if ( error ) 
			{
				exact = qfalse;

				decay = 60.0 + yawSpeed * 3;
				decay *= 50.0f / 1000.0f;//msec

				if ( error < 0.0 ) 
				{
					error += decay;
					if ( error > 0.0 ) 
					{
						error = 0.0;
					}
				}
				else 
				{
					error -= decay;
					if ( error < 0.0 ) 
					{
						error = 0.0;
					}
				}
			}
		}
		
		ucmd.angles[YAW] = ANGLE2SHORT( targetYaw + error ) - client->ps.delta_angles[YAW];
	}

	//FIXME: have a pitchSpeed?
	if( doPitch )
	{
		// decay pitch error
		error = AngleDelta ( NPC->client->ps.viewangles[PITCH], targetPitch );
		if ( fabs(error) > MIN_ANGLE_ERROR )
		{
			if ( error ) 
			{
				exact = qfalse;

				decay = 60.0 + yawSpeed * 3;
				decay *= 50.0f / 1000.0f;//msec

				if ( error < 0.0 ) 
				{
					error += decay;
					if ( error > 0.0 ) 
					{
						error = 0.0;
					}
				}
				else 
				{
					error -= decay;
					if ( error < 0.0 ) 
					{
						error = 0.0;
					}
				}
			}
		}

		ucmd.angles[PITCH] = ANGLE2SHORT( targetPitch + error ) - client->ps.delta_angles[PITCH];
	}

	ucmd.angles[ROLL] = ANGLE2SHORT ( NPC->client->ps.viewangles[ROLL] ) - client->ps.delta_angles[ROLL];

	if ( exact && Q3_TaskIDPending( NPC, TID_ANGLE_FACE ) )
	{
		Q3_TaskIDComplete( NPC, TID_ANGLE_FACE );
	}
	return exact;

#else

	float		error;
	float		decay;
	float		targetPitch = 0;
	float		targetYaw = 0;
	float		yawSpeed;
	//float		runningMod = NPCInfo->currentSpeed/100.0f;
	qboolean	exact = qtrue;
	qboolean	doSound = qfalse;

	// if angle changes are locked; just keep the current angles
	if ( level.time < NPCInfo->aimTime ) 
	{
		if(doPitch)
			targetPitch = NPCInfo->lockedDesiredPitch;
		if(doYaw)
			targetYaw = NPCInfo->lockedDesiredYaw;
	}
	else 
	{
		if(doPitch)
			targetPitch = NPCInfo->desiredPitch;
		if(doYaw)
			targetYaw = NPCInfo->desiredYaw;

//		NPCInfo->aimTime = level.time + 250;
		if(doPitch)
			NPCInfo->lockedDesiredPitch = NPCInfo->desiredPitch;
		if(doYaw)
			NPCInfo->lockedDesiredYaw = NPCInfo->desiredYaw;
	}

	yawSpeed = NPCInfo->stats.yawSpeed;

	if(doYaw)
	{
		// decay yaw error
		error = AngleDelta ( NPC->client->ps.viewangles[YAW], targetYaw );
		if( fabs(error) > MIN_ANGLE_ERROR )
		{
			/*
			if(NPC->client->playerTeam == TEAM_BORG&&
				NPCInfo->behaviorState != BS_FACE&&NPCInfo->tempBehavior!= BS_FACE)
			{//HACK - borg turn more jittery
				if ( error ) 
				{
					exact = qfalse;

					decay = 60.0 + yawSpeed * 3;
					decay *= 50.0 / 1000.0;//msec
					//Snap to
					if(fabs(error) > 10)
					{
						if(random() > 0.6)
						{
							doSound = qtrue;
						}
					}

					if ( error < 0.0)//-10.0 ) 
					{
						error += decay;
						if ( error > 0.0 ) 
						{
							error = 0.0;
						}
					}
					else if ( error > 0.0)//10.0 ) 
					{
						error -= decay;
						if ( error < 0.0 ) 
						{
							error = 0.0;
						}
					}
				}
			}
			else*/
			
			if ( error ) 
			{
				exact = qfalse;

				decay = 60.0 + yawSpeed * 3;
				decay *= 50.0 / 1000.0;//msec

				if ( error < 0.0 ) 
				{
					error += decay;
					if ( error > 0.0 ) 
					{
						error = 0.0;
					}
				}
				else 
				{
					error -= decay;
					if ( error < 0.0 ) 
					{
						error = 0.0;
					}
				}
			}
		}
		ucmd.angles[YAW] = ANGLE2SHORT( targetYaw + error ) - client->ps.delta_angles[YAW];
	}

	//FIXME: have a pitchSpeed?
	if(doPitch)
	{
		// decay pitch error
		error = AngleDelta ( NPC->client->ps.viewangles[PITCH], targetPitch );
		if ( fabs(error) > MIN_ANGLE_ERROR )
		{
			/*
			if(NPC->client->playerTeam == TEAM_BORG&&
				NPCInfo->behaviorState != BS_FACE&&NPCInfo->tempBehavior!= BS_FACE)
			{//HACK - borg turn more jittery
				if ( error ) 
				{
					exact = qfalse;

					decay = 60.0 + yawSpeed * 3;
					decay *= 50.0 / 1000.0;//msec
					//Snap to
					if(fabs(error) > 10)
					{
						if(random() > 0.6)
						{
							doSound = qtrue;
						}
					}

					if ( error < 0.0)//-10.0 ) 
					{
						error += decay;
						if ( error > 0.0 ) 
						{
							error = 0.0;
						}
					}
					else if ( error > 0.0)//10.0 ) 
					{
						error -= decay;
						if ( error < 0.0 ) 
						{
							error = 0.0;
						}
					}
				}
			}
			else*/
			
			if ( error ) 
			{
				exact = qfalse;

				decay = 60.0 + yawSpeed * 3;
				decay *= 50.0 / 1000.0;//msec

				if ( error < 0.0 ) 
				{
					error += decay;
					if ( error > 0.0 ) 
					{
						error = 0.0;
					}
				}
				else 
				{
					error -= decay;
					if ( error < 0.0 ) 
					{
						error = 0.0;
					}
				}
			}
		}
		ucmd.angles[PITCH] = ANGLE2SHORT( targetPitch + error ) - client->ps.delta_angles[PITCH];
	}

	ucmd.angles[ROLL] = ANGLE2SHORT ( NPC->client->ps.viewangles[ROLL] ) - client->ps.delta_angles[ROLL];

	/*
	if(doSound)
	{
		G_Sound(NPC, G_SoundIndex(va("sound/enemies/borg/borgservo%d.wav", Q_irand(1, 8))));
	}
	*/

	return exact;

#endif

}

void NPC_AimWiggle( vec3_t enemy_org )
{
	//shoot for somewhere between the head and torso
	//NOTE: yes, I know this looks weird, but it works
	if ( NPCInfo->aimErrorDebounceTime < level.time )
	{
		NPCInfo->aimOfs[0] = 0.3*Q_flrand(NPC->enemy->mins[0], NPC->enemy->maxs[0]);
		NPCInfo->aimOfs[1] = 0.3*Q_flrand(NPC->enemy->mins[1], NPC->enemy->maxs[1]);
		if ( NPC->enemy->maxs[2] > 0 )
		{
			NPCInfo->aimOfs[2] = NPC->enemy->maxs[2]*Q_flrand(0.0f, -1.0f);
		}
	}
	VectorAdd( enemy_org, NPCInfo->aimOfs, enemy_org );
}

/*
qboolean NPC_UpdateFiringAngles ( qboolean doPitch, qboolean doYaw ) 

  Includes aim when determining angles - so they don't always hit...
  */
qboolean NPC_UpdateFiringAngles ( qboolean doPitch, qboolean doYaw ) 
{

#if 0

	float		diff;
	float		error;
	float		targetPitch = 0;
	float		targetYaw = 0;
	qboolean	exact = qtrue;

	if ( level.time < NPCInfo->aimTime ) 
	{
		if( doPitch )
			targetPitch = NPCInfo->lockedDesiredPitch;

		if( doYaw )
			targetYaw = NPCInfo->lockedDesiredYaw;
	}
	else 
	{
		if( doPitch )
		{
			targetPitch = NPCInfo->desiredPitch;
			NPCInfo->lockedDesiredPitch = NPCInfo->desiredPitch;
		}

		if( doYaw )
		{
			targetYaw = NPCInfo->desiredYaw;
			NPCInfo->lockedDesiredYaw = NPCInfo->desiredYaw;
		}			
	}

	if( doYaw )
	{
		// add yaw error based on NPCInfo->aim value
		error = ((float)(6 - NPCInfo->stats.aim)) * Q_flrand(-1, 1);

		if(Q_irand(0, 1))
			error *= -1;

		diff = AngleDelta ( NPC->client->ps.viewangles[YAW], targetYaw );

		if ( diff )
			exact = qfalse;
		
		ucmd.angles[YAW] = ANGLE2SHORT( targetYaw + diff + error ) - client->ps.delta_angles[YAW];
	}

	if( doPitch )
	{
		// add pitch error based on NPCInfo->aim value
		error = ((float)(6 - NPCInfo->stats.aim)) * Q_flrand(-1, 1);

		diff = AngleDelta ( NPC->client->ps.viewangles[PITCH], targetPitch );

		if ( diff )
			exact = qfalse;
		
		ucmd.angles[PITCH] = ANGLE2SHORT( targetPitch + diff + error ) - client->ps.delta_angles[PITCH];
	}

	ucmd.angles[ROLL] = ANGLE2SHORT ( NPC->client->ps.viewangles[ROLL] ) - client->ps.delta_angles[ROLL];

	return exact;
	
#else

	float		error, diff;
	float		decay;
	float		targetPitch = 0;
	float		targetYaw = 0;
	qboolean	exact = qtrue;

	// if angle changes are locked; just keep the current angles
	if ( level.time < NPCInfo->aimTime ) 
	{
		if(doPitch)
			targetPitch = NPCInfo->lockedDesiredPitch;
		if(doYaw)
			targetYaw = NPCInfo->lockedDesiredYaw;
	}
	else 
	{
		if(doPitch)
			targetPitch = NPCInfo->desiredPitch;
		if(doYaw)
			targetYaw = NPCInfo->desiredYaw;

//		NPCInfo->aimTime = level.time + 250;
		if(doPitch)
			NPCInfo->lockedDesiredPitch = NPCInfo->desiredPitch;
		if(doYaw)
			NPCInfo->lockedDesiredYaw = NPCInfo->desiredYaw;
	}

	if ( NPCInfo->aimErrorDebounceTime < level.time )
	{
		if ( Q_irand(0, 1 ) )
		{
			NPCInfo->lastAimErrorYaw = ((float)(6 - NPCInfo->stats.aim)) * Q_flrand(-1, 1);
		}
		if ( Q_irand(0, 1 ) )
		{
			NPCInfo->lastAimErrorPitch = ((float)(6 - NPCInfo->stats.aim)) * Q_flrand(-1, 1);
		}
		NPCInfo->aimErrorDebounceTime = level.time + Q_irand(250, 2000);
	}

	if(doYaw)
	{
		// decay yaw diff
		diff = AngleDelta ( NPC->client->ps.viewangles[YAW], targetYaw );
		
		if ( diff) 
		{
			exact = qfalse;

			decay = 60.0 + 80.0;
			decay *= 50.0f / 1000.0f;//msec
			if ( diff < 0.0 ) 
			{
				diff += decay;
				if ( diff > 0.0 ) 
				{
					diff = 0.0;
				}
			}
			else 
			{
				diff -= decay;
				if ( diff < 0.0 ) 
				{
					diff = 0.0;
				}
			}
		}
				
		// add yaw error based on NPCInfo->aim value
		error = NPCInfo->lastAimErrorYaw;

		/*
		if(Q_irand(0, 1))
		{
			error *= -1;
		}
		*/

		ucmd.angles[YAW] = ANGLE2SHORT( targetYaw + diff + error ) - client->ps.delta_angles[YAW];
	}

	if(doPitch)
	{
		// decay pitch diff
		diff = AngleDelta ( NPC->client->ps.viewangles[PITCH], targetPitch );
		if ( diff) 
		{
			exact = qfalse;

			decay = 60.0 + 80.0;
			decay *= 50.0f / 1000.0f;//msec
			if ( diff < 0.0 ) 
			{
				diff += decay;
				if ( diff > 0.0 ) 
				{
					diff = 0.0;
				}
			}
			else 
			{
				diff -= decay;
				if ( diff < 0.0 ) 
				{
					diff = 0.0;
				}
			}
		}
		
		error = NPCInfo->lastAimErrorPitch;

		ucmd.angles[PITCH] = ANGLE2SHORT( targetPitch + diff + error ) - client->ps.delta_angles[PITCH];
	}

	ucmd.angles[ROLL] = ANGLE2SHORT ( NPC->client->ps.viewangles[ROLL] ) - client->ps.delta_angles[ROLL];

	return exact;

#endif

}
//===================================================================================

/*
static void NPC_UpdateShootAngles (vec3_t angles, qboolean doPitch, qboolean doYaw ) 

Does update angles on shootAngles
*/

void NPC_UpdateShootAngles (vec3_t angles, qboolean doPitch, qboolean doYaw ) 
{//FIXME: shoot angles either not set right or not used!
	float		error;
	float		decay;
	float		targetPitch = 0;
	float		targetYaw = 0;

	if(doPitch)
		targetPitch = angles[PITCH];
	if(doYaw)
		targetYaw = angles[YAW];


	if(doYaw)
	{
		// decay yaw error
		error = AngleDelta ( NPCInfo->shootAngles[YAW], targetYaw );
		if ( error ) 
		{
			decay = 60.0 + 80.0 * NPCInfo->stats.aim;
			decay *= 100.0f / 1000.0f;//msec
			if ( error < 0.0 ) 
			{
				error += decay;
				if ( error > 0.0 ) 
				{
					error = 0.0;
				}
			}
			else 
			{
				error -= decay;
				if ( error < 0.0 ) 
				{
					error = 0.0;
				}
			}
		}
		NPCInfo->shootAngles[YAW] = targetYaw + error;
	}

	if(doPitch)
	{
		// decay pitch error
		error = AngleDelta ( NPCInfo->shootAngles[PITCH], targetPitch );
		if ( error ) 
		{
			decay = 60.0 + 80.0 * NPCInfo->stats.aim;
			decay *= 100.0f / 1000.0f;//msec
			if ( error < 0.0 ) 
			{
				error += decay;
				if ( error > 0.0 ) 
				{
					error = 0.0;
				}
			}
			else 
			{
				error -= decay;
				if ( error < 0.0 ) 
				{
					error = 0.0;
				}
			}
		}
		NPCInfo->shootAngles[PITCH] = targetPitch + error;
	}
}

/*
void SetTeamNumbers (void)

Sets the number of living clients on each team

FIXME: Does not account for non-respawned players!
FIXME: Don't include medics?
*/
void SetTeamNumbers (void)
{
	gentity_t	*found;
	int			i;

	for( i = 0; i < TEAM_NUM_TEAMS; i++ )
	{
		teamNumbers[i] = 0;
		teamStrength[i] = 0;
	}

	for( i = 0; i < 1 ; i++ )
	{
		found = &g_entities[i];

		if( found->client )
		{
			if( found->health > 0 )//FIXME: or if a player!
			{
				teamNumbers[found->client->playerTeam]++;
				teamStrength[found->client->playerTeam] += found->health;
			}
		}
	}

	for( i = 0; i < TEAM_NUM_TEAMS; i++ )
	{//Get the average health
		teamStrength[i] = floor( ((float)(teamStrength[i])) / ((float)(teamNumbers[i])) );
	}
}

extern stringID_table_t BSTable[];
extern stringID_table_t BSETTable[];
qboolean G_ActivateBehavior (gentity_t *self, int bset )
{
	bState_t	bSID = (bState_t)-1;
	char *bs_name = NULL;

	if ( !self )
	{
		return qfalse;
	}

	bs_name = self->behaviorSet[bset];
	
	if( !(VALIDSTRING( bs_name )) )
	{
		return qfalse;
	}

	if ( self->NPC )
	{
		bSID = (bState_t)(GetIDForString( BSTable, bs_name ));
	}

	if(bSID > -1)
	{
		self->NPC->tempBehavior = BS_DEFAULT;
		self->NPC->behaviorState = bSID;
	}
	else
	{
		/*
		char			newname[MAX_FILENAME_LENGTH];		
		sprintf((char *) &newname, "%s/%s", Q3_SCRIPT_DIR, bs_name );
		*/
		
		//FIXME: between here and actually getting into the ICARUS_RunScript function, the stack gets blown!
		if ( ( ICARUS_entFilter == -1 ) || ( ICARUS_entFilter == self->s.number ) )
		{
			Q3_DebugPrint( WL_VERBOSE, "%s attempting to run bSet %s (%s)\n", self->targetname, GetStringForID( BSETTable, bset ), bs_name );
		}
		ICARUS_RunScript( self, va( "%s/%s", Q3_SCRIPT_DIR, bs_name ) );
	}
	return qtrue;
}


/*
=============================================================================

	Extended Functions

=============================================================================
*/


/*
-------------------------
NPC_ValidEnemy
-------------------------
*/

qboolean NPC_ValidEnemy( gentity_t *ent )
{
	//Must be a valid pointer
	if ( ent == NULL )
		return qfalse;

	//Must not be me
	if ( ent == NPC )
		return qfalse;

	//Must not be deleted
	if ( ent->inuse == qfalse )
		return qfalse;

	//Must be alive
	if ( ent->health <= 0 )
		return qfalse;

	//In case they're in notarget mode
	if ( ent->flags & FL_NOTARGET )
		return qfalse;

	//Must be an NPC
	if ( ent->client == NULL )
	{
		if ( ent->svFlags&SVF_NONNPC_ENEMY )
		{//still potentially valid
			if ( ent->noDamageTeam == NPC->client->playerTeam )
			{
				return qfalse;
			}
			else
			{
				return qtrue;
			}
		}
		else
		{
			return qfalse;
		}
	}

	//Can't be on the same team
	if ( ent->client->playerTeam == NPC->client->playerTeam )
		return qfalse;

	//if haven't seen him in a while, give up
	//if ( NPCInfo->enemyLastSeenTime != 0 && level.time - NPCInfo->enemyLastSeenTime > 7000 )//FIXME: make a stat?
	//	return qfalse;

	return qtrue;
}

/*
-------------------------
NPC_TargetVisible
-------------------------
*/

qboolean NPC_TargetVisible( gentity_t *ent )
{
	//Make sure we're in a valid range
	if ( DistanceSquared( ent->currentOrigin, NPC->currentOrigin ) > ( NPCInfo->stats.visrange * NPCInfo->stats.visrange ) )
		return qfalse;

	//Check our FOV
	if ( InFOV( ent, NPC, NPCInfo->stats.hfov, NPCInfo->stats.vfov ) == qfalse )
		return qfalse;

	//Check for sight
	if ( NPC_ClearLOS( ent ) == qfalse )
		return qfalse;

	return qtrue;
}

/*
-------------------------
NPC_GetCheckDelta
-------------------------
*/

/*
#define	CHECK_TIME_BASE				250
#define CHECK_TIME_BASE_SQUARED		( CHECK_TIME_BASE * CHECK_TIME_BASE )

static int NPC_GetCheckDelta( void )
{
	if ( NPC_ValidEnemy( NPC->enemy ) == qfalse )
	{
		int distance = DistanceSquared( NPC->currentOrigin, g_entities[0].currentOrigin );

		distance /= CHECK_TIME_BASE_SQUARED;

		return ( CHECK_TIME_BASE * distance );
	}

	return 0;
}
*/

/*
-------------------------
NPC_FindNearestEnemy
-------------------------
*/

#define	MAX_RADIUS_ENTS			256	//NOTE: This can cause entities to be lost
#define NEAR_DEFAULT_RADIUS		256

int NPC_FindNearestEnemy( gentity_t *ent )
{
	gentity_t	*radiusEnts[ MAX_RADIUS_ENTS ];
	vec3_t		mins, maxs;
	int			nearestEntID = -1;
	float		nearestDist = (float)WORLD_SIZE*(float)WORLD_SIZE;
	float		distance;
	int			numEnts, numChecks = 0;
	int			i;

	//Setup the bbox to search in
	for ( i = 0; i < 3; i++ )
	{
		mins[i] = ent->currentOrigin[i] - NPCInfo->stats.visrange;
		maxs[i] = ent->currentOrigin[i] + NPCInfo->stats.visrange;
	}

	//Get a number of entities in a given space
	numEnts = gi.EntitiesInBox( mins, maxs, radiusEnts, MAX_RADIUS_ENTS );

	for ( i = 0; i < numEnts; i++ )
	{
		//Don't consider self
		if ( radiusEnts[i] == ent )
			continue;

		//Must be valid
		if ( NPC_ValidEnemy( radiusEnts[i] ) == qfalse )
			continue;

		numChecks++;
		//Must be visible
		if ( NPC_TargetVisible( radiusEnts[i] ) == qfalse )
			continue;

		distance = DistanceSquared( ent->currentOrigin, radiusEnts[i]->currentOrigin );

		//Found one closer to us
		if ( distance < nearestDist )
		{
			nearestEntID = radiusEnts[i]->s.number;
			nearestDist = distance;
		}
	}

	return nearestEntID;
}

/*
-------------------------
NPC_PickEnemyExt
-------------------------
*/

gentity_t *NPC_PickEnemyExt( qboolean checkAlerts = qfalse )
{
	//Check for Hazard Team status and remove this check
	/*
	if ( NPC->client->playerTeam != TEAM_STARFLEET )
	{
		//If we've found the player, return it
		if ( NPC_FindPlayer() )
			return &g_entities[0];
	}
	*/

	//If we've asked for the closest enemy
	int entID = NPC_FindNearestEnemy( NPC );

	//If we have a valid enemy, use it
	if ( entID >= 0 )
		return &g_entities[entID];

	if ( checkAlerts )
	{
		int alertEvent = NPC_CheckAlertEvents( qtrue, qtrue, -1, qtrue, AEL_DISCOVERED );

		//There is an event to look at
		if ( alertEvent >= 0 )
		{
			alertEvent_t *event = &level.alertEvents[alertEvent];

			//Don't pay attention to our own alerts
			if ( event->owner == NPC )
				return NULL;

			if ( event->level >= AEL_DISCOVERED )
			{
				//If it's the player, attack him
				if ( event->owner == &g_entities[0] )
					return event->owner;

				//If it's on our team, then take its enemy as well
				if ( ( event->owner->client ) && ( event->owner->client->playerTeam == NPC->client->playerTeam ) )
					return event->owner->enemy;
			}
		}
	}

	return NULL;
}

/*
-------------------------
NPC_FindPlayer
-------------------------
*/

qboolean NPC_FindPlayer( void )
{
	return NPC_TargetVisible( &g_entities[0] );
}

/*
-------------------------
NPC_CheckPlayerDistance
-------------------------
*/

static qboolean NPC_CheckPlayerDistance( void )
{
	//Make sure we have an enemy
	if ( NPC->enemy == NULL )
		return qfalse;

	//Only do this for non-players
	if ( NPC->enemy->s.number == 0 )
		return qfalse;

	//must be set up to get mad at player
	if ( !NPC->client || NPC->client->enemyTeam != TEAM_PLAYER )
		return qfalse;

	//Must be within our FOV
	if ( InFOV( &g_entities[0], NPC, NPCInfo->stats.hfov, NPCInfo->stats.vfov ) == qfalse )
		return qfalse;

	float	distance = DistanceSquared( NPC->currentOrigin, NPC->enemy->currentOrigin );

	if ( distance > DistanceSquared( NPC->currentOrigin, g_entities[0].currentOrigin ) )
	{
		G_SetEnemy( NPC, &g_entities[0] );
		return qtrue;
	}

	return qfalse;
}

/*
-------------------------
NPC_FindEnemy
-------------------------
*/

qboolean NPC_FindEnemy( qboolean checkAlerts = qfalse )
{
	//We're ignoring all enemies for now
	if( NPC->svFlags & SVF_IGNORE_ENEMIES )
	{
		G_ClearEnemy( NPC );
		return qfalse;
	}

	//we can't pick up any enemies for now
	if( NPCInfo->confusionTime > level.time )
	{
		return qfalse;
	}

	//Don't want a new enemy
	if ( ( ValidEnemy( NPC->enemy ) ) && ( NPC->svFlags & SVF_LOCKEDENEMY ) )
		return qtrue;

	//See if the player is closer than our current enemy
	if ( NPC_CheckPlayerDistance() )
	{
		return qtrue;
	}

	//Otherwise, turn off the flag
	NPC->svFlags &= ~SVF_LOCKEDENEMY;

	//If we've gotten here alright, then our target it still valid
	if ( NPC_ValidEnemy( NPC->enemy ) )
		return qtrue;

	gentity_t *newenemy = NPC_PickEnemyExt( checkAlerts );

	//if we found one, take it as the enemy
	if( NPC_ValidEnemy( newenemy ) )
	{
		G_SetEnemy( NPC, newenemy );
		return qtrue;
	}

	return qfalse;
}

/*
-------------------------
NPC_CheckEnemyExt
-------------------------
*/

qboolean NPC_CheckEnemyExt( qboolean checkAlerts )
{
	//Make sure we're ready to think again
/*
	if ( NPCInfo->enemyCheckDebounceTime > level.time )
		return qfalse;

	//Get our next think time
	NPCInfo->enemyCheckDebounceTime = level.time + NPC_GetCheckDelta();

	//Attempt to find an enemy
	return NPC_FindEnemy();
*/
	return NPC_FindEnemy( checkAlerts );
}

/*
-------------------------
NPC_FacePosition
-------------------------
*/

qboolean NPC_FacePosition( vec3_t position, qboolean doPitch )
{
	vec3_t		muzzle;
	qboolean	facing = qtrue;

	//Get the positions
	if ( NPC->client && NPC->client->NPC_class == CLASS_GALAKMECH )
	{
		CalcEntitySpot( NPC, SPOT_WEAPON, muzzle );
	}
	else
	{
		CalcEntitySpot( NPC, SPOT_HEAD_LEAN, muzzle );//SPOT_HEAD
	}

	//Find the desired angles
	vec3_t	angles;

	GetAnglesForDirection( muzzle, position, angles );

	NPCInfo->desiredYaw		= AngleNormalize360( angles[YAW] );
	NPCInfo->desiredPitch	= AngleNormalize360( angles[PITCH] );

	if ( NPC->enemy && NPC->enemy->client && NPC->enemy->client->NPC_class == CLASS_ATST )
	{
		// FIXME: this is kind of dumb, but it was the easiest way to get it to look sort of ok
		NPCInfo->desiredYaw	+= Q_flrand( -5, 5 ) + sin( level.time * 0.004f ) * 7;
		NPCInfo->desiredPitch += Q_flrand( -2, 2 );
	}
	//Face that yaw
	NPC_UpdateAngles( qtrue, qtrue );

	//Find the delta between our goal and our current facing
	float yawDelta = AngleNormalize360( NPCInfo->desiredYaw - ( SHORT2ANGLE( ucmd.angles[YAW] + client->ps.delta_angles[YAW] ) ) );
	
	//See if we are facing properly
	if ( fabs( yawDelta ) > VALID_ATTACK_CONE )
		facing = qfalse;

	if ( doPitch )
	{
		//Find the delta between our goal and our current facing
		float currentAngles = ( SHORT2ANGLE( ucmd.angles[PITCH] + client->ps.delta_angles[PITCH] ) );
		float pitchDelta = NPCInfo->desiredPitch - currentAngles;
		
		//See if we are facing properly
		if ( fabs( pitchDelta ) > VALID_ATTACK_CONE )
			facing = qfalse;
	}

	return facing;
}

/*
-------------------------
NPC_FaceEntity
-------------------------
*/

qboolean NPC_FaceEntity( gentity_t *ent, qboolean doPitch )
{
	vec3_t		entPos;

	//Get the positions
	CalcEntitySpot( ent, SPOT_HEAD_LEAN, entPos );

	return NPC_FacePosition( entPos, doPitch );
}

/*
-------------------------
NPC_FaceEnemy
-------------------------
*/

qboolean NPC_FaceEnemy( qboolean doPitch )
{
	if ( NPC == NULL )
		return qfalse;

	if ( NPC->enemy == NULL )
		return qfalse;

	return NPC_FaceEntity( NPC->enemy, doPitch );
}

/*
-------------------------
NPC_CheckCanAttackExt
-------------------------
*/

qboolean NPC_CheckCanAttackExt( void )
{
	//We don't want them to shoot
	if( NPCInfo->scriptFlags & SCF_DONT_FIRE )
		return qfalse;

	//Turn to face
	if ( NPC_FaceEnemy( qtrue ) == qfalse )
		return qfalse;

	//Must have a clear line of sight to the target
	if ( NPC_ClearShot( NPC->enemy ) == qfalse )
		return qfalse;

	return qtrue;
}

/*
-------------------------
NPC_ClearLookTarget
-------------------------
*/

void NPC_ClearLookTarget( gentity_t *self )
{
	if ( !self->client )
	{
		return;
	}

	self->client->renderInfo.lookTarget = ENTITYNUM_NONE;//ENTITYNUM_WORLD;
	self->client->renderInfo.lookTargetClearTime = 0;
}

/*
-------------------------
NPC_SetLookTarget
-------------------------
*/
void NPC_SetLookTarget( gentity_t *self, int entNum, int clearTime )
{
	if ( !self->client )
	{
		return;
	}

	self->client->renderInfo.lookTarget = entNum;
	self->client->renderInfo.lookTargetClearTime = clearTime;
}

/*
-------------------------
NPC_CheckLookTarget
-------------------------
*/
qboolean NPC_CheckLookTarget( gentity_t *self )
{
	if ( self->client )
	{
		if ( self->client->renderInfo.lookTarget >= 0 && self->client->renderInfo.lookTarget < ENTITYNUM_WORLD )
		{//within valid range
			if ( (&g_entities[self->client->renderInfo.lookTarget] == NULL) || !g_entities[self->client->renderInfo.lookTarget].inuse )
			{//lookTarget not inuse or not valid anymore
				NPC_ClearLookTarget( self );
			}
			else if ( self->client->renderInfo.lookTargetClearTime && self->client->renderInfo.lookTargetClearTime < level.time )
			{//Time to clear lookTarget
				NPC_ClearLookTarget( self );
			}
			else if ( g_entities[self->client->renderInfo.lookTarget].client && self->enemy && (&g_entities[self->client->renderInfo.lookTarget] != self->enemy) )
			{//should always look at current enemy if engaged in battle... FIXME: this could override certain scripted lookTargets...???
				NPC_ClearLookTarget( self );
			}
			else
			{
				return qtrue;
			}
		}
	}

	return qfalse;
}

/*
-------------------------
NPC_CheckCharmed
-------------------------
*/
extern void G_AddVoiceEvent( gentity_t *self, int event, int speakDebounceTime );
void NPC_CheckCharmed( void )
{
	if ( NPC->client->playerTeam == TEAM_PLAYER && NPCInfo->charmedTime && NPCInfo->charmedTime < level.time && NPC->client )
	{//we were charmed, set us back!
		//NOTE: presumptions here...
		team_t	savTeam = NPC->client->enemyTeam;
		NPC->client->enemyTeam = NPC->client->playerTeam;
		NPC->client->playerTeam = savTeam;
		NPC->client->leader = NULL;
		if ( NPCInfo->tempBehavior == BS_FOLLOW_LEADER )
		{
			NPCInfo->tempBehavior = BS_DEFAULT;
		}
		G_ClearEnemy( NPC );
		NPCInfo->charmedTime = 0;
		//say something to let player know you've snapped out of it
		G_AddVoiceEvent( NPC, Q_irand(EV_CONFUSE1, EV_CONFUSE3), 2000 );
	}

}

