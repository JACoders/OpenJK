#include "client/client.h"	//FIXME!! EVIL - just include the definitions needed
#include "tr_local.h"
#include "qcommon/matcomp.h"
#include "qcommon/qcommon.h"
#include "ghoul2/G2.h"
#include "ghoul2/g2_local.h"
#ifdef _G2_GORE
#include "G2_gore_r2.h"
#endif

#ifdef _MSC_VER
#pragma warning (disable: 4512)	//default assignment operator could not be gened
#endif
#include "qcommon/disablewarnings.h"
#include "tr_cache.h"

#define	LL(x) x=LittleLong(x)

#ifdef G2_PERFORMANCE_ANALYSIS
#include "qcommon/timing.h"

timing_c G2PerformanceTimer_RenderSurfaces;
timing_c G2PerformanceTimer_R_AddGHOULSurfaces;
timing_c G2PerformanceTimer_G2_TransformGhoulBones;
timing_c G2PerformanceTimer_G2_ProcessGeneratedSurfaceBolts;
timing_c G2PerformanceTimer_ProcessModelBoltSurfaces;
timing_c G2PerformanceTimer_G2_ConstructGhoulSkeleton;
timing_c G2PerformanceTimer_RB_SurfaceGhoul;
timing_c G2PerformanceTimer_G2_SetupModelPointers;
timing_c G2PerformanceTimer_PreciseFrame;

int G2PerformanceCounter_G2_TransformGhoulBones = 0;

int G2Time_RenderSurfaces = 0;
int G2Time_R_AddGHOULSurfaces = 0;
int G2Time_G2_TransformGhoulBones = 0;
int G2Time_G2_ProcessGeneratedSurfaceBolts = 0;
int G2Time_ProcessModelBoltSurfaces = 0;
int G2Time_G2_ConstructGhoulSkeleton = 0;
int G2Time_RB_SurfaceGhoul = 0;
int G2Time_G2_SetupModelPointers = 0;
int G2Time_PreciseFrame = 0;

void G2Time_ResetTimers(void)
{
	G2Time_RenderSurfaces = 0;
	G2Time_R_AddGHOULSurfaces = 0;
	G2Time_G2_TransformGhoulBones = 0;
	G2Time_G2_ProcessGeneratedSurfaceBolts = 0;
	G2Time_ProcessModelBoltSurfaces = 0;
	G2Time_G2_ConstructGhoulSkeleton = 0;
	G2Time_RB_SurfaceGhoul = 0;
	G2Time_G2_SetupModelPointers = 0;
	G2Time_PreciseFrame = 0;
	G2PerformanceCounter_G2_TransformGhoulBones = 0;
}

void G2Time_ReportTimers(void)
{
	Com_Printf("\n---------------------------------\nRenderSurfaces: %i\nR_AddGhoulSurfaces: %i\nG2_TransformGhoulBones: %i\nG2_ProcessGeneratedSurfaceBolts: %i\nProcessModelBoltSurfaces: %i\nG2_ConstructGhoulSkeleton: %i\nRB_SurfaceGhoul: %i\nG2_SetupModelPointers: %i\n\nPrecise frame time: %i\nTransformGhoulBones calls: %i\n---------------------------------\n\n",
		G2Time_RenderSurfaces,
		G2Time_R_AddGHOULSurfaces,
		G2Time_G2_TransformGhoulBones,
		G2Time_G2_ProcessGeneratedSurfaceBolts,
		G2Time_ProcessModelBoltSurfaces,
		G2Time_G2_ConstructGhoulSkeleton,
		G2Time_RB_SurfaceGhoul,
		G2Time_G2_SetupModelPointers,
		G2Time_PreciseFrame,
		G2PerformanceCounter_G2_TransformGhoulBones
	);
}
#endif

//rww - RAGDOLL_BEGIN
#ifdef __linux__
#include <math.h>
#else
#include <float.h>
#endif

//rww - RAGDOLL_END

static const int MAX_RENDERABLE_SURFACES = 2048;
static CRenderableSurface renderSurfHeap[MAX_RENDERABLE_SURFACES];
static int currentRenderSurfIndex = 0;

static CRenderableSurface *AllocGhoul2RenderableSurface()
{
	if ( currentRenderSurfIndex >= MAX_RENDERABLE_SURFACES - 1)
	{
		ResetGhoul2RenderableSurfaceHeap();
		ri.Printf( PRINT_DEVELOPER, "AllocRenderableSurface: Reached maximum number of Ghoul2 renderable surfaces (%d)\n", MAX_RENDERABLE_SURFACES );
	}

	CRenderableSurface *rs = &renderSurfHeap[currentRenderSurfIndex++];

	rs->Init();

	return rs;
}

void ResetGhoul2RenderableSurfaceHeap()
{
	currentRenderSurfIndex = 0;
}

bool HackadelicOnClient = false; // means this is a render traversal

qboolean G2_SetupModelPointers(CGhoul2Info *ghlInfo);
qboolean G2_SetupModelPointers(CGhoul2Info_v &ghoul2);

extern cvar_t	*r_Ghoul2AnimSmooth;
extern cvar_t	*r_Ghoul2UnSqashAfterSmooth;

static const mdxaBone_t identityMatrix =
{
	{
		{ 0.0f, -1.0f, 0.0f, 0.0f },
		{ 1.0f, 0.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 1.0f, 0.0f }
	}
};

// I hate doing this, but this is the simplest way to get this into the routines it needs to be
mdxaBone_t		worldMatrix;
mdxaBone_t		worldMatrixInv;
#ifdef _G2_GORE
qhandle_t		goreShader=-1;
#endif

// nasty little matrix multiply going on here..
void Mat3x4_Multiply(mdxaBone_t *out, const mdxaBone_t *in2, const mdxaBone_t *in)
{
	assert(out != nullptr);
	assert(in2 != nullptr);
	assert(in != nullptr);

	// Let's say we are doing R = N * M
	const float n00 = in2->matrix[0][0];
	const float n01 = in2->matrix[0][1];
	const float n02 = in2->matrix[0][2];
	const float n03 = in2->matrix[0][3];
	const float n10 = in2->matrix[1][0];
	const float n11 = in2->matrix[1][1];
	const float n12 = in2->matrix[1][2];
	const float n13 = in2->matrix[1][3];
	const float n20 = in2->matrix[2][0];
	const float n21 = in2->matrix[2][1];
	const float n22 = in2->matrix[2][2];
	const float n23 = in2->matrix[2][3];

	const float m00 = in->matrix[0][0];
	const float m01 = in->matrix[0][1];
	const float m02 = in->matrix[0][2];
	const float m03 = in->matrix[0][3];
	const float m10 = in->matrix[1][0];
	const float m11 = in->matrix[1][1];
	const float m12 = in->matrix[1][2];
	const float m13 = in->matrix[1][3];
	const float m20 = in->matrix[2][0];
	const float m21 = in->matrix[2][1];
	const float m22 = in->matrix[2][2];
	const float m23 = in->matrix[2][3];

	// first row of out
	out->matrix[0][0] = (n00 * m00) + (n01 * m10) + (n02 * m20);
	out->matrix[0][1] = (n00 * m01) + (n01 * m11) + (n02 * m21);
	out->matrix[0][2] = (n00 * m02) + (n01 * m12) + (n02 * m22);
	out->matrix[0][3] = (n00 * m03) + (n01 * m13) + (n02 * m23) + n03;

	// second row of outf out
	out->matrix[1][0] = (n10 * m00) + (n11 * m10) + (n12 * m20);
	out->matrix[1][1] = (n10 * m01) + (n11 * m11) + (n12 * m21);
	out->matrix[1][2] = (n10 * m02) + (n11 * m12) + (n12 * m22);
	out->matrix[1][3] = (n10 * m03) + (n11 * m13) + (n12 * m23) + n13;

	// third row of out  out
	out->matrix[2][0] = (n20 * m00) + (n21 * m10) + (n22 * m20);
	out->matrix[2][1] = (n20 * m01) + (n21 * m11) + (n22 * m21);
	out->matrix[2][2] = (n20 * m02) + (n21 * m12) + (n22 * m22);
	out->matrix[2][3] = (n20 * m03) + (n21 * m13) + (n22 * m23) + n23;
}

void Mat3x4_Scale( mdxaBone_t *result, const mdxaBone_t *lhs, const float scale )
{
	for ( int i = 0; i < 3; ++i )
	{
		for ( int j = 0; j < 3; ++j )
		{
			result->matrix[i][j] = lhs->matrix[i][j] * scale;
		}
	}
}

void Mat3x4_Lerp(
	mdxaBone_t *result,
	const mdxaBone_t *lhs,
	const mdxaBone_t *rhs,
	const float t )
{
	for ( int i = 0; i < 3; ++i )
	{
		for ( int j = 0; j < 4; ++j )
		{
			result->matrix[i][j] = lhs->matrix[i][j] * t + rhs->matrix[i][j] * (1.0f - t);
		}
	}
}

const mdxaBone_t operator +( const mdxaBone_t& lhs, const mdxaBone_t& rhs )
{
	mdxaBone_t result;
	for ( int i = 0; i < 3; ++i )
	{
		for ( int j = 0; j < 4; ++j )
		{
			result.matrix[i][j] = lhs.matrix[i][j] + rhs.matrix[i][j];
		}
	}
	return result;
}

const mdxaBone_t operator -( const mdxaBone_t& lhs, const mdxaBone_t& rhs )
{
	mdxaBone_t result;
	for ( int i = 0; i < 3; ++i )
	{
		for ( int j = 0; j < 4; ++j )
		{
			result.matrix[i][j] = lhs.matrix[i][j] - rhs.matrix[i][j];
		}
	}
	return result;
}

const mdxaBone_t operator *( const mdxaBone_t& lhs, const mdxaBone_t& rhs )
{
	mdxaBone_t result;
	Mat3x4_Multiply(&result, &lhs, &rhs);
	return result;
}

const mdxaBone_t operator *( const mdxaBone_t& lhs, const float scale )
{
	mdxaBone_t result;
	for ( int i = 0; i < 3; ++i )
	{
		for ( int j = 0; j < 4; ++j )
		{
			result.matrix[i][j] = lhs.matrix[i][j] * scale;
		}
	}
	return result;
}

const mdxaBone_t operator *( const float scale, const mdxaBone_t& rhs )
{
	return rhs * scale;
}


class CConstructBoneList
{
public:
	int				surfaceNum;
	int				*boneUsedList;
	surfaceInfo_v	&rootSList;
	model_t			*currentModel;
	boneInfo_v		&boneList;

	CConstructBoneList(
		int initsurfaceNum,
		int *initboneUsedList,
		surfaceInfo_v& initrootSList,
		model_t *initcurrentModel,
		boneInfo_v& initboneList)
		: surfaceNum(initsurfaceNum)
		, boneUsedList(initboneUsedList)
		, rootSList(initrootSList)
		, currentModel(initcurrentModel)
		, boneList(initboneList)
	{
	}
};

class CTransformBone
{
public:
	int touch; // for minimal recalculation
	int touchRender;
	mdxaBone_t boneMatrix; //final matrix
	int parent; // only set once

	CTransformBone()
		: touch(0)
		, touchRender(0)
	{
	}

};

struct SBoneCalc
{
	int newFrame;
	int	currentFrame;
	float backlerp;
	float blendFrame;
	int blendOldFrame;
	bool blendMode;
	float blendLerp;
};

class CBoneCache;
static void G2_TransformBone( int index, CBoneCache &CB );

class CBoneCache
{
	void EvalLow( int index )
	{
		assert(index >= 0 && index < (int)mBones.size());

		if ( mFinalBones[index].touch != mCurrentTouch )
		{
			// need to evaluate the bone
			assert((mFinalBones[index].parent >= 0
						&& mFinalBones[index].parent < (int)mFinalBones.size()) ||
					(index == 0 && mFinalBones[index].parent == -1));

			if ( mFinalBones[index].parent >= 0 )
			{
				EvalLow(mFinalBones[index].parent); // make sure parent is evaluated

				const SBoneCalc &par = mBones[mFinalBones[index].parent];
				SBoneCalc &bone = mBones[index];
				bone.newFrame = par.newFrame;
				bone.currentFrame = par.currentFrame;
				bone.backlerp = par.backlerp;
				bone.blendFrame = par.blendFrame;
				bone.blendOldFrame = par.blendOldFrame;
				bone.blendMode = par.blendMode;
				bone.blendLerp = par.blendLerp;
			}

			G2_TransformBone(index, *this);

			mFinalBones[index].touch = mCurrentTouch;
		}
	}

	void SmoothLow( int index )
	{
		if ( mSmoothBones[index].touch == mLastTouch )
		{
			const mdxaBone_t& newM = mFinalBones[index].boneMatrix;
			mdxaBone_t& oldM = mSmoothBones[index].boneMatrix;

			oldM = mSmoothFactor * (oldM - newM) + newM;
		}
		else
		{
			mSmoothBones[index].boneMatrix = mFinalBones[index].boneMatrix;
		}

		const mdxaSkelOffsets_t *offsets =
			(mdxaSkelOffsets_t *)((byte *)header + sizeof(mdxaHeader_t));
		const mdxaSkel_t *skel =
			(mdxaSkel_t *)((byte *)offsets + offsets->offsets[index]);

		mdxaBone_t tempMatrix =
			mSmoothBones[index].boneMatrix * skel->BasePoseMat;
		const float maxl = VectorLength(&skel->BasePoseMat.matrix[0][0]);
		VectorNormalize(&tempMatrix.matrix[0][0]);
		VectorNormalize(&tempMatrix.matrix[1][0]);
		VectorNormalize(&tempMatrix.matrix[2][0]);
		Mat3x4_Scale(&tempMatrix, &tempMatrix, maxl);

		mSmoothBones[index].boneMatrix = tempMatrix * skel->BasePoseMatInv;
		mSmoothBones[index].touch = mCurrentTouch;

#ifdef _DEBUG
		for ( int i = 0; i < 3; i++ )
		{
			for ( int j = 0; j < 4; j++ )
			{
				assert(!Q_isnan(mSmoothBones[index].boneMatrix.matrix[i][j]));
			}
		}
#endif// _DEBUG
	}

public:
	int frameSize;
	const mdxaHeader_t *header;
	const model_t *mod;

	// these are split for better cpu cache behavior
	std::vector<SBoneCalc> mBones;
	std::vector<CTransformBone> mFinalBones;
	std::vector<CTransformBone> mSmoothBones; // for render smoothing

	boneInfo_v *rootBoneList;
	mdxaBone_t rootMatrix;
	int incomingTime;

	int mCurrentTouch;
	int mCurrentTouchRender;
	int mLastTouch;
	int mLastLastTouch;

	// for render smoothing
	bool mSmoothingActive;
	bool mUnsquash;
	float mSmoothFactor;

	// GPU Data
	mat3x4_t boneMatrices[MAX_G2_BONES];
	int      uboOffset;
	int	     uboGPUFrame;

	CBoneCache( const model_t *amod, const mdxaHeader_t *aheader )
		: header(aheader)
		, mod(amod)
		, mBones(header->numBones)
		, mFinalBones(header->numBones)
		, mSmoothBones(header->numBones)
		, mCurrentTouch(3)
		, mLastTouch(2)
		, mLastLastTouch(1)
		, mSmoothingActive(false)
		, mUnsquash(false)
		, mSmoothFactor(0.0f)
		, uboOffset(-1)
		, uboGPUFrame(-1)
	{
		assert(amod);
		assert(aheader);

		Com_Memset(boneMatrices, 0, sizeof(boneMatrices));

		int numBones = header->numBones;
		mdxaSkelOffsets_t *offsets =
			(mdxaSkelOffsets_t *)((byte *)header + sizeof(mdxaHeader_t));

		for ( int i = 0; i < numBones; ++i )
		{
			mdxaSkel_t *skel =
				(mdxaSkel_t *)((byte *)offsets + offsets->offsets[i]);
			mFinalBones[i].parent = skel->parent;
		}
	}

	SBoneCalc &Root()
	{
		assert(mBones.size());
		return mBones[0];
	}

	const mdxaBone_t &EvalUnsmooth( int index )
	{
		EvalLow(index);
		if ( mSmoothingActive && mSmoothBones[index].touch )
		{
			return mSmoothBones[index].boneMatrix;
		}

		return mFinalBones[index].boneMatrix;
	}

	const mdxaBone_t &Eval( int index )
	{
		//Hey, this is what sof2 does. Let's try it out.
		assert(index >= 0 && index <( int)mBones.size());
		if ( mFinalBones[index].touch != mCurrentTouch )
		{
			EvalLow(index);
		}

		return mFinalBones[index].boneMatrix;
	}

	const mdxaBone_t &EvalRender( int index )
	{
		assert(index >= 0 && index < (int)mBones.size());
		if ( mFinalBones[index].touch != mCurrentTouch )
		{
			mFinalBones[index].touchRender = mCurrentTouchRender;
			EvalLow(index);
		}

		if ( mSmoothingActive )
		{
			if ( mSmoothBones[index].touch != mCurrentTouch )
			{
				SmoothLow(index);
			}

			return mSmoothBones[index].boneMatrix;
		}

		return mFinalBones[index].boneMatrix;
	}

	bool WasRendered( int index )
	{
		assert(index >= 0 && index < (int)mBones.size());
		return mFinalBones[index].touchRender == mCurrentTouchRender;
	}

	int GetParent( int index )
	{
		if ( index == 0 )
		{
			return -1;
		}

		assert( index > 0 && index < (int)mBones.size());
		return mFinalBones[index].parent;
	}
};

void RemoveBoneCache(CBoneCache *boneCache)
{
#ifdef _FULL_G2_LEAK_CHECKING
	g_Ghoul2Allocations -= sizeof(*boneCache);
#endif

	delete boneCache;
}

#ifdef _G2_LISTEN_SERVER_OPT
void CopyBoneCache(CBoneCache *to, CBoneCache *from)
{
	memcpy(to, from, sizeof(CBoneCache));
}
#endif

const mdxaBone_t &EvalBoneCache(int index,CBoneCache *boneCache)
{
	assert(boneCache);
	return boneCache->Eval(index);
}

// rww - RAGDOLL_BEGIN
const mdxaHeader_t *G2_GetModA(CGhoul2Info &ghoul2)
{
	if (!ghoul2.mBoneCache)
	{
		return 0;
	}

	CBoneCache &boneCache=*ghoul2.mBoneCache;
	return boneCache.header;
}

int G2_GetBoneDependents(CGhoul2Info &ghoul2,int boneNum,int *tempDependents,int maxDep)
{
	// fixme, these should be precomputed
	if (!ghoul2.mBoneCache || !maxDep)
	{
		return 0;
	}

	CBoneCache &boneCache = *ghoul2.mBoneCache;
	mdxaSkelOffsets_t *offsets =
		(mdxaSkelOffsets_t *)((byte *)boneCache.header + sizeof(mdxaHeader_t));
	mdxaSkel_t *skel =
		(mdxaSkel_t *)((byte *)offsets + offsets->offsets[boneNum]);

	int numDependencies = 0;
	for (int i = 0; i < skel->numChildren; i++)
	{
		if (!maxDep)
		{
			return i; // number added
		}

		*tempDependents = skel->children[i];
		assert(*tempDependents > 0 && *tempDependents < boneCache.header->numBones);
		maxDep--;
		tempDependents++;
		numDependencies++;
	}

	for (int i = 0; i < skel->numChildren; i++)
	{
		int num = G2_GetBoneDependents(ghoul2, skel->children[i], tempDependents, maxDep);
		tempDependents += num;
		numDependencies += num;
		maxDep -= num;
		assert(maxDep >= 0);
		if (!maxDep)
		{
			break;
		}
	}

	return numDependencies;
}

bool G2_WasBoneRendered(CGhoul2Info &ghoul2,int boneNum)
{
	if (!ghoul2.mBoneCache)
	{
		return false;
	}

	CBoneCache &boneCache = *ghoul2.mBoneCache;
	return boneCache.WasRendered(boneNum);
}

void G2_GetBoneBasepose(
	CGhoul2Info &ghoul2,
	int boneNum,
	mdxaBone_t *&retBasepose,
	mdxaBone_t *&retBaseposeInv)
{
	if (!ghoul2.mBoneCache)
	{
		// yikes
		retBasepose = const_cast<mdxaBone_t *>(&identityMatrix);
		retBaseposeInv = const_cast<mdxaBone_t *>(&identityMatrix);
		return;
	}

	assert(ghoul2.mBoneCache);
	CBoneCache &boneCache = *ghoul2.mBoneCache;
	assert(boneCache.mod);
	assert(boneNum >= 0 && boneNum < boneCache.header->numBones);

	mdxaSkelOffsets_t *offsets =
		(mdxaSkelOffsets_t *)((byte *)boneCache.header + sizeof(mdxaHeader_t));
	mdxaSkel_t *skel = (mdxaSkel_t *)((byte *)offsets + offsets->offsets[boneNum]);

	retBasepose = &skel->BasePoseMat;
	retBaseposeInv = &skel->BasePoseMatInv;
}

char *G2_GetBoneNameFromSkel(CGhoul2Info &ghoul2, int boneNum)
{
	if (!ghoul2.mBoneCache)
	{
		return NULL;
	}

	CBoneCache &boneCache = *ghoul2.mBoneCache;
	assert(boneCache.mod);
	assert(boneNum >= 0 && boneNum < boneCache.header->numBones);

	mdxaSkelOffsets_t *offsets =
		(mdxaSkelOffsets_t *)((byte *)boneCache.header + sizeof(mdxaHeader_t));
	mdxaSkel_t *skel = (mdxaSkel_t *)((byte *)offsets + offsets->offsets[boneNum]);

	return skel->name;
}

void G2_RagGetBoneBasePoseMatrixLow(
	CGhoul2Info &ghoul2,
	int boneNum,
	mdxaBone_t &boneMatrix,
	mdxaBone_t &retMatrix,
	vec3_t scale)
{
	assert(ghoul2.mBoneCache);
	CBoneCache &boneCache = *ghoul2.mBoneCache;
	assert(boneCache.mod);
	assert(boneNum >= 0 && boneNum < boneCache.header->numBones);

	mdxaSkelOffsets_t *offsets =
		(mdxaSkelOffsets_t *)((byte *)boneCache.header + sizeof(mdxaHeader_t));
	mdxaSkel_t *skel = (mdxaSkel_t *)((byte *)offsets + offsets->offsets[boneNum]);

	Mat3x4_Multiply(&retMatrix, &boneMatrix, &skel->BasePoseMat);

	if (scale[0])
	{
		retMatrix.matrix[0][3] *= scale[0];
	}

	if (scale[1])
	{
		retMatrix.matrix[1][3] *= scale[1];
	}

	if (scale[2])
	{
		retMatrix.matrix[2][3] *= scale[2];
	}

	VectorNormalize((float *)&retMatrix.matrix[0]);
	VectorNormalize((float *)&retMatrix.matrix[1]);
	VectorNormalize((float *)&retMatrix.matrix[2]);
}

void G2_GetBoneMatrixLow(
	CGhoul2Info &ghoul2,
	int boneNum,
	const vec3_t scale,
	mdxaBone_t &retMatrix,
	mdxaBone_t *&retBasepose,
	mdxaBone_t *&retBaseposeInv)
{
	if ( !ghoul2.mBoneCache )
	{
		retMatrix = identityMatrix;
		// yikes
		retBasepose = const_cast<mdxaBone_t *>(&identityMatrix);
		retBaseposeInv = const_cast<mdxaBone_t *>(&identityMatrix);
		return;
	}

	mdxaBone_t bolt;
	assert(ghoul2.mBoneCache);
	CBoneCache &boneCache = *ghoul2.mBoneCache;
	assert(boneCache.mod);
	assert(boneNum >= 0 && boneNum < boneCache.header->numBones);

	mdxaSkelOffsets_t *offsets =
		(mdxaSkelOffsets_t *)((byte *)boneCache.header + sizeof(mdxaHeader_t));
	mdxaSkel_t *skel = (mdxaSkel_t *)((byte *)offsets + offsets->offsets[boneNum]);

	Mat3x4_Multiply(
		&bolt,
		&boneCache.Eval(boneNum),
		&skel->BasePoseMat);
	retBasepose = &skel->BasePoseMat;
	retBaseposeInv = &skel->BasePoseMatInv;

	if (scale[0])
	{
		bolt.matrix[0][3] *= scale[0];
	}

	if (scale[1])
	{
		bolt.matrix[1][3] *= scale[1];
	}

	if (scale[2])
	{
		bolt.matrix[2][3] *= scale[2];
	}

	VectorNormalize((float *)&bolt.matrix[0]);
	VectorNormalize((float *)&bolt.matrix[1]);
	VectorNormalize((float *)&bolt.matrix[2]);

	Mat3x4_Multiply(&retMatrix, &worldMatrix, &bolt);

#ifdef _DEBUG
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			assert(!Q_isnan(retMatrix.matrix[i][j]));
		}
	}
#endif // _DEBUG
}

int G2_GetParentBoneMatrixLow(
	CGhoul2Info &ghoul2,
	int boneNum,
	const vec3_t scale,
	mdxaBone_t &retMatrix,
	mdxaBone_t *&retBasepose,
	mdxaBone_t *&retBaseposeInv)
{
	int parent = -1;
	if ( ghoul2.mBoneCache )
	{
		CBoneCache &boneCache = *ghoul2.mBoneCache;
		assert(boneCache.mod);
		assert(boneNum >= 0 && boneNum < boneCache.header->numBones);

		parent = boneCache.GetParent(boneNum);
		if ( parent < 0 || parent >= boneCache.header->numBones )
		{
			parent = -1;
			retMatrix = identityMatrix;

			// yikes
			retBasepose = const_cast<mdxaBone_t *>(&identityMatrix);
			retBaseposeInv = const_cast<mdxaBone_t *>(&identityMatrix);
		}
		else
		{
			G2_GetBoneMatrixLow(
				ghoul2,
				parent,
				scale,
				retMatrix,
				retBasepose,
				retBaseposeInv);
		}
	}
	return parent;
}
//rww - RAGDOLL_END

class CRenderSurface
{
public:
	int				surfaceNum;
	surfaceInfo_v	&rootSList;
	shader_t		*cust_shader;
	int				fogNum;
	qboolean		personalModel;
	CBoneCache		*boneCache;
	int				renderfx;
	skin_t			*skin;
	model_t			*currentModel;
	int				lod;
	boltInfo_v		&boltList;
#ifdef _G2_GORE
	shader_t		*gore_shader;
	CGoreSet		*gore_set;
#endif

	CRenderSurface(
		int				initsurfaceNum,
		surfaceInfo_v	&initrootSList,
		shader_t		*initcust_shader,
		int				initfogNum,
		qboolean		initpersonalModel,
		CBoneCache		*initboneCache,
		int				initrenderfx,
		skin_t			*initskin,
		model_t			*initcurrentModel,
		int				initlod,
#ifdef _G2_GORE
		boltInfo_v		&initboltList,
		shader_t		*initgore_shader,
		CGoreSet		*initgore_set
#else
		boltInfo_v		&initboltList
#endif
		)
		: surfaceNum(initsurfaceNum)
		, rootSList(initrootSList)
		, cust_shader(initcust_shader)
		, fogNum(initfogNum)
		, personalModel(initpersonalModel)
		, boneCache(initboneCache)
		, renderfx(initrenderfx)
		, skin(initskin)
		, currentModel(initcurrentModel)
		, lod(initlod)
		, boltList(initboltList)
#ifdef _G2_GORE
		, gore_shader(initgore_shader)
		, gore_set(initgore_set)
#endif
	{
	}
};

/*

All bones should be an identity orientation to display the mesh exactly
as it is specified.

For all other frames, the bones represent the transformation from the
orientation of the bone in the base frame to the orientation in this
frame.

*/


/*
=============
R_ACullModel
=============
*/
static int R_GCullModel( trRefEntity_t *ent ) {

	// scale the radius if need be
	float largestScale = ent->e.modelScale[0];

	if (ent->e.modelScale[1] > largestScale)
	{
		largestScale = ent->e.modelScale[1];
	}
	if (ent->e.modelScale[2] > largestScale)
	{
		largestScale = ent->e.modelScale[2];
	}
	if (!largestScale)
	{
		largestScale = 1;
	}

	// cull bounding sphere
  	switch ( R_CullLocalPointAndRadius( vec3_origin,  ent->e.radius * largestScale) )
  	{
  	case CULL_OUT:
  		tr.pc.c_sphere_cull_md3_out++;
  		return CULL_OUT;

	case CULL_IN:
		tr.pc.c_sphere_cull_md3_in++;
		return CULL_IN;

	case CULL_CLIP:
		tr.pc.c_sphere_cull_md3_clip++;
		return CULL_IN;
 	}
	return CULL_IN;
}


/*
=================
R_AComputeFogNum

=================
*/
static int R_GComputeFogNum( trRefEntity_t *ent ) {

	int				i, j;
	fog_t			*fog;

	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL ) {
		return 0;
	}

	for ( i = 1 ; i < tr.world->numfogs ; i++ ) {
		fog = &tr.world->fogs[i];
		for ( j = 0 ; j < 3 ; j++ ) {
			if ( ent->e.origin[j] - ent->e.radius >= fog->bounds[1][j] ) {
				break;
			}
			if ( ent->e.origin[j] + ent->e.radius <= fog->bounds[0][j] ) {
				break;
			}
		}
		if ( j == 3 ) {
			return i;
		}
	}

	return 0;
}

// work out lod for this entity.
static int G2_ComputeLOD( trRefEntity_t *ent, const model_t *currentModel, int lodBias )
{
	float flod, lodscale;
	float projectedRadius;
	int lod;

	if ( currentModel->numLods < 2 )
	{	// model has only 1 LOD level, skip computations and bias
		return(0);
	}

	if ( r_lodbias->integer > lodBias )
	{
		lodBias = r_lodbias->integer;
	}

	// scale the radius if need be
	float largestScale = ent->e.modelScale[0];

	if (ent->e.modelScale[1] > largestScale)
	{
		largestScale = ent->e.modelScale[1];
	}
	if (ent->e.modelScale[2] > largestScale)
	{
		largestScale = ent->e.modelScale[2];
	}
	if (!largestScale)
	{
		largestScale = 1;
	}

	projectedRadius = ProjectRadius( 0.75*largestScale*ent->e.radius, ent->e.origin );

	// we reduce the radius to make the LOD match other model types which use
	// the actual bound box size
	if ( projectedRadius != 0 )
 	{
 		lodscale = (r_lodscale->value+r_autolodscalevalue->value);
 		if ( lodscale > 20 )
		{
			lodscale = 20;
		}
		else if ( lodscale < 0 )
		{
			lodscale = 0;
		}
 		flod = 1.0f - projectedRadius * lodscale;
 	}
 	else
 	{
 		// object intersects near view plane, e.g. view weapon
 		flod = 0;
 	}
 	flod *= currentModel->numLods;
	lod = Q_ftol( flod );

 	if ( lod < 0 )
 	{
 		lod = 0;
 	}
 	else if ( lod >= currentModel->numLods )
 	{
 		lod = currentModel->numLods - 1;
 	}


	lod += lodBias;

	if ( lod >= currentModel->numLods )
		lod = currentModel->numLods - 1;
	if ( lod < 0 )
		lod = 0;

	return lod;
}

//======================================================================
//
// Bone Manipulation code


void G2_CreateQuaterion(mdxaBone_t *mat, vec4_t quat)
{
	// this is revised for the 3x4 matrix we use in G2.
    float t = 1 + mat->matrix[0][0] + mat->matrix[1][1] + mat->matrix[2][2];
	float s;

	// If the trace of the matrix is greater than zero, then
	// perform an "instant" calculation.
	// Important note wrt. rouning errors:
	// Test if ( T > 0.00000001 ) to avoid large distortions!
	if (t > 0.00000001)
	{
      s = sqrt(t) * 2;
      quat[0] = ( mat->matrix[1][2] - mat->matrix[2][1] ) / s;
      quat[1] = ( mat->matrix[2][0] - mat->matrix[0][2] ) / s;
      quat[2] = ( mat->matrix[0][1] - mat->matrix[1][0] ) / s;
      quat[3] = 0.25 * s;
	}
	else
	{
		// If the trace of the matrix is equal to zero then identify
		// which major diagonal element has the greatest value.

		// Depending on this, calculate the following:
		if ( mat->matrix[0][0] > mat->matrix[1][1] && mat->matrix[0][0] > mat->matrix[2][2] )  {
			// Column 0:
			s  = sqrt( 1.0 + mat->matrix[0][0] - mat->matrix[1][1] - mat->matrix[2][2])* 2;
			quat[0] = 0.25 * s;
			quat[1] = (mat->matrix[0][1] + mat->matrix[1][0] ) / s;
			quat[2] = (mat->matrix[2][0] + mat->matrix[0][2] ) / s;
			quat[3] = (mat->matrix[1][2] - mat->matrix[2][1] ) / s;

		} else if ( mat->matrix[1][1] > mat->matrix[2][2] ) {
			// Column 1:
			s  = sqrt( 1.0 + mat->matrix[1][1] - mat->matrix[0][0] - mat->matrix[2][2] ) * 2;
			quat[0] = (mat->matrix[0][1] + mat->matrix[1][0] ) / s;
			quat[1] = 0.25 * s;
			quat[2] = (mat->matrix[1][2] + mat->matrix[2][1] ) / s;
			quat[3] = (mat->matrix[2][0] - mat->matrix[0][2] ) / s;

		} else {
			// Column 2:
			s  = sqrt( 1.0 + mat->matrix[2][2] - mat->matrix[0][0] - mat->matrix[1][1] ) * 2;
			quat[0] = (mat->matrix[2][0]+ mat->matrix[0][2] ) / s;
			quat[1] = (mat->matrix[1][2] + mat->matrix[2][1] ) / s;
			quat[2] = 0.25 * s;
			quat[3] = (mat->matrix[0][1] - mat->matrix[1][0] ) / s;
		}
	}
}

void G2_CreateMatrixFromQuaterion(mdxaBone_t *mat, vec4_t quat)
{
    const float xx = quat[0] * quat[0];
    const float xy = quat[0] * quat[1];
    const float xz = quat[0] * quat[2];
    const float xw = quat[0] * quat[3];

    const float yy = quat[1] * quat[1];
    const float yz = quat[1] * quat[2];
    const float yw = quat[1] * quat[3];

    const float zz = quat[2] * quat[2];
    const float zw = quat[2] * quat[3];

    mat->matrix[0][0]  = 1 - 2 * ( yy + zz );
    mat->matrix[1][0]  =     2 * ( xy - zw );
    mat->matrix[2][0]  =     2 * ( xz + yw );

    mat->matrix[0][1]  =     2 * ( xy + zw );
    mat->matrix[1][1]  = 1 - 2 * ( xx + zz );
    mat->matrix[2][1]  =     2 * ( yz - xw );

    mat->matrix[0][2]  =     2 * ( xz - yw );
    mat->matrix[1][2]  =     2 * ( yz + xw );
    mat->matrix[2][2]  = 1 - 2 * ( xx + yy );

    mat->matrix[0][3]  = mat->matrix[1][3] = mat->matrix[2][3] = 0;
}

static int G2_GetBonePoolIndex(	const mdxaHeader_t *pMDXAHeader, int iFrame, int iBone)
{
	const int iOffsetToIndex = (iFrame * pMDXAHeader->numBones * 3) + (iBone * 3);
	mdxaIndex_t *pIndex = (mdxaIndex_t *)((byte*)pMDXAHeader + pMDXAHeader->ofsFrames + iOffsetToIndex);

	return (pIndex->iIndex[2] << 16) + (pIndex->iIndex[1] << 8) + (pIndex->iIndex[0]);
}

/*static inline*/ void UnCompressBone(
	float mat[3][4],
	int iBoneIndex,
	const mdxaHeader_t *pMDXAHeader,
	int iFrame)
{
	mdxaCompQuatBone_t *pCompBonePool =
		(mdxaCompQuatBone_t *)((byte *)pMDXAHeader + pMDXAHeader->ofsCompBonePool);
	MC_UnCompressQuat(
		mat,
		pCompBonePool[G2_GetBonePoolIndex(pMDXAHeader, iFrame, iBoneIndex)].Comp);
}

#define DEBUG_G2_TIMING (0)
#define DEBUG_G2_TIMING_RENDER_ONLY (1)

void G2_TimingModel(
	boneInfo_t &bone,
	int currentTime,
	int numFramesInFile,
	int &currentFrame,
	int &newFrame,
	float& lerp)
{
	assert(bone.startFrame >= 0);
	assert(bone.startFrame <= numFramesInFile);
	assert(bone.endFrame >= 0);
	assert(bone.endFrame <= numFramesInFile);

	// yes - add in animation speed to current frame
	const float animSpeed = bone.animSpeed;
	float time;
	if (bone.pauseTime)
	{
		time = (bone.pauseTime - bone.startTime) / 50.0f;
	}
	else
	{
		time = (currentTime - bone.startTime) / 50.0f;
	}

	time = Q_max(0.0f, time);
	float newLerpFrame = bone.startFrame + (time * animSpeed);

	const int numFramesInAnim = bone.endFrame - bone.startFrame;
	const float endFrame = (float)bone.endFrame;

	// we are supposed to be animating right?
	if ( numFramesInAnim != 0 )
	{
		// did we run off the end?
		if ( (animSpeed > 0.0f && newLerpFrame > (endFrame - 1)) ||
			(animSpeed < 0.0f && newLerpFrame < (endFrame + 1)) )
		{
			// yep - decide what to do
			if ( bone.flags & BONE_ANIM_OVERRIDE_LOOP )
			{
				// get our new animation frame back within the bounds of the animation set
				if ( animSpeed < 0.0f )
				{
					// we don't use this case, or so I am told
					// if we do, let me know, I need to insure the mod works

					// should we be creating a virtual frame?
					if ( (newLerpFrame < (endFrame + 1)) && (newLerpFrame >= endFrame) )
					{
						// now figure out what we are lerping between delta is
						// the fraction between this frame and the next, since
						// the new anim is always at a .0f;
						lerp = endFrame + 1 - newLerpFrame;

						// frames are easy to calculate
						currentFrame = endFrame;
						newFrame = bone.startFrame;
					}
					else
					{
						if ( newLerpFrame <= (endFrame + 1) )
						{
							newLerpFrame =
								endFrame + fmod(newLerpFrame - endFrame, numFramesInAnim) -
								numFramesInAnim;
						}

						// now figure out what we are lerping between delta is
						// the fraction between this frame and the next, since
						// the new anim is always at a .0f;
						lerp = ceil(newLerpFrame) - newLerpFrame;

						// frames are easy to calculate
						currentFrame = ceil(newLerpFrame);

						// should we be creating a virtual frame?
						if ( currentFrame <= (endFrame + 1) )
						{
							newFrame = bone.startFrame;
						}
						else
						{
							newFrame = currentFrame - 1;
						}
					}
				}
				else
				{
					// should we be creating a virtual frame?
					if ( (newLerpFrame > (endFrame - 1)) && (newLerpFrame < endFrame))
					{
						// now figure out what we are lerping between delta is
						// the fraction between this frame and the next, since
						// the new anim is always at a .0f;
						lerp = newLerpFrame - (int)newLerpFrame;

						// frames are easy to calculate
						currentFrame = (int)newLerpFrame;
						newFrame = bone.startFrame;
					}
					else
					{
						if ( newLerpFrame >= endFrame )
						{
							newLerpFrame =
								endFrame + fmod(newLerpFrame - endFrame, numFramesInAnim) -
								numFramesInAnim;
						}

						// now figure out what we are lerping between delta is
						// the fraction between this frame and the next, since
						// the new anim is always at a .0f;
						lerp = newLerpFrame - (int)newLerpFrame;

						// frames are easy to calculate
						currentFrame = (int)newLerpFrame;

						// should we be creating a virtual frame?
						if ( newLerpFrame >= (endFrame - 1) )
						{
							newFrame = bone.startFrame;
						}
						else
						{
							newFrame = currentFrame + 1;
						}
					}
				}
			}
			else
			{
				if ( ((bone.flags & BONE_ANIM_OVERRIDE_FREEZE) == BONE_ANIM_OVERRIDE_FREEZE) )
				{
					// if we are supposed to reset the default anim, then do so
					if ( animSpeed > 0.0f )
					{
						currentFrame = bone.endFrame - 1;
					}
					else
					{
						currentFrame = bone.endFrame + 1;
					}

					newFrame = currentFrame;
					lerp = 0.0f;
				}
				else
				{
					bone.flags &= ~BONE_ANIM_TOTAL;
				}
			}
		}
		else
		{
			if (animSpeed> 0.0)
			{
				// frames are easy to calculate
				currentFrame = (int)newLerpFrame;

				// figure out the difference between the two frames	- we have
				// to decide what frame and what percentage of that frame we
				// want to display
				lerp = (newLerpFrame - currentFrame);

				assert(currentFrame>=0&&currentFrame<numFramesInFile);

				newFrame = currentFrame + 1;

				// are we now on the end frame?
				assert((int)endFrame<=numFramesInFile);
				if (newFrame >= (int)endFrame)
				{
					// we only want to lerp with the first frame of the anim if
					// we are looping
					if (bone.flags & BONE_ANIM_OVERRIDE_LOOP)
					{
					  	newFrame = bone.startFrame;
					}
					else
					{
						// if we intend to end this anim or freeze after this, then
						// just keep on the last frame
						newFrame = bone.endFrame-1;
					}
				}
			}
			else
			{
				lerp = (ceil(newLerpFrame)-newLerpFrame);

				// frames are easy to calculate
				currentFrame = ceil(newLerpFrame);
				if (currentFrame>bone.startFrame)
				{
					currentFrame=bone.startFrame;
					newFrame = currentFrame;
					lerp = 0.0f;
				}
				else
				{
					newFrame = currentFrame-1;

					// are we now on the end frame?
					if (newFrame < endFrame+1)
					{
						// we only want to lerp with the first frame of the
						// anim if we are looping
						if (bone.flags & BONE_ANIM_OVERRIDE_LOOP)
						{
					  		newFrame = bone.startFrame;
						}
						// if we intend to end this anim or freeze after this,
						// then just keep on the last frame
						else
						{
							newFrame = bone.endFrame+1;
						}
					}
				}
			}
		}
	}
	else
	{
		if ( animSpeed < 0.0 )
		{
			currentFrame = bone.endFrame + 1;
		}
		else
		{
			currentFrame = bone.endFrame - 1;
		}

		currentFrame = Q_max(0, currentFrame);
		newFrame = currentFrame;

		lerp = 0.0f;
	}

	assert(currentFrame >= 0 && currentFrame < numFramesInFile);
	assert(newFrame >= 0 && newFrame < numFramesInFile);
	assert(lerp >= 0.0f && lerp <= 1.0f);
}

#ifdef _RAG_PRINT_TEST
void G2_RagPrintMatrix(mdxaBone_t *mat);
#endif
// basically construct a seperate skeleton with full hierarchy to store a matrix
// off which will give us the desired settling position given the frame in the skeleton
// that should be used -rww
int G2_Add_Bone(const model_t *mod, boneInfo_v &blist, const char *boneName);
int G2_Find_Bone(const model_t *mod, boneInfo_v &blist, const char *boneName);

void G2_RagGetAnimMatrix(
	CGhoul2Info &ghoul2,
	const int boneNum,
	mdxaBone_t &matrix,
	const int frame)
{
	mdxaBone_t animMatrix;
	mdxaSkel_t *skel;
	mdxaSkel_t *pskel;
	mdxaSkelOffsets_t *offsets;
	int parent;
	int bListIndex;
	int parentBlistIndex;
#ifdef _RAG_PRINT_TEST
	bool actuallySet = false;
#endif

	assert(ghoul2.mBoneCache);
	assert(ghoul2.animModel);

	offsets = (mdxaSkelOffsets_t *)((byte *)ghoul2.mBoneCache->header + sizeof(mdxaHeader_t));
	skel = (mdxaSkel_t *)((byte *)offsets + offsets->offsets[boneNum]);

	//find/add the bone in the list
	if (!skel->name[0])
	{
		bListIndex = -1;
	}
	else
	{
		bListIndex = G2_Find_Bone(ghoul2.animModel, ghoul2.mBlist, skel->name);
		if (bListIndex == -1)
		{
#ifdef _RAG_PRINT_TEST
			Com_Printf("Attempting to add %s\n", skel->name);
#endif
			bListIndex = G2_Add_Bone(ghoul2.animModel, ghoul2.mBlist, skel->name);
		}
	}

	assert(bListIndex != -1);

	boneInfo_t &bone = ghoul2.mBlist[bListIndex];

	if (bone.hasAnimFrameMatrix == frame)
	{ //already calculated so just grab it
		matrix = bone.animFrameMatrix;
		return;
	}

	//get the base matrix for the specified frame
	UnCompressBone(animMatrix.matrix, boneNum, ghoul2.mBoneCache->header, frame);

	parent = skel->parent;
	if (boneNum > 0 && parent > -1)
	{
		// recursively call to assure all parent matrices are set up
		G2_RagGetAnimMatrix(ghoul2, parent, matrix, frame);

		// assign the new skel ptr for our parent
		pskel = (mdxaSkel_t *)((byte *)offsets + offsets->offsets[parent]);

		// taking bone matrix for the skeleton frame and parent's
		// animFrameMatrix into account, determine our final animFrameMatrix
		if (!pskel->name[0])
		{
			parentBlistIndex = -1;
		}
		else
		{
			parentBlistIndex = G2_Find_Bone(ghoul2.animModel, ghoul2.mBlist, pskel->name);
			if (parentBlistIndex == -1)
			{
				parentBlistIndex = G2_Add_Bone(ghoul2.animModel, ghoul2.mBlist, pskel->name);
			}
		}

		assert(parentBlistIndex != -1);

		boneInfo_t &pbone = ghoul2.mBlist[parentBlistIndex];

		// this should have been calc'd in the recursive call
		assert(pbone.hasAnimFrameMatrix == frame);

		Mat3x4_Multiply(&bone.animFrameMatrix, &pbone.animFrameMatrix, &animMatrix);

#ifdef _RAG_PRINT_TEST
		if (parentBlistIndex != -1 && bListIndex != -1)
		{
			actuallySet = true;
		}
		else
		{
			Com_Printf("BAD LIST INDEX: %s, %s [%i]\n", skel->name, pskel->name, parent);
		}
#endif
	}
	else
	{
		// root
		Mat3x4_Multiply(&bone.animFrameMatrix, &ghoul2.mBoneCache->rootMatrix, &animMatrix);
#ifdef _RAG_PRINT_TEST
		if (bListIndex != -1)
		{
			actuallySet = true;
		}
		else
		{
			Com_Printf("BAD LIST INDEX: %s\n", skel->name);
		}
#endif
	}

	//never need to figure it out again
	bone.hasAnimFrameMatrix = frame;

#ifdef _RAG_PRINT_TEST
	if (!actuallySet)
	{
		Com_Printf("SET FAILURE\n");
		G2_RagPrintMatrix(&bone.animFrameMatrix);
	}
#endif

	matrix = bone.animFrameMatrix;
}

static void G2_TransformBone( int child, CBoneCache& BC )
{
	SBoneCalc &TB = BC.mBones[child];
	mdxaBone_t currentBone;
	boneInfo_v& boneList = *BC.rootBoneList;
	int angleOverride = 0;

#if DEBUG_G2_TIMING
	bool printTiming=false;
#endif
	// should this bone be overridden by a bone in the bone list?
	int boneListIndex = G2_Find_Bone_In_List(boneList, child);
	if (boneListIndex != -1)
	{
		// we found a bone in the list - we need to override something here.
		boneInfo_t& bone = boneList[boneListIndex];
		int boneFlags = bone.flags;

		// do we override the rotational angles?
		if ( boneFlags & BONE_ANGLES_TOTAL )
		{
			angleOverride = boneFlags & BONE_ANGLES_TOTAL;
		}

		// set blending stuff if we need to
		if ( boneFlags & BONE_ANIM_BLEND )
		{
			const float blendTime = BC.incomingTime - bone.blendStart;

			// only set up the blend anim if we actually have some blend time
			// left on this bone anim - otherwise we might corrupt some blend
			// higher up the hiearchy
			if ( blendTime >= 0.0f && blendTime < bone.blendTime )
			{
				TB.blendFrame = bone.blendFrame;
				TB.blendOldFrame = bone.blendLerpFrame;
				TB.blendLerp = (blendTime / bone.blendTime);
				TB.blendMode = true;
			}
			else
			{
				TB.blendMode = false;
			}
		}
		else if ( boneFlags & (BONE_ANIM_OVERRIDE_LOOP | BONE_ANIM_OVERRIDE) )
		{
			// turn off blending if we are just doing a straing animation
			// override
			TB.blendMode = false;
		}

		// should this animation be overridden by an animation in the bone
		// list?
		if (boneFlags & (BONE_ANIM_OVERRIDE_LOOP | BONE_ANIM_OVERRIDE))
		{
			G2_TimingModel(
				bone,
				BC.incomingTime,
				BC.header->numFrames,
				TB.currentFrame,
				TB.newFrame,
				TB.backlerp);
		}

#if DEBUG_G2_TIMING
		printTiming=true;
#endif
	}

	// figure out where the location of the bone animation data is
	if ( !(TB.newFrame >= 0 && TB.newFrame < BC.header->numFrames) )
	{
		TB.newFrame = 0;
	}

	if ( !(TB.currentFrame >= 0 && TB.currentFrame < BC.header->numFrames) )
	{
		TB.currentFrame = 0;
	}

	// figure out where the location of the blended animation data is
	if ( TB.blendFrame < 0.0 || TB.blendFrame >= (BC.header->numFrames + 1) )
	{
		TB.blendFrame = 0.0;
	}

	if ( !(TB.blendOldFrame >= 0 && TB.blendOldFrame < BC.header->numFrames) )
	{
		TB.blendOldFrame = 0;
	}

#if DEBUG_G2_TIMING

#if DEBUG_G2_TIMING_RENDER_ONLY
	if (!HackadelicOnClient)
	{
		printTiming = false;
	}
#endif

	if (printTiming)
	{
		char mess[1000];
		if (TB.blendMode)
		{
			sprintf(
				mess,
				"b %2d %5d   %4d %4d %4d %4d  %f %f\n",
				boneListIndex,
				BC.incomingTime,
				(int)TB.newFrame,
				(int)TB.currentFrame,
				(int)TB.blendFrame,
				(int)TB.blendOldFrame,
				TB.backlerp,
				TB.blendLerp);
		}
		else
		{
			sprintf(
				mess,
				"a %2d %5d   %4d %4d            %f\n",
				boneListIndex,
				BC.incomingTime,
				TB.newFrame,
				TB.currentFrame,
				TB.backlerp);
		}

		Com_OPrintf("%s",mess);

		const boneInfo_t &bone=boneList[boneListIndex];
		if (bone.flags&BONE_ANIM_BLEND)
		{
			sprintf(
				mess,
				"                                                                    bfb[%2d] %5d  %5d  (%5d-%5d) %4.2f %4x   bt(%5d-%5d) %7.2f %5d\n",
				boneListIndex,
				BC.incomingTime,
				bone.startTime,
				bone.startFrame,
				bone.endFrame,
				bone.animSpeed,
				bone.flags,
				bone.blendStart,
				bone.blendStart+bone.blendTime,
				bone.blendFrame,
				bone.blendLerpFrame);
		}
		else
		{
			sprintf(mess,"                                                                    bfa[%2d] %5d  %5d  (%5d-%5d) %4.2f %4x\n",
				boneListIndex,
				BC.incomingTime,
				bone.startTime,
				bone.startFrame,
				bone.endFrame,
				bone.animSpeed,
				bone.flags);
		}
	}
#endif

	assert(child >=0 && child < BC.header->numBones);

	// decide where the transformed bone is going

	// lerp this bone - use the temp space on the ref entity to put the bone
	// transforms into
  	if ( !TB.backlerp )
  	{
		UnCompressBone(currentBone.matrix, child, BC.header, TB.currentFrame);
  	}
	else
  	{
		mdxaBone_t newFrameBone;
		mdxaBone_t currentFrameBone;
		UnCompressBone(newFrameBone.matrix, child, BC.header, TB.newFrame);
		UnCompressBone(currentFrameBone.matrix, child, BC.header, TB.currentFrame);
		Mat3x4_Lerp(&currentBone, &newFrameBone, &currentFrameBone, TB.backlerp);
	}

	// are we blending with another frame of anim?
	// blend in the other frame if we need to
	if ( TB.blendMode )
	{
		mdxaBone_t blendFrameBone;
		mdxaBone_t blendOldFrameBone;
		UnCompressBone(blendFrameBone.matrix, child, BC.header, TB.blendFrame);
		UnCompressBone(blendOldFrameBone.matrix, child, BC.header, TB.blendOldFrame);

		const float backlerp = TB.blendFrame - (int)TB.blendFrame;
		mdxaBone_t lerpFrameBone;
		Mat3x4_Lerp(&lerpFrameBone, &blendFrameBone, &blendOldFrameBone, backlerp);
		Mat3x4_Lerp(&currentBone, &currentBone, &lerpFrameBone, TB.blendLerp);
	}

	if ( !child )
	{
		// now multiply by the root matrix, so we can offset this model
		// should we need to
		BC.mFinalBones[child].boneMatrix = BC.rootMatrix * currentBone;
	}

	// figure out where the bone hirearchy info is
	mdxaSkelOffsets_t *offsets =
		(mdxaSkelOffsets_t *)((byte *)BC.header + sizeof(mdxaHeader_t));
	mdxaSkel_t *skel =
		(mdxaSkel_t *)((byte *)offsets + offsets->offsets[child]);

	int parent = BC.mFinalBones[child].parent;
	assert((parent == -1 && child == 0) || (parent >= 0 && parent < (int)BC.mBones.size()));

	if ( angleOverride & BONE_ANGLES_REPLACE )
	{
		const bool isRag =
			((angleOverride & (BONE_ANGLES_RAGDOLL | BONE_ANGLES_IK)) != 0);

		mdxaBone_t& bone = BC.mFinalBones[child].boneMatrix;
		const boneInfo_t& boneOverride = boneList[boneListIndex];

		// give us the matrix the animation thinks we should have, so we
		// can get the correct X&Y coors
		const mdxaBone_t firstPass = BC.mFinalBones[parent].boneMatrix * currentBone;
		if (isRag)
		{
			// this is crazy, we are gonna drive the animation to ID while we
			// are doing post mults to compensate.
			mdxaBone_t temp = firstPass * skel->BasePoseMat;
			const float matrixScale = VectorLength((float*)&temp);
			mdxaBone_t toMatrix =
			{
				{
					{ 1.0f, 0.0f, 0.0f, 0.0f },
					{ 0.0f, 1.0f, 0.0f, 0.0f },
					{ 0.0f, 0.0f, 1.0f, 0.0f }
				}
			};
			toMatrix.matrix[0][0] = matrixScale;
			toMatrix.matrix[1][1] = matrixScale;
			toMatrix.matrix[2][2] = matrixScale;
			toMatrix.matrix[0][3] = temp.matrix[0][3];
			toMatrix.matrix[1][3] = temp.matrix[1][3];
			toMatrix.matrix[2][3] = temp.matrix[2][3];

			temp = toMatrix * skel->BasePoseMatInv;

			float blendTime = BC.incomingTime - boneOverride.boneBlendStart;
			float blendLerp = (blendTime / boneOverride.boneBlendTime);
			if (blendLerp > 0.0f)
			{
				// has started
				if (blendLerp > 1.0f)
				{
					bone = temp;
				}
				else
				{
					Mat3x4_Lerp(&bone, &temp, &currentBone, blendLerp);
				}
			}
		}
		else
		{
			// are we attempting to blend with the base animation? and still
			// within blend time?
			if (boneOverride.boneBlendTime > 0.0f &&
				(((boneOverride.boneBlendStart + boneOverride.boneBlendTime) < BC.incomingTime)))
			{
				// ok, we are supposed to be blending. Work out lerp
				const float blendTime = BC.incomingTime - boneOverride.boneBlendStart;
				const float blendLerp = (blendTime / boneOverride.boneBlendTime);

				if (blendLerp <= 1)
				{
					if (blendLerp < 0)
					{
						assert(0);
					}

					// now work out the matrix we want to get *to* - firstPass
					// is where we are coming *from*
					mdxaBone_t temp = firstPass * skel->BasePoseMat;
					const float matrixScale = VectorLength((const float*)&temp);

					const mdxaBone_t& m =
						HackadelicOnClient ? boneOverride.newMatrix : boneOverride.matrix;

					mdxaBone_t newMatrixTemp;
					Mat3x4_Scale(&newMatrixTemp, &m, matrixScale);
					newMatrixTemp.matrix[0][3] = temp.matrix[0][3];
					newMatrixTemp.matrix[1][3] = temp.matrix[1][3];
					newMatrixTemp.matrix[2][3] = temp.matrix[2][3];

 					temp = newMatrixTemp * skel->BasePoseMatInv;
					Mat3x4_Lerp(&bone, &temp, &firstPass, blendLerp);
				}
				else
				{
					bone = firstPass;
				}
			}
			else
			{
				// no, so just override it directly
				const mdxaBone_t temp = firstPass * skel->BasePoseMat;
				const float matrixScale = VectorLength((const float*)&temp);

				const mdxaBone_t& m =
					HackadelicOnClient ? boneOverride.newMatrix : boneOverride.matrix;

				mdxaBone_t newMatrixTemp;
				Mat3x4_Scale(&newMatrixTemp, &m, matrixScale);
				newMatrixTemp.matrix[0][3] = temp.matrix[0][3];
				newMatrixTemp.matrix[1][3] = temp.matrix[1][3];
				newMatrixTemp.matrix[2][3] = temp.matrix[2][3];

 				bone = newMatrixTemp * skel->BasePoseMatInv;
			}
		}
	}
	else if (angleOverride & BONE_ANGLES_PREMULT)
	{
		const mdxaBone_t& boneA =
			child ? BC.mFinalBones[parent].boneMatrix : BC.rootMatrix;

		const mdxaBone_t& boneB =
			HackadelicOnClient ?
				boneList[boneListIndex].newMatrix :
				boneList[boneListIndex].matrix;

		BC.mFinalBones[child].boneMatrix = boneA * boneB;

		if ((angleOverride & (BONE_ANGLES_RAGDOLL | BONE_ANGLES_IK)) == 0)
		{
			BC.mFinalBones[child].boneMatrix =
				BC.mFinalBones[child].boneMatrix * currentBone;
		}
	}
	else if (child)
	{
		// now transform the matrix by it's parent, asumming we have a parent, and
		// we aren't overriding the angles absolutely
		BC.mFinalBones[child].boneMatrix =
			BC.mFinalBones[parent].boneMatrix * currentBone;
	}

	// now multiply our resulting bone by an override matrix should we need to
	if (angleOverride & BONE_ANGLES_POSTMULT)
	{
		const mdxaBone_t& postMultMatrix =
			HackadelicOnClient ?
				boneList[boneListIndex].newMatrix :
				boneList[boneListIndex].matrix;

		BC.mFinalBones[child].boneMatrix =
			BC.mFinalBones[child].boneMatrix * postMultMatrix;
	}
}

void G2_SetUpBolts(
	mdxaHeader_t *header,
	CGhoul2Info &ghoul2,
	mdxaBone_v &bonePtr,
	boltInfo_v &boltList)
{
	mdxaSkelOffsets_t *offsets =
		(mdxaSkelOffsets_t *)((byte *)header + sizeof(mdxaHeader_t));

	for ( size_t i = 0; i < boltList.size(); ++i )
	{
		if ( boltList[i].boneNumber == -1 )
		{
			continue;
		}

		// figure out where the bone hirearchy info is
		mdxaSkel_t *skel =
			(mdxaSkel_t *)((byte *)offsets + offsets->offsets[boltList[i].boneNumber]);
		boltList[i].position = bonePtr[boltList[i].boneNumber].second * skel->BasePoseMat;
	}
}

#define		GHOUL2_RAG_STARTED						0x0010
//rwwFIXMEFIXME: Move this into the stupid header or something.

static void G2_TransformGhoulBones(
	boneInfo_v &rootBoneList,
	mdxaBone_t &rootMatrix,
	CGhoul2Info &ghoul2,
	int time,
	bool smooth = true)
{
#ifdef G2_PERFORMANCE_ANALYSIS
	G2PerformanceTimer_G2_TransformGhoulBones.Start();
	G2PerformanceCounter_G2_TransformGhoulBones++;
#endif

	model_t *currentModel = (model_t *)ghoul2.currentModel;
	mdxaHeader_t *aHeader = (mdxaHeader_t *)ghoul2.aHeader;

	assert(ghoul2.aHeader);
	assert(ghoul2.currentModel);
	assert(ghoul2.currentModel->data.glm && ghoul2.currentModel->data.glm->header);
	if (!aHeader->numBones)
	{
		assert(0); // this would be strange
		return;
	}
	if (!ghoul2.mBoneCache)
	{
		ghoul2.mBoneCache=new CBoneCache(currentModel,aHeader);

#ifdef _FULL_G2_LEAK_CHECKING
		g_Ghoul2Allocations += sizeof(*ghoul2.mBoneCache);
#endif
	}
	ghoul2.mBoneCache->mod=currentModel;
	ghoul2.mBoneCache->header=aHeader;
	assert(ghoul2.mBoneCache->mBones.size()==(unsigned)aHeader->numBones);

	ghoul2.mBoneCache->mSmoothingActive=false;
	ghoul2.mBoneCache->mUnsquash=false;

	// master smoothing control
	if (HackadelicOnClient && smooth && !ri.Cvar_VariableIntegerValue("dedicated"))
	{
		ghoul2.mBoneCache->mLastTouch = ghoul2.mBoneCache->mLastLastTouch;

		// master smoothing control
		float val = r_Ghoul2AnimSmooth->value;
		if (val > 0.0f && val < 1.0f)
		{
			if (ghoul2.mFlags & GHOUL2_CRAZY_SMOOTH)
			{
				val = 0.9f;
			}
			else if (ghoul2.mFlags & GHOUL2_RAG_STARTED)
			{
				for (size_t k = 0; k < rootBoneList.size(); k++)
				{
					boneInfo_t &bone = rootBoneList[k];
					if (bone.flags & BONE_ANGLES_RAGDOLL)
					{
						if (bone.firstCollisionTime &&
							bone.firstCollisionTime > (time - 250) &&
							bone.firstCollisionTime < time)
						{
							val = 0.9f;
						}
						else if (bone.airTime > time)
						{
							val = 0.2f;
						}
						else
						{
							val = 0.8f;
						}
						break;
					}
				}
			}

			ghoul2.mBoneCache->mSmoothFactor = val; // meaningless formula
			ghoul2.mBoneCache->mSmoothingActive = true;

			if (r_Ghoul2UnSqashAfterSmooth->integer)
			{
				ghoul2.mBoneCache->mUnsquash = true;
			}
		}
	}
	else
	{
		ghoul2.mBoneCache->mSmoothFactor = 1.0f;
	}

	ghoul2.mBoneCache->mCurrentTouch++;

	if (HackadelicOnClient)
	{
		ghoul2.mBoneCache->mLastLastTouch = ghoul2.mBoneCache->mCurrentTouch;
		ghoul2.mBoneCache->mCurrentTouchRender = ghoul2.mBoneCache->mCurrentTouch;
	}
	else
	{
		ghoul2.mBoneCache->mCurrentTouchRender = 0;
	}

	ghoul2.mBoneCache->frameSize = 0;

	ghoul2.mBoneCache->rootBoneList = &rootBoneList;
	ghoul2.mBoneCache->rootMatrix = rootMatrix;
	ghoul2.mBoneCache->incomingTime = time;

	SBoneCalc &TB = ghoul2.mBoneCache->Root();
	TB.newFrame = 0;
	TB.currentFrame = 0;
	TB.backlerp = 0.0f;
	TB.blendFrame = 0;
	TB.blendOldFrame = 0;
	TB.blendMode = false;
	TB.blendLerp = 0;

#ifdef G2_PERFORMANCE_ANALYSIS
	G2Time_G2_TransformGhoulBones += G2PerformanceTimer_G2_TransformGhoulBones.End();
#endif
}


#define MDX_TAG_ORIGIN 2

//======================================================================
//
// Surface Manipulation code


// We've come across a surface that's designated as a bolt surface, process it and put it in the appropriate bolt place
void G2_ProcessSurfaceBolt(
	mdxaBone_v &bonePtr,
	mdxmSurface_t *surface,
	int boltNum,
	boltInfo_v &boltList,
	surfaceInfo_t *surfInfo,
	model_t *mod)
{
	mdxmVertex_t *v, *vert0, *vert1, *vert2;
	vec3_t axes[3], sides[3];
	float pTri[3][3], d;
	int j, k;

	// now there are two types of tag surface - model ones and procedural
	// generated types - lets decide which one we have here.
	if (surfInfo && surfInfo->offFlags == G2SURFACEFLAG_GENERATED)
	{
		int surfNumber = surfInfo->genPolySurfaceIndex & 0x0ffff;
		int polyNumber = (surfInfo->genPolySurfaceIndex >> 16) & 0x0ffff;

		// find original surface our original poly was in.
		mdxmSurface_t *originalSurf =
			(mdxmSurface_t *)G2_FindSurface((void *)mod, surfNumber, surfInfo->genLod);
		mdxmTriangle_t *originalTriangleIndexes =
			(mdxmTriangle_t *)((byte *)originalSurf + originalSurf->ofsTriangles);

		// get the original polys indexes
		int index0 = originalTriangleIndexes[polyNumber].indexes[0];
		int index1 = originalTriangleIndexes[polyNumber].indexes[1];
		int index2 = originalTriangleIndexes[polyNumber].indexes[2];

		// decide where the original verts are

		vert0 = (mdxmVertex_t *)((byte *)originalSurf + originalSurf->ofsVerts);
		vert0 += index0;

		vert1 = (mdxmVertex_t *)((byte *)originalSurf + originalSurf->ofsVerts);
		vert1 += index1;

		vert2 = (mdxmVertex_t *)((byte *)originalSurf + originalSurf->ofsVerts);
		vert2 += index2;

		// clear out the triangle verts to be
		VectorClear(pTri[0]);
		VectorClear(pTri[1]);
		VectorClear(pTri[2]);

		int *piBoneRefs = (int *)((byte *)originalSurf + originalSurf->ofsBoneReferences);

		// now go and transform just the points we need from the surface that
		// was hit originally
		//		w = vert0->weights;
		float fTotalWeight = 0.0f;
		int iNumWeights = G2_GetVertWeights(vert0);
		for (k = 0; k < iNumWeights; k++)
		{
			int iBoneIndex = G2_GetVertBoneIndex(vert0, k);
			float fBoneWeight = G2_GetVertBoneWeight(vert0, k, fTotalWeight, iNumWeights);

			pTri[0][0] +=
				fBoneWeight *
				(DotProduct(bonePtr[piBoneRefs[iBoneIndex]].second.matrix[0], vert0->vertCoords) +
				 bonePtr[piBoneRefs[iBoneIndex]].second.matrix[0][3]);
			pTri[0][1] +=
				fBoneWeight *
				(DotProduct(bonePtr[piBoneRefs[iBoneIndex]].second.matrix[1], vert0->vertCoords) +
				 bonePtr[piBoneRefs[iBoneIndex]].second.matrix[1][3]);
			pTri[0][2] +=
				fBoneWeight *
				(DotProduct(bonePtr[piBoneRefs[iBoneIndex]].second.matrix[2], vert0->vertCoords) +
				 bonePtr[piBoneRefs[iBoneIndex]].second.matrix[2][3]);
		}

		fTotalWeight = 0.0f;
		iNumWeights = G2_GetVertWeights(vert1);
		for (k = 0; k < iNumWeights; k++)
		{
			int iBoneIndex = G2_GetVertBoneIndex(vert1, k);
			float fBoneWeight = G2_GetVertBoneWeight(vert1, k, fTotalWeight, iNumWeights);

			pTri[1][0] +=
				fBoneWeight *
				(DotProduct(bonePtr[piBoneRefs[iBoneIndex]].second.matrix[0], vert1->vertCoords) +
				 bonePtr[piBoneRefs[iBoneIndex]].second.matrix[0][3]);
			pTri[1][1] +=
				fBoneWeight *
				(DotProduct(bonePtr[piBoneRefs[iBoneIndex]].second.matrix[1], vert1->vertCoords) +
				 bonePtr[piBoneRefs[iBoneIndex]].second.matrix[1][3]);
			pTri[1][2] +=
				fBoneWeight *
				(DotProduct(bonePtr[piBoneRefs[iBoneIndex]].second.matrix[2], vert1->vertCoords) +
				 bonePtr[piBoneRefs[iBoneIndex]].second.matrix[2][3]);
		}

		fTotalWeight = 0.0f;
		iNumWeights = G2_GetVertWeights(vert2);
		for (k = 0; k < iNumWeights; k++)
		{
			int iBoneIndex = G2_GetVertBoneIndex(vert2, k);
			float fBoneWeight = G2_GetVertBoneWeight(vert2, k, fTotalWeight, iNumWeights);

			pTri[2][0] +=
				fBoneWeight *
				(DotProduct(bonePtr[piBoneRefs[iBoneIndex]].second.matrix[0], vert2->vertCoords) +
				 bonePtr[piBoneRefs[iBoneIndex]].second.matrix[0][3]);
			pTri[2][1] +=
				fBoneWeight *
				(DotProduct(bonePtr[piBoneRefs[iBoneIndex]].second.matrix[1], vert2->vertCoords) +
				 bonePtr[piBoneRefs[iBoneIndex]].second.matrix[1][3]);
			pTri[2][2] +=
				fBoneWeight *
				(DotProduct(bonePtr[piBoneRefs[iBoneIndex]].second.matrix[2], vert2->vertCoords) +
				 bonePtr[piBoneRefs[iBoneIndex]].second.matrix[2][3]);
		}

		vec3_t normal;
		vec3_t up;
		vec3_t right;
		vec3_t vec0, vec1;
		// work out baryCentricK
		float baryCentricK = 1.0 - (surfInfo->genBarycentricI + surfInfo->genBarycentricJ);

		// now we have the model transformed into model space, now generate an origin.
		boltList[boltNum].position.matrix[0][3] = (pTri[0][0] * surfInfo->genBarycentricI) +
												  (pTri[1][0] * surfInfo->genBarycentricJ) +
												  (pTri[2][0] * baryCentricK);
		boltList[boltNum].position.matrix[1][3] = (pTri[0][1] * surfInfo->genBarycentricI) +
												  (pTri[1][1] * surfInfo->genBarycentricJ) +
												  (pTri[2][1] * baryCentricK);
		boltList[boltNum].position.matrix[2][3] = (pTri[0][2] * surfInfo->genBarycentricI) +
												  (pTri[1][2] * surfInfo->genBarycentricJ) +
												  (pTri[2][2] * baryCentricK);

		// generate a normal to this new triangle
		VectorSubtract(pTri[0], pTri[1], vec0);
		VectorSubtract(pTri[2], pTri[1], vec1);

		CrossProduct(vec0, vec1, normal);
		VectorNormalize(normal);

		// forward vector
		boltList[boltNum].position.matrix[0][0] = normal[0];
		boltList[boltNum].position.matrix[1][0] = normal[1];
		boltList[boltNum].position.matrix[2][0] = normal[2];

		// up will be towards point 0 of the original triangle.
		// so lets work it out. Vector is hit point - point 0
		up[0] = boltList[boltNum].position.matrix[0][3] - pTri[0][0];
		up[1] = boltList[boltNum].position.matrix[1][3] - pTri[0][1];
		up[2] = boltList[boltNum].position.matrix[2][3] - pTri[0][2];

		// normalise it
		VectorNormalize(up);

		// that's the up vector
		boltList[boltNum].position.matrix[0][1] = up[0];
		boltList[boltNum].position.matrix[1][1] = up[1];
		boltList[boltNum].position.matrix[2][1] = up[2];

		// right is always straight

		CrossProduct(normal, up, right);
		// that's the up vector
		boltList[boltNum].position.matrix[0][2] = right[0];
		boltList[boltNum].position.matrix[1][2] = right[1];
		boltList[boltNum].position.matrix[2][2] = right[2];
	}
	// no, we are looking at a normal model tag
	else
	{
		int *piBoneRefs = (int *)((byte *)surface + surface->ofsBoneReferences);

		// whip through and actually transform each vertex
		v = (mdxmVertex_t *)((byte *)surface + surface->ofsVerts);
		for (j = 0; j < 3; j++)
		{
			// 			mdxmWeight_t	*w;

			VectorClear(pTri[j]);
			//			w = v->weights;

			const int iNumWeights = G2_GetVertWeights(v);
			float fTotalWeight = 0.0f;
			for (k = 0; k < iNumWeights; k++)
			{
				int iBoneIndex = G2_GetVertBoneIndex(v, k);
				float fBoneWeight = G2_GetVertBoneWeight(v, k, fTotalWeight, iNumWeights);

				// bone = bonePtr + piBoneRefs[w->boneIndex];
				pTri[j][0] +=
					fBoneWeight *
					(DotProduct(bonePtr[piBoneRefs[iBoneIndex]].second.matrix[0], v->vertCoords) +
					 bonePtr[piBoneRefs[iBoneIndex]].second.matrix[0][3]);
				pTri[j][1] +=
					fBoneWeight *
					(DotProduct(bonePtr[piBoneRefs[iBoneIndex]].second.matrix[1], v->vertCoords) +
					 bonePtr[piBoneRefs[iBoneIndex]].second.matrix[1][3]);
				pTri[j][2] +=
					fBoneWeight *
					(DotProduct(bonePtr[piBoneRefs[iBoneIndex]].second.matrix[2], v->vertCoords) +
					 bonePtr[piBoneRefs[iBoneIndex]].second.matrix[2][3]);
			}

			v++;
		}

		// clear out used arrays
		memset(axes, 0, sizeof(axes));
		memset(sides, 0, sizeof(sides));

		// work out actual sides of the tag triangle
		for (j = 0; j < 3; j++)
		{
			sides[j][0] = pTri[(j + 1) % 3][0] - pTri[j][0];
			sides[j][1] = pTri[(j + 1) % 3][1] - pTri[j][1];
			sides[j][2] = pTri[(j + 1) % 3][2] - pTri[j][2];
		}

		// do math trig to work out what the matrix will be from this
		// triangle's translated position
		VectorNormalize2(sides[iG2_TRISIDE_LONGEST], axes[0]);
		VectorNormalize2(sides[iG2_TRISIDE_SHORTEST], axes[1]);

		// project shortest side so that it is exactly 90 degrees to the longer
		// side
		d = DotProduct(axes[0], axes[1]);
		VectorMA(axes[0], -d, axes[1], axes[0]);
		VectorNormalize2(axes[0], axes[0]);

		CrossProduct(sides[iG2_TRISIDE_LONGEST], sides[iG2_TRISIDE_SHORTEST], axes[2]);
		VectorNormalize2(axes[2], axes[2]);

		// set up location in world space of the origin point in out going
		// matrix
		boltList[boltNum].position.matrix[0][3] = pTri[MDX_TAG_ORIGIN][0];
		boltList[boltNum].position.matrix[1][3] = pTri[MDX_TAG_ORIGIN][1];
		boltList[boltNum].position.matrix[2][3] = pTri[MDX_TAG_ORIGIN][2];

		// copy axis to matrix - do some magic to orient minus Y to positive X
		// and so on so bolt on stuff is oriented correctly
		boltList[boltNum].position.matrix[0][0] = axes[1][0];
		boltList[boltNum].position.matrix[0][1] = axes[0][0];
		boltList[boltNum].position.matrix[0][2] = -axes[2][0];

		boltList[boltNum].position.matrix[1][0] = axes[1][1];
		boltList[boltNum].position.matrix[1][1] = axes[0][1];
		boltList[boltNum].position.matrix[1][2] = -axes[2][1];

		boltList[boltNum].position.matrix[2][0] = axes[1][2];
		boltList[boltNum].position.matrix[2][1] = axes[0][2];
		boltList[boltNum].position.matrix[2][2] = -axes[2][2];
	}
}

// now go through all the generated surfaces that aren't included in the model
// surface hierarchy and create the correct bolt info for them
void G2_ProcessGeneratedSurfaceBolts(CGhoul2Info &ghoul2, mdxaBone_v &bonePtr, model_t *mod_t)
{
#ifdef G2_PERFORMANCE_ANALYSIS
	G2PerformanceTimer_G2_ProcessGeneratedSurfaceBolts.Start();
#endif

	// look through the surfaces off the end of the pre-defined model surfaces
	for ( size_t i = 0, numSurfaces = ghoul2.mSlist.size(); i < numSurfaces; i++ )
	{
		// only look for bolts if we are actually a generated surface, and not
		// just an overriden one
		if (ghoul2.mSlist[i].offFlags & G2SURFACEFLAG_GENERATED)
		{
			// well alrighty then. Lets see if there is a bolt that is
			// attempting to use it
			int boltNum = G2_Find_Bolt_Surface_Num(
				ghoul2.mBltlist, i, G2SURFACEFLAG_GENERATED);

			if (boltNum != -1)
			{
				G2_ProcessSurfaceBolt(
					bonePtr,
					nullptr,
					boltNum,
					ghoul2.mBltlist,
					&ghoul2.mSlist[i], mod_t);
			}
		}
	}

#ifdef G2_PERFORMANCE_ANALYSIS
	G2Time_G2_ProcessGeneratedSurfaceBolts +=
		G2PerformanceTimer_G2_ProcessGeneratedSurfaceBolts.End();
#endif
}

void RenderSurfaces( CRenderSurface &RS, const trRefEntity_t *ent, int entityNum )
{
#ifdef G2_PERFORMANCE_ANALYSIS
	G2PerformanceTimer_RenderSurfaces.Start();
#endif

	int i;
	const shader_t *shader = 0;
	int offFlags = 0;
#ifdef _G2_GORE
	bool drawGore = true;
#endif

	assert(RS.currentModel);
	assert(RS.currentModel->data.glm && RS.currentModel->data.glm->header);
	// back track and get the surfinfo struct for this surface
	mdxmSurface_t *surface =
		(mdxmSurface_t *)G2_FindSurface(RS.currentModel, RS.surfaceNum, RS.lod);
	mdxmHierarchyOffsets_t *surfIndexes = (mdxmHierarchyOffsets_t *)
		((byte *)RS.currentModel->data.glm->header + sizeof(mdxmHeader_t));
	mdxmSurfHierarchy_t *surfInfo = (mdxmSurfHierarchy_t *)
		((byte *)surfIndexes + surfIndexes->offsets[surface->thisSurfaceIndex]);

	// see if we have an override surface in the surface list
	const surfaceInfo_t	*surfOverride = G2_FindOverrideSurface(RS.surfaceNum, RS.rootSList);

	// really, we should use the default flags for this surface unless it's been overriden
	offFlags = surfInfo->flags;

	// set the off flags if we have some
	if (surfOverride)
	{
		offFlags = surfOverride->offFlags;
	}

	// if this surface is not off, add it to the shader render list
	if (!offFlags)
	{
 		if ( RS.cust_shader )
		{
			shader = RS.cust_shader;
		}
		else if ( RS.skin )
		{
			int		j;

			// match the surface name to something in the skin file
			shader = tr.defaultShader;
			for ( j = 0 ; j < RS.skin->numSurfaces ; j++ )
			{
				// the names have both been lowercased
				if ( !strcmp( RS.skin->surfaces[j]->name, surfInfo->name ) )
				{
					shader = (shader_t*)RS.skin->surfaces[j]->shader;
					break;
				}
			}
		}
		else
		{
			shader = R_GetShaderByHandle( surfInfo->shaderIndex );
		}

		// Get dlightBits and Cubemap
		float radius;
		// scale the radius if needed
		float largestScale = MAX(ent->e.modelScale[0], MAX(ent->e.modelScale[1], ent->e.modelScale[2]));
		radius = ent->e.radius * largestScale;
		int dlightBits = R_DLightsForPoint(ent->e.origin, radius);
		int cubemapIndex = R_CubemapForPoint(ent->e.origin);

		// don't add third_person objects if not viewing through a portal
		if ( !RS.personalModel )
		{
			// set the surface info to point at the where the transformed bone
			// list is going to be for when the surface gets rendered out
			CRenderableSurface *newSurf = AllocGhoul2RenderableSurface();
			newSurf->vboMesh = &RS.currentModel->data.glm->vboModels[RS.lod].vboMeshes[RS.surfaceNum];
			assert (newSurf->vboMesh != NULL && RS.surfaceNum == surface->thisSurfaceIndex);
			newSurf->surfaceData = surface;
			newSurf->boneCache = RS.boneCache;
			newSurf->dlightBits = dlightBits;

			// render shadows?
			if (r_shadows->integer == 2
				&& (RS.renderfx & (RF_NOSHADOW | RF_DEPTHHACK))
				&& shader->sort == SS_OPAQUE)
				newSurf->genShadows = qtrue;

			R_AddDrawSurf(
				(surfaceType_t *)newSurf,
				entityNum,
				(shader_t *)shader,
				RS.fogNum,
				qfalse,
				R_IsPostRenderEntity(ent),
				cubemapIndex);

#ifdef _G2_GORE
			if ( RS.gore_set && drawGore )
			{
				int curTime = G2API_GetTime(tr.refdef.time);

				auto range = RS.gore_set->mGoreRecords.equal_range(RS.surfaceNum);
				CRenderableSurface *last = newSurf;
				for ( auto k = range.first; k != range.second; /* blank */ )
				{
					auto kcur = k;
					k++;

					R2GoreTextureCoordinates *tex = FindR2GoreRecord(kcur->second.mGoreTag);
					if (!tex ||	// it is gone, lets get rid of it
						(kcur->second.mDeleteTime &&
						 curTime >= kcur->second.mDeleteTime)) // out of time
					{
						RS.gore_set->mGoreRecords.erase(kcur);
					}
					else if (tex->tex[RS.lod])
					{
						CRenderableSurface *newSurf2 = AllocGhoul2RenderableSurface();
						*newSurf2 = *newSurf;
						newSurf2->goreChain = 0;
						newSurf2->alternateTex = tex->tex[RS.lod];
						newSurf2->scale = 1.0f;
						newSurf2->fade = 1.0f;
						newSurf2->impactTime = 1.0f;  // done with
						int magicFactor42 = 500; // ms, impact time
						if (curTime > kcur->second.mGoreGrowStartTime &&
							curTime < (kcur->second.mGoreGrowStartTime + magicFactor42) )
						{
							newSurf2->impactTime =
								float(curTime - kcur->second.mGoreGrowStartTime) /
								float(magicFactor42);  // linear
						}
#ifdef REND2_SP_MAYBE
						if (curTime < kcur->second.mGoreGrowEndTime)
						{
							newSurf2->scale = Q_max(
								1.0f,
								1.0f /
									((curTime - kcur->second.mGoreGrowStartTime) *
									kcur->second.mGoreGrowFactor +
									kcur->second.mGoreGrowOffset));
						}
#endif
						shader_t *gshader;
						if (kcur->second.shader)
						{
 							gshader = R_GetShaderByHandle(kcur->second.shader);
						}
						else
						{
							gshader = R_GetShaderByHandle(goreShader);
						}
#ifdef REND2_SP_MAYBE
						// Set fade on surf.
						// Only if we have a fade time set, and let us fade on
						// rgb if we want -rww
						if (kcur->second.mDeleteTime && kcur->second.mFadeTime)
						{
							if ( (kcur->second.mDeleteTime - curTime) < kcur->second.mFadeTime )
							{
								newSurf2->fade =
									(float)(kcur->second.mDeleteTime - curTime) /
									kcur->second.mFadeTime;
								if (kcur->second.mFadeRGB)
								{
									// RGB fades are scaled from 2.0f to 3.0f
									// (simply to differentiate)
									newSurf2->fade = Q_max(2.01f, newSurf2->fade + 2.0f);
								}
							}
						}
#endif
						last->goreChain = newSurf2;
						last = newSurf2;
						R_AddDrawSurf(
							(surfaceType_t *)newSurf2,
							entityNum,
							gshader,
							RS.fogNum,
							qfalse,
							R_IsPostRenderEntity(ent),
							cubemapIndex);
					}
				}
			}
#endif
		}

		// projection shadows work fine with personal models
		if (r_shadows->integer == 3
			&& RS.fogNum == 0
			&& (RS.renderfx & (RF_NOSHADOW | RF_DEPTHHACK))
			&& shader->sort == SS_OPAQUE) {

			CRenderableSurface *newSurf = AllocGhoul2RenderableSurface();
			newSurf->vboMesh = &RS.currentModel->data.glm->vboModels[RS.lod].vboMeshes[RS.surfaceNum];
			assert(newSurf->vboMesh != NULL && RS.surfaceNum == surface->thisSurfaceIndex);
			newSurf->surfaceData = surface;
			newSurf->boneCache = RS.boneCache;
			R_AddDrawSurf((surfaceType_t *)newSurf, entityNum, tr.projectionShadowShader, 0, qfalse, qfalse, 0);
		}
	}

	// if we are turning off all descendants, then stop this recursion now
	if (offFlags & G2SURFACEFLAG_NODESCENDANTS)
	{
		return;
	}

	// now recursively call for the children
	for (i=0; i< surfInfo->numChildren; i++)
	{
		RS.surfaceNum = surfInfo->childIndexes[i];
		RenderSurfaces(RS, ent, entityNum);
	}

#ifdef G2_PERFORMANCE_ANALYSIS
	G2Time_RenderSurfaces += G2PerformanceTimer_RenderSurfaces.End();
#endif
}

// Go through the model and deal with just the surfaces that are tagged as bolt
// on points - this is for the server side skeleton construction
void ProcessModelBoltSurfaces(
	int surfaceNum,
	surfaceInfo_v &rootSList,
	mdxaBone_v &bonePtr,
	model_t *currentModel,
	int lod,
	boltInfo_v &boltList)
{
#ifdef G2_PERFORMANCE_ANALYSIS
	G2PerformanceTimer_ProcessModelBoltSurfaces.Start();
#endif
	int i;
	int offFlags = 0;

	// back track and get the surfinfo struct for this surface
	mdxmSurface_t *surface = (mdxmSurface_t *)
		G2_FindSurface((void *)currentModel, surfaceNum, 0);
	mdxmHierarchyOffsets_t *surfIndexes = (mdxmHierarchyOffsets_t *)
		((byte *)currentModel->data.glm->header + sizeof(mdxmHeader_t));
	mdxmSurfHierarchy_t *surfInfo = (mdxmSurfHierarchy_t *)
		((byte *)surfIndexes + surfIndexes->offsets[surface->thisSurfaceIndex]);

	// see if we have an override surface in the surface list
	surfaceInfo_t *surfOverride = G2_FindOverrideSurface(surfaceNum, rootSList);

	// really, we should use the default flags for this surface unless it's been overriden
	offFlags = surfInfo->flags;

	// set the off flags if we have some
	if (surfOverride)
	{
		offFlags = surfOverride->offFlags;
	}

	// is this surface considered a bolt surface?
	if (surfInfo->flags & G2SURFACEFLAG_ISBOLT)
	{
		// well alrighty then. Lets see if there is a bolt that is attempting to use it
		int boltNum = G2_Find_Bolt_Surface_Num(boltList, surfaceNum, 0);
		// yes - ok, processing time.
		if (boltNum != -1)
		{
			G2_ProcessSurfaceBolt(bonePtr, surface, boltNum, boltList, surfOverride, currentModel);
		}
	}

	// if we are turning off all descendants, then stop this recursion now
	if (offFlags & G2SURFACEFLAG_NODESCENDANTS)
	{
		return;
	}

	// now recursively call for the children
	for (i=0; i< surfInfo->numChildren; i++)
	{
		ProcessModelBoltSurfaces(
			surfInfo->childIndexes[i], rootSList, bonePtr, currentModel, lod, boltList);
	}

#ifdef G2_PERFORMANCE_ANALYSIS
	G2Time_ProcessModelBoltSurfaces += G2PerformanceTimer_ProcessModelBoltSurfaces.End();
#endif
}


// build the used bone list so when doing bone transforms we can determine if we need to do it or not
void G2_ConstructUsedBoneList(CConstructBoneList &CBL)
{
	int	 		i, j;
	int			offFlags = 0;
	mdxmHeader_t *mdxm = CBL.currentModel->data.glm->header;

	// back track and get the surfinfo struct for this surface
	const mdxmSurface_t			*surface = (mdxmSurface_t *)G2_FindSurface((void *)CBL.currentModel, CBL.surfaceNum, 0);
	const mdxmHierarchyOffsets_t	*surfIndexes = (mdxmHierarchyOffsets_t *)((byte *)mdxm + sizeof(mdxmHeader_t));
	const mdxmSurfHierarchy_t	*surfInfo = (mdxmSurfHierarchy_t *)((byte *)surfIndexes + surfIndexes->offsets[surface->thisSurfaceIndex]);
	const model_t				*mod_a = R_GetModelByHandle(mdxm->animIndex);
	mdxaHeader_t				*mdxa = mod_a->data.gla;
	const mdxaSkelOffsets_t		*offsets = (mdxaSkelOffsets_t *)((byte *)mdxa + sizeof(mdxaHeader_t));
	const mdxaSkel_t			*skel, *childSkel;

	// see if we have an override surface in the surface list
	const surfaceInfo_t	*surfOverride = G2_FindOverrideSurface(CBL.surfaceNum, CBL.rootSList);

	// really, we should use the default flags for this surface unless it's been overriden
	offFlags = surfInfo->flags;

	// set the off flags if we have some
	if (surfOverride)
	{
		offFlags = surfOverride->offFlags;
	}

	// if this surface is not off, add it to the shader render list
	if (!(offFlags & G2SURFACEFLAG_OFF))
	{
		int	*bonesReferenced = (int *)((byte*)surface + surface->ofsBoneReferences);
		// now whip through the bones this surface uses
		for (i=0; i<surface->numBoneReferences;i++)
		{
			int iBoneIndex = bonesReferenced[i];
			CBL.boneUsedList[iBoneIndex] = 1;

			// now go and check all the descendant bones attached to this bone and see if any have the always flag on them. If so, activate them
 			skel = (mdxaSkel_t *)((byte *)mdxa + sizeof(mdxaHeader_t) + offsets->offsets[iBoneIndex]);

			// for every child bone...
			for (j=0; j< skel->numChildren; j++)
			{
				// get the skel data struct for each child bone of the referenced bone
 				childSkel = (mdxaSkel_t *)((byte *)mdxa + sizeof(mdxaHeader_t) + offsets->offsets[skel->children[j]]);

				// does it have the always on flag on?
				if (childSkel->flags & G2BONEFLAG_ALWAYSXFORM)
				{
					// yes, make sure it's in the list of bones to be transformed.
					CBL.boneUsedList[skel->children[j]] = 1;
				}
			}

			// now we need to ensure that the parents of this bone are actually active...
			//
			int iParentBone = skel->parent;
			while (iParentBone != -1)
			{
				if (CBL.boneUsedList[iParentBone])	// no need to go higher
					break;
				CBL.boneUsedList[iParentBone] = 1;
				skel = (mdxaSkel_t *)((byte *)mdxa + sizeof(mdxaHeader_t) + offsets->offsets[iParentBone]);
				iParentBone = skel->parent;
			}
		}
	}
 	else
	// if we are turning off all descendants, then stop this recursion now
	if (offFlags & G2SURFACEFLAG_NODESCENDANTS)
	{
		return;
	}

	// now recursively call for the children
	for (i=0; i< surfInfo->numChildren; i++)
	{
		CBL.surfaceNum = surfInfo->childIndexes[i];
		G2_ConstructUsedBoneList(CBL);
	}
}


// sort all the ghoul models in this list so if they go in reference order.
// This will ensure the bolt on's are attached to the right place on the
// previous model, since it ensures the model being attached to is built and
// rendered first.
// NOTE!! This assumes at least one model will NOT have a parent. If it does -
// we are screwed
static void G2_Sort_Models(
	CGhoul2Info_v& ghoul2,
	int *const modelList,
	int modelListCapacity,
	int *const modelCount )
{
	*modelCount = 0;

	if ( modelListCapacity < ghoul2.size() )
	{
		return;
	}

	// first walk all the possible ghoul2 models, and stuff the out array with
	// those with no parents
	for ( int i = 0, numModels = ghoul2.size(); i < numModels; ++i )
	{
		CGhoul2Info& g2Info = ghoul2[i];

		// have a ghoul model here?
		if ( g2Info.mModelindex == -1 )
		{
			continue;
		}

		if ( !g2Info.mValid )
		{
			continue;
		}

		// are we attached to anything?
		if ( g2Info.mModelBoltLink == -1 )
		{
			// no, insert us first
			modelList[(*modelCount)++] = i;
	 	}
	}

	int startPoint = 0;
	int endPoint = *modelCount;

	// now, using that list of parentless models, walk the descendant tree for
	// each of them, inserting the descendents in the list
	while ( startPoint != endPoint )
	{
		for ( int i = 0, numModels = ghoul2.size(); i < numModels; ++i )
		{
			CGhoul2Info& g2Info = ghoul2[i];

			// have a ghoul model here?
			if ( g2Info.mModelindex == -1 )
			{
				continue;
			}

			if ( !g2Info.mValid )
			{
				continue;
			}

			// what does this model think it's attached to?
			if ( g2Info.mModelBoltLink == -1 )
			{
				continue;
			}

			int boltTo = (g2Info.mModelBoltLink >> MODEL_SHIFT) & MODEL_AND;

			// is it any of the models we just added to the list?
			for ( int j = startPoint; j < endPoint; ++j )
			{
				// is this my parent model?
				if ( boltTo == modelList[j] )
				{
					// yes, insert into list and exit now
					modelList[(*modelCount)++] = i;
					break;
				}
			}
		}

		// update start and end points
		startPoint = endPoint;
		endPoint = *modelCount;
	}
}

void *G2_FindSurface_BC(const model_s *mod, int index, int lod)
{
	mdxmHeader_t *mdxm = mod->data.glm->header;
	assert(mod);
	assert(mdxm);

	// point at first lod list
	byte	*current = (byte*)((intptr_t)mdxm + (intptr_t)mdxm->ofsLODs);
	int i;

	//walk the lods
	assert(lod>=0&&lod<mdxm->numLODs);
	for (i=0; i<lod; i++)
	{
		mdxmLOD_t *lodData = (mdxmLOD_t *)current;
		current += lodData->ofsEnd;
	}

	// avoid the lod pointer data structure
	current += sizeof(mdxmLOD_t);

	mdxmLODSurfOffset_t *indexes = (mdxmLODSurfOffset_t *)current;
	// we are now looking at the offset array
	assert(index>=0&&index<mdxm->numSurfaces);
	current += indexes->offsets[index];

	return (void *)current;
}

// We've come across a surface that's designated as a bolt surface, process it and put it in the appropriate bolt place
void G2_ProcessSurfaceBolt2(
	CBoneCache &boneCache,
	const mdxmSurface_t *surface,
	int boltNum,
	boltInfo_v &boltList,
	const surfaceInfo_t *surfInfo,
	const model_t *mod,
	mdxaBone_t &retMatrix)
{
 	float pTri[3][3];

	// now there are two types of tag surface - model ones and procedural
	// generated types - lets decide which one we have here.
	if (surfInfo && surfInfo->offFlags == G2SURFACEFLAG_GENERATED)
	{
		const int surfNumber = surfInfo->genPolySurfaceIndex & 0x0ffff;
		const int polyNumber = (surfInfo->genPolySurfaceIndex >> 16) & 0x0ffff;

		// find original surface our original poly was in.
		const mdxmSurface_t *originalSurf =
			(mdxmSurface_t *)G2_FindSurface_BC(mod, surfNumber, surfInfo->genLod);
		const mdxmTriangle_t *originalTriangleIndexes =
			(mdxmTriangle_t *)((byte*)originalSurf + originalSurf->ofsTriangles);

		// get the original polys indexes
		const int index0 = originalTriangleIndexes[polyNumber].indexes[0];
		const int index1 = originalTriangleIndexes[polyNumber].indexes[1];
		const int index2 = originalTriangleIndexes[polyNumber].indexes[2];

		// decide where the original verts are
		const mdxmVertex_t *surfVerts =
 			(mdxmVertex_t *)((byte *)originalSurf + originalSurf->ofsVerts);
		const mdxmVertex_t *verts[3] = {
 			surfVerts + index0,
			surfVerts + index1,
			surfVerts + index2
		};

		// clear out the triangle verts to be
 	   	VectorClear(pTri[0]);
 	   	VectorClear(pTri[1]);
 	   	VectorClear(pTri[2]);
		const int *piBoneReferences =
			(int *)((byte*)originalSurf + originalSurf->ofsBoneReferences);

		for ( int i = 0; i < 3; ++i )
		{
			// now go and transform just the points we need from the surface that
			// was hit originally
			const int iNumWeights = G2_GetVertWeights(verts[i]);
			float fTotalWeight = 0.0f;
			for ( int k = 0 ; k < iNumWeights ; k++ )
			{
				const int iBoneIndex = G2_GetVertBoneIndex(verts[i], k);
				const float fBoneWeight = G2_GetVertBoneWeight(
					verts[i], k, fTotalWeight, iNumWeights);
				const mdxaBone_t &bone = boneCache.Eval(piBoneReferences[iBoneIndex]);

				pTri[i][0] += fBoneWeight *
					(DotProduct(bone.matrix[0], verts[i]->vertCoords) + bone.matrix[0][3]);
				pTri[i][1] += fBoneWeight *
					(DotProduct(bone.matrix[1], verts[i]->vertCoords) + bone.matrix[1][3]);
				pTri[i][2] += fBoneWeight *
					(DotProduct(bone.matrix[2], verts[i]->vertCoords) + bone.matrix[2][3]);
			}
		}

		const float baryCentricK =
			1.0f - (surfInfo->genBarycentricI + surfInfo->genBarycentricJ);

		// now we have the model transformed into model space, now generate an origin.
		for ( int i = 0; i < 3; ++i )
		{
			retMatrix.matrix[i][3] =
				(pTri[0][i] * surfInfo->genBarycentricI) +
				(pTri[1][i] * surfInfo->genBarycentricJ) +
				(pTri[2][i] * baryCentricK);
		}

		// generate a normal to this new triangle
		vec3_t vec0, vec1;
		VectorSubtract(pTri[0], pTri[1], vec0);
		VectorSubtract(pTri[2], pTri[1], vec1);

   		vec3_t normal;
		CrossProduct(vec0, vec1, normal);
		VectorNormalize(normal);

		// forward vector
		retMatrix.matrix[0][0] = normal[0];
		retMatrix.matrix[1][0] = normal[1];
		retMatrix.matrix[2][0] = normal[2];

		// up will be towards point 0 of the original triangle.
		// so lets work it out. Vector is hit point - point 0
		vec3_t up;
		up[0] = retMatrix.matrix[0][3] - pTri[0][0];
		up[1] = retMatrix.matrix[1][3] - pTri[0][1];
		up[2] = retMatrix.matrix[2][3] - pTri[0][2];

		// normalise it
		VectorNormalize(up);

		// that's the up vector
		retMatrix.matrix[0][1] = up[0];
		retMatrix.matrix[1][1] = up[1];
		retMatrix.matrix[2][1] = up[2];

		// right is always straight
		vec3_t right;
		CrossProduct( normal, up, right );

		// that's the up vector
		retMatrix.matrix[0][2] = right[0];
		retMatrix.matrix[1][2] = right[1];
		retMatrix.matrix[2][2] = right[2];

	}
	// no, we are looking at a normal model tag
	else
	{
	 	// whip through and actually transform each vertex
		const mdxmVertex_t *v =
			(const mdxmVertex_t *)((byte *)surface + surface->ofsVerts);
		const int *piBoneReferences =
			(const int*)((byte *)surface + surface->ofsBoneReferences);
 		for ( int j = 0; j < 3; j++ )
 		{
 			VectorClear(pTri[j]);

			const int iNumWeights = G2_GetVertWeights( v );
			float fTotalWeight = 0.0f;
 			for ( int k = 0 ; k < iNumWeights ; k++)
 			{
				const int iBoneIndex = G2_GetVertBoneIndex(v, k);
				const float fBoneWeight =
					G2_GetVertBoneWeight(v, k, fTotalWeight, iNumWeights);
				const mdxaBone_t &bone = boneCache.Eval(piBoneReferences[iBoneIndex]);

 				pTri[j][0] += fBoneWeight *
					(DotProduct(bone.matrix[0], v->vertCoords) + bone.matrix[0][3]);
 				pTri[j][1] += fBoneWeight *
					(DotProduct(bone.matrix[1], v->vertCoords) + bone.matrix[1][3]);
 				pTri[j][2] += fBoneWeight *
					(DotProduct(bone.matrix[2], v->vertCoords) + bone.matrix[2][3]);
 			}

 			v++;
 		}

 		// clear out used arrays
		vec3_t axes[3] = {};
		vec3_t sides[3] = {};

 		// work out actual sides of the tag triangle
 		for ( int j = 0; j < 3; j++ )
 		{
 			sides[j][0] = pTri[(j + 1) % 3][0] - pTri[j][0];
 			sides[j][1] = pTri[(j + 1) % 3][1] - pTri[j][1];
 			sides[j][2] = pTri[(j + 1) % 3][2] - pTri[j][2];
 		}

		// do math trig to work out what the matrix will be from this
		// triangle's translated position
 		VectorNormalize2(sides[iG2_TRISIDE_LONGEST], axes[0]);
 		VectorNormalize2(sides[iG2_TRISIDE_SHORTEST], axes[1]);

		// project shortest side so that it is exactly 90 degrees to the longer
		// side
 		float d = DotProduct(axes[0], axes[1]);
 		VectorMA(axes[0], -d, axes[1], axes[0]);
 		VectorNormalize2(axes[0], axes[0]);

 		CrossProduct(sides[iG2_TRISIDE_LONGEST], sides[iG2_TRISIDE_SHORTEST], axes[2]);
 		VectorNormalize2(axes[2], axes[2]);

		// set up location in world space of the origin point in out going
		// matrix
 		retMatrix.matrix[0][3] = pTri[MDX_TAG_ORIGIN][0];
 		retMatrix.matrix[1][3] = pTri[MDX_TAG_ORIGIN][1];
 		retMatrix.matrix[2][3] = pTri[MDX_TAG_ORIGIN][2];

		// copy axis to matrix - do some magic to orient minus Y to positive X
		// and so on so bolt on stuff is oriented correctly
		retMatrix.matrix[0][0] = axes[1][0];
		retMatrix.matrix[0][1] = axes[0][0];
		retMatrix.matrix[0][2] = -axes[2][0];

		retMatrix.matrix[1][0] = axes[1][1];
		retMatrix.matrix[1][1] = axes[0][1];
		retMatrix.matrix[1][2] = -axes[2][1];

		retMatrix.matrix[2][0] = axes[1][2];
		retMatrix.matrix[2][1] = axes[0][2];
		retMatrix.matrix[2][2] = -axes[2][2];
	}
}

void G2_GetBoltMatrixLow(
	CGhoul2Info& ghoul2,
	int boltNum,
	const vec3_t scale,
	mdxaBone_t& retMatrix)
{
	if ( !ghoul2.mBoneCache )
	{
		retMatrix = identityMatrix;
		return;
	}

	CBoneCache& boneCache = *ghoul2.mBoneCache;
	assert(boneCache.mod);

	boltInfo_v& boltList = ghoul2.mBltlist;
	if ( boltList.empty() || boltNum >= boltList.size() )
	{
		retMatrix = identityMatrix;
		return;
	}

	boltInfo_t& bolt = boltList[boltNum];
	int boltBone = bolt.boneNumber;
	int boltSurface = bolt.surfaceNumber;

	if ( boltBone >= 0 )
	{
		mdxaSkelOffsets_t *offsets =
			(mdxaSkelOffsets_t *)((byte *)boneCache.header + sizeof(mdxaHeader_t));
		mdxaSkel_t *skel =
			(mdxaSkel_t *)((byte *)offsets + offsets->offsets[boltBone]);

		Mat3x4_Multiply(
			&retMatrix,
			(mdxaBone_t *)&boneCache.EvalUnsmooth(boltBone),
			&skel->BasePoseMat);
	}
	else if ( boltSurface >= 0 )
	{
		const surfaceInfo_t *surfInfo = nullptr;
		for ( const surfaceInfo_t& t : ghoul2.mSlist )
		{
			if ( t.surface == boltSurface )
			{
				surfInfo = &t;
			}
		}

		mdxmSurface_t *surface = nullptr;
		if ( !surfInfo )
		{
			surface = (mdxmSurface_t *)G2_FindSurface_BC(
				boneCache.mod, boltSurface, 0);
		}

		if ( !surface && surfInfo && surfInfo->surface < 10000 )
		{
			surface = (mdxmSurface_t *)G2_FindSurface_BC(
				boneCache.mod, surfInfo->surface, 0);
		}

		G2_ProcessSurfaceBolt2(
			boneCache,
			surface,
			boltNum,
			boltList,
			surfInfo,
			(model_t *)boneCache.mod,
			retMatrix);
	}
	else
	{
		 // we have a bolt without a bone or surface, not a huge problem but we
		 // ought to at least clear the bolt matrix
		retMatrix = identityMatrix;
	}
}

static void RootMatrix(
	CGhoul2Info_v &ghoul2,
	int time,
	const vec3_t scale,
	mdxaBone_t &retMatrix)
{
	for ( int i = 0, numModels = ghoul2.size(); i < numModels; ++i )
	{
		if ( ghoul2[i].mModelindex == -1 || !ghoul2[i].mValid )
		{
			continue;
		}

		if ( ghoul2[i].mFlags & GHOUL2_NEWORIGIN )
		{
			mdxaBone_t bolt;
			mdxaBone_t tempMatrix;

			G2_ConstructGhoulSkeleton(ghoul2, time, false, scale);
			G2_GetBoltMatrixLow(ghoul2[i], ghoul2[i].mNewOrigin, scale, bolt);

			tempMatrix.matrix[0][0] = 1.0f;
			tempMatrix.matrix[0][1] = 0.0f;
			tempMatrix.matrix[0][2] = 0.0f;
			tempMatrix.matrix[0][3] = -bolt.matrix[0][3];

			tempMatrix.matrix[1][0] = 0.0f;
			tempMatrix.matrix[1][1] = 1.0f;
			tempMatrix.matrix[1][2] = 0.0f;
			tempMatrix.matrix[1][3] = -bolt.matrix[1][3];

			tempMatrix.matrix[2][0] = 0.0f;
			tempMatrix.matrix[2][1] = 0.0f;
			tempMatrix.matrix[2][2] = 1.0f;
			tempMatrix.matrix[2][3] = -bolt.matrix[2][3];

			retMatrix = tempMatrix * identityMatrix;
			return;
		}
	}

	retMatrix = identityMatrix;
}

/*
==============
R_AddGHOULSurfaces
==============
*/

void R_AddGhoulSurfaces( trRefEntity_t *ent, int entityNum )
{
#ifdef G2_PERFORMANCE_ANALYSIS
	G2PerformanceTimer_R_AddGHOULSurfaces.Start();
#endif

	CGhoul2Info_v &ghoul2 = *((CGhoul2Info_v *)ent->e.ghoul2);

	if ( !ghoul2.IsValid() )
	{
		return;
	}

	// if we don't want server ghoul2 models and this is one, or we just don't
	// want ghoul2 models at all, then return
	if (r_noServerGhoul2->integer)
	{
		return;
	}

	if (!G2_SetupModelPointers(ghoul2))
	{
		return;
	}

	int currentTime = G2API_GetTime(tr.refdef.time);

	// cull the entire model if merged bounding box of both frames is outside
	// the view frustum.
	int cull = R_GCullModel(ent);
	if ( cull == CULL_OUT )
	{
		return;
	}

	HackadelicOnClient = true;

   	// don't add third_person objects if not in a portal
	qboolean personalModel = (qboolean)(
		(ent->e.renderfx & RF_THIRD_PERSON) &&
		!(tr.viewParms.isPortal ||
			(tr.viewParms.flags & VPF_DEPTHSHADOW)));

	int modelList[256];
	assert(ghoul2.size() < ARRAY_LEN(modelList));
	modelList[255] = 548;

	// see if we are in a fog volume
	int fogNum = R_GComputeFogNum(ent);

	// order sort the ghoul 2 models so bolt ons get bolted to the right model
	int modelCount;
	G2_Sort_Models(ghoul2, modelList, ARRAY_LEN(modelList), &modelCount);
	assert(modelList[255] == 548);

#ifdef _G2_GORE
	if ( goreShader == -1 )
	{
		goreShader = RE_RegisterShader("gfx/damage/burnmark1");
	}
#endif

	// walk each possible model for this entity and try rendering it out
	for (int j = 0; j < modelCount; ++j )
	{
		CGhoul2Info& g2Info = ghoul2[modelList[j]];

		if ( !g2Info.mValid )
		{
			continue;
		}

		if ( (g2Info.mFlags & (GHOUL2_NOMODEL | GHOUL2_NORENDER)) != 0 )
		{
			continue;
		}

		// figure out whether we should be using a custom shader for this model
		skin_t *skin = nullptr;
		shader_t *cust_shader = nullptr;

		if (ent->e.customShader)
		{
			cust_shader = R_GetShaderByHandle(ent->e.customShader );
		}
		else
		{
			cust_shader = nullptr;
			// figure out the custom skin thing
			if (g2Info.mCustomSkin)
			{
				skin = R_GetSkinByHandle(g2Info.mCustomSkin );
			}
			else if (ent->e.customSkin)
			{
				skin = R_GetSkinByHandle(ent->e.customSkin );
			}
			else if ( g2Info.mSkin > 0 && g2Info.mSkin < tr.numSkins )
			{
				skin = R_GetSkinByHandle( g2Info.mSkin );
			}
		}

		int whichLod = G2_ComputeLOD( ent, g2Info.currentModel, g2Info.mLodBias );
		G2_FindOverrideSurface(-1, g2Info.mSlist); //reset the quick surface override lookup;

#ifdef _G2_GORE
		CGoreSet *gore = nullptr;
		if ( g2Info.mGoreSetTag )
		{
			gore = FindGoreSet(g2Info.mGoreSetTag);
			if ( !gore ) // my gore is gone, so remove it
			{
				g2Info.mGoreSetTag = 0;
			}
		}

		CRenderSurface RS(g2Info.mSurfaceRoot,
			g2Info.mSlist,
			cust_shader,
			fogNum,
			personalModel,
			g2Info.mBoneCache,
			ent->e.renderfx,
			skin,
			(model_t *)g2Info.currentModel,
			whichLod,
			g2Info.mBltlist,
			nullptr,
			gore);
#else
		CRenderSurface RS(g2Info.mSurfaceRoot,
			g2Info.mSlist,
			cust_shader,
			fogNum,
			personalModel,
			g2Info.mBoneCache,
			ent->e.renderfx,
			skin,
			(model_t *)g2Info.currentModel,
			whichLod,
			g2Info.mBltlist);
#endif
		if ( !personalModel && (RS.renderfx & RF_SHADOW_PLANE) )
		{
			RS.renderfx |= RF_NOSHADOW;
		}

		RenderSurfaces(RS, ent, entityNum);
	}

	HackadelicOnClient = false;

#ifdef G2_PERFORMANCE_ANALYSIS
	G2Time_R_AddGHOULSurfaces += G2PerformanceTimer_R_AddGHOULSurfaces.End();
#endif
}

#ifdef _G2_LISTEN_SERVER_OPT
qboolean G2API_OverrideServerWithClientData(CGhoul2Info *serverInstance);
#endif

bool G2_NeedsRecalc(CGhoul2Info *ghlInfo,int frameNum)
{
	G2_SetupModelPointers(ghlInfo);
	// not sure if I still need this test, probably
	if (ghlInfo->mSkelFrameNum!=frameNum||
		!ghlInfo->mBoneCache||
		ghlInfo->mBoneCache->mod!=ghlInfo->currentModel)
	{
#ifdef _G2_LISTEN_SERVER_OPT
		if (ghlInfo->entityNum != ENTITYNUM_NONE &&
			G2API_OverrideServerWithClientData(ghlInfo))
		{ //if we can manage this, then we don't have to reconstruct
			return false;
		}
#endif
		ghlInfo->mSkelFrameNum=frameNum;
		return true;
	}
	return false;
}

/*
==============
G2_ConstructGhoulSkeleton
builds a complete skeleton for all ghoul models in a CGhoul2Info_v class
using LOD 0
==============
*/
void G2_ConstructGhoulSkeleton(
	CGhoul2Info_v &ghoul2,
	const int frameNum,
	bool checkForNewOrigin,
	const vec3_t scale)
{
#ifdef G2_PERFORMANCE_ANALYSIS
	G2PerformanceTimer_G2_ConstructGhoulSkeleton.Start();
#endif
	int modelCount;
	mdxaBone_t rootMatrix;
	int modelList[256];

	assert(ghoul2.size() <= ARRAY_LEN(modelList));
	modelList[255] = 548;

	if ( checkForNewOrigin )
	{
		RootMatrix(ghoul2, frameNum, scale, rootMatrix);
	}
	else
	{
		rootMatrix = identityMatrix;
	}

	G2_Sort_Models(ghoul2, modelList, ARRAY_LEN(modelList), &modelCount);
	assert(modelList[255] == 548);

	for ( int j = 0; j < modelCount; j++ )
	{
		int i = modelList[j];
		CGhoul2Info& g2Info = ghoul2[i];

		if ( !g2Info.mValid )
		{
			continue;
		}

		if ( j && g2Info.mModelBoltLink != -1 )
		{
			int	boltMod = (g2Info.mModelBoltLink >> MODEL_SHIFT) & MODEL_AND;
			int	boltNum = (g2Info.mModelBoltLink >> BOLT_SHIFT) & BOLT_AND;

			mdxaBone_t bolt;
			G2_GetBoltMatrixLow(ghoul2[boltMod], boltNum, scale, bolt);
			G2_TransformGhoulBones(
				g2Info.mBlist,
				bolt,
				g2Info,
				frameNum,
				checkForNewOrigin);
		}
		else
#ifdef _G2_LISTEN_SERVER_OPT
		if (g2Info.entityNum == ENTITYNUM_NONE || g2Info.mSkelFrameNum != frameNum)
#endif
		{
			G2_TransformGhoulBones(
				g2Info.mBlist,
				rootMatrix,
				g2Info,
				frameNum,
				checkForNewOrigin);
		}
	}

#ifdef G2_PERFORMANCE_ANALYSIS
	G2Time_G2_ConstructGhoulSkeleton += G2PerformanceTimer_G2_ConstructGhoulSkeleton.End();
#endif
}

static inline float G2_GetVertBoneWeightNotSlow( const mdxmVertex_t *pVert, const int iWeightNum)
{
	float fBoneWeight;

	int iTemp = pVert->BoneWeightings[iWeightNum];

	iTemp|= (pVert->uiNmWeightsAndBoneIndexes >> (iG2_BONEWEIGHT_TOPBITS_SHIFT+(iWeightNum*2)) ) & iG2_BONEWEIGHT_TOPBITS_AND;

	fBoneWeight = fG2_BONEWEIGHT_RECIPROCAL_MULT * iTemp;

	return fBoneWeight;
}

void RB_TransformBones(const trRefEntity_t *ent, const trRefdef_t *refdef, int currentFrameNum, gpuFrame_t *frame)
{
	if (!ent->e.ghoul2 || !G2API_HaveWeGhoul2Models(*((CGhoul2Info_v *)ent->e.ghoul2)))
		return;

	CGhoul2Info_v &ghoul2 = *((CGhoul2Info_v *)ent->e.ghoul2);

	if (!ghoul2.IsValid())
		return;

	// if we don't want server ghoul2 models and this is one, or we just don't
	// want ghoul2 models at all, then return
	if (r_noServerGhoul2->integer)
	{
		return;
	}

	if (!G2_SetupModelPointers(ghoul2))
	{
		return;
	}

	int currentTime = G2API_GetTime(refdef->time);

	HackadelicOnClient = true;

	mdxaBone_t rootMatrix;
	RootMatrix(ghoul2, currentTime, ent->e.modelScale, rootMatrix);

	int modelList[256];
	assert(ghoul2.size() < ARRAY_LEN(modelList));
	modelList[255] = 548;

	// order sort the ghoul 2 models so bolt ons get bolted to the right model
	int modelCount;
	G2_Sort_Models(ghoul2, modelList, ARRAY_LEN(modelList), &modelCount);
	assert(modelList[255] == 548);

	// construct a world matrix for this entity
	G2_GenerateWorldMatrix(ent->e.angles, ent->e.origin);

	// walk each possible model for this entity and try transforming all bones
	for (int j = 0; j < modelCount; ++j)
	{
		CGhoul2Info& g2Info = ghoul2[modelList[j]];

		if (!g2Info.mValid)
		{
			continue;
		}

		if ((g2Info.mFlags & (GHOUL2_NOMODEL | GHOUL2_NORENDER)) != 0)
		{
			continue;
		}

		if (j && g2Info.mModelBoltLink != -1)
		{
			int	boltMod = (g2Info.mModelBoltLink >> MODEL_SHIFT) & MODEL_AND;
			int	boltNum = (g2Info.mModelBoltLink >> BOLT_SHIFT) & BOLT_AND;

			mdxaBone_t bolt;
			G2_GetBoltMatrixLow(ghoul2[boltMod], boltNum, ent->e.modelScale, bolt);
			G2_TransformGhoulBones(g2Info.mBlist, bolt, g2Info, currentTime);
		}
		else
		{
			G2_TransformGhoulBones(g2Info.mBlist, rootMatrix, g2Info, currentTime);
		}

		CBoneCache *bc = g2Info.mBoneCache;
		if (bc->uboGPUFrame == currentFrameNum)
			return;

		for (int bone = 0; bone < (int)bc->mBones.size(); bone++)
		{
			const mdxaBone_t& b = bc->EvalRender(bone);
			Com_Memcpy(
				bc->boneMatrices + bone,
				&b.matrix[0][0],
				sizeof(mat3x4_t));
		}
		bc->uboOffset = -1;

		SkeletonBoneMatricesBlock bonesBlock = {};
		Com_Memcpy(
			bonesBlock.matrices,
			bc->boneMatrices,
			sizeof(mat3x4_t) * bc->mBones.size());

		int uboOffset = RB_AppendConstantsData(
			frame, &bonesBlock, sizeof(mat3x4_t) * bc->mBones.size());

		bc->uboOffset = uboOffset;
		bc->uboGPUFrame = currentFrameNum;
	}
}

int RB_GetBoneUboOffset(CRenderableSurface *surf)
{
	if (surf->boneCache)
		return surf->boneCache->uboOffset;
	else
		return -1;
}

void RB_SetBoneUboOffset(CRenderableSurface *surf, int offset, int currentFrameNum)
{
	surf->boneCache->uboOffset = offset;
	surf->boneCache->uboGPUFrame = currentFrameNum;
}

void RB_FillBoneBlock(CRenderableSurface *surf, mat3x4_t *outMatrices)
{
	Com_Memcpy(
		outMatrices,
		surf->boneCache->boneMatrices,
		sizeof(surf->boneCache->boneMatrices));
}

void RB_SurfaceGhoul( CRenderableSurface *surf )
{
	mdxmVBOMesh_t *surface = surf->vboMesh;

	if ( surface->vbo == NULL || surface->ibo == NULL )
	{
		return;
	}

	int numIndexes = surface->numIndexes;
	int numVertexes = surface->numVertexes;
	int minIndex = surface->minIndex;
	int maxIndex = surface->maxIndex;
	int indexOffset = surface->indexOffset;

#ifdef _G2_GORE
	if (surf->alternateTex)
	{
		R_BindVBO(tr.goreVBO);
		R_BindIBO(tr.goreIBO);
		tess.externalIBO = tr.goreIBO;

		numIndexes = surf->alternateTex->numIndexes;
		numVertexes = surf->alternateTex->numVerts;
		minIndex = surf->alternateTex->firstVert;
		maxIndex = surf->alternateTex->firstVert + surf->alternateTex->numVerts;
		indexOffset = surf->alternateTex->firstIndex;

#ifdef REND2_SP_MAYBE
		// UNTESTED CODE
		if (surf->scale > 1.0f)
		{
			tess.scale = true;
			tess.texCoords[tess.firstIndex][0][0] = surf->scale;
		}

		//now check for fade overrides -rww
		if (surf->fade)
		{
			static int lFade;
			if (surf->fade < 1.0)
			{
				tess.fade = true;
				lFade = Q_ftol(254.4f*surf->fade);
				tess.svars.colors[tess.firstIndex][0] =
				tess.svars.colors[tess.firstIndex][1] =
				tess.svars.colors[tess.firstIndex][2] = Q_ftol(1.0f);
				tess.svars.colors[tess.firstIndex][3] = lFade;
			}
			else if (surf->fade > 2.0f && surf->fade < 3.0f)
			{ //hack to fade out on RGB if desired (don't want to add more to CRenderableSurface) -rww
				tess.fade = true;
				lFade = Q_ftol(254.4f*(surf->fade - 2.0f));
				if (lFade < tess.svars.colors[tess.firstIndex][0])
				{ //don't set it unless the fade is less than the current r value (to avoid brightening suddenly before we start fading)
					tess.svars.colors[tess.firstIndex][0] =
					tess.svars.colors[tess.firstIndex][1] =
					tess.svars.colors[tess.firstIndex][2] = lFade;
				}
				tess.svars.colors[tess.firstIndex][3] = lFade;
			}
		}
		tess.scale = false;
		tess.fade = false;
#endif
	} else {
#endif

		R_BindVBO(surface->vbo);
		R_BindIBO(surface->ibo);
		tess.externalIBO = surface->ibo;

		glState.genShadows = surf->genShadows;
#ifdef _G2_GORE
	}
#endif
	int i, mergeForward, mergeBack;
	GLvoid *firstIndexOffset, *lastIndexOffset;

	// merge this into any existing multidraw primitives
	mergeForward = -1;
	mergeBack = -1;
	firstIndexOffset = BUFFER_OFFSET(indexOffset * sizeof(glIndex_t));
	lastIndexOffset = BUFFER_OFFSET((indexOffset + numIndexes) * sizeof(glIndex_t));

	if (r_mergeMultidraws->integer)
	{
		i = 0;

		if (r_mergeMultidraws->integer == 1)
		{
			// lazy merge, only check the last primitive
			if (tess.multiDrawPrimitives)
			{
				i = tess.multiDrawPrimitives - 1;
			}
		}

		for (; i < tess.multiDrawPrimitives; i++)
		{
			if (tess.multiDrawLastIndex[i] == firstIndexOffset)
			{
				mergeBack = i;
			}

			if (lastIndexOffset == tess.multiDrawFirstIndex[i])
			{
				mergeForward = i;
			}
		}
	}

	if (mergeBack != -1 && mergeForward == -1)
	{
		tess.multiDrawNumIndexes[mergeBack] += numIndexes;
		tess.multiDrawLastIndex[mergeBack] = tess.multiDrawFirstIndex[mergeBack] + tess.multiDrawNumIndexes[mergeBack];
		tess.multiDrawMinIndex[mergeBack] = MIN(tess.multiDrawMinIndex[mergeBack], minIndex);
		tess.multiDrawMaxIndex[mergeBack] = MAX(tess.multiDrawMaxIndex[mergeBack], maxIndex);
		backEnd.pc.c_multidrawsMerged++;
	}
	else if (mergeBack == -1 && mergeForward != -1)
	{
		tess.multiDrawNumIndexes[mergeForward] += numIndexes;
		tess.multiDrawFirstIndex[mergeForward] = (glIndex_t *)firstIndexOffset;
		tess.multiDrawLastIndex[mergeForward] = tess.multiDrawFirstIndex[mergeForward] + tess.multiDrawNumIndexes[mergeForward];
		tess.multiDrawMinIndex[mergeForward] = MIN(tess.multiDrawMinIndex[mergeForward], minIndex);
		tess.multiDrawMaxIndex[mergeForward] = MAX(tess.multiDrawMaxIndex[mergeForward], maxIndex);
		backEnd.pc.c_multidrawsMerged++;
	}
	else if (mergeBack != -1 && mergeForward != -1)
	{
		tess.multiDrawNumIndexes[mergeBack] += numIndexes + tess.multiDrawNumIndexes[mergeForward];
		tess.multiDrawLastIndex[mergeBack] = tess.multiDrawFirstIndex[mergeBack] + tess.multiDrawNumIndexes[mergeBack];
		tess.multiDrawMinIndex[mergeBack] = MIN(tess.multiDrawMinIndex[mergeBack], MIN(tess.multiDrawMinIndex[mergeForward], minIndex));
		tess.multiDrawMaxIndex[mergeBack] = MAX(tess.multiDrawMaxIndex[mergeBack], MAX(tess.multiDrawMaxIndex[mergeForward], maxIndex));
		tess.multiDrawPrimitives--;

		if (mergeForward != tess.multiDrawPrimitives)
		{
			tess.multiDrawNumIndexes[mergeForward] = tess.multiDrawNumIndexes[tess.multiDrawPrimitives];
			tess.multiDrawFirstIndex[mergeForward] = tess.multiDrawFirstIndex[tess.multiDrawPrimitives];
		}
		backEnd.pc.c_multidrawsMerged += 2;
	}
	else if (mergeBack == -1 && mergeForward == -1)
	{
		tess.multiDrawNumIndexes[tess.multiDrawPrimitives] = numIndexes;
		tess.multiDrawFirstIndex[tess.multiDrawPrimitives] = (glIndex_t *)firstIndexOffset;
		tess.multiDrawLastIndex[tess.multiDrawPrimitives] = (glIndex_t *)lastIndexOffset;
		tess.multiDrawMinIndex[tess.multiDrawPrimitives] = minIndex;
		tess.multiDrawMaxIndex[tess.multiDrawPrimitives] = maxIndex;
		tess.multiDrawPrimitives++;
	}

	backEnd.pc.c_multidraws++;

	tess.numIndexes += numIndexes;
	tess.numVertexes += numVertexes;
	tess.useInternalVBO = qfalse;
	tess.dlightBits |= surf->dlightBits;

	glState.skeletalAnimation = qtrue;
}

/*
=================
R_LoadMDXM - load a Ghoul 2 Mesh file
=================
*/

/*

Some information used in the creation of the JK2 - JKA bone remap table

These are the old bones:
Complete list of all 72 bones:

*/

int OldToNewRemapTable[72] = {
0,// Bone 0:   "model_root":           Parent: ""  (index -1)
1,// Bone 1:   "pelvis":               Parent: "model_root"  (index 0)
2,// Bone 2:   "Motion":               Parent: "pelvis"  (index 1)
3,// Bone 3:   "lfemurYZ":             Parent: "pelvis"  (index 1)
4,// Bone 4:   "lfemurX":              Parent: "pelvis"  (index 1)
5,// Bone 5:   "ltibia":               Parent: "pelvis"  (index 1)
6,// Bone 6:   "ltalus":               Parent: "pelvis"  (index 1)
6,// Bone 7:   "ltarsal":              Parent: "pelvis"  (index 1)
7,// Bone 8:   "rfemurYZ":             Parent: "pelvis"  (index 1)
8,// Bone 9:   "rfemurX":	            Parent: "pelvis"  (index 1)
9,// Bone10:   "rtibia":	            Parent: "pelvis"  (index 1)
10,// Bone11:   "rtalus":	            Parent: "pelvis"  (index 1)
10,// Bone12:   "rtarsal":              Parent: "pelvis"  (index 1)
11,// Bone13:   "lower_lumbar":         Parent: "pelvis"  (index 1)
12,// Bone14:   "upper_lumbar":	        Parent: "lower_lumbar"  (index 13)
13,// Bone15:   "thoracic":	            Parent: "upper_lumbar"  (index 14)
14,// Bone16:   "cervical":	            Parent: "thoracic"  (index 15)
15,// Bone17:   "cranium":              Parent: "cervical"  (index 16)
16,// Bone18:   "ceyebrow":	            Parent: "face_always_"  (index 71)
17,// Bone19:   "jaw":                  Parent: "face_always_"  (index 71)
18,// Bone20:   "lblip2":	            Parent: "face_always_"  (index 71)
19,// Bone21:   "leye":		            Parent: "face_always_"  (index 71)
20,// Bone22:   "rblip2":	            Parent: "face_always_"  (index 71)
21,// Bone23:   "ltlip2":               Parent: "face_always_"  (index 71)
22,// Bone24:   "rtlip2":	            Parent: "face_always_"  (index 71)
23,// Bone25:   "reye":		            Parent: "face_always_"  (index 71)
24,// Bone26:   "rclavical":            Parent: "thoracic"  (index 15)
25,// Bone27:   "rhumerus":             Parent: "thoracic"  (index 15)
26,// Bone28:   "rhumerusX":            Parent: "thoracic"  (index 15)
27,// Bone29:   "rradius":              Parent: "thoracic"  (index 15)
28,// Bone30:   "rradiusX":             Parent: "thoracic"  (index 15)
29,// Bone31:   "rhand":                Parent: "thoracic"  (index 15)
29,// Bone32:   "mc7":                  Parent: "thoracic"  (index 15)
34,// Bone33:   "r_d5_j1":              Parent: "thoracic"  (index 15)
35,// Bone34:   "r_d5_j2":              Parent: "thoracic"  (index 15)
35,// Bone35:   "r_d5_j3":              Parent: "thoracic"  (index 15)
30,// Bone36:   "r_d1_j1":              Parent: "thoracic"  (index 15)
31,// Bone37:   "r_d1_j2":              Parent: "thoracic"  (index 15)
31,// Bone38:   "r_d1_j3":              Parent: "thoracic"  (index 15)
32,// Bone39:   "r_d2_j1":              Parent: "thoracic"  (index 15)
33,// Bone40:   "r_d2_j2":              Parent: "thoracic"  (index 15)
33,// Bone41:   "r_d2_j3":              Parent: "thoracic"  (index 15)
32,// Bone42:   "r_d3_j1":			    Parent: "thoracic"  (index 15)
33,// Bone43:   "r_d3_j2":		        Parent: "thoracic"  (index 15)
33,// Bone44:   "r_d3_j3":              Parent: "thoracic"  (index 15)
34,// Bone45:   "r_d4_j1":              Parent: "thoracic"  (index 15)
35,// Bone46:   "r_d4_j2":	            Parent: "thoracic"  (index 15)
35,// Bone47:   "r_d4_j3":		        Parent: "thoracic"  (index 15)
36,// Bone48:   "rhang_tag_bone":	    Parent: "thoracic"  (index 15)
37,// Bone49:   "lclavical":            Parent: "thoracic"  (index 15)
38,// Bone50:   "lhumerus":	            Parent: "thoracic"  (index 15)
39,// Bone51:   "lhumerusX":	        Parent: "thoracic"  (index 15)
40,// Bone52:   "lradius":	            Parent: "thoracic"  (index 15)
41,// Bone53:   "lradiusX":	            Parent: "thoracic"  (index 15)
42,// Bone54:   "lhand":	            Parent: "thoracic"  (index 15)
42,// Bone55:   "mc5":		            Parent: "thoracic"  (index 15)
43,// Bone56:   "l_d5_j1":	            Parent: "thoracic"  (index 15)
44,// Bone57:   "l_d5_j2":	            Parent: "thoracic"  (index 15)
44,// Bone58:   "l_d5_j3":	            Parent: "thoracic"  (index 15)
43,// Bone59:   "l_d4_j1":	            Parent: "thoracic"  (index 15)
44,// Bone60:   "l_d4_j2":	            Parent: "thoracic"  (index 15)
44,// Bone61:   "l_d4_j3":	            Parent: "thoracic"  (index 15)
45,// Bone62:   "l_d3_j1":	            Parent: "thoracic"  (index 15)
46,// Bone63:   "l_d3_j2":	            Parent: "thoracic"  (index 15)
46,// Bone64:   "l_d3_j3":	            Parent: "thoracic"  (index 15)
45,// Bone65:   "l_d2_j1":	            Parent: "thoracic"  (index 15)
46,// Bone66:   "l_d2_j2":	            Parent: "thoracic"  (index 15)
46,// Bone67:   "l_d2_j3":	            Parent: "thoracic"  (index 15)
47,// Bone68:   "l_d1_j1":				Parent: "thoracic"  (index 15)
48,// Bone69:   "l_d1_j2":	            Parent: "thoracic"  (index 15)
48,// Bone70:   "l_d1_j3":				Parent: "thoracic"  (index 15)
52// Bone71:   "face_always_":			Parent: "cranium"  (index 17)
};


/*

Bone   0:   "model_root":
            Parent: ""  (index -1)
            #Kids:  1
            Child 0: (index 1), name "pelvis"

Bone   1:   "pelvis":
            Parent: "model_root"  (index 0)
            #Kids:  4
            Child 0: (index 2), name "Motion"
            Child 1: (index 3), name "lfemurYZ"
            Child 2: (index 7), name "rfemurYZ"
            Child 3: (index 11), name "lower_lumbar"

Bone   2:   "Motion":
            Parent: "pelvis"  (index 1)
            #Kids:  0

Bone   3:   "lfemurYZ":
            Parent: "pelvis"  (index 1)
            #Kids:  3
            Child 0: (index 4), name "lfemurX"
            Child 1: (index 5), name "ltibia"
            Child 2: (index 49), name "ltail"

Bone   4:   "lfemurX":
            Parent: "lfemurYZ"  (index 3)
            #Kids:  0

Bone   5:   "ltibia":
            Parent: "lfemurYZ"  (index 3)
            #Kids:  1
            Child 0: (index 6), name "ltalus"

Bone   6:   "ltalus":
            Parent: "ltibia"  (index 5)
            #Kids:  0

Bone   7:   "rfemurYZ":
            Parent: "pelvis"  (index 1)
            #Kids:  3
            Child 0: (index 8), name "rfemurX"
            Child 1: (index 9), name "rtibia"
            Child 2: (index 50), name "rtail"

Bone   8:   "rfemurX":
            Parent: "rfemurYZ"  (index 7)
            #Kids:  0

Bone   9:   "rtibia":
            Parent: "rfemurYZ"  (index 7)
            #Kids:  1
            Child 0: (index 10), name "rtalus"

Bone  10:   "rtalus":
            Parent: "rtibia"  (index 9)
            #Kids:  0

Bone  11:   "lower_lumbar":
            Parent: "pelvis"  (index 1)
            #Kids:  1
            Child 0: (index 12), name "upper_lumbar"

Bone  12:   "upper_lumbar":
            Parent: "lower_lumbar"  (index 11)
            #Kids:  1
            Child 0: (index 13), name "thoracic"

Bone  13:   "thoracic":
            Parent: "upper_lumbar"  (index 12)
            #Kids:  5
            Child 0: (index 14), name "cervical"
            Child 1: (index 24), name "rclavical"
            Child 2: (index 25), name "rhumerus"
            Child 3: (index 37), name "lclavical"
            Child 4: (index 38), name "lhumerus"

Bone  14:   "cervical":
            Parent: "thoracic"  (index 13)
            #Kids:  1
            Child 0: (index 15), name "cranium"

Bone  15:   "cranium":
            Parent: "cervical"  (index 14)
            #Kids:  1
            Child 0: (index 52), name "face_always_"

Bone  16:   "ceyebrow":
            Parent: "face_always_"  (index 52)
            #Kids:  0

Bone  17:   "jaw":
            Parent: "face_always_"  (index 52)
            #Kids:  0

Bone  18:   "lblip2":
            Parent: "face_always_"  (index 52)
            #Kids:  0

Bone  19:   "leye":
            Parent: "face_always_"  (index 52)
            #Kids:  0

Bone  20:   "rblip2":
            Parent: "face_always_"  (index 52)
            #Kids:  0

Bone  21:   "ltlip2":
            Parent: "face_always_"  (index 52)
            #Kids:  0

Bone  22:   "rtlip2":
            Parent: "face_always_"  (index 52)
            #Kids:  0

Bone  23:   "reye":
            Parent: "face_always_"  (index 52)
            #Kids:  0

Bone  24:   "rclavical":
            Parent: "thoracic"  (index 13)
            #Kids:  0

Bone  25:   "rhumerus":
            Parent: "thoracic"  (index 13)
            #Kids:  2
            Child 0: (index 26), name "rhumerusX"
            Child 1: (index 27), name "rradius"

Bone  26:   "rhumerusX":
            Parent: "rhumerus"  (index 25)
            #Kids:  0

Bone  27:   "rradius":
            Parent: "rhumerus"  (index 25)
            #Kids:  9
            Child 0: (index 28), name "rradiusX"
            Child 1: (index 29), name "rhand"
            Child 2: (index 30), name "r_d1_j1"
            Child 3: (index 31), name "r_d1_j2"
            Child 4: (index 32), name "r_d2_j1"
            Child 5: (index 33), name "r_d2_j2"
            Child 6: (index 34), name "r_d4_j1"
            Child 7: (index 35), name "r_d4_j2"
            Child 8: (index 36), name "rhang_tag_bone"

Bone  28:   "rradiusX":
            Parent: "rradius"  (index 27)
            #Kids:  0

Bone  29:   "rhand":
            Parent: "rradius"  (index 27)
            #Kids:  0

Bone  30:   "r_d1_j1":
            Parent: "rradius"  (index 27)
            #Kids:  0

Bone  31:   "r_d1_j2":
            Parent: "rradius"  (index 27)
            #Kids:  0

Bone  32:   "r_d2_j1":
            Parent: "rradius"  (index 27)
            #Kids:  0

Bone  33:   "r_d2_j2":
            Parent: "rradius"  (index 27)
            #Kids:  0

Bone  34:   "r_d4_j1":
            Parent: "rradius"  (index 27)
            #Kids:  0

Bone  35:   "r_d4_j2":
            Parent: "rradius"  (index 27)
            #Kids:  0

Bone  36:   "rhang_tag_bone":
            Parent: "rradius"  (index 27)
            #Kids:  0

Bone  37:   "lclavical":
            Parent: "thoracic"  (index 13)
            #Kids:  0

Bone  38:   "lhumerus":
            Parent: "thoracic"  (index 13)
            #Kids:  2
            Child 0: (index 39), name "lhumerusX"
            Child 1: (index 40), name "lradius"

Bone  39:   "lhumerusX":
            Parent: "lhumerus"  (index 38)
            #Kids:  0

Bone  40:   "lradius":
            Parent: "lhumerus"  (index 38)
            #Kids:  9
            Child 0: (index 41), name "lradiusX"
            Child 1: (index 42), name "lhand"
            Child 2: (index 43), name "l_d4_j1"
            Child 3: (index 44), name "l_d4_j2"
            Child 4: (index 45), name "l_d2_j1"
            Child 5: (index 46), name "l_d2_j2"
            Child 6: (index 47), name "l_d1_j1"
            Child 7: (index 48), name "l_d1_j2"
            Child 8: (index 51), name "lhang_tag_bone"

Bone  41:   "lradiusX":
            Parent: "lradius"  (index 40)
            #Kids:  0

Bone  42:   "lhand":
            Parent: "lradius"  (index 40)
            #Kids:  0

Bone  43:   "l_d4_j1":
            Parent: "lradius"  (index 40)
            #Kids:  0

Bone  44:   "l_d4_j2":
            Parent: "lradius"  (index 40)
            #Kids:  0

Bone  45:   "l_d2_j1":
            Parent: "lradius"  (index 40)
            #Kids:  0

Bone  46:   "l_d2_j2":
            Parent: "lradius"  (index 40)
            #Kids:  0

Bone  47:   "l_d1_j1":
            Parent: "lradius"  (index 40)
            #Kids:  0

Bone  48:   "l_d1_j2":
            Parent: "lradius"  (index 40)
            #Kids:  0

Bone  49:   "ltail":
            Parent: "lfemurYZ"  (index 3)
            #Kids:  0

Bone  50:   "rtail":
            Parent: "rfemurYZ"  (index 7)
            #Kids:  0

Bone  51:   "lhang_tag_bone":
            Parent: "lradius"  (index 40)
            #Kids:  0

Bone  52:   "face_always_":
            Parent: "cranium"  (index 15)
            #Kids:  8
            Child 0: (index 16), name "ceyebrow"
            Child 1: (index 17), name "jaw"
            Child 2: (index 18), name "lblip2"
            Child 3: (index 19), name "leye"
            Child 4: (index 20), name "rblip2"
            Child 5: (index 21), name "ltlip2"
            Child 6: (index 22), name "rtlip2"
            Child 7: (index 23), name "reye"



*/

qboolean R_LoadMDXM(model_t *mod, void *buffer, const char *mod_name, qboolean &bAlreadyCached)
{
	int					i,l, j;
	mdxmHeader_t		*pinmodel, *mdxm;
	mdxmLOD_t			*lod;
	mdxmSurface_t		*surf;
	int					version;
	int					size;
	mdxmSurfHierarchy_t	*surfInfo;

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
		Com_Printf (S_COLOR_YELLOW  "R_LoadMDXM: %s has wrong version (%i should be %i)\n",
				 mod_name, version, MDXM_VERSION);
		return qfalse;
	}

	mod->type	   = MOD_MDXM;
	mod->dataSize += size;

	qboolean bAlreadyFound = qfalse;
	mdxm = (mdxmHeader_t*)CModelCache->Allocate(size, buffer, mod_name, &bAlreadyFound, TAG_MODEL_GLM);
	mod->data.glm = (mdxmData_t *)ri.Hunk_Alloc (sizeof (mdxmData_t), h_low);
	mod->data.glm->header = mdxm;

	//RE_RegisterModels_Malloc(size, buffer, mod_name, &bAlreadyFound, TAG_MODEL_GLM);

	assert(bAlreadyCached == bAlreadyFound);

	if (!bAlreadyFound)
	{
		// horrible new hackery, if !bAlreadyFound then we've just done a
		// tag-morph, so we need to set the bool reference passed into this
		// function to true, to tell the caller NOT to do an ri.FS_Freefile
		// since we've hijacked that memory block...
		//
		// Aaaargh. Kill me now...
		//
		bAlreadyCached = qtrue;
		assert( mdxm == buffer );

		LL(mdxm->ident);
		LL(mdxm->version);
		LL(mdxm->numLODs);
		LL(mdxm->ofsLODs);
		LL(mdxm->numSurfaces);
		LL(mdxm->ofsSurfHierarchy);
		LL(mdxm->ofsEnd);
	}

	// first up, go load in the animation file we need that has the skeletal
	// animation info for this model
	mdxm->animIndex = RE_RegisterModel(va ("%s.gla",mdxm->animName));

	if (!mdxm->animIndex)
	{
		Com_Printf (S_COLOR_YELLOW  "R_LoadMDXM: missing animation file %s for mesh %s\n", mdxm->animName, mdxm->name);
		return qfalse;
	}

	mod->numLods = mdxm->numLODs -1 ;	//copy this up to the model for ease of use - it wil get inced after this.

	if (bAlreadyFound)
	{
		return qtrue;	// All done. Stop, go no further, do not LittleLong(), do not pass Go...
	}

	bool isAnOldModelFile = false;
	if (mdxm->numBones == 72 && strstr(mdxm->animName,"_humanoid") )
	{
		isAnOldModelFile = true;
	}

	surfInfo = (mdxmSurfHierarchy_t *)( (byte *)mdxm + mdxm->ofsSurfHierarchy);
 	for ( i = 0 ; i < mdxm->numSurfaces ; i++)
	{
		LL(surfInfo->numChildren);
		LL(surfInfo->parentIndex);

		Q_strlwr(surfInfo->name);	//just in case
		if ( !strcmp( &surfInfo->name[strlen(surfInfo->name)-4],"_off") )
		{
			surfInfo->name[strlen(surfInfo->name)-4]=0;	//remove "_off" from name
		}

		// do all the children indexs
		for (j=0; j<surfInfo->numChildren; j++)
		{
			LL(surfInfo->childIndexes[j]);
		}

		shader_t	*sh;
		// get the shader name
		sh = R_FindShader( surfInfo->shader, lightmapsNone, stylesDefault, qtrue );
		// insert it in the surface list
		if ( sh->defaultShader )
		{
			surfInfo->shaderIndex = 0;
		}
		else
		{
			surfInfo->shaderIndex = sh->index;
		}

		CModelCache->StoreShaderRequest(mod_name, &surfInfo->shader[0], &surfInfo->shaderIndex);

		// find the next surface
		surfInfo = (mdxmSurfHierarchy_t *)((byte *)surfInfo + offsetof(mdxmSurfHierarchy_t, childIndexes) + sizeof(int) * surfInfo->numChildren);
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
				Com_Error(
					ERR_DROP,
					"R_LoadMDXM: %s has more than %i verts on a surface (%i)",
					mod_name,
					SHADER_MAX_VERTEXES,
					surf->numVerts);
			}
			if ( surf->numTriangles*3 > SHADER_MAX_INDEXES ) {
				Com_Error(
					ERR_DROP,
					"R_LoadMDXM: %s has more than %i triangles on a surface (%i)",
					mod_name,
					SHADER_MAX_INDEXES / 3,
					surf->numTriangles);
			}

			// change to surface identifier
			surf->ident = SF_MDX;
			// register the shaders

			if (isAnOldModelFile)
			{
				int *boneRef = (int *) ( (byte *)surf + surf->ofsBoneReferences );
				for ( j = 0 ; j < surf->numBoneReferences ; j++ )
				{
					if (boneRef[j] >= 0 && boneRef[j] < 72)
					{
						boneRef[j]=OldToNewRemapTable[boneRef[j]];
					}
					else
					{
						boneRef[j]=0;
					}
				}
			}
			// find the next surface
			surf = (mdxmSurface_t *)( (byte *)surf + surf->ofsEnd );
		}
		// find the next LOD
		lod = (mdxmLOD_t *)( (byte *)lod + lod->ofsEnd );
	}

	// Make a copy on the GPU
	lod = (mdxmLOD_t *)((byte *)mdxm + mdxm->ofsLODs);

	mod->data.glm->vboModels = (mdxmVBOModel_t *)ri.Hunk_Alloc (sizeof (mdxmVBOModel_t) * mdxm->numLODs, h_low);
	for ( l = 0; l < mdxm->numLODs; l++ )
	{
		mdxmVBOModel_t *vboModel = &mod->data.glm->vboModels[l];
		mdxmVBOMesh_t *vboMeshes;

		vec3_t *verts;
		uint32_t *normals;
		vec2_t *texcoords;
		byte *bonerefs;
		byte *weights;
		uint32_t *tangents;

		byte *data;
		int dataSize = 0;
		int ofsPosition, ofsNormals, ofsTexcoords, ofsBoneRefs, ofsWeights, ofsTangents;
		int stride = 0;
		int numVerts = 0;
		int numTriangles = 0;

		// +1 to add total vertex count
		int *baseVertexes = (int *)ri.Hunk_AllocateTempMemory (sizeof (int) * (mdxm->numSurfaces + 1));
		int *indexOffsets = (int *)ri.Hunk_AllocateTempMemory (sizeof (int) * mdxm->numSurfaces);

		vboModel->numVBOMeshes = mdxm->numSurfaces;
		vboModel->vboMeshes = (mdxmVBOMesh_t *)ri.Hunk_Alloc (sizeof (mdxmVBOMesh_t) * mdxm->numSurfaces, h_low);
		vboMeshes = vboModel->vboMeshes;

		surf = (mdxmSurface_t *)((byte *)lod + sizeof (mdxmLOD_t) + (mdxm->numSurfaces * sizeof (mdxmLODSurfOffset_t)));

		// Calculate the required size of the vertex buffer.
		for ( int n = 0; n < mdxm->numSurfaces; n++ )
		{
			baseVertexes[n] = numVerts;
			indexOffsets[n] = numTriangles * 3;

			numVerts += surf->numVerts;
			numTriangles += surf->numTriangles;

			surf = (mdxmSurface_t *)((byte *)surf + surf->ofsEnd);
		}

		baseVertexes[mdxm->numSurfaces] = numVerts;

		dataSize += numVerts * sizeof (*verts);
		dataSize += numVerts * sizeof (*normals);
		dataSize += numVerts * sizeof (*texcoords);
		dataSize += numVerts * sizeof (*weights) * 4;
		dataSize += numVerts * sizeof (*bonerefs) * 4;
		dataSize += numVerts * sizeof (*tangents);

		// Allocate and write to memory
		data = (byte *)ri.Hunk_AllocateTempMemory (dataSize);

		ofsPosition = stride;
		verts = (vec3_t *)(data + ofsPosition);
		stride += sizeof (*verts);

		ofsNormals = stride;
		normals = (uint32_t *)(data + ofsNormals);
		stride += sizeof (*normals);

		ofsTexcoords = stride;
		texcoords = (vec2_t *)(data + ofsTexcoords);
		stride += sizeof (*texcoords);

		ofsBoneRefs = stride;
		bonerefs = data + ofsBoneRefs;
		stride += sizeof (*bonerefs) * 4;

		ofsWeights = stride;
		weights = data + ofsWeights;
		stride += sizeof (*weights) * 4;

		ofsTangents = stride;
		tangents = (uint32_t *)(data + ofsTangents);
		stride += sizeof (*tangents);

		// Fill in the index buffer and compute tangents
		glIndex_t *indices = (glIndex_t *)ri.Hunk_AllocateTempMemory(sizeof(glIndex_t) * numTriangles * 3);
		glIndex_t *index = indices;
		uint32_t *tangentsf = (uint32_t *)ri.Hunk_AllocateTempMemory(sizeof(uint32_t) * numVerts);

		surf = (mdxmSurface_t *)((byte *)lod + sizeof(mdxmLOD_t) + (mdxm->numSurfaces * sizeof(mdxmLODSurfOffset_t)));

		for (int n = 0; n < mdxm->numSurfaces; n++)
		{
			mdxmTriangle_t *t = (mdxmTriangle_t *)((byte *)surf + surf->ofsTriangles);
			glIndex_t *surf_indices = (glIndex_t *)ri.Hunk_AllocateTempMemory(sizeof(glIndex_t) * surf->numTriangles * 3);
			glIndex_t *surf_index = surf_indices;

			for (int k = 0; k < surf->numTriangles; k++, index += 3, surf_index += 3)
			{
				index[0] = t[k].indexes[0] + baseVertexes[n];
				assert(index[0] >= 0 && index[0] < numVerts);

				index[1] = t[k].indexes[1] + baseVertexes[n];
				assert(index[1] >= 0 && index[1] < numVerts);

				index[2] = t[k].indexes[2] + baseVertexes[n];
				assert(index[2] >= 0 && index[2] < numVerts);

				surf_index[0] = t[k].indexes[0];
				surf_index[1] = t[k].indexes[1];
				surf_index[2] = t[k].indexes[2];
			}

			// Build tangent space
			mdxmVertex_t *vertices = (mdxmVertex_t *)((byte *)surf + surf->ofsVerts);
			mdxmVertexTexCoord_t *textureCoordinates = (mdxmVertexTexCoord_t *)(vertices + surf->numVerts);

			R_CalcMikkTSpaceGlmSurface(
				surf->numTriangles,
				vertices,
				textureCoordinates,
				tangentsf + baseVertexes[n],
				surf_indices
			);

			ri.Hunk_FreeTempMemory(surf_indices);

			surf = (mdxmSurface_t *)((byte *)surf + surf->ofsEnd);
		}

		assert(index == (indices + numTriangles * 3));

		surf = (mdxmSurface_t *)((byte *)lod + sizeof (mdxmLOD_t) + (mdxm->numSurfaces * sizeof (mdxmLODSurfOffset_t)));

		for (int n = 0; n < mdxm->numSurfaces; n++)
		{
			// Positions and normals
			mdxmVertex_t *v = (mdxmVertex_t *)((byte *)surf + surf->ofsVerts);
			int *boneRef = (int *)((byte *)surf + surf->ofsBoneReferences);

			for (int k = 0; k < surf->numVerts; k++)
			{
				VectorCopy(v[k].vertCoords, *verts);
				*normals = R_VboPackNormal(v[k].normal);

				verts = (vec3_t *)((byte *)verts + stride);
				normals = (uint32_t *)((byte *)normals + stride);
			}

			// Weights
			for (int k = 0; k < surf->numVerts; k++)
			{
				int numWeights = G2_GetVertWeights(&v[k]);
				int lastWeight = 255;
				int lastInfluence = numWeights - 1;
				for (int w = 0; w < lastInfluence; w++)
				{
					float weight = G2_GetVertBoneWeightNotSlow(&v[k], w);
					weights[w] = (byte)(weight * 255.0f);
					int packedIndex = G2_GetVertBoneIndex(&v[k], w);
					bonerefs[w] = boneRef[packedIndex];

					lastWeight -= weights[w];
				}

				assert(lastWeight > 0);

				// Ensure that all the weights add up to 1.0
				weights[lastInfluence] = lastWeight;
				int packedIndex = G2_GetVertBoneIndex(&v[k], lastInfluence);
				bonerefs[lastInfluence] = boneRef[packedIndex];

				// Fill in the rest of the info with zeroes.
				for (int w = numWeights; w < 4; w++)
				{
					weights[w] = 0;
					bonerefs[w] = 0;
				}

				weights += stride;
				bonerefs += stride;
			}

			// Texture coordinates
			mdxmVertexTexCoord_t *tc = (mdxmVertexTexCoord_t *)(v + surf->numVerts);
			for (int k = 0; k < surf->numVerts; k++)
			{
				(*texcoords)[0] = tc[k].texCoords[0];
				(*texcoords)[1] = tc[k].texCoords[1];

				texcoords = (vec2_t *)((byte *)texcoords + stride);
			}

			for (int k = 0; k < surf->numVerts; k++)
			{
				*tangents = *(tangentsf + baseVertexes[n] + k);
				tangents = (uint32_t *)((byte *)tangents + stride);
			}

			surf = (mdxmSurface_t *)((byte *)surf + surf->ofsEnd);
		}

		assert ((byte *)verts == (data + dataSize));

		const char *modelName = strrchr (mdxm->name, '/');
		if (modelName == NULL)
		{
			modelName = mdxm->name;
		}
		VBO_t *vbo = R_CreateVBO (data, dataSize, VBO_USAGE_STATIC);
		IBO_t *ibo = R_CreateIBO((byte *)indices, sizeof(glIndex_t) * numTriangles * 3, VBO_USAGE_STATIC);

		ri.Hunk_FreeTempMemory (data);
		ri.Hunk_FreeTempMemory (tangentsf);
		ri.Hunk_FreeTempMemory (indices);

		vbo->offsets[ATTR_INDEX_POSITION] = ofsPosition;
		vbo->offsets[ATTR_INDEX_NORMAL] = ofsNormals;
		vbo->offsets[ATTR_INDEX_TEXCOORD0] = ofsTexcoords;
		vbo->offsets[ATTR_INDEX_BONE_INDEXES] = ofsBoneRefs;
		vbo->offsets[ATTR_INDEX_BONE_WEIGHTS] = ofsWeights;
		vbo->offsets[ATTR_INDEX_TANGENT] = ofsTangents;

		vbo->strides[ATTR_INDEX_POSITION] = stride;
		vbo->strides[ATTR_INDEX_NORMAL] = stride;
		vbo->strides[ATTR_INDEX_TEXCOORD0] = stride;
		vbo->strides[ATTR_INDEX_BONE_INDEXES] = stride;
		vbo->strides[ATTR_INDEX_BONE_WEIGHTS] = stride;
		vbo->strides[ATTR_INDEX_TANGENT] = stride;

		vbo->sizes[ATTR_INDEX_POSITION] = sizeof(*verts);
		vbo->sizes[ATTR_INDEX_NORMAL] = sizeof(*normals);
		vbo->sizes[ATTR_INDEX_TEXCOORD0] = sizeof(*texcoords);
		vbo->sizes[ATTR_INDEX_BONE_WEIGHTS] = sizeof(*weights);
		vbo->sizes[ATTR_INDEX_BONE_INDEXES] = sizeof(*bonerefs);
		vbo->sizes[ATTR_INDEX_TANGENT] = sizeof(*tangents);

		surf = (mdxmSurface_t *)((byte *)lod + sizeof (mdxmLOD_t) + (mdxm->numSurfaces * sizeof (mdxmLODSurfOffset_t)));

		for ( int n = 0; n < mdxm->numSurfaces; n++ )
		{
			vboMeshes[n].vbo = vbo;
			vboMeshes[n].ibo = ibo;

			vboMeshes[n].indexOffset = indexOffsets[n];
			vboMeshes[n].minIndex = baseVertexes[n];
			vboMeshes[n].maxIndex = baseVertexes[n + 1] - 1;
			vboMeshes[n].numVertexes = surf->numVerts;
			vboMeshes[n].numIndexes = surf->numTriangles * 3;

			surf = (mdxmSurface_t *)((byte *)surf + surf->ofsEnd);
		}

		vboModel->vbo = vbo;
		vboModel->ibo = ibo;

		ri.Hunk_FreeTempMemory (indexOffsets);
		ri.Hunk_FreeTempMemory (baseVertexes);

		lod = (mdxmLOD_t *)((byte *)lod + lod->ofsEnd);
	}

	return qtrue;
}

//#define CREATE_LIMB_HIERARCHY

#ifdef CREATE_LIMB_HIERARCHY

#define NUM_ROOTPARENTS				4
#define NUM_OTHERPARENTS			12
#define NUM_BOTTOMBONES				4

#define CHILD_PADDING				4 //I don't know, I guess this can be changed.

static const char *rootParents[NUM_ROOTPARENTS] =
{
	"rfemurYZ",
	"rhumerus",
	"lfemurYZ",
	"lhumerus"
};

static const char *otherParents[NUM_OTHERPARENTS] =
{
	"rhumerusX",
	"rradius",
	"rradiusX",
	"lhumerusX",
	"lradius",
	"lradiusX",
	"rfemurX",
	"rtibia",
	"rtalus",
	"lfemurX",
	"ltibia",
	"ltalus"
};

static const char *bottomBones[NUM_BOTTOMBONES] =
{
	"rtarsal",
	"rhand",
	"ltarsal",
	"lhand"
};

qboolean BoneIsRootParent(char *name)
{
	int i = 0;

	while (i < NUM_ROOTPARENTS)
	{
		if (!Q_stricmp(name, rootParents[i]))
		{
			return qtrue;
		}

		i++;
	}

	return qfalse;
}

qboolean BoneIsOtherParent(char *name)
{
	int i = 0;

	while (i < NUM_OTHERPARENTS)
	{
		if (!Q_stricmp(name, otherParents[i]))
		{
			return qtrue;
		}

		i++;
	}

	return qfalse;
}

qboolean BoneIsBottom(char *name)
{
	int i = 0;

	while (i < NUM_BOTTOMBONES)
	{
		if (!Q_stricmp(name, bottomBones[i]))
		{
			return qtrue;
		}

		i++;
	}

	return qfalse;
}

void ShiftMemoryDown(mdxaSkelOffsets_t *offsets, mdxaHeader_t *mdxa, int boneIndex, byte **endMarker)
{
	int i = 0;

	//where the next bone starts
	byte *nextBone = ((byte *)mdxa + sizeof(mdxaHeader_t) + offsets->offsets[boneIndex+1]);
	int size = (*endMarker - nextBone);

	memmove((nextBone+CHILD_PADDING), nextBone, size);
	memset(nextBone, 0, CHILD_PADDING);
	*endMarker += CHILD_PADDING;
	// Move the whole thing down CHILD_PADDING amount in memory, clear the new
	// preceding space, and increment the end pointer.

	i = boneIndex+1;

	// Now add CHILD_PADDING amount to every offset beginning at the offset of
	// the bone that was moved.
	while (i < mdxa->numBones)
	{
		offsets->offsets[i] += CHILD_PADDING;
		i++;
	}

	mdxa->ofsFrames += CHILD_PADDING;
	mdxa->ofsCompBonePool += CHILD_PADDING;
	mdxa->ofsEnd += CHILD_PADDING;
	// ofsSkel does not need to be updated because we are only moving memory
	// after that point.
}

//Proper/desired hierarchy list
static const char *BoneHierarchyList[] =
{
	"lfemurYZ",
	"lfemurX",
	"ltibia",
	"ltalus",
	"ltarsal",

	"rfemurYZ",
	"rfemurX",
	"rtibia",
	"rtalus",
	"rtarsal",

	"lhumerus",
	"lhumerusX",
	"lradius",
	"lradiusX",
	"lhand",

	"rhumerus",
	"rhumerusX",
	"rradius",
	"rradiusX",
	"rhand",

	0
};

//Gets the index of a child or parent. If child is passed as qfalse then parent is assumed.
int BoneParentChildIndex(
	mdxaHeader_t *mdxa,
	mdxaSkelOffsets_t *offsets,
	mdxaSkel_t *boneInfo,
	qboolean child)
{
	int i = 0;
	int matchindex = -1;
	mdxaSkel_t *bone;
	const char *match = NULL;

	while (BoneHierarchyList[i])
	{
		if (!Q_stricmp(boneInfo->name, BoneHierarchyList[i]))
		{
			// we have a match, the slot above this will be our desired parent.
			// (or below for child)
			if (child)
			{
				match = BoneHierarchyList[i+1];
			}
			else
			{
				match = BoneHierarchyList[i-1];
			}
			break;
		}
		i++;
	}

	if (!match)
	{ //no good
		return -1;
	}

	i = 0;

	while (i < mdxa->numBones)
	{
		bone = (mdxaSkel_t *)((byte *)mdxa + sizeof(mdxaHeader_t) + offsets->offsets[i]);

		if (bone && !Q_stricmp(bone->name, match))
		{ //this is the one
			matchindex = i;
			break;
		}

		i++;
	}

	return matchindex;
}
#endif //CREATE_LIMB_HIERARCHY

/*
=================
R_LoadMDXA - load a Ghoul 2 animation file
=================
*/
qboolean R_LoadMDXA(model_t *mod, void *buffer, const char *mod_name, qboolean &bAlreadyCached)
{

	mdxaHeader_t *pinmodel, *mdxa;
	int version;
	int size;
#ifdef CREATE_LIMB_HIERARCHY
	int oSize = 0;
	byte *sizeMarker;
#endif

#if 0 //#ifndef _M_IX86
	int					j, k, i;
	int					frameSize;
	mdxaFrame_t			*cframe;
	mdxaSkel_t			*boneInfo;
#endif

	pinmodel = (mdxaHeader_t *)buffer;
	//
	// read some fields from the binary, but only LittleLong() them when we know this wasn't an
	// already-cached model...
	//
	version = (pinmodel->version);
	size = (pinmodel->ofsEnd);

	if (!bAlreadyCached)
	{
		LL(version);
		LL(size);
	}

	if (version != MDXA_VERSION)
	{
		Com_Printf(
			S_COLOR_YELLOW "R_LoadMDXA: %s has wrong version (%i should be %i)\n",
			mod_name,
			version,
			MDXA_VERSION);
		return qfalse;
	}

	mod->type = MOD_MDXA;
	mod->dataSize += size;

	qboolean bAlreadyFound = qfalse;

#ifdef CREATE_LIMB_HIERARCHY
	oSize = size;

	int childNumber = (NUM_ROOTPARENTS + NUM_OTHERPARENTS);

	// Allocate us some extra space so we can shift memory down.
	size += (childNumber * (CHILD_PADDING * 8));
#endif // CREATE_LIMB_HIERARCHY

	mdxa = (mdxaHeader_t *)CModelCache->Allocate(
		size, buffer, mod_name, &bAlreadyFound, TAG_MODEL_GLA);
	mod->data.gla = mdxa;

	// I should probably eliminate 'bAlreadyFound', but wtf?
	assert(bAlreadyCached == bAlreadyFound);

	if (!bAlreadyFound)
	{
#ifdef CREATE_LIMB_HIERARCHY
		memcpy(mdxa, buffer, oSize);
#else
		// horrible new hackery, if !bAlreadyFound then we've just done a
		// tag-morph, so we need to set the bool reference passed into this
		// function to true, to tell the caller NOT to do an
		// ri.FS_Freefile since we've hijacked that memory block...
		//
		// Aaaargh. Kill me now...
		//
		bAlreadyCached = qtrue;
		assert(mdxa == buffer);
#endif
		LL(mdxa->ident);
		LL(mdxa->version);
		LL(mdxa->numFrames);
		LL(mdxa->numBones);
		LL(mdxa->ofsFrames);
		LL(mdxa->ofsEnd);
	}

#ifdef CREATE_LIMB_HIERARCHY
	if (!bAlreadyFound)
	{
		mdxaSkel_t *boneParent;

		sizeMarker = (byte *)mdxa + mdxa->ofsEnd;

		// rww - This is probably temporary until we put actual hierarchy in
		// for the models.  It is necessary for the correct operation of
		// ragdoll.
		mdxaSkelOffsets_t *offsets = (mdxaSkelOffsets_t *)((byte *)mdxa + sizeof(mdxaHeader_t));

		for (i = 0; i < mdxa->numBones; i++)
		{
			boneInfo = (mdxaSkel_t *)((byte *)offsets + offsets->offsets[i]);

			if (boneInfo)
			{
				char *bname = boneInfo->name;

				if (BoneIsRootParent(bname))
				{
					// These are the main parent bones. We don't want to change
					// their parents, but we want to give them children.
					ShiftMemoryDown(offsets, mdxa, i, &sizeMarker);

					boneInfo = (mdxaSkel_t *)((byte *)offsets + offsets->offsets[i]);

					int newChild = BoneParentChildIndex(mdxa, offsets, boneInfo, qtrue);

					if (newChild != -1)
					{
						boneInfo->numChildren++;
						boneInfo->children[boneInfo->numChildren - 1] = newChild;
					}
					else
					{
						assert(!"Failed to find matching child for bone in hierarchy creation");
					}
				}
				else if (BoneIsOtherParent(bname) || BoneIsBottom(bname))
				{
					if (!BoneIsBottom(bname))
					{ // unless it's last in the chain it has the next bone as a child.
						ShiftMemoryDown(offsets, mdxa, i, &sizeMarker);

						boneInfo = (mdxaSkel_t *)
							((byte *)mdxa + sizeof(mdxaHeader_t) + offsets->offsets[i]);
						int newChild = BoneParentChildIndex(mdxa, offsets, boneInfo, qtrue);

						if (newChild != -1)
						{
							boneInfo->numChildren++;
							boneInfo->children[boneInfo->numChildren - 1] = newChild;
						}
						else
						{
							assert(!"Failed to find matching child for bone in hierarchy creation");
						}
					}

					// Before we set the parent we want to remove this as a
					// child for whoever was parenting it.
					int oldParent = boneInfo->parent;

					if (oldParent > -1)
					{
						boneParent = (mdxaSkel_t *)
							((byte *)offsets + offsets->offsets[oldParent]);
					}
					else
					{
						boneParent = NULL;
					}

					if (boneParent)
					{
						k = 0;

						while (k < boneParent->numChildren)
						{
							if (boneParent->children[k] == i)
							{ // this bone is the child
								k++;
								while (k < boneParent->numChildren)
								{
									boneParent->children[k - 1] = boneParent->children[k];
									k++;
								}
								boneParent->children[k - 1] = 0;
								boneParent->numChildren--;
								break;
							}
							k++;
						}
					}

					// Now that we have cleared the original parent of
					// ownership, mark the bone's new parent.
					int newParent = BoneParentChildIndex(mdxa, offsets, boneInfo, qfalse);

					if (newParent != -1)
					{
						boneInfo->parent = newParent;
					}
					else
					{
						assert(!"Failed to find matching parent for bone in hierarchy creation");
					}
				}
			}
		}
	}
#endif // CREATE_LIMB_HIERARCHY

	if (mdxa->numFrames < 1)
	{
		Com_Printf(S_COLOR_YELLOW "R_LoadMDXA: %s has no frames\n", mod_name);
		return qfalse;
	}

	if (bAlreadyFound)
	{
		return qtrue; // All done, stop here, do not LittleLong() etc. Do not pass go...
	}

	return qtrue;
}

