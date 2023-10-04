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

#include "server/exe_headers.h"

#ifndef __Q_SHARED_H
	#include "qcommon/q_shared.h"
#endif

#if !defined(TR_LOCAL_H)
	#include "tr_local.h"
#endif

#if !defined(G2_H_INC)
	#include "ghoul2/G2.h"
#endif

#include "rd-common/tr_types.h"
#include "tr_local.h"
#ifdef _MSC_VER
#pragma warning(disable : 4512)		//assignment op could not be genereated
#endif

#define G2_MODEL_OK(g) ((g)&&(g)->mValid&&(g)->aHeader&&(g)->currentModel&&(g)->animModel)

class CConstructBoneList
{
public:
	int				surfaceNum;
	int*			boneUsedList;
	surfaceInfo_v&	rootSList;
	model_t*		currentModel;
	boneInfo_v&		boneList; 

	CConstructBoneList(
	int				initsurfaceNum,
	int*			initboneUsedList,
	surfaceInfo_v&	initrootSList,
	model_t*		initcurrentModel,
	boneInfo_v&		initboneList
	)
		: surfaceNum(initsurfaceNum)
		, boneUsedList(initboneUsedList)
		, rootSList(initrootSList)
		, currentModel(initcurrentModel)
		, boneList(initboneList)
	{ }
};

class CQuickOverride
{
	int mOverride[512];
	int mAt[512];
	int mCurrentTouch;
public:
	CQuickOverride()
	{
		mCurrentTouch = 1;		
		memset(mOverride, 0, sizeof(int) * 512);
	}
	void Invalidate()
	{
		mCurrentTouch++;
	}
	void Set(int index, int pos)
	{
		if (index == 10000)
		{
			return;
		}
		assert(index >= 0 && index < 512);
		mOverride[index] = mCurrentTouch;
		mAt[index] = pos;
	}
	int Test(int index)
	{
		assert(index >= 0 && index < 512);
		return (mOverride[index] == mCurrentTouch)
			? mAt[index]
			: -1;
	}
};

// functions preferinition
extern void G2_ConstructUsedBoneList(CConstructBoneList &CBL);
extern int G2_DecideTraceLod(CGhoul2Info& ghoul2, int useLod);

//=====================================================================================================================
// Surface List handling routines - so entities can determine what surfaces attached to a model are operational or not.

// find a particular surface in the surface override list
static CQuickOverride QuickOverride;


const surfaceInfo_t *G2_FindOverrideSurface(int surfaceNum, const surfaceInfo_v &surfaceList)
{
	if (surfaceNum < 0)
	{
		// starting a new lookup
		QuickOverride.Invalidate();
		for (size_t i = 0; i < surfaceList.size(); i++)
		{
			if (surfaceList[i].surface >= 0)
			{
				QuickOverride.Set(surfaceList[i].surface, i);
			}
		}
		return NULL;
	}
	int idx = QuickOverride.Test(surfaceNum);
	if (idx < 0)
	{
		if (surfaceNum == 10000)
		{
			for (size_t i = 0; i < surfaceList.size(); i++)
			{
				if (surfaceList[i].surface == surfaceNum)
				{
					return &surfaceList[i];
				}
			}
		}
#if _DEBUG
		// look through entire list
		size_t i;
		for (i = 0; i < surfaceList.size(); i++)
		{
			if (surfaceList[i].surface == surfaceNum)
			{
				break;
			}
		}
		// didn't find it.
		assert(i == surfaceList.size()); // our quickoverride is not working right
#endif
		return NULL;
	}
	assert(idx >= 0 && idx < (int)surfaceList.size());
	assert(surfaceList[idx].surface == surfaceNum);
	return &surfaceList[idx];
}


// given a surface name, lets see if it's legal in the model
int G2_IsSurfaceLegal(const model_s* mod_m, const char* surfaceName, uint32_t* flags)
{
	assert(mod_m);
	assert(mod_m->data.glm->header);
	// damn include file dependancies
	mdxmSurfHierarchy_t* surf;
	surf = (mdxmSurfHierarchy_t*)((byte*)mod_m->data.glm->header + mod_m->data.glm->header->ofsSurfHierarchy);

	for (int i = 0; i < mod_m->data.glm->header->numSurfaces; i++)
	{
		if (!Q_stricmp(surfaceName, surf->name))
		{
			*flags = surf->flags;
			return i;
		}
		// find the next surface
		surf = (mdxmSurfHierarchy_t*)((byte*)surf + (intptr_t)(&((mdxmSurfHierarchy_t*)0)->childIndexes[surf->numChildren]));
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
	mdxmHierarchyOffsets_t *surfIndexes = (mdxmHierarchyOffsets_t *)((byte *)mod->data.glm->header + sizeof(mdxmHeader_t));

	// did we find a ghoul 2 model or not?
	if (!mod->data.glm || !mod->data.glm->header)
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
			mdxmSurface_t	*surf = (mdxmSurface_t *)G2_FindSurface(mod, slist[i].surface, 0);
			// back track and get the surfinfo struct for this surface
			const mdxmSurfHierarchy_t* surfInfo = (mdxmSurfHierarchy_t *)((byte *)surfIndexes + surfIndexes->offsets[surf->thisSurfaceIndex]);

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

qboolean G2_SetSurfaceOnOff(CGhoul2Info* ghlInfo, surfaceInfo_v& slist, const char* surfaceName, const int offFlags)
{
	int					surfIndex = -1;
	surfaceInfo_t		temp_slist_entry;

	// find the model we want
	model_t				*mod = (model_t *)ghlInfo->currentModel;

	// did we find a ghoul 2 model or not?
	if (!mod->data.glm || !mod->data.glm->header)
	{
		assert(0);
		return qfalse;
	}
 
 	// first find if we already have this surface in the list
	const mdxmSurface_t* surf = G2_FindSurface(ghlInfo, slist, surfaceName, &surfIndex);
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
		uint32_t flags;
		int surfaceNum = G2_IsSurfaceLegal(mod, surfaceName, &flags);
		if (surfaceNum != -1)
		{
			uint32_t newflags = flags;
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

// set a named surface offFlags - if it doesn't find a surface with this name in the list then it will add one.
qboolean G2_SetSurfaceOnOff(CGhoul2Info* ghlInfo, const char* surfaceName, const int offFlags)
{
	return G2_SetSurfaceOnOff(ghlInfo, ghlInfo->mSlist, surfaceName, offFlags);
}

void G2_SetSurfaceOnOffFromSkin(CGhoul2Info *ghlInfo, qhandle_t renderSkin)
{
	const skin_t *skin = R_GetSkinByHandle( renderSkin );

	//FIXME:  using skin handles means we have to increase the numsurfs in a skin, but reading directly would cause file hits, we need another way to cache or just deal with the larger skin_t

	if (skin)
	{
		ghlInfo->mSlist.clear(); // remove any overrides we had before.
		ghlInfo->mMeshFrameNum = 0;

		for (int j = 0; j < skin->numSurfaces; j++)
		{
			uint32_t flags;
			int surfaceNum = G2_IsSurfaceLegal(ghlInfo->currentModel, skin->surfaces[j]->name, &flags);

			// the names have both been lowercased
			if (!(flags & G2SURFACEFLAG_OFF) && !strcmp(((shader_t*)(skin->surfaces[j]->shader))->name, "*off"))
			{
				G2_SetSurfaceOnOff(ghlInfo, skin->surfaces[j]->name, G2SURFACEFLAG_OFF);
			}
			else
			{
				// only turn on if it's not an "_off" surface
				if ((surfaceNum != -1) && (!(flags & G2SURFACEFLAG_OFF)))
				{
					//G2_SetSurfaceOnOff(ghlInfo, skin->surfaces[j]->name, 0);
				}
			}
		}
	}
}

// return a named surfaces off flags - should tell you if this surface is on or off.
/*
int G2_IsSurfaceOff (CGhoul2Info *ghlInfo, surfaceInfo_v &slist, const char *surfaceName)
{
	model_t				*mod = (model_t *)ghlInfo->currentModel;
	int					surfIndex = -1;
	mdxmSurface_t		*surf = 0;
	mdxmHeader_t *mdxm = mod->data.glm->header;

	// did we find a ghoul 2 model or not?
	if (!mdxm)
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

	mdxmSurfHierarchy_t	*surface = (mdxmSurfHierarchy_t *) ( (byte *)mdxm + mdxm->ofsSurfHierarchy );

	for ( int i = 0 ; i < mdxm->numSurfaces ; i++) 
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
*/

void G2_FindRecursiveSurface(const model_t* currentModel, int surfaceNum, surfaceInfo_v& rootList, int* activeSurfaces)
{
	assert(currentModel);
	assert(currentModel->data.glm);
	int						i;
 	mdxmSurface_t			*surface = (mdxmSurface_t *)G2_FindSurface(currentModel, surfaceNum, 0);
	mdxmHierarchyOffsets_t	*surfIndexes = (mdxmHierarchyOffsets_t *)((byte *)currentModel->data.glm->header + sizeof(mdxmHeader_t));
	mdxmSurfHierarchy_t		*surfInfo = (mdxmSurfHierarchy_t *)((byte *)surfIndexes + surfIndexes->offsets[surface->thisSurfaceIndex]);

	// see if we have an override surface in the surface list
	const surfaceInfo_t	*surfOverride = G2_FindOverrideSurface(surfaceNum, rootList);

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
	uint32_t			flags;

	assert(modelIndex >= 0 && modelIndex < ghoul2.size());
	assert(ghoul2[modelIndex].currentModel && ghoul2[modelIndex].animModel);

	model_t				*mod_m = (model_t *)ghoul2[modelIndex].currentModel;
	model_t				*mod_a = (model_t *)ghoul2[modelIndex].animModel;
	mdxmHeader_t *mdxm = mod_m->data.glm->header;
	mdxaHeader_t *mdxa = mod_a->data.gla;

	// did we find a ghoul 2 model or not?
	if (!mdxm)
	{
		return qfalse;
	}

	// first find if we already have this surface in the list
	surf = G2_IsSurfaceLegal(mod_m, surfaceName, &flags);
	if (surf != -1)
	{
		// set the root surface
		ghoul2[modelIndex].mSurfaceRoot = surf;

		return qtrue;
	}
	assert(0);
	return qfalse;
}


int G2_AddSurface(CGhoul2Info *ghoul2, int surfaceNumber, int polyNumber, float BarycentricI, float BarycentricJ, int lod )
{

	surfaceInfo_t temp_slist_entry;

	// decide if LOD is legal
	lod = G2_DecideTraceLod(*ghoul2, lod);

	// first up, see if we have a free one already set up  - look only from the end of the constant surfaces onwards
	size_t i;
	for (i=0; i<ghoul2->mSlist.size(); i++)
	{
		// is the surface count -1? That would indicate it's free
		if (ghoul2->mSlist[i].surface == -1)
		{
			break;
		}
	}

	if (i == ghoul2->mSlist.size())
	{
		ghoul2->mSlist.push_back(surfaceInfo_t());
	}

	ghoul2->mSlist[i].offFlags = G2SURFACEFLAG_GENERATED;
	ghoul2->mSlist[i].surface = 10000; // no model will ever have 10000 surfaces
	ghoul2->mSlist[i].genBarycentricI = BarycentricI;
	ghoul2->mSlist[i].genBarycentricJ = BarycentricJ;
	ghoul2->mSlist[i].genPolySurfaceIndex = ((polyNumber & 0xffff) << 16) | (surfaceNumber & 0xffff);
	ghoul2->mSlist[i].genLod = lod;

	return i;
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
	mdxmHierarchyOffsets_t	*surfIndexes = (mdxmHierarchyOffsets_t *)((byte *)mod->data.glm->header + sizeof(mdxmHeader_t));
	mdxmSurfHierarchy_t		*surfInfo = 0;

	// walk each surface and see if this index is listed in it's children
	surf = (mdxmSurface_t *)G2_FindSurface(mod, index, 0);
	surfInfo = (mdxmSurfHierarchy_t *)((byte *)surfIndexes + surfIndexes->offsets[surf->thisSurfaceIndex]);

	return surfInfo->parentIndex;

}

int G2_GetSurfaceIndex(CGhoul2Info *ghlInfo, const char *surfaceName)
{
	model_t		*mod = (model_t *)ghlInfo->currentModel;
	uint32_t	flags;
	
	return G2_IsSurfaceLegal(mod, surfaceName, &flags);
}

int G2_IsSurfaceRendered(CGhoul2Info *ghlInfo, const char *surfaceName, surfaceInfo_v &slist)
{
	uint32_t				flags = 0;//, surfFlags = 0;
	int						surfIndex = 0;
	assert(ghlInfo->currentModel);
	assert(ghlInfo->currentModel->data.glm && ghlInfo->currentModel->data.glm->header);
	if (!ghlInfo->currentModel->data.glm || !ghlInfo->currentModel->data.glm->header)
	{
		return -1;
	}

	// now travel up the skeleton to see if any of it's ancestors have a 'no descendants' turned on

	// find the original surface in the surface list
	int surfNum = G2_IsSurfaceLegal((model_t *)ghlInfo->currentModel, surfaceName, &flags);
	if ( surfNum != -1 )
	{//must be legal
		const mdxmHierarchyOffsets_t	*surfIndexes = (mdxmHierarchyOffsets_t *)((byte *)ghlInfo->currentModel->data.glm->header + sizeof(mdxmHeader_t));
		const mdxmSurfHierarchy_t *surfInfo = (mdxmSurfHierarchy_t *)((byte *)surfIndexes + surfIndexes->offsets[surfNum]);
		surfNum = surfInfo->parentIndex;
		// walk the surface hierarchy up until we hit the root
		while (surfNum != -1)
		{
			const mdxmSurface_t*		parentSurf;
			uint32_t					parentFlags;
			const mdxmSurfHierarchy_t*	parentSurfInfo;

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
