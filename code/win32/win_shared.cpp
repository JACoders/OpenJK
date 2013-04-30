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
#include <shlobj.h>

// Used to determine where to store user-specific files
static char homePath[ MAX_OSPATH ] = { 0 };

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
