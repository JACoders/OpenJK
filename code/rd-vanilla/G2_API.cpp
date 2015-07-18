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

#include "../server/exe_headers.h"

#include <list>
#include <string>

#include "../qcommon/q_shared.h"
#include "tr_local.h"
#include "../ghoul2/G2.h"
#include "../qcommon/MiniHeap.h"

#ifdef FINAL_BUILD
#define G2API_DEBUG (0) // please don't change this
#else
	#if defined(_DEBUG)
		#define G2API_DEBUG (1)
	#else
		#define G2API_DEBUG (0) // change this to test g2api in release
	#endif
#endif

//rww - RAGDOLL_BEGIN
#include "../ghoul2/ghoul2_gore.h"
//rww - RAGDOLL_END

extern mdxaBone_t		worldMatrix;
extern mdxaBone_t		worldMatrixInv;

extern	cvar_t	*r_Ghoul2TimeBase;

extern refexport_t	re;

#define G2_MODEL_OK(g) ((g)&&(g)->mValid&&(g)->aHeader&&(g)->currentModel&&(g)->animModel)


#define G2_DEBUG_TIME (0)

static int G2TimeBases[NUM_G2T_TIME];

bool G2_TestModelPointers(CGhoul2Info *ghlInfo);

#if G2API_DEBUG
#include <float.h> //for isnan


#define MAX_ERROR_PRINTS (3)
class ErrorReporter
{
	std::string mName;
	std::map<std::string,int> mErrors;
public:
	ErrorReporter(const std::string &name) :
	  mName(name)
	{
	}
	~ErrorReporter()
	{
		char mess[1000];
		int total=0;
		sprintf(mess,"****** %s Error Report Begin******\n",mName.c_str());
		Com_DPrintf(mess);

		std::map<std::string,int>::iterator i;
		for (i=mErrors.begin();i!=mErrors.end();i++)
		{
			total+=(*i).second;
			sprintf(mess,"%s (hits %d)\n",(*i).first.c_str(),(*i).second);
			Com_DPrintf(mess);
		}

		sprintf(mess,"****** %s Error Report End   %d errors of %ld kinds******\n",mName.c_str(),total,mErrors.size());
		Com_DPrintf(mess);
	}
	int AnimTest(CGhoul2Info_v &ghoul2,const char *m,const char *, int line)
	{
		if (G2_SetupModelPointers(ghoul2))
		{
			int i;
			for (i=0; i<ghoul2.size(); i++)
			{
				AnimTest(&ghoul2[i],m,0,line);
			}
			return i;
		}
		return 666; //these return values are to saisfy the optimizer
	}
	int AnimTest(CGhoul2Info *ghlInfo,const char *m,const char *, int line)
	{
		bool ok=G2_TestModelPointers(ghlInfo);
		if (!ok)
		{
			return 5; // I guess this happens from time to time
		}
		size_t i;
		int ret=0;

		char GLAName1[1000];
		char GLAName2[1000];
		char GLMName1[1000];
		char GLMName2[1000];

		strcpy(GLAName1,ghlInfo->animModel->name);
		strcpy(GLAName2,ghlInfo->aHeader->name);
		strcpy(GLMName1,ghlInfo->mFileName);
		strcpy(GLMName2,ghlInfo->currentModel->name);

		int numFramesInFile=ghlInfo->aHeader->numFrames;

		int numActiveBones=0;
		for (i=0;i<ghlInfo->mBlist.size();i++)
		{
			if (ghlInfo->mBlist[i].boneNumber!=-1) // slot used?
			{
				if (ghlInfo->mBlist[i].flags&BONE_ANIM_TOTAL) // anim on this?
				{
					numActiveBones++;
					bool loop=!!(ghlInfo->mBlist[i].flags&BONE_ANIM_OVERRIDE_LOOP);
					bool not_loop=!!(ghlInfo->mBlist[i].flags&BONE_ANIM_OVERRIDE);

					if (loop==not_loop)
					{
						Error("Unusual animation flags, should have some sort of override, but not both",1,0,line);
					}

					bool freeze=(ghlInfo->mBlist[i].flags&BONE_ANIM_OVERRIDE_FREEZE) == BONE_ANIM_OVERRIDE_FREEZE;

					if (loop&&freeze)
					{
						Error("Unusual animation flags, loop and freeze",1,0,line);
					}
					bool no_lerp=!!(ghlInfo->mBlist[i].flags&BONE_ANIM_NO_LERP);
					bool blend=!!(ghlInfo->mBlist[i].flags&BONE_ANIM_BLEND);


					//comments according to jake
					int			startFrame=ghlInfo->mBlist[i].startFrame;		// start frame for animation
					int			endFrame=ghlInfo->mBlist[i].endFrame;		// end frame for animation NOTE anim actually ends on endFrame+1
					int			startTime=ghlInfo->mBlist[i].startTime;		// time we started this animation
					int			pauseTime=ghlInfo->mBlist[i].pauseTime;		// time we paused this animation - 0 if not paused
					float		animSpeed=ghlInfo->mBlist[i].animSpeed;		// speed at which this anim runs. 1.0f means full speed of animation incoming - ie if anim is 20hrtz, we run at 20hrts. If 5hrts, we run at 5 hrts

					float		blendFrame=0.0f;		// frame PLUS LERP value to blend
					int			blendLerpFrame=0;	// frame to lerp the blend frame with.

					if (blend)
					{
						blendFrame=ghlInfo->mBlist[i].blendFrame;
						blendLerpFrame=ghlInfo->mBlist[i].blendLerpFrame;
						if (floor(blendFrame)<0.0f)
						{
							Error("negative blendFrame",1,0,line);
						}
						if (ceil(blendFrame)>=float(numFramesInFile))
						{
							Error("blendFrame >= numFramesInFile",1,0,line);
						}
						if (blendLerpFrame<0)
						{
							Error("negative blendLerpFrame",1,0,line);
						}
						if (blendLerpFrame>=numFramesInFile)
						{
							Error("blendLerpFrame >= numFramesInFile",1,0,line);
						}
					}
					if (startFrame<0)
					{
						Error("negative startFrame",1,0,line);
					}
					if (startFrame>=numFramesInFile)
					{
						Error("startFrame >= numFramesInFile",1,0,line);
					}
					if (endFrame<0)
					{
						Error("negative endFrame",1,0,line);
					}
					if (endFrame==0&&animSpeed>0.0f)
					{
						Error("Zero endFrame",1,0,line);
					}
					if (endFrame>numFramesInFile)
					{
						Error("endFrame > numFramesInFile",1,0,line);
					}
					// mikeg call out here for further checks.
					ret=(int)startTime+(int)pauseTime+(int)no_lerp; // quiet VC.
				}
			}
		}
		return ret;
	}
	int Error(const char *m,int kind,const char *, int line)
	{
		char mess[1000];
		assert(m);
		std::string full=mName;
		if (kind==2)
		{
			full+=":NOTE:     ";
		}
		else if (kind==1)
		{
//			assert(!"G2API Warning");
			full+=":WARNING:  ";
		}
		else
		{
//			assert(!"G2API Error");
			full+=":ERROR  :  ";
		}
		full+=m;
		sprintf(mess,"  [line %d]",line);
		full+=mess;

		// assert(0);
		int ret=0; //place a breakpoint here
		std::map<std::string,int>::iterator f=mErrors.find(full);
		if (f==mErrors.end())
		{
			ret++; // or a breakpoint here for the first occurance
			mErrors.insert(std::make_pair(full,0));
			f=mErrors.find(full);
		}
		assert(f!=mErrors.end());
		(*f).second++;
		if ((*f).second==1000)
		{
			ret*=-1; // breakpoint to find a specific occurance of an error
		}
		if ((*f).second<=MAX_ERROR_PRINTS&&kind<2)
		{
			Com_Printf("%s (hit # %d)\n",full.c_str(),(*f).second);
			if (1)
			{
				sprintf(mess,"%s (hit # %d)\n",full.c_str(),(*f).second);
				Com_DPrintf(mess);
			}
		}
		return ret;
	}
};

#include "assert.h"
ErrorReporter &G2APIError()
{
	static ErrorReporter singleton("G2API");
	return singleton;
}

#define G2ERROR(exp,m) (void)( (exp) || (G2APIError().Error(m,0,__FILE__,__LINE__), 0) )
#define G2WARNING(exp,m) (void)( (exp) || (G2APIError().Error(m,1,__FILE__,__LINE__), 0) )
#define G2NOTE(exp,m) (void)( (exp) || (G2APIError().Error(m,2,__FILE__,__LINE__), 0) )
#define G2ANIM(ghlInfo,m) (void)((G2APIError().AnimTest(ghlInfo,m,__FILE__,__LINE__), 0) )
#else

#define G2ERROR(exp,m)		((void)0)
#define G2WARNING(exp,m)     ((void)0)
#define G2NOTE(exp,m)     ((void)0)
#define G2ANIM(ghlInfo,m) ((void)0)

#endif

#ifdef _DEBUG
void G2_Bone_Not_Found(const char *boneName,const char *modName)
{
	G2ERROR(boneName,"NULL Bone Name");
	G2ERROR(boneName[0],"Empty Bone Name");
	if (boneName)
	{
		G2NOTE(0,va("Bone Not Found (%s:%s)",boneName,modName));
	}
}

void G2_Bolt_Not_Found(const char *boneName,const char *modName)
{
	G2ERROR(boneName,"NULL Bolt/Bone Name");
	G2ERROR(boneName[0],"Empty Bolt/Bone Name");
	if (boneName)
	{
		G2NOTE(0,va("Bolt/Bone Not Found (%s:%s)",boneName,modName));
	}
}
#endif

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


// must be a power of two
#define MAX_G2_MODELS (512)
#define G2_MODEL_BITS (9)
#define G2_INDEX_MASK (MAX_G2_MODELS-1)

void RemoveBoneCache(CBoneCache *boneCache);

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
	std::vector<CGhoul2Info>	mInfos[MAX_G2_MODELS];
	int					mIds[MAX_G2_MODELS];
	std::list<int>			mFreeIndecies;
	void DeleteLow(int idx)
	{
		{
			size_t	model;
			for (model=0; model< mInfos[idx].size(); model++)
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
		size_t i;
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
		if (mFreeIndecies.size()<MAX_G2_MODELS)
		{
#if G2API_DEBUG
			char mess[1000];
			sprintf(mess,"************************\nLeaked %ld ghoul2info slots\n", MAX_G2_MODELS - mFreeIndecies.size());
			Com_DPrintf(mess);
#endif
			int i;
			for (i=0;i<MAX_G2_MODELS;i++)
			{
				std::list<int>::iterator j;
				for (j=mFreeIndecies.begin();j!=mFreeIndecies.end();j++)
				{
					if (*j==i)
						break;
				}
				if (j==mFreeIndecies.end())
				{
#if G2API_DEBUG
					sprintf(mess,"Leaked Info idx=%d id=%d sz=%ld\n", i, mIds[i], mInfos[i].size());
					Com_DPrintf(mess);
					if (mInfos[i].size())
					{
						sprintf(mess,"%s\n", mInfos[i][0].mFileName);
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
		if (!handle)
		{
			return false;
		}
		assert(handle>0); //negative handle???
		assert((handle&G2_INDEX_MASK)>=0&&(handle&G2_INDEX_MASK)<MAX_G2_MODELS); //junk handle
		if (mIds[handle&G2_INDEX_MASK]!=handle) // not a valid handle, could be old
		{
			return false;
		}
		return true;
	}
	void Delete(int handle)
	{
		if (!handle)
		{
			return;
		}
		assert(handle>0); //null handle
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
		assert (!(handle<=0||(handle&G2_INDEX_MASK)<0||(handle&G2_INDEX_MASK)>=MAX_G2_MODELS||mIds[handle&G2_INDEX_MASK]!=handle));

		return mInfos[handle&G2_INDEX_MASK];
	}
	const std::vector<CGhoul2Info> &Get(int handle) const
	{
		assert(handle>0);
		assert(mIds[handle&G2_INDEX_MASK]==handle); // not a valid handle, could be old or garbage
		return mInfos[handle&G2_INDEX_MASK];
	}

#if G2API_DEBUG
	std::vector<CGhoul2Info> &GetDebug(int handle)
	{
		assert (!(handle<=0||(handle&G2_INDEX_MASK)<0||(handle&G2_INDEX_MASK)>=MAX_G2_MODELS||mIds[handle&G2_INDEX_MASK]!=handle));

		return mInfos[handle&G2_INDEX_MASK];
	}
	void TestAllAnims()
	{
		for (size_t j=0;j<MAX_G2_MODELS;j++)
		{
			std::vector<CGhoul2Info> &ghoul2=mInfos[j];
			for (size_t i=0; i<ghoul2.size(); i++)
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
std::vector<CGhoul2Info> &DebugG2Info(int handle)
{
	return ((Ghoul2InfoArray *)(&TheGhoul2InfoArray()))->GetDebug(handle);
}

CGhoul2Info &DebugG2InfoI(int handle,int item)
{
	return ((Ghoul2InfoArray *)(&TheGhoul2InfoArray()))->GetDebug(handle)[item];
}

void TestAllGhoul2Anims()
{
	((Ghoul2InfoArray *)(&TheGhoul2InfoArray()))->TestAllAnims();
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
			singleton->Deserialize ((const char *)data, size);
		Z_Free ((void *)data);

		assert (read == size);
	}
}

void SaveGhoul2InfoArray()
{
	size_t size = singleton->GetSerializedSize();
	void *data = Z_Malloc (size, TAG_GHOUL2, qfalse);
#ifdef _DEBUG
	size_t written =
#endif // _DEBUG
		singleton->Serialize ((char *)data);

	assert (written == size);

	if ( !ri.PD_Store (PERSISTENT_G2DATA, data, size) )
	{
		Com_Printf (S_COLOR_RED "ERROR: Failed to store persistent renderer data.\n");
	}
}

// this is the ONLY function to read entity states directly
void G2API_CleanGhoul2Models(CGhoul2Info_v &ghoul2)
{
#ifdef _G2_GORE
	G2API_ClearSkinGore ( ghoul2 );
#endif
	ghoul2.~CGhoul2Info_v();
}

qhandle_t G2API_PrecacheGhoul2Model(const char *fileName)
{
	return RE_RegisterModel((char *)fileName);
}

// initialise all that needs to be on a new Ghoul II model
int G2API_InitGhoul2Model(CGhoul2Info_v &ghoul2, const char *fileName, int, qhandle_t customSkin,
						  qhandle_t customShader, int modelFlags, int lodBias)
{
	int				model = -1;

	G2ERROR(fileName&&fileName[0],"NULL filename");

	if (!fileName||!fileName[0])
	{
		assert(fileName[0]);
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
	if (model==ghoul2.size())
	{
		assert(model < 8);	//arb, just catching run-away models
		CGhoul2Info info;
		Q_strncpyz(info.mFileName, fileName, sizeof(info.mFileName));
		info.mModelindex = 0;
		if(G2_TestModelPointers(&info)) {
		ghoul2.push_back(CGhoul2Info());
		} else {
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
	G2ERROR(ghlInfo,"NULL ghlInfo");
	if (ghlInfo)
	{
		ghlInfo->mLodBias = lodBias;
		return qtrue;
	}
	return qfalse;
}

qboolean G2API_SetSkin(CGhoul2Info *ghlInfo, qhandle_t customSkin, qhandle_t renderSkin)
{
	G2ERROR(ghlInfo,"NULL ghlInfo");
#ifdef JK2_MODE
	if (ghlInfo)
	{
		ghlInfo->mCustomSkin = customSkin;
		return qtrue;
	}
	return qfalse;
#else
	if (ghlInfo)
	{
		ghlInfo->mCustomSkin = customSkin;
		if (renderSkin)
		{//this is going to set the surfs on/off matching the skin file
extern void G2API_SetSurfaceOnOffFromSkin (CGhoul2Info *ghlInfo, qhandle_t renderSkin);	//tr_ghoul2.cpp
			G2API_SetSurfaceOnOffFromSkin( ghlInfo, renderSkin );
		}
		return qtrue;
	}
#endif
	return qfalse;
}

qboolean G2API_SetShader(CGhoul2Info *ghlInfo, qhandle_t customShader)
{
	G2ERROR(ghlInfo,"NULL ghlInfo");
	if (ghlInfo)
	{
		ghlInfo->mCustomShader = customShader;
		return qtrue;
	}
	return qfalse;
}

qboolean G2API_SetSurfaceOnOff(CGhoul2Info *ghlInfo, const char *surfaceName, const int flags)
{
	if (G2_SetupModelPointers(ghlInfo))
	{
		G2ERROR(!(flags&~(G2SURFACEFLAG_OFF | G2SURFACEFLAG_NODESCENDANTS)),"G2API_SetSurfaceOnOff Illegal Flags");
		// ensure we flush the cache
		ghlInfo->mMeshFrameNum = 0;
		return G2_SetSurfaceOnOff(ghlInfo, surfaceName, flags);
	}
	return qfalse;
}


qboolean G2API_SetRootSurface(CGhoul2Info_v &ghlInfo, const int modelIndex, const char *surfaceName)
{
	G2ERROR(ghlInfo.IsValid(),"Invalid ghlInfo");
	G2ERROR(surfaceName,"Invalid surfaceName");
	if (G2_SetupModelPointers(ghlInfo))
	{
		G2ERROR(modelIndex>=0&&modelIndex<ghlInfo.size(),"Bad Model Index");
		if (modelIndex>=0&&modelIndex<ghlInfo.size())
		{
			return G2_SetRootSurface(ghlInfo, modelIndex,  surfaceName);
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

int G2API_GetSurfaceRenderStatus(CGhoul2Info *ghlInfo, const char *surfaceName)
{
	G2ERROR(surfaceName,"Invalid surfaceName");
	if (G2_SetupModelPointers(ghlInfo))
	{
		return G2_IsSurfaceRendered(ghlInfo, surfaceName, ghlInfo->mSlist);
	}
	return -1;
}

qboolean G2API_RemoveGhoul2Model(CGhoul2Info_v &ghlInfo, const int modelIndex)
{
	// sanity check
	if (!ghlInfo.size() || (ghlInfo.size() <= modelIndex) || modelIndex < 0 || (ghlInfo[modelIndex].mModelindex <0))
	{
		// if we hit this assert then we are trying to delete a ghoul2 model on a ghoul2 instance that
		// one way or another is already gone.
		G2ERROR(0,"Remove Nonexistant Model");
		assert(0 && "remove non existing model");
		return qfalse;
	}

#ifdef _G2_GORE
	// Cleanup the gore attached to this model
	if ( ghlInfo[modelIndex].mGoreSetTag )
	{
		DeleteGoreSet ( ghlInfo[modelIndex].mGoreSetTag );
		ghlInfo[modelIndex].mGoreSetTag = 0;
	}
#endif

	RemoveBoneCache(ghlInfo[modelIndex].mBoneCache);
	ghlInfo[modelIndex].mBoneCache=0;

	 // set us to be the 'not active' state
	ghlInfo[modelIndex].mModelindex = -1;
	ghlInfo[modelIndex].mFileName[0]=0;

	ghlInfo[modelIndex]=CGhoul2Info();
	return qtrue;
}

//rww - RAGDOLL_BEGIN
#define		GHOUL2_RAG_STARTED						0x0010
#define		GHOUL2_RAG_FORCESOLVE					0x1000		//api-override, determine if ragdoll should be forced to continue solving even if it thinks it is settled
//rww - RAGDOLL_END

int		 G2API_GetAnimIndex(CGhoul2Info *ghlInfo)
{
	if (ghlInfo)
	{
		return ghlInfo->animModelIndexOffset;
	}
	return 0;
}

qboolean G2API_SetAnimIndex(CGhoul2Info *ghlInfo, const int index)
{
	// Is This A Valid G2 Model?
	//---------------------------
	if (ghlInfo)
	{
		// Is This A New Anim Index?
		//---------------------------
		if (ghlInfo->animModelIndexOffset != index)
		{
			ghlInfo->animModelIndexOffset = index;
			ghlInfo->currentAnimModelSize = 0;					// Clear anim size so SetupModelPointers recalcs

//			RemoveBoneCache(ghlInfo[0].mBoneCache);
//			ghlInfo[0].mBoneCache=0;

			// Kill All Existing Animation, Blending, Etc.
			//---------------------------------------------
			for (size_t index=0; index<ghlInfo->mBlist.size(); index++)
			{
				ghlInfo->mBlist[index].flags &= ~(BONE_ANIM_TOTAL);
				ghlInfo->mBlist[index].flags &= ~(BONE_ANGLES_TOTAL);
//				G2_Remove_Bone_Index(ghlInfo->mBlist, index);
			}
		}
		return qtrue;
	}
	return qfalse;
}

qboolean G2API_SetBoneAnimIndex(CGhoul2Info *ghlInfo, const int index, const int startFrame, const int endFrame, const int flags, const float animSpeed, const int AcurrentTime, const float setFrame, const int blendTime)
{
	//rww - RAGDOLL_BEGIN
	if (ghlInfo && (ghlInfo->mFlags & GHOUL2_RAG_STARTED))
	{
		return qfalse;
	}
	//rww - RAGDOLL_END

	qboolean ret=qfalse;
	if (G2_SetupModelPointers(ghlInfo))
	{
		G2ERROR(startFrame>=0,"startframe<0");
		G2ERROR(startFrame<ghlInfo->aHeader->numFrames,"startframe>=numframes");
		G2ERROR(endFrame>0,"endframe<=0");
		G2ERROR(endFrame<=ghlInfo->aHeader->numFrames,"endframe>numframes");
		G2ERROR(setFrame<ghlInfo->aHeader->numFrames,"setframe>=numframes");
		G2ERROR(setFrame==-1.0f||setFrame>=0.0f,"setframe<0 but not -1");
		if (startFrame<0||startFrame>=ghlInfo->aHeader->numFrames)
		{
			*(int *)&startFrame=0; // cast away const
		}
		if (endFrame<=0||endFrame>ghlInfo->aHeader->numFrames)
		{
			*(int *)&endFrame=1;
		}
		if (setFrame!=-1.0f&&(setFrame<0.0f||setFrame>=(float)ghlInfo->aHeader->numFrames))
		{
			*(float *)&setFrame=0.0f;
		}
		ghlInfo->mSkelFrameNum = 0;
		G2ERROR(index>=0&&index<(int)ghlInfo->mBlist.size(),va("Out of Range Bone Index (%s)",ghlInfo->mFileName));
		if (index>=0&&index<(int)ghlInfo->mBlist.size())
		{
			G2ERROR(ghlInfo->mBlist[index].boneNumber>=0,va("Bone Index is not Active (%s)",ghlInfo->mFileName));
			int currentTime=G2API_GetTime(AcurrentTime);
#if 0
			/*
			if ( ge->ValidateAnimRange( startFrame, endFrame, animSpeed ) == -1 )
			{
				int wtf = 1;
			}
			*/
#endif
 			ret=G2_Set_Bone_Anim_Index(ghlInfo->mBlist, index, startFrame, endFrame, flags, animSpeed, currentTime, setFrame, blendTime,ghlInfo->aHeader->numFrames);
			G2ANIM(ghlInfo,"G2API_SetBoneAnimIndex");
		}
	}
	G2WARNING(ret,va("G2API_SetBoneAnimIndex Failed (%s)",ghlInfo->mFileName));
	return ret;
}

qboolean G2API_SetBoneAnim(CGhoul2Info *ghlInfo, const char *boneName, const int startFrame, const int endFrame, const int flags, const float animSpeed, const int AcurrentTime, const float setFrame, const int blendTime)
{
	//rww - RAGDOLL_BEGIN
	if (ghlInfo && ghlInfo->mFlags & GHOUL2_RAG_STARTED)
	{
		return qfalse;
	}
	//rww - RAGDOLL_END

	qboolean ret=qfalse;
	G2ERROR(boneName,"NULL boneName");
	if (boneName&&G2_SetupModelPointers(ghlInfo))
	{
		G2ERROR(startFrame>=0,"startframe<0");
		G2ERROR(startFrame<ghlInfo->aHeader->numFrames,"startframe>=numframes");
		G2ERROR(endFrame>0,"endframe<=0");
		G2ERROR(endFrame<=ghlInfo->aHeader->numFrames,"endframe>numframes");
		G2ERROR(setFrame<ghlInfo->aHeader->numFrames,"setframe>=numframes");
		G2ERROR(setFrame==-1.0f||setFrame>=0.0f,"setframe<0 but not -1");
		if (startFrame<0||startFrame>=ghlInfo->aHeader->numFrames)
		{
			*(int *)&startFrame=0; // cast away const
		}
		if (endFrame<=0||endFrame>ghlInfo->aHeader->numFrames)
		{
			*(int *)&endFrame=1;
		}
		if (setFrame!=-1.0f&&(setFrame<0.0f||setFrame>=(float)ghlInfo->aHeader->numFrames))
		{
			*(float *)&setFrame=0.0f;
		}
		ghlInfo->mSkelFrameNum = 0;
		int currentTime=G2API_GetTime(AcurrentTime);
 		ret=G2_Set_Bone_Anim(ghlInfo, ghlInfo->mBlist, boneName, startFrame, endFrame, flags, animSpeed, currentTime, setFrame, blendTime);
		G2ANIM(ghlInfo,"G2API_SetBoneAnim");
	}
	G2WARNING(ret,"G2API_SetBoneAnim Failed");
	return ret;
}

qboolean G2API_GetBoneAnim(CGhoul2Info *ghlInfo, const char *boneName, const int AcurrentTime, float *currentFrame,
						   int *startFrame, int *endFrame, int *flags, float *animSpeed, int *)
{
	qboolean ret=qfalse;
	G2ERROR(boneName,"NULL boneName");
	if (G2_SetupModelPointers(ghlInfo))
	{
		int currentTime=G2API_GetTime(AcurrentTime);
 		ret=G2_Get_Bone_Anim(ghlInfo, ghlInfo->mBlist, boneName, currentTime, currentFrame,
			startFrame, endFrame, flags, animSpeed);
	}
	G2WARNING(ret,"G2API_GetBoneAnim Failed");
	return ret;
}

qboolean G2API_GetBoneAnimIndex(CGhoul2Info *ghlInfo, const int iBoneIndex, const int AcurrentTime, float *currentFrame,
						   int *startFrame, int *endFrame, int *flags, float *animSpeed, int *)
{
	qboolean ret=qfalse;
	if (G2_SetupModelPointers(ghlInfo))
	{
		int currentTime=G2API_GetTime(AcurrentTime);
		G2NOTE(iBoneIndex>=0&&iBoneIndex<(int)ghlInfo->mBlist.size(),va("Bad Bone Index (%d:%s)",iBoneIndex,ghlInfo->mFileName));
		if (iBoneIndex>=0&&iBoneIndex<(int)ghlInfo->mBlist.size())
		{
			G2NOTE(ghlInfo->mBlist[iBoneIndex].flags & (BONE_ANIM_OVERRIDE_LOOP | BONE_ANIM_OVERRIDE),"GetBoneAnim on non-animating bone.");
			if ((ghlInfo->mBlist[iBoneIndex].flags & (BONE_ANIM_OVERRIDE_LOOP | BONE_ANIM_OVERRIDE)))
			{
				int sf,ef;
				ret=G2_Get_Bone_Anim_Index(	ghlInfo->mBlist,// boneInfo_v &blist,
											iBoneIndex,		// const int index,
											currentTime,	// const int currentTime,
											currentFrame,	// float *currentFrame,
											&sf,		// int *startFrame,
											&ef,		// int *endFrame,
											flags,			// int *flags,
											animSpeed,		// float *retAnimSpeed,
											ghlInfo->aHeader->numFrames
											);
				G2ERROR(sf>=0,"returning startframe<0");
				G2ERROR(sf<ghlInfo->aHeader->numFrames,"returning startframe>=numframes");
				G2ERROR(ef>0,"returning endframe<=0");
				G2ERROR(ef<=ghlInfo->aHeader->numFrames,"returning endframe>numframes");
				if (currentFrame)
				{
					G2ERROR(*currentFrame>=0.0f,"returning currentframe<0");
					G2ERROR(((int)(*currentFrame))<ghlInfo->aHeader->numFrames,"returning currentframe>=numframes");
				}
				if (endFrame)
				{
					*endFrame=ef;
				}
				if (startFrame)
				{
					*startFrame=sf;
				}
				G2ANIM(ghlInfo,"G2API_GetBoneAnimIndex");
			}
		}
	}
	if (!ret)
	{
		*endFrame=1;
		*startFrame=0;
		*flags=0;
		*currentFrame=0.0f;
		*animSpeed=1.0f;
	}
	G2NOTE(ret,"G2API_GetBoneAnimIndex Failed");
	return ret;
}

qboolean G2API_GetAnimRange(CGhoul2Info *ghlInfo, const char *boneName,	int *startFrame, int *endFrame)
{
	qboolean ret=qfalse;
	G2ERROR(boneName,"NULL boneName");
	if (boneName&&G2_SetupModelPointers(ghlInfo))
	{
 		ret=G2_Get_Bone_Anim_Range(ghlInfo, ghlInfo->mBlist, boneName, startFrame, endFrame);
		G2ANIM(ghlInfo,"G2API_GetAnimRange");
	}
//	looks like the game checks the return value
//	G2WARNING(ret,"G2API_GetAnimRange Failed");
	return ret;
}

qboolean G2API_GetAnimRangeIndex(CGhoul2Info *ghlInfo, const int boneIndex, int *startFrame, int *endFrame)
{
	qboolean ret=qfalse;
	if (G2_SetupModelPointers(ghlInfo))
	{
		G2ERROR(boneIndex>=0&&boneIndex<(int)ghlInfo->mBlist.size(),"Bad Bone Index");
		if (boneIndex>=0&&boneIndex<(int)ghlInfo->mBlist.size())
		{
 			ret=G2_Get_Bone_Anim_Range_Index(ghlInfo->mBlist, boneIndex, startFrame, endFrame);
			G2ANIM(ghlInfo,"G2API_GetAnimRange");
		}
	}
//	looks like the game checks the return value
//	G2WARNING(ret,"G2API_GetAnimRangeIndex Failed");
	return ret;
}

qboolean G2API_PauseBoneAnim(CGhoul2Info *ghlInfo, const char *boneName, const int AcurrentTime)
{
	qboolean ret=qfalse;
	G2ERROR(boneName,"NULL boneName");
	if (boneName&&G2_SetupModelPointers(ghlInfo))
	{
		int currentTime=G2API_GetTime(AcurrentTime);
 		ret=G2_Pause_Bone_Anim(ghlInfo, ghlInfo->mBlist, boneName, currentTime);
		G2ANIM(ghlInfo,"G2API_PauseBoneAnim");
	}
	G2NOTE(ret,"G2API_PauseBoneAnim Failed");
	return ret;
}

qboolean G2API_PauseBoneAnimIndex(CGhoul2Info *ghlInfo, const int boneIndex, const int AcurrentTime)
{
	qboolean ret=qfalse;
	if (G2_SetupModelPointers(ghlInfo))
	{
		int currentTime=G2API_GetTime(AcurrentTime);
		G2ERROR(boneIndex>=0&&boneIndex<(int)ghlInfo->mBlist.size(),"Bad Bone Index");
		if (boneIndex>=0&&boneIndex<(int)ghlInfo->mBlist.size())
		{
	 		ret=G2_Pause_Bone_Anim_Index(ghlInfo->mBlist, boneIndex, currentTime,ghlInfo->aHeader->numFrames);
			G2ANIM(ghlInfo,"G2API_PauseBoneAnimIndex");
		}
	}
	G2WARNING(ret,"G2API_PauseBoneAnimIndex Failed");
	return ret;
}

qboolean	G2API_IsPaused(CGhoul2Info *ghlInfo, const char *boneName)
{
	qboolean ret=qfalse;
	G2ERROR(boneName,"NULL boneName");
	if (boneName&&G2_SetupModelPointers(ghlInfo))
	{
 		ret=G2_IsPaused(ghlInfo, ghlInfo->mBlist, boneName);
	}
	G2WARNING(ret,"G2API_IsPaused Failed");
	return ret;
}

qboolean G2API_StopBoneAnimIndex(CGhoul2Info *ghlInfo, const int index)
{
	qboolean ret=qfalse;
	G2ERROR(ghlInfo,"NULL ghlInfo");
	if (G2_SetupModelPointers(ghlInfo))
	{
		G2ERROR(index>=0&&index<(int)ghlInfo->mBlist.size(),"Bad Bone Index");
		if (index>=0&&index<(int)ghlInfo->mBlist.size())
		{
 			ret=G2_Stop_Bone_Anim_Index(ghlInfo->mBlist, index);
			G2ANIM(ghlInfo,"G2API_StopBoneAnimIndex");
		}
	}
	//G2WARNING(ret,"G2API_StopBoneAnimIndex Failed");
	return ret;
}

qboolean G2API_StopBoneAnim(CGhoul2Info *ghlInfo, const char *boneName)
{
	qboolean ret=qfalse;
	G2ERROR(boneName,"NULL boneName");
	if (boneName&&G2_SetupModelPointers(ghlInfo))
	{
 		ret=G2_Stop_Bone_Anim(ghlInfo, ghlInfo->mBlist, boneName);
		G2ANIM(ghlInfo,"G2API_StopBoneAnim");
	}
	G2WARNING(ret,"G2API_StopBoneAnim Failed");
	return ret;
}

qboolean G2API_SetBoneAnglesIndex(CGhoul2Info *ghlInfo, const int index, const vec3_t angles, const int flags,
							 const Eorientations yaw, const Eorientations pitch, const Eorientations roll,
							 qhandle_t *, int blendTime, int AcurrentTime)
{
	//rww - RAGDOLL_BEGIN
	if (ghlInfo && ghlInfo->mFlags & GHOUL2_RAG_STARTED)
	{
		return qfalse;
	}
	//rww - RAGDOLL_END

	qboolean ret=qfalse;
	if (G2_SetupModelPointers(ghlInfo))
	{
		int currentTime=G2API_GetTime(AcurrentTime);
		// ensure we flush the cache
		ghlInfo->mSkelFrameNum = 0;
		G2ERROR(index>=0&&index<(int)ghlInfo->mBlist.size(),"G2API_SetBoneAnglesIndex:Invalid bone index");
		if (index>=0&&index<(int)ghlInfo->mBlist.size())
		{
			ret=G2_Set_Bone_Angles_Index(ghlInfo, ghlInfo->mBlist, index, angles, flags, yaw, pitch, roll,blendTime, currentTime);
		}
	}
	G2WARNING(ret,"G2API_SetBoneAnglesIndex Failed");
	return ret;
}

qboolean G2API_SetBoneAngles(CGhoul2Info *ghlInfo, const char *boneName, const vec3_t angles, const int flags,
							 const Eorientations up, const Eorientations left, const Eorientations forward,
							 qhandle_t *, int blendTime, int AcurrentTime )
{
	//rww - RAGDOLL_BEGIN
	if (ghlInfo && ghlInfo->mFlags & GHOUL2_RAG_STARTED)
	{
		return qfalse;
	}
	//rww - RAGDOLL_END

	qboolean ret=qfalse;
	G2ERROR(boneName,"NULL boneName");
	if (boneName&&G2_SetupModelPointers(ghlInfo))
	{
		int currentTime=G2API_GetTime(AcurrentTime);
			// ensure we flush the cache
		ghlInfo->mSkelFrameNum = 0;
		ret=G2_Set_Bone_Angles(ghlInfo, ghlInfo->mBlist, boneName, angles, flags, up, left, forward, blendTime, currentTime);
	}
	G2WARNING(ret,"G2API_SetBoneAngles Failed");
	return ret;
}

qboolean G2API_SetBoneAnglesMatrixIndex(CGhoul2Info *ghlInfo, const int index, const mdxaBone_t &matrix,
								   const int flags, qhandle_t *, int blendTime, int AcurrentTime)
{
	qboolean ret=qfalse;
	if (G2_SetupModelPointers(ghlInfo))
	{
		int currentTime=G2API_GetTime(AcurrentTime);
		// ensure we flush the cache
		ghlInfo->mSkelFrameNum = 0;
		G2ERROR(index>=0&&index<(int)ghlInfo->mBlist.size(),"Bad Bone Index");
		if (index>=0&&index<(int)ghlInfo->mBlist.size())
		{
			ret=G2_Set_Bone_Angles_Matrix_Index(ghlInfo->mBlist, index, matrix, flags, blendTime, currentTime);
		}
	}
	G2WARNING(ret,"G2API_SetBoneAnglesMatrixIndex Failed");
	return ret;
}

qboolean G2API_SetBoneAnglesMatrix(CGhoul2Info *ghlInfo, const char *boneName, const mdxaBone_t &matrix,
								   const int flags, qhandle_t *modelList, int blendTime, int AcurrentTime)
{
	qboolean ret=qfalse;
	G2ERROR(boneName,"NULL boneName");
	if (boneName&&G2_SetupModelPointers(ghlInfo))
	{
		int currentTime=G2API_GetTime(AcurrentTime);
		// ensure we flush the cache
		ghlInfo->mSkelFrameNum = 0;
		ret=G2_Set_Bone_Angles_Matrix(ghlInfo, ghlInfo->mBlist, boneName, matrix, flags, blendTime, currentTime);
	}
	G2WARNING(ret,"G2API_SetBoneAnglesMatrix Failed");
	return ret;
}

qboolean G2API_StopBoneAnglesIndex(CGhoul2Info *ghlInfo, const int index)
{
	qboolean ret=qfalse;
	if (G2_SetupModelPointers(ghlInfo))
	{
		// ensure we flush the cache
		ghlInfo->mSkelFrameNum = 0;
		G2ERROR(index>=0&&index<(int)ghlInfo->mBlist.size(),"Bad Bone Index");
		if (index>=0&&index<(int)ghlInfo->mBlist.size())
		{
	 		ret=G2_Stop_Bone_Angles_Index(ghlInfo->mBlist, index);
		}
	}
	G2WARNING(ret,"G2API_StopBoneAnglesIndex Failed");
	return ret;
}

qboolean G2API_StopBoneAngles(CGhoul2Info *ghlInfo, const char *boneName)
{
	qboolean ret=qfalse;
	G2ERROR(boneName,"NULL boneName");
	if (boneName&&G2_SetupModelPointers(ghlInfo))
	{
		// ensure we flush the cache
		ghlInfo->mSkelFrameNum = 0;
 		ret=G2_Stop_Bone_Angles(ghlInfo, ghlInfo->mBlist, boneName);
	}
	G2WARNING(ret,"G2API_StopBoneAngles Failed");
	return ret;
}

//rww - RAGDOLL_BEGIN
class CRagDollParams;
void G2_SetRagDoll(CGhoul2Info_v &ghoul2V,CRagDollParams *parms);
void G2API_SetRagDoll(CGhoul2Info_v &ghoul2,CRagDollParams *parms)
{
	G2_SetRagDoll(ghoul2,parms);
}
//rww - RAGDOLL_END

qboolean G2API_RemoveBone(CGhoul2Info *ghlInfo, const char *boneName)
{
	qboolean ret=qfalse;
	G2ERROR(boneName,"NULL boneName");
	if (boneName&&G2_SetupModelPointers(ghlInfo))
	{
		// ensure we flush the cache
		ghlInfo->mSkelFrameNum = 0;
 		ret=G2_Remove_Bone(ghlInfo, ghlInfo->mBlist, boneName);
		G2ANIM(ghlInfo,"G2API_RemoveBone");
	}
	G2WARNING(ret,"G2API_RemoveBone Failed");
	return ret;
}

//rww - RAGDOLL_BEGIN
#ifdef _DEBUG
extern int ragTraceTime;
extern int ragSSCount;
extern int ragTraceCount;
#endif

void G2API_AnimateG2Models(CGhoul2Info_v &ghoul2, int AcurrentTime,CRagDollUpdateParams *params)
{
#ifdef JK2_MODE
	return;			// handled elsewhere
#endif
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
//	Com_Printf("Rag trace time: %i (%i STARTSOLID, %i TOTAL)\n", ragTraceTime, ragSSCount, ragTraceCount);

//	assert(ragTraceTime < 15);
	//assert(ragTraceCount < 600);
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
	qboolean ret=qfalse;
	if (G2_SetupModelPointers(ghlInfo))
	{
 		ret=G2_Remove_Bolt( ghlInfo->mBltlist, index);
	}
	G2WARNING(ret,"G2API_RemoveBolt Failed");
	return ret;
}

int G2API_AddBolt(CGhoul2Info *ghlInfo, const char *boneName)
{
	int ret=-1;
	G2ERROR(boneName,"NULL boneName");
	if (boneName&&G2_SetupModelPointers(ghlInfo))
	{
		ret=G2_Add_Bolt(ghlInfo, ghlInfo->mBltlist, ghlInfo->mSlist, boneName);
		G2NOTE(ret>=0,va("G2API_AddBolt Failed (%s:%s)",boneName,ghlInfo->mFileName));
	}
	return ret;
}

int G2API_AddBoltSurfNum(CGhoul2Info *ghlInfo, const int surfIndex)
{
	int ret=-1;
	if (G2_SetupModelPointers(ghlInfo))
	{
		ret=G2_Add_Bolt_Surf_Num(ghlInfo, ghlInfo->mBltlist, ghlInfo->mSlist, surfIndex);
	}
	G2WARNING(ret>=0,"G2API_AddBoltSurfNum Failed");
	return ret;
}


qboolean G2API_AttachG2Model(CGhoul2Info *ghlInfo, CGhoul2Info *ghlInfoTo, int toBoltIndex, int toModel)
{
	qboolean ret=qfalse;
	if (G2_SetupModelPointers(ghlInfo)&&G2_SetupModelPointers(ghlInfoTo))
	{
		G2ERROR(toBoltIndex>=0&&toBoltIndex<(int)ghlInfoTo->mBltlist.size(),"Invalid Bolt Index");
		G2ERROR(ghlInfoTo->mBltlist.size()>0,"Empty Bolt List");
		assert( toBoltIndex >= 0 );
		if ( toBoltIndex >= 0 && ghlInfoTo->mBltlist.size())
		{
			// make sure we have a model to attach, a model to attach to, and a bolt on that model
			if (((ghlInfoTo->mBltlist[toBoltIndex].boneNumber != -1) || (ghlInfoTo->mBltlist[toBoltIndex].surfaceNumber != -1)))
			{
				// encode the bolt address into the model bolt link
			   toModel &= MODEL_AND;
			   toBoltIndex &= BOLT_AND;
			   ghlInfo->mModelBoltLink = (toModel << MODEL_SHIFT)  | (toBoltIndex << BOLT_SHIFT);
			   ret=qtrue;
			}
		}
	}
	G2WARNING(ret,"G2API_AttachG2Model Failed");
	return ret;
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

qboolean G2API_AttachEnt(int *boltInfo, CGhoul2Info *ghlInfoTo, int toBoltIndex, int entNum, int toModelNum)
{
	qboolean ret=qfalse;
	G2ERROR(boltInfo,"NULL boltInfo");
	if (boltInfo&&G2_SetupModelPointers(ghlInfoTo))
	{
		// make sure we have a model to attach, a model to attach to, and a bolt on that model
		if ( ghlInfoTo->mBltlist.size() && ((ghlInfoTo->mBltlist[toBoltIndex].boneNumber != -1) || (ghlInfoTo->mBltlist[toBoltIndex].surfaceNumber != -1)))
		{
			// encode the bolt address into the model bolt link
		   toModelNum &= MODEL_AND;
		   toBoltIndex &= BOLT_AND;
		   entNum &= ENTITY_AND;
		   *boltInfo =  (toBoltIndex << BOLT_SHIFT) | (toModelNum << MODEL_SHIFT) | (entNum << ENTITY_SHIFT);
		   ret=qtrue;
		}
	}
	G2WARNING(ret,"G2API_AttachEnt Failed");
	return ret;
}

void G2API_DetachEnt(int *boltInfo)
{
	G2ERROR(boltInfo,"NULL boltInfo");
	if (boltInfo)
	{
		*boltInfo = 0;
	}
}


bool G2_NeedsRecalc(CGhoul2Info *ghlInfo,int frameNum);

qboolean G2API_GetBoltMatrix(CGhoul2Info_v &ghoul2, const int modelIndex, const int boltIndex, mdxaBone_t *matrix, const vec3_t angles,
							 const vec3_t position, const int AframeNum, qhandle_t *modelList, const vec3_t scale )
{
	G2ERROR(ghoul2.IsValid(),"Invalid ghlInfo");
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
			int frameNum=G2API_GetTime(AframeNum);
			CGhoul2Info *ghlInfo = &ghoul2[modelIndex];
			G2ERROR(boltIndex >= 0 && (boltIndex < (int)ghlInfo->mBltlist.size()),va("Invalid Bolt Index (%d:%s)",boltIndex,ghlInfo->mFileName));

			if (boltIndex >= 0 && ghlInfo && (boltIndex < (int)ghlInfo->mBltlist.size()) )
			{
				mdxaBone_t bolt;

				if (G2_NeedsRecalc(ghlInfo,frameNum))
				{
					G2_ConstructGhoulSkeleton(ghoul2,frameNum,true,scale);
				}

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
						assert( !Q_isnan(matrix->matrix[i][j]));
					}
				}
#endif// _DEBUG
				G2ANIM(ghlInfo,"G2API_GetBoltMatrix");
				return qtrue;
			}
		}
	}
	else
	{
		G2WARNING(0,"G2API_GetBoltMatrix Failed on empty or bad model");
	}
	Multiply_3x4Matrix(matrix, &worldMatrix, &identityMatrix);
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
	return !!ghoul2.IsValid();
}

// run through the Ghoul2 models and set each of the mModel values to the correct one from the cgs.gameModel offset lsit
void G2API_SetGhoul2ModelIndexes(CGhoul2Info_v &ghoul2, qhandle_t *modelList, qhandle_t *skinList)
{
	G2ERROR(ghoul2.IsValid(),"Invalid ghlInfo");
	int i;
	for (i=0; i<ghoul2.size(); i++)
	{
		if (ghoul2[i].mModelindex != -1)
		{
			ghoul2[i].mSkin = skinList[ghoul2[i].mCustomSkin];
		}
	}
}


char *G2API_GetAnimFileNameIndex(qhandle_t modelIndex)
{
	model_t		*mod_m = R_GetModelByHandle(modelIndex);
	G2ERROR(mod_m&&mod_m->mdxm,"Bad Model");
	if (mod_m&&mod_m->mdxm)
	{
		return mod_m->mdxm->animName;
	}
	return "";
}

// as above, but gets the internal embedded name, not the name of the disk file.
// This is needed for some unfortunate jiggery-hackery to do with frameskipping & the animevents.cfg file
//
char *G2API_GetAnimFileInternalNameIndex(qhandle_t modelIndex)
{
	model_t		*mod_a = R_GetModelByHandle(modelIndex);
	G2ERROR(mod_a&&mod_a->mdxa,"Bad Model");
	if (mod_a&&mod_a->mdxa)
	{
		return mod_a->mdxa->name;
	}
	return "";
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
	qboolean ret=qfalse;
	if (G2_SetupModelPointers(ghlInfo))
	{
		ret=G2_GetAnimFileName(ghlInfo->mFileName, filename);
	}
	G2WARNING(ret,"G2API_GetAnimFileName Failed");
	return ret;
}

/*
=======================
SV_QsortEntityNumbers
=======================
*/
static int QDECL QsortDistance( const void *a, const void *b ) {
	const float	&ea = ((CCollisionRecord*)a)->mDistance;
	const float	&eb = ((CCollisionRecord*)b)->mDistance;

	if ( ea < eb ) {
		return -1;
	}
	return 1;
}


void G2API_CollisionDetect(CCollisionRecord *collRecMap, CGhoul2Info_v &ghoul2, const vec3_t angles, const vec3_t position,
							int AframeNumber, int entNum, vec3_t rayStart, vec3_t rayEnd, vec3_t scale, CMiniHeap *,
							EG2_Collision eG2TraceType, int useLod, float fRadius)
{
	G2ERROR(ghoul2.IsValid(),"Invalid ghlInfo");
	G2ERROR(collRecMap,"NULL Collision Rec");
	if (G2_SetupModelPointers(ghoul2)&&collRecMap)
	{
		int frameNumber=G2API_GetTime(AframeNumber);

		vec3_t	transRayStart, transRayEnd;

		// make sure we have transformed the whole skeletons for each model
		G2_ConstructGhoulSkeleton(ghoul2, frameNumber,true, scale);

		// pre generate the world matrix - used to transform the incoming ray
		G2_GenerateWorldMatrix(angles, position);

		ri.GetG2VertSpaceServer()->ResetHeap();

		// now having done that, time to build the model
#ifdef _G2_GORE
		G2_TransformModel(ghoul2, frameNumber, scale, ri.GetG2VertSpaceServer(), useLod, false);
#else
		G2_TransformModel(ghoul2, frameNumber, scale,ri.GetG2VertSpaceServer(), useLod);
#endif

		// model is built. Lets check to see if any triangles are actually hit.
		// first up, translate the ray to model space
		TransformAndTranslatePoint(rayStart, transRayStart, &worldMatrixInv);
		TransformAndTranslatePoint(rayEnd, transRayEnd, &worldMatrixInv);

		// now walk each model and check the ray against each poly - sigh, this is SO expensive. I wish there was a better way to do this.
#ifdef _G2_GORE
		G2_TraceModels(ghoul2, transRayStart, transRayEnd, collRecMap, entNum, eG2TraceType, useLod, fRadius,0,0,0,0,0,qfalse);
#else
		G2_TraceModels(ghoul2, transRayStart, transRayEnd, collRecMap, entNum, eG2TraceType, useLod, fRadius);
#endif

		ri.GetG2VertSpaceServer()->ResetHeap();
		// now sort the resulting array of collision records so they are distance ordered
		qsort( collRecMap, MAX_G2_COLLISIONS,
			sizeof( CCollisionRecord ), QsortDistance );
		G2ANIM(ghoul2,"G2API_CollisionDetect");
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
void G2API_GiveMeVectorFromMatrix(mdxaBone_t &boltMatrix, Eorientations flags, vec3_t &vec)
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
void G2API_CopyGhoul2Instance(CGhoul2Info_v &ghoul2From, CGhoul2Info_v &ghoul2To, int modelIndex)
{
	//Ensiform: I'm commenting this out because modelIndex appears unused and legitimately set in gamecode
	//assert(modelIndex==-1); // copy individual bolted parts is not used in jk2 and I didn't want to deal with it
							// if ya want it, we will add it back correctly

	G2ERROR(ghoul2From.IsValid(),"Invalid ghlInfo");
	if (ghoul2From.IsValid())
	{
		ghoul2To.DeepCopy(ghoul2From);
#ifdef _G2_GORE //check through gore stuff then, as well.
		int model = 0;

		//(since we are sharing this gore set with the copied instance we will have to increment
		//the reference count - if the goreset is "removed" while the refcount is > 0, the refcount
		//is decremented to avoid giving other instances an invalid pointer -rww)
		while (model < ghoul2To.size())
		{
			if ( ghoul2To[model].mGoreSetTag )
			{
				CGoreSet* gore = FindGoreSet ( ghoul2To[model].mGoreSetTag );
				assert(gore);
				if (gore)
				{
					gore->mRefCount++;
				}
			}

			model++;
		}
#endif
		G2ANIM(ghoul2From,"G2API_CopyGhoul2Instance (source)");
		G2ANIM(ghoul2To,"G2API_CopyGhoul2Instance (dest)");
	}
}

char *G2API_GetSurfaceName(CGhoul2Info *ghlInfo, int surfNumber)
{
	static char noSurface[1] = "";
	if (G2_SetupModelPointers(ghlInfo))
	{
		mdxmSurface_t		*surf = 0;
		mdxmSurfHierarchy_t	*surfInfo = 0;


		surf = (mdxmSurface_t *)G2_FindSurface(ghlInfo->currentModel, surfNumber, 0);
		if (surf)
		{
			assert(G2_MODEL_OK(ghlInfo));
			mdxmHierarchyOffsets_t	*surfIndexes = (mdxmHierarchyOffsets_t *)((byte *)ghlInfo->currentModel->mdxm + sizeof(mdxmHeader_t));
			surfInfo = (mdxmSurfHierarchy_t *)((byte *)surfIndexes + surfIndexes->offsets[surf->thisSurfaceIndex]);
			return surfInfo->name;
		}
	}
	G2WARNING(0,"Surface Not Found");
	return noSurface;
}


int	G2API_GetSurfaceIndex(CGhoul2Info *ghlInfo, const char *surfaceName)
{
	int ret=-1;
	G2ERROR(surfaceName,"NULL surfaceName");
	if (surfaceName&&G2_SetupModelPointers(ghlInfo))
	{
		ret=G2_GetSurfaceIndex(ghlInfo, surfaceName);
	}
	G2WARNING(ret>=0,"G2API_GetSurfaceIndex Failed");
	return ret;
}

char *G2API_GetGLAName(CGhoul2Info *ghlInfo)
{
	if (G2_SetupModelPointers(ghlInfo))
	{
		assert(G2_MODEL_OK(ghlInfo));
		return (char*)ghlInfo->aHeader->name;
		//return ghlInfo->currentModel->mdxm->animName;
	}
	return 0;
}

qboolean G2API_SetNewOrigin(CGhoul2Info *ghlInfo, const int boltIndex)
{
	if (G2_SetupModelPointers(ghlInfo))
	{
		G2ERROR(boltIndex>=0&&boltIndex<(int)ghlInfo->mBltlist.size(),"invalid boltIndex");

		if (boltIndex>=0&&boltIndex<(int)ghlInfo->mBltlist.size())
		{
			ghlInfo->mNewOrigin = boltIndex;
			ghlInfo->mFlags |= GHOUL2_NEWORIGIN;
		}
		return qtrue;
	}
	return qfalse;
}

int G2API_GetBoneIndex(CGhoul2Info *ghlInfo, const char *boneName, qboolean bAddIfNotFound)
{
	int ret=-1;
	G2ERROR(boneName,"NULL boneName");
	if (boneName&&G2_SetupModelPointers(ghlInfo))
	{
		ret=G2_Get_Bone_Index(ghlInfo, boneName, bAddIfNotFound);
		G2ANIM(ghlInfo,"G2API_GetBoneIndex");
	}
	G2NOTE(ret>=0,"G2API_GetBoneIndex Failed");
	return ret;
}

void G2API_SaveGhoul2Models(CGhoul2Info_v &ghoul2)
{
	G2ANIM(ghoul2,"G2API_SaveGhoul2Models");
	G2_SaveGhoul2Models(ghoul2);
}

void G2API_LoadGhoul2Models(CGhoul2Info_v &ghoul2, char *buffer)
{
	G2_LoadGhoul2Model(ghoul2, buffer);
	G2ANIM(ghoul2,"G2API_LoadGhoul2Models");
//	G2ERROR(ghoul2.IsValid(),"Invalid ghlInfo after load");
}

// this is kinda sad, but I need to call the destructor in this module (exe), not the game.dll...
//
void G2API_LoadSaveCodeDestructGhoul2Info(CGhoul2Info_v &ghoul2)
{
	ghoul2.~CGhoul2Info_v();	// so I can load junk over it then memset to 0 without orphaning
}

#ifdef _G2_GORE
void ResetGoreTag(); // put here to reduce coupling

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
	if (!gore.useTheta)
	{
		vec3_t t;
		VectorCopy(gore.uaxis,t);
		TransformPoint(t, gore.uaxis, &worldMatrixInv);
	}

	int lod;
	ResetGoreTag();
	const int lodbias=Com_Clamp ( 0, 2,G2_DecideTraceLod(ghoul2[0],r_lodbias->integer));
	const int maxLod =Com_Clamp (0,ghoul2[0].currentModel->numLods,3);	//limit to the number of lods the main model has
	for(lod=lodbias;lod<maxLod;lod++)
	{
		// now having done that, time to build the model
		ri.GetG2VertSpaceServer()->ResetHeap();

		G2_TransformModel(ghoul2, gore.currentTime, gore.scale,ri.GetG2VertSpaceServer(),lod,true,&gore);

		// now walk each model and compute new texture coordinates
		G2_TraceModels(ghoul2, transHitLocation, transRayDirection, 0, gore.entNum, G2_NOCOLLIDE,lod,1.0f,gore.SSize,gore.TSize,gore.theta,gore.shader,&gore,qtrue);
	}
 }
#else
void G2API_ClearSkinGore ( CGhoul2Info_v &ghoul2 )
{
}

void G2API_AddSkinGore(CGhoul2Info_v &ghoul2,SSkinGoreData &gore)
{
}
#endif

bool G2_TestModelPointers(CGhoul2Info *ghlInfo) // returns true if the model is properly set up
{
	G2ERROR(ghlInfo,"NULL ghlInfo");
	if (!ghlInfo)
	{
		return false;
	}
	ghlInfo->mValid=false;
	if (ghlInfo->mModelindex != -1)
	{
		ghlInfo->mModel = RE_RegisterModel(ghlInfo->mFileName);
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
				ghlInfo->animModel =  R_GetModelByHandle(ghlInfo->currentModel->mdxm->animIndex + ghlInfo->animModelIndexOffset);
				if (ghlInfo->animModel)
				{
					ghlInfo->aHeader =ghlInfo->animModel->mdxa;
					G2ERROR(ghlInfo->aHeader,va("Model has no mdxa (gla) %s",ghlInfo->mFileName));
					if (!ghlInfo->aHeader)
					{
						Com_Error(ERR_DROP, "Ghoul2 Model has no mdxa (gla) %s",ghlInfo->mFileName);
					}
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
	if (!ghlInfo->mValid)
	{
		ghlInfo->currentModel=0;
		ghlInfo->currentModelSize=0;
		ghlInfo->animModel=0;
		ghlInfo->currentAnimModelSize=0;
		ghlInfo->aHeader=0;
	}
	return ghlInfo->mValid;
}

bool G2_SetupModelPointers(CGhoul2Info *ghlInfo) // returns true if the model is properly set up
{
	G2ERROR(ghlInfo,"NULL ghlInfo");
	if (!ghlInfo)
	{
		return false;
	}
	ghlInfo->mValid=false;
//	G2WARNING(ghlInfo->mModelindex != -1,"Setup request on non-used info slot?");
	if (ghlInfo->mModelindex != -1)
	{
		G2ERROR(ghlInfo->mFileName[0],"empty ghlInfo->mFileName");
		ghlInfo->mModel = RE_RegisterModel(ghlInfo->mFileName);
		ghlInfo->currentModel = R_GetModelByHandle(ghlInfo->mModel);
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

				ghlInfo->animModel =  R_GetModelByHandle(ghlInfo->currentModel->mdxm->animIndex + ghlInfo->animModelIndexOffset);
				G2ERROR(ghlInfo->animModel,va("NULL Model (gla) %s",ghlInfo->mFileName));
				if (ghlInfo->animModel)
				{
					ghlInfo->aHeader =ghlInfo->animModel->mdxa;
					G2ERROR(ghlInfo->aHeader,va("Model has no mdxa (gla) %s",ghlInfo->mFileName));
					if (!ghlInfo->aHeader)
					{
						Com_Error(ERR_DROP, "Ghoul2 Model has no mdxa (gla) %s",ghlInfo->mFileName);
					}
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
	if (!ghlInfo->mValid)
	{
		ghlInfo->currentModel=0;
		ghlInfo->currentModelSize=0;
		ghlInfo->animModel=0;
		ghlInfo->currentAnimModelSize=0;
		ghlInfo->aHeader=0;
	}
	return ghlInfo->mValid;
}

bool G2_SetupModelPointers(CGhoul2Info_v &ghoul2) // returns true if any model is properly set up
{
	bool ret=false;
	int i;
	for (i=0; i<ghoul2.size(); i++)
	{
		bool r=G2_SetupModelPointers(&ghoul2[i]);
		ret=ret||r;
	}
	return ret;
}


