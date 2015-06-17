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

////////////////////////////////////////////////////////////////////////////////////////
// RAVEN STANDARD TEMPLATE LIBRARY
//  (c) 2002 Activision
//
//
// Vector
// ------
// The vector class is a simple addition to the array.  It supports some useful additions
// like sort and binary search, as well as keeping track of the number of objects
// contained within.
//
//
//
//
//
// NOTES:
//
//
////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////////////
#include "ratl_common.h"

namespace ratl
{


////////////////////////////////////////////////////////////////////////////////////////
// The Vector Class
////////////////////////////////////////////////////////////////////////////////////////
template<class T>
class vector_base : public ratl_base
{
public:
	typedef T TStorageTraits;
	typedef typename T::TValue TTValue;
    ////////////////////////////////////////////////////////////////////////////////////
	// Capacity Enum
    ////////////////////////////////////////////////////////////////////////////////////
 	enum
	{
		CAPACITY		= T::CAPACITY
	};
private:
	array_base<TStorageTraits>		mArray;			// The Memory
	int								mSize;
public:

	////////////////////////////////////////////////////////////////////////////////////
	// Constructor
    ////////////////////////////////////////////////////////////////////////////////////
	vector_base()
	{
		mSize = 0;
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Copy Constructor
	////////////////////////////////////////////////////////////////////////////////////
	vector_base(const vector_base &B)
	{
		for (int i=0; i<B.size(); i++)
		{
			mArray[i] = B.mArray[i];
		}
		mSize = B.mSize;
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// How Many Objects Can Be Added?
    ////////////////////////////////////////////////////////////////////////////////////
	int				capacity() const
	{
		assert(mSize>=0&&mSize<=CAPACITY);
		return (CAPACITY);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// How Many Objects Have Been Added To This Vector?
    ////////////////////////////////////////////////////////////////////////////////////
	int				size() const
	{
		assert(mSize>=0&&mSize<=CAPACITY);
		return (mSize);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Have Any Objects Have Been Added To This Vector?
    ////////////////////////////////////////////////////////////////////////////////////
	bool			empty() const
	{
		assert(mSize>=0&&mSize<=CAPACITY);
		return (!mSize);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Have Any Objects Have Been Added To This Vector?
    ////////////////////////////////////////////////////////////////////////////////////
	bool			full() const
	{
		assert(mSize>=0&&mSize<=CAPACITY);
		return (mSize==CAPACITY);
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Clear Out Entire Array
	////////////////////////////////////////////////////////////////////////////////////
	void			clear()
	{
		mArray.clear();
		mSize = 0;
	}
	// Constant Access Operator
	////////////////////////////////////////////////////////////////////////////////////
	const TTValue&		operator[](int index) const
	{
		assert(index>=0&&index<mSize);
		return mArray[index];
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Access Operator
	////////////////////////////////////////////////////////////////////////////////////
	TTValue&				operator[](int index)
	{
		assert(index>=0&&index<mSize);
		return mArray[index];
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Access To The Raw Array Pointer
	////////////////////////////////////////////////////////////////////////////////////
	TTValue *				raw_array()
	{
		  // this (intentionally) won't compile for anything except value semantics
			// could be done with object semantics, but I would want to assert that all objects are contructed
		  return T::raw_array(mArray);
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Access To The Raw Array Pointer
	////////////////////////////////////////////////////////////////////////////////////
	const TTValue*				raw_array() const
	{
		  // this (intentionally) won't compile for anything except value semantics
			// could be done with object semantics, but I would want to assert that all objects are contructed
		  return T::raw_array(mArray);
	}


	////////////////////////////////////////////////////////////////////////////////////
	// Assignment Operator
	////////////////////////////////////////////////////////////////////////////////////
	vector_base&		operator=(const vector_base& val)
	{
		for (int i=0; i<val.size(); i++)
		{
			mArray[i] = val.mArray[i];
		}
		mSize = val.mSize;
		return *this;
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Add
	////////////////////////////////////////////////////////////////////////////////////
	TTValue &				push_back()
	{
		assert(mSize>=0&&mSize<CAPACITY);
		mArray.construct(mSize);
		mSize++;
		return (mArray[mSize-1]);
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Add (And Set)
	////////////////////////////////////////////////////////////////////////////////////
	void			push_back(const TTValue& value)
	{
		assert(mSize>=0&&mSize<CAPACITY);
		mArray.construct(mSize,value);
		mSize++;
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Add raw
	////////////////////////////////////////////////////////////////////////////////////
	TRatlNew *			push_back_raw()
	{
		assert(mSize>=0&&mSize<CAPACITY);
		mSize++;
		return mArray.alloc_raw(mSize-1);
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Remove
	////////////////////////////////////////////////////////////////////////////////////
	void			pop_back()
	{
		assert(mSize>0);
		mSize--;
		mArray.destruct(mSize);
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Resizes The Array.  If New Elements Are Needed, It Uses The (value) Param
	////////////////////////////////////////////////////////////////////////////////////
	void			resize(int nSize, const TTValue& value)
	{
		int i;
		for (i=(mSize-1); i>=nSize; i--)
		{
			mArray.destruct(i);
			mSize--;
		}
		for (i=mSize; i<nSize; i++)
		{
			mArray.construct(i,value);
		}
		mSize = nSize;
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Resizes The Array.  If New Elements Are Needed, It Uses The (value) Param
	////////////////////////////////////////////////////////////////////////////////////
	void			resize(int nSize)
	{
		int i;
		for (i=mSize-1; i>=nSize; i--)
		{
			mArray.destruct(i);
			mSize--;
		}
		for (i=mSize; i<nSize; i++)
		{
			mArray.construct(i);
		}
		mSize = nSize;
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Swap the values at two locations
	////////////////////////////////////////////////////////////////////////////////////
	void swap(int i,int j)
	{
		assert(i<mSize && j<mSize);
		mArray.swap(i, j);
	}



	////////////////////////////////////////////////////////////////////////////////////
	// Erase An Iterator Location... NOTE: THIS DOES NOT PRESERVE ORDER IN THE VECTOR!!
	////////////////////////////////////////////////////////////////////////////////////
	void erase_swap(int Index)
	{
		assert(Index>=0 && Index<mSize);
		if (Index != mSize - 1)
		{
			mArray.swap(Index, mSize - 1);
		}
		pop_back();
	}


	////////////////////////////////////////////////////////////////////////////////////
	// Binary Search
	////////////////////////////////////////////////////////////////////////////////////
	int				find_index(const TTValue& value) const
	{
		int base = 0;
		int head = mSize-1;

		while (1)
		{
			int searchAt = (base+head)/2;

			if (base == head && searchAt == head)
			{
				break;
			}

			if (value < mArray[searchAt])
			{
				head = searchAt-1;
			}
			else if (mArray[searchAt] < value)
			{
				base = searchAt;
			}
			else
			{
				return searchAt;
			}
			if (head < base)
			{
				break;
			}
		}

		return mSize; //not found!
	}


	////////////////////////////////////////////////////////////////////////////////////
	// Heap Sort
	//
	// This sort algorithm has all the advantages of merge sort in terms of guarenteeing
	// O(n log n) worst case, as well as all the advantages of quick sort in that it is
	// "in place" and requires no additional storage.
	//
	////////////////////////////////////////////////////////////////////////////////////
	void			sort()
	{
		// Temporary Data
		//----------------
		int	HeapSize;	// How Large The Heap Is (Grows In PHASE 1, Shrinks In PHASE 2)
		int	Pos;		// The Location We Are AT During "re-heapify" Loops
		int	Compare;	// The Location We Are Comparing AGAINST During "re-heapify" Loops




		// PHASE 1, CONSTRUCT THE HEAP										  O(n log n)
		//===============================================================================
		for (HeapSize=1; HeapSize<mSize; HeapSize++)
		{
			// We Now Have An Element At Heap Size Which Is Not In It's Correct Place
			//------------------------------------------------------------------------
			Pos		= HeapSize;
			Compare	= parent(Pos);
			while (mArray[Compare]<mArray[Pos])
			{
				// Swap The Compare Element With The Pos Element
				//-----------------------------------------------
				mArray.swap(Compare, Pos);

				// Move Pos To The Current Compare, And Recalc Compare
				//------------------------------------------------------
				Pos		= Compare;
				Compare = parent(Pos);
			}
		}




		// PHASE 2, POP OFF THE TOP OF THE HEAP ONE AT A TIME (AND FIX)       O(n log n)
		//===============================================================================
		for (HeapSize=(mSize-1); HeapSize>0; HeapSize--)
		{
			// Swap The End And Front Of The "Heap" Half Of The Array
			//--------------------------------------------------------
			mArray.swap(0, HeapSize);

			// We Now Have A Bogus Element At The Root, So Fix The Heap
			//----------------------------------------------------------
			Pos		= 0;
			Compare = largest_child(Pos, HeapSize);
			while (mArray[Pos]<mArray[Compare])
			{
				// Swap The Compare Element With The Pos Element
				//-----------------------------------------------
				mArray.swap(Compare, Pos);

				// Move Pos To The Current Compare, And Recalc Compare
				//------------------------------------------------------
				Pos		= Compare;
				Compare	= largest_child(Pos, HeapSize);
			}
		}
	}


	////////////////////////////////////////////////////////////////////////////////////
	// THIS IS A QUICK VALIDATION OF A SORTED LIST
	////////////////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
	void			sort_validate() const
	{
		for (int a=0; a<(mSize-1); a++)
		{
			assert(mArray[a] < mArray[a+1]);
		}
	}
#endif

private:
	////////////////////////////////////////////////////////////////////////////////////
	// For Heap Sort
	// Returns The Location Of Node (i)'s Parent Node (The Parent Node Of Zero Is Zero)
	////////////////////////////////////////////////////////////////////////////////////
	static int			parent(int i)
	{
		return ((i-1)/2);
	}

	////////////////////////////////////////////////////////////////////////////////////
	// For Heap Sort
	// Returns The Location Of Node (i)'s Left Child (The Child Of A Leaf Is The Leaf)
	////////////////////////////////////////////////////////////////////////////////////
	static int			left(int i)
	{
		return ((2*i)+1);
	}

	////////////////////////////////////////////////////////////////////////////////////
	// For Heap Sort
	// Returns The Location Of Node (i)'s Right Child (The Child Of A Leaf Is The Leaf)
	////////////////////////////////////////////////////////////////////////////////////
	static int			right(int i)
	{
		return ((2*i)+2);
	}

	////////////////////////////////////////////////////////////////////////////////////
	// For Heap Sort
	// Returns The Location Of Largest Child Of Node (i)
	////////////////////////////////////////////////////////////////////////////////////
	int			largest_child(int i, int Size) const
	{
		if (left(i)<Size)
		{
			if (right(i)<Size)
			{
				return ( (mArray[right(i)] < mArray[left(i)]) ? (left(i)) : (right(i)) );
			}
			return left(i);	// Node i only has a left child, so by default it is the biggest
		}
		return i;		// Node i is a leaf, so just return it
	}

public:
    ////////////////////////////////////////////////////////////////////////////////////
	// Iterator
    ////////////////////////////////////////////////////////////////////////////////////
	class const_iterator;
	class iterator
	{
		friend class vector_base<T>;
		friend class const_iterator;
		// Data
		//------
		int						mLoc;
		vector_base<T>*			mOwner;

	public:
		// Constructors
		//--------------
		iterator()						: mOwner(0), mLoc(0)
		{}
		iterator(vector_base<T>* p, int t)	: mOwner(p), mLoc(t)
		{}

		iterator(const iterator &t)		: mOwner(t.mOwner), mLoc(t.mLoc)
		{}

		// Assignment Operator
		//---------------------
		void		operator= (const iterator &t)
		{
			mOwner	= t.mOwner;
			mLoc	= t.mLoc;
		}


		// Equality Operators
		//--------------------
		bool		operator!=(const iterator &t)	const
		{
			return (mLoc!=t.mLoc  || mOwner!=t.mOwner);
		}
		bool		operator==(const iterator &t)	const
		{
			return (mLoc==t.mLoc && mOwner==t.mOwner);
		}

		// DeReference Operator
		//----------------------
		TTValue&			operator* ()	const
		{
			assert(mLoc>=0 && mLoc<mOwner->mSize);
			return (mOwner->mArray[mLoc]);
		}
		// DeReference Operator
		//----------------------
		TTValue&			value()	const
		{
			assert(mLoc>=0 && mLoc<mOwner->mSize);
			return (mOwner->mArray[mLoc]);
		}

		// DeReference Operator
		//----------------------
		TTValue*			operator-> ()	const
		{
			assert(mLoc>=0 && mLoc<mOwner->mSize);
			return (&mOwner->mArray[mLoc]);
		}

		// Inc Operator
		//--------------
		iterator	operator++(int) //postfix
		{
			assert(mLoc>=0 && mLoc<mOwner->mSize);
			iterator old(*this);
			mLoc ++;
			return old;
		}

		// Inc Operator
		//--------------
		iterator	operator++()
		{
			assert(mLoc>=0 && mLoc<mOwner->mSize);
			mLoc ++;
			return *this;
		}

	};

    ////////////////////////////////////////////////////////////////////////////////////
	// Constant Iterator
    ////////////////////////////////////////////////////////////////////////////////////
	class const_iterator
	{
		friend class vector_base<T>;

		int							mLoc;
		const vector_base<T>*		mOwner;

	public:
		// Constructors
		//--------------
		const_iterator()							: mOwner(0), mLoc(0)
		{}
		const_iterator(const vector_base<T>* p, int t)	: mOwner(p), mLoc(t)
		{}
		const_iterator(const const_iterator &t)		: mOwner(t.mOwner), mLoc(t.mLoc)
		{}
		const_iterator(const iterator &t)			: mOwner(t.mOwner), mLoc(t.mLoc)
		{}

		// Assignment Operator
		//---------------------
		void		operator= (const const_iterator &t)
		{
			mOwner	= t.mOwner;
			mLoc	= t.mLoc;
		}
		// Assignment Operator
		//---------------------
		void		operator= (const iterator &t)
		{
			mOwner	= t.mOwner;
			mLoc	= t.mLoc;
		}



		// Equality Operators
		//--------------------
		bool		operator!=(const iterator &t)		const
		{
			return (mLoc!=t.mLoc  || mOwner!=t.mOwner);
		}
		bool		operator==(const iterator &t)		const
		{
			return (mLoc==t.mLoc && mOwner==t.mOwner);
		}

		// Equality Operators
		//--------------------
		bool		operator!=(const const_iterator &t)		const
		{
			return (mLoc!=t.mLoc || mOwner!=t.mOwner);
		}
		bool		operator==(const const_iterator &t)		const
		{
			return (mLoc==t.mLoc  && mOwner==t.mOwner);
		}

		// DeReference Operator
		//----------------------
		const TTValue&			operator* ()		const
		{
			assert(mLoc>=0 && mLoc<mOwner->mSize);
			return (mOwner->mArray[mLoc]);
		}

		// DeReference Operator
		//----------------------
		const TTValue&			value()		const
		{
			assert(mLoc>=0 && mLoc<mOwner->mSize);
			return (mOwner->mArray[mLoc]);
		}

		// DeReference Operator
		//----------------------
		const TTValue*			operator-> ()		const
		{
			assert(mLoc>=0 && mLoc<mOwner->mSize);
			return (&mOwner->mArray[mLoc]);
		}

		// Inc Operator
		//--------------
		const_iterator operator++(int)
		{
			assert(mLoc>=0 && mLoc<mOwner->mSize);
			const_iterator old(*this);
			mLoc ++;
			return old;
		}

		// Inc Operator
		//--------------
		const_iterator operator++()
		{
			assert(mLoc>=0 && mLoc<mOwner->mSize);
			mLoc ++;
			return *this;
		}


	};
	friend class				iterator;
	friend class				const_iterator;


    ////////////////////////////////////////////////////////////////////////////////////
	// Iterator Begin (Starts At Address 0)
    ////////////////////////////////////////////////////////////////////////////////////
	iterator	begin()
	{
		return iterator(this, 0);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Iterator End (Set To Address mSize)
    ////////////////////////////////////////////////////////////////////////////////////
	iterator	end()
	{
		return iterator(this, mSize);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Iterator Begin (Starts At Address 0)
    ////////////////////////////////////////////////////////////////////////////////////
	const_iterator	begin() const
	{
		return const_iterator(this, 0);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Iterator End (Set To Address mSize)
    ////////////////////////////////////////////////////////////////////////////////////
	const_iterator	end() const
	{
		return const_iterator(this, mSize);
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Iterator Find (If Fails To Find, Returns iterator end()
	////////////////////////////////////////////////////////////////////////////////////
	iterator	find(const TTValue& value)
	{
		int		index = find_index(value);		// Call Find By Index
		if (index<mSize)
		{
			return iterator(this, index);		// Found It, Return An Iterator To Index
		}
		return end();							// Return "end" Iterator If Not Found
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Iterator Find (If Fails To Find, Returns iterator end()
	////////////////////////////////////////////////////////////////////////////////////
	const_iterator	find(const TTValue& value) const
	{
		int		index = find_index(value);		// Call Find By Index
		if (index<mSize)
		{
			return const_iterator(this, index);		// Found It, Return An Iterator To Index
		}
		return end();							// Return "end" Iterator If Not Found
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Erase An Iterator Location... NOTE: THIS DOES NOT PRESERVE ORDER IN THE VECTOR!!
	////////////////////////////////////////////////////////////////////////////////////
	iterator erase_swap(const iterator &it)
	{
		assert(it.mLoc>=0 && it.mLoc<it.mOwner->mSize);
		if (it.mLoc != mSize - 1)
		{
			mArray.swap(it.mLoc, mSize - 1);
		}
		pop_back();
		return it;
	}
	template<class CAST_TO>
	CAST_TO *verify_alloc(CAST_TO *p) const
	{
		return mArray.verify_alloc(p);
	}
};

template<class T, int ARG_CAPACITY>
class vector_vs : public vector_base<storage::value_semantics<T,ARG_CAPACITY> >
{
public:
	typedef typename storage::value_semantics<T,ARG_CAPACITY> TStorageTraits;
	typedef typename TStorageTraits::TValue TTValue;
 	enum
	{
		CAPACITY		= ARG_CAPACITY
	};
	vector_vs() {}
};

template<class T, int ARG_CAPACITY>
class vector_os : public vector_base<storage::object_semantics<T,ARG_CAPACITY> >
{
public:
	typedef typename storage::object_semantics<T,ARG_CAPACITY> TStorageTraits;
	typedef typename TStorageTraits::TValue TTValue;
 	enum
	{
		CAPACITY		= ARG_CAPACITY
	};
	vector_os() {}
};

template<class T, int ARG_CAPACITY, int ARG_MAX_CLASS_SIZE>
class vector_is : public vector_base<storage::virtual_semantics<T,ARG_CAPACITY,ARG_MAX_CLASS_SIZE> >
{
public:
	typedef typename storage::virtual_semantics<T,ARG_CAPACITY,ARG_MAX_CLASS_SIZE> TStorageTraits;
	typedef typename TStorageTraits::TValue TTValue;
 	enum
	{
		CAPACITY		= ARG_CAPACITY,
		MAX_CLASS_SIZE	= ARG_MAX_CLASS_SIZE
	};
	vector_is() {}
};

}
