// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_servercmds.c -- reliably sequenced text commands sent by the server
// these are processed at snapshot transition time, so there will definately
// be a valid snapshot this frame

#include "cg_local.h"
#include "ui/menudef.h"
#if !defined(CL_LIGHT_H_INC)
	#include "cg_lights.h"
#endif
#include "ghoul2/G2.h"
#include "ui/ui_public.h"

/*
=================
CG_ParseScores

=================
*/
static void CG_ParseScores( void ) {
	int		i, powerups, readScores;

	cg.numScores = atoi( CG_Argv( 1 ) );

	readScores = cg.numScores;

	if (readScores > MAX_CLIENT_SCORE_SEND)
	{
		readScores = MAX_CLIENT_SCORE_SEND;
	}

	if ( cg.numScores > MAX_CLIENTS ) {
		cg.numScores = MAX_CLIENTS;
	}

	cg.numScores = readScores;

	cg.teamScores[0] = atoi( CG_Argv( 2 ) );
	cg.teamScores[1] = atoi( CG_Argv( 3 ) );

	memset( cg.scores, 0, sizeof( cg.scores ) );
	for ( i = 0 ; i < readScores ; i++ ) {
		//
		cg.scores[i].client = atoi( CG_Argv( i * 14 + 4 ) );
		cg.scores[i].score = atoi( CG_Argv( i * 14 + 5 ) );
		cg.scores[i].ping = atoi( CG_Argv( i * 14 + 6 ) );
		cg.scores[i].time = atoi( CG_Argv( i * 14 + 7 ) );
		cg.scores[i].scoreFlags = atoi( CG_Argv( i * 14 + 8 ) );
		powerups = atoi( CG_Argv( i * 14 + 9 ) );
		cg.scores[i].accuracy = atoi(CG_Argv(i * 14 + 10));
		cg.scores[i].impressiveCount = atoi(CG_Argv(i * 14 + 11));
		cg.scores[i].excellentCount = atoi(CG_Argv(i * 14 + 12));
		cg.scores[i].guantletCount = atoi(CG_Argv(i * 14 + 13));
		cg.scores[i].defendCount = atoi(CG_Argv(i * 14 + 14));
		cg.scores[i].assistCount = atoi(CG_Argv(i * 14 + 15));
		cg.scores[i].perfect = atoi(CG_Argv(i * 14 + 16));
		cg.scores[i].captures = atoi(CG_Argv(i * 14 + 17));

		if ( cg.scores[i].client < 0 || cg.scores[i].client >= MAX_CLIENTS ) {
			cg.scores[i].client = 0;
		}
		cgs.clientinfo[ cg.scores[i].client ].score = cg.scores[i].score;
		cgs.clientinfo[ cg.scores[i].client ].powerups = powerups;

		cg.scores[i].team = cgs.clientinfo[cg.scores[i].client].team;
	}
	CG_SetScoreSelection(NULL);
}

/*
=================
CG_ParseTeamInfo

=================
*/
static void CG_ParseTeamInfo( void ) {
	int		i;
	int		client;

	numSortedTeamPlayers = atoi( CG_Argv( 1 ) );

	for ( i = 0 ; i < numSortedTeamPlayers ; i++ ) {
		client = atoi( CG_Argv( i * 6 + 2 ) );

		sortedTeamPlayers[i] = client;

		cgs.clientinfo[ client ].location = atoi( CG_Argv( i * 6 + 3 ) );
		cgs.clientinfo[ client ].health = atoi( CG_Argv( i * 6 + 4 ) );
		cgs.clientinfo[ client ].armor = atoi( CG_Argv( i * 6 + 5 ) );
		cgs.clientinfo[ client ].curWeapon = atoi( CG_Argv( i * 6 + 6 ) );
		cgs.clientinfo[ client ].powerups = atoi( CG_Argv( i * 6 + 7 ) );
	}
}


/*
================
CG_ParseServerinfo

This is called explicitly when the gamestate is first received,
and whenever the server updates any serverinfo flagged cvars
================
*/
void CG_ParseServerinfo( void ) {
	const char *info = NULL, *tinfo = NULL;
	char *mapname;
	int i;

	info = CG_ConfigString( CS_SERVERINFO );

	cgs.debugMelee = atoi( Info_ValueForKey( info, "g_debugMelee" ) ); //trap_Cvar_GetHiddenVarValue("g_iknowkungfu");
	cgs.stepSlideFix = atoi( Info_ValueForKey( info, "g_stepSlideFix" ) );

	cgs.noSpecMove = atoi( Info_ValueForKey( info, "g_noSpecMove" ) );

	trap_Cvar_Set("bg_fighterAltControl", Info_ValueForKey( info, "bg_fighterAltControl" ));

	cgs.siegeTeamSwitch = atoi( Info_ValueForKey( info, "g_siegeTeamSwitch" ) );

	cgs.showDuelHealths = atoi( Info_ValueForKey( info, "g_showDuelHealths" ) );

	cgs.gametype = atoi( Info_ValueForKey( info, "g_gametype" ) );
	trap_Cvar_Set("g_gametype", va("%i", cgs.gametype));
	cgs.needpass = atoi( Info_ValueForKey( info, "needpass" ) );
	cgs.jediVmerc = atoi( Info_ValueForKey( info, "g_jediVmerc" ) );
	cgs.wDisable = atoi( Info_ValueForKey( info, "wdisable" ) );
	cgs.fDisable = atoi( Info_ValueForKey( info, "fdisable" ) );
	cgs.dmflags = atoi( Info_ValueForKey( info, "dmflags" ) );
	cgs.duel_fraglimit = atoi( Info_ValueForKey( info, "duel_fraglimit" ) );
	cgs.capturelimit = atoi( Info_ValueForKey( info, "capturelimit" ) );

	// reset fraglimit warnings
	i = atoi( Info_ValueForKey( info, "fraglimit" ) );
	if ( cgs.fraglimit < i )
		cg.fraglimitWarnings &= ~(1|2|4);
	cgs.fraglimit = i;

	// reset timelimit warnings
	i = atoi( Info_ValueForKey( info, "timelimit" ) );
	if ( cgs.timelimit != i )
		cg.timelimitWarnings &= ~(1|2);
	cgs.timelimit = i;

	cgs.maxclients = atoi( Info_ValueForKey( info, "sv_maxclients" ) );
	mapname = Info_ValueForKey( info, "mapname" );

	//rww - You must do this one here, Info_ValueForKey always uses the same memory pointer.
	trap_Cvar_Set ( "ui_about_mapname", mapname );

	Com_sprintf( cgs.mapname, sizeof( cgs.mapname ), "maps/%s.bsp", mapname );
//	Q_strncpyz( cgs.redTeam, Info_ValueForKey( info, "g_redTeam" ), sizeof(cgs.redTeam) );
//	trap_Cvar_Set("g_redTeam", cgs.redTeam);
//	Q_strncpyz( cgs.blueTeam, Info_ValueForKey( info, "g_blueTeam" ), sizeof(cgs.blueTeam) );
//	trap_Cvar_Set("g_blueTeam", cgs.blueTeam);

	trap_Cvar_Set ( "ui_about_gametype", va("%i", cgs.gametype ) );
	trap_Cvar_Set ( "ui_about_fraglimit", va("%i", cgs.fraglimit ) );
	trap_Cvar_Set ( "ui_about_duellimit", va("%i", cgs.duel_fraglimit ) );
	trap_Cvar_Set ( "ui_about_capturelimit", va("%i", cgs.capturelimit ) );
	trap_Cvar_Set ( "ui_about_timelimit", va("%i", cgs.timelimit ) );
	trap_Cvar_Set ( "ui_about_maxclients", va("%i", cgs.maxclients ) );
	trap_Cvar_Set ( "ui_about_dmflags", va("%i", cgs.dmflags ) );
	trap_Cvar_Set ( "ui_about_hostname", Info_ValueForKey( info, "sv_hostname" ) );
	trap_Cvar_Set ( "ui_about_needpass", Info_ValueForKey( info, "g_needpass" ) );
	trap_Cvar_Set ( "ui_about_botminplayers", Info_ValueForKey ( info, "bot_minplayers" ) );

	//Set the siege teams based on what the server has for overrides.
	trap_Cvar_Set("cg_siegeTeam1", Info_ValueForKey(info, "g_siegeTeam1"));
	trap_Cvar_Set("cg_siegeTeam2", Info_ValueForKey(info, "g_siegeTeam2"));

	tinfo = CG_ConfigString( CS_TERRAINS + 1 );
	if ( !tinfo || !*tinfo )
	{
		cg.mInRMG = qfalse;
	}
	else
	{
		int weather = 0;

		cg.mInRMG = qtrue;
		trap_Cvar_Set("RMG", "1");

		weather = atoi( Info_ValueForKey( info, "RMG_weather" ) );

		trap_Cvar_Set("RMG_weather", va("%i", weather));

		if (weather == 1 || weather == 2)
		{
			cg.mRMGWeather = qtrue;
		}
		else
		{
			cg.mRMGWeather = qfalse;
		}
	}

	//Raz: Fix bogus vote strings
	Q_strncpyz( cgs.voteString, CG_ConfigString( CS_VOTE_STRING ), sizeof( cgs.voteString ) );
}

/*
==================
CG_ParseWarmup
==================
*/
static void CG_ParseWarmup( void ) {
	const char	*info;
	int			warmup;

	info = CG_ConfigString( CS_WARMUP );

	warmup = atoi( info );
	cg.warmupCount = -1;

	cg.warmup = warmup;
}

//Raz: This is a reverse map of flag statuses as seen in g_team.c
//static char ctfFlagStatusRemap[] = { '0', '1', '*', '*', '2' };
static char ctfFlagStatusRemap[] = { 	
	FLAG_ATBASE,
	FLAG_TAKEN,			// CTF
	// server doesn't use FLAG_TAKEN_RED or FLAG_TAKEN_BLUE
	// which was originally for 1-flag CTF.
	FLAG_DROPPED
};

/*
================
CG_SetConfigValues

Called on load to set the initial values from configure strings
================
*/
void CG_SetConfigValues( void ) 
{
	const char *s;
	const char *str;

	cgs.scores1 = atoi( CG_ConfigString( CS_SCORES1 ) );
	cgs.scores2 = atoi( CG_ConfigString( CS_SCORES2 ) );
	cgs.levelStartTime = atoi( CG_ConfigString( CS_LEVEL_START_TIME ) );
	if( cgs.gametype == GT_CTF || cgs.gametype == GT_CTY ) {
		int redflagId = 0, blueflagId = 0;

		s = CG_ConfigString( CS_FLAGSTATUS );

		redflagId = s[0] - '0';
		blueflagId = s[1] - '0';

		// fix: proper flag statuses mapping for dropped flag
		if ( redflagId >= 0 && redflagId < ARRAY_LEN( ctfFlagStatusRemap ) ) 
			cgs.redflag = ctfFlagStatusRemap[redflagId];

		if ( blueflagId >= 0 && blueflagId < ARRAY_LEN( ctfFlagStatusRemap ) ) 
			cgs.blueflag = ctfFlagStatusRemap[blueflagId];
	}
	cg.warmup = atoi( CG_ConfigString( CS_WARMUP ) );

	// Track who the jedi master is
	cgs.jediMaster = atoi ( CG_ConfigString ( CS_CLIENT_JEDIMASTER ) );
	cgs.duelWinner = atoi ( CG_ConfigString ( CS_CLIENT_DUELWINNER ) );

	str = CG_ConfigString(CS_CLIENT_DUELISTS);

	if (str && str[0])
	{
		char buf[64];
		int c = 0;
		int i = 0;

		while (str[i] && str[i] != '|')
		{
			buf[c] = str[i];
			c++;
			i++;
		}
		buf[c] = 0;

		cgs.duelist1 = atoi ( buf );
		c = 0;

		i++;
		while (str[i])
		{
			buf[c] = str[i];
			c++;
			i++;
		}
		buf[c] = 0;

		cgs.duelist2 = atoi ( buf );
	}
}

/*
=====================
CG_ShaderStateChanged
=====================
*/
void CG_ShaderStateChanged(void) {
	char originalShader[MAX_QPATH];
	char newShader[MAX_QPATH];
	char timeOffset[16];
	const char *o;
	char *n,*t;

	o = CG_ConfigString( CS_SHADERSTATE );
	while (o && *o) {
		n = strstr(o, "=");
		if (n && *n) {
			strncpy(originalShader, o, n-o);
			originalShader[n-o] = 0;
			n++;
			t = strstr(n, ":");
			if (t && *t) {
				strncpy(newShader, n, t-n);
				newShader[t-n] = 0;
			} else {
				break;
			}
			t++;
			o = strstr(t, "@");
			if (o) {
				strncpy(timeOffset, t, o-t);
				timeOffset[o-t] = 0;
				o++;
				trap_R_RemapShader( originalShader, newShader, timeOffset );
			}
		} else {
			break;
		}
	}
}

extern char *cg_customSoundNames[MAX_CUSTOM_SOUNDS];
extern const char *cg_customCombatSoundNames[MAX_CUSTOM_COMBAT_SOUNDS];
extern const char *cg_customExtraSoundNames[MAX_CUSTOM_EXTRA_SOUNDS];
extern const char *cg_customJediSoundNames[MAX_CUSTOM_JEDI_SOUNDS];
extern const char *cg_customDuelSoundNames[MAX_CUSTOM_DUEL_SOUNDS];

static const char *GetCustomSoundForType(int setType, int index)
{
	switch (setType)
	{
	case 1:
		return cg_customSoundNames[index];
	case 2:
		return cg_customCombatSoundNames[index];
	case 3:
		return cg_customExtraSoundNames[index];
	case 4:
		return cg_customJediSoundNames[index];
	case 5:
		return bg_customSiegeSoundNames[index];
	case 6:
		return cg_customDuelSoundNames[index];
	default:
		assert(0);
		return NULL;
	}
}

void SetCustomSoundForType(clientInfo_t *ci, int setType, int index, sfxHandle_t sfx)
{
	switch (setType)
	{
	case 1:
		ci->sounds[index] = sfx;
		break;
	case 2:
		ci->combatSounds[index] = sfx;
		break;
	case 3:
		ci->extraSounds[index] = sfx;
		break;
	case 4:
		ci->jediSounds[index] = sfx;
		break;
	case 5:
		ci->siegeSounds[index] = sfx;
		break;
	case 6:
		ci->duelSounds[index] = sfx;
		break;
	default:
		assert(0);
		break;
	}
}

static void CG_RegisterCustomSounds(clientInfo_t *ci, int setType, const char *psDir)
{
	int iTableEntries = 0;
	int i;

	switch (setType)
	{
	case 1:
		iTableEntries = MAX_CUSTOM_SOUNDS;
		break;
	case 2:
		iTableEntries = MAX_CUSTOM_COMBAT_SOUNDS;
		break;
	case 3:
		iTableEntries = MAX_CUSTOM_EXTRA_SOUNDS;
		break;
	case 4:
		iTableEntries = MAX_CUSTOM_JEDI_SOUNDS;
		break;
	case 5:
		iTableEntries = MAX_CUSTOM_SIEGE_SOUNDS;
	default:
		assert(0);
		return;
	}

	for ( i = 0 ; i<iTableEntries; i++ ) 
	{
		sfxHandle_t hSFX;
		const char *s = GetCustomSoundForType(setType, i);

		if ( !s ) 
		{
			break;
		}

		s++;
		hSFX = trap_S_RegisterSound( va("sound/chars/%s/misc/%s", psDir, s) );

		if (hSFX == 0)
		{
			char modifiedSound[MAX_QPATH];
			char *p;

			strcpy(modifiedSound, s);
			p = strchr(modifiedSound,'.');

			if (p)
			{
				char testNumber[2];
				p--;

				//before we destroy it.. we want to see if this is actually a number.
				//If it isn't a number then don't try decrementing and registering as
				//it will only cause a disk hit (we don't try precaching such files)
				testNumber[0] = *p;
				testNumber[1] = 0;
				if (atoi(testNumber))
				{
					*p = 0;

					strcat(modifiedSound, "1.wav");

					hSFX = trap_S_RegisterSound( va("sound/chars/%s/misc/%s", psDir, modifiedSound) );
				}
			}
		}
		
		SetCustomSoundForType(ci, setType, i, hSFX);
	}
}

void CG_PrecacheNPCSounds(const char *str)
{
	char sEnd[MAX_QPATH];
	char pEnd[MAX_QPATH];
	int i = 0;
	int j = 0;
	int k = 0;

	k = 2;

	while (str[k])
	{
		pEnd[k-2] = str[k];
		k++;
	}
	pEnd[k-2] = 0;

	while (i < 4) //4 types
	{ //It would be better if we knew what type this actually was (extra, combat, jedi, etc).
	  //But that would require extra configstring indexing and that is a bad thing.

		while (j < MAX_CUSTOM_SOUNDS)
		{
			const char *s = GetCustomSoundForType(i+1, j);

			if (s && s[0])
			{ //whatever it is, try registering it under this folder.
				k = 1;
				while (s[k])
				{
					sEnd[k-1] = s[k];
					k++;
				}
				sEnd[k-1] = 0;

				trap_S_ShutUp(qtrue);
				trap_S_RegisterSound( va("sound/chars/%s/misc/%s", pEnd, sEnd) );
				trap_S_ShutUp(qfalse);
			}
			else
			{ //move onto the next set
				break;
			}

			j++;
		}

		j = 0;
		i++;
	}
}

void CG_HandleNPCSounds(centity_t *cent)
{
	if (!cent->npcClient)
	{
		return;
	}

	//standard
	if (cent->currentState.csSounds_Std)
	{
		const char *s = CG_ConfigString( CS_SOUNDS + cent->currentState.csSounds_Std );

		if (s && s[0])
		{
			char sEnd[MAX_QPATH];
			int i = 2;
			int j = 0;

			//Parse past the initial "*" which indicates this is a custom sound, and the $ which indicates
			//it is an NPC custom sound dir.
			while (s[i])
			{
				sEnd[j] = s[i];
				j++;
				i++;
			}
			sEnd[j] = 0;

			CG_RegisterCustomSounds(cent->npcClient, 1, sEnd);
		}
	}
	else
	{
		memset(&cent->npcClient->sounds, 0, sizeof(cent->npcClient->sounds));
	}

	//combat
	if (cent->currentState.csSounds_Combat)
	{
		const char *s = CG_ConfigString( CS_SOUNDS + cent->currentState.csSounds_Combat );

		if (s && s[0])
		{
			char sEnd[MAX_QPATH];
			int i = 2;
			int j = 0;

			//Parse past the initial "*" which indicates this is a custom sound, and the $ which indicates
			//it is an NPC custom sound dir.
			while (s[i])
			{
				sEnd[j] = s[i];
				j++;
				i++;
			}
			sEnd[j] = 0;

			CG_RegisterCustomSounds(cent->npcClient, 2, sEnd);
		}
	}
	else
	{
		memset(&cent->npcClient->combatSounds, 0, sizeof(cent->npcClient->combatSounds));
	}

	//extra
	if (cent->currentState.csSounds_Extra)
	{
		const char *s = CG_ConfigString( CS_SOUNDS + cent->currentState.csSounds_Extra );

		if (s && s[0])
		{
			char sEnd[MAX_QPATH];
			int i = 2;
			int j = 0;

			//Parse past the initial "*" which indicates this is a custom sound, and the $ which indicates
			//it is an NPC custom sound dir.
			while (s[i])
			{
				sEnd[j] = s[i];
				j++;
				i++;
			}
			sEnd[j] = 0;

			CG_RegisterCustomSounds(cent->npcClient, 3, sEnd);
		}
	}
	else
	{
		memset(&cent->npcClient->extraSounds, 0, sizeof(cent->npcClient->extraSounds));
	}

	//jedi
	if (cent->currentState.csSounds_Jedi)
	{
		const char *s = CG_ConfigString( CS_SOUNDS + cent->currentState.csSounds_Jedi );

		if (s && s[0])
		{
			char sEnd[MAX_QPATH];
			int i = 2;
			int j = 0;

			//Parse past the initial "*" which indicates this is a custom sound, and the $ which indicates
			//it is an NPC custom sound dir.
			while (s[i])
			{
				sEnd[j] = s[i];
				j++;
				i++;
			}
			sEnd[j] = 0;

			CG_RegisterCustomSounds(cent->npcClient, 4, sEnd);
		}
	}
	else
	{
		memset(&cent->npcClient->jediSounds, 0, sizeof(cent->npcClient->jediSounds));
	}
}

int CG_HandleAppendedSkin(char *modelName);
void CG_CacheG2AnimInfo(char *modelName);

// nmckenzie: DUEL_HEALTH - fixme - we could really clean this up immensely with some helper functions.
void SetDuelistHealthsFromConfigString ( const char *str ) {
	char buf[64];
	int c = 0;
	int i = 0;

	while (str[i] && str[i] != '|')
	{
		buf[c] = str[i];
		c++;
		i++;
	}
	buf[c] = 0;

	cgs.duelist1health = atoi ( buf );

	c = 0;
	i++;
	while (str[i] && str[i] != '|')
	{
		buf[c] = str[i];
		c++;
		i++;
	}
	buf[c] = 0;

	cgs.duelist2health = atoi ( buf );

	c = 0;
	i++;
	if ( str[i] == '!' )
	{	// we only have 2 duelists, apparently.
		cgs.duelist3health = -1;
		return;
	}

	while (str[i] && str[i] != '|')
	{
		buf[c] = str[i];
		c++;
		i++;
	}
	buf[c] = 0;

	cgs.duelist3health = atoi ( buf );
}

/*
================
CG_ConfigStringModified

================
*/
extern int cgSiegeRoundState;
extern int cgSiegeRoundTime;
void CG_ParseSiegeObjectiveStatus(const char *str);
void CG_ParseWeatherEffect(const char *str);
extern void CG_ParseSiegeState(const char *str); //cg_main.c
extern int cg_beatingSiegeTime;
extern int cg_siegeWinTeam;
static void CG_ConfigStringModified( void ) {
	const char	*str;
	int		num;

	num = atoi( CG_Argv( 1 ) );

	// get the gamestate from the client system, which will have the
	// new configstring already integrated
	trap_GetGameState( &cgs.gameState );

	// look up the individual string that was modified
	str = CG_ConfigString( num );

	// do something with it if necessary
	if ( num == CS_MUSIC ) {
		CG_StartMusic( qtrue );
	} else if ( num == CS_SERVERINFO ) {
		CG_ParseServerinfo();
	} else if ( num == CS_WARMUP ) {
		CG_ParseWarmup();
	} else if ( num == CS_SCORES1 ) {
		cgs.scores1 = atoi( str );
	} else if ( num == CS_SCORES2 ) {
		cgs.scores2 = atoi( str );
	} else if ( num == CS_CLIENT_JEDIMASTER ) {
		cgs.jediMaster = atoi ( str );
	}
	else if ( num == CS_CLIENT_DUELWINNER )
	{
		cgs.duelWinner = atoi ( str );
	}
	else if ( num == CS_CLIENT_DUELISTS )
	{
		char buf[64];
		int c = 0;
		int i = 0;

		while (str[i] && str[i] != '|')
		{
			buf[c] = str[i];
			c++;
			i++;
		}
		buf[c] = 0;

		cgs.duelist1 = atoi ( buf );
		c = 0;

		i++;
		while (str[i] && str[i] != '|')
		{
			buf[c] = str[i];
			c++;
			i++;
		}
		buf[c] = 0;

		cgs.duelist2 = atoi ( buf );

		if (str[i])
		{
			c = 0;
			i++;

			while (str[i])
			{
				buf[c] = str[i];
				c++;
				i++;
			}
			buf[c] = 0;

			cgs.duelist3 = atoi(buf);
		}
	}
	else if ( num == CS_CLIENT_DUELHEALTHS ) {	// nmckenzie: DUEL_HEALTH
		SetDuelistHealthsFromConfigString(str);
	}
	else if ( num == CS_LEVEL_START_TIME ) {
		cgs.levelStartTime = atoi( str );
	} else if ( num == CS_VOTE_TIME ) {
		cgs.voteTime = atoi( str );
		cgs.voteModified = qtrue;
	} else if ( num == CS_VOTE_YES ) {
		cgs.voteYes = atoi( str );
		cgs.voteModified = qtrue;
	} else if ( num == CS_VOTE_NO ) {
		cgs.voteNo = atoi( str );
		cgs.voteModified = qtrue;
	} else if ( num == CS_VOTE_STRING ) {
		Q_strncpyz( cgs.voteString, str, sizeof( cgs.voteString ) );
	} else if ( num >= CS_TEAMVOTE_TIME && num <= CS_TEAMVOTE_TIME + 1) {
		cgs.teamVoteTime[num-CS_TEAMVOTE_TIME] = atoi( str );
		cgs.teamVoteModified[num-CS_TEAMVOTE_TIME] = qtrue;
	} else if ( num >= CS_TEAMVOTE_YES && num <= CS_TEAMVOTE_YES + 1) {
		cgs.teamVoteYes[num-CS_TEAMVOTE_YES] = atoi( str );
		cgs.teamVoteModified[num-CS_TEAMVOTE_YES] = qtrue;
	} else if ( num >= CS_TEAMVOTE_NO && num <= CS_TEAMVOTE_NO + 1) {
		cgs.teamVoteNo[num-CS_TEAMVOTE_NO] = atoi( str );
		cgs.teamVoteModified[num-CS_TEAMVOTE_NO] = qtrue;
	} else if ( num >= CS_TEAMVOTE_STRING && num <= CS_TEAMVOTE_STRING + 1) {
		Q_strncpyz( cgs.teamVoteString[num-CS_TEAMVOTE_STRING], str, sizeof( cgs.teamVoteString ) );
	} else if ( num == CS_INTERMISSION ) {
		cg.intermissionStarted = atoi( str );
	} else if ( num >= CS_MODELS && num < CS_MODELS+MAX_MODELS ) {
		char modelName[MAX_QPATH];
		strcpy(modelName, str);
		if (strstr(modelName, ".glm") || modelName[0] == '$')
		{ //Check to see if it has a custom skin attached.
			CG_HandleAppendedSkin(modelName);
			CG_CacheG2AnimInfo(modelName);
		}

		if (modelName[0] != '$' && modelName[0] != '@')
		{ //don't register vehicle names and saber names as models.
			cgs.gameModels[ num-CS_MODELS ] = trap_R_RegisterModel( modelName );
		}
		else
		{
            cgs.gameModels[ num-CS_MODELS ] = 0;
		}
// GHOUL2 Insert start
		/*
	} else if ( num >= CS_CHARSKINS && num < CS_CHARSKINS+MAX_CHARSKINS ) {
		cgs.skins[ num-CS_CHARSKINS ] = trap_R_RegisterSkin( str );
		*/
		//rww - removed and replaced with CS_G2BONES
// Ghoul2 Insert end
	} else if ( num >= CS_SOUNDS && num < CS_SOUNDS+MAX_SOUNDS ) {
		if ( str[0] != '*' ) {	// player specific sounds don't register here
			cgs.gameSounds[ num-CS_SOUNDS] = trap_S_RegisterSound( str );
		}
		else if (str[1] == '$')
		{ //an NPC soundset
			CG_PrecacheNPCSounds(str);
		}
	} else if ( num >= CS_EFFECTS && num < CS_EFFECTS+MAX_FX ) {
		if (str[0] == '*')
		{ //it's a special global weather effect
			CG_ParseWeatherEffect(str);
			cgs.gameEffects[ num-CS_EFFECTS] = 0;
		}
		else
		{
			cgs.gameEffects[ num-CS_EFFECTS] = trap_FX_RegisterEffect( str );
		}
	}
	else if ( num >= CS_SIEGE_STATE && num < CS_SIEGE_STATE+1 )
	{
		if (str[0])
		{
			CG_ParseSiegeState(str);
		}
	}
	else if ( num >= CS_SIEGE_WINTEAM && num < CS_SIEGE_WINTEAM+1 )
	{
		if (str[0])
		{
			cg_siegeWinTeam = atoi(str);
		}
	}
	else if ( num >= CS_SIEGE_OBJECTIVES && num < CS_SIEGE_OBJECTIVES+1 )
	{
		CG_ParseSiegeObjectiveStatus(str);
	}
	else if (num >= CS_SIEGE_TIMEOVERRIDE && num < CS_SIEGE_TIMEOVERRIDE+1)
	{
		cg_beatingSiegeTime = atoi(str);
		CG_SetSiegeTimerCvar ( cg_beatingSiegeTime );
	}
	else if ( num >= CS_PLAYERS && num < CS_PLAYERS+MAX_CLIENTS )
	{
		CG_NewClientInfo( num - CS_PLAYERS, qtrue);
		CG_BuildSpectatorString();
	} else if ( num == CS_FLAGSTATUS ) {
		if( cgs.gametype == GT_CTF || cgs.gametype == GT_CTY ) {
			// format is rb where its red/blue, 0 is at base, 1 is taken, 2 is dropped
			int redflagId = str[0] - '0', blueflagId = str[1] - '0';

			//Raz: improved flag status remapping
			if ( redflagId >= 0 && redflagId < ARRAY_LEN( ctfFlagStatusRemap ) ) 
				cgs.redflag = ctfFlagStatusRemap[redflagId];

			if ( blueflagId >= 0 && blueflagId < ARRAY_LEN( ctfFlagStatusRemap ) )  
				cgs.blueflag = ctfFlagStatusRemap[blueflagId];
		}
	}
	else if ( num == CS_SHADERSTATE ) {
		CG_ShaderStateChanged();
	}
	else if ( num >= CS_LIGHT_STYLES && num < CS_LIGHT_STYLES + (MAX_LIGHT_STYLES * 3))
	{
		CG_SetLightstyle(num - CS_LIGHT_STYLES);
	}
		
}

//frees all ghoul2 stuff and npc stuff from a centity -rww
void CG_KillCEntityG2(int entNum)
{
	int j;
	clientInfo_t *ci = NULL;
	centity_t *cent = &cg_entities[entNum];

	if (entNum < MAX_CLIENTS)
	{
		ci = &cgs.clientinfo[entNum];
	}
	else
	{
		ci = cent->npcClient;
	}

	if (ci)
	{
		if (ci == cent->npcClient)
		{ //never going to be != cent->ghoul2, unless cent->ghoul2 has already been removed (and then this ptr is not valid)
			ci->ghoul2Model = NULL;
		}
		else if (ci->ghoul2Model == cent->ghoul2)
		{
			ci->ghoul2Model = NULL;
		}
		else if (ci->ghoul2Model && trap_G2_HaveWeGhoul2Models(ci->ghoul2Model))
		{
			trap_G2API_CleanGhoul2Models(&ci->ghoul2Model);
			ci->ghoul2Model = NULL;
		}

		//Clean up any weapon instances for custom saber stuff
		j = 0;
		while (j < MAX_SABERS)
		{
			if (ci->ghoul2Weapons[j] && trap_G2_HaveWeGhoul2Models(ci->ghoul2Weapons[j]))
			{
				trap_G2API_CleanGhoul2Models(&ci->ghoul2Weapons[j]);
				ci->ghoul2Weapons[j] = NULL;
			}

			j++;
		}
	}

	if (cent->ghoul2 && trap_G2_HaveWeGhoul2Models(cent->ghoul2))
	{
		trap_G2API_CleanGhoul2Models(&cent->ghoul2);
		cent->ghoul2 = NULL;
	}

	if (cent->grip_arm && trap_G2_HaveWeGhoul2Models(cent->grip_arm))
	{
		trap_G2API_CleanGhoul2Models(&cent->grip_arm);
		cent->grip_arm = NULL;
	}

	if (cent->frame_hold && trap_G2_HaveWeGhoul2Models(cent->frame_hold))
	{
		trap_G2API_CleanGhoul2Models(&cent->frame_hold);
		cent->frame_hold = NULL;
	}

	if (cent->npcClient)
	{
		CG_DestroyNPCClient(&cent->npcClient);
	}

	cent->isRagging = qfalse; //just in case.
	cent->ikStatus = qfalse;

	cent->localAnimIndex = 0;
}

void CG_KillCEntityInstances(void)
{
	int i = 0;
	centity_t *cent;

	while (i < MAX_GENTITIES)
	{
		cent = &cg_entities[i];

		if (i >= MAX_CLIENTS && cent->currentState.number == i)
		{ //do not clear G2 instances on client ents, they are constant
			CG_KillCEntityG2(i);
		}

		cent->bolt1 = 0;
		cent->bolt2 = 0;
		cent->bolt3 = 0;
		cent->bolt4 = 0;

		cent->bodyHeight = 0;//SABER_LENGTH_MAX;
		//cent->saberExtendTime = 0;

		cent->boltInfo = 0;

		cent->frame_minus1_refreshed = 0;
		cent->frame_minus2_refreshed = 0;
		cent->dustTrailTime = 0;
		cent->ghoul2weapon = NULL;
		//cent->torsoBolt = 0;
		cent->trailTime = 0;
		cent->frame_hold_time = 0;
		cent->frame_hold_refreshed = 0;
		cent->trickAlpha = 0;
		cent->trickAlphaTime = 0;
		VectorClear(cent->turAngles);
		cent->weapon = 0;
		cent->teamPowerEffectTime = 0;
		cent->teamPowerType = 0;
		cent->numLoopingSounds = 0;

		cent->localAnimIndex = 0;

		i++;
	}
}

/*
===============
CG_MapRestart

The server has issued a map_restart, so the next snapshot
is completely new and should not be interpolated to.

A tournement restart will clear everything, but doesn't
require a reload of all the media
===============
*/
static void CG_MapRestart( void ) {
	if ( cg_showMiss.integer ) {
		CG_Printf( "CG_MapRestart\n" );
	}

	trap_R_ClearDecals ( );
	//FIXME: trap_FX_Reset?

	CG_InitLocalEntities();
	CG_InitMarkPolys();
	CG_ClearParticles ();
	CG_KillCEntityInstances();

	// make sure the "3 frags left" warnings play again
	cg.fraglimitWarnings = 0;

	cg.timelimitWarnings = 0;

	cg.intermissionStarted = qfalse;

	cgs.voteTime = 0;

	cg.mapRestart = qtrue;

	CG_StartMusic(qtrue);

	trap_S_ClearLoopingSounds();

	// we really should clear more parts of cg here and stop sounds

	// play the "fight" sound if this is a restart without warmup
	if ( cg.warmup == 0 && cgs.gametype != GT_SIEGE && cgs.gametype != GT_POWERDUEL/* && cgs.gametype == GT_DUEL */) {
		trap_S_StartLocalSound( cgs.media.countFightSound, CHAN_ANNOUNCER );
		CG_CenterPrint( CG_GetStringEdString("MP_SVGAME", "BEGIN_DUEL"), 120, GIANTCHAR_WIDTH*2 );
	}
	/*
	if (cg_singlePlayerActive.integer) {
		trap_Cvar_Set("ui_matchStartTime", va("%i", cg.time));
		if (cg_recordSPDemo.integer && cg_recordSPDemoName.string && *cg_recordSPDemoName.string) {
			trap_SendConsoleCommand(va("set g_synchronousclients 1 ; record %s \n", cg_recordSPDemoName.string));
		}
	}
	*/
//	trap_Cvar_Set("cg_thirdPerson", "0");
}

/*
=================
CG_RemoveChatEscapeChar
=================
*/
static void CG_RemoveChatEscapeChar( char *text ) {
	int i, l;

	l = 0;
	for ( i = 0; text[i]; i++ ) {
		if (text[i] == '\x19')
			continue;
		text[l++] = text[i];
	}
	text[l] = '\0';
}

#define MAX_STRINGED_SV_STRING 1024	// this is an quake-engine limit, not a StringEd limit

void CG_CheckSVStringEdRef(char *buf, const char *str)
{ //I don't really like doing this. But it utilizes the system that was already in place.
	int i = 0;
	int b = 0;
	int strLen = 0;
	qboolean gotStrip = qfalse;

	if (!str || !str[0])
	{
		if (str)
		{
			strcpy(buf, str);
		}
		return;
	}

	strcpy(buf, str);

	strLen = strlen(str);

	if (strLen >= MAX_STRINGED_SV_STRING)
	{
		return;
	}

	while (i < strLen && str[i])
	{
		gotStrip = qfalse;

		if (str[i] == '@' && (i+1) < strLen)
		{
			if (str[i+1] == '@' && (i+2) < strLen)
			{
				if (str[i+2] == '@' && (i+3) < strLen)
				{ //@@@ should mean to insert a StringEd reference here, so insert it into buf at the current place
					char stringRef[MAX_STRINGED_SV_STRING];
					int r = 0;

					while (i < strLen && str[i] == '@')
					{
						i++;
					}

					while (i < strLen && str[i] && str[i] != ' ' && str[i] != ':' && str[i] != '.' && str[i] != '\n')
					{
						stringRef[r] = str[i];
						r++;
						i++;
					}
					stringRef[r] = 0;

					buf[b] = 0;
					Q_strcat(buf, MAX_STRINGED_SV_STRING, CG_GetStringEdString("MP_SVGAME", stringRef));
					b = strlen(buf);
				}
			}
		}

		if (!gotStrip)
		{
			buf[b] = str[i];
			b++;
		}
		i++;
	}

	buf[b] = 0;
}

static void CG_BodyQueueCopy(centity_t *cent, int clientNum, int knownWeapon)
{
	centity_t		*source;
	animation_t		*anim;
	float			animSpeed;
	int				flags=BONE_ANIM_OVERRIDE_FREEZE;
	clientInfo_t	*ci;

	if (cent->ghoul2)
	{
		trap_G2API_CleanGhoul2Models(&cent->ghoul2);
	}

	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
	{
		return;
	}

	source = &cg_entities[ clientNum ];
	ci = &cgs.clientinfo[ clientNum ];

	if (!source)
	{
		return;
	}

	if (!source->ghoul2)
	{
		return;
	}

	cent->isRagging = qfalse; //reset in case it's still set from another body that was in this cent slot.
	cent->ownerRagging = source->isRagging; //if the owner was in ragdoll state, then we want to go into it too right away.

#if 0
	VectorCopy(source->lerpOriginOffset, cent->lerpOriginOffset);
#endif

	cent->bodyFadeTime = 0;
	cent->bodyHeight = 0;

	cent->dustTrailTime = source->dustTrailTime;

	trap_G2API_DuplicateGhoul2Instance(source->ghoul2, &cent->ghoul2);

	if (source->isRagging)
	{ //just reset it now.
		source->isRagging = qfalse;
		trap_G2API_SetRagDoll(source->ghoul2, NULL); //calling with null parms resets to no ragdoll.
	}

	//either force the weapon from when we died or remove it if it was a dropped weapon
	if (knownWeapon > WP_BRYAR_PISTOL && trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), 1))
	{
		trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), 1);
	}
	else if (trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), 1))
	{
		trap_G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, knownWeapon), 0, cent->ghoul2, 1);
	}

	if (!cent->ownerRagging)
	{
		int aNum;
		int eFrame;
		qboolean fallBack = qfalse;

		//anim = &bgAllAnims[cent->localAnimIndex].anims[ cent->currentState.torsoAnim ];
		if (!BG_InDeathAnim(source->currentState.torsoAnim))
		{ //then just snap the corpse into a default
			anim = &bgAllAnims[source->localAnimIndex].anims[ BOTH_DEAD1 ];
			fallBack = qtrue;
		}
		else
		{
			anim = &bgAllAnims[source->localAnimIndex].anims[ source->currentState.torsoAnim ];
		}
		animSpeed = 50.0f / anim->frameLerp;

		if (!fallBack)
		{
			//this will just set us to the last frame of the animation, in theory
			aNum = cgs.clientinfo[source->currentState.number].frame+1;

			while (aNum >= anim->firstFrame+anim->numFrames)
			{
				aNum--;
			}

			if (aNum < anim->firstFrame-1)
			{ //wrong animation...?
				aNum = (anim->firstFrame+anim->numFrames)-1;
			}
		}
		else
		{
			aNum = anim->firstFrame;
		}

		eFrame = anim->firstFrame + anim->numFrames;

		//if (!cgs.clientinfo[source->currentState.number].frame || (cent->currentState.torsoAnim) != (source->currentState.torsoAnim) )
		//{
		//	aNum = (anim->firstFrame+anim->numFrames)-1;
		//}

		trap_G2API_SetBoneAnim(cent->ghoul2, 0, "upper_lumbar", aNum, eFrame, flags, animSpeed, cg.time, -1, 150);
		trap_G2API_SetBoneAnim(cent->ghoul2, 0, "model_root", aNum, eFrame, flags, animSpeed, cg.time, -1, 150);
		trap_G2API_SetBoneAnim(cent->ghoul2, 0, "Motion", aNum, eFrame, flags, animSpeed, cg.time, -1, 150);
	}

	//After we create the bodyqueue, regenerate any limbs on the real instance
	if (source->torsoBolt)
	{
		CG_ReattachLimb(source);
	}
}

/*
=================
CG_ServerCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
void CG_SiegeBriefingDisplay(int team, int dontshow);
void CG_ParseSiegeExtendedData(void);
extern void CG_ChatBox_AddString(char *chatStr); //cg_draw.c
static void CG_ServerCommand( void ) {
	const char	*cmd;
	char		text[MAX_SAY_TEXT];
	qboolean	IRCG = qfalse;

	cmd = CG_Argv(0);

	if ( !cmd[0] ) {
		// server claimed the command
		return;
	}

#if 0
	// never seems to get used -Ste
	if ( !strcmp( cmd, "spd" ) ) 
	{
		const char *ID;
		int holdInt,count,i;
		char string[1204];

		count = trap_Argc();

		ID =  CG_Argv(1);
		holdInt = atoi(ID);

		memset( &string, 0, sizeof( string ) );

		Com_sprintf( string,sizeof(string)," \"%s\"", (const char *) CG_Argv(2));

		for (i=3;i<count;i++)
		{
			Com_sprintf( string,sizeof(string)," %s \"%s\"", string, (const char *) CG_Argv(i));
		}

		trap_SP_Print(holdInt, (byte *)string);
		return;
	}
#endif

	if (!strcmp(cmd, "sxd"))
	{ //siege extended data, contains extra info certain classes may want to know about other clients
        CG_ParseSiegeExtendedData();
		return;
	}

	if (!strcmp(cmd, "sb"))
	{ //siege briefing display
		CG_SiegeBriefingDisplay(atoi(CG_Argv(1)), 0);
		return;
	}

	if ( !strcmp( cmd, "scl" ) )
	{
		//if (!( trap_Key_GetCatcher() & KEYCATCH_UI ))
		//Well, I want it to come up even if the briefing display is up.
		{
			trap_OpenUIMenu(UIMENU_CLASSSEL); //UIMENU_CLASSSEL
		}
		return;
	}

	if ( !strcmp( cmd, "spc" ) )
	{
		trap_Cvar_Set("ui_myteam", "3");
		trap_OpenUIMenu(UIMENU_PLAYERCONFIG); //UIMENU_CLASSSEL
		return;
	}

	if ( !strcmp( cmd, "nfr" ) )
	{ //"nfr" == "new force rank" (want a short string)
		int doMenu = 0;
		int setTeam = 0;
		int newRank = 0;

		if (trap_Argc() < 3)
		{
#ifdef _DEBUG
			Com_Printf("WARNING: Invalid newForceRank string\n");
#endif
			return;
		}

		newRank = atoi(CG_Argv(1));
		doMenu = atoi(CG_Argv(2));
		setTeam = atoi(CG_Argv(3));

		trap_Cvar_Set("ui_rankChange", va("%i", newRank));

		trap_Cvar_Set("ui_myteam", va("%i", setTeam));

		if (!( trap_Key_GetCatcher() & KEYCATCH_UI ) && doMenu)
		{
			trap_OpenUIMenu(UIMENU_PLAYERCONFIG);
		}

		return;
	}

	if ( !strcmp( cmd, "kg2" ) )
	{ //Kill a ghoul2 instance in this slot.
	  //If it has been occupied since this message was sent somehow, the worst that can (should) happen
	  //is the instance will have to reinit with its current info.
		int indexNum = 0;
		int argNum = trap_Argc();
		int i = 1;
		
		if (argNum < 1)
		{
			return;
		}

		while (i < argNum)
		{
			indexNum = atoi(CG_Argv(i));

			if (cg_entities[indexNum].ghoul2 && trap_G2_HaveWeGhoul2Models(cg_entities[indexNum].ghoul2))
			{
				if (indexNum < MAX_CLIENTS)
				{ //You try to do very bad thing!
#ifdef _DEBUG
					Com_Printf("WARNING: Tried to kill a client ghoul2 instance with a kg2 command!\n");
#endif
					return;
				}

				CG_KillCEntityG2(indexNum);
			}

			i++;
		}
		
		return;
	}

	if (!strcmp(cmd, "kls"))
	{ //kill looping sounds
		int indexNum = 0;
		int argNum = trap_Argc();
		centity_t *clent = NULL;
		centity_t *trackerent = NULL;
		
		if (argNum < 1)
		{
			assert(0);
			return;
		}

		indexNum = atoi(CG_Argv(1));

		if (indexNum != -1)
		{
			clent = &cg_entities[indexNum];
		}

		if (argNum >= 2)
		{
			indexNum = atoi(CG_Argv(2));

			if (indexNum != -1)
			{
				trackerent = &cg_entities[indexNum];
			}
		}

		if (clent)
		{
			CG_S_StopLoopingSound(clent->currentState.number, -1);
		}
		if (trackerent)
		{
			CG_S_StopLoopingSound(trackerent->currentState.number, -1);
		}

		return;
	}

	if (!strcmp(cmd, "ircg"))
	{ //this means param 2 is the body index and we want to copy to bodyqueue on it
		IRCG = qtrue;
	}

	if (!strcmp(cmd, "rcg") || IRCG)
	{ //rcg - Restore Client Ghoul (make sure limbs are reattached and ragdoll state is reset - this must be done reliably)
		int indexNum = 0;
		int argNum = trap_Argc();
		centity_t *clent;
		
		if (argNum < 1)
		{
			assert(0);
			return;
		}

		indexNum = atoi(CG_Argv(1));
		if (indexNum < 0 || indexNum >= MAX_CLIENTS)
		{
			assert(0);
			return;
		}

		clent = &cg_entities[indexNum];

		//assert(clent->ghoul2);
		if (!clent->ghoul2)
		{ //this can happen while connecting as a client
			return;
		}

#ifdef _DEBUG
		if (!trap_G2_HaveWeGhoul2Models(clent->ghoul2))
		{
			assert(!"Tried to reset state on a bad instance. Crash is inevitable.");
		}
#endif

		if (IRCG)
		{
			int bodyIndex = 0;
			int weaponIndex = 0;
			int side = 0;
			centity_t *body;

			assert(argNum >= 3);
			bodyIndex = atoi(CG_Argv(2));
			weaponIndex = atoi(CG_Argv(3));
			side = atoi(CG_Argv(4));

			body = &cg_entities[bodyIndex];

			if (side)
			{
				body->teamPowerType = qtrue; //light side
			}
			else
			{
				body->teamPowerType = qfalse; //dark side
			}

			CG_BodyQueueCopy(body, clent->currentState.number, weaponIndex);
		}

		//reattach any missing limbs
		if (clent->torsoBolt)
		{
			CG_ReattachLimb(clent);
		}

		//make sure ragdoll state is reset
		if (clent->isRagging)
		{
			clent->isRagging = qfalse;
			trap_G2API_SetRagDoll(clent->ghoul2, NULL); //calling with null parms resets to no ragdoll.
		}
		
		//clear all the decals as well
		trap_G2API_ClearSkinGore(clent->ghoul2);

		clent->weapon = 0;
		clent->ghoul2weapon = NULL; //force a weapon reinit

		return;
	}

	if ( !strcmp( cmd, "cp" ) ) {
		char strEd[MAX_STRINGED_SV_STRING];
		CG_CheckSVStringEdRef(strEd, CG_Argv(1));
		CG_CenterPrint( strEd, SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH );
		return;
	}

	if ( !strcmp( cmd, "cps" ) ) {
		char strEd[MAX_STRINGED_SV_STRING];
		char *x = (char *)CG_Argv(1);
		if (x[0] == '@')
		{
			x++;
		}
		trap_SP_GetStringTextString(x, strEd, MAX_STRINGED_SV_STRING);
		CG_CenterPrint( strEd, SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH );
		return;
	}

	if ( !strcmp( cmd, "cs" ) ) {
		CG_ConfigStringModified();
		return;
	}

	if ( !strcmp( cmd, "print" ) ) {
		char strEd[MAX_STRINGED_SV_STRING];
		CG_CheckSVStringEdRef(strEd, CG_Argv(1));
		CG_Printf( "%s", strEd );
		return;
	}

	if ( !strcmp( cmd, "chat" ) ) {
		if ( !cg_teamChatsOnly.integer ) {
			trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
			Q_strncpyz( text, CG_Argv(1), MAX_SAY_TEXT );
			CG_RemoveChatEscapeChar( text );
			CG_ChatBox_AddString(text);
			CG_Printf( "*%s\n", text );
		}
		return;
	}

	if ( !strcmp( cmd, "tchat" ) ) {
		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
		Q_strncpyz( text, CG_Argv(1), MAX_SAY_TEXT );
		CG_RemoveChatEscapeChar( text );
		CG_ChatBox_AddString(text);
		CG_Printf( "*%s\n", text );

		return;
	}

	//chat with location, possibly localized.
	if ( !strcmp( cmd, "lchat" ) ) {
		if ( !cg_teamChatsOnly.integer ) {
			char name[MAX_STRING_CHARS];
			char loc[MAX_STRING_CHARS];
			char color[8];
			char message[MAX_STRING_CHARS];

			if (trap_Argc() < 4)
			{
				return;
			}

			Q_strncpyz( name, CG_Argv( 1 ), sizeof( name ) );
			Q_strncpyz( loc, CG_Argv( 2 ), sizeof( loc ) );
			Q_strncpyz( color, CG_Argv( 3 ), sizeof( color ) );
			Q_strncpyz( message, CG_Argv( 4 ), sizeof( message ) );

			if (loc[0] == '@')
			{ //get localized text
				trap_SP_GetStringTextString(loc+1, loc, sizeof( loc ) );
			}

			trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
			//Q_strncpyz( text, CG_Argv(1), MAX_SAY_TEXT );
			Com_sprintf(text, sizeof( text ), "%s^7<%s> ^%s%s", name, loc, color, message);
			CG_RemoveChatEscapeChar( text );
			CG_ChatBox_AddString(text);
			CG_Printf( "*%s\n", text );
		}
		return;
	}
	if ( !strcmp( cmd, "ltchat" ) ) {
		char name[MAX_STRING_CHARS];
		char loc[MAX_STRING_CHARS];
		char color[8];
		char message[MAX_STRING_CHARS];

		if (trap_Argc() < 4)
		{
			return;
		}

		Q_strncpyz( name, CG_Argv( 1 ), sizeof( name ) );
		Q_strncpyz( loc, CG_Argv( 2 ), sizeof( loc ) );
		Q_strncpyz( color, CG_Argv( 3 ), sizeof( color ) );
		Q_strncpyz( message, CG_Argv( 4 ), sizeof( message ) );

		if (loc[0] == '@')
		{ //get localized text
			trap_SP_GetStringTextString(loc+1, loc, sizeof( loc ) );
		}

		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
		//Q_strncpyz( text, CG_Argv(1), MAX_SAY_TEXT );
		Com_sprintf(text, sizeof( text ), "%s^7<%s> ^%s%s", name, loc, color, message);
		CG_RemoveChatEscapeChar( text );
		CG_ChatBox_AddString(text);
		CG_Printf( "*%s\n", text );

		return;
	}

	if ( !strcmp( cmd, "scores" ) ) {
		CG_ParseScores();
		return;
	}

	if ( !strcmp( cmd, "tinfo" ) ) {
		CG_ParseTeamInfo();
		return;
	}

	if ( !strcmp( cmd, "map_restart" ) ) {
		CG_MapRestart();
		return;
	}

	//Raz: Buffer overflow fix
#if 0
	if ( Q_stricmp (cmd, "remapShader") == 0 ) {
		if (trap_Argc() == 4) {
			trap_R_RemapShader(CG_Argv(1), CG_Argv(2), CG_Argv(3));
		}
	}
#else
	if ( !Q_stricmp( cmd, "remapShader" ) )
	{
		if ( trap_Argc() == 4 )
		{
			char shader1[MAX_QPATH];
			char shader2[MAX_QPATH];
			Q_strncpyz( shader1, CG_Argv( 1 ), sizeof( shader1 ) );
			Q_strncpyz( shader2, CG_Argv( 2 ), sizeof( shader2 ) );
			trap_R_RemapShader( shader1, shader2, CG_Argv( 3 ) );
			return;
		}
		return;
	}
#endif

	// loaddeferred can be both a servercmd and a consolecmd
	if ( !strcmp( cmd, "loaddefered" ) ) {	// FIXME: spelled wrong, but not changing for demo
		CG_LoadDeferredPlayers();
		return;
	}

	// clientLevelShot is sent before taking a special screenshot for
	// the menu system during development
	if ( !strcmp( cmd, "clientLevelShot" ) ) {
		cg.levelShot = qtrue;
		return;
	}

	CG_Printf( "Unknown client game command: %s\n", cmd );
}


/*
====================
CG_ExecuteNewServerCommands

Execute all of the server commands that were received along
with this this snapshot.
====================
*/
void CG_ExecuteNewServerCommands( int latestSequence ) {
	while ( cgs.serverCommandSequence < latestSequence ) {
		if ( trap_GetServerCommand( ++cgs.serverCommandSequence ) ) {
			CG_ServerCommand();
		}
	}
}
