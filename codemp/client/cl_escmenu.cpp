/*
===========================================================================
Copyright (C) 2026, OpenJK contributors

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

// cl_escmenu.cpp -- the menu Esc opens in game, drawn by the engine: Modern or KOTOR
//
// JKA's own in-game menu is a bar across the top with no room left. This one has every entry the
// game and this client have, in categories, as many as there are:
//
//   Modern   the categories down the left, the picked one's entries as cards on the right,
//            scrolled, searched as you type
//   KOTOR    as KOTOR's in-game screens: a framed panel, tabs along the top, the tab's entries
//            on the left and the picked one told about on the right
//
// cl_escMenu picks: 0 JKA's own menu (Classic, the default), 1 Modern, 2 KOTOR. Both draw the same entries; an
// entry runs a command (a tool), opens a page of JKA's own menus (Controls, Setup, Profile: the
// router in cl_rendermenu.cpp), or shows a panel of its own here: the servers, the camera, the
// menu's style, or Leave and Quit asking first.

#include <vector>
#include "client.h"
#include "cl_escmenu.h"
#include "cl_uiapi.h"
#include "cl_widgets.h"

#define EM_CMD			"escmenu"

typedef enum {
	EM_CLASSIC,
	EM_MODERN,
	EM_KOTOR
} emStyle_t;

// the Modern categories, and the KOTOR tabs
enum { MC_GAME, MC_CHARACTER, MC_ABILITIES, MC_MESSAGES, MC_TOOLS, MC_SETTINGS, MC_NUM };
enum { KT_CHARACTER, KT_EQUIPMENT, KT_INVENTORY, KT_ABILITIES, KT_MESSAGES, KT_JOURNAL, KT_MAP, KT_OPTIONS, KT_NUM };

static const char *emModernCats[MC_NUM] = { "Game", "Character", "Abilities", "Messages", "Tools", "Settings" };
static const char *emModernBlurb[MC_NUM] = {
	"Back to it, your team, the servers, leaving",
	"Who you are: your profile, presets and skins",
	"Moves, emotes and radial menus",
	"The chat, your datapad's news and forum, feedback",
	"Every tool this client adds",
	"Controls, video, the camera and this menu",
};
static const char *emKotorTabs[KT_NUM] = { "Character", "Equipment", "Inventory", "Abilities", "Messages", "Journal", "Map", "Options" };

typedef enum {
	EA_CMD,			// a command, the menu closed first
	EA_RESUME,
	EA_PROFILE,		// JKA's own Profile page
	EA_ROUTE,		// a page of JKA's own menus, through the router
	EA_PANEL		// a panel of this menu's own
} emKind_t;

typedef enum {
	EP_NONE,
	EP_SERVERS,
	EP_CAMERA,
	EP_STYLE,
	EP_LEAVE,
	EP_QUIT
} emPanel_t;

typedef struct {
	const char	*name;
	const char	*desc;
	const char	*arg;		// the command, the route, or the panel (EP_ as a number)
	emKind_t	kind;
	int			cat, tab;	// Modern's category, KOTOR's tab
} emEntry_t;

static const emEntry_t emBase[] = {
	{ "Resume",				"Back to the game",									NULL,				EA_RESUME,	MC_GAME,		-1 },
	{ "Join the game",		"Play: you join the free team",						"team free",		EA_CMD,		MC_GAME,		KT_MAP },
	{ "Spectate",			"Watch: you go to the spectators",					"team spectator",	EA_CMD,		MC_GAME,		KT_MAP },
	{ "Servers",			"JEDI's servers, their maps and players, joined in a click", (const char *)EP_SERVERS, EA_PANEL, MC_GAME, KT_MAP },
	{ "Leave the server",	"Disconnect, back to the main menu",				(const char *)EP_LEAVE,	EA_PANEL,	MC_GAME,		KT_MAP },
	{ "Profile",			"Your name, model, sabers and colours (the game's own page)", NULL,	EA_PROFILE,	MC_CHARACTER,	KT_CHARACTER },
	{ "Presets",			"Characters saved, swapped to in a click",			"presets",			EA_CMD,		MC_CHARACTER,	KT_CHARACTER },
	{ "Skin Select",		"Every skin of your model, worn in a click",		"skinselect",		EA_CMD,		MC_CHARACTER,	KT_EQUIPMENT },
	{ "Languages",			"Your characters' languages",						"languages",		EA_CMD,		MC_CHARACTER,	KT_CHARACTER },
	{ "Effect Replacer",	"Your own powers' and saber's effects as others",	"effectreplacer",	EA_CMD,		MC_CHARACTER,	KT_EQUIPMENT },
	{ "Special Moves",		"Saber moves explained, played from a key",			"specialmoves",		EA_CMD,		MC_ABILITIES,	KT_ABILITIES },
	{ "Animation Manager",	"Every animation; emote chains, custom emotes",		"animationmanager",	EA_CMD,		MC_ABILITIES,	KT_ABILITIES },
	{ "Radial Menus",		"Make radial menus and bind keys to them",			"radialsettings",	EA_CMD,		MC_ABILITIES,	KT_ABILITIES },
	{ "Chat Log",			"The chat kept as text and a console-like page",	"chatlog",			EA_CMD,		MC_MESSAGES,	KT_MESSAGES },
	{ "Datapad",			"Your datapad: in the corner, or open on the right", "datapad",			EA_CMD,		MC_MESSAGES,	KT_JOURNAL },
	{ "HoloNet",			"JEDI's news from jediholo.net, on your datapad",	"datapad holonet",	EA_CMD,		MC_MESSAGES,	KT_JOURNAL },
	{ "Comport",			"JEDI's forum, read and replied to on your datapad", "datapad comport",	EA_CMD,		MC_MESSAGES,	KT_JOURNAL },
	{ "Pazaak",				"The card game, against a player on your server",	"datapad pazaak",	EA_CMD,		MC_MESSAGES,	KT_JOURNAL },
	{ "Feedback",			"Report a bug or send an idea",						"feedback",			EA_CMD,		MC_MESSAGES,	KT_MESSAGES },
	{ "Controls",			"Keys and the mouse (the game's own page)",			"controls",			EA_ROUTE,	MC_SETTINGS,	KT_OPTIONS },
	{ "Setup",				"Video, sound and game options (the game's own page)", "setup",			EA_ROUTE,	MC_SETTINGS,	KT_OPTIONS },
	{ "Renderer",			"OpenGL or Vulkan, and their graphics settings",	"renderer",			EA_ROUTE,	MC_SETTINGS,	KT_OPTIONS },
	{ "Camera",				"The third person camera, and the field of view",	(const char *)EP_CAMERA,	EA_PANEL,	MC_SETTINGS,	KT_OPTIONS },
	{ "Console Settings",	"How the console looks",							"consolesettings",	EA_CMD,		MC_SETTINGS,	KT_OPTIONS },
	{ "Menu Style",			"This menu as Modern or KOTOR, or the game's own",	(const char *)EP_STYLE,	EA_PANEL,	MC_SETTINGS,	KT_OPTIONS },
	{ "Quit the game",		"Close Jedi Academy",								(const char *)EP_QUIT,	EA_PANEL,	MC_GAME,		KT_OPTIONS },
};

// every tool this client adds, the classic menu's TOOLS panel's too
const escTool_t escTools[] = {
	{ "Model Manager",		"modelmanager",		"Place models, move the map's (rpmodel)" },
	{ "NPC Manager",		"npcmanager",		"Spawn NPCs and command them" },
	{ "Effect Manager",		"effectmanager",	"Preview effects and play them (rpeffect)" },
	{ "Effect Replacer",	"effectreplacer",	"Your own powers' and saber's effects as others" },
	{ "Shader Manager",		"shadermanager",	"Remap shaders, overlays, the sky" },
	{ "Map Editor",			"mapedit",			"Move, hide and retexture the map; lights, fog" },
	{ "Storyteller",		"storyteller",		"Missions out of steps, played for everyone" },
	{ "Cinematic Manager",	"cinematicmanager",	"Camera shots and what everyone does in them" },
	{ "Sound Manager",		"soundmanager",		"Hear sounds and play them (rpsound)" },
	{ "Music Manager",		"musicmanager",		"Hear music and play it for everyone" },
	{ "Animation Manager",	"animationmanager",	"Every animation; emote chains, custom emotes" },
	{ "Asset Manager",		"assetmanager",		"What's on the map, its pk3s, packed in one" },
	{ "Presets",			"presets",			"Characters saved, swapped to in a click" },
	{ "Skin Select",		"skinselect",		"Every skin of your model, worn in a click" },
	{ "Special Moves",		"specialmoves",		"Saber moves explained, played from a key" },
	{ "Chat Log",			"chatlog",			"The chat kept as text and a console-like page" },
	{ "Datapad",			"datapad",			"Your datapad: in the corner, or open on the right" },
	{ "Languages",			"languages",		"Your characters' languages" },
	{ "Radial Menus",		"radialsettings",	"Make radial menus and bind keys to them" },
	{ "Console Settings",	"consolesettings",	"How the console looks" },
	{ "Feedback",			"feedback",			"Report a bug or send an idea" },
	{ "Esc Menu",			EM_CMD,				"The menu Esc opens: Modern, KOTOR or the game's own" },
};
const int escNumTools = (int)ARRAY_LEN( escTools );

static std::vector<emEntry_t>	emEntries;

static cvar_t *cl_escMenu;

static struct {
	qboolean	open;
	emStyle_t	style;
	qhandle_t	font, titleFont;
	sfxHandle_t	clickSound, moveSound;
	float		cursorX, cursorY;
	qboolean	click;
	float		clickX, clickY;
	int			cat;				// Modern's category, or KOTOR's tab
	int			sel;				// in the shown list
	float		scroll, content;
	char		search[48];
	qboolean	typing;
	emPanel_t	panel;
	int			camDrag;			// the camera slider being dragged, -1 none
	int			openTime;
	char		message[128];
	int			messageTime;
	std::vector<int>	shown;		// the entries the list shows
} em;

/*
===============================================================================

Drawing helpers

===============================================================================
*/

static const float emWhite[4]		= { 1.0f, 1.0f, 1.0f, 1.0f };

// Modern
static const float mdBack[4]		= { 0.01f, 0.015f, 0.03f, 0.80f };
static const float mdSide[4]		= { 0.035f, 0.05f, 0.085f, 0.95f };
static const float mdAccent[4]		= { 0.24f, 0.66f, 1.0f, 1.0f };
static const float mdAccentSoft[4]	= { 0.24f, 0.66f, 1.0f, 0.16f };
static const float mdCard[4]		= { 1.0f, 1.0f, 1.0f, 0.04f };
static const float mdCardHover[4]	= { 1.0f, 1.0f, 1.0f, 0.08f };
static const float mdText[4]		= { 0.93f, 0.95f, 1.0f, 1.0f };
static const float mdDim[4]			= { 0.56f, 0.62f, 0.72f, 1.0f };
static const float mdLine[4]		= { 1.0f, 1.0f, 1.0f, 0.08f };
static const float mdDanger[4]		= { 0.95f, 0.35f, 0.35f, 1.0f };
static const float mdDangerSoft[4]	= { 0.95f, 0.35f, 0.35f, 0.14f };

// KOTOR
static const float ktBack[4]		= { 0.0f, 0.02f, 0.05f, 0.90f };
static const float ktScan[4]		= { 0.45f, 0.7f, 1.0f, 0.035f };
static const float ktSteel[4]		= { 0.42f, 0.5f, 0.58f, 1.0f };
static const float ktSteelDark[4]	= { 0.09f, 0.13f, 0.19f, 0.96f };
static const float ktSteelLight[4]	= { 0.62f, 0.72f, 0.8f, 1.0f };
static const float ktGlow[4]		= { 0.45f, 0.8f, 1.0f, 1.0f };
static const float ktTab[4]			= { 0.12f, 0.33f, 0.58f, 1.0f };
static const float ktPanel[4]		= { 0.02f, 0.06f, 0.12f, 0.92f };
static const float ktRow[4]			= { 0.25f, 0.55f, 0.9f, 0.28f };
static const float ktRowHover[4]	= { 0.25f, 0.55f, 0.9f, 0.12f };
static const float ktText[4]		= { 0.72f, 0.85f, 0.97f, 1.0f };
static const float ktDim[4]			= { 0.45f, 0.58f, 0.7f, 1.0f };
static const float ktGold[4]		= { 1.0f, 0.82f, 0.4f, 1.0f };

static float EM_Wide( float w ) {
	return w * ( cls.widthRatioCoef > 0.0f ? cls.widthRatioCoef : 1.0f );
}

static void EM_Fill( float x, float y, float w, float h, const float *color ) {
	re->SetColor( color );
	re->DrawStretchPic( x, y, w, h, 0, 0, 0, 0, cls.whiteShader );
	re->SetColor( NULL );
}

static void EM_Frame( float x, float y, float w, float h, float t, const float *color ) {
	EM_Fill( x, y, w, t, color );
	EM_Fill( x, y + h - t, w, t, color );
	EM_Fill( x, y + t, t, h - t * 2, color );
	EM_Fill( x + w - t, y + t, t, h - t * 2, color );
}

static float EM_TextW( const char *text, int font, float scale ) {
	return (float)re->Font_StrLenPixels( text, font, scale );
}

// cut to fit with ..., left, centred or right in w (align 0, 1, 2)
static void EM_Text( float x, float y, float w, const char *text, const float *color, int font, float scale, int align ) {
	char buf[MAX_STRING_CHARS];
	int len;

	Q_strncpyz( buf, text, sizeof( buf ) );
	len = (int)strlen( buf );
	while ( len > 3 && EM_TextW( buf, font, scale ) > w ) {
		len--;
		buf[len - 3] = buf[len - 2] = buf[len - 1] = '.';
		buf[len] = '\0';
	}
	if ( align == 1 )
		x += ( w - EM_TextW( buf, font, scale ) ) * 0.5f;
	else if ( align == 2 )
		x += w - EM_TextW( buf, font, scale );
	re->Font_DrawString( (int)x, (int)y, buf, color, font | STYLE_DROPSHADOW, -1, scale );
}

static void EM_Upper( const char *in, char *out, int size ) {
	Q_strncpyz( out, in, size );
	Q_strupr( out );
}

static qboolean EM_In( float x, float y, float w, float h ) {
	return (qboolean)( em.cursorX >= x && em.cursorX < x + w && em.cursorY >= y && em.cursorY < y + h );
}

static qboolean EM_Clicked( float x, float y, float w, float h ) {
	if ( !em.click || em.clickX < x || em.clickX >= x + w || em.clickY < y || em.clickY >= y + h )
		return qfalse;
	em.click = qfalse;
	S_StartLocalSound( em.clickSound, CHAN_LOCAL_SOUND );
	return qtrue;
}

static void EM_Message( const char *text ) {
	Q_strncpyz( em.message, text, sizeof( em.message ) );
	em.messageTime = cls.realtime;
}

// a button in the style's look; kind 0 plain, 1 the main one, 2 a dangerous one
static qboolean EM_Button( float x, float y, float w, float h, const char *label, int kind ) {
	const qboolean hover = EM_In( x, y, w, h );

	if ( em.style == EM_KOTOR ) {
		char up[64];

		EM_Fill( x, y, w, h, hover ? ktTab : ktSteelDark );
		EM_Fill( x, y, w, 1, ktSteelLight );
		EM_Fill( x, y + h - 1, w, 1, ktSteel );
		EM_Fill( x, y, 1, h, ktSteel );
		EM_Fill( x + w - 1, y, 1, h, ktSteel );
		if ( kind == 1 )
			EM_Fill( x + 2, y + h - 3, w - 4, 1, ktGlow );
		EM_Upper( label, up, sizeof( up ) );
		EM_Text( x, y + ( h - 11 ) * 0.5f, w, up, kind == 2 ? ktGold : hover ? emWhite : ktText, em.titleFont, 0.42f, 1 );
	} else {
		const float *fill = kind == 1 ? mdAccent : kind == 2 ? mdDangerSoft : mdCard;

		EM_Fill( x, y, w, h, hover && kind != 1 ? mdCardHover : fill );
		if ( kind == 2 )
			EM_Frame( x, y, w, h, 1, mdDanger );
		else if ( kind == 0 )
			EM_Frame( x, y, w, h, 1, hover ? mdAccent : mdLine );
		EM_Text( x, y + ( h - 12 ) * 0.5f, w, label, kind == 1 ? emWhite : kind == 2 ? mdDanger : mdText,
			em.font, 0.72f, 1 );
	}
	return EM_Clicked( x, y, w, h );
}

static void EM_StyleWG( wgStyle_t *style, const float *text, float scale ) {
	memset( style, 0, sizeof( *style ) );
	style->font = em.font;
	style->scale = scale;
	style->lineH = 14.0f * scale / 0.75f;
	style->padX = 5;
	style->padY = 3;
	style->fill = em.style == EM_KOTOR ? ktPanel : mdSide;
	style->border = em.style == EM_KOTOR ? ktSteel : mdAccent;
	style->text = text;
	style->dim = em.style == EM_KOTOR ? ktDim : mdDim;
}

/*
===============================================================================

What the list shows

===============================================================================
*/

static void EM_BuildEntries( void ) {
	emEntries.assign( emBase, emBase + ARRAY_LEN( emBase ) );
	for ( int i = 0; i < escNumTools; i++ ) {
		emEntry_t e;

		e.name = escTools[i].name;
		e.desc = escTools[i].desc;
		e.arg = escTools[i].command;
		e.kind = EA_CMD;
		e.cat = MC_TOOLS;
		e.tab = KT_INVENTORY;
		emEntries.push_back( e );
	}
}

// the picked category's, or tab's; searched, every entry whose name or text has it (once each)
static void EM_BuildShown( void ) {
	em.shown.clear();
	for ( int i = 0; i < (int)emEntries.size(); i++ ) {
		const emEntry_t &e = emEntries[i];

		if ( em.style == EM_MODERN && em.search[0] ) {
			qboolean seen = qfalse;

			if ( !Q_stristr( e.name, em.search ) && !Q_stristr( e.desc, em.search ) )
				continue;
			for ( int k : em.shown )
				seen = (qboolean)( seen || !Q_stricmp( emEntries[k].name, e.name ) );
			if ( !seen )
				em.shown.push_back( i );
		} else if ( em.style == EM_KOTOR ? e.tab == em.cat : e.cat == em.cat ) {
			em.shown.push_back( i );
		}
	}
	em.sel = Com_Clampi( 0, Q_max( 0, (int)em.shown.size() - 1 ), em.sel );
}

static void EM_PickCat( int cat ) {
	const int num = em.style == EM_KOTOR ? KT_NUM : MC_NUM;

	cat = ( cat + num ) % num;
	if ( cat != em.cat )
		S_StartLocalSound( em.moveSound, CHAN_LOCAL_SOUND );
	em.cat = cat;
	em.sel = 0;
	em.scroll = 0;
	em.panel = EP_NONE;
	em.search[0] = '\0';
	em.typing = qfalse;
	EM_BuildShown();
}

/*
===============================================================================

Opening, closing, doing

===============================================================================
*/

static void EM_Close( void ) {
	if ( !em.open )
		return;
	em.open = qfalse;
	em.typing = qfalse;
	em.camDrag = -1;
	Key_SetCatcher( Key_GetCatcher() & ~KEYCATCH_ESCMENU );
}

void CL_EscMenu_Close( void ) {
	EM_Close();
}

static qboolean EM_Open( emStyle_t style ) {
	if ( cls.state != CA_ACTIVE || clc.demoplaying )
		return qfalse;
	if ( !em.font ) {
		em.font = re->RegisterFont( "arialnb" );
		em.titleFont = re->RegisterFont( "ergoec" );
		em.clickSound = S_RegisterSound( "sound/interface/button1.wav" );
		em.moveSound = S_RegisterSound( "sound/interface/menuroam.wav" );
	}
	if ( em.style != style ) {
		em.cat = 0;
		em.sel = 0;
	}
	em.style = style;
	em.open = qtrue;
	em.click = qfalse;
	em.panel = EP_NONE;
	em.search[0] = '\0';
	em.typing = qfalse;
	em.camDrag = -1;
	em.scroll = 0;
	em.openTime = cls.realtime;
	em.cursorX = SCREEN_WIDTH * 0.5f;
	em.cursorY = SCREEN_HEIGHT * 0.5f;
	CL_Presets_Close();
	Key_SetCatcher( Key_GetCatcher() | KEYCATCH_ESCMENU );
	EM_BuildShown();
	return qtrue;
}

// Esc in game, with nothing else open: ours, unless Classic is picked
qboolean CL_EscMenu_OpenFromEscape( void ) {
	const int style = cl_escMenu ? cl_escMenu->integer : EM_CLASSIC;

	if ( style != EM_MODERN && style != EM_KOTOR )
		return qfalse;
	return EM_Open( (emStyle_t)style );
}

static void EM_Do( const emEntry_t &e ) {
	switch ( e.kind ) {
	case EA_RESUME:
		EM_Close();
		break;
	case EA_CMD:
		EM_Close();
		Cbuf_AddText( va( "%s\n", e.arg ) );
		break;
	case EA_PROFILE:
		EM_Close();
		UIVM_SetActiveMenu( UIMENU_PLAYERCONFIG );
		break;
	case EA_ROUTE:
		EM_Close();
		if ( !CL_RenderMenu_Route( e.arg ) )
			UIVM_SetActiveMenu( UIMENU_INGAME );
		break;
	case EA_PANEL:
		em.panel = (emPanel_t)(intptr_t)e.arg;
		em.camDrag = -1;
		if ( em.panel == EP_SERVERS )
			Cbuf_AddText( "quickconnect refresh\n" );
		break;
	}
}

static void EM_SetStyle( emStyle_t style ) {
	Cvar_Set( "cl_escMenu", va( "%i", style ) );
	if ( style == EM_CLASSIC ) {
		EM_Message( "Esc opens the game's own menu from now on (/escmenu opens this one)" );
		return;
	}
	if ( style != em.style ) {
		em.style = style;
		em.cat = em.style == EM_KOTOR ? KT_OPTIONS : MC_SETTINGS;
		EM_BuildShown();
		for ( int i = 0; i < (int)em.shown.size(); i++ ) {
			if ( emEntries[em.shown[i]].kind == EA_PANEL && (emPanel_t)(intptr_t)emEntries[em.shown[i]].arg == EP_STYLE )
				em.sel = i;
		}
	}
}

/*
===============================================================================

Panels: drawn in a box of the style's, the same for both

===============================================================================
*/

static const float *EM_TextColor( void ) {
	return em.style == EM_KOTOR ? ktText : mdText;
}

static const float *EM_DimColor( void ) {
	return em.style == EM_KOTOR ? ktDim : mdDim;
}

static const float *EM_AccentColor( void ) {
	return em.style == EM_KOTOR ? ktGlow : mdAccent;
}

static void EM_PanelServers( float x, float y, float w, float h ) {
	const int n = CL_QuickConnect_Count();
	const float rowH = Q_min( 52.0f, ( h - 28 ) / Q_max( 1, n ) );

	if ( EM_Button( x + w - EM_Wide( 70 ), y, EM_Wide( 70 ), 18, "Refresh", 0 ) )
		Cbuf_AddText( "quickconnect refresh\n" );
	EM_Text( x, y + 3, w - EM_Wide( 80 ), "JEDI's servers", EM_DimColor(), em.font, 0.7f, 0 );
	y += 28;
	for ( int i = 0; i < n; i++, y += rowH ) {
		const char *map = Cvar_VariableString( va( "ui_qc%i_map", i + 1 ) );
		const char *players = Cvar_VariableString( va( "ui_qc%i_players", i + 1 ) );

		EM_Fill( x, y, w, rowH - 6, em.style == EM_KOTOR ? ktRowHover : mdCard );
		EM_Text( x + 8, y + 5, w - EM_Wide( 100 ), CL_QuickConnect_Name( i ), EM_AccentColor(), em.font, 0.8f, 0 );
		EM_Text( x + 8, y + 21, w - EM_Wide( 100 ), va( "%s%s%s", map[0] ? map : "asking...", players[0] ? "   " : "", players ),
			EM_DimColor(), em.font, 0.65f, 0 );
		if ( EM_Button( x + w - EM_Wide( 84 ), y + ( rowH - 6 - 20 ) * 0.5f, EM_Wide( 76 ), 20, "Connect", 1 ) ) {
			EM_Close();
			Cbuf_AddText( va( "quickconnect connect %i\n", i + 1 ) );
			return;
		}
	}
}

static void EM_PanelCamera( float x, float y, float w, float h ) {
	const float rowH = 22.0f, labelW = w * 0.36f, trackX = x + labelW + 8, trackW = w - labelW - EM_Wide( 60 );
	int row = 0;

	for ( ;; row++ ) {
		const char *label;
		float def, min, max;
		qboolean toggle;
		const char *cvar = CL_RenderMenu_CameraRow( row, &label, &def, &min, &max, &toggle );
		const float ry = y + row * rowH;
		const float value = cvar ? Cvar_VariableValue( cvar ) : 0;

		if ( !cvar || ry + rowH > y + h - 26 )
			break;
		EM_Text( x, ry + 4, labelW, label, EM_TextColor(), em.font, 0.7f, 2 );
		if ( toggle ) {
			if ( EM_Button( trackX, ry + 2, EM_Wide( 60 ), rowH - 5, value != 0 ? "On" : "Off", value != 0 ? 1 : 0 ) )
				Cvar_Set( cvar, value != 0 ? "0" : "1" );
			continue;
		}
		// a track and its knob; a click or a drag on it sets it
		{
			const float frac = max > min ? Com_Clamp( 0, 1, ( value - min ) / ( max - min ) ) : 0;
			const float ty = ry + rowH * 0.5f;

			EM_Fill( trackX, ty - 1, trackW, 2, em.style == EM_KOTOR ? ktSteel : mdLine );
			EM_Fill( trackX, ty - 1, trackW * frac, 2, EM_AccentColor() );
			EM_Fill( trackX + trackW * frac - EM_Wide( 3 ), ty - 6, EM_Wide( 6 ), 12, EM_In( trackX - 4, ry, trackW + 8, rowH ) || em.camDrag == row
				? emWhite : EM_AccentColor() );
			EM_Text( trackX + trackW + 6, ry + 4, EM_Wide( 50 ), max - min > 10 ? va( "%i", (int)value ) : va( "%.2f", value ),
				EM_DimColor(), em.font, 0.65f, 0 );
			if ( em.click && EM_In( trackX - 4, ry, trackW + 8, rowH ) && em.clickY >= ry && em.clickY < ry + rowH ) {
				em.click = qfalse;
				em.camDrag = row;
			}
			if ( em.camDrag == row ) {
				if ( !Key_IsDown( A_MOUSE1 ) ) {
					em.camDrag = -1;
				} else {
					float v = min + Com_Clamp( 0, 1, ( em.cursorX - trackX ) / trackW ) * ( max - min );

					v = max - min > 10 ? floorf( v + 0.5f ) : floorf( v * 20.0f + 0.5f ) / 20.0f;
					Cvar_Set( cvar, va( "%g", v ) );
				}
			}
		}
	}
	if ( EM_Button( x, y + h - 22, EM_Wide( 120 ), 20, "Back to defaults", 0 ) ) {
		for ( int r = 0; ; r++ ) {
			const char *label;
			float def, min, max;
			qboolean toggle;
			const char *cvar = CL_RenderMenu_CameraRow( r, &label, &def, &min, &max, &toggle );

			if ( !cvar )
				break;
			Cvar_Set( cvar, va( "%g", def ) );
		}
	}
}

static void EM_PanelStyle( float x, float y, float w, float h ) {
	static const char *names[3] = { "Classic", "Modern", "KOTOR" };
	static const char *descs[3] = {
		"The game's own bar across the top, as it always was",
		"Categories down the side, a list you scroll and search",
		"KOTOR's look: a framed screen with tabs along the top",
	};
	const int now = cl_escMenu ? cl_escMenu->integer : EM_CLASSIC;
	const float cardH = Q_min( 56.0f, ( h - 10 ) / 3 );

	for ( int i = 0; i < 3; i++ ) {
		const int s = i == 0 ? EM_MODERN : i == 1 ? EM_KOTOR : EM_CLASSIC;
		const float cy = y + i * cardH;
		const qboolean on = (qboolean)( now == s );
		const qboolean hover = EM_In( x, cy, w, cardH - 6 );

		EM_Fill( x, cy, w, cardH - 6, on ? ( em.style == EM_KOTOR ? ktRow : mdAccentSoft ) : hover ? ( em.style == EM_KOTOR ? ktRowHover : mdCardHover )
			: ( em.style == EM_KOTOR ? ktSteelDark : mdCard ) );
		if ( on )
			EM_Frame( x, cy, w, cardH - 6, 1, EM_AccentColor() );
		EM_Text( x + 10, cy + 6, w - 20, va( "%s%s", names[s], on ? "   (now)" : "" ), on ? EM_AccentColor() : EM_TextColor(), em.font, 0.85f, 0 );
		EM_Text( x + 10, cy + 25, w - 20, descs[s], EM_DimColor(), em.font, 0.65f, 0 );
		if ( EM_Clicked( x, cy, w, cardH - 6 ) )
			EM_SetStyle( (emStyle_t)s );
	}
}

static void EM_PanelAsk( float x, float y, float w, float h, qboolean quit ) {
	const float bw = EM_Wide( 90 );

	EM_Text( x, y + h * 0.3f, w, quit ? "Quit Jedi Academy?" : "Leave the server?", EM_TextColor(), em.font, 1.0f, 1 );
	EM_Text( x, y + h * 0.3f + 22, w, quit ? "The game closes." : "You go back to the main menu.", EM_DimColor(), em.font, 0.7f, 1 );
	if ( EM_Button( x + w * 0.5f - bw - 6, y + h * 0.3f + 50, bw, 22, quit ? "Quit" : "Leave", 2 ) ) {
		EM_Close();
		Cbuf_AddText( quit ? "quit\n" : "disconnect\n" );
		return;
	}
	if ( EM_Button( x + w * 0.5f + 6, y + h * 0.3f + 50, bw, 22, "Stay", 0 ) )
		em.panel = EP_NONE;
}

static const char *EM_PanelTitle( void ) {
	switch ( em.panel ) {
	case EP_SERVERS:	return "Servers";
	case EP_CAMERA:		return "Camera";
	case EP_STYLE:		return "Menu Style";
	case EP_LEAVE:		return "Leave the server";
	case EP_QUIT:		return "Quit the game";
	default:			return "";
	}
}

static void EM_DrawPanel( float x, float y, float w, float h ) {
	switch ( em.panel ) {
	case EP_SERVERS:	EM_PanelServers( x, y, w, h );			break;
	case EP_CAMERA:		EM_PanelCamera( x, y, w, h );			break;
	case EP_STYLE:		EM_PanelStyle( x, y, w, h );			break;
	case EP_LEAVE:		EM_PanelAsk( x, y, w, h, qfalse );		break;
	case EP_QUIT:		EM_PanelAsk( x, y, w, h, qtrue );		break;
	default:			break;
	}
}

static const char *EM_KindNote( const emEntry_t &e ) {
	switch ( e.kind ) {
	case EA_CMD:		return va( "/%s", e.arg );
	case EA_PROFILE:
	case EA_ROUTE:		return "the game's own page";
	case EA_PANEL:		return em.style == EM_KOTOR ? "" : ">";
	default:			return "";
	}
}

static const char *EM_ServerName( void ) {
	const char *info = cl.gameState.stringData + cl.gameState.stringOffsets[CS_SERVERINFO];
	const char *host = Info_ValueForKey( info, "sv_hostname" );
	static char clean[128];

	Q_strncpyz( clean, host, sizeof( clean ) );
	Q_StripColor( clean );
	return clean[0] ? clean : "Jedi Academy";
}

/*
===============================================================================

Modern

===============================================================================
*/

#define MD_SIDE_W		EM_Wide( 150.0f )
#define MD_CARD_H		38.0f
#define MD_CARD_GAP		6.0f

static void EM_DrawModern( void ) {
	const float cx = MD_SIDE_W + 22, cw = SCREEN_WIDTH - cx - 22;
	const float listY = 64, listH = SCREEN_HEIGHT - listY - 34;
	wgStyle_t style;

	EM_Fill( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, mdBack );
	// a glow along the top, fading down
	for ( int i = 0; i < 12; i++ ) {
		const float glow[4] = { mdAccent[0], mdAccent[1], mdAccent[2], 0.05f * ( 1.0f - i / 12.0f ) };

		EM_Fill( MD_SIDE_W, i * 4.0f, SCREEN_WIDTH - MD_SIDE_W, 4, glow );
	}

	// the side: the server, the categories, Resume and Leave
	EM_Fill( 0, 0, MD_SIDE_W, SCREEN_HEIGHT, mdSide );
	EM_Fill( MD_SIDE_W - 1, 0, 1, SCREEN_HEIGHT, mdLine );
	EM_Text( 16, 16, MD_SIDE_W - 28, "MENU", mdText, em.titleFont, 0.75f, 0 );
	EM_Fill( 16, 38, EM_Wide( 24 ), 2, mdAccent );
	EM_Text( 16, 44, MD_SIDE_W - 28, EM_ServerName(), mdDim, em.font, 0.6f, 0 );
	EM_Text( 16, 56, MD_SIDE_W - 28, Cvar_VariableString( "mapname" ), mdDim, em.font, 0.55f, 0 );
	for ( int c = 0; c < MC_NUM; c++ ) {
		const float y = 82 + c * 32.0f;
		const qboolean on = (qboolean)( c == em.cat && !em.search[0] );
		const qboolean hover = EM_In( 8, y, MD_SIDE_W - 16, 28 );
		char letter[2] = { emModernCats[c][0], 0 };

		if ( on || hover )
			EM_Fill( 8, y, MD_SIDE_W - 16, 28, on ? mdAccentSoft : mdCardHover );
		if ( on )
			EM_Fill( 8, y, 3, 28, mdAccent );
		EM_Fill( 18, y + 5, EM_Wide( 18 ), 18, on ? mdAccent : mdCard );
		EM_Text( 18, y + 7, EM_Wide( 18 ), letter, on ? emWhite : mdText, em.font, 0.7f, 1 );
		EM_Text( 18 + EM_Wide( 18 ) + 10, y + 7, MD_SIDE_W - 60, emModernCats[c], on ? emWhite : mdText, em.font, 0.78f, 0 );
		if ( EM_Clicked( 8, y, MD_SIDE_W - 16, 28 ) )
			EM_PickCat( c );
	}
	if ( EM_Button( 12, SCREEN_HEIGHT - 70, MD_SIDE_W - 24, 24, "Resume", 1 ) ) {
		EM_Close();
		return;
	}
	if ( EM_Button( 12, SCREEN_HEIGHT - 40, MD_SIDE_W - 24, 22, "Leave", 2 ) )
		em.panel = EP_LEAVE;

	// the content: its title, the search, the list or a panel
	if ( em.panel != EP_NONE ) {
		if ( EM_Button( cx, 18, EM_Wide( 60 ), 20, "< Back", 0 ) ) {
			em.panel = EP_NONE;
		} else {
			EM_Text( cx + EM_Wide( 72 ), 18, cw - EM_Wide( 72 ), EM_PanelTitle(), mdText, em.titleFont, 0.7f, 0 );
			EM_Fill( cx, 48, cw, 1, mdLine );
			EM_DrawPanel( cx, listY, cw, listH );
		}
	} else {
		const float searchW = EM_Wide( 170 );

		EM_Text( cx, 16, cw - searchW - 20, em.search[0] ? "Search" : emModernCats[em.cat], mdText, em.titleFont, 0.8f, 0 );
		EM_Text( cx, 38, cw - searchW - 20, em.search[0] ? va( "%i found", (int)em.shown.size() ) : emModernBlurb[em.cat], mdDim, em.font, 0.62f, 0 );
		EM_StyleWG( &style, mdText, 0.7f );
		EM_Fill( cx + cw - searchW, 20, searchW, 18, em.typing ? mdCardHover : mdCard );
		EM_Frame( cx + cw - searchW, 20, searchW, 18, 1, em.typing ? mdAccent : mdLine );
		WG_Field( &style, cx + cw - searchW, 20, searchW, 18, em.search, em.typing, "Type to search everything", em.cursorX, em.cursorY );
		if ( EM_Clicked( cx + cw - searchW, 20, searchW, 18 ) ) {
			if ( !em.typing )
				WG_EditReset();
			em.typing = qtrue;
			WG_FieldClick( em.search, em.clickX, em.clickY );
		}
		EM_Fill( cx, 54, cw, 1, mdLine );

		// the cards, scrolled
		{
			const float step = MD_CARD_H + MD_CARD_GAP;
			const qboolean bar = WG_ScrollbarPixels( cx + cw - WG_SCROLL_W, listY, listH, em.content, &em.scroll, em.cursorX, em.cursorY,
				&em.click, em.clickX, em.clickY );
			const float rw = cw - ( bar ? WG_SCROLL_W + 4 : 0 );

			em.content = em.shown.size() * step;
			em.scroll = Com_Clamp( 0, Q_max( 0.0f, em.content - listH ), em.scroll );
			for ( int i = 0; i < (int)em.shown.size(); i++ ) {
				const emEntry_t &e = emEntries[em.shown[i]];
				const float y = listY + i * step - em.scroll;
				const qboolean on = (qboolean)( i == em.sel );
				const qboolean hover = (qboolean)( y >= listY && y + MD_CARD_H <= listY + listH && EM_In( cx, y, rw, MD_CARD_H ) );

				if ( y < listY - 0.5f || y + MD_CARD_H > listY + listH + 0.5f )
					continue;
				EM_Fill( cx, y, rw, MD_CARD_H, on ? mdAccentSoft : hover ? mdCardHover : mdCard );
				if ( on )
					EM_Fill( cx, y, 3, MD_CARD_H, mdAccent );
				EM_Text( cx + 14, y + 6, rw * 0.7f, e.name, on ? emWhite : mdText, em.font, 0.8f, 0 );
				EM_Text( cx + 14, y + 22, rw - 28, e.desc, mdDim, em.font, 0.62f, 0 );
				EM_Text( cx + rw * 0.55f, y + 7, rw * 0.45f - 12, EM_KindNote( e ), mdDim, em.font, 0.58f, 2 );
				if ( EM_Clicked( cx, y, rw, MD_CARD_H ) ) {
					em.sel = i;
					EM_Do( e );
					if ( !em.open )
						return;
				}
			}
			if ( em.shown.empty() )
				EM_Text( cx, listY + 20, cw, "Nothing here has that", mdDim, em.font, 0.75f, 1 );
		}
	}

	// the hints, and a message
	if ( em.message[0] && cls.realtime - em.messageTime < 5000 )
		EM_Text( cx, SCREEN_HEIGHT - 22, cw, em.message, mdAccent, em.font, 0.62f, 0 );
	else
		EM_Text( cx, SCREEN_HEIGHT - 22, cw, "Enter: open   Up / Down: pick   Tab: next category   type: search   Esc: close", mdDim, em.font, 0.58f, 0 );
	WG_DrawOverlay();
}

/*
===============================================================================

KOTOR

===============================================================================
*/

#define KT_X		14.0f
#define KT_Y		10.0f
#define KT_W		( SCREEN_WIDTH - KT_X * 2 )
#define KT_H		( SCREEN_HEIGHT - KT_Y * 2 )
#define KT_ROW_H	24.0f

// a framed box: steel, a blue line inside it, its corners bracketed
static void EM_KotorBox( float x, float y, float w, float h, qboolean brackets ) {
	const float c = EM_Wide( 10 );

	EM_Fill( x, y, w, h, ktPanel );
	EM_Frame( x, y, w, h, 1, ktSteel );
	EM_Frame( x + 2, y + 2, w - 4, h - 4, 1, ktRowHover );
	if ( !brackets )
		return;
	EM_Fill( x - 1, y - 1, c, 2, ktGlow );
	EM_Fill( x - 1, y - 1, 2, 10, ktGlow );
	EM_Fill( x + w - c + 1, y - 1, c, 2, ktGlow );
	EM_Fill( x + w - 1, y - 1, 2, 10, ktGlow );
	EM_Fill( x - 1, y + h - 1, c, 2, ktGlow );
	EM_Fill( x - 1, y + h - 9, 2, 10, ktGlow );
	EM_Fill( x + w - c + 1, y + h - 1, c, 2, ktGlow );
	EM_Fill( x + w - 1, y + h - 9, 2, 10, ktGlow );
}

static void EM_DrawKotor( void ) {
	const float tabY = KT_Y + 10, tabH = 24;
	const float tabW = ( KT_W - 20 - ( KT_NUM - 1 ) * 3 ) / KT_NUM;
	const float listX = KT_X + 10, listY = KT_Y + 74, listW = KT_W * 0.44f, listH = KT_H - 74 - 44;
	const float infoX = listX + listW + 10, infoW = KT_X + KT_W - 10 - infoX;
	char up[64];

	EM_Fill( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ktBack );
	for ( float y = 0; y < SCREEN_HEIGHT; y += 3 )
		EM_Fill( 0, y, SCREEN_WIDTH, 1, ktScan );
	// the outer frame
	EM_Frame( KT_X, KT_Y, KT_W, KT_H, 2, ktSteel );
	EM_Frame( KT_X + 4, KT_Y + 4, KT_W - 8, KT_H - 8, 1, ktRowHover );

	// the tabs
	for ( int t = 0; t < KT_NUM; t++ ) {
		const float x = KT_X + 10 + t * ( tabW + 3 );
		const qboolean on = (qboolean)( t == em.cat );
		const qboolean hover = EM_In( x, tabY, tabW, tabH );

		EM_Fill( x, tabY, tabW, tabH, on ? ktTab : hover ? ktRowHover : ktSteelDark );
		EM_Fill( x, tabY, tabW, 1, on ? ktGlow : ktSteelLight );
		EM_Fill( x, tabY + tabH - 1, tabW, 1, ktSteel );
		EM_Fill( x, tabY, 1, tabH, ktSteel );
		EM_Fill( x + tabW - 1, tabY, 1, tabH, ktSteel );
		if ( on )
			EM_Fill( x + 3, tabY + tabH + 1, tabW - 6, 2, ktGlow );
		EM_Upper( emKotorTabs[t], up, sizeof( up ) );
		EM_Text( x + 2, tabY + 7, tabW - 4, up, on ? emWhite : ktText, em.titleFont, 0.38f, 1 );
		if ( EM_Clicked( x, tabY, tabW, tabH ) )
			EM_PickCat( t );
	}
	// the tab's name, lines either side
	EM_Upper( em.panel != EP_NONE ? EM_PanelTitle() : emKotorTabs[em.cat], up, sizeof( up ) );
	{
		const float tw = EM_TextW( up, em.titleFont, 0.7f );

		EM_Fill( KT_X + 20, KT_Y + 56, ( KT_W - tw ) * 0.5f - 30, 1, ktSteel );
		EM_Fill( KT_X + KT_W * 0.5f + tw * 0.5f + 10, KT_Y + 56, ( KT_W - tw ) * 0.5f - 30, 1, ktSteel );
		EM_Text( KT_X, KT_Y + 47, KT_W, up, ktGold, em.titleFont, 0.7f, 1 );
	}

	// the list
	EM_KotorBox( listX, listY, listW, listH, qtrue );
	{
		const float inner = listH - 12;
		const qboolean bar = WG_ScrollbarPixels( listX + listW - WG_SCROLL_W - 4, listY + 6, inner, em.content, &em.scroll, em.cursorX, em.cursorY,
			&em.click, em.clickX, em.clickY );
		const float rw = listW - 12 - ( bar ? WG_SCROLL_W + 2 : 0 );

		em.content = em.shown.size() * KT_ROW_H;
		em.scroll = Com_Clamp( 0, Q_max( 0.0f, em.content - inner ), em.scroll );
		for ( int i = 0; i < (int)em.shown.size(); i++ ) {
			const emEntry_t &e = emEntries[em.shown[i]];
			const float y = listY + 6 + i * KT_ROW_H - em.scroll;
			const qboolean on = (qboolean)( i == em.sel );
			const qboolean hover = EM_In( listX + 6, y, rw, KT_ROW_H - 2 );

			if ( y < listY + 5 || y + KT_ROW_H > listY + 6 + inner + 1 )
				continue;
			if ( on || hover )
				EM_Fill( listX + 6, y, rw, KT_ROW_H - 2, on ? ktRow : ktRowHover );
			if ( on )
				EM_Fill( listX + 6, y, 2, KT_ROW_H - 2, ktGlow );
			EM_Fill( listX + 14, y + KT_ROW_H * 0.5f - 3, EM_Wide( 4 ), 4, on ? ktGold : ktSteel );
			EM_Text( listX + 14 + EM_Wide( 10 ), y + 5, rw - 24, e.name, on ? emWhite : ktText, em.font, 0.75f, 0 );
			// KOTOR picks on a click and does on the next, or Enter, or Open
			if ( EM_Clicked( listX + 6, y, rw, KT_ROW_H - 2 ) ) {
				if ( on ) {
					EM_Do( e );
					if ( !em.open )
						return;
				} else {
					em.sel = i;
					em.panel = EP_NONE;
				}
			}
		}
	}

	// what's picked, told about; or a panel
	EM_KotorBox( infoX, listY, infoW, listH, qtrue );
	if ( em.panel != EP_NONE ) {
		if ( EM_Button( infoX + 10, listY + 10, EM_Wide( 56 ), 18, "Back", 0 ) )
			em.panel = EP_NONE;
		else
			EM_DrawPanel( infoX + 12, listY + 36, infoW - 24, listH - 46 );
	} else {
		float y = listY + 12;

		// the Character tab: you
		if ( em.cat == KT_CHARACTER ) {
			const float ph = 150;

			EM_Fill( infoX + 10, y, infoW - 20, ph, ktSteelDark );
			EM_Frame( infoX + 10, y, infoW - 20, ph, 1, ktSteel );
			CL_Presets_DrawPortrait( infoX + 12, y + 2, infoW - 24, ph - 4, 20.0f + sinf( cls.realtime * 0.0015f ) * 25.0f );
			{
				char name[MAX_NAME_LENGTH];

				Q_strncpyz( name, Cvar_VariableString( "name" ), sizeof( name ) );
				Q_StripColor( name );
				EM_Text( infoX + 10, y + ph + 4, infoW - 20, name, ktGold, em.font, 0.85f, 1 );
				EM_Text( infoX + 10, y + ph + 20, infoW - 20, Cvar_VariableString( "model" ), ktDim, em.font, 0.6f, 1 );
			}
			y += ph + 38;
		}
		if ( !em.shown.empty() ) {
			const emEntry_t &e = emEntries[em.shown[em.sel]];
			wgStyle_t style;
			int lines;

			EM_Upper( e.name, up, sizeof( up ) );
			EM_Text( infoX + 12, y, infoW - 24, up, emWhite, em.titleFont, 0.5f, 0 );
			EM_Fill( infoX + 12, y + 15, infoW - 24, 1, ktSteel );
			EM_StyleWG( &style, ktText, 0.72f );
			lines = WG_TextLines( &style, e.desc, infoW - 24 );
			WG_DrawText( &style, infoX + 12, y + 22, infoW - 24, e.desc, 0, lines, ktText );
			EM_Text( infoX + 12, y + 26 + lines * style.lineH, infoW - 24, EM_KindNote( e ), ktDim, em.font, 0.6f, 0 );
			if ( EM_Button( infoX + infoW - EM_Wide( 90 ) - 10, listY + listH - 30, EM_Wide( 90 ), 20, "Open", 1 ) ) {
				EM_Do( e );
				if ( !em.open )
					return;
			}
		}
	}

	// along the bottom
	if ( EM_Button( KT_X + 10, KT_Y + KT_H - 32, EM_Wide( 100 ), 22, "Resume", 1 ) ) {
		EM_Close();
		return;
	}
	if ( EM_Button( KT_X + KT_W - 10 - EM_Wide( 100 ), KT_Y + KT_H - 32, EM_Wide( 100 ), 22, "Exit", 2 ) )
		em.panel = EP_LEAVE;
	if ( em.message[0] && cls.realtime - em.messageTime < 5000 )
		EM_Text( KT_X + 120, KT_Y + KT_H - 27, KT_W - 240, em.message, ktGold, em.font, 0.6f, 1 );
	else
		EM_Text( KT_X + 120, KT_Y + KT_H - 27, KT_W - 240, "Left / Right: tabs   Up / Down: pick   Enter: open   Esc: close", ktDim, em.font, 0.58f, 1 );
}

void CL_EscMenu_Draw( void ) {
	if ( !em.open )
		return;
	if ( cls.state != CA_ACTIVE ) {
		EM_Close();
		return;
	}
	if ( em.style == EM_KOTOR )
		EM_DrawKotor();
	else
		EM_DrawModern();
	em.click = qfalse;
	if ( em.open )
		re->DrawStretchPic( em.cursorX, em.cursorY, 32, 32, 0, 0, 1, 1, cls.cursorShader );
}

/*
===============================================================================

Keys and the mouse

===============================================================================
*/

// the picked one kept in sight
static void EM_SeeSel( void ) {
	const float rowH = em.style == EM_KOTOR ? KT_ROW_H : MD_CARD_H + MD_CARD_GAP;
	const float view = em.style == EM_KOTOR ? KT_H - 74 - 44 - 12 : SCREEN_HEIGHT - 64 - 34;
	const float top = em.sel * rowH;

	if ( top < em.scroll )
		em.scroll = top;
	else if ( top + rowH > em.scroll + view )
		em.scroll = top + rowH - view;
}

void CL_EscMenu_KeyEvent( int key, qboolean down ) {
	const int num = em.style == EM_KOTOR ? KT_NUM : MC_NUM;

	if ( !em.open || !down )
		return;
	if ( key == A_MOUSE1 ) {
		em.click = qtrue;
		em.clickX = em.cursorX;
		em.clickY = em.cursorY;
		if ( em.typing && !( em.style == EM_MODERN && em.cursorY >= 20 && em.cursorY < 38 && em.cursorX >= SCREEN_WIDTH - 22 - EM_Wide( 170 ) ) )
			em.typing = qfalse;
		return;
	}
	if ( key == A_MWHEELUP || key == A_MWHEELDOWN ) {
		em.scroll += key == A_MWHEELUP ? -40.0f : 40.0f;
		return;
	}
	if ( em.typing ) {
		if ( key == A_ENTER || key == A_KP_ENTER ) {
			em.typing = qfalse;
			if ( !em.shown.empty() )
				EM_Do( emEntries[em.shown[em.sel]] );
			return;
		}
		if ( key == A_CURSOR_UP || key == A_CURSOR_DOWN ) {
			em.typing = qfalse;
		} else {
			if ( WG_EditKey( em.search, sizeof( em.search ), key, NULL ) & WG_EDIT_CHANGED ) {
				em.sel = 0;
				em.scroll = 0;
				EM_BuildShown();
			}
			return;
		}
	}
	switch ( key ) {
	case A_CURSOR_UP:
	case A_CURSOR_DOWN:
		if ( em.panel != EP_NONE || em.shown.empty() )
			return;
		em.sel = Com_Clampi( 0, (int)em.shown.size() - 1, em.sel + ( key == A_CURSOR_UP ? -1 : 1 ) );
		S_StartLocalSound( em.moveSound, CHAN_LOCAL_SOUND );
		EM_SeeSel();
		break;
	case A_HOME:
	case A_END:
		if ( em.panel == EP_NONE && !em.shown.empty() ) {
			em.sel = key == A_HOME ? 0 : (int)em.shown.size() - 1;
			EM_SeeSel();
		}
		break;
	case A_TAB:
		EM_PickCat( em.cat + ( Key_IsDown( A_SHIFT ) ? -1 : 1 ) );
		break;
	case A_CURSOR_LEFT:
	case A_CURSOR_RIGHT:
		EM_PickCat( em.cat + ( key == A_CURSOR_LEFT ? -1 : 1 ) );
		break;
	case A_ENTER:
	case A_KP_ENTER:
		if ( em.panel == EP_NONE && !em.shown.empty() ) {
			S_StartLocalSound( em.clickSound, CHAN_LOCAL_SOUND );
			EM_Do( emEntries[em.shown[em.sel]] );
		}
		break;
	case A_BACKSPACE:
		if ( em.panel != EP_NONE )
			em.panel = EP_NONE;
		break;
	default:
		break;
	}
	(void)num;
}

// typing anywhere in Modern searches
void CL_EscMenu_CharEvent( int ch ) {
	if ( !em.open || em.style != EM_MODERN || em.panel != EP_NONE )
		return;
	if ( !em.typing ) {
		if ( ch <= ' ' || ch >= 127 )
			return;
		em.typing = qtrue;
		WG_EditReset();
	}
	if ( WG_EditChar( em.search, sizeof( em.search ), ch, NULL ) ) {
		em.sel = 0;
		em.scroll = 0;
		EM_BuildShown();
	}
}

void CL_EscMenu_MouseEvent( int dx, int dy ) {
	em.cursorX = Com_Clamp( 0, SCREEN_WIDTH, em.cursorX + dx );
	em.cursorY = Com_Clamp( 0, SCREEN_HEIGHT, em.cursorY + dy );
}

// Esc: the search cleared, then a panel put away, then the menu
void CL_EscMenu_Escape( void ) {
	if ( em.search[0] || em.typing ) {
		em.search[0] = '\0';
		em.typing = qfalse;
		EM_BuildShown();
		return;
	}
	if ( em.panel != EP_NONE ) {
		em.panel = EP_NONE;
		return;
	}
	EM_Close();
}

// /escmenu [modern|kotor|classic] [category]: opens it in that style, once, on that category or tab;
// classic opens the game's own
static void EM_Cmd_f( void ) {
	const char *arg = Cmd_Argv( 1 ), *cat = Cmd_Argv( 2 );

	if ( em.open && !arg[0] ) {
		EM_Close();
		return;
	}
	if ( !Q_stricmp( arg, "classic" ) ) {
		EM_Close();
		if ( cls.state == CA_ACTIVE )
			UIVM_SetActiveMenu( UIMENU_INGAME );
		return;
	}
	if ( arg[0] && Q_stricmp( arg, "modern" ) && Q_stricmp( arg, "kotor" ) ) {
		Com_Printf( "escmenu [modern|kotor|classic] [category] : the menu Esc opens, in that style, on that category or tab\n" );
		Com_Printf( "  (cl_escMenu picks what Esc opens: 0 classic, 1 modern, 2 kotor)\n" );
		return;
	}
	if ( !EM_Open( !Q_stricmp( arg, "kotor" ) ? EM_KOTOR : !Q_stricmp( arg, "modern" ) ? EM_MODERN
		: cl_escMenu && cl_escMenu->integer == EM_KOTOR ? EM_KOTOR : EM_MODERN ) ) {
		Com_Printf( "Esc menu: join a server first\n" );
		return;
	}
	for ( int c = 0; cat[0] && c < ( em.style == EM_KOTOR ? KT_NUM : MC_NUM ); c++ ) {
		if ( !Q_stricmp( cat, em.style == EM_KOTOR ? emKotorTabs[c] : emModernCats[c] ) )
			EM_PickCat( c );
	}
}

void CL_EscMenu_Init( void ) {
	cl_escMenu = Cvar_Get( "cl_escMenu", "0", CVAR_ARCHIVE_ND, "The menu Esc opens in game: 0 the game's own, 1 Modern, 2 KOTOR" );
	Cmd_AddCommand( EM_CMD, EM_Cmd_f, "The menu Esc opens, in a style: modern, kotor or classic" );
	em.camDrag = -1;
	EM_BuildEntries();
}
