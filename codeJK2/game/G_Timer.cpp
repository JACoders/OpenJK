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

// leave this line at the top for all g_xxxx.cpp files...
#include "g_headers.h"


#include "g_local.h"

typedef map		< string, int >	timer_m;

timer_m	g_timers[ MAX_GENTITIES ];

/*
-------------------------
TIMER_Clear
-------------------------
*/

void TIMER_Clear( void )
{
	for ( int i = 0; i < MAX_GENTITIES; i++ )
	{
		g_timers[i].clear();
	}
}

/*
-------------------------
TIMER_Clear
-------------------------
*/

void TIMER_Clear( gentity_t *ent )
{
	// rudimentary safety checks, might be other things to check?
	if ( ent && ent->s.number > 0 && ent->s.number < MAX_GENTITIES )
	{
		g_timers[ent->s.number].clear();
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
		int numTimers = g_timers[ent->s.number].size();
		int	i;

		//Write out the timer information
		gi.AppendToSaveGame(INT_ID('T','I','M','E'), (void *)&numTimers, sizeof(numTimers));
		
		timer_m::iterator	ti;

		for ( i = 0, ti = g_timers[ j ].begin(); i < numTimers; i++, ti++ )
		{
			const char *id = ((*ti).first).c_str();
			int			length = strlen( id );

			//Write out the string size and data
			gi.AppendToSaveGame(INT_ID('T','S','L','N'), (void *) &length, sizeof(length) );
			gi.AppendToSaveGame(INT_ID('T','S','N','M'), (void *) id, length );

			//Write out the timer data
			gi.AppendToSaveGame(INT_ID('T','D','T','A'), (void *) &(*ti).second, sizeof( (*ti).second ) );
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
		int numTimers;

		gi.ReadFromSaveGame( INT_ID('T','I','M','E'), (void *)&numTimers, sizeof(numTimers), NULL );

		//Make sure there's something to read
		if ( numTimers == 0 )
			continue;
		
		//Read back all entries
		for ( int i = 0; i < numTimers; i++ )
		{
			int		length, time;
			char	tempBuffer[1024];	//FIXME: Blech!

			gi.ReadFromSaveGame( INT_ID('T','S','L','N'), (void *) &length, sizeof( length ), NULL );

			//Validity check, though this will never happen (unless of course you pass in gibberish)
			if ( length >= 1024 )
			{
				assert(0);
				continue;
			}

			//Read the id and time
			gi.ReadFromSaveGame( INT_ID('T','S','N','M'), (char *) tempBuffer, length, NULL );
			gi.ReadFromSaveGame( INT_ID('T','D','T','A'), (void *) &time, sizeof( time ), NULL );

			//Restore it
			g_timers[ j ][(const char *) tempBuffer ] = time;
		}
	}
}

/*
-------------------------
TIMER_Set
-------------------------
*/

void TIMER_Set( gentity_t *ent, const char *identifier, int duration )
{
	g_timers[ent->s.number][identifier] = level.time + duration;
}

/*
-------------------------
TIMER_Get
-------------------------
*/

int	TIMER_Get( gentity_t *ent, const char *identifier )
{
	timer_m::iterator	ti;

	ti = g_timers[ent->s.number].find( identifier );

	if ( ti == g_timers[ent->s.number].end() )
	{
		//assert(0);
		return -1;
	}

	return (*ti).second;
}

/*
-------------------------
TIMER_Done
-------------------------
*/

qboolean TIMER_Done( gentity_t *ent, const char *identifier )
{
	timer_m::iterator	ti;

	ti = g_timers[ent->s.number].find( identifier );

	if ( ti == g_timers[ent->s.number].end() )
		return true;

	return ( (*ti).second < level.time );
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
	timer_m::iterator	ti;

	ti = g_timers[ent->s.number].find( identifier );

	if ( ti == g_timers[ent->s.number].end() )
	{
		return qfalse;
	}

	qboolean res =((*ti).second < level.time );

	if ( res && remove )
	{
		// Timer is done and it was requested that it should be removed.
		g_timers[ent->s.number].erase( ti );
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
	timer_m::iterator	ti;

	ti = g_timers[ent->s.number].find( identifier );

	if ( ti == g_timers[ent->s.number].end( ))
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
	timer_m::iterator	ti;

	ti = g_timers[ent->s.number].find( identifier );

	if ( ti != g_timers[ent->s.number].end() )
	{
		g_timers[ent->s.number].erase( ti );
	}
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
