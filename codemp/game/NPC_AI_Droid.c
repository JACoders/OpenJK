#include "b_local.h"

//static void R5D2_LookAround( void );
float NPC_GetPainChance( gentity_t *self, int damage );
extern void G_SoundOnEnt( gentity_t *ent, soundChannel_t channel, const char *soundPath );

#define TURN_OFF   0x00000100

//Local state enums
enum
{
	LSTATE_NONE = 0,
	LSTATE_BACKINGUP,
	LSTATE_SPINNING,
	LSTATE_PAIN,
	LSTATE_DROP
};

/*
-------------------------
R2D2_PartsMove
-------------------------
*/
void R2D2_PartsMove(void)
{
	// Front 'eye' lense
	if ( TIMER_Done(NPC,"eyeDelay") )
	{
		NPC->pos1[1] = AngleNormalize360( NPC->pos1[1]);

		NPC->pos1[0]+=Q_irand( -20, 20 );	// Roll	
		NPC->pos1[1]=Q_irand( -20, 20 );	
		NPC->pos1[2]=Q_irand( -20, 20 );	

		/*
		if (NPC->genericBone1)
		{
			gi.G2API_SetBoneAnglesIndex( &NPC->ghoul2[NPC->playerModel], NPC->genericBone1, NPC->pos1, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL ); 
		}
		*/
		NPC_SetBoneAngles(NPC, "f_eye", NPC->pos1);
		

		TIMER_Set( NPC, "eyeDelay", Q_irand( 100, 1000 ) );
	}
}

/*
-------------------------
NPC_BSDroid_Idle
-------------------------
*/
void Droid_Idle( void )
{
//	VectorCopy( NPCInfo->investigateGoal, lookPos );

//	NPC_FacePosition( lookPos );
}

/*
-------------------------
R2D2_TurnAnims
-------------------------
*/
void R2D2_TurnAnims ( void )
{
	float turndelta;
	int		anim;

	turndelta = AngleDelta(NPC->r.currentAngles[YAW], NPCInfo->desiredYaw);

	if ((fabs(turndelta) > 20) && ((NPC->client->NPC_class == CLASS_R2D2) || (NPC->client->NPC_class == CLASS_R5D2)))
	{
		anim = NPC->client->ps.legsAnim;
		if (turndelta<0)
		{
			if (anim != BOTH_TURN_LEFT1)
			{
				NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_TURN_LEFT1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			}
		}
		else
		{
			if (anim != BOTH_TURN_RIGHT1)
			{
				NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_TURN_RIGHT1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			}
		}
	}
	else
	{
			NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_RUN1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	}

}

/*
-------------------------
Droid_Patrol
-------------------------
*/
void Droid_Patrol( void )
{

	NPC->pos1[1] = AngleNormalize360( NPC->pos1[1]);

	if ( NPC->client && NPC->client->NPC_class != CLASS_GONK )
	{
		if (NPC->client->NPC_class != CLASS_R5D2)
		{ //he doesn't have an eye.
			R2D2_PartsMove();		// Get his eye moving.
		}
		R2D2_TurnAnims();
	}

	//If we have somewhere to go, then do that
	if ( UpdateGoal() )
	{
		ucmd.buttons |= BUTTON_WALKING;
		NPC_MoveToGoal( qtrue );

		if( NPC->client && NPC->client->NPC_class == CLASS_MOUSE )
		{
			NPCInfo->desiredYaw += sin(level.time*.5) * 25; // Weaves side to side a little

			if (TIMER_Done(NPC,"patrolNoise"))
			{
				G_SoundOnEnt( NPC, CHAN_AUTO, va("sound/chars/mouse/misc/mousego%d.wav", Q_irand(1, 3)) );

				TIMER_Set( NPC, "patrolNoise", Q_irand( 2000, 4000 ) );
			}
		}
		else if( NPC->client && NPC->client->NPC_class == CLASS_R2D2 )
		{
			if (TIMER_Done(NPC,"patrolNoise"))
			{
				G_SoundOnEnt( NPC, CHAN_AUTO, va("sound/chars/r2d2/misc/r2d2talk0%d.wav",	Q_irand(1, 3)) );

				TIMER_Set( NPC, "patrolNoise", Q_irand( 2000, 4000 ) );
			}
		}
		else if( NPC->client && NPC->client->NPC_class == CLASS_R5D2 )
		{
			if (TIMER_Done(NPC,"patrolNoise"))
			{
				G_SoundOnEnt( NPC, CHAN_AUTO, va("sound/chars/r5d2/misc/r5talk%d.wav", Q_irand(1, 4)) );

				TIMER_Set( NPC, "patrolNoise", Q_irand( 2000, 4000 ) );
			}
		}
		if( NPC->client && NPC->client->NPC_class == CLASS_GONK )
		{
			if (TIMER_Done(NPC,"patrolNoise"))
			{
				G_SoundOnEnt( NPC, CHAN_AUTO, va("sound/chars/gonk/misc/gonktalk%d.wav", Q_irand(1, 2)) );

				TIMER_Set( NPC, "patrolNoise", Q_irand( 2000, 4000 ) );
			}
		}
//		else
//		{
//			R5D2_LookAround();
//		}
	}

	NPC_UpdateAngles( qtrue, qtrue );

}

/*
-------------------------
Droid_Run
-------------------------
*/
void Droid_Run( void )
{
	R2D2_PartsMove();

	if ( NPCInfo->localState == LSTATE_BACKINGUP )
	{
		ucmd.forwardmove = -127;
		NPCInfo->desiredYaw += 5; 

		NPCInfo->localState = LSTATE_NONE;	// So he doesn't constantly backup.
	}
	else 
	{
		ucmd.forwardmove = 64;
		//If we have somewhere to go, then do that
		if ( UpdateGoal() )
		{
			if (NPC_MoveToGoal( qfalse ))
			{
				NPCInfo->desiredYaw += sin(level.time*.5) * 5; // Weaves side to side a little
			}
		}
	}

	NPC_UpdateAngles( qtrue, qtrue );
}

/*
-------------------------
void Droid_Spin( void )
-------------------------
*/
void Droid_Spin( void )
{
	vec3_t dir = {0,0,1};

	R2D2_TurnAnims();

						
	// Head is gone, spin and spark
	if ( NPC->client->NPC_class == CLASS_R5D2 
		|| NPC->client->NPC_class == CLASS_R2D2 )
	{
		// No head?
		if (trap_G2API_GetSurfaceRenderStatus( NPC->ghoul2, 0, "head" )>0)
		{
			if (TIMER_Done(NPC,"smoke") && !TIMER_Done(NPC,"droidsmoketotal"))
			{
				TIMER_Set( NPC, "smoke", 100);
				G_PlayEffectID( G_EffectIndex("volumetric/droid_smoke") , NPC->r.currentOrigin,dir);
			}

			if (TIMER_Done(NPC,"droidspark"))
			{
				TIMER_Set( NPC, "droidspark", Q_irand(100,500));
				G_PlayEffectID( G_EffectIndex("sparks/spark"), NPC->r.currentOrigin,dir);
			}

			ucmd.forwardmove = Q_irand( -64, 64);

			if (TIMER_Done(NPC,"roam"))
			{	
				TIMER_Set( NPC, "roam", Q_irand( 250, 1000 ) );
				NPCInfo->desiredYaw = Q_irand( 0, 360 ); // Go in random directions
			}
		}
		else
		{
			if (TIMER_Done(NPC,"roam"))
			{
				NPCInfo->localState = LSTATE_NONE;
			}
			else
			{
				NPCInfo->desiredYaw = AngleNormalize360(NPCInfo->desiredYaw + 40); // Spin around
			}
		}
	}
	else 
	{
		if (TIMER_Done(NPC,"roam"))
		{
			NPCInfo->localState = LSTATE_NONE;
		}
		else
		{
			NPCInfo->desiredYaw = AngleNormalize360(NPCInfo->desiredYaw + 40); // Spin around
		}
	}

	NPC_UpdateAngles( qtrue, qtrue );
}

/*
-------------------------
NPC_BSDroid_Pain
-------------------------
*/
void NPC_Droid_Pain(gentity_t *self, gentity_t *attacker, int damage)
{
	gentity_t *other = attacker;
	int		anim;
	int		mod = gPainMOD;
	float	pain_chance;

	VectorCopy( self->NPC->lastPathAngles, self->s.angles );

	if ( self->client->NPC_class == CLASS_R5D2 )
	{
		pain_chance = NPC_GetPainChance( self, damage );

		// Put it in pain
		if ( mod == MOD_DEMP2 || mod == MOD_DEMP2_ALT || random() < pain_chance )	// Spin around in pain? Demp2 always does this
		{
			// Health is between 0-30 or was hit by a DEMP2 so pop his head
			if ( !self->s.m_iVehicleNum
				&& ( self->health < 30 || mod == MOD_DEMP2 || mod == MOD_DEMP2_ALT ) )
			{
				if (!(self->spawnflags & 2))	// Doesn't have to ALWAYSDIE
				{
					if ((self->NPC->localState != LSTATE_SPINNING) && 
						(!trap_G2API_GetSurfaceRenderStatus( self->ghoul2, 0, "head" )))
					{
						NPC_SetSurfaceOnOff( self, "head", TURN_OFF );

						if ( self->client->ps.m_iVehicleNum )
						{
							vec3_t	up;
							AngleVectors( self->r.currentAngles, NULL, NULL, up );
							G_PlayEffectID( G_EffectIndex("chunks/r5d2head_veh"), self->r.currentOrigin, up );
						}
						else
						{
							G_PlayEffectID( G_EffectIndex("small_chunks") , self->r.currentOrigin, vec3_origin );
							G_PlayEffectID( G_EffectIndex("chunks/r5d2head"), self->r.currentOrigin, vec3_origin );
						}

						//self->s.powerups |= ( 1 << PW_SHOCKED );
						//self->client->ps.powerups[PW_SHOCKED] = level.time + 3000;
						self->client->ps.electrifyTime = level.time + 3000;

						TIMER_Set( self, "droidsmoketotal", 5000);
						TIMER_Set( self, "droidspark", 100);
						self->NPC->localState = LSTATE_SPINNING;
					}
				}
			}
			// Just give him normal pain for a little while
			else
			{
				anim = self->client->ps.legsAnim;

				if ( anim == BOTH_STAND2 )	// On two legs?
				{
					anim = BOTH_PAIN1;
				}
				else						// On three legs
				{
					anim = BOTH_PAIN2;
				}

				NPC_SetAnim( self, SETANIM_BOTH, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );

				// Spin around in pain
				self->NPC->localState = LSTATE_SPINNING;
				TIMER_Set( self, "roam", Q_irand(1000,2000));
			} 
		}
	}
	else if (self->client->NPC_class == CLASS_MOUSE)
	{
		if ( mod == MOD_DEMP2 || mod == MOD_DEMP2_ALT )
		{
			self->NPC->localState = LSTATE_SPINNING;
			//self->s.powerups |= ( 1 << PW_SHOCKED );
			//self->client->ps.powerups[PW_SHOCKED] = level.time + 3000;
			self->client->ps.electrifyTime = level.time + 3000;
		}
		else
		{
			self->NPC->localState = LSTATE_BACKINGUP;
		}

		self->NPC->scriptFlags &= ~SCF_LOOK_FOR_ENEMIES;
	}
	else if ((self->client->NPC_class == CLASS_R2D2))
	{

		pain_chance = NPC_GetPainChance( self, damage );

		if ( mod == MOD_DEMP2 || mod == MOD_DEMP2_ALT || random() < pain_chance )	// Spin around in pain? Demp2 always does this
		{
			// Health is between 0-30 or was hit by a DEMP2 so pop his head
			if ( !self->s.m_iVehicleNum
				&& ( self->health < 30 || mod == MOD_DEMP2 || mod == MOD_DEMP2_ALT ) )
			{
				if (!(self->spawnflags & 2))	// Doesn't have to ALWAYSDIE
				{
					if ((self->NPC->localState != LSTATE_SPINNING) && 
						(!trap_G2API_GetSurfaceRenderStatus( self->ghoul2, 0, "head" )))
					{
						NPC_SetSurfaceOnOff( self, "head", TURN_OFF );

						if ( self->client->ps.m_iVehicleNum )
						{
							vec3_t	up;
							AngleVectors( self->r.currentAngles, NULL, NULL, up );
							G_PlayEffectID( G_EffectIndex("chunks/r2d2head_veh"), self->r.currentOrigin, up );
						}
						else
						{
							G_PlayEffectID( G_EffectIndex("small_chunks") , self->r.currentOrigin, vec3_origin );
							G_PlayEffectID( G_EffectIndex("chunks/r2d2head"), self->r.currentOrigin, vec3_origin );
						}

						//self->s.powerups |= ( 1 << PW_SHOCKED );
						//self->client->ps.powerups[PW_SHOCKED] = level.time + 3000;
						self->client->ps.electrifyTime = level.time + 3000;

						TIMER_Set( self, "droidsmoketotal", 5000);
						TIMER_Set( self, "droidspark", 100);
						self->NPC->localState = LSTATE_SPINNING;
					}
				}
			}
			// Just give him normal pain for a little while
			else
			{
				anim = self->client->ps.legsAnim;

				if ( anim == BOTH_STAND2 )	// On two legs?
				{
					anim = BOTH_PAIN1;
				}
				else						// On three legs
				{
					anim = BOTH_PAIN2;
				}

				NPC_SetAnim( self, SETANIM_BOTH, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );

				// Spin around in pain
				self->NPC->localState = LSTATE_SPINNING;
				TIMER_Set( self, "roam", Q_irand(1000,2000));
			}
		} 
	}
	else if ( self->client->NPC_class == CLASS_INTERROGATOR && ( mod == MOD_DEMP2 || mod == MOD_DEMP2_ALT ) && other )
	{
		vec3_t dir;

		VectorSubtract( self->r.currentOrigin, other->r.currentOrigin, dir );
		VectorNormalize( dir );

		VectorMA( self->client->ps.velocity, 550, dir, self->client->ps.velocity );
		self->client->ps.velocity[2] -= 127;
	}

	NPC_Pain( self, attacker, damage);
}


/*
-------------------------
Droid_Pain
-------------------------
*/
void Droid_Pain(void)
{
	if (TIMER_Done(NPC,"droidpain"))	//He's done jumping around
	{
		NPCInfo->localState = LSTATE_NONE;
	}
}

/*
-------------------------
NPC_Mouse_Precache
-------------------------
*/
void NPC_Mouse_Precache( void )
{
	int	i;

	for (i = 1; i < 4; i++)
	{
		G_SoundIndex( va( "sound/chars/mouse/misc/mousego%d.wav", i ) );
	}

	G_EffectIndex( "env/small_explode" );
	G_SoundIndex( "sound/chars/mouse/misc/death1" );
	G_SoundIndex( "sound/chars/mouse/misc/mouse_lp" );
}

/*
-------------------------
NPC_R5D2_Precache
-------------------------
*/
void NPC_R5D2_Precache(void)
{
	int i;

	for ( i = 1; i < 5; i++)
	{
		G_SoundIndex( va( "sound/chars/r5d2/misc/r5talk%d.wav", i ) );
	}
	//G_SoundIndex( "sound/chars/r5d2/misc/falling1.wav" );
	G_SoundIndex( "sound/chars/mark2/misc/mark2_explo" ); // ??
	G_SoundIndex( "sound/chars/r2d2/misc/r2_move_lp2.wav" );
	G_EffectIndex( "env/med_explode");
	G_EffectIndex( "volumetric/droid_smoke" );
	G_EffectIndex("sparks/spark");
	G_EffectIndex( "chunks/r5d2head");
	G_EffectIndex( "chunks/r5d2head_veh");
}

/*
-------------------------
NPC_R2D2_Precache
-------------------------
*/
void NPC_R2D2_Precache(void)
{
	int i;

	for ( i = 1; i < 4; i++)
	{
		G_SoundIndex( va( "sound/chars/r2d2/misc/r2d2talk0%d.wav", i ) );
	}
	//G_SoundIndex( "sound/chars/r2d2/misc/falling1.wav" );
	G_SoundIndex( "sound/chars/mark2/misc/mark2_explo" ); // ??
	G_SoundIndex( "sound/chars/r2d2/misc/r2_move_lp.wav" );
	G_EffectIndex( "env/med_explode");
	G_EffectIndex( "volumetric/droid_smoke" );
	G_EffectIndex("sparks/spark");
	G_EffectIndex( "chunks/r2d2head");
	G_EffectIndex( "chunks/r2d2head_veh");
}

/*
-------------------------
NPC_Gonk_Precache
-------------------------
*/
void NPC_Gonk_Precache( void )
{
	G_SoundIndex("sound/chars/gonk/misc/gonktalk1.wav");
	G_SoundIndex("sound/chars/gonk/misc/gonktalk2.wav");

	G_SoundIndex("sound/chars/gonk/misc/death1.wav");
	G_SoundIndex("sound/chars/gonk/misc/death2.wav");
	G_SoundIndex("sound/chars/gonk/misc/death3.wav");

	G_EffectIndex( "env/med_explode");
}

/*
-------------------------
NPC_Protocol_Precache
-------------------------
*/
void NPC_Protocol_Precache( void )
{
	G_SoundIndex( "sound/chars/mark2/misc/mark2_explo" );
	G_EffectIndex( "env/med_explode");
}

/*
static void R5D2_OffsetLook( float offset, vec3_t out )
{
	vec3_t	angles, forward, temp;

	GetAnglesForDirection( NPC->r.currentOrigin, NPCInfo->investigateGoal, angles );
	angles[YAW] += offset;
	AngleVectors( angles, forward, NULL, NULL );
	VectorMA( NPC->r.currentOrigin, 64, forward, out );
	
	CalcEntitySpot( NPC, SPOT_HEAD, temp );
	out[2] = temp[2];
}
*/

/*
-------------------------
R5D2_LookAround
-------------------------
*/
/*
static void R5D2_LookAround( void )
{
	vec3_t	lookPos;
	float	perc = (float) ( level.time - NPCInfo->pauseTime ) / (float) NPCInfo->investigateDebounceTime;

	//Keep looking at the spot
	if ( perc < 0.25 )
	{
		VectorCopy( NPCInfo->investigateGoal, lookPos );
	}
	else if ( perc < 0.5f )		//Look up but straight ahead
	{
		R5D2_OffsetLook( 0.0f, lookPos );
	}
	else if ( perc < 0.75f )	//Look right
	{
		R5D2_OffsetLook( 45.0f, lookPos );
	}
	else	//Look left
	{
		R5D2_OffsetLook( -45.0f, lookPos );
	}

	NPC_FacePosition( lookPos );
}

*/

/*
-------------------------
NPC_BSDroid_Default
-------------------------
*/
void NPC_BSDroid_Default( void )
{

	if ( NPCInfo->localState == LSTATE_SPINNING )
	{
		Droid_Spin();
	}
	else if ( NPCInfo->localState == LSTATE_PAIN )
	{
		Droid_Pain();
	}
	else if ( NPCInfo->localState == LSTATE_DROP )
	{
		NPC_UpdateAngles( qtrue, qtrue );
		ucmd.upmove = crandom() * 64;
	}
	else if ( NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES )
	{
		Droid_Patrol();
	}
	else
	{
		Droid_Run();
	}
}
