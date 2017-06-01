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
// Stack
// -----
// This is a very simple wrapper around a stack object.
//
// First In, Last Out
//
//
//
// NOTES:
//
//
//
////////////////////////////////////////////////////////////////////////////////////////
#if !defined(RATL_STACK_VS_INC)
#define RATL_STACK_VS_INC


////////////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////////////
#if !defined(RATL_COMMON_INC)
	#include "ratl_common.h"
#endif
namespace ratl
{

////////////////////////////////////////////////////////////////////////////////////////
// The stack Class
////////////////////////////////////////////////////////////////////////////////////////
template <class T>
class stack_base : public ratl_base
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
	array_base<TStorageTraits>		mData;			// The Memory
	int								mSize;

public:

    ////////////////////////////////////////////////////////////////////////////////////
	// Constructor
    ////////////////////////////////////////////////////////////////////////////////////
	stack_base() : mSize(0)
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
	// Empty Out The Entire stack
	////////////////////////////////////////////////////////////////////////////////////
	void			clear()
	{
		mSize = 0;
		mData.clear();
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Add A Value, returns a reference to the value in place
    ////////////////////////////////////////////////////////////////////////////////////
	TTValue &		push()
	{
		assert(!full());
		mData.construct(mSize);
		mSize++;
		return mData[mSize-1];
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Add A Value to the stack
    ////////////////////////////////////////////////////////////////////////////////////
	void push(const TTValue& v)
	{
		assert(!full());
		mData.construct(mSize,v);
		mSize++;
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Add A Value to the stack, returning a void * to the memory
    ////////////////////////////////////////////////////////////////////////////////////
	TRatlNew *push_raw()
	{
		assert(!full());
		mSize++;
		return mData.alloc_raw(mSize-1);
	}
    ////////////////////////////////////////////////////////////////////////////////////
	// Remove A Value From The stack
    ////////////////////////////////////////////////////////////////////////////////////
	void			pop()
	{
		assert(!empty());
		mSize--;
		mData.destruct(mSize);
	}

	TTValue & top()
	{
		assert(!empty());
		return mData[mSize-1];
	}

	const TTValue & top() const
	{
		assert(!empty());
		return mData[mSize-1];
	}
	template<class CAST_TO>
	CAST_TO *verify_alloc(CAST_TO *p) const
	{
		return mData.verify_alloc(p);
	}
};

template<class T, int ARG_CAPACITY>
class stack_vs : public stack_base<storage::value_semantics<T,ARG_CAPACITY> >
{
public:
	typedef typename storage::value_semantics<T,ARG_CAPACITY> TStorageTraits;
	typedef typename TStorageTraits::TValue TTValue;
	static const int CAPACITY		= ARG_CAPACITY;
	stack_vs() {}
};

template<class T, int ARG_CAPACITY>
class stack_os : public stack_base<storage::object_semantics<T,ARG_CAPACITY> >
{
public:
	typedef typename storage::object_semantics<T,ARG_CAPACITY> TStorageTraits;
	typedef typename TStorageTraits::TValue TTValue;
	static const int CAPACITY		= ARG_CAPACITY;
	stack_os() {}
};

template<class T, int ARG_CAPACITY, int ARG_MAX_CLASS_SIZE>
class stack_is : public stack_base<storage::virtual_semantics<T,ARG_CAPACITY,ARG_MAX_CLASS_SIZE> >
{
public:
	typedef typename storage::virtual_semantics<T,ARG_CAPACITY,ARG_MAX_CLASS_SIZE> TStorageTraits;
	typedef typename TStorageTraits::TValue TTValue;
	static const int CAPACITY		= ARG_CAPACITY;
	static const int MAX_CLASS_SIZE	= ARG_MAX_CLASS_SIZE;
	stack_is() {}
};


}
#endif
