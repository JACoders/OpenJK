// leave this line at the top for all g_xxxx.cpp files...
#include "g_headers.h"

#include "G_Local.h"
#include "../rufl/hstring.h"

#define MAX_GTIMERS	16384

typedef struct gtimer_s
{
	hstring id;				// Use handle strings, so that things work after loading
	int time;
	struct gtimer_s *next;	// In either free list or current list
} gtimer_t;

gtimer_t g_timerPool[ MAX_GTIMERS ];
gtimer_t *g_timers[ MAX_GENTITIES ];
gtimer_t *g_timerFreeList;


static int TIMER_GetCount(int num)
{
	gtimer_t *p = g_timers[num];
	int count = 0;

	while (p)
	{
		count++;
		p = p->next;
	}
	
	return count;
}	


/*
-------------------------
TIMER_RemoveHelper

Scans an entities timer list to remove a given
timer from the list and put it on the free list

Doesn't do much error checking, only called below
-------------------------
*/
static void TIMER_RemoveHelper( int num, gtimer_t *timer )
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

void TIMER_Clear( int idx )
{
	// rudimentary safety checks, might be other things to check?
	if ( idx >= 0 && idx < MAX_GENTITIES )
	{
		gtimer_t *p = g_timers[idx];

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
		g_timerFreeList = g_timers[idx];
		g_timers[idx] = NULL;
		return;
	}
}


/*
-------------------------
TIMER_Save
-------------------------
*/

void TIMER_Save( void )
{
	int			j;
	gentity_t	*ent;

	for ( j = 0, ent = &g_entities[0]; j < MAX_GENTITIES; j++, ent++ )
	{
		unsigned char numTimers = TIMER_GetCount(j);

		if ( !ent->inuse && numTimers)
		{
//			Com_Printf( "WARNING: ent with timers not inuse\n" );
			assert(numTimers);
			TIMER_Clear( j );
			numTimers = 0;
		}

		//Write out the timer information
		gi.AppendToSaveGame('TIME', (void *)&numTimers, sizeof(numTimers));
	
		gtimer_t *p = g_timers[j];
		assert ((numTimers && p) || (!numTimers && !p));

		while(p)
		{
			const char	*timerID = p->id.c_str();
			const int	length = strlen(timerID) + 1;
			const int	time = p->time - level.time;	//convert this back to delta so we can use SET after loading

			assert( length < 1024 );//This will cause problems when loading the timer if longer

			//Write out the id string
			gi.AppendToSaveGame('TMID', (void *) timerID, length);

			//Write out the timer data
			gi.AppendToSaveGame('TDTA', (void *) &time, sizeof( time ) );
			p = p->next;
		}
	}
}

/*
-------------------------
TIMER_Load
-------------------------
*/

void TIMER_Load( void )
{
	int j;
	gentity_t	*ent;

	for ( j = 0, ent = &g_entities[0]; j < MAX_GENTITIES; j++, ent++ )
	{
		unsigned char numTimers;

		gi.ReadFromSaveGame( 'TIME', (void *)&numTimers, sizeof(numTimers) );

		//Read back all entries
		for ( int i = 0; i < numTimers; i++ )
		{
			int		time;
			char	tempBuffer[1024];	// Still ugly. Setting ourselves up for 007 AUF all over again. =)

			assert (sizeof(g_timers[0]->time) == sizeof(time) );//make sure we're reading the same size as we wrote

			//Read the id string and time
			gi.ReadFromSaveGame( 'TMID', (char *) tempBuffer, 0 );
			gi.ReadFromSaveGame( 'TDTA', (void *) &time, sizeof( time ) );

			//this is odd, we saved all the timers in the autosave, but not all the ents are spawned yet from an auto load, so skip it
			if (ent->inuse)
			{	//Restore it
				TIMER_Set(ent, tempBuffer, time);
			}
		}
	}
}


static gtimer_t *TIMER_GetNew(int num, const char *identifier)
{
	assert(num < ENTITYNUM_MAX_NORMAL);//don't want timers on NONE or the WORLD
	gtimer_t *p = g_timers[num];

	// Search for an existing timer with this name
	while (p)
	{
		if (p->id == identifier)
		{ // Found it
			return p;
		}

		p = p->next;
	}

	// No existing timer with this name was found, so grab one from the free list
	if (!g_timerFreeList)
	{//oh no, none free!
		assert(g_timerFreeList);
		return NULL;
	}

	p = g_timerFreeList;
	g_timerFreeList = g_timerFreeList->next;
	p->next = g_timers[num];
	g_timers[num] = p;
	return p;
}


gtimer_t *TIMER_GetExisting(int num, const char *identifier)
{
	gtimer_t *p = g_timers[num];

	while (p)
	{
		if (p->id == identifier)
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
	assert(ent->inuse);
	gtimer_t *timer = TIMER_GetNew(ent->s.number, identifier);

	if (timer)
	{
		timer->id	= identifier;
		timer->time = level.time + duration;
	}
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
	return (qboolean)TIMER_GetExisting(ent->s.number, identifier);
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
