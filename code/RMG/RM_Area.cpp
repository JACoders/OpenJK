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
 *	Copyright (C) 2001-2002 Raven Software
 *
 *  RM_Area.cpp
 *
 ************************************************************************************************/

#include "../server/exe_headers.h"

#include "RM_Headers.h"

#ifdef _WIN32
#pragma optimize("p", on)
#endif

/************************************************************************************************
 * CRMArea::CRMArea
 *	constructor
 *
 * inputs:
 *  none
 *
 * return:
 *	none
 *
 ************************************************************************************************/
CRMArea::CRMArea ( 
	float	spacingRadius,
	float	paddingSize,
	float	confineRadius,
	vec3_t	confineOrigin,
	vec3_t	lookAtOrigin,
	bool	flatten,
	int		symmetric
	) 
{
	mMoveCount		= 0;
	mAngle			= 0;
	mCollision		= true;
	mConfineRadius	= confineRadius;
	mPaddingSize    = paddingSize;
	mSpacingRadius  = spacingRadius;
	mFlatten		= flatten;
	mLookAt			= true;
	mLockOrigin		= false;
	mSymmetric		= symmetric;
	mRadius			= spacingRadius;

	VectorCopy ( confineOrigin, mConfineOrigin );
	VectorCopy ( lookAtOrigin, mLookAtOrigin );
}

/************************************************************************************************
 * CRMArea::LookAt
 *	Angle the area towards the given point
 *
 * inputs:
 *  lookat - the origin to look at
 *
 * return:
 *	the angle in radians that was calculated
 *
 ************************************************************************************************/
float CRMArea::LookAt ( vec3_t lookat )
{
	if (mLookAt)
	{	// this area orients itself towards a point
		vec3_t a;

		VectorCopy ( lookat, mLookAtOrigin );
		VectorSubtract ( lookat, mOrigin, a );

		mAngle = atan2 ( a[1], a[0] );
	}

	return mAngle;
}

/************************************************************************************************
 * CRMArea::Mirror
 *	Mirrors the area to the other side of the map.  This includes mirroring the confine origin
 *  and lookat origin
 *
 * inputs:
 *  none
 *
 * return:
 *	none
 *
 ************************************************************************************************/
void CRMArea::Mirror ( void )
{
	mOrigin[0] = -mOrigin[0];
	mOrigin[1] = -mOrigin[1];

	mConfineOrigin[0] = -mConfineOrigin[0];
	mConfineOrigin[1] = -mConfineOrigin[1];

	mLookAtOrigin[0] = -mLookAtOrigin[0];
	mLookAtOrigin[1] = -mLookAtOrigin[1];
}

/************************************************************************************************
 * CRMAreaManager::CRMAreaManager
 *	constructor
 *
 * inputs:
 *  none
 *
 * return:
 *	none
 *
 ************************************************************************************************/
CRMAreaManager::CRMAreaManager ( const vec3_t mins, const vec3_t maxs)
{
	VectorCopy ( mins, mMins );
	VectorCopy ( maxs, mMaxs );

	mWidth  = mMaxs[0] - mMins[0];
	mHeight = mMaxs[1] - mMins[1];
}

/************************************************************************************************
 * CRMAreaManager::~CRMAreaManager
 *	Removes all managed areas
 *
 * inputs:
 *  none
 *
 * return:
 *	none
 *
 ************************************************************************************************/
CRMAreaManager::~CRMAreaManager ( )
{
	int i;

	for ( i = mAreas.size() - 1; i >=0; i -- )
	{
		delete mAreas[i];
	}

	mAreas.clear();
}

/************************************************************************************************
 * CRMAreaManager::MoveArea
 *	Moves an area within the area manager thus shifting any other areas as needed
 *
 * inputs:
 *  area   - area to be moved
 *  origin - new origin to attempt to move to
 *
 * return:
 *	none
 *
 ************************************************************************************************/
void CRMAreaManager::MoveArea ( CRMArea* movedArea, vec3_t origin)
{
	int	index;
	int size;

	// Increment the addcount (this is for infinite protection)
	movedArea->AddMoveCount ();
	
	// Infinite recursion prevention
	if ( movedArea->GetMoveCount() > 250 )
	{
//		assert ( 0 );
		movedArea->EnableCollision ( false );
		return;
	}

	// First set the area's origin, This may cause it to be in collision with
	// another area but that will get fixed later
	movedArea->SetOrigin ( origin );

	// when symmetric we want to ensure that no instances end up on the "other" side of the imaginary diaganol that cuts the map in two
	// mSymmetric tells us which side of the map is legal
	if ( movedArea->GetSymmetric ( ) )
	{
		const vec3pair_t& bounds = TheRandomMissionManager->GetLandScape()->GetBounds();
		
		vec3_t point;
		vec3_t dir;
		vec3_t tang;
		bool   push;
		float  len;

		VectorSubtract( movedArea->GetOrigin(), bounds[0], point );
		VectorSubtract( bounds[1], bounds[0], dir );
		VectorNormalize(dir);	

		dir[2] = 0;
		point[2] = 0;
		VectorMA( bounds[0], DotProduct(point, dir), dir, tang );
		VectorSubtract ( movedArea->GetOrigin(), tang, dir );

		dir[2] = 0;
		push   = false;
		len    = VectorNormalize(dir);

		if ( len < movedArea->GetRadius ( ) )
		{
			if ( movedArea->GetLockOrigin ( ) )
			{
				movedArea->EnableCollision ( false );
				return;
			}

			VectorMA ( point, (movedArea->GetSpacingRadius() - len) + TheRandomMissionManager->GetLandScape()->irand(10,movedArea->GetSpacingRadius()), dir, point );
			origin[0] = point[0] + bounds[0][0];
			origin[1] = point[1] + bounds[0][1];
			movedArea->SetOrigin ( origin );
		}

		switch ( movedArea->GetSymmetric ( ) )
		{
			case SYMMETRY_TOPLEFT:
				if ( origin[1] > origin[0] )
				{
					movedArea->Mirror ( );
				}
				break;

			case SYMMETRY_BOTTOMRIGHT:
				if ( origin[1] < origin[0] )
				{ 
					movedArea->Mirror ( );
				}

				break;

			default:
				// unknown symmetry type
				assert ( 0 );
				break;
		}
	}

	// Confine to area unless we are being pushed back by the same guy who pushed us last time (infinite loop)
	if ( movedArea->GetConfineRadius() )
	{
		if ( movedArea->GetMoveCount() < 25 )
		{
			vec3_t cdiff;
			float  cdist;

			VectorSubtract ( movedArea->GetOrigin(), movedArea->GetConfineOrigin(), cdiff );
			cdiff[2] = 0;
			cdist = VectorLength ( cdiff );

			if ( cdist + movedArea->GetSpacingRadius() > movedArea->GetConfineRadius() )
			{
				cdist = movedArea->GetConfineRadius() - movedArea->GetSpacingRadius();
				VectorNormalize ( cdiff );
		
				VectorMA ( movedArea->GetConfineOrigin(), cdist, cdiff, movedArea->GetOrigin());
			}	
		}
		else
		{
			index = 0;
		}
	}

	// See if it fell off the world in the x direction
	if ( movedArea->GetOrigin()[0] + movedArea->GetSpacingRadius() > mMaxs[0] )
		movedArea->GetOrigin()[0] = mMaxs[0] - movedArea->GetSpacingRadius() - (TheRandomMissionManager->GetLandScape()->irand(10,200));
	else if ( movedArea->GetOrigin()[0] - movedArea->GetSpacingRadius() < mMins[0] )
		movedArea->GetOrigin()[0] = mMins[0] + movedArea->GetSpacingRadius() + (TheRandomMissionManager->GetLandScape()->irand(10,200));

	// See if it fell off the world in the y direction
	if ( movedArea->GetOrigin()[1] + movedArea->GetSpacingRadius() > mMaxs[1] )
		movedArea->GetOrigin()[1] = mMaxs[1] - movedArea->GetSpacingRadius() - (TheRandomMissionManager->GetLandScape()->irand(10,200));
	else if ( movedArea->GetOrigin()[1] - movedArea->GetSpacingRadius() < mMins[1] )
		movedArea->GetOrigin()[1] = mMins[1] + movedArea->GetSpacingRadius() + (TheRandomMissionManager->GetLandScape()->irand(10,200));

	// Look at what we need to look at
	movedArea->LookAt ( movedArea->GetLookAtOrigin() );
	
	// Dont collide against things that have no collision
//	if ( !movedArea->IsCollisionEnabled ( ) )
//	{
//		return;
//	}

	// See if its colliding
	for(index = 0, size = mAreas.size(); index < size; index ++ )
	{
		CRMArea	*area = mAreas[index];
		vec3_t	diff;
		vec3_t	newOrigin;
		float	dist;
		float	targetdist;

		// Skip the one that was moved in the first place
		if ( area == movedArea )
		{
			continue;
		}

		if ( area->GetLockOrigin ( ) && movedArea->GetLockOrigin( ) )
		{
			continue;
		}

		// Dont collide against things that have no collision
		if ( !area->IsCollisionEnabled ( ) )
		{
			continue;
		}

		// Grab the distance between the two
		// only want the horizontal distance -- dmv
		//dist		= Distance ( movedArea->GetOrigin ( ), area->GetOrigin ( ));
		vec3_t	maOrigin;
		vec3_t	aOrigin;
		VectorCopy(movedArea->GetOrigin(), maOrigin);
		VectorCopy(area->GetOrigin(), aOrigin);
		maOrigin[2] = aOrigin[2] = 0;
		dist		= Distance ( maOrigin, aOrigin );
		targetdist  = movedArea->GetSpacingRadius() + area->GetSpacingRadius() + maximum(movedArea->GetPaddingSize(),area->GetPaddingSize());

		if ( dist == 0 )
		{
			area->GetOrigin()[0] += (50 * (float)(TheRandomMissionManager->GetLandScape()->irand(0,99))/100.0f);
			area->GetOrigin()[1] += (50 * (float)(TheRandomMissionManager->GetLandScape()->irand(0,99))/100.0f);

			VectorCopy(area->GetOrigin(), aOrigin);
			aOrigin[2] = 0;

			dist = Distance ( maOrigin, aOrigin );
		}

		// Are they are enough apart?
		if ( dist >= targetdist )
		{
			continue;	
		}

		// Dont move a step if locked
		if ( area->GetLockOrigin ( ) )
		{
			MoveArea ( area, area->GetOrigin ( ) );			
			continue;
		}

		// we got a collision, move the guy we hit
		VectorSubtract ( area->GetOrigin(), movedArea->GetOrigin(), diff );
		diff[2] = 0;
		VectorNormalize ( diff );

		// Push by the difference in the distance and no-collide radius
		VectorMA ( area->GetOrigin(), targetdist - dist + 1 , diff, newOrigin );

		// Move the area now
		MoveArea ( area, newOrigin );
	}
}

/************************************************************************************************
 * CRMAreaManager::CreateArea
 *	Creates an area and adds it to the list of managed areas
 *
 * inputs:
 *  none
 *
 * return:
 *	a pointer to the newly added area class
 *
 ************************************************************************************************/
CRMArea* CRMAreaManager::CreateArea ( 
	vec3_t	origin, 
	float	spacingRadius,
	int		spacingLine,
	float	paddingSize,
	float	confineRadius,
	vec3_t	confineOrigin,
	vec3_t	lookAtOrigin,
	bool	flatten,
	bool	collide,
	bool	lockorigin,
	int		symmetric
	)
{
	CRMArea* area = new CRMArea ( spacingRadius, paddingSize, confineRadius, confineOrigin, lookAtOrigin, flatten, symmetric );

	if ( lockorigin || spacingLine )
	{
		area->LockOrigin ( );
	}

	if (origin[0] != lookAtOrigin[0] || origin[1] != lookAtOrigin[1])
		area->EnableLookAt(true);

	// First add the area to the list
	mAreas.push_back ( area );

	area->EnableCollision(collide);

	// Set the real radius which is used for center line detection
	if ( spacingLine )
	{
		area->SetRadius ( spacingRadius + (spacingLine - 1) * spacingRadius );
	}

	// Now move the area around
	MoveArea ( area, origin );

	if ( (origin[0] != lookAtOrigin[0] || origin[1] != lookAtOrigin[1]) )
	{
		int i;
		vec3_t	 linedir;
		vec3_t	 dir;
		vec3_t   up = {0,0,1};
		vec3_t	 zerodvec;

		VectorClear(zerodvec);

		VectorSubtract ( lookAtOrigin, origin, dir );
		VectorNormalize ( dir );
		dir[2] = 0;
		CrossProduct ( dir, up, linedir );

		for ( i = 0; i < spacingLine - 1; i ++ )
		{
			CRMArea* linearea;
			vec3_t	 lineorigin;

			linearea = new CRMArea ( spacingRadius, paddingSize, 0, zerodvec, zerodvec, false, symmetric );
			linearea->LockOrigin ( );
			linearea->EnableCollision(collide);

			VectorMA ( origin, spacingRadius + (spacingRadius * 2 * i), linedir, lineorigin );
			mAreas.push_back ( linearea );				
			MoveArea ( linearea, lineorigin );

			linearea = new CRMArea ( spacingRadius, paddingSize, 0, zerodvec, zerodvec, false, symmetric );
			linearea->LockOrigin ( );
			linearea->EnableCollision(collide);

			VectorMA ( origin, -spacingRadius - (spacingRadius * 2 * i), linedir, lineorigin );
			mAreas.push_back ( linearea );				
			MoveArea ( linearea, lineorigin );
		}
	}

	// Return it for convienience
	return area;
}

/************************************************************************************************
 * CRMAreaManager::EnumArea
 *	Allows for enumeration through the area list. If an invalid index is given then NULL will
 *  be returned;
 *
 * inputs:
 *  index - current enumeration index
 *
 * return:
 *	requested area class pointer or NULL if the index was invalid
 *
 ************************************************************************************************/
CRMArea* CRMAreaManager::EnumArea ( const int index )
{
	// This isnt an assertion case because there is no size method for
	// the area manager so the areas are enumerated until NULL is returned.
	if ( index < 0 || index >= mAreas.size ( ) )
	{
		return NULL;
	}

	return mAreas[index];
}

#ifdef _WIN32
#pragma optimize("p", off)
#endif
