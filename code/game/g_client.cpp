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

#include "../icarus/IcarusInterface.h"
#include "../cgame/cg_local.h"
#include "Q3_Interface.h"
#include "g_local.h"
#include "g_functions.h"
#include "anims.h"
#include "wp_saber.h"
#include "g_vehicles.h"
#include "objectives.h"
#include "b_local.h"

extern int WP_SaberInitBladeData( gentity_t *ent );
extern void G_CreateG2AttachedWeaponModel( gentity_t *ent, const char *weaponModel, int boltNum, int weaponNum );
extern qboolean	CheatsOk( gentity_t *ent );
extern void Boba_Precache( void );

extern cvar_t	*g_char_model;
extern cvar_t	*g_char_skin_head;
extern cvar_t	*g_char_skin_torso;
extern cvar_t	*g_char_skin_legs;
extern cvar_t	*g_char_color_red;
extern cvar_t	*g_char_color_green;
extern cvar_t	*g_char_color_blue;
extern cvar_t	*g_saber;
extern cvar_t	*g_saber2;
extern cvar_t	*g_saber_color;
extern cvar_t	*g_saber2_color;
extern cvar_t	*g_saberDarkSideSaberColor;

// g_client.c -- client functions that don't happen every frame

float DEFAULT_MINS_0 = -16;
float DEFAULT_MINS_1 = -16;
float DEFAULT_MAXS_0 = 16;
float DEFAULT_MAXS_1 = 16;
float DEFAULT_PLAYER_RADIUS	= sqrt((DEFAULT_MAXS_0*DEFAULT_MAXS_0) + (DEFAULT_MAXS_1*DEFAULT_MAXS_1));
vec3_t playerMins = {DEFAULT_MINS_0, DEFAULT_MINS_1, DEFAULT_MINS_2};
vec3_t playerMinsStep = {DEFAULT_MINS_0, DEFAULT_MINS_1, DEFAULT_MINS_2+STEPSIZE};
vec3_t playerMaxs = {DEFAULT_MAXS_0, DEFAULT_MAXS_1, DEFAULT_MAXS_2};

void SP_misc_teleporter_dest (gentity_t *ent);

/*QUAK-ED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 32) KEEP_PREV DROPTOFLOOR x x x STUN_BATON NOWEAPON x
potential spawning position for deathmatch games.
Targets will be fired when someone spawns in on them.
*/
void SP_info_player_deathmatch(gentity_t *ent) {
	SP_misc_teleporter_dest (ent);
	
	if ( ent->spawnflags & 32 ) // STUN_BATON
	{
		RegisterItem( FindItemForWeapon( WP_STUN_BATON ));
	}
	else
	{
		RegisterItem( FindItemForWeapon( WP_SABER ) );	//these are given in ClientSpawn(), but we register them now before cgame starts
		saberInfo_t	saber;
		WP_SaberParseParms( g_saber->string, &saber );//get saber sounds and models cached before client begins
		if (saber.model) G_ModelIndex( saber.model );
		if (saber.brokenSaber1) G_ModelIndex( saber.brokenSaber1 );
		if (saber.brokenSaber2) G_ModelIndex( saber.brokenSaber2 );
		if (saber.skin) G_SkinIndex( saber.skin );
		WP_SaberFreeStrings(saber);
	}
}

/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32) KEEP_PREV DROPTOFLOOR x x x STUN_BATON NOWEAPON x
KEEP_PREV - keep previous health + armor
DROPTOFLOOR - Player will start on the first solid structure under it
STUN_BATON - Gives player the stun baton and bryar pistol, but not the saber, plus any weapons they may have carried over from previous levels.

Targets will be fired when someone spawns in on them.
equivalant to info_player_deathmatch
*/
void SP_info_player_start(gentity_t *ent) {
	ent->classname = "info_player_deathmatch";

	SP_info_player_deathmatch( ent );
}



/*
=======================================================================

  SelectSpawnPoint

=======================================================================
*/

/*
================
SpotWouldTelefrag

================
*/
qboolean SpotWouldTelefrag( gentity_t *spot, team_t checkteam ) 
{
	int			i, num;
	gentity_t	*touch[MAX_GENTITIES], *hit;
	vec3_t		mins, maxs;

	// If we have a mins, use that instead of the hardcoded bounding box
	if ( spot->mins && VectorLength( spot->mins ) )
		VectorAdd( spot->s.origin, spot->mins, mins );
	else
		VectorAdd( spot->s.origin, playerMins, mins );

	// If we have a maxs, use that instead of the hardcoded bounding box
	if ( spot->maxs && VectorLength( spot->maxs ) )
		VectorAdd( spot->s.origin, spot->maxs, maxs );
	else
		VectorAdd( spot->s.origin, playerMaxs, maxs );

	num = gi.EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	for (i=0 ; i<num ; i++) 
	{
		hit = touch[i];
		if ( hit != spot && hit->client && hit->client->ps.stats[STAT_HEALTH] > 0 ) 
		{
			if ( hit->contents & CONTENTS_BODY )
			{
				if( checkteam == TEAM_FREE || hit->client->playerTeam == checkteam )
				{//checking against teammates only...?
					return qtrue;
				}
			}
		}
	}

	return qfalse;
}

qboolean SpotWouldTelefrag2( gentity_t *mover, vec3_t dest ) 
{
	int			i, num;
	gentity_t	*touch[MAX_GENTITIES], *hit;
	vec3_t		mins, maxs;

	VectorAdd( dest, mover->mins, mins );
	VectorAdd( dest, mover->maxs, maxs );
	num = gi.EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	for (i=0 ; i<num ; i++) 
	{
		hit = touch[i];
		if ( hit == mover )
		{
			continue;
		}

		if ( hit->contents & mover->contents )
		{
			return qtrue;
		}
	}

	return qfalse;
}
/*
================
SelectNearestDeathmatchSpawnPoint

Find the spot that we DON'T want to use
================
*/
#define	MAX_SPAWN_POINTS	128
gentity_t *SelectNearestDeathmatchSpawnPoint( vec3_t from, team_t team ) {
	gentity_t	*spot;
	float		dist, nearestDist;
	gentity_t	*nearestSpot;

	nearestDist = (float)WORLD_SIZE*(float)WORLD_SIZE;
	nearestSpot = NULL;
	spot = NULL;

	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
		/*if ( team == TEAM_RED && ( spot->spawnflags & 2 ) ) {
			continue;
		}
		if ( team == TEAM_BLUE && ( spot->spawnflags & 1 ) ) {
			continue;
		}*/

		if ( spot->targetname != NULL ) {
			//this search routine should never find a spot that is targetted
			continue;
		}
		dist = DistanceSquared( spot->s.origin, from );
		if ( dist < nearestDist ) {
			nearestDist = dist;
			nearestSpot = spot;
		}
	}

	return nearestSpot;
}


/*
================
SelectRandomDeathmatchSpawnPoint

go to a random point that doesn't telefrag
================
*/
#define	MAX_SPAWN_POINTS	128
gentity_t *SelectRandomDeathmatchSpawnPoint( team_t team ) {
	gentity_t	*spot;
	int			count;
	int			selection;
	gentity_t	*spots[MAX_SPAWN_POINTS];

	count = 0;
	spot = NULL;

	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
		/*if ( team == TEAM_RED && ( spot->spawnflags & 2 ) ) {
			continue;
		}
		if ( team == TEAM_BLUE && ( spot->spawnflags & 1 ) ) {
			continue;
		}*/

		if ( spot->targetname != NULL ) {
			//this search routine should never find a spot that is targetted
			continue;
		}
		if ( SpotWouldTelefrag( spot, TEAM_FREE ) ) {
			continue;
		}
		spots[ count ] = spot;
		count++;
	}

	if ( !count ) {	// no spots that won't telefrag
		spot = G_Find( NULL, FOFS(classname), "info_player_deathmatch");
		if ( !spot )
		{
			return NULL;
		}
		if ( spot->targetname != NULL ) 
		{
			//this search routine should never find a spot that is targetted
			return NULL;
		}
		else
		{
			return spot;
		}
	}

	selection = rand() % count;
	return spots[ selection ];
}


/*
===========
SelectSpawnPoint

Chooses a player start, deathmatch start, etc
============
*/
gentity_t *SelectSpawnPoint ( vec3_t avoidPoint, team_t team, vec3_t origin, vec3_t angles ) {
	gentity_t	*spot;
	gentity_t	*nearestSpot;

	if ( level.spawntarget != NULL && level.spawntarget[0] )
	{//we have a spawnpoint specified, try to find it
		if ( (nearestSpot = spot = G_Find( NULL, FOFS(targetname), level.spawntarget )) == NULL )
		{//you HAVE to be able to find the desired spot
			G_Error( "Couldn't find spawntarget %s\n", level.spawntarget );
			return NULL;
		}
	}
	else 
	{//not looking for a special startspot
		nearestSpot = SelectNearestDeathmatchSpawnPoint( avoidPoint, team );

		spot = SelectRandomDeathmatchSpawnPoint ( team );
		if ( spot == nearestSpot ) {
			// roll again if it would be real close to point of death
			spot = SelectRandomDeathmatchSpawnPoint ( team );
		}
	}

	// find a single player start spot
	if (!spot) {
		G_Error( "Couldn't find a spawn point\n" );
	}


	VectorCopy( spot->s.origin, origin );
	if ( spot->spawnflags & 2 )
	{
		trace_t		tr;

		origin[2] = MIN_WORLD_COORD;
		gi.trace(&tr, spot->s.origin, playerMins, playerMaxs, origin, ENTITYNUM_NONE, MASK_PLAYERSOLID, (EG2_Collision)0, 0 );
		if ( tr.fraction < 1.0 && !tr.allsolid && !tr.startsolid )
		{//found a floor
			VectorCopy(tr.endpos, origin );
		}
		else
		{//In solid or too far
			VectorCopy( spot->s.origin, origin );
		}
	}

	origin[2] += 9;
	VectorCopy (spot->s.angles, angles);

	return spot;
}


//======================================================================


/*
==================
SetClientViewAngle

==================
*/
void SetClientViewAngle( gentity_t *ent, vec3_t angle ) {
	int			i;

	// set the delta angle
	for (i=0 ; i<3 ; i++) 
	{
		ent->client->ps.delta_angles[i] = (ANGLE2SHORT(angle[i]) - ent->client->pers.cmd_angles[i])&0xffff;
	}
	VectorCopy( angle, ent->s.angles );
	VectorCopy (ent->s.angles, ent->client->ps.viewangles);
}

/*
================
respawn
================
*/
void respawn( gentity_t *ent ) {

	gi.SendConsoleCommand("load *respawn\n");	// special case
}

/*
===========
ClientCheckName
============
*/
static void ClientCleanName( const char *in, char *out, int outSize )
{
	int outpos = 0, colorlessLen = 0, spaces = 0, ats = 0;

	// discard leading spaces
	for ( ; *in == ' '; in++);

	// discard leading asterisk's (fail raven for using * as a skipnotify)
	// apparently .* causes the issue too so... derp
	//for(; *in == '*'; in++);
	
	for(; *in && outpos < outSize - 1; in++)
	{
		out[outpos] = *in;

		if ( *in == ' ' )
		{// don't allow too many consecutive spaces
			if ( spaces > 2 )
				continue;

			spaces++;
		}
		else if ( *in == '@' )
		{// don't allow too many consecutive at signs
			if ( ats > 2 )
				continue;

			ats++;
		}
		else if ( outpos > 0 && out[outpos-1] == Q_COLOR_ESCAPE )
		{
			if ( Q_IsColorStringExt( &out[outpos-1] ) )
			{
				colorlessLen--;
				
#if 0
				if ( ColorIndex( *in ) == 0 )
				{// Disallow color black in names to prevent players from getting advantage playing in front of black backgrounds
					outpos--;
					continue;
				}
#endif
			}
			else
			{
				spaces = ats = 0;
				colorlessLen++;
			}
		}
		else
		{
			spaces = ats = 0;
			colorlessLen++;
		}
		
		outpos++;
	}

	out[outpos] = '\0';

	// don't allow empty names
	if ( *out == '\0' || colorlessLen == 0 )
		Q_strncpyz( out, "Padawan", outSize );
}

/*
===========
ClientUserInfoChanged

Called from ClientConnect when the player first connects and
directly by the server system when the player updates a userinfo variable.

The game can override any of the settings and call gi.SetUserinfo
if desired.
============
*/
void ClientUserinfoChanged( int clientNum ) {
	gentity_t	*ent = g_entities + clientNum;
	gclient_t	*client = ent->client;
	int			health=100, maxHealth=100;
	const char	*s=NULL;
	char		userinfo[MAX_INFO_STRING]={0},	buf[MAX_INFO_STRING]={0},
				sound[MAX_STRING_CHARS]={0},	oldname[34]={0};

	gi.GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	// check for malformed or illegal info strings
	/*if ( !Info_Validate(userinfo) ) {
		strcpy (userinfo, "\\name\\badinfo");
	}*/

	// set name
	Q_strncpyz ( oldname, client->pers.netname, sizeof( oldname ) );
	s = Info_ValueForKey (userinfo, "name");
	ClientCleanName( s, client->pers.netname, sizeof( client->pers.netname ) );

	// set max health
	maxHealth = 100;
	health = Com_Clampi( 1, 100, atoi( Info_ValueForKey( userinfo, "handicap" ) ) );
	client->pers.maxHealth = health;
	if ( client->pers.maxHealth < 1 || client->pers.maxHealth > maxHealth )
		client->pers.maxHealth = 100;
	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;

	// sounds
	Q_strncpyz( sound, Info_ValueForKey (userinfo, "snd"), sizeof( sound ) );

	// send over a subset of the userinfo keys so other clients can
	// print scoreboards, display models, and play custom sounds
	buf[0] = '\0';
	Q_strcat( buf, sizeof( buf ), va( "n\\%s\\", client->pers.netname ) );
	Q_strcat( buf, sizeof( buf ), va( "t\\%i\\", client->sess.sessionTeam ) );
	Q_strcat( buf, sizeof( buf ),	  "headModel\\\\" );
	Q_strcat( buf, sizeof( buf ),	  "torsoModel\\\\" );
	Q_strcat( buf, sizeof( buf ),	  "legsModel\\\\" );
	Q_strcat( buf, sizeof( buf ), va( "hc\\%i\\", client->pers.maxHealth ) );
	Q_strcat( buf, sizeof( buf ), va( "snd\\%s\\", sound ) );

	gi.SetConfigstring( CS_PLAYERS+clientNum, buf );
}


/*
===========
ClientConnect

Called when a player begins connecting to the server.
Called again for every map change or tournement restart.

The session information will be valid after exit.

Return NULL if the client should be allowed, otherwise return
a string with the reason for denial.

Otherwise, the client will be sent the current gamestate
and will eventually get to ClientBegin.

firstTime will be qtrue the very first time a client connects
to the server machine, but qfalse on map changes and tournement
restarts.
============
*/
char *ClientConnect( int clientNum, qboolean firstTime, SavedGameJustLoaded_e eSavedGameJustLoaded ) 
{
	gentity_t	*ent = &g_entities[ clientNum ];
	char		userinfo[MAX_INFO_STRING] = {0};

	gi.GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	// they can connect
	ent->client = level.clients + clientNum;
	gclient_t *client = ent->client;

//	if (!qbFromSavedGame)
	if (eSavedGameJustLoaded != eFULL)
	{
		clientSession_t savedSess = client->sess;	// 
		memset( client, 0, sizeof(*client) );
		client->sess = savedSess; 
		if ( firstTime ) {	//not loading full, and directconnect
			client->playerTeam = TEAM_PLAYER;	//set these now because after an auto_load kyle can see your team for a bit before you really join.
			client->enemyTeam = TEAM_ENEMY;
		}
	}

	client->pers.connected = CON_CONNECTING;

	if (eSavedGameJustLoaded == eFULL)//qbFromSavedGame)
	{
		// G_WriteClientSessionData( client ); // forget it, this is DM stuff anyway
		// get and distribute relevent paramters
		ClientUserinfoChanged( clientNum );
	}
	else
	{
		// read or initialize the session data
		if ( firstTime ) {
			G_InitSessionData( client, userinfo );
		}
		G_ReadSessionData( client );

		// get and distribute relevent paramters
		ClientUserinfoChanged( clientNum );

		// don't do the "xxx connected" messages if they were caried over from previous level
		if ( firstTime ) {
			gi.SendServerCommand( -1, "print \"%s connected\n\"", client->pers.netname);
		}
	}

	return NULL;
}

/*
===========
ClientBegin

called when a client has finished connecting, and is ready
to be placed into the level.  This will happen every level load,
and on transition between teams, but doesn't happen on respawns
============
*/
void ClientBegin( int clientNum, usercmd_t *cmd, SavedGameJustLoaded_e eSavedGameJustLoaded)		
//												qboolean qbFromSavedGame 
{
	gentity_t	*ent;
	gclient_t	*client;

	ent = g_entities + clientNum;
	client = level.clients + clientNum;

	if (eSavedGameJustLoaded == eFULL)//qbFromSavedGame)
	{
		client->pers.connected = CON_CONNECTED;
		ent->client = client;
		ClientSpawn( ent, eSavedGameJustLoaded );
	}
	else
	{
		if ( ent->linked ) {
			gi.unlinkentity( ent );
		}
		G_InitGentity( ent, qfalse );
		ent->e_TouchFunc = touchF_NULL;
		ent->e_PainFunc  = painF_PlayerPain;//painF_NULL;
		ent->client = client;

		client->pers.connected = CON_CONNECTED;
		client->pers.teamState.state = TEAM_BEGIN;
		_VectorCopy( cmd->angles, client->pers.cmd_angles );

		memset( &client->ps, 0, sizeof( client->ps ) );
		if( gi.Cvar_VariableIntegerValue( "g_clearstats" ) )
		{
			memset( &client->sess.missionStats, 0, sizeof( client->sess.missionStats ) );
			client->sess.missionStats.totalSecrets = gi.Cvar_VariableIntegerValue("newTotalSecrets");
		}
		
		// locate ent at a spawn point
		if ( ClientSpawn( ent, eSavedGameJustLoaded) )	// SavedGameJustLoaded_e
		{
			// send teleport event
		}
		client->ps.inventory[INV_GOODIE_KEY] = 0;
		client->ps.inventory[INV_SECURITY_KEY] = 0;
	}
}



/*
============
Player_CacheFromPrevLevel
  Description	: just need to grab the weapon items we're going to have when we spawn so they'll be cached
  Return type	: void 
  Argument		: void
============
*/
void Player_CacheFromPrevLevel(void)
{
	char	s[MAX_STRING_CHARS];
	int i;
	
	gi.Cvar_VariableStringBuffer( sCVARNAME_PLAYERSAVE, s, sizeof(s) );
	
	if (s[0])	// actually this would be safe anyway because of the way sscanf() works, but this is clearer
	{
		int iDummy, bits, ibits;

		sscanf( s, "%i %i %i %i", 
			&iDummy,//client->ps.stats[STAT_HEALTH],
			&iDummy,//client->ps.stats[STAT_ARMOR],
			&bits,	//client->ps.stats[STAT_WEAPONS]
			&ibits	//client->ps.stats[STAT_ITEMS]
			);

		for ( i = 1 ; i < 16 ; i++ ) 
		{
			if ( bits & ( 1 << i ) ) 
			{
				RegisterItem( FindItemForWeapon( (weapon_t)i ) );
			}
		}

extern gitem_t	*FindItemForInventory( int inv );

		for ( i = 1 ; i < 16 ; i++ ) 
		{
			if ( ibits & ( 1 << i ) ) 
			{
				RegisterItem( FindItemForInventory( i-1 ));
			}
		}
	}
}

/*
============
Player_RestoreFromPrevLevel
  Description	: retrieve maptransition data recorded by server when exiting previous level (to carry over weapons/ammo/health/etc)
  Return type	: void 
  Argument		: gentity_t *ent
============
*/
static void Player_RestoreFromPrevLevel(gentity_t *ent, SavedGameJustLoaded_e eSavedGameJustLoaded)
{
	gclient_t	*client = ent->client;
	int			i;

	assert(client);
	if (client)	// though I can't see it not being true...
	{
		char	s[MAX_STRING_CHARS];
		char	saber0Name[MAX_QPATH];
		char	saber1Name[MAX_QPATH];
		const char	*var;
						
		gi.Cvar_VariableStringBuffer( sCVARNAME_PLAYERSAVE, s, sizeof(s) );

		if (strlen(s))	// actually this would be safe anyway because of the way sscanf() works, but this is clearer
		{//				|general info				  |-force powers |-saber 1										   |-saber 2										  |-general saber
			unsigned int saber1BladeColor[8];
			unsigned int saber2BladeColor[8];

			sscanf( s, "%i %i %i %i %i %i %i %f %f %f %i %i %i %i %i %s %i %i %i %i %i %i %i %i %u %u %u %u %u %u %u %u %s %i %i %i %i %i %i %i %i %u %u %u %u %u %u %u %u %i %i %i %i", 
								&client->ps.stats[STAT_HEALTH],
								&client->ps.stats[STAT_ARMOR],
								&client->ps.stats[STAT_WEAPONS],
								&client->ps.stats[STAT_ITEMS],
								&client->ps.weapon,
								&client->ps.weaponstate,
								&client->ps.batteryCharge,
								&client->ps.viewangles[0],
								&client->ps.viewangles[1],
								&client->ps.viewangles[2],
								//force power data
								&client->ps.forcePowersKnown,
								&client->ps.forcePower,
								&client->ps.forcePowerMax,
								&client->ps.forcePowerRegenRate,
								&client->ps.forcePowerRegenAmount,
								//saber 1 data
								saber0Name,
								&client->ps.saber[0].blade[0].active,
								&client->ps.saber[0].blade[1].active,
								&client->ps.saber[0].blade[2].active,
								&client->ps.saber[0].blade[3].active,
								&client->ps.saber[0].blade[4].active,
								&client->ps.saber[0].blade[5].active,
								&client->ps.saber[0].blade[6].active,
								&client->ps.saber[0].blade[7].active,
								&saber1BladeColor[0],
								&saber1BladeColor[1],
								&saber1BladeColor[2],
								&saber1BladeColor[3],
								&saber1BladeColor[4],
								&saber1BladeColor[5],
								&saber1BladeColor[6],
								&saber1BladeColor[7],
								//saber 2 data
								saber1Name,
								&client->ps.saber[1].blade[0].active,
								&client->ps.saber[1].blade[1].active,
								&client->ps.saber[1].blade[2].active,
								&client->ps.saber[1].blade[3].active,
								&client->ps.saber[1].blade[4].active,
								&client->ps.saber[1].blade[5].active,
								&client->ps.saber[1].blade[6].active,
								&client->ps.saber[1].blade[7].active,
								&saber2BladeColor[0],
								&saber2BladeColor[1],
								&saber2BladeColor[2],
								&saber2BladeColor[3],
								&saber2BladeColor[4],
								&saber2BladeColor[5],
								&saber2BladeColor[6],
								&saber2BladeColor[7],
								//general saber data
								&client->ps.saberStylesKnown,
								&client->ps.saberAnimLevel,
								&client->ps.saberLockEnemy,
								&client->ps.saberLockTime
					);
			for (int j = 0; j < 8; j++)
			{
				client->ps.saber[0].blade[j].color = (saber_colors_t)saber1BladeColor[j];
				client->ps.saber[1].blade[j].color = (saber_colors_t)saber2BladeColor[j];
			}

			ent->health = client->ps.stats[STAT_HEALTH];

			if(ent->client->ps.saber[0].name && gi.bIsFromZone(ent->client->ps.saber[0].name, TAG_G_ALLOC)) {
				gi.Free(ent->client->ps.saber[0].name);
			}
			ent->client->ps.saber[0].name=0;

			if(ent->client->ps.saber[1].name && gi.bIsFromZone(ent->client->ps.saber[1].name, TAG_G_ALLOC) ) {
				gi.Free(ent->client->ps.saber[1].name);
			}
			ent->client->ps.saber[1].name=0;
			//NOTE: if sscanf can get a "(null)" out of strings that had NULL string pointers plugged into the original string
			if ( saber0Name[0] && Q_stricmp( "(null)", saber0Name ) != 0 )
			{
				ent->client->ps.saber[0].name = G_NewString( saber0Name );
			}
			if ( saber1Name[0] && Q_stricmp( "(null)", saber1Name ) != 0 )
			{//have a second saber
				ent->client->ps.saber[1].name = G_NewString( saber1Name );
				ent->client->ps.dualSabers = qtrue;
			}
			else
			{//have only 1 saber
				ent->client->ps.dualSabers = qfalse;
			}

// slight issue with ths for the moment in that although it'll correctly restore angles it doesn't take into account
//	the overall map orientation, so (eg) exiting east to enter south will be out by 90 degrees, best keep spawn angles for now
//
//			VectorClear (ent->client->pers.cmd_angles);
//
//			SetClientViewAngle( ent, ent->client->ps.viewangles);

			//ammo
			gi.Cvar_VariableStringBuffer( "playerammo", s, sizeof(s) );
			i=0;
			var = strtok( s, " " );
			while( var != NULL )
			{
			  /* While there are tokens in "s" */
			  client->ps.ammo[i++] = atoi(var);
			  /* Get next token: */
			  var = strtok( NULL, " " );
			}
			assert (i==AMMO_MAX);

			//inventory
			gi.Cvar_VariableStringBuffer( "playerinv", s, sizeof(s) );
			i=0;
			var = strtok( s, " " );
			while( var != NULL )
			{
			  /* While there are tokens in "s" */
			  client->ps.inventory[i++] = atoi(var);
			  /* Get next token: */
			  var = strtok( NULL, " " );
			}
			assert (i==INV_MAX);


			// the new JK2 stuff - force powers, etc...
			//
			gi.Cvar_VariableStringBuffer( "playerfplvl", s, sizeof(s) );
			i=0;
			var = strtok( s, " " );
			while( var != NULL )
			{
				/* While there are tokens in "s" */
				  client->ps.forcePowerLevel[i++] = atoi(var);
				/* Get next token: */
				var = strtok( NULL, " " );
			}
			assert (i==NUM_FORCE_POWERS);

			client->ps.forceGripEntityNum = client->ps.forceDrainEntityNum = ENTITYNUM_NONE;
		}
	}
}

/*
Ghoul2 Insert Start
*/

static void G_SetSkin( gentity_t *ent )
{
	char	skinName[MAX_QPATH];
	//ok, lets register the skin name, and then pass that name to the config strings so the client can get it too.
	if (Q_stricmp( "hoth2", level.mapname ) == 0	//hack, is this the only map?
		||
		Q_stricmp( "hoth3", level.mapname ) == 0	// no! ;-)
		)
	{
		Com_sprintf( skinName, sizeof( skinName ), "models/players/%s/|%s|%s|%s", g_char_model->string, g_char_skin_head->string, "torso_g1", "lower_e1" );
	}
	else if(Q_stricmp(g_char_skin_head->string, "model_default") == 0 && Q_stricmp(g_char_skin_torso->string, "model_default") == 0 && Q_stricmp(g_char_skin_legs->string, "model_default") == 0)
	{
		Com_sprintf( skinName, sizeof( skinName ), "models/players/%s/model_default.skin", g_char_model->string );
	}
	else
	{
		Com_sprintf( skinName, sizeof( skinName ), "models/players/%s/|%s|%s|%s", g_char_model->string, g_char_skin_head->string, g_char_skin_torso->string, g_char_skin_legs->string );
	}

	// lets see if it's out there
	int skin = gi.RE_RegisterSkin( skinName );
	if ( skin )
	{//what if this returns 0 because *one* part of a multi-skin didn't load?
		// put it in the config strings
		// and set the ghoul2 model to use it
		gi.G2API_SetSkin( &ent->ghoul2[ent->playerModel], G_SkinIndex( skinName ), skin );
	}
	
	//color tinting
	if ( g_char_color_red->integer 
		|| g_char_color_green->integer
		|| g_char_color_blue->integer )
	{
		ent->client->renderInfo.customRGBA[0] = g_char_color_red->integer;
		ent->client->renderInfo.customRGBA[1] = g_char_color_green->integer;
		ent->client->renderInfo.customRGBA[2] = g_char_color_blue->integer;
		ent->client->renderInfo.customRGBA[3] = 255;
	}
}

qboolean G_StandardHumanoid( gentity_t *self )
{
	if ( !self || !self->ghoul2.size() )
	{
		return qfalse;
	}
	if ( self->playerModel < 0 || self->playerModel >= self->ghoul2.size() )
	{
		return qfalse;
	}
	const char	*GLAName = gi.G2API_GetGLAName( &self->ghoul2[self->playerModel] );
	assert(GLAName);
	if (GLAName) 
	{
		if ( !Q_stricmpn( "models/players/_humanoid", GLAName, 24 ) )///_humanoid", GLAName, 36) )
		{//only _humanoid skeleton is expected to have these
			return qtrue;
		}
		if ( !Q_stricmp( "models/players/protocol/protocol", GLAName ) )
		{//protocol droid duplicates many of these
			return qtrue;
		}
		if ( !Q_stricmp( "models/players/assassin_droid/model", GLAName ) )
		{//assassin_droid duplicates many of these
			return qtrue;
		}
		if ( !Q_stricmp( "models/players/saber_droid/model", GLAName ) )
		{//saber_droid duplicates many of these
			return qtrue;
		}
		if ( !Q_stricmp( "models/players/hazardtrooper/hazardtrooper", GLAName ) )
		{//hazardtrooper duplicates many of these
			return qtrue;
		}
		if ( !Q_stricmp( "models/players/rockettrooper/rockettrooper", GLAName ) )
		{//rockettrooper duplicates many of these
			return qtrue;
		}
		if ( !Q_stricmp( "models/players/wampa/wampa", GLAName ) )
		{//rockettrooper duplicates many of these
			return qtrue;
		}
	}
	return qfalse;
}

qboolean G_ClassHasBadBones( int NPC_class )
{
	switch ( NPC_class )
	{
	case CLASS_WAMPA:
	case CLASS_ROCKETTROOPER:
	case CLASS_SABER_DROID:
	case CLASS_HAZARD_TROOPER:
	case CLASS_ASSASSIN_DROID:
	case CLASS_RANCOR:
		return qtrue;
	}
	return qfalse;
}

const char *AxesNames[] = 
{
	"ORIGIN",//ORIGIN, 
	"POSITIVE_X",//POSITIVE_X,
	"POSITIVE_Z",//POSITIVE_Z,
	"POSITIVE_Y",//POSITIVE_Y,
	"NEGATIVE_X",//NEGATIVE_X,
	"NEGATIVE_Z",//NEGATIVE_Z,
	"NEGATIVE_Y"//NEGATIVE_Y
};

Eorientations testAxes[3]={POSITIVE_X,POSITIVE_Z,POSITIVE_Y};
int axes_0 = POSITIVE_X;
int axes_1 = POSITIVE_Z;
int axes_2 = POSITIVE_Y;
void G_NextTestAxes( void )
{
	static int whichAxes = 0;
	int axesCount = 0;
	do
	{
		whichAxes++;
		if ( whichAxes > 216 )
		{
			whichAxes = 0;
			Com_Printf( S_COLOR_RED"WRAPPED\n" );
			break;
		}
		axesCount = 0;
		axes_0 = 0;
		axes_1 = 0;
		axes_2 = 0;
		for ( axes_0 = 0; axes_0 < 6 && (axesCount<whichAxes); axes_0++ )
		{
			axesCount++;
			for ( axes_1 = 0; axes_1 < 6 && (axesCount<whichAxes); axes_1++ )
			{
				axesCount++;
				for ( axes_2 = 0; axes_2 < 6 && (axesCount<whichAxes); axes_2++ )
				{
					axesCount++;
				}
			}
		}
		testAxes[0] = (Eorientations)((axes_0%6)+1);
		testAxes[1] = (Eorientations)((axes_1%6)+1);
		testAxes[2] = (Eorientations)((axes_2%6)+1);
	} while ( testAxes[1] == testAxes[0] || (testAxes[1]-testAxes[0]) == 3 || (testAxes[0]-testAxes[1]) == 3
		|| testAxes[2] == testAxes[0] || (testAxes[2]-testAxes[0]) == 3 || (testAxes[0]-testAxes[2]) == 3
		|| testAxes[2] == testAxes[1] || (testAxes[2]-testAxes[1]) == 3 || (testAxes[1]-testAxes[2]) == 3 );

	Com_Printf( "Up: %s\nRight: %s\nForward: %s\n", AxesNames[testAxes[0]], AxesNames[testAxes[1]], AxesNames[testAxes[2]] );
	if ( testAxes[0] == POSITIVE_X
		&& testAxes[1] == POSITIVE_Z
		&& testAxes[2] == POSITIVE_Y )
	{
		Com_Printf( S_COLOR_RED"WRAPPED\n" );
	}
}

void G_BoneOrientationsForClass( int NPC_class, const char *boneName, Eorientations *oUp, Eorientations *oRt, Eorientations *oFwd )
{
	//defaults
	*oUp = POSITIVE_X;
	*oRt = NEGATIVE_Y;
	*oFwd = NEGATIVE_Z;
	//switch off class
	switch ( NPC_class )
	{
	case CLASS_RANCOR:
		*oUp = NEGATIVE_X;
		*oRt = POSITIVE_Y;
		*oFwd = POSITIVE_Z;
		//*oUp = testAxes[0];
		//*oRt = testAxes[1];
		//*oFwd = testAxes[2];
		break;
	case CLASS_ROCKETTROOPER:
	case CLASS_HAZARD_TROOPER:
		//Root is:
		//*oUp = POSITIVE_Z;
		//*oRt = NEGATIVE_X;
		//*oFwd = NEGATIVE_Y;
		if ( Q_stricmp( "pelvis", boneName ) == 0 )
		{//child of root
			//in ModView:
			//*oUp = NEGATIVE_X;
			//*oRt = NEGATIVE_Z;
			//*oFwd = NEGATIVE_Y;
			//actual, when differences with root are accounted for:
			*oUp = POSITIVE_Z;
			*oRt = NEGATIVE_X;
			*oFwd = NEGATIVE_Y;
		}
		else
		{//all the rest are the same, children of root (not pelvis)
			//in ModView:
			//*oUp = POSITIVE_X;
			//*oRt = POSITIVE_Y;
			//*oFwd = POSITIVE_Z;
			//actual, when differences with root are accounted for:
			//*oUp = POSITIVE_Z;
			//*oRt = NEGATIVE_Y;
			//*oFwd = NEGATIVE_X;
			*oUp = NEGATIVE_X;
			*oRt = POSITIVE_Y;
			*oFwd = POSITIVE_Z;
		}
		break;
	case CLASS_SABER_DROID:
		if ( Q_stricmp( "pelvis", boneName ) == 0
			|| Q_stricmp( "thoracic", boneName ) == 0 )
		{
			*oUp = NEGATIVE_X;
			*oRt = NEGATIVE_Z;
			*oFwd = NEGATIVE_Y;
		}
		else
		{
			*oUp = NEGATIVE_X;//POSITIVE_X;
			*oRt = POSITIVE_Y;
			*oFwd = POSITIVE_Z;
		}
		break;
	case CLASS_WAMPA:
		if ( Q_stricmp( "pelvis", boneName ) == 0 )
		{
			*oUp = NEGATIVE_X;
			*oRt = POSITIVE_Y;
			*oFwd = NEGATIVE_Z;
		}
		else
		{
			//*oUp = POSITIVE_X;
			//*oRt = POSITIVE_Y;
			//*oFwd = POSITIVE_Z;
			//kinda worked
			*oUp = NEGATIVE_X;
			*oRt = POSITIVE_Y;
			*oFwd = POSITIVE_Z;
		}
		break;
	case CLASS_ASSASSIN_DROID:
		if ( Q_stricmp( "pelvis", boneName ) == 0
			|| Q_stricmp( "lower_lumbar", boneName ) == 0
			|| Q_stricmp( "upper_lumbar", boneName ) == 0 )
		{//only these 3 bones on them are wrong
			//*oUp = POSITIVE_X;
			//*oRt = POSITIVE_Y;
			//*oFwd = POSITIVE_Z;
			*oUp = NEGATIVE_X;
			*oRt = POSITIVE_Y;
			*oFwd = POSITIVE_Z;
		}
		break;
	}
}

extern void G_LoadAnimFileSet( gentity_t *ent, const char *modelName );
qboolean G_SetG2PlayerModelInfo( gentity_t *ent, const char *modelName, const char *customSkin, const char *surfOff, const char *surfOn )
{
	if ( ent->playerModel != -1 )
	{// we found the model ok
		vec3_t	angles = {0,0,0};
		const char	*token;
		const char	*p;

		//Now turn on/off any surfaces
		if ( surfOff && surfOff[0] )
		{
			p = surfOff;
			COM_BeginParseSession();
			while ( 1 ) 
			{
				token = COM_ParseExt( &p, qtrue );
				if ( !token[0] ) 
				{//reached end of list
					break;
				}
				//turn off this surf
				gi.G2API_SetSurfaceOnOff( &ent->ghoul2[ent->playerModel], token, 0x00000002/*G2SURFACEFLAG_OFF*/ );
				
			}
			COM_EndParseSession();
		}
		if ( surfOn && surfOn[0] )
		{
			p = surfOn;
			COM_BeginParseSession();
			while ( 1 )
			{
				token = COM_ParseExt( &p, qtrue );
				if ( !token[0] ) 
				{//reached end of list
					break;
				}
				//turn on this surf
				gi.G2API_SetSurfaceOnOff( &ent->ghoul2[ent->playerModel], token, 0 );
			}
			COM_EndParseSession();
		}
		if ( ent->client->NPC_class == CLASS_IMPERIAL && ent->message )
		{//carrying a key, turn on the key sleeve surface (assuming we have one)
			gi.G2API_SetSurfaceOnOff( &ent->ghoul2[ent->playerModel], "l_arm_key", 0 );
		}

		G_LoadAnimFileSet( ent, modelName );
		
		ent->headBolt = ent->cervicalBolt = ent->torsoBolt = ent->gutBolt = ent->chestBolt = 
			ent->crotchBolt = ent->elbowLBolt = ent->elbowRBolt = ent->handLBolt = 
			ent->handRBolt = ent->kneeLBolt = ent->kneeRBolt = ent->footLBolt = 
			ent->footRBolt = -1;
		// now turn on the bolt in the hand - this one would be best always turned on.
		if ( G_StandardHumanoid( ent ) )
		{//only _humanoid skeleton is expected to have these
			ent->headBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*head_eyes");
			ent->cervicalBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "cervical" );
			if ( !Q_stricmp("protocol", modelName ) )
			{//*sigh*, no thoracic bone
				ent->gutBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "upper_lumbar");
				ent->chestBolt = ent->gutBolt;
			}
			else
			{
				ent->chestBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "thoracic");
				ent->gutBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "upper_lumbar");
			}
			ent->torsoBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "lower_lumbar");
			ent->crotchBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "pelvis");
			ent->elbowLBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*l_arm_elbow");
			ent->elbowRBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*r_arm_elbow");
			ent->handLBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*l_hand");
			ent->handRBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*r_hand");
			ent->kneeLBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*hips_l_knee");
			ent->kneeRBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*hips_r_knee");
			ent->footLBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*l_leg_foot");
			ent->footRBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*r_leg_foot");
			if ( ent->client->NPC_class == CLASS_BOBAFETT
				|| ent->client->NPC_class == CLASS_ROCKETTROOPER )
			{//get jet bolts
				ent->genericBolt1 = gi.G2API_AddBolt( &ent->ghoul2[ent->playerModel], "*jet1" );
				ent->genericBolt2 = gi.G2API_AddBolt( &ent->ghoul2[ent->playerModel], "*jet2" );
			}
			if ( ent->client->NPC_class == CLASS_BOBAFETT )
			{//get the flamethrower bolt
				ent->genericBolt3 = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*flamethrower");
			}
		}
		else 
		{
			if ( ent->client->NPC_class == CLASS_VEHICLE )
			{//do vehicles tags

				// Setup the driver tag (where the driver is mounted to).
				ent->crotchBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*driver");

				// Setup the droid unit (or other misc tag we're using this for).
				ent->m_pVehicle->m_iDroidUnitTag = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*droidunit");

				char strTemp[128];

				// Setup the Exhausts.
				for ( int i = 0; i < MAX_VEHICLE_EXHAUSTS; i++ )
				{
					Com_sprintf( strTemp, 128, "*exhaust%d", i + 1 );
					ent->m_pVehicle->m_iExhaustTag[i] = gi.G2API_AddBolt( &ent->ghoul2[ent->playerModel], strTemp );
				}

				// Setup the Muzzles.
				for ( int i = 0; i < MAX_VEHICLE_MUZZLES; i++ )
				{
					Com_sprintf( strTemp, 128, "*muzzle%d", i + 1 );
					ent->m_pVehicle->m_iMuzzleTag[i] = gi.G2API_AddBolt( &ent->ghoul2[ent->playerModel], strTemp );
					if ( ent->m_pVehicle->m_iMuzzleTag[i] == -1 )
					{//ergh, try *flash?
						Com_sprintf( strTemp, 128, "*flash%d", i + 1 );
						ent->m_pVehicle->m_iMuzzleTag[i] = gi.G2API_AddBolt( &ent->ghoul2[ent->playerModel], strTemp );
					}
				}
			}
			else if ( ent->client->NPC_class == CLASS_HOWLER )
			{
				ent->genericBolt1 = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "Tongue01" );// tongue base
				ent->genericBolt2 = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "Tongue08" );// tongue tip
			}
			else if ( !Q_stricmp( "gonk", modelName ) || !Q_stricmp( "seeker", modelName ) || !Q_stricmp( "remote", modelName ) 
						|| !Q_stricmpn( "r2d2", modelName, 4 ) || !Q_stricmpn( "r5d2", modelName, 4 ) )
			{//TEMP HACK: not a non-humanoid droid
				ent->headBolt = -1;
			}
			else if (!Q_stricmp( "interrogator",modelName))
			{
				ent->headBolt = -1;				
			}
			else if (!Q_stricmpn( "probe",modelName, 5))
			{
				ent->headBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "cranium");		// head pivot point
				ent->genericBolt1 = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*flash");		// Gun 1
			}
			else if (!Q_stricmp( "sentry",modelName))
			{
				ent->headBolt = -1;				
				ent->genericBolt1 = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*flash1");		// Gun 1
				ent->genericBolt2 = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*flash2");		// Gun 2
				ent->genericBolt3 = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*flash03");		// Gun 3
			}
			else if (!Q_stricmp( "mark1",modelName))
			{
				ent->headBolt = -1;				
				ent->handRBolt = ent->genericBolt1 = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*flash1");		// Blaster Gun 1
				ent->genericBolt2 = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*flash2");		// Blaster Gun 2
				ent->genericBolt3 = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*flash3");		// Blaster Gun 3
				ent->genericBolt4 = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*flash4");		// Blaster Gun 4
				ent->genericBolt5 = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*flash5");		// Missile Gun 1
			}
			else if (!Q_stricmp( "mark2",modelName))
			{
				ent->headBolt = -1;				
				ent->handRBolt = ent->genericBolt1 = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*flash");		// Blaster Gun 1
			}
			else if (!Q_stricmp( "atst",modelName) )//&& (ent->client->playerTeam != TEAM_PLAYER))
			{
				ent->headBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*head");

				ent->handLBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*flash1");		// Front guns
				ent->handRBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*flash2");

				ent->genericBolt1 = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*flash3");		// Left side gun
				ent->genericBolt2 = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*flash4");		// Right side missle launcher

				ent->footLBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*l_foot");
				ent->footRBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*r_foot");
			}
			else if ( !Q_stricmp( "minemonster", modelName ))
			{
				ent->handRBolt = ent->headBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*head_f1");
			}
			else if ( !Q_stricmp( "rancor", modelName )
					|| !Q_stricmp( "mutant_rancor", modelName ))
			{
				ent->handLBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*l_hand");
				ent->handRBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*r_hand");
				ent->headBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*head_eyes");
				ent->gutBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*head_mouth");
			}
			else if ( !Q_stricmp( "sand_creature", modelName ))
			{
				ent->gutBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*mouth");
				ent->crotchBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*ground");
			}
			else if ( !Q_stricmp( "wampa", modelName ))
			{
				ent->headBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*head_eyes");
				ent->cervicalBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "neck_bone" );
				ent->chestBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "upper_spine");
				ent->gutBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "mid_spine");
				ent->torsoBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "lower_spine");
				ent->crotchBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "rear_bone");
				ent->elbowLBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*l_arm_elbow");
				ent->elbowRBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*r_arm_elbow");
				ent->handLBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*l_hand");
				ent->handRBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*r_hand");
				ent->kneeLBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*hips_l_knee");
				ent->kneeRBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*hips_r_knee");
				ent->footLBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*l_leg_foot");
				ent->footRBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*r_leg_foot");
			}
			else
			{//TEMP HACK: not a non-humanoid droid
				ent->handRBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*weapon");//should be r_hand
				if ( Q_stricmp( "atst", modelName ) )
				{//not an ATST
					ent->headBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*headg");
					ent->cervicalBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "cervical" );
					ent->torsoBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "lower_lumbar");
					ent->gutBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "upper_lumbar");
					ent->chestBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "thoracic");
					ent->crotchBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "pelvis");
					ent->elbowLBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*bicep_lg");
					ent->elbowRBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*bicep_rg");
					ent->handLBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*hand_l");
					ent->kneeLBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*thigh_lg");
					ent->kneeRBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*thigh_rg");
					ent->footLBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*foot_lg");
					ent->footRBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*foot_rg");
				}
			}
		}

		ent->faceBone = BONE_INDEX_INVALID;
		ent->craniumBone = BONE_INDEX_INVALID;
		ent->cervicalBone = BONE_INDEX_INVALID;
		ent->thoracicBone = BONE_INDEX_INVALID;
		ent->upperLumbarBone = BONE_INDEX_INVALID;
		ent->lowerLumbarBone = BONE_INDEX_INVALID;
		ent->motionBone = BONE_INDEX_INVALID;
		ent->hipsBone = BONE_INDEX_INVALID;
		ent->rootBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "model_root", qtrue );
#ifndef FINAL_BUILD
		if ( g_developer->integer && ent->rootBone == -1 )
		{
			Com_Error(ERR_DROP,"ERROR: model %s has no model_root bone (and hence cannot animate)!!!\n", modelName );
		}
#endif
		ent->footLBone = BONE_INDEX_INVALID;
		ent->footRBone = BONE_INDEX_INVALID;
		ent->humerusRBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "rhumerus", qtrue );

		// now add overrides on specific joints so the client can set angle overrides on the legs, torso and head
		if ( ent->client->NPC_class == CLASS_VEHICLE )
		{//do vehicles tags
			//vehicleInfo_t *vehicle = ent->m_pVehicle->m_pVehicleInfo;
		}
		else if ( ent->client->NPC_class == CLASS_HOWLER )
		{
		}
		else if ( !Q_stricmp( "gonk", modelName ) || !Q_stricmp( "seeker", modelName ) || !Q_stricmp( "remote", modelName ) )
		{//
		}		
		else if (!Q_stricmp( "sentry",modelName))
		{
		}
		else if (!Q_stricmpn( "probe", modelName, 5 ))
		{
			ent->craniumBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "cranium", qtrue );
			if (ent->craniumBone>=0)
			{
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->craniumBone, angles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, 0 ); 
			}
			ent->thoracicBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "pelvis", qtrue );
			if (ent->thoracicBone>=0)
			{
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->thoracicBone, angles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, 0 ); 
			}
		}
		else if (!Q_stricmp( "interrogator", modelName ))
		{
			ent->genericBone1 = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "left_arm", qtrue );
			if (ent->genericBone1>=0)
			{
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->genericBone1, angles, BONE_ANGLES_POSTMULT, NEGATIVE_Y, NEGATIVE_X, NEGATIVE_Z, NULL, 0, 0 ); 
			}
			ent->genericBone2 = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "right_arm", qtrue );
			if (ent->genericBone2>=0)
			{
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->genericBone2, angles, BONE_ANGLES_POSTMULT, NEGATIVE_Y, NEGATIVE_X, NEGATIVE_Z, NULL, 0, 0 ); 
			}
			ent->genericBone3 = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "claw", qtrue );
			if (ent->genericBone3>=0)
			{
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->genericBone3, angles, BONE_ANGLES_POSTMULT, NEGATIVE_Y, NEGATIVE_X, NEGATIVE_Z, NULL, 0, 0 ); 
			}
		}
		else if (!Q_stricmpn( "r2d2", modelName, 4 ))
		{
			ent->craniumBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "cranium", qtrue );
			if (ent->craniumBone>=0)
			{
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->craniumBone, angles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, 0 ); 
			}
			ent->thoracicBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "body", qtrue );
			if (ent->thoracicBone>=0)
			{
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->thoracicBone, angles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, 0 ); 
			}
			ent->genericBone1 = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "f_eye", qtrue );
			if (ent->genericBone1>=0)
			{
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->genericBone1, angles, BONE_ANGLES_POSTMULT, NEGATIVE_Y, NEGATIVE_X, NEGATIVE_Z, NULL, 0, 0 ); 
			}
		}
		else if (!Q_stricmpn( "r5d2", modelName, 4 ))
		{
			ent->craniumBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "cranium", qtrue );
			if (ent->craniumBone>=0)
			{
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->craniumBone, angles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, 0 ); 
			}
			ent->thoracicBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "body", qtrue );
			if (ent->thoracicBone>=0)
			{
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->thoracicBone, angles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, 0 ); 
			}
		}
		else if ( !Q_stricmp( "atst", modelName ))
		{
			ent->craniumBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "cranium", qtrue );
			if (ent->craniumBone>=0)
			{
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->craniumBone, angles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, 0 ); 
			}
			ent->thoracicBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "thoracic", qtrue );
			if (ent->thoracicBone>=0)
			{
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->thoracicBone, angles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, 0 ); 
			}
			ent->footLBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "l_tarsal", qtrue );
			if (ent->footLBone>=0)
			{
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->footLBone, angles, BONE_ANGLES_POSTMULT, POSITIVE_Z, NEGATIVE_Y, NEGATIVE_X, NULL, 0, 0 ); 
			}
			ent->footRBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "r_tarsal", qtrue );
			if (ent->footRBone>=0)
			{
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->footRBone, angles, BONE_ANGLES_POSTMULT, POSITIVE_Z, NEGATIVE_Y, NEGATIVE_X, NULL, 0, 0 ); 
			}
		}
		else if ( !Q_stricmp( "mark1", modelName ))
		{
			ent->craniumBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "cranium", qtrue );
			if (ent->craniumBone>=0)
			{
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->craniumBone, angles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, 0 ); 
			}
			ent->upperLumbarBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "cranium", qtrue );
			if (ent->upperLumbarBone>=0)
			{
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->upperLumbarBone, angles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, 0 ); 
			}
		}
		else if ( !Q_stricmp( "mark2", modelName ))
		{
			ent->craniumBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "cranium", qtrue );
			if (ent->craniumBone>=0)
			{
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->craniumBone, angles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, 0 ); 
			}
			ent->thoracicBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "thoracic", qtrue );
			if (ent->thoracicBone>=0)
			{
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->thoracicBone, angles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, 0 ); 
			}
		}
		else if ( !Q_stricmp( "minemonster", modelName ))
		{
			ent->thoracicBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "thoracic1", qtrue );
			if (ent->thoracicBone>=0)
			{
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->thoracicBone, angles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, 0 ); 
			}
			ent->craniumBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "cranium", qtrue );
			if (ent->craniumBone>=0)
			{
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->craniumBone, angles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, 0 ); 
			}
		}
		else if ( ent->client->NPC_class == CLASS_RANCOR )
			/*!Q_stricmp( "rancor", modelName )	|| !Q_stricmp( "mutant_rancor", modelName ) )*/
		{
			Eorientations oUp, oRt, oFwd;
			//regular bones we need
			ent->lowerLumbarBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "lower_spine", qtrue );
			if (ent->lowerLumbarBone>=0)
			{
				G_BoneOrientationsForClass( ent->client->NPC_class, "lower_lumbar", &oUp, &oRt, &oFwd );
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->lowerLumbarBone, angles, BONE_ANGLES_POSTMULT, oUp, oRt, oFwd, NULL, 0, 0 ); 
			}
			ent->upperLumbarBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "mid_spine", qtrue );
			if (ent->upperLumbarBone>=0)
			{
				G_BoneOrientationsForClass( ent->client->NPC_class, "upper_lumbar", &oUp, &oRt, &oFwd );
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->upperLumbarBone, angles, BONE_ANGLES_POSTMULT, oUp, oRt, oFwd, NULL, 0, 0 ); 
			}
			ent->thoracicBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "upper_spine", qtrue );
			if (ent->thoracicBone>=0)
			{
				G_BoneOrientationsForClass( ent->client->NPC_class, "thoracic", &oUp, &oRt, &oFwd );
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->thoracicBone, angles, BONE_ANGLES_POSTMULT, oUp, oRt, oFwd, NULL, 0, 0 ); 
			}
		}
		else if ( !Q_stricmp( "sand_creature", modelName ))
		{
		}
		else if ( !Q_stricmp( "wampa", modelName ) )
		{
			//Eorientations oUp, oRt, oFwd;
			//bone needed for turning anims
			/*
			//SIGH... fucks him up BAD
			ent->hipsBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "pelvis", qtrue );
			if (ent->hipsBone>=0)
			{
				G_BoneOrientationsForClass( ent->client->NPC_class, "pelvis", &oUp, &oRt, &oFwd );
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->hipsBone, angles, BONE_ANGLES_POSTMULT, oUp, oRt, oFwd, NULL ); 
			}
			*/
			/*
			//SIGH... no anim split
			ent->lowerLumbarBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "lower_lumbar", qtrue );
			if (ent->lowerLumbarBone>=0)
			{
				G_BoneOrientationsForClass( ent->client->NPC_class, "lower_lumbar", &oUp, &oRt, &oFwd );
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->lowerLumbarBone, angles, BONE_ANGLES_POSTMULT, oUp, oRt, oFwd, NULL ); 
			}
			*/
			/*
			//SIGH... spine wiggles fuck all this shit
			ent->upperLumbarBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "upper_lumbar", qtrue );
			if (ent->upperLumbarBone>=0)
			{
				G_BoneOrientationsForClass( ent->client->NPC_class, "upper_lumbar", &oUp, &oRt, &oFwd );
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->upperLumbarBone, angles, BONE_ANGLES_POSTMULT, oUp, oRt, oFwd, NULL ); 
			}
			ent->thoracicBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "thoracic", qtrue );
			if (ent->thoracicBone>=0)
			{
				G_BoneOrientationsForClass( ent->client->NPC_class, "thoracic", &oUp, &oRt, &oFwd );
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->thoracicBone, angles, BONE_ANGLES_POSTMULT, oUp, oRt, oFwd, NULL ); 
			}
			ent->cervicalBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "cervical", qtrue );
			if (ent->cervicalBone>=0)
			{
				G_BoneOrientationsForClass( ent->client->NPC_class, "cervical", &oUp, &oRt, &oFwd );
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->cervicalBone, angles, BONE_ANGLES_POSTMULT, oUp, oRt, oFwd, NULL ); 
			}
			ent->craniumBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "cranium", qtrue );
			if (ent->craniumBone>=0)
			{
				G_BoneOrientationsForClass( ent->client->NPC_class, "cranium", &oUp, &oRt, &oFwd );
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->craniumBone, angles, BONE_ANGLES_POSTMULT, oUp, oRt, oFwd, NULL ); 
			}
			*/
		}
		else if ( !Q_stricmp( "rockettrooper", modelName )
			|| !Q_stricmp( "hazardtrooper", modelName )
			|| !Q_stricmp( "saber_droid", modelName )
			|| !Q_stricmp( "assassin_droid", modelName ) )
		{
			Eorientations oUp, oRt, oFwd;
			if ( Q_stricmp( "saber_droid", modelName ) )
			{//saber droid doesn't use these lower bones
				//regular bones we need
				ent->upperLumbarBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "upper_lumbar", qtrue );
				if (ent->upperLumbarBone>=0)
				{
					G_BoneOrientationsForClass( ent->client->NPC_class, "upper_lumbar", &oUp, &oRt, &oFwd );
					gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->upperLumbarBone, angles, BONE_ANGLES_POSTMULT, oUp, oRt, oFwd, NULL, 0, 0 ); 
				}
				ent->lowerLumbarBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "lower_lumbar", qtrue );
				if (ent->lowerLumbarBone>=0)
				{
					G_BoneOrientationsForClass( ent->client->NPC_class, "lower_lumbar", &oUp, &oRt, &oFwd );
					gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->lowerLumbarBone, angles, BONE_ANGLES_POSTMULT, oUp, oRt, oFwd, NULL, 0, 0 ); 
				}
			}
			if ( Q_stricmp( "hazardtrooper", modelName ) )
			{//hazard trooper doesn't have these upper bones
				if ( Q_stricmp( "saber_droid", modelName ) )
				{//saber droid doesn't use thoracic bone
					ent->thoracicBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "thoracic", qtrue );
					if (ent->thoracicBone>=0)
					{
						G_BoneOrientationsForClass( ent->client->NPC_class, "thoracic", &oUp, &oRt, &oFwd );
						gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->thoracicBone, angles, BONE_ANGLES_POSTMULT, oUp, oRt, oFwd, NULL, 0, 0 ); 
					}
				}
				ent->cervicalBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "cervical", qtrue );
				if (ent->cervicalBone>=0)
				{
					G_BoneOrientationsForClass( ent->client->NPC_class, "cervical", &oUp, &oRt, &oFwd );
					gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->cervicalBone, angles, BONE_ANGLES_POSTMULT, oUp, oRt, oFwd, NULL, 0, 0 ); 
				}
				ent->craniumBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "cranium", qtrue );
				if (ent->craniumBone>=0)
				{
					G_BoneOrientationsForClass( ent->client->NPC_class, "cranium", &oUp, &oRt, &oFwd );
					gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->craniumBone, angles, BONE_ANGLES_POSTMULT, oUp, oRt, oFwd, NULL, 0, 0 ); 
				}
			}
		}
		else
		{
			//special case motion bone - to match up split anims
			ent->motionBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "Motion", qtrue );
			if (ent->motionBone>=0)
			{
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->motionBone, angles, BONE_ANGLES_POSTMULT, POSITIVE_Z, NEGATIVE_X, NEGATIVE_Y, NULL, 0, 0 ); 
			}
			ent->motionBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "Motion");
			//bone needed for turning anims
			ent->hipsBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "pelvis", qtrue );
			if (ent->hipsBone>=0)
			{
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->hipsBone, angles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, 0 ); 
			}
			//regular bones we need
			ent->upperLumbarBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "upper_lumbar", qtrue );
			if (ent->upperLumbarBone>=0)
			{
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->upperLumbarBone, angles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, 0 ); 
			}
			ent->lowerLumbarBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "lower_lumbar", qtrue );
			if (ent->lowerLumbarBone>=0)
			{
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->lowerLumbarBone, angles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, 0 ); 
			}

			ent->faceBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "face", qtrue );
			if (ent->faceBone>=0)
			{
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->faceBone, angles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, 0 ); 
			}
			ent->craniumBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "cranium", qtrue );
			if (ent->craniumBone>=0)
			{
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->craniumBone, angles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, 0 ); 
			}
			ent->cervicalBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "cervical", qtrue );
			if (ent->cervicalBone>=0)
			{
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->cervicalBone, angles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, 0 ); 
			}
			ent->thoracicBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "thoracic", qtrue );
			if (ent->thoracicBone>=0)
			{
				gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->thoracicBone, angles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, 0 ); 
			}
		}
		ent->client->clientInfo.infoValid = qtrue;

	}

	if ( ent->client->NPC_class == CLASS_SAND_CREATURE )
	{
		ent->s.radius = 256;
	}
	else if ( ent->client->NPC_class == CLASS_RANCOR )
	{
		if ( (ent->spawnflags&1) )
		{//mutant
			ent->s.radius = 300;
		}
		else
		{
			ent->s.radius = 150;
		}
	}
	else if ( ent->s.radius <= 0 )//radius cannot be negative or zero
	{//set the radius to be the largest axial distance on the entity
		float	max;
		max = ent->mins[0];//NOTE: mins is always negative
		if ( max > ent->mins[1] )
		{
			max = ent->mins[1];
		}

		if ( max > ent->mins[2] )
		{
			max = ent->mins[2];
		}

		max = fabs(max);//convert to positive to compare with maxs
		if ( max < ent->maxs[0] )
		{
			max = ent->maxs[0];
		}

		if ( max < ent->maxs[1] )
		{
			max = ent->maxs[1];
		}

		if ( max < ent->maxs[2] )
		{
			max = ent->maxs[2];
		}

		ent->s.radius = (int)max;

		if (!ent->s.radius)	// Still no radius?
		{
			ent->s.radius = 60;
		}
	}

	// set the weaponmodel to -1 so we don't try to remove it in Pmove before we have it built
	ent->weaponModel[0] = -1;

	if ( ent->playerModel == -1 )
	{
		return qfalse;
	}
	return qtrue;
}

void G_SetG2PlayerModel( gentity_t * const ent, const char *modelName, const char *customSkin, const char *surfOff, const char *surfOn )
{
	char	skinName[MAX_QPATH];

	//ok, lets register the skin name, and then pass that name to the config strings so the client can get it too.
	if ( !customSkin )
	{//use the default
		Com_sprintf( skinName, sizeof( skinName ), "models/players/%s/model_default.skin", modelName );
	}
	else
	{
		if (strchr(customSkin, '|'))
		{//three part skin
			Com_sprintf( skinName, sizeof( skinName ), "models/players/%s/|%s", modelName, customSkin );
		}
		else
		{
			Com_sprintf( skinName, sizeof( skinName ), "models/players/%s/model_%s.skin", modelName, customSkin );
		}
	}
	int skin = gi.RE_RegisterSkin( skinName );
	//now generate the ghoul2 model this client should be.
	if ( ent->client->NPC_class == CLASS_VEHICLE )
	{//vehicles actually grab their model from the appropriate vehicle data entry
		
		// This will register the model and other assets.
		Vehicle_t *pVeh = ent->m_pVehicle;
		pVeh->m_pVehicleInfo->RegisterAssets( pVeh );
		ent->playerModel = gi.G2API_InitGhoul2Model( ent->ghoul2, va("models/players/%s/model.glm", modelName), pVeh->m_pVehicleInfo->modelIndex, G_SkinIndex( skinName ), NULL_HANDLE, 0, 0 );
	}
	else
	{
		//NOTE: it still loads the default skin's tga's because they're referenced in the .glm.
		ent->playerModel = gi.G2API_InitGhoul2Model( ent->ghoul2, va("models/players/%s/model.glm", modelName), G_ModelIndex( va("models/players/%s/model.glm", modelName) ), G_SkinIndex( skinName ), NULL_HANDLE, 0, 0 );
	}
	if (ent->playerModel == -1)
	{//try the stormtrooper as a default
		gi.Printf( S_COLOR_RED"G_SetG2PlayerModel: cannot load model %s\n", modelName );
		modelName = "stormtrooper";
		Com_sprintf( skinName, sizeof( skinName ), "models/players/%s/model_default.skin", modelName );
		skin = gi.RE_RegisterSkin( skinName );
		ent->playerModel = gi.G2API_InitGhoul2Model( ent->ghoul2, va("models/players/%s/model.glm", modelName), G_ModelIndex( va("models/players/%s/model.glm", modelName) ), NULL_HANDLE, NULL_HANDLE, 0, 0 );
	}
	if (ent->playerModel == -1)
	{//very bad thing here!
		Com_Error(ERR_DROP, "Cannot fall back to default model %s!", modelName);
	}

	gi.G2API_SetSkin( &ent->ghoul2[ent->playerModel], G_SkinIndex( skinName ), skin );//this is going to set the surfs on/off matching the skin file

	// did we find a ghoul2 model? if so, load the animation.cfg file
	if ( !G_SetG2PlayerModelInfo( ent, modelName, customSkin, surfOff, surfOn ) )
	{//couldn't set g2 info, fall back to a mouse md3
		NPC_ParseParms( "mouse", ent );
		Com_Printf( S_COLOR_RED"couldn't load playerModel %s!\n", va("models/players/%s/model.glm", modelName) );
	}
}
/*
Ghoul2 Insert End
*/

void G_RemovePlayerModel( gentity_t *ent )
{
	if ( ent->playerModel >= 0 && ent->ghoul2.size() )
	{
		gi.G2API_RemoveGhoul2Model( ent->ghoul2, ent->playerModel );
		ent->playerModel = -1;
	}
}

void G_RemoveWeaponModels( gentity_t *ent )
{
	if ( ent->ghoul2.size() )
	{
		if ( ent->weaponModel[0] > 0 )
		{
			gi.G2API_RemoveGhoul2Model( ent->ghoul2, ent->weaponModel[0] );
			ent->weaponModel[0] = -1;
		}
		if ( ent->weaponModel[1] > 0 )
		{
			gi.G2API_RemoveGhoul2Model( ent->ghoul2, ent->weaponModel[1] );
			ent->weaponModel[1] = -1;
		}
	}
}

void G_AddWeaponModels( gentity_t *ent )
{
	if ( !ent || !ent->client )
	{
		return;
	}
	if ( ent->weaponModel[0] == -1 )
	{
		if ( ent->client->ps.weapon == WP_SABER )
		{
			WP_SaberAddG2SaberModels( ent );
		}
		else if ( ent->client->ps.weapon != WP_NONE )
		{
			G_CreateG2AttachedWeaponModel( ent, weaponData[ent->client->ps.weapon].weaponMdl, ent->handRBolt, 0 );
		}
	}
}

extern saber_colors_t TranslateSaberColor( const char *name );
extern void WP_RemoveSaber( gentity_t *ent, int saberNum );
void G_ChangePlayerModel( gentity_t *ent, const char *newModel );
void G_SetSabersFromCVars( gentity_t *ent )
{
	if ( g_saber->string
		&& g_saber->string[0]
		&& Q_stricmp( "none", g_saber->string )
		&& Q_stricmp( "NULL", g_saber->string ) )
	{//FIXME: how to specify second saber?
		WP_SaberParseParms( g_saber->string, &ent->client->ps.saber[0] );
		if ( ent->client->ps.saber[0].stylesLearned )
		{
			ent->client->ps.saberStylesKnown |= ent->client->ps.saber[0].stylesLearned;
		}
		if ( ent->client->ps.saber[0].singleBladeStyle )
		{
			ent->client->ps.saberStylesKnown |= ent->client->ps.saber[0].singleBladeStyle;
		}
	}

	if ( player 
		&& player->client 
		&& player->client->sess.mission_objectives[LIGHTSIDE_OBJ].status == 2 
		&& g_saberDarkSideSaberColor->integer )
	{//dark side!
		//always use red
		for ( int n = 0; n < MAX_BLADES; n++ )
		{
			ent->client->ps.saber[0].blade[n].color = SABER_RED;
		}
	}
	else if ( g_saber_color->string )
	{//FIXME: how to specify color for each blade and/or color for second saber?
		saber_colors_t color = TranslateSaberColor( g_saber_color->string );
		for ( int n = 0; n < MAX_BLADES; n++ )
		{
			ent->client->ps.saber[0].blade[n].color = color;
		}
	}
	if ( g_saber2->string 
		&& g_saber2->string[0]
		&& Q_stricmp( "none", g_saber2->string )
		&& Q_stricmp( "NULL", g_saber2->string ) )
	{
		if ( !(ent->client->ps.saber[0].saberFlags&SFL_TWO_HANDED) )
		{//can't use a second saber if first one is a two-handed saber...?
			WP_SaberParseParms( g_saber2->string, &ent->client->ps.saber[1] );
			if ( ent->client->ps.saber[1].stylesLearned )
			{
				ent->client->ps.saberStylesKnown |= ent->client->ps.saber[1].stylesLearned;
			}
			if ( ent->client->ps.saber[1].singleBladeStyle )
			{
				ent->client->ps.saberStylesKnown |= ent->client->ps.saber[1].singleBladeStyle;
			}
			if ( (ent->client->ps.saber[1].saberFlags&SFL_TWO_HANDED) )
			{//tsk tsk, can't use a twoHanded saber as second saber
				WP_RemoveSaber( ent, 1 );
			}
			else
			{
				ent->client->ps.dualSabers = qtrue;
				if ( player 
					&& player->client 
					&& player->client->sess.mission_objectives[LIGHTSIDE_OBJ].status == 2 
					&& g_saberDarkSideSaberColor->integer )
				{//dark side!
					//always use red
					for ( int n = 0; n < MAX_BLADES; n++ )
					{
						ent->client->ps.saber[1].blade[n].color = SABER_RED;
					}
				}
				else if ( g_saber2_color->string )
				{//FIXME: how to specify color for each blade and/or color for second saber?
					saber_colors_t color = TranslateSaberColor( g_saber2_color->string );
					for ( int n = 0; n < MAX_BLADES; n++ )
					{
						ent->client->ps.saber[1].blade[n].color = color;
					}
				}
			}
		}
	}
}

void G_InitPlayerFromCvars( gentity_t *ent )
{
	//set model based on cvars
	if(Q_stricmp(g_char_skin_head->string, "model_default") == 0 && Q_stricmp(g_char_skin_torso->string, "model_default") == 0 && Q_stricmp(g_char_skin_legs->string, "model_default") == 0)
		G_ChangePlayerModel( ent, va("%s|model_default", g_char_model->string) );
	else
		G_ChangePlayerModel( ent, va("%s|%s|%s|%s", g_char_model->string, g_char_skin_head->string, g_char_skin_torso->string, g_char_skin_legs->string) );

	//FIXME: parse these 2 from some cvar or require playermodel to be in a *.npc?
	if( ent->NPC_type && gi.bIsFromZone(ent->NPC_type, TAG_G_ALLOC) ) {
		gi.Free(ent->NPC_type);
	}

	// Bad casting I know, but NPC_type can also come the memory manager,
	// and you can't free a const-pointer later on. This seemed like the
	// better options.
	ent->NPC_type = (char *)"player";//default for now
	if( ent->client->clientInfo.customBasicSoundDir && gi.bIsFromZone(ent->client->clientInfo.customBasicSoundDir, TAG_G_ALLOC) ) {
		gi.Free(ent->client->clientInfo.customBasicSoundDir);
	}

	char snd[512];
	gi.Cvar_VariableStringBuffer( "snd", snd, sizeof(snd) );

	ent->client->clientInfo.customBasicSoundDir = G_NewString(snd);	//copy current cvar

	//set the lightsaber
	G_RemoveWeaponModels( ent );
	G_SetSabersFromCVars( ent );
	//set up weapon models, etc.
	G_AddWeaponModels( ent );
	NPC_SetAnim( ent, SETANIM_LEGS, ent->client->ps.legsAnim, SETANIM_FLAG_NORMAL|SETANIM_FLAG_RESTART );
	NPC_SetAnim( ent, SETANIM_TORSO, ent->client->ps.torsoAnim, SETANIM_FLAG_NORMAL|SETANIM_FLAG_RESTART );
	if ( !ent->s.number )
	{//the actual player, not an NPC pretending to be a player
		ClientUserinfoChanged( ent->s.number );
	}
	//color tinting
	//FIXME: the customRGBA shouldn't be set if the shader this guys .skin is using doesn't have the tinting on it
	if ( g_char_color_red->integer 
		|| g_char_color_green->integer
		|| g_char_color_blue->integer )
	{
		ent->client->renderInfo.customRGBA[0] = g_char_color_red->integer;
		ent->client->renderInfo.customRGBA[1] = g_char_color_green->integer;
		ent->client->renderInfo.customRGBA[2] = g_char_color_blue->integer;
		ent->client->renderInfo.customRGBA[3] = 255;
	}
}

void G_ChangePlayerModel( gentity_t *ent, const char *newModel )
{
	if ( !ent || !ent->client || !newModel )
	{
		return;
	}
	
	G_RemovePlayerModel( ent );
	if ( Q_stricmp( "player", newModel ) == 0 )
	{
		G_InitPlayerFromCvars( ent );
		return;
	}

	//attempt to free the string (currently can't since it's always "player" )
	if( ent->NPC_type && gi.bIsFromZone(ent->NPC_type, TAG_G_ALLOC) ) {
		gi.Free(ent->NPC_type);
	}
	ent->NPC_type = G_NewString( newModel );
	G_RemoveWeaponModels( ent );

	if ( strchr(newModel,'|') )
	{
		char name[MAX_QPATH];
		strcpy(name, newModel);
		char *p = strchr(name, '|');
		*p=0;
		p++;

		if ( strstr(p, "model_default" ) )
			G_SetG2PlayerModel( ent, name, NULL, NULL, NULL );
		else
			G_SetG2PlayerModel( ent, name, p, NULL, NULL );
	}
	else
	{
		//FIXME: everything but force powers gets reset, those should, too... 
		//		currently leaves them as is except where otherwise noted in the NPCs.cfg?
		//FIXME: remove all weapons?
		if ( NPC_ParseParms( ent->NPC_type, ent ) )
		{
			G_AddWeaponModels( ent );
			NPC_SetAnim( ent, SETANIM_LEGS, ent->client->ps.legsAnim, SETANIM_FLAG_NORMAL|SETANIM_FLAG_RESTART );
			NPC_SetAnim( ent, SETANIM_TORSO, ent->client->ps.torsoAnim, SETANIM_FLAG_NORMAL|SETANIM_FLAG_RESTART );
			ClientUserinfoChanged( ent->s.number );
			//Ugh, kind of a hack for now:
			if ( ent->client->NPC_class == CLASS_BOBAFETT 
				|| ent->client->NPC_class == CLASS_ROCKETTROOPER )
			{
				//FIXME: remove saber, too?
				Boba_Precache();	// player as boba?
			}
		}
		else
		{
			gi.Printf( S_COLOR_RED"G_ChangePlayerModel: cannot find NPC %s\n", newModel );
			G_ChangePlayerModel( ent, "stormtrooper" );	//need a better fallback?
		}
	}
}

void G_ReloadSaberData( gentity_t *ent )
{
	//dualSabers should already be set
	if ( ent->client->ps.saber[0].name != NULL )
	{
		WP_SaberParseParms( ent->client->ps.saber[0].name, &ent->client->ps.saber[0], qfalse );
		if ( ent->client->ps.saber[0].stylesLearned )
		{
			ent->client->ps.saberStylesKnown |= ent->client->ps.saber[0].stylesLearned;
		}
		if ( ent->client->ps.saber[0].singleBladeStyle )
		{
			ent->client->ps.saberStylesKnown |= ent->client->ps.saber[0].singleBladeStyle;
		}
	}
	if ( ent->client->ps.saber[1].name != NULL )
	{
		WP_SaberParseParms( ent->client->ps.saber[1].name, &ent->client->ps.saber[1], qfalse );
		if ( ent->client->ps.saber[1].stylesLearned )
		{
			ent->client->ps.saberStylesKnown |= ent->client->ps.saber[1].stylesLearned;
		}
		if ( ent->client->ps.saber[1].singleBladeStyle )
		{
			ent->client->ps.saberStylesKnown |= ent->client->ps.saber[1].singleBladeStyle;
		}
	}
}

qboolean G_PlayerSpawned( void )
{
	if ( !player
		|| !player->client 
		|| player->client->pers.teamState.state != TEAM_ACTIVE 
		|| level.time - player->client->pers.enterTime < 100 )
	{//player hasn't spawned yet
		return qfalse;
	}
	return qtrue;
}

/*
===========
ClientSpawn

Called every time a client is placed fresh in the world:
after the first ClientBegin, and after each respawn
Initializes all non-persistant parts of playerState
============
*/

qboolean G_CheckPlayerDarkSide( void )
{
	if ( player && player->client && player->client->sess.mission_objectives[LIGHTSIDE_OBJ].status == 2 )
	{//dark side player!
		player->client->playerTeam = TEAM_FREE;
		player->client->enemyTeam = TEAM_FREE;
		if ( g_saberDarkSideSaberColor->integer )
		{//dark side!
			//always use red
			for ( int n = 0; n < MAX_BLADES; n++ )
			{
				player->client->ps.saber[0].blade[n].color = player->client->ps.saber[1].blade[n].color = SABER_RED;
			}
		}
		G_SoundIndex( "sound/chars/jedi2/28je2008.wav" );
		G_SoundIndex( "sound/chars/jedi2/28je2009.wav" );
		G_SoundIndex( "sound/chars/jedi2/28je2012.wav" );
		return qtrue;
	}
	return qfalse;
}

void G_ChangePlayerModel( gentity_t *ent, const char *newModel );
qboolean ClientSpawn(gentity_t *ent, SavedGameJustLoaded_e eSavedGameJustLoaded ) 
{
	int		index;
	vec3_t	spawn_origin, spawn_angles;
	gclient_t	*client;
	int		i;
	clientPersistant_t	saved;
	clientSession_t		savedSess;
	clientInfo_t		savedCi;
	int		persistant[MAX_PERSISTANT];
	usercmd_t	ucmd;
	gentity_t	*spawnPoint;
	qboolean	beamInEffect = qfalse;
	extern qboolean g_qbLoadTransition;

	index = ent - g_entities;
	client = ent->client;

	if ( eSavedGameJustLoaded == eFULL && g_qbLoadTransition == qfalse )//qbFromSavedGame)
	{//loading up a full save game
		ent->client->pers.teamState.state = TEAM_ACTIVE;

		// increment the spawncount so the client will detect the respawn
		client->ps.persistant[PERS_SPAWN_COUNT]++;
		client->ps.persistant[PERS_TEAM] = client->sess.sessionTeam;

		client->airOutTime = level.time + 12000;

		for (i=0; i<3; i++)
		{
			ent->client->pers.cmd_angles[i] = 0.0f;
		}

		SetClientViewAngle( ent, ent->client->ps.viewangles);//spawn_angles );

		gi.linkentity (ent);

		// run the presend to set anything else
		ClientEndFrame( ent );

		// clear entity state values
		PlayerStateToEntityState( &client->ps, &ent->s );

		// ALL OF MY RAGE... they decided it would be a great idea to treat NPC_type like a player model here,
		// which is all kinds of unbelievable. I will be having a stern talk with James later. --eez
		if( ent->NPC_type &&
			Q_stricmp( ent->NPC_type, "player" ) )
		{
			// FIXME: game doesn't like it when you pass ent->NPC_type into this func. Insert all kinds of noises here --eez
			char bleh[1024];
			Q_strncpyz(bleh, ent->NPC_type, sizeof(bleh));

			G_ChangePlayerModel( ent, bleh );
		}
		else
		{
			G_LoadAnimFileSet( ent, ent->NPC_type );
			G_SetSkin( ent );
		}

		//setup sabers
		G_ReloadSaberData( ent );
		//force power levels should already be set
	}
	else
	{
		// find a spawn point
		// do it before setting health back up, so farthest
		// ranging doesn't count this client
		// don't spawn near existing origin if possible
		spawnPoint = SelectSpawnPoint ( ent->client->ps.origin, 
			(team_t) ent->client->ps.persistant[PERS_TEAM], spawn_origin, spawn_angles);

		ent->client->pers.teamState.state = TEAM_ACTIVE;

		// clear everything but the persistant data
		saved = client->pers;
		savedSess = client->sess;
		for ( i = 0 ; i < MAX_PERSISTANT ; i++ ) 
		{
			persistant[i] = client->ps.persistant[i];
		}
		//Preserve clientInfo
		memcpy (&savedCi, &client->clientInfo, sizeof(clientInfo_t));
		
		memset (client, 0, sizeof(*client));

		memcpy (&client->clientInfo, &savedCi, sizeof(clientInfo_t));

		client->pers = saved;
		client->sess = savedSess;
		for ( i = 0 ; i < MAX_PERSISTANT ; i++ ) 
		{
			client->ps.persistant[i] = persistant[i];
		}

		// increment the spawncount so the client will detect the respawn
		client->ps.persistant[PERS_SPAWN_COUNT]++;
		client->ps.persistant[PERS_TEAM] = client->sess.sessionTeam;

		client->airOutTime = level.time + 12000;

		// clear entity values
		client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;
		ent->s.groundEntityNum = ENTITYNUM_NONE;
		ent->client = &level.clients[index];
		ent->mass = 10;
		ent->takedamage = qtrue;
		ent->inuse = qtrue;
		SetInUse(ent);
		ent->m_iIcarusID = IIcarusInterface::ICARUS_INVALID;
		if ( !ent->NPC_type )
		{
			ent->NPC_type = (char *)"player";
		}
		ent->classname = "player";
		ent->targetname = ent->script_targetname = "player";
		if ( ent->client->NPC_class == CLASS_NONE )
		{
			ent->client->NPC_class = CLASS_PLAYER;
		}
		client->playerTeam = TEAM_PLAYER;
		client->enemyTeam = TEAM_ENEMY;
		ent->contents = CONTENTS_BODY;
		ent->clipmask = MASK_PLAYERSOLID;
		ent->e_DieFunc = dieF_player_die;
		ent->waterlevel = 0;
		ent->watertype = 0;
		client->ps.friction = 6;
		client->ps.gravity = g_gravity->value;
		ent->flags &= ~FL_NO_KNOCKBACK;
		client->renderInfo.lookTarget = ENTITYNUM_NONE;
		client->renderInfo.lookTargetClearTime = 0;
		client->renderInfo.lookMode = LM_ENT;

		VectorCopy (playerMins, ent->mins);
		VectorCopy (playerMaxs, ent->maxs);
		client->crouchheight = CROUCH_MAXS_2;
		client->standheight = DEFAULT_MAXS_2;

		client->ps.clientNum = index;

		// give default weapons
		//these are precached in g_items, ClearRegisteredItems()
		client->ps.stats[STAT_WEAPONS] = ( 1 << WP_NONE );
		//client->ps.inventory[INV_ELECTROBINOCULARS] = 1;
		//ent->client->ps.inventory[INV_BACTA_CANISTER] = 1;

		// give EITHER the saber or the stun baton..never both
		if ( spawnPoint->spawnflags & 32 ) // STUN_BATON
		{
			client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_STUN_BATON );
			client->ps.weapon = WP_STUN_BATON;
		}
		else
		{	// give the saber because most test maps will not have the STUN BATON flag set
			client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_SABER );	//this is precached in SP_info_player_deathmatch 
			client->ps.weapon = WP_SABER;
		}
		// force the base weapon up
		client->ps.weaponstate = WEAPON_READY;

		for ( i = FIRST_WEAPON; i < MAX_PLAYER_WEAPONS; i++ ) // don't give ammo for explosives
		{
			if ( (client->ps.stats[STAT_WEAPONS]&(1<<i)) )
			{//if starting with this weapon, gimme max ammo for it
				client->ps.ammo[weaponData[i].ammoIndex] = ammoData[weaponData[i].ammoIndex].max;
			}
		}

		if ( eSavedGameJustLoaded == eNO )
		{
			//FIXME: get player's info from NPCs.cfg
			client->ps.dualSabers = qfalse;
			WP_SaberParseParms( g_saber->string, &client->ps.saber[0] );//get saber info

			client->ps.saberStylesKnown |= (1<<gi.Cvar_VariableIntegerValue("g_fighting_style"));

//			if ( client->ps.saber[0].stylesLearned )
//			{
//				client->ps.saberStylesKnown |= client->ps.saber[0].stylesLearned;
//			}
//			if ( ent->client->ps.saber[1].singleSaberStyle )
//			{
//				ent->client->ps.saberStylesKnown |= ent->client->ps.saber[1].singleSaberStyle;
//			}
			WP_InitForcePowers( ent );//Initialize force powers
		}
		else
		{//autoload, will be taken care of below
		}
		//
		
		ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH];
		ent->client->dismemberProbHead = 0;
		ent->client->dismemberProbArms = 5;
		ent->client->dismemberProbHands = 20;
		ent->client->dismemberProbWaist = 0;
		ent->client->dismemberProbLegs = 0;

		ent->client->ps.batteryCharge = 2500;

		VectorCopy( spawn_origin, client->ps.origin );
		VectorCopy( spawn_origin, ent->currentOrigin );

		// the respawned flag will be cleared after the attack and jump keys come up
		client->ps.pm_flags |= PMF_RESPAWNED;

		SetClientViewAngle( ent, spawn_angles );

		G_KillBox( ent );
		gi.linkentity (ent);

		// don't allow full run speed for a bit
		client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		client->ps.pm_time = 100;

		client->respawnTime = level.time;
		client->latched_buttons = 0;

		// set default animations
		client->ps.torsoAnim = BOTH_STAND2;
		client->ps.legsAnim = BOTH_STAND2;

		//clear IK grabbing stuff
		client->ps.heldClient = client->ps.heldByClient = ENTITYNUM_NONE;
		client->ps.saberLockEnemy = ENTITYNUM_NONE;	//duh, don't think i'm locking with myself

		// restore some player data 
		//
		Player_RestoreFromPrevLevel(ent, eSavedGameJustLoaded);

		//FIXME: put this BEFORE the Player_RestoreFromPrevLevel check above?
		if (eSavedGameJustLoaded == eNO)
		{//fresh start
			if (!(spawnPoint->spawnflags&1))		// not KEEP_PREV
			{//then restore health and armor
				ent->health = client->ps.stats[STAT_ARMOR] = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH];
				ent->client->ps.forcePower = ent->client->ps.forcePowerMax;
			}
			G_InitPlayerFromCvars( ent );
		}
		else
		{//autoload
			if( ent->NPC_type &&
			Q_stricmp( ent->NPC_type, "player" ) )
			{
				// FIXME: game doesn't like it when you pass ent->NPC_type into this func. Insert all kinds of noises here --eez
				char bleh[1024];
				strncpy(bleh, ent->NPC_type, strlen(ent->NPC_type));
				bleh[strlen(ent->NPC_type)] = '\0';

				G_ChangePlayerModel( ent, bleh );
			}
			else
			{
				G_LoadAnimFileSet( ent, ent->NPC_type );
				G_SetSkin( ent );
			}
			G_ReloadSaberData( ent );
			//force power levels should already be set
		}

		//NEVER start a map with either of your sabers or blades on...
		ent->client->ps.SaberDeactivate();
		// run a client frame to drop exactly to the floor,
		// initialize animations and other things
		client->ps.commandTime = level.time - 100;
		ucmd = client->pers.lastCommand;
		ucmd.serverTime = level.time;
		_VectorCopy( client->pers.cmd_angles, ucmd.angles );
		ucmd.weapon = client->ps.weapon;	// client think calls Pmove which sets the client->ps.weapon to ucmd.weapon, so ...
		ent->client->ps.groundEntityNum = ENTITYNUM_NONE;
		ClientThink( ent-g_entities, &ucmd );

		// run the presend to set anything else
		ClientEndFrame( ent );

		// clear entity state values
		PlayerStateToEntityState( &client->ps, &ent->s );

		//ICARUS include
		Quake3Game()->FreeEntity( ent );
		Quake3Game()->InitEntity( ent );

		// Make sure no Sequencer exists then Get a new one.
		IIcarusInterface::GetIcarus()->DeleteIcarusID( ent->m_iIcarusID );
		ent->m_iIcarusID = IIcarusInterface::GetIcarus()->GetIcarusID( ent->s.number );

		if ( spawnPoint->spawnflags & 64 )	//NOWEAPON
		{//player starts with absolutely no weapons
			ent->client->ps.stats[STAT_WEAPONS] = ( 1 << WP_NONE );
			ent->client->ps.ammo[weaponData[WP_NONE].ammoIndex] = 32000;
			ent->client->ps.weapon = WP_NONE;
			ent->client->ps.weaponstate = WEAPON_READY;
			ent->client->ps.dualSabers = qfalse;
		}

		if ( ent->client->ps.stats[STAT_WEAPONS] & ( 1 << WP_SABER ) )
		{//set up so has lightsaber
			WP_SaberInitBladeData( ent );
			if ( (ent->weaponModel[0] <= 0 || (ent->weaponModel[1]<=0&&ent->client->ps.dualSabers)) //one or both of the saber models is not initialized
				&& ent->client->ps.weapon == WP_SABER )//current weapon is saber
			{//add the proper models
				WP_SaberAddG2SaberModels( ent );
			}
		}
		if ( ent->weaponModel[0] == -1 && ent->client->ps.weapon != WP_NONE )
		{
			G_CreateG2AttachedWeaponModel( ent, weaponData[ent->client->ps.weapon].weaponMdl, ent->handRBolt, 0 );
		}

		{
			// fire the targets of the spawn point
			G_UseTargets( spawnPoint, ent );
			//Designers needed them to fire off target2's as well... this is kind of messy
			G_UseTargets2( spawnPoint, ent, spawnPoint->target2 );

			/*
			// select the highest weapon number available, after any
			// spawn given items have fired
			client->ps.weapon = 1;
			for ( i = WP_NUM_WEAPONS - 1 ; i > 0 ; i-- ) {
				if ( client->ps.stats[STAT_WEAPONS] & ( 1 << i ) ) {
					client->ps.weapon = i;
					break;
				}
			}*/
		}
	}

	client->pers.enterTime = level.time;//needed mainly to stop the weapon switch to WP_NONE that happens on loads
	ent->max_health = client->ps.stats[STAT_MAX_HEALTH];

	if ( eSavedGameJustLoaded == eNO )
	{//on map transitions, Ghoul2 frame gets reset to zero, restart our anim
		NPC_SetAnim( ent, SETANIM_LEGS, ent->client->ps.legsAnim, SETANIM_FLAG_NORMAL|SETANIM_FLAG_RESTART );
		NPC_SetAnim( ent, SETANIM_TORSO, ent->client->ps.torsoAnim, SETANIM_FLAG_NORMAL|SETANIM_FLAG_RESTART );
	}

	if ( ent->s.number == 0 )
	{//player
		G_CheckPlayerDarkSide();
	}

	if ( (ent->client->ps.stats[STAT_WEAPONS]&(1<<WP_SABER))
		&& !ent->client->ps.saberStylesKnown )
	{//um, if you have a saber, you need at least 1 style to use it with...
		ent->client->ps.saberStylesKnown |= (1<<SS_MEDIUM);
	}

	return beamInEffect;
}


/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.
============
*/
void ClientDisconnect( int clientNum ) {
	gentity_t	*ent;

	ent = g_entities + clientNum;
	if ( !ent->client ) {
		return;
	}

	// send effect if they were completely connected
/*	if ( ent->client->pers.connected == CON_CONNECTED ) {
		// They don't get to take powerups with them!
		// Especially important for stuff like CTF flags
		TossClientItems ( ent );
	}
*/
	gi.unlinkentity (ent);
	ent->s.modelindex = 0;
	ent->inuse = qfalse;
	ClearInUse(ent);
	ent->classname = "disconnected";
	ent->client->pers.connected = CON_DISCONNECTED;
	ent->client->ps.persistant[PERS_TEAM] = TEAM_FREE;

	gi.SetConfigstring( CS_PLAYERS + clientNum, "");

	IIcarusInterface::GetIcarus()->DeleteIcarusID(ent->m_iIcarusID);

}


																  
