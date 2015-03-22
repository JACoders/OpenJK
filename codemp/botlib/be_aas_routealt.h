/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
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

/*****************************************************************************
 * name:		be_aas_routealt.h
 *
 * desc:		AAS
 *
 * $Archive: /source/code/botlib/be_aas_routealt.h $
 * $Author: Mrelusive $
 * $Revision: 2 $
 * $Modtime: 10/05/99 3:32p $
 * $Date: 10/05/99 3:42p $
 *
 *****************************************************************************/

#pragma once

#ifdef AASINTERN
void AAS_InitAlternativeRouting(void);
void AAS_ShutdownAlternativeRouting(void);
#endif //AASINTERN


int AAS_AlternativeRouteGoals(vec3_t start, int startareanum, vec3_t goal, int goalareanum, int travelflags,
										aas_altroutegoal_t *altroutegoals, int maxaltroutegoals,
										int type);
