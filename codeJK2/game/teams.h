/*
This file is part of Jedi Knight 2.

    Jedi Knight 2 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Jedi Knight 2 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Knight 2.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

#ifndef TEAMS_H
#define TEAMS_H

typedef enum //# team_e
{
	TEAM_FREE,			// caution, some code checks a team_t via "if (!team_t_varname)" so I guess this should stay as entry 0, great or what? -slc
//	TEAM_STARFLEET,
//	TEAM_BORG,
//	TEAM_PARASITE,
//	TEAM_SCAVENGERS,
//	TEAM_KLINGON,
//	TEAM_MALON,
//	TEAM_HIROGEN,
//	TEAM_IMPERIAL,
//	TEAM_STASIS,
//	TEAM_8472,
//	TEAM_BOTS,
//	TEAM_FORGE,
//	TEAM_DISGUISE,
	TEAM_PLAYER,
	TEAM_ENEMY,
	TEAM_NEUTRAL,	// most droids are team_neutral, there are some exceptions like Probe,Seeker,Interrogator

	//# #eol
	TEAM_NUM_TEAMS
} team_t;

// This list is made up from the model directories, this MUST be in the same order as the ClassNames array in NPC_stats.cpp
typedef enum 
{
	CLASS_NONE,				// hopefully this will never be used by an npc, just covering all bases
	CLASS_ATST,				// technically droid...
	CLASS_BARTENDER,
	CLASS_BESPIN_COP,		
	CLASS_CLAW,
	CLASS_COMMANDO,
	CLASS_DESANN,			
	CLASS_FISH,
	CLASS_FLIER2,
	CLASS_GALAK,
	CLASS_GLIDER,
	CLASS_GONK,				// droid
	CLASS_GRAN,
	CLASS_HOWLER,
	CLASS_IMPERIAL,
	CLASS_IMPWORKER,
	CLASS_INTERROGATOR,		// droid 
	CLASS_JAN,				
	CLASS_JEDI,				
	CLASS_KYLE,				
	CLASS_LANDO,			
	CLASS_LIZARD,
	CLASS_LUKE,				
	CLASS_MARK1,			// droid
	CLASS_MARK2,			// droid
	CLASS_GALAKMECH,		// droid
	CLASS_MINEMONSTER,
	CLASS_MONMOTHA,			
	CLASS_MORGANKATARN,
	CLASS_MOUSE,			// droid
	CLASS_MURJJ,
	CLASS_PRISONER,
	CLASS_PROBE,			// droid
	CLASS_PROTOCOL,			// droid
	CLASS_R2D2,				// droid
	CLASS_R5D2,				// droid
	CLASS_REBEL,
	CLASS_REBORN,
	CLASS_REELO,
	CLASS_REMOTE,
	CLASS_RODIAN,
	CLASS_SEEKER,			// droid
	CLASS_SENTRY,
	CLASS_SHADOWTROOPER,
	CLASS_STORMTROOPER,
	CLASS_SWAMP,
	CLASS_SWAMPTROOPER,
	CLASS_TAVION,
	CLASS_TRANDOSHAN,
	CLASS_UGNAUGHT,
	CLASS_WEEQUAY,
	
	CLASS_NUM_CLASSES
} class_t;

#endif	// #ifndef TEAMS_H
