// leave this at the top of all UI_xxxx files for PCH reasons...
//
#include "../server/exe_headers.h"

#include "ui_local.h"

/*
===============================================================================

CONNECTION SCREEN

===============================================================================
*/

char		connectionDialogString[1024];
char		connectionMessageString[1024];


/*
========================
UI_DrawConnect

========================
*/
void UI_DrawConnect( const char *servername, const char *updateInfoString )
{
	// We need to use this special hack variable, nothing else is set up yet:
	const char *s = Cvar_VariableString( "ui_mapname" );

	// Special case for first map:
	extern SavedGameJustLoaded_e g_eSavedGameJustLoaded;
	if ( g_eSavedGameJustLoaded != eFULL && (!strcmp(s,"yavin1") || !strcmp(s,"demo")) )
	{
		ui.R_SetColor( colorTable[CT_BLACK] );
		uis.whiteShader = ui.R_RegisterShaderNoMip( "*white" );
		ui.R_DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f, 1.0f, uis.whiteShader );

		const char *t = SE_GetString( "SP_INGAME_ALONGTIME" );
		int w = ui.R_Font_StrLenPixels( t, uiInfo.uiDC.Assets.qhMediumFont, 1.0f );
		ui.R_Font_DrawString( (320)-(w/2), 140, t, colorTable[CT_ICON_BLUE], uiInfo.uiDC.Assets.qhMediumFont, -1, 1.0f );
		return;
	}

	// Grab the loading menu:
	extern menuDef_t *Menus_FindByName( const char *p );
	menuDef_t *menu = Menus_FindByName( "loadingMenu" );
	if( !menu )
		return;

	// Find the levelshot item, so we can fix up it's shader:
	itemDef_t *item = Menu_FindItemByName( menu, "mappic" );
	if( !item )
		return;

	item->window.background = ui.R_RegisterShaderNoMip( va( "levelshots/%s", s ) );
	if (!item->window.background) {
		item->window.background = ui.R_RegisterShaderNoMip( "menu/art/unknownmap" );	
	}

	// Do the second levelshot as well:
	qhandle_t firstShot = item->window.background;
	item = Menu_FindItemByName( menu, "mappic2" );
	if( !item )
		return;

	item->window.background = ui.R_RegisterShaderNoMip( va( "levelshots/%s2", s ) );
	if (!item->window.background) {
		item->window.background = firstShot;
	}

	const char *b = SE_GetString( "BRIEFINGS", s );
	if( b && b[0] )
		Cvar_Set( "ui_missionbriefing", va("@BRIEFINGS_%s", s) );
	else
		Cvar_Set( "ui_missionbriefing", "@BRIEFINGS_NONE" );

	// Now, force it to draw:
	extern void Menu_Paint( menuDef_t *menu, qboolean forcePaint );
	Menu_Paint( menu, qtrue );
}


/*
========================
UI_UpdateConnectionString

========================
*/
void UI_UpdateConnectionString( char *string ) {
	Q_strncpyz( connectionDialogString, string, sizeof( connectionDialogString )  );
	UI_UpdateScreen();
}

/*
========================
UI_UpdateConnectionMessageString

========================
*/
void UI_UpdateConnectionMessageString( char *string ) {
	char		*s;

	Q_strncpyz( connectionMessageString, string, sizeof( connectionMessageString ) );

	// strip \n
	s = strstr( connectionMessageString, "\n" );
	if ( s ) {
		*s = 0;
	}
	UI_UpdateScreen();
}

/*
===================
UI_KeyConnect
===================
*/
void UI_KeyConnect( int key ) 
{
	if ( key == A_ESCAPE ) 
	{
		ui.Cmd_ExecuteText( EXEC_APPEND, "disconnect\n" );
		return;
	}
}


