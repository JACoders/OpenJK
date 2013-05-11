#pragma once

#define MAX_RANDOM_MODELS		8

class CRandomModel
{
private:
	char		mModelName[MAX_QPATH];
	float		mFrequency;
	float		mMinScale;
	float		mMaxScale;
public:
	CRandomModel(void) { }
	~CRandomModel(void) { }

	// Accessors 
	const bool GetModel( void ) const { return(!!strlen(mModelName)); }
	const char *GetModelName( void ) const { return(mModelName); }
	void SetModel(const char *name) { Com_sprintf(mModelName, MAX_QPATH, "%s.md3", name); }
	const float GetFrequency(void) const { return(mFrequency); }
	void SetFrequency(const float freq) { mFrequency = freq; }
	const float GetMinScale(void) const { return(mMinScale); }
	void SetMinScale(const float minscale) { mMinScale = minscale; }
	const float GetMaxScale(void) const { return(mMaxScale); }
	void SetMaxScale(const float maxscale) { mMaxScale = maxscale; }
};

class CCGHeightDetails
{
private:
	int				mNumModels;
	int				mTotalFrequency;
	CRandomModel	mModels[MAX_RANDOM_MODELS];
public:
	// Constructors
	CCGHeightDetails( void ) { memset(this, 0, sizeof(*this)); }
	~CCGHeightDetails( void ) { }

	// Accessors
	const int GetNumModels(void) const { return(mNumModels); }
	const int GetAverageFrequency(void) const { return(mTotalFrequency / mNumModels); }

	// Prototypes
	void AddModel(const CRandomModel *hd);
	CRandomModel *GetRandomModel(CCMLandScape *land);
};

class CCGPatch
{
private:
	class CCMLandScape	*owner;
	class CCGLandScape	*localowner;
	CCMPatch			*common;
public:
};

class CRMLandScape
{
private:
	CCMLandScape			*common;

	byte					*mDensityMap;							// Data image of model densities
	int						mModelCount;							// Count of spawned client models
	CCGHeightDetails		mHeightDetails[HEIGHT_RESOLUTION];		// Array of info specific to height

public:
	CRMLandScape(void);
	~CRMLandScape(void);

	// Accessors
	void SetCommon(CCMLandScape *landscape) { common = landscape; }
	const CCMLandScape *GetCommon( void ) const { return(common); }
	const thandle_t GetCommonId( void ) const { return(common->GetTerrainId()); }
	const int GetTerxels(void) const { return(common->GetTerxels()); }
	const int GetRealWidth(void) const { return(common->GetRealWidth()); }
	const float GetPatchScalarSize(void) const { return(common->GetPatchScalarSize()); }
	const CCGHeightDetails *GetHeightDetail(int height) const { return(mHeightDetails + height); }
	void ClearModelCount(void) { mModelCount = 0; }
	const int GetModelCount(void) const { return(mModelCount); }

	// Prototypes
	void SetShaders(const int height, const qhandle_t shader);
	void AddModel(const int height, int maxheight, const CRandomModel *hd);
	void LoadMiscentDef(const char *td);
	void LoadDensityMap(const char *td);
	void SpawnPatchModels(CCMPatch *patch);
	void Sprinkle(CCMPatch *patch, CCGHeightDetails *hd, int level);
	void CreateRandomDensityMap(byte *imageData, int width, int height, int seed);
};

void RM_CreateRandomModels(int terrainId, const char *terrainInfo);
void RM_InitTerrain(void);
void RM_ShutdownTerrain(void);
