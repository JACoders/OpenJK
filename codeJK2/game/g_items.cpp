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

// leave this line at the top for all g_xxxx.cpp files...
#include "g_headers.h"

#include "g_local.h"
#include "g_functions.h"
#include "g_items.h"
#include "wp_saber.h"

extern qboolean	missionInfo_Updated;

extern void CrystalAmmoSettings(gentity_t *ent);
extern void G_CreateG2AttachedWeaponModel( gentity_t *ent, const char *weaponModel );
extern void ChangeWeapon( gentity_t *ent, int newWeapon );
extern void G_SoundOnEnt( gentity_t *ent, soundChannel_t channel, const char *soundPath );
extern qboolean PM_InKnockDown( playerState_t *ps );
extern qboolean PM_InGetUp( playerState_t *ps );

extern	cvar_t	*g_spskill;

#define MAX_BACTA_HEAL_AMOUNT		25

/*

  Items are any object that a player can touch to gain some effect.

  Pickup will return the number of seconds until they should respawn.

  all items should pop when dropped in lava or slime

  Respawnable items don't actually go away when picked up, they are
  just made invisible and untouchable.  This allows them to ride
  movers and respawn apropriately.
*/

// Item Spawn flags
#define ITMSF_SUSPEND		1
#define ITMSF_TEAM			2
#define ITMSF_MONSTER		4
#define ITMSF_NOTSOLID		8
#define ITMSF_VERTICAL		16
#define ITMSF_INVISIBLE		32

//======================================================================

/*
===============
G_InventorySelectable
===============
*/
qboolean G_InventorySelectable( int index,gentity_t *other)
{
	if (other->client->ps.inventory[index])
	{
		return qtrue;
	}

	return qfalse;
}

extern qboolean INV_GoodieKeyGive( gentity_t *target );
extern qboolean INV_SecurityKeyGive( gentity_t *target, const char *keyname );
int Pickup_Holdable( gentity_t *ent, gentity_t *other )
{
	int		i,original;

	other->client->ps.stats[STAT_ITEMS] |= (1<<ent->item->giTag);

	if ( ent->item->giTag == INV_SECURITY_KEY )
	{//give the key
		//FIXME: temp message
		gi.SendServerCommand( 0, "cp @INGAME_YOU_TOOK_SECURITY_KEY" );
		INV_SecurityKeyGive( other, ent->message );
	}
	else if ( ent->item->giTag == INV_GOODIE_KEY )
	{//give the key
		//FIXME: temp message
		gi.SendServerCommand( 0, "cp @INGAME_YOU_TOOK_SUPPLY_KEY" );
		INV_GoodieKeyGive( other );
	}
	else
	{// Picking up a normal item?
		other->client->ps.inventory[ent->item->giTag]++;
	}
	// Got a security key

	// Set the inventory select, just in case it hasn't
	original = cg.inventorySelect;
	for ( i = 0 ; i < INV_MAX ; i++ )
	{
		if ((cg.inventorySelect < INV_ELECTROBINOCULARS) || (cg.inventorySelect >= INV_MAX))
		{
			cg.inventorySelect = (INV_MAX - 1);
		}

		if ( G_InventorySelectable( cg.inventorySelect,other ) )
		{
			return 60;
		}
		cg.inventorySelect++;
	}

	cg.inventorySelect = original;

	return 60;
}


//======================================================================
int Add_Ammo2 (gentity_t *ent, int ammoType, int count)
{

	if (ammoType != AMMO_FORCE)
	{
		ent->client->ps.ammo[ammoType] += count;

		// since the ammo is the weapon in this case, picking up ammo should actually give you the weapon
		switch( ammoType )
		{
		case AMMO_THERMAL:
			ent->client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_THERMAL );
			break;
		case AMMO_DETPACK:
			ent->client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_DET_PACK );
			break;
		case AMMO_TRIPMINE:
			ent->client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_TRIP_MINE );
			break;
		}

		if ( ent->client->ps.ammo[ammoType] > ammoData[ammoType].max )
		{
			ent->client->ps.ammo[ammoType] = ammoData[ammoType].max;
			return qfalse;
		}
	}
	else
	{
		if ( ent->client->ps.forcePower >= ammoData[ammoType].max )
		{//if have full force, just get 25 extra per crystal
			ent->client->ps.forcePower += 25;
		}
		else
		{//else if don't have full charge, give full amount, up to max + 25
			ent->client->ps.forcePower += count;
			if ( ent->client->ps.forcePower >= ammoData[ammoType].max + 25 )
			{//cap at max + 25
				ent->client->ps.forcePower = ammoData[ammoType].max + 25;
			}
		}

		if ( ent->client->ps.forcePower >= ammoData[ammoType].max*2 )
		{//always cap at twice a full charge
			ent->client->ps.forcePower = ammoData[ammoType].max*2;
			return qfalse;		// can't hold any more
		}
	}
	return qtrue;
}

//-------------------------------------------------------
void Add_Ammo (gentity_t *ent, int weapon, int count)
{
	Add_Ammo2(ent,weaponData[weapon].ammoIndex,count);
}

//-------------------------------------------------------
int Pickup_Ammo (gentity_t *ent, gentity_t *other)
{
	int		quantity;

	if ( ent->count ) {
		quantity = ent->count;
	} else {
		quantity = ent->item->quantity;
	}

	Add_Ammo2 (other, ent->item->giTag, quantity);

	return 30;
}

//======================================================================
void Add_Batteries( gentity_t *ent, int *count )
{
	if ( ent->client && ent->client->ps.batteryCharge < MAX_BATTERIES && *count )
	{
		if ( *count + ent->client->ps.batteryCharge > MAX_BATTERIES )
		{
			// steal what we need, then leave the rest for later
			*count -= ( MAX_BATTERIES - ent->client->ps.batteryCharge );
			ent->client->ps.batteryCharge = MAX_BATTERIES;
		}
		else
		{
			// just drain all of the batteries
			ent->client->ps.batteryCharge += *count;
			*count = 0;
		}

		G_AddEvent( ent, EV_BATTERIES_CHARGED, 0 );
	}
}

//-------------------------------------------------------
int Pickup_Battery( gentity_t *ent, gentity_t *other )
{
	int	quantity;

	if ( ent->count )
	{
		quantity = ent->count;
	}
	else
	{
		quantity = ent->item->quantity;
	}

	// There may be some left over in quantity if the player is close to full, but with pickup items, this amount will just be lost
	Add_Batteries( other, &quantity );

	return 30;
}

//======================================================================


extern void WP_SaberInitBladeData( gentity_t *ent );
extern void CG_ChangeWeapon( int num );
int Pickup_Weapon (gentity_t *ent, gentity_t *other)
{
	int		quantity;
	qboolean	hadWeapon = qfalse;

	/*
	if ( ent->count || (ent->activator && !ent->activator->s.number) )
	{
		quantity = ent->count;
	}
	else
	{
		quantity = ent->item->quantity;
	}
	*/

	// dropped items are always picked up
	if ( ent->flags & FL_DROPPED_ITEM )
	{
		quantity = ent->count;
	}
	else
	{//wasn't dropped
		quantity = ent->item->quantity?ent->item->quantity:50;
	}

	// add the weapon
	if ( other->client->ps.stats[STAT_WEAPONS] & ( 1 << ent->item->giTag ) )
	{
		hadWeapon = qtrue;
	}
	other->client->ps.stats[STAT_WEAPONS] |= ( 1 << ent->item->giTag );

	if ( ent->item->giTag == WP_SABER && !hadWeapon )
	{
		WP_SaberInitBladeData( other );
	}

	if ( other->s.number )
	{//NPC
		if ( other->s.weapon == WP_NONE )
		{//NPC with no weapon picked up a weapon, change to this weapon
			//FIXME: clear/set the alt-fire flag based on the picked up weapon and my class?
			other->client->ps.weapon = ent->item->giTag;
			other->client->ps.weaponstate = WEAPON_RAISING;
			ChangeWeapon( other, ent->item->giTag );
			if ( ent->item->giTag == WP_SABER )
			{
				other->client->ps.saberActive = qtrue;
				G_CreateG2AttachedWeaponModel( other, other->client->ps.saberModel );
			}
			else
			{
				G_CreateG2AttachedWeaponModel( other, weaponData[ent->item->giTag].weaponMdl );
			}
		}
	}

	if ( quantity )
	{
		// Give ammo
		Add_Ammo( other, ent->item->giTag, quantity );
	}
	return 5;
}


//======================================================================

int ITM_AddHealth (gentity_t *ent, int count)
{

	ent->health += count;

	if (ent->health > ent->client->ps.stats[STAT_MAX_HEALTH])	// Past max health
	{
		ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];

		return qfalse;
	}

	return qtrue;

}

int Pickup_Health (gentity_t *ent, gentity_t *other) {
	int			max;
	int			quantity;

	max = other->client->ps.stats[STAT_MAX_HEALTH];

	if ( ent->count ) {
		quantity = ent->count;
	} else {
		quantity = ent->item->quantity;
	}

	other->health += quantity;

	if (other->health > max ) {
		other->health = max;
	}

	if ( ent->item->giTag == 100 ) {		// mega health respawns slow
		return 120;
	}

	return 30;
}

//======================================================================

int ITM_AddArmor (gentity_t *ent, int count)
{

	ent->client->ps.stats[STAT_ARMOR] += count;

	if (ent->client->ps.stats[STAT_ARMOR] > ent->client->ps.stats[STAT_MAX_HEALTH])
	{
		ent->client->ps.stats[STAT_ARMOR] = ent->client->ps.stats[STAT_MAX_HEALTH];
		return qfalse;
	}

	return qtrue;
}


int Pickup_Armor( gentity_t *ent, gentity_t *other ) {

	// make sure that the shield effect is on
	other->client->ps.powerups[PW_BATTLESUIT] = Q3_INFINITE;

	other->client->ps.stats[STAT_ARMOR] += ent->item->quantity;
	if ( other->client->ps.stats[STAT_ARMOR] > other->client->ps.stats[STAT_MAX_HEALTH] ) {
		other->client->ps.stats[STAT_ARMOR] = other->client->ps.stats[STAT_MAX_HEALTH];
	}

	return 30;
}



//======================================================================

int Pickup_Holocron( gentity_t *ent, gentity_t *other )
{
	int forcePower = ent->item->giTag;
	int forceLevel = ent->count;
	// check if out of range
	if( forceLevel < 0 || forceLevel >= NUM_FORCE_POWER_LEVELS )
	{
		gi.Printf(" Pickup_Holocron : count %d not in valid range\n", forceLevel );
		return 1;
	}

	// don't pick up if already known AND your level is higher than pickup level
	if ( ( other->client->ps.forcePowersKnown & ( 1 << forcePower )) )
	{
		//don't pickup if item is lower than current level
		if( other->client->ps.forcePowerLevel[forcePower] >= forceLevel )
		{
			return 1;
		}
	}

	other->client->ps.forcePowerLevel[forcePower] = forceLevel;
	other->client->ps.forcePowersKnown |= ( 1 << forcePower );

	missionInfo_Updated = qtrue;	// Activate flashing text
	gi.cvar_set("cg_updatedDataPadForcePower1", va("%d",forcePower+1)); // The +1 is offset in the print routine.
	cg_updatedDataPadForcePower1.integer = forcePower+1;
	gi.cvar_set("cg_updatedDataPadForcePower2", "0"); // The +1 is offset in the print routine.
	cg_updatedDataPadForcePower2.integer = 0;
	gi.cvar_set("cg_updatedDataPadForcePower3", "0"); // The +1 is offset in the print routine.
	cg_updatedDataPadForcePower3.integer = 0;

	return 1;
}


//======================================================================

/*
===============
RespawnItem
===============
*/
void RespawnItem( gentity_t *ent ) {
}


qboolean CheckItemCanBePickedUpByNPC( gentity_t *item, gentity_t *pickerupper )
{
	if ( !item->item ) {
		return qfalse;
	}
	if ( item->item->giType == IT_HOLDABLE &&
		item->item->giTag == INV_SECURITY_KEY ) {
		return qfalse;
	}
	if ( (item->flags&FL_DROPPED_ITEM)
		&& item->activator != &g_entities[0]
		&& pickerupper->s.number
		&& pickerupper->s.weapon == WP_NONE
		&& pickerupper->enemy
		&& pickerupper->painDebounceTime < level.time
		&& pickerupper->NPC && pickerupper->NPC->surrenderTime < level.time //not surrendering
		&& !(pickerupper->NPC->scriptFlags&SCF_FORCED_MARCH) ) // not being forced to march
	{//non-player, in combat, picking up a dropped item that does NOT belong to the player and it *not* a security key
		if ( level.time - item->s.time < 3000 )//was 5000
		{
			return qfalse;
		}
		return qtrue;
	}
	return qfalse;
}
/*
===============
Touch_Item
===============
*/
extern cvar_t		*g_timescale;
void Touch_Item (gentity_t *ent, gentity_t *other, trace_t *trace) {
	int			respawn = 0;

	if (!other->client)
		return;
	if (other->health < 1)
		return;		// dead people can't pickup

	if ( other->client->ps.pm_time > 0 )
	{//cant pick up when out of control
		return;
	}

	// Only monsters can pick it up
	if ((ent->spawnflags &  ITMSF_MONSTER) && (other->client->playerTeam == TEAM_PLAYER))
	{
		return;
	}

	// Only starfleet can pick it up
	if ((ent->spawnflags &  ITMSF_TEAM) && (other->client->playerTeam != TEAM_PLAYER))
	{
		return;
	}


	if ( other->client->NPC_class == CLASS_ATST ||
		other->client->NPC_class == CLASS_GONK ||
		other->client->NPC_class == CLASS_MARK1 ||
		other->client->NPC_class == CLASS_MARK2 ||
		other->client->NPC_class == CLASS_MOUSE ||
		other->client->NPC_class == CLASS_PROBE ||
		other->client->NPC_class == CLASS_PROTOCOL ||
		other->client->NPC_class == CLASS_R2D2 ||
		other->client->NPC_class == CLASS_R5D2 ||
		other->client->NPC_class == CLASS_SEEKER ||
		other->client->NPC_class == CLASS_REMOTE ||
		other->client->NPC_class == CLASS_SENTRY )
	{//FIXME: some flag would be better
		//droids can't pick up items/weapons!
		return;
	}

	//FIXME: need to make them run toward a dropped weapon when fleeing without one?
	//FIXME: need to make them come out of flee mode when pick up their old weapon?
	if ( CheckItemCanBePickedUpByNPC( ent, other ) )
	{
		if ( other->NPC && other->NPC->goalEntity && other->NPC->goalEntity->enemy == ent )
		{//they were running to pick me up, they did, so clear goal
			other->NPC->goalEntity = NULL;
			other->NPC->squadState = SQUAD_STAND_AND_SHOOT;
		}
	}
	else if (!(ent->spawnflags &  ITMSF_TEAM) && !(ent->spawnflags &  ITMSF_MONSTER))
	{// Only player can pick it up
		if ( other->s.number != 0 )	// Not the player?
		{
			return;
		}
	}

	// the same pickup rules are used for client side and server side
	if ( !BG_CanItemBeGrabbed( &ent->s, &other->client->ps ) ) {
		return;
	}

	if ( other->client )
	{
		if ( other->client->ps.eFlags&EF_FORCE_GRIPPED )
		{//can't pick up anything while being gripped
			return;
		}
		if ( PM_InKnockDown( &other->client->ps ) && !PM_InGetUp( &other->client->ps ) )
		{//can't pick up while in a knockdown
			return;
		}
	}
	if (!ent->item) {		//not an item!
		gi.Printf( "Touch_Item: %s is not an item!\n", ent->classname);
		return;
	}
	qboolean bHadWeapon = qfalse;
	// call the item-specific pickup function
	switch( ent->item->giType )
	{
	case IT_WEAPON:
		if ( other->NPC && other->s.weapon == WP_NONE )
		{//Make them duck and sit here for a few seconds
			int pickUpTime = Q_irand( 1000, 3000 );
			TIMER_Set( other, "duck", pickUpTime );
			TIMER_Set( other, "roamTime", pickUpTime );
			TIMER_Set( other, "stick", pickUpTime );
			TIMER_Set( other, "verifyCP", pickUpTime );
			TIMER_Set( other, "attackDelay", 600 );
			respawn = 0;
		}
		if ( other->client->ps.stats[STAT_WEAPONS] & ( 1 << ent->item->giTag ) )
		{
			bHadWeapon = qtrue;
		}
		respawn = Pickup_Weapon(ent, other);
		break;
	case IT_AMMO:
		respawn = Pickup_Ammo(ent, other);
		break;
	case IT_ARMOR:
		respawn = Pickup_Armor(ent, other);
		break;
	case IT_HEALTH:
		respawn = Pickup_Health(ent, other);
		break;
	case IT_HOLDABLE:
		respawn = Pickup_Holdable(ent, other);
		break;
	case IT_BATTERY:
		respawn = Pickup_Battery( ent, other );
		break;
	case IT_HOLOCRON:
		respawn = Pickup_Holocron( ent, other );
		break;
	default:
		return;
	}

	if ( !respawn )
	{
		return;
	}

	// play the normal pickup sound
	if ( !other->s.number && g_timescale->value < 1.0f  )
	{//SIGH... with timescale on, you lose events left and right
extern void CG_ItemPickup( int itemNum, qboolean bHadItem );
		// but we're SP so we'll cheat
		cgi_S_StartSound( NULL, other->s.number, CHAN_AUTO,	cgi_S_RegisterSound( ent->item->pickup_sound ) );
		// show icon and name on status bar
		CG_ItemPickup( ent->s.modelindex, bHadWeapon );
	}
	else
	{
		if ( bHadWeapon )
		{
			G_AddEvent( other, EV_ITEM_PICKUP, -ent->s.modelindex );
		}
		else
		{
			G_AddEvent( other, EV_ITEM_PICKUP, ent->s.modelindex );
		}
	}

	// fire item targets
	G_UseTargets (ent, other);

	// wait of -1 will not respawn
//	if ( ent->wait == -1 )
	{
		//why not just remove me?
		G_FreeEntity( ent );
		/*
		//NOTE: used to do this:  (for respawning?)
		ent->svFlags |= SVF_NOCLIENT;
		ent->s.eFlags |= EF_NODRAW;
		ent->contents = 0;
		ent->unlinkAfterEvent = qtrue;
		*/
		return;
	}
}


//======================================================================

/*
================
LaunchItem

Spawns an item and tosses it forward
================
*/
gentity_t *LaunchItem( gitem_t *item, vec3_t origin, vec3_t velocity, char *target ) {
	gentity_t	*dropped;

	dropped = G_Spawn();

	dropped->s.eType = ET_ITEM;
	dropped->s.modelindex = item - bg_itemlist;	// store item number in modelindex
	dropped->s.modelindex2 = 1; // This is non-zero is it's a dropped item

	dropped->classname = item->classname;
	dropped->item = item;

	// try using the "correct" mins/maxs first
	VectorSet( dropped->mins, item->mins[0], item->mins[1], item->mins[2] );
	VectorSet( dropped->maxs, item->maxs[0], item->maxs[1], item->maxs[2] );

	if ((!dropped->mins[0] && !dropped->mins[1] && !dropped->mins[2]) &&
		(!dropped->maxs[0] && !dropped->maxs[1] && !dropped->maxs[2]))
	{
		VectorSet( dropped->maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS );
		VectorScale( dropped->maxs, -1, dropped->mins );
	}

	dropped->contents = CONTENTS_TRIGGER|CONTENTS_ITEM;//CONTENTS_TRIGGER;//not CONTENTS_BODY for dropped items, don't need to ID them

	if ( target && target[0] )
	{
		dropped->target = G_NewString( target );
	}
	else
	{
		// if not targeting something, auto-remove after 30 seconds
		// only if it's NOT a security or goodie key
		if (dropped->item->giTag != INV_SECURITY_KEY )
		{
			dropped->e_ThinkFunc = thinkF_G_FreeEntity;
			dropped->nextthink = level.time + 30000;
		}

		if ( dropped->item->giType == IT_AMMO && dropped->item->giTag == AMMO_FORCE )
		{
			dropped->nextthink = -1;
			dropped->e_ThinkFunc = thinkF_NULL;
		}
	}

	dropped->e_TouchFunc = touchF_Touch_Item;

	if ( item->giType == IT_WEAPON )
	{
		// give weapon items zero pitch, a random yaw, and rolled onto their sides...but would be bad to do this for a bowcaster
		if ( item->giTag != WP_BOWCASTER
			&& item->giTag != WP_THERMAL
			&& item->giTag != WP_TRIP_MINE
			&& item->giTag != WP_DET_PACK )
		{
			VectorSet( dropped->s.angles, 0, crandom() * 180, 90.0f );
			G_SetAngles( dropped, dropped->s.angles );
		}
	}

	G_SetOrigin( dropped, origin );
	dropped->s.pos.trType = TR_GRAVITY;
	dropped->s.pos.trTime = level.time;
	VectorCopy( velocity, dropped->s.pos.trDelta );

	dropped->s.eFlags |= EF_BOUNCE_HALF;

	dropped->flags = FL_DROPPED_ITEM;

	gi.linkentity (dropped);

	return dropped;
}

/*
================
Drop_Item

Spawns an item and tosses it forward
================
*/
gentity_t *Drop_Item( gentity_t *ent, gitem_t *item, float angle, qboolean copytarget ) {
	gentity_t	*dropped = NULL;
	vec3_t	velocity;
	vec3_t	angles;

	VectorCopy( ent->s.apos.trBase, angles );
	angles[YAW] += angle;
	angles[PITCH] = 0;	// always forward

	AngleVectors( angles, velocity, NULL, NULL );
	VectorScale( velocity, 150, velocity );
	velocity[2] += 200 + crandom() * 50;

	if ( copytarget )
	{
		dropped = LaunchItem( item, ent->s.pos.trBase, velocity, ent->opentarget );
	}
	else
	{
		dropped = LaunchItem( item, ent->s.pos.trBase, velocity, NULL );
	}

	dropped->activator = ent;//so we know who we belonged to so they can pick it back up later
	dropped->s.time = level.time;//mark this time so we aren't picked up instantly by the guy who dropped us
	return dropped;
}


/*
================
Use_Item

Respawn the item
================
*/
void Use_Item( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	if ( (ent->svFlags&SVF_PLAYER_USABLE) && other && !other->s.number )
	{//used directly by the player, pick me up
		GEntity_TouchFunc( ent, other, NULL );
	}
	else
	{//use me
		if ( ent->spawnflags & 32 ) // invisible
		{
			// If it was invisible, first use makes it visible....
			ent->s.eFlags &= ~EF_NODRAW;
			ent->contents = CONTENTS_TRIGGER|CONTENTS_ITEM;

			ent->spawnflags &= ~32;
			return;
		}

		G_ActivateBehavior( ent, BSET_USE );
		RespawnItem( ent );
	}
}

//======================================================================

/*
================
FinishSpawningItem

Traces down to find where an item should rest, instead of letting them
free fall from their spawn points
================
*/
#ifndef FINAL_BUILD
extern int delayedShutDown;
#endif
void FinishSpawningItem( gentity_t *ent ) {
	trace_t		tr;
	vec3_t		dest;
	gitem_t		*item;
	int			itemNum;

	itemNum=1;
	for ( item = bg_itemlist + 1 ; item->classname ; item++,itemNum++)
	{
		if (!strcmp(item->classname,ent->classname))
		{
			break;
		}
	}

	// Set bounding box for item
	VectorSet( ent->mins, item->mins[0],item->mins[1] ,item->mins[2]);
	VectorSet( ent->maxs, item->maxs[0],item->maxs[1] ,item->maxs[2]);

	if ((!ent->mins[0] && !ent->mins[1] && !ent->mins[2]) &&
		(!ent->maxs[0] && !ent->maxs[1] && !ent->maxs[2]))
	{
		VectorSet (ent->mins, -ITEM_RADIUS, -ITEM_RADIUS, -2);//to match the comments in the items.dat file!
		VectorSet (ent->maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS);
	}

	if ((item->quantity) && (item->giType == IT_AMMO))
	{
		ent->count = item->quantity;
	}

	if ((item->quantity) && (item->giType == IT_BATTERY))
	{
		ent->count = item->quantity;
	}


//	if ( item->giType == IT_WEAPON ) // NOTE: james thought it was ok to just always do this?
	{
		ent->s.radius = 20;
		VectorSet( ent->s.modelScale, 1.0f, 1.0f, 1.0f );
		gi.G2API_InitGhoul2Model( ent->ghoul2, ent->item->world_model, G_ModelIndex( ent->item->world_model ), NULL_HANDLE, NULL_HANDLE, 0, 0);
	}

	// Set crystal ammo amount based on skill level
/*	if ((itemNum == ITM_AMMO_CRYSTAL_BORG) ||
		(itemNum == ITM_AMMO_CRYSTAL_DN) ||
		(itemNum == ITM_AMMO_CRYSTAL_FORGE) ||
		(itemNum == ITM_AMMO_CRYSTAL_SCAVENGER) ||
		(itemNum == ITM_AMMO_CRYSTAL_STASIS))
	{
		CrystalAmmoSettings(ent);
	}
*/
	ent->s.eType = ET_ITEM;
	ent->s.modelindex = ent->item - bg_itemlist;		// store item number in modelindex
	ent->s.modelindex2 = 0; // zero indicates this isn't a dropped item

	ent->contents = CONTENTS_TRIGGER|CONTENTS_ITEM;//CONTENTS_BODY;//CONTENTS_TRIGGER|
	ent->e_TouchFunc = touchF_Touch_Item;
	// useing an item causes it to respawn
	ent->e_UseFunc = useF_Use_Item;
	ent->svFlags |= SVF_PLAYER_USABLE;//so player can pick it up

	// Hang in air?
	ent->s.origin[2] += 1;//just to get it off the damn ground because coplanar = insolid
	if ( ent->spawnflags & ITMSF_SUSPEND)
	{
		// suspended
		G_SetOrigin( ent, ent->s.origin );
	}
	else
	{
		// drop to floor
		VectorSet( dest, ent->s.origin[0], ent->s.origin[1], MIN_WORLD_COORD );
		gi.trace( &tr, ent->s.origin, ent->mins, ent->maxs, dest, ent->s.number, MASK_SOLID|CONTENTS_PLAYERCLIP, G2_NOCOLLIDE, 0 );
		if ( tr.startsolid )
		{
			if ( &g_entities[tr.entityNum] != NULL )
			{
				gi.Printf (S_COLOR_RED"FinishSpawningItem: removing %s startsolid at %s (in a %s)\n", ent->classname, vtos(ent->s.origin), g_entities[tr.entityNum].classname );
			}
			else
			{
				gi.Printf (S_COLOR_RED"FinishSpawningItem: removing %s startsolid at %s (in a %s)\n", ent->classname, vtos(ent->s.origin) );
			}
			assert( 0 && "item starting in solid");
#ifndef FINAL_BUILD
			if (!g_entities[ENTITYNUM_WORLD].s.radius){	//not a region
				delayedShutDown = level.time + 100;
			}
#endif
			G_FreeEntity( ent );
			return;
		}

		// allow to ride movers
		ent->s.groundEntityNum = tr.entityNum;

		G_SetOrigin( ent, tr.endpos );
	}

/* ? don't need this
	// team slaves and targeted items aren't present at start
	if ( ( ent->flags & FL_TEAMSLAVE ) || ent->targetname ) {
		ent->s.eFlags |= EF_NODRAW;
		ent->contents = 0;
		return;
	}
*/
	if ( ent->spawnflags & ITMSF_INVISIBLE ) // invisible
	{
		ent->s.eFlags |= EF_NODRAW;
		ent->contents = 0;
	}

	if ( ent->spawnflags & ITMSF_NOTSOLID ) // not solid
	{
		ent->contents = 0;
	}

	gi.linkentity (ent);
}


char itemRegistered[MAX_ITEMS+1];


/*
==============
ClearRegisteredItems
==============
*/
void ClearRegisteredItems( void ) {
	for ( int i = 0; i < bg_numItems; i++ )
	{
		itemRegistered[i] = '0';
	}
	itemRegistered[ bg_numItems ] = 0;

	RegisterItem( FindItemForWeapon( WP_BRYAR_PISTOL ) );	//these are given in g_client, ClientSpawn(), but MUST be registered HERE, BEFORE cgame starts.
	RegisterItem( FindItemForWeapon( WP_STUN_BATON ) );			//these are given in g_client, ClientSpawn(), but MUST be registered HERE, BEFORE cgame starts.
	RegisterItem( FindItemForInventory( INV_ELECTROBINOCULARS ));
	// saber or baton is cached in SP_info_player_deathmatch now.

extern void Player_CacheFromPrevLevel(void);//g_client.cpp
	Player_CacheFromPrevLevel();	//reads from transition carry-over;
}

/*
===============
RegisterItem

The item will be added to the precache list
===============
*/
void RegisterItem( gitem_t *item ) {
	if ( !item ) {
		G_Error( "RegisterItem: NULL" );
	}
	itemRegistered[ item - bg_itemlist ] = '1';
	gi.SetConfigstring(CS_ITEMS, itemRegistered);	//Write the needed items to a config string
}


/*
===============
SaveRegisteredItems

Write the needed items to a config string
so the client will know which ones to precache
===============
*/
void SaveRegisteredItems( void ) {
/*	char	string[MAX_ITEMS+1];
	int		i;
	int		count;

	count = 0;
	for ( i = 0 ; i < bg_numItems ; i++ ) {
		if ( itemRegistered[i] ) {
			count++;
			string[i] = '1';
		} else {
			string[i] = '0';
		}
	}
	string[ bg_numItems ] = 0;

	gi.Printf( "%i items registered\n", count );
	gi.SetConfigstring(CS_ITEMS, string);
*/
	gi.SetConfigstring(CS_ITEMS, itemRegistered);
}

/*
============
item_spawn_use

 if an item is given a targetname, it will be spawned in when used
============
*/
void item_spawn_use( gentity_t *self, gentity_t *other, gentity_t *activator )
//-----------------------------------------------------------------------------
{
	self->nextthink = level.time + 50;
	self->e_ThinkFunc = thinkF_FinishSpawningItem;
	// I could be fancy and add a count or something like that to be able to spawn the item numerous times...
	self->e_UseFunc = useF_NULL;
}

/*
============
G_SpawnItem

Sets the clipping size and plants the object on the floor.

Items can't be immediately dropped to floor, because they might
be on an entity that hasn't spawned yet.
============
*/
void G_SpawnItem (gentity_t *ent, gitem_t *item) {
	G_SpawnFloat( "random", "0", &ent->random );
	G_SpawnFloat( "wait", "0", &ent->wait );

	RegisterItem( item );
	ent->item = item;

	// targetname indicates they want to spawn it later
	if( ent->targetname )
	{
		ent->e_UseFunc = useF_item_spawn_use;
	}
	else
	{	// some movers spawn on the second frame, so delay item
		// spawns until the third frame so they can ride trains
		ent->nextthink = level.time + START_TIME_MOVERS_SPAWNED + 50;
		ent->e_ThinkFunc = thinkF_FinishSpawningItem;
	}

	ent->physicsBounce = 0.50;		// items are bouncy

	// Set a default infoString text color
	// NOTE: if we want to do cool cross-hair colors for items, we can just modify this, but for now, don't do it
	VectorSet( ent->startRGBA, 1.0f, 1.0f, 1.0f );
}


/*
================
G_BounceItem

================
*/
void G_BounceItem( gentity_t *ent, trace_t *trace ) {
	vec3_t	velocity;
	float	dot;
	int		hitTime;

	// reflect the velocity on the trace plane
	hitTime = level.previousTime + ( level.time - level.previousTime ) * trace->fraction;
	EvaluateTrajectoryDelta( &ent->s.pos, hitTime, velocity );
	dot = DotProduct( velocity, trace->plane.normal );
	VectorMA( velocity, -2*dot, trace->plane.normal, ent->s.pos.trDelta );

	// cut the velocity to keep from bouncing forever
	VectorScale( ent->s.pos.trDelta, ent->physicsBounce, ent->s.pos.trDelta );

	// check for stop
	if ( trace->plane.normal[2] > 0 && ent->s.pos.trDelta[2] < 40 ) {
		G_SetOrigin( ent, trace->endpos );
		ent->s.groundEntityNum = trace->entityNum;
		return;
	}

	VectorAdd( ent->currentOrigin, trace->plane.normal, ent->currentOrigin);
	VectorCopy( ent->currentOrigin, ent->s.pos.trBase );
	ent->s.pos.trTime = level.time;
}


/*
================
G_RunItem

================
*/
void G_RunItem( gentity_t *ent ) {
	vec3_t		origin;
	trace_t		tr;
	int			contents;
	int			mask;

	// if groundentity has been set to -1, it may have been pushed off an edge
	if ( ent->s.groundEntityNum == ENTITYNUM_NONE )
	{
		if ( ent->s.pos.trType != TR_GRAVITY )
		{
			ent->s.pos.trType = TR_GRAVITY;
			ent->s.pos.trTime = level.time;
		}
	}

	if ( ent->s.pos.trType == TR_STATIONARY )
	{
		// check think function
		G_RunThink( ent );
		if ( !g_gravity->value )
		{
			ent->s.pos.trType = TR_GRAVITY;
			ent->s.pos.trTime = level.time;
			ent->s.pos.trDelta[0] += crandom() * 40.0f; // I dunno, just do this??
			ent->s.pos.trDelta[1] += crandom() * 40.0f;
			ent->s.pos.trDelta[2] += random() * 20.0f;
		}
		return;
	}

	// get current position
	EvaluateTrajectory( &ent->s.pos, level.time, origin );

	// trace a line from the previous position to the current position
	if ( ent->clipmask )
	{
		mask = ent->clipmask;
	}
	else
	{
		mask = MASK_SOLID|CONTENTS_PLAYERCLIP;//shouldn't be able to get anywhere player can't
	}

	int ignore = ENTITYNUM_NONE;
	if ( ent->owner )
	{
		ignore = ent->owner->s.number;
	}
	else if ( ent->activator )
	{
		ignore = ent->activator->s.number;
	}
	gi.trace( &tr, ent->currentOrigin, ent->mins, ent->maxs, origin, ignore, mask, G2_NOCOLLIDE, 0 );

	VectorCopy( tr.endpos, ent->currentOrigin );

	if ( tr.startsolid )
	{
		tr.fraction = 0;
	}

	gi.linkentity( ent );	// FIXME: avoid this for stationary?

	// check think function
	G_RunThink( ent );

	if ( tr.fraction == 1 )
	{
		if ( g_gravity->value <= 0 )
		{
			if ( ent->s.apos.trType != TR_LINEAR )
			{
				VectorCopy( ent->currentAngles, ent->s.apos.trBase );
				ent->s.apos.trType = TR_LINEAR;
				ent->s.apos.trDelta[1] = Q_flrand( -300, 300 );
				ent->s.apos.trDelta[0] = Q_flrand( -10, 10 );
				ent->s.apos.trDelta[2] = Q_flrand( -10, 10 );
				ent->s.apos.trTime = level.time;
			}
		}
		//friction in zero-G
		if ( !g_gravity->value )
		{
			float friction = 0.975f;
			/*friction -= ent->mass/1000.0f;
			if ( friction < 0.1 )
			{
				friction = 0.1f;
			}
			*/
			VectorScale( ent->s.pos.trDelta, friction, ent->s.pos.trDelta );
			VectorCopy( ent->currentOrigin, ent->s.pos.trBase );
			ent->s.pos.trTime = level.time;
		}
		return;
	}

	// if it is in a nodrop volume, remove it
	contents = gi.pointcontents( ent->currentOrigin, -1 );
	if ( contents & CONTENTS_NODROP )
	{
		G_FreeEntity( ent );
		return;
	}

	if ( !tr.startsolid )
	{
		G_BounceItem( ent, &tr );
	}
}

/*
================
ItemUse_Bacta

================
*/
void ItemUse_Bacta(gentity_t *ent)
{
	if (!ent || !ent->client)
	{
		return;
	}

	if (ent->health >= ent->client->ps.stats[STAT_MAX_HEALTH] || !ent->client->ps.inventory[INV_BACTA_CANISTER] )
	{
		return;
	}

	ent->health += MAX_BACTA_HEAL_AMOUNT;

	if (ent->health > ent->client->ps.stats[STAT_MAX_HEALTH])
	{
		ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
	}

	ent->client->ps.inventory[INV_BACTA_CANISTER]--;

	G_SoundOnEnt( ent, CHAN_VOICE, va( "sound/weapons/force/heal%d.mp3", Q_irand( 1, 4 ) ) );
}
