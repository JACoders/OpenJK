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

#include "ui_local.h"

//
// Cvar callbacks
//

static int UI_GetScreenshotFormatForString( const char *str ) {
	if ( !Q_stricmp(str, "jpg") || !Q_stricmp(str, "jpeg") )
		return SSF_JPEG;
	else if ( !Q_stricmp(str, "tga") )
		return SSF_TGA;
	else if ( !Q_stricmp(str, "png") )
		return SSF_PNG;
	else
		return -1;
}

static const char *UI_GetScreenshotFormatString( int format )
{
	switch ( format )
	{
	default:
	case SSF_JPEG:
		return "jpg";
	case SSF_TGA:
		return "tga";
	case SSF_PNG:
		return "png";
	}
}

static void UI_UpdateScreenshot( void )
{
	qboolean changed = qfalse;
	// check some things
	if ( ui_screenshotType.string[0] && isalpha( ui_screenshotType.string[0] ) )
	{
		int ssf = UI_GetScreenshotFormatForString( ui_screenshotType.string );
		if ( ssf == -1 )
		{
			trap->Print( "UI Screenshot Format Type '%s' unrecognised, defaulting to JPEG\n", ui_screenshotType.string );
			uiInfo.uiDC.screenshotFormat = SSF_JPEG;
			changed = qtrue;
		}
		else
			uiInfo.uiDC.screenshotFormat = ssf;
	}
	else if ( ui_screenshotType.integer < SSF_JPEG || ui_screenshotType.integer > SSF_PNG )
	{
		trap->Print( "ui_screenshotType %i is out of range, defaulting to 0 (JPEG)\n", ui_screenshotType.integer );
		uiInfo.uiDC.screenshotFormat = SSF_JPEG;
		changed = qtrue;
	}
	else {
		uiInfo.uiDC.screenshotFormat = atoi( ui_screenshotType.string );
		changed = qtrue;
	}

	if ( changed ) {
		trap->Cvar_Set( "ui_screenshotType", UI_GetScreenshotFormatString( uiInfo.uiDC.screenshotFormat ) );
		trap->Cvar_Update( &ui_screenshotType );
	}
}


//
// Cvar table
//

typedef struct cvarTable_s {
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	void		(*update)( void );
	uint32_t	cvarFlags;
} cvarTable_t;

#define XCVAR_DECL
	#include "ui_xcvar.h"
#undef XCVAR_DECL

static const cvarTable_t uiCvarTable[] = {
	#define XCVAR_LIST
		#include "ui_xcvar.h"
	#undef XCVAR_LIST
};
static const size_t uiCvarTableSize = ARRAY_LEN( uiCvarTable );

void UI_RegisterCvars( void ) {
	size_t i = 0;
	const cvarTable_t *cv = NULL;

	for ( i=0, cv=uiCvarTable; i<uiCvarTableSize; i++, cv++ ) {
		trap->Cvar_Register( cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags );
		if ( cv->update )
			cv->update();
	}
}

void UI_UpdateCvars( void ) {
	size_t i = 0;
	const cvarTable_t *cv = NULL;

	for ( i=0, cv=uiCvarTable; i<uiCvarTableSize; i++, cv++ ) {
		if ( cv->vmCvar ) {
			int modCount = cv->vmCvar->modificationCount;
			trap->Cvar_Update( cv->vmCvar );
			if ( cv->vmCvar->modificationCount != modCount ) {
				if ( cv->update )
					cv->update();
			}
		}
	}
}
