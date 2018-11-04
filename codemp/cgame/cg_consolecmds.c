/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2005 - 2015, ioquake3 contributors
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

// cg_consolecmds.c -- text commands typed in at the local console, or
// executed by a key binding

#include "cg_local.h"
#include "game/bg_saga.h"
#include "ui/ui_shared.h"

/*
=================
CG_TargetCommand_f

=================
*/
void CG_TargetCommand_f( void ) {
	int		targetNum;
	char	test[4];

	targetNum = CG_CrosshairPlayer();
	if ( targetNum == -1 ) {
		return;
	}

	trap->Cmd_Argv( 1, test, 4 );
	trap->SendClientCommand( va( "gc %i %i", targetNum, atoi( test ) ) );
}

/*
=================
CG_SizeUp_f

Keybinding command
=================
*/
static void CG_SizeUp_f (void) {
	trap->Cvar_Set( "cg_viewsize", va( "%i", Q_min( cg_viewsize.integer + 10, 100 ) ) );
}

/*
=================
CG_SizeDown_f

Keybinding command
=================
*/
static void CG_SizeDown_f (void) {
	trap->Cvar_Set( "cg_viewsize", va( "%i", Q_max( cg_viewsize.integer - 10, 30 ) ) );
}

/*
=============
CG_Viewpos_f

Debugging command to print the current position
=============
*/
static void CG_Viewpos_f (void) {
	trap->Print ("%s (%i %i %i) : %i\n", cgs.mapname, (int)cg.refdef.vieworg[0],
		(int)cg.refdef.vieworg[1], (int)cg.refdef.vieworg[2],
		(int)cg.refdef.viewangles[YAW]);
}

static void CG_PTelemark_f (void) {
	int x, y, z, yaw;

	if ((cg.clientNum == cg.predictedPlayerState.clientNum && !cg.demoPlayback) || !cg.snap ) {
		x = cg.predictedPlayerState.origin[0];
		y = cg.predictedPlayerState.origin[1];
		z = cg.predictedPlayerState.origin[2];
		yaw = cg.predictedPlayerState.viewangles[YAW];
	}
	else {
		x = cg.snap->ps.origin[0];
		y = cg.snap->ps.origin[1];
		z = cg.snap->ps.origin[2];
		yaw = cg.snap->ps.viewangles[YAW];
	}

	cg.telemarkX = x;
	cg.telemarkY = y;
	cg.telemarkZ = z;
	cg.telemarkYaw = yaw;

	trap->Print("Telemark set (%i %i %i) : %i\n", x, y, z, yaw);
}

static void CG_PTele_f (void) {
	if (cg.telemarkX || cg.telemarkY || cg.telemarkZ || cg.telemarkYaw)
		trap->SendConsoleCommand(va("setviewpos %i %i %i %i\n", cg.telemarkX, cg.telemarkY, cg.telemarkZ, cg.telemarkYaw));
	else 
		trap->Print("No telemark set!\n");
}

/*
=================
CG_ScoresDown_f

=================
*/
static void CG_ScoresDown_f( void ) {

	CG_BuildSpectatorString();
	if ( cg.scoresRequestTime + 2000 < cg.time ) {
		// the scores are more than two seconds out of data,
		// so request new ones
		cg.scoresRequestTime = cg.time;
		trap->SendClientCommand( "score" );

		// leave the current scores up if they were already
		// displayed, but if this is the first hit, clear them out
		if ( !cg.showScores ) {
			cg.showScores = qtrue;
			cg.numScores = 0;
		}
	} else {
		// show the cached contents even if they just pressed if it
		// is within two seconds
		cg.showScores = qtrue;
	}
}

/*
=================
CG_ScoresUp_f

=================
*/
static void CG_ScoresUp_f( void ) {
	if ( cg.showScores ) {
		cg.showScores = qfalse;
		cg.scoreFadeTime = cg.time;
	}
}

void CG_ClientList_f( void )
{
	clientInfo_t *ci;
	int i;
	int count = 0;

	for( i = 0; i < MAX_CLIENTS; i++ )
	{
		ci = &cgs.clientinfo[ i ];
		if( !ci->infoValid )
			continue;

		switch( ci->team )
		{
		case TEAM_FREE:
			Com_Printf( "%2d " S_COLOR_YELLOW "F   " S_COLOR_WHITE "%s" S_COLOR_WHITE "%s\n", i, ci->name, (ci->botSkill != -1) ? " (bot)" : "" );
			break;

		case TEAM_RED:
			Com_Printf( "%2d " S_COLOR_RED "R   " S_COLOR_WHITE "%s" S_COLOR_WHITE "%s\n", i,
				ci->name, (ci->botSkill != -1) ? " (bot)" : "" );
			break;

		case TEAM_BLUE:
			Com_Printf( "%2d " S_COLOR_BLUE "B   " S_COLOR_WHITE "%s" S_COLOR_WHITE "%s\n", i,
				ci->name, (ci->botSkill != -1) ? " (bot)" : "" );
			break;

		default:
		case TEAM_SPECTATOR:
			Com_Printf( "%2d " S_COLOR_YELLOW "S   " S_COLOR_WHITE "%s" S_COLOR_WHITE "%s\n", i, ci->name, (ci->botSkill != -1) ? " (bot)" : "" );
			break;
		}

		count++;
	}

	Com_Printf( "Listed %2d clients\n", count );
}


static void CG_TellTarget_f( void ) {
	int		clientNum;
	char	command[MAX_SAY_TEXT+10];
	char	message[MAX_SAY_TEXT];

	clientNum = CG_CrosshairPlayer();
	if ( clientNum == -1 ) {
		return;
	}

	trap->Cmd_Args( message, sizeof(message) );
	Com_sprintf( command, sizeof(command), "tell %i %s", clientNum, message );
	trap->SendClientCommand( command );
}

static void CG_TellAttacker_f( void ) {
	int		clientNum;
	char	command[MAX_SAY_TEXT + 10];
	char	message[MAX_SAY_TEXT];

	clientNum = CG_LastAttacker();
	if ( clientNum == -1 ) {
		return;
	}

	trap->Cmd_Args( message, sizeof(message) );
	Com_sprintf( command, sizeof(command), "tell %i %s", clientNum, message );
	trap->SendClientCommand( command );
}

/*
==================
CG_StartOrbit_f
==================
*/

static void CG_StartOrbit_f( void ) {
	char var[MAX_TOKEN_CHARS];

	trap->Cvar_VariableStringBuffer( "developer", var, sizeof( var ) );
	if ( !atoi(var) ) {
		return;
	}
	if (cg_cameraOrbit.value != 0) {
		trap->Cvar_Set ("cg_cameraOrbit", "0");
		trap->Cvar_Set("cg_thirdPerson", "0");
	} else {
		trap->Cvar_Set("cg_cameraOrbit", "5");
		trap->Cvar_Set("cg_thirdPerson", "1");
		trap->Cvar_Set("cg_thirdPersonAngle", "0");
		trap->Cvar_Set("cg_thirdPersonRange", "100");
	}
}

void CG_SiegeBriefingDisplay(int team, int dontshow);
static void CG_SiegeBriefing_f(void)
{
	int team;

	if (cgs.gametype != GT_SIEGE)
	{ //Cannot be displayed unless in this gametype
		return;
	}

	team = cg.predictedPlayerState.persistant[PERS_TEAM];

	if (team != SIEGETEAM_TEAM1 &&
		team != SIEGETEAM_TEAM2)
	{ //cannot be displayed if not on a valid team
		return;
	}

	CG_SiegeBriefingDisplay(team, 0);
}

static void CG_SiegeCvarUpdate_f(void)
{
	int team;

	if (cgs.gametype != GT_SIEGE)
	{ //Cannot be displayed unless in this gametype
		return;
	}

	team = cg.predictedPlayerState.persistant[PERS_TEAM];

	if (team != SIEGETEAM_TEAM1 &&
		team != SIEGETEAM_TEAM2)
	{ //cannot be displayed if not on a valid team
		return;
	}

	CG_SiegeBriefingDisplay(team, 1);
}

static void CG_SiegeCompleteCvarUpdate_f(void)
{
	if (cgs.gametype != GT_SIEGE)
	{ //Cannot be displayed unless in this gametype
		return;
	}

	// Set up cvars for both teams
	CG_SiegeBriefingDisplay(SIEGETEAM_TEAM1, 1);
	CG_SiegeBriefingDisplay(SIEGETEAM_TEAM2, 1);
}

void CG_LoadHud_f( void ) {
	const char *hudSet;

	if (cg_hudFiles.integer > 2) {
		hudSet = "ui/elegance_hud.txt";
	}
	else {
		hudSet = cg_hudFiles.string;
		if ( hudSet[0] == '\0' )
		{
			hudSet = "ui/jahud.txt";
		}
	}

	String_Init();
	Menu_Reset();
	CG_LoadMenus( hudSet );
}

void CG_SanitizeString2(const char *in, char *out)
{
	int i = 0, r = 0;

	while (in[i]) {
		if (i >= MAX_NAME_LENGTH - 1) {//the ui truncates the name here..
			break;
		}
		if (in[i] == '^') {
			if (in[i + 1] >= 48 && in[i + 1] <= 57) { //only skip it if there's a number after it for the color
				i += 2;
				continue;
			}
			else { //just skip the ^
				i++;
				continue;
			}
		}
		if (in[i] < 32) {
			i++;
			continue;
		}
		out[r] = tolower(in[i]);//lowercase please
		r++;
		i++;
	}
	out[r] = 0;
}

static int CG_ClientNumberFromString(const char *s)
{
	clientInfo_t *cl;
	int			idnum, i, match = -1;
	char		s2[MAX_STRING_CHARS];
	char		n2[MAX_STRING_CHARS];
	idnum = atoi(s);

	// numeric values are just slot numbers
	if (s[0] >= '0' && s[0] <= '9' && strlen(s) == 1) //changed this to only recognize numbers 0-31 as client numbers, otherwise interpret as a name, in which case sanitize2 it and accept partial matches (return error if multiple matches)
	{
		idnum = atoi(s);
		cl = &cgs.clientinfo[idnum];
		if (!cl->infoValid) {
			Com_Printf("Client '%i' is not active\n", idnum);
			return -1;
		}
		return idnum;
	}

	if ((s[0] == '1' || s[0] == '2') && (s[1] >= '0' && s[1] <= '9' && strlen(s) == 2))  //changed and to or ..
	{
		idnum = atoi(s);
		cl = &cgs.clientinfo[idnum];
		if (!cl->infoValid) {
			Com_Printf("Client '%i' is not active\n", idnum);
			return -1;
		}
		return idnum;
	}

	if (s[0] == '3' && (s[1] >= '0' && s[1] <= '1' && strlen(s) == 2))
	{
		idnum = atoi(s);
		cl = &cgs.clientinfo[idnum];
		if (!cl->infoValid) {
			Com_Printf("Client '%i' is not active\n", idnum);
			return -1;
		}
		return idnum;
	}

	// check for a name match
	CG_SanitizeString2(s, s2);
	for (idnum = 0, cl = cgs.clientinfo; idnum < cgs.maxclients; ++idnum, ++cl) {
		if (!cl->infoValid) {
			continue;
		}
		CG_SanitizeString2(cl->name, n2);

		for (i = 0; i < cgs.maxclients; i++)
		{
			cl = &cgs.clientinfo[i];
			if (!cl->infoValid) {
				continue;
			}
			CG_SanitizeString2(cl->name, n2);
			if (strstr(n2, s2))
			{
				if (match != -1)
				{ //found more than one match
					Com_Printf("More than one user '%s' on the server\n", s);
					return -2;
				}
				match = i;
			}
		}
		if (match != -1)//uhh
			return match;
	}
	if (!atoi(s)) //Uhh.. well.. whatever. fixes amtele spam problem when teleporting to x y z yaw
		Com_Printf("User '%s' is not on the server\n", s);
	return -1;
}

static void CG_IgnoreVGS_f(void)
{
	if (trap->Cmd_Argc() == 1) {
		int i;
		clientInfo_t *cl;

		Com_Printf("VGS Ignored client(s):\n");
		for (i = 0, cl = cgs.clientinfo; i < cgs.maxclients; ++i, ++cl) {
			if (cl->infoValid && (cgs.ignoredVGS & (1 << i))) {
				trap->Print("^5%2d^3: ^7%s\n", i, cl->name);
			}
		}
	}
	else {
		char playername[MAX_NETNAME];
		trap->Cmd_Argv(1, playername, sizeof(playername));
		if (!Q_stricmp(playername, "-1")) {
			if (cgs.ignoredVGS) {
				cgs.ignoredVGS = 0;
				Com_Printf("VGS Unignored all\n");
			}
			else {
				cgs.ignoredVGS = 0xFFFFFFFF;
				Com_Printf("VGS Ignored all\n");
			}
		}
		else {
			const int target = CG_ClientNumberFromString(playername);
			if (target == -1 || target == -2) {
				return;
			}

			if (cgs.ignoredVGS & (1 << target)) {
				cgs.ignoredVGS ^= (1 << target);
				Com_Printf("VGS Unignored %s\n", playername);
			}
			else {
				cgs.ignoredVGS |= (1 << target);
				Com_Printf("VGS Ignored %s\n", playername);
			}
		}
	}
}

static void CG_ShowSpecCamera_f(void)
{
	int thirdPersonRange, vertOffset;

	thirdPersonRange = cg.predictedPlayerState.persistant[PERS_CAMERA_SETTINGS];
	if (thirdPersonRange < 0)
		thirdPersonRange = -thirdPersonRange;
	//Chop off end two digits
	thirdPersonRange /= 100;
	if (thirdPersonRange < 1)
		thirdPersonRange = cg_thirdPersonRange.value;

	vertOffset = cg.predictedPlayerState.persistant[PERS_CAMERA_SETTINGS];
	if (vertOffset < 0)
		vertOffset = -vertOffset;
	//Leave only last 2 digits
	vertOffset = (int)vertOffset % 100;
	if (vertOffset < 1)
		vertOffset = cg_thirdPersonVertOffset.value;

	trap->Print("^5Camera Settings^3:^7 Range %i, Offset %i^7\n", thirdPersonRange, vertOffset);
}

//JAPRO - Clientside - Serversettings? - Start
static void CG_ServerConfig_f(void) // this should be serverside for JAPRO.  Clientside for JAPLUS etc?
{
	if (cgs.isJAPro) {
		trap->SendClientCommand( "serverconfig" );
	}
	else {
		//(sv_fps.integer > 0) ? CG_Printf("^5sv_fps^3: ^7%i\n", sv_fps.integer) : CG_Printf("^5sv_fps^3: ^7?\n"); 
		if (cgs.isJAPlus) {
			trap->Print("^5Flipkick^3:^7 %s\n", (cgs.cinfo & JAPLUS_CINFO_FLIPKICK) ? "^2Yes" : "^1No");
			trap->Print("^5JK2 roll^3:^7 %s\n", (cgs.cinfo & JAPLUS_CINFO_FIXROLL3) ? "^2Yes" : "^1No");
			trap->Print("^5Improve yellow DFA^3:^7 %s\n", (cgs.cinfo & JAPLUS_CINFO_YELLOWDFA) ? "^2Yes" : "^1No");
			trap->Print("^5Headslide^3:^7 %s\n", (cgs.cinfo & JAPLUS_CINFO_HEADSLIDE) ? "^2Yes" : "^1No");
			trap->Print("^5SP attacks^3:^7 %s\n", (cgs.cinfo & JAPLUS_CINFO_SPATTACKS) ? "^2Yes" : "^1No");
			trap->Print("^5New close-range DFA^3:^7 %s\n", (cgs.cinfo & JAPLUS_CINFO_NEWDFA) ? "^2Yes" : "^1No");
			trap->Print("^5Modelscale^3:^7 %s\n", (cgs.cinfo & JAPLUS_CINFO_MODELSCALE) ? "^2Yes" : "^1No");
			trap->Print("^5Modelscale damage speed scale^3:^7 %s\n", (cgs.cinfo & JAPLUS_CINFO_DMGSPEEDSCALE) ? "^2Yes" : "^1No");
			trap->Print("^5Macroscan^3:^7 %s\n", (cgs.cinfo & JAPLUS_CINFO_MACROSCAN1 || cgs.cinfo & JAPLUS_CINFO_MACROSCAN2) ? "^2Yes" : "^1No");
			trap->Print("^5JK2 red dfa^3:^7 %s\n", (cgs.cinfo & JAPLUS_CINFO_JK2DFA) ? "^2Yes" : "^1No");
			trap->Print("^5Remove kata^3:^7 %s\n", (cgs.cinfo & JAPLUS_CINFO_NOKATA) ? "^2Yes" : "^1No");
			trap->Print("^5Remove auto replier^3:^7 %s\n", (cgs.cinfo & JAPLUS_CINFO_NO_AUTO_REPLIER) ? "^2Yes" : "^1No");
			trap->Print("^5New GLA anims^3:^7 %s\n", (cgs.cinfo & JAPLUS_CINFO_GLA_ANIMS) ? "^2Yes" : "^1No");
			trap->Print("^5Ledgegrab^3:^7 %s\n", (cgs.cinfo & JAPLUS_CINFO_LEDGEGRAB) ? "^2Yes" : "^1No");
			trap->Print("^5Alternate dimension^3:^7 %s\n", (cgs.cinfo & JAPLUS_CINFO_ALTDIM) ? "^2Yes" : "^1No");
		}
		else {
			trap->Print("^5Server is not running jaPRO or JAPlus.\n"); 
		}
	}
}
//JAPRO - Clientside - Serversettings? - End

static void CG_Login_f(void)
{
	char username[MAX_QPATH], password[MAX_QPATH];

	if (cgs.isJAPlus || cgs.isBase) //Block this on mods that dont have /login to help avoid leaking
		return;
	if (cg.predictedPlayerState.pm_type == PM_INTERMISSION && !(cgs.isJAPro || cgs.isBaseEnhanced || cgs.isOJKAlt))
		return;

	trap->Cmd_Argv(1, username, sizeof(username));
	trap->Cmd_Argv(2, password, sizeof(password));
	trap->SendClientCommand(va("login %s %s", username, password));
}

static void CG_ModVersion_f(void)
{
	trap->Print("^5Your client version of the mod was compiled on %s at %s\n", __DATE__, __TIME__);//ass
	trap->SendConsoleCommand("ui_modversion\n");
	if (cgs.isJAPro) {
		trap->SendClientCommand( "modversion" );
		trap->Cvar_Set("cjp_client", "1.4JAPRO"); //Do this manually here i guess, just incase it does not do it when game is created due to ja+ or something
	}
}

static void CG_FollowBlueFlag_f(void) {
	int i;
	clientInfo_t	*ci;

	if (!cg.snap)
		return;

	for (i = 0 ; i < cgs.maxclients ; i++) {
		if (i == cg.snap->ps.clientNum)
				continue;

		ci = &cgs.clientinfo[ i ];

		if (ci->powerups & (1 << PW_BLUEFLAG)) {
			trap->SendClientCommand(va("follow %i", i));
			return;
		}
	}
}

static void CG_FollowRedFlag_f(void) {
	int i;
	clientInfo_t	*ci;

	if (!cg.snap)
		return;

	for (i = 0 ; i < cgs.maxclients ; i++) {
		if (i == cg.snap->ps.clientNum)
			continue;

		ci = &cgs.clientinfo[ i ];

		if (ci->powerups & (1 << PW_REDFLAG)) {
			trap->SendClientCommand(va("follow %i", i));
			return;
		}
	}
}

static void CG_FollowFastest_f(void) {
	int i, fastestPlayer = -1, currentSpeed, fastestSpeed = 0;
	centity_t *cent;

	if (!cg.snap)
		return;

	for (i=0;i<MAX_CLIENTS;i++) {
			if (i == cg.snap->ps.clientNum)
				continue;

			cent = &cg_entities[i];

			if (!cent)
				continue;
			if (cent->currentState.eType != ET_PLAYER)
				continue;

			currentSpeed = VectorLengthSquared(cent->currentState.pos.trDelta);

			if (currentSpeed > fastestSpeed) {
				fastestSpeed = currentSpeed;
				fastestPlayer = i;
			}

	}
	if (fastestPlayer >= 0)
		trap->SendClientCommand(va("follow %i", fastestPlayer));
}

static void CG_RemapShader_f(void) {
	char oldShader[MAX_QPATH], newShader[MAX_QPATH];

	if (trap->Cmd_Argc() != 3) {
		Com_Printf("Usage: /remapShader <old> <new>\n");
		return;
	}

	trap->Cmd_Argv( 1, oldShader, sizeof( oldShader ) );
	trap->Cmd_Argv( 2, newShader, sizeof( newShader ) );

	//validate this shit ?
	//how to stop from using trans shaders..?

	trap->R_RemapShader(oldShader, newShader, NULL);	 //the fuck is timeoffset for

}

static void CG_ListRemaps_f(void) {
	const char	*info;
	char info2[MAX_CONFIGSTRINGS];
	info = CG_ConfigString( CS_SHADERSTATE );

	Q_strncpyz( info2, info, sizeof(info2));

	Q_strstrip( info2, ":", "\n" );	

	Com_Printf("Remaps: \n %s \n", info2);

	//Replace : with newline
	//replace 0.30@ with null?
	//replace = with " -> "

	//keep track of local remaps somehow
	//either directly or add remap text to array when added, list here

}

#if 1
void CG_StrafeTrailLine( vec3_t start, vec3_t end, int time, int clientNum, int number);
void CG_SpawnStrafeTrailFromCFG_f(void) //loda fixme
{
	fileHandle_t f;	
	int		fLen = 0, MAX_NUM_ITEMS = 100000, args = 1, row = 0, clientNum = 28;  //use max num warps idk
	char	input[MAX_QPATH], fileName[MAX_QPATH], buf[512*1024] = {0};//eh
	char*	pch;
	vec3_t spot = {0}, lastSpot = {0}, diff;
	//float radius;

	if (trap->Cmd_Argc() != 2 && trap->Cmd_Argc() != 3) {
		Com_Printf("Usage: /loadTrail <filename> <clientNum (optional)>\n");
		return;
	}

	trap->Cmd_Argv( 1, input, sizeof( input ) );
	Q_strstrip( input, "\n\r;:?*<>|\"\\/ ", NULL );
	Q_CleanStr( input );
	Q_strlwr(fileName);//dat linux
	Com_sprintf(fileName, sizeof(fileName), "strafetrails/%s.cfg", input);
	//Q_strcat(filename, sizeof(filename), ".cfg");

	fLen = trap->FS_Open(fileName, &f, FS_READ);

	if (!f) {
		Com_Printf ("Couldn't load trail locations from %s\n", fileName);
		return;
	}
	if (fLen >= sizeof(buf)) {
		trap->FS_Close(f);
		Com_Printf ("Couldn't load trail locations from %s, file is too large\n", fileName);
		return;
	}

	trap->FS_Read(buf, fLen, f);
	buf[fLen] = 0;
	trap->FS_Close(f);

	trap->Cmd_Argv( 2, input, sizeof( input ) );
	clientNum = atoi(input);
	if (clientNum < 0)
		clientNum = 0;
	else if (clientNum > 28) //idk
		clientNum = 28;

	pch = strtok (buf," \n\t");  //loda fixme why is this broken
	while (pch != NULL && row < MAX_NUM_ITEMS)
	{
		if ((args % 3) == 1)
			spot[0] = atoi(pch);
		else if ((args % 3) == 2)
			spot[1] = atoi(pch);
		else if ((args % 3) == 0) {
			spot[2] = atoi(pch);
			VectorSubtract(spot, lastSpot, diff);
			if (VectorLengthSquared(diff) < 512*512) {
				CG_StrafeTrailLine( spot, lastSpot, 12*60*60*1000, clientNum, row + 1); //clientnum 28 cuz why not
			}
			//trap->Print("Warp added: %s, <%i, %i, %i, %i>\n", warpList[row].name, warpList[row].x, warpList[row].y, warpList[row].z, warpList[row].yaw);
			VectorCopy(spot, lastSpot);
			row++;
		}
    	pch = strtok (NULL, " \n\t");
		args++;
	}

	Com_Printf ("Loaded strafe trail from %s\n", fileName);
}
#endif

void CG_Do_f(void) //loda fixme
{
	char vstr[MAX_QPATH], delay[32];
	int delayMS;

	if (trap->Cmd_Argc() != 3) {
		Com_Printf("Usage: /do <vstr> <delay>\n");
		return;
	}

	if (cgs.restricts & RESTRICT_DO) {
		return;
	}

	if ((cg.clientNum == cg.predictedPlayerState.clientNum) || !cg.snap) {
		if (cg.predictedPlayerState.stats[STAT_RACEMODE])
			return;
	}
	else {
		if (cg.snap->ps.stats[STAT_RACEMODE])
			return;
	}

	trap->Cmd_Argv( 1, vstr, sizeof( vstr ) );
	trap->Cmd_Argv( 2, delay, sizeof( delay ) );
	
	delayMS = atoi(delay);
	if (delay < 0)
		delayMS = 0;
	else if (delayMS > 1000*60*60)
		delayMS = 1000*60*60;

	Com_sprintf(cg.doVstr, sizeof(cg.doVstr), "vstr %s\n", vstr);
	cg.doVstrTime = cg.time + delayMS;
}

static void CG_Saber_f(void) // this should be serverside for JAPRO.  Clientside for JAPLUS etc?
{
	char saber1[MAX_QPATH], saber2[MAX_QPATH];
	if (trap->Cmd_Argc() == 2) {
		trap->Cmd_Argv( 1, saber1, sizeof( saber1 ) );
		if (cgs.isJAPlus || cgs.isJAPro)
			trap->SendClientCommand(va("saber %s", saber1));
		trap->SendConsoleCommand(va("set saber1 %s\n", saber1));
	}
	else if (trap->Cmd_Argc() == 3) {
		trap->Cmd_Argv( 1, saber1, sizeof( saber1 ) );
		trap->Cmd_Argv( 2, saber2, sizeof( saber2 ) );
		if (cgs.isJAPlus || cgs.isJAPro)
			trap->SendClientCommand(va("saber %s %s", saber1, saber2));
		trap->SendConsoleCommand(va("set saber1 %s\n", saber1));
		trap->SendConsoleCommand(va("set saber2 %s\n", saber2));
	}
}

static void CG_Autologin_f(void)
{
	char currentAddress[MAX_ADDRESSLENGTH], autoLoginString[MAX_ADDRESSLENGTH];

	if (cg.predictedPlayerState.pm_type == PM_INTERMISSION && cgs.isJAPlus)
		return;

	Q_strncpyz( currentAddress, cl_currentServerAddress.string, sizeof(currentAddress));

	if ( strchr(currentAddress, ':') == NULL) {
		Q_strcat(currentAddress, sizeof(currentAddress), ":29070");
	}

	//Check is loginserver string has semicolon. if not, append :29070
	//Check is curent string has semicolon, if not, append :29070

	Q_strncpyz( autoLoginString, cg_autoLoginServer1.string, sizeof(autoLoginString));
	if ( strchr(autoLoginString, ':') == NULL) {
		Q_strcat(autoLoginString, sizeof(autoLoginString), ":29070");
	}
	//Com_Printf("Checking match1, current %s, auto %s\n", currentAddress, autoLoginString);
	if (!Q_stricmp(currentAddress, autoLoginString)) {
		trap->SendClientCommand(va( "amLogin %s", cg_autoLoginPass1.string));
		return;
	}

	Q_strncpyz( autoLoginString, cg_autoLoginServer2.string, sizeof(autoLoginString));
	if ( strchr(autoLoginString, ':') == NULL) {
		Q_strcat(autoLoginString, sizeof(autoLoginString), ":29070");
	}
	//Com_Printf("Checking match2, current %s, auto %s\n", currentAddress, autoLoginString);
	if (!Q_stricmp(currentAddress, autoLoginString)) {
		trap->SendClientCommand(va( "amLogin %s", cg_autoLoginPass2.string));
		return;
	}

	Q_strncpyz( autoLoginString, cg_autoLoginServer3.string, sizeof(autoLoginString));
	if ( strchr(autoLoginString, ':') == NULL) {
		Q_strcat(autoLoginString, sizeof(autoLoginString), ":29070");
	}
	//Com_Printf("Checking match3, current %s, auto %s\n", currentAddress, autoLoginString);
	if (!Q_stricmp(currentAddress, autoLoginString)) { 
		trap->SendClientCommand(va( "amLogin %s", cg_autoLoginPass3.string));
		return;
	}

}

static void CG_Sabercolor_f(void)
{
	char redStr[8], blueStr[8], greenStr[8];
	int red, blue, green;
	if (trap->Cmd_Argc() != 4)
	{
		Com_Printf("Usage: /saberColor <red> <green> <blue> e.g. /saberColor 255 255 0\n");
		return;
	}

	//Ideally this should also take saber # into account and let them specify different colors for each saber if using duals but whatever.

	trap->Cmd_Argv(1, redStr, sizeof(redStr));
	trap->Cmd_Argv(2, greenStr, sizeof(greenStr));
	trap->Cmd_Argv(3, blueStr, sizeof(blueStr));

	red = atoi(redStr);
	blue = atoi(blueStr);
	green = atoi(greenStr);

	trap->Cvar_Set("color1", va("%i", SABER_RGB));
	trap->Cvar_Set("cp_sbRGB1", va("%i", red | ((green | (blue << 8)) << 8)));
	trap->Cvar_Set("color2", va("%i", SABER_RGB));
	trap->Cvar_Set("cp_sbRGB2", va("%i", red | ((green | (blue << 8)) << 8)));

}

static void CG_Amcolor_f(void)
{
	char red[8], blue[8], green[8];
	if (trap->Cmd_Argc() != 4)
	{
		Com_Printf("Usage: /amcolor <red> <green> <blue> e.g. /amColor 255 255 0\n");
		return;
	}

	trap->Cmd_Argv( 1, red, sizeof( red ) );
	trap->Cmd_Argv( 2, green, sizeof( green ) );
	trap->Cmd_Argv( 3, blue, sizeof( blue ) );

	trap->Cvar_Set("char_color_red", red);
	trap->Cvar_Set("char_color_green", green); 
	trap->Cvar_Set("char_color_blue", blue);
}
//JAPRO - Clientside - Amcolor - End

static void CG_Flipkick_f(void)
{
	if (cgs.restricts & RESTRICT_FLIPKICKBIND)
		return;

	//Well we always want to do the first kick, unless we are doing some really advanced predictive shit..
	
	//Ok, we started out flipkick.  Each frame we want to remove/add jump (+moveup and -moveup).

	cg.numFKFrames = 1;


	//How to make the perfect KS?
	//Get frametime or com_maxfps ?
	//Do however many taps super fast until they are at max jump kick height?
	//trap->SendConsoleCommand("+moveup;wait 2;-moveup;wait 2;+moveup;wait 2;-moveup;wait 2;+moveup;wait 2;-moveup;wait 2;-moveup;wait 2;+moveup;wait 2;-moveup;wait 2;+moveup;wait 2;-moveup;wait 2;+moveup;wait 2;-moveup;wait 2;+moveup;wait 2;-moveup;wait 2;+moveup;wait 2;-moveup;wait 2;+moveup;wait 2;-moveup;wait 2;+moveup;wait 2;-moveup\n");
}

static void CG_Lowjump_f(void)
{
	if ((cgs.isJAPro && cg.predictedPlayerState.stats[STAT_RACEMODE]) || (cgs.restricts & RESTRICT_DO)) {
		trap->SendConsoleCommand("+moveup;wait 2;-moveup\n");
		return;
	}

	trap->SendConsoleCommand("+moveup\n");
	Q_strncpyz(cg.doVstr, "-moveup\n", sizeof(cg.doVstr));
	cg.doVstrTime = cg.time;
}

static void CG_NorollDown_f(void)
{
	if ((cgs.isJAPro && cg.predictedPlayerState.stats[STAT_RACEMODE]) || (cgs.restricts & RESTRICT_DO)) {
		trap->SendConsoleCommand("+speed;wait 2;-moveup;+movedown;-speed\n");
		return;
	}

	trap->SendConsoleCommand("+speed;-moveup\n");
	Q_strncpyz(cg.doVstr, "+movedown;-speed\n", sizeof(cg.doVstr));
	cg.doVstrTime = cg.time;
}

static void CG_NorollUp_f(void)
{
	if ((cgs.isJAPro && cg.predictedPlayerState.stats[STAT_RACEMODE]) || cgs.restricts & RESTRICT_DO) {
		trap->SendConsoleCommand("-movedown\n");
		return;
	}

	Q_strncpyz(cg.doVstr, "-movedown;-speed\n", sizeof(cg.doVstr)); //?
	cg.doVstrTime = cg.time;
}

qboolean CG_WeaponSelectable(int i);
void CG_LastWeapon_f(void) //loda fixme. japro
{
	if (cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_SPECTATOR)
		return;
	if (cg.predictedPlayerState.pm_flags & PMF_FOLLOW)
		return;

	if (!cg.lastWeaponSelect[0])
		cg.lastWeaponSelect[0] = cg.predictedPlayerState.weapon;
	if (!cg.lastWeaponSelect[1])
		cg.lastWeaponSelect[1] = cg.predictedPlayerState.weapon;

	if (cg.lastWeaponSelect[1] == cg.lastWeaponSelect[0]) { //if the weapon we spawned with is still equipped
		int i;
		for (i = LAST_USEABLE_WEAPON; i > 0; i--) {	//cycle to the next available one
			if ((i != cg.weaponSelect) && CG_WeaponSelectable(i)) {
				cg.lastWeaponSelect[1] = i;
				break;
			}
		}
	}

	if (cg.lastWeaponSelect[0] != cg.weaponSelect) { //Current does not match selected
		cg.lastWeaponSelect[1] = cg.lastWeaponSelect[0]; //Set last to current
		cg.lastWeaponSelect[0] = cg.weaponSelect; //Set current to selected
	}

	cg.weaponSelect = cg.lastWeaponSelect[1]; //Set selected to last

	cg.weaponSelectTime = cg.time;
	if (cg.weaponSelect != cg.lastWeaponSelect[1])
		trap->S_MuteSound(cg.predictedPlayerState.clientNum, CHAN_WEAPON);

}

typedef struct bitInfo_S {
	const char	*string;
} bitInfo_T;

static bitInfo_T strafeTweaks[] = {
	{"Original style"},//0
	{"Updated style"},//1
	{"Cgaz style"},//2
	{"Warsow style"},//3
	{"Sound"},//4
	{"W"},//5
	{"WA"},//6
	{"WD"},//7
	{"A"},//8
	{"D"},//9
	{"Rear"},//10
	{"Center"},//11
	{"Accel bar"},//12
	{"Weze style"},//13
	{"Line Crosshair"}//13
};

static const int MAX_STRAFEHELPER_TWEAKS = ARRAY_LEN( strafeTweaks );
void CG_StrafeHelper_f( void ) {
	if ( trap->Cmd_Argc() == 1 ) {
		int i = 0;
		for ( i = 0; i < MAX_STRAFEHELPER_TWEAKS; i++ ) {
			if ( (cg_strafeHelper.integer & (1 << i)) ) {
				Com_Printf( "%2d [X] %s\n", i, strafeTweaks[i].string );
			}
			else {
				Com_Printf( "%2d [ ] %s\n", i, strafeTweaks[i].string );
			}
		}
		return;
	}
	else {
		char arg[8] = { 0 };
		int index;
		const uint32_t mask = (1 << MAX_STRAFEHELPER_TWEAKS) - 1;

		trap->Cmd_Argv( 1, arg, sizeof(arg) );
		index = atoi( arg );

		if ( index < 0 || index >= MAX_STRAFEHELPER_TWEAKS ) {
			Com_Printf( "strafeHelper: Invalid range: %i [0, %i]\n", index, MAX_STRAFEHELPER_TWEAKS - 1 );
			return;
		}

		if ((index == 0 || index == 1 || index == 2 || index == 3 || index == 13)) { //Radio button these options
			//Toggle index, and make sure everything else in this group (0,1,2,3,13) is turned off
			int groupMask = (1 << 0) + (1 << 1) + (1 << 2) + (1 << 3) + (1 << 13);
			int value = cg_strafeHelper.integer;

			groupMask &= ~(1 << index); //Remove index from groupmask
			value &= ~(groupMask); //Turn groupmask off
			value ^= (1 << index); //Toggle index item

			trap->Cvar_Set("cg_strafeHelper", va("%i", value));
		}
		else {
			trap->Cvar_Set("cg_strafeHelper", va("%i", (1 << index) ^ (cg_strafeHelper.integer & mask)));
		}
		trap->Cvar_Update( &cg_strafeHelper );

		Com_Printf( "%s %s^7\n", strafeTweaks[index].string, ((cg_strafeHelper.integer & (1 << index))
			? "^2Enabled" : "^1Disabled") );
	}
}

static qboolean japroPluginDisables[] = {
	qfalse,//{"New drain FX"},//1
	qfalse,//{"Duel see others"},//2
	qfalse,//{"End duel rotation"},//3
	qtrue,//{"Black saber disable"},//4
	qfalse,//{"Auto reply disable"},//5
	qfalse,//{"New force effect"},//6
	qfalse,//{"New deathmsg disable"},//7
	qfalse,//{"New sight effect"},//8
	qfalse,//{"No alt dim effect"},//9
	qfalse,//{"Holstered saber"},//10
	qfalse,//{"Ledge grab"},//11
	qfalse,//{"New DFA Primary"},//12
	qfalse,//{"New DFA Alt"},//13
	qfalse,//{"No SP Cartwheel"},//14
	qfalse,//{"No Auto DL Redirect"},//15

	qfalse,//{"No Kata"},//16   
	qfalse,//{"No Butterfly"},//17
	qfalse,//{"No Stab"},//18
	qfalse,//{"No DFA"},//19

	qtrue,//{"Disable forcejumps"},//19
	qtrue,//{"Disable rolls"},//20
	qtrue,//{"Disable cartwheels"},//21
	qtrue,//{"New run animation"},//22
	qtrue,//{"Disable duel tele"},//23
	qtrue,//{"Disable centerprint checkpoints"},//24
	qtrue,//{"Show chatbox checkpoints"},//25
	qtrue,//{"Disable damage numbers"},//26
	qtrue,//{"Centermuzzle"},//27
	qtrue,//{"Show checkpoints in console only"}//28
};

static qboolean japlusPluginDisables[] = {
	qtrue,//{"New drain FX"},//1
	qtrue,//{"Duel see others"},//2
	qtrue,//{"End duel rotation"},//3
	qtrue,//{"Black saber disable"},//4
	qtrue,//{"Auto reply disable"},//5
	qtrue,//{"New force effect"},//6
	qtrue,//{"New deathmsg disable"},//7
	qtrue,//{"New sight effect"},//8
	qtrue,//{"No alt dim effect"},//9
	qtrue,//{"Holstered saber"},//10
	qtrue,//{"Ledge grab"},//11
	qtrue,//{"New DFA Primary"},//12
	qtrue,//{"New DFA Alt"},//13
	qtrue,//{"No SP Cartwheel"},//14
	qtrue,//{"No Auto DL Redirect"},//15

	qfalse,//{"No Kata"},//16   
	qfalse,//{"No Butterfly"},//17
	qfalse,//{"No Stab"},//18
	qfalse,//{"No DFA"},//19

	qfalse,//{"Disable forcejumps"},//19
	qfalse,//{"Disable rolls"},//20
	qfalse,//{"Disable cartwheels"},//21
	qfalse,//{"New run animation"},//22
	qfalse,//{"Disable duel tele"},//23
	qfalse,//{"Disable centerprint checkpoints"},//24
	qfalse,//{"Show chatbox checkpoints"},//25
	qfalse,//{"Disable damage numbers"},//26
	qfalse,//{"Centermuzzle"},//27
	qfalse,//{"Show checkpoints in console only"}//28
};

static bitInfo_T pluginDisables[] = { // MAX_WEAPON_TWEAKS tweaks (24)
	{"New drain FX"},//1
	{"Duel see others"},//2
	{"End duel rotation"},//3
	{"No black sabers"},//4
	{"No auto replier"},//5
	{"New force effect"},//6
	{"No new deathmsg"},//7
	{"New sight effect"},//8
	{"No alt dim effect"},//9
	{"Holstered saber"},//10
	{"Ledge grab"},//11
	{"New DFA Primary"},//12
	{"New DFA Alt"},//13
	{"No SP Cartwheel"},//14
	{"No Auto DL Redirect"},//15

	{"No Kata"},//16   
	{"No Butterfly"},//17
	{"No Stab"},//18
	{"No DFA"},//19

	{"Disable forcejumps"},//19
	{"Disable rolls"},//20
	{"Disable cartwheels"},//21
	{"New run animation"},//22
	{"Disable duel tele"},//23
	{"Disable centerprint checkpoints"},//24
	{"Show chatbox checkpoints"},//25
	{"Disable damage numbers"},//26
	{"Centermuzzle"},//27
	{"Show checkpoints in console only"}//28
};
static const int MAX_PLUGINDISABLES = ARRAY_LEN( pluginDisables );

void CG_PluginDisable_f( void ) {

	if (!cgs.isJAPro && !cgs.isJAPlus) {
		return;
	}

	if ( trap->Cmd_Argc() == 1 ) {
		int i = 0, display = 0;

		for ( i = 0; i < MAX_PLUGINDISABLES; i++ ) {

			if (cgs.isJAPlus && !japlusPluginDisables[i])
				continue;
			if (cgs.isJAPro && !japroPluginDisables[i])
				continue;

			if ( (cp_pluginDisable.integer & (1 << i)) ) {
				Com_Printf( "%2d [X] %s\n", display, pluginDisables[i].string );
			}
			else {
				Com_Printf( "%2d [ ] %s\n", display, pluginDisables[i].string );
			}
			display++;
		}
		return;
	}
	else {
		char arg[8] = { 0 };
		int index, index2, i, n = 0;
		const uint32_t mask = (1 << MAX_PLUGINDISABLES) - 1;

		trap->Cmd_Argv( 1, arg, sizeof(arg) );
		index = atoi( arg );
		index2 = index;

		for ( i = 0; i < MAX_PLUGINDISABLES; i++ ) {		
			//ok so, if they type /plugin #
			//go through the list of plugindisables, from 0 to max, 
			//for each qtrue, increment I
			//once I = #, thats the actual index we want

			if ((cgs.isJAPlus && japlusPluginDisables[i]) || (cgs.isJAPro && japroPluginDisables[i])) {
				//Com_Printf("Option found %i, %s, n is %i, index is %i\n", i, pluginDisables[i], n, index);
				if (n == index) {
					index2 = i;
					break;
				}
				n++;
			}
		}

		if ( index2 < 0 || index2 >= MAX_PLUGINDISABLES ) {
			Com_Printf( "plugin: Invalid range: %i [0, %i]\n", index2, MAX_PLUGINDISABLES - 1 );
			return;
		}

		trap->Cvar_Set( "cp_pluginDisable", va( "%i", (1 << index2) ^ (cp_pluginDisable.integer & mask ) ) );
		trap->Cvar_Update( &cp_pluginDisable );

		Com_Printf( "%s %s^7\n", pluginDisables[index2].string, ((cp_pluginDisable.integer & (1 << index2))
			? "^2Enabled" : "^1Disabled") );
	}
}

//Playermodel shit
static qboolean japroPlayerStyles[] = {
	qtrue,//Fullbright skins
	qtrue,//Private duel shell
	qtrue,//Hide duelers in FFA
	qtrue,//Hide racers in FFA
	qtrue,//Hide non-racers in race mode
	qtrue,//Hide racers in race mode
	qtrue,//Disable racer VFX
	qtrue,//Disable non-racer VFX
	qtrue,//VFX duelers
	qfalse,//VFX am alt dim
	qfalse,//Hide non duelers
	qtrue,//Hide ysal shell
	qtrue,//LOD player model
	qtrue,//Fade corpses immediately
	qtrue,//Disable corpse fading SFX
	qtrue,//Color respawn bubbles by team
	qtrue//Hide player cosmetics
};

//JA+ Specific = amaltdim ?
//Can we treat altdim same as racemode?
static qboolean japlusPlayerStyles[] = {
	qtrue,//Fullbright skins
	qtrue,//Private duel shell
	qtrue,//Hide duelers in FFA
	qfalse,//Hide racers in FFA
	qfalse,//Hide non-racers in race mode
	qfalse,//Hide racers if racer
	qfalse,//Disable racer VFX
	qfalse,//Disable non-racer VFX
	qtrue,//VFX duelers
	qtrue,//VFX am alt dim
	qfalse,//Hide non dueler
	qtrue,//Hide ysal shell
	qtrue,//LOD player model
	qtrue,//Fade corpses immediately
	qtrue,//Disable corpse fading SFX
	qtrue,//Color respawn bubbles by team
	qfalse//Hide player cosmetics
};

static bitInfo_T playerStyles[] = { // MAX_WEAPON_TWEAKS tweaks (24)
	{ "Fullbright skins" },//0
	{ "Private duel shell" },//1//need better name for this?
	{ "Hide duelers in FFA" },//2
	{ "Hide racers in FFA" },//3
	{ "Hide non-racers in race mode" },//4
	{ "Hide racers in race mode" },//5
	{ "Disable racer VFX" },//6
	{ "Disable non-racer VFX in race mode" },//7
	{ "VFX duelers 1" },//8
	{ "VFX am alt dim 1" },//9
	{ "Hide non duelers" },//10
	{ "Hide ysal shell" },//11
	{ "LOD player model" },//12 need better name for this
	{ "Fade corpses immediately" },//13
	{ "Disable corpse fading SFX" },//14
	{ "Color respawn bubbles by team" },//15
	{ "Hide player cosmetics" }//16
};
static const int MAX_PLAYERSTYLES = ARRAY_LEN(playerStyles);

void CG_StylePlayer_f(void)
{
	if (trap->Cmd_Argc() == 1) {
		int i = 0, display = 0;

		for (i = 0; i < MAX_PLAYERSTYLES; i++) {

			if (cgs.isJAPlus && !japlusPlayerStyles[i])
				continue;
			if (cgs.isJAPro && !japroPlayerStyles[i])
				continue;

			if ((cg_stylePlayer.integer & (1 << i))) {
				Com_Printf("%2d [X] %s\n", display, playerStyles[i].string);
			}
			else {
				Com_Printf("%2d [ ] %s\n", display, playerStyles[i].string);
			}
			display++;
		}
		return;
	}
	else {
		char arg[8] = { 0 };
		int index, index2, i, n = 0;
		const uint32_t mask = (1 << MAX_PLAYERSTYLES) - 1;

		trap->Cmd_Argv(1, arg, sizeof(arg));
		index = atoi(arg);
		index2 = index;

		for (i = 0; i < MAX_PLAYERSTYLES; i++) {
			//ok so, if they type /plugin #
			//go through the list of plugindisables, from 0 to max, 
			//for each qtrue, increment I
			//once I = #, thats the actual index we want

			if ((cgs.isJAPlus && japlusPlayerStyles[i]) || (cgs.isJAPro && japroPlayerStyles[i])) {
				//Com_Printf("Option found %i, %s, n is %i, index is %i\n", i, pluginDisables[i], n, index);
				if (n == index) {
					index2 = i;
					break;
				}
				n++;
			}
		}

		if (index2 < 0 || index2 >= MAX_PLAYERSTYLES) {
			Com_Printf("style: Invalid range: %i [0, %i]\n", index2, MAX_PLAYERSTYLES - 1);
			return;
		}

		//if (index == ..., ...)
		if (0) { //Radio button these options
			//Toggle index, and make sure everything else in this group (0,1,2,3,13) is turned off
			int groupMask = (1 << 0) + (1 << 1) + (1 << 2) + (1 << 3) + (1 << 13);
			int value = cg_stylePlayer.integer;

			groupMask &= ~(1 << index); //Remove index from groupmask
			value &= ~(groupMask); //Turn groupmask off
			value ^= (1 << index); //Toggle index item

			trap->Cvar_Set("cg_stylePlayer", va("%i", value));
		}
		else {
			trap->Cvar_Set("cg_stylePlayer", va("%i", (1 << index2) ^ (cg_stylePlayer.integer & mask)));
		}
		trap->Cvar_Update(&cg_stylePlayer);

		Com_Printf("%s %s^7\n", playerStyles[index2].string, ((cg_stylePlayer.integer & (1 << index2))
			? "^2Enabled" : "^1Disabled"));
	}
}

static bitInfo_T speedometerSettings[] = { // MAX_WEAPON_TWEAKS tweaks (24)
	{ "Enable speedometer" },//0
	{ "Pre-speed display" },//1
	{ "Jump height display" },//2
	{ "Jump distance display" },//3
	{ "Vertical speed indicator" },//4
	{ "Yaw speed indicator" },//5
	{ "Accel meter" },//6
	{ "Speed graph" },//7
	{ "Display speed in kilometers instead of units" },//8
	{ "Display speed in imperial miles instead of units" },//9
};
static const int MAX_SPEEDOMETER_SETTINGS = ARRAY_LEN(speedometerSettings);

void CG_SpeedometerSettings_f(void)
{
	if (trap->Cmd_Argc() == 1) {
		int i = 0, display = 0;

		for (i = 0; i < MAX_SPEEDOMETER_SETTINGS; i++) {
			if (cg_speedometer.integer & (1 << i)) {
				Com_Printf("%2d [X] %s\n", display, speedometerSettings[i].string);
			}
			else {
				Com_Printf("%2d [ ] %s\n", display, speedometerSettings[i].string);
			}
			display++;
		}
		return;
	}
	else {
		char arg[8] = { 0 };
		int index, index2;
		const uint32_t mask = (1 << MAX_SPEEDOMETER_SETTINGS) - 1;

		trap->Cmd_Argv(1, arg, sizeof(arg));
		index = atoi(arg);
		index2 = index;

		if (index2 < 0 || index2 >= MAX_SPEEDOMETER_SETTINGS) {
			Com_Printf("style: Invalid range: %i [0, %i]\n", index2, MAX_SPEEDOMETER_SETTINGS - 1);
			return;
		}

		if (index == 8 || index == 9) { //Radio button these options
		//Toggle index, and make sure everything else in this group (8,9) is turned off
			int groupMask = (1 << 8) + (1 << 9);
			int value = cg_speedometer.integer;

			groupMask &= ~(1 << index); //Remove index from groupmask
			value &= ~(groupMask); //Turn groupmask off
			value ^= (1 << index); //Toggle index item

			trap->Cvar_Set("cg_speedometer", va("%i", value));
		}
		else {
			trap->Cvar_Set("cg_speedometer", va("%i", (1 << index) ^ (cg_speedometer.integer & mask)));
		}
		trap->Cvar_Update(&cg_speedometer);

		Com_Printf("%s %s^7\n", speedometerSettings[index2].string, ((cg_speedometer.integer & (1 << index2))
			? "^2Enabled" : "^1Disabled"));
	}
}

static bitInfo_T cosmetics[] = {
	{ "santa" },
	{ "jack-o-lantern" },
	{ "bass pro baseball cap?" },
	{ "indiana jones" },
	{ "Kane's Kringe Kap" }
};
static const int MAX_COSMETICS = ARRAY_LEN(cosmetics);

static void CG_Cosmetics_f(void)
{
	if (!cgs.isJAPro)
		return;

	if (trap->Cmd_Argc() == 1) {
		int i = 0, display = 0;

		for (i = 0; i < MAX_COSMETICS; i++) {
			if ((cp_cosmetics.integer & (1 << i))) {
				Com_Printf("%2d [X] %s\n", display, cosmetics[i].string);
			}
			else {
				Com_Printf("%2d [ ] %s\n", display, cosmetics[i].string);
			}
			display++;
		}
		return;
	}
	else {
		char arg[8] = { 0 };
		int index, index2, i, n = 0;
		const uint32_t mask = (1 << MAX_COSMETICS) - 1;

		trap->Cmd_Argv(1, arg, sizeof(arg));
		index = atoi(arg);
		index2 = index;

		for (i = 0; i < MAX_COSMETICS; i++) {
			if (n == index) {
				index2 = i;
				break;
			}
			n++;
		}

		if (index2 < 0 || index2 >= MAX_COSMETICS) {
			Com_Printf("style: Invalid range: %i [0, %i]\n", index2, MAX_PLAYERSTYLES - 1);
			return;
		}

		//Radio button all options for now
		trap->Cvar_Set("cp_cosmetics", "0");

		if (!(cp_cosmetics.integer & (1 << index)))
			trap->Cvar_Set("cp_cosmetics", va("%i", (1 << index)));

		trap->Cvar_Update(&cp_cosmetics);

		Com_Printf("%s %s^7\n", cosmetics[index2].string, ((cp_cosmetics.integer & (1 << index2))
			? "^2Enabled" : "^1Disabled"));
	}
}

static void CG_AmRun_f(void)
{
	const uint32_t mask = (1 << MAX_PLUGINDISABLES) - 1;

	if (!cgs.isJAPro)
		return;

	trap->Cvar_Set( "cp_pluginDisable", va( "%i", (JAPRO_PLUGIN_JAWARUN) ^ (cp_pluginDisable.integer & mask ) ) );
	trap->Cvar_Update( &cp_pluginDisable ); //needed ?
}

/*
qboolean IsCheckpointEmpty(clientCheckpoint_t clientCheckpoint) {
	if (clientCheckpoint.x1 || clientCheckpoint.y1 || clientCheckpoint.z1 || clientCheckpoint.x2 || clientCheckpoint.y2 || clientCheckpoint.z2)
		return qfalse;
	return qtrue;
}
*/

static void CG_AddSpeedpoint_f(void)
{
	char input[8];
	int speed, i, j;

	if (trap->Cmd_Argc() != 2) { //Addspeedpoint <speed>
		Com_Printf("Usage: /addSpeedSound <speed>\n");
		return;
	}

	trap->Cmd_Argv(1, input, sizeof(input));
	speed = atoi(input);

	for (i = 0; i<MAX_CLIENT_SPEEDPOINTS; i++) { //Add a speedpoint to the first available slot
		if (!cg.clientSpeedpoints[i].isSet)
			break;
	}

	if (i >= MAX_CLIENT_SPEEDPOINTS) {
		Com_Printf("Too many speedsounds!\n");
		return;
	}

	if (speed < 0)
		speed = -speed;

	//Check for duplicate?
	for (j = 0; j<MAX_CLIENT_SPEEDPOINTS; j++) {
		if (cg.clientSpeedpoints[j].isSet && cg.clientSpeedpoints[j].isSet == speed) {
			Com_Printf("Duplicate speedsound!\n");
			return;
		}
	}

	cg.clientSpeedpoints[i].speed = speed;
	cg.clientSpeedpoints[i].isSet = qtrue;

	Com_Printf("Speedsound added.\n");
}

static void CG_DeleteSpeedpoint_f(void) //This should reorder them so no empty checkpoints in the middle of set checkpoints?
{
	char arg[8];
	int i;

	if (trap->Cmd_Argc() != 2) {
		Com_Printf("Usage: /deleteSpeedsound <speedsound #>\n");
		return;
	}

	trap->Cmd_Argv(1, arg, sizeof(arg));
	i = atoi(arg);

	if (i == -1) {
		for (i = 0; i<MAX_CLIENT_SPEEDPOINTS; i++) {
			cg.clientSpeedpoints[i].speed = 0;
			cg.clientSpeedpoints[i].isSet = qfalse;
		}
		Com_Printf("Speedpoints deleted.\n");
		return;
	}

	if (i < 0 || i >= MAX_CLIENT_SPEEDPOINTS) { //Allow -1
		Com_Printf("Speedsound #%i is out of range!\n", i);
		return;
	}

	if (!cg.clientSpeedpoints[i].isSet) {
		Com_Printf("Speedsound #%i is already empty!\n", i);
		return;
	}

	cg.clientSpeedpoints[i].speed = 0;
	cg.clientSpeedpoints[i].isSet = qfalse;

	Com_Printf("Speedpoint deleted.\n");
}

static void CG_ListSpeedpoints_f(void)
{
	int i;

	if (trap->Cmd_Argc() != 1) {
		Com_Printf("Usage: /listSpeedsounds\n");
		return;
	}

	for (i = 0; i<MAX_CLIENT_SPEEDPOINTS; i++) { //Add a checkpoint to the first available slot
		if (cg.clientSpeedpoints[i].isSet)
			Com_Printf("^5%i^3: ^3%i\n", i, cg.clientSpeedpoints[i].speed);
	}
}

extern void CG_CubeOutline(vec3_t mins, vec3_t maxs, int time, unsigned int color, float alpha);
static void CG_AddCheckpoint_f(void) 
{
	char x1s[8], x2s[8], y1s[8], y2s[8], z1s[8], z2s[8];
	int x1, x2, y1, y2, z1, z2;
	int i;
	vec3_t mins, maxs;

	if (trap->Cmd_Argc() != 7) { //Addcheckpoint X Y Z X Y Z
		Com_Printf("Usage: /addCheckpoint X1 Y1 Z1 X2 Y2 Z2\n");
		return;
	}

	trap->Cmd_Argv( 1, x1s, sizeof(x1s) );
	trap->Cmd_Argv( 2, y1s, sizeof(y1s) );
	trap->Cmd_Argv( 3, z1s, sizeof(z1s) );
	trap->Cmd_Argv( 4, x2s, sizeof(x2s) );
	trap->Cmd_Argv( 5, y2s, sizeof(y2s) );
	trap->Cmd_Argv( 6, z2s, sizeof(z2s) );

	x1 = atoi(x1s);
	y1 = atoi(y1s);
	z1 = atoi(z1s);
	x2 = atoi(x2s);
	y2 = atoi(y2s);
	z2 = atoi(z2s);

	for (i=0; i<MAX_CLIENT_CHECKPOINTS; i++) { //Add a checkpoint to the first available slot
		if (!cg.clientCheckpoints[i].isSet)
			break;
	}

	if (i >= MAX_CLIENT_CHECKPOINTS) {
		Com_Printf("Too many checkpoints!\n");
		return;
	}

	if (x1 < x2) {
		cg.clientCheckpoints[i].x1 = x1;
		cg.clientCheckpoints[i].x2 = x2;
	}
	else {
		cg.clientCheckpoints[i].x1 = x2;
		cg.clientCheckpoints[i].x2 = x1;
	}
	if (y1 < y2) {
		cg.clientCheckpoints[i].y1 = y1;
		cg.clientCheckpoints[i].y2 = y2;
	}
	else {
		cg.clientCheckpoints[i].y1 = y2;
		cg.clientCheckpoints[i].y2 = y1;
	}
	if (z1 < z2) {
		cg.clientCheckpoints[i].z1 = z1;
		cg.clientCheckpoints[i].z2 = z2;
	}
	else {
		cg.clientCheckpoints[i].z1 = z2;
		cg.clientCheckpoints[i].z2 = z1;
	}

	mins[0] = cg.clientCheckpoints[i].x1;
	mins[1] = cg.clientCheckpoints[i].y1;
	mins[2] = cg.clientCheckpoints[i].z1;
	maxs[0] = cg.clientCheckpoints[i].x2;
	maxs[1] = cg.clientCheckpoints[i].y2;
	maxs[2] = cg.clientCheckpoints[i].z2;

	cg.clientCheckpoints[i].isSet = qtrue;

	Com_Printf("Checkpoint added.\n");

	for (i=0; i<3; i++) {
		if (mins[i] == maxs[i])
			Com_Printf("Error: Checkpoint bounding box has no volume!\n");
	}

	//Com_Printf("Drawing cube %.1f %.1f %.1f, %.1f %.1f %.1f\n", mins[0], mins[1], mins[2], maxs[0], maxs[1], maxs[2]);

	CG_CubeOutline( mins, maxs, 8000, COLOR_RED, 0.25 );
}

static void CG_DeleteCheckpoint_f(void) //This should reorder them so no empty checkpoints in the middle of set checkpoints?
{
	char arg[8];
	int i;

	if (trap->Cmd_Argc() != 2) {
		Com_Printf("Usage: /deleteCheckpoint <checkpoint #>\n");
		return;
	}

	trap->Cmd_Argv( 1, arg, sizeof(arg) );
	i = atoi(arg);

	if (i < 0 || i >= MAX_CLIENT_CHECKPOINTS) {
		Com_Printf("Checkpoint #%i is out of range!\n", i);
		return;
	}

	if (!cg.clientCheckpoints[i].isSet) {
		Com_Printf("Checkpoint #%i is already empty!\n", i);
		return;
	}

	cg.clientCheckpoints[i].x1 = cg.clientCheckpoints[i].y1 = cg.clientCheckpoints[i].z1 = cg.clientCheckpoints[i].x2 = cg.clientCheckpoints[i].y2 = cg.clientCheckpoints[i].z2 = 0;
	cg.clientCheckpoints[i].isSet = qfalse;

	Com_Printf("Checkpoint deleted.\n");
}

static void CG_ListCheckpoints_f(void)
{
	int i;

	if (trap->Cmd_Argc() != 1) {
		Com_Printf("Usage: /listCheckpoints\n");
		return;
	}

	Com_Printf("^5#  Bounding Box\n");

	for (i=0; i<MAX_CLIENT_CHECKPOINTS; i++) { //Add a checkpoint to the first available slot
		if (cg.clientCheckpoints[i].isSet)
			Com_Printf("^5%i^3: ^3(^7%i %i %i^3, ^7%i %i %i^3)\n",
				i, cg.clientCheckpoints[i].x1, cg.clientCheckpoints[i].y1, cg.clientCheckpoints[i].z1, cg.clientCheckpoints[i].x2, cg.clientCheckpoints[i].y2, cg.clientCheckpoints[i].z2);
	}
}

static void CG_TeleToCheckpoint_f(void)
{
	char arg[8];
	int i;
	int midX, midY, z;

	if (trap->Cmd_Argc() != 2) {
		Com_Printf("Usage: /teleToCheckpoint <checkpoint #>\n");
		return;
	}

	trap->Cmd_Argv( 1, arg, sizeof(arg) );
	i = atoi(arg);

	if (i < 0 || i >= MAX_CLIENT_CHECKPOINTS) {
		Com_Printf("Checkpoint #%i is out of range!\n", i);
		return;
	}

	if (!cg.clientCheckpoints[i].isSet) {
		Com_Printf("Checkpoint #%i is empty!\n", i);
		return;
	}

	midX = (cg.clientCheckpoints[i].x1 + cg.clientCheckpoints[i].x2) * 0.5f;
	midY = (cg.clientCheckpoints[i].y1 + cg.clientCheckpoints[i].y2) * 0.5f;
	//midZ = (cg.clientCheckpoints[i].z1 + cg.clientCheckpoints[i].z2) * 0.5f;

	if (cg.clientCheckpoints[i].z1 < cg.clientCheckpoints[i].z2)
		z = cg.clientCheckpoints[i].z2;
	else
		z = cg.clientCheckpoints[i].z1;

	trap->SendConsoleCommand(va("amtele %i %i %i\n", midX, midY, z));
}

#if _NEWTRAILS
void CG_RemoveStrafeTrail(int clientNum);
#else
void CG_DeleteLocalEntity(int clientNum);
#endif
static void CG_DeleteStrafeTrail_f(void)
{
	int clientNum;
	char arg[8];

	if (trap->Cmd_Argc() != 2) {
		Com_Printf("Usage: /clearTrail <client #>\n");
		return;
	}

	trap->Cmd_Argv( 1, arg, sizeof(arg) );
	clientNum = atoi(arg);

	//get clientnum arg
#if _NEWTRAILS
	CG_RemoveStrafeTrail(clientNum);
#else
	CG_DeleteLocalEntity(clientNum);
#endif

}

static void CG_AddStrafeTrail_f(void)
{
	int clientNum;
	char arg[8];
	const uint32_t mask = (1 << (MAX_CLIENTS-2)) - 1; //okay?

	if (trap->Cmd_Argc() == 1) {
		int i;
		clientInfo_t *cl;

		for (i = 0, cl = cgs.clientinfo; i < cgs.maxclients; ++i, ++cl) {
			if (cl->infoValid) {
				if (cg_strafeTrailPlayers.integer & (1<<i))
					trap->Print("^5%2d^3: ^7[X] ^7%s\n", i, cl->name); 
				else
					trap->Print("^5%2d^3: ^7[ ] ^7%s\n", i, cl->name); 
			}
		}
	}
	else if (trap->Cmd_Argc() == 2) {
		trap->Cmd_Argv( 1, arg, sizeof(arg) );
		clientNum = atoi(arg);

		if (clientNum == -1) {
			if (cg_strafeTrailPlayers.integer) {
				trap->Cvar_Set( "cg_strafeTrailPlayers",  "0" );
				Com_Printf("All strafetrails stopped\n");
			}
			else {
				trap->Cvar_Set( "cg_strafeTrailPlayers",  "1073741823" );
				Com_Printf("All strafetrails added\n");
			}
		}
		else {
			trap->Cvar_Set( "cg_strafeTrailPlayers", va( "%i", (1 << clientNum) ^ (cg_strafeTrailPlayers.integer & mask ) ) );
			if (cg_strafeTrailPlayers.integer & (1<<clientNum))
				Com_Printf("Strafetrail stopped\n");
			else
				Com_Printf("Strafetrail added\n");
		}
	}
	else
		Com_Printf("Usage: /strafeTrail <client #>\n");

}

void CG_Say_f( void ) {
	char msg[MAX_SAY_TEXT] = {0};
	char word[MAX_SAY_TEXT] = {0};
	char numberStr[MAX_SAY_TEXT] = {0};
	int i, number = 0, numWords = trap->Cmd_Argc();
//	qboolean command = qfalse;

	for (i = 1; i < numWords; i++) {
		trap->Cmd_Argv( i, word, sizeof(word));

		//if (i==1 && !Q_stricmpn(word, "/", 1))
			//command = qtrue;

		if (!Q_stricmp(word, "%H%")) {
			if (pm)
				number = pm->ps->stats[STAT_HEALTH];
			Com_sprintf(numberStr, sizeof(numberStr), "%i", number);
			Q_strncpyz( word, numberStr, sizeof(word));
		}
		else if (!Q_stricmp(word, "%S%")) {
			if (pm)
				number = pm->ps->stats[STAT_ARMOR];
			Com_sprintf(numberStr, sizeof(numberStr), "%i", number);
			Q_strncpyz( word, numberStr, sizeof(word));
		}
		else if (!Q_stricmp(word, "%F%")) {
			if (pm)
				number = pm->ps->fd.forcePower;
			Com_sprintf(numberStr, sizeof(numberStr), "%i", number);
			Q_strncpyz( word, numberStr, sizeof(word));
		}
		else if (!Q_stricmp(word, "%W%")) {
			if (pm)
				number = pm->ps->weapon;
			switch (number) {
				case 1:	Com_sprintf(numberStr, sizeof(numberStr), "Stun baton"); break;
				case 2: Com_sprintf(numberStr, sizeof(numberStr), "Melee"); break;
				case 4:	Com_sprintf(numberStr, sizeof(numberStr), "Pistol"); break;
				case 5:	Com_sprintf(numberStr, sizeof(numberStr), "E11"); break;
				case 6:	Com_sprintf(numberStr, sizeof(numberStr), "Sniper"); break;
				case 7:	Com_sprintf(numberStr, sizeof(numberStr), "Bowcaster");	break;
				case 8:	Com_sprintf(numberStr, sizeof(numberStr), "Repeater"); break;
				case 9:	Com_sprintf(numberStr, sizeof(numberStr), "Demp2");	break;
				case 10: Com_sprintf(numberStr, sizeof(numberStr), "Flechette"); break;
				case 11: Com_sprintf(numberStr, sizeof(numberStr), "Rocket"); break;
				case 12: Com_sprintf(numberStr, sizeof(numberStr), "Thermal"); break;
				case 13: Com_sprintf(numberStr, sizeof(numberStr), "Tripmine"); break;
				case 14: Com_sprintf(numberStr, sizeof(numberStr), "Detpack"); break;
				case 15: Com_sprintf(numberStr, sizeof(numberStr), "Concussion rifle"); break;
				case 16: Com_sprintf(numberStr, sizeof(numberStr), "Bryar"); break;
				default: Com_sprintf(numberStr, sizeof(numberStr), "Saber"); break;
				}
			Q_strncpyz( word, numberStr, sizeof(word));
		}
		else if (!Q_stricmp(word, "%A%")) {
			if (pm)
				number = pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex];
			Com_sprintf(numberStr, sizeof(numberStr), "%i", number);
			Q_strncpyz( word, numberStr, sizeof(word));
		}

		Q_strcat(word, MAX_SAY_TEXT, " ");
		Q_strcat(msg, MAX_SAY_TEXT, word);
	}
	//if (command)
		//trap->SendClientCommand(va( "%s", msg));
	//else
		trap->SendClientCommand(va( "say %s", msg));
}

void CG_TeamSay_f( void ) { 
	char word[MAX_SAY_TEXT] = {0}, msg[MAX_SAY_TEXT] = {0}, numberStr[MAX_SAY_TEXT] = {0};//eh
	int i, number = 0, numWords = trap->Cmd_Argc();

	for (i = 1; i < numWords; i++) {
		trap->Cmd_Argv( i, word, sizeof(word));

		if (!Q_stricmp(word, "%H%")) {
			if (pm)
				number = pm->ps->stats[STAT_HEALTH];
			Com_sprintf(numberStr, sizeof(numberStr), "%i", number);
			Q_strncpyz( word, numberStr, sizeof(word));
		}
		else if (!Q_stricmp(word, "%S%")) {
			if (pm)
				number = pm->ps->stats[STAT_ARMOR];
			Com_sprintf(numberStr, sizeof(numberStr), "%i", number);
			Q_strncpyz( word, numberStr, sizeof(word));
		}
		else if (!Q_stricmp(word, "%F%")) {
			if (pm)
				number = pm->ps->fd.forcePower;
			Com_sprintf(numberStr, sizeof(numberStr), "%i", number);
			Q_strncpyz( word, numberStr, sizeof(word));
		}
		else if (!Q_stricmp(word, "%W%")) {
			if (pm)
				number = pm->ps->weapon;
			switch (number) {
				case 1:	Com_sprintf(numberStr, sizeof(numberStr), "Stun baton"); break;
				case 2: Com_sprintf(numberStr, sizeof(numberStr), "Melee"); break;
				case 4:	Com_sprintf(numberStr, sizeof(numberStr), "Pistol"); break;
				case 5:	Com_sprintf(numberStr, sizeof(numberStr), "E11"); break;
				case 6:	Com_sprintf(numberStr, sizeof(numberStr), "Sniper"); break;
				case 7:	Com_sprintf(numberStr, sizeof(numberStr), "Bowcaster");	break;
				case 8:	Com_sprintf(numberStr, sizeof(numberStr), "Repeater"); break;
				case 9:	Com_sprintf(numberStr, sizeof(numberStr), "Demp2");	break;
				case 10: Com_sprintf(numberStr, sizeof(numberStr), "Flechette"); break;
				case 11: Com_sprintf(numberStr, sizeof(numberStr), "Rocket"); break;
				case 12: Com_sprintf(numberStr, sizeof(numberStr), "Thermal"); break;
				case 13: Com_sprintf(numberStr, sizeof(numberStr), "Tripmine"); break;
				case 14: Com_sprintf(numberStr, sizeof(numberStr), "Detpack"); break;
				case 15: Com_sprintf(numberStr, sizeof(numberStr), "Concussion rifle"); break;
				case 16: Com_sprintf(numberStr, sizeof(numberStr), "Bryar"); break;
				default: Com_sprintf(numberStr, sizeof(numberStr), "Saber"); break;
				}
			Q_strncpyz( word, numberStr, sizeof(word));
		}
		else if (!Q_stricmp(word, "%A%")) {
			if (pm)
				number = pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex];
			Com_sprintf(numberStr, sizeof(numberStr), "%i", number);
			Q_strncpyz( word, numberStr, sizeof(word));
		}

		Q_strcat(word, MAX_SAY_TEXT, " ");
		Q_strcat(msg, MAX_SAY_TEXT, word);
	}

		trap->SendClientCommand(va( "say_team %s", msg));
}

typedef struct consoleCommand_s {
	const char	*cmd;
	void		(*func)(void);
} consoleCommand_t;

int cmdcmp( const void *a, const void *b ) {
	return Q_stricmp( (const char *)a, ((consoleCommand_t*)b)->cmd );
}

static consoleCommand_t	commands[] = {
	{ "+scores",					CG_ScoresDown_f },
	{ "-scores",					CG_ScoresUp_f },
	{ "briefing",					CG_SiegeBriefing_f },
	{ "clientlist",					CG_ClientList_f },
	{ "forcenext",					CG_NextForcePower_f },
	{ "forceprev",					CG_PrevForcePower_f },
	{ "invnext",					CG_NextInventory_f },
	{ "invprev",					CG_PrevInventory_f },
	{ "loaddeferred",				CG_LoadDeferredPlayers },
	{ "loadhud",					CG_LoadHud_f },
	{ "nextframe",					CG_TestModelNextFrame_f },
	{ "nextskin",					CG_TestModelNextSkin_f },
	{ "prevframe",					CG_TestModelPrevFrame_f },
	{ "prevskin",					CG_TestModelPrevSkin_f },
	{ "siegeCompleteCvarUpdate",	CG_SiegeCompleteCvarUpdate_f },
	{ "siegeCvarUpdate",			CG_SiegeCvarUpdate_f },
	{ "sizedown",					CG_SizeDown_f },
	{ "sizeup",						CG_SizeUp_f },
	{ "startOrbit",					CG_StartOrbit_f },
	{ "tcmd",						CG_TargetCommand_f },
	{ "tell_attacker",				CG_TellAttacker_f },
	{ "tell_target",				CG_TellTarget_f },
	{ "testgun",					CG_TestGun_f },
	{ "testmodel",					CG_TestModel_f },
	{ "viewpos",					CG_Viewpos_f },
	{ "weapnext",					CG_NextWeapon_f },
	{ "weapon",						CG_Weapon_f },
	{ "weaponclean",				CG_WeaponClean_f },
	{ "weapprev",					CG_PrevWeapon_f },
	{ "showPlayerId",				CG_ClientList_f },
	{ "cameraSettings",				CG_ShowSpecCamera_f },
	{ "ignoreVGS",					CG_IgnoreVGS_f },
	{ "serverconfig",				CG_ServerConfig_f },
	{ "autoLogin",					CG_Autologin_f },
	{ "+zoom",						CG_ZoomDown_f },
	{ "-zoom",						CG_ZoomUp_f },
	{ "say",						CG_Say_f },
	{ "say_team",					CG_TeamSay_f },
	{ "saber",						CG_Saber_f },
	{ "saberColor",					CG_Sabercolor_f },
	{ "amColor",					CG_Amcolor_f },
	{ "amrun",						CG_AmRun_f },
	{ "modversion",					CG_ModVersion_f },

	{ "followRedFlag",				CG_FollowRedFlag_f },
	{ "followBlueFlag",				CG_FollowBlueFlag_f },
	{ "followFastest",				CG_FollowFastest_f },

	{ "login",						CG_Login_f },

	{ "strafeHelper",				CG_StrafeHelper_f },
	{ "flipkick",					CG_Flipkick_f },
	{ "lowjump",					CG_Lowjump_f },
	{ "+duck",						CG_NorollDown_f },
	{ "-duck",						CG_NorollUp_f },
	{ "plugin",						CG_PluginDisable_f },
	{ "pluginDisable",				CG_PluginDisable_f },
	{ "stylePlayer",				CG_StylePlayer_f },
	{ "speedometer",				CG_SpeedometerSettings_f },
	{ "cosmetics",					CG_Cosmetics_f },

	{ "addSpeedsound",				CG_AddSpeedpoint_f },
	{ "listSpeedsounds",			CG_ListSpeedpoints_f },
	{ "deleteSpeedsound",			CG_DeleteSpeedpoint_f },

	{ "addCheckpoint",				CG_AddCheckpoint_f },
	{ "listCheckpoints",			CG_ListCheckpoints_f },
	{ "deleteCheckpoint",			CG_DeleteCheckpoint_f },
	{ "teleToCheckpoint",			CG_TeleToCheckpoint_f },
	{ "clearTrail",					CG_DeleteStrafeTrail_f },
	{ "strafeTrail",				CG_AddStrafeTrail_f },

	{ "PTelemark",					CG_PTelemark_f },
	{ "PTele",						CG_PTele_f },

	{ "remapShader",				CG_RemapShader_f },
	{ "listRemaps",					CG_ListRemaps_f },
	{ "loadTrail",					CG_SpawnStrafeTrailFromCFG_f },
	{ "weaplast",					CG_LastWeapon_f },
	{ "do",							CG_Do_f }
};

static const size_t numCommands = ARRAY_LEN( commands );

/*
=================
CG_ConsoleCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
qboolean CG_ConsoleCommand( void ) {
	consoleCommand_t	*command = NULL;

	command = (consoleCommand_t *)Q_LinearSearch( CG_Argv( 0 ), commands, numCommands, sizeof( commands[0] ), cmdcmp );

	if ( !command || !command->func )
		return qfalse;

	command->func();
	return qtrue;
}

static const char *gcmds[] = {
	"addbot",
	"callteamvote",
	"callvote",
	"duelteam",
	"follow",
	"follownext",
	"followprev",
	"forcechanged",
	"give",
	"god",
	"kill",
	"levelshot",
	"loaddefered",
	"noclip",
	"notarget",
	"NPC",
	"say",
	"say_team",
	"setviewpos",
	"siegeclass",
	"stats",
	//"stopfollow",
	"team",
	"teamtask",
	"teamvote",
	"tell",
	"voice_cmd",
	"vote",
	"where",
	"zoom",
	"clanSay",
	"amSay",
	"amLogin",
	"clanPass",
	"clanJoin",
	"clanCreate",
	"clanAdmin",
	"clanInvite",
	"clanLeave",
	"clanList",
	"clanInfo",
	"clanWhoIs",
	"say_team_mod",
	"master",
	"masterlist",
	"amForceTeam",
	"amLockTeam",
	"amWhois",
	"amStatus",
	"amKick",
	"amBan",
	"amVstr",
	"ignore",
	"duelWhois",
	"amMotd",
	"amInfo",
	"amMerc",
	"amEmpower",
	"amProtect",
	"amRename",
	"npc",
	"amTele",
	"amTeleMark",
	"amSleep",
	"amWake",
	"amSlap",
	"amSilence",
	"amKillVote",
	"amLogout",
	"amPoll",
	"amShowMotd",
	"origin",
	"amGhost",
	"amDenyVote",
	"amMindTrick",
	"amSeeGhost",
	"amMap",
	"ampSay",
	"amWeather",
	"amForceAltDim",
	"amUnForceAltDim",
	"admCmdNext",
	"admCmdPrev",
	"admCmdExe",
	"playerPrev",
	"playerNext",
	"amColor",
	"amDmgs",
	"amMove",
	"amAltDim",
	"amFreeze",
	"engage_fullforceduel",
	"engage_gunduel",

	"amBeg",
	"amBeg2",
	"amBreakDance",
	"amBreakDance2",
	"amBreakDance3",
	"amBreakDance4",
	"amCheer",
	"amCower",
	"amDance",
	"amHug",
	"amNoisy",
	"amPoint",
	"amRage",
	"amSit",
	"amSit2",
	"amSit3",
	"amSit4",
	"amSit5",
	"amSurrender",
	"amSmack",
	"amTaunt",
	"amTaunt2",
	"amVictory",
	"amGrantAdmin",
	"amListMaps",
	"race",
	"warp",
	"warpList",

	"register",
	"login",
	"logout",
	"rHardest",
	"rTop",
	"rRank",
	"rLatest",
	"rWorst",
	"rPopular",
	"rFind",
	"whois",
	"changePassword",
	"amLookup",
	"best",
	"rocketChange",
	"hide",
	"move",
	"notCompleted",
	"spot",
	"jump",
	"printStats",
	"showNet",
	"practice",
	"launch",
	"ysal",

	"vgs_cmd",

	"top",
	"saberColor",
};
static const size_t numgcmds = ARRAY_LEN( gcmds );

/*
=================
CG_InitConsoleCommands

Let the client system know about all of our commands
so it can perform tab completion
=================
*/
void CG_InitConsoleCommands( void ) {
	size_t i;

	for ( i = 0; i < numCommands; i++ )
		trap->AddCommand( commands[i].cmd );

	//
	// the game server will interpret these commands, which will be automatically
	// forwarded to the server after they are not recognized locally
	//
	for( i = 0; i < numgcmds; i++ )
		trap->AddCommand( gcmds[i] );
}
