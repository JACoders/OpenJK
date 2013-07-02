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
