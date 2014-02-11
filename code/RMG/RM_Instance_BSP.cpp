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
 * RM_Instance_BSP.cpp
 *
 * Implements the CRMBSPInstance class.  This class is reponsible for parsing a 
 * bsp instance as well as spawning it into a landscape.
 *
 ************************************************************************************************/

#include "../server/exe_headers.h"

#include "../qcommon/cm_local.h"
#include "../server/server.h"
#include "RM_Headers.h"

#include "RM_Instance_BSP.h"

#include "../client/vmachine.h"

/************************************************************************************************
 * CRMBSPInstance::CRMBSPInstance
 *	constructs a building instance object using the given parser group
 *
 * inputs:
 *  instance:  parser group containing information about the building instance
 *
 * return:
 *	none
 *
 ************************************************************************************************/
CRMBSPInstance::CRMBSPInstance(CGPGroup *instGroup, CRMInstanceFile& instFile)  : CRMInstance ( instGroup, instFile )
{
	strcpy(mBsp, instGroup->FindPairValue("file", ""));

	mAngleVariance	= DEG2RAD(atof(instGroup->FindPairValue("anglevariance", "0")));
	mBaseAngle		= DEG2RAD(atof(instGroup->FindPairValue("baseangle", "0")));
	mAngleDiff		= DEG2RAD(atof(instGroup->FindPairValue("anglediff", "0")));
	mSpacingRadius	= atof( instGroup->FindPairValue ( "spacing", "100" ) );
	mSpacingLine	= atoi( instGroup->FindPairValue ( "spacingline", "0" ) );
	mSurfaceSprites = (!Q_stricmp ( instGroup->FindPairValue ( "surfacesprites", "no" ), "yes")) ? true : false;
	mLockOrigin     = (!Q_stricmp ( instGroup->FindPairValue ( "lockorigin", "no" ), "yes")) ? true : false;
	mFlattenRadius	= atof( instGroup->FindPairValue ( "flatten", "0" ) );
	mHoleRadius		= atof( instGroup->FindPairValue ( "hole", "0" ) );

	const char * automapSymName = instGroup->FindPairValue ( "automap_symbol", "building" );
	if (0 == Q_stricmp(automapSymName, "none"))	   	mAutomapSymbol = AUTOMAP_NONE ;
	else if (0 == Q_stricmp(automapSymName, "building"))  	mAutomapSymbol = AUTOMAP_BLD  ;
	else if (0 == Q_stricmp(automapSymName, "objective")) 	mAutomapSymbol = AUTOMAP_OBJ  ;
	else if (0 == Q_stricmp(automapSymName, "start"))	   	mAutomapSymbol = AUTOMAP_START;
	else if (0 == Q_stricmp(automapSymName, "end"))	   	mAutomapSymbol = AUTOMAP_END  ;
	else if (0 == Q_stricmp(automapSymName, "enemy"))	   	mAutomapSymbol = AUTOMAP_ENEMY;
	else if (0 == Q_stricmp(automapSymName, "friend"))	   	mAutomapSymbol = AUTOMAP_FRIEND;
	else if (0 == Q_stricmp(automapSymName, "wall"))	   	mAutomapSymbol = AUTOMAP_WALL;
	else mAutomapSymbol	= atoi( automapSymName );

	// optional instance objective strings
	SetMessage(instGroup->FindPairValue("objective_message",""));
	SetDescription(instGroup->FindPairValue("objective_description",""));
	SetInfo(instGroup->FindPairValue("objective_info",""));

	mBounds[0][0] = 0;
	mBounds[0][1] = 0;
	mBounds[1][0] = 0;
	mBounds[1][1] = 0;

	mBaseAngle += (TheRandomMissionManager->GetLandScape()->irand(0,mAngleVariance) - mAngleVariance/2);
}

/************************************************************************************************
 * CRMBSPInstance::Spawn
 *	spawns a bsp into the world using the previously aquired origin
 *
 * inputs:
 *  none
 *
 * return:
 *	none
 *
 ************************************************************************************************/
bool CRMBSPInstance::Spawn ( CRandomTerrain* terrain, qboolean IsServer)
{
//	TEntity*	ent;
	float		yaw;
	char		temp[10000];
	char		*savePtr;
	vec3_t		origin;
	vec3_t		notmirrored;
	float	water_level = terrain->GetLandScape()->GetWaterHeight();

	const vec3_t&	  terxelSize = terrain->GetLandScape()->GetTerxelSize ( );
	const vec3pair_t& bounds     = terrain->GetLandScape()->GetBounds();

	// If this entity somehow lost its collision flag then boot it
	if ( !GetArea().IsCollisionEnabled ( ) )
	{
		return false;
	}

	// copy out the unmirrored version
	VectorCopy(GetOrigin(), notmirrored);

	// we want to mirror it before determining the Z value just in case the landscape isn't perfectly mirrored
	if (mMirror)
	{
		GetOrigin()[0] = TheRandomMissionManager->GetLandScape()->GetBounds()[0][0] + TheRandomMissionManager->GetLandScape()->GetBounds()[1][0] - GetOrigin()[0];
		GetOrigin()[1] = TheRandomMissionManager->GetLandScape()->GetBounds()[0][1] + TheRandomMissionManager->GetLandScape()->GetBounds()[1][1] - GetOrigin()[1];
	}

	// Align the instance to the center of a terxel
	GetOrigin ( )[0] = bounds[0][0] + (int)((GetOrigin ( )[0] - bounds[0][0] + terxelSize[0] / 2) / terxelSize[0]) * terxelSize[0];
	GetOrigin ( )[1] = bounds[0][1] + (int)((GetOrigin ( )[1] - bounds[0][1] + terxelSize[1] / 2) / terxelSize[1]) * terxelSize[1];

	// Make sure the bsp is resting on the ground, not below or above it
	// NOTE: This check is basically saying "is this instance not a bridge", because when instances are created they are all
	// placed above the world's Z boundary, EXCEPT FOR BRIDGES. So this call to GetWorldHeight will move all other instances down to
	// ground level except bridges 
	if ( GetOrigin()[2] > terrain->GetBounds()[1][2] )
	{
		if( GetFlattenRadius() )
		{
			terrain->GetLandScape()->GetWorldHeight ( GetOrigin(), GetBounds ( ), false );
			GetOrigin()[2] += 5;
		}
		else if (IsServer)
		{	// if this instance does not flatten the ground around it, do a trace to more accurately determine its Z value
			trace_t		tr;
			vec3_t		end;
			vec3_t		start;

			VectorCopy(GetOrigin(), end);
			VectorCopy(GetOrigin(), start);
			// start the trace below the top height of the landscape
			start[2] = TheRandomMissionManager->GetLandScape()->GetBounds()[1][2] - 1;
			// end the trace at the bottom of the world
			end[2] = MIN_WORLD_COORD;
	
			memset ( &tr, 0, sizeof ( tr ) );
			SV_Trace( &tr, start, vec3_origin, vec3_origin, end, ENTITYNUM_NONE, CONTENTS_TERRAIN|CONTENTS_SOLID, G2_NOCOLLIDE, 0); //qfalse, 0, 10 );

			if( !(tr.contents & CONTENTS_TERRAIN) || (tr.fraction == 1.0) )
			{
				if ( 0 )
				assert(0); // this should never happen

				// restore the unmirrored origin
				VectorCopy( notmirrored, GetOrigin() );
				// don't spawn
				return false;
			}
			// assign the Z-value to wherever it hit the terrain	
			GetOrigin()[2] = tr.endpos[2];
			// lower it a little, otherwise the bottom of the instance might be exposed if on some weird sloped terrain
			GetOrigin()[2] -= 16; // FIXME: would it be better to use a number related to the instance itself like 1/5 it's height or something...
		}

	}
	else
	{
		terrain->GetLandScape()->GetWorldHeight ( GetOrigin(), GetBounds ( ), true );
	}
	
	// save away the origin
	VectorCopy(GetOrigin(), origin);
	// make sure not to spawn if in water
	if (!HasObjective() && GetOrigin()[2] < water_level)
		return false;
	// restore the origin
	VectorCopy(origin, GetOrigin());

	if (mMirror)
	{	// change blue things to red for symmetric maps
		if (strlen(mFilter) > 0)
		{
			char * blue = strstr(mFilter,"blue");
			if (blue)
			{
				blue[0] = (char) 0;
				strcat(mFilter, "red");
				SetSide(SIDE_RED);
			}
		}
		if (strlen(mTeamFilter) > 0)
		{
			char * blue = strstr(mTeamFilter,"blue");
			if (blue)
			{
				strcpy(mTeamFilter, "red");
				SetSide(SIDE_RED);
			}
		}
		yaw = RAD2DEG(mArea->GetAngle() + mBaseAngle) + 180;
	}
	else
	{
		yaw = RAD2DEG(mArea->GetAngle() + mBaseAngle);
	}

/*
	if( TheRandomMissionManager->GetMission()->GetSymmetric() )
	{
		vec3_t	diagonal;
		vec3_t	lineToPoint;
		vec3_t	mins;
		vec3_t	maxs;
		vec3_t	point;
		vec3_t	vProj;
		vec3_t	vec;
		float	distance;

		VectorCopy( TheRandomMissionManager->GetLandScape()->GetBounds()[1], maxs );
		VectorCopy( TheRandomMissionManager->GetLandScape()->GetBounds()[0], mins );
		VectorCopy( GetOrigin(), point );
		mins[2] = maxs[2] = point[2] = 0;
		VectorSubtract( point, mins, lineToPoint );
		VectorSubtract( maxs, mins, diagonal);


		VectorNormalize(diagonal);	
		VectorMA( mins, DotProduct(lineToPoint, diagonal), diagonal, vProj);
		VectorSubtract(point, vProj, vec );
		distance = VectorLength(vec);

		// if an instance is too close to the imaginary diagonal that cuts the world in half, don't spawn it
		// otherwise you can get overlapping instances
		if( distance < GetSpacingRadius() )
		{
#ifdef _DEBUG
			mAutomapSymbol = AUTOMAP_END;
#endif
			if( !HasObjective() )
			{
				return false;
			}
		}
	}
*/

	// Spawn in the bsp model
	sprintf(temp, 
		"{\n"
		"\"classname\"   \"misc_bsp\"\n"
		"\"bspmodel\"    \"%s\"\n"
		"\"origin\"      \"%f %f %f\"\n"
		"\"angles\"      \"0 %f 0\"\n"
		"\"filter\"      \"%s\"\n"
		"\"teamfilter\"  \"%s\"\n"
		"\"spacing\"	 \"%d\"\n"
		"\"flatten\"	 \"%d\"\n"
		"}\n",
		mBsp,
		GetOrigin()[0], GetOrigin()[1], GetOrigin()[2],
		AngleNormalize360(yaw),
		mFilter,
		mTeamFilter,
		(int)GetSpacingRadius(),
		(int)GetFlattenRadius()
		);

	if (IsServer)
	{	// only allow for true spawning on the server
		savePtr = sv.entityParsePoint;
		sv.entityParsePoint = temp;
//		VM_Call( cgvm, GAME_SPAWN_RMG_ENTITY );
	//	char *s;
		int bufferSize = 1024;
		char buffer[1024];

	//	s = COM_Parse( (const char **)&sv.entityParsePoint );
		Q_strncpyz( buffer, sv.entityParsePoint, bufferSize );
		if ( sv.entityParsePoint && sv.entityParsePoint[0] ) 
		{
			ge->GameSpawnRMGEntity(sv.entityParsePoint);
		}
		sv.entityParsePoint = savePtr;
	}

	
#ifndef DEDICATED
	DrawAutomapSymbol();
#endif
	Com_DPrintf( "RMG:  Building '%s' spawned at (%f %f %f)\n", mBsp, GetOrigin()[0], GetOrigin()[1], GetOrigin()[2] );
	// now restore the instances un-mirrored origin
	// NOTE: all this origin flipping, setting the side etc... should be done when mMirror is set
	// because right after this function is called, mMirror is set to 0 but all the instance data is STILL MIRRORED -- not good
	VectorCopy(notmirrored, GetOrigin());

	return true;
}



