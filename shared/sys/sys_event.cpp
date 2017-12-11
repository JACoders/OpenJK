/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2005 - 2015, ioquake3 contributors
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

#include "qcommon/qcommon.h"
#include "sys_local.h"
#include "sys_public.h"

/*
========================================================================

EVENT LOOP

========================================================================
*/

#define	MAX_QUED_EVENTS		256
#define	MASK_QUED_EVENTS	( MAX_QUED_EVENTS - 1 )

static sysEvent_t	eventQue[MAX_QUED_EVENTS] = {};
static sysEvent_t	*lastEvent = nullptr;
static uint32_t		eventHead = 0, eventTail = 0;

static const char *Sys_EventName( sysEventType_t evType ) {

	static const char *evNames[SE_MAX] = {
		"SE_NONE",
		"SE_KEY",
		"SE_CHAR",
		"SE_MOUSE",
		"SE_JOYSTICK_AXIS",
		"SE_CONSOLE"
	};

	if ( evType >= SE_MAX ) {
		return "SE_UNKNOWN";
	} else {
		return evNames[evType];
	}
}

sysEvent_t Sys_GetEvent( void ) {
	sysEvent_t	ev;
	char		*s;

	// return if we have data
	if ( eventHead > eventTail ) {
		eventTail++;
		return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
	}

	// check for console commands
	s = Sys_ConsoleInput();
	if ( s ) {
		char	*b;
		int		len;

		len = strlen( s ) + 1;
		b = (char *)Z_Malloc( len,TAG_EVENT,qfalse );
		strcpy( b, s );
		Sys_QueEvent( 0, SE_CONSOLE, 0, 0, len, b );
	}

	// return if we have data
	if ( eventHead > eventTail ) {
		eventTail++;
		return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
	}

	// create an empty event to return

	memset( &ev, 0, sizeof( ev ) );
	ev.evTime = Sys_Milliseconds();

	return ev;
}

/*
================
Sys_QueEvent

A time of 0 will get the current time
Ptr should either be null, or point to a block of data that can
be freed by the game later.
================
*/
void Sys_QueEvent( int evTime, sysEventType_t evType, int value, int value2, int ptrLength, void *ptr ) {
	sysEvent_t	*ev;

	if ( evTime == 0 ) {
		evTime = Sys_Milliseconds();
	}

	// try to combine all sequential mouse moves in one event
	if ( evType == SE_MOUSE && lastEvent && lastEvent->evType == SE_MOUSE ) {
		// try to reuse already processed item
		if ( eventTail == eventHead ) {
			lastEvent->evValue = value;
			lastEvent->evValue2 = value2;
			eventTail--;
		} else {
			lastEvent->evValue += value;
			lastEvent->evValue2 += value2;
		}
		lastEvent->evTime = evTime;
		return;
	}

	ev = &eventQue[ eventHead & MASK_QUED_EVENTS ];

	if ( eventHead - eventTail >= MAX_QUED_EVENTS ) {
		Com_Printf( "Sys_QueEvent(%s,time=%i): overflow\n", Sys_EventName(evType), evTime );
		// we are discarding an event, but don't leak memory
		if ( ev->evPtr ) {
			Z_Free( ev->evPtr );
		}
		eventTail++;
	}

	eventHead++;

	ev->evTime = evTime;
	ev->evType = evType;
	ev->evValue = value;
	ev->evValue2 = value2;
	ev->evPtrLength = ptrLength;
	ev->evPtr = ptr;

	lastEvent = ev;
}
