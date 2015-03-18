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
// Graph Triangulate
// -----------------
// Triangulation is the process of generating graph edges between "nearby" points.
//
// This class is designed to work with the ragl_graph template class, and requires that
// the same template parameters for that class be used here.  The memory requirements
// of this class are not inconsequential, so it is best to allocate this class during
// a preprocess step and then throw it away.
//
// NOTE: This is a 2D triangulation!  All Z Coordinates are ignored!
//
//
//
//
// How Do I Triangulate A Raw Set Of Points?
// -----------------------------------------
// First of all, in order to construct a triangulation, you need to have your graph and
// pass it in to the constructor:
//
//   typedef	ragl::graph_triangulate<TNODE, MAXNODES, TEDGE, MAXEDGES>	TTriangulation
//   TTriangulation		MyTriangulation(mMyGraph);
//
// Next, you are free to call any of the public functions in any order, but the best use
// is to call them in this order:
//
//   MyTriangulation.insertion_hull();
//   MyTriangulation.delaunay_edge_flip();
//   MyTriangulation.alpha_shape(MyGraphUser, <MIN>, <MAX>);
//
// For documentation on the above functions, look at their def below.  Also, the doc on
// the Graph User class is in graph_vs.h
//
//
// Finally, when you are ready, call the finish() function.  That will populate your
// graph (which has not been altered in any way up until now).  After calling finish()
// you can dump the triangulation class, as it has done it's job and all the data is
// now stored in the class.
//
//   MyTriangulation.finish();
//
//
//
//
// How Does It Work?  (Overview)
// -----------------------------
// The details of how each step works are outlined below, however, here is the general
// idea:
//
// - Call insertion hull to generate a "rough and dirty" triangulation of the point set.
//   The algorithm is relativly fast, and as a handy bi-product, generates the convex
//   hull of the points.  The resulting mesh is ugly though.  You probably won't want
//   to use it in the rough state.  The basic idea of this algorithm is to iterativly
//   add points which have been presorted along the x-axis into the triangulation.  It
//   is easy to do so, because you always know it will be on the right side of any edge
//   it needs to connect with.
//
// - Now that you have a functional triangulation with edges and faces, there is fairly
//   simple and fast algorithm to "clean it up" called EdgeFlipping.  The idea is simple.
//   Just scan through the edges, if you find one that is "bad", flip it!  Continue until
//   you find no "bad" edges.  NOTE: This algorithm can lock up if any four points are
//   colinear!
//
// - Finally, Alpha Shape is just a simple prune scan of the edges for anything that is
//   too big or too small.  This step is totally optional.
//
//
////////////////////////////////////////////////////////////////////////////////////////
#if !defined(RATL_GRAPH_TRIANGULATE_INC)
#define RATL_GRAPH_TRIANGULATE_INC


////////////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////////////
#if defined(RA_DEBUG_LINKING)
	#pragma message("...including graph_triangulate.h")
#endif
#if !defined(RAGL_COMMON_INC)
	#include "ragl_common.h"
#endif
#if !defined(RAGL_GRAPH_VS_INC)
	#include "graph_vs.h"
#endif
#if !defined(RATL_LIST_VS_INC)
	#include "..\Ratl\list_vs.h"
#endif
namespace ragl
{






////////////////////////////////////////////////////////////////////////////////////////
// The Graph Class
////////////////////////////////////////////////////////////////////////////////////////
template <class TNODE, int MAXNODES, class TEDGE, int MAXEDGES, int MAXNODENEIGHBORS>
class graph_triangulate : public ratl::ratl_base
{
public:
    ////////////////////////////////////////////////////////////////////////////////////
	// Capacity Enum
    ////////////////////////////////////////////////////////////////////////////////////
 	enum 
	{
		CAPACITY = MAXNODES,
		MAXFACES = MAXEDGES*2,
		NULLEDGE = -1
	};

	typedef		graph_vs<TNODE, MAXNODES, TEDGE, MAXEDGES, MAXNODENEIGHBORS>			TGraph;


public:
    ////////////////////////////////////////////////////////////////////////////////////
	// Constructor
    ////////////////////////////////////////////////////////////////////////////////////
	graph_triangulate(TGraph& Graph)	: mGraph(Graph), mHull(), mHullIter(mHull.begin())
	{
		mLinks.init(0);
	}


    ////////////////////////////////////////////////////////////////////////////////////
	// Clear Out All Temp Data So We Can Triangulate Again
    ////////////////////////////////////////////////////////////////////////////////////
	void	clear()
	{
		mLinks.init(0);
		mEdges.clear();
		mFaces.clear();

		mHull.clear();
		mHullIter = mHull.begin();

		mSortNodes.clear();
	}


    ////////////////////////////////////////////////////////////////////////////////////
	// Insertion Hull
	//
	// This is a "quick and dirty" triangulation technique.  It does not give you a very
	// nice looking or terribly useful mesh, but it is a good place to start.  Once
	// you have an insertion hull triangulation, you can call delauny_edge_flip() to 
	// clean it up some.
	//
	// This algorithm's complexity isbounded in the worst case where all the points in 
	// the mesh are on the "hull", in which case it is O(n^2).  However the number of
	// points on the hull for most common point clouds is more likely to be log n.
	//
    ////////////////////////////////////////////////////////////////////////////////////
	void	insertion_hull()
	{
		assert(mGraph.size_nodes()>3);		// We Need More Than 3 Points To Triangulate

		// STEP ONE: Sort all points along the x axis in increasing order
		//----------------------------------------------------------------
		// COMPLEXITY: O(n log n)    Heapsort

		sort_points();



		// STEP TWO: Manually constructe the first face of the triangulation out of the 3 rightmost points
		//--------------------------------------------------------------------------------------------------
		// COMPLEXITY: O(1)

		add_face(mSortNodes[0].mNodeHandle, mSortNodes[1].mNodeHandle, mSortNodes[2].mNodeHandle);



		// STEP THREE: Add each remaining point to the hull, constructing new faces as we go
		//-----------------------------------------------------------------------------------
		// COMPLEXITY: O(n*c)  (n = num nodes,   c = num nodes on hull)

		for (int i=3; i<mSortNodes.size(); i++)
		{
			insert_point(mSortNodes[i].mNodeHandle);
		}
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Delaunay Edge Flipping
	//
	// This algorithm iterativly rotates edges which do not fit the "delaunay" criterion
	// of all points on two adjacent faces containment within the circumscribed circles
	// of the two faces.  It solves the all pairs nearest neighbors problem.
	//
	// The routine is sadly bounded by n^2 complexity, but in practice perfromes very
	// well - much better than n^2 (closer to n log n).
	//
    ////////////////////////////////////////////////////////////////////////////////////
	void	delaunay_edge_flip()
	{
		int	CurFlipped;
		int	TotalFlipped = 0;

		do
		{
			CurFlipped = flip();
			TotalFlipped += CurFlipped;
		}
		while (CurFlipped!=0 && TotalFlipped<10000 /*Sanity Condition*/);
	}


    ////////////////////////////////////////////////////////////////////////////////////
	// This function attempts to prune out edges which connect across "floors" and
	//
	//
	//
	////////////////////////////////////////////////////////////////////////////////////
	void	floor_shape(typename TGraph::user& user, float maxzdelta)
	{
		ratl::vector_vs<int, MAXEDGES>		CullEdges;
		int		nEdge;
		TEdges::iterator stop=mEdges.end();
		for (TEdges::iterator it=mEdges.begin(); it!=mEdges.end(); it++)
		{
			if (!(*it).mOnHull)
			{
				edge&	EdgeAt	= *it;
				face&	FaceR	= mFaces[EdgeAt.mRight];
				face&	FaceL	= mFaces[EdgeAt.mLeft];

//				int		Edge	= mEdges.index_to_handle(it.index());
				int		R		= FaceR.opposing_node(EdgeAt.mA, EdgeAt.mB);
				int		L		= FaceL.opposing_node(EdgeAt.mA, EdgeAt.mB);
				int		RInd	= mGraph.node_index(R);
				int		LInd	= mGraph.node_index(L);

				TNODE&	PtA		= mGraph.get_node(EdgeAt.mA);
				TNODE&	PtB		= mGraph.get_node(EdgeAt.mB);
				TNODE&	PtR		= mGraph.get_node(R);
				TNODE&	PtL		= mGraph.get_node(L);


				if (
					(user.on_same_floor(PtR, PtL)) &&
					(mLinks.get(RInd, LInd)==0) &&
					(mLinks.get(LInd, RInd)==0) &&
					(!user.on_same_floor(PtL, PtA) || !user.on_same_floor(PtL, PtB))
				   )
				{
					nEdge= mEdges.alloc();

					mEdges[nEdge].mA		= R;
					mEdges[nEdge].mB		= L;
					mEdges[nEdge].mHullLoc	= mHullIter;
					mEdges[nEdge].mOnHull	= true;
					mEdges[nEdge].mFlips	= 0;
					mEdges[nEdge].mLeft		= 0;
					mEdges[nEdge].mRight	= 0;


					mLinks.get(RInd, LInd) = nEdge;
					mLinks.get(LInd, RInd) = nEdge;
				}

				if (!user.on_same_floor(PtA, PtB))
				{
					mLinks.get(mGraph.node_index(EdgeAt.mA), mGraph.node_index(EdgeAt.mB)) = 0;
					mLinks.get(mGraph.node_index(EdgeAt.mB), mGraph.node_index(EdgeAt.mA)) = 0;
					
					CullEdges.push_back(it.index());
				}
			}
		}

		for (int i=0; i<CullEdges.size(); i++)
		{
			mEdges.free_index(CullEdges[i]);
		}
	}



    ////////////////////////////////////////////////////////////////////////////////////
	// This function is a simple routine to prune out any edges which are larger or
	// smaller than the desired range (min, max).
    ////////////////////////////////////////////////////////////////////////////////////
	void	alpha_shape(typename TGraph::user& user, float max, float min=0)
	{
		ratl::vector_vs<int, MAXEDGES>		CullEdges;
		float cost;
		for (TEdges::iterator it=mEdges.begin(); it!=mEdges.end(); it++)
		{
			cost = user.cost(mGraph.get_node((*it).mA), mGraph.get_node((*it).mB));
			if (cost<min || cost>max)
			{
				mLinks.get(mGraph.node_index((*it).mA), mGraph.node_index((*it).mB)) = 0;
				mLinks.get(mGraph.node_index((*it).mB), mGraph.node_index((*it).mA)) = 0;
				
				CullEdges.push_back(it.index());
			}
		}

		for (int i=0; i<CullEdges.size(); i++)
		{
			mEdges.free_index(CullEdges[i]);
		}
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Call this function when you are done with the triangulation and want to copy all
	// the temp data into your graph.
    ////////////////////////////////////////////////////////////////////////////////////
	void	finish(typename TGraph::user& user)
	{
		mGraph.clear_edges();
		TEDGE	DefaultEdge;
		for (TEdges::iterator it=mEdges.begin(); it!=mEdges.end(); it++)
		{
			user.setup_edge(DefaultEdge, (*it).mA, (*it).mB, (*it).mOnHull, mGraph.get_node((*it).mA), mGraph.get_node((*it).mB));
			mGraph.connect_node(DefaultEdge, (*it).mA, (*it).mB);
		}
	}





private:
	////////////////////////////////////////////////////////////////////////////////////
	//
	////////////////////////////////////////////////////////////////////////////////////
	typedef		typename ratl::list_vs<int, MAXNODES>						THull;
	typedef		typename ratl::list_vs<int, MAXNODES>::iterator				THullIter;
	typedef		typename ratl::grid2_vs<int,	MAXNODES, MAXNODES>				TLinks;

    
	////////////////////////////////////////////////////////////////////////////////////
	// The Local Edge Class
	//
	//       RIGHT    
	//   B<-<-<-<-<-<-A 
	//       LEFT    
	//
    ////////////////////////////////////////////////////////////////////////////////////
	class	edge
	{
	public:
		int			mA;
		int			mB;

		int			mLeft;
		int			mRight;
		int			mFlips;

		THullIter	mHullLoc;
		bool		mOnHull;

		void		flip_face(int OldFace, int NewFace)
		{
			assert(mRight!=mLeft);
			assert(mLeft!=NewFace && mRight!=NewFace);
			if (mLeft==OldFace)
			{
				mLeft=NewFace;
			}
			else
			{
				assert(mRight==OldFace);
				mRight = NewFace;
			}
			assert(mRight!=mLeft);
		}
		void	verify(int PtA, int PtB, int Edge)
		{
			assert(PtA==mA || PtA==mB);
			assert(PtB==mA || PtB==mB);
			assert(mRight==Edge || mLeft==Edge);
			assert(mRight!=mLeft);
			assert(mA!=mB);
		}
		void	verify(int PtA, int PtB, int PtC, int Edge)
		{
			assert((PtC==mA && (PtA==mB || PtB==mB)) || (PtC==mB && (PtA==mA || PtB==mA)));

			assert(mRight==Edge || mLeft==Edge);
			assert(mRight!=mLeft);
			assert(mA!=mB);
		}
	};

    ////////////////////////////////////////////////////////////////////////////////////
	// The Local Face Class
	// 
	//       _ C
	//       /|  \
	//  LEFT/     \RIGHT
	//     /       \  
	//    B-<-<-<-<-A 
	//       BOTTOM    
	//  
    ////////////////////////////////////////////////////////////////////////////////////
	class	face
	{
	public:
		int			mA;
		int			mB;
		int			mC;

		int			mLeft;
		int			mRight;
		int			mBottom;

		int			mFlips;

		int&		opposing_node(int A, int B)
		{
			if (mA!=A && mA!=B)
			{
				return mA;
			}
			if (mB!=A && mB!=B)
			{
				return mB;
			}
			assert(mC!=A && mC!=B);
			return mC;
		}

		int&		relative_left(int edge)
		{
			if (edge==mLeft)
			{
				return mRight;
			}
			if (edge==mRight)
			{
				return mBottom;
			}
			assert(edge==mBottom);	// If you hit this assert, then the edge is not in this face
			return mLeft;
		}
		int&		relative_right(int edge)
		{
			if (edge==mLeft)
			{
				return mBottom;
			}
			if (edge==mRight)
			{
				return mLeft;
			}
			assert(edge==mBottom);	// If you hit this assert, then the edge is not in this face
			return mRight;
		}
	};

    ////////////////////////////////////////////////////////////////////////////////////
	// The Sort Node Class
	//
	// Used To Sort Nodes In Increasing X Order
    ////////////////////////////////////////////////////////////////////////////////////
	class	sort_node
	{
	public:
		bool			operator<(const sort_node& r) const
		{
			return ((*r.mNodePointer)[0] < (*mNodePointer)[0]);
		}

		int		mNodeHandle;
		TNODE*	mNodePointer;
	};


	////////////////////////////////////////////////////////////////////////////////////
	//
	////////////////////////////////////////////////////////////////////////////////////
	typedef		typename ratl::handle_pool_vs<edge,	MAXEDGES>				TEdges;
	typedef		typename ratl::handle_pool_vs<edge,	MAXEDGES>::iterator		TEdgesIter;
	typedef		typename ratl::handle_pool_vs<face,	MAXFACES>				TFaces;
	typedef		typename ratl::vector_vs<sort_node, MAXNODES>				TSortNodes;


	TGraph&		mGraph;			// A Reference To The Graph Points To Triangulate

	TLinks		mLinks;
	TEdges		mEdges;
	TFaces		mFaces;

	THull		mHull;			// The Convex Hull
	THullIter	mHullIter;

	TSortNodes	mSortNodes;		// Need To Presort Nodes On (x-Axis) For Insertion Hull
	sort_node	mSortNode;




private:
    ////////////////////////////////////////////////////////////////////////////////////
	// Copy All The Graph Nodes To Our Sort Node Class And Run Heap Sort
    ////////////////////////////////////////////////////////////////////////////////////
	void	sort_points()
	{
		mSortNodes.clear();
		for (TGraph::TNodes::iterator i=mGraph.nodes_begin(); i!=mGraph.nodes_end(); i++)
		{
			mSortNode.mNodeHandle	= mGraph.node_handle(i);
			mSortNode.mNodePointer	= &(*i);
			mSortNodes.push_back(mSortNode);
		}
		mSortNodes.sort();		

	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Create A New Edge A->B, And Fix Up The Face
    ////////////////////////////////////////////////////////////////////////////////////
	int		add_edge(int A, int B, int Face=0, bool OnHull=true)
	{
		assert(A!=B );

		int	nEdge = mLinks.get(mGraph.node_index(A), mGraph.node_index(B));

		// Apparently This Edge Does Not Exist, So Make A New One
		//--------------------------------------------------------
		if (nEdge==0)
		{
			nEdge= mEdges.alloc();
			
			mHull.insert_after(mHullIter, nEdge);
			assert(mHullIter!=mHull.end());

			mEdges[nEdge].mA		= A;
			mEdges[nEdge].mB		= B;
			mEdges[nEdge].mHullLoc	= mHullIter;
			mEdges[nEdge].mOnHull	= true;
			mEdges[nEdge].mFlips	= 0;
			mEdges[nEdge].mLeft		= 0;
			mEdges[nEdge].mRight	= 0;


			mLinks.get(mGraph.node_index(A), mGraph.node_index(B)) = nEdge;
			mLinks.get(mGraph.node_index(B), mGraph.node_index(A)) = nEdge;
		}

		// If This Edge DOES Already Exist, Then We Need To Remove It From The Hull
		//--------------------------------------------------------------------------
		else if (mEdges[nEdge].mOnHull)
		{
			assert(mEdges[nEdge].mHullLoc!=mHull.end());

			if (mHullIter==mEdges[nEdge].mHullLoc)
			{
				mHull.erase(mHullIter);					// Make Sure To Fix Up The Hull Iter If That Is What We Are Removing
			}
			else
			{
				mHull.erase(mEdges[nEdge].mHullLoc);
			}
			mEdges[nEdge].mOnHull = false;
		}


		// If The Edge Was Made With The Same Orientation Currently Asked For (A->B), Then Mark Face As Right
		//----------------------------------------------------------------------------------------------------
		if (mEdges[nEdge].mA==A)
		{
			mEdges[nEdge].mRight	= Face;
		}
		else
		{
			mEdges[nEdge].mLeft		= Face;
		}
		return nEdge;
	}


    ////////////////////////////////////////////////////////////////////////////////////
	// Create A New Face A->B->C, And Fix Up The Edges & Neighboring Faces
    ////////////////////////////////////////////////////////////////////////////////////
	int		add_face(int A, int B, int C)
	{
		int		Temp	= 0;
		int		nFace	= mFaces.alloc();

		// First, Make Sure Node A.x Is Greater Than B and C.  If Not, Swap With B or C
		//------------------------------------------------------------------------------
		if (mGraph.get_node(B)[0]>mGraph.get_node(A)[0])
		{
			Temp	= A;
			A		= B;
			B		= Temp;
		}
		if (mGraph.get_node(C)[0]>mGraph.get_node(A)[0])
		{
			Temp	= A;
			A		= C;
			C		= Temp;
		}

		// Similarly, Make Sure Node B.y Is Greater Than Node C.y
		//--------------------------------------------------------
		if (mGraph.get_node(C).LRTest(mGraph.get_node(A), mGraph.get_node(B))==Side_Left)
		{
			Temp	= C;
			C		= B;
			B		= Temp;
		}

		// DEBUG ASSERTS
		//====================================================================================
		// IF YOU HIT THESE ASSERTS, CHANCES ARE THAT YOU ARE TRYING TO TRIANGULATE OVER A SET
		// WITH MORE THAN 2 COLINEAR POINTS ON THE SAME FACE.  INSERT HULL WILL FAIL IN THIS 
		// FACE.  INSERT HULL WILL FAIL IN THIS SITUATION
		
		assert(mGraph.get_node(C).LRTest(mGraph.get_node(A), mGraph.get_node(B))==Side_Right);
		assert(mGraph.get_node(A).LRTest(mGraph.get_node(B), mGraph.get_node(C))==Side_Right);
		assert(mGraph.get_node(B).LRTest(mGraph.get_node(C), mGraph.get_node(A))==Side_Right);
		//====================================================================================

		mFaces[nFace].mA		= A;
		mFaces[nFace].mB		= B;
		mFaces[nFace].mC		= C;

		mFaces[nFace].mRight	= add_edge(C, A, nFace);
		mFaces[nFace].mBottom	= add_edge(A, B, nFace);
		mFaces[nFace].mLeft		= add_edge(B, C, nFace);

		mFaces[nFace].mFlips	= 0;

		return nFace;
	}


    ////////////////////////////////////////////////////////////////////////////////////
	// Insertion Hull Triangulation
	//
	// This algorithm works by scanning the outer convex hull of the set of points that
	// have already been triangulated.  When encountering a hull edge which evaluates
	// LEFT in a left right test (remember, the triangles always have clockwise orientation)
	// it adds a face to the triangulation including the edge as one side of the triangle
	// and two new edges to the node handle.  It's very important to traverse the convex
	// hull in counter clockwise order (backwards).
	//
	// In the example below, we assume the convex hull starts at the edge (CA).  (nodeHandle) is
	// RIGHT of (C->A), so it skips that edge and moves on to (D->C).  (nodeHandle) is in fact
	// LEFT of (D->C), so we would add a new face that would go (D->nodeHandle->C), and we remove
	// (D->C) from the hull.
	//
	//
	//
	//                              (C)-------------(A)
	//                              / \         __/   \
	//       (nodeHandle)         /    \     __/       \
	//                          /       \   /           \
	//                        (D)----____(B)_            \
	//                          \         |  \ __
	//                           \        |      \__
	//                            \       |         \
	//
    ////////////////////////////////////////////////////////////////////////////////////
	void	insert_point(int nodeHandle)
	{
		// Iterate Over The Existing Convex Hull
		//---------------------------------------
		for (mHullIter = mHull.begin(); mHullIter!=mHull.end(); mHullIter++)
		{
			edge&	curEdge = mEdges[*mHullIter];

			// Can This Edge "See" The node Handle We Have Passed In?
			//---------------------------------------------------------
			if ( mGraph.get_node(nodeHandle).LRTest(mGraph.get_node(curEdge.mA), mGraph.get_node(curEdge.mB))==Side_Left )
			{
				// Then Add The Face And Remove This Edge From The Hull
				//------------------------------------------------------
				add_face(curEdge.mA, curEdge.mB, nodeHandle);
			}
		}
	}



    ////////////////////////////////////////////////////////////////////////////////////
	// Edge Flip Function
	// 
	// This function scans the edge list for any edge that is "bad" (defined as not 
	// fitting within the circumscribed circle of either adjoining face).  When it
	// encounters one, it "flips" the edge in question and fixes up the adjoining faces
	// which were altered.
	//
	//
	// The Flip Edge (PtA->PtB):
	//
	//
	//
	//          BEFORE                         AFTER
	//
	//           (PtR)				           (PtA)				
	//           /   \				           / | \				
	//         /       \			         /   |   \			
	//       /  (FaceR)  \			       /     V     \			
	//     /               \		     /       |       \		
	//  (PtB)-<---------<-(PtA)		  (PtR)      |     (PtL)		
	//     \               /		     \       |       /		
	//       \  (FaceL)  /			       \     V     /			
	//         \       /			         \   |   /			
	//           \   /				           \ | /				
	//           (PtL)				           (PtB)				
	//
    ////////////////////////////////////////////////////////////////////////////////////
	int		flip()
	{
		int		Flipped = 0;
		
		int		EdgeHandle;
		int		PtR,	PtL,	PtA,	PtB;
		int		EdgeRL,	EdgeRR,	EdgeLL,	EdgeLR;


		// Iterate Through All The Edges Looking For Potential NON-Delauney Edges
		//------------------------------------------------------------------------
		for (TEdgesIter CurEdge=mEdges.begin(); CurEdge!=mEdges.end(); CurEdge++)
		{
			// If It Is On The Hull, We Don't Even Need To Look At It
			//--------------------------------------------------------
			if (!(*CurEdge).mOnHull)
			{
				edge&	EdgeAt	= *CurEdge;
				face&	FaceR	= mFaces[EdgeAt.mRight];
				face&	FaceL	= mFaces[EdgeAt.mLeft];

				EdgeHandle		= mEdges.index_to_handle(CurEdge.index());
				PtA				= EdgeAt.mA;
				PtB				= EdgeAt.mB;
				PtR				= FaceR.opposing_node(PtA, PtB);
				PtL				= FaceL.opposing_node(PtA, PtB);

				assert(EdgeAt.mRight!=EdgeAt.mLeft);
				assert(PtA!=PtB);
				assert(PtR!=PtL);
				assert(PtA!=PtR && PtA!=PtL);
				assert(PtB!=PtR && PtB!=PtL);

				// Is This Edge Invalid For Delaunay?
				//-------------------------------------
				if (!mGraph.get_node(PtB).InCircle(mGraph.get_node(PtR), mGraph.get_node(PtL), mGraph.get_node(PtA)) &&
					!mGraph.get_node(PtA).InCircle(mGraph.get_node(PtR), mGraph.get_node(PtB), mGraph.get_node(PtL))					
					)
				{
					// Change The Link: Remove The Old, Add The New
					//----------------------------------------------
					mLinks.get(mGraph.node_index(PtA), mGraph.node_index(PtB)) = 0;
					mLinks.get(mGraph.node_index(PtB), mGraph.node_index(PtA)) = 0;

					mLinks.get(mGraph.node_index(PtR), mGraph.node_index(PtL)) = EdgeHandle;
					mLinks.get(mGraph.node_index(PtL), mGraph.node_index(PtR)) = EdgeHandle;


					Flipped++;
					EdgeAt.mFlips++;
					FaceL.mFlips++;
					FaceR.mFlips++;

					// Flip The Edge We Found
					//------------------------
					EdgeAt.mA		= PtR;
					EdgeAt.mB		= PtL;

					// Calculate Relatave Edges And Points Assuming (EdgeAt) Were mBottom For The Two Faces
					//--------------------------------------------------------------------------------------
					EdgeRL			= FaceR.relative_left(EdgeHandle);
					EdgeRR			= FaceR.relative_right(EdgeHandle);

					EdgeLL			= FaceL.relative_left(EdgeHandle);
					EdgeLR			= FaceL.relative_right(EdgeHandle);


					// Fix Edges Which Had Been Rotated To New Faces
					//-----------------------------------------------
					mEdges[EdgeLR].flip_face(EdgeAt.mLeft, EdgeAt.mRight);
					mEdges[EdgeRR].flip_face(EdgeAt.mRight, EdgeAt.mLeft);

					// Rotate The Edges Clockwise
					//----------------------------
					FaceR.mLeft		= EdgeLR;
					FaceR.mRight	= EdgeRL;
					FaceR.mBottom	= EdgeHandle;

					FaceL.mLeft		= EdgeRR;
					FaceL.mRight	= EdgeLL;
					FaceL.mBottom	= EdgeHandle;

					FaceR.mA		= PtR;
					FaceR.mB		= PtL;
					FaceR.mC		= PtB;

					FaceL.mA		= PtR;
					FaceL.mB		= PtL;
					FaceL.mC		= PtA;


					// DEBUG VERIFICATION
					//========================================================================
					#ifdef _DEBUG
					mEdges[FaceR.mLeft  ].verify(FaceR.mA, FaceR.mB, FaceR.mC,	EdgeAt.mRight);
					mEdges[FaceR.mRight ].verify(FaceR.mA, FaceR.mB, FaceR.mC,	EdgeAt.mRight);
					mEdges[FaceR.mBottom].verify(FaceR.mA, FaceR.mB,			EdgeAt.mRight);

					mEdges[FaceL.mLeft  ].verify(FaceL.mA, FaceL.mB, FaceL.mC,	EdgeAt.mLeft);
					mEdges[FaceL.mRight ].verify(FaceL.mA, FaceL.mB, FaceL.mC,	EdgeAt.mLeft);
					mEdges[FaceL.mBottom].verify(FaceL.mA, FaceL.mB,			EdgeAt.mLeft);
					#endif

					assert(EdgeAt.mRight!=EdgeAt.mLeft);
					assert(PtA!=PtB);
					assert(PtR!=PtL);
					//========================================================================
				}
			}
		}
		return Flipped;
	}

};




}
#endif