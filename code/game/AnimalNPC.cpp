// leave this line at the top for all g_xxxx.cpp files...
#include "g_headers.h"

#include "g_local.h"
#include "g_functions.h"
#include "g_vehicles.h"

extern float DotToSpot( vec3_t spot, vec3_t from, vec3_t fromAngles );
extern vmCvar_t	cg_thirdPersonAlpha;
extern vec3_t playerMins;
extern vec3_t playerMaxs;
extern cvar_t	*g_speederControlScheme;


// Update death sequence.
void CAnimalNPC::DeathUpdate()
{
	if ( level.time >= m_iDieTime )
	{
		// If the vehicle is not empty.
		if ( !(m_pParentEntity->client->ps.eFlags & EF_EMPTY_VEHICLE) )
		{
			EjectAll();
		}
		else
		{
			// Waste this sucker.
		}

		// Die now...
/*		else
		{
			vec3_t	mins, maxs, bottom;
			trace_t	trace;

			if ( m_pVehicleInfo->explodeFX )
			{
				G_PlayEffect( m_pVehicleInfo->explodeFX, m_pParentEntity->currentOrigin );
				//trace down and place mark
				VectorCopy( m_pParentEntity->currentOrigin, bottom );
				bottom[2] -= 80;
				gi.trace( &trace, m_pParentEntity->currentOrigin, vec3_origin, vec3_origin, bottom, m_pParentEntity->s.number, CONTENTS_SOLID );
				if ( trace.fraction < 1.0f )
				{
					VectorCopy( trace.endpos, bottom );
					bottom[2] += 2;
					G_PlayEffect( "ships/ship_explosion_mark", trace.endpos );
				}
			}

			m_pParentEntity->takedamage = qfalse;//so we don't recursively damage ourselves
			if ( m_pVehicleInfo->explosionRadius > 0 && m_pVehicleInfo->explosionDamage > 0 )
			{
				VectorCopy( m_pParentEntity->mins, mins );
				mins[2] = -4;//to keep it off the ground a *little*
				VectorCopy( m_pParentEntity->maxs, maxs );
				VectorCopy( m_pParentEntity->currentOrigin, bottom );
				bottom[2] += m_pParentEntity->mins[2] - 32;
				gi.trace( &trace, m_pParentEntity->currentOrigin, mins, maxs, bottom, m_pParentEntity->s.number, CONTENTS_SOLID );
				G_RadiusDamage( trace.endpos, NULL, m_pVehicleInfo->explosionDamage, m_pVehicleInfo->explosionRadius, NULL, MOD_EXPLOSIVE );//FIXME: extern damage and radius or base on fuel
			}

			m_pParentEntity->e_ThinkFunc = thinkF_G_FreeEntity;
			m_pParentEntity->nextthink = level.time + FRAMETIME;
		}*/
	}
}

// Like a think or move command, this updates various vehicle properties.
bool CAnimalNPC::Update( const usercmd_t *pUmcd )
{
	// Bucking so we can't do anything.
	if ( m_ulFlags & VEH_BUCKING || m_ulFlags & VEH_FLYING || m_ulFlags & VEH_CRASHING )
	{
		m_pParentEntity->client->ps.speed = 0;
		return false;
	}

	return CVehicleNPC::Update( pUmcd );
}

// ProcessMoveCommands the Vehicle.
void CAnimalNPC::ProcessMoveCommands()
{
	/************************************************************************************/
	/*	BEGIN	Here is where we move the vehicle (forward or back or whatever). BEGIN	*/
	/************************************************************************************/

	//Client sets ucmds and such for speed alterations
	float speedInc, speedIdleDec, speedIdle, speedIdleAccel, speedMin, speedMax;

	speedIdleDec = m_pVehicleInfo->decelIdle * m_fTimeModifier;
	speedMax = m_pVehicleInfo->speedMax;

	speedIdle = m_pVehicleInfo->speedIdle;
	speedIdleAccel = m_pVehicleInfo->accelIdle * m_fTimeModifier;
	speedMin = m_pVehicleInfo->speedMin;

	if ( m_pParentEntity->client->ps.eFlags & EF_EMPTY_VEHICLE  )
	{//drifts to a stop
		speedInc = speedIdle * m_fTimeModifier;
		VectorClear( client->ps.moveDir );
		//m_ucmd.forwardmove = 127;
		m_pParentEntity->client->ps.speed = 0;
	}
	else
	{
		speedInc = m_pVehicleInfo->acceleration * m_fTimeModifier;
	}

	if ( m_pParentEntity->client->ps.speed || client->ps.groundEntityNum == ENTITYNUM_NONE  ||
		 m_ucmd.forwardmove || m_ucmd.upmove > 0 )
	{ 
		if ( m_ucmd.forwardmove > 0 && speedInc )
		{
			m_pParentEntity->client->ps.speed += speedInc;
		}
		else if ( m_ucmd.forwardmove < 0 )
		{
			if ( m_pParentEntity->client->ps.speed > speedIdle )
			{
				m_pParentEntity->client->ps.speed -= speedInc;
			}
			else if ( client->ps.speed > speedMin )
			{
				m_pParentEntity->client->ps.speed -= speedIdleDec;
			}
		}
		// No input, so coast to stop.
		else if ( m_pParentEntity->client->ps.speed > 0.0f )
		{
			m_pParentEntity->client->ps.speed -= speedIdleDec;
			if ( m_pParentEntity->client->ps.speed < 0.0f )
			{
				m_pParentEntity->client->ps.speed = 0.0f;
			}
		}
		else if ( m_pParentEntity->client->ps.speed < 0.0f )
		{
			m_pParentEntity->client->ps.speed += speedIdleDec;
			if ( m_pParentEntity->client->ps.speed > 0.0f )
			{
				m_pParentEntity->client->ps.speed = 0.0f;
			}
		}
	}
	else
	{
		if ( m_ucmd.forwardmove < 0 )
		{
			m_ucmd.forwardmove = 0;
		}
		if ( m_ucmd.upmove < 0 )
		{
			m_ucmd.upmove = 0;
		}

		m_ucmd.rightmove = 0;

		/*if ( !m_pVehicleInfo->strafePerc 
			|| (!g_speederControlScheme->value && !m_pParentEntity->s.number) )
		{//if in a strafe-capable vehicle, clear strafing unless using alternate control scheme
			m_ucmd.rightmove = 0;
		}*/
	}

	float fWalkSpeedMax = speedMax * 0.275f;
	if ( m_ucmd.buttons & BUTTON_WALKING && m_pParentEntity->client->ps.speed > fWalkSpeedMax )
	{
		m_pParentEntity->client->ps.speed = fWalkSpeedMax;
	}
	else if ( m_pParentEntity->client->ps.speed > speedMax )
	{
		m_pParentEntity->client->ps.speed = speedMax;
	}
	else if ( m_pParentEntity->client->ps.speed < speedMin )
	{
		m_pParentEntity->client->ps.speed = speedMin;
	}

	/********************************************************************************/
	/*	END Here is where we move the vehicle (forward or back or whatever). END	*/
	/********************************************************************************/
}

// ProcessOrientCommands the Vehicle.
void CAnimalNPC::ProcessOrientCommands()
{
	/********************************************************************************/
	/*	BEGIN	Here is where make sure the vehicle is properly oriented.	BEGIN	*/
	/********************************************************************************/

	gentity_t *rider = m_pParentEntity->owner;
	if ( !rider || !rider->client )
	{
		rider = m_pParentEntity;
	}

	float speed;
	speed = VectorLength( m_pParentEntity->client->ps.velocity );

	// If the player is the rider...
	if ( !rider->s.number )
	{//FIXME: use the vehicle's turning stat in this calc
		m_vOrientation[YAW] = rider->client->ps.viewangles[YAW];
	}
	else
	{
		float turnSpeed = m_pVehicleInfo->turningSpeed;
		if ( !m_pVehicleInfo->turnWhenStopped 
			&& !m_pParentEntity->client->ps.speed )//FIXME: or !m_ucmd.forwardmove?
		{//can't turn when not moving
			//FIXME: or ramp up to max turnSpeed?
			turnSpeed = 0.0f;
		}
		if ( !rider || rider->NPC )
		{//help NPCs out some
			turnSpeed *= 2.0f;
			if ( m_pParentEntity->client->ps.speed > 200.0f )
			{
				turnSpeed += turnSpeed * m_pParentEntity->client->ps.speed/200.0f*0.05f;
			}
		}
		turnSpeed *= m_fTimeModifier;

		//default control scheme: strafing turns, mouselook aims
		if ( m_ucmd.rightmove < 0 )
		{
			m_vOrientation[YAW] += turnSpeed;
		}
		else if ( m_ucmd.rightmove > 0 )
		{
			m_vOrientation[YAW] -= turnSpeed;
		}

		if ( m_iArmor <= 25 )
		{//damaged badly
		}
	}

	/********************************************************************************/
	/*	END	Here is where make sure the vehicle is properly oriented.	END			*/
	/********************************************************************************/
}

extern void PM_SetAnim(pmove_t	*pm,int setAnimParts,int anim,int setAnimFlags, int blendTime);
extern int PM_AnimLength( int index, animNumber_t anim );

// This function makes sure that the vehicle is properly animated.
void CAnimalNPC::AnimateVehicle()
{
	animNumber_t Anim = BOTH_VT_IDLE; 
	int iFlags = SETANIM_FLAG_NORMAL, iBlend = 300;

	// We're dead (boarding is reused here so I don't have to make another variable :-).
	if ( m_pParentEntity->health <= 0 ) 
	{
		if ( m_iBoarding != -999 )	// Animate the death just once!
		{
			m_iBoarding = -999;
			iFlags = SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD; 

			// FIXME! Why do you keep repeating over and over!!?!?!? Bastard!
			NPC_SetAnim( m_pParentEntity, SETANIM_LEGS, BOTH_VT_DEATH1, iFlags, iBlend );
		}
		return;
	}

	// If they're bucking, play the animation and leave...
	if ( m_pParentEntity->client->ps.legsAnim == BOTH_VT_BUCK )
	{
		// Done with animation? Erase the flag.
		if ( m_pParentEntity->client->ps.legsAnimTimer <= 0 )
		{
			m_ulFlags &= ~VEH_BUCKING;
		}
		else
		{
			return;
		}
	}
	else if ( m_ulFlags & VEH_BUCKING )
	{
		iFlags = SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD;
		Anim = BOTH_VT_BUCK;
		iBlend = 500;
		NPC_SetAnim( m_pParentEntity, SETANIM_LEGS, BOTH_VT_BUCK, iFlags, iBlend );
		return;
	}

	// Boarding animation.
	if ( m_iBoarding != 0 )
	{
		// We've just started boarding, set the amount of time it will take to finish boarding.
		if ( m_iBoarding < 0 )
		{
			// Boarding from left...
			if ( m_iBoarding == -1 )
			{
				Anim = BOTH_VT_MOUNT_L;
			}
			else if ( m_iBoarding == -2 )
			{
				Anim = BOTH_VT_MOUNT_R;
			}
			else if ( m_iBoarding == -3 )
			{
				Anim = BOTH_VT_MOUNT_B;
			}

			// Set the delay time (which happens to be the time it takes for the animation to complete).
			// NOTE: Here I made it so the delay is actually 70% (0.7f) of the animation time.
			int iAnimLen = PM_AnimLength( m_pParentEntity->client->clientInfo.animFileIndex, Anim ) * 0.7f;
			m_iBoarding = level.time + iAnimLen;

			// Set the animation, which won't be interrupted until it's completed.
			// TODO: But what if he's killed? Should the animation remain persistant???
			iFlags = SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD;

			NPC_SetAnim( m_pParentEntity, SETANIM_LEGS, Anim, iFlags, iBlend );
			return;
		}
		// Otherwise we're done.
		else if ( m_iBoarding <= level.time )
		{
			m_iBoarding = 0;
		}
	}

	// Percentage of maximum speed relative to current speed.
	//float fSpeed = VectorLength( client->ps.velocity );
	float fSpeedPercToMax = m_pParentEntity->client->ps.speed / m_pVehicleInfo->speedMax; 

	// If we're moving...
	if ( fSpeedPercToMax > 0.0f ) //fSpeedPercToMax >= 0.85f )
	{  
		iBlend = 300;
		iFlags = SETANIM_FLAG_OVERRIDE;
		float fYawDelta = m_vPrevOrientation[YAW] - m_vOrientation[YAW];

		// NOTE: Mikes suggestion for fixing the stuttering walk (left/right) is to maintain the
		// current frame between animations. I have no clue how to do this and have to work on other
		// stuff so good luck to him :-p AReis

		// If we're walking (or our speed is less than .275%)...
		if ( ( m_ucmd.buttons & BUTTON_WALKING ) || fSpeedPercToMax < 0.275f )
		{ 
			// Make them lean if we're turning.
			/*if ( fYawDelta < -0.0001f )
			{
				Anim = BOTH_VT_WALK_FWD_L;
			}
			else if ( fYawDelta > 0.0001 )
			{
				Anim = BOTH_VT_WALK_FWD_R;
			}
			else*/
			{
				Anim = BOTH_VT_WALK_FWD;
			}
		}
		// otherwise we're running.
		else
		{
			// Make them lean if we're turning.
			/*if ( fYawDelta < -0.0001f )
			{
				Anim = BOTH_VT_RUN_FWD_L;
			}
			else if ( fYawDelta > 0.0001 )
			{
				Anim = BOTH_VT_RUN_FWD_R;
			}
			else*/
			{
				Anim = BOTH_VT_RUN_FWD;
			}
		}
	}
	else
	{
		// Going in reverse...
		if ( fSpeedPercToMax < -0.018f )
		{
			iFlags = SETANIM_FLAG_OVERRIDE;
			Anim = BOTH_VT_WALK_REV;
			iBlend = 500;
		}
		else
		{
			int iChance = Q_irand( 0, 20000 ); 

			// Every once in a while buck or do a different idle...
			iFlags = SETANIM_FLAG_NORMAL | SETANIM_FLAG_RESTART | SETANIM_FLAG_HOLD; 
			iBlend = 600;
			Anim = BOTH_VT_IDLE;
			if ( iChance <= 15000 ) 
			{
				Anim = BOTH_VT_IDLE;
			}
			else if ( iChance > 15000 && iChance <= 19990 )
			{
				Anim = BOTH_VT_IDLE1;
			}
			else if ( iChance > 19990 && iChance <= 20000 )
			{
				//m_ulFlags |= VEH_BUCKING;
			}
		}
	}

	// Crashing.
	if ( m_ulFlags & VEH_CRASHING )
	{
		m_ulFlags &= ~VEH_CRASHING;	// Remove the flag, we are doing the animation.
		iBlend = 0;
		Anim = BOTH_VT_LAND; 
		iFlags = SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD;
	}

	// In the Air.
	if ( m_ulFlags & VEH_FLYING ) 
	{
		m_ulFlags &= ~VEH_FLYING;
		iBlend = 10;
		Anim = BOTH_VT_AIR;
		iFlags = SETANIM_FLAG_OVERRIDE;
	}

	NPC_SetAnim( m_pParentEntity, SETANIM_LEGS, Anim, iFlags, iBlend );
}

// This function makes sure that the rider's in this vehicle are properly animated.
void CAnimalNPC::AnimateRiders()
{
	animNumber_t Anim = BOTH_VT_IDLE;
	int iFlags = SETANIM_FLAG_NORMAL, iBlend = 500;

	// Boarding animation.
	if ( m_iBoarding != 0 )
	{
		// We've just started moarding, set the amount of time it will take to finish moarding.
		if ( m_iBoarding < 0 )
		{
			iBlend = 0;
			// Boarding from left...
			if ( m_iBoarding == -1 )
			{
				Anim = BOTH_VT_MOUNT_L;
			}
			else if ( m_iBoarding == -2 )
			{
				Anim = BOTH_VT_MOUNT_R;
			}
			else if ( m_iBoarding == -3 )
			{
				Anim = BOTH_VT_MOUNT_B;
			}

			// Set the animation, which won't be interrupted until it's completed.
			// TODO: But what if he's killed? Should the animation remain persistant???
			iFlags = SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD;
			
			NPC_SetAnim( m_pPilot, SETANIM_BOTH, Anim, iFlags, iBlend );
			return;
		}
		// Otherwise we're done.
		else if ( m_iBoarding <= level.time )
		{
			m_iBoarding = 0;
		}
	}

	// Percentage of maximum speed relative to current speed.
	float fSpeedPercToMax = m_pParentEntity->client->ps.speed / m_pVehicleInfo->speedMax;

	// Going in reverse...
	if ( fSpeedPercToMax < -0.01f )
	{
		Anim = BOTH_VT_WALK_REV;
		iBlend = 600;
	}
	else
	{
		// If they have a weapon...
		if ( m_pPilot->client->ps.weapon != WP_NONE && m_pPilot->client->ps.weapon != WP_MELEE )
		{
			// If they're firing, play the right fire animation.
			if ( m_ucmd.buttons & ( BUTTON_ATTACK | BUTTON_ALT_ATTACK ) ) 
			{
				iFlags = SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD;
				iBlend = 0;
				switch ( m_pPilot->client->ps.weapon )
				{
					case WP_SABER:
						// If we're already in an attack animation, leave (let it continue).
						if ( m_pPilot->client->ps.torsoAnimTimer > 0 && (m_pPilot->client->ps.torsoAnim == BOTH_VT_ATR_S ||
								m_pPilot->client->ps.torsoAnim == BOTH_VT_ATL_S) )
						{
							//FIXME: no need to even call the PM_SetAnim at all in this case
							Anim = (animNumber_t)m_pPilot->client->ps.torsoAnim;
							iFlags = SETANIM_FLAG_NORMAL;
							break;
						}

						// Start the attack.
						if ( m_ucmd.rightmove > 0 )	//right side attack
						{
							Anim = BOTH_VT_ATR_S;
						}
						else if ( m_ucmd.rightmove < 0 )	//left-side attack
						{
							Anim = BOTH_VT_ATL_S;
						}
						else	//random
						{
							//FIXME: alternate back and forth or auto-aim?
							if ( !Q_irand( 0, 1 ) )
							{
								Anim = BOTH_VT_ATR_S;
							}
							else
							{
								Anim = BOTH_VT_ATL_S;
							}
						}
						break;

					case WP_BLASTER:
						// Override the shoot anim.
						if ( m_pPilot->client->ps.torsoAnim == BOTH_ATTACK3 )
						{
							if ( m_ucmd.rightmove > 0 )			//right side attack
							{
								Anim = BOTH_VT_ATR_G;
							}
							else if ( m_ucmd.rightmove < 0 )	//left side
							{
								Anim = BOTH_VT_ATL_G;
							}
							else	//frontal
							{
								Anim = BOTH_VT_ATF_G;
							}
						}
						else if ( m_pPilot->client->ps.torsoAnim == BOTH_VT_ATR_G ||
								  m_pPilot->client->ps.torsoAnim == BOTH_VT_ATL_G ||
								  m_pPilot->client->ps.torsoAnim == BOTH_VT_ATF_G )
						{
							Anim = (animNumber_t)m_pPilot->client->ps.torsoAnim;
							iFlags = SETANIM_FLAG_RESTART;
						}
						else
						{
							Anim = (animNumber_t)m_pPilot->client->ps.torsoAnim;
							iFlags = 0;
						}
						break;

					case WP_THERMAL:
						// Override throw animation.
						if ( m_pPilot->client->ps.torsoAnim == BOTH_THERMAL_THROW 
							|| m_pPilot->client->ps.torsoAnim == BOTH_ATTACK10 )
						{
							if ( m_ucmd.rightmove > 0 )			//right side attack
							{
								Anim = BOTH_VT_ATR_T;
							}
							else if ( m_ucmd.rightmove < 0 )	//left side
							{
								Anim = BOTH_VT_ATL_T;
							}
							else	//frontal
							{
								Anim = BOTH_VT_ATF_T;
							}
							break;
						}
						else
						{
							Anim = (animNumber_t)m_pPilot->client->ps.torsoAnim;
							iFlags = 0;
						}
				}
			}
			// They're not firing so play the Idle for the weapon.
			else
			{
				iFlags = SETANIM_FLAG_NORMAL;
				iBlend = 1500;

				switch ( m_pPilot->client->ps.weapon )
				{
					case WP_SABER:
						Anim = BOTH_VT_IDLE_S;
						break;

					case WP_BLASTER:
						Anim = BOTH_VT_IDLE_G;
						break;

					case WP_THERMAL:
						Anim = BOTH_VT_IDLE_T;
						break;
				}
			}
		}
		// Do normal Idle's and other things (crash, air move, etc...)
		else
		{
			// In the Air.
			if ( m_ulFlags & VEH_FLYING )
			{
				iBlend = 1500;
				Anim = BOTH_VT_AIR;
				iFlags = SETANIM_FLAG_OVERRIDE;
			}
			else
			{
				// If we're moving...
				if ( fSpeedPercToMax > 0.0f ) //fSpeedPercToMax >= 0.85f )
				{
					// If we're walking (or our speed is less than 0.275f%)...
					if ( ( m_ucmd.buttons & BUTTON_WALKING ) || fSpeedPercToMax < 0.275f )
					{
						iBlend = 800;
						// Make them lean if we're turning.
						if ( m_ucmd.rightmove < 0 )
						{
							Anim = BOTH_VT_WALK_FWD_L;
						}
						else if ( m_ucmd.rightmove > 0 )
						{
							Anim = BOTH_VT_WALK_FWD_R;
						}
						else
						{
							Anim = BOTH_VT_WALK_FWD;
						}
					}
					// otherwise we're running.
					else
					{
						iBlend = 1000;
						// Make them lean if we're turning.
						if ( m_ucmd.rightmove < 0 )
						{
							Anim = BOTH_VT_RUN_FWD_L;
						}
						else if ( m_ucmd.rightmove > 0 )
						{
							Anim = BOTH_VT_RUN_FWD_R;
						}
						else
						{
							Anim = BOTH_VT_RUN_FWD;
						}
					}
				}
				else
				{
					int iChance = Q_irand( 0, 20000 );

					// Every once in a while buck or do a different idle...
					iFlags = SETANIM_FLAG_NORMAL | SETANIM_FLAG_RESTART;
					iBlend = 600;
					Anim = BOTH_VT_IDLE;
					if ( iChance > 19990 && iChance <= 20000 )
					{
						m_ulFlags |= VEH_BUCKING;
						Anim = BOTH_VT_BUCK;
						iFlags = SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD;
					}
				}
			}
		}
	}
	 
	NPC_SetAnim( m_pPilot, SETANIM_BOTH, Anim, iFlags, iBlend );
}