// win_main.c
//Anything above this #include will be ignored by the compiler
#include "../qcommon/exe_headers.h"

#include "../client/client.h"
#include "win_local.h"
#include "resource.h"
#include <errno.h>
#include <float.h>
#include <fcntl.h>
#include <stdio.h>
#include <direct.h>
#include <io.h>
#include <conio.h>
#include "../qcommon/stringed_ingame.h"

#define	CD_BASEDIR	"gamedata\\gamedata"
#define	CD_EXE		"jamp.exe"
#define	CD_BASEDIR_LINUX	"bin\\x86\\glibc-2.1"
#define	CD_EXE_LINUX "jamp"
#define	CD_VOLUME	"JEDIACAD"
#define MEM_THRESHOLD 128*1024*1024

static char		sys_cmdline[MAX_STRING_CHARS];

// enable this for executable checksumming
#ifdef FINAL_BUILD
#define SPANK_MONKEYS
#endif
static int	sys_monkeySpank;
static int	sys_checksum;


/*
==================
Sys_LowPhysicalMemory()
==================
*/

qboolean Sys_LowPhysicalMemory() {
	static MEMORYSTATUS stat;
	static qboolean bAsked = qfalse;
	static cvar_t* sys_lowmem = Cvar_Get( "sys_lowmem", "0", 0 );

	if (!bAsked)	// just in case it takes a little time for GlobalMemoryStatus() to gather stats on
	{				//	stuff we don't care about such as virtual mem etc.
		bAsked = qtrue;
		GlobalMemoryStatus (&stat);
	}
	if (sys_lowmem->integer)
	{
		return qtrue;
	}
	return (stat.dwTotalPhys <= MEM_THRESHOLD) ? qtrue : qfalse;
}

/*
==================
Sys_FunctionCmp
==================
*/
int Sys_FunctionCmp(void *f1, void *f2) {

	int i, j, l;
	byte func_end[32] = {0xC3, 0x90, 0x90, 0x00};
	byte *ptr, *ptr2;
	byte *f1_ptr, *f2_ptr;

	ptr = (byte *) f1;
	if (*(byte *)ptr == 0xE9) {
		//Com_Printf("f1 %p1 jmp %d\n", (int *) f1, *(int*)(ptr+1));
		f1_ptr = (byte*)(((byte*)f1) + (*(int *)(ptr+1)) + 5);
	}
	else {
		f1_ptr = ptr;
	}
	//Com_Printf("f1 ptr %p\n", f1_ptr);

	ptr = (byte *) f2;
	if (*(byte *)ptr == 0xE9) {
		//Com_Printf("f2 %p jmp %d\n", (int *) f2, *(int*)(ptr+1));
		f2_ptr = (byte*)(((byte*)f2) + (*(int *)(ptr+1)) + 5);
	}
	else {
		f2_ptr = ptr;
	}
	//Com_Printf("f2 ptr %p\n", f2_ptr);

#ifdef _DEBUG
	sprintf((char *)func_end, "%c%c%c%c%c%c%c", 0x5F, 0x5E, 0x5B, 0x8B, 0xE5, 0x5D, 0xC3);
#endif
	for (i = 0; i < 1024; i++) {
		for (j = 0; func_end[j]; j++) {
			if (f1_ptr[i+j] != func_end[j])
				break;
		}
		if (!func_end[j]) {
			break;
		}
	}
#ifdef _DEBUG
	l = i + 7;
#else
	l = i + 2;
#endif
	//Com_Printf("function length = %d\n", l);

	for (i = 0; i < l; i++) {
		// check for a potential function call
		if (*((byte *) &f1_ptr[i]) == 0xE8) {
			// get the function pointers in case this really is a function call
			ptr = (byte *) (((byte *) &f1_ptr[i]) + (*(int *) &f1_ptr[i+1])) + 5;
			ptr2 = (byte *) (((byte *) &f2_ptr[i]) + (*(int *) &f2_ptr[i+1])) + 5;
			// if it was a function call and both f1 and f2 call the same function
			if (ptr == ptr2) {
				i += 4;
				continue;
			}
		}
		if (f1_ptr[i] != f2_ptr[i])
			return qfalse;
	}
	return qtrue;
}

/*
==================
Sys_FunctionCheckSum
==================
*/
int Sys_FunctionCheckSum(void *f1) {

	int i, j, l;
	byte func_end[32] = {0xC3, 0x90, 0x90, 0x00};
	byte *ptr;
	byte *f1_ptr;

	ptr = (byte *) f1;
	if (*(byte *)ptr == 0xE9) {
		//Com_Printf("f1 %p1 jmp %d\n", (int *) f1, *(int*)(ptr+1));
		f1_ptr = (byte*)(((byte*)f1) + (*(int *)(ptr+1)) + 5);
	}
	else {
		f1_ptr = ptr;
	}
	//Com_Printf("f1 ptr %p\n", f1_ptr);

#ifdef _DEBUG
	sprintf((char *)func_end, "%c%c%c%c%c%c%c", 0x5F, 0x5E, 0x5B, 0x8B, 0xE5, 0x5D, 0xC3);
#endif
	for (i = 0; i < 1024; i++) {
		for (j = 0; func_end[j]; j++) {
			if (f1_ptr[i+j] != func_end[j])
				break;
		}
		if (!func_end[j]) {
			break;
		}
	}
#ifdef _DEBUG
	l = i + 7;
#else
	l = i + 2;
#endif
	//Com_Printf("function length = %d\n", l);
	return Com_BlockChecksum( f1_ptr, l );
}

/*
==================
Sys_MonkeyShouldBeSpanked
==================
*/
int Sys_MonkeyShouldBeSpanked( void ) {
	return sys_monkeySpank;
}

/*
==================
Sys_CodeInMemoryChecksum
==================
*/
#define MakePtr( cast, ptr, addValue ) (cast)( (DWORD)(ptr) + (addValue) )

int Sys_CodeInMemoryChecksum( void *codeBase ) {
	PIMAGE_DOS_HEADER dosHeader;
	PIMAGE_NT_HEADERS pNTHeader;
	PIMAGE_SECTION_HEADER section;

	dosHeader = (PIMAGE_DOS_HEADER)codeBase;
	pNTHeader = MakePtr( PIMAGE_NT_HEADERS, dosHeader, dosHeader->e_lfanew );

	// First, verify that the e_lfanew field gave us a reasonable
	// pointer, then verify the PE signature.
	if ( IsBadReadPtr(pNTHeader, sizeof(IMAGE_NT_HEADERS)) ||
	     pNTHeader->Signature != IMAGE_NT_SIGNATURE )
	{
		//printf("Unhandled EXE type, or invalid .EXE\n");
		return 0;
	}
	// first section oughta be the code section
	section = (PIMAGE_SECTION_HEADER)(pNTHeader+1);
	/*
	// the name of the code section should be .text
	if ( Q_stricmp( section->Name, ".text" ) ) {
		return 0;
	}
	*/

	return Com_BlockChecksum( ((byte *) codeBase) + section->VirtualAddress, section->SizeOfRawData );
}

/*
==================
Sys_ChecksumExe
==================
*/

// make sure this string is unique in the executable
//					   01234567890123
byte *exeChecksumId = (unsigned char *)"q3monkeyid\0\0\0\0";

void Sys_ChecksumExe( void *codeBase ) {
	TCHAR szPathOrig[_MAX_PATH], szPathClone[_MAX_PATH];
	STARTUPINFO si;
	TCHAR szCmdLine[512];
	HANDLE hfile, hProcessOrig;
	PROCESS_INFORMATION pi;
	int l, i, n;
	FILE *f;
	byte *buf, *ptr;

	// Is this the original EXE or the clone EXE?
	if ( Q_stricmp(__argv[1], "monkey") ) {
		// Original EXE: Spawn clone EXE to delete this EXE

		GetModuleFileName(NULL, szPathOrig, _MAX_PATH);
		GetTempPath(_MAX_PATH, szPathClone);
		GetTempFileName(szPathClone, __TEXT("Del"), 0, szPathClone); 
		CopyFile(szPathOrig, szPathClone, FALSE);

		// Open the clone EXE using FILE_FLAG_DELETE_ON_CLOSE
		hfile = CreateFile(szPathClone, 0, FILE_SHARE_READ, NULL,   	                       
								OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE, NULL);
		// Spawn the clone EXE passing it our EXE's process handle
		// and the full path name to the original EXE file.
		hProcessOrig = OpenProcess(SYNCHRONIZE, TRUE,
									GetCurrentProcessId());
									wsprintf(szCmdLine, __TEXT("%s monkey %d %d \"%s\""), szPathClone,
												sys_checksum, hProcessOrig, szPathOrig);
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		CreateProcess(NULL, szCmdLine, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
		CloseHandle(hProcessOrig);
		CloseHandle(hfile);
	} else {
		// Clone EXE: When original EXE terminates, overwrite it with a new one
		sys_checksum = atoi( __argv[2] );
		hProcessOrig = (HANDLE) atoi( __argv[3] );
		WaitForSingleObject(hProcessOrig, INFINITE);
		CloseHandle(hProcessOrig);
		// open the original executable
		f = fopen( __argv[4], "rb" );
		if ( !f ) {
			return;
		}
		fseek (f, 0, SEEK_END);
		l = ftell (f);
		fseek (f, 0, SEEK_SET);
		buf = (unsigned char *)malloc(l);
		if ( fread(buf, l, 1, f) != 1 ) {
			return;
		}
		fclose(f);
		// search for the exe name string, nice brute force
		n = strlen((const char *)exeChecksumId);
		for ( i = 0; i < l; i++ ) {
			if ( !Q_strncmp((const char *)(buf + i), (const char *)exeChecksumId, n) ) {
				break;
			}
		}
		if ( i >= l ) {
			return;
		}
		ptr = buf + i;
		// write checksum into exe memory image
		ptr[0] = (sys_checksum >> 24) & 0xFF;
		ptr[1] = (sys_checksum >> 16) & 0xFF;
		ptr[2] = (sys_checksum >>  8) & 0xFF;
		ptr[3] = (sys_checksum >>  0) & 0xFF;
		ptr[4] = ptr[5] = ptr[6] = ptr[7] = ptr[8] = ptr[9] = 0;
		// write out new exe with checksum
		f = fopen( __argv[4], "wb" );
		if ( !f ) {
			return;
		}
		if ( fwrite(buf, l, 1, f) != 1 ) {
			return;
		}
		fclose(f);
		free(buf);
		// The system will delete the clone EXE automatically 
		// because it was opened with FILE_FLAG_DELETE_ON_CLOSE
	}
	//
	exit(0);
}

/*
==================
Sys_VerifyCodeChecksum
==================
*/
void Sys_VerifyCodeChecksum( void *codeBase ) {
	// NOTE: should not checksum code in debug mode because the memory image changes
	//		 as soon as you set a break point!
#if defined(SPANK_MONKEYS) && !defined(_DEBUG)
	int exeChecksum;

	// if the checksum is not yet stored in the executable
	if ( exeChecksumId[4] != 0 ) {
		// spawn another process that will replace this executable with one that has a checksum
		Sys_ChecksumExe( codeBase );
		return;
	}

	exeChecksum = (exeChecksumId[0] << 24) | (exeChecksumId[1] << 16) | (exeChecksumId[2] << 8) | exeChecksumId[3];
	if ( exeChecksum != sys_checksum ) {
		sys_monkeySpank = qtrue;
	}
#endif
}

/*
==================
Sys_BeginProfiling
==================
*/
void Sys_BeginProfiling( void ) {
	// this is just used on the mac build
}

/*
=============
Sys_Error

Show the early console as an error dialog
=============
*/
void QDECL Sys_Error( const char *error, ... ) {
	va_list		argptr;
	char		text[4096];
    MSG        msg;

	va_start (argptr, error);
	vsprintf (text, error, argptr);
	va_end (argptr);

	Conbuf_AppendText( text );
	Conbuf_AppendText( "\n" );

	Sys_SetErrorText( text );
	Sys_ShowConsole( 1, qtrue );

	timeEndPeriod( 1 );

	IN_Shutdown();

	// wait for the user to quit
	while ( 1 ) {
		if (!GetMessage (&msg, NULL, 0, 0))
			Com_Quit_f ();
		TranslateMessage (&msg);
      	DispatchMessage (&msg);
	}

	Sys_DestroyConsole();
	Com_ShutdownZoneMemory();
 	Com_ShutdownHunkMemory();

	exit (1);
}

/*
==============
Sys_Quit
==============
*/
void Sys_Quit( void ) {
	timeEndPeriod( 1 );
	IN_Shutdown();
	Sys_DestroyConsole();
	Com_ShutdownZoneMemory();
 	Com_ShutdownHunkMemory();

	exit (0);
}

/*
==============
Sys_Print
==============
*/
void Sys_Print( const char *msg ) {
	Conbuf_AppendText( msg );
}


/*
==============
Sys_Mkdir
==============
*/
void Sys_Mkdir( const char *path ) {
	_mkdir (path);
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

/*
==============
Sys_DefaultCDPath
==============
*/
char *Sys_DefaultCDPath( void ) {
	return "";
}

/*
==============
Sys_DefaultBasePath
==============
*/
char *Sys_DefaultBasePath( void ) {
	return Sys_Cwd();
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
	int			findhandle;
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
	int			findhandle;
	int			flag;
	int			i;

	if (filter) {

		nfiles = 0;
		Sys_ListFilteredFiles( directory, "", filter, list, &nfiles );

		list[ nfiles ] = 0;
		*numfiles = nfiles;

		if (!nfiles)
			return NULL;

		listCopy = (char **)Z_Malloc( ( nfiles + 1 ) * sizeof( *listCopy ), TAG_FILESYS );
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

	listCopy = (char **)Z_Malloc( ( nfiles + 1 ) * sizeof( *listCopy ), TAG_FILESYS );
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

//========================================================


/*
================
Sys_ScanForCD

Search all the drives to see if there is a valid CD to grab
the cddir from
================
*/
#ifdef FINAL_BUILD
static qboolean Sys_ScanForCD( void ) {
	char		drive[4];
	FILE		*f;
	char		test[MAX_OSPATH];

	drive[0] = 'c';
	drive[1] = ':';
	drive[2] = '\\';
	drive[3] = 0;

	// scan the drives
	for ( drive[0] = 'c' ; drive[0] <= 'z' ; drive[0]++ ) {
		if ( GetDriveType (drive) == DRIVE_CDROM ) {
			BOOL Result;
			char VolumeName[MAX_PATH],FileSystemName[MAX_PATH];
			DWORD VolumeSerialNumber,MaximumComponentLength,FileSystemFlags;
			
			Result = GetVolumeInformation(drive,VolumeName,sizeof(VolumeName),&VolumeSerialNumber,
				&MaximumComponentLength,&FileSystemFlags,FileSystemName,sizeof(FileSystemName));
			
			if (Result && (strnicmp(VolumeName,CD_VOLUME,8) == 0 ) )
			{
				sprintf (test, "%s%s\\%s",drive, CD_BASEDIR, CD_EXE);
				f = fopen( test, "r" );
				if ( f ) {
					fclose (f);
					return qtrue;
				} else {
					sprintf(test, "%s%s\\%s", drive, CD_BASEDIR, CD_EXE_LINUX);
					f = fopen( test, "r" );
					if ( f ) {
						fclose (f);
						return qtrue;
					}
				}
			}
		}
	}

	return qfalse;
}
#endif
/*
================
Sys_CheckCD

Return true if the proper CD is in the drive
================
*/
qboolean	Sys_CheckCD( void ) {
#ifdef FINAL_BUILD
	return Sys_ScanForCD();
#else
	return qtrue;
#endif
}


/*
================
Sys_GetClipboardData

================
*/
char *Sys_GetClipboardData( void ) {
	char *data = NULL;
	char *cliptext;

	if ( OpenClipboard( NULL ) != 0 ) {
		HANDLE hClipboardData;

		if ( ( hClipboardData = GetClipboardData( CF_TEXT ) ) != 0 ) {
			if ( ( cliptext = (char *)GlobalLock( hClipboardData ) ) != 0 ) {
				data = (char *)Z_Malloc( GlobalSize( hClipboardData ) + 1, TAG_CLIPBOARD);
				Q_strncpyz( data, cliptext, GlobalSize( hClipboardData )+1 );
				GlobalUnlock( hClipboardData );
				
				strtok( data, "\n\r\b" );
			}
		}
		CloseClipboard();
	}
	return data;
}


/*
========================================================================

LOAD/UNLOAD DLL

========================================================================
*/

/*
=================
Sys_UnloadDll

=================
*/
void Sys_UnloadDll( void *dllHandle ) {
	if ( !dllHandle ) {
		return;
	}
	if ( !FreeLibrary( (struct HINSTANCE__ *)dllHandle ) ) {
		Com_Error (ERR_FATAL, "Sys_UnloadDll FreeLibrary failed");
	}
}

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

	f = FS_FOpenFileWrite( name );
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

/*
=================
Sys_LoadDll

Used to load a development dll instead of a virtual machine
=================
*/
extern char		*FS_BuildOSPath( const char *base, const char *game, const char *qpath );

void * QDECL Sys_LoadDll( const char *name, int (QDECL **entryPoint)(int, ...),
				  int (QDECL *systemcalls)(int, ...) ) {
	static int	lastWarning = 0;
	HINSTANCE	libHandle;
	void	(QDECL *dllEntry)( int (QDECL *syscallptr)(int, ...) );
	char	*basepath;
	char	*cdpath;
	char	*gamedir;
	char	*fn;
#ifdef NDEBUG
	int		timestamp;
  int   ret;
#endif
	char	filename[MAX_QPATH];

	Com_sprintf( filename, sizeof( filename ), "%sx86.dll", name );

#ifdef NDEBUG
	timestamp = Sys_Milliseconds();
//	if( ((timestamp - lastWarning) > (5 * 60000)) && !Cvar_VariableIntegerValue( "dedicated" )
//		&& !Cvar_VariableIntegerValue( "com_blindlyLoadDLLs" ) ) {
	if (0) {
		if (FS_FileExists(filename)) {
			lastWarning = timestamp;
			ret = MessageBoxEx( NULL, "You are about to load a .DLL executable that\n"
				  "has not been verified for use with Quake III Arena.\n"
				  "This type of file can compromise the security of\n"
				  "your computer.\n\n"
				  "Select 'OK' if you choose to load it anyway.",
				  "Security Warning", MB_OKCANCEL | MB_ICONEXCLAMATION | MB_DEFBUTTON2 | MB_TOPMOST | MB_SETFOREGROUND,
				  MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ) );
			if( ret != IDOK ) {
				return NULL;
			}
		}
	}
#endif

	if (!Sys_UnpackDLL(filename))
	{
		return NULL;
	}

// rjr disable for final release #ifndef NDEBUG
	libHandle = LoadLibrary( filename );
	if ( !libHandle ) {
//#endif
	basepath = Cvar_VariableString( "fs_basepath" );
	cdpath = Cvar_VariableString( "fs_cdpath" );
	gamedir = Cvar_VariableString( "fs_game" );

	fn = FS_BuildOSPath( basepath, gamedir, filename );
	libHandle = LoadLibrary( fn );

	if ( !libHandle ) {
		if( cdpath[0] ) {
			fn = FS_BuildOSPath( cdpath, gamedir, filename );
			libHandle = LoadLibrary( fn );
		}

		if ( !libHandle ) {
			return NULL;
		}
	}
//#ifndef NDEBUG
	}
//#endif

	dllEntry = ( void (QDECL *)( int (QDECL *)( int, ... ) ) )GetProcAddress( libHandle, "dllEntry" ); 
	*entryPoint = (int (QDECL *)(int,...))GetProcAddress( libHandle, "vmMain" );
	if ( !*entryPoint || !dllEntry ) {
		FreeLibrary( libHandle );
		return NULL;
	}
	dllEntry( systemcalls );

	return libHandle;
}


/*
========================================================================

BACKGROUND FILE STREAMING

========================================================================
*/

#if 1

void Sys_InitStreamThread( void ) {
}

void Sys_ShutdownStreamThread( void ) {
}

void Sys_BeginStreamedFile( fileHandle_t f, int readAhead ) {
}

void Sys_EndStreamedFile( fileHandle_t f ) {
}

int Sys_StreamedRead( void *buffer, int size, int count, fileHandle_t f ) {
   return FS_Read( buffer, size * count, f );
}

void Sys_StreamSeek( fileHandle_t f, int offset, int origin ) {
   FS_Seek( f, offset, origin );
}


#else

typedef struct {
	fileHandle_t	file;
	byte	*buffer;
	qboolean	eof;
	qboolean	active;
	int		bufferSize;
	int		streamPosition;	// next byte to be returned by Sys_StreamRead
	int		threadPosition;	// next byte to be read from file
} streamsIO_t;

typedef struct {
	HANDLE				threadHandle;
	int					threadId;
	CRITICAL_SECTION	crit;
	streamsIO_t			sIO[MAX_FILE_HANDLES];
} streamState_t;

streamState_t	stream;

/*
===============
Sys_StreamThread

A thread will be sitting in this loop forever
================
*/
void Sys_StreamThread( void ) {
	int		buffer;
	int		count;
	int		readCount;
	int		bufferPoint;
	int		r, i;

	while (1) {
		Sleep( 10 );
//		EnterCriticalSection (&stream.crit);

		for (i=1;i<MAX_FILE_HANDLES;i++) {
			// if there is any space left in the buffer, fill it up
			if ( stream.sIO[i].active  && !stream.sIO[i].eof ) {
				count = stream.sIO[i].bufferSize - (stream.sIO[i].threadPosition - stream.sIO[i].streamPosition);
				if ( !count ) {
					continue;
				}

				bufferPoint = stream.sIO[i].threadPosition % stream.sIO[i].bufferSize;
				buffer = stream.sIO[i].bufferSize - bufferPoint;
				readCount = buffer < count ? buffer : count;

				r = FS_Read( stream.sIO[i].buffer + bufferPoint, readCount, stream.sIO[i].file );
				stream.sIO[i].threadPosition += r;

				if ( r != readCount ) {
					stream.sIO[i].eof = qtrue;
				}
			}
		}
//		LeaveCriticalSection (&stream.crit);
	}
}

/*
===============
Sys_InitStreamThread

================
*/
void Sys_InitStreamThread( void ) {
	int i;

	InitializeCriticalSection ( &stream.crit );

	// don't leave the critical section until there is a
	// valid file to stream, which will cause the StreamThread
	// to sleep without any overhead
//	EnterCriticalSection( &stream.crit );

	stream.threadHandle = CreateThread(
	   NULL,	// LPSECURITY_ATTRIBUTES lpsa,
	   0,		// DWORD cbStack,
	   (LPTHREAD_START_ROUTINE)Sys_StreamThread,	// LPTHREAD_START_ROUTINE lpStartAddr,
	   0,			// LPVOID lpvThreadParm,
	   0,			//   DWORD fdwCreate,
	   &stream.threadId);
	for(i=0;i<MAX_FILE_HANDLES;i++) {
		stream.sIO[i].active = qfalse;
	}
}

/*
===============
Sys_ShutdownStreamThread

================
*/
void Sys_ShutdownStreamThread( void ) {
}


/*
===============
Sys_BeginStreamedFile

================
*/
void Sys_BeginStreamedFile( fileHandle_t f, int readAhead ) {
	if ( stream.sIO[f].file ) {
		Sys_EndStreamedFile( stream.sIO[f].file );
	}

	stream.sIO[f].file = f;
	stream.sIO[f].buffer = Z_Malloc( readAhead );
	stream.sIO[f].bufferSize = readAhead;
	stream.sIO[f].streamPosition = 0;
	stream.sIO[f].threadPosition = 0;
	stream.sIO[f].eof = qfalse;
	stream.sIO[f].active = qtrue;

	// let the thread start running
//	LeaveCriticalSection( &stream.crit );
}

/*
===============
Sys_EndStreamedFile

================
*/
void Sys_EndStreamedFile( fileHandle_t f ) {
	if ( f != stream.sIO[f].file ) {
		Com_Error( ERR_FATAL, "Sys_EndStreamedFile: wrong file");
	}
	// don't leave critical section until another stream is started
	EnterCriticalSection( &stream.crit );

	stream.sIO[f].file = 0;
	stream.sIO[f].active = qfalse;

	Z_Free( stream.sIO[f].buffer );

	LeaveCriticalSection( &stream.crit );
}


/*
===============
Sys_StreamedRead

================
*/
int Sys_StreamedRead( void *buffer, int size, int count, fileHandle_t f ) {
	int		available;
	int		remaining;
	int		sleepCount;
	int		copy;
	int		bufferCount;
	int		bufferPoint;
	byte	*dest;

	if (stream.sIO[f].active == qfalse) {
		Com_Error( ERR_FATAL, "Streamed read with non-streaming file" );
	}

	dest = (byte *)buffer;
	remaining = size * count;

	if ( remaining <= 0 ) {
		Com_Error( ERR_FATAL, "Streamed read with non-positive size" );
	}

	sleepCount = 0;
	while ( remaining > 0 ) {
		available = stream.sIO[f].threadPosition - stream.sIO[f].streamPosition;
		if ( !available ) {
			if ( stream.sIO[f].eof ) {
				break;
			}
			if ( sleepCount == 1 ) {
				Com_DPrintf( "Sys_StreamedRead: waiting\n" );
			}
			if ( ++sleepCount > 100 ) {
				Com_Error( ERR_FATAL, "Sys_StreamedRead: thread has died");
			}
			Sleep( 10 );
			continue;
		}

		EnterCriticalSection( &stream.crit );

		bufferPoint = stream.sIO[f].streamPosition % stream.sIO[f].bufferSize;
		bufferCount = stream.sIO[f].bufferSize - bufferPoint;

		copy = available < bufferCount ? available : bufferCount;
		if ( copy > remaining ) {
			copy = remaining;
		}
		memcpy( dest, stream.sIO[f].buffer + bufferPoint, copy );
		stream.sIO[f].streamPosition += copy;
		dest += copy;
		remaining -= copy;

		LeaveCriticalSection( &stream.crit );
	}

	return (count * size - remaining) / size;
}

/*
===============
Sys_StreamSeek

================
*/
void Sys_StreamSeek( fileHandle_t f, int offset, int origin ) {

	// halt the thread
	EnterCriticalSection( &stream.crit );

	// clear to that point
	FS_Seek( f, offset, origin );
	stream.sIO[f].streamPosition = 0;
	stream.sIO[f].threadPosition = 0;
	stream.sIO[f].eof = qfalse;

	// let the thread start running at the new position
	LeaveCriticalSection( &stream.crit );
}

#endif

/*
========================================================================

EVENT LOOP

========================================================================
*/

#define	MAX_QUED_EVENTS		256
#define	MASK_QUED_EVENTS	( MAX_QUED_EVENTS - 1 )

sysEvent_t	eventQue[MAX_QUED_EVENTS];
int			eventHead, eventTail;
byte		sys_packetReceived[MAX_MSGLEN];

/*
================
Sys_QueEvent

A time of 0 will get the current time
Ptr should either be null, or point to a block of data that can
be freed by the game later.
================
*/
void Sys_QueEvent( int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr ) {
	sysEvent_t	*ev;

#ifdef _XBOX
	if(ClientManager::Shared().splitScreenMode == qtrue)
	{
		ev = &ClientManager::ActiveClient().eventQue[ ClientManager::ActiveClient().eventHead & MASK_QUED_EVENTS ];
		if ( ClientManager::ActiveClient().eventHead - ClientManager::ActiveClient().eventTail >= MAX_QUED_EVENTS ) {
			Com_Printf("Sys_QueEvent: overflow\n");
			// we are discarding an event, but don't leak memory
			if ( ev->evPtr ) {
				Z_Free( ev->evPtr );
			}
			ClientManager::ActiveClient().eventTail++;
		}

		ClientManager::ActiveClient().eventHead++;
	}
	else
	{
#endif // _XBOX
	ev = &eventQue[ eventHead & MASK_QUED_EVENTS ];
	if ( eventHead - eventTail >= MAX_QUED_EVENTS ) {
		Com_Printf("Sys_QueEvent: overflow\n");
		// we are discarding an event, but don't leak memory
		if ( ev->evPtr ) {
			Z_Free( ev->evPtr );
		}
		eventTail++;
	}

	eventHead++;

#ifdef _XBOX
	}
#endif 

	if ( time == 0 ) {
		time = Sys_Milliseconds();
	}

	ev->evTime = time;
	ev->evType = type;
	ev->evValue = value;
	ev->evValue2 = value2;
	ev->evPtrLength = ptrLength;
	ev->evPtr = ptr;
}

/*
================
Sys_GetEvent

================
*/
sysEvent_t Sys_GetEvent( void ) {
    MSG			msg;
	sysEvent_t	ev;
	char		*s;
	msg_t		netmsg;
	netadr_t	adr;

	// return if we have data
#ifdef _XBOX
	if(ClientManager::Shared().splitScreenMode == qtrue)
	{
		if ( ClientManager::ActiveClient().eventHead > ClientManager::ActiveClient().eventTail ) 
		{
			ClientManager::ActiveClient().eventTail++;
			return ClientManager::ActiveClient().eventQue[ ( ClientManager::ActiveClient().eventTail - 1 ) & MASK_QUED_EVENTS ];
		}
	}
	else
	{
#endif // _XBOX
	if ( eventHead > eventTail ) {
		eventTail++;
		return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
	}
#ifdef _XBOX
	}
#endif

	// pump the message loop
	while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE)) {
		if ( !GetMessage (&msg, NULL, 0, 0) ) {
			Com_Quit_f();
		}

		// save the msg time, because wndprocs don't have access to the timestamp
		g_wv.sysMsgTime = msg.time;

		TranslateMessage (&msg);
      	DispatchMessage (&msg);
	}

	// check for console commands
	s = Sys_ConsoleInput();
	if ( s ) {
		char	*b;
		int		len;

		len = strlen( s ) + 1;
		b = (char *)Z_Malloc( len, TAG_EVENT );
		Q_strncpyz( b, s, len );
		Sys_QueEvent( 0, SE_CONSOLE, 0, 0, len, b );
	}

	// check for network packets
	MSG_Init( &netmsg, sys_packetReceived, sizeof( sys_packetReceived ) );
	if ( Sys_GetPacket ( &adr, &netmsg ) ) {
		netadr_t		*buf;
		int				len;

		// copy out to a seperate buffer for qeueing
		// the readcount stepahead is for SOCKS support
		len = sizeof( netadr_t ) + netmsg.cursize - netmsg.readcount;
		buf = (netadr_t *)Z_Malloc( len, TAG_EVENT, qtrue );
		*buf = adr;
		memcpy( buf+1, &netmsg.data[netmsg.readcount], netmsg.cursize - netmsg.readcount );
		Sys_QueEvent( 0, SE_PACKET, 0, 0, len, buf );
	}

	// return if we have data
#ifdef _XBOX
	if(ClientManager::Shared().splitScreenMode == qtrue)
	{
		if ( ClientManager::ActiveClient().eventHead > ClientManager::ActiveClient().eventTail ) 
		{
			ClientManager::ActiveClient().eventTail++;
			return ClientManager::ActiveClient().eventQue[ ( ClientManager::ActiveClient().eventTail - 1 ) & MASK_QUED_EVENTS ];
		}
	}
	else
	{
#endif // _XBOX
	if ( eventHead > eventTail ) {
		eventTail++;
		return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
	}
#ifdef _XBOX
	}
#endif

	// create an empty event to return

	memset( &ev, 0, sizeof( ev ) );
	ev.evTime = timeGetTime();

	return ev;
}

//================================================================

/*
=================
Sys_In_Restart_f

Restart the input subsystem
=================
*/
void Sys_In_Restart_f( void ) {
	IN_Shutdown();
	IN_Init();
}


/*
=================
Sys_Net_Restart_f

Restart the network subsystem
=================
*/
void Sys_Net_Restart_f( void ) {
	NET_Restart();
}

static bool Sys_IsExpired()
{
#if 0
//								sec min Hr Day Mon Yr
    struct tm t_valid_start	= { 0, 0, 8, 23, 6, 103 };	//zero based months!
//								sec min Hr Day Mon Yr
    struct tm t_valid_end	= { 0, 0, 20, 30, 6, 103 };
//    struct tm t_valid_end	= t_valid_start;
//	t_valid_end.tm_mday += 8;
	time_t startTime  = mktime( &t_valid_start);
	time_t expireTime = mktime( &t_valid_end);
	time_t now;
	time(&now);
	if((now < startTime) || (now> expireTime))
	{
		return true;
	}
#endif
	return false;
}

/*
================
Sys_Init

Called after the common systems (cvars, files, etc)
are initialized
================
*/
#define OSR2_BUILD_NUMBER 1111
#define WIN98_BUILD_NUMBER 1998

void Sys_Init( void ) {
	int cpuid;

	// make sure the timer is high precision, otherwise
	// NT gets 18ms resolution
	timeBeginPeriod( 1 );

	Cmd_AddCommand ("in_restart", Sys_In_Restart_f);
	Cmd_AddCommand ("net_restart", Sys_Net_Restart_f);

	g_wv.osversion.dwOSVersionInfoSize = sizeof( g_wv.osversion );

	if (!GetVersionEx (&g_wv.osversion))
		Sys_Error ("Couldn't get OS info");
	if (Sys_IsExpired()) {
		g_wv.osversion.dwPlatformId = VER_PLATFORM_WIN32s;	//sneaky: hide the expire with this error
	}

	if (g_wv.osversion.dwMajorVersion < 4)
		Sys_Error ("This game requires Windows version 4 or greater");
	if (g_wv.osversion.dwPlatformId == VER_PLATFORM_WIN32s)
		Sys_Error ("This game doesn't run on Win32s");

	if ( g_wv.osversion.dwPlatformId == VER_PLATFORM_WIN32_NT )
	{
		Cvar_Set( "arch", "winnt" );
	}
	else if ( g_wv.osversion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
	{
		if ( LOWORD( g_wv.osversion.dwBuildNumber ) >= WIN98_BUILD_NUMBER )
		{
			Cvar_Set( "arch", "win98" );
		}
		else if ( LOWORD( g_wv.osversion.dwBuildNumber ) >= OSR2_BUILD_NUMBER )
		{
			Cvar_Set( "arch", "win95 osr2.x" );
		}
		else
		{
			Cvar_Set( "arch", "win95" );
		}
	}
	else
	{
		Cvar_Set( "arch", "unknown Windows variant" );
	}

	// save out a couple things in rom cvars for the renderer to access
	Cvar_Get( "win_hinstance", va("%i", (int)g_wv.hInstance), CVAR_ROM );
	Cvar_Get( "win_wndproc", va("%i", (int)MainWndProc), CVAR_ROM );

	//
	// figure out our CPU
	//
	Cvar_Get( "sys_cpustring", "detect", CVAR_ROM );
	if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring"), "detect" ) )
	{
		Com_Printf( "...detecting CPU, found " );

		cpuid = Sys_GetProcessorId();

		switch ( cpuid )
		{
		case CPUID_GENERIC:
			Cvar_Set( "sys_cpustring", "generic" );
			break;
		case CPUID_INTEL_UNSUPPORTED:
			Cvar_Set( "sys_cpustring", "x86 (pre-Pentium)" );
			break;
		case CPUID_INTEL_PENTIUM:
			Cvar_Set( "sys_cpustring", "x86 (P5/PPro, non-MMX)" );
			break;
		case CPUID_INTEL_MMX:
			Cvar_Set( "sys_cpustring", "x86 (P5/Pentium2, MMX)" );
			break;
		case CPUID_INTEL_KATMAI:
			Cvar_Set( "sys_cpustring", "Intel Pentium III" );
			break;
		case CPUID_INTEL_WILLIAMETTE:
			Cvar_Set( "sys_cpustring", "Intel Pentium IV" );
			break;
		case CPUID_AMD_3DNOW:
			Cvar_Set( "sys_cpustring", "AMD w/ 3DNow!" );
			break;
		case CPUID_AXP:
			Cvar_Set( "sys_cpustring", "Alpha AXP" );
			break;
		default:
			Com_Error( ERR_FATAL, "Unknown cpu type %d\n", cpuid );
			break;
		}
	}
	else
	{
		Com_Printf( "...forcing CPU type to " );
		if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring" ), "generic" ) )
		{
			cpuid = CPUID_GENERIC;
		}
		else if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring" ), "x87" ) )
		{
			cpuid = CPUID_INTEL_PENTIUM;
		}
		else if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring" ), "mmx" ) )
		{
			cpuid = CPUID_INTEL_MMX;
		}
		else if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring" ), "3dnow" ) )
		{
			cpuid = CPUID_AMD_3DNOW;
		}
		else if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring" ), "PentiumIII" ) )
		{
			cpuid = CPUID_INTEL_KATMAI;
		}
		else if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring" ), "PentiumIV" ) )
		{
			cpuid = CPUID_INTEL_WILLIAMETTE;
		}
		else if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring" ), "axp" ) )
		{
			cpuid = CPUID_AXP;
		}
		else
		{
			Com_Printf( "WARNING: unknown sys_cpustring '%s'\n", Cvar_VariableString( "sys_cpustring" ) );
			cpuid = CPUID_GENERIC;
		}
	}
	Cvar_SetValue( "sys_cpuid", cpuid );
	Com_Printf( "%s\n", Cvar_VariableString( "sys_cpustring" ) );

	Cvar_Set( "username", Sys_GetCurrentUser() );
	Cvar_SetValue( "sys_cpuspeed", Sys_GetCPUSpeed() );
	Cvar_SetValue( "sys_memory", Sys_GetPhysicalMemory() );

	IN_Init();		// FIXME: not in dedicated?
}

// do a quick mem test to check for any potential future mem problems...
//
void QuickMemTest(void)
{
//	if (!Sys_LowPhysicalMemory())
	{
		const int iMemTestMegs = 128;	// useful search label
		// special test, 
		void *pvData = malloc(iMemTestMegs * 1024 * 1024);
		if (pvData)
		{
			free(pvData);
		}
		else
		{
			// err...
			//
			extern qboolean Language_IsAsian(void);
			LPCSTR psContinue = Language_IsAsian() ? 
								"Your machine failed to allocate %dMB in a memory test, which may mean you'll have problems running this game all the way through.\n\nContinue anyway?"
								: 
								SE_GetString("CON_TEXT_FAILED_MEMTEST");
								// ( since it's too much hassle doing MBCS code pages and decodings etc for MessageBox command )

			#define GetYesNo(psQuery)	(!!(MessageBox(NULL,psQuery,"Query",MB_YESNO|MB_ICONWARNING|MB_TASKMODAL)==IDYES))
			if (!GetYesNo(va(psContinue,iMemTestMegs)))
			{
				LPCSTR psNoMem = Language_IsAsian() ?
								"Insufficient memory to run this game!\n"
								:
								SE_GetString("CON_TEXT_INSUFFICIENT_MEMORY");
								// ( since it's too much hassle doing MBCS code pages and decodings etc for MessageBox command )

				Com_Error( ERR_FATAL, psNoMem );
			}
		}
	}
}


//=======================================================================
//int	totalMsec, countMsec;

/*
==================
WinMain

==================
*/
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
//	int			startTime, endTime;

    // should never get a previous instance in Win32
    if ( hPrevInstance ) {
        return 0;
	}

	sys_checksum = Sys_CodeInMemoryChecksum( hInstance );
	Sys_VerifyCodeChecksum( hInstance );

	g_wv.hInstance = hInstance;
	Q_strncpyz( sys_cmdline, lpCmdLine, sizeof( sys_cmdline ) );

	// done before Com/Sys_Init since we need this for error output
	Sys_CreateConsole();

	// no abort/retry/fail errors
	SetErrorMode( SEM_FAILCRITICALERRORS );

	// get the initial time base
	Sys_Milliseconds();

#if 0
	// if we find the CD, add a +set cddir xxx command line
	Sys_ScanForCD();
#endif


	Sys_InitStreamThread();

	Com_Init( sys_cmdline );

#if !defined(DEDICATED)
		QuickMemTest();
#endif

	NET_Init();

	// hide the early console since we've reached the point where we
	// have a working graphics subsystems
	if ( !com_dedicated->integer && !com_viewlog->integer ) {
		Sys_ShowConsole( 0, qfalse );
	}

#ifdef _DEBUG
	if ( sys_monkeySpank ) {
		Cvar_Set("cl_trn", "666");
	}
#endif

    // main game loop
	while( 1 ) {
		// if not running as a game client, sleep a bit
		if ( g_wv.isMinimized || ( com_dedicated && com_dedicated->integer ) ) {
			Sleep( 5 );
		}

#ifdef _DEBUG
		if (!g_wv.activeApp)
		{
			Sleep(50);
		}
#endif // _DEBUG

		// set low precision every frame, because some system calls
		// reset it arbitrarily
//		_controlfp( _PC_24, _MCW_PC );
 
//		startTime = Sys_Milliseconds();

		// make sure mouse and joystick are only called once a frame
		IN_Frame();

		// run the game
		Com_Frame();

//		endTime = Sys_Milliseconds();
//		totalMsec += endTime - startTime;
//		countMsec++;
	}

	// never gets here
}


