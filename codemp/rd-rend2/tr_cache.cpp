// tr_cache.cpp - Cache models, images, and more..

#include "tr_local.h"

// This differs significantly from Raven's own caching code.
// For starters, we are allowed to use ri-> whatever because we don't care about running on dedicated (use rd-vanilla!)
using namespace std;

CImageCacheManager *CImgCache = new CImageCacheManager();
CModelCacheManager *CModelCache = new CModelCacheManager();

static int GetModelDataAllocSize(void)
{
	return	Z_MemSize( TAG_MODEL_MD3) +
			Z_MemSize( TAG_MODEL_GLM) +
			Z_MemSize( TAG_MODEL_GLA);
}

/*
 * CCacheManager::SearchLoaded
 * Return -1 if asset not currently loaded, return positive qhandle_t if found
 */
qhandle_t CCacheManager::SearchLoaded( const char *fileName )
{
	loadedMap_t::iterator it = loaded.find(fileName);
	if( it == loaded.end() )
		return -1; // asset not found
	return it->second.handle;
}

/*
 * CCacheManager::InsertLoaded
 * We have a loaded model/image, let's insert it into the list of loaded models
 */
void CCacheManager::InsertLoaded( const char *fileName, qhandle_t handle )
{
	FileHash_t fh;
	fh.handle = handle;
	Q_strncpyz( fh.fileName, fileName, sizeof(fh.fileName) );
	loaded.insert(make_pair(fileName, fh));
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

/*
 * CCacheManager::LoadFile
 * Load the file and chuck the contents into ppFileBuffer, OR
 * if we're cached already, chuck cached contents into ppFileBuffer
 * and set *pbAlreadyCached to qtrue (otherwise, *pbAlreadyCached = false)
 */
qboolean CCacheManager::LoadFile( const char *pFileName, void **ppFileBuffer, qboolean *pbAlreadyCached )
{
	char sFileName[MAX_QPATH];

	Q_strncpyz(sFileName, pFileName, MAX_QPATH);
	Q_strlwr  (sFileName);

	auto cacheEntry = cache.find (sFileName);
	if ( cacheEntry != cache.end() )
	{
		*ppFileBuffer = cacheEntry->second.pDiskImage;
		*pbAlreadyCached = qtrue;

		return qtrue;
	}

	*pbAlreadyCached = qfalse;

	// special case intercept first...
	if (!strcmp (sDEFAULT_GLA_NAME ".gla", pFileName))
	{
		// return fake params as though it was found on disk...
		void *pvFakeGLAFile = Z_Malloc (sizeof (FakeGLAFile), TAG_FILESYS, qfalse);

		memcpy (pvFakeGLAFile, &FakeGLAFile[0], sizeof (FakeGLAFile));
		*ppFileBuffer = pvFakeGLAFile;

		return qtrue;	
	}

	int len = ri->FS_ReadFile( pFileName, ppFileBuffer );
	if ( *ppFileBuffer == NULL )
	{
		return qfalse;
	}

	ri->Printf( PRINT_DEVELOPER, "C_LoadFile(): Loaded %s from disk\n", pFileName );

	return qtrue;
}

/*
 * CCacheManager::Allocate
 * Allocate appropriate memory for stuff dealing with cached images
 * FIXME: only applies to models?
 */
void* CCacheManager::Allocate( int iSize, void *pvDiskBuffer, const char *psModelFileName, qboolean *bAlreadyFound, memtag_t eTag )
{
	int		iChecksum;
	char	sModelName[MAX_QPATH];

	/* Standard NULL checking. */
	if( !psModelFileName || !psModelFileName[0] )
		return NULL;

	if( !bAlreadyFound )
		return NULL;

	/* Convert psModelFileName to lowercase */
	Q_strncpyz(sModelName, psModelFileName, MAX_QPATH);
	Q_strlwr  (sModelName);

	CachedFile_t *pFile;
	auto cacheEntry = cache.find (sModelName);

	if (cacheEntry == cache.end())
	{ /* Create this image. */
		pFile = &cache[sModelName];

		if( pvDiskBuffer )
			Z_MorphMallocTag( pvDiskBuffer, eTag );
		else
			pvDiskBuffer		= Z_Malloc(iSize, eTag, qfalse);
		pFile->pDiskImage	= pvDiskBuffer;
		pFile->iAllocSize	= iSize;

		if( ri->FS_FileIsInPAK( psModelFileName, &iChecksum ) )
			pFile->iPAKChecksum = iChecksum;  /* Otherwise, it will be -1. */

		*bAlreadyFound = qfalse;
	}
	else
	{
		/*
		 * Already found it.
		 * TODO: shader caching.
		 */
		*bAlreadyFound = qtrue;
		pFile = &cacheEntry->second;
	}

	pFile->iLevelLastUsedOn = tr.currentLevel;

	return pFile->pDiskImage;
}

/*
 * CCacheManager::DeleteAll
 * Clears out the cache (done on renderer shutdown I suppose)
 */
void CCacheManager::DeleteAll( void )
{
	for( auto it = cache.begin(); it != cache.end(); ++it )
	{
		Z_Free(it->second.pDiskImage);
	}

	cache.clear();
	loaded.clear();
}

/*
 * CCacheManager::DumpNonPure
 * Scans the cache for assets which don't match the checksum, and dumps
 * those that don't match.
 */
void CCacheManager::DumpNonPure( void )
{
	ri->Printf( PRINT_DEVELOPER,  "CCacheManager::DumpNonPure():\n");

	for(assetCache_t::iterator it = cache.begin(); it != cache.end(); /* empty */ )
	{
		int iChecksum;
		int iInPak = ri->FS_FileIsInPAK( it->first.c_str(), &iChecksum );
		qboolean bEraseOccurred = qfalse;

		if( iInPak == -1 || iChecksum != it->second.iPAKChecksum )
		{
			/* Erase the file because it doesn't match the checksum */
			ri->Printf( PRINT_DEVELOPER, "Dumping none pure model \"%s\"", it->first );

			if( it->second.pDiskImage )
				Z_Free( it->second.pDiskImage );

			it = cache.erase(it);
		}
		else
		{
			++it;
		}
	}

	ri->Printf( PRINT_DEVELOPER, "CCacheManager::DumpNonPure(): Ok\n");	
}

/*
 * LevelLoadEnd virtual funcs
 */

/*
 * This function is /badly/ designed in vanilla. For starters, LETS NOT USE ANY INCREMENTERS BECAUSE THATS COOL
 * Secondly, it randomly decides to keep track of the amount of model memory and break out of the loop if we run
 * higher than r_modelpoolmegs...thing is, logically that doesn't make any sense, since you're freeing memory
 * here, not allocating more of it. Fail.
 */
qboolean CModelCacheManager::LevelLoadEnd( qboolean bDeleteEverythingNotUsedThisLevel )
{
	qboolean bAtLeastOneModelFreed	= qfalse;

	ri->Printf( PRINT_DEVELOPER, S_COLOR_GREEN "CModelCacheManager::LevelLoadEnd():\n");

	for(auto it = cache.begin(); it != cache.end(); /* empty */)
	{
		CachedFile_t pFile			= it->second;
		bool bDeleteThis			= false;

		if( bDeleteEverythingNotUsedThisLevel )
			bDeleteThis = ( pFile.iLevelLastUsedOn != tr.currentLevel );
		else
			bDeleteThis = ( pFile.iLevelLastUsedOn < tr.currentLevel );

		if( bDeleteThis )
		{
			const char *psModelName = it->first.c_str();

			ri->Printf( PRINT_DEVELOPER, S_COLOR_GREEN "Dumping \"%s\"", psModelName);

			if( pFile.pDiskImage )
			{
				Z_Free( pFile.pDiskImage );
				bAtLeastOneModelFreed = qtrue;	// FIXME: is this correct? shouldn't it be in the next lower scope?
			}

			it = cache.erase(it);
		}
		else
		{
			++it;
		}
	}

	ri->Printf( PRINT_DEVELOPER, S_COLOR_GREEN "CModelCacheManager::LevelLoadEnd(): Ok\n");	

	return bAtLeastOneModelFreed;
}

qboolean CImageCacheManager::LevelLoadEnd( qboolean bDeleteEverythingNotUsedThisLevel /* unused */ )
{
	return qtrue;
}

/*
 * Wrappers for the above funcs so they export properly.
 */

qboolean C_Models_LevelLoadEnd( qboolean bDeleteEverythingNotUsedInThisLevel )
{
	return CModelCache->LevelLoadEnd( bDeleteEverythingNotUsedInThisLevel );
}

qboolean C_Images_LevelLoadEnd( void )
{
	return CImgCache->LevelLoadEnd( qfalse );
}

/*
 * Shader storage and retrieval, unique to the model caching
 */

void CModelCacheManager::StoreShaderRequest( const char *psModelFileName, const char *psShaderName, int *piShaderIndexPoke )
{
	char sModelName[MAX_QPATH];

	Q_strncpyz(sModelName, psModelFileName, sizeof(sModelName));
	Q_strlwr  (sModelName);

	auto cacheEntry = cache.find (sModelName);
	if ( cacheEntry == cache.end() )
	{
		return;
	}

	CachedFile_t &rFile = cacheEntry->second;
	if( rFile.pDiskImage == NULL )
	{
		/* Shouldn't even happen. */
		assert(0);
		return;
	}
	
	int iNameOffset =		  psShaderName		- (char *)rFile.pDiskImage;
	int iPokeOffset = (char*) piShaderIndexPoke	- (char *)rFile.pDiskImage;

	rFile.shaderCache.push_back( make_pair( iNameOffset, iPokeOffset ) );
}

void CModelCacheManager::AllocateShaders( const char *psFileName )
{
	// if we already had this model entry, then re-register all the shaders it wanted...
	//
	char sModelName[MAX_QPATH];

	Q_strncpyz(sModelName, psFileName, sizeof(sModelName));
	Q_strlwr  (sModelName);

	auto cacheEntry = cache.find (sModelName);
	if ( cacheEntry == cache.end() )
	{
		return;
	}

	CachedFile_t &rFile = cacheEntry->second;

	if( rFile.pDiskImage == NULL )
	{
		/* Shouldn't even happen. */
		assert(0);
		return;
	}

	for( auto storedShader = rFile.shaderCache.begin(); storedShader != rFile.shaderCache.end(); ++storedShader )
	{
		int iShaderNameOffset	= storedShader->first;
		int iShaderPokeOffset	= storedShader->second;

		char *psShaderName		=		  &((char*)rFile.pDiskImage)[iShaderNameOffset];
		int  *piShaderPokePtr	= (int *) &((char*)rFile.pDiskImage)[iShaderPokeOffset];

		shader_t *sh = R_FindShader( psShaderName, lightmapsNone, stylesDefault, qtrue );
	            
		if ( sh->defaultShader ) 
			*piShaderPokePtr = 0;
		else 
			*piShaderPokePtr = sh->index;
	}
}

/*
 * These processes occur outside of the CacheManager class. They are exported by the renderer.
 */

void C_LevelLoadBegin(const char *psMapName, ForceReload_e eForceReload)
{
	static char sPrevMapName[MAX_QPATH]={0};
	bool bDeleteModels = eForceReload == eForceReload_MODELS || eForceReload == eForceReload_ALL;

	if( bDeleteModels )
		CModelCache->DeleteAll();
	else if( ri->Cvar_VariableIntegerValue( "sv_pure" ) )
		CModelCache->DumpNonPure();

	tr.numBSPModels = 0;

	//R_Images_DeleteLightMaps();

	/* If we're switching to the same level, don't increment current level */
	if (Q_stricmp( psMapName,sPrevMapName ))
	{
		Q_strncpyz( sPrevMapName, psMapName, sizeof(sPrevMapName) );
		tr.currentLevel++;
	}
}

int C_GetLevel( void )
{
	return tr.currentLevel;
}

void C_LevelLoadEnd( void )
{
	CModelCache->LevelLoadEnd( qfalse );
	CImgCache->LevelLoadEnd( qfalse );
	ri->SND_RegisterAudio_LevelLoadEnd( qfalse );
	ri->S_RestartMusic();
}