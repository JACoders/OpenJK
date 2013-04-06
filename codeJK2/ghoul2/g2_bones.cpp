// leave this as first line for PCH reasons...
//
#include "../server/exe_headers.h"



#ifndef __Q_SHARED_H
	#include "../game/q_shared.h"
#endif

#if !defined(TR_LOCAL_H)
	#include "../renderer/tr_local.h"
#endif
#if !defined(_QCOMMON_H_)
	#include "../qcommon/qcommon.h"
#endif

#include "../renderer/MatComp.h"

#if !defined(G2_H_INC)
	#include "G2.h"
#endif

extern	cvar_t	*r_Ghoul2BlendMultiplier;

void G2_Bone_Not_Found(const char *boneName,const char *modName);
	 
//=====================================================================================================================
// Bone List handling routines - so entities can override bone info on a bone by bone level, and also interrogate this info

// Given a bone name, see if that bone is already in our bone list - note the model_t pointer that gets passed in here MUST point at the 
// gla file, not the glm file type.
int G2_Find_Bone(CGhoul2Info *ghlInfo, boneInfo_v &blist, const char *boneName)
{
	int					i;
	mdxaSkel_t			*skel;
	mdxaSkelOffsets_t	*offsets;
   	offsets = (mdxaSkelOffsets_t *)((byte *)ghlInfo->aHeader + sizeof(mdxaHeader_t));
	skel = (mdxaSkel_t *)((byte *)ghlInfo->aHeader + sizeof(mdxaHeader_t) + offsets->offsets[0]);

	// look through entire list
	for(i=0; i<blist.size(); i++)
	{
		// if this bone entry has no info in it, bounce over it
		if (blist[i].boneNumber == -1)
		{
			continue;
		}

		// figure out what skeletal info structure this bone entry is looking at
		skel = (mdxaSkel_t *)((byte *)ghlInfo->aHeader + sizeof(mdxaHeader_t) + offsets->offsets[blist[i].boneNumber]);

		// if name is the same, we found it
		if (!stricmp(skel->name, boneName))
		{
			return i;
		}
	}
#if _DEBUG
	G2_Bone_Not_Found(boneName,ghlInfo->mFileName);
#endif	
	// didn't find it
	return -1;
}

#define DEBUG_G2_BONES (0)

// we need to add a bone to the list - find a free one and see if we can find a corresponding bone in the gla file
int G2_Add_Bone (const model_t *mod, boneInfo_v &blist, const char *boneName)
{
	int i, x;
	mdxaSkel_t			*skel;
	mdxaSkelOffsets_t	*offsets;
	boneInfo_t			tempBone;
	 
   	offsets = (mdxaSkelOffsets_t *)((byte *)mod->mdxa + sizeof(mdxaHeader_t));

 	// walk the entire list of bones in the gla file for this model and see if any match the name of the bone we want to find
 	for (x=0; x< mod->mdxa->numBones; x++)
 	{
 		skel = (mdxaSkel_t *)((byte *)mod->mdxa + sizeof(mdxaHeader_t) + offsets->offsets[x]);
 		// if name is the same, we found it
 		if (!stricmp(skel->name, boneName))
		{
			break;
		}
	}

	// check to see we did actually make a match with a bone in the model
	if (x == mod->mdxa->numBones)
	{
#if _DEBUG
		G2_Bone_Not_Found(boneName,mod->name);
#endif	
		return -1;
	}

	// look through entire list - see if it's already there first
	for(i=0; i<blist.size(); i++)
	{
		// if this bone entry has info in it, bounce over it
		if (blist[i].boneNumber != -1)
		{
			skel = (mdxaSkel_t *)((byte *)mod->mdxa + sizeof(mdxaHeader_t) + offsets->offsets[blist[i].boneNumber]);
			// if name is the same, we found it
			if (!stricmp(skel->name, boneName))
			{
#if DEBUG_G2_BONES
				{
					char mess[1000];
					sprintf(mess,"ADD BONE1 blistIndex=%3d    physicalIndex=%3d   %s\n",
						i,
						x,
						boneName);
					OutputDebugString(mess);
				}
#endif
				return i;
			}
		}
		else
		{
			// if we found an entry that had a -1 for the bonenumber, then we hit a bone slot that was empty
			blist[i].boneNumber = x;
			blist[i].flags = 0;
#if DEBUG_G2_BONES
			{
				char mess[1000];
				sprintf(mess,"ADD BONE1 blistIndex=%3d    physicalIndex=%3d   %s\n",
					i,
					x,
					boneName);
				OutputDebugString(mess);
			}
#endif
	 		return i;
		}
	}
	
	// ok, we didn't find an existing bone of that name, or an empty slot. Lets add an entry
	tempBone.boneNumber = x;
	tempBone.flags = 0;
	blist.push_back(tempBone);
#if DEBUG_G2_BONES
	{
		char mess[1000];
		sprintf(mess,"ADD BONE1 blistIndex=%3d    physicalIndex=%3d   %s\n",
			blist.size()-1,
			x,
			boneName);
		OutputDebugString(mess);
	}
#endif
	return blist.size()-1;
}


// Given a model handle, and a bone name, we want to remove this bone from the bone override list
qboolean G2_Remove_Bone_Index ( boneInfo_v &blist, int index)
{
	// did we find it?
	if (index != -1)
	{
		// check the flags first - if it's still being used Do NOT remove it
		if (!blist[index].flags)
		{
			// set this bone to not used
			blist[index].boneNumber = -1;
		}
		return qtrue;
	}
	return qfalse;
}

// given a bone number, see if there is an override bone in the bone list
int	G2_Find_Bone_In_List(boneInfo_v &blist, const int boneNum)
{
	int i; 
	
	// look through entire list
	for(i=0; i<blist.size(); i++)
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
	int			index = G2_Find_Bone(ghlInfo, blist, boneName);
	if (index==-1)
	{
		return false;
	}
 
	return G2_Remove_Bone_Index(blist, index);
}

#define DEBUG_PCJ (0)

// Given a model handle, and a bone name, we want to set angles specifically for overriding
qboolean G2_Set_Bone_Angles_Index(CGhoul2Info *ghlInfo, boneInfo_v &blist, const int index,
							const float *angles, const int flags, const Eorientations yaw,
							const Eorientations pitch, const Eorientations roll,
							const int blendTime, const int currentTime)
{
	
	if (index<0||(index >= blist.size()) || (blist[index].boneNumber == -1))
	{
		return qfalse;
	}
	
	// yes, so set the angles and flags correctly
	blist[index].flags &= ~(BONE_ANGLES_TOTAL);
	blist[index].flags |= flags;
	blist[index].boneBlendStart = currentTime;
	blist[index].boneBlendTime = blendTime;
#if DEBUG_PCJ
	OutputDebugString(va("%8x  %2d %6d   (%6.2f,%6.2f,%6.2f) %d %d %d %d\n",(int)ghlInfo,index,currentTime,angles[0],angles[1],angles[2],yaw,pitch,roll,flags));
#endif
	G2_Generate_Matrix(ghlInfo->animModel, blist, index, angles, flags, yaw, pitch, roll);
	return qtrue;

}

// Given a model handle, and a bone name, we want to set angles specifically for overriding
qboolean G2_Set_Bone_Angles(CGhoul2Info *ghlInfo, boneInfo_v &blist, const char *boneName, const float *angles,
							const int flags, const Eorientations up, const Eorientations left, const Eorientations forward,
							const int blendTime, const int currentTime)
{
	int			index = G2_Find_Bone(ghlInfo, blist, boneName);
	if (index == -1)
	{
		index = G2_Add_Bone(ghlInfo->animModel, blist, boneName);
	}
	if (index != -1)
	{
		blist[index].flags &= ~(BONE_ANGLES_TOTAL);
		blist[index].flags |= flags;
		blist[index].boneBlendStart = currentTime;
		blist[index].boneBlendTime = blendTime;

		G2_Generate_Matrix(ghlInfo->animModel, blist, index, angles, flags, up, left, forward);
		return qtrue;
	}
	return qfalse;
}

// Given a model handle, and a bone name, we want to set angles specifically for overriding - using a matrix directly
qboolean G2_Set_Bone_Angles_Matrix_Index(boneInfo_v &blist, const int index,
								   const mdxaBone_t &matrix, const int flags,
								   const int blendTime, const int currentTime)
{

	if (index<0||(index >= blist.size()) || (blist[index].boneNumber == -1))
	{
		// we are attempting to set a bone override that doesn't exist
		assert(0);
		return qfalse;
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
qboolean G2_Set_Bone_Angles_Matrix(CGhoul2Info *ghlInfo, boneInfo_v &blist, const char *boneName, const mdxaBone_t &matrix,
								   const int flags,const int blendTime, const int currentTime)
{
	int			index = G2_Find_Bone(ghlInfo, blist, boneName);
 	if (index == -1)
	{
		index = G2_Add_Bone(ghlInfo->animModel, blist, boneName);
	}
	if (index != -1)
	{
		blist[index].flags &= ~(BONE_ANGLES_TOTAL);
		blist[index].flags |= flags;
		memcpy(&blist[index].matrix, &matrix, sizeof(mdxaBone_t));
		memcpy(&blist[index].newMatrix, &matrix, sizeof(mdxaBone_t));
		return qtrue;
	}
	return qfalse;
}

#define DEBUG_G2_TIMING (0)

// given a model, bone name, a bonelist, a start/end frame number, a anim speed and some anim flags, set up or modify an existing bone entry for a new set of anims
qboolean G2_Set_Bone_Anim_Index(boneInfo_v &blist, const int index, const int startFrame, 
						  const int endFrame, const int flags, const float animSpeed, const int currentTime, const float setFrame, const int AblendTime,int numFrames)
{
	int			modFlags = flags;
	int			blendTime=AblendTime;

	if (r_Ghoul2BlendMultiplier)
	{
		if (r_Ghoul2BlendMultiplier->value!=1.0f)
		{
			if (r_Ghoul2BlendMultiplier->value<=0.0f)
			{
				modFlags&=~BONE_ANIM_BLEND;
			}
			else
			{
				blendTime=ceil(float(AblendTime)*r_Ghoul2BlendMultiplier->value);
			}
		}
	}
 

	if (index<0||index >= blist.size()||blist[index].boneNumber<0)
	{
		return qfalse;
	}

	// sanity check to see if setfram is within animation bounds
	if (setFrame != -1)
	{
		assert((setFrame >= startFrame) && (setFrame < endFrame));
	}


	// since we already existed, we can check to see if we want to start some blending
	if (modFlags & BONE_ANIM_BLEND)
	{
		float	currentFrame, animSpeed; 
		int 	startFrame, endFrame, flags;
		// figure out where we are now
		if (G2_Get_Bone_Anim_Index(blist, index, currentTime, &currentFrame, &startFrame, &endFrame, &flags, &animSpeed,numFrames))
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
					if (blist[index].blendFrame >= blist[index].endFrame )
					{
						// we only want to lerp with the first frame of the anim if we are looping 
						if (blist[index].flags & BONE_ANIM_OVERRIDE_LOOP)
						{
							blist[index].blendFrame = blist[index].startFrame;
						}
						// if we intend to end this anim or freeze after this, then just keep on the last frame
						else
						{
							assert(blist[index].endFrame>0);
							blist[index].blendFrame = blist[index].endFrame -1;
						}
					}
					
					// cope with if the lerp frame is actually off the end of the anim
					if (blist[index].blendLerpFrame >= blist[index].endFrame )
					{
						// we only want to lerp with the first frame of the anim if we are looping 
						if (blist[index].flags & BONE_ANIM_OVERRIDE_LOOP)
						{
							blist[index].blendLerpFrame = blist[index].startFrame;
						}
						// if we intend to end this anim or freeze after this, then just keep on the last frame
						else
						{
							assert(blist[index].endFrame>0);
							blist[index].blendLerpFrame = blist[index].endFrame - 1;
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
			// we aren't blending, so remove the option to do so
			modFlags &= ~BONE_ANIM_BLEND;
			//return qfalse;
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
	assert(blist[index].blendFrame>=0&&blist[index].blendFrame<numFrames);
	assert(blist[index].blendLerpFrame>=0&&blist[index].blendLerpFrame<numFrames);
	// start up the animation:)
	if (setFrame != -1)
	{
		blist[index].startTime = (currentTime - (((setFrame - (float)startFrame) * 50.0)/ animSpeed));
/*
		setFrame = bone.startFrame + ((currentTime - bone.startTime) / 50.0f * animSpeed);

		(setFrame - bone.startFrame) = ((currentTime - bone.startTime) / 50.0f * animSpeed);

		(setFrame - bone.startFrame)*50/animSpeed - currentTime = - bone.startTime;

		currentTime - (setFrame - bone.startFrame)*50/animSpeed  =  bone.startTime;
*/
	}
	else
	{
		blist[index].startTime = currentTime;
	}
	blist[index].flags &= ~(BONE_ANIM_TOTAL);
	blist[index].flags |= modFlags;

#if DEBUG_G2_TIMING
	if (index>-10) 
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
		OutputDebugString(mess);
	}
#endif
//		assert(blist[index].startTime <= currentTime);
	return qtrue;

}

// given a model, bone name, a bonelist, a start/end frame number, a anim speed and some anim flags, set up or modify an existing bone entry for a new set of anims
qboolean G2_Set_Bone_Anim(CGhoul2Info *ghlInfo, boneInfo_v &blist, const char *boneName, const int startFrame, 
						  const int endFrame, const int flags, const float animSpeed, const int currentTime, const float setFrame, const int blendTime)
{
	int			modFlags = flags;
	int			index = G2_Find_Bone(ghlInfo, blist, boneName);
 
	// sanity check to see if setfram is within animation bounds
	if (setFrame != -1)
	{
		assert((setFrame >= startFrame) && (setFrame <= endFrame));
	}

	// did we find it?
	if (index != -1)
	{
		return G2_Set_Bone_Anim_Index(blist, index, startFrame, endFrame, flags, animSpeed, currentTime, setFrame, blendTime,ghlInfo->aHeader->numFrames);
	}
	// no - lets try and add this bone in
	index = G2_Add_Bone(ghlInfo->animModel, blist, boneName);
	
	// did we find a free one?
	if (index != -1)
	{	
		blist[index].blendFrame = blist[index].blendLerpFrame = 0;
		blist[index].blendTime = 0;
		// we aren't blending, so remove the option to do so
		modFlags &= ~BONE_ANIM_BLEND;
		// yes, so set the anim data and flags correctly
		blist[index].endFrame = endFrame;
		blist[index].startFrame = startFrame;
		blist[index].animSpeed = animSpeed;
		blist[index].pauseTime = 0;
		// start up the animation:)
		if (setFrame != -1)
		{
			blist[index].startTime = (currentTime - (((setFrame - (float)startFrame) * 50.0)/ animSpeed));
		}
		else
		{
			blist[index].startTime = currentTime;
		}
		blist[index].flags &= ~BONE_ANIM_TOTAL;
		blist[index].flags |= modFlags;
		assert(blist[index].blendFrame>=0&&blist[index].blendFrame<ghlInfo->aHeader->numFrames);
		assert(blist[index].blendLerpFrame>=0&&blist[index].blendLerpFrame<ghlInfo->aHeader->numFrames);
#if DEBUG_G2_TIMING
		if (index>-10) 
		{
			const boneInfo_t &bone=blist[index];
			char mess[1000];
			if (bone.flags&BONE_ANIM_BLEND)
			{
				sprintf(mess,"s2b[%2d] %5d  %5d  (%5d-%5d) %4.2f %4x   bt(%5d-%5d) %7.2f %5d\n",
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
				sprintf(mess,"s2a[%2d] %5d  %5d  (%5d-%5d) %4.2f %4x\n",
					index,
					currentTime,
					bone.startTime,
					bone.startFrame,
					bone.endFrame,
					bone.animSpeed,
					bone.flags
					);
			}
			OutputDebugString(mess);
		}
#endif
		return qtrue;
	}

	//assert(index != -1);
	// no
	return qfalse;
}

qboolean G2_Get_Bone_Anim_Range_Index(boneInfo_v &blist, const int boneIndex, int *startFrame, int *endFrame)
{
	if (boneIndex != -1)
	{ 
		assert(boneIndex>=0&&boneIndex<blist.size());
		// are we an animating bone?
		if (blist[boneIndex].flags & (BONE_ANIM_OVERRIDE_LOOP | BONE_ANIM_OVERRIDE))
		{
			*startFrame = blist[boneIndex].startFrame;
			*endFrame = blist[boneIndex].endFrame;
			return qtrue;
		}
	}
	return qfalse;
}
qboolean G2_Get_Bone_Anim_Range(CGhoul2Info *ghlInfo, boneInfo_v &blist, const char *boneName, int *startFrame, int *endFrame)
{
	int			index = G2_Find_Bone(ghlInfo, blist, boneName);
	if (index==-1)
	{
		return qfalse;
	}	
	if (G2_Get_Bone_Anim_Range_Index( blist, index, startFrame, endFrame))
	{
		assert(*startFrame>=0&&*startFrame<ghlInfo->aHeader->numFrames);
		assert(*endFrame>0&&*endFrame<=ghlInfo->aHeader->numFrames);
		return qtrue;
	}
	return qfalse;
}

// given a model, bonelist and bonename, return the current frame, startframe and endframe of the current animation
// NOTE if we aren't running an animation, then qfalse is returned
qboolean G2_Get_Bone_Anim_Index( boneInfo_v &blist, const int index, const int currentTime, 
						  float *retcurrentFrame, int *startFrame, int *endFrame, int *flags, float *retAnimSpeed,int numFrames)
{
  
	// did we find it?
	if ((index>=0) && !((index >= blist.size()) || (blist[index].boneNumber == -1)))
	{ 

		// are we an animating bone?
		if (blist[index].flags & (BONE_ANIM_OVERRIDE_LOOP | BONE_ANIM_OVERRIDE))
		{
			int currentFrame,newFrame;
			float lerp;
			G2_TimingModel(blist[index],currentTime,numFrames,currentFrame,newFrame,lerp);

			*retcurrentFrame =float(currentFrame)+lerp;
			*startFrame = blist[index].startFrame;
			*endFrame = blist[index].endFrame;
			*flags = blist[index].flags;
			*retAnimSpeed = blist[index].animSpeed;
			return qtrue;
		}
	}
	*startFrame=0;
	*endFrame=1;
	*retcurrentFrame=0.0f;
	*flags=0;
	*retAnimSpeed=0.0f;
	return qfalse;
}

// given a model, bonelist and bonename, return the current frame, startframe and endframe of the current animation
// NOTE if we aren't running an animation, then qfalse is returned
qboolean G2_Get_Bone_Anim(CGhoul2Info *ghlInfo, boneInfo_v &blist, const char *boneName, const int currentTime, 
						  float *currentFrame, int *startFrame, int *endFrame, int *flags, float *retAnimSpeed)
{
	int			index = G2_Find_Bone(ghlInfo, blist, boneName);
	if (index==-1)
	{
		return qfalse;
	}
 
	assert(ghlInfo->aHeader);
	if (G2_Get_Bone_Anim_Index(blist, index, currentTime, currentFrame, startFrame, endFrame, flags, retAnimSpeed,ghlInfo->aHeader->numFrames))
	{
		assert(*startFrame>=0&&*startFrame<ghlInfo->aHeader->numFrames);
		assert(*endFrame>0&&*endFrame<=ghlInfo->aHeader->numFrames);
		assert(*currentFrame>=0.0f&&((int)(*currentFrame))<ghlInfo->aHeader->numFrames);
		return qtrue;
	}
	return qfalse;
}


// given a model, bonelist and bonename, lets pause an anim if it's playing.
qboolean G2_Pause_Bone_Anim_Index( boneInfo_v &blist, const int boneIndex, const int currentTime,int numFrames)
{
	if (boneIndex>=0&&boneIndex<blist.size())
	{
		// are we pausing or un pausing?
		if (blist[boneIndex].pauseTime)
		{
			int		startFrame, endFrame, flags;
			float	currentFrame, animSpeed;

			// figure out what frame we are on now
			if	(G2_Get_Bone_Anim_Index( blist, boneIndex, blist[boneIndex].pauseTime, &currentFrame, &startFrame, &endFrame, &flags, &animSpeed,numFrames))
			{
				// reset start time so we are actually on this frame right now
				G2_Set_Bone_Anim_Index( blist, boneIndex, startFrame, endFrame, flags, animSpeed, currentTime, currentFrame, 0,numFrames);
				// no pausing anymore
				blist[boneIndex].pauseTime = 0;
			}
			else
			{
				return qfalse;
			}
		}
		// ahh, just pausing, the easy bit
		else
		{
			blist[boneIndex].pauseTime = currentTime;
		}
		
		return qtrue;
	}
	assert(0);
	return qfalse;
}
qboolean G2_Pause_Bone_Anim(CGhoul2Info *ghlInfo, boneInfo_v &blist, const char *boneName, const int currentTime)
{
	int			index = G2_Find_Bone(ghlInfo, blist, boneName);
	if (index==-1)
	{
		return qfalse;
	}
	
	return (G2_Pause_Bone_Anim_Index( blist, index, currentTime,ghlInfo->aHeader->numFrames) );
}

qboolean	G2_IsPaused(CGhoul2Info *ghlInfo, boneInfo_v &blist, const char *boneName)
{
	int			index = G2_Find_Bone(ghlInfo, blist, boneName);
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
 
	if (index<0 || (index >= blist.size()) || (blist[index].boneNumber == -1))
	{
		// we are attempting to set a bone override that doesn't exist
		return qfalse;
	}

	blist[index].flags &= ~(BONE_ANIM_TOTAL);
	return G2_Remove_Bone_Index(blist, index);
}

// given a model, bonelist and bonename, lets stop an anim if it's playing.
qboolean G2_Stop_Bone_Anim(CGhoul2Info *ghlInfo, boneInfo_v &blist, const char *boneName)
{
	int			index = G2_Find_Bone(ghlInfo, blist, boneName);
	if (index != -1)
	{
		blist[index].flags &= ~(BONE_ANIM_TOTAL);
		return G2_Remove_Bone_Index(blist, index);
	}
	assert(0);
	return qfalse;
}

// given a model, bonelist and bonename, lets stop an anim if it's playing.
qboolean G2_Stop_Bone_Angles_Index(boneInfo_v &blist, const int index)
{
 
	if ((index >= blist.size()) || (blist[index].boneNumber == -1))
	{
		// we are attempting to set a bone override that doesn't exist
		assert(0);
		return qfalse;
	} 
	blist[index].flags &= ~(BONE_ANGLES_TOTAL);
	return G2_Remove_Bone_Index(blist, index);

}

// given a model, bonelist and bonename, lets stop an anim if it's playing.
qboolean G2_Stop_Bone_Angles(CGhoul2Info *ghlInfo, boneInfo_v &blist, const char *boneName)
{
	int			index = G2_Find_Bone(ghlInfo, blist, boneName);
	if (index != -1)
	{
		blist[index].flags &= ~(BONE_ANGLES_TOTAL);
		return G2_Remove_Bone_Index(blist, index);
	}
	assert(0);
	return qfalse;
}

// set the bone list to all unused so the bone transformation routine ignores it.
void G2_Init_Bone_List(boneInfo_v &blist)
{
	blist.clear();
}

int	G2_Get_Bone_Index(CGhoul2Info *ghoul2, const char *boneName, qboolean bAddIfNotFound)
{
	if (bAddIfNotFound)
	{
		return G2_Add_Bone(ghoul2->animModel, ghoul2->mBlist, boneName);
	}
	else
	{
		return G2_Find_Bone(ghoul2, ghoul2->mBlist, boneName);
	}
}