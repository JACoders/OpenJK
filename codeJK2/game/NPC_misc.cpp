/*
This file is part of Jedi Knight 2.

    Jedi Knight 2 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Jedi Knight 2 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Knight 2.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

//
// NPC_misc.cpp
//

// leave this line at the top for all NPC_xxxx.cpp files...
#include "g_headers.h"




#include "b_local.h"
#include "../../code/qcommon/q_shared.h"
/*
Debug_Printf
*/
void Debug_Printf (cvar_t *cv, int debugLevel, char *fmt, ...)
{
	char		*color;
	va_list		argptr;
	char		msg[1024];

	if (cv->value < debugLevel)
		return;

	if (debugLevel == DEBUG_LEVEL_DETAIL)
		color = S_COLOR_WHITE;
	else if (debugLevel == DEBUG_LEVEL_INFO)
		color = S_COLOR_GREEN;
	else if (debugLevel == DEBUG_LEVEL_WARNING)
		color = S_COLOR_YELLOW;
	else if (debugLevel == DEBUG_LEVEL_ERROR)
		color = S_COLOR_RED;
	else
		color = S_COLOR_RED;

	va_start (argptr,fmt);
	vsprintf (msg, fmt, argptr);
	va_end (argptr);

	gi.Printf("%s%5i:%s", color, level.time, msg);
}


/*
Debug_NPCPrintf
*/
void Debug_NPCPrintf (gentity_t *printNPC, cvar_t *cv, int debugLevel, char *fmt, ...)
{
	int			color;
	va_list		argptr;
	char		msg[1024];

	if (cv->value < debugLevel)
	{
		return;
	}

	if ( debugNPCName->string[0] && Q_stricmp( debugNPCName->string, printNPC->targetname) != 0 ) 
	{
		return;
	}

	if (debugLevel == DEBUG_LEVEL_DETAIL)
		color = COLOR_WHITE;
	else if (debugLevel == DEBUG_LEVEL_INFO)
		color = COLOR_GREEN;
	else if (debugLevel == DEBUG_LEVEL_WARNING)
		color = COLOR_YELLOW;
	else if (debugLevel == DEBUG_LEVEL_ERROR)
		color = COLOR_RED;
	else
		color = COLOR_RED;

	va_start (argptr,fmt);
	vsprintf (msg, fmt, argptr);
	va_end (argptr);

	gi.Printf ("%c%c%5i (%s) %s", Q_COLOR_ESCAPE, color, level.time, printNPC->targetname, msg);
}
