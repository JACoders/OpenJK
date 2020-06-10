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

#pragma once

#include "../qcommon/q_shared.h"

class IHeapAllocator
{
public:
	virtual ~IHeapAllocator() {}

	virtual void ResetHeap() = 0;
	virtual char *MiniHeapAlloc ( int size ) = 0;
};

class CMiniHeap : public IHeapAllocator
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
	CMiniHeap (int size)
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
		if ((size_t)size < (mSize - ((size_t)mCurrentHeap - (size_t)mHeap)))
		{
			char *tempAddress =  mCurrentHeap;
			mCurrentHeap += size;
			return tempAddress;
		}
		return NULL;
	}

};

// this is in the parent executable, so access ri->GetG2VertSpaceServer() from the rd backends!
extern IHeapAllocator *G2VertSpaceServer;
extern IHeapAllocator *G2VertSpaceClient;
