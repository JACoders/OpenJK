/*
This file is part of Jedi Academy.

    Jedi Academy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Jedi Academy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Academy.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

/************************************************************************************************
 *
 * RM_Instance_Random.cpp
 *
 * Implements the CRMRandomInstance class.  This class is reponsible for parsing a 
 * random instance as well as spawning it into a landscape.
 *
 ************************************************************************************************/

#include "../server/exe_headers.h"

#include "RM_Headers.h"

#include "RM_Instance_Random.h"

/************************************************************************************************
 * CRMRandomInstance::CRMRandomInstance
 *	constructs a random instance by choosing one of the sub instances and creating it
 *
 * inputs:
 *  instGroup:  parser group containing infromation about this instance
 *  instFile:   reference to an open instance file for creating sub instances
 *
 * return:
 *	none
 *
 ************************************************************************************************/
CRMRandomInstance::CRMRandomInstance ( CGPGroup *instGroup, CRMInstanceFile& instFile ) 
	: CRMInstance ( instGroup, instFile )
{
	CGPGroup* group;
	CGPGroup* groups[MAX_RANDOM_INSTANCES];
	int		  numGroups;

	// Build a list of the groups one can be chosen
	for ( numGroups = 0, group = instGroup->GetSubGroups ( ); 
		  group; 
		  group = group->GetNext ( ) )
	{
		// If this isnt an instance group then skip it
		if ( stricmp ( group->GetName ( ), "instance" ) )
		{
			continue;
		}

		int multiplier = atoi(group->FindPairValue ( "multiplier", "1" ));
		for ( ; multiplier > 0 && numGroups < MAX_RANDOM_INSTANCES; multiplier -- )
		{ 
			groups[numGroups++] = group;
		}
	}

	// No groups, no instance
	if ( !numGroups )
	{
		// Initialize this now
		mInstance = NULL;

		Com_Printf ( "WARNING: No sub instances specified for random instance '%s'\n", group->FindPairValue ( "name", "unknown" ) );
		return;
	}

	// Now choose a group to parse	
	instGroup = groups[TheRandomMissionManager->GetLandScape()->irand(0,numGroups-1)];

	// Create the child instance now.  If the instance create fails then the
	// IsValid routine will return false and this instance wont be added
	mInstance = instFile.CreateInstance ( instGroup->FindPairValue ( "name", "" ) );
	mInstance->SetFilter(mFilter);
	mInstance->SetTeamFilter(mTeamFilter);

	mAutomapSymbol = mInstance->GetAutomapSymbol();

	SetMessage(mInstance->GetMessage());
	SetDescription(mInstance->GetDescription());
	SetInfo(mInstance->GetInfo());
}

/************************************************************************************************
 * CRMRandomInstance::~CRMRandomInstance
 *	Deletes the sub instance
 *
 * inputs:
 *  none
 *
 * return:
 *	none
 *
 ************************************************************************************************/
CRMRandomInstance::~CRMRandomInstance(void)
{
	if ( mInstance )
	{
		delete mInstance;
	}
}

void CRMRandomInstance::SetMirror(int mirror)
{ 
	CRMInstance::SetMirror(mirror);
	if (mInstance)
	{
		mInstance->SetMirror(mirror);
	}
}

void CRMRandomInstance::SetFilter( const char *filter )
{
	CRMInstance::SetFilter(filter);
	if (mInstance)
	{
		mInstance->SetFilter(filter);
	}
}

void CRMRandomInstance::SetTeamFilter( const char *teamFilter )
{
	CRMInstance::SetTeamFilter(teamFilter);
	if (mInstance)
	{
		mInstance->SetTeamFilter(teamFilter);
	}
}

/************************************************************************************************
 * CRMRandomInstance::PreSpawn
 *	Prepares for the spawn of the random instance
 *
 * inputs:
 *  landscape: landscape object this instance will be spawned on
 *
 * return:
 *	true: preparation successful
 *  false: preparation failed
 *
 ************************************************************************************************/
bool CRMRandomInstance::PreSpawn ( CRandomTerrain* terrain, qboolean IsServer )
{
	assert ( mInstance );

	mInstance->SetFlattenHeight ( GetFlattenHeight( ) );

	return mInstance->PreSpawn ( terrain, IsServer );
}

/************************************************************************************************
 * CRMRandomInstance::Spawn
 *	Spawns the instance onto the landscape
 *
 * inputs:
 *  landscape: landscape object this instance will be spawned on
 *
 * return:
 *	true: spawn successful
 *  false: spawn failed
 *
 ************************************************************************************************/
bool CRMRandomInstance::Spawn ( CRandomTerrain* terrain, qboolean IsServer )
{
	mInstance->SetObjective(GetObjective());
	mInstance->SetSide(GetSide());

	if ( !mInstance->Spawn ( terrain, IsServer ) )
	{
		return false;
	}

	return true;
}

/************************************************************************************************
 * CRMRandomInstance::SetArea
 *	Forwards the given area off to the internal instance
 *
 * inputs:
 *  area: area to be set
 *
 * return:
 *	none
 *
 ************************************************************************************************/
void CRMRandomInstance::SetArea ( CRMAreaManager* amanager, CRMArea* area )
{
	CRMInstance::SetArea ( amanager, area );

	mInstance->SetArea ( amanager, mArea );
}
