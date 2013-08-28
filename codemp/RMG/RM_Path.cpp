//Anything above this #include will be ignored by the compiler
#include "qcommon/exe_headers.h"

/************************************************************************************************
 *
 *	Copyright (C) 2001-2002 Raven Software
 *
 *  RM_Path.cpp
 *
 ************************************************************************************************/

#include "RM_Headers.h"

#ifndef max
	#define max(a,b)    (((a) > (b)) ? (a) : (b))
#endif

/************************************************************************************************
 * CRMNode::CRMNode
 *	constructor
 *
 * inputs:
 *  none
 *
 * return:
 *	none
 *
 ************************************************************************************************/
CRMNode::CRMNode ( )
{
	int i;

	mFlattenHeight = -1;

	mPos[0] = 0;
	mPos[1] = 0;
	mPos[2] = 0;

	// no paths
	for (i = 0; i < DIR_MAX; i++)
		mPathID[i] = -1;

	mAreaPointPlaced = false;
}


/************************************************************************************************
 * CRMPathManager::CRMPathManager
 *	constructor
 *
 * inputs:
 *  none
 *
 * return:
 *	none
 *
 ************************************************************************************************/
CRMPathManager::CRMPathManager ( CRandomTerrain* terrain )
: mXNodes(0), mYNodes(0), mPathCount(0), mRiverCount(0), mMaxDepth(0), mDepth(0),
  mCrossed(false), 
  mPathPoints(10), mPathMinWidth(0.02f), mPathMaxWidth(0.04f), mPathDepth(0.3f), mPathDeviation(0.03f), mPathBreadth(5),
  mRiverDepth(5), mRiverPoints(10), mRiverMinWidth(0.01f), mRiverMaxWidth(0.02f), mRiverBedDepth(1), mRiverDeviation(0.01f), mRiverBreadth(7),
  mTerrain(terrain)
{
}

CRMPathManager::~CRMPathManager ( )
{
	int i,j;

	for ( i = mLocations.size() - 1; i >=0; i-- )
	{
		if (mLocations[i])
			delete mLocations[i];
	}
	mLocations.clear();

	for ( j = mNodes.size() - 1; j >=0; j-- )
	{
		if (mNodes[j])
			delete mNodes[j];
	}
	mNodes.clear();
}

void CRMPathManager::CreateLocation ( const char* name, const int min_depth, int max_depth, const int min_paths, int max_paths )
{
	int i;

	// sanity checks -- dmv
	if( max_paths < min_paths )
	{
		Com_Printf("[CreateLocation()] ERROR : max_paths < min_paths :: set max_paths = min_paths\n" );
		max_paths = min_paths;
	}
	if( max_depth < min_depth )
	{
		Com_Printf("[CreateLocation()] ERROR : max_depth < min_depth :: set max_depth = min_depth\n" );
		max_depth = min_depth;
	}

	for (i = mLocations.size()-1; i>=0; --i)
		if ( !Q_stricmp ( name, mLocations[i]->GetName ( ) ) )
		{
			mLocations[i]->SetMinDepth(min_depth);
			mLocations[i]->SetMaxDepth(max_depth);
			mLocations[i]->SetMinPaths(min_paths);
			mLocations[i]->SetMaxPaths(max_paths);
			return;
		}

	CRMLoc* pLoc= new CRMLoc(name, min_depth, max_depth, min_paths, max_paths); 
	mLocations.push_back(pLoc); 
	mMaxDepth =  max(mMaxDepth, max_depth);
}

void CRMPathManager::ClearCells(int x_nodes, int y_nodes)
{
	int x,y;

	// clear cell array - used for generating paths
	CRMCell empty;
	for (x=0; x < x_nodes * y_nodes; x++)
	{
		if (x >= (int)mCells.size())
			mCells.push_back(empty);
		else
			mCells[x] = empty;
	}

	// set borders of world
	for (y = 0; y < y_nodes; y++)
	{
		mCells[y * x_nodes].SetBorder(DIR_W );
		mCells[y * x_nodes].SetBorder(DIR_SW );
		mCells[y * x_nodes].SetBorder(DIR_NW );
		mCells[y * x_nodes + x_nodes-1].SetBorder( DIR_E );
		mCells[y * x_nodes + x_nodes-1].SetBorder( DIR_NE );
		mCells[y * x_nodes + x_nodes-1].SetBorder( DIR_SE );
	}

	for (x = 0; x < x_nodes; x++)
	{
		mCells[x].SetBorder( DIR_N );
		mCells[x].SetBorder( DIR_NE );
		mCells[x].SetBorder( DIR_NW );
		mCells[(y_nodes-1) * x_nodes + x].SetBorder( DIR_S );
		mCells[(y_nodes-1) * x_nodes + x].SetBorder( DIR_SE );
		mCells[(y_nodes-1) * x_nodes + x].SetBorder( DIR_SW );
	}
}

/************************************************************************************************
 * CRMPathManager::CreateArray
 *	Create array of nodes that are spaced over the landscape.
 *	Create array of cells, which is used to determine how nodes are connected.
 *
 * inputs:
 *  x_nodes, y_nodes - how many nodes in each dimension to layout
 *
 * return:
 *	true if the node array was created, false if we have a problem
 *
 ************************************************************************************************/
bool CRMPathManager::CreateArray(const int x_nodes, const int y_nodes)
{
	mXNodes = x_nodes;
	mYNodes = y_nodes;

	// fill node array with positions that are spaced over the landscape
	int x,y;

	// dump existing nodes
	for ( x = mNodes.size() - 1; x >=0; x-- )
	{
		if (mNodes[x])
			delete mNodes[x];
	}
	mNodes.clear();
	mNodes.resize(mXNodes * mYNodes, NULL);

	// add a small amount of random jitter to spots chosen
	float x_rnd = 0.2f / (mXNodes+1);
	float y_rnd = 0.2f / (mYNodes+1);

	for (x = 0; x < mXNodes; x++)
	{
		float cell_x = (x + 1.0f) / (mXNodes+1);
//		float cell_x = (x + 2.0f) / (mXNodes+3);

		for (y = 0; y < mYNodes; y++)
		{
			vec3_t pos;
			CRMNode * pnode = new CRMNode();
			mNodes[x + y*mXNodes] = pnode;

			float cell_y = (y + 1.0f) / (mYNodes+1);
//			float cell_y = (y + 2.0f) / (mYNodes+3);

			pos[0] = TheRandomMissionManager->GetLandScape()->flrand(cell_x - x_rnd, cell_x + x_rnd);
			pos[1] = TheRandomMissionManager->GetLandScape()->flrand(cell_y - y_rnd, cell_y + y_rnd);
			pos[2] = 0;
			 
			SetNodePos(x, y, pos);
		}
	}

	ClearCells(mXNodes, mYNodes);

	return true;
}

// neighbor offsets - easy way to turn a direction into the array position for a neighboring cell or node
int CRMPathManager::neighbor_x[DIR_MAX] = { 0, 1, 1, 1, 0,-1,-1,-1};
int CRMPathManager::neighbor_y[DIR_MAX] = {-1,-1, 0, 1, 1, 1, 0,-1};

/************************************************************************************************
 * CRMPathManager::PlaceLocation
 * This method is used to determine if a named location should be placed at this node.
 *
 * inputs:
 *  c_x, c_y - cell to examine
 *
 * return:
 *	none
 *
 ************************************************************************************************/
void CRMPathManager::PlaceLocation(const int c_x, const int c_y)
{
	if ( !Node(c_x,c_y)->IsLocation() )
	{	// not currently a location

		// how many paths lead to this cell?
		int count_paths = 0;
		int i;

		for (i = 0; i<DIR_MAX; i++)
			if (Node(c_x,c_y)->PathExist(i))
				count_paths++;

		int deepest_depth = -1;
		int deepest_loc = -1;
		for (i = mLocations.size()-1; i>=0; --i)
		{
			if (!mLocations[i]->Placed() &&					// node has not been placed
				 mLocations[i]->MinDepth() <= mDepth &&		// our current depth is in the proper range
				 mLocations[i]->MaxDepth() >= mDepth &&
				 mLocations[i]->MinPaths() <= count_paths && // our path count is in the proper range
				 mLocations[i]->MaxPaths() >= count_paths &&
				 mLocations[i]->MaxDepth() > deepest_depth)	// and this is the deepest location of the ones that match
			{
				deepest_loc = i;
				deepest_depth = mLocations[i]->MaxDepth();
			}
		}

		if (deepest_loc >= 0 && deepest_loc < (int)mLocations.size())
		{	// found a location to place at this node / cell
			const char * name = mLocations[deepest_loc]->GetName();
			Node(c_x,c_y)->SetName(name);
			mLocations[deepest_loc]->SetPlaced(true);

			// need a new max depth
			int max_depth = -1;
			for (i = mLocations.size()-1; i>=0; --i)
			{
				// figure out new max depth based on the max depth of unplaced locations
				if (!mLocations[i]->Placed() &&	   			// node has not been placed
					 mLocations[i]->MaxDepth() > max_depth)	// and this is the deepest 
				{
					max_depth = mLocations[i]->MaxDepth();
				}
			}
			mMaxDepth = max_depth;
		}
	}
}

/************************************************************************************************
 * CRMPathManager::PathVisit
 * This method is called recursively to create a network of nodes connected with paths.
 *
 * inputs:
 *  c_x, c_y - cell to visit
 *
 * return:
 *	none
 *
 ************************************************************************************************/
void CRMPathManager::PathVisit(const int c_x, const int c_y)
{
	// does this cell have any neighbors with all walls intact?
	int i,off;
	
	// look at neighbors in random order
	off = TheRandomMissionManager->GetLandScape()->irand(DIR_FIRST, DIR_MAX-1);

	++mDepth;	// track our depth of recursion

	for (i = DIR_FIRST; i<DIR_MAX && mDepth <= mMaxDepth; i++)
	{
		int d = (i + off) % DIR_MAX;
		if ( !Cell(c_x, c_y).Border(d) )
		{	// we can move this way, since no border
			int new_c_x = c_x + neighbor_x[d];
			int new_c_y = c_y + neighbor_y[d];
			if (Cell(new_c_x,new_c_y).Wall() == DIR_ALL)
			{	// we have a new cell that has not been visited!
				int new_dir;
				// d is the direction relative to the current cell
				// new_dir is the direction relative to the next cell (N becomes S, NE becomes SW, etc...)
				if( d < HALF_DIR_MAX )
				{
					new_dir = d + HALF_DIR_MAX;
				}
				else
				{
					new_dir = d - HALF_DIR_MAX;
				}
	
				// knock down walls
				Cell(c_x,c_y).RemoveWall(d);
				Cell(new_c_x,new_c_y).RemoveWall(new_dir); //DIR_MAX - d);

				// set path id
				Node(c_x, c_y)->SetPath(d, mPathCount);
				Node(new_c_x, new_c_y)->SetPath(new_dir, mPathCount); //DIR_MAX - d, mPathCount);

				// create path between cells
				mTerrain->CreatePath( mPathCount++,
									-1,
									0,
									mPathPoints,
									GetNodePos(c_x,c_y)[0],
									GetNodePos(c_x,c_y)[1],
									GetNodePos(new_c_x,new_c_y)[0],
									GetNodePos(new_c_x,new_c_y)[1],
									mPathMinWidth,
									mPathMaxWidth,
									mPathDepth,
									mPathDeviation,
									mPathBreadth );

				// flatten a small spot
				CArea		area;
				float flat_radius = mPathMaxWidth * 
					fabs(TheRandomMissionManager->GetLandScape()->GetBounds()[1][0] - TheRandomMissionManager->GetLandScape()->GetBounds()[0][0]);
				area.Init( GetNodePos(c_x,c_y), flat_radius, 0.0f, AT_NONE, 0, 0 );
				TheRandomMissionManager->GetLandScape()->FlattenArea(&area, 255 * mPathDepth, false, true, true );

				// recurse
				PathVisit(new_c_x, new_c_y);
			}
		}
	}
	
	--mDepth;

	// NOTE: *whoop* hack alert, the first time this is reached, it should be the very last placed node.
	if( !mCrossed && TheRandomMissionManager->GetMission()->GetSymmetric() &&
		TheRandomMissionManager->GetMission()->GetBackUpPath() )
	{	
		mCrossed = true; 
	
		int directionSet[3][3] = { {DIR_NW,DIR_W,DIR_SW},{DIR_N,-1,DIR_S},{DIR_NE,DIR_E,DIR_SE}};
		int	ncx	= (mXNodes-1)-c_x;
		int	ncy	= (mYNodes-1)-c_y;

		int x_delta = ncx - c_x;
		int y_delta = ncy - c_y;

		if( x_delta < -1 )
		{
			x_delta = -1;
		}
		else if( x_delta > 1 )
		{
			x_delta = 1;
		}

		if( y_delta < -1 )
		{
			y_delta = -1;
		}
		else if( y_delta > 1 )
		{
			y_delta = 1;
		}
		
		// make sure the mirror is actually in a different position than then un-mirrored node
		if( x_delta || y_delta )
		{

			int d = directionSet[x_delta][y_delta];
			int new_dir;
			// d is the direction relative to the current cell
			// new_dir is the direction relative to the next cell (N becomes S, NE becomes SW, etc...)
			if( d < HALF_DIR_MAX )
			{
				new_dir = d + HALF_DIR_MAX;
			}
			else
			{
				new_dir = d - HALF_DIR_MAX;
			}

			//NOTE: Knocking down these walls will cause instances to be created on this new artificial path
			// Since this path could span more than just the normal 1 cell, these walls being knocked down are not exactly correct... but get the job done

			// knock down walls
			Cell(c_x,c_y).RemoveWall(d);
			Cell(ncx,ncy).RemoveWall(new_dir); //DIR_MAX - d);

			// set path id
			Node(c_x, c_y)->SetPath(d, mPathCount);
			Node(ncx, ncy)->SetPath(new_dir, mPathCount); //DIR_MAX - d, mPathCount);
		
			// create an artificial path that crosses over to connect the symmetric and non-symmetric map parts
			mTerrain->CreatePath( mPathCount++,
								-1,
								0,
								mPathPoints,
								GetNodePos(c_x,c_y)[0],
								GetNodePos(c_x,c_y)[1],
								GetNodePos(ncx,ncy)[0],
								GetNodePos(ncx,ncy)[1],
								mPathMinWidth,
								mPathMaxWidth,
								mPathDepth,
								mPathDeviation,
								mPathBreadth );
		
		}
	}

	PlaceLocation(c_x, c_y);
}


/************************************************************************************************
 * CRMPathManager::FindNodeByName
 *	Finds the managed node with the matching case-insensivity name
 *
 * inputs:
 *  name - name of the node to find
 *
 * return:
 *	a pointer to the found node or NULL if the node couldn't be found
 *
 ************************************************************************************************/
CRMNode* CRMPathManager::FindNodeByName ( const char* name )
{
	int		 j;

	for ( j = mNodes.size() - 1; j >=0; j-- )
	{
		if ( !Q_stricmp ( name, mNodes[j]->GetName ( ) ) )
			return mNodes[j];
	}
	return NULL;
}


/************************************************************************************************
 * CRMPathManager::SetPathStyle
 *	sets style for all paths
 *
 * inputs:
 *  settings for paths that are created
 *
 * return:
 *	none
 *
 ************************************************************************************************/
void CRMPathManager::SetPathStyle ( 
	const int	points,
	const float	minwidth,
	const float	maxwidth,
	const float	depth,
	const float	deviation,
	const float	breadth
	)
{
	// save path style
	mPathPoints   = points  ;
	mPathMinWidth = minwidth;
	mPathMaxWidth = maxwidth;
	mPathDepth    = depth   ;
	mPathDeviation= deviation;
	mPathBreadth  = breadth ;
}

/************************************************************************************************
 * CRMPathManager::SetRiverStyle
 *	sets style for all rivers
 *
 * inputs:
 *  settings for river paths that are created
 *
 * return:
 *	none
 *
 ************************************************************************************************/
void CRMPathManager::SetRiverStyle (const int   depth,
									const int	points,
									const float minwidth,
									const float maxwidth,
									const float beddepth, 
									const float deviation, 
									const float breadth,
									string bridge_name)
{
	// save river style
	mRiverDepth    = depth;
	mRiverPoints   = points  ;
	mRiverMinWidth = minwidth;
	mRiverMaxWidth = maxwidth;
	mRiverBedDepth = beddepth   ;
	mRiverDeviation= deviation;
	mRiverBreadth  = breadth ;
	mRiverBridge   = bridge_name;
}

vec3_t&	CRMPathManager::GetRiverPos( const int x, const int y ) 
{ 
	mRiverPos[0] = (float)(x + 1.0f) / (float)(mXNodes+2);
	mRiverPos[1] = (float)(y + 1.0f) / (float)(mYNodes+2);
	return mRiverPos; 
}

void CRMPathManager::RiverVisit(const int c_x, const int c_y)
{
	// does this cell have any neighbors with all walls intact?
	int i,off;

	// look at neighbors in random order
	off = TheRandomMissionManager->GetLandScape()->irand(DIR_FIRST, DIR_MAX-1);

	++mDepth;	// track our depth of recursion

	for (i = DIR_FIRST; i<DIR_MAX && mDepth <= mMaxDepth; i+=2)
	{
		int d = (i + off) % DIR_MAX;
		if ( !Cell(c_x, c_y).Border(d) )
		{	// we can move this way, since no border
			int new_c_x = c_x + neighbor_x[d];
			int new_c_y = c_y + neighbor_y[d];
			if (RiverCell(new_c_x,new_c_y).Wall() == DIR_ALL)
			{	// we have a new cell that has not been visited!

				int new_dir;
				// d is the direction relative to the current cell
				// new_dir is the direction relative to the next cell (N becomes S, NE becomes SW, etc...)
				if( d < HALF_DIR_MAX )
				{
					new_dir = d + HALF_DIR_MAX;
				}
				else
				{
					new_dir = d - HALF_DIR_MAX;
				}
				// knock down walls
				RiverCell(c_x,c_y).RemoveWall(d);
				RiverCell(new_c_x,new_c_y).RemoveWall(new_dir); //DIR_MAX - d);

				// create river between cells
				mTerrain->CreatePath ( mPathCount++,
									-1,
									0,
									mRiverPoints,
									GetRiverPos(c_x,c_y)[0],
									GetRiverPos(c_x,c_y)[1],
									GetRiverPos(new_c_x,new_c_y)[0],
									GetRiverPos(new_c_x,new_c_y)[1],
									mRiverMinWidth,
									mRiverMaxWidth,
									mRiverBedDepth,
									mRiverDeviation,
									mRiverBreadth );

				// flatten a small spot
				CArea		area;
				float flat_radius = mRiverMinWidth * 
					fabs(TheRandomMissionManager->GetLandScape()->GetBounds()[1][0] - TheRandomMissionManager->GetLandScape()->GetBounds()[0][0]);
				area.Init( GetRiverPos(c_x,c_y), flat_radius, 0.0f, AT_NONE, 0, 0 );
				TheRandomMissionManager->GetLandScape()->FlattenArea (&area, 255 * mRiverBedDepth, false, true, true );

				// recurse
				RiverVisit(new_c_x, new_c_y);
			}
		}
	}
	
//	--mDepth;
}

/************************************************************************************************
 * CRMPathManager::GenerateRivers
 *	Creates a river which intersects the main path
 *
 * inputs:
 *  none
 *
 * return:
 *	none
 *
 ************************************************************************************************/
void CRMPathManager::GenerateRivers ()
{
	if (mRiverBedDepth == 1)
		// no rivers
		return;

	mMaxDepth = mRiverDepth;
	mDepth = 0;

	int cell_x = 0;
	int cell_y = 0;

	// choose starting cell along an edge
	int edge = TheRandomMissionManager->GetLandScape()->irand(0, 7);
	switch ( edge )
	{
		case 0:
			cell_x = mXNodes / 2; cell_y = 0;
			break;
		case 1:
			cell_x = mXNodes; cell_y = 0;
			break;
		case 2:
			cell_x = mXNodes; cell_y = mYNodes / 2;
			break;
		case 3:
			cell_x = mXNodes; cell_y = mYNodes;
			break;
		case 4:
			cell_x = mXNodes / 2; cell_y = mYNodes;
			break;
		case 5:
			cell_x = 0; cell_y = mYNodes;
			break;
		case 6:
			cell_x = 0; cell_y = mYNodes / 2;
			break;
		case 7:
			cell_x = 0; cell_y = 0;
			break;
	}

	ClearCells(mXNodes+1, mYNodes+1);

	mRiverCount = mPathCount;

	// visit the first cell
	RiverVisit(cell_x,cell_y);

	mRiverCount = mPathCount - mRiverCount;

	return;
}

/************************************************************************************************
 * CRMPathManager::GeneratePaths
 *	Creates all paths 
 *
 * inputs:
 *  none
 *
 * return:
 *	none
 *
 ************************************************************************************************/
void CRMPathManager::GeneratePaths ( symmetry_t symmetric )
{
	int cell_x = 0;
	int cell_y = 0;

	switch ( symmetric )
	{
		case SYMMETRY_TOPLEFT:
			cell_x = mXNodes-1;
			cell_y = 0;
			break;

		case SYMMETRY_BOTTOMRIGHT:
			cell_x = 0;
			cell_y = mYNodes-1;
			break;

		default:
		case SYMMETRY_NONE:
			// choose starting cell along an edge
			switch ( TheRandomMissionManager->GetLandScape()->irand(0, 7) )
			{
				case 0:
					cell_x = mXNodes / 2;
					break;
				case 1:
					cell_x = mXNodes-1;
					break;
				case 2:
					cell_x = mXNodes-1; cell_y = mYNodes / 2;
					break;
				case 3:
					cell_x = mXNodes-1; cell_y = mYNodes-1;
					break;
				case 4:
					cell_x = mXNodes / 2; cell_y = mYNodes-1;
					break;
				case 5:
					cell_y = mYNodes-1;
					break;
				case 6:
					cell_y = mYNodes / 2;
					break;
				default:
				case 7:
					break;
			}
			break;
	}

	// visit the first cell
	PathVisit(cell_x,cell_y);
}

