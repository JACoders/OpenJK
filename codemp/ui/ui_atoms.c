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

/**********************************************************************
	UI_ATOMS.C

	User interface building blocks and support functions.
**********************************************************************/
#include "ui_local.h"

#define NUM_UI_ARGSTRS (4)
#define UI_ARGSTR_MASK (NUM_UI_ARGSTRS-1)
static char tempArgStrs[NUM_UI_ARGSTRS][MAX_STRING_CHARS];

static char *UI_Argv( int arg ) {
	static int index=0;
	char *s = tempArgStrs[index++ & UI_ARGSTR_MASK];
	trap->Cmd_Argv( arg, s, MAX_STRING_CHARS );
	return s;
}

#define NUM_UI_CVARSTRS (4)
#define UI_CVARSTR_MASK (NUM_UI_CVARSTRS-1)
static char tempCvarStrs[NUM_UI_CVARSTRS][MAX_CVAR_VALUE_STRING];

char *UI_Cvar_VariableString( const char *name ) {
	static int index=0;
	char *s = tempCvarStrs[index++ & UI_ARGSTR_MASK];
	trap->Cvar_VariableStringBuffer( name, s, MAX_CVAR_VALUE_STRING );
	return s;
}

static void	UI_Cache_f( void ) {
	Display_CacheAll();
	if ( trap->Cmd_Argc() == 2 ) {
		int i;
		for ( i=0; i<uiInfo.q3HeadCount; i++ ) {
			trap->Print( "model %s\n", uiInfo.q3HeadNames[i] );
		}
	}
}

static void UI_OpenMenu_f( void ) {
	Menus_CloseAll();
	if ( Menus_ActivateByName( UI_Argv( 1 ) ) )
		trap->Key_SetCatcher( KEYCATCH_UI );
}

static void UI_OpenSiegeMenu_f( void ) {
	if ( trap->Cvar_VariableValue( "g_gametype" ) == GT_SIEGE ) {
		Menus_CloseAll();
		if ( Menus_ActivateByName( UI_Argv( 1 ) ) )
			trap->Key_SetCatcher( KEYCATCH_UI );
	}
}

typedef struct consoleCommand_s {
	const char	*cmd;
	void		(*func)(void);
} consoleCommand_t;

int cmdcmp( const void *a, const void *b ) {
	return Q_stricmp( (const char *)a, ((consoleCommand_t*)b)->cmd );
}

static consoleCommand_t	commands[] = {
	{ "ui_cache",			UI_Cache_f },
	{ "ui_load",			UI_Load },
	{ "ui_openmenu",		UI_OpenMenu_f },
	{ "ui_opensiegemenu",	UI_OpenSiegeMenu_f },
	{ "ui_report",			UI_Report },
};

static const size_t numCommands = ARRAY_LEN( commands );

/*
=================
UI_ConsoleCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
qboolean UI_ConsoleCommand( int realTime ) {
	consoleCommand_t *command = NULL;

	uiInfo.uiDC.frameTime = realTime - uiInfo.uiDC.realTime;
	uiInfo.uiDC.realTime = realTime;

	command = (consoleCommand_t *)Q_LinearSearch( UI_Argv( 0 ), commands, numCommands, sizeof( commands[0] ), cmdcmp );

	if ( !command )
		return qfalse;

	command->func();
	return qtrue;
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
