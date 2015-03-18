/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#ifndef __DMSTATES_H__
#define __DMSTATES_H__

//dynamic music
typedef enum //# dynamicMusic_e
{
	DM_AUTO,	//# let the game determine the dynamic music as normal
	DM_SILENCE,	//# stop the music
	DM_EXPLORE,	//# force the exploration music to play
	DM_ACTION,	//# force the action music to play
	DM_BOSS,	//# force the boss battle music to play (if there is any)
	DM_DEATH	//# force the "player dead" music to play
} dynamicMusic_t;

#endif//#ifndef __DMSTATES_H__
