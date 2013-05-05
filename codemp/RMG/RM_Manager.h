#pragma once

#include "qcommon/cm_landscape.h"

class CRMManager
{
private:

	CRMMission*				mMission;
	CCMLandScape*			mLandScape;
	CRandomTerrain*			mTerrain;
	int						mPreviewTimer;
	int						mCurPriority;
	bool					mUseTimeLimit;

	rmAutomapSymbol_t		mAutomapSymbols[MAX_AUTOMAP_SYMBOLS];
	int						mAutomapSymbolCount;

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

	void				AddAutomapSymbol ( int type, vec3_t origin, int side );
	int					GetAutomapSymbolCount ( void );
	rmAutomapSymbol_t*	GetAutomapSymbol ( int index );
	static void			ProcessAutomapSymbols ( int count, rmAutomapSymbol_t* symbols );

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
