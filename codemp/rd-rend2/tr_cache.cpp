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
	FileHash_t *fh = (FileHash_t *)ri->Hunk_Alloc( sizeof(FileHash_t), h_low );
	fh->handle = handle;
	Q_strncpyz( fh->fileName, fileName, sizeof(fh->fileName) );
	loaded.insert(make_pair<std::string, FileHash_t>(fileName, *fh));
}

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

	CachedFile_t pFile;
	pFile = cache[sFileName];	// this might cause an assert?? (I dunno, works fine in Raven code..)

	if(1)
	{
		*pbAlreadyCached = qfalse;
		ri->FS_ReadFile( pFileName, ppFileBuffer );
		qboolean bSuccess = !!(*ppFileBuffer)?qtrue:qfalse;

		if( bSuccess )
			ri->Printf( PRINT_DEVELOPER, "C_LoadFile(): Loaded %s from disk\n", pFileName );

		return bSuccess;
	}
	else
	{
		*ppFileBuffer = pFile.pDiskImage;
		*pbAlreadyCached = qtrue;
		return qtrue;
	}
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

	CachedFile_t pFile = cache[sModelName];

	if( !pFile.pDiskImage )
	{ /* Create this image. */
		/* 
		 * We aren't going to do that ugly VV hack that supposedly reduces mem overhead.
		 * Said hack can cause heap corruption (!) if not properly checked, and I suspect
		 * that I'll forget to do it later. Forgive me for not cutting down on overhead,
		 * but I figured it was marginal, considering it was designed in order to cut back
		 * on mem usage on machines that ran with like 64MB RAM... --eez
		 */
		pvDiskBuffer		= Z_Malloc(iSize, eTag, qfalse);
		pFile.pDiskImage	= pvDiskBuffer;
		pFile.iAllocSize	= iSize;

		if( ri->FS_FileIsInPAK( psModelFileName, &iChecksum ) )
			pFile.iPAKChecksum = iChecksum;  /* Otherwise, it will be -1. */

		*bAlreadyFound = qfalse;
	}
	else
	{
		/*
		 * Already found it.
		 * TODO: shader caching.
		 */
		*bAlreadyFound = qtrue;
	}

	return pFile.pDiskImage;
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
		it = cache.erase(it);
	}
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

			it = cache.erase(it++);
			bEraseOccurred = qtrue;
		}

		if( !bEraseOccurred )
			++it;
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
	//int iLoadedModelBytes			=	GetModelDataAllocSize();
	//const int iMaxModelBytes		=	r_modelpoolmegs->integer * 1024 * 1024;
	qboolean bAtLeastOneModelFreed	= qfalse;

	/*if( iLoadedModelBytes > iMaxModelBytes )
	{
		Com_Printf("CModelCacheManager::LevelLoadEnd(): iLoadedModelBytes > iMaxModelBytes (raise r_modelpoolmegs?)\n");
		return bAtLeastOneModelFreed;
	}*/

	ri->Printf( PRINT_DEVELOPER, S_COLOR_GREEN "CModelCacheManager::LevelLoadEnd():\n");

	for(auto it = cache.begin(); it != cache.end(); ++it)
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
			// FIXME: like right here?
			it = cache.erase(it);
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