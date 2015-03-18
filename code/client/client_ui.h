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

// client_ui.h -- header for client access to ui funcs
#ifndef __CLIENTUI_H__
#define __CLIENTUI_H__

#include "../ui/ui_public.h"

void _UI_KeyEvent( int key, qboolean down );
void UI_SetActiveMenu( const char* menuname,const char *menuID );
void UI_UpdateConnectionMessageString( char *string );
qboolean UI_ConsoleCommand( void ) ;
qboolean _UI_IsFullscreen( void );

#endif //__CLIENTUI_H__