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


// FIXME Not used.
#if 0

#pragma once
#if !defined(GHOUL2_SHARED_H_INC)
#define GHOUL2_SHARED_H_INC

/*
Ghoul2 Insert Start
*/
#pragma warning (push, 3)	//go back down to 3 for the stl include
#pragma warning (disable:4503)	// decorated name length xceeded, name was truncated
#include <vector>
#include <map>
#pragma warning (pop)
#pragma warning (disable:4503)	// decorated name length xceeded, name was truncated
/*
Ghoul2 Insert End
*/

#define G2T_SV_TIME (0)
#define G2T_CG_TIME (1)
#define NUM_G2T_TIME (2)

void		G2API_SetTime(int currentTime,int clock);
int			G2API_GetTime(int argTime); // this may or may not return arg depending on ghoul2_time cvar


//===================================================================
//
//   G H O U L  I I  D E F I N E S
//
// we save the whole surfaceInfo_t struct
struct surfaceInfo_t
{
	int			offFlags;		// what the flags are for this model
	int			surface;		// index into array held inside the model definition of pointers to the actual surface data loaded in - used by both client and game
	float		genBarycentricJ;	// point 0 barycentric coors
	float		genBarycentricI;	// point 1 barycentric coors - point 2 is 1 - point0 - point1
	int			genPolySurfaceIndex; // used to point back to the original surface and poly if this is a generated surface
	int			genLod;			// used to determine original lod of original surface and poly hit location

surfaceInfo_t():
	offFlags(0),
	surface(0),
	genBarycentricJ(0),
	genBarycentricI(0),
	genPolySurfaceIndex(0),
	genLod(0)
	{}

};

#define BONE_ANGLES_PREMULT			0x0001
#define BONE_ANGLES_POSTMULT		0x0002
#define BONE_ANGLES_REPLACE			0x0004
#define BONE_ANGLES_TOTAL			( BONE_ANGLES_PREMULT | BONE_ANGLES_POSTMULT | BONE_ANGLES_REPLACE )

#define BONE_ANIM_OVERRIDE			0x0008
#define BONE_ANIM_OVERRIDE_LOOP		0x0010	// Causes Last Frame To Lerp to First Frame And Start Over
#define BONE_ANIM_OVERRIDE_FREEZE	(0x0040 + BONE_ANIM_OVERRIDE)	// Causes Last Frame To Freeze And Not Loop To Beginning
#define BONE_ANIM_BLEND				0x0080		// Blends to and from previously played frame on same bone for given time
#define	BONE_ANIM_NO_LERP			0x1000
#define BONE_ANIM_TOTAL				(BONE_ANIM_NO_LERP| BONE_ANIM_OVERRIDE | BONE_ANIM_OVERRIDE_LOOP | BONE_ANIM_OVERRIDE_FREEZE | BONE_ANIM_BLEND )

#define BONE_INDEX_INVALID			-1

/*#define MDXABONEDEF				// used in the mdxformat.h file to stop redefinitions of the bone struct.

typedef struct {
	float		matrix[3][4];
} mdxaBone_t;
*/
#include "../../code/renderer/mdx_format.h"

// we save the whole structure here.
struct  boneInfo_t
{
	int			boneNumber;		// what bone are we overriding?
	mdxaBone_t	matrix;			// details of bone angle overrides - some are pre-done on the server, some in ghoul2
	int			flags;			// flags for override
	int			startFrame;		// start frame for animation
	int			endFrame;		// end frame for animation NOTE anim actually ends on endFrame+1
	int			startTime;		// time we started this animation
	int			pauseTime;		// time we paused this animation - 0 if not paused
	float		animSpeed;		// speed at which this anim runs. 1.0f means full speed of animation incoming - ie if anim is 20hrtz, we run at 20hrts. If 5hrts, we run at 5 hrts
	float		blendFrame;		// frame PLUS LERP value to blend from
	int			blendLerpFrame;	// frame to lerp the blend frame with.
	int			blendTime;		// Duration time for blending - used to calc amount each frame of new anim is blended with last frame of the last anim
	int			blendStart;		// Time when blending starts - not necessarily the same as startTime since we might start half way through an anim
	int			boneBlendTime;	// time for duration of bone angle blend with normal animation
	int			boneBlendStart;	// time bone angle blend with normal animation began
	mdxaBone_t	newMatrix;		// This is the lerped matrix that Ghoul2 uses on the client side - does not go across the network

boneInfo_t():
	boneNumber(-1),
	flags(0),
	startFrame(0),
	endFrame(0),
	startTime(0),
	pauseTime(0),
	animSpeed(0),
	blendFrame(0),
	blendLerpFrame(0),
	blendTime(0),
	blendStart(0),
	boneBlendTime(0),
	boneBlendStart(0)
	{
		matrix.matrix[0][0] = matrix.matrix[0][1] = matrix.matrix[0][2] = matrix.matrix[0][3] =
		matrix.matrix[1][0] = matrix.matrix[1][1] = matrix.matrix[1][2] = matrix.matrix[1][3] =
		matrix.matrix[2][0] = matrix.matrix[2][1] = matrix.matrix[2][2] = matrix.matrix[2][3] = 0.0f;
	}

};
//we save from top to boltUsed here. Don't bother saving the position, it gets rebuilt every frame anyway
struct boltInfo_t{
	int			boneNumber;		// bone number bolt attaches to
	int			surfaceNumber;	// surface number bolt attaches to
	int			surfaceType;	// if we attach to a surface, this tells us if it is an original surface or a generated one - doesn't go across the network
	int			boltUsed;		// nor does this
	boltInfo_t():
	boneNumber(-1),
	surfaceNumber(-1),
	surfaceType(0),
	boltUsed(0)
	{}
};


#define MAX_GHOUL_COUNT_BITS 8 // bits required to send across the MAX_G2_MODELS inside of the networking - this is the only restriction on ghoul models possible per entity

typedef vector <surfaceInfo_t> surfaceInfo_v;
typedef vector <boneInfo_t> boneInfo_v;
typedef vector <boltInfo_t> boltInfo_v;
typedef vector <mdxaBone_t> mdxaBone_v;

// defines for stuff to go into the mflags
#define		GHOUL2_NOCOLLIDE 0x001
#define		GHOUL2_NORENDER	 0x002
#define		GHOUL2_NOMODEL	 0x004
#define		GHOUL2_NEWORIGIN 0x008


// NOTE order in here matters. We save out from mModelindex to mFlags, but not the STL vectors that are at the top or the bottom.
class CBoneCache;
struct model_s;
//struct mdxaHeader_t;

class CGhoul2Info
{
public:
	surfaceInfo_v 	mSlist;
	boltInfo_v		mBltlist;
	boneInfo_v		mBlist;
// save from here
#define BSAVE_START_FIELD mModelindex	// this is the start point for loadsave, keep it up to date it you change anything
	int				mModelindex;
#ifndef __NO_JKA
	int				animModelIndexOffset;
#endif
	qhandle_t		mCustomShader;
	qhandle_t		mCustomSkin;
	int				mModelBoltLink;
	int				mSurfaceRoot;
	int				mLodBias;
	int				mNewOrigin;	// this contains the bolt index of the new origin for this model
#ifdef _G2_GORE
	int				mGoreSetTag;
#endif
	qhandle_t		mModel;		// this and the next entries do NOT go across the network. They are for gameside access ONLY
	char			mFileName[MAX_QPATH];
	int				mAnimFrameDefault;
	int				mSkelFrameNum;
	int				mMeshFrameNum;
	int				mFlags;	// used for determining whether to do full collision detection against this object
// to here
#define BSAVE_END_FIELD mTransformedVertsArray	// this is the end point for loadsave, keep it up to date it you change anything
	int				*mTransformedVertsArray;	// used to create an array of pointers to transformed verts per surface for collision detection
	CBoneCache		*mBoneCache;
	int				mSkin;

	// these occasionally are not valid (like after a vid_restart)
	// call the questionably efficient G2_SetupModelPointers(this) to insure validity
	bool				mValid; // all the below are proper and valid
	const model_s		*currentModel;
	int					currentModelSize;
	const model_s		*animModel;
	int					currentAnimModelSize;
	const mdxaHeader_t	*aHeader;

	CGhoul2Info():
	mModelindex(-1),
	mCustomShader(0),
	mCustomSkin(0),
	mModelBoltLink(0),
	mModel(0),
	mSurfaceRoot(0),
	mAnimFrameDefault(0),
	mSkelFrameNum(-1),
	mMeshFrameNum(-1),
	mFlags(0),
	mTransformedVertsArray(0),
	mLodBias(0),
	mSkin(0),
	mNewOrigin(-1),
#ifdef _G2_GORE
	mGoreSetTag(0),
#endif
	mBoneCache(0),
	currentModel(0),
	currentModelSize(0),
	animModel(0),
#ifndef __NO_JKA
	animModelIndexOffset(0),
#endif
	currentAnimModelSize(0),
	aHeader(0),
	mValid(false)
	{
		mFileName[0] = 0;
	}
};

class CGhoul2Info_v;

class IGhoul2InfoArray
{
public:
	virtual int New()=0;
	virtual void Delete(int handle)=0;
	virtual bool IsValid(int handle) const=0;
	virtual vector<CGhoul2Info> &Get(int handle)=0;
	virtual const vector<CGhoul2Info> &Get(int handle) const=0;
};

IGhoul2InfoArray &TheGhoul2InfoArray();
IGhoul2InfoArray &TheGameGhoul2InfoArray();

class CGhoul2Info_v
{
	int mItem;

	IGhoul2InfoArray &InfoArray() const
	{
#ifdef _JK2EXE
		return TheGhoul2InfoArray();
#else
		return TheGameGhoul2InfoArray();
#endif
	}

	void Alloc()
	{
		assert(!mItem); //already alloced
		mItem=InfoArray().New();
		assert(!Array().size());
	}
	void Free()
	{
		if (mItem)
		{
			assert(InfoArray().IsValid(mItem));
			InfoArray().Delete(mItem);
			mItem=0;
		}
	}
	vector<CGhoul2Info> &Array()
	{
		assert(InfoArray().IsValid(mItem));
		return InfoArray().Get(mItem);
	}
	const vector<CGhoul2Info> &Array() const
	{
		assert(InfoArray().IsValid(mItem));
		return InfoArray().Get(mItem);
	}
public:
	CGhoul2Info_v()
	{
		mItem=0;
	}
	~CGhoul2Info_v()
	{
		Free(); //this had better be taken care of via the clean ghoul2 models call
	}
	void operator=(const CGhoul2Info_v &other)
	{
		mItem=other.mItem;
	}
	void DeepCopy(const CGhoul2Info_v &other)
	{
		Free();
		if (other.mItem)
		{
			Alloc();
			Array()=other.Array();
			int i;
			for (i=0;i<size();i++)
			{
				Array()[i].mBoneCache=0;
				Array()[i].mTransformedVertsArray=0;
				Array()[i].mSkelFrameNum=0;
				Array()[i].mMeshFrameNum=0;
			}
		}
	}
	CGhoul2Info &operator[](int idx)
	{
		assert(mItem);
		assert(idx>=0&&idx<size());
		return Array()[idx];
	}
	const CGhoul2Info &operator[](int idx) const
	{
		assert(mItem);
		assert(idx>=0&&idx<size());
		return Array()[idx];
	}
	void resize(int num)
	{
		assert(num>=0);
		if (num)
		{
			if (!mItem)
			{
				Alloc();
			}
		}
		if (mItem||num)
		{
			Array().resize(num);
		}
	}
	void clear()
	{
		Free();
	}
	void push_back(const CGhoul2Info &model)
	{
		if (!mItem)
		{
			Alloc();
		}
		Array().push_back(model);
	}
	int size() const
	{
		if (!IsValid())
		{
			return 0;
		}
		return Array().size();
	}
	bool IsValid() const
	{
		return InfoArray().IsValid(mItem);
	}
	void kill()
	{
		// this scary method zeros the infovector handle without actually freeing it
		// it is used for some places where a copy is made, but we don't want to go through the trouble
		// of making a deep copy
		mItem=0;
	}
};



// collision detection stuff
#define G2_FRONTFACE 1
#define	G2_BACKFACE	 0


class CCollisionRecord
{
public:
	float		mDistance;
	int			mEntityNum;
	int			mModelIndex;
	int			mPolyIndex;
	int			mSurfaceIndex;
	vec3_t		mCollisionPosition;
	vec3_t		mCollisionNormal;
	int			mFlags;
	int			mMaterial;
	int			mLocation;
	float		mBarycentricI; // two barycentic coodinates for the hit point
	float		mBarycentricJ; // K = 1-I-J

	CCollisionRecord():
	mEntityNum(-1),
	mDistance(100000)
	{}
};

// calling defines for the trace function
enum EG2_Collision
{
	G2_NOCOLLIDE,
	G2_COLLIDE,
	G2_RETURNONHIT
};



//====================================================================

#endif // GHOUL2_SHARED_H_INC

#endif // 0
