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

#pragma once
#if !defined(RM_INSTANCE_BSP_H_INC)
#define RM_INSTANCE_BSP_H_INC

#ifdef DEBUG_LINKING
	#pragma message("...including RM_Instance_BSP.h")
#endif

class CRMBSPInstance : public CRMInstance
{
private:

	char		mBsp[MAX_QPATH];
	float		mAngleVariance;
	float		mBaseAngle;
	float		mAngleDiff;

	float		mHoleRadius;

public:

	CRMBSPInstance	 ( CGPGroup *instance, CRMInstanceFile& instFile );

	virtual int			GetPreviewColor		( )		{ return (255<<24)+255; }

	virtual float		GetHoleRadius		( ) { return mHoleRadius; }

	virtual bool		Spawn				( CRandomTerrain* terrain, qboolean IsServer );

	const char*			GetModelName	 (void) const { return(mBsp); }
	float				GetAngleDiff	 (void) const { return(mAngleDiff); }
	bool				GetAngularType	 (void) const { return(mAngleDiff != 0.0f); }
};

#endif