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
// The page's rows are ui_rd_* copies of the settings: most of them only take with a vid_restart, and
// a latched cvar reads as it was until then. "renderersettings sync" fills the copies as the page
// opens, and the video restart's Yes runs "renderersettings apply" before its vid_restart.

#include "client.h"
#include <string>
#include <vector>

// botlib/l_script.cpp: a script file read, before it's parsed
extern const char *( *LoadScriptFile_Patch )( const char *filename, const char *text, int *length );

#define RM_GROUP		"renderer"
#define RM_BUTTON		"renderermenubutton"
#define RM_GLOW			"rendererbutton_glow"
#define RM_VULKAN		"rd-vulkan"

typedef struct {
	const char	*ui;			// the page's copy
	const char	*real;			// the setting
	const char	*def;			// when the renderer that has it hasn't made it yet
	const char	*label;
	const char	*desc;
	const char	*list;			// cvarStrList (commas between) or cvarFloatList (none), braces and all
	qboolean	vulkanOnly;		// shown when Vulkan is picked
} rmRow_t;

static const rmRow_t rmRows[] = {
	{ "ui_rd_renderer", "cl_renderer", "rd-vanilla", "Renderer:",
		"OpenGL is the usual one; Vulkan is TaystJK's, with more effects (experimental)",
		"cvarStrList { \"OpenGL\" , \"rd-vanilla\" , \"Vulkan\" , \"rd-vulkan\" }", qfalse },
	{ "ui_rd_msaa", "r_ext_multisample", "0", "Anti-aliasing:",
		"Smooths jagged edges; more costs more frames",
		"cvarFloatList { \"Off\" 0 \"2x\" 2 \"4x\" 4 \"8x\" 8 }", qfalse },
	{ "ui_rd_glow", "r_DynamicGlow", "0", "Dynamic Glow:",
		"Glowing lights and sabers; with Vulkan it needs Post Effects",
		"cvarFloatList { \"Off\" 0 \"On\" 1 }", qfalse },
	{ "ui_rd_fbo", "r_fbo", "0", "Post Effects:",
		"Draws to a picture first: needed for bloom, HDR, dithering and glow",
		"cvarFloatList { \"Off\" 0 \"On\" 1 }", qtrue },
	{ "ui_rd_bloom", "r_bloom", "0", "Bloom:",
		"Bright light bleeds into its surroundings; needs Post Effects",
		"cvarFloatList { \"Off\" 0 \"On\" 1 }", qtrue },
	{ "ui_rd_hdr", "r_hdr", "1", "HDR:",
		"More colour depth in the picture, less banding; needs Post Effects",
		"cvarFloatList { \"Off\" 0 \"On\" 1 }", qtrue },
	{ "ui_rd_dither", "r_dither", "0", "Dithering:",
		"Hides banding in smooth gradients; needs Post Effects",
		"cvarFloatList { \"Off\" 0 \"On\" 1 }", qtrue },
	{ "ui_rd_vbo", "r_vbo", "0", "World Caching:",
		"Keeps the map on the graphics card: more frames per second",
		"cvarFloatList { \"Off\" 0 \"On\" 1 }", qtrue },
	{ "ui_rd_vbomodels", "r_vbo_models", "0", "Model Caching:",
		"Keeps models on the graphics card: more frames per second",
		"cvarFloatList { \"Off\" 0 \"On\" 1 }", qtrue },
	// Vulkan's own additions (rd-vulkan's vk_rpmod_post.cpp); JKA's Setup menu has room for 15 rows
	{ "ui_rd_ssao", "r_ssao", "0", "Ambient Occlusion:",
		"Darkens corners, creases and the ground under things; needs Post Effects",
		"cvarFloatList { \"Off\" 0 \"On\" 1 }", qtrue },
	{ "ui_rd_soft", "r_softParticles", "0", "Soft Particles:",
		"Smoke and fire fade where they meet walls instead of cutting into them; needs Post Effects",
		"cvarFloatList { \"Off\" 0 \"On\" 1 }", qtrue },
	{ "ui_rd_fxaa", "r_fxaa", "0", "FXAA:",
		"Smooths edges over the finished picture, grass and fences too; needs Post Effects",
		"cvarFloatList { \"Off\" 0 \"On\" 1 }", qtrue },
	{ "ui_rd_sharpen", "r_sharpen", "0", "Sharpening:",
		"Crisper detail, against the softness of FXAA or a lower render scale; needs Post Effects",
		"cvarFloatList { \"Off\" 0 \"Low\" 0.4 \"High\" 0.8 }", qtrue },
	{ "ui_rd_shafts", "r_sunShafts", "0", "Sun Shafts:",
		"Beams of light from the sun through gaps, on maps with a sun; needs Post Effects",
		"cvarFloatList { \"Off\" 0 \"On\" 1 }", qtrue },
	{ "ui_rd_tonemap", "r_tonemap", "0", "Tone Mapping:",
		"Bright light rolls off like film instead of clipping to white; needs Post Effects",
		"cvarFloatList { \"Off\" 0 \"On\" 1 }", qtrue },
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
		for ( int i = 0; i < rmNumRows; i++ )
			Cvar_Set( rmRows[i].ui, RM_Effective( &rmRows[i] ) );
	} else if ( !Q_stricmp( how, "apply" ) ) {
		for ( int i = 0; i < rmNumRows; i++ ) {
			const char *want = Cvar_VariableString( rmRows[i].ui );

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
		// its page instead of DEFAULTS', with the settings as they are
		RM_ReplaceCommand( button, "show", "defaults", "hide defaults show " RM_GROUP " exec \"renderersettings sync\"" );

		RM_ReplaceWord( shine, "gamedefaultsbutton_glow", RM_GLOW );
		RM_SetRectY( shine, y );

		add += "\n" + shine + "\n" + button + "\n";
	}

	// the rows, as VIDEO QUALITY is
	for ( int i = 0; i < rmNumRows; i++ ) {
		const rmRow_t *rr = &rmRows[i];
		std::string item = s.substr( row->start, row->end - row->start );
		char highlight[32];

		RM_SetValue( item, "name", va( "rd_row%i", i ) );
		RM_SetValue( item, "group", RM_GROUP );
		RM_SetValue( item, "text", RM_Quote( rr->label ) );
		RM_SetValue( item, "descText", RM_Quote( rr->desc ) );
		RM_SetValue( item, "visible", "0" );
		RM_SetValue( item, "cvar", RM_Quote( rr->ui ) );
		RM_SetRectY( item, rowY + rowStep * i );
		RM_ReplaceBlock( item, "cvarFloatList", "" );
		RM_ReplaceBlock( item, "cvarStrList", "" );
		RM_ReplaceBlock( item, "action", "action { play \"sound/interface/button1\" show applyChanges }" );
		Com_sprintf( highlight, sizeof( highlight ), "highlight%i", i + 1 );
		RM_ReplaceWord( item, "highlight1", highlight );
		item.insert( item.rfind( '}' ), std::string( rr->list ) + "\n" );
		if ( rr->vulkanOnly )
			item.insert( item.rfind( '}' ), "cvarTest \"ui_rd_renderer\" showCvar { \"" RM_VULKAN "\" }\n" );
		add += item + "\n";
	}

	// the menu's own scripts: its other pages hide this one, and the restart applies it
	RM_AfterCommand( s, "show", "video2", " hide " RM_GROUP );
	RM_AfterCommand( s, "hide", "video2", " hide " RM_GROUP );
	RM_AfterCommand( s, "show", "defaults", " hide " RM_GROUP );
	RM_AfterCommand( s, "hide", "defaults", " hide " RM_GROUP );
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

	rmPatched.swap( s );
	*length = (int)rmPatched.size();
	return rmPatched.c_str();
}

/*
================================================================================
init
================================================================================
*/

void CL_RenderMenu_Init( void ) {
	for ( int i = 0; i < rmNumRows; i++ )
		Cvar_Get( rmRows[i].ui, rmRows[i].def, 0, "The Setup menu's RENDERER page: renderersettings apply sets it" );
	Cmd_AddCommand( "renderersettings", RM_Settings_f, "The Setup menu's RENDERER page: sync or apply" );
	LoadScriptFile_Patch = RM_Patch;
}

void CL_RenderMenu_Shutdown( void ) {
	LoadScriptFile_Patch = NULL;
	Cmd_RemoveCommand( "renderersettings" );
}
