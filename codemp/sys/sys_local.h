#pragma once

#include "../qcommon/qcommon.h"

typedef struct glconfig_s glconfig_t;

window_t	WIN_Init( graphicsApi_t api, glconfig_t *glConfig );
void		WIN_Present( window_t *window );
void		WIN_SetGamma( glconfig_t *glConfig, byte red[256], byte green[256], byte blue[256] );
void		WIN_Shutdown( void );
void *		WIN_GL_GetProcAddress( const char *proc );

void 		IN_Init( void *windowData );
void 		IN_Frame (void);
void 		IN_Shutdown( void );
void 		IN_Restart( void );

qboolean	Sys_GetPacket ( netadr_t *net_from, msg_t *net_message );
char		*Sys_ConsoleInput (void);
void 		Sys_QueEvent( int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr );
void 		Sys_ListFilteredFiles( const char *basedir, char *subdirs, char *filter, char **psList, int *numfiles );
void		Sys_Sleep( int msec );

bool Sys_UnpackDLL(const char *name);

void Sys_Exit( int ex );
