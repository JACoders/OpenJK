#include "qcommon/q_shared.h"
#include "../game/ghoul2_shared.h"
#include "qcommon/ojk_sg_wrappers.h"


void surfaceInfo_t::sg_export(
    SgType& dst) const
{
    ::sg_export(offFlags, dst.offFlags);
    ::sg_export(surface, dst.surface);
    ::sg_export(genBarycentricJ, dst.genBarycentricJ);
    ::sg_export(genBarycentricI, dst.genBarycentricI);
    ::sg_export(genPolySurfaceIndex, dst.genPolySurfaceIndex);
    ::sg_export(genLod, dst.genLod);
}

void surfaceInfo_t::sg_import(
    const SgType& src)
{
    ::sg_import(src.offFlags, offFlags);
    ::sg_import(src.surface, surface);
    ::sg_import(src.genBarycentricJ, genBarycentricJ);
    ::sg_import(src.genBarycentricI, genBarycentricI);
    ::sg_import(src.genPolySurfaceIndex, genPolySurfaceIndex);
    ::sg_import(src.genLod, genLod);
}


void boneInfo_t::sg_export(
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

void boneInfo_t::sg_import(
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


void boltInfo_t::sg_export(
    SgType& dst) const
{
    ::sg_export(boneNumber, dst.boneNumber);
    ::sg_export(surfaceNumber, dst.surfaceNumber);
    ::sg_export(surfaceType, dst.surfaceType);
    ::sg_export(boltUsed, dst.boltUsed);
}

void boltInfo_t::sg_import(
    const SgType& src)
{
    ::sg_import(src.boneNumber, boneNumber);
    ::sg_import(src.surfaceNumber, surfaceNumber);
    ::sg_import(src.surfaceType, surfaceType);
    ::sg_import(src.boltUsed, boltUsed);
}


void CGhoul2Info::sg_export(
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

void CGhoul2Info::sg_import(
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


void CGhoul2Info_v::sg_export(
    SgType& dst) const
{
    ::sg_export(mItem, dst.mItem);
}

void CGhoul2Info_v::sg_import(
    const SgType& src)
{
    ::sg_import(src.mItem, mItem);
}
