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
// Handle Pool
// -----------
// The memory pool class is a simple routine for constant time allocation and deallocation
// from a fixed size pool of objects.  The class uses a simple array to hold the actual
// data, a queue for the free list, and a bit field to mark which spots in the array
// are allocated.
//
// In addition to the standard memory pool features, this Handle Pool provides a fast
// iterator, asserts on attempting to access unused data, and a unique ID "handle" for
// the external system to use.
//
//
//
// NOTES:
// 
//
//
////////////////////////////////////////////////////////////////////////////////////////
#if !defined(RATL_HANDLE_POOL_VS_INC)
#define RATL_HANDLE_POOL_VS_INC

////////////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////////////
#if !defined(RATL_COMMON_INC)
	#include "ratl_common.h"
#endif
#if !defined(RATL_POOL_VS_INC)
	#include "pool_vs.h"
#endif

namespace ratl
{


template <class T>
class handle_pool_base : public pool_root<T>
{
public:
    typedef T TStorageTraits;
	typedef typename T::TValue TTValue;
 	////////////////////////////////////////////////////////////////////////////////////
	// Capacity Enum
    ////////////////////////////////////////////////////////////////////////////////////
	static const int CAPACITY		= T::CAPACITY;

private:
	int							mHandles[CAPACITY];
	int							mMASK_HANDLE_TO_INDEX;
	int							mMASK_NUM_BITS;

	void IncHandle(int index)
	{
		mHandles[index] += (1<<mMASK_NUM_BITS);
		if (mHandles[index]<0)
		{
			// we rolled over
			mHandles[index] = index;			// Reset The ID Counter
			mHandles[index] |= (1<<mMASK_NUM_BITS);
		}
	}
public:
	
    ////////////////////////////////////////////////////////////////////////////////////
	// Constructor
	//
	// We need a routine to calculate the MASK used to convert a handle to an index and
	// the number of bits needed to shift by.
	//
    ////////////////////////////////////////////////////////////////////////////////////
	handle_pool_base()
	{
		mMASK_HANDLE_TO_INDEX = 0;
		mMASK_NUM_BITS = 0;

		int	value=CAPACITY-1;
		while(value)
		{
			value>>= 1;

			mMASK_HANDLE_TO_INDEX <<= 1;
			mMASK_HANDLE_TO_INDEX |= 1;
			mMASK_NUM_BITS++;
		}
		for (int i=0; i<CAPACITY; i++)
		{
#ifdef _DEBUG
			mHandles[i] = i;			// Reset The ID Counter
			mHandles[i] |= ((++HandleSaltValue)<<mMASK_NUM_BITS);
#else
			mHandles[i] = i;			// Reset The ID Counter
			mHandles[i] |= (1<<mMASK_NUM_BITS);
#endif
		}
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Clear - Removes all allocation information
    ////////////////////////////////////////////////////////////////////////////////////
	void		clear()
	{
		pool_root<T>::clear();
		// note that we do not refill the handles cause we want old handles to still be stale
		for (int i=0; i<CAPACITY; i++)
		{
			IncHandle(i);
		}
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Constant Accessor
    ////////////////////////////////////////////////////////////////////////////////////
	const TTValue&	operator[](int handle) const 									
	{
		assert(is_used(handle));		//typically this is a stale handle (already been freed)
		return pool_root<T>::value_at_index(handle&mMASK_HANDLE_TO_INDEX);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Accessor
    ////////////////////////////////////////////////////////////////////////////////////
	TTValue&			operator[](int i)											
	{
		assert(is_used(i));		//typically this is a stale handle (already been freed)
		return pool_root<T>::value_at_index(i&mMASK_HANDLE_TO_INDEX);
	}

	bool				is_used(int i) const
	{
		if (mHandles[i&mMASK_HANDLE_TO_INDEX]==i)
		{
			return pool_root<T>::is_used_index(i&mMASK_HANDLE_TO_INDEX);
		}
		return false;
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Swap two items based on handle
    ////////////////////////////////////////////////////////////////////////////////////
	void swap(int i,int j)
	{
		assert(is_used(i));		//typically this is a stale handle (already been freed)
		assert(is_used(j));		//typically this is a stale handle (already been freed)
		swap_index(handle_to_index(i),handle_to_index(j));
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// The Allocator	returns a handle
    ////////////////////////////////////////////////////////////////////////////////////
	int			alloc()
	{
		int index=pool_root<T>::alloc_index();
		return	mHandles[index];
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// The Allocator, with value, returns a handle
    ////////////////////////////////////////////////////////////////////////////////////
	int			alloc(const TTValue &v)
	{
		int index=pool_root<T>::alloc_index(v);
		return	mHandles[index];
	}
    ////////////////////////////////////////////////////////////////////////////////////
	// The Deallocator, by index, not something generally needed
    ////////////////////////////////////////////////////////////////////////////////////
	void		free_index(int index)
	{
		pool_root<T>::free_index(index);
		IncHandle(index);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// The Deallocator, by handle
    ////////////////////////////////////////////////////////////////////////////////////
	void		free(int handle)
	{
		assert(is_used(handle));
		free_index(handle&mMASK_HANDLE_TO_INDEX);
	}
	
    ////////////////////////////////////////////////////////////////////////////////////
	// The Deallocator, by pointer
    ////////////////////////////////////////////////////////////////////////////////////
	void		free(TTValue *me)
	{
		free_index(pointer_to_index(me));
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Convert a handle to a raw index, not generally something you should use
    ////////////////////////////////////////////////////////////////////////////////////
	int			handle_to_index(int handle) const
	{
		assert(is_used(handle));
		return (handle&mMASK_HANDLE_TO_INDEX);
	}

	////////////////////////////////////////////////////////////////////////////////////
	// FInd the handle for a given index, this cannot check for stale handles, so it is ill advised
    ////////////////////////////////////////////////////////////////////////////////////
	int			index_to_handle(int index) const
	{
		assert(index>=0 && index<CAPACITY && pool_root<T>::is_used_index(index)); //disallowing this on stale handles
		return (mHandles[index]);
	}

	////////////////////////////////////////////////////////////////////////////////////
	// converts a T pointer to a handle, generally not something you need, cannot check for stale handles
    ////////////////////////////////////////////////////////////////////////////////////
	int			pointer_to_handle(const TTValue *me) const 
	{
		return index_to_handle(pool_root<T>::pointer_to_index(me));
	}
	////////////////////////////////////////////////////////////////////////////////////
	// converts a T pointer to a handle, generally not something you need, cannot check for stale handles
    ////////////////////////////////////////////////////////////////////////////////////
	int			pointer_to_handle(const TRatlNew *me) const 
	{
		return index_to_handle(pool_root<T>::pointer_to_index(me));
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Get An Iterator To The Object At handle
    ////////////////////////////////////////////////////////////////////////////////////
    typename pool_root<T>::iterator	at(int handle)
	{
		assert(is_used(handle));
		return pool_root<T>::at_index(handle&mMASK_HANDLE_TO_INDEX);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Get An Iterator To The Object At handle
    ////////////////////////////////////////////////////////////////////////////////////
    typename pool_root<T>::const_iterator	at(int handle) const
	{
		assert(is_used(handle));
		return pool_root<T>::at_index(handle&mMASK_HANDLE_TO_INDEX);
	}
};

template<class T, int ARG_CAPACITY>
class handle_pool_vs : public handle_pool_base<storage::value_semantics<T,ARG_CAPACITY> >
{
public:
	typedef typename storage::value_semantics<T,ARG_CAPACITY> TStorageTraits;
	typedef typename TStorageTraits::TValue TTValue;
	static const int CAPACITY		= ARG_CAPACITY;
	handle_pool_vs() {}
};

template<class T, int ARG_CAPACITY>
class handle_pool_os : public handle_pool_base<storage::object_semantics<T,ARG_CAPACITY> >
{
public:
	typedef typename storage::object_semantics<T,ARG_CAPACITY> TStorageTraits;
	typedef typename TStorageTraits::TValue TTValue;
	static const int CAPACITY		= ARG_CAPACITY;
	handle_pool_os() {}
};

template<class T, int ARG_CAPACITY, int ARG_MAX_CLASS_SIZE>
class handle_pool_is : public handle_pool_base<storage::virtual_semantics<T,ARG_CAPACITY,ARG_MAX_CLASS_SIZE> >
{
public:
	typedef typename storage::virtual_semantics<T,ARG_CAPACITY,ARG_MAX_CLASS_SIZE> TStorageTraits;
	typedef typename TStorageTraits::TValue TTValue;
	static const int CAPACITY		= ARG_CAPACITY;
	static const int MAX_CLASS_SIZE	= ARG_MAX_CLASS_SIZE;
	handle_pool_is() {}
};

}
#endif
