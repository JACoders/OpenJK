/*
===========================================================================
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

#include "client/client.h"	//FIXME!! EVIL - just include the definitions needed
#include "tr_local.h"
#include "qcommon/matcomp.h"
#include "qcommon/qcommon.h"
#include "ghoul2/G2.h"
#include "ghoul2/g2_local.h"
#ifdef _G2_GORE
#include "ghoul2/G2_gore.h"
#endif

#include "qcommon/disablewarnings.h"

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

bool HackadelicOnClient=false; // means this is a render traversal

qboolean G2_SetupModelPointers(CGhoul2Info *ghlInfo);
qboolean G2_SetupModelPointers(CGhoul2Info_v &ghoul2);

extern cvar_t	*r_Ghoul2AnimSmooth;
extern cvar_t	*r_Ghoul2UnSqashAfterSmooth;

#if 0
static inline int G2_Find_Bone_ByNum(const model_t *mod, boneInfo_v &blist, const int boneNum)
{
	size_t i = 0;

	while (i < blist.size())
	{
		if (blist[i].boneNumber == boneNum)
		{
			return i;
		}
		i++;
	}

	return -1;
}
#endif

const static mdxaBone_t		identityMatrix =
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

class CConstructBoneList
{
public:
	int				surfaceNum;
	int				*boneUsedList;
	surfaceInfo_v	&rootSList;
	model_t			*currentModel;
	boneInfo_v		&boneList;

	CConstructBoneList(
	int				initsurfaceNum,
	int				*initboneUsedList,
	surfaceInfo_v	&initrootSList,
	model_t			*initcurrentModel,
	boneInfo_v		&initboneList):

	surfaceNum(initsurfaceNum),
	boneUsedList(initboneUsedList),
	rootSList(initrootSList),
	currentModel(initcurrentModel),
	boneList(initboneList) { }

};

class CTransformBone
{
public:
	int				touch; // for minimal recalculation
	//rww - RAGDOLL_BEGIN
	int				touchRender;
	//rww - RAGDOLL_END
	mdxaBone_t		boneMatrix; //final matrix
	int				parent; // only set once

	CTransformBone()
	{
		touch=0;
	//rww - RAGDOLL_BEGIN
		touchRender = 0;
	//rww - RAGDOLL_END
	}

};

struct SBoneCalc
{
	int				newFrame;
	int				currentFrame;
	float			backlerp;
	float			blendFrame;
	int				blendOldFrame;
	bool			blendMode;
	float			blendLerp;
};

class CBoneCache;
void G2_TransformBone(int index,CBoneCache &CB);

class CBoneCache
{
	void SetRenderMatrix(CTransformBone *bone) {
	}

	void EvalLow(int index)
	{
		assert(index>=0&&index<(int)mBones.size());
		if (mFinalBones[index].touch!=mCurrentTouch)
		{
			// need to evaluate the bone
			assert((mFinalBones[index].parent>=0&&mFinalBones[index].parent<(int)mFinalBones.size())||(index==0&&mFinalBones[index].parent==-1));
			if (mFinalBones[index].parent>=0)
			{
				EvalLow(mFinalBones[index].parent); // make sure parent is evaluated
				SBoneCalc &par=mBones[mFinalBones[index].parent];
				mBones[index].newFrame=par.newFrame;
				mBones[index].currentFrame=par.currentFrame;
				mBones[index].backlerp=par.backlerp;
				mBones[index].blendFrame=par.blendFrame;
				mBones[index].blendOldFrame=par.blendOldFrame;
				mBones[index].blendMode=par.blendMode;
				mBones[index].blendLerp=par.blendLerp;
			}
			G2_TransformBone(index,*this);
			mFinalBones[index].touch=mCurrentTouch;
		}
	}
//rww - RAGDOLL_BEGIN
	void SmoothLow(int index)
	{
		if (mSmoothBones[index].touch==mLastTouch)
		{
			int i;
			float *oldM=&mSmoothBones[index].boneMatrix.matrix[0][0];
			float *newM=&mFinalBones[index].boneMatrix.matrix[0][0];
#if 0 //this is just too slow. I need a better way.
			static float smoothFactor;

			smoothFactor = mSmoothFactor;

			//Special rag smoothing -rww
			if (smoothFactor < 0)
			{ //I need a faster way to do this but I do not want to store more in the bonecache
				static int blistIndex;
				assert(mod);
				assert(rootBoneList);
				blistIndex = G2_Find_Bone_ByNum(mod, *rootBoneList, index);

				assert(blistIndex != -1);

				boneInfo_t &bone = (*rootBoneList)[blistIndex];

				if (bone.flags & BONE_ANGLES_RAGDOLL)
				{
					if ((bone.RagFlags & (0x00008)) || //pelvis
                        (bone.RagFlags & (0x00004))) //model_root
					{ //pelvis and root do not smooth much
						smoothFactor = 0.2f;
					}
					else if (bone.solidCount > 4)
					{ //if stuck in solid a lot then snap out quickly
						smoothFactor = 0.1f;
					}
					else
					{ //otherwise smooth a bunch
						smoothFactor = 0.8f;
					}
				}
				else
				{ //not a rag bone
					smoothFactor = 0.3f;
				}
			}
#endif

			for (i=0;i<12;i++,oldM++,newM++)
			{
				*oldM=mSmoothFactor*(*oldM-*newM)+*newM;
			}
		}
		else
		{
			memcpy(&mSmoothBones[index].boneMatrix,&mFinalBones[index].boneMatrix,sizeof(mdxaBone_t));
		}
		mdxaSkelOffsets_t *offsets = (mdxaSkelOffsets_t *)((byte *)header + sizeof(mdxaHeader_t));
		mdxaSkel_t *skel = (mdxaSkel_t *)((byte *)header + sizeof(mdxaHeader_t) + offsets->offsets[index]);
		mdxaBone_t tempMatrix;
		Multiply_3x4Matrix(&tempMatrix,&mSmoothBones[index].boneMatrix, &skel->BasePoseMat);
		float maxl;
		maxl=VectorLength(&skel->BasePoseMat.matrix[0][0]);
		VectorNormalize(&tempMatrix.matrix[0][0]);
		VectorNormalize(&tempMatrix.matrix[1][0]);
		VectorNormalize(&tempMatrix.matrix[2][0]);

		VectorScale(&tempMatrix.matrix[0][0],maxl,&tempMatrix.matrix[0][0]);
		VectorScale(&tempMatrix.matrix[1][0],maxl,&tempMatrix.matrix[1][0]);
		VectorScale(&tempMatrix.matrix[2][0],maxl,&tempMatrix.matrix[2][0]);
		Multiply_3x4Matrix(&mSmoothBones[index].boneMatrix,&tempMatrix,&skel->BasePoseMatInv);
		mSmoothBones[index].touch=mCurrentTouch;
#ifdef _DEBUG
		for ( int i = 0; i < 3; i++ )
		{
			for ( int j = 0; j < 4; j++ )
			{
				assert( !Q_isnan(mSmoothBones[index].boneMatrix.matrix[i][j]));
			}
		}
#endif// _DEBUG
	}
//rww - RAGDOLL_END
public:
	int					frameSize;
	const mdxaHeader_t	*header;
	const model_t		*mod;

	// these are split for better cpu cache behavior
	std::vector<SBoneCalc> mBones;
	std::vector<CTransformBone> mFinalBones;

	std::vector<CTransformBone> mSmoothBones; // for render smoothing
	//vector<mdxaSkel_t *>   mSkels;

	boneInfo_v		*rootBoneList;
	mdxaBone_t		rootMatrix;
	int				incomingTime;

	int				mCurrentTouch;
	//rww - RAGDOLL_BEGIN
	int				mCurrentTouchRender;
	int				mLastTouch;
	int				mLastLastTouch;
	//rww - RAGDOLL_END

	// for render smoothing
	bool			mSmoothingActive;
	bool			mUnsquash;
	float			mSmoothFactor;

	CBoneCache(const model_t *amod,const mdxaHeader_t *aheader) :
		header(aheader),
		mod(amod)
	{
		assert(amod);
		assert(aheader);
		mSmoothingActive=false;
		mUnsquash=false;
		mSmoothFactor=0.0f;

		int numBones=header->numBones;
		mBones.resize(numBones);
		mFinalBones.resize(numBones);
		mSmoothBones.resize(numBones);
//		mSkels.resize(numBones);
		//rww - removed mSkels
		mdxaSkelOffsets_t *offsets;
		mdxaSkel_t		*skel;
		offsets = (mdxaSkelOffsets_t *)((byte *)header + sizeof(mdxaHeader_t));

		int i;
		for (i=0;i<numBones;i++)
		{
			skel = (mdxaSkel_t *)((byte *)header + sizeof(mdxaHeader_t) + offsets->offsets[i]);
			//mSkels[i]=skel;
			//ditto
			mFinalBones[i].parent=skel->parent;
		}
		mCurrentTouch=3;
//rww - RAGDOLL_BEGIN
		mLastTouch=2;
		mLastLastTouch=1;
//rww - RAGDOLL_END
	}

	SBoneCalc &Root()
	{
		assert(mBones.size());
		return mBones[0];
	}
	const mdxaBone_t &EvalUnsmooth(int index)
	{
		EvalLow(index);
		if (mSmoothingActive&&mSmoothBones[index].touch)
		{
			return mSmoothBones[index].boneMatrix;
		}
		return mFinalBones[index].boneMatrix;
	}
	const mdxaBone_t &Eval(int index)
	{
		/*
		bool wasEval=EvalLow(index);
		if (mSmoothingActive)
		{
			if (mSmoothBones[index].touch!=incomingTime||wasEval)
			{
				float dif=float(incomingTime)-float(mSmoothBones[index].touch);
				if (mSmoothBones[index].touch&&dif<300.0f)
				{

					if (dif<16.0f)  // 60 fps
					{
						dif=16.0f;
					}
					if (dif>100.0f) // 10 fps
					{
						dif=100.0f;
					}
					float f=1.0f-pow(1.0f-mSmoothFactor,16.0f/dif);

					int i;
					float *oldM=&mSmoothBones[index].boneMatrix.matrix[0][0];
					float *newM=&mFinalBones[index].boneMatrix.matrix[0][0];
					for (i=0;i<12;i++,oldM++,newM++)
					{
						*oldM=f*(*oldM-*newM)+*newM;
					}
					if (mUnsquash)
					{
						mdxaBone_t tempMatrix;
						Multiply_3x4Matrix(&tempMatrix,&mSmoothBones[index].boneMatrix, &mSkels[index]->BasePoseMat);
						float maxl;
						maxl=VectorLength(&mSkels[index]->BasePoseMat.matrix[0][0]);
						VectorNormalize(&tempMatrix.matrix[0][0]);
						VectorNormalize(&tempMatrix.matrix[1][0]);
						VectorNormalize(&tempMatrix.matrix[2][0]);

						VectorScale(&tempMatrix.matrix[0][0],maxl,&tempMatrix.matrix[0][0]);
						VectorScale(&tempMatrix.matrix[1][0],maxl,&tempMatrix.matrix[1][0]);
						VectorScale(&tempMatrix.matrix[2][0],maxl,&tempMatrix.matrix[2][0]);
						Multiply_3x4Matrix(&mSmoothBones[index].boneMatrix,&tempMatrix,&mSkels[index]->BasePoseMatInv);
					}
				}
				else
				{
					memcpy(&mSmoothBones[index].boneMatrix,&mFinalBones[index].boneMatrix,sizeof(mdxaBone_t));
				}
				mSmoothBones[index].touch=incomingTime;
			}
			return mSmoothBones[index].boneMatrix;
		}
		return mFinalBones[index].boneMatrix;
		*/

		//Hey, this is what sof2 does. Let's try it out.
		assert(index>=0&&index<(int)mBones.size());
		if (mFinalBones[index].touch!=mCurrentTouch)
		{
			EvalLow(index);
		}
		return mFinalBones[index].boneMatrix;
	}
	//rww - RAGDOLL_BEGIN
	const inline mdxaBone_t &EvalRender(int index)
	{
		assert(index>=0&&index<(int)mBones.size());
		if (mFinalBones[index].touch!=mCurrentTouch)
		{
			mFinalBones[index].touchRender=mCurrentTouchRender;
			EvalLow(index);
		}
		if (mSmoothingActive)
		{
			if (mSmoothBones[index].touch!=mCurrentTouch)
			{
				SmoothLow(index);
			}
			return mSmoothBones[index].boneMatrix;
		}
		return mFinalBones[index].boneMatrix;
	}
	//rww - RAGDOLL_END
	//rww - RAGDOLL_BEGIN
	bool WasRendered(int index)
	{
		assert(index>=0&&index<(int)mBones.size());
		return mFinalBones[index].touchRender==mCurrentTouchRender;
	}
	int GetParent(int index)
	{
		if (index==0)
		{
			return -1;
		}
		assert(index>=0&&index<(int)mBones.size());
		return mFinalBones[index].parent;
	}
	//rww - RAGDOLL_END
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

//rww - RAGDOLL_BEGIN
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
	if (!ghoul2.mBoneCache||!maxDep)
	{
		return 0;
	}

	CBoneCache &boneCache=*ghoul2.mBoneCache;
	mdxaSkel_t		*skel;
	mdxaSkelOffsets_t *offsets;
	offsets = (mdxaSkelOffsets_t *)((byte *)boneCache.header + sizeof(mdxaHeader_t));
	skel = (mdxaSkel_t *)((byte *)boneCache.header + sizeof(mdxaHeader_t) + offsets->offsets[boneNum]);
	int i;
	int ret=0;
	for (i=0;i<skel->numChildren;i++)
	{
		if (!maxDep)
		{
			return i; // number added
		}
		*tempDependents=skel->children[i];
		assert(*tempDependents>0&&*tempDependents<boneCache.header->numBones);
		maxDep--;
		tempDependents++;
		ret++;
	}
	for (i=0;i<skel->numChildren;i++)
	{
		int num=G2_GetBoneDependents(ghoul2,skel->children[i],tempDependents,maxDep);
		tempDependents+=num;
		ret+=num;
		maxDep-=num;
		assert(maxDep>=0);
		if (!maxDep)
		{
			break;
		}
	}
	return ret;
}

bool G2_WasBoneRendered(CGhoul2Info &ghoul2,int boneNum)
{
	if (!ghoul2.mBoneCache)
	{
		return false;
	}
	CBoneCache &boneCache=*ghoul2.mBoneCache;

	return boneCache.WasRendered(boneNum);
}

void G2_GetBoneBasepose(CGhoul2Info &ghoul2,int boneNum,mdxaBone_t *&retBasepose,mdxaBone_t *&retBaseposeInv)
{
	if (!ghoul2.mBoneCache)
	{
		// yikes
		retBasepose=const_cast<mdxaBone_t *>(&identityMatrix);
		retBaseposeInv=const_cast<mdxaBone_t *>(&identityMatrix);
		return;
	}
	assert(ghoul2.mBoneCache);
	CBoneCache &boneCache=*ghoul2.mBoneCache;
	assert(boneCache.mod);
	assert(boneNum>=0&&boneNum<boneCache.header->numBones);

	mdxaSkel_t		*skel;
	mdxaSkelOffsets_t *offsets;
	offsets = (mdxaSkelOffsets_t *)((byte *)boneCache.header + sizeof(mdxaHeader_t));
	skel = (mdxaSkel_t *)((byte *)boneCache.header + sizeof(mdxaHeader_t) + offsets->offsets[boneNum]);
	retBasepose=&skel->BasePoseMat;
	retBaseposeInv=&skel->BasePoseMatInv;
}

char *G2_GetBoneNameFromSkel(CGhoul2Info &ghoul2, int boneNum)
{
	if (!ghoul2.mBoneCache)
	{
		return NULL;
	}
	CBoneCache &boneCache=*ghoul2.mBoneCache;
	assert(boneCache.mod);
	assert(boneNum>=0&&boneNum<boneCache.header->numBones);

	mdxaSkel_t		*skel;
	mdxaSkelOffsets_t *offsets;
	offsets = (mdxaSkelOffsets_t *)((byte *)boneCache.header + sizeof(mdxaHeader_t));
	skel = (mdxaSkel_t *)((byte *)boneCache.header + sizeof(mdxaHeader_t) + offsets->offsets[boneNum]);

	return skel->name;
}

void G2_RagGetBoneBasePoseMatrixLow(CGhoul2Info &ghoul2, int boneNum, mdxaBone_t &boneMatrix, mdxaBone_t &retMatrix, vec3_t scale)
{
	assert(ghoul2.mBoneCache);
	CBoneCache &boneCache=*ghoul2.mBoneCache;
	assert(boneCache.mod);
	assert(boneNum>=0&&boneNum<boneCache.header->numBones);

	mdxaSkel_t		*skel;
	mdxaSkelOffsets_t *offsets;
	offsets = (mdxaSkelOffsets_t *)((byte *)boneCache.header + sizeof(mdxaHeader_t));
	skel = (mdxaSkel_t *)((byte *)boneCache.header + sizeof(mdxaHeader_t) + offsets->offsets[boneNum]);
	Multiply_3x4Matrix(&retMatrix, &boneMatrix, &skel->BasePoseMat);

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

	VectorNormalize((float*)&retMatrix.matrix[0]);
	VectorNormalize((float*)&retMatrix.matrix[1]);
	VectorNormalize((float*)&retMatrix.matrix[2]);
}

void G2_GetBoneMatrixLow(CGhoul2Info &ghoul2,int boneNum,const vec3_t scale,mdxaBone_t &retMatrix,mdxaBone_t *&retBasepose,mdxaBone_t *&retBaseposeInv)
{
	if (!ghoul2.mBoneCache)
	{
		retMatrix=identityMatrix;
		// yikes
		retBasepose=const_cast<mdxaBone_t *>(&identityMatrix);
		retBaseposeInv=const_cast<mdxaBone_t *>(&identityMatrix);
		return;
	}
	mdxaBone_t bolt;
	assert(ghoul2.mBoneCache);
	CBoneCache &boneCache=*ghoul2.mBoneCache;
	assert(boneCache.mod);
	assert(boneNum>=0&&boneNum<boneCache.header->numBones);

	mdxaSkel_t		*skel;
	mdxaSkelOffsets_t *offsets;
	offsets = (mdxaSkelOffsets_t *)((byte *)boneCache.header + sizeof(mdxaHeader_t));
	skel = (mdxaSkel_t *)((byte *)boneCache.header + sizeof(mdxaHeader_t) + offsets->offsets[boneNum]);
	Multiply_3x4Matrix(&bolt, (mdxaBone_t *)&boneCache.Eval(boneNum), &skel->BasePoseMat); // DEST FIRST ARG
	retBasepose=&skel->BasePoseMat;
	retBaseposeInv=&skel->BasePoseMatInv;

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
	VectorNormalize((float*)&bolt.matrix[0]);
	VectorNormalize((float*)&bolt.matrix[1]);
	VectorNormalize((float*)&bolt.matrix[2]);

	Multiply_3x4Matrix(&retMatrix,&worldMatrix, &bolt);

#ifdef _DEBUG
	for ( int i = 0; i < 3; i++ )
	{
		for ( int j = 0; j < 4; j++ )
		{
			assert( !Q_isnan(retMatrix.matrix[i][j]));
		}
	}
#endif// _DEBUG
}

int G2_GetParentBoneMatrixLow(CGhoul2Info &ghoul2,int boneNum,const vec3_t scale,mdxaBone_t &retMatrix,mdxaBone_t *&retBasepose,mdxaBone_t *&retBaseposeInv)
{
	int parent=-1;
	if (ghoul2.mBoneCache)
	{
		CBoneCache &boneCache=*ghoul2.mBoneCache;
		assert(boneCache.mod);
		assert(boneNum>=0&&boneNum<boneCache.header->numBones);
		parent=boneCache.GetParent(boneNum);
		if (parent<0||parent>=boneCache.header->numBones)
		{
			parent=-1;
			retMatrix=identityMatrix;
			// yikes
			retBasepose=const_cast<mdxaBone_t *>(&identityMatrix);
			retBaseposeInv=const_cast<mdxaBone_t *>(&identityMatrix);
		}
		else
		{
			G2_GetBoneMatrixLow(ghoul2,parent,scale,retMatrix,retBasepose,retBaseposeInv);
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
	CGoreSet		*initgore_set):
#else
	boltInfo_v		&initboltList):
#endif

	surfaceNum(initsurfaceNum),
	rootSList(initrootSList),
	cust_shader(initcust_shader),
	fogNum(initfogNum),
	personalModel(initpersonalModel),
	boneCache(initboneCache),
	renderfx(initrenderfx),
	skin(initskin),
	currentModel(initcurrentModel),
	lod(initlod),
#ifdef _G2_GORE
	boltList(initboltList),
	gore_shader(initgore_shader),
	gore_set(initgore_set)
#else
	boltList(initboltList)
#endif
	{}
};

#ifdef _G2_GORE
#define MAX_RENDER_SURFACES (2048)
static CRenderableSurface RSStorage[MAX_RENDER_SURFACES];
static unsigned int NextRS=0;

CRenderableSurface *AllocRS()
{
	CRenderableSurface *ret=&RSStorage[NextRS];
	ret->Init();
	NextRS++;
	NextRS%=MAX_RENDER_SURFACES;
	return ret;
}
#endif

/*

All bones should be an identity orientation to display the mesh exactly
as it is specified.

For all other frames, the bones represent the transformation from the
orientation of the bone in the base frame to the orientation in this
frame.

*/

//======================================================================
//
// Bone Manipulation code


void G2_CreateQuaterion(mdxaBone_t *mat, vec4_t quat)
{
	// this is revised for the 3x4 matrix we use in G2.
    float t = 1 + mat->matrix[0][0] + mat->matrix[1][1] + mat->matrix[2][2];
	float s;

    //If the trace of the matrix is greater than zero, then
    //perform an "instant" calculation.
    //Important note wrt. rouning errors:
    //Test if ( T > 0.00000001 ) to avoid large distortions!
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
		//If the trace of the matrix is equal to zero then identify
		//which major diagonal element has the greatest value.

		//Depending on this, calculate the following:

		if ( mat->matrix[0][0] > mat->matrix[1][1] && mat->matrix[0][0] > mat->matrix[2][2] )  {	// Column 0:
			s  = sqrt( 1.0 + mat->matrix[0][0] - mat->matrix[1][1] - mat->matrix[2][2])* 2;
			quat[0] = 0.25 * s;
			quat[1] = (mat->matrix[0][1] + mat->matrix[1][0] ) / s;
			quat[2] = (mat->matrix[2][0] + mat->matrix[0][2] ) / s;
			quat[3] = (mat->matrix[1][2] - mat->matrix[2][1] ) / s;

		} else if ( mat->matrix[1][1] > mat->matrix[2][2] ) {			// Column 1:
			s  = sqrt( 1.0 + mat->matrix[1][1] - mat->matrix[0][0] - mat->matrix[2][2] ) * 2;
			quat[0] = (mat->matrix[0][1] + mat->matrix[1][0] ) / s;
			quat[1] = 0.25 * s;
			quat[2] = (mat->matrix[1][2] + mat->matrix[2][1] ) / s;
			quat[3] = (mat->matrix[2][0] - mat->matrix[0][2] ) / s;

		} else {						// Column 2:
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

    float xx      = quat[0] * quat[0];
    float xy      = quat[0] * quat[1];
    float xz      = quat[0] * quat[2];
    float xw      = quat[0] * quat[3];

    float yy      = quat[1] * quat[1];
    float yz      = quat[1] * quat[2];
    float yw      = quat[1] * quat[3];

    float zz      = quat[2] * quat[2];
    float zw      = quat[2] * quat[3];

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

// nasty little matrix multiply going on here..
void Multiply_3x4Matrix(mdxaBone_t *out, mdxaBone_t *in2, mdxaBone_t *in)
{
	// first row of out
	out->matrix[0][0] = (in2->matrix[0][0] * in->matrix[0][0]) + (in2->matrix[0][1] * in->matrix[1][0]) + (in2->matrix[0][2] * in->matrix[2][0]);
	out->matrix[0][1] = (in2->matrix[0][0] * in->matrix[0][1]) + (in2->matrix[0][1] * in->matrix[1][1]) + (in2->matrix[0][2] * in->matrix[2][1]);
	out->matrix[0][2] = (in2->matrix[0][0] * in->matrix[0][2]) + (in2->matrix[0][1] * in->matrix[1][2]) + (in2->matrix[0][2] * in->matrix[2][2]);
	out->matrix[0][3] = (in2->matrix[0][0] * in->matrix[0][3]) + (in2->matrix[0][1] * in->matrix[1][3]) + (in2->matrix[0][2] * in->matrix[2][3]) + in2->matrix[0][3];
	// second row of outf out
	out->matrix[1][0] = (in2->matrix[1][0] * in->matrix[0][0]) + (in2->matrix[1][1] * in->matrix[1][0]) + (in2->matrix[1][2] * in->matrix[2][0]);
	out->matrix[1][1] = (in2->matrix[1][0] * in->matrix[0][1]) + (in2->matrix[1][1] * in->matrix[1][1]) + (in2->matrix[1][2] * in->matrix[2][1]);
	out->matrix[1][2] = (in2->matrix[1][0] * in->matrix[0][2]) + (in2->matrix[1][1] * in->matrix[1][2]) + (in2->matrix[1][2] * in->matrix[2][2]);
	out->matrix[1][3] = (in2->matrix[1][0] * in->matrix[0][3]) + (in2->matrix[1][1] * in->matrix[1][3]) + (in2->matrix[1][2] * in->matrix[2][3]) + in2->matrix[1][3];
	// third row of out  out
	out->matrix[2][0] = (in2->matrix[2][0] * in->matrix[0][0]) + (in2->matrix[2][1] * in->matrix[1][0]) + (in2->matrix[2][2] * in->matrix[2][0]);
	out->matrix[2][1] = (in2->matrix[2][0] * in->matrix[0][1]) + (in2->matrix[2][1] * in->matrix[1][1]) + (in2->matrix[2][2] * in->matrix[2][1]);
	out->matrix[2][2] = (in2->matrix[2][0] * in->matrix[0][2]) + (in2->matrix[2][1] * in->matrix[1][2]) + (in2->matrix[2][2] * in->matrix[2][2]);
	out->matrix[2][3] = (in2->matrix[2][0] * in->matrix[0][3]) + (in2->matrix[2][1] * in->matrix[1][3]) + (in2->matrix[2][2] * in->matrix[2][3]) + in2->matrix[2][3];
}


static int G2_GetBonePoolIndex(const mdxaHeader_t *pMDXAHeader, int iFrame, int iBone)
{
	const int iOffsetToIndex	= (iFrame * pMDXAHeader->numBones * 3) + (iBone * 3);
	mdxaIndex_t *pIndex			= (mdxaIndex_t *)((byte*)pMDXAHeader + pMDXAHeader->ofsFrames + iOffsetToIndex);

	return (pIndex->iIndex[2] << 16) + (pIndex->iIndex[1] << 8) + (pIndex->iIndex[0]);
}


/*static inline*/ void UnCompressBone(float mat[3][4], int iBoneIndex, const mdxaHeader_t *pMDXAHeader, int iFrame)
{
	mdxaCompQuatBone_t *pCompBonePool = (mdxaCompQuatBone_t *) ((byte *)pMDXAHeader + pMDXAHeader->ofsCompBonePool);
	MC_UnCompressQuat(mat, pCompBonePool[ G2_GetBonePoolIndex( pMDXAHeader, iFrame, iBoneIndex ) ].Comp);
}

#define DEBUG_G2_TIMING (0)
#define DEBUG_G2_TIMING_RENDER_ONLY (1)

void G2_TimingModel(boneInfo_t &bone,int currentTime,int numFramesInFile,int &currentFrame,int &newFrame,float &lerp)
{
	assert(bone.startFrame>=0);
	assert(bone.startFrame<=numFramesInFile);
	assert(bone.endFrame>=0);
	assert(bone.endFrame<=numFramesInFile);

	// yes - add in animation speed to current frame
	float	animSpeed = bone.animSpeed;
	float	time;
	if (bone.pauseTime)
	{
		time = (bone.pauseTime - bone.startTime) / 50.0f;
	}
	else
	{
		time = (currentTime - bone.startTime) / 50.0f;
	}
	if (time<0.0f)
	{
		time=0.0f;
	}
	float	newFrame_g = bone.startFrame + (time * animSpeed);

	int		animSize = bone.endFrame - bone.startFrame;
	float	endFrame = (float)bone.endFrame;
	// we are supposed to be animating right?
	if (animSize)
	{
		// did we run off the end?
		if (((animSpeed > 0.0f) && (newFrame_g > endFrame - 1)) ||
			((animSpeed < 0.0f) && (newFrame_g < endFrame+1)))
		{
			// yep - decide what to do
			if (bone.flags & BONE_ANIM_OVERRIDE_LOOP)
			{
				// get our new animation frame back within the bounds of the animation set
				if (animSpeed < 0.0f)
				{
					// we don't use this case, or so I am told
					// if we do, let me know, I need to insure the mod works

					// should we be creating a virtual frame?
					if ((newFrame_g < endFrame+1) && (newFrame_g >= endFrame))
					{
						// now figure out what we are lerping between
						// delta is the fraction between this frame and the next, since the new anim is always at a .0f;
						lerp = float(endFrame+1)-newFrame_g;
						// frames are easy to calculate
						currentFrame = endFrame;
						assert(currentFrame>=0&&currentFrame<numFramesInFile);
						newFrame = bone.startFrame;
						assert(newFrame>=0&&newFrame<numFramesInFile);
					}
					else
					{
						if (newFrame_g <= endFrame+1)
						{
							newFrame_g=endFrame+fmod(newFrame_g-endFrame,animSize)-animSize;
						}
						// now figure out what we are lerping between
						// delta is the fraction between this frame and the next, since the new anim is always at a .0f;
						lerp = (ceil(newFrame_g)-newFrame_g);
						// frames are easy to calculate
						currentFrame = ceil(newFrame_g);
						assert(currentFrame>=0&&currentFrame<numFramesInFile);
						// should we be creating a virtual frame?
						if (currentFrame <= endFrame+1 )
						{
							newFrame = bone.startFrame;
							assert(newFrame>=0&&newFrame<numFramesInFile);
						}
						else
						{
							newFrame = currentFrame - 1;
							assert(newFrame>=0&&newFrame<numFramesInFile);
						}
					}
				}
				else
				{
					// should we be creating a virtual frame?
					if ((newFrame_g > endFrame - 1) && (newFrame_g < endFrame))
					{
						// now figure out what we are lerping between
						// delta is the fraction between this frame and the next, since the new anim is always at a .0f;
						lerp = (newFrame_g - (int)newFrame_g);
						// frames are easy to calculate
						currentFrame = (int)newFrame_g;
						assert(currentFrame>=0&&currentFrame<numFramesInFile);
						newFrame = bone.startFrame;
						assert(newFrame>=0&&newFrame<numFramesInFile);
					}
					else
					{
						if (newFrame_g >= endFrame)
						{
							newFrame_g=endFrame+fmod(newFrame_g-endFrame,animSize)-animSize;
						}
						// now figure out what we are lerping between
						// delta is the fraction between this frame and the next, since the new anim is always at a .0f;
						lerp = (newFrame_g - (int)newFrame_g);
						// frames are easy to calculate
						currentFrame = (int)newFrame_g;
						assert(currentFrame>=0&&currentFrame<numFramesInFile);
						// should we be creating a virtual frame?
						if (newFrame_g >= endFrame - 1)
						{
							newFrame = bone.startFrame;
							assert(newFrame>=0&&newFrame<numFramesInFile);
						}
						else
						{
							newFrame = currentFrame + 1;
							assert(newFrame>=0&&newFrame<numFramesInFile);
						}
					}
				}
				// sanity check
				assert (((newFrame < endFrame) && (newFrame >= bone.startFrame)) || (animSize < 10));
			}
			else
			{
				if (((bone.flags & (BONE_ANIM_OVERRIDE_FREEZE)) == (BONE_ANIM_OVERRIDE_FREEZE)))
				{
					// if we are supposed to reset the default anim, then do so
					if (animSpeed > 0.0f)
					{
						currentFrame = bone.endFrame - 1;
						assert(currentFrame>=0&&currentFrame<numFramesInFile);
					}
					else
					{
						currentFrame = bone.endFrame+1;
						assert(currentFrame>=0&&currentFrame<numFramesInFile);
					}

					newFrame = currentFrame;
					assert(newFrame>=0&&newFrame<numFramesInFile);
					lerp = 0;
				}
				else
				{
					bone.flags &= ~(BONE_ANIM_TOTAL);
				}

			}
		}
		else
		{
			if (animSpeed> 0.0)
			{
				// frames are easy to calculate
				currentFrame = (int)newFrame_g;

				// figure out the difference between the two frames	- we have to decide what frame and what percentage of that
				// frame we want to display
				lerp = (newFrame_g - currentFrame);

				assert(currentFrame>=0&&currentFrame<numFramesInFile);

				newFrame = currentFrame + 1;
				// are we now on the end frame?
				assert((int)endFrame<=numFramesInFile);
				if (newFrame >= (int)endFrame)
				{
					// we only want to lerp with the first frame of the anim if we are looping
					if (bone.flags & BONE_ANIM_OVERRIDE_LOOP)
					{
					  	newFrame = bone.startFrame;
						assert(newFrame>=0&&newFrame<numFramesInFile);
					}
					// if we intend to end this anim or freeze after this, then just keep on the last frame
					else
					{
						newFrame = bone.endFrame-1;
						assert(newFrame>=0&&newFrame<numFramesInFile);
					}
				}
				assert(newFrame>=0&&newFrame<numFramesInFile);
			}
			else
			{
				lerp = (ceil(newFrame_g)-newFrame_g);
				// frames are easy to calculate
				currentFrame = ceil(newFrame_g);
				if (currentFrame>bone.startFrame)
				{
					currentFrame=bone.startFrame;
					newFrame = currentFrame;
					lerp=0.0f;
				}
				else
				{
					newFrame=currentFrame-1;
					// are we now on the end frame?
					if (newFrame < endFrame+1)
					{
						// we only want to lerp with the first frame of the anim if we are looping
						if (bone.flags & BONE_ANIM_OVERRIDE_LOOP)
						{
					  		newFrame = bone.startFrame;
							assert(newFrame>=0&&newFrame<numFramesInFile);
						}
						// if we intend to end this anim or freeze after this, then just keep on the last frame
						else
						{
							newFrame = bone.endFrame+1;
							assert(newFrame>=0&&newFrame<numFramesInFile);
						}
					}
				}
				assert(currentFrame>=0&&currentFrame<numFramesInFile);
				assert(newFrame>=0&&newFrame<numFramesInFile);
			}
		}
	}
	else
	{
		if (animSpeed<0.0)
		{
			currentFrame = bone.endFrame+1;
		}
		else
		{
			currentFrame = bone.endFrame-1;
		}
		if (currentFrame<0)
		{
			currentFrame=0;
		}
		assert(currentFrame>=0&&currentFrame<numFramesInFile);
		newFrame = currentFrame;
		assert(newFrame>=0&&newFrame<numFramesInFile);
		lerp = 0;

	}
	assert(currentFrame>=0&&currentFrame<numFramesInFile);
	assert(newFrame>=0&&newFrame<numFramesInFile);
	assert(lerp>=0.0f&&lerp<=1.0f);
}

#ifdef _RAG_PRINT_TEST
void G2_RagPrintMatrix(mdxaBone_t *mat);
#endif
//basically construct a seperate skeleton with full hierarchy to store a matrix
//off which will give us the desired settling position given the frame in the skeleton
//that should be used -rww
int G2_Add_Bone (const model_t *mod, boneInfo_v &blist, const char *boneName);
int G2_Find_Bone(const model_t *mod, boneInfo_v &blist, const char *boneName);
void G2_RagGetAnimMatrix(CGhoul2Info &ghoul2, const int boneNum, mdxaBone_t &matrix, const int frame)
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
	skel = (mdxaSkel_t *)((byte *)ghoul2.mBoneCache->header + sizeof(mdxaHeader_t) + offsets->offsets[boneNum]);

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
		//recursively call to assure all parent matrices are set up
		G2_RagGetAnimMatrix(ghoul2, parent, matrix, frame);

		//assign the new skel ptr for our parent
		pskel = (mdxaSkel_t *)((byte *)ghoul2.mBoneCache->header + sizeof(mdxaHeader_t) + offsets->offsets[parent]);

		//taking bone matrix for the skeleton frame and parent's animFrameMatrix into account, determine our final animFrameMatrix
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

		assert(pbone.hasAnimFrameMatrix == frame); //this should have been calc'd in the recursive call

		Multiply_3x4Matrix(&bone.animFrameMatrix, &pbone.animFrameMatrix, &animMatrix);

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
	{ //root
		Multiply_3x4Matrix(&bone.animFrameMatrix, &ghoul2.mBoneCache->rootMatrix, &animMatrix);
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
		//bone.animFrameMatrix = ghoul2.mBoneCache->mFinalBones[boneNum].boneMatrix;
		//Maybe use this for the root, so that the orientation is in sync with the current
		//root matrix? However this would require constant recalculation of this base
		//skeleton which I currently do not want.
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

void G2_TransformBone (int child,CBoneCache &BC)
{
	SBoneCalc &TB=BC.mBones[child];
	static mdxaBone_t		tbone[6];
// 	mdxaFrame_t		*aFrame=0;
//	mdxaFrame_t		*bFrame=0;
//	mdxaFrame_t		*aoldFrame=0;
//	mdxaFrame_t		*boldFrame=0;
	static mdxaSkel_t		*skel;
	static mdxaSkelOffsets_t *offsets;
	boneInfo_v		&boneList = *BC.rootBoneList;
	static int				j, boneListIndex;
	int				angleOverride = 0;

#if DEBUG_G2_TIMING
	bool printTiming=false;
#endif
	// should this bone be overridden by a bone in the bone list?
	boneListIndex = G2_Find_Bone_In_List(boneList, child);
	if (boneListIndex != -1)
	{
		// we found a bone in the list - we need to override something here.

		// do we override the rotational angles?
		if ((boneList[boneListIndex].flags) & (BONE_ANGLES_TOTAL))
		{
			angleOverride = (boneList[boneListIndex].flags) & (BONE_ANGLES_TOTAL);
		}

		// set blending stuff if we need to
		if (boneList[boneListIndex].flags & BONE_ANIM_BLEND)
		{
			float blendTime = BC.incomingTime - boneList[boneListIndex].blendStart;
			// only set up the blend anim if we actually have some blend time left on this bone anim - otherwise we might corrupt some blend higher up the hiearchy
			if (blendTime>=0.0f&&blendTime < boneList[boneListIndex].blendTime)
			{
				TB.blendFrame	 = boneList[boneListIndex].blendFrame;
				TB.blendOldFrame = boneList[boneListIndex].blendLerpFrame;
				TB.blendLerp = (blendTime / boneList[boneListIndex].blendTime);
				TB.blendMode = true;
			}
			else
			{
				TB.blendMode = false;
			}
		}
		else if (/*r_Ghoul2NoBlend->integer||*/((boneList[boneListIndex].flags) & (BONE_ANIM_OVERRIDE_LOOP | BONE_ANIM_OVERRIDE)))
		// turn off blending if we are just doing a straing animation override
		{
			TB.blendMode = false;
		}

		// should this animation be overridden by an animation in the bone list?
		if ((boneList[boneListIndex].flags) & (BONE_ANIM_OVERRIDE_LOOP | BONE_ANIM_OVERRIDE))
		{
			G2_TimingModel(boneList[boneListIndex],BC.incomingTime,BC.header->numFrames,TB.currentFrame,TB.newFrame,TB.backlerp);
		}
#if DEBUG_G2_TIMING
		printTiming=true;
#endif
		/*
		if ((r_Ghoul2NoLerp->integer)||((boneList[boneListIndex].flags) & (BONE_ANIM_NO_LERP)))
		{
			TB.backlerp = 0.0f;
		}
		*/
		//rwwFIXMEFIXME: Use?
	}
	// figure out where the location of the bone animation data is
	assert(TB.newFrame>=0&&TB.newFrame<BC.header->numFrames);
	if (!(TB.newFrame>=0&&TB.newFrame<BC.header->numFrames))
	{
		TB.newFrame=0;
	}
//	aFrame = (mdxaFrame_t *)((byte *)BC.header + BC.header->ofsFrames + TB.newFrame * BC.frameSize );
	assert(TB.currentFrame>=0&&TB.currentFrame<BC.header->numFrames);
	if (!(TB.currentFrame>=0&&TB.currentFrame<BC.header->numFrames))
	{
		TB.currentFrame=0;
	}
//	aoldFrame = (mdxaFrame_t *)((byte *)BC.header + BC.header->ofsFrames + TB.currentFrame * BC.frameSize );

	// figure out where the location of the blended animation data is
	assert(!(TB.blendFrame < 0.0 || TB.blendFrame >= (BC.header->numFrames+1)));
	if (TB.blendFrame < 0.0 || TB.blendFrame >= (BC.header->numFrames+1) )
	{
		TB.blendFrame=0.0;
	}
//	bFrame = (mdxaFrame_t *)((byte *)BC.header + BC.header->ofsFrames + (int)TB.blendFrame * BC.frameSize );
	assert(TB.blendOldFrame>=0&&TB.blendOldFrame<BC.header->numFrames);
	if (!(TB.blendOldFrame>=0&&TB.blendOldFrame<BC.header->numFrames))
	{
		TB.blendOldFrame=0;
	}
#if DEBUG_G2_TIMING

#if DEBUG_G2_TIMING_RENDER_ONLY
	if (!HackadelicOnClient)
	{
		printTiming=false;
	}
#endif
	if (printTiming)
	{
		char mess[1000];
		if (TB.blendMode)
		{
			sprintf(mess,"b %2d %5d   %4d %4d %4d %4d  %f %f\n",boneListIndex,BC.incomingTime,(int)TB.newFrame,(int)TB.currentFrame,(int)TB.blendFrame,(int)TB.blendOldFrame,TB.backlerp,TB.blendLerp);
		}
		else
		{
			sprintf(mess,"a %2d %5d   %4d %4d            %f\n",boneListIndex,BC.incomingTime,TB.newFrame,TB.currentFrame,TB.backlerp);
		}
		OutputDebugString(mess);
		const boneInfo_t &bone=boneList[boneListIndex];
		if (bone.flags&BONE_ANIM_BLEND)
		{
			sprintf(mess,"                                                                    bfb[%2d] %5d  %5d  (%5d-%5d) %4.2f %4x   bt(%5d-%5d) %7.2f %5d\n",
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
				bone.blendLerpFrame
				);
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
				bone.flags
				);
		}
//		OutputDebugString(mess);
	}
#endif
//	boldFrame = (mdxaFrame_t *)((byte *)BC.header + BC.header->ofsFrames + TB.blendOldFrame * BC.frameSize );

//	mdxaCompBone_t	*compBonePointer = (mdxaCompBone_t *)((byte *)BC.header + BC.header->ofsCompBonePool);

	assert(child>=0&&child<BC.header->numBones);
//	assert(bFrame->boneIndexes[child]>=0);
//	assert(boldFrame->boneIndexes[child]>=0);
//	assert(aFrame->boneIndexes[child]>=0);
//	assert(aoldFrame->boneIndexes[child]>=0);

	// decide where the transformed bone is going

	// are we blending with another frame of anim?
	if (TB.blendMode)
	{
		float backlerp = TB.blendFrame - (int)TB.blendFrame;
		float frontlerp = 1.0 - backlerp;

// 		MC_UnCompress(tbone[3].matrix,compBonePointer[bFrame->boneIndexes[child]].Comp);
// 		MC_UnCompress(tbone[4].matrix,compBonePointer[boldFrame->boneIndexes[child]].Comp);
		UnCompressBone(tbone[3].matrix, child, BC.header, TB.blendFrame);
		UnCompressBone(tbone[4].matrix, child, BC.header, TB.blendOldFrame);

		for ( j = 0 ; j < 12 ; j++ )
		{
  			((float *)&tbone[5])[j] = (backlerp * ((float *)&tbone[3])[j])
				+ (frontlerp * ((float *)&tbone[4])[j]);
		}
	}

  	//
  	// lerp this bone - use the temp space on the ref entity to put the bone transforms into
  	//
  	if (!TB.backlerp)
  	{
// 		MC_UnCompress(tbone[2].matrix,compBonePointer[aoldFrame->boneIndexes[child]].Comp);
		UnCompressBone(tbone[2].matrix, child, BC.header, TB.currentFrame);

		// blend in the other frame if we need to
		if (TB.blendMode)
		{
			float blendFrontlerp = 1.0 - TB.blendLerp;
	  		for ( j = 0 ; j < 12 ; j++ )
			{
  				((float *)&tbone[2])[j] = (TB.blendLerp * ((float *)&tbone[2])[j])
					+ (blendFrontlerp * ((float *)&tbone[5])[j]);
			}
		}

  		if (!child)
		{
			// now multiply by the root matrix, so we can offset this model should we need to
			Multiply_3x4Matrix(&BC.mFinalBones[child].boneMatrix, &BC.rootMatrix, &tbone[2]);
 		}
  	}
	else
  	{
		float frontlerp = 1.0 - TB.backlerp;
// 		MC_UnCompress(tbone[0].matrix,compBonePointer[aFrame->boneIndexes[child]].Comp);
//		MC_UnCompress(tbone[1].matrix,compBonePointer[aoldFrame->boneIndexes[child]].Comp);
		UnCompressBone(tbone[0].matrix, child, BC.header, TB.newFrame);
		UnCompressBone(tbone[1].matrix, child, BC.header, TB.currentFrame);

		for ( j = 0 ; j < 12 ; j++ )
		{
  			((float *)&tbone[2])[j] = (TB.backlerp * ((float *)&tbone[0])[j])
				+ (frontlerp * ((float *)&tbone[1])[j]);
		}

		// blend in the other frame if we need to
		if (TB.blendMode)
		{
			float blendFrontlerp = 1.0 - TB.blendLerp;
	  		for ( j = 0 ; j < 12 ; j++ )
			{
  				((float *)&tbone[2])[j] = (TB.blendLerp * ((float *)&tbone[2])[j])
					+ (blendFrontlerp * ((float *)&tbone[5])[j]);
			}
		}

  		if (!child)
  		{
			// now multiply by the root matrix, so we can offset this model should we need to
			Multiply_3x4Matrix(&BC.mFinalBones[child].boneMatrix, &BC.rootMatrix, &tbone[2]);
  		}
	}
	// figure out where the bone hirearchy info is
	offsets = (mdxaSkelOffsets_t *)((byte *)BC.header + sizeof(mdxaHeader_t));
	skel = (mdxaSkel_t *)((byte *)BC.header + sizeof(mdxaHeader_t) + offsets->offsets[child]);
//	skel = BC.mSkels[child];
	//rww - removed mSkels

	int parent=BC.mFinalBones[child].parent;
	assert((parent==-1&&child==0)||(parent>=0&&parent<(int)BC.mBones.size()));
	if (angleOverride & BONE_ANGLES_REPLACE)
	{
		bool isRag=!!(angleOverride & BONE_ANGLES_RAGDOLL);
		if (!isRag)
		{ //do the same for ik.. I suppose.
			isRag = !!(angleOverride & BONE_ANGLES_IK);
		}

		mdxaBone_t &bone = BC.mFinalBones[child].boneMatrix;
		boneInfo_t &boneOverride = boneList[boneListIndex];

		if (isRag)
		{
			mdxaBone_t temp, firstPass;
			// give us the matrix the animation thinks we should have, so we can get the correct X&Y coors
			Multiply_3x4Matrix(&firstPass, &BC.mFinalBones[parent].boneMatrix, &tbone[2]);
			// this is crazy, we are gonna drive the animation to ID while we are doing post mults to compensate.
			Multiply_3x4Matrix(&temp,&firstPass, &skel->BasePoseMat);
			float	matrixScale = VectorLength((float*)&temp);
			static mdxaBone_t		toMatrix =
			{
				{
					{ 1.0f, 0.0f, 0.0f, 0.0f },
					{ 0.0f, 1.0f, 0.0f, 0.0f },
					{ 0.0f, 0.0f, 1.0f, 0.0f }
				}
			};
			toMatrix.matrix[0][0]=matrixScale;
			toMatrix.matrix[1][1]=matrixScale;
			toMatrix.matrix[2][2]=matrixScale;
			toMatrix.matrix[0][3]=temp.matrix[0][3];
			toMatrix.matrix[1][3]=temp.matrix[1][3];
			toMatrix.matrix[2][3]=temp.matrix[2][3];

 			Multiply_3x4Matrix(&temp, &toMatrix,&skel->BasePoseMatInv); //dest first arg

			float blendTime = BC.incomingTime - boneList[boneListIndex].boneBlendStart;
			float blendLerp = (blendTime / boneList[boneListIndex].boneBlendTime);
			if (blendLerp>0.0f)
			{
				// has started
				if (blendLerp>1.0f)
				{
					// done
//					Multiply_3x4Matrix(&bone, &BC.mFinalBones[parent].boneMatrix,&temp);
					memcpy (&bone,&temp, sizeof(mdxaBone_t));
				}
				else
				{
//					mdxaBone_t lerp;
					// now do the blend into the destination
					float blendFrontlerp = 1.0 - blendLerp;
	  				for ( j = 0 ; j < 12 ; j++ )
					{
  						((float *)&bone)[j] = (blendLerp * ((float *)&temp)[j])
							+ (blendFrontlerp * ((float *)&tbone[2])[j]);
					}
//					Multiply_3x4Matrix(&bone, &BC.mFinalBones[parent].boneMatrix,&lerp);
				}
			}
		}
		else
		{
			mdxaBone_t temp, firstPass;

			// give us the matrix the animation thinks we should have, so we can get the correct X&Y coors
			Multiply_3x4Matrix(&firstPass, &BC.mFinalBones[parent].boneMatrix, &tbone[2]);

			// are we attempting to blend with the base animation? and still within blend time?
			if (boneOverride.boneBlendTime && (((boneOverride.boneBlendTime + boneOverride.boneBlendStart) < BC.incomingTime)))
			{
				// ok, we are supposed to be blending. Work out lerp
				float blendTime = BC.incomingTime - boneList[boneListIndex].boneBlendStart;
				float blendLerp = (blendTime / boneList[boneListIndex].boneBlendTime);

				if (blendLerp <= 1)
				{
					if (blendLerp < 0)
					{
						assert(0);
					}

					// now work out the matrix we want to get *to* - firstPass is where we are coming *from*
					Multiply_3x4Matrix(&temp, &firstPass, &skel->BasePoseMat);

					float	matrixScale = VectorLength((float*)&temp);

					mdxaBone_t	newMatrixTemp;

					if (HackadelicOnClient)
					{
						for (int i=0; i<3;i++)
						{
							for(int x=0;x<3; x++)
							{
								newMatrixTemp.matrix[i][x] = boneOverride.newMatrix.matrix[i][x]*matrixScale;
							}
						}

						newMatrixTemp.matrix[0][3] = temp.matrix[0][3];
						newMatrixTemp.matrix[1][3] = temp.matrix[1][3];
						newMatrixTemp.matrix[2][3] = temp.matrix[2][3];
					}
					else
					{
						for (int i=0; i<3;i++)
						{
							for(int x=0;x<3; x++)
							{
								newMatrixTemp.matrix[i][x] = boneOverride.matrix.matrix[i][x]*matrixScale;
							}
						}

						newMatrixTemp.matrix[0][3] = temp.matrix[0][3];
						newMatrixTemp.matrix[1][3] = temp.matrix[1][3];
						newMatrixTemp.matrix[2][3] = temp.matrix[2][3];
					}

 					Multiply_3x4Matrix(&temp, &newMatrixTemp,&skel->BasePoseMatInv);

					// now do the blend into the destination
					float blendFrontlerp = 1.0 - blendLerp;
	  				for ( j = 0 ; j < 12 ; j++ )
					{
  						((float *)&bone)[j] = (blendLerp * ((float *)&temp)[j])
							+ (blendFrontlerp * ((float *)&firstPass)[j]);
					}
				}
				else
				{
					bone = firstPass;
				}
			}
			// no, so just override it directly
			else
			{

				Multiply_3x4Matrix(&temp,&firstPass, &skel->BasePoseMat);
				float	matrixScale = VectorLength((float*)&temp);

				mdxaBone_t	newMatrixTemp;

				if (HackadelicOnClient)
				{
					for (int i=0; i<3;i++)
					{
						for(int x=0;x<3; x++)
						{
							newMatrixTemp.matrix[i][x] = boneOverride.newMatrix.matrix[i][x]*matrixScale;
						}
					}

					newMatrixTemp.matrix[0][3] = temp.matrix[0][3];
					newMatrixTemp.matrix[1][3] = temp.matrix[1][3];
					newMatrixTemp.matrix[2][3] = temp.matrix[2][3];
				}
				else
				{
					for (int i=0; i<3;i++)
					{
						for(int x=0;x<3; x++)
						{
							newMatrixTemp.matrix[i][x] = boneOverride.matrix.matrix[i][x]*matrixScale;
						}
					}

					newMatrixTemp.matrix[0][3] = temp.matrix[0][3];
					newMatrixTemp.matrix[1][3] = temp.matrix[1][3];
					newMatrixTemp.matrix[2][3] = temp.matrix[2][3];
				}

 				Multiply_3x4Matrix(&bone, &newMatrixTemp,&skel->BasePoseMatInv);
			}
		}
	}
	else if (angleOverride & BONE_ANGLES_PREMULT)
	{
		if ((angleOverride&BONE_ANGLES_RAGDOLL) || (angleOverride&BONE_ANGLES_IK))
		{
			mdxaBone_t	tmp;
			if (!child)
			{
				if (HackadelicOnClient)
				{
					Multiply_3x4Matrix(&tmp, &BC.rootMatrix, &boneList[boneListIndex].newMatrix);
				}
				else
				{
					Multiply_3x4Matrix(&tmp, &BC.rootMatrix, &boneList[boneListIndex].matrix);
				}
			}
			else
			{
				if (HackadelicOnClient)
				{
					Multiply_3x4Matrix(&tmp, &BC.mFinalBones[parent].boneMatrix, &boneList[boneListIndex].newMatrix);
				}
				else
				{
					Multiply_3x4Matrix(&tmp, &BC.mFinalBones[parent].boneMatrix, &boneList[boneListIndex].matrix);
				}
			}
			Multiply_3x4Matrix(&BC.mFinalBones[child].boneMatrix,&tmp, &tbone[2]);
		}
		else
		{
			if (!child)
			{
				// use the in coming root matrix as our basis
				if (HackadelicOnClient)
				{
					Multiply_3x4Matrix(&BC.mFinalBones[child].boneMatrix, &BC.rootMatrix, &boneList[boneListIndex].newMatrix);
				}
				else
				{
					Multiply_3x4Matrix(&BC.mFinalBones[child].boneMatrix, &BC.rootMatrix, &boneList[boneListIndex].matrix);
				}
 			}
			else
			{
				// convert from 3x4 matrix to a 4x4 matrix
				if (HackadelicOnClient)
				{
					Multiply_3x4Matrix(&BC.mFinalBones[child].boneMatrix, &BC.mFinalBones[parent].boneMatrix, &boneList[boneListIndex].newMatrix);
				}
				else
				{
					Multiply_3x4Matrix(&BC.mFinalBones[child].boneMatrix, &BC.mFinalBones[parent].boneMatrix, &boneList[boneListIndex].matrix);
				}
			}
		}
	}
	else
	// now transform the matrix by it's parent, asumming we have a parent, and we aren't overriding the angles absolutely
	if (child)
	{
		Multiply_3x4Matrix(&BC.mFinalBones[child].boneMatrix, &BC.mFinalBones[parent].boneMatrix, &tbone[2]);
	}

	// now multiply our resulting bone by an override matrix should we need to
	if (angleOverride & BONE_ANGLES_POSTMULT)
	{
		mdxaBone_t	tempMatrix;
		memcpy (&tempMatrix,&BC.mFinalBones[child].boneMatrix, sizeof(mdxaBone_t));
		if (HackadelicOnClient)
		{
		  	Multiply_3x4Matrix(&BC.mFinalBones[child].boneMatrix, &tempMatrix, &boneList[boneListIndex].newMatrix);
		}
		else
		{
		  	Multiply_3x4Matrix(&BC.mFinalBones[child].boneMatrix, &tempMatrix, &boneList[boneListIndex].matrix);
		}
	}
	/*
	if (r_Ghoul2UnSqash->integer)
	{
		mdxaBone_t tempMatrix;
		Multiply_3x4Matrix(&tempMatrix,&BC.mFinalBones[child].boneMatrix, &skel->BasePoseMat);
		float maxl;
		maxl=VectorLength(&skel->BasePoseMat.matrix[0][0]);
		VectorNormalize(&tempMatrix.matrix[0][0]);
		VectorNormalize(&tempMatrix.matrix[1][0]);
		VectorNormalize(&tempMatrix.matrix[2][0]);

		VectorScale(&tempMatrix.matrix[0][0],maxl,&tempMatrix.matrix[0][0]);
		VectorScale(&tempMatrix.matrix[1][0],maxl,&tempMatrix.matrix[1][0]);
		VectorScale(&tempMatrix.matrix[2][0],maxl,&tempMatrix.matrix[2][0]);
		Multiply_3x4Matrix(&BC.mFinalBones[child].boneMatrix,&tempMatrix,&skel->BasePoseMatInv);
	}
	*/
	//rwwFIXMEFIXME: Care?

}

void G2_SetUpBolts( mdxaHeader_t *header, CGhoul2Info &ghoul2, mdxaBone_v &bonePtr, boltInfo_v &boltList)
{
	mdxaSkel_t		*skel;
	mdxaSkelOffsets_t *offsets;
	offsets = (mdxaSkelOffsets_t *)((byte *)header + sizeof(mdxaHeader_t));

	for (size_t i=0; i<boltList.size(); i++)
	{
		if (boltList[i].boneNumber != -1)
		{
			// figure out where the bone hirearchy info is
			skel = (mdxaSkel_t *)((byte *)header + sizeof(mdxaHeader_t) + offsets->offsets[boltList[i].boneNumber]);
			Multiply_3x4Matrix(&boltList[i].position, &bonePtr[boltList[i].boneNumber].second, &skel->BasePoseMat);
		}
	}
}

//rww - RAGDOLL_BEGIN
#define		GHOUL2_RAG_STARTED						0x0010
//rww - RAGDOLL_END
//rwwFIXMEFIXME: Move this into the stupid header or something.

void G2_TransformGhoulBones(boneInfo_v &rootBoneList,mdxaBone_t &rootMatrix, CGhoul2Info &ghoul2, int time,bool smooth=true)
{
#ifdef G2_PERFORMANCE_ANALYSIS
	G2PerformanceTimer_G2_TransformGhoulBones.Start();
	G2PerformanceCounter_G2_TransformGhoulBones++;
#endif

	/*
	model_t			*currentModel;
	model_t			*animModel;
	mdxaHeader_t	*aHeader;

	//currentModel = R_GetModelByHandle(RE_RegisterModel(ghoul2.mFileName));
	currentModel = R_GetModelByHandle(ghoul2.mModel);
	assert(currentModel);
	assert(currentModel->mdxm);

	animModel =  R_GetModelByHandle(currentModel->mdxm->animIndex);
	assert(animModel);
	aHeader = animModel->mdxa;
	assert(aHeader);
	*/
	model_t			*currentModel = (model_t *)ghoul2.currentModel;
	mdxaHeader_t	*aHeader = (mdxaHeader_t *)ghoul2.aHeader;


	assert(ghoul2.aHeader);
	assert(ghoul2.currentModel);
	assert(ghoul2.currentModel->mdxm);
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
	assert((int)ghoul2.mBoneCache->mBones.size()==aHeader->numBones);

	ghoul2.mBoneCache->mSmoothingActive=false;
	ghoul2.mBoneCache->mUnsquash=false;

	// master smoothing control
	if (HackadelicOnClient && smooth && !ri.Cvar_VariableIntegerValue( "dedicated" ))
	{
		ghoul2.mBoneCache->mLastTouch=ghoul2.mBoneCache->mLastLastTouch;
		/*
		float val=r_Ghoul2AnimSmooth->value;
		if (smooth&&val>0.0f&&val<1.0f)
		{
		//	if (HackadelicOnClient)
		//	{
				ghoul2.mBoneCache->mLastTouch=ghoul2.mBoneCache->mLastLastTouch;
		//	}

			ghoul2.mBoneCache->mSmoothFactor=val;
			ghoul2.mBoneCache->mSmoothingActive=true;
			if (r_Ghoul2UnSqashAfterSmooth->integer)
			{
				ghoul2.mBoneCache->mUnsquash=true;
			}
		}
		else
		{
			ghoul2.mBoneCache->mSmoothFactor=1.0f;
		}
		*/

		// master smoothing control
		float val=r_Ghoul2AnimSmooth->value;
		if (val>0.0f&&val<1.0f)
		{
			//if (ghoul2.mFlags&GHOUL2_RESERVED_FOR_RAGDOLL)
#if 1
			if(ghoul2.mFlags & GHOUL2_CRAZY_SMOOTH)
			{
				val = 0.9f;
			}
			else if(ghoul2.mFlags & GHOUL2_RAG_STARTED)
			{
				for (size_t k=0;k<rootBoneList.size();k++)
				{
					boneInfo_t &bone=rootBoneList[k];
					if (bone.flags&BONE_ANGLES_RAGDOLL)
					{
						if (bone.firstCollisionTime &&
							bone.firstCollisionTime>time-250 &&
							bone.firstCollisionTime<time)
						{
							val=0.9f;//(val+0.8f)/2.0f;
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
#endif

//			ghoul2.mBoneCache->mSmoothFactor=(val + 1.0f-pow(1.0f-val,50.0f/dif))/2.0f;  // meaningless formula
			ghoul2.mBoneCache->mSmoothFactor=val;  // meaningless formula
			ghoul2.mBoneCache->mSmoothingActive=true;

			if (r_Ghoul2UnSqashAfterSmooth->integer)
			{
				ghoul2.mBoneCache->mUnsquash=true;
			}
		}
	}
	else
	{
		ghoul2.mBoneCache->mSmoothFactor=1.0f;
	}

	ghoul2.mBoneCache->mCurrentTouch++;

//rww - RAGDOLL_BEGIN
	if (HackadelicOnClient)
	{
		ghoul2.mBoneCache->mLastLastTouch=ghoul2.mBoneCache->mCurrentTouch;
		ghoul2.mBoneCache->mCurrentTouchRender=ghoul2.mBoneCache->mCurrentTouch;
	}
	else
	{
		ghoul2.mBoneCache->mCurrentTouchRender=0;
	}
//rww - RAGDOLL_END

	ghoul2.mBoneCache->frameSize = 0;// can be deleted in new G2 format	//(size_t)( &((mdxaFrame_t *)0)->boneIndexes[ ghoul2.aHeader->numBones ] );

	ghoul2.mBoneCache->rootBoneList=&rootBoneList;
	ghoul2.mBoneCache->rootMatrix=rootMatrix;
	ghoul2.mBoneCache->incomingTime=time;

	SBoneCalc &TB=ghoul2.mBoneCache->Root();
	TB.newFrame=0;
	TB.currentFrame=0;
	TB.backlerp=0.0f;
	TB.blendFrame=0;
	TB.blendOldFrame=0;
	TB.blendMode=false;
	TB.blendLerp=0;

#ifdef G2_PERFORMANCE_ANALYSIS
	G2Time_G2_TransformGhoulBones += G2PerformanceTimer_G2_TransformGhoulBones.End();
#endif
}


#define MDX_TAG_ORIGIN 2

//======================================================================
//
// Surface Manipulation code


// We've come across a surface that's designated as a bolt surface, process it and put it in the appropriate bolt place
void G2_ProcessSurfaceBolt(mdxaBone_v &bonePtr, mdxmSurface_t *surface, int boltNum, boltInfo_v &boltList, surfaceInfo_t *surfInfo, model_t *mod)
{
 	mdxmVertex_t 	*v, *vert0, *vert1, *vert2;
 	matrix3_t		axes, sides;
 	float			pTri[3][3], d;
 	int				j, k;

	// now there are two types of tag surface - model ones and procedural generated types - lets decide which one we have here.
	if (surfInfo && surfInfo->offFlags == G2SURFACEFLAG_GENERATED)
	{
		int surfNumber = surfInfo->genPolySurfaceIndex & 0x0ffff;
		int	polyNumber = (surfInfo->genPolySurfaceIndex >> 16) & 0x0ffff;

		// find original surface our original poly was in.
		mdxmSurface_t	*originalSurf = (mdxmSurface_t *)G2_FindSurface((void*)mod, surfNumber, surfInfo->genLod);
		mdxmTriangle_t	*originalTriangleIndexes = (mdxmTriangle_t *)((byte*)originalSurf + originalSurf->ofsTriangles);

		// get the original polys indexes
		int index0 = originalTriangleIndexes[polyNumber].indexes[0];
		int index1 = originalTriangleIndexes[polyNumber].indexes[1];
		int index2 = originalTriangleIndexes[polyNumber].indexes[2];

		// decide where the original verts are

 		vert0 = (mdxmVertex_t *) ((byte *)originalSurf + originalSurf->ofsVerts);
		vert0+= index0;

 		vert1 = (mdxmVertex_t *) ((byte *)originalSurf + originalSurf->ofsVerts);
		vert1+= index1;

 		vert2 = (mdxmVertex_t *) ((byte *)originalSurf + originalSurf->ofsVerts);
		vert2+= index2;

		// clear out the triangle verts to be
 	   	VectorClear( pTri[0] );
 	   	VectorClear( pTri[1] );
 	   	VectorClear( pTri[2] );

//		mdxmWeight_t	*w;

		int *piBoneRefs = (int*) ((byte*)originalSurf + originalSurf->ofsBoneReferences);

		// now go and transform just the points we need from the surface that was hit originally
//		w = vert0->weights;
		float fTotalWeight = 0.0f;
		int iNumWeights = G2_GetVertWeights( vert0 );
 		for ( k = 0 ; k < iNumWeights ; k++ )
 		{
			int		iBoneIndex	= G2_GetVertBoneIndex( vert0, k );
			float	fBoneWeight	= G2_GetVertBoneWeight( vert0, k, fTotalWeight, iNumWeights );

 			pTri[0][0] += fBoneWeight * ( DotProduct( bonePtr[piBoneRefs[iBoneIndex]].second.matrix[0], vert0->vertCoords ) + bonePtr[piBoneRefs[iBoneIndex]].second.matrix[0][3] );
 			pTri[0][1] += fBoneWeight * ( DotProduct( bonePtr[piBoneRefs[iBoneIndex]].second.matrix[1], vert0->vertCoords ) + bonePtr[piBoneRefs[iBoneIndex]].second.matrix[1][3] );
 			pTri[0][2] += fBoneWeight * ( DotProduct( bonePtr[piBoneRefs[iBoneIndex]].second.matrix[2], vert0->vertCoords ) + bonePtr[piBoneRefs[iBoneIndex]].second.matrix[2][3] );
		}
//		w = vert1->weights;
		fTotalWeight = 0.0f;
		iNumWeights = G2_GetVertWeights( vert1 );
 		for ( k = 0 ; k < iNumWeights ; k++ )
 		{
			int		iBoneIndex	= G2_GetVertBoneIndex( vert1, k );
			float	fBoneWeight	= G2_GetVertBoneWeight( vert1, k, fTotalWeight, iNumWeights );

 			pTri[1][0] += fBoneWeight * ( DotProduct( bonePtr[piBoneRefs[iBoneIndex]].second.matrix[0], vert1->vertCoords ) + bonePtr[piBoneRefs[iBoneIndex]].second.matrix[0][3] );
 			pTri[1][1] += fBoneWeight * ( DotProduct( bonePtr[piBoneRefs[iBoneIndex]].second.matrix[1], vert1->vertCoords ) + bonePtr[piBoneRefs[iBoneIndex]].second.matrix[1][3] );
 			pTri[1][2] += fBoneWeight * ( DotProduct( bonePtr[piBoneRefs[iBoneIndex]].second.matrix[2], vert1->vertCoords ) + bonePtr[piBoneRefs[iBoneIndex]].second.matrix[2][3] );
		}
//		w = vert2->weights;
		fTotalWeight = 0.0f;
		iNumWeights = G2_GetVertWeights( vert2 );
 		for ( k = 0 ; k < iNumWeights ; k++ )
 		{
			int		iBoneIndex	= G2_GetVertBoneIndex( vert2, k );
			float	fBoneWeight	= G2_GetVertBoneWeight( vert2, k, fTotalWeight, iNumWeights );

 			pTri[2][0] += fBoneWeight * ( DotProduct( bonePtr[piBoneRefs[iBoneIndex]].second.matrix[0], vert2->vertCoords ) + bonePtr[piBoneRefs[iBoneIndex]].second.matrix[0][3] );
 			pTri[2][1] += fBoneWeight * ( DotProduct( bonePtr[piBoneRefs[iBoneIndex]].second.matrix[1], vert2->vertCoords ) + bonePtr[piBoneRefs[iBoneIndex]].second.matrix[1][3] );
 			pTri[2][2] += fBoneWeight * ( DotProduct( bonePtr[piBoneRefs[iBoneIndex]].second.matrix[2], vert2->vertCoords ) + bonePtr[piBoneRefs[iBoneIndex]].second.matrix[2][3] );
		}

   		vec3_t normal;
		vec3_t up;
		vec3_t right;
		vec3_t vec0, vec1;
		// work out baryCentricK
		float baryCentricK = 1.0 - (surfInfo->genBarycentricI + surfInfo->genBarycentricJ);

		// now we have the model transformed into model space, now generate an origin.
		boltList[boltNum].position.matrix[0][3] = (pTri[0][0] * surfInfo->genBarycentricI) + (pTri[1][0] * surfInfo->genBarycentricJ) + (pTri[2][0] * baryCentricK);
		boltList[boltNum].position.matrix[1][3] = (pTri[0][1] * surfInfo->genBarycentricI) + (pTri[1][1] * surfInfo->genBarycentricJ) + (pTri[2][1] * baryCentricK);
		boltList[boltNum].position.matrix[2][3] = (pTri[0][2] * surfInfo->genBarycentricI) + (pTri[1][2] * surfInfo->genBarycentricJ) + (pTri[2][2] * baryCentricK);

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

		CrossProduct( normal, up, right );
		// that's the up vector
		boltList[boltNum].position.matrix[0][2] = right[0];
		boltList[boltNum].position.matrix[1][2] = right[1];
		boltList[boltNum].position.matrix[2][2] = right[2];


	}
	// no, we are looking at a normal model tag
	else
	{
		int *piBoneRefs = (int*) ((byte*)surface + surface->ofsBoneReferences);

	 	// whip through and actually transform each vertex
 		v = (mdxmVertex_t *) ((byte *)surface + surface->ofsVerts);
 		for ( j = 0; j < 3; j++ )
 		{
// 			mdxmWeight_t	*w;

 			VectorClear( pTri[j] );
 //			w = v->weights;

			const int iNumWeights = G2_GetVertWeights( v );
			float fTotalWeight = 0.0f;
 			for ( k = 0 ; k < iNumWeights ; k++ )
 			{
				int		iBoneIndex	= G2_GetVertBoneIndex( v, k );
				float	fBoneWeight	= G2_GetVertBoneWeight( v, k, fTotalWeight, iNumWeights );

 				//bone = bonePtr + piBoneRefs[w->boneIndex];
 				pTri[j][0] += fBoneWeight * ( DotProduct( bonePtr[piBoneRefs[iBoneIndex]].second.matrix[0], v->vertCoords ) + bonePtr[piBoneRefs[iBoneIndex]].second.matrix[0][3] );
 				pTri[j][1] += fBoneWeight * ( DotProduct( bonePtr[piBoneRefs[iBoneIndex]].second.matrix[1], v->vertCoords ) + bonePtr[piBoneRefs[iBoneIndex]].second.matrix[1][3] );
 				pTri[j][2] += fBoneWeight * ( DotProduct( bonePtr[piBoneRefs[iBoneIndex]].second.matrix[2], v->vertCoords ) + bonePtr[piBoneRefs[iBoneIndex]].second.matrix[2][3] );
 			}

 			v++;// = (mdxmVertex_t *)&v->weights[/*v->numWeights*/surface->maxVertBoneWeights];
 		}

 		// clear out used arrays
 		memset( axes, 0, sizeof( axes ) );
 		memset( sides, 0, sizeof( sides ) );

 		// work out actual sides of the tag triangle
 		for ( j = 0; j < 3; j++ )
 		{
 			sides[j][0] = pTri[(j+1)%3][0] - pTri[j][0];
 			sides[j][1] = pTri[(j+1)%3][1] - pTri[j][1];
 			sides[j][2] = pTri[(j+1)%3][2] - pTri[j][2];
 		}

 		// do math trig to work out what the matrix will be from this triangle's translated position
 		VectorNormalize2( sides[iG2_TRISIDE_LONGEST], axes[0] );
 		VectorNormalize2( sides[iG2_TRISIDE_SHORTEST], axes[1] );

 		// project shortest side so that it is exactly 90 degrees to the longer side
 		d = DotProduct( axes[0], axes[1] );
 		VectorMA( axes[0], -d, axes[1], axes[0] );
 		VectorNormalize2( axes[0], axes[0] );

 		CrossProduct( sides[iG2_TRISIDE_LONGEST], sides[iG2_TRISIDE_SHORTEST], axes[2] );
 		VectorNormalize2( axes[2], axes[2] );

 		// set up location in world space of the origin point in out going matrix
 		boltList[boltNum].position.matrix[0][3] = pTri[MDX_TAG_ORIGIN][0];
 		boltList[boltNum].position.matrix[1][3] = pTri[MDX_TAG_ORIGIN][1];
 		boltList[boltNum].position.matrix[2][3] = pTri[MDX_TAG_ORIGIN][2];

 		// copy axis to matrix - do some magic to orient minus Y to positive X and so on so bolt on stuff is oriented correctly
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


// now go through all the generated surfaces that aren't included in the model surface hierarchy and create the correct bolt info for them
void G2_ProcessGeneratedSurfaceBolts(CGhoul2Info &ghoul2, mdxaBone_v &bonePtr, model_t *mod_t)
{
#ifdef G2_PERFORMANCE_ANALYSIS
	G2PerformanceTimer_G2_ProcessGeneratedSurfaceBolts.Start();
#endif
	// look through the surfaces off the end of the pre-defined model surfaces
	for (size_t i=0; i< ghoul2.mSlist.size(); i++)
	{
		// only look for bolts if we are actually a generated surface, and not just an overriden one
		if (ghoul2.mSlist[i].offFlags & G2SURFACEFLAG_GENERATED)
		{
	   		// well alrighty then. Lets see if there is a bolt that is attempting to use it
			int boltNum = G2_Find_Bolt_Surface_Num(ghoul2.mBltlist, i, G2SURFACEFLAG_GENERATED);
			// yes - ok, processing time.
			if (boltNum != -1)
			{
				G2_ProcessSurfaceBolt(bonePtr, NULL, boltNum, ghoul2.mBltlist, &ghoul2.mSlist[i], mod_t);
			}
		}
	}
#ifdef G2_PERFORMANCE_ANALYSIS
	G2Time_G2_ProcessGeneratedSurfaceBolts += G2PerformanceTimer_G2_ProcessGeneratedSurfaceBolts.End();
#endif
}

// Go through the model and deal with just the surfaces that are tagged as bolt on points - this is for the server side skeleton construction
void ProcessModelBoltSurfaces(int surfaceNum, surfaceInfo_v &rootSList,
					mdxaBone_v &bonePtr, model_t *currentModel, int lod, boltInfo_v &boltList)
{
#ifdef G2_PERFORMANCE_ANALYSIS
	G2PerformanceTimer_ProcessModelBoltSurfaces.Start();
#endif
	int			i;
	int			offFlags = 0;

	// back track and get the surfinfo struct for this surface
	mdxmSurface_t			*surface = (mdxmSurface_t *)G2_FindSurface((void *)currentModel, surfaceNum, 0);
	mdxmHierarchyOffsets_t	*surfIndexes = (mdxmHierarchyOffsets_t *)((byte *)currentModel->mdxm + sizeof(mdxmHeader_t));
	mdxmSurfHierarchy_t		*surfInfo = (mdxmSurfHierarchy_t *)((byte *)surfIndexes + surfIndexes->offsets[surface->thisSurfaceIndex]);

	// see if we have an override surface in the surface list
	surfaceInfo_t	*surfOverride = G2_FindOverrideSurface(surfaceNum, rootSList);

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
		ProcessModelBoltSurfaces(surfInfo->childIndexes[i], rootSList, bonePtr, currentModel, lod, boltList);
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

	// back track and get the surfinfo struct for this surface
	const mdxmSurface_t			*surface = (mdxmSurface_t *)G2_FindSurface((void *)CBL.currentModel, CBL.surfaceNum, 0);
	const mdxmHierarchyOffsets_t	*surfIndexes = (mdxmHierarchyOffsets_t *)((byte *)CBL.currentModel->mdxm + sizeof(mdxmHeader_t));
	const mdxmSurfHierarchy_t	*surfInfo = (mdxmSurfHierarchy_t *)((byte *)surfIndexes + surfIndexes->offsets[surface->thisSurfaceIndex]);
	const model_t				*mod_a = R_GetModelByHandle(CBL.currentModel->mdxm->animIndex);
	const mdxaSkelOffsets_t		*offsets = (mdxaSkelOffsets_t *)((byte *)mod_a->mdxa + sizeof(mdxaHeader_t));
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
 			skel = (mdxaSkel_t *)((byte *)mod_a->mdxa + sizeof(mdxaHeader_t) + offsets->offsets[iBoneIndex]);

			// for every child bone...
			for (j=0; j< skel->numChildren; j++)
			{
				// get the skel data struct for each child bone of the referenced bone
 				childSkel = (mdxaSkel_t *)((byte *)mod_a->mdxa + sizeof(mdxaHeader_t) + offsets->offsets[skel->children[j]]);

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
				skel = (mdxaSkel_t *)((byte *)mod_a->mdxa + sizeof(mdxaHeader_t) + offsets->offsets[iParentBone]);
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


// sort all the ghoul models in this list so if they go in reference order. This will ensure the bolt on's are attached to the right place
// on the previous model, since it ensures the model being attached to is built and rendered first.

// NOTE!! This assumes at least one model will NOT have a parent. If it does - we are screwed
static void G2_Sort_Models(CGhoul2Info_v &ghoul2, int * const modelList, int * const modelCount)
{
	int		startPoint, endPoint;
	int		i, boltTo, j;

	*modelCount = 0;

	// first walk all the possible ghoul2 models, and stuff the out array with those with no parents
	for (i=0; i<ghoul2.size();i++)
	{
		// have a ghoul model here?
		if (ghoul2[i].mModelindex == -1)
		{
			continue;
		}

		if (!ghoul2[i].mValid)
		{
			continue;
		}

		// are we attached to anything?
		if (ghoul2[i].mModelBoltLink == -1)
		{
			// no, insert us first
			modelList[(*modelCount)++] = i;
	 	}
	}

	startPoint = 0;
	endPoint = *modelCount;

	// now, using that list of parentless models, walk the descendant tree for each of them, inserting the descendents in the list
	while (startPoint != endPoint)
	{
		for (i=0; i<ghoul2.size(); i++)
		{
			// have a ghoul model here?
			if (ghoul2[i].mModelindex == -1)
			{
				continue;
			}

			if (!ghoul2[i].mValid)
			{
				continue;
			}

			// what does this model think it's attached to?
			if (ghoul2[i].mModelBoltLink != -1)
			{
				boltTo = (ghoul2[i].mModelBoltLink >> MODEL_SHIFT) & MODEL_AND;
				// is it any of the models we just added to the list?
				for (j=startPoint; j<endPoint; j++)
				{
					// is this my parent model?
					if (boltTo == modelList[j])
					{
						// yes, insert into list and exit now
						modelList[(*modelCount)++] = i;
						break;
					}
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
	assert(mod);
	assert(mod->mdxm);

	// point at first lod list
	byte	*current = (byte*)((intptr_t)mod->mdxm + (intptr_t)mod->mdxm->ofsLODs);
	int i;

	//walk the lods
	assert(lod>=0&&lod<mod->mdxm->numLODs);
	for (i=0; i<lod; i++)
	{
		mdxmLOD_t *lodData = (mdxmLOD_t *)current;
		current += lodData->ofsEnd;
	}

	// avoid the lod pointer data structure
	current += sizeof(mdxmLOD_t);

	mdxmLODSurfOffset_t *indexes = (mdxmLODSurfOffset_t *)current;
	// we are now looking at the offset array
	assert(index>=0&&index<mod->mdxm->numSurfaces);
	current += indexes->offsets[index];

	return (void *)current;
}

//#define G2EVALRENDER

// We've come across a surface that's designated as a bolt surface, process it and put it in the appropriate bolt place
void G2_ProcessSurfaceBolt2(CBoneCache &boneCache, const mdxmSurface_t *surface, int boltNum, boltInfo_v &boltList, const surfaceInfo_t *surfInfo, const model_t *mod,mdxaBone_t &retMatrix)
{
 	mdxmVertex_t 	*v, *vert0, *vert1, *vert2;
 	matrix3_t		axes, sides;
 	float			pTri[3][3], d;
 	int				j, k;

	// now there are two types of tag surface - model ones and procedural generated types - lets decide which one we have here.
	if (surfInfo && surfInfo->offFlags == G2SURFACEFLAG_GENERATED)
	{
		int surfNumber = surfInfo->genPolySurfaceIndex & 0x0ffff;
		int	polyNumber = (surfInfo->genPolySurfaceIndex >> 16) & 0x0ffff;

		// find original surface our original poly was in.
		mdxmSurface_t	*originalSurf = (mdxmSurface_t *)G2_FindSurface_BC(mod, surfNumber, surfInfo->genLod);
		mdxmTriangle_t	*originalTriangleIndexes = (mdxmTriangle_t *)((byte*)originalSurf + originalSurf->ofsTriangles);

		// get the original polys indexes
		int index0 = originalTriangleIndexes[polyNumber].indexes[0];
		int index1 = originalTriangleIndexes[polyNumber].indexes[1];
		int index2 = originalTriangleIndexes[polyNumber].indexes[2];

		// decide where the original verts are
 		vert0 = (mdxmVertex_t *) ((byte *)originalSurf + originalSurf->ofsVerts);
		vert0+=index0;

		vert1 = (mdxmVertex_t *) ((byte *)originalSurf + originalSurf->ofsVerts);
		vert1+=index1;

		vert2 = (mdxmVertex_t *) ((byte *)originalSurf + originalSurf->ofsVerts);
		vert2+=index2;

		// clear out the triangle verts to be
 	   	VectorClear( pTri[0] );
 	   	VectorClear( pTri[1] );
 	   	VectorClear( pTri[2] );
		int *piBoneReferences = (int*) ((byte*)originalSurf + originalSurf->ofsBoneReferences);

//		mdxmWeight_t	*w;

		// now go and transform just the points we need from the surface that was hit originally
//		w = vert0->weights;
		float fTotalWeight = 0.0f;
		int iNumWeights = G2_GetVertWeights( vert0 );
 		for ( k = 0 ; k < iNumWeights ; k++ )
 		{
			int		iBoneIndex	= G2_GetVertBoneIndex( vert0, k );
			float	fBoneWeight	= G2_GetVertBoneWeight( vert0, k, fTotalWeight, iNumWeights );

#ifdef G2EVALRENDER
			const mdxaBone_t &bone=boneCache.EvalRender(piBoneReferences[iBoneIndex]);
#else
			const mdxaBone_t &bone=boneCache.Eval(piBoneReferences[iBoneIndex]);
#endif

			pTri[0][0] += fBoneWeight * ( DotProduct( bone.matrix[0], vert0->vertCoords ) + bone.matrix[0][3] );
 			pTri[0][1] += fBoneWeight * ( DotProduct( bone.matrix[1], vert0->vertCoords ) + bone.matrix[1][3] );
 			pTri[0][2] += fBoneWeight * ( DotProduct( bone.matrix[2], vert0->vertCoords ) + bone.matrix[2][3] );
		}

//		w = vert1->weights;
		fTotalWeight = 0.0f;
		iNumWeights = G2_GetVertWeights( vert1 );
 		for ( k = 0 ; k < iNumWeights ; k++)
 		{
			int		iBoneIndex	= G2_GetVertBoneIndex( vert1, k );
			float	fBoneWeight	= G2_GetVertBoneWeight( vert1, k, fTotalWeight, iNumWeights );

#ifdef G2EVALRENDER
			const mdxaBone_t &bone=boneCache.EvalRender(piBoneReferences[iBoneIndex]);
#else
			const mdxaBone_t &bone=boneCache.Eval(piBoneReferences[iBoneIndex]);
#endif

 			pTri[1][0] += fBoneWeight * ( DotProduct( bone.matrix[0], vert1->vertCoords ) + bone.matrix[0][3] );
 			pTri[1][1] += fBoneWeight * ( DotProduct( bone.matrix[1], vert1->vertCoords ) + bone.matrix[1][3] );
 			pTri[1][2] += fBoneWeight * ( DotProduct( bone.matrix[2], vert1->vertCoords ) + bone.matrix[2][3] );
		}

//		w = vert2->weights;
		fTotalWeight = 0.0f;
		iNumWeights = G2_GetVertWeights( vert2 );
 		for ( k = 0 ; k < iNumWeights ; k++)
 		{
			int		iBoneIndex	= G2_GetVertBoneIndex( vert2, k );
			float	fBoneWeight	= G2_GetVertBoneWeight( vert2, k, fTotalWeight, iNumWeights );

#ifdef G2EVALRENDER
			const mdxaBone_t &bone=boneCache.EvalRender(piBoneReferences[iBoneIndex]);
#else
			const mdxaBone_t &bone=boneCache.Eval(piBoneReferences[iBoneIndex]);
#endif

 			pTri[2][0] += fBoneWeight * ( DotProduct( bone.matrix[0], vert2->vertCoords ) + bone.matrix[0][3] );
 			pTri[2][1] += fBoneWeight * ( DotProduct( bone.matrix[1], vert2->vertCoords ) + bone.matrix[1][3] );
 			pTri[2][2] += fBoneWeight * ( DotProduct( bone.matrix[2], vert2->vertCoords ) + bone.matrix[2][3] );
		}

   		vec3_t normal;
		vec3_t up;
		vec3_t right;
		vec3_t vec0, vec1;
		// work out baryCentricK
		float baryCentricK = 1.0 - (surfInfo->genBarycentricI + surfInfo->genBarycentricJ);

		// now we have the model transformed into model space, now generate an origin.
		retMatrix.matrix[0][3] = (pTri[0][0] * surfInfo->genBarycentricI) + (pTri[1][0] * surfInfo->genBarycentricJ) + (pTri[2][0] * baryCentricK);
		retMatrix.matrix[1][3] = (pTri[0][1] * surfInfo->genBarycentricI) + (pTri[1][1] * surfInfo->genBarycentricJ) + (pTri[2][1] * baryCentricK);
		retMatrix.matrix[2][3] = (pTri[0][2] * surfInfo->genBarycentricI) + (pTri[1][2] * surfInfo->genBarycentricJ) + (pTri[2][2] * baryCentricK);

		// generate a normal to this new triangle
		VectorSubtract(pTri[0], pTri[1], vec0);
		VectorSubtract(pTri[2], pTri[1], vec1);

		CrossProduct(vec0, vec1, normal);
		VectorNormalize(normal);

		// forward vector
		retMatrix.matrix[0][0] = normal[0];
		retMatrix.matrix[1][0] = normal[1];
		retMatrix.matrix[2][0] = normal[2];

		// up will be towards point 0 of the original triangle.
		// so lets work it out. Vector is hit point - point 0
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
 		v = (mdxmVertex_t *) ((byte *)surface + surface->ofsVerts);
		int *piBoneReferences = (int*) ((byte*)surface + surface->ofsBoneReferences);
 		for ( j = 0; j < 3; j++ )
 		{
// 			mdxmWeight_t	*w;

 			VectorClear( pTri[j] );
 //			w = v->weights;

			const int iNumWeights = G2_GetVertWeights( v );

			float fTotalWeight = 0.0f;
 			for ( k = 0 ; k < iNumWeights ; k++)
 			{
				int		iBoneIndex	= G2_GetVertBoneIndex( v, k );
				float	fBoneWeight	= G2_GetVertBoneWeight( v, k, fTotalWeight, iNumWeights );

#ifdef G2EVALRENDER
				const mdxaBone_t &bone=boneCache.EvalRender(piBoneReferences[iBoneIndex]);
#else
				const mdxaBone_t &bone=boneCache.Eval(piBoneReferences[iBoneIndex]);
#endif

 				pTri[j][0] += fBoneWeight * ( DotProduct( bone.matrix[0], v->vertCoords ) + bone.matrix[0][3] );
 				pTri[j][1] += fBoneWeight * ( DotProduct( bone.matrix[1], v->vertCoords ) + bone.matrix[1][3] );
 				pTri[j][2] += fBoneWeight * ( DotProduct( bone.matrix[2], v->vertCoords ) + bone.matrix[2][3] );
 			}

 			v++;// = (mdxmVertex_t *)&v->weights[/*v->numWeights*/surface->maxVertBoneWeights];
 		}

 		// clear out used arrays
 		memset( axes, 0, sizeof( axes ) );
 		memset( sides, 0, sizeof( sides ) );

 		// work out actual sides of the tag triangle
 		for ( j = 0; j < 3; j++ )
 		{
 			sides[j][0] = pTri[(j+1)%3][0] - pTri[j][0];
 			sides[j][1] = pTri[(j+1)%3][1] - pTri[j][1];
 			sides[j][2] = pTri[(j+1)%3][2] - pTri[j][2];
 		}

 		// do math trig to work out what the matrix will be from this triangle's translated position
 		VectorNormalize2( sides[iG2_TRISIDE_LONGEST], axes[0] );
 		VectorNormalize2( sides[iG2_TRISIDE_SHORTEST], axes[1] );

 		// project shortest side so that it is exactly 90 degrees to the longer side
 		d = DotProduct( axes[0], axes[1] );
 		VectorMA( axes[0], -d, axes[1], axes[0] );
 		VectorNormalize2( axes[0], axes[0] );

 		CrossProduct( sides[iG2_TRISIDE_LONGEST], sides[iG2_TRISIDE_SHORTEST], axes[2] );
 		VectorNormalize2( axes[2], axes[2] );

 		// set up location in world space of the origin point in out going matrix
 		retMatrix.matrix[0][3] = pTri[MDX_TAG_ORIGIN][0];
 		retMatrix.matrix[1][3] = pTri[MDX_TAG_ORIGIN][1];
 		retMatrix.matrix[2][3] = pTri[MDX_TAG_ORIGIN][2];

 		// copy axis to matrix - do some magic to orient minus Y to positive X and so on so bolt on stuff is oriented correctly
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

void G2_GetBoltMatrixLow(CGhoul2Info &ghoul2,int boltNum,const vec3_t scale,mdxaBone_t &retMatrix)
{
	if (!ghoul2.mBoneCache)
	{
		retMatrix=identityMatrix;
		return;
	}
	assert(ghoul2.mBoneCache);
	CBoneCache &boneCache=*ghoul2.mBoneCache;
	assert(boneCache.mod);
	boltInfo_v &boltList=ghoul2.mBltlist;

	if ( boltList.size() < 1 ) {
		retMatrix=identityMatrix;
		return;
	}

	assert(boltNum>=0&&boltNum<(int)boltList.size());
#if 0 //rwwFIXMEFIXME: Disable this before release!!!!!! I am just trying to find a crash bug.
	if (boltNum < 0 || boltNum >= boltList.size())
	{
		char fName[MAX_QPATH];
		char mName[MAX_QPATH];
		int bLink = ghoul2.mModelBoltLink;

		if (ghoul2.currentModel)
		{
			strcpy(mName, ghoul2.currentModel->name);
		}
		else
		{
			strcpy(mName, "NULL!");
		}

		if (ghoul2.mFileName && ghoul2.mFileName[0])
		{
			strcpy(fName, ghoul2.mFileName);
		}
		else
		{
			strcpy(fName, "None?!");
		}

		Com_Error(ERR_DROP, "Write down or save this error message, show it to Rich\nBad bolt index on model %s (filename %s), index %i boltlink %i\n", mName, fName, boltNum, bLink);
	}
#endif
	if (boltList[boltNum].boneNumber>=0)
	{
		mdxaSkel_t		*skel;
		mdxaSkelOffsets_t *offsets;
		offsets = (mdxaSkelOffsets_t *)((byte *)boneCache.header + sizeof(mdxaHeader_t));
		skel = (mdxaSkel_t *)((byte *)boneCache.header + sizeof(mdxaHeader_t) + offsets->offsets[boltList[boltNum].boneNumber]);
		Multiply_3x4Matrix(&retMatrix, (mdxaBone_t *)&boneCache.EvalUnsmooth(boltList[boltNum].boneNumber), &skel->BasePoseMat);
	}
	else if (boltList[boltNum].surfaceNumber>=0)
	{
		const surfaceInfo_t *surfInfo=0;
		{
			for (size_t i=0;i<ghoul2.mSlist.size();i++)
			{
				surfaceInfo_t &t=ghoul2.mSlist[i];
				if (t.surface==boltList[boltNum].surfaceNumber)
				{
					surfInfo=&t;
				}
			}
		}
		mdxmSurface_t *surface = 0;
		if (!surfInfo)
		{
			surface = (mdxmSurface_t *)G2_FindSurface_BC(boneCache.mod,boltList[boltNum].surfaceNumber, 0);
		}
		if (!surface&&surfInfo&&surfInfo->surface<10000)
		{
			surface = (mdxmSurface_t *)G2_FindSurface_BC(boneCache.mod,surfInfo->surface, 0);
		}
		G2_ProcessSurfaceBolt2(boneCache,surface,boltNum,boltList,surfInfo,(model_t *)boneCache.mod,retMatrix);
	}
	else
	{
		 // we have a bolt without a bone or surface, not a huge problem but we ought to at least clear the bolt matrix
		retMatrix=identityMatrix;
	}
}

static void RootMatrix(CGhoul2Info_v &ghoul2,int time,const vec3_t scale,mdxaBone_t &retMatrix)
{
	int i;
	for (i=0; i<ghoul2.size(); i++)
	{
		if (ghoul2[i].mModelindex != -1 && ghoul2[i].mValid)
		{
			if (ghoul2[i].mFlags & GHOUL2_NEWORIGIN)
			{
				mdxaBone_t bolt;
				mdxaBone_t		tempMatrix;

				G2_ConstructGhoulSkeleton(ghoul2,time,false,scale);
				G2_GetBoltMatrixLow(ghoul2[i],ghoul2[i].mNewOrigin,scale,bolt);
				tempMatrix.matrix[0][0]=1.0f;
				tempMatrix.matrix[0][1]=0.0f;
				tempMatrix.matrix[0][2]=0.0f;
				tempMatrix.matrix[0][3]=-bolt.matrix[0][3];
				tempMatrix.matrix[1][0]=0.0f;
				tempMatrix.matrix[1][1]=1.0f;
				tempMatrix.matrix[1][2]=0.0f;
				tempMatrix.matrix[1][3]=-bolt.matrix[1][3];
				tempMatrix.matrix[2][0]=0.0f;
				tempMatrix.matrix[2][1]=0.0f;
				tempMatrix.matrix[2][2]=1.0f;
				tempMatrix.matrix[2][3]=-bolt.matrix[2][3];
//				Inverse_Matrix(&bolt, &tempMatrix);
				Multiply_3x4Matrix(&retMatrix, &tempMatrix, (mdxaBone_t*)&identityMatrix);
				return;
			}
		}
	}
	retMatrix=identityMatrix;
}

#if 0
extern cvar_t	*r_shadowRange;
static inline bool bInShadowRange(vec3_t location)
{
	const float c = DotProduct( tr.viewParms.ori.axis[0], tr.viewParms.ori.origin );
	const float dist = DotProduct( tr.viewParms.ori.axis[0], location ) - c;

//	return (dist < tr.distanceCull/1.5f);
	return (dist < r_shadowRange->value);
}
#endif

/*
==============
R_AddGHOULSurfaces
==============
*/
void R_AddGhoulSurfaces( trRefEntity_t *ent ) {
}

#ifdef _G2_LISTEN_SERVER_OPT
qboolean G2API_OverrideServerWithClientData(CGhoul2Info_v& ghoul2, int modelIndex);
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
G2_ConstructGhoulSkeleton - builds a complete skeleton for all ghoul models in a CGhoul2Info_v class	- using LOD 0
==============
*/
void G2_ConstructGhoulSkeleton( CGhoul2Info_v &ghoul2,const int frameNum,bool checkForNewOrigin,const vec3_t scale)
{
#ifdef G2_PERFORMANCE_ANALYSIS
	G2PerformanceTimer_G2_ConstructGhoulSkeleton.Start();
#endif
	int				i, j;
	int				modelCount;
	mdxaBone_t		rootMatrix;

	int modelList[256];
	assert(ghoul2.size()<=255);
	modelList[255]=548;

	if (checkForNewOrigin)
	{
		RootMatrix(ghoul2,frameNum,scale,rootMatrix);
	}
	else
	{
		rootMatrix = identityMatrix;
	}

	G2_Sort_Models(ghoul2, modelList, &modelCount);
	assert(modelList[255]==548);

	for (j=0; j<modelCount; j++)
	{
		// get the sorted model to play with
		i = modelList[j];

		if (ghoul2[i].mValid)
		{
			if (j&&ghoul2[i].mModelBoltLink != -1)
			{
				int	boltMod = (ghoul2[i].mModelBoltLink >> MODEL_SHIFT) & MODEL_AND;
				int	boltNum = (ghoul2[i].mModelBoltLink >> BOLT_SHIFT) & BOLT_AND;

				mdxaBone_t bolt;
				G2_GetBoltMatrixLow(ghoul2[boltMod],boltNum,scale,bolt);
				G2_TransformGhoulBones(ghoul2[i].mBlist,bolt,ghoul2[i],frameNum,checkForNewOrigin);
			}
#ifdef _G2_LISTEN_SERVER_OPT
			else if (ghoul2[i].entityNum == ENTITYNUM_NONE || ghoul2[i].mSkelFrameNum != frameNum)
#else
			else
#endif
			{
				G2_TransformGhoulBones(ghoul2[i].mBlist,rootMatrix,ghoul2[i],frameNum,checkForNewOrigin);
			}
		}
	}
#ifdef G2_PERFORMANCE_ANALYSIS
	G2Time_G2_ConstructGhoulSkeleton += G2PerformanceTimer_G2_ConstructGhoulSkeleton.End();
#endif
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


qboolean R_LoadMDXM( model_t *mod, void *buffer, const char *mod_name, qboolean &bAlreadyCached ) {
	int					i,l, j;
	mdxmHeader_t		*pinmodel, *mdxm;
	mdxmLOD_t			*lod;
	mdxmSurface_t		*surf;
	int					version;
	int					size;
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
		Com_Printf (S_COLOR_YELLOW  "R_LoadMDXM: %s has wrong version (%i should be %i)\n",
				 mod_name, version, MDXM_VERSION);
		return qfalse;
	}

	mod->type	   = MOD_MDXM;
	mod->dataSize += size;

	qboolean bAlreadyFound = qfalse;
	mdxm = mod->mdxm = (mdxmHeader_t*) //Hunk_Alloc( size );
										RE_RegisterModels_Malloc(size, buffer, mod_name, &bAlreadyFound, TAG_MODEL_GLM);

	assert(bAlreadyCached == bAlreadyFound);

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
		surfInfo->shaderIndex = 0;
		RE_RegisterModels_StoreShaderRequest(mod_name, &surfInfo->shader[0], &surfInfo->shaderIndex);

		// find the next surface
		surfInfo = (mdxmSurfHierarchy_t *)( (byte *)surfInfo + (size_t)( &((mdxmSurfHierarchy_t *)0)->childIndexes[ surfInfo->numChildren ] ));
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
				Com_Error (ERR_DROP, "R_LoadMDXM: %s has more than %i verts on a surface (%i)",
					mod_name, SHADER_MAX_VERTEXES, surf->numVerts );
			}
			if ( surf->numTriangles*3 > SHADER_MAX_INDEXES ) {
				Com_Error (ERR_DROP, "R_LoadMDXM: %s has more than %i triangles on a surface (%i)",
					mod_name, SHADER_MAX_INDEXES / 3, surf->numTriangles );
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

			if (isAnOldModelFile)
			{
				int *boneRef = (int *) ( (byte *)surf + surf->ofsBoneReferences );
				for ( j = 0 ; j < surf->numBoneReferences ; j++ )
				{
					assert(boneRef[j] >= 0 && boneRef[j] < 72);
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
	//Move the whole thing down CHILD_PADDING amount in memory, clear the new preceding space, and increment the end pointer.

	i = boneIndex+1;

	//Now add CHILD_PADDING amount to every offset beginning at the offset of the bone that was moved.
	while (i < mdxa->numBones)
	{
		offsets->offsets[i] += CHILD_PADDING;
		i++;
	}

	mdxa->ofsFrames += CHILD_PADDING;
	mdxa->ofsCompBonePool += CHILD_PADDING;
	mdxa->ofsEnd += CHILD_PADDING;
	//ofsSkel does not need to be updated because we are only moving memory after that point.
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
int BoneParentChildIndex(mdxaHeader_t *mdxa, mdxaSkelOffsets_t *offsets, mdxaSkel_t *boneInfo, qboolean child)
{
	int i = 0;
	int matchindex = -1;
	mdxaSkel_t *bone;
	const char *match = NULL;

	while (BoneHierarchyList[i])
	{
		if (!Q_stricmp(boneInfo->name, BoneHierarchyList[i]))
		{ //we have a match, the slot above this will be our desired parent. (or below for child)
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
qboolean R_LoadMDXA( model_t *mod, void *buffer, const char *mod_name, qboolean &bAlreadyCached ) {

	mdxaHeader_t		*pinmodel, *mdxa;
	int					version;
	int					size;
#ifdef CREATE_LIMB_HIERARCHY
	int					oSize = 0;
	byte				*sizeMarker;
#endif

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
		Com_Printf (S_COLOR_YELLOW  "R_LoadMDXA: %s has wrong version (%i should be %i)\n",
				 mod_name, version, MDXA_VERSION);
		return qfalse;
	}

	mod->type		= MOD_MDXA;
	mod->dataSize  += size;

	qboolean bAlreadyFound = qfalse;

#ifdef CREATE_LIMB_HIERARCHY
	oSize = size;

	int childNumber = (NUM_ROOTPARENTS + NUM_OTHERPARENTS);
	size += (childNumber*(CHILD_PADDING*8)); //Allocate us some extra space so we can shift memory down.
#endif //CREATE_LIMB_HIERARCHY

	mdxa = mod->mdxa = (mdxaHeader_t*) //Hunk_Alloc( size );
										RE_RegisterModels_Malloc(size,
										#ifdef CREATE_LIMB_HIERARCHY
											NULL,	// I think this'll work, can't really test on PC
										#else
											buffer,
										#endif
										mod_name, &bAlreadyFound, TAG_MODEL_GLA);

	assert(bAlreadyCached == bAlreadyFound);	// I should probably eliminate 'bAlreadyFound', but wtf?

	if (!bAlreadyFound)
	{
#ifdef CREATE_LIMB_HIERARCHY
		memcpy( mdxa, buffer, oSize );
#else
		// horrible new hackery, if !bAlreadyFound then we've just done a tag-morph, so we need to set the
		//	bool reference passed into this function to true, to tell the caller NOT to do an ri.FS_Freefile since
		//	we've hijacked that memory block...
		//
		// Aaaargh. Kill me now...
		//
		bAlreadyCached = qtrue;
		assert( mdxa == buffer );
//		memcpy( mdxa, buffer, size );	// and don't do this now, since it's the same thing
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
		mdxaSkel_t			*boneParent;
#if 0 //#ifdef _M_IX86
		mdxaSkel_t			*boneInfo;
		int i, k;
#endif

		sizeMarker = (byte *)mdxa + mdxa->ofsEnd;

		//rww - This is probably temporary until we put actual hierarchy in for the models.
		//It is necessary for the correct operation of ragdoll.
   		mdxaSkelOffsets_t *offsets = (mdxaSkelOffsets_t *)((byte *)mdxa + sizeof(mdxaHeader_t));

		for ( i = 0 ; i < mdxa->numBones ; i++)
		{
			boneInfo = (mdxaSkel_t *)((byte *)mdxa + sizeof(mdxaHeader_t) + offsets->offsets[i]);

			if (boneInfo)
			{
				char *bname = boneInfo->name;

				if (BoneIsRootParent(bname))
				{ //These are the main parent bones. We don't want to change their parents, but we want to give them children.
					ShiftMemoryDown(offsets, mdxa, i, &sizeMarker);

					boneInfo = (mdxaSkel_t *)((byte *)mdxa + sizeof(mdxaHeader_t) + offsets->offsets[i]);

					int newChild = BoneParentChildIndex(mdxa, offsets, boneInfo, qtrue);

					if (newChild != -1)
					{
						boneInfo->numChildren++;
						boneInfo->children[boneInfo->numChildren-1] = newChild;
					}
					else
					{
						assert(!"Failed to find matching child for bone in hierarchy creation");
					}
				}
				else if (BoneIsOtherParent(bname) || BoneIsBottom(bname))
				{
					if (!BoneIsBottom(bname))
					{ //unless it's last in the chain it has the next bone as a child.
						ShiftMemoryDown(offsets, mdxa, i, &sizeMarker);

						boneInfo = (mdxaSkel_t *)((byte *)mdxa + sizeof(mdxaHeader_t) + offsets->offsets[i]);

						int newChild = BoneParentChildIndex(mdxa, offsets, boneInfo, qtrue);

						if (newChild != -1)
						{
							boneInfo->numChildren++;
							boneInfo->children[boneInfo->numChildren-1] = newChild;
						}
						else
						{
							assert(!"Failed to find matching child for bone in hierarchy creation");
						}
					}

					//Before we set the parent we want to remove this as a child for whoever was parenting it.
					int oldParent = boneInfo->parent;

					if (oldParent > -1)
					{
						boneParent = (mdxaSkel_t *)((byte *)mdxa + sizeof(mdxaHeader_t) + offsets->offsets[oldParent]);
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
							{ //this bone is the child
								k++;
								while (k < boneParent->numChildren)
								{
									boneParent->children[k-1] = boneParent->children[k];
									k++;
								}
								boneParent->children[k-1] = 0;
								boneParent->numChildren--;
								break;
							}
							k++;
						}
					}

					//Now that we have cleared the original parent of ownership, mark the bone's new parent.
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
#endif //CREATE_LIMB_HIERARCHY

 	if ( mdxa->numFrames < 1 ) {
		Com_Printf (S_COLOR_YELLOW  "R_LoadMDXA: %s has no frames\n", mod_name );
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
		boneInfo += (size_t)( &((mdxaSkel_t *)0)->children[ boneInfo->numChildren ] );
	}


	// swap all the frames
	frameSize = (size_t)( &((mdxaFrame_t *)0)->bones[ mdxa->numBones ] );
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







