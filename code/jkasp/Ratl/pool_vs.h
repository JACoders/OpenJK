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
// Memory Pool
// -----------
// The memory pool class is a simple routine for constant time allocation and deallocation
// from a fixed size pool of objects.  The class uses a simple array to hold the actual
// data, a queue for the free list, and a bit field to mark which spots in the array
// are allocated.
//
//
//
// NOTES:
//
//
//
////////////////////////////////////////////////////////////////////////////////////////
#if !defined(RATL_POOL_VS_INC)
#define RATL_POOL_VS_INC


////////////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////////////
#if !defined(RATL_COMMON_INC)
	#include "ratl_common.h"
#endif
#if !defined(RATL_QUEUE_VS_INC)
	#include "queue_vs.h"
#endif

namespace ratl
{

// fixme, this could be made more efficient by keepingtrack of the highest slot ever used
// then there is no need to fill the free list
template <class T>
class pool_root : public ratl_base
{
public:
    typedef T TStorageTraits;
	typedef typename T::TValue TTValue;
    ////////////////////////////////////////////////////////////////////////////////////
	// Capacity Enum
    ////////////////////////////////////////////////////////////////////////////////////
	static const int CAPACITY		= T::CAPACITY;
private:
    ////////////////////////////////////////////////////////////////////////////////////
	// Data
    ////////////////////////////////////////////////////////////////////////////////////
	array_base<TStorageTraits>	mData;
	queue_vs<int, CAPACITY>		mFree;
	bits_base<CAPACITY>			mUsed;
	int							mSize;


	void FillFreeList()
	{
		mFree.clear();
		int i;
		for (i=0;i<CAPACITY;i++)
		{
			mFree.push(i);
		}
	}
	int			alloc_low()
	{
		assert(mSize<CAPACITY);
		assert(!mUsed[mFree.top()]);

		int NextIndex = mFree.top();				// Get The First Available Location

		mUsed.set_bit(NextIndex);
		mFree.pop();
		mSize ++;
		return	NextIndex;
	}
public:


    ////////////////////////////////////////////////////////////////////////////////////
	// Constructor
    ////////////////////////////////////////////////////////////////////////////////////
	pool_root() : mSize(0)
	{
		FillFreeList();
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// The Number Of Objects Allocated
    ////////////////////////////////////////////////////////////////////////////////////
	int			size()	 const
	{
		return (mSize);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Check To See If This Memory Pool Is Empty
    ////////////////////////////////////////////////////////////////////////////////////
	bool		empty()	 const
	{
		return (mSize==0);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Check To See If This Memory Pool Is Full
    ////////////////////////////////////////////////////////////////////////////////////
	bool		full()	 const
	{
		return (mSize==CAPACITY);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Constant Accessor
    ////////////////////////////////////////////////////////////////////////////////////
	const TTValue&	value_at_index(int i) const
	{
		assert(mUsed[i]);
		return (mData[i]);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Accessor
    ////////////////////////////////////////////////////////////////////////////////////
	TTValue&			value_at_index(int i)
	{
		assert(mUsed[i]);
		return (mData[i]);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Clear - Removes all allocation information
    ////////////////////////////////////////////////////////////////////////////////////
	void		clear()
	{
		mSize = 0;
		mUsed.clear();
		mData.clear();
		FillFreeList();
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Check If An Index Has Been Allocated
    ////////////////////////////////////////////////////////////////////////////////////
	bool		is_used_index(int i) const
	{
		return mUsed[i];
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Convert a pointer back to an index
    ////////////////////////////////////////////////////////////////////////////////////
	int pointer_to_index(const TTValue *me) const
	{
		assert(mSize>0);

		int index=mData.pointer_to_index(me);

		assert(index>=0 && index<CAPACITY);
		assert(mUsed[index]);	// I am disallowing obtaining the index of a freed item

		return index;
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Convert a pointer back to an index
    ////////////////////////////////////////////////////////////////////////////////////
	int pointer_to_index(const TRatlNew *me) const
	{
		assert(mSize>0);

		int index=mData.pointer_to_index(me);

		assert(index>=0 && index<CAPACITY);
		assert(mUsed[index]);	// I am disallowing obtaining the index of a freed item

		return index;
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Swap two items based on index
    ////////////////////////////////////////////////////////////////////////////////////
	void swap_index(int i,int j)
	{
		assert(i>=0 && i<CAPACITY);
		assert(j>=0 && j<CAPACITY);
		mData.swap(i,j);
	}


    ////////////////////////////////////////////////////////////////////////////////////
	// The Allocator
    ////////////////////////////////////////////////////////////////////////////////////
	int			alloc_index()
	{
		int NextIndex = alloc_low();
		mData.construct(NextIndex);
		return	NextIndex;
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// The Allocator
    ////////////////////////////////////////////////////////////////////////////////////
	int			alloc_index(const TTValue &v)
	{
		int NextIndex = alloc_low();
		mData.construct(NextIndex,v);
		return	NextIndex;
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// The Allocator
    ////////////////////////////////////////////////////////////////////////////////////
	TRatlNew *		alloc_raw()
	{
		int NextIndex = alloc_low();
		return mData.alloc_raw(NextIndex);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// The Deallocator
    ////////////////////////////////////////////////////////////////////////////////////
	void		free_index(int i)
	{
		assert(mSize>0);
		assert(i>=0 && i<CAPACITY);
		assert(mUsed[i]);

		mData.destruct(i);

		mUsed.clear_bit(i);
		mFree.push(i);
		mSize --;
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// The Deallocator
    ////////////////////////////////////////////////////////////////////////////////////
	void		free(TTValue *me)
	{
		free(pointer_to_index(me));
	}


    ////////////////////////////////////////////////////////////////////////////////////
	// Iterator
    ////////////////////////////////////////////////////////////////////////////////////
	class const_iterator;
	class iterator
	{
		friend class pool_root<T>;
		friend class const_iterator;
		int				mIndex;
		pool_root<T>*	mOwner;
	public:
		// Constructors
		//--------------
		iterator()									: mOwner(0)
		{}
		iterator(pool_root<T>* p, int index)	: mIndex(index), mOwner(p)
		{}
		iterator(const iterator &t)	: mIndex(t.mIndex), mOwner(t.mOwner)
		{}

		// Assignment Operator
		//---------------------
		void		operator= (const iterator &t)
		{
			mOwner	= t.mOwner;
			mIndex	= t.mIndex;
		}

		// Equality Operators
		//--------------------
		bool	operator!=(const iterator& t)	{assert(mOwner && mOwner==t.mOwner);		return (mIndex!=t.mIndex);}
		bool	operator==(const iterator& t)	{assert(mOwner && mOwner==t.mOwner);		return (mIndex==t.mIndex);}

		// Dereference Operators
		//----------------------
		TTValue&		operator* () const				{assert(mOwner && mOwner->is_used_index(mIndex));	return (mOwner->mData[mIndex]);}
		TTValue*		operator->() const				{assert(mOwner && mOwner->is_used_index(mIndex));	return (&mOwner->mData[mIndex]);}

		// Handle & Index Access
		//-----------------------
		int		index()							{assert(mOwner && mOwner->is_used_index(mIndex));	return mIndex;}

		// Inc Operator
		//-------------
		iterator operator++(int)	 // postfix
		{
			assert(mIndex>=0&&mIndex<CAPACITY);						// this typically means you did end()++
			assert(mOwner && mOwner->is_used_index(mIndex));

			iterator ret(*this);
			mIndex = mOwner->mUsed.next_bit(mIndex+1);
			return ret;
		}
		// Inc Operator
		//-------------
		iterator operator++()	 // prefix
		{
			assert(mIndex>=0&&mIndex<CAPACITY);						// this typically means you did end()++
			assert(mOwner && mOwner->is_used_index(mIndex));
			mIndex = mOwner->mUsed.next_bit(mIndex+1);
			return *this;
		}
	};
	friend class iterator;

    ////////////////////////////////////////////////////////////////////////////////////
	// Iterator
    ////////////////////////////////////////////////////////////////////////////////////
	class const_iterator
	{
		int					mIndex;
		const pool_root<T>*	mOwner;
	public:
		// Constructors
		//--------------
		const_iterator()									: mOwner(0)
		{}
		const_iterator(const pool_root<T>* p, int index)		: mOwner(p), mIndex(index)
		{}
		const_iterator(const iterator &t)							: mOwner(t.mOwner), mIndex(t.mIndex)
		{}
		const_iterator(const const_iterator &t)					: mOwner(t.mOwner), mIndex(t.mIndex)
		{}

		// Equality Operators
		//--------------------
		bool	operator!=(const const_iterator& t) const		{assert(mOwner && mOwner==t.mOwner);		return (mIndex!=t.mIndex);}
		bool	operator==(const const_iterator& t) const		{assert(mOwner && mOwner==t.mOwner);		return (mIndex==t.mIndex);}
		bool	operator!=(const iterator& t) const				{assert(mOwner && mOwner==t.mOwner);		return (mIndex!=t.mIndex);}
		bool	operator==(const iterator& t) const				{assert(mOwner && mOwner==t.mOwner);		return (mIndex==t.mIndex);}

		// Dereference Operators
		//----------------------
		const TTValue&		operator* () const				{assert(mOwner && mOwner->is_used_index(mIndex));	return (mOwner->mData[mIndex]);}
		const TTValue*		operator->() const				{assert(mOwner && mOwner->is_used_index(mIndex));	return (&mOwner->mData[mIndex]);}

		// Handle & Index Access
		//-----------------------
		int		index() const					{assert(mOwner && mOwner->is_used_index(mIndex));	return mIndex;}

		// Inc Operator
		//-------------
		const_iterator operator++(int)	 // postfix
		{
			assert(mIndex>=0&&mIndex<CAPACITY);						// this typically means you did end()++
			assert(mOwner && mOwner->is_used_index(mIndex));

			const_iterator ret(*this);
			mIndex = mOwner->mUsed.next_bit(mIndex+1);
			return ret;
		}
		// Inc Operator
		//-------------
		const_iterator operator++()	 // prefix
		{
			assert(mIndex>=0&&mIndex<CAPACITY);						// this typically means you did end()++
			assert(mOwner && mOwner->is_used_index(mIndex));
			mIndex = mOwner->mUsed.next_bit(mIndex+1);
			return *this;
		}
	};
	friend class const_iterator;

    ////////////////////////////////////////////////////////////////////////////////////
	// Get An Iterator To The First Allocated Memory Block
    ////////////////////////////////////////////////////////////////////////////////////
	iterator	begin()
	{
		int idx=mUsed.next_bit(0);
		return iterator(this,idx);			// Find The First Allocated
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Get An Iterator To The Object At index
    ////////////////////////////////////////////////////////////////////////////////////
	iterator	at_index(int index)
	{
		assert(mUsed[index]);  // disallow iterators to non alloced things
		return iterator(this, index);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Get An Iterator To The End Of The Memroy (One Step Beyond)
    ////////////////////////////////////////////////////////////////////////////////////
	iterator	end()
	{
		return iterator(this, CAPACITY);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Get An Iterator To The First Allocated Memory Block
    ////////////////////////////////////////////////////////////////////////////////////
	const_iterator	begin() const
	{
		int idx=mUsed.next_bit(0);
		return iterator(this,idx);			// Find The First Allocated
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Get An Iterator To The Object At index
    ////////////////////////////////////////////////////////////////////////////////////
	const_iterator	at_index(int index) const
	{
		assert(mUsed[index]);  // disallow iterators to non alloced things
		return iterator(this, index);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Get An Iterator To The End Of The Memroy (One Step Beyond)
    ////////////////////////////////////////////////////////////////////////////////////
	const_iterator	end() const
	{
		return iterator(this, CAPACITY);
	}

	template<class CAST_TO>
	CAST_TO *verify_alloc(CAST_TO *p) const
	{
		return mData.verify_alloc(p);
	}
};

/*
pool_base, base class for the pools

operations:

size()
empty()
full()
clear()							op[]
at_index()						op[]
at_index() const
index pointer_to_index(ptr)
index alloc_index()				alloc()
index alloc_index(ref)			alloc()
ptr alloc_raw()
free_index(index)
free(ptr)
is_used_index(index)

*/

template <class T>
class pool_base : public pool_root<T>
{
public:
	typedef typename T::TValue TTValue;

    ////////////////////////////////////////////////////////////////////////////////////
	// Constant Accessor
    ////////////////////////////////////////////////////////////////////////////////////
	const TTValue&	operator[](int i) const
	{
		return pool_root<T>::value_at_index(i);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Accessor
    ////////////////////////////////////////////////////////////////////////////////////
	TTValue&			operator[](int i)
	{
		return pool_root<T>::value_at_index(i);
	}

	bool				is_used(int i) const
	{
		return pool_root<T>::is_used_index(i);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Swap two items based on index
    ////////////////////////////////////////////////////////////////////////////////////
	void swap(int i,int j)
	{
		pool_root<T>::swap_index(i,j);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// The Allocator	returns an index
    ////////////////////////////////////////////////////////////////////////////////////
	int			alloc()
	{
		return	pool_root<T>::alloc_index();
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// The Allocator	returns an index
    ////////////////////////////////////////////////////////////////////////////////////
	int			alloc(const TTValue &v)
	{
		return	pool_root<T>::alloc_index(v);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// The Deallocator
    ////////////////////////////////////////////////////////////////////////////////////
	void		free(int i)
	{
		pool_root<T>::free_index(i);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Get An Iterator To The Object At index
    ////////////////////////////////////////////////////////////////////////////////////
    typename pool_root<T>::iterator	at(int index)
	{
		return pool_root<T>::at_index(index);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Get An Iterator To The Object At index
    ////////////////////////////////////////////////////////////////////////////////////
    typename pool_root<T>::const_iterator	at(int index) const
	{
		return pool_root<T>::at_index(index);
	}
};

template<class T, int ARG_CAPACITY>
class pool_vs : public pool_base<storage::value_semantics<T,ARG_CAPACITY> >
{
public:
	typedef typename storage::value_semantics<T,ARG_CAPACITY> TStorageTraits;
	typedef typename TStorageTraits::TValue TTValue;
	static const int CAPACITY		= ARG_CAPACITY;
	pool_vs() {}
};

template<class T, int ARG_CAPACITY>
class pool_os : public pool_base<storage::object_semantics<T,ARG_CAPACITY> >
{
public:
	typedef typename storage::object_semantics<T,ARG_CAPACITY> TStorageTraits;
	typedef typename TStorageTraits::TValue TTValue;
	static const int CAPACITY		= ARG_CAPACITY;
	pool_os() {}
};

template<class T, int ARG_CAPACITY, int ARG_MAX_CLASS_SIZE>
class pool_is : public pool_base<storage::virtual_semantics<T,ARG_CAPACITY,ARG_MAX_CLASS_SIZE> >
{
public:
	typedef typename storage::virtual_semantics<T,ARG_CAPACITY,ARG_MAX_CLASS_SIZE> TStorageTraits;
	typedef typename TStorageTraits::TValue TTValue;
	static const int CAPACITY		= ARG_CAPACITY;
	static const int MAX_CLASS_SIZE	= ARG_MAX_CLASS_SIZE;
	pool_is() {}
};

}
#endif
