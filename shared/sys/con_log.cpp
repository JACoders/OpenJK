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

	if ( consoleLog.writeHead == MAX_CONSOLE_LOG_SIZE )
	{
		fwrite( consoleLog.text + consoleLog.writeHead, MAX_CONSOLE_LOG_SIZE - consoleLog.writeHead, 1, fp );
	}

	fwrite( consoleLog.text, consoleLog.writeHead, 1, fp );
}
