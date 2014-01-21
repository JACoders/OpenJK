// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"
#include "bg_saga.h"

#include "ui/menudef.h"			// for the voice chats

//rww - for getting bot commands...
int AcceptBotCommand(char *cmd, gentity_t *pl);
//end rww

void WP_SetSaber( int entNum, saberInfo_t *sabers, int saberNum, const char *saberName );

void Cmd_NPC_f( gentity_t *ent );
void SetTeamQuick(gentity_t *ent, int team, qboolean doBegin);
extern void AddIP( char *str );
qboolean G_SaberModelSetup(gentity_t *ent);
extern vmCvar_t	d_saberSPStyleDamage;

/*
==================
DeathmatchScoreboardMessage

==================
*/
void DeathmatchScoreboardMessage( gentity_t *ent ) {
	char		entry[1024];
	char		string[1400];
	int			stringlength;
	int			i, j;
	gclient_t	*cl;
	int			numSorted, scoreFlags, accuracy, perfect;

	// send the latest information on all clients
	string[0] = 0;
	stringlength = 0;
	scoreFlags = 0;

	numSorted = level.numConnectedClients;
	
	if (numSorted > MAX_CLIENT_SCORE_SEND)
	{
		numSorted = MAX_CLIENT_SCORE_SEND;
	}

	for (i=0 ; i < numSorted ; i++) {
		int		ping;

		cl = &level.clients[level.sortedClients[i]];

		if ( cl->pers.connected == CON_CONNECTING ) {
			ping = -1;
		} else {
			ping = cl->ps.ping < 999 ? cl->ps.ping : 999;

			if (g_fakeClients.integer && (g_entities[cl - level.clients].r.svFlags & SVF_BOT))
				ping = Q_irand(30, 60);
		}

		if( cl->accuracy_shots ) {
			accuracy = cl->accuracy_hits * 100 / cl->accuracy_shots;
		}
		else {
			accuracy = 0;
		}
		perfect = ( cl->ps.persistant[PERS_RANK] == 0 && cl->ps.persistant[PERS_KILLED] == 0 ) ? 1 : 0;

//JAPRO - Serverside - Scoreboard Deaths - Start
		if (ent->client->pers.isJAPRO == qtrue)
		{
			Com_sprintf (entry, sizeof(entry),
				" %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i", 
				level.sortedClients[i], 
				cl->ps.persistant[PERS_SCORE], 
				ping, 
				(level.time - cl->pers.enterTime)/60000,
				scoreFlags,
				g_entities[level.sortedClients[i]].s.powerups,
				accuracy, 
				cl->ps.persistant[PERS_IMPRESSIVE_COUNT],
				cl->ps.persistant[PERS_EXCELLENT_COUNT],
				cl->ps.persistant[PERS_GAUNTLET_FRAG_COUNT], 
				cl->ps.persistant[PERS_DEFEND_COUNT], 
				cl->ps.persistant[PERS_ASSIST_COUNT], 
				perfect,
				cl->ps.persistant[PERS_CAPTURES],
				cl->ps.persistant[PERS_KILLED]);
		}
		else
		{
			Com_sprintf (entry, sizeof(entry),
				" %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
				level.sortedClients[i],
				cl->ps.persistant[PERS_SCORE],
				ping,
				(level.time - cl->pers.enterTime)/60000,
				scoreFlags,
				g_entities[level.sortedClients[i]].s.powerups,
				accuracy, 
				cl->ps.persistant[PERS_IMPRESSIVE_COUNT],
				cl->ps.persistant[PERS_EXCELLENT_COUNT],
				cl->ps.persistant[PERS_GAUNTLET_FRAG_COUNT], 
				cl->ps.persistant[PERS_DEFEND_COUNT], 
				cl->ps.persistant[PERS_ASSIST_COUNT], 
				perfect,
				cl->ps.persistant[PERS_CAPTURES]);
		}
//JAPRO - Serverside - Scoreboard Deaths - End
		j = strlen(entry);
		if (stringlength + j > 1022)
			break;
		strcpy (string + stringlength, entry);
		stringlength += j;
	}

	//still want to know the total # of clients
	i = level.numConnectedClients;

	trap->SendServerCommand( ent-g_entities, va("scores %i %i %i%s", i, 
		level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE],
		string ) );
}


/*
==================
Cmd_Score_f

Request current scoreboard information
==================
*/
void Cmd_Score_f( gentity_t *ent ) {
	DeathmatchScoreboardMessage( ent );
}

/*
==================
ConcatArgs
==================
*/
char	*ConcatArgs( int start ) {
	int		i, c, tlen;
	static char	line[MAX_STRING_CHARS];
	int		len;
	char	arg[MAX_STRING_CHARS];

	len = 0;
	c = trap->Argc();
	for ( i = start ; i < c ; i++ ) {
		trap->Argv( i, arg, sizeof( arg ) );
		tlen = strlen( arg );
		if ( len + tlen >= MAX_STRING_CHARS - 1 ) {
			break;
		}
		memcpy( line + len, arg, tlen );
		len += tlen;
		if ( i != c - 1 ) {
			line[len] = ' ';
			len++;
		}
	}

	line[len] = 0;

	return line;
}

/*
==================
StringIsInteger
==================
*/
qboolean StringIsInteger( const char *s ) {
	int			i=0, len=0;
	qboolean	foundDigit=qfalse;

	for ( i=0, len=strlen( s ); i<len; i++ )
	{
		if ( !isdigit( s[i] ) )
			return qfalse;

		foundDigit = qtrue;
	}

	return foundDigit;
}

//[JAPRO - Serverside - All - Redid and moved sanitizestring2 for partial name recognition - Start]
/*
==================
SanitizeString2

Rich's revised version of SanitizeString
==================
*/
void SanitizeString2( const char *in, char *out )
{
	int i = 0;
	int r = 0;

	while (in[i])
	{
		if (i >= MAX_NAME_LENGTH-1)
		{ //the ui truncates the name here..
			break;
		}

		if (in[i] == '^')
		{
			if (in[i+1] >= 48 && //'0'
				in[i+1] <= 57) //'9'
			{ //only skip it if there's a number after it for the color
				i += 2;
				continue;
			}
			else
			{ //just skip the ^
				i++;
				continue;
			}
		}

		if (in[i] < 32)
		{
			i++;
			continue;
		}

		out[r] = tolower(in[i]);//lowercase please
		r++;
		i++;
	}
	out[r] = 0;
}
//[JAPRO - Serverside - All - Redid and moved sanitizestring2 for partial name recognition - End]

//[JAPRO - Serverside - All - Redid and clientnumberfromstring for partial name recognition - Start]
/*
==================
ClientNumberFromString

Returns a player number for either a number or name string
Returns -1 if invalid
==================
*/
static int JP_ClientNumberFromString(gentity_t *to, const char *s) 
{
	gclient_t	*cl;
	int			idnum, i, match = -1;
	char		s2[MAX_STRING_CHARS];
	char		n2[MAX_STRING_CHARS];

	// numeric values are just slot numbers
	if (s[0] >= '0' && s[0] <= '9' && strlen(s) == 1) //changed this to only recognize numbers 0-31 as client numbers, otherwise interpret as a name, in which case sanitize2 it and accept partial matches (return error if multiple matches)
		{
		idnum = atoi( s );
		cl = &level.clients[idnum];
		if ( cl->pers.connected != CON_CONNECTED ) {
			trap->SendServerCommand( to-g_entities, va("print \"Client '%i' is not active\n\"", idnum));
			return -1;
		}
		return idnum;
	}

	else if (s[0] == '1' && s[0] == '2' && (s[1] >= '0' && s[1] <= '9' && strlen(s) == 2)) 
	{
		idnum = atoi( s );
		cl = &level.clients[idnum];
		if ( cl->pers.connected != CON_CONNECTED ) {
			trap->SendServerCommand( to-g_entities, va("print \"Client '%i' is not active\n\"", idnum));
			return -1;
		}
		return idnum;
	}

	else if (s[0] == '3' && (s[1] >= '0' && s[1] <= '1' && strlen(s) == 2)) 
	{
		idnum = atoi( s );
		cl = &level.clients[idnum];
		if ( cl->pers.connected != CON_CONNECTED ) {
			trap->SendServerCommand( to-g_entities, va("print \"Client '%i' is not active\n\"", idnum));
			return -1;
		}
		return idnum;
	}
	// check for a name match
	SanitizeString2( s, s2 );
	for ( idnum=0,cl=level.clients ; idnum < level.maxclients ; idnum++,cl++ ){
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		SanitizeString2( cl->pers.netname, n2 );

		for (i=0 ; i < level.numConnectedClients ; i++) 
		{
			cl=&level.clients[level.sortedClients[i]];
			SanitizeString2( cl->pers.netname, n2 );
			if (strstr(n2, s2)) 
			{
				if(match != -1)
				{ //found more than one match
					trap->SendServerCommand( to-g_entities, va("print \"More than one user '%s' on the server\n\"", s));
					return -2;
				}
				match = level.sortedClients[i];
			}
		}
		if (match != -1)//uhh
			return match;
	}
	trap->SendServerCommand(to-g_entities, va("print \"User '%s' is not on the server\n\"", s));
	return -1;
}
//[JAPRO - Serverside - All - Redid and clientnumberfromstring for partial name recognition - End]

/*
==================
ClientNumberFromString

Returns a player number for either a number or name string
Returns -1 if invalid
==================
*/
int ClientNumberFromString( gentity_t *to, const char *s, qboolean allowconnecting ) {
	gclient_t	*cl;
	int			idnum;
	char		cleanInput[MAX_NETNAME];

	if ( StringIsInteger( s ) )
	{// numeric values could be slot numbers
		idnum = atoi( s );
		if ( idnum >= 0 && idnum < level.maxclients )
		{
			cl = &level.clients[idnum];
			if ( cl->pers.connected == CON_CONNECTED )
				return idnum;
			else if ( allowconnecting && cl->pers.connected == CON_CONNECTING )
				return idnum;
		}
	}

	Q_strncpyz( cleanInput, s, sizeof(cleanInput) );
	Q_StripColor( cleanInput );

	for ( idnum=0,cl=level.clients; idnum < level.maxclients; idnum++,cl++ )
	{// check for a name match
		if ( cl->pers.connected != CON_CONNECTED )
			if ( !allowconnecting || cl->pers.connected < CON_CONNECTING )
				continue;

		if ( !Q_stricmp( cl->pers.netname_nocolor, cleanInput ) )
			return idnum;
	}

	trap->SendServerCommand( to-g_entities, va( "print \"User %s is not on the server\n\"", s ) );
	return -1;
}

qboolean QINLINE ClientIsIgnored(const int selfID, const int targetID) {
	return (qboolean) (selfID != targetID && ((level.clients[selfID].sess.ignore ^ 0xFFFFFFFF) == 0 || (level.clients[selfID].sess.ignore & (1 << targetID))));
}

qboolean QINLINE ClientIgnore(const int selfID, const int targetID) {
	qboolean ignored;
	const int targetFlag = (1 << targetID);
	if (level.clients[selfID].sess.ignore & targetFlag) {
		level.clients[selfID].sess.ignore ^= targetFlag;
		ignored = qfalse;
	} else {
		level.clients[selfID].sess.ignore |= targetFlag;
		ignored = qtrue;
	}
	return ignored;
}

qboolean QINLINE ClientIgnoreAll(const int selfID) {
	qboolean ignoredAll;
	if (level.clients[selfID].sess.ignore ^ 0xFFFFFFFF) {
		level.clients[selfID].sess.ignore = 0xFFFFFFFF;
		ignoredAll = qtrue;
	} else {
		level.clients[selfID].sess.ignore = 0;
		ignoredAll = qfalse;
	}
	return ignoredAll;
}



/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void G_Give( gentity_t *ent, const char *name, const char *args, int argc )
{
	gitem_t		*it;
	int			i;
	qboolean	give_all = qfalse;
	gentity_t	*it_ent;
	trace_t		trace;

	if ( !Q_stricmp( name, "all" ) )
		give_all = qtrue;

	if ( give_all )
	{
		for ( i=0; i<HI_NUM_HOLDABLE; i++ )
			ent->client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << i);
	}

	if ( give_all || !Q_stricmp( name, "health") )
	{
		if ( argc == 3 )
			ent->health = Com_Clampi( 1, ent->client->ps.stats[STAT_MAX_HEALTH], atoi( args ) );
		else
		{
			if ( level.gametype == GT_SIEGE && ent->client->siegeClass != -1 )
				ent->health = bgSiegeClasses[ent->client->siegeClass].maxhealth;
			else
				ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
		}
		if ( !give_all )
			return;
	}

	if ( give_all || !Q_stricmp( name, "armor" ) || !Q_stricmp( name, "shield" ) )
	{
		if ( argc == 3 )
			ent->client->ps.stats[STAT_ARMOR] = Com_Clampi( 0, 16383, atoi( args ) );
		else
		{
			if ( level.gametype == GT_SIEGE && ent->client->siegeClass != -1 )
				ent->client->ps.stats[STAT_ARMOR] = bgSiegeClasses[ent->client->siegeClass].maxarmor;
			else
				ent->client->ps.stats[STAT_ARMOR] = ent->client->ps.stats[STAT_MAX_HEALTH];
		}

		if ( !give_all )
			return;
	}

	if ( give_all || !Q_stricmp( name, "force" ) )
	{
		if ( argc == 3 )
			ent->client->ps.fd.forcePower = Com_Clampi( 0, ent->client->ps.fd.forcePowerMax, atoi( args ) );
		else
			ent->client->ps.fd.forcePower = ent->client->ps.fd.forcePowerMax;

		if ( !give_all )
			return;
	}

	if ( give_all || !Q_stricmp( name, "weapons" ) )
	{
		ent->client->ps.stats[STAT_WEAPONS] = (1 << (LAST_USEABLE_WEAPON+1)) - ( 1 << WP_NONE );
		if ( !give_all )
			return;
	}
	
	if ( !give_all && !Q_stricmp( name, "weaponnum" ) )
	{
		ent->client->ps.stats[STAT_WEAPONS] |= (1 << atoi( args ));
		return;
	}

	if ( give_all || !Q_stricmp( name, "ammo" ) )
	{
		int num = 999;
		if ( argc == 3 )
			num = atoi( args );
		for ( i=0; i<MAX_WEAPONS; i++ )
			ent->client->ps.ammo[i] = num;
		if ( !give_all )
			return;
	}

	if ( !Q_stricmp( name, "excellent" ) ) {
		ent->client->ps.persistant[PERS_EXCELLENT_COUNT]++;
		return;
	}
	if ( !Q_stricmp( name, "impressive" ) ) {
		ent->client->ps.persistant[PERS_IMPRESSIVE_COUNT]++;
		return;
	}
	if ( !Q_stricmp( name, "gauntletaward" ) ) {
		ent->client->ps.persistant[PERS_GAUNTLET_FRAG_COUNT]++;
		return;
	}
	if ( !Q_stricmp( name, "defend" ) ) {
		ent->client->ps.persistant[PERS_DEFEND_COUNT]++;
		return;
	}
	if ( !Q_stricmp( name, "assist" ) ) {
		ent->client->ps.persistant[PERS_ASSIST_COUNT]++;
		return;
	}

	// spawn a specific item right on the player
	if ( !give_all ) {
		it = BG_FindItem( name );
		if ( !it )
			return;

		it_ent = G_Spawn();
		VectorCopy( ent->r.currentOrigin, it_ent->s.origin );
		it_ent->classname = it->classname;
		G_SpawnItem( it_ent, it );
		FinishSpawningItem( it_ent );
		if ( !it_ent || !it_ent->inuse )
			return;
		memset( &trace, 0, sizeof( trace ) );
		Touch_Item( it_ent, ent, &trace );
		if ( it_ent->inuse )
			G_FreeEntity( it_ent );
	}
}

void Cmd_Give_f( gentity_t *ent )
{
	char name[MAX_TOKEN_CHARS] = {0};

	trap->Argv( 1, name, sizeof( name ) );
	G_Give( ent, name, ConcatArgs( 2 ), trap->Argc() );
}

void Cmd_GiveOther_f( gentity_t *ent )
{
	char		name[MAX_TOKEN_CHARS] = {0};
	int			i;
	char		otherindex[MAX_TOKEN_CHARS];
	gentity_t	*otherEnt = NULL;

	if ( trap->Argc () < 3 ) {
		trap->SendServerCommand( ent-g_entities, "print \"Usage: giveother <player id> <givestring>\n\"" );
		return;
	}

	trap->Argv( 1, otherindex, sizeof( otherindex ) );
	i = JP_ClientNumberFromString( ent, otherindex );
	if ( i == -1 || i == -2) {
		return;
	}

	otherEnt = &g_entities[i];
	if ( !otherEnt->inuse || !otherEnt->client ) {
		return;
	}

	if ( (otherEnt->health <= 0 || otherEnt->client->tempSpectate >= level.time || otherEnt->client->sess.sessionTeam == TEAM_SPECTATOR) )
	{
		// Intentionally displaying for the command user
		trap->SendServerCommand( ent-g_entities, "print \"Target must be alive to use this command.\n\"" );
		return;
	}

	trap->Argv( 2, name, sizeof( name ) );

	G_Give( otherEnt, name, ConcatArgs( 3 ), trap->Argc()-1 );
}

/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f( gentity_t *ent ) {
	char *msg = NULL;

	ent->flags ^= FL_GODMODE;
	if ( !(ent->flags & FL_GODMODE) )
		msg = "godmode OFF";
	else
		msg = "godmode ON";

	trap->SendServerCommand( ent-g_entities, va( "print \"%s\n\"", msg ) );
}


/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f( gentity_t *ent ) {
	char *msg = NULL;

	ent->flags ^= FL_NOTARGET;
	if ( !(ent->flags & FL_NOTARGET) )
		msg = "notarget OFF";
	else
		msg = "notarget ON";

	trap->SendServerCommand( ent-g_entities, va( "print \"%s\n\"", msg ) );
}


/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f( gentity_t *ent ) {
	char *msg = NULL;

	if (!sv_cheats.integer)
	{
		if (ent->r.svFlags & SVF_FULLADMIN)//Logged in as full admin
		{
			if (!(g_fullAdminLevel.integer & (1 << A_NOCLIP)))
			{
				trap->SendServerCommand( ent-g_entities, va("print \"You are not authorized to use this command (noclip).\n\"") );
				return;
			}
		}
		else if (ent->r.svFlags & SVF_JUNIORADMIN)//Logged in as junior admin
		{
			if (!(g_juniorAdminLevel.integer & (1 << A_NOCLIP)))
			{
				trap->SendServerCommand( ent-g_entities, va("print \"You are not authorized to use this command (noclip).\n\"") );
				return;
			}
		}
		else//Not logged in
		{
			trap->SendServerCommand( ent-g_entities, "print \"Cheats are not enabled. You must be logged in to use this command (noclip).\n\"" );//replaces "Cheats are not enabled on this server." msg
			return;
		}
	}

	ent->client->noclip = !ent->client->noclip;
	if ( !ent->client->noclip )
		msg = "noclip OFF";
	else
		msg = "noclip ON";

	trap->SendServerCommand( ent-g_entities, va( "print \"%s\n\"", msg ) );
}


/*
==================
Cmd_LevelShot_f

This is just to help generate the level pictures
for the menus.  It goes to the intermission immediately
and sends over a command to the client to resize the view,
hide the scoreboard, and take a special screenshot
==================
*/
void Cmd_LevelShot_f( gentity_t *ent )
{
	if ( !ent->client->pers.localClient )
	{
		trap->SendServerCommand(ent-g_entities, "print \"The levelshot command must be executed by a local client\n\"");
		return;
	}

	// doesn't work in single player
	if ( level.gametype == GT_SINGLE_PLAYER )
	{
		trap->SendServerCommand(ent-g_entities, "print \"Must not be in singleplayer mode for levelshot\n\"" );
		return;
	}

	BeginIntermission();
	trap->SendServerCommand( ent-g_entities, "clientLevelShot" );
}

#if 0
/*
==================
Cmd_TeamTask_f

From TA.
==================
*/
void Cmd_TeamTask_f( gentity_t *ent ) {
	char userinfo[MAX_INFO_STRING];
	char		arg[MAX_TOKEN_CHARS];
	int task;
	int client = ent->client - level.clients;

	if ( trap->Argc() != 2 ) {
		return;
	}
	trap->Argv( 1, arg, sizeof( arg ) );
	task = atoi( arg );

	trap->GetUserinfo(client, userinfo, sizeof(userinfo));
	Info_SetValueForKey(userinfo, "teamtask", va("%d", task));
	trap->SetUserinfo(client, userinfo);
	ClientUserinfoChanged(client);
}
#endif

void G_Kill( gentity_t *ent ) {
	if ((level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL) &&
		level.numPlayingClients > 1 && !level.warmupTime)
	{
		if (!g_allowDuelSuicide.integer)
		{
			trap->SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "ATTEMPTDUELKILL")) );
			return;
		}
	}
	else if (level.gametype >= GT_TEAM && level.numPlayingClients > 1 && !level.warmupTime)
	{
		if (!g_allowTeamSuicide.integer)
		{
			trap->SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "ATTEMPTDUELKILL")) );
			return;
		}
	}

	ent->flags &= ~FL_GODMODE;
	ent->client->ps.stats[STAT_HEALTH] = ent->health = -999;
	player_die (ent, ent, ent, 100000, MOD_SUICIDE);
}

/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f( gentity_t *ent ) {
	G_Kill( ent );
}

void Cmd_KillOther_f( gentity_t *ent )
{
	int			i;
	char		otherindex[MAX_TOKEN_CHARS];
	gentity_t	*otherEnt = NULL;

	if ( trap->Argc () < 2 ) {
		trap->SendServerCommand( ent-g_entities, "print \"Usage: killother <player id>\n\"" );
		return;
	}

	trap->Argv( 1, otherindex, sizeof( otherindex ) );
	i = JP_ClientNumberFromString( ent, otherindex );
	if ( i == -1 || i == -2) {
		return;
	}

	otherEnt = &g_entities[i];
	if ( !otherEnt->inuse || !otherEnt->client ) {
		return;
	}

	if ( (otherEnt->health <= 0 || otherEnt->client->tempSpectate >= level.time || otherEnt->client->sess.sessionTeam == TEAM_SPECTATOR) )
	{
		// Intentionally displaying for the command user
		trap->SendServerCommand( ent-g_entities, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "MUSTBEALIVE" ) ) );
		return;
	}

	G_Kill( otherEnt );
}

gentity_t *G_GetDuelWinner(gclient_t *client)
{
	gclient_t *wCl;
	int i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		wCl = &level.clients[i];
		
		if (wCl && wCl != client && /*wCl->ps.clientNum != client->ps.clientNum &&*/
			wCl->pers.connected == CON_CONNECTED && wCl->sess.sessionTeam != TEAM_SPECTATOR)
		{
			return &g_entities[wCl->ps.clientNum];
		}
	}

	return NULL;
}

/*
=================
BroadCastTeamChange

Let everyone know about a team change
=================
*/
void BroadcastTeamChange( gclient_t *client, int oldTeam )
{
	client->ps.fd.forceDoInit = 1; //every time we change teams make sure our force powers are set right

	if (level.gametype == GT_SIEGE)
	{ //don't announce these things in siege
		return;
	}

	if ( client->sess.sessionTeam == TEAM_RED ) {
		trap->SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
			client->pers.netname, G_GetStringEdString("MP_SVGAME", "JOINEDTHEREDTEAM")) );
	} else if ( client->sess.sessionTeam == TEAM_BLUE ) {
		trap->SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
		client->pers.netname, G_GetStringEdString("MP_SVGAME", "JOINEDTHEBLUETEAM")));
	} else if ( client->sess.sessionTeam == TEAM_SPECTATOR && oldTeam != TEAM_SPECTATOR ) {
		trap->SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
		client->pers.netname, G_GetStringEdString("MP_SVGAME", "JOINEDTHESPECTATORS")));
	} else if ( client->sess.sessionTeam == TEAM_FREE ) {
		if (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
		{
			/*
			gentity_t *currentWinner = G_GetDuelWinner(client);

			if (currentWinner && currentWinner->client)
			{
				trap->SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s %s\n\"",
				currentWinner->client->pers.netname, G_GetStringEdString("MP_SVGAME", "VERSUS"), client->pers.netname));
			}
			else
			{
				trap->SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
				client->pers.netname, G_GetStringEdString("MP_SVGAME", "JOINEDTHEBATTLE")));
			}
			*/
			//NOTE: Just doing a vs. once it counts two players up
		}
		else
		{
			trap->SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
			client->pers.netname, G_GetStringEdString("MP_SVGAME", "JOINEDTHEBATTLE")));
		}
	}

	G_LogPrintf ( "setteam:  %i %s %s\n",
				  client - &level.clients[0],
				  TeamName ( oldTeam ),
				  TeamName ( client->sess.sessionTeam ) );
}

qboolean G_PowerDuelCheckFail(gentity_t *ent)
{
	int			loners = 0;
	int			doubles = 0;

	if (!ent->client || ent->client->sess.duelTeam == DUELTEAM_FREE)
	{
		return qtrue;
	}

	G_PowerDuelCount(&loners, &doubles, qfalse);

	if (ent->client->sess.duelTeam == DUELTEAM_LONE && loners >= 1)
	{
		return qtrue;
	}

	if (ent->client->sess.duelTeam == DUELTEAM_DOUBLE && doubles >= 2)
	{
		return qtrue;
	}

	return qfalse;
}

/*
=================
SetTeam
=================
*/
qboolean g_dontPenalizeTeam = qfalse;
qboolean g_preventTeamBegin = qfalse;
void SetTeam( gentity_t *ent, char *s, qboolean forcedToJoin ) {//JAPRO - Modified for proper amforceteam
	int					team, oldTeam;
	gclient_t			*client;
	int					clientNum;
	spectatorState_t	specState;
	int					specClient;
	int					teamLeader;

	// fix: this prevents rare creation of invalid players
	if (!ent->inuse)
	{
		return;
	}

	//
	// see what change is requested
	//
	client = ent->client;

	clientNum = client - level.clients;
	specClient = 0;
	specState = SPECTATOR_NOT;
	if ( !Q_stricmp( s, "scoreboard" ) || !Q_stricmp( s, "score" )  ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FREE; // SPECTATOR_SCOREBOARD disabling this for now since it is totally broken on client side
	} else if ( !Q_stricmp( s, "follow1" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -1;
	} else if ( !Q_stricmp( s, "follow2" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -2;
	} else if ( !Q_stricmp( s, "spectator" ) || !Q_stricmp( s, "s" )  || !Q_stricmp( s, "spectate" ) ) {
		if(level.isLockedspec && !forcedToJoin)
		{
				trap->SendServerCommand( ent->client->ps.clientNum, va("print \"^7The ^3Spectator ^7Access has been locked!\n\""));
				return;
		}
		else
		{
			team = TEAM_SPECTATOR;
			specState = SPECTATOR_FREE;
		}
	} else if ( g_gametype.integer >= GT_TEAM ) {
		// if running a team game, assign player to one of the teams
		specState = SPECTATOR_NOT;
		if ( !Q_stricmp(s, "red") || !Q_stricmp(s, "r"))
		{
			if(level.isLockedred && !forcedToJoin)
			{
				trap->SendServerCommand( ent->client->ps.clientNum, va("print \"^7The ^1Red ^7team is locked!\n\""));
				return;
			}
			else
			{
				team = TEAM_RED;
			}
		} else if (!Q_stricmp(s, "blue") || !Q_stricmp(s, "b"))
		{
			if(level.isLockedblue && !forcedToJoin)
			{
				trap->SendServerCommand( ent->client->ps.clientNum, va("print \"^7The ^4Blue ^7team is locked!\n\""));
				return;
			}
			else
			{
				team = TEAM_BLUE;
			}
		} else {
			// pick the team with the least number of players
			//For now, don't do this. The legalize function will set powers properly now.
			/*
			if (g_forceBasedTeams.integer)
			{
				if (ent->client->ps.fd.forceSide == FORCE_LIGHTSIDE)
				{
					team = TEAM_BLUE;
				}
				else
				{
					team = TEAM_RED;
				}
			}
			else
			{
			*/
				team = PickTeam( clientNum );
				if(team == TEAM_BLUE && level.isLockedblue && !forcedToJoin) {
					trap->SendServerCommand( ent->client->ps.clientNum, va("print \"^7The ^4Blue ^7team is locked!\n\""));
					return;
				}
				else if(team == TEAM_RED && level.isLockedred && !forcedToJoin) {
					trap->SendServerCommand( ent->client->ps.clientNum, va("print \"^7The ^1Red ^7team is locked!\n\""));
					return;
				}
			//}
		}

		if ( g_teamForceBalance.integer && !g_jediVmerc.integer ) {
			int		counts[TEAM_NUM_TEAMS];

			//JAC: Invalid clientNum was being used
			counts[TEAM_BLUE] = TeamCount( ent-g_entities, TEAM_BLUE );
			counts[TEAM_RED] = TeamCount( ent-g_entities, TEAM_RED );

			// We allow a spread of two
			if ( team == TEAM_RED && counts[TEAM_RED] - counts[TEAM_BLUE] > 1 ) {
				//For now, don't do this. The legalize function will set powers properly now.
				/*
				if (g_forceBasedTeams.integer && ent->client->ps.fd.forceSide == FORCE_DARKSIDE)
				{
					trap->SendServerCommand( ent->client->ps.clientNum, 
						va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TOOMANYRED_SWITCH")) );
				}
				else
				*/
				{
					//JAC: Invalid clientNum was being used
					trap->SendServerCommand( ent-g_entities, 
						va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TOOMANYRED")) );
				}
				return; // ignore the request
			}
			if ( team == TEAM_BLUE && counts[TEAM_BLUE] - counts[TEAM_RED] > 1 ) {
				//For now, don't do this. The legalize function will set powers properly now.
				/*
				if (g_forceBasedTeams.integer && ent->client->ps.fd.forceSide == FORCE_LIGHTSIDE)
				{
					trap->SendServerCommand( ent->client->ps.clientNum, 
						va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TOOMANYBLUE_SWITCH")) );
				}
				else
				*/
				{
					//JAC: Invalid clientNum was being used
					trap->SendServerCommand( ent-g_entities, 
						va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TOOMANYBLUE")) );
				}
				return; // ignore the request
			}

			// It's ok, the team we are switching to has less or same number of players
		}

		//For now, don't do this. The legalize function will set powers properly now.
		/*
		if (g_forceBasedTeams.integer)
		{
			if (team == TEAM_BLUE && ent->client->ps.fd.forceSide != FORCE_LIGHTSIDE)
			{
				trap->SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "MUSTBELIGHT")) );
				return;
			}
			if (team == TEAM_RED && ent->client->ps.fd.forceSide != FORCE_DARKSIDE)
			{
				trap->SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "MUSTBEDARK")) );
				return;
			}
		}
		*/

	} else {
		// force them to spectators if there aren't any spots free
		team = TEAM_FREE;
	}

	oldTeam = client->sess.sessionTeam;

	if (level.gametype == GT_SIEGE)
	{
		if (client->tempSpectate >= level.time &&
			team == TEAM_SPECTATOR)
		{ //sorry, can't do that.
			return;
		}

		if ( team == oldTeam && team != TEAM_SPECTATOR )
			return;

		client->sess.siegeDesiredTeam = team;
		//oh well, just let them go.
		/*
		if (team != TEAM_SPECTATOR)
		{ //can't switch to anything in siege unless you want to switch to being a fulltime spectator
			//fill them in on their objectives for this team now
			trap->SendServerCommand(ent-g_entities, va("sb %i", client->sess.siegeDesiredTeam));

			trap->SendServerCommand( ent-g_entities, va("print \"You will be on the selected team the next time the round begins.\n\"") );
			return;
		}
		*/
		if (client->sess.sessionTeam != TEAM_SPECTATOR &&
			team != TEAM_SPECTATOR)
		{ //not a spectator now, and not switching to spec, so you have to wait til you die.
			//trap->SendServerCommand( ent-g_entities, va("print \"You will be on the selected team the next time you respawn.\n\"") );
			qboolean doBegin;
			if (ent->client->tempSpectate >= level.time)
			{
				doBegin = qfalse;
			}
			else
			{
				doBegin = qtrue;
			}

			if (doBegin)
			{
				// Kill them so they automatically respawn in the team they wanted.
				if (ent->health > 0)
				{
					ent->flags &= ~FL_GODMODE;
					ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die( ent, ent, ent, 100000, MOD_TEAM_CHANGE ); 
				}
			}

			if (ent->client->sess.sessionTeam != ent->client->sess.siegeDesiredTeam)
			{
				SetTeamQuick(ent, ent->client->sess.siegeDesiredTeam, qfalse);
			}

			return;
		}
	}

	// override decision if limiting the players
	if ( (level.gametype == GT_DUEL)
		&& level.numNonSpectatorClients >= 2 )
	{
		team = TEAM_SPECTATOR;
	}
	else if ( (level.gametype == GT_POWERDUEL)
		&& (level.numPlayingClients >= 3 || G_PowerDuelCheckFail(ent)) )
	{
		team = TEAM_SPECTATOR;
	}
	else if ( g_maxGameClients.integer > 0 && 
		level.numNonSpectatorClients >= g_maxGameClients.integer )
	{
		team = TEAM_SPECTATOR;
	}

	//
	// decide if we will allow the change
	//
	if ( team == oldTeam && team != TEAM_SPECTATOR ) {
		return;
	}

	//
	// execute the team change
	//

	//If it's siege then show the mission briefing for the team you just joined.
//	if (level.gametype == GT_SIEGE && team != TEAM_SPECTATOR)
//	{
//		trap->SendServerCommand(clientNum, va("sb %i", team));
//	}

	// if the player was dead leave the body
	if ( client->ps.stats[STAT_HEALTH] <= 0 && client->sess.sessionTeam != TEAM_SPECTATOR ) {
		MaintainBodyQueue(ent);
	}

	// he starts at 'base'
	client->pers.teamState.state = TEAM_BEGIN;
	if ( oldTeam != TEAM_SPECTATOR ) {
		// Kill him (makes sure he loses flags, etc)
		ent->flags &= ~FL_GODMODE;
		ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
		g_dontPenalizeTeam = qtrue;
		player_die (ent, ent, ent, 100000, MOD_SUICIDE);
		g_dontPenalizeTeam = qfalse;

	}
	// they go to the end of the line for tournaments
	if ( team == TEAM_SPECTATOR && oldTeam != team )
		AddTournamentQueue( client );

	// clear votes if going to spectator (specs can't vote)
	if ( team == TEAM_SPECTATOR )
		G_ClearVote( ent );
	// also clear team votes if switching red/blue or going to spec
	G_ClearTeamVote( ent, oldTeam );

	client->sess.sessionTeam = (team_t)team;
	client->sess.spectatorState = specState;
	client->sess.spectatorClient = specClient;

	client->sess.teamLeader = qfalse;
	if ( team == TEAM_RED || team == TEAM_BLUE ) {
		teamLeader = TeamLeader( team );
		// if there is no team leader or the team leader is a bot and this client is not a bot
		if ( teamLeader == -1 || ( !(g_entities[clientNum].r.svFlags & SVF_BOT) && (g_entities[teamLeader].r.svFlags & SVF_BOT) ) ) {
			//SetLeader( team, clientNum );
		}
	}
	// make sure there is a team leader on the team the player came from
	if ( oldTeam == TEAM_RED || oldTeam == TEAM_BLUE ) {
		CheckTeamLeader( oldTeam );
	}

	BroadcastTeamChange( client, oldTeam );

	//make a disappearing effect where they were before teleporting them to the appropriate spawn point,
	//if we were not on the spec team
	if (oldTeam != TEAM_SPECTATOR)
	{
		gentity_t *tent = G_TempEntity( client->ps.origin, EV_PLAYER_TELEPORT_OUT );
		tent->s.clientNum = clientNum;
	}

	// get and distribute relevent paramters
	if ( !ClientUserinfoChanged( clientNum ) )
		return;

	if (!g_preventTeamBegin)
	{
		ClientBegin( clientNum, qfalse );
	}
}

/*
=================
StopFollowing

If the client being followed leaves the game, or you just want to drop
to free floating spectator mode
=================
*/
extern void G_LeaveVehicle( gentity_t *ent, qboolean ConCheck );
void StopFollowing( gentity_t *ent ) {
	int i=0;
	ent->client->ps.persistant[ PERS_TEAM ] = TEAM_SPECTATOR;	
	ent->client->sess.sessionTeam = TEAM_SPECTATOR;	
	ent->client->sess.spectatorState = SPECTATOR_FREE;
	ent->client->ps.pm_flags &= ~PMF_FOLLOW;
	ent->r.svFlags &= ~SVF_BOT;
	ent->client->ps.clientNum = ent - g_entities;
	ent->client->ps.weapon = WP_NONE;
	G_LeaveVehicle( ent, qfalse ); // clears m_iVehicleNum as well
	ent->client->ps.emplacedIndex = 0;
	//ent->client->ps.m_iVehicleNum = 0;
	ent->client->ps.viewangles[ROLL] = 0.0f;
	ent->client->ps.forceHandExtend = HANDEXTEND_NONE;
	ent->client->ps.forceHandExtendTime = 0;
	ent->client->ps.zoomMode = 0;
	ent->client->ps.zoomLocked = 0;
	ent->client->ps.zoomLockTime = 0;
	ent->client->ps.saberMove = LS_NONE;
	ent->client->ps.legsAnim = 0;
	ent->client->ps.legsTimer = 0;
	ent->client->ps.torsoAnim = 0;
	ent->client->ps.torsoTimer = 0;
	ent->client->ps.isJediMaster = qfalse; // major exploit if you are spectating somebody and they are JM and you reconnect
	ent->client->ps.cloakFuel = 100; // so that fuel goes away after stop following them
	ent->client->ps.jetpackFuel = 100; // so that fuel goes away after stop following them
	ent->health = ent->client->ps.stats[STAT_HEALTH] = 100; // so that you don't keep dead angles if you were spectating a dead person
	ent->client->ps.bobCycle = 0;
	ent->client->ps.pm_type = PM_SPECTATOR;
	ent->client->ps.eFlags &= ~EF_DISINTEGRATION;
	for ( i=0; i<PW_NUM_POWERUPS; i++ )
		ent->client->ps.powerups[i] = 0;
}

/*
=================
Cmd_Team_f
=================
*/
void Cmd_Team_f( gentity_t *ent ) {
	int			oldTeam;
	char		s[MAX_TOKEN_CHARS];

	oldTeam = ent->client->sess.sessionTeam;

	if ( trap->Argc() != 2 ) {		
		switch ( oldTeam ) {
		case TEAM_BLUE:
			trap->SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PRINTBLUETEAM")) );
			break;
		case TEAM_RED:
			trap->SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PRINTREDTEAM")) );
			break;
		case TEAM_FREE:
			trap->SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PRINTFREETEAM")) );
			break;
		case TEAM_SPECTATOR:
			trap->SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PRINTSPECTEAM")) );
			break;
		}
		return;
	}

	if ( ent->client->switchTeamTime > level.time ) {
		trap->SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOSWITCH")) );
		return;
	}

	if (gEscaping)
	{
		return;
	}

	// if they are playing a tournament game, count as a loss
	if ( level.gametype == GT_DUEL
		&& ent->client->sess.sessionTeam == TEAM_FREE ) {//in a tournament game
		//disallow changing teams
		trap->SendServerCommand( ent-g_entities, "print \"Cannot switch teams in Duel\n\"" );
		return;
		//FIXME: why should this be a loss???
		//ent->client->sess.losses++;
	}

	if (level.gametype == GT_POWERDUEL)
	{ //don't let clients change teams manually at all in powerduel, it will be taken care of through automated stuff
		trap->SendServerCommand( ent-g_entities, "print \"Cannot switch teams in Power Duel\n\"" );
		return;
	}

	trap->Argv( 1, s, sizeof( s ) );

	SetTeam( ent, s , qfalse);

	// fix: update team switch time only if team change really happend
	if (oldTeam != ent->client->sess.sessionTeam)
		ent->client->switchTeamTime = level.time + 5000;
}

/*
=================
Cmd_DuelTeam_f
=================
*/
void Cmd_DuelTeam_f(gentity_t *ent)
{
	int			oldTeam;
	char		s[MAX_TOKEN_CHARS];

	if (level.gametype != GT_POWERDUEL)
	{ //don't bother doing anything if this is not power duel
		return;
	}

	/*
	if (ent->client->sess.sessionTeam != TEAM_SPECTATOR)
	{
		trap->SendServerCommand( ent-g_entities, va("print \"You cannot change your duel team unless you are a spectator.\n\""));
		return;
	}
	*/

	if ( trap->Argc() != 2 )
	{ //No arg so tell what team we're currently on.
		oldTeam = ent->client->sess.duelTeam;
		switch ( oldTeam )
		{
		case DUELTEAM_FREE:
			trap->SendServerCommand( ent-g_entities, va("print \"None\n\"") );
			break;
		case DUELTEAM_LONE:
			trap->SendServerCommand( ent-g_entities, va("print \"Single\n\"") );
			break;
		case DUELTEAM_DOUBLE:
			trap->SendServerCommand( ent-g_entities, va("print \"Double\n\"") );
			break;
		default:
			break;
		}
		return;
	}

	if ( ent->client->switchDuelTeamTime > level.time )
	{ //debounce for changing
		trap->SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOSWITCH")) );
		return;
	}

	trap->Argv( 1, s, sizeof( s ) );

	oldTeam = ent->client->sess.duelTeam;

	if (!Q_stricmp(s, "free"))
	{
		ent->client->sess.duelTeam = DUELTEAM_FREE;
	}
	else if (!Q_stricmp(s, "single"))
	{
		ent->client->sess.duelTeam = DUELTEAM_LONE;
	}
	else if (!Q_stricmp(s, "double"))
	{
		ent->client->sess.duelTeam = DUELTEAM_DOUBLE;
	}
	else
	{
		trap->SendServerCommand( ent-g_entities, va("print \"'%s' not a valid duel team.\n\"", s) );
	}

	if (oldTeam == ent->client->sess.duelTeam)
	{ //didn't actually change, so don't care.
		return;
	}

	if (ent->client->sess.sessionTeam != TEAM_SPECTATOR)
	{ //ok..die
		int curTeam = ent->client->sess.duelTeam;
		ent->client->sess.duelTeam = oldTeam;
		G_Damage(ent, ent, ent, NULL, ent->client->ps.origin, 99999, DAMAGE_NO_PROTECTION, MOD_SUICIDE);
		ent->client->sess.duelTeam = curTeam;
	}
	//reset wins and losses
	ent->client->sess.wins = 0;
	ent->client->sess.losses = 0;

	//get and distribute relevent paramters
	if ( ClientUserinfoChanged( ent->s.number ) )
		return;

	ent->client->switchDuelTeamTime = level.time + 5000;
}

int G_TeamForSiegeClass(const char *clName)
{
	int i = 0;
	int team = SIEGETEAM_TEAM1;
	siegeTeam_t *stm = BG_SiegeFindThemeForTeam(team);
	siegeClass_t *scl;

	if (!stm)
	{
		return 0;
	}

	while (team <= SIEGETEAM_TEAM2)
	{
		scl = stm->classes[i];

		if (scl && scl->name[0])
		{
			if (!Q_stricmp(clName, scl->name))
			{
				return team;
			}
		}

		i++;
		if (i >= MAX_SIEGE_CLASSES || i >= stm->numClasses)
		{
			if (team == SIEGETEAM_TEAM2)
			{
				break;
			}
			team = SIEGETEAM_TEAM2;
			stm = BG_SiegeFindThemeForTeam(team);
			i = 0;
		}
	}

	return 0;
}

/*
=================
Cmd_SiegeClass_f
=================
*/
void Cmd_SiegeClass_f( gentity_t *ent )
{
	char className[64];
	int team = 0;
	int preScore;
	qboolean startedAsSpec = qfalse;

	if (level.gametype != GT_SIEGE)
	{ //classes are only valid for this gametype
		return;
	}

	if (!ent->client)
	{
		return;
	}

	if (trap->Argc() < 1)
	{
		return;
	}

	if ( ent->client->switchClassTime > level.time )
	{
		trap->SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOCLASSSWITCH")) );
		return;
	}

	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		startedAsSpec = qtrue;
	}

	trap->Argv( 1, className, sizeof( className ) );

	team = G_TeamForSiegeClass(className);

	if (!team)
	{ //not a valid class name
		return;
	}

	if (ent->client->sess.sessionTeam != team)
	{ //try changing it then
		g_preventTeamBegin = qtrue;
		if (team == TEAM_RED)
		{
			SetTeam(ent, "red", qfalse);
		}
		else if (team == TEAM_BLUE)
		{
			SetTeam(ent, "blue", qfalse);
		}
		g_preventTeamBegin = qfalse;

		if (ent->client->sess.sessionTeam != team)
		{ //failed, oh well
			if (ent->client->sess.sessionTeam != TEAM_SPECTATOR ||
				ent->client->sess.siegeDesiredTeam != team)
			{
				trap->SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOCLASSTEAM")) );
				return;
			}
		}
	}

	//preserve 'is score
	preScore = ent->client->ps.persistant[PERS_SCORE];

	//Make sure the class is valid for the team
	BG_SiegeCheckClassLegality(team, className);

	//Set the session data
	strcpy(ent->client->sess.siegeClass, className);

	// get and distribute relevent paramters
	if ( !ClientUserinfoChanged( ent->s.number ) )
		return;

	if (ent->client->tempSpectate < level.time)
	{
		// Kill him (makes sure he loses flags, etc)
		if (ent->health > 0 && !startedAsSpec)
		{
			ent->flags &= ~FL_GODMODE;
			ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
			player_die (ent, ent, ent, 100000, MOD_SUICIDE);
		}

		if (ent->client->sess.sessionTeam == TEAM_SPECTATOR || startedAsSpec)
		{ //respawn them instantly.
			ClientBegin( ent->s.number, qfalse );
		}
	}
	//set it back after we do all the stuff
	ent->client->ps.persistant[PERS_SCORE] = preScore;

	ent->client->switchClassTime = level.time + 5000;
}

/*
=================
Cmd_ForceChanged_f
=================
*/
void Cmd_ForceChanged_f( gentity_t *ent )
{
	char fpChStr[1024];
	const char *buf;
//	Cmd_Kill_f(ent);
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{ //if it's a spec, just make the changes now
		//trap->SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "FORCEAPPLIED")) );
		//No longer print it, as the UI calls this a lot.
		WP_InitForcePowers( ent );
		goto argCheck;
	}

	buf = G_GetStringEdString("MP_SVGAME", "FORCEPOWERCHANGED");

	strcpy(fpChStr, buf);

	trap->SendServerCommand( ent-g_entities, va("print \"%s%s\n\"", S_COLOR_GREEN, fpChStr) );

	ent->client->ps.fd.forceDoInit = 1;
argCheck:
	if (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
	{ //If this is duel, don't even bother changing team in relation to this.
		return;
	}

	if (trap->Argc() > 1)
	{
		char	arg[MAX_TOKEN_CHARS];

		trap->Argv( 1, arg, sizeof( arg ) );

		if ( arg[0] )
		{ //if there's an arg, assume it's a combo team command from the UI.
			Cmd_Team_f(ent);
		}
	}
}

extern qboolean WP_SaberStyleValidForSaber( saberInfo_t *saber1, saberInfo_t *saber2, int saberHolstered, int saberAnimLevel );
extern qboolean WP_UseFirstValidSaberStyle( saberInfo_t *saber1, saberInfo_t *saber2, int saberHolstered, int *saberAnimLevel );
qboolean G_SetSaber(gentity_t *ent, int saberNum, char *saberName, qboolean siegeOverride)
{
	char truncSaberName[MAX_QPATH] = {0};

	if ( !siegeOverride && level.gametype == GT_SIEGE && ent->client->siegeClass != -1 &&
		(bgSiegeClasses[ent->client->siegeClass].saberStance || bgSiegeClasses[ent->client->siegeClass].saber1[0] || bgSiegeClasses[ent->client->siegeClass].saber2[0]) )
	{ //don't let it be changed if the siege class has forced any saber-related things
		return qfalse;
	}

	Q_strncpyz( truncSaberName, saberName, sizeof( truncSaberName ) );

	if ( saberNum == 0 && (!Q_stricmp( "none", truncSaberName ) || !Q_stricmp( "remove", truncSaberName )) )
	{ //can't remove saber 0 like this
		Q_strncpyz( truncSaberName, DEFAULT_SABER, sizeof( truncSaberName ) );
	}

	//Set the saber with the arg given. If the arg is
	//not a valid sabername defaults will be used.
	WP_SetSaber( ent->s.number, ent->client->saber, saberNum, truncSaberName );

	if ( !ent->client->saber[0].model[0] )
	{
		assert(0); //should never happen!
		Q_strncpyz( ent->client->pers.saber1, DEFAULT_SABER, sizeof( ent->client->pers.saber1 ) );
	}
	else
		Q_strncpyz( ent->client->pers.saber1, ent->client->saber[0].name, sizeof( ent->client->pers.saber1 ) );

	if ( !ent->client->saber[1].model[0] )
		Q_strncpyz( ent->client->pers.saber2, "none", sizeof( ent->client->pers.saber2 ) );
	else
		Q_strncpyz( ent->client->pers.saber2, ent->client->saber[1].name, sizeof( ent->client->pers.saber2 ) );

	if ( !WP_SaberStyleValidForSaber( &ent->client->saber[0], &ent->client->saber[1], ent->client->ps.saberHolstered, ent->client->ps.fd.saberAnimLevel ) )
	{
		WP_UseFirstValidSaberStyle( &ent->client->saber[0], &ent->client->saber[1], ent->client->ps.saberHolstered, &ent->client->ps.fd.saberAnimLevel );
		ent->client->ps.fd.saberAnimLevelBase = ent->client->saberCycleQueue = ent->client->ps.fd.saberAnimLevel;
	}

	return qtrue;
}

/*
=================
Cmd_Follow_f
=================
*/
void Cmd_Follow_f( gentity_t *ent ) {
	int		i;
	char	arg[MAX_TOKEN_CHARS];

	if ( ent->client->sess.spectatorState == SPECTATOR_NOT && ent->client->switchTeamTime > level.time ) {
		trap->SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOSWITCH")) );
		return;
	}

	if ( trap->Argc() != 2 ) {
		if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
			StopFollowing( ent );
		}
		return;
	}

	trap->Argv( 1, arg, sizeof( arg ) );
	i = JP_ClientNumberFromString( ent, arg );
	if ( i == -1 || i == -2) {
		return;
	}

	// can't follow self
	if ( &level.clients[ i ] == ent->client ) {
		return;
	}

	// can't follow another spectator
	if ( level.clients[ i ].sess.sessionTeam == TEAM_SPECTATOR ) {
		return;
	}

	if ( level.clients[ i ].tempSpectate >= level.time ) {
		return;
	}

	// if they are playing a tournament game, count as a loss
	if ( (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
		&& ent->client->sess.sessionTeam == TEAM_FREE ) {
		//WTF???
		ent->client->sess.losses++;
	}

	// first set them to spectator
	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		SetTeam( ent, "spectator" , qfalse);
		// fix: update team switch time only if team change really happend
		if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
			ent->client->switchTeamTime = level.time + 5000;
	}

	ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
	ent->client->sess.spectatorClient = i;
}

/*
=================
Cmd_FollowCycle_f
=================
*/
void Cmd_FollowCycle_f( gentity_t *ent, int dir ) {
	int		clientnum;
	int		original;
	qboolean	looped = qfalse;

	if ( ent->client->sess.spectatorState == SPECTATOR_NOT && ent->client->switchTeamTime > level.time ) {
		trap->SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOSWITCH")) );
		return;
	}

	// if they are playing a tournament game, count as a loss
	if ( (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
		&& ent->client->sess.sessionTeam == TEAM_FREE ) {\
		//WTF???
		ent->client->sess.losses++;
	}
	// first set them to spectator
	if ( ent->client->sess.spectatorState == SPECTATOR_NOT ) {
		SetTeam( ent, "spectator" , qfalse);
		// fix: update team switch time only if team change really happend
		if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
			ent->client->switchTeamTime = level.time + 5000;
	}

	if ( dir != 1 && dir != -1 ) {
		trap->Error( ERR_DROP, "Cmd_FollowCycle_f: bad dir %i", dir );
	}

	clientnum = ent->client->sess.spectatorClient;
	original = clientnum;

	do {
		clientnum += dir;
		if ( clientnum >= level.maxclients )
		{
			//JAC: Avoid /team follow1 crash
			if ( looped )
			{
				clientnum = original;
				break;
			}
			else
			{
				clientnum = 0;
				looped = qtrue;
			}
		}
		if ( clientnum < 0 ) {
			if ( looped )
			{
				clientnum = original;
				break;
			}
			else
			{
				clientnum = level.maxclients - 1;
				looped = qtrue;
			}
		}

		// can only follow connected clients
		if ( level.clients[ clientnum ].pers.connected != CON_CONNECTED ) {
			continue;
		}

		// can't follow another spectator
		if ( level.clients[ clientnum ].sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}

		// can't follow another spectator
		if ( level.clients[ clientnum ].tempSpectate >= level.time ) {
			return;
		}

		// this is good, we can use it
		ent->client->sess.spectatorClient = clientnum;
		ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
		return;
	} while ( clientnum != original );

	// leave it where it was
}

void Cmd_FollowNext_f( gentity_t *ent ) {
	Cmd_FollowCycle_f( ent, 1 );
}

void Cmd_FollowPrev_f( gentity_t *ent ) {
	Cmd_FollowCycle_f( ent, -1 );
}

/*
==================
G_Say
==================
*/

static void G_SayTo( gentity_t *ent, gentity_t *other, int mode, int color, const char *name, const char *message, char *locMsg )
{
	if (!other) {
		return;
	}
	if (!other->inuse) {
		return;
	}
	if (!other->client) {
		return;
	}
	if ( other->client->pers.connected != CON_CONNECTED ) {
		return;
	}
	if ( mode == SAY_TEAM && ((g_gametype.integer >= GT_TEAM && !OnSameTeam(ent, other)) || (g_gametype.integer < GT_TEAM && (ent->client->sess.sessionTeam != other->client->sess.sessionTeam)))) {
		return;
	}

	if (mode == SAY_CLAN && ((Q_stricmp(ent->client->pers.clanpass, other->client->pers.clanpass) || ent->client->pers.clanpass[0] == 0 || other->client->pers.clanpass[0] == 0)))//Idk
		return;//Ignore it
	if (mode == SAY_ADMIN && !(other->r.svFlags & SVF_FULLADMIN || other->r.svFlags & SVF_FULLADMIN) && ent != other)
		return;

	if (ClientIsIgnored(other-g_entities, ent-g_entities)) {//Also make sure clanpass is set?
		return;
	}
	/*
	// no chatting to players in tournaments
	if ( (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
		&& other->client->sess.sessionTeam == TEAM_FREE
		&& ent->client->sess.sessionTeam != TEAM_FREE ) {
		//Hmm, maybe some option to do so if allowed?  Or at least in developer mode...
		return;
	}
	*/
	//They've requested I take this out.

	if (level.gametype == GT_SIEGE &&
		ent->client && (ent->client->tempSpectate >= level.time || ent->client->sess.sessionTeam == TEAM_SPECTATOR) &&
		other->client->sess.sessionTeam != TEAM_SPECTATOR &&
		other->client->tempSpectate < level.time)
	{ //siege temp spectators should not communicate to ingame players
		return;
	}

	if (locMsg)
	{
		trap->SendServerCommand( other-g_entities, va("%s \"%s\" \"%s\" \"%c\" \"%s\" %i", 
			mode == SAY_TEAM ? "ltchat" : "lchat",
			name, locMsg, color, message, ent->s.number));
	}
	else
	{
		trap->SendServerCommand( other-g_entities, va("%s \"%s%c%c%s\" %i", 
			mode == SAY_TEAM ? "tchat" : "chat",
			name, Q_COLOR_ESCAPE, color, message, ent->s.number));
	}
}

void G_Say( gentity_t *ent, gentity_t *target, int mode, const char *chatText ) {
	int			j;
	gentity_t	*other;
	int			color;
	char		name[64];
	// don't let text be too long for malicious reasons
	char		text[MAX_SAY_TEXT];
	char		location[64];
	char		*locMsg = NULL;

	//if ( level.gametype < GT_TEAM && mode == SAY_TEAM ) {
	//	mode = SAY_ALL;
	//}

	if (mode == SAY_TEAM) {
		if (ent->client->pers.sayteammod == 1)//clanpass
			mode = SAY_CLAN;
		if (ent->client->pers.sayteammod == 2)//clanpass
			mode = SAY_ADMIN;
	}

	Q_strncpyz( text, chatText, sizeof(text) );

	Q_strstrip( text, "\n\r", "  " );

	switch ( mode ) {
	default:
	case SAY_ALL:
		G_LogPrintf( "say: %s: %s\n", ent->client->pers.netname, text );
		Com_sprintf (name, sizeof(name), "%s%c%c"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_GREEN;
		break;
	case SAY_TEAM:
		G_LogPrintf( "sayteam: %s: %s\n", ent->client->pers.netname, text );
		if (Team_GetLocationMsg(ent, location, sizeof(location)))
		{
			Com_sprintf (name, sizeof(name), EC"(%s%c%c"EC")"EC": ", 
				ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
			locMsg = location;
		}
		else
		{
			Com_sprintf (name, sizeof(name), EC"(%s%c%c"EC")"EC": ", 
				ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		}
		color = COLOR_CYAN;
		break;
	case SAY_TELL:
		if (target && target->inuse && target->client && level.gametype >= GT_TEAM &&
			target->client->sess.sessionTeam == ent->client->sess.sessionTeam &&
			Team_GetLocationMsg(ent, location, sizeof(location)))
		{
			Com_sprintf (name, sizeof(name), EC"[%s%c%c"EC"]"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
			locMsg = location;
		}
		else
		{
			Com_sprintf (name, sizeof(name), EC"[%s%c%c"EC"]"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		}
		color = COLOR_MAGENTA;
		break;
	case SAY_CLAN:
		G_LogPrintf( "sayclan: %s: %s\n", ent->client->pers.netname, chatText );
		if (Team_GetLocationMsg(ent, location, sizeof(location)))
		{
			Com_sprintf (name, sizeof(name), EC"^1<Clan>^7(%s%c%c"EC")"EC": ", 
				ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
			locMsg = location;
		}
		else
		{
			Com_sprintf (name, sizeof(name), EC"^1<Clan>^7(%s%c%c"EC")"EC": ", 
				ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		}
		color = COLOR_RED;
		break;
	case SAY_ADMIN:
		G_LogPrintf( "sayadmin: %s: %s\n", ent->client->pers.netname, chatText );
		if (Team_GetLocationMsg(ent, location, sizeof(location)))
		{
			Com_sprintf (name, sizeof(name), EC"^3<Admin>^7(%s%c%c"EC")"EC": ", 
				ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
			locMsg = location;
		}
		else
		{
			Com_sprintf (name, sizeof(name), EC"^3<Admin>^7(%s%c%c"EC")"EC": ", 
				ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		}
		color = COLOR_YELLOW;
		break;
	}

	if ( target ) {
		G_SayTo( ent, target, mode, color, name, text, locMsg );
		return;
	}

	// echo the text to the console
	if ( dedicated.integer ) {
		trap->Print( "%s%s\n", name, text);
	}

	// send it to all the appropriate clients
	for (j = 0; j < level.maxclients; j++) {
		other = &g_entities[j];
		G_SayTo( ent, other, mode, color, name, text, locMsg );
	}
}


/*
==================
Cmd_Say_f
==================
*/
static void Cmd_Say_f( gentity_t *ent ) {
	char *p = NULL;

	if ( trap->Argc () < 2 )
		return;

	p = ConcatArgs( 1 );

	if ( strlen( p ) >= MAX_SAY_TEXT ) {
		p[MAX_SAY_TEXT-1] = '\0';
		G_SecurityLogPrintf( "Cmd_Say_f from %d (%s) has been truncated: %s\n", ent->s.number, ent->client->pers.netname, p );
	}

	G_Say( ent, NULL, SAY_ALL, p );
}

static void Cmd_Clansay_f( gentity_t *ent ) {
	char *p = NULL;

	if ( trap->Argc () < 2 )
		return;

	p = ConcatArgs( 1 );

	//Raz: BOF
	if ( strlen( p ) > MAX_SAY_TEXT )
	{
		p[MAX_SAY_TEXT-1] = '\0';
		G_SecurityLogPrintf( "Cmd_Say_f from %d (%s) has been truncated: %s\n", ent->s.number, ent->client->pers.netname, p );
	}

	G_Say( ent, NULL, SAY_CLAN, p );
}

static void Cmd_Amsay_f( gentity_t *ent ) {
	char *p = NULL;

	if ( trap->Argc () < 2 )
		return;

	p = ConcatArgs( 1 );

	//Raz: BOF
	if ( strlen( p ) > MAX_SAY_TEXT )
	{
		p[MAX_SAY_TEXT-1] = '\0';
		G_SecurityLogPrintf( "Cmd_Say_f from %d (%s) has been truncated: %s\n", ent->s.number, ent->client->pers.netname, p );
	}

	G_Say( ent, NULL, SAY_ADMIN, p );
}

/*
==================
Cmd_SayTeam_f
==================
*/
static void Cmd_SayTeam_f( gentity_t *ent ) {
	char *p = NULL;

	if ( trap->Argc () < 2 )
		return;

	p = ConcatArgs( 1 );

	if ( strlen( p ) >= MAX_SAY_TEXT ) {
		p[MAX_SAY_TEXT-1] = '\0';
		G_SecurityLogPrintf( "Cmd_SayTeam_f from %d (%s) has been truncated: %s\n", ent->s.number, ent->client->pers.netname, p );
	}

	//G_Say( ent, NULL, (level.gametype>=GT_TEAM) ? SAY_TEAM : SAY_ALL, p );
	G_Say( ent, NULL, SAY_TEAM, p );
}

/*
==================
Cmd_Tell_f
==================
*/
static void Cmd_Tell_f( gentity_t *ent ) {
	int			targetNum;
	gentity_t	*target;
	char		*p;
	char		arg[MAX_TOKEN_CHARS];

	if ( trap->Argc () < 3 ) {
		trap->SendServerCommand( ent-g_entities, "print \"Usage: tell <player id> <message>\n\"" );
		return;
	}

	trap->Argv( 1, arg, sizeof( arg ) );
	targetNum = JP_ClientNumberFromString( ent, arg );
	if ( targetNum == -1 || targetNum == -2) {
		return;
	}

	target = &g_entities[targetNum];
	if ( !target->inuse || !target->client ) {
		return;
	}

	p = ConcatArgs( 2 );

	if ( strlen( p ) >= MAX_SAY_TEXT ) {
		p[MAX_SAY_TEXT-1] = '\0';
		G_SecurityLogPrintf( "Cmd_Tell_f from %d (%s) has been truncated: %s\n", ent->s.number, ent->client->pers.netname, p );
	}

	G_LogPrintf( "tell: %s to %s: %s\n", ent->client->pers.netname, target->client->pers.netname, p );
	G_Say( ent, target, SAY_TELL, p );
	// don't tell to the player self if it was already directed to this player
	// also don't send the chat back to a bot
	if ( ent != target && !(ent->r.svFlags & SVF_BOT)) {
		G_Say( ent, ent, SAY_TELL, p );
	}
}

//siege voice command
static void Cmd_VoiceCommand_f(gentity_t *ent)
{
	gentity_t *te;
	char arg[MAX_TOKEN_CHARS];
	char *s;
	int i = 0;

	if (level.gametype < GT_TEAM)
	{
		return;
	}

	if (trap->Argc() < 2)
	{
		return;
	}

	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR ||
		ent->client->tempSpectate >= level.time)
	{
		trap->SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOICECHATASSPEC")) );
		return;
	}

	trap->Argv(1, arg, sizeof(arg));

	if (arg[0] == '*')
	{ //hmm.. don't expect a * to be prepended already. maybe someone is trying to be sneaky.
		return;
	}

	s = va("*%s", arg);

	//now, make sure it's a valid sound to be playing like this.. so people can't go around
	//screaming out death sounds or whatever.
	while (i < MAX_CUSTOM_SIEGE_SOUNDS)
	{
		if (!bg_customSiegeSoundNames[i])
		{
			break;
		}
		if (!Q_stricmp(bg_customSiegeSoundNames[i], s))
		{ //it matches this one, so it's ok
			break;
		}
		i++;
	}

	if (i == MAX_CUSTOM_SIEGE_SOUNDS || !bg_customSiegeSoundNames[i])
	{ //didn't find it in the list
		return;
	}

	te = G_TempEntity(vec3_origin, EV_VOICECMD_SOUND);
	te->s.groundEntityNum = ent->s.number;
	te->s.eventParm = G_SoundIndex((char *)bg_customSiegeSoundNames[i]);
	te->r.svFlags |= SVF_BROADCAST;
}


static char	*gc_orders[] = {
	"hold your position",
	"hold this position",
	"come here",
	"cover me",
	"guard location",
	"search and destroy",
	"report"
};
static size_t numgc_orders = ARRAY_LEN( gc_orders );

void Cmd_GameCommand_f( gentity_t *ent ) {
	int				targetNum;
	unsigned int	order;
	gentity_t		*target;
	char			arg[MAX_TOKEN_CHARS] = {0};

	if ( trap->Argc() != 3 ) {
		trap->SendServerCommand( ent-g_entities, va( "print \"Usage: gc <player id> <order 0-%d>\n\"", numgc_orders - 1 ) );
		return;
	}

	trap->Argv( 2, arg, sizeof( arg ) );
	order = atoi( arg );

	if ( order >= numgc_orders ) {
		trap->SendServerCommand( ent-g_entities, va("print \"Bad order: %i\n\"", order));
		return;
	}

	trap->Argv( 1, arg, sizeof( arg ) );
	targetNum = JP_ClientNumberFromString( ent, arg );
	if ( targetNum == -1 || targetNum == -2)
		return;

	target = &g_entities[targetNum];
	if ( !target->inuse || !target->client )
		return;

	G_LogPrintf( "tell: %s to %s: %s\n", ent->client->pers.netname, target->client->pers.netname, gc_orders[order] );
	G_Say( ent, target, SAY_TELL, gc_orders[order] );
	// don't tell to the player self if it was already directed to this player
	// also don't send the chat back to a bot
	if ( ent != target && !(ent->r.svFlags & SVF_BOT) )
		G_Say( ent, ent, SAY_TELL, gc_orders[order] );
}

/*
==================
Cmd_Where_f
==================
*/
void Cmd_Where_f( gentity_t *ent ) {
	//JAC: This wasn't working for non-spectators since s.origin doesn't update for active players.
	if(ent->client && ent->client->sess.sessionTeam != TEAM_SPECTATOR )
	{//active players use currentOrigin
		trap->SendServerCommand( ent-g_entities, va("print \"%s\n\"", vtos( ent->r.currentOrigin ) ) );
	}
	else
	{
		trap->SendServerCommand( ent-g_entities, va("print \"%s\n\"", vtos( ent->s.origin ) ) );
	}
	//trap->SendServerCommand( ent-g_entities, va("print \"%s\n\"", vtos( ent->s.origin ) ) );
}

static const char *gameNames[] = {
	"Free For All",
	"Holocron FFA",
	"Jedi Master",
	"Duel",
	"Power Duel",
	"Single Player",
	"Team FFA",
	"Siege",
	"Capture the Flag",
	"Capture the Ysalamiri"
};

/*
==================
Cmd_CallVote_f
==================
*/
extern void SiegeClearSwitchData(void); //g_saga.c

qboolean G_VoteCapturelimit( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int n = Com_Clampi( 0, 0x7FFFFFFF, atoi( arg2 ) );
	Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %i", arg1, n );
	Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

qboolean G_VoteClientkick( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int n = atoi ( arg2 );

	if ( n < 0 || n >= level.maxclients ) {
		trap->SendServerCommand( ent-g_entities, va( "print \"invalid client number %d.\n\"", n ) );
		return qfalse;
	}

	if ( g_entities[n].client->pers.connected == CON_DISCONNECTED ) {
		trap->SendServerCommand( ent-g_entities, va( "print \"there is no client with the client number %d.\n\"", n ) );
		return qfalse;
	}

	Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s", arg1, arg2 );
	Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s %s", arg1, g_entities[n].client->pers.netname );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

qboolean G_VoteFraglimit( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int n = Com_Clampi( 0, 0x7FFFFFFF, atoi( arg2 ) );
	Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %i", arg1, n );
	Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

qboolean G_VoteGametype( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int gt = atoi( arg2 );

	// ffa, ctf, tdm, etc
	if ( arg2[0] && isalpha( arg2[0] ) ) {
		gt = BG_GetGametypeForString( arg2 );
		if ( gt == -1 )
		{
			trap->SendServerCommand( ent-g_entities, va( "print \"Gametype (%s) unrecognised, defaulting to FFA/Deathmatch\n\"", arg2 ) );
			gt = GT_FFA;
		}
	}
	// numeric but out of range
	else if ( gt < 0 || gt >= GT_MAX_GAME_TYPE ) {
		trap->SendServerCommand( ent-g_entities, va( "print \"Gametype (%i) is out of range, defaulting to FFA/Deathmatch\n\"", gt ) );
		gt = GT_FFA;
	}

	// logically invalid gametypes, or gametypes not fully implemented in MP
	if ( gt == GT_SINGLE_PLAYER ) {
		trap->SendServerCommand( ent-g_entities, va( "print \"This gametype is not supported (%s).\n\"", arg2 ) );
		return qfalse;
	}

	level.votingGametype = qtrue;
	level.votingGametypeTo = gt;

	Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %d", arg1, gt );
	Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s %s", arg1, gameNames[gt] );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

qboolean G_VoteKick( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int clientid = JP_ClientNumberFromString( ent, arg2 );
	gentity_t *target = NULL;

	if (clientid == -1 || clientid == -2)
		return qfalse;

	target = &g_entities[clientid];
	if ( !target || !target->inuse || !target->client )
		return qfalse;

	Com_sprintf( level.voteString, sizeof( level.voteString ), "clientkick %d", clientid );
	Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "kick %s", target->client->pers.netname );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

const char *G_GetArenaInfoByMap( const char *map );

void Cmd_MapList_f( gentity_t *ent ) {
	int i, toggle=0;
	char map[24] = "--", buf[512] = {0};

	Q_strcat( buf, sizeof( buf ), "Map list:" );

	for ( i=0; i<level.arenas.num; i++ ) {
		Q_strncpyz( map, Info_ValueForKey( level.arenas.infos[i], "map" ), sizeof( map ) );
		Q_StripColor( map );

		if ( G_DoesMapSupportGametype( map, level.gametype ) ) {
			char *tmpMsg = va( " ^%c%s", (++toggle&1) ? COLOR_GREEN : COLOR_YELLOW, map );
			if ( strlen( buf ) + strlen( tmpMsg ) >= sizeof( buf ) ) {
				trap->SendServerCommand( ent-g_entities, va( "print \"%s\"", buf ) );
				buf[0] = '\0';
			}
			Q_strcat( buf, sizeof( buf ), tmpMsg );
		}
	}

	trap->SendServerCommand( ent-g_entities, va( "print \"%s\n\"", buf ) );
}

qboolean G_VoteMap( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	char s[MAX_CVAR_VALUE_STRING] = {0}, bspName[MAX_QPATH] = {0}, *mapName = NULL, *mapName2 = NULL;
	fileHandle_t fp = NULL_FILE;
	const char *arenaInfo;

	// didn't specify a map, show available maps
	if ( numArgs < 3 ) {
		Cmd_MapList_f( ent );
		return qfalse;
	}

	if ( strchr( arg2, '\\' ) ) {
		trap->SendServerCommand( ent-g_entities, "print \"Can't have mapnames with a \\\n\"" );
		return qfalse;
	}

	Com_sprintf( bspName, sizeof(bspName), "maps/%s.bsp", arg2 );
	if ( trap->FS_Open( bspName, &fp, FS_READ ) <= 0 ) {
		trap->SendServerCommand( ent-g_entities, va( "print \"Can't find map %s on server\n\"", bspName ) );
		if( fp != NULL_FILE )
			trap->FS_Close( fp );
		return qfalse;
	}
	trap->FS_Close( fp );

	if ( !G_DoesMapSupportGametype( arg2, level.gametype ) ) {
		trap->SendServerCommand( ent-g_entities, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "NOVOTE_MAPNOTSUPPORTEDBYGAME" ) ) );
		return qfalse;
	}

	// preserve the map rotation
	trap->Cvar_VariableStringBuffer( "nextmap", s, sizeof( s ) );
	if ( *s )
		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s; set nextmap \"%s\"", arg1, arg2, s );
	else
		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s", arg1, arg2 );

	arenaInfo = G_GetArenaInfoByMap(arg2);
	if ( arenaInfo ) {
		mapName = Info_ValueForKey( arenaInfo, "longname" );
		mapName2 = Info_ValueForKey( arenaInfo, "map" );
	}

	if ( !mapName || !mapName[0] )
		mapName = "ERROR";

	if ( !mapName2 || !mapName2[0] )
		mapName2 = "ERROR";

	Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "map %s (%s)", mapName, mapName2 );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

qboolean G_VoteMapRestart( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int n = Com_Clampi( 0, 60, atoi( arg2 ) );
	if ( numArgs < 3 )
		n = 5;
	Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %i", arg1, n );
	Q_strncpyz( level.voteDisplayString, level.voteString, sizeof( level.voteDisplayString ) );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

qboolean G_VoteNextmap( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	char s[MAX_CVAR_VALUE_STRING];

	trap->Cvar_VariableStringBuffer( "nextmap", s, sizeof( s ) );
	if ( !*s ) {
		trap->SendServerCommand( ent-g_entities, "print \"nextmap not set.\n\"" );
		return qfalse;
	}
	SiegeClearSwitchData();
	Com_sprintf( level.voteString, sizeof( level.voteString ), "vstr nextmap");
	Q_strncpyz( level.voteDisplayString, level.voteString, sizeof( level.voteDisplayString ) );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

qboolean G_VoteTimelimit( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	float tl = Com_Clamp( 0.0f, 35790.0f, atof( arg2 ) );
	if ( Q_isintegral( tl ) )
		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %i", arg1, (int)tl );
	else
		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %.3f", arg1, tl );
	Q_strncpyz( level.voteDisplayString, level.voteString, sizeof( level.voteDisplayString ) );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

qboolean G_VoteWarmup( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int n = Com_Clampi( 0, 1, atoi( arg2 ) );
	Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %i", arg1, n );
	Q_strncpyz( level.voteDisplayString, level.voteString, sizeof( level.voteDisplayString ) );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

typedef struct voteString_s {
	const char	*string;
	const char	*aliases;	// space delimited list of aliases, will always show the real vote string
	qboolean	(*func)(gentity_t *ent, int numArgs, const char *arg1, const char *arg2);
	int			numArgs;	// number of REQUIRED arguments, not total/optional arguments
	uint32_t	validGT;	// bit-flag of valid gametypes
	qboolean	voteDelay;	// if true, will delay executing the vote string after it's accepted by g_voteDelay
	const char	*shortHelp;	// NULL if no arguments needed
} voteString_t;

static voteString_t validVoteStrings[] = {
	//	vote string				aliases										# args	valid gametypes							exec delay		short help
	{	"capturelimit",			"caps",				G_VoteCapturelimit,		1,		GTB_CTF|GTB_CTY,						qtrue,			"<num>" },
	{	"clientkick",			NULL,				G_VoteClientkick,		1,		GTB_ALL,								qfalse,			"<clientnum>" },
	{	"fraglimit",			"frags",			G_VoteFraglimit,		1,		GTB_ALL & ~(GTB_SIEGE|GTB_CTF|GTB_CTY),	qtrue,			"<num>" },
	{	"g_doWarmup",			"dowarmup warmup",	G_VoteWarmup,			1,		GTB_ALL,								qtrue,			"<0-1>" },
	{	"g_gametype",			"gametype gt mode",	G_VoteGametype,			1,		GTB_ALL,								qtrue,			"<num or name>" },
	{	"kick",					NULL,				G_VoteKick,				1,		GTB_ALL,								qfalse,			"<client name>" },
	{	"map",					NULL,				G_VoteMap,				0,		GTB_ALL,								qtrue,			"<name>" },
	{	"map_restart",			"restart",			G_VoteMapRestart,		0,		GTB_ALL,								qtrue,			"<optional delay>" },
	{	"nextmap",				NULL,				G_VoteNextmap,			0,		GTB_ALL,								qtrue,			NULL },
	{	"timelimit",			"time",				G_VoteTimelimit,		1,		GTB_ALL,								qtrue,			"<num>" },
};
static const int validVoteStringsSize = ARRAY_LEN( validVoteStrings );

void Cmd_CallVote_f( gentity_t *ent ) {
	int				i=0, numArgs=0;
	char			arg1[MAX_CVAR_VALUE_STRING] = {0};
	char			arg2[MAX_CVAR_VALUE_STRING] = {0};
	voteString_t	*vote = NULL;

	// not allowed to vote at all
	if ( !g_allowVote.integer ) {
		trap->SendServerCommand( ent-g_entities, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "NOVOTE" ) ) );
		return;
	}

	// vote in progress
	else if ( level.voteTime ) {
		trap->SendServerCommand( ent-g_entities, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "VOTEINPROGRESS" ) ) );
		return;
	}

	// can't vote as a spectator, except in (power)duel
	else if ( level.gametype != GT_DUEL && level.gametype != GT_POWERDUEL && ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap->SendServerCommand( ent-g_entities, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "NOSPECVOTE" ) ) );
		return;
	}

	// make sure it is a valid command to vote on
	numArgs = trap->Argc();
	trap->Argv( 1, arg1, sizeof( arg1 ) );
	if ( numArgs > 1 )
		Q_strncpyz( arg2, ConcatArgs( 2 ), sizeof( arg2 ) );

	// filter ; \n \r
	if ( Q_strchrs( arg1, ";\r\n" ) || Q_strchrs( arg2, ";\r\n" ) ) {
		trap->SendServerCommand( ent-g_entities, "print \"Invalid vote string.\n\"" );
		return;
	}

	if (!Q_stricmp(arg1, "g_gametype")) {
		if (g_votesDisable.integer & VOTE_GAMETYPE)
			return;
	}
	else if (!Q_stricmp(arg1, "map")) {
		if (g_votesDisable.integer & VOTE_MAP)
			return;
	}
	else if ((!Q_stricmp(arg1, "clientkick")) || (!Q_stricmp(arg1, "kick"))) {
		if (g_votesDisable.integer & VOTE_KICK)
			return;
	}
	else if (!Q_stricmp(arg1, "nextmap")) {
		if (g_votesDisable.integer & VOTE_NEXTMAP)
			return;
	}
	else if (!Q_stricmp(arg1, "timelimit")) {
		if (g_votesDisable.integer & VOTE_TIMELIMIT)
			return;
	}
	else if (!Q_stricmp( arg1, "fraglimit")) {
		if (g_votesDisable.integer & VOTE_FRAGLIMIT)
			return;
	}
	else if (!Q_stricmp( arg1, "capturelimit")) {
		if (g_votesDisable.integer & VOTE_CAPTURELIMIT)
			return;
	}

	// check for invalid votes
	for ( i=0; i<validVoteStringsSize; i++ ) {
		if ( !(g_allowVote.integer & (1<<i)) )
			continue;

		if ( !Q_stricmp( arg1, validVoteStrings[i].string ) )
			break;

		// see if they're using an alias, and set arg1 to the actual vote string
		if ( validVoteStrings[i].aliases ) {
			char tmp[MAX_TOKEN_CHARS] = {0}, *p = NULL;
			const char *delim = " ";
			Q_strncpyz( tmp, validVoteStrings[i].aliases, sizeof( tmp ) );
			p = strtok( tmp, delim );
			while ( p != NULL ) {
				if ( !Q_stricmp( arg1, p ) ) {
					Q_strncpyz( arg1, validVoteStrings[i].string, sizeof( arg1 ) );
					goto validVote;
				}
				p = strtok( NULL, delim );
			}
		}
	}
	// invalid vote string, abandon ship
	if ( i == validVoteStringsSize ) {
		char buf[1024] = {0};
		int toggle = 0;
		trap->SendServerCommand( ent-g_entities, "print \"Invalid vote string.\n\"" );
		trap->SendServerCommand( ent-g_entities, "print \"Allowed vote strings are: \"" );
		for ( i=0; i<validVoteStringsSize; i++ ) {
			if ( !(g_allowVote.integer & (1<<i)) )
				continue;

			toggle = !toggle;
			if ( validVoteStrings[i].shortHelp ) {
				Q_strcat( buf, sizeof( buf ), va( "^%c%s %s ",
					toggle ? COLOR_GREEN : COLOR_YELLOW,
					validVoteStrings[i].string,
					validVoteStrings[i].shortHelp ) );
			}
			else {
				Q_strcat( buf, sizeof( buf ), va( "^%c%s ",
					toggle ? COLOR_GREEN : COLOR_YELLOW,
					validVoteStrings[i].string ) );
			}
		}

		//FIXME: buffer and send in multiple messages in case of overflow
		trap->SendServerCommand( ent-g_entities, va( "print \"%s\n\"", buf ) );
		return;
	}

validVote:
	vote = &validVoteStrings[i];
	if ( !(vote->validGT & (1<<level.gametype)) ) {
		trap->SendServerCommand( ent-g_entities, va( "print \"%s is not applicable in this gametype.\n\"", arg1 ) );
		return;
	}

	if ( numArgs < vote->numArgs+2 ) {
		trap->SendServerCommand( ent-g_entities, va( "print \"%s requires more arguments: %s\n\"", arg1, vote->shortHelp ) );
		return;
	}

	level.votingGametype = qfalse;

	level.voteExecuteDelay = vote->voteDelay ? g_voteDelay.integer : 0;

	// there is still a vote to be executed, execute it and store the new vote
	if ( level.voteExecuteTime ) {
		level.voteExecuteTime = 0;
		trap->SendConsoleCommand( EXEC_APPEND, va( "%s\n", level.voteString ) );
	}

	// pass the args onto vote-specific handlers for parsing/filtering
	if ( vote->func ) {
		if ( !vote->func( ent, numArgs, arg1, arg2 ) )
			return;
	}
	// otherwise assume it's a command
	else {
		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s \"%s\"", arg1, arg2 );
		Q_strncpyz( level.voteDisplayString, level.voteString, sizeof( level.voteDisplayString ) );
		Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	}
	Q_strstrip( level.voteStringClean, "\"\n\r", NULL );

	trap->SendServerCommand( -1, va( "print \"%s^7 %s (%s)\n\"", ent->client->pers.netname, G_GetStringEdString( "MP_SVGAME", "PLCALLEDVOTE" ), level.voteStringClean ) );

	// start the voting, the caller automatically votes yes
	level.voteTime = level.time;
	level.voteYes = 1;
	level.voteNo = 0;

	for ( i=0; i<level.maxclients; i++ ) {
		level.clients[i].mGameFlags &= ~PSG_VOTED;
		level.clients[i].pers.vote = 0;
	}

	ent->client->mGameFlags |= PSG_VOTED;
	ent->client->pers.vote = 1;

	trap->SetConfigstring( CS_VOTE_TIME,	va( "%i", level.voteTime ) );
	trap->SetConfigstring( CS_VOTE_STRING,	level.voteDisplayString );	
	trap->SetConfigstring( CS_VOTE_YES,		va( "%i", level.voteYes ) );
	trap->SetConfigstring( CS_VOTE_NO,		va( "%i", level.voteNo ) );	
}

/*
==================
Cmd_Vote_f
==================
*/
void Cmd_Vote_f( gentity_t *ent ) {
	char		msg[64] = {0};

	if ( !level.voteTime ) {
		trap->SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOTEINPROG")) );
		return;
	}
	if ( ent->client->mGameFlags & PSG_VOTED ) {
		trap->SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "VOTEALREADY")) );
		return;
	}
	if (level.gametype != GT_DUEL && level.gametype != GT_POWERDUEL)
	{
		if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
			trap->SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOTEASSPEC")) );
			return;
		}
	}

	trap->SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PLVOTECAST")) );

	ent->client->mGameFlags |= PSG_VOTED;

	trap->Argv( 1, msg, sizeof( msg ) );

	if ( tolower( msg[0] ) == 'y' || msg[0] == '1' ) {
		level.voteYes++;
		ent->client->pers.vote = 1;
		trap->SetConfigstring( CS_VOTE_YES, va("%i", level.voteYes ) );
	} else {
		level.voteNo++;
		ent->client->pers.vote = 2;
		trap->SetConfigstring( CS_VOTE_NO, va("%i", level.voteNo ) );	
	}

	// a majority will be determined in CheckVote, which will also account
	// for players entering or leaving
}

qboolean G_TeamVoteLeader( gentity_t *ent, int cs_offset, team_t team, int numArgs, const char *arg1, const char *arg2 ) {
	int clientid = numArgs == 2 ? ent->s.number : JP_ClientNumberFromString( ent, arg2 );
	gentity_t *target = NULL;

	if ( clientid == -1 || clientid == -2)
		return qfalse;

	target = &g_entities[clientid];
	if ( !target || !target->inuse || !target->client )
		return qfalse;

	if ( target->client->sess.sessionTeam != team )
	{
		trap->SendServerCommand( ent-g_entities, va( "print \"User %s is not on your team\n\"", arg2 ) );
		return qfalse;
	}

	Com_sprintf( level.teamVoteString[cs_offset], sizeof( level.teamVoteString[cs_offset] ), "leader %d", clientid );
	Q_strncpyz( level.teamVoteDisplayString[cs_offset], level.teamVoteString[cs_offset], sizeof( level.teamVoteDisplayString[cs_offset] ) );
	Q_strncpyz( level.teamVoteStringClean[cs_offset], level.teamVoteString[cs_offset], sizeof( level.teamVoteStringClean[cs_offset] ) );
	return qtrue;
}

/*
==================
Cmd_CallTeamVote_f
==================
*/
void Cmd_CallTeamVote_f( gentity_t *ent ) {
	team_t	team = ent->client->sess.sessionTeam;
	int		i=0, cs_offset=0, numArgs=0;
	char	arg1[MAX_CVAR_VALUE_STRING] = {0};
	char	arg2[MAX_CVAR_VALUE_STRING] = {0};

	if ( team == TEAM_RED )
		cs_offset = 0;
	else if ( team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	// not allowed to vote at all
	if ( !g_allowTeamVote.integer ) {
		trap->SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOTE")) );
		return;
	}

	// vote in progress
	else if ( level.teamVoteTime[cs_offset] ) {
		trap->SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TEAMVOTEALREADY")) );
		return;
	}

	// can't vote as a spectator
	else if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap->SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOSPECVOTE")) );
		return;
	}

	// make sure it is a valid command to vote on
	numArgs = trap->Argc();
	trap->Argv( 1, arg1, sizeof( arg1 ) );
	if ( numArgs > 1 )
		Q_strncpyz( arg2, ConcatArgs( 2 ), sizeof( arg2 ) );

	// filter ; \n \r
	if ( Q_strchrs( arg1, ";\r\n" ) || Q_strchrs( arg2, ";\r\n" ) ) {
		trap->SendServerCommand( ent-g_entities, "print \"Invalid team vote string.\n\"" );
		return;
	}

	// pass the args onto vote-specific handlers for parsing/filtering
	if ( !Q_stricmp( arg1, "leader" ) ) {
		if ( !G_TeamVoteLeader( ent, cs_offset, team, numArgs, arg1, arg2 ) )
			return;
	}
	else {
		trap->SendServerCommand( ent-g_entities, "print \"Invalid team vote string.\n\"" );
		trap->SendServerCommand( ent-g_entities, va("print \"Allowed team vote strings are: ^%c%s %s\n\"", COLOR_GREEN, "leader", "<optional client name or number>" ));
		return;
	}

	Q_strstrip( level.teamVoteStringClean[cs_offset], "\"\n\r", NULL );

	for ( i=0; i<level.maxclients; i++ ) {
		if ( level.clients[i].pers.connected == CON_DISCONNECTED )
			continue;
		if ( level.clients[i].sess.sessionTeam == team )
			trap->SendServerCommand( i, va("print \"%s^7 called a team vote (%s)\n\"", ent->client->pers.netname, level.teamVoteStringClean[cs_offset] ) );
	}

	// start the voting, the caller autoamtically votes yes
	level.teamVoteTime[cs_offset] = level.time;
	level.teamVoteYes[cs_offset] = 1;
	level.teamVoteNo[cs_offset] = 0;

	for ( i=0; i<level.maxclients; i++ ) {
		if ( level.clients[i].pers.connected == CON_DISCONNECTED )
			continue;
		if ( level.clients[i].sess.sessionTeam == team ) {
			level.clients[i].mGameFlags &= ~PSG_TEAMVOTED;
			level.clients[i].pers.teamvote = 0;
		}
	}
	ent->client->mGameFlags |= PSG_TEAMVOTED;
	ent->client->pers.teamvote = 1;

	trap->SetConfigstring( CS_TEAMVOTE_TIME + cs_offset, va("%i", level.teamVoteTime[cs_offset] ) );
	trap->SetConfigstring( CS_TEAMVOTE_STRING + cs_offset, level.teamVoteDisplayString[cs_offset] );
	trap->SetConfigstring( CS_TEAMVOTE_YES + cs_offset, va("%i", level.teamVoteYes[cs_offset] ) );
	trap->SetConfigstring( CS_TEAMVOTE_NO + cs_offset, va("%i", level.teamVoteNo[cs_offset] ) );
}

/*
==================
Cmd_TeamVote_f
==================
*/
void Cmd_TeamVote_f( gentity_t *ent ) {
	team_t		team = ent->client->sess.sessionTeam;
	int			cs_offset=0;
	char		msg[64] = {0};

	if ( team == TEAM_RED )
		cs_offset = 0;
	else if ( team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !level.teamVoteTime[cs_offset] ) {
		trap->SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOTEAMVOTEINPROG")) );
		return;
	}
	if ( ent->client->mGameFlags & PSG_TEAMVOTED ) {
		trap->SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TEAMVOTEALREADYCAST")) );
		return;
	}
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap->SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOTEASSPEC")) );
		return;
	}

	trap->SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PLTEAMVOTECAST")) );

	ent->client->mGameFlags |= PSG_TEAMVOTED;

	trap->Argv( 1, msg, sizeof( msg ) );

	if ( tolower( msg[0] ) == 'y' || msg[0] == '1' ) {
		level.teamVoteYes[cs_offset]++;
		ent->client->pers.teamvote = 1;
		trap->SetConfigstring( CS_TEAMVOTE_YES + cs_offset, va("%i", level.teamVoteYes[cs_offset] ) );
	} else {
		level.teamVoteNo[cs_offset]++;
		ent->client->pers.teamvote = 2;
		trap->SetConfigstring( CS_TEAMVOTE_NO + cs_offset, va("%i", level.teamVoteNo[cs_offset] ) );	
	}

	// a majority will be determined in TeamCheckVote, which will also account
	// for players entering or leaving
}


/*
=================
Cmd_SetViewpos_f
=================
*/
void Cmd_SetViewpos_f( gentity_t *ent ) {
	vec3_t		origin, angles;
	char		buffer[MAX_TOKEN_CHARS];
	int			i;

	if ( trap->Argc() != 5 ) {
		trap->SendServerCommand( ent-g_entities, va("print \"usage: setviewpos x y z yaw\n\""));
		return;
	}

	VectorClear( angles );
	for ( i = 0 ; i < 3 ; i++ ) {
		trap->Argv( i + 1, buffer, sizeof( buffer ) );
		origin[i] = atof( buffer );
	}

	trap->Argv( 4, buffer, sizeof( buffer ) );
	angles[YAW] = atof( buffer );

	TeleportPlayer( ent, origin, angles );
}

void G_LeaveVehicle( gentity_t* ent, qboolean ConCheck ) {

	if (ent->client->ps.m_iVehicleNum)
	{ //tell it I'm getting off
		gentity_t *veh = &g_entities[ent->client->ps.m_iVehicleNum];

		if (veh->inuse && veh->client && veh->m_pVehicle)
		{
			if ( ConCheck ) { // check connection
				clientConnected_t pCon = ent->client->pers.connected;
				ent->client->pers.connected = CON_DISCONNECTED;
				veh->m_pVehicle->m_pVehicleInfo->Eject(veh->m_pVehicle, (bgEntity_t *)ent, qtrue);
				ent->client->pers.connected = pCon;
			} else { // or not.
				veh->m_pVehicle->m_pVehicleInfo->Eject(veh->m_pVehicle, (bgEntity_t *)ent, qtrue);
			}
		}
	}

	ent->client->ps.m_iVehicleNum = 0;
}

int G_ItemUsable(playerState_t *ps, int forcedUse)
{
	vec3_t fwd, fwdorg, dest, pos;
	vec3_t yawonly;
	vec3_t mins, maxs;
	vec3_t trtest;
	trace_t tr;

	// fix: dead players shouldn't use items
	if (ps->stats[STAT_HEALTH] <= 0) {
		return 0;
	}

	if (ps->m_iVehicleNum)
	{
		return 0;
	}
	
	if (ps->pm_flags & PMF_USE_ITEM_HELD)
	{ //force to let go first
		return 0;
	}

	if (!forcedUse)
	{
		forcedUse = bg_itemlist[ps->stats[STAT_HOLDABLE_ITEM]].giTag;
	}

	if (!BG_IsItemSelectable(ps, forcedUse))
	{
		return 0;
	}

	switch (forcedUse)
	{
	case HI_MEDPAC:
	case HI_MEDPAC_BIG:
		if (ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH])
		{
			return 0;
		}

		if (ps->stats[STAT_HEALTH] <= 0)
		{
			return 0;
		}

		return 1;
	case HI_SEEKER:
		if (ps->eFlags & EF_SEEKERDRONE)
		{
			G_AddEvent(&g_entities[ps->clientNum], EV_ITEMUSEFAIL, SEEKER_ALREADYDEPLOYED);
			return 0;
		}

		return 1;
	case HI_SENTRY_GUN:
		if (ps->fd.sentryDeployed)
		{
			G_AddEvent(&g_entities[ps->clientNum], EV_ITEMUSEFAIL, SENTRY_ALREADYPLACED);
			return 0;
		}

		yawonly[ROLL] = 0;
		yawonly[PITCH] = 0;
		yawonly[YAW] = ps->viewangles[YAW];

		VectorSet( mins, -8, -8, 0 );
		VectorSet( maxs, 8, 8, 24 );

		AngleVectors(yawonly, fwd, NULL, NULL);

		fwdorg[0] = ps->origin[0] + fwd[0]*64;
		fwdorg[1] = ps->origin[1] + fwd[1]*64;
		fwdorg[2] = ps->origin[2] + fwd[2]*64;

		trtest[0] = fwdorg[0] + fwd[0]*16;
		trtest[1] = fwdorg[1] + fwd[1]*16;
		trtest[2] = fwdorg[2] + fwd[2]*16;

		trap->Trace(&tr, ps->origin, mins, maxs, trtest, ps->clientNum, MASK_PLAYERSOLID, qfalse, 0, 0);

		if ((tr.fraction != 1 && tr.entityNum != ps->clientNum) || tr.startsolid || tr.allsolid)
		{
			G_AddEvent(&g_entities[ps->clientNum], EV_ITEMUSEFAIL, SENTRY_NOROOM);
			return 0;
		}

		return 1;
	case HI_SHIELD:
		mins[0] = -8;
		mins[1] = -8;
		mins[2] = 0;

		maxs[0] = 8;
		maxs[1] = 8;
		maxs[2] = 8;

		AngleVectors (ps->viewangles, fwd, NULL, NULL);
		fwd[2] = 0;
		VectorMA(ps->origin, 64, fwd, dest);
		trap->Trace(&tr, ps->origin, mins, maxs, dest, ps->clientNum, MASK_SHOT, qfalse, 0, 0 );
		if (tr.fraction > 0.9 && !tr.startsolid && !tr.allsolid)
		{
			VectorCopy(tr.endpos, pos);
			VectorSet( dest, pos[0], pos[1], pos[2] - 4096 );
			trap->Trace( &tr, pos, mins, maxs, dest, ps->clientNum, MASK_SOLID, qfalse, 0, 0 );
			if ( !tr.startsolid && !tr.allsolid )
			{
				return 1;
			}
		}
		G_AddEvent(&g_entities[ps->clientNum], EV_ITEMUSEFAIL, SHIELD_NOROOM);
		return 0;
	case HI_JETPACK: //do something?
		return 1;
	case HI_HEALTHDISP:
		return 1;
	case HI_AMMODISP:
		return 1;
	case HI_EWEB:
		return 1;
	case HI_CLOAK:
		return 1;
	default:
		return 1;
	}
}

void saberKnockDown(gentity_t *saberent, gentity_t *saberOwner, gentity_t *other);

void Cmd_ToggleSaber_f(gentity_t *ent)
{
//[JAPRO - Serverside - Force - Fix Saber in grip - Start]
	if (!g_fixSaberInGrip.integer > 2 && ent->client->ps.fd.forceGripCripple)
	{ //if they are being gripped, don't let them unholster their saber
		if (ent->client->ps.saberHolstered)
		{
			return;
		}
	}
//[JAPRO - Serverside - Force - Fix Saber in grip - End]

	if (ent->client->ps.saberInFlight)
	{
		if (ent->client->ps.saberEntityNum)
		{ //turn it off in midair
			saberKnockDown(&g_entities[ent->client->ps.saberEntityNum], ent, ent);
		}
		return;
	}

	if (ent->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		return;
	}

	if (ent->client->ps.weapon != WP_SABER)
	{
		return;
	}

//	if (ent->client->ps.duelInProgress && !ent->client->ps.saberHolstered)
//	{
//		return;
//	}

	if (ent->client->ps.duelTime >= level.time)
	{
		return;
	}

	if (ent->client->ps.saberLockTime >= level.time)
	{
		return;
	}

	if (ent->client && ent->client->ps.weaponTime < 1)
	{
		if (ent->client->ps.saberHolstered == 2)
		{
			ent->client->ps.saberHolstered = 0;

			if (ent->client->saber[0].soundOn)
			{
				G_Sound(ent, CHAN_AUTO, ent->client->saber[0].soundOn);
			}
			if (ent->client->saber[1].soundOn)
			{
				G_Sound(ent, CHAN_AUTO, ent->client->saber[1].soundOn);
			}
		}
		else
		{
			ent->client->ps.saberHolstered = 2;
			if (ent->client->saber[0].soundOff)
			{
				G_Sound(ent, CHAN_AUTO, ent->client->saber[0].soundOff);
			}
			if (ent->client->saber[1].soundOff &&
				ent->client->saber[1].model[0])
			{
				G_Sound(ent, CHAN_AUTO, ent->client->saber[1].soundOff);
			}
			//prevent anything from being done for 400ms after holster
			ent->client->ps.weaponTime = 400;
		}
	}
}

extern vmCvar_t		d_saberStanceDebug;

extern qboolean WP_SaberCanTurnOffSomeBlades( saberInfo_t *saber );
void Cmd_SaberAttackCycle_f(gentity_t *ent)
{
	int selectLevel = 0;
	qboolean usingSiegeStyle = qfalse;
	
	if ( !ent || !ent->client )
	{
		return;
	}
	if ( ent->client->ps.weapon != WP_SABER )
	{
        return;
	}
	/*
	if (ent->client->ps.weaponTime > 0)
	{ //no switching attack level when busy
		return;
	}
	*/

	if (ent->client->saber[0].model[0] && ent->client->saber[1].model[0])
	{ //no cycling for akimbo
		if ( WP_SaberCanTurnOffSomeBlades( &ent->client->saber[1] ) )
		{//can turn second saber off 
			if ( ent->client->ps.saberHolstered == 1 )
			{//have one holstered
				//unholster it
				G_Sound(ent, CHAN_AUTO, ent->client->saber[1].soundOn);
				ent->client->ps.saberHolstered = 0;
				//g_active should take care of this, but...
				ent->client->ps.fd.saberAnimLevel = SS_DUAL;
			}
			else if ( ent->client->ps.saberHolstered == 0 )
			{//have none holstered
				if ( (ent->client->saber[1].saberFlags2&SFL2_NO_MANUAL_DEACTIVATE) )
				{//can't turn it off manually
				}
				else if ( ent->client->saber[1].bladeStyle2Start > 0
					&& (ent->client->saber[1].saberFlags2&SFL2_NO_MANUAL_DEACTIVATE2) )
				{//can't turn it off manually
				}
				else
				{
					//turn it off
					G_Sound(ent, CHAN_AUTO, ent->client->saber[1].soundOff);
					ent->client->ps.saberHolstered = 1;
					//g_active should take care of this, but...
					ent->client->ps.fd.saberAnimLevel = SS_FAST;
				}
			}

			if (d_saberStanceDebug.integer)
			{
				trap->SendServerCommand( ent-g_entities, va("print \"SABERSTANCEDEBUG: Attempted to toggle dual saber blade.\n\"") );
			}
			return;
		}
	}
	else if (ent->client->saber[0].numBlades > 1
		&& WP_SaberCanTurnOffSomeBlades( &ent->client->saber[0] ) )
	{ //use staff stance then.
		if ( ent->client->ps.saberHolstered == 1 )
		{//second blade off
			if ( ent->client->ps.saberInFlight )
			{//can't turn second blade back on if it's in the air, you naughty boy!
				if (d_saberStanceDebug.integer)
				{
					trap->SendServerCommand( ent-g_entities, va("print \"SABERSTANCEDEBUG: Attempted to toggle staff blade in air.\n\"") );
				}
				return;
			}
			//turn it on
			G_Sound(ent, CHAN_AUTO, ent->client->saber[0].soundOn);
			ent->client->ps.saberHolstered = 0;
			//g_active should take care of this, but...
			if ( ent->client->saber[0].stylesForbidden )
			{//have a style we have to use
				WP_UseFirstValidSaberStyle( &ent->client->saber[0], &ent->client->saber[1], ent->client->ps.saberHolstered, &selectLevel );
				if ( ent->client->ps.weaponTime <= 0 )
				{ //not busy, set it now
					ent->client->ps.fd.saberAnimLevel = selectLevel;
				}
				else
				{ //can't set it now or we might cause unexpected chaining, so queue it
					ent->client->saberCycleQueue = selectLevel;
				}
			}
		}
		else if ( ent->client->ps.saberHolstered == 0 )
		{//both blades on
			if ( (ent->client->saber[0].saberFlags2&SFL2_NO_MANUAL_DEACTIVATE) )
			{//can't turn it off manually
			}
			else if ( ent->client->saber[0].bladeStyle2Start > 0
				&& (ent->client->saber[0].saberFlags2&SFL2_NO_MANUAL_DEACTIVATE2) )
			{//can't turn it off manually
			}
			else
			{
				//turn second one off
				G_Sound(ent, CHAN_AUTO, ent->client->saber[0].soundOff);
				ent->client->ps.saberHolstered = 1;
				//g_active should take care of this, but...
				if ( ent->client->saber[0].singleBladeStyle != SS_NONE )
				{
					if ( ent->client->ps.weaponTime <= 0 )
					{ //not busy, set it now
						ent->client->ps.fd.saberAnimLevel = ent->client->saber[0].singleBladeStyle;
					}
					else
					{ //can't set it now or we might cause unexpected chaining, so queue it
						ent->client->saberCycleQueue = ent->client->saber[0].singleBladeStyle;
					}
				}
			}
		}
		if (d_saberStanceDebug.integer)
		{
			trap->SendServerCommand( ent-g_entities, va("print \"SABERSTANCEDEBUG: Attempted to toggle staff blade.\n\"") );
		}
		return;
	}

	if (ent->client->saberCycleQueue)
	{ //resume off of the queue if we haven't gotten a chance to update it yet
		selectLevel = ent->client->saberCycleQueue;
	}
	else
	{
		selectLevel = ent->client->ps.fd.saberAnimLevel;
	}

	if (level.gametype == GT_SIEGE &&
		ent->client->siegeClass != -1 &&
		bgSiegeClasses[ent->client->siegeClass].saberStance)
	{ //we have a flag of useable stances so cycle through it instead
		int i = selectLevel+1;

		usingSiegeStyle = qtrue;

		while (i != selectLevel)
		{ //cycle around upward til we hit the next style or end up back on this one
			if (i >= SS_NUM_SABER_STYLES)
			{ //loop back around to the first valid
				i = SS_FAST;
			}

			if (bgSiegeClasses[ent->client->siegeClass].saberStance & (1 << i))
			{ //we can use this one, select it and break out.
				selectLevel = i;
				break;
			}
			i++;
		}

		if (d_saberStanceDebug.integer)
		{
			trap->SendServerCommand( ent-g_entities, va("print \"SABERSTANCEDEBUG: Attempted to cycle given class stance.\n\"") );
		}
	}
	else
	{
		selectLevel++;
		if ( selectLevel > ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] )
		{
			selectLevel = FORCE_LEVEL_1;
		}
		if (d_saberStanceDebug.integer)
		{
			trap->SendServerCommand( ent-g_entities, va("print \"SABERSTANCEDEBUG: Attempted to cycle stance normally.\n\"") );
		}
	}
/*
#ifndef FINAL_BUILD
	switch ( selectLevel )
	{
	case FORCE_LEVEL_1:
		trap->SendServerCommand( ent-g_entities, va("print \"Lightsaber Combat Style: %sfast\n\"", S_COLOR_BLUE) );
		break;
	case FORCE_LEVEL_2:
		trap->SendServerCommand( ent-g_entities, va("print \"Lightsaber Combat Style: %smedium\n\"", S_COLOR_YELLOW) );
		break;
	case FORCE_LEVEL_3:
		trap->SendServerCommand( ent-g_entities, va("print \"Lightsaber Combat Style: %sstrong\n\"", S_COLOR_RED) );
		break;
	}
#endif
*/
	if ( !usingSiegeStyle )
	{
		//make sure it's valid, change it if not
		WP_UseFirstValidSaberStyle( &ent->client->saber[0], &ent->client->saber[1], ent->client->ps.saberHolstered, &selectLevel );
	}

	if (ent->client->ps.weaponTime <= 0)
	{ //not busy, set it now
		ent->client->ps.fd.saberAnimLevelBase = ent->client->ps.fd.saberAnimLevel = selectLevel;
	}
	else
	{ //can't set it now or we might cause unexpected chaining, so queue it
		ent->client->ps.fd.saberAnimLevelBase = ent->client->saberCycleQueue = selectLevel;
	}
}

qboolean G_OtherPlayersDueling(void)
{
	int i = 0;
	gentity_t *ent;

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent && ent->inuse && ent->client && ent->client->ps.duelInProgress)
		{
			return qtrue;
		}
		i++;
	}

	return qfalse;
}

//JAPRO - Serverside - Fullforce Duels - Start
void Cmd_ForceDuel_f(gentity_t *ent)
{
	Cmd_EngageDuel_f(ent, 1);
}

void Cmd_GunDuel_f(gentity_t *ent)
{
	char weapStr[64];//Idk, could be less
	int weap = WP_NONE;

	if (trap->Argc() > 2)
	{
		trap->SendServerCommand( ent-g_entities, "print \"Usage: /engage_gunduel or engage_gunduel <weapon name>.\n\"" );
		return;
	}

	if (trap->Argc() == 1)//Use current weapon
	{
		weap = ent->s.weapon;
		if (weap == 17)//fuck off
			weap = WP_NONE;
	}
	else if (trap->Argc() == 2)//Use specified weapon
	{
		int i = 0;
		trap->Argv(1, weapStr, sizeof(weapStr));

		while (weapStr[i]) {
			weapStr[i] = tolower(weapStr[i]);
			i++;
		}

		if (!Q_stricmp( "melee", weapStr) || !Q_stricmp( "fists", weapStr) || !Q_stricmp( "fisticuffs", weapStr))
			weap = WP_MELEE;
		else if (!Q_stricmp( "stun", weapStr) || !Q_stricmp( "stunbaton", weapStr))
			weap = WP_STUN_BATON + 16;
		else if (!Q_stricmp( "pistol", weapStr) || !Q_stricmp( "dl44", weapStr))
			weap = WP_BRYAR_PISTOL;
		else if (!Q_stricmp( "bryar", weapStr) || !Q_stricmp( "bryarpistol", weapStr) )
			weap = WP_BRYAR_OLD;
		else if (!Q_stricmp( "blaster", weapStr) || !Q_stricmp( "e11", weapStr))
			weap = WP_BLASTER;
		else if (!Q_stricmp( "sniper", weapStr) || !Q_stricmp( "disruptor", weapStr))
			weap = WP_DISRUPTOR;
		else if (!Q_stricmp( "bowcaster", weapStr))
			weap = WP_BOWCASTER;
		else if (!Q_stricmp( "repeater", weapStr))
			weap = WP_REPEATER;
		else if (!Q_stricmp( "demp2", weapStr) || !Q_stricmp( "demp", weapStr))
			weap = WP_DEMP2;
		else if (!Q_stricmp( "flechette", weapStr) || !Q_stricmp( "shotgun", weapStr))
			weap = WP_FLECHETTE;
		else if (!Q_stricmp( "concussion", weapStr) || !Q_stricmp( "conc", weapStr))
			weap = WP_CONCUSSION;
		else if (!Q_stricmp( "rocket", weapStr) || !Q_stricmp( "rockets", weapStr) || !Q_stricmp( "rocketlauncher", weapStr))
			weap = WP_ROCKET_LAUNCHER;
		else if (!Q_stricmp( "detpack", weapStr) || !Q_stricmp( "detpacks", weapStr))
			weap = WP_DET_PACK;
		else if (!Q_stricmp( "thermal", weapStr) || !Q_stricmp( "thermals", weapStr) || !Q_stricmp( "grenade", weapStr) || !Q_stricmp( "grenades", weapStr) || !Q_stricmp( "dets", weapStr))
			weap = WP_THERMAL;
		else if (!Q_stricmp( "tripmine", weapStr) || !Q_stricmp( "trip", weapStr) || !Q_stricmp( "trips", weapStr))
			weap = WP_TRIP_MINE;

	}

	if (weap == WP_NONE || weap == WP_SABER || (weap > WP_BRYAR_OLD && weap != 17)) {
		//trap_SendServerCommand( ent-g_entities, "print \"Invalid weapon specified, using default case: Bryar\n\"" );
		weap = WP_BRYAR_PISTOL;//pff
	}

	Cmd_EngageDuel_f(ent, weap + 2);//Fuck it right here
}

void Cmd_EngageDuel_f(gentity_t *ent, int dueltype)//JAPRO - Serverside - Fullforce Duels
{
	trace_t tr;
	vec3_t forward, fwdOrg;

	if (!g_privateDuel.integer)
		return;

	if (g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL || g_gametype.integer >= GT_TEAM)
	{ //rather pointless in this mode..
		if (dueltype == 0 || dueltype == 1) 
			trap->SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NODUEL_GAMETYPE")) );
		else
			trap->SendServerCommand(ent-g_entities, "print \"This gametype does not support gun dueling.\n\"");
		return;
	}

	if (ent->client->ps.duelTime >= level.time)
		return;

	if ((dueltype == 0 || dueltype == 1) && ent->client->ps.weapon != WP_SABER)
		return;

	if ((dueltype == 0 || dueltype == 1) && (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK)))//JAPRO - Disallow jetpack in NF,FF duels?
		return;

	if (ent->client->ps.saberInFlight)
		return;

	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) // Does this stop spectating someone and challenging them to a duel?
		return;

	if (ent->client->ps.forceHandExtend == HANDEXTEND_KNOCKDOWN)
		return;

	if (ent->client->ps.powerups[PW_NEUTRALFLAG])
		return;

	if (ent->client->ps.duelInProgress)
		return;

	if (ent->client->pers.raceMode)
		return;

	AngleVectors( ent->client->ps.viewangles, forward, NULL, NULL );

	fwdOrg[0] = ent->client->ps.origin[0] + forward[0]*256;
	fwdOrg[1] = ent->client->ps.origin[1] + forward[1]*256;
	fwdOrg[2] = (ent->client->ps.origin[2]+ent->client->ps.viewheight) + forward[2]*256;

	JP_Trace(&tr, ent->client->ps.origin, NULL, NULL, fwdOrg, ent->s.number, MASK_PLAYERSOLID, qfalse, 0, 0);

	if (tr.fraction != 1 && tr.entityNum < MAX_CLIENTS)
	{
		gentity_t *challenged = &g_entities[tr.entityNum];

		if (!challenged || !challenged->client || !challenged->inuse ||
			challenged->health < 1 || challenged->client->ps.stats[STAT_HEALTH] < 1 ||
			(challenged->client->ps.weapon != WP_SABER && (dueltype == 0 || dueltype == 1)) || challenged->client->ps.duelInProgress ||
			challenged->client->ps.saberInFlight)
		{
			return;
		}

		if (g_gametype.integer >= GT_TEAM && OnSameTeam(ent, challenged))
		{
			return;
		}

//JAPRO - Serverside - Fullforce Duels + Duel Messages - Start
		if (challenged->client->ps.duelIndex == ent->s.number && challenged->client->ps.duelTime >= level.time && 
			(
			((dueltypes[challenged->client->ps.clientNum] == dueltype) && (dueltype == 0 || dueltype == 1)) 
			|| 
			(dueltypes[challenged->client->ps.clientNum] != 0 && (dueltypes[challenged->client->ps.clientNum] != 1) && dueltype > 1)
			))
		{
			ent->client->ps.duelInProgress = qtrue;
			challenged->client->ps.duelInProgress = qtrue;

			if (dueltype == 0 || dueltype == 1)
				dueltypes[ent->client->ps.clientNum] = dueltype;//y isnt this syncing the weapons they use? gun duels
			else {
				dueltypes[ent->client->ps.clientNum] = dueltypes[challenged->client->ps.clientNum];//dueltype;//k this is y
			}

			ent->client->ps.duelTime = level.time + 2000;
			challenged->client->ps.duelTime = level.time + 2000;

			switch (dueltype)
			{
				case	0:
					//Saber Duel
					G_AddEvent(ent, EV_PRIVATE_DUEL, 1);
					G_AddEvent(challenged, EV_PRIVATE_DUEL, 1);
					trap->SendServerCommand(-1, va("print \"%s^7 %s %s^7! (Saber)\n\"", challenged->client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLDUELACCEPT"), ent->client->pers.netname) );
					break;
				case	1://FF Duel
					G_AddEvent(ent, EV_PRIVATE_DUEL, 2);
					G_AddEvent(challenged, EV_PRIVATE_DUEL, 2);
					trap->SendServerCommand(-1, va("print \"%s^7 %s %s^7! (Force)\n\"", challenged->client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLDUELACCEPT"), ent->client->pers.netname) );
					break;
				default://Gun duel
					G_AddEvent(ent, EV_PRIVATE_DUEL, dueltypes[ent->client->ps.clientNum]);
					G_AddEvent(challenged, EV_PRIVATE_DUEL, dueltypes[challenged->client->ps.clientNum]);
					trap->SendServerCommand(-1, va("print \"%s^7 %s %s^7! (Gun)\n\"", challenged->client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLDUELACCEPT"), ent->client->pers.netname) );
					break;
			}
						
			G_SetAnim(ent, &ent->client->pers.cmd, SETANIM_BOTH, BOTH_STAND1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);
			G_SetAnim(challenged, &ent->client->pers.cmd, BOTH_STAND1, BOTH_STAND1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);
					
			if ( ent->client->ps.saberHolstered < 2 )
			{
				if (ent->client->saber[0].soundOff){
					G_Sound(ent, CHAN_AUTO, ent->client->saber[0].soundOff);
				}
				if (ent->client->saber[1].soundOff && ent->client->saber[1].model[0]){
					G_Sound(ent, CHAN_AUTO, ent->client->saber[1].soundOff);
				}
				ent->client->ps.saberHolstered = 2;
				ent->client->ps.weaponTime = 400;
			}
			if ( challenged->client->ps.saberHolstered < 2 )
			{
				if (challenged->client->saber[0].soundOff)
				{
					G_Sound(challenged, CHAN_AUTO, challenged->client->saber[0].soundOff);
				}
				if (challenged->client->saber[1].soundOff && challenged->client->saber[1].model[0])
				{
					G_Sound(challenged, CHAN_AUTO, challenged->client->saber[1].soundOff);
				}
				challenged->client->ps.saberHolstered = 2;
				challenged->client->ps.weaponTime = 400;
			}
			if (g_duelStartHealth.integer)
			{
				ent->health = ent->client->ps.stats[STAT_HEALTH] = g_duelStartHealth.integer;
				ent->client->ps.stats[STAT_ARMOR] = g_duelStartArmor.integer;
				challenged->health = challenged->client->ps.stats[STAT_HEALTH] = g_duelStartHealth.integer;
				challenged->client->ps.stats[STAT_ARMOR] = g_duelStartArmor.integer;
			}
			ent->client->ps.fd.forcePower = ent->client->ps.fd.forcePowerMax; //max force power too!
			challenged->client->ps.fd.forcePower = challenged->client->ps.fd.forcePowerMax; //max force power too!
			
			if (dueltypes[challenged->client->ps.clientNum] > 2) {
				int weapon = dueltypes[challenged->client->ps.clientNum] - 2;
				if (weapon != WP_STUN_BATON && weapon != WP_MELEE && weapon != WP_BRYAR_PISTOL) {
					ent->client->ps.ammo[weaponData[weapon].ammoIndex] = 999; //gun duel ammo
					challenged->client->ps.ammo[weaponData[weapon].ammoIndex] = 999; //gun duel ammo
				}
			}
		}
		else
		{
			char weapStr[64];
			//Print the message that a player has been challenged in private, only announce the actual duel initiation in private
			switch (dueltype) {
			case	0:
				trap->SendServerCommand( challenged-g_entities, va("cp \"%s ^7%s\n^2(Saber Duel)\n\"", ent->client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLDUELCHALLENGE")) );
				trap->SendServerCommand( ent-g_entities, va("cp \"%s %s\n^2(Saber Duel)\n\"", G_GetStringEdString("MP_SVGAME", "PLDUELCHALLENGED"), challenged->client->pers.netname) );
				break;
			case	1:
				trap->SendServerCommand( challenged-g_entities, va("cp \"%s ^7%s\n^4(Force Duel)\n\"", ent->client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLDUELCHALLENGE")) );
				trap->SendServerCommand( ent-g_entities, va("cp \"%s %s\n^4(Force Duel)\n\"", G_GetStringEdString("MP_SVGAME", "PLDUELCHALLENGED"), challenged->client->pers.netname) );
				break;
			default:
				switch (dueltype - 2) {
					case 1:	Com_sprintf(weapStr, sizeof(weapStr), "Stun baton"); break;
					case 2: Com_sprintf(weapStr, sizeof(weapStr), "Melee"); break;
					case 4:	Com_sprintf(weapStr, sizeof(weapStr), "Pistol"); break;
					case 5:	Com_sprintf(weapStr, sizeof(weapStr), "E11"); break;
					case 6:	Com_sprintf(weapStr, sizeof(weapStr), "Sniper"); break;
					case 7:	Com_sprintf(weapStr, sizeof(weapStr), "Bowcaster");	break;
					case 8:	Com_sprintf(weapStr, sizeof(weapStr), "Repeater"); break;
					case 9:	Com_sprintf(weapStr, sizeof(weapStr), "Demp2");	break;
					case 10: Com_sprintf(weapStr, sizeof(weapStr), "Flechette"); break;
					case 11: Com_sprintf(weapStr, sizeof(weapStr), "Rocket"); break;
					case 12: Com_sprintf(weapStr, sizeof(weapStr), "Thermal"); break;
					case 13: Com_sprintf(weapStr, sizeof(weapStr), "Tripmine"); break;
					case 14: Com_sprintf(weapStr, sizeof(weapStr), "Detpack"); break;
					case 15: Com_sprintf(weapStr, sizeof(weapStr), "Concussion rifle"); break;
					case 16: Com_sprintf(weapStr, sizeof(weapStr), "Bryar"); break;
					default: Com_sprintf(weapStr, sizeof(weapStr), "Gun"); break;
				}
				trap->SendServerCommand( challenged-g_entities, va("cp \"%s ^7%s\n^4(%s Duel)\n\"", ent->client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLDUELCHALLENGE"), weapStr) );
				trap->SendServerCommand( ent-g_entities, va("cp \"%s %s\n^4(%s Duel)\n\"", G_GetStringEdString("MP_SVGAME", "PLDUELCHALLENGED"), challenged->client->pers.netname, weapStr) );
				break;
			}
		}
//JAPRO - Serverside - Fullforce Duels + Duel Messages - End

		challenged->client->ps.fd.privateDuelTime = 0; //reset the timer in case this player just got out of a duel. He should still be able to accept the challenge.

		ent->client->ps.forceHandExtend = HANDEXTEND_DUELCHALLENGE;
		ent->client->ps.forceHandExtendTime = level.time + 1000;

		ent->client->ps.duelIndex = challenged->s.number;
		ent->client->ps.duelTime = level.time + 2000;//Is this where the freeze comes from?
		if (dueltype == 0 || dueltype == 1) 
			dueltypes[ent->client->ps.clientNum] = dueltype;//JAPRO - Serverside - Fullforce Duels
		else if (!ent->client->ps.duelInProgress)
			dueltypes[ent->client->ps.clientNum] = dueltype;
	}
}

#ifndef FINAL_BUILD
extern stringID_table_t animTable[MAX_ANIMATIONS+1];

void Cmd_DebugSetSaberMove_f(gentity_t *self)
{
	int argNum = trap->Argc();
	char arg[MAX_STRING_CHARS];

	if (argNum < 2)
	{
		return;
	}

	trap->Argv( 1, arg, sizeof( arg ) );

	if (!arg[0])
	{
		return;
	}

	self->client->ps.saberMove = atoi(arg);
	self->client->ps.saberBlocked = BLOCKED_BOUNCE_MOVE;

	if (self->client->ps.saberMove >= LS_MOVE_MAX)
	{
		self->client->ps.saberMove = LS_MOVE_MAX-1;
	}

	Com_Printf("Anim for move: %s\n", animTable[saberMoveData[self->client->ps.saberMove].animToUse].name);
}

void Cmd_DebugSetBodyAnim_f(gentity_t *self)
{
	int argNum = trap->Argc();
	char arg[MAX_STRING_CHARS];
	int i = 0;

	if (argNum < 2)
	{
		return;
	}

	trap->Argv( 1, arg, sizeof( arg ) );

	if (!arg[0])
	{
		return;
	}

	while (i < MAX_ANIMATIONS)
	{
		if (!Q_stricmp(arg, animTable[i].name))
		{
			break;
		}
		i++;
	}

	if (i == MAX_ANIMATIONS)
	{
		Com_Printf("Animation '%s' does not exist\n", arg);
		return;
	}

	G_SetAnim(self, NULL, SETANIM_BOTH, i, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);

	Com_Printf("Set body anim to %s\n", arg);
}
#endif

void StandardSetBodyAnim(gentity_t *self, int anim, int flags)
{
	G_SetAnim(self, NULL, SETANIM_BOTH, anim, flags, 0);
}

void DismembermentTest(gentity_t *self);

void Bot_SetForcedMovement(int bot, int forward, int right, int up);

#ifndef FINAL_BUILD
extern void DismembermentByNum(gentity_t *self, int num);
extern void G_SetVehDamageFlags( gentity_t *veh, int shipSurf, int damageLevel );
#endif

qboolean TryGrapple(gentity_t *ent)
{
	if (ent->client->ps.weaponTime > 0)
	{ //weapon busy
		return qfalse;
	}
	if (ent->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{ //force power or knockdown or something
		return qfalse;
	}
	if (ent->client->grappleState)
	{ //already grappling? but weapontime should be > 0 then..
		return qfalse;
	}

	if (ent->client->ps.weapon != WP_SABER && ent->client->ps.weapon != WP_MELEE)
	{
		return qfalse;
	}

	if (ent->client->ps.weapon == WP_SABER && !ent->client->ps.saberHolstered)
	{
		Cmd_ToggleSaber_f(ent);
		if (!ent->client->ps.saberHolstered)
		{ //must have saber holstered
			return qfalse;
		}
	}

	//G_SetAnim(ent, &ent->client->pers.cmd, SETANIM_BOTH, BOTH_KYLE_PA_1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);
	G_SetAnim(ent, &ent->client->pers.cmd, SETANIM_BOTH, BOTH_KYLE_GRAB, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);
	if (ent->client->ps.torsoAnim == BOTH_KYLE_GRAB)
	{ //providing the anim set succeeded..
		ent->client->ps.torsoTimer += 500; //make the hand stick out a little longer than it normally would
		if (ent->client->ps.legsAnim == ent->client->ps.torsoAnim)
		{
			ent->client->ps.legsTimer = ent->client->ps.torsoTimer;
		}
		ent->client->ps.weaponTime = ent->client->ps.torsoTimer;
		return qtrue;
	}

	return qfalse;
}

void Cmd_TargetUse_f( gentity_t *ent )
{
	if ( trap->Argc() > 1 )
	{
		char sArg[MAX_STRING_CHARS] = {0};
		gentity_t *targ;

		trap->Argv( 1, sArg, sizeof( sArg ) );
		targ = G_Find( NULL, FOFS( targetname ), sArg );

		while ( targ )
		{
			if ( targ->use )
				targ->use( targ, ent, ent );
			targ = G_Find( targ, FOFS( targetname ), sArg );
		}
	}
}

void Cmd_TheDestroyer_f( gentity_t *ent ) {
	if ( !ent->client->ps.saberHolstered || ent->client->ps.weapon != WP_SABER )
		return;

	Cmd_ToggleSaber_f( ent );
}

void Cmd_BotMoveForward_f( gentity_t *ent ) {
	int arg = 4000;
	int bCl = 0;
	char sarg[MAX_STRING_CHARS];

	assert( trap->Argc() > 1 );
	trap->Argv( 1, sarg, sizeof( sarg ) );

	assert( sarg[0] );
	bCl = atoi( sarg );
	Bot_SetForcedMovement( bCl, arg, -1, -1 );
}

void Cmd_BotMoveBack_f( gentity_t *ent ) {
	int arg = -4000;
	int bCl = 0;
	char sarg[MAX_STRING_CHARS];

	assert( trap->Argc() > 1 );
	trap->Argv( 1, sarg, sizeof( sarg ) );

	assert( sarg[0] );
	bCl = atoi( sarg );
	Bot_SetForcedMovement( bCl, arg, -1, -1 );
}

void Cmd_BotMoveRight_f( gentity_t *ent ) {
	int arg = 4000;
	int bCl = 0;
	char sarg[MAX_STRING_CHARS];

	assert( trap->Argc() > 1 );
	trap->Argv( 1, sarg, sizeof( sarg ) );

	assert( sarg[0] );
	bCl = atoi( sarg );
	Bot_SetForcedMovement( bCl, -1, arg, -1 );
}

void Cmd_BotMoveLeft_f( gentity_t *ent ) {
	int arg = -4000;
	int bCl = 0;
	char sarg[MAX_STRING_CHARS];

	assert( trap->Argc() > 1 );
	trap->Argv( 1, sarg, sizeof( sarg ) );

	assert( sarg[0] );
	bCl = atoi( sarg );
	Bot_SetForcedMovement( bCl, -1, arg, -1 );
}

void Cmd_BotMoveUp_f( gentity_t *ent ) {
	int arg = 4000;
	int bCl = 0;
	char sarg[MAX_STRING_CHARS];

	assert( trap->Argc() > 1 );
	trap->Argv( 1, sarg, sizeof( sarg ) );

	assert( sarg[0] );
	bCl = atoi( sarg );
	Bot_SetForcedMovement( bCl, -1, -1, arg );
}

void Cmd_AddBot_f( gentity_t *ent ) {
	//because addbot isn't a recognized command unless you're the server, but it is in the menus regardless
	trap->SendServerCommand( ent-g_entities, va( "print \"%s.\n\"", G_GetStringEdString( "MP_SVGAME", "ONLY_ADD_BOTS_AS_SERVER" ) ) );
}

//[JAPRO - Serverside - All - Saber change Function - Start]
void Cmd_Saber_f(gentity_t *ent)
{
	int numSabers;
	int i;
	char saberNames[MAX_SABERS][64];
	char userinfo[MAX_INFO_STRING];

	numSabers = trap->Argc() - 1;

	if (!g_allowSaberSwitch.integer) {
		trap->SendServerCommand( ent-g_entities, "print \"Command not allowed. (saber).\n\"" );
		return;
	}

	if (ent->client->ps.duelInProgress) {
		trap->SendServerCommand( ent-g_entities, "print \"You are not allowed to use this command during a duel (saber).\n\"" );
		return;
	}

	if (g_gametype.integer != GT_FFA) {
		trap->SendServerCommand( ent-g_entities, "print \"You are only allowed to use this command during the FFA gametype (saber).\n\"" );
		return;
	}

	if (level.time - ent->client->ps.footstepTime < 1000 
		|| level.time - ent->client->ps.forceHandExtendTime < 1000 
		|| ent->client->ps.saberMove != LS_READY 
		|| ent->client->ps.saberInFlight) {
		trap->SendServerCommand( ent-g_entities, "print \"You must be idle to use this command (saber).\n\"" );
		return;
	}

	if (numSabers > MAX_SABERS || numSabers < 1) {
		trap->SendServerCommand(ent-g_entities, "print \"Usage: /saber <saber1> <saber2 (optional)>.\n\"");
		return;
	}

	if (numSabers == 1) {
		trap->Argv(1, saberNames[0], sizeof(saberNames[0]));
		strcpy(saberNames[1], "none");
	} else if (numSabers == 2) {
		trap->Argv(1, saberNames[0], sizeof(saberNames[0]));
		trap->Argv(2, saberNames[1], sizeof(saberNames[1]));
	}

	// update userinfo
	trap->GetUserinfo(ent-g_entities, userinfo, sizeof(userinfo));
	for (i = 0; i < MAX_SABERS; i++) {
		Info_SetValueForKey(userinfo, va("saber%i", i+1), saberNames[i]);
		G_SetSaber(ent, i, saberNames[i], qfalse);
	}
	trap->SetUserinfo(ent-g_entities, userinfo);
	ClientUserinfoChanged(ent-g_entities);

	G_SaberModelSetup(ent);

	// update saber
	if (ent->client->saber[0].model[0] && ent->client->saber[1].model[0]) {// dual
		ent->client->ps.fd.saberAnimLevelBase = ent->client->ps.fd.saberAnimLevel = ent->client->ps.fd.saberDrawAnimLevel = SS_DUAL;
	} else if (ent->client->saber[0].saberFlags & SFL_TWO_HANDED) {// staff
		ent->client->ps.fd.saberAnimLevel = ent->client->ps.fd.saberDrawAnimLevel = SS_STAFF;
	} else {// single
		if (ent->client->sess.saberLevel < SS_FAST) {
			ent->client->sess.saberLevel = SS_FAST;
		} else if (ent->client->sess.saberLevel > SS_STRONG) {
			ent->client->sess.saberLevel = SS_STRONG;
		}
		ent->client->ps.fd.saberAnimLevelBase = ent->client->ps.fd.saberAnimLevel = ent->client->ps.fd.saberDrawAnimLevel = ent->client->sess.saberLevel;
		if (ent->client->ps.fd.saberAnimLevel > ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE]) {
			ent->client->ps.fd.saberAnimLevelBase = ent->client->ps.fd.saberAnimLevel = ent->client->ps.fd.saberDrawAnimLevel = ent->client->sess.saberLevel = ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE];
		}
	}

	// let's just make sure the styles we chose are cool
	if (!WP_SaberStyleValidForSaber(&ent->client->saber[0], &ent->client->saber[1], ent->client->ps.saberHolstered, ent->client->ps.fd.saberAnimLevel)) {
		WP_UseFirstValidSaberStyle( &ent->client->saber[0], &ent->client->saber[1], ent->client->ps.saberHolstered, &ent->client->ps.fd.saberAnimLevel );
		ent->client->ps.fd.saberAnimLevelBase = ent->client->saberCycleQueue = ent->client->ps.fd.saberAnimLevel;
	}
}
//[JAPRO - Serverside - All - Saber change Function - End]

void Cmd_Ignore_f(gentity_t *ent)
{
	const int clientNum = ent - g_entities;
	if (trap->Argc() == 1) {
		int i;
		trap->SendServerCommand(clientNum, "print \"Ignored client(s):\n\"");
		for (i = 0; i < level.maxclients; ++i) {
			if (level.clients[i].pers.connected == CON_CONNECTED
				&& ClientIsIgnored(clientNum, i)) {
				trap->SendServerCommand(clientNum, va("print \" %s\n\"", level.clients[i].pers.netname));
			}
		}
	} else if (trap->Argc() == 2) {
		char netname[MAX_NETNAME];
		trap->Argv(1, netname, sizeof(netname));
		if (Q_stricmp(netname, "-1") == 0) {
			trap->SendServerCommand(clientNum, ClientIgnoreAll(clientNum) ? "print \"Ignored all\n\"" : "print \"Unignored all\n\"");
		} else {
			const int targetID = JP_ClientNumberFromString(ent, netname);
			if (targetID == clientNum) {
				trap->SendServerCommand(clientNum, "print \"Cannot ignore self\n\"");
			} else if (targetID != -1) {
				trap->SendServerCommand(clientNum, va(ClientIgnore(clientNum, targetID) ?  "print \"Ignored %s\n\"" : "print \"Unignored %s\n\"", g_entities[targetID].client->pers.netname));
			}
		}
	} else {
		trap->SendServerCommand(clientNum, "print \"Invalid usage\n\"");
	}
}
//[JAPRO - Serverside - All - Main Ignore Function - End]

void Cmd_Clanpass_f(gentity_t *ent)
{
	char   pass[MAX_STRING_CHARS]; 

	trap->Argv(1, pass, sizeof(pass)); //Password

	if (trap->Argc() == 1) {
		trap->SendServerCommand( ent-g_entities, "print \"Usage: clanPass <password>\n\"" ); 
		return; 
	}
	if (trap->Argc() == 2) {
		Q_strncpyz( ent->client->pers.clanpass, pass, sizeof(ent->client->pers.clanpass) );
	}
}

void Cmd_SayTeamMod_f(gentity_t *ent)//clanpass
{
	char   type[MAX_STRING_CHARS]; 

	trap->Argv(1, type, sizeof(type)); //Type

	if (trap->Argc() == 1) {
		trap->SendServerCommand( ent-g_entities, "print \"Usage: say_team_mod <normal, clan, or admin>\n\"" ); 
		return; 
	}
	if (trap->Argc() == 2) {
		if (!Q_stricmp(type, "normal"))
			ent->client->pers.sayteammod = 0;
		else if (!Q_stricmp(type, "clan"))
			ent->client->pers.sayteammod = 1;
		else if (!Q_stricmp(type, "admin"))
			ent->client->pers.sayteammod = 2;
	}
}


//[JAPRO - Serverside - All - Amlogin Function - Start]
/*
=================
Cmd_Amlogin_f
=================
*/
void Cmd_Amlogin_f(gentity_t *ent)
{
	char   pass[MAX_STRING_CHARS]; 

	trap->Argv( 1, pass, sizeof( pass ) ); //Password

	if (trap->Argc() == 1)
	{
		trap->SendServerCommand( ent-g_entities, "print \"Usage: amLogin <password>\n\"" ); 
		return; 
	}
	if (trap->Argc() == 2) 
	{
		if ( ent->r.svFlags & SVF_JUNIORADMIN || ent->r.svFlags & SVF_FULLADMIN)
		{
			trap->SendServerCommand( ent-g_entities, "print \"You are already logged in. Type in /amLogout to remove admin status.\n\"" ); 
			return; 
		}
		if (!Q_stricmp( pass, "" ))
		{
			trap->SendServerCommand( ent-g_entities, "print \"Usage: amLogin <password>\n\"" ); 
			return;
		}
		if ( !Q_stricmp( pass, g_fullAdminPass.string ) )
		{
			if ( !Q_stricmp( "", g_fullAdminPass.string ) )//dunno
				return;
			ent->r.svFlags |= SVF_FULLADMIN;
			trap->SendServerCommand( ent-g_entities, "print \"^2You are now logged in with full admin privileges.\n\"");
			if (Q_stricmp(g_fullAdminMsg.string, "" ))
				trap->SendServerCommand( -1, va("print \"%s ^7%s\n\"", ent->client->pers.netname, g_fullAdminMsg.string ));
			return; 
		}
		if ( !Q_stricmp( pass, g_juniorAdminPass.string ) )
		{
			if ( !Q_stricmp( "", g_juniorAdminPass.string ) )
				return;
			ent->r.svFlags |= SVF_JUNIORADMIN;
			trap->SendServerCommand( ent-g_entities, "print \"^2You are now logged in with junior admin privileges.\n\"");
			if (Q_stricmp(g_juniorAdminMsg.string, "" ))
				trap->SendServerCommand( -1, va("print \"%s ^7%s\n\"", ent->client->pers.netname, g_juniorAdminMsg.string ));
			return; 
		}
		else 
		{
			trap->SendServerCommand( ent-g_entities, "print \"^3Failed to log in: Incorrect password!\n\"");
		}
	}
}
//[JAPRO - Serverside - All - Amlogin Function - End]

//[JAPRO - Serverside - All - Amlogout Function - Start]
/*
=================
Cmd_Amlogout_f
=================
*/
void Cmd_Amlogout_f(gentity_t *ent)
{
	if (ent->r.svFlags & SVF_FULLADMIN || ent->r.svFlags & SVF_JUNIORADMIN)
	{ 
		ent->r.svFlags &= ~SVF_FULLADMIN;
		ent->r.svFlags &= ~SVF_JUNIORADMIN;
		trap->SendServerCommand( ent-g_entities, "print \"You are no longer an admin.\n\"");         
	}
}
//[JAPRO - Serverside - All - Amlogout Function - End]


//[JAPRO - Serverside - All - Amlockteam Function - Start]
/*
=================
Cmd_Amlockteam_f
=================
*/
void Cmd_Amlockteam_f(gentity_t *ent)
{
	char teamname[MAX_TEAMNAME];

		if (ent->r.svFlags & SVF_FULLADMIN)//Logged in as full admin
		{
			if (!(g_fullAdminLevel.integer & (1 << A_LOCKTEAM)))
			{
				trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command (amLockTeam).\n\"" );
				return;
			}
		}
		else if (ent->r.svFlags & SVF_JUNIORADMIN)//Logged in as junior admin
		{
			if (!(g_juniorAdminLevel.integer & (1 << A_LOCKTEAM)))
			{
				trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command (amLockTeam).\n\"" );
				return;
			}
		}
		else//Not logged in
		{
			trap->SendServerCommand( ent-g_entities, "print \"You must be logged in to use this command.\n\"" );
			return;
		}

		if (g_gametype.integer >= GT_TEAM || g_gametype.integer == GT_FFA)
		{
			if (trap->Argc() != 2)
			{
				trap->SendServerCommand( ent-g_entities, "print \"Usage: /amLockTeam <team>\n\"");
				return;
			}

			trap->Argv( 1, teamname, sizeof( teamname ) );
				
			if (!Q_stricmp(teamname, "red") || !Q_stricmp( teamname, "r"))
			{
				if (level.isLockedred == qfalse)
				{
					level.isLockedred = qtrue;
					trap->SendServerCommand( -1, "print \"The Red team is now locked.\n\"");
				}
				else
				{
					level.isLockedred = qfalse;
					trap->SendServerCommand( -1, "print \"The Red team is now unlocked.\n\"");
				}
			}
			else if (!Q_stricmp( teamname, "blue") || !Q_stricmp(teamname, "b"))
			{
				if (level.isLockedblue == qfalse)
				{
					level.isLockedblue = qtrue;
					trap->SendServerCommand( -1, "print \"The Blue team is now locked.\n\"");
				}
				else
				{
					level.isLockedblue = qfalse;
					trap->SendServerCommand( -1, "print \"The Blue team is now unlocked.\n\"");
				}
			}
			else if(!Q_stricmp(teamname, "s") || !Q_stricmp( teamname, "spectator") || !Q_stricmp(teamname, "spec") || !Q_stricmp(teamname, "spectate"))
			{
				if (level.isLockedspec == qfalse)
				{
					level.isLockedspec = qtrue;
					trap->SendServerCommand( -1, "print \"The Spectator team is now locked.\n\"");
				}
				else
				{
					level.isLockedspec = qfalse;
					trap->SendServerCommand( -1, "print \"The spectator team is now unlocked.\n\"");
				}
			}
			else if(!Q_stricmp(teamname, "f") || !Q_stricmp(teamname, "free") || !Q_stricmp(teamname, "join") || !Q_stricmp(teamname, "enter") || !Q_stricmp(teamname, "j"))
			{
				if (level.isLockedfree == qfalse)
				{
					level.isLockedfree = qtrue;
					trap->SendServerCommand( -1, "print \"The Free team is now locked.\n\"");
				}
				else
				{
					level.isLockedfree = qfalse;
					trap->SendServerCommand( -1, "print \"The Free team is now unlocked.\n\"");
				}
			}
			else
			{
				trap->SendServerCommand( ent-g_entities, "print \"Usage: /amLockTeam <team>\n\"");
				return;
			}
		}
		else
		{
			trap->SendServerCommand( ent-g_entities, "print \"You can not use this command in this gametype (amLockTeam).\n\"" );
			return;
		}
}
//[JAPRO - Serverside - All - Amlockteam Function - End]


//[JAPRO - Serverside - All - Amforceteam Function - Start]
/*
=================
Cmd_Amforceteam_f

=================
*/
void Cmd_Amforceteam_f(gentity_t *ent)
{
		char arg[MAX_NETNAME]; 
		char teamname[MAX_TEAMNAME];
		int  clientid = 0;//stfu compiler

		if (ent->r.svFlags & SVF_FULLADMIN)//Logged in as full admin
		{
			if (!(g_fullAdminLevel.integer & (1 << A_FORCETEAM)))
			{
				trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command (amForceTeam).\n\"" );
				return;
			}
		}
		else if (ent->r.svFlags & SVF_JUNIORADMIN)//Logged in as junior admin
		{
			if (!(g_juniorAdminLevel.integer & (1 << A_FORCETEAM)))
			{
				trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command (amForceTeam).\n\"" );
				return;
			}
		}
		else//Not logged in
		{
			trap->SendServerCommand( ent-g_entities, "print \"You must be logged in to use this command (amForceTeam).\n\"" );
			return;
		}

		if (trap->Argc() != 3) 
        { 
            trap->SendServerCommand( ent-g_entities, "print \"Usage: amForceTeam <client> <team>\n\"" ); 
            return; 
        }

		if (g_gametype.integer >= GT_TEAM || g_gametype.integer == GT_FFA)
		{	
			qboolean everyone = qfalse;
			gclient_t *client;
			int i;

			trap->Argv(1, arg, sizeof(arg));

			if (!Q_stricmp(arg, "-1"))
				everyone = qtrue;

			if (!everyone)
			{
				clientid = JP_ClientNumberFromString(ent, arg);

				if (clientid == -1 || clientid == -2)//No clients or multiple clients are a match
				{ 
					return; 
				} 

				if ((g_entities[clientid].r.svFlags & SVF_FULLADMIN) || (ent->r.svFlags & SVF_JUNIORADMIN && g_entities[clientid].r.svFlags & SVF_JUNIORADMIN))
				{
					if (g_entities[clientid].client->ps.clientNum != ent->client->ps.clientNum)
						return;
					else
						trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command on this player (amForceTeam).\n\"" );
				}
			}

			trap->Argv(2, teamname, sizeof(teamname));

			if (!Q_stricmp(teamname, "red") || !Q_stricmp(teamname, "r") && g_gametype.integer >= GT_TEAM)
			{
				if (everyone)
				{
					for (i = 0, client = level.clients; i < level.maxclients; ++i, ++client)
					{
						if (client->pers.connected != CON_CONNECTED)//client->sess.sessionTeam
							continue;
						if (client->sess.sessionTeam != TEAM_RED)
							SetTeam(&g_entities[i], "red", qtrue );
					}
				}
				else
				{
					if (g_entities[clientid].client->sess.sessionTeam != TEAM_RED) {
						SetTeam(&g_entities[clientid], "red", qtrue );
						trap->SendServerCommand( -1, va("print \"%s ^7has been forced to the ^1Red ^7team.\n\"", g_entities[clientid].client->pers.netname));
					}
				}
			}
			else if (!Q_stricmp(teamname, "blue") || !Q_stricmp( teamname, "b") && g_gametype.integer >= GT_TEAM)
			{
				if (everyone)
				{
					for (i = 0, client = level.clients; i < level.maxclients; ++i, ++client)
					{
						if (client->pers.connected != CON_CONNECTED)//client->sess.sessionTeam
							continue;
						if (client->sess.sessionTeam != TEAM_BLUE)
							SetTeam(&g_entities[i], "blue", qtrue );
					}
				}
				else
				{
					if (g_entities[clientid].client->sess.sessionTeam != TEAM_BLUE) {
						SetTeam(&g_entities[clientid], "blue", qtrue );
						trap->SendServerCommand( -1, va("print \"%s ^7has been forced to the ^4Blue ^7team.\n\"", g_entities[clientid].client->pers.netname));
					}
				}
			}
			else if (!Q_stricmp( teamname, "s") || !Q_stricmp(teamname, "spectator")  || !Q_stricmp(teamname, "spec") || !Q_stricmp(teamname, "spectate"))
			{
				if (everyone)
				{
					for (i = 0, client = level.clients; i < level.maxclients; ++i, ++client)
					{
						if (client->pers.connected != CON_CONNECTED)//client->sess.sessionTeam
							continue;
						if (client->sess.sessionTeam != TEAM_SPECTATOR)
							SetTeam(&g_entities[i], "spectator", qtrue );
					}
				}
				else
				{
					if (g_entities[clientid].client->sess.sessionTeam != TEAM_SPECTATOR) {
						SetTeam(&g_entities[clientid], "spectator", qtrue );
						trap->SendServerCommand( -1, va("print \"%s ^7has been forced to the ^3Spectator ^7team.\n\"", g_entities[clientid].client->pers.netname));
					}
				}
			}
			else if (!Q_stricmp( teamname, "f") || !Q_stricmp( teamname, "free") || !Q_stricmp(teamname, "join") || !Q_stricmp(teamname, "j") || !Q_stricmp(teamname, "enter"))
			{
				if (everyone)
				{
					for (i = 0, client = level.clients; i < level.maxclients; ++i, ++client)
					{
						if (client->pers.connected != CON_CONNECTED)//client->sess.sessionTeam
							continue;
						if (client->sess.sessionTeam != TEAM_FREE)
							SetTeam(&g_entities[i], "free", qtrue );
					}
				}
				else
				{
					if (g_entities[clientid].client->sess.sessionTeam != TEAM_FREE) {
						SetTeam(&g_entities[clientid], "free", qtrue );
						trap->SendServerCommand( -1, va("print \"%s ^7has been forced to the ^2Free ^7team.\n\"", g_entities[clientid].client->pers.netname));
					}
				}
			}
			else
			{
				trap->SendServerCommand( ent-g_entities, "print \"Usage: amForceTeam <client> <team>\n\"" ); 
			}
		}
		else
		{
			trap->SendServerCommand( ent-g_entities, "print \"You can not use this command in this gametype (amForceTeam).\n\"" );
			return;
		}
}
//[JAPRO - Serverside - All - Amforceteam Function - End]

//[JAPRO - Serverside - All - Ampsay Function - Start]
/*
=================
Cmd_Ampsay_f

=================
*/
void Cmd_Ampsay_f(gentity_t *ent)
{
		int pos = 0;
		char real_msg[MAX_STRING_CHARS];
		char *msg = ConcatArgs(1); 

		while(*msg)
		{ 
			if(msg[0] == '\\' && msg[1] == 'n')
			{ 
				msg++;           // \n is 2 chars, so increase by one here. (one, cuz it's increased down there again...) 
				real_msg[pos++] = '\n';  // put in a real \n 
			} 
			else 
			{ 
				  real_msg[pos++] = *msg;  // otherwise just copy 
			} 
			msg++;                         // increase the msg pointer 
		}
		real_msg[pos] = 0;

		if (ent->r.svFlags & SVF_FULLADMIN)//Logged in as full admin
		{
			if (!(g_fullAdminLevel.integer & (1 << A_CSPRINT)))
			{
				trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command (amPsay).\n\"" );
				return;
			}
		}
		else if (ent->r.svFlags & SVF_JUNIORADMIN)//Logged in as junior admin
		{
			if (!(g_juniorAdminLevel.integer & (1 << A_CSPRINT)))
			{
				trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command (amPsay).\n\"" );
				return;
			}
		}
		else//Not logged in
		{
			trap->SendServerCommand( ent-g_entities, "print \"You must be logged in to use this command (amPsay).\n\"" );
			return;
		}

		if (trap->Argc() < 2) 
		{ 
			trap->SendServerCommand( ent-g_entities, "print \"Usage: /amPsay <message>.\n\"" );
			return; 
		}

		trap->SendServerCommand(-1, va("cp \"%s\"", real_msg));

}
//[JAPRO - Serverside - All - Ampsay Function - End]

//[JAPRO - Serverside - All - Amfreeze Function - Start]
/*
=================
Cmd_Amfreeze_f

=================
*/
void Cmd_Amfreeze_f(gentity_t *ent)
{
		gentity_t * targetplayer;
		int i;
		int clientid = -1; 
		char   arg[MAX_NETNAME]; 

		if (ent->r.svFlags & SVF_FULLADMIN)//Logged in as full admin
		{
			if (!(g_fullAdminLevel.integer & (1 << A_FREEZE)))
			{
				trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command (amFreeze).\n\"" );
				return;
			}
		}
		else if (ent->r.svFlags & SVF_JUNIORADMIN)//Logged in as junior admin
		{
			if (!(g_juniorAdminLevel.integer & (1 << A_FREEZE)))
			{
				trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command (amFreeze).\n\"" );
				return;
			}
		}
		else//Not logged in
		{
			trap->SendServerCommand( ent-g_entities, "print \"You must be logged in to use this command (amFreeze).\n\"" );
			return;
		}

		if (trap->Argc() != 2) 
        { 
			trap->SendServerCommand( ent-g_entities, "print \"Usage: /amFreeze <client> or /amFreeze +all or /amFreeze -all.\n\"" );
			return; 
		}

		trap->Argv(1, arg, sizeof(arg));
		if( Q_stricmp(arg, "+all") == 0)
		{
			for(i = 0; i < level.maxclients; i++)
			{
				targetplayer = &g_entities[i];

				if( targetplayer->client && targetplayer->client->pers.connected)
				{
					if ((g_entities[clientid].r.svFlags & SVF_FULLADMIN) || (ent->r.svFlags & SVF_JUNIORADMIN && g_entities[clientid].r.svFlags & SVF_JUNIORADMIN))
						continue;
					if (!targetplayer->client->pers.amfreeze)
						targetplayer->client->pers.amfreeze = qtrue;
						G_ScreenShake(targetplayer->client->ps.origin, &g_entities[clientid],  3.0f, 2000, qtrue); 
						G_Sound(targetplayer, CHAN_AUTO, G_SoundIndex("sound/ambience/thunder_close1"));
				}
			}
			trap->SendServerCommand( -1, "cp \"You have all been frozen\n\"" ); 
			return;
		}
		else if(Q_stricmp(arg, "-all") == 0)
		{
			for(i = 0; i < level.maxclients; i++)
			{
				targetplayer = &g_entities[i];

				if(targetplayer->client && targetplayer->client->pers.connected)
				{
					if ((g_entities[clientid].r.svFlags & SVF_FULLADMIN) || (ent->r.svFlags & SVF_JUNIORADMIN && g_entities[clientid].r.svFlags & SVF_JUNIORADMIN))
						continue;
					if (targetplayer->client->pers.amfreeze)
					{
						targetplayer->client->pers.amfreeze = qfalse; 
						targetplayer->client->ps.saberCanThrow = qtrue;
						targetplayer->client->ps.forceRestricted = qfalse;
						targetplayer->takedamage = qtrue;
					}
				}
			}
			trap->SendServerCommand( -1, "cp \"You have all been unfrozen\n\"" ); 
			return;
		}
		else
		{
			clientid = JP_ClientNumberFromString(ent, arg);
		}

        if (clientid == -1 || clientid == -2) 
        { 
            return; 
        } 

		if ((g_entities[clientid].r.svFlags & SVF_FULLADMIN) || (ent->r.svFlags & SVF_JUNIORADMIN && g_entities[clientid].r.svFlags & SVF_JUNIORADMIN))
		{
			if (g_entities[clientid].client->ps.clientNum != ent->client->ps.clientNum)
				return;
			else
				trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command on this player (amFreeze).\n\"" );
		}

		if (g_entities[clientid].client->noclip == qtrue)
		{
			g_entities[clientid].client->noclip = qfalse;
		}

		if (g_entities[clientid].client->pers.amfreeze)
		{
			g_entities[clientid].client->pers.amfreeze = qfalse; 
			g_entities[clientid].client->ps.saberCanThrow = qtrue;
			g_entities[clientid].client->ps.forceRestricted = qfalse;
			g_entities[clientid].takedamage = qtrue;
			trap->SendServerCommand(clientid, "cp \"You have been unfrozen\n\""); 
		}
		else
		{
			g_entities[clientid].client->pers.amfreeze = qtrue;
			trap->SendServerCommand(clientid, "cp \"You have been frozen\n\""); 
			G_ScreenShake(g_entities[clientid].client->ps.origin, &g_entities[clientid],  3.0f, 2000, qtrue); 
			G_Sound(&g_entities[clientid], CHAN_AUTO, G_SoundIndex("sound/ambience/thunder_close1"));
		 }
	  }
//[JAPRO - Serverside - All - Amfreeze Function - End]

//[JAPRO - Serverside - All - Amkick Function - Start]
 /*
==================
Cmd_Amkick_f

==================
*/
void Cmd_Amkick_f(gentity_t *ent)
{
		int clientid = -1; 
		char   arg[MAX_NETNAME]; 

		if (ent->r.svFlags & SVF_FULLADMIN)//Logged in as full admin
		{
			if (!(g_fullAdminLevel.integer & (1 << A_ADMINKICK)))
			{
				trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command (amKick).\n\"" );
				return;
			}
		}
		else if (ent->r.svFlags & SVF_JUNIORADMIN)//Logged in as junior admin
		{
			if (!(g_juniorAdminLevel.integer & (1 << A_ADMINKICK)))
			{
				trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command (amKick).\n\"" );
				return;
			}
		}
		else//Not logged in
		{
			trap->SendServerCommand( ent-g_entities, "print \"You must be logged in to use this command (amKick).\n\"" );
			return;
		}

		if (trap->Argc() != 2) 
        { 
			trap->SendServerCommand( ent-g_entities, "print \"Usage: /amKick <client>.\n\"" );
            return; 
        } 

		trap->Argv(1, arg, sizeof(arg)); 
        clientid = JP_ClientNumberFromString(ent, arg);

        if (clientid == -1 || clientid == -2)  
        { 
			return; 
        } 

		if ((g_entities[clientid].r.svFlags & SVF_FULLADMIN) || (ent->r.svFlags & SVF_JUNIORADMIN && g_entities[clientid].r.svFlags & SVF_JUNIORADMIN))
		{
			if (g_entities[clientid].client->ps.clientNum != ent->client->ps.clientNum)
				return;
			else
				trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command on this player (amKick).\n\"" );
		}

		trap->SendConsoleCommand( EXEC_APPEND, va("clientkick %i", clientid) );

}
//[JAPRO - Serverside - All - Amkick Function - End]

//[JAPRO - Serverside - All - Amban Function - Start]
 /*
==================
Cmd_Amban_f

==================
*/
void Cmd_Amban_f(gentity_t *ent)
{
		int clientid = -1; 
		char   arg[MAX_NETNAME]; 

		if (ent->r.svFlags & SVF_FULLADMIN)//Logged in as full admin
		{
			if (!(g_fullAdminLevel.integer & (1 << A_ADMINBAN)))
			{
				trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command (amBan).\n\"" );
				return;
			}
		}
		else if (ent->r.svFlags & SVF_JUNIORADMIN)//Logged in as junior admin
		{
			if (!(g_juniorAdminLevel.integer & (1 << A_ADMINBAN)))
			{
				trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command (amBan).\n\"" );
				return;
			}
		}
		else//Not logged in
		{
			trap->SendServerCommand( ent-g_entities, "print \"You must be logged in to use this command (amBan).\n\"" );
			return;
		}

		if (trap->Argc() != 2) 
        { 
			trap->SendServerCommand( ent-g_entities, "print \"Usage: /amBan <client>.\n\"" );
            return; 
        } 

		trap->Argv(1, arg, sizeof(arg)); 
        clientid = JP_ClientNumberFromString(ent, arg);

        if (clientid == -1 || clientid == -2)  
        { 
			return; 
        } 

		if ((g_entities[clientid].r.svFlags & SVF_FULLADMIN) || (ent->r.svFlags & SVF_JUNIORADMIN && g_entities[clientid].r.svFlags & SVF_JUNIORADMIN))
		{
			if (g_entities[clientid].client->ps.clientNum != ent->client->ps.clientNum)
				return;
			else
				trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command on this player (amBan).\n\"" );
		}

		AddIP(g_entities[clientid].client->sess.IP);
		trap->SendConsoleCommand( EXEC_APPEND, va("clientkick %i", clientid) );

}
//[JAPRO - Serverside - All - Amban Function - End]

//[JAPRO - Serverside - All - Ampsay Function - Start]
/*
=================
Cmd_Ammap_f
=================
*/
void Cmd_Ammap_f(gentity_t *ent)
{
		char    gametype[2]; 
		int		gtype;
		char    mapname[MAX_MAPNAMELENGTH]; 

		if (ent->r.svFlags & SVF_FULLADMIN)//Logged in as full admin
		{
			if (!(g_fullAdminLevel.integer & (1 << A_CHANGEMAP)))
			{
				trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command (amMap).\n\"" );
				return;
			}
		}
		else if (ent->r.svFlags & SVF_JUNIORADMIN)//Logged in as junior admin
		{
			if (!(g_juniorAdminLevel.integer & (1 << A_CHANGEMAP)))
			{
				trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command (amMap).\n\"" );
				return;
			}
		}
		else//Not logged in
		{
			trap->SendServerCommand( ent-g_entities, "print \"You must be logged in to use this command (amMap).\n\"" );
			return;
		}

		if (trap->Argc() != 3) 
		{ 
			trap->SendServerCommand( ent-g_entities, "print \"Usage: /amMap <gametype #> <map>.\n\"" );
			return; 
		}

		trap->Argv(1, gametype, sizeof(gametype));
		trap->Argv(2, mapname, sizeof(mapname));

		if (strchr(mapname, ';') ||  strchr( mapname,'\r') || strchr(mapname, '\n'))
		{
			trap->SendServerCommand( ent-g_entities, "print \"Invalid map string.\n\"" );
			return;
		}

		if (gametype[0] < '0' && gametype[0] > '8')
		{
			trap->SendServerCommand( ent-g_entities, "print \"Invalid gametype.\n\"" );
			return;
		}
	
		gtype = atoi(gametype);

		trap->SendConsoleCommand( EXEC_APPEND, va("g_gametype %i\n", gtype));
		trap->SendConsoleCommand( EXEC_APPEND, va("map %s\n", mapname));

}
//[JAPRO - Serverside - All - Ammap Function - End]

//[JAPRO - Serverside - All - Amvstr Function - Start]
/*
=================
Cmd_Amvstr_f
=================
*/
void Cmd_Amvstr_f(gentity_t *ent)
{
		char   arg[MAX_STRING_CHARS]; 

		if (ent->r.svFlags & SVF_FULLADMIN)//Logged in as full admin
		{
			if (!(g_fullAdminLevel.integer & (1 << A_VSTR)))
			{
				trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command (amVstr).\n\"" );
				return;
			}
		}
		else if (ent->r.svFlags & SVF_JUNIORADMIN)//Logged in as junior admin
		{
			if (!(g_juniorAdminLevel.integer & (1 << A_VSTR)))
			{
				trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command (amVstr).\n\"" );
				return;
			}
		}
		else//Not logged in
		{
			trap->SendServerCommand( ent-g_entities, "print \"You must be logged in to use this command (amVstr).\n\"" );
			return;
		}

		if (trap->Argc() != 2) 
		{ 
			trap->SendServerCommand( ent-g_entities, "print \"Usage: /amVstr <vstr>.\n\"" );
			return; 
		}

		trap->Argv(1, arg, sizeof(arg));

		if (strchr(arg, ';') ||  strchr( arg,'\r') || strchr(arg, '\n'))
		{
			trap->SendServerCommand( ent-g_entities, "print \"Invalid vstr string.\n\"" );
			return;
		}

		trap->SendConsoleCommand( EXEC_APPEND, va("vstr %s\n", arg));

}
//[JAPRO - Serverside - All - Amvstr Function - End]

//[JAPRO - Serverside - All - Amgrantadmin Function - Start]
/*
=================
Cmd_Amgrantadmin_f
=================
*/
void Cmd_Amgrantadmin_f(gentity_t *ent)
{
		char arg[MAX_NETNAME];
		int clientid = -1; 

		if (ent->r.svFlags & SVF_FULLADMIN)//Logged in as full admin
		{
			if (!(g_fullAdminLevel.integer & (1 << A_GRANTADMIN)))
			{
				trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command (amGrantAdmin).\n\"" );
				return;
			}
		}
		else if (ent->r.svFlags & SVF_JUNIORADMIN)//Logged in as junior admin
		{
			if (!(g_juniorAdminLevel.integer & (1 << A_GRANTADMIN)))
			{
				trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command (amGrantAdmin).\n\"" );
				return;
			}
		}
		else//Not logged in
		{
			trap->SendServerCommand( ent-g_entities, "print \"You must be logged in to use this command (amGrantAdmin).\n\"" );
			return;
		}

		if (trap->Argc() != 2 && trap->Argc() != 3) 
		{ 
			trap->SendServerCommand( ent-g_entities, "print \"Usage: /amGrantAdmin <client>.\n\"" );
			return; 
		}

		trap->Argv(1, arg, sizeof(arg)); 
		clientid = JP_ClientNumberFromString(ent, arg);

		if (clientid == -1 || clientid == -2)  
			return;  

		if ((g_entities[clientid].r.svFlags & SVF_FULLADMIN) || (ent->r.svFlags & SVF_JUNIORADMIN && g_entities[clientid].r.svFlags & SVF_JUNIORADMIN))
		{
			if (g_entities[clientid].client->ps.clientNum != ent->client->ps.clientNum)
				return;
			else
				trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command on this player (amGrantAdmin).\n\"" );
		}

		if (trap->Argc() == 2) {

			if (!(g_entities[clientid].r.svFlags & SVF_JUNIORADMIN) && !(g_entities[clientid].r.svFlags & SVF_FULLADMIN))
			{
				trap->SendServerCommand( clientid, "print \"You have been granted Junior admin privileges.\n\"" );
				g_entities[clientid].r.svFlags |= SVF_JUNIORADMIN;
			}
		}
		else if (trap->Argc() == 3) {
			trap->Argv(2, arg, sizeof(arg)); 
			if (!Q_stricmp(arg, "none")) {
				g_entities[clientid].r.svFlags &= ~SVF_JUNIORADMIN;
			}
		}

}
//[JAPRO - Serverside - All - Amgrantadmin Function - End]

void Cmd_Showmotd_f(gentity_t *ent)
{
	if (Q_stricmp(g_centerMOTD.string, "" ))
		strcpy(ent->client->csMessage, G_NewString(va("^7%s\n", g_centerMOTD.string )));//Loda fixme, resize this so it does not allocate more than it needs (game_memory crash eventually?)
	ent->client->csTimeLeft = g_centerMOTDTime.integer;
}


//[JAPRO - Serverside - All - Aminfo Function - Start]
/*
=================
Cmd_Aminfo_f
=================
*/
void Cmd_Aminfo_f(gentity_t *ent)
{
	if (ent && ent->client)
		trap->SendServerCommand( ent-g_entities, va("print \"^5 Hi there, %s^5.  This server is using the jaPRO mod.\n\"", ent->client->pers.netname));
	trap->SendServerCommand( ent-g_entities, "print \"   ^3To display server settings, type ^7serverConfig\n\"" );

	trap->SendServerCommand( ent-g_entities, "print \"   ^3Chat commands: \"" );
	trap->SendServerCommand( ent-g_entities, "print \"ignore \"" ); 
	trap->SendServerCommand( ent-g_entities, "print \"clanPass \"" ); 
	trap->SendServerCommand( ent-g_entities, "print \"clanWhoIs \"" ); 
	trap->SendServerCommand( ent-g_entities, "print \"clanSay \"" ); 
	trap->SendServerCommand( ent-g_entities, "print \"amSay \"" ); 
	trap->SendServerCommand( ent-g_entities, "print \"say_team_mod \"" ); 
	trap->SendServerCommand( ent-g_entities, "print \"\n\"" );

	trap->SendServerCommand( ent-g_entities, "print \"   ^3Game commands: \"" );
	trap->SendServerCommand( ent-g_entities, "print \"amMOTD \"" ); 
	if (g_privateDuel.integer) {
		trap->SendServerCommand( ent-g_entities, "print \"engage_FullForceDuel \"" ); 
		trap->SendServerCommand( ent-g_entities, "print \"engage_gunDuel \"" ); 
	}
	if (g_raceMode.integer > 1) 
		trap->SendServerCommand( ent-g_entities, "print \"race \"" ); 
	if (g_raceMode.integer) 
		trap->SendServerCommand( ent-g_entities, "print \"movementStyle \"" ); 
	if (g_allowSaberSwitch.integer) 
		trap->SendServerCommand( ent-g_entities, "print \"saber \"" ); 
	if (g_allowFlagThrow.integer) 
		trap->SendServerCommand( ent-g_entities, "print \"throwFlag \"" ); 
	if (g_tweakJetpack.integer) 
		trap->SendServerCommand( ent-g_entities, "print \"+button12 (jetpack) \"" ); 
	trap->SendServerCommand( ent-g_entities, "print \"\n\"" );

	trap->SendServerCommand( ent-g_entities, "print \"   ^3Emote commands: \"" );
	if (!(g_emotesDisable.integer & (1 << E_BEG)))
		trap->SendServerCommand( ent-g_entities, "print \"amBeg \"" ); 
	if (!(g_emotesDisable.integer & (1 << E_BEG2)))
		trap->SendServerCommand( ent-g_entities, "print \"amBeg2 \"" ); 
	if (!(g_emotesDisable.integer & (1 << E_BERNIE)))
		trap->SendServerCommand( ent-g_entities, "print \"amBernie \"" ); 
	if (!(g_emotesDisable.integer & (1 << E_BREAKDANCE)))
		trap->SendServerCommand( ent-g_entities, "print \"amBreakdance \"" ); 
	if (!(g_emotesDisable.integer & (1 << E_BREAKDANCE2)))
		trap->SendServerCommand( ent-g_entities, "print \"amBreakdance2 \"" ); 
	if (!(g_emotesDisable.integer & (1 << E_BREAKDANCE3)))
		trap->SendServerCommand( ent-g_entities, "print \"amBreakdance3 \"" ); 
	if (!(g_emotesDisable.integer & (1 << E_BREAKDANCE4)))
		trap->SendServerCommand( ent-g_entities, "print \"amBreakdance4 \"" ); 
	if (!(g_emotesDisable.integer & (1 << E_CHEER)))
		trap->SendServerCommand( ent-g_entities, "print \"amCheer \"" ); 
	if (!(g_emotesDisable.integer & (1 << E_COWER)))
		trap->SendServerCommand( ent-g_entities, "print \"amCower \"" ); 
	if (!(g_emotesDisable.integer & (1 << E_DANCE)))
		trap->SendServerCommand( ent-g_entities, "print \"amDance \"" ); 
	if (!(g_emotesDisable.integer & (1 << E_HUG)))
		trap->SendServerCommand( ent-g_entities, "print \"amHug \"" ); 
	if (!(g_emotesDisable.integer & (1 << E_NOISY)))
		trap->SendServerCommand( ent-g_entities, "print \"amNoisy \"" ); 
	if (!(g_emotesDisable.integer & (1 << E_POINT)))
		trap->SendServerCommand( ent-g_entities, "print \"amPoint \"" ); 
	if (!(g_emotesDisable.integer & (1 << E_RAGE)))
		trap->SendServerCommand( ent-g_entities, "print \"amRage \"" ); 
	if (!(g_emotesDisable.integer & (1 << E_SIT)))
		trap->SendServerCommand( ent-g_entities, "print \"amSit \"" ); 
	if (!(g_emotesDisable.integer & (1 << E_SIT2)))
		trap->SendServerCommand( ent-g_entities, "print \"amSit2 \"" ); 
	if (!(g_emotesDisable.integer & (1 << E_SIT3)))
		trap->SendServerCommand( ent-g_entities, "print \"amSit3 \"" ); 
	if (!(g_emotesDisable.integer & (1 << E_SIT4)))
		trap->SendServerCommand( ent-g_entities, "print \"amSit4 \"" ); 
	if (!(g_emotesDisable.integer & (1 << E_SIT5)))
		trap->SendServerCommand( ent-g_entities, "print \"amSit5 \"" ); 
	if (!(g_emotesDisable.integer & (1 << E_SURRENDER)))
		trap->SendServerCommand( ent-g_entities, "print \"amSurrender \"" ); 
	if (!(g_emotesDisable.integer & (1 << E_SMACK)))
		trap->SendServerCommand( ent-g_entities, "print \"amSmack \"" ); 
	if (!(g_emotesDisable.integer & (1 << E_TAUNT)))
		trap->SendServerCommand( ent-g_entities, "print \"amTaunt \"" ); 
	if (!(g_emotesDisable.integer & (1 << E_TAUNT2)))
		trap->SendServerCommand( ent-g_entities, "print \"amTaunt2 \"" ); 
	if (!(g_emotesDisable.integer & (1 << E_VICTORY)))
		trap->SendServerCommand( ent-g_entities, "print \"amVictory \"" ); 
	if (!(g_emotesDisable.integer & (1 << E_JAWARUN)))
		trap->SendServerCommand( ent-g_entities, "print \"amRun \"" ); 
	trap->SendServerCommand( ent-g_entities, "print \"\n\"" );

	trap->SendServerCommand( ent-g_entities, "print \"   ^3Admin commands: \"" );
	if (!(ent->r.svFlags & SVF_FULLADMIN) && !(ent->r.svFlags & SVF_JUNIORADMIN))
		trap->SendServerCommand( ent-g_entities, "print \"you are not an administrator on this server.\n\"" );
	else {
		if ((ent->r.svFlags & SVF_FULLADMIN) && (g_fullAdminLevel.integer & (1 << A_ADMINTELE))) 
			trap->SendServerCommand( ent-g_entities, "print \"amTele \"" ); 
		if ((ent->r.svFlags & SVF_JUNIORADMIN) && (g_juniorAdminLevel.integer & (1 << A_ADMINTELE))) 
			trap->SendServerCommand( ent-g_entities, "print \"amTele \"" ); 
		if ((ent->r.svFlags & SVF_FULLADMIN) && (g_fullAdminLevel.integer & (1 << A_TELEMARK))) 
			trap->SendServerCommand( ent-g_entities, "print \"amTeleMark \"" ); 
		if ((ent->r.svFlags & SVF_JUNIORADMIN) && (g_juniorAdminLevel.integer & (1 << A_TELEMARK))) 
			trap->SendServerCommand( ent-g_entities, "print \"amTeleMark \"" ); 
		if ((ent->r.svFlags & SVF_FULLADMIN) && (g_fullAdminLevel.integer & (1 << A_FREEZE))) 
			trap->SendServerCommand( ent-g_entities, "print \"amFreeze \"" ); 
		if ((ent->r.svFlags & SVF_JUNIORADMIN) && (g_juniorAdminLevel.integer & (1 << A_FREEZE))) 
			trap->SendServerCommand( ent-g_entities, "print \"amFreeze \"" ); 
		if ((ent->r.svFlags & SVF_FULLADMIN) && (g_fullAdminLevel.integer & (1 << A_ADMINBAN))) 
			trap->SendServerCommand( ent-g_entities, "print \"amBan \"" ); 
		if ((ent->r.svFlags & SVF_JUNIORADMIN) && (g_juniorAdminLevel.integer & (1 << A_ADMINBAN))) 
			trap->SendServerCommand( ent-g_entities, "print \"amBan \"" ); 
		if ((ent->r.svFlags & SVF_FULLADMIN) && (g_fullAdminLevel.integer & (1 << A_ADMINKICK))) 
			trap->SendServerCommand( ent-g_entities, "print \"amKick \"" ); 
		if ((ent->r.svFlags & SVF_JUNIORADMIN) && (g_juniorAdminLevel.integer & (1 << A_ADMINKICK))) 
			trap->SendServerCommand( ent-g_entities, "print \"amKick \"" ); 
		if ((ent->r.svFlags & SVF_FULLADMIN) && (g_fullAdminLevel.integer & (1 << A_NPC))) 
			trap->SendServerCommand( ent-g_entities, "print \"NPC \"" ); 
		if ((ent->r.svFlags & SVF_JUNIORADMIN) && (g_juniorAdminLevel.integer & (1 << A_NPC))) 
			trap->SendServerCommand( ent-g_entities, "print \"NPC \"" ); 
		if ((ent->r.svFlags & SVF_FULLADMIN) && (g_fullAdminLevel.integer & (1 << A_NOCLIP))) 
			trap->SendServerCommand( ent-g_entities, "print \"Noclip \"" ); 
		if ((ent->r.svFlags & SVF_JUNIORADMIN) && (g_juniorAdminLevel.integer & (1 << A_NOCLIP))) 
			trap->SendServerCommand( ent-g_entities, "print \"Noclip \"" ); 
		if ((ent->r.svFlags & SVF_FULLADMIN) && (g_fullAdminLevel.integer & (1 << A_GRANTADMIN))) 
			trap->SendServerCommand( ent-g_entities, "print \"amGrantAdmin \"" ); 
		if ((ent->r.svFlags & SVF_JUNIORADMIN) && (g_juniorAdminLevel.integer & (1 << A_GRANTADMIN))) 
			trap->SendServerCommand( ent-g_entities, "print \"amGrantAdmin \"" ); 
		if ((ent->r.svFlags & SVF_FULLADMIN) && (g_fullAdminLevel.integer & (1 << A_CHANGEMAP))) 
			trap->SendServerCommand( ent-g_entities, "print \"amMap \"" ); 
		if ((ent->r.svFlags & SVF_JUNIORADMIN) && (g_juniorAdminLevel.integer & (1 << A_CHANGEMAP))) 
			trap->SendServerCommand( ent-g_entities, "print \"amMap \"" ); 
		if ((ent->r.svFlags & SVF_FULLADMIN) && (g_fullAdminLevel.integer & (1 << A_CSPRINT))) 
			trap->SendServerCommand( ent-g_entities, "print \"amPsay \"" ); 
		if ((ent->r.svFlags & SVF_JUNIORADMIN) && (g_juniorAdminLevel.integer & (1 << A_CSPRINT))) 
			trap->SendServerCommand( ent-g_entities, "print \"amPsay \"" ); 
		if ((ent->r.svFlags & SVF_FULLADMIN) && (g_fullAdminLevel.integer & (1 << A_FORCETEAM))) 
			trap->SendServerCommand( ent-g_entities, "print \"amForceTeam \"" ); 
		if ((ent->r.svFlags & SVF_JUNIORADMIN) && (g_juniorAdminLevel.integer & (1 << A_FORCETEAM))) 
			trap->SendServerCommand( ent-g_entities, "print \"amForceTeam \"" ); 
		if ((ent->r.svFlags & SVF_FULLADMIN) && (g_fullAdminLevel.integer & (1 << A_LOCKTEAM))) 
			trap->SendServerCommand( ent-g_entities, "print \"amLockTeam \"" ); 
		if ((ent->r.svFlags & SVF_JUNIORADMIN) && (g_juniorAdminLevel.integer & (1 << A_LOCKTEAM))) 
			trap->SendServerCommand( ent-g_entities, "print \"amLockTeam \"" ); 
		if ((ent->r.svFlags & SVF_FULLADMIN) && (g_fullAdminLevel.integer & (1 << A_VSTR))) 
			trap->SendServerCommand( ent-g_entities, "print \"amVstr \"" ); 
		if ((ent->r.svFlags & SVF_JUNIORADMIN) && (g_juniorAdminLevel.integer & (1 << A_VSTR))) 
			trap->SendServerCommand( ent-g_entities, "print \"amVstr \"" ); 
		if ((ent->r.svFlags & SVF_FULLADMIN) && (g_fullAdminLevel.integer & (1 << A_STATUS))) 
			trap->SendServerCommand( ent-g_entities, "print \"amStatus \"" ); 
		if ((ent->r.svFlags & SVF_JUNIORADMIN) && (g_juniorAdminLevel.integer & (1 << A_STATUS))) 
			trap->SendServerCommand( ent-g_entities, "print \"amStatus \"" ); 
		if ((ent->r.svFlags & SVF_FULLADMIN) && (g_fullAdminLevel.integer & (1 << A_RENAME))) 
			trap->SendServerCommand( ent-g_entities, "print \"amRename \"" ); 
		if ((ent->r.svFlags & SVF_JUNIORADMIN) && (g_juniorAdminLevel.integer & (1 << A_RENAME))) 
			trap->SendServerCommand( ent-g_entities, "print \"amRename \"" ); 
		trap->SendServerCommand( ent-g_entities, "print \"\n\"" );
	}
	
	if (ent->client->pers.isJAPRO)
		trap->SendServerCommand( ent-g_entities, "print \"   ^2You are using the client plugin recommended by the server.\n\"" ); 
	else
		trap->SendServerCommand( ent-g_entities, "print \"   ^1You do not have the client plugin.  Download at www.upsgaming.com\n\"" ); 

}
//[JAPRO - Serverside - All - Aminfo Function - End]
//[JAPRO - Serverside - All - Amstatus Function - Start]
static void Cmd_Amstatus_f( gentity_t *ent )
{//Display list of players + clientNum + IP + admin
	int              i;
	char             msg[1024-128] = {0};
	gclient_t        *cl;

	if (ent->r.svFlags & SVF_FULLADMIN) {//Logged in as full admin
		if (!(g_fullAdminLevel.integer & (1 << A_STATUS))) {
			trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command (amStatus).\n\"" );
			return;
		}
	}
	else if (ent->r.svFlags & SVF_JUNIORADMIN) {//Logged in as junior admin
		if (!(g_juniorAdminLevel.integer & (1 << A_STATUS))) {
			trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command (amStatus).\n\"" );
			return;
		}
	}
	else {//Not logged in
		trap->SendServerCommand( ent-g_entities, "print \"You must be logged in to use this command (amStatus).\n\"" );
		return;
	}

	Q_strcat( msg, sizeof( msg ), S_COLOR_CYAN"ID   IP                Plugin    Admin       Name^7\n" );

	for (i=0; i<MAX_CLIENTS; i++)
	{//Build a list of clients
		char *tmpMsg = NULL;
		if (!g_entities[i].inuse)
			continue;

		cl = &level.clients[i];
		if (cl->pers.netname[0])
		{
			char strNum[12] = {0};
			char strName[MAX_NETNAME] = {0};
			char strIP[NET_ADDRSTRMAXLEN] = {0};
			char strAdmin[32] = {0};
			char strPlugin[32] = {0};
			//char *p = NULL;

			Q_strncpyz(strNum, va("(%i)", i), sizeof(strNum));
			Q_strncpyz(strName, cl->pers.netname, sizeof(strName));
			Q_strncpyz(strIP, cl->sess.IP, sizeof(strIP));
			//p = strchr(strIP, ':');
			//if (p) //loda - fix ip sometimes not printing in amstatus?
				//*p = 0;
			if (g_entities[i].r.svFlags & SVF_FULLADMIN)
				Q_strncpyz( strAdmin, "^3Full^7", sizeof(strAdmin));
			else if (g_entities[i].r.svFlags & SVF_JUNIORADMIN)
				Q_strncpyz(strAdmin, "^3Junior^7", sizeof(strAdmin));
			else
				Q_strncpyz(strAdmin, "^7None^7", sizeof(strAdmin));

			Q_strncpyz(strPlugin, (cl->pers.isJAPRO) ? "^2Yes^7" : "^1No^7", sizeof(strPlugin));
			tmpMsg = va( "%-5s%-18s^7%-14s%-16s%s^7\n", strNum, strIP, strPlugin, strAdmin, strName);

			if (strlen(msg) + strlen(tmpMsg) >= sizeof( msg)) {
				trap->SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
				msg[0] = '\0';
			}
			Q_strcat(msg, sizeof(msg), tmpMsg);
		}
	}
	trap->SendServerCommand(ent-g_entities, va("print \"%s\"", msg));
}
//[JAPRO - Serverside - All - Amstatus Function - End]

//Jetpack start
static void Cmd_Jetpack_f(gentity_t *ent)
{
	if (!(g_startingItems.integer & (1 << HI_JETPACK))) {
		trap->SendServerCommand( ent-g_entities, "print \"Command not allowed. (jetpack).\n\"" );
		return;
	}

	if (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK))//Already have it
		ent->client->ps.stats[STAT_HOLDABLE_ITEMS] &= ~(1 << HI_JETPACK);
	else if (!ent->client->ps.duelInProgress)//Dont have it, and not dueling
		ent->client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << HI_JETPACK);
}
//Jetpack end
//[JAPRO - Serverside - All - Clanwhois Function - Start]
/*
=================
Cmd_Clanwhois_f
=================
*/
static void Cmd_Clanwhois_f(gentity_t *ent)
{
		gclient_t *client;
		const int clientNum = ent - g_entities;
		char clanPass[MAX_QPATH];//meh
		int i;

		if (!ent->client)
			return;

		if (trap->Argc() > 1)//Clanwhois <clanpass>
			trap->Argv(1, clanPass, sizeof(clanPass));
		else if (ent->client->pers.clanpass[0] == 0)//Normal clanwhois, and we have no clanpass
			return;

		for (i = 0, client = level.clients; i < level.maxclients; ++i, ++client)
		{
			if (client->pers.connected != CON_CONNECTED)
				continue;

			if (i == ent->client->ps.clientNum)
				continue;

			if (trap->Argc() <= 1)
				Q_strncpyz(clanPass, ent->client->pers.clanpass, sizeof(clanPass));

			if (Q_stricmp(clanPass, client->pers.clanpass))
				continue;

			trap->SendServerCommand(clientNum, va("print \"^5%i ^1- ^7(%s^7)\n\"", i, client->pers.netname)); 
		}
}
//[JAPRO - Serverside - All - Clanwhois Function - End]

extern qboolean BG_InKnockDown( int anim ); //bg_pmove.c

static void DoEmote(gentity_t *ent, int anim, qboolean freeze, qboolean nosaber)
{
	if (!ent->client)
		return;
	if (ent->client->ps.weaponTime > 1 || ent->client->ps.saberMove > 1 || ent->client->ps.fd.forcePowersActive)
		return;
	if (ent->client->ps.groundEntityNum == ENTITYNUM_NONE) //Not on ground
		return;
	if (ent->client->ps.duelInProgress) {
		trap->SendServerCommand(ent-g_entities, "print \"^7Emotes not allowed in duel!\n\"");
		return;
	}
	if (BG_InKnockDown(ent->s.legsAnim))
		return;
	if (BG_InRoll(&ent->client->ps, ent->s.legsAnim))//memes, is this crashing? if ps is null or something?
		return;

	if (freeze) { // Do the anim and freeze it, or cancel if already in it
		if (ent->client->ps.legsAnim == anim) // Cancel the anim if already in it?
			ent->client->emote_freeze = qfalse;
		else //Do the anim
			ent->client->emote_freeze = qtrue;
	}
	if (nosaber && ent->client->ps.weapon == WP_SABER && ent->client->ps.saberHolstered < 2) {
		ent->client->ps.saberCanThrow = qfalse;
		ent->client->ps.saberMove = LS_NONE;
		ent->client->ps.saberBlocked = 0;
		ent->client->ps.saberBlocking = 0;
		ent->client->ps.saberHolstered = 2;
		if (ent->client->saber[0].soundOff)
			G_Sound(ent, CHAN_AUTO, ent->client->saber[0].soundOff);
		if (ent->client->saber[1].soundOff && ent->client->saber[1].model[0])
			G_Sound(ent, CHAN_AUTO, ent->client->saber[1].soundOff);
	}
	StandardSetBodyAnim(ent, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_HOLDLESS);
}

static void Cmd_EmoteSit_f(gentity_t *ent)
{
	if (g_emotesDisable.integer & (1 << E_SIT)) {
		trap->SendServerCommand(ent-g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_SIT1, qtrue, qtrue);
}

static void Cmd_EmoteSit2_f(gentity_t *ent)
{
	if (g_emotesDisable.integer & (1 << E_SIT2)) {
		trap->SendServerCommand(ent-g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_SIT3, qtrue, qtrue);
}

static void Cmd_EmoteSit3_f(gentity_t *ent)
{
	if (g_emotesDisable.integer & (1 << E_SIT3)) {
		trap->SendServerCommand(ent-g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_SIT6, qtrue, qtrue);
}

static void Cmd_EmoteSit4_f(gentity_t *ent)
{
	if (g_emotesDisable.integer & (1 << E_SIT4)) {
		trap->SendServerCommand(ent-g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_SIT2, qtrue, qtrue);
}

static void Cmd_EmoteSurrender_f(gentity_t *ent)
{
	if (g_emotesDisable.integer & (1 << E_SURRENDER)) {
		trap->SendServerCommand(ent-g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, TORSO_SURRENDER_START, qtrue, qtrue);
}

static void Cmd_EmoteCheer_f(gentity_t *ent)
{
	if (g_emotesDisable.integer & (1 << E_CHEER)) {
		trap->SendServerCommand(ent-g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_TUSKENTAUNT1, qfalse, qtrue);
}

static void Cmd_EmoteTaunt_f(gentity_t *ent)
{
	if (g_emotesDisable.integer & (1 << E_TAUNT)) {
		trap->SendServerCommand(ent-g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_ALORA_TAUNT, qfalse, qfalse);
}

static void Cmd_EmoteVictory_f(gentity_t *ent)
{
	if (g_emotesDisable.integer & (1 << E_VICTORY)) {
		trap->SendServerCommand(ent-g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_TAVION_SCEPTERGROUND, qfalse, qfalse);
}

static void Cmd_EmoteRage_f(gentity_t *ent)
{
	if (g_emotesDisable.integer & (1 << E_RAGE)) {
		trap->SendServerCommand(ent-g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_FORCE_RAGE, qfalse, qfalse);
}

static void Cmd_EmoteSmack_f(gentity_t *ent)
{
	if (g_emotesDisable.integer & (1 << E_SMACK)) {
		trap->SendServerCommand(ent-g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_TOSS1, qfalse, qfalse);
}

static void Cmd_EmoteCower_f(gentity_t *ent)
{
	if (g_emotesDisable.integer & (1 << E_COWER)) {
		trap->SendServerCommand(ent-g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_COWER1, qfalse, qtrue);
}


static void Cmd_EmoteNoisy_f(gentity_t *ent)
{
	if (g_emotesDisable.integer & (1 << E_NOISY)) {
		trap->SendServerCommand(ent-g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_SONICPAIN_HOLD, qfalse, qtrue);
}

static void Cmd_EmoteHug_f(gentity_t *ent)
{
	if (g_emotesDisable.integer & (1 << E_HUG)) {
		trap->SendServerCommand(ent-g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_HUGGER1, qfalse, qtrue);
}

static void Cmd_EmoteBeg_f(gentity_t *ent)
{
	if (g_emotesDisable.integer & (1 << E_BEG)) {
		trap->SendServerCommand(ent-g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_KNEES1, qtrue, qtrue);
}

static void Cmd_EmoteBeg2_f(gentity_t *ent)
{
	if (g_emotesDisable.integer & (1 << E_BEG2)) {
		trap->SendServerCommand(ent-g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_KNEES2, qtrue, qtrue);
}

static void Cmd_EmoteBernie_f(gentity_t *ent)
{
	if (g_emotesDisable.integer & (1 << E_BERNIE)) {
		trap->SendServerCommand(ent-g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_FORCE_DRAIN_GRABBED, qtrue, qfalse);
}

static void Cmd_EmoteBreakdance_f(gentity_t *ent)
{
	if (g_emotesDisable.integer & (1 << E_BREAKDANCE)) {
		trap->SendServerCommand(ent-g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_FORCE_GETUP_B2, qfalse, qtrue);
}

static void Cmd_EmoteBreakdance2_f(gentity_t *ent)
{
	if (g_emotesDisable.integer & (1 << E_BREAKDANCE2)) {
		trap->SendServerCommand(ent-g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_FORCE_GETUP_B4, qfalse, qtrue);
}

static void Cmd_EmoteBreakdance3_f(gentity_t *ent)
{
	if (g_emotesDisable.integer & (1 << E_BREAKDANCE3)) {
		trap->SendServerCommand(ent-g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_FORCE_GETUP_B5, qfalse, qtrue);
}

static void Cmd_EmoteBreakdance4_f(gentity_t *ent)
{
	if (g_emotesDisable.integer & (1 << E_BREAKDANCE4)) {
		trap->SendServerCommand(ent-g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_FORCE_GETUP_B6, qfalse, qtrue);
}

static void Cmd_EmoteTaunt2_f(gentity_t *ent)
{
	if (g_emotesDisable.integer & (1 << E_TAUNT2)) {
		trap->SendServerCommand(ent-g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_SCEPTER_HOLD, qfalse, qfalse);
}

static void Cmd_EmoteSit5_f(gentity_t *ent)
{
	if (g_emotesDisable.integer & (1 << E_SIT5)) {
		trap->SendServerCommand(ent-g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_SLEEP6STOP, qtrue, qtrue);
}

static void Cmd_EmoteDance_f(gentity_t *ent)
{
	if (g_emotesDisable.integer & (1 << E_DANCE)) {
		trap->SendServerCommand(ent-g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_STEADYSELF1, qfalse, qtrue);
}

static void Cmd_EmotePoint_f(gentity_t *ent)
{
	if (g_emotesDisable.integer & (1 << E_POINT)) {
		trap->SendServerCommand(ent-g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_STAND5TOAIM, qtrue, qfalse);
}

static void Cmd_AmRun_f(gentity_t *ent)
{
	if (!ent->client)
		return;

	if (ent->client->pers.isJAPRO)//Client is doing all this clientside with prediction, return.
		return;

	if (ent->client->pers.JAWARUN)//Toggle it
		ent->client->pers.JAWARUN = qfalse;
	else
		ent->client->pers.JAWARUN = qtrue;

	//If emote is allowed, print toggle msg?
}

static void Cmd_MovementStyle_f(gentity_t *ent)
{
	char mStyle[32];

	if (!ent->client)
		return;

	if (trap->Argc() != 2) {
		trap->SendServerCommand( ent-g_entities, "print \"Usage: /movementStyle <siege, vq3, qw, or cpm>.\n\"" );
		return;
	}

	if (!g_raceMode.integer) {
		trap->SendServerCommand(ent-g_entities, "print \"This command is not allowed in this gamemode!\n\"");
		return;
	}

	if (!ent->client->pers.raceMode) {
		trap->SendServerCommand(ent-g_entities, "print \"You must be in racemode to use this command!\n\"");
		return;
	}

	if (VectorLength(ent->client->ps.velocity)) {
		trap->SendServerCommand(ent-g_entities, "print \"You must be standing still to use this command!\n\"");
		return;
	}

	if (ent->client->pers.stats.startTime || ent->client->pers.stats.startTimeFlag) {
		trap->SendServerCommand(ent-g_entities, "print \"Movement style updated: timer reset.\n\"");
		ent->client->pers.stats.startLevelTime = 0;
		ent->client->pers.stats.startTime = 0;
		ent->client->pers.stats.topSpeed = 0;
		ent->client->pers.stats.displacement = 0;
		ent->client->pers.stats.startTimeFlag = 0;
		ent->client->pers.stats.topSpeedFlag = 0;
		ent->client->pers.stats.displacementFlag = 0;
	}
	else 
		trap->SendServerCommand(ent-g_entities, "print \"Movement style updated.\n\"");

	trap->Argv(1, mStyle, sizeof(mStyle));

	if (!Q_stricmp("siege", mStyle) || !Q_stricmp("0", mStyle)) {
		ent->client->ps.stats[STAT_MOVEMENTSTYLE] = 0;
		ent->client->pers.movementStyle = 0;
	}
	else if (!Q_stricmp("vq3", mStyle) || !Q_stricmp("jka", mStyle) || !Q_stricmp("1", mStyle)) {
		ent->client->ps.stats[STAT_MOVEMENTSTYLE] = 1;
		ent->client->pers.movementStyle = 1;
	}
	else if (!Q_stricmp("hl2", mStyle) || !Q_stricmp("hl1", mStyle) || !Q_stricmp("hl", mStyle) || !Q_stricmp("qw", mStyle) || !Q_stricmp("2", mStyle)) {
		ent->client->ps.stats[STAT_MOVEMENTSTYLE] = 2;
		ent->client->pers.movementStyle = 2;
	}
	else if (!Q_stricmp("cpm", mStyle) || !Q_stricmp("cpma", mStyle) || !Q_stricmp("3", mStyle)) {
		ent->client->ps.stats[STAT_MOVEMENTSTYLE] = 3;
		ent->client->pers.movementStyle = 3;
	}
}

//[JAPRO - Serverside - All - Amtelemark Function - Start]
void Cmd_Amtelemark_f(gentity_t *ent)
{
		if (ent->r.svFlags & SVF_FULLADMIN)//Logged in as full admin
		{
			if (!(g_fullAdminLevel.integer & (1 << A_TELEMARK)))
			{
				trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command (amTelemark).\n\"" );
				return;
			}
		}
		else if (ent->r.svFlags & SVF_JUNIORADMIN)//Logged in as junior admin
		{
			if (!(g_juniorAdminLevel.integer & (1 << A_TELEMARK)))
			{
				trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command (amTelemark).\n\"" );
				return;
			}
		}
		else//Not logged in
		{
			trap->SendServerCommand( ent-g_entities, "print \"You must be logged in to use this command (amTelemark).\n\"" );
			return;
		}

		VectorCopy(ent->client->ps.origin, ent->client->pers.telemarkOrigin);
		if (ent->client->sess.sessionTeam == TEAM_SPECTATOR && (ent->client->ps.pm_flags & PMF_FOLLOW))
			ent->client->pers.telemarkOrigin[2] += 58;
		ent->client->pers.telemarkAngle = (int)ent->client->ps.viewangles[YAW];
		trap->SendServerCommand( ent-g_entities, va("print \"Teleport Marker: ^3<%i, %i, %i> %i\n\"", (int)ent->client->pers.telemarkOrigin[0], (int)ent->client->pers.telemarkOrigin[1], (int)ent->client->pers.telemarkOrigin[2], ent->client->pers.telemarkAngle ));
}
//[JAPRO - Serverside - All - Amtelemark Function - End]

//[JAPRO - Serverside - All - Amtele Function - Start]
void Cmd_Amtele_f(gentity_t *ent)
{
	gentity_t	*teleporter;// = NULL;
	char client1[MAX_NETNAME], client2[MAX_NETNAME];
	char x[32], y[32], z[32], yaw[32];
	int clientid1 = -1, clientid2 = -1;
	vec3_t	angles = {0, 0, 0}, origin;

		if (ent->r.svFlags & SVF_FULLADMIN)//Logged in as full admin
		{
			if (!(g_fullAdminLevel.integer & (1 << A_ADMINTELE)))
			{
				trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command (amTele).\n\"" );
				return;
			}
		}
		else if (ent->r.svFlags & SVF_JUNIORADMIN)//Logged in as junior admin
		{
			if (!(g_juniorAdminLevel.integer & (1 << A_ADMINTELE)))
			{
				trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command (amTele).\n\"" );
				return;
			}
		}
		else//Not logged in
		{
			trap->SendServerCommand( ent-g_entities, "print \"You must be logged in to use this command (amTele).\n\"" );
			return;
		}

		if (trap->Argc() > 6)
		{
			trap->SendServerCommand( ent-g_entities, "print \"Usage: /amTele or /amTele <client> or /amTele <client> <client> or /amTele <X> <Y> <Z> <YAW> or /amTele <player> <X> <Y> <Z> <YAW>.\n\"" );
			return;
		}

		if (trap->Argc() == 1)//Amtele to telemark
		{ 
			if (ent->client->pers.telemarkOrigin[0] != 0 || ent->client->pers.telemarkOrigin[1] != 0 || ent->client->pers.telemarkOrigin[2] != 0 || ent->client->pers.telemarkAngle != 0)
			{
				angles[YAW] = ent->client->pers.telemarkAngle;
				AmTeleportPlayer( ent, ent->client->pers.telemarkOrigin, angles );
			}
			else
				trap->SendServerCommand( ent-g_entities, "print \"No telemark set!.\n\"" );

			return;
		}

		ent->client->pers.stats.startLevelTime = 0;
		ent->client->pers.stats.startTime = 0;//Dont let admins cheat on timers with this ;d
		ent->client->pers.stats.topSpeed = 0;
		ent->client->pers.stats.displacement = 0;
		ent->client->pers.stats.startTimeFlag = 0;
		ent->client->pers.stats.topSpeedFlag = 0;
		ent->client->pers.stats.displacementFlag = 0;

		if (trap->Argc() == 2)//Amtele to player
		{ 
			trap->Argv(1, client1, sizeof(client1));
			clientid1 = JP_ClientNumberFromString(ent, client1);

			if (clientid1 == -1 || clientid1 == -2)  
				return; 

			origin[0] = g_entities[clientid1].client->ps.origin[0];
			origin[1] = g_entities[clientid1].client->ps.origin[1];
			origin[2] = g_entities[clientid1].client->ps.origin[2] + 96;
			AmTeleportPlayer( ent, origin, angles );
			return;
		}

		if (trap->Argc() == 3)//Amtele player to player
		{ 
			trap->Argv(1, client1, sizeof(client1));
			trap->Argv(2, client2, sizeof(client2));
			clientid1 = JP_ClientNumberFromString(ent, client1);
			clientid2 = JP_ClientNumberFromString(ent, client2);

			if (clientid1 == -1 || clientid1 == -2 || clientid2 == -1 || clientid2 == -2)  
				return; 

			if ((g_entities[clientid1].r.svFlags & SVF_FULLADMIN) || (ent->r.svFlags & SVF_JUNIORADMIN && g_entities[clientid1].r.svFlags & SVF_JUNIORADMIN))//He has admin
			{	
				if (g_entities[clientid1].client->ps.clientNum != ent->client->ps.clientNum)//Hes not me
				{
					trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command on this player (amTele).\n\"" );
					return;
				}
			}

			teleporter = &g_entities[clientid1];

			origin[0] = g_entities[clientid2].client->ps.origin[0];
			origin[1] = g_entities[clientid2].client->ps.origin[1];
			origin[2] = g_entities[clientid2].client->ps.origin[2] + 96;

			AmTeleportPlayer( teleporter, origin, angles );
			return;
		}

		if (trap->Argc() == 4)//|| trap->Argc() == 5)//Amtele to origin (if no angle specified, default 0?)
		{ 
			trap->Argv(1, x, sizeof(x));
			trap->Argv(2, y, sizeof(y));
			trap->Argv(3, z, sizeof(z));

			origin[0] = atoi(x);
			origin[1] = atoi(y);
			origin[2] = atoi(z);

			/*if (trap->Argc() == 5)
			{
				trap->Argv(4, yaw, sizeof(yaw));
				angles[YAW] = atoi(yaw);
			}*/
			
			AmTeleportPlayer( ent, origin, angles );
			return;
		}

		if (trap->Argc() == 5)//Amtele to angles + origin, OR Amtele player to origin
		{
			trap->Argv(1, client1, sizeof(client1));
			clientid1 = JP_ClientNumberFromString(ent, client1);

			if (clientid1 == -1 || clientid1 == -2)//Amtele to origin + angles
			{
				trap->Argv(1, x, sizeof(x));
				trap->Argv(2, y, sizeof(y));
				trap->Argv(3, z, sizeof(z));

				origin[0] = atoi(x);
				origin[1] = atoi(y);
				origin[2] = atoi(z);

				trap->Argv(4, yaw, sizeof(yaw));
				angles[YAW] = atoi(yaw);
			
				AmTeleportPlayer( ent, origin, angles );
			}

			else//Amtele other player to origin
			{
				if ((g_entities[clientid1].r.svFlags & SVF_FULLADMIN) || (ent->r.svFlags & SVF_JUNIORADMIN && g_entities[clientid1].r.svFlags & SVF_JUNIORADMIN))//He has admin
				{	
					if (g_entities[clientid1].client->ps.clientNum != ent->client->ps.clientNum)//Hes not me
					{
						trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command on this player (amTele).\n\"" );
						return;
					}
				}

				teleporter = &g_entities[clientid1];

				trap->Argv(2, x, sizeof(x));
				trap->Argv(3, y, sizeof(y));
				trap->Argv(4, z, sizeof(z));

				origin[0] = atoi(x);
				origin[1] = atoi(y);
				origin[2] = atoi(z);

				AmTeleportPlayer( teleporter, origin, angles );
			}
			return;

		}

		if (trap->Argc() == 6)//Amtele player to angles + origin
		{
			trap->Argv(1, client1, sizeof(client1));
			clientid1 = JP_ClientNumberFromString(ent, client1);

			if (clientid1 == -1 || clientid1 == -2)
				return;

			if ((g_entities[clientid1].r.svFlags & SVF_FULLADMIN) || (ent->r.svFlags & SVF_JUNIORADMIN && g_entities[clientid1].r.svFlags & SVF_JUNIORADMIN))//He has admin
			{	
				if (g_entities[clientid1].client->ps.clientNum != ent->client->ps.clientNum)//Hes not me
				{
					trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command on this player (amTele).\n\"" );
					return;
				}
			}

			teleporter = &g_entities[clientid1];

			trap->Argv(2, x, sizeof(x));
			trap->Argv(3, y, sizeof(y));
			trap->Argv(4, z, sizeof(z));

			origin[0] = atoi(x);
			origin[1] = atoi(y);
			origin[2] = atoi(z);

			trap->Argv(5, yaw, sizeof(yaw));
			angles[YAW] = atoi(yaw);
			
			AmTeleportPlayer( teleporter, origin, angles );
			return;
		}

}
//[JAPRO - Serverside - All - Amtele Function - End]
//[JAPRO - Serverside - All - Amrename - Start]
void Cmd_Amrename_f(gentity_t *ent)
{
	int		clientid = -1; 
	char	arg[MAX_STRING_CHARS];
	char	userinfo[MAX_INFO_STRING]; 
   
	if (ent->r.svFlags & SVF_FULLADMIN)//Logged in as full admin
	{
		if (!(g_fullAdminLevel.integer & (1 << A_RENAME)))
		{
			trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command (amRename).\n\"" );
			return;
		}
	}
	else if (ent->r.svFlags & SVF_JUNIORADMIN)//Logged in as junior admin
	{
		if (!(g_juniorAdminLevel.integer & (1 << A_RENAME)))
		{
			trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command (amRename).\n\"" );
			return;
		}
	}
	else//Not logged in
	{
		trap->SendServerCommand( ent-g_entities, "print \"You must be logged in to use this command (amRename).\n\"" );
		return;
	}
 
   if ( trap->Argc() != 3) 
   { 
     trap->SendServerCommand( ent-g_entities, "print \"Usage: /amRename <client> <newname>.\n\"" );
      return;
   } 
   trap->Argv(1, arg, sizeof(arg)); 

   clientid = JP_ClientNumberFromString(ent, arg);

	if (clientid == -1 || clientid == -2)
		return;

   trap->Argv(2, arg, sizeof(arg)); 

   trap->GetUserinfo(clientid, userinfo, sizeof(userinfo)); 
   Info_SetValueForKey(userinfo, "name", arg);
   trap->SetUserinfo(clientid, userinfo); 
   ClientUserinfoChanged(clientid); 
   level.clients[clientid].pers.netnameTime = level.time + 5000;
}
//[JAPRO - Serverside - All - Amrename - End]
void Cmd_Race_f(gentity_t *ent)
{
	if (!ent->client)
		return;

	if (g_raceMode.integer < 2) {
		trap->SendServerCommand(ent-g_entities, "print \"^5This command is not allowed!\n\"");
		ent->client->pers.raceMode = qfalse;
		return;
	}

	if (ent->client->sess.sessionTeam != TEAM_SPECTATOR) {
		trap->SendServerCommand(ent-g_entities, "print \"^5You can only use this command in spectate!\n\"");
		return;
	}

	if (g_gametype.integer != GT_FFA) {
		trap->SendServerCommand(ent-g_entities, "print \"^5This command is not allowed in this gametype!\n\"");
		return;
	}

	//Must be in spectate to use?

	if (ent->client->pers.raceMode) {//Toggle it
		ent->client->pers.raceMode = qfalse;
		trap->SendServerCommand(ent-g_entities, "print \"^5Race mode toggled off.\n\"");
	}
	else {
		ent->client->pers.raceMode = qtrue;
		trap->SendServerCommand(ent-g_entities, "print \"^5Race mode toggled on.\n\"");
	}
}
//[JAPRO - Serverside - All - Serverconfig - Start]
void Cmd_ServerConfig_f(gentity_t *ent)
{
	//Global, important changes
	trap->SendServerCommand(ent-g_entities, "print \" ^3Global Changes:\n\"");
	trap->SendServerCommand(ent-g_entities, va("print \"     ^5Server tickrate^3: ^2%i\n\"", sv_fps.integer));
	trap->SendServerCommand(ent-g_entities, va("print \"     ^5Force regen time (BaseJKA 'sv_fps 20' equivalent)^3: ^2%i\n\"", g_forceRegenTime.integer - (1000/20)));
	trap->SendServerCommand(ent-g_entities, va("print \"     ^5Location based damage^3: ^2%s\n\"", (g_locationBasedDamage.integer) ? "Yes" : "No"));
	if (!(dmflags.integer & DF_NO_FALLING) && g_maxFallDmg.integer)
		trap->SendServerCommand(ent-g_entities, va("print \"     ^5Fall damage capped at^3: ^2%i\n\"", g_maxFallDmg.integer));
	if (g_fixKillCredit.integer == 1)
		trap->SendServerCommand(ent-g_entities, "print \"     ^5Prevents suiciding/spectating to avoid a kill\n\"");
	else if (g_fixKillCredit.integer > 1)
		trap->SendServerCommand(ent-g_entities, "print \"     ^5Prevents suiciding/spectating/disconnecting to avoid a kill\n\"");
	if (g_corpseRemovalTime.integer != 30)
		trap->SendServerCommand(ent-g_entities, va("print \"     ^5Corpses dissapear after ^2%i ^5seconds\n\"", g_corpseRemovalTime.integer));
	if (g_newBotAI.integer)
		trap->SendServerCommand(ent-g_entities, va("print \"     ^5New bot AI for force and saber-only combat\n\"", g_corpseRemovalTime.integer));
	if (g_raceMode.integer == 1)
		trap->SendServerCommand(ent-g_entities, "print \"     ^5/race mode is enabled\n\"");
	else if (g_raceMode.integer > 1)
		trap->SendServerCommand(ent-g_entities, "print \"     ^5/race mode option is enabled\n\"");

	//Saber changes
	trap->SendServerCommand(ent-g_entities, "print \" ^3Saber Changes:\n\"");
	trap->SendServerCommand(ent-g_entities, va("print \"     ^5Saber style damage^3: ^2%s\n\"", (d_saberSPStyleDamage.integer) ? "SP" : "MP"));
	if (!d_saberGhoul2Collision.integer)
		trap->SendServerCommand(ent-g_entities, "print \"     ^5Larger, square hitboxes for lightsabers\n\"");
	if (d_saberBoxTraceSize.integer)
		trap->SendServerCommand(ent-g_entities, va("print \"     ^5Increased saber hitbox size by: ^2%i\n\"", d_saberBoxTraceSize.integer));
	if (d_saberInterpolate.integer)
		trap->SendServerCommand(ent-g_entities, "print \"     ^5Saber interpolate\n\"");	
	if (g_duelStartArmor.integer)
		trap->SendServerCommand(ent-g_entities, va("print \"     ^5Duelers start with ^2%i ^5armor\n\"", g_duelStartArmor.integer));
	if (g_duelStartHealth.integer && g_duelStartHealth.integer != 100)
		trap->SendServerCommand(ent-g_entities, va("print \"     ^5Duelers start with ^2%i ^5health\n\"", g_duelStartHealth.integer));
	if (g_allowSaberSwitch.integer)
		trap->SendServerCommand(ent-g_entities, "print \"     ^5Allow saber switch\n\"");
	if (!d_saberSPStyleDamage.integer && !g_saberTouchDmg.integer)
		trap->SendServerCommand(ent-g_entities, "print \"     ^5No saber touch damage\n\"");
	trap->SendServerCommand(ent-g_entities, va("print \"     ^5Saber kick tweak^3: ^2%s\n\"", (d_saberKickTweak.integer) ? "Yes" : "No"));
	if (g_fixGroundStab.integer == 1)
		trap->SendServerCommand(ent-g_entities, "print \"     ^5Groundstabs damage players not on ground\n\"");
	else if (g_fixGroundStab.integer > 1)
		trap->SendServerCommand(ent-g_entities, "print \"     ^5Groundstabs damage players not on ground, but with reduced damage\n\"");
	if (g_backslashDamageScale.value != 1)
		trap->SendServerCommand(ent-g_entities, va("print \"     ^5Backslash damage scale^3: ^2%.2f\n\"", g_backslashDamageScale.value));
	if (g_tweakYellowDFA.integer)
		trap->SendServerCommand(ent-g_entities, "print \"     ^5JK2 Style yellow DFA\n\"");
	if (g_spinBackslash.integer)
		trap->SendServerCommand(ent-g_entities, "print \"     ^5Spinable backslash\n\"");
	if (g_spinRedDFA.integer)
		trap->SendServerCommand(ent-g_entities, "print \"     ^5Spinable red DFA\n\"");
	if (g_jk2Lunge.integer)
		trap->SendServerCommand(ent-g_entities, "print \"     ^5JK2 style lunge\n\"");
	if (g_maxSaberDefense.integer)
		trap->SendServerCommand(ent-g_entities, va("print \"     ^5Saber defense level capped at^3: ^2%i\n\"", g_maxSaberDefense.integer));

	//Gun changes
	if (g_weaponDisable.integer < (1<<WP_CONCUSSION) || g_startingWeapons.integer > (1<<WP_BRYAR_PISTOL)) { // Weapons are enabled?
		trap->SendServerCommand(ent-g_entities, "print \" ^3Weapon Changes:\n\"");
		trap->SendServerCommand(ent-g_entities, va("print \"     ^5Fast weapon switch: ^2%i\n\"", g_fastWeaponSwitch.integer));
		if (d_projectileGhoul2Collision.integer == 0)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Larger, square hitboxes for projectiles and hitscan\n\"");
		else if (d_projectileGhoul2Collision.integer > 1)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Larger, square hitboxes for projectiles\n\"");
		if (g_selfDamageScale.integer != 0.5)
			trap->SendServerCommand(ent-g_entities, va("print \"     ^5Self damage scale: ^2%.2f\n\"", g_selfDamageScale.value));
		if (g_fullInheritance.integer && g_projectileInheritance.value)
			trap->SendServerCommand(ent-g_entities, va("print \"     ^5Full projectile inheritance: ^2%.2f%\n\"", g_projectileInheritance.value * 100.0f));
		else if (g_projectileInheritance.value)
			trap->SendServerCommand(ent-g_entities, va("print \"     ^5Forwards projectile inheritance: ^2%.2f%\n\"", g_projectileInheritance.value * 100.0f));
		if ((g_unlagged.integer & UNLAGGED_PROJ_NUDGE) && !(g_unlagged.integer & UNLAGGED_HITSCAN)) // proj nudge only
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Lag compensation for projectiles\n\"");
		else if ((g_unlagged.integer & UNLAGGED_HITSCAN) && !(g_unlagged.integer & UNLAGGED_PROJ_NUDGE)) // hitscan only
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Lag compensation for hitscan\n\"");
		else if ((g_unlagged.integer & UNLAGGED_HITSCAN) && (g_unlagged.integer & UNLAGGED_PROJ_NUDGE)) //Proj + hitscan
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Lag compensation for projectiles and hitscan\n\"");
		if (g_projectileVelocityScale.value)
			trap->SendServerCommand(ent-g_entities, va("print \"     ^5Projectile velocity scale: ^2%.2f\n\"", g_projectileVelocityScale.value));
		if (g_weaponDamageScale.value)
			trap->SendServerCommand(ent-g_entities, va("print \"     ^5Weapon damage scale: ^2%.2f\n\"", g_weaponDamageScale.value));
		if (g_tweakWeapons.integer & DEMP2_RANDOM)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Nonrandom Demp2 alt damage\n\"");
		if (g_tweakWeapons.integer & DEMP2_DAM)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Increased Demp2 primary damage\n\"");
		if (g_tweakWeapons.integer & DISRUPTOR_DAM)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Decreased sniper alt max damage\n\"");
		if (g_tweakWeapons.integer & BOWCASTER_SPRD)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Modified bowcaster primary spread\n\"");
		if (g_tweakWeapons.integer & REPEATER_ALT_DAM)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Increased repeater alt damage\n\"");
		if (g_tweakWeapons.integer & FLECHETTE_SPRD)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Nonrandom flechette prim spread\n\"");
		if (g_tweakWeapons.integer & FLECHETTE_ALT_DAM)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Decreased flechette alt damage\n\"");
		if (g_tweakWeapons.integer & FLECHETTE_ALT_SPRD)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Nonrandom flechette alt spread/velocity\n\"");
		if (g_tweakWeapons.integer & CONC_ALT_DAM)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Increased concussion rifle alt damage\n\"");
		if (g_tweakWeapons.integer & PROJECTILE_KNOCKBACK)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Removed knockback from most projectiles\n\"");
		if (g_tweakWeapons.integer & STUN_LG)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Lightning gun stun baton\n\"");
		if (g_tweakWeapons.integer & STUN_SHOCKLANCE)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Shocklance stun baton\n\"");
		if (g_tweakWeapons.integer & PROJECTILE_GRAVITY)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Gravity affected projectiles\n\"");
		if (g_tweakWeapons.integer & CENTER_MUZZLEPOINT)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Allowed center muzzlepoint setting\n\"");
		if (g_tweakWeapons.integer & PSEUDORANDOM_FIRE)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Pseudo random weapon spread\n\"");
		if (g_tweakWeapons.integer & ROCKET_MORTAR)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Rocket launcher is replaced with mortar\n\"");
		else if (g_tweakWeapons.integer & ROCKET_REDEEMER)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Rocket launcher is replaced with redeemer\n\"");
	}

	//CTF changes
	if (g_gametype.integer == GT_CTF || (g_gametype.integer == GT_FFA && g_rabbit.integer)) {// CTF Settings
		trap->SendServerCommand(ent-g_entities, "print \" ^3CTF Changes:\n\"");
		if (g_flagDrag.value)
			trap->SendServerCommand(ent-g_entities, va("print \"     ^5Flag Drag: ^2%.3f\n\"", g_flagDrag.value));
		if (g_fixFlagSuicide.integer)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Flag suicide fix enabled\n\"");
		if (g_allowFlagThrow.integer)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Flag throw enabled\n\"");
		if (g_fixFlagHitbox.integer)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Increased flag hitbox size\n\"");
		if (g_fixCTFScores.integer)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Tweaked CTF scores\n\"");
	}

	//Physics changes
	trap->SendServerCommand(ent-g_entities, "print \" ^3Physics Changes:\n\"");
	if (g_onlyBhop.integer == 1)
		trap->SendServerCommand(ent-g_entities, "print \"     ^5No force jumps forced by server\n\"");
	else if (g_onlyBhop.integer > 1)
		trap->SendServerCommand(ent-g_entities, "print \"     ^5No force jumps option enabled\n\"");
	if (g_tweakJetpack.integer)
		trap->SendServerCommand(ent-g_entities, "print \"     ^5Tweaked jetpack physics\n\"");
	if (g_movementStyle.integer == 3)
		trap->SendServerCommand(ent-g_entities, "print \"     ^5CPM style air control\n\"");
	else if (g_movementStyle.integer == 2)
		trap->SendServerCommand(ent-g_entities, "print \"     ^5QuakeWorld style air control\n\"");
	else if (g_movementStyle.integer == 0)
		trap->SendServerCommand(ent-g_entities, "print \"     ^5Siege style air control\n\"");
	if (g_fixRoll.integer == 1)
		trap->SendServerCommand(ent-g_entities, "print \"     ^5Tweaked roll\n\""); // idk what the fuck this actually does to roll
	else if (g_fixRoll.integer == 2)
		trap->SendServerCommand(ent-g_entities, "print \"     ^5Chainable roll\n\"");
	else if (g_fixRoll.integer > 2)
		trap->SendServerCommand(ent-g_entities, "print \"     ^5JK2 style roll\n\"");
	if (g_fixHighFPSAbuse.integer)
		trap->SendServerCommand(ent-g_entities, "print \"     ^5Fixed physics changed due to high FPS\n\"");
	
	//Force changes
	if (g_forcePowerDisable.integer < (1<<NUM_FORCE_POWERS)) {
		trap->SendServerCommand(ent-g_entities, "print \" ^3Force Changes:\n\"");
		if (g_forceCombo.integer)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Force combo enabled\n\"");
		if (g_flipKick.integer == 1)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5JA+ style flipkick\n\"");
		else if (g_flipKick.integer == 2)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Floodprotected flipkick\n\"");
		else if (g_flipKick.integer > 2)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Flipkick enabled with JK2 style\n\"");
		if (g_fastGrip.integer)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Increased grip runspeed\n\"");
		if (g_fixGetups.integer == 1)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Grip during knockdown recovery\n\"");
		else if (g_fixGetups.integer > 1)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Grip/push/pull during knockdown recovery\n\"");
		if (g_fixLightning.integer == 1)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Lightning gives forcepoints to target\n\"");
		else if (g_fixLightning.integer == 2)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Lightning gives forcepoints to target, and has reduced damage\n\"");
		else if (g_fixLightning.integer > 2)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Lightning gives forcepoints to target, has reduced damage, and no melee bonus\n\"");
		if (g_fixSaberInGrip.integer == 1)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Grip does not turn off targets lightsaber\n\"");
		else if (g_fixSaberInGrip.integer == 2)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Grip does not turn off targets lightsaber, and target can turn on/off lightsaber in grip\n\"");
		else if (g_fixSaberInGrip.integer > 2)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Grip does not turn off targets lightsaber, target can turn on/off lightsaber in grip, and target can switch weapons in grip\n\"");
		if (g_pushPullKnockdown.integer)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Knocked down players are affected by push/pull\n\"");
		if ((!(g_forcePowerDisable.integer & FP_PULL) || !(g_forcePowerDisable.integer & FP_PUSH)) && g_unlagged.integer & UNLAGGED_PUSHPULL)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Lag compensation for force push/pull\n\"");
		if (g_fixGripAbsorb.integer)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5Force absorb does not gain forcepoints from grip\n\"");
		if (g_jk2Grip.integer)
			trap->SendServerCommand(ent-g_entities, "print \"     ^5JK2 1.02 style grip\n\"");
	}
}
//[JAPRO - Serverside - All - Serverconfig - End]

void Cmd_Throwflag_f( gentity_t *ent ) {
	gentity_t	*thrown;
	gitem_t		*item;

	if (g_gametype.integer == GT_CTF) {
	}
	else if ((g_gametype.integer == GT_FFA || g_gametype.integer == GT_TEAM) && g_rabbit.integer) {
	}
	else return;
	
	if (!g_allowFlagThrow.integer)
		return;

	if (ent->client->ps.powerups[PW_REDFLAG])
	{
		item = BG_FindItemForPowerup( PW_REDFLAG );
		thrown = Drop_Flag( ent, item, 0 );
		thrown->count = ( ent->client->ps.powerups[PW_REDFLAG] - level.time ) / 1000;
		thrown->r.contents = CONTENTS_TRIGGER|CONTENTS_CORPSE;
		ent->client->lastThrowTime = level.time;
		if ( thrown->count < 1 ) {
			thrown->count = 1;
		}
		ent->client->ps.powerups[ PW_REDFLAG ] = 0;
		ent->client->lastThrowTime = level.time;
	}
	else if (ent->client->ps.powerups[PW_BLUEFLAG])
	{
		item = BG_FindItemForPowerup( PW_BLUEFLAG );
		thrown = Drop_Flag( ent, item, 0 );
		thrown->count = ( ent->client->ps.powerups[PW_BLUEFLAG] - level.time ) / 1000;
		thrown->r.contents = CONTENTS_TRIGGER|CONTENTS_CORPSE;
		ent->client->lastThrowTime = level.time;
		if ( thrown->count < 1 ) {
			thrown->count = 1;
		}
		ent->client->ps.powerups[ PW_BLUEFLAG ] = 0;
		ent->client->lastThrowTime = level.time;
	}
	else if (ent->client->ps.powerups[PW_NEUTRALFLAG])
	{
		item = BG_FindItemForPowerup( PW_NEUTRALFLAG );
		thrown = Drop_Flag( ent, item, 0 );
		thrown->count = ( ent->client->ps.powerups[PW_NEUTRALFLAG] - level.time ) / 1000;
		thrown->r.contents = CONTENTS_TRIGGER|CONTENTS_CORPSE;
		ent->client->lastThrowTime = level.time;
		if ( thrown->count < 1 ) {
			thrown->count = 1;
		}
		ent->client->ps.powerups[ PW_NEUTRALFLAG ] = 0;
		ent->client->lastThrowTime = level.time;
	}

}

/*
=================
ClientCommand
=================
*/

#define CMD_NOINTERMISSION		(1<<0)
#define CMD_CHEAT				(1<<1)
#define CMD_ALIVE				(1<<2)

typedef struct command_s {
	const char	*name;
	void		(*func)(gentity_t *ent);
	int			flags;
} command_t;

int cmdcmp( const void *a, const void *b ) {
	return Q_stricmp( (const char *)a, ((command_t*)b)->name );
}

/* This array MUST be sorted correctly by alphabetical name field */
command_t commands[] = {
	{ "addbot",				Cmd_AddBot_f,				0 },
	{ "amban",				Cmd_Amban_f,				0 },
	{ "ambeg",				Cmd_EmoteBeg_f,				CMD_NOINTERMISSION|CMD_ALIVE },//EMOTE
	{ "ambeg2",				Cmd_EmoteBeg2_f,			CMD_NOINTERMISSION|CMD_ALIVE },//EMOTE
	{ "ambernie",			Cmd_EmoteBernie_f,			CMD_NOINTERMISSION|CMD_ALIVE },//EMOTE
	{ "ambreakdance",		Cmd_EmoteBreakdance_f,		CMD_NOINTERMISSION|CMD_ALIVE },//EMOTE
	{ "ambreakdance2",		Cmd_EmoteBreakdance2_f,		CMD_NOINTERMISSION|CMD_ALIVE },//EMOTE
	{ "ambreakdance3",		Cmd_EmoteBreakdance3_f,		CMD_NOINTERMISSION|CMD_ALIVE },//EMOTE
	{ "ambreakdance4",		Cmd_EmoteBreakdance4_f,		CMD_NOINTERMISSION|CMD_ALIVE },//EMOTE
	{ "amcheer",			Cmd_EmoteCheer_f,			CMD_NOINTERMISSION|CMD_ALIVE },//EMOTE
	{ "amcower",			Cmd_EmoteCower_f,			CMD_NOINTERMISSION|CMD_ALIVE },//EMOTE
	{ "amdance",			Cmd_EmoteDance_f,			CMD_NOINTERMISSION|CMD_ALIVE },//EMOTE
	{ "amforceteam",		Cmd_Amforceteam_f,			CMD_NOINTERMISSION },
	{ "amfreeze",			Cmd_Amfreeze_f,				CMD_NOINTERMISSION },
	{ "amgrantadmin",		Cmd_Amgrantadmin_f,			0 },
	{ "amhug",				Cmd_EmoteHug_f,				CMD_NOINTERMISSION|CMD_ALIVE },//EMOTE
	{ "aminfo",				Cmd_Aminfo_f,				0 },
	{ "amkick",				Cmd_Amkick_f,				0 },
	{ "amlockteam",			Cmd_Amlockteam_f,			CMD_NOINTERMISSION },
	{ "amlogin",			Cmd_Amlogin_f,				0 },
	{ "amlogout",			Cmd_Amlogout_f,				0 },
	{ "ammap",				Cmd_Ammap_f,				CMD_NOINTERMISSION },
	{ "ammotd",				Cmd_Showmotd_f,				CMD_NOINTERMISSION },
	{ "amnoisy",			Cmd_EmoteNoisy_f,			CMD_NOINTERMISSION|CMD_ALIVE },//EMOTE
	{ "ampoint",			Cmd_EmotePoint_f,			CMD_NOINTERMISSION|CMD_ALIVE },//EMOTE
	{ "ampsay",				Cmd_Ampsay_f,				CMD_NOINTERMISSION },
	{ "amrage",				Cmd_EmoteRage_f,			CMD_NOINTERMISSION|CMD_ALIVE },//EMOTE
	{ "amrename",			Cmd_Amrename_f,				CMD_NOINTERMISSION },
	{ "amrun",				Cmd_AmRun_f,				CMD_NOINTERMISSION},//EMOTE
	{ "amsay",				Cmd_Amsay_f,				0 },
	{ "amsit",				Cmd_EmoteSit_f,				CMD_NOINTERMISSION|CMD_ALIVE },//EMOTE
	{ "amsit2",				Cmd_EmoteSit2_f,			CMD_NOINTERMISSION|CMD_ALIVE },//EMOTE
	{ "amsit3",				Cmd_EmoteSit3_f,			CMD_NOINTERMISSION|CMD_ALIVE },//EMOTE
	{ "amsit4",				Cmd_EmoteSit4_f,			CMD_NOINTERMISSION|CMD_ALIVE },//EMOTE
	{ "amsit5",				Cmd_EmoteSit5_f,			CMD_NOINTERMISSION|CMD_ALIVE },//EMOTE
	{ "amsmack",			Cmd_EmoteSmack_f,			CMD_NOINTERMISSION|CMD_ALIVE },//EMOTE
	{ "amstatus",			Cmd_Amstatus_f,				0 },
	{ "amsurrender",		Cmd_EmoteSurrender_f,		CMD_NOINTERMISSION|CMD_ALIVE },//EMOTE
	{ "amtaunt",			Cmd_EmoteTaunt_f,			CMD_NOINTERMISSION|CMD_ALIVE },//EMOTE
	{ "amtaunt2",			Cmd_EmoteTaunt2_f,			CMD_NOINTERMISSION|CMD_ALIVE },//EMOTE
	{ "amtele",				Cmd_Amtele_f,				CMD_NOINTERMISSION },
	{ "amtelemark",			Cmd_Amtelemark_f,			CMD_NOINTERMISSION },
	{ "amvictory",			Cmd_EmoteVictory_f,			CMD_NOINTERMISSION|CMD_ALIVE },//EMOTE
	{ "amvstr",				Cmd_Amvstr_f,				CMD_NOINTERMISSION },
	{ "callteamvote",		Cmd_CallTeamVote_f,			CMD_NOINTERMISSION },
	{ "callvote",			Cmd_CallVote_f,				CMD_NOINTERMISSION },
	{ "clanpass",			Cmd_Clanpass_f,				CMD_NOINTERMISSION },
	{ "clansay",			Cmd_Clansay_f,				0 },
	{ "clanwhois",			Cmd_Clanwhois_f,			0 },
	{ "debugBMove_Back",	Cmd_BotMoveBack_f,			CMD_CHEAT|CMD_ALIVE },
	{ "debugBMove_Forward",	Cmd_BotMoveForward_f,		CMD_CHEAT|CMD_ALIVE },
	{ "debugBMove_Left",	Cmd_BotMoveLeft_f,			CMD_CHEAT|CMD_ALIVE },
	{ "debugBMove_Right",	Cmd_BotMoveRight_f,			CMD_CHEAT|CMD_ALIVE },
	{ "debugBMove_Up",		Cmd_BotMoveUp_f,			CMD_CHEAT|CMD_ALIVE },
	//{ "debugsetbodyanim",	Cmd_DebugSetBodyAnim_f,		CMD_CHEAT|CMD_ALIVE },
	{ "duelteam",			Cmd_DuelTeam_f,				CMD_NOINTERMISSION },
	{ "engage_fullforceduel",	Cmd_ForceDuel_f,		CMD_NOINTERMISSION },//JAPRO - Serverside - Fullforce Duels
	{ "engage_gunduel",		Cmd_GunDuel_f,				CMD_NOINTERMISSION },//JAPRO - Serverside - Fullforce Duels
	{ "follow",				Cmd_Follow_f,				CMD_NOINTERMISSION },
	{ "follownext",			Cmd_FollowNext_f,			CMD_NOINTERMISSION },
	{ "followprev",			Cmd_FollowPrev_f,			CMD_NOINTERMISSION },
	{ "forcechanged",		Cmd_ForceChanged_f,			0 },
	{ "gc",					Cmd_GameCommand_f,			CMD_NOINTERMISSION },
	{ "give",				Cmd_Give_f,					CMD_CHEAT|CMD_ALIVE|CMD_NOINTERMISSION },
	{ "giveother",			Cmd_GiveOther_f,			CMD_CHEAT|CMD_ALIVE|CMD_NOINTERMISSION },
	{ "god",				Cmd_God_f,					CMD_CHEAT|CMD_ALIVE|CMD_NOINTERMISSION },
	{ "ignore",				Cmd_Ignore_f,				0 },//[JAPRO - Serverside - All - Ignore]
	{ "jetpack",			Cmd_Jetpack_f,				CMD_NOINTERMISSION|CMD_ALIVE },
	{ "kill",				Cmd_Kill_f,					CMD_ALIVE|CMD_NOINTERMISSION },
	{ "killother",			Cmd_KillOther_f,			CMD_CHEAT|CMD_ALIVE },
//	{ "kylesmash",			TryGrapple,					0 },
	{ "levelshot",			Cmd_LevelShot_f,			CMD_CHEAT|CMD_ALIVE|CMD_NOINTERMISSION },
	{ "movementstyle",		Cmd_MovementStyle_f,		CMD_NOINTERMISSION},//EMOTE
	{ "noclip",				Cmd_Noclip_f,				CMD_ALIVE|CMD_NOINTERMISSION },//change for admin?
	{ "notarget",			Cmd_Notarget_f,				CMD_CHEAT|CMD_ALIVE|CMD_NOINTERMISSION },
	{ "npc",				Cmd_NPC_f,					CMD_ALIVE },//removed cheat for admin
	{ "race",				Cmd_Race_f,					CMD_NOINTERMISSION },
	{ "saber",				Cmd_Saber_f,				CMD_NOINTERMISSION },
	{ "say",				Cmd_Say_f,					0 },
	{ "say_team",			Cmd_SayTeam_f,				0 },
	{ "say_team_mod",		Cmd_SayTeamMod_f,			0 },
	{ "score",				Cmd_Score_f,				0 },
	{ "serverconfig",		Cmd_ServerConfig_f,			0 },
	{ "setviewpos",			Cmd_SetViewpos_f,			CMD_NOINTERMISSION },
	{ "siegeclass",			Cmd_SiegeClass_f,			CMD_NOINTERMISSION },
	{ "team",				Cmd_Team_f,					CMD_NOINTERMISSION },
//	{ "teamtask",			Cmd_TeamTask_f,				CMD_NOINTERMISSION },
	{ "teamvote",			Cmd_TeamVote_f,				CMD_NOINTERMISSION },
	{ "tell",				Cmd_Tell_f,					0 },
	{ "thedestroyer",		Cmd_TheDestroyer_f,			CMD_CHEAT|CMD_ALIVE },
	{ "throwflag",			Cmd_Throwflag_f,			CMD_ALIVE|CMD_NOINTERMISSION },
	{ "t_use",				Cmd_TargetUse_f,			CMD_CHEAT|CMD_ALIVE },
	{ "voice_cmd",			Cmd_VoiceCommand_f,			0 },
	{ "vote",				Cmd_Vote_f,					CMD_NOINTERMISSION },
	{ "where",				Cmd_Where_f,				CMD_NOINTERMISSION },
};
static size_t numCommands = ARRAY_LEN( commands );

void ClientCommand( int clientNum ) {
	gentity_t	*ent = NULL;
	char		cmd[MAX_TOKEN_CHARS] = {0};
	command_t	*command = NULL;

	ent = g_entities + clientNum;
	if ( !ent->client || ent->client->pers.connected != CON_CONNECTED ) {
		G_SecurityLogPrintf( "ClientCommand(%d) without an active connection\n", clientNum );
		return;		// not fully in game yet
	}

	trap->Argv( 0, cmd, sizeof( cmd ) );

	//rww - redirect bot commands
	if ( strstr( cmd, "bot_" ) && AcceptBotCommand( cmd, ent ) )
		return;
	//end rww

	command = (command_t *)bsearch( cmd, commands, numCommands, sizeof( commands[0] ), cmdcmp );
	if ( !command )
	{
		trap->SendServerCommand( clientNum, va( "print \"Unknown command %s\n\"", cmd ) );
		return;
	}

	else if ( (command->flags & CMD_NOINTERMISSION)
		&& ( level.intermissionQueued || level.intermissiontime ) )
	{
		trap->SendServerCommand( clientNum, va( "print \"%s (%s)\n\"", G_GetStringEdString( "MP_SVGAME", "CANNOT_TASK_INTERMISSION" ), cmd ) );
		return;
	}

	else if ( (command->flags & CMD_CHEAT)
		&& !sv_cheats.integer )
	{
		trap->SendServerCommand( clientNum, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "NOCHEATS" ) ) );
		return;
	}

	else if ( (command->flags & CMD_ALIVE)
		&& (ent->health <= 0
			|| ent->client->tempSpectate >= level.time
			|| ent->client->sess.sessionTeam == TEAM_SPECTATOR) )
	{
		trap->SendServerCommand( clientNum, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "MUSTBEALIVE" ) ) );
		return;
	}

	else
		command->func( ent );
}
