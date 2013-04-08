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