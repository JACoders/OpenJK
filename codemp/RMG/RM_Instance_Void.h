#pragma once
#if !defined(RM_INSTANCE_VOID_H_INC)
#define RM_INSTANCE_VOID_H_INC

#ifdef DEBUG_LINKING
	#pragma message("...including RM_Instance_Void.h")
#endif

class CRMVoidInstance : public CRMInstance
{
public:

	CRMVoidInstance ( CGPGroup* instGroup, CRMInstanceFile& instFile );

	virtual void		SetArea		( CRMAreaManager* amanager, CRMArea* area );
};

#endif