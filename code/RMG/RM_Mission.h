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
#if !defined(RM_MISSION_H_INC)
#define RM_MISSION_H_INC

#ifdef DEBUG_LINKING
	#pragma message("...including RM_Mission.h")
#endif

// maximum random choices
#define MAX_RANDOM_CHOICES 100

typedef vector<int> 	rmIntVector_t;


class CRMMission
{
private:

	rmObjectiveList_t		mObjectives;
	rmInstanceList_t		mInstances;

	CRMInstanceFile			mInstanceFile;
	CRMObjective*			mCurrentObjective;

	bool					mValidNodes;
	bool					mValidPaths;
	bool					mValidRivers;
	bool					mValidWeapons;
	bool					mValidAmmo;
	bool					mValidObjectives;
	bool					mValidInstances;

	int						mTimeLimit;
	int						mMaxInstancePosition;

	// npc multipliers
	float					mAccuracyMultiplier;
	float					mHealthMultiplier;

	// % chance that RMG pickup is actually spawned
	float					mPickupHealth;
	float					mPickupArmor;
	float					mPickupAmmo;
	float					mPickupWeapon;
	float					mPickupEquipment;

	string					mDescription;
	string					mExitScreen;
	string					mTimeExpiredScreen;

	// symmetric landscape style
	symmetry_t				mSymmetric;

	// if set to 1 in the mission file, adds an extra connecting path in symmetric maps
	// to ensure both sides actually do connect
	int						mBackUpPath;

	int						mDefaultPadding;

	CRMAreaManager*			mAreaManager;

	CRMPathManager*			mPathManager;

	CRandomTerrain*			mLandScape;

public:	

	CRMMission ( CRandomTerrain* );
	~CRMMission ( );

	bool			Load					( const char* name, const char* instances, const char* difficulty );
	bool			Spawn					( CRandomTerrain* terrain, qboolean IsServer );

	void			Preview					( const vec3_t from );									
										
	CRMObjective*	FindObjective			( const char* name );
	CRMObjective*	GetCurrentObjective		( ) { return mCurrentObjective; }
	
	void			CompleteMission			(void);
	void			FailedMission			(bool TimeExpired);
	void			CompleteObjective		( CRMObjective* ojective );

	int				GetTimeLimit			(void)	{ return mTimeLimit; }
	int				GetMaxInstancePosition	(void)  { return mMaxInstancePosition; }
	const char*		GetDescription			(void)  { return mDescription.c_str(); }
	const char*		GetExitScreen			(void)  { return mExitScreen.c_str(); }
	int				GetSymmetric			(void)  { return mSymmetric; }
	int				GetBackUpPath			(void)  { return mBackUpPath; }
	int				GetDefaultPadding		(void)  { return mDefaultPadding; }

//	void			CreateMap				( void );

	bool			DenyPickupHealth	() {return mLandScape->flrand(0.0f,1.0f) > mPickupHealth;}
	bool			DenyPickupArmor		() {return mLandScape->flrand(0.0f,1.0f) > mPickupArmor;}
	bool			DenyPickupAmmo		() {return mLandScape->flrand(0.0f,1.0f) > mPickupAmmo;}
	bool			DenyPickupWeapon	() {return mLandScape->flrand(0.0f,1.0f) > mPickupWeapon;}
	bool			DenyPickupEquipment	() {return mLandScape->flrand(0.0f,1.0f) > mPickupEquipment;}

private:
					
//	void			PurgeUnlinkedTriggers	( );
//	void			PurgeTrigger			( CEntity* trigger );

	void			MirrorPos				(vec3_t pos);
	CGPGroup*		ParseRandom				( CGPGroup* random );
	bool			ParseOrigin				( CGPGroup* originGroup, vec3_t origin, vec3_t lookat, int* flattenHeight );
	bool			ParseNodes				( CGPGroup* group );
	bool			ParsePaths				( CGPGroup *paths);					
	bool			ParseRivers				( CGPGroup *rivers);
	void			PlaceBridges			();
	void			PlaceWallInstance(CRMInstance*	instance, float xpos, float ypos, float zpos, int x, int y, float angle);


	bool			ParseDifficulty			( CGPGroup* difficulty, CGPGroup *parent );
	bool			ParseWeapons			( CGPGroup* weapons );
	bool			ParseAmmo				( CGPGroup* ammo );
	bool			ParseOutfit				( CGPGroup* outfit );
	bool			ParseObjectives			( CGPGroup* objectives );
	bool			ParseInstance			( CGPGroup* instance );
	bool			ParseInstances			( CGPGroup* instances );
	bool			ParseInstancesOnPath	( CGPGroup* group );
	bool			ParseWallRect			( CGPGroup* group, int side);

//	void			SpawnNPCTriggers		( CCMLandScape* landscape );
//	void			AttachNPCTriggers		( CCMLandScape* landscape );
};


#endif
