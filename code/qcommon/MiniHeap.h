/*
This file is part of Jedi Academy.

    Jedi Academy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Jedi Academy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Academy.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

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
	if ((intptr_t)mCurrentHeap - (intptr_t)mHeap>mMaxAlloc)
	{
		mMaxAlloc=(intptr_t)mCurrentHeap - (intptr_t)mHeap;
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
	if (size < (mSize - ((intptr_t)mCurrentHeap - (intptr_t)mHeap)))
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
