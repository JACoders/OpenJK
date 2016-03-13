#pragma once

#include <qcommon/q_shared.h>
#include <vector>

/*
 * Cache information specific to each file (FIXME: do we need?)
 */
enum CacheType
{
	CACHE_NONE,
	CACHE_IMAGE,
	CACHE_MODEL
};

/*
 * This stores the loaded file information that we need on retrieval
 */
struct Asset
{
	qhandle_t		handle;
	char			path[MAX_QPATH];
};

/* and shaderCache_t is needed for the model cache manager */
typedef std::pair<int,int> shaderCacheEntry_t;
typedef std::vector<shaderCacheEntry_t> shaderCache_t;

/*
 * The actual data stored in the cache
 */
struct CachedFile
{
	void			*pDiskImage;			// pointer to data loaded from disk
	int				iLevelLastUsedOn;		// level we last used this on
	int				iPAKChecksum;			// -1 = not from PAK
	int				iAllocSize;				//

	shaderCache_t	shaderCache;

	char			path[MAX_QPATH];

	CachedFile();
};

/* The actual manager itself, which is used in the model and image loading routines. */
class CCacheManager
{
public:
	typedef std::vector<CachedFile> FileCache;

public:
	virtual ~CCacheManager() {}

	/*
	 * Load the file and chuck the contents into ppFileBuffer, OR
	 * if we're cached already, chuck cached contents into ppFileBuffer
	 * and set *pbAlreadyCached to qtrue (otherwise, *pbAlreadyCached = false)
	 */
	qboolean			LoadFile( const char *pFileName, void **ppFileBuffer, qboolean *pbAlreadyCached );

	/*
	 * Allocate appropriate memory for stuff dealing with cached images
	 * FIXME: only applies to models?
	*/
	void				*Allocate( int iSize, void *pvDiskBuffer, const char *psModelFileName, qboolean *bAlreadyFound, memtag_t eTag );
	void				DeleteAll( void );
	void				DumpNonPure( void );

	virtual qboolean	LevelLoadEnd( qboolean deleteUnusedByLevel ) = 0;

protected:
	virtual void		DeleteRemaining() {}

	Asset *				AddAsset( const Asset& asset );
	CachedFile *		AddFile( const CachedFile& file );

	FileCache::iterator	FindFile( const char *name );

protected:
	FileCache files;
};

class CImageCacheManager : public CCacheManager
{
public:
	virtual qboolean	LevelLoadEnd( qboolean deleteUnusedByLevel );

	void				DeleteLightMaps( void );
};

class CModelCacheManager : public CCacheManager
{
public:
	typedef std::vector<Asset> AssetCache;

public:
	/*
	 * Return -1 if asset not currently loaded, return positive qhandle_t if found
	 */
	qhandle_t			GetModelHandle( const char *fileName );

	/*
	 * We have a loaded model/image, let's insert it into the list of loaded models
	 */
	void				InsertModelHandle( const char *fileName, qhandle_t handle );

	virtual qboolean	LevelLoadEnd( qboolean deleteUnusedByLevel );
	void				StoreShaderRequest( const char *psModelFileName, const char *psShaderName, int *piShaderIndexPoke );
	void				AllocateShaders( const char *psFileName );

protected:
	virtual void		DeleteRemaining();

private:
	AssetCache::iterator FindAsset( const char *name );
	AssetCache assets;
};

qboolean C_Models_LevelLoadEnd( qboolean deleteUnusedByLevel );
qboolean C_Images_LevelLoadEnd();

extern CImageCacheManager *CImgCache;
extern CModelCacheManager *CModelCache;
