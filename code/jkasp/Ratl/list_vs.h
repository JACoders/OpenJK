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
// List
// ----
// The list class supports ordered insertion and deletion in O(1) constant time.
// It simulates a linked list of pointers by allocating free spots in a pool and
// maintaining "links" as indicies to the pool array objects.
//
//
//
// NOTES:
//
//
//
////////////////////////////////////////////////////////////////////////////////////////
#if !defined(RATL_LIST_VS_INC)
#define RATL_LIST_VS_INC


////////////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////////////
#if defined(RA_DEBUG_LINKING)
	#pragma message("...including list_vs.h")
#endif
#if !defined(RATL_COMMON_INC)
	#include "ratl_common.h"
#endif
#if !defined(RATL_POOL_VS_INC)
	#include "pool_vs.h"
#endif
namespace ratl
{


// this is private to the list, but you have no access to it, soooo..
class list_node
{
public:
	int		mNext;
	int		mPrev;
};

////////////////////////////////////////////////////////////////////////////////////////
// The List Class
////////////////////////////////////////////////////////////////////////////////////////
template <class T>
class list_base : public ratl_base
{
public:
	typedef typename T TStorageTraits;
	typedef typename T::TValue TTValue;
    ////////////////////////////////////////////////////////////////////////////////////
	// Capacity Enum
    ////////////////////////////////////////////////////////////////////////////////////
	static const int CAPACITY		= T::CAPACITY;
private:
    ////////////////////////////////////////////////////////////////////////////////////
	// Data
    ////////////////////////////////////////////////////////////////////////////////////
	pool_base<TStorageTraits>		mPool;				// The Allocation Data Pool

	int				mFront;				// The Beginning Of The List
	int				mBack;				// The End Of The List
	static const int NULL_NODE		= -1;

public:
    ////////////////////////////////////////////////////////////////////////////////////
	// Constructor
    ////////////////////////////////////////////////////////////////////////////////////
	list_base() : mFront(NULL_NODE), mBack(NULL_NODE)
	{
	}

	////////////////////////////////////////////////////////////////////////////////////
	// How Many Objects Are In This List
    ////////////////////////////////////////////////////////////////////////////////////
	int			size() const
	{
		return (mPool.size());
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Are There Any Objects In This List?
    ////////////////////////////////////////////////////////////////////////////////////
	bool		empty() const
	{
		assert(mFront!=NULL_NODE || size()==0);
		return (mFront==NULL_NODE);
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Is This List Filled?
    ////////////////////////////////////////////////////////////////////////////////////
	bool		full() const
	{
		return (mPool.full());
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Clear All Elements
    ////////////////////////////////////////////////////////////////////////////////////
	void		clear()
	{
		mFront = NULL_NODE;
		mBack  = NULL_NODE;
		mPool.clear();
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Get The First Object In The List
    ////////////////////////////////////////////////////////////////////////////////////
	TTValue &			front()
	{
		assert(mFront!=NULL_NODE);  // this is empty
		return mPool[mFront];
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Get The Last Object In The List
    ////////////////////////////////////////////////////////////////////////////////////
	TTValue &			back()
	{
		assert(mBack!=NULL_NODE);
		return mPool[mBack];
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Get The First Object In The List
    ////////////////////////////////////////////////////////////////////////////////////
	const TTValue &			front() const
	{
		assert(mFront!=NULL_NODE);  // this is empty
		return mPool[mFront];
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Get The Last Object In The List
    ////////////////////////////////////////////////////////////////////////////////////
	const TTValue &			back() const
	{
		assert(mBack!=NULL_NODE);
		return mPool[mBack];
	}

public:
    ////////////////////////////////////////////////////////////////////////////////////
	// Iterator
    ////////////////////////////////////////////////////////////////////////////////////
	class const_iterator;
	class iterator
	{
		friend class list_base<T>;
		friend class const_iterator;

		int						mLoc;
		list_base<T>*			mOwner;


	public:
		// Constructors
		//--------------
		iterator()						: mOwner(0), mLoc(0)
		{}
		iterator(list_base* p, int t)		: mOwner(p), mLoc(t)
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
			assert(mLoc>=0 && mLoc<T::CAPACITY);
			return (mOwner->mPool[mLoc]);
		}

		// DeReference Operator
		//----------------------
		TTValue&			value()	const
		{
			assert(mLoc>=0 && mLoc<T::CAPACITY);
			return (mOwner->mPool[mLoc]);
		}

		// DeReference Operator
		//----------------------
		TTValue*			operator-> ()	const
		{
			assert(mLoc>=0 && mLoc<T::CAPACITY);
			return (&mOwner->mPool[mLoc]);
		}

		// prefix Inc Operator
		//--------------
		iterator operator++(int)
		{
			assert(mLoc>=0 && mLoc<T::CAPACITY);
			iterator old(*this);
			mLoc = T::node(mOwner->mPool[mLoc]).mNext;
			return old;
		}

		// postfix Inc Operator
		//--------------
		iterator operator++()
		{
			assert(mLoc>=0 && mLoc<T::CAPACITY);
			mLoc = T::node(mOwner->mPool[mLoc]).mNext;
			return *this;
		}
		// prefix Inc Operator
		//--------------
		iterator operator--(int)
		{
			assert(mLoc>=0 && mLoc<T::CAPACITY);
			iterator old(*this);
			mLoc = T::node(mOwner->mPool[mLoc]).mPrev;
			return old;
		}

		//--------------
		// postfix Dec Operator
		//--------------
		iterator operator--()
		{
			assert(mLoc>=0 && mLoc<T::CAPACITY);
			mLoc = T::node(mOwner->mPool[mLoc]).mPrev;
			return *this;
		}

	};

    ////////////////////////////////////////////////////////////////////////////////////
	// Constant Iterator
    ////////////////////////////////////////////////////////////////////////////////////
	class const_iterator
	{
		friend class list_base<T>;

		int								mLoc;
		const list_base<T>*				mOwner;

	public:
		// Constructors
		//--------------
		const_iterator()							: mOwner(0), mLoc(0)
		{}
		const_iterator(const list_base* p, int t)	: mOwner(p), mLoc(t)
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
			assert(mLoc>=0 && mLoc<T::CAPACITY);
			return (mOwner->mPool[mLoc]);
		}

		// DeReference Operator
		//----------------------
		const TTValue*			operator-> ()		const
		{
			assert(mLoc>=0 && mLoc<T::CAPACITY);
			return (&mOwner->mPool[mLoc]);
		}

		// DeReference Operator
		//----------------------
		const TTValue&			value()	const
		{
			assert(mLoc>=0 && mLoc<T::CAPACITY);
			return (mOwner->mPool[mLoc]);
		}

		// prefix Inc Operator
		//--------------
		const_iterator operator++(int)
		{
			assert(mLoc>=0 && mLoc<T::CAPACITY);
			const_iterator old(*this);
			mLoc = T::node(mOwner->mPool[mLoc]).mNext;
			return old;
		}

		// postfix Inc Operator
		//--------------
		const_iterator operator++()
		{
			assert(mLoc>=0 && mLoc<T::CAPACITY);
			mLoc = T::node(mOwner->mPool[mLoc]).mNext;
			return *this;
		}
		// prefix Inc Operator
		//--------------
		const_iterator operator--(int)
		{
			assert(mLoc>=0 && mLoc<T::CAPACITY);
			const_iterator old(*this);
			mLoc = T::node(mOwner->mPool[mLoc]).mPrev;
			return old;
		}

		//--------------
		// postfix Dec Operator
		//--------------
		const_iterator operator--()
		{
			assert(mLoc>=0 && mLoc<T::CAPACITY);
			mLoc = T::node(mOwner->mPool[mLoc]).mPrev;
			return *this;
		}
	};
	friend class				iterator;
	friend class				const_iterator;

    ////////////////////////////////////////////////////////////////////////////////////
	// Iterator Begin (Starts At Address mFront)
    ////////////////////////////////////////////////////////////////////////////////////
	iterator	begin()
	{
		return iterator(this, mFront);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Iterator Begin (Starts At Address mFront)
    ////////////////////////////////////////////////////////////////////////////////////
	const_iterator	begin() const
	{
		return const_iterator(this, mFront);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Reverse Iterator Begin (Starts At Address mBack)
    ////////////////////////////////////////////////////////////////////////////////////
	iterator	rbegin()
	{
		return iterator(this, mBack);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Reverse Iterator Begin (Starts At Address mBack)
    ////////////////////////////////////////////////////////////////////////////////////
	const_iterator	rbegin() const
	{
		return const_iterator(this, mBack);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Iterator End (Set To Address NULL_NODE)  Should Work For Forward & Backward Iteration
    ////////////////////////////////////////////////////////////////////////////////////
	iterator	end()
	{
		return iterator(this, NULL_NODE);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Iterator End (Set To Address NULL_NODE)  Should Work For Forward & Backward Iteration
    ////////////////////////////////////////////////////////////////////////////////////
	const_iterator	end() const
	{
		return const_iterator(this, NULL_NODE);
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Iterator Insert (BEFORE POINTED ELEMENT)
    ////////////////////////////////////////////////////////////////////////////////////
	T&			insert(const iterator& it)
	{
		int nNew= mPool.alloc();
		insert_low(it,nNew);
		return mPool[mNew];
	}
	////////////////////////////////////////////////////////////////////////////////////
	// Iterator Insert Value(BEFORE POINTED ELEMENT)
    ////////////////////////////////////////////////////////////////////////////////////
	void	insert(const iterator& it, const TTValue& val)
	{
		int nNew= mPool.alloc(val);
		insert_low(it,nNew);
	}
	////////////////////////////////////////////////////////////////////////////////////
	// Iterator Insert Raw(BEFORE POINTED ELEMENT)
    ////////////////////////////////////////////////////////////////////////////////////
	TRatlNew*	insert_raw(const iterator& it)
	{
		TRatlNew *ret = mPool.alloc_raw();
		insert_low(it,mPool.pointer_to_index(ret));
		return ret;
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Iterator Insert (AFTER POINTED ELEMENT) (ALSO - NOT CONSTANT, WILL CHANGE it)
    ////////////////////////////////////////////////////////////////////////////////////
	T&			insert_after(iterator& it)
	{
		int nNew= mPool.alloc();
		insert_low_after(it,nNew);
		it = iterator(this, nNew);
		return mPool[mNew];
	}
	////////////////////////////////////////////////////////////////////////////////////
	// Iterator Insert Value(AFTER POINTED ELEMENT) (ALSO - NOT CONSTANT, WILL CHANGE it)
    ////////////////////////////////////////////////////////////////////////////////////
	void	insert_after(iterator& it, const TTValue& val)
	{
		int nNew= mPool.alloc(val);
		insert_low_after(it,nNew);
		it = iterator(this, nNew);
	}
	////////////////////////////////////////////////////////////////////////////////////
	// Iterator Insert Raw(AFTER POINTED ELEMENT) (ALSO - NOT CONSTANT, WILL CHANGE it)
    ////////////////////////////////////////////////////////////////////////////////////
	TRatlNew*	insert_raw_after(iterator& it)
	{
		TRatlNew *ret = mPool.alloc_raw();
		insert_low_after(it,mPool.pointer_to_index(ret));
		it = iterator(this, mPool.pointer_to_index(ret));
		return ret;
	}




	////////////////////////////////////////////////////////////////////////////////////
	// Front Insert (BEFORE POINTED ELEMENT)
    ////////////////////////////////////////////////////////////////////////////////////
	T&			push_front()
	{
		int nNew= mPool.alloc();
		insert_low(begin(),nNew);
		return mPool[mNew];
	}
	////////////////////////////////////////////////////////////////////////////////////
	// Front Insert Value(BEFORE POINTED ELEMENT)
    ////////////////////////////////////////////////////////////////////////////////////
	void	push_front(const TTValue& val)
	{
		int nNew= mPool.alloc(val);
		insert_low(begin(),nNew);
	}
	////////////////////////////////////////////////////////////////////////////////////
	// Front Insert Raw(BEFORE POINTED ELEMENT)
    ////////////////////////////////////////////////////////////////////////////////////
	TRatlNew *	push_front_raw()
	{
		TRatlNew *ret = mPool.alloc_raw();
		insert_low(begin(),mPool.pointer_to_index(ret));
		return ret;
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Front Insert (BEFORE POINTED ELEMENT)
    ////////////////////////////////////////////////////////////////////////////////////
	T&			push_back()
	{
		int nNew= mPool.alloc();
		insert_low(end(),nNew);
		return mPool[mNew];
	}
	////////////////////////////////////////////////////////////////////////////////////
	// Front Insert Value(BEFORE POINTED ELEMENT)
    ////////////////////////////////////////////////////////////////////////////////////
	void	push_back(const TTValue& val)
	{
		int nNew= mPool.alloc(val);
		insert_low(end(),nNew);
	}
	////////////////////////////////////////////////////////////////////////////////////
	// Front Insert Raw(BEFORE POINTED ELEMENT)
    ////////////////////////////////////////////////////////////////////////////////////
	TRatlNew *	push_back_raw()
	{
		TRatlNew *ret = mPool.alloc_raw();
		insert_low(end(),mPool.pointer_to_index(ret));
		return ret;
	}
	////////////////////////////////////////////////////////////////////////////////////
	// Iterator Erase
    ////////////////////////////////////////////////////////////////////////////////////
	void		erase(iterator& it)
	{
		assert(it.mOwner==this);		// Iterators must be mixed up, this is from a different list.
		assert(it.mLoc!=NULL_NODE);

		int		At				= it.mLoc;
		int		Prev			= T::node(mPool[At]).mPrev;
		int		Next			= T::node(mPool[At]).mNext;

		// LINK: (Prev)<-(At)--(Next)
		//--------------------------------------------
		if (Next!=NULL_NODE)
		{
			T::node(mPool[Next]).mPrev	= Prev;
		}

		// LINK: (Prev)--(At)->(Next)
		//--------------------------------------------
		if (Prev!=NULL_NODE)
		{
			T::node(mPool[Prev]).mNext	= Next;
		}

		// UPDATE: Front & Back
		//----------------------
		if (At==mBack)
		{
			mBack				= Prev;
		}
		if (At==mFront)
		{
			mFront				= Next;
		}

		// ERASE At
		//--------------------------------------------
		mPool.free(At);
		it.mLoc = Prev;
	}
	template<class CAST_TO>
	CAST_TO *verify_alloc(CAST_TO *p) const
	{
		return mPool.verify_alloc(p);
	}
private:

	////////////////////////////////////////////////////////////////////////////////////
	// Iterator Insert, returns pool index
    ////////////////////////////////////////////////////////////////////////////////////
	void insert_low(const iterator& it,int nNew)
	{
		assert(it.mOwner==this);	// Iterators must be mixed up, this is from a different list.

		int		Next				= it.mLoc;
		int		Prev				= NULL_NODE;
		if (Next!=NULL_NODE)
		{
			Prev = T::node(mPool[Next]).mPrev;
		}
		else
		{
			Prev = mBack;
		}


		assert(nNew!=Next && nNew!=Prev);


		// LINK: (Prev)<-(New)->(Next)
		//--------------------------------------------
		T::node(mPool[nNew]).mPrev			= Prev;
		T::node(mPool[nNew]).mNext			= Next;


		// LINK:         (New)<-(Next)
		//--------------------------------------------
		if (Next!=NULL_NODE)
		{
			T::node(mPool[Next]).mPrev		= nNew;
			assert(T::node(mPool[Next]).mPrev!=T::node(mPool[Next]).mNext);
		}
		else
		{
			mBack					= nNew;
		}


		// LINK: (Prev)->(New)
		//--------------------------------------------
		if (Prev!=NULL_NODE)
		{
			T::node(mPool[Prev]).mNext		= nNew;
			assert(T::node(mPool[Prev]).mPrev!=T::node(mPool[Prev]).mNext);
		}
		else
		{
			mFront					= nNew;
		}
	}


	////////////////////////////////////////////////////////////////////////////////////
	// Iterator Insert, returns pool index (AFTER POINTED ELEMENT)
    ////////////////////////////////////////////////////////////////////////////////////
	void insert_low_after(const iterator& it,int nNew)
	{
		assert(it.mOwner==this);	// Iterators must be mixed up, this is from a different list.

		int		Next				= NULL_NODE;//it.mLoc;
		int		Prev				= it.mLoc;//NULL_NODE;
		if (Prev!=NULL_NODE)
		{
			Next = T::node(mPool[Prev]).mNext;
		}
		else
		{
			Prev = mFront;
		}


		assert(nNew!=Next && nNew!=Prev);


		// LINK: (Prev)<-(New)->(Next)
		//--------------------------------------------
		T::node(mPool[nNew]).mPrev			= Prev;
		T::node(mPool[nNew]).mNext			= Next;


		// LINK:         (New)<-(Next)
		//--------------------------------------------
		if (Next!=NULL_NODE)
		{
			T::node(mPool[Next]).mPrev		= nNew;
			assert(T::node(mPool[Next]).mPrev!=T::node(mPool[Next]).mNext);
		}
		else
		{
			mBack					= nNew;
		}


		// LINK: (Prev)->(New)
		//--------------------------------------------
		if (Prev!=NULL_NODE)
		{
			T::node(mPool[Prev]).mNext		= nNew;
			assert(T::node(mPool[Prev]).mPrev!=T::node(mPool[Prev]).mNext);
		}
		else
		{
			mFront					= nNew;
		}
	}

};

template<class T, int ARG_CAPACITY>
class list_vs : public list_base<storage::value_semantics_node<T,ARG_CAPACITY,list_node> >
{
public:
	typedef typename storage::value_semantics_node<T,ARG_CAPACITY,list_node> TStorageTraits;
	typedef typename TStorageTraits::TValue TTValue;
	static const int CAPACITY		= ARG_CAPACITY;
	list_vs() {}
};

template<class T, int ARG_CAPACITY>
class list_os : public list_base<storage::object_semantics_node<T,ARG_CAPACITY,list_node> >
{
public:
	typedef typename storage::object_semantics_node<T,ARG_CAPACITY,list_node> TStorageTraits;
	typedef typename TStorageTraits::TValue TTValue;
	static const int CAPACITY		= ARG_CAPACITY;
	list_os() {}
};

template<class T, int ARG_CAPACITY, int ARG_MAX_CLASS_SIZE>
class list_is : public list_base<storage::virtual_semantics_node<T,ARG_CAPACITY,ARG_MAX_CLASS_SIZE,list_node> >
{
public:
	typedef typename storage::virtual_semantics_node<T,ARG_CAPACITY,ARG_MAX_CLASS_SIZE,list_node> TStorageTraits;
	typedef typename TStorageTraits::TValue TTValue;
	static const int CAPACITY		= ARG_CAPACITY;
	static const int MAX_CLASS_SIZE	= ARG_MAX_CLASS_SIZE;
	list_is() {}
};

}
#endif
