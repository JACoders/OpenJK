#pragma once
#if !defined(RM_MANAGER_H_INC)
#define RM_MANAGER_H_INC

#if !defined(CM_LANDSCAPE_H_INC)
#include "../qcommon/cm_landscape.h"
#endif

class CRMManager
{
private:

	CRMMission*			mMission;
	CCMLandScape*		mLandScape;
	CRandomTerrain*		mTerrain;
	int					mPreviewTimer;
	int					mCurPriority;
	bool				mUseTimeLimit;

	void			UpdateStatisticCvars ( void );

public:

	// Constructors
	CRMManager (void);
	~CRMManager (void);

	bool			LoadMission		( qboolean IsServer );
	bool			SpawnMission	( qboolean IsServer );

	// Accessors
	void			SetLandScape	(CCMLandScape *landscape);
	void			SetCurPriority	(int priority) { mCurPriority = priority; }

	CRandomTerrain*	GetTerrain		(void)	{ return mTerrain; }
	CCMLandScape*	GetLandScape	(void)	{ return mLandScape; }
	CRMMission*		GetMission		(void)  { return mMission; }
	int				GetCurPriority	(void) { return mCurPriority; }

	void			Preview			( const vec3_t from );

	bool			IsMissionComplete		(void);
	bool			HasTimeExpired			(void);
	void			CompleteObjective		( CRMObjective *obj );
	void			CompleteMission			(void);
	void			FailedMission			(bool TimeExpired);

	// eek
	static CRMObjective	*mCurObjective;
};

extern CRMManager*	TheRandomMissionManager;


#endif // RANDOMMISSION_H_INC