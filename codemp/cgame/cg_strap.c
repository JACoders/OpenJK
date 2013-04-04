//rww - shared trap call system
#include "cg_local.h"

#include "../namespace_begin.h"

qboolean strap_G2API_GetBoltMatrix(void *ghoul2, const int modelIndex, const int boltIndex, mdxaBone_t *matrix,
								const vec3_t angles, const vec3_t position, const int frameNum, qhandle_t *modelList, vec3_t scale)
{
	return trap_G2API_GetBoltMatrix(ghoul2, modelIndex, boltIndex, matrix, angles, position, frameNum, modelList, scale);
}

qboolean strap_G2API_GetBoltMatrix_NoReconstruct(void *ghoul2, const int modelIndex, const int boltIndex, mdxaBone_t *matrix,
								const vec3_t angles, const vec3_t position, const int frameNum, qhandle_t *modelList, vec3_t scale)
{
	return trap_G2API_GetBoltMatrix_NoReconstruct(ghoul2, modelIndex, boltIndex, matrix, angles, position, frameNum, modelList, scale);
}

qboolean strap_G2API_GetBoltMatrix_NoRecNoRot(void *ghoul2, const int modelIndex, const int boltIndex, mdxaBone_t *matrix,
								const vec3_t angles, const vec3_t position, const int frameNum, qhandle_t *modelList, vec3_t scale)
{
	return trap_G2API_GetBoltMatrix_NoRecNoRot(ghoul2, modelIndex, boltIndex, matrix, angles, position, frameNum, modelList, scale);
}

qboolean strap_G2API_SetBoneAngles(void *ghoul2, int modelIndex, const char *boneName, const vec3_t angles, const int flags,
								const int up, const int right, const int forward, qhandle_t *modelList,
								int blendTime , int currentTime )
{
	return trap_G2API_SetBoneAngles(ghoul2, modelIndex, boneName, angles, flags, up, right, forward, modelList, blendTime, currentTime);
}

qboolean strap_G2API_SetBoneAnim(void *ghoul2, const int modelIndex, const char *boneName, const int startFrame, const int endFrame,
							  const int flags, const float animSpeed, const int currentTime, const float setFrame , const int blendTime )
{
	return trap_G2API_SetBoneAnim(ghoul2, modelIndex, boneName, startFrame, endFrame, flags, animSpeed, currentTime, setFrame, blendTime);
}

qboolean strap_G2API_GetBoneAnim(void *ghoul2, const char *boneName, const int currentTime, float *currentFrame,
						   int *startFrame, int *endFrame, int *flags, float *animSpeed, int *modelList, const int modelIndex)
{
	return trap_G2API_GetBoneAnim(ghoul2, boneName, currentTime, currentFrame, startFrame, endFrame, flags, animSpeed, modelList, modelIndex);
}

void strap_G2API_SetRagDoll(void *ghoul2, sharedRagDollParams_t *params)
{
	trap_G2API_SetRagDoll(ghoul2, params);
}

void strap_G2API_AnimateG2Models(void *ghoul2, int time, sharedRagDollUpdateParams_t *params)
{
	trap_G2API_AnimateG2Models(ghoul2, time, params);
}

qboolean strap_G2API_SetBoneIKState(void *ghoul2, int time, const char *boneName, int ikState, sharedSetBoneIKStateParams_t *params)
{
	return trap_G2API_SetBoneIKState(ghoul2, time, boneName, ikState, params);
}

qboolean strap_G2API_IKMove(void *ghoul2, int time, sharedIKMoveParams_t *params)
{
	return trap_G2API_IKMove(ghoul2, time, params);
}

void strap_TrueMalloc(void **ptr, int size)
{
	trap_TrueMalloc(ptr, size);
}

void strap_TrueFree(void **ptr)
{
	trap_TrueFree(ptr);
}

#include "../namespace_end.h"
