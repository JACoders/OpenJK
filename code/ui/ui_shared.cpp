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

//
// string allocation/managment

// leave this at the top of all UI_xxxx files for PCH reasons...
//
#include "../server/exe_headers.h"

#include "ui_local.h"

//rww - added for ui ghoul2 models
#define UI_SHARED_CPP

#include "../game/anims.h"
#include "../cgame/animtable.h"

#include "ui_shared.h"
#include "menudef.h"

#include "qcommon/stringed_ingame.h"

void		UI_LoadMenus(const char *menuFile, qboolean reset);

extern vmCvar_t	ui_char_color_red;
extern vmCvar_t	ui_char_color_green;
extern vmCvar_t	ui_char_color_blue;

void *UI_Alloc( int size );

void		Controls_GetConfig( void );
void		Fade(int *flags, float *f, float clamp, int *nextTime, int offsetTime, qboolean bFlags, float fadeAmount);
void		Item_Init(itemDef_t *item);
void		Item_InitControls(itemDef_t *item);
qboolean	Item_Parse(itemDef_t *item);
void		Item_RunScript(itemDef_t *item, const char *s);
void		Item_SetupKeywordHash(void);
void		Item_Text_AutoWrapped_Paint(itemDef_t *item);
void		Item_UpdatePosition(itemDef_t *item);
void		Item_ValidateTypeData(itemDef_t *item);
void		LerpColor(vec4_t a, vec4_t b, vec4_t c, float t);
itemDef_t	*Menu_FindItemByName(menuDef_t *menu, const char *p);
itemDef_t	*Menu_GetMatchingItemByNumber(menuDef_t *menu, int index, const char *name);
void		Menu_Paint(menuDef_t *menu, qboolean forcePaint);
void		Menu_SetupKeywordHash(void);
void		Menus_ShowItems(const char *menuName);
qboolean	ParseRect(const char **p, rectDef_t *r);
const char	*String_Alloc(const char *p);
void		ToWindowCoords(float *x, float *y, windowDef_t *window);
void		Window_Paint(Window *w, float fadeAmount, float fadeClamp, float fadeCycle);
int			Item_ListBox_ThumbDrawPosition(itemDef_t *item);
int			Item_ListBox_ThumbPosition(itemDef_t *item);
int			Item_ListBox_MaxScroll(itemDef_t *item);
static qboolean Rect_ContainsPoint(rectDef_t *rect, float x, float y) ;
static qboolean	Item_Paint(itemDef_t *item, qboolean bDraw);
int Item_TextScroll_ThumbDrawPosition ( itemDef_t *item );
static void Item_TextScroll_BuildLines ( itemDef_t* item );

//static qboolean debugMode = qfalse;
static qboolean g_waitingForKey = qfalse;
static qboolean g_editingField = qfalse;

static itemDef_t *g_bindItem = NULL;
static itemDef_t *g_editItem = NULL;
static itemDef_t *itemCapture = NULL;   // item that has the mouse captured ( if any )

#define DOUBLE_CLICK_DELAY 300
static int lastListBoxClickTime = 0;

static void (*captureFunc) (void *p) = NULL;
static void *captureData = NULL;

//const char defaultString[10] = {"default"};
#ifdef CGAME
#define MEM_POOL_SIZE  128 * 1024
#else
//#define MEM_POOL_SIZE  1024 * 1024
#define MEM_POOL_SIZE  2048 * 1024
#endif

#define SCROLL_TIME_START				500
#define SCROLL_TIME_ADJUST				150
#define SCROLL_TIME_ADJUSTOFFSET		40
#define SCROLL_TIME_FLOOR				20

typedef struct scrollInfo_s {
	int nextScrollTime;
	int nextAdjustTime;
	int adjustValue;
	int scrollKey;
	float xStart;
	float yStart;
	itemDef_t *item;
	qboolean scrollDir;
} scrollInfo_t;

static scrollInfo_t scrollInfo;

static char		memoryPool[MEM_POOL_SIZE];
static int		allocPoint, outOfMemory;

displayContextDef_t *DC = NULL;

menuDef_t Menus[MAX_MENUS];		// defined menus
int menuCount = 0;				// how many

menuDef_t *menuStack[MAX_OPEN_MENUS];
int openMenuCount = 0;

static int strPoolIndex = 0;
static char strPool[STRING_POOL_SIZE];

typedef struct stringDef_s {
	struct stringDef_s *next;
	const char *str;
} stringDef_t;

#define HASH_TABLE_SIZE 2048

static int strHandleCount = 0;
static stringDef_t *strHandle[HASH_TABLE_SIZE];

typedef struct  itemFlagsDef_s {
	const char *string;
	int value;
}	itemFlagsDef_t;

itemFlagsDef_t itemFlags [] = {
	{ "WINDOW_INACTIVE",	WINDOW_INACTIVE },
	{ NULL,					0 }
};

const char *styles [] = {
"WINDOW_STYLE_EMPTY",
"WINDOW_STYLE_FILLED",
"WINDOW_STYLE_GRADIENT",
"WINDOW_STYLE_SHADER",
"WINDOW_STYLE_TEAMCOLOR",
"WINDOW_STYLE_CINEMATIC",
NULL
};

const char *types [] = {
"ITEM_TYPE_TEXT",
"ITEM_TYPE_BUTTON",
"ITEM_TYPE_RADIOBUTTON",
"ITEM_TYPE_CHECKBOX",
"ITEM_TYPE_EDITFIELD",
"ITEM_TYPE_COMBO",
"ITEM_TYPE_LISTBOX",
"ITEM_TYPE_MODEL",
"ITEM_TYPE_OWNERDRAW",
"ITEM_TYPE_NUMERICFIELD",
"ITEM_TYPE_SLIDER",
"ITEM_TYPE_YESNO",
"ITEM_TYPE_MULTI",
"ITEM_TYPE_BIND",
"ITEM_TYPE_TEXTSCROLL",
NULL
};

const char *alignment [] = {
"ITEM_ALIGN_LEFT",
"ITEM_ALIGN_CENTER",
"ITEM_ALIGN_RIGHT",
NULL
};

/*
==================
Init_Display

Initializes the display with a structure to all the drawing routines
 ==================
*/
void Init_Display(displayContextDef_t *dc)
{
	DC = dc;
}

/*
==================
Window_Init

Initializes a window structure ( windowDef_t ) with defaults

==================
*/
void Window_Init(Window *w)
{
	memset(w, 0, sizeof(windowDef_t));
	w->borderSize = 1;
	w->foreColor[0] = w->foreColor[1] = w->foreColor[2] = w->foreColor[3] = 1.0;
	w->cinematic = -1;
}

/*
=================
PC_SourceError
=================
*/
void PC_SourceError(int handle, char *format, ...)
{
	int line;
	char filename[128];
	va_list argptr;
	static char string[4096];

	va_start (argptr, format);
	Q_vsnprintf (string, sizeof(string), format, argptr);
	va_end (argptr);

	filename[0] = '\0';
	line = 0;

	Com_Printf(S_COLOR_RED "ERROR: %s, line %d: %s\n", filename, line, string);
}


/*
=================
PC_ParseStringMem
=================
*/
qboolean PC_ParseStringMem(const char **out)
{
	const char *temp;

	if (PC_ParseString(&temp))
	{
		return qfalse;
	}

	*(out) = String_Alloc(temp);

    return qtrue;
}

/*
=================
PC_ParseRect
=================
*/
qboolean PC_ParseRect(rectDef_t *r)
{
	if (!PC_ParseFloat(&r->x))
	{
		if (!PC_ParseFloat(&r->y))
		{
			if (!PC_ParseFloat(&r->w))
			{
				if (!PC_ParseFloat(&r->h))
				{
					return qtrue;
				}
			}
		}
	}
	return qfalse;
}


/*
=================
PC_Script_Parse
=================
*/
qboolean PC_Script_Parse(const char **out)
{
	char script[4096];
//	pc_token_t token;
	char *token2;

	script[0]=0;
	// scripts start with { and have ; separated command lists.. commands are command, arg..
	// basically we want everything between the { } as it will be interpreted at run time

	token2 = PC_ParseExt();
	if (!token2)
	{
		return qfalse;
	}

	if (*token2 !='{')
	{
	    return qfalse;
	}

	while ( 1 )
	{
		token2 = PC_ParseExt();
		if (!token2)
		{
			return qfalse;
		}

		if (*token2 =='}')	// End of the script?
		{
			*out = String_Alloc(script);
			return qtrue;
		}

		if (*(token2 +1) != '\0')
		{
			Q_strcat(script, sizeof(script), va("\"%s\"", token2));
		}
		else
		{
			Q_strcat(script, sizeof(script), token2);
		}
		Q_strcat(script, sizeof(script), " ");
	}
}

//--------------------------------------------------------------------------------------------
//	Menu Keyword Parse functions
//--------------------------------------------------------------------------------------------

/*
=================
MenuParse_font
=================
*/
qboolean MenuParse_font( itemDef_t *item)
{
	menuDef_t *menu = (menuDef_t*)item;

	if (!PC_ParseStringMem(&menu->font))
	{
		return qfalse;
	}

	if (!DC->Assets.fontRegistered)
	{
		DC->Assets.qhMediumFont = DC->registerFont(menu->font);
		DC->Assets.fontRegistered = qtrue;
	}
	return qtrue;
}


/*
=================
MenuParse_name
=================
*/
qboolean MenuParse_name(itemDef_t *item)
{
	menuDef_t *menu = (menuDef_t*)item;

	if (!PC_ParseStringMem((const char **) &menu->window.name))
	{
		return qfalse;
	}

//	if (Q_stricmp(menu->window.name, "main") == 0)
//	{
		// default main as having focus
//		menu->window.flags |= WINDOW_HASFOCUS;
//	}
	return qtrue;
}

/*
=================
MenuParse_fullscreen
=================
*/

qboolean MenuParse_fullscreen( itemDef_t *item )
{
	menuDef_t *menu = (menuDef_t*)item;

	if (PC_ParseInt((int *) &menu->fullScreen))
	{
		return qfalse;
	}
	return qtrue;
}

/*
=================
MenuParse_rect
=================
*/

qboolean MenuParse_rect( itemDef_t *item)
{
	menuDef_t *menu = (menuDef_t*)item;

	if (!PC_ParseRect(&menu->window.rect))
	{
		return qfalse;
	}
	return qtrue;
}

/*
=================
MenuParse_style
=================
*/
qboolean MenuParse_style( itemDef_t *item)
{
	int		i;
	const char	*tempStr;
	menuDef_t *menu = (menuDef_t*)item;

	if (PC_ParseString(&tempStr))
	{
		return qfalse;
	}

	i=0;
	while (styles[i])
	{
		if (Q_stricmp(tempStr,styles[i])==0)
		{
			menu->window.style = i;
			break;
		}
		i++;
	}

	if (styles[i] == NULL)
	{
		PC_ParseWarning(va("Unknown menu style value '%s'",tempStr));
	}
	return qtrue;
}


/*
=================
MenuParse_visible
=================
*/
qboolean MenuParse_visible( itemDef_t *item )
{
	int i;
	menuDef_t *menu = (menuDef_t*)item;

	if (PC_ParseInt(&i))
	{
		return qfalse;
	}

	if (i)
	{
		menu->window.flags |= WINDOW_VISIBLE;
	}
	return qtrue;
}

/*
=================
MenuParse_ignoreescape
=================
*/
qboolean MenuParse_ignoreescape( itemDef_t *item )
{
	int i;
	menuDef_t *menu = (menuDef_t*)item;

	if (PC_ParseInt(&i))
	{
		return qfalse;
	}

	if (i)
	{
		menu->window.flags |= WINDOW_IGNORE_ESCAPE;
	}
	return qtrue;
}

/*
=================
MenuParse_onOpen
=================
*/
qboolean MenuParse_onOpen( itemDef_t *item)
{
	menuDef_t *menu = (menuDef_t*)item;

	if (!PC_Script_Parse(&menu->onOpen))
	{
		return qfalse;
	}
	return qtrue;
}

/*
=================
MenuParse_onClose
=================
*/
qboolean MenuParse_onClose( itemDef_t *item )
{
	menuDef_t *menu = (menuDef_t*)item;

	if (!PC_Script_Parse(&menu->onClose))
	{
		return qfalse;
	}
	return qtrue;
}

//JLFACCEPT MPMOVED
/*
=================
MenuParse_onAccept
=================
*/
qboolean MenuParse_onAccept( itemDef_t *item )
{
	menuDef_t *menu = (menuDef_t*)item;

	if (!PC_Script_Parse(&menu->onAccept))
	{
		return qfalse;
	}
	return qtrue;
}


/*
=================
MenuParse_onESC
=================
*/
qboolean MenuParse_onESC( itemDef_t *item )
{
	menuDef_t *menu = (menuDef_t*)item;

	if (!PC_Script_Parse(&menu->onESC))
	{
		return qfalse;
	}
	return qtrue;
}


/*
=================
MenuParse_border
=================
*/
qboolean MenuParse_border( itemDef_t *item)
{
	menuDef_t *menu = (menuDef_t*)item;

	if (PC_ParseInt(&menu->window.border))
	{
		return qfalse;
	}
	return qtrue;
}

/*
=================
MenuParse_borderSize
=================
*/
qboolean MenuParse_borderSize( itemDef_t *item)
{
	menuDef_t *menu = (menuDef_t*)item;

	if (PC_ParseFloat(&menu->window.borderSize))
	{
		return qfalse;
	}
	return qtrue;
}

/*
=================
MenuParse_backcolor
=================
*/
qboolean MenuParse_backcolor( itemDef_t *item )
{
	int i;
	float f;
	menuDef_t *menu = (menuDef_t*)item;

	for (i = 0; i < 4; i++)
	{
		if (PC_ParseFloat(&f))
		{
			return qfalse;
		}
		menu->window.backColor[i]  = f;
	}
	return qtrue;
}

/*
=================
MenuParse_forecolor
=================
*/
qboolean MenuParse_forecolor( itemDef_t *item)
{
	int i;
	float f;
	menuDef_t *menu = (menuDef_t*)item;

	for (i = 0; i < 4; i++)
	{
		if (PC_ParseFloat(&f))
		{
			return qfalse;
		}
		if (f < 0)
		{	//special case for player color
			menu->window.flags |= WINDOW_PLAYERCOLOR;
			return qtrue;
		}
		menu->window.foreColor[i]  = f;
		menu->window.flags |= WINDOW_FORECOLORSET;
	}
	return qtrue;
}

/*
=================
MenuParse_bordercolor
=================
*/
qboolean MenuParse_bordercolor( itemDef_t *item )
{
	int i;
	float f;
	menuDef_t *menu = (menuDef_t*)item;

	for (i = 0; i < 4; i++)
	{
		if (PC_ParseFloat(&f))
		{
			return qfalse;
		}
		menu->window.borderColor[i]  = f;
	}
	return qtrue;
}

/*
=================
MenuParse_focuscolor
=================
*/
qboolean MenuParse_focuscolor( itemDef_t *item)
{
	int i;
	float f;
	menuDef_t *menu = (menuDef_t*)item;

	for (i = 0; i < 4; i++)
	{
		if (PC_ParseFloat(&f))
		{
			return qfalse;
		}
		menu->focusColor[i]  = f;
	}
	return qtrue;
}


/*
=================
MenuParse_focuscolor
=================
*/
qboolean MenuParse_appearanceIncrement( itemDef_t *item)
{
	menuDef_t *menu = (menuDef_t*)item;

	if (PC_ParseFloat(&menu->appearanceIncrement))
	{
		return qfalse;
	}
	return qtrue;
}



/*
=================
MenuParse_descAlignment
=================
*/
qboolean MenuParse_descAlignment( itemDef_t *item)
{
	menuDef_t *menu = (menuDef_t*)item;
	const char *tempStr;
	int	i;

	if (PC_ParseString(&tempStr))
	{
		return qfalse;
	}

	i=0;
	while (alignment[i])
	{
		if (Q_stricmp(tempStr,alignment[i])==0)
		{
			menu->descAlignment = i;
			break;
		}
		i++;
	}

	if (alignment[i] == NULL)
	{
		PC_ParseWarning(va("Unknown desc alignment value '%s'",tempStr));
	}

	return qtrue;
}

/*
=================
MenuParse_descTextStyle
=================
*/
qboolean MenuParse_descTextStyle( itemDef_t *item)
{
	menuDef_t *menu = (menuDef_t*)item;

	if (PC_ParseInt(&menu->descTextStyle))
	{
		return qfalse;
	}
	return qtrue;
}
/*
=================
MenuParse_descX
=================
*/
qboolean MenuParse_descX( itemDef_t *item)
{
	menuDef_t *menu = (menuDef_t*)item;

	if (PC_ParseInt(&menu->descX))
	{
		return qfalse;
	}
	return qtrue;
}

/*
=================
MenuParse_descY
=================
*/
qboolean MenuParse_descY( itemDef_t *item)
{
	menuDef_t *menu = (menuDef_t*)item;

	if (PC_ParseInt(&menu->descY))
	{
		return qfalse;
	}
	return qtrue;
}


/*
=================
MenuParse_descScale
=================
*/
qboolean MenuParse_descScale( itemDef_t *item)
{
	menuDef_t *menu = (menuDef_t*)item;

	if (PC_ParseFloat(&menu->descScale))
	{
		return qfalse;
	}
	return qtrue;
}

/*
=================
MenuParse_descColor
=================
*/
qboolean MenuParse_descColor( itemDef_t *item)
{
	int i;
	float f;
	menuDef_t *menu = (menuDef_t*)item;

	for (i = 0; i < 4; i++)
	{
		if (PC_ParseFloat(&f))
		{
			return qfalse;
		}
		menu->descColor[i]  = f;
	}
	return qtrue;
}

/*
=================
MenuParse_disablecolor
=================
*/
qboolean MenuParse_disablecolor( itemDef_t *item)
{
	int i;
	float f;
	menuDef_t *menu = (menuDef_t*)item;

	for (i = 0; i < 4; i++)
	{
		if (PC_ParseFloat(&f))
		{
			return qfalse;
		}
		menu->disableColor[i]  = f;
	}
	return qtrue;
}


/*
=================
MenuParse_outlinecolor
=================
*/
qboolean MenuParse_outlinecolor( itemDef_t *item)
{
	menuDef_t *menu = (menuDef_t*)item;

	if (PC_ParseColor(&menu->window.outlineColor))
	{
		return qfalse;
	}
	return qtrue;
}

/*
=================
MenuParse_background
=================
*/
qboolean MenuParse_background( itemDef_t *item)
{
	const char *buff;
	menuDef_t *menu = (menuDef_t*)item;

	if (PC_ParseString(&buff))
	{
		return qfalse;
	}

	menu->window.background = ui.R_RegisterShaderNoMip(buff);
	return qtrue;
}

/*
=================
MenuParse_cinematic
=================
*/
qboolean MenuParse_cinematic( itemDef_t *item)
{
	menuDef_t *menu = (menuDef_t*)item;

	if (!PC_ParseStringMem((const char **) &menu->window.cinematicName))
	{
		return qfalse;
	}
	return qtrue;
}

/*
=================
MenuParse_ownerdrawFlag
=================
*/
qboolean MenuParse_ownerdrawFlag( itemDef_t *item)
{
	int i;
	menuDef_t *menu = (menuDef_t*)item;

	if (PC_ParseInt(&i))
	{
		return qfalse;
	}
	menu->window.ownerDrawFlags |= i;
	return qtrue;
}

/*
=================
MenuParse_ownerdraw
=================
*/
qboolean MenuParse_ownerdraw( itemDef_t *item)
{
	menuDef_t *menu = (menuDef_t*)item;

	if (PC_ParseInt(&menu->window.ownerDraw))
	{
		return qfalse;
	}
	return qtrue;
}


/*
=================
MenuParse_popup
=================
*/
qboolean MenuParse_popup( itemDef_t *item)
{
	menuDef_t *menu = (menuDef_t*)item;
	menu->window.flags |= WINDOW_POPUP;
	return qtrue;
}


/*
=================
MenuParse_outOfBounds
=================
*/
qboolean MenuParse_outOfBounds( itemDef_t *item)
{
	menuDef_t *menu = (menuDef_t*)item;

	menu->window.flags |= WINDOW_OOB_CLICK;
	return qtrue;
}

/*
=================
MenuParse_soundLoop
=================
*/
qboolean MenuParse_soundLoop( itemDef_t *item)
{
	menuDef_t *menu = (menuDef_t*)item;

	if (!PC_ParseStringMem((const char **) &menu->soundName))
	{
		return qfalse;
	}
	return qtrue;
}

/*
================
MenuParse_fadeClamp
================
*/
qboolean MenuParse_fadeClamp( itemDef_t *item)
{
	menuDef_t *menu = (menuDef_t*)item;

	if (PC_ParseFloat(&menu->fadeClamp))
	{
		return qfalse;
	}
	return qtrue;
}

/*
================
MenuParse_fadeAmount
================
*/
qboolean MenuParse_fadeAmount( itemDef_t *item)
{
	menuDef_t *menu = (menuDef_t*)item;

	if (PC_ParseFloat(&menu->fadeAmount))
	{
		return qfalse;
	}
	return qtrue;
}


/*
================
MenuParse_fadeCycle
================
*/
qboolean MenuParse_fadeCycle( itemDef_t *item)
{
	menuDef_t *menu = (menuDef_t*)item;

	if (PC_ParseInt(&menu->fadeCycle))
	{
		return qfalse;
	}
	return qtrue;
}


/*
================
MenuParse_itemDef
================
*/
qboolean MenuParse_itemDef( itemDef_t *item)
{
	menuDef_t *menu = (menuDef_t*)item;
	if (menu->itemCount < MAX_MENUITEMS)
	{
		menu->items[menu->itemCount] = (struct itemDef_s *) UI_Alloc(sizeof(itemDef_t));
		Item_Init(menu->items[menu->itemCount]);
		if (!Item_Parse(menu->items[menu->itemCount]))
		{
			return qfalse;
		}
		Item_InitControls(menu->items[menu->itemCount]);
		menu->items[menu->itemCount++]->parent = menu;
	}
	else
	{
		PC_ParseWarning(va("Exceeded item/menu max of %d", MAX_MENUITEMS));
	}
	return qtrue;
}

#define KEYWORDHASH_SIZE	512

typedef struct keywordHash_s
{
	const char	*keyword;
	qboolean	(*func)(itemDef_t *item);
	struct		keywordHash_s *next;
} keywordHash_t;

keywordHash_t menuParseKeywords[] = {
	{"appearanceIncrement",	MenuParse_appearanceIncrement},
	{"backcolor",			MenuParse_backcolor,	},
	{"background",			MenuParse_background,	},
	{"border",				MenuParse_border,		},
	{"bordercolor",			MenuParse_bordercolor,	},
	{"borderSize",			MenuParse_borderSize,	},
	{"cinematic",			MenuParse_cinematic,	},
	{"descAlignment",		MenuParse_descAlignment	},
	{"descTextStyle",		MenuParse_descTextStyle	},
	{"desccolor",			MenuParse_descColor		},
	{"descX",				MenuParse_descX			},
	{"descY",				MenuParse_descY			},
	{"descScale",			MenuParse_descScale		},
	{"disablecolor",		MenuParse_disablecolor,	},
	{"fadeClamp",			MenuParse_fadeClamp,	},
	{"fadeCycle",			MenuParse_fadeCycle,	},
	{"fadeAmount",			MenuParse_fadeAmount,	},
	{"focuscolor",			MenuParse_focuscolor,	},
	{"font",				MenuParse_font,			},
	{"forecolor",			MenuParse_forecolor,	},
	{"fullscreen",			MenuParse_fullscreen,	},
	{"itemDef",				MenuParse_itemDef,		},
	{"name",				MenuParse_name,			},
	{"onClose",				MenuParse_onClose,		},
//JLFACCEPT MPMOVED
	{"onAccept",			MenuParse_onAccept,		},
	{"onESC",				MenuParse_onESC,		},
	{"onOpen",				MenuParse_onOpen,		},
	{"outlinecolor",		MenuParse_outlinecolor,	},
	{"outOfBoundsClick",	MenuParse_outOfBounds,	},
	{"ownerdraw",			MenuParse_ownerdraw,	},
	{"ownerdrawFlag",		MenuParse_ownerdrawFlag,},
	{"popup",				MenuParse_popup,		},
	{"rect",				MenuParse_rect,			},
	{"soundLoop",			MenuParse_soundLoop,	},
	{"style",				MenuParse_style,		},
	{"visible",				MenuParse_visible,		},
	{"ignoreescape",		MenuParse_ignoreescape,	},
	{NULL,					NULL,					}
};

keywordHash_t *menuParseKeywordHash[KEYWORDHASH_SIZE];

/*
================
KeywordHash_Key
================
*/
int KeywordHash_Key(const char *keyword)
{
	int register hash, i;

	hash = 0;
	for (i = 0; keyword[i] != '\0'; i++) {
		if (keyword[i] >= 'A' && keyword[i] <= 'Z')
			hash += (keyword[i] + ('a' - 'A')) * (119 + i);
		else
			hash += keyword[i] * (119 + i);
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20)) & (KEYWORDHASH_SIZE-1);
	return hash;
}

/*
================
KeywordHash_Add
================
*/
void KeywordHash_Add(keywordHash_t *table[], keywordHash_t *key)
{
	int hash;

	hash = KeywordHash_Key(key->keyword);
	key->next = table[hash];
	table[hash] = key;
}

/*
===============
KeywordHash_Find
===============
*/
keywordHash_t *KeywordHash_Find(keywordHash_t *table[], const char *keyword)
{
	keywordHash_t *key;
	int hash;

	hash = KeywordHash_Key(keyword);
	for (key = table[hash]; key; key = key->next)
	{
		if (!Q_stricmp(key->keyword, keyword))
			return key;
	}
	return NULL;
}

/*
================
hashForString

return a hash value for the string
================
*/
static unsigned hashForString(const char *str)
{
	int		i;
	unsigned	hash;
	char	letter;

	hash = 0;
	i = 0;
	while (str[i] != '\0')
	{
		letter = tolower((unsigned char)str[i]);
		hash += (unsigned)(letter)*(i + 119);
		i++;
	}
	hash &= (HASH_TABLE_SIZE-1);
	return hash;
}

/*
=================
String_Alloc
=================
*/
const char *String_Alloc(const char *p)
{
	int len;
	unsigned hash;
	stringDef_t *str, *last;
	static const char *staticNULL = "";

	if (p == NULL)
	{
		return NULL;
	}

	if (*p == 0)
	{
		return staticNULL;
	}

	hash = hashForString(p);

	str = strHandle[hash];
	while (str)
	{
		if (strcmp(p, str->str) == 0)
		{
			return str->str;
		}
		str = str->next;
	}

	len = strlen(p);
	if (len + strPoolIndex + 1 < STRING_POOL_SIZE)
	{
		int ph = strPoolIndex;
		strcpy(&strPool[strPoolIndex], p);
		strPoolIndex += len + 1;

		str = strHandle[hash];
		last = str;
		while (last && last->next)
		{
			last = last->next;
		}

		str  = (stringDef_s *) UI_Alloc( sizeof(stringDef_t));
		str->next = NULL;
		str->str = &strPool[ph];
		if (last)
		{
			last->next = str;
		}
		else
		{
			strHandle[hash] = str;
		}

		return &strPool[ph];
	}
	else
	{
		Com_Printf("WARNING: Ran out of strPool space\n");
	}

	return NULL;
}

/*
=================
String_Report
=================
*/
void String_Report(void)
{
	float f;
	Com_Printf("Memory/String Pool Info\n");
	Com_Printf("----------------\n");
	f = strPoolIndex;
	f /= STRING_POOL_SIZE;
	f *= 100;
	Com_Printf("String Pool is %.1f%% full, %i bytes out of %i used.\n", f, strPoolIndex, STRING_POOL_SIZE);
	f = allocPoint;
	f /= MEM_POOL_SIZE;
	f *= 100;
	Com_Printf("Memory Pool is %.1f%% full, %i bytes out of %i used.\n", f, allocPoint, MEM_POOL_SIZE);
}

/*
=================
String_Init
=================
*/
void String_Init(void)
{
	int i;

	for (i = 0; i < HASH_TABLE_SIZE; i++)
	{
		strHandle[i] = 0;
	}

	strHandleCount = 0;
	strPoolIndex = 0;
	UI_InitMemory();
	Item_SetupKeywordHash();
	Menu_SetupKeywordHash();

	if (DC && DC->getBindingBuf)
	{
		Controls_GetConfig();
	}
}



//---------------------------------------------------------------------------------------------------------
//		Memory
//---------------------------------------------------------------------------------------------------------

/*
===============
UI_Alloc
===============
*/
void *UI_Alloc( int size )
{
	char	*p;

	if ( allocPoint + size > MEM_POOL_SIZE )
	{
		outOfMemory = qtrue;
		if (DC->Print)
		{
			DC->Print("UI_Alloc: Failure. Out of memory!\n");
		}
		return NULL;
	}

	p = &memoryPool[allocPoint];

	allocPoint += ( size + 15 ) & ~15;

	return p;
}

/*
===============
UI_InitMemory
===============
*/
void UI_InitMemory( void )
{
	allocPoint = 0;
	outOfMemory = qfalse;
}


/*
===============
Menu_ItemsMatchingGroup
===============
*/
int Menu_ItemsMatchingGroup(menuDef_t *menu, const char *name)
{
	int i;
	int count = 0;

	for (i = 0; i < menu->itemCount; i++)
	{
		if ((!menu->items[i]->window.name) && (!menu->items[i]->window.group))
		{
			Com_Printf(S_COLOR_YELLOW"WARNING: item has neither name or group\n");
			continue;
		}

		if (Q_stricmp(menu->items[i]->window.name, name) == 0 || (menu->items[i]->window.group && Q_stricmp(menu->items[i]->window.group, name) == 0))
		{
			count++;
		}
	}

	return count;
}

/*
===============
Menu_GetMatchingItemByNumber
===============
*/
itemDef_t *Menu_GetMatchingItemByNumber(menuDef_t *menu, int index, const char *name)
{
	int i;
	int count = 0;
	for (i = 0; i < menu->itemCount; i++)
	{
		if (Q_stricmp(menu->items[i]->window.name, name) == 0 || (menu->items[i]->window.group && Q_stricmp(menu->items[i]->window.group, name) == 0))
		{
			if (count == index)
			{
				return menu->items[i];
			}
			count++;
		}
	}
	return NULL;
}

/*
===============
Menu_FadeItemByName
===============
*/
void Menu_FadeItemByName(menuDef_t *menu, const char *p, qboolean fadeOut)
{
	itemDef_t *item;
	int i;
	int count = Menu_ItemsMatchingGroup(menu, p);
	for (i = 0; i < count; i++)
	{
		item = Menu_GetMatchingItemByNumber(menu, i, p);
		if (item != NULL)
		{
			if (fadeOut)
			{
				item->window.flags |= (WINDOW_FADINGOUT | WINDOW_VISIBLE);
				item->window.flags &= ~WINDOW_FADINGIN;
			}
			else
			{
				item->window.flags |= (WINDOW_VISIBLE | WINDOW_FADINGIN);
				item->window.flags &= ~WINDOW_FADINGOUT;
			}
		}
	}
}

/*
===============
Menu_ShowItemByName
===============
*/
void Menu_ShowItemByName(menuDef_t *menu, const char *p, qboolean bShow)
{
	itemDef_t *item;
	int i;
	int count;

	count = Menu_ItemsMatchingGroup(menu, p);

	if (!count)
	{
		Com_Printf(S_COLOR_YELLOW"WARNING: Menu_ShowItemByName - unable to locate any items named: \"%s\"\n",p);
	}

	for (i = 0; i < count; i++)
	{
		item = Menu_GetMatchingItemByNumber(menu, i, p);
		if (item != NULL)
		{
			if (bShow)
			{
				item->window.flags |= WINDOW_VISIBLE;
			}
			else
			{
				item->window.flags &= ~(WINDOW_VISIBLE | WINDOW_HASFOCUS);
				// stop cinematics playing in the window
				if (item->window.cinematic >= 0)
				{
					DC->stopCinematic(item->window.cinematic);
					item->window.cinematic = -1;
				}
			}
		}
	}
}

/*
===============
Menu_GetFocused
===============
*/
menuDef_t *Menu_GetFocused(void)
{
	int i;

	for (i = 0; i < menuCount; i++)
	{
		if ((Menus[i].window.flags & WINDOW_HASFOCUS) && (Menus[i].window.flags & WINDOW_VISIBLE))
		{
	      return &Menus[i];
		}
	}

	return NULL;
}

/*
===============
Menus_OpenByName
===============
*/
void Menus_OpenByName(const char *p)
{
	Menus_ActivateByName(p);
}

/*
===============
Menus_FindByName
===============
*/
menuDef_t *Menus_FindByName(const char *p)
{
	int i;
	for (i = 0; i < menuCount; i++)
	{
		if (Q_stricmp(Menus[i].window.name, p) == 0)
		{
			return &Menus[i];
		}
	}
	return NULL;
}

/*
===============
Menu_RunCloseScript
===============
*/
static void Menu_RunCloseScript(menuDef_t *menu)
{
	if (menu && menu->window.flags & WINDOW_VISIBLE && menu->onClose)
	{
		itemDef_t item;
		item.parent = menu;
		Item_RunScript(&item, menu->onClose);
	}
}

/*
===============
Item_ActivateByName
===============
*/
void Item_ActivateByName(const char *menuName,const char *itemName)
{
	itemDef_t *item;
	menuDef_t *menu;

	menu = Menus_FindByName(menuName);

	item = (itemDef_s *) Menu_FindItemByName((menuDef_t *) menu, itemName);

	if (item != NULL)
	{
		item->window.flags &= ~WINDOW_INACTIVE;
	}
}

/*
===============
Menus_CloseByName
===============
*/
void Menus_CloseByName(const char *p)
{
	menuDef_t *menu = Menus_FindByName(p);

	// If the menu wasnt found just exit
	if (menu == NULL)
	{
		return;
	}

	// Run the close script for the menu
	Menu_RunCloseScript(menu);

	// If this window had the focus then take it away
	if ( menu->window.flags & WINDOW_HASFOCUS )
	{
		// If there is something still in the open menu list then
		// set it to have focus now
		if ( openMenuCount )
		{
			// Subtract one from the open menu count to prepare to
			// remove the top menu from the list
			openMenuCount -= 1;

			// Set the top menu to have focus now
			menuStack[openMenuCount]->window.flags |= WINDOW_HASFOCUS;

			// Remove the top menu from the list
			menuStack[openMenuCount] = NULL;
		}
	}

	// Window is now invisible and doenst have focus
	menu->window.flags &= ~(WINDOW_VISIBLE | WINDOW_HASFOCUS);
}

/*
===============
Menu_FindItemByName
===============
*/
itemDef_t *Menu_FindItemByName(menuDef_t *menu, const char *p)
{
	int i;
	if (menu == NULL || p == NULL)
	{
		return NULL;
	}

	for (i = 0; i < menu->itemCount; i++)
	{
		if (Q_stricmp(p, menu->items[i]->window.name) == 0)
		{
			return menu->items[i];
		}
	}

	return NULL;
}

/*
=================
Menu_ClearFocus
=================
*/
itemDef_t *Menu_ClearFocus(menuDef_t *menu)
{
	int i;
	itemDef_t *ret = NULL;

	if (menu == NULL)
	{
		return NULL;
	}
	for (i = 0; i < menu->itemCount; i++)
	{
		if (menu->items[i]->window.flags & WINDOW_HASFOCUS)
		{
			ret = menu->items[i];
			menu->items[i]->window.flags &= ~WINDOW_HASFOCUS;
			if (menu->items[i]->leaveFocus)
			{
				Item_RunScript(menu->items[i], menu->items[i]->leaveFocus);
			}
		}
	}
	return ret;
}

// Set all the items within a given menu, with the given itemName, to the given shader
void Menu_SetItemBackground(const menuDef_t *menu,const char *itemName, const char *background)
{
	itemDef_t	*item;
	int			j, count;

	if (!menu)	// No menu???
	{
		return;
	}

	count = Menu_ItemsMatchingGroup( (menuDef_t *) menu, itemName);

	for (j = 0; j < count; j++)
	{
		item = Menu_GetMatchingItemByNumber( (menuDef_t *) menu, j, itemName);
		if (item != NULL)
		{
//			item->window.background = DC->registerShaderNoMip(background);
			item->window.background = ui.R_RegisterShaderNoMip(background);
		}
	}
}

// Set all the items within a given menu, with the given itemName, to the given text
void Menu_SetItemText(const menuDef_t *menu,const char *itemName, const char *text)
{
	itemDef_t	*item;
	int			j, count;

	if (!menu)	// No menu???
	{
		return;
	}

	count = Menu_ItemsMatchingGroup( (menuDef_t *) menu, itemName);

	for (j = 0; j < count; j++)
	{
		item = Menu_GetMatchingItemByNumber( (menuDef_t *) menu, j, itemName);
		if (item != NULL)
		{
			if (text[0] == '*')
			{
				item->cvar = text+1;
				// Just copying what was in ItemParse_cvar()
				if ( item->typeData)
				{
					editFieldDef_t *editPtr;
					editPtr = (editFieldDef_t*)item->typeData;
					editPtr->minVal = -1;
					editPtr->maxVal = -1;
					editPtr->defVal = -1;
				}
			}
			else
			{
				if (item->type == ITEM_TYPE_TEXTSCROLL )
				{
					char cvartext[1024];
					textScrollDef_t *scrollPtr = (textScrollDef_t*)item->typeData;
					if ( scrollPtr )
					{
						scrollPtr->startPos = 0;
						scrollPtr->endPos = 0;
					}

					if (item->cvar)
					{
						DC->getCVarString(item->cvar, cvartext, sizeof(cvartext));
						item->text = cvartext;
					}
					else
					{
						item->text = (char *) text;
					}

					Item_TextScroll_BuildLines ( item );
				}
				else
				{
					item->text = (char *) text;
				}
			}
		}
	}
}

/*
=================
Menu_TransitionItemByName
=================
*/
void Menu_TransitionItemByName(menuDef_t *menu, const char *p, const rectDef_t *rectFrom, const rectDef_t *rectTo, int time, float amt)
{
	itemDef_t *item;
	int i;
	int count = Menu_ItemsMatchingGroup(menu, p);
	for (i = 0; i < count; i++)
	{
		item = Menu_GetMatchingItemByNumber(menu, i, p);
		if (item != NULL)
		{
			if (!rectFrom)
			{
				rectFrom = &item->window.rect;	//if there are more than one of these with the same name, they'll all use the FIRST one's FROM.
			}
			item->window.flags |= (WINDOW_INTRANSITION | WINDOW_VISIBLE);
			item->window.offsetTime = time;
			memcpy(&item->window.rectClient, rectFrom, sizeof(rectDef_t));
			memcpy(&item->window.rectEffects, rectTo, sizeof(rectDef_t));
			item->window.rectEffects2.x = abs(rectTo->x - rectFrom->x) / amt;
			item->window.rectEffects2.y = abs(rectTo->y - rectFrom->y) / amt;
			item->window.rectEffects2.w = abs(rectTo->w - rectFrom->w) / amt;
			item->window.rectEffects2.h = abs(rectTo->h - rectFrom->h) / amt;
			Item_UpdatePosition(item);
		}
	}
}

/*
=================
Menu_TransitionItemByName
=================
*/
//JLF MOVED
#define _TRANS3
#ifdef _TRANS3
void Menu_Transition3ItemByName(menuDef_t *menu, const char *p, const float minx, const float miny, const float minz,
								const float maxx, const float maxy, const float maxz, const float fovtx, const float fovty,
								const int time, const float amt)
{
	itemDef_t *item;
	int i;
	int count = Menu_ItemsMatchingGroup(menu, p);
	modelDef_t  * modelptr;
	for (i = 0; i < count; i++)
	{
		item = Menu_GetMatchingItemByNumber(menu, i, p);
		if (item != NULL)
		{
			if ( item->type == ITEM_TYPE_MODEL)
			{
				modelptr = (modelDef_t*)item->typeData;

				item->window.flags |= (WINDOW_INTRANSITIONMODEL | WINDOW_VISIBLE);
				item->window.offsetTime = time;
				modelptr->fov_x2 = fovtx;
				modelptr->fov_y2 = fovty;
				VectorSet(modelptr->g2maxs2, maxx, maxy, maxz);
				VectorSet(modelptr->g2mins2, minx, miny, minz);

//				//modelptr->g2maxs2.x= maxx;
//				modelptr->g2maxs2.y= maxy;
//				modelptr->g2maxs2.z= maxz;
//				modelptr->g2mins2.x= minx;
//				modelptr->g2mins2.y= miny;
//				modelptr->g2mins2.z= minz;

//				VectorSet(modelptr->g2maxs2, maxx, maxy, maxz);

				modelptr->g2maxsEffect[0] = abs(modelptr->g2maxs2[0] - modelptr->g2maxs[0]) / amt;
				modelptr->g2maxsEffect[1] = abs(modelptr->g2maxs2[1] - modelptr->g2maxs[1]) / amt;
				modelptr->g2maxsEffect[2] = abs(modelptr->g2maxs2[2] - modelptr->g2maxs[2]) / amt;

				modelptr->g2minsEffect[0] = abs(modelptr->g2mins2[0] - modelptr->g2mins[0]) / amt;
				modelptr->g2minsEffect[1] = abs(modelptr->g2mins2[1] - modelptr->g2mins[1]) / amt;
				modelptr->g2minsEffect[2] = abs(modelptr->g2mins2[2] - modelptr->g2mins[2]) / amt;


				modelptr->fov_Effectx = abs(modelptr->fov_x2 - modelptr->fov_x) / amt;
				modelptr->fov_Effecty = abs(modelptr->fov_y2 - modelptr->fov_y) / amt;
			}

		}
	}
}

#endif




/*
=================
Menu_OrbitItemByName
=================
*/
void Menu_OrbitItemByName(menuDef_t *menu, const char *p, float x, float y, float cx, float cy, int time)
{
	itemDef_t *item;
	int i;
	int count = Menu_ItemsMatchingGroup(menu, p);
	for (i = 0; i < count; i++)
	{
		item = Menu_GetMatchingItemByNumber(menu, i, p);
		if (item != NULL)
		{
			item->window.flags |= (WINDOW_ORBITING | WINDOW_VISIBLE);
			item->window.offsetTime = time;
			item->window.rectEffects.x = cx;
			item->window.rectEffects.y = cy;
			item->window.rectClient.x = x;
			item->window.rectClient.y = y;
			Item_UpdatePosition(item);
		}
	}
}

/*
=================
Rect_Parse
=================
*/
qboolean Rect_Parse(const char **p, rectDef_t *r)
{
	if (!COM_ParseFloat(p, &r->x))
	{
		if (!COM_ParseFloat(p, &r->y))
		{
			if (!COM_ParseFloat(p, &r->w))
			{
				if (!COM_ParseFloat(p, &r->h))
				{
					return qtrue;
				}
			}
		}
	}
	return qfalse;
}

qboolean Script_SetItemRect(itemDef_t *item, const char **args)
{
	const char *itemname;
	rectDef_t *out;
	rectDef_t rect;

	// expecting type of color to set and 4 args for the color
	if (String_Parse(args, &itemname))
	{
		itemDef_t *item2;
		int j;
		int count = Menu_ItemsMatchingGroup((menuDef_t *) item->parent, itemname);

		if (!Rect_Parse(args, &rect))
		{
			return qtrue;
		}

		for (j = 0; j < count; j++)
		{
			item2 = Menu_GetMatchingItemByNumber((menuDef_t *) item->parent, j, itemname);
			if (item2 != NULL)
			{
				out = &item2->window.rect;

				if (out)
				{
					item2->window.rect.x = rect.x;
					item2->window.rect.y = rect.y;
					item2->window.rect.w = rect.w;
					item2->window.rect.h = rect.h;
				}
			}
		}
	}
	return qtrue;
}

/*
=================
Script_SetItemBackground
=================
*/
qboolean Script_SetItemBackground(itemDef_t *item, const char **args)
{
	const char *itemName;
	const char *name;

	// expecting name of shader
	if (String_Parse(args, &itemName) && String_Parse(args, &name))
	{
		Menu_SetItemBackground((menuDef_t *) item->parent, itemName, name);
	}
	return qtrue;
}

/*
=================
Script_SetItemText
=================
*/
qboolean Script_SetItemText(itemDef_t *item, const char **args)
{
	const char *itemName;
	const char *text;

	// expecting text
	if (String_Parse(args, &itemName) && String_Parse(args, &text))
	{
		Menu_SetItemText((menuDef_t *) item->parent, itemName, text);
	}
	return qtrue;
}

/*
=================
Script_FadeIn
=================
*/
qboolean Script_FadeIn(itemDef_t *item, const char **args)
{
	const char *name;
	if (String_Parse(args, &name))
	{
		Menu_FadeItemByName((menuDef_t *) item->parent, name, qfalse);
	}

	return qtrue;
}

/*
=================
Script_FadeOut
=================
*/
qboolean Script_FadeOut(itemDef_t *item, const char **args)
{
	const char *name;
	if (String_Parse(args, &name))
	{
		Menu_FadeItemByName((menuDef_t *) item->parent, name, qtrue);
	}

	return qtrue;
}

/*
=================
Script_Show
=================
*/
qboolean Script_Show(itemDef_t *item, const char **args)
{
	const char *name;
	if (String_Parse(args, &name))
	{
		Menu_ShowItemByName((menuDef_t *) item->parent, name, qtrue);
	}

	return qtrue;
}



/*
=================
Script_ShowMenu
=================
*/
qboolean Script_ShowMenu(itemDef_t *item, const char **args)
{
	const char *name;
	if (String_Parse(args, &name))
	{
		Menus_ShowItems(name);
	}

	return qtrue;
}


/*
=================
Script_Hide
=================
*/
qboolean Script_Hide(itemDef_t *item, const char **args)
{
	const char *name;
	if (String_Parse(args, &name))
	{
		Menu_ShowItemByName((menuDef_t *) item->parent, name, qfalse);
	}

	return qtrue;
}

/*
=================
Script_SetColor
=================
*/
qboolean Script_SetColor(itemDef_t *item, const char **args)
{
	const char *name;
	int i;
	float f;
	vec4_t *out;

	// expecting type of color to set and 4 args for the color
	if (String_Parse(args, &name))
	{
		out = NULL;
		if (Q_stricmp(name, "backcolor") == 0)
		{
			out = &item->window.backColor;
			item->window.flags |= WINDOW_BACKCOLORSET;
		}
		else if (Q_stricmp(name, "forecolor") == 0)
		{
			out = &item->window.foreColor;
			item->window.flags |= WINDOW_FORECOLORSET;
		}
		else if (Q_stricmp(name, "bordercolor") == 0)
		{
			out = &item->window.borderColor;
		}

		if (out)
		{
			for (i = 0; i < 4; i++)
			{
//				if (!Float_Parse(args, &f))
				if (COM_ParseFloat( args, &f))
				{
					return qtrue;
				}
				(*out)[i] = f;
			}
		}
	}

	return qtrue;
}

/*
=================
Script_Open
=================
*/
qboolean Script_Open(itemDef_t *item, const char **args)
{
	const char *name;
	if (String_Parse(args, &name))
	{
		Menus_OpenByName(name);
	}

	return qtrue;
}

qboolean Script_OpenGoToMenu(itemDef_t *item, const char **args)
{
	Menus_OpenByName(GoToMenu);				// Give warning
	return qtrue;
}


/*
=================
Script_Close
=================
*/
qboolean Script_Close(itemDef_t *item, const char **args)
{
	const char *name;
	if (String_Parse(args, &name))
	{
		if (Q_stricmp(name, "all") == 0)
		{
			Menus_CloseAll();
		}
		else
		{
			Menus_CloseByName(name);
		}
	}

	return qtrue;
}

/*
=================
Script_Activate
=================
*/
qboolean Script_Activate(itemDef_t *item, const char **args)
{
	const char *name, *menu;

	if (String_Parse(args, &menu))
	{
		if (String_Parse(args, &name))
		{
			Item_ActivateByName(menu,name);
		}
	}

	return qtrue;
}

/*
=================
Script_SetBackground
=================
*/
qboolean Script_SetBackground(itemDef_t *item, const char **args)
{
	const char *name;
	// expecting name to set asset to
	if (String_Parse(args, &name))
	{
		item->window.background = DC->registerShaderNoMip(name);
	}

	return qtrue;
}

/*
=================
Script_SetAsset
=================
*/
qboolean Script_SetAsset(itemDef_t *item, const char **args)
{
	const char *name;
	// expecting name to set asset to
	if (String_Parse(args, &name))
	{
		// check for a model
		if (item->type == ITEM_TYPE_MODEL)
		{
		}
	}

	return qtrue;
}

/*
=================
Script_SetFocus
=================
*/
qboolean Script_SetFocus(itemDef_t *item, const char **args)
{
	const char *name;
	itemDef_t *focusItem;

	if (String_Parse(args, &name))
	{
		focusItem = (itemDef_s *) Menu_FindItemByName((menuDef_t *) item->parent, name);
		if (focusItem && !(focusItem->window.flags & WINDOW_DECORATION) && !(focusItem->window.flags & WINDOW_HASFOCUS))
		{
			Menu_ClearFocus((menuDef_t *) item->parent);
//JLF
			focusItem->window.flags |= WINDOW_HASFOCUS;
//END JLF

			if (focusItem->onFocus)
			{
				Item_RunScript(focusItem, focusItem->onFocus);
			}
			if (DC->Assets.itemFocusSound)
			{
				DC->startLocalSound( DC->Assets.itemFocusSound, CHAN_LOCAL_SOUND );
			}
		}
	}

	return qtrue;
}


/*
=================
Script_SetItemFlag
=================
*/
qboolean Script_SetItemFlag(itemDef_t *item, const char **args)
{
	const char		*itemName,*number;

	if (String_Parse(args, &itemName))
	{
		item = (itemDef_s *) Menu_FindItemByName((menuDef_t *) item->parent, itemName);

		if (String_Parse(args, &number))
		{
			int amount = atoi(number);
			item->window.flags |= amount;
		}
	}

	return qtrue;
}

void UI_SetItemVisible(menuDef_t *menu,const char *itemname,qboolean visible)
{
	itemDef_t *item;
	int j;
	int count = Menu_ItemsMatchingGroup(menu, itemname);

	for (j = 0; j < count; j++)
	{
		item = Menu_GetMatchingItemByNumber(menu, j, itemname);
		if (item != NULL)
		{
			if (visible==qtrue)
			{
				item->window.flags |= WINDOW_VISIBLE;
			}
			else
			{
				item->window.flags &= ~WINDOW_VISIBLE;
			}
		}
	}
}

void UI_SetItemColor(itemDef_t *item,const char *itemname,const char *name,vec4_t color)
{
	itemDef_t *item2;
	int i,j;
	vec4_t *out;
	int count = Menu_ItemsMatchingGroup((menuDef_t *) item->parent, itemname);

	for (j = 0; j < count; j++)
	{
		item2 = Menu_GetMatchingItemByNumber((menuDef_t *) item->parent, j, itemname);
		if (item2 != NULL)
		{
			out = NULL;
			if (Q_stricmp(name, "backcolor") == 0)
			{
				out = &item2->window.backColor;
			}
			else if (Q_stricmp(name, "forecolor") == 0)
			{
				out = &item2->window.foreColor;
				item2->window.flags |= WINDOW_FORECOLORSET;
			}
			else if (Q_stricmp(name, "bordercolor") == 0)
			{
				out = &item2->window.borderColor;
			}

			if (out)
			{
				for (i = 0; i < 4; i++)
				{
					(*out)[i] = color[i];
				}
			}
		}
	}
}

/*
=================
Script_SetItemColor
=================
*/
qboolean Script_SetItemColor(itemDef_t *item, const char **args)
{
	const char *itemname;
	const char *name;
	vec4_t color;

	// expecting type of color to set and 4 args for the color
	if (String_Parse(args, &itemname) && String_Parse(args, &name))
	{
		if (COM_ParseVec4(args, &color))
		{
			return qtrue;
		}

		UI_SetItemColor(item,itemname,name,color);

	}

	return qtrue;
}

/*
=================
Script_Defer

Defers the rest of the script based on the defer condition.  The deferred
portion of the script can later be run with the "rundeferred"
=================
*/
qboolean Script_Defer ( itemDef_t* item, const char **args )
{
	// Should the script be deferred?
	if ( DC->deferScript ( args ) )
	{
		// Need the item the script was being run on
		uiInfo.deferredScriptItem = item;

		// Save the rest of the script
		Q_strncpyz ( uiInfo.deferredScript, *args, MAX_DEFERRED_SCRIPT, qfalse );

		// No more running
		return qfalse;
	}

	// Keep running the script, its ok
	return qtrue;
}

/*
=================
Script_RunDeferred

Runs the last deferred script, there can only be one script deferred at a
time so be careful of recursion
=================
*/
qboolean Script_RunDeferred ( itemDef_t* item, const char **args )
{
	// Make sure there is something to run.
	if ( !uiInfo.deferredScript[0] || !uiInfo.deferredScriptItem )
	{
		return qtrue;
	}

	// Run the deferred script now
	Item_RunScript ( uiInfo.deferredScriptItem, uiInfo.deferredScript );

	return qtrue;
}

/*
=================
Script_Delay

Delays the rest of the script for the specified amount of time
=================
*/
qboolean Script_Delay ( itemDef_t* item, const char **args )
{
	int time;

	if (Int_Parse(args, &time))
	{
		item->window.flags |= WINDOW_SCRIPTWAITING;
		item->window.delayTime = DC->realTime + time;	// Flag to set delay time on next paint
		item->window.delayedScript = (char *)*args;	// Copy current location, we'll resume executing here later
	}
	else
	{
		Com_Printf(S_COLOR_YELLOW"WARNING: Script_Delay: error parsing\n" );
	}

	// Stop running
	return qfalse;
}

/*
=================
Script_Transition

transition		rtvscr		321 0 202 264  415 0 202 264  20 25
=================
*/
qboolean Script_Transition(itemDef_t *item, const char **args)
{
	const char *name;
	rectDef_t rectFrom, rectTo;
	int time;
	float amt;

	if (String_Parse(args, &name))
	{
		if ( ParseRect(args, &rectFrom) && ParseRect(args, &rectTo) && Int_Parse(args, &time) && !COM_ParseFloat(args, &amt))
		{
			Menu_TransitionItemByName((menuDef_t *) item->parent, name, &rectFrom, &rectTo, time, amt);
		}
		else
		{
			Com_Printf(S_COLOR_YELLOW"WARNING: Script_Transition: error parsing '%s'\n", name );
		}
	}

	return qtrue;
}


/*
=================
Script_Transition2
uses current origin instead of specifing a starting origin

transition2		lfvscr		25 0 202 264  20 25
=================
*/
qboolean Script_Transition2(itemDef_t *item, const char **args)
{
	const char *name;
	rectDef_t rectTo;
	int time;
	float amt;

	if (String_Parse(args, &name))
	{
		if ( ParseRect(args, &rectTo) && Int_Parse(args, &time) && !COM_ParseFloat(args, &amt))
		{
			Menu_TransitionItemByName((menuDef_t *) item->parent, name, 0, &rectTo, time, amt);
		}
		else
		{
			Com_Printf(S_COLOR_YELLOW"WARNING: Script_Transition2: error parsing '%s'\n", name );
		}
	}

	return qtrue;
}

#ifdef _TRANS3
/*
JLF MPMOVED
=================
Script_Transition3

used exclusively with model views
uses current origin instead of specifing a starting origin


transition3		lfvscr		(min extent) (max extent) (fovx,y)  20 25
=================
*/
qboolean Script_Transition3(itemDef_t *item, const char **args)
{
	const char *name = NULL;
	const char *value = NULL;
	float minx, miny, minz, maxx, maxy, maxz, fovtx, fovty;
	int time;
	float amt;

	if (String_Parse(args, &name))
	{
		if (String_Parse( args, &value))
		{
			minx = atof(value);
			if (String_Parse( args, &value))
			{
				miny = atof(value);
				if (String_Parse( args, &value))
				{
					minz = atof(value);
					if (String_Parse( args, &value))
					{
						maxx = atof(value);
						if (String_Parse( args, &value))
						{
							maxy = atof(value);
							if (String_Parse( args, &value))
							{
								maxz = atof(value);
								if (String_Parse( args, &value))
								{
									fovtx = atof(value);
									if (String_Parse( args, &value))
									{
										fovty = atof(value);

										if (String_Parse( args, &value))
										{
											time = atoi(value);
											if (String_Parse( args, &value))
											{
												amt = atof(value);
												//set up the variables
												Menu_Transition3ItemByName((menuDef_t *) item->parent,
														name,
														minx, miny, minz,
														maxx, maxy, maxz,
														fovtx, fovty,
														time, amt);

												return qtrue;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	if ( name ) {
		Com_Printf( S_COLOR_YELLOW "WARNING: Script_Transition2: error parsing '%s'\n", name );
	}
	return qtrue;
}
#endif


//only works on some feeders
int GetCurrentFeederIndex(itemDef_t * item)
{
	float feederID = item->special;
	const char * name;
	int i, max;

	if (feederID == FEEDER_PLAYER_SPECIES)
	{
		return uiInfo.playerSpeciesIndex;
	}
	if (feederID == FEEDER_PLAYER_SKIN_HEAD)
	{
		name = Cvar_VariableString("ui_char_skin_head");
		max = uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHeadCount;
		for ( i = 0; i < max ; i++)
		{
			if (!Q_stricmp(name, uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHead[i].name))
			{
				return i;
			}

		//	Cvar_Set("ui_char_skin_head", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHeadNames[index]);
		}
		return -1;
	}
	else if (feederID == FEEDER_PLAYER_SKIN_TORSO)
	{
		name = Cvar_VariableString("ui_char_skin_torso");
		max = uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorsoCount;
		for ( i = 0; i < max ; i++)
		{
			if (!Q_stricmp(name, uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorso[i].name))
			{
				return i;
			}
		//	Cvar_Set("ui_char_skin_head", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHeadNames[index]);
		}
		return -1;

	}
	else if (feederID == FEEDER_PLAYER_SKIN_LEGS)
	{
		name = Cvar_VariableString("ui_char_skin_legs");
		max = uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLegCount;
		for ( i = 0; i < max ; i++)
		{
			if (!Q_stricmp(name, uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLeg[i].name))
			{
				return i;
			}
		//	Cvar_Set("ui_char_skin_head", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHeadNames[index]);
		}
		return -1;


	//	if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLegCount)
	//	{
	//		Cvar_Set("ui_char_skin_legs", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLegNames[index]);
	//	}
	}


	else if (feederID == FEEDER_COLORCHOICES)
	{
		extern void	Item_RunScript(itemDef_t *item, const char *s);		//from ui_shared;
		int currR, currG, currB, newR, newG, newB;
		currR = Cvar_VariableIntegerValue( "ui_char_color_red");
		currG = Cvar_VariableIntegerValue( "ui_char_color_green");
		currB = Cvar_VariableIntegerValue( "ui_char_color_blue");

		max = uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].ColorCount;
		for ( i = 0; i < max ; i++)
		{
			Item_RunScript(item, uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Color[i].actionText);
			newR = Cvar_VariableIntegerValue( "ui_char_color_red");
			newG = Cvar_VariableIntegerValue( "ui_char_color_green");
			newB = Cvar_VariableIntegerValue( "ui_char_color_blue");
			if ( currR == newR && currG == newG && currB == newB)
				return i;
		}
		return -1;



//JLF junk copied code
		/*
extern void	Item_RunScript(itemDef_t *item, const char *s);		//from ui_shared;
		name = Cvar_VariableString("ui_char_skin_legs");
		max = uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].ColorCount;
		for ( i = 0; i < max ; i++)
		if (!qstrcmp(name, uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLegNames[i]))
		{
			return i;
		//	Cvar_Set("ui_char_skin_head", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHeadNames[index]);
		}
		return -1;



		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].ColorCount)
		{
			Item_RunScript(item, uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].ColorActionText[index]);
		}
	*/
	}
	return -1;

}









qboolean Script_IncrementFeeder(itemDef_t * item, const char ** args)
{

	int feedercount = uiInfo.uiDC.feederCount(item->special);
	int value = GetCurrentFeederIndex(item);
	value++;
	if ( value >= feedercount)
		value = 0;
	DC->feederSelection(item->special, value, item);
	return qtrue;
}

qboolean Script_DecrementFeeder(itemDef_t * item, const char ** args)
{

	int feedercount = uiInfo.uiDC.feederCount(item->special);
	int value = GetCurrentFeederIndex(item);
	value--;
	if ( value < 0)
		value = feedercount-1;
	DC->feederSelection(item->special, value, item);
	return qtrue;
}


/*
=================
Script_SetCvar
=================
*/
qboolean Script_SetCvar(itemDef_t *item, const char **args)
{
	const char *cvar, *val;
	if (String_Parse(args, &cvar) && String_Parse(args, &val))
	{
		if(!Q_stricmp(val,"(NULL)"))
		{
			DC->setCVar(cvar, "");
		}
		else {
			DC->setCVar(cvar, val);
		}
	}

	return qtrue;
}

/*
=================
Script_Exec
=================
*/
qboolean Script_Exec ( itemDef_t *item, const char **args)
{
	const char *val;
	if (String_Parse(args, &val))
	{
		DC->executeText(EXEC_APPEND, va("%s ; ", val));
	}

	return qtrue;
}

/*
=================
Script_Play
=================
*/
static qboolean Script_Play(itemDef_t *item, const char **args)
{
	const char *val;
	if (String_Parse(args, &val))
	{
		DC->startLocalSound(DC->registerSound(val, qfalse), CHAN_AUTO );
	}

	return qtrue;
}

/*
=================
Script_PlayVoice
=================
*/
static qboolean Script_PlayVoice(itemDef_t *item, const char **args)
{
	const char *val;
	if (String_Parse(args, &val))
	{
		DC->startLocalSound(DC->registerSound(val, qfalse), CHAN_VOICE );
	}

	return qtrue;
}

/*
=================
Script_StopVoice
=================
*/
static qboolean Script_StopVoice(itemDef_t *item, const char **args)
{
	DC->startLocalSound(uiInfo.uiDC.Assets.nullSound, CHAN_VOICE );

	return qtrue;
}

/*
=================
Script_playLooped
=================
*/
/*
qboolean Script_playLooped(itemDef_t *item, const char **args)
{
	const char *val;
	if (String_Parse(args, &val))
	{
		// FIXME BOB - is this needed?
		DC->stopBackgroundTrack();
		DC->startBackgroundTrack(val, val);
	}

	return qtrue;
}
*/

/*
=================
Script_Orbit
=================
*/
qboolean Script_Orbit(itemDef_t *item, const char **args)
{
	const char *name;
	float cx, cy, x, y;
	int time;

	if (String_Parse(args, &name))
	{
//		if ( Float_Parse(args, &x) && Float_Parse(args, &y) && Float_Parse(args, &cx) && Float_Parse(args, &cy) && Int_Parse(args, &time) )
		if ( !COM_ParseFloat(args, &x) && !COM_ParseFloat(args, &y) && !COM_ParseFloat(args, &cx) && !COM_ParseFloat(args, &cy) && Int_Parse(args, &time) )
		{
			Menu_OrbitItemByName((menuDef_t *) item->parent, name, x, y, cx, cy, time);
		}
	}

	return qtrue;
}


commandDef_t commandList[] =
{
  {"activate",		&Script_Activate},				// menu
  {"close",			&Script_Close},					// menu
  {"exec",			&Script_Exec},					// group/name
  {"fadein",		&Script_FadeIn},				// group/name
  {"fadeout",		&Script_FadeOut},				// group/name
  {"hide",			&Script_Hide},					// group/name
  {"open",			&Script_Open},					// menu
  {"openGoToMenu",	&Script_OpenGoToMenu},			//
  {"orbit",			&Script_Orbit},					// group/name
  {"play",			&Script_Play},					// group/name
  {"playVoice",		&Script_PlayVoice},					// group/name
  {"stopVoice",		&Script_StopVoice},					// group/name
//  {"playlooped",	&Script_playLooped},			// group/name
  {"setasset",		&Script_SetAsset},				// works on this
  {"setbackground", &Script_SetBackground},			// works on this
  {"setcolor",		&Script_SetColor},				// works on this
  {"setcvar",		&Script_SetCvar},				// group/name
  {"setfocus",		&Script_SetFocus},				// sets this background color to team color
  {"setitemcolor",	&Script_SetItemColor},			// group/name
  {"setitemflag",	&Script_SetItemFlag},			// name
  {"show",			&Script_Show},					// group/name
  {"showMenu",		&Script_ShowMenu},				// menu
  {"transition",	&Script_Transition},			// group/name
  {"transition2",	&Script_Transition2},			// group/name
  {"setitembackground", &Script_SetItemBackground},	// group/name
  {"setitemtext",	&Script_SetItemText},			// group/name
  {"setitemrect",	&Script_SetItemRect},			// group/name
  {"defer",			&Script_Defer},					//
  {"rundeferred",	&Script_RunDeferred},			//
  {"delay",			&Script_Delay},					// works on this (script)
  {"transition3",   &Script_Transition3},			// model exclusive transition
  {"incrementfeeder", &Script_IncrementFeeder},
  {"decrementfeeder", &Script_DecrementFeeder}
};

int scriptCommandCount = sizeof(commandList) / sizeof(commandDef_t);


/*
===============
Item_Init
===============
*/
void Item_Init(itemDef_t *item)
{
	memset(item, 0, sizeof(itemDef_t));
	item->textscale = 0.55f;
	Window_Init(&item->window);
}

/*
===============
Item_Multi_Setting
===============
*/
const char *Item_Multi_Setting(itemDef_t *item)
{
	char buff[1024];
	float value = 0;
	int i;
	multiDef_t *multiPtr = (multiDef_t*)item->typeData;
	if (multiPtr)
	{
		if (multiPtr->strDef)
		{
			if (item->cvar)
			{
			    DC->getCVarString(item->cvar, buff, sizeof(buff));
			}
			else
			{

			}
		}
		else
		{
			if (item->cvar)	// Was a cvar given?
			{
				value = DC->getCVarValue(item->cvar);
			}
			else
			{
				value = item->value;
			}
		}

		for (i = 0; i < multiPtr->count; i++)
		{
			if (multiPtr->strDef)
			{
				if (Q_stricmp(buff, multiPtr->cvarStr[i]) == 0)
				{
					return multiPtr->cvarList[i];
				}
			}
			else
			{
 				if (multiPtr->cvarValue[i] == value)
				{
					return multiPtr->cvarList[i];
 				}
 			}
 		}
	}

	return "";
}

//---------------------------------------------------------------------------------------------------------
//		Item Keyword Parse functions
//---------------------------------------------------------------------------------------------------------

/*
===============
ItemParse_name
	name <string>
===============
*/
qboolean ItemParse_name( itemDef_t *item)
{
	if (!PC_ParseStringMem((const char **)&item->window.name))
	{
		return qfalse;
	}

	return qtrue;
}



qboolean ItemParse_font( itemDef_t *item )
{
	if (PC_ParseInt(&item->font))
	{
		return qfalse;
	}
	return qtrue;
}


/*
===============
ItemParse_focusSound
	name <string>
===============
*/
qboolean ItemParse_focusSound( itemDef_t *item)
{
	const char *temp;

	if (PC_ParseString(&temp))
	{
		return qfalse;
	}
	item->focusSound = DC->registerSound(temp, qfalse);
	return qtrue;
}

/*
===============
ItemParse_text
	text <string>
===============
*/
qboolean ItemParse_text( itemDef_t *item)
{
	if (!PC_ParseStringMem((const char **) &item->text))
	{
		return qfalse;
	}

//#ifdef _DEBUG
//	UI_Debug_EnterReference("TEXT", item->text);
//#endif

	return qtrue;
}

/*
===============
ItemParse_descText
	text <string>
===============
*/
qboolean ItemParse_descText( itemDef_t *item)
{
	if (!PC_ParseStringMem((const char **) &item->descText))
	{
		return qfalse;
	}

//#ifdef _DEBUG
//	UI_Debug_EnterReference("DESC", item->descText);
//#endif

	return qtrue;
}

/*
===============
ItemParse_text
	text <string>
===============
*/
qboolean ItemParse_text2( itemDef_t *item)
{
	if (!PC_ParseStringMem((const char **) &item->text2))
	{
		return qfalse;
	}

//#ifdef _DEBUG
//	UI_Debug_EnterReference("TXT2", item->text2);
//#endif

	return qtrue;
}

/*
===============
ItemParse_group
	group <string>
===============
*/
qboolean ItemParse_group( itemDef_t *item)
{
	if (!PC_ParseStringMem((const char **)&item->window.group))
	{
		return qfalse;
	}

	return qtrue;
}

/*
===============
ItemParse_asset_model
	asset_model <string>
===============
*/
qboolean ItemParse_asset_model_go( itemDef_t *item, const char *name )
{
	modelDef_t *modelPtr;
	Item_ValidateTypeData(item);
	modelPtr = (modelDef_t*)item->typeData;

	if (!Q_stricmp(&name[strlen(name) - 4], ".glm"))
	{ //it's a ghoul2 model then
		if ( item->ghoul2.size() && item->ghoul2[0].mModelindex >= 0)
		{
			DC->g2_RemoveGhoul2Model( item->ghoul2, 0 );
			item->flags &= ~ITF_G2VALID;
		}
		int g2Model = DC->g2_InitGhoul2Model(item->ghoul2, name, 0, 0, 0, 0, 0);
		if (g2Model >= 0)
		{
			item->flags |= ITF_G2VALID;

			if (modelPtr->g2anim)
			{ //does the menu request this model be playing an animation?
				DC->g2hilev_SetAnim(&item->ghoul2[0], "model_root", modelPtr->g2anim, qfalse);
			}
			if ( modelPtr->g2skin )
			{
				DC->g2_SetSkin( &item->ghoul2[0], 0, modelPtr->g2skin );//this is going to set the surfs on/off matching the skin file
			}
		}
	}
	else if(!(item->asset))
	{ //guess it's just an md3
		item->asset = DC->registerModel(name);
		item->flags &= ~ITF_G2VALID;
	}
	return qtrue;
}

qboolean ItemParse_asset_model( itemDef_t *item )
{
	const char *temp;
	Item_ValidateTypeData(item);

	if (PC_ParseString(&temp))
	{
		return qfalse;
	}
	char modelPath[MAX_QPATH];

	if (!Q_stricmp(temp,"ui_char_model") )
	{
		Com_sprintf( modelPath, sizeof( modelPath ), "models/players/%s/model.glm", Cvar_VariableString ( "g_char_model" ) );
	}
	else
	{
		Com_sprintf( modelPath, sizeof( modelPath ), temp);
	}
	return (ItemParse_asset_model_go( item, modelPath ));
}

/*
===============
ItemParse_asset_model
	asset_shader <string>
===============
*/
qboolean ItemParse_asset_shader( itemDef_t *item)
{
	const char *temp;

	if (PC_ParseString(&temp))
	{
		return qfalse;
	}
	item->asset = DC->registerShaderNoMip(temp);
	return qtrue;
}

/*
===============
ItemParse_asset_model
	model_origin <number> <number> <number>
===============
*/
qboolean ItemParse_model_origin( itemDef_t *item)
{
	modelDef_t *modelPtr;
	Item_ValidateTypeData(item);
	modelPtr = (modelDef_t*)item->typeData;

	if (PC_ParseFloat(&modelPtr->origin[0]))
	{
		if (PC_ParseFloat(&modelPtr->origin[1]))
		{
			if (PC_ParseFloat(&modelPtr->origin[2]))
			{
				return qtrue;
			}
		}
	}
	return qfalse;
}

/*
===============
ItemParse_model_fovx
	model_fovx <number>
===============
*/
qboolean ItemParse_model_fovx( itemDef_t *item)
{
	modelDef_t *modelPtr;
	Item_ValidateTypeData(item);
	modelPtr = (modelDef_t*)item->typeData;

	if (PC_ParseFloat(&modelPtr->fov_x))
	{
		return qfalse;
	}
	return qtrue;
}

/*
===============
ItemParse_model_fovy
	model_fovy <number>
===============
*/
qboolean ItemParse_model_fovy( itemDef_t *item)
{
	modelDef_t *modelPtr;
	Item_ValidateTypeData(item);
	modelPtr = (modelDef_t*)item->typeData;

	if (PC_ParseFloat(&modelPtr->fov_y))
	{
		return qfalse;
	}
	return qtrue;
}

/*
===============
ItemParse_model_rotation
	model_rotation <integer>
===============
*/
qboolean ItemParse_model_rotation( itemDef_t *item)
{
	modelDef_t *modelPtr;
	Item_ValidateTypeData(item);
	modelPtr = (modelDef_t*)item->typeData;

	if (PC_ParseInt(&modelPtr->rotationSpeed))
	{
		return qfalse;
	}
	return qtrue;
}

/*
===============
ItemParse_model_angle
	model_angle <integer>
===============
*/
qboolean ItemParse_model_angle( itemDef_t *item)
{
	modelDef_t *modelPtr;
	Item_ValidateTypeData(item);
	modelPtr = (modelDef_t*)item->typeData;

	if (PC_ParseInt(&modelPtr->angle))
	{
		return qfalse;
	}
	return qtrue;
}

// model_g2mins <number> <number> <number>
qboolean ItemParse_model_g2mins( itemDef_t *item ) {
	modelDef_t *modelPtr;
	Item_ValidateTypeData(item);
	modelPtr = (modelDef_t*)item->typeData;

	if (!PC_ParseFloat(&modelPtr->g2mins[0])) {
		if (!PC_ParseFloat(&modelPtr->g2mins[1])) {
			if (!PC_ParseFloat(&modelPtr->g2mins[2])) {
				return qtrue;
			}
		}
	}
	return qfalse;
}

// model_g2maxs <number> <number> <number>
qboolean ItemParse_model_g2maxs( itemDef_t *item ) {
	modelDef_t *modelPtr;
	Item_ValidateTypeData(item);
	modelPtr = (modelDef_t*)item->typeData;

	if (!PC_ParseFloat(&modelPtr->g2maxs[0])) {
		if (!PC_ParseFloat(&modelPtr->g2maxs[1])) {
			if (!PC_ParseFloat(&modelPtr->g2maxs[2])) {
				return qtrue;
			}
		}
	}
	return qfalse;
}

// model_g2skin <string>
qboolean ItemParse_model_g2skin_go( itemDef_t *item, const char *skinName )
{
	modelDef_t *modelPtr;

	Item_ValidateTypeData(item);
	modelPtr = (modelDef_t*)item->typeData;

	if (!skinName || !skinName[0])
	{ //it was parsed cor~rectly so still return true.
		modelPtr->g2skin = 0;
		DC->g2_SetSkin( &item->ghoul2[0], -1, 0 );//turn off custom skin
		return qtrue;
	}

	modelPtr->g2skin = DC->registerSkin(skinName);
	if ( item->ghoul2.IsValid() )
	{
		DC->g2_SetSkin( &item->ghoul2[0], 0, modelPtr->g2skin );//this is going to set the surfs on/off matching the skin file
	}

	return qtrue;
}

qboolean ItemParse_model_g2skin( itemDef_t *item )
{
	const char *skinName;

	if (PC_ParseString(&skinName)) {
		return qfalse;
	}

	return (ItemParse_model_g2skin_go( item, skinName ));
}

// model_g2anim <number>
qboolean ItemParse_model_g2anim_go( itemDef_t *item, const char *animName )
{
	modelDef_t *modelPtr;
	int i = 0;

	Item_ValidateTypeData(item);
	modelPtr = (modelDef_t*)item->typeData;

	if (!animName || !animName[0])
	{ //it was parsed correctly so still return true.
		return qtrue;
	}

	while (i < MAX_ANIMATIONS)
	{
		if (!Q_stricmp(animName, animTable[i].name))
		{ //found it
			modelPtr->g2anim = animTable[i].id;
			return qtrue;
		}
		i++;
	}

	Com_Printf("Could not find '%s' in the anim table\n", animName);
	return qtrue;
}

qboolean ItemParse_model_g2anim( itemDef_t *item ) {
	const char *animName;

	if (PC_ParseString(&animName)) {
		return qfalse;
	}

	return ItemParse_model_g2anim_go( item, animName );
}

/*
===============
ItemParse_rect
	rect <rectangle>
===============
*/
qboolean ItemParse_rect( itemDef_t *item)
{
	if (!PC_ParseRect(&item->window.rectClient))
	{
		return qfalse;
	}

	return qtrue;
}

/*
===============
ItemParse_flag
	flag <integer>
===============
*/
qboolean ItemParse_flag( itemDef_t *item)
{
	int		i;
	const char	*tempStr;

	if (PC_ParseString(&tempStr))
	{
		return qfalse;
	}

	i=0;
	while (itemFlags[i].string)
	{
		if (Q_stricmp(tempStr,itemFlags[i].string)==0)
		{
			item->window.flags |= itemFlags[i].value;
			break;
		}
		i++;
	}

	if (itemFlags[i].string == NULL)
	{
		PC_ParseWarning(va("Unknown item flag value '%s'",tempStr));
	}

	return qtrue;
}


/*
===============
ItemParse_style
	style <integer>
===============
*/
qboolean ItemParse_style( itemDef_t *item)
{
	int		i;
	const char	*tempStr;

	if (PC_ParseString(&tempStr))
	{
		return qfalse;
	}

	i=0;
	while (styles[i])
	{
		if (Q_stricmp(tempStr,styles[i])==0)
		{
			item->window.style = i;
			break;
		}
		i++;
	}

	if (styles[i] == NULL)
	{
		PC_ParseWarning(va("Unknown item style value '%s'",tempStr));
	}

	return qtrue;
}

/*
===============
ItemParse_decoration
	decoration
===============
*/
qboolean ItemParse_decoration( itemDef_t *item )
{
	item->window.flags |= WINDOW_DECORATION;
	return qtrue;
}

/*
===============
ItemParse_notselectable
	notselectable
===============
*/
qboolean ItemParse_notselectable( itemDef_t *item )
{
	listBoxDef_t *listPtr;
	Item_ValidateTypeData(item);
	listPtr = (listBoxDef_t*)item->typeData;

	if (item->type == ITEM_TYPE_LISTBOX && listPtr)
	{
		listPtr->notselectable = qtrue;
	}
	return qtrue;
}

/*
===============
ItemParse_scrollhidden
	scrollhidden
===============
*/
qboolean ItemParse_scrollhidden( itemDef_t *item )
{
	listBoxDef_t *listPtr;
	Item_ValidateTypeData(item);
	listPtr = (listBoxDef_t*)item->typeData;

	if (item->type == ITEM_TYPE_LISTBOX && listPtr)
	{
		listPtr->scrollhidden = qtrue;
	}
	return qtrue;
}


/*
===============
ItemParse_wrapped
	manually wrapped
===============
*/
qboolean ItemParse_wrapped( itemDef_t *item )
{
	item->window.flags |= WINDOW_WRAPPED;
	return qtrue;
}


/*
===============
ItemParse_autowrapped
	auto wrapped
===============
*/
qboolean ItemParse_autowrapped( itemDef_t *item)
{
	item->window.flags |= WINDOW_AUTOWRAPPED;
	return qtrue;
}


/*
===============
ItemParse_horizontalscroll
	horizontalscroll
===============
*/
qboolean ItemParse_horizontalscroll( itemDef_t *item )
{
	item->window.flags |= WINDOW_HORIZONTAL;
	return qtrue;
}


/*
===============
ItemParse_type
	type <integer>
===============
*/
qboolean ItemParse_type( itemDef_t *item )
{
	int		i;
	const char	*tempStr;

	if (PC_ParseString(&tempStr))
	{
		return qfalse;
	}

	i=0;
	while (types[i])
	{
		if (Q_stricmp(tempStr,types[i])==0)
		{
			item->type = i;
			break;
		}
		i++;
	}

	if (types[i] == NULL)
	{
		PC_ParseWarning(va("Unknown item type value '%s'",tempStr));
	}
	else
	{
		Item_ValidateTypeData(item);
	}
	return qtrue;
}

/*
===============
ItemParse_elementwidth
	 elementwidth, used for listbox image elements
	 uses textalignx for storage
===============
*/
qboolean ItemParse_elementwidth( itemDef_t *item )
{
	listBoxDef_t *listPtr;

	Item_ValidateTypeData(item);
	listPtr = (listBoxDef_t*)item->typeData;
	if (PC_ParseFloat(&listPtr->elementWidth))
	{
		return qfalse;
	}
	return qtrue;

}

/*
===============
ItemParse_elementheight
	elementheight, used for listbox image elements
	uses textaligny for storage
===============
*/
qboolean ItemParse_elementheight( itemDef_t *item )
{
	listBoxDef_t *listPtr;

	Item_ValidateTypeData(item);
	listPtr = (listBoxDef_t*)item->typeData;
	if (PC_ParseFloat(&listPtr->elementHeight))
	{
		return qfalse;
	}
	return qtrue;
}

/*
===============
ItemParse_feeder
	feeder <float>
===============
*/
qboolean ItemParse_feeder( itemDef_t *item )
{
	if (PC_ParseFloat( &item->special))
	{
		return qfalse;
	}
	return qtrue;
}

/*
===============
ItemParse_elementtype
	elementtype, used to specify what type of elements a listbox contains
	uses textstyle for storage
===============
*/
qboolean ItemParse_elementtype( itemDef_t *item )
{
	listBoxDef_t *listPtr;

	Item_ValidateTypeData(item);
	if (!item->typeData)
	{
		return qfalse;
	}

	listPtr = (listBoxDef_t*)item->typeData;
	if (PC_ParseInt(&listPtr->elementStyle))
	{
		return qfalse;
	}
	return qtrue;
}

/*
===============
ItemParse_columns
	columns sets a number of columns and an x pos and width per..
===============
*/
qboolean ItemParse_columns( itemDef_t *item)
{
	int num, i;
	listBoxDef_t *listPtr;

	Item_ValidateTypeData(item);
	if (!item->typeData)
	{
		return qfalse;
	}

	listPtr = (listBoxDef_t*)item->typeData;
	if (!PC_ParseInt(&num))
	{
		if (num > MAX_LB_COLUMNS)
		{
			num = MAX_LB_COLUMNS;
		}
		listPtr->numColumns = num;
		for (i = 0; i < num; i++)
		{
			int pos, width, maxChars;

			if (!PC_ParseInt(&pos) && !PC_ParseInt(&width) && !PC_ParseInt(&maxChars))
			{
				listPtr->columnInfo[i].pos = pos;
				listPtr->columnInfo[i].width = width;
				listPtr->columnInfo[i].maxChars = maxChars;
			}
			else
			{
				return qfalse;
			}
		}
	}
	else
	{
		return qfalse;
	}

	return qtrue;
}

/*
===============
ItemParse_border
===============
*/
qboolean ItemParse_border( itemDef_t *item)
{
	if (PC_ParseInt(&item->window.border))
	{
		return qfalse;
	}

	return qtrue;
}

/*
===============
ItemParse_bordersize
===============
*/
qboolean ItemParse_bordersize( itemDef_t *item )
{
	if (PC_ParseFloat(&item->window.borderSize))
	{
		return qfalse;
	}
	return qtrue;
}

/*
===============
ItemParse_visible
===============
*/
qboolean ItemParse_visible( itemDef_t *item)
{
	int i;

	if (PC_ParseInt(&i))
	{
		return qfalse;
	}
	if (i)
	{
		item->window.flags |= WINDOW_VISIBLE;
	}
	return qtrue;
}

/*
===============
ItemParse_ownerdraw
===============
*/
qboolean ItemParse_ownerdraw( itemDef_t *item)
{
	if (PC_ParseInt(&item->window.ownerDraw))
	{
		return qfalse;
	}
	item->type = ITEM_TYPE_OWNERDRAW;
	return qtrue;
}

/*
===============
ItemParse_align
===============
*/
qboolean ItemParse_align( itemDef_t *item)
{
	if (PC_ParseInt(&item->alignment))
	{
		return qfalse;
	}
	return qtrue;
}

/*
===============
ItemParse_align
===============
*/
qboolean ItemParse_Appearance_slot( itemDef_t *item)
{
	if (PC_ParseInt(&item->appearanceSlot))
	{
		return qfalse;
	}
	return qtrue;
}


/*
===============
ItemParse_textalign
===============
*/
qboolean ItemParse_textalign( itemDef_t *item )
{
	const char *tempStr;
	int	i;

	if (PC_ParseString(&tempStr))
	{
		return qfalse;
	}

	i=0;
	while (alignment[i])
	{
		if (Q_stricmp(tempStr,alignment[i])==0)
		{
			item->textalignment = i;
			break;
		}
		i++;
	}

	if (alignment[i] == NULL)
	{
		PC_ParseWarning(va("Unknown text alignment value '%s'",tempStr));
	}

	return qtrue;

}

/*
===============
ItemParse_text2alignx
===============
*/
qboolean ItemParse_text2alignx( itemDef_t *item)
{
	if (PC_ParseFloat(&item->text2alignx))
	{
		return qfalse;
	}
	return qtrue;
}

/*
===============
ItemParse_text2aligny
===============
*/
qboolean ItemParse_text2aligny( itemDef_t *item)
{
	if (PC_ParseFloat(&item->text2aligny))
	{
		return qfalse;
	}
	return qtrue;
}

/*
===============
ItemParse_textalignx
===============
*/
qboolean ItemParse_textalignx( itemDef_t *item)
{
	if (PC_ParseFloat(&item->textalignx))
	{
		return qfalse;
	}
	return qtrue;
}

/*
===============
ItemParse_textaligny
===============
*/
qboolean ItemParse_textaligny( itemDef_t *item)
{
	if (PC_ParseFloat(&item->textaligny))
	{
		return qfalse;
	}
	return qtrue;
}

/*
===============
ItemParse_textscale
===============
*/
qboolean ItemParse_textscale( itemDef_t *item )
{
	if (PC_ParseFloat(&item->textscale))
	{
		return qfalse;
	}
	return qtrue;
}

/*
===============
ItemParse_textstyle
===============
*/
qboolean ItemParse_textstyle( itemDef_t *item)
{
	if (PC_ParseInt(&item->textStyle))
	{
		return qfalse;
	}
	return qtrue;
}


/*
===============
ItemParse_invertyesno
===============
*/
qboolean ItemParse_invertyesno( itemDef_t *item)
{
	if (PC_ParseInt(&item->invertYesNo))
	{
		return qfalse;
	}
	return qtrue;
}

/*
===============
ItemParse_xoffset (used for yes/no and multi)
===============
*/
qboolean ItemParse_xoffset( itemDef_t *item)
{
	if (PC_ParseInt(&item->xoffset))
	{
		return qfalse;
	}
	return qtrue;
}


/*
===============
ItemParse_backcolor
===============
*/
qboolean ItemParse_backcolor( itemDef_t *item)
{
	int i;
	float f;

	for (i = 0; i < 4; i++)
	{
		if (PC_ParseFloat(&f))
		{
			return qfalse;
		}
		item->window.backColor[i]  = f;
	}
	return qtrue;
}

/*
===============
ItemParse_forecolor
===============
*/
qboolean ItemParse_forecolor( itemDef_t *item)
{
	int i;
	float f;

	for (i = 0; i < 4; i++)
	{
		if (PC_ParseFloat(&f))
		{
			return qfalse;
		}
		if (f < 0)
		{	//special case for player color
			item->window.flags |= WINDOW_PLAYERCOLOR;
			return qtrue;
		}
		item->window.foreColor[i]  = f;
		item->window.flags |= WINDOW_FORECOLORSET;
	}
	return qtrue;
}

/*
===============
ItemParse_bordercolor
===============
*/
qboolean ItemParse_bordercolor( itemDef_t *item)
{
	int i;
	float f;

	for (i = 0; i < 4; i++)
	{
		if (PC_ParseFloat(&f))
		{
			return qfalse;
		}
		item->window.borderColor[i]  = f;
	}
	return qtrue;
}

/*
===============
ItemParse_outlinecolor
===============
*/
qboolean ItemParse_outlinecolor( itemDef_t *item)
{
	if (PC_ParseColor(&item->window.outlineColor))
	{
		return qfalse;
	}
	return qtrue;
}

/*
===============
ItemParse_background
===============
*/
qboolean ItemParse_background( itemDef_t *item)
{
	const char *temp;

	if (PC_ParseString(&temp))
	{
		return qfalse;
	}
	item->window.background = ui.R_RegisterShaderNoMip(temp);
	return qtrue;
}

/*
===============
ItemParse_cinematic
===============
*/
qboolean ItemParse_cinematic( itemDef_t *item)
{
	if (!PC_ParseStringMem((const char **) &item->window.cinematicName))
	{
		return qfalse;
	}
	return qtrue;
}

/*
===============
ItemParse_doubleClick
===============
*/
qboolean ItemParse_doubleClick( itemDef_t *item)
{
	listBoxDef_t *listPtr;

	Item_ValidateTypeData(item);
	if (!item->typeData)
	{
		return qfalse;
	}

	listPtr = (listBoxDef_t*)item->typeData;

	if (!PC_Script_Parse(&listPtr->doubleClick))
	{
		return qfalse;
	}
	return qtrue;
}

/*
===============
ItemParse_onFocus
===============
*/
qboolean ItemParse_onFocus( itemDef_t *item)
{
	if (!PC_Script_Parse(&item->onFocus))
	{
		return qfalse;
	}
	return qtrue;
}

/*
===============
ItemParse_leaveFocus
===============
*/
qboolean ItemParse_leaveFocus( itemDef_t *item )
{
	if (!PC_Script_Parse(&item->leaveFocus))
	{
		return qfalse;
	}
	return qtrue;
}

/*
===============
ItemParse_mouseEnter
===============
*/
qboolean ItemParse_mouseEnter( itemDef_t *item)
{
	if (!PC_Script_Parse(&item->mouseEnter))
	{
		return qfalse;
	}
	return qtrue;
}

/*
===============
ItemParse_mouseExit
===============
*/
qboolean ItemParse_mouseExit( itemDef_t *item)
{
	if (!PC_Script_Parse(&item->mouseExit))
	{
		return qfalse;
	}
	return qtrue;
}

/*
===============
ItemParse_mouseEnterText
===============
*/
qboolean ItemParse_mouseEnterText( itemDef_t *item)
{
	if (!PC_Script_Parse(&item->mouseEnterText))
	{
		return qfalse;
	}
	return qtrue;
}

/*
===============
ItemParse_mouseExitText
===============
*/
qboolean ItemParse_mouseExitText( itemDef_t *item)
{
	if (!PC_Script_Parse(&item->mouseExitText))
	{
		return qfalse;
	}
	return qtrue;
}


/*
===============
ItemParse_accept
===============
*/
qboolean ItemParse_accept( itemDef_t *item)
{
	if (!PC_Script_Parse(&item->accept))
	{
		return qfalse;
	}
	return qtrue;
}


//JLFDPADSCRIPT
/*
===============
ItemParse_selectionNext
===============
*/
qboolean ItemParse_selectionNext( itemDef_t *item)
{
	if (!PC_Script_Parse(&item->selectionNext))
	{
		return qfalse;
	}
	return qtrue;
}

/*
===============
ItemParse_selectionPrev
===============
*/
qboolean ItemParse_selectionPrev( itemDef_t *item)
{
	if (!PC_Script_Parse(&item->selectionPrev))
	{
		return qfalse;
	}
	return qtrue;
}
// END JLFDPADSCRIPT






/*
===============
ItemParse_action
===============
*/
qboolean ItemParse_action( itemDef_t *item)
{
	if (!PC_Script_Parse(&item->action))
	{
		return qfalse;
	}
	return qtrue;
}


/*
===============
ItemParse_special
===============
*/
qboolean ItemParse_special( itemDef_t *item)
{
	if (PC_ParseFloat(&item->special))
	{
		return qfalse;
	}
	return qtrue;
}

/*
===============
ItemParse_cvarTest
===============
*/
qboolean ItemParse_cvarTest( itemDef_t *item)
{
	if (!PC_ParseStringMem((const char **) &item->cvarTest))
	{
		return qfalse;
	}
	return qtrue;
}

/*
===============
ItemParse_cvar
===============
*/
qboolean ItemParse_cvar( itemDef_t *item)
{
	editFieldDef_t *editPtr;

	Item_ValidateTypeData(item);
	if (!PC_ParseStringMem(&item->cvar))
	{
		return qfalse;
	}

	if ( item->typeData)
	{
		switch ( item->type )
		{
			case ITEM_TYPE_EDITFIELD:
			case ITEM_TYPE_NUMERICFIELD:
			case ITEM_TYPE_YESNO:
			case ITEM_TYPE_BIND:
			case ITEM_TYPE_SLIDER:
			case ITEM_TYPE_TEXT:
			case ITEM_TYPE_TEXTSCROLL:
				editPtr = (editFieldDef_t*)item->typeData;
				editPtr->minVal = -1;
				editPtr->maxVal = -1;
				editPtr->defVal = -1;
				break;
		}
	}
	return qtrue;
}

/*
===============
ItemParse_maxChars
===============
*/
qboolean ItemParse_maxChars( itemDef_t *item)
{
	editFieldDef_t *editPtr;
	int maxChars;

	Item_ValidateTypeData(item);
	if (!item->typeData)
	{
		return qfalse;
	}

	if (PC_ParseInt(&maxChars))
	{
		return qfalse;
	}
	editPtr = (editFieldDef_t*)item->typeData;
	editPtr->maxChars = maxChars;
	return qtrue;
}

/*
===============
ItemParse_maxPaintChars
===============
*/
qboolean ItemParse_maxPaintChars( itemDef_t *item)
{
	editFieldDef_t *editPtr;
	int maxChars;

	Item_ValidateTypeData(item);
	if (!item->typeData)
	{
		return qfalse;
	}

	if (PC_ParseInt(&maxChars))
	{
		return qfalse;
	}
	editPtr = (editFieldDef_t*)item->typeData;
	editPtr->maxPaintChars = maxChars;
	return qtrue;
}


qboolean ItemParse_lineHeight( itemDef_t *item)
{
	textScrollDef_t *scrollPtr;
	int				height;

	Item_ValidateTypeData(item);
	if (!item->typeData)
	{
		return qfalse;
	}

	if (PC_ParseInt(&height))
	{
		return qfalse;
	}

	scrollPtr = (textScrollDef_t*)item->typeData;
	scrollPtr->lineHeight = height;

	return qtrue;
}

/*
===============
ItemParse_cvarFloat
===============
*/
qboolean ItemParse_cvarFloat( itemDef_t *item)
{
	editFieldDef_t *editPtr;

	Item_ValidateTypeData(item);
	if (!item->typeData)
	{
		return qfalse;
	}
	editPtr = (editFieldDef_t*)item->typeData;
	if (PC_ParseStringMem((const char **) &item->cvar) &&
		!PC_ParseFloat(&editPtr->defVal) &&
		!PC_ParseFloat(&editPtr->minVal) &&
		!PC_ParseFloat(&editPtr->maxVal))
	{
		if (!Q_stricmp(item->cvar,"r_ext_texture_filter_anisotropic"))
		{//hehe, hook up the correct max value here.
			editPtr->maxVal=cls.glconfig.maxTextureFilterAnisotropy;
		}
		return qtrue;
	}

	return qfalse;
}

/*
===============
ItemParse_cvarStrList
===============
*/
qboolean ItemParse_cvarStrList( itemDef_t *item)
{
	const char	*token;
	multiDef_t *multiPtr;
	int pass;

	Item_ValidateTypeData(item);
	if (!item->typeData)
	{
		return qfalse;
	}
	multiPtr = (multiDef_t*)item->typeData;
	multiPtr->count = 0;
	multiPtr->strDef = qtrue;

	if (PC_ParseString(&token))
	{
		return qfalse;
	}

	if (!Q_stricmp(token,"feeder") && item->special == FEEDER_PLAYER_SPECIES)
	{
		for (; multiPtr->count < uiInfo.playerSpeciesCount; multiPtr->count++)
		{
			multiPtr->cvarList[multiPtr->count] = String_Alloc(Q_strupr(va("@MENUS_%s",uiInfo.playerSpecies[multiPtr->count].Name )));	//look up translation
			multiPtr->cvarStr[multiPtr->count] = uiInfo.playerSpecies[multiPtr->count].Name;	//value
		}
		return qtrue;
	}
	// languages
	if (!Q_stricmp(token,"feeder") && item->special == FEEDER_LANGUAGES)
	{
		for (; multiPtr->count < uiInfo.languageCount; multiPtr->count++)
		{
			// The displayed text
			multiPtr->cvarList[multiPtr->count] = "@MENUS_MYLANGUAGE";
			// The cvar value that goes into se_language

#ifndef JK2_MODE // FIXME
				multiPtr->cvarStr[multiPtr->count] = SE_GetLanguageName(multiPtr->count);
#endif
		}
		return qtrue;
	}
	if (*token != '{')
	{
		return qfalse;
	}

	pass = 0;
	while ( 1 )
	{
		if (!PC_ParseStringMem(&token))
		{
			PC_ParseWarning("end of file inside menu item\n");
			return qfalse;
		}
		if (*token == '}')
		{
			return qtrue;
		}

		if (*token == ',' || *token == ';')
		{
			continue;
		}

		if (pass == 0)
		{
			multiPtr->cvarList[multiPtr->count] = token;
			pass = 1;
		}
		else
		{
			multiPtr->cvarStr[multiPtr->count] = token;
			pass = 0;
			multiPtr->count++;
			if (multiPtr->count >= MAX_MULTI_CVARS)
			{
				return qfalse;
			}
		}
	}

	return qfalse;
}

/*
===============
ItemParse_cvarFloatList
===============
*/
qboolean ItemParse_cvarFloatList( itemDef_t *item)
{
	const char		*token;
	multiDef_t	*multiPtr;

	Item_ValidateTypeData(item);
	if (!item->typeData)
	{
		return qfalse;
	}
	multiPtr = (multiDef_t*)item->typeData;
	multiPtr->count = 0;
	multiPtr->strDef = qfalse;

	if (PC_ParseString(&token))
	{
		return qfalse;
	}

	if (*token != '{')
	{
		return qfalse;
	}

	while ( 1 )
	{
		if (!PC_ParseStringMem(&token))
		{
			PC_ParseWarning("end of file inside menu item\n");
			return qfalse;
		}
		if (*token == '}')
		{
			return qtrue;
		}

		if (*token == ',' || *token == ';')
		{
			continue;
		}

		multiPtr->cvarList[multiPtr->count] = token;	//a StringAlloc ptr
		if (PC_ParseFloat(&multiPtr->cvarValue[multiPtr->count]))
		{
			return qfalse;
		}

		multiPtr->count++;
		if (multiPtr->count >= MAX_MULTI_CVARS)
		{
			return qfalse;
		}

	}

	return qfalse;
}


/*
===============
ItemParse_addColorRange
===============
*/
qboolean ItemParse_addColorRange( itemDef_t *item)
{
	colorRangeDef_t color;

	if (PC_ParseFloat(&color.low) &&
		PC_ParseFloat(&color.high) &&
		PC_ParseColor(&color.color) )
	{

		if (item->numColors < MAX_COLOR_RANGES)
		{
			memcpy(&item->colorRanges[item->numColors], &color, sizeof(color));
			item->numColors++;
		}
		return qtrue;
	}
	return qfalse;
}

/*
===============
ItemParse_ownerdrawFlag
===============
*/
qboolean ItemParse_ownerdrawFlag( itemDef_t *item )
{
	int i;
	if (PC_ParseInt(&i))
	{
		return qfalse;
	}
	item->window.ownerDrawFlags |= i;
	return qtrue;
}

/*
===============
ItemParse_enableCvar
===============
*/
qboolean ItemParse_enableCvar( itemDef_t *item)
{
	if (PC_Script_Parse(&item->enableCvar))
	{
		item->cvarFlags = CVAR_ENABLE;
		return qtrue;
	}
	return qfalse;
}

/*
===============
ItemParse_disableCvar
===============
*/
qboolean ItemParse_disableCvar( itemDef_t *item )
{
	if (PC_Script_Parse(&item->enableCvar))
	{
		item->cvarFlags = CVAR_DISABLE;
		return qtrue;
	}
	return qfalse;
}

/*
===============
ItemParse_showCvar
===============
*/
qboolean ItemParse_showCvar( itemDef_t *item )
{
	if (PC_Script_Parse(&item->enableCvar))
	{
		item->cvarFlags = CVAR_SHOW;
		return qtrue;
	}
	return qfalse;
}

/*
===============
ItemParse_hideCvar
===============
*/
qboolean ItemParse_hideCvar( itemDef_t *item )
{
	if (PC_Script_Parse(&item->enableCvar))
	{
		item->cvarFlags = CVAR_HIDE;
		return qtrue;
	}
	return qfalse;
}

/*
===============
ItemParse_cvarsubstring
===============
*/
qboolean ItemParse_cvarsubstring( itemDef_t *item )
{
	assert(item->cvarFlags);	//need something set first, then we or in our flag.
	item->cvarFlags |= CVAR_SUBSTRING;
	return qtrue;
}

/*
===============
Item_ValidateTypeData
===============
*/
void Item_ValidateTypeData(itemDef_t *item)
{
	if (item->typeData)
	{
		return;
	}

	if (item->type == ITEM_TYPE_LISTBOX)
	{
		item->typeData = UI_Alloc(sizeof(listBoxDef_t));
		memset(item->typeData, 0, sizeof(listBoxDef_t));
	}
	else if (item->type == ITEM_TYPE_EDITFIELD || item->type == ITEM_TYPE_NUMERICFIELD || item->type == ITEM_TYPE_YESNO || item->type == ITEM_TYPE_BIND || item->type == ITEM_TYPE_SLIDER || item->type == ITEM_TYPE_TEXT)
	{
		item->typeData = UI_Alloc(sizeof(editFieldDef_t));
		memset(item->typeData, 0, sizeof(editFieldDef_t));
		if (item->type == ITEM_TYPE_EDITFIELD)
		{
			if (!((editFieldDef_t *) item->typeData)->maxPaintChars)
			{
				((editFieldDef_t *) item->typeData)->maxPaintChars = MAX_EDITFIELD;
			}
		}
	}
	else if (item->type == ITEM_TYPE_MULTI)
	{
		item->typeData = UI_Alloc(sizeof(multiDef_t));
	}
	else if (item->type == ITEM_TYPE_MODEL)
	{
		item->typeData = UI_Alloc(sizeof(modelDef_t));
		memset(item->typeData, 0, sizeof(modelDef_t));
	}
	else if (item->type == ITEM_TYPE_TEXTSCROLL )
	{
		item->typeData = UI_Alloc(sizeof(textScrollDef_t));
	}
}

qboolean ItemParse_isCharacter( itemDef_t *item )
{
	int i;
	if ( !PC_ParseInt(&i) )
	{
		if ( i )
		{
			item->flags |= ITF_ISCHARACTER;
		}
		else
		{
			item->flags &= ~ITF_ISCHARACTER;
		}
		return qtrue;
	}
	return qfalse;
}

qboolean ItemParse_isSaber( itemDef_t *item )
{
extern void UI_SaberLoadParms( void );
extern qboolean ui_saber_parms_parsed;
extern void UI_CacheSaberGlowGraphics( void );
	int	i;
	if ( !PC_ParseInt(&i) )
	{
		if ( i )
		{
			item->flags |= ITF_ISSABER;
			UI_CacheSaberGlowGraphics();
			if ( !ui_saber_parms_parsed )
			{
				UI_SaberLoadParms();
			}
		}
		else
		{
			item->flags &= ~ITF_ISSABER;
		}
		return qtrue;
	}
	return qfalse;
}

qboolean ItemParse_isSaber2( itemDef_t *item )
{
extern void UI_SaberLoadParms( void );
extern qboolean ui_saber_parms_parsed;
extern void UI_CacheSaberGlowGraphics( void );
	int	i;
	if ( !PC_ParseInt(&i) )
	{
		if ( i )
		{
			item->flags |= ITF_ISSABER2;
			UI_CacheSaberGlowGraphics();
			if ( !ui_saber_parms_parsed )
			{
				UI_SaberLoadParms();
			}
		}
		else
		{
			item->flags &= ~ITF_ISSABER2;
		}
		return qtrue;
	}
	return qfalse;
}

keywordHash_t itemParseKeywords[] = {
	{"accept",			ItemParse_accept,			},
	{"selectNext",		ItemParse_selectionNext,	},
	{"selectPrev",		ItemParse_selectionPrev,	},
	{"action",			ItemParse_action,			},
	{"addColorRange",	ItemParse_addColorRange,	},
	{"align",			ItemParse_align,			},
	{"appearance_slot",	ItemParse_Appearance_slot,	},
	{"asset_model",		ItemParse_asset_model,		},
	{"asset_shader",	ItemParse_asset_shader,		},
	{"isCharacter",		ItemParse_isCharacter,		},
	{"isSaber",			ItemParse_isSaber,			},
	{"isSaber2",		ItemParse_isSaber2,			},
	{"autowrapped",		ItemParse_autowrapped,		},
	{"backcolor",		ItemParse_backcolor,		},
	{"background",		ItemParse_background,		},
	{"border",			ItemParse_border,			},
	{"bordercolor",		ItemParse_bordercolor,		},
	{"bordersize",		ItemParse_bordersize,		},
	{"cinematic",		ItemParse_cinematic,		},
	{"columns",			ItemParse_columns,			},
	{"cvar",			ItemParse_cvar,				},
	{"cvarFloat",		ItemParse_cvarFloat,		},
	{"cvarFloatList",	ItemParse_cvarFloatList,	},
	{"cvarSubString",	ItemParse_cvarsubstring		},
	{"cvarStrList",		ItemParse_cvarStrList,		},
	{"cvarTest",		ItemParse_cvarTest,			},
	{"decoration",		ItemParse_decoration,		},
	{"desctext",		ItemParse_descText			},
	{"disableCvar",		ItemParse_disableCvar,		},
	{"doubleclick",		ItemParse_doubleClick,		},
	{"elementheight",	ItemParse_elementheight,	},
	{"elementtype",		ItemParse_elementtype,		},
	{"elementwidth",	ItemParse_elementwidth,		},
	{"enableCvar",		ItemParse_enableCvar,		},
	{"feeder",			ItemParse_feeder,			},
	{"flag",			ItemParse_flag,				},
	{"focusSound",		ItemParse_focusSound,		},
	{"font",			ItemParse_font,				},
	{"forecolor",		ItemParse_forecolor,		},
	{"group",			ItemParse_group,			},
	{"hideCvar",		ItemParse_hideCvar,			},
	{"horizontalscroll",ItemParse_horizontalscroll, },
	{"leaveFocus",		ItemParse_leaveFocus,		},
	{"maxChars",		ItemParse_maxChars,			},
	{"maxPaintChars",	ItemParse_maxPaintChars,	},
	{"model_angle",		ItemParse_model_angle,		},
	{"model_fovx",		ItemParse_model_fovx,		},
	{"model_fovy",		ItemParse_model_fovy,		},
	{"model_origin",	ItemParse_model_origin,		},
	{"model_rotation",	ItemParse_model_rotation,	},
	//rww - g2 begin
	{"model_g2mins",	ItemParse_model_g2mins,		},
	{"model_g2maxs",	ItemParse_model_g2maxs,		},
	{"model_g2skin",	ItemParse_model_g2skin,		},
	{"model_g2anim",	ItemParse_model_g2anim,		},
	//rww - g2 end
	{"mouseEnter",		ItemParse_mouseEnter,		},
	{"mouseEnterText",	ItemParse_mouseEnterText,	},
	{"mouseExit",		ItemParse_mouseExit,		},
	{"mouseExitText",	ItemParse_mouseExitText,	},
	{"name",			ItemParse_name				},
	{"notselectable",	ItemParse_notselectable,	},
//JLF
	{"scrollhidden",	ItemParse_scrollhidden, 	},
//JLF END
	{"onFocus",			ItemParse_onFocus,			},
	{"outlinecolor",	ItemParse_outlinecolor,		},
	{"ownerdraw",		ItemParse_ownerdraw,		},
	{"ownerdrawFlag",	ItemParse_ownerdrawFlag,	},
	{"rect",			ItemParse_rect,				},
	{"showCvar",		ItemParse_showCvar,			},
	{"special",			ItemParse_special,			},
	{"style",			ItemParse_style,			},
	{"text",			ItemParse_text				},
	{"text2",			ItemParse_text2				},
	{"text2alignx",		ItemParse_text2alignx,		},
	{"text2aligny",		ItemParse_text2aligny,		},
	{"textalign",		ItemParse_textalign,		},
	{"textalignx",		ItemParse_textalignx,		},
	{"textaligny",		ItemParse_textaligny,		},
	{"textscale",		ItemParse_textscale,		},
	{"textstyle",		ItemParse_textstyle,		},
	{"type",			ItemParse_type,				},
	{"visible",			ItemParse_visible,			},
	{"wrapped",			ItemParse_wrapped,			},
	{"invertyesno",		ItemParse_invertyesno		},
	{"xoffset",			ItemParse_xoffset			},//for yes/no and multi



	// Text scroll specific
	{"lineHeight",		ItemParse_lineHeight,		NULL	},

	{NULL,				NULL,						}
};

keywordHash_t *itemParseKeywordHash[KEYWORDHASH_SIZE];

/*
===============
Item_SetupKeywordHash
===============
*/
void Item_SetupKeywordHash(void)
{
	int i;

	memset(itemParseKeywordHash, 0, sizeof(itemParseKeywordHash));
	for (i = 0; itemParseKeywords[i].keyword; i++)
	{
		KeywordHash_Add(itemParseKeywordHash, &itemParseKeywords[i]);
	}
}


/*
===============
Item_Parse
===============
*/
qboolean Item_Parse(itemDef_t *item)
{
	keywordHash_t *key;
	const char *token;


	if (PC_ParseString(&token))
	{
		return qfalse;
	}

	if (*token != '{')
	{
		return qfalse;
	}

	while ( 1 )
	{
		if (PC_ParseString(&token))
		{
			PC_ParseWarning("End of file inside menu item");
			return qfalse;
		}

		if (*token == '}')
		{
/*			if (!item->window.name)
			{
				item->window.name = defaultString;
				Com_Printf(S_COLOR_YELLOW"WARNING: Menu item has no name\n");
			}

			if (!item->window.group)
			{
				item->window.group = defaultString;
				Com_Printf(S_COLOR_YELLOW"WARNING: Menu item has no group\n");
			}
*/
			return qtrue;
		}

		key = (keywordHash_s *) KeywordHash_Find(itemParseKeywordHash, token);
		if (!key)
		{
			PC_ParseWarning(va("Unknown item keyword '%s'", token));
			continue;
		}

		if ( !key->func(item) )
		{
			PC_ParseWarning(va("Couldn't parse item keyword '%s'", token));
			return qfalse;
		}
	}
}

static void Item_TextScroll_BuildLines ( itemDef_t* item )
{
	// new asian-aware line breaker...  (pasted from elsewhere late @ night, hence aliasing-vars ;-)
	//
	textScrollDef_t* scrollPtr = (textScrollDef_t*) item->typeData;
	const char *psText = item->text;	// for copy/paste ease
	int iBoxWidth = item->window.rect.w - SCROLLBAR_SIZE - 10;

	// this could probably be simplified now, but it was converted from something else I didn't originally write,
	//	and it works anyway so wtf...
	//
	const char *psCurrentTextReadPos;
	const char *psReadPosAtLineStart;
	const char *psBestLineBreakSrcPos;
	const char *psLastGood_s;	// needed if we get a full screen of chars with no punctuation or space (see usage notes)
	qboolean bIsTrailingPunctuation;
	unsigned int uiLetter;

	if (!psText)
	{
		return;
	}

	if (*psText == '@')	// string reference
	{
//		trap_SP_GetStringTextString( &psText[1], text, sizeof(text));
		psText = SE_GetString( &psText[1] );
	}

	psCurrentTextReadPos = psText;
	psReadPosAtLineStart = psCurrentTextReadPos;
	psBestLineBreakSrcPos = psCurrentTextReadPos;

	scrollPtr->iLineCount = 0;
	memset((char*)scrollPtr->pLines,0,sizeof(scrollPtr->pLines));

	while (*psCurrentTextReadPos && (scrollPtr->iLineCount < MAX_TEXTSCROLL_LINES) )
	{
		char sLineForDisplay[2048];	// ott

		// construct a line...
		//
		psCurrentTextReadPos = psReadPosAtLineStart;
		sLineForDisplay[0] = '\0';
		while ( *psCurrentTextReadPos )
		{
			int iAdvanceCount;
			psLastGood_s = psCurrentTextReadPos;

			// read letter...
			//
			uiLetter = ui.AnyLanguage_ReadCharFromString((char *)psCurrentTextReadPos, &iAdvanceCount, &bIsTrailingPunctuation);
			psCurrentTextReadPos += iAdvanceCount;

			// concat onto string so far...
			//
			if (uiLetter == 32 && sLineForDisplay[0] == '\0')
			{
				psReadPosAtLineStart++;
				continue;	// unless it's a space at the start of a line, in which case ignore it.
			}

			if (uiLetter > 255)
			{
				Q_strcat(sLineForDisplay, sizeof(sLineForDisplay),va("%c%c",uiLetter >> 8, uiLetter & 0xFF));
			}
			else
			{
				Q_strcat(sLineForDisplay, sizeof(sLineForDisplay),va("%c",uiLetter & 0xFF));
			}

			if (uiLetter == '\n')
			{
				// explicit new line...
				//
				sLineForDisplay[ strlen(sLineForDisplay)-1 ] = '\0';	// kill the CR
				psReadPosAtLineStart = psCurrentTextReadPos;
				psBestLineBreakSrcPos = psCurrentTextReadPos;

				// hack it to fit in with this code...
				//
				scrollPtr->pLines[ scrollPtr->iLineCount ] = String_Alloc ( sLineForDisplay );
				break;	// print this line
			}
			else
			if ( DC->textWidth( sLineForDisplay, item->textscale, item->font ) >= iBoxWidth )
			{
				// reached screen edge, so cap off string at bytepos after last good position...
				//
				if (uiLetter > 255 && bIsTrailingPunctuation && !ui.Language_UsesSpaces())
				{
					// Special case, don't consider line breaking if you're on an asian punctuation char of
					//	a language that doesn't use spaces...
					//
				}
				else
				{
					if (psBestLineBreakSrcPos == psReadPosAtLineStart)
					{
						//  aarrrggh!!!!!   we'll only get here is someone has fed in a (probably) garbage string,
						//		since it doesn't have a single space or punctuation mark right the way across one line
						//		of the screen.  So far, this has only happened in testing when I hardwired a taiwanese
						//		string into this function while the game was running in english (which should NEVER happen
						//		normally).  On the other hand I suppose it's entirely possible that some taiwanese string
						//		might have no punctuation at all, so...
						//
						psBestLineBreakSrcPos = psLastGood_s;	// force a break after last good letter
					}

					sLineForDisplay[ psBestLineBreakSrcPos - psReadPosAtLineStart ] = '\0';
					psReadPosAtLineStart = psCurrentTextReadPos = psBestLineBreakSrcPos;

					// hack it to fit in with this code...
					//
					scrollPtr->pLines[ scrollPtr->iLineCount ] = String_Alloc( sLineForDisplay );
					break;	// print this line
				}
			}

			// record last-good linebreak pos...  (ie if we've just concat'd a punctuation point (western or asian) or space)
			//
			if (bIsTrailingPunctuation || uiLetter == ' ' || (uiLetter > 255 && !ui.Language_UsesSpaces()))
			{
				psBestLineBreakSrcPos = psCurrentTextReadPos;
			}
		}

		/// arrgghh, this is gettng horrible now...
		//
		if (scrollPtr->pLines[ scrollPtr->iLineCount ] == NULL && strlen(sLineForDisplay))
		{
			// then this is the last line and we've just run out of text, no CR, no overflow etc...
			//
			scrollPtr->pLines[ scrollPtr->iLineCount ] = String_Alloc( sLineForDisplay );
		}

		scrollPtr->iLineCount++;
	}
}

/*
===============
Item_InitControls
	init's special control types
===============
*/
void Item_InitControls(itemDef_t *item)
{
	if (item == NULL)
	{
		return;
	}
	if (item->type == ITEM_TYPE_LISTBOX)
	{
		listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;
		item->cursorPos = 0;
		if (listPtr)
		{
			listPtr->cursorPos = 0;
			listPtr->startPos = 0;
			listPtr->endPos = 0;
			listPtr->cursorPos = 0;
		}
	}
}

/*
=================
Int_Parse
=================
*/
qboolean Int_Parse(const char **p, int *i)
{
	char	*token;
	token = COM_ParseExt(p, qfalse);

	if (token && token[0] != 0)
	{
		*i = atoi(token);
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

/*
=================
String_Parse
=================
*/
qboolean String_Parse(const char **p, const char **out)
{
	char *token;

	token = COM_ParseExt(p, qfalse);
	if (token && token[0] != 0)
	{
		*(out) = String_Alloc(token);
		return *(out)!=NULL;
	}
	return qfalse;
}

/*
===============
Item_RunScript
===============
*/
void Item_RunScript(itemDef_t *item, const char *s)
{
	const char *p;
	int i;
	qboolean bRan;

	uiInfo.runScriptItem = item;

	if (item && s && s[0])
	{
		p = s;
		COM_BeginParseSession();
		while (1)
		{
			const char *command;
			// expect command then arguments, ; ends command, NULL ends script
			if (!String_Parse(&p, &command))
			{
				break;
			}

			if (command[0] == ';' && command[1] == '\0')
			{
				continue;
			}

			bRan = qfalse;
			for (i = 0; i < scriptCommandCount; i++)
			{
				if (Q_stricmp(command, commandList[i].name) == 0)
				{
					if ( !(commandList[i].handler(item, &p)) )
					{
						COM_EndParseSession();
						return;
					}

					bRan = qtrue;
					break;
				}
			}
			// not in our auto list, pass to handler
			if (!bRan)
			{
				// Allow any script command to fail
				if ( !DC->runScript(&p) )
				{
					break;
				}
			}
		}
		COM_EndParseSession();
	}
}


/*
===============
Menu_SetupKeywordHash
===============
*/
void Menu_SetupKeywordHash(void)
{
	int i;

	memset(menuParseKeywordHash, 0, sizeof(menuParseKeywordHash));
	for (i = 0; menuParseKeywords[i].keyword; i++)
	{
		KeywordHash_Add(menuParseKeywordHash, &menuParseKeywords[i]);
	}
}


/*
===============
Menus_ActivateByName
===============
*/
void Menu_HandleMouseMove(menuDef_t *menu, float x, float y);
menuDef_t *Menus_ActivateByName(const char *p)
{
	int i;
	menuDef_t *m = NULL;
	menuDef_t *focus = Menu_GetFocused();

	for (i = 0; i < menuCount; i++)
	{
		// Look for the name in the current list of windows
		if (Q_stricmp(Menus[i].window.name, p) == 0)
		{
			m = &Menus[i];
			Menus_Activate(m);
			if (openMenuCount < MAX_OPEN_MENUS && focus != NULL)
			{
				menuStack[openMenuCount++] = focus;
			}
		}
		else
		{
			Menus[i].window.flags &= ~WINDOW_HASFOCUS;
		}
	}

	if (!m)
	{	// A hack so we don't have to load all three mission menus before we know what tier we're on
		if (!Q_stricmp( p, "ingameMissionSelect1" ) )
		{
			UI_LoadMenus("ui/tier1.txt",qfalse);
			Menus_CloseAll();
			Menus_OpenByName("ingameMissionSelect1");
		}
		else if (!Q_stricmp( p, "ingameMissionSelect2" ) )
		{
			UI_LoadMenus("ui/tier2.txt",qfalse);
			Menus_CloseAll();
			Menus_OpenByName("ingameMissionSelect2");
		}
		else if (!Q_stricmp( p, "ingameMissionSelect3" ) )
		{
			UI_LoadMenus("ui/tier3.txt",qfalse);
			Menus_CloseAll();
			Menus_OpenByName("ingameMissionSelect3");
		}
		else
		{
			Com_Printf(S_COLOR_YELLOW"WARNING: Menus_ActivateByName: Unable to find menu \"%s\"\n",p);
		}
	}

	// First time, show force select instructions
	if (!Q_stricmp( p, "ingameForceSelect" ) )
	{
		int	tier_storyinfo = Cvar_VariableIntegerValue( "tier_storyinfo" );

		if (tier_storyinfo==1)
		{
			Menus_OpenByName("ingameForceHelp");
		}
	}

	// First time, show weapons select instructions
	if (!Q_stricmp( p, "ingameWpnSelect" ) )
	{
		int	tier_storyinfo = Cvar_VariableIntegerValue( "tier_storyinfo" );

		if (tier_storyinfo==1)
		{
			Menus_OpenByName("ingameWpnSelectHelp");
		}
	}

	// Want to handle a mouse move on the new menu in case your already over an item
	Menu_HandleMouseMove ( m, DC->cursorx, DC->cursory );

	return m;
}

/*
===============
Menus_Activate
===============
*/
void  Menus_Activate(menuDef_t *menu)
{
	menu->window.flags |= (WINDOW_HASFOCUS | WINDOW_VISIBLE);

	if (menu->onOpen)
	{
		itemDef_t item;
		item.parent = menu;
		item.window.flags = 0;	//err, item is fake here, but we want a valid flag before calling runscript
		Item_RunScript(&item, menu->onOpen);

		if (item.window.flags & WINDOW_SCRIPTWAITING)	//in case runscript set waiting, copy it up to the menu
		{
			menu->window.flags |= WINDOW_SCRIPTWAITING;
			menu->window.delayedScript = item.window.delayedScript;
			menu->window.delayTime = item.window.delayTime;
		}
	}

//	menu->appearanceTime = DC->realTime + 1000;
	menu->appearanceTime = 0;
	menu->appearanceCnt = 0;

}

static const char *g_bindCommands[] = {
	"+altattack",
	"+attack",
	"+back",
#ifndef JK2_MODE
	"+force_drain",
#endif
	"+force_grip",
	"+force_lightning",
	"+forward",
	"+left",
	"+lookdown",
	"+lookup",
	"+mlook",
	"+movedown",
	"+moveleft",
	"+moveright",
	"+moveup",
	"+right",
	"+speed",
	"+strafe",
	"+use",
	"+useforce",
	"centerview",
	"cg_thirdperson !",
	"datapad",
	"exitview",
#ifndef JK2_MODE
	"force_absorb",
#endif
	"force_distract",
	"force_heal",
#ifndef JK2_MODE
	"force_protect",
#endif
	"force_pull",
#ifndef JK2_MODE
	"force_rage",
	"force_sight",
#endif
	"force_speed",
	"force_throw",
	"forcenext",
	"forceprev",
	"invnext",
	"invprev",
	"invuse",
	"load auto",
#ifdef JK2_MODE
	"load quik",
#else
	"load quick",
#endif
	"saberAttackCycle",
#ifdef JK2_MODE
	"save quik*",
#else
	"save quick",
#endif
	"taunt",
	"uimenu ingameloadmenu",
	"uimenu ingamesavemenu",
	"use_bacta",
	"use_electrobinoculars",
	"use_lightamp_goggles",
	"use_seeker",
	"use_sentry",
	"weapnext",
	"weapon 0",
	"weapon 1",
	"weapon 10",
	"weapon 11",
	"weapon 12",
	"weapon 13",
	"weapon 2",
	"weapon 3",
	"weapon 4",
	"weapon 5",
	"weapon 6",
	"weapon 7",
	"weapon 8",
	"weapon 9",
	"weapprev",
	"zoom"
};

#define g_bindCount ARRAY_LEN(g_bindCommands)

static int g_bindKeys[g_bindCount][2];

/*
=================
Controls_GetKeyAssignment
=================
*/
static void Controls_GetKeyAssignment( const char *command, int *twokeys )
{
	int		count;
	int		j;
	char	b[256];

	twokeys[0] = twokeys[1] = -1;
	count = 0;

	for ( j=0; j<MAX_KEYS; j++ ) {
		DC->getBindingBuf( j, b, sizeof( b ) );
		if ( *b && !Q_stricmp( b, command ) ) {
			twokeys[count] = j;
			count++;
			if ( count == 2 )
				break;
		}
	}
}

/*
=================
Controls_GetConfig
=================
*/
void Controls_GetConfig( void )
{
	size_t	i;

	// iterate each command, get its numeric binding
	for ( i = 0; i < g_bindCount; i++ )
		Controls_GetKeyAssignment( g_bindCommands[i], g_bindKeys[i] );
}


/*
===============
Item_SetScreenCoords
===============
*/
void Item_SetScreenCoords(itemDef_t *item, float x, float y)
{

	if (item == NULL)
	{
		return;
	}

	if (item->window.border != 0)
	{
		x += item->window.borderSize;
		y += item->window.borderSize;
	}

	item->window.rect.x = x + item->window.rectClient.x;
	item->window.rect.y = y + item->window.rectClient.y;
	item->window.rect.w = item->window.rectClient.w;
	item->window.rect.h = item->window.rectClient.h;

	// force the text rects to recompute
	item->textRect.w = 0;
	item->textRect.h = 0;

	switch ( item->type)
	{
		case ITEM_TYPE_TEXTSCROLL:
		{
			textScrollDef_t *scrollPtr = (textScrollDef_t*)item->typeData;
			if ( scrollPtr )
			{
				scrollPtr->startPos = 0;
				scrollPtr->endPos = 0;
			}

			Item_TextScroll_BuildLines ( item );

			break;
		}
	}
}


static void Menu_FreeGhoulItems(menuDef_t *menu)
{
	int i,j;
	for (i = 0; i < menu->itemCount; i++)
	{
		for (j=0; j < menu->items[i]->ghoul2.size(); j++)
		{
			if ( menu->items[i]->ghoul2[j].mModelindex >= 0)
			{
				DC->g2_RemoveGhoul2Model( menu->items[i]->ghoul2, j );
			}
		}
		(menu->items[i]->ghoul2).clear();	//clear is the public Free so i can actually remove this slot
	}
}
/*
===============
Menu_Reset
===============
*/
void Menu_Reset(void)
{
	//FIXME iterate menus to destoy G2 assets.
	int i;

	for (i = 0; i < menuCount; i++)
	{
		Menu_FreeGhoulItems( &Menus[i] );
	}

	menuCount = 0;
}

/*
===============
Menu_UpdatePosition
===============
*/
void Menu_UpdatePosition(menuDef_t *menu)
{
  int i;
  float x, y;

  if (menu == NULL)
  {
    return;
  }

  x = menu->window.rect.x;
  y = menu->window.rect.y;
  if (menu->window.border != 0)
  {
    x += menu->window.borderSize;
    y += menu->window.borderSize;
  }

  for (i = 0; i < menu->itemCount; i++)
  {
    Item_SetScreenCoords(menu->items[i], x, y);
  }
}

/*
===============
Menu_PostParse
===============
*/
void Menu_PostParse(menuDef_t *menu)
{
	if (menu == NULL)
	{
		return;
	}

	if (menu->fullScreen)
	{
		menu->window.rect.x = 0;
		menu->window.rect.y = 0;
		menu->window.rect.w = 640;
		menu->window.rect.h = 480;
	}
	Menu_UpdatePosition(menu);
}

/*
===============
Menu_Init
===============
*/
void Menu_Init(menuDef_t *menu)
{
	memset(menu, 0, sizeof(menuDef_t));
	menu->cursorItem = -1;

	UI_Cursor_Show(qtrue);

	if (DC)
	{
		menu->fadeAmount = DC->Assets.fadeAmount;
		menu->fadeClamp = DC->Assets.fadeClamp;
		menu->fadeCycle = DC->Assets.fadeCycle;
	}

	Window_Init(&menu->window);
}

/*
===============
Menu_Parse
===============
*/
qboolean Menu_Parse(char *inbuffer, menuDef_t *menu)
{
//	pc_token_t token;
	keywordHash_t *key;
	char	*token2;
	char * buffer;
	bool nest= false;
	buffer = inbuffer;

	token2 = PC_ParseExt();

	if (!token2)
	{
		return qfalse;
	}

	if (*token2 != '{')
	{
		PC_ParseWarning("Misplaced {");
		return qfalse;
	}
	while ( 1 )
	{

		token2 = PC_ParseExt();
		if (!token2)
		{
			PC_ParseWarning("End of file inside menu.");
			return qfalse;
		}

		if (*token2 == '}')
		{
			return qtrue;
		}

		if (nest && (*token2 == 0))
		{
			PC_EndParseSession(buffer);

			nest = false;
			continue;
		}
		key = KeywordHash_Find(menuParseKeywordHash, token2);

		if (!key)
		{
			PC_ParseWarning(va("Unknown menu keyword %s",token2));
			continue;
		}

		if ( !key->func((itemDef_t*)menu) )
		{
			PC_ParseWarning(va("Couldn't parse menu keyword %s as %s",token2, key->keyword));
			return qfalse;
		}
	}
}

/*
===============
Menu_New
===============
*/
void Menu_New(char *buffer)
{
	menuDef_t *menu = &Menus[menuCount];

	if (menuCount < MAX_MENUS)
	{
		Menu_Init(menu);
		if (Menu_Parse(buffer, menu))
		{
			Menu_PostParse(menu);
			menuCount++;
		}
	}
}

/*
===============
Menus_CloseAll
===============
*/
void  Menus_CloseAll(void)
{
	int i;

	for (i = 0; i < menuCount; i++)
	{
		Menu_RunCloseScript ( &Menus[i] );
		Menus[i].window.flags &= ~(WINDOW_HASFOCUS | WINDOW_VISIBLE);
	}

	// Clear the menu stack
	openMenuCount = 0;
}

/*
===============
PC_StartParseSession
===============
*/

int PC_StartParseSession(const char *fileName,char **buffer)
{
	int	len;

	// Try to open file and read it in.
	len = ui.FS_ReadFile( fileName,(void **) buffer  );

	// Not there?
	if ( len>0 )
	{
		COM_BeginParseSession();

		Q_strncpyz(parseData[parseDataCount].fileName, fileName, sizeof (parseData[0].fileName));
		parseData[parseDataCount].bufferStart = *buffer;
		parseData[parseDataCount].bufferCurrent = *buffer;
	}

	return len;
}

/*
===============
PC_EndParseSession
===============
*/
void PC_EndParseSession(char *buffer)
{
	COM_EndParseSession();
	ui.FS_FreeFile( buffer );	//let go of the buffer
}

/*
===============
PC_ParseWarning
===============
*/
void PC_ParseWarning(const char *message)
{
	ui.Printf(S_COLOR_YELLOW "WARNING: %s Line #%d of File '%s'\n", message,parseData[parseDataCount].com_lines,parseData[parseDataCount].fileName);
}

char *PC_ParseExt(void)
{
	if(parseDataCount < 0)
		Com_Error(ERR_FATAL, "PC_ParseExt: parseDataCount < 0 (be sure to call PC_StartParseSession!)");
	return (COM_ParseExt(&parseData[parseDataCount].bufferCurrent, qtrue));
}

qboolean PC_ParseString(const char **string)
{
	int	hold;

	if(parseDataCount < 0)
		Com_Error(ERR_FATAL, "PC_ParseString: parseDataCount < 0 (be sure to call PC_StartParseSession!)");

	hold = COM_ParseString(&parseData[parseDataCount].bufferCurrent,string);

	while (hold==0 && **string == 0)
	{
		hold = COM_ParseString(&parseData[parseDataCount].bufferCurrent,string);
	}

	return(hold);
}

qboolean PC_ParseInt(int *number)
{
	if(parseDataCount < 0)
		Com_Error(ERR_FATAL, "PC_ParseInt: parseDataCount < 0 (be sure to call PC_StartParseSession!)");

	return(COM_ParseInt(&parseData[parseDataCount].bufferCurrent,number));
}

qboolean PC_ParseFloat(float *number)
{
	if(parseDataCount < 0)
		Com_Error(ERR_FATAL, "PC_ParseFloat: parseDataCount < 0 (be sure to call PC_StartParseSession!)");

	return(COM_ParseFloat(&parseData[parseDataCount].bufferCurrent,number));
}

qboolean PC_ParseColor(vec4_t *color)
{
	if(parseDataCount < 0)
		Com_Error(ERR_FATAL, "PC_ParseColor: parseDataCount < 0 (be sure to call PC_StartParseSession!)");

	return(COM_ParseVec4(&parseData[parseDataCount].bufferCurrent, color));
}


/*
=================
Menu_Count
=================
*/
int Menu_Count(void)
{
	return menuCount;
}

/*
=================
Menu_PaintAll
=================
*/
void Menu_PaintAll(void)
{
	int i;
	if (captureFunc)
	{
		captureFunc(captureData);
	}

	for (i = 0; i < menuCount; i++)
	{
		Menu_Paint(&Menus[i], qfalse);
	}

	if (uis.debugMode)
	{
		vec4_t v = {1, 1, 1, 1};
		DC->drawText(5, 25, .75, v, va("(%d,%d)",DC->cursorx,DC->cursory), 0, 0, DC->Assets.qhMediumFont);
		DC->drawText(5, 10, .75, v, va("fps: %f", DC->FPS), 0, 0, DC->Assets.qhMediumFont);
	}
}

/*
=================
Menu_Paint
=================
*/
void Menu_Paint(menuDef_t *menu, qboolean forcePaint)
{
	int i;

	if (menu == NULL)
	{
		return;
	}

	if (menu->window.flags & WINDOW_SCRIPTWAITING)
	{
		if (DC->realTime > menu->window.delayTime)
		{ // Time has elapsed, resume running whatever script we saved
			itemDef_t item;
			item.parent = menu;
			item.window.flags = 0;	//clear before calling RunScript
			menu->window.flags &= ~WINDOW_SCRIPTWAITING;
			Item_RunScript(&item, menu->window.delayedScript);

			// Could have hit another delay. Need to hoist from fake item
			if (item.window.flags & WINDOW_SCRIPTWAITING)
			{
				menu->window.flags |= WINDOW_SCRIPTWAITING;
				menu->window.delayedScript = item.window.delayedScript;
				menu->window.delayTime = item.window.delayTime;
			}
		}
	}

	if (!(menu->window.flags & WINDOW_VISIBLE) &&  !forcePaint)
	{
		return;
	}

//	if (menu->window.ownerDrawFlags && DC->ownerDrawVisible && !DC->ownerDrawVisible(menu->window.ownerDrawFlags))
//	{
//		return;
//	}

	if (forcePaint)
	{
		menu->window.flags |= WINDOW_FORCED;
	}

	// draw the background if necessary
	if (menu->fullScreen)
	{

		vec4_t	color;
		color[0] = menu->window.backColor[0];
		color[1] = menu->window.backColor[1];
		color[2] = menu->window.backColor[2];
		color[3] = menu->window.backColor[3];

		ui.R_SetColor( color);

		if (menu->window.background==0)	// No background shader given? Make it blank
		{
			menu->window.background = uis.whiteShader;
		}

		DC->drawHandlePic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, menu->window.background );
	}
	else if (menu->window.background)
	{
		// this allows a background shader without being full screen
		//UI_DrawHandlePic(menu->window.rect.x, menu->window.rect.y, menu->window.rect.w, menu->window.rect.h, menu->backgroundShader);
	}

	// paint the background and or border
	Window_Paint(&menu->window, menu->fadeAmount, menu->fadeClamp, menu->fadeCycle );

	// Loop through all items for the menu and paint them
	int iSlotsVisible = 0;
	for (i = 0; i < menu->itemCount; i++)
	{
		if (!menu->items[i]->appearanceSlot)
		{
			Item_Paint(menu->items[i], qtrue);
		}
		else // Timed order of appearance
		{
			if (Item_Paint(menu->items[i], qfalse) )
			{
				iSlotsVisible++;	//It would paint
			}
			if (menu->items[i]->appearanceSlot<=menu->appearanceCnt)
			{
				Item_Paint(menu->items[i], qtrue);
			}
		}
	}
	if (iSlotsVisible && menu->appearanceTime < DC->realTime && menu->appearanceCnt < menu->itemCount)	// Time to show another item
	{
		menu->appearanceTime = DC->realTime + menu->appearanceIncrement;
		menu->appearanceCnt++;
	}


	if (uis.debugMode)
	{
		vec4_t color;
		color[0] = color[2] = color[3] = 1;
		color[1] = 0;
		DC->drawRect(menu->window.rect.x, menu->window.rect.y, menu->window.rect.w, menu->window.rect.h, 1, color);
	}
}

/*
=================
Item_EnableShowViaCvar
=================
*/
qboolean Item_EnableShowViaCvar(itemDef_t *item, int flag)
{
	if (item && item->enableCvar && *item->enableCvar && item->cvarTest && *item->cvarTest)
	{
		char script[1024];
		const char *p;
		char buff[1024];
		if (item->cvarFlags & CVAR_SUBSTRING)
		{
			const char *val;
			p = item->enableCvar;
			COM_BeginParseSession();
			if (!String_Parse(&p, &val))
			{//strip the quotes off
				COM_EndParseSession();
				return (item->cvarFlags & flag) ? qfalse : qtrue;
			}

			COM_EndParseSession();
			Q_strncpyz(buff, val, sizeof(buff), qtrue);
			DC->getCVarString(item->cvarTest, script, sizeof(script));
			p = script;
		}
		else
		{
			DC->getCVarString(item->cvarTest, buff, sizeof(buff));
			Q_strncpyz(script, item->enableCvar, sizeof(script), qtrue);
			p = script;
		}
		COM_BeginParseSession();
		while (1)
		{
			const char *val;
			// expect value then ; or NULL, NULL ends list
			if (!String_Parse(&p, &val))
			{
				COM_EndParseSession();
				return (item->cvarFlags & flag) ? qfalse : qtrue;
			}

			if (val[0] == ';' && val[1] == '\0')
			{
				continue;
			}

			// enable it if any of the values are true
			if (item->cvarFlags & flag)
			{
				if (Q_stricmp(buff, val) == 0)
				{
					COM_EndParseSession();
					return qtrue;
				}
			}
			else
			{
				// disable it if any of the values are true
				if (Q_stricmp(buff, val) == 0)
				{
					COM_EndParseSession();
					return qfalse;
				}
			}
		}
		COM_EndParseSession();
		return (item->cvarFlags & flag) ? qfalse : qtrue;
	}
	return qtrue;
}

bool HasStringLanguageChanged ( const itemDef_t *item )
{
	if ( !item->text || item->text[0] == '\0' )
	{
		return false;
	}

	int modificationCount;
#ifdef JK2_MODE
	modificationCount = sp_language->modificationCount;
#else
	modificationCount = se_language->modificationCount;
#endif

	return item->asset != modificationCount;
}

/*
=================
Item_SetTextExtents
=================
*/
void Item_SetTextExtents(itemDef_t *item, int *width, int *height, const char *text)
{
	const char *textPtr = (text) ? text : item->text;

	if (textPtr == NULL )
	{
		return;
	}

	*width = item->textRect.w;
	*height = item->textRect.h;

	// keeps us from computing the widths and heights more than once
	if (*width == 0 ||
		(item->type == ITEM_TYPE_OWNERDRAW && item->textalignment == ITEM_ALIGN_CENTER) ||
		HasStringLanguageChanged (item))
	{
		int originalWidth;

		originalWidth = DC->textWidth(textPtr, item->textscale, item->font);

		if (item->type == ITEM_TYPE_OWNERDRAW && (item->textalignment == ITEM_ALIGN_CENTER || item->textalignment == ITEM_ALIGN_RIGHT))
		{
			originalWidth += DC->ownerDrawWidth(item->window.ownerDraw, item->textscale);
		}
		else if (item->type == ITEM_TYPE_EDITFIELD && item->textalignment == ITEM_ALIGN_CENTER && item->cvar)
		{
			char buff[256];
			DC->getCVarString(item->cvar, buff, 256);
			originalWidth += DC->textWidth(buff, item->textscale, item->font);
		}

		*width = DC->textWidth(textPtr, item->textscale, item->font);
		*height = DC->textHeight(textPtr, item->textscale, item->font);

		item->textRect.w = *width;
		item->textRect.h = *height;
		item->textRect.x = item->textalignx;
		item->textRect.y = item->textaligny;
		if (item->textalignment == ITEM_ALIGN_RIGHT)
		{
			item->textRect.x = item->textalignx - originalWidth;
		}
		else if (item->textalignment == ITEM_ALIGN_CENTER)
		{
			item->textRect.x = item->textalignx - originalWidth / 2;
		}

		ToWindowCoords(&item->textRect.x, &item->textRect.y, &item->window);
#ifdef JK2_MODE
		if(item->text && item->text[0]=='@')
			item->asset = sp_language->modificationCount;
#else
		if (item->text && item->text[0]=='@' )//string package
			item->asset = se_language->modificationCount; //mark language
#endif
	}
}

/*
=================
Item_TextColor
=================
*/
void Item_TextColor(itemDef_t *item, vec4_t *newColor)
{
	vec4_t lowLight;
	const vec4_t greyColor = { .5, .5, .5, 1};
	menuDef_t *parent = (menuDef_t*)item->parent;

	Fade(&item->window.flags, &item->window.foreColor[3], parent->fadeClamp, &item->window.nextTime, parent->fadeCycle, qtrue, parent->fadeAmount);

	if ( !(item->type == ITEM_TYPE_TEXT && item->window.flags & WINDOW_AUTOWRAPPED) && item->window.flags & WINDOW_HASFOCUS)
	{
		lowLight[0] = 0.8 * parent->focusColor[0];
		lowLight[1] = 0.8 * parent->focusColor[1];
		lowLight[2] = 0.8 * parent->focusColor[2];
		lowLight[3] = 0.8 * parent->focusColor[3];
		LerpColor(parent->focusColor,lowLight,*newColor,0.5+0.5*sin((float)(DC->realTime / PULSE_DIVISOR)));
	}
/*	else if (item->textStyle == ITEM_TEXTSTYLE_BLINK && !((DC->realTime/BLINK_DIVISOR) & 1))
	{
		lowLight[0] = 0.8 * item->window.foreColor[0];
		lowLight[1] = 0.8 * item->window.foreColor[1];
		lowLight[2] = 0.8 * item->window.foreColor[2];
		lowLight[3] = 0.8 * item->window.foreColor[3];
		LerpColor(item->window.foreColor,lowLight,*newColor,0.5+0.5*sin(DC->realTime / PULSE_DIVISOR));
	}
*/	else
	{
		memcpy(newColor, &item->window.foreColor, sizeof(vec4_t));
	}

	// items can be enabled and disabled based on cvars
	if (item->enableCvar && *item->enableCvar && item->cvarTest && *item->cvarTest)
	{
		if (item->cvarFlags & (CVAR_ENABLE | CVAR_DISABLE) && !Item_EnableShowViaCvar(item, CVAR_ENABLE))
		{
			memcpy(newColor, &parent->disableColor, sizeof(vec4_t));
		}
	}

	if (item->window.flags & WINDOW_INACTIVE)
	{
		memcpy(newColor, &greyColor, sizeof(vec4_t));
	}
}

/*
=================
Item_Text_Wrapped_Paint
=================
*/
void Item_Text_Wrapped_Paint(itemDef_t *item)
{
	char text[1024];
	const char *p, *start, *textPtr;
	char buff[1024];
	int width, height;
	float x, y;
	vec4_t color;

	// now paint the text and/or any optional images
	// default to left

	if (item->text == NULL)
	{
		if (item->cvar == NULL)
		{
			return;
		}
		else
		{
			DC->getCVarString(item->cvar, text, sizeof(text));
			textPtr = text;
		}
	}
	else
	{
		textPtr = item->text;
	}
	if (*textPtr == '@')	// string reference
	{
		textPtr = SE_GetString( &textPtr[1] );
	}

	if (*textPtr == '\0')
	{
		return;
	}

	Item_TextColor(item, &color);
	Item_SetTextExtents(item, &width, &height, textPtr);

	x = item->textRect.x;
	y = item->textRect.y;
	start = textPtr;
	p = strchr(textPtr, '\r');
	while (p && *p)
	{
		strncpy(buff, start, p-start+1);
		buff[p-start] = '\0';
		DC->drawText(x, y, item->textscale, color, buff, 0, item->textStyle, item->font);
		y += height + 5;
		start += p - start + 1;
		p = strchr(p+1, '\r');
	}
	DC->drawText(x, y, item->textscale, color, start, 0, item->textStyle, item->font);
}


/*
=================
Menu_Paint
=================
*/
void Item_Text_Paint(itemDef_t *item)
{
	char text[1024];
	const char *textPtr;
	int height, width;
	vec4_t color;

	if (item->window.flags & WINDOW_WRAPPED)
	{
		Item_Text_Wrapped_Paint(item);
		return;
	}

	if (item->window.flags & WINDOW_AUTOWRAPPED)
	{
		Item_Text_AutoWrapped_Paint(item);
		return;
	}

	if (item->text == NULL)
	{
		if (item->cvar == NULL)
		{
			return;
		}
		else
		{
			DC->getCVarString(item->cvar, text, sizeof(text));
			textPtr = text;
		}
	}
	else
	{
		textPtr = item->text;
	}
#ifdef JK2_MODE
	if (*textPtr == '@')
	{
		textPtr = JK2SP_GetStringTextString(&textPtr[1]);
	}
#else
	if (*textPtr == '@')	// string reference
	{
		textPtr = SE_GetString( &textPtr[1] );
	}
#endif

	// this needs to go here as it sets extents for cvar types as well
	Item_SetTextExtents(item, &width, &height, textPtr);

	if (*textPtr == '\0')
	{
		return;
	}

	Item_TextColor(item, &color);
	DC->drawText(item->textRect.x, item->textRect.y, item->textscale, color, textPtr, 0, item->textStyle, item->font);

	if (item->text2)	// Is there a second line of text?
	{
		textPtr = item->text2;
		if (*textPtr == '@')	// string reference
		{
			textPtr = SE_GetString( &textPtr[1] );
		}

		Item_TextColor(item, &color);
		DC->drawText(item->textRect.x + item->text2alignx, item->textRect.y + item->text2aligny, item->textscale, color, textPtr, 0, item->textStyle, item->font);
	}
}

/*
=================
Item_UpdatePosition
=================
*/
// FIXME: consolidate this with nearby stuff
void Item_UpdatePosition(itemDef_t *item)
{
	float x, y;
	menuDef_t *menu;

	if (item == NULL || item->parent == NULL)
	{
		return;
	}

	menu = (menuDef_t *) item->parent;

	x = menu->window.rect.x;
	y = menu->window.rect.y;

	if (menu->window.border != 0)
	{
		x += menu->window.borderSize;
		y += menu->window.borderSize;
	}

	Item_SetScreenCoords(item, x, y);

}

/*
=================
Item_TextField_Paint
=================
*/
void Item_TextField_Paint(itemDef_t *item)
{
	char buff[1024];
	vec4_t newColor, lowLight;
	int offset;
	menuDef_t *parent = (menuDef_t*)item->parent;
	editFieldDef_t *editPtr = (editFieldDef_t*)item->typeData;

	Item_Text_Paint(item);

	buff[0] = '\0';

	if (item->cvar)
	{
		DC->getCVarString(item->cvar, buff, sizeof(buff));
	}

	if (item->window.flags & WINDOW_HASFOCUS)
	{
		lowLight[0] = 0.8 * parent->focusColor[0];
		lowLight[1] = 0.8 * parent->focusColor[1];
		lowLight[2] = 0.8 * parent->focusColor[2];
		lowLight[3] = 0.8 * parent->focusColor[3];
		LerpColor(parent->focusColor,lowLight,newColor,0.5+0.5*sin((float)(DC->realTime / PULSE_DIVISOR)));
	}
	else
	{
		memcpy(&newColor, &item->window.foreColor, sizeof(vec4_t));
	}

	offset = 8;//(item->text && *item->text) ? 8 : 0;
	if (item->window.flags & WINDOW_HASFOCUS && g_editingField)
	{
		char cursor = DC->getOverstrikeMode() ? '_' : '|';
		DC->drawTextWithCursor(item->textRect.x + item->textRect.w + offset, item->textRect.y, item->textscale, newColor, buff + editPtr->paintOffset, item->cursorPos - editPtr->paintOffset , cursor, /*editPtr->maxPaintChars*/ item->window.rect.w, item->textStyle, item->font);
	}
	else
	{
		DC->drawText(item->textRect.x + item->textRect.w + offset, item->textRect.y, item->textscale, newColor, buff + editPtr->paintOffset, /*editPtr->maxPaintChars*/ item->window.rect.w, item->textStyle, item->font);
	}
}

void Item_TextScroll_Paint(itemDef_t *item)
{
	char cvartext[1024];
	float x, y, size, count, thumb;
	int	  i;
	textScrollDef_t *scrollPtr = (textScrollDef_t*)item->typeData;

	size = item->window.rect.h - 2;

	// Re-arranged this function. Previous version had a plethora of bugs.
	// Still a little iffy - BTO (VV)
	if (item->cvar)
	{
		DC->getCVarString(item->cvar, cvartext, sizeof(cvartext));
		item->text = cvartext;
	}

	Item_TextScroll_BuildLines ( item );
	count = scrollPtr->iLineCount;

	// Draw scroll bar if text goes beyond bottom
	if (( scrollPtr->iLineCount * scrollPtr->lineHeight ) > size)
	{
		// draw scrollbar to right side of the window
		x = item->window.rect.x + item->window.rect.w - SCROLLBAR_SIZE - 1;
		y = item->window.rect.y + 1;
		DC->drawHandlePic(x, y, SCROLLBAR_SIZE, SCROLLBAR_SIZE, DC->Assets.scrollBarArrowUp);
		y += SCROLLBAR_SIZE - 1;

		scrollPtr->endPos = scrollPtr->startPos;
		size = item->window.rect.h - (SCROLLBAR_SIZE * 2);
		DC->drawHandlePic(x, y, SCROLLBAR_SIZE, size+1, DC->Assets.scrollBar);
		y += size - 1;
		DC->drawHandlePic(x, y, SCROLLBAR_SIZE, SCROLLBAR_SIZE, DC->Assets.scrollBarArrowDown);

		// thumb
		thumb = Item_TextScroll_ThumbDrawPosition(item);
		if (thumb > y - SCROLLBAR_SIZE - 1)
		{
			thumb = y - SCROLLBAR_SIZE - 1;
		}
		DC->drawHandlePic(x, thumb, SCROLLBAR_SIZE, SCROLLBAR_SIZE, DC->Assets.scrollBarThumb);
	}

	// adjust size for item painting
	size = item->window.rect.h - 2;
	x	 = item->window.rect.x + item->textalignx + 1;
	y	 = item->window.rect.y + item->textaligny + 1;


	for (i = scrollPtr->startPos; i < count; i++)
	{
		const char *text;

		text = scrollPtr->pLines[i];
		if (!text)
		{
			continue;
		}

		DC->drawText(x + 4, y, item->textscale, item->window.foreColor, text, 0, item->textStyle, item->font);

		size -= scrollPtr->lineHeight;
		if (size < scrollPtr->lineHeight)
		{
			scrollPtr->drawPadding = scrollPtr->lineHeight - size;
			break;
		}

		scrollPtr->endPos++;
		y += scrollPtr->lineHeight;
	}
}

/*
=================
Item_ListBox_Paint
=================
*/

void Item_ListBox_Paint(itemDef_t *item)
{
	float x, y, size;
	int count, i, thumb;
	qhandle_t image;
	qhandle_t optionalImage;
	listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;

	// the listbox is horizontal or vertical and has a fixed size scroll bar going either direction
	// elements are enumerated from the DC and either text or image handles are acquired from the DC as well
	// textscale is used to size the text, textalignx and textaligny are used to size image elements
	// there is no clipping available so only the last completely visible item is painted
	count = DC->feederCount(item->special);

	if (listPtr->startPos > (count?count-1:count))
	{//probably changed feeders, so reset
		listPtr->startPos = 0;
	}

	if (item->cursorPos > (count?count-1:count))
	{//probably changed feeders, so reset
		item->cursorPos = 0;
	}
	// default is vertical if horizontal flag is not here
	if (item->window.flags & WINDOW_HORIZONTAL)
	{
//JLF new variable (code just indented)
		if (!listPtr->scrollhidden)
		{
			// draw scrollbar in bottom of the window
			// bar
			if (Item_ListBox_MaxScroll(item) > 0)
			{
				x = item->window.rect.x + 1;
				y = item->window.rect.y + item->window.rect.h - SCROLLBAR_SIZE - 1;
				DC->drawHandlePic(x, y, SCROLLBAR_SIZE, SCROLLBAR_SIZE, DC->Assets.scrollBarArrowLeft);
				x += SCROLLBAR_SIZE - 1;
				size = item->window.rect.w - (SCROLLBAR_SIZE * 2);
				DC->drawHandlePic(x, y, size+1, SCROLLBAR_SIZE, DC->Assets.scrollBar);
				x += size - 1;
				DC->drawHandlePic(x, y, SCROLLBAR_SIZE, SCROLLBAR_SIZE, DC->Assets.scrollBarArrowRight);
				// thumb
				thumb = Item_ListBox_ThumbDrawPosition(item);//Item_ListBox_ThumbPosition(item);
				if (thumb > x - SCROLLBAR_SIZE - 1)
				{
					thumb = x - SCROLLBAR_SIZE - 1;
				}
				DC->drawHandlePic(thumb, y, SCROLLBAR_SIZE, SCROLLBAR_SIZE, DC->Assets.scrollBarThumb);
			}
			else if (listPtr->startPos > 0)
			{
				listPtr->startPos = 0;
			}
		}
//JLF end
		//
		listPtr->endPos = listPtr->startPos;
		size = item->window.rect.w - 2;
		// items
		// size contains max available space
		if (listPtr->elementStyle == LISTBOX_IMAGE)
		{
			// fit = 0;
			x = item->window.rect.x + 1;
			y = item->window.rect.y + 1;
			for (i = listPtr->startPos; i < count; i++)
			{
				// always draw at least one
				// which may overdraw the box if it is too small for the element
				image = DC->feederItemImage(item->special, i);
				if (image)
				{
					if (item->window.flags & WINDOW_PLAYERCOLOR)
					{
						vec4_t	color;
						color[0] = ui_char_color_red.integer/255.0f;
						color[1] = ui_char_color_green.integer/255.0f;
						color[2] = ui_char_color_blue.integer/255.0f;
						color[3] = 1;
						ui.R_SetColor(color);
					}
					DC->drawHandlePic(x+1, y+1, listPtr->elementWidth - 2, listPtr->elementHeight - 2, image);
				}

				if (i == item->cursorPos)
				{
					DC->drawRect(x, y, listPtr->elementWidth-1, listPtr->elementHeight-1, item->window.borderSize, item->window.borderColor);
				}

				size -= listPtr->elementWidth;
				if (size < listPtr->elementWidth)
				{
					listPtr->drawPadding = size; //listPtr->elementWidth - size;
					break;
				}
				x += listPtr->elementWidth;
				listPtr->endPos++;
				// fit++;
			}
		}
		else
		{
			//
		}
	}
	else
	{
//JLF new variable (code idented with if)
		if (!listPtr->scrollhidden)
		{
			// draw scrollbar to right side of the window
			x = item->window.rect.x + item->window.rect.w - SCROLLBAR_SIZE - 1;
			y = item->window.rect.y + 1;
			DC->drawHandlePic(x, y, SCROLLBAR_SIZE, SCROLLBAR_SIZE, DC->Assets.scrollBarArrowUp);
			y += SCROLLBAR_SIZE - 1;

			listPtr->endPos = listPtr->startPos;
			size = item->window.rect.h - (SCROLLBAR_SIZE * 2);
			DC->drawHandlePic(x, y, SCROLLBAR_SIZE, size+1, DC->Assets.scrollBar);
			y += size - 1;
			DC->drawHandlePic(x, y, SCROLLBAR_SIZE, SCROLLBAR_SIZE, DC->Assets.scrollBarArrowDown);
			// thumb
			thumb = Item_ListBox_ThumbDrawPosition(item);//Item_ListBox_ThumbPosition(item);
			if (thumb > y - SCROLLBAR_SIZE - 1)
			{
				thumb = y - SCROLLBAR_SIZE - 1;
			}
			DC->drawHandlePic(x, thumb, SCROLLBAR_SIZE, SCROLLBAR_SIZE, DC->Assets.scrollBarThumb);
		}
//JLF end
		// adjust size for item painting
		size = item->window.rect.h - 2;
		if (listPtr->elementStyle == LISTBOX_IMAGE)
		{
			// fit = 0;
			x = item->window.rect.x + 1;
			y = item->window.rect.y + 1;

			for (i = listPtr->startPos; i < count; i++)
			{
				// always draw at least one
				// which may overdraw the box if it is too small for the element
				image = DC->feederItemImage(item->special, i);
				if (image)
				{
					DC->drawHandlePic(x+1, y+1, listPtr->elementWidth - 2, listPtr->elementHeight - 2, image);
				}

				if (i == item->cursorPos)
				{
					DC->drawRect(x, y, listPtr->elementWidth - 1, listPtr->elementHeight - 1, item->window.borderSize, item->window.borderColor);
				}

				listPtr->endPos++;
				size -= listPtr->elementHeight;
				if (size < listPtr->elementHeight)
				{
					listPtr->drawPadding = listPtr->elementHeight - size;
					break;
				}
				y += listPtr->elementHeight;
				// fit++;
			}
		}
		else
		{
			x = item->window.rect.x + 1;
			y = item->window.rect.y + 1 - listPtr->elementHeight;
			i = listPtr->startPos;

			for (; i < count; i++)
			{
				const char *text;
				// always draw at least one
				// which may overdraw the box if it is too small for the element

				if (listPtr->numColumns > 0)
				{
					int j;
					for (j = 0; j < listPtr->numColumns; j++)
					{
						text = DC->feederItemText(item->special, i, j, &optionalImage);
						if (text[0]=='@')
						{
							text = SE_GetString( &text[1] );
						}

						if (optionalImage >= 0)
						{
							DC->drawHandlePic(x + 4 + listPtr->columnInfo[j].pos, y - 1 + listPtr->elementHeight / 2, listPtr->columnInfo[j].width, listPtr->columnInfo[j].width, optionalImage);
						}
						else if (text)
						{
							vec4_t	*color;
							menuDef_t *parent = (menuDef_t*)item->parent;

							// Use focus color is it has focus.
							if (i == item->cursorPos)
							{
								color = &parent->focusColor;
							}
							else
							{
								color = &item->window.foreColor;
							}


							int textyOffset;
							textyOffset = 0;

							DC->drawText(x + 4 + listPtr->columnInfo[j].pos, y + listPtr->elementHeight+ textyOffset, item->textscale, *color, text, listPtr->columnInfo[j].maxChars, item->textStyle, item->font);
						}
					}
				}
				else
				{

					text = DC->feederItemText(item->special, i, 0, &optionalImage);
					if (optionalImage >= 0)
					{
						//DC->drawHandlePic(x + 4 + listPtr->elementHeight, y, listPtr->columnInfo[j].width, listPtr->columnInfo[j].width, optionalImage);
					}
					else if (text)
					{
						DC->drawText(x + 4, y + listPtr->elementHeight, item->textscale, item->window.foreColor, text, 0, item->textStyle, item->font);
					}

				}

				// The chosen text
				if (i == item->cursorPos)
				{
					DC->fillRect(x + 2, y + listPtr->elementHeight + 2, item->window.rect.w - SCROLLBAR_SIZE - 4, listPtr->elementHeight+2, item->window.outlineColor);
				}

				size -= listPtr->elementHeight;
				if (size < listPtr->elementHeight)
				{
					listPtr->drawPadding = listPtr->elementHeight - size;
					break;
				}
				listPtr->endPos++;
				y += listPtr->elementHeight;
				// fit++;
			}
		}
	}
}

/*
=================
BindingIDFromName
=================
*/
int BindingIDFromName( const char *name ) {
	size_t i;

	// iterate each command, set its default binding
	for ( i = 0; i < g_bindCount; i++ ) {
		if ( !Q_stricmp( name, g_bindCommands[i] ) )
			return i;
	}
	return -1;
}

char g_nameBind[96];

/*
=================
BindingFromName
=================
*/
void BindingFromName( const char *cvar ) {
	size_t	i;
	int		b1, b2;
	char	sOR[32];

	// iterate each command, set its default binding
	for ( i=0; i < g_bindCount; i++ ) {
		if ( !Q_stricmp(cvar, g_bindCommands[i] ) ) {
			b2 = g_bindKeys[i][1];
			b1 = g_bindKeys[i][0];
			if ( b1 == -1 )
				break;

			if ( b2 != -1 ) {
				char keyname[2][32];

				DC->keynumToStringBuf( b1, keyname[0], sizeof( keyname[0] ) );
// do NOT do this or it corrupts asian text!!!					Q_strupr(keyname[0]);
				DC->keynumToStringBuf( b2, keyname[1], sizeof( keyname[1] ) );
// do NOT do this or it corrupts asian text!!!					Q_strupr(keyname[1]);

#ifdef JK2_MODE
				Q_strncpyz( sOR, ui.SP_GetStringTextString( "MENUS3_KEYBIND_OR" ), sizeof(sOR) );
#else
				Q_strncpyz( sOR, SE_GetString( "MENUS_KEYBIND_OR" ), sizeof(sOR) );
#endif

				Com_sprintf( g_nameBind, sizeof( g_nameBind ), "%s %s %s", keyname[0], sOR, keyname[1] );
			}
			else {
				DC->keynumToStringBuf( b1, g_nameBind, sizeof( g_nameBind ) );
// do NOT do this or it corrupts asian text!!!					Q_strupr(g_nameBind);
			}
			return;
		}
	}
	Q_strncpyz( g_nameBind, "???", sizeof( g_nameBind ) );
}

/*
=================
Item_Bind_Paint
=================
*/
void Item_Bind_Paint(itemDef_t *item)
{
	vec4_t	newColor, lowLight;
	float	value,textScale,textWidth;
	int		maxChars = 0, textHeight,yAdj,startingXPos;

	menuDef_t *parent = (menuDef_t*)item->parent;
	editFieldDef_t *editPtr = (editFieldDef_t*)item->typeData;

	if (editPtr)
	{
		maxChars = editPtr->maxPaintChars;
	}

	value = (item->cvar) ? DC->getCVarValue(item->cvar) : 0;

	if (item->window.flags & WINDOW_HASFOCUS)
	{
		if (g_bindItem == item)
		{
			lowLight[0] = 0.8f * 1.0f;
			lowLight[1] = 0.8f * 0.0f;
			lowLight[2] = 0.8f * 0.0f;
			lowLight[3] = 0.8f * 1.0f;
		}
		else
		{
			lowLight[0] = 0.8f * parent->focusColor[0];
			lowLight[1] = 0.8f * parent->focusColor[1];
			lowLight[2] = 0.8f * parent->focusColor[2];
			lowLight[3] = 0.8f * parent->focusColor[3];
		}
		LerpColor(parent->focusColor,lowLight,newColor,0.5+0.5*sin((float)(DC->realTime / PULSE_DIVISOR)));
	}
	else
	{
		Item_TextColor(	item,&newColor);
	}

	if (item->text)
	{
		Item_Text_Paint(item);
		BindingFromName(item->cvar);

		// If the text runs past the limit bring the scale down until it fits.
		textScale = item->textscale;
		textWidth = DC->textWidth(g_nameBind,(float) textScale, uiInfo.uiDC.Assets.qhMediumFont);

		startingXPos = (item->textRect.x + item->textRect.w + 8);

		while ((startingXPos + textWidth) >= SCREEN_WIDTH)
		{
			textScale -= .05f;
			textWidth = DC->textWidth(g_nameBind,(float) textScale, uiInfo.uiDC.Assets.qhMediumFont);
		}

		// Try to adjust it's y placement if the scale has changed.
		yAdj = 0;
		if (textScale != item->textscale)
		{
			textHeight = DC->textHeight(g_nameBind, item->textscale, uiInfo.uiDC.Assets.qhMediumFont);
			yAdj = textHeight - DC->textHeight(g_nameBind, textScale, uiInfo.uiDC.Assets.qhMediumFont);
		}

		DC->drawText(startingXPos, item->textRect.y + yAdj, textScale, newColor, g_nameBind, maxChars/*item->textRect.w*/, item->textStyle, item->font);
	}
	else
	{
		DC->drawText(item->textRect.x, item->textRect.y, item->textscale, newColor, (value != 0) ? "FIXME 1" : "FIXME 0", maxChars/*item->textRect.w*/, item->textStyle, item->font);
	}
}

void UI_ScaleModelAxis(refEntity_t	*ent)

{		// scale the model should we need to
		if (ent->modelScale[0] && ent->modelScale[0] != 1.0f)
		{
			VectorScale( ent->axis[0], ent->modelScale[0] , ent->axis[0] );
			ent->nonNormalizedAxes = qtrue;
		}
		if (ent->modelScale[1] && ent->modelScale[1] != 1.0f)
		{
			VectorScale( ent->axis[1], ent->modelScale[1] , ent->axis[1] );
			ent->nonNormalizedAxes = qtrue;
		}
		if (ent->modelScale[2] && ent->modelScale[2] != 1.0f)
		{
			VectorScale( ent->axis[2], ent->modelScale[2] , ent->axis[2] );
			ent->nonNormalizedAxes = qtrue;
		}
}

/*
=================
Item_Model_Paint
=================
*/

extern int	s_entityWavVol[MAX_GENTITIES];	//from snd_dma.cpp
void UI_TalkingHead(itemDef_t *item)
{
//	static int facial_blink = DC->realTime + Q_flrand(4000.0, 8000.0);
	static int facial_timer = DC->realTime + Q_flrand(10000.0, 30000.0);
//	static animNumber_t facial_anim = FACE_ALERT;
	int anim = -1;

	//are we blinking?
/*	if (facial_blink < 0)
	{	// yes, check if we are we done blinking ?
		if (-(facial_blink) < DC->realTime)
		{	// yes, so reset blink timer
			facial_blink = DC->realTime + Q_flrand(4000.0, 8000.0);
			CG_G2SetHeadBlink( cent, qfalse );	//stop the blink
		}
	}
	else // no we aren't blinking
	{
		if (facial_blink < DC->realTime)// but should we start ?
		{
			CG_G2SetHeadBlink( cent, qtrue );
			if (facial_blink == 1)
			{//requested to stay shut by SET_FACEEYESCLOSED
				facial_blink = -(DC->realTime + 99999999.0f);// set blink timer
			}
			else
			{
				facial_blink = -(DC->realTime + 300.0f);// set blink timer
			}
		}
	}
*/

	if (s_entityWavVol[0] > 0)	// if we aren't talking, then it will be 0, -1 for talking but paused
	{
		anim = FACE_TALK1 + s_entityWavVol[0]-1;
		if( anim > FACE_TALK4 )
		{
			anim = FACE_TALK4;
		}
		// reset timers so we don't start right away after talking
		facial_timer = DC->realTime + Q_flrand(2000.0, 7000.0);
	}
	else if (s_entityWavVol[0] == -1)
	{// talking, but silent
		anim = FACE_TALK0;
		// reset timers so we don't start right away after talking
		facial_timer = DC->realTime + Q_flrand(2000.0, 7000.0);
	}
/*	else if (s_entityWavVol[0] == 0)	//don't anim if in a slient part of speech
	{//not talking
		if (facial_timer < 0)	// are animating ?
		{	//yes
			if (-(facial_timer) < DC->realTime)// are we done animating ?
			{	// yes, reset timer
				facial_timer = DC->realTime + Q_flrand(10000.0, 30000.0);
			}
			else
			{	// not yet, so choose anim
				anim = facial_anim;
			}
		}
		else // no we aren't animating
		{	// but should we start ?
			if (facial_timer < DC->realTime)
			{//yes
				facial_anim = FACE_ALERT + Q_irand(0,2);	//alert, smile, frown
				// set aux timer
				facial_timer = -(DC->realTime + 2000.0);
				anim = facial_anim;
			}
		}
	}//talking
*/
	if (facial_timer < DC->realTime)
	{//restart the base anim
//		modelDef_t *modelPtr = (modelDef_t*)item->typeData;
		//ItemParse_model_g2anim_go( item, "BOTH_STAND5IDLE1" );
//		facial_timer = DC->realTime + Q_flrand(2000.0, 7000.0) + DC->g2hilev_SetAnim(&item->ghoul2[0], "model_root", modelPtr->g2anim, qtrue);
	}
	if (anim != -1)
	{
		DC->g2hilev_SetAnim(&item->ghoul2[0], "face", anim, qfalse);
	}
}

/*
=================
Item_Model_Paint
=================
*/
extern void UI_SaberDrawBlades( itemDef_t *item, vec3_t origin, float curYaw );

void Item_Model_Paint(itemDef_t *item)
{
	float x, y, w, h;
	refdef_t refdef;
	refEntity_t		ent;
	vec3_t			mins, maxs, origin;
	vec3_t			angles;
	const modelDef_t *modelPtr = (modelDef_t*)item->typeData;

	if (modelPtr == NULL)
	{
		return;
	}

	// Fuck all the logic --eez
#ifdef JK2_MODE
	// setup the refdef
	memset( &refdef, 0, sizeof( refdef ) );
	refdef.rdflags = RDF_NOWORLDMODEL;
	AxisClear( refdef.viewaxis );
	x = item->window.rect.x+1;
	y = item->window.rect.y+1;
	w = item->window.rect.w-2;
	h = item->window.rect.h-2;

	refdef.x = x * DC->xscale;
	refdef.y = y * DC->yscale;
	refdef.width = w * DC->xscale;
	refdef.height = h * DC->yscale;

	DC->modelBounds( item->asset, mins, maxs );

	origin[2] = -0.5 * ( mins[2] + maxs[2] );
	origin[1] = 0.5 * ( mins[1] + maxs[1] );

	// calculate distance so the model nearly fills the box
	if (qtrue)
	{
		float len = 0.5 * ( maxs[2] - mins[2] );
		origin[0] = len / 0.268;	// len / tan( fov/2 )
		//origin[0] = len / tan(w/2);
	}
	else
	{
		origin[0] = item->textscale;
	}
	// WTF..? --eez
	//refdef.fov_x = (modelPtr->fov_x) ? modelPtr->fov_x : w;
	//refdef.fov_y = (modelPtr->fov_y) ? modelPtr->fov_y : h;

	refdef.fov_x = 45;
	refdef.fov_y = 45;

	//refdef.fov_x = (int)((float)refdef.width / 640.0f * 90.0f);
	//xx = refdef.width / tan( refdef.fov_x / 360 * M_PI );
	//refdef.fov_y = atan2( refdef.height, xx );
	//refdef.fov_y *= ( 360 / M_PI );

	DC->clearScene();

	refdef.time = DC->realTime;

	// add the model

	memset( &ent, 0, sizeof(ent) );

	//adjust = 5.0 * sin( (float)uis.realtime / 500 );
	//adjust = 360 % (int)((float)uis.realtime / 1000);
	//VectorSet( angles, 0, 0, 1 );

	// use item storage to track
/*
	if (modelPtr->rotationSpeed)
	{
		if (DC->realTime > item->window.nextTime)
		{
			item->window.nextTime = DC->realTime + modelPtr->rotationSpeed;
			modelPtr->angle = (int)(modelPtr->angle + 1) % 360;
		}
	}
	VectorSet( angles, 0, modelPtr->angle, 0 );
*/
	VectorSet( angles, 0, (float)(refdef.time/20.0f), 0);

	AnglesToAxis( angles, ent.axis );

	ent.hModel = item->asset;
	VectorCopy( origin, ent.origin );
	VectorCopy( ent.origin, ent.oldorigin );

	// Set up lighting
	VectorCopy( refdef.vieworg, ent.lightingOrigin );
	ent.renderfx = RF_LIGHTING_ORIGIN | RF_NOSHADOW;

	DC->addRefEntityToScene( &ent );
	DC->renderScene( &refdef );
#else
	// a moves datapad anim is playing
	if (uiInfo.moveAnimTime && (uiInfo.moveAnimTime < uiInfo.uiDC.realTime))
	{
		modelDef_t *modelPtr;
		modelPtr = (modelDef_t*)item->typeData;
		if (modelPtr)
		{
			//HACKHACKHACK: check for any multi-part anim sequences, and play the next anim, if needbe
			switch( modelPtr->g2anim )
			{
			case BOTH_FORCEWALLREBOUND_FORWARD:
			case BOTH_FORCEJUMP1:
				ItemParse_model_g2anim_go( item, animTable[BOTH_FORCEINAIR1].name );
				uiInfo.moveAnimTime = DC->g2hilev_SetAnim(&item->ghoul2[0], "model_root", modelPtr->g2anim, qtrue);
				if ( !uiInfo.moveAnimTime )
				{
					uiInfo.moveAnimTime = 500;
				}
				uiInfo.moveAnimTime += uiInfo.uiDC.realTime;
				break;
			case BOTH_FORCEINAIR1:
				ItemParse_model_g2anim_go( item, animTable[BOTH_FORCELAND1].name );
				uiInfo.moveAnimTime = DC->g2hilev_SetAnim(&item->ghoul2[0], "model_root", modelPtr->g2anim, qtrue);
				uiInfo.moveAnimTime += uiInfo.uiDC.realTime;
				break;
			case BOTH_FORCEWALLRUNFLIP_START:
				ItemParse_model_g2anim_go( item, animTable[BOTH_FORCEWALLRUNFLIP_END].name );
				uiInfo.moveAnimTime = DC->g2hilev_SetAnim(&item->ghoul2[0], "model_root", modelPtr->g2anim, qtrue);
				uiInfo.moveAnimTime += uiInfo.uiDC.realTime;
				break;
			case BOTH_FORCELONGLEAP_START:
				ItemParse_model_g2anim_go( item, animTable[BOTH_FORCELONGLEAP_LAND].name );
				uiInfo.moveAnimTime = DC->g2hilev_SetAnim(&item->ghoul2[0], "model_root", modelPtr->g2anim, qtrue);
				uiInfo.moveAnimTime += uiInfo.uiDC.realTime;
				break;
			case BOTH_KNOCKDOWN3://on front - into force getup
				DC->startLocalSound(uiInfo.uiDC.Assets.datapadmoveJumpSound, CHAN_LOCAL );
				ItemParse_model_g2anim_go( item, animTable[BOTH_FORCE_GETUP_F1].name );
				uiInfo.moveAnimTime = DC->g2hilev_SetAnim(&item->ghoul2[0], "model_root", modelPtr->g2anim, qtrue);
				uiInfo.moveAnimTime += uiInfo.uiDC.realTime;
				break;
			case BOTH_KNOCKDOWN2://on back - kick forward getup
				DC->startLocalSound(uiInfo.uiDC.Assets.datapadmoveJumpSound, CHAN_LOCAL );
				ItemParse_model_g2anim_go( item, animTable[BOTH_GETUP_BROLL_F].name );
				uiInfo.moveAnimTime = DC->g2hilev_SetAnim(&item->ghoul2[0], "model_root", modelPtr->g2anim, qtrue);
				uiInfo.moveAnimTime += uiInfo.uiDC.realTime;
				break;
			case BOTH_KNOCKDOWN1://on back - roll-away
				DC->startLocalSound(uiInfo.uiDC.Assets.datapadmoveRollSound, CHAN_LOCAL );
				ItemParse_model_g2anim_go( item, animTable[BOTH_GETUP_BROLL_R].name );
				uiInfo.moveAnimTime = DC->g2hilev_SetAnim(&item->ghoul2[0], "model_root", modelPtr->g2anim, qtrue);
				uiInfo.moveAnimTime += uiInfo.uiDC.realTime;
				break;
			default:
				ItemParse_model_g2anim_go( item, uiInfo.movesBaseAnim );
				DC->g2hilev_SetAnim(&item->ghoul2[0], "model_root", modelPtr->g2anim, qtrue);
				uiInfo.moveAnimTime = 0;
				break;
			}
		}
	}

	// setup the refdef
	memset( &refdef, 0, sizeof( refdef ) );
	refdef.rdflags = RDF_NOWORLDMODEL;
	AxisClear( refdef.viewaxis );
	x = item->window.rect.x+1;
	y = item->window.rect.y+1;
	w = item->window.rect.w-2;
	h = item->window.rect.h-2;

	refdef.x = x * DC->xscale;
	refdef.y = y * DC->yscale;
	refdef.width = w * DC->xscale;
	refdef.height = h * DC->yscale;

	if (item->flags&ITF_G2VALID)
	{ //ghoul2 models don't have bounds, so we have to parse them.
		VectorCopy(modelPtr->g2mins, mins);
		VectorCopy(modelPtr->g2maxs, maxs);

		if (!mins[0] && !mins[1] && !mins[2] &&
			!maxs[0] && !maxs[1] && !maxs[2])
		{ //we'll use defaults then I suppose.
			VectorSet(mins, -16, -16, -24);
			VectorSet(maxs, 16, 16, 32);
		}
	}
	else
	{
		DC->modelBounds( item->asset, mins, maxs );
	}

	origin[2] = -0.5 * ( mins[2] + maxs[2] );
	origin[1] = 0.5 * ( mins[1] + maxs[1] );

	refdef.fov_x = (modelPtr->fov_x) ? modelPtr->fov_x : (int)((float)refdef.width / 640.0f * 90.0f);
	refdef.fov_y = (modelPtr->fov_y) ? modelPtr->fov_y : atan2( refdef.height, refdef.width / tan( refdef.fov_x / 360 * M_PI ) ) * ( 360 / M_PI );

//	refdef.fov_x = (modelPtr->fov_x) ? modelPtr->fov_x : refdef.width;
//	refdef.fov_y = (modelPtr->fov_y) ? modelPtr->fov_y : refdef.height;

	// calculate distance so the model nearly fills the box
	float len = 0.5 * ( maxs[2] - mins[2] );
	origin[0] = len / 0.268;

	DC->clearScene();

	refdef.time = DC->realTime;

	// add the model

	memset( &ent, 0, sizeof(ent) );

	// use item storage to track
	float curYaw = modelPtr->angle;
	if (modelPtr->rotationSpeed)
	{
		curYaw += (float)refdef.time/modelPtr->rotationSpeed;
	}
	if ( item->flags&ITF_ISANYSABER && !(item->flags&ITF_ISCHARACTER) )
	{//hack to put saber on it's side
		VectorSet( angles, curYaw, 0, 90 );
	}
	else
	{
		VectorSet( angles, 0, curYaw, 0 );
	}


	AnglesToAxis( angles, ent.axis );

	if (item->flags&ITF_G2VALID)
	{
		ent.ghoul2 = &item->ghoul2;
		ent.radius = 1000;
		ent.customSkin = modelPtr->g2skin;

		if ( (item->flags&ITF_ISCHARACTER) )
		{
			ent.shaderRGBA[0] = ui_char_color_red.integer;
			ent.shaderRGBA[1] = ui_char_color_green.integer;
			ent.shaderRGBA[2] = ui_char_color_blue.integer;
			ent.shaderRGBA[3] = 255;
			UI_TalkingHead(item);
		}
		if ( item->flags&ITF_ISANYSABER )
		{//UGH, draw the saber blade!
			UI_SaberDrawBlades( item, origin, curYaw );
		}
	}
	else
	{
		ent.hModel = item->asset;
	}
	VectorCopy( origin, ent.origin );
	VectorCopy( ent.origin, ent.oldorigin );

	// Set up lighting
	//VectorCopy( refdef.vieworg, ent.lightingOrigin );
	ent.renderfx = RF_NOSHADOW;

	ui.R_AddLightToScene(refdef.vieworg, 500, 1, 1, 1);	//fixme: specify in menu file!

	DC->addRefEntityToScene( &ent );
	DC->renderScene( &refdef );
#endif
}

/*
=================
Item_OwnerDraw_Paint
=================
*/
void Item_OwnerDraw_Paint(itemDef_t *item)
{
	menuDef_t *parent;

	if (item == NULL)
	{
		return;
	}

	parent = (menuDef_t*)item->parent;

	if (DC->ownerDrawItem)
	{
		vec4_t color, lowLight;
		Fade(&item->window.flags, &item->window.foreColor[3], parent->fadeClamp, &item->window.nextTime, parent->fadeCycle, qtrue, parent->fadeAmount);
		memcpy(&color, &item->window.foreColor, sizeof(color));
		if (item->numColors > 0 && DC->getValue)
		{
			// if the value is within one of the ranges then set color to that, otherwise leave at default
			int i;
			float f = DC->getValue(item->window.ownerDraw);
			for (i = 0; i < item->numColors; i++)
			{
				if (f >= item->colorRanges[i].low && f <= item->colorRanges[i].high)
				{
					memcpy(&color, &item->colorRanges[i].color, sizeof(color));
					break;
				}
			}
		}

		if (item->window.flags & WINDOW_HASFOCUS)
		{
			lowLight[0] = 0.8 * parent->focusColor[0];
			lowLight[1] = 0.8 * parent->focusColor[1];
			lowLight[2] = 0.8 * parent->focusColor[2];
			lowLight[3] = 0.8 * parent->focusColor[3];
			LerpColor(parent->focusColor,lowLight,color,0.5+0.5*sin((float)(DC->realTime / PULSE_DIVISOR)));
		}
		else if (item->textStyle == ITEM_TEXTSTYLE_BLINK && !((DC->realTime/BLINK_DIVISOR) & 1))
		{
			lowLight[0] = 0.8 * item->window.foreColor[0];
			lowLight[1] = 0.8 * item->window.foreColor[1];
			lowLight[2] = 0.8 * item->window.foreColor[2];
			lowLight[3] = 0.8 * item->window.foreColor[3];
			LerpColor(item->window.foreColor,lowLight,color,0.5+0.5*sin((float)(DC->realTime / PULSE_DIVISOR)));
		}

		if (item->cvarFlags & (CVAR_ENABLE | CVAR_DISABLE) && !Item_EnableShowViaCvar(item, CVAR_ENABLE))
		{
			memcpy(color, parent->disableColor, sizeof(vec4_t));
		}

		if (item->text)
		{
			Item_Text_Paint(item);

			// +8 is an offset kludge to properly align owner draw items that have text combined with them
			DC->ownerDrawItem(item->textRect.x + item->textRect.w + 8, item->window.rect.y, item->window.rect.w, item->window.rect.h, 0, item->textaligny, item->window.ownerDraw, item->window.ownerDrawFlags, item->alignment, item->special, item->textscale, color, item->window.background, item->textStyle, item->font );
		}
		else
		{
			DC->ownerDrawItem(item->window.rect.x, item->window.rect.y, item->window.rect.w, item->window.rect.h, item->textalignx, item->textaligny, item->window.ownerDraw, item->window.ownerDrawFlags, item->alignment, item->special, item->textscale, color, item->window.background, item->textStyle, item->font );
		}
	}
}

void Item_YesNo_Paint(itemDef_t *item)
{
	vec4_t newColor, lowLight;
	float value;
	menuDef_t *parent = (menuDef_t*)item->parent;

	value = (item->cvar) ? DC->getCVarValue(item->cvar) : 0;

	if (item->window.flags & WINDOW_HASFOCUS)
	{
		lowLight[0] = 0.8 * parent->focusColor[0];
		lowLight[1] = 0.8 * parent->focusColor[1];
		lowLight[2] = 0.8 * parent->focusColor[2];
		lowLight[3] = 0.8 * parent->focusColor[3];
		LerpColor(parent->focusColor,lowLight,newColor,0.5+0.5*sin((float)(DC->realTime / PULSE_DIVISOR)));
	}
	else
	{
		memcpy(&newColor, &item->window.foreColor, sizeof(vec4_t));
	}

#ifdef JK2_MODE
	const char *psYes = ui.SP_GetStringTextString( "MENUS0_YES" );
	const char *psNo = ui.SP_GetStringTextString( "MENUS0_NO" );
#else
	const char *psYes = SE_GetString( "MENUS_YES" );
	const char *psNo  = SE_GetString( "MENUS_NO" );
#endif
	const char *yesnovalue;

	if (item->invertYesNo)
		yesnovalue = (value == 0) ? psYes : psNo;
	else
		yesnovalue = (value != 0) ? psYes : psNo;

	if (item->text)
	{
		Item_Text_Paint(item);
		DC->drawText(item->textRect.x + item->textRect.w + 8, item->textRect.y, item->textscale, newColor, yesnovalue, 0, item->textStyle, item->font);

	}
	else
	{
		DC->drawText(item->textRect.x, item->textRect.y, item->textscale, newColor, yesnovalue , 0, item->textStyle, item->font);
	}

}

/*
=================
Item_Multi_Paint
=================
*/
void Item_Multi_Paint(itemDef_t *item)
{
	vec4_t newColor, lowLight;
	const char *text = "";
	menuDef_t *parent = (menuDef_t*)item->parent;

	if (item->window.flags & WINDOW_HASFOCUS)
	{
		lowLight[0] = 0.8 * parent->focusColor[0];
		lowLight[1] = 0.8 * parent->focusColor[1];
		lowLight[2] = 0.8 * parent->focusColor[2];
		lowLight[3] = 0.8 * parent->focusColor[3];
		LerpColor(parent->focusColor,lowLight,newColor,0.5+0.5*sin((float)(DC->realTime / PULSE_DIVISOR)));
	}
	else
	{
		memcpy(&newColor, &item->window.foreColor, sizeof(vec4_t));
	}

	text = Item_Multi_Setting(item);
	if (*text == '@')	// string reference
	{
		text = SE_GetString( &text[1] );
	}


	if (item->text)
	{
		Item_Text_Paint(item);
		DC->drawText(item->textRect.x + item->textRect.w + 8, item->textRect.y, item->textscale, newColor, text, 0, item->textStyle, item->font);
	}
	else
	{
//JLF added xoffset
		DC->drawText(item->textRect.x +item->xoffset, item->textRect.y, item->textscale, newColor, text, 0, item->textStyle, item->font);
	}
}

int Item_TextScroll_MaxScroll ( itemDef_t *item )
{
	textScrollDef_t *scrollPtr = (textScrollDef_t*)item->typeData;

	int count = scrollPtr->iLineCount;
	int max   = count - (int)(item->window.rect.h / scrollPtr->lineHeight) + 1;

	if (max < 0)
	{
		return 0;
	}

	return max;
}

int Item_TextScroll_ThumbPosition ( itemDef_t *item )
{
	float max, pos, size;
	textScrollDef_t *scrollPtr = (textScrollDef_t*)item->typeData;

	max  = Item_TextScroll_MaxScroll ( item );
	size = item->window.rect.h - (SCROLLBAR_SIZE * 2) - 2;

	if (max > 0)
	{
		pos = (size-SCROLLBAR_SIZE) / (float) max;
	}
	else
	{
		pos = 0;
	}

	pos *= scrollPtr->startPos;

	return item->window.rect.y + 1 + SCROLLBAR_SIZE + pos;
}

int Item_TextScroll_ThumbDrawPosition ( itemDef_t *item )
{
	int min, max;

	if (itemCapture == item)
	{
		min = item->window.rect.y + SCROLLBAR_SIZE + 1;
		max = item->window.rect.y + item->window.rect.h - 2*SCROLLBAR_SIZE - 1;

		if (DC->cursory >= min + SCROLLBAR_SIZE/2 && DC->cursory <= max + SCROLLBAR_SIZE/2)
		{
			return DC->cursory - SCROLLBAR_SIZE/2;
		}

		return Item_TextScroll_ThumbPosition(item);
	}

	return Item_TextScroll_ThumbPosition(item);
}

int Item_TextScroll_OverLB ( itemDef_t *item, float x, float y )
{
	rectDef_t		r;
	textScrollDef_t *scrollPtr;
	int				thumbstart;

	scrollPtr = (textScrollDef_t*)item->typeData;

	// Scroll bar isn't drawing so ignore this input
	if ((( scrollPtr->iLineCount * scrollPtr->lineHeight ) <= (item->window.rect.h - 2)) && (item->type == ITEM_TYPE_TEXTSCROLL))
	{
		return 0;
	}

	r.x = item->window.rect.x + item->window.rect.w - SCROLLBAR_SIZE;
	r.y = item->window.rect.y;
	r.h = r.w = SCROLLBAR_SIZE;
	if (Rect_ContainsPoint(&r, x, y))
	{
		return WINDOW_LB_LEFTARROW;
	}

	r.y = item->window.rect.y + item->window.rect.h - SCROLLBAR_SIZE;
	if (Rect_ContainsPoint(&r, x, y))
	{
		return WINDOW_LB_RIGHTARROW;
	}

	thumbstart = Item_TextScroll_ThumbPosition(item);
	r.y = thumbstart;
	if (Rect_ContainsPoint(&r, x, y))
	{
		return WINDOW_LB_THUMB;
	}

	r.y = item->window.rect.y + SCROLLBAR_SIZE;
	r.h = thumbstart - r.y;
	if (Rect_ContainsPoint(&r, x, y))
	{
		return WINDOW_LB_PGUP;
	}

	r.y = thumbstart + SCROLLBAR_SIZE;
	r.h = item->window.rect.y + item->window.rect.h - SCROLLBAR_SIZE;
	if (Rect_ContainsPoint(&r, x, y))
	{
		return WINDOW_LB_PGDN;
	}

	return 0;
}

void Item_TextScroll_MouseEnter (itemDef_t *item, float x, float y)
{
	item->window.flags &= ~(WINDOW_LB_LEFTARROW | WINDOW_LB_RIGHTARROW | WINDOW_LB_THUMB | WINDOW_LB_PGUP | WINDOW_LB_PGDN);
	item->window.flags |= Item_TextScroll_OverLB(item, x, y);
}

/*
=================
Item_Slider_ThumbPosition
=================
*/
int Item_ListBox_ThumbDrawPosition(itemDef_t *item)
{
	int min, max;

	if (itemCapture == item)
	{
		if (item->window.flags & WINDOW_HORIZONTAL)
		{
			min = item->window.rect.x + SCROLLBAR_SIZE + 1;
			max = item->window.rect.x + item->window.rect.w - 2*SCROLLBAR_SIZE - 1;
			if (DC->cursorx >= min + SCROLLBAR_SIZE/2 && DC->cursorx <= max + SCROLLBAR_SIZE/2)
			{
				return DC->cursorx - SCROLLBAR_SIZE/2;
			}
			else
			{
				return Item_ListBox_ThumbPosition(item);
			}
		}
		else
		{
			min = item->window.rect.y + SCROLLBAR_SIZE + 1;
			max = item->window.rect.y + item->window.rect.h - 2*SCROLLBAR_SIZE - 1;
			if (DC->cursory >= min + SCROLLBAR_SIZE/2 && DC->cursory <= max + SCROLLBAR_SIZE/2)
			{
				return DC->cursory - SCROLLBAR_SIZE/2;
			}
			else
			{
				return Item_ListBox_ThumbPosition(item);
			}
		}
	}
	else
	{
		return Item_ListBox_ThumbPosition(item);
	}
}

/*
=================
Item_Slider_ThumbPosition
=================
*/
float Item_Slider_ThumbPosition(itemDef_t *item)
{
	float value, range, x;
	editFieldDef_t *editDef = (editFieldDef_t *) item->typeData;

	if (item->text)
	{
		x = item->textRect.x + item->textRect.w + 8;
	}
	else
	{
		x = item->window.rect.x;
	}

	if (!editDef || !item->cvar)
	{
		return x;
	}

	value = DC->getCVarValue(item->cvar);

	if (value < editDef->minVal)
	{
		value = editDef->minVal;
	}
	else if (value > editDef->maxVal)
	{
		value = editDef->maxVal;
	}

	range = editDef->maxVal - editDef->minVal;
	value -= editDef->minVal;
	value /= range;
	//value /= (editDef->maxVal - editDef->minVal);
	value *= SLIDER_WIDTH;
	x += value;
	// vm fuckage
	//x = x + (((float)value / editDef->maxVal) * SLIDER_WIDTH);
	return x;
}

/*
=================
Item_Slider_Paint
=================
*/
void Item_Slider_Paint(itemDef_t *item)
{
	vec4_t newColor, lowLight;
	float x, y;
	menuDef_t *parent = (menuDef_t*)item->parent;

	if (item->window.flags & WINDOW_HASFOCUS)
	{
		lowLight[0] = 0.8 * parent->focusColor[0];
		lowLight[1] = 0.8 * parent->focusColor[1];
		lowLight[2] = 0.8 * parent->focusColor[2];
		lowLight[3] = 0.8 * parent->focusColor[3];
		LerpColor(parent->focusColor,lowLight,newColor,0.5+0.5*sin((float)(DC->realTime / PULSE_DIVISOR)));
	}
	else
	{
		memcpy(&newColor, &item->window.foreColor, sizeof(vec4_t));
	}

	y = item->window.rect.y;
	if (item->text)
	{
		Item_Text_Paint(item);
		x = item->textRect.x + item->textRect.w + 8;
	}
	else
	{
		x = item->window.rect.x;
	}
	DC->setColor(newColor);
	DC->drawHandlePic( x, y+2, SLIDER_WIDTH, SLIDER_HEIGHT, DC->Assets.sliderBar );

	x = Item_Slider_ThumbPosition(item);
//	DC->drawHandlePic( x - (SLIDER_THUMB_WIDTH / 2), y - 2, SLIDER_THUMB_WIDTH, SLIDER_THUMB_HEIGHT, DC->Assets.sliderThumb );
	DC->drawHandlePic( x - (SLIDER_THUMB_WIDTH / 2), y+2, SLIDER_THUMB_WIDTH, SLIDER_THUMB_HEIGHT, DC->Assets.sliderThumb );

}

/*
=================
Item_Paint
=================
*/
static qboolean Item_Paint(itemDef_t *item, qboolean bDraw)
{
	int		xPos,textWidth;
	vec4_t red;
	menuDef_t *parent;
	red[0] = red[3] = 1;
	red[1] = red[2] = 0;

	if (item == NULL)
	{
		return qfalse;
	}

	parent = (menuDef_t*)item->parent;

	if (item->window.flags & WINDOW_SCRIPTWAITING)
	{
		if (DC->realTime > item->window.delayTime)
		{ // Time has elapsed, resume running whatever script we saved
			item->window.flags &= ~WINDOW_SCRIPTWAITING;
			Item_RunScript(item, item->window.delayedScript);
		}
	}

	if (item->window.flags & WINDOW_ORBITING)
	{
		if (DC->realTime > item->window.nextTime)
		{
			float rx, ry, a, c, s, w, h;
			item->window.nextTime = DC->realTime + item->window.offsetTime;
			// translate
			w = item->window.rectClient.w / 2;
			h = item->window.rectClient.h / 2;
			rx = item->window.rectClient.x + w - item->window.rectEffects.x;
			ry = item->window.rectClient.y + h - item->window.rectEffects.y;
			a = (float) (3 * M_PI / 180);
  			c = cos(a);
			s = sin(a);
			item->window.rectClient.x = (rx * c - ry * s) + item->window.rectEffects.x - w;
			item->window.rectClient.y = (rx * s + ry * c) + item->window.rectEffects.y - h;
			Item_UpdatePosition(item);

		}
	}


	if (item->window.flags & WINDOW_INTRANSITION)
	{
		if (DC->realTime > item->window.nextTime)
		{
			int done = 0;
			item->window.nextTime = DC->realTime + item->window.offsetTime;

			// transition the x,y
			if (item->window.rectClient.x == item->window.rectEffects.x)
			{
				done++;
			}
			else
			{
				if (item->window.rectClient.x < item->window.rectEffects.x)
				{
					item->window.rectClient.x += item->window.rectEffects2.x;
					if (item->window.rectClient.x > item->window.rectEffects.x)
					{
						item->window.rectClient.x = item->window.rectEffects.x;
						done++;
					}
				}
				else
				{
					item->window.rectClient.x -= item->window.rectEffects2.x;
					if (item->window.rectClient.x < item->window.rectEffects.x)
					{
						item->window.rectClient.x = item->window.rectEffects.x;
						done++;
					}
				}
			}

			if (item->window.rectClient.y == item->window.rectEffects.y)
			{
				done++;
			}
			else
			{
				if (item->window.rectClient.y < item->window.rectEffects.y)
				{
					item->window.rectClient.y += item->window.rectEffects2.y;
					if (item->window.rectClient.y > item->window.rectEffects.y)
					{
						item->window.rectClient.y = item->window.rectEffects.y;
						done++;
					}
				}
				else
				{
					item->window.rectClient.y -= item->window.rectEffects2.y;
					if (item->window.rectClient.y < item->window.rectEffects.y)
					{
						item->window.rectClient.y = item->window.rectEffects.y;
						done++;
					}
				}
			}

			if (item->window.rectClient.w == item->window.rectEffects.w)
			{
				done++;
			}
			else
			{
				if (item->window.rectClient.w < item->window.rectEffects.w)
				{
					item->window.rectClient.w += item->window.rectEffects2.w;
					if (item->window.rectClient.w > item->window.rectEffects.w)
					{
						item->window.rectClient.w = item->window.rectEffects.w;
						done++;
					}
				}
				else
				{
					item->window.rectClient.w -= item->window.rectEffects2.w;
					if (item->window.rectClient.w < item->window.rectEffects.w)
					{
						item->window.rectClient.w = item->window.rectEffects.w;
						done++;
					}
				}
			}

			if (item->window.rectClient.h == item->window.rectEffects.h)
			{
				done++;
			}
			else
			{
				if (item->window.rectClient.h < item->window.rectEffects.h)
				{
					item->window.rectClient.h += item->window.rectEffects2.h;
					if (item->window.rectClient.h > item->window.rectEffects.h)
					{
						item->window.rectClient.h = item->window.rectEffects.h;
						done++;
					}
				}
				else
				{
					item->window.rectClient.h -= item->window.rectEffects2.h;
					if (item->window.rectClient.h < item->window.rectEffects.h)
					{
						item->window.rectClient.h = item->window.rectEffects.h;
						done++;
					}
				}
			}

			Item_UpdatePosition(item);

			if (done == 4)
			{
				item->window.flags &= ~WINDOW_INTRANSITION;
			}

		}
	}

#ifdef _TRANS3

//JLF begin model transition stuff
	if (item->window.flags & WINDOW_INTRANSITIONMODEL)
	{
		if ( item->type == ITEM_TYPE_MODEL)
		{
//fields ing modelptr
//				vec3_t g2mins2, g2maxs2, g2minsEffect, g2maxsEffect;
//	float fov_x2, fov_y2, fov_Effectx, fov_Effecty;

			modelDef_t  * modelptr = (modelDef_t *)item->typeData;

			if (DC->realTime > item->window.nextTime)
			{
				int done = 0;
				item->window.nextTime = DC->realTime + item->window.offsetTime;


// transition the x,y,z max
				if (modelptr->g2maxs[0] == modelptr->g2maxs2[0])
				{
					done++;
				}
				else
				{
					if (modelptr->g2maxs[0] < modelptr->g2maxs2[0])
					{
						modelptr->g2maxs[0] += modelptr->g2maxsEffect[0];
						if (modelptr->g2maxs[0] > modelptr->g2maxs2[0])
						{
							modelptr->g2maxs[0] = modelptr->g2maxs2[0];
							done++;
						}
					}
					else
					{
						modelptr->g2maxs[0] -= modelptr->g2maxsEffect[0];
						if (modelptr->g2maxs[0] < modelptr->g2maxs2[0])
						{
							modelptr->g2maxs[0] = modelptr->g2maxs2[0];
							done++;
						}
					}
				}
//y
				if (modelptr->g2maxs[1] == modelptr->g2maxs2[1])
				{
					done++;
				}
				else
				{
					if (modelptr->g2maxs[1] < modelptr->g2maxs2[1])
					{
						modelptr->g2maxs[1] += modelptr->g2maxsEffect[1];
						if (modelptr->g2maxs[1] > modelptr->g2maxs2[1])
						{
							modelptr->g2maxs[1] = modelptr->g2maxs2[1];
							done++;
						}
					}
					else
					{
						modelptr->g2maxs[1] -= modelptr->g2maxsEffect[1];
						if (modelptr->g2maxs[1] < modelptr->g2maxs2[1])
						{
							modelptr->g2maxs[1] = modelptr->g2maxs2[1];
							done++;
						}
					}
				}


//z

				if (modelptr->g2maxs[2] == modelptr->g2maxs2[2])
				{
					done++;
				}
				else
				{
					if (modelptr->g2maxs[2] < modelptr->g2maxs2[2])
					{
						modelptr->g2maxs[2] += modelptr->g2maxsEffect[2];
						if (modelptr->g2maxs[2] > modelptr->g2maxs2[2])
						{
							modelptr->g2maxs[2] = modelptr->g2maxs2[2];
							done++;
						}
					}
					else
					{
						modelptr->g2maxs[2] -= modelptr->g2maxsEffect[2];
						if (modelptr->g2maxs[2] < modelptr->g2maxs2[2])
						{
							modelptr->g2maxs[2] = modelptr->g2maxs2[2];
							done++;
						}
					}
				}

// transition the x,y,z min
				if (modelptr->g2mins[0] == modelptr->g2mins2[0])
				{
					done++;
				}
				else
				{
					if (modelptr->g2mins[0] < modelptr->g2mins2[0])
					{
						modelptr->g2mins[0] += modelptr->g2minsEffect[0];
						if (modelptr->g2mins[0] > modelptr->g2mins2[0])
						{
							modelptr->g2mins[0] = modelptr->g2mins2[0];
							done++;
						}
					}
					else
					{
						modelptr->g2mins[0] -= modelptr->g2minsEffect[0];
						if (modelptr->g2mins[0] < modelptr->g2mins2[0])
						{
							modelptr->g2mins[0] = modelptr->g2mins2[0];
							done++;
						}
					}
				}
//y
				if (modelptr->g2mins[1] == modelptr->g2mins2[1])
				{
					done++;
				}
				else
				{
					if (modelptr->g2mins[1] < modelptr->g2mins2[1])
					{
						modelptr->g2mins[1] += modelptr->g2minsEffect[1];
						if (modelptr->g2mins[1] > modelptr->g2mins2[1])
						{
							modelptr->g2mins[1] = modelptr->g2mins2[1];
							done++;
						}
					}
					else
					{
						modelptr->g2mins[1] -= modelptr->g2minsEffect[1];
						if (modelptr->g2mins[1] < modelptr->g2mins2[1])
						{
							modelptr->g2mins[1] = modelptr->g2mins2[1];
							done++;
						}
					}
				}


//z

				if (modelptr->g2mins[2] == modelptr->g2mins2[2])
				{
					done++;
				}
				else
				{
					if (modelptr->g2mins[2] < modelptr->g2mins2[2])
					{
						modelptr->g2mins[2] += modelptr->g2minsEffect[2];
						if (modelptr->g2mins[2] > modelptr->g2mins2[2])
						{
							modelptr->g2mins[2] = modelptr->g2mins2[2];
							done++;
						}
					}
					else
					{
						modelptr->g2mins[2] -= modelptr->g2minsEffect[2];
						if (modelptr->g2mins[2] < modelptr->g2mins2[2])
						{
							modelptr->g2mins[2] = modelptr->g2mins2[2];
							done++;
						}
					}
				}



//fovx
				if (modelptr->fov_x == modelptr->fov_x2)
				{
					done++;
				}
				else
				{
					if (modelptr->fov_x < modelptr->fov_x2)
					{
						modelptr->fov_x += modelptr->fov_Effectx;
						if (modelptr->fov_x > modelptr->fov_x2)
						{
							modelptr->fov_x = modelptr->fov_x2;
							done++;
						}
					}
					else
					{
						modelptr->fov_x -= modelptr->fov_Effectx;
						if (modelptr->fov_x < modelptr->fov_x2)
						{
							modelptr->fov_x = modelptr->fov_x2;
							done++;
						}
					}
				}

//fovy
				if (modelptr->fov_y == modelptr->fov_y2)
				{
					done++;
				}
				else
				{
					if (modelptr->fov_y < modelptr->fov_y2)
					{
						modelptr->fov_y += modelptr->fov_Effecty;
						if (modelptr->fov_y > modelptr->fov_y2)
						{
							modelptr->fov_y = modelptr->fov_y2;
							done++;
						}
					}
					else
					{
						modelptr->fov_y -= modelptr->fov_Effecty;
						if (modelptr->fov_y < modelptr->fov_y2)
						{
							modelptr->fov_y = modelptr->fov_y2;
							done++;
						}
					}
				}

				if (done == 5)
				{
					item->window.flags &= ~WINDOW_INTRANSITIONMODEL;
				}

			}
		}
	}
#endif
//JLF end transition stuff for models

	if (item->window.ownerDrawFlags && DC->ownerDrawVisible)
	{
		if (!DC->ownerDrawVisible(item->window.ownerDrawFlags))
		{
			item->window.flags &= ~WINDOW_VISIBLE;
		}
		else
		{
			item->window.flags |= WINDOW_VISIBLE;
		}
	}

	if (item->cvarFlags & (CVAR_SHOW | CVAR_HIDE))
	{
		if (!Item_EnableShowViaCvar(item, CVAR_SHOW))
		{
			return qfalse;
		}
	}


	if (item->window.flags & WINDOW_TIMEDVISIBLE)
	{

	}

	if (!(item->window.flags & WINDOW_VISIBLE))
	{
		return qfalse;
	}

	if (!bDraw)
	{
		return qtrue;
	}
	//okay to paint
	//JLFMOUSE

	if (item->window.flags & WINDOW_MOUSEOVER)
	{
		if (item->descText && !Display_KeyBindPending())
		{
			// Make DOUBLY sure that this item should have desctext.
			// NOTE : we can't just check the mouse position on this, what if we TABBED
			// to the current menu item -- in that case our mouse isn't over the item.
			// Removing the WINDOW_MOUSEOVER flag just prevents the item's OnExit script from running
		//	if (!Rect_ContainsPoint(&item->window.rect, DC->cursorx, DC->cursory))
		//	{	// It isn't something that should, because it isn't live anymore.
		//		item->window.flags &= ~WINDOW_MOUSEOVER;
		//	}
		//	else

	//END JLFMOUSE

			// items can be enabled and disabled based on cvars
			if( !(item->cvarFlags & (CVAR_ENABLE | CVAR_DISABLE) && !Item_EnableShowViaCvar(item, CVAR_ENABLE)) )
			{	// Draw the desctext
				const char *textPtr = item->descText;
				if (*textPtr == '@')	// string reference
				{
					textPtr = SE_GetString( &textPtr[1] );
				}

				vec4_t color = {1, 1, 1, 1};
				Item_TextColor(item, &color);

				float fDescScale = parent->descScale ? parent->descScale : 1;
				float fDescScaleCopy = fDescScale;
				while (1)
				{
					// FIXME - add some type of parameter in the menu file like descfont to specify the font for the descriptions for this menu.
					textWidth = DC->textWidth(textPtr, fDescScale, 4);	//  item->font);

					if (parent->descAlignment == ITEM_ALIGN_RIGHT)
					{
						xPos = parent->descX - textWidth;	// Right justify
					}
					else if (parent->descAlignment == ITEM_ALIGN_CENTER)
					{
						xPos = parent->descX - (textWidth/2);	// Center justify
					}
					else										// Left justify
					{
						xPos = parent->descX;
					}

					if (parent->descAlignment == ITEM_ALIGN_CENTER)
					{
						// only this one will auto-shrink the scale until we eventually fit...
						//
						if (xPos + textWidth > (SCREEN_WIDTH-4)) {
							fDescScale -= 0.001f;
							continue;
						}
					}


					// Try to adjust it's y placement if the scale has changed...
					//
					int iYadj = 0;
					if (fDescScale != fDescScaleCopy)
					{
						int iOriginalTextHeight = DC->textHeight(textPtr, fDescScaleCopy, uiInfo.uiDC.Assets.qhMediumFont);
						iYadj = iOriginalTextHeight - DC->textHeight(textPtr, fDescScale, uiInfo.uiDC.Assets.qhMediumFont);
					}

					// FIXME - add some type of parameter in the menu file like descfont to specify the font for the descriptions for this menu.
					DC->drawText(xPos, parent->descY + iYadj, fDescScale, parent->descColor, textPtr, 0, parent->descTextStyle, 4);	//item->font);
					break;
				}
			}
		}
	}

	// paint the rect first..
	Window_Paint(&item->window, parent->fadeAmount , parent->fadeClamp, parent->fadeCycle);

	// Print a box showing the extents of the rectangle, when in debug mode
	if (uis.debugMode)
	{
		vec4_t color;
		color[1] = color[3] = 1;
		color[0] = color[2] = 0;
		DC->drawRect(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			1,
			color);
	}

  //DC->drawRect(item->window.rect.x, item->window.rect.y, item->window.rect.w, item->window.rect.h, 1, red);

	switch (item->type)
	{
		case ITEM_TYPE_OWNERDRAW:
			Item_OwnerDraw_Paint(item);
			break;

		case ITEM_TYPE_TEXT:
		case ITEM_TYPE_BUTTON:
			Item_Text_Paint(item);
			break;
		case ITEM_TYPE_RADIOBUTTON:
			break;
		case ITEM_TYPE_CHECKBOX:
			break;
		case ITEM_TYPE_EDITFIELD:
		case ITEM_TYPE_NUMERICFIELD:
			Item_TextField_Paint(item);
			break;
		case ITEM_TYPE_COMBO:
			break;
		case ITEM_TYPE_LISTBOX:
			Item_ListBox_Paint(item);
			break;
		case ITEM_TYPE_TEXTSCROLL:
			Item_TextScroll_Paint ( item );
			break;
		case ITEM_TYPE_MODEL:
			Item_Model_Paint(item);
			break;
		case ITEM_TYPE_YESNO:
			Item_YesNo_Paint(item);
			break;
		case ITEM_TYPE_MULTI:
			Item_Multi_Paint(item);
			break;
		case ITEM_TYPE_BIND:
			Item_Bind_Paint(item);
			break;
		case ITEM_TYPE_SLIDER:
			Item_Slider_Paint(item);
			break;
		default:
			break;
	}
	return qtrue;
}

/*
=================
LerpColor
=================
*/
void LerpColor(vec4_t a, vec4_t b, vec4_t c, float t)
{
	int i;

	// lerp and clamp each component
	for (i=0; i<4; i++)
	{
		c[i] = a[i] + t*(b[i]-a[i]);
		if (c[i] < 0)
		{
			c[i] = 0;
		}
		else if (c[i] > 1.0)
		{
			c[i] = 1.0;
		}
	}
}

/*
=================
Fade
=================
*/
void Fade(int *flags, float *f, float clamp, int *nextTime, int offsetTime, qboolean bFlags, float fadeAmount)
{
	if (*flags & (WINDOW_FADINGOUT | WINDOW_FADINGIN))
	{
		if (DC->realTime > *nextTime)
		{
			*nextTime = DC->realTime + offsetTime;
			if (*flags & WINDOW_FADINGOUT)
			{
				*f -= fadeAmount;
				if (bFlags && *f <= 0.0)
				{
					*flags &= ~(WINDOW_FADINGOUT | WINDOW_VISIBLE);
				}
			}
			else
			{
				*f += fadeAmount;
				if (*f >= clamp)
				{
				  *f = clamp;
					if (bFlags)
					{
						*flags &= ~WINDOW_FADINGIN;
					}
				}
			}
		}
	}
}

/*
=================
GradientBar_Paint
=================
*/
void GradientBar_Paint(rectDef_t *rect, vec4_t color)
{
	// gradient bar takes two paints
	DC->setColor( color );
	DC->drawHandlePic(rect->x, rect->y, rect->w, rect->h, DC->Assets.gradientBar);
	DC->setColor( NULL );
}

/*
=================
Window_Paint
=================
*/
void Window_Paint(Window *w, float fadeAmount, float fadeClamp, float fadeCycle)
{
  //float bordersize = 0;
  vec4_t color;
  rectDef_t fillRect = w->rect;


	if (uis.debugMode)
	{
		color[0] = color[1] = color[2] = color[3] = 1;
		DC->drawRect(w->rect.x, w->rect.y, w->rect.w, w->rect.h, 1, color);
	}

	if (w == NULL || (w->style == 0 && w->border == 0))
	{
		return;
	}

	if (w->border != 0)
	{
		fillRect.x += w->borderSize;
		fillRect.y += w->borderSize;
		fillRect.w -= w->borderSize + 1;
		fillRect.h -= w->borderSize + 1;
	}

	if (w->style == WINDOW_STYLE_FILLED)
	{
		// box, but possible a shader that needs filled
		if (w->background)
		{
			Fade(&w->flags, &w->backColor[3], fadeClamp, &w->nextTime, fadeCycle, qtrue, fadeAmount);
			DC->setColor(w->backColor);
			DC->drawHandlePic(fillRect.x, fillRect.y, fillRect.w, fillRect.h, w->background);
			DC->setColor(NULL);
		}
		else
		{
			DC->fillRect(fillRect.x, fillRect.y, fillRect.w, fillRect.h, w->backColor);
		}
	}
	else if (w->style == WINDOW_STYLE_GRADIENT)
	{
		GradientBar_Paint(&fillRect, w->backColor);
		// gradient bar
	}
	else if (w->style == WINDOW_STYLE_SHADER)
	{
		if (w->flags & WINDOW_PLAYERCOLOR)
		{
			vec4_t	color;
			color[0] = ui_char_color_red.integer/255.0f;
			color[1] = ui_char_color_green.integer/255.0f;
			color[2] = ui_char_color_blue.integer/255.0f;
			color[3] = 1;
			ui.R_SetColor(color);
		}
		else if (w->flags & WINDOW_FORECOLORSET)
		{
			DC->setColor(w->foreColor);
		}
		DC->drawHandlePic(fillRect.x, fillRect.y, fillRect.w, fillRect.h, w->background);
		DC->setColor(NULL);
	}

	if (w->border == WINDOW_BORDER_FULL)
	{
		// full
		// HACK HACK HACK
		if (w->style == WINDOW_STYLE_TEAMCOLOR)
		{
			if (color[0] > 0)
			{
				// red
				color[0] = 1;
				color[1] = color[2] = .5;
			}
			else
			{
				color[2] = 1;
				color[0] = color[1] = .5;
			}
			color[3] = 1;
			DC->drawRect(w->rect.x, w->rect.y, w->rect.w, w->rect.h, w->borderSize, color);
		}
		else
		{
			DC->drawRect(w->rect.x, w->rect.y, w->rect.w, w->rect.h, w->borderSize, w->borderColor);
		}
	}
	else if (w->border == WINDOW_BORDER_HORZ)
	{
		// top/bottom
		DC->setColor(w->borderColor);
		DC->drawTopBottom(w->rect.x, w->rect.y, w->rect.w, w->rect.h, w->borderSize);
		DC->setColor( NULL );
	}
	else if (w->border == WINDOW_BORDER_VERT)
	{
		// left right
		DC->setColor(w->borderColor);
		DC->drawSides(w->rect.x, w->rect.y, w->rect.w, w->rect.h, w->borderSize);
		DC->setColor( NULL );
	}
	else if (w->border == WINDOW_BORDER_KCGRADIENT)
	{
		// this is just two gradient bars along each horz edge
		rectDef_t r = w->rect;
		r.h = w->borderSize;
		GradientBar_Paint(&r, w->borderColor);
		r.y = w->rect.y + w->rect.h - 1;
		GradientBar_Paint(&r, w->borderColor);
	}
}

/*
=================
Display_KeyBindPending
=================
*/
qboolean Display_KeyBindPending(void)
{
	return g_waitingForKey;
}

/*
=================
ToWindowCoords
=================
*/
void ToWindowCoords(float *x, float *y, windowDef_t *window)
{
	if (window->border != 0)
	{
		*x += window->borderSize;
		*y += window->borderSize;
	}
	*x += window->rect.x;
	*y += window->rect.y;
}

/*
=================
Item_Text_AutoWrapped_Paint
=================
*/
void Item_Text_AutoWrapped_Paint(itemDef_t *item)
{
	char text[1024];
	const char *p, *textPtr, *newLinePtr;
	char buff[1024];
	int height, len, textWidth, newLine, newLineWidth;
	float y;
	vec4_t color;

	textWidth = 0;
	newLinePtr = NULL;

	if (item->text == NULL)
	{
		if (item->cvar == NULL)
		{
			return;
		}
		else
		{
			DC->getCVarString(item->cvar, text, sizeof(text));
			textPtr = text;
		}
	}
	else
	{
		textPtr = item->text;
	}
	if (*textPtr == '@')	// string reference
	{
		textPtr = SE_GetString( &textPtr[1] );
	}

	if (*textPtr == '\0')
	{
		return;
	}
	Item_TextColor(item, &color);
	//Item_SetTextExtents(item, &width, &height, textPtr);
	if (item->value == 0)
	{
		item->value = (int)(0.5 + (float)DC->textWidth(textPtr, item->textscale, item->font) / item->window.rect.w);
	}
	height = DC->textHeight(textPtr, item->textscale, item->font);
	item->special = 0;

	y = item->textaligny;
	len = 0;
	buff[0] = '\0';
	newLine = 0;
	newLineWidth = 0;
	p = textPtr;
	int line = 1;
	while (1) //findmeste (this will break widechar languages)!
	{
		if (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\0')
		{
			newLine = len;
			newLinePtr = p+1;
			newLineWidth = textWidth;
		}
		textWidth = DC->textWidth(buff, item->textscale, 0);
		if ( (newLine && textWidth >= item->window.rect.w - item->textalignx) || *p == '\n' || *p == '\0')
		{
			if (line > item->cursorPos)	//scroll
		{
			if (len)
			{
				if (item->textalignment == ITEM_ALIGN_LEFT)
				{
					item->textRect.x = item->textalignx;
				}
				else if (item->textalignment == ITEM_ALIGN_RIGHT)
				{
					item->textRect.x = item->textalignx - newLineWidth;
				}
				else if (item->textalignment == ITEM_ALIGN_CENTER)
				{
					item->textRect.x = item->textalignx - newLineWidth / 2;
				}
				item->textRect.y = y;
				ToWindowCoords(&item->textRect.x, &item->textRect.y, &item->window);
				//
				buff[newLine] = '\0';

					if ( *p && y + height + 4 > item->window.rect.h - height)
					{
						item->special = 1;
						strcat(buff,"...");//uhh, let's render some ellipses
					}
				DC->drawText(item->textRect.x, item->textRect.y, item->textscale, color, buff, 0, item->textStyle, item->font);
			}
				y += height + 4;
				if ( y > item->window.rect.h - height)
				{//reached the bottom of the box, so stop
					break;
				}
				len = 0;
			}
			else
			{
				strcpy(buff,"...");
				len = 3;
			}
			if (*p == '\0')
			{	//end of string
				break;
			}
			p = newLinePtr;
			newLine = 0;
			newLineWidth = 0;
			line++;
		}
		buff[len++] = *p++;
		buff[len] = '\0';
	}
	item->textRect = item->window.rect;
}

/*
=================
Rect_ContainsPoint
=================
*/
static qboolean Rect_ContainsPoint(rectDef_t *rect, float x, float y)
{
	//JLF  ignore mouse pointer location
//	return true;
	// END JLF
	if (rect)
	{
//		if ((x > rect->x) && (x < (rect->x + rect->w)) && (y > rect->y) && (y < (rect->y + rect->h)))
		if ((x > rect->x) && (x < (rect->x + rect->w)))
		{
			if ((y > rect->y) && (y < (rect->y + rect->h)))
			{
				return qtrue;
			}
		}
	}
	return qfalse;
}

qboolean Item_TextScroll_HandleKey ( itemDef_t *item, int key, qboolean down, qboolean force)
{
	textScrollDef_t *scrollPtr = (textScrollDef_t*)item->typeData;
	int				max;
	int				viewmax;

	if (force || (Rect_ContainsPoint(&item->window.rect, DC->cursorx, DC->cursory) && item->window.flags & WINDOW_HASFOCUS))
	{
		max = Item_TextScroll_MaxScroll(item);

		viewmax = (item->window.rect.h / scrollPtr->lineHeight);
		if ( key == A_CURSOR_UP || key == A_KP_8 )
		{
			scrollPtr->startPos--;
			if (scrollPtr->startPos < 0)
			{
				scrollPtr->startPos = 0;
			}
			return qtrue;
		}

		if ( key == A_CURSOR_DOWN || key == A_KP_2 )
		{
			scrollPtr->startPos++;
			if (scrollPtr->startPos > max)
			{
				scrollPtr->startPos = max;
			}

			return qtrue;
		}

		//Raz: Added
		if ( key == A_MWHEELUP )
		{
			scrollPtr->startPos--;
			if (scrollPtr->startPos < 0)
			{
				scrollPtr->startPos = 0;
				Display_MouseMove(NULL, DC->cursorx, DC->cursory);
				return qfalse;
			}
			Display_MouseMove(NULL, DC->cursorx, DC->cursory);
			return qtrue;
		}
		if ( key == A_MWHEELDOWN )
		{
			scrollPtr->startPos++;
			if (scrollPtr->startPos > max)
			{
				scrollPtr->startPos = max;
				Display_MouseMove(NULL, DC->cursorx, DC->cursory);
				return qfalse;
			}
			Display_MouseMove(NULL, DC->cursorx, DC->cursory);
			return qtrue;
		}

		// mouse hit
		if (key == A_MOUSE1 || key == A_MOUSE2)
		{
			if (item->window.flags & WINDOW_LB_LEFTARROW)
			{
				scrollPtr->startPos--;
				if (scrollPtr->startPos < 0)
				{
					scrollPtr->startPos = 0;
				}
			}
			else if (item->window.flags & WINDOW_LB_RIGHTARROW)
			{
				// one down
				scrollPtr->startPos++;
				if (scrollPtr->startPos > max)
				{
					scrollPtr->startPos = max;
				}
			}
			else if (item->window.flags & WINDOW_LB_PGUP)
			{
				// page up
				scrollPtr->startPos -= viewmax;
				if (scrollPtr->startPos < 0)
				{
					scrollPtr->startPos = 0;
				}
			}
			else if (item->window.flags & WINDOW_LB_PGDN)
			{
				// page down
				scrollPtr->startPos += viewmax;
				if (scrollPtr->startPos > max)
				{
					scrollPtr->startPos = max;
				}
			}
			else if (item->window.flags & WINDOW_LB_THUMB)
			{
				// Display_SetCaptureItem(item);
			}

			return qtrue;
		}

		if ( key == A_HOME || key == A_KP_7)
		{
			// home
			scrollPtr->startPos = 0;
			return qtrue;
		}
		if ( key == A_END || key == A_KP_1)
		{
			// end
			scrollPtr->startPos = max;
			return qtrue;
		}
		if (key == A_PAGE_UP || key == A_KP_9 )
		{
			scrollPtr->startPos -= viewmax;
			if (scrollPtr->startPos < 0)
			{
					scrollPtr->startPos = 0;
			}

			return qtrue;
		}
		if ( key == A_PAGE_DOWN || key == A_KP_3 )
		{
			scrollPtr->startPos += viewmax;
			if (scrollPtr->startPos > max)
			{
				scrollPtr->startPos = max;
			}
			return qtrue;
		}
	}

	return qfalse;
}

/*
=================
Item_ListBox_MaxScroll
=================
*/
int Item_ListBox_MaxScroll(itemDef_t *item)
{
	listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;
	int count = DC->feederCount(item->special);
	int max;

	if (item->window.flags & WINDOW_HORIZONTAL)
	{
		max = count - (item->window.rect.w / listPtr->elementWidth) + 1;
	}
	else
	{
		max = count - (item->window.rect.h / listPtr->elementHeight) + 1;
	}

	if (max < 0)
	{
		return 0;
	}
	return max;
}

/*
=================
Item_ListBox_ThumbPosition
=================
*/
int Item_ListBox_ThumbPosition(itemDef_t *item)
{
	float max, pos, size;
	listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;

	max = Item_ListBox_MaxScroll(item);
	if (item->window.flags & WINDOW_HORIZONTAL) {

		size = item->window.rect.w - (SCROLLBAR_SIZE * 2) - 2;
		if (max > 0)
		{
			pos = (size-SCROLLBAR_SIZE) / (float) max;
		}
		else
		{
			pos = 0;
		}
		pos *= listPtr->startPos;
		return item->window.rect.x + 1 + SCROLLBAR_SIZE + pos;
	}
	else
	{
		size = item->window.rect.h - (SCROLLBAR_SIZE * 2) - 2;
		if (max > 0)
		{
			pos = (size-SCROLLBAR_SIZE) / (float) max;
		}
		else
		{
			pos = 0;
		}
		pos *= listPtr->startPos;
		return item->window.rect.y + 1 + SCROLLBAR_SIZE + pos;
	}
}

/*
=================
Item_ListBox_OverLB
=================
*/
int Item_ListBox_OverLB(itemDef_t *item, float x, float y)
{
	rectDef_t r;
	int thumbstart;

	if (item->window.flags & WINDOW_HORIZONTAL)
	{
		// check if on left arrow
		r.x = item->window.rect.x;
		r.y = item->window.rect.y + item->window.rect.h - SCROLLBAR_SIZE;
		r.h = r.w = SCROLLBAR_SIZE;
		if (Rect_ContainsPoint(&r, x, y))
		{
			return WINDOW_LB_LEFTARROW;
		}
		// check if on right arrow
		r.x = item->window.rect.x + item->window.rect.w - SCROLLBAR_SIZE;
		if (Rect_ContainsPoint(&r, x, y))
		{
			return WINDOW_LB_RIGHTARROW;
		}
		// check if on thumb
		thumbstart = Item_ListBox_ThumbPosition(item);
		r.x = thumbstart;
		if (Rect_ContainsPoint(&r, x, y))
		{
			return WINDOW_LB_THUMB;
		}
		r.x = item->window.rect.x + SCROLLBAR_SIZE;
		r.w = thumbstart - r.x;
		if (Rect_ContainsPoint(&r, x, y))
		{
			return WINDOW_LB_PGUP;
		}
		r.x = thumbstart + SCROLLBAR_SIZE;
		r.w = item->window.rect.x + item->window.rect.w - SCROLLBAR_SIZE;
		if (Rect_ContainsPoint(&r, x, y))
		{
			return WINDOW_LB_PGDN;
		}
	}
	else
	{
		r.x = item->window.rect.x + item->window.rect.w - SCROLLBAR_SIZE;
		r.y = item->window.rect.y;
		r.h = r.w = SCROLLBAR_SIZE;
		if (Rect_ContainsPoint(&r, x, y))
		{
			return WINDOW_LB_LEFTARROW;
		}
		r.y = item->window.rect.y + item->window.rect.h - SCROLLBAR_SIZE;
		if (Rect_ContainsPoint(&r, x, y))
		{
			return WINDOW_LB_RIGHTARROW;
		}
		thumbstart = Item_ListBox_ThumbPosition(item);
		r.y = thumbstart;
		if (Rect_ContainsPoint(&r, x, y))
		{
			return WINDOW_LB_THUMB;
		}
		r.y = item->window.rect.y + SCROLLBAR_SIZE;
		r.h = thumbstart - r.y;
		if (Rect_ContainsPoint(&r, x, y))
		{
			return WINDOW_LB_PGUP;
		}
		r.y = thumbstart + SCROLLBAR_SIZE;
		r.h = item->window.rect.y + item->window.rect.h - SCROLLBAR_SIZE;
		if (Rect_ContainsPoint(&r, x, y))
		{
			return WINDOW_LB_PGDN;
		}
	}
	return 0;
}

/*
=================
Item_ListBox_MouseEnter
=================
*/
void Item_ListBox_MouseEnter(itemDef_t *item, float x, float y)
{
	rectDef_t r;
	listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;

	item->window.flags &= ~(WINDOW_LB_LEFTARROW | WINDOW_LB_RIGHTARROW | WINDOW_LB_THUMB | WINDOW_LB_PGUP | WINDOW_LB_PGDN);
	item->window.flags |= Item_ListBox_OverLB(item, x, y);

	if (item->window.flags & WINDOW_HORIZONTAL)
	{
		if (!(item->window.flags & (WINDOW_LB_LEFTARROW | WINDOW_LB_RIGHTARROW | WINDOW_LB_THUMB | WINDOW_LB_PGUP | WINDOW_LB_PGDN)))
		{
			// check for selection hit as we have exausted buttons and thumb
			if (listPtr->elementStyle == LISTBOX_IMAGE)
			{
				r.x = item->window.rect.x;
				r.y = item->window.rect.y;
				r.h = item->window.rect.h - SCROLLBAR_SIZE;
				r.w = item->window.rect.w - listPtr->drawPadding;
				if (Rect_ContainsPoint(&r, x, y))
				{
					listPtr->cursorPos =  (int)((x - r.x) / listPtr->elementWidth)  + listPtr->startPos;
					if (listPtr->cursorPos > listPtr->endPos)
					{
						listPtr->cursorPos = listPtr->endPos;
					}
				}
			}
			else
			{
				// text hit..
			}
		}
	}
	else if (!(item->window.flags & (WINDOW_LB_LEFTARROW | WINDOW_LB_RIGHTARROW | WINDOW_LB_THUMB | WINDOW_LB_PGUP | WINDOW_LB_PGDN)))
	{
		r.x = item->window.rect.x;
		r.y = item->window.rect.y;
		r.w = item->window.rect.w - SCROLLBAR_SIZE;
		r.h = item->window.rect.h - listPtr->drawPadding;
		if (Rect_ContainsPoint(&r, x, y))
		{
			listPtr->cursorPos =  (int)((y - 2 - r.y) / listPtr->elementHeight)  + listPtr->startPos;
			if (listPtr->cursorPos > listPtr->endPos)
			{
				listPtr->cursorPos = listPtr->endPos;
			}
		}
	}
}

/*
=================
Item_MouseEnter
=================
*/
void Item_MouseEnter(itemDef_t *item, float x, float y)
{
	rectDef_t r;
	//JLFMOUSE
	if (item)
	{
		r = item->textRect;
//		r.y -= r.h;			// NOt sure why this is here, but I commented it out.
		// in the text rect?

		// items can be enabled and disabled based on cvars
		if (item->cvarFlags & (CVAR_ENABLE | CVAR_DISABLE) && !Item_EnableShowViaCvar(item, CVAR_ENABLE))
		{
			return;
		}

		if (item->cvarFlags & (CVAR_SHOW | CVAR_HIDE) && !Item_EnableShowViaCvar(item, CVAR_SHOW))
		{
			return;
		}

//JLFMOUSE
		if (Rect_ContainsPoint(&r, x, y))
		{
			if (!(item->window.flags & WINDOW_MOUSEOVERTEXT))
			{
				Item_RunScript(item, item->mouseEnterText);
				item->window.flags |= WINDOW_MOUSEOVERTEXT;
			}

			if (!(item->window.flags & WINDOW_MOUSEOVER))
			{
				Item_RunScript(item, item->mouseEnter);
				item->window.flags |= WINDOW_MOUSEOVER;
			}

		}
		else
		{
			// not in the text rect
			if (item->window.flags & WINDOW_MOUSEOVERTEXT)
			{
				// if we were
				Item_RunScript(item, item->mouseExitText);
				item->window.flags &= ~WINDOW_MOUSEOVERTEXT;
			}

			if (!(item->window.flags & WINDOW_MOUSEOVER))
			{
				Item_RunScript(item, item->mouseEnter);
				item->window.flags |= WINDOW_MOUSEOVER;
			}

			if (item->type == ITEM_TYPE_LISTBOX)
			{
				Item_ListBox_MouseEnter(item, x, y);
			}
			else if ( item->type == ITEM_TYPE_TEXTSCROLL )
			{
				Item_TextScroll_MouseEnter ( item, x, y );
			}
		}
	}
}



/*
=================
Item_SetFocus
=================
*/
// will optionaly set focus to this item
qboolean Item_SetFocus(itemDef_t *item, float x, float y)
{
	int i;
	itemDef_t *oldFocus;
	sfxHandle_t *sfx = &DC->Assets.itemFocusSound;
	qboolean playSound = qfalse;
	// sanity check, non-null, not a decoration and does not already have the focus
	if (item == NULL || item->window.flags & WINDOW_DECORATION || item->window.flags & WINDOW_HASFOCUS || !(item->window.flags & WINDOW_VISIBLE) || (item->window.flags & WINDOW_INACTIVE))
	{
		return qfalse;
	}
	menuDef_t *parent = (menuDef_t*)item->parent;

	// items can be enabled and disabled based on cvars
	if (item->cvarFlags & (CVAR_ENABLE | CVAR_DISABLE) && !Item_EnableShowViaCvar(item, CVAR_ENABLE))
	{
		return qfalse;
	}

	if (item->cvarFlags & (CVAR_SHOW | CVAR_HIDE) && !Item_EnableShowViaCvar(item, CVAR_SHOW))
	{
		return qfalse;
	}

	oldFocus = Menu_ClearFocus((menuDef_t *) item->parent);

	if (item->type == ITEM_TYPE_TEXT)
	{
		rectDef_t r;
		r = item->textRect;
		//r.y -= r.h;
//JLFMOUSE
		if (Rect_ContainsPoint(&r, x, y))
		{
			item->window.flags |= WINDOW_HASFOCUS;
			if (item->focusSound)
			{
				sfx = &item->focusSound;
			}
			playSound = qtrue;
		}
		else

		{
			if (oldFocus)
			{
				oldFocus->window.flags |= WINDOW_HASFOCUS;
				if (oldFocus->onFocus)
				{
					Item_RunScript(oldFocus, oldFocus->onFocus);
				}
			}
		}
	}
	else
	{
	    item->window.flags |= WINDOW_HASFOCUS;
		if (item->onFocus)
		{
			Item_RunScript(item, item->onFocus);
		}
		if (item->focusSound)
		{
			sfx = &item->focusSound;
		}
		playSound = qtrue;
	}

	if (playSound && sfx)
	{
		DC->startLocalSound( *sfx, CHAN_LOCAL_SOUND );
	}

	for (i = 0; i < parent->itemCount; i++)
	{
		if (parent->items[i] == item)
		{
			parent->cursorItem = i;
			break;
		}
	}

	return qtrue;
}


/*
=================
IsVisible
=================
*/
qboolean IsVisible(int flags)
{
  return (flags & WINDOW_VISIBLE && !(flags & WINDOW_FADINGOUT));
}

/*
=================
Item_MouseLeave
=================
*/
void Item_MouseLeave(itemDef_t *item)
{
	if (item)
	{
		if (item->window.flags & WINDOW_MOUSEOVERTEXT)
		{
			Item_RunScript(item, item->mouseExitText);
			item->window.flags &= ~WINDOW_MOUSEOVERTEXT;
		}
		Item_RunScript(item, item->mouseExit);
		item->window.flags &= ~(WINDOW_LB_RIGHTARROW | WINDOW_LB_LEFTARROW);
	}
}

/*
=================
Item_SetMouseOver
=================
*/
void Item_SetMouseOver(itemDef_t *item, qboolean focus)
{
	if (item)
	{
		if (focus)
		{
			item->window.flags |= WINDOW_MOUSEOVER;
		}
		else
		{
			item->window.flags &= ~WINDOW_MOUSEOVER;
		}
	}
}

/*
=================
Menu_HandleMouseMove
=================
*/
void Menu_HandleMouseMove(menuDef_t *menu, float x, float y)
{
	int i, pass;
	qboolean focusSet = qfalse;

	itemDef_t *overItem;
	if (menu == NULL)
	{
		return;
	}

	if (!(menu->window.flags & (WINDOW_VISIBLE | WINDOW_FORCED)))
	{
		return;
	}

	if (itemCapture)
	{
		//Item_MouseMove(itemCapture, x, y);
		return;
	}

	if (g_waitingForKey || g_editingField)
	{
		return;
	}

	// FIXME: this is the whole issue of focus vs. mouse over..
	// need a better overall solution as i don't like going through everything twice
	for (pass = 0; pass < 2; pass++)
	{
		for (i = 0; i < menu->itemCount; i++)
		{
			// turn off focus each item
			// menu->items[i].window.flags &= ~WINDOW_HASFOCUS;

			if (!(menu->items[i]->window.flags & (WINDOW_VISIBLE | WINDOW_FORCED)))
			{
				continue;
			}

			if (menu->items[i]->window.flags & WINDOW_INACTIVE)
			{
				continue;
			}

			// items can be enabled and disabled based on cvars
			if (menu->items[i]->cvarFlags & (CVAR_ENABLE | CVAR_DISABLE) && !Item_EnableShowViaCvar(menu->items[i], CVAR_ENABLE))
			{
				continue;
			}

			if (menu->items[i]->cvarFlags & (CVAR_SHOW | CVAR_HIDE) && !Item_EnableShowViaCvar(menu->items[i], CVAR_SHOW))
			{
				continue;
			}

			if (Rect_ContainsPoint(&menu->items[i]->window.rect, x, y))
			{
				if (pass == 1)
				{
					overItem = menu->items[i];
					if (overItem->type == ITEM_TYPE_TEXT && overItem->text)
					{
						if (!Rect_ContainsPoint(&overItem->window.rect, x, y))
						{
							continue;
						}
					}

					// if we are over an item
					if (IsVisible(overItem->window.flags))
					{
						// different one
						Item_MouseEnter(overItem, x, y);
						// Item_SetMouseOver(overItem, qtrue);

						// if item is not a decoration see if it can take focus
						if (!focusSet)
						{
							focusSet = Item_SetFocus(overItem, x, y);
						}
					}
				}

			} else if (menu->items[i]->window.flags & WINDOW_MOUSEOVER)
			{
				Item_MouseLeave(menu->items[i]);
				Item_SetMouseOver(menu->items[i], qfalse);
			}
		}
	}
}

/*
=================
Display_MouseMove
=================
*/
qboolean Display_MouseMove(void *p, int x, int y)
{
	int i;
	menuDef_t *menu = (menuDef_t *) p;

	if (menu == NULL)
	{
	    menu = Menu_GetFocused();
		if (menu)
		{
			if (menu->window.flags & WINDOW_POPUP)
			{
				Menu_HandleMouseMove(menu, x, y);
				return qtrue;
			}
		}

		for (i = 0; i < menuCount; i++)
		{
			Menu_HandleMouseMove(&Menus[i], x, y);
		}
	}
	else
	{
		menu->window.rect.x += x;
		menu->window.rect.y += y;
		Menu_UpdatePosition(menu);
	}
 	return qtrue;
}

/*
=================
Menus_AnyFullScreenVisible
=================
*/
qboolean Menus_AnyFullScreenVisible(void)
{
	int i;

	for (i = 0; i < menuCount; i++)
	{
		if (Menus[i].window.flags & WINDOW_VISIBLE && Menus[i].fullScreen)
		{
			return qtrue;
		}

	}
	return qfalse;
}

/*
=================
Controls_SetConfig
=================
*/
void Controls_SetConfig( void )
{
	size_t	i;

	// iterate each command, get its numeric binding
	for ( i=0; i<g_bindCount; i++ ) {
		if ( g_bindKeys[i][0] != -1 ) {
			DC->setBinding( g_bindKeys[i][0], g_bindCommands[i] );

			if ( g_bindKeys[i][1] != -1 )
				DC->setBinding( g_bindKeys[i][1], g_bindCommands[i] );
		}
	}
}

void Controls_SetDefaults( void )
{
	size_t	i;

	for ( i=0; i<g_bindCount; i++ ) {
		g_bindKeys[i][0] = -1;
		g_bindKeys[i][1] = -1;
	}
}

void Item_Bind_Ungrey(itemDef_t *item)
{
	menuDef_t *menu;
	int i;

	menu = (menuDef_t *) item->parent;
	for (i=0;i<menu->itemCount;++i)
	{
		if (menu->items[i] == item)
		{
			continue;
		}

		menu->items[i]->window.flags &= ~WINDOW_INACTIVE;
	}
}

/*
=================
Item_Bind_HandleKey
=================
*/
qboolean Item_Bind_HandleKey(itemDef_t *item, int key, qboolean down)
{
	int			id;
	int			i;
	menuDef_t *menu;

	if (key == A_MOUSE1 && Rect_ContainsPoint(&item->window.rect, DC->cursorx, DC->cursory) && !g_waitingForKey)
	{
		if (down)
		{
			g_waitingForKey = qtrue;
			g_bindItem = item;

			// Set all others in the menu to grey
			menu = (menuDef_t *) item->parent;
			for (i=0;i<menu->itemCount;++i)
			{
				if (menu->items[i] == item)
				{
					continue;
				}
				menu->items[i]->window.flags |= WINDOW_INACTIVE;
			}

		}
		return qtrue;
	}
	else if (key == A_ENTER && !g_waitingForKey)
	{
		if (down)
		{
			g_waitingForKey = qtrue;
			g_bindItem = item;

			// Set all others in the menu to grey
			menu = (menuDef_t *) item->parent;
			for (i=0;i<menu->itemCount;++i)
			{
				if (menu->items[i] == item)
				{
					continue;
				}
				menu->items[i]->window.flags |= WINDOW_INACTIVE;
			}

		}
		return qtrue;
	}
	else
	{
		if (!g_waitingForKey || g_bindItem == NULL)
		{
			return qfalse;
		}

		if (key & K_CHAR_FLAG)
		{
			return qtrue;
		}

		switch (key)
		{
			case A_ESCAPE:
				g_waitingForKey = qfalse;
				Item_Bind_Ungrey(item);
				return qtrue;

			case A_BACKSPACE:
				id = BindingIDFromName(item->cvar);
				if ( id != -1 )
				{
					if ( g_bindKeys[id][0] != -1 )
						DC->setBinding( g_bindKeys[id][0], "" );

					if ( g_bindKeys[id][1] != -1 )
						DC->setBinding( g_bindKeys[id][1], "" );

					g_bindKeys[id][0] = -1;
					g_bindKeys[id][1] = -1;
				}
				Controls_SetConfig();
				g_waitingForKey = qfalse;
				g_bindItem = NULL;
				Item_Bind_Ungrey(item);
				return qtrue;
				break;
			case '`':
				return qtrue;
		}
	}

	// Is the same key being bound to something else?
	if ( key != -1 ) {
		size_t	b;

		for ( b=0; b<g_bindCount; b++ )
		{
			if ( g_bindKeys[b][1] == key )
				g_bindKeys[b][1] = -1;

			if ( g_bindKeys[b][0] == key ) {
				g_bindKeys[b][0] = g_bindKeys[b][1];
				g_bindKeys[b][1] = -1;
			}
		}
	}

	id = BindingIDFromName( item->cvar );

	if ( id != -1 ) {
		if ( key == -1 ) {
			if ( g_bindKeys[id][0] != -1 ) {
				DC->setBinding( g_bindKeys[id][0], "" );
				g_bindKeys[id][0] = -1;
			}
			if ( g_bindKeys[id][1] != -1 ) {
				DC->setBinding( g_bindKeys[id][1], "" );
				g_bindKeys[id][1] = -1;
			}
		}
		else if ( g_bindKeys[id][0] == -1 )
			g_bindKeys[id][0] = key;
		else if ( g_bindKeys[id][0] != key && g_bindKeys[id][1] == -1 )
			g_bindKeys[id][1] = key;
		else {
			DC->setBinding( g_bindKeys[id][0], "" );
			DC->setBinding( g_bindKeys[id][1], "" );
			g_bindKeys[id][0] = key;
			g_bindKeys[id][1] = -1;
		}
	}

	Controls_SetConfig();
	g_waitingForKey = qfalse;
	Item_Bind_Ungrey(item);

	return qtrue;
}

/*
=================
Menu_SetNextCursorItem
=================
*/
itemDef_t *Menu_SetNextCursorItem(menuDef_t *menu)
{

	qboolean wrapped = qfalse;
	int oldCursor = menu->cursorItem;


	if (menu->cursorItem == -1)
	{
		menu->cursorItem = 0;
		wrapped = qtrue;
	}

	while (menu->cursorItem < menu->itemCount)
	{

		menu->cursorItem++;
		if (menu->cursorItem >= menu->itemCount && !wrapped)
		{
		  wrapped = qtrue;
		  menu->cursorItem = 0;
		}

		if (Item_SetFocus(menu->items[menu->cursorItem], DC->cursorx, DC->cursory))
		{
			Menu_HandleMouseMove(menu, menu->items[menu->cursorItem]->window.rect.x + 1, menu->items[menu->cursorItem]->window.rect.y + 1);
			return menu->items[menu->cursorItem];
		}
	}

	menu->cursorItem = oldCursor;
	return NULL;
}

/*
=================
Menu_SetPrevCursorItem
=================
*/
itemDef_t *Menu_SetPrevCursorItem(menuDef_t *menu)
{
	qboolean wrapped = qfalse;
	int oldCursor = menu->cursorItem;

	if (menu->cursorItem < 0)
	{
		menu->cursorItem = menu->itemCount-1;
		wrapped = qtrue;
	}

	while (menu->cursorItem > -1)
	{
		menu->cursorItem--;
		if (menu->cursorItem < 0 )
		{
			if (wrapped)
		{
				break;
			}
			wrapped = qtrue;
			menu->cursorItem = menu->itemCount -1;
		}

		if (Item_SetFocus(menu->items[menu->cursorItem], DC->cursorx, DC->cursory))
		{
			Menu_HandleMouseMove(menu, menu->items[menu->cursorItem]->window.rect.x + 1, menu->items[menu->cursorItem]->window.rect.y + 1);
			return menu->items[menu->cursorItem];
		}
	}
	menu->cursorItem = oldCursor;
	return NULL;

}
/*
=================
Item_TextField_HandleKey
=================
*/
qboolean Item_TextField_HandleKey(itemDef_t *item, int key)
{
	char buff[1024];
	int len;
	itemDef_t *newItem = NULL;
	editFieldDef_t *editPtr = (editFieldDef_t*)item->typeData;

	if (item->cvar)
	{

		memset(buff, 0, sizeof(buff));
		DC->getCVarString(item->cvar, buff, sizeof(buff));
		len = strlen(buff);
		if (editPtr->maxChars && len > editPtr->maxChars)
		{
			len = editPtr->maxChars;
		}

		if ( key & K_CHAR_FLAG )
		{
			key &= ~K_CHAR_FLAG;


			if (key == 'h' - 'a' + 1 )
			{	// ctrl-h is backspace
				if ( item->cursorPos > 0 )
				{
					memmove( &buff[item->cursorPos - 1], &buff[item->cursorPos], len + 1 - item->cursorPos);
					item->cursorPos--;
					if (item->cursorPos < editPtr->paintOffset)
					{
						editPtr->paintOffset--;
					}
				}
				DC->setCVar(item->cvar, buff);
	    		return qtrue;
			}

			//
			// ignore any non printable chars
			//
			if ( key < 32 || !item->cvar)
			{
			    return qtrue;
		    }

			if (item->type == ITEM_TYPE_NUMERICFIELD)
			{
				if (key < '0' || key > '9')
				{
					return qfalse;
				}
			}

			if (!DC->getOverstrikeMode())
			{
				if (( len == MAX_EDITFIELD - 1 ) || (editPtr->maxChars && len >= editPtr->maxChars))
				{
					return qtrue;
				}
				memmove( &buff[item->cursorPos + 1], &buff[item->cursorPos], len + 1 - item->cursorPos );
			}
			else
			{
				if (editPtr->maxChars && item->cursorPos >= editPtr->maxChars)
				{
					return qtrue;
				}
			}

			buff[item->cursorPos] = key;

			DC->setCVar(item->cvar, buff);

			if (item->cursorPos < len + 1)
			{
				item->cursorPos++;
				if (editPtr->maxPaintChars && item->cursorPos > editPtr->maxPaintChars)
				{
					editPtr->paintOffset++;
				}
			}

		}
		else
		{

			if ( key == A_DELETE || key == A_KP_PERIOD )
			{
				if ( item->cursorPos < len )
				{
					memmove( buff + item->cursorPos, buff + item->cursorPos + 1, len - item->cursorPos);
					DC->setCVar(item->cvar, buff);
				}
				return qtrue;
			}

			if ( key == A_CURSOR_RIGHT || key == A_KP_6 )
			{
				if (editPtr->maxPaintChars && item->cursorPos >= editPtr->maxPaintChars && item->cursorPos < len)
				{
					item->cursorPos++;
					editPtr->paintOffset++;
					return qtrue;
				}

				if (item->cursorPos < len)
				{
					item->cursorPos++;
				}
				return qtrue;
			}

			if ( key == A_CURSOR_LEFT|| key == A_KP_4 )
			{
				if ( item->cursorPos > 0 )
				{
					item->cursorPos--;
				}
				if (item->cursorPos < editPtr->paintOffset)
				{
					editPtr->paintOffset--;
				}
				return qtrue;
			}

			if ( key == A_HOME || key == A_KP_7)
			{
				item->cursorPos = 0;
				editPtr->paintOffset = 0;
				return qtrue;
			}

			if ( key == A_END || key == A_KP_1)
			{
				item->cursorPos = len;
				if(item->cursorPos > editPtr->maxPaintChars)
				{
					editPtr->paintOffset = len - editPtr->maxPaintChars;
				}
				return qtrue;
			}

			if ( key == A_INSERT || key == A_KP_0 )
			{
				DC->setOverstrikeMode(!DC->getOverstrikeMode());
				return qtrue;
			}
		}

		if (key == A_TAB || key == A_CURSOR_DOWN || key == A_KP_2)
		{
			newItem = Menu_SetNextCursorItem((menuDef_t *) item->parent);
			if (newItem && (newItem->type == ITEM_TYPE_EDITFIELD || newItem->type == ITEM_TYPE_NUMERICFIELD))
			{
				g_editItem = newItem;
			}
		}

		if (key == A_CURSOR_UP || key == A_KP_8)
		{
			newItem = Menu_SetPrevCursorItem((menuDef_t *) item->parent);
			if (newItem && (newItem->type == ITEM_TYPE_EDITFIELD || newItem->type == ITEM_TYPE_NUMERICFIELD))
			{
				g_editItem = newItem;
			}
		}

		if ( key == A_ENTER || key == A_KP_ENTER || key == A_ESCAPE)
		{
			return qfalse;
		}

		return qtrue;
	}
	return qfalse;

}

static void Scroll_TextScroll_AutoFunc (void *p)
{
	scrollInfo_t *si = (scrollInfo_t*)p;

	if (DC->realTime > si->nextScrollTime)
	{
		// need to scroll which is done by simulating a click to the item
		// this is done a bit sideways as the autoscroll "knows" that the item is a listbox
		// so it calls it directly
		Item_TextScroll_HandleKey(si->item, si->scrollKey, qtrue, qfalse);
		si->nextScrollTime = DC->realTime + si->adjustValue;
	}

	if (DC->realTime > si->nextAdjustTime)
	{
		si->nextAdjustTime = DC->realTime + SCROLL_TIME_ADJUST;
		if (si->adjustValue > SCROLL_TIME_FLOOR)
		{
			si->adjustValue -= SCROLL_TIME_ADJUSTOFFSET;
		}
	}
}

static void Scroll_TextScroll_ThumbFunc(void *p)
{
	scrollInfo_t *si = (scrollInfo_t*)p;
	rectDef_t	 r;
	int			 pos;
	int			 max;

	textScrollDef_t *scrollPtr = (textScrollDef_t*)si->item->typeData;

	if (DC->cursory != si->yStart)
	{
		r.x = si->item->window.rect.x + si->item->window.rect.w - SCROLLBAR_SIZE - 1;
		r.y = si->item->window.rect.y + SCROLLBAR_SIZE + 1;
		r.h = si->item->window.rect.h - (SCROLLBAR_SIZE*2) - 2;
		r.w = SCROLLBAR_SIZE;
		max = Item_TextScroll_MaxScroll(si->item);
		//
		pos = (DC->cursory - r.y - SCROLLBAR_SIZE/2) * max / (r.h - SCROLLBAR_SIZE);
		if (pos < 0)
		{
			pos = 0;
		}
		else if (pos > max)
		{
			pos = max;
		}

		scrollPtr->startPos = pos;
		si->yStart = DC->cursory;
	}

	if (DC->realTime > si->nextScrollTime)
	{
		// need to scroll which is done by simulating a click to the item
		// this is done a bit sideways as the autoscroll "knows" that the item is a listbox
		// so it calls it directly
		Item_TextScroll_HandleKey(si->item, si->scrollKey, qtrue, qfalse);
		si->nextScrollTime = DC->realTime + si->adjustValue;
	}

	if (DC->realTime > si->nextAdjustTime)
	{
		si->nextAdjustTime = DC->realTime + SCROLL_TIME_ADJUST;
		if (si->adjustValue > SCROLL_TIME_FLOOR)
		{
			si->adjustValue -= SCROLL_TIME_ADJUSTOFFSET;
		}
	}
}

/*
=================
Menu_OverActiveItem
=================
*/
static qboolean Menu_OverActiveItem(menuDef_t *menu, float x, float y)
{
	if (menu && menu->window.flags & (WINDOW_VISIBLE | WINDOW_FORCED))
	{
		if (Rect_ContainsPoint(&menu->window.rect, x, y))
		{
			int i;
			for (i = 0; i < menu->itemCount; i++)
			{
			// turn off focus each item
			// menu->items[i].window.flags &= ~WINDOW_HASFOCUS;

				if (!(menu->items[i]->window.flags & (WINDOW_VISIBLE | WINDOW_FORCED)))
				{
					continue;
				}

				if (menu->items[i]->window.flags & WINDOW_DECORATION)
				{
					continue;
				}

				if (Rect_ContainsPoint(&menu->items[i]->window.rect, x, y))
				{
					itemDef_t *overItem = menu->items[i];
					if (overItem->type == ITEM_TYPE_TEXT && overItem->text)
					{
						if (Rect_ContainsPoint(&overItem->window.rect, x, y))
						{
							return qtrue;
						}
						else
						{
							continue;
						}
					}
					else
					{
						return qtrue;
					}
				}
			}
		}
	}
	return qfalse;
}

/*
=================
Display_VisibleMenuCount
=================
*/
int Display_VisibleMenuCount(void)
{
	int i, count;
	count = 0;
	for (i = 0; i < menuCount; i++)
	{
		if (Menus[i].window.flags & (WINDOW_FORCED | WINDOW_VISIBLE))
		{
			count++;
		}
	}
	return count;
}

/*
=================
Window_CloseCinematic
=================
*/
static void Window_CloseCinematic(windowDef_t *window)
{
	if (window->style == WINDOW_STYLE_CINEMATIC && window->cinematic >= 0)
	{
		DC->stopCinematic(window->cinematic);
		window->cinematic = -1;
	}
}
/*
=================
Menu_CloseCinematics
=================
*/
static void Menu_CloseCinematics(menuDef_t *menu)
{
	if (menu)
	{
		int i;
		Window_CloseCinematic(&menu->window);
		for (i = 0; i < menu->itemCount; i++)
		{
			Window_CloseCinematic(&menu->items[i]->window);
			if (menu->items[i]->type == ITEM_TYPE_OWNERDRAW)
			{
				DC->stopCinematic(0-menu->items[i]->window.ownerDraw);
			}
		}
	}
}

/*
=================
Display_CloseCinematics
=================
*/
static void Display_CloseCinematics()
{
	int i;
	for (i = 0; i < menuCount; i++)
	{
		Menu_CloseCinematics(&Menus[i]);
	}
}

/*
=================
Menus_HandleOOBClick
=================
*/
void Menus_HandleOOBClick(menuDef_t *menu, int key, qboolean down)
{
	if (menu)
	{
		int i;
		// basically the behaviour we are looking for is if there are windows in the stack.. see if
		// the cursor is within any of them.. if not close them otherwise activate them and pass the
		// key on.. force a mouse move to activate focus and script stuff
		if (down && menu->window.flags & WINDOW_OOB_CLICK)
		{
			Menu_RunCloseScript(menu);
			menu->window.flags &= ~(WINDOW_HASFOCUS | WINDOW_VISIBLE);
		}

		for (i = 0; i < menuCount; i++)
		{
			if (Menu_OverActiveItem(&Menus[i], DC->cursorx, DC->cursory))
			{
				Menu_RunCloseScript(menu);
				menu->window.flags &= ~(WINDOW_HASFOCUS | WINDOW_VISIBLE);
			//	Menus_Activate(&Menus[i]);
				Menu_HandleMouseMove(&Menus[i], DC->cursorx, DC->cursory);
				Menu_HandleKey(&Menus[i], key, down);
			}
		}

		if (Display_VisibleMenuCount() == 0)
		{
			if (DC->Pause)
			{
				DC->Pause(qfalse);
			}
		}
		Display_CloseCinematics();
	}
}

/*
=================
Item_StopCapture
=================
*/
void Item_StopCapture(itemDef_t *item)
{

}

/*
=================
Item_ListBox_HandleKey
=================
*/
qboolean Item_ListBox_HandleKey(itemDef_t *item, int key, qboolean down, qboolean force)
{
	listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;
	int count = DC->feederCount(item->special);
	int max, viewmax;
	if (force || (Rect_ContainsPoint(&item->window.rect, DC->cursorx, DC->cursory) && item->window.flags & WINDOW_HASFOCUS))
	{
		max = Item_ListBox_MaxScroll(item);
		if (item->window.flags & WINDOW_HORIZONTAL)
		{
			viewmax = (item->window.rect.w / listPtr->elementWidth);
			if ( key == A_CURSOR_LEFT || key == A_KP_4 )
			{
				if (!listPtr->notselectable)
				{
					listPtr->cursorPos--;
					if (listPtr->cursorPos < 0)
					{
						listPtr->cursorPos = 0;
					}
					if (listPtr->cursorPos < listPtr->startPos)
					{
						listPtr->startPos = listPtr->cursorPos;
					}
					if (listPtr->cursorPos >= listPtr->startPos + viewmax)
					{
						listPtr->startPos = listPtr->cursorPos - viewmax + 1;
					}
					item->cursorPos = listPtr->cursorPos;

					DC->feederSelection(item->special, item->cursorPos, item);

				}
				else
				{
					listPtr->startPos--;
					if (listPtr->startPos < 0)
					{
						listPtr->startPos = 0;
					}
				}
				return qtrue;
			}
			if ( key == A_CURSOR_RIGHT || key == A_KP_6 )
			{
				if (!listPtr->notselectable)
				{
					listPtr->cursorPos++;

					if (listPtr->cursorPos < listPtr->startPos)
					{
						listPtr->startPos = listPtr->cursorPos;
					}
					if (listPtr->cursorPos >= count)
					{
						listPtr->cursorPos = count-1;
					}
					if (listPtr->cursorPos >= listPtr->startPos + viewmax)
					{
						listPtr->startPos = listPtr->cursorPos - viewmax + 1;
					}
					item->cursorPos = listPtr->cursorPos;

					DC->feederSelection(item->special, item->cursorPos, item);
				}
				else
				{
					listPtr->startPos++;

					if (listPtr->startPos >= count)
					{
						listPtr->startPos = count-1;
					}
				}
				return qtrue;
			}
		}
		else
		{
			viewmax = (item->window.rect.h / listPtr->elementHeight);
			if ( key == A_CURSOR_UP || key == A_KP_8 )
			{
				if (!listPtr->notselectable)
				{
					listPtr->cursorPos--;

					if (listPtr->cursorPos < 0)
					{
						listPtr->cursorPos = 0;
					}
					if (listPtr->cursorPos < listPtr->startPos)
					{
						listPtr->startPos = listPtr->cursorPos;
					}
					if (listPtr->cursorPos >= listPtr->startPos + viewmax)
					{
						listPtr->startPos = listPtr->cursorPos - viewmax + 1;
					}
					item->cursorPos = listPtr->cursorPos;

					DC->feederSelection(item->special, item->cursorPos, item);
				}
				else
				{
					listPtr->startPos--;

					if (listPtr->startPos < 0)
					{
						listPtr->startPos = 0;
					}
				}
				return qtrue;
			}
			if ( key == A_CURSOR_DOWN || key == A_KP_2 )
			{
				if (!listPtr->notselectable)
				{
					listPtr->cursorPos++;
					if (listPtr->cursorPos < listPtr->startPos)
					{
						listPtr->startPos = listPtr->cursorPos;
					}
					if (listPtr->cursorPos >= count)
					{
						listPtr->cursorPos = count-1;
					}
					if (listPtr->cursorPos >= listPtr->startPos + viewmax)
					{
						listPtr->startPos = listPtr->cursorPos - viewmax + 1;
					}
					item->cursorPos = listPtr->cursorPos;

					DC->feederSelection(item->special, item->cursorPos, item);

				}
				else
				{
					listPtr->startPos++;

					if (listPtr->startPos > max)
					{
						listPtr->startPos = max;
					}
				}
				return qtrue;
			}

			//Raz: Added
			if ( key == A_MWHEELUP )
			{
				listPtr->startPos -= ((int)item->special == FEEDER_Q3HEADS) ? viewmax : 1;
				if (listPtr->startPos < 0)
				{
					listPtr->startPos = 0;
					Display_MouseMove(NULL, DC->cursorx, DC->cursory);
					return qfalse;
				}
				Display_MouseMove(NULL, DC->cursorx, DC->cursory);
				return qtrue;
			}
			if ( key == A_MWHEELDOWN )
			{
				listPtr->startPos += ((int)item->special == FEEDER_Q3HEADS) ? viewmax : 1;
				if (listPtr->startPos > max)
				{
					listPtr->startPos = max;
					Display_MouseMove(NULL, DC->cursorx, DC->cursory);
					return qfalse;
				}
				Display_MouseMove(NULL, DC->cursorx, DC->cursory);
				return qtrue;
			}
		}
		// mouse hit
		if (key == A_MOUSE1 || key == A_MOUSE2)
		{
			if (item->window.flags & WINDOW_LB_LEFTARROW)
			{
	 			listPtr->startPos--;
				if (listPtr->startPos < 0)
				{
					listPtr->startPos = 0;
				}
			}
			else if (item->window.flags & WINDOW_LB_RIGHTARROW)
			{
				// one down
				listPtr->startPos++;
				if (listPtr->startPos > max)
				{
					listPtr->startPos = max;
				}
			}
			else if (item->window.flags & WINDOW_LB_PGUP)
			{
				// page up
				listPtr->startPos -= viewmax;
				if (listPtr->startPos < 0)
				{
					listPtr->startPos = 0;
				}
			}
			else if (item->window.flags & WINDOW_LB_PGDN)
			{
				// page down
				listPtr->startPos += viewmax;
				if (listPtr->startPos > max)
				{
					listPtr->startPos = max;
				}
			}
			else if (item->window.flags & WINDOW_LB_THUMB)
			{
				// Display_SetCaptureItem(item);
			}
			else
			{
				// select an item
				if (DC->realTime < lastListBoxClickTime && listPtr->doubleClick)
				{
					Item_RunScript(item, listPtr->doubleClick);
				}
				lastListBoxClickTime = DC->realTime + DOUBLE_CLICK_DELAY;
				item->cursorPos = listPtr->cursorPos;
				DC->feederSelection(item->special, item->cursorPos, item);
			}
			return qtrue;
		}
		if ( key == A_HOME || key == A_KP_7)
		{
			// home
			listPtr->startPos = 0;
			return qtrue;
		}
		if ( key == A_END || key == A_KP_1)
		{
			// end
			listPtr->startPos = max;
			return qtrue;
		}
		if (key == A_PAGE_UP || key == A_KP_9 )
		{
			// page up
			if (!listPtr->notselectable)
			{
				listPtr->cursorPos -= viewmax;
				if (listPtr->cursorPos < 0)
				{
					listPtr->cursorPos = 0;
				}
				if (listPtr->cursorPos < listPtr->startPos)
				{
					listPtr->startPos = listPtr->cursorPos;
				}
				if (listPtr->cursorPos >= listPtr->startPos + viewmax)
				{
					listPtr->startPos = listPtr->cursorPos - viewmax + 1;
				}
				item->cursorPos = listPtr->cursorPos;
				DC->feederSelection(item->special, item->cursorPos, item);
			}
			else
			{
				listPtr->startPos -= viewmax;
				if (listPtr->startPos < 0)
				{
					listPtr->startPos = 0;
				}
			}
			return qtrue;
		}
		if ( key == A_PAGE_DOWN || key == A_KP_3 )
		{
			// page down
			if (!listPtr->notselectable)
			{
				listPtr->cursorPos += viewmax;
				if (listPtr->cursorPos < listPtr->startPos)
				{
					listPtr->startPos = listPtr->cursorPos;
				}
				if (listPtr->cursorPos >= count)
				{
					listPtr->cursorPos = count-1;
				}
				if (listPtr->cursorPos >= listPtr->startPos + viewmax)
				{
					listPtr->startPos = listPtr->cursorPos - viewmax + 1;
				}
				item->cursorPos = listPtr->cursorPos;
				DC->feederSelection(item->special, item->cursorPos, item);
			}
			else
			{
				listPtr->startPos += viewmax;
				if (listPtr->startPos > max)
				{
					listPtr->startPos = max;
				}
			}
			return qtrue;
		}
	}
	return qfalse;
}

/*
=================
Scroll_ListBox_AutoFunc
=================
*/
static void Scroll_ListBox_AutoFunc(void *p)
{
	scrollInfo_t *si = (scrollInfo_t*)p;
	if (DC->realTime > si->nextScrollTime)
	{
		// need to scroll which is done by simulating a click to the item
		// this is done a bit sideways as the autoscroll "knows" that the item is a listbox
		// so it calls it directly
		Item_ListBox_HandleKey(si->item, si->scrollKey, qtrue, qfalse);
		si->nextScrollTime = DC->realTime + si->adjustValue;
	}

	if (DC->realTime > si->nextAdjustTime)
	{
		si->nextAdjustTime = DC->realTime + SCROLL_TIME_ADJUST;
		if (si->adjustValue > SCROLL_TIME_FLOOR)
		{
			si->adjustValue -= SCROLL_TIME_ADJUSTOFFSET;
		}
	}
}

/*
=================
Scroll_ListBox_ThumbFunc
=================
*/
static void Scroll_ListBox_ThumbFunc(void *p)
{
	scrollInfo_t *si = (scrollInfo_t*)p;
	rectDef_t r;
	int pos, max;

	listBoxDef_t *listPtr = (listBoxDef_t*)si->item->typeData;
	if (si->item->window.flags & WINDOW_HORIZONTAL)
	{
		if (DC->cursorx == si->xStart)
		{
			return;
		}
		r.x = si->item->window.rect.x + SCROLLBAR_SIZE + 1;
		r.y = si->item->window.rect.y + si->item->window.rect.h - SCROLLBAR_SIZE - 1;
		r.h = SCROLLBAR_SIZE;
		r.w = si->item->window.rect.w - (SCROLLBAR_SIZE*2) - 2;
		max = Item_ListBox_MaxScroll(si->item);
		//
		pos = (DC->cursorx - r.x - SCROLLBAR_SIZE/2) * max / (r.w - SCROLLBAR_SIZE);
		if (pos < 0)
		{
			pos = 0;
		}
		else if (pos > max)
		{
			pos = max;
		}
		listPtr->startPos = pos;
		si->xStart = DC->cursorx;
	}
	else if (DC->cursory != si->yStart)
	{

		r.x = si->item->window.rect.x + si->item->window.rect.w - SCROLLBAR_SIZE - 1;
		r.y = si->item->window.rect.y + SCROLLBAR_SIZE + 1;
		r.h = si->item->window.rect.h - (SCROLLBAR_SIZE*2) - 2;
		r.w = SCROLLBAR_SIZE;
		max = Item_ListBox_MaxScroll(si->item);
		//
		pos = (DC->cursory - r.y - SCROLLBAR_SIZE/2) * max / (r.h - SCROLLBAR_SIZE);
		if (pos < 0)
		{
			pos = 0;
		}
		else if (pos > max)
		{
			pos = max;
		}
		listPtr->startPos = pos;
		si->yStart = DC->cursory;
	}

	if (DC->realTime > si->nextScrollTime)
	{
		// need to scroll which is done by simulating a click to the item
		// this is done a bit sideways as the autoscroll "knows" that the item is a listbox
		// so it calls it directly
		Item_ListBox_HandleKey(si->item, si->scrollKey, qtrue, qfalse);
		si->nextScrollTime = DC->realTime + si->adjustValue;
	}

	if (DC->realTime > si->nextAdjustTime)
	{
		si->nextAdjustTime = DC->realTime + SCROLL_TIME_ADJUST;
		if (si->adjustValue > SCROLL_TIME_FLOOR)
		{
			si->adjustValue -= SCROLL_TIME_ADJUSTOFFSET;
		}
	}
}

/*
=================
Item_Slider_OverSlider
=================
*/
int Item_Slider_OverSlider(itemDef_t *item, float x, float y)
{
	rectDef_t r;

	r.x = Item_Slider_ThumbPosition(item) - (SLIDER_THUMB_WIDTH / 2);
	r.y = item->window.rect.y - 2;
	r.w = SLIDER_THUMB_WIDTH;
	r.h = SLIDER_THUMB_HEIGHT;

	if (Rect_ContainsPoint(&r, x, y))
	{
		return WINDOW_LB_THUMB;
	}
	return 0;
}

/*
=================
Scroll_Slider_ThumbFunc
=================
*/
static void Scroll_Slider_ThumbFunc(void *p)
{
	float x, value, cursorx;
	scrollInfo_t *si = (scrollInfo_t*)p;
	editFieldDef_t *editDef = (struct editFieldDef_s *) si->item->typeData;

	if (si->item->text)
	{
		x = si->item->textRect.x + si->item->textRect.w + 8;
	}
	else
	{
		x = si->item->window.rect.x;
	}

	cursorx = DC->cursorx;

	if (cursorx < x)
	{
		cursorx = x;
	}
	else if (cursorx > x + SLIDER_WIDTH)
	{
		cursorx = x + SLIDER_WIDTH;
	}
	value = cursorx - x;
	value /= SLIDER_WIDTH;
	value *= (editDef->maxVal - editDef->minVal);
	value += editDef->minVal;
	DC->setCVar(si->item->cvar, va("%f", value));
}
/*
=================
Item_StartCapture
=================
*/
void Item_StartCapture(itemDef_t *item, int key)
{
	int flags;
	switch (item->type)
	{
		case ITEM_TYPE_EDITFIELD:
		case ITEM_TYPE_NUMERICFIELD:

		case ITEM_TYPE_LISTBOX:
		{
			flags = Item_ListBox_OverLB(item, DC->cursorx, DC->cursory);
			if (flags & (WINDOW_LB_LEFTARROW | WINDOW_LB_RIGHTARROW))
			{
				scrollInfo.nextScrollTime = DC->realTime + SCROLL_TIME_START;
				scrollInfo.nextAdjustTime = DC->realTime + SCROLL_TIME_ADJUST;
				scrollInfo.adjustValue = SCROLL_TIME_START;
				scrollInfo.scrollKey = key;
				scrollInfo.scrollDir = (flags & WINDOW_LB_LEFTARROW) ? qtrue : qfalse;
				scrollInfo.item = item;
				captureData = &scrollInfo;
				captureFunc = &Scroll_ListBox_AutoFunc;
				itemCapture = item;
			}
			else if (flags & WINDOW_LB_THUMB)
			{
				scrollInfo.scrollKey = key;
				scrollInfo.item = item;
				scrollInfo.xStart = DC->cursorx;
				scrollInfo.yStart = DC->cursory;
				captureData = &scrollInfo;
				captureFunc = &Scroll_ListBox_ThumbFunc;
				itemCapture = item;
			}
			break;
		}

		case ITEM_TYPE_TEXTSCROLL:
			flags = Item_TextScroll_OverLB (item, DC->cursorx, DC->cursory);
			if (flags & (WINDOW_LB_LEFTARROW | WINDOW_LB_RIGHTARROW))
			{
				scrollInfo.nextScrollTime = DC->realTime + SCROLL_TIME_START;
				scrollInfo.nextAdjustTime = DC->realTime + SCROLL_TIME_ADJUST;
				scrollInfo.adjustValue = SCROLL_TIME_START;
				scrollInfo.scrollKey = key;
				scrollInfo.scrollDir = (flags & WINDOW_LB_LEFTARROW) ? qtrue : qfalse;
				scrollInfo.item = item;
				captureData = &scrollInfo;
				captureFunc = &Scroll_TextScroll_AutoFunc;
				itemCapture = item;
			}
			else if (flags & WINDOW_LB_THUMB)
			{
				scrollInfo.scrollKey = key;
				scrollInfo.item = item;
				scrollInfo.xStart = DC->cursorx;
				scrollInfo.yStart = DC->cursory;
				captureData = &scrollInfo;
				captureFunc = &Scroll_TextScroll_ThumbFunc;
				itemCapture = item;
			}
			break;

		case ITEM_TYPE_SLIDER:
		{
			flags = Item_Slider_OverSlider(item, DC->cursorx, DC->cursory);
			if (flags & WINDOW_LB_THUMB)
			{
				scrollInfo.scrollKey = key;
				scrollInfo.item = item;
				scrollInfo.xStart = DC->cursorx;
				scrollInfo.yStart = DC->cursory;
				captureData = &scrollInfo;
				captureFunc = &Scroll_Slider_ThumbFunc;
				itemCapture = item;
			}
			break;
		}
	}
}

/*
=================
Item_YesNo_HandleKey
=================
*/
qboolean Item_YesNo_HandleKey(itemDef_t *item, int key)
{
  if (Rect_ContainsPoint(&item->window.rect, DC->cursorx, DC->cursory) && item->window.flags & WINDOW_HASFOCUS && item->cvar)
	{
		if (key == A_MOUSE1 || key == A_ENTER || key == A_MOUSE2 || key == A_MOUSE3)
		{
			DC->setCVar(item->cvar, va("%i", !DC->getCVarValue(item->cvar)));
			return qtrue;
		}
	}

	return qfalse;

}

/*
=================
Item_Multi_FindCvarByValue
=================
*/
int Item_Multi_FindCvarByValue(itemDef_t *item)
{
	char buff[1024];
	float value = 0;
	int i;
	multiDef_t *multiPtr = (multiDef_t*)item->typeData;
	if (multiPtr)
	{
		if (multiPtr->strDef)
		{
			DC->getCVarString(item->cvar, buff, sizeof(buff));
		}
		else
		{
			value = DC->getCVarValue(item->cvar);
		}
		for (i = 0; i < multiPtr->count; i++)
		{
			if (multiPtr->strDef)
			{
				if (Q_stricmp(buff, multiPtr->cvarStr[i]) == 0)
				{
					return i;
				}
			}
			else
			{
 				if (multiPtr->cvarValue[i] == value)
				{
 					return i;
 				}
 			}
 		}
	}
	return 0;
}

/*
=================
Item_Multi_CountSettings
=================
*/
int Item_Multi_CountSettings(itemDef_t *item)
{
	multiDef_t *multiPtr = (multiDef_t*)item->typeData;
	if (multiPtr == NULL)
	{
		return 0;
	}
	return multiPtr->count;
}



/*
=================
Item_OwnerDraw_HandleKey
=================
*/
qboolean Item_OwnerDraw_HandleKey(itemDef_t *item, int key)
{
  if (item && DC->ownerDrawHandleKey)
  {
		return DC->ownerDrawHandleKey(item->window.ownerDraw, item->window.ownerDrawFlags, &item->special, key);
  }
  return qfalse;
}

/*
=================
Item_Text_HandleKey
=================
*/
qboolean Item_Text_HandleKey(itemDef_t *item, int key)
{
	if (Rect_ContainsPoint(&item->window.rect, DC->cursorx, DC->cursory) && item->window.flags & WINDOW_AUTOWRAPPED)


	{
		if (key == A_MOUSE1 || key == A_ENTER || key == A_MOUSE2 || key == A_MOUSE3)
		{
			if (key == A_MOUSE2)
			{
				item->cursorPos--;
				if ( item->cursorPos < 0 )
				{
					item->cursorPos = 0;
				}
			}
			else if (item->special)
			{
				item->cursorPos++;
				if ( item->cursorPos >= item->value )
				{
					item->cursorPos = 0;
				}
			}
			return qtrue;
		}
	}

	return qfalse;
}


/*
=================
Item_Multi_HandleKey
=================
*/
qboolean Item_Multi_HandleKey(itemDef_t *item, int key)
{
	multiDef_t *multiPtr = (multiDef_t*)item->typeData;
	if (multiPtr)
	{
		if (Rect_ContainsPoint(&item->window.rect, DC->cursorx, DC->cursory) && item->window.flags & WINDOW_HASFOCUS)
		{
			//Raz: Scroll on multi buttons!
			if (key == A_MOUSE1 || key == A_ENTER || key == A_MOUSE2 || key == A_MOUSE3 || key == A_MWHEELDOWN || key == A_MWHEELUP)
			//if (key == A_MOUSE1 || key == A_ENTER || key == A_MOUSE2 || key == A_MOUSE3)
			{
				if (item->cvar)
				{
					int current = Item_Multi_FindCvarByValue(item);
					int max = Item_Multi_CountSettings(item);

					if (key == A_MOUSE2 || key == A_MWHEELDOWN)
					{
						current--;
						if ( current < 0 )
						{
							current = max-1;
						}
					}
					else
					{
						current++;
						if ( current >= max )
						{
							current = 0;
						}
					}

					if (multiPtr->strDef)
					{
						DC->setCVar(item->cvar, multiPtr->cvarStr[current]);
					}
					else
					{
						float value = multiPtr->cvarValue[current];
						if (((float)((int) value)) == value)
						{
							DC->setCVar(item->cvar, va("%i", (int) value ));
						}
						else
						{
							DC->setCVar(item->cvar, va("%f", value ));
						}
					}
					if (item->special)
					{//its a feeder?
						DC->feederSelection(item->special, current, item);
					}
				}
				else
				{
					int max = Item_Multi_CountSettings(item);

					if (key == A_MOUSE2 || key == A_MWHEELDOWN)
					{
						item->value--;
						if ( item->value < 0 )
						{
							item->value = max;
						}
					}
					else
					{
						item->value++;
						if ( item->value >= max )
						{
							item->value = 0;
						}
					}
					if (item->special)
					{//its a feeder?
						DC->feederSelection(item->special, item->value, item);
					}
				}

				return qtrue;
			}
		}
	}
	return qfalse;
}

/*
=================
Item_Slider_HandleKey
=================
*/
qboolean Item_Slider_HandleKey(itemDef_t *item, int key, qboolean down)
{
	//DC->Print("slider handle key\n");
//JLF MPMOVED

	float x, value, width, work;

	if (item->window.flags & WINDOW_HASFOCUS && item->cvar && Rect_ContainsPoint(&item->window.rect, DC->cursorx, DC->cursory))
	{

		if (key == A_MOUSE1 || key == A_ENTER || key == A_MOUSE2 || key == A_MOUSE3)
		{
			editFieldDef_t *editDef = (editFieldDef_s *) item->typeData;
			if (editDef)
			{
				rectDef_t testRect;
				width = SLIDER_WIDTH;
				if (item->text)
				{
					x = item->textRect.x + item->textRect.w + 8;
				}
				else
				{
					x = item->window.rect.x;
				}

				testRect = item->window.rect;
				testRect.x = x;
				value = (float)SLIDER_THUMB_WIDTH / 2;
				testRect.x -= value;
				//DC->Print("slider x: %f\n", testRect.x);
				testRect.w = (SLIDER_WIDTH + (float)SLIDER_THUMB_WIDTH / 2);
				//DC->Print("slider w: %f\n", testRect.w);
				if (Rect_ContainsPoint(&testRect, DC->cursorx, DC->cursory))
				{
					work = DC->cursorx - x;
					value = work / width;
					value *= (editDef->maxVal - editDef->minVal);
					// vm fuckage
					// value = (((float)(DC->cursorx - x)/ SLIDER_WIDTH) * (editDef->maxVal - editDef->minVal));
					value += editDef->minVal;
					DC->setCVar(item->cvar, va("%f", value));
					return qtrue;
				}
			}
		}
	}

	//DC->Print("slider handle key exit\n");
	return qfalse;
}

/*
=================
Item_HandleKey
=================
*/
qboolean Item_HandleKey(itemDef_t *item, int key, qboolean down)
{

	if (itemCapture)
	{
		Item_StopCapture(itemCapture);
		itemCapture = NULL;
		captureFunc = NULL;
		captureData = NULL;
	}
	else
	{
		if (down && (key == A_MOUSE1 || key == A_MOUSE2 || key == A_MOUSE3))
		{
			Item_StartCapture(item, key);
		}
	}

	if (!down)
	{
		return qfalse;
	}

	switch (item->type)
	{
		case ITEM_TYPE_BUTTON:
			return qfalse;
			break;
		case ITEM_TYPE_RADIOBUTTON:
			return qfalse;
			break;
		case ITEM_TYPE_CHECKBOX:
			return qfalse;
			break;
		case ITEM_TYPE_EDITFIELD:
		case ITEM_TYPE_NUMERICFIELD:
			//return Item_TextField_HandleKey(item, key);
			return qfalse;
			break;
		case ITEM_TYPE_COMBO:
			return qfalse;
			break;
		case ITEM_TYPE_LISTBOX:
			return Item_ListBox_HandleKey(item, key, down, qfalse);
			break;
		case ITEM_TYPE_TEXTSCROLL:
			return Item_TextScroll_HandleKey(item, key, down, qfalse);
			break;
		case ITEM_TYPE_YESNO:
			return Item_YesNo_HandleKey(item, key);
			break;
		case ITEM_TYPE_MULTI:
			return Item_Multi_HandleKey(item, key);
			break;
		case ITEM_TYPE_OWNERDRAW:
			return Item_OwnerDraw_HandleKey(item, key);
			break;
		case ITEM_TYPE_BIND:
			return Item_Bind_HandleKey(item, key, down);
			break;
		case ITEM_TYPE_SLIDER:
			return Item_Slider_HandleKey(item, key, down);
			break;
//JLF MPMOVED
		case ITEM_TYPE_TEXT:
			return Item_Text_HandleKey(item, key);
			break;
		default:
			return qfalse;
		break;
	}

  //return qfalse;
}



//JLFACCEPT MPMOVED
/*
-----------------------------------------
Item_HandleAccept
	If Item has an accept script, run it.
-------------------------------------------
*/
qboolean Item_HandleAccept(itemDef_t * item)
{
	if (item->accept)
	{
		Item_RunScript(item, item->accept);
		return true;
	}
	return false;
}


//JLFDPADSCRIPT MPMOVED
/*
-----------------------------------------
Item_HandleSelectionNext
	If Item has an selectionNext script, run it.
-------------------------------------------
*/
qboolean Item_HandleSelectionNext(itemDef_t * item)
{
	if (item->selectionNext)
	{
		Item_RunScript(item, item->selectionNext);
		return true;
	}
	return false;
}

//JLFDPADSCRIPT MPMOVED
/*
-----------------------------------------
Item_HandleSelectionPrev
	If Item has an selectionPrev script, run it.
-------------------------------------------
*/
qboolean Item_HandleSelectionPrev(itemDef_t * item)
{
	if (item->selectionPrev)
	{
		Item_RunScript(item, item->selectionPrev);
		return true;
	}
	return false;
}


/*
=================
Item_Action
=================
*/
void Item_Action(itemDef_t *item)
{
	if (item)
	{
		Item_RunScript(item, item->action);
	}
}

/*
=================
Menu_HandleKey
=================
*/
void Menu_HandleKey(menuDef_t *menu, int key, qboolean down)
{
	int i;
	itemDef_t *item = NULL;
	qboolean inHandler = qfalse;

	if (inHandler)
	{
		return;
	}

	inHandler = qtrue;
	if (g_waitingForKey && down)
	{
		Item_Bind_HandleKey(g_bindItem, key, down);
		inHandler = qfalse;
		return;
	}

	if (g_editingField && down)
	{
		if (!Item_TextField_HandleKey(g_editItem, key))
		{
			g_editingField = qfalse;
			g_editItem = NULL;
			inHandler = qfalse;
			return;
		}
		else if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_MOUSE3)
		{
			g_editingField = qfalse;
			g_editItem = NULL;
			Display_MouseMove(NULL, DC->cursorx, DC->cursory);
		}
		else if (key == A_TAB || key == A_CURSOR_UP || key == A_CURSOR_DOWN)
		{
			return;
		}
	}

	if (menu == NULL)
	{
		inHandler = qfalse;
		return;
	}

	//JLFMOUSE  MPMOVED
		// see if the mouse is within the window bounds and if so is this a mouse click
	if (down && !(menu->window.flags & WINDOW_POPUP) && !Rect_ContainsPoint(&menu->window.rect, DC->cursorx, DC->cursory))
	{
		static qboolean inHandleKey = qfalse;
		if (!inHandleKey && (key == A_MOUSE1 || key == A_MOUSE2 || key == A_MOUSE3))
		{
			inHandleKey = qtrue;
			Menus_HandleOOBClick(menu, key, down);
			inHandleKey = qfalse;
			inHandler = qfalse;
			return;
		}
	}

	// get the item with focus
	for (i = 0; i < menu->itemCount; i++)
	{
		if (menu->items[i]->window.flags & WINDOW_HASFOCUS)
		{
			item = menu->items[i];
			break;
		}
	}

	if (item != NULL)
	{
		if (Item_HandleKey(item, key, down))
//JLFLISTBOX
		{
			Item_Action(item);
			inHandler = qfalse;
			return;
		}

	}

	if (!down)
	{
		inHandler = qfalse;
		return;
	}

	// Special Data Pad key handling (gotta love the datapad)
	if (!(key & K_CHAR_FLAG) )
	{	//only check keys not chars
		char	b[256];
		DC->getBindingBuf( key, b, 256 );
		if (Q_stricmp(b,"datapad") == 0)	// They hit the datapad key again.
		{
			if (( Q_stricmp(menu->window.name,"datapadMissionMenu") == 0) ||
			 (Q_stricmp(menu->window.name,"datapadWeaponsMenu") == 0) ||
			 (Q_stricmp(menu->window.name,"datapadForcePowersMenu") == 0) ||
			 (Q_stricmp(menu->window.name,"datapadInventoryMenu") == 0))
			{
				key = A_ESCAPE;	//pop on outta here
			}
		}
	}
	// default handling
	switch ( key )
	{
		case A_F11:
			if (DC->getCVarValue("developer"))
			{
				uis.debugMode ^= 1;
			}
			break;

		case A_F12:
			if (DC->getCVarValue("developer"))
			{
				DC->executeText(EXEC_APPEND, "screenshot\n");
			}
			break;
		case A_KP_8:
		case A_CURSOR_UP:
			Menu_SetPrevCursorItem(menu);
			break;


		case A_ESCAPE:
			if (!g_waitingForKey && menu->onESC)
			{
				itemDef_t it;
				it.parent = menu;
				Item_RunScript(&it, menu->onESC);
			}
			break;
		case A_TAB:
		case A_KP_2:
		case A_CURSOR_DOWN:
			Menu_SetNextCursorItem(menu);
			break;

		case A_MOUSE1:
		case A_MOUSE2:
			if (item)
			{
				if (item->type == ITEM_TYPE_TEXT)
				{
					if (Rect_ContainsPoint(&item->window.rect, DC->cursorx, DC->cursory))
					{
						if ( item->action )
							Item_Action(item);
						else
						{
							if (menu->onAccept)
							{
								itemDef_t it;
								it.parent = menu;
								Item_RunScript(&it, menu->onAccept);
							}
						}

					}
				}
				else if (item->type == ITEM_TYPE_EDITFIELD || item->type == ITEM_TYPE_NUMERICFIELD)
				{
					if (Rect_ContainsPoint(&item->window.rect, DC->cursorx, DC->cursory))
					{
						item->cursorPos = 0;
						g_editingField = qtrue;
						g_editItem = item;
						//DC->setOverstrikeMode(qtrue);
					}
				}
//JLFACCEPT
// add new types here as needed
/* Notes:
	Most controls will use the dpad to move through the selection possibilies.  Buttons are the only exception.
	Buttons will be assumed to all be on one menu together.  If the start or A button is pressed on a control focus, that
	means that the menu is accepted and move onto the next menu.  If the start or A button is pressed on a button focus it
	should just process the action and not support the accept functionality.
*/
//JLFACCEPT
				else if ( item->type == ITEM_TYPE_MULTI || item->type == ITEM_TYPE_YESNO || item->type == ITEM_TYPE_SLIDER)
				{

					if (Item_HandleAccept(item))
					{
						//Item processed it overriding the menu processing
						return;
					}
					else if (menu->onAccept)
					{
						itemDef_t it;
						it.parent = menu;
						Item_RunScript(&it, menu->onAccept);
					}


				}

//END JLFACCEPT
				else
				{
					if (Rect_ContainsPoint(&item->window.rect, DC->cursorx, DC->cursory))
					{
						Item_Action(item);
					}
				}
			}
			else if (menu->onAccept)
			{
				itemDef_t it;
				it.parent = menu;
				Item_RunScript(&it, menu->onAccept);
			}
			break;

		case A_JOY0:
		case A_JOY1:
		case A_JOY2:
		case A_JOY3:
		case A_JOY4:
		case A_AUX0:
		case A_AUX1:
		case A_AUX2:
		case A_AUX3:
		case A_AUX4:
		case A_AUX5:
		case A_AUX6:
		case A_AUX7:
		case A_AUX8:
		case A_AUX9:
		case A_AUX10:
		case A_AUX11:
		case A_AUX12:
		case A_AUX13:
		case A_AUX14:
		case A_AUX15:
		case A_AUX16:
			break;
		case A_KP_ENTER:
		case A_ENTER:
			if (item)
			{
				if (item->type == ITEM_TYPE_EDITFIELD || item->type == ITEM_TYPE_NUMERICFIELD)
				{
					item->cursorPos = 0;
					g_editingField = qtrue;
					g_editItem = item;
					//DC->setOverstrikeMode(qtrue);
				}
				else
				{
						Item_Action(item);
				}
			}
			break;
	}
	inHandler = qfalse;
}

/*
=================
ParseRect
=================
*/
qboolean ParseRect(const char **p, rectDef_t *r)
{
	if (!COM_ParseFloat(p, &r->x))
	{
		if (!COM_ParseFloat(p, &r->y))
		{
			if (!COM_ParseFloat(p, &r->w))
			{
				if (!COM_ParseFloat(p, &r->h))
				{
					return qtrue;
				}
			}
		}
	}
	return qfalse;
}


/*
=================
Menus_HideItems
=================
*/
void Menus_HideItems(const char *menuName)
{
	menuDef_t	*menu;
	int			i;

	menu =Menus_FindByName(menuName);	// Get menu

	if (!menu)
	{
		Com_Printf(S_COLOR_YELLOW"WARNING: No menu was found. Could not hide items.\n");
		return;
	}

	menu->window.flags &= ~(WINDOW_HASFOCUS | WINDOW_VISIBLE);

	for (i = 0; i < menu->itemCount; i++)
	{
		menu->items[i]->cvarFlags = CVAR_HIDE;
	}
}

/*
=================
Menus_ShowItems
=================
*/
void Menus_ShowItems(const char *menuName)
{
	menuDef_t	*menu;
	int			i;

	menu =Menus_FindByName(menuName);	// Get menu

	if (!menu)
	{
		Com_Printf(S_COLOR_YELLOW"WARNING: No menu was found. Could not show items.\n");
		return;
	}

	menu->window.flags |= (WINDOW_HASFOCUS | WINDOW_VISIBLE);

	for (i = 0; i < menu->itemCount; i++)
	{
		menu->items[i]->cvarFlags = CVAR_SHOW;
	}
}

/*
=================
UI_Cursor_Show
=================
*/
void UI_Cursor_Show(qboolean flag)
{
	DC->cursorShow = flag;

	if ((DC->cursorShow != qtrue) && (DC->cursorShow != qfalse))
	{
		DC->cursorShow = qtrue;
	}
}
