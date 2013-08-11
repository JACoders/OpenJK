// Copyright (C) 1999-2000 Id Software, Inc.
//
/**********************************************************************
	UI_ATOMS.C

	User interface building blocks and support functions.
**********************************************************************/
#include "ui_local.h"

qboolean		m_entersound;		// after a frame, so caching won't disrupt the sound

qboolean newUI = qfalse;


/*
=================
UI_ClampCvar
=================
*/
float UI_ClampCvar( float min, float max, float value )
{
	if ( value < min ) return min;
	if ( value > max ) return max;
	return value;
}

/*
=================
UI_StartDemoLoop
=================
*/
void UI_StartDemoLoop( void ) {
	trap->Cmd_ExecuteText( EXEC_APPEND, "d1\n" );
}


char *UI_Argv( int arg ) {
	static char	buffer[MAX_STRING_CHARS];

	trap->Cmd_Argv( arg, buffer, sizeof( buffer ) );

	return buffer;
}


char *UI_Cvar_VariableString( const char *var_name ) {
	static char	buffer[MAX_STRING_CHARS];

	trap->Cvar_VariableStringBuffer( var_name, buffer, sizeof( buffer ) );

	return buffer;
}



void UI_SetBestScores(postGameInfo_t *newInfo, qboolean postGame) {
	trap->Cvar_Set( "ui_scoreAccuracy",		va( "%i%%", newInfo->accuracy ) );
	trap->Cvar_Set( "ui_scoreImpressives",	va( "%i", newInfo->impressives ) );
	trap->Cvar_Set( "ui_scoreExcellents", 	va( "%i", newInfo->excellents ) );
	trap->Cvar_Set( "ui_scoreDefends", 		va( "%i", newInfo->defends ) );
	trap->Cvar_Set( "ui_scoreAssists", 		va( "%i", newInfo->assists ) );
	trap->Cvar_Set( "ui_scoreGauntlets", 		va( "%i", newInfo->gauntlets ) );
	trap->Cvar_Set( "ui_scoreScore", 			va( "%i", newInfo->score ) );
	trap->Cvar_Set( "ui_scorePerfect",	 	va( "%i", newInfo->perfects ) );
	trap->Cvar_Set( "ui_scoreTeam",			va( "%i to %i", newInfo->redScore, newInfo->blueScore ) );
	trap->Cvar_Set( "ui_scoreBase",			va( "%i", newInfo->baseScore ) );
	trap->Cvar_Set( "ui_scoreTimeBonus",		va( "%i", newInfo->timeBonus ) );
	trap->Cvar_Set( "ui_scoreSkillBonus",		va( "%i", newInfo->skillBonus ) );
	trap->Cvar_Set( "ui_scoreShutoutBonus",	va( "%i", newInfo->shutoutBonus ) );
	trap->Cvar_Set( "ui_scoreTime",			va( "%02i:%02i", newInfo->time / 60, newInfo->time % 60 ) );
	trap->Cvar_Set( "ui_scoreCaptures",		va( "%i", newInfo->captures ) );
  if (postGame) {
		trap->Cvar_Set( "ui_scoreAccuracy2",		va( "%i%%", newInfo->accuracy ) );
		trap->Cvar_Set( "ui_scoreImpressives2",	va( "%i", newInfo->impressives ) );
		trap->Cvar_Set( "ui_scoreExcellents2", 	va( "%i", newInfo->excellents ) );
		trap->Cvar_Set( "ui_scoreDefends2", 		va( "%i", newInfo->defends ) );
		trap->Cvar_Set( "ui_scoreAssists2", 		va( "%i", newInfo->assists ) );
		trap->Cvar_Set( "ui_scoreGauntlets2", 	va( "%i", newInfo->gauntlets ) );
		trap->Cvar_Set( "ui_scoreScore2", 		va( "%i", newInfo->score ) );
		trap->Cvar_Set( "ui_scorePerfect2",	 	va( "%i", newInfo->perfects ) );
		trap->Cvar_Set( "ui_scoreTeam2",			va( "%i to %i", newInfo->redScore, newInfo->blueScore ) );
		trap->Cvar_Set( "ui_scoreBase2",			va( "%i", newInfo->baseScore ) );
		trap->Cvar_Set( "ui_scoreTimeBonus2",		va( "%i", newInfo->timeBonus ) );
		trap->Cvar_Set( "ui_scoreSkillBonus2",	va( "%i", newInfo->skillBonus ) );
		trap->Cvar_Set( "ui_scoreShutoutBonus2",	va( "%i", newInfo->shutoutBonus ) );
		trap->Cvar_Set( "ui_scoreTime2",			va( "%02i:%02i", newInfo->time / 60, newInfo->time % 60 ) );
		trap->Cvar_Set( "ui_scoreCaptures2",		va( "%i", newInfo->captures ) );
	}
}

void UI_LoadBestScores(const char *map, int game) {
	char		fileName[MAX_QPATH];
	fileHandle_t f;
	postGameInfo_t newInfo;
	memset(&newInfo, 0, sizeof(postGameInfo_t));
	Com_sprintf(fileName, MAX_QPATH, "games/%s_%i.game", map, game);
	if (trap->FS_Open(fileName, &f, FS_READ) >= 0) {
		int size = 0;
		trap->FS_Read(&size, sizeof(int), f);
		if (size == sizeof(postGameInfo_t)) {
			trap->FS_Read(&newInfo, sizeof(postGameInfo_t), f);
		}
		trap->FS_Close(f);
	}
	UI_SetBestScores(&newInfo, qfalse);

	Com_sprintf(fileName, MAX_QPATH, "demos/%s_%d.dm_%d", map, game, (int)trap->Cvar_VariableValue("protocol"));
	uiInfo.demoAvailable = qfalse;
	if (trap->FS_Open(fileName, &f, FS_READ) >= 0) {
		uiInfo.demoAvailable = qtrue;
		trap->FS_Close(f);
	} 
}

/*
===============
UI_ClearScores
===============
*/
void UI_ClearScores() {
	char	gameList[4096];
	char *gameFile;
	int		i, len, count, size;
	fileHandle_t f;
	postGameInfo_t newInfo;

	count = trap->FS_GetFileList( "games", "game", gameList, sizeof(gameList) );

	size = sizeof(postGameInfo_t);
	memset(&newInfo, 0, size);

	if (count > 0) {
		gameFile = gameList;
		for ( i = 0; i < count; i++ ) {
			len = strlen(gameFile);
			if (trap->FS_Open(va("games/%s",gameFile), &f, FS_WRITE) >= 0) {
				trap->FS_Write(&size, sizeof(int), f);
				trap->FS_Write(&newInfo, size, f);
				trap->FS_Close(f);
			}
			gameFile += len + 1;
		}
	}
	
	UI_SetBestScores(&newInfo, qfalse);

}



static void	UI_Cache_f() {
	int i;
	Display_CacheAll();
	if (trap->Cmd_Argc() == 2) {
		for (i = 0; i < uiInfo.q3HeadCount; i++)
		{
			trap->Print( va("model %s\n", uiInfo.q3HeadNames[i]) );
		}
	}
}

/*
=======================
UI_CalcPostGameStats
=======================
*/
static void UI_CalcPostGameStats() {
	char		map[MAX_QPATH];
	char		fileName[MAX_QPATH];
	char		info[MAX_INFO_STRING];
	fileHandle_t f;
	int size, game, time, adjustedTime;
	postGameInfo_t oldInfo;
	postGameInfo_t newInfo;
	qboolean newHigh = qfalse;

	trap->GetConfigString( CS_SERVERINFO, info, sizeof(info) );
	Q_strncpyz( map, Info_ValueForKey( info, "mapname" ), sizeof(map) );
	game = atoi(Info_ValueForKey(info, "g_gametype"));

	// compose file name
	Com_sprintf(fileName, MAX_QPATH, "games/%s_%i.game", map, game);
	// see if we have one already
	memset(&oldInfo, 0, sizeof(postGameInfo_t));
	if (trap->FS_Open(fileName, &f, FS_READ) >= 0) {
	// if so load it
		size = 0;
		trap->FS_Read(&size, sizeof(int), f);
		if (size == sizeof(postGameInfo_t)) {
			trap->FS_Read(&oldInfo, sizeof(postGameInfo_t), f);
		}
		trap->FS_Close(f);
	}					 

	newInfo.accuracy = atoi(UI_Argv(3));
	newInfo.impressives = atoi(UI_Argv(4));
	newInfo.excellents = atoi(UI_Argv(5));
	newInfo.defends = atoi(UI_Argv(6));
	newInfo.assists = atoi(UI_Argv(7));
	newInfo.gauntlets = atoi(UI_Argv(8));
	newInfo.baseScore = atoi(UI_Argv(9));
	newInfo.perfects = atoi(UI_Argv(10));
	newInfo.redScore = atoi(UI_Argv(11));
	newInfo.blueScore = atoi(UI_Argv(12));
	time = atoi(UI_Argv(13));
	newInfo.captures = atoi(UI_Argv(14));

	newInfo.time = (time - trap->Cvar_VariableValue("ui_matchStartTime")) / 1000;
	adjustedTime = uiInfo.mapList[ui_currentMap.integer].timeToBeat[game];
	if (newInfo.time < adjustedTime) { 
		newInfo.timeBonus = (adjustedTime - newInfo.time) * 10;
	} else {
		newInfo.timeBonus = 0;
	}

	if (newInfo.redScore > newInfo.blueScore && newInfo.blueScore <= 0) {
		newInfo.shutoutBonus = 100;
	} else {
		newInfo.shutoutBonus = 0;
	}

	newInfo.skillBonus = trap->Cvar_VariableValue("g_spSkill");
	if (newInfo.skillBonus <= 0) {
		newInfo.skillBonus = 1;
	}
	newInfo.score = newInfo.baseScore + newInfo.shutoutBonus + newInfo.timeBonus;
	newInfo.score *= newInfo.skillBonus;

	// see if the score is higher for this one
	newHigh = (newInfo.redScore > newInfo.blueScore && newInfo.score > oldInfo.score);

	if  (newHigh) {
		// if so write out the new one
		uiInfo.newHighScoreTime = uiInfo.uiDC.realTime + 20000;
		if (trap->FS_Open(fileName, &f, FS_WRITE) >= 0) {
			size = sizeof(postGameInfo_t);
			trap->FS_Write(&size, sizeof(int), f);
			trap->FS_Write(&newInfo, sizeof(postGameInfo_t), f);
			trap->FS_Close(f);
		}
	}

	if (newInfo.time < oldInfo.time) {
		uiInfo.newBestTime = uiInfo.uiDC.realTime + 20000;
	}
 
	// put back all the ui overrides
	trap->Cvar_Set( "capturelimit",	UI_Cvar_VariableString( "ui_saveCaptureLimit" ) );
	trap->Cvar_Set( "fraglimit",		UI_Cvar_VariableString( "ui_saveFragLimit" ) );
	trap->Cvar_Set( "duel_fraglimit",	UI_Cvar_VariableString( "ui_saveDuelLimit" ) );
	trap->Cvar_Set( "cg_drawTimer",	UI_Cvar_VariableString( "ui_drawTimer" ) );
	trap->Cvar_Set( "g_doWarmup",		UI_Cvar_VariableString( "ui_doWarmup" ) );
	trap->Cvar_Set( "g_Warmup",		UI_Cvar_VariableString( "ui_Warmup" ) );
	trap->Cvar_Set( "sv_pure",		UI_Cvar_VariableString( "ui_pure" ) );
	trap->Cvar_Set( "g_friendlyFire",	UI_Cvar_VariableString( "ui_friendlyFire" ) );

	UI_SetBestScores(&newInfo, qtrue);
	UI_ShowPostGame(newHigh);


}


/*
=================
UI_ConsoleCommand
=================
*/
qboolean UI_ConsoleCommand( int realTime ) {
	char	*cmd;

	uiInfo.uiDC.frameTime = realTime - uiInfo.uiDC.realTime;
	uiInfo.uiDC.realTime = realTime;

	cmd = UI_Argv( 0 );

	// ensure minimum menu data is available
	//Menu_Cache();

	if ( Q_stricmp (cmd, "ui_test") == 0 ) {
		UI_ShowPostGame(qtrue);
	}

	if ( Q_stricmp (cmd, "ui_report") == 0 ) {
		UI_Report();
		return qtrue;
	}
	
	if ( Q_stricmp (cmd, "ui_load") == 0 ) {
		UI_Load();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "ui_opensiegemenu" ) == 0 ) 
	{
		if ( trap->Cvar_VariableValue ( "g_gametype" ) == GT_SIEGE )
		{
			Menus_CloseAll();
			if (Menus_ActivateByName(UI_Argv(1)))
			{
				trap->Key_SetCatcher( KEYCATCH_UI );
			}
		}
		return qtrue;
	}

	if ( Q_stricmp (cmd, "ui_openmenu" ) == 0 ) 
	{
		//if ( trap->Cvar_VariableValue ( "developer" ) )
		{
			Menus_CloseAll();
			if (Menus_ActivateByName(UI_Argv(1)))
			{
				trap->Key_SetCatcher( KEYCATCH_UI );
			}
			return qtrue;
		}
	}

	/*
	if ( Q_stricmp (cmd, "remapShader") == 0 ) {
		if (trap->Argc() == 4) {
			char shader1[MAX_QPATH];
			char shader2[MAX_QPATH];
			Q_strncpyz(shader1, UI_Argv(1), sizeof(shader1));
			Q_strncpyz(shader2, UI_Argv(2), sizeof(shader2));
			trap->R_RemapShader(shader1, shader2, UI_Argv(3));
			return qtrue;
		}
	}
	*/

	if ( Q_stricmp (cmd, "postgame") == 0 ) {
		UI_CalcPostGameStats();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "ui_cache") == 0 ) {
		UI_Cache_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "ui_teamOrders") == 0 ) {
		//UI_TeamOrdersMenu_f();
		return qtrue;
	}


	return qfalse;
}

/*
=================
UI_Shutdown
=================
*/
void UI_Shutdown( void ) {
}


void UI_DrawNamedPic( float x, float y, float width, float height, const char *picname ) {
	qhandle_t	hShader;

	hShader = trap->R_RegisterShaderNoMip( picname );
	trap->R_DrawStretchPic( x, y, width, height, 0, 0, 1, 1, hShader );
}

void UI_DrawHandlePic( float x, float y, float w, float h, qhandle_t hShader ) {
	float	s0;
	float	s1;
	float	t0;
	float	t1;

	if( w < 0 ) {	// flip about vertical
		w  = -w;
		s0 = 1;
		s1 = 0;
	}
	else {
		s0 = 0;
		s1 = 1;
	}

	if( h < 0 ) {	// flip about horizontal
		h  = -h;
		t0 = 1;
		t1 = 0;
	}
	else {
		t0 = 0;
		t1 = 1;
	}
	
	trap->R_DrawStretchPic( x, y, w, h, s0, t0, s1, t1, hShader );
}

/*
================
UI_FillRect

Coordinates are 640*480 virtual values
=================
*/
void UI_FillRect( float x, float y, float width, float height, const float *color ) {
	trap->R_SetColor( color );
	trap->R_DrawStretchPic( x, y, width, height, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap->R_SetColor( NULL );
}

void UI_DrawSides(float x, float y, float w, float h) {
	trap->R_DrawStretchPic( x, y, 1, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap->R_DrawStretchPic( x + w - 1, y, 1, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
}

void UI_DrawTopBottom(float x, float y, float w, float h) {
	trap->R_DrawStretchPic( x, y, w, 1, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap->R_DrawStretchPic( x, y + h - 1, w, 1, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
}
/*
================
UI_DrawRect

Coordinates are 640*480 virtual values
=================
*/
void UI_DrawRect( float x, float y, float width, float height, const float *color ) {
	trap->R_SetColor( color );

	UI_DrawTopBottom(x, y, width, height);
	UI_DrawSides(x, y, width, height);

	trap->R_SetColor( NULL );
}

void UI_SetColor( const float *rgba ) {
	trap->R_SetColor( rgba );
}

void UI_UpdateScreen( void ) {
	trap->UpdateScreen();
}


void UI_DrawTextBox (int x, int y, int width, int lines)
{
	UI_FillRect( x + BIGCHAR_WIDTH/2, y + BIGCHAR_HEIGHT/2, ( width + 1 ) * BIGCHAR_WIDTH, ( lines + 1 ) * BIGCHAR_HEIGHT, colorBlack );
	UI_DrawRect( x + BIGCHAR_WIDTH/2, y + BIGCHAR_HEIGHT/2, ( width + 1 ) * BIGCHAR_WIDTH, ( lines + 1 ) * BIGCHAR_HEIGHT, colorWhite );
}

qboolean UI_CursorInRect (int x, int y, int width, int height)
{
	if (uiInfo.uiDC.cursorx < x ||
		uiInfo.uiDC.cursory < y ||
		uiInfo.uiDC.cursorx > x+width ||
		uiInfo.uiDC.cursory > y+height)
		return qfalse;

	return qtrue;
}
