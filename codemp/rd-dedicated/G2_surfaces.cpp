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

#include "rd-common/tr_types.h"
#include "ghoul2/G2.h"
#include "ghoul2/g2_local.h"
#include "tr_local.h"

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

extern void G2_ConstructUsedBoneList(CConstructBoneList &CBL);


//=====================================================================================================================
// Surface List handling routines - so entities can determine what surfaces attached to a model are operational or not.

// find a particular surface in the surface override list
surfaceInfo_t *G2_FindOverrideSurface(int surfaceNum, surfaceInfo_v &surfaceList)
{
	// look through entire list
	for(size_t i=0; i<surfaceList.size(); i++)
	{
		if (surfaceList[i].surface == surfaceNum)
		{
			return &surfaceList[i];
		}
	}
	// didn't find it.
	return NULL;
}

// given a surface name, lets see if it's legal in the model
int G2_IsSurfaceLegal(void *mod, const char *surfaceName, int *flags)
{
	// damn include file dependancies
	mdxmSurfHierarchy_t	*surf;
	model_t *mod_m = (model_t *)mod;
	surf = (mdxmSurfHierarchy_t *) ( (byte *)mod_m->mdxm + mod_m->mdxm->ofsSurfHierarchy );

	for ( int i = 0 ; i < mod_m->mdxm->numSurfaces ; i++)
	{
	 	if (!Q_stricmp(surfaceName, surf->name))
	 	{
			*flags = surf->flags;
			return i;
		}
		// find the next surface
  		surf = (mdxmSurfHierarchy_t *)( (byte *)surf + (size_t)( &((mdxmSurfHierarchy_t *)0)->childIndexes[ surf->numChildren ] ));
	}
	return -1;
}


/************************************************************************************************
 * G2_FindSurface
 *    find a surface in a ghoul2 surface override list based on it's name
 *
 * Input
 *    filename of model, surface list of model instance, name of surface, int to be filled in
 * with the index of this surface (defaults to NULL)
 *
 * Output
 *    pointer to surface if successful, false otherwise
 *
 ************************************************************************************************/
mdxmSurface_t *G2_FindSurface(CGhoul2Info *ghlInfo, surfaceInfo_v &slist, const char *surfaceName,
							 int *surfIndex/*NULL*/)
{
	int						i = 0;
	// find the model we want
	model_t				*mod = (model_t *)ghlInfo->currentModel;
	mdxmHierarchyOffsets_t *surfIndexes = (mdxmHierarchyOffsets_t *)((byte *)mod->mdxm + sizeof(mdxmHeader_t));
	mdxmSurfHierarchy_t	*surfInfo;

	// did we find a ghoul 2 model or not?
	if (!mod->mdxm)
	{
		assert(0);
		if (surfIndex)
		{
			*surfIndex = -1;
		}
		return 0;
	}

 	// first find if we already have this surface in the list
	for (i = slist.size() - 1; i >= 0; i--)
	{
		if ((slist[i].surface != 10000) && (slist[i].surface != -1))
		{
			mdxmSurface_t	*surf = (mdxmSurface_t *)G2_FindSurface((void *)mod, slist[i].surface, 0);
			// back track and get the surfinfo struct for this surface
			surfInfo = (mdxmSurfHierarchy_t *)((byte *)surfIndexes + surfIndexes->offsets[surf->thisSurfaceIndex]);

  			// are these the droids we're looking for?
			if (!Q_stricmp (surfInfo->name, surfaceName))
			{
				// yup
				if (surfIndex)
				{
					*surfIndex = i;
				}
				return surf;
			}
		}
	}
	// didn't find it
	if (surfIndex)
	{
		*surfIndex = -1;
	}
	return 0;
}

// set a named surface offFlags - if it doesn't find a surface with this name in the list then it will add one.
qboolean G2_SetSurfaceOnOff (CGhoul2Info *ghlInfo, surfaceInfo_v &slist, const char *surfaceName, const int offFlags)
{
	int					surfIndex = -1;
	surfaceInfo_t		temp_slist_entry;
	mdxmSurface_t		*surf;
	// find the model we want
	model_t				*mod = (model_t *)ghlInfo->currentModel;

	// did we find a ghoul 2 model or not?
	if (!mod->mdxm)
	{
		assert(0);
		return qfalse;
	}

 	// first find if we already have this surface in the list
	surf = G2_FindSurface(ghlInfo, slist, surfaceName, &surfIndex);
	if (surf)
	{
		// set descendants value

		// slist[surfIndex].offFlags = offFlags;
		// seems to me that we shouldn't overwrite the other flags.
		// the only bit we really care about in the incoming flags is the off bit
		slist[surfIndex].offFlags &= ~(G2SURFACEFLAG_OFF | G2SURFACEFLAG_NODESCENDANTS);
		slist[surfIndex].offFlags |= offFlags & (G2SURFACEFLAG_OFF | G2SURFACEFLAG_NODESCENDANTS);
		return qtrue;
	}
	else
	{
		// ok, not in the list already - in that case, lets verify this surface exists in the model mesh
		int	flags;
		int surfaceNum = G2_IsSurfaceLegal((void*)mod, surfaceName, &flags);
		if (surfaceNum != -1)
		{
			int newflags = flags;
			// the only bit we really care about in the incoming flags is the off bit
			newflags &= ~(G2SURFACEFLAG_OFF | G2SURFACEFLAG_NODESCENDANTS);
			newflags |= offFlags & (G2SURFACEFLAG_OFF | G2SURFACEFLAG_NODESCENDANTS);

			if (newflags != flags)
			{	// insert here then because it changed, no need to add an override otherwise
				temp_slist_entry.offFlags = newflags;
				temp_slist_entry.surface = surfaceNum;

				slist.push_back(temp_slist_entry);
			}
			return qtrue;
		}
	}
	return qfalse;
}

void G2_SetSurfaceOnOffFromSkin (CGhoul2Info *ghlInfo, qhandle_t renderSkin)
{
	int j;
	const skin_t *skin = R_GetSkinByHandle( renderSkin );

	ghlInfo->mSlist.clear();	//remove any overrides we had before.
	ghlInfo->mMeshFrameNum = 0;

	for ( j = 0 ; j < skin->numSurfaces ; j++ )
	{
		// the names have both been lowercased
		//FIXME: why is this using the shader name and not the surface name?
		if ( !strcmp( ((shader_t *)skin->surfaces[j]->shader)->name, "*off") ) {
			G2_SetSurfaceOnOff(ghlInfo, ghlInfo->mSlist, skin->surfaces[j]->name, G2SURFACEFLAG_OFF);
		}
		else
		{
			int	flags;
			int surfaceNum = G2_IsSurfaceLegal((void *)ghlInfo->currentModel, skin->surfaces[j]->name, &flags);
			if ( (surfaceNum != -1) && (!(flags&G2SURFACEFLAG_OFF)) )	//only turn on if it's not an "_off" surface
			{
				G2_SetSurfaceOnOff(ghlInfo, ghlInfo->mSlist, skin->surfaces[j]->name, 0);
			}
		}
	}
}

// return a named surfaces off flags - should tell you if this surface is on or off.
int G2_IsSurfaceOff (CGhoul2Info *ghlInfo, surfaceInfo_v &slist, const char *surfaceName)
{
	model_t				*mod = (model_t *)ghlInfo->currentModel;
	int					surfIndex = -1;
	mdxmSurface_t		*surf = 0;

	// did we find a ghoul 2 model or not?
	if (!mod->mdxm)
	{
		return 0;
	}

 	// first find if we already have this surface in the list
	surf = G2_FindSurface(ghlInfo, slist, surfaceName, &surfIndex);
	if (surf)
	{
		// set descendants value
		return slist[surfIndex].offFlags;
	}
	// ok, we didn't find it in the surface list. Lets look at the original surface then.

	mdxmSurfHierarchy_t	*surface = (mdxmSurfHierarchy_t *) ( (byte *)mod->mdxm + mod->mdxm->ofsSurfHierarchy );

	for ( int i = 0 ; i < mod->mdxm->numSurfaces ; i++)
	{
	 	if (!Q_stricmp(surfaceName, surface->name))
	 	{
			return surface->flags;
		}
		// find the next surface
  		surface = (mdxmSurfHierarchy_t *)( (byte *)surface + (intptr_t)( &((mdxmSurfHierarchy_t *)0)->childIndexes[ surface->numChildren ] ));
	}

	assert(0);
	return 0;
}

void G2_FindRecursiveSurface(model_t *currentModel, int surfaceNum, surfaceInfo_v &rootList, int *activeSurfaces)
{
	int						i;
 	mdxmSurface_t			*surface = (mdxmSurface_t *)G2_FindSurface((void *)currentModel, surfaceNum, 0);
	mdxmHierarchyOffsets_t	*surfIndexes = (mdxmHierarchyOffsets_t *)((byte *)currentModel->mdxm + sizeof(mdxmHeader_t));
	mdxmSurfHierarchy_t		*surfInfo = (mdxmSurfHierarchy_t *)((byte *)surfIndexes + surfIndexes->offsets[surface->thisSurfaceIndex]);

	// see if we have an override surface in the surface list
	surfaceInfo_t	*surfOverride = G2_FindOverrideSurface(surfaceNum, rootList);

	// really, we should use the default flags for this surface unless it's been overriden
	int offFlags = surfInfo->flags;

	// set the off flags if we have some
	if (surfOverride)
	{
		offFlags = surfOverride->offFlags;
	}

	// if this surface is not off, indicate as such in the active surface list
	if (!(offFlags & G2SURFACEFLAG_OFF))
	{
		activeSurfaces[surfaceNum] = 1;
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
		surfaceNum = surfInfo->childIndexes[i];
		G2_FindRecursiveSurface(currentModel, surfaceNum, rootList, activeSurfaces);
	}

}

void G2_RemoveRedundantGeneratedSurfaces(surfaceInfo_v &slist, int *activeSurfaces)
{
	// walk the surface list, removing surface overrides or generated surfaces that are pointing at surfaces that aren't active anymore
	for (size_t i=0; i<slist.size(); i++)
	{
		if (slist[i].surface != -1)
		{
			// is this a generated surface?
			if (slist[i].offFlags & G2SURFACEFLAG_GENERATED)
			{
				// if it's not in the list, remove it
				if (!activeSurfaces[slist[i].genPolySurfaceIndex & 0xffff])
				{
					G2_RemoveSurface(slist, i);
				}
			}
			// no, so it does point back at a legal surface
			else
			{
				// if it's not in the list, remove it
				if (!activeSurfaces[slist[i].surface])
				{
					G2_RemoveSurface(slist, i);
				}
			}
		}
	}
}

qboolean G2_SetRootSurface(CGhoul2Info_v &ghoul2, const int modelIndex, const char *surfaceName)
{
	int					surf;
	int					flags;
	int					*activeSurfaces, *activeBones;

	assert(ghoul2[modelIndex].currentModel && ghoul2[modelIndex].animModel);

	model_t				*mod_m = (model_t *)ghoul2[modelIndex].currentModel;
	model_t				*mod_a = (model_t *)ghoul2[modelIndex].animModel;

	// did we find a ghoul 2 model or not?
	if (!mod_m->mdxm)
	{
		return qfalse;
	}

	// first find if we already have this surface in the list
	surf = G2_IsSurfaceLegal(mod_m, surfaceName, &flags);
	if (surf != -1)
	{
		// first see if this ghoul2 model already has this as a root surface
		if (ghoul2[modelIndex].mSurfaceRoot == surf)
		{
			return qtrue;
		}

		// set the root surface
		ghoul2[modelIndex].mSurfaceRoot = surf;

		// ok, now the tricky bits.
		// firstly, generate a list of active / on surfaces below the root point

		// gimme some space to put this list into
		activeSurfaces = (int *)Z_Malloc(mod_m->mdxm->numSurfaces * 4, TAG_GHOUL2, qtrue);
		memset(activeSurfaces, 0, (mod_m->mdxm->numSurfaces * 4));
		activeBones = (int *)Z_Malloc(mod_a->mdxa->numBones * 4, TAG_GHOUL2, qtrue);
		memset(activeBones, 0, (mod_a->mdxa->numBones * 4));

		G2_FindRecursiveSurface(mod_m, surf, ghoul2[modelIndex].mSlist, activeSurfaces);

		// now generate the used bone list
		CConstructBoneList	CBL(ghoul2[modelIndex].mSurfaceRoot,
							activeBones,
							ghoul2[modelIndex].mSlist,
							mod_m,
							ghoul2[modelIndex].mBlist);

		G2_ConstructUsedBoneList(CBL);

		// now remove all procedural or override surfaces that refer to surfaces that arent on this list
		G2_RemoveRedundantGeneratedSurfaces(ghoul2[modelIndex].mSlist, activeSurfaces);

		// now remove all bones that are pointing at bones that aren't active
		G2_RemoveRedundantBoneOverrides(ghoul2[modelIndex].mBlist, activeBones);

		// then remove all bolts that point at surfaces or bones that *arent* active.
		G2_RemoveRedundantBolts(ghoul2[modelIndex].mBltlist, ghoul2[modelIndex].mSlist, activeSurfaces, activeBones);

		// then remove all models on this ghoul2 instance that use those bolts that are being removed.
		for (int i=0; i<ghoul2.size(); i++)
		{
			// are we even bolted to anything?
			if (ghoul2[i].mModelBoltLink != -1)
			{
				int	boltMod = (ghoul2[i].mModelBoltLink >> MODEL_SHIFT) & MODEL_AND;
				int	boltNum = (ghoul2[i].mModelBoltLink >> BOLT_SHIFT) & BOLT_AND;
				// if either the bolt list is too small, or the bolt we are pointing at references nothing, remove this model
				if (((int)ghoul2[boltMod].mBltlist.size() <= boltNum) ||
					((ghoul2[boltMod].mBltlist[boltNum].boneNumber == -1) &&
					 (ghoul2[boltMod].mBltlist[boltNum].surfaceNumber == -1)))
				{
					CGhoul2Info_v *g2i = &ghoul2;
					G2API_RemoveGhoul2Model((CGhoul2Info_v **)&g2i, i);
				}
			}
		}
		//No support for this, for now.

		// remember to free what we used
		Z_Free(activeSurfaces);
		Z_Free(activeBones);

		return (qtrue);
	}
/*
//g2r	if (entstate->ghoul2)
	{
		CGhoul2Info_v &ghoul2 = *((CGhoul2Info_v *)entstate->ghoul2);
		model_t				*mod_m = R_GetModelByHandle(RE_RegisterModel(ghoul2[modelIndex].mFileName));
		model_t				*mod_a = R_GetModelByHandle(mod_m->mdxm->animIndex);
		int					surf;
		int					flags;
		int					*activeSurfaces, *activeBones;

		// did we find a ghoul 2 model or not?
		if (!mod_m->mdxm)
		{
			return qfalse;
		}

 		// first find if we already have this surface in the list
		surf = G2_IsSurfaceLegal(mod_m, surfaceName, &flags);
		if (surf != -1)
		{
			// first see if this ghoul2 model already has this as a root surface
			if (ghoul2[modelIndex].mSurfaceRoot == surf)
			{
				return qtrue;
			}

			// set the root surface
			ghoul2[modelIndex].mSurfaceRoot = surf;

			// ok, now the tricky bits.
			// firstly, generate a list of active / on surfaces below the root point

			// gimme some space to put this list into
			activeSurfaces = (int *)Z_Malloc(mod_m->mdxm->numSurfaces * 4, TAG_GHOUL2, qtrue);
			memset(activeSurfaces, 0, (mod_m->mdxm->numSurfaces * 4));
			activeBones = (int *)Z_Malloc(mod_a->mdxa->numBones * 4, TAG_GHOUL2, qtrue);
			memset(activeBones, 0, (mod_a->mdxa->numBones * 4));

			G2_FindRecursiveSurface(mod_m, surf, ghoul2[modelIndex].mSlist, activeSurfaces);

			// now generate the used bone list
			CConstructBoneList	CBL(ghoul2[modelIndex].mSurfaceRoot,
								activeBones,
								ghoul2[modelIndex].mSlist,
								mod_m,
								ghoul2[modelIndex].mBlist);

			G2_ConstructUsedBoneList(CBL);

			// now remove all procedural or override surfaces that refer to surfaces that arent on this list
			G2_RemoveRedundantGeneratedSurfaces(ghoul2[modelIndex].mSlist, activeSurfaces);

			// now remove all bones that are pointing at bones that aren't active
			G2_RemoveRedundantBoneOverrides(ghoul2[modelIndex].mBlist, activeBones);

			// then remove all bolts that point at surfaces or bones that *arent* active.
			G2_RemoveRedundantBolts(ghoul2[modelIndex].mBltlist, ghoul2[modelIndex].mSlist, activeSurfaces, activeBones);

			// then remove all models on this ghoul2 instance that use those bolts that are being removed.
			for (int i=0; i<ghoul2.size(); i++)
			{
				// are we even bolted to anything?
				if (ghoul2[i].mModelBoltLink != -1)
				{
					int	boltMod = (ghoul2[i].mModelBoltLink >> MODEL_SHIFT) & MODEL_AND;
					int	boltNum = (ghoul2[i].mModelBoltLink >> BOLT_SHIFT) & BOLT_AND;
					// if either the bolt list is too small, or the bolt we are pointing at references nothing, remove this model
					if ((ghoul2[boltMod].mBltlist.size() <= boltNum) ||
						((ghoul2[boltMod].mBltlist[boltNum].boneNumber == -1) &&
						 (ghoul2[boltMod].mBltlist[boltNum].surfaceNumber == -1)))
					{
						G2API_RemoveGhoul2Model(entstate, i);
					}
				}
			}

			// remember to free what we used
			Z_Free(activeSurfaces);
			Z_Free(activeBones);

			return (qtrue);
		}
	}
	assert(0);*/
	return qfalse;
}


extern int G2_DecideTraceLod(CGhoul2Info &ghoul2, int useLod);
int G2_AddSurface(CGhoul2Info *ghoul2, int surfaceNumber, int polyNumber, float BarycentricI, float BarycentricJ, int lod )
{

	surfaceInfo_t temp_slist_entry;

	// decide if LOD is legal
	lod = G2_DecideTraceLod(*(CGhoul2Info *)(ghoul2), lod);

	// first up, see if we have a free one already set up  - look only from the end of the constant surfaces onwards
	for (size_t i=0; i<ghoul2->mSlist.size(); i++)
	{
		// is the surface count -1? That would indicate it's free
		if (ghoul2->mSlist[i].surface == -1)
		{
			ghoul2->mSlist[i].offFlags = G2SURFACEFLAG_GENERATED;
			ghoul2->mSlist[i].surface = 10000;		// no model will ever have 10000 surfaces
			ghoul2->mSlist[i].genBarycentricI = BarycentricI;
			ghoul2->mSlist[i].genBarycentricJ = BarycentricJ;
			ghoul2->mSlist[i].genPolySurfaceIndex = ((polyNumber & 0xffff) << 16) | (surfaceNumber & 0xffff);
			ghoul2->mSlist[i].genLod = lod;
			return i;
		}
	}

	// ok, didn't find one. Better create one

	temp_slist_entry.offFlags = G2SURFACEFLAG_GENERATED;
	temp_slist_entry.surface = 10000;
	temp_slist_entry.genBarycentricI = BarycentricI;
	temp_slist_entry.genBarycentricJ = BarycentricJ;
	temp_slist_entry.genPolySurfaceIndex = ((polyNumber & 0xffff) << 16) | (surfaceNumber & 0xffff);
	temp_slist_entry.genLod = lod;

	ghoul2->mSlist.push_back(temp_slist_entry);

	return (ghoul2->mSlist.size() -1 );
}

qboolean G2_RemoveSurface(surfaceInfo_v &slist, const int index)
{
		// did we find it?
	if (index != -1)
	{
		 // set us to be the 'not active' state
		slist[index].surface = -1;

		unsigned int newSize = slist.size();
		// now look through the list from the back and see if there is a block of -1's we can resize off the end of the list
		for (int i=slist.size()-1; i>-1; i--)
		{
			if (slist[i].surface == -1)
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
		if (newSize != slist.size())
		{
			// yes, so lets do it
			slist.resize(newSize);
		}

		return qtrue;
	}

	assert(0);

	// no
	return qfalse;
}


int G2_GetParentSurface(CGhoul2Info *ghlInfo, const int index)
{
	model_t					*mod = (model_t *)ghlInfo->currentModel;
	mdxmSurface_t			*surf = 0;
	mdxmHierarchyOffsets_t	*surfIndexes = (mdxmHierarchyOffsets_t *)((byte *)mod->mdxm + sizeof(mdxmHeader_t));
	mdxmSurfHierarchy_t		*surfInfo = 0;

	// walk each surface and see if this index is listed in it's children
	surf = (mdxmSurface_t *)G2_FindSurface((void *)mod, index, 0);
	surfInfo = (mdxmSurfHierarchy_t *)((byte *)surfIndexes + surfIndexes->offsets[surf->thisSurfaceIndex]);

	return surfInfo->parentIndex;

}

int G2_GetSurfaceIndex(CGhoul2Info *ghlInfo, const char *surfaceName)
{
	model_t		*mod = (model_t *)ghlInfo->currentModel;
	int			flags;

	return G2_IsSurfaceLegal(mod, surfaceName, &flags);
}

int G2_IsSurfaceRendered(CGhoul2Info *ghlInfo, const char *surfaceName, surfaceInfo_v &slist)
{
	int						flags = 0;//, surfFlags = 0;
	int						surfIndex = 0;
	assert(ghlInfo->currentModel);
	assert(ghlInfo->currentModel->mdxm);
	if (!ghlInfo->currentModel->mdxm)
	{
		return -1;
	}

	// now travel up the skeleton to see if any of it's ancestors have a 'no descendants' turned on

	// find the original surface in the surface list
	int surfNum = G2_IsSurfaceLegal((model_t *)ghlInfo->currentModel, surfaceName, &flags);
	if ( surfNum != -1 )
	{//must be legal
		const mdxmHierarchyOffsets_t	*surfIndexes = (mdxmHierarchyOffsets_t *)((byte *)ghlInfo->currentModel->mdxm + sizeof(mdxmHeader_t));
		const mdxmSurfHierarchy_t *surfInfo = (mdxmSurfHierarchy_t *)((byte *)surfIndexes + surfIndexes->offsets[surfNum]);
		surfNum = surfInfo->parentIndex;
		// walk the surface hierarchy up until we hit the root
		while (surfNum != -1)
		{
			const mdxmSurface_t		*parentSurf;
			int						parentFlags = 0;
			const mdxmSurfHierarchy_t		*parentSurfInfo;

			parentSurfInfo = (mdxmSurfHierarchy_t *)((byte *)surfIndexes + surfIndexes->offsets[surfNum]);

			// find the original surface in the surface list
			//G2 was bug, above comment was accurate, but we don't want the original flags, we want the parent flags
			G2_IsSurfaceLegal((model_t *)ghlInfo->currentModel, parentSurfInfo->name, &parentFlags);

			// now see if we already have overriden this surface in the slist
			parentSurf = G2_FindSurface(ghlInfo, slist, parentSurfInfo->name, &surfIndex);
			if (parentSurf)
			{
				// set descendants value
				parentFlags = slist[surfIndex].offFlags;
			}
			// now we have the parent flags, lets see if any have the 'no descendants' flag set
			if (parentFlags & G2SURFACEFLAG_NODESCENDANTS)
			{
				flags |= G2SURFACEFLAG_OFF;
				break;
			}
			// set up scan of next parent
			surfNum = parentSurfInfo->parentIndex;
		}
	}
	else
	{
		return -1;
	}
	if ( flags == 0 )
	{//it's not being overridden by a parent
		// now see if we already have overriden this surface in the slist
		const mdxmSurface_t *surf = G2_FindSurface(ghlInfo, slist, surfaceName, &surfIndex);
		if (surf)
		{
			// set descendants value
			flags = slist[surfIndex].offFlags;
		}
	  	// ok, at this point in flags we have what this surface is set to, and the index of the surface itself
	}
	return flags;
}
