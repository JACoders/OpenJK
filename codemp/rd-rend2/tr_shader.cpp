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

static char *s_shaderText;

// the shader is parsed into these global variables, then copied into
// dynamically allocated memory if it is valid.
static	shaderStage_t	stages[MAX_SHADER_STAGES];
static	shader_t		shader;
static	texModInfo_t	texMods[MAX_SHADER_STAGES][TR_MAX_TEXMODS];

// Hash value (generated using the generateHashValueForText function) for the original
// retail JKA shader for gfx/2d/wedge.
#define RETAIL_ROCKET_WEDGE_SHADER_HASH (1217042)

#define FILE_HASH_SIZE		1024
static	shader_t*		hashTable[FILE_HASH_SIZE];

#define MAX_SHADERTEXT_HASH		2048
static char **shaderTextHashTable[MAX_SHADERTEXT_HASH] = { 0 };

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

qhandle_t RE_RegisterShaderLightMap( const char *name, const int *lightmapIndexes, const byte *styles );

void KillTheShaderHashTable(void)
{
	memset(shaderTextHashTable, 0, sizeof(shaderTextHashTable));
}

qboolean ShaderHashTableExists(void)
{
	if (shaderTextHashTable[0])
	{
		return qtrue;
	}
	return qfalse;
}

static void ClearGlobalShader(void)
{
	int	i;

	memset( &shader, 0, sizeof( shader ) );
	memset( &stages, 0, sizeof( stages ) );
	for ( i = 0 ; i < MAX_SHADER_STAGES ; i++ ) {
		stages[i].bundle[0].texMods = texMods[i];
		//stages[i].mGLFogColorOverride = GLFOGOVERRIDE_NONE;

		// default normal/specular
		VectorSet4(stages[i].normalScale, 0.0f, 0.0f, 0.0f, 0.0f);
		stages[i].specularScale[0] =
		stages[i].specularScale[1] =
		stages[i].specularScale[2] = r_baseSpecular->value;
		stages[i].specularScale[3] = 0.99f;
	}

	shader.contentFlags = CONTENTS_SOLID | CONTENTS_OPAQUE;
}

static uint32_t generateHashValueForText( const char *string, size_t length )
{
	int i = 0;
	uint32_t hash = 0;

	while ( length-- )
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
		if (letter =='.') break;				// don't include extension
		if (letter =='\\') letter = '/';		// damn path names
		if (letter == PATH_SEP) letter = '/';		// damn path names
		hash+=(long)(letter)*(i+119);
		i++;
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20));
	hash &= (size-1);
	return hash;
}

void R_RemapShader(const char *shaderName, const char *newShaderName, const char *timeOffset) {
	char		strippedName[MAX_QPATH];
	int			hash;
	shader_t	*sh, *sh2;
	qhandle_t	h;

	sh = R_FindShaderByName( shaderName );
	if (sh == NULL || sh == tr.defaultShader) {
		h = RE_RegisterShaderLightMap (shaderName, lightmapsNone, stylesDefault);
		sh = R_GetShaderByHandle(h);
	}
	if (sh == NULL || sh == tr.defaultShader) {
		ri.Printf( PRINT_WARNING, "WARNING: R_RemapShader: shader %s not found\n", shaderName );
		return;
	}

	sh2 = R_FindShaderByName( newShaderName );
	if (sh2 == NULL || sh2 == tr.defaultShader) {
		h = RE_RegisterShaderLightMap (newShaderName, lightmapsNone, stylesDefault);
		sh2 = R_GetShaderByHandle(h);
	}

	if (sh2 == NULL || sh2 == tr.defaultShader) {
		ri.Printf( PRINT_WARNING, "WARNING: R_RemapShader: new shader %s not found\n", newShaderName );
		return;
	}

	// remap all the shaders with the given name
	// even tho they might have different lightmaps
	COM_StripExtension(shaderName, strippedName, sizeof(strippedName));
	hash = generateHashValue(strippedName, FILE_HASH_SIZE);
	for (sh = hashTable[hash]; sh; sh = sh->next) {
		if (Q_stricmp(sh->name, strippedName) == 0) {
			if (sh != sh2) {
				sh->remappedShader = sh2;
			} else {
				sh->remappedShader = NULL;
			}
		}
	}
	if (timeOffset) {
		sh2->timeOffset = atof(timeOffset);
	}
}

/*
===============
ParseVector
===============
*/
static qboolean ParseVector( const char **text, int count, float *v ) {
	char	*token;
	int		i;

	// FIXME: spaces are currently required after parens, should change parseext...
	token = COM_ParseExt( text, qfalse );
	if ( strcmp( token, "(" ) ) {
		ri.Printf( PRINT_WARNING, "WARNING: missing parenthesis in shader '%s'\n", shader.name );
		return qfalse;
	}

	for ( i = 0 ; i < count ; i++ ) {
		token = COM_ParseExt( text, qfalse );
		if ( !token[0] ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing vector element in shader '%s'\n", shader.name );
			return qfalse;
		}
		v[i] = atof( token );
	}

	token = COM_ParseExt( text, qfalse );
	if ( strcmp( token, ")" ) ) {
		ri.Printf( PRINT_WARNING, "WARNING: missing parenthesis in shader '%s'\n", shader.name );
		return qfalse;
	}

	return qtrue;
}


/*
===============
ParseAlphaTestFunc
===============
*/
static void ParseAlphaTestFunc( shaderStage_t *stage, const char *funcname )
{
	stage->alphaTestType = ALPHA_TEST_NONE;

	if ( !Q_stricmp( funcname, "GT0" ) )
		stage->alphaTestType = ALPHA_TEST_GT0;
	else if ( !Q_stricmp( funcname, "LT128" ) )
		stage->alphaTestType = ALPHA_TEST_LT128;
	else if ( !Q_stricmp( funcname, "GE128" ) )
		stage->alphaTestType = ALPHA_TEST_GE128;
	else if ( !Q_stricmp( funcname, "GE192" ) )
		stage->alphaTestType = ALPHA_TEST_GE192;
	else
		ri.Printf( PRINT_WARNING,
				"WARNING: invalid alphaFunc name '%s' in shader '%s'\n",
				funcname, shader.name );
}


/*
===============
NameToSrcBlendMode
===============
*/
static int NameToSrcBlendMode( const char *name )
{
	if ( !Q_stricmp( name, "GL_ONE" ) )
	{
		return GLS_SRCBLEND_ONE;
	}
	else if ( !Q_stricmp( name, "GL_ZERO" ) )
	{
		return GLS_SRCBLEND_ZERO;
	}
	else if ( !Q_stricmp( name, "GL_DST_COLOR" ) )
	{
		return GLS_SRCBLEND_DST_COLOR;
	}
	else if ( !Q_stricmp( name, "GL_ONE_MINUS_DST_COLOR" ) )
	{
		return GLS_SRCBLEND_ONE_MINUS_DST_COLOR;
	}
	else if ( !Q_stricmp( name, "GL_SRC_ALPHA" ) )
	{
		return GLS_SRCBLEND_SRC_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_ONE_MINUS_SRC_ALPHA" ) )
	{
		return GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_DST_ALPHA" ) )
	{
		if ( r_ignoreDstAlpha->integer )
		{
			return GLS_SRCBLEND_ONE;
		}

		return GLS_SRCBLEND_DST_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_ONE_MINUS_DST_ALPHA" ) )
	{
		if ( r_ignoreDstAlpha->integer )
		{
			return GLS_SRCBLEND_ZERO;
		}

		return GLS_SRCBLEND_ONE_MINUS_DST_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_SRC_ALPHA_SATURATE" ) )
	{
		return GLS_SRCBLEND_ALPHA_SATURATE;
	}

	ri.Printf( PRINT_WARNING, "WARNING: unknown blend mode '%s' in shader '%s', substituting GL_ONE\n", name, shader.name );
	return GLS_SRCBLEND_ONE;
}

/*
===============
NameToDstBlendMode
===============
*/
static int NameToDstBlendMode( const char *name )
{
	if ( !Q_stricmp( name, "GL_ONE" ) )
	{
		return GLS_DSTBLEND_ONE;
	}
	else if ( !Q_stricmp( name, "GL_ZERO" ) )
	{
		return GLS_DSTBLEND_ZERO;
	}
	else if ( !Q_stricmp( name, "GL_SRC_ALPHA" ) )
	{
		return GLS_DSTBLEND_SRC_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_ONE_MINUS_SRC_ALPHA" ) )
	{
		return GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_DST_ALPHA" ) )
	{
		if ( r_ignoreDstAlpha->integer )
		{
			return GLS_DSTBLEND_ONE;
		}

		return GLS_DSTBLEND_DST_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_ONE_MINUS_DST_ALPHA" ) )
	{
		if ( r_ignoreDstAlpha->integer )
		{
			return GLS_DSTBLEND_ZERO;
		}

		return GLS_DSTBLEND_ONE_MINUS_DST_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_SRC_COLOR" ) )
	{
		return GLS_DSTBLEND_SRC_COLOR;
	}
	else if ( !Q_stricmp( name, "GL_ONE_MINUS_SRC_COLOR" ) )
	{
		return GLS_DSTBLEND_ONE_MINUS_SRC_COLOR;
	}

	ri.Printf( PRINT_WARNING, "WARNING: unknown blend mode '%s' in shader '%s', substituting GL_ONE\n", name, shader.name );
	return GLS_DSTBLEND_ONE;
}

/*
===============
NameToGenFunc
===============
*/
static genFunc_t NameToGenFunc( const char *funcname )
{
	if ( !Q_stricmp( funcname, "sin" ) )
	{
		return GF_SIN;
	}
	else if ( !Q_stricmp( funcname, "square" ) )
	{
		return GF_SQUARE;
	}
	else if ( !Q_stricmp( funcname, "triangle" ) )
	{
		return GF_TRIANGLE;
	}
	else if ( !Q_stricmp( funcname, "sawtooth" ) )
	{
		return GF_SAWTOOTH;
	}
	else if ( !Q_stricmp( funcname, "inversesawtooth" ) )
	{
		return GF_INVERSE_SAWTOOTH;
	}
	else if ( !Q_stricmp( funcname, "noise" ) )
	{
		return GF_NOISE;
	}
	else if ( !Q_stricmp( funcname, "random" ) )
	{
		return GF_RAND;
	}

	ri.Printf( PRINT_WARNING, "WARNING: invalid genfunc name '%s' in shader '%s'\n", funcname, shader.name );
	return GF_SIN;
}


/*
===================
ParseWaveForm
===================
*/
static void ParseWaveForm( const char **text, waveForm_t *wave )
{
	char *token;

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 )
	{
		ri.Printf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->func = NameToGenFunc( token );

	// BASE, AMP, PHASE, FREQ
	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 )
	{
		ri.Printf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->base = atof( token );

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 )
	{
		ri.Printf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->amplitude = atof( token );

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 )
	{
		ri.Printf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->phase = atof( token );

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 )
	{
		ri.Printf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->frequency = atof( token );
}


/*
===================
ParseTexMod
===================
*/
static void ParseTexMod( const char *_text, shaderStage_t *stage )
{
	const char *token;
	const char **text = &_text;
	texModInfo_t *tmi;

	if ( stage->bundle[0].numTexMods == TR_MAX_TEXMODS ) {
		ri.Error( ERR_DROP, "ERROR: too many tcMod stages in shader '%s'", shader.name );
		return;
	}

	tmi = &stage->bundle[0].texMods[stage->bundle[0].numTexMods];
	stage->bundle[0].numTexMods++;

	token = COM_ParseExt( text, qfalse );

	//
	// turb
	//
	if ( !Q_stricmp( token, "turb" ) )
	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing tcMod turb parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.base = atof( token );
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing tcMod turb in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.amplitude = atof( token );
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing tcMod turb in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.phase = atof( token );
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing tcMod turb in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.frequency = atof( token );

		tmi->type = TMOD_TURBULENT;
	}
	//
	// scale
	//
	else if ( !Q_stricmp( token, "scale" ) )
	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing scale parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->scale[0] = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing scale parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->scale[1] = atof( token );
		tmi->type = TMOD_SCALE;
	}
	//
	// scroll
	//
	else if ( !Q_stricmp( token, "scroll" ) )
	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing scale scroll parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->scroll[0] = atof( token );
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing scale scroll parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->scroll[1] = atof( token );
		tmi->type = TMOD_SCROLL;
	}
	//
	// stretch
	//
	else if ( !Q_stricmp( token, "stretch" ) )
	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.func = NameToGenFunc( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.base = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.amplitude = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.phase = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.frequency = atof( token );

		tmi->type = TMOD_STRETCH;
	}
	//
	// transform
	//
	else if ( !Q_stricmp( token, "transform" ) )
	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->matrix[0][0] = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->matrix[0][1] = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->matrix[1][0] = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->matrix[1][1] = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->translate[0] = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->translate[1] = atof( token );

		tmi->type = TMOD_TRANSFORM;
	}
	//
	// rotate
	//
	else if ( !Q_stricmp( token, "rotate" ) )
	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing tcMod rotate parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->rotateSpeed = atof( token );
		tmi->type = TMOD_ROTATE;
	}
	//
	// entityTranslate
	//
	else if ( !Q_stricmp( token, "entityTranslate" ) )
	{
		tmi->type = TMOD_ENTITY_TRANSLATE;
	}
	else
	{
		ri.Printf( PRINT_WARNING, "WARNING: unknown tcMod '%s' in shader '%s'\n", token, shader.name );
	}
}

static animMapType_t AnimMapType( const char *token )
{
	if ( !Q_stricmp( token, "clampanimMap" ) ) { return ANIMMAP_CLAMP; }
	else if ( !Q_stricmp( token, "oneshotanimMap" ) ) { return ANIMMAP_ONESHOT; }
	else { return ANIMMAP_NORMAL; }
}

static const char *animMapNames[] = {
	"animMap",
	"clapanimMap",
	"oneshotanimMap"
};

static bool ParseSurfaceSprites( const char *buffer, shaderStage_t *stage )
{
	const char *token;
	const char **text = &buffer;
	surfaceSpriteType_t sstype = SURFSPRITE_NONE;

	// spritetype
	token = COM_ParseExt( text, qfalse );
	if ( token[0] == '\0' )
	{
		ri.Printf(PRINT_ALL,
				S_COLOR_YELLOW "WARNING: missing surfaceSprites params in shader '%s'\n",
				shader.name);
		return false;
	}

	if ( !Q_stricmp(token, "vertical") )
	{
		sstype = SURFSPRITE_VERTICAL;
	}
	else if ( !Q_stricmp(token, "oriented") )
	{
		sstype = SURFSPRITE_ORIENTED;
	}
	else if ( !Q_stricmp(token, "effect") )
	{
		sstype = SURFSPRITE_EFFECT;
	}
	else if ( !Q_stricmp(token, "flattened") )
	{
		sstype = SURFSPRITE_FLATTENED;
	}
	else
	{
		ri.Printf(PRINT_ALL,
				S_COLOR_YELLOW "WARNING: invalid type in shader '%s'\n",
				shader.name);
		return false;
	}

	// width
	token = COM_ParseExt( text, qfalse );
	if ( token[0] == '\0' )
	{
		ri.Printf(PRINT_ALL,
				S_COLOR_YELLOW "WARNING: missing surfaceSprites params in shader '%s'\n",
				shader.name);
		return false;
	}

	float width = atof(token);
	if ( width <= 0.0f )
	{
		ri.Printf(PRINT_ALL,
				S_COLOR_YELLOW "WARNING: invalid width in shader '%s'\n",
				shader.name);
		return false;
	}

	// height
	token = COM_ParseExt( text, qfalse );
	if ( token[0] == '\0' )
	{
		ri.Printf(PRINT_ALL,
				S_COLOR_YELLOW "WARNING: missing surfaceSprites params in shader '%s'\n",
				shader.name);
		return false;
	}

	float height = atof(token);
	if ( height <= 0.0f )
	{
		ri.Printf(PRINT_ALL,
				S_COLOR_YELLOW "WARNING: invalid height in shader '%s'\n",
				shader.name);
		return false;
	}

	// density
	token = COM_ParseExt( text, qfalse );
	if ( token[0] == '\0' )
	{
		ri.Printf(PRINT_ALL,
				S_COLOR_YELLOW "WARNING: missing surfaceSprites params in shader '%s'\n",
				shader.name);
		return false;
	}

	float density = atof(token);
	if ( density <= 0.0f )
	{
		ri.Printf(PRINT_ALL,
				S_COLOR_YELLOW "WARNING: invalid density in shader '%s'\n",
				shader.name);
		return false;
	}

	// fadedist
	token = COM_ParseExt( text, qfalse );
	if ( token[0] == '\0' )
	{
		ri.Printf(PRINT_ALL,
				S_COLOR_YELLOW "WARNING: missing surfaceSprites params in shader '%s'\n",
				shader.name);
		return false;
	}

	float fadedist = atof(token);
	if ( fadedist < 32.0f )
	{
		ri.Printf(PRINT_ALL,
				S_COLOR_YELLOW "WARNING: invalid fadedist (%.2f < 32) in shader '%s'\n",
				fadedist, shader.name);
		return false;
	}

	if ( !stage->ss )
	{
		stage->ss = (surfaceSprite_t *)Hunk_Alloc( sizeof( surfaceSprite_t ), h_low );

	}

	// These are all set by the command lines.
	stage->ss->type = sstype;
	stage->ss->width = width;
	stage->ss->height = height;
	stage->ss->density = density;
	stage->ss->fadeDist = fadedist;

	// These are defaults that can be overwritten.
	stage->ss->fadeMax = fadedist * 1.33f;
	stage->ss->fadeScale = 0.0f;
	stage->ss->wind = 0.0f;
	stage->ss->windIdle = 0.0f;
	stage->ss->variance[0] = 0.0f;
	stage->ss->variance[1] = 0.0f;
	stage->ss->facing = SURFSPRITE_FACING_NORMAL;

	// A vertical parameter that needs a default regardless
	stage->ss->vertSkew = 0.0f;

	// These are effect parameters that need defaults nonetheless.
	stage->ss->fxDuration = 1000;		// 1 second
	stage->ss->fxGrow[0] = 0.0f;
	stage->ss->fxGrow[1] = 0.0f;
	stage->ss->fxAlphaStart = 1.0f;
	stage->ss->fxAlphaEnd = 0.0f;

	return true;
}

// Parses the following keywords in a shader stage:
//
// 		ssFademax <fademax>
// 		ssFadescale <fadescale>
// 		ssVariance <varwidth> <varheight>
// 		ssHangdown
// 		ssAnyangle
// 		ssFaceup
// 		ssWind <wind>
// 		ssWindIdle <windidle>
// 		ssVertSkew <skew>
// 		ssFXDuration <duration>
// 		ssFXGrow <growwidth> <growheight>
// 		ssFXAlphaRange <alphastart> <startend>
// 		ssFXWeather
static bool ParseSurfaceSpritesOptional(
		const char *param,
		const char *buffer,
		shaderStage_t *stage
)
{
	const char *token;
	const char **text = &buffer;
	float value;

	if (!stage->ss)
	{
		stage->ss = (surfaceSprite_t *)Hunk_Alloc( sizeof( surfaceSprite_t ), h_low );
	}

	// TODO: Tidy this up some how. There's a lot of repeated code

	//
	// fademax
	//
	if (!Q_stricmp(param, "ssFademax"))
	{
		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfacesprite fademax in shader '%s'\n", shader.name );
			return false;
		}
		value = atof(token);
		if (value <= stage->ss->fadeDist)
		{
			ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid surfacesprite fademax (%.2f <= fadeDist(%.2f)) in shader '%s'\n", value, stage->ss->fadeDist, shader.name );
			return false;
		}
		stage->ss->fadeMax=value;
		return true;
	}

	//
	// fadescale
	//
	if (!Q_stricmp(param, "ssFadescale"))
	{
		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfacesprite fadescale in shader '%s'\n", shader.name );
			return false;
		}
		value = atof(token);
		stage->ss->fadeScale=value;
		return true;
	}

	//
	// variance
	//
	if (!Q_stricmp(param, "ssVariance"))
	{
		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfacesprite variance width in shader '%s'\n", shader.name );
			return false;
		}
		value = atof(token);
		if (value < 0)
		{
			ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid surfacesprite variance width in shader '%s'\n", shader.name );
			return false;
		}
		stage->ss->variance[0]=value;

		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfacesprite variance height in shader '%s'\n", shader.name );
			return false;
		}
		value = atof(token);
		if (value < 0)
		{
			ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid surfacesprite variance height in shader '%s'\n", shader.name );
			return false;
		}
		stage->ss->variance[1]=value;
		return true;
	}

	//
	// hangdown
	//
	if (!Q_stricmp(param, "ssHangdown"))
	{
		if (stage->ss->facing != SURFSPRITE_FACING_NORMAL)
		{
			ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: Hangdown facing overrides previous facing in shader '%s'\n", shader.name );
			return false;
		}
		stage->ss->facing=SURFSPRITE_FACING_DOWN;
		return true;
	}

	//
	// anyangle
	//
	if (!Q_stricmp(param, "ssAnyangle"))
	{
		if (stage->ss->facing != SURFSPRITE_FACING_NORMAL)
		{
			ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: Anyangle facing overrides previous facing in shader '%s'\n", shader.name );
			return false;
		}
		stage->ss->facing=SURFSPRITE_FACING_ANY;
		return true;
	}

	//
	// faceup
	//
	if (!Q_stricmp(param, "ssFaceup"))
	{
		if (stage->ss->facing != SURFSPRITE_FACING_NORMAL)
		{
			ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: Faceup facing overrides previous facing in shader '%s'\n", shader.name );
			return false;
		}
		stage->ss->facing=SURFSPRITE_FACING_UP;
		return true;
	}

	//
	// wind
	//
	if (!Q_stricmp(param, "ssWind"))
	{
		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfacesprite wind in shader '%s'\n", shader.name );
			return false;
		}
		value = atof(token);
		if (value < 0.0)
		{
			ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid surfacesprite wind in shader '%s'\n", shader.name );
			return false;
		}
		stage->ss->wind=value;
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
		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfacesprite windidle in shader '%s'\n", shader.name );
			return false;
		}
		value = atof(token);
		if (value < 0.0)
		{
			ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid surfacesprite windidle in shader '%s'\n", shader.name );
			return false;
		}
		stage->ss->windIdle=value;
		return true;
	}

	//
	// vertskew
	//
	if (!Q_stricmp(param, "ssVertskew"))
	{
		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfacesprite vertskew in shader '%s'\n", shader.name );
			return false;
		}
		value = atof(token);
		if (value < 0.0)
		{
			ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid surfacesprite vertskew in shader '%s'\n", shader.name );
			return false;
		}
		stage->ss->vertSkew=value;
		return true;
	}

	//
	// fxduration
	//
	if (!Q_stricmp(param, "ssFXDuration"))
	{
		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfacesprite duration in shader '%s'\n", shader.name );
			return false;
		}
		value = atof(token);
		if (value <= 0)
		{
			ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid surfacesprite duration in shader '%s'\n", shader.name );
			return false;
		}
		stage->ss->fxDuration=value;
		return true;
	}

	//
	// fxgrow
	//
	if (!Q_stricmp(param, "ssFXGrow"))
	{
		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfacesprite grow width in shader '%s'\n", shader.name );
			return false;
		}
		value = atof(token);
		if (value < 0)
		{
			ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid surfacesprite grow width in shader '%s'\n", shader.name );
			return false;
		}
		stage->ss->fxGrow[0]=value;

		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfacesprite grow height in shader '%s'\n", shader.name );
			return false;
		}
		value = atof(token);
		if (value < 0)
		{
			ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid surfacesprite grow height in shader '%s'\n", shader.name );
			return false;
		}
		stage->ss->fxGrow[1]=value;
		return true;
	}

	//
	// fxalpharange
	//
	if (!Q_stricmp(param, "ssFXAlphaRange"))
	{
		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfacesprite fxalpha start in shader '%s'\n", shader.name );
			return false;
		}
		value = atof(token);
		if (value < 0 || value > 1.0)
		{
			ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid surfacesprite fxalpha start in shader '%s'\n", shader.name );
			return false;
		}
		stage->ss->fxAlphaStart=value;

		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfacesprite fxalpha end in shader '%s'\n", shader.name );
			return false;
		}
		value = atof(token);
		if (value < 0 || value > 1.0)
		{
			ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid surfacesprite fxalpha end in shader '%s'\n", shader.name );
			return false;
		}
		stage->ss->fxAlphaEnd=value;
		return true;
	}

	//
	// fxweather
	//
	if (!Q_stricmp(param, "ssFXWeather"))
	{
		if (stage->ss->type != SURFSPRITE_EFFECT)
		{
			ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: weather applied to non-effect surfacesprite in shader '%s'\n", shader.name );
			return false;
		}
		stage->ss->type = SURFSPRITE_WEATHERFX;
		return true;
	}

	//
	// invalid ss command.
	//
	ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid optional surfacesprite param '%s' in shader '%s'\n", param, shader.name );
	return false;
}

/*
===================
ParseStage
===================
*/
static qboolean ParseStage( shaderStage_t *stage, const char **text )
{
	char *token;
	unsigned depthMaskBits = GLS_DEPTHMASK_TRUE;
	unsigned blendSrcBits = 0;
	unsigned blendDstBits = 0;
	unsigned depthFuncBits = 0;
	qboolean depthMaskExplicit = qfalse;
	char bufferPackedTextureName[MAX_QPATH];
	char bufferBaseColorTextureName[MAX_QPATH];

	stage->active = qtrue;
	stage->specularType = SPEC_NONE;

	while ( 1 )
	{
		token = COM_ParseExt( text, qtrue );
		if ( !token[0] )
		{
			ri.Printf( PRINT_WARNING, "WARNING: no matching '}' found\n" );
			return qfalse;
		}

		if ( token[0] == '}' )
		{
			break;
		}
		//
		// map <name> or clampmap <name>
		//
		else if ( !Q_stricmp( token, "map" ) || (!Q_stricmp(token, "clampmap")))
		{
			int flags = !Q_stricmp(token, "clampmap") ? IMGFLAG_CLAMPTOEDGE : IMGFLAG_NONE;

			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'map' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}

			if ( !Q_stricmp( token, "$whiteimage" ) )
			{
				stage->bundle[0].image[0] = tr.whiteImage;
				continue;
			}
			else if ( !Q_stricmp( token, "$lightmap" ) )
			{
				stage->bundle[0].isLightmap = qtrue;
				if ( shader.lightmapIndex[0] < 0 || shader.lightmapIndex[0] >= tr.numLightmaps ) {
#ifndef FINAL_BUILD
					ri.Printf (PRINT_ALL, S_COLOR_RED "Lightmap requested but none avilable for shader '%s'\n", shader.name);
#endif
					stage->bundle[0].image[0] = tr.whiteImage;
				} else {
					stage->bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex[0]];
					if (r_deluxeMapping->integer && tr.worldDeluxeMapping)
						stage->bundle[TB_DELUXEMAP].image[0] = tr.deluxemaps[shader.lightmapIndex[0]];
				}
				continue;
			}
			else if ( !Q_stricmp( token, "$deluxemap" ) )
			{
				if (!tr.worldDeluxeMapping)
				{
					ri.Printf( PRINT_WARNING, "WARNING: shader '%s' wants a deluxe map in a map compiled without them\n", shader.name );
					return qfalse;
				}

				stage->bundle[0].isLightmap = qtrue;
				if ( shader.lightmapIndex[0] < 0 || shader.lightmapIndex[0] >= tr.numLightmaps ) {
					stage->bundle[0].image[0] = tr.whiteImage;
				} else {
					stage->bundle[0].image[0] = tr.deluxemaps[shader.lightmapIndex[0]];
				}
				continue;
			}
			else
			{
				imgType_t type = IMGTYPE_COLORALPHA;

				if (!shader.noMipMaps)
					flags |= IMGFLAG_MIPMAP;

				if (!shader.noPicMip)
					flags |= IMGFLAG_PICMIP;

				if (shader.noTC)
					flags |= IMGFLAG_NO_COMPRESSION;

				if (r_genNormalMaps->integer)
					flags |= IMGFLAG_GENNORMALMAP;

				if (shader.isHDRLit == qtrue)
					flags |= IMGFLAG_SRGB;

				Q_strncpyz(bufferBaseColorTextureName, token, sizeof(bufferBaseColorTextureName));
				stage->bundle[0].image[0] = R_FindImageFile(bufferBaseColorTextureName, type, flags);

				if ( !stage->bundle[0].image[0] )
				{
					ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
					return qfalse;
				}
			}
		}
		//
		// normalMap <name> or normalHeightMap <name>
		//
		else if (!Q_stricmp(token, "normalMap") || !Q_stricmp(token, "normalHeightMap"))
		{
			imgType_t type = !Q_stricmp(token, "normalHeightMap") ? IMGTYPE_NORMALHEIGHT : IMGTYPE_NORMAL;

			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				ri.Printf(PRINT_WARNING, "WARNING: missing parameter for 'normalMap' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}

			int flags = IMGFLAG_NONE;

			if (!shader.noMipMaps)
				flags |= IMGFLAG_MIPMAP;

			if (!shader.noPicMip)
				flags |= IMGFLAG_PICMIP;

			if (shader.noTC)
				flags |= IMGFLAG_NO_COMPRESSION;

			flags |= IMGFLAG_NOLIGHTSCALE;
			stage->bundle[TB_NORMALMAP].image[0] = R_FindImageFile(token, type, flags);

			if (!stage->bundle[TB_NORMALMAP].image[0])
			{
				ri.Printf(PRINT_WARNING, "WARNING: R_FindImageFile could not find normalMap '%s' in shader '%s'\n", token, shader.name);
				return qfalse;
			}

			VectorSet4(stage->normalScale, r_baseNormalX->value, r_baseNormalY->value, 1.0f, r_baseParallax->value);
		}
		//
		// specMap <name> || specularMap <name>
		//
		else if (!Q_stricmp(token, "specMap") || !Q_stricmp(token, "specularMap"))
		{
			imgType_t type = IMGTYPE_COLORALPHA;

			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				ri.Printf(PRINT_WARNING, "WARNING: missing parameter for 'specularMap' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}

			stage->specularType = SPEC_SPECGLOSS;
			VectorSet4(stage->specularScale, 1.0f, 1.0f, 1.0f, 0.0f);
			if (!Q_stricmp(token, "$whiteimage"))
			{
				stage->bundle[TB_SPECULARMAP].image[0] = tr.whiteImage;
				continue;
			}

			int flags = IMGFLAG_NONE;

			if (!shader.noMipMaps)
				flags |= IMGFLAG_MIPMAP;

			if (!shader.noPicMip)
				flags |= IMGFLAG_PICMIP;

			if (shader.noTC)
				flags |= IMGFLAG_NO_COMPRESSION;

			// Always srgb to have correct reflectivity
			if (shader.isHDRLit == qtrue)
				stage->bundle[TB_SPECULARMAP].image[0] = R_FindImageFile(token, type, flags | IMGFLAG_SRGB);
			else
				stage->bundle[TB_SPECULARMAP].image[0] = R_BuildSDRSpecGlossImage(stage, token, flags);

			if (!stage->bundle[TB_SPECULARMAP].image[0])
			{
				ri.Printf(PRINT_WARNING, "WARNING: R_FindImageFile could not find specMap '%s' in shader '%s'\n", token, shader.name);
				return qfalse;
			}

		}
		//
		// rmoMap <name> || rmosMap <name>
		//
		else if (!Q_stricmp(token, "rmoMap") || !Q_stricmp(token, "rmosMap"))
		{
			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				ri.Printf(PRINT_WARNING, "WARNING: missing parameter for 'rmoMap' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}
			stage->specularType = !Q_stricmp(token, "rmosMap") ? SPEC_RMOS : SPEC_RMO;
			if (!Q_stricmp(token, "$whiteimage"))
			{
				stage->bundle[TB_SPECULARMAP].image[0] = tr.whiteImage;
				continue;
			}
			Q_strncpyz(bufferPackedTextureName, token, sizeof(bufferPackedTextureName));
		}
		//
		// moxrMap <name> || mosrMap <name>
		//
		else if (!Q_stricmp(token, "moxrMap") || !Q_stricmp(token, "mosrMap"))
		{
			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				ri.Printf(PRINT_WARNING, "WARNING: missing parameter for 'moxrMap' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}
			stage->specularType = !Q_stricmp(token, "mosrMap") ? SPEC_MOSR : SPEC_MOXR;
			if (!Q_stricmp(token, "$whiteimage"))
			{
				stage->bundle[TB_SPECULARMAP].image[0] = tr.whiteImage;
				continue;
			}
			Q_strncpyz(bufferPackedTextureName, token, sizeof(bufferPackedTextureName));
		}
		//
		// ormMap <name> || ormsMap <name>
		//
		else if (!Q_stricmp(token, "ormMap") || !Q_stricmp(token, "ormsMap"))
		{
			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				ri.Printf(PRINT_WARNING, "WARNING: missing parameter for 'ormMap' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}
			stage->specularType = !Q_stricmp(token, "ormsMap") ? SPEC_ORMS : SPEC_ORM;
			if (!Q_stricmp(token, "$whiteimage"))
			{
				stage->bundle[TB_SPECULARMAP].image[0] = tr.whiteImage;
				continue;
			}
			Q_strncpyz(bufferPackedTextureName, token, sizeof(bufferPackedTextureName));
		}
		//
		// animMap <frequency> <image1> .... <imageN>
		//
		else if ( !Q_stricmp( token, "animMap" ) || !Q_stricmp( token, "clampanimMap" ) || !Q_stricmp( token, "oneshotanimMap" ) )
		{
			animMapType_t type = AnimMapType( token );
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for '%s' keyword in shader '%s'\n", animMapNames[type], shader.name );
				return qfalse;
			}
			stage->bundle[0].imageAnimationSpeed = atof( token );
			stage->bundle[0].oneShotAnimMap = (qboolean)(type == ANIMMAP_ONESHOT);

			// parse up to MAX_IMAGE_ANIMATIONS animations
			while ( 1 ) {
				int		num;

				token = COM_ParseExt( text, qfalse );
				if ( !token[0] ) {
					break;
				}
				num = stage->bundle[0].numImageAnimations;
				if ( num < MAX_IMAGE_ANIMATIONS ) {
					imgType_t imgtype = IMGTYPE_COLORALPHA;
					int flags = type == ANIMMAP_CLAMP ? IMGFLAG_CLAMPTOEDGE : IMGFLAG_NONE;

					if (!shader.noMipMaps)
						flags |= IMGFLAG_MIPMAP;

					if (!shader.noPicMip)
						flags |= IMGFLAG_PICMIP;

					if (shader.isHDRLit == qtrue)
						flags |= IMGFLAG_SRGB;

					if (shader.noTC)
						flags |= IMGFLAG_NO_COMPRESSION;

					if (r_genNormalMaps->integer)
						flags |= IMGFLAG_GENNORMALMAP;

					stage->bundle[0].image[num] = R_FindImageFile( token, imgtype, flags );
					if ( !stage->bundle[0].image[num] )
					{
						ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
						return qfalse;
					}
					stage->bundle[0].numImageAnimations++;
				}
			}
		}
		else if ( !Q_stricmp( token, "videoMap" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'videoMap' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}
			stage->bundle[0].videoMapHandle = ri.CIN_PlayCinematic( token, 0, 0, 256, 256, (CIN_loop | CIN_silent | CIN_shader));
			if (stage->bundle[0].videoMapHandle != -1) {
				stage->bundle[0].isVideoMap = qtrue;
				stage->bundle[0].image[0] = tr.scratchImage[stage->bundle[0].videoMapHandle];
			}
		}
		//
		// alphafunc <func>
		//
		else if ( !Q_stricmp( token, "alphaFunc" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'alphaFunc' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			ParseAlphaTestFunc( stage, token );
		}
		//
		// depthFunc <func>
		//
		else if ( !Q_stricmp( token, "depthfunc" ) )
		{
			token = COM_ParseExt( text, qfalse );

			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'depthfunc' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			if ( !Q_stricmp( token, "lequal" ) )
			{
				depthFuncBits = 0;
			}
			else if ( !Q_stricmp( token, "equal" ) )
			{
				depthFuncBits = GLS_DEPTHFUNC_EQUAL;
			}
			else if ( !Q_stricmp( token, "disable" ) )
			{
				depthFuncBits = GLS_DEPTHTEST_DISABLE;
			}
			else
			{
				ri.Printf( PRINT_WARNING, "WARNING: unknown depthfunc '%s' in shader '%s'\n", token, shader.name );
				continue;
			}
		}
		//
		// detail
		//
		else if ( !Q_stricmp( token, "detail" ) )
		{
			stage->isDetail = qtrue;
		}
		//
		// blendfunc <srcFactor> <dstFactor>
		// or blendfunc <add|filter|blend>
		//
		else if ( !Q_stricmp( token, "blendfunc" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parm for blendFunc in shader '%s'\n", shader.name );
				continue;
			}
			// check for "simple" blends first
			if ( !Q_stricmp( token, "add" ) ) {
				blendSrcBits = GLS_SRCBLEND_ONE;
				blendDstBits = GLS_DSTBLEND_ONE;
			} else if ( !Q_stricmp( token, "filter" ) ) {
				blendSrcBits = GLS_SRCBLEND_DST_COLOR;
				blendDstBits = GLS_DSTBLEND_ZERO;
			} else if ( !Q_stricmp( token, "blend" ) ) {
				blendSrcBits = GLS_SRCBLEND_SRC_ALPHA;
				blendDstBits = GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
			} else {
				// complex double blends
				blendSrcBits = NameToSrcBlendMode( token );

				token = COM_ParseExt( text, qfalse );
				if ( token[0] == 0 )
				{
					ri.Printf( PRINT_WARNING, "WARNING: missing parm for blendFunc in shader '%s'\n", shader.name );
					continue;
				}
				blendDstBits = NameToDstBlendMode( token );
			}

			// clear depth mask for blended surfaces
			if ( !depthMaskExplicit )
			{
				depthMaskBits = 0;
			}
		}
		//
		// specularReflectance <value>
		//
		else if (!Q_stricmp(token, "specularreflectance"))
		{
			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for specular reflectance in shader '%s'\n", shader.name );
				continue;
			}
			stage->specularScale[0] =
			stage->specularScale[1] =
			stage->specularScale[2] = Com_Clamp( 0.0f, 1.0f, atof( token ) );
		}
		//
		// specularExponent <value>
		//
		else if (!Q_stricmp(token, "specularexponent"))
		{
			float exponent;

			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for specular exponent in shader '%s'\n", shader.name );
				continue;
			}
			exponent = atof( token );

			// Change shininess to gloss
			// FIXME: assumes max exponent of 8192 and min of 1, must change here if altered in lightall_fp.glsl
			exponent = CLAMP(exponent, 1.0, 8192.0);
			stage->specularScale[3] = 1.0f - (log(exponent) / log(8192.0));
		}
		//
		// gloss <value>
		//
		else if ( !Q_stricmp( token, "gloss" ) )
		{
			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for gloss in shader '%s'\n", shader.name );
				continue;
			}

			stage->specularScale[3] = 1.0 - atof( token );
		}
		//
		// roughness <value>
		//
		else if (!Q_stricmp(token, "roughness"))
		{
			token = COM_ParseExt(text, qfalse);
			if (token[0] == 0)
			{
				ri.Printf(PRINT_WARNING, "WARNING: missing parameter for roughness in shader '%s'\n", shader.name);
				continue;
			}

			stage->specularScale[3] = atof(token);
		}
		//
		// parallaxDepth <value>
		//
		else if (!Q_stricmp(token, "parallaxdepth"))
		{
			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for parallaxDepth in shader '%s'\n", shader.name );
				continue;
			}

			stage->normalScale[3] = atof( token );
		}
		//
		// parallaxBias <value>
		//
		else if (!Q_stricmp(token, "parallaxbias"))
		{
			token = COM_ParseExt(text, qfalse);
			if (token[0] == 0)
			{
				ri.Printf(PRINT_WARNING, "WARNING: missing parameter for parallaxBias in shader '%s'\n", shader.name);
				continue;
			}

			stage->parallaxBias = atof(token);
		}
		//
		// normalScale <xy>
		// or normalScale <x> <y>
		// or normalScale <x> <y> <height>
		//
		else if (!Q_stricmp(token, "normalscale"))
		{
			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for normalScale in shader '%s'\n", shader.name );
				continue;
			}

			stage->normalScale[0] = atof( token );

			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				// one value, applies to X/Y
				stage->normalScale[1] = stage->normalScale[0];
				continue;
			}

			stage->normalScale[1] = atof( token );

			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				// two values, no height
				continue;
			}

			stage->normalScale[3] = atof( token );
		}
		//
		// specularScale <rgb> <gloss>
		// or specularScale <r> <g> <b>
		// or specularScale <r> <g> <b> <gloss>
		// or specularScale <metalness> <specular> <unused> <gloss> when metal roughness workflow is used
		//
		else if (!Q_stricmp(token, "specularscale"))
		{
			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for specularScale in shader '%s'\n", shader.name );
				continue;
			}

			stage->specularScale[0] = atof( token );

			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for specularScale in shader '%s'\n", shader.name );
				continue;
			}

			stage->specularScale[1] = atof( token );

			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				// two values, rgb then gloss
				stage->specularScale[3] = 1.0 - stage->specularScale[1];
				stage->specularScale[1] =
				stage->specularScale[2] = stage->specularScale[0];
				continue;
			}

			stage->specularScale[2] = atof( token );

			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				// three values, rgb
				continue;
			}

			stage->specularScale[3] = 1.0 - atof( token );
		}
		//
		// rgbGen
		//
		else if ( !Q_stricmp( token, "rgbGen" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameters for rgbGen in shader '%s'\n", shader.name );
				continue;
			}

			if ( !Q_stricmp( token, "wave" ) )
			{
				ParseWaveForm( text, &stage->rgbWave );
				stage->rgbGen = CGEN_WAVEFORM;
			}
			else if ( !Q_stricmp( token, "const" ) )
			{
				vec3_t	color;

				ParseVector( text, 3, color );
				stage->constantColor[0] = color[0];
				stage->constantColor[1] = color[1];
				stage->constantColor[2] = color[2];
				if (tr.hdrLighting == qfalse)
				{
					stage->constantColor[0] = MIN(color[0], 1.0f);
					stage->constantColor[1] = MIN(color[1], 1.0f);
					stage->constantColor[2] = MIN(color[2], 1.0f);
				}

				stage->rgbGen = CGEN_CONST;
			}
			else if ( !Q_stricmp( token, "identity" ) )
			{
				stage->rgbGen = CGEN_IDENTITY;
			}
			else if ( !Q_stricmp( token, "identityLighting" ) )
			{
				stage->rgbGen = CGEN_IDENTITY_LIGHTING;
			}
			else if ( !Q_stricmp( token, "entity" ) )
			{
				stage->rgbGen = CGEN_ENTITY;
			}
			else if ( !Q_stricmp( token, "oneMinusEntity" ) )
			{
				stage->rgbGen = CGEN_ONE_MINUS_ENTITY;
			}
			else if ( !Q_stricmp( token, "vertex" ) )
			{
				stage->rgbGen = CGEN_VERTEX;
				if ( stage->alphaGen == 0 ) {
					stage->alphaGen = AGEN_VERTEX;
				}
			}
			else if ( !Q_stricmp( token, "exactVertex" ) )
			{
				stage->rgbGen = CGEN_EXACT_VERTEX;
			}
			else if ( !Q_stricmp( token, "vertexLit" ) )
			{
				stage->rgbGen = CGEN_VERTEX_LIT;
				if ( stage->alphaGen == 0 ) {
					stage->alphaGen = AGEN_VERTEX;
				}
			}
			else if ( !Q_stricmp( token, "exactVertexLit" ) )
			{
				stage->rgbGen = CGEN_EXACT_VERTEX_LIT;
			}
			else if ( !Q_stricmp( token, "lightingDiffuse" ) )
			{
				stage->rgbGen = CGEN_LIGHTING_DIFFUSE;
			}
			else if ( !Q_stricmp( token, "lightingDiffuseEntity" ) )
			{
				if (shader.lightmapIndex[0] != LIGHTMAP_NONE)
				{
					Com_Printf( S_COLOR_RED "ERROR: rgbGen lightingDiffuseEntity used on a misc_model! in shader '%s'\n", shader.name );
				}
				stage->rgbGen = CGEN_LIGHTING_DIFFUSE_ENTITY;
			}
			else if ( !Q_stricmp( token, "oneMinusVertex" ) )
			{
				stage->rgbGen = CGEN_ONE_MINUS_VERTEX;
			}
			else
			{
				ri.Printf( PRINT_WARNING, "WARNING: unknown rgbGen parameter '%s' in shader '%s'\n", token, shader.name );
				continue;
			}
		}
		//
		// alphaGen
		//
		else if ( !Q_stricmp( token, "alphaGen" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameters for alphaGen in shader '%s'\n", shader.name );
				continue;
			}

			if ( !Q_stricmp( token, "wave" ) )
			{
				ParseWaveForm( text, &stage->alphaWave );
				stage->alphaGen = AGEN_WAVEFORM;
			}
			else if ( !Q_stricmp( token, "const" ) )
			{
				token = COM_ParseExt( text, qfalse );
				stage->constantColor[3] = atof( token );
				stage->alphaGen = AGEN_CONST;
			}
			else if ( !Q_stricmp( token, "identity" ) )
			{
				stage->alphaGen = AGEN_IDENTITY;
			}
			else if ( !Q_stricmp( token, "entity" ) )
			{
				stage->alphaGen = AGEN_ENTITY;
			}
			else if ( !Q_stricmp( token, "oneMinusEntity" ) )
			{
				stage->alphaGen = AGEN_ONE_MINUS_ENTITY;
			}
			else if ( !Q_stricmp( token, "vertex" ) )
			{
				stage->alphaGen = AGEN_VERTEX;
			}
			else if ( !Q_stricmp( token, "lightingSpecular" ) )
			{
				stage->alphaGen = AGEN_LIGHTING_SPECULAR;
			}
			else if ( !Q_stricmp( token, "oneMinusVertex" ) )
			{
				stage->alphaGen = AGEN_ONE_MINUS_VERTEX;
			}
			else if ( !Q_stricmp( token, "dot" ) )
			{
				//stage->alphaGen = AGEN_DOT;
			}
			else if ( !Q_stricmp( token, "oneMinusDot" ) )
			{
				//stage->alphaGen = AGEN_ONE_MINUS_DOT;
			}
			else if ( !Q_stricmp( token, "portal" ) )
			{
				stage->alphaGen = AGEN_PORTAL;
				token = COM_ParseExt( text, qfalse );
				if ( token[0] == 0 )
				{
					shader.portalRange = 256;
					ri.Printf( PRINT_WARNING, "WARNING: missing range parameter for alphaGen portal in shader '%s', defaulting to 256\n", shader.name );
				}
				else
				{
					shader.portalRange = atof( token );
				}
			}
			else
			{
				ri.Printf( PRINT_WARNING, "WARNING: unknown alphaGen parameter '%s' in shader '%s'\n", token, shader.name );
				continue;
			}
		}
		//
		// tcGen <function>
		//
		else if ( !Q_stricmp(token, "texgen") || !Q_stricmp( token, "tcGen" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing texgen parm in shader '%s'\n", shader.name );
				continue;
			}

			if ( !Q_stricmp( token, "environment" ) )
			{
				stage->bundle[0].tcGen = TCGEN_ENVIRONMENT_MAPPED;
			}
			else if ( !Q_stricmp( token, "lightmap" ) )
			{
				stage->bundle[0].tcGen = TCGEN_LIGHTMAP;
			}
			else if ( !Q_stricmp( token, "texture" ) || !Q_stricmp( token, "base" ) )
			{
				stage->bundle[0].tcGen = TCGEN_TEXTURE;
			}
			else if ( !Q_stricmp( token, "vector" ) )
			{
				ParseVector( text, 3, stage->bundle[0].tcGenVectors[0] );
				ParseVector( text, 3, stage->bundle[0].tcGenVectors[1] );

				stage->bundle[0].tcGen = TCGEN_VECTOR;
			}
			else
			{
				ri.Printf( PRINT_WARNING, "WARNING: unknown texgen parm in shader '%s'\n", shader.name );
			}
		}
		//
		// tcMod <type> <...>
		//
		else if ( !Q_stricmp( token, "tcMod" ) )
		{
			char buffer[1024] = "";

			while ( 1 )
			{
				token = COM_ParseExt( text, qfalse );
				if ( token[0] == 0 )
					break;
				Q_strcat( buffer, sizeof( buffer ), token );
				Q_strcat( buffer, sizeof( buffer ), " " );
			}

			ParseTexMod( buffer, stage );

			continue;
		}
		//
		// depthmask
		//
		else if ( !Q_stricmp( token, "depthwrite" ) )
		{
			depthMaskBits = GLS_DEPTHMASK_TRUE;
			depthMaskExplicit = qtrue;

			continue;
		}
		// If this stage has glow...	GLOWXXX
		else if ( Q_stricmp( token, "glow" ) == 0 )
		{
			stage->glow = qtrue;

			continue;
		}
		//
		// If this stage is cloth
		else if (Q_stricmp(token, "cloth") == 0)
		{
			stage->cloth = qtrue;

			continue;
		}
		// surfaceSprites <type> ...
		//
		else if ( !Q_stricmp( token, "surfacesprites" ) )
		{
			char buffer[1024] = {};

			while ( 1 )
			{
				token = COM_ParseExt( text, qfalse );
				if ( token[0] == 0 )
					break;
				Q_strcat( buffer, sizeof( buffer ), token );
				Q_strcat( buffer, sizeof( buffer ), " " );
			}

			bool hasSS = (stage->ss != nullptr);
			if ( ParseSurfaceSprites( buffer, stage ) && !hasSS )
			{
				++shader.numSurfaceSpriteStages;
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
		else if (!Q_stricmpn(token, "ss", 2))
		{
			char buffer[1024] = {};
			char param[128] = {};
			Q_strncpyz( param, token, sizeof( param ) );

			while ( 1 )
			{
				token = COM_ParseExt( text, qfalse );
				if ( token[0] == '\0' )
					break;
				Q_strcat( buffer, sizeof( buffer ), token );
				Q_strcat( buffer, sizeof( buffer ), " " );
			}

			bool hasSS = (stage->ss != nullptr);
			if ( ParseSurfaceSpritesOptional( param, buffer, stage ) && !hasSS )
			{
				++shader.numSurfaceSpriteStages;
			}

			continue;
		}
		else
		{
			ri.Printf( PRINT_WARNING, "WARNING: unknown parameter '%s' in shader '%s'\n", token, shader.name );
			return qfalse;
		}
	}

	//
	// load packed textures
	//
	if (stage->specularType != SPEC_SPECGLOSS && stage->specularType != SPEC_NONE)
	{
		int flags = IMGFLAG_NOLIGHTSCALE;

		if (!shader.noMipMaps)
			flags |= IMGFLAG_MIPMAP;

		if (!shader.noPicMip)
			flags |= IMGFLAG_PICMIP;

		if (shader.noTC)
			flags |= IMGFLAG_NO_COMPRESSION;

		R_LoadPackedMaterialImage(stage, bufferPackedTextureName, flags);
	}

	//
	// search for shader based external lightmaps that were created by q3map2
	//
	if (stage->bundle[0].tcGen == TCGEN_LIGHTMAP && stage->bundle[0].isLightmap == qfalse)
	{
		const char *filename = { va("%s/lm_", tr.worldName) };
		if (!Q_strncmp(filename, bufferBaseColorTextureName, sizeof(filename)))
		{
			if (shader.isHDRLit)
			{
				image_t *hdrImage = R_FindImageFile(bufferBaseColorTextureName, IMGTYPE_COLORALPHA, IMGFLAG_NOLIGHTSCALE | IMGFLAG_HDR_LIGHTMAP | IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE);
				if (hdrImage)
					stage->bundle[0].image[0] = hdrImage;
			}

			stage->bundle[0].isLightmap = qtrue;
			shader.lightmapIndex[0] = LIGHTMAP_EXTERNAL;

			// shader based external lightmaps can't utilize lightstyles
			shader.styles[0] = 0;
			shader.styles[1] = 255;
			shader.styles[2] = 255;
			shader.styles[3] = 255;

			// it obviously receives light, so receive dlights too
			shader.surfaceFlags &= ~SURF_NODLIGHT;
		}
	}

	//
	// if cgen isn't explicitly specified, use either identity or identitylighting
	//
	if ( stage->rgbGen == CGEN_BAD ) {
		if ( blendSrcBits == 0 ||
			blendSrcBits == GLS_SRCBLEND_ONE ||
			blendSrcBits == GLS_SRCBLEND_SRC_ALPHA ) {
			stage->rgbGen = CGEN_IDENTITY_LIGHTING;
		} else {
			stage->rgbGen = CGEN_IDENTITY;
		}
	}


	//
	// implicitly assume that a GL_ONE GL_ZERO blend mask disables blending
	//
	if ( ( blendSrcBits == GLS_SRCBLEND_ONE ) &&
		 ( blendDstBits == GLS_DSTBLEND_ZERO ) )
	{
		blendDstBits = blendSrcBits = 0;
		depthMaskBits = GLS_DEPTHMASK_TRUE;
	}

	// decide which agens we can skip
	if ( stage->alphaGen == AGEN_IDENTITY ) {
		if ( stage->rgbGen == CGEN_IDENTITY
			|| stage->rgbGen == CGEN_LIGHTING_DIFFUSE ) {
			stage->alphaGen = AGEN_SKIP;
		}
	}

	//
	// compute state bits
	//
	stage->stateBits = depthMaskBits |
		               blendSrcBits | blendDstBits |
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
	char	*token;
	deformStage_t	*ds;

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 )
	{
		ri.Printf( PRINT_WARNING, "WARNING: missing deform parm in shader '%s'\n", shader.name );
		return;
	}

	if ( shader.numDeforms == MAX_SHADER_DEFORMS ) {
		ri.Printf( PRINT_WARNING, "WARNING: MAX_SHADER_DEFORMS in '%s'\n", shader.name );
		return;
	}

	ds = &shader.deforms[ shader.numDeforms ];
	shader.numDeforms++;

	if ( !Q_stricmp( token, "projectionShadow" ) ) {
		ds->deformation = DEFORM_PROJECTION_SHADOW;
		return;
	}

	if ( !Q_stricmp( token, "autosprite" ) ) {
		ds->deformation = DEFORM_AUTOSPRITE;
		return;
	}

	if ( !Q_stricmp( token, "autosprite2" ) ) {
		ds->deformation = DEFORM_AUTOSPRITE2;
		return;
	}

	if ( !Q_stricmpn( token, "text", 4 ) ) {
		int		n;

		n = token[4] - '0';
		if ( n < 0 || n > 7 ) {
			n = 0;
		}
		ds->deformation = (deform_t)(DEFORM_TEXT0 + n);
		return;
	}

	if ( !Q_stricmp( token, "bulge" ) )	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name );
			return;
		}
		ds->bulgeWidth = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name );
			return;
		}
		ds->bulgeHeight = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name );
			return;
		}
		ds->bulgeSpeed = atof( token );

		ds->deformation = DEFORM_BULGE;
		return;
	}

	if ( !Q_stricmp( token, "wave" ) )
	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name );
			return;
		}

		if ( atof( token ) != 0 )
		{
			ds->deformationSpread = 1.0f / atof( token );
		}
		else
		{
			ds->deformationSpread = 100.0f;
			ri.Printf( PRINT_WARNING, "WARNING: illegal div value of 0 in deformVertexes command for shader '%s'\n", shader.name );
		}

		ParseWaveForm( text, &ds->deformationWave );
		ds->deformation = DEFORM_WAVE;
		return;
	}

	if ( !Q_stricmp( token, "normal" ) )
	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name );
			return;
		}
		ds->deformationWave.amplitude = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name );
			return;
		}
		ds->deformationWave.frequency = atof( token );

		ds->deformation = DEFORM_NORMALS;
		return;
	}

	if ( !Q_stricmp( token, "move" ) ) {
		int		i;

		for ( i = 0 ; i < 3 ; i++ ) {
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name );
				return;
			}
			ds->moveVector[i] = atof( token );
		}

		ParseWaveForm( text, &ds->deformationWave );
		ds->deformation = DEFORM_MOVE;
		return;
	}

	ri.Printf( PRINT_WARNING, "WARNING: unknown deformVertexes subtype '%s' found in shader '%s'\n", token, shader.name );
}


/*
===============
ParseSkyParms

skyParms <outerbox> <cloudheight> <innerbox>
===============
*/
static void ParseSkyParms( const char **text ) {
	char				*token;
	static const char	*suf[6] = {"rt", "lf", "bk", "ft", "up", "dn"};
	char		pathname[MAX_QPATH];
	int			i;
	int imgFlags = IMGFLAG_MIPMAP | IMGFLAG_PICMIP | IMGFLAG_CLAMPTOEDGE;

	if (shader.noTC)
		imgFlags |= IMGFLAG_NO_COMPRESSION;

	if (tr.hdrLighting == qtrue)
		imgFlags |= IMGFLAG_SRGB | IMGFLAG_HDR | IMGFLAG_NOLIGHTSCALE; // srgb or hdr are requested. If hdr is found, the srgb flag will be ignored

	// outerbox
	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name );
		return;
	}
	if ( strcmp( token, "-" ) ) {
		for (i=0 ; i<6 ; i++) {
			Com_sprintf( pathname, sizeof(pathname), "%s_%s", token, suf[i] );
			shader.sky.outerbox[i] = R_FindImageFile( ( char * ) pathname, IMGTYPE_COLORALPHA, imgFlags );

			if ( !shader.sky.outerbox[i] ) {
				if ( i )
					shader.sky.outerbox[i] = shader.sky.outerbox[i-1];	//not found, so let's use the previous image
				else
					shader.sky.outerbox[i] = tr.defaultImage;
			}
		}
	}

	// cloudheight
	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: 'skyParms' missing cloudheight in shader '%s'\n", shader.name );
		return;
	}
	shader.sky.cloudHeight = atof( token );
	if ( !shader.sky.cloudHeight ) {
		shader.sky.cloudHeight = 512;
	}
	R_InitSkyTexCoords( shader.sky.cloudHeight );

	// innerbox
	token = COM_ParseExt( text, qfalse );
	if ( strcmp( token, "-" ) ) {
		ri.Printf( PRINT_WARNING, "WARNING: in shader '%s' 'skyParms', innerbox is not supported!", shader.name );
	}

	shader.isSky = qtrue;
}


/*
=================
ParseSort
=================
*/
void ParseSort( const char **text ) {
	char	*token;

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: missing sort parameter in shader '%s'\n", shader.name );
		return;
	}

	if ( !Q_stricmp( token, "portal" ) ) {
		shader.sort = SS_PORTAL;
	} else if ( !Q_stricmp( token, "sky" ) ) {
		shader.sort = SS_ENVIRONMENT;
	} else if ( !Q_stricmp( token, "opaque" ) ) {
		shader.sort = SS_OPAQUE;
	} else if ( !Q_stricmp( token, "decal" ) ) {
		shader.sort = SS_DECAL;
	} else if ( !Q_stricmp( token, "seeThrough" ) ) {
		shader.sort = SS_SEE_THROUGH;
	} else if ( !Q_stricmp( token, "banner" ) ) {
		shader.sort = SS_BANNER;
	} else if ( !Q_stricmp( token, "additive" ) ) {
		shader.sort = SS_BLEND1;
	} else if ( !Q_stricmp( token, "nearest" ) ) {
		shader.sort = SS_NEAREST;
	} else if ( !Q_stricmp( token, "underwater" ) ) {
		shader.sort = SS_UNDERWATER;
	} else if ( !Q_stricmp( token, "inside" ) ) {
		shader.sort = SS_INSIDE;
	} else if ( !Q_stricmp( token, "mid_inside" ) ) {
		shader.sort = SS_MID_INSIDE;
	} else if ( !Q_stricmp( token, "middle" ) ) {
		shader.sort = SS_MIDDLE;
	} else if ( !Q_stricmp( token, "mid_outside" ) ) {
		shader.sort = SS_MID_OUTSIDE;
	} else if ( !Q_stricmp( token, "outside" ) ) {
		shader.sort = SS_OUTSIDE;
	}
	else {
		shader.sort = atof( token );
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

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 )
	{
		Com_Printf (S_COLOR_YELLOW  "WARNING: missing material in shader '%s'\n", shader.name );
		return;
	}
	for(i = 0; i < MATERIAL_LAST; i++)
	{
		if ( !Q_stricmp( token, materialNames[i] ) )
		{
			shader.surfaceFlags |= i;
			break;
		}
	}
}


// this table is also present in q3map

typedef struct infoParm_s {
	const char	*name;
	uint32_t	clearSolid, surfaceFlags, contents;
} infoParm_t;

infoParm_t	infoParms[] = {
	// Game content Flags
	{ "nonsolid",		~CONTENTS_SOLID,					SURF_NONE,			CONTENTS_NONE },		// special hack to clear solid flag
	{ "nonopaque",		~CONTENTS_OPAQUE,					SURF_NONE,			CONTENTS_NONE },		// special hack to clear opaque flag
	{ "lava",			~CONTENTS_SOLID,					SURF_NONE,			CONTENTS_LAVA },		// very damaging
	{ "slime",			~CONTENTS_SOLID,					SURF_NONE,			CONTENTS_SLIME },		// mildly damaging
	{ "water",			~CONTENTS_SOLID,					SURF_NONE,			CONTENTS_WATER },		//
	{ "fog",			~CONTENTS_SOLID,					SURF_NONE,			CONTENTS_FOG},			// carves surfaces entering
	{ "shotclip",		~CONTENTS_SOLID,					SURF_NONE,			CONTENTS_SHOTCLIP },	// block shots, but not people
	{ "playerclip",		~(CONTENTS_SOLID|CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_PLAYERCLIP },	// block only the player
	{ "monsterclip",	~(CONTENTS_SOLID|CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_MONSTERCLIP },	//
	{ "botclip",		~(CONTENTS_SOLID|CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_BOTCLIP },		// for bots
	{ "trigger",		~(CONTENTS_SOLID|CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_TRIGGER },		//
	{ "nodrop",			~(CONTENTS_SOLID|CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_NODROP },		// don't drop items or leave bodies (death fog, lava, etc)
	{ "terrain",		~(CONTENTS_SOLID|CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_TERRAIN },		// use special terrain collsion
	{ "ladder",			~(CONTENTS_SOLID|CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_LADDER },		// climb up in it like water
	{ "abseil",			~(CONTENTS_SOLID|CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_ABSEIL },		// can abseil down this brush
	{ "outside",		~(CONTENTS_SOLID|CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_OUTSIDE },		// volume is considered to be in the outside (i.e. not indoors)
	{ "inside",			~(CONTENTS_SOLID|CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_INSIDE },		// volume is considered to be inside (i.e. indoors)

	{ "detail",			CONTENTS_ALL,						SURF_NONE,			CONTENTS_DETAIL },		// don't include in structural bsp
	{ "trans",			CONTENTS_ALL,						SURF_NONE,			CONTENTS_TRANSLUCENT },	// surface has an alpha component

	/* Game surface flags */
	{ "sky",			CONTENTS_ALL,						SURF_SKY,			CONTENTS_NONE },		// emit light from an environment map
	{ "slick",			CONTENTS_ALL,						SURF_SLICK,			CONTENTS_NONE },		//

	{ "nodamage",		CONTENTS_ALL,						SURF_NODAMAGE,		CONTENTS_NONE },		//
	{ "noimpact",		CONTENTS_ALL,						SURF_NOIMPACT,		CONTENTS_NONE },		// don't make impact explosions or marks
	{ "nomarks",		CONTENTS_ALL,						SURF_NOMARKS,		CONTENTS_NONE },		// don't make impact marks, but still explode
	{ "nodraw",			CONTENTS_ALL,						SURF_NODRAW,		CONTENTS_NONE },		// don't generate a drawsurface (or a lightmap)
	{ "nosteps",		CONTENTS_ALL,						SURF_NOSTEPS,		CONTENTS_NONE },		//
	{ "nodlight",		CONTENTS_ALL,						SURF_NODLIGHT,		CONTENTS_NONE },		// don't ever add dynamic lights
	{ "metalsteps",		CONTENTS_ALL,						SURF_METALSTEPS,	CONTENTS_NONE },		//
	{ "nomiscents",		CONTENTS_ALL,						SURF_NOMISCENTS,	CONTENTS_NONE },		// No misc ents on this surface
	{ "forcefield",		CONTENTS_ALL,						SURF_FORCEFIELD,	CONTENTS_NONE },		//
	{ "forcesight",		CONTENTS_ALL,						SURF_FORCESIGHT,	CONTENTS_NONE },		// only visible with force sight
};


/*
===============
ParseSurfaceParm

surfaceparm <name>
===============
*/
static void ParseSurfaceParm( const char **text ) {
	char	*token;
	int		numInfoParms = ARRAY_LEN( infoParms );
	int		i;

	token = COM_ParseExt( text, qfalse );
	for ( i = 0 ; i < numInfoParms ; i++ ) {
		if ( !Q_stricmp( token, infoParms[i].name ) ) {
			shader.surfaceFlags |= infoParms[i].surfaceFlags;
			shader.contentFlags |= infoParms[i].contents;
			shader.contentFlags &= infoParms[i].clearSolid;
			break;
		}
	}
}

/*
=================
ParseShader

The current text pointer is at the explicit text definition of the
shader.  Parse it into the global shader variable.  Later functions
will optimize it.
=================
*/
static qboolean ParseShader( const char **text )
{
	char *token;
	const char *begin = *text;
	int s;

	s = 0;

	token = COM_ParseExt( text, qtrue );
	if ( token[0] != '{' )
	{
		ri.Printf( PRINT_WARNING, "WARNING: expecting '{', found '%s' instead in shader '%s'\n", token, shader.name );
		return qfalse;
	}

	while ( 1 )
	{
		token = COM_ParseExt( text, qtrue );
		if ( !token[0] )
		{
			ri.Printf( PRINT_WARNING, "WARNING: no concluding '}' in shader %s\n", shader.name );
			return qfalse;
		}

		// end of shader definition
		if ( token[0] == '}' )
		{
			break;
		}
		// stage definition
		else if ( token[0] == '{' )
		{
			if ( s >= MAX_SHADER_STAGES ) {
				ri.Printf( PRINT_WARNING, "WARNING: too many stages in shader %s\n", shader.name );
				return qfalse;
			}

			if ( !ParseStage( &stages[s], text ) )
			{
				return qfalse;
			}
			stages[s].active = qtrue;
			s++;

			continue;
		}
		// skip stuff that only the QuakeEdRadient needs
		else if ( !Q_stricmpn( token, "qer", 3 ) ) {
			SkipRestOfLine( text );
			continue;
		}
		// material deprecated as of 11 Jan 01
		// material undeprecated as of 7 May 01 - q3map_material deprecated
		else if ( !Q_stricmp( token, "material" ) || !Q_stricmp( token, "q3map_material" ) )
		{
			ParseMaterial( text );
		}
		// sun parms
		else if ( !Q_stricmp( token, "sun" ) || !Q_stricmp( token, "q3map_sun" ) || !Q_stricmp( token, "q3map_sunExt" ) || !Q_stricmp( token, "q3gl2_sun" ) ) {
			float	a, b;
			qboolean isGL2Sun = qfalse;

			if (!Q_stricmp( token, "q3gl2_sun" ) && r_sunShadows->integer )
			{
				isGL2Sun = qtrue;
				tr.sunShadows = qtrue;
			}

			token = COM_ParseExt( text, qfalse );
			tr.sunLight[0] = atof( token );
			token = COM_ParseExt( text, qfalse );
			tr.sunLight[1] = atof( token );
			token = COM_ParseExt( text, qfalse );
			tr.sunLight[2] = atof( token );

			VectorNormalize( tr.sunLight );

			token = COM_ParseExt( text, qfalse );
			a = atof( token );
			VectorScale( tr.sunLight, a, tr.sunLight);

			token = COM_ParseExt( text, qfalse );
			a = atof( token );
			a = a / 180 * M_PI;

			token = COM_ParseExt( text, qfalse );
			b = atof( token );
			b = b / 180 * M_PI;

			tr.sunDirection[0] = cos( a ) * cos( b );
			tr.sunDirection[1] = sin( a ) * cos( b );
			tr.sunDirection[2] = sin( b );

			if (isGL2Sun)
			{
				token = COM_ParseExt( text, qfalse );
				tr.mapLightScale = atof(token);

				token = COM_ParseExt( text, qfalse );
				tr.sunShadowScale = atof(token);

				if (tr.sunShadowScale < 0.0f)
				{
					ri.Printf (PRINT_WARNING, "WARNING: q3gl2_sun's 'shadow scale' value must be between 0 and 1. Clamping to 0.0.\n");
					tr.sunShadowScale = 0.0f;
				}
				else if (tr.sunShadowScale > 1.0f)
				{
					ri.Printf (PRINT_WARNING, "WARNING: q3gl2_sun's 'shadow scale' value must be between 0 and 1. Clamping to 1.0.\n");
					tr.sunShadowScale = 1.0f;
				}
			}

			SkipRestOfLine( text );
			continue;
		}
		// tonemap parms
		else if ( !Q_stricmp( token, "q3gl2_tonemap" ) ) {
			token = COM_ParseExt( text, qfalse );
			tr.toneMinAvgMaxLevel[0] = atof( token );
			token = COM_ParseExt( text, qfalse );
			tr.toneMinAvgMaxLevel[1] = atof( token );
			token = COM_ParseExt( text, qfalse );
			tr.toneMinAvgMaxLevel[2] = atof( token );

			token = COM_ParseExt( text, qfalse );
			tr.autoExposureMinMax[0] = atof( token );
			token = COM_ParseExt( text, qfalse );
			tr.autoExposureMinMax[1] = atof( token );

			tr.explicitToneMap = true;

			SkipRestOfLine( text );
			continue;
		}
		// q3map_surfacelight deprecated as of 16 Jul 01
		else if ( !Q_stricmp( token, "surfacelight" ) || !Q_stricmp( token, "q3map_surfacelight" ) )
		{
			SkipRestOfLine( text );
			continue;
		}
		else if ( !Q_stricmp( token, "lightColor" ) )
		{
			SkipRestOfLine( text );
			continue;
		}
		else if ( !Q_stricmp( token, "deformvertexes" ) || !Q_stricmp( token, "deform" ) ) {
			ParseDeform( text );
			continue;
		}
		else if ( !Q_stricmp( token, "tesssize" ) ) {
			SkipRestOfLine( text );
			continue;
		}
		else if ( !Q_stricmp( token, "clampTime" ) ) {
			token = COM_ParseExt( text, qfalse );
			if (token[0]) {
				shader.clampTime = atof(token);
			}
		}
		// skip stuff that only the q3map needs
		else if ( !Q_stricmpn( token, "q3map", 5 ) ) {
			SkipRestOfLine( text );
			continue;
		}
		// skip stuff that only q3map or the server needs
		else if ( !Q_stricmp( token, "surfaceParm" ) ) {
			ParseSurfaceParm( text );
			continue;
		}
		// no mip maps
		else if ( !Q_stricmp( token, "nomipmaps" ) )
		{
			shader.noMipMaps = qtrue;
			shader.noPicMip = qtrue;
			continue;
		}
		// refractive surface
		else if ( !Q_stricmp(token, "refractive" ) )
		{
			shader.useDistortion = qtrue;
			continue;
		}
		// no picmip adjustment
		else if ( !Q_stricmp( token, "nopicmip" ) )
		{
			shader.noPicMip = qtrue;
			continue;
		}
		else if ( !Q_stricmp( token, "noglfog" ) )
		{
			//shader.fogPass = FP_NONE;
			continue;
		}
		// polygonOffset
		else if ( !Q_stricmp( token, "polygonOffset" ) )
		{
			shader.polygonOffset = qtrue;
			continue;
		}
		else if ( !Q_stricmp( token, "noTC" ) )
		{
			shader.noTC = qtrue;
			continue;
		}
		// entityMergable, allowing sprite surfaces from multiple entities
		// to be merged into one batch.  This is a savings for smoke
		// puffs and blood, but can't be used for anything where the
		// shader calcs (not the surface function) reference the entity color or scroll
		else if ( !Q_stricmp( token, "entityMergable" ) )
		{
			shader.entityMergable = qtrue;
			continue;
		}
		// fogParms
		else if ( !Q_stricmp( token, "fogParms" ) )
		{
			if ( !ParseVector( text, 3, shader.fogParms.color ) ) {
				return qfalse;
			}

			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parm for 'fogParms' keyword in shader '%s'\n", shader.name );
				continue;
			}
			shader.fogParms.depthForOpaque = atof( token );

			// skip any old gradient directions
			SkipRestOfLine( text );
			continue;
		}
		// portal
		else if ( !Q_stricmp(token, "portal") )
		{
			shader.sort = SS_PORTAL;
			shader.isPortal = qtrue;
			continue;
		}
		// skyparms <cloudheight> <outerbox> <innerbox>
		else if ( !Q_stricmp( token, "skyparms" ) )
		{
			ParseSkyParms( text );
			continue;
		}
		// light <value> determines flaring in q3map, not needed here
		else if ( !Q_stricmp(token, "light") )
		{
			COM_ParseExt( text, qfalse );
			continue;
		}
		// cull <face>
		else if ( !Q_stricmp( token, "cull") )
		{
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing cull parms in shader '%s'\n", shader.name );
				continue;
			}

			if ( !Q_stricmp( token, "none" ) || !Q_stricmp( token, "twosided" ) || !Q_stricmp( token, "disable" ) )
			{
				shader.cullType = CT_TWO_SIDED;
			}
			else if ( !Q_stricmp( token, "back" ) || !Q_stricmp( token, "backside" ) || !Q_stricmp( token, "backsided" ) )
			{
				shader.cullType = CT_BACK_SIDED;
			}
			else
			{
				ri.Printf( PRINT_WARNING, "WARNING: invalid cull parm '%s' in shader '%s'\n", token, shader.name );
			}
			continue;
		}
		// sort
		else if ( !Q_stricmp( token, "sort" ) )
		{
			ParseSort( text );
			continue;
		}
		else
		{
			ri.Printf( PRINT_WARNING, "WARNING: unknown general shader parameter '%s' in '%s'\n", token, shader.name );
			return qfalse;
		}
	}

	//
	// ignore shaders that don't have any stages, unless it is a sky or fog
	//
	if ( s == 0 && !shader.isSky && !(shader.contentFlags & CONTENTS_FOG ) ) {
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
	uint32_t shaderHash = generateHashValueForText( begin, *text - begin );
	if ( shaderHash == RETAIL_ROCKET_WEDGE_SHADER_HASH &&
		Q_stricmp( shader.name, "gfx/2d/wedge" ) == 0 )
	{
		stages[0].stateBits &= ~(GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS);
		stages[0].stateBits |= GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	}

	return qtrue;
}

/*
========================================================================================

SHADER OPTIMIZATION AND FOGGING

========================================================================================
*/

/*
===================
ComputeStageIteratorFunc

See if we can use on of the simple fastpath stage functions,
otherwise set to the generic stage function
===================
*/
static void ComputeStageIteratorFunc( void )
{
	shader.optimalStageIteratorFunc = RB_StageIteratorGeneric;

	//
	// see if this should go into the sky path
	//
	if ( shader.isSky )
	{
		shader.optimalStageIteratorFunc = RB_StageIteratorSky;
		return;
	}
}

/*
===================
ComputeVertexAttribs

Check which vertex attributes we only need, so we
don't need to submit/copy all of them.
===================
*/
static void ComputeVertexAttribs(void)
{
	int i, stage;

	// dlights always need ATTR_NORMAL
	shader.vertexAttribs = ATTR_POSITION | ATTR_NORMAL | ATTR_COLOR;

	if (shader.defaultShader)
	{
		shader.vertexAttribs |= ATTR_TEXCOORD0;
		return;
	}

	if(shader.numDeforms)
	{
		for ( i = 0; i < shader.numDeforms; i++)
		{
			deformStage_t  *ds = &shader.deforms[i];

			switch (ds->deformation)
			{
				case DEFORM_BULGE:
					shader.vertexAttribs |= ATTR_NORMAL | ATTR_TEXCOORD0;
					break;

				case DEFORM_AUTOSPRITE:
					shader.vertexAttribs |= ATTR_NORMAL | ATTR_COLOR;
					break;

				case DEFORM_WAVE:
				case DEFORM_NORMALS:
				case DEFORM_TEXT0:
				case DEFORM_TEXT1:
				case DEFORM_TEXT2:
				case DEFORM_TEXT3:
				case DEFORM_TEXT4:
				case DEFORM_TEXT5:
				case DEFORM_TEXT6:
				case DEFORM_TEXT7:
					shader.vertexAttribs |= ATTR_NORMAL;
					break;

				default:
				case DEFORM_NONE:
				case DEFORM_MOVE:
				case DEFORM_PROJECTION_SHADOW:
				case DEFORM_AUTOSPRITE2:
					break;
			}
		}
	}

	for ( stage = 0; stage < MAX_SHADER_STAGES; stage++ )
	{
		shaderStage_t *pStage = &stages[stage];

		if ( !pStage->active )
		{
			break;
		}

		if (pStage->glslShaderGroup == tr.lightallShader)
		{
			shader.vertexAttribs |= ATTR_NORMAL;

			if ((pStage->glslShaderIndex & LIGHTDEF_LIGHTTYPE_MASK) &&
					(r_normalMapping->integer != 0 ||
					 r_specularMapping->integer != 0))
			{
				shader.vertexAttribs |= ATTR_TANGENT;
			}

			switch (pStage->glslShaderIndex & LIGHTDEF_LIGHTTYPE_MASK)
			{
				case LIGHTDEF_USE_LIGHTMAP:
				case LIGHTDEF_USE_LIGHT_VERTEX:
					shader.vertexAttribs |= ATTR_LIGHTDIRECTION;
					break;
				default:
					break;
			}
		}

		for (i = 0; i < NUM_TEXTURE_BUNDLES; i++)
		{
			if ( pStage->bundle[i].image[0] == 0 )
			{
				continue;
			}

			switch(pStage->bundle[i].tcGen)
			{
				case TCGEN_TEXTURE:
					shader.vertexAttribs |= ATTR_TEXCOORD0;
					break;
				case TCGEN_LIGHTMAP:
				case TCGEN_LIGHTMAP1:
				case TCGEN_LIGHTMAP2:
				case TCGEN_LIGHTMAP3:
					shader.vertexAttribs |= (ATTR_TEXCOORD1 | ATTR_TEXCOORD2 |
											 ATTR_TEXCOORD3 | ATTR_TEXCOORD4);
					break;
				case TCGEN_ENVIRONMENT_MAPPED:
					shader.vertexAttribs |= ATTR_NORMAL;
					break;

				default:
					break;
			}
		}

		switch(pStage->rgbGen)
		{
			case CGEN_EXACT_VERTEX:
			case CGEN_VERTEX:
			case CGEN_EXACT_VERTEX_LIT:
			case CGEN_VERTEX_LIT:
			case CGEN_ONE_MINUS_VERTEX:
				shader.vertexAttribs |= ATTR_COLOR;
				break;

			case CGEN_LIGHTING_DIFFUSE:
			case CGEN_LIGHTING_DIFFUSE_ENTITY:
				shader.vertexAttribs |= ATTR_NORMAL;
				break;

			default:
				break;
		}

		switch(pStage->alphaGen)
		{
			case AGEN_LIGHTING_SPECULAR:
				shader.vertexAttribs |= ATTR_NORMAL;
				break;

			case AGEN_VERTEX:
			case AGEN_ONE_MINUS_VERTEX:
				shader.vertexAttribs |= ATTR_COLOR;
				break;

			default:
				break;
		}
	}
}

static void CollapseStagesToLightall(shaderStage_t *stage, shaderStage_t *lightmap,
	qboolean useLightVector, qboolean useLightVertex, qboolean tcgen)
{
	int defs = 0;

	//ri.Printf(PRINT_ALL, "shader %s has diffuse %s", shader.name, stage->bundle[0].image[0]->imgName);

	// reuse diffuse, mark others inactive
	stage->type = ST_GLSL;

	if (lightmap)
	{
		//ri.Printf(PRINT_ALL, ", lightmap");
		stage->bundle[TB_LIGHTMAP] = lightmap->bundle[0];
		defs |= LIGHTDEF_USE_LIGHTMAP;
	}
	else if (useLightVector)
	{
		defs |= LIGHTDEF_USE_LIGHT_VECTOR;
	}
	else if (useLightVertex)
	{
		defs |= LIGHTDEF_USE_LIGHT_VERTEX;
	}

	if (r_deluxeMapping->integer && tr.worldDeluxeMapping && lightmap)
	{
		//ri.Printf(PRINT_ALL, ", deluxemap");
		stage->bundle[TB_DELUXEMAP] = lightmap->bundle[TB_DELUXEMAP];
	}

	if (r_normalMapping->integer)
	{
		image_t *diffuseImg;
		if (stage->bundle[TB_NORMALMAP].image[0])
		{
			if (stage->bundle[TB_NORMALMAP].image[0]->type == IMGTYPE_NORMALHEIGHT &&
				r_parallaxMapping->integer &&
				defs & LIGHTDEF_LIGHTTYPE_MASK)
				defs |= LIGHTDEF_USE_PARALLAXMAP;
			//ri.Printf(PRINT_ALL, ", normalmap %s", stage->bundle[TB_NORMALMAP].image[0]->imgName);
		}
		else if ((lightmap || useLightVector || useLightVertex) && (diffuseImg = stage->bundle[TB_DIFFUSEMAP].image[0]) != NULL)
		{
			char normalName[MAX_QPATH];
			image_t *normalImg;
			int normalFlags = (diffuseImg->flags & ~(IMGFLAG_GENNORMALMAP | IMGFLAG_SRGB)) | IMGFLAG_NOLIGHTSCALE;
			qboolean parallax = qfalse;
			// try a normalheight image first
			COM_StripExtension(diffuseImg->imgName, normalName, sizeof(normalName));
			Q_strcat(normalName, sizeof(normalName), "_nh");

			normalImg = R_FindImageFile(normalName, IMGTYPE_NORMALHEIGHT, normalFlags);

			if (normalImg)
			{
				parallax = qtrue;
			}
			else
			{
				// try a normal image ("_n" suffix)
				normalName[strlen(normalName) - 1] = '\0';
				normalImg = R_FindImageFile(normalName, IMGTYPE_NORMAL, normalFlags);
			}

			if (normalImg)
			{
				stage->bundle[TB_NORMALMAP] = stage->bundle[0];
				stage->bundle[TB_NORMALMAP].numImageAnimations = 0;
				stage->bundle[TB_NORMALMAP].image[0] = normalImg;

				if (parallax && r_parallaxMapping->integer)
					defs |= LIGHTDEF_USE_PARALLAXMAP;

				VectorSet4(stage->normalScale, r_baseNormalX->value, r_baseNormalY->value, 1.0f, r_baseParallax->value);
			}
		}
	}

	if (r_specularMapping->integer)
	{
		image_t *diffuseImg;
		if (stage->bundle[TB_SPECULARMAP].image[0])
		{
			if (stage->specularType == SPEC_SPECGLOSS)
				defs |= LIGHTDEF_USE_SPEC_GLOSS;
		}
		else if ((lightmap || useLightVector || useLightVertex) && (diffuseImg = stage->bundle[TB_DIFFUSEMAP].image[0]) != NULL)
		{
			char imageName[MAX_QPATH];
			image_t *specularImg;
			int specularFlags = diffuseImg->flags & ~(IMGFLAG_GENNORMALMAP);

			COM_StripExtension(diffuseImg->imgName, imageName, MAX_QPATH);
			Q_strcat(imageName, MAX_QPATH, "_specGloss");

			if (diffuseImg->flags & IMGFLAG_SRGB)
				specularImg = R_FindImageFile(imageName, IMGTYPE_COLORALPHA, specularFlags);
			else
				specularImg = R_BuildSDRSpecGlossImage(stage, imageName, specularFlags);

			if (specularImg)
			{
				stage->bundle[TB_SPECULARMAP] = stage->bundle[0];
				stage->bundle[TB_SPECULARMAP].numImageAnimations = 0;
				stage->bundle[TB_SPECULARMAP].image[0] = specularImg;
				stage->specularType = SPEC_SPECGLOSS;
				defs |= LIGHTDEF_USE_SPEC_GLOSS;
				VectorSet4(stage->specularScale, 1.0f, 1.0f, 1.0f, 0.0f);
			}
			else
			{
				// Data images shouldn't be lightscaled
				specularFlags |= IMGFLAG_NOLIGHTSCALE;
				COM_StripExtension(diffuseImg->imgName, imageName, MAX_QPATH);
				Q_strcat(imageName, MAX_QPATH, "_rmo");
				stage->specularType = SPEC_RMO;
				R_LoadPackedMaterialImage(stage, imageName, specularFlags);

				if (!stage->bundle[TB_ORMSMAP].image[0])
				{
					COM_StripExtension(diffuseImg->imgName, imageName, MAX_QPATH);
					Q_strcat(imageName, MAX_QPATH, "_orm");
					stage->specularType = SPEC_ORM;
					R_LoadPackedMaterialImage(stage, imageName, specularFlags);
					if (!stage->bundle[TB_ORMSMAP].image[0])
					{
						stage->specularScale[0] = 0.0f;
						stage->specularScale[2] =
						stage->specularScale[3] = 1.0f;
						stage->specularScale[1] = 0.5f;
					}
				}
			}
		}
	}

	if (tcgen || stage->bundle[0].numTexMods)
	{
		defs |= LIGHTDEF_USE_TCGEN_AND_TCMOD;
	}

	if (stage->glow)
		defs |= LIGHTDEF_USE_GLOW_BUFFER;

	if (stage->cloth)
		defs |= LIGHTDEF_USE_CLOTH_BRDF;

	if (stage->alphaTestType != ALPHA_TEST_NONE)
		defs |= LIGHTDEF_USE_ALPHA_TEST;

	//ri.Printf(PRINT_ALL, ".\n");

	stage->glslShaderGroup = tr.lightallShader;
	stage->glslShaderIndex = defs;
}


static qboolean CollapseStagesToGLSL(void)
{
	int i, j, numStages;
	qboolean skip = qfalse;

	ri.Printf (PRINT_DEVELOPER, "Collapsing stages for shader '%s'\n", shader.name);

	// skip shaders with deforms
	if (shader.numDeforms == 1)
	{
		skip = qtrue;
		ri.Printf (PRINT_DEVELOPER, "> Shader has vertex deformations. Aborting stage collapsing\n");
	}

	ri.Printf (PRINT_DEVELOPER, "> Original shader stage order:\n");

	for ( int i = 0; i < MAX_SHADER_STAGES; i++ )
	{
		shaderStage_t *stage = &stages[i];

		if ( !stage->active )
		{
			continue;
		}

		ri.Printf (PRINT_DEVELOPER, "-> %s\n", stage->bundle[0].image[0]->imgName);
	}

	if (!skip)
	{
		// scan for shaders that aren't supported
		for (i = 0; i < MAX_SHADER_STAGES; i++)
		{
			shaderStage_t *pStage = &stages[i];

			if (!pStage->active)
				continue;

			if (pStage->adjustColorsForFog)
			{
				skip = qtrue;
				break;
			}

			if (pStage->bundle[0].tcGen >= TCGEN_LIGHTMAP &&
				pStage->bundle[0].tcGen <= TCGEN_LIGHTMAP3)
			{
				int blendBits = pStage->stateBits & ( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS );

				if (blendBits != (GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO)
					&& blendBits != (GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR)
					&& blendBits != (GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE)) //lightstyle lightmap stages
				{
					skip = qtrue;
					break;
				}
			}

			switch(pStage->bundle[0].tcGen)
			{
				case TCGEN_TEXTURE:
				case TCGEN_LIGHTMAP:
				case TCGEN_LIGHTMAP1:
				case TCGEN_LIGHTMAP2:
				case TCGEN_LIGHTMAP3:
				case TCGEN_ENVIRONMENT_MAPPED:
				case TCGEN_VECTOR:
					break;
				default:
					skip = qtrue;
					break;
			}

			switch(pStage->alphaGen)
			{
				case AGEN_PORTAL:
				case AGEN_LIGHTING_SPECULAR:
					skip = qtrue;
					break;
				default:
					break;
			}
		}
	}

	if (!skip)
	{
		shaderStage_t *lightmaps[MAX_SHADER_STAGES] = {};

		for (i = 0; i < MAX_SHADER_STAGES; i++)
		{
			shaderStage_t *pStage = &stages[i];

			if (!pStage->active)
				continue;

			if (pStage->bundle[0].tcGen >= TCGEN_LIGHTMAP && pStage->bundle[0].tcGen <= TCGEN_LIGHTMAP3)
			{
				lightmaps[i] = pStage;
			}
		}

		for (i = 0; i < MAX_SHADER_STAGES; i++)
		{
			shaderStage_t *pStage = &stages[i];
			shaderStage_t *diffuse, *lightmap;
			qboolean parallax, tcgen, diffuselit, vertexlit;

			if (!pStage->active)
				continue;

			// skip normal and specular maps
			if (pStage->type != ST_COLORMAP)
				continue;

			// skip lightmaps
			if (pStage->bundle[0].tcGen >= TCGEN_LIGHTMAP && pStage->bundle[0].tcGen <= TCGEN_LIGHTMAP3)
				continue;

			diffuse  = pStage;
			parallax = qfalse;
			lightmap = NULL;

			// we have a diffuse map, find matching lightmap
			for (j = i + 1; j < MAX_SHADER_STAGES; j++)
			{
				shaderStage_t *pStage2 = &stages[j];

				if (!pStage2->active)
					continue;

				if (pStage2->glow)
					continue;

				if (pStage2->bundle[0].tcGen >= TCGEN_LIGHTMAP &&
					pStage2->bundle[0].tcGen <= TCGEN_LIGHTMAP3 &&
					pStage2->rgbGen != CGEN_EXACT_VERTEX)
				{
					ri.Printf (PRINT_DEVELOPER, "> Setting lightmap for %s to %s\n", pStage->bundle[0].image[0]->imgName, pStage2->bundle[0].image[0]->imgName);
					lightmap = pStage2;
					lightmaps[j] = NULL;
					break;
				}
			}

			tcgen = qfalse;
			if (diffuse->bundle[0].tcGen == TCGEN_ENVIRONMENT_MAPPED
			    || (diffuse->bundle[0].tcGen >= TCGEN_LIGHTMAP && diffuse->bundle[0].tcGen <= TCGEN_LIGHTMAP3)
			    || diffuse->bundle[0].tcGen == TCGEN_VECTOR)
			{
				tcgen = qtrue;
			}

			diffuselit = qfalse;
			if (diffuse->rgbGen == CGEN_LIGHTING_DIFFUSE ||
				diffuse->rgbGen == CGEN_LIGHTING_DIFFUSE_ENTITY)
			{
				diffuselit = qtrue;
			}

			vertexlit = qfalse;
			if (diffuse->rgbGen == CGEN_VERTEX_LIT || diffuse->rgbGen == CGEN_EXACT_VERTEX_LIT || diffuse->rgbGen == CGEN_VERTEX || diffuse->rgbGen == CGEN_EXACT_VERTEX)
			{
				vertexlit = qtrue;
			}

			CollapseStagesToLightall(diffuse, lightmap, diffuselit, vertexlit, tcgen);

			//find lightstyle stages
			if (lightmap != NULL)
			{
				for (j = i + 2; j < MAX_SHADER_STAGES; j++)
				{
					shaderStage_t *pStage2 = &stages[j];

					int blendBits = pStage2->stateBits & (GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS);

					if (blendBits != (GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE)
						|| (pStage2->bundle[0].tcGen < TCGEN_LIGHTMAP && pStage2->bundle[0].tcGen > TCGEN_LIGHTMAP3)
						|| pStage2->rgbGen != CGEN_LIGHTMAPSTYLE)
						break;

					//style stage is the previous lightmap stage which is not used anymore
					shaderStage_t *styleStage = &stages[j - 1];

					int style = pStage2->lightmapStyle;
					int stateBits = pStage2->stateBits;
					textureBundle_t tbLightmap = pStage2->bundle[0];
					textureBundle_t tbDeluxemap = pStage2->bundle[TB_DELUXEMAP];

					memcpy(styleStage, diffuse, sizeof(shaderStage_t));
					styleStage->lightmapStyle = style;
					styleStage->stateBits = stateBits;
					styleStage->rgbGen = CGEN_LIGHTMAPSTYLE;

					styleStage->bundle[TB_LIGHTMAP] = tbLightmap;
					styleStage->bundle[TB_DELUXEMAP] = tbDeluxemap;

					lightmaps[j] = NULL;
				}
			}
		}

		// deactivate lightmap stages
		for (i = 0; i < MAX_SHADER_STAGES; i++)
		{
			shaderStage_t *pStage = &stages[i];

			if (!pStage->active)
				continue;

			if (pStage->bundle[0].tcGen >= TCGEN_LIGHTMAP &&
				pStage->bundle[0].tcGen <= TCGEN_LIGHTMAP3 &&
				lightmaps[i] == NULL)
			{
				pStage->active = qfalse;
			}
		}
	}

	if (skip)
	{
		// set default specular scale for skipped shaders that will use metalness workflow by default
		for (i = 0; i < MAX_SHADER_STAGES; i++)
		{
			shaderStage_t *pStage = &stages[i];
			pStage->specularScale[0] = 0.0f;
			pStage->specularScale[2] =
			pStage->specularScale[3] = 1.0f;
			pStage->specularScale[1] = 0.5f;
		}
	}

	// remove inactive stages
	numStages = 0;
	for (i = 0; i < MAX_SHADER_STAGES; i++)
	{
		if (!stages[i].active)
			continue;

		if (i == numStages)
		{
			numStages++;
			continue;
		}

		stages[numStages] = stages[i];
		stages[i].active = qfalse;
		numStages++;
	}

	// convert any remaining lightmap stages to a lighting pass with a white texture
	// only do this with r_sunlightMode non-zero, as it's only for correct shadows.
	if (r_sunlightMode->integer && shader.numDeforms != 1)
	{
		for (i = 0; i < MAX_SHADER_STAGES; i++)
		{
			shaderStage_t *pStage = &stages[i];

			if (!pStage->active)
				continue;

			if (pStage->adjustColorsForFog)
				continue;

			if (pStage->bundle[TB_DIFFUSEMAP].tcGen >= TCGEN_LIGHTMAP
				&& pStage->bundle[TB_DIFFUSEMAP].tcGen <= TCGEN_LIGHTMAP3
				&& pStage->rgbGen != CGEN_LIGHTMAPSTYLE) //don't convert lightstyled lightmap stages
			{
				pStage->glslShaderGroup = tr.lightallShader;
				pStage->glslShaderIndex = LIGHTDEF_USE_LIGHTMAP;
				pStage->bundle[TB_LIGHTMAP] = pStage->bundle[TB_DIFFUSEMAP];
				pStage->bundle[TB_DIFFUSEMAP].image[0] = tr.whiteImage;
				pStage->bundle[TB_DIFFUSEMAP].isLightmap = qfalse;
				pStage->bundle[TB_DIFFUSEMAP].tcGen = TCGEN_TEXTURE;
			}
		}
	}

	// convert any remaining lightingdiffuse stages to a lighting pass
	if (shader.numDeforms != 1)
	{
		for (i = 0; i < MAX_SHADER_STAGES; i++)
		{
			shaderStage_t *pStage = &stages[i];

			if (!pStage->active)
				continue;

			if (pStage->adjustColorsForFog)
				continue;

			if (pStage->rgbGen == CGEN_LIGHTING_DIFFUSE ||
				pStage->rgbGen == CGEN_LIGHTING_DIFFUSE_ENTITY)
			{
				if (pStage->glslShaderGroup != tr.lightallShader)
				{
					pStage->glslShaderGroup = tr.lightallShader;
					pStage->glslShaderIndex = LIGHTDEF_USE_LIGHT_VECTOR;
				}
				else if (!(pStage->glslShaderIndex & LIGHTDEF_USE_LIGHT_VECTOR))
				{
					pStage->glslShaderIndex &= ~LIGHTDEF_LIGHTTYPE_MASK;
					pStage->glslShaderIndex |= LIGHTDEF_USE_LIGHT_VECTOR;
				}

				if (pStage->bundle[0].tcGen != TCGEN_TEXTURE || pStage->bundle[0].numTexMods != 0)
					pStage->glslShaderIndex |= LIGHTDEF_USE_TCGEN_AND_TCMOD;
			}
		}
	}

	ri.Printf (PRINT_DEVELOPER, "> New shader stage order:\n");

	for ( int i = 0; i < MAX_SHADER_STAGES; i++ )
	{
		shaderStage_t *stage = &stages[i];

		if ( !stage->active )
		{
			continue;
		}

		ri.Printf (PRINT_DEVELOPER, "-> %s\n", stage->bundle[0].image[0]->imgName);
	}

	return (qboolean)numStages;
}

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
	if( !gServerSkinHack ) {
		renderCommandList_t	*cmdList = &backEndData->commands;

		if( cmdList ) {
			const void *curCmd = cmdList->cmds;

			while ( 1 ) {
				curCmd = PADP(curCmd, sizeof(void *));

				switch ( *(const int *)curCmd ) {
				case RC_SET_COLOR:
					{
					const setColorCommand_t *sc_cmd = (const setColorCommand_t *)curCmd;
					curCmd = (const void *)(sc_cmd + 1);
					break;
					}
				case RC_STRETCH_PIC:
					{
					const stretchPicCommand_t *sp_cmd = (const stretchPicCommand_t *)curCmd;
					curCmd = (const void *)(sp_cmd + 1);
					break;
					}
				case RC_ROTATE_PIC:
				case RC_ROTATE_PIC2:
					{
						const rotatePicCommand_t *sp_cmd = (const rotatePicCommand_t *)curCmd;
						curCmd = (const void *)(sp_cmd + 1);
						break;
					}
				case RC_DRAW_SURFS:
					{
					int i;
					drawSurf_t	*drawSurf;
					shader_t	*shader;
					int         postRender;
					int			sortedIndex;
					int			cubemap;
					int			entityNum;
					const drawSurfsCommand_t *ds_cmd =  (const drawSurfsCommand_t *)curCmd;

					for( i = 0, drawSurf = ds_cmd->drawSurfs; i < ds_cmd->numDrawSurfs; i++, drawSurf++ ) {
						R_DecomposeSort( drawSurf->sort, &entityNum, &shader, &cubemap, &postRender );
						sortedIndex = (( drawSurf->sort >> QSORT_SHADERNUM_SHIFT ) & (MAX_SHADERS-1));
						if( sortedIndex >= newShader ) {
							sortedIndex++;
							drawSurf->sort = R_CreateSortKey(entityNum, sortedIndex, cubemap, postRender);
						}
					}
					curCmd = (const void *)(ds_cmd + 1);
					break;
					}
				case RC_DRAW_BUFFER:
					{
					const drawBufferCommand_t *db_cmd = (const drawBufferCommand_t *)curCmd;
					curCmd = (const void *)(db_cmd + 1);
					break;
					}
				case RC_SWAP_BUFFERS:
					{
					const swapBuffersCommand_t *sb_cmd = (const swapBuffersCommand_t *)curCmd;
					curCmd = (const void *)(sb_cmd + 1);
					break;
					}
				case RC_END_OF_LIST:
				default:
					return;
				}
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
static void SortNewShader( void ) {
	int		i;
	float	sort;
	shader_t	*newShader;

	newShader = tr.shaders[ tr.numShaders - 1 ];
	sort = newShader->sort;

	int numStagesInNewShader = 0;
	while ( newShader->stages[numStagesInNewShader] )
		numStagesInNewShader++;

	for ( i = tr.numShaders - 2 ; i >= 0 ; i-- ) {
		shader_t *shader = tr.sortedShaders[i];
		int numStages = 0;
		while ( shader->stages[numStages] )
			numStages++;

		if ( shader->sort < sort ) {
			break;
		}

		if ( shader->sort == sort && numStages <= numStagesInNewShader )
		{
			break;
		}

		tr.sortedShaders[i+1] = shader;
		tr.sortedShaders[i+1]->sortedIndex++;
	}

	// Arnout: fix rendercommandlist
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=493
	FixRenderCommandList( i+1 );

	newShader->sortedIndex = i+1;
	tr.sortedShaders[i+1] = newShader;
}


/*
====================
GeneratePermanentShader
====================
*/
static shader_t *GeneratePermanentShader( void ) {
	shader_t	*newShader;
	int			i, b;
	int			size, hash;

	if ( tr.numShaders == MAX_SHADERS ) {
		ri.Printf( PRINT_WARNING, "WARNING: GeneratePermanentShader - MAX_SHADERS hit\n");
		return tr.defaultShader;
	}

	newShader = (shader_t *)ri.Hunk_Alloc( sizeof( shader_t ), h_low );

	*newShader = shader;

	if ( shader.sort <= SS_SEE_THROUGH ) {
		newShader->fogPass = FP_EQUAL;
	} else if ( shader.contentFlags & CONTENTS_FOG ) {
		newShader->fogPass = FP_LE;
	} else {
		newShader->fogPass = FP_NONE;
	}

	// determain if fog pass can use depth equal or not
	if (newShader->fogPass == FP_EQUAL)
	{
		newShader->fogPass = FP_LE;
		bool allPassesAlpha = true;
		for (int stage = 0; stage < MAX_SHADER_STAGES; stage++) {
			shaderStage_t *pStage = &stages[stage];

			if (!pStage->active)
				continue;

			if (pStage->glow && stage > 0)
				continue;

			if (pStage->adjustColorsForFog != ACFF_MODULATE_ALPHA)
				allPassesAlpha = false;

			if (pStage->stateBits & GLS_DEPTHMASK_TRUE)
			{
				newShader->fogPass = FP_EQUAL;
				break;
			}
		}

		if (allPassesAlpha)
			newShader->fogPass = FP_NONE;
	}

	tr.shaders[ tr.numShaders ] = newShader;
	newShader->index = tr.numShaders;

	tr.sortedShaders[ tr.numShaders ] = newShader;
	newShader->sortedIndex = tr.numShaders;

	tr.numShaders++;

	for ( i = 0 ; i < newShader->numUnfoggedPasses ; i++ ) {
		if ( !stages[i].active ) {
			break;
		}
		newShader->stages[i] = (shaderStage_t *)ri.Hunk_Alloc( sizeof( stages[i] ), h_low );
		*newShader->stages[i] = stages[i];

		for ( b = 0 ; b < NUM_TEXTURE_BUNDLES ; b++ ) {
			size = newShader->stages[i]->bundle[b].numTexMods * sizeof( texModInfo_t );
			newShader->stages[i]->bundle[b].texMods = (texModInfo_t *)ri.Hunk_Alloc( size, h_low );
			Com_Memcpy( newShader->stages[i]->bundle[b].texMods, stages[i].bundle[b].texMods, size );
		}
	}

	RB_AddShaderToShaderInstanceUBO(newShader);

	SortNewShader();

	hash = generateHashValue(newShader->name, FILE_HASH_SIZE);
	newShader->next = hashTable[hash];
	hashTable[hash] = newShader;

	return newShader;
}

/*
=================
VertexLightingCollapse

If vertex lighting is enabled, only render a single
pass, trying to guess which is the correct one to best aproximate
what it is supposed to look like.
=================
*/
static void VertexLightingCollapse( void ) {
	int		stage;
	shaderStage_t	*bestStage;
	int		bestImageRank;
	int		rank;

	// if we aren't opaque, just use the first pass
	if ( shader.sort == SS_OPAQUE ) {

		// pick the best texture for the single pass
		bestStage = &stages[0];
		bestImageRank = -999999;

		for ( stage = 0; stage < MAX_SHADER_STAGES; stage++ ) {
			shaderStage_t *pStage = &stages[stage];

			if ( !pStage->active ) {
				break;
			}
			rank = 0;

			if ( pStage->bundle[0].isLightmap ) {
				rank -= 100;
			}
			if ( pStage->bundle[0].tcGen != TCGEN_TEXTURE ) {
				rank -= 5;
			}
			if ( pStage->bundle[0].numTexMods ) {
				rank -= 5;
			}
			if ( pStage->rgbGen != CGEN_IDENTITY && pStage->rgbGen != CGEN_IDENTITY_LIGHTING ) {
				rank -= 3;
			}

			if ( rank > bestImageRank  ) {
				bestImageRank = rank;
				bestStage = pStage;
			}
		}

		stages[0].bundle[0] = bestStage->bundle[0];
		stages[0].stateBits &= ~( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS );
		stages[0].stateBits |= GLS_DEPTHMASK_TRUE;
		if ( shader.lightmapIndex[0] == LIGHTMAP_NONE ) {
			stages[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
		} else {
			stages[0].rgbGen = CGEN_EXACT_VERTEX;
		}
		stages[0].alphaGen = AGEN_SKIP;
	} else {
		// don't use a lightmap (tesla coils)
		if ( stages[0].bundle[0].isLightmap ) {
			stages[0] = stages[1];
		}

		// if we were in a cross-fade cgen, hack it to normal
		if ( stages[0].rgbGen == CGEN_ONE_MINUS_ENTITY || stages[1].rgbGen == CGEN_ONE_MINUS_ENTITY ) {
			stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
		if ( ( stages[0].rgbGen == CGEN_WAVEFORM && stages[0].rgbWave.func == GF_SAWTOOTH )
			&& ( stages[1].rgbGen == CGEN_WAVEFORM && stages[1].rgbWave.func == GF_INVERSE_SAWTOOTH ) ) {
			stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
		if ( ( stages[0].rgbGen == CGEN_WAVEFORM && stages[0].rgbWave.func == GF_INVERSE_SAWTOOTH )
			&& ( stages[1].rgbGen == CGEN_WAVEFORM && stages[1].rgbWave.func == GF_SAWTOOTH ) ) {
			stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
	}

	for ( stage = 1; stage < MAX_SHADER_STAGES; stage++ ) {
		shaderStage_t *pStage = &stages[stage];

		if ( !pStage->active ) {
			break;
		}

		Com_Memset( pStage, 0, sizeof( *pStage ) );
	}
}

int FindFirstLightmapStage ( const shaderStage_t *stages, int numStages )
{
	for ( int i = 0; i < numStages; i++ )
	{
		const shaderStage_t *stage = &stages[i];
		if ( stage->active && stage->bundle[0].isLightmap )
		{
			return i;
		}
	}

	return numStages;
}

int GetNumStylesInShader ( const shader_t *shader )
{
	for ( int i = 0; i < MAXLIGHTMAPS; i++ )
	{
		if ( shader->styles[i] >= LS_UNUSED )
		{
			return i - 1;
		}
	}

	return MAXLIGHTMAPS - 1;
}

/*
=========================
FinishShader

Returns a freshly allocated shader with all the needed info
from the current global working shader
=========================
*/
static shader_t *FinishShader( void ) {
	int stage;
	uint32_t shaderStateBits = 0;
	qboolean hasLightmapStage = qfalse;

	//
	// set sky stuff appropriate
	//
	if ( shader.isSky ) {
		shader.sort = SS_ENVIRONMENT;
	}

	//
	// set polygon offset
	//
	if ( shader.polygonOffset ) {
		shaderStateBits |= GLS_POLYGON_OFFSET_FILL;
		if ( !shader.sort ) {
			shader.sort = SS_DECAL;
		}
	}

	int lmStage;
	for(lmStage = 0; lmStage < MAX_SHADER_STAGES; lmStage++)
	{
		shaderStage_t *pStage = &stages[lmStage];
		if (pStage->active && pStage->bundle[0].isLightmap)
		{
			break;
		}
	}

	if (lmStage < MAX_SHADER_STAGES)
	{
		if (shader.lightmapIndex[0] == LIGHTMAP_BY_VERTEX)
		{
			if (lmStage == 0)	//< MAX_SHADER_STAGES-1)
			{//copy the rest down over the lightmap slot
				memmove(&stages[lmStage], &stages[lmStage+1], sizeof(shaderStage_t) * (MAX_SHADER_STAGES-lmStage-1));
				memset(&stages[MAX_SHADER_STAGES-1], 0, sizeof(shaderStage_t));
				//change blending on the moved down stage
				stages[lmStage].stateBits = GLS_DEFAULT;
			}
			//change anything that was moved down (or the *white if LM is first) to use vertex color
			stages[lmStage].rgbGen = CGEN_EXACT_VERTEX;
			stages[lmStage].alphaGen = AGEN_SKIP;
			lmStage = MAX_SHADER_STAGES;	//skip the style checking below
		}
	}

	// if 2+ stages and first stage is lightmap, switch them
	// this makes it easier for the later bits to process
	if (stages[0].active &&
		stages[0].bundle[0].isLightmap &&
		stages[1].active &&
		shader.numDeforms != 1)
	{
		int blendBits = stages[1].stateBits & (GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS);

		if (blendBits == (GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO)
			|| blendBits == (GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR))
		{
			int stateBits0 = stages[0].stateBits;
			int stateBits1 = stages[1].stateBits;
			shaderStage_t swapStage;

			swapStage = stages[0];
			stages[0] = stages[1];
			stages[1] = swapStage;

			stages[0].stateBits = stateBits0;
			stages[1].stateBits = stateBits1;

			lmStage = 1;
		}
	}

	if (lmStage < MAX_SHADER_STAGES)// && !r_fullbright->value)
	{
		int	numStyles;
		int	i;

		for(numStyles=0;numStyles<MAXLIGHTMAPS;numStyles++)
		{
			if (shader.styles[numStyles] >= LS_UNUSED)
			{
				break;
			}
		}
		numStyles--;
		if (numStyles > 0)
		{
			for(i=MAX_SHADER_STAGES-1;i>lmStage+numStyles;i--)
			{
				stages[i] = stages[i-numStyles];
			}

			for(i=0;i<numStyles;i++)
			{
				stages[lmStage+i+1] = stages[lmStage];
				if (shader.lightmapIndex[i+1] == LIGHTMAP_BY_VERTEX)
				{
					stages[lmStage+i+1].bundle[0].image[0] = tr.whiteImage;
				}
				else if (shader.lightmapIndex[i+1] < 0)
				{
					Com_Error( ERR_DROP, "FinishShader: light style with no light map or vertex color for shader %s", shader.name);
				}
				else
				{
					stages[lmStage+i+1].bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex[i+1]];
					stages[lmStage+i+1].bundle[0].tcGen = (texCoordGen_t)(TCGEN_LIGHTMAP+i+1);
					if (r_deluxeMapping->integer && tr.worldDeluxeMapping)
					{
						stages[lmStage+i+1].bundle[TB_DELUXEMAP].image[0] = tr.deluxemaps[shader.lightmapIndex[i+1]];
						stages[lmStage+i+1].bundle[TB_DELUXEMAP].tcGen = (texCoordGen_t)(TCGEN_LIGHTMAP+i+1);
					}
				}
				stages[lmStage+i+1].rgbGen = CGEN_LIGHTMAPSTYLE;
				stages[lmStage+i+1].stateBits &= ~(GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS);
				stages[lmStage+i+1].stateBits |= GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE;
			}
		}

		for(i=0;i<=numStyles;i++)
		{
			stages[lmStage+i].lightmapStyle = shader.styles[i];
		}
	}

	//
	// set appropriate stage information
	//
	for ( stage = 0; stage < MAX_SHADER_STAGES; ) {
		shaderStage_t *pStage = &stages[stage];

		if ( !pStage->active ) {
			break;
		}

		// check for a missing texture
		if ( !pStage->bundle[0].image[0] ) {
			ri.Printf( PRINT_WARNING, "Shader %s, stage %d has no image. This stage will be ignored\n", shader.name, stage + 1 );
			pStage->active = qfalse;
			stage++;
			continue;
		}

		//
		// ditch this stage if it's detail and detail textures are disabled
		//
		if ( pStage->isDetail && !r_detailTextures->integer )
		{
			int index;

			for(index = stage + 1; index < MAX_SHADER_STAGES; index++)
			{
				if(!stages[index].active)
					break;
			}

			if(index < MAX_SHADER_STAGES)
				memmove(pStage, pStage + 1, sizeof(*pStage) * (index - stage));
			else
			{
				if(stage + 1 < MAX_SHADER_STAGES)
					memmove(pStage, pStage + 1, sizeof(*pStage) * (index - stage - 1));

				Com_Memset(&stages[index - 1], 0, sizeof(*stages));
			}

			continue;
		}

		pStage->stateBits |= shaderStateBits;

		//
		// default texture coordinate generation
		//
		if ( pStage->bundle[0].isLightmap ) {
			if ( pStage->bundle[0].tcGen == TCGEN_BAD ) {
				pStage->bundle[0].tcGen = TCGEN_LIGHTMAP;
				if (r_deluxeMapping->integer && tr.worldDeluxeMapping)
					pStage->bundle[TB_DELUXEMAP].tcGen = TCGEN_LIGHTMAP;
			}
			hasLightmapStage = qtrue;
		} else {
			if ( pStage->bundle[0].tcGen == TCGEN_BAD ) {
				pStage->bundle[0].tcGen = TCGEN_TEXTURE;
			}
		}

		//
		// determine sort order and fog color adjustment
		//
		if ( ( pStage->stateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) &&
			 ( stages[0].stateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) ) {
			int blendSrcBits = pStage->stateBits & GLS_SRCBLEND_BITS;
			int blendDstBits = pStage->stateBits & GLS_DSTBLEND_BITS;

			// fog color adjustment only works for blend modes that have a contribution
			// that aproaches 0 as the modulate values aproach 0 --
			// GL_ONE, GL_ONE
			// GL_ZERO, GL_ONE_MINUS_SRC_COLOR
			// GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA

			// modulate, additive
			if ( ( ( blendSrcBits == GLS_SRCBLEND_ONE ) && ( blendDstBits == GLS_DSTBLEND_ONE ) ) ||
				( ( blendSrcBits == GLS_SRCBLEND_ZERO ) && ( blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_COLOR ) ) ) {
				pStage->adjustColorsForFog = ACFF_MODULATE_RGB;
			}
			// strict blend
			else if ( ( blendSrcBits == GLS_SRCBLEND_SRC_ALPHA ) && ( blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA ) )
			{
				pStage->adjustColorsForFog = ACFF_MODULATE_ALPHA;
			}
			// premultiplied alpha
			else if ( ( blendSrcBits == GLS_SRCBLEND_ONE ) && ( blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA ) )
			{
				pStage->adjustColorsForFog = ACFF_MODULATE_RGBA;
			} else {
				// we can't adjust this one correctly, so it won't be exactly correct in fog
			}

			if (pStage->alphaGen == AGEN_PORTAL && pStage->adjustColorsForFog == ACFF_MODULATE_ALPHA)
				pStage->adjustColorsForFog = ACFF_NONE;

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

	//
	// if we are in r_vertexLight mode, never use a lightmap texture
	//
	if ( stage > 1 && (r_vertexLight->integer && !r_uiFullScreen->integer) ) {
		VertexLightingCollapse();
		hasLightmapStage = qfalse;
	}

	//
	// look for multitexture potential
	//
	stage = CollapseStagesToGLSL();

	if ( shader.lightmapIndex[0] >= 0 && !hasLightmapStage ) {
		ri.Printf( PRINT_DEVELOPER, "WARNING: shader '%s' has lightmap but no lightmap stage!\n", shader.name );
		// Don't set this, it will just add duplicate shaders to the hash
  		//shader.lightmapIndex = LIGHTMAP_NONE;
	}

	//
	// compute number of passes
	//
	shader.numUnfoggedPasses = stage;

	// fogonly shaders don't have any normal passes
	if (stage == 0 && !shader.isSky)
		shader.sort = SS_FOG;

	// determain if the shader can be simplified when beeing rendered to depth
	if (shader.sort == SS_OPAQUE &&
		shader.numDeforms == 0)
	{
		for (stage = 0; stage < MAX_SHADER_STAGES; stage++) {
			shaderStage_t *pStage = &stages[stage];

			if (!pStage->active)
				continue;

			if (pStage->alphaTestType == ALPHA_TEST_NONE)
				shader.useSimpleDepthShader = qtrue;
			break;
		}
	}

	// determine which stage iterator function is appropriate
	ComputeStageIteratorFunc();

	// determine which vertex attributes this shader needs
	ComputeVertexAttribs();

	return GeneratePermanentShader();
}

//========================================================================================

/*
====================
FindShaderInShaderText

Scans the combined text description of all the shader files for
the given shader name.

return NULL if not found

If found, it will return a valid shader
=====================
*/
static const char *FindShaderInShaderText( const char *shadername ) {

	char *token;
	const char *p;

	int i, hash;

	hash = generateHashValue(shadername, MAX_SHADERTEXT_HASH);

	if(shaderTextHashTable[hash])
	{
		for (i = 0; shaderTextHashTable[hash][i]; i++)
		{
			p = shaderTextHashTable[hash][i];
			token = COM_ParseExt(&p, qtrue);

			if(!Q_stricmp(token, shadername))
				return p;
		}
	}

	p = s_shaderText;

	if ( !p ) {
		return NULL;
	}

	// look for label
	while ( 1 ) {
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 ) {
			break;
		}

		if ( !Q_stricmp( token, shadername ) ) {
			return p;
		}
		else {
			// skip the definition
			SkipBracedSection( &p, 0 );
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

	if ( (name==NULL) || (name[0] == 0) ) {
		return tr.defaultShader;
	}

	COM_StripExtension(name, strippedName, sizeof(strippedName));

	hash = generateHashValue(strippedName, FILE_HASH_SIZE);

	//
	// see if the shader is already loaded
	//
	for (sh=hashTable[hash]; sh; sh=sh->next) {
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if (Q_stricmp(sh->name, strippedName) == 0) {
			// match found
			return sh;
		}
	}

	return tr.defaultShader;
}

static qboolean IsShader ( const shader_t *sh, const char *name, const int *lightmapIndexes, const byte *styles )
{
	if ( Q_stricmp (sh->name, name) != 0 )
	{
		return qfalse;
	}

	if ( sh->defaultShader )
	{
		return qtrue;
	}

	for ( int i = 0; i < MAXLIGHTMAPS; i++ )
	{
		if ( sh->lightmapIndex[i] != lightmapIndexes[i] )
		{
			return qfalse;
		}

		if ( sh->styles[i] != styles[i] )
		{
			return qfalse;
		}
	}

	return qtrue;
}

/*
===============
R_FindLightmaps
===============
*/
static inline const int *R_FindLightmaps(const int *lightmapIndexes)
{
	// don't bother with vertex lighting
	if (*lightmapIndexes < 0)
		return lightmapIndexes;

	// do the lightmaps exist?
	for (int i = 0; i < MAXLIGHTMAPS; i++)
	{
		if (lightmapIndexes[i] >= tr.numLightmaps || tr.lightmaps[lightmapIndexes[i]] == NULL)
			return lightmapsVertex;
	}
	return lightmapIndexes;
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
shader_t *R_FindShader( const char *name, const int *lightmapIndexes, const byte *styles, qboolean mipRawImage ) {
	char		strippedName[MAX_QPATH];
	int			hash, flags;
	const char	*shaderText;
	image_t		*image;
	shader_t	*sh;

	if ( name[0] == 0 ) {
		return tr.defaultShader;
	}

	// use (fullbright) vertex lighting if the bsp file doesn't have
	// lightmaps
	if ( lightmapIndexes[0] >= 0 && lightmapIndexes[0] >= tr.numLightmaps ) {
		lightmapIndexes = lightmapsVertex;
	} else if ( lightmapIndexes[0] < LIGHTMAP_2D ) {
		// negative lightmap indexes cause stray pointers (think tr.lightmaps[lightmapIndex])
		ri.Printf( PRINT_WARNING, "WARNING: shader '%s' has invalid lightmap index of %d\n", name, lightmapIndexes[0]  );
		lightmapIndexes = lightmapsVertex;
	}

	lightmapIndexes = R_FindLightmaps(lightmapIndexes);

	COM_StripExtension(name, strippedName, sizeof(strippedName));

	hash = generateHashValue(strippedName, FILE_HASH_SIZE);

	//
	// see if the shader is already loaded
	//
	for (sh = hashTable[hash]; sh; sh = sh->next) {
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if ( IsShader (sh, strippedName, lightmapIndexes, styles) ) {
			// match found
			return sh;
		}
	}

	// clear the global shader
	ClearGlobalShader();
	Q_strncpyz(shader.name, strippedName, sizeof(shader.name));
	Com_Memcpy (shader.lightmapIndex, lightmapIndexes, sizeof (shader.lightmapIndex));
	Com_Memcpy (shader.styles, styles, sizeof (shader.styles));
	switch (lightmapIndexes[0]) {
	case LIGHTMAP_2D:
	case LIGHTMAP_WHITEIMAGE:
	{
		shader.isHDRLit = qfalse;
		break;
	}
	default:
	{
		shader.isHDRLit = tr.hdrLighting;
		break;
	}
	}

	//
	// attempt to define shader from an explicit parameter file
	//
	shaderText = FindShaderInShaderText( strippedName );
	if ( shaderText ) {
		// enable this when building a pak file to get a global list
		// of all explicit shaders
		if ( r_printShaders->integer ) {
			ri.Printf( PRINT_ALL, "*SHADER* %s\n", name );
		}

		if ( !ParseShader( &shaderText ) ) {
			// had errors, so use default shader
			shader.defaultShader = qtrue;
		}
		sh = FinishShader();
		return sh;
	}


	//
	// if not defined in the in-memory shader descriptions,
	// look for a single supported image file
	//

	flags = IMGFLAG_NONE;

	if (shader.isHDRLit == qtrue)
		flags |= IMGFLAG_SRGB;

	if (mipRawImage)
	{
		flags |= IMGFLAG_MIPMAP | IMGFLAG_PICMIP;

		if (r_genNormalMaps->integer)
			flags |= IMGFLAG_GENNORMALMAP;
	}
	else
	{
		flags |= IMGFLAG_CLAMPTOEDGE;
	}

	image = R_FindImageFile( name, IMGTYPE_COLORALPHA, flags );
	if ( !image ) {
		ri.Printf( PRINT_DEVELOPER, "Couldn't find image file for shader %s\n", name );
		shader.defaultShader = qtrue;
		return FinishShader();
	}

	//
	// create the default shading commands
	//
	if ( shader.lightmapIndex[0] == LIGHTMAP_NONE ) {
		// dynamic colors at vertexes
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
		stages[0].stateBits = GLS_DEFAULT;
	} else if ( shader.lightmapIndex[0] == LIGHTMAP_BY_VERTEX ) {
		// explicit colors at vertexes
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_EXACT_VERTEX;
		stages[0].alphaGen = AGEN_SKIP;
		stages[0].stateBits = GLS_DEFAULT;
	} else if ( shader.lightmapIndex[0] == LIGHTMAP_2D ) {
		// GUI elements
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_VERTEX;
		stages[0].alphaGen = AGEN_VERTEX;
		stages[0].stateBits = GLS_DEPTHTEST_DISABLE |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	} else if ( shader.lightmapIndex[0] == LIGHTMAP_WHITEIMAGE ) {
		// fullbright level
		stages[0].bundle[0].image[0] = tr.whiteImage;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	} else {
		// two pass lightmap
		stages[0].bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex[0]];
		stages[0].bundle[0].isLightmap = qtrue;
		if (r_deluxeMapping->integer && tr.worldDeluxeMapping)
			stages[0].bundle[TB_DELUXEMAP].image[0] = tr.deluxemaps[shader.lightmapIndex[0]];
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY;	// lightmaps are scaled on creation
													// for identitylight
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	}

	return FinishShader();
}

shader_t *R_FindServerShader( const char *name, const int *lightmapIndexes, const byte *styles, qboolean mipRawImage )
{
	char		strippedName[MAX_QPATH];
	int			hash;
	shader_t	*sh;

	if ( name[0] == 0 ) {
		return tr.defaultShader;
	}

	COM_StripExtension( name, strippedName, sizeof( strippedName ) );

	hash = generateHashValue(strippedName, FILE_HASH_SIZE);

	//
	// see if the shader is already loaded
	//
	for (sh = hashTable[hash]; sh; sh = sh->next) {
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if ( IsShader (sh, name, lightmapIndexes, styles) ) {
			// match found
			return sh;
		}
	}

	// clear the global shader
	ClearGlobalShader();
	Q_strncpyz(shader.name, strippedName, sizeof(shader.name));
	Com_Memcpy (shader.lightmapIndex, lightmapIndexes, sizeof (shader.lightmapIndex));

	shader.defaultShader = qtrue;
	return FinishShader();
}

qhandle_t RE_RegisterShaderFromImage(const char *name, const int *lightmapIndexes, const byte *styles, image_t *image, qboolean mipRawImage) {
	int			hash;
	shader_t	*sh;

	hash = generateHashValue(name, FILE_HASH_SIZE);

	// probably not necessary since this function
	// only gets called from tr_font.c with lightmapIndex == LIGHTMAP_2D
	// but better safe than sorry.
	if ( lightmapIndexes[0] >= tr.numLightmaps ) {
		lightmapIndexes = lightmapsFullBright;
	}

	//
	// see if the shader is already loaded
	//
	for (sh=hashTable[hash]; sh; sh=sh->next) {
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if ( IsShader (sh, name, lightmapIndexes, styles) ) {
			// match found
			return sh->index;
		}
	}

	// clear the global shader
	ClearGlobalShader();
	Q_strncpyz(shader.name, name, sizeof(shader.name));
	Com_Memcpy (shader.lightmapIndex, lightmapIndexes, sizeof (shader.lightmapIndex));

	//
	// create the default shading commands
	//
	shader.isHDRLit = tr.hdrLighting;
	if ( shader.lightmapIndex[0] == LIGHTMAP_NONE ) {
		// dynamic colors at vertexes
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
		stages[0].stateBits = GLS_DEFAULT;
	} else if ( shader.lightmapIndex[0] == LIGHTMAP_BY_VERTEX ) {
		// explicit colors at vertexes
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_EXACT_VERTEX;
		stages[0].alphaGen = AGEN_SKIP;
		stages[0].stateBits = GLS_DEFAULT;
	} else if ( shader.lightmapIndex[0] == LIGHTMAP_2D ) {
		// GUI elements
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_VERTEX;
		stages[0].alphaGen = AGEN_VERTEX;
		stages[0].stateBits = GLS_DEPTHTEST_DISABLE |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
		shader.isHDRLit = qfalse;
	} else if ( shader.lightmapIndex[0] == LIGHTMAP_WHITEIMAGE ) {
		// fullbright level
		stages[0].bundle[0].image[0] = tr.whiteImage;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
		shader.isHDRLit = qfalse;
	} else {
		// two pass lightmap
		stages[0].bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex[0]];
		stages[0].bundle[0].isLightmap = qtrue;
		if (r_deluxeMapping->integer && tr.worldDeluxeMapping)
			stages[0].bundle[TB_DELUXEMAP].image[0] = tr.deluxemaps[shader.lightmapIndex[0]];
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY;	// lightmaps are scaled on creation
													// for identitylight
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	}

	sh = FinishShader();
  return sh->index;
}


/*
====================
RE_RegisterShader

This is the exported shader entry point for the rest of the system
It will always return an index that will be valid.

This should really only be used for explicit shaders, because there is no
way to ask for different implicit lighting modes (vertex, lightmap, etc)
====================
*/
qhandle_t RE_RegisterShaderLightMap( const char *name, const int *lightmapIndexes, const byte *styles ) {
	shader_t	*sh;

	if ( strlen( name ) >= MAX_QPATH ) {
		ri.Printf( PRINT_ALL, "Shader name exceeds MAX_QPATH\n" );
		return 0;
	}

	sh = R_FindShader( name, lightmapIndexes, styles, qtrue );

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if ( sh->defaultShader ) {
		return 0;
	}

	return sh->index;
}


/*
====================
RE_RegisterShader

This is the exported shader entry point for the rest of the system
It will always return an index that will be valid.

This should really only be used for explicit shaders, because there is no
way to ask for different implicit lighting modes (vertex, lightmap, etc)
====================
*/
qhandle_t RE_RegisterShader( const char *name ) {
	shader_t	*sh;

	if ( strlen( name ) >= MAX_QPATH ) {
		ri.Printf( PRINT_ALL, "Shader name exceeds MAX_QPATH\n" );
		return 0;
	}

	sh = R_FindShader( name, lightmaps2d, stylesDefault, qtrue );

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if ( sh->defaultShader ) {
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
qhandle_t RE_RegisterShaderNoMip( const char *name ) {
	shader_t	*sh;

	if ( strlen( name ) >= MAX_QPATH ) {
		ri.Printf( PRINT_ALL, "Shader name exceeds MAX_QPATH\n" );
		return 0;
	}

	sh = R_FindShader( name, lightmaps2d, stylesDefault, qfalse );

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if ( sh->defaultShader ) {
		return 0;
	}

	return sh->index;
}

//added for ui -rww
const char *RE_ShaderNameFromIndex(int index)
{
	assert(index >= 0 && index < tr.numShaders && tr.shaders[index]);
	return tr.shaders[index]->name;
}

/*
====================
R_GetShaderByHandle

When a handle is passed in by another module, this range checks
it and returns a valid (possibly default) shader_t to be used internally.
====================
*/
shader_t *R_GetShaderByHandle( qhandle_t hShader ) {
	if ( hShader < 0 ) {
	  ri.Printf( PRINT_WARNING, "R_GetShaderByHandle: out of range hShader '%d'\n", hShader );
		return tr.defaultShader;
	}
	if ( hShader >= tr.numShaders ) {
		ri.Printf( PRINT_WARNING, "R_GetShaderByHandle: out of range hShader '%d'\n", hShader );
		return tr.defaultShader;
	}
	return tr.shaders[hShader];
}

/*
===============
R_ShaderList_f

Dump information on all valid shaders to the console
A second parameter will cause it to print in sorted order
===============
*/
void	R_ShaderList_f (void) {
	int			i;
	int			count;
	shader_t	*shader;

	ri.Printf (PRINT_ALL, "-----------------------\n");

	count = 0;
	for ( i = 0 ; i < tr.numShaders ; i++ ) {
		if ( ri.Cmd_Argc() > 1 ) {
			shader = tr.sortedShaders[i];
		} else {
			shader = tr.shaders[i];
		}

		ri.Printf( PRINT_ALL, "%i ", shader->numUnfoggedPasses );

		if (shader->lightmapIndex[0] >= 0 ) {
			ri.Printf (PRINT_ALL, "L ");
		} else {
			ri.Printf (PRINT_ALL, "  ");
		}

		if ( shader->explicitlyDefined ) {
			ri.Printf( PRINT_ALL, "E " );
		} else {
			ri.Printf( PRINT_ALL, "  " );
		}

		if ( shader->optimalStageIteratorFunc == RB_StageIteratorGeneric ) {
			ri.Printf( PRINT_ALL, "gen " );
		} else if ( shader->optimalStageIteratorFunc == RB_StageIteratorSky ) {
			ri.Printf( PRINT_ALL, "sky " );
		} else {
			ri.Printf( PRINT_ALL, "    " );
		}

		if ( shader->defaultShader ) {
			ri.Printf (PRINT_ALL,  ": %s (DEFAULTED)\n", shader->name);
		} else {
			ri.Printf (PRINT_ALL,  ": %s\n", shader->name);
		}
		count++;
	}
	ri.Printf (PRINT_ALL, "%i total shaders\n", count);
	ri.Printf (PRINT_ALL, "------------------\n");
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
	char **shaderFiles;
	char *buffers[MAX_SHADER_FILES];
	const char *p;
	int numShaderFiles;
	int i;
	char *oldp, *token, *hashMem, *textEnd;
	int shaderTextHashTableSizes[MAX_SHADERTEXT_HASH], hash, size;
	char shaderName[MAX_QPATH];
	int shaderLine;

	long sum = 0, summand;
	// scan for shader files
	shaderFiles = ri.FS_ListFiles( "shaders", ".shader", &numShaderFiles );

	if ( !shaderFiles || !numShaderFiles )
	{
		ri.Printf( PRINT_WARNING, "WARNING: no shader files found\n" );
		return;
	}

	if ( numShaderFiles > MAX_SHADER_FILES ) {
		numShaderFiles = MAX_SHADER_FILES;
	}

	// load and parse shader files
	for ( i = 0; i < numShaderFiles; i++ )
	{
		char filename[MAX_QPATH];

		// look for a .mtr file first
		{
			char *ext;
			Com_sprintf( filename, sizeof( filename ), "shaders/%s", shaderFiles[i] );
			if ( (ext = strrchr(filename, '.')) )
			{
				strcpy(ext, ".mtr");
			}

			if ( ri.FS_ReadFile( filename, NULL ) <= 0 )
			{
				Com_sprintf( filename, sizeof( filename ), "shaders/%s", shaderFiles[i] );
			}
		}

		ri.Printf( PRINT_DEVELOPER, "...loading '%s'\n", filename );
		summand = ri.FS_ReadFile( filename, (void **)&buffers[i] );

		if ( !buffers[i] )
			ri.Error( ERR_DROP, "Couldn't load %s", filename );

		// Do a simple check on the shader structure in that file to make sure one bad shader file cannot fuck up all other shaders.
		p = buffers[i];
		COM_BeginParseSession(filename);
		while(1)
		{
			token = COM_ParseExt(&p, qtrue);

			if(!*token)
				break;

			Q_strncpyz(shaderName, token, sizeof(shaderName));
			shaderLine = COM_GetCurrentParseLine();

			if (token[0] == '#')
			{
				ri.Printf(PRINT_WARNING, "WARNING: Deprecated shader comment \"%s\" on line %d in file %s.  Ignoring line.\n",
					shaderName, shaderLine, filename);
				SkipRestOfLine(&p);
				continue;
			}

			token = COM_ParseExt(&p, qtrue);
			if(token[0] != '{' || token[1] != '\0')
			{
				ri.Printf(PRINT_WARNING, "WARNING: Ignoring shader file %s. Shader \"%s\" on line %d missing opening brace",
							filename, shaderName, shaderLine);
				if (token[0])
				{
					ri.Printf(PRINT_WARNING, " (found \"%s\" on line %d)", token, COM_GetCurrentParseLine());
				}
				ri.Printf(PRINT_WARNING, ".\n");
				ri.FS_FreeFile(buffers[i]);
				buffers[i] = NULL;
				break;
			}

			if(!SkipBracedSection(&p, 1))
			{
				ri.Printf(PRINT_WARNING, "WARNING: Ignoring shader file %s. Shader \"%s\" on line %d missing closing brace.\n",
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
	s_shaderText = (char *)ri.Hunk_Alloc( sum + numShaderFiles*2, h_low );
	s_shaderText[ 0 ] = '\0';
	textEnd = s_shaderText;

	// free in reverse order, so the temp files are all dumped
	for ( i = numShaderFiles - 1; i >= 0 ; i-- )
	{
		if ( !buffers[i] )
			continue;

		strcat( textEnd, buffers[i] );
		strcat( textEnd, "\n" );
		textEnd += strlen( textEnd );
		ri.FS_FreeFile( buffers[i] );
	}

	COM_Compress( s_shaderText );

	// free up memory
	ri.FS_FreeFileList( shaderFiles );

	Com_Memset(shaderTextHashTableSizes, 0, sizeof(shaderTextHashTableSizes));
	size = 0;

	p = s_shaderText;
	// look for shader names
	while ( 1 ) {
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 ) {
			break;
		}

		hash = generateHashValue(token, MAX_SHADERTEXT_HASH);
		shaderTextHashTableSizes[hash]++;
		size++;
		SkipBracedSection(&p, 0);
	}

	size += MAX_SHADERTEXT_HASH;

	hashMem = (char *)ri.Hunk_Alloc( size * sizeof(char *), h_low );

	for (i = 0; i < MAX_SHADERTEXT_HASH; i++) {
		shaderTextHashTable[i] = (char **) hashMem;
		hashMem = ((char *) hashMem) + ((shaderTextHashTableSizes[i] + 1) * sizeof(char *));
	}

	Com_Memset(shaderTextHashTableSizes, 0, sizeof(shaderTextHashTableSizes));

	p = s_shaderText;
	// look for shader names
	while ( 1 ) {
		oldp = (char *)p;
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 ) {
			break;
		}

		hash = generateHashValue(token, MAX_SHADERTEXT_HASH);
		shaderTextHashTable[hash][shaderTextHashTableSizes[hash]++] = oldp;

		SkipBracedSection(&p, 0);
	}

	return;

}

shader_t *R_CreateShaderFromTextureBundle(
		const char *name,
		const textureBundle_t *bundle,
		uint32_t stateBits)
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
====================
CreateInternalShaders
====================
*/
static void CreateInternalShaders( void ) {
	tr.numShaders = 0;

	// init the default shader
	Com_Memset( &shader, 0, sizeof( shader ) );
	Com_Memset( &stages, 0, sizeof( stages ) );

	Q_strncpyz( shader.name, "<default>", sizeof( shader.name ) );

	Com_Memcpy (shader.lightmapIndex, lightmapsNone, sizeof (shader.lightmapIndex));
	for ( int i = 0 ; i < MAX_SHADER_STAGES ; i++ ) {
		stages[i].bundle[0].texMods = texMods[i];
	}

	stages[0].bundle[0].image[0] = tr.defaultImage;
	stages[0].active = qtrue;
	stages[0].stateBits = GLS_DEFAULT;
	tr.defaultShader = FinishShader();

	// shadow shader is just a marker
	Q_strncpyz( shader.name, "<stencil shadow>", sizeof( shader.name ) );
	shader.sort = SS_BANNER; //SS_STENCIL_SHADOW;
	tr.shadowShader = FinishShader();

	// distortion shader is just a marker
	Q_strncpyz( shader.name, "internal_distortion", sizeof( shader.name ) );
	shader.sort = SS_BLEND0;
	shader.defaultShader = qfalse;
	tr.distortionShader = FinishShader();
	tr.distortionShader->useDistortion = qtrue;
	shader.defaultShader = qtrue;

	// weather shader placeholder
	Q_strncpyz(shader.name, "<weather>", sizeof(shader.name));
	shader.sort = SS_SEE_THROUGH;
	tr.weatherInternalShader = FinishShader();
}

static void CreateExternalShaders( void ) {
	tr.projectionShadowShader = R_FindShader( "projectionShadow", lightmapsNone, stylesDefault, qtrue );
	tr.flareShader = R_FindShader( "gfx/misc/flare", lightmapsNone, stylesDefault, qtrue );

	tr.sunShader = R_FindShader( "sun", lightmapsNone, stylesDefault, qtrue );

	tr.sunFlareShader = R_FindShader( "gfx/2d/sunflare", lightmapsNone, stylesDefault, qtrue);

	// HACK: if sunflare is missing, make one using the flare image or dlight image
	if (tr.sunFlareShader->defaultShader)
	{
		image_t *image;

		if (!tr.flareShader->defaultShader && tr.flareShader->stages[0] && tr.flareShader->stages[0]->bundle[0].image[0])
			image = tr.flareShader->stages[0]->bundle[0].image[0];
		else
			image = tr.dlightImage;

		Com_Memset( &shader, 0, sizeof( shader ) );
		Com_Memset( &stages, 0, sizeof( stages ) );

		Q_strncpyz( shader.name, "gfx/2d/sunflare", sizeof( shader.name ) );

		Com_Memcpy (shader.lightmapIndex, lightmapsNone, sizeof (shader.lightmapIndex));
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].stateBits = GLS_DEFAULT;
		tr.sunFlareShader = FinishShader();
	}

}

/*
==================
R_InitShaders
==================
*/
void R_InitShaders( qboolean server ) {
	ri.Printf( PRINT_ALL, "Initializing Shaders\n" );

	Com_Memset(hashTable, 0, sizeof(hashTable));

	if ( !server )
	{
		CreateInternalShaders();

		ScanAndLoadShaderFiles();

		CreateExternalShaders();
	}
}
