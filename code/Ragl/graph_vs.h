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
// Graph
// -----
// A Graph object is one of the most generic data structures.  Theoretically, a graph
// could be used to make a tree, list, or any other standard structure.  A graph has
// two data types: NODE and EDGE, and maintains connection information how edges can
// link two nodes.
//
// One of the most intuitive uses of a graph class is a navigation bouy system.  In such
// a use, the nodes would probably be vector based objects with positional information
// and the edges might contain portal information (door is open / closed / locked).
//
// Another example might be a web page, or decision tree, or any other problem space
// which requires object connection data.
//
//
//
//
// Implimentation
// --------------
// This template allocates a pool for NODES, a pool for EDGES, and a grid2_vs<int> to serve
// as an Adjacency Matrix (called Links).  The Adj. Matrix stores indicies to EDGE objects
// in the EDGE pool.
//
//
//
//
// What If You Do Not Need Any Edge Objects?
// -----------------------------------------
// It's fairly common to have a graph with no connection information other than the
// existance of the link.  For this case, you should be able to create a graph with a
// MAXEDGES of 1.  You will want to call the version of connect_node() which does not
// take an edge object, and uses 1 as the "index" in the Adj. Matrix.
//
// 
//
//
// How Do You Search?
// -------------------
// This graph supports 3 search methods:
//  Breadth First - Exausts as many links close to start as possible
//  Depth First	  - Gets as far from start as quickly as possible
//  A*            - Uses a distance heuristic toward end point
//
// First, create a (graph_vs::search) object with the start and end points that you want
// to search for.  Then, call either bfs(), dfs(), or astar().  When you get the
// object back, it will have a vector of all the nodes that were visited and methods
// for iterating over that vector to get the path.
//
//	for (TestSearch.path_begin(); !TestSearch.path_end(); TestSearch.path_inc())
//	{
//		sprintf(Buf, "(%d)", TestSearch.path_at());
//		OutputDebugString(Buf);
//	}
//
// 
//
//
// Complexity Analisis
// -------------------
// All data operations except remove_node() are O(1) constant time.
// remove_node() can be O(n) where n is the number of NODES in the graph.
// 
// Search routines:
//  BFS - 
//  DFS - 
//  A*  - 
//
//
//
//
////////////////////////////////////////////////////////////////////////////////////////
#if !defined(RAGL_GRAPH_VS_INC)
#define RAGL_GRAPH_VS_INC


////////////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////////////
#if defined(RA_DEBUG_LINKING)
	#pragma message("...including graph_vs.h")
#endif
#if !defined(RAGL_COMMON_INC)
	#include "ragl_common.h"
#endif
#if !defined(RATL_ARRAY_VS_INC)
	#include "../Ratl/array_vs.h"
#endif
#if !defined(RATL_VECTOR_VS_INC)
	#include "../Ratl/vector_vs.h"
#endif
#if !defined(RATL_BITS_INC)
	#include "../Ratl/bits_vs.h"
#endif
#if !defined(RATL_QUEUE_VS_INC)
	#include "../Ratl/queue_vs.h"
#endif
#if !defined(RATL_STACK_VS_INC)
	#include "../Ratl/stack_vs.h"
#endif
#if !defined(RBTREE_MAP_VS_INC)
	#include "../Ratl/map_vs.h"
#endif
#if !defined(RATL_POOL_VS_INC)
	#include "../Ratl/pool_vs.h"
#endif
#if !defined(RATL_GRID_VS_INC)
	#include "../Ratl/grid_vs.h"
#endif
namespace ragl
{









////////////////////////////////////////////////////////////////////////////////////////
// The Graph Class
////////////////////////////////////////////////////////////////////////////////////////
template <class TNODE, int MAXNODES, class TEDGE, int MAXEDGES, int MAXNODENEIGHBORS>
class graph_vs : public ratl::ratl_base
{
public:
    ////////////////////////////////////////////////////////////////////////////////////
	// The Graph User Class
	//
	// When executing a search (in particular an A* search), you may want to derive your
	// own user class so that you can provide specific functionality to the search.  For
	// example, you might want characters on one team to bias the cost of going to nodes
	// which are occupied by the other team.  Or you might want to allow specific
	// characters to access some edges where others cannot.  Perhaps one can fly or has
	// a special key to a door...
	//
    ////////////////////////////////////////////////////////////////////////////////////
	class user
	{
	public:
		////////////////////////////////////////////////////////////////////////////////////
		// can be invalid edge (For Region)
		////////////////////////////////////////////////////////////////////////////////////
		virtual		bool	can_be_invalid(const TEDGE& Edge) const = 0;
		
		////////////////////////////////////////////////////////////////////////////////////
		// valid edge (For A* and Region)
		////////////////////////////////////////////////////////////////////////////////////
		virtual		bool	is_valid(TEDGE& Edge, int EndPoint=0) const = 0;

		////////////////////////////////////////////////////////////////////////////////////
		// cost estimate from A to B (For A*)
		////////////////////////////////////////////////////////////////////////////////////
		virtual		float	cost(const TNODE& A, const TNODE& B) const = 0;

		////////////////////////////////////////////////////////////////////////////////////
		// cost estimate of Edge (For A*)
		////////////////////////////////////////////////////////////////////////////////////
		virtual		float	cost(const TEDGE& Edge, const TNODE& B) const = 0;

		////////////////////////////////////////////////////////////////////////////////////
		// same floor check (For Triangulation)
		////////////////////////////////////////////////////////////////////////////////////
		virtual		bool	on_same_floor(const TNODE& A, const TNODE& B) const = 0;

		////////////////////////////////////////////////////////////////////////////////////
		// setup the edge (For Triangulation)
		////////////////////////////////////////////////////////////////////////////////////
		virtual		void	setup_edge(TEDGE& Edge, int A, int B, bool OnHull, const TNODE& NodeA, const TNODE& NodeB, bool CanBeInvalid=false) = 0;
	};



    ////////////////////////////////////////////////////////////////////////////////////
	// Capacity Enum
    ////////////////////////////////////////////////////////////////////////////////////
 	enum 
	{
		CAPACITY = MAXNODES,
		NULLEDGE = -1,
	};


	typedef typename ratl::pool_vs<TNODE,		MAXNODES>				TNodes;
	typedef typename ratl::pool_vs<TEDGE,		MAXEDGES>				TEdges;
	struct	SNodeNeighbor
	{
		short	mEdge;
		short	mNode;
	};
	typedef typename ratl::vector_vs<SNodeNeighbor,  MAXNODENEIGHBORS>	TNodeNeighbors;
	typedef typename ratl::array_vs< TNodeNeighbors, MAXNODES>			TLinks;


	typedef	typename ragl::graph_vs<TNODE, MAXNODES, TEDGE, MAXEDGES, MAXNODENEIGHBORS>	TGraph;


    ////////////////////////////////////////////////////////////////////////////////////
	// cells class
    ////////////////////////////////////////////////////////////////////////////////////
	template <int NODESPERCELL, int CELLSX, int CELLSY>
	class cells : public ratl::ratl_base
	{
	public:
	 	enum 
		{
			SIZEX = CELLSX,
			SIZEY = CELLSY,
			SIZENODES = NODESPERCELL
		};


		struct SSortNode
		{
			float	mCost;
			short	mHandle;
			bool			operator<(const SSortNode& t) const	
			{
				return	(mCost<t.mCost);
			}
		};
		typedef	typename ratl::vector_vs<SSortNode, (NODESPERCELL*25)>		TSortNodes;
		typedef	typename ratl::vector_vs<short, NODESPERCELL>				TCellNodes;
		struct SCell
		{
			TCellNodes	mNodes;
			TCellNodes	mEdges;
		};
		typedef	typename ratl::grid2_vs<SCell,	CELLSX, CELLSY>				TCells;


	    ////////////////////////////////////////////////////////////////////////////////
		//
	    ////////////////////////////////////////////////////////////////////////////////
		cells(TGraph& g) : mGraph(g)
		{
		}

		void		clear()
		{
			mCells.clear();
			SCell		EmptyCell;
			mCells.init(EmptyCell);
		}

		void		get_cell_upperleft(int x, int y, float& xReal, float& yReal)
		{
			mCells.get_cell_upperleft(x,y,xReal,yReal);
		}
		void		get_cell_lowerright(int x, int y, float& xReal, float& yReal)
		{
			mCells.get_cell_lowerright(x,y,xReal,yReal);
		}

	    ////////////////////////////////////////////////////////////////////////////////
		//
	    ////////////////////////////////////////////////////////////////////////////////
		void			expand_bounds(int nodeHandle)
		{
			TNODE&	node = mGraph.get_node(nodeHandle);
			mCells.expand_bounds(node[0], node[1]);
		}

	    ////////////////////////////////////////////////////////////////////////////////
		//
	    ////////////////////////////////////////////////////////////////////////////////
		SCell&			get_cell(float x, float y)
		{
			return mCells.get(x,y);
		}
		SCell&			get_cell(int x, int y)
		{
			return mCells.get(x,y);
		}
		void			convert_to_cell_coords(float x, float y, int& xint, int& yint)
		{
			mCells.get_cell_coords(x, y, xint, yint);
		}

	    ////////////////////////////////////////////////////////////////////////////////
		//
	    ////////////////////////////////////////////////////////////////////////////////
		void			fill_cells_nodes(float range)
		{
			// I. Fill All The Cells With The Points Contained By Those Cells
			//----------------------------------------------------------------
			bool full = false;
			for (typename TNodes::iterator it=mGraph.nodes_begin(); it!=mGraph.nodes_end() && !full; it++)
			{
				TNODE&	node = (*it);
				SCell&	cell = mCells.get(node[0], node[1]);

				cell.mNodes.push_back(it.index());
				full = cell.mNodes.full();
				assert(!full || "Cell Filled On Inital Containment"==0);
			}

			mCells.scale_by_largest_axis(range);


			// II. Go To All Neighboring Cells And Get Them
			//==============================================
			int					iRange = (int)(range) + 1;
			typename TCells::riterator	rcell;
			typename TCells::riterator	rcellend;
			CVec3				cellCenter(0,0,0);
			CVec3				nodeCenter(0,0,0);

			TSortNodes			*sortnodes	= new TSortNodes;
			SSortNode			sortnode;

			TCells				*sortcells	= new TCells;
			sortcells->copy_bounds(mCells);



			// For Every Cell
			//----------------
			for (int x=0; x<CELLSX; x++)
			{
				for (int y=0; y<CELLSY; y++)
				{
					// Setup the Sortnodes vector And Range Iterators
					//------------------------------------------------
					sortnodes->clear();

					mCells.get_cell_position(x,y, cellCenter.v[0], cellCenter.v[1]);

					for (rcell = mCells.rangeBegin(iRange,x,y); !rcell.at_end(); rcell++)
					{
						SCell& cell = (*rcell);

						// Add The Nodes To The Sort List
						//--------------------------------
						for (int i=0; i<cell.mNodes.size() && !sortnodes->full(); i++)
						{
							int	nodeHandle = cell.mNodes[i];

							TNODE&	node  = mGraph.get_node(nodeHandle);
							nodeCenter[0] = node[0];
							nodeCenter[1] = node[1];

							sortnode.mHandle	= nodeHandle;
							sortnode.mCost		= cellCenter.Dist2(nodeCenter);
							sortnodes->push_back(sortnode);
						}
					}

					// Sort The Results
					//------------------
					sortnodes->sort();


					// Copy The Sorted Nodes Vector Into The Sorted Cell (Of The Sorted Cell Grid)
					//----------------------------------------------------------------------------
					SCell&	cell = sortcells->get(x,y);
					cell.mNodes.clear();
					for (int i=0; i<sortnodes->size() && i<NODESPERCELL; i++)
					{
						cell.mNodes.push_back((*sortnodes)[i].mHandle);
					}
				}
			}

			// Now Copy The Sorted Results To The Main Cells
			//-----------------------------------------------
			for (int xb=0; xb<CELLSX; xb++)
			{
				for (int yb=0; yb<CELLSY; yb++)
				{
					SCell&	scell = sortcells->get(xb,yb);
					SCell&	mcell = mCells.get(xb,yb);
					mcell.mNodes = scell.mNodes;
				}
			}

			delete sortnodes;
			delete sortcells;
		}


	    ////////////////////////////////////////////////////////////////////////////////
		//
	    ////////////////////////////////////////////////////////////////////////////////
		void			fill_cells_edges(float range)
		{
			// I. Fill All The Cells With The Points Contained By Those Cells
			//----------------------------------------------------------------
			bool full = false;
			for (typename TEdges::iterator eit=mGraph.edges_begin(); eit!=mGraph.edges_end() && !full; eit++)
			{
				TEDGE&	edge = (*eit);
				SCell&	cell = mCells.get(edge[0], edge[1]);

				cell.mEdges.push_back(eit.index());
				full = cell.mEdges.full();
				assert(!full || "Cell Filled On Inital Containment"==0);
			}


			mCells.scale_by_largest_axis(range);


			// II. Go To All Neighboring Cells And Get Them
			//==============================================
			int					iRange = (int)(range) + 1;
			typename TCells::riterator	rcell;
			typename TCells::riterator	rcellend;
			CVec3				cellCenter(0,0,0);
			CVec3				nodeCenter(0,0,0);

			TSortNodes			*sortedges	= new TSortNodes;
			SSortNode			sortnode;

			TCells				*sortcells	= new TCells;
			sortcells->copy_bounds(mCells);


			// For Every Cell
			//----------------
			for (int x=0; x<CELLSX; x++)
			{
				for (int y=0; y<CELLSY; y++)
				{
					// Setup the Sortnodes vector And Range Iterators
					//------------------------------------------------
					sortedges->clear();

					mCells.get_cell_position(x,y, cellCenter.v[0], cellCenter.v[1]);

					for (rcell = mCells.rangeBegin(iRange,x,y); !rcell.at_end(); rcell++)
					{
						SCell& cell = (*rcell);

						// Add The Edges To The Sort List
						//--------------------------------
						for (int e=0; e<cell.mEdges.size() && !sortedges->full(); e++)
						{
							int	edgeHandle = cell.mEdges[e];

							TEDGE&	edge  = mGraph.get_edge(edgeHandle);
							nodeCenter[0] = edge[0];
							nodeCenter[1] = edge[1];

							sortnode.mHandle	= edgeHandle;
							sortnode.mCost		= cellCenter.Dist2(nodeCenter);
							sortedges->push_back(sortnode);
						}
					}

					// Sort The Results
					//------------------
					sortedges->sort();


					// Copy The Sorted Nodes Vector Into The Sorted Cell (Of The Sorted Cell Grid)
					//----------------------------------------------------------------------------
					SCell&	cell = sortcells->get(x,y);
					cell.mEdges.clear();
					for (int j=0; j<sortedges->size() && j<NODESPERCELL; j++)
					{
						cell.mEdges.push_back((*sortedges)[j].mHandle);
					}
				}
			}

			// Now Copy The Sorted Results To The Main Cells
			//-----------------------------------------------
			for (int xb=0; xb<CELLSX; xb++)
			{
				for (int yb=0; yb<CELLSY; yb++)
				{
					SCell&	scell = sortcells->get(xb,yb);
					SCell&	mcell = mCells.get(xb,yb);
					mcell.mEdges = scell.mEdges;
				}
			}

			delete sortedges;
			delete sortcells;
		}


	private:
		TGraph&		mGraph;
		TCells		mCells;
	};






    ////////////////////////////////////////////////////////////////////////////////////
	// Remove All Edges
    ////////////////////////////////////////////////////////////////////////////////////
	void		clear_edges()
	{	
		mEdges.clear();
		mEdges.alloc();		// Alloc a dummy edge at location 0
		for (int i=0; i<MAXNODES; i++)
		{
			mLinks[i].clear();
		}
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Clear Out All Nodes And Edges
    ////////////////////////////////////////////////////////////////////////////////////
	void	clear()
	{
		mNodes.clear();
		mNodes.alloc();		// Alloc A dummy node at location 0
		clear_edges();

#if !defined(FINAL_BUILD)
		mSearchCount = 0;
		mSearchMemorySize = 0;

		mSearchSuccess = 0;
		mSearchSuccessVisited = 0;
		mSearchSuccessPathLen = 0;

		mSearchFail = 0;
		mSearchFailVisited = 0;
#endif
	}


    ////////////////////////////////////////////////////////////////////////////////////
	// Constructor
    ////////////////////////////////////////////////////////////////////////////////////
	graph_vs()
	{
		clear();
	}

	~graph_vs()
	{
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Number Of Nodes
    ////////////////////////////////////////////////////////////////////////////////////
	int			size_nodes()
	{
		return (mNodes.size()-1);
	}


    ////////////////////////////////////////////////////////////////////////////////////
	// Access A Node
    ////////////////////////////////////////////////////////////////////////////////////
	TNODE&		get_node(int i)
	{
		return mNodes[i];
	}


    ////////////////////////////////////////////////////////////////////////////////////
	// Access The Beginning Of The Nodes List
    ////////////////////////////////////////////////////////////////////////////////////
	typename TNodes::iterator	nodes_begin()
	{
		typename TNodes::iterator x = mNodes.begin();
		x++;
		return x;
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Access The End Of The Nodes List
    ////////////////////////////////////////////////////////////////////////////////////
	typename TNodes::iterator	nodes_end()
	{
		return mNodes.end();
	}



    ////////////////////////////////////////////////////////////////////////////////////
	// Number Of Edges
    ////////////////////////////////////////////////////////////////////////////////////
	int			size_edges()
	{
		return (mEdges.size()-1);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Access An Edge
    ////////////////////////////////////////////////////////////////////////////////////
	TEDGE&		get_edge(int i)
	{
		return mEdges[i];
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Get The Edge That Connects nodeA to nodeB
	//
	// NOTE: This is now a linear scan witout the adjacency matrix (worst case 8 depth)
    ////////////////////////////////////////////////////////////////////////////////////
	int			get_edge_across(int nodeA, int nodeB)
	{
		int numNeighbors = mLinks[nodeA].size();
		for (int curNeighbor=0; curNeighbor<numNeighbors; curNeighbor++)
		{
			if (mLinks[nodeA][curNeighbor].mNode==nodeB)
			{
				if (mLinks[nodeA][curNeighbor].mEdge)
				{
					return mLinks[nodeA][curNeighbor].mEdge;
				}
				return -1;	// -1 signifies that a link exists with no edge
			}
		}
		return 0;
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Access The Beginning Of The Nodes List
    ////////////////////////////////////////////////////////////////////////////////////
	typename TEdges::iterator	edges_begin()
	{
		typename TEdges::iterator x = mEdges.begin();
		x++;
		return x;
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Access The End Of The Nodes List
    ////////////////////////////////////////////////////////////////////////////////////
	typename TEdges::iterator	edges_end()
	{
		return mEdges.end();
	}

	int							edge_index(const TEDGE& edge)
	{
		return mEdges.pointer_to_index(&edge);
	}




    ////////////////////////////////////////////////////////////////////////////////////
	// Get All The Neighbors Of A Given Node
    ////////////////////////////////////////////////////////////////////////////////////
	TNodeNeighbors&		get_node_neighbors(const int nodeA)
	{
		return mLinks[nodeA];
	}
	bool						node_has_neighbors(const int nodeA)
	{
		return !(mLinks[nodeA].empty());
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Add A Node
    ////////////////////////////////////////////////////////////////////////////////////
	int			insert_node(const TNODE& t)
	{
		int	nNode = mNodes.alloc();
		mNodes[nNode] = t;
		return nNode;
	}


    ////////////////////////////////////////////////////////////////////////////////////
	// Remove Node And Clear Out All Edges Once Connected To This Node
	//   Note: This is a fairly expensive operation, not to be done often
    ////////////////////////////////////////////////////////////////////////////////////
	void		remove_node(int node)
	{
		mNodes.free(node);

		// For Each Link To A Neighboring Node
		//--------------------------------------
		for (int i=0; i<mLinks[node].size(); i++)
		{
			int	curNeighbor = mLinks[node][i].mNode;
			int curEdge		= mLinks[node][i].mEdge;

			// Free The Edge
			//---------------
			if (curEdge)
			{
				mEdges.free(curEdge);
			}


			// Remove The Edge From Any Recorded Neighbors
			//---------------------------------------------
			TNodeNeighbors& neighbors = mLinks[curNeighbor];
			for (int j=0; j<neighbors.size(); j++)
			{
				if (neighbors[j].mNode==node)
				{
					neighbors.erase_swap(j);
					break;
				}
			}
		}
		mLinks[node].clear();
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Connect Node With An Edge Object  (A->B)  if reflexive, also (B->A)
    ////////////////////////////////////////////////////////////////////////////////////
	int			connect_node(const TEDGE& t, int nodeA, int nodeB, bool reflexive=true)
	{
		if (nodeA==nodeB || !nodeA || !nodeB || !mNodes.is_used(nodeA) || !mNodes.is_used(nodeB))
		{
			assert("ERROR: Cannot Connect A and B!"==0);
			return 0;
		}

		if (mLinks[nodeA].full() || (reflexive && mLinks[nodeB].full()))
		{
			assert("ERROR: Max edges per node exceeded!"==0);
			return 0;
		}

		if (mEdges.full())
		{
			assert("ERROR: Max edges exceeded!"==0);
			return 0;
		}

		SNodeNeighbor	nNbr;

		nNbr.mNode = nodeB;
		nNbr.mEdge = mEdges.alloc();
		mEdges[nNbr.mEdge] = t;


		mLinks[nodeA].push_back(nNbr);
		if (reflexive)
		{
			nNbr.mNode = nodeA;
			mLinks[nodeB].push_back(nNbr);
		}

		return nNbr.mEdge;
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Connect Node Without Allocating An Edge Object  (A->B)  if reflexive, also (B->A)
    ////////////////////////////////////////////////////////////////////////////////////
	void		connect_node(int nodeA, int nodeB, bool reflexive=true)
	{
		if (nodeA==nodeB || !nodeA || !nodeB || !mNodes.is_used(nodeA) || !mNodes.is_used(nodeB))
		{
			assert("ERROR: Cannot Connect A and B!"==0);
			return;
		}

		if (mLinks[nodeA].full() || (reflexive && mLinks[nodeB].full()))
		{
			assert("ERROR: Max edges per node exceeded!"==0);
			return;
		}


		SNodeNeighbor	nNbr;

		nNbr.mNode = nodeB;
		nNbr.mEdge = 0;


		mLinks[nodeA].push_back(nNbr);
		if (reflexive)
		{
			nNbr.mNode = nodeA;
			mLinks[nodeB].push_back(nNbr);
		}

		return;
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Remove Edge
    ////////////////////////////////////////////////////////////////////////////////////
	void		remove_edge(int nodeA, int nodeB, bool reflexive=true)
	{
		if (!mNodes.is_used(nodeA) || (!mNodes.is_used(nodeB) && nodeA==nodeB))
		{
			assert("Unable To Remove Edge"==0);
			return;
		}

		int	linkNum=0;

		for (linkNum=0; linkNum<mLinks[nodeA].size(); linkNum++)
		{
			if (mLinks[nodeA][linkNum].mNode==nodeB)
			{
				if (mLinks[nodeA][linkNum].mEdge && mEdges.is_used(mLinks[nodeA][linkNum].mEdge))
				{
					mEdges.free(mLinks[nodeA][linkNum].mEdge);
				}
				mLinks[nodeA].erase_swap(linkNum);
				break;
			}
		}


		for (linkNum=0; linkNum<mLinks[nodeB].size(); linkNum++)
		{
			if (mLinks[nodeB][linkNum].mNode==nodeA)
			{
				if (mLinks[nodeB][linkNum].mEdge && mEdges.is_used(mLinks[nodeB][linkNum].mEdge))
				{
					mEdges.free(mLinks[nodeB][linkNum].mEdge);
				}
				mLinks[nodeB].erase_swap(linkNum);
				break;
			}
		}
	}


    ////////////////////////////////////////////////////////////////////////////////////
	// Data
    ////////////////////////////////////////////////////////////////////////////////////
private:
	TNodes		mNodes;
	TEdges		mEdges;
	TLinks		mLinks;


	////////////////////////////////////////////////////////////////////////////////////////
	// The Handle Heap Class
	//
	// A handle heap is a specially designed heap class which remembers the handle of any
	// value that is inserted.  Should the value of an already inserted object change, it is
	// possible to reheapify around that value, no matter where the value lies inside the heap
	// because of the additional memory to convert handle->index.
	//
	// The handle heap is used by A* to sort the open list by cost.
	//
	////////////////////////////////////////////////////////////////////////////////////////
	template<class T>
	class handle_heap
	{
	public:

		////////////////////////////////////////////////////////////////////////////////////
		// Constructor
		////////////////////////////////////////////////////////////////////////////////////
		handle_heap(TNodes& Nodes) : mNodes(Nodes)
		{
			clear();
		}
    
		////////////////////////////////////////////////////////////////////////////////////
		// Get The Size (The Difference Between The Push And Pop "Pointers")
		////////////////////////////////////////////////////////////////////////////////////
		int				size() const
		{
			return (mPush);
		}
		
		////////////////////////////////////////////////////////////////////////////////////
		// Check To See If The Size Is Zero
		////////////////////////////////////////////////////////////////////////////////////
		bool			empty() const
		{
			return (!size());
		}

		////////////////////////////////////////////////////////////////////////////////////
		// Check To See If The Size Is Full
		////////////////////////////////////////////////////////////////////////////////////
		bool			full() const
		{
			return (size()==MAXNODES);
		}

		////////////////////////////////////////////////////////////////////////////////////
		// Empty Out The Entire Heap
		////////////////////////////////////////////////////////////////////////////////////
		void			clear()
		{
			mPush = 0;
			for (int i=0; i<MAXNODES; i++)
			{
				mHandleToPos[i] = -1;
			}
		}

		////////////////////////////////////////////////////////////////////////////////////
		// Check If The Handle Has Been Added To This Heap
		////////////////////////////////////////////////////////////////////////////////////
		bool			used(int handle)
		{
			return (mHandleToPos[handle]!=-1 && mData[mHandleToPos[handle]].handle()==handle);
		}

		////////////////////////////////////////////////////////////////////////////////////
		// Get The Data Value At The Top Of The Heap
		////////////////////////////////////////////////////////////////////////////////////
		T&				top()
		{
			assert(mPush>0);		// Don't Try To Look At This If There Is Nothing In Here
			return (mData[0]);
		}

		////////////////////////////////////////////////////////////////////////////////////
		// Accessor
		////////////////////////////////////////////////////////////////////////////////////
		T&			operator[](int handle)											
		{
			// If You Hit This Assert, Then You Are Asking For Unallocated Data
			//------------------------------------------------------------------
			assert(used(handle));
			return mData[mHandleToPos[handle]];
		}


		////////////////////////////////////////////////////////////////////////////////////
		// Add A Value To The Queue
		////////////////////////////////////////////////////////////////////////////////////
		void			push(const T& nValue)
		{
			assert(size()<MAXNODES);

			// Get The Handle From The Value And Make Sure We Don't Already Have One Stored There
			//------------------------------------------------------------------------------------
			assert(mHandleToPos[nValue.handle()] == -1);

			// Add It
			//--------
			mData[mPush]										= nValue;
			mHandleToPos[nValue.handle()]	= mPush;

			

			// Fix Possible Heap Inconsistancies
			//-----------------------------------
			reheapify_upward(mPush);
			
			mPush++;
		}

		////////////////////////////////////////////////////////////////////////////////////
		// Remove A Value From The Queue
		////////////////////////////////////////////////////////////////////////////////////
		void			pop()
		{
			assert(size()>0);

			mPush--;

			assert(mHandleToPos[mData[0].handle()]==0);
			

			// Swap The Lowest Element Up To The Spot We Just "Erased"
			//---------------------------------------------------------
			swap(0, mPush);

			mHandleToPos[mData[mPush].handle()] = -1;	// Erase This Handles Marker


			// Fix Possible Heap Inconsistancies
			//-----------------------------------
			reheapify_downward(0);
		}

		////////////////////////////////////////////////////////////////////////////////////
		// Call This Func If The Value At The Given Handle Has Changed & Needs Adjustment
		////////////////////////////////////////////////////////////////////////////////////
		void			reheapify(int handle)
		{
			assert(used(handle));

			int Pos = mHandleToPos[handle];
			reheapify_upward(Pos);
			reheapify_downward(Pos);
		}

	private:
		////////////////////////////////////////////////////////////////////////////////////
		// Returns The Location Of Node (i)'s Parent Node (The Parent Node Of Zero Is Zero)
		////////////////////////////////////////////////////////////////////////////////////
		int			parent(int i)				
		{
			return ((i-1)/2);
		}

		////////////////////////////////////////////////////////////////////////////////////
		// Returns The Location Of Node (i)'s Left Child (The Child Of A Leaf Is The Leaf)
		////////////////////////////////////////////////////////////////////////////////////
		int			left(int i)	
		{
			return (2*i)+1;
		}

		////////////////////////////////////////////////////////////////////////////////////
		// Returns The Location Of Node (i)'s Right Child (The Child Of A Leaf Is The Leaf)
		////////////////////////////////////////////////////////////////////////////////////
		int			right(int i)
		{
			return (2*i)+2;
		}

		////////////////////////////////////////////////////////////////////////////////////
		// Returns The Location Of Largest Child Of Node (i)
		////////////////////////////////////////////////////////////////////////////////////
		int			largest_child(int i)
		{
			if (left(i)<mPush)
			{
				if (right(i)<mPush)
				{
					return ( (mData[right(i)] < mData[left(i)]) ? (left(i)) : (right(i)) );
				}
				return left(i);	// Node i only has a left child, so by default it is the biggest
			}
			return i;		// Node i is a leaf, so just return it
		}


		////////////////////////////////////////////////////////////////////////////////////
		// Swaps Two Element Locations
		////////////////////////////////////////////////////////////////////////////////////
		void		swap(int a, int b)
		{
			if (a==b)
			{
				return;
			}

			assert(a>=0 && b>=0 && a<MAXNODES && b<MAXNODES);
			assert(mHandleToPos[mData[a].handle()]==a);
			assert(mHandleToPos[mData[b].handle()]==b);

			// Swap Handles
			//--------------
			mHandleToPos[mData[a].handle()] = b;
			mHandleToPos[mData[b].handle()] = a;

			// Swap Data
			//-----------
			mData[MAXNODES]	= mData[a];		// a->TEMP
			mData[a]		= mData[b];		// b->a
			mData[b]		= mData[MAXNODES];	// TEMP->B

			assert(mHandleToPos[mData[a].handle()]==a);
			assert(mHandleToPos[mData[b].handle()]==b);
		}

		////////////////////////////////////////////////////////////////////////////////////
		// Swaps The Data Up The Heap Until It Reaches A Valid Location
		////////////////////////////////////////////////////////////////////////////////////
		void		reheapify_upward(int Pos)
		{
			while (Pos && mData[parent(Pos)]<mData[Pos])
			{
				swap(parent(Pos), Pos);
				Pos = parent(Pos);
			}
		}

		////////////////////////////////////////////////////////////////////////////////////
		// Swaps The Data Down The Heap Until It Reaches A Valid Location
		////////////////////////////////////////////////////////////////////////////////////
		void		reheapify_downward(int Pos)
		{
			int largestChild = largest_child(Pos);
			while (largestChild!=Pos && mData[Pos]<mData[largestChild])
			{
				swap(largestChild, Pos);
				Pos = largestChild;
				largestChild = largest_child(Pos);
			}
		}

		////////////////////////////////////////////////////////////////////////////////////
		// Validate Will Run Through The Heap And Make Sure The Top Element Is Smallest
		////////////////////////////////////////////////////////////////////////////////////
	#ifdef _DEBUG
		void		validate()
		{
			for (int i=1; i<mPush; i++)
			{
				assert(mData[i]<mData[0]);
			}
		}
	#endif



		////////////////////////////////////////////////////////////////////////////////////
		// Data
		////////////////////////////////////////////////////////////////////////////////////
	private:
		ratl::array_vs<T, MAXNODES+1>	mData;				// The Memory (Plus One For Temp Sort)
		ratl::array_vs<int, MAXNODES>	mHandleToPos;		//

		int								mPush;				// Address Of Next Add Location

		TNodes&							mNodes;
	};







	////////////////////////////////////////////////////////////////////////////////////
	// A Search Node
    ////////////////////////////////////////////////////////////////////////////////////
	class	search_node
	{
	public:
		////////////////////////////////////////////////////////////////////////////////
		// Constructors
		////////////////////////////////////////////////////////////////////////////////
		search_node(int Node=-1, int Parent=-1) : 
			mNode(Node), 
			mParentVisit(Parent),
			mCostToGoal(-1),
			mCostFromStart(0)
		{}
		search_node(const search_node& t) : 
			mNode(t.mNode), 
			mParentVisit(t.mParentVisit),
			mCostToGoal(t.mCostToGoal),
			mCostFromStart(t.mCostFromStart)
		{}

		////////////////////////////////////////////////////////////////////////////////
		// Assignment Operator
		////////////////////////////////////////////////////////////////////////////////
		void		operator=(const search_node& t)
		{
			mNode			= (t.mNode); 
			mParentVisit	= (t.mParentVisit);
			mCostToGoal		= (t.mCostToGoal);
			mCostFromStart	= (t.mCostFromStart);	
		}

		////////////////////////////////////////////////////////////////////////////////
		// 
		////////////////////////////////////////////////////////////////////////////////
		int			handle() const
		{
			return mNode;
		}

		////////////////////////////////////////////////////////////////////////////////
		// 
		////////////////////////////////////////////////////////////////////////////////
		float		cost_estimate() const
		{
			return mCostFromStart+mCostToGoal;
		}

		////////////////////////////////////////////////////////////////////////////////
		// 
		////////////////////////////////////////////////////////////////////////////////
		bool		operator<  (const search_node& t) const	
		{
			return (cost_estimate() > t.cost_estimate());
		}

		////////////////////////////////////////////////////////////////////////////////
		// Data
		////////////////////////////////////////////////////////////////////////////////
		int		mNode;					// Which Node Is This (Index To Pool mNodes)
		int		mParentVisit;			// Which Search Node (In Visited)

		float	mCostToGoal;			// How Far From The Start Of The Search Are We?
		float	mCostFromStart;			// How Far From The End Of The Search Are We?
	};



public:
 	typedef		ratl::vector_vs<search_node, MAXNODES>	TVisited;
 	typedef		ratl::array_vs<int, MAXNODES>			TVisitedHandles;
	typedef		ratl::bits_vs<MAXNODES>					TNodeState;

	////////////////////////////////////////////////////////////////////////////////////
	// The Search Data Object
    ////////////////////////////////////////////////////////////////////////////////////
	class	search
	{
	public:
		////////////////////////////////////////////////////////////////////////////////
		// Constructor
		////////////////////////////////////////////////////////////////////////////////
		search(int nodeStart=0, int nodeEnd=0) :
			mStart(nodeStart),
			mEnd(nodeEnd)
		{
			clear(true, false);
		}

		enum
		{
			NULL_NODE			= 0,
			NULL_NODE_INDEX		= -1,
			NULL_VISIT_INDEX	= -1,
			NULL_COST			= -1,
		};


		////////////////////////////////////////////////////////////////////////////////
		// Reset All The Search Parameters
		////////////////////////////////////////////////////////////////////////////////
		void			clear(bool clearNodesPtr=true, bool clearStartAndEnd=true)
		{
			// Reset All Data
			//----------------
			mClosed.clear();
			mVisited.clear();
			mNodeIndexToVisited.fill(NULL_VISIT_INDEX);


			mNext.mNode			= NULL_NODE;
			mNext.mParentVisit	= NULL_VISIT_INDEX;
			mNext.mCostToGoal	= NULL_COST;
			mNext.mCostFromStart= NULL_COST;

			mPrevIndex			= NULL_NODE_INDEX;
			mNextIndex			= NULL_NODE_INDEX;

			mPathVisit			= NULL_VISIT_INDEX;

			if (clearNodesPtr)
			{
				mNodesPtr		= 0;
			}

			// Clear Out The Start And End Handles
			//-------------------------------------
			if (clearStartAndEnd)
			{
				mStart			= NULL_NODE;
				mEnd			= NULL_NODE;
			}

			// Otherwise, We Can Start The Next Index
			//----------------------------------------
			else if (mNodesPtr && mStart!=NULL_NODE && mEnd!=NULL_NODE)
			{
				mNextIndex		= mStart;
			}

		}

		////////////////////////////////////////////////////////////////////////////////
		// Call This Function To Clear Out Everything EXCEPT Start And End Points
		////////////////////////////////////////////////////////////////////////////////
		void			reset()
		{
			clear(false, false);
		}

		////////////////////////////////////////////////////////////////////////////////
		// Mark This Node As Closed
		////////////////////////////////////////////////////////////////////////////////
		void			close(int node)
		{
			mClosed.set_bit(node);
		}
		////////////////////////////////////////////////////////////////////////////////
		// Mark This Node As Closed
		////////////////////////////////////////////////////////////////////////////////
		void			close(const TNodeState& AllNodesToClose)
		{
			mClosed |= AllNodesToClose;
		}


		////////////////////////////////////////////////////////////////////////////////
		// Return True If We Have Found The Node We Were Searching For
		////////////////////////////////////////////////////////////////////////////////
		bool			success()
		{
			if (mEnd && mPrevIndex!=NULL_NODE_INDEX)
			{
				return (mPrevIndex==mEnd);
			}
			return false;
		}

		////////////////////////////////////////////////////////////////////////////////
		// Get An Index To The Beginning Of The Path
		////////////////////////////////////////////////////////////////////////////////
		void			path_begin()
		{
			if (success())
			{
				mPathVisit = (mVisited.size()-1);
			}
			else
			{
				mPathVisit = NULL_VISIT_INDEX;
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		// Check ForThe End Of The Path (Use In A for() Loop)
		////////////////////////////////////////////////////////////////////////////////
		bool			path_end()
		{
			return (mPathVisit==NULL_VISIT_INDEX);
		}

		////////////////////////////////////////////////////////////////////////////////
		// Index Path Inc (Get The Old Path Node's Parent)
		////////////////////////////////////////////////////////////////////////////////
		void			path_inc()
		{
			assert(mPathVisit!=NULL_VISIT_INDEX);
			mPathVisit = mVisited[mPathVisit].mParentVisit;
		}

		////////////////////////////////////////////////////////////////////////////////
		// Get the node handle of the current node
		////////////////////////////////////////////////////////////////////////////////
		int				path_at()
		{
			assert(mPathVisit!=NULL_VISIT_INDEX);
			return mVisited[mPathVisit].mNode;
		}

		////////////////////////////////////////////////////////////////////////////////
		// The Total Cost Of The Path
		////////////////////////////////////////////////////////////////////////////////
		float			path_cost()
		{
			if (success())
			{
				return (mVisited[(mVisited.size()-1)].mCostFromStart);
			}
			return NULL_COST;
		}


		////////////////////////////////////////////////////////////////////////////////
		// How Many Nodes Were Looked At In This Search
		////////////////////////////////////////////////////////////////////////////////
		int				num_visited()
		{
			return mVisited.size();
		}

		////////////////////////////////////////////////////////////////////////////////
		// Get An Iterator To The Beginning Of The Visited Pool
		////////////////////////////////////////////////////////////////////////////////
		typename TVisited::iterator	visited_begin()
		{
			return mVisited.begin();
		}

		////////////////////////////////////////////////////////////////////////////////
		// Get An Iterator To The End Of The Visited Pool
		////////////////////////////////////////////////////////////////////////////////
		typename TVisited::iterator	visited_end()
		{
			return mVisited.end();
		}





	private:
		////////////////////////////////////////////////////////////////////////////////
		// Start At The First NON Closed Node
		////////////////////////////////////////////////////////////////////////////////
		void			setup(TNodes* nodesPtr)
		{
			mNodesPtr			= nodesPtr;
			clear(false, false);
		}


		////////////////////////////////////////////////////////////////////////////////
		// Pretend the next index is open, probably because we found a shorter route 
		// than the first time it was closed, and want it back on the open list
		////////////////////////////////////////////////////////////////////////////////
		void			reopen_next_index()
		{
			assert(mNextIndex!=NULL_NODE_INDEX);

			mNodeIndexToVisited[mNextIndex] = NULL_VISIT_INDEX;
			mClosed.clear_bit(mNextIndex);
		}

		////////////////////////////////////////////////////////////////////////////////
		// The current estimated cost of reaching a give node, if the node was never
		// visited, but IS closed, it returns NULL cost
		////////////////////////////////////////////////////////////////////////////////
		float			visited_cost(int NodeIndex)
		{
			int VisitedIndex = mNodeIndexToVisited[NodeIndex];
			if (VisitedIndex!=NULL_VISIT_INDEX)
			{
				return mVisited[VisitedIndex].cost_estimate();
			}
			return NULL_COST;
		}


		////////////////////////////////////////////////////////////////////////////////
		// Add This Search Node To The Visited List And Keep Track Of It For Later
		////////////////////////////////////////////////////////////////////////////////
		void			visit(search_node& t)
		{
			assert(mNodesPtr!=0);
			mPrevIndex						= t.mNode;

			// Add It To The Visited List, And Mark The Location In The Node Index Array
			//---------------------------------------------------------------------------
			mVisited.push_back(t);
			mNodeIndexToVisited[mPrevIndex] = (mVisited.size()-1);
			mClosed.set_bit(mPrevIndex);

			// Setup Our Next Node To Know It Came From The Last Location In The Visited Vector
			//----------------------------------------------------------------------------------
			mNext.mParentVisit				= (mVisited.size()-1);
		}

		////////////////////////////////////////////////////////////////////////////////
		// Check To See If The Next Index Has Already Been Closed
		////////////////////////////////////////////////////////////////////////////////
		bool			next_index_closed()
		{
			assert(mNextIndex!=NULL_NODE_INDEX);

			return (mClosed.get_bit(mNextIndex));
		}

		////////////////////////////////////////////////////////////////////////////////
		// The Simple "Get Next" Just Converts The Next Index To A Handle In mNext
		////////////////////////////////////////////////////////////////////////////////
		search_node&	get_next()
		{
			assert(mNodesPtr);
			assert(mNextIndex!=NULL_NODE_INDEX);

			mClosed.set_bit(mNextIndex);

			mNext.mNode				= mNextIndex;
			mNext.mCostToGoal		= 0;
			mNext.mCostFromStart	= 0;

			return mNext;
		}
		
		////////////////////////////////////////////////////////////////////////////////
		// This "Get Next" Function Is For A* and Sets Up THe Costs Of The Search Node
		////////////////////////////////////////////////////////////////////////////////
		search_node&	get_next(const user& suser, TEDGE& edge_parent_to_next)
		{
			assert(mNodesPtr);
			assert(mNextIndex!=NULL_NODE_INDEX);

			//NOTE: we do not do a "mClosed.set_bit(mNextIndex);" here because A* only closes nodes that are visited

			mNext.mNode				= mNextIndex;
			mNext.mCostToGoal		= suser.cost((*mNodesPtr)[mNext.mNode], (*mNodesPtr)[mEnd]);
			mNext.mCostFromStart	= suser.cost(edge_parent_to_next, (*mNodesPtr)[mNext.mNode]);
			
			if (mNext.mParentVisit!=NULL_VISIT_INDEX)
			{
				mNext.mCostFromStart += mVisited[mNext.mParentVisit].mCostFromStart;
			}
			return mNext;
		}

		////////////////////////////////////////////////////////////////////////////////
		// Parameters (Start And End Are External Handles)
		////////////////////////////////////////////////////////////////////////////////
	public:
		int							mStart;
		int							mEnd;


		////////////////////////////////////////////////////////////////////////////////
		// Data
		////////////////////////////////////////////////////////////////////////////////
	private:
		TNodes*						mNodesPtr;
		
		int							mPathVisit;
		int							mPrevIndex;
		int							mNextIndex;
		search_node					mNext;

		ratl::bits_vs<MAXNODES>		mClosed;
		TVisited					mVisited;
		TVisitedHandles				mNodeIndexToVisited;

		friend class graph_vs<TNODE, MAXNODES, TEDGE, MAXEDGES, MAXNODENEIGHBORS>;
	};

    ////////////////////////////////////////////////////////////////////////////////////
	// Setup The Search Data
    ////////////////////////////////////////////////////////////////////////////////////
	void		setup_search(search& sdata)
	{
		sdata.setup(&mNodes);
	}







    ////////////////////////////////////////////////////////////////////////////////////
	// A* Search
    ////////////////////////////////////////////////////////////////////////////////////
	void		astar(search& sdata, const user& suser)
	{
		// Make Sure The Nodes We Are Searching For Exist
		//------------------------------------------------
		assert(MAXEDGES>1);
		sdata.setup(&mNodes);

		// Allocate Our Data Structures
		//------------------------------
		handle_heap<search_node>		open(mNodes);
		int								curNeighbor;
		int								curEdge;
		float							curCost;

		// Run Through The Open List
		//---------------------------
		open.push(sdata.get_next());
		while (!open.empty() && !sdata.success())
		{
			sdata.visit(open.top());
			open.pop();

			// Search Through The Non Closed Nodes Edges
			//-------------------------------------------
			TNodeNeighbors&		curNeighbors = get_node_neighbors(sdata.mPrevIndex);
			for (curNeighbor=0; curNeighbor<curNeighbors.size(); curNeighbor++)
			{
				curEdge = curNeighbors[curNeighbor].mEdge;
				if (curEdge==-1 || suser.is_valid(mEdges[curEdge], sdata.mEnd))
				{
					sdata.mNextIndex			= curNeighbors[curNeighbor].mNode;
					search_node& snode			= sdata.get_next(suser, mEdges[curEdge]);
					curCost						= snode.cost_estimate();

					// Is It Already In The Open List?
					//---------------------------------
					if (open.used(snode.mNode))
					{
						if (curCost<(open[snode.mNode]).cost_estimate())
						{
							open[snode.mNode]		= snode;		// Use This As The Node (With New Parent & Cost)
							open.reheapify(snode.mNode);			// Resort the node in the heap
						}
					}

					// Is It Already In The Closed List?
					//-----------------------------------
					else if (sdata.next_index_closed())
					{
						if (curCost < sdata.visited_cost(snode.mNode))
						{
							sdata.reopen_next_index();				// Pull it off the closed list
							open.push(snode);						// Add it to open
						}
					}

					// It Must Be A Whole New Node
					//------------------------------
					else
					{
						open.push(snode);
					}
				}
			}
		}


#if !defined(FINAL_BUILD)
		mSearchCount++;
		mSearchMemorySize += (sizeof(sdata) + sizeof(suser) + sizeof(open));

		if (sdata.success())
		{
			mSearchSuccess++;
			mSearchSuccessVisited += sdata.num_visited();
			for (sdata.path_begin(); !sdata.path_end(); sdata.path_inc())
			{
				mSearchSuccessPathLen ++;
			}
		}
		else
		{
			mSearchFail++;
			mSearchFailVisited += sdata.num_visited();
		}
#endif
	}








    ////////////////////////////////////////////////////////////////////////////////////
	// Breadth First Search (Use Queue Open List)
    ////////////////////////////////////////////////////////////////////////////////////
	void		bfs(search& sdata)
	{
		sdata.setup(&mNodes);

		// Allocate Our Data Structures
		//------------------------------
		ratl::queue_vs<search_node, MAXNODES>	open;

		// Run Through The Open List
		//---------------------------
		open.push(sdata.get_next());
		while (open.size()>0 && !sdata.success())
		{
			sdata.visit(open.top());
			open.pop();


			// Search Through The Non Closed Nodes Edges
			//-------------------------------------------
			for (sdata.mNextIndex=0; sdata.mNextIndex<MAXNODES; sdata.mNextIndex++)
			{
				if (!sdata.next_index_closed())
				{
					if (get_edge_across(sdata.mPrevIndex, sdata.mNextIndex))
					{
						open.push(sdata.get_next());
					}
				}
			}
		}

#if !defined(FINAL_BUILD)
		mSearchCount++;
		mSearchMemorySize += (sizeof(sdata) + sizeof(open));

		if (sdata.success())
		{
			mSearchSuccess++;
			mSearchSuccessVisited += sdata.num_visited();
		}
		else
		{
			mSearchFail++;
			mSearchFailVisited += sdata.num_visited();
		}
#endif
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Depth First Search (Use Stack Open List)
    ////////////////////////////////////////////////////////////////////////////////////
	void		dfs(search& sdata)
	{
		sdata.setup(&mNodes);

		// Allocate Our Data Structures
		//------------------------------
		ratl::stack_vs<search_node, MAXNODES>	open;

		// Run Through The Open List
		//---------------------------
		open.push(sdata.get_next());
		while (open.size()>0 && !sdata.success())
		{
			sdata.visit(open.top());
			open.pop();


			// Search Through The Non Closed Nodes Edges
			//-------------------------------------------
			for (sdata.mNextIndex=0; sdata.mNextIndex<MAXNODES; sdata.mNextIndex++)
			{
				if (!sdata.next_index_closed())
				{
					if (get_edge_across(sdata.mPrevIndex, sdata.mNextIndex))
					{
						open.push(sdata.get_next());
					}
				}
			}
		}

#if !defined(FINAL_BUILD)
		mSearchCount++;
		mSearchMemorySize += (sizeof(sdata) + sizeof(open));

		if (sdata.success())
		{
			mSearchSuccess++;
			mSearchSuccessVisited += sdata.num_visited();
		}
		else
		{
			mSearchFail++;
			mSearchFailVisited += sdata.num_visited();
		}
#endif
	}



#if !defined(FINAL_BUILD)
	int		mSearchCount;
	int		mSearchMemorySize;

	int		mSearchSuccess;
	int		mSearchSuccessVisited;
	int		mSearchSuccessPathLen;

	int		mSearchFail;
	int		mSearchFailVisited;

	void	ProfileSpew()
	{
		ProfilePrint("");
		ProfilePrint("");
		ProfilePrint("--------------------------------------------------------");
		ProfilePrint("RAVEN STANDARD LIBRARY  -  COMPUTATIONAL GEOMETRY MODULE");
		ProfilePrint("               Graph Profile Results                    ");
		ProfilePrint("--------------------------------------------------------");
		ProfilePrint("");
		ProfilePrint("GRAPH SIZE (Bytes): (%d)  (KiloBytes): (%5.3f)  MeggaBytes(%3.3f)", 
			(sizeof(*this)), 
			((float)(sizeof(*this))/1024.0f), 
			((float)(sizeof(*this))/1048576.0f)
			);
		ProfilePrint("GRAPH COUNT: (%d) Nodes  (%d) Edges", mNodes.size(), mEdges.size());
		if (mNodes.size())
		{
			ProfilePrint("GRAPH COUNT: (%f) Edges/Node", (float)mEdges.size()/(float)mNodes.size());
		}
		ProfilePrint("");
		if (mSearchCount)
		{
			ProfilePrint("SEARCH: (%d) Searches  (%f) AveSize", mSearchCount,  ((float)mSearchMemorySize/(float)mSearchCount));
			if (mSearchSuccess)
			{
				ProfilePrint("SEARCH: (%d) Successes  (%f) AveVisited  (%f) AvePathLen", 
					mSearchSuccess,  
					((float)mSearchSuccessVisited/(float)mSearchSuccess),
					((float)mSearchSuccessPathLen/(float)mSearchSuccess)					
					);
			}
			if (mSearchFail)
			{
				ProfilePrint("SEARCH: (%d) Failures  (%f) AveVisited", mSearchFail,  ((float)mSearchFailVisited/(float)mSearchFail));
			}
		}
		ProfilePrint("");
	};
#endif
};

}
#endif
