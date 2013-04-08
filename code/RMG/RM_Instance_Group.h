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
#if !defined(RM_INSTANCE_GROUP_H_INC)
#define RM_INSTANCE_GROUP_H_INC

#ifdef DEBUG_LINKING
	#pragma message("...including RM_Instance_Group.h")
#endif

class CRMGroupInstance : public CRMInstance
{
protected:

	rmInstanceList_t	mInstances;
	float				mConfineRadius;
	float				mPaddingSize;

public:

	CRMGroupInstance( CGPGroup* instGroup, CRMInstanceFile& instFile);
	~CRMGroupInstance();

	virtual bool		PreSpawn			( CRandomTerrain* terrain, qboolean IsServer );
	virtual bool		Spawn				( CRandomTerrain* terrain, qboolean IsServer );

	virtual void		Preview				( const vec3_t from );

	virtual void		SetFilter			( const char *filter );
	virtual void		SetTeamFilter		( const char *teamFilter );
	virtual void		SetArea				( CRMAreaManager* amanager, CRMArea* area );

	virtual int			GetPreviewColor		( )		{ return (255<<24)+(255<<8); }
	virtual float		GetSpacingRadius	( )		{ return 0; }
	virtual float		GetFlattenRadius	( )		{ return 0; }
	virtual void  		SetMirror(int mirror);

protected:

	void	RemoveInstances	 ( );
};

#endif