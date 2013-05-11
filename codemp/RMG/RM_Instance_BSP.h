#pragma once

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
