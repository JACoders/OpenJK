#pragma once

#include <cstdio>

/* con_passive.cpp | con_win32.cpp | con_tty.cpp */
void CON_Shutdown( void );
void CON_Init( void );
char *CON_Input( void );
void CON_Print( const char *msg );

/* con_log.cpp */
void ConsoleLogAppend( const char *string );
void ConsoleLogWriteOut( FILE *fp );
