/*
===========================================================================
Copyright (C) 2026, TaystJK contributors

This file is part of the TaystJK source code.

TaystJK is free software; you can redistribute it and/or modify it
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

// cl_soundmanager.cpp -- engine-side front ends for RPMod's /rpsound and /rpmusic
//
// Sound manager: every sound/**/*.wav and *.mp3 the filesystem can see, Base (the game's
// own pk3s) or Custom by the pk3 it comes from, heard on this client only, then sent as
// "rpsound once" (to everyone, the player or another player), "rpsound add" (looping:
// everywhere, on a player or at a spot) or "rpsound local". Playing tab: the looping
// sounds from the server's "rpsound list", to remove one or all of them.
//
// Music manager: every music/**/*.mp3 and *.wav the same way. A preview takes over this
// client's music until it is stopped, then the map's comes back; "rpmusic" plays one for
// everyone, the map's own again (default) or none.
//
// Both share the browser, and only one is open at a time. This lives in the engine
// rather than cgame so it works whatever cgame the server's mod ships (RPMod loads its own).

#include "client.h"
#include "qcommon/cm_public.h"
#include "cl_widgets.h"

#define SM_SOUND_ROOT		"sound"
#define SM_MUSIC_ROOT		"music"
#define SM_SOUND_CMD		"soundmanager"
#define SM_MUSIC_CMD		"musicmanager"
#define SM_BASE_PAKS		BASEGAME "/assets"		// base/assets0.pk3 to assets3.pk3 are the game's own

#define SM_MAX_ASSETS		32768
#define SM_NAME_POOL		( SM_MAX_ASSETS * 40 )
#define SM_HASH_SIZE		( SM_MAX_ASSETS * 2 )
#define SM_MAX_ROWS			4096
#define SM_MAX_SOURCES		1024
#define SM_MAX_QUEUE		64
#define SM_MAX_LOOPS		256
#define SM_FIELD_LEN		64

#define SM_LIST_TIMEOUT		2500	// ms the reply to "rpsound list" has to start in
#define SM_LIST_LINE_GAP	1500	// ms between its lines before it's over
#define SM_REPLY_MS			3000	// prints this soon after a command are shown as its reply
#define SM_MESSAGE_MS		5000
#define SM_CONFIRM_MS		3000
#define SM_DOUBLECLICK_MS	400

// layout, in 640x480, as the effect manager has it
#define SM_ROW_H			13.0f
#define SM_CTRL_H			15.0f
#define SM_TEXT_SCALE		0.8f
#define SM_SEARCH_Y			46.0f
#define SM_LIST_Y			66.0f
#define SM_LIST_H			364.0f
#define SM_FOLDER_X			16.0f
#define SM_FOLDER_W			150.0f
#define SM_ASSET_X			170.0f
#define SM_ASSET_W			160.0f
#define SM_RIGHT_X			338.0f
#define SM_RIGHT_W			286.0f
#define SM_CTRL_X			( SM_RIGHT_X + 52.0f )
#define SM_CTRL_W			( SM_RIGHT_X + SM_RIGHT_W - SM_CTRL_X )
#define SM_FILTER_W			44.0f
#define SM_BUTTON_Y			410.0f
#define SM_LOOPS_W			310.0f
#define SM_POPUP_X			150.0f
#define SM_POPUP_Y			60.0f
#define SM_POPUP_W			340.0f
#define SM_POPUP_H			360.0f

/*
===============================================================================

THE BROWSER

===============================================================================
*/

typedef struct {
	const char	*path;		// relative to the root, with its extension: what the commands take
	const char	*base;		// the name without its folder
	int			source;
	qboolean	custom;
} smAsset_t;

typedef enum {
	SM_ROW_CATEGORY,	// Base or Custom
	SM_ROW_SOURCE,		// a custom pk3
	SM_ROW_FOLDER
} smRowType_t;

typedef struct {
	smRowType_t	type;
	char		name[MAX_QPATH];
	int			first, count;	// a range of the sorted assets
	qboolean	custom;
} smRow_t;

typedef struct {
	int			sel, scroll;
	int			lastClickRow, lastClickTime;
} smList_t;

typedef enum {
	SM_FILTER_ALL,
	SM_FILTER_BASE,
	SM_FILTER_CUSTOM,
	SM_FILTER_COUNT
} smFilter_t;

static const char *smFilterNames[SM_FILTER_COUNT] = { "All", "Base", "Custom" };

typedef struct {
	const char	*root;
	const char	*exts[2];
	const char	*what;			// "sounds", for the messages
	qboolean	indexed;

	smAsset_t	*assets;
	int			numAssets, numCustom;
	char		*pool;
	int			poolUsed;
	int			*hash;			// index + 1, 0 = empty
	const char	*sources[SM_MAX_SOURCES];
	int			numSources;

	smRow_t		*rows;
	int			numRows;
	int			*view;
	int			numView;

	smFilter_t	filter;
	char		search[SM_FIELD_LEN];
	int			row;			// the folder listed
	int			asset;			// selected, -1 = none
	smList_t	folderList, assetList;
} smBrowser_t;

static smBrowser_t	smSounds = { SM_SOUND_ROOT, { ".wav", ".mp3" }, "sounds" };
static smBrowser_t	smMusic = { SM_MUSIC_ROOT, { ".mp3", ".wav" }, "music files" };

static int SM_HashName( const char *s ) {
	unsigned int h = 5381;

	while ( *s ) {
		h = h * 33 + (unsigned int)tolower( (unsigned char)*s );
		s++;
	}
	return (int)( h % SM_HASH_SIZE );
}

static const char *SM_PoolString( smBrowser_t *b, const char *s ) {
	const int len = strlen( s ) + 1;
	char *out;

	if ( b->poolUsed + len > SM_NAME_POOL )
		return NULL;
	out = b->pool + b->poolUsed;
	memcpy( out, s, len );
	b->poolUsed += len;
	return out;
}

// "base/assets<digits>.pk3"; anything else people named assets_something isn't
static qboolean SM_IsBaseSource( const char *source ) {
	const int prefixLen = strlen( SM_BASE_PAKS );
	const char *c = source + prefixLen;

	if ( Q_stricmpn( source, SM_BASE_PAKS, prefixLen ) || !isdigit( (unsigned char)*c ) )
		return qfalse;
	while ( isdigit( (unsigned char)*c ) )
		c++;
	return (qboolean)!Q_stricmp( c, ".pk3" );
}

static int SM_SourceIndex( smBrowser_t *b, const char *source ) {
	const char *stored;

	// the files of one pk3 come together
	for ( int i = b->numSources - 1; i >= 0; i-- ) {
		if ( !Q_stricmp( b->sources[i], source ) )
			return i;
	}
	if ( b->numSources >= SM_MAX_SOURCES || !( stored = SM_PoolString( b, source ) ) )
		return 0;
	b->sources[b->numSources] = stored;
	return b->numSources++;
}

// name is a full game path, e.g. "sound/movers/doors/door1.wav"; the first one seen is the one in use
static void SM_AddAsset( const char *name, const char *source, void *ctx ) {
	smBrowser_t *b = (smBrowser_t *)ctx;
	const int rootLen = strlen( b->root );
	char path[MAX_QPATH];
	const char *stored, *slash;
	int h;

	if ( (int)strlen( name ) <= rootLen + 1 || Q_stricmpn( name, b->root, rootLen ) || ( name[rootLen] != '/' && name[rootLen] != '\\' ) )
		return;
	Q_strncpyz( path, name + rootLen + 1, sizeof( path ) );
	for ( char *c = path; *c; c++ ) {
		if ( *c == '\\' )
			*c = '/';
	}

	// the same file can be in several pk3s and folders
	h = SM_HashName( path );
	while ( b->hash[h] ) {
		if ( !Q_stricmp( b->assets[b->hash[h] - 1].path, path ) )
			return;
		h = ( h + 1 ) % SM_HASH_SIZE;
	}
	if ( b->numAssets >= SM_MAX_ASSETS || !( stored = SM_PoolString( b, path ) ) )
		return;

	slash = strrchr( stored, '/' );
	b->assets[b->numAssets].path = stored;
	b->assets[b->numAssets].base = slash ? slash + 1 : stored;
	b->assets[b->numAssets].source = SM_SourceIndex( b, source );
	b->assets[b->numAssets].custom = (qboolean)!SM_IsBaseSource( source );
	b->hash[h] = ++b->numAssets;
}

static int SM_FolderLen( const smAsset_t *a ) {
	return (int)( a->base - a->path ) - ( a->base != a->path ? 1 : 0 );
}

static smBrowser_t *smSortBrowser;

// Base first, then Custom pk3 by pk3, then by folder: every row is one contiguous range
static int QDECL SM_CompareAssets( const void *pa, const void *pb ) {
	const smAsset_t *a = (const smAsset_t *)pa, *b = (const smAsset_t *)pb;
	const int la = SM_FolderLen( a ), lb = SM_FolderLen( b );
	int cmp;

	if ( a->custom != b->custom )
		return a->custom ? 1 : -1;
	if ( a->custom && a->source != b->source && ( cmp = Q_stricmp( smSortBrowser->sources[a->source], smSortBrowser->sources[b->source] ) ) )
		return cmp;
	cmp = Q_stricmpn( a->path, b->path, la < lb ? la : lb );
	if ( cmp )
		return cmp;
	if ( la != lb )
		return la - lb;
	return Q_stricmp( a->base, b->base );
}

static qboolean SM_PassesFilter( const smBrowser_t *b, const smAsset_t *a ) {
	return (qboolean)( b->filter == SM_FILTER_ALL || ( b->filter == SM_FILTER_CUSTOM ) == ( a->custom != qfalse ) );
}

static int SM_AddRow( smBrowser_t *b, smRowType_t type, const char *name, int first, qboolean custom ) {
	smRow_t *r;

	if ( b->numRows >= SM_MAX_ROWS )
		return -1;
	r = &b->rows[b->numRows];
	r->type = type;
	Q_strncpyz( r->name, name, sizeof( r->name ) );
	r->first = first;
	r->count = 0;
	r->custom = custom;
	return b->numRows++;
}

// Base and its folders, then Custom with each pk3 and its folders, of what the filter lets through
static void SM_BuildRows( smBrowser_t *b ) {
	int category = -1, source = -1, catRow = -1, srcRow = -1, folderRow = -1;

	b->numRows = 0;
	for ( int i = 0; i < b->numAssets; i++ ) {
		const smAsset_t *a = &b->assets[i];
		const int len = SM_FolderLen( a );

		if ( !SM_PassesFilter( b, a ) )
			continue;
		if ( a->custom != category ) {
			category = a->custom;
			catRow = SM_AddRow( b, SM_ROW_CATEGORY, a->custom ? "Custom" : "Base", i, a->custom );
			source = -1;
			folderRow = -1;
		}
		if ( a->custom && a->source != source ) {
			source = a->source;
			srcRow = SM_AddRow( b, SM_ROW_SOURCE, b->sources[source], i, qtrue );
			folderRow = -1;
		}
		if ( folderRow < 0 || (int)strlen( b->rows[folderRow].name ) != len || Q_stricmpn( b->rows[folderRow].name, a->path, len ) ) {
			char folder[MAX_QPATH];

			Q_strncpyz( folder, a->path, Q_min( len + 1, (int)sizeof( folder ) ) );
			folderRow = SM_AddRow( b, SM_ROW_FOLDER, folder, i, a->custom );
		}
		if ( catRow < 0 || folderRow < 0 )
			break;	// out of rows
		b->rows[catRow].count++;
		if ( a->custom && srcRow >= 0 )
			b->rows[srcRow].count++;
		b->rows[folderRow].count++;
	}
}

static int SM_FirstFolderRow( const smBrowser_t *b ) {
	for ( int i = 0; i < b->numRows; i++ ) {
		if ( b->rows[i].type == SM_ROW_FOLDER )
			return i;
	}
	return 0;
}

static qboolean SM_ContainsNoCase( const char *haystack, const char *needle ) {
	const int nlen = strlen( needle );

	if ( !nlen )
		return qtrue;
	for ( ; *haystack; haystack++ ) {
		if ( !Q_stricmpn( haystack, needle, nlen ) )
			return qtrue;
	}
	return qfalse;
}

static void SM_ListClamp( smList_t *list, int count, int visible ) {
	if ( list->sel >= count )
		list->sel = count - 1;
	if ( list->sel >= 0 ) {
		if ( list->sel < list->scroll )
			list->scroll = list->sel;
		if ( list->sel >= list->scroll + visible )
			list->scroll = list->sel - visible + 1;
	}
	if ( list->scroll > count - visible )
		list->scroll = count - visible;
	if ( list->scroll < 0 )
		list->scroll = 0;
}

static void SM_ListMove( smList_t *list, int count, int visible, int delta ) {
	if ( !count )
		return;
	list->sel = Com_Clampi( 0, count - 1, ( list->sel < 0 ? 0 : list->sel ) + delta );
	SM_ListClamp( list, count, visible );
}

static void SM_RebuildView( smBrowser_t *b ) {
	b->numView = 0;
	if ( b->search[0] ) {
		for ( int i = 0; i < b->numAssets; i++ ) {
			if ( SM_PassesFilter( b, &b->assets[i] ) && SM_ContainsNoCase( b->assets[i].path, b->search ) )
				b->view[b->numView++] = i;
		}
	} else if ( b->row >= 0 && b->row < b->numRows ) {
		// a folder, a pk3 or all of Base or Custom
		const smRow_t *r = &b->rows[b->row];

		for ( int i = 0; i < r->count; i++ )
			b->view[b->numView++] = r->first + i;
	}

	// the selected one stays selected if it is still listed
	b->assetList.sel = -1;
	for ( int i = 0; i < b->numView; i++ ) {
		if ( b->view[i] == b->asset ) {
			b->assetList.sel = i;
			break;
		}
	}
	SM_ListClamp( &b->assetList, b->numView, (int)( SM_LIST_H / SM_ROW_H ) );
}

static void SM_SetRow( smBrowser_t *b, int row ) {
	if ( !b->numRows )
		return;
	b->row = Com_Clampi( 0, b->numRows - 1, row );
	b->folderList.sel = b->row;
	b->search[0] = '\0';
	b->assetList.sel = b->assetList.scroll = 0;
	SM_ListMove( &b->folderList, b->numRows, (int)( SM_LIST_H / SM_ROW_H ), 0 );
	SM_RebuildView( b );
}

static void SM_SetFilter( smBrowser_t *b, smFilter_t filter ) {
	if ( filter == b->filter )
		return;
	b->filter = filter;
	SM_BuildRows( b );
	b->row = SM_FirstFolderRow( b );
	if ( b->search[0] ) {
		b->assetList.sel = b->assetList.scroll = 0;
		SM_RebuildView( b );
	} else {
		SM_SetRow( b, b->row );
	}
}

static void SM_BuildIndex( smBrowser_t *b ) {
	const int start = Sys_Milliseconds();

	if ( !b->assets ) {
		b->assets = (smAsset_t *)Z_Malloc( SM_MAX_ASSETS * sizeof( smAsset_t ), TAG_GENERAL, qfalse );
		b->pool = (char *)Z_Malloc( SM_NAME_POOL, TAG_GENERAL, qfalse );
		b->hash = (int *)Z_Malloc( SM_HASH_SIZE * sizeof( int ), TAG_GENERAL, qfalse );
		b->rows = (smRow_t *)Z_Malloc( SM_MAX_ROWS * sizeof( smRow_t ), TAG_GENERAL, qfalse );
		b->view = (int *)Z_Malloc( SM_MAX_ASSETS * sizeof( int ), TAG_GENERAL, qfalse );
	}
	b->numAssets = b->numCustom = 0;
	b->poolUsed = 0;
	b->numSources = 0;
	memset( b->hash, 0, SM_HASH_SIZE * sizeof( int ) );

	for ( int i = 0; i < 2; i++ )
		FS_ListFilesRecursiveFrom( b->root, b->exts[i], SM_AddAsset, b );

	smSortBrowser = b;
	qsort( b->assets, b->numAssets, sizeof( b->assets[0] ), SM_CompareAssets );
	for ( int i = 0; i < b->numAssets; i++ ) {
		if ( b->assets[i].custom )
			b->numCustom++;
	}

	b->indexed = qtrue;
	b->asset = -1;
	memset( &b->folderList, 0, sizeof( b->folderList ) );
	memset( &b->assetList, 0, sizeof( b->assetList ) );
	b->folderList.lastClickRow = b->assetList.lastClickRow = -1;
	SM_BuildRows( b );
	SM_SetRow( b, SM_FirstFolderRow( b ) );
	Com_Printf( "%s: indexed %i %s, %i of them custom, in %i rows (%i ms)\n", b == &smSounds ? "Sound manager" : "Music manager",
		b->numAssets, b->what, b->numCustom, b->numRows, Sys_Milliseconds() - start );
}

static const smAsset_t *SM_Selected( const smBrowser_t *b ) {
	return ( b->asset >= 0 && b->asset < b->numAssets ) ? &b->assets[b->asset] : NULL;
}

static void SM_SelectViewRow( smBrowser_t *b, int row ) {
	if ( row < 0 || row >= b->numView )
		return;
	b->assetList.sel = row;
	b->asset = b->view[row];
}

// "Base, from base/assets1.pk3" or "Custom, from ..."
static const char *SM_SourceLabel( const smBrowser_t *b, const smAsset_t *a ) {
	return va( "%s, from %s", a->custom ? "Custom" : "Base", b->sources[a->source] );
}

/*
===============================================================================

STATE

===============================================================================
*/

typedef enum {
	SM_OFF,
	SM_SOUND,
	SM_MUSIC
} smOpen_t;

typedef enum {
	SM_TAB_SOUNDS,
	SM_TAB_PLAYING
} smTab_t;

typedef enum {
	SM_PLAY_ONCE,		// rpsound once
	SM_PLAY_LOOP,		// rpsound add
	SM_PLAY_LOCAL		// rpsound local
} smPlay_t;

typedef enum {
	SM_TO_EVERYONE,
	SM_TO_ME,
	SM_TO_PLAYER,
	SM_TO_SPOT
} smTo_t;

typedef enum {
	SM_F_NONE = -1,
	SM_F_SEARCH,		// the open browser's
	SM_F_X,
	SM_F_Y,
	SM_F_Z,
	SM_NUM_FIELDS
} smField_t;

// a looping sound, as the server's list has it
typedef struct {
	int			num;
	char		text[MAX_STRING_CHARS / 4];	// the line after the number
	char		path[MAX_QPATH];			// the sound in it, "" if none was found
} smLoop_t;

static struct {
	smOpen_t	open;
	int			font;
	qboolean	forStory;		// opened from a storyteller step, or the cinematic manager
	qboolean	forCine;		// opened from the cinematic manager

	// input; clicks and the wheel are handled where the control under them is drawn
	float		cursorX, cursorY;
	qboolean	click;
	float		clickX, clickY;
	int			wheel;
	smField_t	focus;
	char		fields[SM_NUM_FIELDS][SM_FIELD_LEN];	// the search box is the browser's own
	qboolean	playerPicker;
	smList_t	playerList;
	int			confirmTime;

	// the sound manager
	smTab_t		tab;
	smPlay_t	play;
	smTo_t		to;
	int			player;			// client number, -1 = none picked
	sfxHandle_t	previewSfx;
	int			previewUntil;	// the preview plays until then

	// its playing tab
	smLoop_t	loops[SM_MAX_LOOPS];
	int			numLoops;
	char		listNote[MAX_STRING_CHARS / 4];	// what else the list said, "no sounds playing"
	smList_t	loopList;
	int			listRequestTime;
	int			listLineTime;
	qboolean	listOurs;
	qboolean	listTaken;		// the reply started: the old list went

	// the music manager
	qboolean	musicPreview;	// this client's music is the preview, not the map's

	// commands go out one at a time, as fast as the server's flood protection lets them
	char		queue[SM_MAX_QUEUE][MAX_STRING_CHARS];
	int			numQueue;
	int			nextSend;
	int			sentTime;
	char		reply[MAX_STRING_CHARS];
	int			replyTime;
	char		message[MAX_STRING_CHARS];
	int			messageTime;
} sm;

static vec4_t smWhite		= { 1.0f, 1.0f, 1.0f, 1.0f };
static vec4_t smPanel		= { 0.0f, 0.0f, 0.0f, 0.65f };
static vec4_t smPanelLight	= { 0.15f, 0.15f, 0.15f, 0.8f };
static vec4_t smPanelFocus	= { 0.1f, 0.2f, 0.3f, 0.9f };
static vec4_t smPanelOpaque	= { 0.05f, 0.05f, 0.05f, 0.95f };
static vec4_t smHighlight	= { 0.2f, 0.45f, 0.8f, 0.8f };
static vec4_t smHover		= { 1.0f, 1.0f, 1.0f, 0.12f };
static vec4_t smHoverBox	= { 0.25f, 0.25f, 0.25f, 0.9f };
static vec4_t smDanger		= { 0.6f, 0.15f, 0.15f, 0.9f };
static vec4_t smBorder		= { 0.5f, 0.5f, 0.5f, 0.8f };
static vec4_t smDim			= { 0.7f, 0.7f, 0.7f, 1.0f };
static vec4_t smCustom		= { 0.6f, 1.0f, 0.6f, 1.0f };

static smBrowser_t *SM_Browser( void ) {
	return sm.open == SM_MUSIC ? &smMusic : &smSounds;
}

static void SM_Message( const char *text ) {
	Q_strncpyz( sm.message, text, sizeof( sm.message ) );
	sm.messageTime = cls.realtime;
}

/*
===============================================================================

COMMANDS

===============================================================================
*/

// "1000" as sv_floodProtect gives it, how long to wait between commands
static int SM_CommandGap( void ) {
	const char *info = cl.gameState.stringData + cl.gameState.stringOffsets[CS_SERVERINFO];
	const char *flood = Info_ValueForKey( info, "sv_floodProtect" );
	int value;

	if ( !flood[0] )
		return 1100;	// not told, play safe
	value = atoi( flood );
	if ( value <= 0 )
		return 0;
	return ( value == 1 ? 1000 : value ) + 100;
}

static void SM_Queue( const char *cmd ) {
	// opened from the cinematic manager or the storyteller, it goes in that instead
	if ( CL_Cinematic_CaptureSound( cmd ) )
		return;
	if ( CL_Storyteller_Capture( cmd ) )
		return;
	if ( sm.numQueue >= SM_MAX_QUEUE ) {
		Com_Printf( S_COLOR_YELLOW "Sound manager: too many commands waiting, dropped %s\n", cmd );
		return;
	}
	Q_strncpyz( sm.queue[sm.numQueue++], cmd, sizeof( sm.queue[0] ) );
}

static void SM_RunQueue( void ) {
	if ( !sm.numQueue || cls.realtime < sm.nextSend || cls.state != CA_ACTIVE )
		return;

	CL_AddReliableCommand( sm.queue[0], qfalse );
	if ( !Q_stricmp( sm.queue[0], "rpsound list" ) ) {
		sm.listRequestTime = cls.realtime;
		sm.listTaken = qfalse;
	} else {
		sm.sentTime = cls.realtime;
		Com_Printf( S_COLOR_GREEN "%s: " S_COLOR_WHITE "%s\n", !Q_stricmpn( sm.queue[0], "rpmusic", 7 ) ? "Music manager" : "Sound manager", sm.queue[0] );
	}
	sm.numQueue--;
	memmove( sm.queue[0], sm.queue[1], sm.numQueue * sizeof( sm.queue[0] ) );
	sm.nextSend = cls.realtime + SM_CommandGap();
}

static void SM_RequestList( void ) {
	for ( int i = 0; i < sm.numQueue; i++ ) {
		if ( !Q_stricmp( sm.queue[i], "rpsound list" ) )
			return;
	}
	SM_Queue( "rpsound list" );
}

// the list once what was just sent has happened
static void SM_SendAndRefresh( const char *cmd ) {
	SM_Queue( cmd );
	SM_RequestList();
}

static const char *SM_PlayerName( int clientNum ) {
	const char *cs;

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS )
		return "";
	cs = cl.gameState.stringData + cl.gameState.stringOffsets[CS_PLAYERS + clientNum];
	return Info_ValueForKey( cs, "n" );
}

// the engine plays an mp3 sound once and no more, so a looping one would go quiet
static qboolean SM_IsMp3( const char *path ) {
	const int len = strlen( path );

	return (qboolean)( len > 4 && !Q_stricmp( path + len - 4, ".mp3" ) );
}

// the targets each way of playing takes, as RPMod documents them
static qboolean SM_ToAllowed( smPlay_t play, smTo_t to ) {
	switch ( play ) {
	case SM_PLAY_ONCE:	return (qboolean)( to == SM_TO_EVERYONE || to == SM_TO_ME || to == SM_TO_PLAYER );
	case SM_PLAY_LOOP:	return (qboolean)( to == SM_TO_EVERYONE || to == SM_TO_PLAYER || to == SM_TO_SPOT );
	default:			return (qboolean)( to == SM_TO_ME || to == SM_TO_PLAYER );
	}
}

static void SM_SetPlay( smPlay_t play ) {
	sm.play = play;
	if ( !SM_ToAllowed( play, sm.to ) )
		sm.to = play == SM_PLAY_LOCAL ? SM_TO_ME : SM_TO_EVERYONE;
}

// the command for the selected sound, NULL and why if something is missing
static const char *SM_SoundCommand( const char **why ) {
	static char cmd[MAX_STRING_CHARS];
	static const char *verbs[] = { "once", "add", "local" };
	const smAsset_t *a = SM_Selected( &smSounds );

	*why = NULL;
	if ( !a ) {
		*why = "Pick a sound on the left";
		return NULL;
	}
	if ( strchr( a->path, ' ' ) ) {
		*why = "RPMod can't take a sound with a space in its name";
		return NULL;
	}
	if ( sm.play == SM_PLAY_LOOP && SM_IsMp3( a->path ) ) {
		*why = "An mp3 only plays once in this engine: loop a .wav, or play this once";
		return NULL;
	}
	Com_sprintf( cmd, sizeof( cmd ), "rpsound %s %s", verbs[sm.play], a->path );
	switch ( sm.to ) {
	case SM_TO_EVERYONE:
		Q_strcat( cmd, sizeof( cmd ), " all" );
		break;
	case SM_TO_PLAYER:
		if ( sm.player < 0 || !SM_PlayerName( sm.player )[0] ) {
			*why = "Pick the player";
			return NULL;
		}
		Q_strcat( cmd, sizeof( cmd ), va( " %i", sm.player ) );
		break;
	case SM_TO_SPOT:
		if ( !sm.fields[SM_F_X][0] || !sm.fields[SM_F_Y][0] || !sm.fields[SM_F_Z][0] ) {
			*why = "Set the spot: X, Y and Z, or Where I stand";
			return NULL;
		}
		Q_strcat( cmd, sizeof( cmd ), va( " %i %i %i", atoi( sm.fields[SM_F_X] ), atoi( sm.fields[SM_F_Y] ), atoi( sm.fields[SM_F_Z] ) ) );
		break;
	default:
		break;
	}
	return cmd;
}

static void SM_SendSound( void ) {
	const char *why, *cmd = SM_SoundCommand( &why );

	if ( !cmd ) {
		SM_Message( va( S_COLOR_YELLOW "%s", why ) );
		return;
	}
	// a looping one shows in the list
	if ( sm.play == SM_PLAY_LOOP )
		SM_SendAndRefresh( cmd );
	else
		SM_Queue( cmd );
}

// the music file as rpmusic takes it, NULL if it can't
static const char *SM_MusicArg( void ) {
	const smAsset_t *a = SM_Selected( &smMusic );

	return ( a && !strchr( a->path, ' ' ) ) ? a->path : NULL;
}

/*
===============================================================================

PREVIEWS

===============================================================================
*/

static void SM_StopPreview( void ) {
	if ( sm.previewUntil && cls.state == CA_ACTIVE )
		S_MuteSound( cl.snap.ps.clientNum, CHAN_MENU1 );
	sm.previewUntil = 0;
}

static void SM_PreviewSound( void ) {
	const smAsset_t *a = SM_Selected( &smSounds );

	SM_StopPreview();
	if ( !a )
		return;
	sm.previewSfx = S_RegisterSound( va( SM_SOUND_ROOT "/%s", a->path ) );
	if ( !sm.previewSfx ) {
		SM_Message( va( S_COLOR_YELLOW "Can't play %s", a->path ) );
		return;
	}
	S_StartLocalSound( sm.previewSfx, CHAN_MENU1 );
	sm.previewUntil = cls.realtime + Q_max( 250, (int)S_GetSampleLengthInMilliSeconds( sm.previewSfx ) );
}

// the map's music, as the configstring has it
static void SM_RestoreMusic( void ) {
	const char *s = cl.gameState.stringData + cl.gameState.stringOffsets[CS_MUSIC];
	char intro[MAX_QPATH], loop[MAX_QPATH];

	sm.musicPreview = qfalse;
	if ( cls.state != CA_ACTIVE )
		return;
	Q_strncpyz( intro, COM_Parse( &s ), sizeof( intro ) );
	Q_strncpyz( loop, COM_Parse( &s ), sizeof( loop ) );
	if ( !intro[0] ) {
		S_StopBackgroundTrack();
		return;
	}
	S_StartBackgroundTrack( intro, loop[0] ? loop : intro, qfalse );
}

static void SM_PreviewMusic( void ) {
	const smAsset_t *a = SM_Selected( &smMusic );
	const char *path;

	if ( !a )
		return;
	path = va( SM_MUSIC_ROOT "/%s", a->path );
	S_StopBackgroundTrack();
	S_StartBackgroundTrack( path, path, qfalse );
	sm.musicPreview = qtrue;
	if ( Cvar_VariableValue( "s_musicvolume" ) <= 0 )
		SM_Message( S_COLOR_YELLOW "Your music volume is 0: raise it in the sound settings to hear it" );
}

/*
===============================================================================

THE SERVER'S REPLIES

===============================================================================
*/

// "12: sound/ambience/rain.wav (1024 512 64)" or the like: the number, then whatever is said
// of it. The sound is the word ending in .wav or .mp3, "sound/" taken off
static qboolean SM_ParseLoopLine( const char *line, smLoop_t *out ) {
	const char *c = line, *word;
	int len;

	if ( !isdigit( (unsigned char)*c ) )
		return qfalse;
	out->num = atoi( c );
	while ( isdigit( (unsigned char)*c ) )
		c++;
	while ( *c == ' ' )
		c++;
	if ( *c != ':' && *c != '.' && *c != ')' && *c != '-' )
		return qfalse;
	c++;
	while ( *c == ' ' )
		c++;
	Q_strncpyz( out->text, c, sizeof( out->text ) );

	out->path[0] = '\0';
	for ( word = c; *word; ) {
		while ( *word == ' ' || *word == '(' || *word == '"' )
			word++;
		for ( len = 0; word[len] && word[len] != ' ' && word[len] != ')' && word[len] != '"'; len++ )
			;
		if ( len > 4 && ( !Q_stricmpn( word + len - 4, ".wav", 4 ) || !Q_stricmpn( word + len - 4, ".mp3", 4 ) ) ) {
			const int skip = !Q_stricmpn( word, SM_SOUND_ROOT "/", 6 ) ? 6 : 0;
			Q_strncpyz( out->path, word + skip, Q_min( len - skip + 1, (int)sizeof( out->path ) ) );
			break;
		}
		word += len;
		if ( *word )
			word++;
	}
	return qtrue;
}

// every server print goes through here on its way to the cgame; qtrue keeps it out of the console
qboolean CL_SoundManager_ServerPrint( const char *text ) {
	char buf[MAX_STRING_CHARS], *line, *next;
	qboolean swallow = qtrue, any = qfalse;
	const qboolean listing = (qboolean)( ( sm.listRequestTime && cls.realtime - sm.listRequestTime < SM_LIST_TIMEOUT )
		|| ( sm.listLineTime && cls.realtime - sm.listLineTime < SM_LIST_LINE_GAP ) );

	if ( cls.state != CA_ACTIVE )
		return qfalse;

	Q_strncpyz( buf, text, sizeof( buf ) );
	for ( line = buf; line; line = next ) {
		next = strchr( line, '\n' );
		if ( next )
			*next++ = '\0';
		Q_StripColor( line );
		while ( *line == ' ' || *line == '\t' )
			line++;
		for ( int len = strlen( line ); len > 0 && ( line[len - 1] == ' ' || line[len - 1] == '\r' ); len-- )
			line[len - 1] = '\0';
		if ( !line[0] )
			continue;
		any = qtrue;

		// the reply to the list we asked for: its numbered lines are the sounds, the rest is said of them
		if ( listing ) {
			smLoop_t loop;

			if ( !sm.listTaken ) {
				sm.listTaken = qtrue;
				sm.numLoops = 0;
				sm.listNote[0] = '\0';
			}
			sm.listRequestTime = 0;
			sm.listLineTime = cls.realtime;
			if ( SM_ParseLoopLine( line, &loop ) ) {
				if ( sm.numLoops < SM_MAX_LOOPS )
					sm.loops[sm.numLoops++] = loop;
			} else if ( !sm.listNote[0] ) {
				Q_strncpyz( sm.listNote, line, sizeof( sm.listNote ) );
			}
			continue;
		}

		swallow = qfalse;
		if ( sm.open != SM_OFF && cls.realtime - sm.sentTime < SM_REPLY_MS ) {
			Q_strncpyz( sm.reply, line, sizeof( sm.reply ) );
			sm.replyTime = cls.realtime;
		}
	}
	return (qboolean)( any && swallow );
}

/*
===============================================================================

OPENING AND CLOSING

===============================================================================
*/

static void SM_Close( void ) {
	if ( sm.open == SM_OFF )
		return;
	SM_StopPreview();
	if ( sm.musicPreview )
		SM_RestoreMusic();
	sm.open = SM_OFF;
	sm.focus = SM_F_NONE;
	sm.playerPicker = qfalse;
	Key_SetCatcher( Key_GetCatcher() & ~KEYCATCH_SOUNDMANAGER );
}

void CL_SoundManager_Close( void ) {
	SM_Close();
}

static qboolean SM_Open( smOpen_t which ) {
	smBrowser_t *b = which == SM_MUSIC ? &smMusic : &smSounds;

	if ( cls.state != CA_ACTIVE || !cls.cgameStarted ) {
		Com_Printf( "%s: join a server first\n", which == SM_MUSIC ? "Music manager" : "Sound manager" );
		return qfalse;
	}
	if ( sm.open == which )
		return qtrue;
	SM_Close();
	CL_ModelManager_Close();
	CL_NpcManager_Close();
	CL_ShaderManager_Close();
	CL_EffectManager_Close();
	CL_Storyteller_Close();
	CL_RpCinematic_EditorClose();
	CL_MapEditor_Close();

	sm.font = re->RegisterFont( "arialnb" );
	if ( !sm.font )
		sm.font = cls.menuFont;
	if ( !b->indexed )
		SM_BuildIndex( b );
	sm.open = which;
	sm.forStory = qfalse;
	sm.forCine = qfalse;
	sm.focus = SM_F_NONE;
	sm.playerPicker = qfalse;
	sm.cursorX = SCREEN_WIDTH * 0.5f;
	sm.cursorY = SCREEN_HEIGHT * 0.5f;
	sm.click = qfalse;
	sm.wheel = 0;
	if ( which == SM_SOUND && sm.tab == SM_TAB_PLAYING )
		SM_RequestList();
	Key_SetCatcher( Key_GetCatcher() | KEYCATCH_SOUNDMANAGER );
	return qtrue;
}

qboolean CL_SoundManager_Active( void ) {
	return (qboolean)( sm.open == SM_SOUND );
}

qboolean CL_MusicManager_Active( void ) {
	return (qboolean)( sm.open == SM_MUSIC );
}

// opened from a storyteller step
void CL_SoundManager_OpenForStory( void ) {
	if ( SM_Open( SM_SOUND ) ) {
		sm.forStory = qtrue;
		sm.tab = SM_TAB_SOUNDS;
		SM_Message( S_COLOR_GREEN "For the story: what you send here goes in it as steps, not to the server" );
	}
}

// opened from the cinematic manager's Sounds tab
void CL_SoundManager_OpenForCinematic( void ) {
	if ( SM_Open( SM_SOUND ) ) {
		sm.forStory = qtrue;
		sm.forCine = qtrue;
		sm.tab = SM_TAB_SOUNDS;
		SM_Message( S_COLOR_GREEN "For the cinematic: pick a sound and add it; it goes in the cinematic, not to the server" );
	}
}

void CL_MusicManager_OpenForStory( void ) {
	if ( SM_Open( SM_MUSIC ) ) {
		sm.forStory = qtrue;
		SM_Message( S_COLOR_GREEN "For the story: what you send here goes in it as steps, not to the server" );
	}
}

void CL_SoundManager_Init( void ) {
	memset( &sm, 0, sizeof( sm ) );
	sm.player = -1;
	sm.focus = SM_F_NONE;
	sm.loopList.lastClickRow = -1;
	sm.playerList.lastClickRow = -1;
}

// cgame is going away: map change, disconnect or vid_restart
void CL_SoundManager_Shutdown( void ) {
	SM_Close();
	sm.musicPreview = qfalse;
	sm.numQueue = 0;
	sm.listRequestTime = sm.listLineTime = 0;
	sm.numLoops = 0;
	sm.listNote[0] = '\0';
	// the pure pk3 list can change with the next map or server
	smSounds.indexed = smMusic.indexed = qfalse;
}

void CL_SoundManager_Frame( void ) {
	// queued commands go out with the manager closed too
	SM_RunQueue();
	if ( sm.previewUntil && cls.realtime >= sm.previewUntil )
		sm.previewUntil = 0;
}

/*
===============================================================================

CONSOLE COMMANDS

===============================================================================
*/

static void SM_Toggle( smOpen_t which ) {
	if ( sm.open == which )
		SM_Close();
	else
		SM_Open( which );
}

void CL_SoundManager_f( void ) {
	const char *arg = Cmd_Argv( 1 );

	if ( !arg[0] ) {
		SM_Toggle( SM_SOUND );
	} else if ( !Q_stricmp( arg, "close" ) ) {
		SM_Close();
	} else if ( !Q_stricmp( arg, "reindex" ) ) {
		SM_BuildIndex( &smSounds );
	} else if ( !Q_stricmp( arg, "list" ) ) {
		if ( cls.state == CA_ACTIVE )
			CL_AddReliableCommand( "rpsound list", qfalse );
	} else if ( !Q_stricmp( arg, "help" ) || !Q_stricmp( arg, "?" ) ) {
		Com_Printf( "usage: soundmanager            toggle the sound manager (bind a key to it)\n" );
		Com_Printf( "       soundmanager <search>   open it filtered by <search>\n" );
		Com_Printf( "       soundmanager list       the looping sounds playing (rpsound list)\n" );
		Com_Printf( "       soundmanager reindex    rescan the filesystem for sounds\n" );
		Com_Printf( "       soundmanager close      close it\n" );
	} else if ( SM_Open( SM_SOUND ) ) {
		// the browser filtered by it
		Q_strncpyz( smSounds.search, Cmd_ArgsFrom( 1 ), sizeof( smSounds.search ) );
		sm.tab = SM_TAB_SOUNDS;
		SM_RebuildView( &smSounds );
		SM_SelectViewRow( &smSounds, 0 );
	}
}


void CL_MusicManager_f( void ) {
	const char *arg = Cmd_Argv( 1 );

	if ( !arg[0] ) {
		SM_Toggle( SM_MUSIC );
	} else if ( !Q_stricmp( arg, "close" ) ) {
		SM_Close();
	} else if ( !Q_stricmp( arg, "reindex" ) ) {
		SM_BuildIndex( &smMusic );
	} else if ( !Q_stricmp( arg, "help" ) || !Q_stricmp( arg, "?" ) ) {
		Com_Printf( "usage: musicmanager            toggle the music manager (bind a key to it)\n" );
		Com_Printf( "       musicmanager <search>   open it filtered by <search>\n" );
		Com_Printf( "       musicmanager reindex    rescan the filesystem for music\n" );
		Com_Printf( "       musicmanager close      close it\n" );
	} else if ( SM_Open( SM_MUSIC ) ) {
		Q_strncpyz( smMusic.search, Cmd_ArgsFrom( 1 ), sizeof( smMusic.search ) );
		SM_RebuildView( &smMusic );
		SM_SelectViewRow( &smMusic, 0 );
	}
}

/*
===============================================================================

INPUT

===============================================================================
*/

static qboolean SM_InRect( float x, float y, float w, float h ) {
	return (qboolean)( sm.cursorX >= x && sm.cursorX < x + w && sm.cursorY >= y && sm.cursorY < y + h );
}

static qboolean SM_CtrlDown( void ) {
	return (qboolean)( Key_IsDown( A_CTRL ) || Key_IsDown( A_CTRL2 ) );
}

// the field typing goes to: a box clicked, else the search
static char *SM_FieldText( smField_t f ) {
	return f == SM_F_SEARCH ? SM_Browser()->search : sm.fields[f];
}

static smField_t SM_ActiveField( void ) {
	if ( sm.playerPicker )
		return SM_F_NONE;
	if ( sm.focus != SM_F_NONE )
		return sm.focus;
	if ( sm.open == SM_SOUND && sm.tab == SM_TAB_PLAYING )
		return SM_F_NONE;
	return SM_F_SEARCH;
}

// binds don't run while the manager holds the keys, so honour the user's toggle bind here
static qboolean SM_IsToggleKey( int key ) {
	const char *binding;

	// a printable key belongs to the text box
	if ( SM_ActiveField() != SM_F_NONE && key >= A_SPACE && key <= A_TILDE )
		return qfalse;
	binding = Key_GetBinding( key );
	return (qboolean)( VALIDSTRING( binding ) && !Q_stricmp( binding, sm.open == SM_MUSIC ? SM_MUSIC_CMD : SM_SOUND_CMD ) );
}

static qboolean SM_AllowChar( int ch ) {
	const smField_t f = SM_ActiveField();

	if ( f == SM_F_NONE || ch < ' ' || ch >= 127 )
		return qfalse;
	if ( f != SM_F_SEARCH && !( ( ch >= '0' && ch <= '9' ) || ch == '-' || ch == '.' ) )
		return qfalse;
	return qtrue;
}

static void SM_FieldChanged( smField_t f ) {
	if ( f == SM_F_SEARCH ) {
		smBrowser_t *b = SM_Browser();

		b->assetList.sel = b->assetList.scroll = 0;
		SM_RebuildView( b );
		if ( b->search[0] )
			SM_SelectViewRow( b, 0 );
	}
}

static void SM_Send( void ) {
	const char *arg;

	if ( sm.open == SM_SOUND ) {
		SM_SendSound();
		return;
	}
	arg = SM_MusicArg();
	if ( arg )
		SM_Queue( va( "rpmusic %s", arg ) );
}

static void SM_Preview( void ) {
	if ( sm.open == SM_SOUND )
		SM_PreviewSound();
	else
		SM_PreviewMusic();
}

static void SM_ListKey( int key ) {
	smBrowser_t *b = SM_Browser();
	const int visible = (int)( SM_LIST_H / SM_ROW_H );
	smList_t *list;
	int count, delta;

	if ( sm.playerPicker )
		return;
	if ( sm.open == SM_SOUND && sm.tab == SM_TAB_PLAYING ) {
		list = &sm.loopList;
		count = sm.numLoops;
	} else {
		if ( key == A_CURSOR_LEFT || key == A_CURSOR_RIGHT ) {
			if ( !b->search[0] ) {
				SM_SetRow( b, b->row + ( key == A_CURSOR_LEFT ? -1 : 1 ) );
				SM_SelectViewRow( b, 0 );
			}
			return;
		}
		list = &b->assetList;
		count = b->numView;
	}

	switch ( key ) {
	case A_CURSOR_UP:	delta = -1; break;
	case A_CURSOR_DOWN:	delta = 1; break;
	case A_PAGE_UP:		delta = -visible; break;
	case A_PAGE_DOWN:	delta = visible; break;
	case A_HOME:		delta = -count; break;
	case A_END:			delta = count; break;
	default:			return;
	}
	SM_ListMove( list, count, visible, delta );
	if ( list == &b->assetList )
		SM_SelectViewRow( b, list->sel );
}

void CL_SoundManager_KeyEvent( int key, qboolean down ) {
	smField_t f;

	if ( sm.open == SM_OFF || !down )
		return;
	if ( SM_IsToggleKey( key ) ) {
		SM_Close();
		return;
	}

	switch ( key ) {
	case A_MOUSE1:
		sm.click = qtrue;
		sm.clickX = sm.cursorX;
		sm.clickY = sm.cursorY;
		return;
	case A_MWHEELUP:
		sm.wheel -= 3;
		return;
	case A_MWHEELDOWN:
		sm.wheel += 3;
		return;
	case A_TAB:
		if ( sm.open == SM_SOUND ) {
			sm.tab = sm.tab == SM_TAB_SOUNDS ? SM_TAB_PLAYING : SM_TAB_SOUNDS;
			sm.focus = SM_F_NONE;
			if ( sm.tab == SM_TAB_PLAYING )
				SM_RequestList();
		}
		return;
	case A_ENTER:
	case A_KP_ENTER:
		if ( sm.playerPicker )
			return;
		if ( SM_CtrlDown() )
			SM_Preview();
		else if ( !( sm.open == SM_SOUND && sm.tab == SM_TAB_PLAYING ) )
			SM_Send();
		return;
	default:
		break;
	}

	f = SM_ActiveField();
	if ( f != SM_F_NONE && ( sm.focus != SM_F_NONE || !( key == A_CURSOR_LEFT || key == A_CURSOR_RIGHT || key == A_CURSOR_UP
		|| key == A_CURSOR_DOWN || key == A_HOME || key == A_END ) ) ) {
		const int edit = WG_EditKey( SM_FieldText( f ), SM_FIELD_LEN, key, SM_AllowChar );

		if ( edit & WG_EDIT_CHANGED )
			SM_FieldChanged( f );
		if ( edit )
			return;
	}
	SM_ListKey( key );
}

void CL_SoundManager_CharEvent( int ch ) {
	const smField_t f = SM_ActiveField();

	if ( sm.open == SM_OFF || f == SM_F_NONE )
		return;
	if ( WG_EditChar( SM_FieldText( f ), SM_FIELD_LEN, ch, SM_AllowChar ) )
		SM_FieldChanged( f );
}

void CL_SoundManager_MouseEvent( int dx, int dy ) {
	sm.cursorX = Com_Clamp( 0, SCREEN_WIDTH, sm.cursorX + dx );
	sm.cursorY = Com_Clamp( 0, SCREEN_HEIGHT, sm.cursorY + dy );
}

// escape backs out one level: the player list, a text box, then the manager
void CL_SoundManager_Escape( void ) {
	if ( sm.playerPicker )
		sm.playerPicker = qfalse;
	else if ( sm.focus != SM_F_NONE )
		sm.focus = SM_F_NONE;
	else
		SM_Close();
}

/*
===============================================================================

DRAWING

===============================================================================
*/

static void SM_Fill( float x, float y, float w, float h, const float *color ) {
	re->SetColor( color );
	re->DrawStretchPic( x, y, w, h, 0, 0, 0, 0, cls.whiteShader );
	re->SetColor( NULL );
}

static void SM_Box( float x, float y, float w, float h, const float *fill ) {
	if ( fill )
		SM_Fill( x, y, w, h, fill );
	SM_Fill( x, y, w, 1, smBorder );
	SM_Fill( x, y + h - 1, w, 1, smBorder );
	SM_Fill( x, y, 1, h, smBorder );
	SM_Fill( x + w - 1, y, 1, h, smBorder );
}

static void SM_Text( float x, float y, const char *text, const float *color ) {
	re->Font_DrawString( (int)x, (int)y, text, color, sm.font | STYLE_DROPSHADOW, -1, SM_TEXT_SCALE );
}

static float SM_TextWidth( const char *text ) {
	return (float)re->Font_StrLenPixels( text, sm.font, SM_TEXT_SCALE );
}

static void SM_TextClipped( float x, float y, float w, const char *text, const float *color ) {
	char buf[MAX_STRING_CHARS];
	int len;

	Q_strncpyz( buf, text, sizeof( buf ) );
	len = strlen( buf );
	while ( len > 3 && SM_TextWidth( buf ) > w ) {
		len--;
		buf[len - 3] = '.';
		buf[len - 2] = '.';
		buf[len - 1] = '.';
		buf[len] = '\0';
	}
	SM_Text( x, y, buf, color );
}

// eats this frame's click if it landed in the rect
static qboolean SM_Clicked( float x, float y, float w, float h ) {
	if ( !sm.click || sm.clickX < x || sm.clickX >= x + w || sm.clickY < y || sm.clickY >= y + h )
		return qfalse;
	sm.click = qfalse;
	return qtrue;
}

static qboolean SM_Button( float x, float y, float w, const char *label, qboolean selected, qboolean enabled ) {
	const float *fill = selected ? smHighlight : ( enabled && SM_InRect( x, y, w, SM_CTRL_H ) ) ? smHoverBox : smPanelLight;

	SM_Box( x, y, w, SM_CTRL_H, fill );
	SM_Text( x + ( w - SM_TextWidth( label ) ) * 0.5f, y + 2, label, enabled ? smWhite : smDim );
	return (qboolean)( SM_Clicked( x, y, w, SM_CTRL_H ) && enabled );
}

// a button that has to be clicked twice
static qboolean SM_DangerButton( float x, float y, float w, const char *label, qboolean enabled ) {
	const qboolean armed = (qboolean)( sm.confirmTime && cls.realtime - sm.confirmTime < SM_CONFIRM_MS );
	const char *text = armed ? "Click again" : label;

	SM_Box( x, y, w, SM_CTRL_H, armed ? smDanger : ( enabled && SM_InRect( x, y, w, SM_CTRL_H ) ) ? smHoverBox : smPanelLight );
	SM_Text( x + ( w - SM_TextWidth( text ) ) * 0.5f, y + 2, text, enabled ? smWhite : smDim );
	if ( !SM_Clicked( x, y, w, SM_CTRL_H ) || !enabled )
		return qfalse;
	if ( armed ) {
		sm.confirmTime = 0;
		return qtrue;
	}
	sm.confirmTime = cls.realtime;
	return qfalse;
}

static void SM_Field( smField_t f, float x, float y, float w, const char *placeholder ) {
	const qboolean focused = (qboolean)( SM_ActiveField() == f );
	char *text = SM_FieldText( f );
	wgStyle_t style;

	style.font = sm.font;
	style.scale = SM_TEXT_SCALE;
	style.lineH = SM_ROW_H;
	style.padX = 4.0f;
	style.padY = 2.0f;
	style.fill = smPanelFocus;
	style.border = smBorder;
	style.text = smWhite;
	style.dim = smDim;
	SM_Box( x, y, w, SM_CTRL_H, focused ? smPanelFocus : smPanelLight );
	WG_Field( &style, x, y, w, SM_CTRL_H, text, focused, placeholder, sm.cursorX, sm.cursorY );
	// a click: typed in, the cursor where it was clicked; the search clicked takes the arrows too
	if ( SM_Clicked( x, y, w, WG_FieldHeight( &style, w, SM_CTRL_H, text, focused ) ) ) {
		sm.focus = f;
		WG_FieldClick( text, sm.clickX, sm.clickY );
	}
}

static void SM_Label( float x, float y, const char *label ) {
	SM_Text( x, y + 2, label, smDim );
}

typedef void ( *smRowFunc_t )( int row, float x, float y, float w );

// returns the row double-clicked, -1 if none; *clicked gets the row clicked once
static int SM_List( smList_t *list, int count, float x, float y, float w, float h, smRowFunc_t drawRow, int *clicked ) {
	const int visible = (int)( h / SM_ROW_H );
	int row, picked = -1;

	if ( clicked )
		*clicked = -1;
	if ( sm.wheel && SM_InRect( x, y, w, h ) ) {
		list->scroll += sm.wheel;
		sm.wheel = 0;
	}
	if ( list->scroll > count - visible )
		list->scroll = count - visible;
	if ( list->scroll < 0 )
		list->scroll = 0;
	SM_Box( x, y, w, h, smPanelLight );
	if ( count > visible ) {
		WG_Scrollbar( x + w - WG_SCROLL_W - 1, y + 1, h - 2, count, visible, &list->scroll, sm.cursorX, sm.cursorY, &sm.click, sm.clickX, sm.clickY );
		w -= WG_SCROLL_W + 1;
	}

	if ( SM_Clicked( x, y, w, h ) ) {
		row = list->scroll + (int)( ( sm.clickY - y ) / SM_ROW_H );
		if ( row < count ) {
			if ( row == list->lastClickRow && cls.realtime - list->lastClickTime < SM_DOUBLECLICK_MS ) {
				list->lastClickTime = 0;
				picked = row;
			} else {
				list->lastClickRow = row;
				list->lastClickTime = cls.realtime;
			}
			list->sel = row;
			if ( clicked )
				*clicked = row;
		}
	}

	for ( int i = 0; i < visible; i++ ) {
		const float ry = y + i * SM_ROW_H;

		row = list->scroll + i;
		if ( row >= count )
			break;
		if ( row == list->sel )
			SM_Fill( x + 1, ry, w - 2, SM_ROW_H, smHighlight );
		else if ( SM_InRect( x, ry, w, SM_ROW_H ) )
			SM_Fill( x + 1, ry, w - 2, SM_ROW_H, smHover );
		drawRow( row, x + 4, ry + 1, w - 8 );
	}
	return picked;
}

// Base and its folders, Custom with its pk3s and theirs indented under them
static void SM_DrawFolderRow( int row, float x, float y, float w ) {
	const smBrowser_t *b = SM_Browser();
	const smRow_t *r = &b->rows[row];
	const float *color = b->search[0] ? smDim : smWhite;
	const char *slash;
	float indent;

	switch ( r->type ) {
	case SM_ROW_CATEGORY:
		SM_TextClipped( x, y, w, va( S_COLOR_YELLOW "%s " S_COLOR_GREY "(%i)", r->name, r->count ), color );
		return;
	case SM_ROW_SOURCE:
		// "base/name.pk3" as "name.pk3", a loose folder as itself
		slash = strrchr( r->name, '/' );
		indent = 6;
		SM_TextClipped( x + indent, y, w - indent, strlen( r->name ) > 4 && !Q_stricmp( r->name + strlen( r->name ) - 4, ".pk3" )
			? va( S_COLOR_GREEN "%s " S_COLOR_GREY "(%i)", slash ? slash + 1 : r->name, r->count )
			: va( S_COLOR_GREEN "%s folder " S_COLOR_GREY "(%i)", r->name, r->count ), color );
		return;
	default:
		indent = r->custom ? 12.0f : 6.0f;
		SM_TextClipped( x + indent, y, w - indent, va( "%s " S_COLOR_GREY "(%i)", r->name[0] ? r->name : "(root)", r->count ), color );
		return;
	}
}

// custom ones in green; a folder lists names, anything wider where they are
static void SM_DrawAssetRow( int row, float x, float y, float w ) {
	const smBrowser_t *b = SM_Browser();
	const smAsset_t *a = &b->assets[b->view[row]];

	SM_TextClipped( x, y, w, b->search[0] || b->rows[b->row].type != SM_ROW_FOLDER ? a->path : a->base, a->custom ? smCustom : smWhite );
}

// the search, the filter, the folders and what's in them; what was double-clicked is heard
static void SM_DrawBrowser( void ) {
	smBrowser_t *b = SM_Browser();
	const float searchW = SM_FOLDER_W + SM_ASSET_W + 4 - SM_FILTER_COUNT * ( SM_FILTER_W + 4 );
	int picked, clicked;

	SM_Field( SM_F_SEARCH, SM_FOLDER_X, SM_SEARCH_Y, searchW, va( "Type to search all %s", b->what ) );
	for ( int i = 0; i < SM_FILTER_COUNT; i++ ) {
		if ( SM_Button( SM_FOLDER_X + searchW + 4 + i * ( SM_FILTER_W + 4 ), SM_SEARCH_Y, SM_FILTER_W, smFilterNames[i], (qboolean)( b->filter == i ), qtrue ) ) {
			SM_SetFilter( b, (smFilter_t)i );
			SM_SelectViewRow( b, 0 );
		}
	}
	b->folderList.sel = b->search[0] ? -1 : b->row;
	SM_List( &b->folderList, b->numRows, SM_FOLDER_X, SM_LIST_Y, SM_FOLDER_W, SM_LIST_H, SM_DrawFolderRow, &clicked );
	if ( clicked >= 0 && clicked < b->numRows ) {
		SM_SetRow( b, clicked );
		SM_SelectViewRow( b, 0 );
	}
	picked = SM_List( &b->assetList, b->numView, SM_ASSET_X, SM_LIST_Y, SM_ASSET_W, SM_LIST_H, SM_DrawAssetRow, &clicked );
	if ( clicked >= 0 && clicked < b->numView )
		SM_SelectViewRow( b, clicked );
	if ( picked >= 0 )
		SM_Preview();
	if ( !b->numView )
		SM_Text( SM_ASSET_X + 4, SM_LIST_Y + 1, b->numAssets ? "Nothing matches" : va( "No %s found", b->what ), smDim );
}

// the selected file: its path, where it's from
static float SM_DrawSelected( float y ) {
	const smBrowser_t *b = SM_Browser();
	const smAsset_t *a = SM_Selected( b );

	if ( !a ) {
		SM_Text( SM_RIGHT_X, y, va( S_COLOR_GREY "Pick one of the %s on the left", b->what ), smWhite );
		return y + 32;
	}
	SM_TextClipped( SM_RIGHT_X, y, SM_RIGHT_W, va( S_COLOR_YELLOW "%s", a->path ), smWhite );
	SM_TextClipped( SM_RIGHT_X, y + 14, SM_RIGHT_W, va( S_COLOR_GREY "%s", SM_SourceLabel( b, a ) ), smWhite );
	return y + 32;
}

static void SM_DrawPlayerRow( int row, float x, float y, float w );

// what will be sent, wrapped over the lines above the buttons
static void SM_DrawCommand( const char *cmd, const char *why ) {
	if ( cmd )
		SM_TextClipped( SM_RIGHT_X, SM_BUTTON_Y - 18, SM_RIGHT_W, va( S_COLOR_GREY "Will send: " S_COLOR_WHITE "%s", cmd ), smWhite );
	else if ( why )
		SM_TextClipped( SM_RIGHT_X, SM_BUTTON_Y - 18, SM_RIGHT_W, va( S_COLOR_YELLOW "%s", why ), smWhite );
}

// where the crosshair meets the world, a little off it
static void SM_AimPoint( vec3_t out ) {
	vec3_t start, end, forward;
	trace_t tr;

	VectorCopy( cl.snap.ps.origin, start );
	start[2] += cl.snap.ps.viewheight;
	AngleVectors( cl.viewangles, forward, NULL, NULL );
	VectorMA( start, 8192, forward, end );
	CM_BoxTrace( &tr, start, end, vec3_origin, vec3_origin, 0, MASK_SOLID, qfalse );
	VectorMA( tr.endpos, -8, forward, out );
}

static void SM_SetSpot( const vec3_t p ) {
	for ( int i = 0; i < 3; i++ )
		Com_sprintf( sm.fields[SM_F_X + i], SM_FIELD_LEN, "%i", (int)floorf( p[i] + 0.5f ) );
}

static void SM_DrawSounds( void ) {
	static const char *toNames[] = { "Everyone", "Me", "A player", "A spot" };
	const smAsset_t *a = SM_Selected( &smSounds );
	const char *cmd, *why;
	float y = SM_SEARCH_Y, x;

	SM_DrawBrowser();
	y = SM_DrawSelected( y );

	// hear it here first
	if ( a && sm.previewSfx && sm.previewUntil )
		SM_TextClipped( SM_RIGHT_X, y - 4, SM_RIGHT_W, va( S_COLOR_GREY "%.1f s", S_GetSampleLengthInMilliSeconds( sm.previewSfx ) / 1000.0f ), smWhite );
	if ( SM_Button( SM_RIGHT_X, y + 10, 120, "Preview (Ctrl+Enter)", (qboolean)( sm.previewUntil != 0 ), (qboolean)( a != NULL ) ) )
		SM_PreviewSound();
	if ( SM_Button( SM_RIGHT_X + 124, y + 10, 60, "Stop", qfalse, (qboolean)( sm.previewUntil != 0 ) ) )
		SM_StopPreview();
	SM_Text( SM_RIGHT_X + 190, y + 12, S_COLOR_GREY "only you hear it", smWhite );

	y += 36;
	SM_Label( SM_RIGHT_X, y, "Play" );
	if ( SM_Button( SM_CTRL_X, y, 74, "Once", (qboolean)( sm.play == SM_PLAY_ONCE ), qtrue ) )
		SM_SetPlay( SM_PLAY_ONCE );
	if ( SM_Button( SM_CTRL_X + 78, y, 74, "Loop (add)", (qboolean)( sm.play == SM_PLAY_LOOP ), (qboolean)!( a && SM_IsMp3( a->path ) ) ) )
		SM_SetPlay( SM_PLAY_LOOP );
	if ( SM_Button( SM_CTRL_X + 156, y, 78, "Local", (qboolean)( sm.play == SM_PLAY_LOCAL ), qtrue ) )
		SM_SetPlay( SM_PLAY_LOCAL );
	y += 18;
	SM_TextClipped( SM_CTRL_X, y, SM_CTRL_W, a && SM_IsMp3( a->path ) ? S_COLOR_GREY "An mp3 plays once only, so it can't loop"
		: sm.play == SM_PLAY_ONCE ? S_COLOR_GREY "Played once, where it's heard from the player"
		: sm.play == SM_PLAY_LOOP ? S_COLOR_GREY "Keeps playing until removed (Playing tab)"
		: S_COLOR_GREY "Played once, heard only by that player", smWhite );

	y += 18;
	SM_Label( SM_RIGHT_X, y, "To" );
	x = SM_CTRL_X;
	for ( int i = 0; i < 4; i++ ) {
		if ( !SM_ToAllowed( sm.play, (smTo_t)i ) )
			continue;
		if ( SM_Button( x, y, 74, toNames[i], (qboolean)( sm.to == i ), qtrue ) ) {
			sm.to = (smTo_t)i;
			if ( sm.to == SM_TO_PLAYER && sm.player < 0 )
				sm.playerPicker = qtrue;
			if ( sm.to == SM_TO_SPOT && !sm.fields[SM_F_X][0] )
				SM_SetSpot( cl.snap.ps.origin );
		}
		x += 78;
	}

	y += 22;
	if ( sm.to == SM_TO_PLAYER ) {
		SM_Label( SM_RIGHT_X, y, "Player" );
		SM_TextClipped( SM_CTRL_X, y + 2, SM_CTRL_W - 84, sm.player >= 0 && SM_PlayerName( sm.player )[0]
			? va( "%s " S_COLOR_GREY "(%i)", SM_PlayerName( sm.player ), sm.player ) : S_COLOR_GREY "none picked", smWhite );
		if ( SM_Button( SM_CTRL_X + SM_CTRL_W - 80, y, 80, "Pick...", qfalse, qtrue ) )
			sm.playerPicker = qtrue;
	} else if ( sm.to == SM_TO_SPOT ) {
		SM_Label( SM_RIGHT_X, y, "At" );
		SM_Field( SM_F_X, SM_CTRL_X, y, 54, "x" );
		SM_Field( SM_F_Y, SM_CTRL_X + 58, y, 54, "y" );
		SM_Field( SM_F_Z, SM_CTRL_X + 116, y, 54, "z" );
		y += 18;
		if ( SM_Button( SM_CTRL_X, y, 112, "Where I stand", qfalse, qtrue ) )
			SM_SetSpot( cl.snap.ps.origin );
		if ( SM_Button( SM_CTRL_X + 116, y, 112, "Where I aim", qfalse, qtrue ) ) {
			vec3_t p;

			SM_AimPoint( p );
			SM_SetSpot( p );
		}
	} else {
		SM_TextClipped( SM_CTRL_X, y + 2, SM_CTRL_W, sm.to == SM_TO_ME ? S_COLOR_GREY "Only to you" : S_COLOR_GREY "Everyone on the server", smWhite );
	}

	cmd = SM_SoundCommand( &why );
	SM_DrawCommand( cmd, a ? why : NULL );
	if ( SM_Button( SM_RIGHT_X, SM_BUTTON_Y, 100, sm.forCine ? "Add to cinematic" : sm.forStory ? "Add to story" : "Send  (Enter)", qfalse, (qboolean)( cmd != NULL ) ) )
		SM_SendSound();
}

static void SM_DrawLoopRow( int row, float x, float y, float w ) {
	const smLoop_t *l = &sm.loops[row];

	SM_TextClipped( x, y, w, va( S_COLOR_CYAN "%i " S_COLOR_WHITE "%s", l->num, l->text ), smWhite );
}

static void SM_DrawPlaying( void ) {
	const smLoop_t *l = sm.loopList.sel >= 0 && sm.loopList.sel < sm.numLoops ? &sm.loops[sm.loopList.sel] : NULL;
	const qboolean waiting = (qboolean)( sm.listRequestTime && cls.realtime - sm.listRequestTime < SM_LIST_TIMEOUT );
	float y = SM_SEARCH_Y;
	int picked;

	SM_Text( SM_FOLDER_X, y + 2, va( "Looping sounds" S_COLOR_GREY "  %i", sm.numLoops ), smWhite );
	picked = SM_List( &sm.loopList, sm.numLoops, SM_FOLDER_X, SM_LIST_Y, SM_LOOPS_W, SM_LIST_H, SM_DrawLoopRow, NULL );
	if ( !sm.numLoops )
		SM_TextClipped( SM_FOLDER_X + 4, SM_LIST_Y + 1, SM_LOOPS_W - 8, waiting ? "Asking the server..."
			: sm.listNote[0] ? sm.listNote : "None that the server listed", smDim );

	const float rx = SM_FOLDER_X + SM_LOOPS_W + 12, rw = SCREEN_WIDTH - SM_FOLDER_X - rx;

	if ( SM_Button( rx, y, 90, waiting ? "Asking..." : "Refresh", qfalse, (qboolean)!waiting ) )
		SM_RequestList();
	if ( sm.numLoops && sm.listNote[0] )
		SM_TextClipped( rx, y + 20, rw, va( S_COLOR_GREY "%s", sm.listNote ), smWhite );

	y = SM_LIST_Y + 24;
	if ( l ) {
		SM_TextClipped( rx, y, rw, va( S_COLOR_YELLOW "%i " S_COLOR_WHITE "%s", l->num, l->text ), smWhite );
		y += 20;
		if ( SM_Button( rx, y, 90, "Remove", qfalse, qtrue ) ) {
			SM_SendAndRefresh( va( "rpsound remove %i", l->num ) );
			SM_Message( va( "Removing sound %i", l->num ) );
		}
		if ( SM_Button( rx + 94, y, 90, "Preview", qfalse, (qboolean)( l->path[0] != '\0' ) ) || ( picked >= 0 && l->path[0] ) ) {
			SM_StopPreview();
			sm.previewSfx = S_RegisterSound( va( SM_SOUND_ROOT "/%s", l->path ) );
			if ( sm.previewSfx ) {
				S_StartLocalSound( sm.previewSfx, CHAN_MENU1 );
				sm.previewUntil = cls.realtime + Q_max( 250, (int)S_GetSampleLengthInMilliSeconds( sm.previewSfx ) );
			}
		}
		if ( SM_Button( rx + 188, y, 90, "Stop preview", qfalse, (qboolean)( sm.previewUntil != 0 ) ) )
			SM_StopPreview();
	} else {
		SM_TextClipped( rx, y, rw, S_COLOR_GREY "Pick one to remove it", smWhite );
	}

	if ( SM_DangerButton( rx, SM_BUTTON_Y, 120, "Remove all", qtrue ) ) {
		SM_SendAndRefresh( "rpsound remove all" );
		SM_Message( "Removing all the looping sounds" );
	}
}

static void SM_DrawMusic( void ) {
	const char *s = cl.gameState.stringData + cl.gameState.stringOffsets[CS_MUSIC];
	const smAsset_t *a = SM_Selected( &smMusic );
	const char *arg = SM_MusicArg();
	char playing[MAX_QPATH];
	float y = SM_SEARCH_Y;

	SM_DrawBrowser();
	y = SM_DrawSelected( y );

	if ( SM_Button( SM_RIGHT_X, y + 10, 120, "Preview (Ctrl+Enter)", sm.musicPreview, (qboolean)( a != NULL ) ) )
		SM_PreviewMusic();
	if ( SM_Button( SM_RIGHT_X + 124, y + 10, 100, "Stop preview", qfalse, sm.musicPreview ) )
		SM_RestoreMusic();
	y += 30;
	SM_TextClipped( SM_RIGHT_X, y, SM_RIGHT_W, sm.musicPreview ? S_COLOR_GREY "Only you hear it; the map's music comes back when it stops"
		: S_COLOR_GREY "Only you hear a preview, instead of the map's music", smWhite );

	y += 26;
	Q_strncpyz( playing, COM_Parse( &s ), sizeof( playing ) );
	SM_Label( SM_RIGHT_X, y, "Now" );
	SM_TextClipped( SM_CTRL_X, y + 2, SM_CTRL_W, playing[0] ? playing : S_COLOR_GREY "no music", smWhite );
	y += 16;
	SM_TextClipped( SM_CTRL_X, y + 2, SM_CTRL_W, S_COLOR_GREY "what the map plays for everyone", smWhite );

	y += 26;
	SM_Label( SM_RIGHT_X, y, "For all" );
	if ( SM_Button( SM_CTRL_X, y, 112, "Map's own music", qfalse, qtrue ) )
		SM_Queue( "rpmusic default" );
	if ( SM_Button( SM_CTRL_X + 116, y, 112, "No music", qfalse, qtrue ) )
		SM_Queue( "rpmusic none" );
	y += 20;
	if ( SM_Button( SM_CTRL_X, y, 112, "Ask the server", qfalse, (qboolean)!sm.forStory ) ) {
		SM_Queue( "rpmusic" );
	}

	if ( a && !arg )
		SM_DrawCommand( NULL, "RPMod can't take a file with a space in its name" );
	else if ( arg )
		SM_DrawCommand( va( "rpmusic %s", arg ), NULL );
	if ( SM_Button( SM_RIGHT_X, SM_BUTTON_Y, 140, sm.forStory ? "Add to story" : "Play for everyone (Enter)", qfalse, (qboolean)( arg != NULL ) ) )
		SM_Queue( va( "rpmusic %s", arg ) );
}

static void SM_DrawPlayerRow( int row, float x, float y, float w ) {
	int n = 0;

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !SM_PlayerName( i )[0] )
			continue;
		if ( n++ == row ) {
			SM_TextClipped( x, y, w, va( S_COLOR_CYAN "%i  " S_COLOR_WHITE "%s", i, SM_PlayerName( i ) ), smWhite );
			return;
		}
	}
}

// the n-th connected player's client number, -1 if none
static int SM_PlayerAt( int row ) {
	int n = 0;

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( SM_PlayerName( i )[0] && n++ == row )
			return i;
	}
	return -1;
}

static void SM_DrawPlayerPicker( void ) {
	int count = 0, picked, clicked;

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( SM_PlayerName( i )[0] )
			count++;
	}
	SM_Box( SM_POPUP_X, SM_POPUP_Y, SM_POPUP_W, SM_POPUP_H, smPanelOpaque );
	SM_Text( SM_POPUP_X + 8, SM_POPUP_Y + 8, va( "Pick a player" S_COLOR_GREY "  %i", count ), smWhite );
	picked = SM_List( &sm.playerList, count, SM_POPUP_X + 8, SM_POPUP_Y + 26, SM_POPUP_W - 16, SM_POPUP_H - 60, SM_DrawPlayerRow, &clicked );
	if ( clicked >= 0 )
		sm.player = SM_PlayerAt( clicked );
	if ( picked >= 0 || SM_Button( SM_POPUP_X + 8, SM_POPUP_Y + SM_POPUP_H - 22, 80, "Pick", qfalse, (qboolean)( sm.playerList.sel >= 0 ) ) ) {
		sm.player = SM_PlayerAt( sm.playerList.sel );
		sm.to = SM_TO_PLAYER;
		sm.playerPicker = qfalse;
	}
	if ( SM_Button( SM_POPUP_X + SM_POPUP_W - 88, SM_POPUP_Y + SM_POPUP_H - 22, 80, "Cancel", qfalse, qtrue ) )
		sm.playerPicker = qfalse;
}

void CL_SoundManager_Draw( void ) {
	char buf[MAX_STRING_CHARS];
	const qboolean popup = sm.playerPicker;
	const qboolean click = sm.click;
	const int wheel = sm.wheel;

	if ( sm.open == SM_OFF )
		return;

	// the popup sits on top and gets the clicks first, but is drawn last
	if ( popup )
		sm.click = qfalse, sm.wheel = 0;

	SM_Fill( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, smPanel );
	if ( sm.open == SM_SOUND ) {
		SM_Text( SM_FOLDER_X, 14, "Sound Manager", smWhite );
		if ( SM_Button( 112, 12, 64, "Sounds", (qboolean)( sm.tab == SM_TAB_SOUNDS ), qtrue ) ) {
			sm.tab = SM_TAB_SOUNDS;
			sm.focus = SM_F_NONE;
		}
		if ( SM_Button( 180, 12, 64, "Playing", (qboolean)( sm.tab == SM_TAB_PLAYING ), (qboolean)!sm.forStory ) ) {
			sm.tab = SM_TAB_PLAYING;
			sm.focus = SM_F_NONE;
			SM_RequestList();
		}
	} else {
		SM_Text( SM_FOLDER_X, 14, "Music Manager", smWhite );
	}

	// the server's answer to the last command, and the manager's own
	if ( sm.reply[0] && cls.realtime - sm.replyTime < 8000 )
		SM_TextClipped( SM_RIGHT_X, 14, SM_RIGHT_W, va( S_COLOR_YELLOW "Server: " S_COLOR_WHITE "%s", sm.reply ), smWhite );
	if ( sm.message[0] && cls.realtime - sm.messageTime < SM_MESSAGE_MS )
		SM_TextClipped( SM_FOLDER_X, 29, SCREEN_WIDTH - SM_FOLDER_X * 2, sm.message, smWhite );

	if ( sm.open == SM_MUSIC )
		SM_DrawMusic();
	else if ( sm.tab == SM_TAB_PLAYING && !sm.forStory )
		SM_DrawPlaying();
	else
		SM_DrawSounds();

	if ( sm.open == SM_SOUND && sm.tab == SM_TAB_PLAYING )
		SM_Text( SM_FOLDER_X, 440, S_COLOR_GREY "Up/Down select   Double-click preview   Wheel scroll   Tab switch tab", smWhite );
	else
		SM_Text( SM_FOLDER_X, 440, va( S_COLOR_GREY "Up/Down pick   Left/Right folder   Double-click or Ctrl+Enter preview   Enter %s%s",
			sm.open == SM_MUSIC ? "play" : "send", sm.open == SM_SOUND ? "   Tab switch tab" : "" ), smWhite );
	Com_sprintf( buf, sizeof( buf ), S_COLOR_GREY "Esc back / close   %s", Key_GetKey( sm.open == SM_MUSIC ? SM_MUSIC_CMD : SM_SOUND_CMD ) >= 0
		? va( "Your %s bind closes", sm.open == SM_MUSIC ? SM_MUSIC_CMD : SM_SOUND_CMD )
		: va( "Tip: bind a key to %s", sm.open == SM_MUSIC ? SM_MUSIC_CMD : SM_SOUND_CMD ) );
	SM_Text( SM_FOLDER_X, 452, buf, smWhite );

	if ( popup && sm.playerPicker ) {
		sm.click = click;
		sm.wheel = wheel;
		SM_DrawPlayerPicker();
	}

	// a click on nothing leaves the text box
	if ( sm.click )
		sm.focus = SM_F_NONE;
	sm.click = qfalse;
	sm.wheel = 0;

	WG_DrawOverlay();
	re->DrawStretchPic( sm.cursorX, sm.cursorY, 32, 32, 0, 0, 1, 1, cls.cursorShader );
}
