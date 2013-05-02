#pragma once

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
