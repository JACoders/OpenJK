/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
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

#ifndef __UI_LOCAL_H__
#define __UI_LOCAL_H__

#include <string.h>
#include <limits.h>

#include "../qcommon/q_shared.h"
#include "../rd-common/tr_types.h"
#include "../qcommon/qcommon.h"
#include "ui_public.h"
#include "ui_shared.h"

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
extern void UI_UpdateConnectionString( const char *string );
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

#define SKIN_LENGTH			16
#define ACTION_BUFFER_SIZE	128

typedef struct {
	char name[SKIN_LENGTH];
} skinName_t;

typedef struct {
	char shader[MAX_QPATH];
	char actionText[ACTION_BUFFER_SIZE];
} playerColor_t;

typedef struct {
	char		Name[64];
	int			SkinHeadCount;
	int			SkinHeadMax;
	skinName_t	*SkinHead;
	int			SkinTorsoCount;
	int			SkinTorsoMax;
	skinName_t	*SkinTorso;
	int			SkinLegCount;
	int			SkinLegMax;
	skinName_t	*SkinLeg;
	int			ColorMax;
	int			ColorCount;
	playerColor_t	*Color;
} playerSpeciesInfo_t;

typedef struct {
	displayContextDef_t uiDC;

	int effectsColor;
	int currentCrosshair;

	modInfo_t modList[MAX_MODS];
	int modIndex;
	int modCount;

	int					playerSpeciesMax;
	int					playerSpeciesCount;
	playerSpeciesInfo_t	*playerSpecies;
	int					playerSpeciesIndex;


	char		deferredScript [ MAX_DEFERRED_SCRIPT ];
	itemDef_t*	deferredScriptItem;

	itemDef_t*	runScriptItem;

	qboolean inGameLoad;
	// Used by Force Power allocation screen
	short	forcePowerUpdated;					// Enum of which power had the point allocated
	// Used by Weapon allocation screen
	short	selectedWeapon1;					// 1st weapon chosen
	char 	selectedWeapon1ItemName[64];		// Item name of weapon chosen
	int		selectedWeapon1AmmoIndex;			// Holds index to ammo
	short	selectedWeapon2;					// 2nd weapon chosen
	char 	selectedWeapon2ItemName[64];		// Item name of weapon chosen
	int		selectedWeapon2AmmoIndex;			// Holds index to ammo
	short	selectedThrowWeapon;				// throwable weapon chosen
	char 	selectedThrowWeaponItemName[64];	// Item name of weapon chosen
	int		selectedThrowWeaponAmmoIndex;		// Holds index to ammo

	itemDef_t *weapon1ItemButton;
	qhandle_t litWeapon1Icon;
	qhandle_t unlitWeapon1Icon;
	itemDef_t *weapon2ItemButton;
	qhandle_t litWeapon2Icon;
	qhandle_t unlitWeapon2Icon;

	itemDef_t *weaponThrowButton;
	qhandle_t litThrowableIcon;
	qhandle_t unlitThrowableIcon;
	short		movesTitleIndex;
	const char	*movesBaseAnim;
	int			moveAnimTime;
	int			languageCount;
	int			languageCountIndex;

	int			forcePowerLevel[NUM_FORCE_POWERS];
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
void			trap_S_StopSounds( void );
sfxHandle_t		trap_S_RegisterSound( const char *sample, qboolean compressed );
void			trap_S_StartLocalSound( sfxHandle_t sfx, int channelNum );



void _UI_Refresh( int realtime );

#endif
