/*
===========================================================================
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

#include "con_local.h"
#include <cassert>
#include <cstdio>


/*
========================================================================

CONSOLE LOG

========================================================================
*/
#define MAX_CONSOLE_LOG_SIZE (65535)

struct ConsoleLog
{
	// Cicular buffer of characters. Be careful, there is no null terminator.
	// You're expected to use the console log length to know where the end
	// of the string is.
	char text[MAX_CONSOLE_LOG_SIZE];

	// Where to start writing the next string
	int writeHead;

	// Length of buffer
	int length;
};

static ConsoleLog consoleLog;

void ConsoleLogAppend( const char *string )
{
	for ( int i = 0; string[i]; i++ )
	{
		consoleLog.text[consoleLog.writeHead] = string[i];
		consoleLog.writeHead = (consoleLog.writeHead + 1) % MAX_CONSOLE_LOG_SIZE;

		consoleLog.length++;
		if ( consoleLog.length > MAX_CONSOLE_LOG_SIZE )
		{
			consoleLog.length = MAX_CONSOLE_LOG_SIZE;
		}
	}
}

void ConsoleLogWriteOut( FILE *fp )
{
	assert( fp );

	if ( consoleLog.length == MAX_CONSOLE_LOG_SIZE &&
			consoleLog.writeHead != MAX_CONSOLE_LOG_SIZE )
	{
		fwrite( consoleLog.text + consoleLog.writeHead, MAX_CONSOLE_LOG_SIZE - consoleLog.writeHead, 1, fp );
	}

	fwrite( consoleLog.text, consoleLog.writeHead, 1, fp );
}
