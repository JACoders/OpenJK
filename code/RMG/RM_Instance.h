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
#if !defined(RM_INSTANCE_H_INC)
#define RM_INSTANCE_H_INC

#ifdef DEBUG_LINKING
	#pragma message("...including RM_Instance.h")
#endif

#if !defined(CM_LANDSCAPE_H_INC)
#include "../qcommon/cm_landscape.h"
#endif

enum	CRMAutomapSymbol
{
	AUTOMAP_NONE = 0,
	AUTOMAP_BLD  = 1,
	AUTOMAP_OBJ  = 2,
	AUTOMAP_START= 3,
	AUTOMAP_END  = 4,
	AUTOMAP_ENEMY= 5,
	AUTOMAP_FRIEND=6,
	AUTOMAP_WALL=7
};

class CRMInstance
{
protected:
	char			mFilter[MAX_QPATH];			// filter of entities inside of this
	char			mTeamFilter[MAX_QPATH];		// team specific filter

	vec3pair_t		mBounds;		// Bounding box for instance itself

	CRMArea*		mArea;			// Position of the instance

	CRMObjective*	mObjective;		// Objective associated with this instance

	// optional instance specific strings for objective
	string			mMessage;		// message outputed when objective is completed
	string			mDescription;	// description of objective
	string			mInfo;			// more info for objective

	float			mSpacingRadius;	// Radius to space instances with
	float			mFlattenRadius;	// Radius to flatten under instances

	int				mSpacingLine;	// Line of spacing radius's, forces locket
	bool			mLockOrigin;	// Origin cant move

	bool			mSurfaceSprites; // allow surface sprites under instance?

	int				mAutomapSymbol; // show which symbol on automap 0=none

	int				mEntityID;		// id of entity spawned
	int				mSide;			// blue or red side
	int				mMirror;		// mirror origin, angle

	int				mFlattenHeight;	// height to flatten land

public:
	
	CRMInstance ( CGPGroup* instance, CRMInstanceFile& instFile);

	virtual ~CRMInstance ( ) { }

	virtual bool		IsValid				( )	{ return true; }

	virtual bool		PreSpawn			( CRandomTerrain* terrain, qboolean IsServer );
	virtual bool		Spawn				( CRandomTerrain* terrain, qboolean IsServer ) { return false; }
	virtual bool		PostSpawn			( CRandomTerrain* terrain, qboolean IsServer );

	virtual void		Preview				( const vec3_t from );

	virtual void		SetArea				( CRMAreaManager* amanager, CRMArea* area ) { mArea = area; }
	virtual void		SetFilter			( const char *filter ) { Q_strncpyz(mFilter, filter, 64); }
	virtual void		SetTeamFilter		( const char *teamFilter ) { Q_strncpyz(mTeamFilter, teamFilter, 64); }
	void				SetObjective		( CRMObjective* obj ) { mObjective = obj; }
	CRMObjective*		GetObjective		(void) {return mObjective;}
	bool				HasObjective		() {return mObjective != NULL;}
	int					GetAutomapSymbol	() {return mAutomapSymbol;}
	void				DrawAutomapSymbol	();
	const char*			GetMessage(void)	{ return mMessage.c_str(); }
	const char*			GetDescription(void){ return mDescription.c_str(); }
	const char*			GetInfo(void)		{ return mInfo.c_str(); }
	void				SetMessage(const char* msg) { mMessage = msg; }
	void				SetDescription(const char* desc) { mDescription = desc; }
	void				SetInfo(const char* info) { mInfo = info; }
	void				SetSide(int side)	{mSide = side;}
	int					GetSide				( ) {return mSide;}

	// NOTE: should consider making SetMirror also set all other variables that need flipping
	// like the origin and Side, etc... Otherwise an Instance may have had it's origin flipped
	// but then later will have mMirror set to false, but the origin is still flipped. So any functions
	// that look at the instance later will see mMirror set to false, but not realize the origin has ALREADY been flipped
	virtual void  		SetMirror(int mirror)	{ mMirror = mirror;}
	int					GetMirror			( ) { return mMirror;}

	virtual bool		GetSurfaceSprites	( )		{ return mSurfaceSprites; }

	virtual bool		GetLockOrigin		( )		{ return mLockOrigin; }
	virtual int			GetSpacingLine		( )		{ return mSpacingLine; }

	virtual int			GetPreviewColor		( )		{ return 0; }
	virtual float		GetSpacingRadius	( )		{ return mSpacingRadius; }
	virtual float		GetFlattenRadius	( )		{ return mFlattenRadius; }
	const char			*GetFilter			( )		{ return mFilter; }
	const char			*GetTeamFilter		( )		{ return mTeamFilter; }
	
	CRMArea&			GetArea				( )		{ return *mArea; }
	vec_t*				GetOrigin			( ) 	{return mArea->GetOrigin(); }
	float				GetAngle			( )		{return mArea->GetAngle();}
	void				SetAngle(float ang )		{ mArea->SetAngle(ang);}
	const vec3pair_t&	GetBounds(void) const		{ return(mBounds); }

	void				SetFlattenHeight	( int height ) { mFlattenHeight = height; }
	int					GetFlattenHeight	( void )	   { return mFlattenHeight; }

	void				SetSpacingRadius	(float spacing) { mSpacingRadius = spacing; }
};

typedef list<CRMInstance*>::iterator	rmInstanceIter_t;
typedef list<CRMInstance*>				rmInstanceList_t;

#endif
