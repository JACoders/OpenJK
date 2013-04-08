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
 *  RM_Area.h
 *
 ************************************************************************************************/

#pragma once
#if !defined(RM_AREA_H_INC)
#define RM_AREA_H_INC

#ifdef DEBUG_LINKING
	#pragma message("...including RM_Area.h")
#endif

class CRMArea
{
private:

	float		mPaddingSize;
	float		mSpacingRadius;
	float		mConfineRadius;
	float		mRadius;
	float		mAngle;
	int			mMoveCount;
	vec3_t		mOrigin;
	vec3_t		mConfineOrigin;
	vec3_t		mLookAtOrigin;
	bool		mCollision;
	bool		mFlatten;
	bool		mLookAt;
	bool		mLockOrigin;
	int			mSymmetric;

public:

	CRMArea ( float spacing, float padding, float confine, vec3_t confineOrigin, vec3_t lookAtOrigin, bool flatten = true, int symmetric = 0 );

	void	Mirror ( void );

	void	SetOrigin(vec3_t origin) { VectorCopy ( origin, mOrigin ); }
	void	SetAngle(float angle)	 { mAngle = angle; }
	void	SetSymmetric(int sym)    { mSymmetric = sym; }

	void	EnableCollision(bool e)	 { mCollision = e; }
	void	EnableLookAt(bool la) {mLookAt = la; }

	float	LookAt(vec3_t lookat);
	void	LockOrigin( void )	{ mLockOrigin = true; }

	void	AddMoveCount()		{ mMoveCount++; }
	void	ClearMoveCount()	{ mMoveCount=0; }

	float	GetPaddingSize()	{ return mPaddingSize; }
	float	GetSpacingRadius()	{ return mSpacingRadius; }
	float	GetRadius()			{ return mRadius; }
	float	GetConfineRadius()	{ return mConfineRadius; }
	float	GetAngle()			{ return mAngle; }
	int		GetMoveCount()		{ return mMoveCount; }
	vec_t*	GetOrigin()			{ return mOrigin; }
	vec_t*	GetConfineOrigin()	{ return mConfineOrigin; }
	vec_t*	GetLookAtOrigin()	{ return mLookAtOrigin; }
	bool	GetLookAt()			{ return mLookAt;}
	bool	GetLockOrigin()		{ return mLockOrigin; }
	int		GetSymmetric()		{ return mSymmetric; }

	void	SetRadius(float r)	{ mRadius = r; }

	bool	IsCollisionEnabled(){ return mCollision; }
	bool	IsFlattened		  (){ return mFlatten; }
};

typedef vector<CRMArea*>			rmAreaVector_t;

class CRMAreaManager
{
private:

	rmAreaVector_t	mAreas;
	vec3_t			mMins;
	vec3_t			mMaxs;
	float			mWidth;
	float			mHeight;

public:
	
	CRMAreaManager ( const vec3_t mins, const vec3_t maxs );
	~CRMAreaManager ( );

	CRMArea*	CreateArea		( vec3_t origin, float spacing, int spacingline, float padding, float confine, vec3_t confineOrigin, vec3_t lookAtOrigin, bool flatten=true, bool collide=true, bool lockorigin=false, int symmetric=0);
	void		MoveArea		( CRMArea* area, vec3_t origin);
	CRMArea*	EnumArea		( const int index );

//	void		CreateMap		( void );
};

#endif

