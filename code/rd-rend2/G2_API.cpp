#include "server/exe_headers.h"

#include "ghoul2/G2.h"
#include "ghoul2/ghoul2_gore.h" // ragdoll
#include "qcommon/q_shared.h"
#include "qcommon/MiniHeap.h"
#include "rd-common/tr_common.h"
#include "tr_local.h"
#include <set>

#ifdef _MSC_VER
	#pragma warning (push, 3)	//go back down to 3 for the stl include
#endif
#include <list>
#ifdef _MSC_VER
	#pragma warning (pop)
#endif

extern mdxaBone_t worldMatrix;
extern mdxaBone_t worldMatrixInv;

#define NUM_G2T_TIME (2)
#ifndef REND2_SP
// must be a power of two
#define MAX_G2_MODELS (1024)
#define G2_MODEL_BITS (10)
#else
#define MAX_G2_MODELS (512)
#define G2_MODEL_BITS (9)
#endif
#define G2_INDEX_MASK (MAX_G2_MODELS-1)
// rww - RAGDOLL_BEGIN
#define GHOUL2_RAG_STARTED						0x0010
#define GHOUL2_RAG_FORCESOLVE					0x1000		//api-override, determine if ragdoll should be forced to continue solving even if it thinks it is settled
// rww - RAGDOLL_END

#if G2API_DEBUG

#define G2ERROR(exp,m) (void)( (exp) || (G2APIError().Error(m,0,__FILE__,__LINE__), 0) )
#define G2WARNING(exp,m) (void)( (exp) || (G2APIError().Error(m,1,__FILE__,__LINE__), 0) )
#define G2NOTE(exp,m) (void)( (exp) || (G2APIError().Error(m,2,__FILE__,__LINE__), 0) )
#define G2ANIM(ghlInfo,m) (void)((G2APIError().AnimTest(ghlInfo,m,__FILE__,__LINE__), 0) )

#else

#define G2ERROR(exp,m) ((void)0)
#define G2WARNING(exp,m) ((void)0)
#define G2NOTE(exp,m) ((void)0)
#define G2ANIM(ghlInfo,m) ((void)0)

#endif

// get/set time
static int G2TimeBases[NUM_G2T_TIME];

// used in tr_init.cpp
qboolean gG2_GBMNoReconstruct;
qboolean gG2_GBMUseSPMethod;

// Local functions pre-definition
qboolean G2_TestModelPointers(CGhoul2Info* ghlInfo);
void CL_InitRef(void);

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
		Com_Printf("g2 leak tracker was never initialized!\n");
		return;
	}

    while (i < MAX_TRACKED_ALLOC)
	{
		if (g_G2AllocTrack[i])
		{
			Com_Printf("Bad guy found in slot %i, attempting to access...", i);
			CGhoul2Info_v &g2v = *g_G2AllocTrack[i];
			CGhoul2Info &g2 = g2v[0];

			if (g2v.IsValid() && g2.mFileName && g2.mFileName[0])
			{
				Com_Printf("Bad guy's filename is %s\n", g2.mFileName);
			}
			else
			{
				Com_Printf("He's not valid! BURN HIM!\n");
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
		Com_Printf("%s could not be fit into g2 debug instance tracker.\n", g2v[0].currentModel->name);
	}
	else
	{
		Com_Printf("Crap g2 instance passed to instance tracker (in).\n");
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
		Com_Printf("%s not in g2 debug instance tracker.\n", g2v[0].currentModel->name);
	}
	else
	{
		Com_Printf("Crap g2 instance passed to instance tracker (out).\n");
	}
}
#endif

// RAGDOLL_BEGIN
void G2API_SetTime(int currentTime,int clock)
{
	assert(clock>=0&&clock<NUM_G2T_TIME);
#if G2_DEBUG_TIME
	Com_Printf("Set Time: before c%6d  s%6d",G2TimeBases[1],G2TimeBases[0]);
#endif
	G2TimeBases[clock]=currentTime;
	if (G2TimeBases[1]>G2TimeBases[0]+200)
	{
		G2TimeBases[1]=0; // use server time instead
		return;
	}
#if G2_DEBUG_TIME
	Com_Printf(" after c%6d  s%6d\n",G2TimeBases[1],G2TimeBases[0]);
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

void RemoveBoneCache(CBoneCache* boneCache);

static size_t GetSizeOfGhoul2Info ( const CGhoul2Info& g2Info )
{
	size_t size = 0;
	
	// This is pretty ugly, but we don't want to save everything in the CGhoul2Info object.
	size += offsetof (CGhoul2Info, mTransformedVertsArray) - offsetof (CGhoul2Info, mModelindex);

	// Surface vector + size
	size += sizeof (int);
	size += g2Info.mSlist.size() * sizeof (surfaceInfo_t);

	// Bone vector + size
	size += sizeof (int);
	size += g2Info.mBlist.size() * sizeof (boneInfo_t);

	// Bolt vector + size
	size += sizeof (int);
	size += g2Info.mBltlist.size() * sizeof (boltInfo_t);

	return size;
}

static size_t SerializeGhoul2Info ( char *buffer, const CGhoul2Info& g2Info )
{
	char *base = buffer;
	size_t blockSize;
	
	// Oh the ugliness...
	blockSize = offsetof (CGhoul2Info, mTransformedVertsArray) - offsetof (CGhoul2Info, mModelindex);
	memcpy (buffer, &g2Info.mModelindex, blockSize);
	buffer += blockSize;

	// Surfaces vector + size
	*(int *)buffer = g2Info.mSlist.size();
	buffer += sizeof (int);

	blockSize = g2Info.mSlist.size() * sizeof (surfaceInfo_t);
	memcpy (buffer, g2Info.mSlist.data(), g2Info.mSlist.size() * sizeof (surfaceInfo_t));
	buffer += blockSize;

	// Bones vector + size
	*(int *)buffer = g2Info.mBlist.size();
	buffer += sizeof (int);
	
	blockSize = g2Info.mBlist.size() * sizeof (boneInfo_t);
	memcpy (buffer, g2Info.mBlist.data(), g2Info.mBlist.size() * sizeof (boneInfo_t));
	buffer += blockSize;

	// Bolts vector + size
	*(int *)buffer = g2Info.mBltlist.size();
	buffer += sizeof (int);
	
	blockSize = g2Info.mBltlist.size() * sizeof (boltInfo_t);
	memcpy (buffer, g2Info.mBltlist.data(), g2Info.mBltlist.size() * sizeof (boltInfo_t));
	buffer += blockSize;

	return static_cast<size_t>(buffer - base);
}

static size_t DeserializeGhoul2Info ( const char *buffer, CGhoul2Info& g2Info )
{
	const char *base = buffer;
	size_t size;

	size = offsetof (CGhoul2Info, mTransformedVertsArray) - offsetof (CGhoul2Info, mModelindex);
	memcpy (&g2Info.mModelindex, buffer, size);
	buffer += size;

	// Surfaces vector
	size = *(int *)buffer;
	buffer += sizeof (int);

	g2Info.mSlist.assign ((surfaceInfo_t *)buffer, (surfaceInfo_t *)buffer + size);
	buffer += sizeof (surfaceInfo_t) * size;

	// Bones vector
	size = *(int *)buffer;
	buffer += sizeof (int);

	g2Info.mBlist.assign ((boneInfo_t *)buffer, (boneInfo_t *)buffer + size);
	buffer += sizeof (boneInfo_t) * size;

	// Bolt vector
	size = *(int *)buffer;
	buffer += sizeof (int);

	g2Info.mBltlist.assign ((boltInfo_t *)buffer, (boltInfo_t *)buffer + size);
	buffer += sizeof (boltInfo_t) * size;

	return static_cast<size_t>(buffer - base);
}

class Ghoul2InfoArray : public IGhoul2InfoArray
{
	std::vector<CGhoul2Info> mInfos[MAX_G2_MODELS];
	int mIds[MAX_G2_MODELS];
	std::list<int> mFreeIndecies;
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

	size_t GetSerializedSize() const
	{
		size_t size = 0;	

		size += sizeof (int); // size of mFreeIndecies linked list
		size += mFreeIndecies.size() * sizeof (int);

		size += sizeof (mIds);

		for ( size_t i = 0; i < MAX_G2_MODELS; i++ )
		{
			size += sizeof (int); // size of the mInfos[i] vector

			for ( size_t j = 0; j < mInfos[i].size(); j++ )
			{
				size += GetSizeOfGhoul2Info (mInfos[i][j]);
			}
		}

		return size;
	}

	size_t Serialize ( char *buffer ) const
	{
		char *base = buffer;

		// Free indices
		*(int *)buffer = mFreeIndecies.size();
		buffer += sizeof (int);

		std::copy (mFreeIndecies.begin(), mFreeIndecies.end(), (int *)buffer);
		buffer += sizeof (int) * mFreeIndecies.size();

		// IDs
		memcpy (buffer, mIds, sizeof (mIds));
		buffer += sizeof (mIds);

		// Ghoul2 infos
		for ( size_t i = 0; i < MAX_G2_MODELS; i++ )
		{
			*(int *)buffer = mInfos[i].size();
			buffer += sizeof (int);

			for ( size_t j = 0; j < mInfos[i].size(); j++ )
			{
				buffer += SerializeGhoul2Info (buffer, mInfos[i][j]);
			}
		}

		return static_cast<size_t>(buffer - base);
	}

	size_t Deserialize ( const char *buffer, size_t size )
	{
		const char *base = buffer;
		size_t count;

		// Free indices
		count = *(int *)buffer;
		buffer += sizeof (int);

		mFreeIndecies.assign ((int *)buffer, (int *)buffer + count);
		buffer += sizeof (int) * count;

		// IDs
		memcpy (mIds, buffer, sizeof (mIds));
		buffer += sizeof (mIds);

		// Ghoul2 infos
		for ( size_t i = 0; i < MAX_G2_MODELS; i++ )
		{
			mInfos[i].clear();

			count = *(int *)buffer;
			buffer += sizeof (int);

			mInfos[i].resize (count);
			
			for ( size_t j = 0; j < count; j++ )
			{
				buffer += DeserializeGhoul2Info (buffer, mInfos[i][j]);
			}
		}

		return static_cast<size_t>(buffer - base);
	}

	~Ghoul2InfoArray()
	{
		if (mFreeIndecies.size() < MAX_G2_MODELS)
		{
#if G2API_DEBUG
			char mess[1000];
			sprintf(mess, "************************\nLeaked %ld ghoul2info slots\n", MAX_G2_MODELS - mFreeIndecies.size());
			Com_DPrintf(mess);
#endif
			for (int i = 0; i < MAX_G2_MODELS; i++)
			{
				std::list<int>::iterator j;
				for (j = mFreeIndecies.begin(); j != mFreeIndecies.end(); ++j)
				{
					if (*j == i)
						break;
				}
				if (j == mFreeIndecies.end())
				{
#if G2API_DEBUG
					sprintf(mess, "Leaked Info idx=%d id=%d sz=%ld\n", i, mIds[i], mInfos[i].size());
					Com_DPrintf(mess);
					if (mInfos[i].size())
					{
						sprintf(mess, "%s\n", mInfos[i][0].mFileName);
						Com_DPrintf(mess);
					}
#endif
					DeleteLow(i);
				}
			}
		}
#if G2API_DEBUG
		else
		{
			Com_DPrintf("No ghoul2 info slots leaked\n");
		}
#endif
	}

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
		assert(handle > 0); //null handle
		assert((handle & G2_INDEX_MASK) >= 0 && (handle & G2_INDEX_MASK) < MAX_G2_MODELS); //junk handle
		assert(mIds[handle & G2_INDEX_MASK] == handle); // not a valid handle, could be old or garbage
		if (mIds[handle & G2_INDEX_MASK] == handle)
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
	std::vector<CGhoul2Info>& GetDebug(int handle)
	{
		assert(!(handle <= 0 || (handle & G2_INDEX_MASK) < 0 || (handle & G2_INDEX_MASK) >= MAX_G2_MODELS || mIds[handle & G2_INDEX_MASK] != handle));

		return mInfos[handle & G2_INDEX_MASK];
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

#if G2API_DEBUG
std::vector<CGhoul2Info>& DebugG2Info(int handle)
{
	return ((Ghoul2InfoArray*)(&TheGhoul2InfoArray()))->GetDebug(handle);
}

CGhoul2Info& DebugG2InfoI(int handle, int item)
{
	return ((Ghoul2InfoArray*)(&TheGhoul2InfoArray()))->GetDebug(handle)[item];
}

void TestAllGhoul2Anims()
{
	((Ghoul2InfoArray*)(&TheGhoul2InfoArray()))->TestAllAnims();
}
#endif

#define PERSISTENT_G2DATA "g2infoarray"

void RestoreGhoul2InfoArray()
{
	if (singleton == NULL)
	{
		// Create the ghoul2 info array
		TheGhoul2InfoArray();

		size_t size;
		const void *data = ri.PD_Load (PERSISTENT_G2DATA, &size);
		if ( data == NULL )
		{
			return;
		}

#ifdef _DEBUG
		size_t read =
#endif // _DEBUG
			singleton->Deserialize((const char*)data, size);
		Z_Free((void*)data);
#ifdef _DEBUG
		assert(read == size);
#endif
	}
}

void SaveGhoul2InfoArray()
{
	size_t size = singleton->GetSerializedSize();
	void *data = R_Malloc (size, TAG_GHOUL2, qfalse);
	size_t written = singleton->Serialize ((char *)data);
#ifdef _DEBUG
	assert(written == size);
#endif // _DEBUG

	if ( !ri.PD_Store (PERSISTENT_G2DATA, data, size) )
	{
		Com_Printf (S_COLOR_RED "ERROR: Failed to store persistent renderer data.\n");
	}
}

void Ghoul2InfoArray_Free(void)
{
	if(singleton) {
		delete singleton;
		singleton = NULL;
	}
}

// this is the ONLY function to read entity states directly
void G2API_CleanGhoul2Models(CGhoul2Info_v& ghoul2)
{
#ifdef _G2_GORE
	G2API_ClearSkinGore(ghoul2);
#endif
	ghoul2.~CGhoul2Info_v();
}

qhandle_t G2API_PrecacheGhoul2Model(const char *fileName)
{
	return RE_RegisterModel(fileName);
}

// initialise all that needs to be on a new Ghoul II model
int G2API_InitGhoul2Model(CGhoul2Info_v& ghoul2, const char* fileName, int modelIndex, qhandle_t customSkin, qhandle_t customShader, int modelFlags, int lodBias)
{
	int model = -1;

	G2ERROR(fileName && fileName[0], "NULL filename");

	// are we actually asking for a model to be loaded.
	if (!fileName || !fileName[0])
	{
		assert(0);
		return -1;
	}

	// find a free spot in the list
	for (model=0; model< ghoul2.size(); model++)
	{
		if (ghoul2[model].mModelindex == -1)
		{
			ghoul2[model]=CGhoul2Info();
			break;
		}
	}
	if (model == ghoul2.size())
	{
		assert(model < 8);	//arb, just catching run-away models (why 4 instead of 8 in MP?)
		CGhoul2Info info;
		Q_strncpyz(info.mFileName, fileName, sizeof(info.mFileName));
		info.mModelindex = 0;

		if (G2_TestModelPointers(&info))
		{
			ghoul2.push_back(CGhoul2Info());
		}
		else
		{
			return -1;
		}
	}

	Q_strncpyz(ghoul2[model].mFileName, fileName, sizeof(ghoul2[model].mFileName));

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
	G2ERROR(ghlInfo, "G2API_SetLodBias: NULL ghlInfo");
	if (G2_SetupModelPointers(ghlInfo))
	{
		ghlInfo->mLodBias = lodBias;
		return qtrue;
	}
	return qfalse;
}

qboolean G2API_SetSkin(CGhoul2Info* ghlInfo, qhandle_t customSkin, qhandle_t renderSkin)
{
	G2ERROR(ghlInfo, "G2API_SetSkin: NULL ghlInfo");
	if (G2_SetupModelPointers(ghlInfo))
	{
		ghlInfo->mCustomSkin = customSkin;
#ifndef JK2_MODE
		if (renderSkin)
		{	
			// this is going to set the surfs on/off matching the skin file
			// header moved to tr_local.h, implemented in G2_surface.cpp
			G2_SetSurfaceOnOffFromSkin( ghlInfo, renderSkin );
		}
#endif
		return qtrue;
	}
	return qfalse;
}

qboolean G2API_SetShader(CGhoul2Info *ghlInfo, qhandle_t customShader)
{
	G2ERROR(ghlInfo, "G2API_SetShader: NULL ghlInfo");
	if (ghlInfo)
	{
		ghlInfo->mCustomShader = customShader;
		return qtrue;
	}
	return qfalse;
}

qboolean G2API_SetSurfaceOnOff(CGhoul2Info* ghlInfo, const char* surfaceName, const int flags)
{
	G2ERROR(ghlInfo, "G2API_SetSurfaceOnOff: NULL ghlInfo");
	if (G2_SetupModelPointers(ghlInfo))
	{
		G2ERROR(!(flags & ~(G2SURFACEFLAG_OFF | G2SURFACEFLAG_NODESCENDANTS)), "G2API_SetSurfaceOnOff Illegal Flags");
		// ensure we flush the cache
		ghlInfo->mMeshFrameNum = 0;
		return G2_SetSurfaceOnOff(ghlInfo, surfaceName, flags);
	}
	return qfalse;
}

qboolean G2API_SetRootSurface(CGhoul2Info_v &ghoul2, const int modelIndex, const char *surfaceName)
{
	G2ERROR(ghoul2.IsValid(), "G2API_SetRootSurface: Invalid ghlInfo");
	G2ERROR(surfaceName, "G2API_SetRootSurface: Invalid surfaceName");

	if (G2_SetupModelPointers(ghoul2))
	{
		G2ERROR(modelIndex >= 0 && modelIndex < ghlInfo.size(), "Bad Model Index");
		if (modelIndex >= 0 && modelIndex < ghoul2.size())
		{
			return G2_SetRootSurface(ghoul2, modelIndex, surfaceName);
		}
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

int G2API_GetSurfaceRenderStatus(CGhoul2Info* ghlInfo, const char* surfaceName)
{
	G2ERROR(surfaceName, "G2API_GetSurfaceRenderStatus: Invalid surfaceName");
	if (G2_SetupModelPointers(ghlInfo))
	{
		return G2_IsSurfaceRendered(ghlInfo, surfaceName, ghlInfo->mSlist);
	}
	return -1;
}

qboolean G2API_RemoveGhoul2Model(CGhoul2Info_v& ghlInfo, const int modelIndex)
{
	// sanity check
	if (!ghlInfo.size() || (ghlInfo.size() <= modelIndex) || modelIndex < 0 || (ghlInfo[modelIndex].mModelindex < 0))
	{
		// if we hit this assert then we are trying to delete a ghoul2 model on a ghoul2 instance that
		// one way or another is already gone.
		assert(0);
		return qfalse;
	}

	if (modelIndex < ghlInfo.size())
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
		ghlInfo[modelIndex].mFileName[0] = 0;
		ghlInfo[modelIndex] = CGhoul2Info();

		int newSize = ghlInfo.size();

		// ! the part below doesn't exist in SP Vanilla, but it makes sence !

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
	}


	return qtrue;
}

int G2API_GetAnimIndex(CGhoul2Info* ghlInfo)
{
	return (ghlInfo)
		? ghlInfo->animModelIndexOffset
		: 0;
}

qboolean G2API_SetAnimIndex(CGhoul2Info* ghlInfo, const int index)
{
	// Is This A Valid G2 Model?
	if (ghlInfo)
	{
		// Is This A New Anim Index?
		if (ghlInfo->animModelIndexOffset != index)
		{
			ghlInfo->animModelIndexOffset = index;
			// Clear anim size so SetupModelPointers recalcs
			ghlInfo->currentAnimModelSize = 0;

			// Kill All Existing Animation, Blending, Etc.
			//---------------------------------------------
			for (size_t index = 0; index < ghlInfo->mBlist.size(); index++)
			{
				ghlInfo->mBlist[index].flags &= ~(BONE_ANIM_TOTAL);
				ghlInfo->mBlist[index].flags &= ~(BONE_ANGLES_TOTAL);
			}
		}
		return qtrue;
	}
	return qfalse;
}

//check if a bone exists on skeleton without actually adding to the bone list -rww
qboolean G2API_DoesBoneExist(CGhoul2Info_v& ghoul2, int modelIndex, const char *boneName)
{
	CGhoul2Info *ghlInfo = &ghoul2[modelIndex];

	if (G2_SetupModelPointers(ghlInfo))
	{ //model is valid
		mdxaHeader_t *mdxa = ghlInfo->currentModel->data.gla;
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


qboolean G2API_SetBoneAnimIndex(CGhoul2Info *ghlInfo, const int index, const int startFrame, const int endFrame, const int flags, const float animSpeed, const int currentTime, const float setFrame, const int blendTime)
{
	//rww - RAGDOLL_BEGIN
	if (ghlInfo && (ghlInfo->mFlags & GHOUL2_RAG_STARTED))
	{
		return qfalse;
	}
	//rww - RAGDOLL_END

	qboolean ret = qfalse;
	if (G2_SetupModelPointers(ghlInfo))
	{
		G2ERROR(startFrame >= 0, "startframe < 0");
		G2ERROR(startFrame < ghlInfo->aHeader->numFrames, "startframe >= numframes");
		G2ERROR(endFrame > 0, "endframe <= 0");
		G2ERROR(endFrame <= ghlInfo->aHeader->numFrames, "endframe > numframes");
		G2ERROR(setFrame < ghlInfo->aHeader->numFrames, "setframe >= numframes");
		G2ERROR(setFrame == -1.0f || setFrame >= 0.0f, "setframe < 0 but not -1");

		// this is cruel
		if (startFrame < 0 || startFrame >= ghlInfo->aHeader->numFrames)
		{
			*(int*)&startFrame = 0; // cast away const
		}
		if (endFrame <= 0 || endFrame > ghlInfo->aHeader->numFrames)
		{
			*(int*)&endFrame = 1;
		}
		if (setFrame != -1.0f && (setFrame < 0.0f || setFrame >= (float)ghlInfo->aHeader->numFrames))
		{
			*(float*)&setFrame = 0.0f;
		}
		ghlInfo->mSkelFrameNum = 0;

		G2ERROR(index >= 0 && index < (int)ghlInfo->mBlist.size(), va("Out of Range Bone Index (%s)", ghlInfo->mFileName));

		if (index >= 0 && index < (int)ghlInfo->mBlist.size())
		{
			G2ERROR(ghlInfo->mBlist[index].boneNumber >= 0, va("Bone Index is not Active (%s)", ghlInfo->mFileName));
			int a_currentTime = G2API_GetTime(currentTime);

			ret = G2_Set_Bone_Anim_Index(ghlInfo->mBlist, index, startFrame, endFrame, flags, animSpeed, a_currentTime, setFrame, blendTime, ghlInfo->aHeader->numFrames);
			G2ANIM(ghlInfo, "G2API_SetBoneAnimIndex");
		}
	}

	G2WARNING(ret, va("G2API_SetBoneAnimIndex Failed (%s)", ghlInfo->mFileName));
	return ret;
}

qboolean G2API_SetBoneAnim(CGhoul2Info* ghlInfo, const char* boneName, const int startFrame, const int endFrame,
						   const int flags, const float animSpeed, const int currentTime, const float setFrame, const int blendTime)
{
	qboolean ret = qfalse;
	G2ERROR(boneName, "G2API_SetBoneAnim: NULL boneName");
	if (boneName)
	{
		int a_startFrame = (startFrame < 0 || startFrame > 100000) ? 0 : startFrame;
		int a_endFrame = (endFrame < 0 || endFrame > 100000) ? 1 : endFrame;
		float a_setFrame = ((setFrame < 0.f && setFrame != -1.f) || setFrame > 100000) ? 0.f : setFrame;

		qboolean setPtrs = qfalse;
		bool res = false;
		
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

		if (res && ghlInfo)
		{
			// ensure we flush the cache
			ghlInfo->mSkelFrameNum = 0;
			int a_currentTime = G2API_GetTime(currentTime);
 			return G2_Set_Bone_Anim(ghlInfo, ghlInfo->mBlist, boneName, a_startFrame, a_endFrame, flags, animSpeed, a_currentTime, a_setFrame, blendTime);
		}
	}
	return qfalse;
}

qboolean G2API_GetBoneAnim(CGhoul2Info* ghlInfo, const char* boneName, const int currentTime, float* currentFrame,
							  int* startFrame, int* endFrame, int* flags, float* animSpeed, qhandle_t* modelList)
{
	G2ERROR(boneName, "G2API_GetBoneAnim: NULL boneName");

	assert(startFrame != endFrame); //this is bad
	assert(startFrame != flags); //this is bad
	assert(endFrame != flags); //this is bad
	assert(currentFrame != animSpeed); //this is bad

	if (G2_SetupModelPointers(ghlInfo))
	{
		int a_CurrentTime=G2API_GetTime(currentTime);
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

		qboolean ret = G2_Get_Bone_Anim(ghlInfo, ghlInfo->mBlist, boneName, a_CurrentTime, currentFrame, startFrame, endFrame, flags, animSpeed);
		return ret;
	}

	G2WARNING(ret, "G2API_GetBoneAnim Failed");
	return qfalse;
}

qboolean G2API_GetBoneAnimIndex(CGhoul2Info* ghlInfo, const int iBoneIndex, const int currentTime, float* currentFrame,
								int* startFrame, int* endFrame, int* flags, float* animSpeed, qhandle_t* modelList)
{
	qboolean ret = qfalse;
	if (G2_SetupModelPointers(ghlInfo))
	{
		int a_currentTime = G2API_GetTime(currentTime);

		if (iBoneIndex >= 0 && iBoneIndex < (int)ghlInfo->mBlist.size())
		{
			if ((ghlInfo->mBlist[iBoneIndex].flags & (BONE_ANIM_OVERRIDE_LOOP | BONE_ANIM_OVERRIDE)))
			{
				int sf, ef;
				ret = G2_Get_Bone_Anim_Index(
					ghlInfo->mBlist,// boneInfo_v &blist,
					iBoneIndex,		// const int index,
					a_currentTime,	// const int a_currentTime,
					currentFrame,	// float *currentFrame,
					&sf,			// int *startFrame,
					&ef,			// int *endFrame,
					flags,			// int *flags,
					animSpeed,		// float *retAnimSpeed,
					ghlInfo->aHeader->numFrames
				);

				G2ERROR(sf >= 0, "returning startframe<0");
				G2ERROR(sf < ghlInfo->aHeader->numFrames, "returning startframe>=numframes");
				G2ERROR(ef > 0, "returning endframe<=0");
				G2ERROR(ef <= ghlInfo->aHeader->numFrames, "returning endframe>numframes");

				if (currentFrame)
				{
					G2ERROR(*currentFrame >= 0.0f, "returning currentframe<0");
					G2ERROR(((int)(*currentFrame)) < ghlInfo->aHeader->numFrames, "returning currentframe>=numframes");
				}
				if (endFrame)
				{
					*endFrame = ef;
				}
				if (startFrame)
				{
					*startFrame = sf;
				}
			}
		}
	}
	if (!ret)
	{
		if (endFrame) *endFrame = 1;
		if (startFrame) *startFrame = 0;
		if (flags) *flags = 0;
		if (currentFrame) *currentFrame = 0.0f;
		if (animSpeed) *animSpeed = 1.0f;
	}
	G2NOTE(ret, "G2API_GetBoneAnimIndex Failed");
	return ret;
}

qboolean G2API_GetAnimRange(CGhoul2Info *ghlInfo, const char *boneName,	int *startFrame, int *endFrame)
{
	G2ERROR(boneName, "G2API_GetAnimRange: NULL boneName");
	assert(startFrame != endFrame); //this is bad

	if (boneName && G2_SetupModelPointers(ghlInfo))
	{
 		qboolean ret=G2_Get_Bone_Anim_Range(ghlInfo, ghlInfo->mBlist, boneName, startFrame, endFrame);
		G2ANIM(ghlInfo, "G2API_GetAnimRange");
#ifdef _DEBUG
		assert(*endFrame > 0);
		assert(*endFrame < 100000);
		assert(*startFrame >= 0);
		assert(*startFrame < 100000);
#endif
		return ret;
	}
	return qfalse;
}

qboolean G2API_GetAnimRangeIndex(CGhoul2Info* ghlInfo, const int boneIndex, int* startFrame, int* endFrame)
{
	if (G2_SetupModelPointers(ghlInfo))
	{
		G2ERROR(boneIndex >= 0 && boneIndex < (int)ghlInfo->mBlist.size(), "Bad Bone Index");
		if (boneIndex >= 0 && boneIndex < (int)ghlInfo->mBlist.size())
		{
			return G2_Get_Bone_Anim_Range_Index(ghlInfo->mBlist, boneIndex, startFrame, endFrame);
		}
	}
	return qfalse;
}

qboolean G2API_PauseBoneAnim(CGhoul2Info *ghlInfo, const char *boneName, const int currentTime)
{
	G2ERROR(boneName, "G2API_PauseBoneAnim: NULL boneName");
	if (G2_SetupModelPointers(ghlInfo))
	{
		int a_currentTime = G2API_GetTime(currentTime);
 		return G2_Pause_Bone_Anim(ghlInfo, ghlInfo->mBlist, boneName, a_currentTime);
	}
	G2WARNING(ret, "G2API_PauseBoneAnim Failed");
	return qfalse;
}

qboolean G2API_PauseBoneAnimIndex(CGhoul2Info* ghlInfo, const int boneIndex, const int currentTime)
{
	if (G2_SetupModelPointers(ghlInfo))
	{
		int a_currentTime = G2API_GetTime(currentTime);

		if (boneIndex >= 0 && boneIndex < (int)ghlInfo->mBlist.size())
		{
			return G2_Pause_Bone_Anim_Index(ghlInfo->mBlist, boneIndex, a_currentTime, ghlInfo->aHeader->numFrames);
		}
	}

	G2WARNING(ret, "G2API_PauseBoneAnimIndex Failed");
	return qfalse;
}

qboolean G2API_IsPaused(CGhoul2Info *ghlInfo, const char *boneName)
{
	G2ERROR(boneName, "G2API_IsPaused: NULL boneName");
	if (G2_SetupModelPointers(ghlInfo))
	{
 		return G2_IsPaused(ghlInfo, ghlInfo->mBlist, boneName);
	}
	return qfalse;
}

qboolean G2API_StopBoneAnimIndex(CGhoul2Info *ghlInfo, const int index)
{
	G2ERROR(ghlInfo, "G2API_StopBoneAnimIndex: NULL ghlInfo");
	if (G2_SetupModelPointers(ghlInfo))
	{
		if (index >= 0 && index < (int)ghlInfo->mBlist.size())
		{
			return G2_Stop_Bone_Anim_Index(ghlInfo->mBlist, index);
		}
	}
	return qfalse;
}

qboolean G2API_StopBoneAnim(CGhoul2Info *ghlInfo, const char *boneName)
{
	G2ERROR(boneName, "G2API_StopBoneAnim: NULL boneName");
	if (G2_SetupModelPointers(ghlInfo))
	{
 		return G2_Stop_Bone_Anim(ghlInfo, ghlInfo->mBlist, boneName);
	}
	G2WARNING(ret, "G2API_StopBoneAnim Failed");
	return qfalse;
}

qboolean G2API_SetBoneAnglesIndex(
	CGhoul2Info *ghlInfo, const int index, const vec3_t angles, const int flags,
	const Eorientations yaw, const Eorientations pitch, const Eorientations roll,
	qhandle_t *modelList, int blendTime, int currentTime)
{
	qboolean setPtrs = qfalse;
	bool res = false;

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

	if (!setPtrs)
	{
		res = G2_SetupModelPointers(ghlInfo);
	}

	if (res)
	{
		// ensure we flush the cache
		ghlInfo->mSkelFrameNum = 0;
		return G2_Set_Bone_Angles_Index(ghlInfo, ghlInfo->mBlist, index, angles, flags, yaw, pitch, roll, blendTime, currentTime);
	}

	return qfalse;
}

qboolean G2API_SetBoneAngles(CGhoul2Info* ghlInfo, const char* boneName, const vec3_t angles, const int flags,
	const Eorientations up, const Eorientations left, const Eorientations forward, qhandle_t* modelList, int blendTime, int currentTime)
{
	//rww - RAGDOLL_BEGIN
	if (ghlInfo && ghlInfo->mFlags & GHOUL2_RAG_STARTED)
	{
		return qfalse;
	}
	//rww - RAGDOLL_END

	qboolean ret = qfalse;

	G2ERROR(boneName, "G2API_SetBoneAngles: NULL boneName");
	if (boneName && G2_SetupModelPointers(ghlInfo))
	{
		currentTime = G2API_GetTime(currentTime);
		// ensure we flush the cache
		ghlInfo->mSkelFrameNum = 0;
		ret = G2_Set_Bone_Angles(ghlInfo, ghlInfo->mBlist, boneName, angles, flags, up, left, forward, blendTime, currentTime);
	}

	G2WARNING(ret, "G2API_SetBoneAngles Failed");
	return ret;
}

qboolean G2API_SetBoneAnglesMatrixIndex(
	CGhoul2Info *ghlInfo, const int index, const mdxaBone_t &matrix, const int flags,
	qhandle_t *modelList, int blendTime, int currentTime)
{
	if (G2_SetupModelPointers(ghlInfo))
	{
		currentTime = G2API_GetTime(currentTime);
		// ensure we flush the cache
		ghlInfo->mSkelFrameNum = 0;
		G2ERROR(index >= 0 && index < (int)ghlInfo->mBlist.size(), "Bad Bone Index");
		if (index >= 0 && index < (int)ghlInfo->mBlist.size())
		{
			return G2_Set_Bone_Angles_Matrix_Index(ghlInfo->mBlist, index, matrix, flags, blendTime, currentTime);
		}
	}
	return qfalse;
}

qboolean G2API_SetBoneAnglesMatrix(
	CGhoul2Info *ghlInfo, const char *boneName, const mdxaBone_t &matrix, const int flags,
	qhandle_t *modelList, int blendTime, int currentTime)
{
	G2ERROR(boneName, "G2API_SetBoneAnglesMatrix: NULL boneName");
	if (G2_SetupModelPointers(ghlInfo))
	{
		currentTime = G2API_GetTime(currentTime);
		// ensure we flush the cache
		ghlInfo->mSkelFrameNum = 0;
		return G2_Set_Bone_Angles_Matrix(ghlInfo, ghlInfo->mBlist, boneName, matrix, flags, blendTime, currentTime);
	}
	return qfalse;
}

qboolean G2API_StopBoneAnglesIndex(CGhoul2Info *ghlInfo, const int index)
{
	if (G2_SetupModelPointers(ghlInfo))
	{
		// ensure we flush the cache
		ghlInfo->mSkelFrameNum = 0;
		G2ERROR(index >= 0 && index < (int)ghlInfo->mBlist.size(), "Bad Bone Index");
		if (index >= 0 && index < (int)ghlInfo->mBlist.size())
		{
			return G2_Stop_Bone_Angles_Index(ghlInfo->mBlist, index);
		}
	}
	return qfalse;
}

qboolean G2API_StopBoneAngles(CGhoul2Info *ghlInfo, const char *boneName)
{
	if (boneName && G2_SetupModelPointers(ghlInfo))
	{
		// ensure we flush the cache
		ghlInfo->mSkelFrameNum = 0;
		return G2_Stop_Bone_Angles(
			ghlInfo, ghlInfo->mBlist, boneName);
	}
	return qfalse;
}

void G2API_AbsurdSmoothing(CGhoul2Info_v &ghoul2, qboolean status)
{
	assert(ghoul2.size());
	CGhoul2Info *ghlInfo = &ghoul2[0];
#ifndef REND2_SP
	if (status)
	{ //turn it on
		ghlInfo->mFlags |= GHOUL2_CRAZY_SMOOTH;
	}
	else
	{ //off
		ghlInfo->mFlags &= ~GHOUL2_CRAZY_SMOOTH;
	}
#endif
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

qboolean G2API_RemoveBone(CGhoul2Info* ghlInfo, const char* boneName)
{
	G2ERROR(boneName, "G2API_RemoveBone: NULL boneName");

	if (boneName && G2_SetupModelPointers(ghlInfo))
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

// This is G2API_AnimateG2Models
void G2API_AnimateG2ModelsRag(CGhoul2Info_v &ghoul2, int AcurrentTime, CRagDollUpdateParams *params)
{
	int model;
	int currentTime = G2API_GetTime(AcurrentTime);

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
			G2_Animate_Bone_List(ghoul2, currentTime, model, params);
		}
	}
}
// rww - RAGDOLL_END

int G2_Find_Bone_Rag(
	CGhoul2Info *ghlInfo, boneInfo_v &blist, const char *boneName);
#define RAG_PCJ						(0x00001)
#define RAG_EFFECTOR				(0x00100)

static boneInfo_t* G2_GetRagBoneConveniently(CGhoul2Info_v &ghoul2, const char *boneName)
{
	assert(ghoul2.size());
	CGhoul2Info *ghlInfo = &ghoul2[0];

	if (!(ghlInfo->mFlags & GHOUL2_RAG_STARTED))
	{ // can't do this if not in ragdoll
		return NULL;
	}

	int boneIndex = G2_Find_Bone_Rag(ghlInfo, ghlInfo->mBlist, boneName);

	if (boneIndex < 0)
	{ // bad bone specification
		return NULL;
	}

	boneInfo_t *bone = &ghlInfo->mBlist[boneIndex];

	if (!(bone->flags & BONE_ANGLES_RAGDOLL))
	{ // only want to return rag bones
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
	{ // this function is only for PCJ bones
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
	{ // this function is only for PCJ bones
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
	{ // this function is only for effectors
		return qfalse;
	}

	if (!pos)
	{ // go back to none in case we have one then
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
{
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
		return G2_Remove_Bolt(ghlInfo->mBltlist, index);
	}
	return qfalse;
}

int G2API_AddBolt(CGhoul2Info* ghlInfo, const char* boneName)
{
	//	assert(ghoul2.size()>modelIndex);
	if (boneName && G2_SetupModelPointers(ghlInfo))
	{
		return G2_Add_Bolt(ghlInfo, ghlInfo->mBltlist, ghlInfo->mSlist, boneName);
	}
	return -1;
}

int G2API_AddBoltSurfNum(CGhoul2Info *ghlInfo, const int surfIndex)
{
	if (G2_SetupModelPointers(ghlInfo))
	{
		return G2_Add_Bolt_Surf_Num(
			ghlInfo, ghlInfo->mBltlist, ghlInfo->mSlist, surfIndex);
	}
	return -1;
}

qboolean G2API_AttachG2Model(CGhoul2Info* ghlInfo, CGhoul2Info* ghlInfoTo, int toBoltIndex, int toModel)
{
	qboolean ret = qfalse;

	if (G2_SetupModelPointers(ghlInfo) && G2_SetupModelPointers(ghlInfoTo))
	{
		G2ERROR(toBoltIndex >= 0 && toBoltIndex < (int)ghlInfoTo->mBltlist.size(), "Invalid Bolt Index");
		G2ERROR(ghlInfoTo->mBltlist.size() > 0, "Empty Bolt List");

		assert(toBoltIndex >= 0);

		if (toBoltIndex >= 0 && ghlInfoTo->mBltlist.size())
		{
			// make sure we have a model to attach, a model to attach to, and a bolt on that model
			if (((ghlInfoTo->mBltlist[toBoltIndex].boneNumber != -1) || (ghlInfoTo->mBltlist[toBoltIndex].surfaceNumber != -1)))
			{
				// encode the bolt address into the model bolt link
				toModel &= MODEL_AND;
				toBoltIndex &= BOLT_AND;
				ghlInfo->mModelBoltLink = (toModel << MODEL_SHIFT) | (toBoltIndex << BOLT_SHIFT);
				ret = qtrue;
			}
		}
	}
	G2WARNING(ret, "G2API_AttachG2Model Failed");
	return ret;
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

qboolean G2API_AttachEnt(int* boltInfo, CGhoul2Info* ghlInfoTo, int toBoltIndex, int entNum, int toModelNum)
{
	if (boltInfo && G2_SetupModelPointers(ghlInfoTo))
	{
		// make sure we have a model to attach, a model to attach to, and a
		// bolt on that model
		if (ghlInfoTo->mBltlist.size() &&
			((ghlInfoTo->mBltlist[toBoltIndex].boneNumber != -1) ||
			 (ghlInfoTo->mBltlist[toBoltIndex].surfaceNumber != -1)))
		{
			// encode the bolt address into the model bolt link
			toModelNum &= MODEL_AND;
			toBoltIndex &= BOLT_AND;
			entNum &= ENTITY_AND;
			*boltInfo = (toBoltIndex << BOLT_SHIFT) |
						(toModelNum << MODEL_SHIFT) |
						(entNum << ENTITY_SHIFT);
			return qtrue;
		}
	}
	return qfalse;
}

void G2API_DetachEnt(int* boltInfo)
{
	G2ERROR(boltInfo, "G2API_DetachEnt: NULL boltInfo");
	if (boltInfo)
	{
		*boltInfo = 0;
	}
}

bool G2_NeedsRecalc(CGhoul2Info *ghlInfo, int frameNum);

qboolean G2API_GetBoltMatrix(CGhoul2Info_v& ghoul2, const int modelIndex, const int boltIndex, mdxaBone_t* matrix,
							 const vec3_t angles, const vec3_t position, const int frameNum, qhandle_t* modelList, const vec3_t scale)
{
	G2ERROR(ghoul2.IsValid(), "Invalid ghlInfo");
	G2ERROR(matrix, "NULL matrix");
	G2ERROR(modelIndex >= 0 && modelIndex < ghoul2.size(), "Invalid ModelIndex");
	const static mdxaBone_t identityMatrix =
	{
		{
			{0.0f, -1.0f, 0.0f, 0.0f},
			{1.0f, 0.0f, 0.0f, 0.0f},
			{0.0f, 0.0f, 1.0f, 0.0f}
		}
	};

	G2_GenerateWorldMatrix(angles, position);
	if (G2_SetupModelPointers(ghoul2))
	{
		if (matrix && modelIndex >= 0 && modelIndex < ghoul2.size())
		{
			int tframeNum = G2API_GetTime(frameNum);
			CGhoul2Info *ghlInfo = &ghoul2[modelIndex];
			G2ERROR
				(boltIndex >= 0 && (boltIndex < ghlInfo->mBltlist.size()),
				va("Invalid Bolt Index (%d:%s)",
				   boltIndex,
				   ghlInfo->mFileName));

			if (boltIndex >= 0 && ghlInfo && (boltIndex < (int)ghlInfo->mBltlist.size()))
			{
				mdxaBone_t bolt;

				if (G2_NeedsRecalc(ghlInfo, tframeNum))
				{
					G2_ConstructGhoulSkeleton(ghoul2, tframeNum, true, scale);
				}

				G2_GetBoltMatrixLow(*ghlInfo, boltIndex, scale, bolt);

				// scale the bolt position by the scale factor for this model
				// since at this point its still in model space
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

				VectorNormalize((float *)&bolt.matrix[0]);
				VectorNormalize((float *)&bolt.matrix[1]);
				VectorNormalize((float *)&bolt.matrix[2]);

				Mat3x4_Multiply(matrix, &worldMatrix, &bolt);
#if G2API_DEBUG
				for (int i = 0; i < 3; i++)
				{
					for (int j = 0; j < 4; j++)
					{
						assert(!_isnan(matrix->matrix[i][j]));
					}
				}
#endif // _DEBUG
				G2ANIM(ghlInfo, "G2API_GetBoltMatrix");

				return qtrue;
			}
		}
	}
	else
	{
		G2WARNING(0, "G2API_GetBoltMatrix Failed on empty or bad model");
	}
	Mat3x4_Multiply(matrix, &worldMatrix, (mdxaBone_t *)&identityMatrix);
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
	if (ghoul2.size())
	{
		for (int i = 0; i < ghoul2.size(); i++)
		{
			if (ghoul2[i].mModelindex != -1)
			{
				return qtrue;
			}
		}
	}
	return qfalse;
}

// run through the Ghoul2 models and set each of the mModel values to the
// correct one from the cgs.gameModel offset lsit
void G2API_SetGhoul2ModelIndexes(CGhoul2Info_v &ghoul2, qhandle_t *modelList, qhandle_t *skinList)
{
	G2ERROR(ghoul2.IsValid(), "G2API_SetGhoul2ModelIndexes: Invalid ghlInfo");

	for (int i = 0; i < ghoul2.size(); i++)
	{
		if (ghoul2[i].mModelindex != -1)
		{
			ghoul2[i].mSkin = skinList[ghoul2[i].mCustomSkin];
		}
	}
}

char *G2API_GetAnimFileNameIndex(qhandle_t modelIndex)
{
	model_t* mod_m = R_GetModelByHandle(modelIndex);

	G2ERROR(mod_m && mod_m->data.glm, "G2API_GetAnimFileNameIndex: Bad Model");
	if (mod_m && mod_m->data.glm && mod_m->data.glm->header)
	{
		return mod_m->data.glm->header->animName;
	}
	return "";
}

// As above, but gets the internal embedded name, not the name of the disk file.
// This is needed for some unfortunate jiggery-hackery to do with frameskipping & the animevents.cfg file
char* G2API_GetAnimFileInternalNameIndex(qhandle_t modelIndex)
{
	model_t* mod_a = R_GetModelByHandle(modelIndex);

	G2ERROR(mod_a && mod_a->data.gla, "G2API_GetAnimFileInternalNameIndex: Bad Model");
	if (mod_a && mod_a->data.gla)
	{
		return mod_a->data.gla->name;
	}
	return "";
}

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
static int QDECL QsortDistance(const void *a, const void *b)
{
	const float &ea = ((CCollisionRecord*)a)->mDistance;
	const float &eb = ((CCollisionRecord*)b)->mDistance;
	return (ea < eb) ? -1 : 1;
}

void G2API_CollisionDetect(
	CCollisionRecord* collRecMap, CGhoul2Info_v& ghoul2,
	const vec3_t angles, const vec3_t position, int frameNumber, int entNum,
	vec3_t rayStart, vec3_t rayEnd, vec3_t scale,
	CMiniHeap* G2VertSpace, EG2_Collision eG2TraceType, int useLod, float fRadius)
{
	// not exactly correct, but doesn't matter
	G2ERROR(ghoul2.IsValid(), "G2API_CollisionDetect: Invalid ghlInfo");
	G2ERROR(collRecMap, "G2API_CollisionDetect: NULL Collision Rec");

	if (G2_SetupModelPointers(ghoul2) && collRecMap)
	{
		frameNumber = G2API_GetTime(frameNumber);
		vec3_t transRayStart, transRayEnd;

		// make sure we have transformed the whole skeletons for each model
		G2_ConstructGhoulSkeleton(ghoul2, frameNumber, true, scale);

		// pre generate the world matrix - used to transform the incoming ray
		G2_GenerateWorldMatrix(angles, position);

		ri.GetG2VertSpaceServer()->ResetHeap();

// now having done that, time to build the model
#ifdef _G2_GORE
		G2_TransformModel(ghoul2, frameNumber, scale, ri.GetG2VertSpaceServer(), useLod, false);
#else
		G2_TransformModel(ghoul2, frameNumber, scale, ri.GetG2VertSpaceServer(), useLod);
#endif

		// model is built. Lets check to see if any triangles are actually hit.
		// first up, translate the ray to model space
		TransformAndTranslatePoint(rayStart, transRayStart, &worldMatrixInv);
		TransformAndTranslatePoint(rayEnd, transRayEnd, &worldMatrixInv);

		// now walk each model and check the ray against each poly - sigh, this is SO expensive. I wish there was a better way to do this.
#ifdef _G2_GORE
		G2_TraceModels(ghoul2, transRayStart, transRayEnd, collRecMap, entNum, eG2TraceType, useLod, fRadius, 0, 0, 0, 0, 0, qfalse);
#else
		G2_TraceModels(ghoul2, transRayStart, transRayEnd, collRecMap, entNum, eG2TraceType, useLod, fRadius);
#endif

		ri.GetG2VertSpaceServer()->ResetHeap();
		// now sort the resulting array of collision records so they are distance ordered
		qsort(collRecMap, MAX_G2_COLLISIONS, sizeof(CCollisionRecord), QsortDistance);
		G2ANIM(ghoul2, "G2API_CollisionDetect");
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
void G2API_GiveMeVectorFromMatrix(mdxaBone_t& boltMatrix, Eorientations flags, vec3_t& vec)
{
	switch (flags)
	{
	case ORIGIN:
		vec[0] = boltMatrix.matrix[0][3];
		vec[1] = boltMatrix.matrix[1][3];
		vec[2] = boltMatrix.matrix[2][3];
		break;
	case POSITIVE_Y:
		vec[0] = boltMatrix.matrix[0][1];
		vec[1] = boltMatrix.matrix[1][1];
		vec[2] = boltMatrix.matrix[2][1];
 		break;
	case POSITIVE_X:
		vec[0] = boltMatrix.matrix[0][0];
		vec[1] = boltMatrix.matrix[1][0];
		vec[2] = boltMatrix.matrix[2][0];
		break;
	case POSITIVE_Z:
		vec[0] = boltMatrix.matrix[0][2];
		vec[1] = boltMatrix.matrix[1][2];
		vec[2] = boltMatrix.matrix[2][2];
		break;
	case NEGATIVE_Y:
		vec[0] = -boltMatrix.matrix[0][1];
		vec[1] = -boltMatrix.matrix[1][1];
		vec[2] = -boltMatrix.matrix[2][1];
		break;
	case NEGATIVE_X:
		vec[0] = -boltMatrix.matrix[0][0];
		vec[1] = -boltMatrix.matrix[1][0];
		vec[2] = -boltMatrix.matrix[2][0];
		break;
	case NEGATIVE_Z:
		vec[0] = -boltMatrix.matrix[0][2];
		vec[1] = -boltMatrix.matrix[1][2];
		vec[2] = -boltMatrix.matrix[2][2];
		break;
	}
}

// copy a model from one ghoul2 instance to another, and reset the root surface on the new model if need be
// NOTE if modelIndex = -1 then copy all the models
void G2API_CopyGhoul2Instance(CGhoul2Info_v &g2From, CGhoul2Info_v &g2To, int modelIndex)
{
	G2ERROR(ghoul2From.IsValid(),"G2API_CopyGhoul2Instance: Invalid ghlInfo");

	if (g2From.IsValid())
	{
		g2To.DeepCopy(g2From);

#ifdef _G2_GORE //check through gore stuff then, as well.
		int model = 0;

		//(since we are sharing this gore set with the copied instance we will have to increment
		//the reference count - if the goreset is "removed" while the refcount is > 0, the refcount
		//is decremented to avoid giving other instances an invalid pointer -rww)
		while (model < g2To.size())
		{
			if (g2To[model].mGoreSetTag)
			{
				CGoreSet* gore = FindGoreSet(g2To[model].mGoreSetTag);
				assert(gore);
				if (gore)
				{
					gore->mRefCount++;
				}
			}

			model++;
		}
#endif
		G2ANIM(ghoul2From, "G2API_CopyGhoul2Instance (source)");
		G2ANIM(ghoul2To, "G2API_CopyGhoul2Instance (dest)");
	}
}

char* G2API_GetSurfaceName(CGhoul2Info* ghlInfo, int surfNumber)
{
	static char noSurface[1] = "";

	if (G2_SetupModelPointers(ghlInfo))
	{
		model_t	*mod = (model_t *)ghlInfo->currentModel;
		mdxmSurface_t* surf = 0;
		mdxmSurfHierarchy_t* surfInfo = 0;
		mdxmHeader_t* mdxm;

		mdxm = mod->data.glm->header;

		//ok, I guess it's semi-valid for the user to be passing in surface > numSurfs because they don't know how many surfs a model
		//may have.. but how did they get that surf index to begin with? Oh well.
		if (surfNumber < 0 || surfNumber >= mdxm->numSurfaces)
		{
			Com_Printf("G2API_GetSurfaceName: You passed in an invalid surface number (%i) for model %s.\n", surfNumber, ghlInfo->mFileName);
			return noSurface;
		}

		surf = (mdxmSurface_t*)G2_FindSurface(mod, surfNumber, 0);
		if (surf)
		{
			mdxmHierarchyOffsets_t* surfIndexes = (mdxmHierarchyOffsets_t*)((byte*)ghlInfo->currentModel->data.glm->header + sizeof(mdxmHeader_t));
			surfInfo = (mdxmSurfHierarchy_t*)((byte*)surfIndexes + surfIndexes->offsets[surf->thisSurfaceIndex]);
			return surfInfo->name;
		}
	}
	return noSurface;
}


int	G2API_GetSurfaceIndex(CGhoul2Info *ghlInfo, const char *surfaceName)
{
	G2ERROR(surfaceName, "G2API_GetSurfaceIndex: NULL surfaceName");

	if (G2_SetupModelPointers(ghlInfo))
	{
		return G2_GetSurfaceIndex(ghlInfo, surfaceName);
	}
	return -1;
}

char* G2API_GetGLAName(CGhoul2Info* ghlInfo)
{
	if (G2_SetupModelPointers(ghlInfo))
	{
		assert(ghlInfo && ghlInfo->currentModel->data.glm);
		return (char*)ghlInfo->aHeader->name;
	}
	return NULL;
}

qboolean G2API_SetNewOrigin(CGhoul2Info* ghlInfo, const int boltIndex)
{
	if (G2_SetupModelPointers(ghlInfo))
	{
		if (boltIndex < 0)
		{
            char modelName[MAX_QPATH];
			if (ghlInfo->currentModel && ghlInfo->currentModel->name[0])
			{
				strcpy(modelName, ghlInfo->currentModel->name);
			}
			else
			{
				strcpy(modelName, "[Unknown - unexpected]");
			}

			Com_Error(ERR_DROP, "Bad boltindex (%i) trying to SetNewOrigin (naughty naughty!)\nModel %s\n", boltIndex, modelName);
		}

		ghlInfo->mNewOrigin = boltIndex;
		ghlInfo->mFlags |= GHOUL2_NEWORIGIN;
		return qtrue;
	}
	return qfalse;
}

int G2API_GetBoneIndex(CGhoul2Info *ghlInfo, const char *boneName, qboolean bAddIfNotFound)
{
	G2ERROR(boneName, "G2API_GetBoneIndex: NULL boneName");
	if (boneName && G2_SetupModelPointers(ghlInfo))
	{
		return G2_Get_Bone_Index(ghlInfo, boneName, bAddIfNotFound);
	}
	return -1;
}

void G2API_SaveGhoul2Models(CGhoul2Info_v &ghoul2)
{
	G2_SaveGhoul2Models(ghoul2);
	G2ANIM(ghoul2, "G2API_SaveGhoul2Models");
}

void G2API_LoadGhoul2Models(CGhoul2Info_v &ghoul2, char *buffer)
{
	G2_LoadGhoul2Model(ghoul2, buffer);
	G2ANIM(ghoul2, "G2API_LoadGhoul2Models");
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
			mod->data.glm &&
			mod->data.glm->header)
		{
            mdxmSurfHierarchy_t	*surf;
			int i;
			mdxmHeader_t *mdxm = mod->data.glm->header;

			surf = (mdxmSurfHierarchy_t *) ( (byte *)mdxm + mdxm->ofsSurfHierarchy );

			for (i = 0; i < mdxm->numSurfaces; i++) 
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

int G2API_Ghoul2Size(CGhoul2Info_v &ghoul2)
{
	return ghoul2.size();
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
	for (int i = 0; i < ghoul2.size(); i++)
	{
		if (ghoul2[i].mGoreSetTag)
		{
			DeleteGoreSet(ghoul2[i].mGoreSetTag);
			ghoul2[i].mGoreSetTag = 0;
		}
	}
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
	G2_ConstructGhoulSkeleton(ghoul2, gore.currentTime, true, gore.scale);

	// pre generate the world matrix - used to transform the incoming ray
	G2_GenerateWorldMatrix(gore.angles, gore.position);

	// first up, translate the ray to model space
	vec3_t	transRayDirection, transHitLocation;
	TransformAndTranslatePoint(gore.hitLocation, transHitLocation, &worldMatrixInv);
	TransformPoint(gore.rayDirection, transRayDirection, &worldMatrixInv);
	if (!gore.useTheta)
	{
		vec3_t t;
		VectorCopy(gore.uaxis, t);
		TransformPoint(t, gore.uaxis, &worldMatrixInv);
	}

	int lod;
	ResetGoreTag();
	const int lodbias = Com_Clamp(0, 2, G2_DecideTraceLod(ghoul2[0], r_lodbias->integer));
	const int maxLod = Com_Clamp(0, ghoul2[0].currentModel->numLods, 3);	//limit to the number of lods the main model has

	for(lod = lodbias; lod < maxLod; lod++)
	{
		// now having done that, time to build the model
		ri.GetG2VertSpaceServer()->ResetHeap();

		G2_TransformModel(ghoul2, gore.currentTime, gore.scale, ri.GetG2VertSpaceServer(), lod, true, &gore);

		// now walk each model and compute new texture coordinates
		G2_TraceModels(ghoul2, transHitLocation, transRayDirection, 0, gore.entNum, G2_NOCOLLIDE, lod, 1.0f, gore.SSize, gore.TSize, gore.theta, gore.shader, &gore, qtrue);
	}
}
#else

void G2API_ClearSkinGore(CGhoul2Info_v& ghoul2) {}
void G2API_AddSkinGore(CGhoul2Info_v& ghoul2, SSkinGoreData& gore) {}

#endif

qboolean G2_TestModelPointers(CGhoul2Info *ghlInfo) // returns true if the model is properly set up
{
	G2ERROR(ghlInfo,"G2_TestModelPointers: NULL ghlInfo");
	if (!ghlInfo)
	{
		return qfalse;
	}
	ghlInfo->mValid=false;
	if (ghlInfo->mModelindex != -1)
	{
		ghlInfo->mModel = RE_RegisterModel(ghlInfo->mFileName);
		ghlInfo->currentModel = R_GetModelByHandle(ghlInfo->mModel);
		if (ghlInfo->currentModel &&
			ghlInfo->currentModel->type == MOD_MDXM) //Rend2 - data is a union now, so we need to make sure it's also a glm that is loaded
		{
			if (ghlInfo->currentModel->data.glm &&
				ghlInfo->currentModel->data.glm->header)
			{
				mdxmHeader_t *mdxm = ghlInfo->currentModel->data.glm->header;
				if (ghlInfo->currentModelSize)
				{
					if (ghlInfo->currentModelSize != mdxm->ofsEnd)
					{
						Com_Error(ERR_DROP, "Ghoul2 model was reloaded and has changed, map must be restarted.\n");
					}
				}

				ghlInfo->currentModelSize = mdxm->ofsEnd;
				ghlInfo->animModel = R_GetModelByHandle(mdxm->animIndex + ghlInfo->animModelIndexOffset);

				if (ghlInfo->animModel)
				{
					ghlInfo->aHeader = ghlInfo->animModel->data.gla;
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
		ghlInfo->currentModel = NULL;
		ghlInfo->currentModelSize = 0;
		ghlInfo->animModel = NULL;
		ghlInfo->currentAnimModelSize = 0;
		ghlInfo->aHeader = NULL;
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
	G2ERROR(ghlInfo,"G2_SetupModelPointers: NULL ghlInfo");
	if (!ghlInfo)
	{
		return qfalse;
	}

	ghlInfo->mValid=false;

	if (ghlInfo->mModelindex != -1)
	{
		G2ERROR(ghlInfo->mFileName[0],"empty ghlInfo->mFileName");

		// RJ - experimental optimization!
		ghlInfo->mModel = RE_RegisterModel(ghlInfo->mFileName);
		ghlInfo->currentModel = R_GetModelByHandle(ghlInfo->mModel);

		G2ERROR(ghlInfo->currentModel,va("NULL Model (glm) %s",ghlInfo->mFileName));
		if (ghlInfo->currentModel)
		{
			G2ERROR(ghlInfo->currentModel->modelData, va("Model has no mdxm (glm) %s", ghlInfo->mFileName));

			if (ghlInfo->currentModel->data.glm && ghlInfo->currentModel->data.glm->header)
			{
				mdxmHeader_t *mdxm = ghlInfo->currentModel->data.glm->header;
				if (ghlInfo->currentModelSize)
				{
					if (ghlInfo->currentModelSize!=mdxm->ofsEnd)
					{
						Com_Error(ERR_DROP, "Ghoul2 model was reloaded and has changed, map must be restarted.\n");
					}
				}
				ghlInfo->currentModelSize=mdxm->ofsEnd;
				G2ERROR(ghlInfo->currentModelSize, va("Zero sized Model? (glm) %s",ghlInfo->mFileName));

				ghlInfo->animModel = R_GetModelByHandle(mdxm->animIndex + ghlInfo->animModelIndexOffset);
				G2ERROR(ghlInfo->animModel, va("NULL Model (gla) %s",ghlInfo->mFileName));

				if (ghlInfo->animModel)
				{
					ghlInfo->aHeader = ghlInfo->animModel->data.gla;
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

						ghlInfo->currentAnimModelSize = ghlInfo->aHeader->ofsEnd;
						G2ERROR(ghlInfo->currentAnimModelSize, va("Zero sized Model? (gla) %s",ghlInfo->mFileName));
						ghlInfo->mValid = true;
					}
				}
			}
		}
	}
	if (!ghlInfo->mValid)
	{
		ghlInfo->currentModel = NULL;
		ghlInfo->currentModelSize = 0;
		ghlInfo->animModel = NULL;
		ghlInfo->currentAnimModelSize = 0;
		ghlInfo->aHeader = NULL;
	}

#ifdef G2_PERFORMANCE_ANALYSIS
	G2Time_G2_SetupModelPointers += G2PerformanceTimer_G2_SetupModelPointers.End();
#endif
	return (qboolean)ghlInfo->mValid;
}

qboolean G2_SetupModelPointers(CGhoul2Info_v &ghoul2) // returns true if any model is properly set up
{
	qboolean ret = qfalse;

	for (int i=0; i<ghoul2.size(); i++)
	{
		qboolean r = G2_SetupModelPointers(&ghoul2[i]);
		ret = (qboolean)(ret || r);
	}
	return ret;
}

qboolean G2API_IsGhoul2InfovValid (CGhoul2Info_v& ghoul2)
{
	return (qboolean)ghoul2.IsValid();
}

const char *G2API_GetModelName ( CGhoul2Info_v& ghoul2, int modelIndex )
{
	return ghoul2[modelIndex].mFileName;
}
