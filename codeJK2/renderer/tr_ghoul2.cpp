// leave this as first line for PCH reasons...
//
#include "../server/exe_headers.h"


 
#include "../client/client.h"	//FIXME!! EVIL - just include the definitions needed 
#include "../client/vmachine.h"

						  
#if !defined(TR_LOCAL_H)
	#include "tr_local.h"
#endif

#include "MatComp.h"
#if !defined(_QCOMMON_H_)
	#include "../qcommon/qcommon.h"
#endif
#if !defined(G2_H_INC)
	#include "../ghoul2/G2.h"
#endif

#define	LL(x) x=LittleLong(x)

extern	cvar_t	*r_Ghoul2UnSqash;
extern	cvar_t	*r_Ghoul2AnimSmooth;
extern	cvar_t	*r_Ghoul2NoLerp;
extern	cvar_t	*r_Ghoul2NoBlend;
extern	cvar_t	*r_Ghoul2UnSqashAfterSmooth;

bool HackadelicOnClient=false; // means this is a render traversal

// I hate doing this, but this is the simplest way to get this into the routines it needs to be
mdxaBone_t		worldMatrix;
mdxaBone_t		worldMatrixInv;

const static mdxaBone_t		identityMatrix = 
{ 
	0.0f, -1.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f
};

class CTransformBone
{
public:
	int				touch; // for minimal recalculation
	mdxaBone_t		boneMatrix; //final matrix
	int				parent; // only set once
	CTransformBone()
	{
		touch=0;
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
	bool EvalLow(int index)
	{
		assert(index>=0&&index<mBones.size());
		if (mFinalBones[index].touch!=mCurrentTouch)
		{
			// need to evaluate the bone
			assert((mFinalBones[index].parent>=0&&mFinalBones[index].parent<mFinalBones.size())||(index==0&&mFinalBones[index].parent==-1));
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
			return true;
		}
		return false;
	}
public:
	int					frameSize; 
	const mdxaHeader_t	*header; 
	const model_t		*mod; 

	// these are split for better cpu cache behavior
	vector<SBoneCalc> mBones;
	vector<CTransformBone> mFinalBones;

	vector<CTransformBone> mSmoothBones; // for render smoothing
	vector<mdxaSkel_t *>   mSkels;

	boneInfo_v		*rootBoneList;
	mdxaBone_t		rootMatrix;
	int				incomingTime;

	int				mCurrentTouch;

	// for render smoothing
	bool			mSmoothingActive;
	bool			mUnsquash;
	float			mSmoothFactor;
	int				mWraithID; // this is just used for debug prints, can use it for any int of interest in JK2

	CBoneCache(const model_t *amod,const mdxaHeader_t *aheader) :
		mod(amod),
		header(aheader)
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
		mSkels.resize(numBones);
		mdxaSkelOffsets_t *offsets;
		mdxaSkel_t		*skel;
		offsets = (mdxaSkelOffsets_t *)((byte *)header + sizeof(mdxaHeader_t));

		int i;
		for (i=0;i<numBones;i++)
		{
			skel = (mdxaSkel_t *)((byte *)header + sizeof(mdxaHeader_t) + offsets->offsets[i]);
			mSkels[i]=skel;
			mFinalBones[i].parent=skel->parent;
		}
		mCurrentTouch=3;
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
	}
};

void RemoveBoneCache(CBoneCache *boneCache)
{
	delete boneCache;
}

const mdxaBone_t &EvalBoneCache(int index,CBoneCache *boneCache)
{
	assert(boneCache);
	return boneCache->Eval(index);
}


class CRenderSurface
{
public:
	int				surfaceNum;
	surfaceInfo_v	&rootSList;
	const shader_t		*cust_shader;
	int				fogNum;
	qboolean		personalModel;
	CBoneCache		*boneCache;
	int				renderfx;
	const skin_t	*skin;
	const model_t	*currentModel;
	int				lod;
	boltInfo_v		&boltList;

	CRenderSurface(
		int				initsurfaceNum,
		surfaceInfo_v	&initrootSList,
		const shader_t		*initcust_shader,
		int				initfogNum,
		qboolean		initpersonalModel,
		CBoneCache		*initboneCache,
		int				initrenderfx,
		const skin_t			*initskin,
		const model_t			*initcurrentModel,
		int				initlod,
		boltInfo_v		&initboltList):

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
	boltList(initboltList)
	{}
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
  	switch ( R_CullLocalPointAndRadius( NULL,  ent->e.radius * largestScale) )
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

	int				i;
	fog_t			*fog;

	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL ) {
		return 0;
	}

	if (tr.refdef.doLAGoggles)
	{
		return tr.world->numfogs;
	}

	int partialFog = 0;
	for ( i = 1 ; i < tr.world->numfogs ; i++ ) {
		fog = &tr.world->fogs[i];
		if ( ent->e.origin[0] - ent->e.radius >= fog->bounds[0][0] 
			&& ent->e.origin[0] + ent->e.radius <= fog->bounds[1][0] 
			&& ent->e.origin[1] - ent->e.radius >= fog->bounds[0][1]
			&& ent->e.origin[1] + ent->e.radius <= fog->bounds[1][1] 
			&& ent->e.origin[2] - ent->e.radius >= fog->bounds[0][2]
			&& ent->e.origin[2] + ent->e.radius <= fog->bounds[1][2] ) 
		{//totally inside it
			return i;
			break;
		}
		if ( ( ent->e.origin[0] - ent->e.radius >= fog->bounds[0][0] && ent->e.origin[1] - ent->e.radius >= fog->bounds[0][1] && ent->e.origin[2] - ent->e.radius >= fog->bounds[0][2] &&
			ent->e.origin[0] - ent->e.radius <= fog->bounds[1][0] && ent->e.origin[1] - ent->e.radius <= fog->bounds[1][1] && ent->e.origin[2] - ent->e.radius <= fog->bounds[1][2] ) ||
			( ent->e.origin[0] + ent->e.radius >= fog->bounds[0][0] && ent->e.origin[1] + ent->e.radius >= fog->bounds[0][1] && ent->e.origin[2] + ent->e.radius >= fog->bounds[0][2] &&
			ent->e.origin[0] + ent->e.radius <= fog->bounds[1][0] && ent->e.origin[1] + ent->e.radius <= fog->bounds[1][1] && ent->e.origin[2] + ent->e.radius <= fog->bounds[1][2] ) )
		{//partially inside it
			if ( tr.refdef.fogIndex == i || R_FogParmsMatch( tr.refdef.fogIndex, i ) )
			{//take new one only if it's the same one that the viewpoint is in
				return i;
				break;
			}
			else if ( !partialFog )
			{//first partialFog
				partialFog = i;
			}
		}
	}
	//if nothing else, use the first partial fog you found
	return partialFog;
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

	// leave these two together!!!! (note the 'else')...
	//
#ifdef _NPATCH
	if (currentModel->npatchable && r_ati_pn_triangles->integer )
	{
		lodBias = max(lodBias,r_ati_pn_triangles->integer-1);
	}
	else
#endif
	if (r_lodbias->integer > lodBias) 
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

	if ( ( projectedRadius = ProjectRadius( 0.75*largestScale*ent->e.radius, ent->e.origin ) ) != 0 )	//we reduce the radius to make the LOD match other model types which use the actual bound box size
 	{
 		lodscale = r_lodscale->value;
 		if (lodscale > 20) lodscale = 20;	 
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


void Multiply_3x4Matrix(mdxaBone_t *out,const  mdxaBone_t *in2,const mdxaBone_t *in) 
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

static int G2_GetBonePoolIndex(	const mdxaHeader_t *pMDXAHeader, int iFrame, int iBone)
{
	assert(iFrame>=0&&iFrame<pMDXAHeader->numFrames);
	assert(iBone>=0&&iBone<pMDXAHeader->numBones);
	const int iOffsetToIndex = (iFrame * pMDXAHeader->numBones * 3) + (iBone * 3);

	mdxaIndex_t *pIndex = (mdxaIndex_t *) ((byte*) pMDXAHeader + pMDXAHeader->ofsFrames + iOffsetToIndex);

	return pIndex->iIndex & 0x00FFFFFF;	// this will cause problems for big-endian machines... ;-)
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
				assert ((newFrame < endFrame) && (newFrame >= bone.startFrame) || (animSize < 10)); 
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

// transform each individual bone's information - making sure to use any override information provided, both for angles and for animations, as
// well as multiplying each bone's matrix by it's parents matrix 
void G2_TransformBone (int child,CBoneCache &BC)
{
	SBoneCalc &TB=BC.mBones[child];
	mdxaBone_t		tbone[6];
// 	mdxaFrame_t		*aFrame=0;
//	mdxaFrame_t		*bFrame=0;
//	mdxaFrame_t		*aoldFrame=0;
//	mdxaFrame_t		*boldFrame=0;
	mdxaSkel_t		*skel;
	mdxaSkelOffsets_t *offsets;
	boneInfo_v		&boneList = *BC.rootBoneList;
	int				j, boneListIndex;
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
		else if (r_Ghoul2NoBlend->integer||((boneList[boneListIndex].flags) & (BONE_ANIM_OVERRIDE_LOOP | BONE_ANIM_OVERRIDE)))
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
		if ((r_Ghoul2NoLerp->integer)||((boneList[boneListIndex].flags) & (BONE_ANIM_NO_LERP)))
		{
			TB.backlerp = 0.0f;
		}
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

	int parent=BC.mFinalBones[child].parent;
	assert((parent==-1&&child==0)||(parent>=0&&parent<BC.mBones.size()));
	if (angleOverride & BONE_ANGLES_REPLACE)
	{
		mdxaBone_t temp, firstPass;
		mdxaBone_t &bone = BC.mFinalBones[child].boneMatrix;
		boneInfo_t &boneOverride = boneList[boneListIndex];
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
	else if (angleOverride & BONE_ANGLES_PREMULT)
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

}



// start the recursive hirearchial bone transform and lerp process for this model
void G2_TransformGhoulBones(boneInfo_v &rootBoneList,mdxaBone_t &rootMatrix, CGhoul2Info &ghoul2, int time,bool smooth=true)
{
	assert(ghoul2.aHeader);
	assert(ghoul2.currentModel);
	assert(ghoul2.currentModel->mdxm);
	if (!ghoul2.aHeader->numBones)
	{
		assert(0); // this would be strange
		return;
	}
	if (!ghoul2.mBoneCache)
	{
		ghoul2.mBoneCache=new CBoneCache(ghoul2.currentModel,ghoul2.aHeader);
	}
	ghoul2.mBoneCache->mod=ghoul2.currentModel;
	ghoul2.mBoneCache->header=ghoul2.aHeader; 
	assert(ghoul2.mBoneCache->mBones.size()==ghoul2.aHeader->numBones);

	ghoul2.mBoneCache->mSmoothingActive=false;
	ghoul2.mBoneCache->mUnsquash=false;

	// master smoothing control
	float val=r_Ghoul2AnimSmooth->value;
	if (smooth&&val>0.0f&&val<1.0f)
	{
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
	ghoul2.mBoneCache->mCurrentTouch++;
	ghoul2.mBoneCache->mWraithID=0;
	ghoul2.mBoneCache->frameSize = 0;// can be deleted in new G2 format	//(int)( &((mdxaFrame_t *)0)->boneIndexes[ ghoul2.aHeader->numBones ] );   

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

}


#define MDX_TAG_ORIGIN 2

//======================================================================
//
// Surface Manipulation code 


// We've come across a surface that's designated as a bolt surface, process it and put it in the appropriate bolt place
void G2_ProcessSurfaceBolt2(CBoneCache &boneCache, const mdxmSurface_t *surface, int boltNum, boltInfo_v &boltList, const surfaceInfo_t *surfInfo, const model_t *mod,mdxaBone_t &retMatrix)
{
 	mdxmVertex_t 	*v, *vert0, *vert1, *vert2;
 	vec3_t			axes[3], sides[3];
 	float			pTri[3][3], d;
 	int				j, k;

	// now there are two types of tag surface - model ones and procedural generated types - lets decide which one we have here.
	if (surfInfo && surfInfo->offFlags == G2SURFACEFLAG_GENERATED)
	{
		int surfNumber = surfInfo->genPolySurfaceIndex & 0x0ffff;
		int	polyNumber = (surfInfo->genPolySurfaceIndex >> 16) & 0x0ffff;

		// find original surface our original poly was in.
		mdxmSurface_t	*originalSurf = (mdxmSurface_t *)G2_FindSurface(mod, surfNumber, surfInfo->genLod);
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

			const mdxaBone_t &bone=boneCache.Eval(piBoneReferences[iBoneIndex]);

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

			const mdxaBone_t &bone=boneCache.Eval(piBoneReferences[iBoneIndex]);

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

			const mdxaBone_t &bone=boneCache.Eval(piBoneReferences[iBoneIndex]);

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

				const mdxaBone_t &bone=boneCache.Eval(piBoneReferences[iBoneIndex]);

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
	assert(boltNum>=0&&boltNum<boltList.size());
	if (boltList[boltNum].boneNumber>=0)
	{
		mdxaSkel_t		*skel;
		mdxaSkelOffsets_t *offsets;
		offsets = (mdxaSkelOffsets_t *)((byte *)boneCache.header + sizeof(mdxaHeader_t));
		skel = (mdxaSkel_t *)((byte *)boneCache.header + sizeof(mdxaHeader_t) + offsets->offsets[boltList[boltNum].boneNumber]);
		Multiply_3x4Matrix(&retMatrix, &boneCache.EvalUnsmooth(boltList[boltNum].boneNumber), &skel->BasePoseMat);
	}
	else if (boltList[boltNum].surfaceNumber>=0)
	{
		const surfaceInfo_t *surfInfo=0;
		{
			int i;
			for (i=0;i<ghoul2.mSlist.size();i++)
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
			surface = (mdxmSurface_t *)G2_FindSurface(boneCache.mod,boltList[boltNum].surfaceNumber, 0);
		}
		if (!surface&&surfInfo&&surfInfo->surface<10000)
		{
			surface = (mdxmSurface_t *)G2_FindSurface(boneCache.mod,surfInfo->surface, 0);
		}
		G2_ProcessSurfaceBolt2(boneCache,surface,boltNum,boltList,surfInfo,(model_t *)boneCache.mod,retMatrix);
	}
	else
	{
		 // we have a bolt without a bone or surface, not a huge problem but we ought to at least clear the bolt matrix
		retMatrix=identityMatrix;
	}
}


// set up each surface ready for rendering in the back end
void RenderSurfaces(CRenderSurface &RS)
{
	int			i;
	const shader_t	*shader = 0;
	int			offFlags = 0;
	

	assert(RS.currentModel);
	assert(RS.currentModel->mdxm);
	// back track and get the surfinfo struct for this surface
	mdxmSurface_t			*surface = (mdxmSurface_t *)G2_FindSurface(RS.currentModel, RS.surfaceNum, RS.lod);
	mdxmHierarchyOffsets_t	*surfIndexes = (mdxmHierarchyOffsets_t *)((byte *)RS.currentModel->mdxm + sizeof(mdxmHeader_t));
	mdxmSurfHierarchy_t		*surfInfo = (mdxmSurfHierarchy_t *)((byte *)surfIndexes + surfIndexes->offsets[surface->thisSurfaceIndex]);
	
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
					shader = RS.skin->surfaces[j]->shader;
					break;
				}
			}
		} 
		else 
		{
			shader = R_GetShaderByHandle( surfInfo->shaderIndex );
		}
		// we will add shadows even if the main object isn't visible in the view

		// stencil shadows can't do personal models unless I polyhedron clip
		if ( !RS.personalModel
			&& r_shadows->integer == 2 
			&& RS.fogNum == 0
			&& (RS.renderfx & RF_SHADOW_PLANE )
			&& !(RS.renderfx & ( RF_NOSHADOW | RF_DEPTHHACK ) ) 
			&& shader->sort == SS_OPAQUE ) 
		{		// set the surface info to point at the where the transformed bone list is going to be for when the surface gets rendered out
			CRenderableSurface *newSurf = new CRenderableSurface;
			newSurf->surfaceData = surface;
			newSurf->boneCache = RS.boneCache;
#ifdef _NPATCH
			R_AddDrawSurf( (surfaceType_t *)newSurf, tr.shadowShader, 0, qfalse, (RS.currentModel->npatchable ? 1 : 0) );
#else
			R_AddDrawSurf( (surfaceType_t *)newSurf, tr.shadowShader, 0, qfalse );
#endif // _NPATCH
		}

		// projection shadows work fine with personal models
		if ( r_shadows->integer == 3
			&& RS.fogNum == 0
			&& (RS.renderfx & RF_SHADOW_PLANE )
			&& shader->sort == SS_OPAQUE ) 
		{		// set the surface info to point at the where the transformed bone list is going to be for when the surface gets rendered out
			CRenderableSurface *newSurf = new CRenderableSurface;
			newSurf->surfaceData = surface;
			newSurf->boneCache = RS.boneCache;
#ifdef _NPATCH
			R_AddDrawSurf( (surfaceType_t *)newSurf, tr.projectionShadowShader, 0, qfalse, (RS.currentModel->npatchable ? 1 : 0) );
#else
			R_AddDrawSurf( (surfaceType_t *)newSurf, tr.projectionShadowShader, 0, qfalse );
#endif // _NPATCH
		}

		// don't add third_person objects if not viewing through a portal
		if ( !RS.personalModel ) 
		{		// set the surface info to point at the where the transformed bone list is going to be for when the surface gets rendered out
			CRenderableSurface *newSurf = new CRenderableSurface;
			newSurf->surfaceData = surface;
			newSurf->boneCache = RS.boneCache;
#ifdef _NPATCH
			R_AddDrawSurf( (surfaceType_t *)newSurf, shader, RS.fogNum, qfalse, (RS.currentModel->npatchable ? 1 : 0) );
#else
			R_AddDrawSurf( (surfaceType_t *)newSurf, shader, RS.fogNum, qfalse );
#endif // _NPATCH
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
		RenderSurfaces(RS);
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
		if (ghoul2[i].mModelindex == -1||!ghoul2[i].mValid)
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
			if (ghoul2[i].mModelindex == -1||!ghoul2[i].mValid)
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



static void RootMatrix(CGhoul2Info_v &ghoul2,int time,const vec3_t scale,mdxaBone_t &retMatrix)
{
	int i;
	for (i=0; i<ghoul2.size(); i++)
	{
		if (ghoul2[i].mModelindex != -1&&ghoul2[i].mValid)
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

/*
==============
R_AddGHOULSurfaces
==============
*/
void R_AddGhoulSurfaces( trRefEntity_t *ent ) {
	shader_t		*cust_shader = 0;
	int				fogNum = 0;
	qboolean		personalModel;
	int				cull;
	int				i, whichLod, j;
	skin_t			*skin;
	int				modelCount;
	mdxaBone_t		rootMatrix;
	CGhoul2Info_v	&ghoul2 = *ent->e.ghoul2;

	if ( !ghoul2.IsValid() )
	{
		return;
	}
	// if we don't want server ghoul2 models and this is one, or we just don't want ghoul2 models at all, then return
	if (r_noGhoul2->integer)
	{
		return;
	}
	if (!G2_SetupModelPointers(ghoul2))
	{
		return;
	}

	int currentTime=G2API_GetTime(tr.refdef.time);


	// cull the entire model if merged bounding box of both frames
	// is outside the view frustum.
	cull = R_GCullModel (ent );
	if ( cull == CULL_OUT ) 
	{
		return;
	}
	HackadelicOnClient=true;
	// are any of these models setting a new origin?
	RootMatrix(ghoul2,currentTime, ent->e.modelScale,rootMatrix);

   	// don't add third_person objects if not in a portal
	personalModel = (qboolean)((ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal);

	int modelList[256];
	assert(ghoul2.size()<=255);
	modelList[255]=548;

	// set up lighting now that we know we aren't culled
	if ( !personalModel || r_shadows->integer > 1 ) 
	{
		// FIXME!! Is there something here we should be looking at?
		R_SetupEntityLighting( &tr.refdef, ent );
	}

	// see if we are in a fog volume
	fogNum = R_GComputeFogNum( ent );

	// order sort the ghoul 2 models so bolt ons get bolted to the right model
	G2_Sort_Models(ghoul2, modelList, &modelCount);
	assert(modelList[255]==548);

	// construct a world matrix for this entity
	G2_GenerateWorldMatrix(ent->e.angles, ent->e.origin);

	// walk each possible model for this entity and try rendering it out
	for (j=0; j<modelCount; j++)
	{
		i = modelList[j];
		if (ghoul2[i].mValid&&!(ghoul2[i].mFlags & GHOUL2_NOMODEL)&&!(ghoul2[i].mFlags & GHOUL2_NORENDER))
		{
			//
			// figure out whether we should be using a custom shader for this model
			//
			skin = NULL;
			if (ent->e.customShader)
			{
				cust_shader = R_GetShaderByHandle(ent->e.customShader );
			}
			else
			{
				cust_shader = NULL;
				// figure out the custom skin thing
				if (ent->e.customSkin)
				{
					skin = R_GetSkinByHandle(ent->e.customSkin );
				}
				else if ( ghoul2[i].mSkin > 0 && ghoul2[i].mSkin < tr.numSkins ) 
				{
					skin = R_GetSkinByHandle( ghoul2[i].mSkin );
				}
			}

			if (j&&ghoul2[i].mModelBoltLink != -1)
			{
				int	boltMod = (ghoul2[i].mModelBoltLink >> MODEL_SHIFT) & MODEL_AND;
				int	boltNum = (ghoul2[i].mModelBoltLink >> BOLT_SHIFT) & BOLT_AND;
				mdxaBone_t bolt;
				G2_GetBoltMatrixLow(ghoul2[boltMod],boltNum,ent->e.modelScale,bolt);
				G2_TransformGhoulBones(ghoul2[i].mBlist,bolt, ghoul2[i],currentTime);
			}
			else
			{
				G2_TransformGhoulBones(ghoul2[i].mBlist, rootMatrix, ghoul2[i],currentTime);
			}
			whichLod = G2_ComputeLOD( ent, ghoul2[i].currentModel, ghoul2[i].mLodBias );
			G2_FindOverrideSurface(-1,ghoul2[i].mSlist); //reset the quick surface override lookup;
			CRenderSurface RS(ghoul2[i].mSurfaceRoot, ghoul2[i].mSlist, cust_shader, fogNum, personalModel, ghoul2[i].mBoneCache, ent->e.renderfx, skin,ghoul2[i].currentModel, whichLod, ghoul2[i].mBltlist);
			RenderSurfaces(RS);
		}
	}
	HackadelicOnClient=false;
}	

bool G2_NeedsRecalc(CGhoul2Info *ghlInfo,int frameNum)
{
	G2_SetupModelPointers(ghlInfo);
	// not sure if I still need this test, probably
	if (ghlInfo->mSkelFrameNum!=frameNum||
		!ghlInfo->mBoneCache||
		ghlInfo->mBoneCache->mod!=ghlInfo->currentModel)
	{
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
			else
			{
				G2_TransformGhoulBones(ghoul2[i].mBlist,rootMatrix,ghoul2[i],frameNum,checkForNewOrigin);
			}
		}
	}
}	


/*
==============
RB_SurfaceGhoul
==============
*/
void RB_SurfaceGhoul( CRenderableSurface *surf ) {
	int				 j, k;

	// grab the pointer to the surface info within the loaded mesh file
	mdxmSurface_t	*surface = (mdxmSurface_t *)surf->surfaceData;

	// point us at the bone structure that should have been pre-computed
	CBoneCache *bones = surf->boneCache;

	delete surf;
	//
	// deform the vertexes by the lerped bones
	//
   
	// first up, sanity check our numbers
	RB_CheckOverflow( surface->numVerts, surface->numTriangles );

	// now copy the right number of verts to the temporary area for verts for this shader
	const int baseVertex = tess.numVertexes;
	const int *triangles = (int *) ((byte *)surface + surface->ofsTriangles);
	const int baseIndex = tess.numIndexes;
#if 0
	const int indexes = surface->numTriangles * 3;
	for (j = 0 ; j < indexes ; j++) {
		tess.indexes[baseIndex + j] = baseVertex + triangles[j];
	}
	tess.numIndexes += indexes;
#else
	const int indexes = surface->numTriangles; //*3;	//unrolled 3 times, don't multiply
	unsigned int * tessIndexes = &tess.indexes[baseIndex];
	for (j = 0 ; j < indexes ; j++) {
		*tessIndexes++ = baseVertex + *triangles++;
		*tessIndexes++ = baseVertex + *triangles++;
		*tessIndexes++ = baseVertex + *triangles++;
	}
	tess.numIndexes += indexes*3;
#endif

	
	int *piBoneReferences = (int*) ((byte*)surface + surface->ofsBoneReferences);
	const int numVerts = surface->numVerts;
	const mdxmVertex_t 	*v = (mdxmVertex_t *) ((byte *)surface + surface->ofsVerts);
	mdxmVertexTexCoord_t *pTexCoords = (mdxmVertexTexCoord_t *) &v[numVerts];

	int baseVert = tess.numVertexes;
	for ( j = 0; j < numVerts; j++, baseVert++ ) 
	{
		const int iNumWeights = G2_GetVertWeights( v );
//		const mdxmWeight_t	*w = v->weights;
		const mdxaBone_t *bone;

		VectorClear( tess.xyz[baseVert]);
		VectorClear( tess.normal[baseVert]);
		float fTotalWeight = 0.0f;
		for (k = 0 ; k < iNumWeights ; k++) 
		{
			int		iBoneIndex	= G2_GetVertBoneIndex( v, k );
			float	fBoneWeight	= G2_GetVertBoneWeight( v, k, fTotalWeight, iNumWeights );

			bone = &bones->Eval(piBoneReferences[iBoneIndex]);

			tess.xyz[baseVert][0] += fBoneWeight * ( DotProduct( bone->matrix[0], v->vertCoords ) + bone->matrix[0][3] );
			tess.xyz[baseVert][1] += fBoneWeight * ( DotProduct( bone->matrix[1], v->vertCoords ) + bone->matrix[1][3] );
			tess.xyz[baseVert][2] += fBoneWeight * ( DotProduct( bone->matrix[2], v->vertCoords ) + bone->matrix[2][3] );

			tess.normal[baseVert][0] += fBoneWeight * DotProduct( bone->matrix[0], v->normal );
			tess.normal[baseVert][1] += fBoneWeight * DotProduct( bone->matrix[1], v->normal );
			tess.normal[baseVert][2] += fBoneWeight * DotProduct( bone->matrix[2], v->normal );
		}

		tess.texCoords[baseVert][0][0] = pTexCoords[j].texCoords[0];
		tess.texCoords[baseVert][0][1] = pTexCoords[j].texCoords[1];	

		v++;// = (mdxmVertex_t *)&v->weights[/*v->numWeights*/surface->maxVertBoneWeights];
	}

	tess.numVertexes += surface->numVerts;
}

#ifdef _NPATCH
/*
=================
Problem with n-patches is that all normals should be shared. In case of hard edges,
where normals are not shared visible cracks will appear. The idea is to patch
model dynamically at the load time by inserting degenerate triangle strips at hard edges.
=================
*/

// Max number of degenerate triangles. Make it even!
#define MAX_DEGEN_TRIANGLES 1000
// Threshold for comparing vector values
#define VEC3_THRESH 0.01f

/*
=================
VectorCompareThresh - Compare vectors with some threshold
=================
*/
inline int VectorCompareThresh( const vec3_t v1, const vec3_t v2 ) {
	if ( v1[0] > v2[0]+VEC3_THRESH || v1[0] < v2[0]-VEC3_THRESH
		|| v1[1] > v2[1]+VEC3_THRESH || v1[1] < v2[1]-VEC3_THRESH
		|| v1[2] > v2[2]+VEC3_THRESH || v1[2] < v2[2]-VEC3_THRESH ) {
		return 0;
	}			
	return 1;
}

// fixme: optimise this out sometime, it's pointless now
inline mdxmVertex_t *GetSurfaceVertex( mdxmSurface_t *surf, int vert )
{
	int vertSize = sizeof(mdxmVertex_t);// + sizeof(mdxmWeight_t) * (surf->maxVertBoneWeights - 1);

	return (mdxmVertex_t *) ((byte *)surf + surf->ofsVerts + vert * vertSize);
}

/*
=================
R_SurfFillCreases - Generate a list of crease fillers (degenerate triangles)
Returns number of new degenerate triangles and pointer to the triangle data.

!!! Very inefficient implementation for now.
=================
*/
#if 0
static mdxmTriangle_t *R_SurfFillCreases( mdxmSurface_t *surf, int *triCount )
{
	mdxmTriangle_t *triFix;
	mdxmTriangle_t *tri = (mdxmTriangle_t *) ( (byte *)surf + surf->ofsTriangles );
	int numTriangles = surf->numTriangles;
	int numNewTriangles = 0;
	int i, j;

	// Alloc enough space for extra triangles
	triFix = (mdxmTriangle_t *) Z_Malloc(sizeof(mdxmTriangle_t) * MAX_DEGEN_TRIANGLES, TAG_ATI_NPATCH, qfalse);

	// For each triangle find if any other triangle shares an edge
	for ( i = 0; i < numTriangles; i++ ) 
	{
		mdxmTriangle_t *t1 = &tri[i];

		for ( j = i + 1; j < numTriangles; j++) 
		{
			mdxmTriangle_t *t2 = &tri[j];
			int edge;
			int tri1v1, tri1v2, tri2v1, tri2v2;
			int k;
			mdxmVertex_t *vert11, *vert12, *vert21, *vert22;

			for ( edge = 0; edge < 3; edge++ ) 
			{
				if ( edge == 0 ) 
				{
					tri1v1 = 0;
					tri1v2 = 1;
				}
				else if ( edge == 1 ) 
				{
					tri1v1 = 1;
					tri1v2 = 2;
				}
				else 
				{
					tri1v1 = 2;
					tri1v2 = 0;
				}

				vert11 = GetSurfaceVertex(surf, t1->indexes[tri1v1]);
				vert12 = GetSurfaceVertex(surf, t1->indexes[tri1v2]);

				tri2v1 = -1;
				tri2v2 = -1;
				vert21 = NULL;
				vert22 = NULL;

				// Search for the first matching edge vertex
				for ( k = 0; k < 3; k++ ) 
				{
					vert21 = GetSurfaceVertex(surf, t2->indexes[k]);
					if ( VectorCompareThresh(vert11->vertCoords, vert21->vertCoords) )
					{
						// Found first matching vertex
						tri2v1 = k;
						break;
					}

				}

				// Search for the second matching edge vertex
				for ( k = 0; k < 3; k++ ) 
				{
					vert22 = GetSurfaceVertex(surf, t2->indexes[k]);
					if ( VectorCompareThresh(vert12->vertCoords, vert22->vertCoords) ) 
					{
						// Found second matching vertex
						tri2v2 = k;
						break;
					}

				}

				if ( ( tri2v1 != -1 ) && ( tri2v2 != -1 ) && ( tri2v1 != tri2v2 ) ) {
					// Add crease fillers if edge is shared and normals are not
					if ( numNewTriangles >= MAX_DEGEN_TRIANGLES - 1 ) {
						// Not enough space to store all new triangles
						Z_Free(triFix);
						*triCount = 0;
						return NULL;
					}
					if ( !VectorCompare(vert11->normal, vert21->normal) ) {
						triFix[numNewTriangles].indexes[0] = t1->indexes[tri1v2];
						triFix[numNewTriangles].indexes[1] = t1->indexes[tri1v1];
						triFix[numNewTriangles].indexes[2] = t2->indexes[tri2v1];
						numNewTriangles++;
					}
					if ( !VectorCompare(vert12->normal, vert22->normal) ) {
						triFix[numNewTriangles].indexes[0] = t1->indexes[tri1v2];
						triFix[numNewTriangles].indexes[1] = t2->indexes[tri2v1];
						triFix[numNewTriangles].indexes[2] = t2->indexes[tri2v2];
						numNewTriangles++;
					}
					break;
				}
			}
		}
	}

	if ( numNewTriangles > 0) {
		// Return triangles
		*triCount = numNewTriangles;
		return triFix;
	}

	// Nothing to return
	Z_Free(triFix);
	*triCount = 0;
	return NULL;
}
#endif
/*
=================
R_LoadAndPatchMDXM - load and patch a Ghoul 2 Mesh file
=================
*/
qboolean R_LoadAndPatchMDXM( model_t *mod, void *buffer, const char *mod_name, qboolean bAlreadyCached )
{
#if 0
	int					version;
	int					size;
	int					tempSize;
	byte				*tempBuffer, *tempPtr;
	mdxmHeader_t		*pinmodel, *mdxm;
	mdxmLOD_t			*fileLod, *tempLod;
	mdxmLODSurfOffset_t	*fileSurfOffs, *tempSurfOffs;
	mdxmSurface_t		*fileSurf, *tempSurf;
	int					*fileBoneRef, *tempBoneRef;
	mdxmVertex_t		*fileVert, *tempVert;
	mdxmTriangle_t		*fileTri, *tempTri, *patchTri;
	int					i, l, j, k;
	shader_t			*sh;
	mdxmSurfHierarchy_t	*surfInfo;

	pinmodel= (mdxmHeader_t *)buffer;

	// We're not chached...
	version  = LittleLong(pinmodel->version);
	size     = LittleLong(pinmodel->ofsEnd);

	if (version != MDXM_VERSION) {
		ri.Printf( PRINT_WARNING, "R_LoadAndPatchMDXM: %s has wrong version (%i should be %i)\n",
				 mod_name, version, MDXM_VERSION);
		return qfalse;
	}

	// Temp patched buffer size -- make big enough to patch all surfaces for all LODs
	tempSize = size + LittleLong(pinmodel->numSurfaces) * LittleLong(pinmodel->numLODs) * MAX_DEGEN_TRIANGLES * sizeof(mdxmTriangle_t);
	// Allocate buffer for patched model
	tempPtr = tempBuffer = (byte *) Z_Malloc(tempSize, TAG_ATI_NPATCH, qfalse);

	//
	// Start patching into a temp buffer
	//

	// Copy everything up to the LOD data
	memcpy( tempBuffer, buffer, LittleLong(pinmodel->ofsLODs) );

	mdxm = (mdxmHeader_t *)tempBuffer;
	LL(mdxm->ident);
	LL(mdxm->version);
	LL(mdxm->numLODs);
	LL(mdxm->ofsLODs);
	LL(mdxm->numSurfaces);
	LL(mdxm->ofsSurfHierarchy);
	LL(mdxm->ofsEnd);

	// first up, go load in the animation file we need that has the skeletal animation info for this model
	mdxm->animIndex = RE_RegisterModel(va ("%s.gla",mdxm->animName));
	if (!mdxm->animIndex) 
	{
		ri.Printf( PRINT_WARNING, "R_LoadAndPatchMDXM: missing animation file %s for mesh %s\n", mdxm->animName, mdxm->name);
		return qfalse;
	}

	mod->numLods = mdxm->numLODs -1 ;	//copy this up to the model for ease of use - it wil get inced after this.

	surfInfo = (mdxmSurfHierarchy_t *)( (byte *)mdxm + mdxm->ofsSurfHierarchy);
 	for ( i = 0 ; i < mdxm->numSurfaces ; i++) 
	{
		LL(surfInfo->numChildren);
		LL(surfInfo->parentIndex);

		// do all the children indexs
		for (j=0; j<surfInfo->numChildren; j++)
		{
			LL(surfInfo->childIndexes[j]);
		}

		// find the next surface
		surfInfo = (mdxmSurfHierarchy_t *)( (byte *)surfInfo + (int)( &((mdxmSurfHierarchy_t *)0)->childIndexes[ surfInfo->numChildren ] ));
  	}

	tempPtr += mdxm->ofsLODs;

	// Process LODs
	fileLod = (mdxmLOD_t *) ( (byte *)buffer + mdxm->ofsLODs );
	tempLod = (mdxmLOD_t *) tempPtr;
	for ( l = 0 ; l < mdxm->numLODs ; l++)
	{
		int	triCount = 0;
		int lodOffs = LittleLong(fileLod->ofsEnd);

		// Copy LOD record
		memcpy(tempLod, fileLod, sizeof(mdxmLOD_t));
		tempPtr += sizeof(mdxmLOD_t);

		// Reserve space for surface offsets
		fileSurfOffs = (mdxmLODSurfOffset_t *) ((byte *)fileLod + sizeof (mdxmLOD_t));
		tempSurfOffs = (mdxmLODSurfOffset_t *) tempPtr;
		tempPtr += mdxm->numSurfaces * sizeof(mdxmLODSurfOffset_t);

		// Process all the surfaces
		fileSurf = (mdxmSurface_t *) ( (byte *)fileLod + sizeof (mdxmLOD_t) + (mdxm->numSurfaces * sizeof(mdxmLODSurfOffset_t)) );
		tempSurf = (mdxmSurface_t *) tempPtr;
		for ( i = 0 ; i < mdxm->numSurfaces ; i++) 
		{
			int surfOffs = (int) ((byte *)fileSurf - (byte *)fileSurfOffs);

			// Find and update LOD surface offset
			for ( j = 0; j < mdxm->numSurfaces ; j++ )
			{
				if ( LittleLong(fileSurfOffs->offsets[j]) == surfOffs )
				{
					// Found surface, now update offset
					tempSurfOffs->offsets[j] = (int) ((byte *)tempSurf - (byte *)tempSurfOffs);
					break;
				}
			}

			// Copy Surface
			memcpy(tempSurf, fileSurf, sizeof(mdxmSurface_t));
			tempPtr += sizeof(mdxmSurface_t);

			// Original surface offsets 
			int ofsBoneReferences	= LittleLong(fileSurf->ofsBoneReferences);
			int ofsTriangles		= LittleLong(fileSurf->ofsTriangles);
			int ofsVerts			= LittleLong(fileSurf->ofsVerts);
			int ofsEnd				= LittleLong(fileSurf->ofsEnd);

			LL(tempSurf->numVerts);
			LL(tempSurf->numTriangles);
			LL(tempSurf->numBoneReferences);
//			LL(tempSurf->maxVertBoneWeights);
			// change to surface identifier
			tempSurf->ident = SF_MDX;

			triCount += tempSurf->numTriangles;
										
			if ( tempSurf->numVerts > SHADER_MAX_VERTEXES ) {
				ri.Error (ERR_DROP, "R_LoadAndPatchMDXM: %s has more than %i verts on a surface (%i)",
					mod_name, SHADER_MAX_VERTEXES, tempSurf->numVerts );
			}
			if ( tempSurf->numTriangles*3 > SHADER_MAX_INDEXES ) {
				ri.Error (ERR_DROP, "R_LoadAndPatchMDXM: %s has more than %i triangles on a surface (%i)",
					mod_name, SHADER_MAX_INDEXES / 3, tempSurf->numTriangles );
			}

			// FIXME - is this correct? <---- what does that mean ???
			// Process bone reference data
			fileBoneRef = (int *) ( (byte *)fileSurf + ofsBoneReferences );
			tempBoneRef = (int *) tempPtr;
			// Update new offset
			tempSurf->ofsBoneReferences = (int) ( (byte *)tempBoneRef - (byte *)tempSurf );
			for ( j = 0 ; j < tempSurf->numBoneReferences ; j++ ) 
			{
				tempBoneRef[j] = LittleLong(fileBoneRef[j]);
			}
			tempPtr += tempSurf->numBoneReferences * sizeof(int);

			// Process verices
			fileVert = (mdxmVertex_t *) ( (byte *)fileSurf + ofsVerts );
			tempVert = (mdxmVertex_t *) tempPtr;
			// Update new offset
			tempSurf->ofsVerts = (int) ( (byte *)tempVert - (byte *)tempSurf );
			for ( j = 0 ; j < tempSurf->numVerts ; j++ ) 
			{
				tempVert->vertCoords[0] = LittleFloat( fileVert->vertCoords[0] );
				tempVert->vertCoords[1] = LittleFloat( fileVert->vertCoords[1] );
				tempVert->vertCoords[2] = LittleFloat( fileVert->vertCoords[2] );

				tempVert->normal[0] = LittleFloat( fileVert->normal[0] );
				tempVert->normal[1] = LittleFloat( fileVert->normal[1] );
				tempVert->normal[2] = LittleFloat( fileVert->normal[2] );

				tempVert->texCoords[0] = LittleFloat( fileVert->texCoords[0] );
				tempVert->texCoords[1] = LittleFloat( fileVert->texCoords[1] );

				tempVert->numWeights = LittleLong( fileVert->numWeights );

				for ( k = 0 ; k < /*tempVert->numWeights*/tempSurf->maxVertBoneWeights ; k++ ) 
				{
					tempVert->weights[k].boneIndex = LittleLong( fileVert->weights[k].boneIndex );
					tempVert->weights[k].boneWeight = LittleFloat( fileVert->weights[k].boneWeight );
				}
				tempPtr = (byte *) &tempVert->weights[/*tempVert->numWeights*/tempSurf->maxVertBoneWeights];


				fileVert = (mdxmVertex_t *) &fileVert->weights[/*tempVert->numWeights*/tempSurf->maxVertBoneWeights];
				tempVert = (mdxmVertex_t *) tempPtr;
			}

			// Process triangles
			fileTri = (mdxmTriangle_t *) ( (byte *)fileSurf + ofsTriangles );
			tempTri = (mdxmTriangle_t *) tempPtr;
			// Update new offset
			tempSurf->ofsTriangles = (int) ( (byte *)tempTri - (byte *)tempSurf );
			for ( j = 0 ; j < tempSurf->numTriangles ; j++, tempTri++, fileTri++ ) 
			{
				tempTri->indexes[0] = LittleLong(fileTri->indexes[0]);
				tempTri->indexes[1] = LittleLong(fileTri->indexes[1]);
				tempTri->indexes[2] = LittleLong(fileTri->indexes[2]);
			}
			tempPtr += tempSurf->numTriangles * sizeof(mdxmTriangle_t);

			// Try to patch surface
			int patchTriCount;
			patchTri = R_SurfFillCreases( tempSurf, &patchTriCount );
			if ( (patchTriCount > 0) && (tempSurf->numTriangles + patchTriCount <= SHADER_MAX_INDEXES / 3) )
			{
				// Update triangle counts
				tempSurf->numTriangles += patchTriCount;
				triCount += patchTriCount;
				// Copy triangles
				memcpy((void *)tempPtr, patchTri, patchTriCount * sizeof(mdxmTriangle_t));
				tempPtr += patchTriCount * sizeof(mdxmTriangle_t);
			}

			if ( patchTri )
			{
				Z_Free(patchTri);
			}

			// Update surface end offset
			tempSurf->ofsEnd = (int) (tempPtr - (byte *)tempSurf);
			// Update header offset
			tempSurf->ofsHeader = (int) ((byte *)mdxm - (byte *)tempSurf);

			// Find the next surface
			fileSurf = (mdxmSurface_t *) ( (byte *)fileSurf + ofsEnd );
			tempSurf = (mdxmSurface_t *) tempPtr;
		}
#if _DEBUG
		ri.Printf(0, "R_LoadAndPatchMDXM(): Lod %d has %d tris in %d surfaces with %d bones\n", l, triCount, mdxm->numSurfaces, mdxm->numBones);
#endif
		// Update LOD end offset
		tempLod->ofsEnd = (int) (tempPtr - (byte *)tempLod);

		// Find the next LOD
		fileLod = (mdxmLOD_t *) ( (byte *)fileLod + lodOffs );
		tempLod = (mdxmLOD_t *) tempPtr;
	}

	size = (int) ( tempPtr - (byte *)mdxm );

	// Update file end
	mdxm->ofsEnd = size;

	mod->type	   = MOD_MDXM;
	mod->dataSize += size;	
	
	qboolean bAlreadyFound = qfalse;
	mod->mdxm = (mdxmHeader_t*)RE_RegisterModels_Malloc(size, mod_name, &bAlreadyFound, TAG_MODEL_GLM);

	if (bAlreadyFound)
	{
		// Now this is a problem! We shouldn't be executing R_LoadAndPatchMDXM if model is cached
#if _DEBUG
		ri.Printf(0, "R_LoadAndPatchMDXM(): Model %s patched after found in cache!\n", mod_name);
#endif
		// Just discard changes...
		Z_Free(tempBuffer);
		return qtrue;
	}

	// Copy patched model
	memcpy(mod->mdxm, mdxm, size);
	Z_Free(tempBuffer);

	// Register shaders
	surfInfo = (mdxmSurfHierarchy_t *)( (byte *)mod->mdxm + mod->mdxm->ofsSurfHierarchy);
 	for ( i = 0 ; i < mod->mdxm->numSurfaces ; i++) 
	{
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
		RE_RegisterModels_StoreShaderRequest(mod_name, &surfInfo->shader[0], &surfInfo->shaderIndex);		

		// find the next surface
		surfInfo = (mdxmSurfHierarchy_t *)( (byte *)surfInfo + (int)( &((mdxmSurfHierarchy_t *)0)->childIndexes[ surfInfo->numChildren ] ));
  	}

	return qtrue;
#else
	return qfalse;
#endif
}

#endif // _NPATCH
 
/*
=================
R_LoadMDXM - load a Ghoul 2 Mesh file
=================
*/
qboolean R_LoadMDXM( model_t *mod, void *buffer, const char *mod_name, qboolean bAlreadyCached ) {
	int					i, l, j;
	mdxmHeader_t		*pinmodel, *mdxm;
	mdxmLOD_t			*lod;
	mdxmSurface_t		*surf;
	int					version;
	int					size;
	shader_t			*sh;
	mdxmSurfHierarchy_t	*surfInfo;

#ifndef _M_IX86
	int					k;
	int					frameSize;
//	mdxmTag_t			*tag;
	mdxmTriangle_t		*tri;
	mdxmVertex_t		*v;
 	mdxmFrame_t			*cframe;
	int					*boneRef;
#endif

#ifdef  _NPATCH
	//
	// If n-patches are enabled, load and patch the models
	//
	// fixme: could probably do with this upgrading sometime
//	if (r_ati_pn_triangles->integer && mod->npatchable && !bAlreadyCached)
//	{
//		return R_LoadAndPatchMDXM( mod, buffer, mod_name, bAlreadyCached );
//	}
#endif // _NPATCH

	pinmodel= (mdxmHeader_t *)buffer;
	//
	// read some fields from the binary, but only LittleLong() them when we know this wasn't an already-cached model...
	//	
	version = (pinmodel->version);
	size	= (pinmodel->ofsEnd);

	if (!bAlreadyCached)
	{
		version = LittleLong(version);
		size	= LittleLong(size);
	}

	if (version != MDXM_VERSION) {
#ifdef _DEBUG
		ri.Error( ERR_DROP,       "R_LoadMDXM: %s has wrong version (%i should be %i)\n", mod_name, version, MDXM_VERSION);
#else
		ri.Printf( PRINT_WARNING, "R_LoadMDXM: %s has wrong version (%i should be %i)\n", mod_name, version, MDXM_VERSION);
#endif
		return qfalse;
	}

	mod->type	   = MOD_MDXM;
	mod->dataSize += size;	
	
	qboolean bAlreadyFound = qfalse;
	mdxm = mod->mdxm = (mdxmHeader_t*) //ri.Hunk_Alloc( size );
										RE_RegisterModels_Malloc(size, mod_name, &bAlreadyFound, TAG_MODEL_GLM);

	assert(bAlreadyCached == bAlreadyFound);	// I should probably eliminate 'bAlreadyFound', but wtf?

	if (!bAlreadyFound)
	{
		memcpy( mdxm, buffer, size );	

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
		ri.Printf( PRINT_WARNING, "R_LoadMDXM: missing animation file %s for mesh %s\n", mdxm->animName, mdxm->name);
		return qfalse;
	}

	mod->numLods = mdxm->numLODs -1 ;	//copy this up to the model for ease of use - it wil get inced after this.

	if (bAlreadyFound)
	{
		return qtrue;	// All done. Stop, go no further, do not LittleLong(), do not pass Go...
	}

	surfInfo = (mdxmSurfHierarchy_t *)( (byte *)mdxm + mdxm->ofsSurfHierarchy);
 	for ( i = 0 ; i < mdxm->numSurfaces ; i++) 
	{
		LL(surfInfo->numChildren);
		LL(surfInfo->parentIndex);

		// do all the children indexs
		for (j=0; j<surfInfo->numChildren; j++)
		{
			LL(surfInfo->childIndexes[j]);
		}

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
		RE_RegisterModels_StoreShaderRequest(mod_name, &surfInfo->shader[0], &surfInfo->shaderIndex);		

		// find the next surface
		surfInfo = (mdxmSurfHierarchy_t *)( (byte *)surfInfo + (int)( &((mdxmSurfHierarchy_t *)0)->childIndexes[ surfInfo->numChildren ] ));
  	}
	
#if _DEBUG
	ri.Printf(0, "For Ghoul2 mesh file %s\n", mod_name);
#endif
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
				ri.Error (ERR_DROP, "R_LoadMDXM: %s has more than %i verts on a surface (%i)",
					mod_name, SHADER_MAX_VERTEXES, surf->numVerts );
			}
			if ( surf->numTriangles*3 > SHADER_MAX_INDEXES ) {
				ri.Error (ERR_DROP, "R_LoadMDXM: %s has more than %i triangles on a surface (%i)",
					mod_name, SHADER_MAX_INDEXES / 3, surf->numTriangles );
			}
		
			// change to surface identifier
			surf->ident = SF_MDX;

			// register the shaders
#ifndef _M_IX86
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

			// find the next surface
			surf = (mdxmSurface_t *)( (byte *)surf + surf->ofsEnd );
		}
#if _DEBUG
		ri.Printf(0, "Lod %d has %d tris in %d surfaces with %d bones\n", l, triCount, mdxm->numSurfaces, mdxm->numBones);
#endif
		// find the next LOD
		lod = (mdxmLOD_t *)( (byte *)lod + lod->ofsEnd );
	}


// This is junk, there's no such structure...
//
//#ifndef _M_IX86
////
//// optimisation, we don't bother doing this for standard intel case since our data's already in that format...
////
//	tag = (mdxmTag_t *) ( (byte *)mdxm + mdxm->ofsTags );
//	for ( i = 0 ; i < md4->numTags ; i++) {
//		LL(tag->boneIndex);
//		tag++;
//	}
//#endif

	return qtrue;
}

/*
=================
R_LoadMDXA - load a Ghoul 2 animation file
=================
*/
qboolean R_LoadMDXA( model_t *mod, void *buffer, const char *mod_name, qboolean bAlreadyCached ) {

	mdxaHeader_t		*pinmodel, *mdxa;
	int					version;
	int					size;

#ifndef _M_IX86
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
		version = LittleLong(version);
		size	= LittleLong(size);
	}
	
	if (version != MDXA_VERSION) {
		ri.Printf( PRINT_WARNING, "R_LoadMDXA: %s has wrong version (%i should be %i)\n",
				 mod_name, version, MDXA_VERSION);
		return qfalse;
	}

	mod->type		= MOD_MDXA;
	mod->dataSize  += size;

	qboolean bAlreadyFound = qfalse;
	mdxa = mod->mdxa = (mdxaHeader_t*) //ri.Hunk_Alloc( size );
										RE_RegisterModels_Malloc(size, mod_name, &bAlreadyFound, TAG_MODEL_GLA);

	assert(bAlreadyCached == bAlreadyFound);	// I should probably eliminate 'bAlreadyFound', but wtf?

	if (!bAlreadyFound)
	{
		memcpy( mdxa, buffer, size );

		LL(mdxa->ident);
		LL(mdxa->version);
		LL(mdxa->numFrames);
		LL(mdxa->numBones);
		LL(mdxa->ofsFrames);
		LL(mdxa->ofsEnd);
	}

 	if ( mdxa->numFrames < 1 ) {
		ri.Printf( PRINT_WARNING, "R_LoadMDXA: %s has no frames\n", mod_name );
		return qfalse;
	}

	if (bAlreadyFound)
	{
		return qtrue;	// All done, stop here, do not LittleLong() etc. Do not pass go...
	}

#ifndef _M_IX86

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
		boneInfo += (int)( &((mdxaSkel_t *)0)->children[ boneInfo->numChildren ] );
	}


	// swap all the frames
	frameSize = (int)( &((mdxaFrame_t *)0)->bones[ mdxa->numBones ] );
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

