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

#pragma once

#include "qcommon/q_shared.h"
#include "rd-common/tr_types.h"
#include "keycodes.h"

#include "ui/menudef.h"

#define MAX_MENUNAME				32
#define MAX_ITEMTEXT				64
#define MAX_ITEMACTION				64
#define MAX_MENUDEFFILE				8192 //4096
#define MAX_MENUFILE				65536 //32768
#define MAX_MENUS					128 //64
#define MAX_MENUITEMS				512 //256
#define MAX_COLOR_RANGES			10
#define MAX_OPEN_MENUS				64 //16
#define	MAX_TEXTSCROLL_LINES		256

#define WINDOW_MOUSEOVER			0x00000001	// mouse is over it, non exclusive
#define WINDOW_HASFOCUS				0x00000002	// has cursor focus, exclusive
#define WINDOW_VISIBLE				0x00000004	// is visible
#define WINDOW_INACTIVE				0x00000008	// is visible but grey ( non-active )
#define WINDOW_DECORATION			0x00000010	// for decoration only, no mouse, keyboard, etc..
#define WINDOW_FADINGOUT			0x00000020	// fading out, non-active
#define WINDOW_FADINGIN				0x00000040	// fading in
#define WINDOW_MOUSEOVERTEXT		0x00000080	// mouse is over it, non exclusive
#define WINDOW_INTRANSITION			0x00000100	// window is in transition
#define WINDOW_FORECOLORSET			0x00000200	// forecolor was explicitly set ( used to color alpha images or not )
#define WINDOW_HORIZONTAL			0x00000400	// for list boxes and sliders, vertical is default this is set of horizontal
#define WINDOW_LB_LEFTARROW			0x00000800	// mouse is over left/up arrow
#define WINDOW_LB_RIGHTARROW		0x00001000	// mouse is over right/down arrow
#define WINDOW_LB_THUMB				0x00002000	// mouse is over thumb
#define WINDOW_LB_PGUP				0x00004000	// mouse is over page up
#define WINDOW_LB_PGDN				0x00008000	// mouse is over page down
#define WINDOW_ORBITING				0x00010000	// item is in orbit
#define WINDOW_OOB_CLICK			0x00020000	// close on out of bounds click
#define WINDOW_WRAPPED				0x00040000	// manually wrap text
#define WINDOW_AUTOWRAPPED			0x00080000	// auto wrap text
#define WINDOW_FORCED				0x00100000	// forced open
#define WINDOW_POPUP				0x00200000	// popup
#define WINDOW_BACKCOLORSET			0x00400000	// backcolor was explicitly set
#define WINDOW_TIMEDVISIBLE			0x00800000	// visibility timing ( NOT implemented )
#define WINDOW_PLAYERCOLOR			0x01000000	// hack the forecolor to match ui_char_color_*

//JLF
#define WINDOW_INTRANSITIONMODEL	0x04000000	// delayed script waiting to run


// cgame cursor type bits
#define CURSOR_NONE					0x00000001
#define CURSOR_ARROW				0x00000002
#define CURSOR_SIZER				0x00000004

#define STRING_POOL_SIZE (2*1024*1024)

#define MAX_STRING_HANDLES 4096
#define MAX_SCRIPT_ARGS 12
#define MAX_EDITFIELD 256

#define ART_FX_BASE			"menu/art/fx_base"
#define ART_FX_BLUE			"menu/art/fx_blue"
#define ART_FX_CYAN			"menu/art/fx_cyan"
#define ART_FX_GREEN		"menu/art/fx_grn"
#define ART_FX_RED			"menu/art/fx_red"
#define ART_FX_TEAL			"menu/art/fx_teal"
#define ART_FX_WHITE		"menu/art/fx_white"
#define ART_FX_YELLOW		"menu/art/fx_yel"
#define ART_FX_ORANGE		"menu/art/fx_orange"
#define ART_FX_PURPLE		"menu/art/fx_purple"

#define ASSET_GRADIENTBAR			"ui/assets/gradientbar2.tga"
#define ASSET_SCROLLBAR             "gfx/menus/scrollbar.tga"
#define ASSET_SCROLLBAR_ARROWDOWN   "gfx/menus/scrollbar_arrow_dwn_a.tga"
#define ASSET_SCROLLBAR_ARROWUP     "gfx/menus/scrollbar_arrow_up_a.tga"
#define ASSET_SCROLLBAR_ARROWLEFT   "gfx/menus/scrollbar_arrow_left.tga"
#define ASSET_SCROLLBAR_ARROWRIGHT  "gfx/menus/scrollbar_arrow_right.tga"
#define ASSET_SCROLL_THUMB          "gfx/menus/scrollbar_thumb.tga"
#define ASSET_SLIDER_BAR			"menu/new/slider"
#define ASSET_SLIDER_THUMB			"menu/new/sliderthumb"
#define SCROLLBAR_SIZE 16.0
#define SLIDER_WIDTH 96.0
#define SLIDER_HEIGHT 16.0
#define SLIDER_THUMB_WIDTH 12.0
#define SLIDER_THUMB_HEIGHT 20.0
#define	NUM_CROSSHAIRS			9

enum {
	SSF_JPEG = 0,
	SSF_TGA,
	SSF_PNG
};

typedef struct scriptDef_s {
  const char *command;
  const char *args[MAX_SCRIPT_ARGS];
} scriptDef_t;


typedef struct rectDef_s {
  float x;    // horiz position
  float y;    // vert position
  float w;    // width
  float h;    // height;
} rectDef_t;

//typedef rectDef_t Rectangle;

// FIXME: do something to separate text vs window stuff
typedef struct windowDef_s {
  rectDef_t rect;                 // client coord rectangle
  rectDef_t rectClient;           // screen coord rectangle
  const char *name;               //
  const char *group;              // if it belongs to a group
  const char *cinematicName;		  // cinematic name
  int cinematic;								  // cinematic handle
  int style;                      //
  int border;                     //
  int ownerDraw;									// ownerDraw style
	int ownerDrawFlags;							// show flags for ownerdraw items
  float borderSize;               //
  int flags;                      // visible, focus, mouseover, cursor
  rectDef_t rectEffects;          // for various effects
  rectDef_t rectEffects2;         // for various effects
  int offsetTime;                 // time based value for various effects
  int nextTime;                   // time next effect should cycle
  vec4_t foreColor;               // text color
  vec4_t backColor;               // border color
  vec4_t borderColor;             // border color
  vec4_t outlineColor;            // border color
  qhandle_t background;           // background asset
} windowDef_t;

typedef struct colorRangeDef_s {
	vec4_t	color;
	float		low;
	float		high;
} colorRangeDef_t;

// FIXME: combine flags into bitfields to save space
// FIXME: consolidate all of the common stuff in one structure for menus and items
// THINKABOUTME: is there any compelling reason not to have items contain items
// and do away with a menu per say.. major issue is not being able to dynamically allocate
// and destroy stuff.. Another point to consider is adding an alloc free call for vm's and have
// the engine just allocate the pool for it based on a cvar
// many of the vars are re-used for different item types, as such they are not always named appropriately
// the benefits of c++ in DOOM will greatly help crap like this
// FIXME: need to put a type ptr that points to specific type info per type
//
#define MAX_LB_COLUMNS 16

typedef struct columnInfo_s {
	int pos;
	int width;
	int maxChars;
} columnInfo_t;

typedef struct listBoxDef_s {
	int startPos;
	int endPos;
	int drawPadding;
	int cursorPos;
	float elementWidth;
	float elementHeight;
	int elementStyle;
	int numColumns;
	columnInfo_t columnInfo[MAX_LB_COLUMNS];
	const char *doubleClick;
	qboolean notselectable;
	//JLF MPMOVED
	qboolean	scrollhidden;
} listBoxDef_t;

typedef struct editFieldDef_s {
	float minVal;			//	edit field limits
	float maxVal;			//
	float defVal;			//
	float range;			//
	int maxChars;			// for edit fields
	int maxPaintChars;		// for edit fields
	int paintOffset;		//
} editFieldDef_t;

#define MAX_MULTI_CVARS 64//32

typedef struct multiDef_s {
	const char *cvarList[MAX_MULTI_CVARS];
	const char *cvarStr[MAX_MULTI_CVARS];
	float cvarValue[MAX_MULTI_CVARS];
	int count;
	qboolean strDef;
} multiDef_t;

typedef struct modelDef_s {
	int angle;
	vec3_t origin;
	float fov_x;
	float fov_y;
	int rotationSpeed;

	vec3_t g2mins; //required
	vec3_t g2maxs; //required
	vec3_t g2scale; //optional
	int g2skin; //optional
	int g2anim; //optional
	//JLF
//Transition extras
	vec3_t g2mins2, g2maxs2, g2minsEffect, g2maxsEffect;
	float fov_x2, fov_y2, fov_Effectx, fov_Effecty;
} modelDef_t;

typedef struct textScrollDef_s
{
	int				startPos;
	int				endPos;

	float			lineHeight;
	int				maxLineChars;
	int				drawPadding;

	// changed spelling to make them fall out during compile while I made them asian-aware	-Ste
	//
	int				iLineCount;
	const char*		pLines[MAX_TEXTSCROLL_LINES];	// can contain NULL ptrs that you should skip over during paint.

} textScrollDef_t;

#define ITEM_ALIGN_LEFT		0		// left alignment
#define ITEM_ALIGN_CENTER	1		// center alignment
#define ITEM_ALIGN_RIGHT	2		// right alignment

#define CVAR_ENABLE			0x00000001
#define CVAR_DISABLE		0x00000002
#define CVAR_SHOW			0x00000004
#define CVAR_HIDE			0x00000008

#define ITF_G2VALID			0x0001					// indicates whether or not g2 instance is valid.
#define ITF_ISCHARACTER		0x0002					// a character item, uses customRGBA
#define ITF_ISSABER			0x0004					// first saber item, draws blade
#define ITF_ISSABER2		0x0008					// second saber item, draws blade

#define ITF_ISANYSABER		(ITF_ISSABER|ITF_ISSABER2)	//either saber

typedef struct itemDef_s {
	windowDef_t	window;						// common positional, border, style, layout info
	rectDef_t	textRect;					// rectangle the text ( if any ) consumes
	int			type;						// text, button, radiobutton, checkbox, textfield, listbox, combo
	int			alignment;					// left center right
	int			textalignment;				// ( optional ) alignment for text within rect based on text width
	float		textalignx;					// ( optional ) text alignment x coord
	float		textaligny;					// ( optional ) text alignment x coord
	float		textscale;					// scale percentage from 72pts
	int			textStyle;					// ( optional ) style, normal and shadowed are it for now
	const char	*text;						// display text
	const char	*text2;						// display text, 2nd line
	float		text2alignx;				// ( optional ) text2 alignment x coord
	float		text2aligny;				// ( optional ) text2 alignment y coord
	void		*parent;					// menu owner
	qhandle_t	asset;						// handle to asset
	void		*ghoul2;					// ghoul2 instance if available instead of a model.
	int			flags;						// flags like g2valid, character, saber, saber2, etc.
	const char	*mouseEnterText;			// mouse enter script
	const char	*mouseExitText;				// mouse exit script
	const char	*mouseEnter;				// mouse enter script
	const char	*mouseExit;					// mouse exit script
	const char	*action;					// select script
//JLFACCEPT MPMOVED
	const char  *accept;
//JLFDPADSCRIPT
	const char * selectionNext;
	const char * selectionPrev;

	const char	*onFocus;					// select script
	const char	*leaveFocus;				// select script
	const char	*cvar;						// associated cvar
	const char	*cvarTest;					// associated cvar for enable actions
	const char	*enableCvar;				// enable, disable, show, or hide based on value, this can contain a list
	int			cvarFlags;					//	what type of action to take on cvarenables
	sfxHandle_t focusSound;
	int			numColors;					// number of color ranges
	colorRangeDef_t colorRanges[MAX_COLOR_RANGES];
	float		special;					// used for feeder id's etc.. diff per type
	int			cursorPos;					// cursor position in characters
	union {
		void			*data;
		listBoxDef_t	*listbox;
		textScrollDef_t	*textscroll;
		editFieldDef_t	*edit;
		multiDef_t		*multi;
		modelDef_t		*model;
	} typeData;								// type specific data ptr's
	const char	*descText;					//	Description text
	int			appearanceSlot;				// order of appearance
	int			iMenuFont;					// FONT_SMALL,FONT_MEDIUM,FONT_LARGE	// changed from 'font' so I could see what didn't compile, and differentiate between font handles returned from RegisterFont -ste
	qboolean	disabled;					// Does this item ignore mouse and keyboard focus
	int			invertYesNo;
	int			xoffset;

	qboolean disabledHidden;				// hide the item when 'disabled' is true (for generic image items)
} itemDef_t;

typedef struct menuDef_s {
	windowDef_t window;
	const char  *font;						// font
	qboolean fullScreen;					// covers entire screen
	int itemCount;							// number of items;
	int fontIndex;							//
	int cursorItem;							// which item as the cursor
	int fadeCycle;							//
	float fadeClamp;						//
	float fadeAmount;						//
	const char *onOpen;						// run when the menu is first opened
	const char *onClose;					// run when the menu is closed
//JLFACCEPT
	const char  *onAccept;					// run when menu is closed with acceptance

	const char *onESC;						// run when the menu is closed
	const char *soundName;					// background loop sound for menu

	vec4_t focusColor;						// focus color for items
	vec4_t disableColor;					// focus color for items
	itemDef_t *items[MAX_MENUITEMS];		// items this menu contains
	int			descX;						// X position of description
	int			descY;						// X position of description
	vec4_t		descColor;					// description text color for items
	int			descAlignment;				// Description of alignment
	float		descScale;					// Description scale
	float		appearanceTime;				//	when next item should appear
	int			appearanceCnt;				//	current item displayed
	float		appearanceIncrement;		//
} menuDef_t;

typedef struct cachedAssets_s {
	const char *fontStr;
	const char *cursorStr;
	const char *gradientStr;
	qhandle_t	qhSmallFont;
	qhandle_t	qhSmall2Font;
	qhandle_t	qhMediumFont;
	qhandle_t	qhBigFont;
	qhandle_t cursor;
	qhandle_t gradientBar;
	qhandle_t scrollBarArrowUp;
	qhandle_t scrollBarArrowDown;
	qhandle_t scrollBarArrowLeft;
	qhandle_t scrollBarArrowRight;
	qhandle_t scrollBar;
	qhandle_t scrollBarThumb;
	qhandle_t buttonMiddle;
	qhandle_t buttonInside;
	qhandle_t solidBox;
	qhandle_t sliderBar;
	qhandle_t sliderThumb;
	sfxHandle_t menuEnterSound;
	sfxHandle_t menuExitSound;
	sfxHandle_t menuBuzzSound;
	sfxHandle_t itemFocusSound;
	float fadeClamp;
	int fadeCycle;
	float fadeAmount;
	float shadowX;
	float shadowY;
	vec4_t shadowColor;
	float shadowFadeClamp;
	qboolean fontRegistered;

	qhandle_t needPass;
	qhandle_t noForce;
	qhandle_t forceRestrict;
	qhandle_t saberOnly;
	qhandle_t trueJedi;

	sfxHandle_t moveRollSound;
	sfxHandle_t moveJumpSound;
	sfxHandle_t datapadmoveSaberSound1;
	sfxHandle_t datapadmoveSaberSound2;
	sfxHandle_t datapadmoveSaberSound3;
	sfxHandle_t datapadmoveSaberSound4;
	sfxHandle_t datapadmoveSaberSound5;
	sfxHandle_t datapadmoveSaberSound6;

	// player settings
	qhandle_t fxBasePic;
	qhandle_t fxPic[7];
	qhandle_t crosshairShader[NUM_CROSSHAIRS];
} cachedAssets_t;

typedef struct commandDef_s {
	const char *name;
	qboolean (*handler) (itemDef_t *item, char** args);
} commandDef_t;

typedef struct displayContextDef_s {
	qhandle_t		(*registerShaderNoMip)				( const char *p );
	void			(*setColor)							( const vec4_t v );
	void			(*drawHandlePic)					( float x, float y, float w, float h, qhandle_t asset );
	void			(*drawStretchPic)					( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader );
	void			(*drawText)							( float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style, int iMenuFont );
	int				(*textWidth)						( const char *text, float scale, int iMenuFont );
	int				(*textHeight)						( const char *text, float scale, int iMenuFont );
	qhandle_t		(*registerModel)					( const char *p );
	void			(*modelBounds)						( qhandle_t model, vec3_t min, vec3_t max );
	void			(*fillRect)							( float x, float y, float w, float h, const vec4_t color );
	void			(*drawRect)							( float x, float y, float w, float h, float size, const vec4_t color );
	void			(*drawSides)						( float x, float y, float w, float h, float size );
	void			(*drawTopBottom)					( float x, float y, float w, float h, float size );
	void			(*clearScene)						( void );
	void			(*addRefEntityToScene)				( const refEntity_t *re );
	void			(*renderScene)						( const refdef_t *fd );
	qhandle_t		(*RegisterFont)						( const char *fontName );
	int				(*Font_StrLenPixels)				( const char *text, const int iFontIndex, const float scale );
	int				(*Font_StrLenChars)					( const char *text );
	int				(*Font_HeightPixels)				( const int iFontIndex, const float scale );
	void			(*Font_DrawString)					( int ox, int oy, const char *text, const float *rgba, const int setIndex, int iCharLimit, const float scale );
	qboolean		(*Language_IsAsian)					( void );
	qboolean		(*Language_UsesSpaces)				( void );
	unsigned int	(*AnyLanguage_ReadCharFromString)	( const char *psText, int *piAdvanceCount, qboolean *pbIsTrailingPunctuation );
	void			(*ownerDrawItem)					( float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float special, float scale, vec4_t color, qhandle_t shader, int textStyle, int iMenuFont );
	float			(*getValue)							( int ownerDraw );
	qboolean		(*ownerDrawVisible)					( int flags );
	void			(*runScript)						( char **p );
	qboolean		(*deferScript)						( char **p );
	void			(*getTeamColor)						( vec4_t *color );
	void			(*getCVarString)					( const char *cvar, char *buffer, int bufsize );
	float			(*getCVarValue)						( const char *cvar );
	void			(*setCVar)							( const char *cvar, const char *value );
	void			(*drawTextWithCursor)				( float x, float y, float scale, vec4_t color, const char *text, int cursorPos, char cursor, int limit, int style, int iFontIndex );
	void			(*setOverstrikeMode)				( qboolean b );
	qboolean		(*getOverstrikeMode)				( void );
	void			(*startLocalSound)					( sfxHandle_t sfx, int channelNum );
	qboolean		(*ownerDrawHandleKey)				( int ownerDraw, int flags, float *special, int key );
	int				(*feederCount)						( float feederID );
	const char *	(*feederItemText)					( float feederID, int index, int column, qhandle_t *handle1, qhandle_t *handle2, qhandle_t *handle3 );
	qhandle_t		(*feederItemImage)					( float feederID, int index );
	qboolean		(*feederSelection)					( float feederID, int index, itemDef_t *item );
	void			(*keynumToStringBuf)				( int keynum, char *buf, int buflen );
	void			(*getBindingBuf)					( int keynum, char *buf, int buflen );
	void			(*setBinding)						( int keynum, const char *binding );
	void			(*executeText)						( int exec_when, const char *text );
	NORETURN_PTR void (*Error)( int level, const char *fmt, ... );
	void			(*Print)							( const char *msg, ... );
	void			(*Pause)							( qboolean b );
	int				(*ownerDrawWidth)					( int ownerDraw, float scale );
	sfxHandle_t		(*registerSound)					( const char *name );
	void			(*startBackgroundTrack)				( const char *intro, const char *loop, qboolean bReturnWithoutStarting );
	void			(*stopBackgroundTrack)				( void );
	int				(*playCinematic)					( const char *name, float x, float y, float w, float h );
	void			(*stopCinematic)					( int handle );
	void			(*drawCinematic)					( int handle, float x, float y, float w, float h );
	void			(*runCinematicFrame)				( int handle );

	float			yscale;
	float			xscale;
	float			bias;
	int				realTime;
	int				frameTime;
	int				cursorx;
	int				cursory;
	qboolean		debug;

	cachedAssets_t	Assets;

	glconfig_t		glconfig;
	qhandle_t		whiteShader;
	qhandle_t		gradientImage;
	qhandle_t		cursor;
	float			FPS;
	int				screenshotFormat;

	struct {
		float			(*Font_StrLenPixels)				( const char *text, const int iFontIndex, const float scale );
	} ext;
} displayContextDef_t;


const char *String_Alloc(const char *p);
void String_Init();
void String_Report();
void Init_Display(displayContextDef_t *dc);
void Display_ExpandMacros(char * buff);
void Menu_Init(menuDef_t *menu);
void Item_Init(itemDef_t *item);
void Menu_PostParse(menuDef_t *menu);
menuDef_t *Menu_GetFocused();
void Menu_HandleKey(menuDef_t *menu, int key, qboolean down);
void Menu_HandleMouseMove(menuDef_t *menu, float x, float y);
void Menu_ScrollFeeder(menuDef_t *menu, int feeder, qboolean down);
qboolean Float_Parse(char **p, float *f);
qboolean Color_Parse(char **p, vec4_t *c);
qboolean Int_Parse(char **p, int *i);
qboolean Rect_Parse(char **p, rectDef_t *r);
qboolean String_Parse(char **p, const char **out);
qboolean Script_Parse(char **p, const char **out);
qboolean PC_Float_Parse(int handle, float *f);
qboolean PC_Color_Parse(int handle, vec4_t *c);
qboolean PC_Int_Parse(int handle, int *i);
qboolean PC_Rect_Parse(int handle, rectDef_t *r);
qboolean PC_String_Parse(int handle, const char **out);
qboolean PC_Script_Parse(int handle, const char **out);
int Menu_Count();
void Menu_New(int handle);
void Menu_PaintAll();
menuDef_t *Menus_ActivateByName(const char *p);
void Menu_Reset(void);
qboolean Menus_AnyFullScreenVisible( void );
void  Menus_Activate(menuDef_t *menu);
itemDef_t *Menu_FindItemByName(menuDef_t *menu, const char *p);
void Menu_ShowGroup (menuDef_t *menu, const char *itemName, qboolean showFlag);
void Menu_ItemDisable(menuDef_t *menu, const char *name, qboolean disableFlag);
int Menu_ItemsMatchingGroup(menuDef_t *menu, const char *name);
itemDef_t *Menu_GetMatchingItemByNumber(menuDef_t *menu, int index, const char *name);

displayContextDef_t *Display_GetContext();
void *Display_CaptureItem(int x, int y);
qboolean Display_MouseMove(void *p, int x, int y);
int Display_CursorType(int x, int y);
qboolean Display_KeyBindPending();
void Menus_OpenByName(const char *p);
menuDef_t *Menus_FindByName(const char *p);
void Menus_ShowByName(const char *p);
void Menus_CloseByName(const char *p);
void Display_HandleKey(int key, qboolean down, int x, int y);
void LerpColor(vec4_t a, vec4_t b, vec4_t c, float t);
void Menus_CloseAll();
void Menu_Paint(menuDef_t *menu, qboolean forcePaint);
void Menu_SetFeederSelection(menuDef_t *menu, int feeder, int index, const char *name);
void Display_CacheAll();
void Menu_SetItemBackground(const menuDef_t *menu,const char *itemName, const char *background);

void *UI_Alloc( int size );
void UI_InitMemory( void );
qboolean UI_OutOfMemory();

void Controls_GetConfig( void );
void Controls_SetConfig( void );
void Controls_SetDefaults( void );

/*
Ghoul2 Insert End
*/

extern const char *HolocronIcons[NUM_FORCE_POWERS];
