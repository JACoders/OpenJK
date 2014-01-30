#include "tr_local.h"

// tr_shader.c -- this file deals with the parsing and definition of shaders

#define USE_NEW_SHADER_HASH


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vertex and Pixel Shader definitions.	- AReis
/***********************************************************************************************************/
// This vertex shader basically passes through most values and calculates no lighting. The only
// unusual thing it does is add the inputed texel offsets to all four texture units (this allows
// nearest neighbor pixel peeking).
const unsigned char g_strGlowVShaderARB[] =
{
	"!!ARBvp1.0\
	\
	# Input.\n\
	ATTRIB	iPos		= vertex.position;\
	ATTRIB	iColor		= vertex.color;\
	ATTRIB	iTex0		= vertex.texcoord[0];\
	ATTRIB	iTex1		= vertex.texcoord[1];\
	ATTRIB	iTex2		= vertex.texcoord[2];\
	ATTRIB	iTex3		= vertex.texcoord[3];\
	\
	# Output.\n\
	OUTPUT	oPos		= result.position;\
	OUTPUT	oColor		= result.color;\
	OUTPUT	oTex0		= result.texcoord[0];\
	OUTPUT	oTex1		= result.texcoord[1];\
	OUTPUT	oTex2		= result.texcoord[2];\
	OUTPUT	oTex3		= result.texcoord[3];\
	\
	# Constants.\n\
	PARAM	ModelViewProj[4]= { state.matrix.mvp };\
	PARAM	TexelOffset0	= program.env[0];\
	PARAM	TexelOffset1	= program.env[1];\
	PARAM	TexelOffset2	= program.env[2];\
	PARAM	TexelOffset3	= program.env[3];\
	\
	# Main.\n\
	DP4		oPos.x, ModelViewProj[0], iPos;\
	DP4		oPos.y, ModelViewProj[1], iPos;\
	DP4		oPos.z, ModelViewProj[2], iPos;\
	DP4		oPos.w, ModelViewProj[3], iPos;\
	MOV		oColor, iColor;\
	# Notice the optimization of using one texture coord instead of all four.\n\
	ADD		oTex0, iTex0, TexelOffset0;\
	ADD		oTex1, iTex0, TexelOffset1;\
	ADD		oTex2, iTex0, TexelOffset2;\
	ADD		oTex3, iTex0, TexelOffset3;\
	\
	END"
};

// This Pixel Shader loads four texture units and adds them all together (with a modifier
// multiplied to each in the process). The final output is r0 = t0 + t1 + t2 + t3.
const unsigned char g_strGlowPShaderARB[] =
{
	"!!ARBfp1.0\
	\
	# Input.\n\
	ATTRIB	iColor	= fragment.color.primary;\
	\
	# Output.\n\
	OUTPUT	oColor	= result.color;\
	\
	# Constants.\n\
	PARAM	Weight	= program.env[0];\
	TEMP	t0;\
	TEMP	t1;\
	TEMP	t2;\
	TEMP	t3;\
	TEMP	r0;\
	\
	# Main.\n\
	TEX		t0, fragment.texcoord[0], texture[0], RECT;\
	TEX		t1, fragment.texcoord[1], texture[1], RECT;\
	TEX		t2, fragment.texcoord[2], texture[2], RECT;\
	TEX		t3, fragment.texcoord[3], texture[3], RECT;\
	\
    MUL		r0, t0, Weight;\
	MAD		r0, t1, Weight, r0;\
	MAD		r0, t2, Weight, r0;\
	MAD		r0, t3, Weight, r0;\
	\
	MOV		oColor, r0;\
	\
	END"
};
/***********************************************************************************************************/

#ifdef USE_NEW_SHADER_HASH

static	char	shader_token[MAX_TOKEN_CHARS];
static	char	shader_parsename[MAX_TOKEN_CHARS];
static	int		shader_lines;

static char *Shader_ParseExt( const char **data_p, qboolean allowLineBreaks );

void Shader_BeginParseSession( const char *name )
{
	shader_lines = 0;
	Com_sprintf(shader_parsename, sizeof(shader_parsename), "%s", name);
}

int Shader_GetCurrentParseLine( void )
{
	return shader_lines;
}

static void Shader_ParseWarning( char *format, ... )
{
	va_list argptr;
	static char string[4096];

	va_start (argptr, format);
	Q_vsnprintf (string, sizeof(string), format, argptr);
	va_end (argptr);

	ri->Printf( PRINT_ALL, "WARNING: %s, line %d: %s\n", shader_parsename, shader_lines, string);
}

static int Shader_CompressBracedSection( char **data_p, char **name, char **text, int *nameLength, int *textLength ) { // drakkar - optimized sub-parse function
	char *in, *out;
	int depth, c;

	if( !*data_p ) return -1;

	*name = NULL;
	*text = NULL;

	*nameLength = 0;
	*textLength = 0;

	depth = 0;
	in = out = *data_p;

	if( !*in ) return 0;

	while( (c = *(unsigned char* /*eurofix*/)in++) != '\0' )
	{
		if( c <= '/' || c >= '{' )	// skip lot of conditions if c is regular char
		{
			//  whitespace or newline
			if( c <= ' ' )
			{
				if( out > *data_p && out[-1] <= ' ' ) {
					out--;
					*out = ( c == '\n' ? '\n' : *out );
				}
				else {
					*out = ( c == '\n' ? '\n' : ' ' );
				}
				while( *in && *(unsigned char* /*eurofix*/)in <= ' ' ) {
					if( *in++ == '\n' ) {
						shader_lines++;
						*out = '\n';
					}
				}
				out++;
				continue;
			}

			// skip comments
			if( c == '/' ) {
				// double slash comments
				if( *in == '/' ) {
					in++;
					while( *in && *in != '\n' ) in++;  // ignore until newline
					if( out > *data_p && out[-1] <= ' ' ) out--;
					if( *in ) in++;
					shader_lines++;
					*out++ = '\n';
				}
				// multiline /* */ comments
				else if( *in == '*' ) {
					in++;
					while( *in && ( *in != '*' || in[1] != '/' ) ) {  // ignore until comment close
						if( *in++ == '\n' ) {
							shader_lines++;
						}
					}
					if( *in ) in += 2;
				}
				// not comment
				else {
					*out++ = '/';
				}
				continue;
			}

			// handle quoted strings
			if( c == '"' )
			{
				*out++ = '"';
				while( *in && *in != '"' ) *out++ = *in++;
				*out++ = '"';
				in++;
				continue;
			}

			// brace matching
			if( c == '{' || c == '}' ) {
				if( c == '{' && !*name ) {
					*name = *data_p;
					if( *(*name) <= ' ' ) (*name)++;
					*nameLength = out - *name;
					if( (*name)[*nameLength-1] <= ' ' ) (*nameLength)--;
					*text = out;
				}
				if( out > *data_p && out[-1] > ' ' && out+1 < in ) *out++ = ' ';
				*out++ = c;
				if( out+1 < in ) *out++ = ' ';
				depth += ( c == '{' ? +1 : -1 );
				if( depth <= 0 ) break;
				continue;
			}
		}

		// parse a regular word
		while( c ) {
			*out++ = c;
			c = *in;
			// end of regular chars ?
			if( c <= '/' ) break;
			if( c >= '{' ) break;
			in++;
		}
	}

	if( depth ) {
		Shader_ParseWarning( "Unmatched braces in shader text" );
	}

	if( !c ) in--;

	if( *text && *(*text) <= ' ' ) (*text)++;			// remove begining white char
	if( out > *data_p && out[-1] <= ' ' ) out--;		// remove ending white char
	if( *text ) *textLength = out - *text;	// compressed text length

	c = out - *data_p;						// uncompressed chars parsed

	*data_p = in;

	return c;
}

static char *Shader_ParseExt( const char **data_p, qboolean allowLineBreaks ) // drakkar - new Shader_ParseExt(), optimized version
{
	const char *in;
	char *out;

	shader_token[0] = 0;

	if( !*data_p ) return shader_token;

	in = *data_p;
	out = shader_token;

	if( *in && *in <= '/' ) // skip lot of conditions if *in is regular char
	{
		// ignore while whitespace or newline
		while( *in && *in <= ' ' ) {
			if( *in++ == '\n') {
				shader_lines++;
				if( !allowLineBreaks ) {
					*data_p = in;
					return shader_token;
				}
			}
		}

		// skip comments
		while( *in == '/' ) {
			in++;
			if( *in == '/' ) {
				in++;
				while( *in && *in != '\n' ) in++;  // ignore until newline
				if( *in ) in++;
			}
			else if( *in == '*' ) {
				in++;
				while( *in && ( *in != '*' || in[1] != '/' ) ) in++;  // ignore until comment close
				if( *in ) in += 2;
			}
			else {
				*out++ = '/';
				break;
			}
			while( *in && *in <= ' ' ) {
				if( *in++ == '\n') {
					shader_lines++;
					if( !allowLineBreaks ) {
						*data_p = in;
						return shader_token;
					}
				}
			}
		}

		// handle quoted strings
		if( *in == '"' ) {
			in++;
			while( *in && *in != '"' ) {
				if( (out-shader_token) >= MAX_TOKEN_CHARS-2 ) {
					Shader_ParseWarning( "Token exceeded %d chars, truncated.", MAX_TOKEN_CHARS-2 );
					break;
				}
				*out++ = *in++;
			}
			if( *in ) in++;
			*out = '\0';
			*data_p = in;
			return shader_token;
		}
	}

	// parse a regular word
	while( *in > ' ' ) {
		if( (out-shader_token) >= MAX_TOKEN_CHARS-1 ) {
			Shader_ParseWarning( "Token exceeded %d chars, truncated.", MAX_TOKEN_CHARS-2 );
			break;
		}
		*out++ = *in++;
	}
	*out = '\0';
	*data_p = ( *in ? in : NULL );	// next text point or NULL if end of text reached
	return shader_token;
}

/*
=================
Shader_SkipRestOfLine
=================
*/
static void Shader_SkipRestOfLine ( const char **data ) {
	const char	*p;
	int		c;

	p = *data;
	while ( (c = *p++) != 0 ) {
		if ( c == '\n' ) {
			shader_lines++;
			break;
		}
	}

	*data = p;
}

#else

#define Shader_BeginParseSession COM_BeginParseSession
#define Shader_GetCurrentParseLine COM_GetCurrentParseLine
#define Shader_Parse COM_Parse
#define Shader_Shader_SkipWhitespace SkipWhitespace
#define Shader_Compress COM_Comress
#define Shader_ParseExt COM_ParseExt
#define Shader_SkipBracedSection SkipBracedSection
#define Shader_SkipRestOfLine SkipRestOfLine

#endif


#ifndef USE_NEW_SHADER_HASH
static char *s_shaderText;
#endif

// the shader is parsed into these global variables, then copied into
// dynamically allocated memory if it is valid.
static	shaderStage_t	stages[MAX_SHADER_STAGES];
static	shader_t		shader;
static	texModInfo_t	texMods[MAX_SHADER_STAGES][TR_MAX_TEXMODS];
static	qboolean		deferLoad;

#define FILE_HASH_SIZE		1024
static	shader_t*		hashTable[FILE_HASH_SIZE];

#ifdef USE_NEW_SHADER_HASH
// drakkar - dynamic shaderTextHashTable
#define MAX_SHADERNAME_LENGTH	 127
#define MAX_SHADERTEXT_HASH		4096	// from 2048 to 4096

typedef struct shaderText_s {   // 8 bytes + strlen(text)+1
	struct shaderText_s *next;	// linked list hashtable
	char *name;					// shader name
	char text[1];				// shader text
} shaderText_t;

static shaderText_t *shaderTextHashTable[MAX_SHADERTEXT_HASH];

static int fileShaderCount;		// total .shader files found
static int shaderCount;			// total shaders parsed
// !drakkar
#else
#define MAX_SHADERTEXT_HASH		2048
static char **shaderTextHashTable[MAX_SHADERTEXT_HASH] = { 0 };
#endif

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

/*
Ghoul2 Insert Start
*/

/*
===============
R_CreateExtendedName

  Creates a unique shader name taking into account lightstyles
===============
*/
//rwwRMG - added
void R_CreateExtendedName(char *extendedName, int extendedNameSize, const char *name, const int *lightmapIndex, const byte *styles)
{
	int		i;

	// Set the basename
	COM_StripExtension( name, extendedName, extendedNameSize );

	// Add in lightmaps
	if(lightmapIndex && styles)
	{
		if(lightmapIndex == lightmapsNone)
		{
			strcat(extendedName, "_nolightmap");
		}
		else if(lightmapIndex == lightmaps2d)
		{
			strcat(extendedName, "_2d");
		}
		else if(lightmapIndex == lightmapsVertex)
		{
			strcat(extendedName, "_vertex");
		}
		else if(lightmapIndex == lightmapsFullBright)
		{
			strcat(extendedName, "_fullbright");
		}
		else
		{
			for(i = 0; (i < 4) && (styles[i] != 255); i++)
			{
				switch(lightmapIndex[i])
				{
				case LIGHTMAP_NONE:
					strcat(extendedName, va("_style(%d,none)", styles[i]));
					break;
				case LIGHTMAP_2D:
					strcat(extendedName, va("_style(%d,2d)", styles[i]));
					break;
				case LIGHTMAP_BY_VERTEX:
					strcat(extendedName, va("_style(%d,vert)", styles[i]));
					break;
				case LIGHTMAP_WHITEIMAGE:
					strcat(extendedName, va("_style(%d,fb)", styles[i]));
					break;
				default:
					strcat(extendedName, va("_style(%d,%d)", styles[i], lightmapIndex[i]));
					break;
				}
			}
		}
	}
}

/*
Ghoul2 Insert End
*/

static void ClearGlobalShader(void)
{
	int	i;

	memset( &shader, 0, sizeof( shader ) );
	memset( &stages, 0, sizeof( stages ) );
	for ( i = 0 ; i < MAX_SHADER_STAGES ; i++ ) {
		stages[i].bundle[0].texMods = texMods[i];
		stages[i].mGLFogColorOverride = GLFOGOVERRIDE_NONE;
	}

	shader.contentFlags = CONTENTS_SOLID | CONTENTS_OPAQUE;
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
		h = RE_RegisterShaderLightMap(shaderName, lightmapsNone, stylesDefault);
		sh = R_GetShaderByHandle(h);
	}
	if (sh == NULL || sh == tr.defaultShader) {
		ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: R_RemapShader: shader %s not found\n", shaderName );
		return;
	}

	sh2 = R_FindShaderByName( newShaderName );
	if (sh2 == NULL || sh2 == tr.defaultShader) {
		h = RE_RegisterShaderLightMap(newShaderName, lightmapsNone, stylesDefault);
		sh2 = R_GetShaderByHandle(h);
	}

	if (sh2 == NULL || sh2 == tr.defaultShader) {
		ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: R_RemapShader: new shader %s not found\n", newShaderName );
		return;
	}

	// remap all the shaders with the given name
	// even tho they might have different lightmaps
	COM_StripExtension( shaderName, strippedName, sizeof( strippedName ) );
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
qboolean ParseVector( const char **text, int count, float *v ) {
	char	*token;
	int		i;

	// FIXME: spaces are currently required after parens, should change parseext...
	token = Shader_ParseExt( text, qfalse );
	if ( strcmp( token, "(" ) ) {
		ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing parenthesis in shader '%s'\n", shader.name );
		return qfalse;
	}

	for ( i = 0 ; i < count ; i++ ) {
		token = Shader_ParseExt( text, qfalse );
		if ( !token[0] ) {
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing vector element in shader '%s'\n", shader.name );
			return qfalse;
		}
		v[i] = atof( token );
	}

	token = Shader_ParseExt( text, qfalse );
	if ( strcmp( token, ")" ) ) {
		ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing parenthesis in shader '%s'\n", shader.name );
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

	ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid alphaFunc name '%s' in shader '%s'\n", funcname, shader.name );
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

	ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: unknown blend mode '%s' in shader '%s', substituting GL_ONE\n", name, shader.name );
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

	ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: unknown blend mode '%s' in shader '%s', substituting GL_ONE\n", name, shader.name );
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

	ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid genfunc name '%s' in shader '%s'\n", funcname, shader.name );
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

	token = Shader_ParseExt( text, qfalse );
	if ( token[0] == 0 )
	{
		ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->func = NameToGenFunc( token );

	// BASE, AMP, PHASE, FREQ
	token = Shader_ParseExt( text, qfalse );
	if ( token[0] == 0 )
	{
		ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->base = atof( token );

	token = Shader_ParseExt( text, qfalse );
	if ( token[0] == 0 )
	{
		ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->amplitude = atof( token );

	token = Shader_ParseExt( text, qfalse );
	if ( token[0] == 0 )
	{
		ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->phase = atof( token );

	token = Shader_ParseExt( text, qfalse );
	if ( token[0] == 0 )
	{
		ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing waveform parm in shader '%s'\n", shader.name );
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
		Com_Error( ERR_DROP, "ERROR: too many tcMod stages in shader '%s'\n", shader.name );
		return;
	}

	tmi = &stage->bundle[0].texMods[stage->bundle[0].numTexMods];
	stage->bundle[0].numTexMods++;

	token = Shader_ParseExt( text, qfalse );

	//
	// turb
	//
	if ( !Q_stricmp( token, "turb" ) )
	{
		token = Shader_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing tcMod turb parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.base = atof( token );
		token = Shader_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing tcMod turb in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.amplitude = atof( token );
		token = Shader_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing tcMod turb in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.phase = atof( token );
		token = Shader_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing tcMod turb in shader '%s'\n", shader.name );
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
		token = Shader_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing scale parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->translate[0] = atof( token );	//scale unioned

		token = Shader_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing scale parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->translate[1] = atof( token );	//scale unioned
		tmi->type = TMOD_SCALE;
	}
	//
	// scroll
	//
	else if ( !Q_stricmp( token, "scroll" ) )
	{
		token = Shader_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing scale scroll parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->translate[0] = atof( token );	//scroll unioned
		token = Shader_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing scale scroll parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->translate[1] = atof( token );	//scroll unioned
		tmi->type = TMOD_SCROLL;
	}
	//
	// stretch
	//
	else if ( !Q_stricmp( token, "stretch" ) )
	{
		token = Shader_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.func = NameToGenFunc( token );

		token = Shader_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.base = atof( token );

		token = Shader_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.amplitude = atof( token );

		token = Shader_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.phase = atof( token );

		token = Shader_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing stretch parms in shader '%s'\n", shader.name );
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
		token = Shader_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->matrix[0][0] = atof( token );

		token = Shader_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->matrix[0][1] = atof( token );

		token = Shader_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->matrix[1][0] = atof( token );

		token = Shader_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->matrix[1][1] = atof( token );

		token = Shader_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->translate[0] = atof( token );

		token = Shader_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing transform parms in shader '%s'\n", shader.name );
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
		token = Shader_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing tcMod rotate parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->translate[0]= atof( token );	//rotateSpeed unioned
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
		ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: unknown tcMod '%s' in shader '%s'\n", token, shader.name );
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
static void ParseSurfaceSprites( const char *_text, shaderStage_t *stage )
{
	const char *token;
	const char **text = &_text;
	float width, height, density, fadedist;
	int sstype=SURFSPRITE_NONE;

	//
	// spritetype
	//
	token = Shader_ParseExt( text, qfalse );

	if (token[0]==0)
	{
		ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfaceSprites params in shader '%s'\n", shader.name );
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
		ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid type in shader '%s'\n", shader.name );
		return;
	}

	//
	// width
	//
	token = Shader_ParseExt( text, qfalse );
	if (token[0]==0)
	{
		ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfaceSprites params in shader '%s'\n", shader.name );
		return;
	}
	width=atof(token);
	if (width <= 0)
	{
		ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid width in shader '%s'\n", shader.name );
		return;
	}

	//
	// height
	//
	token = Shader_ParseExt( text, qfalse );
	if (token[0]==0)
	{
		ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfaceSprites params in shader '%s'\n", shader.name );
		return;
	}
	height=atof(token);
	if (height <= 0)
	{
		ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid height in shader '%s'\n", shader.name );
		return;
	}

	//
	// density
	//
	token = Shader_ParseExt( text, qfalse );
	if (token[0]==0)
	{
		ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfaceSprites params in shader '%s'\n", shader.name );
		return;
	}
	density=atof(token);
	if (density <= 0)
	{
		ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid density in shader '%s'\n", shader.name );
		return;
	}

	//
	// fadedist
	//
	token = Shader_ParseExt( text, qfalse );
	if (token[0]==0)
	{
		ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfaceSprites params in shader '%s'\n", shader.name );
		return;
	}
	fadedist=atof(token);
	if (fadedist < 32)
	{
		ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid fadedist (%f < 32) in shader '%s'\n", fadedist, shader.name );
		return;
	}

	if (!stage->ss)
	{
		stage->ss = (surfaceSprite_t *)Hunk_Alloc( sizeof( surfaceSprite_t ), h_low );
	}

	// These are all set by the command lines.
	stage->ss->surfaceSpriteType = sstype;
	stage->ss->width = width;
	stage->ss->height = height;
	stage->ss->density = density;
	stage->ss->fadeDist = fadedist;

	// These are defaults that can be overwritten.
	stage->ss->fadeMax = fadedist*1.33;
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

	if (!stage->ss)
	{
		stage->ss = (surfaceSprite_t *)Hunk_Alloc( sizeof( surfaceSprite_t ), h_low );
	}
	//
	// fademax
	//
	if (!Q_stricmp(param, "ssFademax"))
	{
		token = Shader_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfacesprite fademax in shader '%s'\n", shader.name );
			return;
		}
		value = atof(token);
		if (value <= stage->ss->fadeDist)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid surfacesprite fademax (%.2f <= fadeDist(%.2f)) in shader '%s'\n", value, stage->ss->fadeDist, shader.name );
			return;
		}
		stage->ss->fadeMax=value;
		return;
	}

	//
	// fadescale
	//
	if (!Q_stricmp(param, "ssFadescale"))
	{
		token = Shader_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfacesprite fadescale in shader '%s'\n", shader.name );
			return;
		}
		value = atof(token);
		stage->ss->fadeScale=value;
		return;
	}

	//
	// variance
	//
	if (!Q_stricmp(param, "ssVariance"))
	{
		token = Shader_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfacesprite variance width in shader '%s'\n", shader.name );
			return;
		}
		value = atof(token);
		if (value < 0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid surfacesprite variance width in shader '%s'\n", shader.name );
			return;
		}
		stage->ss->variance[0]=value;

		token = Shader_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfacesprite variance height in shader '%s'\n", shader.name );
			return;
		}
		value = atof(token);
		if (value < 0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid surfacesprite variance height in shader '%s'\n", shader.name );
			return;
		}
		stage->ss->variance[1]=value;
		return;
	}

	//
	// hangdown
	//
	if (!Q_stricmp(param, "ssHangdown"))
	{
		if (stage->ss->facing != SURFSPRITE_FACING_NORMAL)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: Hangdown facing overrides previous facing in shader '%s'\n", shader.name );
			return;
		}
		stage->ss->facing=SURFSPRITE_FACING_DOWN;
		return;
	}

	//
	// anyangle
	//
	if (!Q_stricmp(param, "ssAnyangle"))
	{
		if (stage->ss->facing != SURFSPRITE_FACING_NORMAL)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: Anyangle facing overrides previous facing in shader '%s'\n", shader.name );
			return;
		}
		stage->ss->facing=SURFSPRITE_FACING_ANY;
		return;
	}

	//
	// faceup
	//
	if (!Q_stricmp(param, "ssFaceup"))
	{
		if (stage->ss->facing != SURFSPRITE_FACING_NORMAL)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: Faceup facing overrides previous facing in shader '%s'\n", shader.name );
			return;
		}
		stage->ss->facing=SURFSPRITE_FACING_UP;
		return;
	}

	//
	// wind
	//
	if (!Q_stricmp(param, "ssWind"))
	{
		token = Shader_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfacesprite wind in shader '%s'\n", shader.name );
			return;
		}
		value = atof(token);
		if (value < 0.0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid surfacesprite wind in shader '%s'\n", shader.name );
			return;
		}
		stage->ss->wind=value;
		if (stage->ss->windIdle <= 0)
		{	// Also override the windidle, it usually is the same as wind
			stage->ss->windIdle = value;
		}
		return;
	}

	//
	// windidle
	//
	if (!Q_stricmp(param, "ssWindidle"))
	{
		token = Shader_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfacesprite windidle in shader '%s'\n", shader.name );
			return;
		}
		value = atof(token);
		if (value < 0.0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid surfacesprite windidle in shader '%s'\n", shader.name );
			return;
		}
		stage->ss->windIdle=value;
		return;
	}

	//
	// vertskew
	//
	if (!Q_stricmp(param, "ssVertskew"))
	{
		token = Shader_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfacesprite vertskew in shader '%s'\n", shader.name );
			return;
		}
		value = atof(token);
		if (value < 0.0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid surfacesprite vertskew in shader '%s'\n", shader.name );
			return;
		}
		stage->ss->vertSkew=value;
		return;
	}

	//
	// fxduration
	//
	if (!Q_stricmp(param, "ssFXDuration"))
	{
		token = Shader_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfacesprite duration in shader '%s'\n", shader.name );
			return;
		}
		value = atof(token);
		if (value <= 0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid surfacesprite duration in shader '%s'\n", shader.name );
			return;
		}
		stage->ss->fxDuration=value;
		return;
	}

	//
	// fxgrow
	//
	if (!Q_stricmp(param, "ssFXGrow"))
	{
		token = Shader_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfacesprite grow width in shader '%s'\n", shader.name );
			return;
		}
		value = atof(token);
		if (value < 0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid surfacesprite grow width in shader '%s'\n", shader.name );
			return;
		}
		stage->ss->fxGrow[0]=value;

		token = Shader_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfacesprite grow height in shader '%s'\n", shader.name );
			return;
		}
		value = atof(token);
		if (value < 0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid surfacesprite grow height in shader '%s'\n", shader.name );
			return;
		}
		stage->ss->fxGrow[1]=value;
		return;
	}

	//
	// fxalpharange
	//
	if (!Q_stricmp(param, "ssFXAlphaRange"))
	{
		token = Shader_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfacesprite fxalpha start in shader '%s'\n", shader.name );
			return;
		}
		value = atof(token);
		if (value < 0 || value > 1.0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid surfacesprite fxalpha start in shader '%s'\n", shader.name );
			return;
		}
		stage->ss->fxAlphaStart=value;

		token = Shader_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfacesprite fxalpha end in shader '%s'\n", shader.name );
			return;
		}
		value = atof(token);
		if (value < 0 || value > 1.0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid surfacesprite fxalpha end in shader '%s'\n", shader.name );
			return;
		}
		stage->ss->fxAlphaEnd=value;
		return;
	}

	//
	// fxweather
	//
	if (!Q_stricmp(param, "ssFXWeather"))
	{
		if (stage->ss->surfaceSpriteType != SURFSPRITE_EFFECT)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: weather applied to non-effect surfacesprite in shader '%s'\n", shader.name );
			return;
		}
		stage->ss->surfaceSpriteType = SURFSPRITE_WEATHERFX;
		return;
	}

	//
	// invalid ss command.
	//
	ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid optional surfacesprite param '%s' in shader '%s'\n", param, shader.name );
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
		token = Shader_ParseExt( text, qtrue );
		if ( !token[0] )
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: no matching '}' found\n" );
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
			token = Shader_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing parameter for 'map' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			if ( !Q_stricmp( token, "$whiteimage" ) )
			{
				stage->bundle[0].image = tr.whiteImage;
				continue;
			}
			else if ( !Q_stricmp( token, "$lightmap" ) )
			{
				stage->bundle[0].isLightmap = qtrue;
				if ( shader.lightmapIndex[0] < 0 || shader.lightmapIndex[0] >= tr.numLightmaps )
				{
#ifndef FINAL_BUILD
					ri->Printf( PRINT_ALL, S_COLOR_RED"Lightmap requested but none available for shader %s\n", shader.name);
#endif
					stage->bundle[0].image = tr.whiteImage;
				}
				else
				{
					stage->bundle[0].image = tr.lightmaps[shader.lightmapIndex[0]];
				}
				continue;
			}
			else
			{
				stage->bundle[0].image = R_FindImageFile( token, (qboolean)!shader.noMipMaps, (qboolean)!shader.noPicMip, (qboolean)!shader.noTC, GL_REPEAT );
				if ( !stage->bundle[0].image )
				{
					ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
					return qfalse;
				}
			}
		}
		//
		// clampmap <name>
		//
		else if ( !Q_stricmp( token, "clampmap" ) )
		{
			token = Shader_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing parameter for 'clampmap' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}
			stage->bundle[0].image = R_FindImageFile( token, (qboolean)!shader.noMipMaps, (qboolean)!shader.noPicMip, (qboolean)!shader.noTC, GL_CLAMP );
			if ( !stage->bundle[0].image )
			{
				ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
				return qfalse;
			}
		}
		//
		// animMap <frequency> <image1> .... <imageN>
		//
		else if ( !Q_stricmp( token, "animMap" ) || !Q_stricmp( token, "clampanimMap" ) || !Q_stricmp( token, "oneshotanimMap" ))
		{
			#define	MAX_IMAGE_ANIMATIONS	32
			image_t *images[MAX_IMAGE_ANIMATIONS];
			bool bClamp = !Q_stricmp( token, "clampanimMap" );
			bool oneShot = !Q_stricmp( token, "oneshotanimMap" );

			token = Shader_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing parameter for '%s' keyword in shader '%s'\n", (bClamp ? "animMap":"clampanimMap"), shader.name );
				return qfalse;
			}
			stage->bundle[0].imageAnimationSpeed = atof( token );
			stage->bundle[0].oneShotAnimMap = oneShot;

			// parse up to MAX_IMAGE_ANIMATIONS animations
			while ( 1 ) {
				int		num;

				token = Shader_ParseExt( text, qfalse );
				if ( !token[0] ) {
					break;
				}
				num = stage->bundle[0].numImageAnimations;
				if ( num < MAX_IMAGE_ANIMATIONS ) {
					images[num] = R_FindImageFile( token, (qboolean)!shader.noMipMaps, (qboolean)!shader.noPicMip, (qboolean)!shader.noTC, bClamp?GL_CLAMP:GL_REPEAT );
					if ( !images[num] )
					{
						ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
						return qfalse;
					}
					stage->bundle[0].numImageAnimations++;
				}
			}
			// Copy image ptrs into an array of ptrs
			stage->bundle[0].image = (image_t*) Hunk_Alloc( stage->bundle[0].numImageAnimations * sizeof( image_t* ), h_low );
			memcpy( stage->bundle[0].image,	images,			stage->bundle[0].numImageAnimations * sizeof( image_t* ) );
		}
		else if ( !Q_stricmp( token, "videoMap" ) )
		{
			token = Shader_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing parameter for 'videoMap' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}
			stage->bundle[0].videoMapHandle = ri->CIN_PlayCinematic( token, 0, 0, 256, 256, (CIN_loop | CIN_silent | CIN_shader));
			if (stage->bundle[0].videoMapHandle != -1) {
				stage->bundle[0].isVideoMap = qtrue;
				assert (stage->bundle[0].videoMapHandle<NUM_SCRATCH_IMAGES);
				stage->bundle[0].image = tr.scratchImage[stage->bundle[0].videoMapHandle];
			}
		}

		//
		// alphafunc <func>
		//
		else if ( !Q_stricmp( token, "alphaFunc" ) )
		{
			token = Shader_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing parameter for 'alphaFunc' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			atestBits = NameToAFunc( token );
		}
		//
		// depthFunc <func>
		//
		else if ( !Q_stricmp( token, "depthfunc" ) )
		{
			token = Shader_ParseExt( text, qfalse );

			if ( !token[0] )
			{
				ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing parameter for 'depthfunc' keyword in shader '%s'\n", shader.name );
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
				ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: unknown depthfunc '%s' in shader '%s'\n", token, shader.name );
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
			token = Shader_ParseExt( text, qfalse );
			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing parm for blendFunc in shader '%s'\n", shader.name );
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

				token = Shader_ParseExt( text, qfalse );
				if ( token[0] == 0 )
				{
					ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing parm for blendFunc in shader '%s'\n", shader.name );
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
			token = Shader_ParseExt( text, qfalse );
			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing parameters for rgbGen in shader '%s'\n", shader.name );
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
			else if ( !Q_stricmp( token, "lightingDiffuseEntity" ) )
			{
				if (shader.lightmapIndex[0] != LIGHTMAP_NONE)
				{
					ri->Printf( PRINT_ALL, S_COLOR_RED "ERROR: rgbGen lightingDiffuseEntity used on a misc_model! in shader '%s'\n", shader.name );
				}
				stage->rgbGen = CGEN_LIGHTING_DIFFUSE_ENTITY;

			}
			else if ( !Q_stricmp( token, "oneMinusVertex" ) )
			{
				stage->rgbGen = CGEN_ONE_MINUS_VERTEX;
			}
			else
			{
				ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: unknown rgbGen parameter '%s' in shader '%s'\n", token, shader.name );
				continue;
			}
		}
		//
		// alphaGen
		//
		else if ( !Q_stricmp( token, "alphaGen" ) )
		{
			token = Shader_ParseExt( text, qfalse );
			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing parameters for alphaGen in shader '%s'\n", shader.name );
				continue;
			}

			if ( !Q_stricmp( token, "wave" ) )
			{
				ParseWaveForm( text, &stage->alphaWave );
				stage->alphaGen = AGEN_WAVEFORM;
			}
			else if ( !Q_stricmp( token, "const" ) )
			{
				token = Shader_ParseExt( text, qfalse );
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
			else if ( !Q_stricmp( token, "dot" ) )
			{
				stage->alphaGen = AGEN_DOT;
			}
			else if ( !Q_stricmp( token, "oneMinusDot" ) )
			{
				stage->alphaGen = AGEN_ONE_MINUS_DOT;
			}
			else if ( !Q_stricmp( token, "portal" ) )
			{
				stage->alphaGen = AGEN_PORTAL;
				token = Shader_ParseExt( text, qfalse );
				if ( token[0] == 0 )
				{
					shader.portalRange = 256;
					ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing range parameter for alphaGen portal in shader '%s', defaulting to 256\n", shader.name );
				}
				else
				{
					shader.portalRange = atof( token );
				}
			}
			else
			{
				ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: unknown alphaGen parameter '%s' in shader '%s'\n", token, shader.name );
				continue;
			}
		}
		//
		// tcGen <function>
		//
		else if ( !Q_stricmp(token, "texgen") || !Q_stricmp( token, "tcGen" ) )
		{
			token = Shader_ParseExt( text, qfalse );
			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing texgen parm in shader '%s'\n", shader.name );
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
				stage->bundle[0].tcGenVectors = ( vec3_t *) Hunk_Alloc( 2 * sizeof( vec3_t ), h_low );
				ParseVector( text, 3, stage->bundle[0].tcGenVectors[0] );
				ParseVector( text, 3, stage->bundle[0].tcGenVectors[1] );

				stage->bundle[0].tcGen = TCGEN_VECTOR;
			}
			else
			{
				ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: unknown texgen parm in shader '%s'\n", shader.name );
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
				token = Shader_ParseExt( text, qfalse );
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
		// If this stage has glow...	GLOWXXX
		else if ( Q_stricmp( token, "glow" ) == 0 )
		{
			stage->glow = true;

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
				token = Shader_ParseExt( text, qfalse );
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
				token = Shader_ParseExt( text, qfalse );
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
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: unknown parameter '%s' in shader '%s'\n", token, shader.name );
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

	token = Shader_ParseExt( text, qfalse );
	if ( token[0] == 0 )
	{
		ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing deform parm in shader '%s'\n", shader.name );
		return;
	}

	if ( shader.numDeforms == MAX_SHADER_DEFORMS ) {
		ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: MAX_SHADER_DEFORMS in '%s'\n", shader.name );
		return;
	}

	shader.deforms[ shader.numDeforms ] = (deformStage_t *)Hunk_Alloc( sizeof( deformStage_t ), h_low );

	ds = shader.deforms[ shader.numDeforms ];
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
		token = Shader_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name );
			return;
		}
		ds->bulgeWidth = atof( token );

		token = Shader_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name );
			return;
		}
		ds->bulgeHeight = atof( token );

		token = Shader_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name );
			return;
		}
		ds->bulgeSpeed = atof( token );

		ds->deformation = DEFORM_BULGE;
		return;
	}

	if ( !Q_stricmp( token, "wave" ) )
	{
		token = Shader_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name );
			return;
		}

		if ( atof( token ) != 0 )
		{
			ds->deformationSpread = 1.0f / atof( token );
		}
		else
		{
			ds->deformationSpread = 100.0f;
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: illegal div value of 0 in deformVertexes command for shader '%s'\n", shader.name );
		}

		ParseWaveForm( text, &ds->deformationWave );
		ds->deformation = DEFORM_WAVE;
		return;
	}

	if ( !Q_stricmp( token, "normal" ) )
	{
		token = Shader_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name );
			return;
		}
		ds->deformationWave.amplitude = atof( token );

		token = Shader_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name );
			return;
		}
		ds->deformationWave.frequency = atof( token );

		ds->deformation = DEFORM_NORMALS;
		return;
	}

	if ( !Q_stricmp( token, "move" ) ) {
		int		i;

		for ( i = 0 ; i < 3 ; i++ ) {
			token = Shader_ParseExt( text, qfalse );
			if ( token[0] == 0 ) {
				ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name );
				return;
			}
			ds->moveVector[i] = atof( token );
		}

		ParseWaveForm( text, &ds->deformationWave );
		ds->deformation = DEFORM_MOVE;
		return;
	}

	ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: unknown deformVertexes subtype '%s' found in shader '%s'\n", token, shader.name );
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

	shader.sky = (skyParms_t *)Hunk_Alloc( sizeof( skyParms_t ), h_low );

	// outerbox
	token = Shader_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name );
		return;
	}
	if ( strcmp( token, "-" ) ) {
		for (i=0 ; i<6 ; i++) {
			Com_sprintf( pathname, sizeof(pathname), "%s_%s", token, suf[i] );
			shader.sky->outerbox[i] = R_FindImageFile( ( char * ) pathname, qtrue, qtrue, (qboolean)!shader.noTC, GL_CLAMP );
			if ( !shader.sky->outerbox[i] ) {
				if (i) {
					shader.sky->outerbox[i] = shader.sky->outerbox[i-1];//not found, so let's use the previous image
				}else{
					shader.sky->outerbox[i] = tr.defaultImage;
				}
			}
		}
	}

	// cloudheight
	token = Shader_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: 'skyParms' missing cloudheight in shader '%s'\n", shader.name );
		return;
	}
	shader.sky->cloudHeight = atof( token );
	if ( !shader.sky->cloudHeight ) {
		shader.sky->cloudHeight = 512;
	}
	R_InitSkyTexCoords( shader.sky->cloudHeight );

	// innerbox
	token = Shader_ParseExt( text, qfalse );
	if ( strcmp( token, "-" ) ) {
		ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: in shader '%s' 'skyParms', innerbox is not supported!", shader.name);
	}
}


/*
=================
ParseSort
=================
*/
static void ParseSort( const char **text ) {
	char	*token;

	token = Shader_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing sort parameter in shader '%s'\n", shader.name );
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

	token = Shader_ParseExt( text, qfalse );
	if ( token[0] == 0 )
	{
		ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing material in shader '%s'\n", shader.name );
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
	int		numInfoParms = sizeof(infoParms) / sizeof(infoParms[0]);
	int		i;

	token = Shader_ParseExt( text, qfalse );
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
	int s;

	s = 0;

	token = Shader_ParseExt( text, qtrue );
	if ( token[0] != '{' )
	{
		ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: expecting '{', found '%s' instead in shader '%s'\n", token, shader.name );
		return qfalse;
	}

	while ( 1 )
	{
		token = Shader_ParseExt( text, qtrue );
		if ( !token[0] )
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: no concluding '}' in shader %s\n", shader.name );
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
				ri->Printf( PRINT_WARNING, "WARNING: too many stages in shader %s\n", shader.name );
				return qfalse;
			}

			if ( !ParseStage( &stages[s], text ) )
			{
				return qfalse;
			}
			stages[s].active = qtrue;
			if ( stages[s].glow )
			{
				shader.hasGlow = true;
			}
			s++;
			continue;
		}
		// skip stuff that only the QuakeEdRadient needs
		else if ( !Q_stricmpn( token, "qer", 3 ) ) {
			Shader_SkipRestOfLine( text );
			continue;
		}
		// material deprecated as of 11 Jan 01
		// material undeprecated as of 7 May 01 - q3map_material deprecated
		else if ( !Q_stricmp( token, "material" ) || !Q_stricmp( token, "q3map_material" ) )
		{
			ParseMaterial( text );
		}
		// sun parms
		else if ( !Q_stricmp( token, "sun" ) || !Q_stricmp( token, "q3map_sun" ) || !Q_stricmp( token, "q3map_sunExt" ) )
		{
			token = Shader_ParseExt( text, qfalse );
			tr.sunLight[0] = atof( token );
			token = Shader_ParseExt( text, qfalse );
			tr.sunLight[1] = atof( token );
			token = Shader_ParseExt( text, qfalse );
			tr.sunLight[2] = atof( token );

			VectorNormalize( tr.sunLight );

			token = Shader_ParseExt( text, qfalse );
			float a = atof( token );
			VectorScale( tr.sunLight, a, tr.sunLight);

			token = Shader_ParseExt( text, qfalse );
			a = atof( token );
			a = a / 180 * M_PI;

			token = Shader_ParseExt( text, qfalse );
			float b = atof( token );
			b = b / 180 * M_PI;

			tr.sunDirection[0] = cos( a ) * cos( b );
			tr.sunDirection[1] = sin( a ) * cos( b );
			tr.sunDirection[2] = sin( b );
		}
		// q3map_surfacelight deprecated as of 16 Jul 01
		else if ( !Q_stricmp( token, "surfacelight" ) || !Q_stricmp( token, "q3map_surfacelight" ) )
		{
			token = Shader_ParseExt( text, qfalse );
			tr.sunSurfaceLight = atoi( token );
		}
		else if ( !Q_stricmp( token, "lightColor" ) )
		{
			/*
			if ( !ParseVector( text, 3, tr.sunAmbient ) )
			{
				return qfalse;
			}
			*/
			//SP skips this so I'm skipping it here too.
			Shader_SkipRestOfLine( text );
			continue;
		}
		else if ( !Q_stricmp( token, "deformvertexes" ) || !Q_stricmp( token, "deform" )) {
			ParseDeform( text );
			continue;
		}
		else if ( !Q_stricmp( token, "tesssize" ) ) {
			Shader_SkipRestOfLine( text );
			continue;
		}
		else if ( !Q_stricmp( token, "clampTime" ) ) {
			token = Shader_ParseExt( text, qfalse );
			if (token[0]) {
				shader.clampTime = atof(token);
			}
		}
		// skip stuff that only the q3map needs
		else if ( !Q_stricmpn( token, "q3map", 5 ) ) {
			Shader_SkipRestOfLine( text );
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
			shader.noMipMaps = true;
			shader.noPicMip = true;
			continue;
		}
		// no picmip adjustment
		else if ( !Q_stricmp( token, "nopicmip" ) )
		{
			shader.noPicMip = true;
			continue;
		}
		else if ( !Q_stricmp( token, "noglfog" ) )
		{
			shader.fogPass = FP_NONE;
			continue;
		}
		// polygonOffset
		else if ( !Q_stricmp( token, "polygonOffset" ) )
		{
			shader.polygonOffset = true;
			continue;
		}
		else if ( !Q_stricmp( token, "noTC" ) )
		{
			shader.noTC = true;
			continue;
		}
		// entityMergable, allowing sprite surfaces from multiple entities
		// to be merged into one batch.  This is a savings for smoke
		// puffs and blood, but can't be used for anything where the
		// shader calcs (not the surface function) reference the entity color or scroll
		else if ( !Q_stricmp( token, "entityMergable" ) )
		{
			shader.entityMergable = true;
			continue;
		}
		// fogParms
		else if ( !Q_stricmp( token, "fogParms" ) )
		{
			shader.fogParms = (fogParms_t *)Hunk_Alloc( sizeof( fogParms_t ), h_low );
			if ( !ParseVector( text, 3, shader.fogParms->color ) ) {
				return qfalse;
			}

			token = Shader_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing parm for 'fogParms' keyword in shader '%s'\n", shader.name );
				continue;
			}
			shader.fogParms->depthForOpaque = atof( token );

			// skip any old gradient directions
			Shader_SkipRestOfLine( text );
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
			token = Shader_ParseExt( text, qfalse );
			continue;
		}
		// cull <face>
		else if ( !Q_stricmp( token, "cull") )
		{
			token = Shader_ParseExt( text, qfalse );
			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing cull parms in shader '%s'\n", shader.name );
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
				ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid cull parm '%s' in shader '%s'\n", token, shader.name );
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
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: unknown general shader parameter '%s' in '%s'\n", token, shader.name );
			return qfalse;
		}
	}

	//
	// ignore shaders that don't have any stages, unless it is a sky or fog
	//
	if ( s == 0 && !shader.sky && !(shader.contentFlags & CONTENTS_FOG ) ) {
		return qfalse;
	}

	shader.explicitlyDefined = true;

	return qtrue;
}

/*
========================================================================================

SHADER OPTIMIZATION AND FOGGING

========================================================================================
*/

typedef struct collapse_s {
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
	if ( ( stages[0].rgbGen != stages[1].rgbGen ) ||
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
	if ( stages[0].alphaGen == AGEN_WAVEFORM )
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
					{
					const rotatePicCommand_t *sp_cmd = (const rotatePicCommand_t *)curCmd;
					curCmd = (const void *)(sp_cmd + 1);
					break;
					}
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
					int			fogNum;
					int			entityNum;
					int			dlightMap;
					int			sortedIndex;
					const drawSurfsCommand_t *ds_cmd =  (const drawSurfsCommand_t *)curCmd;

					for( i = 0, drawSurf = ds_cmd->drawSurfs; i < ds_cmd->numDrawSurfs; i++, drawSurf++ ) {
						R_DecomposeSort( drawSurf->sort, &entityNum, &shader, &fogNum, &dlightMap );
						sortedIndex = (( drawSurf->sort >> QSORT_SHADERNUM_SHIFT ) & (MAX_SHADERS-1));
						if( sortedIndex >= newShader ) {
							sortedIndex++;
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

	for ( i = tr.numShaders - 2 ; i >= 0 ; i-- ) {
		if ( tr.sortedShaders[ i ]->sort <= sort ) {
			break;
		}
		tr.sortedShaders[i+1] = tr.sortedShaders[i];
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
	int			size;

	if ( tr.numShaders == MAX_SHADERS ) {
		//ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: GeneratePermanentShader - MAX_SHADERS hit\n");
		ri->Printf( PRINT_ALL, "WARNING: GeneratePermanentShader - MAX_SHADERS hit\n");
		return tr.defaultShader;
	}

	newShader = (struct shader_s *)ri->Hunk_Alloc( sizeof( shader_t ), h_low );

	*newShader = shader;

	if ( shader.sort <= /*SS_OPAQUE*/SS_SEE_THROUGH ) {
		newShader->fogPass = FP_EQUAL;
	} else if ( shader.contentFlags & CONTENTS_FOG ) {
		newShader->fogPass = FP_LE;
	}

	tr.shaders[ tr.numShaders ] = newShader;
	newShader->index = tr.numShaders;

	tr.sortedShaders[ tr.numShaders ] = newShader;
	newShader->sortedIndex = tr.numShaders;

	tr.numShaders++;

	size = newShader->numUnfoggedPasses ? newShader->numUnfoggedPasses * sizeof( stages[0] ) : sizeof( stages[0] );
	newShader->stages = (shaderStage_t *) Hunk_Alloc( size, h_low );

	for ( i = 0 ; i < newShader->numUnfoggedPasses ; i++ ) {
		if ( !stages[i].active ) {
			break;
		}
		newShader->stages[i] = stages[i];

		for ( b = 0 ; b < NUM_TEXTURE_BUNDLES ; b++ ) {
			if (newShader->stages[i].bundle[b].numTexMods)
			{
				size = newShader->stages[i].bundle[b].numTexMods * sizeof( texModInfo_t );
				newShader->stages[i].bundle[b].texMods = (texModInfo_t *)Hunk_Alloc( size, h_low );
				memcpy( newShader->stages[i].bundle[b].texMods, stages[i].bundle[b].texMods, size );
			}
			else
			{
				newShader->stages[i].bundle[b].texMods = 0;	//clear the globabl ptr jic
			}
		}
	}

	SortNewShader();

	const int hash = generateHashValue(newShader->name, FILE_HASH_SIZE);
	newShader->next = hashTable[hash];
	hashTable[hash] = newShader;

	return newShader;
}

/*
=================
VertexLightingCollapse

If vertex lighting is enabled, only render a single
pass, trying to guess which is the correct one to best approximate
what it is supposed to look like.

  OUTPUT:  Number of stages after the collapse (in the case of surfacesprites this isn't one).
=================
*/
//rww - no longer used, at least for now. destroys alpha shaders completely.
#if 0
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
			if ( pStage->ss && pstage->ss->surfaceSpriteType)
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

		if ( pStage->ss && pstage->ss->surfaceSpriteType)
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
#endif

/*
=========================
FinishShader

Returns a freshly allocated shader with all the needed info
from the current global working shader
=========================
*/
static shader_t *FinishShader( void ) {
	int				stage, lmStage, stageIndex; //rwwRMG - stageIndex for AGEN_BLEND
	qboolean		hasLightmapStage;
	qboolean		vertexLightmap;

	hasLightmapStage = qfalse;
	vertexLightmap = qfalse;

	//
	// set sky stuff appropriate
	//
	if ( shader.sky ) {
		shader.sort = SS_ENVIRONMENT;
	}

	//
	// set polygon offset
	//
	if ( shader.polygonOffset && !shader.sort ) {
		shader.sort = SS_DECAL;
	}

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
					stages[lmStage+i+1].bundle[0].image = tr.whiteImage;
				}
				else if (shader.lightmapIndex[i+1] < 0)
				{
					Com_Error( ERR_DROP, "FinishShader: light style with no light map or vertex color for shader %s", shader.name);
				}
				else
				{
					stages[lmStage+i+1].bundle[0].image = tr.lightmaps[shader.lightmapIndex[i+1]];
					stages[lmStage+i+1].bundle[0].tcGen = (texCoordGen_t)(TCGEN_LIGHTMAP+i+1);
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
	stageIndex = 0; //rwwRMG - needed for AGEN_BLEND
	for ( stage = 0; stage < MAX_SHADER_STAGES; ) {
		shaderStage_t *pStage = &stages[stage];

		if ( !pStage->active ) {
			break;
		}

    // check for a missing texture
		if ( !pStage->bundle[0].image ) {
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "Shader %s has a stage with no image\n", shader.name );
			pStage->active = qfalse;
			stage++;
			continue;
		}

		//
		// ditch this stage if it's detail and detail textures are disabled
		//
		if ( pStage->isDetail && !r_detailTextures->integer ) {
			int index;

			for ( index=stage+1; index<MAX_SHADER_STAGES; index++ ) {
				if ( !stages[index].active )
					break;
			}

			if ( index < MAX_SHADER_STAGES )
				memmove( pStage, pStage + 1, sizeof( *pStage ) * ( index - stage ) );
			else {
				if ( stage + 1 < MAX_SHADER_STAGES )
					memmove( pStage, pStage + 1, sizeof( *pStage ) * ( index - stage - 1 ) );

				Com_Memset( &stages[index - 1], 0, sizeof( *stages ) );
			}

			continue;
		}

		pStage->index = stageIndex; //rwwRMG - needed for AGEN_BLEND

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


    // not a true lightmap but we want to leave existing
    // behaviour in place and not print out a warning
    //if (pStage->rgbGen == CGEN_VERTEX) {
    //  vertexLightmap = qtrue;
    //}



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
					/*
					if (( blendSrcBits == GLS_SRCBLEND_ONE ) && ( blendDstBits == GLS_DSTBLEND_ONE ))
					{
						// GL_ONE GL_ONE needs to come a bit later
						shader.sort = SS_BLEND2;
					}
					else if (( blendSrcBits == GLS_SRCBLEND_SRC_ALPHA ) && ( blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA ))
					{ //rww - Pushed SS_BLEND1 up to SS_BLEND2, inserting this so that saber glow will render above water and things.
					  //Unfortunately it still affects other shaders with the same blend settings, but it seems more or less alright.
						shader.sort = SS_BLEND1;
					}
					else
					{
						shader.sort = SS_BLEND0;
					}
					*/
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

		//rww - begin hw fog
		if ((pStage->stateBits & (GLS_SRCBLEND_BITS|GLS_DSTBLEND_BITS)) == (GLS_SRCBLEND_ONE|GLS_DSTBLEND_ONE))
		{
			pStage->mGLFogColorOverride = GLFOGOVERRIDE_BLACK;
		}
		else if ((pStage->stateBits & (GLS_SRCBLEND_BITS|GLS_DSTBLEND_BITS)) == (GLS_SRCBLEND_SRC_ALPHA|GLS_DSTBLEND_ONE) &&
			pStage->alphaGen == AGEN_LIGHTING_SPECULAR && stage)
		{
			pStage->mGLFogColorOverride = GLFOGOVERRIDE_BLACK;
		}
		else if ((pStage->stateBits & (GLS_SRCBLEND_BITS|GLS_DSTBLEND_BITS)) == (GLS_SRCBLEND_ZERO|GLS_DSTBLEND_ZERO))
		{
			pStage->mGLFogColorOverride = GLFOGOVERRIDE_WHITE;
		}
		else if ((pStage->stateBits & (GLS_SRCBLEND_BITS|GLS_DSTBLEND_BITS)) == (GLS_SRCBLEND_ONE|GLS_DSTBLEND_ZERO))
		{
			pStage->mGLFogColorOverride = GLFOGOVERRIDE_WHITE;
		}
		else if ((pStage->stateBits & (GLS_SRCBLEND_BITS|GLS_DSTBLEND_BITS)) == 0 && stage)
		{	//
			pStage->mGLFogColorOverride = GLFOGOVERRIDE_WHITE;
		}
		else if ((pStage->stateBits & (GLS_SRCBLEND_BITS|GLS_DSTBLEND_BITS)) == 0 && pStage->bundle[0].isLightmap && stage < MAX_SHADER_STAGES-1 &&
			stages[stage+1].bundle[0].isLightmap)
		{	// multiple light map blending
			pStage->mGLFogColorOverride = GLFOGOVERRIDE_WHITE;
		}
		else if ((pStage->stateBits & (GLS_SRCBLEND_BITS|GLS_DSTBLEND_BITS)) == (GLS_SRCBLEND_DST_COLOR|GLS_DSTBLEND_ZERO) && pStage->bundle[0].isLightmap)
		{ //I don't know, it works. -rww
			pStage->mGLFogColorOverride = GLFOGOVERRIDE_WHITE;
		}
		else if ((pStage->stateBits & (GLS_SRCBLEND_BITS|GLS_DSTBLEND_BITS)) == (GLS_SRCBLEND_DST_COLOR|GLS_DSTBLEND_ZERO))
		{ //I don't know, it works. -rww
			pStage->mGLFogColorOverride = GLFOGOVERRIDE_BLACK;
		}
		else if ((pStage->stateBits & (GLS_SRCBLEND_BITS|GLS_DSTBLEND_BITS)) == (GLS_SRCBLEND_ONE|GLS_DSTBLEND_ONE_MINUS_SRC_COLOR))
		{ //I don't know, it works. -rww
			pStage->mGLFogColorOverride = GLFOGOVERRIDE_BLACK;
		}
		else
		{
			pStage->mGLFogColorOverride = GLFOGOVERRIDE_NONE;
		}
		//rww - end hw fog

		stageIndex++; //rwwRMG - needed for AGEN_BLEND
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
		//stage = VertexLightingCollapse();
		//rww - since this does bad things, I am commenting it out for now. If you want to attempt a fix, feel free.
		hasLightmapStage = qfalse;
	}

	//
	// look for multitexture potential
	//
	if ( stage > 1 && CollapseMultitexture() ) {
		stage--;
	}

	if ( shader.lightmapIndex[0] >= 0 && !hasLightmapStage )
	{
		if (vertexLightmap)
		{
//			ri->DPrintf( "WARNING: shader '%s' has VERTEX forced lightmap!\n", shader.name );
		}
		else
		{
			ri->Printf( PRINT_DEVELOPER, "WARNING: shader '%s' has lightmap but no lightmap stage!\n", shader.name );
			memcpy(shader.lightmapIndex, lightmapsNone, sizeof(shader.lightmapIndex));
			memcpy(shader.styles, stylesDefault, sizeof(shader.styles));
		}
	}


	//
	// compute number of passes
	//
	shader.numUnfoggedPasses = stage;

	// fogonly shaders don't have any normal passes
	if ( stage == 0 && !shader.sky ) {
		shader.sort = SS_FOG;
	}

	for ( stage = 1; stage < shader.numUnfoggedPasses; stage++ )
	{
		// Make sure stage is non detail and active
		if(stages[stage].isDetail || !stages[stage].active)
		{
			break;
		}
		// MT lightmaps are always in bundle 1
		if(stages[stage].bundle[0].isLightmap)
		{
			continue;
		}
	}

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
#ifdef USE_NEW_SHADER_HASH
	shaderText_t *st;
	int hash;

	hash = generateHashValue(shadername, MAX_SHADERTEXT_HASH);

	for( st = shaderTextHashTable[hash]; st; st = st->next ) {
		if ( !Q_stricmp( st->name, shadername ) ) {
			return st->text;
		}
	}

	// drakkar - if shader does not exists in the hashtable then it does not exist in s_shaderText
	return NULL;
#else
	char *token;
	const char *p;

	int i, hash;

	hash = generateHashValue(shadername, MAX_SHADERTEXT_HASH);

	for (i = 0; shaderTextHashTable[hash][i]; i++) {
		p = shaderTextHashTable[hash][i];
		token = Shader_ParseExt(&p, qtrue);
		if ( !Q_stricmp( token, shadername ) ) {
			return p;
		}
	}

	p = s_shaderText;

	if ( !p ) {
		return NULL;
	}

	// look for label
	while ( 1 ) {
		token = Shader_ParseExt( &p, qtrue );
		if ( token[0] == 0 ) {
			break;
		}

		if ( !Q_stricmp( token, shadername ) ) {
			return p;
		}
		else {
			// skip the definition
			Shader_SkipBracedSection( &p );
		}
	}

	return NULL;
#endif
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

	if ( (name==NULL) || (name[0] == 0) ) {  // bk001205
		return tr.defaultShader;
	}

	COM_StripExtension( name, strippedName, sizeof( strippedName ) );

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
dynamic diffuse lighting applied to it, as appropriate for most
entity skin surfaces.

If lightmapIndex == LIGHTMAP_2D, then the image will be used
for 2D rendering unless an explicit shader is found

If lightmapIndex == LIGHTMAP_BY_VERTEX, then the image will use
the vertex rgba modulate values, as appropriate for misc_model
pre-lit surfaces.

Other lightmapIndex values will have a lightmap stage created
and src*dest blending applied with the texture, as appropriate for
most world construction surfaces.

===============
*/
shader_t *R_FindShader( const char *name, const int *lightmapIndex, const byte *styles, qboolean mipRawImage )
{
	char		strippedName[MAX_QPATH];
	char		fileName[MAX_QPATH];
	int			hash;
	const char	*shaderText;
	image_t		*image;
	shader_t	*sh;

	if ( name[0] == 0 ) {
		return tr.defaultShader;
	}

	// use (fullbright) vertex lighting if the bsp file doesn't have
	// lightmaps
	if ( lightmapIndex[0] >= 0 && lightmapIndex[0] >= tr.numLightmaps )
	{
		lightmapIndex = lightmapsVertex;
	}
	else if ( lightmapIndex[0] < LIGHTMAP_2D )
	{
		// negative lightmap indexes cause stray pointers (think tr.lightmaps[lightmapIndex])
		ri->Printf( PRINT_WARNING, "WARNING: shader '%s' has invalid lightmap index of %d\n", name, lightmapIndex[0] );
		lightmapIndex = lightmapsVertex;
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
		if (IsShader(sh, strippedName, lightmapIndex, styles))
		{
			return sh;
		}
	}

	// clear the global shader
	ClearGlobalShader();
	Q_strncpyz(shader.name, strippedName, sizeof(shader.name));
	Com_Memcpy(shader.lightmapIndex, lightmapIndex, sizeof(shader.lightmapIndex));
	Com_Memcpy(shader.styles, styles, sizeof(shader.styles));

	//
	// attempt to define shader from an explicit parameter file
	//
	shaderText = FindShaderInShaderText( strippedName );
	if ( shaderText ) {
		if ( !ParseShader( &shaderText ) ) {
			// had errors, so use default shader
			shader.defaultShader = true;
		}
		sh = FinishShader();
		return sh;
	}


	//
	// if not defined in the in-memory shader descriptions,
	// look for a single TGA, BMP, or PCX
	//
	COM_StripExtension( name, fileName, sizeof( fileName ) );
	image = R_FindImageFile( fileName, mipRawImage, mipRawImage, qtrue, mipRawImage ? GL_REPEAT : GL_CLAMP );
	if ( !image ) {
		ri->Printf( PRINT_DEVELOPER, S_COLOR_RED "Couldn't find image for shader %s\n", name );
		shader.defaultShader = true;
		return FinishShader();
	}
	//
	// create the default shading commands
	//
	if ( shader.lightmapIndex[0] == LIGHTMAP_NONE ) {
		// dynamic colors at vertexes
		stages[0].bundle[0].image = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
		stages[0].stateBits = GLS_DEFAULT;
	} else if ( shader.lightmapIndex[0] == LIGHTMAP_BY_VERTEX ) {
		// explicit colors at vertexes
		stages[0].bundle[0].image = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_EXACT_VERTEX;
		stages[0].alphaGen = AGEN_SKIP;
		stages[0].stateBits = GLS_DEFAULT;
	} else if ( shader.lightmapIndex[0] == LIGHTMAP_2D ) {
		// GUI elements
		stages[0].bundle[0].image = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_VERTEX;
		stages[0].alphaGen = AGEN_VERTEX;
		stages[0].stateBits = GLS_DEPTHTEST_DISABLE |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	} else if ( shader.lightmapIndex[0] == LIGHTMAP_WHITEIMAGE ) {
		// fullbright level
		stages[0].bundle[0].image = tr.whiteImage;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	} else {
		// two pass lightmap
		stages[0].bundle[0].image = tr.lightmaps[shader.lightmapIndex[0]];
		stages[0].bundle[0].isLightmap = qtrue;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY;	// lightmaps are scaled on creation
													// for identitylight
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	}

	return FinishShader();
}

static void ScanAndLoadShaderFiles( const char *path );
shader_t *R_FindServerShader( const char *name, const int *lightmapIndex, const byte *styles, qboolean mipRawImage )
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
		if (IsShader(sh, strippedName, lightmapIndex, styles))
		{
			return sh;
		}
	}

	// clear the global shader
	ClearGlobalShader();
	Q_strncpyz(shader.name, strippedName, sizeof(shader.name));
	memcpy(shader.lightmapIndex, lightmapIndex, sizeof(shader.lightmapIndex));
	memcpy(shader.styles, styles, sizeof(shader.styles));

	shader.defaultShader = true;
	return FinishShader();
}

qhandle_t RE_RegisterShaderFromImage(const char *name, int *lightmapIndex, byte *styles, image_t *image, qboolean mipRawImage) {
	int			i, hash;
	shader_t	*sh;

	hash = generateHashValue(name, FILE_HASH_SIZE);

	// probably not necessary since this function
	// only gets called from tr_font.c with lightmapIndex == LIGHTMAP_2D
	// but better safe than sorry.
	// Doesn't actually ever get called in JA at all
	if ( lightmapIndex[0] >= tr.numLightmaps ) {
		lightmapIndex = (int *)lightmapsFullBright;
	}

	//
	// see if the shader is already loaded
	//
	for (sh=hashTable[hash]; sh; sh=sh->next) {
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if (IsShader(sh, name, lightmapIndex, styles))
		{
			return sh->index;
		}
	}

	// clear the global shader
	memset( &shader, 0, sizeof( shader ) );
	memset( &stages, 0, sizeof( stages ) );
	Q_strncpyz(shader.name, name, sizeof(shader.name));
	memcpy(shader.lightmapIndex, lightmapIndex, sizeof(shader.lightmapIndex));
	memcpy(shader.styles, styles, sizeof(shader.styles));

	for ( i = 0 ; i < MAX_SHADER_STAGES ; i++ ) {
		stages[i].bundle[0].texMods = texMods[i];
	}

	//
	// create the default shading commands
	//
	if ( shader.lightmapIndex[0] == LIGHTMAP_NONE ) {
		// dynamic colors at vertexes
		stages[0].bundle[0].image = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
		stages[0].stateBits = GLS_DEFAULT;
	} else if ( shader.lightmapIndex[0] == LIGHTMAP_BY_VERTEX ) {
		// explicit colors at vertexes
		stages[0].bundle[0].image = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_EXACT_VERTEX;
		stages[0].alphaGen = AGEN_SKIP;
		stages[0].stateBits = GLS_DEFAULT;
	} else if ( shader.lightmapIndex[0] == LIGHTMAP_2D ) {
		// GUI elements
		stages[0].bundle[0].image = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_VERTEX;
		stages[0].alphaGen = AGEN_VERTEX;
		stages[0].stateBits = GLS_DEPTHTEST_DISABLE |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	} else if ( shader.lightmapIndex[0] == LIGHTMAP_WHITEIMAGE ) {
		// fullbright level
		stages[0].bundle[0].image = tr.whiteImage;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	} else {
		// two pass lightmap
		stages[0].bundle[0].image = tr.lightmaps[shader.lightmapIndex[0]];
		stages[0].bundle[0].isLightmap = qtrue;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY;	// lightmaps are scaled on creation
													// for identitylight
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image = image;
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
qhandle_t RE_RegisterShaderLightMap( const char *name, const int *lightmapIndex, const byte *styles )
{
	shader_t	*sh;

	if ( strlen( name ) >= MAX_QPATH ) {
		ri->Printf( PRINT_ALL, "Shader name exceeds MAX_QPATH\n" );
		return 0;
	}

	sh = R_FindShader( name, lightmapIndex, styles, qtrue );

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
		ri->Printf( PRINT_ALL, "Shader name exceeds MAX_QPATH\n" );
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
		ri->Printf( PRINT_ALL, "Shader name exceeds MAX_QPATH\n" );
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
	  ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "R_GetShaderByHandle: out of range hShader '%d'\n", hShader ); // bk: FIXME name
		return tr.defaultShader;
	}
	if ( hShader >= tr.numShaders ) {
		ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "R_GetShaderByHandle: out of range hShader '%d'\n", hShader );
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

	ri->Printf( PRINT_ALL,  "-----------------------\n");

	count = 0;
	for ( i = 0 ; i < tr.numShaders ; i++ ) {
		if ( ri->Cmd_Argc() > 1 ) {
			shader = tr.sortedShaders[i];
		} else {
			shader = tr.shaders[i];
		}

		ri->Printf( PRINT_ALL, "%i ", shader->numUnfoggedPasses );

		if (shader->lightmapIndex[0] >= 0 ) {
			ri->Printf( PRINT_ALL,  "L ");
		} else {
			ri->Printf( PRINT_ALL,  "  ");
		}
		if ( shader->multitextureEnv == GL_ADD ) {
			ri->Printf( PRINT_ALL, "MT(a) " );
		} else if ( shader->multitextureEnv == GL_MODULATE ) {
			ri->Printf( PRINT_ALL, "MT(m) " );
		} else if ( shader->multitextureEnv == GL_DECAL ) {
			ri->Printf( PRINT_ALL, "MT(d) " );
		} else {
			ri->Printf( PRINT_ALL, "      " );
		}
		if ( shader->explicitlyDefined ) {
			ri->Printf( PRINT_ALL, "E " );
		} else {
			ri->Printf( PRINT_ALL, "  " );
		}

		if ( shader->sky )
		{
			ri->Printf( PRINT_ALL, "sky " );
		} else {
			ri->Printf( PRINT_ALL, "gen " );
		}
		if ( shader->defaultShader ) {
			ri->Printf( PRINT_ALL,   ": %s (DEFAULTED)\n", shader->name);
		} else {
			ri->Printf( PRINT_ALL,   ": %s\n", shader->name);
		}
		count++;
	}
	ri->Printf( PRINT_ALL,  "%i total shaders\n", count);
	ri->Printf( PRINT_ALL,  "------------------\n");
}


#ifdef USE_NEW_SHADER_HASH
// drakkar - extract shaders from the buffer and insert into the hashtable
static void LoadShaderFromBuffer( char *buff )
{
	char shadername[MAX_SHADERNAME_LENGTH+1];
	shaderText_t *st;
	char *p, *name, *text;
	int nameLength, textLength;
	long size, hash;
	int q3ShaderBug = 1;

	if( !buff ) return;

	p = buff;
	while( *p )
	{
		// get next shader name and shader text from buffer
		Shader_CompressBracedSection( &p, &name, &text, &nameLength, &textLength );
		if( !nameLength || !textLength ) continue;

		if( nameLength >= MAX_SHADERNAME_LENGTH ) {
			strncpy( shadername, name, MAX_SHADERNAME_LENGTH );
			shadername[MAX_SHADERNAME_LENGTH] = '\0';
			ri->Printf( PRINT_DEVELOPER, "Warning: Shader name too long '%s'...\n", shadername );
			continue;
		}

		strncpy( shadername, name, nameLength );
		shadername[nameLength] = '\0';
		name = shadername;

		// if shader already exists ignore the new shader text
		hash = generateHashValue( name, MAX_SHADERTEXT_HASH );
		for( st = shaderTextHashTable[hash]; st; st = st->next ) {
			if( !Q_stricmp( name, st->name ) ) {
				if( q3ShaderBug ) { // simulating q3 shader bug: override only the first shader of the buffer
					st->name[0] = '\0';
					st = NULL;
				}
				break;
			}
		}
		if( st ) continue;
		q3ShaderBug = 0;

		// create the new shader
		size = sizeof(shaderText_t) + (textLength) + (nameLength+1);
		st = (shaderText_t *)ri->Hunk_Alloc( size, h_low );

		// copy shader name and shader text
		memcpy( st->text, text, textLength );
		st->text[textLength] = '\0';
		st->name = st->text + (textLength+1);
		strncpy( st->name, name, nameLength );
		st->name[nameLength] = '\0';

		// insert the new shader into hashtable
		st->next = shaderTextHashTable[hash];
		shaderTextHashTable[hash] = st;

		shaderCount++;
	}
}

static void ScanAndLoadShaderFiles( const char *path ) // drakkar - using LoadShaderFromBuffer()
{
	char   filename[MAX_QPATH];
	char  *buffer;
	char **shaderFiles;
	int    i, numShaderFiles;

	// scan for shader files
	shaderFiles = ri->FS_ListFiles( path, ".shader", &numShaderFiles );
	if ( !shaderFiles || !numShaderFiles )
	{
		Com_Error(ERR_FATAL, "ERROR: no shader files found\n");
		return;
	}

	// load and parse shader files
	for( i = numShaderFiles-1; i >= 0; i-- ) // parse shaders in reverse order
	{
		Com_sprintf( filename, sizeof(filename), "%s/%s", path, shaderFiles[i] );
		ri->Printf( PRINT_DEVELOPER, "...loading '%s'\n", filename );
		ri->FS_ReadFile( filename, (void**)&buffer );
		if( !buffer ) Com_Error( ERR_DROP, "Couldn't load %s", filename );

		LoadShaderFromBuffer( buffer ); // extract and index all shaders from the buffer

		ri->FS_FreeFile( buffer );

		fileShaderCount++;
	}

	// free up memory
	ri->FS_FreeFileList( shaderFiles );

	return;

}
// !drakkar
#else
/*
====================
ScanAndLoadShaderFiles

Finds and loads all .shader files, combining them into
a single large text block that can be scanned for shader names

rww - Do not access any ri or render data stuff that doesn't get init'd on
a dedicated server in here. I am calling it for dedicateds now because
we need all this shader BS for looking up surface indicators in skin
files if we want to be like SP.

bto (VV) - Rather than keeping all the buffer pointers around forever and
creating more bugs, do the hash creation with the finalized shadertext.
Previous code only really worked if ri->FS_ReadFile returned contiguous buffers
in ascending order on consecutive calls.
=====================
*/
#define	MAX_SHADER_FILES	4096
static void ScanAndLoadShaderFiles( const char *path )
{
	char **shaderFiles;
	char *buffers[MAX_SHADER_FILES];
	char *p;
	int numShaderFiles;
	int i;
	char *oldp, *token, *hashMem, *textEnd;
	int shaderTextHashTableSizes[MAX_SHADERTEXT_HASH], hash, size;

	long sum = 0, summand;
	// scan for shader files
	shaderFiles = ri->FS_ListFiles( path, ".shader", &numShaderFiles );

	if ( !shaderFiles || !numShaderFiles )
	{
		Com_Error(ERR_FATAL, "ERROR: no shader files found\n");
		return;
	}

	if ( numShaderFiles > MAX_SHADER_FILES ) {
		numShaderFiles = MAX_SHADER_FILES;
	}

	// load and parse shader files
	for ( i = 0; i < numShaderFiles; i++ )
	{
		char filename[MAX_QPATH];

		Com_sprintf( filename, sizeof( filename ), "%s/%s", path, shaderFiles[i] );
		ri->Printf( PRINT_DEVELOPER, "...loading '%s'\n", filename );
		summand = ri->FS_ReadFile( filename, (void **)&buffers[i] );
		if ( !buffers[i] ) {
			ri->Com_Error( ERR_DROP, "Couldn't load %s", filename );
		}

		// Do a simple check on the shader structure in that file to make sure one bad shader file cannot fuck up all other shaders.
		p = buffers[i];
		while(1)
		{
			token = Shader_ParseExt((const char **)&p, qtrue);

			if(!*token)
				break;

			oldp = p;

			token = Shader_ParseExt((const char **)&p, qtrue);
			if(token[0] != '{' && token[1] != '\0')
			{
				ri->Printf( PRINT_ALL, S_COLOR_YELLOW "WARNING: Bad shader file %s has incorrect syntax.\n", filename);
				ri->FS_FreeFile(buffers[i]);
				buffers[i] = NULL;
				break;
			}

			Shader_SkipBracedSection((const char **)&oldp);
			p = oldp;
		}

		if (buffers[i])
			sum += summand;
	}

	// build single large buffer
	s_shaderText = (char *)ri->Hunk_Alloc( sum + numShaderFiles*2, h_low );
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
		ri->FS_FreeFile( buffers[i] );
	}

	Shader_Compress( s_shaderText );

	// free up memory
	ri->FS_FreeFileList( shaderFiles );

	memset(shaderTextHashTableSizes, 0, sizeof(shaderTextHashTableSizes));
	size = 0;

	p = s_shaderText;
	// look for label
	while ( 1 ) {
		token = Shader_ParseExt( (const char **)&p, qtrue );
		if ( token[0] == 0 ) {
			break;
		}

		hash = generateHashValue(token, MAX_SHADERTEXT_HASH);
		shaderTextHashTableSizes[hash]++;
		size++;
		Shader_SkipBracedSection((const char **)&p);
	}

	size += MAX_SHADERTEXT_HASH;

	hashMem = (char *)ri->Hunk_Alloc( size * sizeof(char *), h_low );

	for (i = 0; i < MAX_SHADERTEXT_HASH; i++) {
		shaderTextHashTable[i] = (char **) hashMem;
		hashMem = ((char *) hashMem) + ((shaderTextHashTableSizes[i] + 1) * sizeof(char *));
	}

	memset(shaderTextHashTableSizes, 0, sizeof(shaderTextHashTableSizes));

	p = s_shaderText;
	// look for label
	while ( 1 ) {
		oldp = p;
		token = Shader_ParseExt( (const char **)&p, qtrue );
		if ( token[0] == 0 ) {
			break;
		}

		hash = generateHashValue(token, MAX_SHADERTEXT_HASH);
		shaderTextHashTable[hash][shaderTextHashTableSizes[hash]++] = oldp;

		Shader_SkipBracedSection((const char **)&p);
	}

	return;
}
#endif

/*
====================
R_CreateBlendedShader

  This takes 4 shaders (one per corner of a quad) and creates a blended shader the fades the textures over
  eg.
  if [A][A]
     [B][B]
  then the shader would be texture A at the top fading to texture B at the bottom

  This is highly biased towards terrain shaders ie vertex lit surfaces
====================
*/
//rwwRMG: Added:

static void R_CopyStage(shaderStage_t *orig, shaderStage_t *stage)
{
	// Assumption: this stage has not been collapsed
	*stage = *orig;		//Just copy the whole thing!
}

static void R_CreateBlendedStage(qhandle_t handle, int idx)
{
	shader_t	*work;

	work = R_GetShaderByHandle(handle);
	R_CopyStage(work->stages, stages + idx);
	stages[idx].rgbGen = CGEN_EXACT_VERTEX;
	stages[idx].alphaGen = AGEN_BLEND;
	stages[idx].stateBits = GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE | GLS_DEPTHMASK_TRUE;

	if (stages[idx].ss)
	{
		stages[idx].ss->density *= 0.33f;
	}
}

static qhandle_t R_MergeShaders(const char *blendedName, qhandle_t a, qhandle_t b, qhandle_t c, bool surfaceSprites)
{
	shader_t	*blended;
	shader_t	*work;
	int			current, i;

	R_IssuePendingRenderCommands();

	// Set up default parameters
	ClearGlobalShader();
	Q_strncpyz(shader.name, blendedName, sizeof(shader.name));
	memcpy(shader.lightmapIndex, lightmapsVertex, sizeof(shader.lightmapIndex));
	memcpy(shader.styles, stylesDefault, sizeof(shader.styles));
	shader.fogPass = FP_EQUAL;

	// Get the top left shader and set it up as pass 0 - it should be completely opaque
	work = R_GetShaderByHandle(c);
	stages[0].active = qtrue;
	R_CopyStage(work->stages, stages);
	stages[0].rgbGen = CGEN_EXACT_VERTEX;
	stages[0].alphaGen = AGEN_BLEND;
	stages[0].stateBits = GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK_TRUE;
	shader.multitextureEnv = work->multitextureEnv;	//jic

	// Go through the other verts and add a pass
	R_CreateBlendedStage(a, 1);
	R_CreateBlendedStage(b, 2);

	if ( surfaceSprites )
	{
		current = 3;
		work = R_GetShaderByHandle(a);
		for(i=1;(i<work->numUnfoggedPasses && current<MAX_SHADER_STAGES);i++)
		{
			if (work->stages[i].ss)
			{
				stages[current] = work->stages[i];
	//			stages[current].ss->density *= 0.33f;
				stages[current].ss->density *= 3;
				current++;
			}
		}

		work = R_GetShaderByHandle(b);
		for(i=1;(i<work->numUnfoggedPasses && current<MAX_SHADER_STAGES);i++)
		{
			if (work->stages[i].ss)
			{
				stages[current] = work->stages[i];
	//			stages[current].ss->density *= 0.33f;
				stages[current].ss->density *= 3;
				current++;
			}
		}

		work = R_GetShaderByHandle(c);
		for(i=1;(i<work->numUnfoggedPasses && current<MAX_SHADER_STAGES);i++)
		{
			if (work->stages[i].ss)
			{
				stages[current] = work->stages[i];
	//			stages[current].ss->density *= 0.33f;
				stages[current].ss->density *= 3;
				current++;
			}
		}
	}

	blended = FinishShader();
	return(blended->index);
}


// Create a 3 pass shader - the last 2 passes are alpha'd out

qhandle_t R_CreateBlendedShader(qhandle_t a, qhandle_t b, qhandle_t c, bool surfaceSprites )
{
	qhandle_t	blended;
	shader_t	*work;
	char		blendedName[MAX_QPATH];
	char		extendedName[MAX_QPATH + MAX_QPATH];

	Com_sprintf(blendedName, MAX_QPATH, "blend(%d,%d,%d)", a, b, c);
	if (!surfaceSprites)
	{
		strcat(blendedName, "noSS");
	}

	// Find if this shader has already been created
	R_CreateExtendedName(extendedName, sizeof(extendedName), blendedName, lightmapsVertex, stylesDefault);
	work = hashTable[generateHashValue(extendedName, FILE_HASH_SIZE)];
	for ( ; work; work = work->next)
	{
		if (Q_stricmp(work->name, extendedName) == 0)
		{
			return work->index;
		}
	}

	// Create new shader if it doesn't already exist
	blended = R_MergeShaders(extendedName, a, b, c, surfaceSprites);
	return(blended);
}

/*
====================
CreateInternalShaders
====================
*/
static void CreateInternalShaders( void ) {
	tr.numShaders = 0;

	// init the default shader
	memset( &shader, 0, sizeof( shader ) );
	memset( &stages, 0, sizeof( stages ) );

	Q_strncpyz( shader.name, "<default>", sizeof( shader.name ) );

	memcpy(shader.lightmapIndex, lightmapsNone, sizeof(shader.lightmapIndex));
	memcpy(shader.styles, stylesDefault, sizeof(shader.styles));
	for ( int i = 0 ; i < MAX_SHADER_STAGES ; i++ ) {
		stages[i].bundle[0].texMods = texMods[i];
	}

	stages[0].bundle[0].image = tr.defaultImage;
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
	shader.defaultShader = qtrue;


	#define GL_PROGRAM_ERROR_STRING_ARB						0x8874
	#define GL_PROGRAM_ERROR_POSITION_ARB					0x864B

	// Allocate and Load the global 'Glow' Vertex Program. - AReis
	if ( qglGenProgramsARB )
	{
		qglGenProgramsARB( 1, &tr.glowVShader );
		qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, tr.glowVShader );
		qglProgramStringARB( GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen( ( char * ) g_strGlowVShaderARB ), g_strGlowVShaderARB );

//		const GLubyte *strErr = qglGetString( GL_PROGRAM_ERROR_STRING_ARB );
		int iErrPos = 0;
		qglGetIntegerv( GL_PROGRAM_ERROR_POSITION_ARB, &iErrPos );
		assert( iErrPos == -1 );
	}

	// NOTE: I make an assumption here. If you have (current) nvidia hardware, you obviously support register combiners instead of fragment
	// programs, so use those. The problem with this is that nv30 WILL support fragment shaders, breaking this logic. The good thing is that
	// if you always ask for regcoms before fragment shaders, you'll always just use regcoms (problem solved... for now). - AReis

	// Load Pixel Shaders (either regcoms or fragprogs).
	if ( qglCombinerParameteriNV )
	{
		// The purpose of this regcom is to blend all the pixels together from the 4 texture units, but with their
		// texture coordinates offset by 1 (or more) texels, effectively letting us blend adjoining pixels. The weight is
		// used to either strengthen or weaken the pixel intensity. The more it diffuses (the higher the radius of the glow),
		// the higher the intensity should be for a noticable effect.
		// Regcom result is: ( tex1 * fBlurWeight ) + ( tex2 * fBlurWeight ) + ( tex2 * fBlurWeight ) + ( tex2 * fBlurWeight )

		// VV guys, this is the pixel shader you would use instead :-)
		/*
		// c0 is the blur weight.
		ps 1.1
		tex		t0
		tex		t1
		tex		t2
		tex		t3

		mul		r0, c0, t0;
		madd	r0, c0, t1, r0;
		madd	r0, c0, t2, r0;
		madd	r0, c0, t3, r0;
		*/
		tr.glowPShader = qglGenLists( 1 );
		qglNewList( tr.glowPShader, GL_COMPILE );
			qglCombinerParameteriNV( GL_NUM_GENERAL_COMBINERS_NV, 2 );

			// spare0 = fBlend * tex0 + fBlend * tex1.
			qglCombinerInputNV( GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_A_NV, GL_TEXTURE0_ARB, GL_UNSIGNED_IDENTITY_NV, GL_RGB );
			qglCombinerInputNV( GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_B_NV, GL_CONSTANT_COLOR0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB );
			qglCombinerInputNV( GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_C_NV, GL_TEXTURE1_ARB, GL_UNSIGNED_IDENTITY_NV, GL_RGB );
			qglCombinerInputNV( GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_D_NV, GL_CONSTANT_COLOR0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB );
			qglCombinerOutputNV( GL_COMBINER0_NV, GL_RGB, GL_DISCARD_NV, GL_DISCARD_NV, GL_SPARE0_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE );

			// spare1 = fBlend * tex2 + fBlend * tex3.
			qglCombinerInputNV( GL_COMBINER1_NV, GL_RGB, GL_VARIABLE_A_NV, GL_TEXTURE2_ARB, GL_UNSIGNED_IDENTITY_NV, GL_RGB );
			qglCombinerInputNV( GL_COMBINER1_NV, GL_RGB, GL_VARIABLE_B_NV, GL_CONSTANT_COLOR0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB );
			qglCombinerInputNV( GL_COMBINER1_NV, GL_RGB, GL_VARIABLE_C_NV, GL_TEXTURE3_ARB, GL_UNSIGNED_IDENTITY_NV, GL_RGB );
			qglCombinerInputNV( GL_COMBINER1_NV, GL_RGB, GL_VARIABLE_D_NV, GL_CONSTANT_COLOR0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB );
			qglCombinerOutputNV( GL_COMBINER1_NV, GL_RGB, GL_DISCARD_NV, GL_DISCARD_NV, GL_SPARE1_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE );

			// ( A * B ) + ( ( 1 - A ) * C ) + D = ( spare0 * 1 ) + ( ( 1 - spare0 ) * 0 ) + spare1 == spare0 + spare1.
			qglFinalCombinerInputNV( GL_VARIABLE_A_NV, GL_SPARE0_NV,    GL_UNSIGNED_IDENTITY_NV, GL_RGB );
			qglFinalCombinerInputNV( GL_VARIABLE_B_NV, GL_ZERO,			GL_UNSIGNED_INVERT_NV, GL_RGB );
			qglFinalCombinerInputNV( GL_VARIABLE_C_NV, GL_ZERO,			GL_UNSIGNED_IDENTITY_NV, GL_RGB );
			qglFinalCombinerInputNV( GL_VARIABLE_D_NV, GL_SPARE1_NV,	GL_UNSIGNED_IDENTITY_NV, GL_RGB );
		qglEndList();
	}
	else if ( qglGenProgramsARB )
	{
		qglGenProgramsARB( 1, &tr.glowPShader );
		qglBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, tr.glowPShader );
		qglProgramStringARB( GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen( ( char * ) g_strGlowPShaderARB ), g_strGlowPShaderARB );

//		const GLubyte *strErr = qglGetString( GL_PROGRAM_ERROR_STRING_ARB );
		int iErrPos = 0;
		qglGetIntegerv( GL_PROGRAM_ERROR_POSITION_ARB, &iErrPos );
		assert( iErrPos == -1 );
	}
}

static void CreateExternalShaders( void ) {
	tr.projectionShadowShader = R_FindShader( "projectionShadow", lightmapsNone, stylesDefault, qtrue );
	tr.projectionShadowShader->sort = SS_STENCIL_SHADOW;
	tr.sunShader = R_FindShader( "sun", lightmapsNone, stylesDefault, qtrue );
}

/*
==================
R_InitShaders
==================
*/
void R_InitShaders(qboolean server)
{
	//ri->Printf( PRINT_ALL, "Initializing Shaders\n" );

#if defined(USE_NEW_SHADER_HASH) && !(DEDICATED)
	int time, mem;
	// drakkar - profiling shader parse session
	if (!server)
	{
		Shader_BeginParseSession( "R_InitShaders" );
		time = ri->Milliseconds()*ri->Cvar_VariableValue( "timescale" );
		mem = Hunk_MemoryRemaining();
		fileShaderCount = 0;
		shaderCount = 0;
	}
	// !drakkar
#endif

	memset(hashTable, 0, sizeof(hashTable));
#ifdef USE_NEW_SHADER_HASH
	memset(shaderTextHashTable, 0, sizeof(shaderTextHashTable));  // drakkar - clear shader hashtable
#endif

	deferLoad = qfalse;

	if (!server)
	{
		CreateInternalShaders();

		ScanAndLoadShaderFiles("shaders");

		CreateExternalShaders();
	}

#if defined(USE_NEW_SHADER_HASH) && !(DEDICATED)
// drakkar - print profiling info
	if (!server)
	{
		time = ri->Milliseconds()*ri->Cvar_VariableValue( "timescale" ) - time;
		mem = mem - Hunk_MemoryRemaining();
		ri->Printf( PRINT_ALL, "-------------------------\n" );
		ri->Printf( PRINT_ALL, "%d shader files read \n", fileShaderCount );
		ri->Printf( PRINT_ALL, "%d shaders found\n", shaderCount );
		ri->Printf( PRINT_ALL, "%d code lines\n", Shader_GetCurrentParseLine() );
		ri->Printf( PRINT_ALL, "%.2f MB shader data\n", mem/1024.0f/1024.0f );
		ri->Printf( PRINT_ALL, "%.3f seconds\n", time/1000.0f );
		ri->Printf( PRINT_ALL, "-------------------------\n" );
		Shader_BeginParseSession( "" );
	}
	// !drakkar
#endif
}
