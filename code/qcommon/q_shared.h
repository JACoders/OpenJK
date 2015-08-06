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

#pragma warning(disable : 4996)		// This function or variable may be unsafe.

#endif

//rww - conveniently toggle "gore" code, for model decals and stuff.
#define _G2_GORE

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

#if defined(__GNUC__)
#define NORETURN __attribute__((noreturn))
#elif defined(_MSC_VER)
#define NORETURN __declspec(noreturn)
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

short ShortSwap( short l );
int LongSwap( int l );
float FloatSwap( const float *f );


#include "../qcommon/q_platform.h"

// ================================================================
// TYPE DEFINITIONS
// ================================================================

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long ulong;

typedef enum { qfalse=0, qtrue } qboolean;
#define	qboolean	int		//don't want strict type checking on the qboolean

#define Q_min(x,y) ((x)<(y)?(x):(y))
#define Q_max(x,y) ((x)>(y)?(x):(y))

#if defined (_MSC_VER) && (_MSC_VER >= 1600)

	#include <stdint.h>

	// vsnprintf is ISO/IEC 9899:1999
	// abstracting this to make it portable
	int Q_vsnprintf( char *str, size_t size, const char *format, va_list args );

#elif defined (_MSC_VER)

	#include <io.h>

	typedef signed __int64 int64_t;
	typedef signed __int32 int32_t;
	typedef signed __int16 int16_t;
	typedef signed __int8  int8_t;
	typedef unsigned __int64 uint64_t;
	typedef unsigned __int32 uint32_t;
	typedef unsigned __int16 uint16_t;
	typedef unsigned __int8  uint8_t;

	// vsnprintf is ISO/IEC 9899:1999
	// abstracting this to make it portable
	int Q_vsnprintf( char *str, size_t size, const char *format, va_list args );
#else // not using MSVC

	#if !defined(__STDC_LIMIT_MACROS)
	#define __STDC_LIMIT_MACROS
	#endif
	#include <stdint.h>

	#define Q_vsnprintf vsnprintf

#endif

// 32 bit field aliasing
typedef union byteAlias_u {
	float f;
	int32_t i;
	uint32_t ui;
	qboolean qb;
	byte b[4];
	char c[4];
} byteAlias_t;

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

#define	MAX_QINT			0x7fffffff
#define	MIN_QINT			(-MAX_QINT-1)

#define INT_ID( a, b, c, d ) (uint32_t)((((a) & 0xff) << 24) | (((b) & 0xff) << 16) | (((c) & 0xff) << 8) | ((d) & 0xff))

// angle indexes
#define	PITCH				0		// up / down
#define	YAW					1		// left / right
#define	ROLL				2		// fall over

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


/*
==============================================================

MATHLIB

==============================================================
*/

typedef float	 vec_t;
typedef float	 vec2_t[2], vec3_t[3], vec4_t[4], vec5_t[5];
typedef int		ivec2_t[2], ivec3_t[3], ivec4_t[4], ivec5_t[5];
typedef vec3_t vec3pair_t[2], matrix3_t[3];

typedef	int	fixed4_t, fixed8_t, fixed16_t;

#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif

#if defined(_MSC_VER)
static __inline long Q_ftol(float f)
{
	return (long)f;
}
#else
static inline long Q_ftol(float f)
{
	return (long)f;
}
#endif

#define NUMVERTEXNORMALS	162
extern	vec3_t	bytedirs[NUMVERTEXNORMALS];

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

typedef enum
{
CT_NONE,
CT_BLACK,
CT_RED,
CT_GREEN,
CT_BLUE,
CT_YELLOW,
CT_MAGENTA,
CT_CYAN,
CT_WHITE,
CT_LTGREY,
CT_MDGREY,
CT_DKGREY,
CT_DKGREY2,

CT_VLTORANGE,
CT_LTORANGE,
CT_DKORANGE,
CT_VDKORANGE,

CT_VLTBLUE1,
CT_LTBLUE1,
CT_DKBLUE1,
CT_VDKBLUE1,

CT_VLTBLUE2,
CT_LTBLUE2,
CT_DKBLUE2,
CT_VDKBLUE2,

CT_VLTBROWN1,
CT_LTBROWN1,
CT_DKBROWN1,
CT_VDKBROWN1,

CT_VLTGOLD1,
CT_LTGOLD1,
CT_DKGOLD1,
CT_VDKGOLD1,

CT_VLTPURPLE1,
CT_LTPURPLE1,
CT_DKPURPLE1,
CT_VDKPURPLE1,

CT_VLTPURPLE2,
CT_LTPURPLE2,
CT_DKPURPLE2,
CT_VDKPURPLE2,

CT_VLTPURPLE3,
CT_LTPURPLE3,
CT_DKPURPLE3,
CT_VDKPURPLE3,

CT_VLTRED1,
CT_LTRED1,
CT_DKRED1,
CT_VDKRED1,
CT_VDKRED,
CT_DKRED,

CT_VLTAQUA,
CT_LTAQUA,
CT_DKAQUA,
CT_VDKAQUA,

CT_LTPINK,
CT_DKPINK,
CT_LTCYAN,
CT_DKCYAN,
CT_LTBLUE3,
CT_BLUE3,
CT_DKBLUE3,

CT_HUD_GREEN,
CT_HUD_RED,
CT_ICON_BLUE,
CT_NO_AMMO_RED,
CT_HUD_ORANGE,

CT_TITLE,

CT_MAX
} ct_table_t;

extern vec4_t colorTable[CT_MAX];

#define Q_COLOR_ESCAPE	'^'
#define Q_COLOR_BITS 0xF // was 7

// you MUST have the last bit on here about colour strings being less than 7 or taiwanese strings register as colour!!!!
#define Q_IsColorString(p)	( p && *(p) == Q_COLOR_ESCAPE && *((p)+1) && *((p)+1) != Q_COLOR_ESCAPE && *((p)+1) <= '9' && *((p)+1) >= '0' )
#define Q_IsColorStringExt(p)	((p) && *(p) == Q_COLOR_ESCAPE && *((p)+1) && *((p)+1) >= '0' && *((p)+1) <= '9') // ^[0-9]

#define COLOR_BLACK		'0'
#define COLOR_RED		'1'
#define COLOR_GREEN		'2'
#define COLOR_YELLOW	'3'
#define COLOR_BLUE		'4'
#define COLOR_CYAN		'5'
#define COLOR_MAGENTA	'6'
#define COLOR_WHITE		'7'
#define COLOR_ORANGE	'8'
#define COLOR_GREY		'9'
#define ColorIndex(c)	( ( (c) - '0' ) & Q_COLOR_BITS )

#define S_COLOR_BLACK	"^0"
#define S_COLOR_RED		"^1"
#define S_COLOR_GREEN	"^2"
#define S_COLOR_YELLOW	"^3"
#define S_COLOR_BLUE	"^4"
#define S_COLOR_CYAN	"^5"
#define S_COLOR_MAGENTA	"^6"
#define S_COLOR_WHITE	"^7"
#define S_COLOR_ORANGE	"^8"
#define S_COLOR_GREY	"^9"

extern vec4_t g_color_table[Q_COLOR_BITS+1];

#define	MAKERGB( v, r, g, b ) v[0]=r;v[1]=g;v[2]=b
#define	MAKERGBA( v, r, g, b, a ) v[0]=r;v[1]=g;v[2]=b;v[3]=a

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

#define PI_DIV_180		0.017453292519943295769236907684886
#define INV_PI_DIV_180	57.295779513082320876798154814105

// Punish Aurelio if you don't like these performance enhancements. :-)
#define DEG2RAD( a ) ( ( (a) * PI_DIV_180 ) )
#define RAD2DEG( a ) ( ( (a) * INV_PI_DIV_180 ) )

// A divide can be avoided by just multiplying by PI_DIV_180 which is PI divided by 180. - Aurelio
//#define DEG2RAD( a ) ( ( (a) * M_PI ) / 180.0F )
// A divide can be avoided by just multiplying by INV_PI_DIV_180(inverse of PI/180) which is 180 divided by PI. - Aurelio
//#define RAD2DEG( a ) ( ( (a) * 180.0f ) / M_PI )

#define ENUM2STRING(arg)   { #arg,arg }

struct cplane_s;

extern	const vec3_t	vec3_origin;
extern	const vec3_t	axisDefault[3];

#define	nanmask (255<<23)

#define	IS_NAN(x) (((*(int *)&x)&nanmask)==nanmask)

float Q_fabs( float f );
float Q_rsqrt( float f );		// reciprocal square root

#define SQRTFAST( x ) ( 1.0f / Q_rsqrt( x ) )

signed char ClampChar( int i );
signed short ClampShort( int i );

float Q_powf( float x, int y );

// this isn't a real cheap function to call!
int DirToByte( vec3_t dir );
void ByteToDir( int b, vec3_t dir );

#define _DotProduct(x,y)		((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2])
#define _VectorSubtract(a,b,c)	((c)[0]=(a)[0]-(b)[0],(c)[1]=(a)[1]-(b)[1],(c)[2]=(a)[2]-(b)[2])
#define _VectorAdd(a,b,c)		((c)[0]=(a)[0]+(b)[0],(c)[1]=(a)[1]+(b)[1],(c)[2]=(a)[2]+(b)[2])
#define _VectorCopy(a,b)		((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2])
#define	_VectorScale(v, s, o)	((o)[0]=(v)[0]*(s),(o)[1]=(v)[1]*(s),(o)[2]=(v)[2]*(s))
#define	_VectorMA(v, s, b, o)	((o)[0]=(v)[0]+(b)[0]*(s),(o)[1]=(v)[1]+(b)[1]*(s),(o)[2]=(v)[2]+(b)[2]*(s))


#define VectorClear(a)			((a)[0]=(a)[1]=(a)[2]=0)
#define VectorNegate(a,b)		((b)[0]=-(a)[0],(b)[1]=-(a)[1],(b)[2]=-(a)[2])
#define VectorSet(v, x, y, z)	((v)[0]=(x), (v)[1]=(y), (v)[2]=(z))
#define Vector4Copy(a,b)		((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2],(b)[3]=(a)[3])

#define	SnapVector(v) {v[0]=(int)v[0];v[1]=(int)v[1];v[2]=(int)v[2];}

// just in case you do't want to use the macros
inline void VectorMA( const vec3_t veca, float scale, const vec3_t vecb, vec3_t vecc) {
	vecc[0] = veca[0] + scale*vecb[0];
	vecc[1] = veca[1] + scale*vecb[1];
	vecc[2] = veca[2] + scale*vecb[2];
}

inline vec_t DotProduct( const vec3_t v1, const vec3_t v2 ) {
	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

inline void CrossProduct( const vec3_t v1, const vec3_t v2, vec3_t cross ) {
	cross[0] = v1[1]*v2[2] - v1[2]*v2[1];
	cross[1] = v1[2]*v2[0] - v1[0]*v2[2];
	cross[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

inline void VectorSubtract( const vec3_t veca, const vec3_t vecb, vec3_t o ) {
	o[0] = veca[0]-vecb[0];
	o[1] = veca[1]-vecb[1];
	o[2] = veca[2]-vecb[2];
}

inline void VectorAdd( const vec3_t veca, const vec3_t vecb, vec3_t o ) {
	o[0] = veca[0]+vecb[0];
	o[1] = veca[1]+vecb[1];
	o[2] = veca[2]+vecb[2];
}

inline void VectorCopy( const vec3_t in, vec3_t out ) {
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

inline void VectorScale( const vec3_t i, vec_t scale, vec3_t o ) {
	o[0] = i[0]*scale;
	o[1] = i[1]*scale;
	o[2] = i[2]*scale;
}

float DotProductNormalize( const vec3_t inVec1, const vec3_t inVec2 );

unsigned ColorBytes3 (float r, float g, float b);
unsigned ColorBytes4 (float r, float g, float b, float a);

float NormalizeColor( const vec3_t in, vec3_t out );
float RadiusFromBounds( const vec3_t mins, const vec3_t maxs );

void ClearBounds( vec3_t mins, vec3_t maxs );

inline void AddPointToBounds( const vec3_t v, vec3_t mins, vec3_t maxs ) {
	if ( v[0] < mins[0] ) {
		mins[0] = v[0];
	}
	if ( v[0] > maxs[0]) {
		maxs[0] = v[0];
	}

	if ( v[1] < mins[1] ) {
		mins[1] = v[1];
	}
	if ( v[1] > maxs[1]) {
		maxs[1] = v[1];
	}

	if ( v[2] < mins[2] ) {
		mins[2] = v[2];
	}
	if ( v[2] > maxs[2]) {
		maxs[2] = v[2];
	}
}

inline int VectorCompare( const vec3_t v1, const vec3_t v2 ) {
	if (v1[0] != v2[0] || v1[1] != v2[1] || v1[2] != v2[2]) {
		return 0;
	}			
	return 1;
}

//NOTE: less precise
inline int VectorCompare2( const vec3_t v1, const vec3_t v2 ) {
	if ( v1[0] > v2[0]+0.0001f || v1[0] < v2[0]-0.0001f
		|| v1[1] > v2[1]+0.0001f || v1[1] < v2[1]-0.0001f
		|| v1[2] > v2[2]+0.0001f || v1[2] < v2[2]-0.0001f ) {
		return 0;
	}			
	return 1;
}
inline vec_t VectorLength( const vec3_t v ) {
	return (vec_t)sqrt (v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

inline vec_t VectorLengthSquared( const vec3_t v ) {
	return (v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

inline vec_t Distance( const vec3_t p1, const vec3_t p2 ) {
	vec3_t	v;

	VectorSubtract (p2, p1, v);
	return VectorLength( v );
}

inline vec_t DistanceSquared( const vec3_t p1, const vec3_t p2 ) {
	vec3_t	v;

	VectorSubtract (p2, p1, v);
	return v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
}

// fast vector normalize routine that does not check to make sure
// that length != 0, nor does it return length, uses rsqrt approximation
inline void VectorNormalizeFast( vec3_t v )
{
	float ilength;

	ilength = Q_rsqrt( DotProduct( v, v ) );

	v[0] *= ilength;
	v[1] *= ilength;
	v[2] *= ilength;
}

inline void VectorInverse( vec3_t v ){
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

inline void VectorRotate( const vec3_t in, vec3_t matrix[3], vec3_t out )
{
	out[0] = DotProduct( in, matrix[0] );
	out[1] = DotProduct( in, matrix[1] );
	out[2] = DotProduct( in, matrix[2] );
}

//if length is 0, v is untouched otherwise v is normalized
inline vec_t VectorNormalize( vec3_t v ) {
	float	length, ilength;

	length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
	length = sqrt (length);

	if ( length > 0.0001f ) {
		ilength = 1/length;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}
		
	return length;
}


//if length is 0, out is cleared, otherwise out is normalized
inline vec_t VectorNormalize2( const vec3_t v, vec3_t out) {
	float	length, ilength;

	length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
	length = sqrt (length);

	if (length)
	{
		ilength = 1/length;
		out[0] = v[0]*ilength;
		out[1] = v[1]*ilength;
		out[2] = v[2]*ilength;
	} else {
		VectorClear( out );
	}
		
	return length;
}

int Q_log2(int val);

inline qboolean Q_isnan ( float f ) {
#ifdef _WIN32
	return _isnan (f);
#else
	return isnan (f);
#endif
}

inline int Q_rand( int *seed ) {
	*seed = (69069 * *seed + 1);
	return *seed;
}

inline float Q_random( int *seed ) {
	return ( Q_rand( seed ) & 0xffff ) / (float)0x10000;
}

inline float Q_crandom( int *seed ) {
	return 2.0F * ( Q_random( seed ) - 0.5f );
}

// Returns an integer min <= x <= max (ie inclusive)
inline int Q_irand(int min, int max) {
	assert(min <= max);
	return (rand() % (max - min + 1)) + min;
}

#define random() (rand() / (float)RAND_MAX)

//  Returns a float min <= x < max (exclusive; will get max - 0.00001; but never max
inline float Q_flrand(float min, float max) {
	return (random() * (max - min)) + min;
}

//returns a float between -1 and 1.0
inline float crandom() {
	return Q_flrand(-1.0f, 1.0f);
}

float erandom( float mean );

void AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);

/*
=================
AnglesToAxis
=================
*/
inline void AnglesToAxis( const vec3_t angles, vec3_t axis[3] ) {
	vec3_t	right;

	// angle vectors returns "right" instead of "y axis"
	AngleVectors( angles, axis[0], right, axis[2] );
	VectorSubtract( vec3_origin, right, axis[1] );
}

inline void AxisClear( vec3_t axis[3] ) {
	axis[0][0] = 1;
	axis[0][1] = 0;
	axis[0][2] = 0;
	axis[1][0] = 0;
	axis[1][1] = 1;
	axis[1][2] = 0;
	axis[2][0] = 0;
	axis[2][1] = 0;
	axis[2][2] = 1;
}

inline void AxisCopy( const vec3_t in[3], vec3_t out[3] ) {
	VectorCopy( in[0], out[0] );
	VectorCopy( in[1], out[1] );
	VectorCopy( in[2], out[2] );
}

void vectoangles( const vec3_t value1, vec3_t angles);

vec_t DistanceHorizontal( const vec3_t p1, const vec3_t p2 );
vec_t DistanceHorizontalSquared( const vec3_t p1, const vec3_t p2 );

inline vec_t GetYawForDirection( const vec3_t p1, const vec3_t p2 ) {
	vec3_t v, angles;

	VectorSubtract( p2, p1, v );
	vectoangles( v, angles );

	return angles[YAW];
}

inline void GetAnglesForDirection( const vec3_t p1, const vec3_t p2, vec3_t out ) {
	vec3_t v;

	VectorSubtract( p2, p1, v );
	vectoangles( v, out );
}


void SetPlaneSignbits( struct cplane_s *out );
int BoxOnPlaneSide (vec3_t emins, vec3_t emaxs, struct cplane_s *plane);
//float	AngleMod(float a);

inline float LerpAngle (float from, float to, float frac) {
	float	a;

	if ( to - from > 180 ) {
		to -= 360;
	}
	if ( to - from < -180 ) {
		to += 360;
	}
	a = from + frac * (to - from);

	return a;
}

/*
=================
AngleSubtract

Always returns a value from -180 to 180
=================
*/
inline float	AngleSubtract( float a1, float a2 ) {
	float	a;

	a = a1 - a2;
	while ( a > 180 ) {
		a -= 360;
	}
	while ( a < -180 ) {
		a += 360;
	}
	return a;
}

inline void AnglesSubtract( vec3_t v1, vec3_t v2, vec3_t v3 ) {
	v3[0] = AngleSubtract( v1[0], v2[0] );
	v3[1] = AngleSubtract( v1[1], v2[1] );
	v3[2] = AngleSubtract( v1[2], v2[2] );
}

/*
=================
AngleNormalize360

returns angle normalized to the range [0 <= angle < 360]
=================
*/
inline float AngleNormalize360 ( float angle ) {
	return (360.0 / 65536) * ((int)(angle * (65536 / 360.0)) & 65535);
}

/*
=================
AngleNormalize180

returns angle normalized to the range [-180 < angle <= 180]
=================
*/
inline float AngleNormalize180 ( float angle ) {
	angle = AngleNormalize360( angle );
	if ( angle > 180.0 ) {
		angle -= 360.0;
	}
	return angle;
}

/*
=================
AngleDelta

returns the normalized delta from angle1 to angle2
=================
*/
inline float AngleDelta ( float angle1, float angle2 ) {
	return AngleNormalize180( angle1 - angle2 );
}

qboolean PlaneFromPoints( vec4_t plane, const vec3_t a, const vec3_t b, const vec3_t c );
void ProjectPointOnPlane( vec3_t dst, const vec3_t p, const vec3_t normal );
void RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point, float degrees );
void RotateAroundDirection( vec3_t axis[3], float yaw );
void MakeNormalVectors( const vec3_t forward, vec3_t right, vec3_t up );
// perpendicular vector could be replaced by this

int	PlaneTypeForNormal (vec3_t normal);

void MatrixMultiply(float in1[3][3], float in2[3][3], float out[3][3]);
void PerpendicularVector( vec3_t dst, const vec3_t src );



//=============================================

int Com_Clampi( int min, int max, int value );
float Com_Clamp( float min, float max, float value );
int Com_AbsClampi( int min, int max, int value );
float Com_AbsClamp( float min, float max, float value );

char	*COM_SkipPath( char *pathname );
const char	*COM_GetExtension( const char *name );
void	COM_StripExtension( const char *in, char *out, int destsize );
qboolean COM_CompareExtension(const char *in, const char *ext);
void	COM_DefaultExtension( char *path, int maxSize, const char *extension );

//JLFCALLOUT include MPNOTUSED
void	 COM_BeginParseSession( void );
void	 COM_EndParseSession( void );

// For compatibility with shared code
static inline void COM_BeginParseSession( const char *sessionName )
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

int Q_isprint( int c );
int Q_islower( int c );
int Q_isupper( int c );
int Q_isalpha( int c );
qboolean Q_isanumber( const char *s );
qboolean Q_isintegral( float f );

#if 1
// portable case insensitive compare
int		Q_strncmp (const char *s1, const char *s2, int n);
int		Q_stricmpn (const char *s1, const char *s2, int n);
inline  int Q_stricmp (const char *s1, const char *s2) {return Q_stricmpn (s1, s2, 99999);}
char	*Q_strlwr( char *s1 );
char	*Q_strupr( char *s1 );
char	*Q_strrchr( const char* string, int c );
#else
// NON-portable (but faster) versions
inline int	Q_stricmp (const char *s1, const char *s2) { return stricmp(s1, s2); }
inline int	Q_strncmp (const char *s1, const char *s2, int n) { return strncmp(s1, s2, n); }
inline int	Q_stricmpn (const char *s1, const char *s2, int n) { return strnicmp(s1, s2, n); }
inline char	*Q_strlwr( char *s1 ) { return strlwr(s1); }
inline char	*Q_strupr( char *s1 ) { return strupr(s1); }
inline const char	*Q_strrchr( const char* str, int c ) { return strrchr(str, c); }
#endif


// buffer size safe library replacements
#ifdef __cplusplus
void	Q_strncpyz( char *dest, const char *src, int destsize, qboolean bBarfIfTooLong = qfalse );
#else
void	Q_strncpyz( char *dest, const char *src, int destsize, qboolean bBarfIfTooLong );
#endif
void	Q_strcat( char *dest, int size, const char *src );

const char *Q_stristr( const char *s, const char *find );

// strlen that discounts Quake color sequences
int Q_PrintStrlen( const char *string );
// removes color sequences from string
char *Q_CleanStr( char *string );
void Q_StripColor ( char *string );
void Q_strstrip( char *string, const char *strip, const char *repl );
const char *Q_strchrs( const char *string, const char *search );
//=============================================

//=============================================
/*
short	BigShort(short l);
short	LittleShort(short l);
int		BigLong (int l);
int		LittleLong (int l);
qint64  BigLong64 (qint64 l);
qint64  LittleLong64 (qint64 l);
float	BigFloat (const float *l);
float	LittleFloat (const float *l);

void	Swap_Init (void);
*/

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

// plane types are used to speed some tests
// 0-2 are axial planes
#define	PLANE_X			0
#define	PLANE_Y			1
#define	PLANE_Z			2
#define	PLANE_NON_AXIAL	3


// plane_t structure
// !!! if this is changed, it must be changed in asm code too !!!
typedef struct cplane_s {
	vec3_t	normal;
	float	dist;
	byte	type;			// for fast side tests: 0,1,2 = axial, 3 = nonaxial
	byte	signbits;		// signx + (signy<<1) + (signz<<2), used as lookup during collision
	byte	pad[2];
} cplane_t;

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
#define	MAX_SOUNDS			380

#define MAX_SUB_BSP			32

#define	MAX_SUBMODELS		512		// nine bits

#define MAX_FX				128
#define MAX_WORLD_FX		66		// was 16 // was 4

/*
Ghoul2 Insert Start
*/
#define	MAX_CHARSKINS		64		// character skins
/*
Ghoul2 Insert End
*/

#define	MAX_CONFIGSTRINGS	1300//1024 //rww - I had to up this for terrains

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

#define	CS_SKYBOXORG		(CS_MODELS+MAX_MODELS)		//rww - skybox info

#define	CS_SOUNDS			(CS_SKYBOXORG+1)
#ifdef BASE_SAVE_COMPAT
#define CS_RESERVED1		(CS_SOUNDS+MAX_SOUNDS) // reserved field for base compat from immersion removal
#define	CS_PLAYERS			(CS_RESERVED1 + 96)
#else
#define	CS_PLAYERS			(CS_SOUNDS+MAX_SOUNDS)
#endif
#define	CS_LIGHT_STYLES		(CS_PLAYERS+MAX_CLIENTS)
#define CS_TERRAINS			(CS_LIGHT_STYLES + (MAX_LIGHT_STYLES*3))
#define CS_BSP_MODELS		(CS_TERRAINS + MAX_TERRAINS)
#define CS_EFFECTS			(CS_BSP_MODELS + MAX_SUB_BSP)//(CS_LIGHT_STYLES + (MAX_LIGHT_STYLES*3))
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
	//new Jedi Academy powers
	FP_RAGE,//duration - speed, invincibility and extra damage for short period, drains your health and leaves you weak and slow afterwards.
	FP_PROTECT,//duration - protect against physical/energy (level 1 stops blaster/energy bolts, level 2 stops projectiles, level 3 protects against explosions)
	FP_ABSORB,//duration - protect against dark force powers (grip, lightning, drain - maybe push/pull, too?)
	FP_DRAIN,//hold/duration - drain force power for health
	FP_SEE,//duration - detect/see hidden enemies
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
} saberInfo_t;

//NOTE: Below is the *retail* version of the saberInfo_t structure - it is ONLY used for loading retail-version savegames (we load the savegame into this smaller structure, then copy each field into the appropriate field in the new structure - see SG_ConvertRetailSaberinfoToNewSaberinfo()
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
} saberInfoRetail_t;

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
typedef struct playerState_s {
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

	//FIXME: maybe allocate all these structures (saber, force powers, vehicles) 
	//			or descend them as classes - so not every client has all this info
	saberInfo_t	saber[MAX_SABERS];
	qboolean	dualSabers;
	qboolean	SaberStaff( void ) { return ( saber[0].type == SABER_STAFF || (dualSabers && saber[1].type == SABER_STAFF) ); };
	qboolean	SaberActive() { return ( saber[0].Active() || (dualSabers&&saber[1].Active()) ); };
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

	short		saberMove;
	short		saberMoveNext;
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
	int			saberStylesKnown;
#ifdef JK2_MODE
	char		*saberModel;
#endif

	int			forcePowersKnown;
	int			forcePowerDuration[NUM_FORCE_POWERS];	//for effects that have a duration
	int			forcePowerDebounce[NUM_FORCE_POWERS];	//for effects that must have an interval
	int			forcePower;
	int			forcePowerMax;
	int			forcePowerRegenDebounceTime;
	int			forcePowerRegenRate;				//default is 100ms
	int			forcePowerRegenAmount;				//default is 1
	int			forcePowerLevel[NUM_FORCE_POWERS];		//so we know the max forceJump power you have
	float		forceJumpZStart;					//So when you land, you don't get hurt as much
	float		forceJumpCharge;					//you're current forceJump charge-up level, increases the longer you hold the force jump button down
	int			forceGripEntityNum;					//what entity I'm gripping
	vec3_t		forceGripOrg;						//where the gripped ent should be lifted to
	int			forceDrainEntityNum;				//what entity I'm draining
	vec3_t		forceDrainOrg;						//where the drained ent should be lifted to
	int			forceHealCount;						//how many points of force heal have been applied so far
	
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

	int			taunting;							//replaced BUTTON_GESTURE

	float		jumpZStart;							//So when you land, you don't get hurt as much
	vec3_t		moveDir;

	float		waterheight;						//exactly what the z org of the water is (will be +4 above if under water, -4 below if not in water)
	waterHeightLevel_t	waterHeightLevel;					//how high it really is

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
} playerState_t;


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

	//int		vehicleIndex;		// What kind of vehicle you're driving
	vec3_t	vehicleAngles;		// 
	int		vehicleArmor;		// current armor of your vehicle (explodes if drops to 0)
	// 0 if not in a vehicle, otherwise the client number.
	int m_iVehicleNum;

/*
Ghoul2 Insert Start
*/
	vec3_t	modelScale;		// used to scale models in any axis
	int		radius;			// used for culling all the ghoul models attached to this ent NOTE - this is automatically scaled by Ghoul2 if/when you scale the model. This is a 100% size value
	int		boltInfo;		// info used for bolting entities to Ghoul2 models - NOT used for bolting ghoul2 models to themselves, more for stuff like bolting effects to ghoul2 models
/*
Ghoul2 Insert End
*/

	qboolean	isPortalEnt;

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


#include "../game/genericparser2.h"

#endif	// __Q_SHARED_H
