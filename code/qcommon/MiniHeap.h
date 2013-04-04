#if !defined(MINIHEAP_H_INC)
#define MINIHEAP_H_INC


class CMiniHeap
{
	char	*mHeap;
	char	*mCurrentHeap;
	int		mSize;
#if _DEBUG
	int		mMaxAlloc;
#endif
public:

// reset the heap back to the start
void ResetHeap()
{
#if _DEBUG
	if ((int)mCurrentHeap - (int)mHeap>mMaxAlloc)
	{
		mMaxAlloc=(int)mCurrentHeap - (int)mHeap;
	}
#endif
	mCurrentHeap = mHeap;
}

// initialise the heap
CMiniHeap(int size)
{
	mHeap = (char *)Z_Malloc(size, TAG_GHOUL2, qtrue);
	mSize = size;
#if _DEBUG
	mMaxAlloc=0;
#endif
	if (mHeap)
	{
		ResetHeap();
	}
}

// free up the heap
~CMiniHeap()
{
	if (mHeap)
	{
		// the quake heap will be long gone, no need to free it Z_Free(mHeap);
	}
}

// give me some space from the heap please
char *MiniHeapAlloc(int size)
{
	if (size < (mSize - ((int)mCurrentHeap - (int)mHeap)))
	{
		char *tempAddress =  mCurrentHeap;
		mCurrentHeap += size;
		return tempAddress;
	}
	return NULL;
}

};

extern CMiniHeap *G2VertSpaceServer;


#endif	//MINIHEAP_H_INC
