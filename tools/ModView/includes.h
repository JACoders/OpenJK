// Filename:-	includes.h
//

#ifndef INCLUDES_H
#define INCLUDES_H

#include <gl\gl.h>
#include <gl\glu.h>
#include <assert.h>
#include <math.h>

typedef int	ModelHandle_t;	// keep as sequential int, currently I allow 0..255 models

typedef enum {
	MOD_BAD,
	MOD_BRUSH,
	MOD_MESH,
	MOD_MD4,
	MOD_MDXM,
	MOD_MDXA
} modtype_t;


////////////////////////////////////////////////////
//
// unfortunately, there isn't one tiny file I can just get these defines from without importing 10,000 other lines
//	of crap I don't want, so for now....
//
#ifndef MAX_QPATH
#define MAX_QPATH 64
#define MAX_OSPATH MAX_PATH

#define MAX_SKIN_FILES 1000	// some high number we'll never reach (probably)


// surface geometry should not exceed these limits to be legal within Q3 engine
#define	SHADER_MAX_VERTEXES	1000
#define	SHADER_MAX_INDEXES	(6*SHADER_MAX_VERTEXES)
//
// ... but in order for ModView to work with Xmen models I need to have a higher limit...
//
#define	ACTUAL_SHADER_MAX_VERTEXES	(SHADER_MAX_VERTEXES*3)	// *3 is arbitrary, if we hit the limit, increase it.
#define	ACTUAL_SHADER_MAX_INDEXES	(SHADER_MAX_INDEXES*3)	// ""

extern bool bQ3RulesApply;
extern bool bXMenPathHack;


#define	MAX_STRING_CHARS	1024	// max length of a string passed to Cmd_TokenizeString
#define	MAX_STRING_TOKENS	256		// max tokens resulting from Cmd_TokenizeString
#define	MAX_TOKEN_CHARS		1024	// max length of an individual token

#define	MAX_INFO_STRING		1024
#define	MAX_INFO_KEY		1024
#define	MAX_INFO_VALUE		1024

#define	BIG_INFO_STRING		8192  // used for system info key only
#define	BIG_INFO_KEY		  8192
#define	BIG_INFO_VALUE		8192


#define LL(x) x	// LittleLong(x)	// no need to byteswap for now, this is only a Windoze app
#define LF(x) x	// LittleFloat(x)
#define LS(x) x	// LittleShort(x)

typedef enum {qfalse, qtrue}	qboolean;
typedef int		qhandle_t;
typedef int		TextureHandle_t;
#define	QDECL	__cdecl
// angle indexes
#define	PITCH				0		// up / down
#define	YAW					1		// left / right
#define	ROLL				2		// fall over

#ifdef DOUBLEVEC_T
typedef double vec_t;
#else
typedef float vec_t;
#endif
typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];

#endif	// #ifndef MAX_QPATH
//
////////////////////////////////////////////////////

#include "generic_stuff.h"
#include "gl_bits.h"
#include "model.h"
#include "stl.h"

#ifndef SAFEFREE
#define SAFEFREE(blah)	if (blah){free(blah);blah=NULL;}
#endif

#define ZEROMEM(blah)		memset(&blah,0,sizeof(blah))
#define ZEROMEMPTR(blah)	memset(blah,0,sizeof(*blah))





#endif	// #ifndef INCLUDES_H

////////////////////// eof /////////////////////

