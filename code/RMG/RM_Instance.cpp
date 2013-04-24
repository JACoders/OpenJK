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

#include "../server/exe_headers.h"

#include "RM_Headers.h"
#include "../qcommon/cm_terrainmap.h"

/************************************************************************************************
 * CRMInstance::CRMInstance
 *	constructs a instnace object using the given parser group
 *
 * inputs:
 *  instance:  parser group containing information about the instance
 *
 * return:
 *	none
 *
 ************************************************************************************************/
CRMInstance::CRMInstance ( CGPGroup *instGroup, CRMInstanceFile& instFile )
{
	mObjective		= NULL;
	mSpacingRadius	= 0;
	mFlattenRadius	= 0;
	mFilter[0]		= mTeamFilter[0] = 0;
	mArea			= NULL;
	mAutomapSymbol  = 0;
	mEntityID       = 0;
	mSide			= 0;
	mMirror			= 0;
	mFlattenHeight	= 66;
	mSpacingLine	= 0;
	mSurfaceSprites = true;
	mLockOrigin		= false;
}

/************************************************************************************************
 * CRMInstance::PreSpawn
 *	Prepares the instance for spawning by flattening the ground under it
 *
 * inputs:
 *  landscape: landscape the instance will be spawned on
 *
 * return:
 *	true: spawn preparation successful
 *  false: spawn preparation failed
 *
 ************************************************************************************************/
bool CRMInstance::PreSpawn ( CRandomTerrain* terrain, qboolean IsServer )
{
	vec3_t		origin;
	CArea		area;

	VectorCopy(GetOrigin(), origin);

	if (mMirror)
	{
		origin[0] = TheRandomMissionManager->GetLandScape()->GetBounds()[0][0] + TheRandomMissionManager->GetLandScape()->GetBounds()[1][0] - origin[0];
		origin[1] = TheRandomMissionManager->GetLandScape()->GetBounds()[0][1] + TheRandomMissionManager->GetLandScape()->GetBounds()[1][1] - origin[1];
	}

	const vec3_t&	  terxelSize = terrain->GetLandScape()->GetTerxelSize ( );
	const vec3pair_t& bounds     = terrain->GetLandScape()->GetBounds();

	// Align the instance to the center of a terxel
	origin[0] = bounds[0][0] + (int)((origin[0] - bounds[0][0] + terxelSize[0] / 2) / terxelSize[0]) * terxelSize[0];
	origin[1] = bounds[0][1] + (int)((origin[1] - bounds[0][1] + terxelSize[1] / 2) / terxelSize[1]) * terxelSize[1];


	// This is BAD - By copying the mirrored origin back into the instance, you've now mirrored the original instance
	// so when anything from this point on looks at the instance they'll be looking at a mirrored version but will be expecting the original
	// so later in the spawn functions the instance will be re-mirrored, because it thinks the mInstances have not been changed
//	VectorCopy(origin, GetOrigin());

	// Flatten the area below the instance 
	if ( GetFlattenRadius() )
	{	
		area.Init( origin, GetFlattenRadius(), 0.0f, AT_NONE, 0, 0 );
		terrain->GetLandScape()->FlattenArea( &area, mFlattenHeight | (mSurfaceSprites?0:0x80), false, true, true );
	}

	return true;
}

/************************************************************************************************
 * CRMInstance::PostSpawn
 *	Finishes the spawn by linking any objectives into the world that are associated with it
 *
 * inputs:
 *  landscape: landscape the instance was spawned on
 *
 * return:
 *	true: post spawn successfull
 *  false: post spawn failed
 *
 ************************************************************************************************/
bool CRMInstance::PostSpawn ( CRandomTerrain* terrain, qboolean IsServer )
{
	if ( mObjective )
	{
		return mObjective->Link ( );
	}

	return true;
}
#ifndef DEDICATED
void CRMInstance::DrawAutomapSymbol()
{
	// draw proper symbol on map for instance
	switch (GetAutomapSymbol())
	{
		default:
		case AUTOMAP_NONE:
			if (HasObjective())
				CM_TM_AddObjective(GetOrigin()[0], GetOrigin()[1], GetSide());
			break;
		case AUTOMAP_BLD:
			CM_TM_AddBuilding(GetOrigin()[0], GetOrigin()[1], GetSide());
			if (HasObjective())
				CM_TM_AddObjective(GetOrigin()[0], GetOrigin()[1], GetSide());
			break;
		case AUTOMAP_OBJ:
			CM_TM_AddObjective(GetOrigin()[0], GetOrigin()[1], GetSide());
			break;
		case AUTOMAP_START:
			CM_TM_AddStart(GetOrigin()[0], GetOrigin()[1], GetSide());
			break;
		case AUTOMAP_END:
			CM_TM_AddEnd(GetOrigin()[0], GetOrigin()[1], GetSide());
			break;
		case AUTOMAP_ENEMY:
			if (HasObjective())
				CM_TM_AddObjective(GetOrigin()[0], GetOrigin()[1]);
			if (1 == Cvar_VariableIntegerValue("rmg_automapshowall"))
				CM_TM_AddNPC(GetOrigin()[0], GetOrigin()[1], false);
			break;
		case AUTOMAP_FRIEND:
			if (HasObjective())
				CM_TM_AddObjective(GetOrigin()[0], GetOrigin()[1]);
			if (1 == Cvar_VariableIntegerValue("rmg_automapshowall"))
				CM_TM_AddNPC(GetOrigin()[0], GetOrigin()[1], true);
			break;
		case AUTOMAP_WALL:
			CM_TM_AddWallRect(GetOrigin()[0], GetOrigin()[1], GetSide());
			break;
	}
}
#endif // !DEDICATED
/************************************************************************************************
 * CRMInstance::Preview
 *	Renderings debug information about the instance
 *
 * inputs:																   
 *  none
 *
 * return:
 *	none
 *
 ************************************************************************************************/
void CRMInstance::Preview ( const vec3_t from )
{
/*	CEntity				*tent;

	// Add a cylindar for the whole settlement
	tent = G_TempEntity( GetOrigin(), EV_DEBUG_CYLINDER );
	VectorCopy( GetOrigin(), tent->s.origin2 );
	tent->s.pos.trBase[2] += 40;
	tent->s.origin2[2] += 50;
	tent->s.time = 1050 + ((int)(GetSpacingRadius())<<16);
	tent->s.time2 = GetPreviewColor ( );
	G_AddTempEntity(tent);

	// Origin line
	tent = G_TempEntity( GetOrigin ( ), EV_DEBUG_LINE );
	VectorCopy( GetOrigin(), tent->s.origin2 );
	tent->s.origin2[2] += 400;
	tent->s.time = 1050;
	tent->s.weapon = 10;
	tent->s.time2 = (255<<24) + (255<<16) + (255<<8) + 255;
	G_AddTempEntity(tent);

	if ( GetFlattenRadius ( ) )
	{
		// Add a cylindar for the whole settlement
		tent = G_TempEntity( GetOrigin(), EV_DEBUG_CYLINDER );
		VectorCopy( GetOrigin(), tent->s.origin2 );
		tent->s.pos.trBase[2] += 40;
		tent->s.origin2[2] += 50;
		tent->s.time = 1050 + ((int)(GetFlattenRadius ( ))<<16);
		tent->s.time2 = (255<<24) + (80<<16) +(80<<8) + 80;
		G_AddTempEntity(tent);
	}
*/
}
