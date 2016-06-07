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
// Graph Region
// ------------
//
//
////////////////////////////////////////////////////////////////////////////////////////
#if !defined(RATL_GRAPH_REGION_INC)
#define RATL_GRAPH_REGION_INC



////////////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////////////
#if defined(RA_DEBUG_LINKING)
	#pragma message("...including graph_region.h")
#endif
#if !defined(RAGL_COMMON_INC)
	#include "ragl_common.h"
#endif
#if !defined(RAGL_GRAPH_VS_INC)
	#include "graph_vs.h"
#endif

namespace ragl
{





////////////////////////////////////////////////////////////////////////////////////////
// The Graph Region Class
////////////////////////////////////////////////////////////////////////////////////////
template <class TNODE, int MAXNODES, class TEDGE, int MAXEDGES, int NUM_EDGES_PER_NODE,    int MAXREGIONS, int MAXREGIONEDGES>
class graph_region : public ratl::ratl_base
{
public:
    ////////////////////////////////////////////////////////////////////////////////////
	// Capacity Enum
    ////////////////////////////////////////////////////////////////////////////////////
 	enum
	{
		NULL_REGION	= -1,
		NULL_EDGE	= -1,
		CAPACITY	= MAXREGIONS
	};


    ////////////////////////////////////////////////////////////////////////////////////
	// Some Public Type Defines
    ////////////////////////////////////////////////////////////////////////////////////
	typedef		ragl::graph_vs<TNODE, MAXNODES, TEDGE, MAXEDGES, NUM_EDGES_PER_NODE>	TGraph;
	typedef		ratl::vector_vs<int, MAXNODES>						TRegions;
	typedef		ratl::vector_vs<short, MAXREGIONS>					TRegionEdge;	// List Of All Edges Which Connect RegionA<->RegionB
	typedef		ratl::pool_vs<TRegionEdge, MAXREGIONEDGES>			TEdges;			// Pool Of All RegionEdges
	typedef		ratl::grid2_vs<short, MAXREGIONS, MAXREGIONS>		TLinks;			// Graph Of Links From Region To Region, Each Points To A RegionEdge
	typedef		ratl::bits_vs<MAXREGIONS>							TClosed;


    ////////////////////////////////////////////////////////////////////////////////////
	// Constructor
    ////////////////////////////////////////////////////////////////////////////////////
	graph_region(TGraph& Graph)	: mGraph(Graph)
	{
		clear();
	}
	~graph_region()
	{
	}


    ////////////////////////////////////////////////////////////////////////////////////
	// Clear Out All Temp Data So We Can Recalculate Regions
    ////////////////////////////////////////////////////////////////////////////////////
	void	clear()
	{
		mRegions.resize(0, (int)NULL_REGION);
		mRegions.resize(MAXNODES, (int)NULL_REGION);
		mRegionCount = 0;
		mReservedRegionCount = 0;

		mLinks.init(NULL_EDGE);

		for (int i=0; i<MAXREGIONEDGES; i++)
		{
			if (mEdges.is_used(i))
			{
				mEdges[i].resize(0, NULL_EDGE);
			}
		}
		mEdges.clear();
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// How Many Regions Have Been Created
    ////////////////////////////////////////////////////////////////////////////////////
	int		size()
	{
		return mRegionCount;
	}


    ////////////////////////////////////////////////////////////////////////////////////
	// Get The Region For A Given Node
    ////////////////////////////////////////////////////////////////////////////////////
	int		get_node_region(int Node)
	{
		return mRegions[mGraph.node_index(Node)];
	}



    ////////////////////////////////////////////////////////////////////////////////////
	// Call this function to find out if it is at all possible to get from nodeA to
	// nodeB.  If there is no possible connection, or there is one, but the connection
	// is not valid at the current time, this routine will return false.  Use it as
	// a quick cull routine before a search.
	//
	// In order to use this function, you must have an EdgeQuery class (use the default
	// above, or derive your own for more specialized behavior).
    ////////////////////////////////////////////////////////////////////////////////////
	bool	has_valid_edge(int NodeA, int NodeB, const typename TGraph::user& user)
	{
		int	RegionA = mRegions[NodeA];
		int	RegionB = mRegions[NodeB];

		if (RegionA==RegionB)
		{
			return true;
		}

		mClosed.clear();

		return has_valid_region_edge(RegionA, RegionB, user);
	}


    ////////////////////////////////////////////////////////////////////////////////////
	// Reserve Region
	//
	// Allows a user to pre-allocate a special region for a group of points
    ////////////////////////////////////////////////////////////////////////////////////
	int		reserve()
	{
		assert(mRegionCount < (MAXREGIONS-1));
		if (mRegionCount >= (MAXREGIONS-1) )
		{//stop adding points, we're full, you MUST increase MAXREGIONS for this to work
			return NULL_REGION;
		}
		mReservedRegionCount ++;
		mRegionCount ++;
		return (mRegionCount);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// assign_region
	//
	// Allows a user to pre-allocate a special region for a group of points
    ////////////////////////////////////////////////////////////////////////////////////
	void	assign_region(int NodeIndex, int RegionIndex)
	{
		mRegions[NodeIndex] = RegionIndex;
	}






    ////////////////////////////////////////////////////////////////////////////////////
	// Define Regions
	//
	// Scan through all the nodes (calling the depth first recursive traversal below),
	// and mark regions of nodes which can traverse to one another without needing to check
	// for valid edges.
	//
    ////////////////////////////////////////////////////////////////////////////////////
	bool	find_regions(const typename TGraph::user& user)
	{
		int		CurNodeIndex;
		for (typename TGraph::TNodes::iterator i=mGraph.nodes_begin(); i!=mGraph.nodes_end(); i++)
		{
			CurNodeIndex = i.index();
			if (mRegions[CurNodeIndex] == NULL_REGION)
			{
				assert(mRegionCount < (MAXREGIONS-1));
				if (mRegionCount >= (MAXREGIONS-1) )
				{//stop adding points, we're full, you MUST increase MAXREGIONS for this to work
					return false;
				}
				mRegionCount ++;				// Allocate The New Region
				assign(CurNodeIndex, user);		// Assign All Points To It
			}
		}
		mRegionCount ++;		// Size is actually 1 greater than the number of regions
		return true;
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Search For All Possible Edges Which Connect Regions
	//
	// Once called, this class will have reference data for how to get from one region
	// to another.
    ////////////////////////////////////////////////////////////////////////////////////
	bool	find_region_edges()
	{
		int		RegionA;
		int		RegionB;
		int		RegionLink;
		bool	Success = true;
		bool	ReservedRegionLink;

		for (int indexA=0; indexA<MAXNODES; indexA++)
		{
			RegionA = mRegions[indexA];
			if (RegionA!=NULL_REGION)
			{
				for (int indexB=0; indexB<MAXNODES; indexB++)
				{
					RegionB				= mRegions[indexB];
					ReservedRegionLink	= (RegionA<=mReservedRegionCount || RegionB<=mReservedRegionCount);
					if (RegionB!=NULL_REGION && RegionB!=RegionA && mGraph.get_edge_across(indexA, indexB))
					{
						RegionLink = mLinks.get(RegionA, RegionB);

						// Do We Need To Allocate A New Region Link Vector?
						//--------------------------------------------------
						if (RegionLink==-1)
						{
							if (ReservedRegionLink)
							{
								mLinks.get(RegionA, RegionB) = -2;		// Special Flag For Reserved Regions - they have no edges
								mLinks.get(RegionB, RegionA) = -2;
							}
							else
							{
								if (mEdges.full())
								{
									assert("graph_region: Too Many Region Edges"==0);
									Success = false;
								}
								else
								{
									RegionLink = mEdges.alloc();
									mEdges[RegionLink].resize(0, NULL_EDGE);
									mEdges[RegionLink].push_back(mGraph.get_edge_across(indexA, indexB));

									mLinks.get(RegionA, RegionB) = RegionLink;
									mLinks.get(RegionB, RegionA) = RegionLink;
								}
							}
						}


						// Add This Edge To The Other Region Links
						//-----------------------------------------
						else if (!ReservedRegionLink)
						{
							mEdges[RegionLink].push_back(mGraph.get_edge_across(indexA, indexB));
						}
					}
				}
			}
		}
		return Success;
	}

private:
    ////////////////////////////////////////////////////////////////////////////////////
	// This Routine Is A Depth First Recursive Traversal
	//
	// It will visit all neighbors for each node which have not already been visited
	// and assigned to a region.  Neighbors must always be valid.
    ////////////////////////////////////////////////////////////////////////////////////
	void	assign(int Node, const typename TGraph::user& user)
	{
		mRegions[Node] = mRegionCount;
		for (int i=0; i<MAXNODES; i++)
		{
			if (mRegions[i]==-1)
			{
				int edgeNum = mGraph.get_edge_across(Node, i);
				if (edgeNum && !user.can_be_invalid(mGraph.get_edge(edgeNum)))
				{
					assign(i, user);
				}
			}
		}
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// This Routine Is A Depth First Recursive Search For Target Region
	//
	// Visited regions are makred on the "closed" bit field.
    ////////////////////////////////////////////////////////////////////////////////////
	bool	has_valid_region_edge(int CurRegion, int TargetRegion, const typename TGraph::user& user)
	{
		// Mark The Cur Region As Visited, So We Don't Try To Return To It
		//-----------------------------------------------------------------
		mClosed.set_bit(CurRegion);

		// If The Two Nodes Are In The Same Region, Then This Is Valid
		//-------------------------------------------------------------
		if (CurRegion==TargetRegion)
		{
			return true;
		}

		// Scan Through The Cur Region's Neighbors With Currently Valid Region Edges
		//---------------------------------------------------------------------------
		int	CurRegionEdge;
		for (int NextRegion=0; NextRegion<mRegionCount; NextRegion++)
		{
			// Check If The Link Exists And We Have Not Already Visited The Next Region
			//--------------------------------------------------------------------------
			CurRegionEdge = mLinks.get(CurRegion, NextRegion);
			if (CurRegionEdge!=NULL_EDGE && !mClosed.get_bit(NextRegion))
			{
				if (CurRegion<=mReservedRegionCount)
				{
					// Great, So We Have Found A Valid Neighboring Region, Search There
					//------------------------------------------------------------------
					if (has_valid_region_edge(NextRegion, TargetRegion, user))
					{
						return true;		// HEY!  Somehow, Going To Next Region Got Us To The Target Region!
					}
				}
				else
				{
					// Scan Through This Region Edge List Of Graph Edges For Any Valid One
					//---------------------------------------------------------------------
					assert(mEdges[CurRegionEdge].size()>0);
					for (int j=0; j<mEdges[CurRegionEdge].size(); j++)
					{
						if (user.is_valid(
										mGraph.get_edge(mEdges[CurRegionEdge][j]),
										(NextRegion==TargetRegion)?(-1):(0)
										)
							)
						{
							// Great, So We Have Found A Valid Neighboring Region, Search There
							//------------------------------------------------------------------
							if (has_valid_region_edge(NextRegion, TargetRegion, user))
							{
								return true;		// HEY!  Somehow, Going To Next Region Got Us To The Target Region!
							}

							// Ok, The Target Region Turned Out To Be A Dead End, We Can Stop Trying To Get There
							//------------------------------------------------------------------------------------
							break;
						}
					}
				}
			}
		}

		// Nope, We Failed To Find Any Valid Region Edges Which Lead To The Target Region
		//--------------------------------------------------------------------------------
		return false;
	}


private:
	////////////////////////////////////////////////////////////////////////////////////
	//
	////////////////////////////////////////////////////////////////////////////////////
	TGraph&			mGraph;

	TRegions		mRegions;
	int				mRegionCount;
	int				mReservedRegionCount;

	TLinks			mLinks;
	TEdges			mEdges;
	TClosed			mClosed;





public:
#if !defined(FINAL_BUILD)
	void	ProfileSpew()
	{
		ProfilePrint("");
		ProfilePrint("");
		ProfilePrint("--------------------------------------------------------");
		ProfilePrint("RAVEN STANDARD LIBRARY  -  COMPUTATIONAL GEOMETRY MODULE");
		ProfilePrint("              Region Profile Results                    ");
		ProfilePrint("--------------------------------------------------------");
		ProfilePrint("");
		ProfilePrint("REGION SIZE (Bytes): (%d)  (KiloBytes): (%5.3f)", sizeof(*this), ((float)(sizeof(*this))/1024.0f));
		ProfilePrint("REGION COUNT: (%d) Regions  (%d) Edges", mRegionCount, mEdges.size());
		if (mRegionCount)
		{
			int RegionEdges = 0;
			for (typename TEdges::iterator it=mEdges.begin(); it!=mEdges.end(); ++it)
			{
				RegionEdges += (*it).size();
			}
			ProfilePrint("REGION COUNT: (%f) Ave Edges Size", (float)RegionEdges / (float)mRegionCount);
		}
		ProfilePrint("");
	};
#endif

};


}



#endif