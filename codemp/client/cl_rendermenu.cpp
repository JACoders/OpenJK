/*
===========================================================================
Copyright (C) 2026 OpenJK-rpmod contributors

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

// The renderer's settings in JKA's own Setup menu: a RENDERER button under DEFAULTS, and its page.
//
// Nothing is shipped for it: the Setup menus (base JKA's, or the ones RPMod's pk3 has) are changed as
// the UI reads them. The button and the rows are copies of the menu's own DEFAULTS button and VIDEO
// QUALITY row, so they look as the rest of it does whoever made it; a menu without those is left be.
//
// Two pages: JKA's Setup menu has room for 15 rows, and a button at the end of each goes to the other.
// With Vulkan, the effects that need Post Effects (r_fbo) show only once it's on, under it, and the
// second page with them: a row shows for one cvar only, so ui_rd_postfx says both are so.
//
// The camera panel's settings can be saved by name and loaded again (campreset, camerapresets.txt):
// a name box, < and > going through the saved ones and loading each, Save and Delete.
//
// Setup's Game Options get a Camera Settings row (ui_cameraSettings), and the in-game menu, the bar
// Esc opens: no Vote, Call Vote or Add Bot, a PRESETS button between Profile and Controls (see
// cl_presets.cpp), and with Camera Settings on a panel of the third person camera's settings, the
// game's own cg_thirdPerson* and cg_fov. QUICK CONNECT and TOOLS open panels under the bar: the
// servers, and every tool this client adds (Feedback with them), five a page.
//
// The page's rows are ui_rd_* copies of the settings: most of them only take with a vid_restart, and
// a latched cvar reads as it was until then. "renderersettings sync" fills the copies as the page
// opens, and the video restart's Yes runs "renderersettings apply" before its vid_restart.

#include "client.h"
#include "cl_uiapi.h"
#include "cl_escmenu.h"
#include <string>
#include <vector>

// botlib/l_script.cpp: a script file read, before it's parsed
extern const char *( *LoadScriptFile_Patch )( const char *filename, const char *text, int *length );

#define RM_GROUP		"renderer"		// the first page
#define RM_GROUP2		"renderer2"		// the second
#define RM_BUTTON		"renderermenubutton"
#define RM_GLOW			"rendererbutton_glow"
#define RM_VULKAN		"rd-vulkan"
#define RM_POSTFX		"ui_rd_postfx"	// 1 while the page has Vulkan picked and Post Effects on

// The router: the Esc menus open JKA's own pages (Controls, Setup, its RENDERER page) through the
// in-game menu, as RPMod's UI can be told to open only that. Its onOpen ends with a gate: past it
// only while the engine routes (ui_escNotRouting 0), each route's hidden item is focused, and its
// onFocus passes its own gate (ui_escNot_<route> 0) to open its page. A gate is JKA's own defer
// RulesBackout: what follows runs only with ui_rules_backout 0, which setcvartocvar copies in
#define IM_ROUTE_GATE	"ui_escNotRouting"
#define IM_ROUTE_CVAR	"ui_escNot_"
#define IM_GATE( cvar )	"setcvartocvar ui_rules_backout " cvar " ; defer RulesBackout ;"

static const struct {
	const char	*route;
	const char	*open;		// the page it opens
	qboolean	keepGate;	// its gate's passed on to the page (the RENDERER page, in Setup's onOpen)
} imRoutes[] = {
	{ "controls",	"ingame_controls",	qfalse },
	{ "setup",		"ingame_setup",		qfalse },
	{ "renderer",	"ingame_setup",		qtrue },
};

typedef enum {
	RM_SHOW_ALWAYS,
	RM_SHOW_VULKAN,		// when Vulkan is picked
	RM_SHOW_POSTFX		// when Vulkan is picked and Post Effects on
} rmShow_t;

typedef struct {
	const char	*ui;			// the page's copy; NULL for the button to the other page
	const char	*real;			// the setting
	const char	*def;			// when the renderer that has it hasn't made it yet
	const char	*label;
	const char	*desc;
	const char	*list;			// cvarStrList (commas between) or cvarFloatList (none), braces and all
	rmShow_t	show;
	int			page;			// 0 or 1
} rmRow_t;

static const rmRow_t rmRows[] = {
	{ "ui_rd_renderer", "cl_renderer", "rd-vanilla", "Renderer:",
		"OpenGL is the usual one; Vulkan is TaystJK's, with more effects (experimental)",
		"cvarStrList { \"OpenGL\" , \"rd-vanilla\" , \"Vulkan\" , \"rd-vulkan\" }", RM_SHOW_ALWAYS, 0 },
	{ "ui_rd_msaa", "r_ext_multisample", "0", "Anti-aliasing:",
		"Smooths jagged edges; more costs more frames",
		"cvarFloatList { \"Off\" 0 \"2x\" 2 \"4x\" 4 \"8x\" 8 }", RM_SHOW_ALWAYS, 0 },
	{ "ui_rd_glow", "r_DynamicGlow", "0", "Dynamic Glow:",
		"Glowing lights and sabers; with Vulkan it needs Post Effects",
		"cvarFloatList { \"Off\" 0 \"On\" 1 }", RM_SHOW_ALWAYS, 0 },
	{ "ui_rd_vbo", "r_vbo", "0", "World Caching:",
		"Keeps the map on the graphics card: more frames per second",
		"cvarFloatList { \"Off\" 0 \"On\" 1 }", RM_SHOW_VULKAN, 0 },
	{ "ui_rd_vbomodels", "r_vbo_models", "0", "Model Caching:",
		"Keeps models on the graphics card: more frames per second",
		"cvarFloatList { \"Off\" 0 \"On\" 1 }", RM_SHOW_VULKAN, 0 },
	{ "ui_rd_fbo", "r_fbo", "0", "Post Effects:",
		"Draws to a picture first: on, it shows bloom, HDR, glow and the rest of the effects here",
		"cvarFloatList { \"Off\" 0 \"On\" 1 }", RM_SHOW_VULKAN, 0 },
	// what needs Post Effects, under it
	{ "ui_rd_bloom", "r_bloom", "0", "Bloom:",
		"Bright light bleeds into its surroundings",
		"cvarFloatList { \"Off\" 0 \"On\" 1 }", RM_SHOW_POSTFX, 0 },
	{ "ui_rd_hdr", "r_hdr", "1", "HDR:",
		"More colour depth in the picture, less banding",
		"cvarFloatList { \"Off\" 0 \"On\" 1 }", RM_SHOW_POSTFX, 0 },
	{ "ui_rd_dither", "r_dither", "0", "Dithering:",
		"Hides banding in smooth gradients",
		"cvarFloatList { \"Off\" 0 \"On\" 1 }", RM_SHOW_POSTFX, 0 },
	{ "ui_rd_ssao", "r_ssao", "0", "Ambient Occlusion:",
		"Darkens corners, creases and the ground under things",
		"cvarFloatList { \"Off\" 0 \"On\" 1 }", RM_SHOW_POSTFX, 0 },
	{ "ui_rd_ssaostrength", "r_ssaoIntensity", "0.6", "AO Strength:",
		"How dark ambient occlusion makes corners and creases; lower it if the picture looks too dark",
		"cvarFloatList { \"Low\" 0.3 \"Medium\" 0.6 \"Strong\" 1 }", RM_SHOW_POSTFX, 0 },
	{ NULL, NULL, NULL, "More Settings >",
		"Supersampling, soft particles, FXAA, sharpening, sun shafts, tone mapping, motion blur",
		NULL, RM_SHOW_POSTFX, 0 },
	// the second page: Vulkan's own additions (rd-vulkan's vk_rpmod_post.cpp)
	{ "ui_rd_ssaa", "r_ext_supersample", "0", "Supersampling:",
		"Draws the picture at twice the size and scales it down: smooths every edge, costs about four times the drawing",
		"cvarFloatList { \"Off\" 0 \"On\" 1 }", RM_SHOW_POSTFX, 1 },
	{ "ui_rd_soft", "r_softParticles", "0", "Soft Particles:",
		"Smoke and fire fade where they meet walls instead of cutting into them",
		"cvarFloatList { \"Off\" 0 \"On\" 1 }", RM_SHOW_POSTFX, 1 },
	{ "ui_rd_fxaa", "r_fxaa", "0", "FXAA:",
		"Smooths edges over the finished picture, grass and fences too",
		"cvarFloatList { \"Off\" 0 \"On\" 1 }", RM_SHOW_POSTFX, 1 },
	{ "ui_rd_sharpen", "r_sharpen", "0", "Sharpening:",
		"Crisper detail, against the softness of FXAA or supersampling",
		"cvarFloatList { \"Off\" 0 \"Low\" 0.4 \"High\" 0.8 }", RM_SHOW_POSTFX, 1 },
	{ "ui_rd_shafts", "r_sunShafts", "0", "Sun Shafts:",
		"Beams of light from the sun through gaps, on maps with a sun",
		"cvarFloatList { \"Off\" 0 \"On\" 1 }", RM_SHOW_POSTFX, 1 },
	{ "ui_rd_shaftsstrength", "r_sunShaftsIntensity", "1", "Sun Shaft Strength:",
		"How bright the sun shafts are",
		"cvarFloatList { \"Low\" 0.5 \"Medium\" 1 \"High\" 2 \"Very High\" 3 }", RM_SHOW_POSTFX, 1 },
	{ "ui_rd_tonemap", "r_tonemap", "0", "Tone Mapping:",
		"Bright light rolls off instead of clipping to white; Film (ACES) darkens the shadows too",
		"cvarFloatList { \"Off\" 0 \"Soft\" 1 \"Film\" 2 }", RM_SHOW_POSTFX, 1 },
	{ "ui_rd_mblur", "r_motionBlur", "0", "Motion Blur:",
		"The picture smears as the camera moves or turns",
		"cvarFloatList { \"Off\" 0 \"Low\" 0.3 \"Medium\" 0.6 \"High\" 1 }", RM_SHOW_POSTFX, 1 },
	{ NULL, NULL, NULL, "< Back",
		"The renderer, anti-aliasing, glow, post effects and the rest",
		NULL, RM_SHOW_POSTFX, 1 },
};
static const int rmNumRows = (int)ARRAY_LEN( rmRows );

/*
================================================================================
settings
================================================================================
*/

// what it is, or will be at the next vid_restart
static const char *RM_Effective( const rmRow_t *row ) {
	cvar_t *cv = Cvar_Get( row->real, row->def, 0 );

	return cv->latchedString ? cv->latchedString : cv->string;
}

static void RM_Settings_f( void ) {
	const char *how = Cmd_Argv( 1 );

	if ( !Q_stricmp( how, "sync" ) ) {
		for ( int i = 0; i < rmNumRows; i++ ) {
			if ( rmRows[i].ui )
				Cvar_Set( rmRows[i].ui, RM_Effective( &rmRows[i] ) );
		}
	} else if ( !Q_stricmp( how, "apply" ) ) {
		for ( int i = 0; i < rmNumRows; i++ ) {
			const char *want;

			if ( !rmRows[i].ui )
				continue;
			want = Cvar_VariableString( rmRows[i].ui );

			if ( want[0] && Q_stricmp( want, RM_Effective( &rmRows[i] ) ) )
				Cvar_Set( rmRows[i].real, want );
		}
	} else {
		Com_Printf( "renderersettings sync : the Setup menu's RENDERER page shows the settings as they are\n" );
		Com_Printf( "renderersettings apply : they become what the page has, at the next vid_restart\n" );
	}
}

/*
================================================================================
reading the menu
================================================================================
*/

static const size_t RM_NONE = std::string::npos;

static qboolean RM_WordChar( char c ) {
	return (qboolean)( isalnum( (unsigned char)c ) || c == '_' );
}

// the word on its own, any case, from `from` up to `to`
static size_t RM_FindWord( const std::string &s, const char *word, size_t from, size_t to = RM_NONE ) {
	const size_t len = strlen( word );

	if ( to == RM_NONE || to > s.size() )
		to = s.size();
	for ( size_t i = from; i + len <= to; i++ ) {
		if ( Q_stricmpn( s.c_str() + i, word, (int)len ) )
			continue;
		if ( i > 0 && RM_WordChar( s[i - 1] ) )
			continue;
		if ( i + len < s.size() && RM_WordChar( s[i + len] ) )
			continue;
		return i;
	}
	return RM_NONE;
}

static size_t RM_SkipSpace( const std::string &s, size_t at ) {
	while ( at < s.size() && ( isspace( (unsigned char)s[at] ) || s[at] == ';' ) )
		at++;
	return at;
}

// the token at `at`: a quoted string or a run of anything else; its end in *end
static std::string RM_Token( const std::string &s, size_t at, size_t *end ) {
	size_t i = RM_SkipSpace( s, at ), start = i;

	if ( i < s.size() && s[i] == '"' ) {
		for ( i++; i < s.size() && s[i] != '"'; i++ )
			;
		if ( end )
			*end = i < s.size() ? i + 1 : i;
		return s.substr( start + 1, i - start - 1 );
	}
	while ( i < s.size() && !isspace( (unsigned char)s[i] ) && s[i] != '{' && s[i] != '}' && s[i] != ';' )
		i++;
	if ( end )
		*end = i;
	return s.substr( start, i - start );
}

// just past the brace that closes the one at `open`, quoted text skipped
static size_t RM_BlockEnd( const std::string &s, size_t open ) {
	int depth = 0;

	for ( size_t i = open; i < s.size(); i++ ) {
		if ( s[i] == '"' ) {
			for ( i++; i < s.size() && s[i] != '"'; i++ )
				;
			continue;
		}
		if ( s[i] == '{' )
			depth++;
		else if ( s[i] == '}' && --depth == 0 )
			return i + 1;
	}
	return RM_NONE;
}

typedef struct {
	size_t		start, end;
	std::string	name;
} rmItem_t;

// every itemDef, with its name
static void RM_Items( const std::string &s, std::vector<rmItem_t> &items ) {
	for ( size_t at = RM_FindWord( s, "itemDef", 0 ); at != RM_NONE; ) {
		const size_t open = RM_SkipSpace( s, at + 7 );
		rmItem_t item;
		size_t name;

		if ( open >= s.size() || s[open] != '{' || ( item.end = RM_BlockEnd( s, open ) ) == RM_NONE )
			break;
		item.start = at;
		name = RM_FindWord( s, "name", open, item.end );
		if ( name != RM_NONE )
			item.name = RM_Token( s, name + 4, NULL );
		items.push_back( item );
		at = RM_FindWord( s, "itemDef", item.end );
	}
}

static const rmItem_t *RM_Item( const std::vector<rmItem_t> &items, const char *name ) {
	for ( size_t i = 0; i < items.size(); i++ ) {
		if ( !Q_stricmp( items[i].name.c_str(), name ) )
			return &items[i];
	}
	return NULL;
}

// the item's rect's y, or -1
static float RM_RectY( const std::string &item ) {
	const size_t rect = RM_FindWord( item, "rect", 0 );
	size_t at;

	if ( rect == RM_NONE )
		return -1.0f;
	RM_Token( item, rect + 4, &at );
	return (float)atof( RM_Token( item, at, NULL ).c_str() );
}

/*
================================================================================
changing it
================================================================================
*/

// the token after the word `key`, made `value`; added before the closing brace when there's none
static void RM_SetValue( std::string &item, const char *key, const std::string &value ) {
	const size_t at = RM_FindWord( item, key, 0 );
	size_t start, end;

	if ( at == RM_NONE ) {
		item.insert( item.rfind( '}' ), std::string( key ) + " " + value + "\n" );
		return;
	}
	start = RM_SkipSpace( item, at + strlen( key ) );
	RM_Token( item, start, &end );
	item.replace( start, end - start, value );
}

static void RM_SetRectY( std::string &item, float y ) {
	const size_t rect = RM_FindWord( item, "rect", 0 );
	size_t start, end;

	if ( rect == RM_NONE )
		return;
	RM_Token( item, rect + 4, &start );
	start = RM_SkipSpace( item, start );
	RM_Token( item, start, &end );
	item.replace( start, end - start, va( "%g", y ) );
}

// the word `key` and the token after it taken out
static void RM_RemoveValue( std::string &item, const char *key ) {
	const size_t at = RM_FindWord( item, key, 0 );
	size_t end;

	if ( at == RM_NONE )
		return;
	RM_Token( item, at + strlen( key ), &end );
	item.erase( at, end - at );
}

// every `from`, on its own, made `to`
static void RM_ReplaceWord( std::string &s, const char *from, const std::string &to ) {
	for ( size_t at = RM_FindWord( s, from, 0 ); at != RM_NONE; at = RM_FindWord( s, from, at + to.size() ) )
		s.replace( at, strlen( from ), to );
}

// "key { ... }" taken out, or made `with`
static void RM_ReplaceBlock( std::string &item, const char *key, const std::string &with ) {
	const size_t at = RM_FindWord( item, key, 0 );
	size_t open, end;

	if ( at == RM_NONE ) {
		if ( !with.empty() )
			item.insert( item.rfind( '}' ), with + "\n" );
		return;
	}
	open = RM_SkipSpace( item, at + strlen( key ) );
	if ( open >= item.size() || item[open] != '{' || ( end = RM_BlockEnd( item, open ) ) == RM_NONE )
		return;
	item.replace( at, end - at, with );
}

// "verb target" commands in the menu's scripts: `add` after each
static void RM_AfterCommand( std::string &s, const char *verb, const char *target, const char *add ) {
	for ( size_t at = RM_FindWord( s, verb, 0 ); at != RM_NONE; at = RM_FindWord( s, verb, at + 1 ) ) {
		size_t end;
		const std::string what = RM_Token( s, at + strlen( verb ), &end );

		if ( Q_stricmp( what.c_str(), target ) )
			continue;
		s.insert( end, add );
		at = end + strlen( add ) - 1;
	}
}

// "verb target" commands made `with`
static void RM_ReplaceCommand( std::string &s, const char *verb, const char *target, const char *with ) {
	for ( size_t at = RM_FindWord( s, verb, 0 ); at != RM_NONE; at = RM_FindWord( s, verb, at + 1 ) ) {
		size_t end;
		const std::string what = RM_Token( s, at + strlen( verb ), &end );

		if ( Q_stricmp( what.c_str(), target ) )
			continue;
		s.replace( at, end - at, with );
		at += strlen( with ) - 1;
	}
}

static std::string RM_Quote( const char *text ) {
	return std::string( "\"" ) + text + "\"";
}

/*
================================================================================
the patch
================================================================================
*/

static std::string rmPatched;

// a Setup menu with a RENDERER page; NULL when it's not one, or not one this can find its way in
static const char *RM_Patch( const char *filename, const char *text, int *length ) {
	const char *base = COM_SkipPath( (char *)filename );
	std::string s( text, *length );
	std::vector<rmItem_t> items;
	const rmItem_t *defaults, *glow, *video1, *video2, *row, *high1, *high2;
	std::string add;
	float buttonStep, rowStep, rowY;
	size_t menu, menuOpen, menuEnd;

	if ( Q_stricmp( base, "setup.menu" ) && Q_stricmp( base, "ingame_setup.menu" ) )
		return NULL;
	if ( RM_FindWord( s, RM_BUTTON, 0 ) != RM_NONE )
		return NULL;	// it has one already

	RM_Items( s, items );
	defaults = RM_Item( items, "gamedefaultsmenubutton" );
	glow = RM_Item( items, "gamedefaultsbutton_glow" );
	video1 = RM_Item( items, "video1menubutton" );
	video2 = RM_Item( items, "video2menubutton" );
	row = RM_Item( items, "graphics" );
	high1 = RM_Item( items, "highlight1" );
	high2 = RM_Item( items, "highlight2" );
	menu = RM_FindWord( s, "menuDef", 0 );
	if ( !defaults || !glow || !video1 || !video2 || !row || !high1 || !high2 || menu == RM_NONE ) {
		Com_DPrintf( "Renderer settings: %s isn't laid out as JKA's Setup menu; left as it is\n", filename );
		return NULL;
	}
	menuOpen = RM_SkipSpace( s, menu + 7 );
	if ( menuOpen >= s.size() || s[menuOpen] != '{' || ( menuEnd = RM_BlockEnd( s, menuOpen ) ) == RM_NONE )
		return NULL;

	{
		const std::string v1 = s.substr( video1->start, video1->end - video1->start );
		const std::string v2 = s.substr( video2->start, video2->end - video2->start );
		const std::string h1 = s.substr( high1->start, high1->end - high1->start );
		const std::string h2 = s.substr( high2->start, high2->end - high2->start );
		const std::string r = s.substr( row->start, row->end - row->start );

		buttonStep = RM_RectY( v2 ) - RM_RectY( v1 );
		rowStep = RM_RectY( h2 ) - RM_RectY( h1 );
		rowY = RM_RectY( r );
		if ( buttonStep <= 0.0f || rowStep <= 0.0f || rowY < 0.0f )
			return NULL;
	}

	// the button and its glow, a step under DEFAULTS
	{
		std::string button = s.substr( defaults->start, defaults->end - defaults->start );
		std::string shine = s.substr( glow->start, glow->end - glow->start );
		const float y = RM_RectY( button ) + buttonStep;

		RM_ReplaceWord( button, "gamedefaultsmenubutton", RM_BUTTON );
		RM_ReplaceWord( button, "gamedefaultsbutton_glow", RM_GLOW );
		RM_SetValue( button, "text", RM_Quote( "RENDERER" ) );
		RM_SetValue( button, "descText", RM_Quote( "OpenGL or Vulkan, and their graphics settings" ) );
		RM_SetRectY( button, y );
		// its first page instead of DEFAULTS', with the settings as they are
		RM_ReplaceCommand( button, "show", "defaults", "hide defaults hide " RM_GROUP2 " show " RM_GROUP " exec \"renderersettings sync\"" );

		RM_ReplaceWord( shine, "gamedefaultsbutton_glow", RM_GLOW );
		RM_SetRectY( shine, y );

		add += "\n" + shine + "\n" + button + "\n";
	}

	// the rows, as VIDEO QUALITY is, each page from the top; the buttons to the other page as they are
	// but for the setting
	for ( int i = 0, onPage[2] = { 0, 0 }; i < rmNumRows; i++ ) {
		const rmRow_t *rr = &rmRows[i];
		std::string item = s.substr( row->start, row->end - row->start );
		const int at = onPage[rr->page]++;
		char highlight[32];

		RM_SetValue( item, "name", va( "rd_row%i", i ) );
		RM_SetValue( item, "group", rr->page ? RM_GROUP2 : RM_GROUP );
		RM_SetValue( item, "text", RM_Quote( rr->label ) );
		RM_SetValue( item, "descText", RM_Quote( rr->desc ) );
		RM_SetValue( item, "visible", "0" );
		RM_SetRectY( item, rowY + rowStep * at );
		RM_ReplaceBlock( item, "cvarFloatList", "" );
		RM_ReplaceBlock( item, "cvarStrList", "" );
		Com_sprintf( highlight, sizeof( highlight ), "highlight%i", at + 1 );
		RM_ReplaceWord( item, "highlight1", highlight );
		if ( !rr->ui ) {
			RM_RemoveValue( item, "cvar" );
			RM_SetValue( item, "type", "ITEM_TYPE_BUTTON" );
			RM_ReplaceBlock( item, "action", rr->page
				? "action { play \"sound/interface/button1\" hide " RM_GROUP2 " show " RM_GROUP " }"
				: "action { play \"sound/interface/button1\" hide " RM_GROUP " show " RM_GROUP2 " }" );
		} else {
			RM_SetValue( item, "cvar", RM_Quote( rr->ui ) );
			RM_ReplaceBlock( item, "action", "action { play \"sound/interface/button1\" show applyChanges }" );
			item.insert( item.rfind( '}' ), std::string( rr->list ) + "\n" );
		}
		if ( rr->show == RM_SHOW_VULKAN )
			item.insert( item.rfind( '}' ), "cvarTest \"ui_rd_renderer\" showCvar { \"" RM_VULKAN "\" }\n" );
		else if ( rr->show == RM_SHOW_POSTFX )
			item.insert( item.rfind( '}' ), "cvarTest \"" RM_POSTFX "\" showCvar { \"1\" }\n" );
		add += item + "\n";
	}

	// Game Options: Camera Settings under the language button, as its Draw Crosshair row is
	{
		const rmItem_t *cross = RM_Item( items, "draw_crosshair" );
		const rmItem_t *lang = RM_Item( items, "langapply" );

		if ( cross ) {
			std::string item = s.substr( cross->start, cross->end - cross->start );
			const float crossY = RM_RectY( item );
			// the row after the language button, or the 14th
			const int at = lang ? (int)( ( RM_RectY( s.substr( lang->start, lang->end - lang->start ) ) + 32.0f - crossY ) / rowStep + 0.999f ) : 13;

			RM_SetValue( item, "name", "camerasettings" );
			RM_SetValue( item, "text", RM_Quote( "Camera Settings:" ) );
			RM_SetValue( item, "cvar", RM_Quote( "ui_cameraSettings" ) );
			RM_SetValue( item, "descText", RM_Quote( "In the in-game menu (Esc), the third person camera's settings beside it" ) );
			RM_ReplaceBlock( item, "cvarFloatList", "cvarFloatList { \"Off\" 0 \"On\" 1 }" );
			RM_SetRectY( item, crossY + rowStep * at );
			RM_ReplaceWord( item, "highlight1", va( "highlight%i", at + 1 ) );
			add += "\n" + item + "\n";

			// Discord Status, the row under it
			item = s.substr( cross->start, cross->end - cross->start );
			RM_SetValue( item, "name", "discordstatus" );
			RM_SetValue( item, "text", RM_Quote( "Discord Status:" ) );
			RM_SetValue( item, "cvar", RM_Quote( "cl_discord" ) );
			RM_SetValue( item, "descText", RM_Quote( "Your Discord profile shows the server, your character, the map and the players" ) );
			RM_ReplaceBlock( item, "cvarFloatList", "cvarFloatList { \"Off\" 0 \"On\" 1 }" );
			RM_SetRectY( item, crossY + rowStep * ( at + 1 ) );
			RM_ReplaceWord( item, "highlight1", va( "highlight%i", at + 2 ) );
			add += "\n" + item + "\n";

			// Esc Menu, under that: the menu Esc opens in game (cl_escmenu.cpp)
			item = s.substr( cross->start, cross->end - cross->start );
			RM_SetValue( item, "name", "escmenustyle" );
			RM_SetValue( item, "text", RM_Quote( "Esc Menu:" ) );
			RM_SetValue( item, "cvar", RM_Quote( "cl_escMenu" ) );
			RM_SetValue( item, "descText", RM_Quote( "The menu Esc opens in game: Modern, KOTOR's look, or the game's own" ) );
			RM_ReplaceBlock( item, "cvarFloatList", "cvarFloatList { \"Classic\" 0 \"Modern\" 1 \"KOTOR\" 2 }" );
			RM_SetRectY( item, crossY + rowStep * ( at + 2 ) );
			RM_ReplaceWord( item, "highlight1", va( "highlight%i", at + 3 ) );
			add += "\n" + item + "\n";
		}
	}

	// Sound: Background Audio, the row under EAX, a copy of it; hidden while EAX is on, where the
	// EAX picture goes
	{
		const rmItem_t *eax = RM_Item( items, "eax" );
		const rmItem_t *quality = RM_Item( items, "sound_quality" );

		if ( eax && quality ) {
			std::string item = s.substr( eax->start, eax->end - eax->start );
			const float eaxY = RM_RectY( item );
			const float step = eaxY - RM_RectY( s.substr( quality->start, quality->end - quality->start ) );
			const size_t hl = item.find( "highlight" );

			if ( step > 0.0f && hl != std::string::npos ) {
				const int high = atoi( item.c_str() + hl + 9 );

				RM_SetValue( item, "name", "backgroundaudio" );
				RM_SetValue( item, "text", RM_Quote( "Background Audio:" ) );
				RM_SetValue( item, "cvar", RM_Quote( "s_backgroundAudio" ) );
				RM_SetValue( item, "descText", RM_Quote( "Sound keeps playing with the game minimized or another window in front; cinematics always do" ) );
				RM_ReplaceBlock( item, "cvarFloatList", "cvarFloatList { \"Off\" 0 \"On\" 1 }" );
				RM_ReplaceBlock( item, "action", "action { play \"sound/interface/button1.wav\" }" );
				RM_SetRectY( item, eaxY + step );
				RM_ReplaceWord( item, va( "highlight%i", high ), va( "highlight%i", high + (int)( step / rowStep + 0.5f ) ) );
				item.insert( item.rfind( '}' ), "cvarTest \"s_UseOpenAL\" hideCvar { 1 }\n" );
				add += "\n" + item + "\n";
			}
		}
	}

	// the menu's own scripts: its other pages hide these two, and the restart applies them
	RM_AfterCommand( s, "show", "video2", " hide " RM_GROUP " hide " RM_GROUP2 );
	RM_AfterCommand( s, "hide", "video2", " hide " RM_GROUP " hide " RM_GROUP2 );
	RM_AfterCommand( s, "show", "defaults", " hide " RM_GROUP " hide " RM_GROUP2 );
	RM_AfterCommand( s, "hide", "defaults", " hide " RM_GROUP " hide " RM_GROUP2 );
	{
		const std::string apply = "exec \"renderersettings apply\" ";

		for ( size_t at = RM_FindWord( s, "uiScript", 0 ); at != RM_NONE; at = RM_FindWord( s, "uiScript", at + 1 ) ) {
			if ( Q_stricmp( RM_Token( s, at + 8, NULL ).c_str(), "updatevideosetup" ) )
				continue;
			s.insert( at, apply );
			at += apply.size();
		}
	}

	// the new items at the menu's end, after its own
	menu = RM_FindWord( s, "menuDef", 0 );
	menuOpen = RM_SkipSpace( s, menu + 7 );
	if ( ( menuEnd = RM_BlockEnd( s, menuOpen ) ) == RM_NONE )
		return NULL;
	s.insert( menuEnd - 1, add );

	// opened by the Esc menus' router for the RENDERER page: its onOpen ends with what the button
	// does, past the route's gate (see imRoutes). Done last, as it moves what follows; kept short, as
	// JKA's menus keep 2048 characters of a script
	if ( !Q_stricmp( base, "ingame_setup.menu" ) ) {
		const size_t firstItem = RM_FindWord( s, "itemDef", menuOpen );
		const size_t onOpen = RM_FindWord( s, "onOpen", menuOpen, firstItem );
		const size_t open = onOpen != RM_NONE ? RM_SkipSpace( s, onOpen + 6 ) : RM_NONE;
		const size_t openEnd = open != RM_NONE && s[open] == '{' ? RM_BlockEnd( s, open ) : RM_NONE;

		if ( openEnd != RM_NONE ) {
			s.insert( openEnd - 1, "\n" IM_GATE( IM_ROUTE_CVAR "renderer" ) " setcvar " IM_ROUTE_CVAR "renderer 1 ; hide video ; "
				"show " RM_GROUP " ; exec \"renderersettings sync\" ; setitemcolor videomenubutton forecolor 1 .682 0 1 ; "
				"setitemcolor " RM_BUTTON " forecolor 1 1 1 1\n" );
		}
	}

	rmPatched.swap( s );
	*length = (int)rmPatched.size();
	return rmPatched.c_str();
}

/*
================================================================================
the in-game menu
================================================================================
*/

// with the camera settings on, the camera keeps right behind you: a looser one swings round as you
// turn, and looks to those not used to it as if it spins
#define IM_CAM_FOLLOW		"cg_thirdPersonCameraDamp"

void CL_CameraSettings_Frame( void ) {
	if ( Cvar_VariableIntegerValue( "ui_cameraSettings" ) && Cvar_VariableValue( IM_CAM_FOLLOW ) != 1.0f )
		Cvar_Set( IM_CAM_FOLLOW, "1" );
}

// the camera panel's rows: a setting, its label, and for a slider its default, least and most
typedef struct {
	const char	*cvar;
	const char	*label;
	float		def, min, max;
	const char	*list;			// a choice instead of a slider
} imCamRow_t;

static const imCamRow_t imCamRows[] = {
	{ "cg_thirdPerson",				"Third person",	0, 0, 0,		"cvarFloatList { \"Off\" 0 \"On\" 1 }" },
	{ "cg_thirdPersonRange",		"Distance",		80, 20, 300,	NULL },
	{ "cg_thirdPersonAngle",		"Around",		0, -180, 180,	NULL },
	{ "cg_thirdPersonVertOffset",	"Height",		16, -100, 100,	NULL },
	{ "cg_thirdPersonHorzOffset",	"Side",			0, -100, 100,	NULL },
	{ "cg_thirdPersonPitchOffset",	"Tilt",			0, -90, 90,		NULL },
	{ "cg_thirdPersonTargetDamp",	"Aim follow",	0.5f, 0, 1,		NULL },
	{ "cg_thirdPersonAlpha",		"See-through",	1, 0, 1,		NULL },
	{ "cg_fov",						"Field of view",	80, 20, 130,	NULL },
	{ "cg_draw2D",					"HUD",			1, 0, 0,		"cvarFloatList { \"Off\" 0 \"On\" 1 }" },
};

const char *CL_RenderMenu_CameraRow( int i, const char **label, float *def, float *min, float *max, qboolean *toggle ) {
	if ( i < 0 || i >= (int)ARRAY_LEN( imCamRows ) )
		return NULL;
	*label = imCamRows[i].label;
	*def = imCamRows[i].def;
	*min = imCamRows[i].min;
	*max = imCamRows[i].max;
	*toggle = (qboolean)( imCamRows[i].list != NULL );
	return imCamRows[i].cvar;
}

#define IM_CAM_X		436.0f
#define IM_CAM_Y		40.0f
#define IM_CAM_W		198.0f
#define IM_CAM_ROW		17.0f
#define IM_CAM_LABEL	518.0f		// where the labels end and the sliders start
// the camera panel shows with its setting on and nothing over it, Exit's menu (ingame_leave) or the
// quick connect or tools panel (ui_qcOpen): ui_camPanel is them together, kept by CL_RenderMenu_Frame,
// as an item tests one cvar only
#define IM_CAM_CVAR		"ui_camPanel"
#define IM_LEAVE_CVAR	"ui_leaveOpen"	// Exit's menu is open, set by its own onOpen and onClose

#define IM_CAM_SHOW		"cvarTest \"" IM_CAM_CVAR "\" showCvar { \"1\" }\n"

/*
================================================================================
camera presets
================================================================================
*/

#define CP_FILE			"camerapresets.txt"
#define CP_MAX			64
#define CP_NAME_CVAR	"ui_camPresetName"

typedef struct {
	char	name[32];
	char	values[ARRAY_LEN( imCamRows )][16];
} cpPreset_t;

static cpPreset_t	cpPresets[CP_MAX];
static int			cpNum;

static void CP_Load( void ) {
	char *buf;
	const char *p;

	cpNum = 0;
	if ( FS_ReadFile( CP_FILE, (void **)&buf ) <= 0 || !buf )
		return;
	p = buf;
	COM_BeginParseSession( CP_FILE );
	while ( cpNum < CP_MAX ) {
		const char *token = COM_ParseExt( &p, qtrue );
		cpPreset_t *c;

		if ( !token[0] )
			break;
		c = &cpPresets[cpNum];
		memset( c, 0, sizeof( *c ) );
		Q_strncpyz( c->name, token, sizeof( c->name ) );
		// its settings, as many as the line has
		while ( 1 ) {
			char key[64];

			token = COM_ParseExt( &p, qfalse );
			if ( !token[0] )
				break;
			Q_strncpyz( key, token, sizeof( key ) );
			token = COM_ParseExt( &p, qfalse );
			for ( size_t k = 0; k < ARRAY_LEN( imCamRows ); k++ ) {
				if ( !Q_stricmp( key, imCamRows[k].cvar ) )
					Q_strncpyz( c->values[k], token, sizeof( c->values[k] ) );
			}
		}
		cpNum++;
	}
	FS_FreeFile( buf );
}

static void CP_Save( void ) {
	const fileHandle_t f = FS_FOpenFileWrite( CP_FILE );
	const char *head = "// Camera presets: a name and its settings, a line\n";

	if ( !f )
		return;
	FS_Write( head, strlen( head ), f );
	for ( int i = 0; i < cpNum; i++ ) {
		std::string line = va( "\"%s\"", cpPresets[i].name );

		for ( size_t k = 0; k < ARRAY_LEN( imCamRows ); k++ ) {
			if ( cpPresets[i].values[k][0] )
				line += va( " %s \"%s\"", imCamRows[k].cvar, cpPresets[i].values[k] );
		}
		line += "\n";
		FS_Write( line.c_str(), line.size(), f );
	}
	FS_FCloseFile( f );
}

static int CP_Find( const char *name ) {
	for ( int i = 0; i < cpNum; i++ ) {
		if ( !Q_stricmp( cpPresets[i].name, name ) )
			return i;
	}
	return -1;
}

static void CP_Apply( int i ) {
	for ( size_t k = 0; k < ARRAY_LEN( imCamRows ); k++ ) {
		if ( cpPresets[i].values[k][0] )
			Cvar_Set( imCamRows[k].cvar, cpPresets[i].values[k] );
	}
	Cvar_Set( CP_NAME_CVAR, cpPresets[i].name );
	Com_Printf( "Camera preset %s\n", cpPresets[i].name );
}

static int QDECL CP_Compare( const void *a, const void *b ) {
	return Q_stricmp( ( (const cpPreset_t *)a )->name, ( (const cpPreset_t *)b )->name );
}

// campreset save|load|delete [name], next, prev, list: the name, when it's not given, the panel's box
static void CP_Cmd_f( void ) {
	const char *verb = Cmd_Argv( 1 );
	char name[32];
	int i;

	Q_strncpyz( name, Cmd_Argc() > 2 ? Cmd_ArgsFrom( 2 ) : Cvar_VariableString( CP_NAME_CVAR ), sizeof( name ) );
	for ( char *c = name; *c; c++ ) {
		if ( *c == '"' || *c == ';' || *c == '\n' )
			*c = '_';
	}
	CP_Load();
	if ( !Q_stricmp( verb, "save" ) ) {
		if ( !name[0] ) {
			Com_Printf( "campreset: give it a name first\n" );
			return;
		}
		i = CP_Find( name );
		if ( i < 0 ) {
			if ( cpNum >= CP_MAX ) {
				Com_Printf( "campreset: there are as many as there can be\n" );
				return;
			}
			i = cpNum++;
		}
		memset( &cpPresets[i], 0, sizeof( cpPresets[i] ) );
		Q_strncpyz( cpPresets[i].name, name, sizeof( cpPresets[i].name ) );
		for ( size_t k = 0; k < ARRAY_LEN( imCamRows ); k++ )
			Q_strncpyz( cpPresets[i].values[k], Cvar_VariableString( imCamRows[k].cvar ), sizeof( cpPresets[i].values[k] ) );
		qsort( cpPresets, cpNum, sizeof( cpPresets[0] ), CP_Compare );
		CP_Save();
		Cvar_Set( CP_NAME_CVAR, name );
		Com_Printf( "Camera preset %s saved\n", name );
	} else if ( !Q_stricmp( verb, "load" ) ) {
		i = CP_Find( name );
		if ( i < 0 )
			Com_Printf( "campreset: no camera preset called %s\n", name );
		else
			CP_Apply( i );
	} else if ( !Q_stricmp( verb, "delete" ) ) {
		i = CP_Find( name );
		if ( i < 0 ) {
			Com_Printf( "campreset: no camera preset called %s\n", name );
			return;
		}
		memmove( &cpPresets[i], &cpPresets[i + 1], ( cpNum - i - 1 ) * sizeof( cpPresets[0] ) );
		cpNum--;
		CP_Save();
		Cvar_Set( CP_NAME_CVAR, "" );
		Com_Printf( "Camera preset %s deleted\n", name );
	} else if ( !Q_stricmp( verb, "next" ) || !Q_stricmp( verb, "prev" ) ) {
		const int step = !Q_stricmp( verb, "next" ) ? 1 : -1;

		if ( !cpNum ) {
			Com_Printf( "campreset: none saved yet\n" );
			return;
		}
		i = CP_Find( name );
		i = i < 0 ? ( step > 0 ? 0 : cpNum - 1 ) : ( i + step + cpNum ) % cpNum;
		CP_Apply( i );
	} else if ( !Q_stricmp( verb, "list" ) ) {
		for ( i = 0; i < cpNum; i++ )
			Com_Printf( "  %s\n", cpPresets[i].name );
	} else {
		Com_Printf( "usage: campreset save|load|delete [name]   the camera settings by name; the panel's box when no name\n" );
		Com_Printf( "       campreset next|prev                the next or the one before, loaded\n" );
		Com_Printf( "       campreset list\n" );
	}
}

// a button of the panel's preset row
static std::string IM_CamButton( const char *name, const char *label, float x, float y, float w, const char *cmd ) {
	return va( "itemDef {\nname %s\ngroup camerapanel\nstyle WINDOW_STYLE_FILLED\ntype ITEM_TYPE_BUTTON\ntext \"%s\"\n"
		"rect %g %g %g 13\nfont 4\ntextscale .85\ntextalign ITEM_ALIGN_CENTER\ntextalignx %g\nbackcolor .15 .15 .25 .8\n"
		"forecolor 1 .682 0 1\nvisible 1\n" IM_CAM_SHOW "action { play \"sound/interface/button1.wav\" ; exec \"campreset %s\" }\n}\n",
		name, label, x, y, w, w * 0.5f, cmd );
}

static std::string IM_CameraPanel( void ) {
	const float h = 30.0f + IM_CAM_ROW * ( ARRAY_LEN( imCamRows ) + 1 ) + 40.0f;
	std::string out;
	std::string defaults;

	out += va( "itemDef {\nname camerapanel\ngroup camerapanel\nstyle WINDOW_STYLE_FILLED\nrect %g %g %g %g\nborder 1\nbordercolor .5 .5 .7 1\n"
		"backcolor 0 0 .08 .75\nforecolor 1 1 1 1\nvisible 1\ndecoration\n" IM_CAM_SHOW "}\n", IM_CAM_X, IM_CAM_Y, IM_CAM_W, h );
	out += va( "itemDef {\nname camerapanel_title\ngroup camerapanel\nstyle WINDOW_STYLE_EMPTY\ntext \"Camera\"\nrect %g %g %g 14\n"
		"font 2\ntextscale .7\ntextalign ITEM_ALIGN_CENTER\ntextalignx %g\nforecolor 1 .682 0 1\nvisible 1\ndecoration\n" IM_CAM_SHOW "}\n",
		IM_CAM_X, IM_CAM_Y + 3, IM_CAM_W, IM_CAM_W * 0.5f );
	for ( size_t i = 0; i < ARRAY_LEN( imCamRows ); i++ ) {
		const imCamRow_t *r = &imCamRows[i];
		const float y = IM_CAM_Y + 24 + IM_CAM_ROW * i;

		out += "itemDef {\n";
		out += va( "name camerapanel_%i\ngroup camerapanel\n", (int)i );
		if ( r->list )
			out += va( "type ITEM_TYPE_MULTI\ncvar \"%s\"\n%s\n", r->cvar, r->list );
		else
			out += va( "type ITEM_TYPE_SLIDER\ncvarfloat \"%s\" %g %g %g\n", r->cvar, r->def, r->min, r->max );
		out += va( "text \"%s\"\nrect %g %g 110 12\ntextalign ITEM_ALIGN_RIGHT\ntextalignx 0\ntextaligny 0\nfont 4\ntextscale .9\n"
			"forecolor .8 .8 .8 1\nvisible 1\n" IM_CAM_SHOW "action { play \"sound/interface/button1.wav\" }\n}\n", r->label, IM_CAM_LABEL, y );
		defaults += va( "setcvar %s \"%g\" ; ", r->cvar, r->def );
	}
	// all of them as the game has them
	out += va( "itemDef {\nname camerapanel_defaults\ngroup camerapanel\nstyle WINDOW_STYLE_EMPTY\ntype ITEM_TYPE_BUTTON\ntext \"Defaults\"\n"
		"rect %g %g %g 14\nfont 4\ntextscale .9\ntextalign ITEM_ALIGN_CENTER\ntextalignx %g\nforecolor 1 .682 0 1\nvisible 1\n" IM_CAM_SHOW
		"action { play \"sound/interface/button1.wav\" ; %s }\n}\n",
		IM_CAM_X, IM_CAM_Y + 26 + IM_CAM_ROW * ARRAY_LEN( imCamRows ), IM_CAM_W, IM_CAM_W * 0.5f, defaults.c_str() );

	// presets: < the name >, then Save and Delete
	{
		const float y = IM_CAM_Y + 26 + IM_CAM_ROW * ( ARRAY_LEN( imCamRows ) + 1 );

		out += IM_CamButton( "camerapanel_prev", "<", IM_CAM_X + 6, y, 18, "prev" );
		out += va( "itemDef {\nname camerapanel_name\ngroup camerapanel\ntype ITEM_TYPE_EDITFIELD\nstyle WINDOW_STYLE_FILLED\n"
			"text \"\"\ncvar \"" CP_NAME_CVAR "\"\nmaxChars 30\nmaxPaintChars 20\nrect %g %g %g 13\nfont 4\ntextscale .85\n"
			"textalign ITEM_ALIGN_LEFT\ntextalignx 3\nbackcolor 0 0 0 .6\nforecolor 1 1 1 1\nvisible 1\n" IM_CAM_SHOW
			"descText \"A camera preset's name: < and > go through the saved ones\"\n}\n", IM_CAM_X + 28, y, IM_CAM_W - 56 );
		out += IM_CamButton( "camerapanel_next", ">", IM_CAM_X + IM_CAM_W - 24, y, 18, "next" );
		out += IM_CamButton( "camerapanel_save", "Save", IM_CAM_X + 6, y + 17, IM_CAM_W * 0.5f - 9, "save" );
		out += IM_CamButton( "camerapanel_delete", "Delete", IM_CAM_X + IM_CAM_W * 0.5f + 3, y + 17, IM_CAM_W * 0.5f - 9, "delete" );
	}
	return out;
}

static std::string imPatched;

// an item's whole rect
static void IM_SetRect( std::string &item, float x, float y, float w, float h ) {
	const size_t rect = RM_FindWord( item, "rect", 0 );
	size_t start, end = 0;

	if ( rect == RM_NONE )
		return;
	start = RM_SkipSpace( item, rect + 4 );
	end = start;
	for ( int k = 0; k < 4; k++ )
		RM_Token( item, end, &end );
	item.replace( start, end - start, va( "%g %g %g %g", x, y, w, h ) );
}

#define QC_X		392.0f
#define QC_Y		34.0f
#define QC_W		242.0f
#define QC_ENTRY	54.0f
#define QC_SERVERS	4
#define QC_OPEN_CVAR	"ui_qcOpen"	// 1 while the panel is out: Esc closes it first (see cl_quickconnect.cpp)

// TOOLS: every tool, its command run as the menu closes, five a page
#define TL_X		392.0f
#define TL_Y		34.0f
#define TL_W		242.0f
#define TL_ENTRY	30.0f
#define TL_PER_PAGE	5

// the tools: the Esc menus' list (cl_escmenu.cpp)
#define imTools		escTools
#define TL_NUM		escNumTools
#define TL_PAGES	( ( TL_NUM + TL_PER_PAGE - 1 ) / TL_PER_PAGE )

// every part of the tools panel put away
static const char *IM_ToolsHide( void ) {
	static char out[256];

	Q_strncpyz( out, "hide tools ;", sizeof( out ) );
	for ( int p = 0; p < TL_PAGES; p++ )
		Q_strcat( out, sizeof( out ), va( " hide tools_p%i ;", p ) );
	return out;
}

static std::string IM_ToolsPanel( void ) {
	const float h = 24.0f + TL_ENTRY * TL_PER_PAGE + 4.0f;
	std::string out, hide = IM_ToolsHide();

	out += va( "itemDef {\nname tools_bg\ngroup tools\nstyle WINDOW_STYLE_FILLED\nrect %g %g %g %g\nborder 1\n"
		"bordercolor .5 .5 .7 1\nbackcolor 0 0 .08 .85\nforecolor 1 1 1 1\nvisible 0\ndecoration\n}\n", TL_X, TL_Y, TL_W, h );
	out += va( "itemDef {\nname tools_close\ngroup tools\nstyle WINDOW_STYLE_FILLED\ntype ITEM_TYPE_BUTTON\ntext \"Close\"\n"
		"rect %g %g 50 13\nfont 4\ntextscale .85\ntextalign ITEM_ALIGN_CENTER\ntextalignx 25\nbackcolor .15 .15 .25 .8\nforecolor 1 .682 0 1\n"
		"visible 0\naction { play \"sound/interface/button1.wav\" ; %s show camerapanel ; setcvar " QC_OPEN_CVAR " 0 }\n}\n", TL_X + TL_W - 56, TL_Y + 5, hide.c_str() );
	for ( int p = 0; p < TL_PAGES; p++ ) {
		// the page: where it is, and the way to the ones either side
		out += va( "itemDef {\nname tools_page%i\ngroup tools_p%i\nstyle WINDOW_STYLE_EMPTY\ntext \"Tools  %i/%i\"\n"
			"rect %g %g 80 13\nfont 4\ntextscale .85\ntextalign ITEM_ALIGN_LEFT\ntextalignx 0\nforecolor .8 .8 .8 1\nvisible 0\ndecoration\n}\n",
			p, p, p + 1, TL_PAGES, TL_X + 8, TL_Y + 5 );
		if ( p > 0 )
			out += va( "itemDef {\nname tools_back%i\ngroup tools_p%i\nstyle WINDOW_STYLE_FILLED\ntype ITEM_TYPE_BUTTON\ntext \"Back\"\n"
				"rect %g %g 44 13\nfont 4\ntextscale .85\ntextalign ITEM_ALIGN_CENTER\ntextalignx 22\nbackcolor .15 .15 .25 .8\nforecolor 1 .682 0 1\n"
				"visible 0\naction { play \"sound/interface/button1.wav\" ; hide tools_p%i ; show tools_p%i }\n}\n", p, p, TL_X + 84, TL_Y + 5, p, p - 1 );
		if ( p < TL_PAGES - 1 )
			out += va( "itemDef {\nname tools_next%i\ngroup tools_p%i\nstyle WINDOW_STYLE_FILLED\ntype ITEM_TYPE_BUTTON\ntext \"Next\"\n"
				"rect %g %g 44 13\nfont 4\ntextscale .85\ntextalign ITEM_ALIGN_CENTER\ntextalignx 22\nbackcolor .15 .15 .25 .8\nforecolor 1 .682 0 1\n"
				"visible 0\naction { play \"sound/interface/button1.wav\" ; hide tools_p%i ; show tools_p%i }\n}\n", p, p, TL_X + 132, TL_Y + 5, p, p + 1 );
		for ( int k = 0; k < TL_PER_PAGE; k++ ) {
			const int i = p * TL_PER_PAGE + k;
			const float y = TL_Y + 24.0f + TL_ENTRY * k;

			if ( i >= TL_NUM )
				break;
			// its name, clicked to open it, and what it does under it
			out += va( "itemDef {\nname tools_name%i\ngroup tools_p%i\nstyle WINDOW_STYLE_EMPTY\ntype ITEM_TYPE_BUTTON\ntext \"%s\"\n"
				"rect %g %g %g 14\nfont 2\ntextscale .7\ntextalign ITEM_ALIGN_LEFT\ntextalignx 0\nforecolor 1 .682 0 1\nvisible 0\n"
				"descText \"Open the %s (/%s)\"\naction { play \"sound/interface/button1.wav\" ; setcvar " QC_OPEN_CVAR " 0 ; uiScript closeingame ; exec \"%s\" }\n}\n",
				i, p, imTools[i].name, TL_X + 8, y, TL_W - 16, imTools[i].name, imTools[i].command, imTools[i].command );
			out += va( "itemDef {\nname tools_desc%i\ngroup tools_p%i\nstyle WINDOW_STYLE_EMPTY\ntext \"%s   /%s\"\n"
				"rect %g %g %g 11\nfont 4\ntextscale .75\ntextalign ITEM_ALIGN_LEFT\ntextalignx 0\nforecolor .8 .8 .8 1\nvisible 0\ndecoration\n}\n",
				i, p, imTools[i].desc, imTools[i].command, TL_X + 14, y + 15, TL_W - 20 );
		}
	}
	return out;
}

// the quick connect panel under its button: each server, its address, map and players (ui_qc<n>_*,
// see cl_quickconnect.cpp), clicked to connect
static std::string IM_QuickConnectPanel( void ) {
	static const char *names[QC_SERVERS] = { "JEDI Temple", "JEDI Grounds", "JEDI Region", "JEDI Galaxy" };
	static const char *lines[3] = { "server", "map", "players" };
	const float h = 24.0f + QC_ENTRY * QC_SERVERS + 4.0f;
	std::string out;

	out += va( "itemDef {\nname quickconnect_bg\ngroup quickconnect\nstyle WINDOW_STYLE_FILLED\nrect %g %g %g %g\nborder 1\n"
		"bordercolor .5 .5 .7 1\nbackcolor 0 0 .08 .85\nforecolor 1 1 1 1\nvisible 0\ndecoration\n}\n", QC_X, QC_Y, QC_W, h );
	out += va( "itemDef {\nname quickconnect_refresh\ngroup quickconnect\nstyle WINDOW_STYLE_FILLED\ntype ITEM_TYPE_BUTTON\ntext \"Refresh\"\n"
		"rect %g %g 60 13\nfont 4\ntextscale .85\ntextalign ITEM_ALIGN_CENTER\ntextalignx 30\nbackcolor .15 .15 .25 .8\nforecolor 1 .682 0 1\n"
		"visible 0\naction { play \"sound/interface/button1.wav\" ; exec \"quickconnect refresh\" }\n}\n", QC_X + 6, QC_Y + 5 );
	out += va( "itemDef {\nname quickconnect_close\ngroup quickconnect\nstyle WINDOW_STYLE_FILLED\ntype ITEM_TYPE_BUTTON\ntext \"Close\"\n"
		"rect %g %g 50 13\nfont 4\ntextscale .85\ntextalign ITEM_ALIGN_CENTER\ntextalignx 25\nbackcolor .15 .15 .25 .8\nforecolor 1 .682 0 1\n"
		"visible 0\naction { play \"sound/interface/button1.wav\" ; hide quickconnect ; show camerapanel ; setcvar " QC_OPEN_CVAR " 0 }\n}\n", QC_X + QC_W - 56, QC_Y + 5 );
	for ( int i = 0; i < QC_SERVERS; i++ ) {
		const float y = QC_Y + 24.0f + QC_ENTRY * i;

		// the name, and Connect beside it
		out += va( "itemDef {\nname quickconnect_name%i\ngroup quickconnect\nstyle WINDOW_STYLE_EMPTY\ntype ITEM_TYPE_BUTTON\ntext \"%s\"\n"
			"rect %g %g %g 14\nfont 2\ntextscale .7\ntextalign ITEM_ALIGN_LEFT\ntextalignx 0\nforecolor 1 .682 0 1\nvisible 0\n"
			"descText \"Connect to %s\"\naction { play \"sound/interface/button1.wav\" ; exec \"quickconnect connect %i\" }\n}\n",
			i, names[i], QC_X + 8, y, QC_W - 80, names[i], i + 1 );
		out += va( "itemDef {\nname quickconnect_go%i\ngroup quickconnect\nstyle WINDOW_STYLE_FILLED\ntype ITEM_TYPE_BUTTON\ntext \"Connect\"\n"
			"rect %g %g 64 13\nfont 4\ntextscale .85\ntextalign ITEM_ALIGN_CENTER\ntextalignx 32\nbackcolor .15 .3 .15 .8\nforecolor 1 1 1 1\n"
			"visible 0\naction { play \"sound/interface/button1.wav\" ; exec \"quickconnect connect %i\" }\n}\n", i, QC_X + QC_W - 70, y + 1, i + 1 );
		for ( int k = 0; k < 3; k++ ) {
			out += va( "itemDef {\nname quickconnect_%s%i\ngroup quickconnect\nstyle WINDOW_STYLE_EMPTY\ncvar \"ui_qc%i_%s\"\n"
				"rect %g %g %g 11\nfont 4\ntextscale .8\ntextalign ITEM_ALIGN_LEFT\ntextalignx 0\nforecolor .8 .8 .8 1\nvisible 0\ndecoration\n}\n",
				lines[k], i, i + 1, lines[k], QC_X + 14, y + 15 + k * 11, QC_W - 20 );
		}
	}
	return out;
}

// the in-game menu: no votes, PRESETS, the camera panel
static const char *IM_Patch( const char *filename, const char *text, int *length ) {
	static const char *drop[] = { "vote", "voteButton", "callvote", "callvoteButton", "addBot", "addBotButton" };
	std::string s( text, *length );
	std::vector<rmItem_t> items;
	const rmItem_t *controls, *controlsButton;
	std::string add;
	size_t menu, menuOpen, menuEnd;

	if ( RM_FindWord( s, "presetsButton", 0 ) != RM_NONE )
		return NULL;	// done already
	RM_Items( s, items );
	controls = RM_Item( items, "controls" );
	controlsButton = RM_Item( items, "controlsButton" );
	if ( !controls || !controlsButton ) {
		Com_DPrintf( "Presets: %s isn't laid out as JKA's in-game menu; left as it is\n", filename );
		return NULL;
	}

	// PRESETS: Controls' button and its frame, in the gap after Profile
	{
		std::string button = s.substr( controls->start, controls->end - controls->start );
		std::string frame = s.substr( controlsButton->start, controlsButton->end - controlsButton->start );
		const size_t rect = RM_FindWord( button, "rect", 0 );
		size_t start, end;

		RM_ReplaceWord( button, "controlsButton", "presetsButton" );
		RM_SetValue( button, "name", "presets" );
		RM_SetValue( button, "text", RM_Quote( "Presets" ) );
		RM_RemoveValue( button, "cvarTest" );
		RM_ReplaceBlock( button, "hideCvar", "" );
		RM_ReplaceBlock( button, "showCvar", "" );
		RM_ReplaceBlock( button, "action", "action { play \"sound/interface/button1.wav\" ; uiScript closeingame ; exec \"presets\" }" );
		RM_SetValue( frame, "name", "presetsButton" );
		// x 215: where Voice Chat and Add Bot were
		for ( std::string *item : { &button, &frame } ) {
			const size_t r = RM_FindWord( *item, "rect", 0 );

			if ( r == RM_NONE )
				continue;
			start = RM_SkipSpace( *item, r + 4 );
			RM_Token( *item, start, &end );
			item->replace( start, end - start, "215" );
		}
		(void)rect;
		add += "\n" + frame + "\n" + button + "\n";
	}
	add += IM_CameraPanel();

	// QUICK CONNECT: where Vote and Call Vote were, before Exit
	{
		std::string button = s.substr( controls->start, controls->end - controls->start );
		std::string frame = s.substr( controlsButton->start, controlsButton->end - controlsButton->start );

		RM_ReplaceWord( button, "controlsButton", "quickconnectButton" );
		RM_SetValue( button, "name", "quickconnectmenu" );
		RM_SetValue( button, "text", RM_Quote( "Quick Connect" ) );
		RM_SetValue( button, "textalignx", "42.5" );
		RM_RemoveValue( button, "cvarTest" );
		RM_ReplaceBlock( button, "hideCvar", "" );
		RM_ReplaceBlock( button, "showCvar", "" );
		RM_ReplaceBlock( button, "action", va( "action { play \"sound/interface/button1.wav\" ; %s hide camerapanel ; show quickconnect ; setcvar " QC_OPEN_CVAR " 1 ; exec \"quickconnect refresh\" }", IM_ToolsHide() ) );
		RM_SetValue( frame, "name", "quickconnectButton" );
		IM_SetRect( button, 425, 0, 85, 32 );
		IM_SetRect( frame, 425, 0, 85, 32 );
		add += "\n" + frame + "\n" + button + "\n";
	}
	add += IM_QuickConnectPanel();

	// TOOLS: between Quick Connect and Exit, which is made narrower for it; its panel under it, in
	// Quick Connect's place, Esc putting it away the same way
	{
		std::string button = s.substr( controls->start, controls->end - controls->start );
		std::string frame = s.substr( controlsButton->start, controlsButton->end - controlsButton->start );

		RM_ReplaceWord( button, "controlsButton", "toolsButton" );
		RM_SetValue( button, "name", "toolsmenu" );
		RM_SetValue( button, "text", RM_Quote( "Tools" ) );
		RM_SetValue( button, "textalignx", "31" );
		RM_RemoveValue( button, "cvarTest" );
		RM_ReplaceBlock( button, "hideCvar", "" );
		RM_ReplaceBlock( button, "showCvar", "" );
		RM_ReplaceBlock( button, "action", va( "action { play \"sound/interface/button1.wav\" ; hide quickconnect ; hide camerapanel ; %s show tools ; "
			"show tools_p0 ; setcvar " QC_OPEN_CVAR " 1 }", IM_ToolsHide() ) );
		RM_SetValue( frame, "name", "toolsButton" );
		IM_SetRect( button, 510, 0, 62, 32 );
		IM_SetRect( frame, 510, 0, 62, 32 );
		add += "\n" + frame + "\n" + button + "\n";
	}
	add += IM_ToolsPanel();

	// the router's hidden items: the first only takes the focus off the others, so each is focused anew
	add += "itemDef {\nname escroute_reset\ntype ITEM_TYPE_BUTTON\nrect 0 0 0 0\nvisible 0\n}\n";
	for ( size_t r = 0; r < ARRAY_LEN( imRoutes ); r++ ) {
		add += va( "itemDef {\nname escroute_%s\ntype ITEM_TYPE_BUTTON\nrect 0 0 0 0\nvisible 0\n"
			"onFocus { " IM_GATE( IM_ROUTE_CVAR "%s" ) "%s open %s }\n}\n", imRoutes[r].route, imRoutes[r].route,
			imRoutes[r].keepGate ? "" : va( " setcvar " IM_ROUTE_CVAR "%s 1 ;", imRoutes[r].route ), imRoutes[r].open );
	}

	// the ones that go, last first so the places stay right
	for ( int i = (int)items.size() - 1; i >= 0; i-- ) {
		for ( size_t k = 0; k < ARRAY_LEN( drop ); k++ ) {
			if ( !Q_stricmp( items[i].name.c_str(), drop[k] ) ) {
				s.erase( items[i].start, items[i].end - items[i].start );
				break;
			}
		}
	}

	// Exit and its frame narrower, at the end, past TOOLS
	{
		std::vector<rmItem_t> now;
		const rmItem_t *leave, *leaveButton;

		RM_Items( s, now );
		leave = RM_Item( now, "leave" );
		leaveButton = RM_Item( now, "leaveButton" );
		if ( leave && leaveButton ) {
			// the later one first, so the earlier one's place stays right
			const rmItem_t *order[2] = { leave->start > leaveButton->start ? leave : leaveButton,
				leave->start > leaveButton->start ? leaveButton : leave };

			for ( int k = 0; k < 2; k++ ) {
				std::string text = s.substr( order[k]->start, order[k]->end - order[k]->start );

				IM_SetRect( text, 572, 0, 63, 32 );
				if ( order[k] == leave )
					RM_SetValue( text, "textalignx", "31.5" );
				s.replace( order[k]->start, order[k]->end - order[k]->start, text );
			}
		}
	}

	// the menu's own buttons (Setup, Profile, Exit...) put the quick connect and tools panels away: Exit's
	// menu opens where they are, and Esc there is theirs again
	{
		const std::string putAway = va( " hide quickconnect ; %s setcvar " QC_OPEN_CVAR " 0 ;", IM_ToolsHide() );

		for ( size_t at = RM_FindWord( s, "action", 0 ); at != RM_NONE; at = RM_FindWord( s, "action", at + 1 ) ) {
			const size_t open = RM_SkipSpace( s, at + 6 );

			if ( open < s.size() && s[open] == '{' )
				s.insert( open + 1, putAway );
		}
	}

	menu = RM_FindWord( s, "menuDef", 0 );
	if ( menu == RM_NONE )
		return NULL;
	menuOpen = RM_SkipSpace( s, menu + 7 );
	if ( menuOpen >= s.size() || s[menuOpen] != '{' || ( menuEnd = RM_BlockEnd( s, menuOpen ) ) == RM_NONE )
		return NULL;
	s.insert( menuEnd - 1, add );

	// the menu opens with the quick connect and tools panels put away, the camera panel back
	{
		const std::string resetText = va( " hide quickconnect ; %s show camerapanel ; setcvar " QC_OPEN_CVAR " 0 ;", IM_ToolsHide() );
		const char *reset = resetText.c_str();
		const size_t firstItem = RM_FindWord( s, "itemDef", menuOpen );
		const size_t onOpen = RM_FindWord( s, "onOpen", menuOpen, firstItem );
		const size_t open = onOpen != RM_NONE ? RM_SkipSpace( s, onOpen + 6 ) : RM_NONE;

		if ( open != RM_NONE && open < s.size() && s[open] == '{' )
			s.insert( open + 1, reset );
		else
			s.insert( menuOpen + 1, va( "\nonOpen {%s }\n", reset ) );
	}
	// and ends with the router: past its gate only while routing (a defer stops all that follows)
	{
		const size_t firstItem = RM_FindWord( s, "itemDef", menuOpen );
		const size_t onOpen = RM_FindWord( s, "onOpen", menuOpen, firstItem );
		const size_t open = onOpen != RM_NONE ? RM_SkipSpace( s, onOpen + 6 ) : RM_NONE;
		const size_t openEnd = open != RM_NONE && s[open] == '{' ? RM_BlockEnd( s, open ) : RM_NONE;
		std::string route = " " IM_GATE( IM_ROUTE_GATE ) " setfocus escroute_reset ;";

		for ( size_t r = 0; r < ARRAY_LEN( imRoutes ); r++ )
			route += va( " setfocus escroute_%s ;", imRoutes[r].route );
		if ( openEnd != RM_NONE )
			s.insert( openEnd - 1, route + "\n" );
	}

	imPatched.swap( s );
	*length = (int)imPatched.size();
	return imPatched.c_str();
}

// Exit's menu says when it's open, so the camera panel under it gets out of the way
static const char *LV_Patch( const char *filename, const char *text, int *length ) {
	static std::string patched;
	std::string s( text, *length );
	size_t menu, menuOpen, firstItem, at;

	if ( RM_FindWord( s, IM_LEAVE_CVAR, 0 ) != RM_NONE )
		return NULL;	// done already
	menu = RM_FindWord( s, "menuDef", 0 );
	if ( menu == RM_NONE )
		return NULL;
	menuOpen = RM_SkipSpace( s, menu + 7 );
	if ( menuOpen >= s.size() || s[menuOpen] != '{' )
		return NULL;
	firstItem = RM_FindWord( s, "itemDef", menuOpen );
	// onClose first, so onOpen's place stays right whichever comes first
	at = RM_FindWord( s, "onClose", menuOpen, firstItem );
	if ( at != RM_NONE && ( at = RM_SkipSpace( s, at + 7 ) ) < s.size() && s[at] == '{' )
		s.insert( at + 1, " setcvar " IM_LEAVE_CVAR " 0 ;" );
	else
		s.insert( menuOpen + 1, "\nonClose { setcvar " IM_LEAVE_CVAR " 0 }\n" );
	firstItem = RM_FindWord( s, "itemDef", menuOpen );
	at = RM_FindWord( s, "onOpen", menuOpen, firstItem );
	if ( at != RM_NONE && ( at = RM_SkipSpace( s, at + 6 ) ) < s.size() && s[at] == '{' )
		s.insert( at + 1, " setcvar " IM_LEAVE_CVAR " 1 ;" );
	else
		s.insert( menuOpen + 1, "\nonOpen { setcvar " IM_LEAVE_CVAR " 1 }\n" );
	Com_DPrintf( "%s: tells the camera panel when it's open\n", filename );
	patched.swap( s );
	*length = (int)patched.size();
	return patched.c_str();
}

// every menu read goes by here: the in-game menu, Exit's, or the Setup menus
static const char *MP_Patch( const char *filename, const char *text, int *length ) {
	const char *base = COM_SkipPath( (char *)filename );

	if ( !Q_stricmp( base, "ingame.menu" ) && Q_stristr( filename, "jamp" ) )
		return IM_Patch( filename, text, length );
	if ( !Q_stricmp( base, "ingame_leave.menu" ) && Q_stristr( filename, "jamp" ) )
		return LV_Patch( filename, text, length );
	return RM_Patch( filename, text, length );
}

/*
================================================================================
init
================================================================================
*/

static cvar_t *rmRenderer, *rmFbo, *rmPostfx;

// the page's Vulkan and Post Effects rows, as they are picked on it
void CL_RenderMenu_Frame( void ) {
	qboolean on;

	if ( !rmPostfx )
		return;
	// the camera panel: its setting on, and nothing over it: Exit's menu (gone with the menus), or the
	// quick connect or tools panel
	if ( !( Key_GetCatcher() & KEYCATCH_UI ) && Cvar_VariableIntegerValue( IM_LEAVE_CVAR ) )
		Cvar_Set( IM_LEAVE_CVAR, "0" );
	on = (qboolean)( Cvar_VariableIntegerValue( "ui_cameraSettings" ) == 1 && !Cvar_VariableIntegerValue( IM_LEAVE_CVAR )
		&& !Cvar_VariableIntegerValue( QC_OPEN_CVAR ) );
	if ( Cvar_VariableIntegerValue( IM_CAM_CVAR ) != on )
		Cvar_Set( IM_CAM_CVAR, on ? "1" : "0" );
	on = (qboolean)( !Q_stricmp( rmRenderer->string, RM_VULKAN ) && rmFbo->integer );
	if ( rmPostfx->integer != on )
		Cvar_Set( RM_POSTFX, on ? "1" : "0" );
}

// one of JKA's own pages opened through the in-game menu (see imRoutes): controls, setup or renderer.
// The gates are let down as the menu opens, and up again at once: its scripts run as it opens
qboolean CL_RenderMenu_Route( const char *page ) {
	qboolean known = qfalse;

	for ( size_t r = 0; r < ARRAY_LEN( imRoutes ); r++ ) {
		const qboolean it = (qboolean)!Q_stricmp( page, imRoutes[r].route );

		Cvar_Set( va( IM_ROUTE_CVAR "%s", imRoutes[r].route ), it ? "0" : "1" );
		known = (qboolean)( known || it );
	}
	if ( !known || cls.state != CA_ACTIVE )
		return qfalse;
	Cvar_Set( IM_ROUTE_GATE, "0" );
	UIVM_SetActiveMenu( UIMENU_INGAME );
	Cvar_Set( IM_ROUTE_GATE, "1" );
	for ( size_t r = 0; r < ARRAY_LEN( imRoutes ); r++ )
		Cvar_Set( va( IM_ROUTE_CVAR "%s", imRoutes[r].route ), "1" );
	return qtrue;
}

static void IM_Route_f( void ) {
	if ( !CL_RenderMenu_Route( Cmd_Argv( 1 ) ) )
		Com_Printf( "escroute controls|setup|renderer : opens that page of the game's own menus\n" );
}

void CL_RenderMenu_Init( void ) {
	Cvar_Get( IM_ROUTE_GATE, "1", 0, "The Esc menus' router into JKA's own pages: 0 while it routes" );
	for ( size_t r = 0; r < ARRAY_LEN( imRoutes ); r++ )
		Cvar_Get( va( IM_ROUTE_CVAR "%s", imRoutes[r].route ), "1", 0, "The Esc menus' router: 0 for the page it opens" );
	Cmd_AddCommand( "escroute", IM_Route_f, "Opens a page of the game's own menus: controls, setup or renderer" );
	for ( int i = 0; i < rmNumRows; i++ ) {
		if ( rmRows[i].ui )
			Cvar_Get( rmRows[i].ui, rmRows[i].def, 0, "The Setup menu's RENDERER page: renderersettings apply sets it" );
	}
	Cmd_AddCommand( "renderersettings", RM_Settings_f, "The Setup menu's RENDERER page: sync or apply" );
	rmRenderer = Cvar_Get( "ui_rd_renderer", "rd-vanilla", 0 );
	rmFbo = Cvar_Get( "ui_rd_fbo", "0", 0 );
	rmPostfx = Cvar_Get( RM_POSTFX, "0", CVAR_ROM, "The Setup menu's RENDERER page has Vulkan and Post Effects on: their effects show" );
	Cvar_Get( "ui_cameraSettings", "0", CVAR_ARCHIVE_ND, "The in-game menu shows the third person camera's settings" );
	Cvar_Get( QC_OPEN_CVAR, "0", 0, "The in-game menu's quick connect panel is out: Esc closes it first" );
	Cvar_Get( IM_LEAVE_CVAR, "0", 0, "The in-game menu's Exit menu is open: the camera panel steps out of its way" );
	Cvar_Get( IM_CAM_CVAR, "0", 0, "The in-game menu's camera panel shows: ui_cameraSettings on, and Exit's menu not open" );
	Cvar_Get( CP_NAME_CVAR, "", CVAR_ARCHIVE_ND, "The camera panel's preset name" );
	Cmd_AddCommand( "campreset", CP_Cmd_f, "Camera settings saved and loaded by name" );
	LoadScriptFile_Patch = MP_Patch;
}

void CL_RenderMenu_Shutdown( void ) {
	LoadScriptFile_Patch = NULL;
	Cmd_RemoveCommand( "renderersettings" );
}
