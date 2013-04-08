// common.c -- misc functions used in client and server

#include "../game/q_shared.h"
#include "qcommon.h"
#include "../qcommon/sstring.h"	// to get Gil's string class, because MS's doesn't compile properly in here
#include "stv_version.h"

#ifdef _WIN32
#include <windows.h>	// for Sleep for Z_Malloc recovery attempy
#endif


#define	MAXPRINTMSG	4096

#define MAX_NUM_ARGVS	50

#ifdef DEBUG_ZONE_ALLOCS
int giZoneSnaphotNum=0;
#define DEBUG_ZONE_ALLOC_OPTIONAL_LABEL_SIZE 256
typedef sstring<DEBUG_ZONE_ALLOC_OPTIONAL_LABEL_SIZE> sDebugString_t;
#endif

static void Z_Details_f(void);

// define a string table of all mem tags...
//
#ifdef TAGDEF	// itu?
#undef TAGDEF
#endif
#define TAGDEF(blah) #blah
static const char *psTagStrings[TAG_COUNT+1]=	// +1 because TAG_COUNT will itself become a string here. Oh well.
{
	#include "../qcommon/tags.h"
};


int		com_argc;
char	*com_argv[MAX_NUM_ARGVS+1];

static fileHandle_t	logfile;
static fileHandle_t	speedslog;
static fileHandle_t	camerafile;
fileHandle_t	com_journalFile;
fileHandle_t	com_journalDataFile;		// config files are written here

cvar_t	*com_viewlog;
cvar_t	*com_speeds;
cvar_t	*com_developer;
cvar_t	*com_timescale;
cvar_t	*com_fixedtime;
cvar_t	*com_journal;
cvar_t	*com_maxfps;
cvar_t	*com_sv_running;
cvar_t	*com_cl_running;
cvar_t	*com_logfile;		// 1 = buffer log, 2 = flush after each print
cvar_t	*com_showtrace;
cvar_t	*com_version;
cvar_t	*com_buildScript;	// for automated data building scripts
//cvar_t	*com_FirstTime;
cvar_t	*cl_paused;
cvar_t	*sv_paused;
cvar_t	*com_skippingcin;
cvar_t	*com_speedslog;		// 1 = buffer log, 2 = flush after each print

// com_speeds times
int		time_game;
int		time_frontend;		// renderer frontend time
int		time_backend;		// renderer backend time

int		timeInTrace;
int		timeInPVSCheck;
int		numTraces;

int			com_frameTime;
int			com_frameMsec;
int			com_frameNumber;

qboolean	com_errorEntered;
qboolean	com_fullyInitialized = qfalse;

char	com_errorMessage[MAXPRINTMSG];

void Com_WriteConfig_f( void );

//============================================================================

static char	*rd_buffer;
static int	rd_buffersize;
static void	(*rd_flush)( char *buffer );

void Com_BeginRedirect (char *buffer, int buffersize, void (*flush)( char *) )
{
	if (!buffer || !buffersize || !flush)
		return;
	rd_buffer = buffer;
	rd_buffersize = buffersize;
	rd_flush = flush;

	*rd_buffer = 0;
}

void Com_EndRedirect (void)
{
	if ( rd_flush ) {
		rd_flush(rd_buffer);
	}

	rd_buffer = NULL;
	rd_buffersize = 0;
	rd_flush = NULL;
}

/*
=============
Com_Printf

Both client and server can use this, and it will output
to the apropriate place.

A raw string should NEVER be passed as fmt, because of "%f" type crashers.
=============
*/
void QDECL Com_Printf( const char *fmt, ... ) {
	va_list		argptr;
	char		msg[MAXPRINTMSG];

	va_start (argptr,fmt);
	vsprintf (msg,fmt,argptr);
	va_end (argptr);

	if ( rd_buffer ) {
		if ((strlen (msg) + strlen(rd_buffer)) > (rd_buffersize - 1)) {
			rd_flush(rd_buffer);
			*rd_buffer = 0;
		}
		strcat (rd_buffer, msg);
		return;
	}

	CL_ConsolePrint( msg );

	// echo to dedicated console and early console
	Sys_Print( msg );

	// logfile
	if ( com_logfile && com_logfile->integer ) {
		if ( !logfile ) {
			logfile = FS_FOpenFileWrite( "qconsole.log" );
			if ( com_logfile->integer > 1 ) {
				// force it to not buffer so we get valid
				// data even if we are crashing
				FS_ForceFlush(logfile);
			}
		}
		if ( logfile ) {
			FS_Write(msg, strlen(msg), logfile);
		}
	}
}


/*
================
Com_DPrintf

A Com_Printf that only shows up if the "developer" cvar is set
================
*/
void QDECL Com_DPrintf( const char *fmt, ...) {
	va_list		argptr;
	char		msg[MAXPRINTMSG];
		
	if ( !com_developer || !com_developer->integer ) {
		return;			// don't confuse non-developers with techie stuff...
	}

	va_start (argptr,fmt);
	vsprintf (msg,fmt,argptr);
	va_end (argptr);
	
	Com_Printf ("%s", msg);
}

void Com_WriteCam ( const char *text )
{
	static	char	mapname[MAX_QPATH];
	// camerafile
	if ( !camerafile ) 
	{
		extern	cvar_t	*sv_mapname;

		//NOTE: always saves in working dir if using one...
		sprintf( mapname, "maps/%s_cam.map", sv_mapname->string );
		camerafile = FS_FOpenFileWrite( mapname );
	}

	if ( camerafile ) 
	{
		FS_Printf( camerafile, "%s", text );
	}

	Com_Printf( "%s\n", mapname );
}

/*
=============
Com_Error

Both client and server can use this, and it will
do the apropriate things.
=============
*/

void SG_WipeSavegame(const char *name);	// pretty sucky, but that's how SoF did it...<g>
void SG_Shutdown();
void SCR_UnprecacheScreenshot();

void QDECL Com_Error( int code, const char *fmt, ... ) {
	va_list		argptr;

	// when we are running automated scripts, make sure we
	// know if anything failed
	if ( com_buildScript && com_buildScript->integer ) {
		code = ERR_FATAL;
	}

	if ( com_errorEntered ) {
		Sys_Error( "recursive error after: %s", com_errorMessage );
	}
	
	com_errorEntered = qtrue;

	//reset some game stuff here
	SCR_UnprecacheScreenshot();

	va_start (argptr,fmt);
	vsprintf (com_errorMessage,fmt,argptr);
	va_end (argptr);	

	if ( code != ERR_DISCONNECT ) {
		Cvar_Get("com_errorMessage", "", CVAR_ROM);	//give com_errorMessage a default so it won't come back to life after a resetDefaults
		Cvar_Set("com_errorMessage", com_errorMessage);
	}

	SG_Shutdown();				// close any file pointers
	if ( code == ERR_DISCONNECT ) {
		CL_Disconnect();
		CL_FlushMemory();
		CL_StartHunkUsers();
		com_errorEntered = qfalse;
		throw ("DISCONNECTED\n");
	} else if ( code == ERR_DROP ) {
		// If loading/saving caused the crash/error - delete the temp file
		SG_WipeSavegame("current");	// delete file

		SV_Shutdown (va("Server crashed: %s\n",  com_errorMessage));
		CL_Disconnect();
		CL_FlushMemory();
		CL_StartHunkUsers();
		Com_Printf (S_COLOR_RED"********************\n"S_COLOR_MAGENTA"ERROR: %s\n"S_COLOR_RED"********************\n", com_errorMessage);
		com_errorEntered = qfalse;
		throw ("DROPPED\n");
	} else if ( code == ERR_NEED_CD ) {
		SV_Shutdown( "Server didn't have CD\n" );
		if ( com_cl_running && com_cl_running->integer ) {
			CL_Disconnect();
			CL_FlushMemory();
			CL_StartHunkUsers();
			com_errorEntered = qfalse;
		} else {
			Com_Printf("Server didn't have CD\n" );
		}
		throw ("NEED CD\n");
	} else {
		CL_Shutdown ();
		SV_Shutdown (va(S_COLOR_RED"Server fatal crashed: %s\n", com_errorMessage));
	}

	Com_Shutdown ();

	Sys_Error ("%s", com_errorMessage);
}


/*
=============
Com_Quit_f

Both client and server can use this, and it will
do the apropriate things.
=============
*/
void Com_Quit_f( void ) {
	// don't try to shutdown if we are in a recursive error
	if ( !com_errorEntered ) {
		SV_Shutdown ("Server quit\n");
		CL_Shutdown ();
		Com_Shutdown ();
	}
	Sys_Quit ();
}



/*
============================================================================

COMMAND LINE FUNCTIONS

+ characters seperate the commandLine string into multiple console
command lines.

All of these are valid:

quake3 +set test blah +map test
quake3 set test blah+map test
quake3 set test blah + map test

============================================================================
*/

#define	MAX_CONSOLE_LINES	32
int		com_numConsoleLines;
char	*com_consoleLines[MAX_CONSOLE_LINES];

/*
==================
Com_ParseCommandLine

Break it up into multiple console lines
==================
*/
void Com_ParseCommandLine( char *commandLine ) {
	com_consoleLines[0] = commandLine;
	com_numConsoleLines = 1;

	while ( *commandLine ) {
		// look for a + seperating character
		// if commandLine came from a file, we might have real line seperators
		if ( *commandLine == '+' || *commandLine == '\n' ) {
			if ( com_numConsoleLines == MAX_CONSOLE_LINES ) {
				return;
			}
			com_consoleLines[com_numConsoleLines] = commandLine + 1;
			com_numConsoleLines++;
			*commandLine = 0;
		}
		commandLine++;
	}
}


/*
===================
Com_SafeMode

Check for "safe" on the command line, which will
skip loading of jk2config.cfg
===================
*/
qboolean Com_SafeMode( void ) {
	int		i;

	for ( i = 0 ; i < com_numConsoleLines ; i++ ) {
		Cmd_TokenizeString( com_consoleLines[i] );
		if ( !Q_stricmp( Cmd_Argv(0), "safe" )
			|| !Q_stricmp( Cmd_Argv(0), "cvar_restart" ) ) {
			com_consoleLines[i][0] = 0;
			return qtrue;
		}
	}
	return qfalse;
}


/*
===============
Com_StartupVariable

Searches for command line parameters that are set commands.
If match is not NULL, only that cvar will be looked for.
That is necessary because cddir and basedir need to be set
before the filesystem is started, but all other sets should
be after execing the config and default.
===============
*/
void Com_StartupVariable( const char *match ) {
	int		i;
	char	*s;
	cvar_t	*cv;

	for (i=0 ; i < com_numConsoleLines ; i++) {
		Cmd_TokenizeString( com_consoleLines[i] );
		if ( strcmp( Cmd_Argv(0), "set" ) ) {
			continue;
		}

		s = Cmd_Argv(1);
		if ( !match || !stricmp( s, match ) ) {
			Cvar_Set( s, Cmd_Argv(2) );
			cv = Cvar_Get( s, "", 0 );
			cv->flags |= CVAR_USER_CREATED;
//			com_consoleLines[i] = 0;
		}
	}
}


/*
=================
Com_AddStartupCommands

Adds command line parameters as script statements
Commands are seperated by + signs

Returns qtrue if any late commands were added, which
will keep the demoloop from immediately starting
=================
*/
qboolean Com_AddStartupCommands( void ) {
	int		i;
	qboolean	added;

	added = qfalse;
	// quote every token, so args with semicolons can work
	for (i=0 ; i < com_numConsoleLines ; i++) {
		if ( !com_consoleLines[i] || !com_consoleLines[i][0] ) {
			continue;
		}

		// set commands won't override menu startup
		if ( Q_stricmpn( com_consoleLines[i], "set", 3 ) ) {
			added = qtrue;
		}
		Cbuf_AddText( com_consoleLines[i] );
		Cbuf_AddText( "\n" );
	}

	return added;
}


//============================================================================


void Info_Print( const char *s ) {
	char	key[512];
	char	value[512];
	char	*o;
	int		l;

	if (*s == '\\')
		s++;
	while (*s)
	{
		o = key;
		while (*s && *s != '\\')
			*o++ = *s++;

		l = o - key;
		if (l < 20)
		{
			memset (o, ' ', 20-l);
			key[20] = 0;
		}
		else
			*o = 0;
		Com_Printf ("%s", key);

		if (!*s)
		{
			Com_Printf ("MISSING VALUE\n");
			return;
		}

		o = value;
		s++;
		while (*s && *s != '\\')
			*o++ = *s++;
		*o = 0;

		if (*s)
			s++;
		Com_Printf ("%s\n", value);
	}
}

/*
============
Com_StringContains
============
*/
char *Com_StringContains(char *str1, char *str2, int casesensitive) {
	int len, i, j;

	len = strlen(str1) - strlen(str2);
	for (i = 0; i <= len; i++, str1++) {
		for (j = 0; str2[j]; j++) {
			if (casesensitive) {
				if (str1[j] != str2[j]) {
					break;
				}
			}
			else {
				if (toupper(str1[j]) != toupper(str2[j])) {
					break;
				}
			}
		}
		if (!str2[j]) {
			return str1;
		}
	}
	return NULL;
}

/*
============
Com_Filter
============
*/
int Com_Filter(char *filter, char *name, int casesensitive) {
	char buf[MAX_TOKEN_CHARS];
	char *ptr;
	int i;

	while(*filter) {
		if (*filter == '*') {
			filter++;
			for (i = 0; *filter; i++) {
				if (*filter == '*' || *filter == '?') {
					break;
				}
				buf[i] = *filter;
				filter++;
			}
			buf[i] = '\0';
			if (strlen(buf)) {
				ptr = Com_StringContains(name, buf, casesensitive);
				if (!ptr) {
					return qfalse;
				}
				name = ptr + strlen(buf);
			}
		}
		else if (*filter == '?') {
			filter++;
			name++;
		}
		else {
			if (casesensitive) {
				if (*filter != *name) {
					return qfalse;
				}
			}
			else {
				if (toupper(*filter) != toupper(*name)) {
					return qfalse;
				}
			}
			filter++;
			name++;
		}
	}
	return qtrue;
}



// This handles zone memory allocation.
// It is a wrapper around malloc with a tag id and a magic number at the start

#define ZONE_MAGIC			0x21436587

// if you change ANYTHING in this structure, be sure to update the tables below using DEF_STATIC...
//
typedef struct zoneHeader_s
{
		int					iMagic;
		memtag_t			eTag;
		int					iSize;
struct	zoneHeader_s		*pNext;
struct	zoneHeader_s		*pPrev;

#ifdef DEBUG_ZONE_ALLOCS
		char				sSrcFileBaseName[MAX_QPATH];
		int					iSrcFileLineNum;
		char				sOptionalLabel[DEBUG_ZONE_ALLOC_OPTIONAL_LABEL_SIZE];
		int					iSnapshotNumber;
#endif

} zoneHeader_t;

typedef struct 
{
	int iMagic;

} zoneTail_t;

static inline zoneTail_t *ZoneTailFromHeader(zoneHeader_t *pHeader)
{
	return (zoneTail_t*) ( (char*)pHeader + sizeof(*pHeader) + pHeader->iSize );
}

#ifdef DETAILED_ZONE_DEBUG_CODE
map <void*,int> mapAllocatedZones;
#endif


typedef struct zoneStats_s
{
	int		iCount;
	int		iCurrent;
	int		iPeak;

	// I'm keeping these updated on the fly, since it's quicker for cache-pool
	//	purposes rather than recalculating each time...
	//
	int		iSizesPerTag [TAG_COUNT];	
	int		iCountsPerTag[TAG_COUNT];	

} zoneStats_t;

typedef struct zone_s
{
	zoneStats_t				Stats;
	zoneHeader_t			Header;
} zone_t;

cvar_t	*com_validateZone;

zone_t	TheZone = {0};




// Scans through the linked list of mallocs and makes sure no data has been overwritten

void Z_Validate(void)
{	
	if(!com_validateZone || !com_validateZone->integer)
	{
		return;
	}

	zoneHeader_t *pMemory = TheZone.Header.pNext;
	while (pMemory)
	{
		#ifdef DETAILED_ZONE_DEBUG_CODE
		// this won't happen here, but wtf?
		int& iAllocCount = mapAllocatedZones[pMemory];
		if (iAllocCount <= 0)
		{
			Com_Error(ERR_FATAL, "Z_Validate(): Bad block allocation count!");
			return;
		}		   
		#endif

		if(pMemory->iMagic != ZONE_MAGIC)
		{
			Com_Error(ERR_FATAL, "Z_Validate(): Corrupt zone header!");
			return;
		}

		if (ZoneTailFromHeader(pMemory)->iMagic != ZONE_MAGIC)
		{
			Com_Error(ERR_FATAL, "Z_Validate(): Corrupt zone tail!");
			return;
		}
		
		pMemory = pMemory->pNext;
	}
}
				


// static mem blocks to reduce a lot of small zone overhead
//
#pragma pack(push)
#pragma pack(1)
typedef struct
{
	zoneHeader_t	Header;	
//	byte mem[0];
	zoneTail_t		Tail;
} StaticZeroMem_t;

typedef struct
{
	zoneHeader_t	Header;	
	byte mem[2];
	zoneTail_t		Tail;
} StaticMem_t;
#pragma pack(pop)

const static StaticZeroMem_t gZeroMalloc  =
	{ {ZONE_MAGIC, TAG_STATIC,0,NULL,NULL},{ZONE_MAGIC}};

#ifdef DEBUG_ZONE_ALLOCS
#define DEF_STATIC(_char) {ZONE_MAGIC, TAG_STATIC,2,NULL,NULL, "<static>",0,"",0},_char,'\0',{ZONE_MAGIC}
#else
#define DEF_STATIC(_char) {ZONE_MAGIC, TAG_STATIC,2,NULL,NULL			        },_char,'\0',{ZONE_MAGIC}	
#endif

const static StaticMem_t gEmptyString =
	{ DEF_STATIC('\0') };

const static StaticMem_t gNumberString[] = {
	{ DEF_STATIC('0') },
	{ DEF_STATIC('1') },
	{ DEF_STATIC('2') },
	{ DEF_STATIC('3') },
	{ DEF_STATIC('4') },
	{ DEF_STATIC('5') },
	{ DEF_STATIC('6') },
	{ DEF_STATIC('7') },
	{ DEF_STATIC('8') },
	{ DEF_STATIC('9') },
};

qboolean gbMemFreeupOccured = qfalse;

#ifdef DEBUG_ZONE_ALLOCS
void *_D_Z_Malloc ( int iSize, memtag_t eTag, qboolean bZeroit, const char *psFile, int iLine)
#else
void *Z_Malloc(int iSize, memtag_t eTag, qboolean bZeroit)
#endif
{	
	gbMemFreeupOccured = qfalse;

	if (iSize == 0)
	{
		zoneHeader_t *pMemory = (zoneHeader_t *) &gZeroMalloc;
		return &pMemory[1];
	}

	// Add in tracking info and round to a longword...  (ignore longword aligning now we're not using contiguous blocks)
	//
//	int iRealSize = (iSize + sizeof(zoneHeader_t) + sizeof(zoneTail_t) + 3) & 0xfffffffc;
	int iRealSize = (iSize + sizeof(zoneHeader_t) + sizeof(zoneTail_t));

	// Allocate a chunk...
	//
	zoneHeader_t *pMemory = NULL;
	while (pMemory == NULL)
	{
		#ifdef _WIN32
		if (gbMemFreeupOccured)
		{
			Sleep(100);	// sleep for 1/10 of a second, so Windows has a chance to shuffle mem to de-swiss-cheese it
		}
		#endif

		pMemory = (zoneHeader_t *) malloc ( iRealSize );
		if (!pMemory)
		{
			// new bit, if we fail to malloc memory, try dumping some of the cached stuff that's non-vital and try again...
			//

			// ditch the BSP cache...
			//
			if (CM_DeleteCachedMap(qfalse))
			{
				gbMemFreeupOccured = qtrue;
				continue;		// we've just ditched a whole load of memory, so try again with the malloc
			}


			// ditch any sounds not used on this level...
			//
			extern qboolean SND_RegisterAudio_LevelLoadEnd(qboolean bDeleteEverythingNotUsedThisLevel);
			if (SND_RegisterAudio_LevelLoadEnd(qtrue))
			{
				gbMemFreeupOccured = qtrue;
				continue;		// we've dropped at least one sound, so try again with the malloc
			}


			// ditch any image_t's (and associated GL texture mem) not used on this level...
			//
			extern qboolean RE_RegisterImages_LevelLoadEnd(void);
			if (RE_RegisterImages_LevelLoadEnd())
			{
				gbMemFreeupOccured = qtrue;
				continue;		// we've dropped at least one image, so try again with the malloc
			}


			// ditch the model-binaries cache...  (must be getting desperate here!)
			//
			extern qboolean RE_RegisterModels_LevelLoadEnd(qboolean bDeleteEverythingNotUsedThisLevel);
			if (RE_RegisterModels_LevelLoadEnd(qtrue))
			{
				gbMemFreeupOccured = qtrue;
				continue;
			}

			// as a last panic measure, dump all the audio memory, but not if we're in the audio loader 
			//	(which is annoying, but I'm not sure how to ensure we're not dumping any memory needed by the sound
			//	currently being loaded if that was the case)...
			//
			// note that this keeps querying until it's freed up as many bytes as the requested size, but freeing
			//	several small blocks might not mean that one larger one is satisfiable after freeup, however that'll
			//	just make it go round again and try for freeing up another bunch of blocks until the total is satisfied 
			//	again (though this will have freed twice the requested amount in that case), so it'll either work 
			//	eventually or not free up enough and drop through to the final ERR_DROP. No worries...
			//
			extern qboolean gbInsideLoadSound;			
			extern int SND_FreeOldestSound(void);	// I had to add a void-arg version of this because of link issues, sigh
			if (!gbInsideLoadSound)
			{
				int iBytesFreed = SND_FreeOldestSound();
				if (iBytesFreed)
				{						
					int iTheseBytesFreed = 0;
					while ( (iTheseBytesFreed = SND_FreeOldestSound()) != 0)
					{
						iBytesFreed += iTheseBytesFreed;
						if (iBytesFreed >= iRealSize)
							break;	// early opt-out since we've managed to recover enough (mem-contiguity issues aside)
					}
					gbMemFreeupOccured = qtrue;
					continue;
				}
			}

			// sigh, dunno what else to try, I guess we'll have to give up and report this as an out-of-mem error...
			//
			// findlabel:  "recovermem"

			Com_Printf(S_COLOR_RED"Z_Malloc(): Failed to alloc %d bytes (TAG_%s) !!!!!\n", iSize, psTagStrings[eTag]);
			Z_Details_f();
			Com_Error(ERR_FATAL,"(Repeat): Z_Malloc(): Failed to alloc %d bytes (TAG_%s) !!!!!\n", iSize, psTagStrings[eTag]);
			return NULL;
		}
	}


#ifdef DEBUG_ZONE_ALLOCS
	extern char *Filename_WithoutPath(const char *psFilename);

	Q_strncpyz(pMemory->sSrcFileBaseName, Filename_WithoutPath(psFile), sizeof(pMemory->sSrcFileBaseName));
	pMemory->iSrcFileLineNum	= iLine;	
	pMemory->sOptionalLabel[0]	= '\0';
	pMemory->iSnapshotNumber	= giZoneSnaphotNum;
#endif

	// Link in
	pMemory->iMagic	= ZONE_MAGIC;
	pMemory->eTag	= eTag;
	pMemory->iSize	= iSize;	
	pMemory->pNext  = TheZone.Header.pNext;
	TheZone.Header.pNext = pMemory;
	if (pMemory->pNext)
	{
		pMemory->pNext->pPrev = pMemory;
	}
	pMemory->pPrev = &TheZone.Header;
	//
	// add tail...
	//
	ZoneTailFromHeader(pMemory)->iMagic = ZONE_MAGIC;

	// Update stats...
	//
	TheZone.Stats.iCurrent += iSize;
	TheZone.Stats.iCount++;
	TheZone.Stats.iSizesPerTag	[eTag] += iSize;
	TheZone.Stats.iCountsPerTag	[eTag]++;	

	if (TheZone.Stats.iCurrent > TheZone.Stats.iPeak)
	{
		TheZone.Stats.iPeak	= TheZone.Stats.iCurrent;
	}

#ifdef DETAILED_ZONE_DEBUG_CODE
	mapAllocatedZones[pMemory]++;
#endif
	
	Z_Validate();	// check for corruption

	void *pvReturnMem = &pMemory[1];
	if (bZeroit) {
		memset(pvReturnMem, 0, iSize);
	}
	return pvReturnMem;
}


static void Zone_FreeBlock(zoneHeader_t *pMemory)
{
	if (pMemory->eTag != TAG_STATIC)	// belt and braces, should never hit this though
	{
		// Update stats...
		//
		TheZone.Stats.iCount--;
		TheZone.Stats.iCurrent -= pMemory->iSize;
		TheZone.Stats.iSizesPerTag	[pMemory->eTag] -= pMemory->iSize;
		TheZone.Stats.iCountsPerTag	[pMemory->eTag]--;

		// Sanity checks...
		//
		assert(pMemory->pPrev->pNext == pMemory);
		assert(!pMemory->pNext || (pMemory->pNext->pPrev == pMemory));

		// Unlink and free...
		//
		pMemory->pPrev->pNext = pMemory->pNext;
		if(pMemory->pNext)
		{
			pMemory->pNext->pPrev = pMemory->pPrev;
		}
		free (pMemory);

		
		#ifdef DETAILED_ZONE_DEBUG_CODE
		// this has already been checked for in execution order, but wtf?
		int& iAllocCount = mapAllocatedZones[pMemory];
		if (iAllocCount == 0)
		{
			Com_Error(ERR_FATAL, "Zone_FreeBlock(): Double-freeing block!");
			return;
		}
		iAllocCount--;	
		#endif
	}
}

// stats-query function to ask how big a malloc is...
//
int Z_Size(void *pvAddress)
{
	zoneHeader_t *pMemory = ((zoneHeader_t *)pvAddress) - 1;

	if (pMemory->eTag == TAG_STATIC)
	{
		return 0;	// kind of
	}

	if (pMemory->iMagic != ZONE_MAGIC)
	{
		Com_Error(ERR_FATAL, "Z_Size(): Not a valid zone header!");
		return 0;	// won't get here
	}

	return pMemory->iSize;
}


#ifdef DEBUG_ZONE_ALLOCS
void _D_Z_Label(const void *pvAddress, const char *psLabel)
{
	zoneHeader_t *pMemory = ((zoneHeader_t *)pvAddress) - 1;

	if (pMemory->eTag == TAG_STATIC)
	{
		return;
	}

	if (pMemory->iMagic != ZONE_MAGIC)
	{
		Com_Error(ERR_FATAL, "Z_Size(): Not a valid zone header!");
	}

	Q_strncpyz(	pMemory->sOptionalLabel, psLabel, sizeof(pMemory->sOptionalLabel));
				pMemory->sOptionalLabel[ sizeof(pMemory->sOptionalLabel)-1 ] = '\0';
}
#endif



// Frees a block of memory...
//
void Z_Free(void *pvAddress)
{
	zoneHeader_t *pMemory = ((zoneHeader_t *)pvAddress) - 1;

	if (pMemory->eTag == TAG_STATIC)
	{
		return;
	}

	#ifdef DETAILED_ZONE_DEBUG_CODE
	//
	// check this error *before* barfing on bad magics...
	//
	int& iAllocCount = mapAllocatedZones[pMemory];
	if (iAllocCount <= 0)
	{			
		Com_Error(ERR_FATAL, "Z_Free(): Block already-freed, or not allocated through Z_Malloc!");
		return;
	}		   
	#endif

	if (pMemory->iMagic != ZONE_MAGIC)
	{
		Com_Error(ERR_FATAL, "Z_Free(): Corrupt zone header!");
		return;
	}
	if (ZoneTailFromHeader(pMemory)->iMagic != ZONE_MAGIC)
	{
		Com_Error(ERR_FATAL, "Z_Free(): Corrupt zone tail!");
		return;
	}

	Zone_FreeBlock(pMemory);
}


int Z_MemSize(memtag_t eTag)
{
	return TheZone.Stats.iSizesPerTag[eTag];
}

// Frees all blocks with the specified tag...
//
void Z_TagFree(memtag_t eTag)
{
//#ifdef _DEBUG
//	int iZoneBlocks = TheZone.Stats.iCount;
//#endif

	zoneHeader_t *pMemory = TheZone.Header.pNext;
	while (pMemory)
	{
		zoneHeader_t *pNext = pMemory->pNext;
		if ( (eTag == TAG_ALL) || (pMemory->eTag == eTag))
		{
			Zone_FreeBlock(pMemory);
		}
		pMemory = pNext;
	}

// these stupid pragmas don't work here???!?!?!
//
//#ifdef _DEBUG
//#pragma warning( disable : 4189)
//	int iBlocksFreed = iZoneBlocks - TheZone.Stats.iCount;
//#pragma warning( default : 4189)
//#endif
}


#ifdef DEBUG_ZONE_ALLOCS
void *_D_S_Malloc ( int iSize, const char *psFile, int iLine)
{
	return _D_Z_Malloc( iSize, TAG_SMALL, qfalse, psFile, iLine );
}
#else
void *S_Malloc( int iSize ) 
{
	return Z_Malloc( iSize, TAG_SMALL, qfalse);
}
#endif


#ifdef _DEBUG
static void Z_MemRecoverTest_f(void)
{
	// needs to be in _DEBUG only, not good for final game! 
	// fixme: findmeste: Remove this sometime
	//
	int iTotalMalloc = 0;
	while (1)
	{
		int iThisMalloc = 5* (1024 * 1024);
		Z_Malloc(iThisMalloc, TAG_SPECIAL_MEM_TEST, qfalse);	// and lose, just to consume memory
		iTotalMalloc += iThisMalloc;

		if (gbMemFreeupOccured)
			break;
	}

	Z_TagFree(TAG_SPECIAL_MEM_TEST);
}
#endif

// Gives a summary of the zone memory usage

static void Z_Stats_f(void)
{
	Com_Printf("\nThe zone is using %d bytes (%.2fMB) in %d memory blocks\n", 
								  TheZone.Stats.iCurrent, 
									        (float)TheZone.Stats.iCurrent / 1024.0f / 1024.0f, 
													  TheZone.Stats.iCount
				);

	Com_Printf("The zone peaked at %d bytes (%.2fMB)\n", 
									TheZone.Stats.iPeak, 
									         (float)TheZone.Stats.iPeak / 1024.0f / 1024.0f
				);
}

// Gives a detailed breakdown of the memory blocks in the zone
//
static void Z_Details_f(void)
{

	Com_Printf("---------------------------------------------------------------------------\n");
	Com_Printf("%20s %9s\n","Zone Tag","Bytes");
	Com_Printf("%20s %9s\n","--------","-----");
	for (int i=0; i<TAG_COUNT; i++)
	{
		int iThisCount = TheZone.Stats.iCountsPerTag[i];
		int iThisSize  = TheZone.Stats.iSizesPerTag	[i];

		if (iThisCount)
		{
			// can you believe that using %2.2f as a format specifier doesn't bloody work? 
			//	It ignores the left-hand specifier. Sigh, now I've got to do shit like this...
			//
			float	fSize		= (float)(iThisSize) / 1024.0f / 1024.0f;
			int		iSize		= fSize;
			int		iRemainder 	= 100.0f * (fSize - floor(fSize));
			Com_Printf("%20s %9d (%2d.%02dMB) in %6d blocks (%9d average)\n", 
					    psTagStrings[i], 
							  iThisSize, 
								iSize,iRemainder,
								           iThisCount, iThisSize / iThisCount
					   );
		}
	}
	Com_Printf("---------------------------------------------------------------------------\n");

	Z_Stats_f();
}

#ifdef DEBUG_ZONE_ALLOCS
#pragma warning (disable:4503)	// decorated name length xceeded, name was truncated
typedef map <sDebugString_t,int> LabelRefCount_t;	// yet another place where Gil's tring class works and MS's doesn't
typedef map <sDebugString_t,LabelRefCount_t>	TagBlockLabels_t;
										TagBlockLabels_t AllTagBlockLabels;
#pragma warning (disable:4503)	// decorated name length xceeded, name was truncated
static void Z_Snapshot_f(void)
{	
	AllTagBlockLabels.clear();

	zoneHeader_t *pMemory = TheZone.Header.pNext;
	while (pMemory)
	{
		AllTagBlockLabels[psTagStrings[pMemory->eTag]][pMemory->sOptionalLabel]++;
		pMemory = pMemory->pNext;		
	}

	giZoneSnaphotNum++;
	Com_Printf("Ok.    ( Current snapshot num is now %d )\n",giZoneSnaphotNum);
}

static void Z_TagDebug_f(void)
{
	TagBlockLabels_t AllTagBlockLabels_Local;
	qboolean bSnapShotTestActive = qfalse;

	memtag_t eTag = TAG_ALL;

	const char *psTAGName = Cmd_Argv(1);
	if (psTAGName[0])
	{
		// check optional arg...
		//
		if (!Q_stricmp(psTAGName,"#snap"))
		{
			bSnapShotTestActive = qtrue;

			AllTagBlockLabels_Local = AllTagBlockLabels;	// horrible great STL copy

			psTAGName = Cmd_Argv(2);			
		}

		if (psTAGName[0])
		{
			// skip over "tag_" if user supplied it...
			//
			if (!Q_stricmpn(psTAGName,"TAG_",4))
			{
				psTAGName += 4;
			}

			// see if the user specified a valid tag...
			//
			for (int i=0; i<TAG_COUNT; i++)
			{
				if (!Q_stricmp(psTAGName,psTagStrings[i]))
				{
					eTag = (memtag_t) i;
					break;
				}
			}
		}
	}
	else
	{
		Com_Printf("Usage: 'zone_tagdebug [#snap] <tag>', e.g. TAG_GHOUL2, TAG_ALL (careful!)\n");
		return;
	}

	Com_Printf("Dumping debug data for tag \"%s\"...%s\n\n",psTagStrings[eTag], bSnapShotTestActive?"( since snapshot only )":"");

	Com_Printf("%8s"," ");	// to compensate for code further down:   Com_Printf("(%5d) ",iBlocksListed);
	if (eTag == TAG_ALL)
	{
		Com_Printf("%20s ","Zone Tag");
	}
	Com_Printf("%9s\n","Bytes");
	Com_Printf("%8s"," ");
	if (eTag == TAG_ALL)
	{
		Com_Printf("%20s ","--------");
	}
	Com_Printf("%9s\n","-----");


	if (bSnapShotTestActive)
	{
		// dec ref counts in last snapshot for all current blocks (which will make new stuff go negative)
		//
		zoneHeader_t *pMemory = TheZone.Header.pNext;
		while (pMemory)
		{
			if (pMemory->eTag == eTag || eTag == TAG_ALL)
			{
				AllTagBlockLabels_Local[psTagStrings[pMemory->eTag]][pMemory->sOptionalLabel]--;
			}
			pMemory = pMemory->pNext;		
		}
	}

	// now dump them out...
	//
	int iBlocksListed = 0;
	int iTotalSize = 0;
	zoneHeader_t *pMemory = TheZone.Header.pNext;
	while (pMemory)
	{
		if (	(pMemory->eTag == eTag	|| eTag == TAG_ALL)
			&&  (!bSnapShotTestActive	|| (pMemory->iSnapshotNumber == giZoneSnaphotNum && AllTagBlockLabels_Local[psTagStrings[pMemory->eTag]][pMemory->sOptionalLabel] <0) )
			)
		{
			float	fSize		= (float)(pMemory->iSize) / 1024.0f / 1024.0f;
			int		iSize		= fSize;
			int		iRemainder 	= 100.0f * (fSize - floor(fSize));

			Com_Printf("(%5d) ",iBlocksListed);

			if (eTag == TAG_ALL)
			{
				Com_Printf("%20s",psTagStrings[pMemory->eTag]);
			}

			Com_Printf(" %9d (%2d.%02dMB) File: \"%s\", Line: %d\n",
						  pMemory->iSize,
 							  iSize,iRemainder,
												pMemory->sSrcFileBaseName,
															pMemory->iSrcFileLineNum
					   );
			if (pMemory->sOptionalLabel[0])
			{
				Com_Printf("( Label: \"%s\" )\n",pMemory->sOptionalLabel);
			}
			iBlocksListed++;
			iTotalSize += pMemory->iSize;
			
			if (bSnapShotTestActive)
			{
				// bump ref count so we only 1 warning per new string, not for every one sharing that label...
				//
				AllTagBlockLabels_Local[psTagStrings[pMemory->eTag]][pMemory->sOptionalLabel]++;
			}
		}
		pMemory = pMemory->pNext;		
	}

	Com_Printf("( %d blocks listed, %d bytes (%.2fMB) total )\n",iBlocksListed, iTotalSize, (float)iTotalSize / 1024.0f / 1024.0f);
}
#endif

// Shuts down the zone memory system and frees up all memory
void Com_ShutdownZoneMemory(void)
{
	Cmd_RemoveCommand("zone_stats");
	Cmd_RemoveCommand("zone_details");

#ifdef _DEBUG
	Cmd_RemoveCommand("zone_memrecovertest");
#endif

#ifdef DEBUG_ZONE_ALLOCS
	Cmd_RemoveCommand("zone_tagdebug");
	Cmd_RemoveCommand("zone_snapshot");
#endif

	if(TheZone.Stats.iCount)
	{
		//Com_Printf("Automatically freeing %d blocks making up %d bytes\n", TheZone.Stats.iCount, TheZone.Stats.iCurrent);
		Z_TagFree(TAG_ALL);

		assert(!TheZone.Stats.iCount);
		assert(!TheZone.Stats.iCurrent);
	}
}

// Initialises the zone memory system

void Com_InitZoneMemory( void ) 
{
	Com_Printf("Initialising zone memory .....\n");

	memset(&TheZone, 0, sizeof(TheZone)); 
	TheZone.Header.iMagic = ZONE_MAGIC;

	com_validateZone = Cvar_Get("com_validateZone", "0", 0);

	Cmd_AddCommand("zone_stats",	Z_Stats_f);
	Cmd_AddCommand("zone_details",	Z_Details_f);

#ifdef _DEBUG
	Cmd_AddCommand("zone_memrecovertest", Z_MemRecoverTest_f);
#endif


#ifdef DEBUG_ZONE_ALLOCS
	Cmd_AddCommand("zone_tagdebug",	Z_TagDebug_f);
	Cmd_AddCommand("zone_snapshot",	Z_Snapshot_f);
#endif
}




/*
========================
CopyString

 NOTE:	never write over the memory CopyString returns because
		memory from a memstatic_t might be returned
========================
*/
char *CopyString( const char *in ) {
	char	*out;

	if (!in[0]) {
		return ((char *)&gEmptyString) + sizeof(zoneHeader_t);
	}
	else if (!in[1]) {
		if (in[0] >= '0' && in[0] <= '9') {
			return ((char *)&gNumberString[in[0]-'0']) + sizeof(zoneHeader_t);
		}
	}

	out = (char *) S_Malloc (strlen(in)+1);
	strcpy (out, in);

	Z_Label(out,in);

	return out;
}


/*
=================
Com_Meminfo_f
=================
*/
void Com_Meminfo_f( void ) 
{
	Z_Details_f();
}

/*
===============
Com_TouchMemory

Touch all known used data to make sure it is paged in
===============
*/
void Com_TouchMemory( void ) {
	int		start, end;
	int		i, j;
	int		sum;	
	int		totalTouched;

	Z_Validate();

	start = Sys_Milliseconds();

	sum = 0;
	totalTouched=0;

	zoneHeader_t *pMemory = TheZone.Header.pNext;
	while (pMemory)
	{
		byte *pMem = (byte *) &pMemory[1];
		j = pMemory->iSize >> 2;
		for (i=0; i<j; i+=64){
			sum += ((int*)pMem)[i];
		}
		totalTouched+=pMemory->iSize;
		pMemory = pMemory->pNext;
	}

	end = Sys_Milliseconds();

	Com_Printf( "Com_TouchMemory: %i bytes, %i msec\n", totalTouched, end - start );
}



/*
=================
Com_InitHunkMemory
=================
*/
void Com_InitHunkMemory( void )
{
	Hunk_Clear();

	Cmd_AddCommand( "meminfo", Com_Meminfo_f );
}

// I'm leaving this in just in case we ever need to remember where's a good place to hook something like this in.
//
void Com_ShutdownHunkMemory(void)
{
}


/*
===================
Hunk_SetMark

The server calls this after the level and game VM have been loaded
===================
*/
void Hunk_SetMark( void ) 
{
}



/*
=================
Hunk_ClearToMark

The client calls this before starting a vid_restart or snd_restart
=================
*/
void Hunk_ClearToMark( void ) 
{
	Z_TagFree(TAG_HUNKALLOC);	
}



/*
=================
Hunk_Clear

The server calls this before shutting down or loading a new map
=================
*/
void Hunk_Clear( void ) 
{
	Z_TagFree(TAG_HUNKALLOC);

	extern void CIN_CloseAllVideos();
				CIN_CloseAllVideos();

	extern void R_ClearStuffToStopGhoul2CrashingThings(void);
				R_ClearStuffToStopGhoul2CrashingThings();
}






/*
===================================================================

EVENTS AND JOURNALING

In addition to these events, .cfg files are also copied to the
journaled file
===================================================================
*/

#define	MAX_PUSHED_EVENTS	64
int			com_pushedEventsHead, com_pushedEventsTail;
sysEvent_t	com_pushedEvents[MAX_PUSHED_EVENTS];

/*
=================
Com_InitJournaling
=================
*/
void Com_InitJournaling( void ) {
	Com_StartupVariable( "journal" );
	com_journal = Cvar_Get ("journal", "0", CVAR_INIT);
	if ( !com_journal->integer ) {
		return;
	}

	if ( com_journal->integer == 1 ) {
		Com_Printf( "Journaling events\n");
		com_journalFile = FS_FOpenFileWrite( "journal.dat" );
		com_journalDataFile = FS_FOpenFileWrite( "journaldata.dat" );
	} else if ( com_journal->integer == 2 ) {
		Com_Printf( "Replaying journaled events\n");
		FS_FOpenFileRead( "journal.dat", &com_journalFile, qtrue );
		FS_FOpenFileRead( "journaldata.dat", &com_journalDataFile, qtrue );
	}

   	if ( !com_journalFile || !com_journalDataFile ) {
		Cvar_Set( "com_journal", "0" );
		com_journalFile = 0;
		com_journalDataFile = 0;
		Com_Printf( "Couldn't open journal files\n" );
	}

}

/*
=================
Com_GetRealEvent
=================
*/
sysEvent_t	Com_GetRealEvent( void ) {
	int			r;
	sysEvent_t	ev;

	// either get an event from the system or the journal file
	if ( com_journal->integer == 2 ) {
		r = FS_Read( &ev, sizeof(ev),  com_journalFile );
		if ( r != sizeof(ev) ) {
			Com_Error( ERR_FATAL, "Error reading from journal file" );
		}
		if ( ev.evPtrLength ) {
			ev.evPtr = Z_Malloc( ev.evPtrLength, TAG_EVENT, qfalse);
			r = FS_Read( ev.evPtr, ev.evPtrLength, com_journalFile );
			if ( r != ev.evPtrLength ) {
				Com_Error( ERR_FATAL, "Error reading from journal file" );
			}
		}
	} else {
		ev = Sys_GetEvent();

		// write the journal value out if needed
		if ( com_journal->integer == 1 ) {
			r = FS_Write( &ev, sizeof(ev), com_journalFile );
			if ( r != sizeof(ev) ) {
				Com_Error( ERR_FATAL, "Error writing to journal file" );
			}
			if ( ev.evPtrLength ) {
				r = FS_Write( ev.evPtr, ev.evPtrLength, com_journalFile );
				if ( r != ev.evPtrLength ) {
					Com_Error( ERR_FATAL, "Error writing to journal file" );
				}
			}
		}
	}

	return ev;
}

/*
=================
Com_PushEvent
=================
*/
void Com_PushEvent( sysEvent_t *event ) {
	sysEvent_t		*ev;
	static			printedWarning;

	ev = &com_pushedEvents[ com_pushedEventsHead & (MAX_PUSHED_EVENTS-1) ];

	if ( com_pushedEventsHead - com_pushedEventsTail >= MAX_PUSHED_EVENTS ) {

		// don't print the warning constantly, or it can give time for more...
		if ( !printedWarning ) {
			printedWarning = qtrue;
			Com_Printf( "WARNING: Com_PushEvent overflow\n" );
		}

		if ( ev->evPtr ) {
			Z_Free( ev->evPtr );
		}
		com_pushedEventsTail++;
	} else {
		printedWarning = qfalse;
	}

	*ev = *event;
	com_pushedEventsHead++;
}

/*
=================
Com_GetEvent
=================
*/
sysEvent_t	Com_GetEvent( void ) {
	if ( com_pushedEventsHead > com_pushedEventsTail ) {
		com_pushedEventsTail++;
		return com_pushedEvents[ (com_pushedEventsTail-1) & (MAX_PUSHED_EVENTS-1) ];
	}
	return Com_GetRealEvent();
}

/*
=================
Com_RunAndTimeServerPacket
=================
*/
void Com_RunAndTimeServerPacket( netadr_t *evFrom, msg_t *buf ) {
	int		t1, t2, msec;

	t1 = 0;

	if ( com_speeds->integer ) {
		t1 = Sys_Milliseconds ();
	}

	SV_PacketEvent( *evFrom, buf );

	if ( com_speeds->integer ) {
		t2 = Sys_Milliseconds ();
		msec = t2 - t1;
		if ( com_speeds->integer == 3 ) {
			Com_Printf( "SV_PacketEvent time: %i\n", msec );
		}
	}
}

/*
=================
Com_EventLoop

Returns last event time
=================
*/
int Com_EventLoop( void ) {
	sysEvent_t	ev;
	netadr_t	evFrom;
	byte		bufData[MAX_MSGLEN];
	msg_t		buf;

	MSG_Init( &buf, bufData, sizeof( bufData ) );

	while ( 1 ) {
		ev = Com_GetEvent();

		// if no more events are available
		if ( ev.evType == SE_NONE ) {
			// manually send packet events for the loopback channel
			while ( NET_GetLoopPacket( NS_CLIENT, &evFrom, &buf ) ) {
				CL_PacketEvent( evFrom, &buf );
			}

			while ( NET_GetLoopPacket( NS_SERVER, &evFrom, &buf ) ) {
				// if the server just shut down, flush the events
				if ( com_sv_running->integer ) {
					Com_RunAndTimeServerPacket( &evFrom, &buf );
				}
			}

			return ev.evTime;
		}


		switch ( ev.evType ) {
		default:
			Com_Error( ERR_FATAL, "Com_EventLoop: bad event type %i", ev.evTime );
			break;
        case SE_NONE:
            break;
		case SE_KEY:
			CL_KeyEvent( ev.evValue, ev.evValue2, ev.evTime );
			break;
		case SE_CHAR:
			CL_CharEvent( ev.evValue );
			break;
		case SE_MOUSE:
			CL_MouseEvent( ev.evValue, ev.evValue2, ev.evTime );
			break;
		case SE_JOYSTICK_AXIS:
			CL_JoystickEvent( ev.evValue, ev.evValue2, ev.evTime );
			break;
		case SE_CONSOLE:
			Cbuf_AddText( (char *)ev.evPtr );
			Cbuf_AddText( "\n" );
			break;
		case SE_PACKET:
			evFrom = *(netadr_t *)ev.evPtr;
			buf.cursize = ev.evPtrLength - sizeof( evFrom );

			// we must copy the contents of the message out, because
			// the event buffers are only large enough to hold the
			// exact payload, but channel messages need to be large
			// enough to hold fragment reassembly
			if ( (unsigned)buf.cursize > buf.maxsize ) {
				Com_Printf("Com_EventLoop: oversize packet\n");
				continue;
			}
			memcpy( buf.data, (byte *)((netadr_t *)ev.evPtr + 1), buf.cursize );
			if ( com_sv_running->integer ) {
				Com_RunAndTimeServerPacket( &evFrom, &buf );
			} else {
				CL_PacketEvent( evFrom, &buf );
			}
			break;
		}

		// free any block data
		if ( ev.evPtr ) {
			Z_Free( ev.evPtr );
		}
	}
}

/*
================
Com_Milliseconds

Can be used for profiling, but will be journaled accurately
================
*/
int Com_Milliseconds (void) {
	sysEvent_t	ev;

	// get events and push them until we get a null event with the current time
	do {

		ev = Com_GetRealEvent();
		if ( ev.evType != SE_NONE ) {
			Com_PushEvent( &ev );
		}
	} while ( ev.evType != SE_NONE );
	
	return ev.evTime;
}

//============================================================================

/*
=============
Com_Error_f

Just throw a fatal error to
test error shutdown procedures
=============
*/
static void Com_Error_f (void) {
	if ( Cmd_Argc() > 1 ) {
		Com_Error( ERR_DROP, "Testing drop error" );
	} else {
		Com_Error( ERR_FATAL, "Testing fatal error" );
	}
}


/*
=============
Com_Freeze_f

Just freeze in place for a given number of seconds to test
error recovery
=============
*/
static void Com_Freeze_f (void) {
	float	s;
	int		start, now;

	if ( Cmd_Argc() != 2 ) {
		Com_Printf( "freeze <seconds>\n" );
		return;
	}
	s = atof( Cmd_Argv(1) );

	start = Com_Milliseconds();

	while ( 1 ) {
		now = Com_Milliseconds();
		if ( ( now - start ) * 0.001 > s ) {
			break;
		}
	}
}

/*
=================
Com_Crash_f

A way to force a bus error for development reasons
=================
*/
static void Com_Crash_f( void ) {
	* ( int * ) 0 = 0x12345678;
}

/*
=================
Com_Init
=================
*/
void Com_Init( char *commandLine ) {
	char	*s;

	Com_Printf( "%s %s %s\n", Q3_VERSION, CPUSTRING, __DATE__ );

	try {
		// prepare enough of the subsystems to handle
		// cvar and command buffer management
		Com_ParseCommandLine( commandLine );
		
		Swap_Init ();
		Cbuf_Init ();
		
		Com_InitZoneMemory();

		Cmd_Init ();
		Cvar_Init ();
		
		// get the commandline cvars set
		Com_StartupVariable( NULL );

		// done early so bind command exists
		CL_InitKeyCommands();
		
		FS_InitFilesystem ();	//uses z_malloc
		
		Com_InitJournaling();
		
		Cbuf_AddText ("exec default.cfg\n");

		// skip the jk2config.cfg if "safe" is on the command line
		if ( !Com_SafeMode() ) {
			Cbuf_AddText ("exec jk2config.cfg\n");
		}

		Cbuf_AddText ("exec autoexec.cfg\n");
		
		Cbuf_Execute ();
		
		// override anything from the config files with command line args
		Com_StartupVariable( NULL );
		
		// allocate the stack based hunk allocator
		Com_InitHunkMemory();

		// if any archived cvars are modified after this, we will trigger a writing
		// of the config file
		cvar_modifiedFlags &= ~CVAR_ARCHIVE;
		
		//
		// init commands and vars
		//
		Cmd_AddCommand ("quit", Com_Quit_f);
		Cmd_AddCommand ("writeconfig", Com_WriteConfig_f );
		
		com_maxfps = Cvar_Get ("com_maxfps", "85", CVAR_ARCHIVE);
		
		com_developer = Cvar_Get ("developer", "0", CVAR_TEMP );
		com_logfile = Cvar_Get ("logfile", "0", CVAR_TEMP );
		com_speedslog = Cvar_Get ("speedslog", "0", CVAR_TEMP );
		
		com_timescale = Cvar_Get ("timescale", "1", CVAR_CHEAT );
		com_fixedtime = Cvar_Get ("fixedtime", "0", CVAR_CHEAT);
		com_showtrace = Cvar_Get ("com_showtrace", "0", CVAR_CHEAT);
		com_viewlog = Cvar_Get( "viewlog", "0", CVAR_TEMP );
		com_speeds = Cvar_Get ("com_speeds", "0", 0);
		
		cl_paused	   = Cvar_Get ("cl_paused", "0", CVAR_ROM);
		sv_paused	   = Cvar_Get ("sv_paused", "0", CVAR_ROM);
		com_sv_running = Cvar_Get ("sv_running", "0", CVAR_ROM);
		com_cl_running = Cvar_Get ("cl_running", "0", CVAR_ROM);
		com_skippingcin = Cvar_Get ("skippingCinematic", "0", CVAR_ROM);
		com_buildScript = Cvar_Get( "com_buildScript", "0", 0 );
		
//		com_FirstTime = Cvar_Get( "com_FirstTime", "0", CVAR_ARCHIVE);
		if ( com_developer && com_developer->integer ) {
			Cmd_AddCommand ("error", Com_Error_f);
			Cmd_AddCommand ("crash", Com_Crash_f );
			Cmd_AddCommand ("freeze", Com_Freeze_f);
		}
		
		s = va("%s %s %s", Q3_VERSION, CPUSTRING, __DATE__ );
		com_version = Cvar_Get ("version", s, CVAR_ROM | CVAR_SERVERINFO );

		SP_Init();	// Initialize StripEd
	
		Sys_Init();	// this also detects CPU type, so I can now do this CPU check below...

		Netchan_Init( Com_Milliseconds() & 0xffff );	// pick a port value that should be nice and random
//	VM_Init();
		SV_Init();
		
		CL_Init();
		Sys_ShowConsole( com_viewlog->integer, qfalse );
		
		// set com_frameTime so that if a map is started on the
		// command line it will still be able to count on com_frameTime
		// being random enough for a serverid
		com_frameTime = Com_Milliseconds();

		// add + commands from command line
		if ( !Com_AddStartupCommands() ) {
#ifdef NDEBUG
			// if the user didn't give any commands, run default action
//			if ( !com_dedicated->integer ) 
			{
				Cbuf_AddText ("cinematic openinglogos\n");				
//				if( !com_introPlayed->integer ) {
//					Cvar_Set( com_introPlayed->name, "1" );
//					Cvar_Set( "nextmap", "cinematic intro" );
//				}
			}
#endif	
		}
		com_fullyInitialized = qtrue;
		Com_Printf ("--- Common Initialization Complete ---\n");

//HACKERY FOR THE DEUTSCH		
		if ( (Cvar_VariableIntegerValue("ui_iscensored") == 1) 	//if this was on before, set it again so it gets its flags
			|| sp_language->integer == SP_LANGUAGE_GERMAN )
		{
			Cvar_Get( "ui_iscensored",   "1", CVAR_ARCHIVE|CVAR_ROM|CVAR_INIT|CVAR_CHEAT|CVAR_NORESTART);
			Cvar_Set( "ui_iscensored",   "1");	//just in case it was archived
			Cvar_Get( "g_dismemberment", "0", CVAR_ARCHIVE|CVAR_ROM|CVAR_INIT|CVAR_CHEAT);
			Cvar_Set( "g_dismemberment", "0");	//just in case it was archived
		}
	}

	catch (const char* reason) {
		Sys_Error ("Error during initialization %s", reason);
	}

}

//==================================================================

void Com_WriteConfigToFile( const char *filename ) {
	fileHandle_t	f;

	f = FS_FOpenFileWrite( filename );
	if ( !f ) {
		Com_Printf ("Couldn't write %s.\n", filename );
		return;
	}

	FS_Printf (f, "// generated by Star Wars Jedi Outcast, do not modify\n");
	Key_WriteBindings (f);
	Cvar_WriteVariables (f);
	FS_FCloseFile( f );
}


/*
===============
Com_WriteConfiguration

Writes key bindings and archived cvars to config file if modified
===============
*/
void Com_WriteConfiguration( void ) {
	// if we are quiting without fully initializing, make sure
	// we don't write out anything
	if ( !com_fullyInitialized ) {
		return;
	}

	if ( !(cvar_modifiedFlags & CVAR_ARCHIVE ) ) {
		return;
	}
	cvar_modifiedFlags &= ~CVAR_ARCHIVE;

	Com_WriteConfigToFile( "jk2config.cfg" );
}


/*
===============
Com_WriteConfig_f

Write the config file to a specific name
===============
*/
void Com_WriteConfig_f( void ) {
	char	filename[MAX_QPATH];

	if ( Cmd_Argc() != 2 ) {
		Com_Printf( "Usage: writeconfig <filename>\n" );
		return;
	}

	Q_strncpyz( filename, Cmd_Argv(1), sizeof( filename ) );
	COM_DefaultExtension( filename, sizeof( filename ), ".cfg" );
	Com_Printf( "Writing %s.\n", filename );
	Com_WriteConfigToFile( filename );
}

/*
================
Com_ModifyMsec
================
*/


int Com_ModifyMsec( int msec, float &fraction ) 
{
	int		clampTime;

	fraction=0.0f;

	//
	// modify time for debugging values
	//
	if ( com_fixedtime->integer ) 
	{
		msec = com_fixedtime->integer;
	} 
	else if ( com_timescale->value ) 
	{
		fraction=(float)msec;
		fraction*=com_timescale->value;
		msec=(int)floor(fraction);
		fraction-=(float)msec;
	}
	
	// don't let it scale below 1 msec
	if ( msec < 1 ) 
	{
		msec = 1;
		fraction=0.0f;
	}

	if ( com_skippingcin->integer ) {
		// we're skipping ahead so let it go a bit faster
		clampTime = 500;
	} else {
		// for local single player gaming
		// we may want to clamp the time to prevent players from
		// flying off edges when something hitches.
		clampTime = 200;
	}

	if ( msec > clampTime ) {
		msec = clampTime;
		fraction=0.0f;
	}

	return msec;
}

/*
=================
Com_Frame
=================
*/
static vec3_t corg;
static vec3_t cangles;
static bool bComma;
void Com_SetOrgAngles(vec3_t org,vec3_t angles)
{
	VectorCopy(org,corg);
	VectorCopy(angles,cangles);
}

#pragma warning (disable: 4701)	//local may have been used without init (timing info vars)
void Com_Frame( void ) {
try 
{
	int		timeBeforeFirstEvents, timeBeforeServer, timeBeforeEvents, timeBeforeClient, timeAfter;
	int		msec, minMsec;
	static int	lastTime;
	char		msg[MAXPRINTMSG];

	// write config file if anything changed
	Com_WriteConfiguration(); 

	// if "viewlog" has been modified, show or hide the log console
	if ( com_viewlog->modified ) {
		Sys_ShowConsole( com_viewlog->integer, qfalse );
		com_viewlog->modified = qfalse;
	}

	//
	// main event loop
	//
	if ( com_speeds->integer ) {
		timeBeforeFirstEvents = Sys_Milliseconds ();
	}

	// we may want to spin here if things are going too fast
	if ( com_maxfps->integer > 0 ) {
		minMsec = 1000 / com_maxfps->integer;
	} else {
		minMsec = 1;
	}
	do {
		com_frameTime = Com_EventLoop();
		if ( lastTime > com_frameTime ) {
			lastTime = com_frameTime;		// possible on first frame
		}
		msec = com_frameTime - lastTime;
	} while ( msec < minMsec );
	Cbuf_Execute ();

	lastTime = com_frameTime;

	// mess with msec if needed
	com_frameMsec = msec;
	float fractionMsec=0.0f;
	msec = Com_ModifyMsec( msec, fractionMsec);
	
	//
	// server side
	//
	if ( com_speeds->integer ) {
		timeBeforeServer = Sys_Milliseconds ();
	}

	SV_Frame (msec, fractionMsec);


	//
	// client system
	//
//	if ( !com_dedicated->integer ) 
	{
		//
		// run event loop a second time to get server to client packets
		// without a frame of latency
		//
		if ( com_speeds->integer ) {
			timeBeforeEvents = Sys_Milliseconds ();
		}
		Com_EventLoop();
		Cbuf_Execute ();


		//
		// client side
		//
		if ( com_speeds->integer ) {
			timeBeforeClient = Sys_Milliseconds ();
		}

		CL_Frame (msec, fractionMsec);

		if ( com_speeds->integer ) {
			timeAfter = Sys_Milliseconds ();
		}
	}


	//
	// report timing information
	//
	if ( com_speeds->integer ) {
		int			all, sv, ev, cl;

		all = timeAfter - timeBeforeServer;
		sv = timeBeforeEvents - timeBeforeServer;
		ev = timeBeforeServer - timeBeforeFirstEvents + timeBeforeClient - timeBeforeEvents;
		cl = timeAfter - timeBeforeClient;
		sv -= time_game;
		cl -= time_frontend + time_backend;

		Com_Printf("fr:%i all:%3i sv:%3i ev:%3i cl:%3i gm:%3i tr:%3i pvs:%3i rf:%3i bk:%3i\n", 
					com_frameNumber, all, sv, ev, cl, time_game, timeInTrace, timeInPVSCheck, time_frontend, time_backend);

		// speedslog
		if ( com_speedslog && com_speedslog->integer )
		{
			if(!speedslog)
			{
				speedslog = FS_FOpenFileWrite("speeds.log");
				FS_Write("data={\n", strlen("data={\n"), speedslog);
				bComma=false;
				if ( com_speedslog->integer > 1 ) 
				{
					// force it to not buffer so we get valid
					// data even if we are crashing
					FS_ForceFlush(logfile);
				}
			}
			if (speedslog)
			{
				if(bComma)
				{
					FS_Write(",\n", strlen(",\n"), speedslog);
					bComma=false;
				}
				FS_Write("{", strlen("{"), speedslog);
				Com_sprintf(msg,sizeof(msg),
							"%8.4f,%8.4f,%8.4f,%8.4f,%8.4f,%8.4f,",corg[0],corg[1],corg[2],cangles[0],cangles[1],cangles[2]);
				FS_Write(msg, strlen(msg), speedslog);
				Com_sprintf(msg,sizeof(msg),
					"%i,%3i,%3i,%3i,%3i,%3i,%3i,%3i,%3i,%3i}", 
					com_frameNumber, all, sv, ev, cl, time_game, timeInTrace, timeInPVSCheck, time_frontend, time_backend);
				FS_Write(msg, strlen(msg), speedslog);
				bComma=true;
			}
		}

		timeInTrace = timeInPVSCheck = 0;
	}

	//
	// trace optimization tracking
	//
	if ( com_showtrace->integer ) {
		extern	int c_traces, c_brush_traces, c_patch_traces;
		extern	int	c_pointcontents;

		/*
		Com_Printf( "%4i non-sv_traces, %4i sv_traces, %4i ms, ave %4.2f ms\n", c_traces - numTraces, numTraces, timeInTrace, (float)timeInTrace/(float)numTraces );
		timeInTrace = numTraces = 0;
		c_traces = 0;
		*/
		
		Com_Printf ("%4i traces  (%ib %ip) %4i points\n", c_traces,
			c_brush_traces, c_patch_traces, c_pointcontents);
		c_traces = 0;
		c_brush_traces = 0;
		c_patch_traces = 0;
		c_pointcontents = 0;
	}

	com_frameNumber++;
}//try
	catch (const char* reason) {
		Com_Printf (reason);
		return;			// an ERR_DROP was thrown
	}
}
#pragma warning (default: 4701)	//local may have been used without init

/*
=================
Com_Shutdown
=================
*/
void Com_Shutdown (void) {
	if (logfile) {
		FS_FCloseFile (logfile);
		logfile = 0;
	}

	if (speedslog) {
		FS_Write("\n};", strlen("\n};"), speedslog);
		FS_FCloseFile (speedslog);
		speedslog = 0;
	}

	if (camerafile) {
		FS_FCloseFile (camerafile);
		camerafile = 0;
	}

	if ( com_journalFile ) {
		FS_FCloseFile( com_journalFile );
		com_journalFile = 0;
	}
	SP_Shutdown();//close the string packages
}

