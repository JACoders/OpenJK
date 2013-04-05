// leave this as first line for PCH reasons...
//
#include "../server/exe_headers.h"


#include "tr_local.h"
#include "tr_stl.h"

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
	LS_NONE,
	LS_NONE,
	LS_NONE
};

// tr_shader.c -- this file deals with the parsing and definition of shaders

static char *s_shaderText;

// the shader is parsed into these global variables, then copied into
// dynamically allocated memory if it is valid.
static	shaderStage_t	stages[MAX_SHADER_STAGES];		
static	shader_t		shader;
static	texModInfo_t	texMods[MAX_SHADER_STAGES][TR_MAX_TEXMODS];

#define FILE_HASH_SIZE		1024
static	shader_t*		sh_hashTable[FILE_HASH_SIZE];

static	char damageShaderName[MAX_QPATH];

static void ClearGlobalShader(void)
{
	int	i;

	memset( &shader, 0, sizeof( shader ) );
	for(i=0;i<MAXLIGHTMAPS;i++)
	{
		shader.styles[i] = LS_NONE;
	}
	damageShaderName[0]=0;
}

/*
Ghoul2 Insert Start
*/
/*
hitMatReg_t		hitMatReg[MAX_HITMAT_ENTRIES];
int		hitMatCount;

// see if we already registered this hit / materials file
static int R_FindHitMat(const char *fname)
{
	int i;
	// simple linear search, this should only be called during precache anyway
	for (i=1; i<hitMatCount; i++)
	{
		if (!stricmp(hitMatReg[i].name, fname))
		{
			return i;
		}
	}
	return 0;
}
*/
/*
Ghoul2 Insert End
*/


/*
===============
ParseVector
===============
*/
qboolean ParseVector( const char **text, int count, float *v ) {
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
NameToAFunc
===============
*/
static unsigned NameToAFunc( const char *funcname )
{	
	if ( !Q_stricmp( funcname, "GT0" ) )
	{
		return GLS_ATEST_GT_0;
	}
	else if ( !Q_stricmp( funcname, "LT128" ) )
	{
		return GLS_ATEST_LT_80;
	}
	else if ( !Q_stricmp( funcname, "GE128" ) )
	{
		return GLS_ATEST_GE_80;
	}
	else if ( !Q_stricmp( funcname, "GE192" ) )
	{
		return GLS_ATEST_GE_C0;
	}

	ri.Printf( PRINT_WARNING, "WARNING: invalid alphaFunc name '%s' in shader '%s'\n", funcname, shader.name );
	return 0;
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
		return GLS_SRCBLEND_DST_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_ONE_MINUS_DST_ALPHA" ) )
	{
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
		return GLS_DSTBLEND_DST_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_ONE_MINUS_DST_ALPHA" ) )
	{
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
		ri.Error( ERR_DROP, "ERROR: too many tcMod stages in shader '%s'\n", shader.name );
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
static void ParseSurfaceSprites(const char *_text, shaderStage_t *stage )
{
	const char *token;
	const char **text = &_text;
	float width, height, density, fadedist;
	int sstype=SURFSPRITE_NONE;

	//
	// spritetype
	//
	token = COM_ParseExt( text, qfalse );

	if (token[0]==0)
	{
		ri.Printf( PRINT_WARNING, "WARNING: missing surfaceSprites params in shader '%s'\n", shader.name );
		return;
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
	else
	{
		ri.Printf( PRINT_WARNING, "WARNING: invalid type in shader '%s'\n", shader.name );
		return;
	}

	//
	// width
	//
	token = COM_ParseExt( text, qfalse );
	if (token[0]==0)
	{
		ri.Printf( PRINT_WARNING, "WARNING: missing surfaceSprites params in shader '%s'\n", shader.name );
		return;
	}
	width=atof(token);
	if (width <= 0)
	{
		ri.Printf( PRINT_WARNING, "WARNING: invalid width in shader '%s'\n", shader.name );
		return;
	}

	//
	// height
	//
	token = COM_ParseExt( text, qfalse );
	if (token[0]==0)
	{
		ri.Printf( PRINT_WARNING, "WARNING: missing surfaceSprites params in shader '%s'\n", shader.name );
		return;
	}
	height=atof(token);
	if (height <= 0)
	{
		ri.Printf( PRINT_WARNING, "WARNING: invalid height in shader '%s'\n", shader.name );
		return;
	}

	//
	// density
	//
	token = COM_ParseExt( text, qfalse );
	if (token[0]==0)
	{
		ri.Printf( PRINT_WARNING, "WARNING: missing surfaceSprites params in shader '%s'\n", shader.name );
		return;
	}
	density=atof(token);
	if (density <= 0)
	{
		ri.Printf( PRINT_WARNING, "WARNING: invalid density in shader '%s'\n", shader.name );
		return;
	}

	//
	// fadedist
	//
	token = COM_ParseExt( text, qfalse );
	if (token[0]==0)
	{
		ri.Printf( PRINT_WARNING, "WARNING: missing surfaceSprites params in shader '%s'\n", shader.name );
		return;
	}
	fadedist=atof(token);
	if (fadedist < 32)
	{
		ri.Printf( PRINT_WARNING, "WARNING: invalid fadedist in shader '%s'\n", shader.name );
		return;
	}

	// These are all set by the command lines.
	stage->ss.surfaceSpriteType = sstype;
	stage->ss.width = width;
	stage->ss.height = height;
	stage->ss.density = density;
	stage->ss.fadeDist = fadedist;

	// These are defaults that can be overwritten.
	stage->ss.fadeMax = fadedist*1.33;
	stage->ss.fadeScale = 0.0;
	stage->ss.wind = 0.0;
	stage->ss.windIdle = 0.0;
	stage->ss.variance[0] = 0.0;
	stage->ss.variance[1] = 0.0;
	stage->ss.facing = SURFSPRITE_FACING_NORMAL;

	// A vertical parameter that needs a default regardless
	stage->ss.vertSkew;

	// These are effect parameters that need defaults nonetheless.
	stage->ss.fxDuration = 1000;		// 1 second
	stage->ss.fxGrow[0] = 0.0;
	stage->ss.fxGrow[1] = 0.0;
	stage->ss.fxAlphaStart = 1.0;	
	stage->ss.fxAlphaEnd = 0.0;

	shader.needsNormal = qtrue;
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
static void ParseSurfaceSpritesOptional( const char *param, const char *_text, shaderStage_t *stage )
{
	const char *token;
	const char **text = &_text;
	float	value;

	//
	// fademax
	//
	if (!Q_stricmp(param, "ssFademax"))
	{
		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing surfacesprite fademax in shader '%s'\n", shader.name );
			return;
		}
		value = atof(token);
		if (value <= stage->ss.fadeDist)
		{
			ri.Printf( PRINT_WARNING, "WARNING: invalid surfacesprite fademax in shader '%s'\n", shader.name );
			return;
		}
		stage->ss.fadeMax=value;
		return;
	}

	//
	// fadescale
	//
	if (!Q_stricmp(param, "ssFadescale"))
	{
		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing surfacesprite fadescale in shader '%s'\n", shader.name );
			return;
		}
		value = atof(token);
		stage->ss.fadeScale=value;
		return;
	}

	//
	// variance
	//
	if (!Q_stricmp(param, "ssVariance"))
	{
		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing surfacesprite variance width in shader '%s'\n", shader.name );
			return;
		}
		value = atof(token);
		if (value < 0)
		{
			ri.Printf( PRINT_WARNING, "WARNING: invalid surfacesprite variance width in shader '%s'\n", shader.name );
			return;
		}
		stage->ss.variance[0]=value;

		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing surfacesprite variance height in shader '%s'\n", shader.name );
			return;
		}
		value = atof(token);
		if (value < 0)
		{
			ri.Printf( PRINT_WARNING, "WARNING: invalid surfacesprite variance height in shader '%s'\n", shader.name );
			return;
		}
		stage->ss.variance[1]=value;
		return;
	}

	//
	// hangdown
	//
	if (!Q_stricmp(param, "ssHangdown"))
	{
		if (stage->ss.facing != SURFSPRITE_FACING_NORMAL)
		{
			ri.Printf( PRINT_WARNING, "WARNING: Hangdown facing overrides previous facing in shader '%s'\n", shader.name );
			return;
		}
		stage->ss.facing=SURFSPRITE_FACING_DOWN;
		return;
	}

	//
	// anyangle
	//
	if (!Q_stricmp(param, "ssAnyangle"))
	{
		if (stage->ss.facing != SURFSPRITE_FACING_NORMAL)
		{
			ri.Printf( PRINT_WARNING, "WARNING: Anyangle facing overrides previous facing in shader '%s'\n", shader.name );
			return;
		}
		stage->ss.facing=SURFSPRITE_FACING_ANY;
		return;
	}

	//
	// faceup
	//
	if (!Q_stricmp(param, "ssFaceup"))
	{
		if (stage->ss.facing != SURFSPRITE_FACING_NORMAL)
		{
			ri.Printf( PRINT_WARNING, "WARNING: Faceup facing overrides previous facing in shader '%s'\n", shader.name );
			return;
		}
		stage->ss.facing=SURFSPRITE_FACING_UP;
		return;
	}

	//
	// wind
	//
	if (!Q_stricmp(param, "ssWind"))
	{
		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing surfacesprite wind in shader '%s'\n", shader.name );
			return;
		}
		value = atof(token);
		if (value < 0.0)
		{
			ri.Printf( PRINT_WARNING, "WARNING: invalid surfacesprite wind in shader '%s'\n", shader.name );
			return;
		}
		stage->ss.wind=value;
		if (stage->ss.windIdle <= 0)
		{	// Also override the windidle, it usually is the same as wind
			stage->ss.windIdle = value;
		}
		return;
	}

	//
	// windidle
	//
	if (!Q_stricmp(param, "ssWindidle"))
	{
		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing surfacesprite windidle in shader '%s'\n", shader.name );
			return;
		}
		value = atof(token);
		if (value < 0.0)
		{
			ri.Printf( PRINT_WARNING, "WARNING: invalid surfacesprite windidle in shader '%s'\n", shader.name );
			return;
		}
		stage->ss.windIdle=value;
		return;
	}

	//
	// vertskew
	//
	if (!Q_stricmp(param, "ssVertskew"))
	{
		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing surfacesprite vertskew in shader '%s'\n", shader.name );
			return;
		}
		value = atof(token);
		if (value < 0.0)
		{
			ri.Printf( PRINT_WARNING, "WARNING: invalid surfacesprite vertskew in shader '%s'\n", shader.name );
			return;
		}
		stage->ss.vertSkew=value;
		return;
	}

	//
	// fxduration
	//
	if (!Q_stricmp(param, "ssFXDuration"))
	{
		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing surfacesprite duration in shader '%s'\n", shader.name );
			return;
		}
		value = atof(token);
		if (value <= 0)
		{
			ri.Printf( PRINT_WARNING, "WARNING: invalid surfacesprite duration in shader '%s'\n", shader.name );
			return;
		}
		stage->ss.fxDuration=value;
		return;
	}

	//
	// fxgrow
	//
	if (!Q_stricmp(param, "ssFXGrow"))
	{
		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing surfacesprite grow width in shader '%s'\n", shader.name );
			return;
		}
		value = atof(token);
		if (value < 0)
		{
			ri.Printf( PRINT_WARNING, "WARNING: invalid surfacesprite grow width in shader '%s'\n", shader.name );
			return;
		}
		stage->ss.fxGrow[0]=value;

		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing surfacesprite grow height in shader '%s'\n", shader.name );
			return;
		}
		value = atof(token);
		if (value < 0)
		{
			ri.Printf( PRINT_WARNING, "WARNING: invalid surfacesprite grow height in shader '%s'\n", shader.name );
			return;
		}
		stage->ss.fxGrow[1]=value;
		return;
	}

	//
	// fxalpharange
	//
	if (!Q_stricmp(param, "ssFXAlphaRange"))
	{
		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing surfacesprite fxalpha start in shader '%s'\n", shader.name );
			return;
		}
		value = atof(token);
		if (value < 0 || value > 1.0)
		{
			ri.Printf( PRINT_WARNING, "WARNING: invalid surfacesprite fxalpha start in shader '%s'\n", shader.name );
			return;
		}
		stage->ss.fxAlphaStart=value;

		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing surfacesprite fxalpha end in shader '%s'\n", shader.name );
			return;
		}
		value = atof(token);
		if (value < 0 || value > 1.0)
		{
			ri.Printf( PRINT_WARNING, "WARNING: invalid surfacesprite fxalpha end in shader '%s'\n", shader.name );
			return;
		}
		stage->ss.fxAlphaEnd=value;
		return;
	}

	//
	// fxweather
	//
	if (!Q_stricmp(param, "ssFXWeather"))
	{
		if (stage->ss.surfaceSpriteType != SURFSPRITE_EFFECT)
		{
			ri.Printf( PRINT_WARNING, "WARNING: weather applied to non-effect surfacesprite in shader '%s'\n", shader.name );
			return;
		}
		stage->ss.surfaceSpriteType = SURFSPRITE_WEATHERFX;
		return;
	}

	// 
	// invalid ss command.
	//
	ri.Printf( PRINT_WARNING, "WARNING: invalid optional surfacesprite param '%s' in shader '%s'\n", param, shader.name );
	return;
}


/*
===================
ParseStage
===================
*/
static qboolean ParseStage( shaderStage_t *stage, const char **text )
{
	char *token;
	int depthMaskBits = GLS_DEPTHMASK_TRUE, blendSrcBits = 0, blendDstBits = 0, atestBits = 0, depthFuncBits = 0;
	qboolean depthMaskExplicit = qfalse;

	stage->active = qtrue;

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
		// map <name>
		//
		else if ( !Q_stricmp( token, "map" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'map' keyword in shader '%s'\n", shader.name );
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
				if ( shader.lightmapIndex[0] < 0 ) {
					stage->bundle[0].image[0] = tr.whiteImage;
				} else {
					stage->bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex[0]];
				}
				continue;
			}
			else
			{
				stage->bundle[0].image[0] = R_FindImageFile( token, !shader.noMipMaps, !shader.noPicMip, !shader.noTC, GL_REPEAT );
				if ( !stage->bundle[0].image[0] )
				{
					ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
					return qfalse;
				}
			}
		}
		//
		// clampmap <name>
		//
		else if ( !Q_stricmp( token, "clampmap" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'clampmap' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			stage->bundle[0].image[0] = R_FindImageFile( token, !shader.noMipMaps, !shader.noPicMip, !shader.noTC, GL_CLAMP );
			if ( !stage->bundle[0].image[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
				return qfalse;
			}
		}
		//
		// animMap[/clampanimMap] <frequency> <image1> .... <imageN>
		//
		else if ( !Q_stricmp( token, "animMap" ) || !Q_stricmp( token, "clampanimMap" ) || !Q_stricmp( token, "oneshotanimMap" ))
		{
			qboolean bClamp = !Q_stricmp( token, "clampanimMap" );
			qboolean oneShot = !Q_stricmp( token, "oneshotanimMap" );

			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for '%s' keyword in shader '%s'\n", (bClamp ? "animMap":"clampanimMap"), shader.name );
				return qfalse;
			}
			stage->bundle[0].imageAnimationSpeed = atof( token );
			stage->bundle[0].oneShotAnimMap = oneShot;

			// parse up to MAX_IMAGE_ANIMATIONS animations
			while ( 1 ) {
				int		num;

				token = COM_ParseExt( text, qfalse );
				if ( !token[0] ) {
					break;
				}
				num = stage->bundle[0].numImageAnimations;
				if ( num < MAX_IMAGE_ANIMATIONS ) {
					stage->bundle[0].image[num] = R_FindImageFile( token, !shader.noMipMaps, !shader.noPicMip, !shader.noTC, bClamp?GL_CLAMP:GL_REPEAT );
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
			stage->bundle[0].videoMapHandle = ri.CIN_PlayCinematic( token, 0, 0, 256, 256, (CIN_loop | CIN_silent | CIN_shader), NULL);
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

			atestBits = NameToAFunc( token );
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
				stage->constantColor[0] = 255 * color[0];
				stage->constantColor[1] = 255 * color[1];
				stage->constantColor[2] = 255 * color[2];

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
			else if ( !Q_stricmp( token, "lightingDiffuse" ) )
			{
				stage->rgbGen = CGEN_LIGHTING_DIFFUSE;
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
				stage->constantColor[3] = 255 * atof( token );
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
				strcat( buffer, token );
				strcat( buffer, " " );
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
		//
		// surfaceSprites <type> ...
		//
		else if ( !Q_stricmp( token, "surfaceSprites" ) )
		{
			char buffer[1024] = "";

			while ( 1 )
			{
				token = COM_ParseExt( text, qfalse );
				if ( token[0] == 0 )
					break;
				strcat( buffer, token );
				strcat( buffer, " " );
			}

			ParseSurfaceSprites( buffer, stage );

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
			strcpy(param,token);

			while ( 1 )
			{
				token = COM_ParseExt( text, qfalse );
				if ( token[0] == 0 )
					break;
				strcat( buffer, token );
				strcat( buffer, " " );
			}

			ParseSurfaceSpritesOptional( param, buffer, stage );

			continue;
		}
		else
		{
			ri.Printf( PRINT_WARNING, "WARNING: unknown parameter '%s' in shader '%s'\n", token, shader.name );
			return qfalse;
		}
	}

	//
	// if cgen isn't explicitly specified, use either identity or identitylighting
	//
	if ( stage->rgbGen == CGEN_BAD ) {
		if ( //blendSrcBits == 0 ||
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
	if ( stage->alphaGen == CGEN_IDENTITY ) {
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
		ds->deformation = (deform_t) (DEFORM_TEXT0 + n);
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
	char		*token;
	const char	*suf[6] = {"rt", "lf", "bk", "ft", "up", "dn"};
	char		pathname[MAX_QPATH];
	int			i;

	// outerbox
	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name );
		return;
	}
	if ( strcmp( token, "-" ) ) {
		for (i=0 ; i<6 ; i++) {
			Com_sprintf( pathname, sizeof(pathname), "%s_%s", token, suf[i] );
			shader.sky.outerbox[i] = R_FindImageFile( ( char * ) pathname, qtrue, qtrue, !shader.noTC, GL_CLAMP );
			if ( !shader.sky.outerbox[i] ) {
				if (i) {
					shader.sky.outerbox[i] = shader.sky.outerbox[i-1];//not found, so let's use the previous image
				}else{
					shader.sky.outerbox[i] = tr.defaultImage;
				}
			}
		}
	}

	// cloudheight
	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name );
		return;
	}
	shader.sky.cloudHeight = atof( token );
	if ( !shader.sky.cloudHeight ) {
		shader.sky.cloudHeight = 512;
	}
	R_InitSkyTexCoords( shader.sky.cloudHeight );


	// innerbox
	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name );
		return;
	}
	if ( strcmp( token, "-" ) ) {
		for (i=0 ; i<6 ; i++) {
			Com_sprintf( pathname, sizeof(pathname), "%s_%s", token, suf[i] );
			shader.sky.innerbox[i] = R_FindImageFile( ( char * ) pathname, qtrue, qtrue, !shader.noTC, GL_CLAMP );
			if ( !shader.sky.innerbox[i] ) {
				if (i) {
					shader.sky.innerbox[i] = shader.sky.innerbox[i];//not found, so let's use the previous
				}else{
					shader.sky.innerbox[i] = tr.defaultImage;
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
void ParseSort( const char **text ) 
{
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
	}else if ( !Q_stricmp( token, "decal" ) ) {
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


// this table is also present in q3map

typedef struct {
	char	*name;
	int		clearSolid, surfaceFlags, contents;
} infoParm_t;
		

const infoParm_t	infoParms[] = {
	// Game content Flags
	{"nonsolid", 	~CONTENTS_SOLID,	0, 				0 },						// special hack to clear solid flag
	{"nonopaque", 	~CONTENTS_OPAQUE,	0, 				0 },						// special hack to clear opaque flag
	{"lava",		~CONTENTS_SOLID,	0,				CONTENTS_LAVA },			// very damaging
	{"slime",		~CONTENTS_SOLID,	0,				CONTENTS_SLIME },			// mildly damaging
	{"water",		~CONTENTS_SOLID,	0,				CONTENTS_WATER },
	{"fog",			~CONTENTS_SOLID,	0,				CONTENTS_FOG},				// carves surfaces entering
	{"shotclip",	~CONTENTS_SOLID,	0,				CONTENTS_SHOTCLIP },		/* block shots, but not people */
	{"playerclip",	~(CONTENTS_SOLID|CONTENTS_OPAQUE),0,CONTENTS_PLAYERCLIP },	   	/* block only the player */ 
	{"monsterclip",	~(CONTENTS_SOLID|CONTENTS_OPAQUE),0,CONTENTS_MONSTERCLIP },		
	{"botclip",		~(CONTENTS_SOLID|CONTENTS_OPAQUE),0,CONTENTS_BOTCLIP },		   	/* NPC do not enter */															
	{"trigger",		~(CONTENTS_SOLID|CONTENTS_OPAQUE),0,CONTENTS_TRIGGER },
	{"nodrop",		~(CONTENTS_SOLID|CONTENTS_OPAQUE),0,CONTENTS_NODROP },			// don't drop items or leave bodies (death fog, lava, etc)
	{"terrain",		~(CONTENTS_SOLID|CONTENTS_OPAQUE),0,CONTENTS_TERRAIN },		   	/* use special terrain collsion */										
	{"ladder",		~(CONTENTS_SOLID|CONTENTS_OPAQUE),0,CONTENTS_LADDER },			// climb up in it like water
	{"abseil",		~(CONTENTS_SOLID|CONTENTS_OPAQUE),0,CONTENTS_ABSEIL },			// can abseil down this brush
	{"outside",		~(CONTENTS_SOLID|CONTENTS_OPAQUE),0,CONTENTS_OUTSIDE },			// volume is considered to be in the outside (i.e. not indoors)
																		
	{"detail",		-1,					0,				CONTENTS_DETAIL },			// don't include in structural bsp
	{"trans",		-1,					0,				CONTENTS_TRANSLUCENT },		// surface has an alpha component
	
	/* Game surface flags */
	{"sky",			-1,					SURF_SKY,		0 },					   	/* emit light from an environment map */
	{"slick",		-1,					SURF_SLICK,		0 },

	{"nodamage",	-1,					SURF_NODAMAGE,	0 },					   	   																	
	{"noimpact",	-1,					SURF_NOIMPACT,	0 },					   	/* don't make impact explosions or marks */
	{"nomarks",		-1,					SURF_NOMARKS,	0 },					   	/* don't make impact marks, but still explode */
	{"nodraw",		-1,					SURF_NODRAW,	0 },					   	/* don't generate a drawsurface (or a lightmap) */
	{"nosteps",		-1,					SURF_NOSTEPS,	0 },
	{"nodlight",	-1,					SURF_NODLIGHT,	0 },					   	/* don't ever add dynamic lights */
	{"metalsteps",	-1,					SURF_METALSTEPS,0 },
	{"nomiscents",	-1,					SURF_NOMISCENTS,0 },						/* No misc ents on this surface */
	{"forcefield",	-1,					SURF_FORCEFIELD,0 },
};


/*
===============
ParseSurfaceParm

surfaceparm <name>
===============
*/
static void ParseSurfaceParm( const char **text ) {
	char	*token;
	int		numInfoParms = sizeof(infoParms) / sizeof(infoParms[0]);
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
static qboolean ParseShader( const char  **text )
{
	char *token;
	int s = 0;

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
			if ( !ParseStage( &stages[s], text ) )
			{
				return qfalse;
			}
			stages[s].active = qtrue;
			s++;
			continue;
		}
		// sun parms
		else if ( !Q_stricmp( token, "q3map_sun" ) || !Q_stricmp( token, "sun" )) {
			float	a, b;

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
		}
		else if ( !Q_stricmp( token, "deformVertexes" ) ) {
			ParseDeform( text );
			continue;
		}
		else if ( !Q_stricmp( token, "tesssize" ) ) {
			SkipRestOfLine( text );
			continue;
		}
		// skip stuff that only the QuakeEdRadient needs
		else if ( !Q_stricmpn( token, "qer", 3 ) ) {
			SkipRestOfLine( text );
			continue;
		}
		// skip stuff that only the q3map needs
		else if ( !Q_stricmpn( token, "q3map", 5 ) ) {
			SkipRestOfLine( text );
			continue;
		}
		// skip stuff that JK2 doesn't use
		else if ( !Q_stricmp( token, "lightColor") ) {
			SkipRestOfLine( text );
			continue;
		}
		// surface parms
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
		// no picmip adjustment
		else if ( !Q_stricmp( token, "nopicmip" ) )
		{
			shader.noPicMip = qtrue;
			continue;
		}
		// polygonOffset
		else if ( !Q_stricmp( token, "polygonOffset" ) )
		{
			shader.polygonOffset = qtrue;
			continue;
		}
		// polygonOffset
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
			token = COM_ParseExt( text, qfalse );
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
		// damage shader
		else if ( !stricmp( token, "damageShader" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if(!token[0])
			{
				Com_Printf( S_COLOR_YELLOW "WARNING: missing damageShader Name in shader '%s'\n", shader.name );
				continue;
			}
			Q_strncpyz(damageShaderName, token, MAX_QPATH);
			token = COM_ParseExt( text, qfalse );
			if(!token[0])
			{
				Com_Printf( S_COLOR_YELLOW "WARNING: missing damageShader Value in shader '%s'\n", shader.name );
				continue;
			}
			shader.damageValue = atol(token);
			continue;
		}
/*
Ghoul2 Insert Start
*/

		// 
		// location hit mesh load
		//
		else if ( !Q_stricmp( token, "hitLocation" ) )
		{
		   
			// grab the filename of the hit location texture
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 )
				break;
/*
			byte	*buffer = 0;
			// have we already loaded this?
			shader.hitLocation = R_FindHitMat(token);
			// nope, better load it in and register it
			if (!shader.hitLocation)
			{
				// sanity check
				if (hitMatCount == MAX_HITMAT_ENTRIES)
				{
					ri.Error(ERR_DROP, "Not enough entry space for hit location file %s\n", token);
				}
				// find us a new spot in the material list - and make sure we avoid the 0 spot in the list
				hitMatCount++;

				// now load that file in
				LoadTGAPalletteImage(token, &buffer, &hitMatReg[hitMatCount].width, &hitMatReg[hitMatCount].height);
				if (buffer)
				{
					hitMatReg[hitMatCount].loc = (byte *)ri.Hunk_Alloc(hitMatReg[hitMatCount].height * hitMatReg[hitMatCount].width, qfalse);
					shader.hitLocation = hitMatCount;

					// copy data into new space in the hunk so we can free the temp malloced space
					memcpy(hitMatReg[hitMatCount].loc, buffer,(hitMatReg[hitMatCount].height * hitMatReg[hitMatCount].width)); 

					// now free original space we loaded it into
					
					Z_Free(buffer);

					Com_sprintf(hitMatReg[hitMatCount].name, MAX_QPATH, "%s", token);
				}
				else
				{
					ri.Printf( PRINT_WARNING, "WARNING: missing hit location data file '%s' in shader %s\n", token, shader.name );
					hitMatCount--;
				}
			}
*/
			continue;
		}
		// 
		// location hit material mesh load
		//
		else if ( !Q_stricmp( token, "hitMaterial" ) )
		{

			// grab the filename of the hit location texture
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 )
				break;
/*
			byte	*buffer = 0;
			// have we already loaded this?
			shader.hitMaterial = R_FindHitMat(token);
			// nope, better load it in and register it
			if (!shader.hitMaterial)
			{
				// sanity check
				if (hitMatCount == MAX_HITMAT_ENTRIES)
				{
					ri.Error(ERR_DROP, "Not enough entry space for hit Material file %s\n", token);
				}
				// find us a new spot in the material list - and make sure we avoid the 0 spot in the list
				hitMatCount++;

				// now load that file in
				LoadTGAPalletteImage(token, &buffer, &hitMatReg[hitMatCount].width, &hitMatReg[hitMatCount].height);
				if (buffer)
				{

					hitMatReg[hitMatCount].loc = (byte *)ri.Hunk_Alloc(hitMatReg[hitMatCount].height * hitMatReg[hitMatCount].width, qfalse);
					shader.hitMaterial = hitMatCount;

					// copy data into new space in the hunk so we can free the temp malloced space
					memcpy(hitMatReg[hitMatCount].loc, buffer,(hitMatReg[hitMatCount].height * hitMatReg[hitMatCount].width)); 

					// now free original space we loaded it into
					
					Z_Free(buffer);

					Com_sprintf(hitMatReg[hitMatCount].name, MAX_QPATH, "%s", token);
				}
 				else
				{
					ri.Printf( PRINT_WARNING, "WARNING: missing hit material data file '%s' in shader %s\n", token, shader.name );
					hitMatCount--;
				}

			}

*/			continue;

		}
/*
Ghoul2 Insert End
*/

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
		goto done;
	}

	if ( r_ignoreFastPath->integer )
	{
		return;
	}

	//
	// see if this can go into the vertex lit fast path
	//
	if ( shader.numUnfoggedPasses == 1 )
	{
		if ( stages[0].rgbGen == CGEN_LIGHTING_DIFFUSE )
		{
			if ( stages[0].alphaGen == AGEN_IDENTITY )
			{
				if ( stages[0].bundle[0].tcGen == TCGEN_TEXTURE )
				{
					if ( !shader.polygonOffset )
					{
						if ( !shader.multitextureEnv )
						{
							if ( !shader.numDeforms )
							{
								shader.optimalStageIteratorFunc = RB_StageIteratorVertexLitTexture;
								goto done;
							}
						}
					}
				}
			}
		}
	}

	//
	// see if this can go into an optimized LM, multitextured path
	//
	if ( shader.numUnfoggedPasses == 1 )
	{
		if ( ( stages[0].rgbGen == CGEN_IDENTITY ) && ( stages[0].alphaGen == AGEN_IDENTITY ) )
		{
			if ( stages[0].bundle[0].tcGen == TCGEN_TEXTURE && 
				stages[0].bundle[1].tcGen == TCGEN_LIGHTMAP )
			{
				if ( !shader.polygonOffset )
				{
					if ( !shader.numDeforms )
					{
						if ( shader.multitextureEnv )
						{
							shader.optimalStageIteratorFunc = RB_StageIteratorLightmappedMultitexture;
							goto done;
						}
					}
				}
			}
		}
	}

done:
	return;
}

typedef struct {
	int		blendA;
	int		blendB;

	int		multitextureEnv;
	int		multitextureBlend;
} collapse_t;

static collapse_t	collapse[] = {
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
static qboolean CollapseMultitexture( void ) {
	int abits, bbits;
	int i;
	textureBundle_t tmpBundle;

	if ( !qglActiveTextureARB ) {
		return qfalse;
	}

	// make sure both stages are active
	if ( !stages[0].active || !stages[1].active ) {
		return qfalse;
	}

	abits = stages[0].stateBits;
	bbits = stages[1].stateBits;

	// make sure that both stages have identical state other than blend modes
	if ( ( abits & ~( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS | GLS_DEPTHMASK_TRUE ) ) !=
		( bbits & ~( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS | GLS_DEPTHMASK_TRUE ) ) ) {
		return qfalse;
	}

	abits &= ( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS );
	bbits &= ( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS );

	// search for a valid multitexture blend function
	for ( i = 0; collapse[i].blendA != -1 ; i++ ) {
		if ( abits == collapse[i].blendA
			&& bbits == collapse[i].blendB ) {
			break;
		}
	}

	// nothing found
	if ( collapse[i].blendA == -1 ) {
		return qfalse;
	}

	// GL_ADD is a separate extension
	if ( collapse[i].multitextureEnv == GL_ADD && !glConfig.textureEnvAddAvailable ) {
		return qfalse;
	}

	// make sure waveforms have identical parameters
	if (( stages[0].rgbGen != stages[1].rgbGen ) ||
		( stages[0].alphaGen != stages[1].alphaGen ) )  {
		return qfalse;
	}

	// an add collapse can only have identity colors
	if ( collapse[i].multitextureEnv == GL_ADD && stages[0].rgbGen != CGEN_IDENTITY ) {
		return qfalse;
	}

	if ( stages[0].rgbGen == CGEN_WAVEFORM )
	{
		if ( memcmp( &stages[0].rgbWave,
					 &stages[1].rgbWave,
					 sizeof( stages[0].rgbWave ) ) )
		{
			return qfalse;
		}
	}
	if ( stages[0].alphaGen == CGEN_WAVEFORM )
	{
		if ( memcmp( &stages[0].alphaWave,
					 &stages[1].alphaWave,
					 sizeof( stages[0].alphaWave ) ) )
		{
			return qfalse;
		}
	}


	// make sure that lightmaps are in bundle 1 for 3dfx
	if ( stages[0].bundle[0].isLightmap )
	{
		tmpBundle = stages[0].bundle[0];
		stages[0].bundle[0] = stages[1].bundle[0];
		stages[0].bundle[1] = tmpBundle;
	}
	else
	{
		stages[0].bundle[1] = stages[1].bundle[0];
	}

	// set the new blend state bits
	shader.multitextureEnv = collapse[i].multitextureEnv;
	stages[0].stateBits &= ~( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS );
	stages[0].stateBits |= collapse[i].multitextureBlend;

	//
	// move down subsequent shaders
	//
	memmove( &stages[1], &stages[2], sizeof( stages[0] ) * ( MAX_SHADER_STAGES - 2 ) );
	memset( &stages[MAX_SHADER_STAGES-1], 0, sizeof( stages[0] ) );

	return qtrue;
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

	for ( i = tr.numShaders - 2 ; i >= 0 ; i-- ) {
		if ( tr.sortedShaders[ i ]->sort <= sort ) {
			break;
		}
		tr.sortedShaders[i+1] = tr.sortedShaders[i];
		tr.sortedShaders[i+1]->sortedIndex++;
	}

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
		tr.iNumDeniedShaders++;
		ri.Printf( PRINT_WARNING, "WARNING: GeneratePermanentShader - MAX_SHADERS (%d) hit (overflowed by %d)\n", MAX_SHADERS, tr.iNumDeniedShaders);
		return tr.defaultShader;
	}

	newShader = (shader_t *)ri.Hunk_Alloc( sizeof( shader_t ), qtrue );

	*newShader = shader;

	if ( shader.sort <= SS_OPAQUE ) {
		newShader->fogPass = FP_EQUAL;
	} else if ( shader.contentFlags & CONTENTS_FOG ) {
		newShader->fogPass = FP_LE;
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
		newShader->stages[i] = (shaderStage_t *) ri.Hunk_Alloc( sizeof( stages[i] ), qtrue );
		*newShader->stages[i] = stages[i];

		for ( b = 0 ; b < NUM_TEXTURE_BUNDLES ; b++ ) {
			size = newShader->stages[i]->bundle[b].numTexMods * sizeof( texModInfo_t );
			newShader->stages[i]->bundle[b].texMods = (texModInfo_t *) ri.Hunk_Alloc( size, qfalse );
			memcpy( newShader->stages[i]->bundle[b].texMods, stages[i].bundle[b].texMods, size );
		}
	}

	SortNewShader();

	hash = generateHashValue(newShader->name);
	newShader->next = sh_hashTable[hash];
	sh_hashTable[hash] = newShader;

	return newShader;
}

/*
=================
VertexLightingCollapse

If vertex lighting is enabled, only render a single
pass, trying to guess which is the correct one to best aproximate
what it is supposed to look like.

  OUTPUT:  Number of stages after the collapse (in the case of surfacesprites this isn't one).
=================
*/
static int VertexLightingCollapse( void ) {
	int		stage, nextopenstage;
	shaderStage_t	*bestStage;
	int		bestImageRank;
	int		rank;
	int		finalstagenum=1;

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

			// SurfaceSprites are most certainly NOT desireable as the collapsed surface texture.
			if ( pStage->ss.surfaceSpriteType)
			{
				rank -= 1000;
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

	for ( stage=1, nextopenstage=1; stage < MAX_SHADER_STAGES; stage++ ) {
		shaderStage_t *pStage = &stages[stage];

		if ( !pStage->active ) {
			break;
		}

		if (pStage->ss.surfaceSpriteType)
		{
			// Copy this stage to the next open stage list (that is, we don't want any inactive stages before this one)
			if (nextopenstage != stage)
			{
				stages[nextopenstage] = *pStage;
				stages[nextopenstage].bundle[0] = pStage->bundle[0];
			}
			nextopenstage++;
			finalstagenum++;
			continue;
		}

		memset( pStage, 0, sizeof( *pStage ) );
	}

	return finalstagenum;
}

/*
=========================
FinishShader

Returns a freshly allocated shader with all the needed info
from the current global working shader
=========================
*/
static shader_t *FinishShader( void ) {
	int				stage, lmStage;
	qboolean		hasLightmapStage;

	hasLightmapStage = qfalse;

	//
	// set sky stuff appropriate
	//
	if ( shader.isSky ) {
		shader.sort = SS_ENVIRONMENT;
	}

	//
	// set polygon offset
	//
	if ( shader.polygonOffset && !shader.sort ) {
		shader.sort = SS_DECAL;
	}

	for(lmStage=0;lmStage<MAX_SHADER_STAGES;lmStage++)
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
			if (lmStage < MAX_SHADER_STAGES-1)
			{
				memmove(&stages[lmStage], &stages[lmStage+1], sizeof(shaderStage_t) * (MAX_SHADER_STAGES-lmStage-1));
			}
			memset(&stages[MAX_SHADER_STAGES-1], 0, sizeof(shaderStage_t));
			stages[lmStage].rgbGen = CGEN_EXACT_VERTEX;
			stages[lmStage].alphaGen = AGEN_SKIP;
			stages[lmStage].stateBits = GLS_DEFAULT;
			lmStage = MAX_SHADER_STAGES;
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
				if (shader.lightmapIndex[i+1] < 0)
				{
					ri.Error( ERR_DROP, "FinishShader: light style with no light map for shader %s", shader.name);
				}

				stages[lmStage+i+1] = stages[lmStage];
				stages[lmStage+i+1].bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex[i+1]];
				stages[lmStage+i+1].bundle[0].tcGen = (texCoordGen_t)(TCGEN_LIGHTMAP+i+1);
				stages[lmStage+i+1].rgbGen = (colorGen_t)(CGEN_LIGHTMAP1+i);
				stages[lmStage+i+1].stateBits &= ~(GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS);
//				stages[lmStage+i+1].stateBits |= GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_COLOR;
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
	for ( stage = 0; stage < MAX_SHADER_STAGES; stage++ ) {
		shaderStage_t *pStage = &stages[stage];

		if ( !pStage->active ) {
			break;
		}

		// check for a missing texture
		if ( !pStage->bundle[0].image[0] ) {
			ri.Printf( PRINT_WARNING, "Shader %s has a stage with no image\n", shader.name );
			pStage->active = qfalse;
			continue;
		}

		//
		// ditch this stage if it's detail and detail textures are disabled
		//
		if ( pStage->isDetail && !r_detailTextures->integer ) {
			if ( stage < ( MAX_SHADER_STAGES - 1 ) ) {
				memmove( pStage, pStage + 1, sizeof( *pStage ) * ( MAX_SHADER_STAGES - stage - 1 ) );
				memset(  pStage + ( MAX_SHADER_STAGES - stage - 1 ), 0, sizeof( *pStage ) );	//clear the last one moved down
				stage--;	//look at this stage next time around
			}
			continue;
		}

		//
		// default texture coordinate generation
		//
		if ( pStage->bundle[0].isLightmap ) {
			if ( pStage->bundle[0].tcGen == TCGEN_BAD ) {
				pStage->bundle[0].tcGen = TCGEN_LIGHTMAP;
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

			// don't screw with sort order if this is a portal or environment
			if ( !shader.sort ) {
				// see through item, like a grill or grate
				if ( pStage->stateBits & GLS_DEPTHMASK_TRUE ) 
				{
					shader.sort = SS_SEE_THROUGH;
				} 
				else 
				{
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
	}

	// there are times when you will need to manually apply a sort to
	// opaque alpha tested shaders that have later blend passes
	if ( !shader.sort ) {
		shader.sort = SS_OPAQUE;
	}

	//
	// if we are in r_vertexLight mode, never use a lightmap texture
	//
	if ( stage > 1 && ( r_vertexLight->integer ) ) {
		stage = VertexLightingCollapse();
		hasLightmapStage = qfalse;
	}

	//
	// look for multitexture potential
	//
	if ( stage > 1 && CollapseMultitexture() ) {
		stage--;
	}

	if ( shader.lightmapIndex[0] >= 0 && !hasLightmapStage ) {
		ri.Printf( PRINT_WARNING, "WARNING: shader '%s' has lightmap but no lightmap stage!\n", shader.name );
		memcpy(shader.lightmapIndex, lightmapsNone, sizeof(shader.lightmapIndex));
		memcpy(shader.styles, stylesDefault, sizeof(shader.styles));
	}


	//
	// compute number of passes
	//
	shader.numUnfoggedPasses = stage;

	// fogonly shaders don't have any normal passes
	if ( stage == 0 ) {
		shader.sort = SS_FOG;
	}

	// determine which stage iterator function is appropriate
	ComputeStageIteratorFunc();

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
	char *p = s_shaderText;

	if ( !p ) {
		return NULL;
	}

#ifdef USE_STL_FOR_SHADER_LOOKUPS
	
	char sLowerCaseName[MAX_QPATH];
	Q_strncpyz(sLowerCaseName,shadername,sizeof(sLowerCaseName));
	strlwr(sLowerCaseName);	// Q_strlwr is pretty gay, so I'm not using it

	return ShaderEntryPtrs_Lookup(sLowerCaseName);

#else

	char *token;

	// look for label
	// note that this could get confused if a shader name is used inside
	// another shader definition
	while ( 1 ) {

		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 ) {
			break;
		}

		if ( token[0] == '{' ) {
			// skip the definition
			SkipBracedSection( &p );
		} else if ( !Q_stricmp( token, shadername ) ) {
			return p;
		} else {
			// skip to end of line
			SkipRestOfLine( &p );
		}
	}

	return NULL;

#endif
}

inline qboolean IsShader(shader_t *sh, const char *name, const int *lightmapIndex, const byte *styles)
{
	int	i;

	if (Q_stricmp(sh->name, name))
	{
		return qfalse;
	}

	if (!sh->defaultShader)
	{
		for(i=0;i<MAXLIGHTMAPS;i++)
		{
			if (sh->lightmapIndex[i] != lightmapIndex[i])
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
shader_t *R_FindShader( const char *name, const int *lightmapIndex, const byte *styles, qboolean mipRawImage ) {
	char		strippedName[MAX_QPATH];
	int			i, hash;
	const char 	*shaderText;
	image_t		*image;
	shader_t	*sh;

	if ( strlen( name ) >= MAX_QPATH ) {
		Com_Printf( S_COLOR_RED"Shader name exceeds MAX_QPATH! %s\n",name );
		return tr.defaultShader;
	}
	if ( name[0] == 0 ) {
		return tr.defaultShader;
	}

	// use (fullbright) vertex lighting if the bsp file doesn't have
	// lightmaps
	if ( lightmapIndex[0] >= 0 && lightmapIndex[0] >= tr.numLightmaps ) {
		lightmapIndex = lightmapsVertex;
	}

	COM_StripExtension( name, strippedName );

	hash = generateHashValue(strippedName);

	//
	// see if the shader is already loaded
	//
	for (sh=sh_hashTable[hash]; sh; sh=sh->next) {
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if (IsShader(sh, strippedName, lightmapIndex, styles))
		{	// match found
			return sh;
		}
	}

	// make sure the render thread is stopped, because we are probably
	// going to have to upload an image
	R_SyncRenderThread();

	// clear the global shader
	ClearGlobalShader();
	memset( &stages, 0, sizeof( stages ) );
	Q_strncpyz(shader.name, strippedName, sizeof(shader.name));
	memcpy(shader.lightmapIndex, lightmapIndex, sizeof(shader.lightmapIndex));
	memcpy(shader.styles, styles, sizeof(shader.styles));
	for ( i = 0 ; i < MAX_SHADER_STAGES ; i++ ) {
		stages[i].bundle[0].texMods = texMods[i];
	}

	// FIXME: set these "need" values apropriately
	shader.needsNormal = qtrue;
	shader.needsST1 = qtrue;
	shader.needsST2 = qtrue;
	shader.needsColor = qtrue;

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
		// Precache and give the same lightmaps and styles to the damage shader (if any)
		if(sh->damageValue && damageShaderName[0])
		{
			char rShaderName[MAX_QPATH];
			strcpy (rShaderName, damageShaderName);
			sh->damageShader = R_FindShader(rShaderName, lightmapIndex, styles, mipRawImage);
			if(sh->damageShader->defaultShader)
			{
				sh->damageShader = NULL;
			}
		}
		return sh;
	}


	//
	// if not defined in the in-memory shader descriptions,
	// look for a single TGA, BMP, or PCX
	//
	image = R_FindImageFile( name, mipRawImage, mipRawImage, qtrue, mipRawImage ? GL_REPEAT : GL_CLAMP );
	if ( !image ) {
		if (strncmp(name, "levelshots", 10 ) ) {	//hide these warnings
			ri.Printf( PRINT_WARNING, "Couldn't find image for shader %s\n", name );
		}
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
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY;			// lightmaps are scaled on creation
													// for identitylight
													// light map 0 should always be style 0, which means
													// that this will always be on
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	}

	return FinishShader();
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
		if ( shader->multitextureEnv == GL_ADD ) {
			ri.Printf( PRINT_ALL, "MT(a) " );
		} else if ( shader->multitextureEnv == GL_MODULATE ) {
			ri.Printf( PRINT_ALL, "MT(m) " );
		} else if ( shader->multitextureEnv == GL_DECAL ) {
			ri.Printf( PRINT_ALL, "MT(d) " );
		} else {
			ri.Printf( PRINT_ALL, "      " );
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
		} else if ( shader->optimalStageIteratorFunc == RB_StageIteratorLightmappedMultitexture ) {
			ri.Printf( PRINT_ALL, "lmmt" );
		} else if ( shader->optimalStageIteratorFunc == RB_StageIteratorVertexLitTexture ) {
			ri.Printf( PRINT_ALL, "vlt " );
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



#ifdef USE_STL_FOR_SHADER_LOOKUPS
// setup my STL shortcut list as to where all the shaders are, saves re-parsing every line for every .TGA request.
// 
static void SetupShaderEntryPtrs(void)
{
	const char *p = s_shaderText;
	char *token;

	ShaderEntryPtrs_Clear();	// extra safe, though done elsewhere already

	if ( !p ) 	
		return;

	while (1) 
	{
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 ) 
			break;				// EOF
		
		if ( token[0] == '{' )	// '}'	// counterbrace for matching
		{				
			SkipBracedSection( &p );
		}
		else
		{
			strlwr(token);	// token is always a ptr to com_token here, not the original buffer. 
							//	(Not that it matters, except for reasons of speed by not strlwr'ing the whole buffer)

			// token = a string of this shader name, p = ptr within s_shadertext it's found at, so store it...
			//
			ShaderEntryPtrs_Insert(token,p);			
			SkipRestOfLine( &p );		// now legally skip over this name and go get the next one
		}
	}

	ri.Printf( PRINT_DEVELOPER, "SetupShaderEntryPtrs(): Stored %d shader ptrs\n",ShaderEntryPtrs_Size() );
}
#endif


/*
====================
ScanAndLoadShaderFiles

Finds and loads all .shader files, combining them into
a single large text block that can be scanned for shader names
=====================
*/
#define	MAX_SHADER_FILES	1024
static void ScanAndLoadShaderFiles( void )
{
	char **shaderFiles;
	char *buffers[MAX_SHADER_FILES];
	int numShaders;
	int i;
	long sum = 0;

	// scan for shader files
	shaderFiles = ri.FS_ListFiles( "shaders", ".shader", &numShaders );

	if ( !shaderFiles || !numShaders )
	{
		ri.Printf( PRINT_WARNING, "WARNING: no shader files found\n" );
		return;
	}

	if ( numShaders > MAX_SHADER_FILES ) {
		numShaders = MAX_SHADER_FILES;
	}

	// load and store shader files
	for ( i = 0; i < numShaders; i++ )
	{
		char filename[MAX_QPATH];

		Com_sprintf( filename, sizeof( filename ), "shaders/%s", shaderFiles[i] );
		ri.Printf( PRINT_DEVELOPER, "...loading '%s'\n", filename );
		sum += ri.FS_ReadFile( filename, (void **)&buffers[i] );
		if ( !buffers[i] ) {
			ri.Error( ERR_DROP, "Couldn't load %s", filename );
		}
	}

	// free up memory
	ri.FS_FreeFileList( shaderFiles );

	// build single large buffer
	s_shaderText = (char *) ri.Hunk_Alloc( sum + numShaders*2, qtrue );

	// free in reverse order, so the temp files are all dumped
	for ( i = numShaders - 1; i >= 0 ; i-- ) {
		strcat( s_shaderText, "\n" );
		strcat( s_shaderText, buffers[i] );
		ri.FS_FreeFile( buffers[i] );
	}

	#ifdef USE_STL_FOR_SHADER_LOOKUPS
	SetupShaderEntryPtrs();
	#endif
}


/*
====================
CreateInternalShaders
====================
*/
static void CreateInternalShaders( void ) {
	tr.numShaders = 0;
	tr.iNumDeniedShaders = 0;

	// init the default shader
	memset( &shader, 0, sizeof( shader ) );
	memset( &stages, 0, sizeof( stages ) );

	Q_strncpyz( shader.name, "<default>", sizeof( shader.name ) );

	memcpy(shader.lightmapIndex, lightmapsNone, sizeof(shader.lightmapIndex));
	memcpy(shader.styles, stylesDefault, sizeof(shader.styles));
	stages[0].bundle[0].image[0] = tr.defaultImage;
	stages[0].active = qtrue;
	stages[0].stateBits = GLS_DEFAULT;
	tr.defaultShader = FinishShader();

	// shadow shader is just a marker
	Q_strncpyz( shader.name, "<stencil shadow>", sizeof( shader.name ) );
	shader.sort = SS_STENCIL_SHADOW;
	tr.shadowShader = FinishShader();
}

static void CreateExternalShaders( void ) {
	tr.projectionShadowShader = R_FindShader( "projectionShadow", lightmapsNone, stylesDefault, qtrue );
	tr.sunShader = R_FindShader( "sun", lightmapsNone, stylesDefault, qtrue );
}

/*
==================
R_InitShaders
==================
*/
void R_InitShaders( void ) {
	ri.Printf( PRINT_ALL, "Initializing Shaders\n" );

	memset(sh_hashTable, 0, sizeof(sh_hashTable));
/*
Ghoul2 Insert Start
*/
//	memset(hitMatReg, 0, sizeof(hitMatReg));
//	hitMatCount = 0;
/*
Ghoul2 Insert End
*/


	CreateInternalShaders();

	ScanAndLoadShaderFiles();

	CreateExternalShaders();
}
