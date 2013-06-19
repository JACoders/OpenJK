//Anything above this #include will be ignored by the compiler
#include "qcommon/exe_headers.h"

#include "cm_local.h"
#include "memory.h"
#include "chash.h"

class CCMShaderText
{
private:
	char					mName[MAX_QPATH];
	class CCMShaderText		*mNext;
	const char				*mData;
public:
	// Constructors
	CCMShaderText(const char *name, const char *data) { Q_strncpyz(mName, name, MAX_QPATH); mNext = NULL; mData = data; }
	~CCMShaderText(void) {}

	// Accessors
	const char *GetName(void) const { return(mName); }
	class CCMShaderText *GetNext(void) const { return(mNext); }
	void SetNext(class CCMShaderText *next) { mNext = next; }
	void Destroy(void) { delete this; }

	const char *GetData(void) const { return(mData); }
};

char			   		*shaderText = NULL;
CHash<CCMShaderText>	shaderTextTable;
CHash<CCMShader>		cmShaderTable;

/*
====================
CM_CreateShaderTextHash
=====================
*/
void CM_CreateShaderTextHash(void)
{
	const char			*p;
	qboolean			hasNewLines;
	char				*token;
	CCMShaderText		*shader;

	p = shaderText;
	// look for label
	while (p) 
	{
		p = SkipWhitespace(p, &hasNewLines);
		token = COM_ParseExt( &p, qtrue );
		if ( !token[0] ) 
		{
			break;
		}
		shader = new CCMShaderText(token, p);
		shaderTextTable.insert(shader);

		SkipBracedSection(&p);
	}
}

/*
====================
CM_LoadShaderFiles

Finds and loads all .shader files, combining them into
a single large text block that can be scanned for shader names
=====================
*/
#define	MAX_SHADER_FILES	1024

void CM_LoadShaderFiles( void )
{
	char	**shaderFiles1;
	int		numShaders1;
#ifndef FINAL_BUILD
	char	**shaderFiles2;
	int		numShaders2;
#endif
	char	*buffers[MAX_SHADER_FILES];
	int		numShaders;
	int		i;
	int		sum = 0;

	// scan for shader files
	shaderFiles1 = FS_ListFiles( "shaders", ".shader", &numShaders1 );
#ifndef FINAL_BUILD
	shaderFiles2 = FS_ListFiles( "shaders/test", ".shader", &numShaders2 );
#endif

	if ( !shaderFiles1 || !numShaders1 )
	{
		Com_Printf( S_COLOR_YELLOW "WARNING: no shader files found\n" );
		return;
	}

#ifndef FINAL_BUILD
	numShaders = numShaders1 + numShaders2;
#else
	numShaders = numShaders1;
#endif
	if ( numShaders > MAX_SHADER_FILES ) 
	{
		numShaders = MAX_SHADER_FILES;
	}

	// load and parse shader files
	for ( i = 0; i < numShaders1; i++ )
	{
		char filename[MAX_QPATH];

		Com_sprintf( filename, sizeof( filename ), "shaders/%s", shaderFiles1[i] );
		Com_DPrintf( "...loading '%s'\n", filename );
		sum += FS_ReadFile( filename, (void **)&buffers[i] );
		if ( !buffers[i] ) 
		{
			Com_Error( ERR_FATAL, "Couldn't load %s", filename );
		}
	}
#ifndef FINAL_BUILD
	for ( ; i < numShaders; i++ )
	{
		char filename[MAX_QPATH];

		Com_sprintf( filename, sizeof( filename ), "shaders/test/%s", shaderFiles2[i - numShaders1] );
		Com_DPrintf( "...loading '%s'\n", filename );
		sum += FS_ReadFile( filename, (void **)&buffers[i] );
		if ( !buffers[i] ) 
		{
			Com_Error( ERR_DROP, "Couldn't load %s", filename );
		}
	}
#endif

	// build single large buffer
	shaderText = (char *)Z_Malloc( sum + numShaders * 2, TAG_SHADERTEXT, qtrue);

	// free in reverse order, so the temp files are all dumped
	for ( i = numShaders - 1; i >= 0 ; i-- ) 
	{
		strcat( shaderText, "\n" );
		strcat( shaderText, buffers[i] );
		FS_FreeFile( buffers[i] );
	}

	// free up memory
	FS_FreeFileList( shaderFiles1 );
#ifndef FINAL_BUILD
	FS_FreeFileList( shaderFiles2 );
#endif
}

/*
==================
CM_GetShaderText
==================
*/

const char *CM_GetShaderText(const char *key)
{
	CCMShaderText	*st;

	st = shaderTextTable[key];
	if(st)
	{
		return(st->GetData());
	}
	return(NULL);
}

/*
==================
CM_FreeShaderText
==================
*/

void CM_FreeShaderText(void)
{
	shaderTextTable.clear();
	if(shaderText)
	{
		Z_Free(shaderText);
		shaderText = NULL;
	}
}

/*
==================
CM_LoadShaderText

  Loads in all the .shader files so it can be accessed by the server and the renderer
  Creates a hash table to quickly access the shader text
==================
*/

void CM_LoadShaderText(qboolean forceReload)
{
	if(forceReload)
	{
		CM_FreeShaderText();
	}
	if(shaderText)
	{
		return;
	}
//	Com_Printf("Loading shader text .....\n");
	CM_LoadShaderFiles();
	CM_CreateShaderTextHash();

	//Com_Printf("..... %d shader definitions loaded\n", shaderTextTable.count());
}

/*
===============
ParseSurfaceParm

surfaceparm <name>
===============
*/

typedef struct 
{
	const char	*name;
	int		clearSolid, surfaceFlags, contents;
} infoParm_t;

infoParm_t	svInfoParms[] =
{
	// Game surface flags
	{"sky",					-1,					SURF_SKY,			0 },						// emit light from an environment map
	{"slick",				-1,					SURF_SLICK,			0 },						
																								
	{"nodamage",			-1,					SURF_NODAMAGE,		0 },						
	{"noimpact",			-1,					SURF_NOIMPACT,		0 },						// don't make impact explosions or marks
	{"nomarks",				-1,					SURF_NOMARKS,		0 },						// don't make impact marks, but still explode
	{"nodraw",				-1,					SURF_NODRAW,		0 },						// don't generate a drawsurface (or a lightmap)
	{"nosteps",				-1,					SURF_NOSTEPS,		0 },						
	{"nodlight",			-1,					SURF_NODLIGHT,		0 },						// don't ever add dynamic lights

	// Game content flags
	{"nonsolid", 			~CONTENTS_SOLID,	0, 					0 },						// special hack to clear solid flag
	{"nonopaque", 			~CONTENTS_OPAQUE,	0, 					0 },						// special hack to clear opaque flag
	{"lava",				~CONTENTS_SOLID,	0,					CONTENTS_LAVA },			// very damaging
	{"water",				~CONTENTS_SOLID,	0,					CONTENTS_WATER },
	{"fog",					~CONTENTS_SOLID,	0,					CONTENTS_FOG},				// carves surfaces entering
	{"playerclip",			~CONTENTS_SOLID,	0,					CONTENTS_PLAYERCLIP },
	{"monsterclip",			~CONTENTS_SOLID,	0,					CONTENTS_MONSTERCLIP },
	{"botclip",				~CONTENTS_SOLID,	0,					CONTENTS_BOTCLIP },			// for bots
	{"shotclip",			~CONTENTS_SOLID,	0,					CONTENTS_SHOTCLIP },
	{"trigger",				~CONTENTS_SOLID,	0,					CONTENTS_TRIGGER },
	{"nodrop",				~CONTENTS_SOLID,	0,					CONTENTS_NODROP },			// don't drop items or leave bodies (death fog, lava, etc)
	{"terrain",				~CONTENTS_SOLID,	0,					CONTENTS_TERRAIN },			// use special terrain collsion
	{"ladder",				~CONTENTS_SOLID,	0,					CONTENTS_LADDER },			// climb up in it like water
	{"abseil",				~CONTENTS_SOLID,	0,					CONTENTS_ABSEIL },			// can abseil down this brush
	{"outside",				~CONTENTS_SOLID,	0,					CONTENTS_OUTSIDE },			// volume is considered to be in the outside (i.e. not indoors)
	{"inside",				~CONTENTS_SOLID,	0,					CONTENTS_INSIDE },			// volume is considered to be inside (i.e. indoors)
																	
	{"detail",				-1,					0,					CONTENTS_DETAIL },			// don't include in structural bsp
	{"trans",				-1,					0,					CONTENTS_TRANSLUCENT },		// surface has an alpha component

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
	{"forcesight",	-1,					SURF_FORCESIGHT,0 },						// only visible with force sight
};

void SV_ParseSurfaceParm( CCMShader * shader, const char **text ) 
{
	char	*token;
	int		numsvInfoParms = sizeof(svInfoParms) / sizeof(svInfoParms[0]);
	int		i;

	token = COM_ParseExt( text, qfalse );
	for ( i = 0 ; i < numsvInfoParms ; i++ ) 
	{
		if ( !Q_stricmp( token, svInfoParms[i].name ) ) 
		{
			shader->surfaceFlags |= svInfoParms[i].surfaceFlags;
			shader->contentFlags |= svInfoParms[i].contents;
			shader->contentFlags &= svInfoParms[i].clearSolid;
			break;
		}
	}
}

/*
=================
ParseMaterial
=================
*/
const char *svMaterialNames[MATERIAL_LAST] =
{
	MATERIALS
};

void SV_ParseMaterial( CCMShader *shader, const char **text ) 
{
	char	*token;
	int		i;

	token = COM_ParseExt( text, qfalse );
	if ( !token[0] ) 
	{
		Com_Printf( S_COLOR_YELLOW "WARNING: missing material in shader '%s'\n", shader->shader );
		return;
	}
	for(i = 0; i < MATERIAL_LAST; i++)
	{
		if ( !Q_stricmp( token, svMaterialNames[i] ) ) 
		{
			shader->surfaceFlags |= i;
			break;
		}
	}
}

/*
===============
ParseVector
===============
*/
static qboolean CM_ParseVector( CCMShader *shader, const char **text, int count, float *v ) 
{
	char	*token;
	int		i;

	// FIXME: spaces are currently required after parens, should change parseext...
	token = COM_ParseExt( text, qfalse );
	if ( strcmp( token, "(" ) ) 
	{
		Com_Printf( S_COLOR_YELLOW "WARNING: missing parenthesis in shader '%s'\n", shader->shader );
		return qfalse;
	}

	for ( i = 0 ; i < count ; i++ ) 
	{
		token = COM_ParseExt( text, qfalse );
		if ( !token[0] ) 
		{
			Com_Printf( S_COLOR_YELLOW "WARNING: missing vector element in shader '%s'\n", shader->shader );
			return qfalse;
		}
		v[i] = atof( token );
	}

	token = COM_ParseExt( text, qfalse );
	if ( strcmp( token, ")" ) ) 
	{
		Com_Printf( S_COLOR_YELLOW "WARNING: missing parenthesis in shader '%s'\n", shader->shader );
		return qfalse;
	}
	return qtrue;
}

/*
=================
CM_ParseShader

The current text pointer is at the explicit text definition of the
shader.  Parse it into the global shader variable.

This extracts all the info from the shader required for physics and collision
It is designed to *NOT* load any image files and not require any of the renderer to 
be initialised.
=================
*/
static void CM_ParseShader( CCMShader *shader, const char **text )
{
	char	*token;

	token = COM_ParseExt( text, qtrue );
	if ( token[0] != '{' )
	{
		Com_Printf( S_COLOR_YELLOW "WARNING: expecting '{', found '%s' instead in shader '%s'\n", token, shader->shader );
		return;
	}

	while ( true )
	{
		token = COM_ParseExt( text, qtrue );
		if ( !token[0] )
		{
			Com_Printf( S_COLOR_YELLOW "WARNING: no concluding '}' in shader %s\n", shader->shader );
			return;
		}

		// end of shader definition
		if ( token[0] == '}' )
		{
			break;
		}
		// stage definition
		else if ( token[0] == '{' )
		{
			SkipBracedSection( text );
			continue;
		}
		// material deprecated as of 11 Jan 01
		// material undeprecated as of 7 May 01 - q3map_material deprecated
		else if ( !Q_stricmp( token, "material" ) || !Q_stricmp( token, "q3map_material" ) )
		{
			SV_ParseMaterial( shader, text );
		}
		// sun parms
		// q3map_sun deprecated as of 11 Jan 01
		else if ( !Q_stricmp( token, "sun" ) || !Q_stricmp( token, "q3map_sun" ) ) 
		{
//			float	a, b;

			token = COM_ParseExt( text, qfalse );
//			shader->sunLight[0] = atof( token );
			token = COM_ParseExt( text, qfalse );
//			shader->sunLight[1] = atof( token );
			token = COM_ParseExt( text, qfalse );
//			shader->sunLight[2] = atof( token );
			
//			VectorNormalize( shader->sunLight );

			token = COM_ParseExt( text, qfalse );
//			a = atof( token );
//			VectorScale( shader->sunLight, a, shader->sunLight);

			token = COM_ParseExt( text, qfalse );
//			a = DEG2RAD(atof( token ));

			token = COM_ParseExt( text, qfalse );
//			b = DEG2RAD(atof( token ));

//			shader->sunDirection[0] = cos( a ) * cos( b );
//			shader->sunDirection[1] = sin( a ) * cos( b );
//			shader->sunDirection[2] = sin( b );
		}
		else if ( !Q_stricmp( token, "surfaceParm" ) ) 
		{
			SV_ParseSurfaceParm( shader, text );
			continue;
		}
		else if ( !Q_stricmp( token, "fogParms" ) ) 
		{
			vec3_t				fogColor;
			if ( !CM_ParseVector( shader, text, 3, fogColor ) ) 
			{
				return;
			}

			token = COM_ParseExt( text, qfalse );
			if ( !token[0] ) 
			{
				Com_Printf( S_COLOR_YELLOW "WARNING: missing parm for 'fogParms' keyword in shader '%s'\n", shader->shader );
				continue;
			}
//			shader->depthForOpaque = atof( token );

			// skip any old gradient directions
			SkipRestOfLine( (const char **)text );
			continue;
		}
	}
	return;
}

/*
=================
CM_SetupShaderProperties

  Scans thru the shaders loaded for the map, parses the text of that shader and
  extracts the interesting info *WITHOUT* loading up any images or requiring
  the renderer to be active.
=================
*/

void CM_SetupShaderProperties(void)
{
	int			i;
	const char	*def;
	CCMShader	*shader;

	// Add all basic shaders to the cmShaderTable
	for(i = 0; i < cmg.numShaders; i++)
	{
		cmShaderTable.insert(CM_GetShaderInfo(i));
	}
	// Go through and parse evaluate shader names to shadernums
	for(i = 0; i < cmg.numShaders; i++)
	{
		shader = CM_GetShaderInfo(i);
		def = CM_GetShaderText(shader->shader);
		if(def)
		{
			CM_ParseShader(shader, &def);
		}
	}
}

void CM_ShutdownShaderProperties(void)
{
	if(cmShaderTable.count())
	{
//		Com_Printf("Shutting down cmShaderTable .....\n");
		cmShaderTable.clear();
	}
}

CCMShader *CM_GetShaderInfo( const char *name )
{
	CCMShader	*out;
	const char	*def;

	out = cmShaderTable[name];
	if(out)
	{
		return(out);
	}

	// Create a new CCMShader class
	out = (CCMShader *)Hunk_Alloc( sizeof( CCMShader ), h_high );
	// Set defaults
	Q_strncpyz(out->shader, name, MAX_QPATH);
	out->contentFlags = CONTENTS_SOLID | CONTENTS_OPAQUE;

	// Parse in any text if it exists
	def = CM_GetShaderText(name);
	if(def)
	{
		CM_ParseShader(out, &def);
	}

	cmShaderTable.insert(out);
	return(out);
}

CCMShader *CM_GetShaderInfo( int shaderNum )
{
	CCMShader	*out;

	if((shaderNum < 0) || (shaderNum >= cmg.numShaders))
	{
		return(NULL);
	}
	out = cmg.shaders + shaderNum;
	return(out);
}

// end
