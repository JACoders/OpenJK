/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
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

#ifndef __Q_SHARED_H
#define __Q_SHARED_H

// q_shared.h -- included first by ALL program modules.
// A user mod should never modify this file

#include "qcommon/q_math.h"
#include "qcommon/q_color.h"
#include "qcommon/q_string.h"

#ifdef _MSC_VER

#pragma warning(disable : 4018)     // signed/unsigned mismatch
//#pragma warning(disable : 4032)		//formal parameter 'number' has different type when promoted
//#pragma warning(disable : 4051)		//type conversion; possible loss of data
//#pragma warning(disable : 4057)		// slightly different base types
#pragma warning(disable : 4100)		// unreferenced formal parameter
//#pragma warning(disable : 4115)		//'type' : named type definition in parentheses
#pragma warning(disable : 4125)		// decimal digit terminates octal escape sequence
#pragma warning(disable : 4127)		// conditional expression is constant
//#pragma warning(disable : 4136)		//conversion between different floating-point types
//#pragma warning(disable : 4201)		//nonstandard extension used : nameless struct/union
//#pragma warning(disable : 4214)		//nonstandard extension used : bit field types other than int
//#pragma warning(disable : 4220)		// varargs matches remaining parameters
#pragma warning(disable : 4244)		//'conversion' conversion from 'type1' to 'type2', possible loss of data
#pragma warning(disable : 4284)		// return type not UDT
//#pragma warning(disable : 4305)		// truncation from const double to float
#pragma warning(disable : 4310)		// cast truncates constant value
#pragma warning(disable : 4514)		//unreferenced inline/local function has been removed
#pragma warning(disable : 4710)		// not inlined
#pragma warning(disable : 4711)		// selected for automatic inline expansion
#pragma warning(disable : 4786)		// identifier was truncated
#pragma warning(disable : 5208)		// unnamed class used in typedef name cannot declare members other than non-static data members, member enumerations, or member classes

#pragma warning(disable : 4996)		// This function or variable may be unsafe.

#endif

//rww - conveniently toggle "gore" code, for model decals and stuff.
#ifndef JK2_MODE
#define _G2_GORE
#endif // !JK2_MODE

#if JK2_MODE
#define PRODUCT_NAME			"openjo_sp"

#define CLIENT_WINDOW_TITLE "OpenJO (SP)"
#define CLIENT_CONSOLE_TITLE "OpenJO Console (SP)"
#define HOMEPATH_NAME_UNIX "openjo"
#define HOMEPATH_NAME_WIN "OpenJO"
#define HOMEPATH_NAME_MACOSX HOMEPATH_NAME_WIN
#else
#define PRODUCT_NAME			"openjk_sp"

#define CLIENT_WINDOW_TITLE "OpenJK (SP)"
#define CLIENT_CONSOLE_TITLE "OpenJK Console (SP)"
#define HOMEPATH_NAME_UNIX "openjk"
#define HOMEPATH_NAME_WIN "OpenJK"
#define HOMEPATH_NAME_MACOSX HOMEPATH_NAME_WIN
#endif

#define	BASEGAME "base"
#define OPENJKGAME "OpenJK"

#define Q3CONFIG_NAME PRODUCT_NAME ".cfg"

#define BASE_SAVE_COMPAT // this is defined to disable/fix some changes that break save compatibility

#define VALIDSTRING( a )	( ( a != NULL ) && ( a[0] != '\0' ) )

//JAC: Added
#define ARRAY_LEN( x ) ( sizeof( x ) / sizeof( *(x) ) )
#define STRING( a ) #a
#define XSTRING( a ) STRING( a )

#ifndef FINAL_BUILD
#ifdef _WIN32
#define G2_PERFORMANCE_ANALYSIS
#endif
#endif

#include <assert.h>
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <stddef.h>

#ifdef __cplusplus
#include <cmath>
#endif


//Ignore __attribute__ on non-gcc platforms
#if !defined(__GNUC__) && !defined(__attribute__)
	#define __attribute__(x)
#endif

#if defined(__GNUC__)
	#define UNUSED_VAR __attribute__((unused))
#else
	#define UNUSED_VAR
#endif

#if (defined _MSC_VER)
	#define Q_EXPORT __declspec(dllexport)
#elif (defined __SUNPRO_C)
	#define Q_EXPORT __global
#elif ((__GNUC__ >= 3) && (!__EMX__) && (!sun))
	#define Q_EXPORT __attribute__((visibility("default")))
#else
	#define Q_EXPORT
#endif

// this is the define for determining if we have an asm version of a C function
#if (defined(_M_IX86) || defined(__i386__)) && !defined(__sun__)
	#define id386	1
#else
	#define id386	0
#endif

#if (defined(powerc) || defined(powerpc) || defined(ppc) || defined(__ppc) || defined(__ppc__)) && !defined(C_ONLY)
	#define idppc	1
#else
	#define idppc	0
#endif

#include "qcommon/q_platform.h"
#include "ojk_saved_game_helper_fwd.h"


// ================================================================
// TYPE DEFINITIONS
// ================================================================

typedef int32_t qhandle_t, thandle_t, fxHandle_t, sfxHandle_t, fileHandle_t, clipHandle_t;

#define NULL_HANDLE ((qhandle_t)0)
#define NULL_SOUND ((sfxHandle_t)0)
#define NULL_FX ((fxHandle_t)0)
#define NULL_SFX ((sfxHandle_t)0)
#define NULL_FILE ((fileHandle_t)0)
#define NULL_CLIP ((clipHandle_t)0)

#define PAD(base, alignment)	(((base)+(alignment)-1) & ~((alignment)-1))
#define PADLEN(base, alignment)	(PAD((base), (alignment)) - (base))

#define PADP(base, alignment)	((void *) PAD((intptr_t) (base), (alignment)))

#ifdef __GNUC__
#define QALIGN(x) __attribute__((aligned(x)))
#else
#define QALIGN(x)
#endif

#ifndef NULL
// NOTE: This is all c++ so casting to void * is wrong
#define NULL ((void *)0)
#endif

#define INT_ID( a, b, c, d ) (uint32_t)((((a) & 0xff) << 24) | (((b) & 0xff) << 16) | (((c) & 0xff) << 8) | ((d) & 0xff))

// the game guarantees that no string from the network will ever
// exceed MAX_STRING_CHARS
#define	MAX_STRING_CHARS	1024	// max length of a string passed to Cmd_TokenizeString
#define	MAX_STRING_TOKENS	1024	// max tokens resulting from Cmd_TokenizeString
#define	MAX_TOKEN_CHARS		1024	// max length of an individual token

#define	MAX_INFO_STRING		1024
#define	MAX_INFO_KEY		1024
#define	MAX_INFO_VALUE		1024

#define	BIG_INFO_STRING		8192  // used for system info key only
#define	BIG_INFO_KEY		  8192
#define	BIG_INFO_VALUE		8192

#define	MAX_QPATH			64		// max length of a quake game pathname
#ifdef PATH_MAX
#define MAX_OSPATH			PATH_MAX
#else
#define	MAX_OSPATH			256		// max length of a filesystem pathname
#endif

#define	MAX_NAME_LENGTH		32		// max length of a client name

// paramters for command buffer stuffing
typedef enum {
	EXEC_NOW,			// don't return until completed, a VM should NEVER use this,
						// because some commands might cause the VM to be unloaded...
	EXEC_INSERT,		// insert at current position, but don't run yet
	EXEC_APPEND			// add to end of the command buffer (normal case)
} cbufExec_t;


//
// these aren't needed by any of the VMs.  put in another header?
//
#define	MAX_MAP_AREA_BYTES		32		// bit vector of area visibility

// Light Style Constants

#define LS_STYLES_START			0
#define LS_NUM_STYLES			32
#define	LS_SWITCH_START			(LS_STYLES_START+LS_NUM_STYLES)
#define LS_NUM_SWITCH			32
#define MAX_LIGHT_STYLES		64

// print levels from renderer (FIXME: set up for game / cgame?)
typedef enum {
	PRINT_ALL,
	PRINT_DEVELOPER,		// only print when "developer 1"
	PRINT_WARNING,
	PRINT_ERROR
} printParm_t;

// parameters to the main Error routine
typedef enum {
	ERR_FATAL,					// exit the entire game with a popup window
	ERR_DROP,					// print to console and disconnect from game
	ERR_DISCONNECT,				// don't kill server
} errorParm_t;

// font rendering values used by ui and cgame
#define PROP_GAP_WIDTH			2 // 3
#define PROP_SPACE_WIDTH		4
#define PROP_HEIGHT				16

#define PROP_TINY_SIZE_SCALE	1
#define PROP_SMALL_SIZE_SCALE	1
#define PROP_BIG_SIZE_SCALE		1
#define PROP_GIANT_SIZE_SCALE	2

#define PROP_TINY_HEIGHT		10
#define PROP_GAP_TINY_WIDTH		1
#define PROP_SPACE_TINY_WIDTH	3

#define PROP_BIG_HEIGHT			24
#define PROP_GAP_BIG_WIDTH		3
#define PROP_SPACE_BIG_WIDTH	6


#define BLINK_DIVISOR			600
#define PULSE_DIVISOR			75

#define UI_LEFT			0x00000000	// default
#define UI_CENTER		0x00000001
#define UI_RIGHT		0x00000002
#define UI_FORMATMASK	0x00000007
#define UI_SMALLFONT	0x00000010
#define UI_BIGFONT		0x00000020	// default

#define UI_DROPSHADOW	0x00000800
#define UI_BLINK		0x00001000
#define UI_INVERSE		0x00002000
#define UI_PULSE		0x00004000


#define Com_Memset memset
#define Com_Memcpy memcpy

// stuff for TA's ROQ cinematic code...
//
#define CIN_system	1
#define CIN_loop	2
#define	CIN_hold	4
#define CIN_silent	8
#define CIN_shader	16


// all drawing is done to a 640*480 virtual screen size
// and will be automatically scaled to the real resolution
#define	SCREEN_WIDTH		640
#define	SCREEN_HEIGHT		480

#define TINYCHAR_WIDTH		(SMALLCHAR_WIDTH)
#define TINYCHAR_HEIGHT		(SMALLCHAR_HEIGHT/2)

#define SMALLCHAR_WIDTH		8
#define SMALLCHAR_HEIGHT	16

#define BIGCHAR_WIDTH		16
#define BIGCHAR_HEIGHT		16

#define	GIANTCHAR_WIDTH		32
#define	GIANTCHAR_HEIGHT	48


// Player weapons effects
typedef enum
{
	SABER_RED,
	SABER_ORANGE,
	SABER_YELLOW,
	SABER_GREEN,
	SABER_BLUE,
	SABER_PURPLE

} saber_colors_t;

#define MAX_BATTERIES	2500

#define ENUM2STRING(arg)   { #arg,arg }

//=============================================

char	*COM_SkipPath( char *pathname );
const char	*COM_GetExtension( const char *name );
void	COM_StripExtension( const char *in, char *out, int destsize );
qboolean COM_CompareExtension(const char *in, const char *ext);
void	COM_DefaultExtension( char *path, int maxSize, const char *extension );

//JLFCALLOUT include MPNOTUSED
void	 COM_BeginParseSession( void );
void	 COM_EndParseSession( void );

// For compatibility with shared code
QINLINE void COM_BeginParseSession( const char *sessionName )
{
	COM_BeginParseSession();
}

class COM_ParseSession {
public:
	COM_ParseSession() { COM_BeginParseSession(); };
	~COM_ParseSession() { COM_EndParseSession(); };
};

int		 COM_GetCurrentParseLine( void );
char	*COM_Parse( const char **data_p );
char	*COM_ParseExt( const char **data_p, qboolean allowLineBreak );
int		 COM_Compress( char *data_p );
qboolean COM_ParseString( const char **data, const char **s );
qboolean COM_ParseInt( const char **data, int *i );
qboolean COM_ParseFloat( const char **data, float *f );
qboolean COM_ParseVec4( const char **buffer, vec4_t *c);

// data is an in/out parm, returns a parsed out token

void	COM_MatchToken( char**buf_p, char *match );

void SkipBracedSection (const char **program);
void SkipRestOfLine ( const char **data );

void Parse1DMatrix (const char **buf_p, int x, float *m);
void Parse2DMatrix (const char **buf_p, int y, int x, float *m);
void Parse3DMatrix (const char **buf_p, int z, int y, int x, float *m);
int Com_HexStrToInt( const char *str );

int	QDECL Com_sprintf (char *dest, int size, const char *fmt, ...);

char *Com_SkipTokens( char *s, int numTokens, char *sep );
char *Com_SkipCharset( char *s, char *sep );


// mode parm for FS_FOpenFile
typedef enum {
	FS_READ,
	FS_WRITE,
	FS_APPEND,
	FS_APPEND_SYNC
} fsMode_t;

typedef enum {
	FS_SEEK_CUR,
	FS_SEEK_END,
	FS_SEEK_SET
} fsOrigin_t;

//=============================================
char	* QDECL va(const char *format, ...);

#define TRUNCATE_LENGTH	64
void Com_TruncateLongString( char *buffer, const char *s );

//=============================================

//
// key / value info strings
//
const char *Info_ValueForKey( const char *s, const char *key );
void Info_RemoveKey( char *s, const char *key );
void Info_SetValueForKey( char *s, const char *key, const char *value );
qboolean Info_Validate( const char *s );
void Info_NextPair( const char **s, char key[MAX_INFO_KEY], char value[MAX_INFO_VALUE] );

// this is only here so the functions in q_shared.c and bg_*.c can link
void	NORETURN QDECL Com_Error( int level, const char *error, ... );
void	QDECL Com_Printf( const char *msg, ... );


/*
==========================================================

CVARS (console variables)

Many variables can be used for cheating purposes, so when
cheats is zero, force all unspecified variables to their
default values.
==========================================================
*/

#define	CVAR_TEMP			0	// can be set even when cheats are disabled, but is not archived
#define	CVAR_ARCHIVE		1	// set to cause it to be saved to vars.rc
								// used for system variables, not for player
								// specific configurations
#define	CVAR_USERINFO		2	// sent to server on connect or change
#define	CVAR_SERVERINFO		4	// sent in response to front end requests
#define	CVAR_SYSTEMINFO		8	// these cvars will be duplicated on all clients
#define	CVAR_INIT			16	// don't allow change from console at all,
								// but can be set from the command line
#define	CVAR_LATCH			32	// will only change when C code next does
								// a Cvar_Get(), so it can't be changed
								// without proper initialization.  modified
								// will be set, even though the value hasn't
								// changed yet
#define	CVAR_ROM			64	// display only, cannot be set by user at all
#define	CVAR_USER_CREATED	128	// created by a set command
#define	CVAR_SAVEGAME		256	// store this in the savegame
#define CVAR_CHEAT			512	// can not be changed if cheats are disabled
#define CVAR_NORESTART		1024	// do not clear when a cvar_restart is issued

#define CVAR_SERVER_CREATED	2048	// cvar was created by a server the client connected to.
#define CVAR_VM_CREATED		4096	// cvar was created exclusively in one of the VMs.
#define CVAR_PROTECTED		8192	// prevent modifying this var from VMs or the server
#define CVAR_NODEFAULT		16384	// do not write to config if matching with default value

#define CVAR_ARCHIVE_ND		(CVAR_ARCHIVE | CVAR_NODEFAULT)
// These flags are only returned by the Cvar_Flags() function
#define CVAR_MODIFIED		0x40000000		// Cvar was modified
#define CVAR_NONEXISTENT	0x80000000		// Cvar doesn't exist.

// nothing outside the Cvar_*() functions should modify these fields!
typedef struct cvar_s {
	char		*name;
	char		*string;
	char		*resetString;		// cvar_restart will reset to this value
	char		*latchedString;		// for CVAR_LATCH vars
	int			flags;
	qboolean	modified;			// set each time the cvar is changed
	int			modificationCount;	// incremented each time the cvar is changed
	float		value;				// atof( string )
	int			integer;			// atoi( string )
	qboolean	validate;
	qboolean	integral;
	float		min;
	float		max;
	struct cvar_s *next;
	struct cvar_s *prev;
	struct cvar_s *hashNext;
	struct cvar_s *hashPrev;
	int			hashIndex;
} cvar_t;

#define	MAX_CVAR_VALUE_STRING	256

typedef int	cvarHandle_t;

// the modules that run in the virtual machine can't access the cvar_t directly,
// so they must ask for structured updates
typedef struct {
	cvarHandle_t	handle;
	int			modificationCount;
	float		value;
	int			integer;
	char		string[MAX_CVAR_VALUE_STRING];
} vmCvar_t;

/*
==============================================================

COLLISION DETECTION

==============================================================
*/

#include "../game/surfaceflags.h"			// shared with the q3map utility

/*
Ghoul2 Insert Start
*/

#if !defined(GHOUL2_SHARED_H_INC)
	#include "../game/ghoul2_shared.h"	//for CGhoul2Info_v
#endif

/*
Ghoul2 Insert End
*/

#define MAX_G2_COLLISIONS 16
// a trace is returned when a box is swept through the world
typedef struct {
	qboolean	allsolid;	// if true, plane is not valid
	qboolean	startsolid;	// if true, the initial point was in a solid area
	float		fraction;	// time completed, 1.0 = didn't hit anything
	vec3_t		endpos;		// final position
	cplane_t	plane;		// surface normal at impact, transformed to world space
	int			surfaceFlags;	// surface hit
	int			contents;	// contents on other side of surface hit
	int			entityNum;	// entity the contacted sirface is a part of
/*
Ghoul2 Insert Start
*/
	CCollisionRecord G2CollisionMap[MAX_G2_COLLISIONS];	// map that describes all of the parts of ghoul2 models that got hit
/*
Ghoul2 Insert End
*/


	void sg_export(
		ojk::SavedGameHelper& saved_game) const
	{
		saved_game.write<int8_t>(allsolid);
		saved_game.write<int8_t>(startsolid);
		saved_game.write<float>(fraction);
		saved_game.write<float>(endpos);
		saved_game.write<>(plane);
		saved_game.write<int8_t>(surfaceFlags);
		saved_game.write<int8_t>(contents);
		saved_game.write<int8_t>(entityNum);
		saved_game.write<>(G2CollisionMap);
	}

	void sg_import(
		ojk::SavedGameHelper& saved_game)
	{
		saved_game.read<int8_t>(allsolid);
		saved_game.read<int8_t>(startsolid);
		saved_game.read<float>(fraction);
		saved_game.read<float>(endpos);
		saved_game.read<>(plane);
		saved_game.read<int8_t>(surfaceFlags);
		saved_game.read<int8_t>(contents);
		saved_game.read<int8_t>(entityNum);
		saved_game.read<>(G2CollisionMap);
	}
} trace_t;

// trace->entityNum can also be 0 to (MAX_GENTITIES-1)
// or ENTITYNUM_NONE, ENTITYNUM_WORLD


// markfragments are returned by CM_MarkFragments()
typedef struct {
	int		firstPoint;
	int		numPoints;
} markFragment_t;



typedef struct {
	vec3_t		origin;
	vec3_t		axis[3];
} orientation_t;

//=====================================================================


// in order from highest priority to lowest
// if none of the catchers are active, bound key strings will be executed
#define KEYCATCH_CONSOLE	1
#define	KEYCATCH_UI			2


// sound channels
// channel 0 never willingly overrides
// other channels will allways override a playing sound on that channel
#include "../game/channels.h"

/*
========================================================================

  ELEMENTS COMMUNICATED ACROSS THE NET

========================================================================
*/

#define	ANGLE2SHORT(x)	((int)((x)*65536/360) & 65535)
#define	SHORT2ANGLE(x)	((x)*(360.0/65536))

#define	SNAPFLAG_RATE_DELAYED	1
#define	SNAPFLAG_NOT_ACTIVE		2	// snapshot used during connection and for zombies
#define SNAPFLAG_SERVERCOUNT	4	// toggled every map_restart so transitions can be detected

//
// per-level limits
//
#define	MAX_CLIENTS			1 // 128		// absolute limit
#define MAX_TERRAINS		1 //32

#define	GENTITYNUM_BITS		10		// don't need to send any more
#define	MAX_GENTITIES		(1<<GENTITYNUM_BITS)

// entitynums are communicated with GENTITY_BITS, so any reserved
// values thatare going to be communcated over the net need to
// also be in this range
#define	ENTITYNUM_NONE		(MAX_GENTITIES-1)
#define	ENTITYNUM_WORLD		(MAX_GENTITIES-2)
#define	ENTITYNUM_MAX_NORMAL	(MAX_GENTITIES-2)


#define	MAX_MODELS			256

#ifdef JK2_MODE
#define MAX_SOUNDS (256)
#else
#define	MAX_SOUNDS			380
#endif // JK2_MODE

#define MAX_SUB_BSP			32

#define	MAX_SUBMODELS		512		// nine bits

#define MAX_FX				128

#ifdef JK2_MODE
#define MAX_WORLD_FX (4)
#else
#define MAX_WORLD_FX		66		// was 16 // was 4
#endif // JK2_MODE

/*
Ghoul2 Insert Start
*/
#define	MAX_CHARSKINS		64		// character skins
/*
Ghoul2 Insert End
*/

#ifdef JK2_MODE
#define MAX_CONFIGSTRINGS (1024)
#else
#define	MAX_CONFIGSTRINGS	1300//1024 //rww - I had to up this for terrains
#endif // JK2_MODE

// these are the only configstrings that the system reserves, all the
// other ones are strictly for servergame to clientgame communication
#define	CS_SERVERINFO		0		// an info string with all the serverinfo cvars
#define	CS_SYSTEMINFO		1		// an info string for server system to client system configuration (timescale, etc)

#define	RESERVED_CONFIGSTRINGS	2	// game can't modify below this, only the system can
//
// config strings are a general means of communicating variable length strings
// from the server to all connected clients.
//

// CS_SERVERINFO and CS_SYSTEMINFO are defined in q_shared.h
#define	CS_MUSIC			2
#define	CS_MESSAGE			3		// from the map worldspawn's message field
#define	CS_ITEMS			4		// string of 0's and 1's that tell which items are present
#define CS_AMBIENT_SET		5		// ambient set information for the player

#define	CS_MODELS			10

#ifndef JK2_MODE
#define	CS_SKYBOXORG		(CS_MODELS+MAX_MODELS)		//rww - skybox info
#endif // !JK2_MODE

#ifdef JK2_MODE
#define CS_SOUNDS (CS_MODELS + MAX_MODELS)
#else
#define	CS_SOUNDS			(CS_SKYBOXORG+1)
#endif // JK2_MODE

#ifdef BASE_SAVE_COMPAT
#define CS_RESERVED1		(CS_SOUNDS+MAX_SOUNDS) // reserved field for base compat from immersion removal
#define	CS_PLAYERS			(CS_RESERVED1 + 96)
#else
#define	CS_PLAYERS			(CS_SOUNDS+MAX_SOUNDS)
#endif

#define	CS_LIGHT_STYLES		(CS_PLAYERS+MAX_CLIENTS)

#ifndef JK2_MODE
#define CS_TERRAINS			(CS_LIGHT_STYLES + (MAX_LIGHT_STYLES*3))
#define CS_BSP_MODELS		(CS_TERRAINS + MAX_TERRAINS)
#endif // !JK2_MODE

#ifdef JK2_MODE
#define CS_EFFECTS (CS_LIGHT_STYLES + (MAX_LIGHT_STYLES * 3))
#else
#define CS_EFFECTS			(CS_BSP_MODELS + MAX_SUB_BSP)//(CS_LIGHT_STYLES + (MAX_LIGHT_STYLES*3))
#endif // JK2_MODE

/*
Ghoul2 Insert Start
*/
#define CS_CHARSKINS 		(CS_EFFECTS + MAX_FX)
/*
Ghoul2 Insert End
*/
#define CS_DYNAMIC_MUSIC_STATE	(CS_CHARSKINS + MAX_CHARSKINS)
#define CS_WORLD_FX				(CS_DYNAMIC_MUSIC_STATE + 1)
#define CS_MAX					(CS_WORLD_FX + MAX_WORLD_FX)

#if (CS_MAX) > MAX_CONFIGSTRINGS
#error overflow: (CS_MAX) > MAX_CONFIGSTRINGS
#endif

#define	MAX_GAMESTATE_CHARS	16000
typedef struct {
	int			stringOffsets[MAX_CONFIGSTRINGS];
	char		stringData[MAX_GAMESTATE_CHARS];
	int			dataCount;
} gameState_t;

typedef enum
{
	FP_FIRST = 0,//marker
	FP_HEAL = 0,//instant
	FP_LEVITATION,//hold/duration
	FP_SPEED,//duration
	FP_PUSH,//hold/duration
	FP_PULL,//hold/duration
	FP_TELEPATHY,//instant
	FP_GRIP,//hold/duration
	FP_LIGHTNING,//hold/duration
	FP_SABERTHROW,
	FP_SABER_DEFENSE,
	FP_SABER_OFFENSE,

#ifndef JK2_MODE
	//new Jedi Academy powers
	FP_RAGE,//duration - speed, invincibility and extra damage for short period, drains your health and leaves you weak and slow afterwards.
	FP_PROTECT,//duration - protect against physical/energy (level 1 stops blaster/energy bolts, level 2 stops projectiles, level 3 protects against explosions)
	FP_ABSORB,//duration - protect against dark force powers (grip, lightning, drain - maybe push/pull, too?)
	FP_DRAIN,//hold/duration - drain force power for health
	FP_SEE,//duration - detect/see hidden enemies
#endif // !JK2_MODE

	NUM_FORCE_POWERS
} forcePowers_t;

typedef enum
{
	SABER_NONE = 0,
	SABER_SINGLE,
	SABER_STAFF,
	SABER_DAGGER,
	SABER_BROAD,
	SABER_PRONG,
	SABER_ARC,
	SABER_SAI,
	SABER_CLAW,
	SABER_LANCE,
	SABER_STAR,
	SABER_TRIDENT,
	SABER_SITH_SWORD,
	NUM_SABERS
} saberType_t;

//=========================================================

// bit field limits
#define	MAX_STATS				16

// NOTE!!! be careful about altering this because although it's used to define an array size, the entry indexes come from
//	the typedef'd enum "persEnum_t" in bg_public.h, and there's no compile-tie between the 2 -slc
//
#define	MAX_PERSISTANT			16

#define	MAX_POWERUPS			16
#define	MAX_WEAPONS				32
#define MAX_AMMO				10
#define MAX_INVENTORY			15		// See INV_MAX
#define MAX_SECURITY_KEYS		5
#define MAX_SECURITY_KEY_MESSSAGE		24

#define	MAX_PS_EVENTS			2		// this must be a power of 2 unless you change some &'s to %'s -ste


#define MAX_WORLD_COORD		( 64*1024 )
#define MIN_WORLD_COORD		( -64*1024 )
#define WORLD_SIZE			( MAX_WORLD_COORD - MIN_WORLD_COORD )

typedef enum
{
	WHL_NONE,
	WHL_ANKLES,
	WHL_KNEES,
	WHL_WAIST,
	WHL_TORSO,
	WHL_SHOULDERS,
	WHL_HEAD,
	WHL_UNDER
} waterHeightLevel_t;

// !!!!!!! loadsave affecting struct !!!!!!!
typedef struct
{
	// Actual trail stuff
	int		inAction;	// controls whether should we even consider starting one
	int		duration;	// how long each trail seg stays in existence
	int		lastTime;	// time a saber segement was last stored
	vec3_t	base;
	vec3_t	tip;

	// Marks stuff
	qboolean	haveOldPos[2];
	vec3_t		oldPos[2];
	vec3_t		oldNormal[2];	// store this in case we don't have a connect-the-dots situation
							//	..then we'll need the normal to project a mark blob onto the impact point


	void sg_export(
		ojk::SavedGameHelper& saved_game) const
	{
		saved_game.write<int32_t>(inAction);
		saved_game.write<int32_t>(duration);
		saved_game.write<int32_t>(lastTime);
		saved_game.write<float>(base);
		saved_game.write<float>(tip);
		saved_game.write<int32_t>(haveOldPos);
		saved_game.write<float>(oldPos);
		saved_game.write<float>(oldNormal);
	}

	void sg_import(
		ojk::SavedGameHelper& saved_game)
	{
		saved_game.read<int32_t>(inAction);
		saved_game.read<int32_t>(duration);
		saved_game.read<int32_t>(lastTime);
		saved_game.read<float>(base);
		saved_game.read<float>(tip);
		saved_game.read<int32_t>(haveOldPos);
		saved_game.read<float>(oldPos);
		saved_game.read<float>(oldNormal);
	}
} saberTrail_t;

#define MAX_SABER_TRAIL_SEGS 8

// !!!!!!!!!!!!! loadsave affecting struct !!!!!!!!!!!!!!!
typedef struct
{
	qboolean	active;
	saber_colors_t	color;
	float		radius;
	float		length;
	float		lengthMax;
	float		lengthOld;
	vec3_t		muzzlePoint;
	vec3_t		muzzlePointOld;
	vec3_t		muzzleDir;
	vec3_t		muzzleDirOld;
	saberTrail_t	trail;
	void		ActivateTrail ( float duration )
				{
					trail.inAction = qtrue;
					trail.duration = duration;
				};
	void		DeactivateTrail ( float duration )
				{
					trail.inAction = qfalse;
					trail.duration = duration;
				};


	void sg_export(
		ojk::SavedGameHelper& saved_game) const
	{
		saved_game.write<int32_t>(active);
		saved_game.write<int32_t>(color);
		saved_game.write<float>(radius);
		saved_game.write<float>(length);
		saved_game.write<float>(lengthMax);
		saved_game.write<float>(lengthOld);
		saved_game.write<float>(muzzlePoint);
		saved_game.write<float>(muzzlePointOld);
		saved_game.write<float>(muzzleDir);
		saved_game.write<float>(muzzleDirOld);
		saved_game.write<>(trail);
	}

	void sg_import(
		ojk::SavedGameHelper& saved_game)
	{
		saved_game.read<int32_t>(active);
		saved_game.read<int32_t>(color);
		saved_game.read<float>(radius);
		saved_game.read<float>(length);
		saved_game.read<float>(lengthMax);
		saved_game.read<float>(lengthOld);
		saved_game.read<float>(muzzlePoint);
		saved_game.read<float>(muzzlePointOld);
		saved_game.read<float>(muzzleDir);
		saved_game.read<float>(muzzleDirOld);
		saved_game.read<>(trail);
	}
} bladeInfo_t;

#define MAX_BLADES 8

typedef enum
{
	SS_NONE = 0,
	SS_FAST,
	SS_MEDIUM,
	SS_STRONG,
	SS_DESANN,
	SS_TAVION,
	SS_DUAL,
	SS_STAFF,
	SS_NUM_SABER_STYLES
} saber_styles_t;

//SABER FLAGS
//Old bools converted to a flag now
#define SFL_NOT_LOCKABLE			(1<<0)//can't get into a saberlock
#define SFL_NOT_THROWABLE			(1<<1)//can't be thrown - FIXME: maybe make this a max level of force saber throw that can be used with this saber?
#define SFL_NOT_DISARMABLE			(1<<2)//can't be dropped
#define SFL_NOT_ACTIVE_BLOCKING		(1<<3)//don't to try to block incoming shots with this saber
#define SFL_TWO_HANDED				(1<<4)//uses both hands
#define SFL_SINGLE_BLADE_THROWABLE	(1<<5)//can throw this saber if only the first blade is on
#define SFL_RETURN_DAMAGE			(1<<6)//when returning from a saber throw, it keeps spinning and doing damage
//NEW FLAGS
#define SFL_ON_IN_WATER				(1<<7)//if set, weapon stays active even in water
#define SFL_BOUNCE_ON_WALLS			(1<<8)//if set, the saber will bounce back when it hits solid architecture (good for real-sword type mods)
#define SFL_BOLT_TO_WRIST			(1<<9)//if set, saber model is bolted to wrist, not in hand... useful for things like claws & shields, etc.
//#define SFL_STICK_ON_IMPACT		(1<<?)//if set, the saber will stick in the wall when thrown and hits solid architecture (good for sabers that are meant to be thrown).
//#define SFL_NO_ATTACK				(1<<?)//if set, you cannot attack with the saber (for sabers/weapons that are meant to be thrown only, not used as melee weapons).
//Move Restrictions
#define SFL_NO_PULL_ATTACK			(1<<10)//if set, cannot do pull+attack move (move not available in MP anyway)
#define SFL_NO_BACK_ATTACK			(1<<11)//if set, cannot do back-stab moves
#define SFL_NO_STABDOWN				(1<<12)//if set, cannot do stabdown move (when enemy is on ground)
#define SFL_NO_WALL_RUNS			(1<<13)//if set, cannot side-run or forward-run on walls
#define SFL_NO_WALL_FLIPS			(1<<14)//if set, cannot do backflip off wall or side-flips off walls
#define SFL_NO_WALL_GRAB			(1<<15)//if set, cannot grab wall & jump off
#define SFL_NO_ROLLS				(1<<16)//if set, cannot roll
#define SFL_NO_FLIPS				(1<<17)//if set, cannot do flips
#define SFL_NO_CARTWHEELS			(1<<18)//if set, cannot do cartwheels
#define SFL_NO_KICKS				(1<<19)//if set, cannot do kicks (can't do kicks anyway if using a throwable saber/sword)
#define SFL_NO_MIRROR_ATTACKS		(1<<20)//if set, cannot do the simultaneous attack left/right moves (only available in Dual Lightsaber Combat Style)
#define SFL_NO_ROLL_STAB			(1<<21)//if set, cannot do roll-stab move at end of roll
//SABER FLAGS2
//Primary Blade Style
#define SFL2_NO_WALL_MARKS			(1<<0)//if set, stops the saber from drawing marks on the world (good for real-sword type mods)
#define SFL2_NO_DLIGHT				(1<<1)//if set, stops the saber from drawing a dynamic light (good for real-sword type mods)
#define SFL2_NO_BLADE				(1<<2)//if set, stops the saber from drawing a blade (good for real-sword type mods)
#define SFL2_NO_CLASH_FLARE			(1<<3)//if set, the saber will not do the big, white clash flare with other sabers
#define SFL2_NO_DISMEMBERMENT		(1<<4)//if set, the saber never does dismemberment (good for pointed/blunt melee weapons)
#define SFL2_NO_IDLE_EFFECT			(1<<5)//if set, the saber will not do damage or any effects when it is idle (not in an attack anim).  (good for real-sword type mods)
#define SFL2_ALWAYS_BLOCK			(1<<6)//if set, the blades will always be blocking (good for things like shields that should always block)
#define SFL2_NO_MANUAL_DEACTIVATE	(1<<7)//if set, the blades cannot manually be toggled on and off
#define SFL2_TRANSITION_DAMAGE		(1<<8)//if set, the blade does damage in start, transition and return anims (like strong style does)
//Secondary Blade Style
#define SFL2_NO_WALL_MARKS2			(1<<9)//if set, stops the saber from drawing marks on the world (good for real-sword type mods)
#define SFL2_NO_DLIGHT2				(1<<10)//if set, stops the saber from drawing a dynamic light (good for real-sword type mods)
#define SFL2_NO_BLADE2				(1<<11)//if set, stops the saber from drawing a blade (good for real-sword type mods)
#define SFL2_NO_CLASH_FLARE2		(1<<12)//if set, the saber will not do the big, white clash flare with other sabers
#define SFL2_NO_DISMEMBERMENT2		(1<<13)//if set, the saber never does dismemberment (good for pointed/blunt melee weapons)
#define SFL2_NO_IDLE_EFFECT2		(1<<14)//if set, the saber will not do damage or any effects when it is idle (not in an attack anim).  (good for real-sword type mods)
#define SFL2_ALWAYS_BLOCK2			(1<<15)//if set, the blades will always be blocking (good for things like shields that should always block)
#define SFL2_NO_MANUAL_DEACTIVATE2	(1<<16)//if set, the blades cannot manually be toggled on and off
#define SFL2_TRANSITION_DAMAGE2		(1<<17)//if set, the blade does damage in start, transition and return anims (like strong style does)

// !!!!!!!!!!!! loadsave affecting struct !!!!!!!!!!!!!!!!!!!!!!!!!!
typedef struct
{
	char		*name;						//entry in sabers.cfg, if any
	char		*fullName;					//the "Proper Name" of the saber, shown in the UI
	saberType_t	type;						//none, single or staff
	char		*model;						//hilt model
	char		*skin;						//hilt custom skin
	int			soundOn;					//game soundindex for turning on sound
	int			soundLoop;					//game soundindex for hum/loop sound
	int			soundOff;					//game soundindex for turning off sound
	int			numBlades;
	bladeInfo_t	blade[MAX_BLADES];			//blade info - like length, trail, origin, dir, etc.
	int			stylesLearned;				//styles you get when you get this saber, if any
	int			stylesForbidden;			//styles you cannot use with this saber, if any
	int			maxChain;					//how many moves can be chained in a row with this weapon (-1 is infinite, 0 is use default behavior)
	int			forceRestrictions;			//force powers that cannot be used while this saber is on (bitfield) - FIXME: maybe make this a limit on the max level, per force power, that can be used with this type?
	int			lockBonus;					//in saberlocks, this type of saber pushes harder or weaker
	int			parryBonus;					//added to strength of parry with this saber
	int			breakParryBonus;			//added to strength when hit a parry
	int			breakParryBonus2;			//for bladeStyle2 (see bladeStyle2Start below)
	int			disarmBonus;				//added to disarm chance when win saberlock or have a good parry (knockaway)
	int			disarmBonus2;				//for bladeStyle2 (see bladeStyle2Start below)
	saber_styles_t	singleBladeStyle;		//makes it so that you use a different style if you only have the first blade active
	char		*brokenSaber1;				//if saber is actually hit by another saber, it can be cut in half/broken and will be replaced with this saber in your right hand
	char		*brokenSaber2;				//if saber is actually hit by another saber, it can be cut in half/broken and will be replaced with this saber in your left hand
//===NEW========================================================================================
	//these values are global to the saber, like all of the ones above
	int			saberFlags;					//from SFL_ list above
	int			saberFlags2;				//from SFL2_ list above

	//done in cgame (client-side code)
	qhandle_t	spinSound;					//none - if set, plays this sound as it spins when thrown
	qhandle_t	swingSound[3];				//none - if set, plays one of these 3 sounds when swung during an attack - NOTE: must provide all 3!!!
	qhandle_t	fallSound[3];				//none - if set, plays one of these 3 sounds when weapon drops to the ground - NOTE: must provide all 3!!!

	//done in game (server-side code)
	float		moveSpeedScale;				//1.0 - you move faster/slower when using this saber
	float		animSpeedScale;				//1.0 - plays normal attack animations faster/slower

	//done in both cgame and game (BG code)
	int	kataMove;				//LS_INVALID - if set, player will execute this move when they press both attack buttons at the same time
	int	lungeAtkMove;			//LS_INVALID - if set, player will execute this move when they crouch+fwd+attack
	int	jumpAtkUpMove;			//LS_INVALID - if set, player will execute this move when they jump+attack
	int	jumpAtkFwdMove;			//LS_INVALID - if set, player will execute this move when they jump+fwd+attack
	int	jumpAtkBackMove;		//LS_INVALID - if set, player will execute this move when they jump+back+attack
	int	jumpAtkRightMove;		//LS_INVALID - if set, player will execute this move when they jump+rightattack
	int	jumpAtkLeftMove;		//LS_INVALID - if set, player will execute this move when they jump+left+attack
	int	readyAnim;				//-1 - anim to use when standing idle
	int	drawAnim;				//-1 - anim to use when drawing weapon
	int	putawayAnim;			//-1 - anim to use when putting weapon away
	int	tauntAnim;				//-1 - anim to use when hit "taunt"
	int	bowAnim;				//-1 - anim to use when hit "bow"
	int	meditateAnim;			//-1 - anim to use when hit "meditate"
	int	flourishAnim;			//-1 - anim to use when hit "flourish"
	int	gloatAnim;				//-1 - anim to use when hit "gloat"

	//***NOTE: you can only have a maximum of 2 "styles" of blades, so this next value, "bladeStyle2Start" is the number of the first blade to use these value on... all blades before this use the normal values above, all blades at and after this number use the secondary values below***
	int			bladeStyle2Start;			//0 - if set, blades from this number and higher use the following values (otherwise, they use the normal values already set)

	//***The following can be different for the extra blades - not setting them individually defaults them to the value for the whole saber (and first blade)***

	//===PRIMARY BLADES=====================
	//done in cgame (client-side code)
	int			trailStyle;					//0 - default (0) is normal, 1 is a motion blur and 2 is no trail at all (good for real-sword type mods)
	char		g2MarksShader[MAX_QPATH];	//none - if set, the game will use this shader for marks on enemies instead of the default "gfx/damage/saberglowmark"
	char		g2WeaponMarkShader[MAX_QPATH];	//none - if set, the game will ry to project this shader onto the weapon when it damages a person (good for a blood splatter on the weapon)
	//int		bladeShader;				//none - if set, overrides the shader used for the saber blade?
	//int		trailShader;				//none - if set, overrides the shader used for the saber trail?
	qhandle_t	hitSound[3];				//none - if set, plays one of these 3 sounds when saber hits a person - NOTE: must provide all 3!!!
	qhandle_t	blockSound[3];				//none - if set, plays one of these 3 sounds when saber/sword hits another saber/sword - NOTE: must provide all 3!!!
	qhandle_t	bounceSound[3];				//none - if set, plays one of these 3 sounds when saber/sword hits a wall and bounces off (must set bounceOnWall to 1 to use these sounds) - NOTE: must provide all 3!!!
	int			blockEffect;				//none - if set, plays this effect when the saber/sword hits another saber/sword (instead of "saber/saber_block.efx")
	int			hitPersonEffect;			//none - if set, plays this effect when the saber/sword hits a person (instead of "saber/blood_sparks_mp.efx")
	int			hitOtherEffect;				//none - if set, plays this effect when the saber/sword hits something else damagable (instead of "saber/saber_cut.efx")
	int			bladeEffect;				//none - if set, plays this effect at the blade tag

	//done in game (server-side code)
	float		knockbackScale;				//0 - if non-zero, uses damage done to calculate an appropriate amount of knockback
	float		damageScale;				//1 - scale up or down the damage done by the saber
	float		splashRadius;				//0 - radius of splashDamage
	int			splashDamage;				//0 - amount of splashDamage, 100% at a distance of 0, 0% at a distance = splashRadius
	float		splashKnockback;			//0 - amount of splashKnockback, 100% at a distance of 0, 0% at a distance = splashRadius

	//===SECONDARY BLADES===================
	//done in cgame (client-side code)
	int			trailStyle2;				//0 - default (0) is normal, 1 is a motion blur and 2 is no trail at all (good for real-sword type mods)
	char		g2MarksShader2[MAX_QPATH];	//none - if set, the game will use this shader for marks on enemies instead of the default "gfx/damage/saberglowmark"
	char		g2WeaponMarkShader2[MAX_QPATH];	//none - if set, the game will ry to project this shader onto the weapon when it damages a person (good for a blood splatter on the weapon)
	//int		bladeShader2;				//none - if set, overrides the shader used for the saber blade?
	//int		trailShader2;				//none - if set, overrides the shader used for the saber trail?
	qhandle_t	hit2Sound[3];				//none - if set, plays one of these 3 sounds when saber hits a person - NOTE: must provide all 3!!!
	qhandle_t	block2Sound[3];				//none - if set, plays one of these 3 sounds when saber/sword hits another saber/sword - NOTE: must provide all 3!!!
	qhandle_t	bounce2Sound[3];			//none - if set, plays one of these 3 sounds when saber/sword hits a wall and bounces off (must set bounceOnWall to 1 to use these sounds) - NOTE: must provide all 3!!!
	int			blockEffect2;				//none - if set, plays this effect when the saber/sword hits another saber/sword (instead of "saber/saber_block.efx")
	int			hitPersonEffect2;			//none - if set, plays this effect when the saber/sword hits a person (instead of "saber/blood_sparks_mp.efx")
	int			hitOtherEffect2;			//none - if set, plays this effect when the saber/sword hits something else damagable (instead of "saber/saber_cut.efx")
	int			bladeEffect2;				//none - if set, plays this effect at the blade tag

	//done in game (server-side code)
	float		knockbackScale2;			//0 - if non-zero, uses damage done to calculate an appropriate amount of knockback
	float		damageScale2;				//1 - scale up or down the damage done by the saber
	float		splashRadius2;				//0 - radius of splashDamage
	int			splashDamage2;				//0 - amount of splashDamage, 100% at a distance of 0, 0% at a distance = splashRadius
	float		splashKnockback2;			//0 - amount of splashKnockback, 100% at a distance of 0, 0% at a distance = splashRadius
//=========================================================================================================================================
	void		Activate( void )
				{
					for ( int i = 0; i < numBlades; i++ )
					{
						blade[i].active = qtrue;
					}
				};

	void		Deactivate( void )
				{
					for ( int i = 0; i < numBlades; i++ )
					{
						blade[i].active = qfalse;
					}
				};

	// Description: Activate a specific Blade of this Saber.
	// Created: 10/03/02 by Aurelio Reis, Modified: 10/03/02 by Aurelio Reis.
	//	[in]		int iBlade		Which Blade to activate.
	//	[in]		bool bActive	Whether to activate it (default true), or deactivate it (false).
	//	[return]	void
	void		BladeActivate( int iBlade, qboolean bActive = qtrue )
				{
					// Validate blade ID/Index.
					if ( iBlade < 0 || iBlade >= numBlades )
						return;

					blade[iBlade].active = bActive;
				}

	qboolean	Active()
				{
					for ( int i = 0; i < numBlades; i++ )
					{
						if ( blade[i].active )
						{
							return qtrue;
						}
					}
					return qfalse;
				}
	qboolean	ActiveManualOnly()
				{
					for ( int i = 0; i < numBlades; i++ )
					{
						if ( bladeStyle2Start > 0 )
						{
							if ( i >= bladeStyle2Start )
							{
								if ( (saberFlags2&SFL2_NO_MANUAL_DEACTIVATE2) )
								{//don't count this blade
									continue;
								}
							}
							else if ( (saberFlags2&SFL2_NO_MANUAL_DEACTIVATE) )
							{//don't count this blade
								continue;
							}
						}
						else if ( (saberFlags2&SFL2_NO_MANUAL_DEACTIVATE) )
						{//don't count any of these blades!
							continue;
						}
						else if ( blade[i].active )
						{
							return qtrue;
						}
					}
					return qfalse;
				}
	void		SetLength( float length )
				{
					for ( int i = 0; i < numBlades; i++ )
					{
						blade[i].length = length;
					}
				}
	float		Length()
				{//return largest length
					float len1 = 0;
					for ( int i = 0; i < numBlades; i++ )
					{
						if ( blade[i].length > len1 )
						{
							len1 = blade[i].length;
						}
					}
					return len1;
				};
	float		LengthMax()
				{
					float len1 = 0;
					for ( int i = 0; i < numBlades; i++ )
					{
						if ( blade[i].lengthMax > len1 )
						{
							len1 = blade[i].lengthMax;
						}
					}
					return len1;
				};
	void		ActivateTrail ( float duration )
				{
					for ( int i = 0; i < numBlades; i++ )
					{
						blade[i].ActivateTrail( duration );
					}
				};
	void		DeactivateTrail ( float duration )
				{
					for ( int i = 0; i < numBlades; i++ )
					{
						blade[i].DeactivateTrail( duration );
					}
				};


	void sg_export(
		ojk::SavedGameHelper& saved_game) const
	{
		saved_game.write<int32_t>(name);
		saved_game.write<int32_t>(fullName);
		saved_game.write<int32_t>(type);
		saved_game.write<int32_t>(model);
		saved_game.write<int32_t>(skin);
		saved_game.write<int32_t>(soundOn);
		saved_game.write<int32_t>(soundLoop);
		saved_game.write<int32_t>(soundOff);
		saved_game.write<int32_t>(numBlades);
		saved_game.write<>(blade);
		saved_game.write<int32_t>(stylesLearned);
		saved_game.write<int32_t>(stylesForbidden);
		saved_game.write<int32_t>(maxChain);
		saved_game.write<int32_t>(forceRestrictions);
		saved_game.write<int32_t>(lockBonus);
		saved_game.write<int32_t>(parryBonus);
		saved_game.write<int32_t>(breakParryBonus);
		saved_game.write<int32_t>(breakParryBonus2);
		saved_game.write<int32_t>(disarmBonus);
		saved_game.write<int32_t>(disarmBonus2);
		saved_game.write<int32_t>(singleBladeStyle);
		saved_game.write<int32_t>(brokenSaber1);
		saved_game.write<int32_t>(brokenSaber2);
		saved_game.write<int32_t>(saberFlags);
		saved_game.write<int32_t>(saberFlags2);
		saved_game.write<int32_t>(spinSound);
		saved_game.write<int32_t>(swingSound);
		saved_game.write<int32_t>(fallSound);
		saved_game.write<float>(moveSpeedScale);
		saved_game.write<float>(animSpeedScale);
		saved_game.write<int32_t>(kataMove);
		saved_game.write<int32_t>(lungeAtkMove);
		saved_game.write<int32_t>(jumpAtkUpMove);
		saved_game.write<int32_t>(jumpAtkFwdMove);
		saved_game.write<int32_t>(jumpAtkBackMove);
		saved_game.write<int32_t>(jumpAtkRightMove);
		saved_game.write<int32_t>(jumpAtkLeftMove);
		saved_game.write<int32_t>(readyAnim);
		saved_game.write<int32_t>(drawAnim);
		saved_game.write<int32_t>(putawayAnim);
		saved_game.write<int32_t>(tauntAnim);
		saved_game.write<int32_t>(bowAnim);
		saved_game.write<int32_t>(meditateAnim);
		saved_game.write<int32_t>(flourishAnim);
		saved_game.write<int32_t>(gloatAnim);
		saved_game.write<int32_t>(bladeStyle2Start);
		saved_game.write<int32_t>(trailStyle);
		saved_game.write<int8_t>(g2MarksShader);
		saved_game.write<int8_t>(g2WeaponMarkShader);
		saved_game.write<int32_t>(hitSound);
		saved_game.write<int32_t>(blockSound);
		saved_game.write<int32_t>(bounceSound);
		saved_game.write<int32_t>(blockEffect);
		saved_game.write<int32_t>(hitPersonEffect);
		saved_game.write<int32_t>(hitOtherEffect);
		saved_game.write<int32_t>(bladeEffect);
		saved_game.write<float>(knockbackScale);
		saved_game.write<float>(damageScale);
		saved_game.write<float>(splashRadius);
		saved_game.write<int32_t>(splashDamage);
		saved_game.write<float>(splashKnockback);
		saved_game.write<int32_t>(trailStyle2);
		saved_game.write<int8_t>(g2MarksShader2);
		saved_game.write<int8_t>(g2WeaponMarkShader2);
		saved_game.write<int32_t>(hit2Sound);
		saved_game.write<int32_t>(block2Sound);
		saved_game.write<int32_t>(bounce2Sound);
		saved_game.write<int32_t>(blockEffect2);
		saved_game.write<int32_t>(hitPersonEffect2);
		saved_game.write<int32_t>(hitOtherEffect2);
		saved_game.write<int32_t>(bladeEffect2);
		saved_game.write<float>(knockbackScale2);
		saved_game.write<float>(damageScale2);
		saved_game.write<float>(splashRadius2);
		saved_game.write<int32_t>(splashDamage2);
		saved_game.write<float>(splashKnockback2);
	}

	void sg_import(
		ojk::SavedGameHelper& saved_game)
	{
		saved_game.read<int32_t>(name);
		saved_game.read<int32_t>(fullName);
		saved_game.read<int32_t>(type);
		saved_game.read<int32_t>(model);
		saved_game.read<int32_t>(skin);
		saved_game.read<int32_t>(soundOn);
		saved_game.read<int32_t>(soundLoop);
		saved_game.read<int32_t>(soundOff);
		saved_game.read<int32_t>(numBlades);
		saved_game.read<>(blade);
		saved_game.read<int32_t>(stylesLearned);
		saved_game.read<int32_t>(stylesForbidden);
		saved_game.read<int32_t>(maxChain);
		saved_game.read<int32_t>(forceRestrictions);
		saved_game.read<int32_t>(lockBonus);
		saved_game.read<int32_t>(parryBonus);
		saved_game.read<int32_t>(breakParryBonus);
		saved_game.read<int32_t>(breakParryBonus2);
		saved_game.read<int32_t>(disarmBonus);
		saved_game.read<int32_t>(disarmBonus2);
		saved_game.read<int32_t>(singleBladeStyle);
		saved_game.read<int32_t>(brokenSaber1);
		saved_game.read<int32_t>(brokenSaber2);
		saved_game.read<int32_t>(saberFlags);
		saved_game.read<int32_t>(saberFlags2);
		saved_game.read<int32_t>(spinSound);
		saved_game.read<int32_t>(swingSound);
		saved_game.read<int32_t>(fallSound);
		saved_game.read<float>(moveSpeedScale);
		saved_game.read<float>(animSpeedScale);
		saved_game.read<int32_t>(kataMove);
		saved_game.read<int32_t>(lungeAtkMove);
		saved_game.read<int32_t>(jumpAtkUpMove);
		saved_game.read<int32_t>(jumpAtkFwdMove);
		saved_game.read<int32_t>(jumpAtkBackMove);
		saved_game.read<int32_t>(jumpAtkRightMove);
		saved_game.read<int32_t>(jumpAtkLeftMove);
		saved_game.read<int32_t>(readyAnim);
		saved_game.read<int32_t>(drawAnim);
		saved_game.read<int32_t>(putawayAnim);
		saved_game.read<int32_t>(tauntAnim);
		saved_game.read<int32_t>(bowAnim);
		saved_game.read<int32_t>(meditateAnim);
		saved_game.read<int32_t>(flourishAnim);
		saved_game.read<int32_t>(gloatAnim);
		saved_game.read<int32_t>(bladeStyle2Start);
		saved_game.read<int32_t>(trailStyle);
		saved_game.read<int8_t>(g2MarksShader);
		saved_game.read<int8_t>(g2WeaponMarkShader);
		saved_game.read<int32_t>(hitSound);
		saved_game.read<int32_t>(blockSound);
		saved_game.read<int32_t>(bounceSound);
		saved_game.read<int32_t>(blockEffect);
		saved_game.read<int32_t>(hitPersonEffect);
		saved_game.read<int32_t>(hitOtherEffect);
		saved_game.read<int32_t>(bladeEffect);
		saved_game.read<float>(knockbackScale);
		saved_game.read<float>(damageScale);
		saved_game.read<float>(splashRadius);
		saved_game.read<int32_t>(splashDamage);
		saved_game.read<float>(splashKnockback);
		saved_game.read<int32_t>(trailStyle2);
		saved_game.read<int8_t>(g2MarksShader2);
		saved_game.read<int8_t>(g2WeaponMarkShader2);
		saved_game.read<int32_t>(hit2Sound);
		saved_game.read<int32_t>(block2Sound);
		saved_game.read<int32_t>(bounce2Sound);
		saved_game.read<int32_t>(blockEffect2);
		saved_game.read<int32_t>(hitPersonEffect2);
		saved_game.read<int32_t>(hitOtherEffect2);
		saved_game.read<int32_t>(bladeEffect2);
		saved_game.read<float>(knockbackScale2);
		saved_game.read<float>(damageScale2);
		saved_game.read<float>(splashRadius2);
		saved_game.read<int32_t>(splashDamage2);
		saved_game.read<float>(splashKnockback2);
	}
} saberInfo_t;

//NOTE: Below is the *retail* version of the saberInfo_t structure - it is ONLY used for loading retail-version savegames (we load the savegame into this smaller structure, then copy each field into the appropriate field in the new structure - see SG_ConvertRetailSaberinfoToNewSaberinfo()
class saberInfoRetail_t
{
public:
	char		*name;						//entry in sabers.cfg, if any
	char		*fullName;					//the "Proper Name" of the saber, shown in the UI
	saberType_t	type;						//none, single or staff
	char		*model;						//hilt model
	char		*skin;						//hilt custom skin
	int			soundOn;					//game soundindex for turning on sound
	int			soundLoop;					//game soundindex for hum/loop sound
	int			soundOff;					//game soundindex for turning off sound
	int			numBlades;
	bladeInfo_t	blade[MAX_BLADES];			//blade info - like length, trail, origin, dir, etc.
	saber_styles_t	style;					//locked style to use, if any
	int			maxChain;					//how many moves can be chained in a row with this weapon (-1 is infinite, 0 is use default behavior)
	qboolean	lockable;					//can get into a saberlock
	qboolean	throwable;					//whether or not this saber can be thrown - FIXME: maybe make this a max level of force saber throw that can be used with this saber?
	qboolean	disarmable;					//whether or not this saber can be dropped
	qboolean	activeBlocking;				//whether or not to try to block incoming shots with this saber
	qboolean	twoHanded;					//uses both hands
	int			forceRestrictions;			//force powers that cannot be used while this saber is on (bitfield) - FIXME: maybe make this a limit on the max level, per force power, that can be used with this type?
	int			lockBonus;					//in saberlocks, this type of saber pushes harder or weaker
	int			parryBonus;					//added to strength of parry with this saber
	int			breakParryBonus;			//added to strength when hit a parry
	int			disarmBonus;				//added to disarm chance when win saberlock or have a good parry (knockaway)
	saber_styles_t	singleBladeStyle;		//makes it so that you use a different style if you only have the first blade active
	qboolean	singleBladeThrowable;		//makes it so that you can throw this saber if only the first blade is on
	char		*brokenSaber1;				//if saber is actually hit by another saber, it can be cut in half/broken and will be replaced with this saber in your right hand
	char		*brokenSaber2;				//if saber is actually hit by another saber, it can be cut in half/broken and will be replaced with this saber in your left hand
	qboolean	returnDamage;				//when returning from a saber throw, it keeps spinning and doing damage
	void		Activate( void )
				{
					for ( int i = 0; i < numBlades; i++ )
					{
						blade[i].active = qtrue;
					}
				};

	void		Deactivate( void )
				{
					for ( int i = 0; i < numBlades; i++ )
					{
						blade[i].active = qfalse;
					}
				};

	// Description: Activate a specific Blade of this Saber.
	// Created: 10/03/02 by Aurelio Reis, Modified: 10/03/02 by Aurelio Reis.
	//	[in]		int iBlade		Which Blade to activate.
	//	[in]		bool bActive	Whether to activate it (default true), or deactivate it (false).
	//	[return]	void
	void		BladeActivate( int iBlade, qboolean bActive = qtrue )
				{
					// Validate blade ID/Index.
					if ( iBlade < 0 || iBlade >= numBlades )
						return;

					blade[iBlade].active = bActive;
				}

	qboolean	Active()
				{
					for ( int i = 0; i < numBlades; i++ )
					{
						if ( blade[i].active )
						{
							return qtrue;
						}
					}
					return qfalse;
				}
	void		SetLength( float length )
				{
					for ( int i = 0; i < numBlades; i++ )
					{
						blade[i].length = length;
					}
				}
	float		Length()
				{//return largest length
					float len1 = 0;
					for ( int i = 0; i < numBlades; i++ )
					{
						if ( blade[i].length > len1 )
						{
							len1 = blade[i].length;
						}
					}
					return len1;
				};
	float		LengthMax()
				{
					float len1 = 0;
					for ( int i = 0; i < numBlades; i++ )
					{
						if ( blade[i].lengthMax > len1 )
						{
							len1 = blade[i].lengthMax;
						}
					}
					return len1;
				};
	void		ActivateTrail ( float duration )
				{
					for ( int i = 0; i < numBlades; i++ )
					{
						blade[i].ActivateTrail( duration );
					}
				};
	void		DeactivateTrail ( float duration )
				{
					for ( int i = 0; i < numBlades; i++ )
					{
						blade[i].DeactivateTrail( duration );
					}
				};


	void sg_export(
		ojk::SavedGameHelper& saved_game) const
	{
		saved_game.write<int32_t>(name);
		saved_game.write<int32_t>(fullName);
		saved_game.write<int32_t>(type);
		saved_game.write<int32_t>(model);
		saved_game.write<int32_t>(skin);
		saved_game.write<int32_t>(soundOn);
		saved_game.write<int32_t>(soundLoop);
		saved_game.write<int32_t>(soundOff);
		saved_game.write<int32_t>(numBlades);
		saved_game.write<>(blade);
		saved_game.write<int32_t>(style);
		saved_game.write<int32_t>(maxChain);
		saved_game.write<int32_t>(lockable);
		saved_game.write<int32_t>(throwable);
		saved_game.write<int32_t>(disarmable);
		saved_game.write<int32_t>(activeBlocking);
		saved_game.write<int32_t>(twoHanded);
		saved_game.write<int32_t>(forceRestrictions);
		saved_game.write<int32_t>(lockBonus);
		saved_game.write<int32_t>(parryBonus);
		saved_game.write<int32_t>(breakParryBonus);
		saved_game.write<int32_t>(disarmBonus);
		saved_game.write<int32_t>(singleBladeStyle);
		saved_game.write<int32_t>(singleBladeThrowable);
		saved_game.write<int32_t>(brokenSaber1);
		saved_game.write<int32_t>(brokenSaber2);
		saved_game.write<int32_t>(returnDamage);
	}

	void sg_import(
		ojk::SavedGameHelper& saved_game)
	{
		saved_game.read<int32_t>(name);
		saved_game.read<int32_t>(fullName);
		saved_game.read<int32_t>(type);
		saved_game.read<int32_t>(model);
		saved_game.read<int32_t>(skin);
		saved_game.read<int32_t>(soundOn);
		saved_game.read<int32_t>(soundLoop);
		saved_game.read<int32_t>(soundOff);
		saved_game.read<int32_t>(numBlades);
		saved_game.read<>(blade);
		saved_game.read<int32_t>(style);
		saved_game.read<int32_t>(maxChain);
		saved_game.read<int32_t>(lockable);
		saved_game.read<int32_t>(throwable);
		saved_game.read<int32_t>(disarmable);
		saved_game.read<int32_t>(activeBlocking);
		saved_game.read<int32_t>(twoHanded);
		saved_game.read<int32_t>(forceRestrictions);
		saved_game.read<int32_t>(lockBonus);
		saved_game.read<int32_t>(parryBonus);
		saved_game.read<int32_t>(breakParryBonus);
		saved_game.read<int32_t>(disarmBonus);
		saved_game.read<int32_t>(singleBladeStyle);
		saved_game.read<int32_t>(singleBladeThrowable);
		saved_game.read<int32_t>(brokenSaber1);
		saved_game.read<int32_t>(brokenSaber2);
		saved_game.read<int32_t>(returnDamage);
	}

	void sg_export(
		saberInfo_t& dst) const;
}; // saberInfoRetail_t

#define MAX_SABERS 2	// if this ever changes then update the table "static const save_field_t savefields_gClient[]"!!!!!!!!!!!!

// playerState_t is the information needed by both the client and server
// to predict player motion and actions
// nothing outside of pmove should modify these, or some degree of prediction error
// will occur

// you can't add anything to this without modifying the code in msg.c

// playerState_t is a full superset of entityState_t as it is used by players,
// so if a playerState_t is transmitted, the entityState_t can be fully derived
// from it.
// !!!!!!!!!! LOADSAVE-affecting structure !!!!!!!!!!
template<typename TSaberInfo>
class PlayerStateBase
{
public:
	int			commandTime;		// cmd->serverTime of last executed command
	int			pm_type;
	int			bobCycle;			// for view bobbing and footstep generation
	int			pm_flags;			// ducked, jump_held, etc
	int			pm_time;

	vec3_t		origin;
	vec3_t		velocity;
	int			weaponTime;
	int			weaponChargeTime;
	int			rechargeTime;		// for the phaser
	int			gravity;
	int			leanofs;
	int			friction;
	int			speed;
	int			delta_angles[3];	// add to command angles to get view direction
									// changed by spawns, rotating objects, and teleporters

	int			groundEntityNum;// ENTITYNUM_NONE = in air
	int			legsAnim;		//
	int			legsAnimTimer;	// don't change low priority animations on legs until this runs out
	int			torsoAnim;		//
	int			torsoAnimTimer;	// don't change low priority animations on torso until this runs out
	int			movementDir;	// a number 0 to 7 that represents the relative angle
								// of movement to the view angle (axial and diagonals)
								// when at rest, the value will remain unchanged
								// used to twist the legs during strafing

	int			eFlags;			// copied to entityState_t->eFlags

	int			eventSequence;	// pmove generated events
	int			events[MAX_PS_EVENTS];
	int			eventParms[MAX_PS_EVENTS];

	int			externalEvent;	// events set on player from another source
	int			externalEventParm;
	int			externalEventTime;

	int			clientNum;		// ranges from 0 to MAX_CLIENTS-1
	int			weapon;			// copied to entityState_t->weapon
	int			weaponstate;

	int			batteryCharge;

	vec3_t		viewangles;		// for fixed views
	float		legsYaw;		// actual legs forward facing
	int			viewheight;

	// damage feedback
	int			damageEvent;							// when it changes, latch the other parms
	int			damageYaw;
	int			damagePitch;
	int			damageCount;

	int			stats[MAX_STATS];
	int			persistant[MAX_PERSISTANT];				// stats that aren't cleared on death
	int			powerups[MAX_POWERUPS];					// level.time that the powerup runs out
	int			ammo[MAX_AMMO];
	int			inventory[MAX_INVENTORY];							// Count of each inventory item.
	char  		security_key_message[MAX_SECURITY_KEYS][MAX_SECURITY_KEY_MESSSAGE];	// Security key types

	vec3_t		serverViewOrg;

	qboolean	saberInFlight;
#ifdef JK2_MODE
	qboolean	saberActive;	// -- JK2 --

	int			vehicleModel;	// -- JK2 --
	int			viewEntity;		// For overriding player movement controls and vieworg
	saber_colors_t	saberColor; // -- JK2 --
	float		saberLength;	// -- JK2 --
	float		saberLengthMax;	// -- JK2 --
	int			forcePowersActive;	//prediction needs to know this
#else
	int			viewEntity;		// For overriding player movement controls and vieworg
	int			forcePowersActive;	//prediction needs to know this

#endif

	//NEW vehicle stuff
	// This has been localized to the vehicle stuff (NOTE: We can still use it later, I'm just commenting it to
	// root out all the calls. We can store the data in vehicles and update by copying it here).
	//int			vehicleIndex;	// Index into vehicleData table
	//vec3_t		vehicleAngles;	// current angles of your vehicle
	//int			vehicleArmor;	// current armor of your vehicle (explodes if drops to 0)

	// !!
	// not communicated over the net at all
	// !!
	//int			vehicleLastFXTime;				//timer for all cgame-FX...?
	//int			vehicleExplodeTime;				//when it will go BOOM!

	int			useTime;	//not sent
	int			lastShotTime;//last time you shot your weapon
	int			ping;			// server to game info for scoreboard
	int			lastOnGround;	//last time you were on the ground
	int			lastStationary;	//last time you were on the ground
	int			weaponShotCount;

#ifndef JK2_MODE
	//FIXME: maybe allocate all these structures (saber, force powers, vehicles)
	//			or descend them as classes - so not every client has all this info
	TSaberInfo	saber[MAX_SABERS];
	qboolean	dualSabers;
	qboolean	SaberStaff( void ) { return (qboolean)( saber[0].type == SABER_STAFF || (dualSabers && saber[1].type == SABER_STAFF) ); };
	qboolean	SaberActive() { return (qboolean)( saber[0].Active() || (dualSabers&&saber[1].Active()) ); };
	void		SetSaberLength( float length )
				{
					saber[0].SetLength( length );
					if ( dualSabers )
					{
						saber[1].SetLength( length );
					}
				}
	float		SaberLength()
				{//return largest length
					float len1 = saber[0].Length();
					if ( dualSabers && saber[1].Length() > len1 )
					{
						return saber[1].Length();
					}
					return len1;
				};
	float		SaberLengthMax()
				{
					if ( saber[0].LengthMax() > saber[1].LengthMax() )
					{
						return saber[0].LengthMax();
					}
					else if ( dualSabers )
					{
						return saber[1].LengthMax();
					}
					return 0.0f;
				};

	// Activate or deactivate a specific Blade of a Saber.
	// Created: 10/03/02 by Aurelio Reis, Modified: 10/03/02 by Aurelio Reis.
	//	[in]	int iSaber		Which Saber to modify.
	//	[in]	int iBlade		Which blade to modify (0 - (NUM_BLADES - 1)).
	//	[in]	bool bActive	Whether to make it active (default true) or inactive (false).
	//	[return]	void
	void		SaberBladeActivate( int iSaber, int iBlade, qboolean bActive = qtrue )
	{
		// Validate saber (if it's greater than or equal to zero, OR it above the first saber but we
		// are not doing duel Sabers, leave, something is not right.
		if ( iSaber < 0 || ( iSaber > 0  && !dualSabers ) )
			return;

		saber[iSaber].BladeActivate( iBlade, bActive );
	}

	void		SaberActivate( void )
				{
					saber[0].Activate();
					if ( dualSabers )
					{
						saber[1].Activate();
					}
				}
	void		SaberDeactivate( void )
				{
					saber[0].Deactivate();
					saber[1].Deactivate();
				};
	void		SaberActivateTrail ( float duration )
				{
					saber[0].ActivateTrail( duration );
					if ( dualSabers )
					{
						saber[1].ActivateTrail( duration );
					}
				};
	void		SaberDeactivateTrail ( float duration )
				{
					saber[0].DeactivateTrail( duration );
					if ( dualSabers )
					{
						saber[1].DeactivateTrail( duration );
					}
				};
	int			SaberDisarmBonus( int bladeNum )
				{
					int disarmBonus = 0;
					if ( saber[0].Active() )
					{
						if ( saber[0].bladeStyle2Start > 0
							&& bladeNum >= saber[0].bladeStyle2Start )
						{
							disarmBonus += saber[0].disarmBonus2;
						}
						else
						{
							disarmBonus += saber[0].disarmBonus;
						}
					}
					if ( dualSabers && saber[1].Active() )
					{//bonus for having 2 sabers
						if ( saber[1].bladeStyle2Start > 0
							&& bladeNum >= saber[1].bladeStyle2Start )
						{
                            disarmBonus += 1 + saber[1].disarmBonus2;
						}
						else
						{
                            disarmBonus += 1 + saber[1].disarmBonus;
						}
					}
					return disarmBonus;
				};
	int			SaberParryBonus( void )
				{
					int parryBonus = 0;
					if ( saber[0].Active() )
					{
						parryBonus += saber[0].parryBonus;
					}
					if ( dualSabers && saber[1].Active() )
					{//bonus for having 2 sabers
						parryBonus += 1 + saber[1].parryBonus;
					}
					return parryBonus;
				};
#endif // !JK2_MODE

	short		saberMove;

#ifndef JK2_MODE
	short		saberMoveNext;
#endif // !JK2_MODE

	short		saberBounceMove;
	short		saberBlocking;
	short		saberBlocked;
	short		leanStopDebounceTime;

#ifdef JK2_MODE
	float		saberLengthOld;
#endif
	int			saberEntityNum;
	float		saberEntityDist;
	int			saberThrowTime;
	int			saberEntityState;
	int			saberDamageDebounceTime;
	int			saberHitWallSoundDebounceTime;
	int			saberEventFlags;
	int			saberBlockingTime;
	int			saberAnimLevel;
	int			saberAttackChainCount;
	int			saberLockTime;
	int			saberLockEnemy;

#ifndef JK2_MODE
	int			saberStylesKnown;
#endif // !JK2_MODE

#ifdef JK2_MODE
	char		*saberModel;
#endif

	int			forcePowersKnown;
	int			forcePowerDuration[NUM_FORCE_POWERS];	//for effects that have a duration
	int			forcePowerDebounce[NUM_FORCE_POWERS];	//for effects that must have an interval
	int			forcePower;
	int			forcePowerMax;
	int			forcePowerRegenDebounceTime;

#ifndef JK2_MODE
	int			forcePowerRegenRate;				//default is 100ms
	int			forcePowerRegenAmount;				//default is 1
#endif // !JK2_MODE

	int			forcePowerLevel[NUM_FORCE_POWERS];		//so we know the max forceJump power you have
	float		forceJumpZStart;					//So when you land, you don't get hurt as much
	float		forceJumpCharge;					//you're current forceJump charge-up level, increases the longer you hold the force jump button down
	int			forceGripEntityNum;					//what entity I'm gripping
	vec3_t		forceGripOrg;						//where the gripped ent should be lifted to

#ifndef JK2_MODE
	int			forceDrainEntityNum;				//what entity I'm draining
	vec3_t		forceDrainOrg;						//where the drained ent should be lifted to
#endif // !JK2_MODE

	int			forceHealCount;						//how many points of force heal have been applied so far

#ifndef JK2_MODE
	//new Jedi Academy force powers
	int			forceAllowDeactivateTime;
	int			forceRageDrainTime;
	int			forceRageRecoveryTime;
	int			forceDrainEntNum;
	float		forceDrainTime;
	int			forcePowersForced;					//client is being forced to use these powers (FIXME: and only these?)
	int			pullAttackEntNum;
	int			pullAttackTime;
	int			lastKickedEntNum;
#endif // !JK2_MODE

	int			taunting;							//replaced BUTTON_GESTURE

	float		jumpZStart;							//So when you land, you don't get hurt as much
	vec3_t		moveDir;

	float		waterheight;						//exactly what the z org of the water is (will be +4 above if under water, -4 below if not in water)
	waterHeightLevel_t	waterHeightLevel;					//how high it really is

#ifndef JK2_MODE
	//testing IK grabbing
	qboolean	ikStatus;		//for IK
	int			heldClient;		//for IK, who I'm grabbing, if anyone
	int			heldByClient;	//for IK, someone is grabbing me
	int			heldByBolt;		//for IK, what bolt I'm attached to on the holdersomeone is grabbing me by
	int			heldByBone;		//for IK, what bone I'm being held by

	//vehicle turn-around stuff... FIXME: only vehicles need this in SP...
	int			vehTurnaroundIndex;
	int			vehTurnaroundTime;

	//NOTE: not really used in SP, just for Fighter Vehicle damage stuff
	int			brokenLimbs;
	int			electrifyTime;
#endif // !JK2_MODE


	void sg_export(
		ojk::SavedGameHelper& saved_game) const
	{
		saved_game.write<int32_t>(commandTime);
		saved_game.write<int32_t>(pm_type);
		saved_game.write<int32_t>(bobCycle);
		saved_game.write<int32_t>(pm_flags);
		saved_game.write<int32_t>(pm_time);
		saved_game.write<float>(origin);
		saved_game.write<float>(velocity);
		saved_game.write<int32_t>(weaponTime);
		saved_game.write<int32_t>(weaponChargeTime);
		saved_game.write<int32_t>(rechargeTime);
		saved_game.write<int32_t>(gravity);
		saved_game.write<int32_t>(leanofs);
		saved_game.write<int32_t>(friction);
		saved_game.write<int32_t>(speed);
		saved_game.write<int32_t>(delta_angles);
		saved_game.write<int32_t>(groundEntityNum);
		saved_game.write<int32_t>(legsAnim);
		saved_game.write<int32_t>(legsAnimTimer);
		saved_game.write<int32_t>(torsoAnim);
		saved_game.write<int32_t>(torsoAnimTimer);
		saved_game.write<int32_t>(movementDir);
		saved_game.write<int32_t>(eFlags);
		saved_game.write<int32_t>(eventSequence);
		saved_game.write<int32_t>(events);
		saved_game.write<int32_t>(eventParms);
		saved_game.write<int32_t>(externalEvent);
		saved_game.write<int32_t>(externalEventParm);
		saved_game.write<int32_t>(externalEventTime);
		saved_game.write<int32_t>(clientNum);
		saved_game.write<int32_t>(weapon);
		saved_game.write<int32_t>(weaponstate);
		saved_game.write<int32_t>(batteryCharge);
		saved_game.write<float>(viewangles);
		saved_game.write<float>(legsYaw);
		saved_game.write<int32_t>(viewheight);
		saved_game.write<int32_t>(damageEvent);
		saved_game.write<int32_t>(damageYaw);
		saved_game.write<int32_t>(damagePitch);
		saved_game.write<int32_t>(damageCount);
		saved_game.write<int32_t>(stats);
		saved_game.write<int32_t>(persistant);
		saved_game.write<int32_t>(powerups);
		saved_game.write<int32_t>(ammo);
		saved_game.write<int32_t>(inventory);
		saved_game.write<int8_t>(security_key_message);
		saved_game.write<float>(serverViewOrg);
		saved_game.write<int32_t>(saberInFlight);

#ifdef JK2_MODE
		saved_game.write<int32_t>(saberActive);
		saved_game.write<int32_t>(vehicleModel);
		saved_game.write<int32_t>(viewEntity);
		saved_game.write<int32_t>(saberColor);
		saved_game.write<float>(saberLength);
		saved_game.write<float>(saberLengthMax);
		saved_game.write<int32_t>(forcePowersActive);
#else
		saved_game.write<int32_t>(viewEntity);
		saved_game.write<int32_t>(forcePowersActive);
#endif // JK2_MODE

		saved_game.write<int32_t>(useTime);
		saved_game.write<int32_t>(lastShotTime);
		saved_game.write<int32_t>(ping);
		saved_game.write<int32_t>(lastOnGround);
		saved_game.write<int32_t>(lastStationary);
		saved_game.write<int32_t>(weaponShotCount);

#ifndef JK2_MODE
		saved_game.write<>(saber);
		saved_game.write<int32_t>(dualSabers);
#endif // !JK2_MODE

		saved_game.write<int16_t>(saberMove);

#ifndef JK2_MODE
		saved_game.write<int16_t>(saberMoveNext);
#endif // !JK2_MODE

		saved_game.write<int16_t>(saberBounceMove);
		saved_game.write<int16_t>(saberBlocking);
		saved_game.write<int16_t>(saberBlocked);
		saved_game.write<int16_t>(leanStopDebounceTime);

#ifdef JK2_MODE
		saved_game.skip(2);
		saved_game.write<float>(saberLengthOld);
#endif // JK2_MODE

		saved_game.write<int32_t>(saberEntityNum);
		saved_game.write<float>(saberEntityDist);
		saved_game.write<int32_t>(saberThrowTime);
		saved_game.write<int32_t>(saberEntityState);
		saved_game.write<int32_t>(saberDamageDebounceTime);
		saved_game.write<int32_t>(saberHitWallSoundDebounceTime);
		saved_game.write<int32_t>(saberEventFlags);
		saved_game.write<int32_t>(saberBlockingTime);
		saved_game.write<int32_t>(saberAnimLevel);
		saved_game.write<int32_t>(saberAttackChainCount);
		saved_game.write<int32_t>(saberLockTime);
		saved_game.write<int32_t>(saberLockEnemy);

#ifndef JK2_MODE
		saved_game.write<int32_t>(saberStylesKnown);
#endif // !JK2_MODE

#ifdef JK2_MODE
		saved_game.write<int32_t>(saberModel);
#endif // JK2_MODE

		saved_game.write<int32_t>(forcePowersKnown);
		saved_game.write<int32_t>(forcePowerDuration);
		saved_game.write<int32_t>(forcePowerDebounce);
		saved_game.write<int32_t>(forcePower);
		saved_game.write<int32_t>(forcePowerMax);
		saved_game.write<int32_t>(forcePowerRegenDebounceTime);

#ifndef JK2_MODE
		saved_game.write<int32_t>(forcePowerRegenRate);
		saved_game.write<int32_t>(forcePowerRegenAmount);
#endif // !JK2_MODE

		saved_game.write<int32_t>(forcePowerLevel);
		saved_game.write<float>(forceJumpZStart);
		saved_game.write<float>(forceJumpCharge);
		saved_game.write<int32_t>(forceGripEntityNum);
		saved_game.write<float>(forceGripOrg);

#ifndef JK2_MODE
		saved_game.write<int32_t>(forceDrainEntityNum);
		saved_game.write<float>(forceDrainOrg);
#endif // !JK2_MODE

		saved_game.write<int32_t>(forceHealCount);

#ifndef JK2_MODE
		saved_game.write<int32_t>(forceAllowDeactivateTime);
		saved_game.write<int32_t>(forceRageDrainTime);
		saved_game.write<int32_t>(forceRageRecoveryTime);
		saved_game.write<int32_t>(forceDrainEntNum);
		saved_game.write<float>(forceDrainTime);
		saved_game.write<int32_t>(forcePowersForced);
		saved_game.write<int32_t>(pullAttackEntNum);
		saved_game.write<int32_t>(pullAttackTime);
		saved_game.write<int32_t>(lastKickedEntNum);
#endif // !JK2_MODE

		saved_game.write<int32_t>(taunting);
		saved_game.write<float>(jumpZStart);
		saved_game.write<float>(moveDir);
		saved_game.write<float>(waterheight);
		saved_game.write<int32_t>(waterHeightLevel);

#ifndef JK2_MODE
		saved_game.write<int32_t>(ikStatus);
		saved_game.write<int32_t>(heldClient);
		saved_game.write<int32_t>(heldByClient);
		saved_game.write<int32_t>(heldByBolt);
		saved_game.write<int32_t>(heldByBone);
		saved_game.write<int32_t>(vehTurnaroundIndex);
		saved_game.write<int32_t>(vehTurnaroundTime);
		saved_game.write<int32_t>(brokenLimbs);
		saved_game.write<int32_t>(electrifyTime);
#endif // !JK2_MODE
	}

	void sg_import(
		ojk::SavedGameHelper& saved_game)
	{
		saved_game.read<int32_t>(commandTime);
		saved_game.read<int32_t>(pm_type);
		saved_game.read<int32_t>(bobCycle);
		saved_game.read<int32_t>(pm_flags);
		saved_game.read<int32_t>(pm_time);
		saved_game.read<float>(origin);
		saved_game.read<float>(velocity);
		saved_game.read<int32_t>(weaponTime);
		saved_game.read<int32_t>(weaponChargeTime);
		saved_game.read<int32_t>(rechargeTime);
		saved_game.read<int32_t>(gravity);
		saved_game.read<int32_t>(leanofs);
		saved_game.read<int32_t>(friction);
		saved_game.read<int32_t>(speed);
		saved_game.read<int32_t>(delta_angles);
		saved_game.read<int32_t>(groundEntityNum);
		saved_game.read<int32_t>(legsAnim);
		saved_game.read<int32_t>(legsAnimTimer);
		saved_game.read<int32_t>(torsoAnim);
		saved_game.read<int32_t>(torsoAnimTimer);
		saved_game.read<int32_t>(movementDir);
		saved_game.read<int32_t>(eFlags);
		saved_game.read<int32_t>(eventSequence);
		saved_game.read<int32_t>(events);
		saved_game.read<int32_t>(eventParms);
		saved_game.read<int32_t>(externalEvent);
		saved_game.read<int32_t>(externalEventParm);
		saved_game.read<int32_t>(externalEventTime);
		saved_game.read<int32_t>(clientNum);
		saved_game.read<int32_t>(weapon);
		saved_game.read<int32_t>(weaponstate);
		saved_game.read<int32_t>(batteryCharge);
		saved_game.read<float>(viewangles);
		saved_game.read<float>(legsYaw);
		saved_game.read<int32_t>(viewheight);
		saved_game.read<int32_t>(damageEvent);
		saved_game.read<int32_t>(damageYaw);
		saved_game.read<int32_t>(damagePitch);
		saved_game.read<int32_t>(damageCount);
		saved_game.read<int32_t>(stats);
		saved_game.read<int32_t>(persistant);
		saved_game.read<int32_t>(powerups);
		saved_game.read<int32_t>(ammo);
		saved_game.read<int32_t>(inventory);
		saved_game.read<int8_t>(security_key_message);
		saved_game.read<float>(serverViewOrg);
		saved_game.read<int32_t>(saberInFlight);

#ifdef JK2_MODE
		saved_game.read<int32_t>(saberActive);
		saved_game.read<int32_t>(vehicleModel);
		saved_game.read<int32_t>(viewEntity);
		saved_game.read<int32_t>(saberColor);
		saved_game.read<float>(saberLength);
		saved_game.read<float>(saberLengthMax);
		saved_game.read<int32_t>(forcePowersActive);
#else
		saved_game.read<int32_t>(viewEntity);
		saved_game.read<int32_t>(forcePowersActive);
#endif // JK2_MODE

		saved_game.read<int32_t>(useTime);
		saved_game.read<int32_t>(lastShotTime);
		saved_game.read<int32_t>(ping);
		saved_game.read<int32_t>(lastOnGround);
		saved_game.read<int32_t>(lastStationary);
		saved_game.read<int32_t>(weaponShotCount);

#ifndef JK2_MODE
		saved_game.read<>(saber);
		saved_game.read<int32_t>(dualSabers);
#endif // !JK2_MODE

		saved_game.read<int16_t>(saberMove);

#ifndef JK2_MODE
		saved_game.read<int16_t>(saberMoveNext);
#endif // !JK2_MODE

		saved_game.read<int16_t>(saberBounceMove);
		saved_game.read<int16_t>(saberBlocking);
		saved_game.read<int16_t>(saberBlocked);
		saved_game.read<int16_t>(leanStopDebounceTime);

#ifdef JK2_MODE
		saved_game.skip(2);
		saved_game.read<float>(saberLengthOld);
#endif // JK2_MODE

		saved_game.read<int32_t>(saberEntityNum);
		saved_game.read<float>(saberEntityDist);
		saved_game.read<int32_t>(saberThrowTime);
		saved_game.read<int32_t>(saberEntityState);
		saved_game.read<int32_t>(saberDamageDebounceTime);
		saved_game.read<int32_t>(saberHitWallSoundDebounceTime);
		saved_game.read<int32_t>(saberEventFlags);
		saved_game.read<int32_t>(saberBlockingTime);
		saved_game.read<int32_t>(saberAnimLevel);
		saved_game.read<int32_t>(saberAttackChainCount);
		saved_game.read<int32_t>(saberLockTime);
		saved_game.read<int32_t>(saberLockEnemy);

#ifndef JK2_MODE
		saved_game.read<int32_t>(saberStylesKnown);
#endif // !JK2_MODE

#ifdef JK2_MODE
		saved_game.read<int32_t>(saberModel);
#endif // JK2_MODE

		saved_game.read<int32_t>(forcePowersKnown);
		saved_game.read<int32_t>(forcePowerDuration);
		saved_game.read<int32_t>(forcePowerDebounce);
		saved_game.read<int32_t>(forcePower);
		saved_game.read<int32_t>(forcePowerMax);
		saved_game.read<int32_t>(forcePowerRegenDebounceTime);

#ifndef JK2_MODE
		saved_game.read<int32_t>(forcePowerRegenRate);
		saved_game.read<int32_t>(forcePowerRegenAmount);
#endif // !JK2_MODE

		saved_game.read<int32_t>(forcePowerLevel);
		saved_game.read<float>(forceJumpZStart);
		saved_game.read<float>(forceJumpCharge);
		saved_game.read<int32_t>(forceGripEntityNum);
		saved_game.read<float>(forceGripOrg);

#ifndef JK2_MODE
		saved_game.read<int32_t>(forceDrainEntityNum);
		saved_game.read<float>(forceDrainOrg);
#endif // !JK2_MODE

		saved_game.read<int32_t>(forceHealCount);

#ifndef JK2_MODE
		saved_game.read<int32_t>(forceAllowDeactivateTime);
		saved_game.read<int32_t>(forceRageDrainTime);
		saved_game.read<int32_t>(forceRageRecoveryTime);
		saved_game.read<int32_t>(forceDrainEntNum);
		saved_game.read<float>(forceDrainTime);
		saved_game.read<int32_t>(forcePowersForced);
		saved_game.read<int32_t>(pullAttackEntNum);
		saved_game.read<int32_t>(pullAttackTime);
		saved_game.read<int32_t>(lastKickedEntNum);
#endif // !JK2_MODE

		saved_game.read<int32_t>(taunting);
		saved_game.read<float>(jumpZStart);
		saved_game.read<float>(moveDir);
		saved_game.read<float>(waterheight);
		saved_game.read<int32_t>(waterHeightLevel);

#ifndef JK2_MODE
		saved_game.read<int32_t>(ikStatus);
		saved_game.read<int32_t>(heldClient);
		saved_game.read<int32_t>(heldByClient);
		saved_game.read<int32_t>(heldByBolt);
		saved_game.read<int32_t>(heldByBone);
		saved_game.read<int32_t>(vehTurnaroundIndex);
		saved_game.read<int32_t>(vehTurnaroundTime);
		saved_game.read<int32_t>(brokenLimbs);
		saved_game.read<int32_t>(electrifyTime);
#endif // !JK2_MODE
	}
}; // PlayerStateBase


using playerState_t = PlayerStateBase<saberInfo_t>;
//====================================================================


//
// usercmd_t->button bits, many of which are generated by the client system,
// so they aren't game/cgame only definitions
//
#define	BUTTON_ATTACK		1
#define	BUTTON_FORCE_LIGHTNING	2			// displays talk balloon and disables actions
#define	BUTTON_USE_FORCE	4
#define	BUTTON_FORCE_DRAIN	8			// draining
#define	BUTTON_BLOCKING		8
#define	BUTTON_VEH_SPEED	8			// used for some horrible vehicle hack... :)
#define	BUTTON_WALKING		16			// walking can't just be infered from MOVE_RUN because a key pressed late in the frame will
										// only generate a small move value for that frame walking will use different animations and
										// won't generate footsteps
#define	BUTTON_USE			32			// the ol' use key returns!
#define BUTTON_FORCEGRIP	64			//
#define BUTTON_ALT_ATTACK	128

#define	BUTTON_FORCE_FOCUS	256			// any key whatsoever

#define	MOVE_RUN			120			// if forwardmove or rightmove are >= MOVE_RUN,
										// then BUTTON_WALKING should be set


typedef enum
{
	GENCMD_FORCE_HEAL = 1,
	GENCMD_FORCE_SPEED,
	GENCMD_FORCE_THROW,
	GENCMD_FORCE_PULL,
	GENCMD_FORCE_DISTRACT,
	GENCMD_FORCE_GRIP,
	GENCMD_FORCE_LIGHTNING,
	GENCMD_FORCE_RAGE,
	GENCMD_FORCE_PROTECT,
	GENCMD_FORCE_ABSORB,
	GENCMD_FORCE_DRAIN,
	GENCMD_FORCE_SEEING,
} genCmds_t;


// usercmd_t is sent to the server each client frame
// !!!!!!!!!! LOADSAVE-affecting structure !!!!!!!!!!
typedef struct usercmd_s {
	int		serverTime;
	int		buttons;
	byte	weapon;
	int		angles[3];
	byte	generic_cmd;
	signed char	forwardmove, rightmove, upmove;


	void sg_export(
		ojk::SavedGameHelper& saved_game) const
	{
		saved_game.write<int32_t>(serverTime);
		saved_game.write<int32_t>(buttons);
		saved_game.write<uint8_t>(weapon);
		saved_game.skip(3);
		saved_game.write<int32_t>(angles);
		saved_game.write<uint8_t>(generic_cmd);
		saved_game.write<int8_t>(forwardmove);
		saved_game.write<int8_t>(rightmove);
		saved_game.write<int8_t>(upmove);
	}

	void sg_import(
		ojk::SavedGameHelper& saved_game)
	{
		saved_game.read<int32_t>(serverTime);
		saved_game.read<int32_t>(buttons);
		saved_game.read<uint8_t>(weapon);
		saved_game.skip(3);
		saved_game.read<int32_t>(angles);
		saved_game.read<uint8_t>(generic_cmd);
		saved_game.read<int8_t>(forwardmove);
		saved_game.read<int8_t>(rightmove);
		saved_game.read<int8_t>(upmove);
	}
} usercmd_t;

//===================================================================

// if entityState->solid == SOLID_BMODEL, modelindex is an inline model number
#define	SOLID_BMODEL	0xffffff

typedef enum {// !!!!!!!!!!! LOADSAVE-affecting struct !!!!!!!!!!
	TR_STATIONARY,
	TR_INTERPOLATE,				// non-parametric, but interpolate between snapshots
	TR_LINEAR,
	TR_LINEAR_STOP,
	TR_NONLINEAR_STOP,
	TR_SINE,					// value = base + sin( time / duration ) * delta
	TR_GRAVITY
} trType_t;

typedef struct {// !!!!!!!!!!! LOADSAVE-affecting struct !!!!!!!!!!
	trType_t	trType;
	int		trTime;
	int		trDuration;			// if non 0, trTime + trDuration = stop time
	vec3_t	trBase;
	vec3_t	trDelta;			// velocity, etc


	void sg_export(
		ojk::SavedGameHelper& saved_game) const
	{
		saved_game.write<int32_t>(trType);
		saved_game.write<int32_t>(trTime);
		saved_game.write<int32_t>(trDuration);
		saved_game.write<float>(trBase);
		saved_game.write<float>(trDelta);
	}

	void sg_import(
		ojk::SavedGameHelper& saved_game)
	{
		saved_game.read<int32_t>(trType);
		saved_game.read<int32_t>(trTime);
		saved_game.read<int32_t>(trDuration);
		saved_game.read<float>(trBase);
		saved_game.read<float>(trDelta);
	}
} trajectory_t;


// entityState_t is the information conveyed from the server
// in an update message about entities that the client will
// need to render in some way
// Different eTypes may use the information in different ways
// The messages are delta compressed, so it doesn't really matter if
// the structure size is fairly large

typedef struct entityState_s {// !!!!!!!!!!! LOADSAVE-affecting struct !!!!!!!!!!!!!
	int		number;			// entity index
	int		eType;			// entityType_t
	int		eFlags;

	trajectory_t	pos;	// for calculating position
	trajectory_t	apos;	// for calculating angles

	int		time;
	int		time2;

	vec3_t	origin;
	vec3_t	origin2;

	vec3_t	angles;
	vec3_t	angles2;

	int		otherEntityNum;	// shotgun sources, etc
	int		otherEntityNum2;

	int		groundEntityNum;	// -1 = in air

	int		constantLight;	// r + (g<<8) + (b<<16) + (intensity<<24)
	int		loopSound;		// constantly loop this sound

	int		modelindex;
	int		modelindex2;
	int		modelindex3;
	int		clientNum;		// 0 to (MAX_CLIENTS - 1), for players and corpses
	int		frame;

	int		solid;			// for client side prediction, gi.linkentity sets this properly

	int		event;			// impulse events -- muzzle flashes, footsteps, etc
	int		eventParm;

	// for players
	int		powerups;		// bit flags
	int		weapon;			// determines weapon and flash model, etc
	int		legsAnim;		//
	int		legsAnimTimer;	// don't change low priority animations on legs until this runs out
	int		torsoAnim;		//
	int		torsoAnimTimer;	// don't change low priority animations on torso until this runs out

	int		scale;			//Scale players

	//FIXME: why did IMMERSION dupe these 2 fields here?  There's no reason for this!!!
	qboolean	saberInFlight;
	qboolean	saberActive;

#ifdef JK2_MODE
	int		vehicleModel;	// For overriding your playermodel with a drivable vehicle
#endif

#ifndef JK2_MODE
	//int		vehicleIndex;		// What kind of vehicle you're driving
	vec3_t	vehicleAngles;		//
	int		vehicleArmor;		// current armor of your vehicle (explodes if drops to 0)
	// 0 if not in a vehicle, otherwise the client number.
	int m_iVehicleNum;
#endif // !JK2_MODE

/*
Ghoul2 Insert Start
*/
	vec3_t	modelScale;		// used to scale models in any axis
	int		radius;			// used for culling all the ghoul models attached to this ent NOTE - this is automatically scaled by Ghoul2 if/when you scale the model. This is a 100% size value
	int		boltInfo;		// info used for bolting entities to Ghoul2 models - NOT used for bolting ghoul2 models to themselves, more for stuff like bolting effects to ghoul2 models
/*
Ghoul2 Insert End
*/

#ifndef JK2_MODE
	qboolean	isPortalEnt;
#endif // !JK2_MODE


	void sg_export(
		ojk::SavedGameHelper& saved_game) const
	{
		saved_game.write<int32_t>(number);
		saved_game.write<int32_t>(eType);
		saved_game.write<int32_t>(eFlags);
		saved_game.write<>(pos);
		saved_game.write<>(apos);
		saved_game.write<int32_t>(time);
		saved_game.write<int32_t>(time2);
		saved_game.write<float>(origin);
		saved_game.write<float>(origin2);
		saved_game.write<float>(angles);
		saved_game.write<float>(angles2);
		saved_game.write<int32_t>(otherEntityNum);
		saved_game.write<int32_t>(otherEntityNum2);
		saved_game.write<int32_t>(groundEntityNum);
		saved_game.write<int32_t>(constantLight);
		saved_game.write<int32_t>(loopSound);
		saved_game.write<int32_t>(modelindex);
		saved_game.write<int32_t>(modelindex2);
		saved_game.write<int32_t>(modelindex3);
		saved_game.write<int32_t>(clientNum);
		saved_game.write<int32_t>(frame);
		saved_game.write<int32_t>(solid);
		saved_game.write<int32_t>(event);
		saved_game.write<int32_t>(eventParm);
		saved_game.write<int32_t>(powerups);
		saved_game.write<int32_t>(weapon);
		saved_game.write<int32_t>(legsAnim);
		saved_game.write<int32_t>(legsAnimTimer);
		saved_game.write<int32_t>(torsoAnim);
		saved_game.write<int32_t>(torsoAnimTimer);
		saved_game.write<int32_t>(scale);
		saved_game.write<int32_t>(saberInFlight);
		saved_game.write<int32_t>(saberActive);

#ifdef JK2_MODE
		saved_game.write<int32_t>(vehicleModel);
#endif // JK2_MODE

#ifndef JK2_MODE
		saved_game.write<float>(vehicleAngles);
		saved_game.write<int32_t>(vehicleArmor);
		saved_game.write<int32_t>(m_iVehicleNum);
#endif // !JK2_MODE

		saved_game.write<float>(modelScale);
		saved_game.write<int32_t>(radius);
		saved_game.write<int32_t>(boltInfo);

#ifndef JK2_MODE
		saved_game.write<int32_t>(isPortalEnt);
#endif // !JK2_MODE
	}

	void sg_import(
		ojk::SavedGameHelper& saved_game)
	{
		saved_game.read<int32_t>(number);
		saved_game.read<int32_t>(eType);
		saved_game.read<int32_t>(eFlags);
		saved_game.read<>(pos);
		saved_game.read<>(apos);
		saved_game.read<int32_t>(time);
		saved_game.read<int32_t>(time2);
		saved_game.read<float>(origin);
		saved_game.read<float>(origin2);
		saved_game.read<float>(angles);
		saved_game.read<float>(angles2);
		saved_game.read<int32_t>(otherEntityNum);
		saved_game.read<int32_t>(otherEntityNum2);
		saved_game.read<int32_t>(groundEntityNum);
		saved_game.read<int32_t>(constantLight);
		saved_game.read<int32_t>(loopSound);
		saved_game.read<int32_t>(modelindex);
		saved_game.read<int32_t>(modelindex2);
		saved_game.read<int32_t>(modelindex3);
		saved_game.read<int32_t>(clientNum);
		saved_game.read<int32_t>(frame);
		saved_game.read<int32_t>(solid);
		saved_game.read<int32_t>(event);
		saved_game.read<int32_t>(eventParm);
		saved_game.read<int32_t>(powerups);
		saved_game.read<int32_t>(weapon);
		saved_game.read<int32_t>(legsAnim);
		saved_game.read<int32_t>(legsAnimTimer);
		saved_game.read<int32_t>(torsoAnim);
		saved_game.read<int32_t>(torsoAnimTimer);
		saved_game.read<int32_t>(scale);
		saved_game.read<int32_t>(saberInFlight);
		saved_game.read<int32_t>(saberActive);

#ifdef JK2_MODE
		saved_game.read<int32_t>(vehicleModel);
#endif // JK2_MODE

#ifndef JK2_MODE
		saved_game.read<float>(vehicleAngles);
		saved_game.read<int32_t>(vehicleArmor);
		saved_game.read<int32_t>(m_iVehicleNum);
#endif // !JK2_MODE

		saved_game.read<float>(modelScale);
		saved_game.read<int32_t>(radius);
		saved_game.read<int32_t>(boltInfo);

#ifndef JK2_MODE
		saved_game.read<int32_t>(isPortalEnt);
#endif // !JK2_MODE
	}
} entityState_t;


typedef enum {
	CA_UNINITIALIZED,
	CA_DISCONNECTED, 	// not talking to a server
	CA_CONNECTING,		// sending request packets to the server
	CA_CHALLENGING,		// sending challenge packets to the server
	CA_CONNECTED,		// netchan_t established, getting gamestate
	CA_LOADING,			// only during cgame initialization, never during main loop
	CA_PRIMED,			// got gamestate, waiting for first frame
	CA_ACTIVE,			// game views should be displayed
	CA_CINEMATIC		// playing a cinematic or a static pic, not connected to a server
} connstate_t;

typedef struct SSkinGoreData_s
{
	vec3_t			angles;
	vec3_t			position;
	int				currentTime;
	int				entNum;
	vec3_t			rayDirection;	// in world space
	vec3_t			hitLocation;	// in world space
	vec3_t			scale;
	float			SSize;			// size of splotch in the S texture direction in world units
	float			TSize;			// size of splotch in the T texture direction in world units
	float			theta;			// angle to rotate the splotch
	vec3_t			uaxis;			//mark direction
	float			depthStart;		// limit marks begin depth
	float			depthEnd;		// depth to stop making marks

	bool			useTheta;
	bool			frontFaces;
	bool			backFaces;
	bool			fadeRGB; //specify fade method to modify RGB (by default, the alpha is set instead)

	// growing stuff
	int				growDuration;			// time over which we want this to scale up, set to -1 for no scaling
	float			goreScaleStartFraction; // fraction of the final size at which we want the gore to initially appear

	//qboolean		baseModelOnly;
	int				lifeTime;				// effect expires after this amount of time
	int				firstModel;				// which model to start the gore on (can skip the first)
	int				fadeOutTime;			//specify the duration of fading, from the lifeTime (e.g. 3000 will start fading 3 seconds before removal and be faded entirely by removal)
	//int				shrinkOutTime;			// unimplemented
	//float			alphaModulate;			// unimplemented
	//vec3_t			tint;					// unimplemented
	//float			impactStrength;			// unimplemented

	int				shader; // shader handle

	int				myIndex; // used internally

} SSkinGoreData;

//rww - used for my ik stuff (ported directly from mp)
typedef struct
{
	vec3_t angles;
	vec3_t position;
	vec3_t scale;
	vec3_t velocity;
	int	me;
} sharedRagDollUpdateParams_t;

//rww - update parms for ik bone stuff
typedef struct
{
	char boneName[512]; //name of bone
	vec3_t desiredOrigin; //world coordinate that this bone should be attempting to reach
	vec3_t origin; //world coordinate of the entity who owns the g2 instance that owns the bone
	float movementSpeed; //how fast the bone should move toward the destination
} sharedIKMoveParams_t;


typedef struct
{
	vec3_t pcjMins; //ik joint limit
	vec3_t pcjMaxs; //ik joint limit
	vec3_t origin; //origin of caller
	vec3_t angles; //angles of caller
	vec3_t scale; //scale of caller
	float radius; //bone rad
	int blendTime; //bone blend time
	int pcjOverrides; //override ik bone flags
	int startFrame; //base pose start
	int endFrame; //base pose end
} sharedSetBoneIKStateParams_t;

enum sharedEIKMoveState
{
	IKS_NONE = 0,
	IKS_DYNAMIC
};

/*
========================================================================

String ID Tables

========================================================================
*/
typedef struct stringID_table_s
{
	const char	*name;
	int		id;
} stringID_table_t;

int GetIDForString ( const stringID_table_t *table, const char *string );
const char *GetStringForID( const stringID_table_t *table, int id );

// savegame screenshot size stuff...
//
#define SG_SCR_WIDTH			512	//256
#define SG_SCR_HEIGHT			512	//256
#define iSG_COMMENT_SIZE		64

#define sCVARNAME_PLAYERSAVE	"playersave"	// used for level-transition, accessed by game and server modules


/*
Ghoul2 Insert Start
*/

// For ghoul2 axis use

enum Eorientations
{
	ORIGIN = 0,
	POSITIVE_X,
	POSITIVE_Z,
	POSITIVE_Y,
	NEGATIVE_X,
	NEGATIVE_Z,
	NEGATIVE_Y
};
/*
Ghoul2 Insert End
*/

#define MAX_PARSEFILES	16
typedef struct parseData_s
{
	char	fileName[MAX_QPATH];			// Name of current file being read in
	int		com_lines;						// Number of lines read in
	int		com_tokenline;
	const char	*bufferStart;					// Start address of buffer holding data that was read in
	const char	*bufferCurrent;					// Where data is currently being parsed from buffer
} parseData_t;

//JFLCALLOUT include
//changed to array
extern parseData_t  parseData[];
extern int parseDataCount;


// cinematic states
typedef enum {
	FMV_IDLE,
	FMV_PLAY,		// play
	FMV_EOF,		// all other conditions, i.e. stop/EOF/abort
	FMV_ID_BLT,
	FMV_ID_IDLE,
	FMV_LOOPED,
	FMV_ID_WAIT
} e_status;


// define the new memory tags for the zone, used by all modules now
//
#define TAGDEF(blah) TAG_ ## blah
enum {
	#include "../qcommon/tags.h"
};
typedef unsigned memtag_t;

// stuff to help out during development process, force reloading/uncacheing of certain filetypes...
//
typedef enum
{
	eForceReload_NOTHING,
	eForceReload_BSP,
	eForceReload_MODELS,
	eForceReload_ALL

} ForceReload_e;

qboolean Q_InBitflags( const uint32_t *bits, int index, uint32_t bitsPerByte );
void Q_AddToBitflags( uint32_t *bits, int index, uint32_t bitsPerByte );
void Q_RemoveFromBitflags( uint32_t *bits, int index, uint32_t bitsPerByte );

typedef int( *cmpFunc_t )(const void *a, const void *b);

void *Q_LinearSearch( const void *key, const void *ptr, size_t count,
	size_t size, cmpFunc_t cmp );

#endif	// __Q_SHARED_H
