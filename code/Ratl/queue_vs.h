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
// Queue Template
// --------------
// The queue is a circular buffer of objects which supports a push at the "front" and a
// pop at the "back".  Therefore it is:
//
// First In, First Out
//
// As the pointers to the push and pop locations are changed it wrapps around the end
// of the array and back to the front.  There are asserts to make sure it never goes
// beyond the capacity of the object.
//
//
//
// NOTES:
//
//
//
////////////////////////////////////////////////////////////////////////////////////////
#if !defined(RATL_QUEUE_VS_INC)
#define RATL_QUEUE_VS_INC


////////////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////////////
#if !defined(RATL_COMMON_INC)
	#include "ratl_common.h"
#endif
namespace ratl
{

////////////////////////////////////////////////////////////////////////////////////////
// The Queue Class
////////////////////////////////////////////////////////////////////////////////////////
template <class T>
class queue_base : public ratl_base
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
	array_base<TStorageTraits>	mData;			// The Memory
	int							mPush;			// Address Of Next Add Location
	int							mPop;			// Address Of Next Remove Location
	int							mSize;


	int push_low()
	{
		assert(size()<CAPACITY);

		// Add It
		//--------
		mPush++;
		mSize++;

		// Update Push Location
		//----------------------
		if (mPush>=CAPACITY)
		{
			mPush=0;
			return CAPACITY-1;
		}
		return mPush-1;
	}


public:

    ////////////////////////////////////////////////////////////////////////////////////
	// Constructor
    ////////////////////////////////////////////////////////////////////////////////////
	queue_base() : mPush(0), mPop(0), mSize(0)
	{
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Get The Size (The Difference Between The Push And Pop "Pointers")
    ////////////////////////////////////////////////////////////////////////////////////
	int				size() const
	{
		return mSize;
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Check To See If The Size Is Zero
    ////////////////////////////////////////////////////////////////////////////////////
	bool			empty() const
	{
		return !mSize;
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Check To See If The Size Is Full
    ////////////////////////////////////////////////////////////////////////////////////
	bool			full() const
	{
		return mSize>=CAPACITY;
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Empty Out The Entire Queue
	////////////////////////////////////////////////////////////////////////////////////
	void			clear()
	{
		mPush = 0;
		mPop = 0;
		mSize = 0;
		mData.clear();
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Add A Value, returns a reference to the value in place
    ////////////////////////////////////////////////////////////////////////////////////
	TTValue &		push()
	{
		int idx=push_low();
		mData.construct(idx);
		return mData[idx];
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Add A Value to the Queue
    ////////////////////////////////////////////////////////////////////////////////////
	void push(const TTValue& v)
	{
		mData.construct(push_low(),v);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Add A Value to the Queue, returning a void * to the memory
    ////////////////////////////////////////////////////////////////////////////////////
	TRatlNew *push_raw()
	{
		return mData.alloc_raw(push_low());
	}
    ////////////////////////////////////////////////////////////////////////////////////
	// Remove A Value From The Queue
    ////////////////////////////////////////////////////////////////////////////////////
	void			pop()
	{
		assert(size()>0);

		mData.destruct(mPop);
		// Update Pop Location
		//---------------------
		mPop++;
		if (mPop>=CAPACITY)
		{
			mPop=0;
		}

		mSize--;
	}

	TTValue & top()
	{
		assert(size()>0);
		return mData[mPop];
	}

	const TTValue & top() const
	{
		assert(size()>0);
		return mData[mPop];
	}
	template<class CAST_TO>
	CAST_TO *verify_alloc(CAST_TO *p) const
	{
		return mData.verify_alloc(p);
	}
};

template<class T, int ARG_CAPACITY>
class queue_vs : public queue_base<storage::value_semantics<T,ARG_CAPACITY> >
{
public:
	typedef typename storage::value_semantics<T,ARG_CAPACITY> TStorageTraits;
	typedef typename TStorageTraits::TValue TTValue;
	static const int CAPACITY		= ARG_CAPACITY;
	queue_vs() {}
};

template<class T, int ARG_CAPACITY>
class queue_os : public queue_base<storage::object_semantics<T,ARG_CAPACITY> >
{
public:
	typedef typename storage::object_semantics<T,ARG_CAPACITY> TStorageTraits;
	typedef typename TStorageTraits::TValue TTValue;
	static const int CAPACITY		= ARG_CAPACITY;
	queue_os() {}
};

template<class T, int ARG_CAPACITY, int ARG_MAX_CLASS_SIZE>
class queue_is : public queue_base<storage::virtual_semantics<T,ARG_CAPACITY,ARG_MAX_CLASS_SIZE> >
{
public:
	typedef typename storage::virtual_semantics<T,ARG_CAPACITY,ARG_MAX_CLASS_SIZE> TStorageTraits;
	typedef typename TStorageTraits::TValue TTValue;
	static const int CAPACITY		= ARG_CAPACITY;
	static const int MAX_CLASS_SIZE	= ARG_MAX_CLASS_SIZE;
	queue_is() {}
};

}
#endif
