#ifndef __UI_LOCAL_H__
#define __UI_LOCAL_H__

#include <string.h>
#include <limits.h>

#include "../game/q_shared.h"
#include "../renderer/tr_types.h"
#include "../qcommon/qcommon.h"
#include "ui_public.h"
#include "ui_shared.h"


#define	MAX_DEMOS				256
#define MAX_DEFERRED_SCRIPT		1024

//
// ui_qmenu.c
//
#define	MAX_EDIT_LINE			256

typedef struct {
	int		cursor;
	int		scroll;
	int		widthInChars;
	char	buffer[MAX_EDIT_LINE];
	int		maxchars;
	int		style;
	int		textEnum;		// Label
	int		textcolor;		// Normal color
	int		textcolor2;		// Highlight color
} uifield_t;

extern void		Menu_Cache( void );

//
// ui_field.c
//
extern void	Field_Clear( uifield_t *edit );
extern void	Field_CharEvent( uifield_t *edit, int ch );
extern void Field_Draw( uifield_t *edit, int x, int y, int width, int size,int color,int color2, qboolean showCursor );


//
// ui_menu.c
//
extern void UI_MainMenu(void);
extern void UI_InGameMenu(const char*holoFlag);
extern void AssetCache(void);
extern void UI_DataPadMenu(void);

//
// ui_connect.c
//
extern void UI_DrawConnect( const char *servername, const char * updateInfoString );
extern void UI_DrawConnectText( const char *servername, const char * updateInfoString );
extern void UI_UpdateConnectionString( char *string );
extern void UI_UpdateConnectionMessageString( char *string );


//
// ui_atoms.c
//

#define UI_FADEOUT	0
#define UI_FADEIN	1

typedef struct {
	int					frametime;
	int					realtime;
	int					cursorx;
	int					cursory;
	
	glconfig_t			glconfig;
	qboolean			debugMode;
	qhandle_t			whiteShader;
	qhandle_t			menuBackShader;
	qhandle_t			cursor;
	float				scalex;
	float				scaley;
	//float				bias;
	qboolean			firstdraw;
} uiStatic_t;

extern void			UI_FillRect( float x, float y, float width, float height, const float *color );
extern void			UI_DrawString( int x, int y, const char* str, int style, vec4_t color );
extern void			UI_DrawHandlePic( float x, float y, float w, float h, qhandle_t hShader ); 
extern void			UI_UpdateScreen( void );
extern int			UI_RegisterFont(const char *fontName);
extern void			UI_SetColor( const float *rgba );
extern char			*UI_Cvar_VariableString( const char *var_name );

extern uiStatic_t	uis;
extern uiimport_t	ui;


#define MAX_MOVIES 256
#define MAX_MODS 64

typedef struct {
	const char *modName;
	const char *modDescr;
} modInfo_t;

typedef struct {
	displayContextDef_t uiDC;
	qboolean demoAvailable;

	const char *demoList[MAX_DEMOS];
	int demoCount;
	int demoIndex;

	int effectsColor;
	int currentCrosshair;

	modInfo_t modList[MAX_MODS];
	int modIndex;

	qboolean teamLeader;

	const char *movieList[MAX_MOVIES];
	int movieIndex;
	int previewMovie;

	int modCount;

	char		deferredScript [ MAX_DEFERRED_SCRIPT ];
	itemDef_t*	deferredScriptItem;

}	uiInfo_t;

extern uiInfo_t uiInfo;

//
// ui_main.c
//
void _UI_Init( qboolean inGameLoad );
void _UI_DrawRect( float x, float y, float width, float height, float size, const float *color );
void _UI_MouseEvent( int dx, int dy );
void _UI_KeyEvent( int key, qboolean down );
void UI_Report(void);

extern char GoToMenu[];


//
// ui_syscalls.c
//
int				trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits, const char *psAudioFile /* = NULL */);
int				trap_CIN_StopCinematic(int handle); 
void			trap_Cvar_Set( const char *var_name, const char *value );
float			trap_Cvar_VariableValue( const char *var_name );
void			trap_GetGlconfig( glconfig_t *glconfig );
void			trap_Key_ClearStates( void );
int				trap_Key_GetCatcher( void );
qboolean		trap_Key_GetOverstrikeMode( void );
void			trap_Key_SetBinding( int keynum, const char *binding );
void			trap_Key_SetCatcher( int catcher );
void			trap_Key_SetOverstrikeMode( qboolean state );
void			trap_R_DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader );
void			trap_R_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs );
void			trap_R_SetColor( const float *rgba );
void			trap_R_ClearScene( void );
void			trap_R_AddRefEntityToScene( const refEntity_t *re );
void			trap_R_RenderScene( const refdef_t *fd );
sfxHandle_t		trap_S_RegisterSound( const char *sample, qboolean compressed );
void			trap_S_StartLocalSound( sfxHandle_t sfx, int channelNum );
#ifdef _IMMERSION
ffHandle_t		trap_FF_Register( const char *name, int channel = FF_CHANNEL_MENU );
void			trap_FF_Start( ffHandle_t ff );
#endif // _IMMERSION
int				PASSFLOAT( float x );



void _UI_Refresh( int realtime );

#endif
