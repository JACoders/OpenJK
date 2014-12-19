#include "sys_local.h"
#include <conio.h>
#include <direct.h>
#include <io.h>
#include <ShlObj.h>
#include <Windows.h>

#define MEM_THRESHOLD (128*1024*1024)

// Used to determine where to store user-specific files
static char homePath[ MAX_OSPATH ] = { 0 };

/*
==============
Sys_Basename
==============
*/
const char *Sys_Basename( char *path )
{
	static char base[ MAX_OSPATH ] = { 0 };
	int length;

	length = strlen( path ) - 1;

	// Skip trailing slashes
	while( length > 0 && path[ length ] == '\\' )
		length--;

	while( length > 0 && path[ length - 1 ] != '\\' )
		length--;

	Q_strncpyz( base, &path[ length ], sizeof( base ) );

	length = strlen( base ) - 1;

	// Strip trailing slashes
	while( length > 0 && base[ length ] == '\\' )
    base[ length-- ] = '\0';

	return base;
}

/*
==============
Sys_Dirname
==============
*/
const char *Sys_Dirname( char *path )
{
	static char dir[ MAX_OSPATH ] = { 0 };
	int length;

	Q_strncpyz( dir, path, sizeof( dir ) );
	length = strlen( dir ) - 1;

	while( length > 0 && dir[ length ] != '\\' )
		length--;

	dir[ length ] = '\0';

	return dir;
}

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
	return Sys_Milliseconds(false);
}

/*
================
Sys_RandomBytes
================
*/
qboolean Sys_RandomBytes( byte *string, int len )
{
	HCRYPTPROV  prov;

	if( !CryptAcquireContext( &prov, NULL, NULL,
		PROV_RSA_FULL, CRYPT_VERIFYCONTEXT ) )  {

		return qfalse;
	}

	if( !CryptGenRandom( prov, len, (BYTE *)string ) )  {
		CryptReleaseContext( prov, 0 );
		return qfalse;
	}
	CryptReleaseContext( prov, 0 );
	return qtrue;
}

/*
 ==================
 Sys_GetCurrentUser
 ==================
 */
char *Sys_GetCurrentUser( void )
{
	static char s_userName[1024];
	DWORD size = sizeof( s_userName );

	if ( !GetUserName( s_userName, &size ) )
		strcpy( s_userName, "player" );

	if ( !s_userName[0] )
	{
		strcpy( s_userName, "player" );
	}

	return s_userName;
}

char	*Sys_DefaultHomePath(void) {
#ifdef _PORTABLE_VERSION
	Com_Printf("Portable install requested, skipping homepath support\n");
	return NULL;
#else
	typedef HRESULT (__stdcall * GETFOLDERPATH)(HWND, int, HANDLE, DWORD, LPSTR);

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
#endif
}

static const char *GetErrorString( DWORD error ) {
	static char buf[MAX_STRING_CHARS];
	buf[0] = '\0';

	if ( error ) {
		LPVOID lpMsgBuf;
		DWORD bufLen = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, error, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), (LPTSTR)&lpMsgBuf, 0, NULL );
		if ( bufLen ) {
			LPCSTR lpMsgStr = (LPCSTR)lpMsgBuf;
			Q_strncpyz( buf, lpMsgStr, min( (size_t)(lpMsgStr + bufLen), sizeof(buf) ) );
			LocalFree( lpMsgBuf );
		}
	}
	return buf;
}

void Sys_SetProcessorAffinity( void ) {
	DWORD_PTR processMask, dummy;
	HANDLE handle = GetCurrentProcess();

	if ( !GetProcessAffinityMask( handle, &dummy, &dummy ) )
		return;

	if ( sscanf( com_affinity->string, "%X", &processMask ) != 1 )
		processMask = 1; // set to first core only

	if ( !processMask )
		return;

	if ( !SetProcessAffinityMask( handle, processMask ) )
		Com_DPrintf( "Setting affinity mask failed (%s)\n", GetErrorString( GetLastError() ) );
}

/*
==================
Sys_LowPhysicalMemory()
==================
*/

qboolean Sys_LowPhysicalMemory(void) {
	static MEMORYSTATUSEX stat;
	static qboolean bAsked = qfalse;
	static cvar_t* sys_lowmem = Cvar_Get( "sys_lowmem", "0", 0 );

	if (!bAsked)	// just in case it takes a little time for GlobalMemoryStatusEx() to gather stats on
	{				//	stuff we don't care about such as virtual mem etc.
		bAsked = qtrue;

		stat.dwLength = sizeof (stat);
		GlobalMemoryStatusEx (&stat);
	}
	if (sys_lowmem->integer)
	{
		return qtrue;
	}
	return (stat.ullTotalPhys <= MEM_THRESHOLD) ? qtrue : qfalse;
}

/*
==============
Sys_Mkdir
==============
*/
qboolean Sys_Mkdir( const char *path ) {
	if( !CreateDirectory( path, NULL ) )
	{
		if( GetLastError( ) != ERROR_ALREADY_EXISTS )
			return qfalse;
	}
	return qtrue;
}

/*
==============
Sys_Cwd
==============
*/
char *Sys_Cwd( void ) {
	static char cwd[MAX_OSPATH];

	_getcwd( cwd, sizeof( cwd ) - 1 );
	cwd[MAX_OSPATH-1] = 0;

	return cwd;
}

/* Resolves path names and determines if they are the same */
/* For use with full OS paths not quake paths */
/* Returns true if resulting paths are valid and the same, otherwise false */
bool Sys_PathCmp( const char *path1, const char *path2 ) {
	char *r1, *r2;

	r1 = _fullpath(NULL, path1, MAX_OSPATH);
	r2 = _fullpath(NULL, path2, MAX_OSPATH);

	if(r1 && r2 && !Q_stricmp(r1, r2))
	{
		free(r1);
		free(r2);
		return true;
	}

	free(r1);
	free(r2);
	return false;
}

/*
==============================================================

DIRECTORY SCANNING

==============================================================
*/

#define	MAX_FOUND_FILES	0x1000

void Sys_ListFilteredFiles( const char *basedir, char *subdirs, char *filter, char **psList, int *numfiles ) {
	char		search[MAX_OSPATH], newsubdirs[MAX_OSPATH];
	char		filename[MAX_OSPATH];
	intptr_t	findhandle;
	struct _finddata_t findinfo;

	if ( *numfiles >= MAX_FOUND_FILES - 1 ) {
		return;
	}

	if (strlen(subdirs)) {
		Com_sprintf( search, sizeof(search), "%s\\%s\\*", basedir, subdirs );
	}
	else {
		Com_sprintf( search, sizeof(search), "%s\\*", basedir );
	}

	findhandle = _findfirst (search, &findinfo);
	if (findhandle == -1) {
		return;
	}

	do {
		if (findinfo.attrib & _A_SUBDIR) {
			if (Q_stricmp(findinfo.name, ".") && Q_stricmp(findinfo.name, "..")) {
				if (strlen(subdirs)) {
					Com_sprintf( newsubdirs, sizeof(newsubdirs), "%s\\%s", subdirs, findinfo.name);
				}
				else {
					Com_sprintf( newsubdirs, sizeof(newsubdirs), "%s", findinfo.name);
				}
				Sys_ListFilteredFiles( basedir, newsubdirs, filter, psList, numfiles );
			}
		}
		if ( *numfiles >= MAX_FOUND_FILES - 1 ) {
			break;
		}
		Com_sprintf( filename, sizeof(filename), "%s\\%s", subdirs, findinfo.name );
		if (!Com_FilterPath( filter, filename, qfalse ))
			continue;
		psList[ *numfiles ] = CopyString( filename );
		(*numfiles)++;
	} while ( _findnext (findhandle, &findinfo) != -1 );

	_findclose (findhandle);
}

static qboolean strgtr(const char *s0, const char *s1) {
	int l0, l1, i;

	l0 = strlen(s0);
	l1 = strlen(s1);

	if (l1<l0) {
		l0 = l1;
	}

	for(i=0;i<l0;i++) {
		if (s1[i] > s0[i]) {
			return qtrue;
		}
		if (s1[i] < s0[i]) {
			return qfalse;
		}
	}
	return qfalse;
}

char **Sys_ListFiles( const char *directory, const char *extension, char *filter, int *numfiles, qboolean wantsubs ) {
	char		search[MAX_OSPATH];
	int			nfiles;
	char		**listCopy;
	char		*list[MAX_FOUND_FILES];
	struct _finddata_t findinfo;
	intptr_t	findhandle;
	int			flag;
	int			i;

	if (filter) {

		nfiles = 0;
		Sys_ListFilteredFiles( directory, "", filter, list, &nfiles );

		list[ nfiles ] = 0;
		*numfiles = nfiles;

		if (!nfiles)
			return NULL;

		listCopy = (char **)Z_Malloc( ( nfiles + 1 ) * sizeof( *listCopy ), TAG_LISTFILES );
		for ( i = 0 ; i < nfiles ; i++ ) {
			listCopy[i] = list[i];
		}
		listCopy[i] = NULL;

		return listCopy;
	}

	if ( !extension) {
		extension = "";
	}

	// passing a slash as extension will find directories
	if ( extension[0] == '/' && extension[1] == 0 ) {
		extension = "";
		flag = 0;
	} else {
		flag = _A_SUBDIR;
	}

	Com_sprintf( search, sizeof(search), "%s\\*%s", directory, extension );

	// search
	nfiles = 0;

	findhandle = _findfirst (search, &findinfo);
	if (findhandle == -1) {
		*numfiles = 0;
		return NULL;
	}

	do {
		if ( (!wantsubs && flag ^ ( findinfo.attrib & _A_SUBDIR )) || (wantsubs && findinfo.attrib & _A_SUBDIR) ) {
			if ( nfiles == MAX_FOUND_FILES - 1 ) {
				break;
			}
			list[ nfiles ] = CopyString( findinfo.name );
			nfiles++;
		}
	} while ( _findnext (findhandle, &findinfo) != -1 );

	list[ nfiles ] = 0;

	_findclose (findhandle);

	// return a copy of the list
	*numfiles = nfiles;

	if ( !nfiles ) {
		return NULL;
	}

	listCopy = (char **)Z_Malloc( ( nfiles + 1 ) * sizeof( *listCopy ), TAG_LISTFILES );
	for ( i = 0 ; i < nfiles ; i++ ) {
		listCopy[i] = list[i];
	}
	listCopy[i] = NULL;

	do {
		flag = 0;
		for(i=1; i<nfiles; i++) {
			if (strgtr(listCopy[i-1], listCopy[i])) {
				char *temp = listCopy[i];
				listCopy[i] = listCopy[i-1];
				listCopy[i-1] = temp;
				flag = 1;
			}
		}
	} while(flag);

	return listCopy;
}

void	Sys_FreeFileList( char **psList ) {
	int		i;

	if ( !psList ) {
		return;
	}

	for ( i = 0 ; psList[i] ; i++ ) {
		Z_Free( psList[i] );
	}

	Z_Free( psList );
}

/*
========================================================================

LOAD/UNLOAD DLL

========================================================================
*/
//make sure the dll can be opened by the file system, then write the
//file back out again so it can be loaded is a library. If the read
//fails then the dll is probably not in the pk3 and we are running
//a pure server -rww
bool Sys_UnpackDLL(const char *name)
{
	void *data;
	fileHandle_t f;
	int len = FS_ReadFile(name, &data);
	int ck;

	if (len < 1)
	{ //failed to read the file (out of the pk3 if pure)
		return false;
	}

	if (FS_FileIsInPAK(name, &ck) == -1)
	{ //alright, it isn't in a pk3 anyway, so we don't need to write it.
		//this is allowable when running non-pure.
		FS_FreeFile(data);
		return true;
	}

	f = FS_FOpenFileWrite( name, qfalse );
	if ( !f )
	{ //can't open for writing? Might be in use.
		//This is possibly a malicious user attempt to circumvent dll
		//replacement so we won't allow it.
		FS_FreeFile(data);
		return false;
	}

	if (FS_Write( data, len, f ) < len)
	{ //Failed to write the full length. Full disk maybe?
		FS_FreeFile(data);
		return false;
	}

	FS_FCloseFile( f );
	FS_FreeFile(data);

	return true;
}

static char g_consoleField1[256];
static char g_consoleField2[256];

/*
===============
PrintMatches

===============
*/
static void PrintMatches( const char *s ) {
	if ( !Q_stricmpn( s, g_consoleField1, strlen( g_consoleField1 ) ) ) {
		printf( "    %s\n", s );
	}
}

char *Sys_ConsoleInput(void)
{
	const char ClearLine[] = "\r                                                                               \r";

	static int	len=0;
	static bool bPendingExtended = false;

	if (!_kbhit()) return NULL;

	if (len == 0) memset(g_consoleField1,0,sizeof(g_consoleField1));

	g_consoleField1[len] = _getch();

	if (bPendingExtended)
	{
		switch (g_consoleField1[len])
		{
			case 'H':	//up
				strcpy(g_consoleField1, g_consoleField2);
				printf(ClearLine);
				printf("%s",g_consoleField1);
				len = strlen(g_consoleField1);
			break;

			case 'K':	//left
			break;

			case 'M':	//right
			break;

			case 'P':	//down
			break;
		}
		g_consoleField1[len] = 0;	//erase last key hit
		bPendingExtended = false;
	}
	else
	{
		switch ((unsigned char) g_consoleField1[len])
		{
		case 0x00:	//fkey is next
		case 0xe0:	//extended = arrow keys
			g_consoleField1[len] = 0;	//erase last key hit
			bPendingExtended = true;
			break;
		case 8: // backspace
			printf("%c %c",g_consoleField1[len],g_consoleField1[len]);
			g_consoleField1[len] = 0;
			if (len > 0) len--;
			g_consoleField1[len] = 0;
			break;
		case 9:	//Tab
			if (len) {
				g_consoleField1[len] = 0;	//erase last key hit
				printf( "\n");
				// run through again, printing matches
				Cmd_CommandCompletion( PrintMatches );
				Cvar_CommandCompletion( PrintMatches );
				printf( "\n%s", g_consoleField1);
			}
			break;
		case 27: // esc
			// clear the line
			printf(ClearLine);
			len = 0;
			break;
		case '\r':	//enter
			g_consoleField1[len] = 0;	//erase last key hit
			printf("\n");
			if (len) {
				len = 0;
				strcpy(g_consoleField2, g_consoleField1);
				return g_consoleField1;
			}
			break;
		case 'v' - 'a' + 1:	// ctrl-v is paste
			g_consoleField1[len] = 0;	//erase last key hit
			char *cbd;
			cbd = Sys_GetClipboardData();
			if (cbd) {
				strncpy (&g_consoleField1[len], cbd, sizeof(g_consoleField1) );
				printf("%s",cbd);
				len += strlen(cbd);
				Z_Free( cbd );
				if (len == sizeof(g_consoleField1))
				{
					len = 0;
					return g_consoleField1;
				}
			}
			break;
		default:
			printf("%c",g_consoleField1[len]);
			len++;
			if (len == sizeof(g_consoleField1))
			{
				len = 0;
				return g_consoleField1;
			}
			break;
		}
	}

	return NULL;
}

/*
================
Sys_Init

Called after the common systems (cvars, files, etc)
are initialized
================
*/
void Sys_Init( void ) {
	OSVERSIONINFO osversion = {};
	osversion.dwOSVersionInfoSize = sizeof( osversion );

	if (!GetVersionEx (&osversion))
		Sys_Error ("Couldn't get OS info");

	if (osversion.dwMajorVersion < 4)
		Sys_Error ("This game requires Windows version 4 or greater");
	if (osversion.dwPlatformId == VER_PLATFORM_WIN32s)
		Sys_Error ("This game doesn't run on Win32s");

	// make sure the timer is high precision, otherwise
	// NT gets 18ms resolution
	timeBeginPeriod( 1 );

	Cmd_AddCommand ("in_restart", IN_Restart);
	Cvar_Set( "arch", OS_STRING " " ARCH_STRING );
	Cvar_Set( "username", Sys_GetCurrentUser() );
}

void Sys_Sleep( int msec )
{
	Sleep(msec);
}