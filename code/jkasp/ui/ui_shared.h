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

#ifndef __UI_SHARED_H
#define __UI_SHARED_H

enum {
	SSF_JPEG = 0,
	SSF_TGA,
	SSF_PNG
};

#define MAX_TOKENLENGTH		1024
#define MAX_OPEN_MENUS 16
#define	MAX_TEXTSCROLL_LINES		256

#define MAX_EDITFIELD 256

#ifndef TT_STRING
//token types
#define TT_STRING					1			// string
#define TT_LITERAL					2			// literal
#define TT_NUMBER					3			// number
#define TT_NAME						4			// name
#define TT_PUNCTUATION				5			// punctuation
#endif

#define SLIDER_WIDTH 128.0
#define SLIDER_HEIGHT 16.0
#define SLIDER_THUMB_WIDTH 12.0
#define SLIDER_THUMB_HEIGHT 16.0
#define SCROLLBAR_SIZE 16.0

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
	int			pos;
	int			width;
	int			maxChars;
} columnInfo_t;

typedef struct listBoxDef_s {
	int			startPos;
	int			endPos;
	int			drawPadding;
	int			cursorPos;
	float		elementWidth;
	float		elementHeight;
	int			elementStyle;
	int			numColumns;
	columnInfo_t columnInfo[MAX_LB_COLUMNS];
	const char	*doubleClick;
	qboolean	notselectable;
//JLF MPMOVED
	qboolean	scrollhidden;
} listBoxDef_t;


typedef struct editFieldDef_s {
	float		minVal;						//	edit field limits
	float		maxVal;						//
	float		defVal;						//
	float		range;						//
	int			maxChars;					// for edit fields
	int			maxPaintChars;				// for edit fields
	int			paintOffset;				//
} editFieldDef_t;

#define MAX_MULTI_CVARS 64//32

typedef struct multiDef_s {
	const char *cvarList[MAX_MULTI_CVARS];
	const char *cvarStr[MAX_MULTI_CVARS];
	float		cvarValue[MAX_MULTI_CVARS];
	int			count;
	qboolean	strDef;
} multiDef_t;

#define CVAR_ENABLE		0x00000001
#define CVAR_DISABLE	0x00000002
#define CVAR_SHOW		0x00000004
#define CVAR_HIDE		0x00000008
#define CVAR_SUBSTRING	0x00000010	//when using enable or disable, just check for strstr instead of ==

#define STRING_POOL_SIZE (2*1024*1024)

#define	NUM_CROSSHAIRS			9

typedef struct {
	qhandle_t	qhMediumFont;
	qhandle_t	cursor;
	qhandle_t	gradientBar;
	qhandle_t	scrollBarArrowUp;
	qhandle_t	scrollBarArrowDown;
	qhandle_t	scrollBarArrowLeft;
	qhandle_t	scrollBarArrowRight;
	qhandle_t	scrollBar;
	qhandle_t	scrollBarThumb;
	qhandle_t	buttonMiddle;
	qhandle_t	buttonInside;
	qhandle_t	solidBox;
	qhandle_t	sliderBar;
	qhandle_t	sliderThumb;
	sfxHandle_t menuEnterSound;
	sfxHandle_t menuExitSound;
	sfxHandle_t menuBuzzSound;
	sfxHandle_t itemFocusSound;
	sfxHandle_t forceChosenSound;
	sfxHandle_t forceUnchosenSound;
	sfxHandle_t datapadmoveRollSound;
	sfxHandle_t datapadmoveJumpSound;
	sfxHandle_t datapadmoveSaberSound1;
	sfxHandle_t datapadmoveSaberSound2;
	sfxHandle_t datapadmoveSaberSound3;
	sfxHandle_t datapadmoveSaberSound4;
	sfxHandle_t datapadmoveSaberSound5;
	sfxHandle_t datapadmoveSaberSound6;

	sfxHandle_t nullSound;

	float		fadeClamp;
	int			fadeCycle;
	float		fadeAmount;
	float		shadowX;
	float		shadowY;
	vec4_t		shadowColor;
	float		shadowFadeClamp;
	qboolean	fontRegistered;

  // player settings
//	qhandle_t	fxBasePic;
//	qhandle_t	fxPic[7];
	qhandle_t	crosshairShader[NUM_CROSSHAIRS];

} cachedAssets_t;

struct itemDef_s;

typedef struct {

	void		(*addRefEntityToScene) (const refEntity_t *re );
	void		(*clearScene) ();
	void		(*drawHandlePic) (float x, float y, float w, float h, qhandle_t asset);
	void		(*drawRect) ( float x, float y, float w, float h, float size, const vec4_t color);
	void		(*drawSides) (float x, float y, float w, float h, float size);
	void		(*drawText) (float x, float y, float scale, vec4_t color, const char *text, int iMaxPixelWidth, int style, int iFontIndex );
	void		(*drawTextWithCursor)(float x, float y, float scale, vec4_t color, const char *text, int cursorPos, char cursor, int iMaxPixelWidth, int style, int iFontIndex);
	void		(*drawTopBottom) (float x, float y, float w, float h, float size);
	void		(*executeText)(int exec_when, const char *text );
	int			(*feederCount)(float feederID);
	void		(*feederSelection)(float feederID, int index, struct itemDef_s *item);
	void		(*fillRect) ( float x, float y, float w, float h, const vec4_t color);
	void		(*getBindingBuf)( int keynum, char *buf, int buflen );
	void		(*getCVarString)(const char *cvar, char *buffer, int bufsize);
	float		(*getCVarValue)(const char *cvar);
	qboolean	(*getOverstrikeMode)();
	float		(*getValue) (int ownerDraw);
	void		(*keynumToStringBuf)( int keynum, char *buf, int buflen );
	void		(*modelBounds) (qhandle_t model, vec3_t min, vec3_t max);
	qboolean	(*ownerDrawHandleKey)(int ownerDraw, int flags, float *special, int key);
	void		(*ownerDrawItem) (float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float special, float scale, vec4_t color, qhandle_t shader, int textStyle, int iFontIndex);
	qboolean	(*ownerDrawVisible) (int flags);
	int			(*ownerDrawWidth)(int ownerDraw, float scale);
	void		(*Pause)(qboolean b);
	void		(*Print)(const char *msg, ...);
	int			(*registerFont) (const char *pFontname);
	qhandle_t	(*registerModel) (const char *p);
	qhandle_t	(*registerShaderNoMip) (const char *p);
	sfxHandle_t (*registerSound)(const char *name, qboolean compressed);
	void		(*renderScene) ( const refdef_t *fd );
	qboolean	(*runScript)(const char **p);
	qboolean	(*deferScript)(const char **p);
	void		(*setBinding)( int keynum, const char *binding );
	void		(*setColor) (const vec4_t v);
	void		(*setCVar)(const char *cvar, const char *value);
	void		(*setOverstrikeMode)(qboolean b);
	void		(*startLocalSound)( sfxHandle_t sfx, int channelNum );
	void		(*stopCinematic)(int handle);
	int			(*textHeight) (const char *text, float scale, int iFontIndex);
	int			(*textWidth) (const char *text, float scale, int iFontIndex);
	qhandle_t	(*feederItemImage) (float feederID, int index);
	const char *(*feederItemText) (float feederID, int index, int column, qhandle_t *handle);

	qhandle_t	(*registerSkin)( const char *name );

	//rww - ghoul2 stuff. Add whatever you need here, remember to set it in _UI_Init or it will crash when you try to use it.
	qboolean	(*g2_SetSkin)(CGhoul2Info *ghlInfo, qhandle_t customSkin, qhandle_t renderSkin );
	qboolean	(*g2_SetBoneAnim)(CGhoul2Info *ghlInfo, const char *boneName, const int startFrame, const int endFrame,
					  const int flags, const float animSpeed, const int currentTime, const float setFrame, const int blendTime );
	qboolean	(*g2_RemoveGhoul2Model)(CGhoul2Info_v &ghlInfo, const int modelIndex);
	int			(*g2_InitGhoul2Model)(CGhoul2Info_v &ghoul2, const char *fileName, int, qhandle_t customSkin, qhandle_t customShader, int modelFlags, int lodBias);
	void		(*g2_CleanGhoul2Models)(CGhoul2Info_v &ghoul2);
	int			(*g2_AddBolt)(CGhoul2Info *ghlInfo, const char *boneName);
	qboolean	(*g2_GetBoltMatrix)(CGhoul2Info_v &ghoul2, const int modelIndex, const int boltIndex, mdxaBone_t *matrix,
									const vec3_t angles, const vec3_t position, const int frameNum, qhandle_t *modelList, const vec3_t scale);
	void		(*g2_GiveMeVectorFromMatrix)(mdxaBone_t &boltMatrix, Eorientations flags, vec3_t &vec);

	//Utility functions that don't immediately redirect to ghoul2 functions
	int			(*g2hilev_SetAnim)(CGhoul2Info *ghlInfo, const char *boneName, int animNum, const qboolean freeze);

	float		yscale;
	float		xscale;
	float		bias;
	int			realTime;
	int			frameTime;
	qboolean	cursorShow;
	int			cursorx;
	int			cursory;
	qboolean	debug;

	cachedAssets_t Assets;

	glconfig_t	glconfig;
	qhandle_t	whiteShader;
	qhandle_t	gradientImage;
	float FPS;

	int			screenshotFormat;

} displayContextDef_t;

void UI_InitMemory( void );


#define MAX_COLOR_RANGES	10
#define MAX_MENUITEMS		150
#define MAX_MENUS			64



#define WINDOW_MOUSEOVER		0x00000001	// mouse is over it, non exclusive
#define WINDOW_HASFOCUS			0x00000002	// has cursor focus, exclusive
#define WINDOW_VISIBLE			0x00000004	// is visible
#define WINDOW_INACTIVE			0x00000008	// is visible but grey ( non-active )
#define WINDOW_DECORATION		0x00000010	// for decoration only, no mouse, keyboard, etc..
#define WINDOW_FADINGOUT		0x00000020	// fading out, non-active
#define WINDOW_FADINGIN			0x00000040	// fading in
#define WINDOW_MOUSEOVERTEXT	0x00000080	// mouse is over it, non exclusive
#define WINDOW_INTRANSITION		0x00000100	// window is in transition
#define WINDOW_FORECOLORSET		0x00000200	// forecolor was explicitly set ( used to color alpha images or not )
#define WINDOW_HORIZONTAL		0x00000400	// for list boxes and sliders, vertical is default this is set of horizontal
#define WINDOW_LB_LEFTARROW		0x00000800	// mouse is over left/up arrow
#define WINDOW_LB_RIGHTARROW	0x00001000	// mouse is over right/down arrow
#define WINDOW_LB_THUMB			0x00002000	// mouse is over thumb
#define WINDOW_LB_PGUP			0x00004000	// mouse is over page up
#define WINDOW_LB_PGDN			0x00008000	// mouse is over page down
#define WINDOW_ORBITING			0x00010000	// item is in orbit
#define WINDOW_OOB_CLICK		0x00020000	// close on out of bounds click
#define WINDOW_WRAPPED			0x00040000	// manually wrap text
#define WINDOW_AUTOWRAPPED		0x00080000	// auto wrap text
#define WINDOW_FORCED			0x00100000	// forced open
#define WINDOW_POPUP			0x00200000	// popup
#define WINDOW_BACKCOLORSET		0x00400000	// backcolor was explicitly set
#define WINDOW_TIMEDVISIBLE		0x00800000	// visibility timing ( NOT implemented )
#define WINDOW_PLAYERCOLOR		0x01000000	// hack the forecolor to match ui_char_color_*
#define WINDOW_SCRIPTWAITING	0x02000000	// delayed script waiting to run
//JLF MPMOVED
#define WINDOW_INTRANSITIONMODEL	0x04000000	// delayed script waiting to run
#define WINDOW_IGNORE_ESCAPE	0x08000000	// ignore normal closeall menus escape functionality

typedef struct {
	float		x;							// horiz position
	float		y;							// vert position
	float		w;							// width
	float		h;							// height;
} rectDef_t;

typedef rectDef_t UIRectangle;

// FIXME: do something to separate text vs window stuff
typedef struct {
	UIRectangle	rect;						// client coord rectangle
	UIRectangle	rectClient;					// screen coord rectangle
	char		*name;						//
	char		*group;						// if it belongs to a group
	const char	*cinematicName;				// cinematic name
	int			cinematic;					// cinematic handle
	int			style;                      //
	int			border;                     //
	int			ownerDraw;					// ownerDraw style
	int			ownerDrawFlags;				// show flags for ownerdraw items
	float		borderSize;					//
	int			flags;						// visible, focus, mouseover, cursor
	UIRectangle	rectEffects;				// for various effects
	UIRectangle	rectEffects2;				// for various effects
	int			offsetTime;					// time based value for various effects
	int			nextTime;                   // time next effect should cycle
	int			delayTime;					// time when delay expires
	char		*delayedScript;				// points into another script's text while delaying
	vec4_t		foreColor;					// text color
	vec4_t		backColor;					// border color
	vec4_t		borderColor;				// border color
	vec4_t		outlineColor;				// border color
	qhandle_t	background;					// background asset
} windowDef_t;

typedef windowDef_t Window;

typedef struct {
	vec4_t		color;						//
	float		low;						//
	float		high;						//
} colorRangeDef_t;

typedef struct modelDef_s {
	int		angle;
	vec3_t	origin;
	float	fov_x;
	float	fov_y;
	int		rotationSpeed;

	vec3_t g2mins; //required
	vec3_t g2maxs; //required
	int g2skin; //optional
	int g2anim; //optional
//JLF MPMOVED
//Transition extras
	vec3_t g2mins2, g2maxs2, g2minsEffect, g2maxsEffect;
	float fov_x2, fov_y2, fov_Effectx, fov_Effecty;
} modelDef_t;

#define ITF_G2VALID			0x0001					// indicates whether or not g2 instance is valid.
#define ITF_ISCHARACTER		0x0002					// a character item, uses customRGBA
#define ITF_ISSABER			0x0004					// first saber item, draws blade
#define ITF_ISSABER2		0x0008					// second saber item, draws blade

#define ITF_ISANYSABER		(ITF_ISSABER|ITF_ISSABER2)	//either saber

typedef struct itemDef_s {
	Window		window;						// common positional, border, style, layout info
	UIRectangle	textRect;					// rectangle the text ( if any ) consumes
	int			type;						// text, button, radiobutton, checkbox, textfield, listbox, combo
	int			alignment;					// left center right
	int			textalignment;				// ( optional ) alignment for text within rect based on text width
	float		textalignx;					// ( optional ) text alignment x coord
	float		textaligny;					// ( optional ) text alignment y coord
	float		text2alignx;				// ( optional ) text2 alignment x coord
	float		text2aligny;				// ( optional ) text2 alignment y coord
	float		textscale;					// scale percentage from 72pts
	int			textStyle;					// ( optional ) style, normal and shadowed are it for now
	char		*text;						// display text
	char		*text2;						// display text2
	const char		*descText;				//	Description text
	void		*parent;					// menu owner
	qhandle_t	asset;						// handle to asset
	CGhoul2Info_v ghoul2;					// ghoul2 instance if available instead of a model.
	int			flags;						// flags like g2valid, character, saber, saber2, etc.
	const char	*mouseEnterText;			// mouse enter script
	const char	*mouseExitText;				// mouse exit script
	const char	*mouseEnter;				// mouse enter script
	const char	*mouseExit;					// mouse exit script
	const char	*action;					// select script
//JLFACCEPT MPMOVED
	const char  *accept;
//JLFDPADSCRIPT MPMOVED
	const char * selectionNext;
	const char * selectionPrev;

	const char	*onFocus;					// select script
	const char	*leaveFocus;				// select script
	const char	*cvar;						// associated cvar
	const char	*cvarTest;					// associated cvar for enable actions
	const char	*enableCvar;				// enable, disable, show, or hide based on value, this can contain a list
	int			cvarFlags;					//	what type of action to take on cvarenables
	sfxHandle_t focusSound;					//
	int			numColors;					// number of color ranges
	colorRangeDef_t colorRanges[MAX_COLOR_RANGES];
	float		special;					// used for feeder id's etc.. diff per type
	int			cursorPos;					// cursor position in characters
	void		*typeData;					// type specific data ptr's
	int			appearanceSlot;				// order of appearance
	int			value;						// used by ITEM_TYPE_MULTI that aren't linked to a particular cvar.
	int			font;						// FONT_SMALL,FONT_MEDIUM,FONT_LARGE
	int			invertYesNo;
	int			xoffset;
	qboolean	disabled;					// Does this item ignore mouse and keyboard focus
	qboolean	disabledHidden;				// hide the item when 'disabled' is true (for generic image items)
} itemDef_t;

typedef struct {
	Window window;
	const char  *font;						// font
	qboolean	fullScreen;					// covers entire screen
	int			itemCount;					// number of items;
	int			fontIndex;					//
	int			cursorItem;					// which item as the cursor
	int			fadeCycle;					//
	float		fadeClamp;					//
	float		fadeAmount;					//
	const char	*onOpen;					// run when the menu is first opened
	const char	*onClose;					// run when the menu is closed
//JLFACCEPT MPMOVED
	const char  *onAccept;					// run when menu is closed with acceptance

	const char	*onESC;						// run when the menu is closed
	const char	*soundName;					// background loop sound for menu

	vec4_t		focusColor;					// focus color for items
	vec4_t		disableColor;				// focus color for items
	itemDef_t	*items[MAX_MENUITEMS];		// items this menu contains
	float		appearanceTime;				//	when next item should appear
	int			appearanceCnt;				//	current item displayed
	float		appearanceIncrement;		//
	int			descX;						// X position of description
	int			descY;						// X position of description
	vec4_t		descColor;					// description text color for items
	int			descAlignment;				// Description of alignment
	float		descScale;					// Description scale
	int			descTextStyle;					// ( optional ) style, normal and shadowed are it for now


} menuDef_t;

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

typedef struct
{
	const char *name;
	qboolean (*handler) (itemDef_t *item, const char** args);

} commandDef_t;

menuDef_t	*Menu_GetFocused(void);

void		Controls_GetConfig( void );
void		Controls_SetConfig( void );
void		Controls_SetDefaults( void );
qboolean	Display_KeyBindPending(void);
qboolean	Display_MouseMove(void *p, int x, int y);
int			Display_VisibleMenuCount(void);
qboolean	Int_Parse(const char **p, int *i);
void		Init_Display(displayContextDef_t *dc);
void		Menus_Activate(menuDef_t *menu);
menuDef_t	*Menus_ActivateByName(const char *p);
qboolean	Menus_AnyFullScreenVisible(void);
void		Menus_CloseAll(void);
int			Menu_Count(void);
itemDef_t	*Menu_FindItemByName(menuDef_t *menu, const char *p);
void Menu_ShowGroup (menuDef_t *menu, const char *itemName, qboolean showFlag);
void Menu_ItemDisable(menuDef_t *menu, const char *name, qboolean disableFlag);
int Menu_ItemsMatchingGroup(menuDef_t *menu, const char *name);
itemDef_t *Menu_GetMatchingItemByNumber(menuDef_t *menu, int index, const char *name);

void		Menu_HandleKey(menuDef_t *menu, int key, qboolean down);
void		Menu_New(char *buffer);
void		Menus_OpenByName(const char *p);
void		Menu_PaintAll(void);
void		Menu_Reset(void);
void		PC_EndParseSession(char *buffer);
qboolean	PC_Float_Parse(int handle, float *f);
qboolean	PC_ParseString(const char **tempStr);
qboolean	PC_ParseStringMem(const char **out);
void		PC_ParseWarning(const char *message);
qboolean	PC_String_Parse(int handle, const char **out);
int			PC_StartParseSession(const char *fileName,char **buffer);
char		*PC_ParseExt(void);
qboolean	PC_ParseInt(int *number);
qboolean	PC_ParseFloat(float *number);
qboolean	PC_ParseColor(vec4_t *c);
const char	*String_Alloc(const char *p);
void		String_Init(void);
qboolean	String_Parse(const char **p, const char **out);
void		String_Report(void);
void		UI_Cursor_Show(qboolean flag);
itemDef_t *Menu_GetMatchingItemByNumber(menuDef_t *menu, int index, const char *name);

extern displayContextDef_t *DC;

#endif
