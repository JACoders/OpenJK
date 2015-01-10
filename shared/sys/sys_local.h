#pragma once

#include "qcommon/qcommon.h"

void 		IN_Init( void *windowData );
void 		IN_Frame( void );
void 		IN_Shutdown( void );
void 		IN_Restart( void );

void		Sys_PlatformInit( void );
void		Sys_PlatformQuit( void );
qboolean	Sys_GetPacket( netadr_t *net_from, msg_t *net_message );
char		*Sys_ConsoleInput( void );
void 		Sys_QueEvent( int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr );

bool		Sys_UnpackDLL( const char *name );
