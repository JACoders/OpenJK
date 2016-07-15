#include "qcommon/q_shared.h"
#include "../game/ghoul2_shared.h"
#include "qcommon/ojk_i_saved_game.h"


void surfaceInfo_t::sg_export(
    ojk::ISavedGame* saved_game) const
{
    saved_game->write<int32_t>(offFlags);
    saved_game->write<int32_t>(surface);
    saved_game->write<float>(genBarycentricJ);
    saved_game->write<float>(genBarycentricI);
    saved_game->write<int32_t>(genPolySurfaceIndex);
    saved_game->write<int32_t>(genLod);
}

void surfaceInfo_t::sg_import(
    ojk::ISavedGame* saved_game)
{
    saved_game->read<int32_t>(offFlags);
    saved_game->read<int32_t>(surface);
    saved_game->read<float>(genBarycentricJ);
    saved_game->read<float>(genBarycentricI);
    saved_game->read<int32_t>(genPolySurfaceIndex);
    saved_game->read<int32_t>(genLod);
}


void boneInfo_t::sg_export(
    ojk::ISavedGame* saved_game) const
{
    saved_game->write<int32_t>(boneNumber);
    saved_game->write<>(matrix);
    saved_game->write<int32_t>(flags);
    saved_game->write<int32_t>(startFrame);
    saved_game->write<int32_t>(endFrame);
    saved_game->write<int32_t>(startTime);
    saved_game->write<int32_t>(pauseTime);
    saved_game->write<float>(animSpeed);
    saved_game->write<float>(blendFrame);
    saved_game->write<int32_t>(blendLerpFrame);
    saved_game->write<int32_t>(blendTime);
    saved_game->write<int32_t>(blendStart);
    saved_game->write<int32_t>(boneBlendTime);
    saved_game->write<int32_t>(boneBlendStart);
    saved_game->write(newMatrix);
    saved_game->write<int32_t>(lastTimeUpdated);
    saved_game->write<int32_t>(lastContents);
    saved_game->write<float>(lastPosition);
    saved_game->write<float>(velocityEffector);
    saved_game->write<float>(lastAngles);
    saved_game->write<float>(minAngles);
    saved_game->write<float>(maxAngles);
    saved_game->write<float>(currentAngles);
    saved_game->write<float>(anglesOffset);
    saved_game->write<float>(positionOffset);
    saved_game->write<float>(radius);
    saved_game->write<float>(weight);
    saved_game->write<int32_t>(ragIndex);
    saved_game->write<float>(velocityRoot);
    saved_game->write<int32_t>(ragStartTime);
    saved_game->write<int32_t>(firstTime);
    saved_game->write<int32_t>(firstCollisionTime);
    saved_game->write<int32_t>(restTime);
    saved_game->write<int32_t>(RagFlags);
    saved_game->write<int32_t>(DependentRagIndexMask);
    saved_game->write<>(originalTrueBoneMatrix);
    saved_game->write<>(parentTrueBoneMatrix);
    saved_game->write<>(parentOriginalTrueBoneMatrix);
    saved_game->write<float>(originalOrigin);
    saved_game->write<float>(originalAngles);
    saved_game->write<float>(lastShotDir);
    saved_game->write<int32_t>(basepose);
    saved_game->write<int32_t>(baseposeInv);
    saved_game->write<int32_t>(baseposeParent);
    saved_game->write<int32_t>(baseposeInvParent);
    saved_game->write<int32_t>(parentRawBoneIndex);
    saved_game->write<>(ragOverrideMatrix);
    saved_game->write<>(extraMatrix);
    saved_game->write<float>(extraVec1);
    saved_game->write<float>(extraFloat1);
    saved_game->write<int32_t>(extraInt1);
    saved_game->write<float>(ikPosition);
    saved_game->write<float>(ikSpeed);
    saved_game->write<float>(epVelocity);
    saved_game->write<float>(epGravFactor);
    saved_game->write<int32_t>(solidCount);
    saved_game->write<int8_t>(physicsSettled);
    saved_game->write<int8_t>(snapped);
    saved_game->write<int32_t>(parentBoneIndex);
    saved_game->write<float>(offsetRotation);
    saved_game->write<float>(overGradSpeed);
    saved_game->write<float>(overGoalSpot);
    saved_game->write<int8_t>(hasOverGoal);
    saved_game->write<>(animFrameMatrix);
    saved_game->write<int32_t>(hasAnimFrameMatrix);
    saved_game->write<int32_t>(airTime);
}

void boneInfo_t::sg_import(
    ojk::ISavedGame* saved_game)
{
    saved_game->read<int32_t>(boneNumber);
    saved_game->read<>(matrix);
    saved_game->read<int32_t>(flags);
    saved_game->read<int32_t>(startFrame);
    saved_game->read<int32_t>(endFrame);
    saved_game->read<int32_t>(startTime);
    saved_game->read<int32_t>(pauseTime);
    saved_game->read<float>(animSpeed);
    saved_game->read<float>(blendFrame);
    saved_game->read<int32_t>(blendLerpFrame);
    saved_game->read<int32_t>(blendTime);
    saved_game->read<int32_t>(blendStart);
    saved_game->read<int32_t>(boneBlendTime);
    saved_game->read<int32_t>(boneBlendStart);
    saved_game->read(newMatrix);
    saved_game->read<int32_t>(lastTimeUpdated);
    saved_game->read<int32_t>(lastContents);
    saved_game->read<float>(lastPosition);
    saved_game->read<float>(velocityEffector);
    saved_game->read<float>(lastAngles);
    saved_game->read<float>(minAngles);
    saved_game->read<float>(maxAngles);
    saved_game->read<float>(currentAngles);
    saved_game->read<float>(anglesOffset);
    saved_game->read<float>(positionOffset);
    saved_game->read<float>(radius);
    saved_game->read<float>(weight);
    saved_game->read<int32_t>(ragIndex);
    saved_game->read<float>(velocityRoot);
    saved_game->read<int32_t>(ragStartTime);
    saved_game->read<int32_t>(firstTime);
    saved_game->read<int32_t>(firstCollisionTime);
    saved_game->read<int32_t>(restTime);
    saved_game->read<int32_t>(RagFlags);
    saved_game->read<int32_t>(DependentRagIndexMask);
    saved_game->read<>(originalTrueBoneMatrix);
    saved_game->read<>(parentTrueBoneMatrix);
    saved_game->read<>(parentOriginalTrueBoneMatrix);
    saved_game->read<float>(originalOrigin);
    saved_game->read<float>(originalAngles);
    saved_game->read<float>(lastShotDir);
    saved_game->read<int32_t>(basepose);
    saved_game->read<int32_t>(baseposeInv);
    saved_game->read<int32_t>(baseposeParent);
    saved_game->read<int32_t>(baseposeInvParent);
    saved_game->read<int32_t>(parentRawBoneIndex);
    saved_game->read<>(ragOverrideMatrix);
    saved_game->read<>(extraMatrix);
    saved_game->read<float>(extraVec1);
    saved_game->read<float>(extraFloat1);
    saved_game->read<int32_t>(extraInt1);
    saved_game->read<float>(ikPosition);
    saved_game->read<float>(ikSpeed);
    saved_game->read<float>(epVelocity);
    saved_game->read<float>(epGravFactor);
    saved_game->read<int32_t>(solidCount);
    saved_game->read<int8_t>(physicsSettled);
    saved_game->read<int8_t>(snapped);
    saved_game->read<int32_t>(parentBoneIndex);
    saved_game->read<float>(offsetRotation);
    saved_game->read<float>(overGradSpeed);
    saved_game->read<float>(overGoalSpot);
    saved_game->read<int8_t>(hasOverGoal);
    saved_game->read<>(animFrameMatrix);
    saved_game->read<int32_t>(hasAnimFrameMatrix);
    saved_game->read<int32_t>(airTime);
}


void boltInfo_t::sg_export(
    ojk::ISavedGame* saved_game) const
{
    saved_game->write<int32_t>(boneNumber);
    saved_game->write<int32_t>(surfaceNumber);
    saved_game->write<int32_t>(surfaceType);
    saved_game->write<int32_t>(boltUsed);
}

void boltInfo_t::sg_import(
    ojk::ISavedGame* saved_game)
{
    saved_game->read<int32_t>(boneNumber);
    saved_game->read<int32_t>(surfaceNumber);
    saved_game->read<int32_t>(surfaceType);
    saved_game->read<int32_t>(boltUsed);
}


void CGhoul2Info::sg_export(
    ojk::ISavedGame* saved_game) const
{
    saved_game->write<int32_t>(mModelindex);
    saved_game->write<int32_t>(animModelIndexOffset);
    saved_game->write<int32_t>(mCustomShader);
    saved_game->write<int32_t>(mCustomSkin);
    saved_game->write<int32_t>(mModelBoltLink);
    saved_game->write<int32_t>(mSurfaceRoot);
    saved_game->write<int32_t>(mLodBias);
    saved_game->write<int32_t>(mNewOrigin);
#ifdef _G2_GORE
    saved_game->write<int32_t>(mGoreSetTag);
#endif
    saved_game->write<int32_t>(mModel);
    saved_game->write<int8_t>(mFileName);
    saved_game->write<int32_t>(mAnimFrameDefault);
    saved_game->write<int32_t>(mSkelFrameNum);
    saved_game->write<int32_t>(mMeshFrameNum);
    saved_game->write<int32_t>(mFlags);
}

void CGhoul2Info::sg_import(
    ojk::ISavedGame* saved_game)
{
    saved_game->read<int32_t>(mModelindex);
    saved_game->read<int32_t>(animModelIndexOffset);
    saved_game->read<int32_t>(mCustomShader);
    saved_game->read<int32_t>(mCustomSkin);
    saved_game->read<int32_t>(mModelBoltLink);
    saved_game->read<int32_t>(mSurfaceRoot);
    saved_game->read<int32_t>(mLodBias);
    saved_game->read<int32_t>(mNewOrigin);
#ifdef _G2_GORE
    saved_game->read<int32_t>(mGoreSetTag);
#endif
    saved_game->read<int32_t>(mModel);
    saved_game->read<int8_t>(mFileName);
    saved_game->read<int32_t>(mAnimFrameDefault);
    saved_game->read<int32_t>(mSkelFrameNum);
    saved_game->read<int32_t>(mMeshFrameNum);
    saved_game->read<int32_t>(mFlags);
}


void CGhoul2Info_v::sg_export(
    ojk::ISavedGame* saved_game) const
{
    saved_game->write<int32_t>(mItem);
}

void CGhoul2Info_v::sg_import(
    ojk::ISavedGame* saved_game)
{
    saved_game->read<int32_t>(mItem);
}

void CCollisionRecord::sg_export(
    ojk::ISavedGame* saved_game) const
{
    saved_game->write<float>(mDistance);
    saved_game->write<int32_t>(mEntityNum);
    saved_game->write<int32_t>(mModelIndex);
    saved_game->write<int32_t>(mPolyIndex);
    saved_game->write<int32_t>(mSurfaceIndex);
    saved_game->write<float>(mCollisionPosition);
    saved_game->write<float>(mCollisionNormal);
    saved_game->write<int32_t>(mFlags);
    saved_game->write<int32_t>(mMaterial);
    saved_game->write<int32_t>(mLocation);
    saved_game->write<float>(mBarycentricI);
    saved_game->write<float>(mBarycentricJ);
}

void CCollisionRecord::sg_import(
    ojk::ISavedGame* saved_game)
{
    saved_game->read<float>(mDistance);
    saved_game->read<int32_t>(mEntityNum);
    saved_game->read<int32_t>(mModelIndex);
    saved_game->read<int32_t>(mPolyIndex);
    saved_game->read<int32_t>(mSurfaceIndex);
    saved_game->read<float>(mCollisionPosition);
    saved_game->read<float>(mCollisionNormal);
    saved_game->read<int32_t>(mFlags);
    saved_game->read<int32_t>(mMaterial);
    saved_game->read<int32_t>(mLocation);
    saved_game->read<float>(mBarycentricI);
    saved_game->read<float>(mBarycentricJ);
}
