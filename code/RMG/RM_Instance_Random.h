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
#if !defined(RM_INSTANCE_RANDOM_H_INC)
#define RM_INSTANCE_RANDOM_H_INC

#ifdef DEBUG_LINKING
	#pragma message("...including RM_Instance_Random.h")
#endif

#define	MAX_RANDOM_INSTANCES		64

class CRMRandomInstance : public CRMInstance
{
protected:

	CRMInstance*	mInstance;

public:

	CRMRandomInstance ( CGPGroup* instGroup, CRMInstanceFile& instFile );
	~CRMRandomInstance ( );

	virtual bool		IsValid				( )	{ return mInstance==NULL?false:true; }

	virtual int			GetPreviewColor		( )		{ return mInstance->GetPreviewColor ( ); }

	virtual float		GetSpacingRadius	( )		{ return mInstance->GetSpacingRadius ( ); }
	virtual int			GetSpacingLine		( )		{ return mInstance->GetSpacingLine ( ); }
	virtual float		GetFlattenRadius	( )		{ return mInstance->GetFlattenRadius ( ); }
	virtual bool		GetLockOrigin		( )		{ return mInstance->GetLockOrigin ( ); }

	virtual void		SetFilter			( const char *filter );
	virtual void		SetTeamFilter		( const char *teamFilter );
	virtual void		SetArea				( CRMAreaManager* amanager, CRMArea* area );
	virtual void  		SetMirror			(int mirror);

	virtual bool		PreSpawn			( CRandomTerrain* terrain, qboolean IsServer );
	virtual bool		Spawn				( CRandomTerrain* terrain, qboolean IsServer );
};

#endif