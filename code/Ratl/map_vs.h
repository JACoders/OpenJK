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
// Map
// ---
// This map is based on a red black tree, which guarentees balanced data, no mater what
// order elements are added.  The map uses a memory pool for storage of node data.
//
//
////////////////////////////////////////////////////////////////////////////////////////
#if !defined(RATL_MAP_VS_INC)
#define RATL_MAP_VS_INC


////////////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////////////
#if defined(RA_DEBUG_LINKING)
	#pragma message("...including map_vs.h")
#endif
#if !defined(RATL_COMMON_INC)
	#include "ratl_common.h"
#endif
#if !defined(RATL_POOL_VS_INC)
	#include "pool_vs.h"
#endif
namespace ratl
{


// this is private to the set, but you have no access to it, soooo..

class tree_node
{
	int		mParent;
	int		mLeft;
	int		mRight;
public:
 	enum
	{
		RED_BIT			= 0x40000000, // to save space we are putting the red bool in a high bit
										// this is in the parent only
		NULL_NODE		= 0x3fffffff, // this must not have the red bit set
	};
	void	init()
	{
		mLeft	= NULL_NODE;
		mRight	= NULL_NODE;
		mParent	= NULL_NODE | RED_BIT;
	}
	int left() const
	{
		return mLeft;
	}
	int right() const
	{
		return mRight;
	}
	int parent() const
	{
		return mParent & (~RED_BIT);
	}
	bool red() const
	{
		return !!(mParent & RED_BIT);
	}
	void set_left(int l)
	{
		mLeft = l;
	}
	void set_right(int r)
	{
		mRight = r;
	}
	void set_parent(int p)
	{
		mParent &= RED_BIT;
		mParent	|= p;
	}
	void set_red(bool isRed)
	{
		if (isRed)
		{
			mParent |= RED_BIT;
		}
		else
		{
			mParent &= ~RED_BIT;
		}
	}
};

//fixme void *, comparison function pointer-ize this for code bloat.

template<class T, int IS_MULTI>
class tree_base
{
public:
    typedef T TStorageTraits;
	typedef typename T::TValue TTValue;
    ////////////////////////////////////////////////////////////////////////////////////
	// Capacity Enum
    ////////////////////////////////////////////////////////////////////////////////////
 	static const int CAPACITY = T::CAPACITY;

private:
	pool_base<TStorageTraits>		mPool;				// The Allocation Data Pool
	int								mRoot;
	int								mLastAdd;


	void link_left(int node,int left)
	{
		T::node(mPool[node]).set_left(left);
		if (left!=tree_node::NULL_NODE)
		{
			T::node(mPool[left]).set_parent(node);
		}
	}

	void link_right(int node,int right)
	{
		T::node(mPool[node]).set_right(right);
		if (right!=tree_node::NULL_NODE)
		{
			T::node(mPool[right]).set_parent(node);
		}
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// This is the map internal recursive find function - do not use externally
    ////////////////////////////////////////////////////////////////////////////////////
	int			find_internal(const TTValue &key, int at) const
	{

		// FAIL TO FIND?
		//---------------
		if (at==tree_node::NULL_NODE)
		{
			return tree_node::NULL_NODE;
		}

		// Should We Search Left?
		//------------------------
		if      (key<mPool[at])
		{
			return find_internal(key, T::node(mPool[at]).left());
		}

		// Should We Search Right?
		//------------------------
		else if (mPool[at]<key)
		{
			return find_internal(key, T::node(mPool[at]).right());
		}

		// FOUND!
		//--------
		return at;
	}


    ////////////////////////////////////////////////////////////////////////////////////
	// This is the map internal recursive find function - do not use externally
    ////////////////////////////////////////////////////////////////////////////////////
	int			find_internal(const TTValue &key, int target, int at, int& parent) const
	{
		// FAIL TO FIND?
		//---------------
		if (at==tree_node::NULL_NODE)
		{
			parent = tree_node::NULL_NODE;
			return tree_node::NULL_NODE;
		}

		// FOUND!
		//--------
		if (at==target)
		{
			if (at==mRoot)
			{
				parent = tree_node::NULL_NODE;
			}
			return at;
		}

		// Should We Search Left?
		//------------------------
		if      (key<mPool[at])
		{
			parent = at;
			return find_internal(key, target, T::node(mPool[at]).left(), parent);
		}

		parent = at;
		return find_internal(key, target, T::node(mPool[at]).right(), parent);
	}


    ////////////////////////////////////////////////////////////////////////////////////
	// This is the map internal recursive insertion - do not use externally
    ////////////////////////////////////////////////////////////////////////////////////
	int			insert_internal(const TTValue &key, int &at)
	{
		// If At Is A NULL_NODE, We Have Found A Leaf.
		//----------------------------------------------
		if (at==tree_node::NULL_NODE)
		{
			if (mRoot==tree_node::NULL_NODE)
			{
				mRoot = mLastAdd;
			}
			return tree_node::NULL_NODE;					// There Are No Excess Red Children (No Childeren At All, Actually)
		}


		int		nxtChild;						// The Child We Will Eventually Add Underneath
		//int		altChild;						// The "other" Child
		bool	nxtRotateLeft;
		int		excessRedChild;					// If The Insert Results In An Excess Red Child, This Will Be It


		// Choose Which Side To Add The New Node Under
		//---------------------------------------------
		if (key < mPool[at])						// The Key Classes Must Support A < Operator
		{
			int tmp=T::node(mPool[at]).left();
			excessRedChild = insert_internal(key,tmp);
			link_left(at,tmp);//T::node(mPool[at]).set_left(tmp);

			if (tmp==tree_node::NULL_NODE)
			{

				link_left(at,mLastAdd);//T::node(mPool[at]).set_left(mLastAdd);			// If mLeft Of The Current Node Is NULL, We Must Have Added DIRECTLY Below nAt
			}
			nxtChild = T::node(mPool[at]).left();
			//altChild = T::node(mPool[at]).right();
			nxtRotateLeft = false;
		}
		else if (mPool[at] < key)
		{
			int tmp=T::node(mPool[at]).right();
			excessRedChild = insert_internal(key,tmp);
			link_right(at,tmp); // T::node(mPool[at]).set_right(tmp);

			if (tmp==tree_node::NULL_NODE)
			{
				link_right(at,mLastAdd); // T::node(mPool[at]).set_right(mLastAdd);			// If mRight Of The Current Node Is NULL, We Must Have Added DIRECTLY Below nAt
			}
			nxtChild = T::node(mPool[at]).right();
			//altChild = T::node(mPool[at]).left();
			nxtRotateLeft = true;
		}

		// Exact Match
		//-------------
		else
		{
			// the node of interest is at
			return tree_node::NULL_NODE;
		}


		// If The Add Resulted In An Excess Red Child, We Need To Change Colors And Rotate
		//---------------------------------------------------------------------------------
		if (excessRedChild!=tree_node::NULL_NODE)
		{
			// If Both Childeren Are Red, Just Switch And Be Done
			//----------------------------------------------------
			if (T::node(mPool[at]).right()!=tree_node::NULL_NODE &&
				T::node(mPool[at]).left()!=tree_node::NULL_NODE &&
				T::node(mPool[T::node(mPool[at]).right()]).red() &&
				T::node(mPool[T::node(mPool[at]).left()]).red())
			{
				set_colors(T::node(mPool[at]), true, false);
			}
			else
			{
				int	excessRedChildCompare =
					(nxtRotateLeft)?(T::node(mPool[nxtChild]).right()):(T::node(mPool[nxtChild]).left());
				if (excessRedChild==excessRedChildCompare)
				{
					// Single Rotation
					//-----------------
					rotate(at,   nxtRotateLeft);
				}
				else
				{
					if (nxtRotateLeft)
					{
						int nxt=T::node(mPool[at]).right();
						rotate(nxt, false);
						link_right(at,nxt); // T::node(mPool[at]).set_right(nxt);
					}
					else
					{
						int nxt=T::node(mPool[at]).left();
						rotate(nxt,true);
						link_left(at,nxt); // T::node(mPool[at]).set_left(nxt);
					}
					rotate(at,   nxtRotateLeft);
				}

				set_colors(T::node(mPool[at]), false, true);
			}
		}

		if (T::node(mPool[at]).red())
		{
			if (T::node(mPool[at]).left()!=tree_node::NULL_NODE &&
				T::node(mPool[T::node(mPool[at]).left()]).red())
			{
				return T::node(mPool[at]).left();
			}
			if (T::node(mPool[at]).right()!=tree_node::NULL_NODE &&
				T::node(mPool[T::node(mPool[at]).right()]).red())
			{
				return T::node(mPool[at]).right();
			}
		}
		return tree_node::NULL_NODE;
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// This is the map internal recursive erase - do not use externally
    ////////////////////////////////////////////////////////////////////////////////////
	bool	erase_internal(const TTValue &key, int& at)
	{
		// If At Is A NULL_NODE, We Have Found A Leaf.
		//---------------------------------------------
		if (at==tree_node::NULL_NODE)
		{
			return true;
		}

		//==============================================================================
		// Now The Question Is, Do We Need To Continue Searching?
		//==============================================================================


		// Recurse To The Left?
		//----------------------
		if (key < mPool[at])
		{
			int a=T::node(mPool[at]).left();
			bool r=erase_internal(key, a);
			link_left(at,a); // T::node(mPool[at]).set_left(a);
			if (!r)		// If It Was Not Red, We Need To Rebalance
			{
				return rebalance(at, true);
			}
			return true;
		}

		// Recurse To The Right?
		//-----------------------
		if (mPool[at] < key)
		{
			int a=T::node(mPool[at]).right();
			bool r=erase_internal(key, a);
			link_right(at,a); // T::node(mPool[at]).set_right(a);
			if (!r)		// If It Was Not Red, We Need To Rebalance
			{
				return rebalance(at, false);
			}
			return true;
		}

		//==============================================================================
		// At This Point, We Must Have Discovered An Exact Match For Our Key
		//==============================================================================


		// Are There Any Open Childeren Slots?
		//-------------------------------------
		if (T::node(mPool[at]).left()==tree_node::NULL_NODE || T::node(mPool[at]).right()==tree_node::NULL_NODE)
		{
			bool	atWasRed	= T::node(mPool[at]).red();
			int		oldAt		= at;

			at = (T::node(mPool[at]).left()==tree_node::NULL_NODE)?(T::node(mPool[at]).right()):(T::node(mPool[at]).left());	// If Left Is Null, At Goes Right

			// Actually Free It!
			//-------------------
			mPool.free(oldAt);

			// If We Are Now At A Null Node, Just Return
			//-------------------------------------------
			if (at==tree_node::NULL_NODE)
			{
				return atWasRed;
			}

			// Otherwise, Mark The New Child As Red, And Return That Fact Up
			//---------------------------------------------------------------
			T::node(mPool[at]).set_red(false);
			return true;
		}


		//==============================================================================
		// There Are No Childeren To Link With, We Are In The Middle Of The Tree.
		// We Need To Find An Open Leaf, Swap Data With That Leaf, and Then Go Find It
		//==============================================================================

		// Find A Successor Leaf
		//-----------------------
		int at_parent = T::node(mPool[at]).parent();
		int	successor = T::node(mPool[at]).right();

		int parent_successor=-1;
		while(T::node(mPool[successor]).left()!=tree_node::NULL_NODE)
		{
			parent_successor=successor;
			successor = T::node(mPool[successor]).left();
		}

		int	successor_right = T::node(mPool[successor]).right();


		link_left(successor,T::node(mPool[at]).left());

		bool red=T::node(mPool[successor]).red();
		T::node(mPool[successor]).set_red(T::node(mPool[at]).red());
		T::node(mPool[at]).set_red(red);

		if (parent_successor!=-1)
		{

			link_right(successor,T::node(mPool[at]).right());
			link_left(parent_successor,at);
		}
		else
		{
			link_right(successor,at);
		}

		if (at_parent!=tree_node::NULL_NODE)
		{
			if (T::node(mPool[at_parent]).left()==at)
			{
				//my parents left child
				link_left(at_parent,successor);
			}
			else
			{
				assert(T::node(mPool[at_parent]).right()==at); // better be my parents right child then
				link_right(at_parent,successor);
			}
		}

		link_left(at,tree_node::NULL_NODE);
		link_right(at,successor_right);

		at=successor;
		int a=T::node(mPool[at]).right();
		bool r=erase_internal(key, a);
		link_right(at,a); // T::node(mPool[at]).set_right(a);
		// And Keep Going
		//----------------
		if (!r)		// If It Was Not Red, We Need To Rebalance
		{
			return rebalance(at, false);
		}
		return true;
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// HELPER: Change the color of a node and children
    ////////////////////////////////////////////////////////////////////////////////////
	void		set_colors(tree_node& at, bool red, bool childRed)
	{
		at.set_red(red);
		if (at.left()!=tree_node::NULL_NODE)
		{
			T::node(mPool[at.left()]).set_red(childRed);
		}
		if (at.right()!=tree_node::NULL_NODE)
		{
			T::node(mPool[at.right()]).set_red(childRed);
		}
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// HELPER: Rotate node located at (at) either left or right
    ////////////////////////////////////////////////////////////////////////////////////
	void		rotate(int& at, bool left)
	{
		int	t;
		if (left)
		{
			assert(T::node(mPool[at]).right()!=tree_node::NULL_NODE);

			t					= T::node(mPool[at]).right();
			link_right(at,T::node(mPool[t]).left()); // T::node(mPool[at]).set_right(T::node(mPool[t]).left());
			link_left(t,at);	// T::node(mPool[t]).set_left(at);
			at					= t;
		}
		else
		{
			assert(T::node(mPool[at]).left()!=tree_node::NULL_NODE);

			t					= T::node(mPool[at]).left();
			link_left(at,T::node(mPool[t]).right()); // T::node(mPool[at]).set_left(T::node(mPool[t]).right());
			link_right(t,at);	//T::node(mPool[t]).set_right(at);
			at					= t;
		}
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// HELPER: Localally rebalance the tree here
    ////////////////////////////////////////////////////////////////////////////////////
	bool		rebalance(int& at, bool left)
	{
		// Decide Which Child, Left Or Right?
		//------------------------------------
		int	w = (left)?(T::node(mPool[at]).right()):(T::node(mPool[at]).left());	// w is the child of at
		if (w==tree_node::NULL_NODE)
		{
			bool	atWasRed = T::node(mPool[at]).red();			// Remember what mPool[at] WAS
			T::node(mPool[at]).set_red(false);						// Mark mPool[at] as BLACK
			return  atWasRed;						// Return what it used to be
		}


		// Get A Reference To The Child W, And Record It's Children x And y
		//------------------------------------------------------------------
		tree_node&	wAt = T::node(mPool[w]);
		int		x = (left)?(wAt.left()):(wAt.right());// x and y are the grand children of at
		int		y = (left)?(wAt.right()):(wAt.left());


		// Is The Child Black?
		//---------------------
		if (!wAt.red())
		{
			// If Both X and Y are Empty, Or Both Are Red
			//--------------------------------------------
			if ((x==tree_node::NULL_NODE || !T::node(mPool[x]).red()) &&
				(y==tree_node::NULL_NODE || !T::node(mPool[y]).red()))
			{
				bool	atWasRed = T::node(mPool[at]).red();		// Remember what mPool[at] WAS
				T::node(mPool[at]).set_red(false);		// Mark mPool[at] as BLACK
				wAt.set_red(true);					// Mark The Child As RED
				return  atWasRed;					// Return what it used to be
			}

			// If Y Is Valid
			//---------------
			if (y!=tree_node::NULL_NODE && T::node(mPool[y]).red())
			{
				wAt.set_red(T::node(mPool[at]).red());
				rotate(at, left);
				T::node(mPool[T::node(mPool[at]).left()]).set_red(false);
				T::node(mPool[T::node(mPool[at]).right()]).set_red(false);
				return true;
			}

			// X Must Be Valid
			//-----------------
			T::node(mPool[x]).set_red(T::node(mPool[at]).red());
			T::node(mPool[at]).set_red(false);

			if (left)
			{
				int r=T::node(mPool[at]).right();
				rotate(r,false);
				link_right(at,r); // T::node(mPool[at]).set_right(r);
			}
			else
			{
				int r=T::node(mPool[at]).left();
				rotate(r,true);
				link_left(at,r); // T::node(mPool[at]).set_left(r);
			}
			rotate(at, left);

			return true;
		}

		// The Child Must Have Been Red
		//------------------------------
		wAt.set_red(T::node(mPool[at]).red());				// Switch Child Color
		T::node(mPool[at]).set_red(true);
		rotate(at, left);					// Rotate At

		// Select The Next Rebalance Child And Recurse
		//----------------------------------------------
		if (left)
		{
			int r=T::node(mPool[at]).left();
			rebalance(r,true);
			link_left(at,r); // T::node(mPool[at]).set_left(r);
		}
		else
		{
			int r=T::node(mPool[at]).right();
			rebalance(r,false);
			link_right(at,r); //T::node(mPool[at]).set_right(r);
		}
		return true;
	}
    ////////////////////////////////////////////////////////////////////////////////////
	// This is the map internal recursive front function - do not use externally
    ////////////////////////////////////////////////////////////////////////////////////
	int			front(int at) const
	{
		if (at!=tree_node::NULL_NODE &&
			T::node(mPool[at]).left()!=tree_node::NULL_NODE)
		{
			return front(T::node(mPool[at]).left());
		}
		return at;
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// This is the map internal recursive back function - do not use externally
    ////////////////////////////////////////////////////////////////////////////////////
	int			back(int at) const
	{
		if (at!=tree_node::NULL_NODE && T::node(mPool[at]).right()!=tree_node::NULL_NODE)
		{
			return back(T::node(mPool[at]).right());
		}
		return at;
	}
protected:
	int			front() const
	{
		return front(mRoot);
	}

	int			back() const
	{
		return back(mRoot);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// This is the map internal recursive next function - do not use externally
    ////////////////////////////////////////////////////////////////////////////////////
	int			next(int at) const
	{
		assert(at!=tree_node::NULL_NODE);
		const TTValue&	kAt = mPool[at];
		const tree_node&	nAt = T::node(kAt);
		if (nAt.right()!=tree_node::NULL_NODE)
		{
			return front(nAt.right());
		}

		int		child	= at;
		int		parent	= tree_node::NULL_NODE;
		find_internal(kAt, at, mRoot, parent);

		while(parent!=tree_node::NULL_NODE && (child==T::node(mPool[parent]).right()))
		{
			child = parent;
			find_internal(mPool[parent], parent, mRoot, parent);
		}
		return parent;
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// This is the map internal recursive previous function - do not use externally
    ////////////////////////////////////////////////////////////////////////////////////
	int			previous(int at) const
	{
		assert(at!=tree_node::NULL_NODE);
		const TTValue&	kAt = mPool[at];
		const tree_node&		nAt = T::node(mPool[at]);
		if (kAt.left()!=tree_node::NULL_NODE)
		{
			return back(kAt.left());
		}

		int		child	= at;
		int		parent	= tree_node::NULL_NODE;
		find_internal(nAt, at, mRoot, parent);

		while(parent!=tree_node::NULL_NODE && (child==T::node(mPool[parent]).left()))
		{
			child = parent;
			find_internal(mPool[parent], parent, mRoot, parent);
		}
		return parent;
	}



public:

    ////////////////////////////////////////////////////////////////////////////////////
	// Constructor
    ////////////////////////////////////////////////////////////////////////////////////
    tree_base() : mRoot(tree_node::NULL_NODE), mLastAdd(-1)
	{
	}


	////////////////////////////////////////////////////////////////////////////////////
	// How Many Objects Are In This Map
    ////////////////////////////////////////////////////////////////////////////////////
	int			size() const
	{
		return (mPool.size());
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Are There Any Objects In This Map?
    ////////////////////////////////////////////////////////////////////////////////////
	bool		empty() const
	{
		return (mPool.empty());
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Is This Map Filled?
    ////////////////////////////////////////////////////////////////////////////////////
	bool		full() const
	{
		return (mPool.full());
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Clear All Data From The Map
    ////////////////////////////////////////////////////////////////////////////////////
	void		clear()
	{
		mRoot = tree_node::NULL_NODE;
		mPool.clear();
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Adds Element Value At Location Key  - O(log n)
    ////////////////////////////////////////////////////////////////////////////////////
	void alloc_key(const TTValue &key)
	{

		//fixme handle duplicates more sensibly?
		assert(!full());
		mLastAdd = mPool.alloc(key);			// Grab A New One
		T::node(mPool[mLastAdd]).init();	// Initialize Our Data And Color
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Allocs an item, when filled, call insert_alloced
    ////////////////////////////////////////////////////////////////////////////////////
	TTValue & alloc_key()
	{
		assert(!full());
		mLastAdd = mPool.alloc();			// Grab A New One
		T::node(mPool[mLastAdd]).init();	// Initialize Our Data And Color
		return mPool[mLastAdd];
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Allocs an item (raw), when constucted, call insert_alloced
    ////////////////////////////////////////////////////////////////////////////////////
	TRatlNew *alloc_key_raw()
	{
		assert(!full());
		TRatlNew *ret=mPool.alloc_raw();			// Grab A New One
		mLastAdd = mPool.pointer_to_index(ret);
		T::node(mPool[mLastAdd]).init();	// Initialize Our Data And Color
		return ret;
	}
	template<class CAST_TO>
	CAST_TO *verify_alloc_key(CAST_TO *p) const
	{
		return mPool.verify_alloc(p);
	}

	void insert_alloced_key()
	{
		assert(mLastAdd>=0&&mLastAdd<CAPACITY);
		assert(!IS_MULTI || find_index(mPool[mLastAdd])!=tree_node::NULL_NODE); //fixme handle duplicates more sensibly?

		insert_internal(mPool[mLastAdd],mRoot);
		assert(mRoot!=tree_node::NULL_NODE);
		T::node(mPool[mRoot]).set_red(false);
		T::node(mPool[mRoot]).set_parent(tree_node::NULL_NODE);
	}

	int index_of_alloced_key() const
	{
		assert(mLastAdd>=0&&mLastAdd<CAPACITY);
		return mLastAdd;
	}
    ////////////////////////////////////////////////////////////////////////////////////
	// Removes The Element Pointed To At (it) And Decrements (it)  - O((log n)^2)
    ////////////////////////////////////////////////////////////////////////////////////
	void erase_index(int i)
	{
		assert(i>=0&&i<CAPACITY);
		assert(mRoot!=tree_node::NULL_NODE);

		//fixme this is lame to have to look by key to erase
		erase_internal(mPool[i],mRoot);
		if (mRoot!=tree_node::NULL_NODE)
		{
			T::node(mPool[mRoot]).set_red(false);
			T::node(mPool[mRoot]).set_parent(tree_node::NULL_NODE);
		}
	}
    ////////////////////////////////////////////////////////////////////////////////////
	// Seach For A Given Key.  Will Return -1 if Failed  - O(log n)
    ////////////////////////////////////////////////////////////////////////////////////
	int	find_index(const TTValue &key) const
	{
		return find_internal(key, mRoot);
	}
	const TTValue &index_to_key(int i) const
	{
		assert(i>=0&&i<CAPACITY);
		return mPool[i];
	}

	//fixme lower bound, upper bound, equal range


};



template<class T,int IS_MULTI>
class set_base : public tree_base<T,IS_MULTI>
{

public:
    typedef T TStorageTraits;
	typedef typename T::TValue TTValue;
    ////////////////////////////////////////////////////////////////////////////////////
	// Capacity Enum
    ////////////////////////////////////////////////////////////////////////////////////
	static const int CAPACITY = T::CAPACITY;

    ////////////////////////////////////////////////////////////////////////////////////
	// Adds Element Value At Location Key  - O(log n)
    ////////////////////////////////////////////////////////////////////////////////////
	void insert(const TTValue &key)
	{

		assert(!IS_MULTI || find_index(key)==tree_node::NULL_NODE); //fixme handle duplicates more sensibly?

		alloc_key(key);
		tree_base<T, IS_MULTI>::insert_alloced_key();

	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Allocs an item, when filled, call insert_alloced
    ////////////////////////////////////////////////////////////////////////////////////
	TTValue & alloc()
	{
		return tree_base<T, IS_MULTI>::alloc_key();
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Allocs an item (raw), when constucted, call insert_alloced
    ////////////////////////////////////////////////////////////////////////////////////
	TRatlNew *alloc_raw()
	{
		return tree_base<T, IS_MULTI>::alloc_key_raw();
	}
	template<class CAST_TO>
	CAST_TO *verify_alloc(CAST_TO *p) const
	{
		return verify_alloc_key(p);
	}

	void insert_alloced()
	{
		tree_base<T, IS_MULTI>::insert_alloced_key();
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Removes The First Element With Key (key)  - O(log n)
    ////////////////////////////////////////////////////////////////////////////////////
	void		erase(const TTValue &key)
	{
		//fixme this is a double search currently
		int i=find_index(key);
		if (i!=tree_node::NULL_NODE)
		{
			tree_base<T, IS_MULTI>::erase_index(i);
		}
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Iterator
	//
	// A map is sorted in ascending order on the KEY type.  ++ and -- are both
	// O((log n)^2) operations
	//
    ////////////////////////////////////////////////////////////////////////////////////
	class iterator
	{
		friend class set_base<TStorageTraits,IS_MULTI>;
		friend class const_iterator;

		int			mLoc;
		set_base<TStorageTraits,IS_MULTI>*	mOwner;

	public:
		iterator(set_base<TStorageTraits,IS_MULTI> *owner=0, int loc=tree_node::NULL_NODE) :
			mOwner(owner),
			mLoc(loc)
		{
		}
		iterator(const iterator &o) :
			mOwner(o.mOwner),
			mLoc(o.mLoc)
		{
		}

		void operator=(const iterator &o)
		{
			mOwner=o.mOwner;
			mLoc=o.mLoc;
		}

		iterator	operator++()		//prefix
		{
			assert(mOwner);
			mLoc=mOwner->next(mLoc);
			return *this;
		}
		iterator	operator++(int)		//postfix
		{
			assert(mOwner);
			iterator old(*this);
			mLoc=mOwner->next(mLoc);
			return old;
		}

		iterator	operator--()		//prefix
		{
			assert(mOwner);
			mLoc=mOwner->previous(mLoc);
			return *this;
		}
		iterator	operator--(int)		//postfix
		{
			assert(mOwner);
			iterator old(*this);
			mLoc=mOwner->previous(mLoc);
			return old;
		}

		bool	operator!=(const iterator p) const	{return (mLoc!=p.mLoc || mOwner!=p.mOwner);}
		bool	operator==(const iterator p) const 	{return (mLoc==p.mLoc && mOwner==p.mOwner);}

		const TTValue &	operator*() const
		{
			assert(mOwner);
			assert(mLoc>=0&&mLoc<CAPACITY); // deferencing end()?
			return mOwner->index_to_key(mLoc);
		}
		const TTValue *	operator->() const
		{
			assert(mOwner);
			assert(mLoc>=0&&mLoc<CAPACITY); // deferencing end()?
			return &mOwner->index_to_key(mLoc);
		}
	};
	class const_iterator
	{
		friend class set_base<TStorageTraits,IS_MULTI>;

		int		mLoc;

		const set_base<TStorageTraits,IS_MULTI>*		mOwner;

	public:
		const_iterator(const set_base<TStorageTraits,IS_MULTI> *owner=0, int loc=tree_node::NULL_NODE) :
			mOwner(owner),
			mLoc(loc)
		{
		}
		const_iterator(const const_iterator &o) :
			mOwner(o.mOwner),
			mLoc(o.mLoc)
		{
		}
		const_iterator(const iterator &o) :
			mOwner(o.mOwner),
			mLoc(o.mLoc)
		{
		}
		void operator=(const const_iterator &o)
		{
			mOwner=o.mOwner;
			mLoc=o.mLoc;
		}
		void operator=(const iterator &o)
		{
			mOwner=o.mOwner;
			mLoc=o.mLoc;
		}


		const_iterator	operator++()		//prefix
		{
			assert(mOwner);
			mLoc=mOwner->next(mLoc);
			return *this;
		}
		const_iterator	operator++(int)		//postfix
		{
			assert(mOwner);
			const_iterator old(*this);
			mLoc=mOwner->next(mLoc);
			return old;
		}

		const_iterator	operator--()		//prefix
		{
			assert(mOwner);
			mLoc=mOwner->previous(mLoc);
			return *this;
		}
		const_iterator	operator--(int)		//postfix
		{
			assert(mOwner);
			const_iterator old(*this);
			mLoc=mOwner->previous(mLoc);
			return old;
		}

		bool	operator!=(const const_iterator p) const	{return (mLoc!=p.mLoc || mOwner!=p.mOwner);}
		bool	operator==(const const_iterator p) const 	{return (mLoc==p.mLoc && mOwner==p.mOwner);}
		bool	operator!=(const iterator p) const	{return (mLoc!=p.mLoc || mOwner!=p.mOwner);}
		bool	operator==(const iterator p) const 	{return (mLoc==p.mLoc && mOwner==p.mOwner);}

		const TTValue &	operator*() const
		{
			assert(mOwner);
			assert(mLoc>=0&&mLoc<CAPACITY); // deferencing end()?
			return mOwner->index_to_key(mLoc);
		}
		const TTValue *	operator->() const
		{
			assert(mOwner);
			assert(mLoc>=0&&mLoc<CAPACITY); // deferencing end()?
			return &mOwner->index_to_key(mLoc);
		}
	};
	friend class iterator;
	friend class const_iterator;

    ////////////////////////////////////////////////////////////////////////////////////
	// Seach For A Given Key.  Will Return end() if Failed  - O(log n)
    ////////////////////////////////////////////////////////////////////////////////////
	iterator	find(const TTValue &key)
	{
		return iterator(this,find_index(key));
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Get An Iterator To The Smallest Element  - O(log n)
    ////////////////////////////////////////////////////////////////////////////////////
	iterator	begin()
	{
		return iterator(this, tree_base<T, IS_MULTI>::front());
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Get An Iterator To The Largest Element  - O(log n)
    ////////////////////////////////////////////////////////////////////////////////////
	iterator	rbegin()
	{
		return iterator(this, tree_base<T, IS_MULTI>::back());
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// The Invalid Iterator, Use As A Stop Condition In Your For Loops  - O(1)
    ////////////////////////////////////////////////////////////////////////////////////
	iterator	end()
	{
		return iterator(this);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Seach For A Given Key.  Will Return end() if Failed  - O(log n)
    ////////////////////////////////////////////////////////////////////////////////////
	const_iterator	find(const TTValue &key) const
	{
		return const_iterator(this, find_index(key));
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Get An const_iterator To The Smallest Element  - O(log n)
    ////////////////////////////////////////////////////////////////////////////////////
	const_iterator	begin() const
	{
		return const_iterator(this, tree_base<T, IS_MULTI>::front());
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Get An const_iterator To The Largest Element  - O(log n)
    ////////////////////////////////////////////////////////////////////////////////////
	const_iterator	rbegin() const
	{
		return const_iterator(this, tree_base<T, IS_MULTI>::back());
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// The Invalid const_iterator, Use As A Stop Condition In Your For Loops  - O(1)
    ////////////////////////////////////////////////////////////////////////////////////
	const_iterator	end() const
	{
		return const_iterator(this);
	}
    ////////////////////////////////////////////////////////////////////////////////////
	// Removes The Element Pointed To At (it) And Decrements (it)  - O((log n)^2)
    ////////////////////////////////////////////////////////////////////////////////////
	void		erase(const iterator &it)
	{
		assert(it.mOwner==this && it.mLoc>=0&&it.mLoc<CAPACITY);
		erase_index(it.mLoc);
	}


    ////////////////////////////////////////////////////////////////////////////////////
	//
    ////////////////////////////////////////////////////////////////////////////////////
	iterator	lower_bound(const TTValue &key)
	{
		return iterator(this, find_index(key));
	}

    ////////////////////////////////////////////////////////////////////////////////////
	//
    ////////////////////////////////////////////////////////////////////////////////////
	iterator	upper_bound(const TTValue &key)
	{
		// fixme, this don't work
		iterator ubound(this, find_index(key));
		ubound++;
		return ubound;
	}

};


template<class T, int ARG_CAPACITY>
class set_vs : public set_base<storage::value_semantics_node<T,ARG_CAPACITY,tree_node>,0 >
{
public:
	typedef typename storage::value_semantics_node<T,ARG_CAPACITY,tree_node> TStorageTraits;
	typedef typename TStorageTraits::TValue TTValue;
	static const int CAPACITY = ARG_CAPACITY;
	set_vs() {}
};

template<class T, int ARG_CAPACITY>
class set_os : public set_base<storage::object_semantics_node<T,ARG_CAPACITY,tree_node>,0 >
{
public:
	typedef typename storage::object_semantics_node<T,ARG_CAPACITY,tree_node> TStorageTraits;
	typedef typename TStorageTraits::TValue TTValue;
	static const int CAPACITY = ARG_CAPACITY;
	set_os() {}
};

template<class T, int ARG_CAPACITY, int ARG_MAX_CLASS_SIZE>
class set_is : public set_base<storage::virtual_semantics_node<T,ARG_CAPACITY,ARG_MAX_CLASS_SIZE,tree_node>,0 >
{
public:
	typedef typename storage::virtual_semantics_node<T,ARG_CAPACITY,ARG_MAX_CLASS_SIZE,tree_node> TStorageTraits;
	typedef typename TStorageTraits::TValue TTValue;
	static const int CAPACITY = ARG_CAPACITY;
	static const int MAX_CLASS_SIZE	= ARG_MAX_CLASS_SIZE;
	set_is() {}
};


template<class K,class V,int IS_MULTI>
class map_base : public tree_base<K,IS_MULTI>
{
public:
    typedef K TKeyStorageTraits;
	typedef typename K::TValue TKTValue;

    typedef V TValueStorageTraits;
	typedef typename V::TValue TVTValue;
    ////////////////////////////////////////////////////////////////////////////////////
	// Capacity Enum
    ////////////////////////////////////////////////////////////////////////////////////
	static const int CAPACITY = K::CAPACITY;
private:
	array_base<TValueStorageTraits>	mValues;
public:
	map_base()
	{
		compile_assert<K::CAPACITY==V::CAPACITY>();
	}

	void clear()
	{
		tree_base<K,IS_MULTI>::clear();
		mValues.clear();
	}
    ////////////////////////////////////////////////////////////////////////////////////
	// Adds Element Value At Location Key  - O(log n)
    ////////////////////////////////////////////////////////////////////////////////////
	void insert(const TKTValue &key,const TVTValue &value)
	{
		assert(!IS_MULTI || (tree_base<K,IS_MULTI>::find_index(key)==tree_node::NULL_NODE)); //fixme handle duplicates more sensibly?

		tree_base<K,IS_MULTI>::alloc_key(key);
		tree_base<K,IS_MULTI>::insert_alloced_key();
		assert(check_validity());
		mValues.construct(tree_base<K,IS_MULTI>::index_of_alloced_key(),value);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Adds Element Value At Location Key  returns a reference
    ////////////////////////////////////////////////////////////////////////////////////
	TVTValue &insert(const TKTValue &key)
	{

		assert(!IS_MULTI || (tree_base<K,IS_MULTI>::find_index(key)==tree_node::NULL_NODE)); //fixme handle duplicates more sensibly?

		tree_base<K,IS_MULTI>::alloc_key(key);
		tree_base<K,IS_MULTI>::insert_alloced_key();

		int idx=tree_base<K,IS_MULTI>::index_of_alloced_key();
		assert(check_validity());
		mValues.construct(idx);
		return mValues[idx];
	}
    ////////////////////////////////////////////////////////////////////////////////////
	// Adds Element Value At Location Key  returns a rew pointer for placement new
    ////////////////////////////////////////////////////////////////////////////////////
	TRatlNew *insert_raw(const TKTValue &key)
	{

		assert(!IS_MULTI || (tree_base<K,IS_MULTI>::find_index(key)==tree_node::NULL_NODE)); //fixme handle duplicates more sensibly?

		tree_base<K,IS_MULTI>::alloc_key(key);
		tree_base<K,IS_MULTI>::insert_alloced_key();
		assert(check_validity());
		return mValues.alloc_raw(tree_base<K,IS_MULTI>::index_of_alloced_key());
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// After calling alloc_key*, you may call this to alloc the value
    ////////////////////////////////////////////////////////////////////////////////////
	TVTValue &alloc_value()
	{
		mValues.construct(tree_base<K,IS_MULTI>::index_of_alloced_key());
		return mValues[tree_base<K,IS_MULTI>::index_of_alloced_key()];
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// After calling alloc_key*, you may call this to alloc_raw the value
    ////////////////////////////////////////////////////////////////////////////////////
	TRatlNew *alloc_value_raw()
	{
		return mValues.alloc_raw(tree_base<K,IS_MULTI>::index_of_alloced_key());
	}

	template<class CAST_TO>
	CAST_TO *verify_alloc(CAST_TO *p) const
	{
		return mValues.verify_alloc(p);
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Removes The First Element With Key (key)  - O(log n)
    ////////////////////////////////////////////////////////////////////////////////////
	void		erase(const TKTValue &key)
	{
		//fixme this is a double search currently
		int i=tree_base<K,IS_MULTI>::find_index(key);
		if (i!=tree_node::NULL_NODE)
		{
			tree_base<K,IS_MULTI>::erase_index(i);
			mValues.destruct(i);
		}
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Iterator
	//
	// A map is sorted in ascending order on the KEY type.  ++ and -- are both
	// O((log n)^2) operations
	//
    ////////////////////////////////////////////////////////////////////////////////////
	class const_iterator;
	class iterator
	{
		friend class map_base<K,V, IS_MULTI>;
		friend class const_iterator;

		int							mLoc;
		map_base<K,V, IS_MULTI>*	mOwner;

	public:
		iterator(map_base<K,V, IS_MULTI> *owner=0, int loc=tree_node::NULL_NODE) :
			mLoc(loc),
			mOwner(owner)
		{
		}
		iterator(const iterator &o) :
			mLoc(o.mLoc),
			mOwner(o.mOwner)
		{
		}

		void operator=(const iterator &o)
		{
			mOwner=o.mOwner;
			mLoc=o.mLoc;
		}

		iterator	operator++()		//prefix
		{
			assert(mOwner);
			mLoc=mOwner->next(mLoc);
			return *this;
		}
		iterator	operator++(int)		//postfix
		{
			assert(mOwner);
			iterator old(*this);
			mLoc=mOwner->next(mLoc);
			return old;
		}

		iterator	operator--()		//prefix
		{
			assert(mOwner);
			mLoc=mOwner->previous(mLoc);
			return *this;
		}
		iterator	operator--(int)		//postfix
		{
			assert(mOwner);
			iterator old(*this);
			mLoc=mOwner->previous(mLoc);
			return old;
		}

		bool	operator!=(const iterator &p) const	{return (mLoc!=p.mLoc || mOwner!=p.mOwner);}
		bool	operator==(const iterator &p) const 	{return (mLoc==p.mLoc && mOwner==p.mOwner);}

		TVTValue &	operator*() const
		{
			assert(mOwner);
			assert(mLoc>=0&&mLoc<CAPACITY); // deferencing end()?
			return mOwner->mValues[mLoc];
		}
		const TKTValue &	key() const
		{
			assert(mOwner);
			assert(mLoc>=0&&mLoc<CAPACITY); // deferencing end()?
			return mOwner->index_to_key(mLoc);
		}
		TVTValue &	value() const
		{
			assert(mOwner);
			assert(mLoc>=0&&mLoc<CAPACITY); // deferencing end()?
			return mOwner->mValues[mLoc];
		}
		TVTValue *	operator->() const
		{
			assert(mOwner);
			assert(mLoc>=0&&mLoc<CAPACITY); // deferencing end()?
			return &mOwner->mValues[mLoc];
		}
	};
	class const_iterator
	{
		friend class map_base<K,V,IS_MULTI>;

		int									mLoc;
		const map_base<K,V,IS_MULTI>*		mOwner;

	public:
		const_iterator(const map_base<K,V,IS_MULTI> *owner=0, int loc=tree_node::NULL_NODE) :
			mOwner(owner),
			mLoc(loc)
		{
		}
		const_iterator(const const_iterator &o) :
			mOwner(o.mOwner),
			mLoc(o.mLoc)
		{
		}
		const_iterator(const iterator &o) :
			mOwner(o.mOwner),
			mLoc(o.mLoc)
		{
		}
		void operator=(const const_iterator &o)
		{
			mOwner=o.mOwner;
			mLoc=o.mLoc;
		}
		void operator=(const iterator &o)
		{
			mOwner=o.mOwner;
			mLoc=o.mLoc;
		}


		const_iterator	operator++()		//prefix
		{
			assert(mOwner);
			mLoc=mOwner->next(mLoc);
			return *this;
		}
		const_iterator	operator++(int)		//postfix
		{
			assert(mOwner);
			const_iterator old(*this);
			mLoc=mOwner->next(mLoc);
			return old;
		}

		const_iterator	operator--()		//prefix
		{
			assert(mOwner);
			mLoc=mOwner->previous(mLoc);
			return *this;
		}
		const_iterator	operator--(int)		//postfix
		{
			assert(mOwner);
			const_iterator old(*this);
			mLoc=mOwner->previous(mLoc);
			return old;
		}

		bool	operator!=(const const_iterator &p) const	{return (mLoc!=p.mLoc || mOwner!=p.mOwner);}
		bool	operator==(const const_iterator &p) const 	{return (mLoc==p.mLoc && mOwner==p.mOwner);}
		bool	operator!=(const iterator &p) const	{return (mLoc!=p.mLoc || mOwner!=p.mOwner);}
		bool	operator==(const iterator &p) const 	{return (mLoc==p.mLoc && mOwner==p.mOwner);}

		const TVTValue &	operator*() const
		{
			assert(mOwner);
			assert(mLoc>=0&&mLoc<CAPACITY); // deferencing end()?
			return mOwner->mValues[mLoc];
		}
		const TKTValue &	key() const
		{
			assert(mOwner);
			assert(mLoc>=0&&mLoc<CAPACITY); // deferencing end()?
			return mOwner->index_to_key(mLoc);
		}
		const TVTValue &	value() const
		{
			assert(mOwner);
			assert(mLoc>=0&&mLoc<CAPACITY); // deferencing end()?
			return mOwner->mValues[mLoc];
		}
		const TVTValue *	operator->() const
		{
			assert(mOwner);
			assert(mLoc>=0&&mLoc<CAPACITY); // deferencing end()?
			return &mOwner->mValues[mLoc];
		}
	};
	friend class iterator;
	friend class const_iterator;

    ////////////////////////////////////////////////////////////////////////////////////
	// Seach For A Given Key.  Will Return end() if Failed  - O(log n)
    ////////////////////////////////////////////////////////////////////////////////////
	iterator	find(const TKTValue &key)
	{
		return iterator(this,tree_base<K, IS_MULTI>::find_index(key));
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Get An Iterator To The Smallest Element  - O(log n)
    ////////////////////////////////////////////////////////////////////////////////////
	iterator	begin()
	{
		return iterator(this, tree_base<K,IS_MULTI>::front());
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Get An Iterator To The Largest Element  - O(log n)
    ////////////////////////////////////////////////////////////////////////////////////
	iterator	rbegin()
	{
		return iterator(this, tree_base<K,IS_MULTI>::back());
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// The Invalid Iterator, Use As A Stop Condition In Your For Loops  - O(1)
    ////////////////////////////////////////////////////////////////////////////////////
	iterator	end()
	{
		return iterator(this);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Seach For A Given Key.  Will Return end() if Failed  - O(log n)
    ////////////////////////////////////////////////////////////////////////////////////
	const_iterator	find(const TKTValue &key) const
	{
		return const_iterator(this, find_index(key));
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Get An const_iterator To The Smallest Element  - O(log n)
    ////////////////////////////////////////////////////////////////////////////////////
	const_iterator	begin() const
	{
		return const_iterator(this, tree_base<K,IS_MULTI>::front());
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Get An const_iterator To The Largest Element  - O(log n)
    ////////////////////////////////////////////////////////////////////////////////////
	const_iterator	rbegin() const
	{
		return const_iterator(this, tree_base<K,IS_MULTI>::back());
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// The Invalid const_iterator, Use As A Stop Condition In Your For Loops  - O(1)
    ////////////////////////////////////////////////////////////////////////////////////
	const_iterator	end() const
	{
		return const_iterator(this);
	}
    ////////////////////////////////////////////////////////////////////////////////////
	// Removes The Element Pointed To At (it) And Decrements (it)  - O((log n)^2)
    ////////////////////////////////////////////////////////////////////////////////////
	void		erase(const iterator &it)
	{
		assert(it.mOwner==this && it.mLoc>=0&&it.mLoc<CAPACITY);
		erase_index(it.mLoc);
		mValues.destruct(it.mLoc);
	}
private:
	bool check_validity()
	{
#if (0)
		int cnt=0;
		iterator it=begin();
		for (;it!=end();it++)
		{
			cnt++;
		}
//		assert(cnt==size());
		return cnt==size();
#else
		return true;
#endif
	}
};

template<class K,class V, int ARG_CAPACITY>
class map_vs : public map_base<
	storage::value_semantics_node<K,ARG_CAPACITY,tree_node>,
	storage::value_semantics<V,ARG_CAPACITY>,
	0 >
{
public:
	typedef typename storage::value_semantics<V,ARG_CAPACITY> VStorageTraits;
	typedef typename VStorageTraits::TValue TTValue;
	static const int CAPACITY = ARG_CAPACITY;
	map_vs() {}
};

template<class K,class V, int ARG_CAPACITY>
class map_os : public map_base<
	storage::value_semantics_node<K,ARG_CAPACITY,tree_node>,
	storage::object_semantics<V,ARG_CAPACITY>,
	0 >
{
public:
	typedef typename storage::object_semantics<V,ARG_CAPACITY> VStorageTraits;
	typedef typename VStorageTraits::TValue TTValue;
	static const int CAPACITY = ARG_CAPACITY;
	map_os() {}
};

template<class K,class V, int ARG_CAPACITY,int ARG_MAX_CLASS_SIZE>
class map_is : public map_base<
	storage::value_semantics_node<K,ARG_CAPACITY,tree_node>,
	storage::virtual_semantics<V,ARG_CAPACITY,ARG_MAX_CLASS_SIZE>,
	0 >
{
public:
	typedef typename storage::virtual_semantics<V,ARG_CAPACITY,ARG_MAX_CLASS_SIZE> VStorageTraits;
	typedef typename VStorageTraits::TValue TTValue;
	static const int CAPACITY = ARG_CAPACITY;
	static const int MAX_CLASS_SIZE = ARG_MAX_CLASS_SIZE;
	map_is() {}
};

}

#endif
