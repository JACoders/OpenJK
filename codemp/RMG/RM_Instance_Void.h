#pragma once

#ifdef DEBUG_LINKING
	#pragma message("...including RM_Instance_Void.h")
#endif

class CRMVoidInstance : public CRMInstance
{
public:

	CRMVoidInstance ( CGPGroup* instGroup, CRMInstanceFile& instFile );

	virtual void		SetArea		( CRMAreaManager* amanager, CRMArea* area );
};
