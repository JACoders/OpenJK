/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
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

// tr_models.c -- model loading and caching

#include "tr_local.h"
#include "qcommon/disablewarnings.h"
#include "qcommon/sstring.h"	// #include <string>

#include <vector>
#include <map>

#define	LL(x) x=LittleLong(x)

static qboolean R_LoadMD3 (model_t *mod, int lod, void *buffer, const char *name, qboolean &bAlreadyCached );
/*
Ghoul2 Insert Start
*/

typedef	struct modelHash_s
{
	char		name[MAX_QPATH];
	qhandle_t	handle;
	struct		modelHash_s	*next;

}modelHash_t;

#define FILE_HASH_SIZE		1024
static	modelHash_t 		*mhHashTable[FILE_HASH_SIZE];

/*
Ghoul2 Insert End
*/


// This stuff looks a bit messy, but it's kept here as black box, and nothing appears in any .H files for other
//	modules to worry about. I may make another module for this sometime.
//
typedef std::pair<int,int> StringOffsetAndShaderIndexDest_t;
typedef std::vector <StringOffsetAndShaderIndexDest_t> ShaderRegisterData_t;
struct CachedEndianedModelBinary_s
{
	void	*pModelDiskImage;
	int		iAllocSize;		// may be useful for mem-query, but I don't actually need it
	ShaderRegisterData_t ShaderRegisterData;
	int		iLastLevelUsedOn;
	int		iPAKFileCheckSum;	// else -1 if not from PAK


	CachedEndianedModelBinary_s()
	{
		pModelDiskImage		= 0;
		iAllocSize			= 0;
		ShaderRegisterData.clear();
		iLastLevelUsedOn	= -1;
		iPAKFileCheckSum	= -1;
	}
};
typedef struct CachedEndianedModelBinary_s CachedEndianedModelBinary_t;
typedef std::map <sstring_t,CachedEndianedModelBinary_t>	CachedModels_t;
CachedModels_t *CachedModels = NULL;	// the important cache item.

void RE_RegisterModels_StoreShaderRequest(const char *psModelFileName, const char *psShaderName, int *piShaderIndexPoke)
{
	char sModelName[MAX_QPATH];

	assert(CachedModels);

	Q_strncpyz(sModelName,psModelFileName,sizeof(sModelName));
	Q_strlwr  (sModelName);

	CachedEndianedModelBinary_t &ModelBin = (*CachedModels)[sModelName];

	if (ModelBin.pModelDiskImage == NULL)
	{
		assert(0);	// should never happen, means that we're being called on a model that wasn't loaded
	}
	else
	{
		int iNameOffset =		  psShaderName		- (char *)ModelBin.pModelDiskImage;
		int iPokeOffset = (char*) piShaderIndexPoke	- (char *)ModelBin.pModelDiskImage;

		ModelBin.ShaderRegisterData.push_back( StringOffsetAndShaderIndexDest_t( iNameOffset,iPokeOffset) );
	}
}


static const byte FakeGLAFile[] =
{
0x32, 0x4C, 0x47, 0x41, 0x06, 0x00, 0x00, 0x00, 0x2A, 0x64, 0x65, 0x66, 0x61, 0x75, 0x6C, 0x74,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x01, 0x00, 0x00, 0x00,
0x14, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x18, 0x01, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00,
0x26, 0x01, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x4D, 0x6F, 0x64, 0x56, 0x69, 0x65, 0x77, 0x20,
0x69, 0x6E, 0x74, 0x65, 0x72, 0x6E, 0x61, 0x6C, 0x20, 0x64, 0x65, 0x66, 0x61, 0x75, 0x6C, 0x74,
0x00, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD,
0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD,
0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFD, 0xBF, 0xFE, 0x7F, 0xFE, 0x7F, 0xFE, 0x7F,
0x00, 0x80, 0x00, 0x80, 0x00, 0x80
};

void RE_LoadWorldMap_Actual( const char *name, world_t &worldData, int index );

// returns qtrue if loaded, and sets the supplied qbool to true if it was from cache (instead of disk)
//   (which we need to know to avoid LittleLong()ing everything again (well, the Mac needs to know anyway)...
//
// don't use ri->xxx functions in case running on dedicated...
//
qboolean RE_RegisterModels_GetDiskFile( const char *psModelFileName, void **ppvBuffer, qboolean *pqbAlreadyCached)
{
	char sModelName[MAX_QPATH];

	assert(CachedModels);

	Q_strncpyz(sModelName,psModelFileName,sizeof(sModelName));
	Q_strlwr  (sModelName);

	CachedEndianedModelBinary_t &ModelBin = (*CachedModels)[sModelName];

	if (ModelBin.pModelDiskImage == NULL)
	{
		// didn't have it cached, so try the disk...
		//

			// special case intercept first...
			//
			if (!strcmp(sDEFAULT_GLA_NAME ".gla" , psModelFileName))
			{
				// return fake params as though it was found on disk...
				//
				void *pvFakeGLAFile = Z_Malloc( sizeof(FakeGLAFile), TAG_FILESYS, qfalse );
				memcpy(pvFakeGLAFile, &FakeGLAFile[0],  sizeof(FakeGLAFile));
				*ppvBuffer = pvFakeGLAFile;
				*pqbAlreadyCached = qfalse;	// faking it like this should mean that it works fine on the Mac as well
				return qtrue;
			}

		ri.FS_ReadFile( sModelName, ppvBuffer );
		*pqbAlreadyCached = qfalse;
		qboolean bSuccess = !!(*ppvBuffer)?qtrue:qfalse;

		if (bSuccess)
		{
			ri.Printf( PRINT_DEVELOPER, "RE_RegisterModels_GetDiskFile(): Disk-loading \"%s\"\n",psModelFileName);
		}

		return bSuccess;
	}
	else
	{
		*ppvBuffer = ModelBin.pModelDiskImage;
		*pqbAlreadyCached = qtrue;
		return qtrue;
	}
}


// if return == true, no further action needed by the caller...
//
// don't use ri->xxx functions in case running on dedicated
//
void *RE_RegisterModels_Malloc(int iSize, void *pvDiskBufferIfJustLoaded, const char *psModelFileName, qboolean *pqbAlreadyFound, memtag_t eTag)
{
	char sModelName[MAX_QPATH];

	assert(CachedModels);

	Q_strncpyz(sModelName,psModelFileName,sizeof(sModelName));
	Q_strlwr  (sModelName);

	CachedEndianedModelBinary_t &ModelBin = (*CachedModels)[sModelName];

	if (ModelBin.pModelDiskImage == NULL)
	{
		// ... then this entry has only just been created, ie we need to load it fully...
		//
		// new, instead of doing a Z_Malloc and assigning that we just morph the disk buffer alloc
		//	then don't thrown it away on return - cuts down on mem overhead
		//
		// ... groan, but not if doing a limb hierarchy creation (some VV stuff?), in which case it's NULL
		//
		if ( pvDiskBufferIfJustLoaded )
		{
			Z_MorphMallocTag( pvDiskBufferIfJustLoaded, eTag );
		}
		else
		{
			pvDiskBufferIfJustLoaded =  Z_Malloc(iSize,eTag, qfalse );
		}

		ModelBin.pModelDiskImage	= pvDiskBufferIfJustLoaded;
		ModelBin.iAllocSize			= iSize;

		int iCheckSum;
		if (ri.FS_FileIsInPAK(sModelName, &iCheckSum) == 1)
		{
			ModelBin.iPAKFileCheckSum = iCheckSum;	// else ModelBin's constructor will leave it as -1
		}

		*pqbAlreadyFound = qfalse;
	}
	else
	{
		*pqbAlreadyFound = qtrue;	// tell caller not to re-Endian or re-Shader this binary
	}

	ModelBin.iLastLevelUsedOn = RE_RegisterMedia_GetLevel();

	return ModelBin.pModelDiskImage;
}

// Unfortunately the dedicated server also hates shader loading. So we need an alternate of this func.
//
void *RE_RegisterServerModels_Malloc(int iSize, void *pvDiskBufferIfJustLoaded, const char *psModelFileName, qboolean *pqbAlreadyFound, memtag_t eTag)
{
	char sModelName[MAX_QPATH];

	assert(CachedModels);

	Q_strncpyz(sModelName,psModelFileName,sizeof(sModelName));
	Q_strlwr  (sModelName);

	CachedEndianedModelBinary_t &ModelBin = (*CachedModels)[sModelName];

	if (ModelBin.pModelDiskImage == NULL)
	{
		// new, instead of doing a Z_Malloc and assigning that we just morph the disk buffer alloc
		//	then don't thrown it away on return - cuts down on mem overhead
		//
		// ... groan, but not if doing a limb hierarchy creation (some VV stuff?), in which case it's NULL
		//
		if ( pvDiskBufferIfJustLoaded )
		{
			Z_MorphMallocTag( pvDiskBufferIfJustLoaded, eTag );
		}
		else
		{
			pvDiskBufferIfJustLoaded =  Z_Malloc(iSize,eTag, qfalse );
		}

		ModelBin.pModelDiskImage	= pvDiskBufferIfJustLoaded;
		ModelBin.iAllocSize			= iSize;

		int iCheckSum;
		if (ri.FS_FileIsInPAK(sModelName, &iCheckSum) == 1)
		{
			ModelBin.iPAKFileCheckSum = iCheckSum;	// else ModelBin's constructor will leave it as -1
		}

		*pqbAlreadyFound = qfalse;
	}
	else
	{
		// if we already had this model entry, then re-register all the shaders it wanted...
		//
		/*
		int iEntries = ModelBin.ShaderRegisterData.size();
		for (int i=0; i<iEntries; i++)
		{
			int iShaderNameOffset	= ModelBin.ShaderRegisterData[i].first;
			int iShaderPokeOffset	= ModelBin.ShaderRegisterData[i].second;

			char *psShaderName		=		  &((char*)ModelBin.pModelDiskImage)[iShaderNameOffset];
			int  *piShaderPokePtr	= (int *) &((char*)ModelBin.pModelDiskImage)[iShaderPokeOffset];

			shader_t *sh = R_FindShader( psShaderName, lightmapsNone, stylesDefault, qtrue );

			if ( sh->defaultShader )
			{
				*piShaderPokePtr = 0;
			} else {
				*piShaderPokePtr = sh->index;
			}
		}
		*/
		//No. Bad.
		*pqbAlreadyFound = qtrue;	// tell caller not to re-Endian or re-Shader this binary
	}

	ModelBin.iLastLevelUsedOn = RE_RegisterMedia_GetLevel();

	return ModelBin.pModelDiskImage;
}

// dump any models not being used by this level if we're running low on memory...
//
static int GetModelDataAllocSize(void)
{
	return	Z_MemSize( TAG_MODEL_MD3) +
			Z_MemSize( TAG_MODEL_GLM) +
			Z_MemSize( TAG_MODEL_GLA);
}
extern cvar_t *r_modelpoolmegs;
//
// return qtrue if at least one cached model was freed (which tells z_malloc()-fail recoveryt code to try again)
//
extern qboolean gbInsideRegisterModel;
qboolean RE_RegisterModels_LevelLoadEnd(qboolean bDeleteEverythingNotUsedThisLevel /* = qfalse */)
{
	qboolean bAtLeastoneModelFreed = qfalse;

	assert(CachedModels);

	ri.Printf( PRINT_DEVELOPER, S_COLOR_RED "RE_RegisterModels_LevelLoadEnd():\n");

	if (gbInsideRegisterModel)
	{
		ri.Printf( PRINT_DEVELOPER, "(Inside RE_RegisterModel (z_malloc recovery?), exiting...\n");
	}
	else
	{
		int iLoadedModelBytes	=	GetModelDataAllocSize();
		const int iMaxModelBytes=	r_modelpoolmegs->integer * 1024 * 1024;

		for (CachedModels_t::iterator itModel = CachedModels->begin(); itModel != CachedModels->end() && ( bDeleteEverythingNotUsedThisLevel || iLoadedModelBytes > iMaxModelBytes ); /* empty */ )
		{
			CachedEndianedModelBinary_t &CachedModel = (*itModel).second;

			qboolean bDeleteThis = qfalse;

			if (bDeleteEverythingNotUsedThisLevel)
			{
				bDeleteThis = (CachedModel.iLastLevelUsedOn != RE_RegisterMedia_GetLevel()) ? qtrue : qfalse;
			}
			else
			{
				bDeleteThis = (CachedModel.iLastLevelUsedOn < RE_RegisterMedia_GetLevel()) ? qtrue : qfalse;
			}

			// if it wasn't used on this level, dump it...
			//
			if (bDeleteThis)
			{
				const char *psModelName = (*itModel).first.c_str();
				ri.Printf( PRINT_DEVELOPER, S_COLOR_RED "Dumping \"%s\"", psModelName);

	#ifdef _DEBUG
				ri.Printf( PRINT_DEVELOPER, S_COLOR_RED ", used on lvl %d\n",CachedModel.iLastLevelUsedOn);
	#endif

				if (CachedModel.pModelDiskImage) {
					Z_Free(CachedModel.pModelDiskImage);
					//CachedModel.pModelDiskImage = NULL;	// REM for reference, erase() call below negates the need for it.
					bAtLeastoneModelFreed = qtrue;
				}
				CachedModels->erase(itModel++);

				iLoadedModelBytes = GetModelDataAllocSize();
			}
			else
			{
				++itModel;
			}
		}
	}

	ri.Printf( PRINT_DEVELOPER, S_COLOR_RED "RE_RegisterModels_LevelLoadEnd(): Ok\n");

	return bAtLeastoneModelFreed;
}



// scan through all loaded models and see if their PAK checksums are still valid with the current pure PAK lists,
//	dump any that aren't (so people can't cheat by using models with huge spikes that show through walls etc)
//
// (avoid using ri->xxxx stuff here in case running on dedicated)
//
static void RE_RegisterModels_DumpNonPure(void)
{
	ri.Printf( PRINT_DEVELOPER,  "RE_RegisterModels_DumpNonPure():\n");

	if(!CachedModels) {
		return;
	}

	for (CachedModels_t::iterator itModel = CachedModels->begin(); itModel != CachedModels->end(); /* blank */)
	{
		qboolean bEraseOccured = qfalse;

		const char *psModelName	 = (*itModel).first.c_str();
		CachedEndianedModelBinary_t &CachedModel = (*itModel).second;

		int iCheckSum = -1;
		int iInPak = ri.FS_FileIsInPAK(psModelName, &iCheckSum);

		if (iInPak == -1 || iCheckSum != CachedModel.iPAKFileCheckSum)
		{
			if (Q_stricmp(sDEFAULT_GLA_NAME ".gla" , psModelName))	// don't dump "*default.gla", that's program internal anyway
			{
				// either this is not from a PAK, or it's from a non-pure one, so ditch it...
				//
				ri.Printf( PRINT_DEVELOPER, "Dumping none pure model \"%s\"", psModelName);

				if (CachedModel.pModelDiskImage) {
					Z_Free(CachedModel.pModelDiskImage);
					//CachedModel.pModelDiskImage = NULL;	// REM for reference, erase() call below negates the need for it.
				}

				CachedModels->erase(itModel++);
				bEraseOccured = qtrue;
			}
		}

		if ( !bEraseOccured )
		{
			++itModel;
		}
	}

	ri.Printf( PRINT_DEVELOPER, "RE_RegisterModels_DumpNonPure(): Ok\n");
}

void RE_RegisterModels_Info_f( void )
{
	int iTotalBytes = 0;
	if(!CachedModels) {
		Com_Printf ("%d bytes total (%.2fMB)\n",iTotalBytes, (float)iTotalBytes / 1024.0f / 1024.0f);
		return;
	}

	int iModels = CachedModels->size();
	int iModel  = 0;

	for (CachedModels_t::iterator itModel = CachedModels->begin(); itModel != CachedModels->end(); ++itModel,iModel++)
	{
		CachedEndianedModelBinary_t &CachedModel = (*itModel).second;

		Com_Printf ("%d/%d: \"%s\" (%d bytes)",iModel,iModels,(*itModel).first.c_str(),CachedModel.iAllocSize );

		#ifdef _DEBUG
		Com_Printf (", lvl %d\n",CachedModel.iLastLevelUsedOn);
		#endif

		iTotalBytes += CachedModel.iAllocSize;
	}
	Com_Printf ("%d bytes total (%.2fMB)\n",iTotalBytes, (float)iTotalBytes / 1024.0f / 1024.0f);
}


// (don't use ri->xxx functions since the renderer may not be running here)...
//
static void RE_RegisterModels_DeleteAll(void)
{
	if(!CachedModels) {
		return;	//argh!
	}

	for (CachedModels_t::iterator itModel = CachedModels->begin(); itModel != CachedModels->end(); )
	{
		CachedEndianedModelBinary_t &CachedModel = (*itModel).second;

		if (CachedModel.pModelDiskImage) {
			Z_Free(CachedModel.pModelDiskImage);
		}

		CachedModels->erase(itModel++);
	}
}


// do not use ri->xxx functions in here, the renderer may not be running (ie. if on a dedicated server)...
//
static int giRegisterMedia_CurrentLevel=0;
void RE_RegisterMedia_LevelLoadBegin(const char *psMapName, ForceReload_e eForceReload)
{
	// for development purposes we may want to ditch certain media just before loading a map...
	//
	bool bDeleteModels	= eForceReload == eForceReload_MODELS || eForceReload == eForceReload_ALL;
//	bool bDeleteBSP		= eForceReload == eForceReload_BSP    || eForceReload == eForceReload_ALL;

	if (bDeleteModels)
	{
		RE_RegisterModels_DeleteAll();
	}
	else
	{
		if ( ri.Cvar_VariableIntegerValue( "sv_pure" ) )
		{
			RE_RegisterModels_DumpNonPure();
		}
	}

	tr.numBSPModels = 0;

	// at some stage I'll probably want to put some special logic here, like not incrementing the level number
	//	when going into a map like "brig" or something, so returning to the previous level doesn't require an
	//	asset reload etc, but for now...
	//
	// only bump level number if we're not on the same level.
	//	Note that this will hide uncached models, which is perhaps a bad thing?...
	//
	static char sPrevMapName[MAX_QPATH]={0};
	if (Q_stricmp( psMapName,sPrevMapName ))
	{
		Q_strncpyz( sPrevMapName, psMapName, sizeof(sPrevMapName) );
		giRegisterMedia_CurrentLevel++;
	}
}

int RE_RegisterMedia_GetLevel(void)
{
	return giRegisterMedia_CurrentLevel;
}

// this is now only called by the client, so should be ok to dump media...
//
void RE_RegisterMedia_LevelLoadEnd(void)
{
	RE_RegisterModels_LevelLoadEnd(qfalse);
}



/*
** R_GetModelByHandle
*/
model_t	*R_GetModelByHandle( qhandle_t index ) {
	model_t		*mod;

	// out of range gets the defualt model
	if ( index < 1 || index >= tr.numModels ) {
		return tr.models[0];
	}

	mod = tr.models[index];

	return mod;
}

//===============================================================================

/*
** R_AllocModel
*/
model_t *R_AllocModel( void ) {
	model_t		*mod;

	if ( tr.numModels == MAX_MOD_KNOWN ) {
		return NULL;
	}

	mod = (struct model_s *)Hunk_Alloc( sizeof( *tr.models[tr.numModels] ), h_low );
	mod->index = tr.numModels;
	tr.models[tr.numModels] = mod;
	tr.numModels++;

	return mod;
}

/*
Ghoul2 Insert Start
*/

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
		hash+=(long)(letter)*(i+119);
		i++;
	}
	hash &= (size-1);
	return hash;
}

void RE_InsertModelIntoHash(const char *name, model_t *mod)
{
	int			hash;
	modelHash_t	*mh;

	hash = generateHashValue(name, FILE_HASH_SIZE);

	// insert this file into the hash table so we can look it up faster later
	mh = (modelHash_t*)Hunk_Alloc( sizeof( modelHash_t ), h_low );

	mh->next = mhHashTable[hash];
	mh->handle = mod->index;
	strcpy(mh->name, name);
	mhHashTable[hash] = mh;
}
/*
Ghoul2 Insert End
*/

//rww - Please forgive me for all of the below. Feel free to destroy it and replace it with something better.
//You obviously can't touch anything relating to shaders or ri-> functions here in case a dedicated
//server is running, which is the entire point of having these seperate functions. If anything major
//is changed in the non-server-only versions of these functions it would be wise to incorporate it
//here as well.

/*
=================
ServerLoadMDXA - load a Ghoul 2 animation file
=================
*/
qboolean ServerLoadMDXA( model_t *mod, void *buffer, const char *mod_name, qboolean &bAlreadyCached ) {

	mdxaHeader_t		*pinmodel, *mdxa;
	int					version;
	int					size;

#if 0 //#ifndef _M_IX86
	int					j, k, i;
	int					frameSize;
	mdxaFrame_t			*cframe;
	mdxaSkel_t			*boneInfo;
#endif

 	pinmodel = (mdxaHeader_t *)buffer;
	//
	// read some fields from the binary, but only LittleLong() them when we know this wasn't an already-cached model...
	//
	version = (pinmodel->version);
	size	= (pinmodel->ofsEnd);

	if (!bAlreadyCached)
	{
		LL(version);
		LL(size);
	}

	if (version != MDXA_VERSION) {
		return qfalse;
	}

	mod->type		= MOD_MDXA;
	mod->dataSize  += size;

	qboolean bAlreadyFound = qfalse;
	mdxa = mod->mdxa = (mdxaHeader_t*) //Hunk_Alloc( size );
										RE_RegisterServerModels_Malloc(size, buffer, mod_name, &bAlreadyFound, TAG_MODEL_GLA);

	assert(bAlreadyCached == bAlreadyFound);	// I should probably eliminate 'bAlreadyFound', but wtf?

	if (!bAlreadyFound)
	{
		// horrible new hackery, if !bAlreadyFound then we've just done a tag-morph, so we need to set the
		//	bool reference passed into this function to true, to tell the caller NOT to do an ri.FS_Freefile since
		//	we've hijacked that memory block...
		//
		// Aaaargh. Kill me now...
		//
		bAlreadyCached = qtrue;
		assert( mdxa == buffer );
//		memcpy( mdxa, buffer, size );	// and don't do this now, since it's the same thing

		LL(mdxa->ident);
		LL(mdxa->version);
		LL(mdxa->numFrames);
		LL(mdxa->numBones);
		LL(mdxa->ofsFrames);
		LL(mdxa->ofsEnd);
	}

 	if ( mdxa->numFrames < 1 ) {
		return qfalse;
	}

	if (bAlreadyFound)
	{
		return qtrue;	// All done, stop here, do not LittleLong() etc. Do not pass go...
	}

#if 0 //#ifndef _M_IX86

	//
	// optimisation, we don't bother doing this for standard intel case since our data's already in that format...
	//

	// swap all the skeletal info
	boneInfo = (mdxaSkel_t *)( (byte *)mdxa + mdxa->ofsSkel);
	for ( i = 0 ; i < mdxa->numBones ; i++)
	{
		LL(boneInfo->numChildren);
		LL(boneInfo->parent);
		for (k=0; k<boneInfo->numChildren; k++)
		{
			LL(boneInfo->children[k]);
		}

		// get next bone
		boneInfo += (int)( &((mdxaSkel_t *)0)->children[ boneInfo->numChildren ] );
	}


	// swap all the frames
	frameSize = (int)( &((mdxaFrame_t *)0)->bones[ mdxa->numBones ] );
	for ( i = 0 ; i < mdxa->numFrames ; i++)
	{
		cframe = (mdxaFrame_t *) ( (byte *)mdxa + mdxa->ofsFrames + i * frameSize );
   		cframe->radius = LittleFloat( cframe->radius );
		for ( j = 0 ; j < 3 ; j++ )
		{
			cframe->bounds[0][j] = LittleFloat( cframe->bounds[0][j] );
			cframe->bounds[1][j] = LittleFloat( cframe->bounds[1][j] );
    		cframe->localOrigin[j] = LittleFloat( cframe->localOrigin[j] );
		}
		for ( j = 0 ; j < mdxa->numBones * sizeof( mdxaBone_t ) / 2 ; j++ )
		{
			((short *)cframe->bones)[j] = LittleShort( ((short *)cframe->bones)[j] );
		}
	}
#endif
	return qtrue;
}

/*
=================
ServerLoadMDXM - load a Ghoul 2 Mesh file
=================
*/
qboolean ServerLoadMDXM( model_t *mod, void *buffer, const char *mod_name, qboolean &bAlreadyCached ) {
	int					i,l, j;
	mdxmHeader_t		*pinmodel, *mdxm;
	mdxmLOD_t			*lod;
	mdxmSurface_t		*surf;
	int					version;
	int					size;
	//shader_t			*sh;
	mdxmSurfHierarchy_t	*surfInfo;

#if 0 //#ifndef _M_IX86
	int					k;
	int					frameSize;
	mdxmTag_t			*tag;
	mdxmTriangle_t		*tri;
	mdxmVertex_t		*v;
 	mdxmFrame_t			*cframe;
	int					*boneRef;
#endif

	pinmodel= (mdxmHeader_t *)buffer;
	//
	// read some fields from the binary, but only LittleLong() them when we know this wasn't an already-cached model...
	//
	version = (pinmodel->version);
	size	= (pinmodel->ofsEnd);

	if (!bAlreadyCached)
	{
		LL(version);
		LL(size);
	}

	if (version != MDXM_VERSION) {
		return qfalse;
	}

	mod->type	   = MOD_MDXM;
	mod->dataSize += size;

	qboolean bAlreadyFound = qfalse;
	mdxm = mod->mdxm = (mdxmHeader_t*) //Hunk_Alloc( size );
										RE_RegisterServerModels_Malloc(size, buffer, mod_name, &bAlreadyFound, TAG_MODEL_GLM);

	assert(bAlreadyCached == bAlreadyFound);	// I should probably eliminate 'bAlreadyFound', but wtf?

	if (!bAlreadyFound)
	{
		// horrible new hackery, if !bAlreadyFound then we've just done a tag-morph, so we need to set the
		//	bool reference passed into this function to true, to tell the caller NOT to do an ri.FS_Freefile since
		//	we've hijacked that memory block...
		//
		// Aaaargh. Kill me now...
		//
		bAlreadyCached = qtrue;
		assert( mdxm == buffer );
//		memcpy( mdxm, buffer, size );	// and don't do this now, since it's the same thing

		LL(mdxm->ident);
		LL(mdxm->version);
		LL(mdxm->numLODs);
		LL(mdxm->ofsLODs);
		LL(mdxm->numSurfaces);
		LL(mdxm->ofsSurfHierarchy);
		LL(mdxm->ofsEnd);
	}

	// first up, go load in the animation file we need that has the skeletal animation info for this model
	mdxm->animIndex = RE_RegisterServerModel(va ("%s.gla",mdxm->animName));
	if (!mdxm->animIndex)
	{
		return qfalse;
	}

	mod->numLods = mdxm->numLODs -1 ;	//copy this up to the model for ease of use - it wil get inced after this.

	if (bAlreadyFound)
	{
		return qtrue;	// All done. Stop, go no further, do not LittleLong(), do not pass Go...
	}

	surfInfo = (mdxmSurfHierarchy_t *)( (byte *)mdxm + mdxm->ofsSurfHierarchy);
 	for ( i = 0 ; i < mdxm->numSurfaces ; i++)
	{
		LL(surfInfo->numChildren);
		LL(surfInfo->parentIndex);

		// do all the children indexs
		for (j=0; j<surfInfo->numChildren; j++)
		{
			LL(surfInfo->childIndexes[j]);
		}

		// We will not be using shaders on the server.
		//sh = 0;
		// insert it in the surface list

		surfInfo->shaderIndex = 0;

		RE_RegisterModels_StoreShaderRequest(mod_name, &surfInfo->shader[0], &surfInfo->shaderIndex);

		// find the next surface
		surfInfo = (mdxmSurfHierarchy_t *)( (byte *)surfInfo + (intptr_t)( &((mdxmSurfHierarchy_t *)0)->childIndexes[ surfInfo->numChildren ] ));
  	}

	// swap all the LOD's	(we need to do the middle part of this even for intel, because of shader reg and err-check)
	lod = (mdxmLOD_t *) ( (byte *)mdxm + mdxm->ofsLODs );
	for ( l = 0 ; l < mdxm->numLODs ; l++)
	{
		int	triCount = 0;

		LL(lod->ofsEnd);
		// swap all the surfaces
		surf = (mdxmSurface_t *) ( (byte *)lod + sizeof (mdxmLOD_t) + (mdxm->numSurfaces * sizeof(mdxmLODSurfOffset_t)) );
		for ( i = 0 ; i < mdxm->numSurfaces ; i++)
		{
			LL(surf->numTriangles);
			LL(surf->ofsTriangles);
			LL(surf->numVerts);
			LL(surf->ofsVerts);
			LL(surf->ofsEnd);
			LL(surf->ofsHeader);
			LL(surf->numBoneReferences);
			LL(surf->ofsBoneReferences);
//			LL(surf->maxVertBoneWeights);

			triCount += surf->numTriangles;

			if ( surf->numVerts > SHADER_MAX_VERTEXES ) {
				return qfalse;
			}
			if ( surf->numTriangles*3 > SHADER_MAX_INDEXES ) {
				return qfalse;
			}

			// change to surface identifier
			surf->ident = SF_MDX;

			// register the shaders
#if 0 //#ifndef _M_IX86
//
// optimisation, we don't bother doing this for standard intel case since our data's already in that format...
//
			// FIXME - is this correct?
			// do all the bone reference data
			boneRef = (int *) ( (byte *)surf + surf->ofsBoneReferences );
			for ( j = 0 ; j < surf->numBoneReferences ; j++ )
			{
					LL(boneRef[j]);
			}


			// swap all the triangles
			tri = (mdxmTriangle_t *) ( (byte *)surf + surf->ofsTriangles );
			for ( j = 0 ; j < surf->numTriangles ; j++, tri++ )
			{
				LL(tri->indexes[0]);
				LL(tri->indexes[1]);
				LL(tri->indexes[2]);
			}

			// swap all the vertexes
			v = (mdxmVertex_t *) ( (byte *)surf + surf->ofsVerts );
			for ( j = 0 ; j < surf->numVerts ; j++ )
			{
				v->normal[0] = LittleFloat( v->normal[0] );
				v->normal[1] = LittleFloat( v->normal[1] );
				v->normal[2] = LittleFloat( v->normal[2] );

				v->texCoords[0] = LittleFloat( v->texCoords[0] );
				v->texCoords[1] = LittleFloat( v->texCoords[1] );

				v->numWeights = LittleLong( v->numWeights );
  			    v->offset[0] = LittleFloat( v->offset[0] );
				v->offset[1] = LittleFloat( v->offset[1] );
				v->offset[2] = LittleFloat( v->offset[2] );

				for ( k = 0 ; k < /*v->numWeights*/surf->maxVertBoneWeights ; k++ )
				{
					v->weights[k].boneIndex = LittleLong( v->weights[k].boneIndex );
					v->weights[k].boneWeight = LittleFloat( v->weights[k].boneWeight );
				}
				v = (mdxmVertex_t *)&v->weights[/*v->numWeights*/surf->maxVertBoneWeights];
			}
#endif

			// find the next surface
			surf = (mdxmSurface_t *)( (byte *)surf + surf->ofsEnd );
		}

		// find the next LOD
		lod = (mdxmLOD_t *)( (byte *)lod + lod->ofsEnd );
	}

	return qtrue;
}

/*
====================
RE_RegisterServerModel

Same as RE_RegisterModel, except used by the server to handle ghoul2 instance models.
====================
*/
qhandle_t RE_RegisterServerModel( const char *name ) {
	model_t		*mod;
	unsigned	*buf;
	int			lod;
	int			ident;
	qboolean	loaded;
//	qhandle_t	hModel;
	int			numLoaded;
/*
Ghoul2 Insert Start
*/
	int			hash;
	modelHash_t	*mh;
/*
Ghoul2 Insert End
*/

	if (!r_noServerGhoul2)
	{ //keep it from choking when it gets to these checks in the g2 code. Registering all r_ cvars for the server would be a Bad Thing though.
		r_noServerGhoul2 = ri.Cvar_Get( "r_noserverghoul2", "0", 0, "");
	}

	if ( !name || !name[0] ) {
		return 0;
	}

	if ( strlen( name ) >= MAX_QPATH ) {
		return 0;
	}

	hash = generateHashValue(name, FILE_HASH_SIZE);

	//
	// see if the model is already loaded
	//
	for (mh=mhHashTable[hash]; mh; mh=mh->next) {
		if (Q_stricmp(mh->name, name) == 0) {
			return mh->handle;
		}
	}

	if ( ( mod = R_AllocModel() ) == NULL ) {
		return 0;
	}

	// only set the name after the model has been successfully loaded
	Q_strncpyz( mod->name, name, sizeof( mod->name ) );

	int iLODStart = 0;
	if (strstr (name, ".md3")) {
		iLODStart = MD3_MAX_LODS-1;	// this loads the md3s in reverse so they can be biased
	}
	mod->numLods = 0;

	//
	// load the files
	//
	numLoaded = 0;

	for ( lod = iLODStart; lod >= 0 ; lod-- ) {
		char filename[1024];

		strcpy( filename, name );

		if ( lod != 0 ) {
			char namebuf[80];

			if ( strrchr( filename, '.' ) ) {
				*strrchr( filename, '.' ) = 0;
			}
			sprintf( namebuf, "_%d.md3", lod );
			strcat( filename, namebuf );
		}

		qboolean bAlreadyCached = qfalse;
		if (!RE_RegisterModels_GetDiskFile(filename, (void **)&buf, &bAlreadyCached))
		{
			continue;
		}

		//loadmodel = mod;	// this seems to be fairly pointless

		// important that from now on we pass 'filename' instead of 'name' to all model load functions,
		//	because 'filename' accounts for any LOD mangling etc so guarantees unique lookups for yet more
		//	internal caching...
		//
		ident = *(unsigned *)buf;
		if (!bAlreadyCached)
		{
			LL(ident);
		}

		switch (ident)
		{ //if you're trying to register anything else as a model type on the server, you are out of luck

			case MDXA_IDENT:
				loaded = ServerLoadMDXA( mod, buf, filename, bAlreadyCached );
				break;
			case MDXM_IDENT:
				loaded = ServerLoadMDXM( mod, buf, filename, bAlreadyCached );
				break;
			default:
				goto fail;
		}

		if (!bAlreadyCached){	// important to check!!
			ri.FS_FreeFile (buf);
		}

		if ( !loaded ) {
			if ( lod == 0 ) {
				goto fail;
			} else {
				break;
			}
		} else {
			mod->numLods++;
			numLoaded++;
		}
	}

	if ( numLoaded ) {
		// duplicate into higher lod spots that weren't
		// loaded, in case the user changes r_lodbias on the fly
		for ( lod-- ; lod >= 0 ; lod-- ) {
			mod->numLods++;
			mod->md3[lod] = mod->md3[lod+1];
		}

/*
Ghoul2 Insert Start
*/

	RE_InsertModelIntoHash(name, mod);
	return mod->index;
/*
Ghoul2 Insert End
*/
	}

fail:
	// we still keep the model_t around, so if the model name is asked for
	// again, we won't bother scanning the filesystem
	mod->type = MOD_BAD;
	RE_InsertModelIntoHash(name, mod);
	return 0;
}


/*
====================
RE_RegisterModel

Loads in a model for the given name

Zero will be returned if the model fails to load.
An entry will be retained for failed models as an
optimization to prevent disk rescanning if they are
asked for again.
====================
*/
static qhandle_t RE_RegisterModel_Actual( const char *name ) {
	model_t		*mod;
	unsigned	*buf;
	int			lod;
	int			ident;
	qboolean	loaded;
//	qhandle_t	hModel;
	int			numLoaded;
/*
Ghoul2 Insert Start
*/
	int			hash;
	modelHash_t	*mh;
/*
Ghoul2 Insert End
*/

	if ( !name || !name[0] ) {
		Com_Printf ("RE_RegisterModel: NULL name\n" );
		return 0;
	}

	if ( strlen( name ) >= MAX_QPATH ) {
		ri.Printf( PRINT_DEVELOPER, S_COLOR_RED "Model name exceeds MAX_QPATH\n" );
		return 0;
	}

/*
Ghoul2 Insert Start
*/
//	if (!tr.registered) {
//		Com_Printf (S_COLOR_YELLOW  "RE_RegisterModel (%s) called before ready!\n",name );
//		return 0;
//	}
	//
	// search the currently loaded models
	//
	hash = generateHashValue(name, FILE_HASH_SIZE);

	//
	// see if the model is already loaded
	//
	for (mh=mhHashTable[hash]; mh; mh=mh->next) {
		if (Q_stricmp(mh->name, name) == 0) {
			return mh->handle;
		}
	}

//	for ( hModel = 1 ; hModel < tr.numModels; hModel++ ) {
//		mod = tr.models[hModel];
//		if ( !strcmp( mod->name, name ) ) {
//			if( mod->type == MOD_BAD ) {
//				return 0;
//			}
//			return hModel;
//		}
//	}

	if (name[0] == '#')
	{
		char		temp[MAX_QPATH];

		tr.numBSPModels++;
		Com_sprintf(temp, MAX_QPATH, "*%d-0", tr.numBSPModels);
		hash = generateHashValue(temp, FILE_HASH_SIZE);
		for (mh=mhHashTable[hash]; mh; mh=mh->next)
		{
			if (Q_stricmp(mh->name, temp) == 0)
			{
				return mh->handle;
			}
		}

		return 0;
	}

	if (name[0] == '*')
	{	// don't create a bad model for a bsp model
		if (Q_stricmp(name, "*default.gla"))
		{
			return 0;
		}
	}

/*
Ghoul2 Insert End
*/

	// allocate a new model_t

	if ( ( mod = R_AllocModel() ) == NULL ) {
		Com_Printf (S_COLOR_YELLOW  "RE_RegisterModel: R_AllocModel() failed for '%s'\n", name);
		return 0;
	}

	// only set the name after the model has been successfully loaded
	Q_strncpyz( mod->name, name, sizeof( mod->name ) );

	int iLODStart = 0;
	if (strstr (name, ".md3")) {
		iLODStart = MD3_MAX_LODS-1;	// this loads the md3s in reverse so they can be biased
	}
	mod->numLods = 0;

	//
	// load the files
	//
	numLoaded = 0;

	for ( lod = iLODStart; lod >= 0 ; lod-- ) {
		char filename[1024];

		strcpy( filename, name );

		if ( lod != 0 ) {
			char namebuf[80];

			if ( strrchr( filename, '.' ) ) {
				*strrchr( filename, '.' ) = 0;
			}
			sprintf( namebuf, "_%d.md3", lod );
			strcat( filename, namebuf );
		}

		qboolean bAlreadyCached = qfalse;
		if (!RE_RegisterModels_GetDiskFile(filename, (void **)&buf, &bAlreadyCached))
		{
			continue;
		}

		//loadmodel = mod;	// this seems to be fairly pointless

		// important that from now on we pass 'filename' instead of 'name' to all model load functions,
		//	because 'filename' accounts for any LOD mangling etc so guarantees unique lookups for yet more
		//	internal caching...
		//
		ident = *(unsigned *)buf;
		if (!bAlreadyCached)
		{
			LL(ident);
		}

		switch (ident)
		{
			// if you add any new types of model load in this switch-case, tell me,
			//	or copy what I've done with the cache scheme (-ste).
			//
			case MDXA_IDENT:
				loaded = R_LoadMDXA( mod, buf, filename, bAlreadyCached );
				break;

			case MDXM_IDENT:
				loaded = R_LoadMDXM( mod, buf, filename, bAlreadyCached );
				break;

			case MD3_IDENT:
				loaded = R_LoadMD3( mod, lod, buf, filename, bAlreadyCached );
				break;

			default:
				Com_Printf (S_COLOR_YELLOW"RE_RegisterModel: unknown fileid for %s\n", filename);
				goto fail;
		}

		if (!bAlreadyCached){	// important to check!!
			ri.FS_FreeFile (buf);
		}

		if ( !loaded ) {
			if ( lod == 0 ) {
				goto fail;
			} else {
				break;
			}
		} else {
			mod->numLods++;
			numLoaded++;
			// if we have a valid model and are biased
			// so that we won't see any higher detail ones,
			// stop loading them
			if ( lod <= r_lodbias->integer ) {
				break;
			}
		}
	}

	if ( numLoaded ) {
		// duplicate into higher lod spots that weren't
		// loaded, in case the user changes r_lodbias on the fly
		for ( lod-- ; lod >= 0 ; lod-- ) {
			mod->numLods++;
			mod->md3[lod] = mod->md3[lod+1];
		}

/*
Ghoul2 Insert Start
*/

#ifdef _DEBUG
	if (r_noPrecacheGLA && r_noPrecacheGLA->integer && ident == MDXA_IDENT)
	{ //I expect this will cause leaks, but I don't care because it's a debugging utility.
		return mod->index;
	}
#endif

	RE_InsertModelIntoHash(name, mod);
	return mod->index;
/*
Ghoul2 Insert End
*/
	}
#ifdef _DEBUG
	else {
		Com_Printf (S_COLOR_YELLOW"RE_RegisterModel: couldn't load %s\n", name);
	}
#endif

fail:
	// we still keep the model_t around, so if the model name is asked for
	// again, we won't bother scanning the filesystem
	mod->type = MOD_BAD;
	RE_InsertModelIntoHash(name, mod);
	return 0;
}


// wrapper function needed to avoid problems with mid-function returns so I can safely use this bool to tell the
//	z_malloc-fail recovery code whether it's safe to ditch any model caches...
//
qboolean gbInsideRegisterModel = qfalse;
qhandle_t RE_RegisterModel( const char *name )
{
	const qboolean bWhatitwas = gbInsideRegisterModel;
	gbInsideRegisterModel = qtrue;	// !!!!!!!!!!!!!!

		qhandle_t q = RE_RegisterModel_Actual( name );

	gbInsideRegisterModel = bWhatitwas;

	return q;
}




/*
=================
R_LoadMD3
=================
*/
static qboolean R_LoadMD3 (model_t *mod, int lod, void *buffer, const char *mod_name, qboolean &bAlreadyCached ) {
	int					i, j;
	md3Header_t			*pinmodel;
	md3Surface_t		*surf;
	int					version;
	int					size;

#if 0 //#ifndef _M_IX86
	md3Frame_t			*frame;
	md3Triangle_t		*tri;
	md3St_t				*st;
	md3XyzNormal_t		*xyz;
	md3Tag_t			*tag;
#endif


	pinmodel= (md3Header_t *)buffer;
	//
	// read some fields from the binary, but only LittleLong() them when we know this wasn't an already-cached model...
	//
	version = pinmodel->version;
	size	= pinmodel->ofsEnd;

	if (!bAlreadyCached)
	{
		LL(version);
		LL(size);
	}

	if (version != MD3_VERSION) {
		Com_Printf (S_COLOR_YELLOW  "R_LoadMD3: %s has wrong version (%i should be %i)\n",
				 mod_name, version, MD3_VERSION);
		return qfalse;
	}

	mod->type      = MOD_MESH;
	mod->dataSize += size;

	qboolean bAlreadyFound = qfalse;
	mod->md3[lod] = (md3Header_t *) //Hunk_Alloc( size );
										RE_RegisterModels_Malloc(size, buffer, mod_name, &bAlreadyFound, TAG_MODEL_MD3);

	assert(bAlreadyCached == bAlreadyFound);	// I should probably eliminate 'bAlreadyFound', but wtf?

	if (!bAlreadyFound)
	{
		// horrible new hackery, if !bAlreadyFound then we've just done a tag-morph, so we need to set the
		//	bool reference passed into this function to true, to tell the caller NOT to do an ri.FS_Freefile since
		//	we've hijacked that memory block...
		//
		// Aaaargh. Kill me now...
		//
		bAlreadyCached = qtrue;
		assert( mod->md3[lod] == buffer );
//		memcpy( mod->md3[lod], buffer, size );	// and don't do this now, since it's the same thing

		LL(mod->md3[lod]->ident);
		LL(mod->md3[lod]->version);
		LL(mod->md3[lod]->numFrames);
		LL(mod->md3[lod]->numTags);
		LL(mod->md3[lod]->numSurfaces);
		LL(mod->md3[lod]->ofsFrames);
		LL(mod->md3[lod]->ofsTags);
		LL(mod->md3[lod]->ofsSurfaces);
		LL(mod->md3[lod]->ofsEnd);
	}

	if ( mod->md3[lod]->numFrames < 1 ) {
		Com_Printf (S_COLOR_YELLOW  "R_LoadMD3: %s has no frames\n", mod_name );
		return qfalse;
	}

	if (bAlreadyFound)
	{
		return qtrue;	// All done. Stop, go no further, do not pass Go...
	}

#if 0 //#ifndef _M_IX86
	//
	// optimisation, we don't bother doing this for standard intel case since our data's already in that format...
	//

	// swap all the frames
    frame = (md3Frame_t *) ( (byte *)mod->md3[lod] + mod->md3[lod]->ofsFrames );
    for ( i = 0 ; i < mod->md3[lod]->numFrames ; i++, frame++) {
    	frame->radius = LittleFloat( frame->radius );
        for ( j = 0 ; j < 3 ; j++ ) {
            frame->bounds[0][j] = LittleFloat( frame->bounds[0][j] );
            frame->bounds[1][j] = LittleFloat( frame->bounds[1][j] );
	    	frame->localOrigin[j] = LittleFloat( frame->localOrigin[j] );
        }
	}

	// swap all the tags
    tag = (md3Tag_t *) ( (byte *)mod->md3[lod] + mod->md3[lod]->ofsTags );
    for ( i = 0 ; i < mod->md3[lod]->numTags * mod->md3[lod]->numFrames ; i++, tag++) {
        for ( j = 0 ; j < 3 ; j++ ) {
			tag->origin[j] = LittleFloat( tag->origin[j] );
			tag->axis[0][j] = LittleFloat( tag->axis[0][j] );
			tag->axis[1][j] = LittleFloat( tag->axis[1][j] );
			tag->axis[2][j] = LittleFloat( tag->axis[2][j] );
        }
	}
#endif

	// swap all the surfaces
	surf = (md3Surface_t *) ( (byte *)mod->md3[lod] + mod->md3[lod]->ofsSurfaces );
	for ( i = 0 ; i < mod->md3[lod]->numSurfaces ; i++) {
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

		if ( surf->numVerts > SHADER_MAX_VERTEXES ) {
			Com_Error (ERR_DROP, "R_LoadMD3: %s has more than %i verts on a surface (%i)",
				mod_name, SHADER_MAX_VERTEXES, surf->numVerts );
		}
		if ( surf->numTriangles*3 > SHADER_MAX_INDEXES ) {
			Com_Error (ERR_DROP, "R_LoadMD3: %s has more than %i triangles on a surface (%i)",
				mod_name, SHADER_MAX_INDEXES / 3, surf->numTriangles );
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
#if 0 //#ifndef _M_IX86
//
// optimisation, we don't bother doing this for standard intel case since our data's already in that format...
//

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
            st->st[0] = LittleFloat( st->st[0] );
            st->st[1] = LittleFloat( st->st[1] );
        }

		// swap all the XyzNormals
        xyz = (md3XyzNormal_t *) ( (byte *)surf + surf->ofsXyzNormals );
        for ( j = 0 ; j < surf->numVerts * surf->numFrames ; j++, xyz++ )
		{
            xyz->xyz[0] = LittleShort( xyz->xyz[0] );
            xyz->xyz[1] = LittleShort( xyz->xyz[1] );
            xyz->xyz[2] = LittleShort( xyz->xyz[2] );

            xyz->normal = LittleShort( xyz->normal );
        }
#endif

		// find the next surface
		surf = (md3Surface_t *)( (byte *)surf + surf->ofsEnd );
	}

	return qtrue;
}


//=============================================================================

void R_SVModelInit()
{
	R_ModelInit();
}

/*
===============
R_ModelInit
===============
*/
void R_ModelInit( void )
{
	model_t		*mod;

	if(!CachedModels)
	{
		CachedModels = new CachedModels_t;
	}

	// leave a space for NULL model
	tr.numModels = 0;
	memset(mhHashTable, 0, sizeof(mhHashTable));

	mod = R_AllocModel();
	mod->type = MOD_BAD;
}

extern void KillTheShaderHashTable(void);
void RE_HunkClearCrap(void)
{ //get your dirty sticky assets off me, you damn dirty hunk!
	KillTheShaderHashTable();
	tr.numModels = 0;
	memset(mhHashTable, 0, sizeof(mhHashTable));
	tr.numShaders = 0;
	tr.numSkins = 0;
}

void R_ModelFree(void)
{
	if(CachedModels) {
		RE_RegisterModels_DeleteAll();
		delete CachedModels;
		CachedModels = NULL;
	}
}



/*
================
R_Modellist_f
================
*/
void R_Modellist_f( void ) {
	int		i, j;
	model_t	*mod;
	int		total;
	int		lods;

	total = 0;
	for ( i = 1 ; i < tr.numModels; i++ ) {
		mod = tr.models[i];
		lods = 1;
		for ( j = 1 ; j < MD3_MAX_LODS ; j++ ) {
			if ( mod->md3[j] && mod->md3[j] != mod->md3[j-1] ) {
				lods++;
			}
		}
		Com_Printf ("%8i : (%i) %s\n",mod->dataSize, lods, mod->name );
		total += mod->dataSize;
	}
	Com_Printf ("%8i : Total models\n", total );

#if	0		// not working right with new hunk
	if ( tr.world ) {
		Com_Printf ("\n%8i : %s\n", tr.world->dataSize, tr.world->name );
	}
#endif
}


//=============================================================================


/*
================
R_GetTag
================
*/
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

/*
================
R_LerpTag
================
*/
int R_LerpTag( orientation_t *tag, qhandle_t handle, int startFrame, int endFrame,
					 float frac, const char *tagName ) {
	md3Tag_t	*start, *end;
	int		i;
	float		frontLerp, backLerp;
	model_t		*model;

	model = R_GetModelByHandle( handle );
	if ( !model->md3[0] ) {
		AxisClear( tag->axis );
		VectorClear( tag->origin );
		return qfalse;
	}

	start = R_GetTag( model->md3[0], startFrame, tagName );
	end = R_GetTag( model->md3[0], endFrame, tagName );
	if ( !start || !end ) {
		AxisClear( tag->axis );
		VectorClear( tag->origin );
		return qfalse;
	}

	frontLerp = frac;
	backLerp = 1.0f - frac;

	for ( i = 0 ; i < 3 ; i++ ) {
		tag->origin[i] = start->origin[i] * backLerp +  end->origin[i] * frontLerp;
		tag->axis[0][i] = start->axis[0][i] * backLerp +  end->axis[0][i] * frontLerp;
		tag->axis[1][i] = start->axis[1][i] * backLerp +  end->axis[1][i] * frontLerp;
		tag->axis[2][i] = start->axis[2][i] * backLerp +  end->axis[2][i] * frontLerp;
	}
	VectorNormalize( tag->axis[0] );
	VectorNormalize( tag->axis[1] );
	VectorNormalize( tag->axis[2] );
	return qtrue;
}


/*
====================
R_ModelBounds
====================
*/
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


