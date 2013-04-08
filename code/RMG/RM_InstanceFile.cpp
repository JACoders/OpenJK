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
 * RM_InstanceFile.cpp
 *
 * implements the CRMInstanceFile class.  This class provides functionality to load
 * and create instances from an instance file.  First call Open to open the instance file and
 * then use CreateInstance to create new instances.  When finished call Close to cleanup.
 *
 ************************************************************************************************/

#include "../server/exe_headers.h"

#include "rm_headers.h"

//#include "rm_instance_npc.h"
#include "rm_instance_bsp.h"
#include "rm_instance_random.h"
#include "rm_instance_group.h"
#include "rm_instance_void.h"

/************************************************************************************************
 * CRMInstanceFile::CRMInstanceFile
 *	constructor
 *
 * inputs:
 *  none
 *
 * return:
 *	none
 *
 ************************************************************************************************/
CRMInstanceFile::CRMInstanceFile ( )
{
	mInstances = NULL;
}

/************************************************************************************************
 * CRMInstanceFile::~CRMInstanceFile
 *	Destroys the instance file by freeing the parser
 *
 * inputs:
 *  none
 *
 * return:
 *	none
 *
 ************************************************************************************************/
CRMInstanceFile::~CRMInstanceFile ( )
{
	Close ( );
}

/************************************************************************************************
 * CRMInstanceFile::Open
 *	Opens the given instance file and prepares it for use in instance creation
 *
 * inputs:
 *  instance: Name of instance to open.  Note that the root path will be automatically
 *			  added and shouldnt be included in the given name
 *
 * return:
 *	true: instance file successfully loaded 
 *  false: instance file could not be loaded for some reason
 *
 ************************************************************************************************/
bool CRMInstanceFile::Open ( const char* instance )
{
	char		instanceDef[MAX_QPATH];
	CGPGroup	*basegroup;

	// Build the filename
	Com_sprintf(instanceDef, MAX_QPATH, "ext_data/rmg/%s.instance", instance );

#ifndef FINAL_BUILD
	// Debug message
	Com_Printf("CM_Terrain: Loading and parsing instanceDef %s.....\n", instance);
#endif

	// Parse the text file using the generic parser
	if(!Com_ParseTextFile(instanceDef, mParser ))
	{
		Com_sprintf(instanceDef, MAX_QPATH, "ext_data/arioche/%s.instance", instance );
		if(!Com_ParseTextFile(instanceDef, mParser ))
		{
			Com_Printf(va("CM_Terrain: Could not open instance file '%s'\n", instanceDef));
			return false;
		}
	}

	// The whole file....
	basegroup = mParser.GetBaseParseGroup();

	// The root { } struct
	mInstances = basegroup->GetSubGroups();

	// The "instances" { } structure
	mInstances = mInstances->GetSubGroups ( );

	return true;
}

/************************************************************************************************
 * CRMInstanceFile::Close
 *	Closes an open instance file
 *
 * inputs:
 *  none
 *
 * return:
 *	none
 *
 ************************************************************************************************/
void CRMInstanceFile::Close ( void )
{
	// If not open then dont close  it
	if ( NULL == mInstances )
	{
		return;
	}
	mParser.Clean();
		
	mInstances = NULL;
}

/************************************************************************************************
 * CRMInstanceFile::CreateInstance
 *	Creates an instance (to be freed by caller) using the given instance name.
 *
 * inputs:
 *  name: Name of the instance to read from the instance file
 *
 * return:
 *	NULL: instance could not be read from the instance file
 *  NON-NULL: instance created and returned for further use
 *
 ************************************************************************************************/
CRMInstance* CRMInstanceFile::CreateInstance ( const char* name )
{
	static int instanceID = 0;

	CGPGroup*		group;
	CRMInstance*	instance;

	// Make sure we were loaded
	assert ( mInstances );

	// Search through the instances for the one with the given name
	for ( group = mInstances; group; group = group->GetNext ( ) )
	{
		// Skip it if the name doesnt match
		if ( stricmp ( name, group->FindPairValue ( "name", "" ) ) )
		{
			continue;
		}
		
		// Handle the various forms of instance types
		if ( !stricmp ( group->GetName ( ), "bsp" ) )
		{
			instance = new CRMBSPInstance ( group, *this );
		}
		else if ( !stricmp ( group->GetName ( ), "npc" ) )
		{
//			instance = new CRMNPCInstance ( group, *this );
			continue;
		}
		else if ( !stricmp ( group->GetName ( ), "group" ) )
		{
			instance = new CRMGroupInstance ( group, *this );
		}
		else if ( !stricmp ( group->GetName ( ), "random" ) )
		{
			instance = new CRMRandomInstance ( group, *this );
		}
		else if ( !stricmp ( group->GetName ( ), "void" ) )
		{
			instance = new CRMVoidInstance ( group, *this );
		}
		else
		{
			continue;
		}

		// If the instance isnt valid after being created then delete it
		if ( !instance->IsValid ( ) )
		{
			delete instance;
			return NULL;
		}

		// The instance was successfully created so return it
		return instance;
	}

#ifndef FINAL_BUILD
	// The instance wasnt found in the file so report it
	Com_Printf(va("WARNING:  Instance '%s' was not found in the active instance file\n", name ));
#endif

	return NULL;
}
