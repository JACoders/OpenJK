#pragma once

void 		IN_Init( void *windowData );
void 		IN_Frame (void);
void 		IN_Shutdown( void );
void 		IN_Restart( void );

qboolean	Sys_GetPacket ( netadr_t *net_from, msg_t *net_message );
char		*Sys_ConsoleInput (void);
void 		Sys_QueEvent( int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr );
void 		Sys_ListFilteredFiles( const char *basedir, char *subdirs, char *filter, char **psList, int *numfiles );

void Sys_Exit( int ex );
