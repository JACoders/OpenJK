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
 * RM_Instance_Void.cpp
 *
 * Implements the CRMVoidInstance class.  This class just adds a void into the 
 * area manager to help space things out.
 *
 ************************************************************************************************/

#include "../server/exe_headers.h"

#include "RM_Headers.h"

#include "RM_Instance_Void.h"

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
