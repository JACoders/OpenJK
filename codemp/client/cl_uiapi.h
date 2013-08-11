#pragma once

void		UIVM_Init				( qboolean inGameLoad );
void		UIVM_Shutdown			( void );
void		UIVM_KeyEvent			( int key, qboolean down );
void		UIVM_MouseEvent			( int dx, int dy );
void		UIVM_Refresh			( int realtime );
qboolean	UIVM_IsFullscreen		( void );
void		UIVM_SetActiveMenu		( uiMenuCommand_t menu );
qboolean	UIVM_ConsoleCommand		( int realTime );
void		UIVM_DrawConnectScreen	( qboolean overlay );

void CL_BindUI( void );
void CL_UnbindUI( void );
