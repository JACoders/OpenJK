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

// included in both game dll and client

#include "g_local.h"
#include "bg_public.h"
#include "g_items.h"
#include "g_vehicles.h"


extern weaponData_t weaponData[WP_NUM_WEAPONS];
extern ammoData_t ammoData[AMMO_MAX];


#define PICKUPSOUND "sound/weapons/w_pkup.wav"

/*QUAKED weapon_***** ( 0 0 0 ) (-16 -16 -16) (16 16 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID VERTICAL INVISIBLE NOGLOW USEPICKUP STATIONARY
DO NOT USE THIS CLASS, IT JUST HOLDS GENERAL INFORMATION for weapons, ammo, and item pickups.
The suspended flag will allow items to hang in the air, otherwise they are dropped to the next surface.
The NOPLAYER flag makes it so player cannot pick it up.
The ALLOWNPC flag allows only NPCs to pick it up, too
USEPICKUP - Player must be holding "use" to pick it up
STATIONARY - Cannot be moved around by force push/pull, radius damage, knockback, etc...

If an item is the target of another entity, it will spawn as normal, use INVISIBLE to hide it.

An item fires all of its targets when it is picked up.  If the toucher can't carry it, the targets won't be fired.

"wait"	override the default wait before respawning.  -1 = never respawn automatically, which can be used with targeted spawning.
"random" random number of plus or minus seconds varied from the respawn time
"count" override quantity or duration on most items.
"team" only this team can pick it up
	"player"
	"neutral"
	"enemy"
*/

/*QUAKED weapon_stun_baton (.3 .3 1) (-16 -16 -2) (16 16 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID VERTICAL INVISIBLE NOGLOW USEPICKUP STATIONARY NOGLOW USEPICKUP STATIONARY
model="/models/weapons2/stun_baton/baton.md3"
*/
/*QUAKED weapon_saber (.3 .3 1) (-16 -16 -8) (16 16 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID LEANING INVISIBLE NOGLOW USEPICKUP STATIONARY
SUSPENDED - allow items to hang in the air, otherwise they are dropped to the next surface.
NOPLAYER - makes it so player cannot pick it up.
ALLOWNPC - allows only NPCs to pick it up, too
LEANING - lean back against wall
NOGLOW - No Glow
USEPICKUP - Player must be holding "use" to pick it up
STATIONARY - Cannot be moved around by force push/pull, radius damage, knockback, etc...

model="/models/weapons2/saber/saber_w.md3"
When picked up, will be used as a second saber unless:
	1) It's a two-handed saber
	2) The old saber was two-handed
	3) You set "saberSolo" to "1"
	4) You have 2 sabers and the saber pickup is on your right when you touch it

saberType - entry name from sabers.cfg - which kind of saber this is - use "player" to make it so that the saber will be whatever saber the player is configured to use
saberColor - red, orange, yellow, green, blue, and purple
saberLeftHand - always be added as a left-hand saber
saberSolo - set to "1" and this will be the only saber the person who picks this up will be holding
saberPitch - if set "LEANING" flag, you can specify the exact pitch to lean forward/back
count - how many you can pick up before it's removed (default is 1, -1 is infinite)
*/
/*QUAKED weapon_bryar_pistol (.3 .3 1) (-16 -16 -2) (16 16 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID VERTICAL INVISIBLE NOGLOW USEPICKUP STATIONARY
model="/models/weapons2/briar_pistol/briar_pistol.md3"
*/
/*QUAKED weapon_blaster_pistol (.3 .3 1) (-16 -16 -2) (16 16 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID VERTICAL INVISIBLE NOGLOW USEPICKUP STATIONARY
model="/models/weapons2/blaster_pistol/blaster_pistol.md3"
*/
/*QUAKED weapon_blaster (.3 .3 1) (-16 -16 -2) (16 16 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID VERTICAL INVISIBLE NOGLOW USEPICKUP STATIONARY
model="/models/weapons2/blaster_r/blaster.md3"
*/
/*QUAKED weapon_disruptor (.3 .3 1) (-16 -16 -2) (16 16 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID VERTICAL INVISIBLE NOGLOW USEPICKUP STATIONARY
model="/models/weapons2/disruptor/disruptor.md3"
*/
/*QUAKED weapon_bowcaster (.3 .3 1) (-16 -16 -2) (16 16 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID VERTICAL INVISIBLE NOGLOW USEPICKUP STATIONARY
model="/models/weapons2/bowcaster/bowcaster.md3"
*/
/*QUAKED weapon_repeater (.3 .3 1) (-16 -16 -2) (16 16 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID VERTICAL INVISIBLE NOGLOW USEPICKUP STATIONARY
model="/models/weapons2/heavy_repeater/heavy_repeater.md3"
*/
/*QUAKED weapon_demp2 (.3 .3 1) (-16 -16 -2) (16 16 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID VERTICAL INVISIBLE NOGLOW USEPICKUP STATIONARY
model="/models/weapons2/demp2/demp2.md3"
*/
/*QUAKED weapon_flechette (.3 .3 1) (-16 -16 -2) (16 16 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID VERTICAL INVISIBLE NOGLOW USEPICKUP STATIONARY
model="/models/weapons2/golan_arms/golan_arms.md3"
*/
/*QUAKED weapon_concussion_rifle (.3 .3 1) (-16 -16 -2) (16 16 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID VERTICAL INVISIBLE NOGLOW USEPICKUP STATIONARY
model="/models/weapons2/c_rifle/c_rifle.md3"
*/
/*QUAKED weapon_rocket_launcher (.3 .3 1) (-16 -16 -2) (16 16 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID VERTICAL INVISIBLE NOGLOW USEPICKUP STATIONARY
model="/models/weapons2/merr_sonn/merr_sonn.md3"
*/
/*QUAKED weapon_thermal (.3 .3 1) (-16 -16 -2) (16 16 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID VERTICAL INVISIBLE NOGLOW USEPICKUP STATIONARY
model="/models/weapons2/thermal/thermal.md3"
*/
/*QUAKED weapon_trip_mine (.3 .3 1) (-16 -16 -2) (16 16 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID VERTICAL INVISIBLE NOGLOW USEPICKUP STATIONARY
model="/models/weapons2/laser_trap/laser_trap.md3"
*/
/*QUAKED weapon_det_pack (.3 .3 1) (-16 -16 -2) (16 16 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID VERTICAL INVISIBLE NOGLOW USEPICKUP STATIONARY
model="/models/weapons2/detpack/det_pack.md3"
*/

/*QUAKED item_seeker (.3 .3 1) (-8 -8 -4) (8 8 16) suspended
30 seconds of seeker drone
*/
/*QUAKED item_bacta (.3 .3 1) (-8 -8 0) (8 8 16) suspended
model="/models/items/bacta.md3"
*/
/*QUAKED item_datapad (.3 .3 1) (-8 -8 0) (8 8 16) suspended
model="/models/items/datapad.md3"
*/
/*QUAKED item_binoculars (.3 .3 1) (-8 -8 0) (8 8 16) suspended
model="/models/items/binoculars.md3"
*/
/*QUAKED item_sentry_gun (.3 .3 1) (-8 -8 0) (8 8 16) suspended
*/
/*QUAKED item_la_goggles (.3 .3 1) (-8 -8 0) (8 8 16) suspended
*/
/*QUAKED ammo_force (.3 .5 1) (-8 -8 -0) (8 8 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID
Ammo for the force.
*/
/*QUAKED ammo_blaster (.3 .5 1) (-8 -8 -0) (8 8 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID
Ammo for the Bryar and Blaster pistols.
*/
/*QUAKED ammo_powercell (.3 .5 1) (-8 -8 -0) (8 8 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID
Ammo for Tenloss Disruptor, Wookie Bowcaster, and the Destructive Electro Magnetic Pulse (demp2 ) guns
*/
/*QUAKED ammo_metallic_bolts (.3 .5 1) (-8 -8 -0) (8 8 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID
Ammo for Imperial Heavy Repeater and the Golan Arms Flechette
*/
/*QUAKED ammo_rockets (.3 .5 1) (-8 -8 -0) (8 8 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID
Ammo for Merr-Sonn portable missile launcher
*/
/*QUAKED ammo_thermal (.3 .5 1) (-16 -16 -0) (16 16 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID
Belt of thermal detonators
*/
/*QUAKED ammo_tripmine (.3 .5 1) (-8 -8 -0) (8 8 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID
3 pack of tripmines
*/
/*QUAKED ammo_detpack (.3 .5 1) (-8 -8 -0) (8 8 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID
3 pack of detpacks
*/

/*QUAKED item_medpak_instant (.3 .3 1) (-8 -8 -4) (8 8 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID VERTICAL INVISIBLE NOGLOW USEPICKUP STATIONARY
*/

/*QUAKED item_shield_sm_instant (.3 .3 1) (-8 -8 -4) (8 8 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID VERTICAL INVISIBLE NOGLOW USEPICKUP STATIONARY
*/

/*QUAKED item_shield_lrg_instant (.3 .3 1) (-8 -8 -4) (8 8 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID VERTICAL INVISIBLE NOGLOW USEPICKUP STATIONARY
*/
/*QUAKED item_goodie_key (.3 .3 1) (-8 -8 0) (8 8 16) suspended
*/
/*QUAKED item_security_key (.3 .3 1) (-8 -8 0) (8 8 16) suspended
message - used to differentiate one key from another.
*/
/*QUAKED item_battery (.3 .5 1) (-8 -8 -0) (8 8 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID
model="/models/items/battery.md3"
battery pickup item
*/

/*QUAKED holocron_force_heal (.3 .5 1) (-8 -8 -0) (8 8 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID
force heal pickup item

"count"     level of force power this holocron gives activator ( range: 0-3, default 1)
*/

/*QUAKED holocron_force_levitation (.3 .5 1) (-8 -8 -0) (8 8 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID
force levitation pickup item

"count"     level of force power this holocron gives activator ( range: 0-3, default 1)
*/

/*QUAKED holocron_force_speed (.3 .5 1) (-8 -8 -0) (8 8 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID
force speed pickup item

"count"     level of force power this holocron gives activator ( range: 0-3, default 1)
*/

/*QUAKED holocron_force_push (.3 .5 1) (-8 -8 -0) (8 8 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID
force push pickup item

"count"     level of force power this holocron gives activator ( range: 0-3, default 1)
*/

/*QUAKED holocron_force_pull (.3 .5 1) (-8 -8 -0) (8 8 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID
force pull pickup item

"count"     level of force power this holocron gives activator ( range: 0-3, default 1)
*/

/*QUAKED holocron_force_telepathy (.3 .5 1) (-8 -8 -0) (8 8 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID
force telepathy pickup item

"count"     level of force power this holocron gives activator ( range: 0-3, default 1)
*/

/*QUAKED holocron_force_grip (.3 .5 1) (-8 -8 -0) (8 8 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID
force grip pickup item

"count"     level of force power this holocron gives activator ( range: 0-3, default 1)
*/

/*QUAKED holocron_force_lightining (.3 .5 1) (-8 -8 -0) (8 8 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID
force lighting pickup item

"count"     level of force power this holocron gives activator ( range: 0-3, default 1)
*/

/*QUAKED holocron_force_saberthrow (.3 .5 1) (-8 -8 -0) (8 8 16) SUSPEND NOPLAYER ALLOWNPC NOTSOLID
force saberthrow pickup item

"count"     level of force power this holocron gives activator ( range: 0-3, default 1)
*/

gitem_t	bg_itemlist[ITM_NUM_ITEMS+1];//need a null on the end

//int		bg_numItems = sizeof(bg_itemlist) / sizeof(bg_itemlist[0]) ;
const int		bg_numItems = ITM_NUM_ITEMS;



/*
===============
FindItemForWeapon

===============
*/
gitem_t	*FindItemForWeapon( weapon_t weapon ) {
	int		i;

	for ( i = 1 ; i < bg_numItems ; i++ ) {
		if ( bg_itemlist[i].giType == IT_WEAPON && bg_itemlist[i].giTag == weapon ) {
			return &bg_itemlist[i];
		}
	}

	Com_Error( ERR_DROP, "Couldn't find item for weapon %i", weapon);
	return NULL;
}

//----------------------------------------------
gitem_t	*FindItemForInventory( int inv )
{
	int		i;
	gitem_t	*it;

	// Now just check for any other kind of item.
	for ( i = 1 ; i < bg_numItems ; i++ )
	{
		it = &bg_itemlist[i];

		if ( it->giType == IT_HOLDABLE )
		{
			if ( it->giTag == inv )
			{
				return it;
			}
		}
	}

	Com_Error( ERR_DROP, "Couldn't find item for inventory %i", inv );
	return NULL;
}

/*
===============
FindItemForWeapon

===============
*/
gitem_t	*FindItemForAmmo( ammo_t ammo )
{
	int		i;

	for ( i = 1 ; i < bg_numItems ; i++ )
	{
		if ( bg_itemlist[i].giType == IT_AMMO && bg_itemlist[i].giTag == ammo )
		{
			return &bg_itemlist[i];
		}
	}

	Com_Error( ERR_DROP, "Couldn't find item for ammo %i", ammo );
	return NULL;
}

/*
===============
FindItem

===============
*/
gitem_t	*FindItem( const char *className ) {
	int		i;

	for ( i = 1 ; i < bg_numItems ; i++ ) {
		if ( !Q_stricmp( bg_itemlist[i].classname, className ) )
			return &bg_itemlist[i];
	}

	return NULL;
}


/*
================
BG_CanItemBeGrabbed

Returns false if the item should not be picked up.
This needs to be the same for client side prediction and server use.
================
*/
qboolean	BG_CanItemBeGrabbed( const entityState_t *ent, const playerState_t *ps ) {
	gitem_t	*item;

	if ( ent->modelindex < 1 || ent->modelindex >= bg_numItems ) {
		Com_Error( ERR_DROP, "BG_CanItemBeGrabbed: index out of range" );
	}

	item = &bg_itemlist[ent->modelindex];

	switch( item->giType ) {

	case IT_WEAPON:
		// See if we already have this weapon.
		if ( !(ps->stats[ STAT_WEAPONS ] & ( 1 << item->giTag )))
		{
			// Don't have this weapon yet, so pick it up.
			return qtrue;
		}
		else if ( item->giTag == WP_SABER )
		{//always pick up a saber, might be a new one?
			return qtrue;
		}

		// Make sure that we aren't already full on ammo for this weapon
		if ( ps->ammo[weaponData[item->giTag].ammoIndex] >= ammoData[weaponData[item->giTag].ammoIndex].max )
		{
			// full, so don't grab the item
			return qfalse;
		}

		return qtrue; // could use more of this type of ammo, so grab the item

	case IT_AMMO:

		if (item->giTag != AMMO_FORCE)
		{
			// since the ammo is the weapon in this case, picking up ammo should actually give you the weapon
			switch( item->giTag )
			{
			case AMMO_THERMAL:
				if( !(ps->stats[STAT_WEAPONS] & ( 1 << WP_THERMAL ) ) )
				{
					return qtrue;
				}
				break;
			case AMMO_DETPACK:
				if( !(ps->stats[STAT_WEAPONS] & ( 1 << WP_DET_PACK ) ) )
				{
					return qtrue;
				}
				break;
			case AMMO_TRIPMINE:
				if( !(ps->stats[STAT_WEAPONS] & ( 1 << WP_TRIP_MINE ) ) )
				{
					return qtrue;
				}
				break;
			}

			if ( ps->ammo[ item->giTag ] >= ammoData[item->giTag].max )	// checkme
			{
				return qfalse;		// can't hold any more
			}
		}
		else
		{
			if (ps->forcePower >= ammoData[item->giTag].max*2)
			{
				return qfalse;		// can't hold any more
			}

		}

		return qtrue;

	case IT_ARMOR:
		// we also clamp armor to the maxhealth for handicapping
		if ( ps->stats[STAT_ARMOR] >= ps->stats[STAT_MAX_HEALTH] ) {
			return qfalse;
		}
		return qtrue;

	case IT_HEALTH:
		if ((ps->forcePowersActive & (1 << FP_RAGE)))
		{//ragers can't use health
			return qfalse;
		}
		// don't pick up if already at max
		if ( ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH] ) {
			return qfalse;
		}
		return qtrue;

	case IT_BATTERY:
		// don't pick up if already at max
		if ( ps->batteryCharge >= MAX_BATTERIES )
		{
			return qfalse;
		}
		return qtrue;

	case IT_HOLOCRON:
		// pretty lame but for now you can always pick these up
		return qtrue;


	case IT_HOLDABLE:
		if ( item->giTag >= INV_ELECTROBINOCULARS && item->giTag <= INV_SENTRY )
		{
			// hardcoded--can only pick up five of any holdable
			if ( ps->inventory[item->giTag] >= 5 )
			{
				return qfalse;
			}
		}
		return qtrue;

	default:
		break;
	}

	return qfalse;
}

//======================================================================

/*
================
EvaluateTrajectory

================
*/
void EvaluateTrajectory( const trajectory_t *tr, int atTime, vec3_t result ) {
	float		deltaTime;
	float		phase;

	switch( tr->trType ) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		VectorCopy( tr->trBase, result );
		break;
	case TR_LINEAR:
		deltaTime = ( atTime - tr->trTime ) * 0.001F;	// milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		break;
	case TR_SINE:
		deltaTime = ( atTime - tr->trTime ) / (float) tr->trDuration;
		phase = (float)sin( deltaTime * M_PI * 2 );
		VectorMA( tr->trBase, phase, tr->trDelta, result );
		break;
	case TR_LINEAR_STOP:
		if ( atTime > tr->trTime + tr->trDuration )
		{
			atTime = tr->trTime + tr->trDuration;
		}
		//old totally linear
		deltaTime = ( atTime - tr->trTime ) * 0.001F;	// milliseconds to seconds
		if ( deltaTime < 0 )
		{//going past the total duration
			deltaTime = 0;
		}
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		break;
	case TR_NONLINEAR_STOP:
		if ( atTime > tr->trTime + tr->trDuration )
		{
			atTime = tr->trTime + tr->trDuration;
		}
		//new slow-down at end
		if ( atTime - tr->trTime > tr->trDuration || atTime - tr->trTime <= 0  )
		{
			deltaTime = 0;
		}
		else
		{//FIXME: maybe scale this somehow?  So that it starts out faster and stops faster?
			deltaTime = tr->trDuration*0.001f*((float)cos( DEG2RAD(90.0f - (90.0f*((float)atTime-tr->trTime)/(float)tr->trDuration)) ));
		}
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		break;
	case TR_GRAVITY:
		deltaTime = ( atTime - tr->trTime ) * 0.001F;	// milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		result[2] -= 0.5F * g_gravity->value * deltaTime * deltaTime;//DEFAULT_GRAVITY
		break;
	default:
		Com_Error( ERR_DROP, "EvaluateTrajectory: unknown trType: %i", tr->trTime );
		break;
	}
}

/*
================
EvaluateTrajectoryDelta

Returns current speed at given time
================
*/
void EvaluateTrajectoryDelta( const trajectory_t *tr, int atTime, vec3_t result ) {
	float	deltaTime;
	float	phase;

	switch( tr->trType ) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		VectorClear( result );
		break;
	case TR_LINEAR:
		VectorCopy( tr->trDelta, result );
		break;
	case TR_SINE:
		deltaTime = ( atTime - tr->trTime ) / (float) tr->trDuration;
		phase = (float)cos( deltaTime * M_PI * 2 );	// derivative of sin = cos
		phase *= 0.5;
		VectorScale( tr->trDelta, phase, result );
		break;
	case TR_LINEAR_STOP:
		if ( atTime > tr->trTime + tr->trDuration )
		{
			VectorClear( result );
			return;
		}
		VectorCopy( tr->trDelta, result );
		break;
	case TR_NONLINEAR_STOP:
		if ( atTime - tr->trTime > tr->trDuration || atTime - tr->trTime <= 0  )
		{
			VectorClear( result );
			return;
		}
		deltaTime = tr->trDuration*0.001f*((float)cos( DEG2RAD(90.0f - (90.0f*((float)atTime-tr->trTime)/(float)tr->trDuration)) ));
		VectorScale( tr->trDelta, deltaTime, result );
		break;
	case TR_GRAVITY:
		deltaTime = ( atTime - tr->trTime ) * 0.001F;	// milliseconds to seconds
		VectorCopy( tr->trDelta, result );
		result[2] -= g_gravity->value * deltaTime;		// DEFAULT_GRAVITY
		break;
	default:
		Com_Error( ERR_DROP, "EvaluateTrajectoryDelta: unknown trType: %i", tr->trTime );
		break;
	}
}

/*
===============
AddEventToPlayerstate

Handles the sequence numbers
===============
*/
void AddEventToPlayerstate( int newEvent, int eventParm, playerState_t *ps ) {
	ps->events[ps->eventSequence & (MAX_PS_EVENTS-1)] = newEvent;
	ps->eventParms[ps->eventSequence & (MAX_PS_EVENTS-1)] = eventParm;
	ps->eventSequence++;
}


/*
===============
CurrentPlayerstateEvent

===============
*/
int	CurrentPlayerstateEvent( playerState_t *ps ) {
	return ps->events[ (ps->eventSequence-1) & (MAX_PS_EVENTS-1) ];
}

/*
========================
PlayerStateToEntityState

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void PlayerStateToEntityState( playerState_t *ps, entityState_t *s )
{
	int		i;

	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR )
	{
		s->eType = ET_INVISIBLE;
	}
	/*else if ( ps->stats[STAT_HEALTH] <= GIB_HEALTH )
	{
		s->eType = ET_INVISIBLE;
	} */
	else
	{
		s->eType = ET_PLAYER;
	}

	s->number = ps->clientNum;

	s->pos.trType = TR_INTERPOLATE;
	VectorCopy( ps->origin, s->pos.trBase );
	//SnapVector( s->pos.trBase );

	s->apos.trType = TR_INTERPOLATE;
	VectorCopy( ps->viewangles, s->apos.trBase );
	//SnapVector( s->apos.trBase );

	s->angles2[YAW] = ps->movementDir;
	s->legsAnim = ps->legsAnim;
	s->torsoAnim = ps->torsoAnim;
	s->clientNum = ps->clientNum;		// ET_PLAYER looks here instead of at number
										// so corpses can also reference the proper config
	s->eFlags = ps->eFlags;

	// new sabre stuff
	s->saberActive = ps->SaberActive();//WHY is this on the entityState_t, too???
	s->saberInFlight = ps->saberInFlight;

	// NOTE: Although we store this stuff locally on a vehicle, who's to say we
	// can't bring back these variables and fill them at the appropriate time? -Aurelio
	// We need to bring these in from the vehicle NPC.
	if ( g_entities[ps->clientNum].client && g_entities[ps->clientNum].client->NPC_class == CLASS_VEHICLE && g_entities[ps->clientNum].NPC )
	{
		Vehicle_t *pVeh = g_entities[ps->clientNum].m_pVehicle;
		s->vehicleArmor = pVeh->m_iArmor;
		VectorCopy( pVeh->m_vOrientation, s->vehicleAngles );
	}

	s->weapon = ps->weapon;
	s->groundEntityNum = ps->groundEntityNum;

	s->powerups = 0;
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( ps->powerups[ i ] ) {
			s->powerups |= 1 << i;
		}
	}
#if 0
	if ( ps->externalEvent ) {
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	} else {
		int		seq;

		seq = (ps->eventSequence-1) & (MAX_PS_EVENTS-1);
		s->event = ps->events[ seq ] | ( ( ps->eventSequence & 3 ) << 8 );
		s->eventParm = ps->eventParms[ seq ];
	}

	// show some roll in the body based on velocity and angle
	if ( ps->stats[STAT_HEALTH] > 0 ) {
		vec3_t		right;
		float		sign;
		float		side;
		float		value;

		AngleVectors( ps->viewangles, NULL, right, NULL );

		side = DotProduct (ps->velocity, right);
		sign = side < 0 ? -1 : 1;
		side = fabs(side);

		value = 2;	// g_rollangle->value;

		if (side < 200 /* g_rollspeed->value */ )
			side = side * value / 200; // g_rollspeed->value;
		else
			side = value;

		s->angles[ROLL] = (int)(side*sign * 4);
	}
#endif
}


/*
============
BG_PlayerTouchesItem

Items can be picked up without actually touching their physical bounds
============
*/
qboolean	BG_PlayerTouchesItem( playerState_t *ps, entityState_t *item, int atTime ) {
	vec3_t origin = { 0.0f };

	EvaluateTrajectory( &item->pos, atTime, origin );

	// we are ignoring ducked differences here
	if ( ps->origin[0] - origin[0] > 44
		|| ps->origin[0] - origin[0] < -50
		|| ps->origin[1] - origin[1] > 36
		|| ps->origin[1] - origin[1] < -36
		|| ps->origin[2] - origin[2] > 36
		|| ps->origin[2] - origin[2] < -36 ) {
		return qfalse;
	}

	return qtrue;
}


/*
=================
BG_EmplacedView

Shared code for emplaced angle gun constriction
=================
*/
int BG_EmplacedView(vec3_t baseAngles, vec3_t angles, float *newYaw, float constraint)
{
	float dif = AngleSubtract(baseAngles[YAW], angles[YAW]);

	if (dif > constraint ||
		dif < -constraint)
	{
		float amt;

		if (dif > constraint)
		{
			amt = (dif-constraint);
			dif = constraint;
		}
		else if (dif < -constraint)
		{
			amt = (dif+constraint);
			dif = -constraint;
		}
		else
		{
			amt = 0.0f;
		}

		*newYaw = AngleSubtract(angles[YAW], -dif);

		if (amt > 1.0f || amt < -1.0f)
		{ //significant, force the view
			return 2;
		}
		else
		{ //just a little out of range
			return 1;
		}
	}

	return 0;
}
