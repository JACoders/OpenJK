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

#include "../server/exe_headers.h"

#include "ui_local.h"

/*
===============================================================================

CONNECTION SCREEN

===============================================================================
*/

char		connectionDialogString[1024];
char		connectionMessageString[1024];

#ifdef JK2_MODE
/*
 =================
 UI_DrawThumbNail
 =================
 */
void UI_DrawThumbNail( float x, float y, float w, float h, byte *pic )
{
	ui.DrawStretchRaw( x, y, w, h, SG_SCR_WIDTH, SG_SCR_HEIGHT, pic, 0, qtrue );
}
#endif

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
#ifdef JK2_MODE
	qboolean qValid;
	byte *levelPic = SCR_GetScreenshot(&qValid);
	// draw the dialog background
	if (!qValid)
	{
		UI_DrawHandlePic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.menuBackShader );
	}
	else {
		UI_DrawThumbNail(0,0, SCREEN_WIDTH, SCREEN_HEIGHT, levelPic );
	}
#else
	UI_DrawHandlePic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.menuBackShader );
#endif
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


