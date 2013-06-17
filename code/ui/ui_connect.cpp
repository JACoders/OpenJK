/*
This file is part of Jedi Academy.

    Jedi Academy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Jedi Academy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Academy.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

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

//	qboolean qValid;
//	byte *levelPic = ui.SCR_GetScreenshot(&qValid);
	// draw the dialog background
//	if (!qValid) 
	{
		UI_DrawHandlePic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.menuBackShader );
	} 
//	else {
//		UI_DrawThumbNail(0,SCREEN_HEIGHT, SCREEN_WIDTH, -SCREEN_HEIGHT, levelPic );
//	}
}


/*
========================
UI_UpdateConnectionString

========================
*/
void UI_UpdateConnectionString( const char *string ) {
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


