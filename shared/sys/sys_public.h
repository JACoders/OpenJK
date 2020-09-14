/*
===========================================================================
Copyright (C) 2005 - 2015, ioquake3 contributors
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#pragma once

#include "qcommon/q_shared.h"

#define MAXPRINTMSG 4096

typedef enum netadrtype_s
{
	NA_BAD = 0,					// an address lookup failed
	NA_BOT,
	NA_LOOPBACK,
	NA_BROADCAST,
	NA_IP
} netadrtype_t;

typedef struct netadr_s
{
	netadrtype_t	type;
	union {
		byte		ip[4];
		int32_t		ipi;
	};
	uint16_t	port;
} netadr_t;

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
  // bk001129 - make sure SE_NONE is zero
	SE_NONE = 0,	// evTime is still valid
	SE_KEY,		// evValue is a key code, evValue2 is the down flag
	SE_CHAR,	// evValue is an ascii char
	SE_MOUSE,	// evValue and evValue2 are reletive signed x / y moves
	SE_JOYSTICK_AXIS,	// evValue is an axis number and evValue2 is the current state (-127 to 127)
	SE_CONSOLE,	// evPtr is a char*
	SE_MAX
} sysEventType_t;

typedef struct sysEvent_s {
	int				evTime;
	sysEventType_t	evType;
	int				evValue, evValue2;
	int				evPtrLength;	// bytes of data pointed to by evPtr, for journaling
	void			*evPtr;			// this must be manually freed if not NULL
} sysEvent_t;

extern cvar_t *com_minimized;
extern cvar_t *com_unfocused;
extern cvar_t *com_maxfps;
extern cvar_t *com_maxfpsMinimized;
extern cvar_t *com_maxfpsUnfocused;

sysEvent_t	Sys_GetEvent( void );

void	Sys_Init (void);

// general development dll loading for virtual machine testing
typedef void *GetGameAPIProc( void  *);
typedef intptr_t QDECL VMMainProc( int, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t );
typedef intptr_t QDECL SystemCallProc( intptr_t, ... );
typedef void * QDECL GetModuleAPIProc( int, ... );

void	*Sys_LoadSPGameDll( const char *name, GetGameAPIProc **GetGameAPI );
void	* QDECL Sys_LoadDll(const char *name, qboolean useSystemLib);
void	* QDECL Sys_LoadLegacyGameDll( const char *name, VMMainProc **vmMain, SystemCallProc *systemcalls );
void	* QDECL Sys_LoadGameDll( const char *name, GetModuleAPIProc **moduleAPI );
void	Sys_UnloadDll( void *dllHandle );

char	*Sys_GetCurrentUser( void );

void	NORETURN QDECL Sys_Error( const char *error, ... );
void	NORETURN Sys_Quit (void);
char	*Sys_GetClipboardData( void );	// note that this isn't journaled...

void	Sys_Print( const char *msg );

// Sys_Milliseconds should only be used for profiling purposes,
// any game related timing information should come from event timestamps
int		Sys_Milliseconds (bool baseTime = false);
int		Sys_Milliseconds2(void);
void	Sys_Sleep( int msec );

extern "C" void	Sys_SnapVector( float *v );

bool Sys_RandomBytes( byte *string, int len );

void	Sys_SetErrorText( const char *text );

void	Sys_SendPacket( int length, const void *data, netadr_t to );

qboolean	Sys_StringToAdr( const char *s, netadr_t *a );
//Does NOT parse port numbers, only base addresses.

qboolean	Sys_IsLANAddress (netadr_t adr);
void		Sys_ShowIP(void);

qboolean	Sys_Mkdir( const char *path );
char	*Sys_Cwd( void );
void	Sys_SetDefaultInstallPath(const char *path);
char	*Sys_DefaultInstallPath(void);

#ifdef MACOS_X
char    *Sys_DefaultAppPath(void);
#endif

char	*Sys_DefaultHomePath(void);
const char *Sys_Dirname( char *path );
const char *Sys_Basename( char *path );

bool Sys_PathCmp( const char *path1, const char *path2 );

char **Sys_ListFiles( const char *directory, const char *extension, char *filter, int *numfiles, qboolean wantsubs );
void	Sys_FreeFileList( char **fileList );
//rwwRMG - changed to fileList to not conflict with list type

time_t Sys_FileTime( const char *path );

qboolean Sys_LowPhysicalMemory();

void Sys_SetProcessorAffinity( void );

typedef enum graphicsApi_e
{
	GRAPHICS_API_GENERIC,

	// Only OpenGL needs special treatment..
	GRAPHICS_API_OPENGL,
} graphicsApi_t;

// Graphics API
typedef struct window_s
{
	void *handle; // OS-dependent window handle
	graphicsApi_t api;
} window_t;

typedef enum glProfile_e
{
	GLPROFILE_COMPATIBILITY,
	GLPROFILE_CORE,
	GLPROFILE_ES,
} glProfile_t;

typedef enum glContextFlag_e
{
	GLCONTEXT_DEBUG = (1 << 1),
} glContextFlag_t;

typedef struct windowDesc_s
{
	graphicsApi_t api;

	// Only used if api == GRAPHICS_API_OPENGL
	struct gl_
	{
		int majorVersion;
		int minorVersion;
		glProfile_t profile;
		uint32_t contextFlags;
	} gl;
} windowDesc_t;

typedef struct glconfig_s glconfig_t;
window_t	WIN_Init( const windowDesc_t *desc, glconfig_t *glConfig );
void		WIN_Present( window_t *window );
void		WIN_SetGamma( glconfig_t *glConfig, byte red[256], byte green[256], byte blue[256] );
void		WIN_Shutdown( void );
void *		WIN_GL_GetProcAddress( const char *proc );
qboolean	WIN_GL_ExtensionSupported( const char *extension );

uint8_t ConvertUTF32ToExpectedCharset( uint32_t utf32 );
