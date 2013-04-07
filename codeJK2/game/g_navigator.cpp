// leave this line at the top for all g_xxxx.cpp files...
#include "g_headers.h"
#include <algorithm>

#include "b_local.h"
#include "g_navigator.h"
#include "g_nav.h"
#include <time.h>



extern int NAVNEW_ClearPathBetweenPoints(vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, int ignore, int clipmask);
extern qboolean NAV_CheckNodeFailedForEnt( gentity_t *ent, int nodeNum );
extern qboolean G_EntIsUnlockedDoor( int entityNum );
extern qboolean G_EntIsDoor( int entityNum );
extern qboolean G_EntIsBreakable( int entityNum );
extern qboolean G_EntIsRemovableUsable( int entNum );

extern	cvar_t		*d_altRoutes;
extern	cvar_t		*d_patched;


static vec3_t	wpMaxs = {  16,  16, 32 };
static vec3_t	wpMins = { -16, -16, -24+STEPSIZE };//WTF:  was 16??!!!


static byte CHECKED_NO = 0;
static byte CHECKED_FAILED = 1;
static byte CHECKED_PASSED = 2;


int GetTime ( int lastTime )
{
	int			curtime;
	static int	timeBase = 0;
	static qboolean	initialized = qfalse;

	if (!initialized) {
		timeBase = timeGetTime();
		initialized = qtrue;
	}
	curtime = timeGetTime() - timeBase - lastTime;

	return curtime;
}

/*
-------------------------
CEdge
-------------------------
*/

CEdge::CEdge( void )
{
	CEdge( -1, -1, -1 );
}

CEdge::CEdge( int first, int second, int cost )
{
	m_first		= first;
	m_second	= second;
	m_cost		= cost;
}

CEdge::~CEdge( void )
{
}

/*
-------------------------
CNode
-------------------------
*/

CNode::CNode( void )
{
	m_numEdges		= 0;
	m_radius		= 0;
	m_ranks			= NULL;
}

CNode::~CNode( void )
{
	m_edges.clear();

	if ( m_ranks )
		delete [] m_ranks;
}

/*
-------------------------
Create
-------------------------
*/

CNode *CNode::Create( vec3_t position, int flags, int radius, int ID )
{
	CNode	*node = new CNode;

	VectorCopy( position, node->m_position );
	
	node->m_flags = flags;
	node->m_ID = ID;
	node->m_radius = radius;

	return node;
}

/*
-------------------------
Create
-------------------------
*/

CNode *CNode::Create( void )
{
	return new CNode;
}

/*
-------------------------
AddEdge
-------------------------
*/

void CNode::AddEdge( int ID, int cost, int flags )
{
	if ( m_numEdges )
	{//already have at least 1
		//see if it exists already
		edge_v::iterator	ei;
		STL_ITERATE( ei, m_edges )
		{
			if ( (*ei).ID == ID )
			{//found it
				(*ei).cost	= cost;
				(*ei).flags	= flags;
				return;
			}
		}
	}
	
	edge_t	edge;

	edge.ID		= ID;
	edge.cost	= cost;
	edge.flags	= flags;

	STL_INSERT( m_edges, edge );

	m_numEdges++;

	assert( m_numEdges < 9 );//8 is the max
}

/*
-------------------------
GetEdge
-------------------------
*/

int CNode::GetEdgeNumToNode( int ID )
{
	int count = 0;

	edge_v::iterator	ei;
	STL_ITERATE( ei, m_edges )
	{
		if ( (*ei).ID == ID )
		{
			return count;
		}

		count++;
	}
	return -1;
}

/*
-------------------------
AddRank
-------------------------
*/

void CNode::AddRank( int ID, int rank )
{
	assert( m_ranks );

	m_ranks[ ID ] = rank;
}

/*
-------------------------
Draw
-------------------------
*/

void CNode::Draw( qboolean showRadius )
{
	CG_DrawNode( m_position, NODE_NORMAL );
	if( showRadius )
	{
		CG_DrawRadius( m_position, m_radius, NODE_NORMAL );
	}
}

/*
-------------------------
GetEdge
-------------------------
*/

int CNode::GetEdge( int edgeNum )
{
	if ( edgeNum > m_numEdges )
		return -1;

	int count = 0;

	edge_v::iterator	ei;
	STL_ITERATE( ei, m_edges )
	{
		if ( count == edgeNum )
		{
			return (*ei).ID;
		}

		count++;
	}

	return -1;
}

/*
-------------------------
GetEdgeCost
-------------------------
*/

int CNode::GetEdgeCost( int edgeNum )
{
	if ( edgeNum > m_numEdges )
		return Q3_INFINITE; // return -1;

	int count = 0;

	edge_v::iterator	ei;
	STL_ITERATE( ei, m_edges )
	{
		if ( count == edgeNum )
		{
			return (*ei).cost;
		}

		count++;
	}

	return Q3_INFINITE; // return -1;
}

/*
-------------------------
GetEdgeFlags
-------------------------
*/

BYTE CNode::GetEdgeFlags( int edgeNum )
{
	if ( edgeNum > m_numEdges )
		return 0;

	int count = 0;

	edge_v::iterator	ei;
	STL_ITERATE( ei, m_edges )
	{
		if ( count == edgeNum )
		{
			return (*ei).flags;
		}

		count++;
	}
	
	return 0;
}

/*
-------------------------
SetEdgeFlags
-------------------------
*/

void CNode::SetEdgeFlags( int edgeNum, int newFlags )
{
	if ( edgeNum > m_numEdges )
		return;

	int count = 0;

	edge_v::iterator	ei;
	STL_ITERATE( ei, m_edges )
	{
		if ( count == edgeNum )
		{
			(*ei).flags = newFlags;
			return;
		}

		count++;
	}

}
/*
-------------------------
InitRanks
-------------------------
*/

void CNode::InitRanks( int size )
{
	//Clear it if it's already allocated
	if ( m_ranks != NULL )
	{
		delete [] m_ranks;
		m_ranks = NULL;
	}

	m_ranks = new int[size];

	memset( m_ranks, -1, sizeof(int)*size );
}

/*
-------------------------
GetRank
-------------------------
*/

int CNode::GetRank( int ID )
{
	assert( m_ranks );

	return m_ranks[ ID ];
}


/*
-------------------------
Save
-------------------------
*/

int	CNode::Save( int numNodes, fileHandle_t file )
{
	int i;

	//Write out the header
	unsigned long header = NODE_HEADER_ID;
	gi.FS_Write( &header, sizeof( header ), file );

	//Write out the basic information
	for ( i = 0; i < 3; i++ )
		gi.FS_Write( &m_position[i], sizeof( float ), file );

	gi.FS_Write( &m_flags, sizeof( m_flags ), file );
	gi.FS_Write( &m_ID, sizeof( m_ID ), file );
	gi.FS_Write( &m_radius, sizeof( m_radius ), file );

	//Write out the edge information
	gi.FS_Write( &m_numEdges, sizeof( m_numEdges ), file );

	edge_v::iterator	ei;
	STL_ITERATE( ei, m_edges )
	{
		gi.FS_Write( &(*ei), sizeof( edge_t ), file );
	}

	//Write out the node ranks
	gi.FS_Write( &numNodes, sizeof( numNodes ), file );

	for ( i = 0; i < numNodes; i++ )
	{
		gi.FS_Write( &m_ranks[i], sizeof( int ), file );
	}

	return true;
}

/*
-------------------------
Load
-------------------------
*/

int CNode::Load( int numNodes, fileHandle_t file )
{
	unsigned long header;
	int i;
	gi.FS_Read( &header, sizeof(header), file );

	//Validate the header
	if ( header != NODE_HEADER_ID )
		return false;

	//Get the basic information
	for ( i = 0; i < 3; i++ )
		gi.FS_Read( &m_position[i], sizeof( float ), file );

	gi.FS_Read( &m_flags, sizeof( m_flags ), file );
	gi.FS_Read( &m_ID, sizeof( m_ID ), file );
	gi.FS_Read( &m_radius, sizeof( m_radius ), file );

	//Get the edge information
	gi.FS_Read( &m_numEdges, sizeof( m_numEdges ), file );

	for ( i = 0; i < m_numEdges; i++ )
	{
		edge_t	edge;

		gi.FS_Read( &edge, sizeof( edge_t ), file );

		STL_INSERT( m_edges, edge );
	}

	//Read the node ranks
	int	numRanks;

	gi.FS_Read( &numRanks, sizeof( numRanks ), file );

	//Allocate the memory
	InitRanks( numRanks );

	for ( i = 0; i < numRanks; i++ )
	{
		gi.FS_Read( &m_ranks[i], sizeof( int ), file );
	}

	return true;
}

/*
-------------------------
CNavigator
-------------------------
*/

CNavigator::CNavigator( void )
{
}

CNavigator::~CNavigator( void )
{
}

/*
-------------------------
FlagAllNodes
-------------------------
*/

void CNavigator::FlagAllNodes( int newFlag )
{
	node_v::iterator	ni;

	STL_ITERATE( ni, m_nodes )
	{
		(*ni)->AddFlag( newFlag );
	}
}

/*
-------------------------
GetChar
-------------------------
*/

char CNavigator::GetChar( fileHandle_t file )
{
	char value;

	gi.FS_Read( &value, sizeof( value ), file );

	return value;
}

/*
-------------------------
GetInt
-------------------------
*/

int	CNavigator::GetInt( fileHandle_t file )
{
	int value;

	gi.FS_Read( &value, sizeof( value ), file );

	return value;
}

/*
-------------------------
GetFloat
-------------------------
*/

float CNavigator::GetFloat( fileHandle_t file )
{
	float value;

	gi.FS_Read( &value, sizeof( value ), file );

	return value;
}

/*
-------------------------
GetLong
-------------------------
*/

long CNavigator::GetLong( fileHandle_t file )
{
	long value;

	gi.FS_Read( &value, sizeof( value ), file );

	return value;
}

/*
-------------------------
Init
-------------------------
*/

void CNavigator::Init( void )
{
	Free();
}

/*
-------------------------
Free
-------------------------
*/

void CNavigator::Free( void )
{
	node_v::iterator	ni;

	STL_ITERATE( ni, m_nodes )
	{
		delete (*ni);
	}
}

/*
-------------------------
Load
-------------------------
*/

bool CNavigator::Load( const char *filename, int checksum )
{
	fileHandle_t	file;

	//Attempt to load the file
	gi.FS_FOpenFile( va( "maps/%s.nav", filename ), &file, FS_READ );

	//See if we succeeded
	if ( file == NULL )
		return false;

	//Check the header id
	long navID = GetLong( file );

	if ( navID != NAV_HEADER_ID )
	{
		gi.FS_FCloseFile( file );
		return false;
	}

	//Check the checksum to see if this file is out of date
	int check = GetInt( file );

	if ( check != checksum )
	{
		gi.FS_FCloseFile( file );
		return false;
	}

	int numNodes = GetInt( file );
	
	for ( int i = 0; i < numNodes; i++ )
	{
		CNode	*node = CNode::Create();

		if ( node->Load( numNodes, file ) == false )
		{
			gi.FS_FCloseFile( file );
			return false;
		}

		STL_INSERT( m_nodes, node );
	}

	//read in the failed edges
	gi.FS_Read( &failedEdges, sizeof( failedEdges ), file );
	for ( int j = 0; j < MAX_FAILED_EDGES; j++ )
	{
		m_edgeLookupMap.insert(pair<int, int>(failedEdges[j].startID, j));		
	}
		

	gi.FS_FCloseFile( file );

	return true;
}

/*
-------------------------
Save
-------------------------
*/

bool CNavigator::Save( const char *filename, int checksum )
{
	fileHandle_t	file;

	//Attempt to load the file
	gi.FS_FOpenFile( va( "maps/%s.nav", filename ), &file, FS_WRITE );

	if ( file == NULL )
		return false;

	//Write out the header id
	unsigned long id = NAV_HEADER_ID;

	gi.FS_Write( &id, sizeof (id), file );

	//Write out the checksum
	gi.FS_Write( &checksum, sizeof( checksum ), file );

	int	numNodes = m_nodes.size();

	//Write out the number of nodes to follow
	gi.FS_Write( &numNodes, sizeof(numNodes), file );

	//Write out all the nodes
	node_v::iterator	ni;

	STL_ITERATE( ni, m_nodes )
	{
		(*ni)->Save( numNodes, file );
	}

	//write out failed edges
	gi.FS_Write( &failedEdges, sizeof( failedEdges ), file );

	gi.FS_FCloseFile( file );

	return true;
}

/*
-------------------------
AddRawPoint
-------------------------
*/

int CNavigator::AddRawPoint( vec3_t point, int flags, int radius )
{
	CNode	*node	= CNode::Create( point, flags, radius, m_nodes.size() );

	if ( node == NULL )
	{
		Com_Error( ERR_DROP, "Error adding node!\n" );
		return -1;
	}
	
	//TODO: Validate the position
	//TODO: Correct stuck waypoints

	STL_INSERT( m_nodes, node );

	return node->GetID();
}

/*
-------------------------
GetEdgeCost
-------------------------
*/

int	CNavigator::GetEdgeCost( CNode *first, CNode *second )
{
	trace_t	trace;
	vec3_t	start, end;
	vec3_t	mins, maxs;

	//Setup the player size
	VectorSet( mins, -8, -8, -8 );
	VectorSet( maxs,  8,  8,  8 );

	//Setup the points
	first->GetPosition( start );
	second->GetPosition( end );

	gi.trace( &trace, start, mins, maxs, end, ENTITYNUM_NONE, MASK_SOLID, G2_NOCOLLIDE, 0 );

	if ( trace.fraction < 1.0f || trace.allsolid || trace.startsolid )
		return Q3_INFINITE; // return -1;

	//Connection successful, return the cost
	return Distance( start, end );
}

void CNavigator::SetEdgeCost( int ID1, int ID2, int cost )
{
	if( (ID1 == -1) || (ID2 == -1) )
	{//not valid nodes, must have come from the ClearAllFailedEdges initization-type calls
		return;
	}

	CNode	*node1 = m_nodes[ID1];
	CNode	*node2 = m_nodes[ID2];

	if ( cost == -1 )
	{//they want us to calc it
		//FIXME: can we just remember this instead of recalcing every time?
		vec3_t	pos1, pos2;
		
		node1->GetPosition( pos1 );
		node2->GetPosition( pos2 );
		cost = Distance( pos1, pos2 );
	}

	//set it
	node1->AddEdge( ID2, cost );
	node2->AddEdge( ID1, cost );
}

/*
-------------------------
AddNodeEdges
-------------------------
*/

void CNavigator::AddNodeEdges( CNode *node, int addDist, edge_l	&edgeList, bool *checkedNodes )
{
	//Add all edge
	for ( int i = 0; i < node->GetNumEdges(); i++ )
	{
		//Make sure we don't add an old edge twice
		if ( checkedNodes[ node->GetEdge( i ) ] == true )
			continue;

		//Get the node
		CNode	*nextNode = m_nodes[ node->GetEdge( i ) ];

		//This node has now been checked
		checkedNodes[ nextNode->GetID() ] = true;

		//Add it to the list
		STL_INSERT( edgeList, CEdge( nextNode->GetID(), node->GetID(), addDist + ( node->GetEdgeCost( i ) ) ) );
	}
}

/*
-------------------------
CalculatePath
-------------------------
*/

void CNavigator::CalculatePath( CNode *node )
{
	int	curRank = 0;
	int	i;

	CPriorityQueue	*pathList = new CPriorityQueue();
	BYTE			*checked;

	//Init the completion table
	checked = new BYTE[ m_nodes.size() ];
	memset( checked, 0, m_nodes.size() );

	//Mark this node as checked
	checked[ node->GetID() ] = true;
	node->AddRank( node->GetID(), curRank++ );

	//Add all initial nodes
	for ( i = 0; i < node->GetNumEdges(); i++ )
	{
		CNode	*nextNode = m_nodes[ node->GetEdge(i) ];
		assert(nextNode);

		checked[ nextNode->GetID() ] = true;

		pathList->Push( new CEdge( nextNode->GetID(), nextNode->GetID(), node->GetEdgeCost(i) ) );
	}

	float				minDist;
	CEdge				*test;	

	//Now flood fill all the others
	while ( !pathList->Empty() )
	{
		minDist = Q3_INFINITE;			
		test	 = pathList->Pop();
		
		CNode	*testNode = m_nodes[ (*test).m_first ];
		assert( testNode );

		node->AddRank( testNode->GetID(), curRank++ );

		//Add in all the new edges
		for ( i = 0; i < testNode->GetNumEdges(); i++ )
		{
			CNode	*addNode = m_nodes[ testNode->GetEdge(i) ];
			assert( addNode );

			if ( checked[ addNode->GetID() ] )
				continue;

			int	newDist = (*test).m_cost + testNode->GetEdgeCost(i);
			pathList->Push( new CEdge( addNode->GetID(), (*test).m_second, newDist ) );

			checked[ addNode->GetID() ] = true;
		}
		delete test;

	}

	node->RemoveFlag( NF_RECALC );

	delete pathList;
	delete [] checked;
}

/*
-------------------------
CalculatePaths
-------------------------
*/
extern void CP_FindCombatPointWaypoints( void );
void CNavigator::CalculatePaths( bool	recalc )
{
#ifndef FINAL_BUILD
	int	startTime = GetTime(0);
#endif
#if _HARD_CONNECT
#else
#endif	
	int i;

	for ( i = 0; i < m_nodes.size(); i++ )
	{
		//Allocate the needed memory
		m_nodes[i]->InitRanks( m_nodes.size() );
	}

	for ( i = 0; i < m_nodes.size(); i++ )
	{
		CalculatePath( m_nodes[i] );
	}
	
#ifndef FINAL_BUILD
	if ( pathsCalculated )
	{
		gi.Printf( S_COLOR_CYAN"%s recalced paths in %d ms\n", (NPC!=NULL?NPC->targetname:"NULL"), GetTime(startTime) );
	}
#endif
	
	if(!recalc)	//Mike says doesn't need to happen on recalc
	{
		CP_FindCombatPointWaypoints();
	}

	pathsCalculated = qtrue;
}

/*
-------------------------
ShowNodes
-------------------------
*/

void CNavigator::ShowNodes( void )
{
	node_v::iterator	ni;

	vec3_t	position;
	qboolean showRadius;
	float	dist,
			radius;

	STL_ITERATE( ni, m_nodes )
	{
		(*ni)->GetPosition( position );

		showRadius = qfalse;
		if( NAVDEBUG_showRadius )
		{	
			dist = DistanceSquared( g_entities[0].currentOrigin, position );
			radius = (*ni)->GetRadius();
			// if player within node radius or 256, draw radius (sometimes the radius is really small, so we check for 256 to catch everything)
			if( (dist <= radius*radius) || dist <= 65536 ) 
			{
				showRadius = qtrue;
			}
		}
		else
		{
			dist = DistanceSquared( g_entities[0].currentOrigin, position );
		}
		if ( dist < 1048576 )
		{
			if ( gi.inPVS( g_entities[0].currentOrigin, position ) )
			{
				(*ni)->Draw(showRadius);
			}
		}
	}
}

/*
-------------------------
ShowEdges
-------------------------
*/

typedef	map < int, bool >		drawMap_m;

void CNavigator::ShowEdges( void )
{
	node_v::iterator	ni;
	vec3_t	start, end;

	drawMap_m	*drawMap;

	drawMap = new drawMap_m[ m_nodes.size() ];

	STL_ITERATE( ni, m_nodes )
	{
		(*ni)->GetPosition( start );
		if ( DistanceSquared( g_entities[0].currentOrigin, start ) >= 1048576 )
		{
			continue;
		}

		if ( !gi.inPVSIgnorePortals( g_entities[0].currentOrigin, start ) )
		{
			continue;
		}

		//Attempt to draw each connection
		for ( int i = 0; i < (*ni)->GetNumEdges(); i++ )
		{
			int id = (*ni)->GetEdge( i );

			if ( id == -1 )
				continue;

			//Already drawn?
			if ( drawMap[(*ni)->GetID()].find( id ) != drawMap[(*ni)->GetID()].end() )
				continue;

			BYTE flags = (*ni)->GetEdgeFlags( i );

			CNode	*node = m_nodes[id];

			node->GetPosition( end );

			//Set this as drawn
			drawMap[id][(*ni)->GetID()] = true;

			if ( DistanceSquared( g_entities[0].currentOrigin, end ) >= 1048576 )
			{
				continue;
			}

			if ( !gi.inPVSIgnorePortals( g_entities[0].currentOrigin, end ) )
				continue;

			if ( EdgeFailed( id, (*ni)->GetID() ) != -1 )//flags & EFLAG_FAILED )
				CG_DrawEdge( start, end, EDGE_FAILED );
			else if ( flags & EFLAG_BLOCKED )
				CG_DrawEdge( start, end, EDGE_BLOCKED );
			else
				CG_DrawEdge( start, end, EDGE_NORMAL );
		}
	}

	delete [] drawMap;
}

int CNavigator::GetNodeRadius( int nodeID )
{
	if ( m_nodes.size() == 0 )
		return 0;
	return m_nodes[nodeID]->GetRadius();
}

void CNavigator::CheckBlockedEdges( void )
{
	CNode	*start, *end;
	vec3_t	p1, p2;
	int		flags, first, second;
	trace_t	trace;
	qboolean failed;
	int		edgeNum;
	node_v::iterator	ni;

	//Go through all edges and test the ones that were blocked
	STL_ITERATE( ni, m_nodes )
	{
		//Attempt to draw each connection
		for ( edgeNum = 0; edgeNum < (*ni)->GetNumEdges(); edgeNum++ )
		{
			flags = (*ni)->GetEdgeFlags( edgeNum );
			if ( (flags&EFLAG_BLOCKED) )
			{
				first	= (*ni)->GetID();
				second	= (*ni)->GetEdge( edgeNum );
				start	= m_nodes[first];
				end		= m_nodes[second];
				failed	= qfalse;

				start->GetPosition( p1 );
				end->GetPosition( p2 );

				//FIXME: can't we just store the trace.entityNum from the HardConnect trace?  So we don't have to do another trace here...
				gi.trace( &trace, p1, wpMins, wpMaxs, p2, ENTITYNUM_NONE, MASK_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP, G2_NOCOLLIDE, 0 );

				if ( trace.entityNum < ENTITYNUM_WORLD && (trace.fraction < 1.0f || trace.startsolid == qtrue || trace.allsolid == qtrue) )
				{//could be assumed, since failed before
					if ( G_EntIsDoor( trace.entityNum ) )
					{//door
						if ( !G_EntIsUnlockedDoor( trace.entityNum ) )
						{//locked door
							failed = qtrue;
						}
					}
					else
					{
						if ( G_EntIsBreakable( trace.entityNum ) )
						{//do same for breakable brushes/models/glass?
							failed = qtrue;
						}
						else if ( G_EntIsRemovableUsable( trace.entityNum ) )
						{
							failed = qtrue;
						}
						else if ( trace.allsolid || trace.startsolid )
						{//FIXME: the entitynum would be none here, so how do we know if this is stuck inside an ent or the world?
						}
						else
						{//FIXME: what about func_plats and scripted movers?
						}
					}
				}

				if ( failed )
				{
					//could add the EFLAG_FAILED to the two edges, but we stopped doing that since it was pointless
					AddFailedEdge( ENTITYNUM_NONE, first, second );
				}
			}
		}
	}
}

#if _HARD_CONNECT

/*
-------------------------
HardConnect
-------------------------
*/

void CNavigator::HardConnect( int first, int second )
{
	CNode	*start, *end;

	start	= m_nodes[first];
	end		= m_nodes[second];

	vec3_t	p1, p2;

	start->GetPosition( p1 );
	end->GetPosition( p2 );

	trace_t	trace;
	
	int		flags = EFLAG_NONE;

	gi.trace( &trace, p1, wpMins, wpMaxs, p2, ENTITYNUM_NONE, MASK_SOLID|CONTENTS_BOTCLIP|CONTENTS_MONSTERCLIP, G2_NOCOLLIDE, 0 );

	int cost = Distance( p1, p2 );

	if ( trace.fraction != 1.0f || trace.startsolid == qtrue || trace.allsolid == qtrue )
	{
		flags |= EFLAG_BLOCKED;
	}

	start->AddEdge( second, cost, flags );
	end->AddEdge( first, cost, flags );
}

#endif

/*
-------------------------
TestNodePath
-------------------------
*/

int CNavigator::TestNodePath( gentity_t *ent, int okToHitEntNum, vec3_t position, qboolean includeEnts )
{
	int clipmask = ent->clipmask;

	if ( !includeEnts )
	{
		clipmask &= ~CONTENTS_BODY;
	}
	//Check the path
	if ( NAV_ClearPathToPoint( ent, ent->mins, ent->maxs, position, clipmask, okToHitEntNum ) == false )
		return false;
	
	return true;
}

/*
-------------------------
TestNodeLOS
-------------------------
*/

int CNavigator::TestNodeLOS( gentity_t *ent, vec3_t position )
{
	return NPC_ClearLOS( ent, position );
}

/*
-------------------------
TestBestFirst
-------------------------
*/

int	CNavigator::TestBestFirst( gentity_t *ent, int lastID, int flags )
{
	//Must be a valid one to begin with
	if ( lastID == NODE_NONE )
		return NODE_NONE;

	if ( lastID >= m_nodes.size() )
		return NODE_NONE;

	//Get the info
	vec3_t	nodePos;
	CNode	*node = m_nodes[ lastID ];
	CNode	*testNode;
	int		numEdges = node->GetNumEdges();
	float	dist;

	node->GetPosition( nodePos );

	//Setup our last node as our root, and search for a closer one according to its edges	
	int bestNode = ( TestNodePath( ent, ENTITYNUM_NONE, nodePos, qtrue ) ) ? lastID : NODE_NONE;
	float bestDist = ( bestNode == NODE_NONE ) ? Q3_INFINITE : DistanceSquared( ent->currentOrigin, nodePos );

	//Test all these edges first
	for ( int i = 0; i < numEdges; i++ )
	{
		//Get this node and its distance
		testNode = m_nodes[ node->GetEdge(i) ];

		if ( NodeFailed( ent, testNode->GetID() ) )
		{
			continue;
		}

		testNode->GetPosition( nodePos );

		dist = DistanceSquared( ent->currentOrigin, nodePos );

		//Test against current best
		if ( dist < bestDist )
		{
			//See if this node is valid
			if ( CheckedNode(testNode->GetID(),ent->s.number) == CHECKED_PASSED || TestNodePath( ent, ENTITYNUM_NONE, nodePos, qtrue ) )
			{
				bestDist = dist;
				bestNode = testNode->GetID();
				SetCheckedNode(testNode->GetID(),ent->s.number,CHECKED_PASSED);
			}
			else
			{
				SetCheckedNode(testNode->GetID(),ent->s.number,CHECKED_FAILED);
			}
		}	
	}

	return bestNode;
}

/*
-------------------------
CollectNearestNodes
-------------------------
*/

#define	NODE_COLLECT_MAX	16		//Maximum # of nodes collected at any time
#define NODE_COLLECT_RADIUS	512		//Default radius to search for nodes in
#define NODE_COLLECT_RADIUS_SQR		( NODE_COLLECT_RADIUS * NODE_COLLECT_RADIUS )

int CNavigator::CollectNearestNodes( vec3_t origin, int radius, int maxCollect, nodeChain_l &nodeChain )
{
	node_v::iterator	ni;
	float				dist;
	vec3_t				position;
	int					collected = 0;
	bool				added = false;

	//Get a distance rating for each node in the system
	STL_ITERATE( ni, m_nodes )
	{
		//If we've got our quota, then stop looking
		//Get the distance to the node
		(*ni)->GetPosition( position );
		dist = DistanceSquared( position, origin );

		//Must be within our radius range
		if ( dist > (float) ( radius * radius ) )
			continue;
		
		nodeList_t				nChain;
		nodeChain_l::iterator	nci;

		//Always add the first node
		if ( nodeChain.size() == 0 )
		{
			nChain.nodeID = (*ni)->GetID();
			nChain.distance = dist;

			nodeChain.insert( nodeChain.begin(), nChain );
			continue;
		}

		added = false;

		//Compare it to what we already have
		STL_ITERATE( nci, nodeChain )
		{
			//If we're less, than this entry, then insert before it
			if ( dist < (*nci).distance )
			{
				nChain.nodeID = (*ni)->GetID();
				nChain.distance = dist;

				nodeChain.insert( nci, nChain );
				collected = nodeChain.size();
				added = true;
				
				//If we've hit our collection limit, throw off the oldest one
				if ( nodeChain.size() > maxCollect )
				{
					nodeChain.pop_back();
				}

				break;
			}
		}

		//Otherwise, always pad out the collection if possible so we don't miss anything
		if ( ( added == false ) && ( nodeChain.size() < maxCollect ) )
		{
			nChain.nodeID = (*ni)->GetID();
			nChain.distance = dist;

			nodeChain.insert( nodeChain.end(), nChain );
		}
	}

	return collected;
}

int CNavigator::GetBestPathBetweenEnts( gentity_t *ent, gentity_t *goal, int flags )
{
	//Must have nodes
	if ( m_nodes.size() == 0 )
		return NODE_NONE;

#define	MAX_Z_DELTA	18

	nodeChain_l				nodeChain;
	nodeChain_l::iterator	nci;
	nodeChain_l				nodeChain2;
	nodeChain_l::iterator	nci2;

	//Collect all nodes within a certain radius
	CollectNearestNodes( ent->currentOrigin, NODE_COLLECT_RADIUS, NODE_COLLECT_MAX, nodeChain );
	CollectNearestNodes( goal->currentOrigin, NODE_COLLECT_RADIUS, NODE_COLLECT_MAX, nodeChain2 );

	vec3_t				position;
	vec3_t				position2;
	int					radius;
	int					cost, pathCost, bestCost = Q3_INFINITE;
	CNode				*node, *node2;
	int					nodeNum, nodeNum2;
	int					nextNode = NODE_NONE, bestNode = NODE_NONE;
	int					nodeFlags = 0;
//	bool				recalc = false;

	ent->waypoint = NODE_NONE;
	goal->waypoint = NODE_NONE;

	//Look through all nodes
	STL_ITERATE( nci, nodeChain )
	{
		node = m_nodes[(*nci).nodeID];
		nodeNum = (*nci).nodeID;

		node->GetPosition( position );

		if ( CheckedNode(nodeNum,ent->s.number) == CHECKED_FAILED )
		{//already checked this node against ent and it failed
			continue;
		}
		if ( CheckedNode(nodeNum,ent->s.number) == CHECKED_PASSED )
		{//already checked this node against ent and it passed
		}
		else
		{//haven't checked this node against ent yet
			if ( NodeFailed( ent, nodeNum ) )
			{
				SetCheckedNode( nodeNum, ent->s.number, CHECKED_FAILED );
				continue;
			}
			//okay, since we only have to do this once, let's check to see if this node is even usable (could help us short-circuit a whole loop of the dest nodes)
			radius	= node->GetRadius();
			
			//If we're not within the known clear radius of this node OR out of Z height range...
			if ( (*nci).distance >= (radius*radius) || ( fabs( position[2] - ent->currentOrigin[2] ) >= MAX_Z_DELTA ) )
			{
				//We're not *within* this node, so check clear path, etc.

				//FIXME: any way to call G_FindClosestPointOnLineSegment and see if I can at least get to the waypoint's path
				if ( flags & NF_CLEAR_PATH )//|| flags & NF_CLEAR_LOS )
				{//need a clear path or LOS
					if ( !gi.inPVS( ent->currentOrigin, position ) )
					{//not even potentially clear
						SetCheckedNode( nodeNum, ent->s.number, CHECKED_FAILED );
						continue;
					}
				}

				//Do we need a clear path?
				if ( flags & NF_CLEAR_PATH )
				{
					if ( TestNodePath( ent, goal->s.number, position, qtrue ) == false )
					{
						SetCheckedNode( nodeNum, ent->s.number, CHECKED_FAILED );
						continue;
					}
				}
			}//otherwise, inside the node so it must be clear (?)
			SetCheckedNode( nodeNum, ent->s.number, CHECKED_PASSED );
		}

		if ( d_altRoutes->integer )
		{
			//calc the paths for this node if they're out of date
			nodeFlags = node->GetFlags();
			if ( (nodeFlags&NF_RECALC) )
			{
				//gi.Printf( S_COLOR_CYAN"%d recalcing paths from node %d\n", level.time, nodeNum );
				CalculatePath( node );
			}
		}

		STL_ITERATE( nci2, nodeChain2 )
		{
			node2 = m_nodes[(*nci2).nodeID];
			nodeNum2 = (*nci2).nodeID;
			if ( d_altRoutes->integer )
			{
				//calc the paths for this node if they're out of date
				nodeFlags = node2->GetFlags();
				if ( (nodeFlags&NF_RECALC) )
				{
					//gi.Printf( S_COLOR_CYAN"%d recalcing paths from node %d\n", level.time, nodeNum2 );
					CalculatePath( node2 );
				}
			}

			node2->GetPosition( position2 );
			//Okay, first get the entire path cost, including distance to first node from ents' positions
			cost = floor(Distance( ent->currentOrigin, position ) + Distance( goal->currentOrigin, position2 ));

			if ( d_altRoutes->integer )
			{
				nextNode = GetBestNodeAltRoute( (*nci).nodeID, (*nci2).nodeID, &pathCost, bestNode );
				cost += pathCost;
			}
			else
			{
				cost += GetPathCost( (*nci).nodeID, (*nci2).nodeID );
			}

			if ( cost >= bestCost )
			{
				continue;
			}

			//okay, this is the shortest path we've found yet, check clear path, etc.
			if ( CheckedNode( nodeNum2, goal->s.number ) == CHECKED_FAILED )
			{//already checked this node against goal and it failed
				continue;
			}
			if ( CheckedNode( nodeNum2, goal->s.number ) == CHECKED_PASSED )
			{//already checked this node against goal and it passed
			}
			else
			{//haven't checked this node against goal yet
				if ( NodeFailed( goal, nodeNum2 ) )
				{
					SetCheckedNode( nodeNum2, goal->s.number, CHECKED_FAILED );
					continue;
				}
				radius	= node2->GetRadius();
				
				//If we're not within the known clear radius of this node OR out of Z height range...
				if ( (*nci2).distance >= (radius*radius) || ( fabs( position2[2] - goal->currentOrigin[2] ) >= MAX_Z_DELTA ) )
				{
					//We're not *within* this node, so check clear path, etc.

					if ( flags & NF_CLEAR_PATH )//|| flags & NF_CLEAR_LOS )
					{//need a clear path or LOS
						if ( !gi.inPVS( goal->currentOrigin, position2 ) )
						{//not even potentially clear
							SetCheckedNode( nodeNum2, goal->s.number, CHECKED_FAILED );
							continue;
						}
					}
					//Do we need a clear path?
					if ( flags & NF_CLEAR_PATH )
					{
						if ( TestNodePath( goal, ent->s.number, position2, qfalse ) == false )//qtrue?
						{
							SetCheckedNode( nodeNum2, goal->s.number, CHECKED_FAILED );
							continue;
						}
					}
				}//otherwise, inside the node so it must be clear (?)
				SetCheckedNode( nodeNum2, goal->s.number, CHECKED_PASSED );
			}

			bestCost = cost;
			bestNode = nextNode;
			ent->waypoint = (*nci).nodeID;
			goal->waypoint = (*nci2).nodeID;
		}
	}

	if ( !d_altRoutes->integer )
	{//bestNode would not have been set by GetBestNodeAltRoute above, so get it here
		if ( ent->waypoint != NODE_NONE && goal->waypoint != NODE_NONE )
		{//have 2 valid waypoints which means a valid path
			bestNode = GetBestNodeAltRoute( ent->waypoint, goal->waypoint, &bestCost, NODE_NONE );
		}
	}
	return bestNode;
}

/*
-------------------------
GetNearestWaypoint
-------------------------
*/

int CNavigator::GetNearestNode( gentity_t *ent, int lastID, int flags, int targetID )
{
	int	bestNode = NODE_NONE;
	//Must have nodes
	if ( m_nodes.size() == 0 )
		return NODE_NONE;

	if ( targetID == NODE_NONE )
	{
		//Try and find an early match using our last node
		bestNode = TestBestFirst( ent, lastID, flags );

		if ( bestNode != NODE_NONE )
			return bestNode;
	}//else can't rely on testing last, we want best to targetID

/////////////////////////////////////////////////

#define	MAX_Z_DELTA	18

/////////////////////////////////////////////////

	nodeChain_l				nodeChain;
	nodeChain_l::iterator	nci;

	//Collect all nodes within a certain radius
	CollectNearestNodes( ent->currentOrigin, NODE_COLLECT_RADIUS, NODE_COLLECT_MAX, nodeChain );

	vec3_t				position;
	int					radius;
	int					dist, bestDist = Q3_INFINITE;
	CNode				*node;

	//Look through all nodes
	STL_ITERATE( nci, nodeChain )
	{
		node = m_nodes[(*nci).nodeID];

		node->GetPosition( position );

		radius	= node->GetRadius();
		
		if ( NodeFailed( ent, (*nci).nodeID ) )
		{
			continue;
		}
		//Are we within the known clear radius of this node?
		if ( (*nci).distance < (radius*radius) )
		{
			//Do a z-difference sanity check
			if ( fabs( position[2] - ent->currentOrigin[2] ) < MAX_Z_DELTA )
			{
				//Found one		
				return (*nci).nodeID;
			}
		}

		//We're not *within* this node, so...
		if ( CheckedNode((*nci).nodeID,ent->s.number) == CHECKED_FAILED )
		{
			continue;
		}
		else if ( CheckedNode((*nci).nodeID,ent->s.number) == CHECKED_FAILED )
		{
			continue;
		}
		else
		{
			//Do we need a clear path?
			if ( flags & NF_CLEAR_PATH )
			{
				if ( TestNodePath( ent, ENTITYNUM_NONE, position, qfalse ) == false )//qtrue?
				{
					SetCheckedNode((*nci).nodeID,ent->s.number,CHECKED_FAILED);
					continue;
				}
			}

			//Do we need a clear line of sight?
			/*
			if ( flags & NF_CLEAR_LOS )
			{
				if ( TestNodeLOS( ent, position ) == false )
				{
					nodeChecked[(*nci).nodeID][ent->s.number] = CHECKED_FAILED;
					continue;
				}
			}
			*/
			SetCheckedNode((*nci).nodeID,ent->s.number,CHECKED_PASSED);
		}

		if ( targetID != WAYPOINT_NONE )
		{//we want to find the one with the shortest route here
			dist = GetPathCost( (*nci).nodeID, targetID );
			if ( dist < bestDist )
			{
				bestDist = dist;
				bestNode = (*nci).nodeID;
			}
		}
		else
		{//first one we find is fine
			bestNode = (*nci).nodeID;
			break;
		}
	}

	//Found one, we're done
	return bestNode;
}

/*
-------------------------
ShowPath
-------------------------
*/

void CNavigator::ShowPath( int start, int end )
{	
	//Validate the start position
	if ( ( start < 0 ) || ( start >= m_nodes.size() ) )
		return;

	//Validate the end position
	if ( ( end < 0 ) || ( end >= m_nodes.size() ) )
		return;

	CNode	*startNode	= m_nodes[ start ];
	CNode	*endNode	= m_nodes[ end ];
	
	CNode	*moveNode = startNode;
	CNode	*testNode = NULL;

	int		bestNode;
	vec3_t	startPos, endPos;

	int		runAway = 0;

	//Draw out our path
	while ( moveNode != endNode )
	{
		bestNode = GetBestNode( moveNode->GetID(), end );
		
		//Some nodes may be fragmented
		if ( bestNode == -1 )
		{
			Com_Printf("No connection possible between node %d and %d\n", start, end );
			return;
		}

		//This is our next node on the path
		testNode = m_nodes[ bestNode ];

		//Get their origins
		moveNode->GetPosition( startPos );
		testNode->GetPosition( endPos );
		
		//Draw the edge
		CG_DrawEdge( startPos, endPos, EDGE_PATH );

		//Take a new best node
		moveNode = testNode;

		if ( runAway++ > 64 )
		{
			Com_Printf("Potential Run-away path!\n");
			return;
		}
	}
}

static map<int,byte> CheckedNodes;
void CNavigator::ClearCheckedNodes( void )
{
	CheckedNodes.clear();
}

byte CNavigator::CheckedNode(int wayPoint,int ent)
{
	assert(wayPoint>=0&&wayPoint<MAX_STORED_WAYPOINTS);
	assert(ent>=0&&ent<MAX_GENTITIES);
	map<int,byte>::iterator f=CheckedNodes.find(wayPoint*MAX_GENTITIES+ent);
	if (f!=CheckedNodes.end())
	{
		return (*f).second;
	}
	return CHECKED_NO;
}

void CNavigator::SetCheckedNode(int wayPoint,int ent,byte value)
{
	assert(wayPoint>=0&&wayPoint<MAX_STORED_WAYPOINTS);
	assert(ent>=0&&ent<MAX_GENTITIES);
	assert(value==CHECKED_FAILED||value==CHECKED_PASSED);
	CheckedNodes[wayPoint*MAX_GENTITIES+ent]=value;
}

#define	CHECK_FAILED_EDGE_INTERVAL	1000
#define	CHECK_FAILED_EDGE_INTITIAL	5000//10000

void CNavigator::CheckFailedNodes( gentity_t *ent )
{
	vec3_t	nodePos;
	int		j;

	//Must have nodes
	if ( m_nodes.size() == 0 )
		return;

	if ( ent->failedWaypointCheckTime && ent->failedWaypointCheckTime < level.time )
	{
		int failed = 0;
		//do this only once every 1 second
		for ( j = 0; j < MAX_FAILED_NODES; j++ )
		{
			if ( ent->failedWaypoints[j] != 0 )
			{
				failed++;
				//-1 because 0 is a valid node but also the default, so we add one when we add one
				m_nodes[ent->failedWaypoints[j]-1]->GetPosition( nodePos );
				if ( !NAV_ClearPathToPoint( ent, ent->mins, ent->maxs, nodePos, (CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP), ENTITYNUM_NONE ) )
				{//no path clear of architecture, so clear this since we can't check against entities
					ent->failedWaypoints[j] = 0;
					failed--;
				}
				//have clear architectural path, now check against ents only
				else if ( NAV_ClearPathToPoint( ent, ent->mins, ent->maxs, nodePos, CONTENTS_BODY, ENTITYNUM_NONE ) )
				{//clear of ents, too, so all clear, clear this one out
					ent->failedWaypoints[j] = 0;
					failed--;
				}
			}
		}
		if ( !failed )
		{
			ent->failedWaypointCheckTime = 0;
		}
		else
		{
			ent->failedWaypointCheckTime = level.time + CHECK_FAILED_EDGE_INTERVAL + Q_irand( 0, 1000 );
		}
	}
}

void CNavigator::AddFailedNode( gentity_t *ent, int nodeID )
{
	int j;
	for ( j = 0; j < MAX_FAILED_NODES; j++ )
	{
		if ( ent->failedWaypoints[j] == 0 )
		{
			ent->failedWaypoints[j] = nodeID+1;//+1 because 0 is the default value and that's a valid node, so we take the +1 out when we check the node above
			if ( !ent->failedWaypointCheckTime )
			{
				ent->failedWaypointCheckTime = level.time + CHECK_FAILED_EDGE_INTITIAL;
			}
			return;
		}
		if ( ent->failedWaypoints[j] == nodeID+1 )
		{//already have this one marked as failed
			return;
		}
	}
	if ( j == MAX_FAILED_NODES )//check not needed, but...
	{//ran out of failed nodes, get rid of first one, shift rest up
		for ( j = 0; j < MAX_FAILED_NODES-1; j++ )
		{
			ent->failedWaypoints[j] = ent->failedWaypoints[j+1];
		}
	}
	ent->failedWaypoints[MAX_FAILED_NODES-1] = nodeID+1;
	if ( !ent->failedWaypointCheckTime )
	{
		ent->failedWaypointCheckTime = level.time + CHECK_FAILED_EDGE_INTITIAL;
	}
}

qboolean CNavigator::NodeFailed( gentity_t *ent, int nodeID )
{
	for ( int j = 0; j < MAX_FAILED_NODES; j++ )
	{
		if ( (ent->failedWaypoints[j]-1) == nodeID )
		{
			return qtrue;
		}
	}
	return qfalse;
}

qboolean CNavigator::NodesAreNeighbors( int startID, int endID )
{//See if these 2 are neighbors
	if ( startID == endID )
	{
		return qfalse;
	}

	CNode *start = m_nodes[startID];
	int	nextID = -1;
	//NOTE: we only check start because we assume all connections are 2-way
	for ( int i = 0; i < start->GetNumEdges(); i++ )
	{
		nextID = start->GetEdge(i);
		if ( nextID == endID )
		{
			return qtrue;
		}
	}
	//not neighbors
	return qfalse;
}

void CNavigator::ClearFailedEdge( failedEdge_t *failedEdge )
{
	if ( !failedEdge )
	{
		return;
	}

	//clear the edge failed flags
	/*
	CNode *node = m_nodes[failedEdge->startID];
	int edgeNum = node->GetEdgeNumToNode( failedEdge->endID );
	int	flags;
	if ( edgeNum != -1 )
	{
		flags = node->GetEdgeFlags( edgeNum )&~EFLAG_FAILED;
		node->SetEdgeFlags( edgeNum, flags );
	}
	node = m_nodes[failedEdge->endID];
	edgeNum = node->GetEdgeNumToNode( failedEdge->startID );
	if ( edgeNum != -1 )
	{
		flags = node->GetEdgeFlags( edgeNum )&~EFLAG_FAILED;
		node->SetEdgeFlags( edgeNum, flags );
	}
	*/
	//clear failedEdge info
	SetEdgeCost( failedEdge->startID, failedEdge->endID, -1 );
	failedEdge->startID = failedEdge->endID = WAYPOINT_NONE;
	failedEdge->entID = ENTITYNUM_NONE;
	failedEdge->checkTime = 0;
}

void CNavigator::ClearAllFailedEdges( void )
{
	memset( &failedEdges, WAYPOINT_NONE, sizeof( failedEdges ) );
	for ( int j = 0; j < MAX_FAILED_EDGES; j++ )
	{
		ClearFailedEdge( &failedEdges[j] );
	}
}

int CNavigator::EdgeFailed( int startID, int endID )
{	
	//OPTIMIZED WAY  (bjg 01/02)
	//find in lookup map
	pair <EdgeMultimapIt, EdgeMultimapIt> findValue;
	findValue = m_edgeLookupMap.equal_range(startID);	
	while ( findValue.first != findValue.second ) 
	{
		if( failedEdges[findValue.first->second].endID == endID)
		{	
			return findValue.first->second;		
		}
		++findValue.first;
	}
	findValue = m_edgeLookupMap.equal_range(endID);	
	while ( findValue.first != findValue.second ) 
	{
		if( failedEdges[findValue.first->second].endID == startID)
		{	
			return findValue.first->second;		
		}
		++findValue.first;
	}

	return -1;

	//Old way (linear search)
	/*
	for ( int j = 0; j < MAX_FAILED_EDGES; j++ )
	{
		if ( failedEdges[j].startID == startID )
		{
			if ( failedEdges[j].endID == endID )
			{
				return j;
			}
		}
		else if ( failedEdges[j].startID == endID )
		{
			if ( failedEdges[j].endID == startID )
			{
				return j;
			}
		}
	}
	return -1;
	*/
}

void CNavigator::AddFailedEdge( int entID, int startID, int endID )
{
	int	j;//, nextID;

	//Must have nodes
	if ( m_nodes.size() == 0 )
		return;

	if ( d_patched->integer )
	{//use patch-style navigation
		if ( startID == endID )
		{//not an edge!
			return;
		}
	}

	//Validate the ent number
	if ( ( entID < 0 ) || ( entID > ENTITYNUM_NONE ) )
	{
#ifndef FINAL_BUILD
		gi.Printf( S_COLOR_RED"NAV ERROR: envalid ent %d\n", entID );
		assert(0&&"invalid entID");
#endif
		return;
	}

	//Validate the start position
	if ( ( startID < 0 ) || ( startID >= m_nodes.size() ) )
	{
#ifndef FINAL_BUILD
		gi.Printf( S_COLOR_RED"NAV ERROR: tried to fail invalid waypoint %d\n", startID );
		assert(0&&"invalid failed edge");
#endif
		return;
	}

	//Validate the end position
	if ( ( endID < 0 ) || ( endID >= m_nodes.size() ) )
	{
#ifndef FINAL_BUILD
		gi.Printf( S_COLOR_RED"NAV ERROR: tried to fail invalid waypoint %d\n", endID );
		assert(0&&"invalid failed edge");
#endif
		return;
	}

	//First see if we already have this one
	if ( (j = EdgeFailed( startID, endID )) != -1 )
	{
		//just remember this guy instead
		failedEdges[j].entID = entID;
		return;
	}

	//Okay, new one, find an empty slot
	for ( j = 0; j < MAX_FAILED_EDGES; j++ )
	{
		if ( failedEdges[j].startID == WAYPOINT_NONE )
		{
			failedEdges[j].startID = startID;
			failedEdges[j].endID = endID;
			//Check one second from now to see if it's clear
			failedEdges[j].checkTime = level.time + CHECK_FAILED_EDGE_INTERVAL + Q_irand( 0, 1000 );

			m_edgeLookupMap.insert(pair<int, int>(startID, j));

			/*
			//DISABLED this for now, makes people stand around too long when
			//			collision avoidance just wasn't at it's best but path is clear
			CNode *start = m_nodes[startID];
			CNode *end = m_nodes[endID];

			for ( int i = 0; i < start->GetNumEdges(); i++ )
			{
				nextID = start->GetEdge(i);

				if ( EdgeFailed( startID, nextID ) != -1 )
				{
					//This edge blocked, check next
					continue;
				}

				if ( nextID == endID || end->GetRank( nextID ) >= 0 )
				{//neighbor of or route to end
					//There's an alternate route, so don't check this one for 10 seconds
					failedEdges[j].checkTime = level.time + CHECK_FAILED_EDGE_INTITIAL;
					break;
				}
			}
			*/
			
			//Remember who needed it
			failedEdges[j].entID = entID;

			//set the edge failed flags
			/*
			CNode *node = m_nodes[startID];
			int edgeNum = node->GetEdgeNumToNode( endID );
			int	flags;
			if ( edgeNum != -1 )
			{
				flags = node->GetEdgeFlags( edgeNum )|EFLAG_FAILED;
				node->SetEdgeFlags( edgeNum, flags );
			}
			node = m_nodes[endID];
			edgeNum = node->GetEdgeNumToNode( startID );
			if ( edgeNum != -1 )
			{
				flags = node->GetEdgeFlags( edgeNum )|EFLAG_FAILED;
				node->SetEdgeFlags( edgeNum, flags );
			}
			*/

			//stuff the index to this one in our lookup map

			//now recalc all the paths!
			if ( pathsCalculated )
			{
				//reconnect the nodes and mark every node's flag NF_RECALC
				//gi.Printf( S_COLOR_CYAN"%d marking all nodes for recalc\n", level.time );
				SetEdgeCost( startID, endID, Q3_INFINITE );
				FlagAllNodes( NF_RECALC );
			}
			return;
		}
	}

#ifndef FINAL_BUILD
	gi.Printf( S_COLOR_RED"NAV ERROR: too many blocked waypoint connections (%d)!!!\n", j );
#endif
}

qboolean CNavigator::CheckFailedEdge( failedEdge_t *failedEdge )
{
	if ( !failedEdge )
	{
		return qfalse;
	}

	//Every 1 second, see if our failed edges are clear
	if ( failedEdge->checkTime < level.time )
	{
		if ( failedEdge->startID != WAYPOINT_NONE )
		{
			vec3_t		start, end, mins, maxs;
			int			ignore, clipmask;
			gentity_t	*ent = (failedEdge->entID<ENTITYNUM_WORLD)?&g_entities[failedEdge->entID]:NULL;
			int			hitEntNum;

			if ( !ent || !ent->inuse || !ent->client || ent->health <= 0 )
			{
				VectorSet( mins, DEFAULT_MINS_0, DEFAULT_MINS_1, DEFAULT_MINS_2+STEPSIZE );
				VectorSet( maxs, DEFAULT_MAXS_0, DEFAULT_MAXS_1, DEFAULT_MAXS_2 );
				ignore = ENTITYNUM_NONE;
				clipmask = MASK_NPCSOLID;
			}
			else
			{
				VectorCopy( ent->mins, mins );
				mins[2] += STEPSIZE;
				VectorCopy( ent->maxs, maxs );
				ignore = failedEdge->entID;
				clipmask = ent->clipmask;
			}

			if ( maxs[2] < mins[2] )
			{//don't invert bounding box
				maxs[2] = mins[2];
			}

			m_nodes[failedEdge->startID]->GetPosition( start );
			m_nodes[failedEdge->endID]->GetPosition( end );

			//See if it's NAV_ClearPath...
#if 0
			hitEntNum = NAVNEW_ClearPathBetweenPoints( start, end, mins, maxs, ignore, clipmask|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP );//NOTE: should we really always include monsterclip (physically blocks NPCs) and botclip (do not enter)?
#else
			trace_t	trace;

			//Test if they're even conceivably close to one another
			if ( !gi.inPVSIgnorePortals( start, end ) )
			{
				return qfalse;
			}

			gi.trace( &trace, start, mins, maxs, end, ignore, clipmask|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP, G2_NOCOLLIDE, 0 );//NOTE: should we really always include monsterclip (physically blocks NPCs) and botclip (do not enter)?

			if( trace.startsolid == qtrue || trace.allsolid == qtrue )
			{
				return qfalse;
			}
			hitEntNum = trace.entityNum;
#endif
			//if we did hit something, see if it's just an auto-door and allow it
			if ( hitEntNum != ENTITYNUM_NONE && G_EntIsUnlockedDoor( hitEntNum ) )
			{
				hitEntNum = ENTITYNUM_NONE;
			}
			else if ( hitEntNum == failedEdge->entID )
			{//don't hit the person who initially marked the edge failed
				hitEntNum = ENTITYNUM_NONE;
			}
			if ( hitEntNum == ENTITYNUM_NONE )
			{
				//If so, clear it
				ClearFailedEdge( failedEdge );
				return qtrue;
			}
			else
			{
				//Check again in one second
				failedEdge->checkTime = level.time + CHECK_FAILED_EDGE_INTERVAL + Q_irand( 0, 1000 );
			}
		}
	}
	return qfalse;
}

void CNavigator::CheckAllFailedEdges( void )
{
	failedEdge_t	*failedEdge;
	qboolean		clearedAny = qfalse;

	//Must have nodes
	if ( m_nodes.size() == 0 )
		return;

	for ( int j = 0; j < MAX_FAILED_EDGES; j++ )
	{
		failedEdge = &failedEdges[j];

		clearedAny = CheckFailedEdge( failedEdge )?qtrue:clearedAny;
	}
	if ( clearedAny )
	{//need to recalc the paths
		if ( pathsCalculated )
		{
			//reconnect the nodes and mark every node's flag NF_RECALC
			//gi.Printf( S_COLOR_CYAN"%d marking all nodes for recalc\n", level.time );
			FlagAllNodes( NF_RECALC );
		}
	}
}

qboolean CNavigator::RouteBlocked( int startID, int testEdgeID, int endID, int rejectRank )
{
	int		nextID, edgeID, lastID, bestNextID = NODE_NONE;
	int		bestRank = rejectRank;
	int		testRank;
	qboolean	allEdgesFailed;
	CNode	*end;
	CNode	*next;


	if ( EdgeFailed( startID, testEdgeID ) != -1 )
	{
		return qtrue;
	}

	if ( testEdgeID == endID )
	{//Neighbors, checked out, all clear
		return qfalse;
	}
	
	//Okay, first edge is clear, now check rest of route!
	end	= m_nodes[ endID ];
	nextID = testEdgeID;
	lastID = startID;

	while( 1 )
	{
		next = m_nodes[ nextID ];
		allEdgesFailed = qtrue;

		for ( int i = 0; i < next->GetNumEdges(); i++ )
		{
			edgeID = next->GetEdge(i);

			if ( edgeID == lastID )
			{//Don't backtrack
				continue;
			}

			if ( edgeID == startID )
			{//Don't loop around
				continue;
			}

			if ( EdgeFailed( nextID, edgeID ) != -1 )
			{
				//This edge blocked, check next
				continue;
			}

			if ( edgeID == endID )
			{//We got there all clear!
				return qfalse;
			}

			//Still going...
			testRank = end->GetRank( edgeID );

			if ( testRank < 0 )
			{//No route this way
				continue;
			}

			//Is the rank good enough?
			if ( testRank < bestRank )
			{
				bestNextID = edgeID;
				bestRank = testRank;
				allEdgesFailed = qfalse;
			}
		}
		
		if ( allEdgesFailed )
		{
			//This route has no clear way of getting to end
			return qtrue;
		}
		else
		{
			lastID = nextID;
			nextID = bestNextID;
		}
	}
}

/*
-------------------------
GetBestNodeAltRoute
-------------------------
*/

int CNavigator::GetBestNodeAltRoute( int startID, int endID, int *pathCost, int rejectID )
{
	//Must have nodes
	if ( m_nodes.size() == 0 )
		return WAYPOINT_NONE;

	//Validate the start position
	if ( ( startID < 0 ) || ( startID >= m_nodes.size() ) )
		return WAYPOINT_NONE;

	//Validate the end position
	if ( ( endID < 0 ) || ( endID >= m_nodes.size() ) )
		return WAYPOINT_NONE;

	//Is it the same node?
	if ( startID == endID )
	{
		if ( !d_altRoutes->integer || EdgeFailed( startID, endID ) == -1 )
		{
			return startID;
		}
		else
		{
			return WAYPOINT_NONE;
		}
	}

	CNode	*start	= m_nodes[ startID ];

	int		bestNode = -1;
	int		bestRank = Q3_INFINITE;
	int		testRank, rejectRank = Q3_INFINITE;
	int		bestCost = Q3_INFINITE;

	*pathCost = 0;

	//Find the minimum rank of the edge(s) we want to reject as paths
	if ( rejectID != WAYPOINT_NONE )
	{
		for ( int i = 0; i < start->GetNumEdges(); i++ )
		{
			if ( start->GetEdge(i) == rejectID )
			{
				rejectRank = GetPathCost( startID, endID );//end->GetRank( start->GetEdge(i) );
				break;
			}
		}
	}

	for ( int i = 0; i < start->GetNumEdges(); i++ )
	{
		int	edgeID = start->GetEdge(i);

		testRank = GetPathCost( edgeID, endID );//end->GetRank( edgeID );

		//Make sure it's not worse than our reject rank
		if ( testRank >= rejectRank )
			continue;

		//Found one
		if ( edgeID == endID )
		{
			if ( !d_altRoutes->integer || !RouteBlocked( startID, edgeID, endID, rejectRank ) )
			{
				*pathCost += start->GetEdgeCost( i );
				return edgeID;
			}
			else
			{//this is blocked, can't consider it
				continue;
			}
		}

		//No possible connection
		if ( testRank == NODE_NONE )
		{
			*pathCost = Q3_INFINITE;
			return NODE_NONE;
		}

		//Found a better one
		if ( testRank < bestRank )
		{
			//FIXME: make sure all the edges down from startID through edgeID to endID
			//		does NOT include a failedEdge...
			if ( !d_altRoutes->integer || !RouteBlocked( startID, edgeID, endID, rejectRank ) )
			{
				bestNode = edgeID;
				bestRank = testRank;
				bestCost = start->GetEdgeCost(i)+testRank;
			}
		}
	}

	*pathCost = bestCost;

	return bestNode;
}
/*
-------------------------
GetBestNodeAltRoute
overloaded so you don't have to pass a pathCost int pointer in
-------------------------
*/

int CNavigator::GetBestNodeAltRoute( int startID, int endID, int rejectID )
{
	int	junk;
	return GetBestNodeAltRoute( startID, endID, &junk, rejectID );
}
/*
-------------------------
GetBestNode
-------------------------
*/

int CNavigator::GetBestNode( int startID, int endID, int rejectID )
{
	//Validate the start position
	if ( ( startID < 0 ) || ( startID >= m_nodes.size() ) )
		return WAYPOINT_NONE;

	//Validate the end position
	if ( ( endID < 0 ) || ( endID >= m_nodes.size() ) )
		return WAYPOINT_NONE;

	if ( startID == endID )
		return startID;

	CNode	*start	= m_nodes[ startID ];
	CNode	*end	= m_nodes[ endID ];

	int		bestNode = -1;
	int		bestRank = Q3_INFINITE;
	int		testRank, rejectRank = 0;

	if ( rejectID != WAYPOINT_NONE )
	{
		for ( int i = 0; i < start->GetNumEdges(); i++ )
		{
			if ( start->GetEdge(i) == rejectID )
			{
				rejectRank = end->GetRank( start->GetEdge(i) );
				break;
			}
		}
	}

	for ( int i = 0; i < start->GetNumEdges(); i++ )
	{
		int	edgeID = start->GetEdge(i);

		//Found one
		if ( edgeID == endID )
			return edgeID;

		testRank = end->GetRank( edgeID );

		//Found one
		if ( testRank <= rejectRank )
			continue;

		//No possible connection
		if ( testRank == NODE_NONE )
			return NODE_NONE;

		//Found a better one
		if ( testRank < bestRank )
		{
			bestNode = edgeID;
			bestRank = testRank;
		}
	}

	return bestNode;
}

/*
-------------------------
GetNodePosition
-------------------------
*/

int CNavigator::GetNodePosition( int nodeID, vec3_t out )
{
	//Validate the number
	if ( ( nodeID < 0 ) || ( nodeID >= m_nodes.size() ) )
		return false;

	CNode	*node = m_nodes[ nodeID ];

	node->GetPosition( out );

	return true;
}

/*
-------------------------
GetNodeNumEdges
-------------------------
*/

int CNavigator::GetNodeNumEdges( int nodeID )
{
	if ( ( nodeID < 0 ) || ( nodeID >=  m_nodes.size() ) )
		return -1;

	CNode	*node = m_nodes[ nodeID ];

	assert( node );

	return node->GetNumEdges();
}

/*
-------------------------
GetNodeEdge
-------------------------
*/

int CNavigator::GetNodeEdge( int nodeID, int edge )
{
	if ( ( nodeID < 0 ) || ( nodeID >=  m_nodes.size() ) )
		return -1;

	CNode	*node = m_nodes[ nodeID ];

	assert( node );

	return node->GetEdge( edge );
}

/*
-------------------------
Connected
-------------------------
*/

bool CNavigator::Connected( int startID, int endID )
{
	//Validate the start position
	if ( ( startID < 0 ) || ( startID >= m_nodes.size() ) )
		return false;

	//Validate the end position
	if ( ( endID < 0 ) || ( endID >= m_nodes.size() ) )
		return false;

	if ( startID == endID )
		return true;

	CNode	*start	= m_nodes[ startID ];
	CNode	*end	= m_nodes[ endID ];

	for ( int i = 0; i < start->GetNumEdges(); i++ )
	{
		int	edgeID = start->GetEdge(i);

		//Found one
		if ( edgeID == endID )
			return true;

		if ( ( end->GetRank( edgeID ) ) != NODE_NONE )
			return true;
	}

	return false;
}

/*
-------------------------
GetPathCost
-------------------------
*/

unsigned int CNavigator::GetPathCost( int startID, int endID )
{
	//Validate the start position
	if ( ( startID < 0 ) || ( startID >= m_nodes.size() ) )
		return Q3_INFINITE; // return 0;

	//Validate the end position
	if ( ( endID < 0 ) || ( endID >= m_nodes.size() ) )
		return Q3_INFINITE; // return 0;

	CNode	*startNode	= m_nodes[ startID ];

	if ( !startNode->GetNumEdges() )
	{//WTF?  Solitary waypoint!  Bad designer!
		return Q3_INFINITE; // return 0;
	}

	CNode	*endNode	= m_nodes[ endID ];
	
	CNode	*moveNode = startNode;

	int		bestNode;
	int		pathCost = 0;
	int		bestCost;

	int		bestRank;
	int		testRank;

	//Draw out our path
	while ( moveNode != endNode )
	{
		bestRank = WORLD_SIZE;
		bestNode = -1;
		bestCost = 0;

		for ( int i = 0; i < moveNode->GetNumEdges(); i++ )
		{
			int	edgeID = moveNode->GetEdge(i);

			//Done
			if ( edgeID == endID )
			{
				return pathCost + moveNode->GetEdgeCost( i );
			}	

			testRank = endNode->GetRank( edgeID );

			//No possible connection
			if ( testRank == NODE_NONE )
			{
				return Q3_INFINITE; // return 0;
			}

			//Found a better one
			if ( testRank < bestRank )
			{
				bestNode = edgeID;
				bestRank = testRank;
				bestCost = moveNode->GetEdgeCost( i );
			}
		}

		pathCost += bestCost;

		//Take a new best node
		moveNode = m_nodes[ bestNode ];
	}

	return pathCost;
}

/*
-------------------------
GetEdgeCost
-------------------------
*/

unsigned int CNavigator::GetEdgeCost( int startID, int endID )
{
	//Validate the start position
	if ( ( startID < 0 ) || ( startID >= m_nodes.size() ) )
		return Q3_INFINITE; // return 0;

	//Validate the end position
	if ( ( endID < 0 ) || ( endID >= m_nodes.size() ) )
		return Q3_INFINITE; // return 0;

	CNode	*start	= m_nodes[startID];
	CNode	*end	= m_nodes[endID];

	return GetEdgeCost( start, end );
}

/*
-------------------------
GetProjectedNode
-------------------------
*/

int CNavigator::GetProjectedNode( vec3_t origin, int nodeID )
{
	//Validate the start position
	if ( ( nodeID < 0 ) || ( nodeID >= m_nodes.size() ) )
		return NODE_NONE;

	CNode	*node = m_nodes[nodeID];
	CNode	*tempNode;

	float	bestDot		= 0.0f;
	int		bestNode	= NODE_NONE;

	vec3_t	targetDir, basePos, tempDir, tempPos;
	float	dot;

	//Setup our target direction
	node->GetPosition( basePos );

	VectorSubtract( origin, basePos, targetDir );
	VectorNormalize( targetDir );

	//Go through all the edges
	for ( int i = 0; i < node->GetNumEdges(); i++ )
	{
		tempNode = m_nodes[node->GetEdge(i)];
		tempNode->GetPosition( tempPos );

		VectorSubtract( tempPos, basePos, tempDir );
		VectorNormalize( tempDir );	//FIXME: Retain the length here if you want it

		dot = DotProduct( targetDir, tempDir );

		if ( dot < 0.0f )
			continue;

		if ( dot > bestDot )
		{
			bestDot		= dot;
			bestNode	= tempNode->GetID();
		}
	}

	return bestNode;
}

// This is the PriorityQueue stuff for lists of connections
// better than linear		(1/21/02 BJG)
//////////////////////////////////////////////////////////////////
// Helper pop_mHeap algorithm class
//////////////////////////////////////////////////////////////////
class NodeTotalGreater 
{
public:
   bool operator()( CEdge * first, CEdge * second ) const {
      return( first->m_cost > second->m_cost );
   }
};


//////////////////////////////////////////////////////////////////
// Destructor - Deallocate any remaining pointers in the queue
//////////////////////////////////////////////////////////////////
CPriorityQueue::~CPriorityQueue()
{
	while (!Empty())
	{
		delete Pop();
	}
}

//////////////////////////////////////////////////////////////////
// Standard Iterative Search
//////////////////////////////////////////////////////////////////
CEdge* CPriorityQueue::Find(int npNum)
{
	for(vector<CEdge*>::iterator HeapIter=mHeap.begin(); HeapIter!=mHeap.end(); HeapIter++)
	{
		if ((*HeapIter)->m_first == npNum)
		{
			return *HeapIter;
		}
	}
	return 0;
}

//////////////////////////////////////////////////////////////////
// Remove Node And Resort
//////////////////////////////////////////////////////////////////
CEdge* CPriorityQueue::Pop()
{  
   CEdge *edge = mHeap.front();

   //pop_mHeap will move the node at the front to the position N
   //and then sort the mHeap to make positions 1 through N-1 correct
   //(STL makes no assumptions about your data and doesn't want to change
   //the size of the container.)
   std::pop_heap(mHeap.begin(), mHeap.end(), NodeTotalGreater() );

   //pop_back() will actually remove the last element from the mHeap
   //now the mHeap is sorted for positions 1 through N
   mHeap.pop_back();

   return( edge );
}

//////////////////////////////////////////////////////////////////
// Add New Node And Resort
//////////////////////////////////////////////////////////////////
void CPriorityQueue::Push(CEdge* theEdge )
{    
   //Pushes the node onto the back of the mHeap
   mHeap.push_back( theEdge );

   //Sorts the new element into the mHeap
   std::push_heap( mHeap.begin(), mHeap.end(), NodeTotalGreater() );
}

//////////////////////////////////////////////////////////////////
// Find The Node In Question And Resort mHeap Around It
//////////////////////////////////////////////////////////////////
void CPriorityQueue::Update( CEdge* edge )
{  
   for(vector<CEdge*>::iterator i=mHeap.begin(); i!=mHeap.end(); i++)
   {
      if( (*i)->m_first == edge->m_first )
      {  //Found node - resort from this position in the mHeap
         //(its total value was changed before this function was called)
         std::push_heap( mHeap.begin(), i+1, NodeTotalGreater() );
         return;
      }
   }
}

//////////////////////////////////////////////////////////////////
// Just a wrapper for stl empty function.
//////////////////////////////////////////////////////////////////
bool CPriorityQueue::Empty()
{
   return( mHeap.empty() );
};

