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

#include "qcommon/matcomp.h"

#include "ghoul2/G2.h"
#include "ghoul2/g2_local.h"

//rww - RAGDOLL_BEGIN
#ifndef __linux__
#include <float.h>
#else
#include <math.h>
#endif
#include "ghoul2/G2_gore.h"
#include "tr_local.h"

//#define RAG_TRACE_DEBUG_LINES

#include "client/client.h" //while this is all "shared" code, there are some places where we want to make cgame callbacks (for ragdoll) only if the cgvm exists
//rww - RAGDOLL_END

//=====================================================================================================================
// Bone List handling routines - so entities can override bone info on a bone by bone level, and also interrogate this info

// Given a bone name, see if that bone is already in our bone list - note the model_t pointer that gets passed in here MUST point at the
// gla file, not the glm file type.
int G2_Find_Bone(const model_t *mod, boneInfo_v &blist, const char *boneName)
{
	mdxaSkel_t			*skel;
	mdxaSkelOffsets_t	*offsets;
   	offsets = (mdxaSkelOffsets_t *)((byte *)mod->mdxa + sizeof(mdxaHeader_t));
	skel = (mdxaSkel_t *)((byte *)mod->mdxa + sizeof(mdxaHeader_t) + offsets->offsets[0]);

	// look through entire list
	for(size_t i=0; i<blist.size(); i++)
	{
		// if this bone entry has no info in it, bounce over it
		if (blist[i].boneNumber == -1)
		{
			continue;
		}

		// figure out what skeletal info structure this bone entry is looking at
		skel = (mdxaSkel_t *)((byte *)mod->mdxa + sizeof(mdxaHeader_t) + offsets->offsets[blist[i].boneNumber]);

		// if name is the same, we found it
		if (!Q_stricmp(skel->name, boneName))
		{
			return i;
		}
	}

	// didn't find it
	return -1;
}

// we need to add a bone to the list - find a free one and see if we can find a corresponding bone in the gla file
int G2_Add_Bone (const model_t *mod, boneInfo_v &blist, const char *boneName)
{
	int x;
	mdxaSkel_t			*skel;
	mdxaSkelOffsets_t	*offsets;
	boneInfo_t			tempBone;

	//rww - RAGDOLL_BEGIN
	memset(&tempBone, 0, sizeof(tempBone));
	//rww - RAGDOLL_END

   	offsets = (mdxaSkelOffsets_t *)((byte *)mod->mdxa + sizeof(mdxaHeader_t));

 	// walk the entire list of bones in the gla file for this model and see if any match the name of the bone we want to find
 	for (x=0; x< mod->mdxa->numBones; x++)
 	{
 		skel = (mdxaSkel_t *)((byte *)mod->mdxa + sizeof(mdxaHeader_t) + offsets->offsets[x]);
 		// if name is the same, we found it
 		if (!Q_stricmp(skel->name, boneName))
		{
			break;
		}
	}

	// check to see we did actually make a match with a bone in the model
	if (x == mod->mdxa->numBones)
	{
		// didn't find it? Error
		//assert(0);
#ifdef _DEBUG
		ri.Printf( PRINT_ALL, "WARNING: Failed to add bone %s\n", boneName);
#endif

#ifdef _RAG_PRINT_TEST
		ri.Printf( PRINT_ALL, "WARNING: Failed to add bone %s\n", boneName);
#endif
		return -1;
	}

	// look through entire list - see if it's already there first
	for(size_t i=0; i<blist.size(); i++)
	{
		// if this bone entry has info in it, bounce over it
		if (blist[i].boneNumber != -1)
		{
			skel = (mdxaSkel_t *)((byte *)mod->mdxa + sizeof(mdxaHeader_t) + offsets->offsets[blist[i].boneNumber]);
			// if name is the same, we found it
			if (!Q_stricmp(skel->name, boneName))
			{
				return i;
			}
		}
		else
		{
			// if we found an entry that had a -1 for the bonenumber, then we hit a bone slot that was empty
			blist[i].boneNumber = x;
			blist[i].flags = 0;
	 		return i;
		}
	}

#ifdef _RAG_PRINT_TEST
	ri.Printf( PRINT_ALL, "New bone added for %s\n", boneName);
#endif
	// ok, we didn't find an existing bone of that name, or an empty slot. Lets add an entry
	tempBone.boneNumber = x;
	tempBone.flags = 0;
	blist.push_back(tempBone);
	return blist.size()-1;
}


// Given a model handle, and a bone name, we want to remove this bone from the bone override list
qboolean G2_Remove_Bone_Index ( boneInfo_v &blist, int index)
{
	if (index != -1)
	{
		if (blist[index].flags & BONE_ANGLES_RAGDOLL)
		{
			return qtrue; // don't accept any calls on ragdoll bones
		}
	}

	// did we find it?
	if (index != -1)
	{
		// check the flags first - if it's still being used Do NOT remove it
		if (!blist[index].flags)
		{

			// set this bone to not used
			blist[index].boneNumber = -1;

		   	unsigned int newSize = blist.size();
			// now look through the list from the back and see if there is a block of -1's we can resize off the end of the list
			for (int i=blist.size()-1; i>-1; i--)
			{
				if (blist[i].boneNumber == -1)
				{
					newSize = i;
				}
				// once we hit one that isn't a -1, we are done.
				else
				{
					break;
				}
			}
			// do we need to resize?
			if (newSize != blist.size())
			{
				// yes, so lets do it
				blist.resize(newSize);
			}

			return qtrue;
		}
	}

//	assert(0);
	// no
	return qfalse;
}

// given a bone number, see if there is an override bone in the bone list
int	G2_Find_Bone_In_List(boneInfo_v &blist, const int boneNum)
{
	// look through entire list
	for(size_t i=0; i<blist.size(); i++)
	{
		if (blist[i].boneNumber == boneNum)
		{
			return i;
		}
	}
	return -1;
}

// given a model, bonelist and bonename, lets stop an anim if it's playing.
qboolean G2_Stop_Bone_Index( boneInfo_v &blist, int index, int flags)
{
	// did we find it?
	if (index != -1)
	{
		blist[index].flags &= ~(flags);
		// try and remove this bone if we can
		return G2_Remove_Bone_Index(blist, index);
	}

	assert(0);

	return qfalse;
}

// generate a matrix for a given bone given some new angles for it.
void G2_Generate_Matrix(const model_t *mod, boneInfo_v &blist, int index, const float *angles, int flags,
						const Eorientations up, const Eorientations left, const Eorientations forward)
{
	mdxaSkel_t		*skel;
	mdxaSkelOffsets_t *offsets;
	mdxaBone_t		temp1;
	mdxaBone_t		permutation;
	mdxaBone_t		*boneOverride = &blist[index].matrix;
	vec3_t			newAngles;

	if (flags & (BONE_ANGLES_PREMULT | BONE_ANGLES_POSTMULT))
	{
		// build us a matrix out of the angles we are fed - but swap y and z because of wacky Quake setup
		vec3_t	newAngles;

		// determine what axis newAngles Yaw should revolve around
		switch (up)
		{
		case NEGATIVE_X:
			newAngles[1] = angles[2] + 180;
			break;
		case POSITIVE_X:
			newAngles[1] = angles[2];
			break;
		case NEGATIVE_Y:
			newAngles[1] = angles[0];
			break;
		case POSITIVE_Y:
			newAngles[1] = angles[0];
			break;
		case NEGATIVE_Z:
			newAngles[1] = angles[1] + 180;
			break;
		case POSITIVE_Z:
			newAngles[1] = angles[1];
			break;
		default:
			break;
		}

		// determine what axis newAngles pitch should revolve around
		switch (left)
		{
		case NEGATIVE_X:
			newAngles[0] = angles[2];
			break;
		case POSITIVE_X:
			newAngles[0] = angles[2] + 180;
			break;
		case NEGATIVE_Y:
			newAngles[0] = angles[0];
			break;
		case POSITIVE_Y:
			newAngles[0] = angles[0] + 180;
			break;
		case NEGATIVE_Z:
			newAngles[0] = angles[1];
			break;
		case POSITIVE_Z:
			newAngles[0] = angles[1];
			break;
		default:
			break;
		}

		// determine what axis newAngles Roll should revolve around
		switch (forward)
		{
		case NEGATIVE_X:
			newAngles[2] = angles[2];
			break;
		case POSITIVE_X:
			newAngles[2] = angles[2];
			break;
		case NEGATIVE_Y:
			newAngles[2] = angles[0];
			break;
		case POSITIVE_Y:
			newAngles[2] = angles[0] + 180;
			break;
		case NEGATIVE_Z:
			newAngles[2] = angles[1];
			break;
		case POSITIVE_Z:
			newAngles[2] = angles[1] + 180;
			break;
		default:
			break;
		}

		Create_Matrix(newAngles, boneOverride);

		// figure out where the bone hirearchy info is
		offsets = (mdxaSkelOffsets_t *)((byte *)mod->mdxa + sizeof(mdxaHeader_t));
		skel = (mdxaSkel_t *)((byte *)mod->mdxa + sizeof(mdxaHeader_t) + offsets->offsets[blist[index].boneNumber]);

		Multiply_3x4Matrix(&temp1,  boneOverride,&skel->BasePoseMatInv);
		Multiply_3x4Matrix(boneOverride,&skel->BasePoseMat, &temp1);

	}
	else
	{
		VectorCopy(angles, newAngles);

		// why I should need do this Fuck alone knows. But I do.
		if (left == POSITIVE_Y)
		{
			newAngles[0] +=180;
		}

		Create_Matrix(newAngles, &temp1);

		permutation.matrix[0][0] = permutation.matrix[0][1] = permutation.matrix[0][2] = permutation.matrix[0][3] = 0;
		permutation.matrix[1][0] = permutation.matrix[1][1] = permutation.matrix[1][2] = permutation.matrix[1][3] = 0;
		permutation.matrix[2][0] = permutation.matrix[2][1] = permutation.matrix[2][2] = permutation.matrix[2][3] = 0;

		// determine what axis newAngles Yaw should revolve around
		switch (forward)
		{
		case NEGATIVE_X:
			permutation.matrix[0][0] = -1;		// works
			break;
		case POSITIVE_X:
			permutation.matrix[0][0] = 1;		// works
			break;
		case NEGATIVE_Y:
			permutation.matrix[1][0] = -1;
			break;
		case POSITIVE_Y:
			permutation.matrix[1][0] = 1;
			break;
		case NEGATIVE_Z:
			permutation.matrix[2][0] = -1;
			break;
		case POSITIVE_Z:
			permutation.matrix[2][0] = 1;
			break;
		default:
			break;
		}

		// determine what axis newAngles pitch should revolve around
		switch (left)
		{
		case NEGATIVE_X:
			permutation.matrix[0][1] = -1;
			break;
		case POSITIVE_X:
			permutation.matrix[0][1] = 1;
			break;
		case NEGATIVE_Y:
			permutation.matrix[1][1] = -1;		// works
			break;
		case POSITIVE_Y:
			permutation.matrix[1][1] = 1;		// works
			break;
		case NEGATIVE_Z:
			permutation.matrix[2][1] = -1;
			break;
		case POSITIVE_Z:
			permutation.matrix[2][1] = 1;
			break;
		default:
			break;
		}

		// determine what axis newAngles Roll should revolve around
		switch (up)
		{
		case NEGATIVE_X:
			permutation.matrix[0][2] = -1;
			break;
		case POSITIVE_X:
			permutation.matrix[0][2] = 1;
			break;
		case NEGATIVE_Y:
			permutation.matrix[1][2] = -1;
			break;
		case POSITIVE_Y:
			permutation.matrix[1][2] = 1;
			break;
		case NEGATIVE_Z:
			permutation.matrix[2][2] = -1;		// works
			break;
		case POSITIVE_Z:
			permutation.matrix[2][2] = 1;		// works
			break;
		default:
			break;
		}

		Multiply_3x4Matrix(boneOverride, &temp1,&permutation);

	}

	// keep a copy of the matrix in the newmatrix which is actually what we use
	memcpy(&blist[index].newMatrix, &blist[index].matrix, sizeof(mdxaBone_t));

}

//=========================================================================================
//// Public Bone Routines


// Given a model handle, and a bone name, we want to remove this bone from the bone override list
qboolean G2_Remove_Bone (CGhoul2Info *ghlInfo, boneInfo_v &blist, const char *boneName)
{
	int index;

	assert(ghlInfo->animModel);
	index = G2_Find_Bone(ghlInfo->animModel, blist, boneName);

	return G2_Remove_Bone_Index(blist, index);
}

#define DEBUG_PCJ (0)


// Given a model handle, and a bone name, we want to set angles specifically for overriding
qboolean G2_Set_Bone_Angles_Index( boneInfo_v &blist, const int index,
							const float *angles, const int flags, const Eorientations yaw,
							const Eorientations pitch, const Eorientations roll, qhandle_t *modelList,
							const int modelIndex, const int blendTime, const int currentTime)
{
	if ((index >= (int)blist.size()) || (blist[index].boneNumber == -1))
	{
		// we are attempting to set a bone override that doesn't exist
		assert(0);
		return qfalse;
	}

	if (index != -1)
	{
		if (blist[index].flags & BONE_ANGLES_RAGDOLL)
		{
			return qtrue; // don't accept any calls on ragdoll bones
		}
	}

	if (flags & (BONE_ANGLES_PREMULT | BONE_ANGLES_POSTMULT))
	{
		// you CANNOT call this with an index with these kinds of bone overrides - we need the model details for these kinds of bone angle overrides
		assert(0);
		return qfalse;
	}

	// yes, so set the angles and flags correctly
	blist[index].flags &= ~(BONE_ANGLES_TOTAL);
	blist[index].flags |= flags;
	blist[index].boneBlendStart = currentTime;
	blist[index].boneBlendTime = blendTime;
#if DEBUG_PCJ
	Com_OPrintf("PCJ %2d %6d   (%6.2f,%6.2f,%6.2f) %d %d %d %d\n",index,currentTime,angles[0],angles[1],angles[2],yaw,pitch,roll,flags);
#endif

	G2_Generate_Matrix(NULL, blist, index, angles, flags, yaw, pitch, roll);
	return qtrue;

}

// Given a model handle, and a bone name, we want to set angles specifically for overriding
qboolean G2_Set_Bone_Angles(CGhoul2Info *ghlInfo, boneInfo_v &blist, const char *boneName, const float *angles,
							const int flags, const Eorientations up, const Eorientations left, const Eorientations forward,
							qhandle_t *modelList, const int modelIndex, const int blendTime, const int currentTime)
{
	model_t		*mod_a;

	mod_a = (model_t *)ghlInfo->animModel;

	int			index = G2_Find_Bone(mod_a, blist, boneName);

	// did we find it?
	if (index != -1)
	{
		if (blist[index].flags & BONE_ANGLES_RAGDOLL)
		{
			return qtrue; // don't accept any calls on ragdoll bones
		}

		// yes, so set the angles and flags correctly
		blist[index].flags &= ~(BONE_ANGLES_TOTAL);
		blist[index].flags |= flags;
		blist[index].boneBlendStart = currentTime;
		blist[index].boneBlendTime = blendTime;
#if DEBUG_PCJ
		Com_OPrintf("%2d %6d   (%6.2f,%6.2f,%6.2f) %d %d %d %d\n",index,currentTime,angles[0],angles[1],angles[2],up,left,forward,flags);
#endif

		G2_Generate_Matrix(mod_a, blist, index, angles, flags, up, left, forward);
		return qtrue;
	}

	// no - lets try and add this bone in
	index = G2_Add_Bone(mod_a, blist, boneName);

	// did we find a free one?
	if (index != -1)
	{
		// yes, so set the angles and flags correctly
		blist[index].flags &= ~(BONE_ANGLES_TOTAL);
		blist[index].flags |= flags;
		blist[index].boneBlendStart = currentTime;
		blist[index].boneBlendTime = blendTime;
#if DEBUG_PCJ
		Com_OPrintf("%2d %6d   (%6.2f,%6.2f,%6.2f) %d %d %d %d\n",index,currentTime,angles[0],angles[1],angles[2],up,left,forward,flags);
#endif

		G2_Generate_Matrix(mod_a, blist, index, angles, flags, up, left, forward);
		return qtrue;
	}
//	assert(0);
	//Jeese, we don't need an assert here too. There's already a warning in G2_Add_Bone if it fails.

	// no
	return qfalse;
}

// Given a model handle, and a bone name, we want to set angles specifically for overriding - using a matrix directly
qboolean G2_Set_Bone_Angles_Matrix_Index(boneInfo_v &blist, const int index,
								   const mdxaBone_t &matrix, const int flags, qhandle_t *modelList,
								   const int modelIndex, const int blendTime, const int currentTime)
{

	if ((index >= (int)blist.size()) || (blist[index].boneNumber == -1))
	{
		// we are attempting to set a bone override that doesn't exist
		assert(0);
		return qfalse;
	}
	if (index != -1)
	{
		if (blist[index].flags & BONE_ANGLES_RAGDOLL)
		{
			return qtrue; // don't accept any calls on ragdoll bones
		}
	}
	// yes, so set the angles and flags correctly
	blist[index].flags &= ~(BONE_ANGLES_TOTAL);
	blist[index].flags |= flags;
	blist[index].boneBlendStart = currentTime;
	blist[index].boneBlendTime = blendTime;

	memcpy(&blist[index].matrix, &matrix, sizeof(mdxaBone_t));
	memcpy(&blist[index].newMatrix, &matrix, sizeof(mdxaBone_t));
	return qtrue;

}

// Given a model handle, and a bone name, we want to set angles specifically for overriding - using a matrix directly
qboolean G2_Set_Bone_Angles_Matrix(const char *fileName, boneInfo_v &blist, const char *boneName, const mdxaBone_t &matrix,
								   const int flags, qhandle_t *modelList, const int modelIndex, const int blendTime, const int currentTime)
{
		model_t		*mod_m;
	if (!fileName[0])
	{
		mod_m = R_GetModelByHandle(modelList[modelIndex]);
	}
	else
	{
		mod_m = R_GetModelByHandle(RE_RegisterModel(fileName));
	}
	model_t		*mod_a = R_GetModelByHandle(mod_m->mdxm->animIndex);
	int			index = G2_Find_Bone(mod_a, blist, boneName);

	if (index != -1)
	{
		if (blist[index].flags & BONE_ANGLES_RAGDOLL)
		{
			return qtrue; // don't accept any calls on ragdoll bones
		}
	}

	// did we find it?
	if (index != -1)
	{
		// yes, so set the angles and flags correctly
		blist[index].flags &= ~(BONE_ANGLES_TOTAL);
		blist[index].flags |= flags;

		memcpy(&blist[index].matrix, &matrix, sizeof(mdxaBone_t));
		memcpy(&blist[index].newMatrix, &matrix, sizeof(mdxaBone_t));
		return qtrue;
	}

	// no - lets try and add this bone in
	index = G2_Add_Bone(mod_a, blist, boneName);

	// did we find a free one?
	if (index != -1)
	{
		// yes, so set the angles and flags correctly
		blist[index].flags &= ~(BONE_ANGLES_TOTAL);
		blist[index].flags |= flags;

		memcpy(&blist[index].matrix, &matrix, sizeof(mdxaBone_t));
		memcpy(&blist[index].newMatrix, &matrix, sizeof(mdxaBone_t));
		return qtrue;
	}
	assert(0);

	// no
	return qfalse;
}

#define DEBUG_G2_TIMING (0)

// given a model, bone name, a bonelist, a start/end frame number, a anim speed and some anim flags, set up or modify an existing bone entry for a new set of anims
qboolean G2_Set_Bone_Anim_Index(
	boneInfo_v &blist,
	const int index,
	const int startFrame,
	const int endFrame,
	const int flags,
	const float animSpeed,
	const int currentTime,
	const float setFrame,
	const int blendTime,
	const int numFrames)
{
	int			modFlags = flags;

	if ((index >= (int)blist.size()) || (blist[index].boneNumber == -1))
	{
		// we are attempting to set a bone override that doesn't exist
		assert(0);
		return qfalse;
	}

	if (index != -1)
	{
		if (blist[index].flags & BONE_ANGLES_RAGDOLL)
		{
			return qtrue; // don't accept any calls on ragdoll bones
		}

		//mark it for needing a transform for the cached trace transform stuff
		blist[index].flags |= BONE_NEED_TRANSFORM;
	}

	if (setFrame != -1)
	{
		assert((setFrame >= startFrame) && (setFrame <= endFrame));
	}
	if (flags & BONE_ANIM_BLEND)
	{
		float	currentFrame, animSpeed;
		int 	startFrame, endFrame, flags;
		// figure out where we are now
		if (G2_Get_Bone_Anim_Index(blist, index, currentTime, &currentFrame, &startFrame, &endFrame, &flags, &animSpeed, NULL, numFrames))
		{
			if (blist[index].blendStart == currentTime)	//we're replacing a blend in progress which hasn't started
			{
				// set the amount of time it's going to take to blend this anim with the last frame of the last one
				blist[index].blendTime = blendTime;
			}
			else
			{
				if (animSpeed<0.0f)
				{
					blist[index].blendFrame = floor(currentFrame);
					blist[index].blendLerpFrame = floor(currentFrame);
				}
				else
				{
					blist[index].blendFrame = currentFrame;
					blist[index].blendLerpFrame = currentFrame+1;

					// cope with if the lerp frame is actually off the end of the anim
					if (blist[index].blendFrame >= endFrame )
					{
						// we only want to lerp with the first frame of the anim if we are looping
						if (blist[index].flags & BONE_ANIM_OVERRIDE_LOOP)
						{
							blist[index].blendFrame = startFrame;
						}
						// if we intend to end this anim or freeze after this, then just keep on the last frame
						else
						{
						//	assert(endFrame>0);
							if (endFrame <= 0)
							{
								blist[index].blendLerpFrame = 0;
							}
							else
							{
								blist[index].blendFrame = endFrame -1;
							}
						}
					}

					// cope with if the lerp frame is actually off the end of the anim
					if (blist[index].blendLerpFrame >= endFrame )
					{
						// we only want to lerp with the first frame of the anim if we are looping
						if (blist[index].flags & BONE_ANIM_OVERRIDE_LOOP)
						{
							blist[index].blendLerpFrame = startFrame;
						}
						// if we intend to end this anim or freeze after this, then just keep on the last frame
						else
						{
						//	assert(endFrame>0);
							if (endFrame <= 0)
							{
								blist[index].blendLerpFrame = 0;
							}
							else
							{
								blist[index].blendLerpFrame = endFrame - 1;
							}
						}
					}
				}
				// set the amount of time it's going to take to blend this anim with the last frame of the last one
				blist[index].blendTime = blendTime;
				blist[index].blendStart = currentTime;

			}
		}
		// hmm, we weren't animating on this bone. In which case disable the blend
		else
		{
			blist[index].blendFrame = blist[index].blendLerpFrame = 0;
			blist[index].blendTime = 0;
			modFlags &= ~(BONE_ANIM_BLEND);
		}
	}
	else
	{
		blist[index].blendFrame = blist[index].blendLerpFrame = 0;
		blist[index].blendTime = blist[index].blendStart = 0;
		// we aren't blending, so remove the option to do so
		modFlags &= ~BONE_ANIM_BLEND;
	}
	// yes, so set the anim data and flags correctly
	blist[index].endFrame = endFrame;
	blist[index].startFrame = startFrame;
	blist[index].animSpeed = animSpeed;
	blist[index].pauseTime = 0;
	// start up the animation:)
	if (setFrame != -1)
	{
		blist[index].lastTime = blist[index].startTime = (currentTime - (((setFrame - (float)startFrame) * 50.0)/ animSpeed));
	}
	else
	{
		blist[index].lastTime = blist[index].startTime = currentTime;
	}
	blist[index].flags &= ~(BONE_ANIM_TOTAL);
	if (blist[index].flags < 0)
	{
		blist[index].flags = 0;
	}
	blist[index].flags |= modFlags;

#if DEBUG_G2_TIMING
	if (index==2)
	{
		const boneInfo_t &bone=blist[index];
		char mess[1000];
		if (bone.flags&BONE_ANIM_BLEND)
		{
			sprintf(mess,"sab[%2d] %5d  %5d  (%5d-%5d) %4.2f %4x   bt(%5d-%5d) %7.2f %5d\n",
				index,
				currentTime,
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
			sprintf(mess,"saa[%2d] %5d  %5d  (%5d-%5d) %4.2f %4x\n",
				index,
				currentTime,
				bone.startTime,
				bone.startFrame,
				bone.endFrame,
				bone.animSpeed,
				bone.flags
				);
		}
		Com_OPrintf("%s",mess);
	}
#endif

	return qtrue;

}

// given a model, bone name, a bonelist, a start/end frame number, a anim speed and some anim flags, set up or modify an existing bone entry for a new set of anims
qboolean G2_Set_Bone_Anim(CGhoul2Info *ghlInfo,
						  boneInfo_v &blist,
						  const char *boneName,
						  const int startFrame,
						  const int endFrame,
						  const int flags,
						  const float animSpeed,
						  const int currentTime,
						  const float setFrame,
						  const int blendTime)
{
	model_t		*mod_a = (model_t *)ghlInfo->animModel;

	int			index = G2_Find_Bone(mod_a, blist, boneName);
	if (index == -1)
	{
		index = G2_Add_Bone(mod_a, blist, boneName);
	}

	if (index != -1)
	{
		if (blist[index].flags & BONE_ANGLES_RAGDOLL)
		{
			return qtrue; // don't accept any calls on ragdoll bones
		}
	}

	if (index != -1)
	{
		return G2_Set_Bone_Anim_Index(blist,index,startFrame,endFrame,flags,animSpeed,currentTime,setFrame,blendTime,ghlInfo->aHeader->numFrames);
	}
	return qfalse;
}

qboolean G2_Get_Bone_Anim_Range(CGhoul2Info *ghlInfo, boneInfo_v &blist, const char *boneName, int *startFrame, int *endFrame)
{
	model_t		*mod_a = (model_t *)ghlInfo->animModel;
	int			index = G2_Find_Bone(mod_a, blist, boneName);

	// did we find it?
	if (index != -1)
	{
		// are we an animating bone?
		if (blist[index].flags & (BONE_ANIM_OVERRIDE_LOOP | BONE_ANIM_OVERRIDE))
		{
			*startFrame = blist[index].startFrame;
			*endFrame = blist[index].endFrame;
			return qtrue;
		}
	}
	return qfalse;
}

// given a model, bonelist and bonename, return the current frame, startframe and endframe of the current animation
// NOTE if we aren't running an animation, then qfalse is returned
void G2_TimingModel(boneInfo_t &bone,int currentTime,int numFramesInFile,int &currentFrame,int &newFrame,float &lerp);

qboolean G2_Get_Bone_Anim_Index( boneInfo_v &blist, const int index, const int currentTime,
						  float *currentFrame, int *startFrame, int *endFrame, int *flags, float *retAnimSpeed, qhandle_t *modelList, int numFrames)
{

	// did we find it?
	if ((index>=0) && !((index >= (int)blist.size()) || (blist[index].boneNumber == -1)))
	{

		// are we an animating bone?
		if (blist[index].flags & (BONE_ANIM_OVERRIDE_LOOP | BONE_ANIM_OVERRIDE))
		{
			int lcurrentFrame,newFrame;
			float lerp;
			G2_TimingModel(blist[index],currentTime,numFrames,lcurrentFrame,newFrame,lerp);

			*currentFrame =float(lcurrentFrame)+lerp;
			*startFrame = blist[index].startFrame;
			*endFrame = blist[index].endFrame;
			*flags = blist[index].flags;
			*retAnimSpeed = blist[index].animSpeed;
			return qtrue;
		}
	}
	*startFrame=0;
	*endFrame=1;
	*currentFrame=0.0f;
	*flags=0;
	*retAnimSpeed=0.0f;
	return qfalse;
}

// given a model, bonelist and bonename, return the current frame, startframe and endframe of the current animation
// NOTE if we aren't running an animation, then qfalse is returned
qboolean G2_Get_Bone_Anim(CGhoul2Info *ghlInfo, boneInfo_v &blist, const char *boneName, const int currentTime,
						  float *currentFrame, int *startFrame, int *endFrame, int *flags, float *retAnimSpeed, qhandle_t *modelList, int modelIndex)
{
	model_t		*mod_a = (model_t *)ghlInfo->animModel;

	int			index = G2_Find_Bone(mod_a, blist, boneName);

	if (index==-1)
	{
		index = G2_Add_Bone(mod_a, blist, boneName);

		if (index == -1)
		{
			return qfalse;
		}
	}

	assert(ghlInfo->aHeader);

	if (G2_Get_Bone_Anim_Index(blist, index, currentTime, currentFrame, startFrame, endFrame, flags, retAnimSpeed, modelList, ghlInfo->aHeader->numFrames))
	{
		assert(*startFrame>=0&&*startFrame<ghlInfo->aHeader->numFrames);
		assert(*endFrame>0&&*endFrame<=ghlInfo->aHeader->numFrames);
		assert(*currentFrame>=0.0f&&((int)(*currentFrame))<ghlInfo->aHeader->numFrames);
		return qtrue;
	}

	return qfalse;
}

// given a model, bonelist and bonename, lets pause an anim if it's playing.
qboolean G2_Pause_Bone_Anim(CGhoul2Info *ghlInfo, boneInfo_v &blist, const char *boneName, const int currentTime)
{
	model_t		*mod_a = (model_t *)ghlInfo->animModel;

	int			index = G2_Find_Bone(mod_a, blist, boneName);

	// did we find it?
	if (index != -1)
	{
		// are we pausing or un pausing?
		if (blist[index].pauseTime)
		{
			int		startFrame = 0, endFrame = 0, flags = 0;
			float	currentFrame = 0.0f, animSpeed = 1.0f;

			// figure out what frame we are on now
			G2_Get_Bone_Anim(ghlInfo, blist, boneName, blist[index].pauseTime, &currentFrame, &startFrame, &endFrame, &flags, &animSpeed, NULL, 0);
			// reset start time so we are actually on this frame right now
			G2_Set_Bone_Anim(ghlInfo, blist, boneName, startFrame, endFrame, flags, animSpeed, currentTime, currentFrame, 0);
			// no pausing anymore
			blist[index].pauseTime = 0;
		}
		// ahh, just pausing, the easy bit
		else
		{
			blist[index].pauseTime = currentTime;
		}

		return qtrue;
	}
	assert(0);

	return qfalse;
}

qboolean	G2_IsPaused(const char *fileName, boneInfo_v &blist, const char *boneName)
{
  	model_t		*mod_m = R_GetModelByHandle(RE_RegisterModel(fileName));
	model_t		*mod_a = R_GetModelByHandle(mod_m->mdxm->animIndex);
	int			index = G2_Find_Bone(mod_a, blist, boneName);

	// did we find it?
	if (index != -1)
	{
		// are we paused?
		if (blist[index].pauseTime)
		{
			// yup. paused.
			return qtrue;
		}
		return qfalse;
	}

	return qfalse;
}

// given a model, bonelist and bonename, lets stop an anim if it's playing.
qboolean G2_Stop_Bone_Anim_Index(boneInfo_v &blist, const int index)
{

	if ((index >= (int)blist.size()) || (blist[index].boneNumber == -1))
	{
		// we are attempting to set a bone override that doesn't exist
		assert(0);
		return qfalse;
	}

	blist[index].flags &= ~(BONE_ANIM_TOTAL);
	// try and remove this bone if we can
	return G2_Remove_Bone_Index(blist, index);
}

// given a model, bonelist and bonename, lets stop an anim if it's playing.
qboolean G2_Stop_Bone_Anim(const char *fileName, boneInfo_v &blist, const char *boneName)
{
  	model_t		*mod_m = R_GetModelByHandle(RE_RegisterModel(fileName));
	model_t		*mod_a = R_GetModelByHandle(mod_m->mdxm->animIndex);
	int			index = G2_Find_Bone(mod_a, blist, boneName);

	// did we find it?
	if (index != -1)
	{
		blist[index].flags &= ~(BONE_ANIM_TOTAL);
		// try and remove this bone if we can
		return G2_Remove_Bone_Index(blist, index);
	}
	assert(0);

	return qfalse;
}

// given a model, bonelist and bonename, lets stop an anim if it's playing.
qboolean G2_Stop_Bone_Angles_Index(boneInfo_v &blist, const int index)
{

	if ((index >= (int)blist.size()) || (blist[index].boneNumber == -1))
	{
		// we are attempting to set a bone override that doesn't exist
		assert(0);
		return qfalse;
	}

	blist[index].flags &= ~(BONE_ANGLES_TOTAL);
	// try and remove this bone if we can
	return G2_Remove_Bone_Index(blist, index);

}

// given a model, bonelist and bonename, lets stop an anim if it's playing.
qboolean G2_Stop_Bone_Angles(const char *fileName, boneInfo_v &blist, const char *boneName)
{
  	model_t		*mod_m = R_GetModelByHandle(RE_RegisterModel(fileName));
	model_t		*mod_a = R_GetModelByHandle(mod_m->mdxm->animIndex);
	int			index = G2_Find_Bone(mod_a, blist, boneName);

	// did we find it?
	if (index != -1)
	{
		blist[index].flags &= ~(BONE_ANGLES_TOTAL);
		// try and remove this bone if we can
		return G2_Remove_Bone_Index(blist, index);
	}
	assert(0);

	return qfalse;
}


// actually walk the bone list and update each and every bone if we have ended an animation for them.
void G2_Animate_Bone_List(CGhoul2Info_v &ghoul2, const int currentTime, const int index )
{
	boneInfo_v &blist = ghoul2[index].mBlist;

	// look through entire list
	for(size_t i=0; i<blist.size(); i++)
	{
		// we we a valid bone override?
		if (blist[i].boneNumber != -1)
		{
			// are we animating?
			if (blist[i].flags & (BONE_ANIM_OVERRIDE_LOOP | BONE_ANIM_OVERRIDE))
			{
				// yes - add in animation speed to current frame
				float	animSpeed = blist[i].animSpeed;
				float	endFrame = (float)blist[i].endFrame ;
				float	time = (currentTime - blist[i].startTime) / 50.0f;
				// are we a paused anim?
				if (blist[i].pauseTime)
				{
					time = (blist[i].pauseTime - blist[i].startTime) / 50.0f;
				}
				if (time<0.0f)
				{
					time=0.0f;
				}
				float	newFrame_g = blist[i].startFrame + (time * animSpeed);

				int		animSize = endFrame - blist[i].startFrame;
				// we are supposed to be animating right?
				if (animSize)
				{
					// did we run off the end?
					if (((animSpeed > 0.0f) && (newFrame_g > endFrame-1 )) ||
						((animSpeed < 0.0f) && (newFrame_g < endFrame+1 )))
					{
						// yep - decide what to do
						if (blist[i].flags & BONE_ANIM_OVERRIDE_LOOP)
						{
							// get our new animation frame back within the bounds of the animation set
							if (animSpeed < 0.0f)
							{
								if (newFrame_g <= endFrame+1)
								{
									newFrame_g=endFrame+fmod(newFrame_g-endFrame,animSize)-animSize;
								}
							}
							else
							{
								if (newFrame_g >= endFrame)
								{
									newFrame_g=endFrame+fmod(newFrame_g-endFrame,animSize)-animSize;
								}
							}
							// figure out new start time
							float frameTime =  newFrame_g - blist[i].startFrame ;
							blist[i].startTime = currentTime - (int)((frameTime / animSpeed) * 50.0f);
							if (blist[i].startTime>currentTime)
							{
								blist[i].startTime=currentTime;
							}
							assert(blist[i].startTime <= currentTime);
							blist[i].lastTime = blist[i].startTime;
						}
						else
						{
							if ((blist[i].flags & BONE_ANIM_OVERRIDE_FREEZE) != BONE_ANIM_OVERRIDE_FREEZE)
							{
								// nope, just stop it. And remove the bone if possible
								G2_Stop_Bone_Index(blist, i, (BONE_ANIM_TOTAL));
							}
						}
					}
				}
			}
		}
	}
}

//rww - RAGDOLL_BEGIN
/*


  rag stuff

*/
static void G2_RagDollSolve(CGhoul2Info_v &ghoul2V,int g2Index,float decay,int frameNum,const vec3_t currentOrg,bool LimitAngles,CRagDollUpdateParams *params = NULL);
static void G2_RagDollCurrentPosition(CGhoul2Info_v &ghoul2V,int g2Index,int frameNum,const vec3_t angles,const vec3_t position,const vec3_t scale);
static bool G2_RagDollSettlePositionNumeroTrois(CGhoul2Info_v &ghoul2V,const vec3_t currentOrg,CRagDollUpdateParams *params, int curTime);
static bool G2_RagDollSetup(CGhoul2Info &ghoul2,int frameNum,bool resetOrigin,const vec3_t origin,bool anyRendered);

void G2_GetBoneBasepose(CGhoul2Info &ghoul2,int boneNum,mdxaBone_t *&retBasepose,mdxaBone_t *&retBaseposeInv);
int G2_GetBoneDependents(CGhoul2Info &ghoul2,int boneNum,int *tempDependents,int maxDep);
void G2_GetBoneMatrixLow(CGhoul2Info &ghoul2,int boneNum,const vec3_t scale,mdxaBone_t &retMatrix,mdxaBone_t *&retBasepose,mdxaBone_t *&retBaseposeInv);
int G2_GetParentBoneMatrixLow(CGhoul2Info &ghoul2,int boneNum,const vec3_t scale,mdxaBone_t &retMatrix,mdxaBone_t *&retBasepose,mdxaBone_t *&retBaseposeInv);
bool G2_WasBoneRendered(CGhoul2Info &ghoul2,int boneNum);

#define MAX_BONES_RAG (256)

struct SRagEffector
{
	vec3_t		currentOrigin;
	vec3_t		desiredDirection;
	vec3_t		desiredOrigin;
	float		radius;
	float		weight;
};

#define RAG_MASK (CONTENTS_SOLID|CONTENTS_TERRAIN)//|CONTENTS_SHOTCLIP|CONTENTS_TERRAIN//(/*MASK_SOLID|*/CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_SHOTCLIP|CONTENTS_TERRAIN|CONTENTS_BODY)

extern cvar_t	*broadsword;
extern cvar_t	*broadsword_kickbones;
extern cvar_t	*broadsword_kickorigin;
extern cvar_t	*broadsword_dontstopanim;
extern cvar_t	*broadsword_waitforshot;
extern cvar_t	*broadsword_playflop;

extern cvar_t	*broadsword_effcorr;

extern cvar_t	*broadsword_ragtobase;

extern cvar_t	*broadsword_dircap;

extern cvar_t	*broadsword_extra1;
extern cvar_t	*broadsword_extra2;

#define RAG_PCJ						(0x00001)
#define RAG_PCJ_POST_MULT			(0x00002)	// has the pcj flag as well
#define RAG_PCJ_MODEL_ROOT			(0x00004)	// has the pcj flag as well
#define RAG_PCJ_PELVIS				(0x00008)	// has the pcj flag and POST_MULT as well
#define RAG_EFFECTOR				(0x00100)
#define RAG_WAS_NOT_RENDERED		(0x01000)		// not particularily reliable, more of a hint
#define RAG_WAS_EVER_RENDERED		(0x02000)		// not particularily reliable, more of a hint
#define RAG_BONE_LIGHTWEIGHT		(0x04000)		//used to indicate a bone's velocity treatment
#define RAG_PCJ_IK_CONTROLLED		(0x08000)		//controlled from IK move input
#define RAG_UNSNAPPABLE				(0x10000)		//cannot be broken out of constraints ever

// thiese flags are on the model and correspond to...
//#define		GHOUL2_RESERVED_FOR_RAGDOLL 0x0ff0  // these are not defined here for dependecies sake
#define		GHOUL2_RAG_STARTED						0x0010  // we are actually a ragdoll
#define		GHOUL2_RAG_PENDING						0x0100  // got start death anim but not end death anim
#define		GHOUL2_RAG_DONE							0x0200		// got end death anim
#define		GHOUL2_RAG_COLLISION_DURING_DEATH		0x0400		// ever have gotten a collision (da) event
#define		GHOUL2_RAG_COLLISION_SLIDE				0x0800		// ever have gotten a collision (slide) event
#define		GHOUL2_RAG_FORCESOLVE					0x1000		//api-override, determine if ragdoll should be forced to continue solving even if it thinks it is settled

//#define flrand	Q_flrand

static mdxaBone_t*		ragBasepose[MAX_BONES_RAG];
static mdxaBone_t*		ragBaseposeInv[MAX_BONES_RAG];
static mdxaBone_t		ragBones[MAX_BONES_RAG];
static SRagEffector		ragEffectors[MAX_BONES_RAG];
static boneInfo_t		*ragBoneData[MAX_BONES_RAG];
static int				tempDependents[MAX_BONES_RAG];
static int				ragBlistIndex[MAX_BONES_RAG];
static int				numRags;
static vec3_t			ragBoneMins;
static vec3_t			ragBoneMaxs;
static vec3_t			ragBoneCM;
static bool				haveDesiredPelvisOffset=false;
static vec3_t			desiredPelvisOffset; // this is for the root
static float			ragOriginChange=0.0f;
static vec3_t			ragOriginChangeDir;
//debug
#if 0
static vec3_t			handPos={0,0,0};
static vec3_t			handPos2={0,0,0};
#endif

enum ERagState
{
	ERS_DYNAMIC,
	ERS_SETTLING,
	ERS_SETTLED
};
static int				ragState;

static std::vector<boneInfo_t *>		rag;  // once we get the dependents precomputed this can be local


static void G2_Generate_MatrixRag(
			// caution this must not be called before the whole skeleton is "remembered"
	boneInfo_v			&blist,
	int					index)
{


	boneInfo_t &bone=blist[index];//.sent;

	memcpy(&bone.matrix,&bone.ragOverrideMatrix, sizeof(mdxaBone_t));
#ifdef _DEBUG
	int i,j;
	for (i = 0; i < 3; i++ )
	{
		for (j = 0; j < 4; j++ )
		{
			assert( !Q_isnan(bone.matrix.matrix[i][j]));
		}
	}
#endif// _DEBUG
	memcpy(&blist[index].newMatrix,&bone.matrix, sizeof(mdxaBone_t));
}

int G2_Find_Bone_Rag(CGhoul2Info *ghlInfo, boneInfo_v &blist, const char *boneName)
{
	mdxaSkel_t			*skel;
	mdxaSkelOffsets_t	*offsets;

   	offsets = (mdxaSkelOffsets_t *)((byte *)ghlInfo->aHeader + sizeof(mdxaHeader_t));
	skel = (mdxaSkel_t *)((byte *)ghlInfo->aHeader + sizeof(mdxaHeader_t) + offsets->offsets[0]);

	/*
	model_t			*currentModel;
	model_t			*animModel;
	mdxaHeader_t	*aHeader;

	currentModel = R_GetModelByHandle(RE_RegisterModel(ghlInfo->mFileName));
	assert(currentModel);
	animModel =  R_GetModelByHandle(currentModel->mdxm->animIndex);
	assert(animModel);
	aHeader = animModel->mdxa;
	assert(aHeader);

   	offsets = (mdxaSkelOffsets_t *)((byte *)aHeader + sizeof(mdxaHeader_t));
	skel = (mdxaSkel_t *)((byte *)aHeader + sizeof(mdxaHeader_t) + offsets->offsets[0]);
	*/

	// look through entire list
	for(size_t i=0; i<blist.size(); i++)
	{
		// if this bone entry has no info in it, bounce over it
		if (blist[i].boneNumber == -1)
		{
			continue;
		}

		// figure out what skeletal info structure this bone entry is looking at
		skel = (mdxaSkel_t *)((byte *)ghlInfo->aHeader + sizeof(mdxaHeader_t) + offsets->offsets[blist[i].boneNumber]);
		//skel = (mdxaSkel_t *)((byte *)aHeader + sizeof(mdxaHeader_t) + offsets->offsets[blist[i].boneNumber]);

		// if name is the same, we found it
		if (!Q_stricmp(skel->name, boneName))
		{
			return i;
		}
	}
#if _DEBUG
//	G2_Bone_Not_Found(boneName,ghlInfo->mFileName);
#endif
	// didn't find it
	return -1;
}

static int G2_Set_Bone_Rag(const mdxaHeader_t *mod_a,
	boneInfo_v &blist,
	const char *boneName,
	CGhoul2Info &ghoul2,
	const vec3_t scale,
	const vec3_t origin)
{
	// do not change the state of the skeleton here
	int	index = G2_Find_Bone_Rag(&ghoul2, blist, boneName);

	if (index == -1)
	{
		index = G2_Add_Bone(ghoul2.animModel, blist, boneName);
	}

	if (index != -1)
	{
		boneInfo_t &bone=blist[index];
		VectorCopy(origin,bone.extraVec1);

		G2_GetBoneMatrixLow(ghoul2,bone.boneNumber,scale,bone.originalTrueBoneMatrix,bone.basepose,bone.baseposeInv);
//		bone.parentRawBoneIndex=G2_GetParentBoneMatrixLow(ghoul2,bone.boneNumber,scale,bone.parentTrueBoneMatrix,bone.baseposeParent,bone.baseposeInvParent);
		assert( !Q_isnan(bone.originalTrueBoneMatrix.matrix[1][1]));
		assert( !Q_isnan(bone.originalTrueBoneMatrix.matrix[1][3]));
		bone.originalOrigin[0]=bone.originalTrueBoneMatrix.matrix[0][3];
		bone.originalOrigin[1]=bone.originalTrueBoneMatrix.matrix[1][3];
		bone.originalOrigin[2]=bone.originalTrueBoneMatrix.matrix[2][3];
	}
	return index;
}

static int G2_Set_Bone_Angles_Rag(
	CGhoul2Info &ghoul2,
	const mdxaHeader_t *mod_a,
	boneInfo_v &blist,
	const char *boneName,
	const int flags,
	const float radius,
	const vec3_t angleMin=0,
	const vec3_t angleMax=0,
	const int blendTime=500)
{
	int			index = G2_Find_Bone_Rag(&ghoul2, blist, boneName);

	if (index == -1)
	{
		index = G2_Add_Bone(ghoul2.animModel, blist, boneName);
	}
	if (index != -1)
	{
		boneInfo_t &bone=blist[index];
		bone.flags &= ~(BONE_ANGLES_TOTAL);
		bone.flags |= BONE_ANGLES_RAGDOLL;
		if (flags&RAG_PCJ)
		{
			if (flags&RAG_PCJ_POST_MULT)
			{
				bone.flags |= BONE_ANGLES_POSTMULT;
			}
			else if (flags&RAG_PCJ_MODEL_ROOT)
			{
				bone.flags |= BONE_ANGLES_PREMULT;
//				bone.flags |= BONE_ANGLES_POSTMULT;
			}
			else
			{
				assert(!"Invalid RAG PCJ\n");
			}
		}
		bone.ragStartTime=G2API_GetTime(0);
		bone.boneBlendStart = bone.ragStartTime;
		bone.boneBlendTime = blendTime;
		bone.radius=radius;
		bone.weight=1.0f;

		//init the others to valid values
		bone.epGravFactor = 0;
		VectorClear(bone.epVelocity);
		bone.solidCount = 0;
		bone.physicsSettled = false;
		bone.snapped = false;

		bone.parentBoneIndex = -1;

		bone.offsetRotation = 0.0f;

		bone.overGradSpeed = 0.0f;
		VectorClear(bone.overGoalSpot);
		bone.hasOverGoal = false;
		bone.hasAnimFrameMatrix = -1;

//		bone.weight=pow(radius,1.7f); //cubed was too harsh
//		bone.weight=radius*radius*radius;
		if (angleMin&&angleMax)
		{
			VectorCopy(angleMin,bone.minAngles);
			VectorCopy(angleMax,bone.maxAngles);
		}
		else
		{
			VectorCopy(bone.currentAngles,bone.minAngles); // I guess this isn't a rag pcj then
			VectorCopy(bone.currentAngles,bone.maxAngles);
		}
		if (!bone.lastTimeUpdated)
		{
			static mdxaBone_t		id =
			{
				{
					{ 1.0f, 0.0f, 0.0f, 0.0f },
					{ 0.0f, 1.0f, 0.0f, 0.0f },
					{ 0.0f, 0.0f, 1.0f, 0.0f }
				}
			};
			memcpy(&bone.ragOverrideMatrix,&id, sizeof(mdxaBone_t));
			VectorClear(bone.anglesOffset);
			VectorClear(bone.positionOffset);
			VectorClear(bone.velocityEffector);  // this is actually a velocity now
			VectorClear(bone.velocityRoot);  // this is actually a velocity now
			VectorClear(bone.lastPosition);
			VectorClear(bone.lastShotDir);
			bone.lastContents=0;
			// if this is non-zero, we are in a dynamic state
			bone.firstCollisionTime=bone.ragStartTime;
			// if this is non-zero, we are in a settling state
			bone.restTime=0;
			// if they are both zero, we are in a settled state

			bone.firstTime=0;

			bone.RagFlags=flags;
			bone.DependentRagIndexMask=0;

			G2_Generate_MatrixRag(blist,index); // set everything to th id

#if 0
			VectorClear(bone.currentAngles);
//			VectorAdd(bone.minAngles,bone.maxAngles,bone.currentAngles);
//			VectorScale(bone.currentAngles,0.5f,bone.currentAngles);
#else
			{
				if (
					(flags&RAG_PCJ_MODEL_ROOT) ||
					(flags&RAG_PCJ_PELVIS) ||
					!(flags&RAG_PCJ))
				{
					VectorClear(bone.currentAngles);
				}
				else
				{
					int k;
					for (k=0;k<3;k++)
					{
						float scalar=flrand(-1.0f,1.0f);
						scalar*=flrand(-1.0f,1.0f)*flrand(-1.0f,1.0f);
						// this is a heavily central distribution
						// center it on .5 (and make it small)
						scalar*=0.5f;
						scalar+=0.5f;

						bone.currentAngles[k]=(bone.minAngles[k]-bone.maxAngles[k])*scalar+bone.maxAngles[k];
					}
				}
			}
//			VectorClear(bone.currentAngles);
#endif
			VectorCopy(bone.currentAngles,bone.lastAngles);
		}
	}
	return index;
}

class CRagDollParams;
const mdxaHeader_t *G2_GetModA(CGhoul2Info &ghoul2);


static void G2_RagDollMatchPosition()
{
	haveDesiredPelvisOffset=false;
	int i;
	for (i=0;i<numRags;i++)
	{
		boneInfo_t &bone=*ragBoneData[i];
		SRagEffector &e=ragEffectors[i];

		vec3_t &desiredPos=e.desiredOrigin; // we will save this

		if (0&&(bone.RagFlags & RAG_PCJ_PELVIS))
		{
			// just move to quake origin
			VectorCopy(bone.originalOrigin,desiredPos);
			VectorSubtract(desiredPos,e.currentOrigin,desiredPelvisOffset); // last arg is dest
			haveDesiredPelvisOffset=true;
			VectorCopy(e.currentOrigin,bone.lastPosition); // last arg is dest
			continue;
		}

		if (!(bone.RagFlags & RAG_EFFECTOR))
		{
			continue;
		}
		VectorCopy(bone.originalOrigin,desiredPos);
		VectorSubtract(desiredPos,e.currentOrigin,e.desiredDirection);
		VectorCopy(e.currentOrigin,bone.lastPosition); // last arg is dest
	}
}

qboolean G2_Set_Bone_Anim_No_BS(CGhoul2Info &ghoul2, const mdxaHeader_t *mod,boneInfo_v &blist, const char *boneName, const int argStartFrame,
						  const int argEndFrame, const int flags, const float animSpeed, const int currentTime, const float setFrame,
						  const int blendTime, const int creationID, bool resetBonemap=true)
{
	int			index = G2_Find_Bone_Rag(&ghoul2, blist, boneName);
	int			modFlags = flags;

	int startFrame=argStartFrame;
	int endFrame=argEndFrame;

	if (index != -1)
	{
		blist[index].blendFrame = blist[index].blendLerpFrame = 0;
		blist[index].blendTime = blist[index].blendStart = 0;
		modFlags &= ~(BONE_ANIM_BLEND);

		blist[index].endFrame = endFrame;
		blist[index].startFrame = startFrame;
		blist[index].animSpeed = animSpeed;
		blist[index].pauseTime = 0;
//		blist[index].boneMap = NULL;
//		blist[index].lastTime = blist[index].startTime = (currentTime - (((setFrame - (float)startFrame) * 50.0)/ animSpeed));
		blist[index].flags &= ~(BONE_ANIM_TOTAL);
		blist[index].flags |= modFlags;

		return qtrue;
	}

	index = G2_Add_Bone(ghoul2.animModel, blist, boneName);

	if (index != -1)
	{
		blist[index].blendFrame = blist[index].blendLerpFrame = 0;
		blist[index].blendTime = 0;
		modFlags &= ~(BONE_ANIM_BLEND);
		blist[index].endFrame = endFrame;
		blist[index].startFrame = startFrame;
		blist[index].animSpeed = animSpeed;
		blist[index].pauseTime = 0;
//		blist[index].boneMap = NULL;
//		blist[index].lastTime = blist[index].startTime = (currentTime - (((setFrame - (float)startFrame) * 50.0f)/ animSpeed));
		blist[index].flags &= ~(BONE_ANIM_TOTAL);
		blist[index].flags |= modFlags;

		return qtrue;
	}

	assert(0);
	return qfalse;
}

void G2_ResetRagDoll(CGhoul2Info_v &ghoul2V)
{
	int model;

	for (model = 0; model < ghoul2V.size(); model++)
	{
		if (ghoul2V[model].mModelindex != -1)
		{
			break;
		}
	}

	if (model == ghoul2V.size())
	{
		return;
	}

	CGhoul2Info &ghoul2 = ghoul2V[model];

	if (!(ghoul2.mFlags & GHOUL2_RAG_STARTED))
	{ //no use in doing anything if we aren't ragging
		return;
	}

	boneInfo_v &blist = ghoul2.mBlist;
#if 1
	//Eh, screw it. Ragdoll does a lot of terrible things to the bones that probably aren't directly reversible, so just reset it all.
	G2_Init_Bone_List(blist, ghoul2.aHeader->numBones);
#else //The anims on every bone are messed up too, as are the angles. There's not really any way to get back to a normal state, so just clear the list
	  //and let them re-set their anims/angles gameside.
	int i = 0;
	while (i < blist.size())
	{
		boneInfo_t &bone = blist[i];
		if (bone.boneNumber != -1 && (bone.flags & BONE_ANGLES_RAGDOLL))
		{
			bone.flags &= ~BONE_ANGLES_RAGDOLL;
			bone.flags &= ~BONE_ANGLES_IK;
			bone.RagFlags = 0;
			bone.lastTimeUpdated = 0;
			VectorClear(bone.currentAngles);
			bone.ragStartTime = 0;
		}
		i++;
	}
#endif
	ghoul2.mFlags &= ~(GHOUL2_RAG_PENDING|GHOUL2_RAG_DONE|GHOUL2_RAG_STARTED);
}

void G2_SetRagDoll(CGhoul2Info_v &ghoul2V,CRagDollParams *parms)
{
	if (parms)
	{
		parms->CallRagDollBegin=qfalse;
	}
	if (!broadsword||!broadsword->integer||!parms)
	{
		return;
	}
	int model;
	for (model = 0; model < ghoul2V.size(); model++)
	{
		if (ghoul2V[model].mModelindex != -1)
		{
			break;
		}
	}
	if (model==ghoul2V.size())
	{
		return;
	}
	CGhoul2Info &ghoul2=ghoul2V[model];
	const mdxaHeader_t *mod_a=G2_GetModA(ghoul2);
	if (!mod_a)
	{
		return;
	}
	int curTime=G2API_GetTime(0);
	boneInfo_v &blist = ghoul2.mBlist;
	int	index = G2_Find_Bone_Rag(&ghoul2, blist, "model_root");
	switch (parms->RagPhase)
	{
	case CRagDollParams::RP_START_DEATH_ANIM:
		ghoul2.mFlags|=GHOUL2_RAG_PENDING;
		return;  /// not doing anything with this yet
		break;
	case CRagDollParams::RP_END_DEATH_ANIM:
		ghoul2.mFlags|=GHOUL2_RAG_PENDING|GHOUL2_RAG_DONE;
		if (broadsword_waitforshot &&
			broadsword_waitforshot->integer)
		{
			if (broadsword_waitforshot->integer==2)
			{
				if (!(ghoul2.mFlags&(GHOUL2_RAG_COLLISION_DURING_DEATH|GHOUL2_RAG_COLLISION_SLIDE)))
				{
					//nothing was encountered, lets just wait for the first shot
					return; // we ain't starting yet
				}
			}
			else
			{
				return; // we ain't starting yet
			}
		}
		break;
	case CRagDollParams::RP_DEATH_COLLISION:
		if (parms->collisionType)
		{
			ghoul2.mFlags|=GHOUL2_RAG_COLLISION_SLIDE;
		}
		else
		{
			ghoul2.mFlags|=GHOUL2_RAG_COLLISION_DURING_DEATH;
		}
		if (broadsword_dontstopanim && broadsword_waitforshot &&
			(broadsword_dontstopanim->integer || broadsword_waitforshot->integer)
			)
		{
			if (!(ghoul2.mFlags&GHOUL2_RAG_DONE))
			{
				return; // we ain't starting yet
			}
		}
		break;
	case CRagDollParams::RP_CORPSE_SHOT:
		if (broadsword_kickorigin &&
			broadsword_kickorigin->integer)
		{
			if (index>=0&&index<(int)blist.size())
			{
				boneInfo_t &bone=blist[index];
				if (bone.boneNumber>=0)
				{
					if (bone.flags & BONE_ANGLES_RAGDOLL)
					{
						//rww - Would need ent pointer here. But.. since this is SW, we aren't even having corpse shooting anyway I'd imagine.
						/*
						float magicFactor14=8.0f; //64.0f; // kick strength

						if (parms->fShotStrength)
						{ //if there is a shot strength, use it instead
							magicFactor14 = parms->fShotStrength;
						}

						parms->me->s.pos.trType = TR_GRAVITY;
						parms->me->s.pos.trDelta[0] += bone.lastShotDir[0]*magicFactor14;
						parms->me->s.pos.trDelta[1] += bone.lastShotDir[1]*magicFactor14;
						//parms->me->s.pos.trDelta[2] = fabsf(bone.lastShotDir[2])*magicFactor14;
						//rww - The vertical portion of this doesn't seem to work very well
						//I am just leaving it whatever it is for now, because my velocity scaling
						//only works on x and y and the gravity stuff for NPCs is a bit unpleasent
						//trying to change/work with
						assert( !Q_isnan(bone.lastShotDir[1]));
						*/
					}
				}
			}
		}
		break;
	case CRagDollParams::RP_GET_PELVIS_OFFSET:
		if (parms->RagPhase==CRagDollParams::RP_GET_PELVIS_OFFSET)
		{
			VectorClear(parms->pelvisAnglesOffset);
			VectorClear(parms->pelvisPositionOffset);
		}
		// intentional lack of a break
	case CRagDollParams::RP_SET_PELVIS_OFFSET:
		if (index>=0&&index<(int)blist.size())
		{
			boneInfo_t &bone=blist[index];
			if (bone.boneNumber>=0)
			{
				if (bone.flags & BONE_ANGLES_RAGDOLL)
				{
					if (parms->RagPhase==CRagDollParams::RP_GET_PELVIS_OFFSET)
					{
						VectorCopy(bone.anglesOffset,parms->pelvisAnglesOffset);
						VectorCopy(bone.positionOffset,parms->pelvisPositionOffset);
					}
					else
					{
						VectorCopy(parms->pelvisAnglesOffset,bone.anglesOffset);
						VectorCopy(parms->pelvisPositionOffset,bone.positionOffset);
					}
				}
			}
		}
		return;
		break;
	case CRagDollParams::RP_DISABLE_EFFECTORS:
		// not doing anything with this yet
		return;
		break;
	default:
		assert(0);
		return;
		break;
	}
	if (ghoul2.mFlags&GHOUL2_RAG_STARTED)
	{
		// only going to begin ragdoll once, everything else depends on what happens to the origin
		return;
	}
#if 0
if (index>=0)
{
	Com_OPrintf("death %d %d\n",blist[index].startFrame,blist[index].endFrame);
}
#endif

	ghoul2.mFlags|=GHOUL2_RAG_PENDING|GHOUL2_RAG_DONE|GHOUL2_RAG_STARTED;  // well anyway we are going live
	parms->CallRagDollBegin=qtrue;

	G2_GenerateWorldMatrix(parms->angles, parms->position);
	G2_ConstructGhoulSkeleton(ghoul2V, curTime, false, parms->scale);

	G2_Set_Bone_Rag(mod_a,blist,"model_root",ghoul2,parms->scale,parms->position);
	G2_Set_Bone_Rag(mod_a,blist,"pelvis",ghoul2,parms->scale,parms->position);

	G2_Set_Bone_Rag(mod_a,blist,"lower_lumbar",ghoul2,parms->scale,parms->position);
	G2_Set_Bone_Rag(mod_a,blist,"upper_lumbar",ghoul2,parms->scale,parms->position);
	G2_Set_Bone_Rag(mod_a,blist,"thoracic",ghoul2,parms->scale,parms->position);
	G2_Set_Bone_Rag(mod_a,blist,"cranium",ghoul2,parms->scale,parms->position);
	G2_Set_Bone_Rag(mod_a,blist,"rhumerus",ghoul2,parms->scale,parms->position);
	G2_Set_Bone_Rag(mod_a,blist,"lhumerus",ghoul2,parms->scale,parms->position);
	G2_Set_Bone_Rag(mod_a,blist,"rradius",ghoul2,parms->scale,parms->position);
	G2_Set_Bone_Rag(mod_a,blist,"lradius",ghoul2,parms->scale,parms->position);
	G2_Set_Bone_Rag(mod_a,blist,"rfemurYZ",ghoul2,parms->scale,parms->position);
	G2_Set_Bone_Rag(mod_a,blist,"lfemurYZ",ghoul2,parms->scale,parms->position);
	G2_Set_Bone_Rag(mod_a,blist,"rtibia",ghoul2,parms->scale,parms->position);
	G2_Set_Bone_Rag(mod_a,blist,"ltibia",ghoul2,parms->scale,parms->position);
	G2_Set_Bone_Rag(mod_a,blist,"rhand",ghoul2,parms->scale,parms->position);
	G2_Set_Bone_Rag(mod_a,blist,"lhand",ghoul2,parms->scale,parms->position);
	//G2_Set_Bone_Rag(mod_a,blist,"rtarsal",ghoul2,parms->scale,parms->position);
	//G2_Set_Bone_Rag(mod_a,blist,"ltarsal",ghoul2,parms->scale,parms->position);
	G2_Set_Bone_Rag(mod_a,blist,"rtalus",ghoul2,parms->scale,parms->position);
	G2_Set_Bone_Rag(mod_a,blist,"ltalus",ghoul2,parms->scale,parms->position);
	G2_Set_Bone_Rag(mod_a,blist,"rradiusX",ghoul2,parms->scale,parms->position);
	G2_Set_Bone_Rag(mod_a,blist,"lradiusX",ghoul2,parms->scale,parms->position);
	G2_Set_Bone_Rag(mod_a,blist,"rfemurX",ghoul2,parms->scale,parms->position);
	G2_Set_Bone_Rag(mod_a,blist,"lfemurX",ghoul2,parms->scale,parms->position);
	G2_Set_Bone_Rag(mod_a,blist,"ceyebrow",ghoul2,parms->scale,parms->position);

	//int startFrame = 3665, endFrame = 3665+1;
	int startFrame = parms->startFrame, endFrame = parms->endFrame;

	G2_Set_Bone_Anim_No_BS(ghoul2, mod_a,blist,"upper_lumbar",startFrame,endFrame-1,
		BONE_ANIM_OVERRIDE_FREEZE|BONE_ANIM_BLEND,
		1.0f,curTime,float(startFrame),150,0,true);
	G2_Set_Bone_Anim_No_BS(ghoul2, mod_a,blist,"lower_lumbar",startFrame,endFrame-1,
		BONE_ANIM_OVERRIDE_FREEZE|BONE_ANIM_BLEND,
		1.0f,curTime,float(startFrame),150,0,true);
	G2_Set_Bone_Anim_No_BS(ghoul2, mod_a,blist,"Motion",startFrame,endFrame-1,
		BONE_ANIM_OVERRIDE_FREEZE|BONE_ANIM_BLEND,
		1.0f,curTime,float(startFrame),150,0,true);
//	G2_Set_Bone_Anim_No_BS(ghoul2, mod_a,blist,"model_root",startFrame,endFrame-1,
//		BONE_ANIM_OVERRIDE_FREEZE|BONE_ANIM_BLEND,
//		1.0f,curTime,float(startFrame),150,0,true);
	G2_Set_Bone_Anim_No_BS(ghoul2, mod_a,blist,"lfemurYZ",startFrame,endFrame-1,
		BONE_ANIM_OVERRIDE_FREEZE|BONE_ANIM_BLEND,
		1.0f,curTime,float(startFrame),150,0,true);
	G2_Set_Bone_Anim_No_BS(ghoul2, mod_a,blist,"rfemurYZ",startFrame,endFrame-1,
		BONE_ANIM_OVERRIDE_FREEZE|BONE_ANIM_BLEND,
		1.0f,curTime,float(startFrame),150,0,true);

	G2_Set_Bone_Anim_No_BS(ghoul2, mod_a,blist,"rhumerus",startFrame,endFrame-1,
		BONE_ANIM_OVERRIDE_FREEZE|BONE_ANIM_BLEND,
		1.0f,curTime,float(startFrame),150,0,true);
	G2_Set_Bone_Anim_No_BS(ghoul2, mod_a,blist,"lhumerus",startFrame,endFrame-1,
		BONE_ANIM_OVERRIDE_FREEZE|BONE_ANIM_BLEND,
		1.0f,curTime,float(startFrame),150,0,true);

//    should already be set					G2_GenerateWorldMatrix(parms->angles, parms->position);
	G2_ConstructGhoulSkeleton(ghoul2V, curTime, false, parms->scale);

	static const float fRadScale = 0.3f;//0.5f;

	vec3_t pcjMin,pcjMax;
	VectorSet(pcjMin,-90.0f,-45.0f,-45.0f);
	VectorSet(pcjMax,90.0f,45.0f,45.0f);
	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"model_root",RAG_PCJ_MODEL_ROOT|RAG_PCJ|RAG_UNSNAPPABLE,10.0f*fRadScale,pcjMin,pcjMax,100);
	VectorSet(pcjMin,-45.0f,-45.0f,-45.0f);
	VectorSet(pcjMax,45.0f,45.0f,45.0f);
	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"pelvis",RAG_PCJ_PELVIS|RAG_PCJ|RAG_PCJ_POST_MULT|RAG_UNSNAPPABLE,10.0f*fRadScale,pcjMin,pcjMax,100);

#if 1
	// new base anim, unconscious flop
	int pcjflags=RAG_PCJ|RAG_PCJ_POST_MULT;//|RAG_EFFECTOR;

	VectorSet(pcjMin,-15.0f,-15.0f,-15.0f);
	VectorSet(pcjMax,15.0f,15.0f,15.0f);
	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"lower_lumbar",pcjflags|RAG_UNSNAPPABLE,10.0f*fRadScale,pcjMin,pcjMax,500);
	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"upper_lumbar",pcjflags|RAG_UNSNAPPABLE,10.0f*fRadScale,pcjMin,pcjMax,500);
	VectorSet(pcjMin,-25.0f,-25.0f,-25.0f);
	VectorSet(pcjMax,25.0f,25.0f,25.0f);
	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"thoracic",pcjflags|RAG_EFFECTOR|RAG_UNSNAPPABLE,12.0f*fRadScale,pcjMin,pcjMax,500);

	VectorSet(pcjMin,-10.0f,-10.0f,-90.0f);
	VectorSet(pcjMax,10.0f,10.0f,90.0f);
	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"cranium",pcjflags|RAG_BONE_LIGHTWEIGHT|RAG_UNSNAPPABLE,6.0f*fRadScale,pcjMin,pcjMax,500);

	static const float sFactLeg = 1.0f;
	static const float sFactArm = 1.0f;
	static const float sRadArm = 1.0f;
	static const float sRadLeg = 1.0f;

	VectorSet(pcjMin,-100.0f,-40.0f,-15.0f);
	VectorSet(pcjMax,-15.0f,80.0f,15.0f);
	VectorScale(pcjMin, sFactArm, pcjMin);
	VectorScale(pcjMax, sFactArm, pcjMax);
	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"rhumerus",pcjflags|RAG_BONE_LIGHTWEIGHT|RAG_UNSNAPPABLE,(4.0f*sRadArm)*fRadScale,pcjMin,pcjMax,500);
	VectorSet(pcjMin,-50.0f,-80.0f,-15.0f);
	VectorSet(pcjMax,15.0f,40.0f,15.0f);
	VectorScale(pcjMin, sFactArm, pcjMin);
	VectorScale(pcjMax, sFactArm, pcjMax);
	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"lhumerus",pcjflags|RAG_BONE_LIGHTWEIGHT|RAG_UNSNAPPABLE,(4.0f*sRadArm)*fRadScale,pcjMin,pcjMax,500);

	VectorSet(pcjMin,-25.0f,-20.0f,-20.0f);
	VectorSet(pcjMax,90.0f,20.0f,-20.0f);
	VectorScale(pcjMin, sFactArm, pcjMin);
	VectorScale(pcjMax, sFactArm, pcjMax);
	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"rradius",pcjflags|RAG_BONE_LIGHTWEIGHT,(3.0f*sRadArm)*fRadScale,pcjMin,pcjMax,500);
	VectorSet(pcjMin,-90.0f,-20.0f,-20.0f);
	VectorSet(pcjMax,30.0f,20.0f,-20.0f);
	VectorScale(pcjMin, sFactArm, pcjMin);
	VectorScale(pcjMax, sFactArm, pcjMax);
	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"lradius",pcjflags|RAG_BONE_LIGHTWEIGHT,(3.0f*sRadArm)*fRadScale,pcjMin,pcjMax,500);


	VectorSet(pcjMin,-80.0f,-50.0f,-20.0f);
	VectorSet(pcjMax,30.0f,5.0f,20.0f);
	VectorScale(pcjMin, sFactLeg, pcjMin);
	VectorScale(pcjMax, sFactLeg, pcjMax);
	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"rfemurYZ",pcjflags|RAG_BONE_LIGHTWEIGHT,(6.0f*sRadLeg)*fRadScale,pcjMin,pcjMax,500);
	VectorSet(pcjMin,-60.0f,-5.0f,-20.0f);
	VectorSet(pcjMax,50.0f,50.0f,20.0f);
	VectorScale(pcjMin, sFactLeg, pcjMin);
	VectorScale(pcjMax, sFactLeg, pcjMax);
	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"lfemurYZ",pcjflags|RAG_BONE_LIGHTWEIGHT,(6.0f*sRadLeg)*fRadScale,pcjMin,pcjMax,500);

	VectorSet(pcjMin,-20.0f,-15.0f,-15.0f);
	VectorSet(pcjMax,100.0f,15.0f,15.0f);
	VectorScale(pcjMin, sFactLeg, pcjMin);
	VectorScale(pcjMax, sFactLeg, pcjMax);
	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"rtibia",pcjflags|RAG_EFFECTOR|RAG_BONE_LIGHTWEIGHT,(4.0f*sRadLeg)*fRadScale,pcjMin,pcjMax,500);
	VectorSet(pcjMin,20.0f,-15.0f,-15.0f);
	VectorSet(pcjMax,100.0f,15.0f,15.0f);
	VectorScale(pcjMin, sFactLeg, pcjMin);
	VectorScale(pcjMax, sFactLeg, pcjMax);
	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"ltibia",pcjflags|RAG_EFFECTOR|RAG_BONE_LIGHTWEIGHT,(4.0f*sRadLeg)*fRadScale,pcjMin,pcjMax,500);
#else
	// old base anim
	int pcjflags=RAG_PCJ|RAG_PCJ_POST_MULT|RAG_EFFECTOR;

	VectorSet(pcjMin,-15.0f,-15.0f,-15.0f);
	VectorSet(pcjMax,45.0f,15.0f,15.0f);
	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"lower_lumbar",pcjflags,10.0f,pcjMin,pcjMax,500);
	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"upper_lumbar",pcjflags,10.0f,pcjMin,pcjMax,500);
	VectorSet(pcjMin,-45.0f,-45.0f,-45.0f);
	VectorSet(pcjMax,45.0f,45.0f,45.0f);
	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"thoracic",pcjflags,10.0f,pcjMin,pcjMax,500);

	VectorSet(pcjMin,-10.0f,-10.0f,-90.0f);
	VectorSet(pcjMax,10.0f,10.0f,90.0f);
	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"cranium",pcjflags|RAG_BONE_LIGHTWEIGHT,6.0f,pcjMin,pcjMax,500);

	//VectorSet(pcjMin,-45.0f,-90.0f,-100.0f);
	VectorSet(pcjMin,-180.0f,-180.0f,-100.0f);
	//VectorSet(pcjMax,60.0f,60.0f,45.0f);
	VectorSet(pcjMax,180.0f,180.0f,45.0f);
	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"rhumerus",pcjflags|RAG_BONE_LIGHTWEIGHT,4.0f,pcjMin,pcjMax,500);
	//VectorSet(pcjMin,-45.0f,-60.0f,-45.0f);
	VectorSet(pcjMin,-180.0f,-180.0f,-100.0f);
	//VectorSet(pcjMax,60.0f,90.0f,100.0f);
	VectorSet(pcjMax,180.0f,180.0f,100.0f);
	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"lhumerus",pcjflags|RAG_BONE_LIGHTWEIGHT,4.0f,pcjMin,pcjMax,500);

	//-120/120
	VectorSet(pcjMin,-120.0f,-20.0f,-20.0f);
	VectorSet(pcjMax,50.0f,20.0f,-20.0f);
	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"rradius",pcjflags|RAG_BONE_LIGHTWEIGHT,3.0f,pcjMin,pcjMax,500);
	VectorSet(pcjMin,-120.0f,-20.0f,-20.0f);
	VectorSet(pcjMax,5.0f,20.0f,-20.0f);
	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"lradius",pcjflags|RAG_BONE_LIGHTWEIGHT,3.0f,pcjMin,pcjMax,500);

	VectorSet(pcjMin,-90.0f,-50.0f,-20.0f);
	VectorSet(pcjMax,50.0f,20.0f,20.0f);
	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"rfemurYZ",pcjflags|RAG_BONE_LIGHTWEIGHT,6.0f,pcjMin,pcjMax,500);
	VectorSet(pcjMin,-90.0f,-20.0f,-20.0f);
	VectorSet(pcjMax,50.0f,50.0f,20.0f);
	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"lfemurYZ",pcjflags|RAG_BONE_LIGHTWEIGHT,6.0f,pcjMin,pcjMax,500);

	//120
	VectorSet(pcjMin,-20.0f,-15.0f,-15.0f);
	VectorSet(pcjMax,120.0f,15.0f,15.0f);
	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"rtibia",pcjflags|RAG_EFFECTOR|RAG_BONE_LIGHTWEIGHT,4.0f,pcjMin,pcjMax,500);
	VectorSet(pcjMin,20.0f,-15.0f,-15.0f);
	VectorSet(pcjMax,120.0f,15.0f,15.0f);
	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"ltibia",pcjflags|RAG_EFFECTOR|RAG_BONE_LIGHTWEIGHT,4.0f,pcjMin,pcjMax,500);
#endif


	float sRadEArm = 1.2f;
	float sRadELeg = 1.2f;

//	int rhand=
	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"rhand",RAG_EFFECTOR|RAG_BONE_LIGHTWEIGHT,(6.0f*sRadEArm)*fRadScale);
	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"lhand",RAG_EFFECTOR|RAG_BONE_LIGHTWEIGHT,(6.0f*sRadEArm)*fRadScale);
	//G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"rtarsal",RAG_EFFECTOR|RAG_BONE_LIGHTWEIGHT,(4.0f*sRadELeg)*fRadScale);
	//G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"ltarsal",RAG_EFFECTOR|RAG_BONE_LIGHTWEIGHT,(4.0f*sRadELeg)*fRadScale);
//	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"rtibia",RAG_EFFECTOR|RAG_BONE_LIGHTWEIGHT,(4.0f*sRadELeg)*fRadScale);
//	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"ltibia",RAG_EFFECTOR|RAG_BONE_LIGHTWEIGHT,(4.0f*sRadELeg)*fRadScale);
	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"rtalus",RAG_EFFECTOR|RAG_BONE_LIGHTWEIGHT,(4.0f*sRadELeg)*fRadScale);
	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"ltalus",RAG_EFFECTOR|RAG_BONE_LIGHTWEIGHT,(4.0f*sRadELeg)*fRadScale);
	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"rradiusX",RAG_EFFECTOR|RAG_BONE_LIGHTWEIGHT,(6.0f*sRadEArm)*fRadScale);
	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"lradiusX",RAG_EFFECTOR|RAG_BONE_LIGHTWEIGHT,(6.0f*sRadEArm)*fRadScale);
	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"rfemurX",RAG_EFFECTOR|RAG_BONE_LIGHTWEIGHT,(10.0f*sRadELeg)*fRadScale);
	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"lfemurX",RAG_EFFECTOR|RAG_BONE_LIGHTWEIGHT,(10.0f*sRadELeg)*fRadScale);
	//G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"ceyebrow",RAG_EFFECTOR|RAG_BONE_LIGHTWEIGHT,10.0f*fRadScale);
	G2_Set_Bone_Angles_Rag(ghoul2, mod_a,blist,"ceyebrow",RAG_EFFECTOR|RAG_BONE_LIGHTWEIGHT,5.0f);
//match the currrent animation
	if (!G2_RagDollSetup(ghoul2,curTime,true,parms->position,false))
	{
		assert(!"failed to add any rag bones");
		return;
	}
	G2_RagDollCurrentPosition(ghoul2V,model,curTime,parms->angles,parms->position,parms->scale);
#if 0
	if (rhand>0)
	{
		boneInfo_t &bone=blist[rhand];
		SRagEffector &e=ragEffectors[bone.ragIndex];
		VectorCopy(bone.originalOrigin,handPos);
		VectorCopy(e.currentOrigin,handPos2);
	}
#endif
	int k;

	CRagDollUpdateParams fparms;
	VectorCopy(parms->position, fparms.position);
	VectorCopy(parms->angles, fparms.angles);
	VectorCopy(parms->scale, fparms.scale);
	VectorClear(fparms.velocity);
	fparms.me = parms->me;
	fparms.settleFrame = parms->endFrame;

	//Guess I don't need to do this, do I?
	G2_ConstructGhoulSkeleton(ghoul2V, curTime, false, parms->scale);

	vec3_t dPos;
	VectorCopy(parms->position, dPos);
#ifdef _OLD_STYLE_SETTLE
	dPos[2] -= 6;
#endif

	for (k=0;k</*10*/20;k++)
	{
		G2_RagDollSettlePositionNumeroTrois(ghoul2V,dPos,&fparms,curTime);

		G2_RagDollCurrentPosition(ghoul2V,model,curTime,parms->angles,dPos,parms->scale);
		G2_RagDollMatchPosition();
		G2_RagDollSolve(ghoul2V,model,1.0f*(1.0f-k/40.0f),curTime,dPos,false);
	}
}

void G2_SetRagDollBullet(CGhoul2Info &ghoul2,const vec3_t rayStart,const vec3_t hit)
{
	if (!broadsword||!broadsword->integer)
	{
		return;
	}
	vec3_t shotDir;
	VectorSubtract(hit,rayStart,shotDir);
	float len=VectorLength(shotDir);
	if (len<1.0f)
	{
		return;
	}
	float lenr=1.0f/len;
	shotDir[0]*=lenr;
	shotDir[1]*=lenr;
	shotDir[2]*=lenr;

	bool firstOne=false;
	if (broadsword_kickbones&&broadsword_kickbones->integer)
	{
		int		magicFactor13=150.0f; // squared radius multiplier for shot effects
		boneInfo_v &blist = ghoul2.mBlist;
		for(int i=(int)(blist.size()-1);i>=0;i--)
		{
			boneInfo_t &bone=blist[i];
			if ((bone.flags & BONE_ANGLES_TOTAL))
			{
				if (bone.flags & BONE_ANGLES_RAGDOLL)
				{
					if (!firstOne)
					{
						firstOne=true;
#if 0
						int curTime=G2API_GetTime(0);
						const mdxaHeader_t *mod_a=G2_GetModA(ghoul2);
						int startFrame = 0, endFrame = 0;
#if 1
						TheGhoul2Wraith()->GetAnimFrames(ghoul2.mID, "unconsciousdeadflop01", startFrame, endFrame);
						if (startFrame == -1 && endFrame == -1)
						{ //A bad thing happened! Just use the hardcoded numbers even though they could be wrong.
							startFrame = 3573;
							endFrame = 3583;
							assert(0);
						}
						G2_Set_Bone_Anim_No_BS(mod_a,blist,"upper_lumbar",startFrame,endFrame-1,
							BONE_ANIM_OVERRIDE_FREEZE|BONE_ANIM_BLEND,
							1.0f,curTime,float(startFrame),75,0,true);
						G2_Set_Bone_Anim_No_BS(mod_a,blist,"lfemurYZ",startFrame,endFrame-1,
							BONE_ANIM_OVERRIDE_FREEZE|BONE_ANIM_BLEND,
							1.0f,curTime,float(startFrame),75,0,true);
						G2_Set_Bone_Anim_No_BS(mod_a,blist,"rfemurYZ",startFrame,endFrame-1,
							BONE_ANIM_OVERRIDE_FREEZE|BONE_ANIM_BLEND,
							1.0f,curTime,float(startFrame),75,0,true);
#else
						TheGhoul2Wraith()->GetAnimFrames(ghoul2.mID, "backdeadflop01", startFrame, endFrame);
						if (startFrame == -1 && endFrame == -1)
						{ //A bad thing happened! Just use the hardcoded numbers even though they could be wrong.
							startFrame = 3581;
							endFrame = 3592;
							assert(0);
						}
						G2_Set_Bone_Anim_No_BS(mod_a,blist,"upper_lumbar",endFrame,startFrame+1,
							BONE_ANIM_OVERRIDE_FREEZE,
							-1.0f,curTime,float(endFrame-1),50,0,true);
						G2_Set_Bone_Anim_No_BS(mod_a,blist,"lfemurYZ",endFrame,startFrame+1,
							BONE_ANIM_OVERRIDE_FREEZE,
							-1.0f,curTime,float(endFrame-1),50,0,true);
						G2_Set_Bone_Anim_No_BS(mod_a,blist,"rfemurYZ",endFrame,startFrame+1,
							BONE_ANIM_OVERRIDE_FREEZE,
							-1.0f,curTime,float(endFrame-1),50,0,true);
#endif
#endif
					}

					VectorCopy(shotDir,bone.lastShotDir);
					vec3_t dir;
					VectorSubtract(bone.lastPosition,hit,dir);
					len=VectorLength(dir);
					if (len<1.0f)
					{
						len=1.0f;
					}
					lenr=1.0f/len;
					float effect=lenr;
					effect*=magicFactor13*effect; // this is cubed, one of them is absorbed by the next calc
					bone.velocityEffector[0]=shotDir[0]*(effect+flrand(0.0f,0.05f));
					bone.velocityEffector[1]=shotDir[1]*(effect+flrand(0.0f,0.05f));
					bone.velocityEffector[2]=fabs(shotDir[2])*(effect+flrand(0.0f,0.05f));
//					bone.velocityEffector[0]=shotDir[0]*(effect+flrand(0.0f,0.05f))*flrand(-0.1f,3.0f);
//					bone.velocityEffector[1]=shotDir[1]*(effect+flrand(0.0f,0.05f))*flrand(-0.1f,3.0f);
//					bone.velocityEffector[2]=fabs(shotDir[2])*(effect+flrand(0.0f,0.05f))*flrand(-0.1f,3.0f);
					assert( !Q_isnan(shotDir[2]));
	//				bone.currentAngles[0]+=flrand(-10.0f*lenr,10.0f*lenr);
	//				bone.currentAngles[1]+=flrand(-10.0f*lenr,10.0f*lenr);
	//				bone.currentAngles[2]+=flrand(-10.0f*lenr,10.0f*lenr);
	//				bone.lastAngles[0]+=flrand(-10.0f*lenr,10.0f*lenr);
	//				bone.lastAngles[1]+=flrand(-10.0f*lenr,10.0f*lenr);
	//				bone.lastAngles[2]+=flrand(-10.0f*lenr,10.0f*lenr);

					// go dynamic
					bone.firstCollisionTime=G2API_GetTime(0);
//					bone.firstCollisionTime=0;
					bone.restTime=0;
				}
			}
		}
	}
}


static float G2_RagSetState(CGhoul2Info &ghoul2, boneInfo_t &bone,int frameNum,const vec3_t origin,bool &resetOrigin)
{
	ragOriginChange=DistanceSquared(origin,bone.extraVec1);
	VectorSubtract(origin,bone.extraVec1,ragOriginChangeDir);

	float decay=1.0f;

	int dynamicTime=1000;
	int settleTime=1000;

	if (ghoul2.mFlags & GHOUL2_RAG_FORCESOLVE)
	{
		ragState=ERS_DYNAMIC;
		if (frameNum>bone.firstCollisionTime+dynamicTime)
		{
			VectorCopy(origin,bone.extraVec1);
			if (ragOriginChange>15.0f)
			{ //if we moved, or if this bone is still in solid
				bone.firstCollisionTime=frameNum;
			}
			else
			{
				// settle out
				bone.firstCollisionTime=0;
				bone.restTime=frameNum;
				ragState=ERS_SETTLING;
			}
		}
	}
	else if (bone.firstCollisionTime>0)
	{
		ragState=ERS_DYNAMIC;
		if (frameNum>bone.firstCollisionTime+dynamicTime)
		{
			VectorCopy(origin,bone.extraVec1);
			if (ragOriginChange>15.0f)
			{ //if we moved, or if this bone is still in solid
				bone.firstCollisionTime=frameNum;
			}
			else
			{
				// settle out
				bone.firstCollisionTime=0;
				bone.restTime=frameNum;
				ragState=ERS_SETTLING;
			}
		}
//decay=0.0f;
	}
	else if (bone.restTime>0)
	{
		decay=1.0f-(frameNum-bone.restTime)/float(dynamicTime);
		if (decay<0.0f)
		{
			decay=0.0f;
		}
		if (decay>1.0f)
		{
			decay=1.0f;
		}
		float	magicFactor8=1.0f; // Power for decay
		decay=pow(decay,magicFactor8);
		ragState=ERS_SETTLING;
		if (frameNum>bone.restTime+settleTime)
		{
			VectorCopy(origin,bone.extraVec1);
			if (ragOriginChange>15.0f)
			{
				bone.restTime=frameNum;
			}
			else
			{
				// stop
				bone.restTime=0;
				ragState=ERS_SETTLED;
			}
		}
//decay=0.0f;
	}
	else
	{
		if (bone.RagFlags & RAG_PCJ_IK_CONTROLLED)
		{
			bone.firstCollisionTime=frameNum;
			ragState=ERS_DYNAMIC;
		}
		else if (ragOriginChange>15.0f)
		{
			bone.firstCollisionTime=frameNum;
			ragState=ERS_DYNAMIC;
		}
		else
		{
			ragState=ERS_SETTLED;
		}
		decay=0.0f;
	}
//			ragState=ERS_SETTLED;
//			decay=0.0f;
	return decay;
}

static bool G2_RagDollSetup(CGhoul2Info &ghoul2,int frameNum,bool resetOrigin,const vec3_t origin,bool anyRendered)
{
	int minSurvivingBone=10000;
	//int minSurvivingBoneAt=-1;
	int minSurvivingBoneAlt=10000;
	//int minSurvivingBoneAtAlt=-1;

	assert(ghoul2.mFileName[0]);
	boneInfo_v &blist = ghoul2.mBlist;
	rag.clear();
	int numRendered=0;
	int numNotRendered=0;
	//int pelvisAt=-1;
	for(size_t i=0; i<blist.size(); i++)
	{
		boneInfo_t &bone=blist[i];
		if (bone.boneNumber>=0)
		{
			assert(bone.boneNumber<MAX_BONES_RAG);
			if ((bone.flags & BONE_ANGLES_RAGDOLL) || (bone.flags & BONE_ANGLES_IK))
			{
				//rww - this was (!anyRendered) before. Isn't that wrong? (if anyRendered, then wasRendered should be true)
				bool wasRendered=
					(!anyRendered) ||   // offscreeen or something
					G2_WasBoneRendered(ghoul2,bone.boneNumber);
				if (!wasRendered)
				{
					bone.RagFlags|=RAG_WAS_NOT_RENDERED;
					numNotRendered++;
				}
				else
				{
					bone.RagFlags&=~RAG_WAS_NOT_RENDERED;
					bone.RagFlags|=RAG_WAS_EVER_RENDERED;
					numRendered++;
				}
				if	(bone.RagFlags&RAG_PCJ_PELVIS)
				{
					//pelvisAt=i;
				}
				else if	(bone.RagFlags&RAG_PCJ_MODEL_ROOT)
				{
				}
				else if	(wasRendered && (bone.RagFlags&RAG_PCJ))
				{
					if (minSurvivingBone>bone.boneNumber)
					{
						minSurvivingBone=bone.boneNumber;
						//minSurvivingBoneAt=i;
					}
				}
				else if	(wasRendered)
				{
					if (minSurvivingBoneAlt>bone.boneNumber)
					{
						minSurvivingBoneAlt=bone.boneNumber;
						//minSurvivingBoneAtAlt=i;
					}
				}
				if (
					anyRendered &&
					(bone.RagFlags&RAG_WAS_EVER_RENDERED) &&
					!(bone.RagFlags&RAG_PCJ_MODEL_ROOT) &&
					!(bone.RagFlags&RAG_PCJ_PELVIS) &&
					!wasRendered &&
					(bone.RagFlags&RAG_EFFECTOR)
					)
				{
					// this thing was rendered in the past, but wasn't now, although other bones were, lets get rid of it
//					bone.flags &= ~BONE_ANGLES_RAGDOLL;
//					bone.RagFlags = 0;
//Com_OPrintf("Deleted Effector %d\n",i);
//					continue;
				}
				if ((int)rag.size()<bone.boneNumber+1)
				{
					rag.resize(bone.boneNumber+1,0);
				}
				rag[bone.boneNumber]=&bone;
				ragBlistIndex[bone.boneNumber]=i;

				bone.lastTimeUpdated=frameNum;
				if (resetOrigin)
				{
					VectorCopy(origin,bone.extraVec1); // this is only done incase a limb is removed
				}
			}
		}
	}
#if 0
	if (numRendered<5)  // I think this is a limb
	{
//Com_OPrintf("limb %3d/%3d  (r,N).\n",numRendered,numNotRendered);
		if (minSurvivingBoneAt<0)
		{
			// pelvis is gone, but we have no remaining pcj's
			// just find any remain rag effector
			minSurvivingBoneAt=minSurvivingBoneAtAlt;
		}
		if (
			minSurvivingBoneAt>=0 &&
			pelvisAt>=0)
		{
			{
				 // remove the pelvis as a rag
				boneInfo_t &bone=blist[minSurvivingBoneAt];
				bone.flags&=~BONE_ANGLES_RAGDOLL;
				bone.RagFlags=0;
			}
			{
				// the root-est bone is now our "pelvis
				boneInfo_t &bone=blist[minSurvivingBoneAt];
				VectorSet(bone.minAngles,-14500.0f,-14500.0f,-14500.0f);
				VectorSet(bone.maxAngles,14500.0f,14500.0f,14500.0f);
				bone.RagFlags|=RAG_PCJ_PELVIS|RAG_PCJ; // this guy is our new "pelvis"
				bone.flags |= BONE_ANGLES_POSTMULT;
				bone.ragStartTime=G2API_GetTime(0);
			}
		}
	}
#endif
	numRags=0;
	//int ragStartTime=0;
	for(size_t i=0; i<rag.size(); i++)
	{
		if (rag[i])
		{
			boneInfo_t &bone=*rag[i];
			assert(bone.boneNumber>=0);
			assert(numRags<MAX_BONES_RAG);

			//ragStartTime=bone.ragStartTime;

			bone.ragIndex=numRags;
			ragBoneData[numRags]=&bone;
			ragEffectors[numRags].radius=bone.radius;
			ragEffectors[numRags].weight=bone.weight;
			G2_GetBoneBasepose(ghoul2,bone.boneNumber,bone.basepose,bone.baseposeInv);
			numRags++;
		}
	}
	if (!numRags)
	{
		return false;
	}
	return true;
}

static void G2_RagDoll(CGhoul2Info_v &ghoul2V,int g2Index,CRagDollUpdateParams *params, int curTime)
{
	if (!broadsword||!broadsword->integer)
	{
		return;
	}

	if (!params)
	{
		assert(0);
		return;
	}

	vec3_t dPos;
	VectorCopy(params->position, dPos);
#ifdef _OLD_STYLE_SETTLE
	dPos[2] -= 6;
#endif

//	params->DebugLine(handPos,handPos2,false);
	int frameNum=G2API_GetTime(0);
	CGhoul2Info &ghoul2=ghoul2V[g2Index];
	assert(ghoul2.mFileName[0]);
	boneInfo_v &blist = ghoul2.mBlist;

	// hack for freezing ragdoll (no idea if it works)
#if 0
	if (0)
	{
		// we gotta hack this to basically freeze the timers
		for(i=0; i<blist.size(); i++)
		{
			boneInfo_t &bone=blist[i];
			if (bone.boneNumber>=0)
			{
				assert(bone.boneNumber<MAX_BONES_RAG);
				if (bone.flags & BONE_ANGLES_RAGDOLL)
				{
					bone.ragStartTime+=50;
					if (bone.firstTime)
					{
						bone.firstTime+=50;
					}
					if (bone.firstCollisionTime)
					{
						bone.firstCollisionTime+=50;
					}
					if (bone.restTime)
					{
						bone.restTime+=50;
					}
				}
			}
		}
		return;
	}
#endif

	float decay=1.0f;
	bool resetOrigin=false;
	bool anyRendered=false;

	// this loop checks for settled
	for(size_t i=0; i<blist.size(); i++)
	{
		boneInfo_t &bone=blist[i];
		if (bone.boneNumber>=0)
		{
			assert(bone.boneNumber<MAX_BONES_RAG);
			if (bone.flags & BONE_ANGLES_RAGDOLL)
			{
				// check for state transitions
				decay=G2_RagSetState(ghoul2,bone,frameNum,dPos,resetOrigin); // set the current state

				if (ragState==ERS_SETTLED)
				{
#if 0
					bool noneInSolid = true;

					//first iterate through and make sure no bones are still in solid a lot
					for(int j = 0; j < blist.size(); j++)
					{
						boneInfo_t &bone2 = blist[j];

						if (bone2.boneNumber >= 0 && bone2.solidCount > 8)
						{
							noneInSolid = false;
							break;
						}
					}

					if (noneInSolid)
					{ //we're settled then
						params->RagDollSettled();
						return;
					}
					else
					{
						continue;
					}
#else
					params->RagDollSettled();
					return;
#endif
				}
				if (G2_WasBoneRendered(ghoul2,bone.boneNumber))
				{
					anyRendered=true;
					break;
				}
			}
		}
	}
	//int iters=(ragState==ERS_DYNAMIC)?2:1;
	int iters=(ragState==ERS_DYNAMIC)?4:2;
	//bool kicked=false;
	if (ragOriginChangeDir[2]<-100.0f)
	{
		//kicked=true;
		//iters*=8;
		iters*=2; //rww - changed to this.. it was getting up to around 600 traces at times before (which is insane)
	}
	if (iters)
	{
		if (!G2_RagDollSetup(ghoul2,frameNum,resetOrigin,dPos,anyRendered))
		{
			return;
		}
		// ok, now our data structures are compact and set up in topological order

		for (int i=0;i<iters;i++)
		{
			G2_RagDollCurrentPosition(ghoul2V,g2Index,frameNum,params->angles,dPos,params->scale);

			if (G2_RagDollSettlePositionNumeroTrois(ghoul2V,dPos,params,curTime))
			{
#if 0
				//effectors are start solid alot, so this was pretty extreme
				if (!kicked&&iters<4)
				{
					kicked=true;
					//iters*=4;
					iters*=2;
				}
#endif
			}
			//params->position[2] += 16;
			G2_RagDollSolve(ghoul2V,g2Index,decay*2.0f,frameNum,dPos,true,params);
		}
	}

	if (params->me != ENTITYNUM_NONE)
	{
#if 0
		vec3_t worldMins,worldMaxs;
		worldMins[0]=params->position[0]-17;
		worldMins[1]=params->position[1]-17;
		worldMins[2]=params->position[2];
		worldMaxs[0]=params->position[0]+17;
		worldMaxs[1]=params->position[1]+17;
		worldMaxs[2]=params->position[2];
//Com_OPrintf(va("%f \n",worldMins[2]);
//		params->DebugLine(worldMins,worldMaxs,true);
#endif
		G2_RagDollCurrentPosition(ghoul2V,g2Index,frameNum,params->angles,params->position,params->scale);
//		SV_UnlinkEntity(params->me);
//		params->me->SetMins(BB_SHOOTING_SIZE,ragBoneMins);
//		params->me->SetMaxs(BB_SHOOTING_SIZE,ragBoneMaxs);
//		SV_LinkEntity(params->me);
	}
}

#ifdef _DEBUG
#define _DEBUG_BONE_NAMES
#endif

static inline char *G2_Get_Bone_Name(CGhoul2Info *ghlInfo, boneInfo_v &blist, int boneNum)
{
	mdxaSkel_t			*skel;
	mdxaSkelOffsets_t	*offsets;
   	offsets = (mdxaSkelOffsets_t *)((byte *)ghlInfo->aHeader + sizeof(mdxaHeader_t));
	skel = (mdxaSkel_t *)((byte *)ghlInfo->aHeader + sizeof(mdxaHeader_t) + offsets->offsets[0]);

	// look through entire list
	for(size_t i=0; i<blist.size(); i++)
	{
		// if this bone entry has no info in it, bounce over it
		if (blist[i].boneNumber != boneNum)
		{
			continue;
		}

		// figure out what skeletal info structure this bone entry is looking at
		skel = (mdxaSkel_t *)((byte *)ghlInfo->aHeader + sizeof(mdxaHeader_t) + offsets->offsets[blist[i].boneNumber]);

		return skel->name;
	}

	// didn't find it
	return "BONE_NOT_FOUND";
}

char *G2_GetBoneNameFromSkel(CGhoul2Info &ghoul2, int boneNum);

static void G2_RagDollCurrentPosition(CGhoul2Info_v &ghoul2V,int g2Index,int frameNum,const vec3_t angles,const vec3_t position,const vec3_t scale)
{
	CGhoul2Info &ghoul2=ghoul2V[g2Index];
	assert(ghoul2.mFileName[0]);
//Com_OPrintf("angles %f %f %f\n",angles[0],angles[1],angles[2]);
	G2_GenerateWorldMatrix(angles,position);
	G2_ConstructGhoulSkeleton(ghoul2V, frameNum, false, scale);

	float totalWt=0.0f;
	int i;
	for (i=0;i<numRags;i++)
	{
		boneInfo_t &bone=*ragBoneData[i];
		G2_GetBoneMatrixLow(ghoul2,bone.boneNumber,scale,ragBones[i],ragBasepose[i],ragBaseposeInv[i]);

#ifdef _DEBUG_BONE_NAMES
		char *debugBoneName = G2_Get_Bone_Name(&ghoul2, ghoul2.mBlist, bone.boneNumber);
		assert(debugBoneName);
#endif

		int k;
		//float cmweight=ragEffectors[numRags].weight;
		float cmweight=1.0f;
		for (k=0;k<3;k++)
		{
			ragEffectors[i].currentOrigin[k]=ragBones[i].matrix[k][3];
			assert( !Q_isnan(ragEffectors[i].currentOrigin[k]));
			if (!i)
			{
				// set mins, maxs and cm
				ragBoneCM[k]=ragEffectors[i].currentOrigin[k]*cmweight;
				ragBoneMaxs[k]=ragEffectors[i].currentOrigin[k];
				ragBoneMins[k]=ragEffectors[i].currentOrigin[k];
			}
			else
			{
				ragBoneCM[k]+=ragEffectors[i].currentOrigin[k]*ragEffectors[i].weight;
				if (ragEffectors[i].currentOrigin[k]>ragBoneMaxs[k])
				{
					ragBoneMaxs[k]=ragEffectors[i].currentOrigin[k];
				}
				if (ragEffectors[i].currentOrigin[k]<ragBoneMins[k])
				{
					ragBoneMins[k]=ragEffectors[i].currentOrigin[k];
				}
			}
		}

		totalWt+=cmweight;
	}

	assert(totalWt>0.0f);
	int k;
	{
		float wtInv=1.0f/totalWt;
		for (k=0;k<3;k++)
		{
			ragBoneMaxs[k]-=position[k];
			ragBoneMins[k]-=position[k];
			ragBoneMaxs[k]+=10.0f;
			ragBoneMins[k]-=10.0f;
			ragBoneCM[k]*=wtInv;

			ragBoneCM[k]=ragEffectors[0].currentOrigin[k]; // use the pelvis
		}
	}
}

#ifdef _DEBUG
int ragTraceTime = 0;
int ragSSCount = 0;
int ragTraceCount = 0;
#endif

void Rag_Trace( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, const int passEntityNum, const int contentmask, const EG2_Collision eG2TraceType, const int useLod )
{
#ifdef _DEBUG
	int ragPreTrace = ri.Milliseconds();
#endif
	if ( ri.CGVMLoaded() )
	{
		ragCallbackTraceLine_t *callData = (ragCallbackTraceLine_t *)ri.GetSharedMemory();

		VectorCopy(start, callData->start);
		VectorCopy(end, callData->end);
		VectorCopy(mins, callData->mins);
		VectorCopy(maxs, callData->maxs);
		callData->ignore = passEntityNum;
		callData->mask = contentmask;

		ri.CGVM_RagCallback( RAG_CALLBACK_TRACELINE );

		*results = callData->tr;
	}
	else
	{
		results->entityNum = ENTITYNUM_NONE;
		//SV_Trace(results, start, mins, maxs, end, passEntityNum, contentmask, eG2TraceType, useLod);
		ri.CM_BoxTrace(results, start, end, mins, maxs, 0, contentmask, 0);
		results->entityNum = results->fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
	}

#ifdef _DEBUG
	int ragPostTrace = ri.Milliseconds();

	ragTraceTime += (ragPostTrace - ragPreTrace);
	if (results->startsolid)
	{
		ragSSCount++;
	}
	ragTraceCount++;
#endif
}

//run advanced physics on each bone indivudually
//an adaption of my "exphys" custom game physics model
#define MAX_GRAVITY_PULL 256//512

static inline bool G2_BoneOnGround(const vec3_t org, const vec3_t mins, const vec3_t maxs, const int ignoreNum)
{
	trace_t tr;
	vec3_t gSpot;

	VectorCopy(org, gSpot);
	gSpot[2] -= 1.0f; //seems reasonable to me

	Rag_Trace(&tr, org, mins, maxs, gSpot, ignoreNum, RAG_MASK, G2_NOCOLLIDE, 0);

	if (tr.fraction != 1.0f && !tr.startsolid && !tr.allsolid)
	{ //not in solid, and hit something. Guess it's ground.
		return true;
	}

	return false;
}

static inline bool G2_ApplyRealBonePhysics(boneInfo_t &bone, SRagEffector &e, CRagDollUpdateParams *params, vec3_t goalSpot, const vec3_t testMins, const vec3_t testMaxs,
										   const float gravity, const float mass, const float bounce)
{
	trace_t tr;
	vec3_t projectedOrigin;
	vec3_t vNorm;
	vec3_t ground;
	float velScaling = 0.1f;
	float vTotal = 0.0f;
	bool boneOnGround = false;

	assert(mass <= 1.0f && mass >= 0.01f);

	if (bone.physicsSettled)
	{ //then we have no need to continue
		return true;
	}

	if (gravity)
	{ //factor it in before we do anything.
		VectorCopy(e.currentOrigin, ground);
		ground[2] -= 1.0f;

		Rag_Trace(&tr, e.currentOrigin, testMins, testMaxs, ground, params->me, RAG_MASK, G2_NOCOLLIDE, 0);

		if (tr.entityNum == ENTITYNUM_NONE)
		{
			boneOnGround = false;
		}
		else
		{
			boneOnGround = true;
		}

		if (!boneOnGround)
		{
			if (!params->velocity[2])
			{ //only increase gravitational pull once the actual entity is still
				bone.epGravFactor += gravity;
			}

			if (bone.epGravFactor > MAX_GRAVITY_PULL)
			{ //cap it off if needed
				bone.epGravFactor = MAX_GRAVITY_PULL;
			}

			bone.epVelocity[2] -= bone.epGravFactor;
		}
		else
		{ //if we're sitting on something then reset the gravity factor.
			bone.epGravFactor = 0;
		}
	}
	else
	{
		boneOnGround = G2_BoneOnGround(e.currentOrigin, testMins, testMaxs, params->me);
	}

	if (!bone.epVelocity[0] && !bone.epVelocity[1] && !bone.epVelocity[2])
	{ //nothing to do if we have no velocity even after gravity.
		VectorCopy(e.currentOrigin, goalSpot);
		return true;
	}

	//get the projected origin based on velocity.
	VectorMA(e.currentOrigin, velScaling, bone.epVelocity, projectedOrigin);

	//scale it down based on mass
	VectorScale(bone.epVelocity, 1.0f-mass, bone.epVelocity);

	VectorCopy(bone.epVelocity, vNorm);
	vTotal = VectorNormalize(vNorm);

	if (vTotal < 1 && boneOnGround)
	{ //we've pretty much stopped moving anyway, just clear it out then.
		VectorClear(bone.epVelocity);
		bone.epGravFactor = 0;
		VectorCopy(e.currentOrigin, goalSpot);
		return true;
	}

	Rag_Trace(&tr, e.currentOrigin, testMins, testMaxs, projectedOrigin, params->me, RAG_MASK, G2_NOCOLLIDE, 0);

	if (tr.startsolid || tr.allsolid)
	{ //can't go anywhere from here
		return false;
	}

	//Go ahead and set it to the trace endpoint regardless of what it hit
	VectorCopy(tr.endpos, goalSpot);

	if (tr.fraction == 1.0f)
	{ //Nothing was in the way.
		return true;
	}

	if (bounce)
	{
		vTotal *= bounce; //scale it by bounce

		VectorScale(tr.plane.normal, vTotal, vNorm); //scale the trace plane normal by the bounce factor

		if (vNorm[2] > 0)
		{
			bone.epGravFactor -= vNorm[2]*(1.0f-mass); //The lighter it is the more gravity will be reduced by bouncing vertically.
			if (bone.epGravFactor < 0)
			{
				bone.epGravFactor = 0;
			}
		}

		VectorAdd(bone.epVelocity, vNorm, bone.epVelocity); //add it into the existing velocity.

		//I suppose it could be sort of neat to make a game callback here to actual do stuff
		//when bones slam into things. But it could be slow too.
		/*
		if (tr.entityNum != ENTITYNUM_NONE && ent->touch)
		{ //then call the touch function
			ent->touch(ent, &g_entities[tr.entityNum], &tr);
		}
		*/
	}
	else
	{ //if no bounce, kill when it hits something.
		bone.epVelocity[0] = 0;
		bone.epVelocity[1] = 0;

		if (!gravity)
		{
			bone.epVelocity[2] = 0;
		}
	}
	return true;
}

#ifdef _DEBUG_BONE_NAMES
static inline void G2_RagDebugBox(vec3_t mins, vec3_t maxs, int duration)
{
	if ( !ri.CGVMLoaded() )
		return;

	ragCallbackDebugBox_t *callData = (ragCallbackDebugBox_t *)ri.GetSharedMemory();

	callData->duration = duration;
	VectorCopy(mins, callData->mins);
	VectorCopy(maxs, callData->maxs);

	ri.CGVM_RagCallback( RAG_CALLBACK_DEBUGBOX );
}

static inline void G2_RagDebugLine(vec3_t start, vec3_t end, int time, int color, int radius)
{
	if ( !ri.CGVMLoaded() )
		return;

	ragCallbackDebugLine_t *callData = (ragCallbackDebugLine_t *)ri.GetSharedMemory();

	VectorCopy(start, callData->start);
	VectorCopy(end, callData->end);
	callData->time = time;
	callData->color = color;
	callData->radius = radius;

	ri.CGVM_RagCallback( RAG_CALLBACK_DEBUGLINE );
}
#endif

#ifdef _OLD_STYLE_SETTLE
static bool G2_RagDollSettlePositionNumeroTrois(CGhoul2Info_v &ghoul2V, const vec3_t currentOrg, CRagDollUpdateParams *params, int curTime)
{
	haveDesiredPelvisOffset=false;
	vec3_t desiredPos;
	int i;

	assert(params);
	//assert(params->me); //no longer valid, because me is an index!
	int ignoreNum=params->me;

	bool anyStartSolid=false;

	vec3_t groundSpot={0,0,0};
	// lets find the floor at our quake origin
	{
		vec3_t testStart;
		VectorCopy(currentOrg,testStart); //last arg is dest
		vec3_t testEnd;
		VectorCopy(testStart,testEnd); //last arg is dest
		testEnd[2]-=200.0f;

		vec3_t testMins;
		vec3_t testMaxs;
		VectorSet(testMins,-10,-10,-10);
		VectorSet(testMaxs,10,10,10);

		{
			trace_t		tr;
			assert( !Q_isnan(testStart[1]));
			assert( !Q_isnan(testEnd[1]));
			assert( !Q_isnan(testMins[1]));
			assert( !Q_isnan(testMaxs[1]));
			Rag_Trace(&tr,testStart,testMins,testMaxs,testEnd,ignoreNum,RAG_MASK,G2_NOCOLLIDE,0/*SV_TRACE_NO_PLAYER*/);
			if (tr.entityNum==0)
			{
				VectorAdvance(testStart,.5f,testEnd,tr.endpos);
			}
			if (tr.startsolid)
			{
				//hmmm, punt
				VectorCopy(currentOrg,groundSpot); //last arg is dest
				groundSpot[2]-=30.0f;
			}
			else
			{
				VectorCopy(tr.endpos,groundSpot); //last arg is dest
			}
		}
	}

	for (i=0;i<numRags;i++)
	{
		boneInfo_t &bone=*ragBoneData[i];
		SRagEffector &e=ragEffectors[i];
		if (bone.RagFlags & RAG_PCJ_PELVIS)
		{
			// just move to quake origin
			VectorCopy(currentOrg,desiredPos);
			//desiredPos[2]-=35.0f;
			desiredPos[2]-=20.0f;
//old deathflop			desiredPos[2]-=40.0f;
			VectorSubtract(desiredPos,e.currentOrigin,desiredPelvisOffset); // last arg is dest
			haveDesiredPelvisOffset=true;
			VectorCopy(e.currentOrigin,bone.lastPosition); // last arg is dest
			continue;
		}

		if (!(bone.RagFlags & RAG_EFFECTOR))
		{
			continue;
		}
		vec3_t testMins;
		vec3_t testMaxs;
		VectorSet(testMins,-e.radius,-e.radius,-e.radius);
		VectorSet(testMaxs,e.radius,e.radius,e.radius);

		// first we will see if we are start solid
		// if so, we are gonna run some bonus iterations
		bool iAmStartSolid=false;
		vec3_t testStart;
		VectorCopy(e.currentOrigin,testStart); //last arg is dest
		testStart[2]+=12.0f; // we are no so concerned with minor penetration

		vec3_t testEnd;
		VectorCopy(testStart,testEnd); //last arg is dest
		testEnd[2]-=8.0f;
		assert( !Q_isnan(testStart[1]));
		assert( !Q_isnan(testEnd[1]));
		assert( !Q_isnan(testMins[1]));
		assert( !Q_isnan(testMaxs[1]));
		float vertEffectorTraceFraction=0.0f;
		{
			trace_t		tr;
			Rag_Trace(&tr,testStart,testMins,testMaxs,testEnd,ignoreNum,RAG_MASK,G2_NOCOLLIDE,0);
			if (tr.entityNum==0)
			{
				VectorAdvance(testStart,.5f,testEnd,tr.endpos);
			}
			if (tr.startsolid)
			{
				// above the origin, so lets try lower
				if (e.currentOrigin[2] > groundSpot[2])
				{
					testStart[2]=groundSpot[2]+(e.radius-10.0f);
				}
				else
				{
					// lets try higher
					testStart[2]=groundSpot[2]+8.0f;
					Rag_Trace(&tr,testStart,testMins,testMaxs,testEnd,ignoreNum,RAG_MASK,G2_NOCOLLIDE,0);
					if (tr.entityNum==0)
					{
						VectorAdvance(testStart,.5f,testEnd,tr.endpos);
					}
				}

			}
			if (tr.startsolid)
			{
				iAmStartSolid=true;
				anyStartSolid=true;
				// above the origin, so lets slide away
				if (e.currentOrigin[2] > groundSpot[2])
				{
					if (params)
					{
						//SRagDollEffectorCollision args(e.currentOrigin,tr);
						//params->EffectorCollision(args);
						if ( ri.CGVMLoaded() )
						{ //make a callback and see if the cgame wants to help us out
							ragCallbackBoneInSolid_t *callData = (ragCallbackBoneInSolid_t *)ri.GetSharedMemory();

							VectorCopy(e.currentOrigin, callData->bonePos);
							callData->entNum = params->me;
							callData->solidCount = bone.solidCount;

							ri.CGVM_RagCallback( RAG_CALLBACK_BONEINSOLID );
						}
					}
				}
				else
				{
					//harumph, we are really screwed
				}
			}
			else
			{
				vertEffectorTraceFraction=tr.fraction;
				if (params &&
					vertEffectorTraceFraction < .95f &&
					fabsf(tr.plane.normal[2]) < .707f)
				{
					//SRagDollEffectorCollision args(e.currentOrigin,tr);
					//args.useTracePlane=true;
					//params->EffectorCollision(args);
					if ( ri.CGVMLoaded() )
					{ //make a callback and see if the cgame wants to help us out
						ragCallbackBoneInSolid_t *callData = (ragCallbackBoneInSolid_t *)ri.GetSharedMemory();

						VectorCopy(e.currentOrigin, callData->bonePos);
						callData->entNum = params->me;
						callData->solidCount = bone.solidCount;

						ri.CGVM_RagCallback( RAG_CALLBACK_BONEINSOLID );
					}
				}
			}
		}
		vec3_t effectorGroundSpot;
		VectorAdvance(testStart,vertEffectorTraceFraction,testEnd,effectorGroundSpot);//  VA(a,t,b,c)-> c := (1-t)a+tb
		// trace from the quake origin horzontally to the effector
		// gonna choose the maximum of the ground spot or the effector location
		// and clamp it to be roughly in the bbox
		VectorCopy(groundSpot,testStart); //last arg is dest
		if (iAmStartSolid)
		{
			// we don't have a meaningful ground spot
			VectorCopy(e.currentOrigin,testEnd); //last arg is dest
			bone.solidCount++;
		}
		else
		{
			VectorCopy(effectorGroundSpot,testEnd); //last arg is dest
			bone.solidCount = 0;
		}
		assert( !Q_isnan(testStart[1]));
		assert( !Q_isnan(testEnd[1]));
		assert( !Q_isnan(testMins[1]));
		assert( !Q_isnan(testMaxs[1]));

		float ztest;

		if (testEnd[2]>testStart[2])
		{
			ztest=testEnd[2];
		}
		else
		{
			ztest=testStart[2];
		}
		if (ztest<currentOrg[2]-30.0f)
		{
			ztest=currentOrg[2]-30.0f;
		}
		if (ztest<currentOrg[2]+10.0f)
		{
			ztest=currentOrg[2]+10.0f;
		}
		testStart[2]=ztest;
		testEnd[2]=ztest;

		float		magicFactor44=1.0f; // going to trace a bit longer, this also serves as an expansion parameter
		VectorAdvance(testStart,magicFactor44,testEnd,testEnd);//  VA(a,t,b,c)-> c := (1-t)a+tb

		float horzontalTraceFraction=0.0f;
		vec3_t HorizontalHitSpot={0,0,0};
		{
			trace_t		tr;
			Rag_Trace(&tr,testStart,testMins,testMaxs,testEnd,ignoreNum,RAG_MASK,G2_NOCOLLIDE,0);
			if (tr.entityNum==0)
			{
				VectorAdvance(testStart,.5f,testEnd,tr.endpos);
			}
			horzontalTraceFraction=tr.fraction;
			if (tr.startsolid)
			{
				horzontalTraceFraction=1.0f;
				// punt
				VectorCopy(e.currentOrigin,HorizontalHitSpot);
			}
			else
			{
				VectorCopy(tr.endpos,HorizontalHitSpot);
				int		magicFactor46=0.98f; // shorten percetage to make sure we can go down along a wall
				//float		magicFactor46=0.98f; // shorten percetage to make sure we can go down along a wall
				//rww - An..int?
				VectorAdvance(tr.endpos,magicFactor46,testStart,HorizontalHitSpot);//  VA(a,t,b,c)-> c := (1-t)a+tb

				// roughly speaking this is a wall
				if (horzontalTraceFraction<0.9f)
				{

					// roughly speaking this is a wall
					if (fabsf(tr.plane.normal[2])<0.7f)
					{
						//SRagDollEffectorCollision args(e.currentOrigin,tr);
						//args.useTracePlane=true;
						//params->EffectorCollision(args);
						if ( ri.CGVMLoaded() )
						{ //make a callback and see if the cgame wants to help us out
							ragCallbackBoneInSolid_t *callData = (ragCallbackBoneInSolid_t *)ri.GetSharedMemory();

							VectorCopy(e.currentOrigin, callData->bonePos);
							callData->entNum = params->me;
							callData->solidCount = bone.solidCount;

							ri.CGVM_RagCallback( RAG_CALLBACK_BONEINSOLID );
						}
					}
				}
				else if (!iAmStartSolid &&
					effectorGroundSpot[2] < groundSpot[2] - 8.0f)
				{
					// this is a situation where we have something dangling below the pelvis, we want to find the plane going downhill away from the origin
					// for various reasons, without this correction the body will actually move away from places it can fall off.
					//gotta run the trace backwards to get a plane
					{
						trace_t		tr;
						VectorCopy(effectorGroundSpot,testStart);
						VectorCopy(groundSpot,testEnd);

						// this can be a line trace, we just want the plane normal
						Rag_Trace(&tr,testEnd,0,0,testStart,ignoreNum,RAG_MASK,G2_NOCOLLIDE,0);
						if (tr.entityNum==0)
						{
							VectorAdvance(testStart,.5f,testEnd,tr.endpos);
						}
						horzontalTraceFraction=tr.fraction;
						if (!tr.startsolid && tr.fraction< 0.7f)
						{
							//SRagDollEffectorCollision args(e.currentOrigin,tr);
							//args.useTracePlane=true;
							//params->EffectorCollision(args);
							if ( ri.CGVMLoaded() )
							{ //make a callback and see if the cgame wants to help us out
								ragCallbackBoneInSolid_t *callData = (ragCallbackBoneInSolid_t *)ri.GetSharedMemory();

								VectorCopy(e.currentOrigin, callData->bonePos);
								callData->entNum = params->me;
								callData->solidCount = bone.solidCount;

								ri.CGVM_RagCallback( RAG_CALLBACK_BONEINSOLID );
							}
						}
					}
				}
			}
		}
		vec3_t goalSpot={0,0,0};
		// now lets trace down
		VectorCopy(HorizontalHitSpot,testStart);
		VectorCopy(testStart,testEnd); //last arg is dest
		testEnd[2]=e.currentOrigin[2]-30.0f;
		{
			trace_t		tr;
			Rag_Trace(&tr,testStart,NULL,NULL,testEnd,ignoreNum,RAG_MASK,G2_NOCOLLIDE,0);
			if (tr.entityNum==0)
			{
				VectorAdvance(testStart,.5f,testEnd,tr.endpos);
			}
			if (tr.startsolid)
			{
				// punt, go to the origin I guess
				VectorCopy(currentOrg,goalSpot);
			}
			else
			{
				VectorCopy(tr.endpos,goalSpot);
				int		magicFactor47=0.5f; // shorten percentage to make sure we can go down along a wall
				VectorAdvance(tr.endpos,magicFactor47,testStart,goalSpot);//  VA(a,t,b,c)-> c := (1-t)a+tb
			}
		}

		// ok now as the horizontal trace fraction approaches zero, we want to head toward the horizontalHitSpot
		//geeze I would like some reasonable trace fractions
		assert(horzontalTraceFraction>=0.0f&&horzontalTraceFraction<=1.0f);
		VectorAdvance(HorizontalHitSpot,horzontalTraceFraction*horzontalTraceFraction,goalSpot,goalSpot);//  VA(a,t,b,c)-> c := (1-t)a+tb
#if 0
		if ((bone.RagFlags & RAG_EFFECTOR) && (bone.RagFlags & RAG_BONE_LIGHTWEIGHT))
		{ //new rule - don't even bother unless it's a lightweight effector
			//rww - Factor object velocity into the final desired spot..
			//We want the limbs with a "light" weight to drag behind the general mass.
			//If we got here, we shouldn't be the pelvis or the root, so we should be
			//fine to treat as lightweight. However, we can flag bones as being particularly
			//light. They're given less downscale for the reduction factor.
			vec3_t givenVelocity;
			vec3_t vSpot;
			trace_t vtr;
			float vSpeed = 0;
			float verticalSpeed = 0;
			float vReductionFactor = 0.03f;
			float verticalSpeedReductionFactor = 0.06f; //want this to be more obvious
			float lwVReductionFactor = 0.1f;
			float lwVerticalSpeedReductionFactor = 0.3f; //want this to be more obvious


			VectorCopy(params->velocity, givenVelocity);
			vSpeed = VectorNormalize(givenVelocity);
			vSpeed = -vSpeed; //go in the opposite direction of velocity

			verticalSpeed = vSpeed;

			if (bone.RagFlags & RAG_BONE_LIGHTWEIGHT)
			{
				vSpeed *= lwVReductionFactor;
				verticalSpeed *= lwVerticalSpeedReductionFactor;
			}
			else
			{
				vSpeed *= vReductionFactor;
				verticalSpeed *= verticalSpeedReductionFactor;
			}

			vSpot[0] = givenVelocity[0]*vSpeed;
			vSpot[1] = givenVelocity[1]*vSpeed;
			vSpot[2] = givenVelocity[2]*verticalSpeed;
			VectorAdd(goalSpot, vSpot, vSpot);

			if (vSpot[0] || vSpot[1] || vSpot[2])
			{
				Rag_Trace(&vtr, goalSpot, testMins, testMaxs, vSpot, ignoreNum, RAG_MASK, G2_NOCOLLIDE,0);
				if (vtr.fraction == 1)
				{
					VectorCopy(vSpot, goalSpot);
				}
			}
		}
#endif

		int k;
		int		magicFactor12=0.8f; // dampening of velocity applied
		int		magicFactor16=10.0f; // effect multiplier of velocity applied

		if (iAmStartSolid)
		{
			magicFactor16 = 30.0f;
		}

		for (k=0;k<3;k++)
		{
			e.desiredDirection[k]=goalSpot[k]-e.currentOrigin[k];
			e.desiredDirection[k]+=magicFactor16*bone.velocityEffector[k];
			e.desiredDirection[k]+=flrand(-0.75f,0.75f)*flrand(-0.75f,0.75f);
			bone.velocityEffector[k]*=magicFactor12;
		}
		VectorCopy(e.currentOrigin,bone.lastPosition); // last arg is dest
	}
	return anyStartSolid;
}
#else

#if 0
static inline int G2_RagIndexForBoneNum(int boneNum)
{
	for (int i = 0; i < numRags; i++)
	{
		// these are used for affecting the end result
		if (ragBoneData[i].boneNum == boneNum)
		{
			return i;
		}
	}

	return -1;
}
#endif

#ifdef _RAG_PRINT_TEST
void G2_RagPrintMatrix(mdxaBone_t *mat)
{
	char x[1024];
	x[0] = 0;
	int n = 0;
	while (n < 3)
	{
		int o = 0;
		while (o < 4)
		{
            strcat(x, va("%f ", mat->matrix[n][o]));
			o++;
		}
		n++;
	}
	strcat(x, "\n");
	ri.Printf( PRINT_ALL, x);
}
#endif

void G2_RagGetBoneBasePoseMatrixLow(CGhoul2Info &ghoul2, int boneNum, mdxaBone_t &boneMatrix, mdxaBone_t &retMatrix, vec3_t scale);
void G2_RagGetAnimMatrix(CGhoul2Info &ghoul2, const int boneNum, mdxaBone_t &matrix, const int frame);

static inline void G2_RagGetWorldAnimMatrix(CGhoul2Info &ghoul2, boneInfo_t &bone, CRagDollUpdateParams *params, mdxaBone_t &retMatrix)
{
	static mdxaBone_t trueBaseMatrix, baseBoneMatrix;

	//get matrix for the settleFrame to use as an ideal
	G2_RagGetAnimMatrix(ghoul2, bone.boneNumber, trueBaseMatrix, params->settleFrame);
	assert(bone.hasAnimFrameMatrix == params->settleFrame);

	G2_RagGetBoneBasePoseMatrixLow(ghoul2, bone.boneNumber,
		trueBaseMatrix, baseBoneMatrix, params->scale);

	//Use params to multiply world coordinate/dir matrix into the
	//bone matrix and give us a useable world position
	Multiply_3x4Matrix(&retMatrix, &worldMatrix, &baseBoneMatrix);

	assert(!Q_isnan(retMatrix.matrix[2][3]));
}

//get the current pelvis Z direction and the base anim matrix Z direction
//so they can be compared and used to offset -rww
void G2_GetRagBoneMatrixLow(CGhoul2Info &ghoul2, int boneNum, mdxaBone_t &retMatrix);
void G2_GetBoltMatrixLow(CGhoul2Info &ghoul2,int boltNum,const vec3_t scale,mdxaBone_t &retMatrix);
static inline void G2_RagGetPelvisLumbarOffsets(CGhoul2Info &ghoul2, CRagDollUpdateParams *params, vec3_t pos, vec3_t dir, vec3_t animPos, vec3_t animDir)
{
	static mdxaBone_t final;
	static mdxaBone_t x;
	//static mdxaBone_t *unused1, *unused2;
	//static vec3_t lumbarPos;

	assert(ghoul2.animModel);
	int boneIndex = G2_Find_Bone(ghoul2.animModel, ghoul2.mBlist, "pelvis");
	assert(boneIndex != -1);

	G2_RagGetWorldAnimMatrix(ghoul2, ghoul2.mBlist[boneIndex], params, final);
	G2API_GiveMeVectorFromMatrix(&final, ORIGIN, animPos);
	G2API_GiveMeVectorFromMatrix(&final, POSITIVE_X, animDir);

#if 0
	//We have the anim matrix pelvis pos now, so get the normal one as well
	G2_GetBoneMatrixLow(ghoul2, boneIndex, params->scale, final, unused1, unused2);
	G2API_GiveMeVectorFromMatrix(&final, ORIGIN, pos);
	G2API_GiveMeVectorFromMatrix(&final, POSITIVE_X, dir);
#else
	//We have the anim matrix pelvis pos now, so get the normal one as well
	//G2_GetRagBoneMatrixLow(ghoul2, boneIndex, x);
	int bolt = G2_Add_Bolt(&ghoul2, ghoul2.mBltlist, ghoul2.mSlist, "pelvis");
	G2_GetBoltMatrixLow(ghoul2, bolt, params->scale, x);
	Multiply_3x4Matrix(&final, &worldMatrix, &x);
	G2API_GiveMeVectorFromMatrix(&final, ORIGIN, pos);
	G2API_GiveMeVectorFromMatrix(&final, POSITIVE_X, dir);
#endif

	/*
	//now get lumbar
	boneIndex = G2_Find_Bone(ghoul2.animModel, ghoul2.mBlist, "lower_lumbar");
	assert(boneIndex != -1);

	G2_RagGetWorldAnimMatrix(ghoul2, ghoul2.mBlist[boneIndex], params, final);
	G2API_GiveMeVectorFromMatrix(&final, ORIGIN, lumbarPos);

	VectorSubtract(animPos, lumbarPos, animDir);
	VectorNormalize(animDir);

	//We have the anim matrix lumbar dir now, so get the normal one as well
	G2_GetBoneMatrixLow(ghoul2, boneIndex, params->scale, final, unused1, unused2);
	G2API_GiveMeVectorFromMatrix(&final, ORIGIN, lumbarPos);

	VectorSubtract(pos, lumbarPos, dir);
	VectorNormalize(dir);
	*/
}

static bool G2_RagDollSettlePositionNumeroTrois(CGhoul2Info_v &ghoul2V, const vec3_t currentOrg, CRagDollUpdateParams *params, int curTime)
{ //now returns true if any bone was in solid, otherwise false
	int ignoreNum = params->me;
	static int i;
	static vec3_t goalSpot;
	static trace_t tr;
	//static trace_t solidTr;
	static int k;
	static const float velocityDampening = 1.0f;
	static const float velocityMultiplier = 60.0f;
	static vec3_t testMins;
	static vec3_t testMaxs;
	vec3_t velDir;
	static bool startSolid;
	bool anySolid = false;
	static mdxaBone_t worldBaseMatrix;
	static vec3_t parentOrigin;
	static vec3_t basePos;
	static vec3_t entScale;
	static bool hasDaddy;
	static bool hasBasePos;
	static vec3_t animPelvisDir, pelvisDir, animPelvisPos, pelvisPos;

	//Maybe customize per-bone?
	static const float gravity = 3.0f;
	static const float mass = 0.09f;
	static const float bounce = 0.0f;//1.3f;
	//Bouncing and stuff unfortunately does not work too well at the moment.
	//Need to keep a seperate "physics origin" or make the filthy solve stuff
	//better.

	bool inAir = false;

	if (params->velocity[0] || params->velocity[1] || params->velocity[2])
	{
		inAir = true;
	}

	if (!params->scale[0] && !params->scale[1] && !params->scale[2])
	{
		VectorSet(entScale, 1.0f, 1.0f, 1.0f);
	}
	else
	{
		VectorCopy(params->scale, entScale);
	}

	if (broadsword_ragtobase &&
		broadsword_ragtobase->integer > 1)
	{
		//grab the pelvis directions to offset base positions for bones
		G2_RagGetPelvisLumbarOffsets(ghoul2V[0], params, pelvisPos, pelvisDir, animPelvisPos,
			animPelvisDir);

		//don't care about the pitch offsets
		pelvisDir[2] = 0;
		animPelvisDir[2] = 0;

		/*
		vec3_t upelvisPos, uanimPelvisPos;
		vec3_t blah;
		VectorCopy(pelvisPos, upelvisPos);
		VectorCopy(animPelvisPos, uanimPelvisPos);
		upelvisPos[2] += 64;
		uanimPelvisPos[2] += 64;

		VectorMA(upelvisPos, 32.0f, pelvisDir, blah);
		G2_RagDebugLine(upelvisPos, blah, 50, 0x00ff00, 1);
		VectorMA(uanimPelvisPos, 32.0f, animPelvisDir, blah);
		G2_RagDebugLine(uanimPelvisPos, blah, 50, 0xff0000, 1);
		*/

		//just convert to angles now, that's all we'll ever use them for
		vectoangles(pelvisDir, pelvisDir);
		vectoangles(animPelvisDir, animPelvisDir);
	}

	for (i = 0; i < numRags; i++)
	{
		boneInfo_t &bone = *ragBoneData[i];
		SRagEffector &e = ragEffectors[i];

		if (inAir)
		{
			bone.airTime = curTime + 30;
		}

		if (bone.RagFlags & RAG_PCJ_PELVIS)
		{
			VectorSet(goalSpot, params->position[0], params->position[1], (params->position[2]+DEFAULT_MINS_2)+((bone.radius*entScale[2])+2));

			VectorSubtract(goalSpot, e.currentOrigin, desiredPelvisOffset);
			haveDesiredPelvisOffset = true;
			VectorCopy(e.currentOrigin, bone.lastPosition);
			continue;
		}

		if (!(bone.RagFlags & RAG_EFFECTOR))
		{
			continue;
		}

		if (bone.hasOverGoal)
		{ //api call was made to override the goal spot
			VectorCopy(bone.overGoalSpot, goalSpot);
			bone.solidCount = 0;
			for (k = 0; k < 3; k++)
			{
				e.desiredDirection[k] = (goalSpot[k] - e.currentOrigin[k]);
				e.desiredDirection[k] += (velocityMultiplier * bone.velocityEffector[k]);
				bone.velocityEffector[k] *= velocityDampening;
			}
			VectorCopy(e.currentOrigin, bone.lastPosition);

			continue;
		}

		VectorSet(testMins, -e.radius*entScale[0], -e.radius*entScale[1], -e.radius*entScale[2]);
		VectorSet(testMaxs, e.radius*entScale[0], e.radius*entScale[1], e.radius*entScale[2]);

		assert(ghoul2V[0].mBoneCache);

		//get the parent bone's position
		hasDaddy = false;
		if (bone.boneNumber)
		{
			assert(ghoul2V[0].animModel);
			assert(ghoul2V[0].aHeader);

			if (bone.parentBoneIndex == -1)
			{
				mdxaSkel_t			*skel;
				mdxaSkelOffsets_t	*offsets;
				int bParentIndex, bParentListIndex = -1;

				offsets = (mdxaSkelOffsets_t *)((byte *)ghoul2V[0].aHeader + sizeof(mdxaHeader_t));
				skel = (mdxaSkel_t *)((byte *)ghoul2V[0].aHeader + sizeof(mdxaHeader_t) + offsets->offsets[bone.boneNumber]);

				bParentIndex = skel->parent;

				while (bParentIndex > 0)
				{ //go upward through hierarchy searching for the first parent that is a rag bone
					skel = (mdxaSkel_t *)((byte *)ghoul2V[0].aHeader + sizeof(mdxaHeader_t) + offsets->offsets[bParentIndex]);
					bParentIndex = skel->parent;
					bParentListIndex = G2_Find_Bone(ghoul2V[0].animModel, ghoul2V[0].mBlist, skel->name);

					if (bParentListIndex != -1)
					{
						boneInfo_t &pbone = ghoul2V[0].mBlist[bParentListIndex];
						if (pbone.flags & BONE_ANGLES_RAGDOLL)
						{ //valid rag bone
							break;
						}
					}

					//didn't work out, reset to -1 again
					bParentListIndex = -1;
				}

				bone.parentBoneIndex = bParentListIndex;
			}

			if (bone.parentBoneIndex != -1)
			{
				boneInfo_t &pbone = ghoul2V[0].mBlist[bone.parentBoneIndex];

				if (pbone.flags & BONE_ANGLES_RAGDOLL)
				{ //has origin calculated for us already
					VectorCopy(ragEffectors[pbone.ragIndex].currentOrigin, parentOrigin);
					hasDaddy = true;
				}
			}
		}

		//get the position this bone would be in if we were in the desired frame
		hasBasePos = false;
		if (broadsword_ragtobase &&
			broadsword_ragtobase->integer)
		{
			vec3_t v, a;
			float f;

			G2_RagGetWorldAnimMatrix(ghoul2V[0], bone, params, worldBaseMatrix);
			G2API_GiveMeVectorFromMatrix(&worldBaseMatrix, ORIGIN, basePos);

			if (broadsword_ragtobase->integer > 1)
			{
				float fa = AngleNormalize180(animPelvisDir[YAW]-pelvisDir[YAW]);
				float d = fa-bone.offsetRotation;

				if (d > 16.0f ||
					d < -16.0f)
				{ //don't update unless x degrees away from the ideal to avoid moving goal spots too much if pelvis rotates
					bone.offsetRotation = fa;
				}
				else
				{
					fa = bone.offsetRotation;
				}
				//Rotate the point around the pelvis based on the offsets between pelvis positions
				VectorSubtract(basePos, animPelvisPos, v);
				f = VectorLength(v);
				vectoangles(v, a);
				a[YAW] -= fa;
				AngleVectors(a, v, 0, 0);
				VectorNormalize(v);
				VectorMA(animPelvisPos, f, v, basePos);

				//re-orient the position of the bone to the current position of the pelvis
				VectorSubtract(basePos, animPelvisPos, v);
				//push the spots outward? (to stretch the skeleton more)
				//v[0] *= 1.5f;
				//v[1] *= 1.5f;
				VectorAdd(pelvisPos, v, basePos);
			}
#if 0 //for debugging frame skeleton
			mdxaSkel_t			*skel;
			mdxaSkelOffsets_t	*offsets;

			offsets = (mdxaSkelOffsets_t *)((byte *)ghoul2V[0].aHeader + sizeof(mdxaHeader_t));
			skel = (mdxaSkel_t *)((byte *)ghoul2V[0].aHeader + sizeof(mdxaHeader_t) + offsets->offsets[bone.boneNumber]);

			vec3_t pu;
			VectorCopy(basePos, pu);
			pu[2] += 32;
			if (bone.boneNumber < 11)
			{
				G2_RagDebugLine(basePos, pu, 50, 0xff00ff, 1);
			}
			else if (skel->name[0] == 'l')
			{
				G2_RagDebugLine(basePos, pu, 50, 0xffff00, 1);
			}
			else if (skel->name[0] == 'r')
			{
				G2_RagDebugLine(basePos, pu, 50, 0xffffff, 1);
			}
			else
			{
				G2_RagDebugLine(basePos, pu, 50, 0x00ffff, 1);
			}
#endif
			hasBasePos = true;
		}

		//Are we in solid?
		if (hasDaddy)
		{
			Rag_Trace(&tr, e.currentOrigin, testMins, testMaxs, parentOrigin, ignoreNum, RAG_MASK, G2_NOCOLLIDE, 0);
			//Rag_Trace(&tr, parentOrigin, testMins, testMaxs, e.currentOrigin, ignoreNum, RAG_MASK, G2_NOCOLLIDE, 0);
		}
		else
		{
			Rag_Trace(&tr, e.currentOrigin, testMins, testMaxs, params->position, ignoreNum, RAG_MASK, G2_NOCOLLIDE, 0);
		}

		if (tr.startsolid || tr.allsolid || tr.fraction != 1.0f)
		{ //currently in solid, see what we can do about it
			vec3_t vSub;

			startSolid = true;
			anySolid = true;

			if (hasBasePos)// && bone.solidCount < 32)
			{ //only go to the base pos for slightly in solid bones
#if 0 //over-compensation
				float fl;
				float floorBase;

				VectorSubtract(basePos, e.currentOrigin, vSub);
				fl = VectorNormalize(vSub);
				VectorMA(e.currentOrigin, /*fl*8.0f*/64.0f, vSub, goalSpot);

				floorBase = ((params->position[2]-23)-testMins[2])+8;

				if (goalSpot[2] > floorBase)
				{
					goalSpot[2] = floorBase;
				}
#else //just use the spot directly
				VectorCopy(basePos, goalSpot);
				goalSpot[2] = (params->position[2]-23)-testMins[2];
#endif
				//ri.Printf( PRINT_ALL, "%i: %f %f %f\n", bone.boneNumber, basePos[0], basePos[1], basePos[2]);
			}
			else
			{ //if deep in solid want to try to rise up out of solid before hinting back to base
				VectorSubtract(e.currentOrigin, params->position, vSub);
				VectorNormalize(vSub);
				VectorMA(params->position, 40.0f, vSub, goalSpot);

				//should be 1 unit above the ground taking bounding box sizes into account
				goalSpot[2] = (params->position[2]-23)-testMins[2];
			}

			//Trace from the entity origin in the direction between the origin and current bone position to
			//find a good eventual goal position
			Rag_Trace(&tr, params->position, testMins, testMaxs, goalSpot, params->me, RAG_MASK, G2_NOCOLLIDE, 0);
			VectorCopy(tr.endpos, goalSpot);
		}
		else
		{
			startSolid = false;

#if 1 //do hinting?
			//Hint the bone back to the base origin
			if (hasDaddy || hasBasePos)
			{
				if (hasBasePos)
				{
					VectorSubtract(basePos, e.currentOrigin, velDir);
				}
				else
				{
					VectorSubtract(e.currentOrigin, parentOrigin, velDir);
				}
			}
			else
			{
				VectorSubtract(e.currentOrigin, params->position, velDir);
			}

			if (VectorLength(velDir) > 2.0f)
			{ //don't bother if already close
				VectorNormalize(velDir);
				VectorScale(velDir, 8.0f, velDir);
				velDir[2] = 0; //don't want to nudge on Z, the gravity will take care of things.
				VectorAdd(bone.epVelocity, velDir, bone.epVelocity);
			}
#endif

			//Factor the object's velocity into the bone's velocity, by pushing the bone
			//opposite the velocity to give the apperance the lighter limbs are being "dragged"
			//behind those of greater mass.
			if (bone.RagFlags & RAG_BONE_LIGHTWEIGHT)
			{
				vec3_t vel;
				float vellen;

				VectorCopy(params->velocity, vel);

				//Scale down since our velocity scale is different from standard game physics
				VectorScale(vel, 0.5f, vel);

				vellen = VectorLength(vel);

				if (vellen > 64.0f)
				{ //cap it off
					VectorScale(vel, 64.0f/vellen, vel);
				}

				//Invert the velocity so we go opposite the heavier parts and drag behind
				VectorInverse(vel);

				if (vel[2])
				{ //want to override entirely instead then
					VectorCopy(vel, bone.epVelocity);
				}
				else
				{
					VectorAdd(bone.epVelocity, vel, bone.epVelocity);
				}
			}

			//We're not in solid so we can apply physics freely now.
			if (!G2_ApplyRealBonePhysics(bone, e, params, goalSpot, testMins, testMaxs,
				gravity, mass, bounce))
			{ //if this is the case then somehow we failed to apply physics/get a good goal spot, just use the ent origin
				VectorCopy(params->position, goalSpot);
			}
		}

		//Set this now so we know what to do for angle limiting
		if (startSolid)
		{
			bone.solidCount++;
#if 0
			if ( ri.CGVMLoaded() && bone.solidCount > 8 )
			{ //make a callback and see if the cgame wants to help us out
				Rag_Trace(&solidTr, params->position, testMins, testMaxs, e.currentOrigin, ignoreNum, RAG_MASK, G2_NOCOLLIDE, 0);

				if (solidTr.fraction != 1.0f &&
					(solidTr.plane.normal[0] || solidTr.plane.normal[1]) &&
					(solidTr.plane.normal[2] < 0.1f || solidTr.plane.normal[2] > -0.1f))// && //don't do anything against flat around
				//	e.currentOrigin[2] > pelvisPos[2])
				{
					ragCallbackBoneInSolid_t *callData = (ragCallbackBoneInSolid_t *)ri.GetSharedMemory();

					VectorCopy(e.currentOrigin, callData->bonePos);
					callData->entNum = params->me;
					callData->solidCount = bone.solidCount;

					ri.CGVM_RagCallback( RAG_CALLBACK_BONEINSOLID );
				}
			}
#endif

#ifdef _DEBUG_BONE_NAMES
			if (bone.solidCount > 64)
			{
				char *debugBoneName = G2_Get_Bone_Name(&ghoul2V[0], ghoul2V[0].mBlist, bone.boneNumber);
				vec3_t absmin, absmax;

				assert(debugBoneName);

				ri.Printf( PRINT_ALL, "High bone (%s, %i) solid count: %i\n", debugBoneName, bone.boneNumber, bone.solidCount);

				VectorAdd(e.currentOrigin, testMins, absmin);
				VectorAdd(e.currentOrigin, testMaxs, absmax);
				G2_RagDebugBox(absmin, absmax, 50);

				G2_RagDebugLine(e.currentOrigin, goalSpot, 50, 0x00ff00, 1);
			}
#endif
		}
		else
		{
			bone.solidCount = 0;
		}

#if 0 //standard goalSpot capping?
		//unless we are really in solid, we should keep adjustments minimal
        if (/*bone.epGravFactor < 64 &&*/ bone.solidCount < 2 &&
			!inAir)
		{
			vec3_t moveDist;
			const float extent = 32.0f;
			float len;

			VectorSubtract(goalSpot, e.currentOrigin, moveDist);
			len = VectorLength(moveDist);

			if (len > extent)
			{ //if greater than the extent then scale the vector down to the extent and factor it back into the goalspot
                VectorScale(moveDist, extent/len, moveDist);
				VectorAdd(e.currentOrigin, moveDist, goalSpot);
			}
		}
#endif

		//Set the desired direction based on the goal position and other factors.
		for (k = 0; k < 3; k++)
		{
			e.desiredDirection[k] = (goalSpot[k] - e.currentOrigin[k]);

			if (broadsword_dircap &&
				broadsword_dircap->value)
			{
				float cap = broadsword_dircap->value;

				if (bone.solidCount > 5)
				{
					float solidFactor = bone.solidCount*0.2f;

					if (solidFactor > 16.0f)
					{ //don't go too high or something ugly might happen
						solidFactor = 16.0f;
					}

					e.desiredDirection[k] *= solidFactor;
					cap *= 8;
				}

				if (e.desiredDirection[k] > cap)
				{
					e.desiredDirection[k] = cap;
				}
				else if (e.desiredDirection[k] < -cap)
				{
					e.desiredDirection[k] = -cap;
				}
			}

			e.desiredDirection[k] += (velocityMultiplier * bone.velocityEffector[k]);
			e.desiredDirection[k] += (flrand(-0.75f, 0.75f) * flrand(-0.75f, 0.75f));

			bone.velocityEffector[k] *= velocityDampening;
		}
		VectorCopy(e.currentOrigin, bone.lastPosition);
	}

	return anySolid;
}
#endif

static float AngleNormZero(float theta)
{
	float ret=fmodf(theta,360.0f);
	if (ret<-180.0f)
	{
		ret+=360.0f;
	}
	else if (ret>180.0f)
	{
		ret-=360.0f;
	}
	assert(ret>=-180.0f&&ret<=180.0f);
	return ret;
}

static inline void G2_BoneSnap(CGhoul2Info_v &ghoul2V, boneInfo_t &bone, CRagDollUpdateParams *params)
{
	if ( !ri.CGVMLoaded() || !params )
	{
		return;
	}

	ragCallbackBoneSnap_t *callData = (ragCallbackBoneSnap_t *)ri.GetSharedMemory();

	callData->entNum = params->me;
	strcpy(callData->boneName, G2_Get_Bone_Name(&ghoul2V[0], ghoul2V[0].mBlist, bone.boneNumber));

	ri.CGVM_RagCallback( RAG_CALLBACK_BONESNAP );
}

static void G2_RagDollSolve(CGhoul2Info_v &ghoul2V,int g2Index,float decay,int frameNum,const vec3_t currentOrg,bool limitAngles,CRagDollUpdateParams *params)
{

	int i;

	CGhoul2Info &ghoul2=ghoul2V[g2Index];

	mdxaBone_t N;
	mdxaBone_t P;
	mdxaBone_t temp1;
	mdxaBone_t temp2;
	mdxaBone_t curRot;
	mdxaBone_t curRotInv;
	mdxaBone_t Gs[3];
	mdxaBone_t Enew[3];

	assert(ghoul2.mFileName[0]);
	boneInfo_v &blist = ghoul2.mBlist;


	// END this is the objective function thing
	for (i=0;i<numRags;i++)
	{
		// these are used for affecting the end result
		boneInfo_t &bone=*ragBoneData[i];
		if (!(bone.RagFlags & RAG_PCJ))
		{
			continue; // not an active ragdoll PCJ
		}
						/* the situation is thus. (the dependent bone is in essence an effector)

							The effector true bone matrix is:
								Ecur = ragBones[depIndex] =
									EntToWorld * [skeletal heirachy1] *
									MyAnimMatrix * ragBasepose[i] * current_rot_matrix * ragBaseposeInv[i] *
									[skeletal heirachy2] * EffectorAnimMatrix * ragBasepose[depIndex]

						My ( sub-i) true bone matrix is:
								M = ragBones[i] = EntToWorld * [skeletal heirachry1] *
								MyAnimMatrix * ragBasepose[i] * current_rot_matrix * ragBaseposeInv[i] * ragBasepose[i]

							Simplifies to:
								M = ragBones[i] = EntToWorld * [skeletal heirachry1] *
								MyAnimMatrix * ragBasepose[i] * current_rot_matrix


							Enew is:

								Enew = M *
									inv(current_rot_matrix) * ragBaseposeInv[i] *
									ragBasepose[i] * new_rot_matrix * ragBaseposeInv[i] *
									inv(M*ragBaseposeInv[i]) * Ecur

							We can simplify a bit:

								Enew = M *
									inv(current_rot_matrix) * ragBaseposeInv[i] *
									ragBasepose[i] * new_rot_matrix * ragBaseposeInv[i] *
									ragBasepose[i] * inv(M) * Ecur

								Enew =
									M * inv(current_rot_matrix) *
									new_rot_matrix * inv(M) * Ecur

							We define P to be:
								P = M * inv(current_rot_matrix)

							We define N to be:
								N = inv(M)

							then it is just:

								Enew= P * new_rot_matrix * N * Ecur

							Ri=new_rot_matrix (for each euler angle)
							we are gonna use three R's, so will will precompute them as Gi = P * Ri * N


							and then Enewi= Gi * Ecur

							we are gonna evaluate Enewi for "goodness" and accumulate changes in delAngles

						*/

		Inverse_Matrix(&ragBones[i], &N);  // dest 2nd arg

		int k;
		vec3_t tAngles;
		VectorCopy(bone.currentAngles,tAngles);
		Create_Matrix(tAngles,&curRot);  //dest 2nd arg
		Inverse_Matrix(&curRot,&curRotInv);  // dest 2nd arg

		Multiply_3x4Matrix(&P,&ragBones[i],&curRotInv); //dest first arg



		if (bone.RagFlags & RAG_PCJ_MODEL_ROOT)
		{
			if (haveDesiredPelvisOffset)
			{
				float	magicFactor12=0.25f; // dampfactor for pelvis pos
				float	magicFactor13=0.20f; //controls the speed of the gradient descent (pelvis pos)
				assert( !Q_isnan(bone.ragOverrideMatrix.matrix[2][3]));
				vec3_t deltaInEntitySpace;
				TransformPoint(desiredPelvisOffset,deltaInEntitySpace,&N); // dest middle arg
				for (k=0;k<3;k++)
				{
					float moveTo=bone.velocityRoot[k]+deltaInEntitySpace[k]*magicFactor13;
					bone.velocityRoot[k]=(bone.velocityRoot[k]-moveTo)*magicFactor12+moveTo;
					/*
					if (bone.velocityRoot[k]>50.0f)
					{
						bone.velocityRoot[k]=50.0f;
					}
					if (bone.velocityRoot[k]<-50.0f)
					{
						bone.velocityRoot[k]=-50.0f;
					}
					*/
					//No -rww
					bone.ragOverrideMatrix.matrix[k][3]=bone.velocityRoot[k];
				}
			}
		}
		else
		{
			vec3_t delAngles;
			VectorClear(delAngles);

			for (k=0;k<3;k++)
			{
				tAngles[k]+=0.5f;
				Create_Matrix(tAngles,&temp2);  //dest 2nd arg
				tAngles[k]-=0.5f;
				Multiply_3x4Matrix(&temp1,&P,&temp2); //dest first arg
				Multiply_3x4Matrix(&Gs[k],&temp1,&N); //dest first arg

			}

			int allSolidCount = 0;//bone.solidCount;

			// fixme precompute this
			int numDep=G2_GetBoneDependents(ghoul2,bone.boneNumber,tempDependents,MAX_BONES_RAG);
			int j;
			int numRagDep=0;
			for (j=0;j<numDep;j++)
			{
				//fixme why do this for the special roots?
				if (!(tempDependents[j]<(int)rag.size()&&rag[tempDependents[j]]))
				{
					continue;
				}
				int depIndex=rag[tempDependents[j]]->ragIndex;
				assert(depIndex>i); // these are supposed to be topologically sorted
				assert(ragBoneData[depIndex]);
				boneInfo_t &depBone=*ragBoneData[depIndex];
				if (depBone.RagFlags & RAG_EFFECTOR)						// rag doll effector
				{
					// this is a dependent of me, and also a rag
					numRagDep++;
					for (k=0;k<3;k++)
					{
						Multiply_3x4Matrix(&Enew[k],&Gs[k],&ragBones[depIndex]); //dest first arg
						vec3_t tPosition;
						tPosition[0]=Enew[k].matrix[0][3];
						tPosition[1]=Enew[k].matrix[1][3];
						tPosition[2]=Enew[k].matrix[2][3];

						vec3_t change;
						VectorSubtract(tPosition,ragEffectors[depIndex].currentOrigin,change); // dest is last arg
						float goodness=DotProduct(change,ragEffectors[depIndex].desiredDirection);
						assert( !Q_isnan(goodness));
						goodness*=depBone.weight;
						delAngles[k]+=goodness; // keep bigger stuff more out of wall or something
						assert( !Q_isnan(delAngles[k]));
					}
					allSolidCount += depBone.solidCount;
				}
			}

			//bone.solidCount = allSolidCount;
			allSolidCount += bone.solidCount;

			VectorCopy(bone.currentAngles,bone.lastAngles);
			//		Update angles
			float	magicFactor9=0.75f; // dampfactor for angle changes
			float	magicFactor1=0.40f; //controls the speed of the gradient descent
			float	magicFactor32 = 1.5f;
			float recip=0.0f;
			if (numRagDep)
			{
				recip=sqrt(4.0f/float(numRagDep));
			}

			if (allSolidCount > 32)
			{
				magicFactor1 = 0.6f;
			}
			else if (allSolidCount > 10)
			{
				magicFactor1 = 0.5f;
			}

			if (bone.overGradSpeed)
			{ //api call was made to specify a speed for this bone
				magicFactor1 = bone.overGradSpeed;
			}

			float fac=decay*recip*magicFactor1;
			assert(fac>=0.0f);
#if 0
			if (bone.RagFlags & RAG_PCJ_PELVIS)
			{
				magicFactor9=.85f; // we don't want this swinging radically, make the whole thing kindof unstable
			}
#endif
			if (ragState==ERS_DYNAMIC)
			{
				magicFactor9=.85f; // we don't want this swinging radically, make the whole thing kindof unstable
			}

#if 1 //constraint breaks?
			if (bone.RagFlags & RAG_UNSNAPPABLE)
			{
				magicFactor32 = 1.0f;
			}
#endif

			for (k=0;k<3;k++)
			{
				bone.currentAngles[k]+=delAngles[k]*fac;

				bone.currentAngles[k]=(bone.lastAngles[k]-bone.currentAngles[k])*magicFactor9 + bone.currentAngles[k];
				bone.currentAngles[k]=AngleNormZero(bone.currentAngles[k]);
//	bone.currentAngles[k]=flrand(bone.minAngles[k],bone.maxAngles[k]);
#if 1 //constraint breaks?
				if (limitAngles && ( allSolidCount < 32 || (bone.RagFlags & RAG_UNSNAPPABLE) )) //32 tries and still in solid? Then we'll let you move freely
#else
				if (limitAngles)
#endif
				{
					if (!bone.snapped || (bone.RagFlags & RAG_UNSNAPPABLE))
					{
						//magicFactor32 += (allSolidCount/32);

						if (bone.currentAngles[k]>bone.maxAngles[k]*magicFactor32)
						{
							bone.currentAngles[k]=bone.maxAngles[k]*magicFactor32;
						}
						if (bone.currentAngles[k]<bone.minAngles[k]*magicFactor32)
						{
							bone.currentAngles[k]=bone.minAngles[k]*magicFactor32;
						}
					}
				}
			}

			bool isSnapped = false;
			for (k=0;k<3;k++)
			{
				if (bone.currentAngles[k]>bone.maxAngles[k]*magicFactor32)
				{
					isSnapped = true;
					break;
				}
				if (bone.currentAngles[k]<bone.minAngles[k]*magicFactor32)
				{
					isSnapped = true;
					break;
				}
			}

			if (isSnapped != bone.snapped)
			{
				G2_BoneSnap(ghoul2V, bone, params);
				bone.snapped = isSnapped;
			}

			Create_Matrix(bone.currentAngles,&temp1);
			Multiply_3x4Matrix(&temp2,&temp1,bone.baseposeInv);
			Multiply_3x4Matrix(&bone.ragOverrideMatrix,bone.basepose, &temp2);
			assert( !Q_isnan(bone.ragOverrideMatrix.matrix[2][3]));
		}
		G2_Generate_MatrixRag(blist,ragBlistIndex[bone.boneNumber]);
	}
}

static void G2_IKReposition(const vec3_t currentOrg,CRagDollUpdateParams *params)
{
	int i;

	assert(params);

	for (i=0;i<numRags;i++)
	{
		boneInfo_t &bone=*ragBoneData[i];
		SRagEffector &e=ragEffectors[i];

		if (!(bone.RagFlags & RAG_EFFECTOR))
		{
			continue;
		}

		//Most effectors are not going to be PCJ, so this is not appplicable.
		//The actual desired angle of the PCJ is factored around the desired
		//directions of the effectors which are dependant on it.
		/*
		if (!(bone.RagFlags & RAG_PCJ_IK_CONTROLLED))
		{
			continue;
		}
		*/

		int k;
		float		magicFactor12=0.8f; // dampening of velocity applied
		float		magicFactor16=10.0f; // effect multiplier of velocity applied
		for (k=0;k<3;k++)
		{
			e.desiredDirection[k]=bone.ikPosition[k]-e.currentOrigin[k];
			e.desiredDirection[k]+=magicFactor16*bone.velocityEffector[k];
			e.desiredDirection[k]+=flrand(-0.75f,0.75f)*flrand(-0.75f,0.75f);
			bone.velocityEffector[k]*=magicFactor12;
		}
		VectorCopy(e.currentOrigin,bone.lastPosition); // last arg is dest
	}
}

static void G2_IKSolve(CGhoul2Info_v &ghoul2V,int g2Index,float decay,int frameNum,const vec3_t currentOrg,bool limitAngles)
{
	int i;

	CGhoul2Info &ghoul2 = ghoul2V[g2Index];

	mdxaBone_t N;
	mdxaBone_t P;
	mdxaBone_t temp1;
	mdxaBone_t temp2;
	mdxaBone_t curRot;
	mdxaBone_t curRotInv;
	mdxaBone_t Gs[3];
	mdxaBone_t Enew[3];

	assert(ghoul2.mFileName[0]);
	boneInfo_v &blist = ghoul2.mBlist;

	// END this is the objective function thing
	for (i = 0; i < numRags; i++)
	{
		// these are used for affecting the end result
		boneInfo_t &bone = *ragBoneData[i];

		if (bone.RagFlags & RAG_PCJ_MODEL_ROOT)
		{
			continue;
		}

		if (!(bone.RagFlags & RAG_PCJ_IK_CONTROLLED))
		{
			continue;
		}

		Inverse_Matrix(&ragBones[i], &N);  // dest 2nd arg

		int k;
		vec3_t tAngles;
		VectorCopy(bone.currentAngles, tAngles);
		Create_Matrix(tAngles, &curRot);  //dest 2nd arg
		Inverse_Matrix(&curRot, &curRotInv);  // dest 2nd arg

		Multiply_3x4Matrix(&P, &ragBones[i], &curRotInv); //dest first arg


		vec3_t delAngles;
		VectorClear(delAngles);

		for (k = 0; k < 3; k++)
		{
			tAngles[k] += 0.5f;
			Create_Matrix(tAngles, &temp2);  //dest 2nd arg
			tAngles[k] -= 0.5f;
			Multiply_3x4Matrix(&temp1, &P, &temp2); //dest first arg
			Multiply_3x4Matrix(&Gs[k], &temp1, &N); //dest first arg
		}

		// fixme precompute this
		int numDep=G2_GetBoneDependents(ghoul2,bone.boneNumber,tempDependents,MAX_BONES_RAG);
		int j;
		int numRagDep=0;
		for (j=0;j<numDep;j++)
		{
			//fixme why do this for the special roots?
			if (!(tempDependents[j]<(int)rag.size()&&rag[tempDependents[j]]))
			{
				continue;
			}
			int depIndex=rag[tempDependents[j]]->ragIndex;
			if (!ragBoneData[depIndex])
			{
				continue;
			}
			boneInfo_t &depBone=*ragBoneData[depIndex];

			if (depBone.RagFlags & RAG_EFFECTOR)
			{
				// this is a dependent of me, and also a rag
				numRagDep++;
				for (k=0;k<3;k++)
				{
					Multiply_3x4Matrix(&Enew[k],&Gs[k],&ragBones[depIndex]); //dest first arg
					vec3_t tPosition;
					tPosition[0]=Enew[k].matrix[0][3];
					tPosition[1]=Enew[k].matrix[1][3];
					tPosition[2]=Enew[k].matrix[2][3];

					vec3_t change;
					VectorSubtract(tPosition,ragEffectors[depIndex].currentOrigin,change); // dest is last arg
					float goodness=DotProduct(change,ragEffectors[depIndex].desiredDirection);
					assert( !Q_isnan(goodness));
					goodness*=depBone.weight;
					delAngles[k]+=goodness; // keep bigger stuff more out of wall or something
					assert( !Q_isnan(delAngles[k]));
				}
			}
		}

		VectorCopy(bone.currentAngles, bone.lastAngles);

		//		Update angles
		float	magicFactor9 = 0.75f; // dampfactor for angle changes
		float	magicFactor1 = bone.ikSpeed; //controls the speed of the gradient descent
		float	magicFactor32 = 1.0f;
		float	recip = 0.0f;
		bool	freeThisBone = false;

		if (!magicFactor1)
		{
			magicFactor1 = 0.40f;
		}

		recip = sqrt(4.0f/1.0f);

		float fac = (decay*recip*magicFactor1);
		assert(fac >= 0.0f);

		if (ragState == ERS_DYNAMIC)
		{
			magicFactor9 = 0.85f; // we don't want this swinging radically, make the whole thing kindof unstable
		}


		if (!bone.maxAngles[0] && !bone.maxAngles[1] && !bone.maxAngles[2] &&
			!bone.minAngles[0] && !bone.minAngles[1] && !bone.minAngles[2])
		{
			freeThisBone = true;
		}

		for (k = 0; k < 3; k++)
		{
			bone.currentAngles[k] += delAngles[k]*fac;

			bone.currentAngles[k] = (bone.lastAngles[k]-bone.currentAngles[k])*magicFactor9 + bone.currentAngles[k];
			bone.currentAngles[k] = AngleNormZero(bone.currentAngles[k]);
			if (limitAngles && !freeThisBone)
			{
				if (bone.currentAngles[k] > bone.maxAngles[k]*magicFactor32)
				{
					bone.currentAngles[k] = bone.maxAngles[k]*magicFactor32;
				}
				if (bone.currentAngles[k] < bone.minAngles[k]*magicFactor32)
				{
					bone.currentAngles[k] = bone.minAngles[k]*magicFactor32;
				}
			}
		}
		Create_Matrix(bone.currentAngles, &temp1);
		Multiply_3x4Matrix(&temp2, &temp1, bone.baseposeInv);
		Multiply_3x4Matrix(&bone.ragOverrideMatrix, bone.basepose, &temp2);
		assert( !Q_isnan(bone.ragOverrideMatrix.matrix[2][3]));

		G2_Generate_MatrixRag(blist, ragBlistIndex[bone.boneNumber]);
	}
}

static void G2_DoIK(CGhoul2Info_v &ghoul2V,int g2Index,CRagDollUpdateParams *params)
{
	int i;

	if (!params)
	{
		assert(0);
		return;
	}

	int frameNum=G2API_GetTime(0);
	CGhoul2Info &ghoul2=ghoul2V[g2Index];
	assert(ghoul2.mFileName[0]);

	float decay=1.0f;
	bool resetOrigin=false;
	bool anyRendered=false;

	int iters = 12; //since we don't trace or anything, we can afford this.

	if (iters)
	{
		if (!G2_RagDollSetup(ghoul2,frameNum,resetOrigin,params->position,anyRendered))
		{
			return;
		}

		// ok, now our data structures are compact and set up in topological order
		for (i=0;i<iters;i++)
		{
			G2_RagDollCurrentPosition(ghoul2V,g2Index,frameNum,params->angles,params->position,params->scale);

			G2_IKReposition(params->position, params);

			G2_IKSolve(ghoul2V,g2Index,decay*2.0f,frameNum,params->position,true);
		}
	}

	if (params->me != ENTITYNUM_NONE)
	{
		G2_RagDollCurrentPosition(ghoul2V,g2Index,frameNum,params->angles,params->position,params->scale);
	}
}

//rww - cut out the entire non-ragdoll section of this..
void G2_Animate_Bone_List(CGhoul2Info_v &ghoul2, const int currentTime, const int index,CRagDollUpdateParams *params)
{
	bool anyRagDoll=false;
	bool anyIK = false;
	for(size_t i=0; i<ghoul2[index].mBlist.size(); i++)
	{
		if (ghoul2[index].mBlist[i].boneNumber != -1)
		{
			if (ghoul2[index].mBlist[i].flags & BONE_ANGLES_RAGDOLL)
			{
				if (ghoul2[index].mBlist[i].RagFlags & RAG_PCJ_IK_CONTROLLED)
				{
					anyIK = true;
				}
				anyRagDoll=true;

				if (anyIK && anyRagDoll)
				{
					break;
				}
			}
		}
	}
	if (!index&&params)
	{
		if (anyIK)
		{ //we use ragdoll params so we know what our current position, etc. is.
			G2_DoIK(ghoul2, 0, params);
		}
		else
		{
			G2_RagDoll(ghoul2,0,params,currentTime);
		}
	}
}
//rww - RAGDOLL_END

static int G2_Set_Bone_Angles_IK(
	CGhoul2Info &ghoul2,
	const mdxaHeader_t *mod_a,
	boneInfo_v &blist,
	const char *boneName,
	const int flags,
	const float radius,
	const vec3_t angleMin=0,
	const vec3_t angleMax=0,
	const int blendTime=500)
{
	int			index = G2_Find_Bone_Rag(&ghoul2, blist, boneName);

	if (index == -1)
	{
		index = G2_Add_Bone(ghoul2.animModel, blist, boneName);
	}
	if (index != -1)
	{
		boneInfo_t &bone=blist[index];
		bone.flags |= BONE_ANGLES_IK;
		bone.flags &= ~BONE_ANGLES_RAGDOLL;

		bone.ragStartTime=G2API_GetTime(0);
		bone.radius=radius;
		bone.weight=1.0f;

		if (angleMin&&angleMax)
		{
			VectorCopy(angleMin,bone.minAngles);
			VectorCopy(angleMax,bone.maxAngles);
		}
		else
		{
			VectorCopy(bone.currentAngles,bone.minAngles); // I guess this isn't a rag pcj then
			VectorCopy(bone.currentAngles,bone.maxAngles);
		}
		if (!bone.lastTimeUpdated)
		{
			static mdxaBone_t		id =
			{
				{
					{ 1.0f, 0.0f, 0.0f, 0.0f },
					{ 0.0f, 1.0f, 0.0f, 0.0f },
					{ 0.0f, 0.0f, 1.0f, 0.0f }
				}
			};
			memcpy(&bone.ragOverrideMatrix,&id, sizeof(mdxaBone_t));
			VectorClear(bone.anglesOffset);
			VectorClear(bone.positionOffset);
			VectorClear(bone.velocityEffector);  // this is actually a velocity now
			VectorClear(bone.velocityRoot);  // this is actually a velocity now
			VectorClear(bone.lastPosition);
			VectorClear(bone.lastShotDir);
			bone.lastContents=0;
			// if this is non-zero, we are in a dynamic state
			bone.firstCollisionTime=bone.ragStartTime;
			// if this is non-zero, we are in a settling state
			bone.restTime=0;
			// if they are both zero, we are in a settled state

			bone.firstTime=0;

			bone.RagFlags=flags;
			bone.DependentRagIndexMask=0;

			G2_Generate_MatrixRag(blist,index); // set everything to th id

			VectorClear(bone.currentAngles);
			VectorCopy(bone.currentAngles,bone.lastAngles);
		}
	}
	return index;
}

void G2_InitIK(CGhoul2Info_v &ghoul2V, sharedRagDollUpdateParams_t *parms, int time, const mdxaHeader_t *mod_a, int model)
{
	CGhoul2Info &ghoul2=ghoul2V[model];
	int curTime=time;
	boneInfo_v &blist = ghoul2.mBlist;

	G2_GenerateWorldMatrix(parms->angles, parms->position);
	G2_ConstructGhoulSkeleton(ghoul2V, curTime, false, parms->scale);

	// new base anim, unconscious flop
	int pcjFlags;
#if 0
	vec3_t pcjMin, pcjMax;
	VectorClear(pcjMin);
	VectorClear(pcjMax);

	pcjFlags=RAG_PCJ|RAG_PCJ_POST_MULT;//|RAG_EFFECTOR;

	G2_Set_Bone_Angles_IK(ghoul2, mod_a,blist,"model_root",RAG_PCJ_MODEL_ROOT|RAG_PCJ,10.0f,pcjMin,pcjMax,100);
	G2_Set_Bone_Angles_IK(ghoul2, mod_a,blist,"pelvis",RAG_PCJ_PELVIS|RAG_PCJ|RAG_PCJ_POST_MULT,10.0f,pcjMin,pcjMax,100);

	G2_Set_Bone_Angles_IK(ghoul2, mod_a,blist,"lower_lumbar",pcjFlags,10.0f,pcjMin,pcjMax,500);
	G2_Set_Bone_Angles_IK(ghoul2, mod_a,blist,"upper_lumbar",pcjFlags,10.0f,pcjMin,pcjMax,500);
	G2_Set_Bone_Angles_IK(ghoul2, mod_a,blist,"thoracic",pcjFlags|RAG_EFFECTOR,12.0f,pcjMin,pcjMax,500);
	G2_Set_Bone_Angles_IK(ghoul2, mod_a,blist,"cranium",pcjFlags,6.0f,pcjMin,pcjMax,500);
	G2_Set_Bone_Angles_IK(ghoul2, mod_a,blist,"rhumerus",pcjFlags,4.0f,pcjMin,pcjMax,500);
	G2_Set_Bone_Angles_IK(ghoul2, mod_a,blist,"lhumerus",pcjFlags,4.0f,pcjMin,pcjMax,500);
	G2_Set_Bone_Angles_IK(ghoul2, mod_a,blist,"rradius",pcjFlags,3.0f,pcjMin,pcjMax,500);
	G2_Set_Bone_Angles_IK(ghoul2, mod_a,blist,"lradius",pcjFlags,3.0f,pcjMin,pcjMax,500);
	G2_Set_Bone_Angles_IK(ghoul2, mod_a,blist,"rfemurYZ",pcjFlags,6.0f,pcjMin,pcjMax,500);
	G2_Set_Bone_Angles_IK(ghoul2, mod_a,blist,"lfemurYZ",pcjFlags,6.0f,pcjMin,pcjMax,500);
	G2_Set_Bone_Angles_IK(ghoul2, mod_a,blist,"rtibia",pcjFlags,4.0f,pcjMin,pcjMax,500);
	G2_Set_Bone_Angles_IK(ghoul2, mod_a,blist,"ltibia",pcjFlags,4.0f,pcjMin,pcjMax,500);

	G2_ConstructGhoulSkeleton(ghoul2V, curTime, false, parms->scale);
#endif
	//Only need the standard effectors for this.
	pcjFlags = RAG_PCJ|RAG_PCJ_POST_MULT|RAG_EFFECTOR;

	G2_Set_Bone_Angles_IK(ghoul2, mod_a,blist,"rhand",pcjFlags,6.0f);
	G2_Set_Bone_Angles_IK(ghoul2, mod_a,blist,"lhand",pcjFlags,6.0f);
//	G2_Set_Bone_Angles_IK(ghoul2, mod_a,blist,"rtarsal",pcjFlags,4.0f);
//	G2_Set_Bone_Angles_IK(ghoul2, mod_a,blist,"ltarsal",pcjFlags,4.0f);
	G2_Set_Bone_Angles_IK(ghoul2, mod_a,blist,"rtibia",pcjFlags,4.0f);
	G2_Set_Bone_Angles_IK(ghoul2, mod_a,blist,"ltibia",pcjFlags,4.0f);
	G2_Set_Bone_Angles_IK(ghoul2, mod_a,blist,"rtalus",pcjFlags,4.0f);
	G2_Set_Bone_Angles_IK(ghoul2, mod_a,blist,"ltalus",pcjFlags,4.0f);
	G2_Set_Bone_Angles_IK(ghoul2, mod_a,blist,"rradiusX",pcjFlags,6.0f);
	G2_Set_Bone_Angles_IK(ghoul2, mod_a,blist,"lradiusX",pcjFlags,6.0f);
	G2_Set_Bone_Angles_IK(ghoul2, mod_a,blist,"rfemurX",pcjFlags,10.0f);
	G2_Set_Bone_Angles_IK(ghoul2, mod_a,blist,"lfemurX",pcjFlags,10.0f);
	G2_Set_Bone_Angles_IK(ghoul2, mod_a,blist,"ceyebrow",pcjFlags,10.0f);
}

qboolean G2_SetBoneIKState(CGhoul2Info_v &ghoul2, int time, const char *boneName, int ikState, sharedSetBoneIKStateParams_t *params)
{
	model_t		*mod_a;
	int g2index = 0;
	int curTime = time;
	CGhoul2Info &g2 = ghoul2[g2index];
	const mdxaHeader_t *rmod_a = G2_GetModA(g2);

	boneInfo_v &blist = g2.mBlist;
	mod_a = (model_t *)g2.animModel;

	if (!boneName)
	{ //null bonename param means it's time to init the ik stuff on this instance
		sharedRagDollUpdateParams_t sRDUP;

		if (ikState == IKS_NONE)
		{ //this means we want to reset the IK state completely.. run through the bone list, and reset all the appropriate flags
			size_t i = 0;
			while (i < blist.size())
			{ //we can't use this method for ragdoll. However, since we expect them to set their anims/angles again on the PCJ
			  //limb after they reset it gameside, it's reasonable for IK bones.
				boneInfo_t &bone = blist[i];
				if (bone.boneNumber != -1)
				{
					bone.flags &= ~BONE_ANGLES_RAGDOLL;
					bone.flags &= ~BONE_ANGLES_IK;
					bone.RagFlags = 0;
					bone.lastTimeUpdated = 0;
				}
				i++;
			}
			return qtrue;
		}
		assert(params);

		if (!params)
		{
			return qfalse;
		}

		sRDUP.me = 0;
		VectorCopy(params->angles, sRDUP.angles);
		VectorCopy(params->origin, sRDUP.position);
		VectorCopy(params->scale, sRDUP.scale);
		VectorClear(sRDUP.velocity);
		G2_InitIK(ghoul2, &sRDUP, curTime, rmod_a, g2index);
		return qtrue;
	}

	if (!rmod_a || !mod_a)
	{
		return qfalse;
	}

	int index = G2_Find_Bone(mod_a, blist, boneName);

	if (index == -1)
	{
		index = G2_Add_Bone(mod_a, blist, boneName);
	}

	if (index == -1)
	{ //couldn't find or add the bone..
		return qfalse;
	}

	boneInfo_t &bone = blist[index];

	if (ikState == IKS_NONE)
	{ //remove the bone from the list then, so it has to reinit. I don't think this should hurt anything since
	  //we don't store bone index handles gameside anywhere.
		if (!(bone.flags & BONE_ANGLES_RAGDOLL))
		{ //you can't set the ik state to none if it's not a rag/ik bone.
			return qfalse;
		}
		//bone.flags = 0;
		//G2_Remove_Bone_Index(blist, index);
		//actually, I want to keep it on the rag list, and remove it as an IK bone instead.
		bone.flags &= ~BONE_ANGLES_RAGDOLL;
		bone.flags |= BONE_ANGLES_IK;
		bone.RagFlags &= ~RAG_PCJ_IK_CONTROLLED;
		return qtrue;
	}

	//need params if we're not resetting.
	if (!params)
	{
		assert(0);
		return qfalse;
	}

	if (bone.flags & BONE_ANGLES_RAGDOLL)
	{ //otherwise if the bone is already flagged as rag, then we can't set it again. (non-active ik bones will be BONE_ANGLES_IK, active are considered rag)
		return qfalse;
	}
#if 0 //this is wrong now.. we're only initing effectors with initik now.. which SHOULDN'T be used as pcj's
	if (!(bone.flags & BONE_ANGLES_IK) && !(bone.flags & BONE_ANGLES_RAGDOLL))
	{ //IK system has not been inited yet, because any bone that can be IK should be in the ragdoll list, not flagged as BONE_ANGLES_RAGDOLL but as BONE_ANGLES_IK
		sharedRagDollUpdateParams_t sRDUP;
		sRDUP.me = 0;
		VectorCopy(params->angles, sRDUP.angles);
		VectorCopy(params->origin, sRDUP.position);
		VectorCopy(params->scale, sRDUP.scale);
		VectorClear(sRDUP.velocity);
		G2_InitIK(ghoul2, &sRDUP, curTime, rmod_a, g2index);

		G2_ConstructGhoulSkeleton(ghoul2, curTime, false, params->scale);
	}
	else
	{
		G2_GenerateWorldMatrix(params->angles, params->origin);
		G2_ConstructGhoulSkeleton(ghoul2, curTime, false, params->scale);
	}
#else
	G2_GenerateWorldMatrix(params->angles, params->origin);
	G2_ConstructGhoulSkeleton(ghoul2, curTime, false, params->scale);
#endif

	int pcjFlags = RAG_PCJ|RAG_PCJ_IK_CONTROLLED|RAG_PCJ_POST_MULT|RAG_EFFECTOR;

	if (params->pcjOverrides)
	{
		pcjFlags = params->pcjOverrides;
	}

	bone.ikSpeed = 0.4f;
	VectorClear(bone.ikPosition);

	G2_Set_Bone_Rag(rmod_a, blist, boneName, g2, params->scale, params->origin);

	int startFrame = params->startFrame, endFrame = params->endFrame;

	if (bone.startFrame != startFrame || bone.endFrame != endFrame || params->forceAnimOnBone)
	{ //if it's already on this anim leave it alone, to allow smooth transitions into IK on the current anim if it is so desired.
		G2_Set_Bone_Anim_No_BS(g2, rmod_a, blist, boneName, startFrame, endFrame-1,
			BONE_ANIM_OVERRIDE_FREEZE|BONE_ANIM_BLEND,
			1.0f, curTime, float(startFrame), 150, 0, true);
	}

	G2_ConstructGhoulSkeleton(ghoul2, curTime, false, params->scale);

	bone.lastTimeUpdated = 0;
	G2_Set_Bone_Angles_Rag(g2, rmod_a, blist, boneName, pcjFlags, params->radius, params->pcjMins, params->pcjMaxs, params->blendTime);

	if (!G2_RagDollSetup(g2,curTime,true,params->origin,false))
	{
		assert(!"failed to add any rag bones");
		return qfalse;
	}

	return qtrue;
}

qboolean G2_IKMove(CGhoul2Info_v &ghoul2, int time, sharedIKMoveParams_t *params)
{
#if 0
	model_t		*mod_a;
	int g2index = 0;
	int curTime = time;
	CGhoul2Info &g2 = ghoul2[g2index];

	boneInfo_v &blist = g2.mBlist;
	mod_a = (model_t *)g2.animModel;

	if (!mod_a)
	{
		return qfalse;
	}

	int index = G2_Find_Bone(mod_a, blist, params->boneName);

	//don't add here if you can't find it.. ik bones should already be there, because they need to have special stuff done to them anyway.
	if (index == -1)
	{ //couldn't find the bone..
		return qfalse;
	}

	if (!params)
	{
		assert(0);
		return qfalse;
	}

	if (!(blist[index].flags & BONE_ANGLES_RAGDOLL) && !(blist[index].flags & BONE_ANGLES_IK))
	{ //no-can-do, buddy
		return qfalse;
	}

	VectorCopy(params->desiredOrigin, blist[index].ikPosition);
	blist[index].ikSpeed = params->movementSpeed;
#else
	int g2index = 0;
	int curTime = time;
	CGhoul2Info &g2 = ghoul2[g2index];

	//rwwFIXMEFIXME: Doing this on all bones at the moment, fix this later?
	if (!G2_RagDollSetup(g2,curTime,true,params->origin,false))
	{ //changed models, possibly.
		return qfalse;
	}

	for (int i=0;i<numRags;i++)
	{
		boneInfo_t &bone=*ragBoneData[i];

		//if (bone.boneNumber == blist[index].boneNumber)
		{
			VectorCopy(params->desiredOrigin, bone.ikPosition);
			bone.ikSpeed = params->movementSpeed;
		}
	}
#endif
	return qtrue;
}

// set the bone list to all unused so the bone transformation routine ignores it.
void G2_Init_Bone_List(boneInfo_v &blist, int numBones)
{
	blist.clear();
	blist.reserve(numBones);
}

void G2_RemoveRedundantBoneOverrides(boneInfo_v &blist, int *activeBones)
{
	// walk the surface list, removing surface overrides or generated surfaces that are pointing at surfaces that aren't active anymore
	for (size_t i=0; i<blist.size(); i++)
	{
		if (blist[i].boneNumber != -1)
		{
			if (!activeBones[blist[i].boneNumber])
			{
				blist[i].flags = 0;
				G2_Remove_Bone_Index(blist, i);
			}
		}
	}
}

int	G2_Get_Bone_Index(CGhoul2Info *ghoul2, const char *boneName)
{
  	model_t		*mod_m = R_GetModelByHandle(RE_RegisterModel(ghoul2->mFileName));
	model_t		*mod_a = R_GetModelByHandle(mod_m->mdxm->animIndex);

	return (G2_Find_Bone(mod_a, ghoul2->mBlist, boneName));
}
