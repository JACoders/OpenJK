#include "../client/client.h"
#include "mac_local.h"
#include <DriverServices.h>
#include <console.h>

/*

TODO:

about box
dir with no extension gives strange results
console input?
dedicated servers
icons
dynamic loading of server game
clipboard pasting

quit from menu

*/

int		sys_ticBase;
int		sys_msecBase;
int		sys_lastEventTic;

cvar_t	*sys_profile;
cvar_t	*sys_waitNextEvent;

void putenv( char *buffer ) {
	// the mac doesn't seem to have the concept of environment vars, so nop this
}

//===========================================================================

void	Sys_UnloadGame (void) {
}
void	*Sys_GetGameAPI (void *parms) {
    void *GetGameAPI (void *import);
    // we are hard-linked in, so no need to load anything
    return GetGameAPI (parms);
}

void	Sys_UnloadUI (void) {
}
void	*Sys_GetUIAPI (void) {
    void *GetUIAPI (void);
    // we are hard-linked in, so no need to load anything
    return GetUIAPI ();
}

void Sys_UnloadBotLib( void ) {
}

void *Sys_GetBotLibAPI (void *parms) {
	return NULL;
}

void *Sys_GetBotAIAPI (void *parms) {
	return NULL;
}

void dllEntry( int (*syscallptr)( int arg,... ) );
int vmMain( int command, ... );

void *Sys_LoadDll( const char *name, int (**entryPoint)(int, ...),
	int (*systemCalls)(int, ...) ) {
	
	dllEntry( systemCalls );
	
	*entryPoint = vmMain;
	
	return (void *)1;
}

void Sys_UnloadDll( void *dllHandle ) {
}

//===========================================================================

char *Sys_GetClipboardData( void ) {	// FIXME
    return NULL;
}

int Sys_GetProcessorId( void ) {
    return CPUID_GENERIC;
}

void Sys_Mkdir( const char *path ) {
	char	ospath[MAX_OSPATH];
	int		err;
	
	Com_sprintf( ospath, sizeof(ospath), "%s:", path );
	
	err = mkdir( ospath, 0777 );
}

char *Sys_Cwd( void ) {
	static char dir[MAX_OSPATH];
	int			l;
	
	getcwd( dir, sizeof( dir ) );
	dir[MAX_OSPATH-1] = 0;
	
	// strip off the last colon
	l = strlen( dir );
	if ( l > 0 ) {
		dir[ l - 1 ] = 0;
	} 
	return dir;
}

char *Sys_DefaultCDPath( void ) {
	return "";
}

char *Sys_DefaultBasePath( void ) {
	return Sys_Cwd();
}

/*
 =================================================================================

 FILE FINDING
 
 =================================================================================
*/

int PStringToCString( char *s ) {
	int		l;
	int		i;
	
	l = ((unsigned char *)s)[0];
	for ( i = 0 ; i < l ; i++ ) {
		s[i] = s[i+1];
	}
	s[l] = 0;
	return l;
}


int CStringToPString( char *s ) {
	int		l;
	int		i;
	
	l = strlen( s );
	for ( i = 0 ; i < l ; i++ ) {
		s[l-i] = s[l-i-1];
	}
	s[0] = l;
	return l;
}

#define	MAX_FOUND_FILES	0x1000

char **Sys_ListFiles( const char *directory, const char *extension, int *numfiles, qboolean wantsubs ) {
	int			nfiles;
	char		**listCopy;
	char		pdirectory[MAX_OSPATH];
	char		*list[MAX_FOUND_FILES];
	int			findhandle;
	int			directoryFlag;
	int			i;
	int			extensionLength;
	int			VRefNum;
	int			DrDirId;
	int			index;
	FSSpec		fsspec;
	
	// get the volume and directory numbers 
	// there has to be a better way than this...
	{
		CInfoPBRec	paramBlock;

		Q_strncpyz( pdirectory, directory, sizeof(pdirectory) );
		CStringToPString( pdirectory );
		FSMakeFSSpec( 0, 0, (unsigned char *)pdirectory, &fsspec );
		
		VRefNum = fsspec.vRefNum;

		memset( &paramBlock, 0, sizeof( paramBlock ) );
		paramBlock.hFileInfo.ioNamePtr = (unsigned char *)pdirectory;		
		PBGetCatInfoSync( &paramBlock );

		DrDirId = paramBlock.hFileInfo.ioDirID;
	}	

	if ( !extension) {
		extension = "";
	}
	extensionLength = strlen( extension );

	if ( wantsubs || (extension[0] == '/' && extension[1] == 0) ) {
		directoryFlag = 16;
	} else {
		directoryFlag = 0;
	}

	nfiles = 0;
	
	for ( index = 1 ; ; index++ ) {
		CInfoPBRec	paramBlock;
		char		fileName[MAX_OSPATH];
		int			length;
		OSErr		err;
		
		memset( &paramBlock, 0, sizeof( paramBlock ) );
		paramBlock.hFileInfo.ioNamePtr = (unsigned char *)fileName;
		paramBlock.hFileInfo.ioVRefNum = VRefNum;
		paramBlock.hFileInfo.ioFDirIndex = index;
		paramBlock.hFileInfo.ioDirID = DrDirId;
		
		err = PBGetCatInfoSync( &paramBlock );
		
		if ( err != noErr ) {
			break;
		}

		if ( directoryFlag ^ ( paramBlock.hFileInfo.ioFlAttrib & 16 ) ) {
			continue;
		}
		
		// convert filename to C string
		length = PStringToCString( fileName );

		// check the extension		
		if ( !directoryFlag ) {			
			if ( length < extensionLength ) {
				continue;
			}
			if ( Q_stricmp( fileName + length - extensionLength, extension ) ) {
				continue;
			} 
		}
		
		// add this file
		if ( nfiles == MAX_FOUND_FILES - 1 ) {
			break;
		}
		list[ nfiles ] = CopyString( fileName );
		nfiles++;		
	}	

	list[ nfiles ] = 0;


	// return a copy of the list
	*numfiles = nfiles;

	if ( !nfiles ) {
		return NULL;
	}

	listCopy = Z_Malloc( ( nfiles + 1 ) * sizeof( *listCopy ) );
	for ( i = 0 ; i < nfiles ; i++ ) {
		listCopy[i] = list[i];
	}
	listCopy[i] = NULL;

	return listCopy;
}

void	Sys_FreeFileList( char **list ) {
	int		i;

	if ( !list ) {
		return;
	}

	for ( i = 0 ; list[i] ; i++ ) {
		Z_Free( list[i] );
	}

	Z_Free( list );
}


//===================================================================


/*
================
Sys_Init

The cvar and file system has been setup, so configurations are loaded
================
*/
void Sys_Init(void) {
	Sys_InitNetworking();
	Sys_InitInput();	
}

/*
=================
Sys_Shutdown
=================
*/
void Sys_Shutdown( void ) {
	Sys_EndProfiling();
	Sys_ShutdownInput();	
	Sys_ShutdownNetworking();
}


/*
=================
Sys_BeginProfiling
=================
*/
static qboolean sys_profiling;
void Sys_BeginProfiling( void ) {
	if ( !sys_profile->integer ) {
		return;
	}
	ProfilerInit(collectDetailed, bestTimeBase, 16384, 64);
	sys_profiling = qtrue;
}

/*
=================
Sys_EndProfiling
=================
*/
void Sys_EndProfiling( void ) {
	unsigned char	pstring[1024];
	
	if ( !sys_profiling ) {
		return;
	}
	sys_profiling = qfalse;
	
	sprintf( (char *)pstring + 1, "%s:profile.txt", Cvar_VariableString( "fs_basepath" ) );
	pstring[0] = strlen( (char *)pstring + 1 );
	ProfilerDump( pstring );
	ProfilerTerm();
}

//================================================================================


/*
================
Sys_Milliseconds
================
*/
int Sys_Milliseconds (void) {
#if 0
	int		c;
	
	c = clock();		// FIXME, make more accurate

	return c*1000/60;
#else
	AbsoluteTime	t;
	Nanoseconds		nano;
	double			doub;

#define kTwoPower32 (4294967296.0)      /* 2^32 */
	
	t = UpTime();
	nano = AbsoluteToNanoseconds( t );
	doub = (((double) nano.hi) * kTwoPower32) + nano.lo;
	
	return doub * 0.000001;	
#endif
}

/*
================
Sys_Error
================
*/
void Sys_Error( const char *error, ... ) {
	va_list		argptr;
	char		string[1024];
	char		string2[1024];

	Sys_Shutdown();

	va_start (argptr,error);
	vsprintf (string2+1,error,argptr);
	va_end (argptr);
	string2[0] = strlen( string2 + 1 );
	
	strcpy( string+1, "Quake 3 Error:" );
	string[0] = strlen( string + 1 );
	
	// set the dialog box strings
	ParamText( (unsigned char *)string, (unsigned char *)string2, 
	(unsigned char *)string2, (unsigned char *)string2 );

	// run a dialog
	StopAlert( 128, NULL );
	
	exit(0);	
}


/*
================
Sys_Quit
================
*/
void Sys_Quit( void ) {
	Sys_Shutdown();
	exit (0);
}


//===================================================================

void Sys_BeginStreamedFile( fileHandle_t f, int readAhead ) {
}

void Sys_EndStreamedFile( fileHandle_t f ) {
}

int Sys_StreamedRead( void *buffer, int size, int count, fileHandle_t f ) {
   return FS_Read( buffer, size, count, f );
}

void Sys_StreamSeek( fileHandle_t f, int offset, int origin ) {
   FS_Seek( f, offset, origin );
}

//=================================================================================


/*
========================================================================

EVENT LOOP

========================================================================
*/

qboolean	Sys_GetPacket ( netadr_t *net_from, msg_t *net_message );

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

   ev = &eventQue[ eventHead & MASK_QUED_EVENTS ];
	if ( eventHead - eventTail >= MAX_QUED_EVENTS ) {
		Com_Printf("Sys_QueEvent: overflow\n");
		// we are discarding an event, but don't leak memory
		if ( ev->evPtr ) {
			free( ev->evPtr );
		}
		eventTail++;
	}
   eventHead++;

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
=================
Sys_PumpEvents
=================
*/
void Sys_PumpEvents( void ) {
	char		*s;
	msg_t		netmsg;
	netadr_t	adr;

   // pump the message loop
	Sys_SendKeyEvents();
   
   // check for console commands
   s = Sys_ConsoleInput();
   if ( s ) {
       char	*b;
       int		len;

       len = strlen( s ) + 1;
       b = malloc( len );
	   if ( !b ) {
		   Com_Error( ERR_FATAL, "malloc failed in Sys_PumpEvents" );
	   }
       strcpy( b, s );
       Sys_QueEvent( 0, SE_CONSOLE, 0, 0, len, b );
   }

   // check for other input devices
   Sys_Input();

   // check for network packets
   MSG_Init( &netmsg, sys_packetReceived, sizeof( sys_packetReceived ) );
   if ( Sys_GetPacket ( &adr, &netmsg ) ) {
       netadr_t		*buf;
       int				len;

       // copy out to a seperate buffer for qeueing
       len = sizeof( netadr_t ) + netmsg.cursize;
       buf = malloc( len );
	   if ( !buf ) {
		   Com_Error( ERR_FATAL, "malloc failed in Sys_PumpEvents" );
	   }
       *buf = adr;
       memcpy( buf+1, netmsg.data, netmsg.cursize );
       Sys_QueEvent( 0, SE_PACKET, 0, 0, len, buf );
   }
}

/*
================
Sys_GetEvent

================
*/
sysEvent_t Sys_GetEvent( void ) {
	sysEvent_t	ev;

	if ( eventHead == eventTail ) {
		Sys_PumpEvents();
	}
	// return if we have data
	if ( eventHead > eventTail ) {
	   eventTail++;
	   return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
	}

	// create an empty event to return
	memset( &ev, 0, sizeof( ev ) );
	ev.evTime = Sys_Milliseconds();

	// track the mac event "when" to milliseconds rate
	sys_ticBase = sys_lastEventTic;
	sys_msecBase = ev.evTime;
	
	return ev;
}


/*
=============
InitMacStuff
=============
*/
void InitMacStuff( void ) {
	Handle 		menuBar;
	char		dir[MAX_OSPATH];
	
	// init toolbox
	MaxApplZone();
	MoreMasters();
	
	InitGraf(&qd.thePort);
	InitFonts();
	FlushEvents(everyEvent, 0);
	SetEventMask( -1 );
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs(nil);
	InitCursor();

	// init menu
	menuBar = GetNewMBar(rMenuBar);
	if(!menuBar) {
		Com_Error( ERR_FATAL, "MenuBar not found.");
	}
	
	SetMenuBar(menuBar);
	DisposeHandle(menuBar);
	AppendResMenu(GetMenuHandle(mApple),'DRVR');
	DrawMenuBar();

	Sys_InitConsole();

	SetEventMask( -1 );
}

//==================================================================================

/*
=============
ReadCommandLineParms

Read startup options from a text file or dialog box
=============
*/
char *ReadCommandLineParms( void ) {
	FILE	*f;
	int		len;
	char	*buf;
	EventRecord	   event;
	
	// flush out all the events and see if shift is held down
	// to bring up the args window
	while ( WaitNextEvent(everyEvent, &event, 0, nil) ) {
	}
	if ( event.modifiers & 512 ) {
		static char	text[1024];
		int		argc;
		char	**argv;
		int		i;
		
		argc = ccommand( &argv );
		text[0] = 0;
		// concat all the args into a string
		// quote each arg seperately, because metrowerks does
		// its own quote combining from the dialog
		for ( i = 1 ; i < argc ; i++ ) {
			if ( argv[i][0] != '+' ) {
				Q_strcat( text, sizeof(text), "\"" );
			}
			Q_strcat( text, sizeof(text), argv[i] );
			if ( argv[i][0] != '+' ) {
				Q_strcat( text, sizeof(text), "\"" );
			}
			Q_strcat( text, sizeof(text), " " );
		}		
		return text;
	}
	
	// otherwise check for a parms file
	f = fopen( "MacQuake3Parms.txt", "r" );
	if ( !f ) {
		return "";
	}
	len = FS_filelength( f );
	buf = malloc( len + 1 );
	if ( !buf ) {
		exit( 1 );
	}
	buf[len] = 0;
	fread( buf, len, 1, f );
	fclose( f );

	return buf;
}

/*
=============
main
=============
*/
void main( void ) {
	char	*commandLine;
		
	InitMacStuff();
	
	commandLine = ReadCommandLineParms( );
	
    Com_Init ( commandLine );

	sys_profile = Cvar_Get( "sys_profile", "0", 0 );
	sys_profile->modified = qfalse;
	
	sys_waitNextEvent = Cvar_Get( "sys_waitNextEvent", "0", 0 );

    while( 1 ) {
        // run the frame
        Com_Frame();
        
        if ( sys_profile->modified ) {
        	sys_profile->modified = qfalse;
        	if ( sys_profile->integer ) {
        		Com_Printf( "Beginning profile.\n" );
				Sys_BeginProfiling() ;
			} else {
				Com_Printf( "Ending profile.\n" );
				Sys_EndProfiling();
        	}
        }
    }
}

