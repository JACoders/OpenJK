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

//rww - rewrite from C++ SP version.
//This is here only to make porting from SP easier, it's really sort of nasty (being static
//now). Basically it's slower and takes more memory.

#include "g_local.h"
//typedef map		< string, int >	timer_m;

#define MAX_GTIMERS	16384

typedef struct gtimer_s
{
	const char *name;
	int time;
	struct gtimer_s *next;	// In either free list or current list
} gtimer_t;

gtimer_t g_timerPool[ MAX_GTIMERS ];
gtimer_t *g_timers[ MAX_GENTITIES ];
gtimer_t *g_timerFreeList;

/*
-------------------------
TIMER_Clear
-------------------------
*/

void TIMER_Clear( void )
{
	int i;
	for (i = 0; i < MAX_GENTITIES; i++)
	{
		g_timers[i] = NULL;
	}

	for (i = 0; i < MAX_GTIMERS - 1; i++)
	{
		g_timerPool[i].next = &g_timerPool[i+1];
	}
	g_timerPool[MAX_GTIMERS-1].next = NULL;
	g_timerFreeList = &g_timerPool[0];
}

/*
-------------------------
TIMER_Clear
-------------------------
*/

void TIMER_Clear2( gentity_t *ent )
{
	// rudimentary safety checks, might be other things to check?
	if ( ent && ent->s.number >= 0 && ent->s.number < MAX_GENTITIES )
	{
		gtimer_t *p = g_timers[ent->s.number];

		// No timers at all -> do nothing
		if (!p)
		{
			return;
		}

		// Find the end of this ents timer list
		while (p->next)
		{
			p = p->next;
		}

		// Splice the lists
		p->next = g_timerFreeList;
		g_timerFreeList = g_timers[ent->s.number];
		g_timers[ent->s.number] = NULL;
		return;
	}
}


//New C "lookup" func.
//Returns existing timer in array if
gtimer_t *TIMER_GetNew(int num, const char *identifier)
{
	gtimer_t *p = g_timers[num];

	// Search for an existing timer with this name
	while (p)
	{
		if (!Q_stricmp(p->name, identifier))
		{ // Found it
			return p;
		}

		p = p->next;
	}

	// No existing timer with this name was found, so grab one from the free list
	if (!g_timerFreeList)
		return NULL;

	p = g_timerFreeList;
	g_timerFreeList = g_timerFreeList->next;
	p->next = g_timers[num];
	g_timers[num] = p;
	return p;
}

//don't return the first free if it doesn't already exist, return null.
gtimer_t *TIMER_GetExisting(int num, const char *identifier)
{
	gtimer_t *p = g_timers[num];

	while (p)
	{
		if (!Q_stricmp(p->name, identifier))
		{ // Found it
			return p;
		}

		p = p->next;
	}

	return NULL;
}

/*
-------------------------
TIMER_Set
-------------------------
*/

void TIMER_Set( gentity_t *ent, const char *identifier, int duration )
{
	gtimer_t *timer = TIMER_GetNew(ent->s.number, identifier);

	if (!timer)
	{
		return;
	}
	timer->name = identifier;
	timer->time = level.time + duration;
}

/*
-------------------------
TIMER_Get
-------------------------
*/

int	TIMER_Get( gentity_t *ent, const char *identifier )
{
	gtimer_t *timer = TIMER_GetExisting(ent->s.number, identifier);

	if (!timer)
	{
		return -1;
	}

	return timer->time;
}

/*
-------------------------
TIMER_Done
-------------------------
*/

qboolean TIMER_Done( gentity_t *ent, const char *identifier )
{
	gtimer_t *timer = TIMER_GetExisting(ent->s.number, identifier);

	if (!timer)
	{
		return qtrue;
	}

	return (timer->time < level.time);
}

/*
-------------------------
TIMER_RemoveHelper

Scans an entities timer list to remove a given
timer from the list and put it on the free list

Doesn't do much error checking, only called below
-------------------------
*/
void TIMER_RemoveHelper( int num, gtimer_t *timer )
{
	gtimer_t *p = g_timers[num];

	// Special case: first timer in list
	if (p == timer)
	{
		g_timers[num] = g_timers[num]->next;
		p->next = g_timerFreeList;
		g_timerFreeList = p;
		return;
	}

	// Find the predecessor
	while (p->next != timer)
	{
		p = p->next;
	}

	// Rewire
	p->next = p->next->next;
	timer->next = g_timerFreeList;
	g_timerFreeList = timer;
	return;
}

/*
-------------------------
TIMER_Done2

Returns false if timer has been
started but is not done...or if
timer was never started
-------------------------
*/

qboolean TIMER_Done2( gentity_t *ent, const char *identifier, qboolean remove )
{
	gtimer_t *timer = TIMER_GetExisting(ent->s.number, identifier);
	qboolean res;

	if (!timer)
	{
		return qfalse;
	}

	res = (timer->time < level.time);

	if (res && remove)
	{
		// Put it back on the free list
		TIMER_RemoveHelper(ent->s.number, timer);
	}

	return res;
}

/*
-------------------------
TIMER_Exists
-------------------------
*/
qboolean TIMER_Exists( gentity_t *ent, const char *identifier )
{
	gtimer_t *timer = TIMER_GetExisting(ent->s.number, identifier);

	if (!timer)
	{
		return qfalse;
	}

	return qtrue;
}

/*
-------------------------
TIMER_Remove
Utility to get rid of any timer
-------------------------
*/
void TIMER_Remove( gentity_t *ent, const char *identifier )
{
	gtimer_t *timer = TIMER_GetExisting(ent->s.number, identifier);

	if (!timer)
	{
		return;
	}

	// Put it back on the free list
	TIMER_RemoveHelper(ent->s.number, timer);
}

/*
-------------------------
TIMER_Start
-------------------------
*/

qboolean TIMER_Start( gentity_t *self, const char *identifier, int duration )
{
	if ( TIMER_Done( self, identifier ) )
	{
		TIMER_Set( self, identifier, duration );
		return qtrue;
	}
	return qfalse;
}
