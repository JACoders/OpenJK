//Anything above this #include will be ignored by the compiler
#include "qcommon/exe_headers.h"

/************************************************************************************************
 *
 * RM_Manager.cpp
 *
 * Implements the CRMManager class.  The CRMManager class manages the arioche system.
 *
 ************************************************************************************************/

#include "RM_Headers.h"
#include "server/server.h"
#include "qcommon/qcommon.h"

CRMObjective *CRMManager::mCurObjective=0;

/************************************************************************************************
 * TheRandomMissionManager
 *	Pointer to only active CRMManager class
 *
 ************************************************************************************************/
CRMManager *TheRandomMissionManager;

/************************************************************************************************
 * CRMManager::CRMManager
 *	constructor
 *
 * inputs:
 *
 * return:
 *
 ************************************************************************************************/
CRMManager::CRMManager(void)
{
	mLandScape = NULL;
	mTerrain   = NULL;
	mMission = NULL;
	mCurPriority = 1;
	mUseTimeLimit = false;
	mAutomapSymbolCount = 0;
}

/************************************************************************************************
 * CRMManager::~CRMManager
 *	destructor
 *
 * inputs:
 *
 * return:
 *
 ************************************************************************************************/
CRMManager::~CRMManager(void)
{
#ifndef FINAL_BUILD
	Com_Printf ("... Shutting down TheRandomMissionManager\n");
#endif
#ifndef DEDICATED
	CM_TM_Free();
#endif
	if (mMission)
	{
		delete mMission;
		mMission = NULL;
	}
}

/************************************************************************************************
 * CRMManager::SetLandscape
 *	Sets the landscape and terrain object used to load a mission
 *
 * inputs:
 *	landscape - landscape object
 *
 * return:
 *	none
 *
 ************************************************************************************************/
void CRMManager::SetLandScape(CCMLandScape *landscape)
{
	mLandScape = landscape;
	mTerrain = landscape->GetRandomTerrain();
}

/************************************************************************************************
 * CRMManager::LoadMission
 *	Loads the mission using the mission name stored in the ar_mission cvar
 *
 * inputs:
 *	none
 *
 * return:
 *	none
 *
 ************************************************************************************************/
bool CRMManager::LoadMission ( qboolean IsServer )
{
	char	instances[MAX_QPATH];
	char	mission[MAX_QPATH];
	char	course[MAX_QPATH];
	char	map[MAX_QPATH];
	char	temp[MAX_QPATH];

#ifndef FINAL_BUILD
	Com_Printf ("--------- Random Mission Manager ---------\n\n");
	Com_Printf ("RMG version : 1.01\n\n");
#endif

	if (!mTerrain)
	{
		return false;
	}

	// Grab the arioche variables
	Cvar_VariableStringBuffer("rmg_usetimelimit", temp, MAX_QPATH);
	if (Q_stricmp(temp, "yes") == 0)
	{
		mUseTimeLimit = true;
	}
	Cvar_VariableStringBuffer("rmg_instances", instances, MAX_QPATH);
	Cvar_VariableStringBuffer("RMG_mission", temp, MAX_QPATH);
	Cvar_VariableStringBuffer("rmg_map", map, MAX_QPATH);
	Com_sprintf(mission, sizeof(mission), "%s_%s", temp, map);
	Cvar_VariableStringBuffer("rmg_course", course, MAX_QPATH);

	// dump existing mission, if any
	if (mMission)
	{
		delete mMission;
		mMission = NULL;
	}

	// Create a new mission file	
	mMission = new CRMMission ( mTerrain );

	if ( IsServer )
	{
		// Load the mission using the arioche variables
		if ( !mMission->Load ( mission, instances, course ) )
		{
			return false;
		}
	
		// set the names of the teams 
		CGenericParser2		parser;
		CGPGroup*			root;

		Cvar_VariableStringBuffer("RMG_terrain", temp, MAX_QPATH);

		// Create the parser for the mission file
		if(Com_ParseTextFile(va("ext_data/rmg/%s.teams", temp), parser))
		{
			root = parser.GetBaseParseGroup()->GetSubGroups();
			if (0 == Q_stricmp(root->GetName(), "teams"))
			{
				/*
				SV_SetConfigstring( CS_GAMETYPE_REDTEAM, root->FindPairValue ( "red", "marine" ));
				SV_SetConfigstring( CS_GAMETYPE_BLUETEAM, root->FindPairValue ( "blue", "thug" ));
				*/
				//rwwFIXMEFIXME: Do we care about this?
			}
			parser.Clean();
		}
	}

	// Must have a valid landscape before we can spawn the mission
	assert ( mLandScape );

#ifndef FINAL_BUILD
	Com_Printf ("------------------------------------------\n");
#endif

	return true;
}

/************************************************************************************************
 * CRMManager::IsMissionComplete
 *	Determines whether or not all the arioche objectives have been met
 *
 * inputs:
 *  none
 *
 * return:
 *	true: all objectives have been completed
 *  false: one or more of the objectives has not been met
 *
 ************************************************************************************************/
bool CRMManager::IsMissionComplete(void)
{
	if ( NULL == mMission->GetCurrentObjective ( ) )
	{
		return true;
	}

	return false;
}

/************************************************************************************************
 * CRMManager::HasTimeExpired
 *	Determines whether or not the time limit (if one) has expired
 *
 * inputs:
 *  none
 *
 * return:
 *	true: time limit has expired
 *  false: time limit has not expired
 *
 ************************************************************************************************/
bool CRMManager::HasTimeExpired(void)
{
/*	if (mMission->GetTimeLimit() == 0 || !mUseTimeLimit)
	{	// no time limit set
		return false;
	}

	if (mMission->GetTimeLimit() * 1000 * 60 > level.time - level.startTime)
	{	// we are still under our time limit
		return false;
	}

	// over our time limit!
	return true;*/

	return false;
}

/************************************************************************************************
 * CRMManager::UpdateStatisticCvars
 *	Updates the statistic cvars with data from the game
 *
 * inputs:
 *  none
 *
 * return:
 *	none
 *
 ************************************************************************************************/
void CRMManager::UpdateStatisticCvars ( void )
{
/*	// No player set then nothing more to do
	if ( mPlayer )
	{
		float accuracy;

		// Calculate the accuracy
		accuracy  = (float)mPlayer->client->ps.persistant[PERS_SHOTS_HIT];
		accuracy /= (float)mPlayer->client->ps.persistant[PERS_SHOTS];
		accuracy *= 100.0f;
		
		// set the accuracy cvar
		gi.Cvar_Set ( "ar_pl_accuracy", va("%d%%",(int)accuracy) );

		// Set the # of kills cvar
		gi.Cvar_Set ( "ar_kills", va("%d", mPlayer->client->ps.persistant[PERS_SCORE] ) );

		int hours;
		int mins;
		int seconds;
		int tens;
		int millisec = (level.time - level.startTime);

		seconds = millisec / 1000;
		hours = seconds / (60 * 60);
		seconds -= (hours * 60 * 60);
		mins = seconds / 60;		
		seconds -= mins * 60;
		tens = seconds / 10;
		seconds -= tens * 10;

		gi.Cvar_Set ( "ar_duration", va("%dhr %dmin %dsec", hours, mins, seconds ) );

		WpnID wpnID = TheWpnSysMgr().GetFavoriteWeapon ( );
		gi.Cvar_Set ( "ar_fav_wp", CWeaponSystem::GetWpnName ( wpnID ) );

		// show difficulty
		char difficulty[MAX_QPATH];
		gi.Cvar_VariableStringBuffer("g_skill", difficulty, MAX_QPATH);
		strupr(difficulty);
		gi.Cvar_Set ( "ar_diff", va("&GENERIC_%s&",difficulty) );

		// compute rank
		float compositeRank = 1;
		int rankMax = 3;  // max rank less 1
		float timeRank = mUseTimeLimit ? (1.0f - (mins / (float)mMission->GetTimeLimit())) : 0;
		float killRank = mPlayer->client->ps.persistant[PERS_SCORE] / (float)GetCharacterManager().GetAllSize();
		killRank = (killRank > 0) ? killRank : 1.0f; 
		float accuRank = (accuracy > 0) ? accuracy*0.01f : 1.0f;	
		float weapRank = 1.0f - CWeaponSystem::GetRank(wpnID);

		compositeRank = ((timeRank + killRank + accuRank + weapRank) / 3.0f) * rankMax + 1;
		
		if (compositeRank > 4)
			compositeRank = 4;

		gi.Cvar_Set ( "ar_rank", va("&RMG_RANK%d&",((int)compositeRank)) );
	}*/
}

/************************************************************************************************
 * CRMManager::CompleteMission
 *	Does end-of-mission stuff (pause game, end screen, return to menu)
 *    <Description>                                                                             *
 * Input                                                                                        *
 *    <Variable>: <Description>                                                                 *
 * Output / Return                                                                              *
 *    <Variable>: <Description>                                                                 *
 ************************************************************************************************/
void CRMManager::CompleteMission(void)
{
	UpdateStatisticCvars ( );

	mMission->CompleteMission();
}

/************************************************************************************************
 * CRMManager::FailedMission
 *	Does end-of-mission stuff (pause game, end screen, return to menu)
 *    <Description>                                                                             *
 * Input                                                                                        *
 *    TimeExpired: indicates if the reason failed was because of time
 * Output / Return                                                                              *
 *    <Variable>: <Description>                                                                 *
 ************************************************************************************************/
void CRMManager::FailedMission(bool TimeExpired)
{
	UpdateStatisticCvars ( );

	mMission->FailedMission(TimeExpired);
}

/************************************************************************************************
 * CRMManager::CompleteObjective
 *	Marks the given objective as completed
 *
 * inputs:
 *  obj:  objective to set as completed
 *
 * return:
 *	none
 *
 ************************************************************************************************/
void CRMManager::CompleteObjective ( CRMObjective *obj )
{
	assert ( obj );

	mMission->CompleteObjective ( obj );
}

/************************************************************************************************
 * CRMManager::Preview
 *	previews the random mission genration information
 *
 * inputs:
 *  from:  origin being previed from
 *
 * return:
 *	none
 *
 ************************************************************************************************/
void CRMManager::Preview ( const vec3_t from )
{
	// Dont bother if we havent reached our timer yet
/*	if ( level.time < mPreviewTimer )
	{
		return;
	}

	// Let the mission do all the previewing
	mMission->Preview ( from );

	// Another second
	mPreviewTimer = level.time + 1000;*/
}

/************************************************************************************************
 * CRMManager::Preview
 *	previews the random mission genration information
 *
 * inputs:
 *  from:  origin being previed from
 *
 * return:
 *	none
 *
 ************************************************************************************************/
bool CRMManager::SpawnMission ( qboolean IsServer )
{
	// Spawn the mission
	mMission->Spawn ( mTerrain, IsServer );

	return true;
}


void CRMManager::AddAutomapSymbol ( int type, vec3_t origin, int side )
{
	if ( !type )
	{
		return;
	}

	mAutomapSymbols[mAutomapSymbolCount].mType = type;
	mAutomapSymbols[mAutomapSymbolCount].mSide = side;
	VectorCopy ( origin, mAutomapSymbols[mAutomapSymbolCount].mOrigin );
	mAutomapSymbolCount++;
}

int CRMManager::GetAutomapSymbolCount ( void )
{
	return mAutomapSymbolCount;
}

rmAutomapSymbol_t* CRMManager::GetAutomapSymbol ( int index )
{
	return &mAutomapSymbols[index];
}

/*
void CRMManager::WriteAutomapSymbols ( msg_t* msg )
{
	rmAutomapSymbolIter_t it;

	MSG_WriteShort ( msg, (unsigned long)mAutomapSymbols.size() );

	for(it = mAutomapSymbols.begin(); it != mAutomapSymbols.end(); it++)
	{
		CRMAutomapSymbol* symbol = (CRMAutomapSymbol*) *it;

		MSG_WriteByte ( msg, (unsigned char) symbol->mType );
		MSG_WriteByte ( msg, (unsigned char) symbol->mSide );
		MSG_WriteLong ( msg, (long) symbol->mOrigin[0] );
		MSG_WriteLong ( msg, (long) symbol->mOrigin[1] );
	}	
}
*/

void CRMManager::ProcessAutomapSymbols ( int count, rmAutomapSymbol_t* symbols )
{
#ifndef DEDICATED
	int i;

	for ( i = 0; i < count; i ++ )
	{
		// draw proper symbol on map for instance
		switch (symbols[i].mType)
		{
			case AUTOMAP_BLD:
				CM_TM_AddBuilding(symbols[i].mOrigin[0], symbols[i].mOrigin[1], symbols[i].mSide );
				break;
			case AUTOMAP_OBJ:
				CM_TM_AddObjective(symbols[i].mOrigin[0], symbols[i].mOrigin[1], symbols[i].mSide );
				break;
			case AUTOMAP_START:
				CM_TM_AddStart(symbols[i].mOrigin[0], symbols[i].mOrigin[1], symbols[i].mSide );
				break;
			case AUTOMAP_END:
				CM_TM_AddEnd(symbols[i].mOrigin[0], symbols[i].mOrigin[1], symbols[i].mSide );
				break;
			case AUTOMAP_ENEMY:
				break;
			case AUTOMAP_FRIEND:
				break;
			case AUTOMAP_WALL:
				CM_TM_AddWallRect(symbols[i].mOrigin[0], symbols[i].mOrigin[1], symbols[i].mSide );
				break;
		}
	}
#endif
}
