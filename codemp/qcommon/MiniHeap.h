#if !defined(MINIHEAP_H_INC)
#define MINIHEAP_H_INC


class CMiniHeap
{
private:
	char	*mHeap;
	char	*mCurrentHeap;
	int		mSize;
public:

// reset the heap back to the start
void ResetHeap()
{
	mCurrentHeap = mHeap;
}

// initialise the heap
CMiniHeap(int size)
{
	mHeap = (char *)malloc(size);
	mSize = size;
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
		free(mHeap);
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

// this is in the parent executable, so access ri.GetG2VertSpaceServer() from the rd backends!
extern CMiniHeap *G2VertSpaceServer;
extern CMiniHeap *G2VertSpaceClient;


#endif	//MINIHEAP_H_INC
