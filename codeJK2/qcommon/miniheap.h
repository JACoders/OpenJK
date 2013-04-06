#if !defined(MINIHEAP_H_INC)
#define MINIHEAP_H_INC


class CMiniHeap
{
	char	*mHeap;
	char	*mCurrentHeap;
	int		mSize;
	int		mMaxAlloc;
public:

// reset the heap back to the start
void ResetHeap()
{
	if ((int)mCurrentHeap - (int)mHeap>mMaxAlloc)
	{
		mMaxAlloc=(int)mCurrentHeap - (int)mHeap;
	}
	mCurrentHeap = mHeap;
}

// initialise the heap
CMiniHeap(int size)
{
	mHeap = (char *)Z_Malloc(size, TAG_GHOUL2, qtrue);
	mSize = size;
	mMaxAlloc=0;
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
		Z_Free(mHeap);
	}
#if _DEBUG
	char mess[1000];
	sprintf(mess,"Max miniheap allocated %d\n",mMaxAlloc);
	OutputDebugString(mess);
#endif
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
