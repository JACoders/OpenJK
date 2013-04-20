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

/************************************************************************************************
 *
 * RM_Objective.cpp
 *
 * Implements the CRMObjective class.  This class is reponsible for parsing an objective 
 * from the mission file as well as linking the objective into the world.
 *
 ************************************************************************************************/

#include "../server/exe_headers.h"

#include "RM_Headers.h"

/************************************************************************************************
 * CRMObjective::CRMObjective
 *	Constructs a random mission objective and fills in the default properties
 *
 * inputs:
 *  none
 *
 * return:
 *	none
 *
 ************************************************************************************************/
CRMObjective::CRMObjective ( CGPGroup* group )
{	
	SetPriority(atoi(group->FindPairValue("priority", "0")));
	SetMessage( group->FindPairValue("message",va("Objective %i Completed", GetPriority()) ) );
	SetDescription(group->FindPairValue("description",va("Objective %i", GetPriority()) ) );
	SetInfo(group->FindPairValue("info",va("Info %i", GetPriority()) ) );
	SetTrigger(group->FindPairValue("trigger",""));
	SetName(group->GetName());
	
/*	const char * soundPath = group->FindPairValue("completed_sound", "" ); 
	if (soundPath)
		mCompleteSoundID = G_SoundIndex(soundPath); 
*/

	mCompleted  = false;
	mOrderIndex = -1;

	// If no priority was specified for this objective then its active by default.
	if ( GetPriority ( ) )
	{
		mActive	= false;
	}
	else
	{
		mActive = true;
	}
}

/************************************************************************************************
 * CRMObjective::FindRandomTrigger
 *	Searches the entitySystem form a random arioche trigger that matches the objective name
 *
 * inputs:
 *  none
 *
 * return:
 *	trigger: a random trigger or NULL if one couldnt be found
 *
 ************************************************************************************************/
/*CTriggerAriocheObjective* CRMObjective::FindRandomTrigger ( )
{	
	CEntity*	search;
	CEntity*	triggers[20];
	int			numTriggers;

	// Start off the first trigger
	numTriggers = 0;
	search      = entitySystem->GetEntityFromClassname ( NULL, "trigger_arioche_objective" );

	// Make a list of triggers
	while ( numTriggers < 20 && search )
	{
		CTriggerAriocheObjective* trigger = (CTriggerAriocheObjective*) search;

		// Move on to the next trigger
		search = entitySystem->GetEntityFromClassname ( search, "trigger_arioche_objective" );

		// See if this trigger is already in use
		if ( trigger->GetObjective ( ) )
		{
			continue;
		}

		// If the objective names dont match then ignore this trigger
		if ( stricmp ( trigger->GetObjectiveName ( ), GetTrigger() ) )
		{
			continue;
		}
	
		// Add the trigger to the list
		triggers[numTriggers++] = trigger;
	}

	// If no matching triggers then just return NULL
	if ( 0 == numTriggers )
	{
		return NULL;
	}

	// Return a random choice from the trigger list
	return (CTriggerAriocheObjective*)triggers[TheRandomMissionManager->GetLandScape()->irand(0,numTriggers-1)];
}
*/
/************************************************************************************************
 * CRMObjective::Link
 *	Links the objective into the world using the current state of the world to determine
 *  where it should link
 *
 * inputs:
 *  none
 *
 * return:
 *	true: objective successfully linked
 *  false: objective failed to link
 *
 ************************************************************************************************/
bool CRMObjective::Link ( )
{
/*	CTriggerAriocheObjective* trigger;

	// Look for a random trigger to associate this objective to.
	trigger = FindRandomTrigger ( );
	if ( NULL != trigger )
	{
		trigger->SetObjective ( this );
	}
*/	
	return true;
}

