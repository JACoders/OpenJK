#pragma once

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
