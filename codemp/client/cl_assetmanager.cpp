/*
===========================================================================
Copyright (C) 2026, OpenJK contributors

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

// cl_assetmanager.cpp -- what the map has put in it, where it comes from, packed (/assetmanager)
//
// Everything present on the map: the rpmodels, the rpeffects, the looping rpsounds, the shaders
// remapped and the overlays. Their lists are asked of the server through their managers, one a
// little after the other for the flood protection, then each is found: its file, and the pk3 the
// copy the game uses comes from, the base game's or a custom one.
//
// The custom ones checked are packed into a new pk3, with what they need that's custom too: a
// model's shaders and their images, its other LODs, a Ghoul2 model's skins and skeleton; an
// effect's models, sounds, shaders and the effects it plays; a shader's .shader file and images.
// It goes in assetpacks/ in the game folder, where the game doesn't load it by itself.

#include <map>
#include <set>
#include <string>
#include <zlib.h>
#include "client.h"
#include "ghoul2/G2.h"
#include "cl_widgets.h"

#define AS_CMD				"assetmanager"
#define AS_DIR				"assetpacks"
#define AS_MAX_ENTRIES		512
#define AS_MAX_FILES		4096
#define AS_STEP_MS			1300	// between the lists asked for, for the server's flood protection
#define AS_REPLY_MS			2500	// the last list's reply has this long to come
#define AS_MESSAGE_MS		8000

#define AS_TEXT_SCALE		0.75f
#define AS_ROW_H			12.0f
#define AS_CTRL_H			15.0f
#define AS_LIST_X			16.0f
#define AS_LIST_Y			60.0f
#define AS_LIST_W			( SCREEN_WIDTH - 32.0f )
#define AS_LIST_ROWS		28

typedef enum {
	AS_MODEL,
	AS_EFFECT,
	AS_SOUND,
	AS_REMAP,
	AS_OVERLAY,
	AS_NUM_KINDS
} asKind_t;

static const char *asKindNames[AS_NUM_KINDS] = { "Model", "Effect", "Sound", "Remap", "Overlay" };

typedef struct {
	int			kind;
	char		name[MAX_QPATH];	// as the server's list has it
	char		note[MAX_QPATH];	// a remap's shader it's over
	char		file[MAX_QPATH];	// the file it is, "" not found
	char		source[MAX_QPATH];	// where that is: "gamedir/name.pk3", "gamedir" a loose file
	qboolean	custom;				// not the base game's
	qboolean	checked;			// to go in the pk3
	int			count;				// how many on the map
} asEntry_t;

static asEntry_t	asEntries[AS_MAX_ENTRIES];
static int			asNumEntries;

// the pk3 being made: its files, each once
static char			asFiles[AS_MAX_FILES][MAX_QPATH];
static int			asNumFiles;

// shader name (lowercase) to the .shader file that has it
static std::map<std::string, std::string> asShaderFiles;
static qboolean		asShadersIndexed;
static std::set<std::string> asShadersDone;	// the pk3's shaders looked into already

static struct {
	qboolean	open;
	qhandle_t	font;
	float		cursorX, cursorY;
	qboolean	click;
	float		clickX, clickY;
	int			wheel;
	int			scroll;
	qboolean	customOnly;
	char		packName[48];
	qboolean	typing;
	int			scanStep;		// 0 not scanning; 1-4 the lists asked for one by one, 5 the replies awaited
	int			scanNext;
	char		message[256];
	int			messageTime;
	int			view[AS_MAX_ENTRIES];
	int			numView;
} as;

static vec4_t asWhite		= { 1.0f, 1.0f, 1.0f, 1.0f };
static vec4_t asPanel		= { 0.0f, 0.0f, 0.0f, 0.75f };
static vec4_t asPanelLight	= { 0.15f, 0.15f, 0.15f, 0.85f };
static vec4_t asHoverBox	= { 0.25f, 0.25f, 0.25f, 0.9f };
static vec4_t asFocus		= { 0.1f, 0.2f, 0.3f, 0.9f };
static vec4_t asBorder		= { 0.5f, 0.5f, 0.5f, 0.8f };
static vec4_t asDim			= { 0.65f, 0.65f, 0.65f, 1.0f };
static vec4_t asCustom		= { 0.45f, 0.9f, 0.5f, 1.0f };
static vec4_t asMissing		= { 1.0f, 0.5f, 0.4f, 1.0f };
static vec4_t asAccentFill	= { 0.45f, 0.35f, 0.1f, 0.9f };

static void AS_Message( const char *text ) {
	Q_strncpyz( as.message, text, sizeof( as.message ) );
	as.messageTime = cls.realtime;
	Com_Printf( "Asset manager: %s\n", text );
}

/*
===============================================================================

FINDING THEM

===============================================================================
*/

static qboolean AS_Exists( const char *path ) {
	return (qboolean)( path[0] && FS_ReadFile( path, NULL ) > 0 );
}

static qboolean AS_BaseSource( const char *source ) {
	return (qboolean)!Q_stricmpn( source, BASEGAME "/assets", strlen( BASEGAME "/assets" ) );
}

// a path as the game takes it: / not \, lowercase, no leading /
static void AS_Clean( const char *in, char *out, int size ) {
	int len = 0;

	while ( *in == '/' || *in == '\\' || *in == ' ' || *in == '"' )
		in++;
	for ( ; *in && *in != '"' && len < size - 1; in++ )
		out[len++] = *in == '\\' ? '/' : (char)tolower( (unsigned char)*in );
	while ( len > 0 && out[len - 1] == ' ' )
		len--;
	out[len] = '\0';
}

// the first of base + each extension that's a file; "" none
static qboolean AS_FindWith( const char *base, const char **exts, int numExts, char *out ) {
	char stripped[MAX_QPATH];

	if ( AS_Exists( base ) && strchr( COM_SkipPath( (char *)base ), '.' ) ) {
		Q_strncpyz( out, base, MAX_QPATH );
		return qtrue;
	}
	COM_StripExtension( base, stripped, sizeof( stripped ) );
	for ( int i = 0; i < numExts; i++ ) {
		const char *path = va( "%s%s", stripped, exts[i] );

		if ( AS_Exists( path ) ) {
			Q_strncpyz( out, path, MAX_QPATH );
			return qtrue;
		}
	}
	out[0] = '\0';
	return qfalse;
}

static const char *asImageExts[] = { ".jpg", ".tga", ".png" };
static const char *asModelExts[] = { ".md3", ".glm" };
static const char *asSoundExts[] = { ".wav", ".mp3", ".ogg" };
static const char *asEffectExts[] = { ".efx" };

// every shader in shaders/*.shader, and the file the copy in use is in
static void AS_IndexShaders( void ) {
	char **files;
	int n;

	asShaderFiles.clear();
	files = FS_ListFiles( "shaders", ".shader", &n );
	for ( int i = 0; i < n; i++ ) {
		const char *path = va( "shaders/%s", files[i] );
		char file[MAX_QPATH], *buf;
		const char *p;

		Q_strncpyz( file, path, sizeof( file ) );
		if ( FS_ReadFile( file, (void **)&buf ) <= 0 || !buf )
			continue;
		p = buf;
		COM_BeginParseSession( file );
		while ( 1 ) {
			const char *token = COM_ParseExt( &p, qtrue );
			char key[MAX_QPATH];

			if ( !token[0] )
				break;
			AS_Clean( token, key, sizeof( key ) );
			token = COM_ParseExt( &p, qtrue );
			if ( strcmp( token, "{" ) || !SkipBracedSection( &p, 1 ) )
				break;
			// the first file to have it is the one the renderer keeps
			if ( asShaderFiles.find( key ) == asShaderFiles.end() )
				asShaderFiles[key] = file;
		}
		FS_FreeFile( buf );
	}
	FS_FreeFileList( files );
	asShadersIndexed = qtrue;
}

static const char *AS_ShaderFile( const char *shader ) {
	char key[MAX_QPATH];
	std::map<std::string, std::string>::const_iterator it;

	if ( !asShadersIndexed )
		AS_IndexShaders();
	AS_Clean( shader, key, sizeof( key ) );
	COM_StripExtension( key, key, sizeof( key ) );
	it = asShaderFiles.find( key );
	return it == asShaderFiles.end() ? NULL : it->second.c_str();
}

// an entry's file, and where it comes from
static void AS_Resolve( asEntry_t *e ) {
	char name[MAX_QPATH];
	const char *source;

	AS_Clean( e->name, name, sizeof( name ) );
	e->file[0] = '\0';
	switch ( e->kind ) {
	case AS_MODEL:
		if ( !AS_FindWith( name, asModelExts, 2, e->file ) )
			AS_FindWith( va( "models/%s", name ), asModelExts, 2, e->file );
		break;
	case AS_EFFECT:
		if ( !AS_FindWith( name, asEffectExts, 1, e->file ) )
			AS_FindWith( va( "effects/%s", name ), asEffectExts, 1, e->file );
		break;
	case AS_SOUND:
		if ( !AS_FindWith( name, asSoundExts, 3, e->file ) )
			AS_FindWith( va( "sound/%s", name ), asSoundExts, 3, e->file );
		break;
	default: {
		const char *file = AS_ShaderFile( name );

		if ( file )
			Q_strncpyz( e->file, file, sizeof( e->file ) );
		else
			AS_FindWith( name, asImageExts, 3, e->file );
		break;
	}
	}
	source = e->file[0] ? FS_FileSource( e->file ) : NULL;
	Q_strncpyz( e->source, source ? source : "", sizeof( e->source ) );
	e->custom = (qboolean)( source && !AS_BaseSource( source ) );
}

static void AS_Add( int kind, const char *name, const char *note ) {
	asEntry_t *e;

	if ( !name || !name[0] )
		return;
	for ( int i = 0; i < asNumEntries; i++ ) {
		if ( asEntries[i].kind == kind && !Q_stricmp( asEntries[i].name, name ) && !Q_stricmp( asEntries[i].note, note ? note : "" ) ) {
			asEntries[i].count++;
			return;
		}
	}
	if ( asNumEntries >= AS_MAX_ENTRIES )
		return;
	e = &asEntries[asNumEntries++];
	memset( e, 0, sizeof( *e ) );
	e->kind = kind;
	e->count = 1;
	Q_strncpyz( e->name, name, sizeof( e->name ) );
	Q_strncpyz( e->note, note ? note : "", sizeof( e->note ) );
	AS_Resolve( e );
	e->checked = e->custom;
}

// what the managers' lists have now
static void AS_Gather( void ) {
	asNumEntries = 0;
	asShadersIndexed = qfalse;
	for ( int i = 0; i < CL_ModelManager_NumPlaced(); i++ )
		AS_Add( AS_MODEL, CL_ModelManager_PlacedName( i ), NULL );
	for ( int i = 0; i < CL_EffectManager_NumActive(); i++ ) {
		const char *path;
		vec3_t origin;
		int num;

		if ( CL_EffectManager_GetActive( i, &num, &path, origin ) )
			AS_Add( AS_EFFECT, path, NULL );
	}
	for ( int i = 0; i < CL_SoundManager_NumLoops(); i++ )
		AS_Add( AS_SOUND, CL_SoundManager_LoopPath( i ), NULL );
	for ( int i = 0; i < CL_ShaderManager_NumRemaps(); i++ )
		AS_Add( AS_REMAP, CL_ShaderManager_RemapTo( i ), CL_ShaderManager_RemapFrom( i ) );
	for ( int i = 0; i < CL_ShaderManager_NumOverlays(); i++ )
		AS_Add( AS_OVERLAY, CL_ShaderManager_OverlayShader( i ), NULL );
}

static int AS_NumCustom( void ) {
	int n = 0;

	for ( int i = 0; i < asNumEntries; i++ )
		n += asEntries[i].custom ? 1 : 0;
	return n;
}

// the lists asked of the server one by one, then what came back read
static void AS_StartScan( void ) {
	if ( cls.state != CA_ACTIVE ) {
		AS_Message( S_COLOR_YELLOW "Join a server first" );
		return;
	}
	as.scanStep = 1;
	as.scanNext = cls.realtime;
}

void CL_AssetManager_Frame( void ) {
	if ( !as.scanStep || cls.realtime < as.scanNext )
		return;
	if ( cls.state != CA_ACTIVE ) {
		as.scanStep = 0;
		return;
	}
	switch ( as.scanStep ) {
	case 1:		CL_ModelManager_RequestList(); break;
	case 2:		CL_EffectManager_RequestList(); break;
	case 3:		CL_SoundManager_RequestList(); break;
	case 4:		CL_ShaderManager_RequestOverlays(); break;
	default:
		AS_Gather();
		as.scanStep = 0;
		as.scroll = 0;
		AS_Message( va( "%i on the map, %i of them custom", asNumEntries, AS_NumCustom() ) );
		return;
	}
	as.scanStep++;
	as.scanNext = cls.realtime + ( as.scanStep == 5 ? AS_REPLY_MS : AS_STEP_MS );
}

/*
===============================================================================

WHAT THEY NEED

===============================================================================
*/

static void AS_AddFile( const char *path );
static void AS_AddAny( const char *token );

// an image by its name, whichever of its extensions there is
static void AS_AddImage( const char *name ) {
	char clean[MAX_QPATH], file[MAX_QPATH];

	AS_Clean( name, clean, sizeof( clean ) );
	if ( !clean[0] || clean[0] == '$' || clean[0] == '*' )
		return;
	if ( AS_FindWith( clean, asImageExts, 3, file ) )
		AS_AddFile( file );
}

// a shader: its .shader file and what its stages draw with, or the image of its name
static void AS_AddShader( const char *name ) {
	char clean[MAX_QPATH], *buf;
	const char *file, *p;

	AS_Clean( name, clean, sizeof( clean ) );
	COM_StripExtension( clean, clean, sizeof( clean ) );
	if ( !clean[0] )
		return;
	if ( !asShadersDone.insert( clean ).second )
		return;
	file = AS_ShaderFile( clean );
	if ( !file ) {
		AS_AddImage( clean );
		return;
	}
	{
		char shaderFile[MAX_QPATH];

		Q_strncpyz( shaderFile, file, sizeof( shaderFile ) );
		AS_AddFile( shaderFile );
		if ( FS_ReadFile( shaderFile, (void **)&buf ) <= 0 || !buf )
			return;
	}
	// its body: the images and the sky's sides it names
	p = buf;
	COM_BeginParseSession( clean );
	while ( 1 ) {
		const char *token = COM_ParseExt( &p, qtrue );
		char key[MAX_QPATH];

		if ( !token[0] )
			break;
		AS_Clean( token, key, sizeof( key ) );
		token = COM_ParseExt( &p, qtrue );
		if ( strcmp( token, "{" ) )
			break;
		if ( Q_stricmp( key, clean ) ) {
			SkipBracedSection( &p, 1 );
			continue;
		}
		for ( int depth = 1; depth > 0; ) {
			token = COM_ParseExt( &p, qtrue );
			if ( !token[0] )
				break;
			if ( !strcmp( token, "{" ) ) {
				depth++;
			} else if ( !strcmp( token, "}" ) ) {
				depth--;
			} else if ( !Q_stricmp( token, "skyParms" ) ) {
				static const char *sides[] = { "rt", "bk", "lf", "ft", "up", "dn" };
				char env[MAX_QPATH];

				Q_strncpyz( env, COM_ParseExt( &p, qfalse ), sizeof( env ) );
				for ( int s = 0; s < 6 && env[0] != '-'; s++ )
					AS_AddImage( va( "%s_%s", env, sides[s] ) );
			} else if ( strchr( token, '/' ) ) {
				// a stage's image (an image is often named as its shader), or a video
				char name[MAX_QPATH];

				AS_Clean( token, name, sizeof( name ) );
				if ( !Q_stricmp( COM_GetExtension( name ), "roq" ) && AS_Exists( name ) )
					AS_AddFile( name );
				else
					AS_AddImage( name );
			}
		}
		break;
	}
	FS_FreeFile( buf );
}

// an .md3's shaders, and its other LODs
static void AS_ScanMd3( const char *path ) {
	byte *buf;
	const int len = FS_ReadFile( path, (void **)&buf );
	const md3Header_t *header = (const md3Header_t *)buf;

	if ( len <= 0 || !buf )
		return;
	if ( len >= (int)sizeof( *header ) && LittleLong( header->ident ) == MD3_IDENT ) {
		int ofs = LittleLong( header->ofsSurfaces );

		for ( int i = 0; i < LittleLong( header->numSurfaces ); i++ ) {
			const md3Surface_t *s;

			if ( ofs < 0 || ofs > len - (int)sizeof( md3Surface_t ) )
				break;
			s = (const md3Surface_t *)( buf + ofs );
			for ( int j = 0; j < LittleLong( s->numShaders ); j++ ) {
				const int at = ofs + LittleLong( s->ofsShaders ) + j * (int)sizeof( md3Shader_t );

				if ( at < 0 || at > len - (int)sizeof( md3Shader_t ) )
					break;
				AS_AddShader( ( (const md3Shader_t *)( buf + at ) )->name );
			}
			if ( LittleLong( s->ofsEnd ) <= 0 )
				break;
			ofs += LittleLong( s->ofsEnd );
		}
	}
	FS_FreeFile( buf );
	// name_1.md3, name_2.md3
	{
		char base[MAX_QPATH];

		COM_StripExtension( path, base, sizeof( base ) );
		if ( strlen( base ) < 2 || base[strlen( base ) - 2] != '_' ) {
			for ( int lod = 1; lod <= 2; lod++ ) {
				const char *lodPath = va( "%s_%i.md3", base, lod );

				if ( AS_Exists( lodPath ) )
					AS_AddFile( lodPath );
			}
		}
	}
}

// a .glm's shaders, its skeleton, and the skins beside it
static void AS_ScanGlm( const char *path ) {
	byte *buf;
	const int len = FS_ReadFile( path, (void **)&buf );
	const mdxmHeader_t *header = (const mdxmHeader_t *)buf;
	char gla[MAX_QPATH], dir[MAX_QPATH], *slash;

	if ( len <= 0 || !buf )
		return;
	gla[0] = '\0';
	if ( len >= (int)sizeof( *header ) && LittleLong( header->ident ) == MDXM_IDENT ) {
		const int ofsTable = (int)sizeof( mdxmHeader_t );

		Q_strncpyz( gla, header->animName, sizeof( gla ) );
		for ( int i = 0; i < LittleLong( header->numSurfaces ); i++ ) {
			int at;

			if ( ofsTable + ( i + 1 ) * 4 > len )
				break;
			at = ofsTable + LittleLong( ( (const int *)( buf + ofsTable ) )[i] );
			if ( at < 0 || at > len - (int)sizeof( mdxmSurfHierarchy_t ) )
				break;
			AS_AddShader( ( (const mdxmSurfHierarchy_t *)( buf + at ) )->shader );
		}
	}
	FS_FreeFile( buf );
	if ( gla[0] )
		AS_AddFile( va( "%s.gla", gla ) );
	Q_strncpyz( dir, path, sizeof( dir ) );
	if ( ( slash = strrchr( dir, '/' ) ) != NULL ) {
		char **list;
		int n;

		*slash = '\0';
		list = FS_ListFiles( dir, ".skin", &n );
		for ( int i = 0; i < n; i++ )
			AS_AddFile( va( "%s/%s", dir, list[i] ) );
		FS_FreeFileList( list );
		// its animation list beside it
		if ( AS_Exists( va( "%s/animation.cfg", dir ) ) )
			AS_AddFile( va( "%s/animation.cfg", dir ) );
	}
}

// a .skin's shaders: "surface,shader" a line
static void AS_ScanSkin( const char *path ) {
	char *buf, *line, *next;

	if ( FS_ReadFile( path, (void **)&buf ) <= 0 || !buf )
		return;
	for ( line = buf; line && *line; line = next ) {
		char *comma;

		next = strchr( line, '\n' );
		if ( next )
			*next++ = '\0';
		comma = strchr( line, ',' );
		if ( comma ) {
			char *end = comma + strlen( comma );

			while ( end > comma && ( end[-1] == '\r' || end[-1] == ' ' ) )
				*--end = '\0';
			if ( Q_stricmp( comma + 1, "*off" ) )
				AS_AddShader( comma + 1 );
		}
	}
	FS_FreeFile( buf );
}

// an effect: every name in it that's a path
static void AS_ScanText( const char *path ) {
	char *buf;
	const char *p;

	if ( FS_ReadFile( path, (void **)&buf ) <= 0 || !buf )
		return;
	p = buf;
	COM_BeginParseSession( path );
	while ( 1 ) {
		const char *token = COM_ParseExt( &p, qtrue );

		if ( !token[0] )
			break;
		if ( strchr( token, '/' ) || strchr( token, '\\' ) ) {
			char keep[MAX_QPATH];

			Q_strncpyz( keep, token, sizeof( keep ) );
			AS_AddAny( keep );
		}
	}
	FS_FreeFile( buf );
}

// a name found in an effect or a shader: whatever it turns out to be
static void AS_AddAny( const char *token ) {
	char clean[MAX_QPATH], file[MAX_QPATH];
	const char *ext;

	AS_Clean( token, clean, sizeof( clean ) );
	if ( !clean[0] )
		return;
	ext = COM_GetExtension( clean );
	if ( !Q_stricmp( ext, "jpg" ) || !Q_stricmp( ext, "tga" ) || !Q_stricmp( ext, "png" ) ) {
		AS_AddImage( clean );
		return;
	}
	if ( ext[0] && AS_Exists( clean ) ) {
		AS_AddFile( clean );
		return;
	}
	if ( AS_ShaderFile( clean ) ) {
		AS_AddShader( clean );
		return;
	}
	if ( AS_FindWith( clean, asImageExts, 3, file ) || AS_FindWith( clean, asModelExts, 2, file ) || AS_FindWith( clean, asSoundExts, 3, file )
		|| AS_FindWith( clean, asEffectExts, 1, file ) || AS_FindWith( va( "effects/%s", clean ), asEffectExts, 1, file ) )
		AS_AddFile( file );
}

// a file into the pk3 when it's custom, then what it needs
static void AS_AddFile( const char *path ) {
	char clean[MAX_QPATH];
	const char *source, *ext;

	AS_Clean( path, clean, sizeof( clean ) );
	if ( !clean[0] || asNumFiles >= AS_MAX_FILES )
		return;
	for ( int i = 0; i < asNumFiles; i++ ) {
		if ( !Q_stricmp( asFiles[i], clean ) )
			return;
	}
	source = FS_FileSource( clean );
	if ( !source || AS_BaseSource( source ) )
		return;
	Q_strncpyz( asFiles[asNumFiles++], clean, sizeof( asFiles[0] ) );
	ext = COM_GetExtension( clean );
	if ( !Q_stricmp( ext, "md3" ) )
		AS_ScanMd3( clean );
	else if ( !Q_stricmp( ext, "glm" ) )
		AS_ScanGlm( clean );
	else if ( !Q_stricmp( ext, "skin" ) )
		AS_ScanSkin( clean );
	else if ( !Q_stricmp( ext, "efx" ) )
		AS_ScanText( clean );
}

/*
===============================================================================

THE PK3

===============================================================================
*/

typedef struct {
	unsigned	crc, csize, usize, offset;
	int			method;
} asZipEntry_t;

// zlib's memory, the game's: its own isn't built in
static voidpf AS_ZAlloc( voidpf opaque, uInt items, uInt size ) {
	return Z_Malloc( items * size, TAG_TEMP_WORKSPACE, qfalse );
}

static void AS_ZFree( voidpf opaque, voidpf address ) {
	Z_Free( address );
}

static void AS_Put16( fileHandle_t f, int v ) {
	byte b[2] = { (byte)( v & 0xff ), (byte)( ( v >> 8 ) & 0xff ) };

	FS_Write( b, 2, f );
}

static void AS_Put32( fileHandle_t f, unsigned v ) {
	byte b[4] = { (byte)( v & 0xff ), (byte)( ( v >> 8 ) & 0xff ), (byte)( ( v >> 16 ) & 0xff ), (byte)( ( v >> 24 ) & 0xff ) };

	FS_Write( b, 4, f );
}

// the checked ones and what they need, into assetpacks/<name>.pk3
static void AS_Build( void ) {
	static asZipEntry_t entries[AS_MAX_FILES];
	char path[MAX_QPATH];
	unsigned offset = 0, cdStart, cdSize = 0, total = 0;
	int dosTime, dosDate, written = 0, checked = 0;
	fileHandle_t f;
	qtime_t t;

	if ( !as.packName[0] ) {
		AS_Message( S_COLOR_YELLOW "Name the pk3 first" );
		return;
	}
	for ( const char *c = as.packName; *c; c++ ) {
		if ( !isalnum( (unsigned char)*c ) && *c != '_' && *c != '-' ) {
			AS_Message( S_COLOR_YELLOW "A pk3's name takes letters, digits, _ and -" );
			return;
		}
	}
	asNumFiles = 0;
	asShadersIndexed = qfalse;
	asShadersDone.clear();
	for ( int i = 0; i < asNumEntries; i++ ) {
		const asEntry_t *e = &asEntries[i];

		if ( !e->checked || !e->file[0] )
			continue;
		checked++;
		if ( e->kind == AS_REMAP || e->kind == AS_OVERLAY )
			AS_AddShader( e->name );
		else
			AS_AddFile( e->file );
	}
	if ( !asNumFiles ) {
		AS_Message( S_COLOR_YELLOW "Nothing custom checked to pack" );
		return;
	}

	Com_sprintf( path, sizeof( path ), AS_DIR "/%s.pk3", as.packName );
	// the game won't write a .pk3 by itself (a download can't plant one): this one's asked for, in a
	// folder the game doesn't load from
	f = FS_FOpenFileWrite( path, qfalse );
	if ( !f ) {
		AS_Message( va( S_COLOR_RED "Couldn't write %s", path ) );
		return;
	}
	Com_RealTime( &t );
	dosTime = ( t.tm_hour << 11 ) | ( t.tm_min << 5 ) | ( t.tm_sec / 2 );
	dosDate = ( Q_max( 0, t.tm_year + 1900 - 1980 ) << 9 ) | ( ( t.tm_mon + 1 ) << 5 ) | t.tm_mday;

	for ( int i = 0; i < asNumFiles; i++ ) {
		asZipEntry_t *z = &entries[written];
		const int nameLen = strlen( asFiles[i] );
		byte *buf, *out = NULL;
		const int len = FS_ReadFile( asFiles[i], (void **)&buf );
		const byte *data;

		if ( len < 0 || !buf )
			continue;
		z->crc = crc32( 0L, buf, len );
		z->usize = len;
		z->method = 0;
		z->csize = len;
		data = buf;
		// deflated when it's smaller so
		if ( len > 0 ) {
			z_stream zs;
			const uLong bound = compressBound( len ) + 64;

			memset( &zs, 0, sizeof( zs ) );
			zs.zalloc = AS_ZAlloc;
			zs.zfree = AS_ZFree;
			out = (byte *)Z_Malloc( bound, TAG_TEMP_WORKSPACE, qfalse );
			if ( deflateInit2( &zs, Z_BEST_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY ) == Z_OK ) {
				zs.next_in = buf;
				zs.avail_in = len;
				zs.next_out = out;
				zs.avail_out = bound;
				if ( deflate( &zs, Z_FINISH ) == Z_STREAM_END && zs.total_out < (uLong)len ) {
					z->method = 8;
					z->csize = zs.total_out;
					data = out;
				}
				deflateEnd( &zs );
			}
		}
		z->offset = offset;
		AS_Put32( f, 0x04034b50 );
		AS_Put16( f, 20 );
		AS_Put16( f, 0 );
		AS_Put16( f, z->method );
		AS_Put16( f, dosTime );
		AS_Put16( f, dosDate );
		AS_Put32( f, z->crc );
		AS_Put32( f, z->csize );
		AS_Put32( f, z->usize );
		AS_Put16( f, nameLen );
		AS_Put16( f, 0 );
		FS_Write( asFiles[i], nameLen, f );
		FS_Write( data, z->csize, f );
		offset += 30 + nameLen + z->csize;
		total += z->usize;
		if ( out )
			Z_Free( out );
		FS_FreeFile( buf );
		// its name kept for the directory
		Q_strncpyz( asFiles[written], asFiles[i], sizeof( asFiles[0] ) );
		written++;
	}
	cdStart = offset;
	for ( int i = 0; i < written; i++ ) {
		const asZipEntry_t *z = &entries[i];
		const int nameLen = strlen( asFiles[i] );

		AS_Put32( f, 0x02014b50 );
		AS_Put16( f, 20 );
		AS_Put16( f, 20 );
		AS_Put16( f, 0 );
		AS_Put16( f, z->method );
		AS_Put16( f, dosTime );
		AS_Put16( f, dosDate );
		AS_Put32( f, z->crc );
		AS_Put32( f, z->csize );
		AS_Put32( f, z->usize );
		AS_Put16( f, nameLen );
		AS_Put16( f, 0 );
		AS_Put16( f, 0 );
		AS_Put16( f, 0 );
		AS_Put16( f, 0 );
		AS_Put32( f, 0 );
		AS_Put32( f, z->offset );
		FS_Write( asFiles[i], nameLen, f );
		cdSize += 46 + nameLen;
	}
	AS_Put32( f, 0x06054b50 );
	AS_Put16( f, 0 );
	AS_Put16( f, 0 );
	AS_Put16( f, written );
	AS_Put16( f, written );
	AS_Put32( f, cdSize );
	AS_Put32( f, cdStart );
	AS_Put16( f, 0 );
	FS_FCloseFile( f );
	asNumFiles = written;
	AS_Message( va( S_COLOR_GREEN "Built %s/%s: %i files from %i checked, %.1f MB, %.1f MB packed. Move it into the game folder to load it",
		FS_GetCurrentGameDir(), path, written, checked, total / 1048576.0f, ( offset + cdSize + 22 ) / 1048576.0f ) );
}

/*
===============================================================================

OPENING AND INPUT

===============================================================================
*/

static void AS_Close( void ) {
	if ( !as.open )
		return;
	as.open = qfalse;
	as.typing = qfalse;
	Key_SetCatcher( Key_GetCatcher() & ~KEYCATCH_ASSETMANAGER );
}

void CL_AssetManager_Close( void ) {
	AS_Close();
}

static void AS_Open( void ) {
	if ( cls.state != CA_ACTIVE || !cls.cgameStarted ) {
		Com_Printf( "Asset manager: join a server first\n" );
		return;
	}
	if ( as.open )
		return;
	CL_ModelManager_Close();
	CL_NpcManager_Close();
	CL_ShaderManager_Close();
	CL_EffectManager_Close();
	CL_Storyteller_Close();
	CL_RpCinematic_EditorClose();
	CL_MapEditor_Close();
	CL_SoundManager_Close();
	CL_AnimManager_Close();
	CL_Presets_Close();

	as.font = re->RegisterFont( "arialnb" );
	if ( !as.font )
		as.font = cls.menuFont;
	as.open = qtrue;
	as.cursorX = SCREEN_WIDTH * 0.5f;
	as.cursorY = SCREEN_HEIGHT * 0.5f;
	as.click = qfalse;
	as.wheel = 0;
	if ( !as.packName[0] ) {
		char map[MAX_QPATH];

		COM_StripExtension( COM_SkipPath( cl.mapname ), map, sizeof( map ) );
		Com_sprintf( as.packName, sizeof( as.packName ), "assets_%s", map[0] ? map : "map" );
	}
	Key_SetCatcher( Key_GetCatcher() | KEYCATCH_ASSETMANAGER );
	AS_StartScan();
}

static void AS_Cmd_f( void ) {
	const char *arg = Cmd_Argv( 1 );

	if ( !arg[0] ) {
		if ( as.open )
			AS_Close();
		else
			AS_Open();
	} else if ( !Q_stricmp( arg, "close" ) ) {
		AS_Close();
	} else if ( !Q_stricmp( arg, "scan" ) ) {
		AS_StartScan();
	} else if ( !Q_stricmp( arg, "list" ) ) {
		for ( int i = 0; i < asNumEntries; i++ ) {
			const asEntry_t *e = &asEntries[i];

			Com_Printf( "  %-7s %s%s%s  %s%s\n", asKindNames[e->kind], e->name, e->note[0] ? " over " : "", e->note,
				e->custom ? S_COLOR_GREEN : S_COLOR_GREY, e->file[0] ? ( e->source[0] ? e->source : "?" ) : S_COLOR_RED "not found" );
		}
		Com_Printf( "%i, %i custom (scan first: " AS_CMD " scan)\n", asNumEntries, AS_NumCustom() );
	} else if ( !Q_stricmp( arg, "build" ) ) {
		if ( Cmd_Argv( 2 )[0] )
			Q_strncpyz( as.packName, Cmd_Argv( 2 ), sizeof( as.packName ) );
		AS_Build();
	} else {
		Com_Printf( "usage: " AS_CMD "              open or close it: what's on the map, and where it comes from\n" );
		Com_Printf( "       " AS_CMD " scan         ask the server what's on the map again\n" );
		Com_Printf( "       " AS_CMD " list         what was found, and where from\n" );
		Com_Printf( "       " AS_CMD " build [name] pack the custom ones checked into " AS_DIR "/<name>.pk3\n" );
	}
}

void CL_AssetManager_Init( void ) {
	memset( &as, 0, sizeof( as ) );
	as.customOnly = qtrue;
	Cmd_AddCommand( AS_CMD, AS_Cmd_f, "What's on the map (rpmodels, rpeffects, rpsounds, remaps, overlays), the pk3 each is from, packed into a new pk3" );
}

void CL_AssetManager_Shutdown( void ) {
	AS_Close();
	as.scanStep = 0;
	asShaderFiles.clear();
	asShadersIndexed = qfalse;
}

void CL_AssetManager_KeyEvent( int key, qboolean down ) {
	if ( !as.open || !down )
		return;
	switch ( key ) {
	case A_MOUSE1:
		as.click = qtrue;
		as.clickX = as.cursorX;
		as.clickY = as.cursorY;
		return;
	case A_MWHEELUP:
		as.wheel -= 3;
		return;
	case A_MWHEELDOWN:
		as.wheel += 3;
		return;
	default:
		break;
	}
	if ( as.typing ) {
		if ( key == A_ENTER || key == A_KP_ENTER )
			as.typing = qfalse;
		else
			WG_EditKey( as.packName, sizeof( as.packName ), key, NULL );
		return;
	}
	{
		const char *binding = Key_GetBinding( key );

		if ( VALIDSTRING( binding ) && !Q_stricmp( binding, AS_CMD ) )
			AS_Close();
	}
}

void CL_AssetManager_CharEvent( int ch ) {
	if ( as.open && as.typing )
		WG_EditChar( as.packName, sizeof( as.packName ), ch, NULL );
}

void CL_AssetManager_MouseEvent( int dx, int dy ) {
	as.cursorX = Com_Clamp( 0, SCREEN_WIDTH, as.cursorX + dx );
	as.cursorY = Com_Clamp( 0, SCREEN_HEIGHT, as.cursorY + dy );
}

void CL_AssetManager_Escape( void ) {
	if ( as.typing ) {
		as.typing = qfalse;
		return;
	}
	AS_Close();
}

/*
===============================================================================

DRAWING

===============================================================================
*/

static void AS_Fill( float x, float y, float w, float h, const float *color ) {
	re->SetColor( color );
	re->DrawStretchPic( x, y, w, h, 0, 0, 0, 0, cls.whiteShader );
	re->SetColor( NULL );
}

static void AS_Box( float x, float y, float w, float h, const float *fill ) {
	AS_Fill( x, y, w, h, fill );
	AS_Fill( x, y, w, 1, asBorder );
	AS_Fill( x, y + h - 1, w, 1, asBorder );
	AS_Fill( x, y, 1, h, asBorder );
	AS_Fill( x + w - 1, y, 1, h, asBorder );
}

static float AS_TextWidth( const char *text ) {
	return (float)re->Font_StrLenPixels( text, as.font, AS_TEXT_SCALE );
}

static void AS_Text( float x, float y, float w, const char *text, const float *color ) {
	char buf[MAX_STRING_CHARS];
	int len;

	Q_strncpyz( buf, text, sizeof( buf ) );
	len = strlen( buf );
	while ( len > 3 && AS_TextWidth( buf ) > w ) {
		len--;
		buf[len - 3] = buf[len - 2] = buf[len - 1] = '.';
		buf[len] = '\0';
	}
	re->Font_DrawString( (int)x, (int)y, buf, color, as.font | STYLE_DROPSHADOW, -1, AS_TEXT_SCALE );
}

static qboolean AS_InRect( float x, float y, float w, float h ) {
	return (qboolean)( as.cursorX >= x && as.cursorX < x + w && as.cursorY >= y && as.cursorY < y + h );
}

static qboolean AS_Clicked( float x, float y, float w, float h ) {
	if ( !as.click || as.clickX < x || as.clickX >= x + w || as.clickY < y || as.clickY >= y + h )
		return qfalse;
	as.click = qfalse;
	return qtrue;
}

static qboolean AS_Button( float x, float y, float w, const char *label, const float *fill, qboolean enabled ) {
	AS_Box( x, y, w, AS_CTRL_H, enabled && AS_InRect( x, y, w, AS_CTRL_H ) ? asHoverBox : fill );
	AS_Text( x + ( w - AS_TextWidth( label ) ) * 0.5f, y + 2, w, label, enabled ? asWhite : asDim );
	return (qboolean)( AS_Clicked( x, y, w, AS_CTRL_H ) && enabled );
}

// a source as it reads: the pk3's name, or the base game
static const char *AS_SourceText( const asEntry_t *e ) {
	if ( !e->file[0] )
		return S_COLOR_RED "not found here";
	if ( !e->source[0] )
		return "?";
	if ( !e->custom )
		return va( S_COLOR_GREY "base game (%s)", COM_SkipPath( (char *)e->source ) );
	if ( !strchr( e->source, '/' ) )
		return va( "a loose file in %s", e->source );
	return e->source;
}

void CL_AssetManager_Draw( void ) {
	const float colKind = AS_LIST_X + 22, colName = AS_LIST_X + 74, colSource = AS_LIST_X + 380, listH = AS_LIST_ROWS * AS_ROW_H + 2;
	float rowW = AS_LIST_W;
	int checked = 0;
	wgStyle_t style;

	if ( !as.open )
		return;
	AS_Fill( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, asPanel );
	re->Font_DrawString( 16, 12, "Asset manager", asWhite, as.font | STYLE_DROPSHADOW, -1, 1.0f );
	AS_Text( 130, 15, 300, as.scanStep ? va( S_COLOR_YELLOW "Asking the server what's on the map (%i of 4)...", Q_min( 4, as.scanStep ) )
		: va( "%i on the map, %i custom", asNumEntries, AS_NumCustom() ), asDim );
	if ( AS_Button( SCREEN_WIDTH - 16 - 60, 12, 60, "Close", asPanelLight, qtrue ) ) {
		AS_Close();
		return;
	}
	if ( AS_Button( SCREEN_WIDTH - 16 - 60 - 6 - 100, 12, 100, "Scan the map", asPanelLight, (qboolean)!as.scanStep ) )
		AS_StartScan();

	// what's shown, and what's checked
	if ( AS_Button( 16, 36, 90, "Custom only", as.customOnly ? asAccentFill : asPanelLight, qtrue ) ) {
		as.customOnly = (qboolean)!as.customOnly;
		as.scroll = 0;
	}
	if ( AS_Button( 110, 36, 70, "Check all", asPanelLight, qtrue ) ) {
		for ( int i = 0; i < asNumEntries; i++ )
			asEntries[i].checked = asEntries[i].custom;
	}
	if ( AS_Button( 184, 36, 70, "None", asPanelLight, qtrue ) ) {
		for ( int i = 0; i < asNumEntries; i++ )
			asEntries[i].checked = qfalse;
	}
	AS_Text( 262, 38, SCREEN_WIDTH - 280, S_COLOR_GREY "Green: from a custom pk3, packable. Click one to check it or not", asWhite );

	as.numView = 0;
	for ( int i = 0; i < asNumEntries; i++ ) {
		if ( !as.customOnly || asEntries[i].custom )
			as.view[as.numView++] = i;
		checked += asEntries[i].checked ? 1 : 0;
	}

	// the list
	AS_Box( AS_LIST_X, AS_LIST_Y, AS_LIST_W, listH, asPanelLight );
	if ( as.wheel && AS_InRect( AS_LIST_X, AS_LIST_Y, AS_LIST_W, listH ) )
		as.scroll += as.wheel;
	as.wheel = 0;
	as.scroll = Com_Clampi( 0, Q_max( 0, as.numView - AS_LIST_ROWS ), as.scroll );
	if ( WG_Scrollbar( AS_LIST_X + AS_LIST_W - WG_SCROLL_W - 1, AS_LIST_Y + 1, AS_LIST_ROWS * AS_ROW_H, as.numView, AS_LIST_ROWS, &as.scroll,
		as.cursorX, as.cursorY, &as.click, as.clickX, as.clickY ) )
		rowW -= WG_SCROLL_W + 1;
	for ( int r = 0; r < AS_LIST_ROWS && as.scroll + r < as.numView; r++ ) {
		asEntry_t *e = &asEntries[as.view[as.scroll + r]];
		const float y = AS_LIST_Y + 1 + r * AS_ROW_H;
		const char *name = e->note[0] ? va( "%s  " S_COLOR_GREY "over %s", e->name, e->note ) : e->name;

		if ( AS_InRect( AS_LIST_X, y, rowW, AS_ROW_H ) )
			AS_Fill( AS_LIST_X + 1, y, rowW - 2, AS_ROW_H, asHoverBox );
		if ( e->custom ) {
			AS_Box( AS_LIST_X + 5, y + 2, 9, 9, asPanelLight );
			if ( e->checked )
				AS_Fill( AS_LIST_X + 7, y + 4, 5, 5, asCustom );
		}
		AS_Text( colKind, y, 50, asKindNames[e->kind], asDim );
		AS_Text( colName, y, colSource - colName - 8, e->count > 1 ? va( "%s " S_COLOR_GREY "x%i", name, e->count ) : name, e->custom ? asCustom : asWhite );
		AS_Text( colSource, y, AS_LIST_X + rowW - colSource - 4, AS_SourceText( e ), e->file[0] ? asWhite : asMissing );
		if ( AS_Clicked( AS_LIST_X, y, rowW, AS_ROW_H ) && e->custom )
			e->checked = (qboolean)!e->checked;
	}
	if ( !as.numView && !as.scanStep )
		AS_Text( AS_LIST_X + 8, AS_LIST_Y + 8, AS_LIST_W - 16, asNumEntries ? "Nothing custom: untick Custom only to see the rest"
			: "Nothing placed on the map that this client heard of. Scan the map asks the server", asDim );

	// the pk3
	{
		const float y = AS_LIST_Y + listH + 8;

		AS_Text( 16, y + 2, 70, "New pk3", asDim );
		style.font = as.font;
		style.scale = AS_TEXT_SCALE;
		style.lineH = AS_ROW_H;
		style.padX = 4.0f;
		style.padY = 2.0f;
		style.fill = asFocus;
		style.border = asBorder;
		style.text = asWhite;
		style.dim = asDim;
		AS_Box( 70, y, 170, AS_CTRL_H, as.typing ? asFocus : asPanelLight );
		WG_Field( &style, 70, y, 170, AS_CTRL_H, as.packName, as.typing, "its name", as.cursorX, as.cursorY );
		if ( AS_Clicked( 70, y, 170, WG_FieldHeight( &style, 170, AS_CTRL_H, as.packName, as.typing ) ) ) {
			if ( !as.typing )
				WG_EditReset();
			as.typing = qtrue;
			WG_FieldClick( as.packName, as.clickX, as.clickY );
		}
		AS_Text( 244, y + 2, 40, ".pk3", asDim );
		if ( AS_Button( 276, y, 170, va( "Build it from the %i checked", checked ), asPanelLight, (qboolean)( checked > 0 && !as.scanStep ) ) )
			AS_Build();
		AS_Text( 452, y + 2, SCREEN_WIDTH - 468, S_COLOR_GREY "with what they need: shaders, images, skins...", asWhite );
	}
	if ( as.message[0] && cls.realtime - as.messageTime < AS_MESSAGE_MS )
		AS_Text( 16, 456, SCREEN_WIDTH - 32, as.message, asWhite );
	if ( as.click )
		as.typing = qfalse;
	as.click = qfalse;
	WG_DrawOverlay();
	re->SetColor( NULL );
	re->DrawStretchPic( as.cursorX, as.cursorY, 32, 32, 0, 0, 1, 1, cls.cursorShader );
}
