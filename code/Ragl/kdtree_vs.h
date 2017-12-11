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
// KD Tree
// -------
//
//
//
// NOTES:
//
//
//
////////////////////////////////////////////////////////////////////////////////////////
#if !defined(RATL_KDTREE_VS_INC)
#define RATL_KDTREE_VS_INC


////////////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////////////
#if defined(RA_DEBUG_LINKING)
	#pragma message("...including kdtree_vs.h")
#endif
#if !defined(RAGL_COMMON_INC)
	#include "ragl_common.h"
#endif
namespace ragl
{



////////////////////////////////////////////////////////////////////////////////////////
// The List Class
////////////////////////////////////////////////////////////////////////////////////////
template <class T, int DIMENSION, int SIZE>
class kdtree_vs : public ratl::ratl_base
{
public:
    ////////////////////////////////////////////////////////////////////////////////////
	// Capacity Enum
    ////////////////////////////////////////////////////////////////////////////////////
 	enum
	{
		CAPACITY	= SIZE,
		NULL_NODE	= SIZE+2,		// Invalid Node ID
		TARG_NODE	= SIZE+3		// Used To Mark Nodes Add Location
	};

    ////////////////////////////////////////////////////////////////////////////////////
	// Constructor
    ////////////////////////////////////////////////////////////////////////////////////
	kdtree_vs() : mRoot(NULL_NODE)
	{
	}

	////////////////////////////////////////////////////////////////////////////////////
	// How Many Objects Are In This Tree
    ////////////////////////////////////////////////////////////////////////////////////
	int			size() const
	{
		return (mPool.size());
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Are There Any Objects In This Tree?
    ////////////////////////////////////////////////////////////////////////////////////
	bool		empty() const
	{
		return (mRoot==NULL_NODE);
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
		mRoot = NULL_NODE;
		mPool.clear();
	}


	////////////////////////////////////////////////////////////////////////////////////
	// Add A New Element To The Tree
    ////////////////////////////////////////////////////////////////////////////////////
	void		add(const T& data)
	{
		// CREATE: New
		//--------------------------------------------
		int nNew					= mPool.alloc();
		mPool[nNew].mData			= data;
		mPool[nNew].mLeft			= NULL_NODE;
		mPool[nNew].mRight			= NULL_NODE;

		// LINK: (nNew)->(Parent)
		//--------------------------------------------
		if (mRoot==NULL_NODE)
		{
			mRoot = nNew;
			mPool[nNew].mParent = NULL_NODE;
			return;
		}

		// LINK: (nNew)->(Parent)
		//--------------------------------------------
		mPool[nNew].mParent			= find_index(data, mRoot, 0, true, true);


		// LINK: (Parent)->(nNew)
		//--------------------------------------------
		if (mPool[mPool[nNew].mParent].mLeft==TARG_NODE)
		{
			mPool[mPool[nNew].mParent].mLeft	= nNew;
		}
		else if (mPool[mPool[nNew].mParent].mRight==TARG_NODE)
		{
			mPool[mPool[nNew].mParent].mRight	= nNew;
		}

		// Hey!  It Didn't Mark Any Targets, Which Means We Found An Exact match To This Data
		//------------------------------------------------------------------------------------
		else
		{
			mPool.free(nNew);
		}
	}






	////////////////////////////////////////////////////////////////////////////////////
	// Does (data) Exist In The Tree?
    ////////////////////////////////////////////////////////////////////////////////////
	bool	find(const T& data)
	{
		assert(mRoot!=NULL_NODE); // If You Hit This Assert, You Are Asking For Data On An Empty Tree

		int		node = find_index(data, mRoot, 0, true, true);

		// Exact Find, Or Found Root?
		//----------------------------
		if (mPool[node].mData==data || mPool[node].mParent==NULL_NODE)
		{
			return true;
		}
		return false;
	}





	////////////////////////////////////////////////////////////////////////////////////
	//
    ////////////////////////////////////////////////////////////////////////////////////
	class		range_query
	{
	public:
		range_query()						{}

	public:
		ratl::vector_vs<T, SIZE>		mReported;
		T								mMins;
		T								mMaxs;
	};

	////////////////////////////////////////////////////////////////////////////////////
	//
    ////////////////////////////////////////////////////////////////////////////////////
	void		find(range_query& query)
	{
		if (mRoot!=NULL_NODE)
		{
			query.mReported.clear();
			tree_search(query);
		}
	}







private:
	////////////////////////////////////////////////////////////////////////////////////
	//
    ////////////////////////////////////////////////////////////////////////////////////
	class node
	{
	public:
		int		mParent;
		int		mLeft;
		int		mRight;

		T		mData;
	};

	////////////////////////////////////////////////////////////////////////////////////
	//
    ////////////////////////////////////////////////////////////////////////////////////
	class		range_bounds
	{
	public:
		int		mMins[DIMENSION];
		int		mMaxs[DIMENSION];
	};



	////////////////////////////////////////////////////////////////////////////////////
	// This Private Function Of The Class Does A Standard Binary Tree Search
    ////////////////////////////////////////////////////////////////////////////////////
	int			find_index(const T& data, int curNode, int curDimension, bool returnClosest, bool markTarget)
	{
		// Did We Just Go Off The End Of The Tree Or Find The Data We Were Looking For?
		//------------------------------------------------------------------------------
		if (curNode==NULL_NODE || mPool[curNode].mData==data)
		{
			return curNode;
		}


		// Calculate The Next Dimension For Searching
		//--------------------------------------------
		int	nextDimension = curDimension+1;
		if (nextDimension>=DIMENSION)
		{
			nextDimension = 0;
		}


		// Search Recursivly Down The Tree Either Left (For Data > Current Node), Or Right
		//---------------------------------------------------------------------------------
		int		findRecursive;
		bool	goLeft = (data[curDimension] < mPool[curNode].mData[curDimension]);
		if (goLeft)
		{
			findRecursive = find_index(data, mPool[curNode].mLeft, nextDimension, returnClosest, markTarget);
		}
		else
		{
			findRecursive = find_index(data, mPool[curNode].mRight, nextDimension, returnClosest, markTarget);
		}

		// Success!
		//----------
		if (findRecursive!=NULL_NODE)
		{
			return findRecursive;
		}

		// If We Want To Return The CLOSEST Node, And We Went Off The End, Then Return This One
		//--------------------------------------------------------------------------------------
		if (returnClosest)
		{
			// If We Are Asked To Mark The Target, We Mark (TARG_NODE) At Either mLeft or mRight,
			// Depending On Where The Node Should Have Been
			//----------------------------------------------------------------------------------
			if (markTarget)
			{
				if (goLeft)
				{
					mPool[curNode].mLeft = TARG_NODE;
				}
				else
				{
					mPool[curNode].mRight = TARG_NODE;
				}
			}

			// Go Ahead And Return This Node, It's The One We Would Have Put As The Child
			return curNode;
		}

		// Return The Results Of The Recursive Call
		//------------------------------------------
		return NULL_NODE;
	}



	////////////////////////////////////////////////////////////////////////////////////
	// This function just sets up the range bounds and starts the recursive tree search
    ////////////////////////////////////////////////////////////////////////////////////
	void		tree_search(range_query& query)
	{
		range_bounds	bounds;
		for (int i=0; i<DIMENSION; i++)
		{
			bounds.mMins[i] = 0;
			bounds.mMaxs[i] = 0;
		}
		tree_search(query, mRoot, 0, bounds);
	}

	////////////////////////////////////////////////////////////////////////////////////
	//
    ////////////////////////////////////////////////////////////////////////////////////
	void		tree_search(range_query& query, int curNode, int curDimension, range_bounds bounds)
	{
		assert(curNode<SIZE);

		// Is This Node In The Query Range?  If So, Report It
		//----------------------------------------------------
		if (curNode!=NULL_NODE && tree_search_node_in_range(query, mPool[curNode]))
		{
			query.mReported.push_back(mPool[curNode].mData);
		}

		// If This Is A Leaf Node, We're Done Here
		//-----------------------------------------
		if (curNode==NULL_NODE || (mPool[curNode].mLeft==NULL_NODE && mPool[curNode].mRight==NULL_NODE))
		{
			return;
		}

		// Calculate The Next Dimension For Searching
		//--------------------------------------------
		int	nextDimension = curDimension+1;
		if (nextDimension>=DIMENSION)
		{
			nextDimension = 0;
		}


		// Test To See If Our Subtree Is In Range
		//----------------------------------------
		ESide	Side = tree_search_bounds_in_range(query, bounds);

		// If The Bounds Are Contained Entirely Within The Query Range, We Report The Sub Tree
		//-------------------------------------------------------------------------------------
		if (Side==Side_AllIn)
		{
			tree_search_report_sub_tree(query, curNode);
		}

		// Otherwise, If Our Bounds Intersect The Query Range, We Need To Look Further
		//-----------------------------------------------------------------------------
		else if (Side==Side_In)
		{
			// Test The Left Child
			//---------------------
			if (mPool[curNode].mLeft!=NULL_NODE)
			{
				int	OldMaxs = bounds.mMaxs[curDimension];
				if ( !bounds.mMins[curDimension] || ((mPool[curNode].mData[curDimension]) < (mPool[bounds.mMins[curDimension]].mData[curDimension])) )
				{
					bounds.mMins[curDimension] = curNode;
				}
				tree_search(query, mPool[curNode].mLeft, nextDimension, bounds);
				bounds.mMaxs[curDimension] = OldMaxs;	// Restore Old Maxs For The Right Child Search
			}

			// Test The Right Child
			//----------------------
			if (mPool[curNode].mRight!=NULL_NODE)
			{
				if ( !bounds.mMaxs[curDimension] || ((mPool[bounds.mMaxs[curDimension]].mData[curDimension]) < (mPool[curNode].mData[curDimension])) )
				{
					bounds.mMaxs[curDimension] = curNode;
				}
				tree_search(query, mPool[curNode].mRight, nextDimension, bounds);
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////////
	// This Function Returns True If The Node Is Within The Query Range
    ////////////////////////////////////////////////////////////////////////////////////
	bool		tree_search_node_in_range(range_query& query, node& n)
	{
		for (int dim=0; dim<DIMENSION; dim++)
		{
			if (n.mData[dim]<query.mMins[dim] || query.mMaxs[dim]<n.mData[dim])
			{
				return false;
			}
		}
		return true;
	}

	////////////////////////////////////////////////////////////////////////////////////
	//
    ////////////////////////////////////////////////////////////////////////////////////
	ESide		tree_search_bounds_in_range(range_query& query, range_bounds& bounds)
	{
		ESide	S = Side_AllIn;
		for (int dim=0; dim<DIMENSION; dim++)
		{
			// If Any Of Our Dimensions Are Undefined Right Now, Always Return INTERSECT
			//---------------------------------------------------------------------------
			if (!bounds.mMaxs[dim] || !bounds.mMins[dim])
			{
				return Side_In;
			}

			// Check To See If They Intersect At All?
			//----------------------------------------
			if ((mPool[bounds.mMaxs[dim]].mData[dim]<query.mMins[dim]) ||
				(query.mMaxs[dim]<mPool[bounds.mMins[dim]].mData[dim]))
			{
				return Side_None;
			}

			// Check To See If It Is Contained
			//---------------------------------
			if ((mPool[bounds.mMins[dim]].mData[dim]<query.mMins[dim]) ||
				(query.mMaxs[dim]<mPool[bounds.mMaxs[dim]].mData[dim]))
			{
				S = Side_In;
			}
		}
		return S;
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Add The Cur Node And All Childeren Of The Cur Node
    ////////////////////////////////////////////////////////////////////////////////////
	void		tree_search_report_sub_tree(range_query& query, int curNode)
	{
		assert(curNode<SIZE);

		if (mPool[curNode].mLeft!=NULL_NODE)
		{
			query.mReported.push_back(mPool[mPool[curNode].mLeft].mData);
			tree_search_report_sub_tree(query, mPool[curNode].mRight);
		}
		if (mPool[curNode].mRight!=NULL_NODE)
		{
			query.mReported.push_back(mPool[mPool[curNode].mRight].mData);
			tree_search_report_sub_tree(query, mPool[curNode].mRight);
		}
	}



    ////////////////////////////////////////////////////////////////////////////////////
	// Data
    ////////////////////////////////////////////////////////////////////////////////////
private:
	ratl::handle_pool_vs<node, SIZE>	mPool;				// The Allocation Data Pool
	int									mRoot;				// The Beginning Of The Tree
};

}
#endif