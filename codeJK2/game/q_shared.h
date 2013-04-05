#ifndef __Q_SHARED_H
#define __Q_SHARED_H

// q_shared.h -- included first by ALL program modules.
// A user mod should never modify this file

#ifdef _WIN32

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

#endif

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>


// this is the define for determining if we have an asm version of a C function
#if (defined _M_IX86 || defined __i386__) && !defined __sun__  && !defined __LCC__
#define id386	1
#else
#define id386	0
#endif

// for windows fastcall option

#define	QDECL

//======================= WIN32 DEFINES =================================

#ifdef WIN32

#define	MAC_STATIC

#undef QDECL
#define	QDECL	__cdecl

// buildstring will be incorporated into the version string
#ifdef NDEBUG
#ifdef _M_IX86
#define	CPUSTRING	"win-x86"
#elif defined _M_ALPHA
#define	CPUSTRING	"win-AXP"
#endif
#else
#ifdef _M_IX86
#define	CPUSTRING	"win-x86-debug"
#elif defined _M_ALPHA
#define	CPUSTRING	"win-AXP-debug"
#endif
#endif


#define	PATH_SEP '\\'

#endif

//======================= MAC OS X SERVER DEFINES =====================

#if defined(__MACH__) && defined(__APPLE__)

#define MAC_STATIC

#ifdef __ppc__
#define CPUSTRING	"MacOSXS-ppc"
#elif defined __i386__
#define CPUSTRING	"MacOSXS-i386"
#else
#define CPUSTRING	"MacOSXS-other"
#endif

#define	PATH_SEP	'/'

#define	GAME_HARD_LINKED
#define	CGAME_HARD_LINKED
#define	UI_HARD_LINKED

#endif

//======================= MAC DEFINES =================================

#ifdef __MACOS__

#define	MAC_STATIC	static

#define	CPUSTRING	"MacOS-PPC"

#define	PATH_SEP ':'

#define	GAME_HARD_LINKED
#define	CGAME_HARD_LINKED
#define	UI_HARD_LINKED

void Sys_PumpEvents( void );

#endif

//======================= LINUX DEFINES =================================

// the mac compiler can't handle >32k of locals, so we
// just waste space and make big arrays static...
#ifdef __linux__

#define	MAC_STATIC

#ifdef __i386__
#define	CPUSTRING	"linux-i386"
#elif defined __axp__
#define	CPUSTRING	"linux-alpha"
#else
#define	CPUSTRING	"linux-other"
#endif

#define	PATH_SEP '/'

#endif

//=============================================================

typedef unsigned long		ulong;
typedef unsigned short		word;

typedef unsigned char 		byte;

typedef enum {qfalse, qtrue}	qboolean;
#define	qboolean	int		//don't want strict type checking on the qboolean

typedef int		qhandle_t;
typedef int		fxHandle_t;
typedef int		sfxHandle_t;
typedef int		fileHandle_t;
typedef int		clipHandle_t;


#ifndef NULL
#define NULL ((void *)0)
#endif

#define	MAX_QINT			0x7fffffff
#define	MIN_QINT			(-MAX_QINT-1)


// angle indexes
#define	PITCH				0		// up / down
#define	YAW					1		// left / right
#define	ROLL				2		// fall over

// the game guarantees that no string from the network will ever
// exceed MAX_STRING_CHARS
#define	MAX_STRING_CHARS	1024	// max length of a string passed to Cmd_TokenizeString
#define	MAX_STRING_TOKENS	256		// max tokens resulting from Cmd_TokenizeString
#define	MAX_TOKEN_CHARS		1024	// max length of an individual token

#define	MAX_INFO_STRING		1024
#define	MAX_INFO_KEY		1024
#define	MAX_INFO_VALUE		1024


#define	MAX_QPATH			64		// max length of a quake game pathname
#define	MAX_OSPATH			128		// max length of a filesystem pathname

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
	ERR_NEED_CD					// pop up the need-cd dialog
} errorParm_t;

// font rendering values used by ui and cgame
#define PROP_GAP_WIDTH			2
//#define PROP_GAP_WIDTH			3
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
#define UI_GIANTFONT	0x00000040
#define UI_DROPSHADOW	0x00000800
#define UI_BLINK		0x00001000
#define UI_INVERSE		0x00002000
#define UI_PULSE		0x00004000
#define UI_UNDERLINE	0x00008000
#define UI_TINYFONT		0x00010000


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


typedef float vec_t;
typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];
typedef vec_t vec5_t[5];

typedef	int	fixed4_t;
typedef	int	fixed8_t;
typedef	int	fixed16_t;

#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
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

CT_MAX
} ct_table_t;

extern vec4_t colorTable[CT_MAX];


#define Q_COLOR_ESCAPE	'^'
// you MUST have the last bit on here about colour strings being less than 7 or taiwanese strings register as colour!!!!
#define Q_IsColorString(p)	( p && *(p) == Q_COLOR_ESCAPE && *((p)+1) && *((p)+1) != Q_COLOR_ESCAPE && *((p)+1) <= '7' )

#define COLOR_BLACK		'0'
#define COLOR_RED		'1'
#define COLOR_GREEN		'2'
#define COLOR_YELLOW	'3'
#define COLOR_BLUE		'4'
#define COLOR_CYAN		'5'
#define COLOR_MAGENTA	'6'
#define COLOR_WHITE		'7'
#define ColorIndex(c)	( ( (c) - '0' ) & 7 )

#define S_COLOR_BLACK	"^0"
#define S_COLOR_RED		"^1"
#define S_COLOR_GREEN	"^2"
#define S_COLOR_YELLOW	"^3"
#define S_COLOR_BLUE	"^4"
#define S_COLOR_CYAN	"^5"
#define S_COLOR_MAGENTA	"^6"
#define S_COLOR_WHITE	"^7"

extern vec4_t	g_color_table[8];

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

#define DEG2RAD( a ) ( ( (a) * M_PI ) / 180.0F )
#define RAD2DEG( a ) ( ( (a) * 180.0f ) / M_PI )

#define ENUM2STRING(arg)   #arg,arg

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

// this isn't a real cheap function to call!
int DirToByte( vec3_t dir );
void ByteToDir( int b, vec3_t dir );

#define _DotProduct(x,y)		((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2])
#define _VectorSubtract(a,b,c)	((c)[0]=(a)[0]-(b)[0],(c)[1]=(a)[1]-(b)[1],(c)[2]=(a)[2]-(b)[2])
#define _VectorAdd(a,b,c)		((c)[0]=(a)[0]+(b)[0],(c)[1]=(a)[1]+(b)[1],(c)[2]=(a)[2]+(b)[2])
#define _VectorCopy(a,b)		((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2])
#define	_VectorScale(v, s, o)	((o)[0]=(v)[0]*(s),(o)[1]=(v)[1]*(s),(o)[2]=(v)[2]*(s))
#define	_VectorMA(v, s, b, o)	((o)[0]=(v)[0]+(b)[0]*(s),(o)[1]=(v)[1]+(b)[1]*(s),(o)[2]=(v)[2]+(b)[2]*(s))


#ifdef __LCC__
#ifdef VectorCopy
#undef VectorCopy
// this is a little hack to get more efficient copies
typedef struct {
	float	v[3];
} vec3struct_t;
#define VectorCopy(a,b)	*(vec3struct_t *)b=*(vec3struct_t *)a;
#endif
#endif

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

inline void VectorSubtract( const vec3_t veca, const vec3_t vecb, vec3_t out ) {
	out[0] = veca[0]-vecb[0];
	out[1] = veca[1]-vecb[1];
	out[2] = veca[2]-vecb[2];
}

inline void VectorAdd( const vec3_t veca, const vec3_t vecb, vec3_t out ) {
	out[0] = veca[0]+vecb[0];
	out[1] = veca[1]+vecb[1];
	out[2] = veca[2]+vecb[2];
}

inline void VectorCopy( const vec3_t in, vec3_t out ) {
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

inline void VectorScale( const vec3_t in, vec_t scale, vec3_t out ) {
	out[0] = in[0]*scale;
	out[1] = in[1]*scale;
	out[2] = in[2]*scale;
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

inline void VectorRotate( vec3_t in, vec3_t matrix[3], vec3_t out )
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

	if ( length ) {
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

//  Returns a float min <= x < max (exclusive; will get max - 0.00001; but never max
inline float Q_flrand(float min, float max) {
	return ((rand() * (max - min)) / 32768.0F) + min;
}

// Returns an integer min <= x <= max (ie inclusive)
inline int Q_irand(int min, int max) {
	max++; //so it can round down
	return ((rand() * (max - min)) >> 15) + min;
}

//returns a float between 0 and 1.0
inline float random() {
	return (rand() / ((float)0x7fff));
}

//returns a float between -1 and 1.0
inline float crandom() {
	return (2.0F * (random() - 0.5F));
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

float Com_Clamp( float min, float max, float value );

char	*COM_SkipPath( char *pathname );
void	COM_StripExtension( const char *in, char *out );
void	COM_DefaultExtension( char *path, int maxSize, const char *extension );

void	 COM_BeginParseSession( void );
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

void	QDECL Com_sprintf (char *dest, int size, const char *fmt, ...);


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

// portable case insensitive compare
//inline  int Q_stricmp (const char *s1, const char *s2) {return Q_stricmpn (s1, s2, 99999);}
//int		Q_strncmp (const char *s1, const char *s2, int n);
//int		Q_stricmpn (const char *s1, const char *s2, int n);
//char	*Q_strlwr( char *s1 );
//char	*Q_strupr( char *s1 );
//char	*Q_strrchr( const char* string, int c );

// NON-portable (but faster) versions
inline int	Q_stricmp (const char *s1, const char *s2) { return stricmp(s1, s2); }
inline int	Q_strncmp (const char *s1, const char *s2, int n) { return strncmp(s1, s2, n); }
inline int	Q_stricmpn (const char *s1, const char *s2, int n) { return strnicmp(s1, s2, n); }
inline char	*Q_strlwr( char *s1 ) { return strlwr(s1); }
inline char	*Q_strupr( char *s1 ) { return strupr(s1); }
inline char	*Q_strrchr( const char* str, int c ) { return (char *)strrchr(str, c); }


// buffer size safe library replacements
void	Q_strncpyz( char *dest, const char *src, int destsize, qboolean bBarfIfTooLong=qfalse );
void	Q_strcat( char *dest, int size, const char *src );

// strlen that discounts Quake color sequences
int Q_PrintStrlen( const char *string );
// removes color sequences from string
char *Q_CleanStr( char *string );

//=============================================

#ifdef _M_IX86
//
// optimised stuff for Intel, since most of our data is in that format anyway...
//
short	BigShort(short l);
int		BigLong (int l);
float	BigFloat (float l);
#define LittleShort(l) l
#define LittleLong(l)  l
#define LittleFloat(l) l
//
#else
//
// standard smart-swap code...
//
short	BigShort(short l);
short	LittleShort(short l);
int		BigLong (int l);
int		LittleLong (int l);
float	BigFloat (float l);
float	LittleFloat (float l);
//
#endif


void	Swap_Init (void);
char	* QDECL va(const char *format, ...);

//=============================================

//
// key / value info strings
//
char *Info_ValueForKey( const char *s, const char *key );
void Info_RemoveKey( char *s, const char *key );
void Info_SetValueForKey( char *s, const char *key, const char *value );
qboolean Info_Validate( const char *s );
void Info_NextPair( const char **s, char key[MAX_INFO_KEY], char value[MAX_INFO_VALUE] );

// this is only here so the functions in q_shared.c and bg_*.c can link
void	QDECL Com_Error( int level, const char *error, ... );
void	QDECL Com_Printf( const char *msg, ... );


/*
==========================================================

CVARS (console variables)

Many variables can be used for cheating purposes, so when
cheats is zero, force all unspecified variables to their
default values.
==========================================================
*/

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
#define	CVAR_TEMP			256	// can be set even when cheats are disabled, but is not archived
#define CVAR_CHEAT			512	// can not be changed if cheats are disabled
#define CVAR_NORESTART		1024	// do not clear when a cvar_restart is issued

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
	struct cvar_s *next;
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

#include "surfaceflags.h"			// shared with the q3map utility

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
	#include "..\game\ghoul2_shared.h"	//for CGhoul2Info_v
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
#define	KEYCATCH_MESSAGE	4


// sound channels
// channel 0 never willingly overrides
// other channels will allways override a playing sound on that channel
#include "channels.h"

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
#define MAX_LOCATIONS		64

#define	GENTITYNUM_BITS		10		// don't need to send any more
#define	MAX_GENTITIES		(1<<GENTITYNUM_BITS)

// entitynums are communicated with GENTITY_BITS, so any reserved
// values thatare going to be communcated over the net need to
// also be in this range
#define	ENTITYNUM_NONE		(MAX_GENTITIES-1)
#define	ENTITYNUM_WORLD		(MAX_GENTITIES-2)
#define	ENTITYNUM_MAX_NORMAL	(MAX_GENTITIES-2)


#define	MAX_MODELS			256		// these are sent over the net as 8 bits
#define	MAX_SOUNDS			256		// so they cannot be blindly increased

#ifdef _IMMERSION
#define MAX_FORCES			96
#endif // _IMMERSION
#define	MAX_SUBMODELS		512		// nine bits

#define MAX_FX				128
#define MAX_WORLD_FX		4

/*
Ghoul2 Insert Start
*/
#define	MAX_CHARSKINS		64		// character skins
/*
Ghoul2 Insert End
*/

#define	MAX_CONFIGSTRINGS	1024

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
#define	CS_SOUNDS			(CS_MODELS+MAX_MODELS)
#ifdef _IMMERSION
#define CS_FORCES			(CS_SOUNDS+MAX_SOUNDS)
#define CS_PLAYERS			(CS_FORCES+MAX_FORCES)
#else
#define	CS_PLAYERS			(CS_SOUNDS+MAX_SOUNDS)
#endif // _IMMERSION
#define	CS_LIGHT_STYLES		(CS_PLAYERS+MAX_CLIENTS)
#define CS_EFFECTS			(CS_LIGHT_STYLES + (MAX_LIGHT_STYLES*3))
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
	NUM_FORCE_POWERS
} forcePowers_t;

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
	qboolean	saberActive;

	int			vehicleModel;	// For overriding your playermodel with a drivable vehicle
	int			viewEntity;		// For overriding player movement controls and vieworg
	saber_colors_t	saberColor;
	float		saberLength;
	float		saberLengthMax;
	int			forcePowersActive;	//prediction needs to know this

	// !!
	// not communicated over the net at all
	// !!
	int			useTime;	//not sent
	int			lastShotTime;//last time you shot your weapon
	int			ping;			// server to game info for scoreboard
	int			lastOnGround;	//last time you were on the ground
	int			lastStationary;	//last time you were on the ground
	int			weaponShotCount;

	short		saberMove;
	short		saberBounceMove;
	short		saberBlocking;
	short		saberBlocked;
	short		leanStopDebounceTime;

	float		saberLengthOld;
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
	char		*saberModel;

	int			forcePowersKnown;
	int			forcePowerDuration[NUM_FORCE_POWERS];	//for effects that have a duration
	int			forcePowerDebounce[NUM_FORCE_POWERS];	//for effects that must have an interval
	int			forcePower;
	int			forcePowerMax;
	int			forcePowerRegenDebounceTime;
	int			forcePowerLevel[NUM_FORCE_POWERS];		//so we know the max forceJump power you have
	float		forceJumpZStart;					//So when you land, you don't get hurt as much
	float		forceJumpCharge;					//you're current forceJump charge-up level, increases the longer you hold the force jump button down
	int			forceGripEntityNum;					//what entity I'm gripping
	vec3_t		forceGripOrg;						//where the gripped ent should be lifted to
	int			forceHealCount;						//how many points of force heal have been applied so far

	int			taunting;							//replaced BUTTON_GESTURE

	float		jumpZStart;							//So when you land, you don't get hurt as much
	vec3_t		moveDir;

	float		waterheight;						//exactly what the z org of the water is (will be +4 above if under water, -4 below if not in water)
	waterHeightLevel_t	waterHeightLevel;					//how high it really is
} playerState_t;


//====================================================================


//
// usercmd_t->button bits, many of which are generated by the client system,
// so they aren't game/cgame only definitions
//
#define	BUTTON_ATTACK		1
#define	BUTTON_FORCE_LIGHTNING	2			// displays talk balloon and disables actions
#define	BUTTON_USE_FORCE	4
#define	BUTTON_BLOCKING		8
#define	BUTTON_WALKING		16			// walking can't just be infered from MOVE_RUN because a key pressed late in the frame will
										// only generate a small move value for that frame walking will use different animations and
										// won't generate footsteps 
#define	BUTTON_USE			32			// the ol' use key returns!
#define BUTTON_FORCEGRIP	64			// 
#define BUTTON_ALT_ATTACK	128

#define	BUTTON_ANY			256			// any key whatsoever

#define	MOVE_RUN			120			// if forwardmove or rightmove are >= MOVE_RUN,
										// then BUTTON_WALKING should be set


// usercmd_t is sent to the server each client frame
// !!!!!!!!!! LOADSAVE-affecting structure !!!!!!!!!!
typedef struct usercmd_s {
	int		serverTime;
	int		buttons;
	byte	weapon;
	int		angles[3];
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

	qboolean	saberInFlight;
	qboolean	saberActive;

	int		vehicleModel;	// For overriding your playermodel with a drivable vehicle

/*
Ghoul2 Insert Start
*/
	vec3_t	modelScale;		// used to scale models in any axis
	int		radius;			// used for culling all the ghoul models attached to this ent NOTE - this is automatically scaled by Ghoul2 if/when you scale the model. This is a 100% size value
	int		boltInfo;		// info used for bolting entities to Ghoul2 models - NOT used for bolting ghoul2 models to themselves, more for stuff like bolting effects to ghoul2 models
/*
Ghoul2 Insert End
*/


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


/*
========================================================================

String ID Tables

========================================================================
*/
typedef struct stringID_table_s
{
	char	*name;
	int		id;
} stringID_table_t;

int GetIDForString ( const stringID_table_t *table, const char *string );
const char *GetStringForID( const stringID_table_t *table, int id );

// savegame screenshot size stuff...
//
#define SG_SCR_WIDTH			512	//256
#define SG_SCR_HEIGHT			512	//256
#define SG_SCR_DISPLAYHEIGHT	(SG_SCR_HEIGHT * 3 / 4)
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
	const char	*bufferStart;					// Start address of buffer holding data that was read in
	const char	*bufferCurrent;					// Where data is currently being parsed from buffer
} parseData_t;

extern parseData_t parseData;


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
typedef enum {
	#include "../qcommon/tags.h"
} memtag_t;

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

#ifdef _IMMERSION
#include "../ff/ff_public.h"
#endif // _IMMERSION

#endif	// __Q_SHARED_H
