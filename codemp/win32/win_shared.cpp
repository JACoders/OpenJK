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
#include <shlobj.h>

// Used to determine where to store user-specific files
static char homePath[ MAX_OSPATH ] = { 0 };

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

typedef HRESULT (__stdcall * GETFOLDERPATH)(HWND, int, HANDLE, DWORD, LPSTR); 
char	*Sys_DefaultHomePath(void) {
	TCHAR szPath[MAX_PATH];
	GETFOLDERPATH qSHGetFolderPath;
	HMODULE shfolder = LoadLibrary("shfolder.dll");

	if(shfolder == NULL)
	{
		Com_Printf("Unable to load SHFolder.dll\n");
		return NULL;
	}

	if(!*homePath && com_homepath)
	{
		qSHGetFolderPath = (GETFOLDERPATH)GetProcAddress(shfolder, "SHGetFolderPathA");
		if(qSHGetFolderPath == NULL)
		{
			Com_Printf("Unable to find SHGetFolderPath in SHFolder.dll\n");
			FreeLibrary(shfolder);
			return NULL;
		}

		if( !SUCCEEDED( qSHGetFolderPath( NULL, CSIDL_PERSONAL,
						NULL, 0, szPath ) ) )
		{
			Com_Printf("Unable to detect CSIDL_PERSONAL\n");
			FreeLibrary(shfolder);
			return NULL;
		}
		
		Com_sprintf(homePath, sizeof(homePath), "%s%cMy Games%c", szPath, PATH_SEP, PATH_SEP);

		if(com_homepath->string[0])
			Q_strcat(homePath, sizeof(homePath), com_homepath->string);
		else
			Q_strcat(homePath, sizeof(homePath), HOMEPATH_NAME_WIN);
	}

	FreeLibrary(shfolder);
	return homePath;
}

char *Sys_DefaultInstallPath(void)
{
	return Sys_Cwd();
}
