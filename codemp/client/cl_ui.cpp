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

#include "client.h"
#include "cl_uiapi.h"

/*
====================
CL_ShutdownUI
====================
*/
void CL_ShutdownUI( void ) {
	Key_SetCatcher( Key_GetCatcher( ) & ~KEYCATCH_UI );

	if ( !cls.uiStarted )
		return;

	cls.uiStarted = qfalse;

	CL_UnbindUI();
}

/*
====================
CL_InitUI
====================
*/

void CL_InitUI( void ) {
	// load the dll
	CL_BindUI();

	// init for this gamestate
	//rww - changed to <= CA_ACTIVE, because that is the state when we did a vid_restart
	//ingame (was just < CA_ACTIVE before, resulting in ingame menus getting wiped and
	//not reloaded on vid restart from ingame menu)
	UIVM_Init( (qboolean)(cls.state >= CA_AUTHORIZING && cls.state <= CA_ACTIVE) );
}

/*
====================
UI_GameCommand

See if the current console command is claimed by the ui
====================
*/
qboolean UI_GameCommand( void ) {
	if ( !cls.uiStarted )
		return qfalse;

	return UIVM_ConsoleCommand( cls.realtime );
}
