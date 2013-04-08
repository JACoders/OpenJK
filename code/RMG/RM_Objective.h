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
#if !defined(RM_OBJECTIVE_H_INC)
#define RM_OBJECTIVE_H_INC

#ifdef DEBUG_LINKING
	#pragma message("...including RM_Objective.h")
#endif

class CRMObjective
{
protected:

	bool		mCompleted;		// Is objective completed?
	bool		mActive;		// set to false if the objective requires another objective to be met first
	int			mPriority;		// sequence in which objectives need to be completed
	int			mOrderIndex;	// objective index in ui
	int			mCompleteSoundID; // sound for when objective is finished
	string		mMessage;		// message outputed when objective is completed
	string		mDescription;	// description of objective
	string		mInfo;			// more info for objective
	string		mName;			// name of objective
	string		mTrigger;		// trigger associated with objective

public:
	
	CRMObjective(CGPGroup *group);
	~CRMObjective(void) {}

	bool			Link			(void);

	bool			IsCompleted		(void) const { return mCompleted; }
	bool			IsActive		(void) const { return mActive; }

	void			Activate		(void)		 { mActive = true; }
	void			Complete		(bool comp)  { mCompleted = comp;}

	// Get methods
	int				GetPriority(void){return mPriority;}
	int				GetOrderIndex(void) { return mOrderIndex; }
	const char*		GetMessage(void) { return mMessage.c_str(); }
	const char*		GetDescription(void) { return mDescription.c_str(); }
	const char*		GetInfo(void) { return mInfo.c_str(); }
	const char*		GetName(void) { return mName.c_str(); }
	const char*		GetTrigger(void) { return mTrigger.c_str(); }
	int				CompleteSoundID() { return mCompleteSoundID; };

	// Set methods
	void			SetPriority(int priority){mPriority = priority;}
	void			SetOrderIndex(int order) { mOrderIndex = order; }
	void			SetMessage(const char* msg) { mMessage = msg; }
	void			SetDescription(const char* desc) { mDescription = desc; }
	void			SetInfo(const char* info) { mInfo = info; }
	void			SetName(const char* name) { mName = name; }
	void			SetTrigger(const char* name) { mTrigger = name; }

private:

//	CTriggerAriocheObjective*		FindRandomTrigger		( );
};

typedef list<CRMObjective *>::iterator	rmObjectiveIter_t;
typedef list<CRMObjective *>			rmObjectiveList_t;


#endif