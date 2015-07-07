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

// cg_event.c -- handle entity events at snapshot or playerstate transitions

#include "cg_local.h"
#include "cg_media.h"
#include "FxScheduler.h"

#include "../game/anims.h"

extern void CG_TryPlayCustomSound( vec3_t origin, int entityNum, soundChannel_t channel, const char *soundName, int customSoundSet );

//==========================================================================

qboolean CG_IsFemale( const char *infostring ) {
	const char		*sex;

	sex = Info_ValueForKey( infostring, "s" );
	if (sex[0] == 'f' || sex[0] == 'F')
		return qtrue;
	return qfalse;
}

const char	*CG_PlaceString( int rank ) {
	static char	str[64];
	char	*s, *t;

	if ( rank & RANK_TIED_FLAG ) {
		rank &= ~RANK_TIED_FLAG;
		t = "Tied for ";
	} else {
		t = "";
	}

	if ( rank == 1 ) {
		s = "\03341st\0337";		// draw in blue
	} else if ( rank == 2 ) {
		s = "\03312nd\0337";		// draw in red
	} else if ( rank == 3 ) {
		s = "\03333rd\0337";		// draw in yellow
	} else if ( rank == 11 ) {
		s = "11th";
	} else if ( rank == 12 ) {
		s = "12th";
	} else if ( rank == 13 ) {
		s = "13th";
	} else if ( rank % 10 == 1 ) {
		s = va("%ist", rank);
	} else if ( rank % 10 == 2 ) {
		s = va("%ind", rank);
	} else if ( rank % 10 == 3 ) {
		s = va("%ird", rank);
	} else {
		s = va("%ith", rank);
	}

	Com_sprintf( str, sizeof( str ), "%s%s", t, s );
	return str;
}


/*
================
CG_ItemPickup

A new item was picked up this frame
================
*/
void CG_ItemPickup( int itemNum, qboolean bHadItem ) {
	cg.itemPickup = itemNum;
	cg.itemPickupTime = cg.time;
	cg.itemPickupBlendTime = cg.time;

	if (bg_itemlist[itemNum].classname && bg_itemlist[itemNum].classname[0])
	{
		char text[1024], data[1024];
		if (cgi_SP_GetStringTextString("INGAME_PICKUPLINE",text, sizeof(text)) )
		{
			if ( cgi_SP_GetStringTextString( va("INGAME_%s",bg_itemlist[itemNum].classname ), data, sizeof( data )))
			{
				Com_Printf("%s %s\n", text, data );
			}
		}
	}

	// see if it should be the grabbed weapon
	if ( bg_itemlist[itemNum].giType == IT_WEAPON )
	{
		const int nCurWpn = cg.predicted_player_state.weapon;
		const int nNewWpn = bg_itemlist[itemNum].giTag;

		if ( nCurWpn == WP_SABER || bHadItem)
		{//never switch away from the saber!
			return;
		}

		// kef -- check cg_autoswitch...
		//
		// 0 == no switching
		// 1 == automatically switch to best SAFE weapon
		// 2 == automatically switch to best weapon, safe or otherwise
		//
		// NOTE: automatically switching to any weapon you pick up is stupid and annoying and we won't do it.
		//

		if ( nNewWpn == WP_SABER )
		{//always switch to saber
			SetWeaponSelectTime();
			cg.weaponSelect = nNewWpn;
		}
		else if (0 == cg_autoswitch.integer)
		{
			// don't switch
		}
		else if (1 == cg_autoswitch.integer)
		{
			// safe switching
			if (	(nNewWpn > nCurWpn) &&
					!(nNewWpn == WP_DET_PACK) &&
					!(nNewWpn == WP_TRIP_MINE) &&
					!(nNewWpn == WP_THERMAL) &&
					!(nNewWpn == WP_ROCKET_LAUNCHER) )
			{
				// switch to new wpn
//				cg.weaponSelectTime = cg.time;
				SetWeaponSelectTime();
				cg.weaponSelect = nNewWpn;
			}
		}
		else if (2 == cg_autoswitch.integer)
		{
			// best
			if (nNewWpn > nCurWpn)
			{
				// switch to new wpn
//				cg.weaponSelectTime = cg.time;
				SetWeaponSelectTime();
				cg.weaponSelect = nNewWpn;
			}
		}
	}
}


/*
===============
UseItem
===============
*/
extern void CG_ToggleBinoculars( void );
extern void CG_ToggleLAGoggles( void );

void UseItem(int itemNum)
{
	centity_t		*cent;

	cent = &cg_entities[cg.snap->ps.clientNum];

	switch ( itemNum )
	{
	case INV_ELECTROBINOCULARS:
		CG_ToggleBinoculars();
		break;
	case INV_LIGHTAMP_GOGGLES:
		CG_ToggleLAGoggles();
		break;
	case INV_GOODIE_KEY:
		if (cent->gent->client->ps.inventory[INV_GOODIE_KEY])
		{
			cent->gent->client->ps.inventory[INV_GOODIE_KEY]--;
		}
		break;
	case INV_SECURITY_KEY:
		if (cent->gent->client->ps.inventory[INV_SECURITY_KEY])
		{
			cent->gent->client->ps.inventory[INV_SECURITY_KEY]--;
		}
		break;
	}
}

/*
===============
CG_UseForce
===============
*/
static void CG_UseForce( centity_t *cent )
{
	//FIXME: sound or graphic change or something?
	//actual force power action is on game/pm side
}

/*
===============
CG_UseItem
===============
*/
static void CG_UseItem( centity_t *cent )
{
	int			itemNum;
	entityState_t *es;

	es = &cent->currentState;

	itemNum = cg.inventorySelect;
	if ( itemNum < 0 || itemNum > INV_MAX )
	{
		itemNum = 0;
	}

	// print a message if the local player
	if ( es->number == cg.snap->ps.clientNum )
	{
		if ( !itemNum )
		{
//			CG_CenterPrint( "No item to use", SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH );
		}
		else
		{
//			item = BG_FindItemForHoldable( itemNum );
//			CG_CenterPrint( va("Use %s", item->pickup_name), SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH );
		}
	}

	UseItem(itemNum);

}

/*
==============
CG_EntityEvent

An entity has an event value
==============
*/
#define	DEBUGNAME(x) if(cg_debugEvents.integer){CG_Printf(x"\n");}
void CG_EntityEvent( centity_t *cent, vec3_t position ) {
	entityState_t	*es;
	int				event;
	vec3_t			axis[3];
	const char		*s, *s2;
	int				clientNum;
	//clientInfo_t	*ci;

	es = &cent->currentState;
	event = es->event & ~EV_EVENT_BITS;

	if ( cg_debugEvents.integer ) {
		CG_Printf( "ent:%3i  event:%3i ", es->number, event );
	}

	if ( !event ) {
		DEBUGNAME("ZEROEVENT");
		return;
	}

	if ( !cent->gent )//|| !cent->gent->client )
	{
		return;
	}

	//ci = &cent->gent->client->clientInfo;
	clientNum = cent->gent->s.number;

	switch ( event ) {
	//
	// movement generated events
	//
	case EV_FOOTSTEP:
		DEBUGNAME("EV_FOOTSTEP");
		if (cg_footsteps.integer) {
			if ( cent->gent && cent->gent->s.number == 0 && !cg.renderingThirdPerson )//!cg_thirdPerson.integer )
			{//Everyone else has keyframed footsteps in animsounds.cfg
				cgi_S_StartSound (NULL, es->number, CHAN_BODY,
					cgs.media.footsteps[ FOOTSTEP_NORMAL ][rand()&3] );
			}
		}
		break;
	case EV_FOOTSTEP_METAL:
		DEBUGNAME("EV_FOOTSTEP_METAL");
		if (cg_footsteps.integer)
		{
			if ( cent->gent && cent->gent->s.number == 0 && !cg.renderingThirdPerson )//!cg_thirdPerson.integer )
			{//Everyone else has keyframed footsteps in animsounds.cfg
				cgi_S_StartSound (NULL, es->number, CHAN_BODY, cgs.media.footsteps[ FOOTSTEP_METAL ][rand()&3] );
			}
		}
		break;
	case EV_FOOTSPLASH:
		DEBUGNAME("EV_FOOTSPLASH");
		if (cg_footsteps.integer) {
			cgi_S_StartSound (NULL, es->number, CHAN_BODY,
				cgs.media.footsteps[ FOOTSTEP_SPLASH ][rand()&3] );
		}
		break;
	case EV_FOOTWADE:
		DEBUGNAME("EV_FOOTWADE");
		if (cg_footsteps.integer) {
			cgi_S_StartSound (NULL, es->number, CHAN_BODY,
				cgs.media.footsteps[ FOOTSTEP_WADE ][rand()&3] );
		}
		break;
	case EV_SWIM:
		DEBUGNAME("EV_SWIM");
		if (cg_footsteps.integer) {
			cgi_S_StartSound (NULL, es->number, CHAN_BODY,
				cgs.media.footsteps[ FOOTSTEP_SWIM ][rand()&3] );
		}
		break;


	case EV_FALL_SHORT:
		DEBUGNAME("EV_FALL_SHORT");
		cgi_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.landSound  );
		if ( clientNum == cg.predicted_player_state.clientNum ) {
			// smooth landing z changes
			cg.landChange = -8;
			cg.landTime = cg.time;
		}
		//FIXME: maybe kick up some dust?
		break;
	case EV_FALL_MEDIUM:
		DEBUGNAME("EV_FALL_MEDIUM");
		// use normal pain sound -
		if ( g_entities[es->number].health <= 0 )
		{//dead
			cgi_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.landSound  );
		}
		else if ( g_entities[es->number].s.weapon == WP_SABER )
		{//jedi
			CG_TryPlayCustomSound( NULL, es->number, CHAN_BODY, "*land1.wav", CS_BASIC );
		}
		else
		{//still alive
			CG_TryPlayCustomSound( NULL, es->number, CHAN_BODY, "*pain100.wav", CS_BASIC );
		}
		if ( clientNum == cg.predicted_player_state.clientNum ) {
			// smooth landing z changes
			cg.landChange = -16;
			cg.landTime = cg.time;
		}
		//FIXME: maybe kick up some dust?
		break;
	case EV_FALL_FAR:
		DEBUGNAME("EV_FALL_FAR");
		CG_TryPlayCustomSound( NULL, es->number, CHAN_BODY, "*land1.wav", CS_BASIC );
		cgi_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.landSound  );
		cent->pe.painTime = cg.time;	// don't play a pain sound right after this
		if ( clientNum == cg.predicted_player_state.clientNum ) {
			// smooth landing z changes
			cg.landChange = -24;
			cg.landTime = cg.time;
		}
		//FIXME: maybe kick up some dust?
		break;

	case EV_STEP_4:
	case EV_STEP_8:
	case EV_STEP_12:
	case EV_STEP_16:		// smooth out step up transitions
		DEBUGNAME("EV_STEP");
	{
		float	oldStep;
		int		delta;
		int		step;

		if ( clientNum != cg.predicted_player_state.clientNum ) {
			break;
		}
		// if we are interpolating, we don't need to smooth steps
		if ( cg_timescale.value >= 1.0f )
		{
			break;
		}
		// check for stepping up before a previous step is completed
		delta = cg.time - cg.stepTime;
		if (delta < STEP_TIME) {
			oldStep = cg.stepChange * (STEP_TIME - delta) / STEP_TIME;
		} else {
			oldStep = 0;
		}

		// add this amount
		step = 4 * (event - EV_STEP_4 + 1 );
		cg.stepChange = oldStep + step;
		if ( cg.stepChange > MAX_STEP_CHANGE ) {
			cg.stepChange = MAX_STEP_CHANGE;
		}
		cg.stepTime = cg.time;
		break;
	}

	case EV_JUMP:
		DEBUGNAME("EV_JUMP");
		CG_TryPlayCustomSound(NULL, es->number, CHAN_AUTO, "*jump1.wav", CS_BASIC );//CHAN_VOICE
		break;

	case EV_ROLL:
		DEBUGNAME("EV_ROLL");
		CG_TryPlayCustomSound(NULL, es->number, CHAN_AUTO, "*jump1.wav", CS_BASIC );//CHAN_VOICE
		cgi_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.rollSound  );//CHAN_AUTO
		//FIXME: need some sort of body impact on ground sound and maybe kick up some dust?
		break;

	case EV_WATER_TOUCH:
		DEBUGNAME("EV_WATER_TOUCH");
		cgi_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.watrInSound );
		break;

	case EV_WATER_LEAVE:
		DEBUGNAME("EV_WATER_LEAVE");
		cgi_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.watrOutSound );
		break;

	case EV_WATER_UNDER:
		DEBUGNAME("EV_WATER_UNDER");
		cgi_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.watrUnSound );
		break;

	case EV_WATER_CLEAR:
		DEBUGNAME("EV_WATER_CLEAR");
		CG_TryPlayCustomSound(NULL, es->number, CHAN_AUTO, "*gasp.wav", CS_BASIC );
		break;

	case EV_WATER_GURP1:
	case EV_WATER_GURP2:
		DEBUGNAME("EV_WATER_GURPx");
		CG_TryPlayCustomSound(NULL, es->number, CHAN_AUTO, va("*gurp%d.wav",event-EV_WATER_GURP1+1), CS_BASIC );
		break;

	case EV_WATER_DROWN:
		DEBUGNAME("EV_WATER_DROWN");
		CG_TryPlayCustomSound(NULL, es->number, CHAN_AUTO, "*drown.wav", CS_BASIC );
		break;

	case EV_ITEM_PICKUP:
		DEBUGNAME("EV_ITEM_PICKUP");
		{
			gitem_t	*item;
			int		index;
			qboolean bHadItem = qfalse;

			index = es->eventParm;		// player predicted

			if ( (char)index < 0 )
			{
				index = -(char)index;
				bHadItem = qtrue;
			}

			if ( index >= bg_numItems ) {
				break;
			}
			item = &bg_itemlist[ index ];
			cgi_S_StartSound (NULL, es->number, CHAN_AUTO,	cgi_S_RegisterSound( item->pickup_sound ) );

			// show icon and name on status bar
			if ( es->number == cg.snap->ps.clientNum ) {
				CG_ItemPickup( index, bHadItem );
			}
		}
		break;

	//
	// weapon events
	//
	case EV_NOAMMO:
		DEBUGNAME("EV_NOAMMO");
		//cgi_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.noAmmoSound );
		if ( es->number == cg.snap->ps.clientNum ) {
			CG_OutOfAmmoChange();
		}
		break;
	case EV_CHANGE_WEAPON:
		DEBUGNAME("EV_CHANGE_WEAPON");
		if ( es->weapon == WP_SABER )
		{
			/*
			if ( !cent->gent || !cent->gent->client || (cent->currentState.saberInFlight == qfalse && cent->currentState.saberActive == qtrue) )
			{
				cgi_S_StartSound (NULL, es->number, CHAN_AUTO, cgi_S_RegisterSound( "sound/weapons/saber/saberoffquick.wav" ) );
			}
			*/
			if ( cent->gent && cent->gent->client )
			{
				//if ( cent->gent->client->ps.saberInFlight )
				{//if it's not in flight or lying around, turn it off!
					cent->currentState.saberActive = qfalse;
				}
			}
		}

		// FIXME: if it happens that you don't want the saber to play the switch sounds, feel free to modify this bit.
		if ( weaponData[cg.weaponSelect].selectSnd[0] )
		{
			// custom select sound
			cgi_S_StartSound (NULL, es->number, CHAN_AUTO, cgi_S_RegisterSound( weaponData[cg.weaponSelect].selectSnd ));
		}
		else
		{
			// generic sound
			cgi_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.selectSound );
		}
		break;

	case EV_FIRE_WEAPON:
		DEBUGNAME("EV_FIRE_WEAPON");
		CG_FireWeapon( cent, qfalse );
		break;

	case EV_ALT_FIRE:
		DEBUGNAME("EV_ALT_FIRE");
		CG_FireWeapon( cent, qtrue );
		break;

	case EV_DISRUPTOR_MAIN_SHOT:
		DEBUGNAME("EV_DISRUPTOR_MAIN_SHOT");
		FX_DisruptorMainShot( cent->currentState.origin2, cent->lerpOrigin );
		break;

	case EV_DISRUPTOR_SNIPER_SHOT:
		DEBUGNAME("EV_DISRUPTOR_SNIPER_SHOT");
		FX_DisruptorAltShot( cent->currentState.origin2, cent->lerpOrigin, cent->gent->alt_fire );
		break;

	case EV_DISRUPTOR_SNIPER_MISS:
		DEBUGNAME("EV_DISRUPTOR_SNIPER_MISS");
		FX_DisruptorAltMiss( cent->lerpOrigin, cent->gent->pos1 );
		break;

	case EV_DEMP2_ALT_IMPACT:
		FX_DEMP2_AltDetonate( cent->lerpOrigin, es->eventParm );
		break;

//	case EV_POWERUP_SEEKER_FIRE:
//		DEBUGNAME("EV_POWERUP_SEEKER_FIRE");
//		CG_FireSeeker( cent );
//		break;

	case EV_POWERUP_BATTLESUIT:
		DEBUGNAME("EV_POWERUP_BATTLESUIT");
		if ( es->number == cg.snap->ps.clientNum ) {
			cg.powerupActive = PW_BATTLESUIT;
			cg.powerupTime = cg.time;
		}
		//cgi_S_StartSound (NULL, es->number, CHAN_ITEM, cgs.media.invulnoProtectSound );
		break;

	//=================================================================

	//
	// other events
	//
	case EV_REPLICATOR:
		DEBUGNAME("EV_REPLICATOR");
//		FX_Replicator( cent, position );
		break;

	case EV_BATTERIES_CHARGED:
		cg.batteryChargeTime = cg.time + 3000;
		cgi_S_StartSound( (float*)vec3_origin, es->number, CHAN_AUTO, cgs.media.batteryChargeSound );
		break;

	case EV_DISINTEGRATION:
		{
			DEBUGNAME("EV_DISINTEGRATION");
			qboolean makeNotSolid = qfalse;
			int disintPW = es->eventParm;
			int disintEffect = 0;
			int disintLength = 0;
			qhandle_t disintSound1 = NULL_HANDLE;
			qhandle_t disintSound2 = NULL_HANDLE;

			switch( disintPW )
			{
			case PW_DISRUPTION:// sniper rifle
				disintEffect = EF_DISINTEGRATION;//ef_
				disintSound1 = cgs.media.disintegrateSound;//with scream
				disintSound2 = cgs.media.disintegrate2Sound;//no scream
				disintLength = 2000;
				makeNotSolid = qtrue;
				break;
/*			case PW_SHOCKED:// arc welder
				disintEffect = EF_DISINT_1;//ef_
				disintSound1 = NULL;//with scream
				disintSound2 = NULL;//no scream
				disintSound3 = NULL;//with inhuman scream
				disintLength = 4000;
				break;
*/
			default:
				return;
				break;
			}

			if ( cent->gent->owner )
			{
				cent->gent->owner->fx_time = cg.time;
				if ( cent->gent->owner->client )
				{
					if ( disintSound1 && disintSound2 )
					{//play an extra sound
					/*
						if ( cent->gent->owner->client->playerTeam == TEAM_STARFLEET ||
								cent->gent->owner->client->playerTeam == TEAM_SCAVENGERS ||
								cent->gent->owner->client->playerTeam == TEAM_MALON ||
								cent->gent->owner->client->playerTeam == TEAM_IMPERIAL ||
								cent->gent->owner->client->playerTeam == TEAM_HIROGEN ||
								cent->gent->owner->client->playerTeam == TEAM_DISGUISE ||
								cent->gent->owner->client->playerTeam == TEAM_KLINGON )
					*/
						// listed all the non-humanoids, because there's a lot more humanoids
						class_t	npc_class = cent->gent->owner->client->NPC_class;
						if( npc_class != CLASS_ATST && npc_class != CLASS_GONK &&
							npc_class != CLASS_INTERROGATOR && npc_class != CLASS_MARK1 &&
							npc_class != CLASS_MARK2 && npc_class != CLASS_MOUSE &&
							npc_class != CLASS_PROBE && npc_class != CLASS_PROTOCOL &&
							npc_class != CLASS_R2D2 && npc_class != CLASS_R5D2 &&
							npc_class != CLASS_SEEKER && npc_class != CLASS_SENTRY)
						{//Only the humanoids scream
							cgi_S_StartSound ( NULL, cent->gent->owner->s.number, CHAN_VOICE, disintSound1 );
						}
						// no more forge or 8472
					//	else if ( cent->gent->owner->client->playerTeam == TEAM_FORGE ||
					//			cent->gent->owner->client->playerTeam == TEAM_8472 )
					//	{
					//		cgi_S_StartSound ( NULL, cent->gent->s.number, CHAN_VOICE, disintSound3 );
					//	}
						else
						{
							cgi_S_StartSound ( NULL, cent->gent->s.number, CHAN_AUTO, disintSound2 );
						}
					}
					cent->gent->owner->s.powerups |= ( 1 << disintPW );
					cent->gent->owner->client->ps.powerups[disintPW] = cg.time + disintLength;

					// Things that are being disintegrated should probably not be solid...
					if ( makeNotSolid && cent->gent->owner->client->playerTeam != TEAM_NEUTRAL )
					{
						cent->gent->contents = CONTENTS_NONE;
					}
				}
				else
				{
					cent->gent->owner->s.eFlags = disintEffect;//FIXME: |= ?
					cent->gent->owner->delay = cg.time + disintLength;
				}
			}
		}
		break;

	// This does not necessarily have to be from a grenade...
	case EV_GRENADE_BOUNCE:
		DEBUGNAME("EV_GRENADE_BOUNCE");
		CG_BounceEffect( cent, es->weapon, position, cent->gent->pos1 );
		break;

	//
	// missile impacts
	//

	case EV_MISSILE_STICK:
		DEBUGNAME("EV_MISSILE_STICK");
		CG_MissileStick( cent, es->weapon, position );
		break;

	case EV_MISSILE_HIT:
		DEBUGNAME("EV_MISSILE_HIT");
		CG_MissileHitPlayer( cent, es->weapon, position, cent->gent->pos1, cent->gent->alt_fire );
		break;

	case EV_MISSILE_MISS:
		DEBUGNAME("EV_MISSILE_MISS");
		CG_MissileHitWall( cent, es->weapon, position, cent->gent->pos1, cent->gent->alt_fire );
		break;

	case EV_BMODEL_SOUND:
		DEBUGNAME("EV_BMODEL_SOUND");
		cgi_S_StartSound( NULL, es->number, CHAN_AUTO, es->eventParm );
		break;

	case EV_GENERAL_SOUND:
		DEBUGNAME("EV_GENERAL_SOUND");
		if ( cgs.sound_precache[ es->eventParm ] )
		{
			cgi_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.sound_precache[ es->eventParm ] );
		}
		else
		{
			s = CG_ConfigString( CS_SOUNDS + es->eventParm );
			CG_TryPlayCustomSound(NULL, es->number, CHAN_AUTO, s, CS_BASIC );
		}
		break;

	case EV_GLOBAL_SOUND:	// play from the player's head so it never diminishes
		DEBUGNAME("EV_GLOBAL_SOUND");
		if ( cgs.sound_precache[ es->eventParm ] ) {
			cgi_S_StartSound (NULL, cg.snap->ps.clientNum, CHAN_AUTO, cgs.sound_precache[ es->eventParm ] );
		} else {
			s = CG_ConfigString( CS_SOUNDS + es->eventParm );
			CG_TryPlayCustomSound( NULL, cg.snap->ps.clientNum, CHAN_AUTO, s, CS_BASIC );
		}
		break;

	case EV_DRUGGED:
		DEBUGNAME("EV_DRUGGED");
		if ( cent->gent && cent->gent->owner && cent->gent->owner->s.number == 0 )
		{
			// Only allow setting up the wonky vision on the player..do it for 10 seconds...must be synchronized with calcs done in cg_view.  Just search for cg.wonkyTime to find 'em.
			cg.wonkyTime = cg.time + 10000;
		}
		break;

	case EV_PAIN:
		{
		char	*snd;
		const int health = es->eventParm;

		if ( cent->gent && cent->gent->NPC && (cent->gent->NPC->aiFlags & NPCAI_DIE_ON_IMPACT) )
		{
			return;
		}
		//FIXME: don't do this if we're falling to our deaths...
		DEBUGNAME("EV_PAIN");
		// don't do more than two pain sounds a second
		if ( cg.time - cent->pe.painTime < 500 ) {
			return;
		}

		if ( health < 25 ) {
			snd = "*pain100.wav";
		} else if ( health < 50 ) {
			snd = "*pain75.wav";
		} else if ( health < 75 ) {
			snd = "*pain50.wav";
		} else {
			snd = "*pain25.wav";
		}
		CG_TryPlayCustomSound( NULL, es->number, CHAN_VOICE, snd, CS_BASIC );

		// save pain time for programitic twitch animation
		cent->pe.painTime = cg.time;
		cent->pe.painDirection ^= 1;
		}
		break;

	case EV_DEATH1:
	case EV_DEATH2:
	case EV_DEATH3:
		DEBUGNAME("EV_DEATHx");
		/*
		if ( cent->gent && cent->gent->NPC && (cent->gent->NPC->aiFlags & NPCAI_DIE_ON_IMPACT) )
		{
			return;
		}
		*/
		CG_TryPlayCustomSound( NULL, es->number, CHAN_VOICE, va("*death%i.wav", event - EV_DEATH1 + 1), CS_BASIC );
		break;

	// Called by the FxRunner entity...usually for Environmental FX Events
	case EV_PLAY_EFFECT:
		DEBUGNAME("EV_PLAY_EFFECT");
		s = CG_ConfigString( CS_EFFECTS + es->eventParm );
// Ghoul2 Insert Start
		if (es->boltInfo != 0)
		{
			theFxScheduler.PlayEffect( s, cent->lerpOrigin, axis, es->boltInfo, -1 );
		}
		else
		{
			VectorCopy( cent->gent->pos3, axis[0] );
			VectorCopy( cent->gent->pos4, axis[1] );
			CrossProduct( axis[0], axis[1], axis[2] );

			// the entNum the effect may be attached to
			if ( es->otherEntityNum )
			{
				theFxScheduler.PlayEffect( s, cent->lerpOrigin, axis, -1, es->otherEntityNum );
			}
			else
			{
				theFxScheduler.PlayEffect( s, cent->lerpOrigin, axis, -1, -1 );
			}
		}
// Ghoul2 Insert End
		break;

		// play an effect bolted onto a muzzle
	case EV_PLAY_MUZZLE_EFFECT:
		DEBUGNAME("EV_PLAY_MUZZLE_EFFECT");
		s = CG_ConfigString( CS_EFFECTS + es->eventParm );

		theFxScheduler.PlayEffect( s, es->otherEntityNum );
		break;

	case EV_TARGET_BEAM_DRAW:
		DEBUGNAME("EV_TARGET_BEAM_DRAW");
		if ( cent->gent )
		{
			s = CG_ConfigString( CS_EFFECTS + es->eventParm );

			if ( s && s[0] )
			{
				if ( cent->gent->delay )
				{
					s2 = CG_ConfigString( CS_EFFECTS + cent->gent->delay );
				}
				else
				{
					s2 = NULL;
				}

				CG_DrawTargetBeam( cent->lerpOrigin, cent->gent->s.origin2, cent->gent->pos1, s, s2 );
			}
/*			else
			{
				int gack = 0; // this is bad if it get's here
			}
*/
		}
		break;

	case EV_ANGER1:	//Say when acquire an enemy when didn't have one before
	case EV_ANGER2:
	case EV_ANGER3:
		DEBUGNAME("EV_ANGERx");
		CG_TryPlayCustomSound( NULL, es->number, CHAN_VOICE, va("*anger%i.wav", event - EV_ANGER1 + 1), CS_COMBAT );
		break;

	case EV_VICTORY1:	//Say when killed an enemy
	case EV_VICTORY2:
	case EV_VICTORY3:
		DEBUGNAME("EV_VICTORYx");
		CG_TryPlayCustomSound( NULL, es->number, CHAN_VOICE, va("*victory%i.wav", event - EV_VICTORY1 + 1), CS_COMBAT );
		break;

	case EV_CONFUSE1:	//Say when confused
	case EV_CONFUSE2:
	case EV_CONFUSE3:
		DEBUGNAME("EV_CONFUSEDx");
		CG_TryPlayCustomSound( NULL, es->number, CHAN_VOICE, va("*confuse%i.wav", event - EV_CONFUSE1 + 1), CS_COMBAT );
		break;

	case EV_PUSHED1:	//Say when pushed
	case EV_PUSHED2:
	case EV_PUSHED3:
		DEBUGNAME("EV_PUSHEDx");
		CG_TryPlayCustomSound( NULL, es->number, CHAN_VOICE, va("*pushed%i.wav", event - EV_PUSHED1 + 1), CS_COMBAT );
		break;

	case EV_CHOKE1:	//Say when choking
	case EV_CHOKE2:
	case EV_CHOKE3:
		DEBUGNAME("EV_CHOKEx");
		CG_TryPlayCustomSound( NULL, es->number, CHAN_VOICE, va("*choke%i.wav", event - EV_CHOKE1 + 1), CS_COMBAT );
		break;

	case EV_FFWARN:	//Warn ally to stop shooting you
		DEBUGNAME("EV_FFWARN");
		CG_TryPlayCustomSound( NULL, es->number, CHAN_VOICE, "*ffwarn.wav", CS_COMBAT );
		break;

	case EV_FFTURN:	//Turn on ally after being shot by them
		DEBUGNAME("EV_FFTURN");
		CG_TryPlayCustomSound( NULL, es->number, CHAN_VOICE, "*ffturn.wav", CS_COMBAT );
		break;

	//extra sounds for ST
	case EV_CHASE1:
	case EV_CHASE2:
	case EV_CHASE3:
		DEBUGNAME("EV_CHASEx");
		CG_TryPlayCustomSound( NULL, es->number, CHAN_VOICE, va("*chase%i.wav", event - EV_CHASE1 + 1), CS_EXTRA );
		break;
	case EV_COVER1:
	case EV_COVER2:
	case EV_COVER3:
	case EV_COVER4:
	case EV_COVER5:
		DEBUGNAME("EV_COVERx");
		CG_TryPlayCustomSound( NULL, es->number, CHAN_VOICE, va("*cover%i.wav", event - EV_COVER1 + 1), CS_EXTRA );
		break;
	case EV_DETECTED1:
	case EV_DETECTED2:
	case EV_DETECTED3:
	case EV_DETECTED4:
	case EV_DETECTED5:
		DEBUGNAME("EV_DETECTEDx");
		CG_TryPlayCustomSound( NULL, es->number, CHAN_VOICE, va("*detected%i.wav", event - EV_DETECTED1 + 1), CS_EXTRA );
		break;
	case EV_GIVEUP1:
	case EV_GIVEUP2:
	case EV_GIVEUP3:
	case EV_GIVEUP4:
		DEBUGNAME("EV_GIVEUPx");
		CG_TryPlayCustomSound( NULL, es->number, CHAN_VOICE, va("*giveup%i.wav", event - EV_GIVEUP1 + 1), CS_EXTRA );
		break;
	case EV_LOOK1:
	case EV_LOOK2:
		DEBUGNAME("EV_LOOKx");
		CG_TryPlayCustomSound( NULL, es->number, CHAN_VOICE, va("*look%i.wav", event - EV_LOOK1 + 1), CS_EXTRA );
		break;
	case EV_LOST1:
		DEBUGNAME("EV_LOST1");
		CG_TryPlayCustomSound( NULL, es->number, CHAN_VOICE, "*lost1.wav", CS_EXTRA );
		break;
	case EV_OUTFLANK1:
	case EV_OUTFLANK2:
		DEBUGNAME("EV_OUTFLANKx");
		CG_TryPlayCustomSound( NULL, es->number, CHAN_VOICE, va("*outflank%i.wav", event - EV_OUTFLANK1 + 1), CS_EXTRA );
		break;
	case EV_ESCAPING1:
	case EV_ESCAPING2:
	case EV_ESCAPING3:
		DEBUGNAME("EV_ESCAPINGx");
		CG_TryPlayCustomSound( NULL, es->number, CHAN_VOICE, va("*escaping%i.wav", event - EV_ESCAPING1 + 1), CS_EXTRA );
		break;
	case EV_SIGHT1:
	case EV_SIGHT2:
	case EV_SIGHT3:
		DEBUGNAME("EV_SIGHTx");
		CG_TryPlayCustomSound( NULL, es->number, CHAN_VOICE, va("*sight%i.wav", event - EV_SIGHT1 + 1), CS_EXTRA );
		break;
	case EV_SOUND1:
	case EV_SOUND2:
	case EV_SOUND3:
		DEBUGNAME("EV_SOUNDx");
		CG_TryPlayCustomSound( NULL, es->number, CHAN_VOICE, va("*sound%i.wav", event - EV_SOUND1 + 1), CS_EXTRA );
		break;
	case EV_SUSPICIOUS1:
	case EV_SUSPICIOUS2:
	case EV_SUSPICIOUS3:
	case EV_SUSPICIOUS4:
	case EV_SUSPICIOUS5:
		DEBUGNAME("EV_SUSPICIOUSx");
		CG_TryPlayCustomSound( NULL, es->number, CHAN_VOICE, va("*suspicious%i.wav", event - EV_SUSPICIOUS1 + 1), CS_EXTRA );
		break;
	//extra sounds for Jedi
	case EV_COMBAT1:
	case EV_COMBAT2:
	case EV_COMBAT3:
		DEBUGNAME("EV_COMBATx");
		CG_TryPlayCustomSound( NULL, es->number, CHAN_VOICE, va("*combat%i.wav", event - EV_COMBAT1 + 1), CS_JEDI );
		break;
	case EV_JDETECTED1:
	case EV_JDETECTED2:
	case EV_JDETECTED3:
		DEBUGNAME("EV_JDETECTEDx");
		CG_TryPlayCustomSound( NULL, es->number, CHAN_VOICE, va("*jdetected%i.wav", event - EV_JDETECTED1 + 1), CS_JEDI );
		break;
	case EV_TAUNT1:
	case EV_TAUNT2:
	case EV_TAUNT3:
		DEBUGNAME("EV_TAUNTx");
		CG_TryPlayCustomSound( NULL, es->number, CHAN_VOICE, va("*taunt%i.wav", event - EV_TAUNT1 + 1), CS_JEDI );
		break;
	case EV_JCHASE1:
	case EV_JCHASE2:
	case EV_JCHASE3:
		DEBUGNAME("EV_JCHASEx");
		CG_TryPlayCustomSound( NULL, es->number, CHAN_VOICE, va("*jchase%i.wav", event - EV_JCHASE1 + 1), CS_JEDI );
		break;
	case EV_JLOST1:
	case EV_JLOST2:
	case EV_JLOST3:
		DEBUGNAME("EV_JLOSTx");
		CG_TryPlayCustomSound( NULL, es->number, CHAN_VOICE, va("*jlost%i.wav", event - EV_JLOST1 + 1), CS_JEDI );
		break;
	case EV_DEFLECT1:
	case EV_DEFLECT2:
	case EV_DEFLECT3:
		DEBUGNAME("EV_DEFLECTx");
		CG_TryPlayCustomSound( NULL, es->number, CHAN_VOICE, va("*deflect%i.wav", event - EV_DEFLECT1 + 1), CS_JEDI );
		break;
	case EV_GLOAT1:
	case EV_GLOAT2:
	case EV_GLOAT3:
		DEBUGNAME("EV_GLOATx");
		CG_TryPlayCustomSound( NULL, es->number, CHAN_VOICE, va("*gloat%i.wav", event - EV_GLOAT1 + 1), CS_JEDI );
		break;
	case EV_PUSHFAIL:
		DEBUGNAME("EV_PUSHFAIL");
		CG_TryPlayCustomSound( NULL, es->number, CHAN_VOICE, "*pushfail.wav", CS_JEDI );
		break;

	case EV_USE_FORCE:
		DEBUGNAME("EV_USE_FORCEITEM");
		CG_UseForce( cent );
		break;

	case EV_USE_ITEM:
		DEBUGNAME("EV_USE_ITEM");
		CG_UseItem( cent );
		break;

	case EV_USE_INV_BINOCULARS:
		DEBUGNAME("EV_USE_INV_BINOCULARS");
		UseItem(INV_ELECTROBINOCULARS );
		break;

	case EV_USE_INV_BACTA:
		DEBUGNAME("EV_USE_INV_BACTA");
		UseItem(INV_BACTA_CANISTER );
		break;

	case EV_USE_INV_SEEKER:
		DEBUGNAME("EV_USE_INV_SEEKER");
		UseItem(INV_SEEKER );
		break;

	case EV_USE_INV_LIGHTAMP_GOGGLES:
		DEBUGNAME("EV_USE_INV_LIGHTAMP_GOGGLES");
		UseItem(INV_LIGHTAMP_GOGGLES );
		break;

	case EV_USE_INV_SENTRY:
		DEBUGNAME("EV_USE_INV_SENTRY");
		UseItem(INV_SENTRY );
		break;

	case EV_DEBUG_LINE:
		DEBUGNAME("EV_DEBUG_LINE");
		CG_TestLine(position, es->origin2, es->time, (unsigned int)(es->time2), es->weapon);
		break;

	default:
		DEBUGNAME("UNKNOWN");
		CG_Error( "Unknown event: %i", event );
		break;
	}

}


/*
==============
CG_CheckEvents

==============
*/
void CG_CheckEvents( centity_t *cent ) {
	// check for event-only entities
	if ( cent->currentState.eType > ET_EVENTS ) {
		if ( cent->previousEvent ) {
			return;	// already fired
		}
		cent->previousEvent = 1;

		cent->currentState.event = cent->currentState.eType - ET_EVENTS;
	} else {
		// check for events riding with another entity
		if ( cent->currentState.event == cent->previousEvent ) {
			return;
		}
		cent->previousEvent = cent->currentState.event;
		if ( ( cent->currentState.event & ~EV_EVENT_BITS ) == 0 ) {
			return;
		}
	}

	// calculate the position at exactly the frame time
	EvaluateTrajectory( &cent->currentState.pos, cg.snap->serverTime, cent->lerpOrigin );
	CG_SetEntitySoundPosition( cent );

	CG_EntityEvent( cent, cent->lerpOrigin );
}

