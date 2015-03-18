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

////////////////////////////////////////////////////////////////////////////////////////
// RAVEN STANDARD TEMPLATE LIBRARY
//  (c) 2002 Activision
//
//
// Hash Pool
// ---------
// The hash pool stores raw data of variable size.  It uses a hash table to check for
// redundant data, and upon finding any, will return the existing handle.  Otherwise
// it copies the data to memory and returns a new handle.
//
//
// NOTES:
// 
//
//
////////////////////////////////////////////////////////////////////////////////////////
#if !defined(RATL_HASH_POOL_VS_INC)
#define RATL_HASH_POOL_VS_INC


////////////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////////////
#if !defined(RATL_COMMON_INC)
	#include "ratl_common.h"
#endif
namespace ratl
{





////////////////////////////////////////////////////////////////////////////////////////
// The Hash Pool
////////////////////////////////////////////////////////////////////////////////////////
template <int SIZE, int SIZE_HANDLES>
class	hash_pool
{
	int		mHandles[SIZE_HANDLES];					// each handle holds the start index of it's data

	int		mDataAlloc;								// where the next chunck of data will go
	char	mData[SIZE];

	#ifdef _DEBUG
	int		mFinds;									// counts how many total finds have run
	int		mCurrentCollisions;						// counts how many collisions on the last find
	int		mTotalCollisions;						// counts the total number of collisions
	int		mTotalAllocs;
	#endif


	////////////////////////////////////////////////////////////////////////////////////
	// This function searches for a handle which already stores the data (assuming the
	// handle is a hash within range SIZE_HANDLES).
	//
	// If it failes, it returns false, and the handle passed in points to the next
	// free slot.
	////////////////////////////////////////////////////////////////////////////////////
	bool		find_existing(int& handle, const void* data, int datasize)
	{
		#ifdef _DEBUG
		mFinds++;
		mCurrentCollisions = 0;
		#endif

		while (mHandles[handle])					// So long as a handle exists there
		{
			if (mem::eql((void*)(&mData[mHandles[handle]]), data, datasize))
			{
				return true;						// found 
			}
			handle=(handle+1)&(SIZE_HANDLES-1);		// incriment the handle

			#ifdef _DEBUG
			mCurrentCollisions ++;
			mTotalCollisions ++;

			//assert(mCurrentCollisions < 16);		// If We Had 16+ Collisions, Hash May Be Inefficient.  
													// Evaluate SIZE and SIZEHANDLES
			#endif
		}
		return false;								// failed to find
	}



	////////////////////////////////////////////////////////////////////////////////////
	// A simple hash function for the range of [0, SIZE_HANDLES]
	////////////////////////////////////////////////////////////////////////////////////
	int			hash(const void* data, int datasize)
	{
		int	 h=0;
		for (int i=0; i<datasize; i++)
		{
			h += ((const char*)(data))[i] * (i + 119);		// 119.  Prime Number?
		}
		h &= SIZE_HANDLES - 1;						// zero out bits beyoned SIZE_HANDLES
		return h;
	}

public:
	hash_pool()
	{
		clear();
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// The Number Of Bytes Allocated
    ////////////////////////////////////////////////////////////////////////////////////
	int			size()	 const													
	{
		return mDataAlloc;
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Check To See If This Memory Pool Is Empty
    ////////////////////////////////////////////////////////////////////////////////////
	bool		empty()	 const													
	{
		return (mDataAlloc==1);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Check To See If This Memory Pool Has Enough Space Left For (minimum) Bytes
    ////////////////////////////////////////////////////////////////////////////////////
	bool		full(int minimum)	 const													
	{
		return ((SIZE - mDataAlloc)<minimum);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Clear - Removes all allocation information - Note!  DOES NOT CLEAR MEMORY
    ////////////////////////////////////////////////////////////////////////////////////
	void		clear()
	{
		mData[0] = 0;
		mDataAlloc = 1;
		for (int i=0; i<SIZE_HANDLES; i++)
		{
			mHandles[i] = 0;
		}

		#ifdef _DEBUG
		mFinds = 0;
		mCurrentCollisions = 0;
		mTotalCollisions = 0;
		mTotalAllocs = 0;
		#endif
	}

	////////////////////////////////////////////////////////////////////////////////////
	// This is the primary functionality of the hash pool.  It will search for existing
	// data of the same size, and failing to find any, it will append the data to the
	// memory.
	//
	// In both cases, it gives you a handle to look up the data later.
	////////////////////////////////////////////////////////////////////////////////////
	int			get_handle(const void* data, int datasize)
	{
		int	handle = hash(data, datasize);				// Initialize Our Handle By Hash Fcn
		if (!find_existing(handle, data, datasize))
		{
			assert(mDataAlloc+datasize < SIZE);			// Is There Enough Memory?

			#ifdef _DEBUG
			mTotalAllocs++;
			#endif

			mem::cpy((void*)(&mData[mDataAlloc]), data, datasize);// Copy Data To Memory
			mHandles[handle] = mDataAlloc;				// Mark Memory In Hash Tbl
			mDataAlloc += datasize;						// Adjust Next Alloc Location
		}
		return handle;									// Return The Hash Tbl handleess
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Constant Access Operator
	////////////////////////////////////////////////////////////////////////////////////
	const void*	operator[](int handle) const
	{
		assert(handle>=0 && handle<SIZE_HANDLES);

		return &(mData[mHandles[handle]]);
	}


#ifdef _DEBUG
	float		average_collisions()	{return ((float)mTotalCollisions / (float)mFinds);}
	int			total_allocs()			{return mTotalAllocs;}
	int			total_finds()			{return mFinds;}
	int			total_collisions()		{return mTotalCollisions;}
#endif

};


}
#endif