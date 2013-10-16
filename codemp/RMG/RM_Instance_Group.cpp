//Anything above this #include will be ignored by the compiler
#include "qcommon/exe_headers.h"

/************************************************************************************************
 *
 * RM_Instance_Group.cpp
 *
 * Implements the CRMGroupInstance class.  This class is reponsible for parsing a 
 * group instance as well as spawning it into a landscape.
 *
 ************************************************************************************************/

#include "RM_Headers.h"

#include "RM_Instance_Group.h"

/************************************************************************************************
 * CRMGroupInstance::CRMGroupInstance
 *	constructur
 *
 * inputs:
 *  settlementID:  ID of the settlement being created 
 *
 * return:
 *	none
 *
 ************************************************************************************************/
CRMGroupInstance::CRMGroupInstance ( CGPGroup *instGroup, CRMInstanceFile& instFile ) 
	:  CRMInstance ( instGroup, instFile )
{
	// Grab the padding and confine radius
	mPaddingSize   = atof ( instGroup->FindPairValue ( "padding", va("%i", TheRandomMissionManager->GetMission()->GetDefaultPadding() ) ) );
	mConfineRadius = atof ( instGroup->FindPairValue ( "confine", "0" ) );

	const char * automapSymName = instGroup->FindPairValue ( "automap_symbol", "none" );
	if (0 == Q_stricmp(automapSymName, "none"))	   	mAutomapSymbol = AUTOMAP_NONE ;
	else if (0 == Q_stricmp(automapSymName, "building"))  	mAutomapSymbol = AUTOMAP_BLD  ;
	else if (0 == Q_stricmp(automapSymName, "objective")) 	mAutomapSymbol = AUTOMAP_OBJ  ;
	else if (0 == Q_stricmp(automapSymName, "start"))	   	mAutomapSymbol = AUTOMAP_START;
	else if (0 == Q_stricmp(automapSymName, "end"))	   	mAutomapSymbol = AUTOMAP_END  ;
	else if (0 == Q_stricmp(automapSymName, "enemy"))	   	mAutomapSymbol = AUTOMAP_ENEMY;
	else if (0 == Q_stricmp(automapSymName, "friend"))	   	mAutomapSymbol = AUTOMAP_FRIEND;
	else mAutomapSymbol	= atoi( automapSymName );

	// optional instance objective strings
	SetMessage(instGroup->FindPairValue("objective_message",""));
	SetDescription(instGroup->FindPairValue("objective_description",""));
	SetInfo(instGroup->FindPairValue("objective_info",""));

	// Iterate through the sub groups to determine the instances which make up the group
	instGroup = instGroup->GetSubGroups ( );

	while ( instGroup )
	{
		CRMInstance* instance;
		const char*  name;
		int			 mincount;
		int			 maxcount;
		int			 count;
		//float		 minrange;
		//float		 maxrange;

		// Make sure only instances are specified as sub groups
		assert ( 0 == Q_stricmp ( instGroup->GetName ( ), "instance" ) );

		// Grab the name
		name     = instGroup->FindPairValue ( "name", "" );

		// Grab the range information
		//minrange = atof(instGroup->FindPairValue ( "minrange", "0" ) );
		//maxrange = atof(instGroup->FindPairValue ( "maxrange", "0" ) );

		// Grab the count information and randomly generate a count value
		mincount = atoi(instGroup->FindPairValue ( "mincount", "1" ) );
		maxcount = atoi(instGroup->FindPairValue ( "maxcount", "1" ) );
		count	 = mincount;

		if ( maxcount > mincount )
		{
			count += (TheRandomMissionManager->GetLandScape()->irand(0, maxcount-mincount));
		}

		// For each count create and add the instance
		for ( ; count ; count -- )
		{
			// Create the instance
			instance = instFile.CreateInstance ( name );

			// Skip this instance if it couldnt be created for some reason.  The CreateInstance
			// method will report an error so no need to do so here.
			if ( NULL == instance )
			{
				continue;
			}

			// Set the min and max range for the instance
			instance->SetFilter(mFilter);
			instance->SetTeamFilter(mTeamFilter);

			// Add the instance to the list
			mInstances.push_back ( instance );
		}

		// Next sub group
		instGroup = instGroup->GetNext ( );
	}
}

/************************************************************************************************
 * CRMGroupInstance::~CRMGroupInstance
 *	Removes all buildings and inhabitants
 *
 * inputs:
 *  none
 *
 * return:
 *	none
 *
 ************************************************************************************************/
CRMGroupInstance::~CRMGroupInstance(void)
{
	// Cleanup
	RemoveInstances ( );
}

/************************************************************************************************
 * CRMGroupInstance::SetFilter
 *	Sets a filter used to exclude instances
 *
 * inputs:
 *  filter: filter name
 *
 * return:
 *	none
 *
 ************************************************************************************************/
void CRMGroupInstance::SetFilter( const char *filter )
{
	rmInstanceIter_t it;

	CRMInstance::SetFilter(filter);
	for(it = mInstances.begin(); it != mInstances.end(); ++it)
	{
		(*it)->SetFilter(filter);
	}
}

/************************************************************************************************
 * CRMGroupInstance::SetTeamFilter
 *	Sets the filter used to exclude team based instances
 *
 * inputs:
 *  teamFilter: filter name
 *
 * return:
 *	none
 *
 ************************************************************************************************/
void CRMGroupInstance::SetTeamFilter( const char *teamFilter )
{
	rmInstanceIter_t it;

	CRMInstance::SetTeamFilter(teamFilter);
	for(it = mInstances.begin(); it != mInstances.end(); ++it)
	{
		(*it)->SetTeamFilter(teamFilter);
	}
}

/************************************************************************************************
 * CRMGroupInstance::SetMirror
 *	Sets the flag to mirror an instance on map
 *
 * inputs:
 *  mirror 
 *
 * return:
 *	none
 *
 ************************************************************************************************/
void CRMGroupInstance::SetMirror(int mirror)
{ 
	rmInstanceIter_t it;

	CRMInstance::SetMirror(mirror);
	for(it = mInstances.begin(); it != mInstances.end(); ++it)
	{
		(*it)->SetMirror(mirror);
	}
}


/************************************************************************************************
 * CRMGroupInstance::RemoveInstances
 *	Removes all instances associated with the group
 *
 * inputs:
 *  none
 *
 * return:
 *	none
 *
 ************************************************************************************************/
void CRMGroupInstance::RemoveInstances ( )
{
	rmInstanceIter_t it;

	for(it = mInstances.begin(); it != mInstances.end(); ++it)
	{
		delete *it;
	}

	mInstances.clear();
}

/************************************************************************************************
 * CRMGroupInstance::PreSpawn
 *	Prepares the group for spawning by 
 *
 * inputs:
 *  landscape: landscape to calculate the position within
 *  instance: instance to calculate the position for
 *
 * return:
 *	none
 *
 ************************************************************************************************/
bool CRMGroupInstance::PreSpawn ( CRandomTerrain* terrain, qboolean IsServer )
{
	rmInstanceIter_t it;

	for(it = mInstances.begin(); it != mInstances.end(); ++it )
	{
		CRMInstance* instance = *it;

		instance->SetFlattenHeight ( mFlattenHeight );

		// Add the instance to the landscape now
		instance->PreSpawn ( terrain, IsServer );		
	}

	return CRMInstance::PreSpawn ( terrain, IsServer );
}

/************************************************************************************************
 * CRMGroupInstance::Spawn
 *	Adds the group instance to the given landscape using the specified origin.  All sub instances
 *  will be added to the landscape within their min and max range from the origin.
 *
 * inputs:
 *  landscape: landscape to add the instance group to
 *  origin: origin of the instance group
 *
 * return:
 *	none
 *
 ************************************************************************************************/
bool CRMGroupInstance::Spawn ( CRandomTerrain* terrain, qboolean IsServer )
{
	rmInstanceIter_t it;

	// Spawn all the instances associated with this group
	for(it = mInstances.begin(); it != mInstances.end(); ++it)
	{
		CRMInstance* instance = *it;
		instance->SetSide(GetSide()); // which side owns it?

		// Add the instance to the landscape now
		instance->Spawn ( terrain, IsServer );
	}

	DrawAutomapSymbol();

	return true;
}

/************************************************************************************************
 * CRMGroupInstance::Preview
 *	Renders debug information for the instance
 *
 * inputs:
 *  from: point to render the preview from
 *
 * return:
 *	none
 *
 ************************************************************************************************/
void CRMGroupInstance::Preview ( const vec3_t from )
{
	rmInstanceIter_t it;

	CRMInstance::Preview ( from );
	
	// Render all the instances
	for(it = mInstances.begin(); it != mInstances.end(); ++it)
	{
		CRMInstance* instance = *it;

		instance->Preview ( from );
	}
}

/************************************************************************************************
 * CRMGroupInstance::SetArea
 *	Overidden to make sure the groups area doesnt eat up any room.  The collision on the
 *  groups area will be turned off
 *
 * inputs:
 *  area: area to set
 *
 * return:
 *	none
 *
 ************************************************************************************************/
void CRMGroupInstance::SetArea ( CRMAreaManager* amanager, CRMArea* area )
{
	rmInstanceIter_t it;

	bool collide = area->IsCollisionEnabled ( );

	// Disable collision
	area->EnableCollision ( false );

	// Do what really needs to get done
	CRMInstance::SetArea ( amanager, area );

	// Prepare for spawn by calculating all the positions of the sub instances
	// and flattening the ground below them.
	for(it = mInstances.begin(); it != mInstances.end(); ++it )
	{
		CRMInstance  *instance = *it;
		CRMArea		 *newarea;
		vec3_t		 origin;

		// Drop it in the center of the group for now
		origin[0] = GetOrigin()[0];
		origin[1] = GetOrigin()[1];
		origin[2] = 2500;

		// Set the area of position
		newarea = amanager->CreateArea ( origin, instance->GetSpacingRadius(), instance->GetSpacingLine(), mPaddingSize, mConfineRadius, GetOrigin(), GetOrigin(), instance->GetFlattenRadius()?true:false, collide, instance->GetLockOrigin(), area->GetSymmetric ( ) );
		instance->SetArea ( amanager, newarea );
	}
}
