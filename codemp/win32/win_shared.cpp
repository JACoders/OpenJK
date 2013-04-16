//Anything above this #include will be ignored by the compiler
#include "qcommon/exe_headers.h"

#include "win_local.h"
#include <lmerr.h>
#include <lmcons.h>
#include <lmwksta.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <direct.h>
#include <io.h>
#include <conio.h>

/*
================
Sys_Milliseconds
================
*/
int Sys_Milliseconds (bool baseTime)
{
	static int sys_timeBase = timeGetTime();
	int			sys_curtime;

	sys_curtime = timeGetTime();
	if(!baseTime)
	{
		sys_curtime -= sys_timeBase;
	}

	return sys_curtime;
}

int Sys_Milliseconds2( void )
{
	static int sys_timeBase = timeGetTime();
	int			sys_curtime;

	sys_curtime = timeGetTime();
	sys_curtime -= sys_timeBase;

	return sys_curtime;
}

/*
================
Sys_SnapVector
================
*/
void Sys_SnapVector( float *v )
{
	int i;
	float f;

	f = *v;
	__asm	fld		f;
	__asm	fistp	i;
	*v = i;
	v++;
	f = *v;
	__asm	fld		f;
	__asm	fistp	i;
	*v = i;
	v++;
	f = *v;
	__asm	fld		f;
	__asm	fistp	i;
	*v = i;
	/*
	*v = myftol(*v);
	v++;
	*v = myftol(*v);
	v++;
	*v = myftol(*v);
	*/
}


//============================================

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

char	*Sys_DefaultHomePath(void) {
	return NULL;
}

char *Sys_DefaultInstallPath(void)
{
	return Sys_Cwd();
}
