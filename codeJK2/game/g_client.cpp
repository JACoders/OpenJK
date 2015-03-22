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
#include "anims.h"
#include "g_icarus.h"
#include "wp_saber.h"

extern void Q3_DebugPrint( int level, const char *format, ... );
extern void WP_SaberInitBladeData( gentity_t *ent );
extern void G_CreateG2AttachedWeaponModel( gentity_t *ent, const char *weaponModel );
extern qboolean	CheatsOk( gentity_t *ent );
extern vmCvar_t	cg_thirdPersonAlpha;

// g_client.c -- client functions that don't happen every frame

float DEFAULT_MINS_0 = -16;
float DEFAULT_MINS_1 = -16;
float DEFAULT_MAXS_0 = 16;
float DEFAULT_MAXS_1 = 16;
float DEFAULT_PLAYER_RADIUS	= sqrt((DEFAULT_MAXS_0*DEFAULT_MAXS_0) + (DEFAULT_MAXS_1*DEFAULT_MAXS_1));
vec3_t playerMins = {DEFAULT_MINS_0, DEFAULT_MINS_1, DEFAULT_MINS_2};
vec3_t playerMaxs = {DEFAULT_MAXS_0, DEFAULT_MAXS_1, DEFAULT_MAXS_2};

void SP_misc_teleporter_dest (gentity_t *ent);

/*QUAK-ED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 32) - - NODRAW
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
		G_SkinIndex("models/players/kyle/model_fpls2.skin");	//preache the skin used in cg_players.cpp
	}
}

/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32) KEEP_PREV DROPTOFLOOR x x x STUN_BATON NOWEAPON x
KEEP_PREV - keep previous health/ammo/etc
DROPTOFLOOR - Player will start on the first solid structure under it
STUN_BATON - Gives player the stun baton and bryar pistol, but not the saber, plus any weapons they may have carried over from previous levels.

Targets will be fired when someone spawns in on them.
equivalant to info_player_deathmatch
*/
void SP_info_player_start(gentity_t *ent) {
	ent->classname = "info_player_deathmatch";

	ent->spawnflags |= 1;	// James suggests force-ORing the KEEP_PREV flag in for now

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
			G_Error( "Couldn't find spawntarget %s", level.spawntarget );
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
		G_Error( "Couldn't find a spawn point" );
	}


	VectorCopy( spot->s.origin, origin );
	if ( spot->spawnflags & 2 )
	{
		trace_t		tr;

		origin[2] = MIN_WORLD_COORD;
		gi.trace(&tr, spot->s.origin, playerMins, playerMaxs, origin, ENTITYNUM_NONE, MASK_PLAYERSOLID, G2_NOCOLLIDE, 0 );
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
================
PickTeam

================
*/
team_t PickTeam( int ignoreClientNum ) {
	int		i;
	int		counts[TEAM_NUM_TEAMS];

	memset( counts, 0, sizeof( counts ) );

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( i == ignoreClientNum ) {
			continue;
		}
		if ( level.clients[i].pers.connected == CON_DISCONNECTED ) {
			continue;
		}
	}

	return TEAM_FREE;
}

/*
===========
ForceClientSkin

Forces a client's skin (for teamplay)
===========
*/
void ForceClientSkin( gclient_t *client, char *model, const char *skin ) {
	char *p;

	if ((p = strchr(model, '/')) != NULL) {
		*p = 0;
	}

	Q_strcat(model, MAX_QPATH, "/");
	Q_strcat(model, MAX_QPATH, skin);
}

/*
===========
ClientCheckName
============
*/
static void ClientCleanName( const char *in, char *out, int outSize )
{
	int outpos = 0, colorlessLen = 0, spaces = 0;

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
				spaces = 0;
				colorlessLen++;
			}
		}
		else
		{
			spaces = 0;
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
	const char	*s=NULL, *sex=NULL;
	char		userinfo[MAX_INFO_STRING]={0},	buf[MAX_INFO_STRING]={0},
				oldname[34]={0};

	gi.GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

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

	// sex
	sex = Info_ValueForKey( userinfo, "sex" );
	if ( !sex[0] ) {
		sex = "m";
	}

	// send over a subset of the userinfo keys so other clients can
	// print scoreboards, display models, and play custom sounds
	buf[0] = '\0';
	Q_strcat( buf, sizeof( buf ), va( "n\\%s\\", client->pers.netname ) );
	Q_strcat( buf, sizeof( buf ), va( "t\\%i\\", client->sess.sessionTeam ) );
	Q_strcat( buf, sizeof( buf ),	  "headModel\\\\" );
	Q_strcat( buf, sizeof( buf ),	  "torsoModel\\\\" );
	Q_strcat( buf, sizeof( buf ),	  "legsModel\\\\" );
	Q_strcat( buf, sizeof( buf ), va( "sex\\%s\\", sex ) );
	Q_strcat( buf, sizeof( buf ), va( "hc\\%i\\", client->pers.maxHealth ) );

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
		G_InitGentity( ent );
		ent->e_TouchFunc = touchF_NULL;
		ent->e_PainFunc  = painF_PlayerPain;//painF_NULL;
		ent->client = client;

		client->pers.connected = CON_CONNECTED;
		client->pers.teamState.state = TEAM_BEGIN;
		_VectorCopy( cmd->angles, client->pers.cmd_angles );

		memset( &client->ps, 0, sizeof( client->ps ) );
		memset( &client->sess.missionStats, 0, sizeof( client->sess.missionStats ) );
		client->sess.missionStats.totalSecrets = gi.Cvar_VariableIntegerValue("newTotalSecrets");

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
	int		i;

	gi.Cvar_VariableStringBuffer( sCVARNAME_PLAYERSAVE, s, sizeof(s) );

	if (strlen(s))	// actually this would be safe anyway because of the way sscanf() works, but this is clearer
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
void Player_RestoreFromPrevLevel(gentity_t *ent)
{
	gclient_t	*client = ent->client;
	int			i;

	assert(client);
	if (client)	// though I can't see it not being true...
	{
		char	s[MAX_STRING_CHARS];
		const char	*var;

		gi.Cvar_VariableStringBuffer( sCVARNAME_PLAYERSAVE, s, sizeof(s) );

		if (strlen(s))	// actually this would be safe anyway because of the way sscanf() works, but this is clearer
		{
			sscanf( s, "%i %i %i %i %i %i %i %f %f %f %i %i %i %i %i %i",
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
								&client->ps.forcePowersKnown,
								&client->ps.forcePower,
								&client->ps.saberActive,
								&client->ps.saberAnimLevel,
								&client->ps.saberLockEnemy,
								&client->ps.saberLockTime
					);
			ent->health = client->ps.stats[STAT_HEALTH];

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

			client->ps.forcePowerMax = FORCE_POWER_MAX;
			client->ps.forceGripEntityNum = ENTITYNUM_NONE;
		}
	}
}

/*
Ghoul2 Insert Start
*/
void G_SetSkin( gentity_t *ent, const char *modelName, const char *customSkin )
{
	char	skinName[MAX_QPATH];
	//ok, lets register the skin name, and then pass that name to the config strings so the client can get it too.
	//FIXME: is have an alternate skin (in modelName, after '/'), replace "default" with that skin name
	if ( !customSkin )
	{
		Com_sprintf( skinName, sizeof( skinName ), "models/players/%s/model_default.skin", modelName );
	}
	else
	{
		Com_sprintf( skinName, sizeof( skinName ), "models/players/%s/model_%s.skin", modelName, customSkin );
	}
	// lets see if it's out there
	int skin = gi.RE_RegisterSkin( skinName );
	if ( skin )
	{
		// put it in the config strings
		// and set the ghoul2 model to use it
		gi.G2API_SetSkin( &ent->ghoul2[ent->playerModel], G_SkinIndex( skinName ), skin );
	}
}

qboolean G_StandardHumanoid( const char *modelName )
{
	if ( !modelName )
	{
		return qfalse;
	}
	if ( !Q_stricmp( "kyle", modelName ) ||
		!Q_strncmp( "st", modelName, 2 ) ||
		!Q_strncmp( "imp", modelName, 3 ) ||
		!Q_strncmp( "gran", modelName, 4 ) ||
		!Q_strncmp( "rodian", modelName, 6 ) ||
		!Q_strncmp( "weequay", modelName, 7 ) ||
		!Q_strncmp( "reborn", modelName, 6 ) ||
		!Q_strncmp( "shadowtrooper", modelName, 13 ) ||
		!Q_strncmp( "swamptrooper", modelName, 12 ) ||
		!Q_stricmp( "rockettrooper", modelName ) ||
		!Q_stricmp( "bespin_cop", modelName ) ||
		!Q_strncmp( "bespincop", modelName, 9 ) ||
		!Q_strncmp( "rebel", modelName, 5 ) ||
		!Q_strncmp( "ugnaught", modelName, 8 ) ||
		!Q_strncmp( "morgan", modelName,6 ) ||
		!Q_strncmp( "protocol", modelName, 8 ) ||
		!Q_strncmp( "jedi", modelName, 4 ) ||
		!Q_strncmp( "prisoner", modelName, 8 ) ||
		!Q_stricmp( "tavion", modelName ) ||
		!Q_stricmp( "desann", modelName ) ||
		!Q_stricmp( "trandoshan", modelName ) ||
		!Q_stricmp( "jan", modelName ) ||
		!Q_stricmp( "luke", modelName ) ||
		!Q_stricmp( "lando", modelName ) ||
		!Q_stricmp( "reelo", modelName ) ||
		!Q_stricmp( "bartender", modelName ) ||
		!Q_stricmp( "monmothma", modelName ) ||
		!Q_stricmp( "chiss", modelName ) ||
		!Q_stricmp( "galak", modelName ) )
	{
		return qtrue;
	}
	return qfalse;
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
		//we shouldn't actually have to do this anymore
		//G_SetSkin( ent, modelName, customSkin );

		ent->headBolt = ent->cervicalBolt = ent->torsoBolt = ent->gutBolt = ent->chestBolt =
			ent->crotchBolt = ent->elbowLBolt = ent->elbowRBolt = ent->handLBolt =
			ent->handRBolt = ent->kneeLBolt = ent->kneeRBolt = ent->footLBolt =
			ent->footRBolt = -1;
		// now turn on the bolt in the hand - this one would be best always turned on.
		if ( G_StandardHumanoid( modelName ) )
		{//temp hack because only the gran are set up right so far
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
		}
		else
		{
			if ( !Q_stricmp( "gonk", modelName ) || !Q_stricmp( "seeker", modelName ) || !Q_stricmp( "remote", modelName )
						|| !Q_strncmp( "r2d2", modelName, 4 ) || !Q_strncmp( "r5d2", modelName, 4 ) )
			{//TEMP HACK: not a non-humanoid droid
				ent->headBolt = -1;
			}
			else if (!Q_stricmp( "interrogator",modelName))
			{
				ent->headBolt = -1;
			}
			else if (!Q_strncmp( "probe",modelName, 5))
			{
				ent->headBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "cranium");		// head pivot point
				ent->genericBolt1 = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*flash");		// Gun 1
			}
			/*
			else if (!Q_strncmp( "protocol",modelName, 8))
			{
				ent->headBolt = -1;
			}
			*/
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
			else if ( !Q_stricmp( "howler", modelName ))
			{
				ent->headBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "cranium"); // FIXME!
			}
			else if ( !Q_stricmp( "galak_mech", modelName ))
			{
				ent->genericBolt1 = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*antenna_effect");
				ent->headBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*head_eyes");
				ent->torsoBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "torso");
				ent->crotchBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "hips");
				ent->handRBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*flasha");
				ent->genericBolt3 = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*flashb");
				ent->handLBolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*flashc");
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
		ent->footLBone = BONE_INDEX_INVALID;
		ent->footRBone = BONE_INDEX_INVALID;
		// now add overrides on specific joints so the client can set angle overrides on the legs, torso and head
		if ( !Q_stricmp( "gonk", modelName ) || !Q_stricmp( "seeker", modelName ) || !Q_stricmp( "remote", modelName ) )
		{//
		}
		else if (!Q_stricmp( "sentry",modelName))
		{
		}
		else if (!Q_strncmp( "probe", modelName, 5 ))
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
		/*
		else if (!Q_strncmp( "protocol",modelName,8))
		{
		}
		*/
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
		else if (!Q_strncmp( "r2d2", modelName, 4 ))
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
		else if (!Q_strncmp( "r5d2", modelName, 4 ))
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
		else if ( !Q_stricmp( "howler", modelName ))
		{
			ent->thoracicBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "thoracic", qtrue );
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

	int		max;
	if ( ent->s.radius <= 0 )//radius cannot be negative or zero
	{//set the radius to be the largest axial distance on the entity
		max = ent->mins[0];//NOTE: mins is always negative
		if ( max > ent->mins[1] )
		{
			max = ent->mins[1];
		}

		if ( max > ent->mins[2] )
		{
			max = ent->mins[2];
		}

		max = fabs((double)max);//convert to positive to compare with maxs
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

		ent->s.radius = max;

		if (!ent->s.radius)	// Still no radius?
		{
			ent->s.radius = 60;
		}
	}

	// set the weaponmodel to -1 so we don't try and remove it in Pmove before we have it built
	ent->weaponModel = -1;

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
		Com_sprintf( skinName, sizeof( skinName ), "models/players/%s/model_%s.skin", modelName, customSkin );
	}
	gi.RE_RegisterSkin( skinName );
	//now generate the ghoul2 model this client should be.
	//NOTE: for some reason, it still loads the default skin's tga's?  Because they're referenced in the .glm?
	ent->playerModel = gi.G2API_InitGhoul2Model( ent->ghoul2, va("models/players/%s/model.glm", modelName),
		G_ModelIndex( va("models/players/%s/model.glm", modelName) ), G_SkinIndex( skinName ), NULL_HANDLE, 0, 0 );
	if (ent->playerModel == -1)
	{//try the stormtrooper as a default
		modelName = "stormtrooper";
		ent->playerModel = gi.G2API_InitGhoul2Model( ent->ghoul2, va("models/players/%s/model.glm", modelName),
			G_ModelIndex( va("models/players/%s/model.glm", modelName) ), NULL_HANDLE, NULL_HANDLE, 0, 0 );
	}

	if ( !Q_stricmp( "kyle", modelName ))
	{
		// Try to get the skin we'll use when we switch to the first person light saber.
		//	We use a new skin to disable certain surfaces so they are not drawn but we can still collide against them
		int skin = gi.RE_RegisterSkin( "models/players/kyle/model_fpls.skin" );
		if ( skin )
		{
			// put it in the config strings
			G_SkinIndex( skinName );
		}
	}

	// did we find a ghoul2 model? if so, load the animation.cfg file
	if ( !G_SetG2PlayerModelInfo( ent, modelName, customSkin, surfOff, surfOn ) )
	{//couldn't set g2 info, fall back to a mouse md3
		NPC_ParseParms( "mouse", ent );
		//Com_Error( ERR_DROP, "couldn't load playerModel %s!\n", va("models/players/%s/model.glm", modelName) );
		Com_Printf( S_COLOR_RED"couldn't load playerModel %s!\n", va("models/players/%s/model.glm", modelName) );
	}

}
/*
Ghoul2 Insert End
*/

void G_ActivatePersonalShield( gentity_t *ent )
{
	ent->client->ps.stats[STAT_ARMOR] = 100;//FIXME: define?
	ent->client->ps.powerups[PW_BATTLESUIT] = Q3_INFINITE;//Doesn't go away until armor does
}

//HACK FOR FLYING
extern void CG_ChangeWeapon( int num );
void G_PilotXWing( gentity_t *ent )
{
	if ( !CheatsOk( ent ) )
	{
		return;
	}
	if ( ent->client->ps.vehicleModel != 0 )
	{
		CG_ChangeWeapon( WP_SABER );
		ent->client->ps.vehicleModel = 0;
		ent->svFlags &= ~SVF_CUSTOM_GRAVITY;
		ent->client->ps.stats[STAT_ARMOR] = 0;//HACK
		//ent->mass = 10;
		//gi.cvar_set( "m_pitchOverride", "0" );
		//gi.cvar_set( "m_yawOverride", "0" );
		if ( ent->client->ps.weapon != WP_SABER )
		{
			gi.cvar_set( "cg_thirdperson", "0" );
		}
		cg.overrides.active |= CG_OVERRIDE_3RD_PERSON_RNG;
		cg.overrides.thirdPersonRange = 240;
		cg.overrides.active &= ~CG_OVERRIDE_FOV;
		cg.overrides.fov = 0;
	}
	else
	{
		ent->client->ps.vehicleModel = G_ModelIndex( "models/map_objects/ships/x_wing.md3" );

		ent->client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_ATST_SIDE );
		ent->client->ps.ammo[weaponData[WP_ATST_SIDE].ammoIndex] = ammoData[weaponData[WP_ATST_SIDE].ammoIndex].max;
		gitem_t *item = FindItemForWeapon( WP_ATST_SIDE );
		RegisterItem( item );	//make sure the weapon is cached in case this runs at startup
		G_AddEvent( ent, EV_ITEM_PICKUP, (item - bg_itemlist) );
		CG_ChangeWeapon( WP_ATST_SIDE );

		ent->client->ps.gravity = 0;
		ent->svFlags |= SVF_CUSTOM_GRAVITY;
		ent->client->ps.stats[STAT_ARMOR] = 200;//FIXME: define?
		//ent->mass = 300;
		ent->client->ps.speed = 0;
		//gi.cvar_set( "m_pitchOverride", "0.01" );//ignore inverse mouse look
		//gi.cvar_set( "m_yawOverride", "0.0075" );
		gi.cvar_set( "cg_thirdperson", "1" );
		cg.overrides.active |= (CG_OVERRIDE_3RD_PERSON_RNG|CG_OVERRIDE_FOV);
		cg.overrides.thirdPersonRange = 240;
		cg.overrides.fov = 100;
	}
}
//HACK FOR FLYING

//HACK FOR ATST
void G_DrivableATSTDie( gentity_t *self )
{
}

void G_DriveATST( gentity_t *ent, gentity_t *atst )
{
	if ( ent->NPC_type && ent->client && (ent->client->NPC_class == CLASS_ATST) )
	{//already an atst, switch back
		//open hatch
		if ( ent->playerModel >= 0 )
		{
			gi.G2API_RemoveGhoul2Model( ent->ghoul2, ent->playerModel );
		}
		ent->NPC_type = "kyle";
		ent->client->NPC_class = CLASS_KYLE;
		ent->flags &= ~FL_SHIELDED;
		ent->client->ps.eFlags &= ~EF_IN_ATST;
		//size
		VectorCopy( playerMins, ent->mins );
		VectorCopy( playerMaxs, ent->maxs );
		ent->client->crouchheight = CROUCH_MAXS_2;
		ent->client->standheight = DEFAULT_MAXS_2;
		G_SetG2PlayerModel( ent, "kyle", NULL, NULL, NULL );
		//FIXME: reset/initialize their weapon
		ent->client->ps.stats[STAT_WEAPONS] &= ~(( 1 << WP_ATST_MAIN )|( 1 << WP_ATST_SIDE ));
		ent->client->ps.ammo[weaponData[WP_ATST_MAIN].ammoIndex] = 0;
		ent->client->ps.ammo[weaponData[WP_ATST_SIDE].ammoIndex] = 0;
		CG_ChangeWeapon( WP_BRYAR_PISTOL );
		//camera
		//if ( ent->client->ps.weapon != WP_SABER )
		{
			gi.cvar_set( "cg_thirdperson", "0" );
		}
		cg.overrides.active &= ~(CG_OVERRIDE_3RD_PERSON_RNG|CG_OVERRIDE_3RD_PERSON_VOF|CG_OVERRIDE_3RD_PERSON_POF|CG_OVERRIDE_3RD_PERSON_APH);
		cg.overrides.thirdPersonRange = cg.overrides.thirdPersonVertOffset = cg.overrides.thirdPersonPitchOffset = 0;
		cg.overrides.thirdPersonAlpha = cg_thirdPersonAlpha.value;
		ent->client->ps.viewheight = ent->maxs[2] + STANDARD_VIEWHEIGHT_OFFSET;
		//ent->mass = 10;
	}
	else
	{//become an atst
		ent->NPC_type = "atst";
		ent->client->NPC_class = CLASS_ATST;
		ent->client->ps.eFlags |= EF_IN_ATST;
		ent->flags |= FL_SHIELDED;
		//size
		VectorSet( ent->mins, ATST_MINS0, ATST_MINS1, ATST_MINS2 );
		VectorSet( ent->maxs, ATST_MAXS0, ATST_MAXS1, ATST_MAXS2 );
		ent->client->crouchheight = ATST_MAXS2;
		ent->client->standheight = ATST_MAXS2;
		if ( ent->playerModel >= 0 )
		{
			gi.G2API_RemoveGhoul2Model( ent->ghoul2, ent->playerModel );
			ent->playerModel = -1;
		}
		if ( ent->weaponModel >= 0 )
		{
			gi.G2API_RemoveGhoul2Model( ent->ghoul2, ent->weaponModel );
			ent->weaponModel = -1;
		}
		if ( !atst )
		{//no ent to copy from
			G_SetG2PlayerModel( ent, "atst", NULL, NULL, NULL );
			NPC_SetAnim( ent, SETANIM_BOTH, BOTH_STAND1, SETANIM_FLAG_OVERRIDE );
		}
		else
		{
			gi.G2API_CopyGhoul2Instance( atst->ghoul2, ent->ghoul2, -1 );
			ent->playerModel = 0;
			G_SetG2PlayerModelInfo( ent, "atst", NULL, NULL, NULL );
			//turn off hatch underside
			gi.G2API_SetSurfaceOnOff( &ent->ghoul2[ent->playerModel], "head_hatchcover_off", 0x00000002/*G2SURFACEFLAG_OFF*/ );
			G_Sound( ent, G_SoundIndex( "sound/chars/atst/atst_hatch_close" ));
		}
		ent->s.radius = 320;
		//weapon
		gitem_t	*item = FindItemForWeapon( WP_ATST_MAIN );	//precache the weapon
		CG_RegisterItemSounds( (item-bg_itemlist) );
		CG_RegisterItemVisuals( (item-bg_itemlist) );
		item = FindItemForWeapon( WP_ATST_SIDE );	//precache the weapon
		CG_RegisterItemSounds( (item-bg_itemlist) );
		CG_RegisterItemVisuals( (item-bg_itemlist) );
		ent->client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_ATST_MAIN )|( 1 << WP_ATST_SIDE );
		ent->client->ps.ammo[weaponData[WP_ATST_MAIN].ammoIndex] = ammoData[weaponData[WP_ATST_MAIN].ammoIndex].max;
		ent->client->ps.ammo[weaponData[WP_ATST_SIDE].ammoIndex] = ammoData[weaponData[WP_ATST_SIDE].ammoIndex].max;
		CG_ChangeWeapon( WP_ATST_MAIN );
		//HACKHACKHACKTEMP
		item = FindItemForWeapon( WP_EMPLACED_GUN );
		CG_RegisterItemSounds( (item-bg_itemlist) );
		CG_RegisterItemVisuals( (item-bg_itemlist) );
		item = FindItemForWeapon( WP_ROCKET_LAUNCHER );
		CG_RegisterItemSounds( (item-bg_itemlist) );
		CG_RegisterItemVisuals( (item-bg_itemlist) );
		item = FindItemForWeapon( WP_BOWCASTER );
		CG_RegisterItemSounds( (item-bg_itemlist) );
		CG_RegisterItemVisuals( (item-bg_itemlist) );
		//HACKHACKHACKTEMP
		//FIXME: these get lost in load/save!  Must use variables that are set every frame or saved/loaded
		//camera
		gi.cvar_set( "cg_thirdperson", "1" );
		cg.overrides.active |= CG_OVERRIDE_3RD_PERSON_RNG;
		cg.overrides.thirdPersonRange = 240;
		//cg.overrides.thirdPersonVertOffset = 100;
		//cg.overrides.thirdPersonPitchOffset = -30;
		//FIXME: this gets stomped in pmove?
		ent->client->ps.viewheight = 120;
		//FIXME: setting these broke things very badly...?
		//ent->client->standheight = 200;
		//ent->client->crouchheight = 200;
		//ent->mass = 300;
		//movement
		//ent->client->ps.speed = 0;//FIXME: override speed?
		//FIXME: slow turn turning/can't turn if not moving?
	}
}
//HACK FOR ATST

/*
===========
ClientSpawn

Called every time a client is placed fresh in the world:
after the first ClientBegin, and after each respawn
Initializes all non-persistant parts of playerState
============
*/

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
	{
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

		if ( ent->client->NPC_class == CLASS_ATST )
		{
			G_LoadAnimFileSet( ent, "atst" );
			G_SetSkin( ent, "atst", NULL );
		}
		else
		{
			G_LoadAnimFileSet( ent, "kyle" );
			G_SetSkin( ent, "kyle", NULL );
		}
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
		ent->classname = "player";
		client->squadname = ent->targetname = ent->script_targetname = ent->NPC_type = "kyle";
		if ( ent->client->NPC_class == CLASS_NONE )
		{
			ent->client->NPC_class = CLASS_KYLE;
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
		client->ps.stats[STAT_WEAPONS] = ( 1 << WP_NONE );
		client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_BRYAR_PISTOL );	//these are precached in g_items, ClearRegisteredItems()
		client->ps.inventory[INV_ELECTROBINOCULARS] = 1;

		// always give the bryar pistol, but we have to give EITHER the saber or the stun baton..never both
		if ( spawnPoint->spawnflags & 32 ) // STUN_BATON
		{
			client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_STUN_BATON );
		}
		else
		{	// give the saber AND the blaster because most test maps will not have the STUN BATON flag set
			client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_SABER );	//this is precached in SP_info_player_deathmatch
		}

		for ( i = 0; i < AMMO_THERMAL; i++ ) // don't give ammo for explosives
		{
			client->ps.ammo[i] = ammoData[i].max;
		}

		client->ps.saberColor = SABER_BLUE;
		client->ps.saberActive = qfalse;
		client->ps.saberLength = 0;
		//Initialize force powers
		WP_InitForcePowers( ent );
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

		{
			G_KillBox( ent );
			gi.linkentity (ent);
			// force the base weapon up
			client->ps.weapon = WP_BRYAR_PISTOL;
			client->ps.weaponstate = WEAPON_READY;
		}

		// don't allow full run speed for a bit
		client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		client->ps.pm_time = 100;

		client->respawnTime = level.time;
		client->inactivityTime = level.time + g_inactivity->integer * 1000;
		client->latched_buttons = 0;

		// set default animations
		client->ps.torsoAnim = BOTH_STAND2;
		client->ps.legsAnim = BOTH_STAND2;

		// restore some player data if this is a spawn point with KEEP_REV (spawnflags&1) set...
		//
		if ( eSavedGameJustLoaded == eAUTO ||
			(spawnPoint->spawnflags&1) ||		// KEEP_PREV
			g_qbLoadTransition == qtrue )
		{
			Player_RestoreFromPrevLevel(ent);
		}


		/*
		Ghoul2 Insert Start
		*/

		if (eSavedGameJustLoaded == eNO)
		{
			ent->weaponModel = -1;
			G_SetG2PlayerModel( ent, "kyle", NULL, NULL, NULL );
		}
		else
		{
			if ( ent->client->NPC_class == CLASS_ATST )
			{
				G_LoadAnimFileSet( ent, "atst" );
				G_SetSkin( ent, "atst", NULL );
			}
			else
			{
				G_LoadAnimFileSet( ent, "kyle" );
				G_SetSkin( ent, "kyle", NULL );
			}
		}
		/*
		Ghoul2 Insert End
		*/

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
		ICARUS_FreeEnt( ent );	//FIXME: This shouldn't need to be done...?
		ICARUS_InitEnt( ent );

		if ( spawnPoint->spawnflags & 64 )
		{//player starts with absolutely no weapons
			ent->client->ps.stats[STAT_WEAPONS] = ( 1 << WP_NONE );
			ent->client->ps.ammo[weaponData[WP_NONE].ammoIndex] = 32000;	// checkme
			ent->client->ps.weapon = WP_NONE;
			ent->client->ps.weaponstate = WEAPON_READY;
		}

		if ( ent->client->ps.stats[STAT_WEAPONS] & ( 1 << WP_SABER ) )
		{//set up so has lightsaber
			WP_SaberInitBladeData( ent );
			if ( ent->weaponModel == -1 && ent->client->ps.weapon == WP_SABER )
			{
				G_CreateG2AttachedWeaponModel( ent, ent->client->ps.saberModel );
			}
		}
		if ( ent->weaponModel == -1 && ent->client->ps.weapon != WP_NONE )
		{
			G_CreateG2AttachedWeaponModel( ent, weaponData[ent->client->ps.weapon].weaponMdl );
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

}


