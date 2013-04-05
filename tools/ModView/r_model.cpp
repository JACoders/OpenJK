// Filename:-	R_Model.cpp
//
// interface to all block-paste other-format code that doesn't know it's inside ModView
//

#include "stdafx.h"
#include "includes.h"
#include "R_Common.h"
#include "textures.h"
//
#include "R_Model.h"


trGlobals_t tr;
refimport_t ri;


void Com_Printf( const char *format, ... )
{
	va_list		argptr;
	static char		string[16][1024];
	static int index = 0;

	index = (++index)&15;
	
	va_start (argptr, format);
	vsprintf (string[index], format,argptr);
	va_end (argptr);

	OutputDebugString(string[index]);	
//	assert(0);
	ErrorBox(string[index]);
}

void Q_strncpyz( char *dest, LPCSTR src, int destlen)
{
	strncpy(dest,src,destlen);
	dest[destlen-1] = '\0';
}

float Com_Clamp( float min, float max, float value ) {
	if ( value < min ) {
		return min;
	}
	if ( value > max ) {
		return max;
	}
	return value;
}


/*
============
COM_SkipPath
============
*/
char *COM_SkipPath (char *pathname)
{
	char	*last;
	
	last = pathname;
	while (*pathname)
	{
		if (*pathname=='/')
			last = pathname+1;
		pathname++;
	}
	return last;
}

/*
============
COM_StripExtension
============
*/
void COM_StripExtension( const char *in, char *out ) {
	while ( *in && *in != '.' ) {
		*out++ = *in++;
	}
	*out = 0;
}


void QDECL Com_sprintf( char *dest, int size, const char *fmt, ...) {
	int		len;
	va_list		argptr;
	char	bigbuffer[32000];	// big, but small enough to fit in PPC stack

	va_start (argptr,fmt);
	len = vsprintf (bigbuffer,fmt,argptr);
	va_end (argptr);
	if ( len >= sizeof( bigbuffer ) ) {
		Com_Error( ERR_FATAL, "Com_sprintf: overflowed bigbuffer" );
	}
	if (len >= size) {
		Com_Printf ("Com_sprintf: overflow of %i in %i ( string: \"%s\" )\n", len, size, bigbuffer);
	}
	Q_strncpyz (dest, bigbuffer, size );
}

int    LongSwap (int l)
{
	byte    b1,b2,b3,b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}



/*
==================
COM_DefaultExtension
==================
*/
void COM_DefaultExtension (char *path, int maxSize, const char *extension ) {
	char	oldPath[MAX_QPATH];
	char    *src;

//
// if path doesn't have a .EXT, append extension
// (extension should include the .)
//
	src = path + strlen(path) - 1;

	while (*src != '/' && src != path) {
		if ( *src == '.' ) {
			return;                 // it has an extension
		}
		src--;
	}

	Q_strncpyz( oldPath, path, sizeof( oldPath ) );
	Com_sprintf( path, maxSize, "%s%s", oldPath, extension );
}


void Crap_Printf( int printLevel, const char *format, ...)
{
	va_list		argptr;
	static char		string[16][16384];
	static int index = 0;

	index = (++index)&15;
	
	va_start (argptr, format);
//	vsprintf (string[index], format,argptr);
	_vsnprintf(string[index], sizeof(string[0]), format, argptr);
	va_end (argptr);
	string[index][sizeof(string[0])-1] = '\0';

	char *psString = string[index];	

	switch (printLevel)
	{
		case PRINT_ALL:			

			InfoBox(psString);
			break;

		case PRINT_DEVELOPER:	
			
			OutputDebugString( psString);	
			break;

		case PRINT_WARNING:

			WarningBox(psString);
			break;

		default: assert(0);
		case PRINT_ERROR:

			ErrorBox(psString);
			break;
	}
}

void Crap_Error( int errorLevel, const char *format, ...)
{
	va_list		argptr;
	static char	string[1024];
	
	va_start (argptr, format);
	vsprintf (string, format,argptr);
	va_end (argptr);

	// should maybe switch-case off these, but for now...
	//
/*
typedef enum {
	ERR_FATAL,					// exit the entire game with a popup window
	ERR_DROP,					// print to console and disconnect from game
	ERR_DISCONNECT,				// don't kill server
	ERR_NEED_CD					// pop up the need-cd dialog
} errorParm_t;
*/

	throw (string );
}

void *Crap_Malloc( int bytes )
{
	void *pvBlah = malloc(bytes);
	if (pvBlah)
	{
		memset(pvBlah,0,bytes);
	}
	else
	{
		ErrorBox(va("Dammit: Failed to allocate %d bytes for some reason\n\nGonna have to exit....",bytes));
		exit(1);
	}

	return pvBlah;
}

void Crap_Free( void *buf )
{
	free(buf);
}

// change this?  Nah....
void *Crap_HunkAlloc(int size)
{
	return Crap_Malloc(size);
}



// ============
// FS_ReadFile
// 
// Filename are relative to the quake search path
// a null buffer will just return the file length without loading
// ============
bool bHackToAllowFullPathDuringTestFunc = false;	// leave as false!!!!!!!!
int Crap_FS_ReadFile( const char *qpath, void **buffer )
{
	byte*	buf = NULL;

	if ( !gamedir[0]) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( !qpath || !qpath[0] ) {
		Com_Error( ERR_FATAL, "FS_ReadFile with empty name\n" );
	}

	char sTemp[1024];
	sprintf(sTemp,"%s%s",bHackToAllowFullPathDuringTestFunc?"":gamedir,qpath);

	if (!FileExists(sTemp))
	{
		if ( buffer ) {
			*buffer = NULL;
		}
		return -1;
	}
	
	if ( !buffer ) {
		return FileLen( sTemp );
	}

	return LoadFile (sTemp, buffer);
}

// ============
// FS_ReadFile
// 
// Filename are relative to the quake search path
// a null buffer will just return the file length without loading
// ============
// -1 return = fail, else len written

int Crap_FS_WriteFile( const char *qpath, const void *pBuffer, int iSize )
{
	if ( !gamedir[0]) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( !qpath || !qpath[0] ) {
		Com_Error( ERR_FATAL, "FS_WriteFile with empty name\n" );
	}

	char sTemp[1024];
	sprintf(sTemp,"%s%s",gamedir,qpath);

	return SaveFile (sTemp, pBuffer, iSize);
}


void Crap_FS_FreeFile( void *buffer ) 
{		
	if ( !buffer ) {
		Com_Error( ERR_FATAL, "FS_FreeFile( NULL )" );
	}

	free(buffer);
}




int R_ComputeLOD( trRefEntity_t *ent )
{
	return AppVars.iLOD;	// MODVIEW change: this is under manual/user control in the viewer)
}


void AxisClear( vec3_t axis[3] ) {
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


/*

the drawsurf sort data is packed into a single 32 bit value so it can be
compared quickly during the qsorting process

the bits are allocated as follows:

22 - 31	: sorted shader index
12 - 21	: entity index
3 - 7	: fog index
2		: used to be clipped flag
0 - 1	: dlightmap index
*/
#define	QSORT_SHADERNUM_SHIFT	22
#define	QSORT_ENTITYNUM_SHIFT	12
#define	QSORT_FOGNUM_SHIFT		3
//
void R_AddDrawSurf( surfaceType_t *surface, GLuint gluiTextureBind)
{
	// instead of checking for overflow, we just mask the index
	// so it wraps around

	int index = tr.refdef.numDrawSurfs & DRAWSURF_MASK;

	tr.refdef.drawSurfs[index].sort = (gluiTextureBind/*shader->sortedIndex*/ << QSORT_SHADERNUM_SHIFT)
									| (tr.currentEntityNum		<< QSORT_ENTITYNUM_SHIFT)
									| (0/* fogIndex*/			<< QSORT_FOGNUM_SHIFT)
									|  0/*(int)dlightMap*/;

	tr.refdef.drawSurfs[index].surface = surface;
	tr.refdef.numDrawSurfs++;
}




void R_DecomposeSort( unsigned sort, int *entityNum, GLuint* gluiTextureBind
					 // MODVIEWREM //,shader_t **shader, int *fogNum, int *dlightMap 
					 )
{
//	*fogNum = ( sort >> QSORT_FOGNUM_SHIFT ) & 31;
//	*shader = tr.sortedShaders[ ( sort >> QSORT_SHADERNUM_SHIFT ) & (MAX_SHADERS-1) ];
	*gluiTextureBind = ( sort >> QSORT_SHADERNUM_SHIFT );
	*entityNum = ( sort >> QSORT_ENTITYNUM_SHIFT ) & 1023;
//	*dlightMap = sort & 3;
}




//model_t	*loadmodel;

static model_t *R_AllocModel( void ) 
{	
	model_t		*mod;

	if ( tr.numModels == MAX_MOD_KNOWN ) {
		return NULL;
	}

	mod = (model_s *)ri.Hunk_Alloc( sizeof( *tr.models[tr.numModels] ) );	
	mod->index = tr.numModels;
	tr.models[tr.numModels] = mod;
	tr.numModels++;

	return mod;
}


static void RE_EnsureOneBadModel(void)
{
	if (tr.numModels == 0)
	{
		model_t *mod = R_AllocModel();
		mod->type = MOD_BAD;
	}
}


// note, I only offer an API call to delete all models at once, since model handles are just indexes into
//	an array, and any stored pointers would invalidate if I deleted any in the middle
//
void RE_DeleteModels( void )
{
	for (int i=0; i<tr.numModels; i++)
	{
		if (strcmp(tr.models[i]->name,"*default.gla"))	// can't free global fake-GLA struct
		{
			SAFEFREE(tr.models[i]->pvData);	
		}
		SAFEFREE(tr.models[i]);
	}

	tr.numModels = 0;
}

static void R_ModelInitOnceOnly( void ) 
{	
	RE_DeleteModels();	
	RE_EnsureOneBadModel();
}


model_t	*R_GetModelByHandle( ModelHandle_t index ) 
{
	RE_EnsureOneBadModel();	

	// out of range gets the default model
	if ( index < 1 || index >= tr.numModels ) {
		return tr.models[0];
	}

	model_t	*mod = tr.models[index];

	return mod;
}

//===============================================================================


// used by higher-level app to gain access to actual loaded data...
//
// returns NULL if error...
//
void *RE_GetModelData( ModelHandle_t hModel )
{
	RE_EnsureOneBadModel();	

	model_t	*pModel = R_GetModelByHandle( hModel );
	if (pModel->type != MOD_BAD)
		return pModel->pvData;

	assert(0);
	return NULL;
}


modtype_t RE_GetModelType( ModelHandle_t hModel )
{
	RE_EnsureOneBadModel();	

	model_t	*pModel = R_GetModelByHandle( hModel );
	return pModel->type;
}


// called only from media_delete...
//
typedef struct
{
	byte *pData;
	int iSize;
} CachedBin_t;
typedef map <string, CachedBin_t>	CachedModelBins_t;
									CachedModelBins_t CachedModelBins;
void RE_ModelBinCache_DeleteAll()
{
	for (CachedModelBins_t::iterator it = CachedModelBins.begin(); it != CachedModelBins.end(); ++it)
	{
		CachedBin_t &Bin = (*it).second;
		free(Bin.pData);
	}

	CachedModelBins.clear();
}

static byte *RE_ModelBinCache_Find( const char *psName, int &iSize )
{
	CachedModelBins_t::iterator it = CachedModelBins.find(psName);
	if (it != CachedModelBins.end())
	{
		CachedBin_t &Bin = (*it).second;
		iSize = Bin.iSize;
		return Bin.pData;
	}

	return NULL;
}


static void RE_ModelBinCache_Insert( const char *psName, int iBytesRead, const byte *pBuf )
{
	// don't exceed (say) 300MB?...
	//
	int iTotalBytes = 0;
	for (CachedModelBins_t::iterator it = CachedModelBins.begin(); it != CachedModelBins.end(); ++it)
	{
		CachedBin_t &Bin = (*it).second;
		iTotalBytes += Bin.iSize;
	}

	if (iTotalBytes < 300 * 1024 * 1024)
	{
		CachedBin_t Cache;
					Cache.pData = (byte *) malloc(iBytesRead);
					Cache.iSize = iBytesRead;

		memcpy(Cache.pData,pBuf,iBytesRead);
		CachedModelBins[ psName ] = Cache;
	}
}


/*
====================
Loads in a model for the given name

Zero will be returned if the model fails to load.
An entry will be retained for failed models as an
optimization to prevent disk rescanning if they are
asked for again.
====================
*/
ModelHandle_t RE_RegisterModel( const char *name ) {
	model_t		*mod;
	unsigned	*buf;
	int			lod;
	int			ident;
	qboolean	loaded;	
	int			numLoaded;

	RE_EnsureOneBadModel();	

	if ( !name || !name[0] ) {
		ri.Printf( PRINT_ALL, "RE_RegisterModel: NULL name\n" );
		return 0;
	}

	if ( strlen( name ) >= MAX_QPATH ) {
		Com_Printf( "Model name %s exceeds MAX_QPATH\n",name );
		return 0;
	}

// MODVIEW: Note, because of the way I pass model handles to functions as unique handles that then have their
	//	owning-containers derived from that handle then they must be unique, so ignore cached models. Yes I know
	//	it's lame, but it was easier thatn rewriting the whole thing once I'd realised what direction this app was
	//	going etc. Anyway, since the model cache is already deleted when the primary is loaded then the only time
	//	this becomes inefficient is when you have (say) a guy with the same weapon bolted into both his left and right
	//	hands, then I end up with 2 copies of the loaded models, not one. Whoopee-do.  This is a quick-an-dirty app, 
	//	and I've got other things to get on with so it's a lot quicker to do something like this that makes no hands-on
	//	difference to the user, no matter how evil it is inside... :-)
/*
	//
	// search the currently loaded models
	//
	for (qhandle_t	hModel = 1 ; hModel < tr.numModels; hModel++ ) {
		mod = tr.models[hModel];
		if ( !strcmp( mod->name, name ) ) {
			if( mod->type == MOD_BAD ) {
				return 0;
			}
			return hModel;
		}
	}
*/
	// allocate a new model_t

	if ( ( mod = R_AllocModel() ) == NULL ) {
		ri.Printf( PRINT_WARNING, "RE_RegisterModel: R_AllocModel() failed for '%s'\n", name);
		return 0;
	}

	Q_strncpyz( mod->name, name, sizeof( mod->name ) );


	// make sure the render thread is stopped
//	R_SyncRenderThread();

	mod->numLods = 0;

	//
	// load the files
	//
	numLoaded = 0;

	int LODStart=MD3_MAX_LODS-1;	//this loads the md3s in reverse so they can be biased
	if (strstr (name, ".mdr") || strstr (name, ".gla") || strstr (name, ".glm")) 
	{
		LODStart = 0;
	}

	for ( lod = LODStart ; lod >= 0 ; lod-- ) 
	{
		char filename[1024];

		strcpy( filename, name );
		strlwr( filename );	// for bin map<> cacheing to work

		if ( lod != 0 ) {
			char namebuf[80];

			if ( strrchr( filename, '.' ) ) 
			{
				*strrchr( filename, '.' ) = 0;
			}
			sprintf( namebuf, "_%d.md3", lod );
			strcat( filename, namebuf );
		}

		bool bIsFakeGLA = false;
		if (!strcmp(filename,"*default.gla"))
		{
			bIsFakeGLA = true;
		}

		if (!bIsFakeGLA)
		{
			int iBytesRead;

			const byte *pCachedVersion = RE_ModelBinCache_Find( filename, iBytesRead );
			if (pCachedVersion)
			{
				buf = (unsigned *) malloc(iBytesRead);
				memcpy(buf,pCachedVersion, iBytesRead);
			}
			else
			{
				iBytesRead = ri.FS_ReadFile( filename, (void **)&buf );

				if (buf)
				{
					RE_ModelBinCache_Insert( filename, iBytesRead, (byte *) buf );
				}
			}
		}
		else
		{
			buf = (unsigned*) GLMModel_GetDefaultGLA();
		}
		
		if ( !buf ) 
		{
			continue;
		}
		
//		loadmodel = mod;
		
		ident = LL(*(unsigned *)buf);	// shouldn't really LL this if a fake GLA, but ModView is Intel-endian, so NP.
		if ( ident == MD4_IDENT ) 
		{
			loaded = R_LoadMD4( mod, buf, name );
			lod = MD3_MAX_LODS;
		}
		else if ( ident == MDXA_IDENT )
		{
			if (!bIsFakeGLA)
			{
				loaded = R_LoadMDXA( mod, buf, name );
			}
			else
			{
				mod->type = MOD_MDXA;
				mod->dataSize += ((mdxaHeader_t *)buf)->ofsEnd;
				mod->mdxa = (mdxaHeader_t *) buf;
				loaded = qtrue;
			}
		}
		else if ( ident == MDXM_IDENT ) 
		{
		   loaded = R_LoadMDXM( mod, buf, name );
		// else try load the file as an md3

		}
		else 
		{
			if ( ident != MD3_IDENT ) 
			{
				ri.Printf (PRINT_WARNING,"RE_RegisterModel: unknown fileid for %s\n", name);
				goto fail;
			}

			loaded = R_LoadMD3( mod, lod, buf, name );
		}
		
		if (!bIsFakeGLA)
		{
			ri.FS_FreeFile (buf);
		}

		if ( !loaded ) 
		{
			if ( lod == 0 ) 
			{
				goto fail;
			} else 
			{
				break;
			}
		} else 
		{
			mod->numLods++;
			numLoaded++;
//			// if we have a valid model and are biased
//			// so that we won't see any higher detail ones,
//			// stop loading them
//			if ( lod <= r_lodbias->integer ) 
//			{
//				break;
//			}
		}
	}

	if ( numLoaded ) 
	{
		// duplicate into higher lod spots that weren't
		// loaded, in case the user changes r_lodbias on the fly
		for ( lod-- ; lod >= 0 ; lod-- ) 
		{
			mod->numLods++;
			mod->md3[lod] = mod->md3[lod+1];
		}

		return mod->index;
	}

fail:
	// we still keep the model_t around, so if the model name is asked for
	// again, we won't bother scanning the filesystem
	mod->type = MOD_BAD;
	return 0;
}


/*
=================
R_LoadMD3
=================
*/
static qboolean R_LoadMD3 (model_t *mod, int lod, void *buffer, const char *mod_name ) {
	int					i, j;
	md3Header_t			*pinmodel;
    md3Frame_t			*frame;
	md3Surface_t		*surf;
	md3Shader_t			*shader;
	md3Triangle_t		*tri;
	md3St_t				*st;
	md3XyzNormal_t		*xyz;
	md3Tag_t			*tag;
	int					version;
	int					size;

	pinmodel = (md3Header_t *)buffer;

	version = LL (pinmodel->version);
	if (version != MD3_VERSION) {
		ri.Printf( PRINT_WARNING, "R_LoadMD3: %s has wrong version (%i should be %i)\n",
				 mod_name, version, MD3_VERSION);
		return qfalse;
	}

	mod->type = MOD_MESH;
	size = LL(pinmodel->ofsEnd);
	mod->dataSize += size;
	mod->md3[lod] = (md3Header_t *)ri.Hunk_Alloc( size );

	memcpy (mod->md3[lod], buffer, LL(pinmodel->ofsEnd) );

    LL(mod->md3[lod]->ident);
    LL(mod->md3[lod]->version);
    LL(mod->md3[lod]->numFrames);
    LL(mod->md3[lod]->numTags);
    LL(mod->md3[lod]->numSurfaces);
    LL(mod->md3[lod]->ofsFrames);
    LL(mod->md3[lod]->ofsTags);
    LL(mod->md3[lod]->ofsSurfaces);
    LL(mod->md3[lod]->ofsEnd);

	if ( mod->md3[lod]->numFrames < 1 ) {
		ri.Printf( PRINT_WARNING, "R_LoadMD3: %s has no frames\n", mod_name );
		return qfalse;
	}
    
	// swap all the frames
    frame = (md3Frame_t *) ( (byte *)mod->md3[lod] + mod->md3[lod]->ofsFrames );
    for ( i = 0 ; i < mod->md3[lod]->numFrames ; i++, frame++) {
    	frame->radius = LF( frame->radius );
        for ( j = 0 ; j < 3 ; j++ ) {
            frame->bounds[0][j] = LF( frame->bounds[0][j] );
            frame->bounds[1][j] = LF( frame->bounds[1][j] );
	    	frame->localOrigin[j] = LF( frame->localOrigin[j] );
        }
	}

	// swap all the tags
    tag = (md3Tag_t *) ( (byte *)mod->md3[lod] + mod->md3[lod]->ofsTags );
    for ( i = 0 ; i < mod->md3[lod]->numTags * mod->md3[lod]->numFrames ; i++, tag++) {
        for ( j = 0 ; j < 3 ; j++ ) {
			tag->origin[j] = LF( tag->origin[j] );
			tag->axis[0][j] = LF( tag->axis[0][j] );
			tag->axis[1][j] = LF( tag->axis[1][j] );
			tag->axis[2][j] = LF( tag->axis[2][j] );
        }
	}

	// swap all the surfaces
	surf = (md3Surface_t *) ( (byte *)mod->md3[lod] + mod->md3[lod]->ofsSurfaces );
	for ( i = 0 ; i < mod->md3[lod]->numSurfaces ; i++) {

        LL(surf->ident);
        LL(surf->flags);
        LL(surf->numFrames);
        LL(surf->numShaders);
        LL(surf->numTriangles);
        LL(surf->ofsTriangles);
        LL(surf->numVerts);
        LL(surf->ofsShaders);
        LL(surf->ofsSt);
        LL(surf->ofsXyzNormals);
        LL(surf->ofsEnd);
		
		if ( surf->numVerts > (bQ3RulesApply?SHADER_MAX_VERTEXES:ACTUAL_SHADER_MAX_VERTEXES) ) {
			ri.Error (ERR_DROP, "R_LoadMD3: %s has more than %i verts on a surface (%i)",
				mod_name, (bQ3RulesApply?SHADER_MAX_VERTEXES:ACTUAL_SHADER_MAX_VERTEXES), surf->numVerts );
		}
		if ( surf->numTriangles*3 > (bQ3RulesApply?SHADER_MAX_INDEXES:ACTUAL_SHADER_MAX_INDEXES) ) {
			ri.Error (ERR_DROP, "R_LoadMD3: %s has more than %i triangles on a surface (%i)",
				mod_name, (bQ3RulesApply?SHADER_MAX_INDEXES:ACTUAL_SHADER_MAX_INDEXES) / 3, surf->numTriangles );
		}
	
		// change to surface identifier
		surf->ident = SF_MD3;

		// lowercase the surface name so skin compares are faster
		Q_strlwr( surf->name );

		// strip off a trailing _1 or _2
		// this is a crutch for q3data being a mess
		j = strlen( surf->name );
		if ( j > 2 && surf->name[j-2] == '_' ) {
			surf->name[j-2] = 0;
		}

        // register the shaders
        shader = (md3Shader_t *) ( (byte *)surf + surf->ofsShaders );
        for ( j = 0 ; j < surf->numShaders ; j++, shader++ ) {
//            shader_t	*sh;

			shader->shaderIndex = Texture_Load(shader->name);
        }

		// swap all the triangles
		tri = (md3Triangle_t *) ( (byte *)surf + surf->ofsTriangles );
		for ( j = 0 ; j < surf->numTriangles ; j++, tri++ ) {
			LL(tri->indexes[0]);
			LL(tri->indexes[1]);
			LL(tri->indexes[2]);
		}

		// swap all the ST
        st = (md3St_t *) ( (byte *)surf + surf->ofsSt );
        for ( j = 0 ; j < surf->numVerts ; j++, st++ ) {
            st->st[0] = LF( st->st[0] );
            st->st[1] = LF( st->st[1] );
        }

		// swap all the XyzNormals
        xyz = (md3XyzNormal_t *) ( (byte *)surf + surf->ofsXyzNormals );
        for ( j = 0 ; j < surf->numVerts * surf->numFrames ; j++, xyz++ ) 
		{
            xyz->xyz[0] = LS( xyz->xyz[0] );
            xyz->xyz[1] = LS( xyz->xyz[1] );
            xyz->xyz[2] = LS( xyz->xyz[2] );

            xyz->normal = LS( xyz->normal );
        }


		// find the next surface
		surf = (md3Surface_t *)( (byte *)surf + surf->ofsEnd );
	}
    
	return qtrue;
}



/*
=================
R_LoadMD4
=================
*/
static qboolean R_LoadMD4( model_t *mod, void *buffer, const char *mod_name ) {
	int					i, j, k;
	md4Header_t			*pinmodel, *md4;
    md4Frame_t			*frame;
	md4LOD_t			*lod;
	md4Surface_t		*surf;
	md4Triangle_t		*tri;
	md4Vertex_t			*v;
	int					version;
	int					size;
//	shader_t			*sh;
	int					frameSize;

	pinmodel = (md4Header_t *)buffer;

	version = LL (pinmodel->version);
	if (version != MD4_VERSION) {
		ri.Printf( PRINT_WARNING, "R_LoadMD4: %s has wrong version (%i should be %i)\n",
				 mod_name, version, MD4_VERSION);
		return qfalse;
	}

	mod->type = MOD_MD4;
	size = LL(pinmodel->ofsEnd);
	mod->dataSize += size;
	md4 = mod->md4 = (md4Header_t *)ri.Hunk_Alloc( size );

	memcpy( md4, buffer, LL(pinmodel->ofsEnd) );

    LL(md4->ident);
    LL(md4->version);
    LL(md4->numFrames);
    LL(md4->numBones);
    LL(md4->numLODs);
    LL(md4->ofsFrames);
    LL(md4->ofsLODs);
    LL(md4->ofsEnd);

	if ( md4->numFrames < 1 ) {
		ri.Printf( PRINT_WARNING, "R_LoadMD4: %s has no frames\n", mod_name );
		return qfalse;
	}

    // we don't need to swap tags in the renderer, they aren't used
    
	// swap all the frames
	frameSize = (int)( &((md4Frame_t *)0)->bones[ md4->numBones ] );
    for ( i = 0 ; i < md4->numFrames ; i++, frame++) {
	    frame = (md4Frame_t *) ( (byte *)md4 + md4->ofsFrames + i * frameSize );
    	frame->radius = LF( frame->radius );
        for ( j = 0 ; j < 3 ; j++ ) {
            frame->bounds[0][j] = LF( frame->bounds[0][j] );
            frame->bounds[1][j] = LF( frame->bounds[1][j] );
	    	frame->localOrigin[j] = LF( frame->localOrigin[j] );
        }
		for ( j = 0 ; j < md4->numBones * sizeof( md4Bone_t ) / 4 ; j++ ) {
			((float *)frame->bones)[j] = LF( ((float *)frame->bones)[j] );
		}
	}

	// swap all the LOD's
	lod = (md4LOD_t *) ( (byte *)md4 + md4->ofsLODs );
	for ( i = 0 ; i < md4->numLODs ; i++) {

		// swap all the surfaces
		surf = (md4Surface_t *) ( (byte *)lod + lod->ofsSurfaces );
		for ( i = 0 ; i < lod->numSurfaces ; i++) {
			LL(surf->ident);
			LL(surf->numTriangles);
			LL(surf->ofsTriangles);
			LL(surf->numVerts);
			LL(surf->ofsVerts);
			LL(surf->ofsEnd);
			
			if ( surf->numVerts > (bQ3RulesApply?SHADER_MAX_VERTEXES:ACTUAL_SHADER_MAX_VERTEXES) ) {
				ri.Error (ERR_DROP, "R_LoadMD3: %s has more than %i verts on a surface (%i)",
					mod_name, (bQ3RulesApply?SHADER_MAX_VERTEXES:ACTUAL_SHADER_MAX_VERTEXES), surf->numVerts );
			}
			if ( surf->numTriangles*3 > (bQ3RulesApply?SHADER_MAX_INDEXES:ACTUAL_SHADER_MAX_INDEXES) ) {
				ri.Error (ERR_DROP, "R_LoadMD3: %s has more than %i triangles on a surface (%i)",
					mod_name, (bQ3RulesApply?SHADER_MAX_INDEXES:ACTUAL_SHADER_MAX_INDEXES) / 3, surf->numTriangles );
			}

			// register the shaders
			surf->shaderIndex = Texture_Load(surf->shader);

			// swap all the triangles
			tri = (md4Triangle_t *) ( (byte *)surf + surf->ofsTriangles );
			for ( j = 0 ; j < surf->numTriangles ; j++, tri++ ) {
				LL(tri->indexes[0]);
				LL(tri->indexes[1]);
				LL(tri->indexes[2]);
			}

			// swap all the vertexes
			v = (md4Vertex_t *) ( (byte *)surf + surf->ofsVerts );
			for ( j = 0 ; j < surf->numVerts ; j++ ) {
				v->normal[0] = LF( v->normal[0] );
				v->normal[1] = LF( v->normal[1] );
				v->normal[2] = LF( v->normal[2] );

				v->texCoords[0] = LF( v->texCoords[0] );
				v->texCoords[1] = LF( v->texCoords[1] );

				v->numWeights = LL( v->numWeights );

				for ( k = 0 ; k < v->numWeights ; k++ ) {
					v->weights[k].boneIndex = LL( v->weights[k].boneIndex );
					v->weights[k].boneWeight = LF( v->weights[k].boneWeight );
				   v->weights[k].offset[0] = LF( v->weights[k].offset[0] );
				   v->weights[k].offset[1] = LF( v->weights[k].offset[1] );
				   v->weights[k].offset[2] = LF( v->weights[k].offset[2] );
				}
				v = (md4Vertex_t *)&v->weights[v->numWeights];
			}

			// find the next surface
			surf = (md4Surface_t *)( (byte *)surf + surf->ofsEnd );
		}

		// find the next LOD
		lod = (md4LOD_t *)( (byte *)lod + lod->ofsEnd );
	}

	return qtrue;
}




//=============================================================================

/*
void RE_BeginRegistration( glconfig_t *glconfigOut ) {
	ri.Hunk_Clear();

	R_Init();
	*glconfigOut = glConfig;

	R_SyncRenderThread();

	tr.viewCluster = -1;		// force markleafs to regenerate
	R_ClearFlares();
	RE_ClearScene();

	tr.registered = qtrue;
}
*/
//=============================================================================


/*
void R_Modellist_f( void ) {
	int		i, j;
	model_t	*mod;
	int		total;
	int		lods;

	total = 0;
	for ( i = 1 ; i < tr.numModels; i++ ) {
		mod = tr.models[i];
		if (mod->mdxm || mod->mdxa)
		{
			ri.Printf( PRINT_ALL, "%8i : (%i) %s\n",mod->dataSize, mod->numLods, mod->name );
		}
		else
		{
			lods = 1;
			for ( j = 1 ; j < MD3_MAX_LODS ; j++ ) {
				if ( mod->md3[j] && mod->md3[j] != mod->md3[j-1] ) {
					lods++;
				}
			}
			ri.Printf( PRINT_ALL, "%8i : (%i) %s\n",mod->dataSize, lods, mod->name );
		}
		total += mod->dataSize;
	}
	ri.Printf( PRINT_ALL, "%8i : Total models\n", total );

#if	0		// not working right with new hunk
	if ( tr.world ) {
		ri.Printf( PRINT_ALL, "\n%8i : %s\n", tr.world->dataSize, tr.world->name );
	}
#endif
}

//=============================================================================


static md3Tag_t *R_GetTag( md3Header_t *mod, int frame, const char *tagName ) {
	md3Tag_t		*tag;
	int				i;

	if ( frame >= mod->numFrames ) {
		// it is possible to have a bad frame while changing models, so don't error
		frame = mod->numFrames - 1;
	}

	tag = (md3Tag_t *)((byte *)mod + mod->ofsTags) + frame * mod->numTags;
	for ( i = 0 ; i < mod->numTags ; i++, tag++ ) {
		if ( !strcmp( tag->name, tagName ) ) {
			return tag;	// found it
		}
	}

	return NULL;
}

void	R_LerpTag( orientation_t *tag, qhandle_t handle, int startFrame, int endFrame, 
					 float frac, const char *tagName ) {
	md3Tag_t	*start, *end;
	int		i;
	float		frontLerp, backLerp;
	model_t		*model;

	model = R_GetModelByHandle( handle );
	if ( !model->md3[0] ) {
		AxisClear( tag->axis );
		VectorClear( tag->origin );
		return;
	}

	start = R_GetTag( model->md3[0], startFrame, tagName );
	end = R_GetTag( model->md3[0], endFrame, tagName );
	if ( !start || !end ) {
		AxisClear( tag->axis );
		VectorClear( tag->origin );
		return;
	}

	frontLerp = frac;
	backLerp = 1.0 - frac;

	for ( i = 0 ; i < 3 ; i++ ) {
		tag->origin[i] = start->origin[i] * backLerp +  end->origin[i] * frontLerp;
		tag->axis[0][i] = start->axis[0][i] * backLerp +  end->axis[0][i] * frontLerp;
		tag->axis[1][i] = start->axis[1][i] * backLerp +  end->axis[1][i] * frontLerp;
		tag->axis[2][i] = start->axis[2][i] * backLerp +  end->axis[2][i] * frontLerp;
	}
	VectorNormalize( tag->axis[0] );
	VectorNormalize( tag->axis[1] );
	VectorNormalize( tag->axis[2] );
}


void R_ModelBounds( qhandle_t handle, vec3_t mins, vec3_t maxs ) {
	model_t		*model;
	md3Header_t	*header;
	md3Frame_t	*frame;

	model = R_GetModelByHandle( handle );

	if ( model->bmodel ) {
		VectorCopy( model->bmodel->bounds[0], mins );
		VectorCopy( model->bmodel->bounds[1], maxs );
		return;
	}

	if ( !model->md3[0] ) {
		VectorClear( mins );
		VectorClear( maxs );
		return;
	}

	header = model->md3[0];

	frame = (md3Frame_t *)( (byte *)header + header->ofsFrames );

	VectorCopy( frame->bounds[0], mins );
	VectorCopy( frame->bounds[1], maxs );
}
*/


void OnceOnlyCrap(void)
{
	ri.Hunk_Alloc				= Crap_HunkAlloc;
	
	ri.Hunk_AllocateTempMemory	= Crap_HunkAlloc;
	ri.Hunk_FreeTempMemory		= Crap_Free;
	
	ri.Malloc					= Crap_Malloc;
	ri.Free						= Crap_Free;	

	ri.Printf					= Crap_Printf;
	ri.Error					= Crap_Error;

	ri.FS_ReadFile				= Crap_FS_ReadFile;
	ri.FS_WriteFile				= Crap_FS_WriteFile;
	ri.FS_FreeFile				= Crap_FS_FreeFile;

	ZEROMEM(tr);
	ZEROMEM(tess);

	R_ModelInitOnceOnly();
}




void trap_G2_SurfaceOffList( int a, void *b)
{
	G2_GetSurfaceList( (qhandle_t) a, (surfaceInfo_t *) b);
}

qboolean trap_G2_SetSurfaceOnOff (qhandle_t model, surfaceInfo_t *slist, const char *surfaceName, const SurfaceOnOff_t offFlags, const int surface)
{
	return G2_SetSurfaceOnOff(model, slist, surfaceName, offFlags, surface);
}

SurfaceOnOff_t trap_G2_IsSurfaceOff (qhandle_t model, surfaceInfo_t *slist, const char *surfaceName)
{
	return G2_IsSurfaceOff (model, slist, surfaceName);
}

void trap_G2_Init_Bone_List(void *a)
{
	G2_Init_Bone_List((boneInfo_t *)a);
}

qboolean trap_G2_Set_Bone_Anim(int a, void *b, void *c, int d, int e, int f, float g)
{
	return G2_Set_Bone_Anim((qhandle_t) a, (boneInfo_t *) b, (char *) c, d, e, f, g);
}



//////////////// eof ///////////////

