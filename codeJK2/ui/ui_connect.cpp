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
=================
UI_DrawThumbNail
=================
*/
void UI_DrawThumbNail( float x, float y, float w, float h, byte *pic ) 
{
	ui.DrawStretchRaw( x, y, w, h, SG_SCR_WIDTH, SG_SCR_HEIGHT, pic, 0, qtrue );
}

/*
========================
UI_DrawConnectText

This will also be overlaid on the cgame info screen during loading
to prevent it from blinking away too rapidly on local or lan games.
matches CG_DrawInformation from cg_info.cpp.
========================
*/
void UI_DrawConnectText( const char *servername, const char *updateInfoString ) {
	char		*s;
	connstate_t	connState;
//	char		info[MAX_INFO_VALUE];
	int			textX,textY;
	int			padY,yPos,xLength;

	Menu_Cache();

	connState = ui.GetClientState();

	yPos = 450;
	xLength = 114;

	// display global MOTD at bottom
//	UI_DrawString( SCREEN_WIDTH/2, SCREEN_HEIGHT-20, Info_ValueForKey( updateInfoString, "motd" ), UI_CENTER|UI_BIGFONT, menu_text_color );

	// print any server info
	if ( connState < CA_CONNECTED ) {
//		UI_DrawString( 16, 160, connectionMessageString, UI_LEFT|UI_BIGFONT, menu_text_color );
	}


	if ( connState == CA_CONNECTING ) {
		s = connectionDialogString;		// try number
	} else {
		s = "";
	}

	textX = 267;
	textY = 270;
	padY = 24;

	// Awaiting callenge
	if ( connState == CA_CONNECTING ) 
	{
		ui.R_SetColor( colorTable[CT_WHITE]);	
		return;
	}

	if ( connState == CA_CHALLENGING ) {
		s = connectionDialogString;		// try number
	} else {
		s = "";
	}

	// Awaiting connection
	textY += padY;
	if ( connState == CA_CHALLENGING ) 
	{
		ui.R_SetColor( colorTable[CT_WHITE]);	
		return;
	}


	// Awaiting gamestate
	textY += padY;
	if ( connState == CA_CONNECTED ) 
	{
		ui.R_SetColor( colorTable[CT_WHITE]);	
		return;
	}

	// Loading
	textY += padY;
	if ( connState == CA_LOADING ) 
	{
		ui.R_SetColor( colorTable[CT_WHITE]);	
		return;
	}

	// Awaiting snap shot
	textY += padY;
	if ( connState == CA_PRIMED ) 
	{
		ui.R_SetColor( colorTable[CT_WHITE]);	
		return;
	}

}

/*
========================
UI_DrawConnect

========================
*/
extern void UI_DrawThumbNail( float x, float y, float w, float h , byte *pic);	//ui_game

void UI_DrawConnect( const char *servername, const char *updateInfoString ) {
#if 0
	// if connecting to a local host, don't draw anything before the
	// gamestate message.  This allows cinematics to start seamlessly
	if ( connState < CA_LOADING && !strcmp( cls.servername, "localhost" ) ) {
		UI_SetColor( g_color_table[0] );
		re.DrawFill (0, 0, re.scrWidth, re.scrHeight);
		UI_SetColor( NULL );
		return;
	}
#endif

	qboolean qValid;
	byte *levelPic = ui.SCR_GetScreenshot(&qValid);
	// draw the dialog background
	if (!qValid) {
		UI_DrawHandlePic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.menuBackShader );
	} 
	else {
		UI_DrawThumbNail(0,SCREEN_HEIGHT, SCREEN_WIDTH, -SCREEN_HEIGHT, levelPic );
		// blend a detail texture over it
//		qhandle_t detail = ui.R_RegisterShader( "levelShotDetail" );
//		UI_DrawHandlePic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, detail );
	}
	UI_DrawConnectText( servername, updateInfoString );
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


