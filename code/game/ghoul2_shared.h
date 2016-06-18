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

#pragma once
#if !defined(GHOUL2_SHARED_H_INC)
#define GHOUL2_SHARED_H_INC

#include <vector>
#include <map>

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
#pragma pack(push, 4)
class SgSurfaceInfo
{
public:
    int32_t offFlags;
    int32_t surface;
    float genBarycentricJ;
    float genBarycentricI;
    int32_t genPolySurfaceIndex;
    int32_t genLod;
}; // SgSurfaceInfo
#pragma pack(pop)

struct surfaceInfo_t
{
    using SgType = SgSurfaceInfo;


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


    void sg_export(
        SgType& dst) const
    {
        ::sg_export(offFlags, dst.offFlags);
        ::sg_export(surface, dst.surface);
        ::sg_export(genBarycentricJ, dst.genBarycentricJ);
        ::sg_export(genBarycentricI, dst.genBarycentricI);
        ::sg_export(genPolySurfaceIndex, dst.genPolySurfaceIndex);
        ::sg_export(genLod, dst.genLod);
    }

    void sg_import(
        const SgType& src)
    {
        ::sg_import(src.offFlags, offFlags);
        ::sg_import(src.surface, surface);
        ::sg_import(src.genBarycentricJ, genBarycentricJ);
        ::sg_import(src.genBarycentricI, genBarycentricI);
        ::sg_import(src.genPolySurfaceIndex, genPolySurfaceIndex);
        ::sg_import(src.genLod, genLod);
    }
};

#define BONE_ANGLES_PREMULT			0x0001
#define BONE_ANGLES_POSTMULT		0x0002
#define BONE_ANGLES_REPLACE			0x0004
//rww - RAGDOLL_BEGIN
#define BONE_ANGLES_RAGDOLL			0x2000  // the rag flags give more details
#define BONE_ANGLES_IK				0x4000  // the rag flags give more details
//rww - RAGDOLL_END
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
#include "../rd-common/mdx_format.h"

// we save the whole structure here.
#pragma pack(push, 4)
class SgBoneInfo
{
public:
    int32_t boneNumber;
    SgMdxaBone matrix;
    int32_t flags;
    int32_t startFrame;
    int32_t endFrame;
    int32_t startTime;
    int32_t pauseTime;
    float animSpeed;
    float blendFrame;
    int32_t blendLerpFrame;
    int32_t blendTime;
    int32_t blendStart;
    int32_t boneBlendTime;
    int32_t boneBlendStart;
    SgMdxaBone newMatrix;
    int32_t lastTimeUpdated;
    int32_t lastContents;
    SgVec3 lastPosition;
    SgVec3 velocityEffector;
    SgVec3 lastAngles;
    SgVec3 minAngles;
    SgVec3 maxAngles;
    SgVec3 currentAngles;
    SgVec3 anglesOffset;
    SgVec3 positionOffset;
    float radius;
    float weight;
    int32_t ragIndex;
    SgVec3 velocityRoot;
    int32_t ragStartTime;
    int32_t firstTime;
    int32_t firstCollisionTime;
    int32_t restTime;
    int32_t RagFlags;
    int32_t DependentRagIndexMask;
    SgMdxaBone originalTrueBoneMatrix;
    SgMdxaBone parentTrueBoneMatrix;
    SgMdxaBone parentOriginalTrueBoneMatrix;
    SgVec3 originalOrigin;
    SgVec3 originalAngles;
    SgVec3 lastShotDir;
    int32_t basepose;
    int32_t baseposeInv;
    int32_t baseposeParent;
    int32_t baseposeInvParent;
    int32_t parentRawBoneIndex;
    SgMdxaBone ragOverrideMatrix;
    SgMdxaBone extraMatrix;
    SgVec3 extraVec1;
    float extraFloat1;
    int32_t extraInt1;
    SgVec3 ikPosition;
    float ikSpeed;
    SgVec3 epVelocity;
    float epGravFactor;
    int32_t solidCount;
    uint8_t physicsSettled;
    uint8_t snapped;
    int32_t parentBoneIndex;
    float offsetRotation;
    float overGradSpeed;
    SgVec3 overGoalSpot;
    uint8_t hasOverGoal;
    SgMdxaBone animFrameMatrix;
    int32_t hasAnimFrameMatrix;
    int32_t airTime;
}; // SgBoneInfo
#pragma pack(pop)

struct  boneInfo_t
{
    using SgType = SgBoneInfo;


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

	//rww - RAGDOLL_BEGIN
	int			lastTimeUpdated;  // if non-zero this is all intialized
	int			lastContents;
	vec3_t		lastPosition;
	vec3_t		velocityEffector;
	vec3_t		lastAngles;
	vec3_t		minAngles;
	vec3_t		maxAngles;
	vec3_t		currentAngles;
	vec3_t		anglesOffset;
	vec3_t		positionOffset;
	float		radius;
	float		weight;   // current radius cubed
	int			ragIndex;
	vec3_t		velocityRoot;		// I am really tired of recomiling the whole game to add a param here
	int			ragStartTime;
	int			firstTime;
	int			firstCollisionTime;
	int			restTime;
	int			RagFlags;
	int			DependentRagIndexMask;
	mdxaBone_t	originalTrueBoneMatrix;
	mdxaBone_t	parentTrueBoneMatrix;			// figure I will need this sooner or later
	mdxaBone_t	parentOriginalTrueBoneMatrix;	// figure I will need this sooner or later
	vec3_t		originalOrigin;
	vec3_t		originalAngles;
	vec3_t		lastShotDir;
	mdxaBone_t  *basepose;
	mdxaBone_t  *baseposeInv;
	mdxaBone_t  *baseposeParent;
	mdxaBone_t  *baseposeInvParent;
	int			parentRawBoneIndex;
	mdxaBone_t	ragOverrideMatrix;	// figure I will need this sooner or later

	mdxaBone_t	extraMatrix;	// figure I will need this sooner or later
	vec3_t		extraVec1;		// I am really tired of recomiling the whole game to add a param here
	float		extraFloat1;
	int			extraInt1;

	vec3_t		ikPosition;
	float		ikSpeed;

	//new ragdoll stuff -rww
	vec3_t		epVelocity; //velocity factor, can be set, and is also maintained by physics based on gravity, mass, etc.
	float		epGravFactor; //gravity factor maintained by bone physics
	int			solidCount; //incremented every time we try to move and are in solid - if we get out of solid, it is reset to 0
	bool		physicsSettled; //true when the bone is on ground and finished bouncing, etc. but may still be pushed into solid by other bones
	bool		snapped; //the bone is broken out of standard constraints

	int			parentBoneIndex;

	float		offsetRotation;

	//user api overrides
	float		overGradSpeed;

	vec3_t		overGoalSpot;
	bool		hasOverGoal;

	mdxaBone_t	animFrameMatrix; //matrix for the bone in the desired settling pose -rww
	int			hasAnimFrameMatrix;

	int			airTime; //base is in air, be more quick and sensitive about collisions
	//rww - RAGDOLL_END

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


    void sg_export(
        SgType& dst) const
    {
        ::sg_export(boneNumber, dst.boneNumber);
        ::sg_export(matrix, dst.matrix);
        ::sg_export(flags, dst.flags);
        ::sg_export(startFrame, dst.startFrame);
        ::sg_export(endFrame, dst.endFrame);
        ::sg_export(startTime, dst.startTime);
        ::sg_export(pauseTime, dst.pauseTime);
        ::sg_export(animSpeed, dst.animSpeed);
        ::sg_export(blendFrame, dst.blendFrame);
        ::sg_export(blendLerpFrame, dst.blendLerpFrame);
        ::sg_export(blendTime, dst.blendTime);
        ::sg_export(blendStart, dst.blendStart);
        ::sg_export(boneBlendTime, dst.boneBlendTime);
        ::sg_export(boneBlendStart, dst.boneBlendStart);
        ::sg_export(newMatrix, dst.newMatrix);
        ::sg_export(lastTimeUpdated, dst.lastTimeUpdated);
        ::sg_export(lastContents, dst.lastContents);
        ::sg_export(lastPosition, dst.lastPosition);
        ::sg_export(velocityEffector, dst.velocityEffector);
        ::sg_export(lastAngles, dst.lastAngles);
        ::sg_export(minAngles, dst.minAngles);
        ::sg_export(maxAngles, dst.maxAngles);
        ::sg_export(currentAngles, dst.currentAngles);
        ::sg_export(anglesOffset, dst.anglesOffset);
        ::sg_export(positionOffset, dst.positionOffset);
        ::sg_export(radius, dst.radius);
        ::sg_export(weight, dst.weight);
        ::sg_export(ragIndex, dst.ragIndex);
        ::sg_export(velocityRoot, dst.velocityRoot);
        ::sg_export(ragStartTime, dst.ragStartTime);
        ::sg_export(firstTime, dst.firstTime);
        ::sg_export(firstCollisionTime, dst.firstCollisionTime);
        ::sg_export(restTime, dst.restTime);
        ::sg_export(RagFlags, dst.RagFlags);
        ::sg_export(DependentRagIndexMask, dst.DependentRagIndexMask);
        ::sg_export(originalTrueBoneMatrix, dst.originalTrueBoneMatrix);
        ::sg_export(parentTrueBoneMatrix, dst.parentTrueBoneMatrix);
        ::sg_export(parentOriginalTrueBoneMatrix, dst.parentOriginalTrueBoneMatrix);
        ::sg_export(originalOrigin, dst.originalOrigin);
        ::sg_export(originalAngles, dst.originalAngles);
        ::sg_export(lastShotDir, dst.lastShotDir);
        ::sg_export(basepose, dst.basepose);
        ::sg_export(baseposeInv, dst.baseposeInv);
        ::sg_export(baseposeParent, dst.baseposeParent);
        ::sg_export(baseposeInvParent, dst.baseposeInvParent);
        ::sg_export(parentRawBoneIndex, dst.parentRawBoneIndex);
        ::sg_export(ragOverrideMatrix, dst.ragOverrideMatrix);
        ::sg_export(extraMatrix, dst.extraMatrix);
        ::sg_export(extraVec1, dst.extraVec1);
        ::sg_export(extraFloat1, dst.extraFloat1);
        ::sg_export(extraInt1, dst.extraInt1);
        ::sg_export(ikPosition, dst.ikPosition);
        ::sg_export(ikSpeed, dst.ikSpeed);
        ::sg_export(epVelocity, dst.epVelocity);
        ::sg_export(epGravFactor, dst.epGravFactor);
        ::sg_export(solidCount, dst.solidCount);
        ::sg_export(physicsSettled, dst.physicsSettled);
        ::sg_export(snapped, dst.snapped);
        ::sg_export(parentBoneIndex, dst.parentBoneIndex);
        ::sg_export(offsetRotation, dst.offsetRotation);
        ::sg_export(overGradSpeed, dst.overGradSpeed);
        ::sg_export(overGoalSpot, dst.overGoalSpot);
        ::sg_export(hasOverGoal, dst.hasOverGoal);
        ::sg_export(animFrameMatrix, dst.animFrameMatrix);
        ::sg_export(hasAnimFrameMatrix, dst.hasAnimFrameMatrix);
        ::sg_export(airTime, dst.airTime);
    }

    void sg_import(
        const SgType& src)
    {
        ::sg_import(src.boneNumber, boneNumber);
        ::sg_import(src.matrix, matrix);
        ::sg_import(src.flags, flags);
        ::sg_import(src.startFrame, startFrame);
        ::sg_import(src.endFrame, endFrame);
        ::sg_import(src.startTime, startTime);
        ::sg_import(src.pauseTime, pauseTime);
        ::sg_import(src.animSpeed, animSpeed);
        ::sg_import(src.blendFrame, blendFrame);
        ::sg_import(src.blendLerpFrame, blendLerpFrame);
        ::sg_import(src.blendTime, blendTime);
        ::sg_import(src.blendStart, blendStart);
        ::sg_import(src.boneBlendTime, boneBlendTime);
        ::sg_import(src.boneBlendStart, boneBlendStart);
        ::sg_import(src.newMatrix, newMatrix);
        ::sg_import(src.lastTimeUpdated, lastTimeUpdated);
        ::sg_import(src.lastContents, lastContents);
        ::sg_import(src.lastPosition, lastPosition);
        ::sg_import(src.velocityEffector, velocityEffector);
        ::sg_import(src.lastAngles, lastAngles);
        ::sg_import(src.minAngles, minAngles);
        ::sg_import(src.maxAngles, maxAngles);
        ::sg_import(src.currentAngles, currentAngles);
        ::sg_import(src.anglesOffset, anglesOffset);
        ::sg_import(src.positionOffset, positionOffset);
        ::sg_import(src.radius, radius);
        ::sg_import(src.weight, weight);
        ::sg_import(src.ragIndex, ragIndex);
        ::sg_import(src.velocityRoot, velocityRoot);
        ::sg_import(src.ragStartTime, ragStartTime);
        ::sg_import(src.firstTime, firstTime);
        ::sg_import(src.firstCollisionTime, firstCollisionTime);
        ::sg_import(src.restTime, restTime);
        ::sg_import(src.RagFlags, RagFlags);
        ::sg_import(src.DependentRagIndexMask, DependentRagIndexMask);
        ::sg_import(src.originalTrueBoneMatrix, originalTrueBoneMatrix);
        ::sg_import(src.parentTrueBoneMatrix, parentTrueBoneMatrix);
        ::sg_import(src.parentOriginalTrueBoneMatrix, parentOriginalTrueBoneMatrix);
        ::sg_import(src.originalOrigin, originalOrigin);
        ::sg_import(src.originalAngles, originalAngles);
        ::sg_import(src.lastShotDir, lastShotDir);
        ::sg_import(src.basepose, basepose);
        ::sg_import(src.baseposeInv, baseposeInv);
        ::sg_import(src.baseposeParent, baseposeParent);
        ::sg_import(src.baseposeInvParent, baseposeInvParent);
        ::sg_import(src.parentRawBoneIndex, parentRawBoneIndex);
        ::sg_import(src.ragOverrideMatrix, ragOverrideMatrix);
        ::sg_import(src.extraMatrix, extraMatrix);
        ::sg_import(src.extraVec1, extraVec1);
        ::sg_import(src.extraFloat1, extraFloat1);
        ::sg_import(src.extraInt1, extraInt1);
        ::sg_import(src.ikPosition, ikPosition);
        ::sg_import(src.ikSpeed, ikSpeed);
        ::sg_import(src.epVelocity, epVelocity);
        ::sg_import(src.epGravFactor, epGravFactor);
        ::sg_import(src.solidCount, solidCount);
        ::sg_import(src.physicsSettled, physicsSettled);
        ::sg_import(src.snapped, snapped);
        ::sg_import(src.parentBoneIndex, parentBoneIndex);
        ::sg_import(src.offsetRotation, offsetRotation);
        ::sg_import(src.overGradSpeed, overGradSpeed);
        ::sg_import(src.overGoalSpot, overGoalSpot);
        ::sg_import(src.hasOverGoal, hasOverGoal);
        ::sg_import(src.animFrameMatrix, animFrameMatrix);
        ::sg_import(src.hasAnimFrameMatrix, hasAnimFrameMatrix);
        ::sg_import(src.airTime, airTime);
    }
};
//we save from top to boltUsed here. Don't bother saving the position, it gets rebuilt every frame anyway
#pragma pack(push, 4)
class SgBoltInfo
{
public:
    int32_t boneNumber;
    int32_t surfaceNumber;
    int32_t surfaceType;
    int32_t boltUsed;
}; // SgBoltInfo
#pragma pack(pop)

struct boltInfo_t{
    using SgType = SgBoltInfo;


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


    void sg_export(
        SgType& dst) const
    {
        ::sg_export(boneNumber, dst.boneNumber);
        ::sg_export(surfaceNumber, dst.surfaceNumber);
        ::sg_export(surfaceType, dst.surfaceType);
        ::sg_export(boltUsed, dst.boltUsed);
    }

    void sg_import(
        const SgType& src)
    {
        ::sg_import(src.boneNumber, boneNumber);
        ::sg_import(src.surfaceNumber, surfaceNumber);
        ::sg_import(src.surfaceType, surfaceType);
        ::sg_import(src.boltUsed, boltUsed);
    }
};


#define MAX_GHOUL_COUNT_BITS 8 // bits required to send across the MAX_G2_MODELS inside of the networking - this is the only restriction on ghoul models possible per entity

typedef std::vector <surfaceInfo_t> surfaceInfo_v;
typedef std::vector <boneInfo_t> boneInfo_v;
typedef std::vector <boltInfo_t> boltInfo_v;
typedef std::vector <mdxaBone_t> mdxaBone_v;

// defines for stuff to go into the mflags
#define		GHOUL2_NOCOLLIDE 0x001
#define		GHOUL2_NORENDER	 0x002
#define		GHOUL2_NOMODEL	 0x004
#define		GHOUL2_NEWORIGIN 0x008


// NOTE order in here matters. We save out from mModelindex to mFlags, but not the STL vectors that are at the top or the bottom.
class CBoneCache;
struct model_s;
//struct mdxaHeader_t;

#ifdef VV_GHOUL_HACKS
class CRenderableSurface
{
public:
	int				ident;			// ident of this surface - required so the materials renderer knows what sort of surface this refers to
 	CBoneCache 		*boneCache;		// pointer to transformed bone list for this surf
	mdxmSurface_t	*surfaceData;	// pointer to surface data loaded into file - only used by client renderer DO NOT USE IN GAME SIDE - if there is a vid restart this will be out of wack on the game

CRenderableSurface():
	ident(8), //SF_MDX
	boneCache(0),
	surfaceData(0)
	{}

CRenderableSurface(const CRenderableSurface& rs):
	ident(rs.ident),
	boneCache(rs.boneCache),
	surfaceData(rs.surfaceData)
	{}
};
#endif

#pragma pack(push, 4)
class SgCGhoul2Info
{
public:
    int32_t mModelindex;
    int32_t animModelIndexOffset;
    int32_t mCustomShader;
    int32_t mCustomSkin;
    int32_t mModelBoltLink;
    int32_t mSurfaceRoot;
    int32_t mLodBias;
    int32_t mNewOrigin;
#ifdef _G2_GORE
    int32_t mGoreSetTag;
#endif
    int32_t mModel;
    SgArray<char, MAX_QPATH> mFileName;
    int32_t mAnimFrameDefault;
    int32_t mSkelFrameNum;
    int32_t mMeshFrameNum;
    int32_t mFlags;
}; // SgCGhoul2Info
#pragma pack(pop)

class CGhoul2Info
{
public:
    using SgType = SgCGhoul2Info;


	surfaceInfo_v 	mSlist;
	boltInfo_v		mBltlist;
	boneInfo_v		mBlist;
// save from here (do not put any ptrs etc within this save block unless you adds special handlers to G2_SaveGhoul2Models / G2_LoadGhoul2Models!!!!!!!!!!!!
#define BSAVE_START_FIELD mModelindex	// this is the start point for loadsave, keep it up to date it you change anything
	int				mModelindex;
	int				animModelIndexOffset;
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
	intptr_t		*mTransformedVertsArray;	// used to create an array of pointers to transformed verts per surface for collision detection
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
	animModelIndexOffset(0),
	mCustomShader(0),
	mCustomSkin(0),
	mModelBoltLink(0),
	mSurfaceRoot(0),
	mLodBias(0),
	mNewOrigin(-1),
#ifdef _G2_GORE
	mGoreSetTag(0),
#endif
	mModel(0),
	mAnimFrameDefault(0),
	mSkelFrameNum(-1),
	mMeshFrameNum(-1),
	mFlags(0),
	mTransformedVertsArray(0),
	mBoneCache(0),
	mSkin(0),
	mValid(false),
	currentModel(0),
	currentModelSize(0),
	animModel(0),
	currentAnimModelSize(0),
	aHeader(0)
	{
		mFileName[0] = 0;
	}


    void sg_export(
        SgType& dst) const
    {
        ::sg_export(mModelindex, dst.mModelindex);
        ::sg_export(animModelIndexOffset, dst.animModelIndexOffset);
        ::sg_export(mCustomShader, dst.mCustomShader);
        ::sg_export(mCustomSkin, dst.mCustomSkin);
        ::sg_export(mModelBoltLink, dst.mModelBoltLink);
        ::sg_export(mSurfaceRoot, dst.mSurfaceRoot);
        ::sg_export(mLodBias, dst.mLodBias);
        ::sg_export(mNewOrigin, dst.mNewOrigin);
#ifdef _G2_GORE
        ::sg_export(mGoreSetTag, dst.mGoreSetTag);
#endif
        ::sg_export(mModel, dst.mModel);
        ::sg_export(mFileName, dst.mFileName);
        ::sg_export(mAnimFrameDefault, dst.mAnimFrameDefault);
        ::sg_export(mSkelFrameNum, dst.mSkelFrameNum);
        ::sg_export(mMeshFrameNum, dst.mMeshFrameNum);
        ::sg_export(mFlags, dst.mFlags);
    }

    void sg_import(
        const SgType& src)
    {
        ::sg_import(src.mModelindex, mModelindex);
        ::sg_import(src.animModelIndexOffset, animModelIndexOffset);
        ::sg_import(src.mCustomShader, mCustomShader);
        ::sg_import(src.mCustomSkin, mCustomSkin);
        ::sg_import(src.mModelBoltLink, mModelBoltLink);
        ::sg_import(src.mSurfaceRoot, mSurfaceRoot);
        ::sg_import(src.mLodBias, mLodBias);
        ::sg_import(src.mNewOrigin, mNewOrigin);
#ifdef _G2_GORE
        ::sg_import(src.mGoreSetTag, mGoreSetTag);
#endif
        ::sg_import(src.mModel, mModel);
        ::sg_import(src.mFileName, mFileName);
        ::sg_import(src.mAnimFrameDefault, mAnimFrameDefault);
        ::sg_import(src.mSkelFrameNum, mSkelFrameNum);
        ::sg_import(src.mMeshFrameNum, mMeshFrameNum);
        ::sg_import(src.mFlags, mFlags);
    }
};

class CGhoul2Info_v;

class IGhoul2InfoArray
{
public:
	virtual int New()=0;
	virtual void Delete(int handle)=0;
	virtual bool IsValid(int handle) const=0;
	virtual std::vector<CGhoul2Info> &Get(int handle)=0;
	virtual const std::vector<CGhoul2Info> &Get(int handle) const=0;
};

#ifdef RENDERER
IGhoul2InfoArray &TheGhoul2InfoArray();
#elif _JK2EXE
IGhoul2InfoArray &_TheGhoul2InfoArray();
#else
IGhoul2InfoArray &TheGameGhoul2InfoArray();
#endif

#pragma pack(push, 4)
class SgCGhoul2InfoV
{
public:
    int32_t mItem;
}; // SgCGhoul2InfoV
#pragma pack(pop)

class CGhoul2Info_v
{
	int mItem;

	IGhoul2InfoArray &InfoArray() const
	{
#ifdef RENDERER
		return TheGhoul2InfoArray();
#elif _JK2EXE
		return _TheGhoul2InfoArray();
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
	std::vector<CGhoul2Info> &Array()
	{
		assert(InfoArray().IsValid(mItem));
		return InfoArray().Get(mItem);
	}
	const std::vector<CGhoul2Info> &Array() const
	{
		assert(InfoArray().IsValid(mItem));
		return InfoArray().Get(mItem);
	}
public:
    using SgType = SgCGhoul2InfoV;


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


    void sg_export(
        SgType& dst) const
    {
        ::sg_export(mItem, dst.mItem);
    }

    void sg_import(
        const SgType& src)
    {
        ::sg_import(src.mItem, mItem);
    }
};



// collision detection stuff
#define G2_FRONTFACE 1
#define	G2_BACKFACE	 0

#pragma pack(push, 4)
class SgCCollisionRecord
{
public:
    float mDistance;
    int32_t mEntityNum;
    int32_t mModelIndex;
    int32_t mPolyIndex;
    int32_t mSurfaceIndex;
    SgVec3 mCollisionPosition;
    SgVec3 mCollisionNormal;
    int32_t mFlags;
    int32_t mMaterial;
    int32_t mLocation;
    float mBarycentricI;
    float mBarycentricJ;
}; // SgCCollisionRecord
#pragma pack(pop)

class CCollisionRecord
{
public:
    using SgType = SgCCollisionRecord;


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
	mDistance(100000),
	mEntityNum(-1)
	{}


    void sg_export(
        SgType& dst) const
    {
        ::sg_export(mDistance, dst.mDistance);
        ::sg_export(mEntityNum, dst.mEntityNum);
        ::sg_export(mModelIndex, dst.mModelIndex);
        ::sg_export(mPolyIndex, dst.mPolyIndex);
        ::sg_export(mSurfaceIndex, dst.mSurfaceIndex);
        ::sg_export(mCollisionPosition, dst.mCollisionPosition);
        ::sg_export(mCollisionNormal, dst.mCollisionNormal);
        ::sg_export(mFlags, dst.mFlags);
        ::sg_export(mMaterial, dst.mMaterial);
        ::sg_export(mLocation, dst.mLocation);
        ::sg_export(mBarycentricI, dst.mBarycentricI);
        ::sg_export(mBarycentricJ, dst.mBarycentricJ);
    }

    void sg_import(
        const SgType& src)
    {
        ::sg_import(src.mDistance, mDistance);
        ::sg_import(src.mEntityNum, mEntityNum);
        ::sg_import(src.mModelIndex, mModelIndex);
        ::sg_import(src.mPolyIndex, mPolyIndex);
        ::sg_import(src.mSurfaceIndex, mSurfaceIndex);
        ::sg_import(src.mCollisionPosition, mCollisionPosition);
        ::sg_import(src.mCollisionNormal, mCollisionNormal);
        ::sg_import(src.mFlags, mFlags);
        ::sg_import(src.mMaterial, mMaterial);
        ::sg_import(src.mLocation, mLocation);
        ::sg_import(src.mBarycentricI, mBarycentricI);
        ::sg_import(src.mBarycentricJ, mBarycentricJ);
    }
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
