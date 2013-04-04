// qcommon.h -- definitions common between client and server, but not game.or ref modules
#ifndef __QCOMMON_H__
#define __QCOMMON_H__

#include "stringed_ingame.h"
#include "../qcommon/cm_public.h"


// some zone mem debugging stuff
#ifndef FINAL_BUILD
	#ifdef _DEBUG
	//
	// both of these should be REM'd unless you specifically need them...
	//
	//#define DEBUG_ZONE_ALLOCS			// adds __FILE__ and __LINE__ info to zone blocks, to see who's leaking
	//#define DETAILED_ZONE_DEBUG_CODE	// this slows things down a LOT, and is only for tracking nasty double-freeing Z_Malloc bugs
	#endif
#endif


//============================================================================

//
// msg.c
//
typedef struct {
	qboolean	allowoverflow;	// if false, do a Com_Error
	qboolean	overflowed;		// set to true if the buffer size failed (with allowoverflow set)
	byte	*data;
	int		maxsize;
	int		cursize;
	int		readcount;
	int		bit;				// for bitwise reads and writes
} msg_t;

void MSG_Init (msg_t *buf, byte *data, int length);
void MSG_Clear (msg_t *buf);
void *MSG_GetSpace (msg_t *buf, int length);
void MSG_WriteData (msg_t *buf, const void *data, int length);


struct usercmd_s;
struct entityState_s;
struct playerState_s;

void MSG_WriteBits( msg_t *msg, int value, int bits );

void MSG_WriteByte (msg_t *sb, int c);
void MSG_WriteShort (msg_t *sb, int c);
void MSG_WriteLong (msg_t *sb, int c);
void MSG_WriteString (msg_t *sb, const char *s);

void	MSG_BeginReading (msg_t *sb);

int		MSG_ReadBits( msg_t *msg, int bits );

int		MSG_ReadByte (msg_t *sb);
int		MSG_ReadShort (msg_t *sb);
int		MSG_ReadLong (msg_t *sb);
char	*MSG_ReadString (msg_t *sb);
char	*MSG_ReadStringLine (msg_t *sb);
void	MSG_ReadData (msg_t *sb, void *buffer, int size);


void MSG_WriteDeltaUsercmd( msg_t *msg, struct usercmd_s *from, struct usercmd_s *to );
void MSG_ReadDeltaUsercmd( msg_t *msg, struct usercmd_s *from, struct usercmd_s *to );

void MSG_WriteDeltaEntity( msg_t *msg, struct entityState_s *from, struct entityState_s *to
						   , qboolean force );
void MSG_ReadDeltaEntity( msg_t *msg, entityState_t *from, entityState_t *to, 
						 int number );
void MSG_ReadEntity( msg_t *msg, entityState_t *to);
void MSG_WriteEntity( msg_t *msg, entityState_t *to, int removeNum);

void MSG_WriteDeltaPlayerstate( msg_t *msg, struct playerState_s *from, struct playerState_s *to );
void MSG_ReadDeltaPlayerstate( msg_t *msg, struct playerState_s *from, struct playerState_s *to );


//============================================================================

#ifdef _M_IX86
//
// optimised stuff for Intel, since most of our data is in that format anyway...
//
extern	short	BigShort (short l);
extern	int		BigLong (int l);
extern	float	BigFloat (float l);
#define LittleShort(l) l
#define LittleLong(l) l
#define LittleFloat(l) l
//
#else
//
// standard smart-swap code...
//
extern	short	BigShort (short l);
extern	short	LittleShort (short l);
extern	int		BigLong (int l);
extern	int		LittleLong (int l);
extern	float	BigFloat (float l);
extern	float	LittleFloat (float l);
//
#endif


/*
==============================================================

NET

==============================================================
*/

#ifdef _XBOX
#define PACKET_BACKUP	2
#else
#define	PACKET_BACKUP	16	// number of old messages that must be kept on client and
#endif						// server for delta comrpession and ping estimation
#define	PACKET_MASK		(PACKET_BACKUP-1)

#define	MAX_PACKET_USERCMDS		32		// max number of usercmd_t in a packet

#define	PORT_ANY			-1

#define	MAX_RELIABLE_COMMANDS	64			// max string commands buffered for restransmit

typedef enum {
	NA_BAD,					// an address lookup failed
	NA_LOOPBACK,
} netadrtype_t;

typedef enum {
	NS_CLIENT,
	NS_SERVER
} netsrc_t;

typedef struct {
	netadrtype_t	type;

	unsigned short	port;
} netadr_t;

void		NET_SendPacket (netsrc_t sock, int length, const void *data, netadr_t to);
void		NET_OutOfBandPrint( netsrc_t net_socket, netadr_t adr, const char *format, ...);

qboolean	NET_CompareAdr (netadr_t a, netadr_t b);
qboolean	NET_CompareBaseAdr (netadr_t a, netadr_t b);
qboolean	NET_IsLocalAddress (netadr_t adr);
qboolean	NET_IsLANAddress (netadr_t adr);
const char	*NET_AdrToString (netadr_t a);
qboolean	NET_StringToAdr ( const char *s, netadr_t *a);
qboolean	NET_GetLoopPacket (netsrc_t sock, netadr_t *net_from, msg_t *net_message);


#define	MAX_MSGLEN				(1*17408)		// max length of a message, which may
//#define	MAX_MSGLEN				(3*16384)		// max length of a message, which may
											// be fragmented into multiple packets


/*
Netchan handles packet fragmentation and out of order / duplicate suppression
*/

typedef struct {
	netsrc_t	sock;

	int			dropped;			// between last packet and previous

	netadr_t	remoteAddress;
	int			qport;				// qport value to write when transmitting

	// sequencing variables
	int			incomingSequence;
	int			incomingAcknowledged;

	int			outgoingSequence;

	// incoming fragment assembly buffer
	int			fragmentSequence;
	int			fragmentLength;	
	byte		fragmentBuffer[MAX_MSGLEN];
} netchan_t;

void Netchan_Init( int qport );
void Netchan_Setup( netsrc_t sock, netchan_t *chan, netadr_t adr, int qport );

void Netchan_Transmit( netchan_t *chan, int length, const byte *data );
qboolean Netchan_Process( netchan_t *chan, msg_t *msg );


/*
==============================================================

PROTOCOL

==============================================================
*/

#define	PROTOCOL_VERSION	40

#define	PORT_SERVER			27960

// the svc_strings[] array in cl_parse.c should mirror this
//
// server to client
//
enum svc_ops_e {
	svc_bad,
	svc_nop,
	svc_gamestate,
	svc_configstring,			// [short] [string] only in gamestate messages
	svc_baseline,				// only in gamestate messages
	svc_serverCommand,			// [string] to be executed by client game module
	svc_download,				// [short] size [size bytes]
	svc_snapshot
};


//
// client to server
//
enum clc_ops_e {
	clc_bad,
	clc_nop, 		
	clc_move,				// [[usercmd_t]
	clc_clientCommand		// [string] message
};


/*
==============================================================

CMD

Command text buffering and command execution

==============================================================
*/

/*

Any number of commands can be added in a frame, from several different sources.
Most commands come from either keybindings or console line input, but entire text
files can be execed.

*/

void Cbuf_Init (void);
// allocates an initial text buffer that will grow as needed

void Cbuf_AddText( const char *text );
// Adds command text at the end of the buffer, does NOT add a final \n

void Cbuf_ExecuteText( int exec_when, const char *text );
// this can be used in place of either Cbuf_AddText or Cbuf_InsertText

void Cbuf_Execute (void);
// Pulls off \n terminated lines of text from the command buffer and sends
// them through Cmd_ExecuteString.  Stops when the buffer is empty.
// Normally called once per frame, but may be explicitly invoked.
// Do not call inside a command function, or current args will be destroyed.

//===========================================================================

/*

Command execution takes a null terminated string, breaks it into tokens,
then searches for a command or variable that matches the first token.

*/

typedef void (*xcommand_t) (void);

void	Cmd_Init (void);

void	Cmd_AddCommand( const char *cmd_name, xcommand_t function );
// called by the init functions of other parts of the program to
// register commands and functions to call for them.
// The cmd_name is referenced later, so it should not be in temp memory
// if function is NULL, the command will be forwarded to the server
// as a clc_clientCommand instead of executed locally

void	Cmd_RemoveCommand( const char *cmd_name );

char 	*Cmd_CompleteCommand( const char *partial );
// attempts to match a partial command for automatic command line completion
// returns NULL if nothing fits

int		Cmd_Argc (void);
char	*Cmd_Argv (int arg);
void	Cmd_ArgvBuffer( int arg, char *buffer, int bufferLength );
char	*Cmd_Args (void);
void	Cmd_ArgsBuffer( char *buffer, int bufferLength );
// The functions that execute commands get their parameters with these
// functions. Cmd_Argv () will return an empty string, not a NULL
// if arg > argc, so string operations are allways safe.

void	Cmd_TokenizeString( const char *text );
// Takes a null terminated string.  Does not need to be /n terminated.
// breaks the string up into arg tokens.

void	Cmd_ExecuteString( const char *text );
// Parses a single line of text into arguments and tries to execute it
// as if it was typed at the console


/*
==============================================================

CVAR

==============================================================
*/

/*

cvar_t variables are used to hold scalar or string variables that can be changed
or displayed at the console or prog code as well as accessed directly
in C code.

The user can access cvars from the console in three ways:
r_draworder			prints the current value
r_draworder 0		sets the current value to 0
set r_draworder 0	as above, but creates the cvar if not present

Cvars are restricted from having the same names as commands to keep this
interface from being ambiguous.

The are also occasionally used to communicated information between different
modules of the program.

*/

cvar_t *Cvar_Get( const char *var_name, const char *value, int flags );
// creates the variable if it doesn't exist, or returns the existing one
// if it exists, the value will not be changed, but flags will be ORed in
// that allows variables to be unarchived without needing bitflags
// if value is "", the value will not override a previously set value.

void	Cvar_Register( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags );
// basically a slightly modified Cvar_Get for the interpreted modules

void	Cvar_Update( vmCvar_t *vmCvar );
// updates an interpreted modules' version of a cvar

void 	Cvar_Set( const char *var_name, const char *value );
// will create the variable with no flags if it doesn't exist

void	Cvar_SetValue( const char *var_name, float value );
// expands value to a string and calls Cvar_Set

float	Cvar_VariableValue( const char *var_name );
int		Cvar_VariableIntegerValue( const char *var_name );
// returns 0 if not defined or non numeric

char	*Cvar_VariableString( const char *var_name );
void	Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );
// returns an empty string if not defined

char 	*Cvar_CompleteVariable( const char *partial );
// attempts to match a partial variable name for command line completion
// returns NULL if nothing fits

void 	Cvar_Reset( const char *var_name );

void	Cvar_SetCheatState( void );
// reset all testing vars to a safe value

qboolean Cvar_Command( void );
// called by Cmd_ExecuteString when Cmd_Argv(0) doesn't match a known
// command.  Returns true if the command was a variable reference that
// was handled. (print or change)

void 	Cvar_WriteVariables( fileHandle_t f );
// writes lines containing "set variable value" for all variables
// with the archive flag set to true.

void	Cvar_Init( void );

char	*Cvar_InfoString( int bit );
// returns an info string containing all the cvars that have the given bit set
// in their flags ( CVAR_USERINFO, CVAR_SERVERINFO, CVAR_SYSTEMINFO, etc )
void	Cvar_InfoStringBuffer( int bit, char *buff, int buffsize );

void	Cvar_Restart_f( void );

extern	int			cvar_modifiedFlags;
// whenever a cvar is modifed, its flags will be OR'd into this, so
// a single check can determine if any CVAR_USERINFO, CVAR_SERVERINFO,
// etc, variables have been modified since the last check.  The bit
// can then be cleared to allow another change detection.

/*
==============================================================

FILESYSTEM

No stdio calls should be used by any part of the game, because
we need to deal with all sorts of directory and seperator char
issues.
==============================================================
*/

qboolean FS_Initialized();

void	FS_InitFilesystem (void);

char	**FS_ListFiles( const char *directory, const char *extension, int *numfiles );
// directory should not have either a leading or trailing /
// if extension is "/", only subdirectories will be returned
// the returned files will not include any directories or /

void	FS_FreeFileList( char **filelist );

int	FS_GetFileList(  const char *path, const char *extension, char *listbuf, int bufsize );

fileHandle_t FS_FOpenFileWrite( const char *qpath );
// will properly create any needed paths and deal with seperater character issues

fileHandle_t FS_FOpenFileAppend( const char *filename );	// this was present already, but no public proto

qboolean FS_GetExtendedInfo_FOpenFileRead(const char *filename, char **ppsFilename, int *piOffset);
//return value is success of opening file, then ppsFilename and piOffset are valid

int		FS_FOpenFileRead( const char *qpath, fileHandle_t *file, qboolean uniqueFILE );
// if uniqueFILE is true, then a new FILE will be fopened even if the file
// is found in an already open pak file.  If uniqueFILE is false, you must call
// FS_FCloseFile instead of fclose, otherwise the pak FILE would be improperly closed
// It is generally safe to always set uniqueFILE to true, because the majority of
// file IO goes through FS_ReadFile, which Does The Right Thing already.

int	FS_FileIsInPAK(const char *filename );
// returns 1 if a file is in the PAK file, otherwise -1

int	FS_Write( const void *buffer, int len, fileHandle_t f );

int	FS_Read( void *buffer, int len, fileHandle_t f );
// properly handles partial reads and reads from other dlls

void	FS_FCloseFile( fileHandle_t f );
// note: you can't just fclose from another DLL, due to MS libc issues

int		FS_ReadFile( const char *qpath, void **buffer );
// returns the length of the file
// a null buffer will just return the file length without loading
// as a quick check for existance. -1 length == not present
// A 0 byte will always be appended at the end, so string ops are safe.
// the buffer should be considered read-only, because it may be cached
// for other uses.

void	FS_ForceFlush( fileHandle_t f );
// forces flush on files we're writing to.

void	FS_FreeFile( void *buffer );
// frees the memory returned by FS_ReadFile

void	FS_WriteFile( const char *qpath, const void *buffer, int size );
// writes a complete file, creating any subdirectories needed

int		FS_filelength( fileHandle_t f );
// doesn't work for files that are opened from a pack file

int		FS_FTell( fileHandle_t f );
// where are we?

void	FS_Flush( fileHandle_t f );

void 	QDECL FS_Printf( fileHandle_t f, const char *fmt, ... );
// like fprintf

int		FS_FOpenFileByMode( const char *qpath, fileHandle_t *f, fsMode_t mode );
// opens a file for reading, writing, or appending depending on the value of mode

int		FS_Seek( fileHandle_t f, long offset, int origin );
// seek on a file (doesn't work for zip files!!!!!!!!)


// These 2 are generally only used by the save games, filenames are local (eg "saves/blah.sav")
//
void		FS_DeleteUserGenFile( const char *filename );
qboolean	FS_MoveUserGenFile  ( const char *filename_src, const char *filename_dst );

/*
==============================================================

MISC

==============================================================
*/

//==========================================================
//
// NOTE NOTE NOTE!!!!!!!!!!!!!
//
// Any CPUID_XXXX defined as higher than CPUID_INTEL_MMX *must* have MMX support (eg like CPUID_AMD_3DNOW (0x30) has),
//	this allows convenient MMX capability checking. If you for some reason want to support some new processor that does
//	*NOT* have MMX (yeah, right), then define it as a lower number. -slc
//
// ( These values are returned by Sys_GetProcessorId )
//
#define CPUID_GENERIC			0			// any unrecognized processor

#define CPUID_AXP				0x10

#define CPUID_INTEL_UNSUPPORTED	0x20			// Intel 386/486
#define CPUID_INTEL_PENTIUM		0x21			// Intel Pentium or PPro
#define CPUID_INTEL_MMX			0x22			// Intel Pentium/MMX or P2/MMX
#define CPUID_INTEL_KATMAI		0x23			// Intel Katmai
#define CPUID_INTEL_WILLIAMETTE	0x24			// Intel Williamette

#define CPUID_AMD_3DNOW			0x30			// AMD K6 3DNOW!
//
//==========================================================

#define RoundUp(N, M) ((N) + ((unsigned int)(M)) - (((unsigned int)(N)) % ((unsigned int)(M))))
#define RoundDown(N, M) ((N) - (((unsigned int)(N)) % ((unsigned int)(M))))

char		*CopyString( const char *in );
void		Info_Print( const char *s );

void		Com_BeginRedirect (char *buffer, int buffersize, void (*flush)(char *));
void		Com_EndRedirect( void );
void 		QDECL Com_Printf( const char *fmt, ... );
void 		QDECL Com_DPrintf( const char *fmt, ... );
void 		QDECL Com_Error( int code, const char *fmt, ... );
void 		Com_Quit_f( void );
int			Com_EventLoop( void );
int			Com_Milliseconds( void );	// will be journaled properly
unsigned	Com_BlockChecksum( const void *buffer, int length );
int			Com_Filter(char *filter, char *name, int casesensitive);

void		Com_StartupVariable( const char *match );
// checks for and removes command line "+set var arg" constructs
// if match is NULL, all set commands will be executed, otherwise
// only a set with the exact name.  Only used during startup.


extern	cvar_t	*com_developer;
extern	cvar_t	*com_speeds;
extern	cvar_t	*com_timescale;
extern	cvar_t	*com_sv_running;
extern	cvar_t	*com_cl_running;
extern	cvar_t	*com_viewlog;			// 0 = hidden, 1 = visible, 2 = minimized
extern	cvar_t	*com_version;

// both client and server must agree to pause
extern	cvar_t	*cl_paused;
extern	cvar_t	*sv_paused;

// com_speeds times
extern	int		time_game;
extern	int		time_frontend;
extern	int		time_backend;		// renderer backend time

extern	int		timeInTrace;
extern	int		timeInPVSCheck;
extern	int		numTraces;

extern	int		com_frameTime;
extern	int		com_frameMsec;

extern	qboolean	com_errorEntered;


#ifndef _XBOX
extern	fileHandle_t	com_journalFile;
extern	fileHandle_t	com_journalDataFile;
#endif

/*

--- low memory ----
server vm
server clipmap
---mark---
renderer initialization (shaders, etc)
UI vm
cgame vm
renderer map
renderer models

---free---

temp file loading
--- high memory ---

*/
int  Z_Validate( void );			// also used to insure all of these are paged in
int   Z_MemSize	( memtag_t eTag );
void  Z_TagFree	( memtag_t eTag );
int   Z_Free	( void *ptr );	//returns bytes freed
int	  Z_Size	( void *pvAddress);
void  Z_MorphMallocTag( void *pvAddress, memtag_t eDesiredTag );
qboolean Z_IsFromZone(void *pvAddress, memtag_t eTag);	//returns size if true

#ifdef DEBUG_ZONE_ALLOCS

	void *_D_Z_Malloc ( int iSize, memtag_t eTag, qboolean bZeroit, const char *psFile, int iLine );
	void *_D_S_Malloc ( int iSize, const char *psFile, int iLine );	
	void  _D_Z_Label  ( const void *pvAddress, const char *pslabel );

	#define Z_Malloc(_iSize, _eTag, _bZeroit)	_D_Z_Malloc (_iSize, _eTag, _bZeroit, __FILE__, __LINE__)
	#define S_Malloc(_iSize)					_D_S_Malloc	(_iSize, __FILE__, __LINE__)	// NOT 0 filled memory only for small allocations	
	
	#define Z_Label(_ptr, _label)				_D_Z_Label	(_ptr, _label)

#else

	void *Z_Malloc  ( int iSize, memtag_t eTag, qboolean bZeroit, int iAlign = 4);	// return memory NOT zero-filled by default
	void *S_Malloc	( int iSize );									// NOT 0 filled memory only for small allocations

	#define Z_Label(_ptr, _label)	/* */

#endif


void Hunk_Clear( void );
void Hunk_ClearToMark( void );
void Hunk_SetMark( void );
// note the opposite default for 'bZeroIt' in Hunk_Alloc to Z_Malloc, since Hunk_Alloc always used to memset(0)...
//
inline void *Hunk_Alloc( int size, qboolean bZeroIt = qtrue) 
{
	return Z_Malloc(size, TAG_HUNKALLOC, bZeroIt);
}


void Com_TouchMemory( void );

// commandLine should not include the executable name (argv[0])
void Com_SetOrgAngles(vec3_t org,vec3_t angles);
void Com_Init( char *commandLine );
void Com_Frame( void );
void Com_Shutdown( void );
void Com_ShutdownZoneMemory(void);
void Com_ShutdownHunkMemory(void);

bool Com_ParseTextFile(const char *file, class CGenericParser2 &parser, bool cleanFirst = true);
CGenericParser2 *Com_ParseTextFile(const char *file, bool cleanFirst, bool writeable);
void Com_ParseTextFileDestroy(class CGenericParser2 &parser);

/*
==============================================================

CLIENT / SERVER SYSTEMS

==============================================================
*/

//
// client interface
//
void CL_InitKeyCommands( void );
// the keyboard binding interface must be setup before execing
// config files, but the rest of client startup will happen later

void CL_Init( void );
void CL_Disconnect( void );
void CL_Shutdown( void );
void CL_Frame( int msec,float fractionMsec );
qboolean CL_GameCommand( void );
void CL_KeyEvent (int key, qboolean down, unsigned time);

void CL_CharEvent( int key );
// char events are for field typing, not game control

void CL_MouseEvent( int dx, int dy, int time );

void CL_JoystickEvent( int axis, int value, int time );

void CL_PacketEvent( netadr_t from, msg_t *msg );

void CL_ConsolePrint( char *text );

void CL_MapLoading( void );
// do a screen update before starting to load a map
// when the server is going to load a new map, the entire hunk
// will be cleared, so the client must shutdown cgame, ui, and
// the renderer

void	CL_ForwardCommandToServer( void );
// adds the current command line as a clc_clientCommand to the client message.
// things like godmode, noclip, etc, are commands directed to the server,
// so when they are typed in at the console, they will need to be forwarded.

void CL_FlushMemory( void );
// dump all memory on an error

void CL_StartHunkUsers( void );

void Key_WriteBindings( fileHandle_t f );
// for writing the config files

void S_ClearSoundBuffer( void );
// call before filesystem access

void SCR_DebugGraph (float value, int color);	// FIXME: move logging to common?


//
// server interface
//
void SV_Init( void );
void SV_Shutdown( char *finalmsg );
void SV_Frame( int msec,float fractionMsec);
void SV_PacketEvent( netadr_t from, msg_t *msg );
qboolean SV_GameCommand( void );


//
// UI interface
//
qboolean UI_GameCommand( void );


/*
==============================================================

NON-PORTABLE SYSTEM SERVICES

==============================================================
*/

typedef enum {
	AXIS_SIDE,
	AXIS_FORWARD,
	AXIS_UP,
	AXIS_ROLL,
	AXIS_YAW,
	AXIS_PITCH,
	MAX_JOYSTICK_AXIS
} joystickAxis_t;

typedef enum {
	SE_NONE,	// evTime is still valid
	SE_KEY,		// evValue is a key code, evValue2 is the down flag
	SE_CHAR,	// evValue is an ascii char
	SE_MOUSE,	// evValue and evValue2 are reletive signed x / y moves
	SE_JOYSTICK_AXIS,	// evValue is an axis number and evValue2 is the current state (-127 to 127)
	SE_CONSOLE,	// evPtr is a char*
	SE_PACKET	// evPtr is a netadr_t followed by data bytes to evPtrLength
} sysEventType_t;

typedef struct {
	int				evTime;
	sysEventType_t	evType;
	int				evValue, evValue2;
	int				evPtrLength;	// bytes of data pointed to by evPtr, for journaling
	void			*evPtr;			// this must be manually freed if not NULL
} sysEvent_t;

sysEvent_t	Sys_GetEvent( void );

void	Sys_Init (void);

char	*Sys_GetCurrentUser( void );

void	QDECL Sys_Error( const char *error, ...);
void	Sys_Quit (void);
char	*Sys_GetClipboardData( void );	// note that this isn't journaled...

void	Sys_Print( const char *msg );
#ifdef _XBOX
void	Sys_Log( const char *file, const char *msg );
void	Sys_Log( const char *file, const void *buffer, int size, bool flush );
#endif

// Sys_Milliseconds should only be used for profiling purposes,
// any game related timing information should come from event timestamps
int		Sys_Milliseconds (void);


// the system console is shown when a dedicated server is running
void	Sys_DisplaySystemConsole( qboolean show );

int		Sys_GetProcessorId( void );

void	Sys_BeginStreamedFile( fileHandle_t f, int readahead );
void	Sys_EndStreamedFile( fileHandle_t f );
int		Sys_StreamedRead( void *buffer, int size, int count, fileHandle_t f );
void	Sys_StreamSeek( fileHandle_t f, int offset, int origin );

void	Sys_ShowConsole( int level, qboolean quitOnClose );
void	Sys_SetErrorText( const char *text );

qboolean	Sys_CheckCD( void );

void	Sys_Mkdir( const char *path );
char	*Sys_Cwd( void );
char	*Sys_DefaultCDPath(void);
char	*Sys_DefaultBasePath(void);

char **Sys_ListFiles( const char *directory, const char *extension, int *numfiles, qboolean wantsubs );
void	Sys_FreeFileList( char **filelist );

void	Sys_BeginProfiling( void );
void	Sys_EndProfiling( void );

qboolean Sys_LowPhysicalMemory();
qboolean Sys_FileOutOfDate( LPCSTR psFinalFileName /* dest */, LPCSTR psDataFileName /* src */ );
qboolean Sys_CopyFile(LPCSTR lpExistingFileName, LPCSTR lpNewFileName, qboolean bOverwrite);


//byte*	SCR_GetScreenshot(qboolean *qValid);
//void	SCR_SetScreenshot(const byte *pbData, int w, int h);
//byte*	SCR_TempRawImage_ReadFromFile(const char *psLocalFilename, int *piWidth, int *piHeight, byte *pbReSampleBuffer, qboolean qbVertFlip);
//void	SCR_TempRawImage_CleanUp();

inline int Round(float value)
{
	return((int)floorf(value + 0.5f));
}


#ifdef _XBOX
//////////////////////////////
//
// Map Lump Loader
//
struct Lump
{
	void* data;
	int len;
	
	Lump() : data(NULL), len(0) {}
	~Lump() { clear(); }

	void load(const char* map, const char* lump)
	{
		clear();

		char path[MAX_QPATH];
		Com_sprintf(path, MAX_QPATH, "%s/%s.mle", map, lump);

		len = FS_ReadFile(path, &data);
		if (len < 0) len = 0;
	}

	void clear(void)
	{
		if (data)
		{
			FS_FreeFile(data);
			data = NULL;
		}
	}
};
#endif _XBOX

#endif //__QCOMMON_H__