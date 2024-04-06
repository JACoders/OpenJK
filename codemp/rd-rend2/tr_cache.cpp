// tr_cache.cpp - Cache models, images, and more..

#include "tr_local.h"
#include "tr_cache.h"
#include <algorithm>

namespace
{

void NormalizePath( char *out, const char *path, size_t outSize )
{
	assert(outSize == MAX_QPATH);
	Q_strncpyz(out, path, outSize);
	Q_strlwr(out);
}

}

// This differs significantly from Raven's own caching code.
// For starters, we are allowed to use ri-> whatever because we don't care about running on dedicated (use rd-vanilla!)

CModelCacheManager *CModelCache = new CModelCacheManager();

CachedFile::CachedFile()
	: pDiskImage(nullptr)
	, iLevelLastUsedOn(0)
	, iPAKChecksum(-1)
	, iAllocSize(0)
{
}

CModelCacheManager::FileCache::iterator CModelCacheManager::FindFile( const char *path )
{
	return std::find_if(
		std::begin(files), std::end(files), [path]( const CachedFile& file )
		{
			return strcmp(path, file.path) == 0;
		});
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

qboolean CModelCacheManager::LoadFile( const char *pFileName, void **ppFileBuffer, qboolean *pbAlreadyCached )
{
	char path[MAX_QPATH];
	NormalizePath(path, pFileName, sizeof(path));

	auto cacheEntry = FindFile(path);
	if ( cacheEntry != std::end(files) )
	{
		*ppFileBuffer = cacheEntry->pDiskImage;
		*pbAlreadyCached = qtrue;

		return qtrue;
	}

	*pbAlreadyCached = qfalse;

	// special case intercept first...
	if (!strcmp (sDEFAULT_GLA_NAME ".gla", path))
	{
		// return fake params as though it was found on disk...
		void *pvFakeGLAFile = Z_Malloc(sizeof (FakeGLAFile), TAG_FILESYS, qfalse);

		memcpy(pvFakeGLAFile, &FakeGLAFile[0], sizeof (FakeGLAFile));
		*ppFileBuffer = pvFakeGLAFile;

		return qtrue;
	}

	int len = ri.FS_ReadFile(path, ppFileBuffer);
	if ( len == -1 || *ppFileBuffer == NULL )
	{
		return qfalse;
	}

	ri.Printf( PRINT_DEVELOPER, "C_LoadFile(): Loaded %s from disk\n", pFileName );

	return qtrue;
}


void* CModelCacheManager::Allocate( int iSize, void *pvDiskBuffer, const char *psModelFileName, qboolean *bAlreadyFound, memtag_t eTag )
{
	int		iChecksum;
	char	sModelName[MAX_QPATH];

	/* Standard NULL checking. */
	if( !psModelFileName || !psModelFileName[0] )
		return NULL;

	if( !bAlreadyFound )
		return NULL;

	NormalizePath(sModelName, psModelFileName, sizeof(sModelName));

	CachedFile *pFile = nullptr;
	auto cacheEntry = FindFile(sModelName);
	if (cacheEntry == files.end())
	{
		/* Create this image. */

		if( pvDiskBuffer )
			Z_MorphMallocTag( pvDiskBuffer, eTag );
		else
			pvDiskBuffer = Z_Malloc(iSize, eTag, qfalse);

		files.emplace_back();
		pFile = &files.back();
		pFile->pDiskImage = pvDiskBuffer;
		pFile->iAllocSize = iSize;
		Q_strncpyz(pFile->path, sModelName, sizeof(pFile->path));

		if( ri.FS_FileIsInPAK( sModelName, &iChecksum ) )
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
		pFile = &(*cacheEntry);
	}

	pFile->iLevelLastUsedOn = tr.currentLevel;

	return pFile->pDiskImage;
}

/*
 * Clears out the cache (done on renderer shutdown I suppose)
 */
void CModelCacheManager::DeleteAll( void )
{
	for ( auto& file : files )
	{
		Z_Free(file.pDiskImage);
	}

	FileCache().swap(files);
	AssetCache().swap(assets);
}

/*
 * Scans the cache for assets which don't match the checksum, and dumps
 * those that don't match.
 */
void CModelCacheManager::DumpNonPure( void )
{
	ri.Printf( PRINT_DEVELOPER,  "CCacheManager::DumpNonPure():\n");

	for ( auto it = files.begin(); it != files.end(); /* empty */ )
	{
		int iChecksum;
		int iInPak = ri.FS_FileIsInPAK( it->path, &iChecksum );

		if( iInPak == -1 || iChecksum != it->iPAKChecksum )
		{
			/* Erase the file because it doesn't match the checksum */
			ri.Printf( PRINT_DEVELOPER, "Dumping none pure model \"%s\"", it->path );

			if( it->pDiskImage )
				Z_Free( it->pDiskImage );

			it = files.erase(it);
		}
		else
		{
			++it;
		}
	}

	ri.Printf( PRINT_DEVELOPER, "CCacheManager::DumpNonPure(): Ok\n");
}

CModelCacheManager::AssetCache::iterator CModelCacheManager::FindAsset( const char *path )
{
	return std::find_if(
		std::begin(assets), std::end(assets), [path]( const Asset& asset )
		{
			return strcmp(path, asset.path) == 0;
		});
}

qhandle_t CModelCacheManager::GetModelHandle( const char *fileName )
{
	char path[MAX_QPATH];
	NormalizePath(path, fileName, sizeof(path));

	const auto it = FindAsset(path);
	if( it == std::end(assets) )
		return -1; // asset not found

	return it->handle;
}

void CModelCacheManager::InsertModelHandle( const char *fileName, qhandle_t handle )
{
	char path[MAX_QPATH];
	NormalizePath(path, fileName, sizeof(path));

	Asset asset;
	asset.handle = handle;
	Q_strncpyz(asset.path, path, sizeof(asset.path));
	assets.emplace_back(asset);
}

qboolean CModelCacheManager::LevelLoadEnd( qboolean deleteUnusedByLevel )
{
	qboolean bAtLeastOneModelFreed	= qfalse;

	ri.Printf( PRINT_DEVELOPER, S_COLOR_GREEN "CModelCacheManager::LevelLoadEnd():\n");

	for ( auto it = files.begin(); it != files.end(); /* empty */ )
	{
		bool bDeleteThis = false;

		if( deleteUnusedByLevel )
			bDeleteThis = (it->iLevelLastUsedOn != tr.currentLevel);
		else
			bDeleteThis = (it->iLevelLastUsedOn < tr.currentLevel);

		if( bDeleteThis )
		{
			ri.Printf( PRINT_DEVELOPER, S_COLOR_GREEN "Dumping \"%s\"", it->path);
			if( it->pDiskImage )
			{
				Z_Free( it->pDiskImage );
				bAtLeastOneModelFreed = qtrue;	// FIXME: is this correct? shouldn't it be in the next lower scope?
			}

			it = files.erase(it);
		}
		else
		{
			++it;
		}
	}

	ri.Printf( PRINT_DEVELOPER, S_COLOR_GREEN "CModelCacheManager::LevelLoadEnd(): Ok\n");

	return bAtLeastOneModelFreed;
}

/*
 * Wrappers for the above funcs so they export properly.
 */

qboolean C_Models_LevelLoadEnd( qboolean deleteUnusedByLevel )
{
	return CModelCache->LevelLoadEnd(deleteUnusedByLevel);
}

qboolean C_Images_LevelLoadEnd()
{
	return qfalse;
}

/*
 * Shader storage and retrieval, unique to the model caching
 */

void CModelCacheManager::StoreShaderRequest( const char *psModelFileName, const char *psShaderName, int *piShaderIndexPoke )
{
	char sModelName[MAX_QPATH];
	NormalizePath(sModelName, psModelFileName, sizeof(sModelName));

	auto file = FindFile(sModelName);
	if ( file == files.end() )
	{
		return;
	}

	if( file->pDiskImage == NULL )
	{
		/* Shouldn't even happen. */
		assert(0);
		return;
	}

	int iNameOffset =		  psShaderName		- (char *)file->pDiskImage;
	int iPokeOffset = (char*) piShaderIndexPoke	- (char *)file->pDiskImage;

	file->shaderCache.push_back(ShaderCacheEntry(iNameOffset, iPokeOffset));
}

void CModelCacheManager::AllocateShaders( const char *psFileName )
{
	// if we already had this model entry, then re-register all the shaders it wanted...

	char sModelName[MAX_QPATH];
	NormalizePath(sModelName, psFileName, sizeof(sModelName));

	auto file = FindFile(sModelName);
	if ( file == files.end() )
	{
		return;
	}

	if( file->pDiskImage == NULL )
	{
		/* Shouldn't even happen. */
		assert(0);
		return;
	}

	for( const ShaderCacheEntry& shader : file->shaderCache )
	{
		char *psShaderName		=		 ((char*)file->pDiskImage + shader.nameOffset);
		int  *piShaderPokePtr	= (int *)((char*)file->pDiskImage + shader.pokeOffset);

		shader_t *sh = R_FindShader(psShaderName, lightmapsNone, stylesDefault, qtrue);
		if ( sh->defaultShader )
			*piShaderPokePtr = 0;
		else
			*piShaderPokePtr = sh->index;
	}
}
