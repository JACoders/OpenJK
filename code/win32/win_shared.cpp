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

// leave this as first line for PCH reasons...
//
#include "../server/exe_headers.h"




#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "win_local.h"

/*
================
Sys_Milliseconds
================
*/
int Sys_Milliseconds (void)
{
	static int sys_timeBase = timeGetTime();
	int			sys_curtime;

	sys_curtime = timeGetTime() - sys_timeBase;

	return sys_curtime;
}

char *Sys_GetCurrentUser( void )
{
#ifdef _XBOX
	return NULL;
#else
	static char s_userName[1024];
	unsigned long size = sizeof( s_userName );


	if ( !GetUserName( s_userName, &size ) )
		Q_strncpyz( s_userName, "player", 1024 );

	if ( !s_userName[0] )
	{
		Q_strncpyz( s_userName, "player", 1024 );
	}

	return s_userName;
#endif
}