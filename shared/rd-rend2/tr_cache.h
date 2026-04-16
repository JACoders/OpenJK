#pragma once

#include <qcommon/q_shared.h>
#include <vector>

/*
 * This stores the loaded file information that we need on retrieval
 */
struct Asset
{
	qhandle_t		handle;
	char			path[MAX_QPATH];
};

/* and shaderCache_t is needed for the model cache manager */
struct ShaderCacheEntry
{
	ShaderCacheEntry( int nameOffset, int pokeOffset )
		: nameOffset(nameOffset)
		, pokeOffset(pokeOffset)
	{
	}

	int nameOffset;
	int pokeOffset;
};
using ShaderCache = std::vector<ShaderCacheEntry>;

/*
 * The actual data stored in the cache
 */
struct CachedFile
{
	void			*pDiskImage;			// pointer to data loaded from disk
	int				iLevelLastUsedOn;		// level we last used this on
	int				iPAKChecksum;			// -1 = not from PAK
	int				iAllocSize;				//

	ShaderCache		shaderCache;

	char			path[MAX_QPATH];

	CachedFile();
};

class CModelCacheManager
{
public:
	using AssetCache = std::vector<Asset>;
	using FileCache = std::vector<CachedFile>;

public:
	/*
	 * Return -1 if asset not currently loaded, return positive qhandle_t if found
	 */
	qhandle_t	GetModelHandle( const char *fileName );

	/*
	 * We have a loaded model, let's insert it into the list of loaded models
	 */
	void		InsertModelHandle( const char *fileName, qhandle_t handle );

	qboolean	LevelLoadEnd( qboolean deleteUnusedByLevel );
	void		StoreShaderRequest( const char *psModelFileName, const char *psShaderName, int *piShaderIndexPoke );
	void		AllocateShaders( const char *psFileName );


	/*
	 * Load the file and chuck the contents into ppFileBuffer, OR
	 * if we're cached already, chuck cached contents into ppFileBuffer
	 * and set *pbAlreadyCached to qtrue (otherwise, *pbAlreadyCached = false)
	 */
	qboolean	LoadFile( const char *pFileName, void **ppFileBuffer, qboolean *pbAlreadyCached );

	/*
	 * Allocate appropriate memory for stuff dealing with cached images
	 * FIXME: only applies to models?
	*/
	void		*Allocate( int iSize, void *pvDiskBuffer, const char *psModelFileName, qboolean *bAlreadyFound, memtag_t eTag );
	void		DeleteAll( void );
	void		DumpNonPure();

private:
	AssetCache::iterator FindAsset( const char *name );
	FileCache::iterator	FindFile( const char *name );

	AssetCache assets;
	FileCache files;
};

qboolean C_Models_LevelLoadEnd( qboolean deleteUnusedByLevel );
qboolean C_Images_LevelLoadEnd();

extern CModelCacheManager *CModelCache;
