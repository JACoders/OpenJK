/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "tr_local.h"

// tr_shader.c -- this file deals with the parsing and definition of shaders

static char *s_shaderText = NULL;

// the shader is parsed into these global variables, then copied into
// dynamically allocated memory if it is valid.
static	shaderStage_t	stages[MAX_SHADER_STAGES] = {{ 0 }};
static	shader_t		shader;
static	texModInfo_t	texMods[MAX_SHADER_STAGES][TR_MAX_TEXMODS];

// Hash value (generated using the generateHashValueForText function) for the original
// retail JKA shader for gfx/2d/wedge.
#define RETAIL_ROCKET_WEDGE_SHADER_HASH (1217042)

// Hash value (generated using the generateHashValueForText function) for the original
// retail JKA shader for gfx/menus/radar/arrow_w.
#define RETAIL_ARROW_W_SHADER_HASH (1650186)

#define FILE_HASH_SIZE		1024
static	shader_t* hashTable[FILE_HASH_SIZE];

#define MAX_SHADERTEXT_HASH		2048
static const char **shaderTextHashTable[MAX_SHADERTEXT_HASH] = { 0 };

const int lightmapsNone[MAXLIGHTMAPS] =
{
	LIGHTMAP_NONE,
	LIGHTMAP_NONE,
	LIGHTMAP_NONE,
	LIGHTMAP_NONE
};

const int lightmaps2d[MAXLIGHTMAPS] =
{
	LIGHTMAP_2D,
	LIGHTMAP_2D,
	LIGHTMAP_2D,
	LIGHTMAP_2D
};

const int lightmapsVertex[MAXLIGHTMAPS] =
{
	LIGHTMAP_BY_VERTEX,
	LIGHTMAP_BY_VERTEX,
	LIGHTMAP_BY_VERTEX,
	LIGHTMAP_BY_VERTEX
};

const int lightmapsFullBright[MAXLIGHTMAPS] =
{
	LIGHTMAP_WHITEIMAGE,
	LIGHTMAP_WHITEIMAGE,
	LIGHTMAP_WHITEIMAGE,
	LIGHTMAP_WHITEIMAGE
};

const byte stylesDefault[MAXLIGHTMAPS] =
{
	LS_NORMAL,
	LS_LSNONE,
	LS_LSNONE,
	LS_LSNONE
};

void setDefaultShader( void )
{
	shader.defaultShader = qtrue;
}

static uint32_t generateHashValueForText( const char *string, size_t length )
{
	int i = 0;
	uint32_t hash = 0;

	while (length--)
	{
		hash += string[i] * (i + 119);
		i++;
	}

	return (hash ^ (hash >> 10) ^ (hash >> 20));
}
/*
================
return a hash value for the filename
================
*/
static long generateHashValue( const char *fname, const int size ) {
	int		i;
	long	hash;
	char	letter;

	hash = 0;
	i = 0;
	while (fname[i] != '\0') {
		letter = tolower((unsigned char)fname[i]);
		if (letter == '.') break;				// don't include extension
		if (letter == '\\') letter = '/';		// damn path names
		if (letter == PATH_SEP) letter = '/';		// damn path names
		hash += (long)(letter) * (i + 119);
		i++;
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20));
	hash &= (size - 1);
	return hash;
}

qboolean ShaderHashTableExists( void )
{
	if (shaderTextHashTable[0])
	{
		return qtrue;
	}
	return qfalse;
}

void KillTheShaderHashTable( void )
{
	memset(shaderTextHashTable, 0, sizeof(shaderTextHashTable));
}

/*
===============
NameToAFunc
===============
*/
static unsigned NameToAFunc( const char *funcname )
{
	if (!Q_stricmp(funcname, "GT0"))
	{
		return GLS_ATEST_GT_0;
	}
	else if (!Q_stricmp(funcname, "LT128"))
	{
		return GLS_ATEST_LT_80;
	}
	else if (!Q_stricmp(funcname, "GE128"))
	{
		return GLS_ATEST_GE_80;
	}
	else if (!Q_stricmp(funcname, "GE192"))
	{
		return GLS_ATEST_GE_C0;
	}

	vk_debug("WARNING: invalid alphaFunc name '%s' in shader '%s'\n", funcname, shader.name);
	return 0;
}

/*
===============
NameToSrcBlendMode
===============
*/
static int NameToSrcBlendMode( const char *name )
{
	if (!Q_stricmp(name, "GL_ONE"))
	{
		return GLS_SRCBLEND_ONE;
	}
	else if (!Q_stricmp(name, "GL_ZERO"))
	{
		return GLS_SRCBLEND_ZERO;
	}
	else if (!Q_stricmp(name, "GL_DST_COLOR"))
	{
		return GLS_SRCBLEND_DST_COLOR;
	}
	else if (!Q_stricmp(name, "GL_ONE_MINUS_DST_COLOR"))
	{
		return GLS_SRCBLEND_ONE_MINUS_DST_COLOR;
	}
	else if (!Q_stricmp(name, "GL_SRC_ALPHA"))
	{
		return GLS_SRCBLEND_SRC_ALPHA;
	}
	else if (!Q_stricmp(name, "GL_ONE_MINUS_SRC_ALPHA"))
	{
		return GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA;
	}
	else if (!Q_stricmp(name, "GL_DST_ALPHA"))
	{
		return GLS_SRCBLEND_DST_ALPHA;
	}
	else if (!Q_stricmp(name, "GL_ONE_MINUS_DST_ALPHA"))
	{
		return GLS_SRCBLEND_ONE_MINUS_DST_ALPHA;
	}
	else if (!Q_stricmp(name, "GL_SRC_ALPHA_SATURATE"))
	{
		return GLS_SRCBLEND_ALPHA_SATURATE;
	}

	vk_debug("WARNING: unknown blend mode '%s' in shader '%s', substituting GL_ONE\n", name, shader.name);
	return GLS_SRCBLEND_ONE;
}

/*
===============
NameToDstBlendMode
===============
*/
static int NameToDstBlendMode( const char *name )
{
	if (!Q_stricmp(name, "GL_ONE"))
	{
		return GLS_DSTBLEND_ONE;
	}
	else if (!Q_stricmp(name, "GL_ZERO"))
	{
		return GLS_DSTBLEND_ZERO;
	}
	else if (!Q_stricmp(name, "GL_SRC_ALPHA"))
	{
		return GLS_DSTBLEND_SRC_ALPHA;
	}
	else if (!Q_stricmp(name, "GL_ONE_MINUS_SRC_ALPHA"))
	{
		return GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	}
	else if (!Q_stricmp(name, "GL_DST_ALPHA"))
	{
		return GLS_DSTBLEND_DST_ALPHA;
	}
	else if (!Q_stricmp(name, "GL_ONE_MINUS_DST_ALPHA"))
	{
		return GLS_DSTBLEND_ONE_MINUS_DST_ALPHA;
	}
	else if (!Q_stricmp(name, "GL_SRC_COLOR"))
	{
		return GLS_DSTBLEND_SRC_COLOR;
	}
	else if (!Q_stricmp(name, "GL_ONE_MINUS_SRC_COLOR"))
	{
		return GLS_DSTBLEND_ONE_MINUS_SRC_COLOR;
	}

	vk_debug("WARNING: unknown blend mode '%s' in shader '%s', substituting GL_ONE\n", name, shader.name);
	return GLS_DSTBLEND_ONE;
}

/*
===============
NameToGenFunc
===============
*/
static genFunc_t NameToGenFunc( const char *funcname )
{
	if (!Q_stricmp(funcname, "sin"))
	{
		return GF_SIN;
	}
	else if (!Q_stricmp(funcname, "square"))
	{
		return GF_SQUARE;
	}
	else if (!Q_stricmp(funcname, "triangle"))
	{
		return GF_TRIANGLE;
	}
	else if (!Q_stricmp(funcname, "sawtooth"))
	{
		return GF_SAWTOOTH;
	}
	else if (!Q_stricmp(funcname, "inversesawtooth"))
	{
		return GF_INVERSE_SAWTOOTH;
	}
	else if (!Q_stricmp(funcname, "noise"))
	{
		return GF_NOISE;
	}
	else if (!Q_stricmp(funcname, "random"))
	{
		return GF_RAND;

	}
	vk_debug("WARNING: invalid genfunc name '%s' in shader '%s'\n", funcname, shader.name);
	return GF_SIN;
}

// this table is also present in q3map

typedef struct infoParm_s {
	const char	*name;
	uint32_t	clearSolid, surfaceFlags, contents;
} infoParm_t;

infoParm_t	infoParms[] = {
	// Game content Flags
	{ "nonsolid",		~CONTENTS_SOLID,						SURF_NONE,			CONTENTS_NONE },		// special hack to clear solid flag
	{ "nonopaque",		~CONTENTS_OPAQUE,						SURF_NONE,			CONTENTS_NONE },		// special hack to clear opaque flag
	{ "lava",			~CONTENTS_SOLID,						SURF_NONE,			CONTENTS_LAVA },		// very damaging
	{ "slime",			~CONTENTS_SOLID,						SURF_NONE,			CONTENTS_SLIME },		// mildly damaging
	{ "water",			~CONTENTS_SOLID,						SURF_NONE,			CONTENTS_WATER },		//
	{ "fog",			~CONTENTS_SOLID,						SURF_NONE,			CONTENTS_FOG},			// carves surfaces entering
	{ "shotclip",		~CONTENTS_SOLID,						SURF_NONE,			CONTENTS_SHOTCLIP },	// block shots, but not people
	{ "playerclip",		~(CONTENTS_SOLID | CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_PLAYERCLIP },	// block only the player
	{ "monsterclip",	~(CONTENTS_SOLID | CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_MONSTERCLIP },	//
	{ "botclip",		~(CONTENTS_SOLID | CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_BOTCLIP },		// for bots
	{ "trigger",		~(CONTENTS_SOLID | CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_TRIGGER },		//
	{ "nodrop",			~(CONTENTS_SOLID | CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_NODROP },		// don't drop items or leave bodies (death fog, lava, etc)
	{ "terrain",		~(CONTENTS_SOLID | CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_TERRAIN },		// use special terrain collsion
	{ "ladder",			~(CONTENTS_SOLID | CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_LADDER },		// climb up in it like water
	{ "abseil",			~(CONTENTS_SOLID | CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_ABSEIL },		// can abseil down this brush
	{ "outside",		~(CONTENTS_SOLID | CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_OUTSIDE },		// volume is considered to be in the outside (i.e. not indoors)
	{ "inside",			~(CONTENTS_SOLID | CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_INSIDE },		// volume is considered to be inside (i.e. indoors)

	{ "detail",			CONTENTS_ALL,							SURF_NONE,			CONTENTS_DETAIL },		// don't include in structural bsp
	{ "trans",			CONTENTS_ALL,							SURF_NONE,			CONTENTS_TRANSLUCENT },	// surface has an alpha component

	/* Game surface flags */
	{ "sky",			CONTENTS_ALL,							SURF_SKY,			CONTENTS_NONE },		// emit light from an environment map
	{ "slick",			CONTENTS_ALL,							SURF_SLICK,			CONTENTS_NONE },		//

	{ "nodamage",		CONTENTS_ALL,							SURF_NODAMAGE,		CONTENTS_NONE },		//
	{ "noimpact",		CONTENTS_ALL,							SURF_NOIMPACT,		CONTENTS_NONE },		// don't make impact explosions or marks
	{ "nomarks",		CONTENTS_ALL,							SURF_NOMARKS,		CONTENTS_NONE },		// don't make impact marks, but still explode
	{ "nodraw",			CONTENTS_ALL,							SURF_NODRAW,		CONTENTS_NONE },		// don't generate a drawsurface (or a lightmap)
	{ "nosteps",		CONTENTS_ALL,							SURF_NOSTEPS,		CONTENTS_NONE },		//
	{ "nodlight",		CONTENTS_ALL,							SURF_NODLIGHT,		CONTENTS_NONE },		// don't ever add dynamic lights
	{ "metalsteps",		CONTENTS_ALL,							SURF_METALSTEPS,	CONTENTS_NONE },		//
	{ "nomiscents",		CONTENTS_ALL,							SURF_NOMISCENTS,	CONTENTS_NONE },		// No misc ents on this surface
	{ "forcefield",		CONTENTS_ALL,							SURF_FORCEFIELD,	CONTENTS_NONE },		//
	{ "forcesight",		CONTENTS_ALL,							SURF_FORCESIGHT,	CONTENTS_NONE },		// only visible with force sight
};

/*
===============
ParseSurfaceParm

surfaceparm <name>
===============
*/
static void ParseSurfaceParm( const char **text ) {
	const char	*token;
	int			numInfoParms = sizeof(infoParms) / sizeof(infoParms[0]);
	int			i;

	token = COM_ParseExt(text, qfalse);
	for (i = 0; i < numInfoParms; i++) {
		if (!Q_stricmp(token, infoParms[i].name)) {
			shader.surfaceFlags |= infoParms[i].surfaceFlags;
			shader.contentFlags |= infoParms[i].contents;
			shader.contentFlags &= infoParms[i].clearSolid;
			break;
		}
	}
}

/*
=================
ParseMaterial
=================
*/
const char *materialNames[MATERIAL_LAST] =
{
	MATERIALS
};

void ParseMaterial( const char **text )
{
	char	*token;
	int		i;

	token = COM_ParseExt(text, qfalse);
	if (token[0] == 0)
	{
		vk_debug("WARNING: missing material in shader '%s'\n", shader.name);
		return;
	}
	for (i = 0; i < MATERIAL_LAST; i++)
	{
		if (!Q_stricmp(token, materialNames[i]))
		{
			shader.surfaceFlags |= i;
			break;
		}
	}
}

/*
/////===== Part of the VERTIGON system =====/////
===================
ParseSurfaceSprites
===================
*/
// surfaceSprites <type> <width> <height> <density> <fadedist>
//
// NOTE:  This parsing function used to be 12 pages long and very complex.  The new version of surfacesprites
// utilizes optional parameters parsed in ParseSurfaceSpriteOptional.
static bool ParseSurfaceSprites( const char *_text, shaderStage_t *stage )
{
	const char *token;
	const char **text = &_text;
	float width, height, density, fadedist;
	int sstype = SURFSPRITE_NONE;

	//
	// spritetype
	//
	token = COM_ParseExt(text, qfalse);

	if (token[0] == 0)
	{
		vk_debug("WARNING: missing surfaceSprites params in shader '%s'\n", shader.name);
		return false;
	}

	if (!Q_stricmp(token, "vertical"))
	{
		sstype = SURFSPRITE_VERTICAL;
	}
	else if (!Q_stricmp(token, "oriented"))
	{
		sstype = SURFSPRITE_ORIENTED;
	}
	else if (!Q_stricmp(token, "effect"))
	{
		sstype = SURFSPRITE_EFFECT;
	}
	else if (!Q_stricmp(token, "flattened"))
	{
		sstype = SURFSPRITE_FLATTENED;
	}
	else
	{
		vk_debug("WARNING: invalid type in shader '%s'\n", shader.name);
		return false;
	}

	//
	// width
	//
	token = COM_ParseExt(text, qfalse);
	if (token[0] == 0)
	{
		vk_debug("WARNING: missing surfaceSprites params in shader '%s'\n", shader.name);
		return false;
	}
	width = atof(token);
	if (width <= 0)
	{
		vk_debug("WARNING: invalid width in shader '%s'\n", shader.name);
		return false;
	}

	//
	// height
	//
	token = COM_ParseExt(text, qfalse);
	if (token[0] == 0)
	{
		vk_debug("WARNING: missing surfaceSprites params in shader '%s'\n", shader.name);
		return false;
	}
	height = atof(token);
	if (height <= 0)
	{
		vk_debug("WARNING: invalid height in shader '%s'\n", shader.name);
		return false;
	}

	//
	// density
	//
	token = COM_ParseExt(text, qfalse);
	if (token[0] == 0)
	{
		vk_debug("WARNING: missing surfaceSprites params in shader '%s'\n", shader.name);
		return false;
	}
	density = atof(token);
	if (density <= 0)
	{
		vk_debug("WARNING: invalid density in shader '%s'\n", shader.name);
		return false;
	}

	//
	// fadedist
	//
	token = COM_ParseExt(text, qfalse);
	if (token[0] == 0)
	{
		vk_debug("WARNING: missing surfaceSprites params in shader '%s'\n", shader.name);
		return false;
	}
	fadedist = atof(token);
	if (fadedist < 32)
	{
		vk_debug("WARNING: invalid fadedist (%f < 32) in shader '%s'\n", fadedist, shader.name);
		return false;
	}

	if (!stage->ss)
	{
		stage->ss = (surfaceSprite_t*)Hunk_Alloc(sizeof(surfaceSprite_t), h_low);
	}

	// These are all set by the command lines.
	stage->ss->type = sstype;
	stage->ss->width = width;
	stage->ss->height = height;
	stage->ss->density = density;
	stage->ss->fadeDist = fadedist;

	// These are defaults that can be overwritten.
	stage->ss->fadeMax = fadedist * 1.33;
	stage->ss->fadeScale = 0.0;
	stage->ss->wind = 0.0;
	stage->ss->windIdle = 0.0;
	stage->ss->variance[0] = 0.0;
	stage->ss->variance[1] = 0.0;
	stage->ss->facing = SURFSPRITE_FACING_NORMAL;

	// A vertical parameter that needs a default regardless
	stage->ss->vertSkew = 0.0f;

	// These are effect parameters that need defaults nonetheless.
	stage->ss->fxDuration = 1000;		// 1 second
	stage->ss->fxGrow[0] = 0.0;
	stage->ss->fxGrow[1] = 0.0;
	stage->ss->fxAlphaStart = 1.0;
	stage->ss->fxAlphaEnd = 0.0;

	return true;
}

/*
/////===== Part of the VERTIGON system =====/////
===========================
ParseSurfaceSpritesOptional
===========================
*/
//
// ssFademax <fademax>
// ssFadescale <fadescale>
// ssVariance <varwidth> <varheight>
// ssHangdown
// ssAnyangle
// ssFaceup
// ssWind <wind>
// ssWindIdle <windidle>
// ssVertSkew <skew>
// ssFXDuration <duration>
// ssFXGrow <growwidth> <growheight>
// ssFXAlphaRange <alphastart> <startend>
// ssFXWeather
//
// Optional parameters that will override the defaults set in the surfacesprites command above.
//
static bool ParseSurfaceSpritesOptional( const char *param, const char *_text, shaderStage_t *stage )
{
	const char *token;
	const char **text = &_text;
	float	value;

	if (!stage->ss)
	{
		stage->ss = (surfaceSprite_t*)Hunk_Alloc(sizeof(surfaceSprite_t), h_low);
	}
	//
	// fademax
	//
	if (!Q_stricmp(param, "ssFademax"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing surfacesprite fademax in shader '%s'\n", shader.name);
			return false;
		}
		value = atof(token);
		if (value <= stage->ss->fadeDist)
		{
			vk_debug("WARNING: invalid surfacesprite fademax (%.2f <= fadeDist(%.2f)) in shader '%s'\n", value, stage->ss->fadeDist, shader.name);
			return false;
		}
		stage->ss->fadeMax = value;
		return true;
	}

	//
	// fadescale
	//
	if (!Q_stricmp(param, "ssFadescale"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing surfacesprite fadescale in shader '%s'\n", shader.name);
			return false;
		}
		value = atof(token);
		stage->ss->fadeScale = value;
		return true;
	}

	//
	// variance
	//
	if (!Q_stricmp(param, "ssVariance"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing surfacesprite variance width in shader '%s'\n", shader.name);
			return false;
		}
		value = atof(token);
		if (value < 0)
		{
			vk_debug("WARNING: invalid surfacesprite variance width in shader '%s'\n", shader.name);
			return false;
		}
		stage->ss->variance[0] = value;

		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing surfacesprite variance height in shader '%s'\n", shader.name);
			return false;
		}
		value = atof(token);
		if (value < 0)
		{
			vk_debug("WARNING: invalid surfacesprite variance height in shader '%s'\n", shader.name);
			return false;
		}
		stage->ss->variance[1] = value;
		return true;
	}

	//
	// hangdown
	//
	if (!Q_stricmp(param, "ssHangdown"))
	{
		if (stage->ss->facing != SURFSPRITE_FACING_NORMAL)
		{
			vk_debug("WARNING: Hangdown facing overrides previous facing in shader '%s'\n", shader.name);
			return false;
		}
		stage->ss->facing = SURFSPRITE_FACING_DOWN;
		return true;
	}

	//
	// anyangle
	//
	if (!Q_stricmp(param, "ssAnyangle"))
	{
		if (stage->ss->facing != SURFSPRITE_FACING_NORMAL)
		{
			vk_debug("WARNING: Anyangle facing overrides previous facing in shader '%s'\n", shader.name);
			return false;
		}
		stage->ss->facing = SURFSPRITE_FACING_ANY;
		return true;
	}

	//
	// faceup
	//
	if (!Q_stricmp(param, "ssFaceup"))
	{
		if (stage->ss->facing != SURFSPRITE_FACING_NORMAL)
		{
			vk_debug("WARNING: Faceup facing overrides previous facing in shader '%s'\n", shader.name);
			return false;
		}
		stage->ss->facing = SURFSPRITE_FACING_UP;
		return true;
	}

	//
	// wind
	//
	if (!Q_stricmp(param, "ssWind"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing surfacesprite wind in shader '%s'\n", shader.name);
			return false;
		}
		value = atof(token);
		if (value < 0.0)
		{
			vk_debug("WARNING: invalid surfacesprite wind in shader '%s'\n", shader.name);
			return false;
		}
		stage->ss->wind = value;
		if (stage->ss->windIdle <= 0)
		{	// Also override the windidle, it usually is the same as wind
			stage->ss->windIdle = value;
		}
		return true;
	}

	//
	// windidle
	//
	if (!Q_stricmp(param, "ssWindidle"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing surfacesprite windidle in shader '%s'\n", shader.name);
			return false;
		}
		value = atof(token);
		if (value < 0.0)
		{
			vk_debug("WARNING: invalid surfacesprite windidle in shader '%s'\n", shader.name);
			return false;
		}
		stage->ss->windIdle = value;
		return true;
	}

	//
	// vertskew
	//
	if (!Q_stricmp(param, "ssVertskew"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing surfacesprite vertskew in shader '%s'\n", shader.name);
			return false;
		}
		value = atof(token);
		if (value < 0.0)
		{
			vk_debug("WARNING: invalid surfacesprite vertskew in shader '%s'\n", shader.name);
			return false;
		}
		stage->ss->vertSkew = value;
		return true;
	}

	//
	// fxduration
	//
	if (!Q_stricmp(param, "ssFXDuration"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing surfacesprite duration in shader '%s'\n", shader.name);
			return false;
		}
		value = atof(token);
		if (value <= 0)
		{
			vk_debug("WARNING: invalid surfacesprite duration in shader '%s'\n", shader.name);
			return false;
		}
		stage->ss->fxDuration = value;
		return true;
	}

	//
	// fxgrow
	//
	if (!Q_stricmp(param, "ssFXGrow"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing surfacesprite grow width in shader '%s'\n", shader.name);
			return false;
		}
		value = atof(token);
		if (value < 0)
		{
			vk_debug("WARNING: invalid surfacesprite grow width in shader '%s'\n", shader.name);
			return false;
		}
		stage->ss->fxGrow[0] = value;

		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing surfacesprite grow height in shader '%s'\n", shader.name);
			return false;
		}
		value = atof(token);
		if (value < 0)
		{
			vk_debug("WARNING: invalid surfacesprite grow height in shader '%s'\n", shader.name);
			return false;
		}
		stage->ss->fxGrow[1] = value;
		return true;
	}

	//
	// fxalpharange
	//
	if (!Q_stricmp(param, "ssFXAlphaRange"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing surfacesprite fxalpha start in shader '%s'\n", shader.name);
			return false;
		}
		value = atof(token);
		if (value < 0 || value > 1.0)
		{
			vk_debug("WARNING: invalid surfacesprite fxalpha start in shader '%s'\n", shader.name);
			return false;
		}
		stage->ss->fxAlphaStart = value;

		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing surfacesprite fxalpha end in shader '%s'\n", shader.name);
			return false;
		}
		value = atof(token);
		if (value < 0 || value > 1.0)
		{
			vk_debug("WARNING: invalid surfacesprite fxalpha end in shader '%s'\n", shader.name);
			return false;
		}
		stage->ss->fxAlphaEnd = value;
		return true;
	}

	//
	// fxweather
	//
	if (!Q_stricmp(param, "ssFXWeather"))
	{
		if (stage->ss->type != SURFSPRITE_EFFECT)
		{
			vk_debug("WARNING: weather applied to non-effect surfacesprite in shader '%s'\n", shader.name);
			return false;
		}
		stage->ss->type = SURFSPRITE_WEATHERFX;
		return true;
	}

	//
	// invalid ss command.
	//
	vk_debug("WARNING: invalid optional surfacesprite param '%s' in shader '%s'\n", param, shader.name);
	return false;
}

/*
===============
ParseVector
===============
*/
qboolean ParseVector( const char **text, int count, float *v ) {
	const char	*token;
	int			i;

	// FIXME: spaces are currently required after parens, should change parseext...
	token = COM_ParseExt(text, qfalse);
	if (strcmp(token, "(")) {
		vk_debug("WARNING: missing parenthesis in shader '%s'\n", shader.name);
		return qfalse;
	}

	for (i = 0; i < count; i++) {
		token = COM_ParseExt(text, qfalse);
		if (!token[0]) {
			vk_debug("WARNING: missing vector element in shader '%s'\n", shader.name);
			return qfalse;
		}
		v[i] = atof(token);
	}

	token = COM_ParseExt(text, qfalse);
	if (strcmp(token, ")")) {
		vk_debug("WARNING: missing parenthesis in shader '%s'\n", shader.name);
		return qfalse;
	}

	return qtrue;
}

/*
===================
ParseTexMod
===================
*/
static void ParseTexMod( char *_text, shaderStage_t *stage )
{
	const char		*token;
	char			**text = &_text;
	texModInfo_t	*tmi;

	if (stage->bundle[0].numTexMods == TR_MAX_TEXMODS) {
		vk_debug("ERROR: too many tcMod stages in shader '%s'\n", shader.name);
		return;
	}

	tmi = &stage->bundle[0].texMods[stage->bundle[0].numTexMods];
	stage->bundle[0].numTexMods++;

	token = COM_ParseExt((const char**)text, qfalse);

	//
	// turb
	//
	if (!Q_stricmp(token, "turb"))
	{
		token = COM_ParseExt((const char**)text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing tcMod turb parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.base = atof(token);
		token = COM_ParseExt((const char**)text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing tcMod turb in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.amplitude = atof(token);
		token = COM_ParseExt((const char**)text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing tcMod turb in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.phase = atof(token);
		token = COM_ParseExt((const char**)text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing tcMod turb in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.frequency = atof(token);

		tmi->type = TMOD_TURBULENT;
	}
	//
	// scale
	//
	else if (!Q_stricmp(token, "scale"))
	{
		token = COM_ParseExt((const char**)text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing scale parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->translate[0] = atof(token);

		token = COM_ParseExt((const char**)text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing scale parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->translate[1] = atof(token);
		tmi->type = TMOD_SCALE;
	}
	//
	// scroll
	//
	else if (!Q_stricmp(token, "scroll"))
	{
		token = COM_ParseExt((const char**)text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing scale scroll parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->translate[0] = atof(token);
		token = COM_ParseExt((const char**)text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing scale scroll parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->translate[1] = atof(token);
		tmi->type = TMOD_SCROLL;
	}
	//
	// stretch
	//
	else if (!Q_stricmp(token, "stretch"))
	{
		token = COM_ParseExt((const char**)text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing stretch parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.func = NameToGenFunc(token);

		token = COM_ParseExt((const char**)text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing stretch parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.base = atof(token);

		token = COM_ParseExt((const char**)text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing stretch parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.amplitude = atof(token);

		token = COM_ParseExt((const char**)text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing stretch parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.phase = atof(token);

		token = COM_ParseExt((const char**)text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing stretch parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.frequency = atof(token);

		tmi->type = TMOD_STRETCH;
	}
	//
	// transform
	//
	else if (!Q_stricmp(token, "transform"))
	{
		token = COM_ParseExt((const char**)text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing transform parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->matrix[0][0] = atof(token);

		token = COM_ParseExt((const char**)text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing transform parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->matrix[0][1] = atof(token);

		token = COM_ParseExt((const char**)text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing transform parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->matrix[1][0] = atof(token);

		token = COM_ParseExt((const char**)text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing transform parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->matrix[1][1] = atof(token);

		token = COM_ParseExt((const char**)text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing transform parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->translate[0] = atof(token);

		token = COM_ParseExt((const char**)text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing transform parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->translate[1] = atof(token);

		tmi->type = TMOD_TRANSFORM;
	}
	//
	// rotate
	//
	else if (!Q_stricmp(token, "rotate"))
	{
		token = COM_ParseExt((const char**)text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing tcMod rotate parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->translate[0] = atof(token);
		tmi->type = TMOD_ROTATE;
	}
	//
	// entityTranslate
	//
	else if (!Q_stricmp(token, "entityTranslate"))
	{
		tmi->type = TMOD_ENTITY_TRANSLATE;
	}
	else
	{
		vk_debug("WARNING: unknown tcMod '%s' in shader '%s'\n", token, shader.name);
	}
}

/*
===================
ParseWaveForm
===================
*/
static void ParseWaveForm( const char **text, waveForm_t *wave )
{
	const char *token;

	token = COM_ParseExt(text, qfalse);
	if (token[0] == 0)
	{
		vk_debug("WARNING: missing waveform parm in shader '%s'\n", shader.name);
		return;
	}
	wave->func = NameToGenFunc(token);

	// BASE, AMP, PHASE, FREQ
	token = COM_ParseExt(text, qfalse);
	if (token[0] == 0)
	{
		vk_debug("WARNING: missing waveform parm in shader '%s'\n", shader.name);
		return;
	}
	wave->base = atof(token);

	token = COM_ParseExt(text, qfalse);
	if (token[0] == 0)
	{
		vk_debug("WARNING: missing waveform parm in shader '%s'\n", shader.name);
		return;
	}
	wave->amplitude = atof(token);

	token = COM_ParseExt(text, qfalse);
	if (token[0] == 0)
	{
		vk_debug("WARNING: missing waveform parm in shader '%s'\n", shader.name);
		return;
	}
	wave->phase = atof(token);

	token = COM_ParseExt(text, qfalse);
	if (token[0] == 0)
	{
		vk_debug("WARNING: missing waveform parm in shader '%s'\n", shader.name);
		return;
	}
	wave->frequency = atof(token);
}

/*
===================
ParseStage
===================
*/
static qboolean ParseStage(shaderStage_t *stage, const char **text)
{
	const char *token;
	int depthMaskBits = GLS_DEPTHMASK_TRUE, blendSrcBits = 0, blendDstBits = 0, atestBits = 0, depthFuncBits = 0;
	qboolean depthMaskExplicit = qfalse;

	stage->active = qtrue;

	while (1)
	{
		token = COM_ParseExt(text, qtrue);
		if (!token[0])
		{
			vk_debug("WARNING: no matching '}' found\n");
			return qfalse;
		}

		if (token[0] == '}')
		{
			break;
		}
		//
		// map <name>
		//
		else if (!Q_stricmp(token, "map"))
		{
			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				vk_debug("WARNING: missing parameter for 'map' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}

			if (!Q_stricmp(token, "$whiteimage"))
			{
				stage->bundle[0].image[0] = tr.whiteImage;
				continue;
			}
			else if (!Q_stricmp(token, "$lightmap"))
			{
				stage->bundle[0].isLightmap = qtrue;
				if (shader.lightmapIndex[0] < 0 || shader.lightmapIndex[0] >= tr.numLightmaps)
				{
#ifndef FINAL_BUILD
					vk_debug("Lightmap requested but none available for shader %s\n", shader.name);
#endif
					stage->bundle[0].image[0] = tr.whiteImage;
				}
				else
				{
					stage->bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex[0]];
				}
				continue;
			}
			else
			{
				imgFlags_t flags = IMGFLAG_NONE;

				if (!shader.noMipMaps)
					flags |= IMGFLAG_MIPMAP;

				if (!shader.noPicMip)
					flags |= IMGFLAG_PICMIP;

				if (shader.noTC)
					flags |= IMGFLAG_NO_COMPRESSION;

				if (shader.noLightScale)
					flags |= IMGFLAG_NOLIGHTSCALE;

				stage->bundle[0].image[0] = R_FindImageFile(token, flags);


				if (!stage->bundle[0].image[0])
				{
					vk_debug("WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name);
					return qfalse;
				}
			}
		}
		//
		// clampmap <name>
		//
		else if (!Q_stricmp(token, "clampmap") || !Q_stricmp(token, "screenMap"))
		{
			imgFlags_t flags;
			flags = IMGFLAG_CLAMPTOEDGE;

			if (!Q_stricmp(token, "screenMap")) {
				flags = IMGFLAG_NONE;
				if (vk.fboActive) {
					stage->bundle[0].isScreenMap = qtrue;
					shader.hasScreenMap = 1;
				}
			}
			else {
				flags = IMGFLAG_CLAMPTOEDGE;
			}

			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				vk_debug("WARNING: missing parameter for '%s' keyword in shader '%s'\n",
					stage->bundle[0].isScreenMap ? "screenMap" : "clampMap", shader.name);
				return qfalse;
			}

			if (!shader.noMipMaps)
				flags |= IMGFLAG_MIPMAP;

			if (!shader.noPicMip)
				flags |= IMGFLAG_PICMIP;

			if (shader.noTC)
				flags |= IMGFLAG_NO_COMPRESSION;

			if (shader.noLightScale)
				flags |= IMGFLAG_NOLIGHTSCALE;

			stage->bundle[0].image[0] = R_FindImageFile(token, flags);

			if (!stage->bundle[0].image[0])
			{
				vk_debug("WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name);
				return qfalse;
			}
		}

		//
		// animMap <frequency> <image1> .... <imageN>
		//
		else if (!Q_stricmp(token, "animMap") || !Q_stricmp(token, "clampanimMap") || !Q_stricmp(token, "oneshotanimMap"))
		{
			int			totalImages = 0;
			int			maxAnimations = MAX_IMAGE_ANIMATIONS;
			image_t		*images[MAX_IMAGE_ANIMATIONS];
			imgFlags_t	flags;
			bool		bClamp = !Q_stricmp( token, "clampanimMap" );
			bool		oneShot = !Q_stricmp( token, "oneshotanimMap" );

			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing parameter for '%s' keyword in shader '%s'\n", (bClamp ? "animMap":"clampanimMap"), shader.name );
				return qfalse;
			}
			stage->bundle[0].imageAnimationSpeed = atof( token );
			stage->bundle[0].oneShotAnimMap = oneShot;

			// parse up to MAX_IMAGE_ANIMATIONS animations
			while ( 1 ) {
				int num;

				token = COM_ParseExt( text, qfalse );
				if ( !token[0] )
					break;

				num = stage->bundle[0].numImageAnimations;
				if ( num < maxAnimations ) {
					flags = IMGFLAG_NONE;

					if ( !shader.noMipMaps )
						flags |= IMGFLAG_MIPMAP;

					if ( !shader.noPicMip )
						flags |= IMGFLAG_PICMIP;

					if ( shader.noTC )
						flags |= IMGFLAG_NO_COMPRESSION;

					if ( shader.noLightScale )
						flags |= IMGFLAG_NOLIGHTSCALE;

					if( bClamp )
						flags |= IMGFLAG_CLAMPTOEDGE;

					images[num] = R_FindImageFile( token, flags );
					if ( !images[num] )
					{
						ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
						return qfalse;
					}
					stage->bundle[0].numImageAnimations++;
				}

				totalImages++;
			}

			// Copy image ptrs into an array of ptrs
			memcpy( stage->bundle[0].image,	images,	stage->bundle[0].numImageAnimations * sizeof( image_t* ) );

			if ( totalImages > maxAnimations ) {
				ri.Printf(PRINT_WARNING, "WARNING: ignoring excess images for 'animMap' (found %d, max is %d) in shader '%s'\n",
					totalImages, maxAnimations, shader.name);
			}
		}
		else if (!Q_stricmp(token, "videoMap"))
		{
			int handle;
			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				vk_debug("WARNING: missing parameter for 'videoMap' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}
			handle = ri.CIN_PlayCinematic(token, 0, 0, 256, 256, (CIN_loop | CIN_silent | CIN_shader));
			if (handle != -1) {
				if (!tr.scratchImage[handle]) {
					tr.scratchImage[handle] = R_CreateImage(va("*scratch%i", handle), NULL, 256, 256, 
						IMGFLAG_CLAMPTOEDGE | IMGFLAG_RGB | IMGFLAG_NOSCALE | IMGFLAG_NO_COMPRESSION);
				}

				stage->bundle[0].isVideoMap = qtrue;
				stage->bundle[0].videoMapHandle = handle;
				stage->bundle[0].image[0] = tr.scratchImage[handle];
			}
			else {
				vk_debug("WARNING: could not load '%s' for 'videoMap' keyword in shader '%s'\n", token, shader.name);
			}
		}

		//
		// alphafunc <func>
		//
		else if (!Q_stricmp(token, "alphaFunc"))
		{
			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				vk_debug("WARNING: missing parameter for 'alphaFunc' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}

			atestBits = NameToAFunc(token);
		}
		//
		// depthFunc <func>
		//
		else if (!Q_stricmp(token, "depthfunc"))
		{
			token = COM_ParseExt(text, qfalse);

			if (!token[0])
			{
				vk_debug("WARNING: missing parameter for 'depthfunc' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}

			if (!Q_stricmp(token, "lequal"))
			{
				depthFuncBits = 0;
			}
			else if (!Q_stricmp(token, "equal"))
			{
				depthFuncBits = GLS_DEPTHFUNC_EQUAL;
			}
			else if (!Q_stricmp(token, "disable"))
			{
				depthFuncBits = GLS_DEPTHTEST_DISABLE;
			}
			else
			{
				vk_debug("WARNING: unknown depthfunc '%s' in shader '%s'\n", token, shader.name);
				continue;
			}
		}
		//
		// detail
		//
		else if (!Q_stricmp(token, "detail"))
		{
			stage->isDetail = qtrue;
		}
		//
		// blendfunc <srcFactor> <dstFactor>
		// or blendfunc <add|filter|blend>
		//
		else if (!Q_stricmp(token, "blendfunc"))
		{
			token = COM_ParseExt(text, qfalse);
			if (token[0] == 0)
			{
				vk_debug("WARNING: missing parm for blendFunc in shader '%s'\n", shader.name);
				continue;
			}
			// check for "simple" blends first
			if (!Q_stricmp(token, "add")) {
				blendSrcBits = GLS_SRCBLEND_ONE;
				blendDstBits = GLS_DSTBLEND_ONE;
			}
			else if (!Q_stricmp(token, "filter")) {
				blendSrcBits = GLS_SRCBLEND_DST_COLOR;
				blendDstBits = GLS_DSTBLEND_ZERO;
			}
			else if (!Q_stricmp(token, "blend")) {
				blendSrcBits = GLS_SRCBLEND_SRC_ALPHA;
				blendDstBits = GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
			}
			else {
				// complex double blends
				blendSrcBits = NameToSrcBlendMode(token);

				token = COM_ParseExt(text, qfalse);
				if (token[0] == 0)
				{
					vk_debug("WARNING: missing parm for blendFunc in shader '%s'\n", shader.name);
					continue;
				}
				blendDstBits = NameToDstBlendMode(token);
			}

			// clear depth mask for blended surfaces
			if ( !depthMaskExplicit ) {
				depthMaskBits = 0;
			}
		}
		//
		// rgbGen
		//
		else if (!Q_stricmp(token, "rgbGen"))
		{
			token = COM_ParseExt(text, qfalse);
			if (token[0] == 0)
			{
				vk_debug("WARNING: missing parameters for rgbGen in shader '%s'\n", shader.name);
				continue;
			}

			if (!Q_stricmp(token, "wave"))
			{
				ParseWaveForm(text, &stage->bundle[0].rgbWave);
				stage->bundle[0].rgbGen = CGEN_WAVEFORM;
			}
			else if (!Q_stricmp(token, "const"))
			{
				vec3_t	color;

				VectorClear(color);

				ParseVector(text, 3, color);
				stage->bundle[0].constantColor[0] = 255 * color[0];
				stage->bundle[0].constantColor[1] = 255 * color[1];
				stage->bundle[0].constantColor[2] = 255 * color[2];

				stage->bundle[0].rgbGen = CGEN_CONST;
			}
			else if (!Q_stricmp(token, "identity"))
			{
				stage->bundle[0].rgbGen = CGEN_IDENTITY;
			}
			else if (!Q_stricmp(token, "identityLighting"))
			{
				stage->bundle[0].rgbGen = CGEN_IDENTITY_LIGHTING;
			}
			else if (!Q_stricmp(token, "entity"))
			{
				stage->bundle[0].rgbGen = CGEN_ENTITY;
			}
			else if (!Q_stricmp(token, "oneMinusEntity"))
			{
				stage->bundle[0].rgbGen = CGEN_ONE_MINUS_ENTITY;
			}
			else if (!Q_stricmp(token, "vertex"))
			{
				stage->bundle[0].rgbGen = CGEN_VERTEX;
				if (stage->bundle[0].alphaGen == 0) {
					stage->bundle[0].alphaGen = AGEN_VERTEX;
				}
			}
			else if (!Q_stricmp(token, "exactVertex"))
			{
				stage->bundle[0].rgbGen = CGEN_EXACT_VERTEX;
			}
			else if (!Q_stricmp(token, "lightingDiffuse"))
			{
				stage->bundle[0].rgbGen = CGEN_LIGHTING_DIFFUSE;

			}
			else if (!Q_stricmp(token, "lightingDiffuseEntity"))
			{
				if (shader.lightmapIndex[0] != LIGHTMAP_NONE)
				{
					vk_debug("ERROR: rgbGen lightingDiffuseEntity used on a misc_model! in shader '%s'\n", shader.name);
				}
				stage->bundle[0].rgbGen = CGEN_LIGHTING_DIFFUSE_ENTITY;

			}
			else if (!Q_stricmp(token, "oneMinusVertex"))
			{
				stage->bundle[0].rgbGen = CGEN_ONE_MINUS_VERTEX;
			}
			else
			{
				vk_debug("WARNING: unknown rgbGen parameter '%s' in shader '%s'\n", token, shader.name);
				continue;
			}
		}
		//
		// alphaGen
		//
		else if (!Q_stricmp(token, "alphaGen"))
		{
			token = COM_ParseExt(text, qfalse);
			if (token[0] == 0)
			{
				vk_debug("WARNING: missing parameters for alphaGen in shader '%s'\n", shader.name);
				continue;
			}

			if (!Q_stricmp(token, "wave"))
			{
				ParseWaveForm(text, &stage->bundle[0].alphaWave);
				stage->bundle[0].alphaGen = AGEN_WAVEFORM;
			}
			else if (!Q_stricmp(token, "const"))
			{
				token = COM_ParseExt(text, qfalse);
				stage->bundle[0].constantColor[3] = 255 * atof(token);
				stage->bundle[0].alphaGen = AGEN_CONST;
			}
			else if (!Q_stricmp(token, "identity"))
			{
				stage->bundle[0].alphaGen = AGEN_IDENTITY;
			}
			else if (!Q_stricmp(token, "entity"))
			{
				stage->bundle[0].alphaGen = AGEN_ENTITY;
			}
			else if (!Q_stricmp(token, "oneMinusEntity"))
			{
				stage->bundle[0].alphaGen = AGEN_ONE_MINUS_ENTITY;
			}
			else if (!Q_stricmp(token, "vertex"))
			{
				stage->bundle[0].alphaGen = AGEN_VERTEX;
			}
			else if (!Q_stricmp(token, "lightingSpecular"))
			{
				stage->bundle[0].alphaGen = AGEN_LIGHTING_SPECULAR;
			}
			else if (!Q_stricmp(token, "oneMinusVertex"))
			{
				stage->bundle[0].alphaGen = AGEN_ONE_MINUS_VERTEX;
			}
			else if (!Q_stricmp(token, "dot"))
			{
				stage->bundle[0].alphaGen = AGEN_DOT;
			}
			else if (!Q_stricmp(token, "oneMinusDot"))
			{
				stage->bundle[0].alphaGen = AGEN_ONE_MINUS_DOT;
			}
			else if (!Q_stricmp(token, "portal"))
			{
				stage->bundle[0].alphaGen = AGEN_PORTAL;
				token = COM_ParseExt(text, qfalse);
				if (token[0] == 0)
				{
					shader.portalRange = 256;
					vk_debug("WARNING: missing range parameter for alphaGen portal in shader '%s', defaulting to 256\n", shader.name);
				}
				else
				{
					shader.portalRange = atof(token);

					if ( shader.portalRange < 0.001f )
						shader.portalRangeR = 0.0f;
					else
						shader.portalRangeR = 1.0f / shader.portalRange;
				}
			}
			else
			{
				vk_debug("WARNING: unknown alphaGen parameter '%s' in shader '%s'\n", token, shader.name);
				continue;
			}
		}
		//
		// tcGen <function>
		//
		else if (!Q_stricmp(token, "texgen") || !Q_stricmp(token, "tcGen"))
		{
			token = COM_ParseExt(text, qfalse);
			if (token[0] == 0)
			{
				vk_debug("WARNING: missing texgen parm in shader '%s'\n", shader.name);
				continue;
			}

			if (!Q_stricmp(token, "environment"))
			{
				stage->bundle[0].tcGen = TCGEN_ENVIRONMENT_MAPPED;
			}
			else if (!Q_stricmp(token, "lightmap"))
			{
				stage->bundle[0].tcGen = TCGEN_LIGHTMAP;
			}
			else if (!Q_stricmp(token, "texture") || !Q_stricmp(token, "base"))
			{
				stage->bundle[0].tcGen = TCGEN_TEXTURE;
			}
			else if (!Q_stricmp(token, "vector"))
			{
				stage->bundle[0].tcGenVectors = (vec3_t*)Hunk_Alloc(2 * sizeof(vec3_t), h_low);
				ParseVector(text, 3, stage->bundle[0].tcGenVectors[0]);
				ParseVector(text, 3, stage->bundle[0].tcGenVectors[1]);

				stage->bundle[0].tcGen = TCGEN_VECTOR;
			}
			else
			{
				vk_debug("WARNING: unknown texgen parm in shader '%s'\n", shader.name);
			}
		}
		//
		// tcMod <type> <...>
		//
		else if (!Q_stricmp(token, "tcMod"))
		{
			char buffer[1024] = "";

			while (1)
			{
				token = COM_ParseExt(text, qfalse);
				if (token[0] == 0)
					break;
				Q_strcat(buffer, sizeof(buffer), token);
				Q_strcat(buffer, sizeof(buffer), " ");
			}

			ParseTexMod(buffer, stage);

			continue;
		}
		//
		// depthmask
		//
		else if (!Q_stricmp(token, "depthwrite"))
		{
			depthMaskBits = GLS_DEPTHMASK_TRUE;
			depthMaskExplicit = qtrue;

			continue;
		}

		// depthFragment
		else if (!Q_stricmp(token, "depthFragment"))
		{
			stage->depthFragment = qtrue;
		}

		// If this stage has glow...	GLOWXXX
		else if (Q_stricmp(token, "glow") == 0)
		{
			stage->glow = stage->bundle[0].glow = true;

			continue;
		}
		//
		// surfaceSprites <type> ...
		//
		else if (!Q_stricmp(token, "surfaceSprites"))
		{
			char buffer[1024] = "";

			while (1)
			{
				token = COM_ParseExt(text, qfalse);
				if (token[0] == 0)
					break;
				Q_strcat(buffer, sizeof(buffer), token);
				Q_strcat(buffer, sizeof(buffer), " ");
			}

			bool hasSS = (stage->ss != nullptr);
			if ( ParseSurfaceSprites( buffer, stage ) && !hasSS )
			{
				shader.surface_sprites.num_stages++;
			}

			continue;
		}
		//
		// ssFademax <fademax>
		// ssFadescale <fadescale>
		// ssVariance <varwidth> <varheight>
		// ssHangdown
		// ssAnyangle
		// ssFaceup
		// ssWind <wind>
		// ssWindIdle <windidle>
		// ssDuration <duration>
		// ssGrow <growwidth> <growheight>
		// ssWeather
		//
		else if (!Q_stricmpn(token, "ss", 2))	// <--- NOTE ONLY COMPARING FIRST TWO LETTERS
		{
			char buffer[1024] = "";
			char param[128];
			strcpy(param, token);

			while (1)
			{
				token = COM_ParseExt(text, qfalse);
				if (token[0] == 0)
					break;
				Q_strcat(buffer, sizeof(buffer), token);
				Q_strcat(buffer, sizeof(buffer), " ");
			}

			bool hasSS = (stage->ss != nullptr);
			if ( ParseSurfaceSpritesOptional( param, buffer, stage ) && !hasSS )
			{
				shader.surface_sprites.num_stages++;
			}

			continue;
		}
		else
		{
			vk_debug("WARNING: unknown parameter '%s' in shader '%s'\n", token, shader.name);
			return qfalse;
		}
	}

	//
	// if cgen isn't explicitly specified, use either identity or identitylighting
	//
	if ( stage->bundle[0].rgbGen == CGEN_BAD ) {
		if ( //blendSrcBits == 0 ||
			blendSrcBits == GLS_SRCBLEND_ONE ||
			blendSrcBits == GLS_SRCBLEND_SRC_ALPHA) {
			stage->bundle[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
		else {
			stage->bundle[0].rgbGen = CGEN_IDENTITY;
		}
	}


	//
	// implicitly assume that a GL_ONE GL_ZERO blend mask disables blending
	//
	if ( ( blendSrcBits == GLS_SRCBLEND_ONE ) && ( blendDstBits == GLS_DSTBLEND_ZERO ) ) {
		blendDstBits = blendSrcBits = 0;
		depthMaskBits = GLS_DEPTHMASK_TRUE;
	}

	// decide which agens we can skip
	if ( stage->bundle[0].alphaGen == AGEN_IDENTITY ) {
		if ( stage->bundle[0].rgbGen == CGEN_IDENTITY || stage->bundle[0].rgbGen == CGEN_LIGHTING_DIFFUSE ) {
			stage->bundle[0].alphaGen = AGEN_SKIP;
		}
	}

#if 0 // disable this for now, because it seems to be causing artifacts instead of fixing them for JK3.
	if ( depthMaskExplicit && shader.sort == SS_BAD ) {
		// fix decals on q3wcp18 and other maps
		if ( blendSrcBits == GLS_SRCBLEND_SRC_ALPHA && blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA ) {
			if ( stage->bundle[0].alphaGen != AGEN_SKIP ) {
				// q3wcp18 @ "textures/ctf_unified/floor_decal_blue" : AGEN_VERTEX, CGEN_VERTEX
				// check for grates on tscabdm3
				if ( atestBits == 0 ) {
					depthMaskBits &= ~GLS_DEPTHMASK_TRUE;
				}
			} else {
				// skip for q3wcp14 jumppads and similar
				// q3wcp14 @ "textures/ctf_unified/bounce_blue" : AGEN_SKIP, CGEN_IDENTITY
			}
			shader.sort = shader.polygonOffset ? SS_DECAL : SS_OPAQUE + 0.01f;
		} else if ( blendSrcBits == GLS_SRCBLEND_ZERO && blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_COLOR && stage->bundle[0].rgbGen == CGEN_EXACT_VERTEX ) {
			depthMaskBits &= ~GLS_DEPTHMASK_TRUE;
			shader.sort = SS_SEE_THROUGH;
		}
	}
#endif

	//
	// compute state bits
	//
	stage->stateBits = depthMaskBits |
		blendSrcBits | blendDstBits |
		atestBits |
		depthFuncBits;

	return qtrue;
}

/*
===============
ParseDeform

deformVertexes wave <spread> <waveform> <base> <amplitude> <phase> <frequency>
deformVertexes normal <frequency> <amplitude>
deformVertexes move <vector> <waveform> <base> <amplitude> <phase> <frequency>
deformVertexes bulge <bulgeWidth> <bulgeHeight> <bulgeSpeed>
deformVertexes projectionShadow
deformVertexes autoSprite
deformVertexes autoSprite2
deformVertexes text[0-7]
===============
*/
static void ParseDeform( const char **text ) {
	const char		*token;
	deformStage_t	*ds;

	token = COM_ParseExt(text, qfalse);
	if (token[0] == 0)
	{
		vk_debug("WARNING: missing deform parm in shader '%s'\n", shader.name);
		return;
	}

	if (shader.numDeforms == MAX_SHADER_DEFORMS) {
		vk_debug("WARNING: MAX_SHADER_DEFORMS in '%s'\n", shader.name);
		return;
	}

	shader.deforms[shader.numDeforms] = (deformStage_t*)Hunk_Alloc(sizeof(deformStage_t), h_low);

	ds = shader.deforms[shader.numDeforms];
	shader.numDeforms++;

	if (!Q_stricmp(token, "projectionShadow")) {
		ds->deformation = DEFORM_PROJECTION_SHADOW;
		return;
	}

	if (!Q_stricmp(token, "autosprite")) {
		ds->deformation = DEFORM_AUTOSPRITE;
		return;
	}

	if (!Q_stricmp(token, "autosprite2")) {
		ds->deformation = DEFORM_AUTOSPRITE2;
		return;
	}

	if (!Q_stricmpn(token, "text", 4)) {
		int		n;

		n = token[4] - '0';
		if (n < 0 || n > 7) {
			n = 0;
		}
		ds->deformation = (deform_t)(DEFORM_TEXT0 + n);
		return;
	}

	if (!Q_stricmp(token, "bulge")) {
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name);
			return;
		}
		ds->bulgeWidth = atof(token);

		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name);
			return;
		}
		ds->bulgeHeight = atof(token);

		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name);
			return;
		}
		ds->bulgeSpeed = atof(token);

		ds->deformation = DEFORM_BULGE;
		return;
	}

	if (!Q_stricmp(token, "wave"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing deformVertexes parm in shader '%s'\n", shader.name);
			return;
		}

		if (atof(token) != 0)
		{
			ds->deformationSpread = 1.0f / atof(token);
		}
		else
		{
			ds->deformationSpread = 100.0f;
			vk_debug("WARNING: illegal div value of 0 in deformVertexes command for shader '%s'\n", shader.name);
		}

		ParseWaveForm(text, &ds->deformationWave);
		ds->deformation = DEFORM_WAVE;
		return;
	}

	if (!Q_stricmp(token, "normal"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing deformVertexes parm in shader '%s'\n", shader.name);
			return;
		}
		ds->deformationWave.amplitude = atof(token);

		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			vk_debug("WARNING: missing deformVertexes parm in shader '%s'\n", shader.name);
			return;
		}
		ds->deformationWave.frequency = atof(token);

		ds->deformation = DEFORM_NORMALS;
		return;
	}

	if (!Q_stricmp(token, "move")) {
		int		i;

		for (i = 0; i < 3; i++) {
			token = COM_ParseExt(text, qfalse);
			if (token[0] == 0) {
				vk_debug("WARNING: missing deformVertexes parm in shader '%s'\n", shader.name);
				return;
			}
			ds->moveVector[i] = atof(token);
		}

		ParseWaveForm(text, &ds->deformationWave);
		ds->deformation = DEFORM_MOVE;
		return;
	}

	vk_debug("WARNING: unknown deformVertexes subtype '%s' found in shader '%s'\n", token, shader.name);
}

/*
===============
ParseSkyParms

skyParms <outerbox> <cloudheight> <innerbox>
===============
*/
static void ParseSkyParms( const char **text ) {
	const char			*token;
	static const char	*suf[6] = { "rt", "bk", "lf", "ft", "up", "dn" };
	char				pathname[MAX_QPATH];
	int					i;
	imgFlags_t			imgFlags;

	imgFlags = IMGFLAG_MIPMAP | IMGFLAG_PICMIP;
	if (shader.noTC) {
		imgFlags |= IMGFLAG_NO_COMPRESSION;
	}

	shader.sky = (skyParms_t*)Hunk_Alloc(sizeof(skyParms_t), h_low);

	// outerbox
	token = COM_ParseExt(text, qfalse);
	if (token[0] == 0) {
		vk_debug("WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name);
		return;
	}
	if (strcmp(token, "-")) {
		for (i = 0; i < 6; i++) {
			Com_sprintf(pathname, sizeof(pathname), "%s_%s", token, suf[i]);
			shader.sky->outerbox[i] = R_FindImageFile((char*)pathname, imgFlags | IMGFLAG_CLAMPTOEDGE);
			if (!shader.sky->outerbox[i]) {
				if (i) {
					shader.sky->outerbox[i] = shader.sky->outerbox[i - 1];//not found, so let's use the previous image
				}
				else {
					shader.sky->outerbox[i] = tr.defaultImage;
				}
			}
		}
	}

	// cloudheight
	token = COM_ParseExt(text, qfalse);
	if (token[0] == 0) {
		vk_debug("WARNING: 'skyParms' missing cloudheight in shader '%s'\n", shader.name);
		return;
	}
	shader.sky->cloudHeight = atof(token);
	if (!shader.sky->cloudHeight) {
		shader.sky->cloudHeight = 512;
	}
	R_InitSkyTexCoords(shader.sky->cloudHeight);

	// innerbox
	token = COM_ParseExt(text, qfalse);
	if (token[0] == 0) {
		ri.Printf(PRINT_WARNING, "WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name);
		return;
	}
	if (strcmp(token, "-")) {
		for (i = 0; i < 6; i++) {
			Com_sprintf(pathname, sizeof(pathname), "%s_%s", token, suf[i]);
			shader.sky->innerbox[i] = R_FindImageFile((char*)pathname, imgFlags);
			if (!shader.sky->innerbox[i]) {
				if (i) {
					shader.sky->innerbox[i] = shader.sky->innerbox[i - 1];//not found, so let's use the previous image
				}
				else {
					shader.sky->innerbox[i] = tr.defaultImage;
				}
			}
		}
	}

	shader.isSky = qtrue;
}

/*
=================
ParseSort
=================
*/
static void ParseSort( const char **text ) {
	const char	*token;

	token = COM_ParseExt(text, qfalse);
	if (token[0] == 0) {
		vk_debug("WARNING: missing sort parameter in shader '%s'\n", shader.name);
		return;
	}

	if (!Q_stricmp(token, "portal")) {
		shader.sort = SS_PORTAL;
	}
	else if (!Q_stricmp(token, "sky")) {
		shader.sort = SS_ENVIRONMENT;
	}
	else if (!Q_stricmp(token, "opaque")) {
		shader.sort = SS_OPAQUE;
	}
	else if (!Q_stricmp(token, "decal")) {
		shader.sort = SS_DECAL;
	}
	else if (!Q_stricmp(token, "seeThrough")) {
		shader.sort = SS_SEE_THROUGH;
	}
	else if (!Q_stricmp(token, "banner")) {
		shader.sort = SS_BANNER;
	}
	else if (!Q_stricmp(token, "additive")) {
		shader.sort = SS_BLEND1;
	}
	else if (!Q_stricmp(token, "nearest")) {
		shader.sort = SS_NEAREST;
	}
	else if (!Q_stricmp(token, "underwater")) {
		shader.sort = SS_UNDERWATER;
	}
	else if (!Q_stricmp(token, "inside")) {
		shader.sort = SS_INSIDE;
	}
	else if (!Q_stricmp(token, "mid_inside")) {
		shader.sort = SS_MID_INSIDE;
	}
	else if (!Q_stricmp(token, "middle")) {
		shader.sort = SS_MIDDLE;
	}
	else if (!Q_stricmp(token, "mid_outside")) {
		shader.sort = SS_MID_OUTSIDE;
	}
	else if (!Q_stricmp(token, "outside")) {
		shader.sort = SS_OUTSIDE;
	}
	else {
		shader.sort = atof(token);
	}
}

/*
====================
FindShaderInShaderText

Scans the combined text description of all the shader files for the given
shader name. If found, it will return a valid shader, return NULL if not found.
=====================
*/
static const char *FindShaderInShaderText( const char *shadername ) {
	char		*token;
	const char	*p;

	int i, hash;

	hash = generateHashValue(shadername, MAX_SHADERTEXT_HASH);

	if (shaderTextHashTable[hash]) {
		for (i = 0; shaderTextHashTable[hash][i]; i++) {
			p = shaderTextHashTable[hash][i];
			token = COM_ParseExt(&p, qtrue);
			if (!Q_stricmp(token, shadername))
				return p;
		}
	}

	p = s_shaderText;

	if (!p) {
		return NULL;
	}

	// look for label
	while (1) {
		token = COM_ParseExt(&p, qtrue);
		if (token[0] == 0) {
			break;
		}

		if (!Q_stricmp(token, shadername)) {
			return p;
		}
		else {
			// skip the definition
			SkipBracedSection(&p, 0);
		}
	}
	return NULL;
}

/*
==================
R_FindShaderByName

Will always return a valid shader, but it might be the
default shader if the real one can't be found.
==================
*/
shader_t *R_FindShaderByName( const char *name ) {
	char		strippedName[MAX_QPATH];
	int			hash;
	shader_t	*sh;

	if ((name == NULL) || (name[0] == 0)) {
		return tr.defaultShader;
	}

	COM_StripExtension(name, strippedName, sizeof(strippedName));

	hash = generateHashValue(strippedName, FILE_HASH_SIZE);

	//
	// see if the shader is already loaded
	//
	sh = hashTable[hash];
	while (sh)
	{
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if (Q_stricmp(sh->name, strippedName) == 0) {
			return sh;
		}

		sh = sh->next;
	}

	return tr.defaultShader;
}

qhandle_t RE_RegisterShaderLightMap( const char *name, const int *lightmapIndex, const byte *styles )
{
	shader_t *sh;

	if (strlen(name) >= MAX_QPATH) {
		ri.Printf(PRINT_ALL, "Shader name exceeds MAX_QPATH\n");
		return 0;
	}

	sh = R_FindShader(name, lightmapIndex, styles, qtrue);

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if (sh->defaultShader) {
		return 0;
	}

	return sh->index;
}

void R_RemapShader( const char *shaderName, const char *newShaderName, const char *timeOffset )
{
	char		strippedName[MAX_QPATH];
	int			hash;
	shader_t	*sh, *sh2;
	qhandle_t	h;

	sh = R_FindShaderByName( shaderName );
	if (sh == NULL || sh == tr.defaultShader) {
		h = RE_RegisterShaderLightMap(shaderName, lightmapsNone, stylesDefault);
		sh = R_GetShaderByHandle(h);
	}
	if (sh == NULL || sh == tr.defaultShader) {
		ri.Printf( PRINT_WARNING, "WARNING: RE_RemapShader: shader %s not found\n", shaderName );
		return;
	}

	sh2 = R_FindShaderByName( newShaderName );
	if (sh2 == NULL || sh2 == tr.defaultShader) {
		h = RE_RegisterShaderLightMap(newShaderName, lightmapsNone, stylesDefault);
		sh2 = R_GetShaderByHandle(h);
	}

	if (sh2 == NULL || sh2 == tr.defaultShader) {
		ri.Printf( PRINT_WARNING, "WARNING: RE_RemapShader: new shader %s not found\n", newShaderName );
		return;
	}

	COM_StripExtension(shaderName, strippedName, sizeof(strippedName));
	hash = generateHashValue(strippedName, FILE_HASH_SIZE);
	sh = hashTable[hash];

	// remap all the shaders with the given name
	// even tho they might have different lightmaps
	while (sh)
	{
		if (Q_stricmp(sh->name, strippedName) == 0)
		{
			if (sh != sh2)
			{
				sh->remappedShader = sh2;
			}
			else
			{
				sh->remappedShader = NULL;
			}
		}
		sh = sh->next;
	}

	if (timeOffset)
	{
		sh2->timeOffset = atof(timeOffset);
	}
}

//added for ui -rww
const char *RE_ShaderNameFromIndex( int index)
{
	assert(index >= 0 && index < tr.numShaders && tr.shaders[index]);
	return tr.shaders[index]->name;
}

/*
===============
R_FindShader

Will always return a valid shader, but it might be the
default shader if the real one can't be found.

In the interest of not requiring an explicit shader text entry to
be defined for every single image used in the game, three default
shader behaviors can be auto-created for any image:

If lightmapIndex == LIGHTMAP_NONE, then the image will have
dynamic diffuse lighting applied to it, as apropriate for most
entity skin surfaces.

If lightmapIndex == LIGHTMAP_2D, then the image will be used
for 2D rendering unless an explicit shader is found

If lightmapIndex == LIGHTMAP_BY_VERTEX, then the image will use
the vertex rgba modulate values, as apropriate for misc_model
pre-lit surfaces.

Other lightmapIndex values will have a lightmap stage created
and src*dest blending applied with the texture, as apropriate for
most world construction surfaces.

===============
*/
inline qboolean IsShader( shader_t *sh, const char *name, const int *lightmapIndex, const byte *styles )
{
	int	i;

	if (Q_stricmp(sh->name, name))
	{
		return qfalse;
	}

	if (!sh->defaultShader)
	{
		for (i = 0; i < MAXLIGHTMAPS; i++)
		{
			if (sh->lightmapSearchIndex[i] != lightmapIndex[i])
			{
				return qfalse;
			}
			if (sh->styles[i] != styles[i])
			{
				return qfalse;
			}
		}
	}

	return qtrue;
}

/*
=================
ParseShader

The current text pointer is at the explicit text definition of the shader.
Parse it into the global shader variable.  Later functions will optimize it.
=================
*/
static qboolean ParseShader( const char **text )
{
	char		*token;
	const char	*begin = *text;
	int			s;

	s = 0;

	token = COM_ParseExt(text, qtrue);
	if (token[0] != '{')
	{
		vk_debug("WARNING: expecting '{', found '%s' instead in shader '%s'\n", token, shader.name);
		return qfalse;
	}

	while (1)
	{
		token = COM_ParseExt(text, qtrue);
		if (!token[0])
		{
			vk_debug("WARNING: no concluding '}' in shader %s\n", shader.name);
			return qfalse;
		}

		// end of shader definition
		if (token[0] == '}')
		{
			break;
		}
		// stage definition
		else if (token[0] == '{')
		{
			if (s >= MAX_SHADER_STAGES) {
				vk_debug("WARNING: too many stages in shader %s (max is %i)\n", shader.name, MAX_SHADER_STAGES);
				return qfalse;
			}

			if (!ParseStage(&stages[s], text))
			{
				return qfalse;
			}
			stages[s].active = qtrue;
			if (stages[s].glow)
			{
				shader.hasGlow = qtrue;
			}
			s++;
			continue;
		}
		// skip stuff that only the QuakeEdRadient needs
		else if (!Q_stricmpn(token, "qer", 3)) {
			SkipRestOfLine(text);
			continue;
		}
		// material deprecated as of 11 Jan 01
		// material undeprecated as of 7 May 01 - q3map_material deprecated
		else if (!Q_stricmp(token, "material") || !Q_stricmp(token, "q3map_material"))
		{
			ParseMaterial(text);
		}
		// sun parms
		else if (!Q_stricmp(token, "sun") || !Q_stricmp(token, "q3map_sun") || !Q_stricmp(token, "q3map_sunExt"))
		{
			token = COM_ParseExt(text, qfalse);
			tr.sunLight[0] = atof(token);
			token = COM_ParseExt(text, qfalse);
			tr.sunLight[1] = atof(token);
			token = COM_ParseExt(text, qfalse);
			tr.sunLight[2] = atof(token);

			VectorNormalize(tr.sunLight);

			token = COM_ParseExt(text, qfalse);
			float a = atof(token);
			VectorScale(tr.sunLight, a, tr.sunLight);

			token = COM_ParseExt(text, qfalse);
			a = atof(token);
			a = a / 180 * M_PI;

			token = COM_ParseExt(text, qfalse);
			float b = atof(token);
			b = b / 180 * M_PI;

			tr.sunDirection[0] = cos(a) * cos(b);
			tr.sunDirection[1] = sin(a) * cos(b);
			tr.sunDirection[2] = sin(b);

			SkipRestOfLine(text);
			continue;
		}
		// q3map_surfacelight deprecated as of 16 Jul 01
		else if (!Q_stricmp(token, "surfacelight") || !Q_stricmp(token, "q3map_surfacelight"))
		{
			token = COM_ParseExt(text, qfalse);
			tr.sunSurfaceLight = atoi(token);
		}
		else if (!Q_stricmp(token, "lightColor"))
		{
			/*
			if ( !ParseVector( text, 3, tr.sunAmbient ) )
			{
				return qfalse;
			}
			*/
			//SP skips this so I'm skipping it here too.
			SkipRestOfLine(text);
			continue;
		}
		else if (!Q_stricmp(token, "deformvertexes") || !Q_stricmp(token, "deform")) {
			ParseDeform(text);
			continue;
		}
		else if (!Q_stricmp(token, "tesssize")) {
			SkipRestOfLine(text);
			continue;
		}
		else if (!Q_stricmp(token, "clampTime")) {
			token = COM_ParseExt(text, qfalse);
			if (token[0]) {
				shader.clampTime = atof(token);
			}
		}
		// skip stuff that only the q3map needs
		else if (!Q_stricmpn(token, "q3map", 5)) {
			SkipRestOfLine(text);
			continue;
		}
		// skip stuff that only q3map or the server needs
		else if (!Q_stricmp(token, "surfaceParm")) {
			ParseSurfaceParm(text);
			continue;
		}
		// no mip maps
		else if (!Q_stricmp(token, "nomipmaps"))
		{
			shader.noMipMaps = 1;
			shader.noPicMip = 1;
			continue;
		}
		// no picmip adjustment
		else if (!Q_stricmp(token, "nopicmip"))
		{
			shader.noPicMip = 1;
			continue;
		}
		else if (!Q_stricmp(token, "noglfog"))
		{
			shader.fogPass = FP_NONE;
			continue;
		}
		// polygonOffset
		else if (!Q_stricmp(token, "polygonOffset"))
		{
			shader.polygonOffset = qtrue;
			continue;
		}
		else if (!Q_stricmp(token, "noTC"))
		{
			shader.noTC = 1;
			continue;
		}
		// entityMergable, allowing sprite surfaces from multiple entities
		// to be merged into one batch.  This is a savings for smoke
		// puffs and blood, but can't be used for anything where the
		// shader calcs (not the surface function) reference the entity color or scroll
		else if (!Q_stricmp(token, "entityMergable"))
		{
			shader.entityMergable = qtrue;
			continue;
		}
		// fogParms
		else if (!Q_stricmp(token, "fogParms"))
		{
			shader.fogParms = (fogParms_t*)Hunk_Alloc(sizeof(fogParms_t), h_low);
			if (!ParseVector(text, 3, shader.fogParms->color)) {
				return qfalse;
			}

			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				vk_debug("WARNING: missing parm for 'fogParms' keyword in shader '%s'\n", shader.name);
				continue;
			}
			shader.fogParms->depthForOpaque = atof(token);

			// skip any old gradient directions
			SkipRestOfLine(text);
			continue;
		}
		// portal
		else if (!Q_stricmp(token, "portal"))
		{
			shader.sort = SS_PORTAL;
			continue;
		}
		// skyparms <cloudheight> <outerbox> <innerbox>
		else if (!Q_stricmp(token, "skyparms"))
		{
			ParseSkyParms(text);
			shader.noPicMip = 1;
			shader.noMipMaps = 1;
			continue;
		}
		// light <value> determines flaring in q3map, not needed here
		else if (!Q_stricmp(token, "light"))
		{
			token = COM_ParseExt(text, qfalse);
			continue;
		}
		// cull <face>
		else if (!Q_stricmp(token, "cull"))
		{
			token = COM_ParseExt(text, qfalse);
			if (token[0] == 0)
			{
				vk_debug("WARNING: missing cull parms in shader '%s'\n", shader.name);
				continue;
			}

			if (!Q_stricmp(token, "none") || !Q_stricmp(token, "twosided") || !Q_stricmp(token, "disable"))
			{
				shader.cullType = CT_TWO_SIDED;
			}
			else if (!Q_stricmp(token, "back") || !Q_stricmp(token, "backside") || !Q_stricmp(token, "backsided"))
			{
				shader.cullType = CT_BACK_SIDED;
			}
			else
			{
				vk_debug("WARNING: invalid cull parm '%s' in shader '%s'\n", token, shader.name);
			}
			continue;
		}
		// sort
		else if (!Q_stricmp(token, "sort"))
		{
			ParseSort(text);
			continue;
		}
		else
		{
			vk_debug("WARNING: unknown general shader parameter '%s' in '%s'\n", token, shader.name);
			return qfalse;
		}
	}

	//
	// ignore shaders that don't have any stages, unless it is a sky or fog
	//
	if (s == 0 && !shader.isSky && !(shader.contentFlags & CONTENTS_FOG)) {
		return qfalse;
	}

	shader.explicitlyDefined = qtrue;

	// The basejka rocket lock wedge shader uses the incorrect blending mode.
	// It only worked because the shader state was not being set, and relied
	// on previous state to be multiplied by alpha. Since fixing RB_RotatePic,
	// the shader needs to be fixed here to render correctly.
	//
	// We match against the retail version of gfx/2d/wedge by calculating the
	// hash value of the shader text, and comparing it against a precalculated
	// value.
	uint32_t shaderHash = generateHashValueForText(begin, *text - begin);
	if (shaderHash == RETAIL_ROCKET_WEDGE_SHADER_HASH &&
		Q_stricmp(shader.name, "gfx/2d/wedge") == 0)
	{
		stages[0].stateBits &= ~(GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS);
		stages[0].stateBits |= GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	}

	// The basejka radar arrow contains an incorrect rgbGen of identity
	// It only worked because the original code didn't check shaders at all,
	// thus setcolor worked fine but with fixing RB_RotatePic it no longer
	// functioned because rgbGen identity doesn't work with setcolor.
	//
	// We match against retail version of gfx/menus/radar/arrow_w by calculating
	// the hash value of the shader text, and comparing it against a 
	// precalculated value.
	if (shaderHash == RETAIL_ARROW_W_SHADER_HASH &&
		Q_stricmp(shader.name, "gfx/menus/radar/arrow_w") == 0)
	{
		stages[0].bundle[0].rgbGen = CGEN_VERTEX;
		stages[0].bundle[0].alphaGen = AGEN_VERTEX;
	}

	return qtrue;
}

/*
====================
This is the exported shader entry point for the rest of the system
It will always return an index that will be valid.

This should really only be used for explicit shaders, because there is no
way to ask for different implicit lighting modes (vertex, lightmap, etc)
====================
*/
qhandle_t RE_RegisterShader( const char *name )
{
	shader_t	*sh;

	if (strlen(name) >= MAX_QPATH) {
		vk_debug("Shader name exceeds MAX_QPATH\n");
		return 0;
	}

	sh = R_FindShader(name, lightmaps2d, stylesDefault, qtrue);

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if (sh->defaultShader) {
		return 0;
	}

	return sh->index;
}

/*
====================
RE_RegisterShaderNoMip

For menu graphics that should never be picmiped
====================
*/
qhandle_t RE_RegisterShaderNoMip( const char *name )
{
	shader_t *sh;

	if (strlen(name) >= MAX_QPATH) {
		vk_debug("Shader name exceeds MAX_QPATH\n");
		return 0;
	}

	sh = R_FindShader(name, lightmaps2d, stylesDefault, qfalse);

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if (sh->defaultShader) {
		return 0;
	}

	return sh->index;
}

static int COM_CompressShader( char *data_p )
{
	char *in, *out;
	int c;
	qboolean newline = qfalse, whitespace = qfalse;

	in = out = data_p;
	if (in)
	{
		while ((c = *in) != 0)
		{
			// skip double slash comments
			if (c == '/' && in[1] == '/')
			{
				while (*in && *in != '\n')
				{
					in++;
				}
			}
			// skip number sign comments
			else if (c == '#')
			{
				while (*in && *in != '\n')
				{
					in++;
				}
			}
			// skip /* */ comments
			else if (c == '/' && in[1] == '*')
			{
				while (*in && (*in != '*' || in[1] != '/'))
					in++;
				if (*in)
					in += 2;
			}
			// record when we hit a newline
			else if (c == '\n' || c == '\r')
			{
				newline = qtrue;
				in++;
			}
			// record when we hit whitespace
			else if (c == ' ' || c == '\t')
			{
				whitespace = qtrue;
				in++;
				// an actual token
			}
			else
			{
				// if we have a pending newline, emit it (and it counts as whitespace)
				if (newline)
				{
					*out++ = '\n';
					newline = qfalse;
					whitespace = qfalse;
				} if (whitespace)
				{
					*out++ = ' ';
					whitespace = qfalse;
				}

				// copy quoted strings unmolested
				if (c == '"')
				{
					*out++ = c;
					in++;
					while (1)
					{
						c = *in;
						if (c && c != '"')
						{
							*out++ = c;
							in++;
						}
						else
						{
							break;
						}
					}
					if (c == '"')
					{
						*out++ = c;
						in++;
					}
				}
				else
				{
					*out = c;
					out++;
					in++;
				}
			}
		}

		*out = 0;
	}
	return out - data_p;
}

/*
====================
ScanAndLoadShaderFiles

Finds and loads all .shader files, combining them into
a single large text block that can be scanned for shader names
=====================
*/
#define	MAX_SHADER_FILES	4096
static void ScanAndLoadShaderFiles( void )
{
	char		**shaderFiles;
	char		*buffers[MAX_SHADER_FILES];
	const char	*p;
	int			numShaderFiles;
	int			i;
	const char	*token, *hashMem;
	char		*oldp, *textEnd;
	int			shaderTextHashTableSizes[MAX_SHADERTEXT_HASH], hash, size;
	char		shaderName[MAX_QPATH];
	int			shaderLine;
	long		sum = 0, summand;

	// scan for shader files
	shaderFiles = ri.FS_ListFiles("shaders", ".shader", &numShaderFiles);

	if (!shaderFiles || !numShaderFiles)
	{
		ri.Error(ERR_FATAL, "ERROR: no shader files found");
		return;
	}

	if (numShaderFiles > MAX_SHADER_FILES) {
		numShaderFiles = MAX_SHADER_FILES;
	}

	// load and parse shader files
	for (i = 0; i < numShaderFiles; i++)
	{
		char filename[MAX_QPATH];

		Com_sprintf(filename, sizeof(filename), "shaders/%s", shaderFiles[i]);
		vk_debug("...loading '%s'\n", filename);
		summand = ri.FS_ReadFile(filename, (void**)&buffers[i]);

		if (!buffers[i]) {
			vk_debug("Couldn't load %s", filename);
		}

		// Do a simple check on the shader structure in that file to make sure one bad shader file cannot fuck up all other shaders.
		p = buffers[i];
		COM_BeginParseSession(filename);
		while (1)
		{
			token = COM_ParseExt(&p, qtrue);

			if (!*token)
				break;

			Q_strncpyz(shaderName, token, sizeof(shaderName));
			shaderLine = COM_GetCurrentParseLine();

			if (token[0] == '#')
			{
				vk_debug("WARNING: Deprecated shader comment \"%s\" on line %d in file %s.  Ignoring line.\n",
					shaderName, shaderLine, filename);
				SkipRestOfLine(&p);
				continue;
			}

			token = COM_ParseExt(&p, qtrue);
			if (token[0] != '{' || token[1] != '\0')
			{
				vk_debug("WARNING: Ignoring shader file %s. Shader \"%s\" on line %d missing opening brace",
					filename, shaderName, shaderLine);
				if (token[0])
				{
					vk_debug(" (found \"%s\" on line %d)", token, COM_GetCurrentParseLine());
				}
				vk_debug(".\n");
				ri.FS_FreeFile(buffers[i]);
				buffers[i] = NULL;
				break;
			}

			if (!SkipBracedSection(&p, 1))
			{
				vk_debug("WARNING: Ignoring shader file %s. Shader \"%s\" on line %d missing closing brace.\n",
					filename, shaderName, shaderLine);
				ri.FS_FreeFile(buffers[i]);
				buffers[i] = NULL;
				break;
			}
		}


		if (buffers[i])
			sum += summand;
	}

	// build single large buffer
	s_shaderText = (char*)ri.Hunk_Alloc(sum + numShaderFiles * 2, h_low);
	s_shaderText[0] = '\0';
	textEnd = s_shaderText;

	// free in reverse order, so the temp files are all dumped
	for (i = numShaderFiles - 1; i >= 0; i--)
	{
		if (!buffers[i])
			continue;

		strcat(textEnd, buffers[i]);
		strcat(textEnd, "\n");
		textEnd += strlen(textEnd);
		ri.FS_FreeFile(buffers[i]);
	}

	COM_CompressShader(s_shaderText);

	// free up memory
	ri.FS_FreeFileList(shaderFiles);

	memset(shaderTextHashTableSizes, 0, sizeof(shaderTextHashTableSizes));
	size = 0;

	p = s_shaderText;
	// look for shader names
	while (1) {
		token = COM_ParseExt(&p, qtrue);
		if (token[0] == 0) {
			break;
		}

		if (token[0] == '#')
		{
			SkipRestOfLine(&p);
			continue;
		}

		hash = generateHashValue(token, MAX_SHADERTEXT_HASH);
		shaderTextHashTableSizes[hash]++;
		size++;
		SkipBracedSection(&p, 0);
	}

	size += MAX_SHADERTEXT_HASH;

	hashMem = (char*)ri.Hunk_Alloc(size * sizeof(char*), h_low);

	for (i = 0; i < MAX_SHADERTEXT_HASH; i++) {
		shaderTextHashTable[i] = (const char**)hashMem;
		hashMem = ((char*)hashMem) + ((shaderTextHashTableSizes[i] + 1) * sizeof(char*));
	}

	memset(shaderTextHashTableSizes, 0, sizeof(shaderTextHashTableSizes));

	p = s_shaderText;
	// look for shader names
	while (1) {
		oldp = (char*)p;
		token = COM_ParseExt(&p, qtrue);
		if (token[0] == 0) {
			break;
		}

		if (token[0] == '#')
		{
			SkipRestOfLine(&p);
			continue;
		}

		hash = generateHashValue(token, MAX_SHADERTEXT_HASH);
		shaderTextHashTable[hash][shaderTextHashTableSizes[hash]++] = oldp;

		SkipBracedSection(&p, 0);
	}

	return;
}

shader_t *R_CreateShaderFromTextureBundle( const char *name, const textureBundle_t *bundle, uint32_t stateBits )
{
	shader_t *result = R_FindShaderByName(name);
	if ( result == tr.defaultShader )
	{
		Com_Memset(&shader, 0, sizeof(shader));
		Com_Memset(&stages, 0, sizeof(stages));

		Q_strncpyz(shader.name, name, sizeof(shader.name));

		stages[0].active = qtrue;
		stages[0].bundle[0] = *bundle;
		stages[0].stateBits = stateBits;
		result = FinishShader();
	}
	return result;
}

/*
===============
InitShader
===============
*/
static void InitShader( const char *name, const int *lightmapIndex, const byte *styles ) {
	int i;

	// clear the global shader
	Com_Memset(&shader, 0, sizeof(shader));
	Com_Memset(&stages, 0, sizeof(stages));

	Q_strncpyz(shader.name, name, sizeof(shader.name));
	//shader->lightmapIndex = lightmapIndex;

	Com_Memcpy(shader.lightmapIndex, lightmapIndex, sizeof(shader.lightmapIndex));
	Com_Memcpy(shader.styles, styles, sizeof(shader.styles));

	// we need to know original (unmodified) lightmap index
	// because shader search functions expects this
	// otherwise they will fail and cause massive duplication
	Com_Memcpy(shader.lightmapSearchIndex, shader.lightmapIndex, sizeof(shader.lightmapSearchIndex));

	for (i = 0; i < MAX_SHADER_STAGES; i++) {
		stages[i].bundle[0].texMods = texMods[i];
		stages[i].bundle[0].mGLFogColorOverride = GLFOGOVERRIDE_NONE;
	}

	shader.contentFlags = CONTENTS_SOLID | CONTENTS_OPAQUE;

#ifdef USE_VBO_SS
	shader.surface_sprites.num_stages = 0;
	shader.surface_sprites.ssbo_index = ~0U;
#endif
}

/*
 ===============
 R_FindLightmap ( needed for -external LMs created by ydnar's q3map2 )
 given a (potentially erroneous) lightmap index, attempts to load
 an external lightmap image and/or sets the index to a valid number
 ===============
 */
#define EXTERNAL_LIGHTMAP     "lm_%04d.tga"     // THIS MUST BE IN SYNC WITH Q3MAP2
static inline const int *R_FindLightmap( const int *lightmapIndex )
{
	// don't bother with vertex lighting
	if ( *lightmapIndex < 0 )
		return lightmapIndex;

	// do the lightmaps exist?
	for ( int i = 0; i < MAXLIGHTMAPS; i++ )
	{
		if ( lightmapIndex[i] >= tr.numLightmaps || tr.lightmaps[lightmapIndex[i]] == NULL )
			return lightmapsVertex;
	}

	return lightmapIndex;
}

shader_t *R_FindShader( const char *name, const int *lightmapIndex, const byte *styles, qboolean mipRawImage )
{
	char		strippedName[MAX_QPATH];
	int			hash;
	const char	*shaderText;
	image_t		*image;
	shader_t	*sh;
	
	if (name[0] == '\0') {
		return tr.defaultShader;
	}

	if ( lightmapIndex[0] >= 0 && lightmapIndex[0] >= tr.numLightmaps ) {
		lightmapIndex = lightmapsVertex;
	} else if (lightmapIndex[0] < LIGHTMAP_2D)
	{
		// negative lightmap indexes cause stray pointers (think tr.lightmaps[lightmapIndex])
		vk_debug("WARNING: shader '%s' has invalid lightmap index of %d\n", name, lightmapIndex[0]);
		lightmapIndex = lightmapsVertex;
	}
	
	lightmapIndex = R_FindLightmap(lightmapIndex);
	
	COM_StripExtension(name, strippedName, sizeof(strippedName));

	hash = generateHashValue(strippedName, FILE_HASH_SIZE);

	sh = hashTable[hash];
	while (sh)
	{
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if (IsShader(sh, strippedName, lightmapIndex, styles))
		{
			return sh;
		}

		sh = sh->next;
	}

	// clear the global shader
	InitShader(strippedName, lightmapIndex, styles);

	//
	// attempt to define shader from an explicit parameter file
	//
	shaderText = FindShaderInShaderText(strippedName);
	
	if (shaderText) {
		//vk_debug("shader [%s] pre load\n", strippedName);

		if (!ParseShader(&shaderText)) {
			// had errors, so use default shader
			setDefaultShader();
		}
		sh = FinishShader();

		//vk_debug("shader [%s] post load\n", strippedName);
		return sh;
	}


	//
	// if not defined in the in-memory shader descriptions,
	// look for a single TGA, BMP, or PCX
	//
	{
		imgFlags_t flags;

		flags = IMGFLAG_NONE;

		if (mipRawImage)
		{
			flags |= IMGFLAG_MIPMAP | IMGFLAG_PICMIP;
		}
		else
		{
			flags |= IMGFLAG_CLAMPTOEDGE;
		}

		image = R_FindImageFile(strippedName, flags);
		if (!image) {
			vk_debug("shader [%s] image not found, fallback to default shader\n", name);
			setDefaultShader();
			return FinishShader();
		}
	}

	R_CreateDefaultShadingCmds(image);

	return FinishShader();
}

shader_t *R_FindServerShader( const char *name, const int *lightmapIndex, const byte *styles, qboolean mipRawImage )
{
	char		strippedName[MAX_QPATH];
	int			hash;
	shader_t	*sh;

	vk_debug("find server shader \n");

	if (name[0] == 0) {
		return tr.defaultShader;
	}

	COM_StripExtension(name, strippedName, sizeof(strippedName));

	hash = generateHashValue(strippedName, FILE_HASH_SIZE);

	sh = hashTable[hash];
	while (sh)
	{
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if (IsShader(sh, strippedName, lightmapIndex, styles))
		{
			return sh;
		}

		sh = sh->next;
	}

	InitShader(strippedName, lightmapIndex, styles);

	setDefaultShader();

	return FinishShader();
}

/*
========================================================================================

SHADER OPTIMIZATION AND FOGGING

========================================================================================
*/

typedef struct {
	int		blendA;
	int		blendB;

	int		multitextureEnv;
	int		multitextureBlend;
} collapse_t;

static const collapse_t collapse[] = {
	{ 0, GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO,
		GL_MODULATE, 0 },

	{ 0, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR,
		GL_MODULATE, 0 },

	{ GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR,
		GL_MODULATE, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR },

	{ GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR,
		GL_MODULATE, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR },

	{ GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR, GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO,
		GL_MODULATE, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR },

	{ GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO, GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO,
		GL_MODULATE, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR },

	{ 0, GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE,
		GL_ADD, 0 },

	{ GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE, GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE,
		GL_ADD, GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE },

	{ GLS_DSTBLEND_ONE | GLS_SRCBLEND_SRC_ALPHA, GLS_DSTBLEND_ONE | GLS_SRCBLEND_SRC_ALPHA,
		GL_BLEND_ALPHA, GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE},

	{ GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA, GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA,
		GL_BLEND_ONE_MINUS_ALPHA, GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE},

	{ 0, GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_SRCBLEND_SRC_ALPHA,
		GL_BLEND_MIX_ALPHA, 0},

	{ 0, GLS_DSTBLEND_SRC_ALPHA | GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA,
		GL_BLEND_MIX_ONE_MINUS_ALPHA, 0},

	{ 0, GLS_DSTBLEND_SRC_ALPHA | GLS_SRCBLEND_DST_COLOR,
		GL_BLEND_DST_COLOR_SRC_ALPHA, 0},

#if 0
	{ 0, GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_SRCBLEND_SRC_ALPHA,
		GL_DECAL, 0 },
#endif
	{ -1 }
};

/*
================
CollapseMultitexture

Attempt to combine two stages into a single multitexture stage
FIXME: I think modulated add + modulated add collapses incorrectly
=================
*/
static int CollapseMultitexture( unsigned int st0bits, shaderStage_t *st0, shaderStage_t *st1, int num_stages, qboolean firstStage ) {
	int abits, bbits;
	int i, mtEnv;
	textureBundle_t tmpBundle;
	qboolean nonIdenticalColors;
	qboolean swapLightmap;

	// make sure both stages are active
	if (!st0->active || !st1->active) {
		return 0;
	}

	// do not collapse surface sprite stages
	if ( st0->ss || st1->ss )
		return 0;

	if (st0->depthFragment || (st0->stateBits & GLS_ATEST_BITS)) {
		return 0;
	}

	abits = st0bits; // st0->stateBits;
	bbits = st1->stateBits;

	// make sure that both stages have identical state other than blend modes
	if ((abits & ~(GLS_BLEND_BITS | GLS_DEPTHMASK_TRUE)) !=
		(bbits & ~(GLS_BLEND_BITS | GLS_DEPTHMASK_TRUE))) {
		return 0;
	}

	abits &= GLS_BLEND_BITS;
	bbits &= GLS_BLEND_BITS;

	// search for a valid multitexture blend function
	for (i = 0; collapse[i].blendA != -1; i++) {
		if (abits == collapse[i].blendA && bbits == collapse[i].blendB) {
			break;
		}
	}

	// nothing found
	if (collapse[i].blendA == -1) {
		return 0;
	}

	mtEnv = collapse[i].multitextureEnv;

	// GL_ADD is a separate extension
	if ( mtEnv == GL_ADD && !glConfig.textureEnvAddAvailable ) {
		return 0;
	}

	if (mtEnv == GL_ADD && st0->bundle[0].rgbGen != CGEN_IDENTITY) {
		mtEnv = GL_ADD_NONIDENTITY;
	}

	if (st0->mtEnv && st0->mtEnv != mtEnv) {
		// we don't support different blend modes in 3x mode, yet
		return 0;
	}

	nonIdenticalColors = qfalse;

	// make sure waveforms have identical parameters
	if ((st0->bundle[0].rgbGen != st1->bundle[0].rgbGen) || (st0->bundle[0].alphaGen != st1->bundle[0].alphaGen)) {
		nonIdenticalColors = qtrue;
	}

	if (st0->bundle[0].rgbGen == CGEN_WAVEFORM)
	{
		if (memcmp(&st0->bundle[0].rgbWave, &st1->bundle[0].rgbWave, sizeof(stages[0].bundle[0].rgbWave)))
		{
			nonIdenticalColors = qtrue;
		}
	}

	if (st0->bundle[0].alphaGen == AGEN_WAVEFORM)
	{
		if (memcmp(&st0->bundle[0].alphaWave, &st1->bundle[0].alphaWave, sizeof(stages[0].bundle[0].alphaWave)))
		{
			nonIdenticalColors = qtrue;
		}
	}

	// in case this makes sense: ..
	// vanilla only collapses first 2 stages and skips non matching color gen ..
	// so, if the first shader stage is not glow but the second stage is and has matching color gen, disable glow stage
	if ( !r_DynamicGlowAllStages->integer 
		&& firstStage && !st0->mtEnv && !nonIdenticalColors && !st0->glow && st1->glow )
	{
		st1->glow = st1->bundle[0].glow = false;
	}

	if (nonIdenticalColors)
	{
		switch (mtEnv)
		{
		case GL_ADD:
		case GL_ADD_NONIDENTITY: mtEnv = GL_BLEND_ADD; break;
		case GL_MODULATE: mtEnv = GL_BLEND_MODULATE; break;
		}
	}

	switch (mtEnv) {
	case GL_MODULATE:
	case GL_ADD:
		swapLightmap = qtrue;
		break;
	default:
		swapLightmap = qfalse;
		break;
	}

	// make sure that lightmaps are in bundle 1
	if (swapLightmap && st0->bundle[0].isLightmap && !st0->mtEnv)
	{
		tmpBundle = st0->bundle[0];
		st0->bundle[0] = st1->bundle[0];
		st0->bundle[1] = tmpBundle;
	}
	else
	{
		if (st0->mtEnv)
			st0->bundle[2] = st1->bundle[0]; // add to third bundle
		else
			st0->bundle[1] = st1->bundle[0];
	}

	// use +cl blend shader for multi-lightmap stage
	if (st0->bundle[0].isLightmap && st1->bundle[0].isLightmap)
	{
		mtEnv = GL_BLEND_ADD;
	}

	// preserve lightmap style
	if (st1->lightmapStyle)
	{
		st0->lightmapStyle[1] = st1->lightmapStyle[0];
	}

	if (st0->mtEnv)
	{
		st0->mtEnv3 = mtEnv;
	}
	else
	{
		// set the new blend state bits
		st0->stateBits &= ~GLS_BLEND_BITS;
		st0->stateBits |= collapse[i].multitextureBlend;

		st0->mtEnv = mtEnv;
		shader.multitextureEnv = qtrue;
	}

	st0->numTexBundles++;

	if( st1->glow )
		st0->glow = true;

	//
	// move down subsequent shaders
	//
	if (num_stages > 2)
	{
		memmove(st1, st1 + 1, sizeof(stages[0]) * (num_stages - 2));
	}

	Com_Memset(st0 + num_stages - 1, 0, sizeof(stages[0]));

	if (vk.maxBoundDescriptorSets >= 8 && num_stages >= 3 && !st0->mtEnv3)
	{
		if (mtEnv == GL_BLEND_ONE_MINUS_ALPHA || mtEnv == GL_BLEND_ALPHA || mtEnv == GL_BLEND_MIX_ALPHA || mtEnv == GL_BLEND_MIX_ONE_MINUS_ALPHA || mtEnv == GL_BLEND_DST_COLOR_SRC_ALPHA)
		{
			// pass original state bits so recursive detection will work for these shaders
			return 1 + CollapseMultitexture(st0bits, st0, st1, num_stages - 1, firstStage);
		}
		if (abits == 0)
		{
			return 1 + CollapseMultitexture(st0->stateBits, st0, st1, num_stages - 1, firstStage);
		}
	}

	return 1;
}

#ifdef USE_PMLIGHT
static int tcmodWeight2( const shaderStage_t* st )
{
	int i;

	for ( i = 0; i < st->bundle[0].numTexMods; i++ ) {
		switch ( st->bundle[0].texMods[i].type ) {
		case TMOD_NONE:
		case TMOD_SCALE:
		case TMOD_TRANSFORM:
			break;
		default:
			return 0;
		}
	}
	return 1;
}


/*
====================
FindLightingStage

Find proper stage for dlight pass.
Peforfm it before multitexture collapse for simplification and to preserve all info (e.g. isDetail)

Key complex shaders to validate/check:
[q3dm17]
* textures/sfx/diamond2cjumppad -> stage #0
* textures/sfx/launchpad_diamond -> stage #1
* textures/base_floor/diamond2c_ow -> stage #1
[q3wcp17]
* textures/scanctf2/bounce_white -> stage #0
[lun3dm5]
* textures/lun3dm5/c_crete6gs -> stage #1
* textures/lun3dm5/c_crete6j -> stage #4
[pom]
* textures/sockter/ter_mossgravel -> stage #1
====================
*/
static void FindLightingStage( const int stage ) {
	int i, selected, lightmap;

	shader.lightingBundle = 0;
	shader.lightingStage = -1;

	if ( shader.isSky || (shader.surfaceFlags & (SURF_NODLIGHT | SURF_SKY)) /* || shader.sort == SS_ENVIRONMENT || shader.sort >= SS_FOG */ ) {
		return;
	}

	selected = -2;
	lightmap = -2;
	for ( i = 0; i < stage; i++ ) {
		const shaderStage_t *st = &stages[i];
		const textureBundle_t *b = &st->bundle[0];
		if ( !st->active ) {
			break;
		}
		if ( b->isLightmap ) {
			// 1. prefer stages near lightmap
			if ( selected == i - 1 ) {
				break;
			}
			lightmap = i;
			continue;
		}
		if ( b->image[0] == tr.whiteImage || b->tcGen != TCGEN_TEXTURE ) {
			continue;
		}
		if ( selected >= 0 ) {
			// 2. skip detail textures
			if ( st->isDetail ) {
				continue;
			}
			// 3. prefer non-animated stages
			if ( stages[selected].bundle[0].numImageAnimations < b->numImageAnimations ) {
				continue;
			}
			// 4. prefer static tcgens
			if ( tcmodWeight2( &stages[selected] ) > tcmodWeight2( st ) ) {
				continue;
			}
			// 5. special case for lun3dm5 crete6gs stage #2
			if ( ( st->stateBits & GLS_BLEND_BITS ) == ( GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_SRC_COLOR ) ) {
				if ( ( stages[selected].stateBits & GLS_BLEND_BITS ) == ( GLS_SRCBLEND_ONE | GLS_DSTBLEND_SRC_ALPHA ) ) {
					continue;
				}
			}
			// 6. special case for q3w8 bounce_red_v/bounce_blue_v
			if ( ( st->stateBits == ( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE ) ) ) {
				if ( stages[selected].stateBits == ( GLS_DEPTHMASK_TRUE | GLS_ATEST_GE_80 ) ) {
					break;
				}
			}
		}
		selected = i;
		// 1. prefer stages near lightmap
		if ( i == lightmap + 1 ) {
			break;
		}
	}

	if ( selected >= 0 ) {
		shader.lightingStage = selected;
		stages[selected].bundle[0].dlight = 1;
	}
}


/*
====================
FindLightingStage

Set shader.lightingStage and shader.lightingBundle depending from marked .dlight field
====================
*/
static void FindLightingBundle( void )
{
	int i, n;

	if ( shader.lightingStage < 0 ) {
		return;
	}

	shader.lightingStage = -1;

	if ( /*shader.isSky || (shader.surfaceFlags & (SURF_SKY)) || */ shader.sort == SS_ENVIRONMENT || shader.sort >= SS_FOG ) {
		return;
	}

	for ( i = 0; i < shader.numUnfoggedPasses; i++ ) {
		const shaderStage_t* st = &stages[i];
		if ( !st->active ) {
			break;
		}
		for ( n = 0; n < st->numTexBundles; n++ ) {
			if ( st->bundle[n].dlight ) {
				shader.lightingStage = i;
				shader.lightingBundle = n;
			}
		}
	}
}
#endif // USE_PMLIGHT

/*
=================
VertexLightingCollapse

If vertex lighting is enabled, only render a single
pass, trying to guess which is the correct one to best aproximate
what it is supposed to look like.
=================
*/
#if 0
static void VertexLightingCollapse( void )
{
	int				stage;
	shaderStage_t	*bestStage, *pStage;
	int				bestImageRank;
	int				rank;
	qboolean		vertexColors;

	// if we aren't opaque, just use the first pass
	if (shader.sort == SS_OPAQUE) {

		// pick the best texture for the single pass
		bestStage = &stages[0];
		bestImageRank = -999999;
		vertexColors = qfalse;

		for (stage = 0; stage < MAX_SHADER_STAGES; stage++) {
			pStage = &stages[stage];

			if (!pStage->active) {
				break;
			}
			rank = 0;

			if (pStage->bundle[0].isLightmap) {
				rank -= 100;
			}
			if (pStage->bundle[0].tcGen != TCGEN_TEXTURE) {
				rank -= 5;
			}
			if (pStage->bundle[0].numTexMods) {
				rank -= 5;
			}
			if (pStage->bundle[0].rgbGen != CGEN_IDENTITY && pStage->bundle[0].rgbGen != CGEN_IDENTITY_LIGHTING) {
				rank -= 3;
			}

			if (rank > bestImageRank) {
				bestImageRank = rank;
				bestStage = pStage;
			}

			// detect missing vertex colors on ojfc-17 for green/dark pink flags
			if (pStage->bundle[0].rgbGen != CGEN_IDENTITY || pStage->bundle[0].tcGen == TCGEN_LIGHTMAP || pStage->stateBits & GLS_ATEST_BITS) {
				vertexColors = qtrue;
			}
		}

		stages[0].bundle[0] = bestStage->bundle[0];
		stages[0].stateBits &= ~(GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS);
		stages[0].stateBits |= GLS_DEPTHMASK_TRUE;
		if (shader.lightmapIndex[0] == LIGHTMAP_NONE) { // probably crashes
			stages[0].bundle[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
		}
		else {
			if (vertexColors) {
				stages[0].bundle[0].rgbGen = CGEN_EXACT_VERTEX;
			}
			else {
				stages[0].bundle[0].rgbGen = CGEN_IDENTITY_LIGHTING;
			}
		}
		stages[0].bundle[0].alphaGen = AGEN_SKIP;
	}
	else {
		// don't use a lightmap (tesla coils)
		if (stages[0].bundle[0].isLightmap) {
			stages[0] = stages[1];
		}

		// if we were in a cross-fade cgen, hack it to normal
		if (stages[0].bundle[0].rgbGen == CGEN_ONE_MINUS_ENTITY || stages[1].bundle[0].rgbGen == CGEN_ONE_MINUS_ENTITY) {
			stages[0].bundle[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
		if ((stages[0].bundle[0].rgbGen == CGEN_WAVEFORM && stages[0].bundle[0].rgbWave.func == GF_SAWTOOTH)
			&& (stages[1].bundle[0].rgbGen == CGEN_WAVEFORM && stages[1].bundle[0].rgbWave.func == GF_INVERSE_SAWTOOTH)) {
			stages[0].bundle[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
		if ((stages[0].bundle[0].rgbGen == CGEN_WAVEFORM && stages[0].bundle[0].rgbWave.func == GF_INVERSE_SAWTOOTH)
			&& (stages[1].bundle[0].rgbGen == CGEN_WAVEFORM && stages[1].bundle[0].rgbWave.func == GF_SAWTOOTH)) {
			stages[0].bundle[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
	}

	for (stage = 1; stage < MAX_SHADER_STAGES; stage++) {
		pStage = &stages[stage];

		if (!pStage->active) {
			break;
		}

		Com_Memset(pStage, 0, sizeof(*pStage));
	}
}
#endif

static qboolean EqualACgen( const shaderStage_t *st1, const shaderStage_t *st2 )
{
	if (st1 == NULL || st2 == NULL) {
		return qfalse;
	}

	if (st1->bundle[0].adjustColorsForFog != st2->bundle[0].adjustColorsForFog) {
		return qfalse;
	}

	return qtrue;
}

static qboolean EqualRGBgen( const shaderStage_t *st1, const shaderStage_t *st2 )
{
	if (st1 == NULL || st2 == NULL) {
		return qfalse;
	}

	if (st1->bundle[0].rgbGen != st2->bundle[0].rgbGen || st1->active != st2->active) {
		return qfalse;
	}

	if (st1->bundle[0].rgbGen == CGEN_CONST) {
		if (memcmp(st1->bundle[0].constantColor, st2->bundle[0].constantColor, 4) != 0) {
			return qfalse;
		}
	}

	if (st1->bundle[0].rgbGen == CGEN_WAVEFORM) {
		if (memcmp(&st1->bundle[0].rgbWave, &st2->bundle[0].rgbWave, sizeof(st1->bundle[0].rgbWave)) != 0) {
			return qfalse;
		}
	}

	if (st1->bundle[0].alphaGen != st2->bundle[0].alphaGen) {
		return qfalse;
	}

	if (st1->bundle[0].alphaGen == AGEN_CONST) {
		if (st1->bundle[0].rgbGen != CGEN_CONST) {
			if (st1->bundle[0].constantColor[3] != st2->bundle[0].constantColor[3]) {
				return qfalse;
			}
		}
	}

	if (st1->bundle[0].alphaGen == AGEN_WAVEFORM) {
		if (memcmp(&st1->bundle[0].alphaWave, &st2->bundle[0].alphaWave, sizeof(st1->bundle[0].alphaWave)) != 0) {
			return qfalse;
		}
	}

	return qtrue;
}

static qboolean EqualTCgen( int bundle, const shaderStage_t *st1, const shaderStage_t *st2 )
{
	const textureBundle_t* b1, * b2;
	const texModInfo_t* tm1, * tm2;
	int tm;

	if (st1 == NULL || st2 == NULL)
		return qfalse;

	if (st1->active != st2->active)
		return qfalse;

	b1 = &st1->bundle[bundle];
	b2 = &st2->bundle[bundle];

	if (b1->tcGen != b2->tcGen) {
		return qfalse;
	}

	if (b1->tcGen == TCGEN_VECTOR) {
		if (memcmp(b1->tcGenVectors, b2->tcGenVectors, sizeof(*b1->tcGenVectors) * 2) != 0) {
			return qfalse;
		}
	}

	//if ( b1->tcGen == TCGEN_ENVIRONMENT_MAPPED_FP ) {
	//	if ( b1->isScreenMap != b2->isScreenMap ) {
	//		return qfalse;
	//	}
	//}

	//if (b1->tcGen != TCGEN_LIGHTMAP && b1->isLightmap != b2->isLightmap && r_mergeLightmaps->integer) {
	//	return qfalse;
	//}

	if (b1->numTexMods != b2->numTexMods) {
		return qfalse;
	}

	for (tm = 0; tm < b1->numTexMods; tm++) {
		tm1 = &b1->texMods[tm];
		tm2 = &b2->texMods[tm];
		if (tm1->type != tm2->type) {
			return qfalse;
		}

		if (tm1->type == TMOD_TURBULENT || tm1->type == TMOD_STRETCH) {
			if (memcmp(&tm1->wave, &tm2->wave, sizeof(tm1->wave)) != 0) {
				return qfalse;
			}
			continue;
		}

		if (tm1->type == TMOD_SCROLL) {
			if (memcmp(tm1->translate, tm2->translate, sizeof(tm1->translate)) != 0) {
				return qfalse;
			}
			continue;
		}

		if (tm1->type == TMOD_SCALE) {
			if (memcmp(tm1->translate, tm2->translate, sizeof(tm1->translate)) != 0) {
				return qfalse;
			}
			continue;
		}

		if (tm1->type == TMOD_TRANSFORM) {
			if (memcmp(tm1->matrix, tm2->matrix, sizeof(tm1->matrix)) != 0) {
				return qfalse;
			}
			if (memcmp(tm1->translate, tm2->translate, sizeof(tm1->translate)) != 0) {
				return qfalse;
			}
			continue;
		}

		//if (tm1->type == TMOD_ROTATE && tm1->rotateSpeed != tm2->rotateSpeed) {
		//	return qfalse;
		//}
	}

	return qtrue;
}

/*
===================
ComputeStageIteratorFunc

See if we can use on of the simple fastpath stage functions,
otherwise set to the generic stage function
===================
*/
static void ComputeStageIteratorFunc( void )
{
	//
	// see if this should go into the sky path
	//
	if (shader.isSky)
	{
		shader.optimalStageIteratorFunc = RB_StageIteratorSky;
	}
	else
	{
		shader.optimalStageIteratorFunc = RB_StageIteratorGeneric;
	}
}

/*
=========================
FinishShader

Returns a freshly allocated shader with all the needed info
from the current global working shader
=========================
*/
shader_t *FinishShader( void )
{
	qboolean		hasLightmapStage;
	int				stage, i, n, m, lmStage, numStyles;
	qboolean		fogCollapse;
	shaderStage_t	*lastStage[NUM_TEXTURE_BUNDLES];

	hasLightmapStage = qfalse;
	fogCollapse = qfalse;

	//
	// set sky stuff appropriate
	//
	if (shader.isSky) {
		shader.sort = SS_ENVIRONMENT;
	}

	//
	// set polygon offset
	//
	if (shader.polygonOffset && shader.sort == SS_BAD) {
		shader.sort = SS_DECAL;
	}

	//
	// set lightmap stage
	//
	for( lmStage = 0; lmStage < MAX_SHADER_STAGES; lmStage++ )
	{
		shaderStage_t *pStage = &stages[lmStage];

		if ( pStage->active && pStage->bundle[0].isLightmap )
			break;
	}

	if ( lmStage < MAX_SHADER_STAGES )
	{
		if ( shader.lightmapIndex[0] == LIGHTMAP_BY_VERTEX )
		{
			if ( lmStage == 0 )	//< MAX_SHADER_STAGES-1)
			{//copy the rest down over the lightmap slot
				memmove(&stages[lmStage], &stages[lmStage+1], sizeof(shaderStage_t) * ( MAX_SHADER_STAGES - lmStage - 1 ));
				memset(&stages[MAX_SHADER_STAGES-1], 0, sizeof(shaderStage_t));
				//change blending on the moved down stage
				stages[lmStage].stateBits = GLS_DEFAULT;
			}
			//change anything that was moved down (or the *white if LM is first) to use vertex color
			stages[lmStage].bundle[0].rgbGen = CGEN_EXACT_VERTEX;
			stages[lmStage].bundle[0].alphaGen = AGEN_SKIP;
			lmStage = MAX_SHADER_STAGES;	//skip the style checking below
		}
	}

	if ( lmStage < MAX_SHADER_STAGES )// && !r_fullbright->value)
	{
		for( numStyles =0 ; numStyles < MAXLIGHTMAPS; numStyles++ ) {
			if ( shader.styles[numStyles] >= LS_UNUSED )
				break;
		}

		numStyles--;

		if ( numStyles > 0 )
		{
			for( i = MAX_SHADER_STAGES - 1; i > lmStage+numStyles; i-- )
				stages[i] = stages[i-numStyles];

			for( i = 0; i <numStyles; i++ )
			{
				stages[lmStage+i+1] = stages[lmStage];
				
				shaderStage_t *pStage = &stages[lmStage+i+1];

				if ( shader.lightmapIndex[ i + 1 ] == LIGHTMAP_BY_VERTEX ) {
					pStage->bundle[0].image[0] = tr.whiteImage;
				}
				else if ( shader.lightmapIndex[ i + 1 ] < 0 ) {
					Com_Error( ERR_DROP, "FinishShader: light style with no light map or vertex color for shader %s", shader.name);
				}
				else {
					pStage->bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex[i+1]];
					pStage->bundle[0].tcGen = (texCoordGen_t)( TCGEN_LIGHTMAP + i + 1 );
				}

				pStage->bundle[0].rgbGen = CGEN_LIGHTMAPSTYLE;
				pStage->stateBits &= ~(GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS);
				pStage->stateBits |= GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE;
			}
		}

		for( i = 0; i <= numStyles; i++ )
			stages[lmStage+i].lightmapStyle[0] = shader.styles[i];
	}

	//
	// set appropriate stage information
	//
	for (stage = 0; stage < MAX_SHADER_STAGES;)
	{
		shaderStage_t* pStage = &stages[stage];

		if (!pStage->active) {
			break;
		}

		// check for a missing texture
		if (!pStage->bundle[0].image[0])
		{
			vk_debug("Shader %s has a stage with no image\n", shader.name);
			pStage->active = qfalse;
			stage++;
			continue;
		}

		//
		// ditch this stage if it's detail and detail textures are disabled
		//
		if (pStage->isDetail && !r_detailTextures->integer)
		{
			int index;

			for (index = stage + 1; index < MAX_SHADER_STAGES; index++)
			{
				if (!stages[index].active)
					break;
			}

			if (index < MAX_SHADER_STAGES)
				memmove(pStage, pStage + 1, sizeof(*pStage) * (index - stage));
			else
			{
				if (stage + 1 < MAX_SHADER_STAGES)
					memmove(pStage, pStage + 1, sizeof(*pStage) * (index - stage - 1));

				Com_Memset(&stages[index - 1], 0, sizeof(*stages));
			}

			continue;
		}

		//
		// default texture coordinate generation
		//
		if (pStage->bundle[0].isLightmap) {
			if (pStage->bundle[0].tcGen == TCGEN_BAD) {
				pStage->bundle[0].tcGen = TCGEN_LIGHTMAP;
			}
			hasLightmapStage = qtrue;
		}
		else {
			if (pStage->bundle[0].tcGen == TCGEN_BAD) {
				pStage->bundle[0].tcGen = TCGEN_TEXTURE;
			}
		}

		// not a true lightmap but we want to leave existing
		// behaviour in place and not print out a warning
		//if (pStage->rgbGen == CGEN_VERTEX) {
		//  vertexLightmap = qtrue;
		//}

		//
		// determine sort order and fog color adjustment
		//
		if ((pStage->stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) &&
			(stages[0].stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS))) {
			int blendSrcBits = pStage->stateBits & GLS_SRCBLEND_BITS;
			int blendDstBits = pStage->stateBits & GLS_DSTBLEND_BITS;

			// fog color adjustment only works for blend modes that have a contribution
			// that aproaches 0 as the modulate values aproach 0 --
			// GL_ONE, GL_ONE
			// GL_ZERO, GL_ONE_MINUS_SRC_COLOR
			// GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA

			// modulate, additive
			if (((blendSrcBits == GLS_SRCBLEND_ONE) && (blendDstBits == GLS_DSTBLEND_ONE)) ||
				((blendSrcBits == GLS_SRCBLEND_ZERO) && (blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_COLOR))) {
				pStage->bundle[0].adjustColorsForFog = ACFF_MODULATE_RGB;
			}
			// strict blend
			else if ((blendSrcBits == GLS_SRCBLEND_SRC_ALPHA) && (blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA))
			{
				pStage->bundle[0].adjustColorsForFog = ACFF_MODULATE_ALPHA;
			}
			// premultiplied alpha
			else if ((blendSrcBits == GLS_SRCBLEND_ONE) && (blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA))
			{
				pStage->bundle[0].adjustColorsForFog = ACFF_MODULATE_RGBA;
			}
			else {
				// we can't adjust this one correctly, so it won't be exactly correct in fog
			}

			// don't screw with sort order if this is a portal or environment
			if ( !shader.sort ) {
				// see through item, like a grill or grate
				if ( pStage->stateBits & GLS_DEPTHMASK_TRUE ) {
					shader.sort = SS_SEE_THROUGH;
				} else {
					if (( blendSrcBits == GLS_SRCBLEND_ONE ) && ( blendDstBits == GLS_DSTBLEND_ONE ))
					{
						// GL_ONE GL_ONE needs to come a bit later
						shader.sort = SS_BLEND1;
					}
					else
					{
						shader.sort = SS_BLEND0;
					}
				}
			}
		}

		stage++;
	}

	// there are times when you will need to manually apply a sort to
	// opaque alpha tested shaders that have later blend passes
	if ( !shader.sort ) {
		shader.sort = SS_OPAQUE;
	}

	// fix alphaGen flags to avoid redundant comparisons in R_ComputeColors()
	for (i = 0; i < MAX_SHADER_STAGES; i++) {
		shaderStage_t* pStage = &stages[i];
		if (!pStage->active)
			break;
		if (pStage->bundle[0].rgbGen == CGEN_IDENTITY && pStage->bundle[0].alphaGen == AGEN_IDENTITY)
			pStage->bundle[0].alphaGen = AGEN_SKIP;
		else if (pStage->bundle[0].rgbGen == CGEN_CONST && pStage->bundle[0].alphaGen == AGEN_CONST)
			pStage->bundle[0].alphaGen = AGEN_SKIP;
		else if (pStage->bundle[0].rgbGen == CGEN_VERTEX && pStage->bundle[0].alphaGen == AGEN_VERTEX)
			pStage->bundle[0].alphaGen = AGEN_SKIP;
	}

	//
	// if we are in r_vertexLight mode, never use a lightmap texture
	//
	if (stage > 1 && (r_vertexLight->integer && !r_uiFullScreen->integer)) {
		//VertexLightingCollapse();
		//stage = 1;
		//rww - since this does bad things, I am commenting it out for now. If you want to attempt a fix, feel free.
		hasLightmapStage = qfalse;
	}

	// whiteimage + "filter" texture == texture
	if (stage > 1 && stages[0].bundle[0].image[0] == tr.whiteImage && stages[0].bundle[0].numImageAnimations <= 1 && stages[0].bundle[0].rgbGen == CGEN_IDENTITY && stages[0].bundle[0].alphaGen == AGEN_SKIP) {
		if (stages[1].stateBits == (GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO)) {
			stages[1].stateBits = stages[0].stateBits & (GLS_DEPTHMASK_TRUE | GLS_DEPTHTEST_DISABLE | GLS_DEPTHFUNC_EQUAL);
			memmove(&stages[0], &stages[1], sizeof(stages[0]) * (stage - 1));
			stages[stage - 1].active = qfalse;
			stage--;
		}
	}

	for (i = 0; i < stage; i++) {
		stages[i].numTexBundles = 1;
	}

#ifdef USE_PMLIGHT
	FindLightingStage( stage );
#endif

	//
	// look for multitexture potential
	//
	//if (r_ext_multitexture->integer) {
	for (i = 0; i < stage - 1; i++) {
		stage -= CollapseMultitexture( stages[i + 0].stateBits, &stages[i + 0], &stages[i + 1], stage - i, ( i == 0 ? qtrue: qfalse ) );
	}
	//}


	if (shader.lightmapIndex[0] >= 0 && !hasLightmapStage)
	{
		vk_debug("WARNING: shader '%s' has lightmap but no lightmap stage!\n", shader.name);
		memcpy(shader.lightmapIndex, lightmapsNone, sizeof(shader.lightmapIndex));
		memcpy(shader.styles, stylesDefault, sizeof(shader.styles));
	}

	//
	// compute number of passes
	//
	shader.numUnfoggedPasses = stage;

	// fogonly shaders don't have any normal passes
	if (stage == 0 && !shader.isSky) {
		shader.sort = SS_FOG;
	}

	if (shader.sort <= /*SS_OPAQUE*/ SS_SEE_THROUGH) {
		shader.fogPass = FP_EQUAL;
	}
	else if (shader.contentFlags & CONTENTS_FOG) {
		shader.fogPass = FP_LE;
	}

#ifdef USE_FOG_COLLAPSE
	if ( vk.maxBoundDescriptorSets >= 6 && !(shader.contentFlags & CONTENTS_FOG) && shader.fogPass != FP_NONE ) {
		fogCollapse = qtrue;
		if ( stage == 1 ) {
			// we can always fog-collapse single-stage shaders
		} else {
			if ( tr.numFogs ) {
				// check for (un)acceptable blend modes
				for ( i = 0; i < stage; i++ ) {
					const uint32_t blendBits = stages[i].stateBits & GLS_BLEND_BITS;
					switch ( blendBits & GLS_SRCBLEND_BITS ) {
					case GLS_SRCBLEND_DST_COLOR:
					case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
					case GLS_SRCBLEND_DST_ALPHA:
					case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
						fogCollapse = qfalse;
						break;
					}
					switch ( blendBits & GLS_DSTBLEND_BITS ) {
						case GLS_DSTBLEND_DST_ALPHA:
						case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
						fogCollapse = qfalse;
						break;
					}
				}
				if ( fogCollapse ) {
					for ( i = 1; i < stage; i++ ) {
						const uint32_t blendBits = stages[i].stateBits & GLS_BLEND_BITS;
						if ( blendBits == (GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA) || blendBits == (GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA) ) {
							if ( stages[i].bundle[0].adjustColorsForFog == ACFF_NONE ) {
								fogCollapse = qfalse;
								break;
							}
						}
					}
				}
				if ( fogCollapse ) {
					// correct add mode
					for ( i = 1; i < stage; i++ ) {
						const uint32_t blendBits = stages[i].stateBits & GLS_BLEND_BITS;
						if ( blendBits == (GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE) ) {
							stages[i].bundle[0].adjustColorsForFog = ACFF_MODULATE_RGBA;
						}
					}
				}
			}
		}
	}
	if ( tr.numFogs == 0 ) {
		// if there is no fogs - assume that we can apply all color optimizations without any restrictions
		fogCollapse = qtrue;
	}
#endif

	shader.tessFlags = TESS_XYZ;
	stages[0].tessFlags = TESS_RGBA0 | TESS_ST0;

	/*
	for (iStage = 1; iStage < shader.numUnfoggedPasses; iStage++)
	{
		// Make sure stage is non detail and active
		if (stages[iStage].isDetail || !stages[iStage].active)
		{
			break;
		}
		// MT lightmaps are always in bundle 1
		if (stages[iStage].bundle[0].isLightmap)
		{
			continue;
		}
	}*/

	{
		// create pipelines for each shader stage
		Vk_Pipeline_Def def;

		Com_Memset(&def, 0, sizeof( def ) );
		def.face_culling = shader.cullType;
		def.polygon_offset = shader.polygonOffset;

		if ((stages[0].stateBits & GLS_DEPTHMASK_TRUE) == 0) {
			def.allow_discard = 1;
		}

		for (i = 0; i < stage; i++)
		{
			int env_mask;
			shaderStage_t *pStage = &stages[i];
			def.state_bits = pStage->stateBits;

			if (pStage->mtEnv3) {
				switch (pStage->mtEnv3) {
				case GL_MODULATE:
					pStage->tessFlags = TESS_RGBA0 | TESS_ST0 | TESS_ST1 | TESS_ST2;
					def.shader_type = TYPE_MULTI_TEXTURE_MUL3;
					break;
				case GL_ADD:
					pStage->tessFlags = TESS_RGBA0 | TESS_ST0 | TESS_ST1 | TESS_ST2;
					def.shader_type = TYPE_MULTI_TEXTURE_ADD3_1_1;
					break;
				case GL_ADD_NONIDENTITY:
					pStage->tessFlags = TESS_RGBA0 | TESS_ST0 | TESS_ST1 | TESS_ST2;
					def.shader_type = TYPE_MULTI_TEXTURE_ADD3;
					break;

				case GL_BLEND_MODULATE:
					pStage->tessFlags = TESS_RGBA0 | TESS_RGBA1 | TESS_RGBA2 | TESS_ST0 | TESS_ST1 | TESS_ST2;
					def.shader_type = TYPE_BLEND3_MUL;
					break;
				case GL_BLEND_ADD:
					pStage->tessFlags = TESS_RGBA0 | TESS_RGBA1 | TESS_RGBA2 | TESS_ST0 | TESS_ST1 | TESS_ST2;
					def.shader_type = TYPE_BLEND3_ADD;
					break;
				case GL_BLEND_ALPHA:
					pStage->tessFlags = TESS_RGBA0 | TESS_RGBA1 | TESS_RGBA2 | TESS_ST0 | TESS_ST1 | TESS_ST2;
					def.shader_type = TYPE_BLEND3_ALPHA;
					break;
				case GL_BLEND_ONE_MINUS_ALPHA:
					pStage->tessFlags = TESS_RGBA0 | TESS_RGBA1 | TESS_RGBA2 | TESS_ST0 | TESS_ST1 | TESS_ST2;
					def.shader_type = TYPE_BLEND3_ONE_MINUS_ALPHA;
					break;
				case GL_BLEND_MIX_ONE_MINUS_ALPHA:
					pStage->tessFlags = TESS_RGBA0 | TESS_RGBA1 | TESS_RGBA2 | TESS_ST0 | TESS_ST1 | TESS_ST2;
					def.shader_type = TYPE_BLEND3_MIX_ONE_MINUS_ALPHA;
					break;
				case GL_BLEND_MIX_ALPHA:
					pStage->tessFlags = TESS_RGBA0 | TESS_RGBA1 | TESS_RGBA2 | TESS_ST0 | TESS_ST1 | TESS_ST2;
					def.shader_type = TYPE_BLEND3_MIX_ALPHA;
					break;
				case GL_BLEND_DST_COLOR_SRC_ALPHA:
					pStage->tessFlags = TESS_RGBA0 | TESS_RGBA1 | TESS_RGBA2 | TESS_ST0 | TESS_ST1 | TESS_ST2;
					def.shader_type = TYPE_BLEND3_DST_COLOR_SRC_ALPHA;
					break;

				default:
					break;
				}
			}
			else {
				switch (pStage->mtEnv) {
				case GL_MODULATE:
					pStage->tessFlags = TESS_RGBA0 | TESS_ST0 | TESS_ST1;
					def.shader_type = TYPE_MULTI_TEXTURE_MUL2;
					if ( ( pStage->bundle[0].adjustColorsForFog == ACFF_NONE && pStage->bundle[1].adjustColorsForFog == ACFF_NONE ) || fogCollapse ) {
						if ( pStage->bundle[0].rgbGen == CGEN_IDENTITY && pStage->bundle[1].rgbGen == CGEN_IDENTITY ) {
							if ( pStage->bundle[1].alphaGen == AGEN_SKIP && pStage->bundle[0].alphaGen == AGEN_SKIP ) {
								pStage->tessFlags = TESS_ST0 | TESS_ST1;
								def.shader_type = TYPE_MULTI_TEXTURE_MUL2_IDENTITY;
							}
						}
						else if ( pStage->bundle[0].rgbGen == CGEN_IDENTITY_LIGHTING && pStage->bundle[1].rgbGen == CGEN_IDENTITY_LIGHTING && pStage->bundle[0].alphaGen == pStage->bundle[1].alphaGen ) {
							if ( pStage->bundle[0].alphaGen == AGEN_SKIP || pStage->bundle[0].alphaGen == AGEN_IDENTITY ) {
								pStage->tessFlags = TESS_ST0 | TESS_ST1;
								def.shader_type = TYPE_MULTI_TEXTURE_MUL2_FIXED_COLOR;
								def.color.rgb = tr.identityLightByte;
								def.color.alpha = pStage->bundle[0].alphaGen == AGEN_IDENTITY ? 255 : tr.identityLightByte;
							}
						}
					}
					break;
				case GL_ADD:
					pStage->tessFlags = TESS_RGBA0 | TESS_ST0 | TESS_ST1;
					def.shader_type = TYPE_MULTI_TEXTURE_ADD2_1_1;
					if ( ( pStage->bundle[0].adjustColorsForFog == ACFF_NONE && pStage->bundle[1].adjustColorsForFog == ACFF_NONE ) || fogCollapse ) {
						if ( pStage->bundle[0].rgbGen == CGEN_IDENTITY && pStage->bundle[1].rgbGen == CGEN_IDENTITY ) {
							if ( pStage->bundle[0].alphaGen == AGEN_SKIP && pStage->bundle[1].alphaGen == AGEN_SKIP ) {
								pStage->tessFlags = TESS_ST0 | TESS_ST1;
								def.shader_type = TYPE_MULTI_TEXTURE_ADD2_IDENTITY;
							}
						}
						else if ( pStage->bundle[0].rgbGen == CGEN_IDENTITY_LIGHTING && pStage->bundle[1].rgbGen == CGEN_IDENTITY_LIGHTING && pStage->bundle[0].alphaGen == pStage->bundle[1].alphaGen ) {
							if ( pStage->bundle[0].alphaGen == AGEN_SKIP || pStage->bundle[0].alphaGen == AGEN_IDENTITY ) {
								pStage->tessFlags = TESS_ST0 | TESS_ST1;
								def.shader_type = TYPE_MULTI_TEXTURE_ADD2_FIXED_COLOR;
								def.color.rgb = tr.identityLightByte;
								def.color.alpha = pStage->bundle[0].alphaGen == AGEN_IDENTITY ? 255 : tr.identityLightByte;
							}
						}
					}
					break;
				case GL_ADD_NONIDENTITY:
					pStage->tessFlags = TESS_RGBA0 | TESS_ST0 | TESS_ST1;
					def.shader_type = TYPE_MULTI_TEXTURE_ADD2;
					if ( ( pStage->bundle[0].adjustColorsForFog == ACFF_NONE && pStage->bundle[1].adjustColorsForFog == ACFF_NONE ) || fogCollapse ) {
						if ( pStage->bundle[0].rgbGen == CGEN_IDENTITY && pStage->bundle[1].rgbGen == CGEN_IDENTITY ) {
							if ( pStage->bundle[0].alphaGen == AGEN_SKIP && pStage->bundle[1].alphaGen == AGEN_SKIP ) {
								pStage->tessFlags = TESS_ST0 | TESS_ST1;
								def.shader_type = TYPE_MULTI_TEXTURE_ADD2_IDENTITY;
							}
						}
						else if ( pStage->bundle[0].rgbGen == CGEN_IDENTITY_LIGHTING && pStage->bundle[1].rgbGen == CGEN_IDENTITY_LIGHTING && pStage->bundle[0].alphaGen == pStage->bundle[1].alphaGen ) {
							if ( pStage->bundle[0].alphaGen == AGEN_SKIP || pStage->bundle[0].alphaGen == AGEN_IDENTITY ) {
								pStage->tessFlags = TESS_ST0 | TESS_ST1;
								def.shader_type = TYPE_MULTI_TEXTURE_ADD2_FIXED_COLOR;
								def.color.rgb = tr.identityLightByte;
								def.color.alpha = pStage->bundle[0].alphaGen == AGEN_IDENTITY ? 255 : tr.identityLightByte;
							}
						}
					}
					break;
				// extended blending modes
				case GL_BLEND_MODULATE:
					pStage->tessFlags = TESS_RGBA0 | TESS_RGBA1 | TESS_ST0 | TESS_ST1;
					def.shader_type = TYPE_BLEND2_MUL;
					break;
				case GL_BLEND_ADD:
					pStage->tessFlags = TESS_RGBA0 | TESS_RGBA1 | TESS_ST0 | TESS_ST1;
					def.shader_type = TYPE_BLEND2_ADD;
					break;
				case GL_BLEND_ALPHA:
					pStage->tessFlags = TESS_RGBA0 | TESS_RGBA1 | TESS_ST0 | TESS_ST1;
					def.shader_type = TYPE_BLEND2_ALPHA;
					break;
				case GL_BLEND_ONE_MINUS_ALPHA:
					pStage->tessFlags = TESS_RGBA0 | TESS_RGBA1 | TESS_ST0 | TESS_ST1;
					def.shader_type = TYPE_BLEND2_ONE_MINUS_ALPHA;
					break;
				case GL_BLEND_MIX_ALPHA:
					pStage->tessFlags = TESS_RGBA0 | TESS_RGBA1 | TESS_ST0 | TESS_ST1;
					def.shader_type = TYPE_BLEND2_MIX_ALPHA;
					break;
				case GL_BLEND_MIX_ONE_MINUS_ALPHA:
					pStage->tessFlags = TESS_RGBA0 | TESS_RGBA1 | TESS_ST0 | TESS_ST1;
					def.shader_type = TYPE_BLEND2_MIX_ONE_MINUS_ALPHA;
					break;
				case GL_BLEND_DST_COLOR_SRC_ALPHA:
					pStage->tessFlags = TESS_RGBA0 | TESS_RGBA1 | TESS_ST0 | TESS_ST1;
					def.shader_type = TYPE_BLEND2_DST_COLOR_SRC_ALPHA;
					break;

				default:
					pStage->tessFlags = TESS_RGBA0 | TESS_ST0;
					def.shader_type = TYPE_SINGLE_TEXTURE;
					if ( pStage->bundle[0].adjustColorsForFog == ACFF_NONE || fogCollapse ) {
						if ( pStage->bundle[0].rgbGen == CGEN_IDENTITY ) {
							if ( pStage->bundle[0].alphaGen == AGEN_SKIP ) {
								pStage->tessFlags = TESS_ST0;
								def.shader_type = TYPE_SINGLE_TEXTURE_IDENTITY;
							}
						}
						else if ( pStage->bundle[0].rgbGen == CGEN_IDENTITY_LIGHTING ) {
							if ( pStage->bundle[0].alphaGen == AGEN_SKIP || pStage->bundle[0].alphaGen == AGEN_IDENTITY ) {
								pStage->tessFlags = TESS_ST0;
								def.shader_type = TYPE_SINGLE_TEXTURE_FIXED_COLOR;
								def.color.rgb = tr.identityLightByte;
								def.color.alpha = pStage->bundle[0].alphaGen == AGEN_IDENTITY ? 255 : tr.identityLightByte;
							}
						}
					}
				}
			} // switch mtEnv3 / mtEnv

			for (env_mask = 0, n = 0; n < pStage->numTexBundles; n++) {
				if (pStage->bundle[n].numTexMods) {
					continue;
				}
				if (pStage->bundle[n].tcGen == TCGEN_ENVIRONMENT_MAPPED && (!pStage->bundle[n].isLightmap)) {
					env_mask |= (1 << n);
				}
			}

			if (env_mask == 1 && !pStage->depthFragment) {
				if (def.shader_type >= TYPE_GENERIC_BEGIN && def.shader_type <= TYPE_GENERIC_END) {
					//def.shader_type++; // switch to *_ENV version
					def.shader_type = (Vk_Shader_Type)((int)def.shader_type + 1);

					shader.tessFlags |= TESS_NNN | TESS_VPOS;
					pStage->tessFlags &= ~TESS_ST0;
					pStage->tessFlags |= TESS_ENV;
					pStage->bundle[0].tcGen = TCGEN_BAD;
				}
			}


			if (pStage->ss && pStage->ss->type) {
				def.face_culling = CT_TWO_SIDED;
			}


			def.mirror = qfalse;
			pStage->vk_pipeline[0] = vk_find_pipeline_ext(0, &def, qtrue);
			def.mirror = qtrue;
			pStage->vk_mirror_pipeline[0] = vk_find_pipeline_ext(0, &def, qfalse);

			if ( pStage->depthFragment ) {
				def.mirror = qfalse;
				def.shader_type = TYPE_SINGLE_TEXTURE_DF;
				pStage->vk_pipeline_df = vk_find_pipeline_ext( 0, &def, qtrue );
				def.mirror = qtrue;
				def.shader_type = TYPE_SINGLE_TEXTURE_DF;
				pStage->vk_mirror_pipeline_df = vk_find_pipeline_ext( 0, &def, qfalse );
			}

			// this will be a copy of the vk_pipeline[0] but with faceculling disabled
			pStage->vk_2d_pipeline = 0;

#ifdef USE_FOG_COLLAPSE
			// single-stage, combined fog pipelines
			if ( fogCollapse && tr.numFogs > 0 ) {
				Vk_Pipeline_Def def;
				Vk_Pipeline_Def def_mirror;

				vk_get_pipeline_def( pStage->vk_pipeline[0], &def );
				vk_get_pipeline_def( pStage->vk_mirror_pipeline[0], &def_mirror );

				def.fog_stage = 1;
				def_mirror.fog_stage = 1;
				def.acff = pStage->bundle[0].adjustColorsForFog;
				def_mirror.acff = pStage->bundle[0].adjustColorsForFog;

				pStage->vk_pipeline[1] = vk_find_pipeline_ext( 0, &def, qfalse );
				pStage->vk_mirror_pipeline[1] = vk_find_pipeline_ext( 0, &def_mirror, qfalse );

				pStage->bundle[0].adjustColorsForFog = ACFF_NONE; // will be handled in shader from now

				shader.fogCollapse = qtrue;
			}
#endif // USE_FOG_COLLAPSE
		}
	}

#ifdef USE_PMLIGHT
	FindLightingBundle();
#endif

	// try to avoid redundant per-stage computations
	Com_Memset(lastStage, 0, sizeof(lastStage));
	for (i = 0; i < shader.numUnfoggedPasses - 1; i++) {
		if (!stages[i + 1].active)
			break;
		for (n = 0; n < NUM_TEXTURE_BUNDLES; n++) {
			if (stages[i].bundle[n].image[0] != NULL) {
				lastStage[n] = &stages[i];
			}
			// collapsed multi-stage shaders during glow pass: 
			// blackimage texture is used on a non-glow bundle and ComputeTexCoords(), ComputeColors() are skipped.
			// TESS_ST0 or TESS_RGBA0 are removed on a glow bundle in the next stage 
			// when tc and rgb are equal to the non-glow bundle in the previous stage, it will use stale tc and/or rgb data.
			// Most noticable shader: textures/rooftop/building_ext3 in t2_rogue (green buildings)
			// 
			// note: leaving flags here also affects the main render pass, that is undesired behaivior
			// moved to vk_shade_geometry: RB_StageIteratorGeneric()
#if 0
			if ( !stages[i].bundle[n].glow && stages[i + 1].bundle[n].glow ) {
				continue;
			}
#endif
			if ( EqualTCgen( n, lastStage[ n ], &stages[ i+1 ] ) && (lastStage[n]->tessFlags & (TESS_ST0 << n) ) ) {
				stages[i + 1].tessFlags &= ~(TESS_ST0 << n);
			}
			if ( EqualRGBgen( lastStage[n], &stages[ i+1 ] ) && EqualACgen( lastStage[n], &stages[ i+1 ] ) && (lastStage[n]->tessFlags & (TESS_RGBA0 << n) ) ) {
				stages[i + 1].tessFlags &= ~(TESS_RGBA0 << n);
			}
		}
	}

	// make sure that amplitude for TMOD_STRETCH is not zero
	for (i = 0; i < shader.numUnfoggedPasses; i++) {
		if (!stages[i].active) {
			continue;
		}
		for (n = 0; n < stages[i].numTexBundles; n++) {
			for (m = 0; m < stages[i].bundle[n].numTexMods; m++) {
				if (stages[i].bundle[n].texMods[m].type == TMOD_STRETCH) {
					if (fabsf(stages[i].bundle[n].texMods[m].wave.amplitude) < EPSILON) {
						if (stages[i].bundle[n].texMods[m].wave.amplitude >= 0.0f) {
							stages[i].bundle[n].texMods[m].wave.amplitude = EPSILON;
						}
						else {
							stages[i].bundle[n].texMods[m].wave.amplitude = -EPSILON;
						}
					}
				}
			}
		}
	}

	// determine which stage iterator function is appropriate
	ComputeStageIteratorFunc();

	return GeneratePermanentShader();
}
//========================================================================================

/*
=============

FixRenderCommandList
https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=493
Arnout: this is a nasty issue. Shaders can be registered after drawsurfaces are generated
but before the frame is rendered. This will, for the duration of one frame, cause drawsurfaces
to be rendered with bad shaders. To fix this, need to go through all render commands and fix
sortedIndex.
==============
*/
extern bool gServerSkinHack;
static void FixRenderCommandList( int newShader ) {
	if ( gServerSkinHack )
		return;

	renderCommandList_t *cmdList = &backEndData->commands;

	if ( cmdList ) {
		const void* curCmd = cmdList->cmds;

		*( (int *)( cmdList->cmds + cmdList->used ) ) = RC_END_OF_LIST;

		while ( 1 ) {
			curCmd = PADP(curCmd, sizeof(void *));

			switch (*(const int*)curCmd) {
			case RC_SET_COLOR:
			{
				const setColorCommand_t* sc_cmd = (const setColorCommand_t*)curCmd;
				curCmd = (const void*)(sc_cmd + 1);
				break;
			}
			case RC_STRETCH_PIC:
			{
				const stretchPicCommand_t* sp_cmd = (const stretchPicCommand_t*)curCmd;
				curCmd = (const void*)(sp_cmd + 1);
				break;
			}
			case RC_ROTATE_PIC:
			{
				const rotatePicCommand_t* sp_cmd = (const rotatePicCommand_t*)curCmd;
				curCmd = (const void*)(sp_cmd + 1);
				break;
			}
			case RC_ROTATE_PIC2:
			{
				const rotatePicCommand_t* sp_cmd = (const rotatePicCommand_t*)curCmd;
				curCmd = (const void*)(sp_cmd + 1);
				break;
			}
			case RC_DRAW_SURFS:
			{
				int i;
				drawSurf_t	*drawSurf;
				shader_t	*shader;
				int			fogNum;
				int			entityNum;
				int			dlightMap;
				int			sortedIndex;
				const drawSurfsCommand_t* ds_cmd = (const drawSurfsCommand_t*)curCmd;

				for ( i = 0, drawSurf = ds_cmd->drawSurfs; i < ds_cmd->numDrawSurfs; i++, drawSurf++ ) {
					R_DecomposeSort( drawSurf->sort, &entityNum, &shader, &fogNum, &dlightMap );
					sortedIndex = (( drawSurf->sort >> QSORT_SHADERNUM_SHIFT ) & SHADERNUM_MASK);
					if ( sortedIndex >= newShader ) {
						sortedIndex = shader->sortedIndex;
						drawSurf->sort = (sortedIndex << QSORT_SHADERNUM_SHIFT) | (entityNum << QSORT_REFENTITYNUM_SHIFT) | ( fogNum << QSORT_FOGNUM_SHIFT ) | (int)dlightMap;
					}
				}
				curCmd = (const void *)(ds_cmd + 1);
				break;
			}
			case RC_DRAW_BUFFER:
			case RC_WORLD_EFFECTS:
			case RC_AUTO_MAP:
			{
				const drawBufferCommand_t* db_cmd = (const drawBufferCommand_t*)curCmd;
				curCmd = (const void*)(db_cmd + 1);
				break;
			}
			case RC_SWAP_BUFFERS:
			{
				const swapBuffersCommand_t* sb_cmd = (const swapBuffersCommand_t*)curCmd;
				curCmd = (const void*)(sb_cmd + 1);
				break;
			}
			case RC_CLEARCOLOR:
			{
				const clearColorCommand_t* cc_cmd = (const clearColorCommand_t*)curCmd;
				curCmd = (const void*)(cc_cmd + 1);
				break;
			}
			case RC_END_OF_LIST:
			default:
				return;
			}
		}
	}
}

/*
==============
SortNewShader

Positions the most recently created shader in the tr.sortedShaders[]
array so that the shader->sort key is sorted reletive to the other
shaders.

Sets shader->sortedIndex
==============
*/
static void SortNewShader( void )
{
	int			i;
	float		sort;
	shader_t	*newShader;

	newShader = tr.shaders[tr.numShaders - 1];
	sort = newShader->sort;

	for (i = tr.numShaders - 2; i >= 0; i--)
	{
		if (tr.sortedShaders[i]->sort <= sort) {
			break;
		}
		tr.sortedShaders[i + 1] = tr.sortedShaders[i];
		tr.sortedShaders[i + 1]->sortedIndex++;
	}

	// Arnout: fix rendercommandlist
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=493
	FixRenderCommandList(i + 1);

	newShader->sortedIndex = i + 1;
	tr.sortedShaders[i + 1] = newShader;
}

shader_t *GeneratePermanentShader( void )
{
	shader_t	*newShader;
	int			i, b, size;

	if ( tr.numShaders == MAX_SHADERS ) {
		vk_debug("WARNING: GeneratePermanentShader - MAX_SHADERS hit\n");
		return tr.defaultShader;
	}

	newShader = (shader_t*)ri.Hunk_Alloc( sizeof(shader_t), h_low );

	*newShader = shader;

	tr.shaders[tr.numShaders] = newShader;
	newShader->index = tr.numShaders;

	tr.sortedShaders[tr.numShaders] = newShader;
	newShader->sortedIndex = tr.numShaders;

	tr.numShaders++;

	for ( i = 0; i < newShader->numUnfoggedPasses; i++ )
	{
		if ( !stages[i].active ) {
			break;
		}
		newShader->stages[i] = (shaderStage_t*)ri.Hunk_Alloc( sizeof(stages[i]), h_low );
		*newShader->stages[i] = stages[i];

		for ( b = 0; b < NUM_TEXTURE_BUNDLES; b++ )
		{
			if ( newShader->stages[i]->bundle[b].numTexMods )
			{
				size = newShader->stages[i]->bundle[b].numTexMods * sizeof( texModInfo_t );
				if ( size ) {
					newShader->stages[i]->bundle[b].texMods = (texModInfo_t*)ri.Hunk_Alloc( size, h_low );
					Com_Memcpy( newShader->stages[i]->bundle[b].texMods, stages[i].bundle[b].texMods, size );
				}
			} 
			else {
				newShader->stages[i]->bundle[b].texMods = 0;	//clear the globabl ptr jic
			}
		}
	}

	SortNewShader();

	const int hash = generateHashValue(newShader->name, FILE_HASH_SIZE);
	newShader->next = hashTable[hash];
	hashTable[hash] = newShader;

	return newShader;
}

void R_CreateDefaultShadingCmds( image_t *image )
{

	if (shader.lightmapIndex[0] == LIGHTMAP_NONE)
	{
		// dynamic colors at vertexes
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].bundle[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
		stages[0].stateBits = GLS_DEFAULT;
	}
	else if (shader.lightmapIndex[0] == LIGHTMAP_BY_VERTEX)
	{
		// explicit colors at vertexes
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].bundle[0].rgbGen = CGEN_EXACT_VERTEX;
		stages[0].bundle[0].alphaGen = AGEN_SKIP;
		stages[0].stateBits = GLS_DEFAULT;
	}
	else if (shader.lightmapIndex[0] == LIGHTMAP_2D)
	{
		// GUI elements
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].bundle[0].rgbGen = CGEN_VERTEX;
		stages[0].bundle[0].alphaGen = AGEN_VERTEX;
		stages[0].stateBits = GLS_DEPTHTEST_DISABLE |
			GLS_SRCBLEND_SRC_ALPHA |
			GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	}
	else if (shader.lightmapIndex[0] == LIGHTMAP_WHITEIMAGE)
	{
		// fullbright level
		stages[0].bundle[0].image[0] = tr.whiteImage;
		stages[0].active = qtrue;
		stages[0].bundle[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = qtrue;
		stages[1].bundle[0].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	}
	else
	{
		// two pass lightmap
		stages[0].bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex[0]];
		stages[0].bundle[0].isLightmap = qtrue;
		stages[0].active = qtrue;
		stages[0].bundle[0].rgbGen = CGEN_IDENTITY;	// lightmaps are scaled on creation for identitylight
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = qtrue;
		stages[1].bundle[0].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	}
}

/*
====================
R_GetShaderByHandle

When a handle is passed in by another module, this range checks
it and returns a valid (possibly default) shader_t to be used internally.
====================
*/
shader_t *R_GetShaderByHandle( qhandle_t hShader )
{
	if (hShader < 0) {
		vk_debug("R_GetShaderByHandle: out of range hShader '%d'\n", hShader); // bk: FIXME name
		return tr.defaultShader;
	}
	if (hShader >= tr.numShaders) {
		vk_debug("R_GetShaderByHandle: out of range hShader '%d'\n", hShader);
		return tr.defaultShader;
	}
	return tr.shaders[hShader];
}

/*
====================
CreateInternalShaders
====================
*/
static void CreateInternalShaders( void )
{
	vk_debug("CreateInternalShaders\n");

	tr.numShaders = 0;

	// init the default shader
	InitShader("<default>", lightmapsNone, stylesDefault);
	stages[0].bundle[0].image[0] = tr.defaultImage;
	stages[0].active = qtrue;
	stages[0].stateBits = GLS_DEFAULT;
	tr.defaultShader = FinishShader();

	// shadow shader is just a marker
	InitShader("<stencil shadow>", lightmapsNone, stylesDefault);
	stages[0].bundle[0].image[0] = tr.defaultImage;
	stages[0].active = qtrue;
	stages[0].stateBits = GLS_DEFAULT;
	shader.sort = SS_BANNER;
	tr.shadowShader = FinishShader();

	// distortion shader is just a marker
	InitShader("internal_distortion", lightmapsNone, stylesDefault);
	stages[0].bundle[0].image[0] = tr.whiteImage;
	stages[0].active = qtrue;
	stages[0].stateBits = GLS_DEFAULT;
	shader.sort = SS_BLEND0;

	if ( vk.refractionActive ) 
	{
		shader.defaultShader = qfalse;
		tr.distortionShader = FinishShader();
		tr.distortionShader->useDistortion = qtrue;
	} 
	else 
	{
		// https://github.com/MBII/OpenJK/blob/8cf83b7a522bb7675074b576de929d6e149b429b/codemp/rd-vulkan/tr_shader.cpp#L4607
		stages[0].bundle[0].rgbGen = CGEN_CONST;
		stages[0].bundle[0].constantColor[0] = 80;
		stages[0].bundle[0].constantColor[1] = 90;
		stages[0].bundle[0].constantColor[2] = 100;
		stages[0].bundle[0].alphaGen = AGEN_WAVEFORM;
		stages[0].bundle[0].alphaWave.func = GF_SIN;
		stages[0].bundle[0].alphaWave.base = 0.07f;
		stages[0].bundle[0].alphaWave.amplitude = 0.03f;
		stages[0].bundle[0].alphaWave.phase = 0;
		stages[0].bundle[0].alphaWave.frequency = 0.33f;
		stages[0].stateBits = GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
		shader.contentFlags = CONTENTS_TRANSLUCENT;
		tr.distortionShader = FinishShader();
	}

	InitShader("<white>", lightmapsNone, stylesDefault);
	stages[0].bundle[0].image[0] = tr.whiteImage;
	stages[0].active = qtrue;
	stages[0].bundle[0].rgbGen = CGEN_EXACT_VERTEX;
	stages[0].stateBits = GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	tr.whiteShader = FinishShader();

	InitShader("<cinematic>", lightmapsNone, stylesDefault);
	stages[0].bundle[0].image[0] = tr.defaultImage; // will be updated by specific cinematic images
	stages[0].active = qtrue;
	stages[0].bundle[0].rgbGen = CGEN_IDENTITY_LIGHTING;
	stages[0].stateBits = GLS_DEPTHTEST_DISABLE;
	tr.cinematicShader = FinishShader();
}

static void CreateExternalShaders( void )
{
	vk_debug("CreateExternalShaders\n");
	tr.projectionShadowShader = R_FindShader("projectionShadow", lightmapsNone, stylesDefault, qtrue);
	tr.projectionShadowShader->sort = SS_BANNER;
	// investigate why default sort order gives issues with saber sort order.
	//tr.projectionShadowShader->sort = SS_STENCIL_SHADOW;

	//tr.flareShader = R_FindShader("flareShader", lightmapsNone, stylesDefault, qtrue);
	tr.flareShader = R_FindShader("gfx/misc/Flareparticle", lightmapsNone, stylesDefault, qtrue);

	// Hack to make fogging work correctly on flares. Fog colors are calculated
	// in tr_flare.c already.
	if (!tr.flareShader->defaultShader)
	{
		int index;

		for (index = 0; index < tr.flareShader->numUnfoggedPasses; index++)
		{
			tr.flareShader->stages[index]->bundle[0].adjustColorsForFog = ACFF_NONE;
			tr.flareShader->stages[index]->stateBits |= GLS_DEPTHTEST_DISABLE;
		}
	}

	tr.sunShader = R_FindShader("sun", lightmapsNone, stylesDefault, qtrue);
}

void R_InitShaders( qboolean server )
{

	vk_debug("Initializing Shaders\n");

	memset(hashTable, 0, sizeof(hashTable));

	if ( !server )
	{
		CreateInternalShaders();

		ScanAndLoadShaderFiles();

		CreateExternalShaders();
	}
}
