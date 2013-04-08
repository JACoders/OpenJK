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
	static char s_userName[1024];
	unsigned long size = sizeof( s_userName );


	if ( !GetUserName( s_userName, &size ) )
		strcpy( s_userName, "player" );

	if ( !s_userName[0] )
	{
		strcpy( s_userName, "player" );
	}

	return s_userName;
}