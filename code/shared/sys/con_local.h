/*
===========================================================================
Copyright (C) 2005 - 2015, ioquake3 contributors
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

#pragma once

#include <cstdio>

/* con_passive.cpp | con_win32.cpp | con_tty.cpp */
void CON_Shutdown( void );
void CON_Init( void );
char *CON_Input( void );
void CON_Print( const char *msg );

/* con_log.cpp */
void ConsoleLogAppend( const char *string );
void ConsoleLogWriteOut( FILE *fp );
