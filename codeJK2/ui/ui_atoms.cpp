/**********************************************************************
	UI_ATOMS.C

	User interface building blocks and support functions.
**********************************************************************/

// leave this at the top of all UI_xxxx files for PCH reasons...
//

#include "../server/exe_headers.h"

#include "ui_local.h"
#include "gameinfo.h"
#include "../qcommon/stv_version.h"

uiimport_t	ui;
uiStatic_t	uis;

//externs
static void UI_LoadMenu_f( void );
static void UI_SaveMenu_f( void );

//locals


/*
=================
UI_ForceMenuOff
=================
*/
void UI_ForceMenuOff (void)
{
	ui.Key_SetCatcher( ui.Key_GetCatcher() & ~KEYCATCH_UI );
	ui.Key_ClearStates();
	ui.Cvar_Set( "cl_paused", "0" );
}


/*
=================
UI_SetActiveMenu - 
	this should be the ONLY way the menu system is brought up
 
=================
*/
void UI_SetActiveMenu( const char* menuname,const char *menuID ) 
{
	// this should be the ONLY way the menu system is brought up (besides the UI_ConsoleCommand below)

	if (cls.state != CA_DISCONNECTED && !ui.SG_GameAllowedToSaveHere(qtrue))	//don't check full sytem, only if incamera
	{
		return;
	}

	if ( !menuname ) {
		UI_ForceMenuOff();
		return;
	}

	//make sure force-speed and slowmodeath doesn't slow down menus - NOTE: they should reset the timescale when the game un-pauses
	Cvar_SetValue( "timescale", 1.0f );

	UI_Cursor_Show(qtrue);

	// enusure minumum menu data is cached
	Menu_Cache();

	if ( Q_stricmp (menuname, "mainMenu") == 0 ) 
	{
		UI_MainMenu();
		return;
	}

	if ( Q_stricmp (menuname, "ingame") == 0 ) 
	{
		ui.Cvar_Set( "cl_paused", "1" );
		UI_InGameMenu(menuID);
		return;
	}

	if ( Q_stricmp (menuname, "datapad") == 0 ) 
	{
		ui.Cvar_Set( "cl_paused", "1" );
		UI_DataPadMenu();
		return;
	}
}


/*
=================
UI_Argv
=================
*/
static char *UI_Argv( int arg ) 
{
	static char	buffer[MAX_STRING_CHARS];

	ui.Argv( arg, buffer, sizeof( buffer ) );

	return buffer;
}


/*
=================
UI_Cvar_VariableString
=================
*/
char *UI_Cvar_VariableString( const char *var_name ) 
{
	static char	buffer[MAX_STRING_CHARS];

	ui.Cvar_VariableStringBuffer( var_name, buffer, sizeof( buffer ) );

	return buffer;
}

/*
=================
UI_Cache
=================
*/
static void UI_Cache_f( void ) 
{
	Menu_Cache();
}


/*
=================
UI_ConsoleCommand
=================
*/
qboolean UI_ConsoleCommand( void ) 
{
	char	*cmd;

	if (!ui.SG_GameAllowedToSaveHere(qtrue))	//only check if incamera
	{
		return qfalse;
	}

	cmd = UI_Argv( 0 );

	// ensure minimum menu data is available
	Menu_Cache();

	if ( Q_stricmp (cmd, "ui_cache") == 0 ) 
	{
		UI_Cache_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "levelselect") == 0 ) 
	{
		UI_LoadMenu_f();
		return qtrue;
	}
	
	if ( Q_stricmp (cmd, "ui_teamOrders") == 0 ) 
	{
		UI_SaveMenu_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "ui_report") == 0 ) 
	{
		UI_Report();
		return qtrue;
	}
	

	return qfalse;
}


/*
=================
UI_Init
=================
*/
void UI_Init( int apiVersion, uiimport_t *uiimport ) 
{
	gameinfo_import_t	gameinfo_import;

	ui = *uiimport;

	if ( apiVersion != UI_API_VERSION ) {
		ui.Error( ERR_FATAL, "Bad UI_API_VERSION: expected %i, got %i\n", UI_API_VERSION, apiVersion );
	}

	_UI_Init(qfalse);

	// get static data (glconfig, media)
	ui.GetGlconfig( &uis.glconfig );

	uis.scaley = uis.glconfig.vidHeight * (1.0/480.0);
	uis.scalex = uis.glconfig.vidWidth * (1.0/640.0);

	gameinfo_import.FS_FOpenFile = ui.FS_FOpenFile;
	gameinfo_import.FS_Read = ui.FS_Read;
	gameinfo_import.FS_ReadFile = ui.FS_ReadFile;
	gameinfo_import.FS_FreeFile = ui.FS_FreeFile;
	gameinfo_import.FS_FCloseFile = ui.FS_FCloseFile;
	gameinfo_import.Cvar_Set = ui.Cvar_Set;
	gameinfo_import.Cvar_VariableStringBuffer = ui.Cvar_VariableStringBuffer;
	gameinfo_import.Cvar_Create = ui.Cvar_Create;
	gameinfo_import.Printf = ui.Printf;
//	GI_Init( &gameinfo_import );

	Menu_Cache( );

	ui.Cvar_Create( "cg_drawCrosshair", "1", CVAR_ARCHIVE );
	ui.Cvar_Create( "cg_marks", "1", CVAR_ARCHIVE );
	ui.Cvar_Create ("s_language", "english", CVAR_ARCHIVE | CVAR_NORESTART);
}

// these are only here so the functions in q_shared.c can link

#ifndef UI_HARD_LINKED

/*
================
Com_Error
=================
*/
/*
void Com_Error( int level, const char *error, ... ) 
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	vsprintf (text, error, argptr);
	va_end (argptr);

	ui.Error( level, "%s", text);
}
*/
/*
================
Com_Printf
=================
*/
/*
void Com_Printf( const char *msg, ... ) 
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	vsprintf (text, msg, argptr);
	va_end (argptr);

	ui.Printf( "%s", text);
}
*/
#endif


/*
================
UI_DrawNamedPic
=================
*/
void UI_DrawNamedPic( float x, float y, float width, float height, const char *picname ) 
{
	qhandle_t	hShader;

	hShader = ui.R_RegisterShaderNoMip( picname );
	ui.R_DrawStretchPic( x, y, width, height, 0, 0, 1, 1, hShader );
}


/*
================
UI_DrawHandlePic
=================
*/
void UI_DrawHandlePic( float x, float y, float w, float h, qhandle_t hShader ) 
{
	float	s0;
	float	s1;
	float	t0;
	float	t1;

	if( w < 0 ) {	// flip about horizontal
		w  = -w;
		s0 = 1;
		s1 = 0;
	}
	else {
		s0 = 0;
		s1 = 1;
	}

	if( h < 0 ) {	// flip about vertical
		h  = -h;
		t0 = 1;
		t1 = 0;
	}
	else {
		t0 = 0;
		t1 = 1;
	}

	ui.R_DrawStretchPic( x, y, w, h, s0, t0, s1, t1, hShader );
}

/*
================
UI_FillRect

Coordinates are 640*480 virtual values
=================
*/
void UI_FillRect( float x, float y, float width, float height, const float *color ) 
{
	ui.R_SetColor( color );

	ui.R_DrawStretchPic( x, y, width, height, 0, 0, 0, 0, uis.whiteShader );

	ui.R_SetColor( NULL );
}

/*
=================
UI_UpdateScreen
=================
*/
void UI_UpdateScreen( void ) 
{
	ui.UpdateScreen();
}


static float fadePercent = 0.0;


/*
===============
UI_LoadMenu_f
===============
*/
static void UI_LoadMenu_f( void ) 
{
	trap_Key_SetCatcher( KEYCATCH_UI );
	Menus_ActivateByName("ingameloadMenu");
}

/*
===============
UI_SaveMenu_f
===============
*/
static void UI_SaveMenu_f( void ) 
{
	ui.PrecacheScreenshot();

	trap_Key_SetCatcher( KEYCATCH_UI );
	Menus_ActivateByName("ingamesaveMenu");
}


//--------------------------------------------

/*
=================
UI_SetColor
=================
*/
void UI_SetColor( const float *rgba ) 
{
	trap_R_SetColor( rgba );
}

/*int registeredFontCount = 0;
#define MAX_FONTS 6
static fontInfo_t registeredFont[MAX_FONTS];
*/

/*
=================
UI_RegisterFont
=================
*/

int UI_RegisterFont(const char *fontName) 
{
	int iFontIndex = ui.R_RegisterFont(fontName);
	if (iFontIndex == 0)
	{
		iFontIndex = ui.R_RegisterFont("ergoec");	// fall back
	}

	return iFontIndex;
}

