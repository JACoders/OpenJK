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

#include "g_local.h"
#include "objectives.h"


/*
=======================================================================

  SESSION DATA

Session data is the only data that stays persistant across level loads
and tournament restarts.
=======================================================================
*/

/*
================
G_WriteClientSessionData

Called on game shutdown
================
*/
void G_WriteClientSessionData( gclient_t *client ) {
	const char	*s;
	const char	*s2;
	const char	*var;
	int		i;

	s = va("%i", client->sess.sessionTeam );
	var = va( "session%i", client - level.clients );
	gi.cvar_set( var, s );

	s2 = "";
	// Throw all status info into a string
//	for (i=0;i< MAX_OBJECTIVES; i++)
//	{
//		s2 = va("%s %i %i",	s2, client->sess.mission_objectives[i].display,	client->sess.mission_objectives[i].status);
//	}

	// We're saving only one objective
	s2 = va("%i %i", client->sess.mission_objectives[LIGHTSIDE_OBJ].display,	client->sess.mission_objectives[LIGHTSIDE_OBJ].status);

	var = va( "sessionobj%i", client - level.clients );
	gi.cvar_set( var, s2 );

	// Throw all mission stats in to a string
	s2 = va("%i %i %i %i %i %i %i %i %i %i %i %i",
			client->sess.missionStats.secretsFound,
			client->sess.missionStats.totalSecrets,
			client->sess.missionStats.shotsFired,
			client->sess.missionStats.hits,
			client->sess.missionStats.enemiesSpawned,
			client->sess.missionStats.enemiesKilled,
			client->sess.missionStats.saberThrownCnt,
			client->sess.missionStats.saberBlocksCnt,
			client->sess.missionStats.legAttacksCnt,
			client->sess.missionStats.armAttacksCnt,
			client->sess.missionStats.torsoAttacksCnt,
			client->sess.missionStats.otherAttacksCnt
			);

	var = va( "missionstats%i", client - level.clients );
	gi.cvar_set( var, s2 );


	s2 = "";
	for (i=0;i< NUM_FORCE_POWERS; i++)
	{
		s2 = va("%s %i",s2, client->sess.missionStats.forceUsed[i]);
	}
	var = va( "sessionpowers%i", client - level.clients );
	gi.cvar_set( var, s2 );


	s2 = "";
	for (i=0;i< WP_NUM_WEAPONS; i++)
	{
		s2 = va("%s %i",s2, client->sess.missionStats.weaponUsed[i]);
	}
	var = va( "sessionweapons%i", client - level.clients );
	gi.cvar_set( var, s2 );


}

/*
================
G_ReadSessionData

Called on a reconnect
================
*/
void G_ReadSessionData( gclient_t *client ) {
	char	s[MAX_STRING_CHARS];
	const char	*var;
	int		i;
	int		lightsideDisplay;

	var = va( "session%i", client - level.clients );
	gi.Cvar_VariableStringBuffer( var, s, sizeof(s) );

	sscanf( s, "%i", &i );
	client->sess.sessionTeam = (team_t)i;

	var = va( "sessionobj%i", client - level.clients );
	gi.Cvar_VariableStringBuffer( var, s, sizeof(s) );

	var = s;
//	var++;

//	for (i=0;i< MAX_OBJECTIVES; i++)
//	{
//		sscanf( var, "%i %i",
//			&client->sess.mission_objectives[i].display,
//			&client->sess.mission_objectives[i].status);
//			var+=4;
//	}
	// Clear the objectives out
	for (i=0;i< MAX_OBJECTIVES; i++)
	{
		client->sess.mission_objectives[i].display = qfalse;
		client->sess.mission_objectives[i].status = OBJECTIVE_STAT_PENDING;
	}

	// Now load the LIGHTSIDE objective. That's the only cross level objective.
	sscanf( var, "%i %i",
		&lightsideDisplay,
		&client->sess.mission_objectives[LIGHTSIDE_OBJ].status);
	client->sess.mission_objectives[LIGHTSIDE_OBJ].display = lightsideDisplay ? qtrue : qfalse;

	var = va( "missionstats%i", client - level.clients );
	gi.Cvar_VariableStringBuffer( var, s, sizeof(s) );
	sscanf( s, "%i %i %i %i %i %i %i %i %i %i %i %i",
		&client->sess.missionStats.secretsFound,
		&client->sess.missionStats.totalSecrets,
		&client->sess.missionStats.shotsFired,
		&client->sess.missionStats.hits,
		&client->sess.missionStats.enemiesSpawned,
		&client->sess.missionStats.enemiesKilled,
		&client->sess.missionStats.saberThrownCnt,
		&client->sess.missionStats.saberBlocksCnt,
		&client->sess.missionStats.legAttacksCnt,
		&client->sess.missionStats.armAttacksCnt,
		&client->sess.missionStats.torsoAttacksCnt,
		&client->sess.missionStats.otherAttacksCnt);


	var = va( "sessionpowers%i", client - level.clients );
	gi.Cvar_VariableStringBuffer( var, s, sizeof(s) );

	i=0;
	var = strtok( s, " " );
	while( var != NULL )
	{
      /* While there are tokens in "s" */
	  client->sess.missionStats.forceUsed[i++] = atoi(var);
      /* Get next token: */
      var = strtok( NULL, " " );
	}
	assert (i==NUM_FORCE_POWERS);

	var = va( "sessionweapons%i", client - level.clients );
	gi.Cvar_VariableStringBuffer( var, s, sizeof(s) );

	i=0;
	var = strtok( s, " " );
	while( var != NULL )
	{
      /* While there are tokens in "s" */
	  client->sess.missionStats.weaponUsed[i++] = atoi(var);
      /* Get next token: */
      var = strtok( NULL, " " );
	}
	assert (i==WP_NUM_WEAPONS);
}


/*
================
G_InitSessionData

Called on a first-time connect
================
*/
void G_InitSessionData( gclient_t *client, char *userinfo ) {
	clientSession_t	*sess;

	sess = &client->sess;

	sess->sessionTeam = TEAM_FREE;

	G_WriteClientSessionData( client );
}


/*
==================
G_InitWorldSession

==================
*/
void G_InitWorldSession( void ) {
}

/*
==================
G_WriteSessionData

==================
*/
void G_WriteSessionData( void ) {
	int		i;

	gi.cvar_set( "session", 0) ;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected == CON_CONNECTED ) {
			G_WriteClientSessionData( &level.clients[i] );
		}
	}
}
