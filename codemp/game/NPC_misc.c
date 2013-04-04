//
// NPC_misc.cpp
//
#include "b_local.h"
#include "q_shared.h"

/*
Debug_Printf
*/
void Debug_Printf (vmCvar_t *cv, int debugLevel, char *fmt, ...)
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

	Com_Printf("%s%5i:%s", color, level.time, msg);
}


/*
Debug_NPCPrintf
*/
void Debug_NPCPrintf (gentity_t *printNPC, vmCvar_t *cv, int debugLevel, char *fmt, ...)
{
	int			color;
	va_list		argptr;
	char		msg[1024];

	if (cv->value < debugLevel)
	{
		return;
	}

//	if ( debugNPCName.string[0] && Q_stricmp( debugNPCName.string, printNPC->targetname) != 0 ) 
//	{
//		return;
//	}

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

	Com_Printf ("%c%c%5i (%s) %s", Q_COLOR_ESCAPE, color, level.time, printNPC->targetname, msg);
}
