// Copyright (C) 1999-2000 Id Software, Inc.
//

// this file holds commands that can be executed by the server console, but not remote clients

#include "g_local.h"

/*
==============================================================================

PACKET FILTERING
 

You can add or remove addresses from the filter list with:

addip <ip>
removeip <ip>

The ip address is specified in dot format, and any unspecified digits will match any value, so you can specify an entire class C network with "addip 192.246.40".

Removeip will only remove an address specified exactly the same way.  You cannot addip a subnet, then removeip a single host.

listip
Prints the current list of filters.

g_filterban <0 or 1>

If 1 (the default), then ip addresses matching the current list will be prohibited from entering the game.  This is the default setting.

If 0, then only addresses matching the list will be allowed.  This lets you easily set up a private game, or a game that only allows players from your local network.


==============================================================================
*/

typedef struct ipFilter_s {
	uint32_t mask, compare;
} ipFilter_t;

#define	MAX_IPFILTERS (1024)

static ipFilter_t	ipFilters[MAX_IPFILTERS];
static int			numIPFilters;

/*
=================
StringToFilter
=================
*/
static qboolean StringToFilter( char *s, ipFilter_t *f ) {
	char num[128];
	int i, j;
	byteAlias_t b, m;

	b.ui = m.ui = 0u;

	for ( i=0; i<4; i++ ) {
		if ( *s < '0' || *s > '9' ) {
			if ( *s == '*' ) {
				// 'match any'
				// b[i] and m[i] to 0
				s++;
				if ( !*s )
					break;
				s++;
				continue;
			}
			trap->Print( "Bad filter address: %s\n", s );
			return qfalse;
		}

		j = 0;
		while ( *s >= '0' && *s <= '9' )
			num[j++] = *s++;

		num[j] = 0;
		b.b[i] = (byte)atoi( num );
		m.b[i] = 0xFF;

		if ( !*s )
			break;

		s++;
	}

	f->mask = m.ui;
	f->compare = b.ui;

	return qtrue;
}

/*
=================
UpdateIPBans
=================
*/
static void UpdateIPBans( void ) {
	byteAlias_t b, m;
	int i, j;
	char ip[NET_ADDRSTRMAXLEN], iplist_final[MAX_CVAR_VALUE_STRING];

	*iplist_final = 0;
	for ( i=0; i<numIPFilters; i++ ) {
		if ( ipFilters[i].compare == 0xFFFFFFFFu )
			continue;

		b.ui = ipFilters[i].compare;
		m.ui = ipFilters[i].mask;
		*ip = 0;
		for ( j=0; j<4; j++ ) {
			if ( m.b[j] != 0xFF )
				Q_strcat( ip, sizeof( ip ), "*" );
			else
				Q_strcat( ip, sizeof( ip ), va( "%i", b.b[j] ) );
			Q_strcat( ip, sizeof( ip ), (j<3) ? "." : " " );
		}
		if ( strlen( iplist_final )+strlen( ip ) < MAX_CVAR_VALUE_STRING )
			Q_strcat( iplist_final, sizeof( iplist_final ), ip );
		else {
			Com_Printf( "g_banIPs overflowed at MAX_CVAR_VALUE_STRING\n" );
			break;
		}
	}

	trap->Cvar_Set( "g_banIPs", iplist_final );
}


/*
=================
G_FilterPacket
=================
*/
qboolean G_FilterPacket( char *from ) {
	int i;
	uint32_t in;
	byteAlias_t m;
	char *p;

	i = 0;
	p = from;
	while ( *p && i < 4 ) {
		m.b[i] = 0;
		while ( *p >= '0' && *p <= '9' ) {
			m.b[i] = m.b[i]*10 + (*p - '0');
			p++;
		}
		if ( !*p || *p == ':' )
			break;
		i++, p++;
	}
	
	in = m.ui;

	for ( i=0; i<numIPFilters; i++ ) {
		if ( (in & ipFilters[i].mask) == ipFilters[i].compare )
			return g_filterBan.integer != 0;
	}

	return g_filterBan.integer == 0;
}

/*
=================
AddIP
=================
*/
void AddIP( char *str ) {
	int i;

	for ( i=0; i<numIPFilters; i++ ) {
		if ( ipFilters[i].compare == 0xFFFFFFFFu )
			break; // free spot
	}
	if ( i == numIPFilters ) {
		if ( numIPFilters == MAX_IPFILTERS ) {
			trap->Print( "IP filter list is full\n" );
			return;
		}
		numIPFilters++;
	}
	
	if ( !StringToFilter( str, &ipFilters[i] ) )
		ipFilters[i].compare = 0xFFFFFFFFu;

	UpdateIPBans();
}

/*
=================
G_ProcessIPBans
=================
*/
void G_ProcessIPBans( void ) {
	char *s = NULL, *t = NULL, str[MAX_CVAR_VALUE_STRING] = {0};

	Q_strncpyz( str, g_banIPs.string, sizeof( str ) );

	for ( t=s=g_banIPs.string; *t; t=s ) {
		s = strchr( s, ' ' );
		if ( !s )
			break;

		while ( *s == ' ' )
			*s++ = 0;

		if ( *t )
			AddIP( t );
	}
}

/*
=================
Svcmd_AddIP_f
=================
*/
void Svcmd_AddIP_f (void)
{
	char		str[MAX_TOKEN_CHARS];

	if ( trap->Argc() < 2 ) {
		trap->Print("Usage: addip <ip-mask>\n");
		return;
	}

	trap->Argv( 1, str, sizeof( str ) );

	AddIP( str );
}

/*
=================
Svcmd_RemoveIP_f
=================
*/
void Svcmd_RemoveIP_f (void)
{
	ipFilter_t	f;
	int			i;
	char		str[MAX_TOKEN_CHARS];

	if ( trap->Argc() < 2 ) {
		trap->Print("Usage: removeip <ip-mask>\n");
		return;
	}

	trap->Argv( 1, str, sizeof( str ) );

	if (!StringToFilter (str, &f))
		return;

	for (i=0 ; i<numIPFilters ; i++) {
		if (ipFilters[i].mask == f.mask	&&
			ipFilters[i].compare == f.compare) {
			ipFilters[i].compare = 0xffffffffu;
			trap->Print ("Removed.\n");

			UpdateIPBans();
			return;
		}
	}

	trap->Print ( "Didn't find %s.\n", str );
}

void Svcmd_ListIP_f (void)
{
	int		i, count = 0;
	byteAlias_t b;

	for(i = 0; i < numIPFilters; i++) {
		if ( ipFilters[i].compare == 0xffffffffu )
			continue;

		b.ui = ipFilters[i].compare;
		trap->Print ("%i.%i.%i.%i\n", b.b[0], b.b[1], b.b[2], b.b[3]);
		count++;
	}
	trap->Print ("%i bans.\n", count);
}

/*
===================
Svcmd_EntityInfo_f
===================
*/
void	Svcmd_EntityInfo_f(void) {
	int totalents;
	int inuse;
	int i;
	gentity_t *e;

	inuse = 0;
	for (e = &g_entities[0], i = 0; i < level.num_entities; e++, i++) {
		if (e->inuse) {
			inuse++;
		}
	}
	trap->Print("Normal entity slots in use: %i/%i (%i slots allocated)\n", inuse, MAX_GENTITIES, level.num_entities);
	totalents = inuse;

	inuse = 0;
	for (e = &g_entities[MAX_GENTITIES], i = 0; i < level.num_logicalents; e++, i++) {
		if (e->inuse) {
			inuse++;
		}
	}
	trap->Print("Logical entity slots in use: %i/%i (%i slots allocated)\n", inuse, MAX_LOGICENTITIES, level.num_logicalents);
	totalents += inuse;
	trap->Print("Total entity count: %i/%i\n", totalents, MAX_ENTITIESTOTAL);
}


/*
===================
Svcmd_EntityList_f
===================
*/
void	Svcmd_EntityList_f (void) {
	int			e;
	gentity_t		*check;

	check = g_entities;
	for (e = 0; e < level.num_entities ; e++, check++) {
		if ( !check->inuse ) {
			continue;
		}
		trap->Print("%3i:", e);
		switch ( check->s.eType ) {
		case ET_GENERAL:
			trap->Print("ET_GENERAL          ");
			break;
		case ET_PLAYER:
			trap->Print("ET_PLAYER           ");
			break;
		case ET_ITEM:
			trap->Print("ET_ITEM             ");
			break;
		case ET_MISSILE:
			trap->Print("ET_MISSILE          ");
			break;
		case ET_SPECIAL:
			trap->Print("ET_SPECIAL          ");
			break;
		case ET_HOLOCRON:
			trap->Print("ET_HOLOCRON         ");
			break;
		case ET_MOVER:
			trap->Print("ET_MOVER            ");
			break;
		case ET_BEAM:
			trap->Print("ET_BEAM             ");
			break;
		case ET_PORTAL:
			trap->Print("ET_PORTAL           ");
			break;
		case ET_SPEAKER:
			trap->Print("ET_SPEAKER          ");
			break;
		case ET_PUSH_TRIGGER:
			trap->Print("ET_PUSH_TRIGGER     ");
			break;
		case ET_TELEPORT_TRIGGER:
			trap->Print("ET_TELEPORT_TRIGGER ");
			break;
		case ET_INVISIBLE:
			trap->Print("ET_INVISIBLE        ");
			break;
		case ET_NPC:
			trap->Print("ET_NPC              ");
			break;
		case ET_BODY:
			trap->Print("ET_BODY             ");
			break;
		case ET_TERRAIN:
			trap->Print("ET_TERRAIN          ");
			break;
		case ET_FX:
			trap->Print("ET_FX               ");
			break;
		default:
			trap->Print("%-3i                ", check->s.eType);
			break;
		}

		if ( check->classname ) {
			trap->Print("%s", check->classname);
		}
		trap->Print("\n");
	}
}

qboolean StringIsInteger( const char *s );
/*
===================
ClientForString
===================
*/
gclient_t	*ClientForString( const char *s ) {
	gclient_t	*cl;
	int			idnum;
	char		cleanInput[MAX_STRING_CHARS];

	// numeric values could be slot numbers
	if ( StringIsInteger( s ) ) {
		idnum = atoi( s );
		if ( idnum >= 0 && idnum < level.maxclients ) {
			cl = &level.clients[idnum];
			if ( cl->pers.connected == CON_CONNECTED ) {
				return cl;
			}
		}
	}

	Q_strncpyz( cleanInput, s, sizeof(cleanInput) );
	Q_StripColor( cleanInput );

	// check for a name match
	for ( idnum=0,cl=level.clients ; idnum < level.maxclients ; idnum++,cl++ ) {
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( !Q_stricmp( cl->pers.netname_nocolor, cleanInput ) ) {
			return cl;
		}
	}

	trap->Print( "User %s is not on the server\n", s );
	return NULL;
}


void SanitizeString2(const char *in, char *out);
static int SV_ClientNumberFromString(const char *s) 
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
			trap->Print( "Client '%i' is not active\n", idnum);
			return -1;
		}
		return idnum;
	}

	else if (s[0] == '1' && s[0] == '2' && (s[1] >= '0' && s[1] <= '9' && strlen(s) == 2)) 
	{
		idnum = atoi( s );
		cl = &level.clients[idnum];
		if ( cl->pers.connected != CON_CONNECTED ) {
			trap->Print( "Client '%i' is not active\n", idnum);
			return -1;
		}
		return idnum;
	}

	else if (s[0] == '3' && (s[1] >= '0' && s[1] <= '1' && strlen(s) == 2)) 
	{
		idnum = atoi( s );
		cl = &level.clients[idnum];
		if ( cl->pers.connected != CON_CONNECTED ) {
			trap->Print( "Client '%i' is not active\n", idnum);
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
					trap->Print( "More than one user '%s' on the server\n", s);
					return -2;
				}
				match = level.sortedClients[i];
			}
		}
		if (match != -1)//uhh
			return match;
	}
	trap->Print( "User '%s' is not on the server\n", s);
	return -1;
}

/*
===================
Svcmd_ForceTeam_f

forceteam <player> <team>
===================
*/
void Svcmd_ForceTeam_f( void ) {
	gclient_t	*cl;
	char		str[MAX_TOKEN_CHARS];

	if ( trap->Argc() < 3 ) {
		trap->Print("Usage: forceteam <player> <team>\n");
		return;
	}

	// find the player
	trap->Argv( 1, str, sizeof( str ) );
	cl = ClientForString( str );
	if ( !cl ) {
		return;
	}

	// set the team
	trap->Argv( 2, str, sizeof( str ) );
	SetTeam( &g_entities[cl - level.clients], str , qtrue); //can just be cl?

	cl->switchTeamTime = level.time + 5000; //might need to be way more
}

void Svcmd_ResetScores_f (void) {
	int i;
	//gclient_t	*cl;
	gentity_t *ent;
	
	//Respawn each player for forcepower updates?
	//bg_legalizeforcepowers

	for (i=0 ; i < level.numConnectedClients ; i++) {
		//cl=&level.clients[level.sortedClients[i]];
		ent = &g_entities[level.sortedClients[i]];

		if (ent->inuse && ent->client) {
			//ent->client->ps.fd.forceDoInit = 1;

			//if (ent->client->sess.sessionTeam != TEAM_SPECTATOR && !ent->client->sess.raceMode) {
				//G_Kill( ent ); //respawn them
			//}

			ent->client->ps.persistant[PERS_SCORE] = 0;
			ent->client->ps.persistant[PERS_HITS] = 0;
			ent->client->ps.persistant[PERS_KILLED] = 0;
			ent->client->ps.persistant[PERS_IMPRESSIVE_COUNT] = 0;
			ent->client->ps.persistant[PERS_EXCELLENT_COUNT] = 0;
			ent->client->ps.persistant[PERS_DEFEND_COUNT] = 0;
			ent->client->ps.persistant[PERS_ASSIST_COUNT] = 0;
			ent->client->ps.persistant[PERS_GAUNTLET_FRAG_COUNT] = 0;
			ent->client->ps.persistant[PERS_CAPTURES] = 0;

			ent->client->pers.stats.damageGiven = 0;
			ent->client->pers.stats.damageTaken = 0;
			ent->client->pers.stats.teamKills = 0;
			ent->client->pers.stats.kills = 0;
			ent->client->pers.stats.teamHealGiven = 0;
			ent->client->pers.stats.teamEnergizeGiven = 0;
			ent->client->pers.stats.enemyDrainDamage = 0;
			ent->client->pers.stats.teamDrainDamage = 0;
			//Cmd_ForceChange_f(ent);
			//WP_InitForcePowers( ent );
		}
	}

	level.teamScores[TEAM_RED] = 0;
	level.teamScores[TEAM_BLUE] = 0;
	CalculateRanks();
	trap->SendServerCommand( -1, "print \"Scores have been reset.\n\"" );
}

static void RemoveCTFFlags(void) {
	int i;
	gclient_t	*cl;
	gentity_t	*ent;

	Team_ReturnFlag(TEAM_RED);
	Team_ReturnFlag(TEAM_BLUE);

	for (i=0;  i<level.numPlayingClients; i++) {
		cl = &level.clients[level.sortedClients[i]];

		cl->ps.powerups[PW_REDFLAG] = 0;
		cl->ps.powerups[PW_BLUEFLAG] = 0;
	}

	for (i = 0; i < level.num_entities; i++) {
		ent = &g_entities[i];
		if (ent->inuse && (ent->s.eType == ET_ITEM) && ((ent->item->giTag == PW_REDFLAG) || (ent->item->giTag == PW_BLUEFLAG)) && (ent->item->giType = IT_TEAM)) {
			G_FreeEntity( ent );
			//return;
		}
	}
}

void SetGametypeFuncSolids( void );
void G_CacheGametype( void );
qboolean G_CallSpawn( gentity_t *ent );
void Svcmd_ChangeGametype_f (void) {
	char	input[16];
	int		gametype, i;
	gclient_t	*cl;

	if ( trap->Argc() != 2 ) {
		trap->Print("Usage: gametype <#>\n");
		return;
	}

	trap->Argv( 1, input, sizeof( input ) );
	gametype = atoi(input);

	if (gametype < GT_FFA || gametype >= GT_MAX_GAME_TYPE)
		return;
	if (gametype == GT_SIEGE || gametype == GT_SINGLE_PLAYER) //Also dont even bother with siege, idk
		return;

	//Check if gametype was actually changed?
	if (gametype != level.gametype) 
		Svcmd_ResetScores_f();

	trap->Cvar_Register(&g_gametype, "g_gametype", "0", CVAR_SERVERINFO);
	trap->Cvar_Set( "g_gametype", va( "%i", gametype ) );
	trap->Cvar_Update( &g_gametype );

	G_CacheGametype();

	if (level.gametype == GT_CTF || level.gametype == GT_CTY) { //
		gentity_t	*ent;

		RemoveCTFFlags(); //Delete the current flag first

		if (level.redFlag) {
			ent = G_Spawn(qtrue);
			ent->classname = "team_CTF_redflag";
			VectorCopy(level.redFlagOrigin, ent->s.origin);
			if (!G_CallSpawn(ent))
				G_FreeEntity( ent );
		}
		if (level.blueFlag) {
			ent = G_Spawn(qtrue);
			ent->classname = "team_CTF_blueflag";
			VectorCopy(level.blueFlagOrigin, ent->s.origin);
			if (!G_CallSpawn(ent))
				G_FreeEntity( ent );
		}
	}
	else {
		RemoveCTFFlags();
	}

	for (i=0;  i<level.numPlayingClients; i++) { //Move people to the proper team if changing from team to non team gametype etc
		cl = &level.clients[level.sortedClients[i]];

		if (level.gametype < GT_TEAM && (cl->sess.sessionTeam == TEAM_RED || cl->sess.sessionTeam == TEAM_BLUE)) {
			cl->sess.sessionTeam = TEAM_FREE;
		}
		else if (level.gametype >= GT_TEAM && !g_raceMode.integer && cl->sess.sessionTeam == TEAM_FREE) {
			cl->sess.sessionTeam = TEAM_SPECTATOR;
		}
	}

	SetGametypeFuncSolids();

	//Spawn / clear ctf flags?  
	//who knows what needs to be done for siege.. forget it.

}

void Svcmd_AmKick_f(void) {
		int clientid = -1; 
		char   arg[MAX_NETNAME]; 

		if (trap->Argc() != 2) {
			trap->Print( "Usage: /amKick <client>.\n");
            return; 
        } 
		trap->Argv(1, arg, sizeof(arg)); 
        clientid = SV_ClientNumberFromString(arg);

        if (clientid == -1 || clientid == -2)  
			return; 
		trap->SendConsoleCommand( EXEC_APPEND, va("clientkick %i", clientid) );

}

void Svcmd_AmBan_f(void) {
		int clientid = -1; 
		char   arg[MAX_NETNAME]; 

		if (trap->Argc() != 2) {
			trap->Print( "Usage: /amBan <client>.\n");
            return; 
        } 
		trap->Argv(1, arg, sizeof(arg)); 
        clientid = SV_ClientNumberFromString(arg);

        if (clientid == -1 || clientid == -2)  
			return; 
		AddIP(g_entities[clientid].client->sess.IP);
		trap->SendConsoleCommand( EXEC_APPEND, va("clientkick %i", clientid) );
}

void Svcmd_Amgrantadmin_f(void)
{
		char arg[MAX_NETNAME];
		int clientid = -1; 

		if (trap->Argc() != 3) {
			trap->Print( "Usage: /amGrantAdmin <client> <level>.\n");
			return; 
		}

		trap->Argv(1, arg, sizeof(arg)); 
		clientid = SV_ClientNumberFromString(arg);

		if (clientid == -1 || clientid == -2)  
			return;  

		if (!g_entities[clientid].client)
			return;

		trap->Argv(2, arg, sizeof(arg)); 
		Q_strlwr(arg);

		if (!Q_stricmp(arg, "none")) {
			g_entities[clientid].client->sess.juniorAdmin = qfalse;
			g_entities[clientid].client->sess.fullAdmin = qfalse;
		}
		else if (!Q_stricmp(arg, "junior")) {
			g_entities[clientid].client->sess.juniorAdmin = qtrue;
			g_entities[clientid].client->sess.fullAdmin = qfalse;
			trap->SendServerCommand( clientid, "print \"You have been granted Junior admin privileges.\n\"" );
		}
		else if (!Q_stricmp(arg, "full")) {
			g_entities[clientid].client->sess.juniorAdmin = qfalse;
			g_entities[clientid].client->sess.fullAdmin = qtrue;
			trap->SendServerCommand( clientid, "print \"You have been granted Full admin privileges.\n\"" );
		}
}

static void SV_Pause_f( void ) {
	//OSP: pause
	if ( level.pause.state == PAUSE_NONE ) {
		level.pause.state = PAUSE_PAUSED;
		level.pause.time = level.time + g_pauseTime.integer * 1000;
	}
	else if ( level.pause.state == PAUSE_PAUSED ) {
		level.pause.state = PAUSE_UNPAUSING;
		level.pause.time = level.time + g_unpauseTime.integer * 1000;
	}
}

char *ConcatArgs( int start );
void Svcmd_Say_f( void ) {
	char *p = NULL;
	// don't let text be too long for malicious reasons
	char text[MAX_SAY_TEXT] = {0};

	if ( trap->Argc () < 2 )
		return;

	p = ConcatArgs( 1 );

	if ( strlen( p ) >= MAX_SAY_TEXT ) {
		p[MAX_SAY_TEXT-1] = '\0';
		G_SecurityLogPrintf( "Cmd_Say_f from -1 (server) has been truncated: %s\n", p );
	}

	Q_strncpyz( text, p, sizeof(text) );
	Q_strstrip( text, "\n\r", "  " );

	G_LogPrintf( "say: server: %s\n", text );
	trap->SendServerCommand( -1, va("print \"server: %s\n\"", text ) );
}

typedef struct bitInfo_S {
	const char	*string;
} bitInfo_T;

static bitInfo_T weaponTweaks[] = { // MAX_WEAPON_TWEAKS tweaks (24)
	{"Nonrandom DEMP2"},//1
	{"Increased DEMP2 primary damage"},//2
	{"Decreased disruptor alt damage"},//3
	{"Nonrandom bowcaster spread"},//4
	{"Increased repeater alt damage"},//5
	{"Nonrandom flechette primary spread"},//6
	{"Decreased flechette alt damage"},//7
	{"Nonrandom flechette alt spread"},//8
	{"Increased concussion rifle alt damage"},//9
	{"Removed projectile knockback"},//10
	{"Stun baton lightning gun"},//11
	{"Stun baton shocklance"},//12
	{"Projectile gravity"},//13
	{"Allow center muzzle"},//14
	{"Pseudo random weapon spread"},//15
	{"Rocket alt fire mortar"},//16
	{"Rocket alt fire redeemer"},//17
	{"Infinite ammo"},//18
	{"Stun baton heal gun"},//19
	{"Weapons can damage vehicles"},//20
	{"Allow gunroll"},//21
	{"Fast weaponswitch"},//22
	{"Impact nitrons"},//23
	{"Flechette stake gun"},//24
	{"Fix dropped mine ammo count"},//25
	{"JK2 Style Alt Tripmine"},//26
	{"Projectile Sniper"},//27
	{"No Spread"},//28
	{"Slow sniper fire rate"},//29
	{"Make rockets solid for their owners"}//29
};
static const int MAX_WEAPON_TWEAKS = ARRAY_LEN( weaponTweaks );

void CVU_TweakWeapons (void);
void Svcmd_ToggleTweakWeapons_f( void ) {
	if ( trap->Argc() == 1 ) {
		int i = 0;
		for ( i = 0; i < MAX_WEAPON_TWEAKS; i++ ) {
			if ( (g_tweakWeapons.integer & (1 << i)) ) {
				trap->Print( "%2d [X] %s\n", i, weaponTweaks[i].string );
			}
			else {
				trap->Print( "%2d [ ] %s\n", i, weaponTweaks[i].string );
			}
		}
		return;
	}
	else {
		char arg[8] = { 0 };
		int index;
		const uint32_t mask = (1 << MAX_WEAPON_TWEAKS) - 1;

		trap->Argv( 1, arg, sizeof(arg) );
		index = atoi( arg );

		if ( index < 0 || index >= MAX_WEAPON_TWEAKS ) {
			trap->Print( "tweakWeapons: Invalid range: %i [0, %i]\n", index, MAX_WEAPON_TWEAKS - 1 );
			return;
		}

		trap->Cvar_Set( "g_tweakWeapons", va( "%i", (1 << index) ^ (g_tweakWeapons.integer & mask ) ) );
		trap->Cvar_Update( &g_tweakWeapons );

		trap->Print( "%s %s^7\n", weaponTweaks[index].string, ((g_tweakWeapons.integer & (1 << index))
			? "^2Enabled" : "^1Disabled") );

		CVU_TweakWeapons();
	}
}

static bitInfo_T saberTweaks[] = { 
	{"Skip saber interpolate for MP dmgs"},//1
	{"JK2 1.02 Style Damage System"},//2
	{"Reduced saberblock for MP damages"},//3
	{"Reduce saberdrops for MP damages"},//4
	{"Allow rollcancel for saber swings"},//5
	{"Remove chainable swings from red stance"},//6
	{"Fixed saberswitch"},//7
	{"No aim backslash"},//8
	{"JK2 red DFA"},//9
	{"Fix yellow DFA"},//10
	{"Spin red DFA"},//11
	{"Spin backslash"},//12
	{"JK2 Lunge"},//13
	{"Remove red DFA Boost"},//14
	{"Make red DFA cost 0 forcepoints"},//15
	{"Remove all backslash restrictions"},//16
	{"Allow Sabergun"},//17
	{"Allow fast style change for single saber"}//17
};
static const int MAX_SABER_TWEAKS = ARRAY_LEN( saberTweaks );

void CVU_TweakSaber (void);
void Svcmd_ToggleTweakSaber_f( void ) {
	if ( trap->Argc() == 1 ) {
		int i = 0;
		for ( i = 0; i < MAX_SABER_TWEAKS; i++ ) {
			if ( (g_tweakSaber.integer & (1 << i)) ) {
				trap->Print( "%2d [X] %s\n", i, saberTweaks[i].string );
			}
			else {
				trap->Print( "%2d [ ] %s\n", i, saberTweaks[i].string );
			}
		}
		return;
	}
	else {
		char arg[8] = { 0 };
		int index;
		const uint32_t mask = (1 << MAX_SABER_TWEAKS) - 1;

		trap->Argv( 1, arg, sizeof(arg) );
		index = atoi( arg );

		//DM Start: New -1 toggle all options.
		if (index < -1 || index >= MAX_SABER_TWEAKS) {  //Whereas we need to allow -1 now, we must change the limit for this value.
			trap->Print("tweakSaber: Invalid range: %i [0-%i, or -1 for toggle all]\n", index, MAX_SABER_TWEAKS - 1);
			return;
		}

		if (index == -1) {
			for (index = 0; index < MAX_SABER_TWEAKS; index++) {  //Read every tweak option and set it to the opposite of what it is currently set to.
				trap->Cvar_Set("g_tweakSaber", va("%i", (1 << index) ^ (g_tweakSaber.integer & mask)));
				trap->Cvar_Update(&g_tweakSaber);
				trap->Print("%s %s^7\n", saberTweaks[index].string, ((g_tweakSaber.integer & (1 << index)) ? "^2Enabled" : "^1Disabled"));
				CVU_TweakSaber();
			}
		} //DM End: New -1 toggle all options.

		trap->Cvar_Set( "g_tweakSaber", va( "%i", (1 << index) ^ (g_tweakSaber.integer & mask ) ) );
		trap->Cvar_Update( &g_tweakSaber );

		trap->Print( "%s %s^7\n", saberTweaks[index].string, ((g_tweakSaber.integer & (1 << index))
			? "^2Enabled" : "^1Disabled") );

		CVU_TweakSaber();
	}
}

static bitInfo_T forceTweaks[] = { 
	{"No forcepower drain for crouch attack"},//1
	{"Fix projectile force push dir"},//2
	{"Can push/pull knocked down players"},//3
	{"Fix grip absorb"},//4
	{"Allow force combo"},//5
	{"Fix pull strength"},//6
	{"JK2 grip"},//7
	{"Fast grip runspeed"},//8
	{"Push/pull items"},//9
	{"Smaller Drain COF"},//10
	{"JK2 push/pull knockdown"},//11
	{"JK2 style knockdown getup"},//12
	{"Allow push/pull during roll like JK2"},//13
	{"Force drain does not give forcepoints to players using force absorb"},//14
	{"Allow grip during roll"},//15
	{"Weak force pull"}//16
};
static const int MAX_FORCE_TWEAKS = ARRAY_LEN( forceTweaks );

void CVU_TweakForce (void);
void Svcmd_ToggleTweakForce_f( void ) {
	if ( trap->Argc() == 1 ) {
		int i = 0;
		for ( i = 0; i < MAX_FORCE_TWEAKS; i++ ) {
			if ( (g_tweakForce.integer & (1 << i)) ) {
				trap->Print( "%2d [X] %s\n", i, forceTweaks[i].string );
			}
			else {
				trap->Print( "%2d [ ] %s\n", i, forceTweaks[i].string );
			}
		}
		return;
	}
	else {
		char arg[8] = { 0 };
		int index;
		const uint32_t mask = (1 << MAX_FORCE_TWEAKS) - 1;

		trap->Argv( 1, arg, sizeof(arg) );
		index = atoi( arg );

		//DM Start: New -1 toggle all options.
		if (index < -1 || index >= MAX_FORCE_TWEAKS) {  //Whereas we need to allow -1 now, we must change the limit for this value.
			trap->Print("tweakForce: Invalid range: %i [0-%i, or -1 for toggle all]\n", index, MAX_FORCE_TWEAKS - 1);
			return;
		}

		if (index == -1) {
			for (index = 0; index < MAX_FORCE_TWEAKS; index++) {  //Read every tweak option and set it to the opposite of what it is currently set to.
				trap->Cvar_Set("g_tweakForce", va("%i", (1 << index) ^ (g_tweakForce.integer & mask)));
				trap->Cvar_Update(&g_tweakForce);
				trap->Print("%s %s^7\n", forceTweaks[index].string, ((g_tweakForce.integer & (1 << index)) ? "^2Enabled" : "^1Disabled"));
				CVU_TweakForce();
			}
		} //DM End: New -1 toggle all options.

		trap->Cvar_Set( "g_tweakForce", va( "%i", (1 << index) ^ (g_tweakForce.integer & mask ) ) );
		trap->Cvar_Update( &g_tweakForce );

		trap->Print( "%s %s^7\n", forceTweaks[index].string, ((g_tweakForce.integer & (1 << index))
			? "^2Enabled" : "^1Disabled") );

		CVU_TweakForce();
	}
}

/*
static bitInfo_T weaponDisable[] = { // MAX_WEAPON_TWEAKS tweaks (24)
	{"Stun Baton"},//1
	{"Melee"},//2
	{"Saber"},//3
	{"Bryar Pistol"},//4
	{"Blaster"},//5
	{"Disruptor"},//6
	{"Bowcaster"},//7
	{"Repeater"},//8
	{"Demp2"},//9
	{"Flechette"},//10
	{"Rocket Launcher"},//11
	{"Thermal"},//12
	{"Trip Mine"},//13
	{"Det Pack"},//14
	{"Concussion Rifle"},//15
	{"Old Bryar Pistol"},//16
	{"Emplaced Gun"},//17
	{"Turret"}//18
};
static const int MAX_WEAPON_DISABLES = ARRAY_LEN( weaponDisable );

void Svcmd_ToggleWeaponDisable_f( void ) {
	if ( trap->Argc() == 1 ) {
		int i = 0;
		for ( i = 0; i < MAX_WEAPON_DISABLES; i++ ) {
			if ( (g_weaponDisable.integer & (1 << i)) ) {
				trap->Print( "%2d [X] %s\n", i, weaponDisable[i].string );
			}
			else {
				trap->Print( "%2d [ ] %s\n", i, weaponDisable[i].string );
			}
		}
		return;
	}
	else {
		char arg[8] = { 0 };
		int index;
		const uint32_t mask = (1 << MAX_WEAPON_DISABLES) - 1;

		trap->Argv( 1, arg, sizeof(arg) );
		index = atoi( arg );

		if ( index < 0 || index >= MAX_WEAPON_DISABLES ) {
			trap->Print( "weaponDisable: Invalid range: %i [0, %i]\n", index, MAX_WEAPON_DISABLES - 1 );
			return;
		}

		trap->Cvar_Set( "g_weaponDisable", va( "%i", (1 << index) ^ (g_weaponDisable.integer & mask ) ) );
		trap->Cvar_Update( &g_weaponDisable );

		trap->Print( "%s %s^7\n", weaponDisable[index].string, ((g_weaponDisable.integer & (1 << index))
			? "^2Enabled" : "^1Disabled") );
	}
}
*/

static bitInfo_T startingWeapons[] = { // MAX_WEAPON_TWEAKS tweaks (24)
	{""},//0
	{"Stun Baton"},//1
	{"Melee"},//2
	{"Saber"},//3
	{"Bryar Pistol"},//4
	{"Blaster"},//5
	{"Disruptor"},//6
	{"Bowcaster"},//7
	{"Repeater"},//8
	{"Demp2"},//9
	{"Flechette"},//10
	{"Rocket Launcher"},//11
	{"Thermal"},//12
	{"Trip Mine"},//13
	{"Det Pack"},//14
	{"Concussion Rifle"},//15
	{"Old Bryar Pistol"}//16
};
static const int MAX_STARTING_WEAPONS = ARRAY_LEN( startingWeapons );

void CVU_StartingWeapons( void );
void Svcmd_ToggleStartingWeapons_f( void ) {
	if ( trap->Argc() == 1 ) {
		int i = 0;
		for ( i = 0; i < MAX_STARTING_WEAPONS; i++ ) {
			if ( (g_startingWeapons.integer & (1 << i)) ) {
				trap->Print( "%2d [X] %s\n", i, startingWeapons[i].string );
			}
			else {
				trap->Print( "%2d [ ] %s\n", i, startingWeapons[i].string );
			}
		}
		return;
	}
	else {
		char arg[8] = { 0 };
		int index;
		const uint32_t mask = (1 << MAX_STARTING_WEAPONS) - 1;

		trap->Argv( 1, arg, sizeof(arg) );
		index = atoi( arg );

		//DM Start: New -1 toggle all options.
		if (index < -1 || index >= MAX_STARTING_WEAPONS) {  //Whereas we need to allow -1 now, we must change the limit for this value.
			trap->Print("startingWeapons: Invalid range: %i [0-%i, or -1 for toggle all]\n", index, MAX_STARTING_WEAPONS - 1);
			return;
		}

		if (index == -1) {
			for (index = 0; index < MAX_STARTING_WEAPONS; index++) {  //Read every tweak option and set it to the opposite of what it is currently set to.
				trap->Cvar_Set("g_startingWeapons", va("%i", (1 << index) ^ (g_startingWeapons.integer & mask)));
				trap->Cvar_Update(&g_startingWeapons);
				trap->Print("%s %s^7\n", saberTweaks[index].string, ((g_startingWeapons.integer & (1 << index)) ? "^2Enabled" : "^1Disabled"));
				CVU_StartingWeapons();
			}
		} //DM End: New -1 toggle all options.

		trap->Cvar_Set( "g_startingWeapons", va( "%i", (1 << index) ^ (g_startingWeapons.integer & mask ) ) );
		trap->Cvar_Update( &g_startingWeapons );

		trap->Print( "%s %s^7\n", startingWeapons[index].string, ((g_startingWeapons.integer & (1 << index))
			? "^2Enabled" : "^1Disabled") );

		CVU_StartingWeapons();
	}
}

static bitInfo_T startingItems[] = { // MAX_WEAPON_TWEAKS tweaks (24)
	{""},//0
	{"Seeker"},//1
	{"Shield"},//2
	{"Bacta"},//3
	{"Big Bacta"},//4
	{"Binoculars"},//5
	{"Sentry Gun"},//6
	{"Jetpack"},//7
	{"Health Dispenser"},//8
	{"Ammo Dispenser"},//9
	{"E-WEB"},//10
	{"Cloak"},//11
	{"Ability to toggle /jetpack"}//11
};
static const int MAX_STARTING_ITEMS = ARRAY_LEN( startingItems );

void Svcmd_ToggleStartingItems_f( void ) {
	if ( trap->Argc() == 1 ) {
		int i = 0;
		for ( i = 0; i < MAX_STARTING_ITEMS; i++ ) {
			if ( (g_startingItems.integer & (1 << i)) ) {
				trap->Print( "%2d [X] %s\n", i, startingItems[i].string );
			}
			else {
				trap->Print( "%2d [ ] %s\n", i, startingItems[i].string );
			}
		}
		return;
	}
	else {
		char arg[8] = { 0 };
		int index;
		const uint32_t mask = (1 << MAX_STARTING_ITEMS) - 1;

		trap->Argv( 1, arg, sizeof(arg) );
		index = atoi( arg );

		//DM Start: New -1 toggle all options.
		if (index < -1 || index >= MAX_STARTING_ITEMS) {  //Whereas we need to allow -1 now, we must change the limit for this value.
			trap->Print("startingItems: Invalid range: %i [0-%i, or -1 for toggle all]\n", index, MAX_STARTING_ITEMS - 1);
			return;
		}

		if (index == -1) {
			for (index = 0; index < MAX_SABER_TWEAKS; index++) {  //Read every tweak option and set it to the opposite of what it is currently set to.
				trap->Cvar_Set("g_startingItems", va("%i", (1 << index) ^ (g_startingItems.integer & mask)));
				trap->Cvar_Update(&g_startingItems);
				trap->Print("%s %s^7\n", startingItems[index].string, ((g_startingItems.integer & (1 << index)) ? "^2Enabled" : "^1Disabled"));
			}
		} //DM End: New -1 toggle all options.

		trap->Cvar_Set( "g_startingItems", va( "%i", (1 << index) ^ (g_startingItems.integer & mask ) ) );
		trap->Cvar_Update( &g_startingItems );

		trap->Print( "%s %s^7\n", startingItems[index].string, ((g_startingItems.integer & (1 << index))
			? "^2Enabled" : "^1Disabled") );
	}
}

static bitInfo_T saberDisables[] = { // MAX_WEAPON_TWEAKS tweaks (24)
	{"Disable blue style"},//0
	{"Disable yellow style"},//1
	{"Disable red style"},//2
	{"Disable duals"},//3
	{"Distable staff"},//4
	{"Force desann style"},//5
	{"Force tavion style"}//6
};
static const int MAX_SABER_DISABLES = ARRAY_LEN( saberDisables );

void Svcmd_ToggleSaberDisable_f( void ) {
	if ( trap->Argc() == 1 ) {
		int i = 0;
		for ( i = 0; i < MAX_SABER_DISABLES; i++ ) {
			if ( (g_saberDisable.integer & (1 << i)) ) {
				trap->Print( "%2d [X] %s\n", i, saberDisables[i].string );
			}
			else {
				trap->Print( "%2d [ ] %s\n", i, saberDisables[i].string );
			}
		}
		return;
	}
	else {
		char arg[8] = { 0 };
		int index;
		const uint32_t mask = (1 << MAX_SABER_DISABLES) - 1;

		trap->Argv( 1, arg, sizeof(arg) );
		index = atoi( arg );

		//DM Start: New -1 toggle all options.
		if (index < -1 || index >= MAX_SABER_DISABLES) {  //Whereas we need to allow -1 now, we must change the limit for this value.
			trap->Print("saberDisable: Invalid range: %i [0-%i, or -1 for toggle all]\n", index, MAX_SABER_DISABLES - 1);
			return;
		}

		if (index == -1) {
			for (index = 0; index < MAX_SABER_DISABLES; index++) {  //Read every tweak option and set it to the opposite of what it is currently set to.
				trap->Cvar_Set("g_saberDisable", va("%i", (1 << index) ^ (g_saberDisable.integer & mask)));
				trap->Cvar_Update(&g_saberDisable);
				trap->Print("%s %s^7\n", saberDisables[index].string, ((g_saberDisable.integer & (1 << index)) ? "^2Enabled" : "^1Disabled"));
			}
		} //DM End: New -1 toggle all options.

		trap->Cvar_Set( "g_saberDisable", va( "%i", (1 << index) ^ (g_saberDisable.integer & mask ) ) );
		trap->Cvar_Update( &g_saberDisable );

		trap->Print( "%s %s^7\n", saberDisables[index].string, ((g_saberDisable.integer & (1 << index))
			? "^2Enabled" : "^1Disabled") );
	}
}

static bitInfo_T adminOptions[] = {
	{"Amtele"},//0
	{"Amfreeze"},//1
	{"Amtelemark"},//2
	{"Amban"},//3
	{"Amkick"},//4
	{"NPC"},//5
	{"Noclip"},//6
	{"Grantadmin"},//7
	{"Ammap"},//8
	{"Ampsay"},//9
	{"Amforceteam"},//10
	{"Amlockteam"},//11
	{"Amvstr"},//12
	{"See IPs"},//13
	{"Amrename"},//14
	{"Amlistmaps"},//15
	{"Rebuild highscores (?)"},//16
	{"Amwhois"},//17
	{"Amlookup"},//18
	{"Use hide"},//19
	{"See hiders"},//20
	{"Callvote"},//21
	{"Killvote"}//22
};
static const int MAX_ADMIN_OPTIONS = ARRAY_LEN( adminOptions );

void Svcmd_ToggleAdmin_f( void ) {
	if ( trap->Argc() == 1 ) {
		trap->Print("Usage: toggleAdmin <admin level (full or junior) admin option>\n");
		return;
	}
	else if ( trap->Argc() == 2 ) {
		int i = 0, level;
		char arg1[8] = {0};

		trap->Argv( 1, arg1, sizeof(arg1) );
		if ( !Q_stricmp(arg1, "j") || !Q_stricmp(arg1, "junior"))
			level = 0;
		else if ( !Q_stricmp(arg1, "f") || !Q_stricmp(arg1, "full"))
			level = 1;
		else {
			trap->Print("Usage: toggleAdmin <admin level (full or junior) admin option>\n");
			return;
		}

		for ( i = 0; i < MAX_ADMIN_OPTIONS; i++ ) {
			if (level == 0) {
				if ( (g_juniorAdminLevel.integer & (1 << i)) ) {
					trap->Print( "%2d [X] %s\n", i, adminOptions[i].string );
				}
				else {
					trap->Print( "%2d [ ] %s\n", i, adminOptions[i].string );
				}
			}
			else if (level == 1) {
				if ( (g_fullAdminLevel.integer & (1 << i)) ) {
					trap->Print( "%2d [X] %s\n", i, adminOptions[i].string );
				}
				else {
					trap->Print( "%2d [ ] %s\n", i, adminOptions[i].string );
				}
			}
		}
		return;
	}
	else {
		char arg1[8] = {0}, arg2[8] = { 0 };
		int index, level;
		const uint32_t mask = (1 << MAX_ADMIN_OPTIONS) - 1;

		trap->Argv( 1, arg1, sizeof(arg1) );
		if ( !Q_stricmp(arg1, "j") || !Q_stricmp(arg1, "junior"))
			level = 0;
		else if ( !Q_stricmp(arg1, "f") || !Q_stricmp(arg1, "full"))
			level = 1;
		else {
			trap->Print("Usage: toggleAdmin <admin level (full or junior) admin option>\n");
			return;
		}
		trap->Argv( 2, arg2, sizeof(arg2) );
		index = atoi( arg2 );

		if ( index < 0 || index >= MAX_ADMIN_OPTIONS ) {
			trap->Print( "toggleAdmin: Invalid range: %i [0, %i]\n", index, MAX_ADMIN_OPTIONS - 1 );
			return;
		}

		if (level == 0) {
			trap->Cvar_Set( "g_juniorAdminLevel", va( "%i", (1 << index) ^ (g_juniorAdminLevel.integer & mask ) ) );
			trap->Cvar_Update( &g_juniorAdminLevel );

			trap->Print( "%s %s^7\n", adminOptions[index].string, ((g_juniorAdminLevel.integer & (1 << index))
				? "^2Enabled" : "^1Disabled") );
		}
		else if (level == 1) {
			trap->Cvar_Set( "g_fullAdminLevel", va( "%i", (1 << index) ^ (g_fullAdminLevel.integer & mask ) ) );
			trap->Cvar_Update( &g_fullAdminLevel );

			trap->Print( "%s %s^7\n", adminOptions[index].string, ((g_fullAdminLevel.integer & (1 << index))
				? "^2Enabled" : "^1Disabled") );
		}
	}
}

static bitInfo_T voteOptions[] = { 
	{"Capturelimit"},//1
	{"Clientkick"},//2
	{"Forcespec"},//3
	{"Fraglimit"},//4
	{"g_doWarmup"},//5
	{"g_gametype"},//6
	{"kick"},//7
	{"map"},//8
	{"map_restart"},//9
	{"nextmap"},//10
	{"sv_maxteamsize"},//11
	{"timelimit"},//12
	{"vstr"},//13
	{"poll"},//14
	{"pause"},//15
	{"score_restart"}//16
};
static const int MAX_VOTE_OPTIONS = ARRAY_LEN( voteOptions );

void Svcmd_ToggleVote_f( void ) {
	if ( trap->Argc() == 1 ) {
		int i = 0;
		for ( i = 0; i < MAX_VOTE_OPTIONS; i++ ) {
			if ( (g_allowVote.integer & (1 << i)) ) {
				trap->Print( "%2d [X] %s\n", i, voteOptions[i].string );
			}
			else {
				trap->Print( "%2d [ ] %s\n", i, voteOptions[i].string );
			}
		}
		return;
	}
	else {
		char arg[8] = { 0 };
		int index;
		const uint32_t mask = (1 << MAX_VOTE_OPTIONS) - 1;

		trap->Argv( 1, arg, sizeof(arg) );
		index = atoi( arg );

		//DM Start: New -1 toggle all options.
		if (index < -1 || index >= MAX_VOTE_OPTIONS) {  //Whereas we need to allow -1 now, we must change the limit for this value.
			trap->Print("toggleVote: Invalid range: %i [0-%i, or -1 for toggle all]\n", index, MAX_VOTE_OPTIONS - 1);
			return;
		}

		if (index == -1) {
			for (index = 0; index < MAX_VOTE_OPTIONS; index++) {  //Read every tweak option and set it to the opposite of what it is currently set to.
				trap->Cvar_Set("g_allowVote", va("%i", (1 << index) ^ (g_allowVote.integer & mask)));
				trap->Cvar_Update(&g_allowVote);
				trap->Print("%s %s^7\n", voteOptions[index].string, ((g_allowVote.integer & (1 << index)) ? "^2Enabled" : "^1Disabled"));
			}
		} //DM End: New -1 toggle all options.

		trap->Cvar_Set( "g_allowVote", va( "%i", (1 << index) ^ (g_allowVote.integer & mask ) ) );
		trap->Cvar_Update( &g_allowVote );

		trap->Print( "%s %s^7\n", voteOptions[index].string, ((g_allowVote.integer & (1 << index))
			? "^2Enabled" : "^1Disabled") );
	}
}

static bitInfo_T voteTweaks[] = { 
	{"Allow spec callvote in siege gametype"},//1
	{"Allow spec callvote in CTF/TFFA gametypes"},//2
	{"Clear vote when going to spectate"},//3
	{"Dont allow callvote for 30s after mapload"},//4
	{"Floodprotect callvotes by IP"},//5
	{"Dont allow map callvotes for 10 minutes at start of each map"},//6
	{"Add vote delay for map callvotes only"},//7
	{"Allow voting from spectate"},//8
	{"Show votes in console"},//9
	{"Only count voters in pass/fail calculation"},//10
	{"Fix mapchange after gametype vote"}//11
};
static const int MAX_VOTE_TWEAKS = ARRAY_LEN( voteTweaks );

void Svcmd_ToggleTweakVote_f( void ) {
	if ( trap->Argc() == 1 ) {
		int i = 0;
		for ( i = 0; i < MAX_VOTE_TWEAKS; i++ ) {
			if ( (g_tweakVote.integer & (1 << i)) ) {
				trap->Print( "%2d [X] %s\n", i, voteTweaks[i].string );
			}
			else {
				trap->Print( "%2d [ ] %s\n", i, voteTweaks[i].string );
			}
		}
		return;
	}
	else {
		char arg[8] = { 0 };
		int index;
		const uint32_t mask = (1 << MAX_VOTE_TWEAKS) - 1;

		trap->Argv( 1, arg, sizeof(arg) );
		index = atoi( arg );

		//DM Start: New -1 toggle all options.
		if (index < -1 || index >= MAX_VOTE_TWEAKS) {  //Whereas we need to allow -1 now, we must change the limit for this value.
			trap->Print("tweakVote: Invalid range: %i [0-%i, or -1 for toggle all]\n", index, MAX_VOTE_TWEAKS - 1);
			return;
		}

		if (index == -1) {
			for (index = 0; index < MAX_VOTE_TWEAKS; index++) {  //Read every tweak option and set it to the opposite of what it is currently set to.
				trap->Cvar_Set("g_tweakVote", va("%i", (1 << index) ^ (g_tweakVote.integer & mask)));
				trap->Cvar_Update(&g_tweakVote);
				trap->Print("%s %s^7\n", voteTweaks[index].string, ((g_tweakVote.integer & (1 << index)) ? "^2Enabled" : "^1Disabled"));
			}
		} //DM End: New -1 toggle all options.

		trap->Cvar_Set( "g_tweakVote", va( "%i", (1 << index) ^ (g_tweakVote.integer & mask ) ) );
		trap->Cvar_Update( &g_tweakVote );

		trap->Print( "%s %s^7\n", voteTweaks[index].string, ((g_tweakVote.integer & (1 << index))
			? "^2Enabled" : "^1Disabled") );
	}
}

static bitInfo_T emoteDisables[] = { 
	{"Beg"},//1
	{"Breakdance"},//2
	{"Cheer"},//3
	{"Cower"},//4
	{"Dance"},//5
	{"Hug"},//6
	{"Noisy"},//7
	{"Point"},//8
	{"Rage"},//9
	{"Sit"},//10
	{"Surrender"},//11
	{"Smack"},//12
	{"Taunt"},//13
	{"Victory"},//14
	{"Jawa run"},//15
	{"Bernie"},//16
	{"Sleep"},//17
	{"Saberflip"},//18
	{"Slap"},//19
	{"Signal"},//20
	{"Taunt/Flourish/Bow/Meditate outside of duel or while moving"},//20
};
static const int MAX_EMOTE_DISABLES = ARRAY_LEN( emoteDisables );

void Svcmd_ToggleEmotes_f( void ) {
	if ( trap->Argc() == 1 ) {
		int i = 0;
		for ( i = 0; i < MAX_EMOTE_DISABLES; i++ ) {
			if ( (g_emotesDisable.integer & (1 << i)) ) {
				trap->Print( "%2d [X] %s\n", i, emoteDisables[i].string );
			}
			else {
				trap->Print( "%2d [ ] %s\n", i, emoteDisables[i].string );
			}
		}
		return;
	}
	else {
		char arg[8] = { 0 };
		int index;
		const uint32_t mask = (1 << MAX_EMOTE_DISABLES) - 1;

		trap->Argv( 1, arg, sizeof(arg) );
		index = atoi( arg );

		//DM Start: New -1 toggle all options.
		if (index < -1 || index >= MAX_EMOTE_DISABLES) {  //Whereas we need to allow -1 now, we must change the limit for this value.
			trap->Print("disableEmotes: Invalid range: %i [0-%i, or -1 for toggle all]\n", index, MAX_EMOTE_DISABLES - 1);
			return;
		}

		if (index == -1) {
			for (index = 0; index < MAX_EMOTE_DISABLES; index++) {  //Read every tweak option and set it to the opposite of what it is currently set to.
				trap->Cvar_Set("g_emotesDisable", va("%i", (1 << index) ^ (g_emotesDisable.integer & mask)));
				trap->Cvar_Update(&g_emotesDisable);
				trap->Print("%s %s^7\n", emoteDisables[index].string, ((g_emotesDisable.integer & (1 << index)) ? "^2Enabled" : "^1Disabled"));
			}
		} //DM End: New -1 toggle all options.

		trap->Cvar_Set( "g_emotesDisable", va( "%i", (1 << index) ^ (g_emotesDisable.integer & mask ) ) );
		trap->Cvar_Update( &g_emotesDisable );

		trap->Print( "%s %s^7\n", emoteDisables[index].string, ((g_emotesDisable.integer & (1 << index))
			? "^2Enabled" : "^1Disabled") );
	}
}

typedef struct svcmd_s {
	const char	*name;
	void		(*func)(void);
	qboolean	dedicated;
} svcmd_t;

int svcmdcmp( const void *a, const void *b ) {
	return Q_stricmp( (const char *)a, ((svcmd_t*)b)->name );
}

void G_CheckFields( void );
void G_CheckSpawns( void );
void Svcmd_ChangePass_f( void );
void Svcmd_Register_f( void );
void Svcmd_AccountInfo_f( void );
void Svcmd_DeleteAccount_f( void );
void Svcmd_RenameAccount_f( void );
void Svcmd_ClearIP_f( void );
void Svcmd_DBInfo_f( void );
#if _ELORANKING
#if 0
void G_TestAddDuel( void );
#endif
void SV_RebuildElo_f( void );
#endif
#if 1//NEWRACERANKING
void SV_RebuildRaceRanks_f( void );
#endif
#if 0
void G_TestAddRace( void );
#endif
void Svcmd_AccountIPLock_f( void );


/* This array MUST be sorted correctly by alphabetical name field */
svcmd_t svcmds[] = {
	{ "accountInfo",				Svcmd_AccountInfo_f,				qfalse },

	{ "addbot",						Svcmd_AddBot_f,						qfalse },

#if 0
	{ "addDuel",					G_TestAddDuel,						qfalse },
#endif

	{ "addip",						Svcmd_AddIP_f,						qfalse },

#if 0
	{ "addRace",					G_TestAddRace,						qfalse },
#endif

	{ "amban",						Svcmd_AmBan_f,						qfalse },
	{ "amgrantadmin",				Svcmd_Amgrantadmin_f,				qfalse },
	{ "amkick",						Svcmd_AmKick_f,						qfalse },

	{ "botlist",					Svcmd_BotList_f,					qfalse },

	{ "changepassword",				Svcmd_ChangePass_f,					qfalse },

	{ "checkfields",				G_CheckFields,						qfalse },
	{ "checkspawns",				G_CheckSpawns,						qfalse },

	{ "clearIP",					Svcmd_ClearIP_f,					qfalse },
	{ "DBInfo",						Svcmd_DBInfo_f,						qfalse },
	{ "deleteAccount",				Svcmd_DeleteAccount_f,				qfalse },

	{ "entityinfo",					Svcmd_EntityInfo_f,					qfalse },
	{ "entitylist",					Svcmd_EntityList_f,					qfalse },
	{ "forceteam",					Svcmd_ForceTeam_f,					qfalse },
	{ "gametype",					Svcmd_ChangeGametype_f,				qfalse },
	{ "game_memory",				Svcmd_GameMem_f,					qfalse },
	{ "iplock",						Svcmd_AccountIPLock_f,				qfalse },
	{ "listip",						Svcmd_ListIP_f,						qfalse },

	{ "pause",						SV_Pause_f,							qfalse },

#if _ELORANKING
	{ "rebuildElo",					SV_RebuildElo_f,					qfalse },
#endif

#if 1//NEWRACERANKING
	{ "rebuildRaces",				SV_RebuildRaceRanks_f,				qfalse },
#endif

	{ "register",					Svcmd_Register_f,					qfalse },

	{ "removeip",					Svcmd_RemoveIP_f,					qfalse },
	{ "renameAccount",				Svcmd_RenameAccount_f,				qfalse },
	{ "resetScores",				Svcmd_ResetScores_f,				qfalse },

	{ "saberDisable",				Svcmd_ToggleSaberDisable_f,			qfalse },

	{ "say",						Svcmd_Say_f,						qtrue },

	{ "startingItems",				Svcmd_ToggleStartingItems_f,		qfalse },
	{ "startingWeapons",			Svcmd_ToggleStartingWeapons_f,		qfalse },
	{ "toggleAdmin",				Svcmd_ToggleAdmin_f,				qfalse },
	{ "toggleEmotes",				Svcmd_ToggleEmotes_f,				qfalse },

	{ "toggleuserinfovalidation",	Svcmd_ToggleUserinfoValidation_f,	qfalse },
	{ "toggleVote",					Svcmd_ToggleVote_f,					qfalse },

	{ "tweakForce",					Svcmd_ToggleTweakForce_f,			qfalse },
	{ "tweakSaber",					Svcmd_ToggleTweakSaber_f,			qfalse },
	{ "tweakVote",					Svcmd_ToggleTweakVote_f,			qfalse },
	{ "tweakWeapons",				Svcmd_ToggleTweakWeapons_f,			qfalse }
};
static const size_t numsvcmds = ARRAY_LEN( svcmds );

/*
=================
ConsoleCommand

=================
*/
qboolean	ConsoleCommand( void ) {
	char	cmd[MAX_TOKEN_CHARS] = {0};
	svcmd_t	*command = NULL;

	trap->Argv( 0, cmd, sizeof( cmd ) );

	command = (svcmd_t *)bsearch( cmd, svcmds, numsvcmds, sizeof( svcmds[0] ), svcmdcmp );
	if ( !command )
		return qfalse;

	if ( command->dedicated && !dedicated.integer )
		return qfalse;

	command->func();
	return qtrue;
}
