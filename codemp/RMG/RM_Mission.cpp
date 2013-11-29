//Anything above this #include will be ignored by the compiler
#include "qcommon/exe_headers.h"

/************************************************************************************************
 *
 * RM_Mission.cpp
 *
 * implements the CRMMission class.  The CRMMission class loads and manages an arioche mission
 *
 ************************************************************************************************/

#include "RM_Headers.h"

#define ARIOCHE_CLIPBRUSH_SIZE	300
#define	CVAR_OBJECTIVE	0

/************************************************************************************************
 * CRMMission::CRMMission
 *	constructor
 *
 * inputs:
 *  none
 *
 * return:
 *	none
 *
 ************************************************************************************************/
CRMMission::CRMMission ( CRandomTerrain* landscape )
{
	mCurrentObjective		= NULL;
	mValidPaths				= false;
	mValidRivers			= false;
	mValidNodes				= false;
	mValidWeapons			= false;
	mValidAmmo				= false;
	mValidObjectives		= false;
	mValidInstances			= false;
	mTimeLimit				= 0;
	mMaxInstancePosition	= 1;
	mAccuracyMultiplier		= 1.0f;
	mHealthMultiplier		= 1.0f;
	mPickupHealth			= 1.0f;
	mPickupArmor			= 1.0f;
	mPickupAmmo				= 1.0f;
	mPickupWeapon			= 1.0f;
	mPickupEquipment		= 1.0f;
	
	mDefaultPadding = 0;
	mSymmetric = SYMMETRY_NONE;

//	mCheckedEnts.clear();

	mLandScape = landscape;

	// cut down the possible area that is 'legal' for area manager to use by 20%
	vec3_t land_min, land_max;

	land_min[0] = mLandScape->GetBounds ( )[0][0] + (mLandScape->GetBounds ( )[1][0]-mLandScape->GetBounds ( )[0][0]) * 0.1f;
	land_min[1] = mLandScape->GetBounds ( )[0][1] + (mLandScape->GetBounds ( )[1][1]-mLandScape->GetBounds ( )[0][1]) * 0.1f;
	land_min[2] = mLandScape->GetBounds ( )[0][2] + (mLandScape->GetBounds ( )[1][2]-mLandScape->GetBounds ( )[0][2]) * 0.1f;

	land_max[0] = mLandScape->GetBounds ( )[1][0] - (mLandScape->GetBounds ( )[1][0]-mLandScape->GetBounds ( )[0][0]) * 0.1f;
	land_max[1] = mLandScape->GetBounds ( )[1][1] - (mLandScape->GetBounds ( )[1][1]-mLandScape->GetBounds ( )[0][1]) * 0.1f;
	land_max[2] = mLandScape->GetBounds ( )[1][2] - (mLandScape->GetBounds ( )[1][2]-mLandScape->GetBounds ( )[0][2]) * 0.1f;

	// Create a new area manager for the landscape
	mAreaManager = new CRMAreaManager ( land_min,
										land_max );

	// Create a new path manager
	mPathManager = new CRMPathManager ( mLandScape );
}

/************************************************************************************************
 * CRMMission::~CRMMission
 *	destructor
 *
 * inputs:
 *  none
 *
 * return:
 *	none
 *
 ************************************************************************************************/
CRMMission::~CRMMission ( )
{
	rmObjectiveIter_t	oit;
	rmInstanceIter_t	iit;

//	mCheckedEnts.clear();

	// Cleanup the objectives
	for (oit = mObjectives.begin(); oit != mObjectives.end(); ++oit)
	{
		delete (*oit);
	}

	// Cleanup the instances
	for (iit = mInstances.begin(); iit != mInstances.end(); ++iit)
	{
		delete (*iit);
	}

	if (mPathManager)
	{
		delete mPathManager;
		mPathManager = 0;
	}

	if (mAreaManager)
	{
		delete mAreaManager;
		mAreaManager = 0;
	}
}

/************************************************************************************************
 * CRMMission::FindObjective
 *	searches through the missions objectives for the one with the given name
 *
 * inputs:
 *  name: name of objective to find
 *
 * return:
 *	objective: objective matching the given name or NULL if it couldnt be found
 *
 ************************************************************************************************/
CRMObjective* CRMMission::FindObjective ( const char* name )
{
	rmObjectiveIter_t it;

	for (it = mObjectives.begin(); it != mObjectives.end(); ++it)
	{
		// Does it match?
		if (!Q_stricmp ((*it)->GetName(), name ))
		{
			return (*it);
		}
	}

	// Not found
	return NULL;
}

void	CRMMission::MirrorPos(vec3_t pos)
{
	pos[0] = 1.0f - pos[0];
	pos[1] = 1.0f - pos[1];
}

/************************************************************************************************
 * CRMMission::ParseOrigin
 *	parses an origin block which includes linking to a node and absolute origins
 *
 * inputs:
 *  group: parser group containing the node or origin 
 *
 * return:
 *	true: parsed successfully
 *  false: failed to parse
 *
 ************************************************************************************************/
bool CRMMission::ParseOrigin ( CGPGroup* originGroup, vec3_t origin, vec3_t lookat, int* flattenHeight )
{
	const char*	szNodeName;
	vec3_t		mins;
	vec3_t		maxs;

	if ( flattenHeight )
	{
		*flattenHeight = 66;
	}
	
	// If no group was given then use 0,0,0
	if ( NULL == originGroup )
	{
		VectorCopy ( vec3_origin, origin );
		return false;
	}

	// See if attaching to a named node
	szNodeName = originGroup->FindPairValue ( "node", "" );
	if ( *szNodeName )
	{
		CRMNode*	node;
		// Find the node being attached to
		node = mPathManager->FindNodeByName ( szNodeName );
		if ( node )
		{
			if ( flattenHeight )
			{
				if ( node->GetFlattenHeight ( ) == -1 )
				{
					node->SetFlattenHeight ( 40 + mLandScape->irand(0,40) );
				}

				*flattenHeight = node->GetFlattenHeight ( );
			}

			VectorCopy(node->GetPos(), origin);

			VectorCopy ( origin, lookat );

			int dir;
			int rnd_offset = mLandScape->irand(0, DIR_MAX-1);
			for (dir=0; dir<DIR_MAX; dir++)
			{
				int d = (dir + rnd_offset) % DIR_MAX;
				if (node->PathExist(d))
				{
					vec4_t tmp_pt, tmp_dir;
					int pathID = node->GetPath(d);
					mLandScape->GetPathInfo(pathID, .1, tmp_pt, tmp_dir );
					lookat[0] = tmp_pt[0];
					lookat[1] = tmp_pt[1];
					lookat[2] = 0;
					return true;
				}
			}
			return true;
		}
	}

	mins[0] = atof( originGroup->FindPairValue ( "left", ".1" ) );
	mins[1] = atof( originGroup->FindPairValue ( "top", ".1" ) );
	maxs[0] = atof( originGroup->FindPairValue ( "right", ".9" ) );
	maxs[1] = atof( originGroup->FindPairValue ( "bottom", ".9" ) );

	lookat[0] = origin[0] = mLandScape->flrand(mins[0],maxs[0]);
	lookat[1] = origin[1] = mLandScape->flrand(mins[1],maxs[1]);
	lookat[2] = origin[2] = 0;

	return true;
}

/************************************************************************************************
 * CRMMission::ParseNodes
 *	parses all the named nodes in the file
 *
 * inputs:
 *  group: parser group containing the named nodes
 *
 * return:
 *	true:  parsed successfully
 *  false: failed to parse
 *
 ************************************************************************************************/
bool CRMMission::ParseNodes ( CGPGroup* group )
{
	// If NULL that means this particular difficulty level has no named nodes
	if ( NULL == group || mValidNodes)
	{
		return true;
	}

	// how many nodes spaced over map?
	int			x_cells;
	int			y_cells;

	x_cells = atoi ( group->FindPairValue ( "x_cells", "3" ) );
	y_cells = atoi ( group->FindPairValue ( "y_cells", "3" ) );

	mPathManager->CreateArray(x_cells, y_cells);

	// Loop through all the nodes and generate each as specified
	for ( group = group->GetSubGroups(); 
		  group; 
		  group=group->GetNext() )
	{
		int min_depth = atof( group->FindPairValue ( "min_depth", "0" ) );
		int max_depth = atof( group->FindPairValue ( "max_depth", "5" ) );
		int min_paths = atoi( group->FindPairValue ( "min_paths", "1" ) );
		int max_paths = atoi( group->FindPairValue ( "max_paths", "1" ) );		

		mPathManager->CreateLocation( group->GetName(), min_depth, max_depth, min_paths, max_paths );
	}

	mValidNodes = true;
	return true;
}

/************************************************************************************************
 * CRMMission::ParsePaths
 *	parses all path styles in the file and then generates paths
 *
 * inputs:
 *  group: parser group containing the list of path styles
 *
 * return:
 *	true:  parsed successfully
 *  false: failed to parse
 *
 ************************************************************************************************/
bool CRMMission::ParsePaths ( CGPGroup* group )
{
	// If NULL that means this particular difficulty level has no paths
	if ( NULL == group || mValidPaths)
	{
		return true;
	}

	// path style info
	float		depth;
	float		deviation;
	float		breadth;
	float		minwidth;
	float		maxwidth;
	int			points;

	points    = atoi ( group->FindPairValue ( "points", "10" ) );
	depth     = atof ( group->FindPairValue ( "depth", ".31" ) );
	deviation = atof ( group->FindPairValue ( "deviation", ".025" ) );
	breadth   = atof ( group->FindPairValue ( "breadth", "5" ) );
	minwidth  = atof ( group->FindPairValue ( "minwidth", ".03" ) );
	maxwidth  = atof ( group->FindPairValue ( "maxwidth", ".05" ) );

	mPathManager->SetPathStyle( points, minwidth, maxwidth, depth, deviation, breadth);

	if (!mValidPaths)
	{	// we must create paths
		mPathManager->GeneratePaths( mSymmetric );
		mValidPaths = true;
	}

	return true;
}

/************************************************************************************************
 * CRMMission::ParseRivers
 *	parses all river styles in the file and then generates rivers
 *
 * inputs:
 *  group: parser group containing the list of path styles
 *
 * return:
 *	true:  parsed successfully
 *  false: failed to parse
 *
 ************************************************************************************************/
bool CRMMission::ParseRivers ( CGPGroup* group )
{
	// If NULL that means this particular difficulty level has no rivers
	if ( NULL == group || mValidRivers)
	{
		return true;
	}

	// river style info
	int			maxdepth;
	float		beddepth;
	float		deviation;
	float		breadth;
	float		minwidth;
	float		maxwidth;
	int			points;
	string		bridge_name;

	maxdepth  = atoi ( group->FindPairValue ( "maxpathdepth", "5" ) );
	points    = atoi ( group->FindPairValue ( "points", "10" ) );
	beddepth  = atof ( group->FindPairValue ( "depth", "1" ) );
	deviation = atof ( group->FindPairValue ( "deviation", ".03" ) );
	breadth   = atof ( group->FindPairValue ( "breadth", "7" ) );
	minwidth  = atof ( group->FindPairValue ( "minwidth", ".01" ) );
	maxwidth  = atof ( group->FindPairValue ( "maxwidth", ".03" ) );
	bridge_name= group->FindPairValue ( "bridge", "" ) ;

	mPathManager->SetRiverStyle( maxdepth, points, minwidth, maxwidth, beddepth, deviation, breadth, bridge_name);

	if (!mValidRivers && 
		beddepth < 1)						// use a depth of 1 if we don't want any rivers
	{	// we must create rivers
		mPathManager->GenerateRivers();
		mValidRivers = true;
	}

	return true;
}

void CRMMission::PlaceBridges()
{
	if (!mValidRivers || strlen(mPathManager->GetBridgeName()) < 1)
		return;

	int		max_bridges = 0;
	int		path;
	float	t;
	float	river_depth = mLandScape->GetLandScape()->GetWaterHeight();
	vec3_t	pos, lastpos;
	vec3pair_t bounds;
	VectorSet(bounds[0], 0,0,0);
	VectorSet(bounds[1], 0,0,0);

	// walk along paths looking for dips
	for (path = 0; path < mPathManager->GetPathCount(); path++)
	{
		vec4_t tmp_pt, tmp_dir;
		bool	new_water = true;

		mLandScape->GetPathInfo(path, 0, tmp_pt, tmp_dir );
		lastpos[0] = mLandScape->GetBounds ( )[0][0] + (mLandScape->GetBounds ( )[1][0]-mLandScape->GetBounds ( )[0][0]) * tmp_pt[0];
		lastpos[1] = mLandScape->GetBounds ( )[0][1] + (mLandScape->GetBounds ( )[1][1]-mLandScape->GetBounds ( )[0][1]) * tmp_pt[1];
		lastpos[2] = mLandScape->GetBounds ( )[0][2] + (mLandScape->GetBounds ( )[1][2]-mLandScape->GetBounds ( )[0][2]) * tmp_pt[2];
		mLandScape->GetLandScape()->GetWorldHeight ( lastpos, bounds, true );

		const float delta = 0.05f;
		for (t= delta; t < 1.0f; t += delta)
		{
			mLandScape->GetPathInfo(path, t, tmp_pt, tmp_dir );
			pos[0] = mLandScape->GetBounds ( )[0][0] + (mLandScape->GetBounds ( )[1][0]-mLandScape->GetBounds ( )[0][0]) * tmp_pt[0];
			pos[1] = mLandScape->GetBounds ( )[0][1] + (mLandScape->GetBounds ( )[1][1]-mLandScape->GetBounds ( )[0][1]) * tmp_pt[1];
			pos[2] = mLandScape->GetBounds ( )[0][2] + (mLandScape->GetBounds ( )[1][2]-mLandScape->GetBounds ( )[0][2]) * tmp_pt[2];
			mLandScape->GetLandScape()->GetWorldHeight ( pos, bounds, true );

			if (new_water && 
				lastpos[2] < river_depth && 
				pos[2] < river_depth && 
				pos[2] > lastpos[2])
			{	// add a bridge
				if (max_bridges < 3)
				{
					CRMArea*		area;
					CRMInstance*	instance;

					max_bridges++;

					// create a single bridge
					lastpos[2] = mLandScape->GetBounds ( )[0][2] + (mLandScape->GetBounds ( )[1][2]-mLandScape->GetBounds ( )[0][2]) * mPathManager->GetPathDepth();
					instance = mInstanceFile.CreateInstance ( mPathManager->GetBridgeName() );

					if ( NULL != instance )
					{	// Set the area 
						area = mAreaManager->CreateArea ( lastpos, instance->GetSpacingRadius(), instance->GetSpacingLine(), GetDefaultPadding(), 0, vec3_origin, pos, instance->GetFlattenRadius()?true:false, false, instance->GetLockOrigin() );
						area->EnableLookAt(false);

						instance->SetArea ( mAreaManager, area );
						mInstances.push_back ( instance );
						new_water = false;
					}
				}
			}
			else if (pos[2] > river_depth)
			{	// hit land again
				new_water = true;
			}
			VectorCopy ( pos, lastpos );
		}
	}
}

void CRMMission::PlaceWallInstance(CRMInstance*	instance, float xpos, float ypos, float zpos, int x, int y, float angle)
{
	if (NULL == instance)
		return;

	float spacing = instance->GetSpacingRadius();
	CRMArea*		area;
	vec3_t			origin;

	origin[0] = xpos + spacing * x;
	origin[1] = ypos + spacing * y;
	origin[2] = zpos;

	// Set the area of position
	area = mAreaManager->CreateArea ( origin, (spacing / 2.1f), 0, GetDefaultPadding(), 0, vec3_origin, origin, instance->GetFlattenRadius()?true:false, false, instance->GetLockOrigin() );
	area->EnableLookAt(false);
	area->SetAngle(angle);
	instance->SetArea ( mAreaManager, area );

	mInstances.push_back ( instance );
}


/************************************************************************************************
 * CRMMission::ParseWallRect
 *	creates instances for walled rectangle at this node (fence)
 *
 * inputs:
 *  group: parser group containing the wall rect info
 *
 * return:
 *	true:  parsed successfully
 *  false: failed to parse
 *
 ************************************************************************************************/
bool CRMMission::ParseWallRect(CGPGroup* group , int side)
{
	CGPGroup* wallGroup = group->FindSubGroup ( "wallrect" ) ;

	// If NULL that means this particular instance has no wall rect
	if ( NULL == group || NULL == wallGroup)
	{
		return true;
	}

	const char* wallName = wallGroup->FindPairValue ( "wall_instance", "" );
	const char* cornerName = wallGroup->FindPairValue ( "corner_instance", "" );
	const char* towerName = wallGroup->FindPairValue ( "tower_instance", "" );
	const char* gateName = wallGroup->FindPairValue ( "gate_instance", "" );
	const char* ripName = wallGroup->FindPairValue ( "rip_instance", "" );

	if ( NULL != wallName )
	{
		int xcount = atoi( wallGroup->FindPairValue ( "xcount", "0" ) );
		int ycount = atoi( wallGroup->FindPairValue ( "ycount", "0" ) );

		int gateCount = atoi( wallGroup->FindPairValue ( "gate_count", "1" ) );
		int gateMin = atoi( wallGroup->FindPairValue ( "gate_min", "0" ) );
		int gateMax = atoi( wallGroup->FindPairValue ( "gate_max", "0" ) );

		int ripCount = atoi( wallGroup->FindPairValue ( "rip_count", "0" ) );
		int ripMin = atoi( wallGroup->FindPairValue ( "rip_min", "0" ) );
		int ripMax = atoi( wallGroup->FindPairValue ( "rip_max", "0" ) );

		int towerCount = atoi( wallGroup->FindPairValue ( "tower_count", "0" ) );
		int towerMin = atoi( wallGroup->FindPairValue ( "tower_min", "0" ) );
		int towerMax = atoi( wallGroup->FindPairValue ( "tower_max", "0" ) );

		if (gateMin != gateMax)
			gateCount = mLandScape->irand(gateMin,gateMax);

		if (ripMin != ripMax)
			ripCount = mLandScape->irand(ripMin,ripMax);

		if (towerMin != towerMax)
			towerCount = mLandScape->irand(towerMin,towerMax);

		if (NULL == gateName)
			gateCount = 0;

		if (NULL == towerName)
			towerCount = 0;

		if (NULL == ripName)
			ripCount = 0;

		const char*		nodename;
		CGPGroup* originGroup = group->FindSubGroup ( "origin" );
		if (originGroup)
		{
			nodename = originGroup->FindPairValue ( "node", "" );
			if (*nodename)
			{
				CRMNode*	node;
				// Find the node being attached to
				node = mPathManager->FindNodeByName ( nodename );
				if ( node )
				{
					CRMInstance*	instance;
					int x,y;
					int halfx = xcount/2;
					int halfy = ycount/2;
					float xpos = mLandScape->GetBounds ( )[0][0] + (mLandScape->GetBounds ( )[1][0]-mLandScape->GetBounds ( )[0][0]) * node->GetPos()[0];
					float ypos = mLandScape->GetBounds ( )[0][1] + (mLandScape->GetBounds ( )[1][1]-mLandScape->GetBounds ( )[0][1]) * node->GetPos()[1];
					float zpos = mLandScape->GetBounds ( )[1][2] + 100;
					float angle = 0;
					int  lastGate = 0;
					int  lastRip = 0;

					// corners
					x = -halfx;
					y = -halfy;
					if (towerCount > 3 || 
  					    (towerCount > 0 && mLandScape->irand(1,2) == 1) )
					{
						towerCount--;
						instance = mInstanceFile.CreateInstance ( towerName );
					}
					else
						instance = mInstanceFile.CreateInstance ( cornerName );
					angle = DEG2RAD(90);
					instance->SetSide(side);
					PlaceWallInstance(instance, xpos, ypos, zpos, x, y, angle);

					x = halfx;
					y = -halfy;
					if (towerCount > 3 || 
  					    (towerCount > 0 && mLandScape->irand(1,2) == 1) )
					{
						towerCount--;
						instance = mInstanceFile.CreateInstance ( towerName );
					}
					else
						instance = mInstanceFile.CreateInstance ( cornerName );
					angle = DEG2RAD(180);
					instance->SetSide(side);
					PlaceWallInstance(instance, xpos, ypos, zpos, x, y, angle);

					x = halfx;
					y = halfy;
					if (towerCount > 3 || 
  					    (towerCount > 0 && mLandScape->irand(1,2) == 1) )
					{
						towerCount--;
						instance = mInstanceFile.CreateInstance ( towerName );
					}
					else
						instance = mInstanceFile.CreateInstance ( cornerName );
					angle = DEG2RAD(270);
					instance->SetSide(side);
					PlaceWallInstance(instance, xpos, ypos, zpos, x, y, angle);

					x = -halfx;
					y = halfy;
					if (towerCount > 3 || 
  					    (towerCount > 0 && mLandScape->irand(1,2) == 1) )
					{
						towerCount--;
						instance = mInstanceFile.CreateInstance ( towerName );
					}
					else
						instance = mInstanceFile.CreateInstance ( cornerName );
					angle = DEG2RAD(0);
					instance->SetSide(side);
					PlaceWallInstance(instance, xpos, ypos, zpos, x, y, angle);

					// walls
					angle = DEG2RAD(0);
					for (x = -halfx+1; x <= halfx-1; x++)
					{
						if (lastGate<1 && gateCount > 0 && mLandScape->irand(1,(halfx+halfy)/gateCount) == 1)
						{	// gate
							gateCount--;
							lastGate = 3;
							instance = mInstanceFile.CreateInstance ( gateName );
						}
						else if (lastRip<1 && ripCount > 0 && mLandScape->irand(1,(halfx+halfy)/ripCount) == 1)
						{	// damaged fence
							ripCount--;
							lastRip = 3;
							instance = mInstanceFile.CreateInstance ( ripName );
						}
						else
						{   // just a wall
							instance = mInstanceFile.CreateInstance ( wallName );
							lastRip--;
							lastGate--;
						}
						instance->SetSide(side);
						PlaceWallInstance(instance, xpos, ypos, zpos, x, -halfy, angle);
					}
					for (x = -halfx+1; x <= halfx-1; x++)
					{
						if (lastGate<1 && gateCount > 0 && mLandScape->irand(1,(halfx+halfy)/gateCount) == 1)
						{	// gate
							gateCount--;
							lastGate = 3;
							instance = mInstanceFile.CreateInstance ( gateName );
						}
						else if (lastRip<1 && ripCount > 0 && mLandScape->irand(1,(halfx+halfy)/ripCount) == 1)
						{	// damaged fence
							ripCount--;
							lastRip = 3;
							instance = mInstanceFile.CreateInstance ( ripName );
						}
						else
						{   // just a wall
							instance = mInstanceFile.CreateInstance ( wallName );
							lastRip--;
							lastGate--;
						}
						instance->SetSide(side);
						PlaceWallInstance(instance, xpos, ypos, zpos, x, halfy, angle);
					}

					angle = DEG2RAD(90);
					for (y = -halfy+1; y <= halfy-1; y++)
					{
						if (lastGate<1 && gateCount > 0 && mLandScape->irand(1,(halfx+halfy)/gateCount) == 1)
						{	// gate
							gateCount--;
							lastGate = 3;
							instance = mInstanceFile.CreateInstance ( gateName );
						}
						else if (lastRip<1 && ripCount > 0 && mLandScape->irand(1,(halfx+halfy)/ripCount) == 1)
						{	// damaged fence
							ripCount--;
							lastRip = 3;
							instance = mInstanceFile.CreateInstance ( ripName );
						}
						else
						{   // just a wall
							instance = mInstanceFile.CreateInstance ( wallName );
							lastRip--;
							lastGate--;
						}
						instance->SetSide(side);
						PlaceWallInstance(instance, xpos, ypos, zpos, -halfx, y, angle);
					}
					for (y = -halfy+1; y <= halfy-1; y++)
					{
						if (lastGate<1 && gateCount > 0 && mLandScape->irand(1,(halfx+halfy)/gateCount) == 1)
						{	// gate
							gateCount--;
							lastGate = 3;
							instance = mInstanceFile.CreateInstance ( gateName );
						}
						else if (lastRip<1 && ripCount > 0 && mLandScape->irand(1,(halfx+halfy)/ripCount) == 1)
						{	// damaged fence
							ripCount--;
							lastRip = 3;
							instance = mInstanceFile.CreateInstance ( ripName );
						}
						else
						{   // just a wall
							instance = mInstanceFile.CreateInstance ( wallName );
							lastRip--;
							lastGate--;
						}
						instance->SetSide(side);
						PlaceWallInstance(instance, xpos, ypos, zpos, halfx, y, angle);
					}
				}
			}
		}
	}			
	else
		return false;

	return true;
}


/************************************************************************************************
 * CRMMission::ParseInstancesOnPath
 *	creates instances on path between nodes
 *
 * inputs:
 *  group: parser group containing the defenses, other instances on the path between nodes
 *
 * return:
 *	true:  parsed successfully
 *  false: failed to parse
 *
 ************************************************************************************************/
bool CRMMission::ParseInstancesOnPath ( CGPGroup* group )
{
	CGPGroup* defenseGroup;
	for ( defenseGroup = group->GetSubGroups(); 
		  defenseGroup; 
		  defenseGroup=defenseGroup->GetNext() )
	if (Q_stricmp ( defenseGroup->GetName ( ), "defenses" )==0 ||
		Q_stricmp ( defenseGroup->GetName(), "instanceonpath")==0)
	{
		const char* defName = defenseGroup->FindPairValue ( "instance", "" );
		if ( *defName )
		{
			float	minpos;
			float	maxpos;
			int		mincount;
			int		maxcount;

			// how far along path does this get placed?
			minpos = atof( defenseGroup->FindPairValue ( "minposition", "0.5" ) );
			maxpos = atof( defenseGroup->FindPairValue ( "maxposition", "0.5" ) );
			mincount = atoi( defenseGroup->FindPairValue ( "mincount", "1" ) );
			maxcount = atoi( defenseGroup->FindPairValue ( "maxcount", "1" ) );

			const char*		nodename;
			CGPGroup* originGroup = group->FindSubGroup ( "origin" );
			if (originGroup)
			{
				nodename = originGroup->FindPairValue ( "node", "" );
				if (*nodename)
				{
					CRMNode*	node;
					// Find the node being attached to
					node = mPathManager->FindNodeByName ( nodename );
					if ( node )
					{
						int dir;
						// look at each connection from this node to others, if there is a path, create a defense
						for (dir=0; dir<DIR_MAX; dir++)
						{
							if (node->PathExist(dir))
							{	// path leads out of this node
								CRMArea*		area;
								CRMInstance*	instance;
								float			spacing;
								vec3_t			origin;
								vec3_t			lookat;
								vec4_t			tmp_pt, tmp_dir;
								int				n,num_insts = mLandScape->irand(mincount,maxcount);
								int				pathID = node->GetPath(dir);

								if (0 == num_insts)
									continue;

								float posdelta = (maxpos - minpos) / num_insts;

								for (n=0; n<num_insts; n++)
								{

									instance = mInstanceFile.CreateInstance ( defName );
									// Failed to create, not good
									if ( NULL == instance )
									{
										continue; 
									}
									// If a spacing radius was specified then override the one thats
									// in the instance
									spacing = atof( defenseGroup->FindPairValue ( "spacing", "0" ) );
									if ( spacing )
									{
										instance->SetSpacingRadius ( spacing );
									}

									instance->SetFilter(group->FindPairValue("filter", ""));
									instance->SetTeamFilter(group->FindPairValue("teamfilter", ""));

									if (strstr(instance->GetTeamFilter(),"red"))
										instance->SetSide(SIDE_RED);
									else if (strstr(instance->GetTeamFilter(),"blue"))
										instance->SetSide(SIDE_BLUE);

									float pos_along_path = mLandScape->flrand(minpos + posdelta*n, minpos + posdelta*(n+1));
									float look_along_path = atof( defenseGroup->FindPairValue ( "pathalign", "1" ) ) ;
									mLandScape->GetPathInfo (pathID, pos_along_path, tmp_pt, tmp_dir );
									origin[0] = tmp_pt[0];
									origin[1] = tmp_pt[1];

									mLandScape->GetPathInfo (pathID, look_along_path, tmp_dir, tmp_pt );
									lookat[0] = tmp_pt[0];
									lookat[1] = tmp_pt[1];

									origin[0] = mLandScape->GetBounds ( )[0][0] + (mLandScape->GetBounds ( )[1][0]-mLandScape->GetBounds ( )[0][0]) * origin[0];
									origin[1] = mLandScape->GetBounds ( )[0][1] + (mLandScape->GetBounds ( )[1][1]-mLandScape->GetBounds ( )[0][1]) * origin[1];
									origin[2] = mLandScape->GetBounds ( )[0][2] ;

									// look at a point along the path at this location
									lookat[0] = mLandScape->GetBounds ( )[0][0] + (mLandScape->GetBounds ( )[1][0]-mLandScape->GetBounds ( )[0][0]) * lookat[0];
									lookat[1] = mLandScape->GetBounds ( )[0][1] + (mLandScape->GetBounds ( )[1][1]-mLandScape->GetBounds ( )[0][1]) * lookat[1];
									lookat[2] = 0;

									// Fixed height?  (used for bridges)
									if ( !atoi(group->FindPairValue ( "nodrop", "0" )) )
									{
										origin[2] = mLandScape->GetBounds ( )[1][2] + 100;
									}
					
									// Set the area of position
									area = mAreaManager->CreateArea ( origin, instance->GetSpacingRadius(), instance->GetSpacingLine(), GetDefaultPadding(), 0, origin, lookat, instance->GetFlattenRadius()?true:false, true, instance->GetLockOrigin(), mSymmetric );
									area->EnableLookAt(false);

									if ( node->GetFlattenHeight ( ) == -1 )
									{
										node->SetFlattenHeight ( 66 + mLandScape->irand(0,40) );
									}
									instance->SetFlattenHeight ( node->GetFlattenHeight ( ) );

									instance->SetArea ( mAreaManager, area );

									mInstances.push_back ( instance );
								}
							}
						}
					}
				}
			}
			else
				return false;
		}			
		else
			return false;

	}

	return true;
}

/************************************************************************************************
 * CRMMission::ParseInstance
 *	Parses an individual instance
 *
 * inputs:
 *  group: parser group containing the list of instances
 *
 * return:
 *	true: instances parsed successfully
 *  false: instances failed to parse
 *
 ************************************************************************************************/
bool CRMMission::ParseInstance ( CGPGroup* group )
{
	CRMArea*		area;
	CRMInstance*	instance;
	float			spacing;
	vec3_t			origin;
	vec3_t			lookat;
	int				flattenHeight;

	// create fences / walls 

	// Create the instance using the instance file helper class
	instance = mInstanceFile.CreateInstance ( group->GetName ( ) );

	// Failed to create, not good
	if ( NULL == instance )
	{
		return false;
	}

	// If a spacing radius was specified then override the one thats
	// in the instance
	spacing = atof( group->FindPairValue ( "spacing", "0" ) );
	if ( spacing )
	{
		instance->SetSpacingRadius ( spacing );
	}

	instance->SetFilter(group->FindPairValue("filter", ""));
	instance->SetTeamFilter(group->FindPairValue("teamfilter", ""));

	if (strstr(instance->GetTeamFilter(),"red"))
		instance->SetSide( SIDE_RED);
	else if (strstr(instance->GetTeamFilter(),"blue"))
		instance->SetSide( SIDE_BLUE );

//	ParseWallRect(group, instance->GetSide());

	// Get its origin now
	ParseOrigin ( group->FindSubGroup ( "origin" ), origin, lookat, &flattenHeight );
	origin[0] = mLandScape->GetBounds ( )[0][0] + (mLandScape->GetBounds ( )[1][0]-mLandScape->GetBounds ( )[0][0]) * origin[0];
	origin[1] = mLandScape->GetBounds ( )[0][1] + (mLandScape->GetBounds ( )[1][1]-mLandScape->GetBounds ( )[0][1]) * origin[1];
	origin[2] = mLandScape->GetBounds ( )[0][2] + (mLandScape->GetBounds ( )[1][2]-mLandScape->GetBounds ( )[0][2]) * origin[2];

	lookat[0] = mLandScape->GetBounds ( )[0][0] + (mLandScape->GetBounds ( )[1][0]-mLandScape->GetBounds ( )[0][0]) * lookat[0];
	lookat[1] = mLandScape->GetBounds ( )[0][1] + (mLandScape->GetBounds ( )[1][1]-mLandScape->GetBounds ( )[0][1]) * lookat[1];
	lookat[2] = mLandScape->GetBounds ( )[0][2] + (mLandScape->GetBounds ( )[1][2]-mLandScape->GetBounds ( )[0][2]) * lookat[2];

	// Fixed height?  (used for bridges)
	if ( !atoi(group->FindPairValue ( "nodrop", "0" )) )
	{
		origin[2] = mLandScape->GetBounds ( )[1][2] + 100;
	}

	// Set the area of position
	area = mAreaManager->CreateArea ( origin, instance->GetSpacingRadius(), instance->GetSpacingLine(), GetDefaultPadding(), 0, vec3_origin, lookat, instance->GetFlattenRadius()?true:false, true, instance->GetLockOrigin(), mSymmetric );
	instance->SetArea ( mAreaManager, area );
	instance->SetFlattenHeight ( flattenHeight );

	mInstances.push_back ( instance );

	// create defenses?
	ParseInstancesOnPath(group );

	return true;
}


/************************************************************************************************
 * CRMMission::ParseInstances
 *	parses all instances within the mission and populates the instance list
 *
 * inputs:
 *  group: parser group containing the list of instances
 *
 * return:
 *	true: instances parsed successfully
 *  false: instances failed to parse
 *
 ************************************************************************************************/
bool CRMMission::ParseInstances ( CGPGroup* group )
{
	// If NULL that means this particular difficulty level has no instances
	if ( NULL == group )
	{
		return true;
	}

	// Loop through all the instances in the mission and add each 
	// to the master list of instances
	for ( group = group->GetSubGroups(); 
		  group; 
		  group=group->GetNext() )
	{
		ParseInstance ( group );
	}

	return true;
}

/************************************************************************************************
 * CRMMission::ParseObjectives
 *	parses all objectives within the mission and populates the objective list
 *
 * inputs:
 *  group: parser group containing the list of objectives
 *
 * return:
 *	true: objectives parsed successfully
 *  false: objectives failed to parse
 *
 ************************************************************************************************/
bool CRMMission::ParseObjectives ( CGPGroup* group )
{
	// If NULL that means this particular difficulty level has no objectives
	if ( NULL == group )
	{
		return true;
	}

	// Loop through all the objectives in the mission and add each
	// to the master list of objectives
	for ( group = group->GetSubGroups(); 
		  group; 
		  group=group->GetNext() )
	{
		CRMObjective* objective;

		// Create the new objective
		objective = new CRMObjective ( group );

		mObjectives.push_back ( objective );
	}

	mValidObjectives = true;
	
	return true;
}

/************************************************************************************************
 * CRMMission::ParseAmmo
 *	parses the given ammo list and sets the necessary ammo cvars to grant those 
 *  weapons to the players
 *
 * inputs:
 *  ammos: parser group containing the ammo list
 *
 * return:
 *	true: ammo parsed successfully
 *  false: ammo failed to parse
 *
 ************************************************************************************************/
bool CRMMission::ParseAmmo ( CGPGroup* ammos )
{
/*	CGPValue* ammo;

	// No weapons, no success
	if ( NULL == ammos )
	{
		return false;
	}

	if (0 == gi.Cvar_VariableIntegerValue("ar_wpnselect"))
	{
		// Make sure the ammo cvars are all reset so ammo from the last map or 
		// another difficulty level wont carry over
		CWeaponSystem::ClearAmmoCvars (TheWpnSysHelper());

		ammo = ammos->GetPairs ( );

		// Loop through the weapons listed and grant them to the player
		while ( ammo )
		{	
			// Grab the weapons ID
			AmmoID id = CWeaponSystem::GetAmmoID ( ammo->GetName ( ) );

			// Now set the weapon cvar with the given data
			TheWpnSysHelper().CvarSet ( CWeaponSystem::GetAmmoCvar ( id ), ammo->GetTopValue ( ), CVAR_AMMO );	

			// Move on to the next weapon
			ammo = (CGPValue*)ammo->GetNext();
		}
	}
*/
	mValidAmmo = true;

	return true;
}

/************************************************************************************************
 * CRMMission::ParseWeapons
 *	parses the given weapon list and sets the necessary weapon cvars to grant those 
 *  weapons to the players
 *
 * inputs:
 *  weapons: parser group containing the weapons list
 *
 * return:
 *	true: weapons parsed successfully
 *  false: weapons failed to parse
 *
 ************************************************************************************************/
bool CRMMission::ParseWeapons ( CGPGroup* weapons )
{
/*	CGPValue*	weapon;
	WpnID		id;

	// No weapons, no success
	if ( NULL == weapons )
	{
		return false;
	}

	if (0 == gi.Cvar_VariableIntegerValue("ar_wpnselect"))
	{
		// Make sure the weapon cvars are all reset so weapons from the last map or 
		// another difficulty level wont carry over
		CWeaponSystem::ClearWpnCvars (TheWpnSysHelper());

		id     = NULL_WpnID;
		weapon = weapons->GetPairs ( );

		// Loop through the weapons listed and grant them to the player
		while ( weapon )
		{	
			// Grab the weapons ID
			id = CWeaponSystem::GetWpnID ( weapon->GetName ( ) );

			// Now set the weapon cvar with the given data
			TheWpnSysHelper().CvarSet ( CWeaponSystem::GetWpnCvar ( id ), weapon->GetTopValue ( ) );	

			// Move on to the next weapon
			weapon = (CGPValue*)weapon->GetNext();
		}

		// If we found at least one weapon then ready the last one in the list
		if ( NULL_WpnID != id )
		{
			TheWpnSysHelper().CvarSet("wp_righthand", va("%i/%i/0/0",id,CWeaponSystem::GetClipSize ( id )), CVAR_MISC );
		}
	}
*/
	mValidWeapons = true;

	return true;
}

/************************************************************************************************
 * CRMMission::ParseOutfit
 *	parses the outfit (weapons and ammo)
 *
 * inputs:
 *  outfit: parser group containing the outfit
 *
 * return:
 *	true: weapons and ammo parsed successfully
 *  false: failed to parse
 *
 ************************************************************************************************/
bool CRMMission::ParseOutfit ( CGPGroup* outfit )
{
	if ( NULL == outfit )
	{
		return false;
	}

/*	// Its ok to fail parsing weapons as long as weapons have
	// already been parsed at some point
	if ( !ParseWeapons ( ParseRandom ( outfit->FindSubGroup ( "weapons" ) ) ) )
	{
		if ( !mValidWeapons )
		{
			return false;
		}
	}

	// Its ok to fail parsing ammo as long as ammo have
	// already been parsed at some point
	if ( !ParseAmmo ( ParseRandom ( outfit->FindSubGroup ( "ammo" ) ) ) )
	{
		if ( !mValidAmmo)
		{
			return false;
		}
	}
*/
	return true;
}

/************************************************************************************************
 * CRMMission::ParseRandom
 *	selects a random sub group with from all within this one
 *
 * inputs:
 *  random: parser group containing the various subgroups
 *
 * return:
 *	true:  parsed successfuly
 *  false: failed to parse
 *
 ************************************************************************************************/
CGPGroup* CRMMission::ParseRandom ( CGPGroup* randomGroup )
{
	if (NULL == randomGroup)
		return NULL;

	CGPGroup* group;
	CGPGroup* groups[MAX_RANDOM_CHOICES];
	int		  numGroups;

	// Build a list of the groups one can be chosen
	for ( numGroups = 0, group = randomGroup->GetSubGroups ( ); 
		  group; 
		  group = group->GetNext ( ) )
	{
		if ( Q_stricmp ( group->GetName ( ), "random_choice" ) )
		{
			continue;
		}

		int weight = atoi ( group->FindPairValue ( "random_weight", "1" ) );
		while (weight-- > 0)
			groups[numGroups++] = group;
		assert (numGroups <= MAX_RANDOM_CHOICES);
	}

	// No groups!
	if ( !numGroups )
	{
		return randomGroup;
	}

	// Now choose a group to parse
	return groups[mLandScape->irand(0,numGroups-1)];
}

/************************************************************************************************
 * CRMMission::ParseDifficulty
 *	parses the given difficulty and populates the mission with its data
 *
 * inputs:
 *  difficulty: parser group containing the difficulties info
 *
 * return:
 *	true: difficulty parsed successfully
 *  false: difficulty failed to parse
 *
 ************************************************************************************************/
bool CRMMission::ParseDifficulty ( CGPGroup* difficulty  )
{
	// If a null difficulty then stop the recursion.  Make sure to
	// return true here so the parsing doesnt fail
	if ( NULL == difficulty )
	{
		return true;
	}
	// is map supposed to be symmetric?
	mSymmetric = (symmetry_t)atoi(difficulty->GetParent()->FindPairValue ( "symmetric", "0" ));
	mBackUpPath = atoi(difficulty->GetParent()->FindPairValue ( "backuppath", "0" ));
	if( mSymmetric )
	{// pick between the 2 starting corners -- yes this is a hack
		mSymmetric = SYMMETRY_TOPLEFT;
		if( TheRandomMissionManager->GetLandScape()->irand(0, 1) )
		{
			mSymmetric = SYMMETRY_BOTTOMRIGHT;
		}
	}
	
	mDefaultPadding = atoi(difficulty->GetParent()->FindPairValue ( "padding", "0" ));
		
	// Parse the nodes
	if ( !ParseNodes (  ParseRandom ( difficulty->FindSubGroup ( "nodes" ) ) ) )
	{
		return false;
	}

	// Parse the paths
	if ( !ParsePaths (  ParseRandom ( difficulty->FindSubGroup ( "paths" ) ) ) )
	{
		return false;
	}

	// Parse the rivers 
	if ( !ParseRivers (  ParseRandom ( difficulty->FindSubGroup ( "rivers" ) ) ) )
	{
		return false;
	}

	// Handle inherited properties
	if ( !ParseDifficulty ( difficulty->GetParent ( )->FindSubGroup ( difficulty->FindPairValue ( "inherit", "" ) ) ) )
	{
		return false;
	}

/*
	// parse the player's outfit (weapons and ammo)
	if ( !ParseOutfit( ParseRandom ( difficulty->FindSubGroup ( "outfit" ) ) ) )
	{
		// Its ok to fail parsing weapons as long as weapons have
		// already been parsed at some point
		if ( !ParseWeapons ( ParseRandom ( difficulty->FindSubGroup ( "weapons" ) ) ) )
		{
			if ( !mValidWeapons )
			{
				return false;
			}
		}

		// Its ok to fail parsing ammo as long as ammo have
		// already been parsed at some point
		if ( !ParseAmmo ( ParseRandom ( difficulty->FindSubGroup ( "ammo" ) ) ) )
		{
			if ( !mValidAmmo)
			{
				return false;
			}
		}
	}

	// Its ok to fail parsing objectives as long as objectives have
	// already been parsed at some point
	if ( !ParseObjectives ( ParseRandom ( difficulty->FindSubGroup ( "objectives" ) ) ) )
	{
		if ( !mValidObjectives )
		{
			return false;
		}
	}
*/

	// Set the cvars with the available values
	Cvar_Set ( "mi_health", difficulty->FindPairValue ( "health", "100" ) );
	Cvar_Set ( "mi_armor", difficulty->FindPairValue ( "armor", "0" ) );

	// Parse out the timelimit
	mTimeLimit = atol(difficulty->FindPairValue("timelimit", "0"));

	// NPC multipliers
	mAccuracyMultiplier = atof(difficulty->FindPairValue("npcaccuracy", "1"));
	mHealthMultiplier = atof(difficulty->FindPairValue("npchealth", "1"));

	// keep only some of RMG pickups 1 = 100%
	mPickupHealth = atof(difficulty->FindPairValue("pickup_health", "1"));
	mPickupArmor = atof(difficulty->FindPairValue("pickup_armor", "1"));
	mPickupAmmo = atof(difficulty->FindPairValue("pickup_ammo", "1"));
	mPickupWeapon = atof(difficulty->FindPairValue("pickup_weapon", "1"));
	mPickupEquipment = atof(difficulty->FindPairValue("pickup_equipment", "1"));

	// Its ok to fail parsing instances as long as instances have
	// already been parsed at some point
	if ( !ParseInstances ( ParseRandom ( difficulty->FindSubGroup ( "instances" ) ) ) )
	{
		if ( !mValidInstances )
		{
			return false;
		}
	}

	return true;
}

/************************************************************************************************
 * CRMMission::Load
 *	Loads the given mission using the given difficulty level
 *
 * inputs:
 *  name: Name of the mission to load (should only be the name rather than the full path)
 *  difficulty: difficulty level to load
 *
 * return:
 *	true: mission successfully loaded
 *  false: mission failed to load
 *
 ************************************************************************************************/
bool CRMMission::Load ( const char* mission, const char* instances, const char* difficulty )
{
	CGenericParser2		parser;
	CGPGroup*			root;

	// Create the parser for the mission file
	if(!Com_ParseTextFile(va("ext_data/rmg/%s.mission", mission), parser))
	{
		if(!Com_ParseTextFile(va("ext_data/arioche/%s.mission", mission), parser))
		{
			Com_Printf("ERROR: Failed to open mission file '%s'\n", mission);
			return false;
		}
	}
	
	// Grab the root parser groop and make sure its mission, otherwise this
	// isnt a valid mission file
	root = parser.GetBaseParseGroup()->GetSubGroups();
	if(Q_stricmp(root->GetName(), "mission"))
	{
		Com_Printf("ERROR: '%s' is not a valid mission file\n", mission );
		parser.Clean();
		return false;
	}
	
	// Grab the mission description and set the cvar for it
	mDescription = root->FindPairValue ( "description", "<MISSION DESCRIPTION MISSING>" );
//	Cvar_Set("ar_obj_main0",mDescription.c_str(), CVAR_OBJECTIVE);
//	Cvar_Set("ar_obj_maincom0", "&OBJECTIVES_INPROGRESS&", CVAR_OBJECTIVE);
//	Cvar_SetValue ("ar_cur_objective", 0, CVAR_OBJECTIVE);

	string mInfo = root->FindPairValue ( "info", "<MISSION ADDITIONAL INFO MISSING>" );
//	Cvar_Set("ar_obj_info0",mInfo.c_str(), CVAR_OBJECTIVE);

	mExitScreen = root->FindPairValue ( "exitScreen", "<EXIT SCREEN MISSING>" );
	mTimeExpiredScreen = root->FindPairValue ( "TimeExpiredScreen", "<TIME EXPIRED SCREEN MISSING>" );
	
	// Open the instance file for the specified instances
	if ( !mInstanceFile.Open ( instances) )
	{
		Com_Printf ( "ERROR: Could not open instance file '%s'\n", instances );
		return false;
	}

	// Start at one and readjust each time we see and instance
	// with a higher value
	mMaxInstancePosition = 1;

	// Now parse the specified difficulty level
	if ( !ParseDifficulty ( root->FindSubGroup ( difficulty ) ) )
	{
		return false;
	}

	// Generate the terrain now
	mLandScape->Generate(mSymmetric);

	// Cleanup
	parser.Clean();

	return true;
}

/************************************************************************************************
 * CRMMission::Spawn
 *	Spawns all of the instances for the entire mission onto the given landscape
 *
 * inputs:
 *  landscape: landscape to spawn instances on
 *
 * return:
 *	true: instances spawned successfully
 *  false: instances failed to spawn
 *
 ***********************************************************************************************/

bool CRMMission::Spawn ( CRandomTerrain* terrain, qboolean IsServer )
{
	rmInstanceIter_t	it;
	int					areaIndex;
	CRMArea*			area;

	if ( IsServer )
	{
		// Prespawn all instances, this is mainly for flattening
		for(it = mInstances.begin(); it != mInstances.end(); ++it)
		{
			CRMInstance* instance = *it;

			// Pre-Spawn
			instance->PreSpawn ( terrain, IsServer );

			if (mSymmetric)
			{
				instance->SetMirror(1);
				instance->PreSpawn ( terrain, IsServer );
				instance->SetMirror(0);
			}
		}

		mLandScape->Smooth ( );

		// place bridges
		PlaceBridges();
	}
#ifndef DEDICATED
	else
	{
		memcpy ( mLandScape->GetLandScape()->GetHeightMap ( ), clc.rmgHeightMap, mLandScape->GetLandScape()->GetRealArea ( ) );
		memcpy ( mLandScape->GetLandScape()->GetFlattenMap ( ), clc.rmgFlattenMap, mLandScape->GetLandScape()->GetRealArea ( ) );
		mLandScape->GetLandScape()->rand_seed ( clc.rmgSeed );
	}
#endif

	mLandScape->GetLandScape()->UpdatePatches();

	if ( IsServer )
	{
		// Spawn all instances
		for(it = mInstances.begin(); it != mInstances.end(); ++it)
		{
			CRMInstance* instance = *it;

			// Spawn
			instance->Spawn ( terrain, IsServer );
			instance->PostSpawn ( terrain, IsServer );

			if (mSymmetric)
			{	// spawn the mirror version
				instance->SetMirror(1);
				instance->Spawn ( terrain, IsServer );
				instance->PostSpawn ( terrain, IsServer );
				instance->SetMirror(0);
			}
		}
	}

	// create automap
	if (!com_dedicated->integer)
	{		
#ifndef DEDICATED
		CM_TM_Create(mLandScape->GetLandScape());

		if ( IsServer )
		{
			CRMManager::ProcessAutomapSymbols ( TheRandomMissionManager->GetAutomapSymbolCount(), TheRandomMissionManager->GetAutomapSymbol(0) );
		}
		else
		{
			CRMManager::ProcessAutomapSymbols ( clc.rmgAutomapSymbolCount, clc.rmgAutomapSymbols );
		}
#endif
	}

#ifndef FINAL_BUILD
	// make sure to write out after the mirror happens so red side is displayed on map
	if (1 == Cvar_VariableIntegerValue("rmg_saveautomap"))
	{	// write out automap for test purposes
		char seed[MAX_QPATH];
		char terrainName[MAX_QPATH];
		char missionName[MAX_QPATH];
		Cvar_VariableStringBuffer("RMG_seed", seed, MAX_QPATH);
		Cvar_VariableStringBuffer("RMG_terrain", terrainName, MAX_QPATH);
		Cvar_VariableStringBuffer("RMG_mission", missionName, MAX_QPATH);

#ifndef DEDICATED
		for(it = mInstances.begin(); it != mInstances.end(); ++it)
		{
			(*it)->DrawAutomapSymbol();
		}
		//gi.CM_TM_SaveImageToDisk(terrainName, missionName, seed);
		CM_TM_SaveImageToDisk(terrainName, missionName, seed);
#endif
		Com_Error (ERR_DROP, "RMG Automap written.");
		return false;
	}
#endif

//	// draw player start on automap
//	CEntity	*spot = NULL;
//	spot = entitySystem->GetEntityFromClassname( spot, "info_player_start");
//	if (spot)
//	{
//		gi.CM_TM_AddStart(spot->GetOrigin()[0], spot->GetOrigin()[1]);
//	}

	// Spawn NPC triggers now
//	SpawnNPCTriggers ( mLandScape );

	// Restory all the NPC's accuracies to the template accuracies times the
	// multiplier
//	INPCEnt::RestoreTemplate ( mAccuracyMultiplier, mHealthMultiplier );

	if ( IsServer )
	{
		// Little trick to set the current objective to the first in the list
		CompleteObjective ( NULL );

		// Iterate through the areas and add each to the landscapes list, this is sorta hacky
		// but bridges the game / common gap
		for ( areaIndex = 0; NULL != (area = mAreaManager->EnumArea ( areaIndex )); areaIndex ++ )
		{
			// Dont bother adding it to the list if collision isnt enabled
			if ( !area->IsCollisionEnabled() )
			{
				continue;
			}

			CArea* newarea = new CArea ( );
			newarea->Init ( area->GetOrigin(), area->GetSpacingRadius (), 0, area->IsFlattened()?AT_FLAT:AT_NONE );
			mLandScape->GetLandScape()->SaveArea( newarea );

			if (mSymmetric)
			{
				CArea* newarea = new CArea ( );
				newarea->Init ( area->GetOrigin(), area->GetSpacingRadius (), 0, area->IsFlattened()?AT_FLAT:AT_NONE );
				newarea->GetPosition()[0] = mLandScape->GetBounds ( )[0][0]+mLandScape->GetBounds ( )[1][0]-newarea->GetPosition()[0];
				newarea->GetPosition()[1] = mLandScape->GetBounds ( )[0][1]+mLandScape->GetBounds ( )[1][1]-newarea->GetPosition()[1];
				mLandScape->GetLandScape()->SaveArea( newarea );
			}
		}

		mInstanceFile.Close ( );
	}

	return true;
}

/************************************************************************************************
 * CRMMission::CompleteMission
 *	Pauses the game, plays an end screen after a brief delay, which then returns the player to
 * the RMG menu.
 *                                                                             *
 * Input                                                                                        *
 *    <Variable>: <Description>                                                                 *
 * Output / Return                                                                              *
 *    <Variable>: <Description>                                                                 *
 ************************************************************************************************/
 void CRMMission::CompleteMission(void)
 {
	Cvar_Set ("cl_paused", "1");

//	AddText(va("killserver; menu %s\n", mExitScreen.c_str()));
	return;
 }

/************************************************************************************************
 * CRMMission::FailedMission
 *	Pauses the game, plays an end screen after a brief delay, which then returns the player to
 * the RMG menu.
 *                                                                             *
 * Input                                                                                        *
 *    TimeExpired: indicates if the reason failed was because of time
 * Output / Return                                                                              *
 *    <Variable>: <Description>                                                                 *
 ************************************************************************************************/
 void CRMMission::FailedMission(bool TimeExpired)
 {
	Cvar_Set ("cl_paused", "1");

	if (TimeExpired)
	{
//		AddText(va("killserver; menu %s\n", mTimeExpiredScreen.c_str()));
	}
	return;
 }

/************************************************************************************************
 * CRMMission::CompleteObjective
 *	Completes the given objective and advances the current objective accordingly
 *
 * inputs:
 *  objective: the objetive to mark complete
 *
 * return:
 *	none
 *
 ************************************************************************************************/
void CRMMission::CompleteObjective ( CRMObjective* objective )
{
	rmObjectiveIter_t it;

	// Set the object as completed
	if ( objective )
	{
		objective->Complete ( true );

		// Set the completed text for the objective
//		gi.Cvar_Set( va("ar_obj_subcom0_%i", objective->GetOrderIndex ( )), "&OBJECTIVES_COMPLETE&", CVAR_OBJECTIVE) ;

/*		CEntity *tent = G_TempEntity( vec3_origin, EV_SUB_PRINT );
		tent->s.time2 = gi.SP_GetStringID ( objective->GetMessage ( ) );	
		tent->r.svFlags |= SVF_BROADCAST;
		G_AddTempEntity(tent);

		if (objective->CompleteSoundID())
			G_SoundBroadcast( tent, objective->CompleteSoundID());
*/
	}

	mCurrentObjective = NULL;

	// Find the next objective
	for (it = mObjectives.begin(); it != mObjectives.end(); ++it)
	{
		objective = (*it);

		// Skip completed objectives
		if ( objective->IsCompleted ( ) )
		{
			continue;
		}

		// Find the objective with the lowest priority
		if ( mCurrentObjective && objective->GetPriority ( ) > mCurrentObjective->GetPriority ( ) )
		{
			continue;
		}

		// Found one
		mCurrentObjective = objective;
	}	

	if ( NULL != mCurrentObjective )
	{
//		Cvar_SetValue ("ar_cur_objective", mCurrentObjective->GetOrderIndex ( ), CVAR_OBJECTIVE);

		mCurrentObjective->Activate ( );
	}
	else
	{
		// Set the completed text for the objective
//		Cvar_Set( "ar_obj_maincom0", "&OBJECTIVES_COMPLETE&", CVAR_OBJECTIVE) ;
	}
}

/************************************************************************************************
 * CRMMission::Preview
 *	Previews the instances within the mission
 *
 * inputs:
 *  from: the origin which the mission is being previewed from
 *
 * return:
 *	none
 *
 ************************************************************************************************/
void CRMMission::Preview ( const vec3_t from )
{
	rmInstanceIter_t	it;

	// Look for settlements close to the player and put up some debug stuff
	for(it = mInstances.begin(); it != mInstances.end(); ++it)
	{
		CRMInstance* instance = *it;

		vec3_t a;
		vec3_t b;

		VectorCopy ( from, a );
		VectorCopy ( instance->GetOrigin(), b );

		a[2] = 0;
		b[2] = 0;

		// Skip stuff thats too far away
		if ( Distance ( a, b) > 2000 )
		{
			continue;
		}

		instance->Preview ( from );
	}
}

/************************************************************************************************
 * CRMMission::PurgeTrigger
 *	Purge the trigger and all its targets
 *
 * inputs:
 *  trigger: trigger to purge
 *
 * return:
 *	none
 *
 ************************************************************************************************/
/*void CRMMission::PurgeTrigger ( CEntity* trigger )
{
	CEntity* target;
	
	// Purge all targets
	target = entitySystem->GetEntityFromTargetName ( NULL, trigger->GetTarget ( ) );
	while ( target )
	{
		PurgeTrigger ( target );

		target = entitySystem->GetEntityFromTargetName ( target, trigger->GetTarget ( ) );
	}

	// Get rid of the purge trigger
	entitySystem->RemoveEntityWithServer ( trigger );
	entitySystem->RemoveEntity ( trigger );
}
*/
/************************************************************************************************
 * CRMMission::PurgeUnlinkedTriggers
 *	Searches the entitySystem form a random arioche trigger that matches the objective name
 *
 * inputs:
 *  none
 *
 * return:
 *	trigger: a random trigger or NULL if one couldnt be found
 *
 ************************************************************************************************/
/*void CRMMission::PurgeUnlinkedTriggers ( )
{
	CTriggerAriocheObjective*	search;

	// Start at the first match of the classname
	search = (CTriggerAriocheObjective*) entitySystem->GetEntityFromClassname ( NULL, "trigger_arioche_objective" );

	// Continue on as long as there are triggers
	while ( search ) 
	{
		CTriggerAriocheObjective* purge = search;

		// move on to the next trigger before deleting the entity 
		// just in case there are some state issues with the search
		search = (CTriggerAriocheObjective*) entitySystem->GetEntityFromClassname ( search, "trigger_arioche_objective" );

		// Dont purge linked triggers
		if ( purge->GetObjective ( ) )
		{
			continue;
		}

		// Purge the trigger and all its targets
		PurgeTrigger ( purge );
	}
}
*/
/************************************************************************************************
 * CRMMission::SpawnNPCTriggers
 *	Spawn triggers across the map which will activate sleeping NPCs
 *
 * inputs:
 *  landscape: landscape to spawn the triggers relative to
 *
 * return:
 *	trigger: a random trigger or NULL if one couldnt be found
 *
 ************************************************************************************************/
/*void CRMMission::SpawnNPCTriggers ( CCMLandScape* landscape )
{
	CEntity* ent;
	int		 i;
	int		 count;
	float	 section;

	// Determine how many NPC sections there are in the map
	count   = (landscape->GetBounds()[1][0] - landscape->GetBounds()[0][0]) / 5000.0f;
	section = (landscape->GetBounds()[1][0] - landscape->GetBounds()[0][0]) / count;

	// Drop a trigger down at each NPC section interval except for the first and last.
	for ( i = 1; i < count - 1; i ++ )
	{
		vec3_t mins;
		vec3_t maxs;
		vec3_t origin;

		VectorCopy ( landscape->GetBounds()[0], mins );
		VectorCopy ( landscape->GetBounds()[1], maxs );

		// Set up the mins and maxs for the trigger
		mins[0] = mins[0] + (section * i) - 100;
		maxs[0] = mins[0] + 100;
		maxs[2] = maxs[2] + 100;
		mins[2] = mins[2] - 100;

		origin[0] = (maxs[0]-mins[0])/2 + mins[0];
		origin[1] = (maxs[1]-mins[1])/2 + mins[1];
		origin[2] = (maxs[2]-mins[2])/2 + mins[2];

		spawnSystem->ClearSpawnFields();
		spawnSystem->AddSpawnField("classname", "trigger_arioche_npcspawner" );
		spawnSystem->AddSpawnField("origin", va("%f %f %f",origin[0],origin[1],origin[2]) );
		spawnSystem->AddSpawnField("target", va("rmg_npc_%i", i + 1) );

		// Spawn the inhabitant, if it fails then fail
		ent = entitySystem->SpawnItem("trigger_arioche_npcspawner");
		if ( !ent )
		{
			continue;
		}

		// Fail if we cant register the entity
		if ( -1 == entitySystem->RegisterEntityWithServer( ent ) )
		{
			entitySystem->RemoveEntity ( ent );
			continue;
		}

		// Normalize the mins and maxs for the X axis since they arent
		// absolute mins and maxs
		mins[0] = -100;
		maxs[0] = 100;

		// Adjust the absmin and absmax for the trigger now
		VectorCopy ( mins, ent->r.mins[0] );
		VectorCopy ( maxs, ent->r.maxs[0] );

		Com_DPrintf( "NPC Trigger spawned at '%f %f %f' for targets 'rmg_npc_%i'\n", origin[0], origin[1], origin[2], i + 1 );

		// Set the "ONLY_ONCE" spawn flag
		ent->AddSpawnflags ( 2 );

#ifdef _GAME
		// initial linking
		gi.SV_LinkEntity( ent ); 
#endif	
	}

	AttachNPCTriggers ( landscape );
}
*/

/************************************************************************************************
 * CRMMission::AttachNPCTriggers
 *	Attaches npc triggers to all unattached npcs
 *
 * inputs:
 *  landscape: landscape triggers were spawned on
 *
 * return:
 *	trigger: a random trigger or NULL if one couldnt be found
 *
 ************************************************************************************************/
/*void CRMMission::AttachNPCTriggers ( CCMLandScape* landscape )
{
	TNPCList	npcList;
	TNPCFinder	npcFinder;
	CNPC*		theNPC = 0;
	int			npcsegment;

	GetCharacterManager().GetCharacterList(npcList);

	// Loop through all npcs and reset their accuracy
	for( npcFinder = npcList.begin(); npcFinder != npcList.end(); npcFinder++)
	{		
		theNPC = (CNPC*) INPCEnt::GetEntity(*npcFinder);
		if(!theNPC)
		{
			continue;
		}

		npcsegment = (theNPC->r.currentOrigin[0] - landscape->GetMins()[0]) / 5000.0f;		

		// All npcs in segment 0 and 1 are immediately spawned, all others wait for the
		// trigger
		if ( npcsegment > 1 )
		{
			entitySystem->RemoveFromTargetNameMap(theNPC);	
			theNPC->SetTargetName ( va("rmg_npc_%i", npcsegment ) );
			entitySystem->AddToTargetNameMap(theNPC);

			// Start the NPC in the off position
			theNPC->SetSpawnflags(1);
			theNPC->r.contents	= 0;
			theNPC->r.svFlags	|= SVF_NOCLIENT;
			theNPC->s.eFlags	|= EF_NODRAW;				
		}
	}
}
*/


