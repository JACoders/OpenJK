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

#include "ghoul2/G2.h"
#include "ghoul2/g2_local.h"
#include "ghoul2/G2_gore.h"

#include "qcommon/MiniHeap.h"
#include "tr_local.h"

#include <set>
#include <list>

#ifdef _FULL_G2_LEAK_CHECKING
int g_Ghoul2Allocations = 0;
int g_G2ServerAlloc = 0;
int g_G2ClientAlloc = 0;
int g_G2AllocServer = 0;

//stupid debug crap to track leaks in case they happen.
//we used to shove everything into a map and delete it all and not care about
//leaks, but that was not the Right Thing. -rww
#define MAX_TRACKED_ALLOC					4096
static bool g_G2AllocTrackInit = false; //want to keep this thing contained
static CGhoul2Info_v *g_G2AllocTrack[MAX_TRACKED_ALLOC];

void G2_DEBUG_InitPtrTracker(void)
{
	memset(g_G2AllocTrack, 0, sizeof(g_G2AllocTrack));
	g_G2AllocTrackInit = true;
}

void G2_DEBUG_ReportLeaks(void)
{
	int i = 0;

	if (!g_G2AllocTrackInit)
	{
		ri.Printf( PRINT_ALL, "g2 leak tracker was never initialized!\n");
		return;
	}

    while (i < MAX_TRACKED_ALLOC)
	{
		if (g_G2AllocTrack[i])
		{
			ri.Printf( PRINT_ALL, "Bad guy found in slot %i, attempting to access...", i);
			CGhoul2Info_v &g2v = *g_G2AllocTrack[i];
			CGhoul2Info &g2 = g2v[0];

			if (g2v.IsValid() && g2.mFileName && g2.mFileName[0])
			{
				ri.Printf( PRINT_ALL, "Bad guy's filename is %s\n", g2.mFileName);
			}
			else
			{
				ri.Printf( PRINT_ALL, "He's not valid! BURN HIM!\n");
			}
		}
		i++;
	}
}

void G2_DEBUG_ShovePtrInTracker(CGhoul2Info_v *g2)
{
	int i = 0;

	if (!g_G2AllocTrackInit)
	{
		G2_DEBUG_InitPtrTracker();
	}

	if (!g_G2AllocTrackInit)
	{
		G2_DEBUG_InitPtrTracker();
	}

	while (i < MAX_TRACKED_ALLOC)
	{
		if (!g_G2AllocTrack[i])
		{
			g_G2AllocTrack[i] = g2;
			return;
		}
		i++;
	}

	CGhoul2Info_v &g2v = *g2;

	if (g2v[0].currentModel && g2v[0].currentModel->name && g2v[0].currentModel->name[0])
	{
		ri.Printf( PRINT_ALL, "%s could not be fit into g2 debug instance tracker.\n", g2v[0].currentModel->name);
	}
	else
	{
		ri.Printf( PRINT_ALL, "Crap g2 instance passed to instance tracker (in).\n");
	}
}

void G2_DEBUG_RemovePtrFromTracker(CGhoul2Info_v *g2)
{
	int i = 0;

	if (!g_G2AllocTrackInit)
	{
		G2_DEBUG_InitPtrTracker();
	}

	while (i < MAX_TRACKED_ALLOC)
	{
		if (g_G2AllocTrack[i] == g2)
		{
			g_G2AllocTrack[i] = NULL;
			return;
		}
		i++;
	}

	CGhoul2Info_v &g2v = *g2;

	if (g2v[0].currentModel && g2v[0].currentModel->name && g2v[0].currentModel->name[0])
	{
		ri.Printf( PRINT_ALL, "%s not in g2 debug instance tracker.\n", g2v[0].currentModel->name);
	}
	else
	{
		ri.Printf( PRINT_ALL, "Crap g2 instance passed to instance tracker (out).\n");
	}
}
#endif

qboolean G2_SetupModelPointers(CGhoul2Info *ghlInfo);
qboolean G2_SetupModelPointers(CGhoul2Info_v &ghoul2);
qboolean G2_TestModelPointers(CGhoul2Info *ghlInfo);

//rww - RAGDOLL_BEGIN
#define NUM_G2T_TIME (2)
static int G2TimeBases[NUM_G2T_TIME];

void G2API_SetTime(int currentTime,int clock)
{
	assert(clock>=0&&clock<NUM_G2T_TIME);
#if G2_DEBUG_TIME
	ri.Printf( PRINT_ALL, "Set Time: before c%6d  s%6d",G2TimeBases[1],G2TimeBases[0]);
#endif
	G2TimeBases[clock]=currentTime;
	if (G2TimeBases[1]>G2TimeBases[0]+200)
	{
		G2TimeBases[1]=0; // use server time instead
		return;
	}
#if G2_DEBUG_TIME
	ri.Printf( PRINT_ALL, " after c%6d  s%6d\n",G2TimeBases[1],G2TimeBases[0]);
#endif
}

int	G2API_GetTime(int argTime) // this may or may not return arg depending on ghoul2_time cvar
{
	int ret=G2TimeBases[1];
	if ( !ret )
	{
		ret = G2TimeBases[0];
	}

	return ret;
}
//rww - RAGDOLL_END

//rww - Stuff to allow association of ghoul2 instances to entity numbers.
//This way, on listen servers when both the client and server are doing
//ghoul2 operations, we can copy relevant data off the client instance
//directly onto the server instance and slash the transforms and whatnot
//right in half.
#ifdef _G2_LISTEN_SERVER_OPT
CGhoul2Info_v *g2ClientAttachments[MAX_GENTITIES];
#endif

void G2API_AttachInstanceToEntNum(CGhoul2Info_v &ghoul2, int entityNum, qboolean server)
{ //Assign the pointers in the arrays
#ifdef _G2_LISTEN_SERVER_OPT
	if (server)
	{
		ghoul2[0].entityNum = entityNum;
	}
	else
	{
		g2ClientAttachments[entityNum] = &ghoul2;
	}
#endif
}

void G2API_ClearAttachedInstance(int entityNum)
{
#ifdef _G2_LISTEN_SERVER_OPT
	g2ClientAttachments[entityNum] = NULL;
#endif
}

void G2API_CleanEntAttachments(void)
{
#ifdef _G2_LISTEN_SERVER_OPT
	int i = 0;

	while (i < MAX_GENTITIES)
	{
		g2ClientAttachments[i] = NULL;
		i++;
	}
#endif
}

#ifdef _G2_LISTEN_SERVER_OPT
void CopyBoneCache(CBoneCache *to, CBoneCache *from);
#endif

qboolean G2API_OverrideServerWithClientData(CGhoul2Info_v& ghoul2, int modelIndex)
{
#ifndef _G2_LISTEN_SERVER_OPT
	return qfalse;
#else
	CGhoul2Info *serverInstance = &ghoul2[modelIndex];
	CGhoul2Info *clientInstance;

	if (ri.Cvar_VariableIntegerValue( "dedicated" ))
	{ //No client to get from!
		return qfalse;
	}

	if (!g2ClientAttachments[serverInstance->entityNum])
	{ //No clientside instance is attached to this entity
		return qfalse;
	}

	CGhoul2Info_v &g2Ref = *g2ClientAttachments[serverInstance->entityNum];
	clientInstance = &g2Ref[0];

	int frameNum = G2API_GetTime(0);

	if (clientInstance->mSkelFrameNum != frameNum)
	{ //it has to be constructed already
		return qfalse;
	}

	if (!clientInstance->mBoneCache)
	{ //that just won't do
		return qfalse;
	}

	//Just copy over the essentials
	serverInstance->aHeader = clientInstance->aHeader;
	serverInstance->animModel = clientInstance->animModel;
	serverInstance->currentAnimModelSize = clientInstance->currentAnimModelSize;
	serverInstance->currentModel = clientInstance->currentModel;
	serverInstance->currentModelSize = clientInstance->currentModelSize;
	serverInstance->mAnimFrameDefault = clientInstance->mAnimFrameDefault;
	serverInstance->mModel = clientInstance->mModel;
	serverInstance->mModelindex = clientInstance->mModelindex;
	serverInstance->mSurfaceRoot = clientInstance->mSurfaceRoot;
	serverInstance->mTransformedVertsArray = clientInstance->mTransformedVertsArray;

	if (!serverInstance->mBoneCache)
	{ //if this is the case.. I guess we can use the client one instead
		serverInstance->mBoneCache = clientInstance->mBoneCache;
	}

	//Copy the contents of the client cache over the contents of the server cache
	if (serverInstance->mBoneCache != clientInstance->mBoneCache)
	{
		CopyBoneCache(serverInstance->mBoneCache, clientInstance->mBoneCache);
	}

	serverInstance->mSkelFrameNum = clientInstance->mSkelFrameNum;
	return qtrue;
#endif
}

// must be a power of two
#define MAX_G2_MODELS (1024)
#define G2_MODEL_BITS (10)

#define G2_INDEX_MASK (MAX_G2_MODELS-1)

class Ghoul2InfoArray : public IGhoul2InfoArray
{
	std::vector<CGhoul2Info>	mInfos[MAX_G2_MODELS];
	int					mIds[MAX_G2_MODELS];
	std::list<int>			mFreeIndecies;
	void DeleteLow(int idx)
	{
		for (size_t model=0; model< mInfos[idx].size(); model++)
		{
			if (mInfos[idx][model].mBoneCache)
			{
				RemoveBoneCache(mInfos[idx][model].mBoneCache);
				mInfos[idx][model].mBoneCache=0;
			}
		}

		mInfos[idx].clear();

		if ((mIds[idx]>>G2_MODEL_BITS)>(1<<(31-G2_MODEL_BITS)))
		{
			mIds[idx]=MAX_G2_MODELS+idx; //rollover reset id to minimum value
			mFreeIndecies.push_back(idx);
		}
		else
		{
			mIds[idx]+=MAX_G2_MODELS;
			mFreeIndecies.push_front(idx);
		}
	}
public:
	Ghoul2InfoArray()
	{
		int i;
		for (i=0;i<MAX_G2_MODELS;i++)
		{
			mIds[i]=MAX_G2_MODELS+i;
			mFreeIndecies.push_back(i);
		}
	}
#if G2API_DEBUG
	~Ghoul2InfoArray()
	{
		if (mFreeIndecies.size()<MAX_G2_MODELS)
		{
			Com_OPrintf("************************\nLeaked %d ghoul2info slots\n", MAX_G2_MODELS - mFreeIndecies.size());
			int i;
			for (i=0;i<MAX_G2_MODELS;i++)
			{
				list<int>::iterator j;
				for (j=mFreeIndecies.begin();j!=mFreeIndecies.end();++j)
				{
					if (*j==i)
						break;
				}
				if (j==mFreeIndecies.end())
				{
					Com_OPrintf("Leaked Info idx=%d id=%d sz=%d\n", i, mIds[i], mInfos[i].size());
					if (mInfos[i].size())
					{
						Com_OPrintf("%s\n", mInfos[i][0].mFileName);
					}
				}
			}
		}
		else
		{
			Com_OPrintf("No ghoul2 info slots leaked\n");
		}
	}
#endif
	int New()
	{
		if (mFreeIndecies.empty())
		{
			assert(0);
			Com_Error(ERR_FATAL, "Out of ghoul2 info slots");

		}
		// gonna pull from the front, doing a
		int idx=*mFreeIndecies.begin();
		mFreeIndecies.erase(mFreeIndecies.begin());
		return mIds[idx];
	}
	bool IsValid(int handle) const
	{
		if ( handle <= 0 )
		{
			return false;
		}
		assert((handle&G2_INDEX_MASK)>=0&&(handle&G2_INDEX_MASK)<MAX_G2_MODELS); //junk handle
		if (mIds[handle&G2_INDEX_MASK]!=handle) // not a valid handle, could be old
		{
			return false;
		}
		return true;
	}
	void Delete(int handle)
	{
		if (handle <= 0)
		{
			return;
		}
		assert((handle&G2_INDEX_MASK)>=0&&(handle&G2_INDEX_MASK)<MAX_G2_MODELS); //junk handle
		assert(mIds[handle&G2_INDEX_MASK]==handle); // not a valid handle, could be old or garbage
		if (mIds[handle&G2_INDEX_MASK]==handle)
		{
			DeleteLow(handle&G2_INDEX_MASK);
		}
	}
	std::vector<CGhoul2Info> &Get(int handle)
	{
		assert(handle>0); //null handle
		assert((handle&G2_INDEX_MASK)>=0&&(handle&G2_INDEX_MASK)<MAX_G2_MODELS); //junk handle
		assert(mIds[handle&G2_INDEX_MASK]==handle); // not a valid handle, could be old or garbage
		assert(!(handle<=0||(handle&G2_INDEX_MASK)<0||(handle&G2_INDEX_MASK)>=MAX_G2_MODELS||mIds[handle&G2_INDEX_MASK]!=handle));

		return mInfos[handle&G2_INDEX_MASK];
	}
	const std::vector<CGhoul2Info> &Get(int handle) const
	{
		assert(handle>0);
		assert(mIds[handle&G2_INDEX_MASK]==handle); // not a valid handle, could be old or garbage
		return mInfos[handle&G2_INDEX_MASK];
	}

#if G2API_DEBUG
	vector<CGhoul2Info> &GetDebug(int handle)
	{
		static vector<CGhoul2Info> null;
		if (handle<=0||(handle&G2_INDEX_MASK)<0||(handle&G2_INDEX_MASK)>=MAX_G2_MODELS||mIds[handle&G2_INDEX_MASK]!=handle)
		{
			return *(vector<CGhoul2Info> *)0; // null reference, intentional
		}
		return mInfos[handle&G2_INDEX_MASK];
	}
	void TestAllAnims()
	{
		int j;
		for (j=0;j<MAX_G2_MODELS;j++)
		{
			vector<CGhoul2Info> &ghoul2=mInfos[j];
			int i;
			for (i=0; i<ghoul2.size(); i++)
			{
				if (G2_SetupModelPointers(&ghoul2[i]))
				{
					G2ANIM(&ghoul2[i],"Test All");
				}
			}
		}
	}

#endif
};

static Ghoul2InfoArray *singleton = NULL;
IGhoul2InfoArray &TheGhoul2InfoArray()
{
	if(!singleton) {
		singleton = new Ghoul2InfoArray;
	}
	return *singleton;
}


void Ghoul2InfoArray_Free(void)
{
	if(singleton) {
		delete singleton;
		singleton = NULL;
	}
}

// this is the ONLY function to read entity states directly
void G2API_CleanGhoul2Models(CGhoul2Info_v **ghoul2Ptr)
{
	if (*ghoul2Ptr)
	{
		CGhoul2Info_v &ghoul2 = *(*ghoul2Ptr);

#if 0 //rwwFIXMEFIXME: Disable this before release!!!!!! I am just trying to find a crash bug.
		extern int R_GetRNumEntities(void);
		extern void R_SetRNumEntities(int num);
		//check if this instance is actually on a refentity
		int i = 0;
		int r = R_GetRNumEntities();

		while (i < r)
		{
			if ((CGhoul2Info_v *)backEndData->entities[i].e.ghoul2 == *ghoul2Ptr)
			{
				char fName[MAX_QPATH];
				char mName[MAX_QPATH];

				if (ghoul2[0].currentModel)
				{
					strcpy(mName, ghoul2[0].currentModel->name);
				}
				else
				{
					strcpy(mName, "NULL!");
				}

				if (ghoul2[0].mFileName && ghoul2[0].mFileName[0])
				{
					strcpy(fName, ghoul2[0].mFileName);
				}
				else
				{
					strcpy(fName, "None?!");
				}

				ri.Printf( PRINT_ALL, "ERROR, GHOUL2 INSTANCE BEING REMOVED BELONGS TO A REFENTITY!\nThis is in caps because it's important. Tell Rich and save the following text.\n\n");
				ri.Printf( PRINT_ALL, "Ref num: %i\nModel: %s\nFilename: %s\n", i, mName, fName);

				R_SetRNumEntities(0); //avoid recursive error
				Com_Error(ERR_DROP, "Write down or save this error message, show it to Rich\nRef num: %i\nModel: %s\nFilename: %s\n", i, mName, fName);
			}
			i++;
		}
#endif

#ifdef _G2_GORE
		G2API_ClearSkinGore ( ghoul2 );
#endif

#ifdef _FULL_G2_LEAK_CHECKING
		if (g_G2AllocServer)
		{
			g_G2ServerAlloc -= sizeof(*ghoul2Ptr);
		}
		else
		{
			g_G2ClientAlloc -= sizeof(*ghoul2Ptr);
		}
		g_Ghoul2Allocations -= sizeof(*ghoul2Ptr);
		G2_DEBUG_RemovePtrFromTracker(*ghoul2Ptr);
#endif

		delete *ghoul2Ptr;
		*ghoul2Ptr = NULL;
	}
}

qboolean G2_ShouldRegisterServer(void)
{
	if ( !ri.GetCurrentVM )
		return qfalse;

	vm_t *currentVM = ri.GetCurrentVM();

	if ( currentVM && currentVM->slot == VM_GAME )
	{
		if ( ri.Cvar_VariableIntegerValue( "cl_running" ) &&
			ri.Com_TheHunkMarkHasBeenMade() && ShaderHashTableExists())
		{ //if the hunk has been marked then we are now loading client assets so don't load on server.
			return qfalse;
		}

		return qtrue;
	}
	return qfalse;
}

qhandle_t G2API_PrecacheGhoul2Model( const char *fileName )
{
	if ( G2_ShouldRegisterServer() )
		return RE_RegisterServerModel( fileName );
	else
		return RE_RegisterModel( fileName );
}

// initialise all that needs to be on a new Ghoul II model
int G2API_InitGhoul2Model(CGhoul2Info_v **ghoul2Ptr, const char *fileName, int modelIndex, qhandle_t customSkin,
						  qhandle_t customShader, int modelFlags, int lodBias)
{
	int				model;

	// are we actually asking for a model to be loaded.
	if (!fileName || !fileName[0])
	{
		assert(0);
		return -1;
	}

	if (!(*ghoul2Ptr))
	{
		*ghoul2Ptr = new CGhoul2Info_v;
#ifdef _FULL_G2_LEAK_CHECKING
		if (g_G2AllocServer)
		{
			g_G2ServerAlloc += sizeof(CGhoul2Info_v);
		}
		else
		{
			g_G2ClientAlloc += sizeof(CGhoul2Info_v);
		}
		g_Ghoul2Allocations += sizeof(CGhoul2Info_v);
		G2_DEBUG_ShovePtrInTracker(*ghoul2Ptr);
#endif
	}

	CGhoul2Info_v &ghoul2 = *(*ghoul2Ptr);

	// find a free spot in the list
	for (model=0; model< ghoul2.size(); model++)
	{
		if (ghoul2[model].mModelindex == -1)
		{
			ghoul2[model]=CGhoul2Info();
			break;
		}
	}
	if (model==ghoul2.size())
	{	//init should not be used to create additional models, only the first one
		assert(ghoul2.size() < 4); //use G2API_CopySpecificG2Model to add models
		ghoul2.push_back(CGhoul2Info());
	}

	strcpy(ghoul2[model].mFileName, fileName);
	ghoul2[model].mModelindex = model;
	if (!G2_TestModelPointers(&ghoul2[model]))
	{
		ghoul2[model].mFileName[0]=0;
		ghoul2[model].mModelindex = -1;
	}
	else
	{
		G2_Init_Bone_List(ghoul2[model].mBlist, ghoul2[model].aHeader->numBones);
		G2_Init_Bolt_List(ghoul2[model].mBltlist);
		ghoul2[model].mCustomShader = customShader;
		ghoul2[model].mCustomSkin = customSkin;
		ghoul2[model].mLodBias = lodBias;
		ghoul2[model].mAnimFrameDefault = 0;
		ghoul2[model].mFlags = 0;

		ghoul2[model].mModelBoltLink = -1;
	}
	return ghoul2[model].mModelindex;
}

qboolean G2API_SetLodBias(CGhoul2Info *ghlInfo, int lodBias)
{
	if (G2_SetupModelPointers(ghlInfo))
	{
		ghlInfo->mLodBias = lodBias;
		return qtrue;
	}
	return qfalse;
}

void G2_SetSurfaceOnOffFromSkin (CGhoul2Info *ghlInfo, qhandle_t renderSkin);
qboolean G2API_SetSkin(CGhoul2Info_v& ghoul2, int modelIndex, qhandle_t customSkin, qhandle_t renderSkin)
{
	CGhoul2Info *ghlInfo = &ghoul2[modelIndex];

	if (G2_SetupModelPointers(ghlInfo))
	{
		ghlInfo->mCustomSkin = customSkin;
		if (renderSkin)
		{//this is going to set the surfs on/off matching the skin file
			G2_SetSurfaceOnOffFromSkin( ghlInfo, renderSkin );
		}

		return qtrue;
	}
	return qfalse;
}

qboolean G2API_SetShader(CGhoul2Info *ghlInfo, qhandle_t customShader)
{
	if (G2_SetupModelPointers(ghlInfo))
	{
		ghlInfo->mCustomShader = customShader;
		return qtrue;
	}
	return qfalse;
}

qboolean G2API_SetSurfaceOnOff(CGhoul2Info_v &ghoul2, const char *surfaceName, const int flags)
{
	CGhoul2Info *ghlInfo = NULL;

	if (ghoul2.size()>0)
	{
		ghlInfo = &ghoul2[0];
	}

	if (G2_SetupModelPointers(ghlInfo))
	{
		// ensure we flush the cache
		ghlInfo->mMeshFrameNum = 0;
		return G2_SetSurfaceOnOff(ghlInfo, ghlInfo->mSlist, surfaceName, flags);
	}
	return qfalse;
}

int G2API_GetSurfaceOnOff(CGhoul2Info *ghlInfo, const char *surfaceName)
{
	if (G2_SetupModelPointers(ghlInfo))
	{
		return G2_IsSurfaceOff(ghlInfo, ghlInfo->mSlist, surfaceName);
	}
	return -1;
}

qboolean G2API_SetRootSurface(CGhoul2Info_v &ghoul2, const int modelIndex, const char *surfaceName)
{
	if (G2_SetupModelPointers(ghoul2))
	{
		return G2_SetRootSurface(ghoul2, modelIndex, surfaceName);
	}

	return qfalse;
}

int G2API_AddSurface(CGhoul2Info *ghlInfo, int surfaceNumber, int polyNumber, float BarycentricI, float BarycentricJ, int lod )
{
	if (G2_SetupModelPointers(ghlInfo))
	{
		// ensure we flush the cache
		ghlInfo->mMeshFrameNum = 0;
		return G2_AddSurface(ghlInfo, surfaceNumber, polyNumber, BarycentricI, BarycentricJ, lod);
	}
	return -1;
}

qboolean G2API_RemoveSurface(CGhoul2Info *ghlInfo, const int index)
{
	if (G2_SetupModelPointers(ghlInfo))
	{
		// ensure we flush the cache
		ghlInfo->mMeshFrameNum = 0;
		return G2_RemoveSurface(ghlInfo->mSlist, index);
	}
	return qfalse;
}

int G2API_GetParentSurface(CGhoul2Info *ghlInfo, const int index)
{
	if (G2_SetupModelPointers(ghlInfo))
	{
		return G2_GetParentSurface(ghlInfo, index);
	}
	return -1;
}

int G2API_GetSurfaceRenderStatus(CGhoul2Info_v& ghoul2, int modelIndex, const char *surfaceName)
{
	CGhoul2Info *ghlInfo = &ghoul2[modelIndex];

	if (G2_SetupModelPointers(ghlInfo))
	{
		return G2_IsSurfaceRendered(ghlInfo, surfaceName, ghlInfo->mSlist);
	}
	return -1;
}

qboolean G2API_HasGhoul2ModelOnIndex(CGhoul2Info_v **ghlRemove, const int modelIndex)
{
	CGhoul2Info_v &ghlInfo = **ghlRemove;

	if (!ghlInfo.size() || (ghlInfo.size() <= modelIndex) || (ghlInfo[modelIndex].mModelindex == -1))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean G2API_RemoveGhoul2Model(CGhoul2Info_v **ghlRemove, const int modelIndex)
{
	CGhoul2Info_v &ghlInfo = **ghlRemove;

	// sanity check
	if (!ghlInfo.size() || (ghlInfo.size() <= modelIndex) || (ghlInfo[modelIndex].mModelindex == -1))
	{
		// if we hit this assert then we are trying to delete a ghoul2 model on a ghoul2 instance that
		// one way or another is already gone.
		assert(0);
		return qfalse;
	}

	if (ghlInfo.size() > modelIndex)
	{
#ifdef _G2_GORE
		// Cleanup the gore attached to this model
		if ( ghlInfo[modelIndex].mGoreSetTag )
		{
			DeleteGoreSet ( ghlInfo[modelIndex].mGoreSetTag );
			ghlInfo[modelIndex].mGoreSetTag = 0;
		}
#endif

		if (ghlInfo[modelIndex].mBoneCache)
		{
			RemoveBoneCache(ghlInfo[modelIndex].mBoneCache);
			ghlInfo[modelIndex].mBoneCache=0;
		}

		// clear out the vectors this model used.
		ghlInfo[modelIndex].mBlist.clear();
		ghlInfo[modelIndex].mBltlist.clear();
		ghlInfo[modelIndex].mSlist.clear();

	   	 // set us to be the 'not active' state
		ghlInfo[modelIndex].mModelindex = -1;

		int newSize = ghlInfo.size();
		// now look through the list from the back and see if there is a block of -1's we can resize off the end of the list
		for (int i=ghlInfo.size()-1; i>-1; i--)
		{
			if (ghlInfo[i].mModelindex == -1)
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
		if (newSize != ghlInfo.size())
		{
			// yes, so lets do it
			ghlInfo.resize(newSize);
		}

		// if we are not using any space, just delete the ghoul2 vector entirely
		if (!ghlInfo.size())
		{
#ifdef _FULL_G2_LEAK_CHECKING
			if (g_G2AllocServer)
			{
				g_G2ServerAlloc -= sizeof(*ghlRemove);
			}
			else
			{
				g_G2ClientAlloc -= sizeof(*ghlRemove);
			}
			g_Ghoul2Allocations -= sizeof(*ghlRemove);
#endif
			delete *ghlRemove;
			*ghlRemove = NULL;
		}
	}


	return qtrue;
}

qboolean G2API_RemoveGhoul2Models(CGhoul2Info_v **ghlRemove)
{//remove 'em ALL!
	CGhoul2Info_v &ghlInfo = **ghlRemove;
	int	modelIndex = 0;
	int newSize = 0;
	int i;

	// sanity check
	if ( !ghlInfo.size() )
	{// if we hit this then we are trying to delete a ghoul2 model on a ghoul2 instance that
		// one way or another is already gone.
		return qfalse;
	}

	for ( modelIndex = 0; modelIndex < ghlInfo.size(); modelIndex++ )
	{
		if ( ghlInfo[modelIndex].mModelindex == -1 )
		{
			continue;
		}
#ifdef _G2_GORE
		// Cleanup the gore attached to this model
		if ( ghlInfo[modelIndex].mGoreSetTag )
		{
			DeleteGoreSet ( ghlInfo[modelIndex].mGoreSetTag );
			ghlInfo[modelIndex].mGoreSetTag = 0;
		}
#endif

		if (ghlInfo[modelIndex].mBoneCache)
		{
			RemoveBoneCache(ghlInfo[modelIndex].mBoneCache);
			ghlInfo[modelIndex].mBoneCache=0;
		}

		// clear out the vectors this model used.
		ghlInfo[modelIndex].mBlist.clear();
		ghlInfo[modelIndex].mBltlist.clear();
		ghlInfo[modelIndex].mSlist.clear();

	   	 // set us to be the 'not active' state
		ghlInfo[modelIndex].mModelindex = -1;
	}

	newSize = ghlInfo.size();
	// now look through the list from the back and see if there is a block of -1's we can resize off the end of the list
	for (i=ghlInfo.size()-1; i>-1; i--)
	{
		if (ghlInfo[i].mModelindex == -1)
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
	if (newSize != ghlInfo.size())
	{
		// yes, so lets do it
		ghlInfo.resize(newSize);
	}

	// if we are not using any space, just delete the ghoul2 vector entirely
	if (!ghlInfo.size())
	{
#ifdef _FULL_G2_LEAK_CHECKING
		if (g_G2AllocServer)
		{
			g_G2ServerAlloc -= sizeof(*ghlRemove);
		}
		else
		{
			g_G2ClientAlloc -= sizeof(*ghlRemove);
		}
		g_Ghoul2Allocations -= sizeof(*ghlRemove);
#endif
		delete *ghlRemove;
		*ghlRemove = NULL;
	}
	return qtrue;
}

//check if a bone exists on skeleton without actually adding to the bone list -rww
qboolean G2API_DoesBoneExist(CGhoul2Info_v& ghoul2, int modelIndex, const char *boneName)
{
	CGhoul2Info *ghlInfo = &ghoul2[modelIndex];

	if (G2_SetupModelPointers(ghlInfo))
	{ //model is valid
		mdxaHeader_t *mdxa = ghlInfo->currentModel->mdxa;
		if (mdxa)
		{ //get the skeleton data and iterate through the bones
			int i;
			mdxaSkel_t *skel;
			mdxaSkelOffsets_t *offsets;

			offsets = (mdxaSkelOffsets_t *)((byte *)mdxa + sizeof(mdxaHeader_t));

 			for (i = 0; i < mdxa->numBones; i++)
 			{
 				skel = (mdxaSkel_t *)((byte *)mdxa + sizeof(mdxaHeader_t) + offsets->offsets[i]);
 				if (!Q_stricmp(skel->name, boneName))
				{ //got it
					return qtrue;
				}
			}
		}
	}

	//guess it doesn't exist
	return qfalse;
}

//rww - RAGDOLL_BEGIN
#define		GHOUL2_RAG_STARTED						0x0010
#define		GHOUL2_RAG_FORCESOLVE					0x1000		//api-override, determine if ragdoll should be forced to continue solving even if it thinks it is settled
//rww - RAGDOLL_END

qboolean G2API_SetBoneAnimIndex(CGhoul2Info *ghlInfo, const int index, const int AstartFrame, const int AendFrame, const int flags, const float animSpeed, const int currentTime, const float AsetFrame, const int blendTime)
{
	qboolean setPtrs = qfalse;
	qboolean res = qfalse;

	//rww - RAGDOLL_BEGIN
	if (ghlInfo)
	{
		res = G2_SetupModelPointers(ghlInfo);
		setPtrs = qtrue;

		if (res)
		{
			if (ghlInfo->mFlags & GHOUL2_RAG_STARTED)
			{
				return qfalse;
			}
		}
	}
	//rww - RAGDOLL_END

	int endFrame=AendFrame;
	int startFrame=AstartFrame;
	float setFrame=AsetFrame;
	assert(endFrame>0);
	assert(startFrame>=0);
	assert(endFrame<100000);
	assert(startFrame<100000);
	assert(setFrame>=0.0f||setFrame==-1.0f);
	assert(setFrame<=100000.0f);
	if (endFrame<=0)
	{
		endFrame=1;
	}
	if (endFrame>=100000)
	{
		endFrame=1;
	}
	if (startFrame<0)
	{
		startFrame=0;
	}
	if (startFrame>=100000)
	{
		startFrame=0;
	}
	if (setFrame<0.0f&&setFrame!=-1.0f)
	{
		setFrame=0.0f;
	}
	if (setFrame>100000.0f)
	{
		setFrame=0.0f;
	}
	if (!setPtrs)
	{
		res = G2_SetupModelPointers(ghlInfo);
	}

	if (res)
	{
		// ensure we flush the cache
		ghlInfo->mSkelFrameNum = 0;
 		return G2_Set_Bone_Anim_Index(ghlInfo->mBlist, index, startFrame, endFrame, flags, animSpeed, currentTime, setFrame, blendTime, ghlInfo->aHeader->numFrames);
	}
	return qfalse;
}

#define _PLEASE_SHUT_THE_HELL_UP

qboolean G2API_SetBoneAnim(CGhoul2Info_v &ghoul2, const int modelIndex, const char *boneName, const int AstartFrame, const int AendFrame, const int flags, const float animSpeed, const int currentTime, const float AsetFrame, const int blendTime)
{
	int endFrame=AendFrame;
	int startFrame=AstartFrame;
	float setFrame=AsetFrame;
#ifndef _PLEASE_SHUT_THE_HELL_UP
	assert(endFrame>0);
	assert(startFrame>=0);
	assert(endFrame<100000);
	assert(startFrame<100000);
	assert(setFrame>=0.0f||setFrame==-1.0f);
	assert(setFrame<=100000.0f);
#endif
	if (endFrame<=0)
	{
		endFrame=1;
	}
	if (endFrame>=100000)
	{
		endFrame=1;
	}
	if (startFrame<0)
	{
		startFrame=0;
	}
	if (startFrame>=100000)
	{
		startFrame=0;
	}
	if (setFrame<0.0f&&setFrame!=-1.0f)
	{
		setFrame=0.0f;
	}
	if (setFrame>100000.0f)
	{
		setFrame=0.0f;
	}
	if (ghoul2.size()>modelIndex)
	{
		CGhoul2Info *ghlInfo = &ghoul2[modelIndex];
		qboolean setPtrs = qfalse;
		qboolean res = qfalse;

		//rww - RAGDOLL_BEGIN
		if (ghlInfo)
		{
			res = G2_SetupModelPointers(ghlInfo);
			setPtrs = qtrue;

			if (res)
			{
				if (ghlInfo->mFlags & GHOUL2_RAG_STARTED)
				{
					return qfalse;
				}
			}
		}
		//rww - RAGDOLL_END

		if (!setPtrs)
		{
			res = G2_SetupModelPointers(ghlInfo);
		}

		if (res)
		{
			// ensure we flush the cache
			ghlInfo->mSkelFrameNum = 0;
 			return G2_Set_Bone_Anim(ghlInfo, ghlInfo->mBlist, boneName, startFrame, endFrame, flags, animSpeed, currentTime, setFrame, blendTime);
		}
	}
	return qfalse;
}

qboolean G2API_GetBoneAnim(CGhoul2Info_v& ghoul2, int modelIndex, const char *boneName, const int currentTime, float *currentFrame,
						   int *startFrame, int *endFrame, int *flags, float *animSpeed, int *modelList)
{
	assert(startFrame!=endFrame); //this is bad
	assert(startFrame!=flags); //this is bad
	assert(endFrame!=flags); //this is bad
	assert(currentFrame!=animSpeed); //this is bad

	CGhoul2Info *ghlInfo = &ghoul2[modelIndex];

	if (G2_SetupModelPointers(ghlInfo))
	{
		int aCurrentTime=G2API_GetTime(currentTime);
 		qboolean ret=G2_Get_Bone_Anim(ghlInfo, ghlInfo->mBlist, boneName, aCurrentTime, currentFrame,
			startFrame, endFrame, flags, animSpeed, modelList, ghlInfo->mModelindex);
#ifdef _DEBUG
		/*
		assert(*endFrame>0);
		assert(*endFrame<100000);
		assert(*startFrame>=0);
		assert(*startFrame<100000);
		assert(*currentFrame>=0.0f);
		assert(*currentFrame<100000.0f);
		*/
		if (*endFrame<1)
		{
			*endFrame=1;
		}
		if (*endFrame>100000)
		{
			*endFrame=1;
		}
		if (*startFrame<0)
		{
			*startFrame=0;
		}
		if (*startFrame>100000)
		{
			*startFrame=1;
		}
		if (*currentFrame<0.0f)
		{
			*currentFrame=0.0f;
		}
		if (*currentFrame>100000)
		{
			*currentFrame=1;
		}
#endif
		return ret;
	}
	return qfalse;
}

qboolean G2API_GetAnimRange(CGhoul2Info *ghlInfo, const char *boneName,	int *startFrame, int *endFrame)
{
	assert(startFrame!=endFrame); //this is bad
	if (G2_SetupModelPointers(ghlInfo))
	{
 		qboolean ret=G2_Get_Bone_Anim_Range(ghlInfo, ghlInfo->mBlist, boneName, startFrame, endFrame);
#ifdef _DEBUG
		assert(*endFrame>0);
		assert(*endFrame<100000);
		assert(*startFrame>=0);
		assert(*startFrame<100000);
		if (*endFrame<1)
		{
			*endFrame=1;
		}
		if (*endFrame>100000)
		{
			*endFrame=1;
		}
		if (*startFrame<0)
		{
			*startFrame=0;
		}
		if (*startFrame>100000)
		{
			*startFrame=1;
		}
#endif
		return ret;
	}
	return qfalse;
}


qboolean G2API_PauseBoneAnim(CGhoul2Info *ghlInfo, const char *boneName, const int currentTime)
{
	if (G2_SetupModelPointers(ghlInfo))
	{
 		return G2_Pause_Bone_Anim(ghlInfo, ghlInfo->mBlist, boneName, currentTime);
	}
	return qfalse;
}

qboolean	G2API_IsPaused(CGhoul2Info *ghlInfo, const char *boneName)
{
	if (G2_SetupModelPointers(ghlInfo))
	{
 		return G2_IsPaused(ghlInfo->mFileName, ghlInfo->mBlist, boneName);
	}
	return qfalse;
}

qboolean G2API_StopBoneAnimIndex(CGhoul2Info *ghlInfo, const int index)
{
	if (G2_SetupModelPointers(ghlInfo))
	{
 		return G2_Stop_Bone_Anim_Index(ghlInfo->mBlist, index);
	}
	return qfalse;
}

qboolean G2API_StopBoneAnim(CGhoul2Info *ghlInfo, const char *boneName)
{
	if (G2_SetupModelPointers(ghlInfo))
	{
 		return G2_Stop_Bone_Anim(ghlInfo->mFileName, ghlInfo->mBlist, boneName);
	}
	return qfalse;
}

qboolean G2API_SetBoneAnglesIndex(CGhoul2Info *ghlInfo, const int index, const vec3_t angles, const int flags,
							 const Eorientations yaw, const Eorientations pitch, const Eorientations roll,
							 qhandle_t *modelList, int blendTime, int currentTime)
{
	qboolean setPtrs = qfalse;
	qboolean res = qfalse;

	//rww - RAGDOLL_BEGIN
	if (ghlInfo)
	{
		res = G2_SetupModelPointers(ghlInfo);
		setPtrs = qtrue;

		if (res)
		{
			if (ghlInfo->mFlags & GHOUL2_RAG_STARTED)
			{
				return qfalse;
			}
		}
	}
	//rww - RAGDOLL_END

	if (!setPtrs)
	{
		res = G2_SetupModelPointers(ghlInfo);
	}

	if (res)
	{
		// ensure we flush the cache
		ghlInfo->mSkelFrameNum = 0;
		return G2_Set_Bone_Angles_Index( ghlInfo->mBlist, index, angles, flags, yaw, pitch, roll, modelList, ghlInfo->mModelindex, blendTime, currentTime);
	}
	return qfalse;
}

qboolean G2API_SetBoneAngles(CGhoul2Info_v &ghoul2, const int modelIndex, const char *boneName, const vec3_t angles, const int flags,
							 const Eorientations up, const Eorientations left, const Eorientations forward,
							 qhandle_t *modelList, int blendTime, int currentTime )
{
	if (ghoul2.size()>modelIndex)
	{
		CGhoul2Info *ghlInfo = &ghoul2[modelIndex];
		qboolean setPtrs = qfalse;
		qboolean res = qfalse;

		//rww - RAGDOLL_BEGIN
		if (ghlInfo)
		{
			res = G2_SetupModelPointers(ghlInfo);
			setPtrs = qtrue;

			if (res)
			{
				if (ghlInfo->mFlags & GHOUL2_RAG_STARTED)
				{
					return qfalse;
				}
			}
		}
		//rww - RAGDOLL_END

		if (!setPtrs)
		{
			res = G2_SetupModelPointers(ghoul2);
		}

		if (res)
		{
				// ensure we flush the cache
			ghlInfo->mSkelFrameNum = 0;
			return G2_Set_Bone_Angles(ghlInfo, ghlInfo->mBlist, boneName, angles, flags, up, left, forward, modelList, ghlInfo->mModelindex, blendTime, currentTime);
		}
	}
	return qfalse;
}

qboolean G2API_SetBoneAnglesMatrixIndex(CGhoul2Info *ghlInfo, const int index, const mdxaBone_t &matrix,
								   const int flags, qhandle_t *modelList, int blendTime, int currentTime)
{
	if (G2_SetupModelPointers(ghlInfo))
	{
		// ensure we flush the cache
		ghlInfo->mSkelFrameNum = 0;
		return G2_Set_Bone_Angles_Matrix_Index(ghlInfo->mBlist, index, matrix, flags, modelList, ghlInfo->mModelindex, blendTime, currentTime);
	}
	return qfalse;
}

qboolean G2API_SetBoneAnglesMatrix(CGhoul2Info *ghlInfo, const char *boneName, const mdxaBone_t &matrix,
								   const int flags, qhandle_t *modelList, int blendTime, int currentTime)
{
	if (G2_SetupModelPointers(ghlInfo))
	{
		// ensure we flush the cache
		ghlInfo->mSkelFrameNum = 0;
		return G2_Set_Bone_Angles_Matrix(ghlInfo->mFileName, ghlInfo->mBlist, boneName, matrix, flags, modelList, ghlInfo->mModelindex, blendTime, currentTime);
	}
	return qfalse;
}

qboolean G2API_StopBoneAnglesIndex(CGhoul2Info *ghlInfo, const int index)
{
	if (G2_SetupModelPointers(ghlInfo))
	{
		// ensure we flush the cache
		ghlInfo->mSkelFrameNum = 0;
 		return G2_Stop_Bone_Angles_Index(ghlInfo->mBlist, index);
	}
	return qfalse;
}

qboolean G2API_StopBoneAngles(CGhoul2Info *ghlInfo, const char *boneName)
{
	if (G2_SetupModelPointers(ghlInfo))
	{
		// ensure we flush the cache
		ghlInfo->mSkelFrameNum = 0;
 		return G2_Stop_Bone_Angles(ghlInfo->mFileName, ghlInfo->mBlist, boneName);
	}
	return qfalse;
}


void G2API_AbsurdSmoothing(CGhoul2Info_v &ghoul2, qboolean status)
{
	assert(ghoul2.size());
	CGhoul2Info *ghlInfo = &ghoul2[0];

	if (status)
	{ //turn it on
		ghlInfo->mFlags |= GHOUL2_CRAZY_SMOOTH;
	}
	else
	{ //off
		ghlInfo->mFlags &= ~GHOUL2_CRAZY_SMOOTH;
	}
}

//rww - RAGDOLL_BEGIN
class CRagDollParams;
void G2_SetRagDoll(CGhoul2Info_v &ghoul2V,CRagDollParams *parms);
void G2API_SetRagDoll(CGhoul2Info_v &ghoul2,CRagDollParams *parms)
{
	G2_SetRagDoll(ghoul2,parms);
}

void G2_ResetRagDoll(CGhoul2Info_v &ghoul2V);
void G2API_ResetRagDoll(CGhoul2Info_v &ghoul2)
{
	G2_ResetRagDoll(ghoul2);
}
//rww - RAGDOLL_END

qboolean G2API_RemoveBone(CGhoul2Info_v& ghoul2, int modelIndex, const char *boneName)
{
	CGhoul2Info *ghlInfo = &ghoul2[modelIndex];

	if (G2_SetupModelPointers(ghlInfo))
	{
		// ensure we flush the cache
		ghlInfo->mSkelFrameNum = 0;
 		return G2_Remove_Bone(ghlInfo, ghlInfo->mBlist, boneName);
	}
	return qfalse;
}

//rww - RAGDOLL_BEGIN
#ifdef _DEBUG
extern int ragTraceTime;
extern int ragSSCount;
extern int ragTraceCount;
#endif

void G2API_AnimateG2ModelsRag(CGhoul2Info_v &ghoul2, int AcurrentTime,CRagDollUpdateParams *params)
{
	int model;
	int currentTime=G2API_GetTime(AcurrentTime);

#ifdef _DEBUG
	ragTraceTime = 0;
	ragSSCount = 0;
	ragTraceCount = 0;
#endif

	// Walk the list and find all models that are active
	for (model = 0; model < ghoul2.size(); model++)
	{
		if (ghoul2[model].mModel)
		{
			G2_Animate_Bone_List(ghoul2,currentTime,model,params);
		}
	}
#ifdef _DEBUG
	/*
	if (ragTraceTime)
	{
		ri.Printf( PRINT_ALL, "Rag trace time: %i (%i STARTSOLID, %i TOTAL)\n", ragTraceTime, ragSSCount, ragTraceCount);
	}
	*/

	//keep sane limits here, if it gets too slow an assert is proper.
//	assert(ragTraceTime < 150);
//	assert(ragTraceCount < 1500);
#endif
}
//rww - RAGDOLL_END

int G2_Find_Bone_Rag(CGhoul2Info *ghlInfo, boneInfo_v &blist, const char *boneName);
#define RAG_PCJ						(0x00001)
#define RAG_EFFECTOR				(0x00100)

static inline boneInfo_t *G2_GetRagBoneConveniently(CGhoul2Info_v &ghoul2, const char *boneName)
{
	assert(ghoul2.size());
	CGhoul2Info *ghlInfo = &ghoul2[0];

	if (!(ghlInfo->mFlags & GHOUL2_RAG_STARTED))
	{ //can't do this if not in ragdoll
		return NULL;
	}

	int boneIndex = G2_Find_Bone_Rag(ghlInfo, ghlInfo->mBlist, boneName);

	if (boneIndex < 0)
	{ //bad bone specification
		return NULL;
	}

	boneInfo_t *bone = &ghlInfo->mBlist[boneIndex];

	if (!(bone->flags & BONE_ANGLES_RAGDOLL))
	{ //only want to return rag bones
		return NULL;
	}

	return bone;
}

qboolean G2API_RagPCJConstraint(CGhoul2Info_v &ghoul2, const char *boneName, vec3_t min, vec3_t max)
{
	boneInfo_t *bone = G2_GetRagBoneConveniently(ghoul2, boneName);

	if (!bone)
	{
		return qfalse;
	}

	if (!(bone->RagFlags & RAG_PCJ))
	{ //this function is only for PCJ bones
		return qfalse;
	}

	VectorCopy(min, bone->minAngles);
	VectorCopy(max, bone->maxAngles);

	return qtrue;
}

qboolean G2API_RagPCJGradientSpeed(CGhoul2Info_v &ghoul2, const char *boneName, const float speed)
{
	boneInfo_t *bone = G2_GetRagBoneConveniently(ghoul2, boneName);

	if (!bone)
	{
		return qfalse;
	}

	if (!(bone->RagFlags & RAG_PCJ))
	{ //this function is only for PCJ bones
		return qfalse;
	}

	bone->overGradSpeed = speed;

	return qtrue;
}

qboolean G2API_RagEffectorGoal(CGhoul2Info_v &ghoul2, const char *boneName, vec3_t pos)
{
	boneInfo_t *bone = G2_GetRagBoneConveniently(ghoul2, boneName);

	if (!bone)
	{
		return qfalse;
	}

	if (!(bone->RagFlags & RAG_EFFECTOR))
	{ //this function is only for effectors
		return qfalse;
	}

	if (!pos)
	{ //go back to none in case we have one then
		bone->hasOverGoal = false;
	}
	else
	{
		VectorCopy(pos, bone->overGoalSpot);
		bone->hasOverGoal = true;
	}
	return qtrue;
}

qboolean G2API_GetRagBonePos(CGhoul2Info_v &ghoul2, const char *boneName, vec3_t pos, vec3_t entAngles, vec3_t entPos, vec3_t entScale)
{ //do something?
	return qfalse;
}

qboolean G2API_RagEffectorKick(CGhoul2Info_v &ghoul2, const char *boneName, vec3_t velocity)
{
	boneInfo_t *bone = G2_GetRagBoneConveniently(ghoul2, boneName);

	if (!bone)
	{
		return qfalse;
	}

	if (!(bone->RagFlags & RAG_EFFECTOR))
	{ //this function is only for effectors
		return qfalse;
	}

	bone->epVelocity[2] = 0;
	VectorAdd(bone->epVelocity, velocity, bone->epVelocity);
	bone->physicsSettled = false;

	return qtrue;
}

qboolean G2API_RagForceSolve(CGhoul2Info_v &ghoul2, qboolean force)
{
	assert(ghoul2.size());
	CGhoul2Info *ghlInfo = &ghoul2[0];

	if (!(ghlInfo->mFlags & GHOUL2_RAG_STARTED))
	{ //can't do this if not in ragdoll
		return qfalse;
	}

	if (force)
	{
		ghlInfo->mFlags |= GHOUL2_RAG_FORCESOLVE;
	}
	else
	{
		ghlInfo->mFlags &= ~GHOUL2_RAG_FORCESOLVE;
	}

	return qtrue;
}

qboolean G2_SetBoneIKState(CGhoul2Info_v &ghoul2, int time, const char *boneName, int ikState, sharedSetBoneIKStateParams_t *params);
qboolean G2API_SetBoneIKState(CGhoul2Info_v &ghoul2, int time, const char *boneName, int ikState, sharedSetBoneIKStateParams_t *params)
{
	return G2_SetBoneIKState(ghoul2, time, boneName, ikState, params);
}

qboolean G2_IKMove(CGhoul2Info_v &ghoul2, int time, sharedIKMoveParams_t *params);
qboolean G2API_IKMove(CGhoul2Info_v &ghoul2, int time, sharedIKMoveParams_t *params)
{
	return G2_IKMove(ghoul2, time, params);
}

qboolean G2API_RemoveBolt(CGhoul2Info *ghlInfo, const int index)
{
	if (G2_SetupModelPointers(ghlInfo))
	{
		return G2_Remove_Bolt( ghlInfo->mBltlist, index);
	}
	return qfalse;
}

int G2API_AddBolt(CGhoul2Info_v &ghoul2, const int modelIndex, const char *boneName)
{
//	assert(ghoul2.size()>modelIndex);

	if (ghoul2.size()>modelIndex)
	{
		CGhoul2Info *ghlInfo = &ghoul2[modelIndex];
		if (G2_SetupModelPointers(ghlInfo))
		{
			return G2_Add_Bolt(ghlInfo, ghlInfo->mBltlist, ghlInfo->mSlist, boneName);
		}
	}
	return -1;
}

int G2API_AddBoltSurfNum(CGhoul2Info *ghlInfo, const int surfIndex)
{
	if (G2_SetupModelPointers(ghlInfo))
	{
		return G2_Add_Bolt_Surf_Num(ghlInfo, ghlInfo->mBltlist, ghlInfo->mSlist, surfIndex);
	}
	return -1;
}


qboolean G2API_AttachG2Model(CGhoul2Info_v &ghoul2From, int modelFrom, CGhoul2Info_v &ghoul2To, int toBoltIndex, int toModel)
{
	assert( toBoltIndex >= 0 );
	if ( toBoltIndex < 0 )
	{
		return qfalse;
	}
	if (G2_SetupModelPointers(ghoul2From)&&G2_SetupModelPointers(ghoul2To))
	{
		// make sure we have a model to attach, a model to attach to, and a bolt on that model
		if ((ghoul2From.size() > modelFrom) &&
			(ghoul2To.size() > toModel) &&
			((ghoul2To[toModel].mBltlist[toBoltIndex].boneNumber != -1) || (ghoul2To[toModel].mBltlist[toBoltIndex].surfaceNumber != -1)))
		{
			// encode the bolt address into the model bolt link
		   toModel &= MODEL_AND;
		   toBoltIndex &= BOLT_AND;
		   ghoul2From[modelFrom].mModelBoltLink = (toModel << MODEL_SHIFT)  | (toBoltIndex << BOLT_SHIFT);
		   return qtrue;
		}
	}
	return qfalse;
}

void G2API_SetBoltInfo(CGhoul2Info_v &ghoul2, int modelIndex, int boltInfo)
{
	if (ghoul2.size() > modelIndex)
	{
		ghoul2[modelIndex].mModelBoltLink = boltInfo;
	}
}

qboolean G2API_DetachG2Model(CGhoul2Info *ghlInfo)
{
	if (G2_SetupModelPointers(ghlInfo))
	{
	   ghlInfo->mModelBoltLink = -1;
	   return qtrue;
	}
	return qfalse;
}

qboolean G2API_AttachEnt(int *boltInfo, CGhoul2Info_v& ghoul2, int modelIndex, int toBoltIndex, int entNum, int toModelNum)
{
	CGhoul2Info *ghlInfoTo = &ghoul2[modelIndex];

	if (boltInfo && G2_SetupModelPointers(ghlInfoTo))
	{
		// make sure we have a model to attach, a model to attach to, and a bolt on that model
		if ( ghlInfoTo->mBltlist.size() && ((ghlInfoTo->mBltlist[toBoltIndex].boneNumber != -1) || (ghlInfoTo->mBltlist[toBoltIndex].surfaceNumber != -1)))
		{
			// encode the bolt address into the model bolt link
		   toModelNum &= MODEL_AND;
		   toBoltIndex &= BOLT_AND;
		   entNum &= ENTITY_AND;
		   *boltInfo =  (toBoltIndex << BOLT_SHIFT) | (toModelNum << MODEL_SHIFT) | (entNum << ENTITY_SHIFT);
		   return qtrue;
		}
	}
	return qfalse;

}

qboolean gG2_GBMNoReconstruct;
qboolean gG2_GBMUseSPMethod;

qboolean G2API_GetBoltMatrix_SPMethod(CGhoul2Info_v &ghoul2, const int modelIndex, const int boltIndex, mdxaBone_t *matrix, const vec3_t angles,
							 const vec3_t position, const int frameNum, qhandle_t *modelList, const vec3_t scale )
{
	assert(ghoul2.size() > modelIndex);

	if ((ghoul2.size() > modelIndex))
	{
		CGhoul2Info *ghlInfo = &ghoul2[modelIndex];

		//assert(boltIndex < ghlInfo->mBltlist.size());

		if (ghlInfo && (boltIndex < (int)ghlInfo->mBltlist.size()) && boltIndex >= 0 )
		{
			// make sure we have transformed the skeleton
			if (!gG2_GBMNoReconstruct)
			{
				G2_ConstructGhoulSkeleton(ghoul2, frameNum, true, scale);
			}

			gG2_GBMNoReconstruct = qfalse;

			mdxaBone_t scaled;
			mdxaBone_t *use;
			use=&ghlInfo->mBltlist[boltIndex].position;

			if (scale[0]||scale[1]||scale[2])
			{
				scaled=*use;
				use=&scaled;

				// scale the bolt position by the scale factor for this model since at this point its still in model space
				if (scale[0])
				{
					scaled.matrix[0][3] *= scale[0];
				}
				if (scale[1])
				{
					scaled.matrix[1][3] *= scale[1];
				}
				if (scale[2])
				{
					scaled.matrix[2][3] *= scale[2];
				}
			}
			// pre generate the world matrix
			G2_GenerateWorldMatrix(angles, position);

			VectorNormalize((float*)use->matrix[0]);
			VectorNormalize((float*)use->matrix[1]);
			VectorNormalize((float*)use->matrix[2]);

			Multiply_3x4Matrix(matrix, &worldMatrix, use);
			return qtrue;
		}
	}
	return qfalse;
}

#define G2ERROR(exp,m)		((void)0) //rwwFIXMEFIXME: This is because I'm lazy.
#define G2WARNING(exp,m)     ((void)0)
#define G2NOTE(exp,m)     ((void)0)
#define G2ANIM(ghlInfo,m) ((void)0)
bool G2_NeedsRecalc(CGhoul2Info *ghlInfo,int frameNum);
void G2_GetBoltMatrixLow(CGhoul2Info &ghoul2,int boltNum,const vec3_t scale,mdxaBone_t &retMatrix);
void G2_GetBoneMatrixLow(CGhoul2Info &ghoul2,int boneNum,const vec3_t scale,mdxaBone_t &retMatrix,mdxaBone_t *&retBasepose,mdxaBone_t *&retBaseposeInv);

//qboolean G2API_GetBoltMatrix(CGhoul2Info_v &ghoul2, const int modelIndex, const int boltIndex, mdxaBone_t *matrix, const vec3_t angles,
//							 const vec3_t position, const int AframeNum, qhandle_t *modelList, const vec3_t scale )
qboolean G2API_GetBoltMatrix(CGhoul2Info_v &ghoul2, const int modelIndex, const int boltIndex, mdxaBone_t *matrix, const vec3_t angles,
							 const vec3_t position, const int frameNum, qhandle_t *modelList, vec3_t scale )
{
//	G2ERROR(ghoul2.IsValid(),"Invalid ghlInfo");
	G2ERROR(matrix,"NULL matrix");
	G2ERROR(modelIndex>=0&&modelIndex<ghoul2.size(),"Invalid ModelIndex");
	const static mdxaBone_t		identityMatrix =
	{
		{
			{ 0.0f, -1.0f, 0.0f, 0.0f },
			{ 1.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 1.0f, 0.0f }
		}
	};
	G2_GenerateWorldMatrix(angles, position);
	if (G2_SetupModelPointers(ghoul2))
	{
		if (matrix&&modelIndex>=0&&modelIndex<ghoul2.size())
		{
			int tframeNum=G2API_GetTime(frameNum);
			CGhoul2Info *ghlInfo = &ghoul2[modelIndex];
			G2ERROR(boltIndex >= 0 && (boltIndex < ghlInfo->mBltlist.size()),va("Invalid Bolt Index (%d:%s)",boltIndex,ghlInfo->mFileName));

			if (boltIndex >= 0 && ghlInfo && (boltIndex < (int)ghlInfo->mBltlist.size()) )
			{
				mdxaBone_t bolt;

#if 0 //yeah, screw it
				if (!gG2_GBMNoReconstruct)
				{ //This should only be used when you know what you're doing.
					if (G2_NeedsRecalc(ghlInfo,tframeNum))
					{
						G2_ConstructGhoulSkeleton(ghoul2,tframeNum,true,scale);
					}
				}
				else
				{
					gG2_GBMNoReconstruct = qfalse;
				}
#else
				if (G2_NeedsRecalc(ghlInfo,tframeNum))
				{
					G2_ConstructGhoulSkeleton(ghoul2,tframeNum,true,scale);
				}
#endif

				G2_GetBoltMatrixLow(*ghlInfo,boltIndex,scale,bolt);
				// scale the bolt position by the scale factor for this model since at this point its still in model space
				if (scale[0])
				{
					bolt.matrix[0][3] *= scale[0];
				}
				if (scale[1])
				{
					bolt.matrix[1][3] *= scale[1];
				}
				if (scale[2])
				{
					bolt.matrix[2][3] *= scale[2];
				}
				VectorNormalize((float*)&bolt.matrix[0]);
				VectorNormalize((float*)&bolt.matrix[1]);
				VectorNormalize((float*)&bolt.matrix[2]);

				Multiply_3x4Matrix(matrix, &worldMatrix, &bolt);
#if G2API_DEBUG
				for ( int i = 0; i < 3; i++ )
				{
					for ( int j = 0; j < 4; j++ )
					{
						assert( !_isnan(matrix->matrix[i][j]));
					}
				}
#endif// _DEBUG
				G2ANIM(ghlInfo,"G2API_GetBoltMatrix");

				if (!gG2_GBMUseSPMethod)
				{ //this is horribly stupid and I hate it. But lots of game code is written to assume this 90 degree offset thing.
					float ftemp;
					ftemp = matrix->matrix[0][0];
					matrix->matrix[0][0] = -matrix->matrix[0][1];
					matrix->matrix[0][1] = ftemp;

					ftemp = matrix->matrix[1][0];
					matrix->matrix[1][0] = -matrix->matrix[1][1];
					matrix->matrix[1][1] = ftemp;

					ftemp = matrix->matrix[2][0];
					matrix->matrix[2][0] = -matrix->matrix[2][1];
					matrix->matrix[2][1] = ftemp;
				}
				else
				{ //reset it
					gG2_GBMUseSPMethod = qfalse;
				}

				return qtrue;
			}
		}
	}
	else
	{
		G2WARNING(0,"G2API_GetBoltMatrix Failed on empty or bad model");
	}
	Multiply_3x4Matrix(matrix, &worldMatrix, (mdxaBone_t *)&identityMatrix);
	return qfalse;
}

void G2API_ListSurfaces(CGhoul2Info *ghlInfo)
{
	if (G2_SetupModelPointers(ghlInfo))
	{
		G2_List_Model_Surfaces(ghlInfo->mFileName);
	}
}

void G2API_ListBones(CGhoul2Info *ghlInfo, int frame)
{
	if (G2_SetupModelPointers(ghlInfo))
	{
		G2_List_Model_Bones(ghlInfo->mFileName, frame);
	}
}

// decide if we have Ghoul2 models associated with this ghoul list or not
qboolean G2API_HaveWeGhoul2Models(CGhoul2Info_v &ghoul2)
{
	for (int i=0; i<ghoul2.size();i++)
	{
		if (ghoul2[i].mModelindex != -1)
		{
			return qtrue;
		}
	}

	return qfalse;
}

// run through the Ghoul2 models and set each of the mModel values to the correct one from the cgs.gameModel offset lsit
void G2API_SetGhoul2ModelIndexes(CGhoul2Info_v &ghoul2, qhandle_t *modelList, qhandle_t *skinList)
{
}


char *G2API_GetAnimFileNameIndex(qhandle_t modelIndex)
{
	model_t		*mod_m = R_GetModelByHandle(modelIndex);
	return mod_m->mdxm->animName;
}

/************************************************************************************************
 * G2API_GetAnimFileName
 *    obtains the name of a model's .gla (animation) file
 *
 * Input
 *    pointer to list of CGhoul2Info's, WraithID of specific model in that list
 *
 * Output
 *    true if a good filename was obtained, false otherwise
 *
 ************************************************************************************************/
qboolean G2API_GetAnimFileName(CGhoul2Info *ghlInfo, char **filename)
{
	if (G2_SetupModelPointers(ghlInfo))
	{
		return G2_GetAnimFileName(ghlInfo->mFileName, filename);
	}
	return qfalse;
}

/*
=======================
SV_QsortEntityNumbers
=======================
*/
static int QDECL QsortDistance( const void *a, const void *b ) {
	const float	&ea = ((CollisionRecord_t*)a)->mDistance;
	const float	&eb = ((CollisionRecord_t*)b)->mDistance;

	if ( ea < eb ) {
		return -1;
	}
	return 1;
}

static inline bool G2_NeedRetransform(CGhoul2Info *g2, int frameNum)
{ //see if we need to do another transform
	size_t i = 0;
	bool needTrans = false;
	while (i < g2->mBlist.size())
	{
		float	time;
		boneInfo_t &bone = g2->mBlist[i];

		if (bone.pauseTime)
		{
			time = (bone.pauseTime - bone.startTime) / 50.0f;
		}
		else
		{
			time = (frameNum - bone.startTime) / 50.0f;
		}
		int newFrame = bone.startFrame + (time * bone.animSpeed);

		if (newFrame < bone.endFrame ||
			(bone.flags & BONE_ANIM_OVERRIDE_LOOP) ||
			(bone.flags & BONE_NEED_TRANSFORM))
		{ //ok, we're gonna have to do it. bone is apparently animating.
			bone.flags &= ~BONE_NEED_TRANSFORM;
			needTrans = true;
		}
		i++;
	}

	return needTrans;
}

void G2API_CollisionDetectCache(CollisionRecord_t *collRecMap, CGhoul2Info_v &ghoul2, const vec3_t angles, const vec3_t position,
										  int frameNumber, int entNum, vec3_t rayStart, vec3_t rayEnd, vec3_t scale, IHeapAllocator *G2VertSpace, int traceFlags, int useLod, float fRadius)
{ //this will store off the transformed verts for the next trace - this is slower, but for models that do not animate
	//frequently it is much much faster. -rww
#if 0 // UNUSED
	int *test = ghoul2[0].mTransformedVertsArray;
#endif
	if (G2_SetupModelPointers(ghoul2))
	{
		vec3_t	transRayStart, transRayEnd;

		int tframeNum=G2API_GetTime(frameNumber);
		// make sure we have transformed the whole skeletons for each model
		if (G2_NeedRetransform(&ghoul2[0], tframeNum) || !ghoul2[0].mTransformedVertsArray)
		{ //optimization, only create new transform space if we need to, otherwise
			//store it off!
			int i = 0;
			while (i < ghoul2.size())
			{
				CGhoul2Info &g2 = ghoul2[i];

				/*
				if ((g2.mFlags & GHOUL2_ZONETRANSALLOC) && g2.mTransformedVertsArray)
				{ //clear it out, yo.
					Z_Free(g2.mTransformedVertsArray);
					g2.mTransformedVertsArray = 0;
				}
				*/
				if (!g2.mTransformedVertsArray || !(g2.mFlags & GHOUL2_ZONETRANSALLOC))
				{ //reworked so we only alloc once!
					//if we have a pointer, but not a ghoul2_zonetransalloc flag, then that means
					//it is a miniheap pointer. Just stomp over it.
					int iSize = g2.currentModel->mdxm->numSurfaces * 4;
					g2.mTransformedVertsArray = (size_t *)Z_Malloc(iSize, TAG_GHOUL2, qtrue);
				}

				g2.mFlags |= GHOUL2_ZONETRANSALLOC;

				i++;
			}
			G2_ConstructGhoulSkeleton(ghoul2, frameNumber, true, scale);
			G2VertSpace->ResetHeap();

			// now having done that, time to build the model
#ifdef _G2_GORE
			G2_TransformModel(ghoul2, frameNumber, scale, G2VertSpace, useLod, false);
#else
			G2_TransformModel(ghoul2, frameNumber, scale, G2VertSpace, useLod);
#endif

			//don't need to do this anymore now that I am using a flag for zone alloc.
			/*
			i = 0;
			while (i < ghoul2.size())
			{
				CGhoul2Info &g2 = ghoul2[i];
				int iSize = g2.currentModel->mdxm->numSurfaces * 4;

				int *zoneMem = (int *)Z_Malloc(iSize, TAG_GHOUL2, qtrue);
				memcpy(zoneMem, g2.mTransformedVertsArray, iSize);
				g2.mTransformedVertsArray = zoneMem;
				g2.mFlags |= GHOUL2_ZONETRANSALLOC;
				i++;
			}
			*/
		}

		// pre generate the world matrix - used to transform the incoming ray
		G2_GenerateWorldMatrix(angles, position);

		// model is built. Lets check to see if any triangles are actually hit.
		// first up, translate the ray to model space
		TransformAndTranslatePoint(rayStart, transRayStart, &worldMatrixInv);
		TransformAndTranslatePoint(rayEnd, transRayEnd, &worldMatrixInv);

		// now walk each model and check the ray against each poly - sigh, this is SO expensive. I wish there was a better way to do this.
#ifdef _G2_GORE
		G2_TraceModels(ghoul2, transRayStart, transRayEnd, collRecMap, entNum, traceFlags, useLod, fRadius,0,0,0,0,0,qfalse);
#else
		G2_TraceModels(ghoul2, transRayStart, transRayEnd, collRecMap, entNum, traceFlags, useLod, fRadius);
#endif
		int i;
		for ( i = 0; i < MAX_G2_COLLISIONS && collRecMap[i].mEntityNum != -1; i ++ );

		// now sort the resulting array of collision records so they are distance ordered
		qsort( collRecMap, i,
			sizeof( CollisionRecord_t ), QsortDistance );
	}
}


void G2API_CollisionDetect(CollisionRecord_t *collRecMap, CGhoul2Info_v &ghoul2, const vec3_t angles, const vec3_t position,
										  int frameNumber, int entNum, vec3_t rayStart, vec3_t rayEnd, vec3_t scale, IHeapAllocator *G2VertSpace, int traceFlags, int useLod, float fRadius)
{
	/*
	if (1)
	{
		G2API_CollisionDetectCache(collRecMap, ghoul2, angles, position, frameNumber, entNum,
			rayStart, rayEnd, scale, G2VertSpace, traceFlags, useLod, fRadius);
		return;
	}
	*/

	if (G2_SetupModelPointers(ghoul2))
	{
		vec3_t	transRayStart, transRayEnd;

		// make sure we have transformed the whole skeletons for each model
		G2_ConstructGhoulSkeleton(ghoul2, frameNumber, true, scale);

		// pre generate the world matrix - used to transform the incoming ray
		G2_GenerateWorldMatrix(angles, position);

		G2VertSpace->ResetHeap();

		// now having done that, time to build the model
#ifdef _G2_GORE
		G2_TransformModel(ghoul2, frameNumber, scale, G2VertSpace, useLod, false);
#else
		G2_TransformModel(ghoul2, frameNumber, scale, G2VertSpace, useLod);
#endif

		// model is built. Lets check to see if any triangles are actually hit.
		// first up, translate the ray to model space
		TransformAndTranslatePoint(rayStart, transRayStart, &worldMatrixInv);
		TransformAndTranslatePoint(rayEnd, transRayEnd, &worldMatrixInv);

		// now walk each model and check the ray against each poly - sigh, this is SO expensive. I wish there was a better way to do this.
#ifdef _G2_GORE
		G2_TraceModels(ghoul2, transRayStart, transRayEnd, collRecMap, entNum, traceFlags, useLod, fRadius,0,0,0,0,0,qfalse);
#else
		G2_TraceModels(ghoul2, transRayStart, transRayEnd, collRecMap, entNum, traceFlags, useLod, fRadius);
#endif
		int i;
		for ( i = 0; i < MAX_G2_COLLISIONS && collRecMap[i].mEntityNum != -1; i ++ );

		// now sort the resulting array of collision records so they are distance ordered
		qsort( collRecMap, i,
			sizeof( CollisionRecord_t ), QsortDistance );
	}
}

qboolean G2API_SetGhoul2ModelFlags(CGhoul2Info *ghlInfo, const int flags)
{
	if (G2_SetupModelPointers(ghlInfo))
	{
		ghlInfo->mFlags &= GHOUL2_NEWORIGIN;
		ghlInfo->mFlags |= flags;
		return qtrue;
	}
	return qfalse;
}

int G2API_GetGhoul2ModelFlags(CGhoul2Info *ghlInfo)
{
	if (G2_SetupModelPointers(ghlInfo))
	{
		return (ghlInfo->mFlags & ~GHOUL2_NEWORIGIN);
	}
	return 0;
}

// given a boltmatrix, return in vec a normalised vector for the axis requested in flags
void G2API_GiveMeVectorFromMatrix(mdxaBone_t *boltMatrix, Eorientations flags, vec3_t vec)
{
	switch (flags)
	{
	case ORIGIN:
		vec[0] = boltMatrix->matrix[0][3];
		vec[1] = boltMatrix->matrix[1][3];
		vec[2] = boltMatrix->matrix[2][3];
		break;
	case POSITIVE_Y:
		vec[0] = boltMatrix->matrix[0][1];
		vec[1] = boltMatrix->matrix[1][1];
		vec[2] = boltMatrix->matrix[2][1];
 		break;
	case POSITIVE_X:
		vec[0] = boltMatrix->matrix[0][0];
		vec[1] = boltMatrix->matrix[1][0];
		vec[2] = boltMatrix->matrix[2][0];
		break;
	case POSITIVE_Z:
		vec[0] = boltMatrix->matrix[0][2];
		vec[1] = boltMatrix->matrix[1][2];
		vec[2] = boltMatrix->matrix[2][2];
		break;
	case NEGATIVE_Y:
		vec[0] = -boltMatrix->matrix[0][1];
		vec[1] = -boltMatrix->matrix[1][1];
		vec[2] = -boltMatrix->matrix[2][1];
		break;
	case NEGATIVE_X:
		vec[0] = -boltMatrix->matrix[0][0];
		vec[1] = -boltMatrix->matrix[1][0];
		vec[2] = -boltMatrix->matrix[2][0];
		break;
	case NEGATIVE_Z:
		vec[0] = -boltMatrix->matrix[0][2];
		vec[1] = -boltMatrix->matrix[1][2];
		vec[2] = -boltMatrix->matrix[2][2];
		break;
	}
}


int G2API_CopyGhoul2Instance(CGhoul2Info_v &g2From, CGhoul2Info_v &g2To, int modelIndex)
{
	assert(modelIndex==-1); // copy individual bolted parts is not used in jk2 and I didn't want to deal with it
							// if ya want it, we will add it back correctly

	//G2ERROR(ghoul2From.IsValid(),"Invalid ghlInfo");
	if (g2From.IsValid())
	{
#ifdef _DEBUG
		if (g2To.IsValid())
		{
			assert(!"Copying to a valid g2 instance?!");

			if (g2To[0].mBoneCache)
			{
				assert(!"Instance has a bonecache too.. it's gonna get stomped");
			}
		}
#endif
		g2To.DeepCopy(g2From);

#ifdef _G2_GORE //check through gore stuff then, as well.
		int model = 0;

		while (model < g2To.size())
		{
			if ( g2To[model].mGoreSetTag )
			{
				CGoreSet* gore = FindGoreSet ( g2To[model].mGoreSetTag );
				assert(gore);
				gore->mRefCount++;
			}

			model++;
		}
#endif
		//G2ANIM(ghoul2From,"G2API_CopyGhoul2Instance (source)");
		//G2ANIM(ghoul2To,"G2API_CopyGhoul2Instance (dest)");
	}

	return -1;
}

void G2API_CopySpecificG2Model(CGhoul2Info_v &ghoul2From, int modelFrom, CGhoul2Info_v &ghoul2To, int modelTo)
{
#if 0
	qboolean forceReconstruct = qtrue;
#endif //model1 was not getting reconstructed like it should for thrown sabers?
	   //might have been a bug in the reconstruct checking which has since been
	   //mangled and probably fixed. -rww

	// assume we actually have a model to copy from
	if (ghoul2From.size() > modelFrom)
	{
		// if we don't have enough models on the to side, resize us so we do
		if (ghoul2To.size() <= modelTo)
		{
			assert (modelTo < 5);
			ghoul2To.resize(modelTo + 1);
#if 0
			forceReconstruct = qtrue;
#endif
		}
		// do the copy

		if (ghoul2To.IsValid() && ghoul2To.size() >= modelTo)
		{ //remove the bonecache before we stomp over this instance.
			if (ghoul2To[modelTo].mBoneCache)
			{
				RemoveBoneCache(ghoul2To[modelTo].mBoneCache);
				ghoul2To[modelTo].mBoneCache = 0;
			}
		}
		ghoul2To[modelTo] = ghoul2From[modelFrom];

#if 0
		if (forceReconstruct)
		{ //rww - we should really do this shouldn't we? If we don't mark a reconstruct after this,
			//and we do a GetBoltMatrix in the same frame, it doesn't reconstruct the skeleton and returns
			//a completely invalid matrix
			ghoul2To[0].mSkelFrameNum = 0;
		}
#endif
	}
}

// This version will automatically copy everything about this model, and make a new one if necessary.
void G2API_DuplicateGhoul2Instance(CGhoul2Info_v &g2From, CGhoul2Info_v **g2To)
{
	//int ignore;

	if (*g2To)
	{	// This is bad.  We only want to do this if there is not yet a to declared.
		assert(0);
		return;
	}

	*g2To = new CGhoul2Info_v;
#ifdef _FULL_G2_LEAK_CHECKING
	if (g_G2AllocServer)
	{
		g_G2ServerAlloc += sizeof(CGhoul2Info_v);
	}
	else
	{
		g_G2ClientAlloc += sizeof(CGhoul2Info_v);
	}
	g_Ghoul2Allocations += sizeof(CGhoul2Info_v);
	G2_DEBUG_ShovePtrInTracker(*g2To);
#endif
	CGhoul2Info_v &ghoul2 = *(*g2To);

	/*ignore = */G2API_CopyGhoul2Instance(g2From, ghoul2, -1);

	return;
}

char *G2API_GetSurfaceName(CGhoul2Info_v& ghoul2, int modelIndex, int surfNumber)
{
	static char noSurface[1] = "";
	CGhoul2Info *ghlInfo = &ghoul2[modelIndex];

	if (G2_SetupModelPointers(ghlInfo))
	{
		model_t	*mod = (model_t *)ghlInfo->currentModel;
		mdxmSurface_t		*surf = 0;
		mdxmSurfHierarchy_t	*surfInfo = 0;

#ifndef FINAL_BUILD
		if (!mod || !mod->mdxm)
		{
			Com_Error(ERR_DROP, "G2API_GetSurfaceName: Bad model on instance %s.", ghlInfo->mFileName);
		}
#endif

		//ok, I guess it's semi-valid for the user to be passing in surface > numSurfs because they don't know how many surfs a model
		//may have.. but how did they get that surf index to begin with? Oh well.
		if (surfNumber < 0 || surfNumber >= mod->mdxm->numSurfaces)
		{
			ri.Printf( PRINT_ALL, "G2API_GetSurfaceName: You passed in an invalid surface number (%i) for model %s.\n", surfNumber, ghlInfo->mFileName);
			return noSurface;
		}


		surf = (mdxmSurface_t *)G2_FindSurface((void *)mod, surfNumber, 0);
		if (surf)
		{
#ifndef FINAL_BUILD
			if (surf->thisSurfaceIndex < 0 || surf->thisSurfaceIndex >= mod->mdxm->numSurfaces)
			{
				Com_Error(ERR_DROP, "G2API_GetSurfaceName: Bad surf num (%i) on surf for instance %s.", surf->thisSurfaceIndex, ghlInfo->mFileName);
			}
#endif
			mdxmHierarchyOffsets_t	*surfIndexes = (mdxmHierarchyOffsets_t *)((byte *)mod->mdxm + sizeof(mdxmHeader_t));
			surfInfo = (mdxmSurfHierarchy_t *)((byte *)surfIndexes + surfIndexes->offsets[surf->thisSurfaceIndex]);
			return surfInfo->name;
		}
	}
	return noSurface;
}


int	G2API_GetSurfaceIndex(CGhoul2Info *ghlInfo, const char *surfaceName)
{
	if (G2_SetupModelPointers(ghlInfo))
	{
		return G2_GetSurfaceIndex(ghlInfo, surfaceName);
	}
	return -1;
}

char *G2API_GetGLAName(CGhoul2Info_v &ghoul2, int modelIndex)
{
	if (G2_SetupModelPointers(ghoul2))
	{
		if (ghoul2.size() > modelIndex)
		{
			//model_t	*mod = R_GetModelByHandle(RE_RegisterModel(ghoul2[modelIndex].mFileName));
			//return mod->mdxm->animName;

			assert(ghoul2[modelIndex].currentModel && ghoul2[modelIndex].currentModel->mdxm);
			return ghoul2[modelIndex].currentModel->mdxm->animName;
		}
	}
	return NULL;
}

qboolean G2API_SetNewOrigin(CGhoul2Info_v &ghoul2, const int boltIndex)
{
	CGhoul2Info *ghlInfo = NULL;

	if (ghoul2.size()>0)
	{
		ghlInfo = &ghoul2[0];
	}

	if (G2_SetupModelPointers(ghlInfo))
	{
		if (boltIndex < 0)
		{
            char modelName[MAX_QPATH];
			if (ghlInfo->currentModel &&
				ghlInfo->currentModel->name[0])
			{
				strcpy(modelName, ghlInfo->currentModel->name);
			}
			else
			{
				strcpy(modelName, "None?!");
			}

			Com_Error(ERR_DROP, "Bad boltindex (%i) trying to SetNewOrigin (naughty naughty!)\nModel %s\n", boltIndex, modelName);
		}

		ghlInfo->mNewOrigin = boltIndex;
		ghlInfo->mFlags |= GHOUL2_NEWORIGIN;
		return qtrue;
	}
	return qfalse;
}

int G2API_GetBoneIndex(CGhoul2Info *ghlInfo, const char *boneName)
{
	if (G2_SetupModelPointers(ghlInfo))
	{
		return G2_Get_Bone_Index(ghlInfo, boneName);
	}
	return -1;
}

qboolean G2API_SaveGhoul2Models(CGhoul2Info_v &ghoul2, char **buffer, int *size)
{
	return G2_SaveGhoul2Models(ghoul2, buffer, size);
}

void G2API_LoadGhoul2Models(CGhoul2Info_v &ghoul2, char *buffer)
{
	G2_LoadGhoul2Model(ghoul2, buffer);
}

void G2API_FreeSaveBuffer(char *buffer)
{
	Z_Free(buffer);
}

// this is kinda sad, but I need to call the destructor in this module (exe), not the game.dll...
//
void G2API_LoadSaveCodeDestructGhoul2Info(CGhoul2Info_v &ghoul2)
{

#ifdef _G2_GORE
	G2API_ClearSkinGore ( ghoul2 );
#endif
	ghoul2.~CGhoul2Info_v();	// so I can load junk over it then memset to 0 without orphaning
}

//see if surfs have any shader info...
qboolean G2API_SkinlessModel(CGhoul2Info_v& ghoul2, int modelIndex)
{
	CGhoul2Info *g2 = &ghoul2[modelIndex];

	if (G2_SetupModelPointers(g2))
	{
		model_t	*mod = (model_t *)g2->currentModel;

		if (mod &&
			mod->mdxm)
		{
            mdxmSurfHierarchy_t	*surf;
			int i;

			surf = (mdxmSurfHierarchy_t *) ( (byte *)mod->mdxm + mod->mdxm->ofsSurfHierarchy );

			for (i = 0; i < mod->mdxm->numSurfaces; i++)
			{
				if (surf->shader[0])
				{ //found a surface with a shader name, ok.
                    return qfalse;
				}

  				surf = (mdxmSurfHierarchy_t *)( (byte *)surf + (intptr_t)( &((mdxmSurfHierarchy_t *)0)->childIndexes[ surf->numChildren ] ));
			}
		}
	}

	//found nothing.
	return qtrue;
}

//#ifdef _SOF2
#ifdef _G2_GORE
void ResetGoreTag(); // put here to reduce coupling

//way of seeing how many marks are on a model currently -rww
int G2API_GetNumGoreMarks(CGhoul2Info_v& ghoul2, int modelIndex)
{
	CGhoul2Info *g2 = &ghoul2[modelIndex];

	if (g2->mGoreSetTag)
	{
		CGoreSet *goreSet = FindGoreSet(g2->mGoreSetTag);

		if (goreSet)
		{
			return goreSet->mGoreRecords.size();
		}
	}

	return 0;
}

void G2API_ClearSkinGore ( CGhoul2Info_v &ghoul2 )
{
	int i;

	for (i=0; i<ghoul2.size(); i++)
	{
		if ( ghoul2[i].mGoreSetTag )
		{
			DeleteGoreSet ( ghoul2[i].mGoreSetTag );
			ghoul2[i].mGoreSetTag = 0;
		}
	}
}

int G2API_Ghoul2Size ( CGhoul2Info_v &ghoul2 )
{
	return ghoul2.size();
}

extern int		G2_DecideTraceLod(CGhoul2Info &ghoul2, int useLod);
void G2API_AddSkinGore(CGhoul2Info_v &ghoul2,SSkinGoreData &gore)
{
	if (VectorLength(gore.rayDirection)<.1f)
	{
		assert(0); // can't add gore without a shot direction
		return;
	}

	// make sure we have transformed the whole skeletons for each model
	//G2_ConstructGhoulSkeleton(ghoul2, gore.currentTime, NULL, true, gore.angles, gore.position, gore.scale, false);
	G2_ConstructGhoulSkeleton(ghoul2, gore.currentTime, true, gore.scale);

	// pre generate the world matrix - used to transform the incoming ray
	G2_GenerateWorldMatrix(gore.angles, gore.position);

	// first up, translate the ray to model space
	vec3_t	transRayDirection, transHitLocation;
	TransformAndTranslatePoint(gore.hitLocation, transHitLocation, &worldMatrixInv);
	TransformPoint(gore.rayDirection, transRayDirection, &worldMatrixInv);

	int lod;
	ResetGoreTag();
	const int lodbias=Com_Clamp ( 0, 2,G2_DecideTraceLod(ghoul2[0], ri.Cvar_VariableIntegerValue( "r_lodbias" )));
	const int maxLod =Com_Clamp (0,ghoul2[0].currentModel->numLods,3);	//limit to the number of lods the main model has
	for(lod=lodbias;lod<maxLod;lod++)
	{
		// now having done that, time to build the model
		ri.GetG2VertSpaceServer()->ResetHeap();

		G2_TransformModel(ghoul2, gore.currentTime, gore.scale,ri.GetG2VertSpaceServer(),lod,true);

		// now walk each model and compute new texture coordinates
		G2_TraceModels(ghoul2, transHitLocation, transRayDirection, 0, gore.entNum, 0,lod,0.0f,gore.SSize,gore.TSize,gore.theta,gore.shader,&gore,qtrue);
	}
}
#endif

qboolean G2_TestModelPointers(CGhoul2Info *ghlInfo) // returns true if the model is properly set up
{
	G2ERROR(ghlInfo,"NULL ghlInfo");
	if (!ghlInfo)
	{
		return qfalse;
	}
	ghlInfo->mValid=false;
	if (ghlInfo->mModelindex != -1)
	{
		if (ri.Cvar_VariableIntegerValue( "dedicated" ) ||
			(G2_ShouldRegisterServer())) //supreme hackery!
		{
			ghlInfo->mModel = RE_RegisterServerModel(ghlInfo->mFileName);
		}
		else
		{
			ghlInfo->mModel = RE_RegisterModel(ghlInfo->mFileName);
		}
		ghlInfo->currentModel = R_GetModelByHandle(ghlInfo->mModel);
		if (ghlInfo->currentModel)
		{
			if (ghlInfo->currentModel->mdxm)
			{
				if (ghlInfo->currentModelSize)
				{
					if (ghlInfo->currentModelSize!=ghlInfo->currentModel->mdxm->ofsEnd)
					{
						Com_Error(ERR_DROP, "Ghoul2 model was reloaded and has changed, map must be restarted.\n");
					}
				}
				ghlInfo->currentModelSize=ghlInfo->currentModel->mdxm->ofsEnd;
				ghlInfo->animModel = R_GetModelByHandle(ghlInfo->currentModel->mdxm->animIndex);
				if (ghlInfo->animModel)
				{
					ghlInfo->aHeader =ghlInfo->animModel->mdxa;
					if (ghlInfo->aHeader)
					{
						if (ghlInfo->currentAnimModelSize)
						{
							if (ghlInfo->currentAnimModelSize!=ghlInfo->aHeader->ofsEnd)
							{
								Com_Error(ERR_DROP, "Ghoul2 model was reloaded and has changed, map must be restarted.\n");
							}
						}
						ghlInfo->currentAnimModelSize=ghlInfo->aHeader->ofsEnd;
						ghlInfo->mValid=true;
					}
				}
			}
		}
	}
	if (!ghlInfo->mValid)
	{
		ghlInfo->currentModel=0;
		ghlInfo->currentModelSize=0;
		ghlInfo->animModel=0;
		ghlInfo->currentAnimModelSize=0;
		ghlInfo->aHeader=0;
	}
	return (qboolean)ghlInfo->mValid;
}

#ifdef G2_PERFORMANCE_ANALYSIS
#include "qcommon/timing.h"
extern timing_c G2PerformanceTimer_G2_SetupModelPointers;
extern int G2Time_G2_SetupModelPointers;
#endif

qboolean G2_SetupModelPointers(CGhoul2Info *ghlInfo) // returns true if the model is properly set up
{
#ifdef G2_PERFORMANCE_ANALYSIS
	G2PerformanceTimer_G2_SetupModelPointers.Start();
#endif
	G2ERROR(ghlInfo,"NULL ghlInfo");
	if (!ghlInfo)
	{
		return qfalse;
	}

//	if (ghlInfo->mValid && ghlInfo->currentModel)
	if (0)
	{ //rww - Why are we bothering with all this? We can't change models like this anyway.
	  //This function goes over 200k on the precision timer (in debug, but still), so I'm
	  //cutting it off here because it gets called constantly.
#ifdef G2_PERFORMANCE_ANALYSIS
		G2Time_G2_SetupModelPointers += G2PerformanceTimer_G2_SetupModelPointers.End();
#endif
		return qtrue;
	}

	ghlInfo->mValid=false;

//	G2WARNING(ghlInfo->mModelindex != -1,"Setup request on non-used info slot?");
	if (ghlInfo->mModelindex != -1)
	{
		G2ERROR(ghlInfo->mFileName[0],"empty ghlInfo->mFileName");

		// RJ - experimental optimization!
		if (!ghlInfo->mModel || 1)
		{
			if (ri.Cvar_VariableIntegerValue( "dedicated" ) ||
				(G2_ShouldRegisterServer())) //supreme hackery!
			{
				ghlInfo->mModel = RE_RegisterServerModel(ghlInfo->mFileName);
			}
			else
			{
				ghlInfo->mModel = RE_RegisterModel(ghlInfo->mFileName);
			}
			ghlInfo->currentModel = R_GetModelByHandle(ghlInfo->mModel);
		}

		G2ERROR(ghlInfo->currentModel,va("NULL Model (glm) %s",ghlInfo->mFileName));
		if (ghlInfo->currentModel)
		{
			G2ERROR(ghlInfo->currentModel->mdxm,va("Model has no mdxm (glm) %s",ghlInfo->mFileName));
			if (ghlInfo->currentModel->mdxm)
			{
				if (ghlInfo->currentModelSize)
				{
					if (ghlInfo->currentModelSize!=ghlInfo->currentModel->mdxm->ofsEnd)
					{
						Com_Error(ERR_DROP, "Ghoul2 model was reloaded and has changed, map must be restarted.\n");
					}
				}
				ghlInfo->currentModelSize=ghlInfo->currentModel->mdxm->ofsEnd;
				G2ERROR(ghlInfo->currentModelSize,va("Zero sized Model? (glm) %s",ghlInfo->mFileName));

				ghlInfo->animModel = R_GetModelByHandle(ghlInfo->currentModel->mdxm->animIndex);
				G2ERROR(ghlInfo->animModel,va("NULL Model (gla) %s",ghlInfo->mFileName));
				if (ghlInfo->animModel)
				{
					ghlInfo->aHeader =ghlInfo->animModel->mdxa;
					G2ERROR(ghlInfo->aHeader,va("Model has no mdxa (gla) %s",ghlInfo->mFileName));
					if (ghlInfo->aHeader)
					{
						if (ghlInfo->currentAnimModelSize)
						{
							if (ghlInfo->currentAnimModelSize!=ghlInfo->aHeader->ofsEnd)
							{
								Com_Error(ERR_DROP, "Ghoul2 model was reloaded and has changed, map must be restarted.\n");
							}
						}
						ghlInfo->currentAnimModelSize=ghlInfo->aHeader->ofsEnd;
						G2ERROR(ghlInfo->currentAnimModelSize,va("Zero sized Model? (gla) %s",ghlInfo->mFileName));
						ghlInfo->mValid=true;
					}
				}
			}
		}
	}
	if (!ghlInfo->mValid)
	{
		ghlInfo->currentModel=0;
		ghlInfo->currentModelSize=0;
		ghlInfo->animModel=0;
		ghlInfo->currentAnimModelSize=0;
		ghlInfo->aHeader=0;
	}

#ifdef G2_PERFORMANCE_ANALYSIS
	G2Time_G2_SetupModelPointers += G2PerformanceTimer_G2_SetupModelPointers.End();
#endif
	return (qboolean)ghlInfo->mValid;
}

qboolean G2_SetupModelPointers(CGhoul2Info_v &ghoul2) // returns true if any model is properly set up
{
	bool ret=false;
	int i;
	for (i=0; i<ghoul2.size(); i++)
	{
		qboolean r = G2_SetupModelPointers(&ghoul2[i]);
		ret=ret||r;
	}
	return (qboolean)ret;
}

qboolean G2API_IsGhoul2InfovValid (CGhoul2Info_v& ghoul2)
{
	return (qboolean)ghoul2.IsValid();
}

const char *G2API_GetModelName ( CGhoul2Info_v& ghoul2, int modelIndex )
{
	return ghoul2[modelIndex].mFileName;
}
