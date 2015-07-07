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

#include "g_headers.h"

#include "g_local.h"
#include "g_functions.h"
#include "../cgame/cg_local.h"
#include "Q3_Interface.h"
#include "wp_saber.h"
#include "g_icarus.h"

#ifdef _DEBUG
	#include <float.h>
#endif //_DEBUG

#define	SLOWDOWN_DIST	128.0f
#define	MIN_NPC_SPEED	16.0f

extern qboolean Q3_TaskIDPending( gentity_t *ent, taskID_t taskType );
extern void G_MaintainFormations(gentity_t *self);
extern void BG_CalculateOffsetAngles( gentity_t *ent, usercmd_t *ucmd );//in bg_pangles.cpp
extern void TryUse( gentity_t *ent );
extern void ChangeWeapon( gentity_t *ent, int newWeapon );
extern void ScoreBoardReset(void);
extern void WP_SaberReflectCheck( gentity_t *self, usercmd_t *ucmd  );
extern void WP_SaberUpdate( gentity_t *self, usercmd_t *ucmd );
extern void WP_SaberStartMissileBlockCheck( gentity_t *self, usercmd_t *ucmd  );
extern void WP_ForcePowersUpdate( gentity_t *self, usercmd_t *ucmd );
extern void WP_SaberInitBladeData( gentity_t *ent );
extern gentity_t *SeekerAcquiresTarget ( gentity_t *ent, vec3_t pos );
extern void FireSeeker( gentity_t *owner, gentity_t *target, vec3_t origin, vec3_t dir );
extern qboolean InFront( vec3_t spot, vec3_t from, vec3_t fromAngles, float threshHold = 0.0f );
extern void NPC_SetLookTarget( gentity_t *self, int entNum, int clearTime );
extern qboolean PM_AdjustAnglesToGripper( gentity_t *gent, usercmd_t *cmd );
extern qboolean PM_AdjustAngleForWallRun( gentity_t *ent, usercmd_t *ucmd, qboolean doMove );
extern qboolean PM_AdjustAnglesForSpinningFlip( gentity_t *ent, usercmd_t *ucmd, qboolean anglesOnly );
extern qboolean PM_AdjustAnglesForBackAttack( gentity_t *ent, usercmd_t *ucmd );
extern qboolean PM_AdjustAnglesForSaberLock( gentity_t *ent, usercmd_t *ucmd );
extern qboolean PM_AdjustAnglesForKnockdown( gentity_t *ent, usercmd_t *ucmd, qboolean angleClampOnly );
extern qboolean PM_HasAnimation( gentity_t *ent, int animation );
extern qboolean PM_SpinningSaberAnim( int anim );
extern qboolean PM_SaberInAttack( int move );
extern int PM_AnimLength( int index, animNumber_t anim );
extern qboolean PM_InKnockDown( playerState_t *ps );
extern qboolean PM_InRoll( playerState_t *ps );
extern void PM_CmdForRoll( int anim, usercmd_t *pCmd );
extern qboolean PM_CrouchAnim( int anim );
extern qboolean PM_FlippingAnim( int anim );
extern qboolean PM_InCartwheel( int anim );
extern qboolean PM_StandingAnim( int anim );
extern qboolean PM_InForceGetUp( playerState_t *ps );
extern void G_CreateG2AttachedWeaponModel( gentity_t *ent, const char *weaponModel );
extern qboolean FlyingCreature( gentity_t *ent );

extern bool		in_camera;
extern qboolean	player_locked;
extern qboolean	stop_icarus;
extern cvar_t	*g_spskill;
extern cvar_t	*g_timescale;
extern cvar_t	*g_saberMoveSpeed;
extern cvar_t	*g_saberAutoBlocking;
extern vmCvar_t	cg_thirdPersonAlpha;
extern vmCvar_t	cg_thirdPersonAutoAlpha;

void ClientEndPowerUps( gentity_t *ent );

int G_FindLookItem( gentity_t *self )
{
//FIXME: should be a more intelligent way of doing this, like auto aim?
//closest, most in front... did damage to... took damage from?  How do we know who the player is focusing on?
	gentity_t	*ent;
	int			bestEntNum = ENTITYNUM_NONE;
	gentity_t	*entityList[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		center, mins, maxs, fwdangles, forward, dir;
	int			i, e;
	float		radius = 256;
	float		rating, bestRating = 0.0f;

	//FIXME: no need to do this in 1st person?
	fwdangles[1] = self->client->ps.viewangles[1];
	AngleVectors( fwdangles, forward, NULL, NULL );

	VectorCopy( self->currentOrigin, center );

	for ( i = 0 ; i < 3 ; i++ ) 
	{
		mins[i] = center[i] - radius;
		maxs[i] = center[i] + radius;
	}
	numListedEntities = gi.EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );
	
	if ( !numListedEntities )
	{
		return ENTITYNUM_NONE;
	}

	for ( e = 0 ; e < numListedEntities ; e++ ) 
	{
		ent = entityList[ e ];

		if ( !ent->item )
		{
			continue;
		}
		if ( ent->s.eFlags&EF_NODRAW )
		{
			continue;
		}
		if ( (ent->spawnflags&4/*ITMSF_MONSTER*/) )
		{//NPCs only
			continue;
		}
		if ( !BG_CanItemBeGrabbed( &ent->s, &self->client->ps ) )
		{//don't need it
			continue;
		}
		if ( !gi.inPVS( self->currentOrigin, ent->currentOrigin ) )
		{//not even potentially visible
			continue;
		}

		if ( !G_ClearLOS( self, self->client->renderInfo.eyePoint, ent ) )
		{//can't see him
			continue;
		}
		//rate him based on how close & how in front he is
		VectorSubtract( ent->currentOrigin, center, dir );
		rating = (1.0f-(VectorNormalize( dir )/radius));
		rating *= DotProduct( forward, dir );
		if ( ent->item->giType == IT_HOLDABLE && ent->item->giTag == INV_SECURITY_KEY )
		{//security keys are of the highest importance
			rating *= 2.0f;
		}
		if ( rating > bestRating )
		{
			bestEntNum = ent->s.number;
			bestRating = rating;
		}
	}
	return bestEntNum;
}

extern void CG_SetClientViewAngles( vec3_t angles, qboolean overrideViewEnt );
qboolean G_ClearViewEntity( gentity_t *ent )
{
	if ( !ent->client->ps.viewEntity )
		return qfalse;

	if ( ent->client->ps.viewEntity > 0 && ent->client->ps.viewEntity < ENTITYNUM_NONE )
	{
		if ( &g_entities[ent->client->ps.viewEntity] )
		{
			g_entities[ent->client->ps.viewEntity].svFlags &= ~SVF_BROADCAST;
			if ( g_entities[ent->client->ps.viewEntity].NPC )
			{
				g_entities[ent->client->ps.viewEntity].NPC->controlledTime = 0;
				SetClientViewAngle( &g_entities[ent->client->ps.viewEntity], g_entities[ent->client->ps.viewEntity].currentAngles );
				G_SetAngles( &g_entities[ent->client->ps.viewEntity], g_entities[ent->client->ps.viewEntity].currentAngles );
				VectorCopy( g_entities[ent->client->ps.viewEntity].currentAngles, g_entities[ent->client->ps.viewEntity].NPC->lastPathAngles );
				g_entities[ent->client->ps.viewEntity].NPC->desiredYaw = g_entities[ent->client->ps.viewEntity].currentAngles[YAW];
			}
		}
		CG_SetClientViewAngles( ent->pos4, qtrue );
		SetClientViewAngle( ent, ent->pos4 );
	}
	ent->client->ps.viewEntity = 0;
	return qtrue;
}

void G_SetViewEntity( gentity_t *self, gentity_t *viewEntity )
{
	if ( !self || !self->client || !viewEntity )
	{
		return;
	}

	if ( self->s.number == 0 && cg.zoomMode )
	{
		// yeah, it should really toggle them so it plays the end sound....
		cg.zoomMode = 0;
	}
	if ( viewEntity->s.number == self->client->ps.viewEntity )
	{
		return;
	}
	//clear old one first
	G_ClearViewEntity( self );
	//set new one
	self->client->ps.viewEntity = viewEntity->s.number;
	viewEntity->svFlags |= SVF_BROADCAST;
	//remember current angles
	VectorCopy( self->client->ps.viewangles, self->pos4 );
	if ( viewEntity->client )
	{
		//vec3_t	clear = {0,0,0};
		CG_SetClientViewAngles( viewEntity->client->ps.viewangles, qtrue );
		//SetClientViewAngle( self, viewEntity->client->ps.viewangles );
		//SetClientViewAngle( viewEntity, clear );
		/*
		VectorCopy( viewEntity->client->ps.viewangles, self->client->ps.viewangles );
		for ( int i = 0; i < 3; i++ )
		{
			self->client->ps.delta_angles[i] = viewEntity->client->ps.delta_angles[i];
		}
		*/
	}
	if ( !self->s.number )
	{
		CG_CenterPrint( "@INGAME_EXIT_VIEW", SCREEN_HEIGHT * 0.95 );
	}
}

qboolean G_ControlledByPlayer( gentity_t *self )
{
	if ( self && self->NPC && self->NPC->controlledTime > level.time )
	{//being controlled
		gentity_t *controller = &g_entities[0];
		if ( controller->client && controller->client->ps.viewEntity == self->s.number )
		{//we're the player's viewEntity
			return qtrue;
		}
	}
	return qfalse;
}

qboolean G_ValidateLookEnemy( gentity_t *self, gentity_t *enemy )
{
	if ( !enemy )
	{
		return qfalse;
	}

	if ( enemy->flags&FL_NOTARGET )
	{
		return qfalse;
	}

	if ( !enemy || enemy == self || !enemy->inuse )
	{
		return qfalse;
	}
	if ( !enemy->client || !enemy->NPC )
	{//not valid
		if ( (enemy->svFlags&SVF_NONNPC_ENEMY) 
			&& enemy->s.weapon == WP_TURRET 
			&& enemy->noDamageTeam != self->client->playerTeam
			&& enemy->health > 0 )
		{//a turret
			//return qtrue;
		}
		else
		{
			return qfalse;
		}
	}
	else 
	{
		if ( enemy->client->playerTeam == self->client->playerTeam )
		{//on same team
			return qfalse;
		}

		if ( enemy->health <= 0 && ((level.time-enemy->s.time) > 3000||!InFront(enemy->currentOrigin,self->currentOrigin,self->client->ps.viewangles,0.2f)||DistanceHorizontal(enemy->currentOrigin,self->currentOrigin)>16384))//>128
		{//corpse, been dead too long or too out of sight to be interesting
			if ( !enemy->message )
			{
				return qfalse;
			}
		}
	}

	if ( (!InFront( enemy->currentOrigin, self->currentOrigin, self->client->ps.viewangles, 0.0f) || !G_ClearLOS( self, self->client->renderInfo.eyePoint, enemy ) ) 
		&& ( DistanceHorizontalSquared( enemy->currentOrigin, self->currentOrigin ) > 65536 || fabs(enemy->currentOrigin[2]-self->currentOrigin[2]) > 384 )  )
	{//(not in front or not clear LOS) & greater than 256 away
		return qfalse;
	}

	//LOS?

	return qtrue;
}

void G_ChooseLookEnemy( gentity_t *self, usercmd_t *ucmd )
{
//FIXME: should be a more intelligent way of doing this, like auto aim?
//closest, most in front... did damage to... took damage from?  How do we know who the player is focusing on?
	gentity_t	*ent, *bestEnt = NULL;
	gentity_t	*entityList[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		center, mins, maxs, fwdangles, forward, dir;
	int			i, e;
	float		radius = 256;
	float		rating, bestRating = 0.0f;

	//FIXME: no need to do this in 1st person?
	fwdangles[0] = 0;		//Must initialize data!
	fwdangles[1] = self->client->ps.viewangles[1];
	fwdangles[2] = 0;
	AngleVectors( fwdangles, forward, NULL, NULL );

	VectorCopy( self->currentOrigin, center );

	for ( i = 0 ; i < 3 ; i++ ) 
	{
		mins[i] = center[i] - radius;
		maxs[i] = center[i] + radius;
	}
	numListedEntities = gi.EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );
	
	if ( !numListedEntities )
	{//should we clear the enemy?
		return;
	}

	for ( e = 0 ; e < numListedEntities ; e++ ) 
	{
		ent = entityList[ e ];

		if ( !gi.inPVS( self->currentOrigin, ent->currentOrigin ) )
		{//not even potentially visible
			continue;
		}

		if ( !G_ValidateLookEnemy( self, ent ) )
		{//doesn't meet criteria of valid look enemy (don't check current since we would have done that before this func's call
			continue;
		}

		if ( !G_ClearLOS( self, self->client->renderInfo.eyePoint, ent ) )
		{//can't see him
			continue;
		}
		//rate him based on how close & how in front he is
		VectorSubtract( ent->currentOrigin, center, dir );
		rating = (1.0f-(VectorNormalize( dir )/radius));
		rating *= DotProduct( forward, dir )+1.0f;
		if ( ent->health <= 0 )
		{
			if ( (ucmd->buttons&BUTTON_ATTACK) || (ucmd->buttons&BUTTON_ALT_ATTACK) )
			{//if attacking, don't consider dead enemies
				continue;
			}
			if ( ent->message )
			{//keyholder
				rating *= 0.5f;
			}
			else
			{
				rating *= 0.1f;
			}
		}
		if ( ent->s.weapon == WP_SABER )
		{
			rating *= 2.0f;
		}
		if ( ent->enemy == self )
		{//he's mad at me, he's more important
			rating *= 2.0f;
		}
		else if ( ent->NPC && ent->NPC->blockedSpeechDebounceTime > level.time - 6000 )
		{//he's detected me, he's more important
			if ( ent->NPC->blockedSpeechDebounceTime > level.time + 4000 )
			{
				rating *= 1.5f;
			}
			else
			{//from 1.0f to 1.5f
				rating += rating * ((float)(ent->NPC->blockedSpeechDebounceTime-level.time) + 6000.0f)/20000.0f;
			}
		}
		/*
		if ( g_crosshairEntNum == ent && !self->enemy )
		{//we don't have an enemy and we are aiming at this guy
			rading *= 2.0f;
		}
		*/
		if ( rating > bestRating )
		{
			bestEnt = ent;
			bestRating = rating;
		}
	}
	if ( bestEnt )
	{
		self->enemy = bestEnt;
	}
}

/*
===============
G_DamageFeedback

Called just before a snapshot is sent to the given player.
Totals up all damage and generates both the player_state_t
damage values to that client for pain blends and kicks, and
global pain sound events for all clients.
===============
*/
void P_DamageFeedback( gentity_t *player ) {
	gclient_t	*client;
	float	count;
	vec3_t	angles;

	client = player->client;
	if ( client->ps.pm_type == PM_DEAD ) {
		return;
	}

	// total points of damage shot at the player this frame
	count = client->damage_blood + client->damage_armor;
	if ( count == 0 ) {
		return;		// didn't take any damage
	}

	if ( count > 255 ) {
		count = 255;
	}

	// send the information to the client

	// world damage (falling, slime, etc) uses a special code
	// to make the blend blob centered instead of positional
	if ( client->damage_fromWorld ) 
	{
		client->ps.damagePitch = 255;
		client->ps.damageYaw = 255;

		client->damage_fromWorld = qfalse;
	} 
	else 
	{
		vectoangles( client->damage_from, angles );
		client->ps.damagePitch = angles[PITCH]/360.0 * 256;
		client->ps.damageYaw = angles[YAW]/360.0 * 256;
	}

	client->ps.damageCount = count;

	//
	// clear totals
	//
	client->damage_blood = 0;
	client->damage_armor = 0;
	client->damage_knockback = 0;
}



/*
=============
P_WorldEffects

Check for lava / slime contents and drowning
=============
*/
void P_WorldEffects( gentity_t *ent ) {
	int			mouthContents = 0;

	if ( ent->client->noclip ) 
	{
		ent->client->airOutTime = level.time + 12000;	// don't need air
		return;
	}

	if ( !in_camera )
	{
		mouthContents = gi.pointcontents( ent->client->renderInfo.eyePoint, ent->s.number );
	}
	//
	// check for drowning
	//
	if ( (mouthContents&(CONTENTS_WATER|CONTENTS_SLIME)) ) 
	{
		if ( ent->client->NPC_class == CLASS_SWAMPTROOPER )
		{//they have air tanks
			ent->client->airOutTime = level.time + 12000;	// don't need air
			ent->damage = 2;
		}
		else if ( ent->client->airOutTime < level.time) 
		{// if out of air, start drowning
			// drown!
			ent->client->airOutTime += 1000;
			if ( ent->health > 0 ) {
				// take more damage the longer underwater
				ent->damage += 2;
				if (ent->damage > 15)
					ent->damage = 15;

				// play a gurp sound instead of a normal pain sound
				if (ent->health <= ent->damage) 
				{
					G_AddEvent( ent, EV_WATER_DROWN, 0 );
				} 
				else
				{
					G_AddEvent( ent, Q_irand(EV_WATER_GURP1, EV_WATER_GURP2), 0 );
				} 

				// don't play a normal pain sound
				ent->painDebounceTime = level.time + 200;

				G_Damage (ent, NULL, NULL, NULL, NULL, 
					ent->damage, DAMAGE_NO_ARMOR, MOD_WATER);
			}
		}
	} 
	else 
	{
		ent->client->airOutTime = level.time + 12000;
		ent->damage = 2;
	}

	//
	// check for sizzle damage (move to pmove?)
	//
	if (ent->waterlevel && 
		(ent->watertype&(CONTENTS_LAVA|CONTENTS_SLIME)) ) {
		if (ent->health > 0
			&& ent->painDebounceTime < level.time	) {

			if (ent->watertype & CONTENTS_LAVA) {
				G_Damage (ent, NULL, NULL, NULL, NULL, 
					15*ent->waterlevel, 0, MOD_LAVA);
			}

			if (ent->watertype & CONTENTS_SLIME) {
				G_Damage (ent, NULL, NULL, NULL, NULL, 
					1, 0, MOD_SLIME);
			}
		}
	}

	// Poisoned?
	if ((ent->client->poisonDamage) && (ent->client->poisonTime < level.time))
	{
		ent->client->poisonDamage -= 2;
		ent->client->poisonTime = level.time + 1000;
		G_Damage( ent, NULL, NULL, 0, 0, 2, DAMAGE_NO_KNOCKBACK|DAMAGE_NO_ARMOR, MOD_UNKNOWN );//FIXME: MOD_POISON?

		if (ent->client->poisonDamage<0)
		{
			ent->client->poisonDamage = 0;
		}
	}

}



/*
===============
G_SetClientSound
===============
*/
void G_SetClientSound( gentity_t *ent ) {
//	if (ent->waterlevel && (ent->watertype&(CONTENTS_LAVA|CONTENTS_SLIME)) )
//		ent->s.loopSound = G_SoundIndex("sound/weapons/stasis/electricloop.wav");

//	else
//		ent->s.loopSound = 0;
}



//==============================================================
void DoImpact( gentity_t *self, gentity_t *other, qboolean damageSelf )
{
	float magnitude, my_mass;
	vec3_t	velocity;

	if( self->client )
	{
		VectorCopy( self->client->ps.velocity, velocity );
		my_mass = self->mass;
	}
	else 
	{
		VectorCopy( self->s.pos.trDelta, velocity );
		if ( self->s.pos.trType == TR_GRAVITY )
		{
			velocity[2] -= 0.25f * g_gravity->value;
		}
		if( !self->mass )
		{
			my_mass = 1;
		}
		else if ( self->mass <= 10 )
		{
			my_mass = 10;
		}
		else
		{
			my_mass = self->mass;///10;
		}
	}

	magnitude = VectorLength( velocity ) * my_mass / 50;

	if ( !self->client || self->client->ps.lastOnGround+300<level.time || ( self->client->ps.lastOnGround+100 < level.time ) )
	{
		vec3_t dir1, dir2;
		float force = 0, dot;

		if ( other->material == MAT_GLASS || other->material == MAT_GLASS_METAL || other->material == MAT_GRATE1 || ((other->svFlags&SVF_BBRUSH)&&(other->spawnflags&4/*THIN*/)) )//(other->absmax[0]-other->absmin[0]<=32||other->absmax[1]-other->absmin[1]<=32||other->absmax[2]-other->absmin[2]<=32)) )
		{//glass and thin breakable brushes (axially aligned only, unfortunately) take more impact damage
			magnitude *= 2;
		}

		//damage them
		if ( magnitude >= 100 && other->s.number < ENTITYNUM_WORLD )
		{
			VectorCopy( velocity, dir1 );
			VectorNormalize( dir1 );
			if( VectorCompare( other->currentOrigin, vec3_origin ) )
			{//a brush with no origin
				VectorCopy ( dir1, dir2 );
			}
			else
			{
				VectorSubtract( other->currentOrigin, self->currentOrigin, dir2 );
				VectorNormalize( dir2 );
			}

			dot = DotProduct( dir1, dir2 );

			if ( dot >= 0.2 )
			{
				force = dot;
			}
			else
			{
				force = 0;
			}

			force *= (magnitude/50);

			int cont = gi.pointcontents( other->absmax, other->s.number );
			if( (cont&CONTENTS_WATER) )//|| (self.classname=="barrel"&&self.aflag))//FIXME: or other watertypes
			{
				force /= 3;							//water absorbs 2/3 velocity
			}

			if ( self->NPC && other->s.number == ENTITYNUM_WORLD )
			{//NPCs take less damage
				force /= 2;
			}

			/*
			if(self.frozen>0&&force>10)
				force=10;
			*/

			if( ( force >= 1 && other->s.number != 0 ) || force >= 10)
			{
	/*			
				dprint("Damage other (");
				dprint(loser.classname);
				dprint("): ");
				dprint(ftos(force));
				dprint("\n");
	*/
				if ( other->svFlags & SVF_GLASS_BRUSH )
				{
					other->splashRadius = (float)(self->maxs[0] - self->mins[0])/4.0f;
				}

				if ( self->forcePushTime > level.time - 1000//was force pushed/pulled in the last 1600 milliseconds
					&& self->forcePuller == other->s.number )//hit the person who pushed/pulled me
				{//ignore the impact
				}
				else if ( other->takedamage )
				{
					if ( !self->client || !other->s.number || !other->client )
					{//aw, fuck it, clients no longer take impact damage from other clients, unless you're the player 
						G_Damage( other, self, self, velocity, self->currentOrigin, force, DAMAGE_NO_ARMOR, MOD_IMPACT );
					}
					else
					{
						GEntity_PainFunc( other, self, self, self->currentOrigin, force, MOD_IMPACT );
						//Hmm, maybe knockdown?
						G_Throw( other, dir2, force );
					}
				}
				else
				{
					//Hmm, maybe knockdown?
					G_Throw( other, dir2, force );
				}
			}
		}

		if ( damageSelf && self->takedamage && !(self->flags&FL_NO_IMPACT_DMG))
		{
			//Now damage me
			//FIXME: more lenient falling damage, especially for when driving a vehicle
			if ( self->client && self->client->ps.forceJumpZStart )
			{//we were force-jumping
				if ( self->currentOrigin[2] >= self->client->ps.forceJumpZStart )
				{//we landed at same height or higher than we landed
					magnitude = 0;
				}
				else
				{//FIXME: take off some of it, at least?
					magnitude = (self->client->ps.forceJumpZStart-self->currentOrigin[2])/3;
				}
			}
			//if(self.classname!="monster_mezzoman"&&self.netname!="spider")//Cats always land on their feet
				if( ( magnitude >= 100 + self->health && self->s.number != 0 && self->s.weapon != WP_SABER ) || ( magnitude >= 700 ) )//&& self.safe_time < level.time ))//health here is used to simulate structural integrity
				{
					if ( (self->s.weapon == WP_SABER || self->s.number == 0) && self->client && self->client->ps.groundEntityNum < ENTITYNUM_NONE && magnitude < 1000 )
					{//players and jedi take less impact damage
						//allow for some lenience on high falls
						magnitude /= 2;
						/*
						if ( self.absorb_time >= time )//crouching on impact absorbs 1/2 the damage
						{
							magnitude/=2;
						}
						*/
					}
					magnitude /= 40;
					magnitude = magnitude - force/2;//If damage other, subtract half of that damage off of own injury
					if ( magnitude >= 1 )
					{
		//FIXME: Put in a thingtype impact sound function
		/*					
						dprint("Damage self (");
						dprint(self.classname);
						dprint("): ");
						dprint(ftos(magnitude));
						dprint("\n");
		*/
						/*
						if ( self.classname=="player_sheep "&& self.flags&FL_ONGROUND && self.velocity_z > -50 )
							return;
						*/
						if ( self->NPC && self->s.weapon == WP_SABER )
						{//FIXME: for now Jedi take no falling damage, but really they should if pushed off?
							magnitude = 0;
						}
						G_Damage( self, NULL, NULL, NULL, self->currentOrigin, magnitude/2, DAMAGE_NO_ARMOR, MOD_FALLING );//FIXME: MOD_IMPACT
					}
				}
		}

		//FIXME: slow my velocity some?
	
		self->lastImpact = level.time;

		/*
		if(self.flags&FL_ONGROUND)
			self.last_onground=time;
		*/
	}
}

/*
==============
ClientImpacts
==============
*/
void ClientImpacts( gentity_t *ent, pmove_t *pm ) {
	int		i, j;
	trace_t	trace;
	gentity_t	*other;

	memset( &trace, 0, sizeof( trace ) );
	for (i=0 ; i<pm->numtouch ; i++) {
		for (j=0 ; j<i ; j++) {
			if (pm->touchents[j] == pm->touchents[i] ) {
				break;
			}
		}
		if (j != i) {
			continue;	// duplicated
		}
		other = &g_entities[ pm->touchents[i] ];

		if ( ( ent->NPC != NULL ) && ( ent->e_TouchFunc != touchF_NULL ) ) {	// last check unneccessary
			GEntity_TouchFunc( ent, other, &trace );
		}

		if ( other->e_TouchFunc == touchF_NULL ) {	// not needed, but I'll leave it I guess (cache-hit issues)
			continue;
		}
		GEntity_TouchFunc( other, ent, &trace );
	}

}

/*
============
G_TouchTriggersLerped

Find all trigger entities that ent's current position touches.
Spectators will only interact with teleporters.

This version checks at 6 unit steps between last and current origins
============
*/
void	G_TouchTriggersLerped( gentity_t *ent ) {
	int			i, num;
	float		dist, curDist = 0;
	gentity_t	*touch[MAX_GENTITIES], *hit;
	trace_t		trace;
	vec3_t		end, mins, maxs, diff;
	const vec3_t	range = { 40, 40, 52 };
	qboolean	touched[MAX_GENTITIES];
	qboolean	done = qfalse;

	if ( !ent->client ) {
		return;
	}

	// dead NPCs don't activate triggers!
	if ( ent->client->ps.stats[STAT_HEALTH] <= 0 ) {
		if ( ent->s.number )
		{
			return;
		}
	}

#ifdef _DEBUG
	for ( int j = 0; j < 3; j++ )
	{
		assert( !Q_isnan(ent->currentOrigin[j]));
		assert( !Q_isnan(ent->lastOrigin[j]));
	}
#endif// _DEBUG
	VectorSubtract( ent->currentOrigin, ent->lastOrigin, diff );
	dist = VectorNormalize( diff );

	memset (touched, qfalse, sizeof(touched) );

	for ( curDist = 0; !done && ent->maxs[1]>0; curDist += (float)ent->maxs[1]/2.0f )
	{
		if ( curDist >= dist )
		{
			VectorCopy( ent->currentOrigin, end );
			done = qtrue;
		}
		else
		{
			VectorMA( ent->lastOrigin, curDist, diff, end );
		}
		VectorSubtract( end, range, mins );
		VectorAdd( end, range, maxs );

		num = gi.EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

		// can't use ent->absmin, because that has a one unit pad
		VectorAdd( end, ent->mins, mins );
		VectorAdd( end, ent->maxs, maxs );

		for ( i=0 ; i<num ; i++ ) {
			hit = touch[i];

			if ( (hit->e_TouchFunc == touchF_NULL) && (ent->e_TouchFunc == touchF_NULL) ) {
				continue;
			}
			if ( !( hit->contents & CONTENTS_TRIGGER ) ) {
				continue;
			}

			if ( touched[i] == qtrue ) {
				continue;//already touched this move
			}
			if ( ent->client->ps.stats[STAT_HEALTH] <= 0 ) 
			{
				if ( Q_stricmp( "trigger_teleport", hit->classname ) || !(hit->spawnflags&16/*TTSF_DEAD_OK*/) )
				{//dead clients can only touch tiogger_teleports that are marked as touchable
					continue;
				}
			}
			// use seperate code for determining if an item is picked up
			// so you don't have to actually contact its bounding box
			/*
			if ( hit->s.eType == ET_ITEM ) {
				if ( !BG_PlayerTouchesItem( &ent->client->ps, &hit->s, level.time ) ) {
					continue;
				}
			} else */
			{
				if ( !gi.EntityContact( mins, maxs, hit ) ) {
					continue;
				}
			}

			touched[i] = qtrue;

			memset( &trace, 0, sizeof(trace) );

			if ( hit->e_TouchFunc != touchF_NULL ) {
				GEntity_TouchFunc(hit, ent, &trace);
			}

			//WTF?  Why would a trigger ever fire off the NPC's touch func??!!!
			/*
			if ( ( ent->NPC != NULL ) && ( ent->e_TouchFunc != touchF_NULL ) ) {
				GEntity_TouchFunc( ent, hit, &trace );
			}
			*/
		}
	}
}

/*
============
G_TouchTriggers

Find all trigger entities that ent's current position touches.
Spectators will only interact with teleporters.
============
*/
void	G_TouchTriggers( gentity_t *ent ) {
	int			i, num;
	gentity_t	*touch[MAX_GENTITIES], *hit;
	trace_t		trace;
	vec3_t		mins, maxs;
	const vec3_t	range = { 40, 40, 52 };

	if ( !ent->client ) {
		return;
	}

	// dead clients don't activate triggers!
	if ( ent->client->ps.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	VectorSubtract( ent->client->ps.origin, range, mins );
	VectorAdd( ent->client->ps.origin, range, maxs );

	num = gi.EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	// can't use ent->absmin, because that has a one unit pad
	VectorAdd( ent->client->ps.origin, ent->mins, mins );
	VectorAdd( ent->client->ps.origin, ent->maxs, maxs );

	for ( i=0 ; i<num ; i++ ) {
		hit = touch[i];

		if ( (hit->e_TouchFunc == touchF_NULL) && (ent->e_TouchFunc == touchF_NULL) ) {
			continue;
		}
		if ( !( hit->contents & CONTENTS_TRIGGER ) ) {
			continue;
		}

		// use seperate code for determining if an item is picked up
		// so you don't have to actually contact its bounding box
		/*
		if ( hit->s.eType == ET_ITEM ) {
			if ( !BG_PlayerTouchesItem( &ent->client->ps, &hit->s, level.time ) ) {
				continue;
			}
		} else */
		{
			if ( !gi.EntityContact( mins, maxs, hit ) ) {
				continue;
			}
		}

		memset( &trace, 0, sizeof(trace) );

		if ( hit->e_TouchFunc != touchF_NULL ) {
			GEntity_TouchFunc(hit, ent, &trace);
		}

		if ( ( ent->NPC != NULL ) && ( ent->e_TouchFunc != touchF_NULL ) ) {
			GEntity_TouchFunc( ent, hit, &trace );
		}
	}
}


/*
============
G_MoverTouchTriggers

Find all trigger entities that ent's current position touches.
Spectators will only interact with teleporters.
============
*/
void G_MoverTouchPushTriggers( gentity_t *ent, vec3_t oldOrg ) 
{
	int			i, num;
	float		step, stepSize, dist;
	gentity_t	*touch[MAX_GENTITIES], *hit;
	trace_t		trace;
	vec3_t		mins, maxs, dir, size, checkSpot;
	const vec3_t	range = { 40, 40, 52 };

	// non-moving movers don't hit triggers!
	if ( !VectorLengthSquared( ent->s.pos.trDelta ) ) 
	{
		return;
	}

	VectorSubtract( ent->mins, ent->maxs, size );
	stepSize = VectorLength( size );
	if ( stepSize < 1 )
	{
		stepSize = 1;
	}

	VectorSubtract( ent->currentOrigin, oldOrg, dir );
	dist = VectorNormalize( dir );
	for ( step = 0; step <= dist; step += stepSize )
	{
		VectorMA( ent->currentOrigin, step, dir, checkSpot );
		VectorSubtract( checkSpot, range, mins );
		VectorAdd( checkSpot, range, maxs );

		num = gi.EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

		// can't use ent->absmin, because that has a one unit pad
		VectorAdd( checkSpot, ent->mins, mins );
		VectorAdd( checkSpot, ent->maxs, maxs );

		for ( i=0 ; i<num ; i++ ) 
		{
			hit = touch[i];

			if ( hit->s.eType != ET_PUSH_TRIGGER )
			{
				continue;
			}

			if ( hit->e_TouchFunc == touchF_NULL ) 
			{
				continue;
			}

			if ( !( hit->contents & CONTENTS_TRIGGER ) ) 
			{
				continue;
			}


			if ( !gi.EntityContact( mins, maxs, hit ) ) 
			{
				continue;
			}

			memset( &trace, 0, sizeof(trace) );

			if ( hit->e_TouchFunc != touchF_NULL ) 
			{
				GEntity_TouchFunc(hit, ent, &trace);
			}
		}
	}
}

void G_MatchPlayerWeapon( gentity_t *ent )
{
	if ( g_entities[0].inuse && g_entities[0].client )
	{//player is around
		int newWeap;
		if ( g_entities[0].client->ps.weapon > WP_DET_PACK )
		{
			newWeap = WP_BRYAR_PISTOL;
		}
		else
		{
			newWeap = g_entities[0].client->ps.weapon;
		}
		if ( newWeap != WP_NONE && ent->client->ps.weapon != newWeap )
		{
			if ( ent->weaponModel >= 0 )
			{
				gi.G2API_RemoveGhoul2Model(ent->ghoul2, ent->weaponModel);
			}
			ent->client->ps.stats[STAT_WEAPONS] = ( 1 << newWeap );
			ent->client->ps.ammo[weaponData[newWeap].ammoIndex] = 999;
			ChangeWeapon( ent, newWeap );
			ent->client->ps.weapon = newWeap;
			ent->client->ps.weaponstate = WEAPON_READY;
			if ( newWeap == WP_SABER )
			{
				//FIXME: AddSound/Sight Event
				WP_SaberInitBladeData( ent );
				G_CreateG2AttachedWeaponModel( ent, ent->client->ps.saberModel );
				ent->client->ps.saberActive = g_entities[0].client->ps.saberActive;
				ent->client->ps.saberLength = g_entities[0].client->ps.saberLength;
				ent->client->ps.saberAnimLevel = g_entities[0].client->ps.saberAnimLevel;
			}
			else
			{
				G_CreateG2AttachedWeaponModel( ent, weaponData[newWeap].weaponMdl );
			}
		}
	}
}

void G_NPCMunroMatchPlayerWeapon( gentity_t *ent )
{
	//special uber hack for cinematic Munro's to match player's weapon
	if ( !in_camera )
	{
		if ( ent && ent->client && ent->NPC && (ent->NPC->aiFlags&NPCAI_MATCHPLAYERWEAPON) )
		{//we're a Kyle NPC
			G_MatchPlayerWeapon( ent );
		}
	}
}
/*
=================
ClientInactivityTimer

Returns qfalse if the client is dropped
=================
*/
qboolean ClientInactivityTimer( gclient_t *client ) {
	if ( ! g_inactivity->integer ) 
	{
		// give everyone some time, so if the operator sets g_inactivity during
		// gameplay, everyone isn't kicked
		client->inactivityTime = level.time + 60 * 1000;
		client->inactivityWarning = qfalse;
	} 
	else if ( client->usercmd.forwardmove || 
		client->usercmd.rightmove || 
		client->usercmd.upmove ||
		(client->usercmd.buttons & BUTTON_ATTACK) ||
		(client->usercmd.buttons & BUTTON_ALT_ATTACK) ) 
	{
		client->inactivityTime = level.time + g_inactivity->integer * 1000;
		client->inactivityWarning = qfalse;
	} 
	else if ( !client->pers.localClient ) 
	{
		if ( level.time > client->inactivityTime ) 
		{
			gi.DropClient( client - level.clients, "Dropped due to inactivity" );
			return qfalse;
		}
		if ( level.time > client->inactivityTime - 10000 && !client->inactivityWarning ) 
		{
			client->inactivityWarning = qtrue;
			gi.SendServerCommand( client - level.clients, "cp \"Ten seconds until inactivity drop!\n\"" );
		}
	}
	else
	{//FIXME: here is where we can decide to play an idle animation
	}
	return qtrue;
}

/*
==================
ClientTimerActions

Actions that happen once a second
==================
*/
void ClientTimerActions( gentity_t *ent, int msec ) {
	gclient_t *client;

	client = ent->client;
	client->timeResidual += msec;

	while ( client->timeResidual >= 1000 ) 
	{
		client->timeResidual -= 1000;

		if ( ent->s.weapon != WP_NONE )
		{
			ent->client->sess.missionStats.weaponUsed[ent->s.weapon]++;
		}
		// if we've got the seeker powerup, see if we can shoot it at someone
/*		if ( ent->client->ps.powerups[PW_SEEKER] > level.time )
		{
			vec3_t	seekerPos, dir;
			gentity_t *enemy = SeekerAcquiresTarget( ent, seekerPos );

			if ( enemy != NULL ) // set the client's enemy to a valid target
			{
				FireSeeker( ent, enemy, seekerPos, dir );

				gentity_t *tent;
				tent = G_TempEntity( seekerPos, EV_POWERUP_SEEKER_FIRE );
				VectorCopy( dir, tent->pos1 );
				tent->s.eventParm = ent->s.number;
			}
		}*/
	}
}

/*
====================
ClientIntermissionThink
====================
*/
static qboolean ClientCinematicThink( gclient_t *client ) {
	client->ps.eFlags &= ~EF_FIRING;

	// swap button actions
	client->oldbuttons = client->buttons;
	client->buttons = client->usercmd.buttons;
	if ( client->buttons & ( BUTTON_USE ) & ( client->oldbuttons ^ client->buttons ) ) {
		return( qtrue );
	}
	return( qfalse );
}


/*
================
ClientEvents

Events will be passed on to the clients for presentation,
but any server game effects are handled here
================
*/
extern void WP_SaberDamageTrace( gentity_t *ent );
extern void WP_SaberUpdateOldBladeData( gentity_t *ent );
void ClientEvents( gentity_t *ent, int oldEventSequence ) {
	int		i;
	int		event;
	gclient_t *client;
	//int		damage;
#ifndef FINAL_BUILD
	qboolean	fired = qfalse;
#endif

	client = ent->client;

	for ( i = oldEventSequence ; i < client->ps.eventSequence ; i++ ) {
		event = client->ps.events[ i & (MAX_PS_EVENTS-1) ];

		switch ( event ) {
		case EV_FALL_MEDIUM:
		case EV_FALL_FAR://these come from bg_pmove, PM_CrashLand
			if ( ent->s.eType != ET_PLAYER ) {
				break;		// not in the player model
			}
			/*
			//FIXME: isn't there a more accurate way to calculate damage from falls?
			if ( event == EV_FALL_FAR ) 
			{
				damage = 50;
			} 
			else 
			{
				damage = 25;
			}
			ent->painDebounceTime = level.time + 200;	// no normal pain sound
			G_Damage (ent, NULL, NULL, NULL, NULL, damage, 0, MOD_FALLING);
			*/
			break;

		case EV_FIRE_WEAPON:
#ifndef FINAL_BUILD
			if ( fired ) {
				gi.Printf( "DOUBLE EV_FIRE_WEAPON AND-OR EV_ALT_FIRE!!\n" );
			}
			fired = qtrue;
#endif
			FireWeapon( ent, qfalse );
			break;

		case EV_ALT_FIRE:
#ifndef FINAL_BUILD
			if ( fired ) {
				gi.Printf( "DOUBLE EV_FIRE_WEAPON AND-OR EV_ALT_FIRE!!\n" );
			}
			fired = qtrue;
#endif
			FireWeapon( ent, qtrue );
			break;

		default:
			break;
		}
	}
	//by the way, if you have your saber in hand and it's on, do the damage trace
	if ( client->ps.weapon == WP_SABER )
	{
		if ( g_timescale->value >= 1.0f || !(client->ps.forcePowersActive&(1<<FP_SPEED)) )
		{
			int wait = FRAMETIME/2;
			//sanity check
			if ( client->ps.saberDamageDebounceTime - level.time > wait )
			{//when you unpause the game with force speed on, the time gets *really* wiggy...
				client->ps.saberDamageDebounceTime = level.time + wait;
			}
			if ( client->ps.saberDamageDebounceTime <= level.time )
			{
				WP_SaberDamageTrace( ent );
				WP_SaberUpdateOldBladeData( ent );
				/*
				if ( g_timescale->value&&client->ps.clientNum==0&&!player_locked&&!MatrixMode&&client->ps.forcePowersActive&(1<<FP_SPEED) )
				{
					wait = floor( (float)wait*g_timescale->value );
				}
				*/
				client->ps.saberDamageDebounceTime = level.time + wait;
			}
		}
	}
}

qboolean G_CheckClampUcmd( gentity_t *ent, usercmd_t *ucmd )
{
	qboolean overridAngles = qfalse;

	if ( (!ent->s.number&&ent->aimDebounceTime>level.time) 
		|| (ent->client->ps.pm_time && (ent->client->ps.pm_flags&PMF_TIME_KNOCKBACK)) 
		|| ent->forcePushTime > level.time )
	{//being knocked back, can't do anything!
		ucmd->buttons = 0;
		ucmd->forwardmove = 0;
		ucmd->rightmove = 0;
		ucmd->upmove = 0;
		if ( ent->NPC )
		{
			VectorClear( ent->client->ps.moveDir );
		}
	}

	overridAngles = (PM_AdjustAnglesForKnockdown( ent, ucmd, qfalse )?qtrue:overridAngles);

	if ( ent->client->ps.saberLockTime > level.time )
	{
		ucmd->forwardmove = ucmd->rightmove = ucmd->upmove = 0;
		if ( ent->client->ps.saberLockTime - level.time > SABER_LOCK_DELAYED_TIME )
		{//2 second delay before either can push
			//FIXME: base on difficulty
			ucmd->buttons = 0;
		}
		else
		{
			ucmd->buttons &= ~(ucmd->buttons&~BUTTON_ATTACK);
		}
		overridAngles = (PM_AdjustAnglesForSaberLock( ent, ucmd )?qtrue:overridAngles);
		if ( ent->NPC )
		{
			VectorClear( ent->client->ps.moveDir );
		}
	}

	if ( ent->client->ps.saberMove == LS_A_LUNGE )
	{//can't move during lunge
		ucmd->rightmove = ucmd->upmove = 0;
		if ( ent->client->ps.legsAnimTimer > 500 && (ent->s.number || !player_locked) )
		{
			ucmd->forwardmove = 127;
		}
		else
		{
			ucmd->forwardmove = 0;
		}
		if ( ent->NPC )
		{//invalid now
			VectorClear( ent->client->ps.moveDir );
		}
	}

	if ( ent->client->ps.saberMove == LS_A_JUMP_T__B_ )
	{//can't move during leap
		if ( ent->client->ps.groundEntityNum != ENTITYNUM_NONE || (!ent->s.number && player_locked) )
		{//hit the ground
			ucmd->forwardmove = 0;
		}
		ucmd->rightmove = ucmd->upmove = 0;
		if ( ent->NPC )
		{//invalid now
			VectorClear( ent->client->ps.moveDir );
		}
	}

	if ( ent->client->ps.saberMove == LS_A_BACK || ent->client->ps.saberMove == LS_A_BACK_CR 
		|| ent->client->ps.saberMove == LS_A_BACKSTAB )
	{//can't move or turn during back attacks
		ucmd->forwardmove = ucmd->rightmove = 0;
		if ( ent->NPC )
		{
			VectorClear( ent->client->ps.moveDir );
		}
		if ( (overridAngles = (PM_AdjustAnglesForBackAttack( ent, ucmd )?qtrue:overridAngles)) == qtrue )
		{
			//pull back the view
			if ( !ent->s.number )
			{
				float animLength = PM_AnimLength( ent->client->clientInfo.animFileIndex, (animNumber_t)ent->client->ps.torsoAnim );
				float elapsedTime = (float)(animLength-ent->client->ps.legsAnimTimer);
				float backDist = 0;
				if ( elapsedTime < animLength/2.0f )
				{//starting anim
					backDist = (elapsedTime/animLength)*120.0f;
				}
				else
				{//ending anim
					backDist = ((animLength-elapsedTime)/animLength)*120.0f;
				}
				cg.overrides.active |= CG_OVERRIDE_3RD_PERSON_RNG;
				cg.overrides.thirdPersonRange = cg_thirdPersonRange.value+backDist;
			}
		}
	}
	else if ( ent->client->ps.torsoAnim == BOTH_WALL_FLIP_BACK1 
		|| ent->client->ps.torsoAnim == BOTH_WALL_FLIP_BACK2 )
	{
		//pull back the view
		if ( !ent->s.number )
		{
			float animLength = PM_AnimLength( ent->client->clientInfo.animFileIndex, (animNumber_t)ent->client->ps.torsoAnim );
			float elapsedTime = (float)(animLength-ent->client->ps.legsAnimTimer);
			float backDist = 0;
			if ( elapsedTime < animLength/2.0f )
			{//starting anim
				backDist = (elapsedTime/animLength)*120.0f;
			}
			else
			{//ending anim
				backDist = ((animLength-elapsedTime)/animLength)*120.0f;
			}
			cg.overrides.active |= CG_OVERRIDE_3RD_PERSON_RNG;
			cg.overrides.thirdPersonRange = cg_thirdPersonRange.value+backDist;
		}
	}
	else if ( !ent->s.number )
	{
		if ( ent->client->NPC_class != CLASS_ATST )
		{
			cg.overrides.active &= ~CG_OVERRIDE_3RD_PERSON_RNG;
			cg.overrides.thirdPersonRange = 0;
		}
	}


	if ( PM_InRoll( &ent->client->ps ) )
	{
		if ( ent->s.number || !player_locked )
		{
			PM_CmdForRoll( ent->client->ps.legsAnim, ucmd );
		}
		if ( ent->NPC )
		{//invalid now
			VectorClear( ent->client->ps.moveDir );
		}
		ent->client->ps.speed = 400;
	}

	if ( PM_InCartwheel( ent->client->ps.legsAnim ) )
	{//can't keep moving in cartwheel
		ucmd->forwardmove = ucmd->rightmove = ucmd->upmove = 0;
		if ( ent->NPC )
		{//invalid now
			VectorClear( ent->client->ps.moveDir );
		}
		if ( ent->s.number || !player_locked )
		{
			switch ( ent->client->ps.legsAnim )
			{
			case BOTH_ARIAL_LEFT:
			case BOTH_CARTWHEEL_LEFT:
				ucmd->rightmove = -127;
				break;
			case BOTH_ARIAL_RIGHT:
			case BOTH_CARTWHEEL_RIGHT:
				ucmd->rightmove = 127;
				break;
			case BOTH_ARIAL_F1:
				ucmd->forwardmove = 127;
				break;
			default:
				break;
			}
		}
	}

	overridAngles = (PM_AdjustAngleForWallRun( ent, ucmd, qtrue )?qtrue:overridAngles);

	return overridAngles;
}

void BG_AddPushVecToUcmd( gentity_t *self, usercmd_t *ucmd )
{
	vec3_t	forward, right, moveDir;
	float	pushSpeed, fMove, rMove;

	if ( !self->client )
	{
		return;
	}
	pushSpeed = VectorLengthSquared(self->client->pushVec);
	if(!pushSpeed)
	{//not being pushed
		return;
	}

	AngleVectors(self->client->ps.viewangles, forward, right, NULL);
	VectorScale(forward, ucmd->forwardmove/127.0f * self->client->ps.speed, moveDir);
	VectorMA(moveDir, ucmd->rightmove/127.0f * self->client->ps.speed, right, moveDir);
	//moveDir is now our intended move velocity

	VectorAdd(moveDir, self->client->pushVec, moveDir);
	self->client->ps.speed = VectorNormalize(moveDir);
	//moveDir is now our intended move velocity plus our push Vector

	fMove = 127.0 * DotProduct(forward, moveDir);
	rMove = 127.0 * DotProduct(right, moveDir);
	ucmd->forwardmove = floor(fMove);//If in the same dir , will be positive
	ucmd->rightmove = floor(rMove);//If in the same dir , will be positive

	if ( self->client->pushVecTime < level.time )
	{
		VectorClear( self->client->pushVec );
	}
}

void NPC_Accelerate( gentity_t *ent, qboolean fullWalkAcc, qboolean fullRunAcc )
{
	if ( !ent->client || !ent->NPC )
	{
		return;
	}

	if ( !ent->NPC->stats.acceleration )
	{//No acceleration means just start and stop
		ent->NPC->currentSpeed = ent->NPC->desiredSpeed;
	}
	//FIXME:  in cinematics always accel/decel?
	else if ( ent->NPC->desiredSpeed <= ent->NPC->stats.walkSpeed )
	{//Only accelerate if at walkSpeeds
		if ( ent->NPC->desiredSpeed > ent->NPC->currentSpeed + ent->NPC->stats.acceleration )
		{
			//ent->client->ps.friction = 0;
			ent->NPC->currentSpeed += ent->NPC->stats.acceleration;
		}
		else if ( ent->NPC->desiredSpeed > ent->NPC->currentSpeed )
		{
			//ent->client->ps.friction = 0;
			ent->NPC->currentSpeed = ent->NPC->desiredSpeed;
		}
		else if ( fullWalkAcc && ent->NPC->desiredSpeed < ent->NPC->currentSpeed - ent->NPC->stats.acceleration )
		{//decelerate even when walking
			ent->NPC->currentSpeed -= ent->NPC->stats.acceleration;
		}
		else if ( ent->NPC->desiredSpeed < ent->NPC->currentSpeed )
		{//stop on a dime
			ent->NPC->currentSpeed = ent->NPC->desiredSpeed;
		}
	}
	else//  if ( ent->NPC->desiredSpeed > ent->NPC->stats.walkSpeed )
	{//Only decelerate if at runSpeeds
		if ( fullRunAcc && ent->NPC->desiredSpeed > ent->NPC->currentSpeed + ent->NPC->stats.acceleration )
		{//Accelerate to runspeed
			//ent->client->ps.friction = 0;
			ent->NPC->currentSpeed += ent->NPC->stats.acceleration;
		}
		else if ( ent->NPC->desiredSpeed > ent->NPC->currentSpeed )
		{//accelerate instantly
			//ent->client->ps.friction = 0;
			ent->NPC->currentSpeed = ent->NPC->desiredSpeed;
		}
		else if ( fullRunAcc && ent->NPC->desiredSpeed < ent->NPC->currentSpeed - ent->NPC->stats.acceleration )
		{
			ent->NPC->currentSpeed -= ent->NPC->stats.acceleration;
		}
		else if ( ent->NPC->desiredSpeed < ent->NPC->currentSpeed )
		{
			ent->NPC->currentSpeed = ent->NPC->desiredSpeed;
		}
	}
}

/*
-------------------------
NPC_GetWalkSpeed
-------------------------
*/

static int NPC_GetWalkSpeed( gentity_t *ent )
{
	int	walkSpeed = 0;

	if ( ( ent->client == NULL ) || ( ent->NPC == NULL ) )
		return 0;

	switch ( ent->client->playerTeam )
	{
	case TEAM_PLAYER:	//To shutup compiler, will add entries later (this is stub code)
	default:
		walkSpeed = ent->NPC->stats.walkSpeed;
		break;
	}

	return walkSpeed;
}

/*
-------------------------
NPC_GetRunSpeed
-------------------------
*/
#define	BORG_RUN_INCR		25
#define SPECIES_RUN_INCR	25	
#define STASIS_RUN_INCR		20
#define	WARBOT_RUN_INCR		20

static int NPC_GetRunSpeed( gentity_t *ent )
{
	int	runSpeed = 0;

	if ( ( ent->client == NULL ) || ( ent->NPC == NULL ) )
		return 0;
/*
	switch ( ent->client->playerTeam )
	{
	case TEAM_BORG:
		runSpeed = ent->NPC->stats.runSpeed;
		runSpeed += BORG_RUN_INCR * (g_spskill->integer%3);
		break;

	case TEAM_8472:
		runSpeed = ent->NPC->stats.runSpeed;
		runSpeed += SPECIES_RUN_INCR * (g_spskill->integer%3);
		break;

	case TEAM_STASIS:
		runSpeed = ent->NPC->stats.runSpeed;
		runSpeed += STASIS_RUN_INCR * (g_spskill->integer%3);
		break;

	case TEAM_BOTS:
		runSpeed = ent->NPC->stats.runSpeed;
		break;

	default:
		runSpeed = ent->NPC->stats.runSpeed;
		break;
	}
*/
	// team no longer indicates species/race.  Use NPC_class to adjust speed for specific npc types
	switch( ent->client->NPC_class)
	{
	case CLASS_PROBE:	// droid cases here to shut-up compiler
	case CLASS_GONK:
	case CLASS_R2D2:
	case CLASS_R5D2:
	case CLASS_MARK1:
	case CLASS_MARK2:
	case CLASS_PROTOCOL:
	case CLASS_ATST: // hmm, not really your average droid
	case CLASS_MOUSE:
	case CLASS_SEEKER:
	case CLASS_REMOTE:
		runSpeed = ent->NPC->stats.runSpeed;
		break;

	default:
		runSpeed = ent->NPC->stats.runSpeed;
		break;
	}

	return runSpeed;
}

void G_UpdateEmplacedWeaponData( gentity_t *ent )
{
	if ( ent && ent->owner && ent->health > 0 )
	{
		gentity_t *chair = ent->owner;
		//take the emplaced gun's waypoint as your own
		ent->waypoint = chair->waypoint;

		//update the actual origin of the sitter
		mdxaBone_t	boltMatrix;
		vec3_t	chairAng = {0, ent->client->ps.viewangles[YAW], 0};

		// Getting the seat bolt here
		gi.G2API_GetBoltMatrix( chair->ghoul2, chair->playerModel, chair->headBolt,
				&boltMatrix, chairAng, chair->currentOrigin, (cg.time?cg.time:level.time),
				NULL, chair->s.modelScale );
		// Storing ent position, bolt position, and bolt axis
		gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, ent->client->ps.origin );
		gi.linkentity( ent );
	}
}

void ExitEmplacedWeapon( gentity_t *ent )
{
	// requesting to unlock from the weapon
	int oldWeapon;

	// Remove this gun from our inventory
	ent->client->ps.stats[STAT_WEAPONS] &= ~( 1 << ent->client->ps.weapon );

	// when we lock or unlock from the the gun, we just swap weapons with it
	oldWeapon = ent->client->ps.weapon;
	ent->client->ps.weapon = ent->owner->s.weapon;
	ent->owner->s.weapon = oldWeapon;

extern void ChangeWeapon( gentity_t *ent, int newWeapon );
	if ( ent->NPC )
	{
		ChangeWeapon( ent, ent->client->ps.weapon );
	}
	else
	{
extern void CG_ChangeWeapon( int num );
		CG_ChangeWeapon( ent->client->ps.weapon );
		if (weaponData[ent->client->ps.weapon].weaponMdl[0])
		{
			//might be NONE, so check if it has a model
			G_CreateG2AttachedWeaponModel( ent, weaponData[ent->client->ps.weapon].weaponMdl );

			if ( ent->client->ps.weapon == WP_SABER && cg_saberAutoThird.value )
			{
				gi.cvar_set( "cg_thirdperson", "1" );
			}
			else if ( ent->client->ps.weapon != WP_SABER && cg_gunAutoFirst.value )
			{
				gi.cvar_set( "cg_thirdperson", "0" );
			}
		}
	}

	if ( ent->client->ps.weapon == WP_SABER )
	{
		 ent->client->ps.saberActive = ent->owner->alt_fire;
	}

	// We'll leave the gun pointed in the direction it was last facing, though we'll cut out the pitch
	if ( ent->client )
	{
		VectorCopy( ent->client->ps.viewangles, ent->owner->s.angles );
		ent->owner->s.angles[PITCH] = 0;
		G_SetAngles( ent->owner, ent->owner->s.angles );
		VectorCopy( ent->owner->s.angles, ent->owner->pos1 );

		// if we are the player we will have put down a brush that blocks NPCs so that we have a clear spot to get back out.
		//gentity_t *place = G_Find( NULL, FOFS(classname), "emp_placeholder" );

		if ( ent->health > 0 && ent->owner->nextTrain )
		{//he's still alive, and we have a placeholder, so put him back
			// reset the players position
			VectorCopy( ent->owner->nextTrain->currentOrigin, ent->client->ps.origin );
			//reset ent's size to normal
			VectorCopy( ent->owner->nextTrain->mins, ent->mins );
			VectorCopy( ent->owner->nextTrain->maxs, ent->maxs );
			//free the placeholder
			G_FreeEntity( ent->owner->nextTrain );
			//re-link the ent
			gi.linkentity( ent );
		}
		else if ( ent->health <= 0 )
		{
			// dead, so give 'em a push out of the chair
			vec3_t dir;
			AngleVectors( ent->owner->s.angles, NULL, dir, NULL );

			if ( rand() & 1 )
			{
				VectorScale( dir, -1, dir );
			}

			VectorMA( ent->client->ps.velocity, 75, dir, ent->client->ps.velocity );
		}
	}

//	gi.G2API_DetachG2Model( &ent->ghoul2[ent->playerModel] );

	ent->s.eFlags &= ~EF_LOCKED_TO_WEAPON;
	ent->client->ps.eFlags &= ~EF_LOCKED_TO_WEAPON;

	ent->owner->noDamageTeam = TEAM_FREE;
	ent->owner->svFlags &= ~SVF_NONNPC_ENEMY;
	ent->owner->delay = level.time;
	ent->owner->activator = NULL;

	if ( !ent->NPC )
	{
		// by keeping the owner, a dead npc can be pushed out of the chair without colliding with it
		ent->owner = NULL;
	}
}

void RunEmplacedWeapon( gentity_t *ent, usercmd_t **ucmd )
{
	if (( (*ucmd)->buttons & BUTTON_USE || (*ucmd)->forwardmove < 0 || (*ucmd)->upmove > 0 ) && ent->owner && ent->owner->delay + 500 < level.time )
	{
		ent->owner->s.loopSound = 0;

		ExitEmplacedWeapon( ent );
		(*ucmd)->buttons &= ~BUTTON_USE;

		G_Sound( ent, G_SoundIndex( "sound/weapons/emplaced/emplaced_dismount.mp3" ));
	}
	else
	{
		// this is a crappy way to put sounds on a moving emplaced gun....
/*		if ( ent->owner )
		{
			if ( !VectorCompare( ent->owner->pos3, ent->owner->movedir ))
			{
				ent->owner->s.loopSound = G_SoundIndex( "sound/weapons/emplaced/emplaced_move_lp.wav" );
				ent->owner->fly_sound_debounce_time = level.time;
			}
			else
			{
				if ( ent->owner->fly_sound_debounce_time + 100 <= level.time )
				{
					ent->owner->s.loopSound = 0;
				}
			}

			VectorCopy( ent->owner->pos3, ent->owner->movedir );
		}
*/
		// don't allow movement, weapon switching, and most kinds of button presses
		(*ucmd)->forwardmove = 0;
		(*ucmd)->rightmove = 0;
		(*ucmd)->upmove = 0;
		(*ucmd)->buttons &= (BUTTON_ATTACK|BUTTON_ALT_ATTACK);

		(*ucmd)->weapon = ent->client->ps.weapon; //WP_EMPLACED_GUN;

		if ( ent->health <= 0 )
		{
			ExitEmplacedWeapon( ent );
		}
	}
}

// yes...   so stop skipping...
void G_StopCinematicSkip( void )
{
	gi.cvar_set("skippingCinematic", "0");
	gi.cvar_set("timescale", "1");
}

void G_StartCinematicSkip( void )
{
	
	if (cinematicSkipScript[0])
	{
		ICARUS_RunScript( &g_entities[0], va( "%s/%s", Q3_SCRIPT_DIR, cinematicSkipScript ) );
		memset( cinematicSkipScript, 0, sizeof( cinematicSkipScript ) );
		gi.cvar_set("skippingCinematic", "1");
		gi.cvar_set("timescale", "100");
	}
	else 
	{
		// no... so start skipping...
		gi.cvar_set("skippingCinematic", "1");
		gi.cvar_set("timescale", "100");
	}
}

void G_CheckClientIdle( gentity_t *ent, usercmd_t *ucmd ) 
{
	if ( !ent || !ent->client || ent->health <= 0 )
	{
		return;
	}
	if ( !ent->s.number && ( !cg.renderingThirdPerson || cg.zoomMode ) )
	{
		if ( ent->client->idleTime < level.time )
		{
			ent->client->idleTime = level.time;
		}
		return;
	}
	if ( !VectorCompare( vec3_origin, ent->client->ps.velocity ) 
		|| ucmd->buttons || ucmd->forwardmove || ucmd->rightmove || ucmd->upmove 
		|| !PM_StandingAnim( ent->client->ps.legsAnim ) 
		|| ent->enemy 
		|| ent->client->ps.legsAnimTimer
		|| ent->client->ps.torsoAnimTimer )
	{//FIXME: also check for turning?
		if ( !VectorCompare( vec3_origin, ent->client->ps.velocity ) 
			|| ucmd->buttons || ucmd->forwardmove || ucmd->rightmove || ucmd->upmove 
			|| ent->enemy )
		{
			//if in an idle, break out
			switch ( ent->client->ps.legsAnim )
			{
			case BOTH_STAND1IDLE1:
			case BOTH_STAND2IDLE1:
			case BOTH_STAND2IDLE2:
			case BOTH_STAND3IDLE1:
			case BOTH_STAND4IDLE1:
				ent->client->ps.legsAnimTimer = 0;
				break;
			}
			switch ( ent->client->ps.torsoAnim )
			{
			case BOTH_STAND1IDLE1:
			case BOTH_STAND2IDLE1:
			case BOTH_STAND2IDLE2:
			case BOTH_STAND3IDLE1:
			case BOTH_STAND4IDLE1:
				ent->client->ps.torsoAnimTimer = 0;
				break;
			}
		}
		//
		if ( ent->client->idleTime < level.time )
		{
			ent->client->idleTime = level.time;
		}
	}
	else if ( level.time - ent->client->idleTime > 5000 )
	{//been idle for 5 seconds
		int	idleAnim = -1;
		switch ( ent->client->ps.legsAnim )
		{
		case BOTH_STAND1:
			idleAnim = BOTH_STAND1IDLE1;
			break;
		case BOTH_STAND2:
			idleAnim = Q_irand(BOTH_STAND2IDLE1,BOTH_STAND2IDLE2);
			break;
		case BOTH_STAND3:
			idleAnim = BOTH_STAND3IDLE1;
			break;
		case BOTH_STAND4:
			idleAnim = BOTH_STAND4IDLE1;
			break;
		}
		if ( idleAnim != -1 && PM_HasAnimation( ent, idleAnim ) )
		{
			NPC_SetAnim( ent, SETANIM_BOTH, idleAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			//don't idle again after this anim for a while
			ent->client->idleTime = level.time + PM_AnimLength( ent->client->clientInfo.animFileIndex, (animNumber_t)idleAnim ) + Q_irand( 0, 2000 );
		}
	}
}

void G_CheckMovingLoopingSounds( gentity_t *ent, usercmd_t *ucmd )
{
	if ( ent->client )
	{
		if ( (ent->NPC&&!VectorCompare( vec3_origin, ent->client->ps.moveDir ))//moving using moveDir
			|| ucmd->forwardmove || ucmd->rightmove//moving using ucmds
			|| (ucmd->upmove&&FlyingCreature( ent ))//flier using ucmds to move
			|| (FlyingCreature( ent )&&!VectorCompare( vec3_origin, ent->client->ps.velocity )&&ent->health>0))//flier using velocity to move
		{
			switch( ent->client->NPC_class )
			{
			case CLASS_R2D2:
				ent->s.loopSound = G_SoundIndex( "sound/chars/r2d2/misc/r2_move_lp.wav" );
				break;
			case CLASS_R5D2:
				ent->s.loopSound = G_SoundIndex( "sound/chars/r2d2/misc/r2_move_lp2.wav" );
				break;
			case CLASS_MARK2:
				ent->s.loopSound = G_SoundIndex( "sound/chars/mark2/misc/mark2_move_lp" );
				break;
			case CLASS_MOUSE:
				ent->s.loopSound = G_SoundIndex( "sound/chars/mouse/misc/mouse_lp" );
				break;
			case CLASS_PROBE:
				ent->s.loopSound = G_SoundIndex( "sound/chars/probe/misc/probedroidloop" );
			default:
				break;
			}
		}
		else
		{//not moving under your own control, stop loopSound
			if ( ent->client->NPC_class == CLASS_R2D2 || ent->client->NPC_class == CLASS_R5D2 
					|| ent->client->NPC_class == CLASS_MARK2 || ent->client->NPC_class == CLASS_MOUSE 
					|| ent->client->NPC_class == CLASS_PROBE )
			{
				ent->s.loopSound = 0;
			}
		}
	}
}

/*
==============
ClientThink

This will be called once for each client frame, which will
usually be a couple times for each server frame on fast clients.

==============
*/

extern int G_FindLocalInterestPoint( gentity_t *self );
void ClientThink_real( gentity_t *ent, usercmd_t *ucmd ) 
{
	gclient_t	*client;
	pmove_t		pm;
	vec3_t		oldOrigin;
	int			oldEventSequence;
	int			msec;
	qboolean	inSpinFlipAttack = PM_AdjustAnglesForSpinningFlip( ent, ucmd, qfalse );
	qboolean	controlledByPlayer = qfalse;

	//Don't let the player do anything if in a camera
	if ( ent->s.number == 0 ) 
	{
extern cvar_t	*g_skippingcin;

		if ( ent->s.eFlags & EF_LOCKED_TO_WEAPON )
		{
			G_UpdateEmplacedWeaponData( ent );
			RunEmplacedWeapon( ent, &ucmd );
		}
		if ( ent->client->ps.saberLockTime > level.time && ent->client->ps.saberLockEnemy != ENTITYNUM_NONE )
		{
			NPC_SetLookTarget( ent, ent->client->ps.saberLockEnemy, level.time+1000 );
		}
		if ( ent->client->renderInfo.lookTargetClearTime < level.time //NOTE: here this is used as a debounce, not an actual timer
			&& ent->health > 0 //must be alive
			&& (!ent->enemy || ent->client->ps.saberMove != LS_A_BACKSTAB) )//don't update if in backstab unless don't currently have an enemy
		{//NOTE: doesn't keep updating to nearest enemy once you're dead
			int	newLookTarget;
			if ( !G_ValidateLookEnemy( ent, ent->enemy ) )
			{
				ent->enemy = NULL;
			}
			//FIXME: make this a little prescient?
			G_ChooseLookEnemy( ent, ucmd );
			if ( ent->enemy )
			{//target
				newLookTarget = ent->enemy->s.number;
				//so we don't change our minds in the next 1 second
				ent->client->renderInfo.lookTargetClearTime = level.time+1000;
				ent->client->renderInfo.lookMode = LM_ENT;
			}
			else
			{//no target
				//FIXME: what about sightalerts and missiles?
				newLookTarget = ENTITYNUM_NONE;
				newLookTarget = G_FindLocalInterestPoint( ent );
				if ( newLookTarget != ENTITYNUM_NONE )
				{//found something of interest
					ent->client->renderInfo.lookMode = LM_INTEREST;
				}
				else
				{//okay, no interesting things and no enemies, so look for items
					newLookTarget = G_FindLookItem( ent );
					ent->client->renderInfo.lookMode = LM_ENT;
				}
			}
			if ( ent->client->renderInfo.lookTarget != newLookTarget )
			{//transitioning
				NPC_SetLookTarget( ent, newLookTarget, level.time+1000 );
			}
		}
		if ( in_camera )
		{
			// watch the code here, you MUST "return" within this IF(), *unless* you're stopping the cinematic skip.
			//
			if ( ClientCinematicThink(ent->client) )
			{
				if (g_skippingcin->integer)	// already doing cinematic skip?
				{// yes...   so stop skipping...
					G_StopCinematicSkip();
				}
				else
				{// no... so start skipping...
					G_StartCinematicSkip();
					return;
				}
			}
			else
			{
				return;
			}
		}
		else if ( ent->client->ps.vehicleModel != 0 && ent->health > 0 )
		{
			float speed;
			speed = VectorLength( ent->client->ps.velocity );
			if ( speed && ent->client->ps.groundEntityNum == ENTITYNUM_NONE )
			{
				int diff = AngleNormalize180(SHORT2ANGLE(ucmd->angles[YAW]+ent->client->ps.delta_angles[YAW]) - floor(ent->client->ps.viewangles[YAW]));
				int slide = floor(((float)(diff))/120.0f*-127.0f);
				
				if ( (slide > 0 && ucmd->rightmove >= 0) || ((slide < 0 && ucmd->rightmove <= 0)) )
				{//note: don't want these to conflict right now because that seems to feel really weird
					//gi.Printf( "slide %i, diff %i, yaw %i\n", slide, diff, ucmd->angles[YAW] );
					ucmd->rightmove += slide;
				}

				if ( ucmd->rightmove > 64 )
				{
					ucmd->rightmove = 64;
				}
				else if ( ucmd->rightmove < -64 )
				{
					ucmd->rightmove = -64;
				}
			}
			else
			{
				ucmd->rightmove = 0;
				ucmd->angles[PITCH] = 0;
				ucmd->angles[YAW] = ANGLE2SHORT( ent->client->ps.viewangles[YAW] ) - ent->client->ps.delta_angles[YAW];
			}
		}
		else 
		{
			if ( g_skippingcin->integer )
			{//We're skipping the cinematic and it's over now
				gi.cvar_set("timescale", "1");
				gi.cvar_set("skippingCinematic", "0");
			}
			if ( ent->client->ps.pm_type == PM_DEAD && cg.missionStatusDeadTime < level.time )
			{//mission status screen is up because player is dead, stop all scripts
				stop_icarus = qtrue;
			}
		}

//		// Don't allow the player to adjust the pitch when they are in third person overhead cam.
//extern vmCvar_t cg_thirdPerson;
//		if ( cg_thirdPerson.integer == 2 )
//		{
//			ucmd->angles[PITCH] = 0;
//		}

		if ( cg.zoomMode == 2 )
		{
			// Any kind of movement when the player is NOT ducked when the disruptor gun is zoomed will cause us to auto-magically un-zoom
			if ( ( (ucmd->forwardmove||ucmd->rightmove) 
				   && ucmd->upmove >= 0 //crouching-moving is ok
				   && !(ucmd->buttons&BUTTON_USE)/*leaning is ok*/ 
				 ) 
				 || ucmd->upmove > 0 //jumping not allowed
			   )
			{
				// already zooming, so must be wanting to turn it off
				G_Sound( ent, G_SoundIndex( "sound/weapons/disruptor/zoomend.wav" ));
				cg.zoomMode = 0;
				cg.zoomTime = cg.time;
				cg.zoomLocked = qfalse;
			}
		}

		if ( (player_locked || (ent->client->ps.eFlags&EF_FORCE_GRIPPED)) && ent->client->ps.pm_type < PM_DEAD ) // unless dead
		{//lock out player control
			if ( !player_locked )
			{
				VectorClear( ucmd->angles );
			}
			ucmd->forwardmove = 0;
			ucmd->rightmove = 0;
			ucmd->buttons = 0;
			ucmd->upmove = 0;
			PM_AdjustAnglesToGripper( ent, ucmd );
		}
		if ( ent->client->ps.leanofs )
		{//no shooting while leaning
			ucmd->buttons &= ~BUTTON_ATTACK;
			if ( ent->client->ps.weapon != WP_DISRUPTOR )
			{//can still zoom around corners
				ucmd->buttons &= ~BUTTON_ALT_ATTACK;
			}
		}
	}
	else
	{
		if ( ent->s.eFlags & EF_LOCKED_TO_WEAPON )
		{
			G_UpdateEmplacedWeaponData( ent );
		}
		if ( player && player->client && player->client->ps.viewEntity == ent->s.number )
		{
			controlledByPlayer = qtrue;
			int sav_weapon = ucmd->weapon;
			memcpy( ucmd, &player->client->usercmd, sizeof( usercmd_t ) );
			ucmd->weapon = sav_weapon;
			ent->client->usercmd = *ucmd;
		}
		G_NPCMunroMatchPlayerWeapon( ent );
	}
	if ( ent->client )
	{
		if ( ent->client->NPC_class == CLASS_GONK || 
			ent->client->NPC_class == CLASS_MOUSE || 
			ent->client->NPC_class == CLASS_R2D2 || 
			ent->client->NPC_class == CLASS_R5D2 )
		{//no jumping or strafing in these guys
			ucmd->upmove = ucmd->rightmove = 0;
		}
		else if ( ent->client->NPC_class == CLASS_ATST )
		{//no jumping in atst
			if (ent->client->ps.pm_type != PM_NOCLIP)
			{
				ucmd->upmove = 0;
			}
			if ( ent->client->ps.groundEntityNum != ENTITYNUM_NONE )
			{//ATST crushed anything underneath it
				gentity_t	*under = &g_entities[ent->client->ps.groundEntityNum];
				if ( under && under->health && under->takedamage )
				{
					vec3_t	down = {0,0,-1};
					//FIXME: we'll be doing traces down from each foot, so we'll have a real impact origin
					G_Damage( under, ent, ent, down, under->currentOrigin, 100, 0, MOD_CRUSH );
				}
				//so they know to run like hell when I get close
				//FIXME: project this in the direction I'm moving?
				if ( !Q_irand( 0, 10 ) )
				{//not so often...
					AddSoundEvent( ent, ent->currentOrigin, ent->maxs[1]*5, AEL_DANGER );
					AddSightEvent( ent, ent->currentOrigin, ent->maxs[1]*5, AEL_DANGER, 100 );
				}
			}
		}
		else if ( ent->client->ps.groundEntityNum < ENTITYNUM_WORLD && !ent->client->ps.forceJumpCharge )
		{//standing on an entity and not currently force jumping
			gentity_t *groundEnt = &g_entities[ent->client->ps.groundEntityNum];
			if ( groundEnt && 
				groundEnt->client && 
				groundEnt->client->ps.groundEntityNum != ENTITYNUM_NONE && 
				groundEnt->health > 0 &&
				!PM_InRoll( &groundEnt->client->ps ) 
				&& !(groundEnt->client->ps.eFlags&EF_LOCKED_TO_WEAPON) 
				&& !inSpinFlipAttack )
			{//landed on a live client who is on the ground, jump off them and knock them down
				if ( ent->health > 0 )
				{
					if ( !PM_InRoll( &ent->client->ps ) 
						&& !PM_FlippingAnim( ent->client->ps.legsAnim ) )
					{
						if ( ent->s.number && ent->s.weapon == WP_SABER )
						{
							ent->client->ps.forceJumpCharge = 320;//FIXME: calc this intelligently?
						}
						else if ( !ucmd->upmove )
						{//if not ducking (which should cause a roll), then jump
							ucmd->upmove = 127;
						}
						if ( !ucmd->forwardmove && !ucmd->rightmove )
						{//  If not moving, don't want to jump straight up
							//FIXME: trace for clear di?
							if ( !Q_irand( 0, 3 ) )
							{
								ucmd->forwardmove = 127;
							}
							else if ( !Q_irand( 0, 3 ) )
							{
								ucmd->forwardmove = -127;
							}
							else if ( !Q_irand( 0, 1 ) )
							{
								ucmd->rightmove = 127;
							}
							else
							{
								ucmd->rightmove = -127;
							}
						}
						if ( !ent->s.number && ucmd->upmove < 0 )
						{//player who should roll- force it
							int	rollAnim = BOTH_ROLL_F;
							if ( ucmd->forwardmove >= 0 )
							{
								rollAnim = BOTH_ROLL_F;
							}
							else if ( ucmd->forwardmove < 0 )
							{
								rollAnim = BOTH_ROLL_B;
							}
							else if ( ucmd->rightmove > 0 )
							{
								rollAnim = BOTH_ROLL_R;
							}
							else if ( ucmd->rightmove < 0 )
							{
								rollAnim = BOTH_ROLL_L;
							}
							NPC_SetAnim(ent,SETANIM_BOTH,rollAnim,SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
							G_AddEvent( ent, EV_ROLL, 0 );
							ent->client->ps.saberMove = LS_NONE;
						}
					}
				}
				else
				{//a corpse?  Shit
					//Hmm, corpses should probably *always* knockdown...
					ent->clipmask &= ~CONTENTS_BODY;
				}
				//FIXME: need impact sound event
				GEntity_PainFunc( groundEnt, ent, ent, groundEnt->currentOrigin, 0, MOD_CRUSH );
				if ( groundEnt->client->NPC_class == CLASS_DESANN && ent->client->NPC_class != CLASS_LUKE )
				{//can't knock down desann unless you're luke
					//FIXME: should he smack you away like Galak Mech?
				}
				else if ( 
					( ( (groundEnt->s.number&&(groundEnt->s.weapon!=WP_SABER||!groundEnt->NPC||groundEnt->NPC->rank<Q_irand(RANK_CIVILIAN,RANK_CAPTAIN+1)))  //an NPC who is either not a saber user or passed the rank-based probability test
							|| ((!ent->s.number||G_ControlledByPlayer(groundEnt)) && !Q_irand( 0, 3 )&&cg.renderingThirdPerson&&!cg.zoomMode) )//or a player in third person, 25% of the time
						&& groundEnt->client->playerTeam != ent->client->playerTeam ) //and not on the same team
					|| ent->client->NPC_class == CLASS_DESANN )//desann always knocks people down
				{
					int knockAnim = BOTH_KNOCKDOWN1;
					if ( PM_CrouchAnim( groundEnt->client->ps.legsAnim ) )
					{//knockdown from crouch
						knockAnim = BOTH_KNOCKDOWN4;
					}
					else
					{
						vec3_t gEFwd, gEAngles = {0,groundEnt->client->ps.viewangles[YAW],0};
						AngleVectors( gEAngles, gEFwd, NULL, NULL );
						if ( DotProduct( ent->client->ps.velocity, gEFwd ) > 50 )
						{//pushing him forward
							knockAnim = BOTH_KNOCKDOWN3;
						}
					}
					NPC_SetAnim( groundEnt, SETANIM_BOTH, knockAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				}
			}
		}
	}


	client = ent->client;

	// mark the time, so the connection sprite can be removed
	client->lastCmdTime = level.time;
	client->pers.lastCommand = *ucmd;

	// sanity check the command time to prevent speedup cheating
	if ( ucmd->serverTime > level.time + 200 ) 
	{
		ucmd->serverTime = level.time + 200;
	}
	if ( ucmd->serverTime < level.time - 1000 ) 
	{
		ucmd->serverTime = level.time - 1000;
	} 

	msec = ucmd->serverTime - client->ps.commandTime;
	if ( msec < 1 ) 
	{
		msec = 1;
	}
	if ( msec > 200 ) 
	{
		msec = 200;
	}

	// check for inactivity timer, but never drop the local client of a non-dedicated server
	if ( !ClientInactivityTimer( client ) ) 
		return;

	if ( client->noclip ) 
	{
		client->ps.pm_type = PM_NOCLIP;
	} 
	else if ( client->ps.stats[STAT_HEALTH] <= 0 ) 
	{
		client->ps.pm_type = PM_DEAD;
	} 
	else 
	{
		client->ps.pm_type = PM_NORMAL;
	}

	//FIXME: if global gravity changes this should update everyone's personal gravity...
	if ( !(ent->svFlags & SVF_CUSTOM_GRAVITY) )
	{
		client->ps.gravity = g_gravity->value;
	}

	// set speed
	if ( ent->NPC != NULL )
	{//we don't actually scale the ucmd, we use actual speeds
		if ( ent->NPC->combatMove == qfalse )
		{
			if ( !(ucmd->buttons & BUTTON_USE) )
			{//Not leaning
				qboolean Flying = (ucmd->upmove && ent->NPC->stats.moveType == MT_FLYSWIM);
				qboolean Climbing = (ucmd->upmove && ent->watertype&CONTENTS_LADDER );

				client->ps.friction = 6;

				if ( ucmd->forwardmove || ucmd->rightmove || Flying )
				{
					//if ( ent->NPC->behaviorState != BS_FORMATION )
					{//In - Formation NPCs set thier desiredSpeed themselves
						if ( ucmd->buttons & BUTTON_WALKING )
						{
							ent->NPC->desiredSpeed = NPC_GetWalkSpeed( ent );//ent->NPC->stats.walkSpeed;
						}
						else//running
						{
							ent->NPC->desiredSpeed = NPC_GetRunSpeed( ent );//ent->NPC->stats.runSpeed;
						}

						if ( ent->NPC->currentSpeed >= 80 && !controlledByPlayer )
						{//At higher speeds, need to slow down close to stuff
							//Slow down as you approach your goal
						//	if ( ent->NPC->distToGoal < SLOWDOWN_DIST && client->race != RACE_BORG && !(ent->NPC->aiFlags&NPCAI_NO_SLOWDOWN) )//128
							if ( ent->NPC->distToGoal < SLOWDOWN_DIST && !(ent->NPC->aiFlags&NPCAI_NO_SLOWDOWN) )//128
							{
								if ( ent->NPC->desiredSpeed > MIN_NPC_SPEED )
								{
									float slowdownSpeed = ((float)ent->NPC->desiredSpeed) * ent->NPC->distToGoal / SLOWDOWN_DIST;

									ent->NPC->desiredSpeed = ceil(slowdownSpeed);
									if ( ent->NPC->desiredSpeed < MIN_NPC_SPEED )
									{//don't slow down too much
										ent->NPC->desiredSpeed = MIN_NPC_SPEED;
									}
								}
							}
						}
					}
				}
				else if ( Climbing )
				{
					ent->NPC->desiredSpeed = ent->NPC->stats.walkSpeed;
				}
				else
				{//We want to stop
					ent->NPC->desiredSpeed = 0;
				}

				NPC_Accelerate( ent, qfalse, qfalse );

				if ( ent->NPC->currentSpeed <= 24 && ent->NPC->desiredSpeed < ent->NPC->currentSpeed )
				{//No-one walks this slow
					client->ps.speed = ent->NPC->currentSpeed = 0;//Full stop
					ucmd->forwardmove = 0;
					ucmd->rightmove = 0;
				}
				else
				{
					if ( ent->NPC->currentSpeed <= ent->NPC->stats.walkSpeed )
					{//Play the walkanim
						ucmd->buttons |= BUTTON_WALKING;
					}
					else
					{
						ucmd->buttons &= ~BUTTON_WALKING;
					}

					if ( ent->NPC->currentSpeed > 0 )
					{//We should be moving
						if ( Climbing || Flying )
						{
							if ( !ucmd->upmove )
							{//We need to force them to take a couple more steps until stopped
								ucmd->upmove = ent->NPC->last_ucmd.upmove;//was last_upmove;
							}
						}
						else if ( !ucmd->forwardmove && !ucmd->rightmove )
						{//We need to force them to take a couple more steps until stopped
							ucmd->forwardmove = ent->NPC->last_ucmd.forwardmove;//was last_forwardmove;
							ucmd->rightmove = ent->NPC->last_ucmd.rightmove;//was last_rightmove;
						}
					}

					client->ps.speed = ent->NPC->currentSpeed;
					if ( player && player->client && player->client->ps.viewEntity == ent->s.number )
					{
					}
					else
					{
						//Slow down on turns - don't orbit!!!
						float turndelta = 0; 
						// if the NPC is locked into a Yaw, we want to check the lockedDesiredYaw...otherwise the NPC can't walk backwards, because it always thinks it trying to turn according to desiredYaw
						if( client->renderInfo.renderFlags & RF_LOCKEDANGLE ) // yeah I know the RF_ flag is a pretty ugly hack...
						{	
							turndelta = (180 - fabs( AngleDelta( ent->currentAngles[YAW], ent->NPC->lockedDesiredYaw ) ))/180;
						}
						else
						{
							turndelta = (180 - fabs( AngleDelta( ent->currentAngles[YAW], ent->NPC->desiredYaw ) ))/180;
						}
												
						if ( turndelta < 0.75f )
						{
							client->ps.speed = 0;
						}
						else if ( ent->NPC->distToGoal < 100 && turndelta < 1.0 )
						{//Turn is greater than 45 degrees or closer than 100 to goal
							client->ps.speed = floor(((float)(client->ps.speed))*turndelta);
						}
					}
				}
			}
		}
		else
		{	
			ent->NPC->desiredSpeed = ( ucmd->buttons & BUTTON_WALKING ) ? NPC_GetWalkSpeed( ent ) : NPC_GetRunSpeed( ent );

			client->ps.speed = ent->NPC->desiredSpeed;
		}
	}
	else
	{//Client sets ucmds and such for speed alterations
		if ( ent->client->ps.vehicleModel != 0 && ent->health > 0 )
		{
			if ( client->ps.speed || client->ps.groundEntityNum == ENTITYNUM_NONE || ucmd->forwardmove > 0 || ucmd->upmove > 0)
			{
				if ( ucmd->forwardmove > 0 )
				{
					client->ps.speed += 20;
				}
				else if ( ucmd->forwardmove < 0 )
				{
					if ( client->ps.speed > 800 )
					{
						client->ps.speed -= 20;
					}
					else
					{
						client->ps.speed -= 5;
					}
				}
				else
				{//accelerate to cruising speed only, otherwise, just coast
					if ( client->ps.speed < 800 )
					{
						client->ps.speed += 10;
						if ( client->ps.speed > 800 )
						{
							client->ps.speed = 800;
						}
					}
				}
				ucmd->forwardmove = 127;
			}
			else
			{
				if ( ucmd->forwardmove < 0 )
				{
					ucmd->forwardmove = 0;
				}
				if ( ucmd->upmove < 0 )
				{
					ucmd->upmove = 0;
				}
				ucmd->rightmove = 0;
			}

			if ( client->ps.speed > 3000 )
			{
				client->ps.speed = 3000;
			}
			else if ( client->ps.speed < 0 )
			{
				client->ps.speed = 0;
			}

			if ( client->ps.speed < 800 )
			{
				client->ps.gravity = (800 - client->ps.speed)/4;
			}
			else
			{
				client->ps.gravity = 0;
			}
		}
		else
		{
			client->ps.speed = g_speed->value;//default is 320
			/*if ( !ent->s.number && ent->painDebounceTime>level.time )
			{
				client->ps.speed *= 0.25f;
			}
			else */if ( PM_SaberInAttack( ent->client->ps.saberMove ) && ucmd->forwardmove < 0 )
			{//if running backwards while attacking, don't run as fast.
				switch( client->ps.saberAnimLevel )
				{
				case FORCE_LEVEL_1:
					client->ps.speed *= 0.75f;
					break;
				case FORCE_LEVEL_2:
					client->ps.speed *= 0.60f;
					break;
				case FORCE_LEVEL_3:
					client->ps.speed *= 0.45f;
					break;
				}
				if ( g_saberMoveSpeed->value != 1.0f )
				{
					client->ps.speed *= g_saberMoveSpeed->value;
				}
			}
			else if ( PM_SpinningSaberAnim( client->ps.legsAnim ) )
			{
				client->ps.speed *= 0.5f;
				if ( g_saberMoveSpeed->value != 1.0f )
				{
					client->ps.speed *= g_saberMoveSpeed->value;
				}
			}
			else if ( client->ps.weapon == WP_SABER && ( ucmd->buttons & BUTTON_ATTACK ) )
			{//if attacking with saber while running, drop your speed
				//FIXME: should be weaponTime?  Or in certain anims?
				switch( client->ps.saberAnimLevel )
				{
				case FORCE_LEVEL_2:
					client->ps.speed *= 0.85f;
					break;
				case FORCE_LEVEL_3:
					client->ps.speed *= 0.70f;
					break;
				}
				if ( g_saberMoveSpeed->value != 1.0f )
				{
					client->ps.speed *= g_saberMoveSpeed->value;
				}
			}
		}
	}
	if ( client->NPC_class == CLASS_ATST && client->ps.legsAnim == BOTH_RUN1START )
	{//HACK: when starting to move as atst, ramp up speed
		//float animLength = PM_AnimLength( client->clientInfo.animFileIndex, (animNumber_t)client->ps.legsAnim);
		//client->ps.speed *= ( animLength - client->ps.legsAnimTimer)/animLength;
		if ( client->ps.legsAnimTimer > 100 )
		{
			client->ps.speed = 0;
		}
	}

	//Apply forced movement
	if ( client->forced_forwardmove )
	{
		ucmd->forwardmove = client->forced_forwardmove;
		if ( !client->ps.speed )
		{
			if ( ent->NPC != NULL )
			{
				client->ps.speed = ent->NPC->stats.runSpeed;
			}
			else
			{
				client->ps.speed = g_speed->value;//default is 320
			}
		}
	}

	if ( client->forced_rightmove )
	{
		ucmd->rightmove = client->forced_rightmove;
		if ( !client->ps.speed )
		{
			if ( ent->NPC != NULL  )
			{
				client->ps.speed = ent->NPC->stats.runSpeed;
			}
			else
			{
				client->ps.speed = g_speed->value;//default is 320
			}
		}
	}

	if ( ucmd->forwardmove < 0 && !(ucmd->buttons&BUTTON_WALKING) && client->ps.groundEntityNum != ENTITYNUM_NONE )
	{//running backwards is slower than running forwards
		client->ps.speed *= 0.75;
	}

	//FIXME: need to do this before check to avoid walls and cliffs (or just cliffs?)
	BG_AddPushVecToUcmd( ent, ucmd );

	G_CheckClampUcmd( ent, ucmd );

	if ( (ucmd->buttons&BUTTON_BLOCKING) && !g_saberAutoBlocking->integer )
	{//blocking with saber
		ent->client->ps.saberBlockingTime = level.time + FRAMETIME;
	}
	WP_ForcePowersUpdate( ent, ucmd );
	//if we have the saber in hand, check for starting a block to reflect shots
	WP_SaberStartMissileBlockCheck( ent, ucmd  );
	// Update the position of the saber, and check to see if we're throwing it

	if ( client->ps.saberEntityNum != ENTITYNUM_NONE )
	{
		int updates = 1;
		if ( ent->NPC )
		{
			updates = 3;//simulate player update rate?
		}
		for ( int update = 0; update < updates; update++ )
		{
			WP_SaberUpdate( ent, ucmd );
		}
	}

	//NEED to do this every frame, since these overrides do not go into the save/load data
	if ( ent->client->ps.vehicleModel != 0 )
	{
		cg.overrides.active |= (CG_OVERRIDE_3RD_PERSON_RNG|CG_OVERRIDE_FOV);
		cg.overrides.thirdPersonRange = 240;
		cg.overrides.fov = 100;
	}
	else if ( client->ps.eFlags&EF_IN_ATST )
	{
		cg.overrides.active |= (CG_OVERRIDE_3RD_PERSON_RNG|CG_OVERRIDE_3RD_PERSON_POF|CG_OVERRIDE_3RD_PERSON_VOF);
		cg.overrides.thirdPersonRange = 240;
		if ( cg_thirdPersonAutoAlpha.integer )
		{
			if ( ent->health > 0 && ent->client->ps.viewangles[PITCH] < 15 && ent->client->ps.viewangles[PITCH] > 0 )
			{
				cg.overrides.active |= CG_OVERRIDE_3RD_PERSON_APH;
				if ( cg.overrides.thirdPersonAlpha > 0.525f )
				{
					cg.overrides.thirdPersonAlpha -= 0.025f;
				}
				else if ( cg.overrides.thirdPersonAlpha > 0.5f )
				{
					cg.overrides.thirdPersonAlpha = 0.5f;
				}
			}
			else if ( cg.overrides.active&CG_OVERRIDE_3RD_PERSON_APH )
			{
				if ( cg.overrides.thirdPersonAlpha > cg_thirdPersonAlpha.value )
				{
					cg.overrides.active &= ~CG_OVERRIDE_3RD_PERSON_APH;
				}
				else if ( cg.overrides.thirdPersonAlpha < cg_thirdPersonAlpha.value-0.1f )
				{
					cg.overrides.thirdPersonAlpha += 0.1f;
				}
				else if ( cg.overrides.thirdPersonAlpha < cg_thirdPersonAlpha.value )
				{
					cg.overrides.thirdPersonAlpha = cg_thirdPersonAlpha.value;
					cg.overrides.active &= ~CG_OVERRIDE_3RD_PERSON_APH;
				}
			}
		}
		if ( ent->client->ps.viewangles[PITCH] > 0 )
		{
			cg.overrides.thirdPersonPitchOffset = ent->client->ps.viewangles[PITCH]*-0.75;
			cg.overrides.thirdPersonVertOffset = 300+ent->client->ps.viewangles[PITCH]*-10;
			if ( cg.overrides.thirdPersonVertOffset < 0 )
			{
				cg.overrides.thirdPersonVertOffset = 0;
			}
		}
		else if ( ent->client->ps.viewangles[PITCH] < 0 )
		{
			cg.overrides.thirdPersonPitchOffset = ent->client->ps.viewangles[PITCH]*-0.75;
			cg.overrides.thirdPersonVertOffset = 300+ent->client->ps.viewangles[PITCH]*-5;
			if ( cg.overrides.thirdPersonVertOffset > 300 )
			{
				cg.overrides.thirdPersonVertOffset = 300;
			}
		}
		else
		{
			cg.overrides.thirdPersonPitchOffset = 0;
			cg.overrides.thirdPersonVertOffset = 200;
		}
	}

	//play/stop any looping sounds tied to controlled movement
	G_CheckMovingLoopingSounds( ent, ucmd );

	//remember your last angles
	VectorCopy ( ent->client->ps.viewangles, ent->lastAngles );
	// set up for pmove
	oldEventSequence = client->ps.eventSequence;

	memset( &pm, 0, sizeof(pm) );

	pm.gent = ent;
	pm.ps = &client->ps;
	pm.cmd = *ucmd;
//	pm.tracemask = MASK_PLAYERSOLID;	// used differently for navgen
	pm.tracemask = ent->clipmask;
	pm.trace = gi.trace;
	pm.pointcontents = gi.pointcontents;
	pm.debugLevel = g_debugMove->integer;
	pm.noFootsteps = 0;//( g_dmflags->integer & DF_NO_FOOTSTEPS ) > 0;

	VectorCopy( client->ps.origin, oldOrigin );

	// perform a pmove
	Pmove( &pm );

	// save results of pmove
	if ( ent->client->ps.eventSequence != oldEventSequence ) 
	{
		ent->eventTime = level.time;
		{
			int		seq;

			seq = (ent->client->ps.eventSequence-1) & (MAX_PS_EVENTS-1);
			ent->s.event = ent->client->ps.events[ seq ] | ( ( ent->client->ps.eventSequence & 3 ) << 8 );
			ent->s.eventParm = ent->client->ps.eventParms[ seq ];
		}
	}
	PlayerStateToEntityState( &ent->client->ps, &ent->s );

	VectorCopy ( ent->currentOrigin, ent->lastOrigin );
#if 1
	// use the precise origin for linking
	VectorCopy( ent->client->ps.origin, ent->currentOrigin );
#else
	//We don't use prediction anymore, so screw this
	// use the snapped origin for linking so it matches client predicted versions
	VectorCopy( ent->s.pos.trBase, ent->currentOrigin );
#endif

	//Had to leave this in, some legacy code must still be using s.angles
	//Shouldn't interfere with interpolation of angles, should it?
	VectorCopy( ent->client->ps.viewangles, ent->currentAngles );

	VectorCopy( pm.mins, ent->mins );
	VectorCopy( pm.maxs, ent->maxs );

	ent->waterlevel = pm.waterlevel;
	ent->watertype = pm.watertype;

	_VectorCopy( ucmd->angles, client->pers.cmd_angles );

	// execute client events
	ClientEvents( ent, oldEventSequence );

	if ( pm.useEvent )
	{
		//TODO: Use
		TryUse( ent );
	}

	// link entity now, after any personal teleporters have been used
	gi.linkentity( ent );
	ent->client->hiddenDist = 0;
	if ( !ent->client->noclip ) 
	{
		G_TouchTriggersLerped( ent );
	}

	// touch other objects
	ClientImpacts( ent, &pm );

	// swap and latch button actions
	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;
	client->latched_buttons |= client->buttons & ~client->oldbuttons;

	// check for respawning
	if ( client->ps.stats[STAT_HEALTH] <= 0 ) 
	{
		// wait for the attack button to be pressed
		if ( ent->NPC == NULL && level.time > client->respawnTime ) 
		{
			// don't allow respawn if they are still flying through the
			// air, unless 10 extra seconds have passed, meaning something
			// strange is going on, like the corpse is caught in a wind tunnel
			/*
			if ( level.time < client->respawnTime + 10000 ) 
			{
				if ( client->ps.groundEntityNum == ENTITYNUM_NONE ) 
				{
					return;
				}
			}
			*/

			// pressing attack or use is the normal respawn method
			if ( ucmd->buttons & ( BUTTON_ATTACK ) ) 
			{
				respawn( ent );
//				gi.SendConsoleCommand( va("disconnect;wait;wait;wait;wait;wait;wait;devmap %s\n",level.mapname) );
			}
		}
		if ( ent 
			&& !ent->s.number 
			&& ent->enemy 
			&& ent->enemy != ent 
			&& ent->enemy->s.number < ENTITYNUM_WORLD 
			&& ent->enemy->inuse 
			&& !(cg.overrides.active&CG_OVERRIDE_3RD_PERSON_ANG) )
		{//keep facing enemy
			vec3_t deadDir;
			float deadYaw;
			VectorSubtract( ent->enemy->currentOrigin, ent->currentOrigin, deadDir );
			deadYaw = AngleNormalize180( vectoyaw ( deadDir ) );
			if ( deadYaw > ent->client->ps.stats[STAT_DEAD_YAW] + 1 )
			{
				ent->client->ps.stats[STAT_DEAD_YAW]++;
			}
			else if ( deadYaw < ent->client->ps.stats[STAT_DEAD_YAW] - 1 )
			{
				ent->client->ps.stats[STAT_DEAD_YAW]--;
			}
			else
			{
				ent->client->ps.stats[STAT_DEAD_YAW] = deadYaw;
			}
		}
		return;
	}

	// perform once-a-second actions
	ClientTimerActions( ent, msec );

	ClientEndPowerUps( ent );
	//DEBUG INFO
/*
	if ( client->ps.clientNum < 1 )
	{//Only a player
		if ( ucmd->buttons & BUTTON_USE )
		{
			NAV_PrintLocalWpDebugInfo( ent );
		}
	}
*/
	//try some idle anims on ent if getting no input and not moving for some time
	G_CheckClientIdle( ent, ucmd );
}

/*
==================
ClientThink

A new command has arrived from the client
==================
*/
extern void PM_CheckForceUseButton( gentity_t *ent, usercmd_t *ucmd  );
extern qboolean PM_GentCantJump( gentity_t *gent );
void ClientThink( int clientNum, usercmd_t *ucmd ) {
	gentity_t *ent;
	qboolean restore_ucmd = qfalse;
	usercmd_t sav_ucmd = {0};

	ent = g_entities + clientNum;

	if ( !ent->s.number )
	{
		if ( ent->client->ps.viewEntity > 0 && ent->client->ps.viewEntity < ENTITYNUM_WORLD )
		{//you're controlling another NPC
			gentity_t *controlled = &g_entities[ent->client->ps.viewEntity];
			qboolean freed = qfalse;
			if ( controlled->NPC 
				&& controlled->NPC->controlledTime
				&& ent->client->ps.forcePowerLevel[FP_TELEPATHY] > FORCE_LEVEL_3 )
			{//An NPC I'm controlling with mind trick
				if ( controlled->NPC->controlledTime < level.time )
				{//time's up!
					G_ClearViewEntity( ent );
					freed = qtrue;
				}
				else if ( ucmd->upmove > 0 )
				{//jumping gets you out of it FIXME: check some other button instead... like ESCAPE... so you could even have total control over an NPC?
					G_ClearViewEntity( ent );
					ucmd->upmove = 0;//ucmd->buttons = 0;
					//stop player from doing anything for a half second after
					ent->aimDebounceTime = level.time + 500;
					freed = qtrue;
				}
			}
			else if ( controlled->NPC //an NPC
				&& PM_GentCantJump( controlled ) //that cannot jump
				&& controlled->NPC->stats.moveType != MT_FLYSWIM ) //and does not use upmove to fly
			{//these types use jump to get out
				if ( ucmd->upmove > 0 )
				{//jumping gets you out of it FIXME: check some other button instead... like ESCAPE... so you could even have total control over an NPC?
					G_ClearViewEntity( ent );
					ucmd->upmove = 0;//ucmd->buttons = 0;
					//stop player from doing anything for a half second after
					ent->aimDebounceTime = level.time + 500;
					freed = qtrue;
				}
			}
			else
			{//others use the blocking key, button3
				if ( (ucmd->buttons&BUTTON_BLOCKING) )
				{//jumping gets you out of it FIXME: check some other button instead... like ESCAPE... so you could even have total control over an NPC?
					G_ClearViewEntity( ent );
					ucmd->buttons = 0;
					freed = qtrue;
				}
			}
			if ( !freed )
			{//still controlling, save off my ucmd and clear it for my actual run through pmove
				restore_ucmd = qtrue;
				memcpy( &sav_ucmd, ucmd, sizeof( usercmd_t ) );
				memset( ucmd, 0, sizeof( usercmd_t ) );
				//to keep pointing in same dir, need to set ucmd->angles
				ucmd->angles[PITCH] = ANGLE2SHORT( ent->client->ps.viewangles[PITCH] ) - ent->client->ps.delta_angles[PITCH];
				ucmd->angles[YAW] = ANGLE2SHORT( ent->client->ps.viewangles[YAW] ) - ent->client->ps.delta_angles[YAW];
				ucmd->angles[ROLL] = 0;
			}
			else
			{
				ucmd->angles[PITCH] = ANGLE2SHORT( ent->client->ps.viewangles[PITCH] ) - ent->client->ps.delta_angles[PITCH];
				ucmd->angles[YAW] = ANGLE2SHORT( ent->client->ps.viewangles[YAW] ) - ent->client->ps.delta_angles[YAW];
				ucmd->angles[ROLL] = 0;
			}
		}
		else if ( ent->client->NPC_class == CLASS_ATST )
		{
			if ( ucmd->upmove > 0 )
			{//get out of ATST
				GEntity_UseFunc( ent->activator, ent, ent );
				ucmd->upmove = 0;//ucmd->buttons = 0;
			}
		}

		if ( (ucmd->buttons&BUTTON_BLOCKING) && !g_saberAutoBlocking->integer )
		{
			ucmd->buttons &= ~(BUTTON_ATTACK|BUTTON_ALT_ATTACK);
		}
		PM_CheckForceUseButton( ent, ucmd );
	}

	ent->client->usercmd = *ucmd;

//	if ( !g_syncronousClients->integer ) 
	{
		ClientThink_real( ent, ucmd );
	}
	// ClientThink_real can end up freeing this ent, need to check
	if ( restore_ucmd && ent->client )
	{//restore ucmd for later so NPC you're controlling can refer to them
		memcpy( &ent->client->usercmd, &sav_ucmd, sizeof( usercmd_t ) );
	}

	if ( ent->s.number )
	{//NPCs drown, burn from lava, etc, also
		P_WorldEffects( ent );
	}
}

void ClientEndPowerUps( gentity_t *ent ) 
{
	int			i;

	if ( ent == NULL || ent->client == NULL ) 
	{
		return;
	}
	// turn off any expired powerups
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) 
	{
		if ( ent->client->ps.powerups[ i ] < level.time ) 
		{
			ent->client->ps.powerups[ i ] = 0;
		}
	}
}
/*
==============
ClientEndFrame

Called at the end of each server frame for each connected client
A fast client will have multiple ClientThink for each ClientEdFrame,
while a slow client may have multiple ClientEndFrame between ClientThink.
==============
*/
void ClientEndFrame( gentity_t *ent ) 
{
	//
	// If the end of unit layout is displayed, don't give
	// the player any normal movement attributes
	//

	// burn from lava, etc
	P_WorldEffects (ent);

	// apply all the damage taken this frame
	P_DamageFeedback (ent);

	// add the EF_CONNECTION flag if we haven't gotten commands recently
	if ( level.time - ent->client->lastCmdTime > 1000 ) {
		ent->s.eFlags |= EF_CONNECTION;
	} else {
		ent->s.eFlags &= ~EF_CONNECTION;
	}

	ent->client->ps.stats[STAT_HEALTH] = ent->health;	// FIXME: get rid of ent->health...

//	G_SetClientSound (ent);
}


