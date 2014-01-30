#pragma once

// Number of TriTreeNodes available
#define POOL_SIZE				(50000)

#define TEXTURE_ALPHA_TL		0x000000ff
#define TEXTURE_ALPHA_TR		0x0000ff00
#define TEXTURE_ALPHA_BL		0x00ff0000
#define TEXTURE_ALPHA_BR		0x000000ff

#define INDEX_TL				0
#define INDEX_TR				1
#define INDEX_BL				2
#define INDEX_BR				3

#define VARIANCE_MIN			0.0f
#define VARIANCE_MAX			2000.0f
#define SPLIT_VARIANCE_SIZE		20
#define SPLIT_VARIANCE_STEP		(VARIANCE_MAX / SPLIT_VARIANCE_SIZE)

class CTerVert
{
public:
	vec3_t		coords;					// real world coords of terxel
	vec3_t		normal;					// required to calculate lighting and used in physics
	color4ub_t	tint;					// tint at this terxel
	float		tex[2];					// texture coordinates at this terxel
	int			height;					// Copy of heightmap data
	int			tessIndex;				// Index of the vert in the tess array
	int			tessRegistration;		// ...... for the tess with this registration

	CTerVert( void ) { memset(this, 0, sizeof(*this)); }
	~CTerVert( void ) { }
};

class CTRHeightDetails
{
private:
	qhandle_t	mShader;
public:
	CTRHeightDetails( void ) { }
	~CTRHeightDetails( void ) { }

	const qhandle_t GetShader( void ) const { return(mShader); }
	void SetShader(const qhandle_t shader) { mShader = shader; }
};

//
// Information of each patch (tessellated area) of a CTRLandScape
//
class CTRPatch
{
private:
	class CCMLandScape	*owner;
	class CTRLandScape	*localowner;

	CCMPatch			*common;
	vec3_t				mCenter;									// Real world center of the patch
//	vec3_t				mNormal[2];
//	float				mDistance[2];

	CTerVert			*mRenderMap;								// Modulation value and texture coords per vertex
	shader_t			*mTLShader;		 							// Dynamically created blended shader for the top left triangle
	shader_t			*mBRShader;		 							// Dynamically created blended shader for the bottom right triangle

	bool				misVisible;									// Is this patch visible in the current frame?

public:
	CTRPatch(void) { }
	~CTRPatch(void) { }

	// Accessors
	const vec3_t &GetWorld(void) const { return(common->GetWorld()); }
	const vec3_t &GetMins(void) const { return(common->GetMins()); }
	const vec3_t &GetMaxs(void) const { return(common->GetMaxs()); }
	const vec3pair_t &GetBounds(void) const { return(common->GetBounds()); }
	shader_t			*GetTLShader(void) { return mTLShader; }
	shader_t			*GetBRShader(void) { return mBRShader; }

	void SetCommon(CCMPatch *in) { common = in; }
	const CCMPatch *GetCommon(void) const { return(common); }
	bool isVisible(void) { return(misVisible); }
	void SetTLShader(qhandle_t in) { mTLShader = R_GetShaderByHandle(in); }
	void SetBRShader(qhandle_t in) { mBRShader = R_GetShaderByHandle(in); }
	void SetOwner(CCMLandScape *in) { owner = in; }
	void SetLocalOwner(CTRLandScape *in) { localowner = in; }
	void Clear(void) { memset(this, 0, sizeof(*this));  }
	void SetCenter(void) { VectorAverage(common->GetMins(), common->GetMaxs(), mCenter); }
	void CalcNormal(void);

	// Prototypes
	void SetVisibility(bool visCheck);
	void RenderCorner(ivec5_t corner);
	void Render(int Part);
	void RecurseRender(int depth, ivec5_t left, ivec5_t right, ivec5_t apex);
	void SetRenderMap(const int x, const int y);
	int RenderWaterVert(int x, int y);
	void RenderWater(void);
	const bool HasWater(void) const;
};


#define	PI_TOP		1
#define PI_BOTTOM	2
#define PI_BOTH		3

typedef struct SPatchInfo
{
	CTRPatch	*mPatch;
	shader_t	*mShader;
	int			mPart;
} TPatchInfo;

//
// The master class used to define an area of terrain
//

class CTRLandScape
{
private:
	const CCMLandScape	*common;
	CTRPatch			*mTRPatches;							// Local patch info
	TPatchInfo			*mSortedPatches;

	int					mPatchMinx, mPatchMaxx;
	int					mPatchMiny, mPatchMaxy;
	int					mMaxNode;								// terxels * terxels = exit condition for splitting
	int					mSortedCount;

	float				mPatchSize;

	shader_t			*mShader;  								// shader the terrain got its contents from

	CTerVert			*mRenderMap;							// modulation value and texture coords per vertex
	float				mTextureScale;							// Scale of texture mapped to terrain

	float				mScalarSize;

	shader_t			*mWaterShader;							// Water shader
	qhandle_t			mFlatShader;							// Flat ground shader

	CTRHeightDetails	mHeightDetails[HEIGHT_RESOLUTION];		// Array of info specific to height
#if	_DEBUG
	int					mCycleCount;
#endif
public:
	CTRLandScape(const char *configstring);
	~CTRLandScape(void);

	// Accessors
	const int GetBlockWidth(void) const { return(common->GetBlockWidth()); }
	const int GetBlockHeight(void) const { return(common->GetBlockHeight()); }
	const vec3_t &GetMins(void) const { return(common->GetMins()); }
	const vec3_t &GetMaxs(void) const { return(common->GetMaxs()); }
	const vec3_t &GetTerxelSize(void) const { return(common->GetTerxelSize()); }
	const vec3_t &GetPatchSize(void) const { return(common->GetPatchSize()); }
	const int GetWidth(void) const { return(common->GetWidth()); }
	const int GetHeight(void) const { return(common->GetHeight()); }
	const int GetRealWidth(void) const { return(common->GetRealWidth()); }
	const int GetRealHeight(void) const { return(common->GetRealHeight()); }

	void SetCommon(const CCMLandScape *landscape) { common = landscape; }
	const CCMLandScape *GetCommon( void ) const { return(common); }
	const thandle_t GetCommonId( void ) const { return(common->GetTerrainId()); }
	shader_t	*GetShader(void) const { return(mShader); }
	CTerVert *GetRenderMap(const int x, const int y) const { return(mRenderMap + x + (y * common->GetRealWidth())); }
	CTRPatch *GetPatch(const int x, const int y) const { return(mTRPatches + (common->GetBlockWidth() * y) + x); }
	const CTRHeightDetails *GetHeightDetail(int height) const { return(mHeightDetails + height); }
	const float GetScalarSize(void) const { return(mScalarSize); }
	const int GetMaxNode(void) const { return(mMaxNode); }

	// Prototypes
	void CalculateRegion(void);
	void Reset(bool visCheck = true);
	void Render(void);
	void CalculateRealCoords(void);
	void CalculateNormals(void);
	void CalculateTextureCoords(void);
	void CalculateLighting(void);
	void CalculateShaders(void);
	qhandle_t GetBlendedShader(qhandle_t a, qhandle_t b, qhandle_t c, bool surfaceSprites);
	void LoadTerrainDef(const char *td);
	void CopyHeightMap(void);
	void SetShaders(const int height, const qhandle_t shader);
};

void R_CalcTerrainVisBounds(CTRLandScape *landscape);
void R_AddTerrainSurfaces(void);
