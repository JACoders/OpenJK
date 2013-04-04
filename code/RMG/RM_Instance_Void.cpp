/************************************************************************************************
 *
 * RM_Instance_Void.cpp
 *
 * Implements the CRMVoidInstance class.  This class just adds a void into the 
 * area manager to help space things out.
 *
 ************************************************************************************************/

#include "../server/exe_headers.h"

#include "rm_headers.h"

#include "rm_instance_void.h"

/************************************************************************************************
 * CRMVoidInstance::CRMVoidInstance
 *	constructs a void instance 
 *
 * inputs:
 *  instGroup:  parser group containing infromation about this instance
 *  instFile:   reference to an open instance file for creating sub instances
 *
 * return:
 *	none
 *
 ************************************************************************************************/
CRMVoidInstance::CRMVoidInstance ( CGPGroup *instGroup, CRMInstanceFile& instFile ) 
	: CRMInstance ( instGroup, instFile )
{
	mSpacingRadius = atof( instGroup->FindPairValue ( "spacing", "0" ) );
	mFlattenRadius = atof( instGroup->FindPairValue ( "flatten", "0" ) );
}

/************************************************************************************************
 * CRMVoidInstance::SetArea
 *	Overidden to make sure the void area doesnt continually.  
 *
 * inputs:
 *  area: area to set
 *
 * return:
 *	none
 *
 ************************************************************************************************/
void CRMVoidInstance::SetArea ( CRMAreaManager* amanager, CRMArea* area )
{
	// Disable collision
	area->EnableCollision ( false );

	// Do what really needs to get done
	CRMInstance::SetArea ( amanager, area );
}
