#if !defined(CM_LANDSCAPE_H_INC)
#define CM_LANDSCAPE_H_INC

#pragma warning (push, 3)	//go back down to 3 for the stl include
#include <list>
#pragma warning (pop)

using namespace std;

// These are the root classes using data shared in both the server and the renderer.
// This common data is also available to physics

#define HEIGHT_RESOLUTION		256

// Trying to make a guess at the optimal step through the patches
// This is the average of 1 side and the diagonal presuming a square patch
#define TERRAIN_STEP_MAGIC		(1.0f / 1.2071f)

#define MIN_TERXELS				2
#define MAX_TERXELS				8
// Defined as 1 << (sqrt(MAX_TERXELS) + 1)
#define MAX_VARIANCE_SIZE		16

// Maximum number of instances to pick from an instance file
#define MAX_INSTANCE_TYPES		16

// Types of areas

typedef enum
{
	AT_NONE,
	AT_FLAT,
	AT_BSP,
	AT_NPC,
	AT_GROUP,
	AT_RIVER,
	AT_OBJECTIVE,
	AT_PLAYER,

} areaType_t;

class CArea
{
private:
	vec3_t		mPosition;
	float		mRadius;
	float		mAngle;
	float		mAngleDiff;
	int			mType;
	int			mVillageID;
public:
	CArea(void) {}
	~CArea(void) {}

	void Init(vec3_t pos, float radius, float angle = 0.0f, int type = AT_NONE, float angleDiff = 0.0f, int villageID = 0) 
	{ 
		VectorCopy(pos, mPosition); 
		mRadius = radius; 
		mAngle = angle; 
		mAngleDiff = angleDiff; 
		mType = type; 
		mVillageID = villageID;
	}
	float GetRadius(void) const { return(mRadius); }
	float GetAngle(void) const { return(mAngle); }
	float GetAngleDiff(void) const { return(mAngleDiff); }
	vec3_t &GetPosition(void) { return(mPosition); }
	int GetType(void) const { return(mType); }
	int GetVillageID(void) const { return(mVillageID); }
};

typedef list<CArea*>					areaList_t;
typedef list<CArea*>::iterator			areaIter_t;

class CCMHeightDetails
{
private:
	int						mContents;
	int						mSurfaceFlags;
public:
	CCMHeightDetails(void) {}
	~CCMHeightDetails(void) {}

	// Accessors
	const int GetSurfaceFlags(void) const { return(mSurfaceFlags); }
	const int GetContents(void) const { return(mContents); }
	void SetFlags(const int con, const int sf) { mContents = con; mSurfaceFlags = sf; }
};

class CCMPatch
{
protected:
	class CCMLandScape		*owner;										// Owning landscape
	int						mHx, mHy;									// Terxel coords of patch
	byte					*mHeightMap;				 				// Pointer to height map to use
	byte					mCornerHeights[4]; 							// Heights at the corners of the patch
	vec3_t					mWorldCoords;				 				// World coordinate offset of this patch.
	vec3pair_t 				mBounds;									// mins and maxs of the patch for culling
	int						mNumBrushes;								// number of brushes to collide with in the patch
	struct cbrush_s			*mPatchBrushData;							// List of brushes that make up the patch
	int						mSurfaceFlags;								// surfaceflag of the heightshader
	int						mContentFlags;								// contents of the heightshader
public:
	// Constructors
	CCMPatch(void) {}
	~CCMPatch(void);

	// Accessors
	const vec3_t &GetWorld(void) const { return(mWorldCoords); }
	const vec3_t &GetMins(void) const { return(mBounds[0]); }
	const vec3_t &GetMaxs(void) const { return(mBounds[1]); }
	const vec3pair_t &GetBounds(void) const { return(mBounds); }
	const int GetHeightMapX(void) const { return(mHx); }
	const int GetHeightMapY(void) const { return(mHy); }
	const int GetHeight(int corner) const { return(mCornerHeights[corner]); }
	const int GetNumBrushes(void) const { return(mNumBrushes); }
	struct cbrush_s *GetCollisionData(void) const { return(mPatchBrushData); }

	void SetSurfaceFlags(const int in) { mSurfaceFlags = in; }
	const int GetSurfaceFlags(void) const { return(mSurfaceFlags); }
	void SetContents(const int in) { mContentFlags = in; }
	const int GetContents(void) const { return(mContentFlags); }

	// Prototypes
	void Init(CCMLandScape *ls, int heightX, int heightY, vec3_t world, byte *hMap, byte *patchBrushData);
	void InitPlane(struct cbrushside_s *side, cplane_t *plane, vec3_t p0, vec3_t p1, vec3_t p2);
	void CreatePatchPlaneData(void);
	
	void*	GetAdjacentBrushX ( int x, int y );
	void*	GetAdjacentBrushY ( int x, int y );
};

class CRandomTerrain;

class CCMLandScape
{
private:
	int						mRefCount;									// Number of times this class is referenced
	thandle_t				mTerrainHandle;
	byte					*mHeightMap;								// Pointer to byte array of height samples
	byte					*mFlattenMap;								// Pointer to byte array of flatten samples
	int						mWidth, mHeight;							// Width and height of heightMap excluding the 1 pixel edge
	int						mTerxels;									// Number of terxels per patch side
	vec3_t					mTerxelSize;								// Vector to scale heightMap samples to real world coords
	vec3pair_t				mBounds;									// Real world bounds of terrain brush
	vec3_t					mSize;										// Size of terrain brush in real world coords excluding 1 patch edge
	vec3_t					mPatchSize;									// Size of each patch in the x and y directions only
	float					mPatchScalarSize;							// Horizontal size of the patch
	int						mBlockWidth, mBlockHeight;					// Width and height of heightfield on blocks
	CCMPatch 				*mPatches;
	byte					*mPatchBrushData;							// Base memory from which the patch brush data is taken
	bool					mHasPhysics;								// Set to true unless disabled
	CRandomTerrain			*mRandomTerrain;
							
	int						mBaseWaterHeight;							// Base water height in terxels
	float					mWaterHeight;								// Real world height of the water
	int						mWaterContents;								// Contents of the water shader
	int						mWaterSurfaceFlags;							// Surface flags of the water shader

	unsigned long			holdrand;
	
	list<CArea *>			mAreas;										// List of flattened areas on this landscape
	list<CArea *>::iterator	mAreasIt;

	CCMHeightDetails		mHeightDetails[HEIGHT_RESOLUTION];			// Surfaceflags per height
	vec3_t					*mCoords;									// Temp storage for real world coords

public:
	CCMLandScape(const char *configstring, bool server);
	~CCMLandScape(void);

	CCMPatch *GetPatch(int x, int y);

	// Prototypes
	void PatchCollide(struct traceWork_s *tw, trace_t &trace, const vec3_t start, const vec3_t end, int checkcount);
	void TerrainPatchIterate(void (*IterateFunc)( CCMPatch *, void * ), void *userdata) const;
	float GetWorldHeight(vec3_t origin, const vec3pair_t bounds, bool aboveGround) const;
	float WaterCollide(const vec3_t begin, const vec3_t end, float fraction) const;
	void UpdatePatches(void);
	void GetTerxelLocalCoords ( int x, int y, vec3_t coords[8] );
	void LoadTerrainDef(const char *td);
	void SetShaders(int height, class CCMShader *shader);
	void FlattenArea(CArea *area, int height, bool save, bool forceHeight, bool smooth);
	void CarveLine ( vec3_t start, vec3_t end, int depth, int width );
	void CarveBezierCurve ( int numCtlPoints, vec3_t* ctlPoints, int steps, int depth, int size );
	void SaveArea(CArea *area);
	float FractionBelowLevel(CArea *area, int height);
	bool AreaCollision(CArea *area, int *areaTypes, int areaTypeCount);
	CArea *GetFirstArea(void);
	CArea *GetFirstObjectiveArea(void);
	CArea *GetPlayerArea(void);
	CArea *GetNextArea(void);
	CArea *GetNextObjectiveArea(void);

	// Accessors
	const int GetRefCount(void) const { return(mRefCount); }
	void IncreaseRefCount(void) { mRefCount++; }
	void DecreaseRefCount(void) { mRefCount--; }
	const vec3pair_t &GetBounds(void) const { return(mBounds); }
	const vec3_t &GetMins(void) const { return(mBounds[0]); }
	const vec3_t &GetMaxs(void) const { return(mBounds[1]); }
	const vec3_t &GetSize(void) const { return(mSize); }
	const vec3_t &GetTerxelSize(void) const { return(mTerxelSize); }
	const vec3_t &GetPatchSize(void) const { return(mPatchSize); }
	const float GetPatchWidth(void) const { return(mPatchSize[0]); }
	const float GetPatchHeight(void) const { return(mPatchSize[1]); }
	const float GetPatchScalarSize(void) const { return(mPatchScalarSize); }
	const int GetTerxels(void) const { return(mTerxels); }
	const int GetRealWidth(void) const { return(mWidth + 1); }
	const int GetRealHeight(void) const { return(mHeight + 1); }
	const int GetRealArea(void) const { return((mWidth + 1) * (mHeight + 1)); }
	const int GetWidth(void) const { return(mWidth); }
	const int GetHeight(void) const { return(mHeight); }
	const int GetArea(void) const { return(mWidth * mHeight); }
	const int GetBlockWidth(void) const { return(mBlockWidth); }
	const int GetBlockHeight(void) const { return(mBlockHeight); }
	const int GetBlockCount(void) const { return(mBlockWidth * mBlockHeight); }
	byte *GetHeightMap(void) const { return(mHeightMap); }
	byte *GetFlattenMap(void) const { return(mFlattenMap); }
	const thandle_t GetTerrainId(void) const { return(mTerrainHandle); }
	void SetTerrainId(const thandle_t terrainId) { mTerrainHandle = terrainId; }
	const float CalcWorldHeight(int height) const { return((height * mTerxelSize[2]) + mBounds[0][2]); }
	const bool GetHasPhysics(void) const { return(mHasPhysics); }
	const bool GetIsRandom(void) const { return(mRandomTerrain != 0); }
	const int GetSurfaceFlags(int height) const { return(mHeightDetails[height].GetSurfaceFlags()); }
	const int GetContentFlags(int height) const { return(mHeightDetails[height].GetContents()); }
	void CalcRealCoords(void);
	vec3_t *GetCoords(void) const { return(mCoords); }

	int GetBaseWaterHeight(void) const { return(mBaseWaterHeight); }
	void SetRealWaterHeight(int height) { mWaterHeight = height * mTerxelSize[2]; }
	float GetWaterHeight(void) const { return(mWaterHeight); }
	int GetWaterContents(void) const { return(mWaterContents); }
	int GetWaterSurfaceFlags(void) const { return(mWaterSurfaceFlags); }

	CRandomTerrain			*GetRandomTerrain(void) { return mRandomTerrain; }

	void			rand_seed(int seed);
	unsigned long	get_rand_seed(void) { return holdrand; }

	float			flrand(float min, float max);
	int				irand(int min, int max);
};

void CM_TerrainPatchIterate(const class CCMLandScape *ls, void (*IterateFunc)( CCMPatch *, void * ), void *userdata);
class CCMLandScape *CM_InitTerrain(const char *configstring, thandle_t terrainId, bool server);
float CM_GetWorldHeight(const CCMLandScape *landscape, vec3_t origin, const vec3pair_t bounds, bool aboveGround);
void CM_FlattenArea(CCMLandScape *landscape, CArea *area, int height, bool save, bool forceHeight, bool smooth);
void CM_CarveBezierCurve (CCMLandScape *landscape, int numCtls, vec3_t* ctls, int steps, int depth, int size );
void CM_SaveArea(CCMLandScape *landscape, CArea *area);
float CM_FractionBelowLevel(CCMLandScape *landscape, CArea *area, int height);
bool CM_AreaCollision(class CCMLandScape *landscape, class CArea *area, int *areaTypes, int areaTypeCount);
CArea *CM_GetFirstArea(CCMLandScape *landscape);
CArea *CM_GetFirstObjectiveArea(CCMLandScape *landscape);
CArea *CM_GetPlayerArea(class CCMLandScape *common);
CArea *CM_GetNextArea(CCMLandScape *landscape);
CArea *CM_GetNextObjectiveArea(CCMLandScape *landscape);

typedef void ( *cm_iterateFunc )( byte *, float, int * );

void CM_CircularIterate(byte *data, int width, int height, int xo, int yo, int insideRadius, int outsideRadius, int *user, cm_iterateFunc callback);

CRandomTerrain *CreateRandomTerrain(const char *config, CCMLandScape *landscape, byte *heightmap, int width, int height);

void SV_LoadMissionDef(const char *configstring, class CCMLandScape *landscape);
void CL_CreateRandomTerrain(const char *config, class CCMLandScape *landscape, byte *image, int width, int height);
void CL_LoadInstanceDef(const char *configstring, class CCMLandScape *landscape);
void CL_LoadMissionDef(const char *configstring, class CCMLandScape *landscape);

extern cvar_t	*com_terrainPhysics;

#endif

// end
