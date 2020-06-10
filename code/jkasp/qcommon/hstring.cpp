#include "cm_local.h"
#include "hstring.h"

#if defined (_DEBUG) && defined (_WIN32)
#define WIN32_LEAN_AND_MEAN 1
//#include <windows.h>	// for Sleep for Z_Malloc recovery attempy
#include "platform.h"
#endif

// mapPoolBlockCount is defined differently in the executable (sv_main.cpp) and the game dll (g_main.cpp) cuz
//we likely don't need as many blocks in the executable as we do in the game
extern int		mapPoolBlockCount;

// Used to fool optimizer during compilation of mem touch routines.
int HaHaOptimizer2=0;


CMapPoolLow &GetMapPool()
{
	// this may need to be ifdefed to be different for different modules
	static CMapPoolLow thePool;
	return thePool;
}

#define MAPBLOCK_SIZE_NODES	(1024)
#define MAPNODE_FREE		(0xa1)
#define MAPNODE_INUSE		(0x94)

struct SMapNode
{
	unsigned char mData[MAP_NODE_SIZE-2];
	unsigned char mMapBlockNum;
	unsigned char mTag;
};

class CMapBlock
{
	int			mId;
	char		mRaw[(MAPBLOCK_SIZE_NODES+1)*MAP_NODE_SIZE];
	SMapNode	*mNodes;
	int			mLastNode;

public:
	CMapBlock(int id,vector <void *> &freeList) :
		mLastNode(0)
	{
		// Alloc node storage for MAPBLOCK_SIZE_NODES worth of nodes.
		mNodes=(SMapNode *)((((unsigned long)mRaw)+MAP_NODE_SIZE)&~(unsigned long)0x1f);
		// Set all nodes to initially be free.
		int i;
		for(i=0;i<MAPBLOCK_SIZE_NODES;i++)
		{
			mNodes[i].mMapBlockNum=id;
			mNodes[i].mTag=MAPNODE_FREE;
			freeList.push_back((void *)&mNodes[i]);
		}
	}

	bool bOwnsNode(void *node)
	{
		return((((SMapNode *)node)>=&mNodes[0])&&(((SMapNode *)node)<&mNodes[MAPBLOCK_SIZE_NODES]));
	}
};

CMapPoolLow::CMapPoolLow()
{
	mLastBlockNum=-1;
}

CMapPoolLow::~CMapPoolLow()
{
#if _DEBUG
	char mess[1000];
#if _GAME
	if(mFreeList.size()<mMapBlocks.size()*MAPBLOCK_SIZE_NODES)
	{
		sprintf(mess,"[MEM][GAME]  !!!! Map Pool Leaked %d nodes\n",(MAPBLOCK_SIZE_NODES*mMapBlocks.size())-mFreeList.size());
		OutputDebugString(mess);
	}
	sprintf(mess, "[MEM][GAME]  Map Pool max. mem used = %d\n",mMapBlocks.size()*MAPBLOCK_SIZE_NODES*MAP_NODE_SIZE);
	OutputDebugString(mess);
#elif _CGAME
	if (mFreeList.size()<mMapBlocks.size()*MAPBLOCK_SIZE_NODES)
	{
		sprintf(mess, "[MEM][CGAME]  !!!! Map Pool Leaked %d nodes\n",(MAPBLOCK_SIZE_NODES*mMapBlocks.size())-mFreeList.size());
		OutputDebugString(mess);
	}
	sprintf(mess, "[MEM][CGAME] Map Pool max. mem used = %d\n",mMapBlocks.size()*MAPBLOCK_SIZE_NODES*MAP_NODE_SIZE);
	OutputDebugString(mess);
#else
	if (mFreeList.size()<mMapBlocks.size()*MAPBLOCK_SIZE_NODES)
	{
		sprintf(mess, "[MEM][EXE]  !!!! Map Pool Leaked %d nodes\n",(MAPBLOCK_SIZE_NODES*mMapBlocks.size())-mFreeList.size());
		OutputDebugString(mess);
	}
	sprintf(mess, "[MEM][EXE] Map Pool max. mem used = %d\n",mMapBlocks.size()*MAPBLOCK_SIZE_NODES*MAP_NODE_SIZE);
	OutputDebugString(mess);
#endif
#endif

	int i;
	for (i=0;i<mMapBlocks.size();i++)
	{
		delete mMapBlocks[i];
	}
}

void *CMapPoolLow::Alloc()
{
	// Try to request a node. First we look in the free-list, but if that
	// happens to be empty, we allocate more storage in the current CMapBlock.
	void *node=0;
	if(mFreeList.size())
	{
		// Retrieve the node to be recycled.
		node=(void *)mFreeList[mFreeList.size()-1];
		mFreeList.pop_back();
	}
	else
	{
		// None free, so alloc another block.
		CMapBlock *block=new CMapBlock(mLastBlockNum+1,mFreeList);
		assert(block);
		mMapBlocks.push_back(block);
		mLastBlockNum++;
		node=(void *)mFreeList[mFreeList.size()-1];
		mFreeList.pop_back();
	}

	// Validate we aren't somehow grabbing something that is already in use
	// and also that the end marker is intact.
	assert(((SMapNode *)node)->mTag==MAPNODE_FREE);
	assert((((SMapNode *)node)->mMapBlockNum)>=0);
	assert((((SMapNode *)node)->mMapBlockNum)<256);
	assert((((SMapNode *)node)->mMapBlockNum)<=mLastBlockNum);
	assert(mMapBlocks[((SMapNode *)node)->mMapBlockNum]->bOwnsNode(node));

	// Ok, mark the node as in use.
	((SMapNode *)node)->mTag=MAPNODE_INUSE;

	return(node);
}

void CMapPoolLow::Free(void *p)
{
	// Validate that someone isn't trying to double free this node and also
	// that the end marker is intact.
	assert(((SMapNode *)p)->mTag==MAPNODE_INUSE);
	assert((((SMapNode *)p)->mMapBlockNum)>=0);
	assert((((SMapNode *)p)->mMapBlockNum)<256);
	assert((((SMapNode *)p)->mMapBlockNum)<=mLastBlockNum);
	assert(mMapBlocks[((SMapNode *)p)->mMapBlockNum]->bOwnsNode(p));

	// Ok, mark the the node as free.
	((SMapNode *)p)->mTag=MAPNODE_FREE;

	// Add a new freelist entry to point at this node.
	mFreeList.push_back(p);
}

void CMapPoolLow::TouchMem()
{
	int				i,j;
	unsigned char	*memory;
	int				totSize=0;
	for(i=0;i<mMapBlocks.size();i++)
	{
		memory=(unsigned char *)mMapBlocks[i];
		totSize+=sizeof(CMapBlock);
		for(j=0;j<sizeof(CMapBlock);j+=256)
		{
			HaHaOptimizer2+=memory[j];
		}
	}
#ifdef _DEBUG
	Com_Printf("MapPool: Bytes touched %i\n",totSize);
#endif
}

////////////
// hString stuff
////////////
#define MAX_HASH (65536*2)

// Max number of strings we can ever deal with.
#define MAX_HSTRINGS 100000

// Size of a string storage block in bytes.
#define	BLOCK_SIZE	65536


int HashFunction(const char *key)
{
	long	hash=0;
	int		i=0;
	unsigned char	letter;

	letter = (unsigned char)*key++;
	while (letter)
	{
		hash += (long)(letter) * (i + 119);
		i++;
		letter = (unsigned char)*key++;
	}
	hash &= MAX_HASH - 1;
	return (int)hash;
}


class CHashHelper
{
	int mHashes[MAX_HASH];
	int mFindPtr;
	int mFindPtrStart;
public:
	CHashHelper()
	{
		int i;
		for (i=0;i<MAX_HASH;i++)
		{
			mHashes[i]=0;
		}
	}
	void Add(int hash,int value)
	{
		assert(hash>=0&&hash<MAX_HASH);
		assert(value); // 0 is the empty marker
		int i=hash;
		while (mHashes[i])
		{
			assert(mHashes[i]!=value); //please don't insert things twice
			i=(i+1)&(MAX_HASH-1);
			assert(i!=hash); //hash table is full?
		}
		mHashes[i]=value;
	}
	int FindFirst(int hash)
	{
		mFindPtr=hash;
		mFindPtrStart=hash;
		return FindNext();
	}
	int FindNext()
	{
		assert(mFindPtr>=0&&mFindPtr<MAX_HASH);
		int val=mHashes[mFindPtr];
		mFindPtr=(mFindPtr+1)&(MAX_HASH-1);
		assert(mFindPtr!=mFindPtrStart); //hash table full?
		return val;
	}
	void TouchMem()
	{
		int	i;
		for(i=0;i<sizeof(mHashes);i+=256)
		{
			HaHaOptimizer2+=((unsigned char	*)mHashes)[i];
		}
#ifdef _DEBUG
		Com_Printf("Hash helper: Bytes touched %i\n",sizeof(mHashes));
#endif
	}
};

CHashHelper &HashHelper()
{
	static CHashHelper It;
	return It;
}

static char	*gCharPtrs[MAX_HSTRINGS];

class CHSBlock
{
	int		mBytesUsed;
	char	mRaw[BLOCK_SIZE];

public:
	CHSBlock(void) :
		mBytesUsed(0)
	{
		// So we can do a comparison of blocks for debug purposes.
		memset(mRaw,0,BLOCK_SIZE);
	};

	char *Alloc(int sizeBytes)
	{
		// Remember to include 0 termintor in size.
		sizeBytes++;

		// Is it WAAAY to big? If so we complain loudly.
		assert(sizeBytes<=BLOCK_SIZE);

		// If we don't have space in the current block, return failure.
		if(sizeBytes>(BLOCK_SIZE-mBytesUsed))
		{
			return(0);
		}

		// Return the pointer to the start of allocated space.
		char *ret=&mRaw[mBytesUsed];
		mBytesUsed+=sizeBytes;
		return ret;
	}

	bool operator== (const CHSBlock &block) const
	{
		if(!memcmp(mRaw,block.mRaw,BLOCK_SIZE))
		{
			return(true);
		}
		return(false);
	}
};

class CPool
{
	vector<CHSBlock *>	mBlockVec;

public:
	int					mNextStringId;
	int					mLastBlockNum;

	CPool(void) :
		mNextStringId(1),
		mLastBlockNum(-1)
	{
		memset(gCharPtrs,0,MAX_HSTRINGS*4);
	}

	~CPool(void)
	{
		int i;
		for (i=0;i<mBlockVec.size();i++)
		{
			delete mBlockVec[i];
		}
	}

	char *Alloc(int sizeBytes,int &id)
	{
		// Can't alloc more than MAX_HSTRINGS.
		assert(mNextStringId<MAX_HSTRINGS);
		char *raw=0;
		if (mLastBlockNum>=0)
		{
			// Get the pointer to the start of allocated space in the current block.
			raw=mBlockVec[mLastBlockNum]->Alloc(sizeBytes);
		}
		if(!raw)
		{
			// Ok, make a new empty block and append it.
			CHSBlock *block=new(CHSBlock);
			mBlockVec.push_back(block);
			mLastBlockNum++;
			raw=mBlockVec[mLastBlockNum]->Alloc(sizeBytes);
		}
		// Should never really happen!!
		assert(raw);

		id=mNextStringId;
		gCharPtrs[mNextStringId]=raw;
		mNextStringId++;

		return(raw);
	}

	bool operator== (const CPool &pool) const
	{
		int i;
		for(i=0;i<mBlockVec.size();i++)
		{
			if(!(*mBlockVec[i]==*pool.mBlockVec[i]))
			{
				return(false);
			}
		}
		return(true);
	}

	void TouchMem()
	{
		int				i,j;
		unsigned char	*memory;
		int				totSize=0;
		for(i=0;i<mBlockVec.size();i++)
		{
			memory=(unsigned char *)mBlockVec[i];
			totSize+=sizeof(CHSBlock);
			for(j=0;j<sizeof(CHSBlock);j+=256)
			{
				HaHaOptimizer2+=memory[j];
			}
		}
#ifdef _DEBUG
		Com_Printf("String Pool: Bytes touched %i\n",totSize);
#endif
	}
};

#ifdef _DEBUG
CPool &TheDebugPool(void);
CPool &ThePool(void);

class CPoolChecker
{
public:
	CPoolChecker()
	{
		TheDebugPool();
		ThePool();
	}
	~CPoolChecker()
	{
#if 0
		int i;
		for (i=1;i<ThePool().mNextStringId;i++)
		{
			OutputDebugString(gCharPtrs[i]);
			OutputDebugString("\n");
		}
#endif
#if _DEBUG
		char mess[1000];
#if _GAME
		sprintf(mess,"[MEM][GAME]  String Pool %d unique strings, %dK\n",ThePool().mNextStringId,(ThePool().mLastBlockNum+1)*BLOCK_SIZE/1024);
#elif _CGAME
		sprintf(mess,"[MEM][CGAME]  String Pool %d unique strings, %dK\n",ThePool().mNextStringId,(ThePool().mLastBlockNum+1)*BLOCK_SIZE/1024);
#else
		sprintf(mess,"[MEM][EXE]  String Pool %d unique strings, %dK\n",ThePool().mNextStringId,(ThePool().mLastBlockNum+1)*BLOCK_SIZE/1024);
#endif
		OutputDebugString(mess);
#endif
		// if this fails it means the string storage is CORRUPTED, let someone know
		assert(TheDebugPool()==ThePool());
	}
};

static CPoolChecker TheCPoolChecker;

CPool &TheDebugPool(void)
{
	static CPool theDebugPool;
	return(theDebugPool);
}


#endif

CPool &ThePool(void)
{
	static CPool thePool;
	return(thePool);
}

void TouchStringPool(void)
{
	ThePool().TouchMem();
	HashHelper().TouchMem();
}

//
// Now the rest of the hString class.
//

void hstring::Init(const char *str)
{
	if(!str)
	{
		mId=0;
		return;
	}
	int hash=HashFunction(str);
	int id=HashHelper().FindFirst(hash);
	while (id)
	{
		assert(id>0&&id<ThePool().mNextStringId);
		if (!strcmp(str,gCharPtrs[id]))
		{
			mId=id;
			return;
		}
		id=HashHelper().FindNext();
	}
	char *raw=ThePool().Alloc(strlen(str),mId);
	strcpy(raw,str);
	HashHelper().Add(hash,mId);
#ifdef _DEBUG
	int test;
	raw=TheDebugPool().Alloc(strlen(str),test);
	assert(test==mId);
	strcpy(raw,str);
#endif

}

const char *hstring::c_str(void) const
{
	if(!mId)
	{
		return("");
	}
	assert(mId>0&&mId<ThePool().mNextStringId);
	return(gCharPtrs[mId]);
}

string hstring::str(void) const
{
	if(!mId)
	{
		return(string());
	}
	assert(mId>0&&mId<ThePool().mNextStringId);
	return string(gCharPtrs[mId]);
}

