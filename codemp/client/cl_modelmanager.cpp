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

// cl_modelmanager.cpp -- engine-side front end for RPMod's /rpmodel command
//
// Browser: every models/**/*.md3 the filesystem can see but the player models, under
// Base (the game's own pk3s) and Custom (by the pk3 they come from), and the
// constructs saved before.
// Free camera: the map's rpmodels, read from the server's "rpmodel list", and new
// ones from the browser or a saved construct. Select one or more, carry, turn and
// scale them together, copy or delete them, save them as a construct, and send it
// all as "rpmodel remove" and "rpmodel add", as each change is made or all at once.
//
// RPMod can't move a model, so a moved one is removed and added again. Its list has
// no angles, scale or solidity; they are read off the model's entity in the snapshot,
// so one too far from the player to be in it can only be deleted.
//
// This lives in the engine rather than cgame so it works whatever cgame the
// server's mod ships (RPMod loads its own). The cgame keeps drawing the world;
// its main view is re-aimed from the free camera as it is handed to the renderer.

#include "client.h"
#include "qcommon/cm_public.h"
#include "cl_gizmo.h"
#include "cl_widgets.h"

#define MM_ROOT				"models"			// where models are listed from, and what RPMod's names are relative to
#define MM_SKIP				"models/players/"	// the player models are no use here
#define MM_EXT				".md3"
#define MM_TOGGLE_CMD		"modelmanager"
#define MM_OLD_TOGGLE_CMD	"modelplacer"		// what it was called, still in people's binds
#define MM_SAVE_DIR			"modelmanager"		// saved constructs, in the game folder
#define MM_SAVE_EXT			".cfg"
#define MM_STATE_DIR		MM_SAVE_DIR "/states"	// saved states, beside the constructs
#define MM_BASE_PAKS		BASEGAME "/assets"	// base/assets0.pk3 to assets3.pk3 are the game's own

#define MM_MAX_MODELS		16384
#define MM_MAX_ROWS			4096
#define MM_MAX_SOURCES		1024
#define MM_NAME_POOL		( MM_MAX_MODELS * 48 )
#define MM_HASH_SIZE		( MM_MAX_MODELS * 2 )
#define MM_MAX_PIECES		1024
#define MM_MAX_QUEUE		2048
#define MM_CMD_LEN			256
#define MM_MAX_SAVES		512
#define MM_MAX_SAVE_PIECES	256
#define MM_NAME_LEN			48

#define MM_LIST_TIMEOUT		3000	// ms without a list line before taking the list as empty
#define MM_LIST_LINE_GAP	1500	// ms between list lines before the next line starts a new list
#define MM_REPLY_MS			3000	// prints this soon after a command are shown as its reply
#define MM_MESSAGE_MS		4000
#define MM_MATCH_DIST		2.0f	// a model this close to where the list puts one is that one
#define MM_STATE_MATCH_DIST	4.0f	// and one this close to where a state had it, after rpdump load
#define MM_TRACE_MASK		(CONTENTS_SOLID|CONTENTS_TERRAIN)
#define MM_TRACE_DIST		8192.0f
#define MM_NO_HIT_DIST		256.0f
#define MM_HOLD_MIN			32.0f	// what's in hand in front of the camera: this near at least
#define MM_HOLD_MAX			2048.0f	// and this far at most
#define MM_HOLD_STEP		16.0f	// nearer or farther by this a wheel notch
#define MM_FLY_SPEED		400.0f
#define MM_SCALE_MIN		1
#define MM_SCALE_MAX		1000
#define MM_DOUBLECLICK_MS	400

// virtual 640x480 layout
#define MM_ROW_H			13.0f
#define MM_TEXT_SCALE		0.8f
#define MM_SEARCH_Y			44.0f
#define MM_LIST_Y			66.0f
#define MM_LIST_H			364.0f
#define MM_FOLDER_X			16.0f
#define MM_FOLDER_W			170.0f
#define MM_MODEL_X			192.0f
#define MM_MODEL_W			190.0f
#define MM_PREVIEW_X		388.0f
#define MM_PREVIEW_Y		66.0f
#define MM_PREVIEW_W		236.0f
#define MM_PREVIEW_H		236.0f
#define MM_BUTTON_Y			384.0f
#define MM_BUTTON_H			20.0f
#define MM_FILTER_W			52.0f	// the All / Base / Custom buttons, right of the search box
#define MM_FILTER_GAP		4.0f
#define MM_SEARCH_W			( SCREEN_WIDTH - MM_FOLDER_X * 2 - MM_FILTER_COUNT * ( MM_FILTER_W + MM_FILTER_GAP ) )
#define MM_VISIBLE_ROWS		((int)(MM_LIST_H / MM_ROW_H))
#define MM_STAR_W			11.0f	// the favorite star a model row starts with
#define MM_FAV_LIST			"models"

typedef enum {
	MM_OFF,
	MM_BROWSE,
	MM_CAMERA
} mmState_t;

typedef struct mmModel_s {
	const char	*path;		// relative to MM_ROOT, no extension, e.g. "map_objects/bespin/panels"
	const char	*base;		// file name part of path
	int			source;		// index into sources
	qboolean	custom;		// not from the game's own pk3s
	qboolean	listed;		// in the browser; lods and models only the server named aren't
	qboolean	checked;	// the files were looked over for things the renderer can't take
	qboolean	failed;		// the renderer couldn't load it, or wouldn't survive trying
	const char	*error;		// why, shown in the preview
} mmModel_t;

typedef enum {
	MM_FILTER_ALL,
	MM_FILTER_BASE,
	MM_FILTER_CUSTOM,
	MM_FILTER_COUNT
} mmFilter_t;

static const char *mmFilterNames[MM_FILTER_COUNT] = { "All", "Base", "Custom" };

typedef enum {
	MM_NAMING_NONE,
	MM_NAMING_CONSTRUCT,	// saving the selection as a construct
	MM_NAMING_TAG,			// naming the models being sent
	MM_NAMING_STATE			// saving the state
} mmNaming_t;

typedef enum {
	MM_ROW_SAVED,
	MM_ROW_STATES,
	MM_ROW_FAVES,		// the starred models
	MM_ROW_CATEGORY,	// Base or Custom
	MM_ROW_SOURCE,		// a custom pk3
	MM_ROW_FOLDER
} mmRowType_t;

typedef struct mmRow_s {
	mmRowType_t	type;
	char		name[MAX_QPATH];	// a folder's path relative to MM_ROOT
	int			first, count;		// range in the sorted model list
} mmRow_t;

// a model in the free camera: one of the server's, or a new one
typedef struct mmPiece_s {
	char		name[MAX_QPATH];	// relative to MM_ROOT, no extension, as rpmodel takes it
	int			model;				// index into models
	vec3_t		origin, angles;
	int			scale;				// percent
	qboolean	solid;
	char		tag[MM_NAME_LEN];	// the name given with Shift+Enter, shared by a construct's models
	qboolean	placed;				// added by this manager, so it goes in the state

	// on the server
	int			num;				// its rpmodel number, -1 = new
	qboolean	known;				// its angles, scale and solidity were seen in a snapshot
	vec3_t		srvOrigin, srvAngles;
	int			srvScale;
	qboolean	srvSolid;

	// editing
	qboolean	selected;
	qboolean	tmpl;				// carried, a new copy goes down with each click
	qboolean	deleted;
	qboolean	sent;				// its commands are on their way; the next list replaces it
	int			sentSeq;			// the list asked for after them
	qboolean	mark;				// to be sent
	vec3_t		carryOrigin, carryAngles;	// where it was picked up
	int			carryScale;
	qboolean	carrySolid;
	qhandle_t	hModel;				// this frame's
} mmPiece_t;

// a piece of a saved construct, relative to the construct
typedef struct mmSavedPiece_s {
	char		name[MAX_QPATH];
	vec3_t		offset, angles;
	int			scale;
	qboolean	solid;
} mmSavedPiece_t;

// a model of a saved state, where the server had it
typedef struct mmStateModel_s {
	char		tag[MM_NAME_LEN];
	char		name[MAX_QPATH];
	vec3_t		origin, angles;
	int			scale;
	qboolean	solid;
	qboolean	known;		// the angles were seen, not guessed
} mmStateModel_t;

typedef struct mmSnap_s {
	int			angle;		// degrees per rotation step
	int			grid;		// units per nudge / follow snap
} mmSnap_t;

static const mmSnap_t mmSnaps[] = {
	{ 1, 1 },
	{ 5, 4 },
	{ 15, 8 },
	{ 45, 16 },
	{ 90, 32 },
};

static struct {
	mmState_t	state;
	int			font;

	// index, rebuilt the first time the manager opens after cgame (re)starts
	qboolean	indexed;
	int			numModels, numListed;
	mmModel_t	models[MM_MAX_MODELS];
	int			numSources;
	const char	*sources[MM_MAX_SOURCES];	// "gamedir/name.pk3", or "gamedir" for loose files
	int			numRows;
	mmRow_t		rows[MM_MAX_ROWS];
	char		pool[MM_NAME_POOL];
	int			poolUsed;
	int			hash[MM_HASH_SIZE];	// model index + 1, 0 = empty

	// saved constructs
	int			numSaves;
	char		saves[MM_MAX_SAVES][MAX_QPATH];
	char		previewSave[MAX_QPATH];		// the one read into savePieces
	int			numSavePieces;
	mmSavedPiece_t savePieces[MM_MAX_SAVE_PIECES];

	// saved states
	int			numStates;
	char		states[MM_MAX_SAVES][MAX_QPATH];
	char		previewState[MAX_QPATH];	// the one summed up in stateSummary
	char		stateSummary[MAX_STRING_CHARS];

	// a state being reloaded, waiting for the list it asked for
	qboolean	reloading;
	int			reloadSeq;
	char		reloadName[MAX_QPATH];
	int			numStateModels;
	mmStateModel_t stateModels[MM_MAX_PIECES];

	// browser
	char		search[64];
	mmFilter_t	filter;					// which models the folders and the search show
	int			row;
	qboolean	viewSaves;				// the list shows saved constructs
	qboolean	viewStates;				// or saved states
	int			view[MM_MAX_MODELS];	// model or save indices currently listed
	int			numView;
	int			selRow;
	int			modelScroll, rowScroll;
	float		cursorX, cursorY;
	int			lastClickTime, lastClickRow;

	// the free camera's models, kept through a vid_restart for their names
	int			numPieces;
	mmPiece_t	pieces[MM_MAX_PIECES];
	char		piecesMap[MAX_STRING_CHARS];	// the server and map they're on
	int			hover;			// piece under the crosshair, -1 = none
	qboolean	hideBoxes;		// no outlines, but on the one peeked at
	int			peek;			// piece under the crosshair while the peek key is held, -1 = none
	qboolean	carrying;		// the selection is in hand
	qboolean	locked;			// in hand, but not following the crosshair
	qboolean	instant;		// each change is sent as it is made, else on Enter
	vec3_t		constructAnchor;	// where a construct being exec'd goes
	vec3_t		defAngles;		// for the next new model
	int			defScale;
	qboolean	defSolid;
	int			snap;
	qboolean	hideHelp;
	mmNaming_t	naming;			// typing a name, and what for
	qboolean	barClick;		// a click on a list's scrollbar, for the frame to take
	float		barClickX, barClickY;
	char		saveName[MM_NAME_LEN];
	char		tmplName[MM_NAME_LEN];	// what the model or construct in hand is called, to offer as its name
	char		message[MAX_STRING_CHARS];
	int			messageTime;

	// camera
	vec3_t		camOrg, camAng;
	qboolean	held[MAX_KEYS];
	int			lastFrameTime;

	// commands, one at a time as the server's flood protection lets them through
	int			numQueue, queueTotal;
	char		queue[MM_MAX_QUEUE][MM_CMD_LEN];
	int			queueSeq[MM_MAX_QUEUE];	// > 0: a list request, numbered
	int			nextSend;
	char		lastCmd[MM_CMD_LEN];
	int			sentTime;
	char		reply[MAX_STRING_CHARS];
	int			replyTime;

	// reading the list
	int			listSeq;			// the last list asked for
	int			listInFlight;		// the last one sent
	int			listRequestTime;	// waiting for its first line, 0 = not waiting
	int			listLineTime;		// last list line read
	int			listReading;		// the request the list being read answers, 0 = someone typed it
	qboolean	listOurs;
	int			listBegan;			// the request the last list read answered
	int			listBeginTime;
	int			numOld;
	mmPiece_t	old[MM_MAX_PIECES];	// the pieces from before the list being read

	// the cgame's last main view, where the free camera starts
	qboolean	haveView;
	vec3_t		viewOrg, viewAng;
	int			viewFrame;		// cls.framecount the main view was last re-aimed on
	float		fovX, fovY;		// the free camera's

	// the move arrows: the mouse a cursor, an arrow dragged moves the selection along it
	qboolean	gizmo;
	qboolean	gzDragging;
	gzDrag_t	gzDrag;
	int			gzHot;
} mm;

static vec4_t mmWhite		= { 1.0f, 1.0f, 1.0f, 1.0f };
static vec4_t mmRed			= { 1.0f, 0.3f, 0.3f, 1.0f };
static vec4_t mmPanel		= { 0.0f, 0.0f, 0.0f, 0.65f };
static vec4_t mmPanelOpaque	= { 0.05f, 0.05f, 0.05f, 0.95f };	// over the help, which would show through
static vec4_t mmPanelLight	= { 0.15f, 0.15f, 0.15f, 0.8f };
static vec4_t mmHighlight	= { 0.2f, 0.45f, 0.8f, 0.8f };
static vec4_t mmHover		= { 1.0f, 1.0f, 1.0f, 0.12f };
static vec4_t mmButtonHover	= { 0.3f, 0.3f, 0.3f, 0.85f };
static vec4_t mmBorder		= { 0.5f, 0.5f, 0.5f, 0.8f };
static vec4_t mmDim			= { 0.7f, 0.7f, 0.7f, 1.0f };
static vec4_t mmAccent		= { 1.0f, 0.8f, 0.3f, 1.0f };

static const byte mmBoxSent[4]		= { 255, 255, 255, 255 };
static const byte mmBoxSolid[4]		= { 64, 255, 96, 255 };
static const byte mmBoxNonSolid[4]	= { 255, 200, 64, 255 };
static const byte mmBoxSelected[4]	= { 64, 220, 255, 255 };
static const byte mmBoxHover[4]		= { 255, 255, 64, 255 };
static const byte mmBoxMoved[4]		= { 255, 96, 255, 255 };
static const byte mmBoxDeleted[4]	= { 255, 64, 64, 255 };
static const byte mmBoxUnknown[4]	= { 140, 140, 140, 255 };

static void MM_RequestList( void );

/*
===============================================================================

INDEX

===============================================================================
*/

static int MM_HashName( const char *s ) {
	unsigned int h = 5381;

	while ( *s ) {
		h = h * 33 + (unsigned int)tolower( (unsigned char)*s );
		s++;
	}
	return (int)( h % MM_HASH_SIZE );
}

static const char *MM_PoolString( const char *s ) {
	int len = strlen( s ) + 1;
	char *out;

	if ( mm.poolUsed + len > MM_NAME_POOL )
		return NULL;
	out = mm.pool + mm.poolUsed;
	memcpy( out, s, len );
	mm.poolUsed += len;
	return out;
}

// "base/assets<digits>.pk3"; anything else people named assets_something isn't
static qboolean MM_IsBaseSource( const char *source ) {
	const int prefixLen = strlen( MM_BASE_PAKS );
	const char *c = source + prefixLen;

	if ( Q_stricmpn( source, MM_BASE_PAKS, prefixLen ) || !isdigit( (unsigned char)*c ) )
		return qfalse;
	while ( isdigit( (unsigned char)*c ) )
		c++;
	return (qboolean)!Q_stricmp( c, ".pk3" );
}

static int MM_SourceIndex( const char *source ) {
	const char *stored;

	// the files of one pk3 come together
	for ( int i = mm.numSources - 1; i >= 0; i-- ) {
		if ( !Q_stricmp( mm.sources[i], source ) )
			return i;
	}
	if ( mm.numSources >= MM_MAX_SOURCES || !( stored = MM_PoolString( source ) ) )
		return 0;
	mm.sources[mm.numSources] = stored;
	return mm.numSources++;
}

static int MM_FindModel( const char *path ) {
	int h = MM_HashName( path );

	while ( mm.hash[h] ) {
		if ( !Q_stricmp( mm.models[mm.hash[h] - 1].path, path ) )
			return mm.hash[h] - 1;
		h = ( h + 1 ) % MM_HASH_SIZE;
	}
	return -1;
}

static void MM_HashModel( int index ) {
	int h = MM_HashName( mm.models[index].path );

	while ( mm.hash[h] )
		h = ( h + 1 ) % MM_HASH_SIZE;
	mm.hash[h] = index + 1;
}

// path relative to MM_ROOT without extension; -1 if there's no room
static int MM_InsertModel( const char *path, int source, qboolean listed ) {
	const char *stored, *slash;
	mmModel_t *m;
	int index = MM_FindModel( path );

	// the same file can be in several pk3s and folders; the first is the one in use
	if ( index >= 0 )
		return index;
	if ( mm.numModels >= MM_MAX_MODELS || !( stored = MM_PoolString( path ) ) )
		return -1;

	slash = strrchr( stored, '/' );
	m = &mm.models[mm.numModels];
	m->path = stored;
	m->base = slash ? slash + 1 : stored;
	m->source = source;
	m->custom = (qboolean)!MM_IsBaseSource( mm.sources[source] );
	m->listed = listed;
	m->checked = qfalse;
	m->failed = qfalse;
	m->error = NULL;
	MM_HashModel( mm.numModels );
	return mm.numModels++;
}

// a model the server named; not in the browser if the files don't have it
static int MM_FindOrAddModel( const char *path ) {
	int index = MM_FindModel( path );

	if ( index < 0 )
		index = MM_InsertModel( path, 0, qfalse );
	return index;
}

// name is a full game path, e.g. "models/map_objects/bespin/panels.md3"
static void MM_AddFile( const char *name, const char *source, void *ctx ) {
	char path[MAX_QPATH];
	const int rootLen = strlen( MM_ROOT ), extLen = strlen( MM_EXT );
	int len = strlen( name );

	if ( len <= rootLen + 1 + extLen || Q_stricmpn( name, MM_ROOT "/", rootLen + 1 ) || !Q_stricmpn( name, MM_SKIP, strlen( MM_SKIP ) ) )
		return;
	Q_strncpyz( path, name + rootLen + 1, sizeof( path ) );
	path[strlen( path ) - extLen] = '\0';
	for ( char *c = path; *c; c++ ) {
		if ( *c == '\\' )
			*c = '/';
	}
	MM_InsertModel( path, MM_SourceIndex( source ), qtrue );
}

static int MM_FolderLen( const mmModel_t *m ) {
	return (int)( m->base - m->path ) - ( m->base != m->path ? 1 : 0 );
}

// map_objects first, it's what most of them are
static int MM_Group( const mmModel_t *m ) {
	return Q_stricmpn( m->path, "map_objects/", 12 ) ? 1 : 0;
}

static int QDECL MM_CompareModels( const void *a, const void *b ) {
	const mmModel_t *ma = (const mmModel_t *)a, *mb = (const mmModel_t *)b;
	int la, lb, cmp;

	if ( ma->listed != mb->listed )
		return ma->listed ? -1 : 1;
	if ( ma->custom != mb->custom )
		return ma->custom ? 1 : -1;
	if ( ma->custom && ma->source != mb->source && ( cmp = Q_stricmp( mm.sources[ma->source], mm.sources[mb->source] ) ) )
		return cmp;
	if ( MM_Group( ma ) != MM_Group( mb ) )
		return MM_Group( ma ) - MM_Group( mb );

	// then by folder, so every folder is one contiguous range
	la = MM_FolderLen( ma );
	lb = MM_FolderLen( mb );
	cmp = Q_stricmpn( ma->path, mb->path, la < lb ? la : lb );
	if ( cmp )
		return cmp;
	if ( la != lb )
		return la - lb;
	return Q_stricmp( ma->base, mb->base );
}

static int MM_AddRow( mmRowType_t type, const char *name, int first ) {
	mmRow_t *r;

	if ( mm.numRows >= MM_MAX_ROWS )
		return -1;
	r = &mm.rows[mm.numRows];
	r->type = type;
	Q_strncpyz( r->name, name, sizeof( r->name ) );
	r->first = first;
	r->count = 0;
	return mm.numRows++;
}

static qboolean MM_PassesFilter( const mmModel_t *m ) {
	return (qboolean)( mm.filter == MM_FILTER_ALL || ( mm.filter == MM_FILTER_CUSTOM ) == ( m->custom != qfalse ) );
}

// the saves first, then the folders of the models the filter lets through
static void MM_BuildRows( void ) {
	int category = -1, source = -1, catRow = -1, srcRow = -1, folderRow = -1;

	mm.numRows = 0;
	MM_AddRow( MM_ROW_SAVED, "Saved constructs", 0 );
	mm.rows[0].count = mm.numSaves;
	MM_AddRow( MM_ROW_STATES, "Saved states", 0 );
	mm.rows[1].count = mm.numStates;
	MM_AddRow( MM_ROW_FAVES, "Favorites", 0 );
	for ( int i = 0; i < mm.numListed; i++ ) {
		if ( MM_PassesFilter( &mm.models[i] ) && CL_Fav_Has( MM_FAV_LIST, mm.models[i].path ) )
			mm.rows[2].count++;
	}

	for ( int i = 0; i < mm.numListed; i++ ) {
		const mmModel_t *m = &mm.models[i];
		int len = MM_FolderLen( m );

		// base and custom are each one block, so a range stays whole
		if ( !MM_PassesFilter( m ) )
			continue;
		if ( m->custom != category ) {
			category = m->custom;
			catRow = MM_AddRow( MM_ROW_CATEGORY, m->custom ? "Custom" : "Base", i );
			source = -1;
			folderRow = -1;
		}
		if ( m->custom && m->source != source ) {
			source = m->source;
			srcRow = MM_AddRow( MM_ROW_SOURCE, mm.sources[source], i );
			folderRow = -1;
		}
		if ( folderRow < 0 || (int)strlen( mm.rows[folderRow].name ) != len || Q_stricmpn( mm.rows[folderRow].name, m->path, len ) ) {
			char folder[MAX_QPATH];

			Q_strncpyz( folder, m->path, len + 1 < (int)sizeof( folder ) ? len + 1 : (int)sizeof( folder ) );
			folderRow = MM_AddRow( MM_ROW_FOLDER, folder, i );
		}
		if ( catRow < 0 || folderRow < 0 )
			break;	// out of rows
		mm.rows[catRow].count++;
		if ( m->custom && srcRow >= 0 )
			mm.rows[srcRow].count++;
		mm.rows[folderRow].count++;
	}
}

static void MM_FirstFolder( void ) {
	mm.row = 0;
	for ( int i = 0; i < mm.numRows; i++ ) {
		if ( mm.rows[i].type == MM_ROW_FOLDER ) {
			mm.row = i;
			break;
		}
	}
	mm.rowScroll = mm.selRow = mm.modelScroll = 0;
}

static void MM_BuildIndex( void ) {
	int start = Sys_Milliseconds(), numCustom = 0;

	mm.numModels = mm.numListed = 0;
	mm.numSources = 0;
	mm.poolUsed = 0;
	memset( mm.hash, 0, sizeof( mm.hash ) );
	MM_SourceIndex( "the server" );	// 0, for models only the server knows

	// the pk3 directories are already in memory, only loose folders touch the disk
	FS_ListFilesRecursiveFrom( MM_ROOT, MM_EXT, MM_AddFile, NULL );

	// a model's lods sit next to it as name_1 and name_2
	for ( int i = 0; i < mm.numModels; i++ ) {
		mmModel_t *m = &mm.models[i];
		int len = strlen( m->path );

		if ( len > 2 && m->path[len - 2] == '_' && ( m->path[len - 1] == '1' || m->path[len - 1] == '2' ) ) {
			char lod0[MAX_QPATH];

			Q_strncpyz( lod0, m->path, len - 1 );
			if ( MM_FindModel( lod0 ) >= 0 )
				m->listed = qfalse;
		}
	}

	qsort( mm.models, mm.numModels, sizeof( mm.models[0] ), MM_CompareModels );
	memset( mm.hash, 0, sizeof( mm.hash ) );
	for ( int i = 0; i < mm.numModels; i++ ) {
		MM_HashModel( i );
		if ( mm.models[i].listed ) {
			mm.numListed++;
			if ( mm.models[i].custom )
				numCustom++;
		}
	}
	MM_BuildRows();

	// what the free camera holds points into the old list
	for ( int i = 0; i < mm.numPieces; i++ )
		mm.pieces[i].model = MM_FindOrAddModel( mm.pieces[i].name );

	mm.indexed = qtrue;
	MM_FirstFolder();
	Com_Printf( "Model manager: indexed %i models, %i of them custom, in %i folders (%i ms)\n",
		mm.numListed, numCustom, mm.numRows, Sys_Milliseconds() - start );
}

static int QDECL MM_CompareSaves( const void *a, const void *b ) {
	return Q_stricmp( (const char *)a, (const char *)b );
}

// the .cfg files in dir, without the extension
static int MM_ListSaves( const char *dir, char (*out)[MAX_QPATH] ) {
	char **list;
	int count, num = 0;

	list = FS_ListFiles( dir, MM_SAVE_EXT, &count );
	for ( int i = 0; i < count && num < MM_MAX_SAVES; i++ ) {
		char *name = out[num];

		Q_strncpyz( name, list[i], MAX_QPATH );
		if ( strlen( name ) > strlen( MM_SAVE_EXT ) )
			name[strlen( name ) - strlen( MM_SAVE_EXT )] = '\0';
		num++;
	}
	FS_FreeFileList( list );
	qsort( out, num, MAX_QPATH, MM_CompareSaves );
	return num;
}

static void MM_RefreshSaves( void ) {
	mm.numSaves = MM_ListSaves( MM_SAVE_DIR, mm.saves );
	mm.numStates = MM_ListSaves( MM_STATE_DIR, mm.states );
	if ( mm.numRows > 1 ) {
		mm.rows[0].count = mm.numSaves;
		mm.rows[1].count = mm.numStates;
	}
	mm.previewSave[0] = '\0';
	mm.previewState[0] = '\0';
}

/*
===============================================================================

HELPERS

===============================================================================
*/

static qboolean MM_ContainsNoCase( const char *haystack, const char *needle ) {
	int nlen = strlen( needle );

	if ( !nlen )
		return qtrue;
	for ( ; *haystack; haystack++ ) {
		if ( !Q_stricmpn( haystack, needle, nlen ) )
			return qtrue;
	}
	return qfalse;
}

static void MM_Message( const char *text ) {
	Q_strncpyz( mm.message, text, sizeof( mm.message ) );
	mm.messageTime = cls.realtime;
}

static const char *MM_FolderLabel( const char *folder ) {
	if ( !folder[0] )
		return "(models)";
	if ( !Q_stricmpn( folder, "map_objects/", 12 ) )
		return folder + 12;
	return folder;
}

static void MM_RebuildView( void ) {
	mm.numView = 0;
	mm.viewSaves = mm.viewStates = qfalse;
	if ( mm.search[0] ) {
		for ( int i = 0; i < mm.numListed; i++ ) {
			if ( !MM_PassesFilter( &mm.models[i] ) )
				continue;
			if ( MM_ContainsNoCase( mm.models[i].path, mm.search ) || MM_ContainsNoCase( mm.sources[mm.models[i].source], mm.search ) )
				mm.view[mm.numView++] = i;
		}
	} else if ( mm.row >= 0 && mm.row < mm.numRows ) {
		const mmRow_t *r = &mm.rows[mm.row];

		if ( r->type == MM_ROW_SAVED ) {
			mm.viewSaves = qtrue;
			for ( int i = 0; i < mm.numSaves; i++ )
				mm.view[mm.numView++] = i;
		} else if ( r->type == MM_ROW_STATES ) {
			mm.viewStates = qtrue;
			for ( int i = 0; i < mm.numStates; i++ )
				mm.view[mm.numView++] = i;
		} else if ( r->type == MM_ROW_FAVES ) {
			for ( int i = 0; i < mm.numListed && mm.numView < MM_MAX_MODELS; i++ ) {
				if ( MM_PassesFilter( &mm.models[i] ) && CL_Fav_Has( MM_FAV_LIST, mm.models[i].path ) )
					mm.view[mm.numView++] = i;
			}
		} else {
			for ( int i = 0; i < r->count; i++ )
				mm.view[mm.numView++] = r->first + i;
		}
	}

	mm.selRow = Com_Clampi( 0, mm.numView ? mm.numView - 1 : 0, mm.selRow );
	mm.modelScroll = Com_Clampi( 0, mm.numView > MM_VISIBLE_ROWS ? mm.numView - MM_VISIBLE_ROWS : 0, mm.modelScroll );
}

static void MM_ScrollToSelection( void ) {
	if ( mm.selRow < mm.modelScroll )
		mm.modelScroll = mm.selRow;
	else if ( mm.selRow >= mm.modelScroll + MM_VISIBLE_ROWS )
		mm.modelScroll = mm.selRow - MM_VISIBLE_ROWS + 1;
}

static void MM_SetRow( int row ) {
	if ( !mm.numRows )
		return;
	mm.row = Com_Clampi( 0, mm.numRows - 1, row );
	mm.search[0] = '\0';
	mm.selRow = 0;
	mm.modelScroll = 0;
	if ( mm.row < mm.rowScroll )
		mm.rowScroll = mm.row;
	else if ( mm.row >= mm.rowScroll + MM_VISIBLE_ROWS )
		mm.rowScroll = mm.row - MM_VISIBLE_ROWS + 1;
	MM_RebuildView();
}

static void MM_SetFilter( mmFilter_t filter ) {
	const qboolean onSaves = (qboolean)( mm.row < mm.numRows && ( mm.rows[mm.row].type == MM_ROW_SAVED || mm.rows[mm.row].type == MM_ROW_STATES
		|| mm.rows[mm.row].type == MM_ROW_FAVES ) );

	if ( filter == mm.filter )
		return;
	mm.filter = filter;
	MM_BuildRows();
	// the saves rows stay where they are, a folder may be gone
	if ( onSaves )
		mm.selRow = mm.modelScroll = 0;
	else
		MM_FirstFolder();
	MM_RebuildView();
}

// the model, or with viewSaves or viewStates the save, on the selected row; -1 = none
static int MM_Selected( void ) {
	if ( mm.selRow < 0 || mm.selRow >= mm.numView )
		return -1;
	return mm.view[mm.selRow];
}

// the renderer drops the whole game (ERR_DROP) on a surface it can't draw instead of
// failing the load, so look the file over first. NULL if it's fine or not there
static const char *MM_CheckMD3( const char *name ) {
	const char *error = NULL;
	const md3Header_t *header;
	byte *buf;
	int len, ofs;

	len = FS_ReadFile( name, (void **)&buf );
	if ( len < 0 || !buf )
		return NULL;

	header = (const md3Header_t *)buf;
	if ( len < (int)sizeof( md3Header_t ) || LittleLong( header->ident ) != MD3_IDENT ) {
		FS_FreeFile( buf );
		return NULL;	// not an md3 at all, let the renderer turn it down
	}

	ofs = LittleLong( header->ofsSurfaces );
	for ( int i = 0; i < LittleLong( header->numSurfaces ) && !error; i++ ) {
		const md3Surface_t *surf = (const md3Surface_t *)( buf + ofs );
		int numVerts, numTriangles;

		if ( ofs < 0 || ofs > len - (int)sizeof( md3Surface_t ) ) {
			error = "Damaged model file";
			break;
		}
		numVerts = LittleLong( surf->numVerts );
		numTriangles = LittleLong( surf->numTriangles );

		// the same tests R_LoadMD3 makes
		if ( numVerts >= SHADER_MAX_VERTEXES ) {
			Com_Printf( S_COLOR_YELLOW "Model manager: %s has more than %i verts on %.*s (%i)\n",
				name, SHADER_MAX_VERTEXES - 1, MAX_QPATH, surf->name[0] ? surf->name : "a surface", numVerts );
			error = "Too many verts on one surface";
		} else if ( numTriangles * 3 >= SHADER_MAX_INDEXES ) {
			Com_Printf( S_COLOR_YELLOW "Model manager: %s has more than %i triangles on %.*s (%i)\n",
				name, SHADER_MAX_INDEXES / 3 - 1, MAX_QPATH, surf->name[0] ? surf->name : "a surface", numTriangles );
			error = "Too many triangles on one surface";
		} else if ( LittleLong( surf->ofsEnd ) <= 0 ) {
			error = "Damaged model file";
		}
		ofs += LittleLong( surf->ofsEnd );
	}

	FS_FreeFile( buf );
	return error;
}

// done once per model, as it first shows up
static void MM_CheckModel( mmModel_t *m ) {
	if ( m->checked )
		return;
	m->checked = qtrue;

	// the renderer also loads the _1 and _2 lods next to it
	for ( int lod = 0; lod < MD3_MAX_LODS && !m->error; lod++ )
		m->error = MM_CheckMD3( lod ? va( "%s/%s_%i%s", MM_ROOT, m->path, lod, MM_EXT ) : va( "%s/%s%s", MM_ROOT, m->path, MM_EXT ) );
	if ( m->error )
		m->failed = qtrue;
}

// the renderer caches models by name and drops them on map change or vid_restart,
// so look the handle up whenever it is needed instead of keeping it
static qhandle_t MM_RegisterModel( int index ) {
	mmModel_t *m;
	qhandle_t h;

	if ( index < 0 || index >= mm.numModels )
		return 0;
	m = &mm.models[index];
	MM_CheckModel( m );
	if ( m->failed )
		return 0;

	h = re->RegisterModel( va( "%s/%s%s", MM_ROOT, m->path, MM_EXT ) );
	if ( !h )
		m->failed = qtrue;
	return h;
}

static int MM_NormalizeAngle( float a ) {
	int i = (int)floorf( a + 0.5f ) % 360;
	return i < 0 ? i + 360 : i;
}

static qboolean MM_SameAngle( float a, float b ) {
	const int d = abs( MM_NormalizeAngle( a ) - MM_NormalizeAngle( b ) );

	return (qboolean)( d <= 1 || d >= 359 );
}

static float MM_SnapTo( float v, int grid ) {
	return grid > 1 ? floorf( v / grid + 0.5f ) * grid : floorf( v + 0.5f );
}

static int MM_Round( float v ) {
	return (int)floorf( v + 0.5f );
}

static void MM_BuildAxis( const vec3_t angles, int scale, matrix3_t axis ) {
	AnglesToAxis( angles, axis );
	if ( scale != 100 ) {
		float s = scale / 100.0f;

		VectorScale( axis[0], s, axis[0] );
		VectorScale( axis[1], s, axis[1] );
		VectorScale( axis[2], s, axis[2] );
	}
}

// corner i of the bounds, through axis, relative to the model origin
static void MM_Corner( const vec3_t mins, const vec3_t maxs, matrix3_t axis, int i, vec3_t out ) {
	vec3_t local;

	local[0] = ( i & 1 ) ? maxs[0] : mins[0];
	local[1] = ( i & 2 ) ? maxs[1] : mins[1];
	local[2] = ( i & 4 ) ? maxs[2] : mins[2];
	for ( int j = 0; j < 3; j++ )
		out[j] = local[0] * axis[0][j] + local[1] * axis[1][j] + local[2] * axis[2][j];
}

static void MM_ModelBounds( qhandle_t h, vec3_t mins, vec3_t maxs ) {
	if ( h ) {
		re->ModelBounds( h, mins, maxs );
		if ( mins[0] <= maxs[0] )
			return;
	}
	VectorSet( mins, -8, -8, -8 );
	VectorSet( maxs, 8, 8, 8 );
}

// the inverse of AnglesToAxis; axis[1] points left
static void MM_AxisToAngles( matrix3_t axis, vec3_t angles ) {
	float sp = Com_Clamp( -1.0f, 1.0f, -axis[0][2] );

	angles[PITCH] = RAD2DEG( asinf( sp ) );
	if ( fabsf( sp ) > 0.9999f ) {
		// straight up or down, yaw and roll turn about the same axis: it all goes in yaw
		angles[YAW] = RAD2DEG( atan2f( -axis[1][0], axis[1][1] ) );
		angles[ROLL] = 0;
	} else {
		angles[YAW] = RAD2DEG( atan2f( axis[0][1], axis[0][0] ) );
		angles[ROLL] = RAD2DEG( atan2f( axis[1][2], axis[2][2] ) );
	}
}

static qboolean MM_ShiftDown( void ) {
	return (qboolean)( mm.held[A_SHIFT] || mm.held[A_SHIFT2] );
}

static qboolean MM_CtrlDown( void ) {
	return (qboolean)( mm.held[A_CTRL] || mm.held[A_CTRL2] );
}

// what's in hand follows the crosshair onto what it aims at, however far, or stays in front of
// the camera, as far as cl_modelHoldDistance or the first thing nearer
static cvar_t *cl_modelFollowFront, *cl_modelHoldDistance;

static float MM_HoldDistance( void ) {
	return Com_Clamp( MM_HOLD_MIN, MM_HOLD_MAX, cl_modelHoldDistance->value );
}

static void MM_ToggleFollowFront( void ) {
	Cvar_SetValue( "cl_modelFollowFront", cl_modelFollowFront->integer ? 0 : 1 );
	MM_Message( cl_modelFollowFront->integer ? va( "In hand, it stays %i units in front of you: Ctrl+Wheel nearer or farther", (int)MM_HoldDistance() )
		: "In hand, it goes onto whatever you aim at, however far" );
}

static void MM_ChangeHoldDistance( int dir ) {
	Cvar_SetValue( "cl_modelHoldDistance", Com_Clamp( MM_HOLD_MIN, MM_HOLD_MAX, MM_HoldDistance() + dir * MM_HOLD_STEP * ( MM_ShiftDown() ? 4 : 1 ) ) );
}

// splits a line into words, "quoted" ones whole; the line is cut up in place
static int MM_SplitLine( char *line, char **argv, int max ) {
	int argc = 0;

	while ( *line && argc < max ) {
		while ( *line == ' ' || *line == '\t' )
			line++;
		if ( !*line )
			break;
		if ( *line == '"' ) {
			argv[argc++] = ++line;
			while ( *line && *line != '"' )
				line++;
		} else {
			argv[argc++] = line;
			while ( *line && *line != ' ' && *line != '\t' )
				line++;
		}
		if ( *line )
			*line++ = '\0';
	}
	return argc;
}

/*
===============================================================================

PIECES

===============================================================================
*/

static qboolean MM_IsNew( const mmPiece_t *p ) {
	return (qboolean)( p->num < 0 );
}

// selected, and not on its way to the server
static qboolean MM_Sel( const mmPiece_t *p ) {
	return (qboolean)( p->selected && !p->sent );
}

// a server model that isn't where the server has it any more
static qboolean MM_Moved( const mmPiece_t *p ) {
	if ( MM_IsNew( p ) || !p->known )
		return qfalse;
	for ( int i = 0; i < 3; i++ ) {
		if ( fabsf( p->origin[i] - p->srvOrigin[i] ) > 0.5f || MM_NormalizeAngle( p->angles[i] ) != MM_NormalizeAngle( p->srvAngles[i] ) )
			return qtrue;
	}
	return (qboolean)( p->scale != p->srvScale || p->solid != p->srvSolid );
}

// a server model the cgame draws where it isn't any more; this draws it instead
static qboolean MM_HideOriginal( const mmPiece_t *p ) {
	return (qboolean)( !MM_IsNew( p ) && !p->deleted && ( MM_Moved( p ) || ( MM_Sel( p ) && mm.carrying ) ) );
}

// where it shows: a deleted one where the server has it
static void MM_Shown( const mmPiece_t *p, vec3_t origin, vec3_t angles, int *scale ) {
	if ( p->deleted ) {
		VectorCopy( p->srvOrigin, origin );
		VectorCopy( p->srvAngles, angles );
		*scale = p->srvScale;
	} else {
		VectorCopy( p->origin, origin );
		VectorCopy( p->angles, angles );
		*scale = p->scale;
	}
}

static int MM_NumSelected( void ) {
	int count = 0;

	for ( int i = 0; i < mm.numPieces; i++ ) {
		if ( MM_Sel( &mm.pieces[i] ) )
			count++;
	}
	return count;
}

static mmPiece_t *MM_FirstSelected( void ) {
	for ( int i = 0; i < mm.numPieces; i++ ) {
		if ( MM_Sel( &mm.pieces[i] ) )
			return &mm.pieces[i];
	}
	return NULL;
}

static qboolean MM_CarryingTemplate( void ) {
	const mmPiece_t *p = MM_FirstSelected();

	return (qboolean)( mm.carrying && p && p->tmpl );
}

static void MM_ClearSelection( void ) {
	for ( int i = 0; i < mm.numPieces; i++ )
		mm.pieces[i].selected = qfalse;
}

static mmPiece_t *MM_NewPiece( const char *name ) {
	mmPiece_t *p;

	if ( mm.numPieces >= MM_MAX_PIECES ) {
		MM_Message( S_COLOR_YELLOW "Too many models in the free camera" );
		return NULL;
	}
	p = &mm.pieces[mm.numPieces++];
	memset( p, 0, sizeof( *p ) );
	Q_strncpyz( p->name, name, sizeof( p->name ) );
	p->model = MM_FindOrAddModel( name );
	p->hModel = MM_RegisterModel( p->model );
	p->scale = 100;
	p->solid = qtrue;
	p->num = -1;
	return p;
}

// the pieces are for one map of one server; a vid_restart keeps them, and their names
static void MM_CheckMap( void ) {
	char key[MAX_STRING_CHARS];

	Com_sprintf( key, sizeof( key ), "%s %s", cls.servername,
		Info_ValueForKey( cl.gameState.stringData + cl.gameState.stringOffsets[CS_SERVERINFO], "mapname" ) );
	if ( !Q_stricmp( key, mm.piecesMap ) )
		return;
	Q_strncpyz( mm.piecesMap, key, sizeof( mm.piecesMap ) );
	mm.numPieces = 0;
	mm.hover = -1;
	mm.carrying = qfalse;
	mm.reloading = qfalse;
}

static int MM_CountTagged( const char *tag ) {
	int count = 0;

	for ( int i = 0; i < mm.numPieces; i++ ) {
		if ( !mm.pieces[i].tmpl && !Q_stricmp( mm.pieces[i].tag, tag ) )
			count++;
	}
	return count;
}

// drops the pieces with mark set
static void MM_RemoveMarked( void ) {
	int keep = 0;

	for ( int i = 0; i < mm.numPieces; i++ ) {
		if ( !mm.pieces[i].mark )
			mm.pieces[keep++] = mm.pieces[i];
	}
	mm.numPieces = keep;
	mm.hover = -1;
}

// the middle of the selected models' origins; what the selection turns about
static void MM_Pivot( vec3_t pivot ) {
	vec3_t mins, maxs;

	ClearBounds( mins, maxs );
	for ( int i = 0; i < mm.numPieces; i++ ) {
		if ( MM_Sel( &mm.pieces[i] ) )
			AddPointToBounds( mm.pieces[i].origin, mins, maxs );
	}
	if ( mins[0] > maxs[0] ) {
		VectorClear( pivot );
		return;
	}
	VectorAdd( mins, maxs, pivot );
	VectorScale( pivot, 0.5f, pivot );
}

static void MM_TranslateSelection( const vec3_t delta ) {
	for ( int i = 0; i < mm.numPieces; i++ ) {
		if ( MM_Sel( &mm.pieces[i] ) )
			VectorAdd( mm.pieces[i].origin, delta, mm.pieces[i].origin );
	}
}

// the model the browser places next is the one placed last
static void MM_KeepDefaults( void ) {
	const mmPiece_t *p = MM_FirstSelected();

	if ( !MM_CarryingTemplate() || MM_NumSelected() != 1 )
		return;
	VectorCopy( p->angles, mm.defAngles );
	mm.defScale = p->scale;
	mm.defSolid = p->solid;
}

// in front of the camera, where new things start before the crosshair takes them
static void MM_InFrontOfCamera( vec3_t out ) {
	vec3_t forward;

	AngleVectors( mm.camAng, forward, NULL, NULL );
	VectorMA( mm.camOrg, 128.0f, forward, out );
}

/*
===============================================================================

COMMANDS AND THE LIST

===============================================================================
*/

static int MM_CommandGap( void ) {
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

static void MM_Queue( const char *cmd, int seq ) {
	// opened from the storyteller, it goes in the story instead
	if ( !seq && CL_Storyteller_Capture( cmd ) )
		return;
	if ( mm.numQueue >= MM_MAX_QUEUE ) {
		Com_Printf( S_COLOR_YELLOW "Model manager: too many commands waiting, dropped %s\n", cmd );
		return;
	}
	Q_strncpyz( mm.queue[mm.numQueue], cmd, sizeof( mm.queue[0] ) );
	mm.queueSeq[mm.numQueue] = seq;
	mm.numQueue++;
	mm.queueTotal++;
}

static void MM_RunQueue( void ) {
	if ( !mm.numQueue ) {
		mm.queueTotal = 0;
		return;
	}
	if ( cls.realtime < mm.nextSend || cls.state != CA_ACTIVE )
		return;

	CL_AddReliableCommand( mm.queue[0], qfalse );
	if ( mm.queueSeq[0] ) {
		mm.listInFlight = mm.queueSeq[0];
		mm.listRequestTime = cls.realtime;
	} else {
		mm.sentTime = cls.realtime;
		Q_strncpyz( mm.lastCmd, mm.queue[0], sizeof( mm.lastCmd ) );
		Com_Printf( S_COLOR_GREEN "Model manager: " S_COLOR_WHITE "%s\n", mm.queue[0] );
	}
	mm.numQueue--;
	memmove( mm.queue[0], mm.queue[1], mm.numQueue * sizeof( mm.queue[0] ) );
	memmove( &mm.queueSeq[0], &mm.queueSeq[1], mm.numQueue * sizeof( mm.queueSeq[0] ) );
	mm.nextSend = cls.realtime + MM_CommandGap();
}

// the list once the commands before it went through; one at the end of the queue does
static void MM_RequestList( void ) {
	if ( mm.numQueue && mm.queueSeq[mm.numQueue - 1] )
		return;
	MM_Queue( "rpmodel list", ++mm.listSeq );
}

static const char *MM_AddCommand( const mmPiece_t *p ) {
	return va( "rpmodel add %s %i %i %i %i %i %i %i %i", p->name,
		MM_Round( p->origin[0] ), MM_Round( p->origin[1] ), MM_Round( p->origin[2] ),
		MM_NormalizeAngle( p->angles[PITCH] ), MM_NormalizeAngle( p->angles[YAW] ), MM_NormalizeAngle( p->angles[ROLL] ),
		p->scale, p->solid ? 1 : 0 );
}

// what is left to send: new models put down, server ones moved or deleted
static void MM_Pending( int *added, int *moved, int *deleted ) {
	*added = *moved = *deleted = 0;
	for ( int i = 0; i < mm.numPieces; i++ ) {
		const mmPiece_t *p = &mm.pieces[i];

		if ( p->sent || p->tmpl )
			continue;
		if ( MM_IsNew( p ) )
			( *added )++;
		else if ( p->deleted )
			( *deleted )++;
		else if ( MM_Moved( p ) )
			( *moved )++;
	}
}

// sends the changes to the pieces with mark set; removes go first, while the
// numbers still name the models they were read for
static int MM_SendMarked( void ) {
	int count = 0;

	for ( int i = 0; i < mm.numPieces; i++ ) {
		mmPiece_t *p = &mm.pieces[i];

		if ( !p->mark || p->sent || p->tmpl || MM_IsNew( p ) )
			continue;
		if ( p->deleted || MM_Moved( p ) )
			MM_Queue( va( "rpmodel remove %i", p->num ), 0 );
	}
	for ( int i = 0; i < mm.numPieces; i++ ) {
		mmPiece_t *p = &mm.pieces[i];

		if ( !p->mark || p->sent || p->tmpl )
			continue;
		if ( MM_IsNew( p ) || ( !p->deleted && MM_Moved( p ) ) ) {
			MM_Queue( MM_AddCommand( p ), 0 );
			p->placed = qtrue;
		} else if ( !p->deleted ) {
			continue;
		}
		p->sent = qtrue;
		count++;
	}
	if ( count ) {
		MM_RequestList();
		for ( int i = 0; i < mm.numPieces; i++ ) {
			if ( mm.pieces[i].mark && mm.pieces[i].sent && !mm.pieces[i].sentSeq )
				mm.pieces[i].sentSeq = mm.listSeq;
		}
	}
	for ( int i = 0; i < mm.numPieces; i++ )
		mm.pieces[i].mark = qfalse;
	return count;
}

static int MM_SendSelected( void ) {
	for ( int i = 0; i < mm.numPieces; i++ )
		mm.pieces[i].mark = MM_Sel( &mm.pieces[i] );
	return MM_SendMarked();
}

static int MM_SendAll( void ) {
	for ( int i = 0; i < mm.numPieces; i++ )
		mm.pieces[i].mark = qtrue;
	return MM_SendMarked();
}

// "130: (   144  4304     6) models/map_objects/cantina/fanfar.md3"
static qboolean MM_ParseListLine( const char *line, int *num, vec3_t origin, char *name, int nameSize ) {
	const int extLen = strlen( MM_EXT );
	const char *rest;
	float x, y, z;
	int n, used = 0, len;

	if ( sscanf( line, "%d: ( %f %f %f )%n", &n, &x, &y, &z, &used ) != 4 || !used )
		return qfalse;
	rest = line + used;
	while ( *rest == ' ' )
		rest++;
	if ( !Q_stricmpn( rest, MM_ROOT "/", strlen( MM_ROOT ) + 1 ) )
		rest += strlen( MM_ROOT ) + 1;
	len = strlen( rest );
	// effects and the like list the same way
	if ( len <= extLen || Q_stricmp( rest + len - extLen, MM_EXT ) )
		return qfalse;
	Q_strncpyz( name, rest, Q_min( len - extLen + 1, nameSize ) );
	for ( char *c = name; *c; c++ ) {
		if ( *c == '\\' )
			*c = '/';
	}
	*num = n;
	VectorSet( origin, x, y, z );
	return qtrue;
}

// a list starts: what the server had goes aside, to be matched with the new lines.
// seq is the request it answers, 0 if someone typed it
static void MM_BeginList( int seq ) {
	int keep = 0;

	MM_CheckMap();
	if ( seq )
		mm.listBegan = seq;
	mm.listBeginTime = cls.realtime;
	mm.numOld = 0;
	for ( int i = 0; i < mm.numPieces; i++ ) {
		mmPiece_t *p = &mm.pieces[i];

		// new ones sent before the list was asked for are on the server now
		if ( !MM_IsNew( p ) || ( p->sent && seq && p->sentSeq <= seq ) )
			mm.old[mm.numOld++] = *p;
		else
			mm.pieces[keep++] = *p;
	}
	mm.numPieces = keep;
	mm.hover = -1;
}

static void MM_AddListed( int num, const vec3_t origin, const char *name, int seq ) {
	mmPiece_t *p;

	// the same model as before, with what was done to it
	for ( int i = 0; i < mm.numOld; i++ ) {
		mmPiece_t *o = &mm.old[i];

		if ( o->num != num || Q_stricmp( o->name, name ) || Distance( o->srvOrigin, origin ) > MM_MATCH_DIST )
			continue;
		// its commands went through, so it should be gone; it wasn't, so it's as the server has it
		if ( o->sent && seq && o->sentSeq <= seq )
			break;
		if ( mm.numPieces < MM_MAX_PIECES )
			mm.pieces[mm.numPieces++] = *o;
		o->name[0] = '\0';
		return;
	}

	p = MM_NewPiece( name );
	if ( !p )
		return;
	p->num = num;
	VectorCopy( origin, p->origin );
	VectorCopy( origin, p->srvOrigin );
	p->srvScale = 100;
	p->srvSolid = qtrue;

	// one this manager just sent: it went with these, and stays selected
	for ( int i = 0; i < mm.numOld; i++ ) {
		mmPiece_t *o = &mm.old[i];

		if ( !o->sent || o->deleted || !o->name[0] || Q_stricmp( o->name, name ) || Distance( o->origin, origin ) > MM_MATCH_DIST )
			continue;
		VectorCopy( o->angles, p->angles );
		VectorCopy( o->angles, p->srvAngles );
		p->scale = p->srvScale = o->scale;
		p->solid = p->srvSolid = o->solid;
		p->known = qtrue;
		p->selected = o->selected;
		p->placed = o->placed;
		Q_strncpyz( p->tag, o->tag, sizeof( p->tag ) );
		o->name[0] = '\0';
		break;
	}
}

// every server print goes through here on its way to the cgame; qtrue keeps it out of the console
qboolean CL_ModelManager_ServerPrint( const char *text ) {
	char buf[MAX_STRING_CHARS], *line, *next, name[MAX_QPATH];
	qboolean swallow = qtrue, any = qfalse;
	vec3_t origin;
	int num;

	// the list is read into the free camera, which needs the index
	if ( cls.state != CA_ACTIVE || !mm.indexed )
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

		if ( MM_ParseListLine( line, &num, origin, name, sizeof( name ) ) ) {
			// there is no header: a line after a pause starts a new list
			if ( !mm.listLineTime || cls.realtime - mm.listLineTime > MM_LIST_LINE_GAP ) {
				// also read a list someone typed /rpmodel list for, just don't hide it
				mm.listOurs = (qboolean)( mm.listRequestTime && cls.realtime - mm.listRequestTime < MM_LIST_TIMEOUT );
				mm.listReading = mm.listOurs ? mm.listInFlight : 0;
				mm.listRequestTime = 0;
				MM_BeginList( mm.listReading );
			}
			mm.listLineTime = cls.realtime;
			MM_AddListed( num, origin, name, mm.listReading );
			if ( !mm.listOurs )
				swallow = qfalse;
		} else {
			swallow = qfalse;
			if ( mm.state != MM_OFF && cls.realtime - mm.sentTime < MM_REPLY_MS ) {
				Q_strncpyz( mm.reply, line, sizeof( mm.reply ) );
				mm.replyTime = cls.realtime;
			}
		}
	}
	return (qboolean)( any && swallow );
}

// the server sends no lines for an empty list
static void MM_CheckListTimeout( void ) {
	if ( !mm.listRequestTime || cls.realtime - mm.listRequestTime < MM_LIST_TIMEOUT )
		return;
	mm.listRequestTime = 0;
	MM_BeginList( mm.listInFlight );
}

static void MM_ModelPath( const char *configString, char *out, int outSize ) {
	const char *s = configString;

	if ( !Q_stricmpn( s, MM_ROOT "/", strlen( MM_ROOT ) + 1 ) )
		s += strlen( MM_ROOT ) + 1;
	Q_strncpyz( out, s, outSize );
	COM_StripExtension( out, out, outSize );
	for ( char *c = out; *c; c++ ) {
		if ( *c == '\\' )
			*c = '/';
	}
}

// the server model's entity in the snapshot, if it's near enough to the player to be in it
static const entityState_t *MM_FindEntity( const mmPiece_t *p ) {
	const entityState_t *byNumber = NULL;
	char path[MAX_QPATH];

	for ( int i = 0; i < cl.snap.numEntities; i++ ) {
		const entityState_t *es = &cl.parseEntities[( cl.snap.parseEntitiesNum + i ) & ( MAX_PARSE_ENTITIES - 1 )];

		if ( Distance( es->pos.trBase, p->srvOrigin ) > MM_MATCH_DIST )
			continue;
		if ( es->modelindex > 0 && es->modelindex < MAX_MODELS ) {
			MM_ModelPath( cl.gameState.stringData + cl.gameState.stringOffsets[CS_MODELS + es->modelindex], path, sizeof( path ) );
			if ( !Q_stricmp( path, p->name ) )
				return es;
		}
		if ( es->number == p->num )
			byNumber = es;
	}
	return byNumber;
}

// angles, scale and solidity aren't in the list; the entity has them
static void MM_SyncFromSnapshot( void ) {
	if ( !cl.snap.valid )
		return;
	for ( int i = 0; i < mm.numPieces; i++ ) {
		mmPiece_t *p = &mm.pieces[i];
		const entityState_t *es;
		qboolean edited;

		if ( MM_IsNew( p ) || p->sent )
			continue;
		es = MM_FindEntity( p );
		if ( !es )
			continue;
		edited = (qboolean)( p->known && ( MM_Moved( p ) || ( MM_Sel( p ) && mm.carrying ) ) );
		p->known = qtrue;
		VectorCopy( es->apos.trBase, p->srvAngles );
		p->srvScale = es->iModelScale > 0 ? es->iModelScale : 100;
		p->srvSolid = (qboolean)( es->solid != 0 );
		if ( !edited ) {
			VectorCopy( p->srvAngles, p->angles );
			p->scale = p->srvScale;
			p->solid = p->srvSolid;
		}
	}
}

/*
===============================================================================

EDITING

===============================================================================
*/

static void MM_ReleaseKeys( void ) {
	memset( mm.held, 0, sizeof( mm.held ) );
}

// the selection goes in hand, for the crosshair to carry or the keys to change
static qboolean MM_Pickup( qboolean locked ) {
	int count = 0;

	if ( mm.carrying )
		return qtrue;
	for ( int i = 0; i < mm.numPieces; i++ ) {
		const mmPiece_t *p = &mm.pieces[i];

		if ( !MM_Sel( p ) )
			continue;
		if ( p->deleted ) {
			MM_Message( S_COLOR_YELLOW "A selected model is deleted; U brings it back" );
			return qfalse;
		}
		if ( !MM_IsNew( p ) && !p->known ) {
			MM_Message( S_COLOR_YELLOW "A selected model is too far from your character to see how it's turned; it can only be deleted" );
			return qfalse;
		}
		count++;
	}
	if ( !count ) {
		MM_Message( S_COLOR_YELLOW "Nothing selected: click a model, or pick one in the browser (Tab)" );
		return qfalse;
	}
	for ( int i = 0; i < mm.numPieces; i++ ) {
		mmPiece_t *p = &mm.pieces[i];

		if ( !MM_Sel( p ) )
			continue;
		VectorCopy( p->origin, p->carryOrigin );
		VectorCopy( p->angles, p->carryAngles );
		p->carryScale = p->scale;
		p->carrySolid = p->solid;
	}
	mm.carrying = qtrue;
	mm.locked = locked;
	return qtrue;
}

// put it back: a template goes away, moved models go back where they were picked up
static void MM_CancelCarry( void ) {
	if ( !mm.carrying )
		return;
	mm.carrying = qfalse;
	for ( int i = 0; i < mm.numPieces; i++ ) {
		mmPiece_t *p = &mm.pieces[i];

		p->mark = (qboolean)( MM_Sel( p ) && p->tmpl );
		if ( MM_Sel( p ) && !p->tmpl ) {
			VectorCopy( p->carryOrigin, p->origin );
			VectorCopy( p->carryAngles, p->angles );
			p->scale = p->carryScale;
			p->solid = p->carrySolid;
		}
	}
	MM_RemoveMarked();
}

// put the selection down; a template leaves a copy and stays in hand
static void MM_Drop( void ) {
	if ( !mm.carrying )
		return;

	if ( MM_CarryingTemplate() ) {
		int numBefore = mm.numPieces;

		for ( int i = 0; i < numBefore; i++ ) {
			mmPiece_t *p = &mm.pieces[i], *copy;

			p->mark = qfalse;
			if ( !MM_Sel( p ) || !p->tmpl )
				continue;
			if ( mm.numPieces >= MM_MAX_PIECES ) {
				MM_Message( S_COLOR_YELLOW "Too many models in the free camera" );
				break;
			}
			copy = &mm.pieces[mm.numPieces++];
			*copy = *p;
			copy->tmpl = qfalse;
			copy->selected = qfalse;
			copy->mark = qtrue;
		}
		if ( mm.instant )
			MM_SendMarked();
		for ( int i = 0; i < mm.numPieces; i++ )
			mm.pieces[i].mark = qfalse;
		return;
	}

	mm.carrying = qfalse;
	if ( mm.instant )
		MM_SendSelected();
}

static void MM_ClickSelect( void ) {
	mmPiece_t *p;

	if ( mm.hover < 0 || mm.hover >= mm.numPieces ) {
		if ( !MM_ShiftDown() )
			MM_ClearSelection();
		return;
	}
	p = &mm.pieces[mm.hover];
	if ( MM_ShiftDown() ) {
		p->selected = (qboolean)!p->selected;
	} else {
		MM_ClearSelection();
		p->selected = qtrue;
	}
}

static void MM_Rotate( int axis, int sign ) {
	const int step = mmSnaps[mm.snap].angle;
	vec3_t pivot, offset, rotated, dir;
	matrix3_t ax;
	float yaw;

	if ( !MM_Pickup( qtrue ) )
		return;

	if ( MM_NumSelected() == 1 ) {
		mmPiece_t *p = MM_FirstSelected();

		p->angles[axis] = (float)MM_NormalizeAngle( MM_SnapTo( p->angles[axis] + sign * step, step ) );
		MM_KeepDefaults();
		return;
	}

	// a group turns about its middle, keeping its shape
	MM_Pivot( pivot );
	yaw = DEG2RAD( floorf( mm.camAng[YAW] / 90.0f + 0.5f ) * 90.0f );
	if ( axis == YAW )
		VectorSet( dir, 0, 0, 1 );
	else if ( axis == PITCH )
		VectorSet( dir, sinf( yaw ), -cosf( yaw ), 0 );	// the level axis closest to the camera's right
	else
		VectorSet( dir, cosf( yaw ), sinf( yaw ), 0 );	// and to its forward
	for ( int i = 0; i < mm.numPieces; i++ ) {
		mmPiece_t *p = &mm.pieces[i];

		if ( !MM_Sel( p ) )
			continue;
		VectorSubtract( p->origin, pivot, offset );
		RotatePointAroundVector( rotated, dir, offset, (float)( sign * step ) );
		VectorAdd( pivot, rotated, p->origin );
		if ( axis == YAW ) {
			p->angles[YAW] = AngleNormalize360( p->angles[YAW] + sign * step );
			continue;
		}
		AnglesToAxis( p->angles, ax );
		for ( int j = 0; j < 3; j++ ) {
			RotatePointAroundVector( rotated, dir, ax[j], (float)( sign * step ) );
			VectorCopy( rotated, ax[j] );
		}
		MM_AxisToAngles( ax, p->angles );
	}
}

static void MM_ResetAngles( qboolean scaleToo ) {
	if ( !MM_Pickup( qtrue ) )
		return;
	for ( int i = 0; i < mm.numPieces; i++ ) {
		if ( !MM_Sel( &mm.pieces[i] ) )
			continue;
		VectorClear( mm.pieces[i].angles );
		if ( scaleToo )
			mm.pieces[i].scale = 100;
	}
	MM_KeepDefaults();
}

static void MM_ChangeScale( int sign ) {
	const int step = MM_ShiftDown() ? 25 : 5;
	mmPiece_t *first;
	vec3_t pivot, offset;
	float f;
	int from, to;

	if ( !MM_Pickup( qtrue ) )
		return;
	first = MM_FirstSelected();
	from = first->scale;
	to = Com_Clampi( MM_SCALE_MIN, MM_SCALE_MAX, from + sign * step );
	if ( to == from )
		return;
	if ( MM_NumSelected() == 1 ) {
		first->scale = to;
		MM_KeepDefaults();
		return;
	}

	// a group grows about its middle
	f = to / (float)from;
	MM_Pivot( pivot );
	for ( int i = 0; i < mm.numPieces; i++ ) {
		mmPiece_t *p = &mm.pieces[i];

		if ( !MM_Sel( p ) )
			continue;
		p->scale = Com_Clampi( MM_SCALE_MIN, MM_SCALE_MAX, MM_Round( p->scale * f ) );
		VectorSubtract( p->origin, pivot, offset );
		VectorMA( pivot, f, offset, p->origin );
	}
}

static void MM_ToggleSolid( void ) {
	qboolean solid;

	if ( !MM_Pickup( qtrue ) )
		return;
	solid = (qboolean)!MM_FirstSelected()->solid;
	for ( int i = 0; i < mm.numPieces; i++ ) {
		if ( MM_Sel( &mm.pieces[i] ) )
			mm.pieces[i].solid = solid;
	}
	MM_KeepDefaults();
}

// moves along the world axis closest to the camera direction, so nudges stay on the grid
static void MM_Nudge( int dx, int dy, int dz ) {
	const int grid = mmSnaps[mm.snap].grid;
	const float yaw = DEG2RAD( floorf( mm.camAng[YAW] / 90.0f + 0.5f ) * 90.0f );
	vec3_t forward, right, delta;

	if ( !MM_Pickup( qtrue ) )
		return;
	mm.locked = qtrue;
	VectorSet( forward, cosf( yaw ), sinf( yaw ), 0 );
	VectorSet( right, sinf( yaw ), -cosf( yaw ), 0 );
	VectorScale( forward, (float)( dx * grid ), delta );
	VectorMA( delta, (float)( dy * grid ), right, delta );
	delta[2] = (float)( dz * grid );
	for ( int i = 0; i < 2; i++ )
		delta[i] = floorf( delta[i] + 0.5f );
	MM_TranslateSelection( delta );
}

static void MM_DeleteSelected( void ) {
	int count = 0;

	if ( MM_CarryingTemplate() ) {
		MM_CancelCarry();
		return;
	}
	MM_CancelCarry();
	for ( int i = 0; i < mm.numPieces; i++ ) {
		mmPiece_t *p = &mm.pieces[i];

		p->mark = qfalse;
		if ( !MM_Sel( p ) )
			continue;
		if ( MM_IsNew( p ) ) {
			p->mark = qtrue;	// never sent, just goes
		} else {
			p->deleted = qtrue;
			count++;
		}
	}
	MM_RemoveMarked();
	if ( count && mm.instant )
		MM_SendSelected();
}

// what hasn't been sent goes back to how the server has it: the selection, or everything
static void MM_Undo( void ) {
	const qboolean all = (qboolean)!MM_NumSelected();

	MM_CancelCarry();
	for ( int i = 0; i < mm.numPieces; i++ ) {
		mmPiece_t *p = &mm.pieces[i];

		p->mark = qfalse;
		if ( p->sent || p->tmpl || ( !all && !p->selected ) )
			continue;
		if ( MM_IsNew( p ) ) {
			p->mark = qtrue;
			continue;
		}
		p->deleted = qfalse;
		if ( p->known ) {
			VectorCopy( p->srvOrigin, p->origin );
			VectorCopy( p->srvAngles, p->angles );
			p->scale = p->srvScale;
			p->solid = p->srvSolid;
		}
	}
	MM_RemoveMarked();
}

// copies of the selection, in hand to put down as many times as wanted
static void MM_CopySelection( void ) {
	int numBefore = mm.numPieces, count = 0;

	if ( mm.carrying )
		return;
	for ( int i = 0; i < numBefore; i++ ) {
		mmPiece_t *p = &mm.pieces[i], *copy;

		if ( !MM_Sel( p ) )
			continue;
		if ( !MM_IsNew( p ) && !p->known ) {
			MM_Message( S_COLOR_YELLOW "A selected model is too far from your character to see how it's turned" );
			continue;
		}
		copy = MM_NewPiece( p->name );
		if ( !copy )
			break;
		p = &mm.pieces[i];
		VectorCopy( p->origin, copy->origin );
		VectorCopy( p->angles, copy->angles );
		copy->scale = p->scale;
		copy->solid = p->solid;
		copy->tmpl = qtrue;
		copy->selected = qtrue;
		p->selected = qfalse;
		// a copy of a named construct is offered its name
		if ( !count )
			Q_strncpyz( mm.tmplName, p->tag, sizeof( mm.tmplName ) );
		count++;
	}
	if ( count ) {
		mm.carrying = qtrue;
		mm.locked = qfalse;
	}
}

// a model from the browser, in hand
static void MM_StartTemplate( int model ) {
	mmPiece_t *p;

	MM_CancelCarry();
	MM_ClearSelection();
	p = MM_NewPiece( mm.models[model].path );
	if ( !p )
		return;
	MM_InFrontOfCamera( p->origin );
	VectorCopy( mm.defAngles, p->angles );
	p->scale = mm.defScale;
	p->solid = mm.defSolid;
	p->tmpl = qtrue;
	p->selected = qtrue;
	mm.carrying = qtrue;
	mm.locked = qfalse;
	Q_strncpyz( mm.tmplName, mm.models[model].base, sizeof( mm.tmplName ) );
}

// a construct, in hand
static void MM_StartConstruct( const mmSavedPiece_t *in, int count ) {
	vec3_t anchor;

	MM_CancelCarry();
	MM_ClearSelection();
	MM_InFrontOfCamera( anchor );
	for ( int i = 0; i < count; i++ ) {
		mmPiece_t *p = MM_NewPiece( in[i].name );

		if ( !p )
			break;
		VectorAdd( anchor, in[i].offset, p->origin );
		VectorCopy( in[i].angles, p->angles );
		p->scale = in[i].scale;
		p->solid = in[i].solid;
		p->tmpl = qtrue;
		p->selected = qtrue;
	}
	if ( MM_NumSelected() ) {
		mm.carrying = qtrue;
		mm.locked = qfalse;
	}
}

/*
===============================================================================

SAVED CONSTRUCTS

===============================================================================
*/

static qboolean MM_SaveNameChar( int ch ) {
	return (qboolean)( ( ch >= 'a' && ch <= 'z' ) || ( ch >= 'A' && ch <= 'Z' ) || ( ch >= '0' && ch <= '9' ) || ch == '_' || ch == '-' );
}

static void MM_WriteLine( fileHandle_t f, const char *text ) {
	FS_Write( text, strlen( text ), f );
}

// a name for a file, or for models: what is said if it can't be one
static qboolean MM_CheckName( const char *name, const char *what ) {
	if ( !name[0] ) {
		MM_Message( va( S_COLOR_YELLOW "Give the %s a name", what ) );
		return qfalse;
	}
	for ( const char *c = name; *c; c++ ) {
		if ( !MM_SaveNameChar( *c ) ) {
			MM_Message( S_COLOR_YELLOW "Use only letters, numbers, - and _ in the name" );
			return qfalse;
		}
	}
	return qtrue;
}

static const char *MM_MapName( void ) {
	return Info_ValueForKey( cl.gameState.stringData + cl.gameState.stringOffsets[CS_SERVERINFO], "mapname" );
}

// the selection, relative to the middle of its floor
static qboolean MM_SaveSelection( const char *name ) {
	vec3_t mins, maxs, anchor;
	fileHandle_t f;
	int count = MM_NumSelected(), unknown = 0;
	char path[MAX_QPATH], mapName[MAX_QPATH];

	if ( !MM_CheckName( name, "construct" ) )
		return qfalse;
	if ( !count ) {
		MM_Message( S_COLOR_YELLOW "Select the models to save first" );
		return qfalse;
	}

	ClearBounds( mins, maxs );
	for ( int i = 0; i < mm.numPieces; i++ ) {
		if ( MM_Sel( &mm.pieces[i] ) )
			AddPointToBounds( mm.pieces[i].origin, mins, maxs );
	}
	VectorSet( anchor, MM_Round( ( mins[0] + maxs[0] ) * 0.5f ), MM_Round( ( mins[1] + maxs[1] ) * 0.5f ), MM_Round( mins[2] ) );

	Com_sprintf( path, sizeof( path ), "%s/%s%s", MM_SAVE_DIR, name, MM_SAVE_EXT );
	f = FS_FOpenFileWrite( path );
	if ( !f ) {
		MM_Message( va( S_COLOR_RED "Couldn't write %s", path ) );
		return qfalse;
	}
	Q_strncpyz( mapName, MM_MapName(), sizeof( mapName ) );
	MM_WriteLine( f, va( "// Model manager construct: %i models, made on %s around %i %i %i\n", count, mapName,
		(int)anchor[0], (int)anchor[1], (int)anchor[2] ) );
	MM_WriteLine( f, "// Load it from the model manager's Saved constructs, or exec this file\n" );
	MM_WriteLine( f, "// piece <model> <x y z from the construct's middle> <pitch yaw roll> <scale%> <solid>\n" );
	MM_WriteLine( f, va( "modelmanager construct %s\n", name ) );
	for ( int i = 0; i < mm.numPieces; i++ ) {
		const mmPiece_t *p = &mm.pieces[i];

		if ( !MM_Sel( p ) )
			continue;
		if ( !MM_IsNew( p ) && !p->known )
			unknown++;
		MM_WriteLine( f, va( "modelmanager piece %s %i %i %i %i %i %i %i %i\n", p->name,
			MM_Round( p->origin[0] - anchor[0] ), MM_Round( p->origin[1] - anchor[1] ), MM_Round( p->origin[2] - anchor[2] ),
			MM_NormalizeAngle( p->angles[PITCH] ), MM_NormalizeAngle( p->angles[YAW] ), MM_NormalizeAngle( p->angles[ROLL] ),
			p->scale, p->solid ? 1 : 0 ) );
	}
	FS_FCloseFile( f );
	MM_RefreshSaves();

	if ( unknown )
		MM_Message( va( S_COLOR_YELLOW "Saved %i models as %s; %i too far from your character were saved unturned", count, path, unknown ) );
	else
		MM_Message( va( S_COLOR_GREEN "Saved %i models as %s", count, path ) );
	Com_Printf( "Model manager: saved %i models as %s\n", count, path );
	return qtrue;
}

static qboolean MM_ParsePiece( char **argv, int argc, void *to ) {
	mmSavedPiece_t *out = (mmSavedPiece_t *)to;

	// modelmanager piece <model> <x y z> <pitch yaw roll> <scale> <solid>
	if ( argc < 11 || Q_stricmp( argv[0], MM_TOGGLE_CMD ) || Q_stricmp( argv[1], "piece" ) )
		return qfalse;
	Q_strncpyz( out->name, argv[2], sizeof( out->name ) );
	VectorSet( out->offset, atof( argv[3] ), atof( argv[4] ), atof( argv[5] ) );
	VectorSet( out->angles, atof( argv[6] ), atof( argv[7] ), atof( argv[8] ) );
	out->scale = Com_Clampi( MM_SCALE_MIN, MM_SCALE_MAX, atoi( argv[9] ) );
	out->solid = (qboolean)( atoi( argv[10] ) != 0 );
	return qtrue;
}

typedef qboolean ( *mmLineParser_t )( char **argv, int argc, void *out );

// the lines of a saved file parse turns into entries of out, each size big; -1 if there's no file
static int MM_ReadLines( const char *path, mmLineParser_t parse, void *out, size_t size, int max ) {
	char *buf, *line, *next, *argv[16];
	int count = 0, argc;

	if ( FS_ReadFile( path, (void **)&buf ) < 0 || !buf )
		return -1;
	for ( line = buf; line && count < max; line = next ) {
		next = strchr( line, '\n' );
		if ( next )
			*next++ = '\0';
		for ( char *c = line; *c; c++ ) {
			if ( *c == '\r' )
				*c = '\0';
		}
		argc = MM_SplitLine( line, argv, ARRAY_LEN( argv ) );
		if ( parse( argv, argc, (byte *)out + count * size ) )
			count++;
	}
	FS_FreeFile( buf );
	return count;
}

static int MM_ReadSave( const char *name, mmSavedPiece_t *out, int max ) {
	return MM_ReadLines( va( "%s/%s%s", MM_SAVE_DIR, name, MM_SAVE_EXT ), MM_ParsePiece, out, sizeof( *out ), max );
}

// the pieces of the save under the browser's cursor, read once per save
static void MM_ReadPreviewSave( int save ) {
	if ( save < 0 || save >= mm.numSaves ) {
		mm.previewSave[0] = '\0';
		mm.numSavePieces = 0;
		return;
	}
	if ( !Q_stricmp( mm.previewSave, mm.saves[save] ) )
		return;
	Q_strncpyz( mm.previewSave, mm.saves[save], sizeof( mm.previewSave ) );
	mm.numSavePieces = MM_ReadSave( mm.saves[save], mm.savePieces, MM_MAX_SAVE_PIECES );
}

/*
===============================================================================

NAMES AND STATES

A name, given with Shift+Enter, stays on the models it was given to, a construct's
all sharing it. RPMod doesn't keep it, so a state remembers it: the models placed
with the manager and the named ones, where the server has them. Once rpdump load
has brought them back, reloading the state finds each one by its model, position,
yaw and roll in a fresh list, and gives it its name again.

===============================================================================
*/

static qboolean MM_EnsureIndex( void ) {
	if ( cls.state != CA_ACTIVE || !cls.cgameStarted ) {
		Com_Printf( "Model manager: join a server first\n" );
		return qfalse;
	}
	MM_CheckMap();
	if ( !mm.indexed )
		MM_BuildIndex();
	return qtrue;
}

// both said, for the console commands that work with the manager closed
static void MM_Report( const char *text ) {
	MM_Message( text );
	Com_Printf( "Model manager: %s\n", text );
}

// names the selection, or what the template in hand puts down, and sends it all
static void MM_NameAndSend( const char *name ) {
	const int numBefore = mm.numPieces;
	int count = 0;

	if ( MM_CarryingTemplate() ) {
		MM_Drop();
		for ( int i = numBefore; i < mm.numPieces; i++ ) {
			Q_strncpyz( mm.pieces[i].tag, name, sizeof( mm.pieces[i].tag ) );
			count++;
		}
	} else {
		for ( int i = 0; i < mm.numPieces; i++ ) {
			if ( !MM_Sel( &mm.pieces[i] ) )
				continue;
			Q_strncpyz( mm.pieces[i].tag, name, sizeof( mm.pieces[i].tag ) );
			count++;
		}
		MM_Drop();
	}
	MM_SendAll();
	if ( name[0] )
		MM_Message( va( S_COLOR_GREEN "Named %i models %s", count, name ) );
	else
		MM_Message( va( "Took the name off %i models", count ) );
}

static void MM_RemoveNamed( const char *name ) {
	int count = 0, waiting = 0;

	if ( !MM_EnsureIndex() )
		return;

	// some of them in hand go back first
	for ( int i = 0; i < mm.numPieces && mm.carrying; i++ ) {
		if ( MM_Sel( &mm.pieces[i] ) && !mm.pieces[i].tmpl && !Q_stricmp( mm.pieces[i].tag, name ) )
			MM_CancelCarry();
	}

	// ones never sent just go, the server's are removed
	for ( int i = 0; i < mm.numPieces; i++ ) {
		mmPiece_t *p = &mm.pieces[i];

		p->mark = qfalse;
		if ( p->tmpl || Q_stricmp( p->tag, name ) )
			continue;
		if ( p->sent ) {
			waiting++;
			continue;
		}
		p->mark = MM_IsNew( p );
		count++;
	}
	MM_RemoveMarked();
	for ( int i = 0; i < mm.numPieces; i++ ) {
		mmPiece_t *p = &mm.pieces[i];

		p->mark = (qboolean)( !p->tmpl && !p->sent && !Q_stricmp( p->tag, name ) );
		if ( p->mark ) {
			p->deleted = qtrue;
			p->selected = qfalse;
		}
	}
	MM_SendMarked();

	if ( !count && !waiting )
		MM_Report( va( S_COLOR_YELLOW "No models are named %s; modelmanager names lists the names", name ) );
	else if ( waiting )
		MM_Report( va( "Removing %i models named %s; %i more are still on their way to the server, try again once they're there", count, name, waiting ) );
	else
		MM_Report( va( "Removing %i models named %s", count, name ) );
}

static void MM_PrintNames( void ) {
	int names = 0;

	for ( int i = 0; i < mm.numPieces; i++ ) {
		const mmPiece_t *p = &mm.pieces[i];
		qboolean seen = qfalse;

		if ( p->tmpl || !p->tag[0] )
			continue;
		for ( int j = 0; j < i && !seen; j++ )
			seen = (qboolean)( !mm.pieces[j].tmpl && !Q_stricmp( mm.pieces[j].tag, p->tag ) );
		if ( seen )
			continue;
		Com_Printf( "%s: %i models\n", p->tag, MM_CountTagged( p->tag ) );
		names++;
	}
	if ( !names )
		Com_Printf( "Model manager: no named models; Shift+Enter in the free camera names what it sends, and reloading a state brings names back\n" );
}

// what a state remembers: the models placed with the manager, and the named ones
static qboolean MM_InState( const mmPiece_t *p ) {
	if ( p->tmpl || p->deleted || ( MM_IsNew( p ) && !p->sent ) )
		return qfalse;
	return (qboolean)( p->placed || p->tag[0] );
}

static int MM_StateCount( void ) {
	int count = 0;

	for ( int i = 0; i < mm.numPieces; i++ ) {
		if ( MM_InState( &mm.pieces[i] ) )
			count++;
	}
	return count;
}

static qboolean MM_SaveState( const char *name ) {
	char path[MAX_QPATH];
	fileHandle_t f;
	int count = MM_StateCount(), named = 0;

	if ( !MM_CheckName( name, "state" ) )
		return qfalse;
	if ( !count ) {
		MM_Message( S_COLOR_YELLOW "Nothing to save: no models were placed or named yet" );
		return qfalse;
	}

	Com_sprintf( path, sizeof( path ), "%s/%s%s", MM_STATE_DIR, name, MM_SAVE_EXT );
	f = FS_FOpenFileWrite( path );
	if ( !f ) {
		MM_Message( va( S_COLOR_RED "Couldn't write %s", path ) );
		return qfalse;
	}
	MM_WriteLine( f, va( "// Model manager state: %i models on %s\n", count, MM_MapName() ) );
	MM_WriteLine( f, "// Once the models are back on the map, after rpdump load, reload it from the model manager's\n" );
	MM_WriteLine( f, "// Saved states, or exec this file, and they get their names back\n" );
	MM_WriteLine( f, "// stated <name, - for none> <model> <x y z> <pitch yaw roll> <scale%> <solid> <angles seen>\n" );
	MM_WriteLine( f, va( "modelmanager reloadstate %s\n", name ) );
	for ( int i = 0; i < mm.numPieces; i++ ) {
		const mmPiece_t *p = &mm.pieces[i];
		// where the server has it, or will once what was sent gets there
		const qboolean current = (qboolean)( MM_IsNew( p ) || p->sent );
		const float *origin = current ? p->origin : p->srvOrigin, *angles = current ? p->angles : p->srvAngles;

		if ( !MM_InState( p ) )
			continue;
		if ( p->tag[0] )
			named++;
		MM_WriteLine( f, va( "modelmanager stated %s %s %i %i %i %i %i %i %i %i %i\n", p->tag[0] ? p->tag : "-", p->name,
			MM_Round( origin[0] ), MM_Round( origin[1] ), MM_Round( origin[2] ),
			MM_NormalizeAngle( angles[PITCH] ), MM_NormalizeAngle( angles[YAW] ), MM_NormalizeAngle( angles[ROLL] ),
			current ? p->scale : p->srvScale, ( current ? p->solid : p->srvSolid ) ? 1 : 0, current || p->known ? 1 : 0 ) );
	}
	FS_FCloseFile( f );
	MM_RefreshSaves();

	MM_Message( va( S_COLOR_GREEN "Saved the state of %i models, %i of them named, as %s", count, named, path ) );
	Com_Printf( "Model manager: saved the state of %i models as %s\n", count, path );
	return qtrue;
}

static qboolean MM_ParseStateModel( char **argv, int argc, void *to ) {
	mmStateModel_t *out = (mmStateModel_t *)to;

	// modelmanager stated <name> <model> <x y z> <pitch yaw roll> <scale> <solid> <angles seen>
	if ( argc < 13 || Q_stricmp( argv[0], MM_TOGGLE_CMD ) || Q_stricmp( argv[1], "stated" ) )
		return qfalse;
	Q_strncpyz( out->tag, strcmp( argv[2], "-" ) ? argv[2] : "", sizeof( out->tag ) );
	Q_strncpyz( out->name, argv[3], sizeof( out->name ) );
	VectorSet( out->origin, atof( argv[4] ), atof( argv[5] ), atof( argv[6] ) );
	VectorSet( out->angles, atof( argv[7] ), atof( argv[8] ), atof( argv[9] ) );
	out->scale = Com_Clampi( MM_SCALE_MIN, MM_SCALE_MAX, atoi( argv[10] ) );
	out->solid = (qboolean)( atoi( argv[11] ) != 0 );
	out->known = (qboolean)( atoi( argv[12] ) != 0 );
	return qtrue;
}

static int MM_ReadState( const char *name, mmStateModel_t *out, int max ) {
	return MM_ReadLines( va( "%s/%s%s", MM_STATE_DIR, name, MM_SAVE_EXT ), MM_ParseStateModel, out, sizeof( *out ), max );
}

// what the state under the browser's cursor holds, as lines of text
static void MM_ReadPreviewState( int state ) {
	const int maxNames = 14;
	mmStateModel_t *models;
	int count, named = 0, names = 0, shown = 0;
	char *out = mm.stateSummary;
	const int size = sizeof( mm.stateSummary );

	if ( state < 0 || state >= mm.numStates ) {
		mm.previewState[0] = mm.stateSummary[0] = '\0';
		return;
	}
	if ( !Q_stricmp( mm.previewState, mm.states[state] ) )
		return;
	Q_strncpyz( mm.previewState, mm.states[state], sizeof( mm.previewState ) );

	models = (mmStateModel_t *)Z_Malloc( sizeof( mmStateModel_t ) * MM_MAX_PIECES, TAG_TEMP_WORKSPACE, qfalse );
	count = MM_ReadState( mm.states[state], models, MM_MAX_PIECES );
	for ( int i = 0; i < count; i++ ) {
		if ( models[i].tag[0] )
			named++;
	}
	Com_sprintf( out, size, "%i models, %i of them named\n", Q_max( 0, count ), named );
	for ( int i = 0; i < count; i++ ) {
		int same = 0;
		qboolean seen = qfalse;

		if ( !models[i].tag[0] )
			continue;
		for ( int j = 0; j < i && !seen; j++ )
			seen = (qboolean)!Q_stricmp( models[j].tag, models[i].tag );
		if ( seen )
			continue;
		names++;
		if ( shown >= maxNames )
			continue;
		for ( int j = i; j < count; j++ ) {
			if ( !Q_stricmp( models[j].tag, models[i].tag ) )
				same++;
		}
		Q_strcat( out, size, va( "  %s " S_COLOR_GREY "(%i)" S_COLOR_WHITE "\n", models[i].tag, same ) );
		shown++;
	}
	if ( names > shown )
		Q_strcat( out, size, va( S_COLOR_GREY "  and %i more names\n", names - shown ) );
	Z_Free( models );
}

// the list asked for is in: each model of the state, the closest of its kind where it was
static void MM_MatchState( void ) {
	int matched = 0, named = 0;

	mm.reloading = qfalse;
	for ( int i = 0; i < mm.numPieces; i++ )
		mm.pieces[i].mark = qfalse;

	for ( int i = 0; i < mm.numStateModels; i++ ) {
		const mmStateModel_t *s = &mm.stateModels[i];
		mmPiece_t *best = NULL;
		float bestDist = MM_STATE_MATCH_DIST, d;

		for ( int j = 0; j < mm.numPieces; j++ ) {
			mmPiece_t *p = &mm.pieces[j];

			if ( p->mark || MM_IsNew( p ) || p->sent || p->deleted || Q_stricmp( p->name, s->name ) )
				continue;
			d = Distance( p->srvOrigin, s->origin );
			if ( d > bestDist )
				continue;
			// a construct's models often share a spot, turned different ways
			if ( p->known && s->known && ( !MM_SameAngle( p->srvAngles[YAW], s->angles[YAW] ) || !MM_SameAngle( p->srvAngles[ROLL], s->angles[ROLL] ) ) )
				continue;
			best = p;
			bestDist = d;
		}
		if ( !best )
			continue;

		best->mark = qtrue;
		best->placed = qtrue;
		if ( s->tag[0] ) {
			Q_strncpyz( best->tag, s->tag, sizeof( best->tag ) );
			named++;
		}
		// one too far from the player to be seen is turned the way the state had it
		if ( !best->known && s->known ) {
			VectorCopy( s->angles, best->srvAngles );
			VectorCopy( s->angles, best->angles );
			best->srvScale = best->scale = s->scale;
			best->srvSolid = best->solid = s->solid;
			best->known = qtrue;
		}
		matched++;
	}

	for ( int i = 0; i < mm.numPieces; i++ )
		mm.pieces[i].mark = qfalse;
	if ( matched == mm.numStateModels )
		MM_Report( va( S_COLOR_GREEN "Reloaded state %s: found all %i models, %i of them named", mm.reloadName, matched, named ) );
	else
		MM_Report( va( S_COLOR_YELLOW "Reloaded state %s: found %i of %i models, %i of them named; the rest aren't on the map, or not where the state had them",
			mm.reloadName, matched, mm.numStateModels, named ) );
}

// waits for the list asked for to have been read whole
static void MM_CheckReload( void ) {
	if ( !mm.reloading || mm.listBegan < mm.reloadSeq || mm.listRequestTime )
		return;
	if ( cls.realtime - Q_max( mm.listLineTime, mm.listBeginTime ) < MM_LIST_LINE_GAP )
		return;
	MM_MatchState();
}

static void MM_ReloadState( const char *name ) {
	int count;

	if ( !MM_EnsureIndex() )
		return;
	count = MM_ReadState( name, mm.stateModels, MM_MAX_PIECES );
	if ( count <= 0 ) {
		MM_Report( va( S_COLOR_YELLOW "%s %s/%s%s", count < 0 ? "No state" : "No models in", MM_STATE_DIR, name, MM_SAVE_EXT ) );
		return;
	}
	mm.numStateModels = count;
	Q_strncpyz( mm.reloadName, name, sizeof( mm.reloadName ) );
	mm.reloading = qtrue;
	MM_RequestList();
	mm.reloadSeq = mm.listSeq;
	MM_Report( va( "Reloading state %s: reading the map's models from the server", name ) );
}

/*
===============================================================================

STATE CHANGES

===============================================================================
*/

static void MM_EnterBrowse( void ) {
	MM_CancelCarry();
	mm.naming = MM_NAMING_NONE;
	mm.state = MM_BROWSE;
	MM_ReleaseKeys();
}

static void MM_EnterCamera( void ) {
	mm.state = MM_CAMERA;
	mm.lastFrameTime = cls.realtime;
	mm.hover = -1;
	MM_ReleaseKeys();
}

static qboolean MM_Open( void ) {
	if ( mm.state != MM_OFF )
		return qtrue;
	if ( !MM_EnsureIndex() )
		return qfalse;
	CL_NpcManager_Close();
	CL_ShaderManager_Close();
	CL_EffectManager_Close();
	CL_MapEditor_Close();
	CL_Storyteller_Close();
	CL_RpCinematic_EditorClose();
	CL_SoundManager_Close();

	mm.font = re->RegisterFont( "arialnb" );
	if ( !mm.font )
		mm.font = cls.menuFont;

	if ( mm.haveView ) {
		VectorCopy( mm.viewOrg, mm.camOrg );
		VectorCopy( mm.viewAng, mm.camAng );
	} else {
		VectorCopy( cl.snap.ps.origin, mm.camOrg );
		mm.camOrg[2] += cl.snap.ps.viewheight;
		VectorCopy( cl.viewangles, mm.camAng );
	}
	if ( mm.camAng[PITCH] > 180.0f )
		mm.camAng[PITCH] -= 360.0f;
	mm.camAng[ROLL] = 0;
	mm.cursorX = SCREEN_WIDTH * 0.5f;
	mm.cursorY = SCREEN_HEIGHT * 0.5f;
	MM_RefreshSaves();
	MM_RebuildView();
	MM_RequestList();
	MM_EnterBrowse();
	Key_SetCatcher( Key_GetCatcher() | KEYCATCH_MODELMANAGER );
	return qtrue;
}

static void MM_Close( void ) {
	MM_CancelCarry();
	mm.naming = MM_NAMING_NONE;
	mm.state = MM_OFF;
	MM_ReleaseKeys();
	Key_SetCatcher( Key_GetCatcher() & ~KEYCATCH_MODELMANAGER );
}

void CL_ModelManager_Close( void ) {
	if ( mm.state != MM_OFF )
		MM_Close();
}

// opened from a storyteller step
void CL_ModelManager_OpenForStory( void ) {
	if ( MM_Open() )
		MM_Message( S_COLOR_GREEN "For the story: what you send here goes in it as steps, not to the server" );
}

static void MM_LoadSave( const char *name ) {
	mmSavedPiece_t *pieces = (mmSavedPiece_t *)Z_Malloc( sizeof( mmSavedPiece_t ) * MM_MAX_SAVE_PIECES, TAG_TEMP_WORKSPACE, qfalse );
	int count = MM_ReadSave( name, pieces, MM_MAX_SAVE_PIECES );

	if ( count < 0 )
		Com_Printf( S_COLOR_YELLOW "Model manager: no construct %s/%s%s\n", MM_SAVE_DIR, name, MM_SAVE_EXT );
	else if ( !count )
		Com_Printf( S_COLOR_YELLOW "Model manager: %s/%s%s has no models in it\n", MM_SAVE_DIR, name, MM_SAVE_EXT );
	else if ( MM_Open() ) {
		MM_EnterCamera();
		MM_StartConstruct( pieces, count );
		Q_strncpyz( mm.tmplName, name, sizeof( mm.tmplName ) );
	}
	Z_Free( pieces );
}

void CL_ModelManager_Init( void ) {
	memset( &mm, 0, sizeof( mm ) );
	mm.defScale = 100;
	mm.defSolid = qtrue;
	mm.snap = 2;
	mm.instant = qtrue;
	mm.hover = -1;
	mm.peek = -1;
	cl_modelFollowFront = Cvar_Get( "cl_modelFollowFront", "0", CVAR_ARCHIVE_ND, "Model manager: 1 keeps what's in hand in front of the camera, 0 puts it onto what the crosshair aims at" );
	cl_modelHoldDistance = Cvar_Get( "cl_modelHoldDistance", "128", CVAR_ARCHIVE_ND, "Model manager: how far in front of the camera what's in hand stays" );
}

// cgame is going away: map change, disconnect or vid_restart
void CL_ModelManager_Shutdown( void ) {
	if ( mm.state != MM_OFF )
		MM_Close();
	// the pure pk3 list can change with the next map or server
	mm.indexed = qfalse;
	mm.haveView = qfalse;
	// the pieces stay for their names, until MM_CheckMap finds another map or server
	mm.numQueue = mm.queueTotal = 0;
	mm.listRequestTime = mm.listLineTime = 0;
}

/*
===============================================================================

CONSOLE COMMAND

===============================================================================
*/

static void MM_PrintUsage( void ) {
	Com_Printf( "usage: modelmanager                    toggle the model manager (bind a key to it)\n" );
	Com_Printf( "       modelmanager <search>           open the browser filtered by <search>\n" );
	Com_Printf( "       modelmanager edit               fly the free camera over the map's models\n" );
	Com_Printf( "       modelmanager save <name>        save the selected models as a construct\n" );
	Com_Printf( "       modelmanager load <name>        pick up a saved construct to place it\n" );
	Com_Printf( "       modelmanager names              list the names given to models with Shift+Enter\n" );
	Com_Printf( "       modelmanager remove <name>      remove the models with that name from the map\n" );
	Com_Printf( "       modelmanager savestate <name>   save the models placed or named, and their names\n" );
	Com_Printf( "       modelmanager reloadstate <name> give the models of a state their names back, after rpdump load\n" );
	Com_Printf( "       modelmanager scale <percent>    set the scale of the selection, or of new models\n" );
	Com_Printf( "       modelmanager solid <0|1>        set whether the selection, or new models, are solid\n" );
	Com_Printf( "       modelmanager angles <p> <y> <r> set the angles of the selection, or of new models\n" );
	Com_Printf( "       modelmanager origin <x> <y> <z> move the selection there\n" );
	Com_Printf( "       modelmanager list               read the map's models from the server again\n" );
	Com_Printf( "       modelmanager reindex            rescan the filesystem for models\n" );
	Com_Printf( "       modelmanager close              close the model manager\n" );
	Com_Printf( "Saved constructs are in the %s folder of the game folder, states in %s; both can be exec'd\n", MM_SAVE_DIR, MM_STATE_DIR );
}

void CL_ModelManager_f( void ) {
	const char *arg = Cmd_Argv( 1 );
	int argc = Cmd_Argc();

	if ( argc < 2 ) {
		if ( mm.state == MM_OFF )
			MM_Open();
		else
			MM_Close();
		return;
	}

	if ( !Q_stricmp( arg, "help" ) || !Q_stricmp( arg, "?" ) ) {
		MM_PrintUsage();
	} else if ( !Q_stricmp( arg, "close" ) ) {
		CL_ModelManager_Close();
	} else if ( !Q_stricmp( arg, "reindex" ) ) {
		if ( mm.state == MM_CAMERA )
			MM_EnterBrowse();
		MM_BuildIndex();
		MM_RefreshSaves();
		MM_RebuildView();
	} else if ( !Q_stricmp( arg, "list" ) ) {
		MM_RequestList();
	} else if ( !Q_stricmp( arg, "edit" ) ) {
		if ( MM_Open() )
			MM_EnterCamera();
	} else if ( !Q_stricmp( arg, "save" ) && argc >= 3 ) {
		MM_SaveSelection( Cmd_Argv( 2 ) );
		Com_Printf( "%s\n", mm.message );
	} else if ( !Q_stricmp( arg, "load" ) && argc >= 3 ) {
		char name[MAX_QPATH];

		Q_strncpyz( name, Cmd_Argv( 2 ), sizeof( name ) );
		COM_StripExtension( name, name, sizeof( name ) );
		MM_LoadSave( name );
	} else if ( !Q_stricmp( arg, "construct" ) ) {
		// a saved construct being exec'd: its pieces follow
		if ( MM_Open() ) {
			MM_EnterCamera();
			MM_CancelCarry();
			MM_ClearSelection();
			MM_InFrontOfCamera( mm.constructAnchor );
			Q_strncpyz( mm.tmplName, Cmd_Argv( 2 ), sizeof( mm.tmplName ) );
		}
	} else if ( !Q_stricmp( arg, "piece" ) ) {
		mmSavedPiece_t piece;
		char *argv[16];
		int n = Q_min( argc, (int)ARRAY_LEN( argv ) );
		mmPiece_t *p;

		for ( int i = 0; i < n; i++ )
			argv[i] = (char *)Cmd_Argv( i );
		if ( mm.state != MM_CAMERA || !MM_ParsePiece( argv, n, &piece ) || !( p = MM_NewPiece( piece.name ) ) )
			return;
		VectorAdd( mm.constructAnchor, piece.offset, p->origin );
		VectorCopy( piece.angles, p->angles );
		p->scale = piece.scale;
		p->solid = piece.solid;
		p->tmpl = qtrue;
		p->selected = qtrue;
		mm.carrying = qtrue;
		mm.locked = qfalse;
	} else if ( !Q_stricmp( arg, "remove" ) && argc >= 3 ) {
		MM_RemoveNamed( Cmd_Argv( 2 ) );
	} else if ( !Q_stricmp( arg, "names" ) ) {
		MM_PrintNames();
	} else if ( !Q_stricmp( arg, "savestate" ) && argc >= 3 ) {
		if ( MM_EnsureIndex() && !MM_SaveState( Cmd_Argv( 2 ) ) )
			Com_Printf( "%s\n", mm.message );
	} else if ( !Q_stricmp( arg, "reloadstate" ) && argc >= 3 ) {
		char name[MAX_QPATH];

		Q_strncpyz( name, Cmd_Argv( 2 ), sizeof( name ) );
		COM_StripExtension( name, name, sizeof( name ) );
		MM_ReloadState( name );
	} else if ( !Q_stricmp( arg, "stated" ) ) {
		// a model of a state being exec'd; its reloadstate line read them all
	} else if ( !Q_stricmp( arg, "scale" ) && argc >= 3 ) {
		const int scale = Com_Clampi( MM_SCALE_MIN, MM_SCALE_MAX, atoi( Cmd_Argv( 2 ) ) );

		if ( MM_NumSelected() && MM_Pickup( qtrue ) ) {
			for ( int i = 0; i < mm.numPieces; i++ ) {
				if ( MM_Sel( &mm.pieces[i] ) )
					mm.pieces[i].scale = scale;
			}
		} else {
			mm.defScale = scale;
		}
		Com_Printf( "Model manager: scale %i%%\n", scale );
	} else if ( !Q_stricmp( arg, "solid" ) && argc >= 3 ) {
		const qboolean solid = (qboolean)( atoi( Cmd_Argv( 2 ) ) != 0 );

		if ( MM_NumSelected() && MM_Pickup( qtrue ) ) {
			for ( int i = 0; i < mm.numPieces; i++ ) {
				if ( MM_Sel( &mm.pieces[i] ) )
					mm.pieces[i].solid = solid;
			}
		} else {
			mm.defSolid = solid;
		}
		Com_Printf( "Model manager: solid %i\n", solid ? 1 : 0 );
	} else if ( !Q_stricmp( arg, "angles" ) && argc >= 5 ) {
		vec3_t angles;

		VectorSet( angles, atof( Cmd_Argv( 2 ) ), atof( Cmd_Argv( 3 ) ), atof( Cmd_Argv( 4 ) ) );
		if ( MM_NumSelected() && MM_Pickup( qtrue ) ) {
			for ( int i = 0; i < mm.numPieces; i++ ) {
				if ( MM_Sel( &mm.pieces[i] ) )
					VectorCopy( angles, mm.pieces[i].angles );
			}
		} else {
			VectorCopy( angles, mm.defAngles );
		}
	} else if ( !Q_stricmp( arg, "origin" ) && argc >= 5 ) {
		vec3_t to, pivot, delta;

		if ( MM_NumSelected() && MM_Pickup( qtrue ) ) {
			VectorSet( to, atof( Cmd_Argv( 2 ) ), atof( Cmd_Argv( 3 ) ), atof( Cmd_Argv( 4 ) ) );
			MM_Pivot( pivot );
			VectorSubtract( to, pivot, delta );
			MM_TranslateSelection( delta );
			mm.locked = qtrue;
		}
	} else {
		// anything else is a search
		if ( !MM_Open() )
			return;
		Q_strncpyz( mm.search, arg, sizeof( mm.search ) );
		mm.selRow = mm.modelScroll = 0;
		MM_RebuildView();
		MM_EnterBrowse();
	}
}

/*
===============================================================================

INPUT

===============================================================================
*/

static qboolean MM_InRect( float x, float y, float w, float h ) {
	return (qboolean)( mm.cursorX >= x && mm.cursorX < x + w && mm.cursorY >= y && mm.cursorY < y + h );
}

// binds don't run while the manager holds the keys, so honour the user's toggle bind here
static qboolean MM_IsToggleKey( int key ) {
	const char *binding;

	// a printable key belongs to the search box while browsing, and to the name being typed
	if ( ( mm.state == MM_BROWSE || mm.naming ) && key >= A_SPACE && key <= A_TILDE )
		return qfalse;
	binding = Key_GetBinding( key );
	return (qboolean)( VALIDSTRING( binding ) && ( !Q_stricmp( binding, MM_TOGGLE_CMD ) || !Q_stricmp( binding, MM_OLD_TOGGLE_CMD ) ) );
}

// the model or construct on the selected row, in hand in the free camera
static void MM_PlaceSelected( void ) {
	const int sel = MM_Selected();

	if ( sel < 0 )
		return;
	if ( mm.viewSaves ) {
		MM_LoadSave( mm.saves[sel] );
		return;
	}
	if ( mm.viewStates ) {
		// the camera shows how it goes
		MM_EnterCamera();
		MM_ReloadState( mm.states[sel] );
		return;
	}
	if ( !MM_RegisterModel( sel ) ) {
		Com_Printf( S_COLOR_YELLOW "Model manager: can't load %s/%s%s%s%s\n", MM_ROOT, mm.models[sel].path, MM_EXT,
			mm.models[sel].error ? ": " : "", mm.models[sel].error ? mm.models[sel].error : "" );
		return;
	}
	MM_EnterCamera();
	MM_StartTemplate( sel );
}

// a model starred or not any more: the Favorites row counts it, and lists it if it's the one shown
static void MM_ToggleFavorite( int model ) {
	const mmModel_t *m = &mm.models[model];
	const qboolean on = CL_Fav_Toggle( MM_FAV_LIST, m->path );
	const int selected = MM_Selected();

	MM_Message( va( on ? "Favorite: %s" : "No longer a favorite: %s", m->path ) );
	for ( int i = 0; i < mm.numRows; i++ ) {
		if ( mm.rows[i].type == MM_ROW_FAVES )
			mm.rows[i].count += on ? 1 : -1;
	}
	if ( !mm.search[0] && mm.row < mm.numRows && mm.rows[mm.row].type == MM_ROW_FAVES ) {
		MM_RebuildView();
		// the selection stays on the model it was on, if it's still listed
		for ( int i = 0; i < mm.numView; i++ ) {
			if ( mm.view[i] == selected )
				mm.selRow = i;
		}
	}
}

// the browser's lists have a scrollbar when there are more than fit
static qboolean MM_RowsBar( void ) {
	return (qboolean)( mm.numRows > MM_VISIBLE_ROWS );
}

static qboolean MM_ModelsBar( void ) {
	return (qboolean)( mm.numView > MM_VISIBLE_ROWS );
}

static float MM_SearchLabelW( void ) {
	return (float)re->Font_StrLenPixels( "Search: ", mm.font, MM_TEXT_SCALE ) + 4.0f;
}

static void MM_BrowseClick( void ) {
	int row;

	if ( ( MM_RowsBar() && MM_InRect( MM_FOLDER_X + MM_FOLDER_W - WG_SCROLL_W - 1, MM_LIST_Y, WG_SCROLL_W + 1, MM_LIST_H ) )
		|| ( MM_ModelsBar() && MM_InRect( MM_MODEL_X + MM_MODEL_W - WG_SCROLL_W - 1, MM_LIST_Y, WG_SCROLL_W + 1, MM_LIST_H ) ) ) {
		mm.barClick = qtrue;
		mm.barClickX = mm.cursorX;
		mm.barClickY = mm.cursorY;
		return;
	}
	if ( MM_InRect( MM_FOLDER_X, MM_SEARCH_Y, MM_SEARCH_W, 16 ) ) {
		WG_FieldClick( mm.search, mm.cursorX, mm.cursorY );
		return;
	}

	for ( int i = 0; i < MM_FILTER_COUNT; i++ ) {
		if ( MM_InRect( MM_FOLDER_X + MM_SEARCH_W + MM_FILTER_GAP + i * ( MM_FILTER_W + MM_FILTER_GAP ), MM_SEARCH_Y, MM_FILTER_W, 16 ) ) {
			MM_SetFilter( (mmFilter_t)i );
			return;
		}
	}

	if ( MM_InRect( MM_FOLDER_X, MM_LIST_Y, MM_FOLDER_W, MM_LIST_H ) ) {
		row = mm.rowScroll + (int)( ( mm.cursorY - MM_LIST_Y ) / MM_ROW_H );
		if ( row < mm.numRows )
			MM_SetRow( row );
		return;
	}

	if ( MM_InRect( MM_MODEL_X, MM_LIST_Y, MM_MODEL_W, MM_LIST_H ) ) {
		row = mm.modelScroll + (int)( ( mm.cursorY - MM_LIST_Y ) / MM_ROW_H );
		if ( row >= mm.numView )
			return;
		// the star: a favorite, or not any more
		if ( !mm.viewSaves && !mm.viewStates && mm.cursorX < MM_MODEL_X + 4 + MM_STAR_W ) {
			MM_ToggleFavorite( mm.view[row] );
			return;
		}
		if ( row == mm.lastClickRow && cls.realtime - mm.lastClickTime < MM_DOUBLECLICK_MS ) {
			mm.lastClickTime = 0;
			MM_PlaceSelected();
			return;
		}
		mm.selRow = row;
		mm.lastClickRow = row;
		mm.lastClickTime = cls.realtime;
		return;
	}

	if ( MM_InRect( MM_PREVIEW_X, MM_BUTTON_Y, MM_PREVIEW_W, MM_BUTTON_H ) )
		MM_PlaceSelected();
	else if ( MM_InRect( MM_PREVIEW_X, MM_BUTTON_Y + MM_BUTTON_H + 4, MM_PREVIEW_W, MM_BUTTON_H ) )
		MM_EnterCamera();
}

static qboolean MM_SearchChar( int ch ) {
	return (qboolean)( ch >= 32 && ch < 127 );
}

static void MM_BrowseChar( int ch ) {
	if ( WG_EditChar( mm.search, sizeof( mm.search ), ch, MM_SearchChar ) ) {
		mm.selRow = mm.modelScroll = 0;
		MM_RebuildView();
	}
}

static void MM_BrowseKey( int key ) {
	const qboolean ctrl = (qboolean)( Key_IsDown( A_CTRL ) || Key_IsDown( A_CTRL2 ) );
	const qboolean shift = (qboolean)( Key_IsDown( A_SHIFT ) || Key_IsDown( A_SHIFT2 ) );

	// the search box: backspace, the clipboard, and with Ctrl or Shift its cursor and selection;
	// the plain arrows, Home and End stay the lists'
	if ( key == A_BACKSPACE || ( ctrl && key != A_MOUSE1 && key != A_MWHEELUP && key != A_MWHEELDOWN )
		|| ( shift && ( key == A_INSERT || key == A_CURSOR_LEFT || key == A_CURSOR_RIGHT || key == A_HOME || key == A_END ) ) ) {
		const int edit = WG_EditKey( mm.search, sizeof( mm.search ), key, MM_SearchChar );

		if ( edit & WG_EDIT_CHANGED ) {
			mm.selRow = mm.modelScroll = 0;
			MM_RebuildView();
		}
		if ( edit )
			return;
	}

	switch ( key ) {
	case A_DELETE:
		mm.search[0] = '\0';
		MM_RebuildView();
		break;
	case A_CURSOR_UP:
		mm.selRow = Com_Clampi( 0, mm.numView - 1, mm.selRow - 1 );
		MM_ScrollToSelection();
		break;
	case A_CURSOR_DOWN:
		mm.selRow = Com_Clampi( 0, mm.numView - 1, mm.selRow + 1 );
		MM_ScrollToSelection();
		break;
	case A_PAGE_UP:
		mm.selRow = Com_Clampi( 0, mm.numView - 1, mm.selRow - MM_VISIBLE_ROWS );
		MM_ScrollToSelection();
		break;
	case A_PAGE_DOWN:
		mm.selRow = Com_Clampi( 0, mm.numView - 1, mm.selRow + MM_VISIBLE_ROWS );
		MM_ScrollToSelection();
		break;
	case A_HOME:
		mm.selRow = 0;
		MM_ScrollToSelection();
		break;
	case A_END:
		mm.selRow = mm.numView ? mm.numView - 1 : 0;
		MM_ScrollToSelection();
		break;
	case A_CURSOR_LEFT:
		MM_SetRow( mm.search[0] ? mm.row : mm.row - 1 );
		break;
	case A_CURSOR_RIGHT:
		MM_SetRow( mm.search[0] ? mm.row : mm.row + 1 );
		break;
	case A_ENTER:
	case A_KP_ENTER:
		MM_PlaceSelected();
		break;
	case A_TAB:
		MM_EnterCamera();
		break;
	case A_MOUSE1:
		MM_BrowseClick();
		break;
	case A_MWHEELUP:
	case A_MWHEELDOWN: {
		int delta = key == A_MWHEELUP ? -3 : 3;

		if ( MM_InRect( MM_FOLDER_X, MM_LIST_Y, MM_FOLDER_W, MM_LIST_H ) )
			mm.rowScroll = Com_Clampi( 0, mm.numRows > MM_VISIBLE_ROWS ? mm.numRows - MM_VISIBLE_ROWS : 0, mm.rowScroll + delta );
		else
			mm.modelScroll = Com_Clampi( 0, mm.numView > MM_VISIBLE_ROWS ? mm.numView - MM_VISIBLE_ROWS : 0, mm.modelScroll + delta );
		break;
	}
	default:
		break;
	}
}

// the name box, offering a name where there is one to offer
static void MM_GzView( gzView_t *v );

static qboolean MM_SaveNameAllow( int ch ) {
	return MM_SaveNameChar( ch );
}

static void MM_StartNaming( mmNaming_t naming ) {
	const mmPiece_t *first = MM_FirstSelected();

	if ( naming == MM_NAMING_STATE ) {
		if ( !MM_StateCount() ) {
			MM_Message( S_COLOR_YELLOW "Nothing to save in a state: no models were placed or named yet" );
			return;
		}
		// the map's name, as a file name
		Q_strncpyz( mm.saveName, COM_SkipPath( (char *)MM_MapName() ), sizeof( mm.saveName ) );
	} else if ( !first ) {
		MM_Message( naming == MM_NAMING_TAG ? S_COLOR_YELLOW "Select the models to name first" : S_COLOR_YELLOW "Select the models to save first" );
		return;
	} else if ( naming == MM_NAMING_TAG && first->tag[0] ) {
		Q_strncpyz( mm.saveName, first->tag, sizeof( mm.saveName ) );
	} else if ( MM_CarryingTemplate() ) {
		Q_strncpyz( mm.saveName, mm.tmplName, sizeof( mm.saveName ) );
	} else {
		mm.saveName[0] = '\0';
	}
	for ( char *c = mm.saveName; *c; c++ ) {
		if ( !MM_SaveNameChar( *c ) )
			*c = '_';
	}
	mm.naming = naming;
	MM_ReleaseKeys();
}

static void MM_NameKey( int key ) {
	if ( WG_EditKey( mm.saveName, sizeof( mm.saveName ), key, MM_SaveNameAllow ) )
		return;
	switch ( key ) {
	case A_ENTER:
	case A_KP_ENTER:
		if ( mm.naming == MM_NAMING_TAG ) {
			MM_NameAndSend( mm.saveName );
			mm.naming = MM_NAMING_NONE;
		} else if ( mm.naming == MM_NAMING_STATE ? MM_SaveState( mm.saveName ) : MM_SaveSelection( mm.saveName ) ) {
			mm.naming = MM_NAMING_NONE;
		}
		break;
	default:
		break;
	}
}

static void MM_CameraKey( int key ) {
	// the arrows: one grabbed moves the selection along it; the right button only looks
	if ( key == GZ_KEY ) {
		mm.gizmo = (qboolean)!mm.gizmo;
		mm.gzDragging = qfalse;
		mm.cursorX = SCREEN_WIDTH * 0.5f;
		mm.cursorY = SCREEN_HEIGHT * 0.5f;
		MM_Message( mm.gizmo ? "Move arrows: drag X, Y or Z to move the selection along it; hold the right mouse button to look; J hides them"
			: "Move arrows hidden" );
		return;
	}
	if ( mm.gizmo && key == A_MOUSE2 )
		return;
	if ( mm.gizmo && key == A_MOUSE1 && mm.gzHot >= 0 && MM_NumSelected() && MM_Pickup( qtrue ) ) {
		gzView_t v;
		vec3_t pivot;

		MM_GzView( &v );
		MM_Pivot( pivot );
		GZ_Begin( &mm.gzDrag, &v, pivot, mm.gzHot, mm.cursorX, mm.cursorY );
		mm.gzDragging = mm.gzDrag.on;
		mm.locked = qtrue;
		return;
	}
	switch ( key ) {
	case A_MOUSE1:
		if ( mm.carrying )
			MM_Drop();
		else
			MM_ClickSelect();
		break;
	case A_ENTER:
	case A_KP_ENTER:
		if ( MM_ShiftDown() ) {
			MM_StartNaming( MM_NAMING_TAG );
			break;
		}
		MM_Drop();
		if ( !MM_SendAll() && !mm.carrying )
			MM_Message( "Nothing to send" );
		break;
	case A_MOUSE2:
	case A_CAP_F:
		if ( mm.carrying )
			mm.locked = (qboolean)!mm.locked;
		else
			MM_Pickup( qfalse );
		break;
	case A_MWHEELUP:
	case A_MWHEELDOWN:
		// held in front: Ctrl and the wheel bring it nearer or push it away
		if ( MM_CtrlDown() && cl_modelFollowFront->integer )
			MM_ChangeHoldDistance( key == A_MWHEELUP ? 1 : -1 );
		else
			MM_Rotate( YAW, key == A_MWHEELUP ? 1 : -1 );
		break;
	case A_CAP_P:
		MM_ToggleFollowFront();
		break;
	case A_CAP_Z:		MM_Rotate( PITCH, -1 );	break;
	case A_CAP_X:		MM_Rotate( PITCH, 1 );	break;
	case A_CAP_Q:		MM_Rotate( ROLL, -1 );	break;
	case A_CAP_E:		MM_Rotate( ROLL, 1 );	break;
	case A_CAP_R:
		MM_ResetAngles( qfalse );
		break;
	case A_BACKSPACE:
		MM_ResetAngles( qtrue );
		break;
	case A_OPEN_SQUARE:
	case A_MINUS:
	case A_KP_MINUS:
		MM_ChangeScale( -1 );
		break;
	case A_CLOSE_SQUARE:
	case A_EQUALS:
	case A_PLUS:
	case A_KP_PLUS:
		MM_ChangeScale( 1 );
		break;
	case A_CAP_T:
		MM_ToggleSolid();
		break;
	case A_CAP_G:
		mm.snap = ( mm.snap + 1 ) % ARRAY_LEN( mmSnaps );
		break;
	case A_CAP_H:
		mm.hideHelp = (qboolean)!mm.hideHelp;
		break;
	case A_CAP_B:
		mm.hideBoxes = (qboolean)!mm.hideBoxes;
		MM_Message( mm.hideBoxes ? "Outlines off: hold O or the middle mouse button to see the one you aim at" : "Outlines on" );
		break;
	case A_CURSOR_UP:		MM_Nudge( 1, 0, 0 );	break;
	case A_CURSOR_DOWN:		MM_Nudge( -1, 0, 0 );	break;
	case A_CURSOR_RIGHT:	MM_Nudge( 0, 1, 0 );	break;
	case A_CURSOR_LEFT:		MM_Nudge( 0, -1, 0 );	break;
	case A_PAGE_UP:			MM_Nudge( 0, 0, 1 );	break;
	case A_PAGE_DOWN:		MM_Nudge( 0, 0, -1 );	break;
	case A_DELETE:
		MM_DeleteSelected();
		break;
	case A_CAP_V:
		MM_CopySelection();
		break;
	case A_CAP_U:
		MM_Undo();
		break;
	case A_CAP_K:
		MM_StartNaming( MM_ShiftDown() ? MM_NAMING_STATE : MM_NAMING_CONSTRUCT );
		break;
	case A_CAP_I:
		mm.instant = (qboolean)!mm.instant;
		break;
	case A_CAP_L:
		MM_RequestList();
		break;
	case A_TAB:
		MM_EnterBrowse();
		break;
	default:
		break;
	}
}

qboolean CL_ModelManager_Active( void ) {
	return (qboolean)( mm.state != MM_OFF );
}

void CL_ModelManager_KeyEvent( int key, qboolean down ) {
	if ( mm.state == MM_OFF )
		return;

	if ( key >= 0 && key < MAX_KEYS )
		mm.held[key] = down;
	// an arrow let go: what's moved goes down, a model being placed stays in hand
	if ( !down && key == A_MOUSE1 && mm.gzDragging ) {
		mm.gzDragging = qfalse;
		if ( !MM_CarryingTemplate() )
			MM_Drop();
		return;
	}
	if ( !down )
		return;

	if ( MM_IsToggleKey( key ) ) {
		MM_Close();
		return;
	}

	if ( mm.state == MM_BROWSE )
		MM_BrowseKey( key );
	else if ( mm.naming )
		MM_NameKey( key );
	else
		MM_CameraKey( key );
}

void CL_ModelManager_CharEvent( int ch ) {
	if ( mm.state == MM_BROWSE ) {
		MM_BrowseChar( ch );
	} else if ( mm.naming ) {
		WG_EditChar( mm.saveName, sizeof( mm.saveName ), ch, MM_SaveNameAllow );
	}
}

void CL_ModelManager_MouseEvent( int dx, int dy ) {
	if ( mm.state == MM_BROWSE ) {
		mm.cursorX = Com_Clamp( 0, SCREEN_WIDTH, mm.cursorX + dx );
		mm.cursorY = Com_Clamp( 0, SCREEN_HEIGHT, mm.cursorY + dy );
	} else if ( mm.state == MM_CAMERA && mm.gizmo && !mm.held[A_MOUSE2] ) {
		mm.cursorX = Com_Clamp( 0, SCREEN_WIDTH, mm.cursorX + dx );
		mm.cursorY = Com_Clamp( 0, SCREEN_HEIGHT, mm.cursorY + dy );
	} else if ( mm.state == MM_CAMERA ) {
		mm.camAng[YAW] -= dx * cl_sensitivity->value * m_yaw->value;
		mm.camAng[PITCH] = Com_Clamp( -89.0f, 89.0f, mm.camAng[PITCH] + dy * cl_sensitivity->value * m_pitch->value );
	}
}

// escape backs out one level: the name being typed, what is in hand, the camera, the browser
void CL_ModelManager_Escape( void ) {
	if ( mm.state == MM_CAMERA ) {
		if ( mm.naming ) {
			mm.naming = MM_NAMING_NONE;
		} else if ( mm.carrying && !MM_CarryingTemplate() ) {
			MM_CancelCarry();
		} else {
			MM_EnterBrowse();
		}
	} else if ( mm.state == MM_BROWSE ) {
		MM_Close();
	}
}

/*
===============================================================================

3D VIEW

===============================================================================
*/

static void MM_UpdateCamera( float dt ) {
	vec3_t forward, right, move;
	float speed;

	if ( mm.naming )
		return;
	AngleVectors( mm.camAng, forward, right, NULL );
	VectorClear( move );
	if ( mm.held[A_CAP_W] )		VectorAdd( move, forward, move );
	if ( mm.held[A_CAP_S] )		VectorSubtract( move, forward, move );
	if ( mm.held[A_CAP_D] )		VectorAdd( move, right, move );
	if ( mm.held[A_CAP_A] )		VectorSubtract( move, right, move );
	if ( mm.held[A_SPACE] )		move[2] += 1.0f;
	if ( mm.held[A_CAP_C] )		move[2] -= 1.0f;

	if ( VectorNormalize( move ) > 0.0f ) {
		speed = MM_FLY_SPEED;
		if ( MM_ShiftDown() )
			speed *= 3.0f;
		if ( MM_CtrlDown() )
			speed *= 0.25f;
		VectorMA( mm.camOrg, speed * dt, move, mm.camOrg );
	}
}

// put what is in hand where the crosshair meets the world, resting on that surface; in front of
// the camera instead, when that's further than it's held
static void MM_FollowCrosshair( void ) {
	vec3_t forward, end, normal, corner, mins, maxs, pivot, to, delta;
	matrix3_t axis;
	trace_t tr;
	float minDot = 0.0f, d;
	qboolean first = qtrue;
	int grid = mmSnaps[mm.snap].grid;

	MM_Pivot( pivot );
	AngleVectors( mm.camAng, forward, NULL, NULL );
	VectorMA( mm.camOrg, MM_TRACE_DIST, forward, end );
	CM_BoxTrace( &tr, mm.camOrg, end, vec3_origin, vec3_origin, 0, MM_TRACE_MASK, qfalse );

	if ( cl_modelFollowFront->integer && !tr.startsolid && tr.fraction * MM_TRACE_DIST > MM_HoldDistance() ) {
		VectorMA( mm.camOrg, MM_HoldDistance(), forward, to );
		for ( int i = 0; i < 3; i++ )
			to[i] = MM_SnapTo( to[i], grid );
	} else if ( tr.fraction < 1.0f && !tr.startsolid && !( tr.surfaceFlags & SURF_SKY ) ) {
		VectorCopy( tr.plane.normal, normal );

		// the lowest corner of them all, along the surface's normal
		for ( int i = 0; i < mm.numPieces; i++ ) {
			const mmPiece_t *p = &mm.pieces[i];

			if ( !MM_Sel( p ) )
				continue;
			MM_ModelBounds( p->hModel, mins, maxs );
			MM_BuildAxis( p->angles, p->scale, axis );
			for ( int c = 0; c < 8; c++ ) {
				MM_Corner( mins, maxs, axis, c, corner );
				VectorAdd( corner, p->origin, corner );
				VectorSubtract( corner, pivot, corner );
				d = DotProduct( corner, normal );
				if ( first || d < minDot )
					minDot = d;
				first = qfalse;
			}
		}
		VectorMA( tr.endpos, -minDot, normal, to );

		// snap along the surface, keep the offset that makes it rest on it
		for ( int i = 0; i < 3; i++ ) {
			if ( fabsf( normal[i] ) < 0.7f )
				to[i] = MM_SnapTo( to[i], grid );
		}
	} else {
		VectorMA( mm.camOrg, MM_NO_HIT_DIST, forward, to );
		for ( int i = 0; i < 3; i++ )
			to[i] = MM_SnapTo( to[i], grid );
	}
	VectorSubtract( to, pivot, delta );
	MM_TranslateSelection( delta );
}

// how far along the ray it enters the turned, scaled box; -1 if it misses or starts inside
static float MM_RayBox( const vec3_t start, const vec3_t dir, const vec3_t origin, const vec3_t angles, int scale,
						const vec3_t mins, const vec3_t maxs ) {
	const float s = scale / 100.0f;
	float tmin = 0.0f, tmax = MM_TRACE_DIST, o, v, t1, t2;
	qboolean inside = qtrue;
	matrix3_t axis;
	vec3_t d;

	AnglesToAxis( angles, axis );
	VectorSubtract( start, origin, d );
	for ( int i = 0; i < 3; i++ ) {
		o = DotProduct( d, axis[i] ) / s;
		v = DotProduct( dir, axis[i] ) / s;
		if ( o < mins[i] || o > maxs[i] )
			inside = qfalse;
		if ( fabsf( v ) < 1e-6f ) {
			if ( o < mins[i] || o > maxs[i] )
				return -1.0f;
			continue;
		}
		t1 = ( mins[i] - o ) / v;
		t2 = ( maxs[i] - o ) / v;
		if ( t1 > t2 ) {
			float t = t1;
			t1 = t2;
			t2 = t;
		}
		tmin = Q_max( tmin, t1 );
		tmax = Q_min( tmax, t2 );
		if ( tmin > tmax )
			return -1.0f;
	}
	// one the camera is in would always be in the way
	return inside ? -1.0f : tmin;
}

static void MM_GzView( gzView_t *v ) {
	GZ_ViewFromAngles( v, mm.camOrg, mm.camAng, mm.fovX > 0.0f ? mm.fovX : 90.0f, mm.fovY > 0.0f ? mm.fovY : 73.74f );
}

// the way the mouse points, from the camera
static void MM_CursorDir( vec3_t dir ) {
	const float fx = mm.fovX > 0.0f ? mm.fovX : 90.0f, fy = mm.fovY > 0.0f ? mm.fovY : 73.74f;
	const float x = ( mm.cursorX / ( SCREEN_WIDTH * 0.5f ) - 1.0f ) * tanf( DEG2RAD( fx * 0.5f ) );
	const float y = ( mm.cursorY / ( SCREEN_HEIGHT * 0.5f ) - 1.0f ) * tanf( DEG2RAD( fy * 0.5f ) );
	vec3_t axis[3];

	AnglesToAxis( mm.camAng, axis );
	VectorCopy( axis[0], dir );
	VectorMA( dir, -x, axis[1], dir );
	VectorMA( dir, -y, axis[2], dir );
	VectorNormalize( dir );
}

// the model along forward from the camera and in front of the world; -1 = none
static int MM_PieceAlong( const vec3_t forward ) {
	vec3_t end, origin, angles, mins, maxs;
	trace_t tr;
	float best, d;
	int found = -1, scale;

	VectorMA( mm.camOrg, MM_TRACE_DIST, forward, end );
	CM_BoxTrace( &tr, mm.camOrg, end, vec3_origin, vec3_origin, 0, MM_TRACE_MASK, qfalse );
	best = tr.fraction * MM_TRACE_DIST + 4.0f;	// models often sit a little in the floor or a wall

	for ( int i = 0; i < mm.numPieces; i++ ) {
		const mmPiece_t *p = &mm.pieces[i];

		// nor what is in hand, which is always under it
		if ( p->sent || p->tmpl || ( mm.carrying && MM_Sel( p ) ) )
			continue;
		MM_Shown( p, origin, angles, &scale );
		MM_ModelBounds( p->hModel, mins, maxs );
		d = MM_RayBox( mm.camOrg, forward, origin, angles, scale, mins, maxs );
		if ( d >= 0.0f && d < best ) {
			best = d;
			found = i;
		}
	}
	return found;
}

// the model under the crosshair, or under the mouse with the arrows up; -1 = none
static int MM_PieceUnderCrosshair( void ) {
	vec3_t forward;

	if ( mm.gizmo )
		MM_CursorDir( forward );
	else
		AngleVectors( mm.camAng, forward, NULL, NULL );
	return MM_PieceAlong( forward );
}

// the arrow held: the selection goes where it says along its axis
static void MM_GizmoFrame( void ) {
	gzView_t v;
	vec3_t pivot, to, delta;

	mm.gzHot = -1;
	if ( !mm.gizmo || !MM_NumSelected() )
		return;
	MM_GzView( &v );
	MM_Pivot( pivot );
	if ( mm.gzDragging ) {
		GZ_Target( &mm.gzDrag, mm.cursorX, mm.cursorY, mmSnaps[mm.snap].grid, to );
		VectorSubtract( to, pivot, delta );
		if ( VectorLengthSquared( delta ) > 0.0001f )
			MM_TranslateSelection( delta );
		mm.gzHot = mm.gzDrag.axis;
		return;
	}
	mm.gzHot = GZ_Hit( &v, pivot, mm.cursorX, mm.cursorY );
}

// called every frame before the cgame draws
void CL_ModelManager_Frame( void ) {
	float dt;

	// queued commands go out even with the manager closed
	MM_RunQueue();
	MM_CheckListTimeout();
	MM_CheckReload();

	if ( mm.state == MM_OFF )
		return;

	if ( !( Key_GetCatcher() & KEYCATCH_MODELMANAGER ) ) {
		// something else cleared the catchers
		MM_CancelCarry();
		mm.naming = MM_NAMING_NONE;
		mm.state = MM_OFF;
		MM_ReleaseKeys();
		return;
	}

	for ( int i = 0; i < mm.numPieces; i++ )
		mm.pieces[i].hModel = MM_RegisterModel( mm.pieces[i].model );
	MM_SyncFromSnapshot();

	dt = Com_Clamp( 0.0f, 0.1f, ( cls.realtime - mm.lastFrameTime ) / 1000.0f );
	mm.lastFrameTime = cls.realtime;

	if ( mm.state == MM_CAMERA ) {
		MM_UpdateCamera( dt );
		MM_GizmoFrame();
		if ( mm.carrying && !mm.locked && !mm.gzDragging )
			MM_FollowCrosshair();
		mm.hover = mm.carrying ? -1 : MM_PieceUnderCrosshair();
		mm.peek = ( mm.hideBoxes && !mm.naming && ( mm.held[A_CAP_O] || mm.held[A_MOUSE3] ) ) ? MM_PieceUnderCrosshair() : -1;
	}
}

static void MM_AddEdge( const vec3_t a, const vec3_t b, const byte *color ) {
	polyVert_t verts[4], back[4];
	vec3_t dir, toCam, side, mid;
	float width;

	VectorSubtract( b, a, dir );
	VectorAdd( a, b, mid );
	VectorScale( mid, 0.5f, mid );
	VectorSubtract( mm.camOrg, mid, toCam );
	width = 0.4f + VectorLength( toCam ) * 0.0015f;
	CrossProduct( dir, toCam, side );
	if ( VectorNormalize( side ) <= 0.0f )
		return;
	VectorScale( side, width, side );

	VectorAdd( a, side, verts[0].xyz );
	VectorAdd( b, side, verts[1].xyz );
	VectorSubtract( b, side, verts[2].xyz );
	VectorSubtract( a, side, verts[3].xyz );
	for ( int i = 0; i < 4; i++ ) {
		verts[i].st[0] = ( i == 1 || i == 2 ) ? 1.0f : 0.0f;
		verts[i].st[1] = ( i >= 2 ) ? 1.0f : 0.0f;
		memcpy( verts[i].modulate, color, sizeof( verts[i].modulate ) );
	}
	// both windings, the white shader is single sided
	for ( int i = 0; i < 4; i++ )
		back[i] = verts[3 - i];
	re->AddPolyToScene( cls.whiteShader, 4, verts, 1 );
	re->AddPolyToScene( cls.whiteShader, 4, back, 1 );
}

static void MM_AddBox( qhandle_t h, const vec3_t origin, const vec3_t angles, int scale, const byte *color ) {
	static const int edges[12][2] = {
		{ 0, 1 }, { 2, 3 }, { 4, 5 }, { 6, 7 },
		{ 0, 2 }, { 1, 3 }, { 4, 6 }, { 5, 7 },
		{ 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 },
	};
	vec3_t corners[8], mins, maxs;
	matrix3_t axis;

	MM_ModelBounds( h, mins, maxs );
	MM_BuildAxis( angles, scale, axis );
	for ( int i = 0; i < 8; i++ ) {
		MM_Corner( mins, maxs, axis, i, corners[i] );
		VectorAdd( corners[i], origin, corners[i] );
	}
	for ( int i = 0; i < 12; i++ )
		MM_AddEdge( corners[edges[i][0]], corners[edges[i][1]], color );
}

static void MM_AddModel( qhandle_t h, const vec3_t origin, const vec3_t angles, int scale ) {
	refEntity_t ent;

	if ( !h )
		return;
	memset( &ent, 0, sizeof( ent ) );
	ent.hModel = h;
	VectorCopy( origin, ent.origin );
	VectorCopy( origin, ent.oldorigin );
	VectorCopy( origin, ent.lightingOrigin );
	MM_BuildAxis( angles, scale, ent.axis );
	ent.nonNormalizedAxes = (qboolean)( scale != 100 );
	ent.renderfx = RF_NOSHADOW;
	re->AddRefEntityToScene( &ent );
}

// what the cgame doesn't draw, and boxes on what is chosen
static void MM_AddPieces( void ) {
	const qboolean flash = (qboolean)( cls.realtime - mm.sentTime < 300 );

	for ( int i = 0; i < mm.numPieces; i++ ) {
		const mmPiece_t *p = &mm.pieces[i];
		// with the outlines off, the one peeked at shows what it is rather than that it's aimed at
		const qboolean hovered = (qboolean)( i == mm.hover && !mm.hideBoxes );
		const byte *color;

		if ( mm.hideBoxes && i != mm.peek ) {
			// what the cgame doesn't draw still has to be
			if ( MM_IsNew( p ) || ( !p->deleted && ( MM_HideOriginal( p ) || ( p->sent && p->known ) ) ) )
				MM_AddModel( p->hModel, p->origin, p->angles, p->scale );
			continue;
		}

		if ( MM_IsNew( p ) ) {
			MM_AddModel( p->hModel, p->origin, p->angles, p->scale );
			if ( p->sent || ( p->tmpl && flash ) )
				color = mmBoxSent;
			else if ( hovered )
				color = mmBoxHover;
			else if ( MM_Sel( p ) )
				color = mmBoxSelected;
			else
				color = p->solid ? mmBoxSolid : mmBoxNonSolid;
			MM_AddBox( p->hModel, p->origin, p->angles, p->scale, color );
		} else if ( p->deleted ) {
			// the cgame still draws it, until the server takes it away
			MM_AddBox( p->hModel, p->srvOrigin, p->srvAngles, p->srvScale, hovered ? mmBoxHover : p->sent ? mmBoxSent : mmBoxDeleted );
		} else if ( MM_HideOriginal( p ) || ( p->sent && p->known ) ) {
			MM_AddModel( p->hModel, p->origin, p->angles, p->scale );
			color = p->sent ? mmBoxSent : hovered ? mmBoxHover : MM_Sel( p ) ? mmBoxSelected : mmBoxMoved;
			MM_AddBox( p->hModel, p->origin, p->angles, p->scale, color );
		} else if ( !p->known ) {
			// not in the snapshot, so not drawn: a box where the list says it is
			MM_AddBox( p->hModel, p->origin, p->angles, p->scale, hovered ? mmBoxHover : MM_Sel( p ) ? mmBoxSelected : mmBoxUnknown );
		} else if ( hovered || MM_Sel( p ) ) {
			MM_AddBox( p->hModel, p->origin, p->angles, p->scale, hovered ? mmBoxHover : mmBoxSelected );
		}
	}
}

// the free camera, for the effects system to cull against
qboolean CL_ModelManager_Camera( vec3_t origin, vec3_t angles ) {
	if ( mm.state == MM_OFF )
		return qfalse;
	VectorCopy( mm.camOrg, origin );
	VectorCopy( mm.camAng, angles );
	return qtrue;
}

// every cgame entity passes through here: the view weapon would float where the
// player stands, and a server model being moved is drawn where it's going instead
qboolean CL_ModelManager_FilterEntity( const refEntity_t *ent ) {
	if ( mm.state == MM_OFF )
		return qfalse;
	if ( ent->renderfx & RF_FIRST_PERSON )
		return qtrue;
	if ( ent->reType != RT_MODEL || !ent->hModel )
		return qfalse;
	for ( int i = 0; i < mm.numPieces; i++ ) {
		const mmPiece_t *p = &mm.pieces[i];

		if ( p->hModel == ent->hModel && ( MM_HideOriginal( p ) || ( p->sent && !p->deleted && !MM_IsNew( p ) && p->known ) )
			&& DistanceSquared( ent->origin, p->srvOrigin ) < MM_MATCH_DIST * MM_MATCH_DIST )
			return qtrue;
	}
	return qfalse;
}

// every cgame scene passes through here before reaching the renderer
void CL_ModelManager_RenderScene( const refdef_t *fd ) {
	refdef_t view;

	if ( fd->rdflags & ( RDF_NOWORLDMODEL | RDF_AUTOMAP ) ) {
		re->RenderScene( fd );
		return;
	}

	if ( fd->rdflags & RDF_SKYBOXPORTAL ) {
		// the sky portal keeps its own origin but looks the way the camera does
		if ( mm.state != MM_OFF ) {
			view = *fd;
			AnglesToAxis( mm.camAng, view.viewaxis );
			VectorCopy( mm.camAng, view.viewangles );
			re->RenderScene( &view );
			return;
		}
		re->RenderScene( fd );
		return;
	}

	// the main view
	if ( mm.state == MM_OFF ) {
		VectorCopy( fd->vieworg, mm.viewOrg );
		vectoangles( fd->viewaxis[0], mm.viewAng );
		mm.haveView = qtrue;
		re->RenderScene( fd );
		return;
	}

	if ( mm.viewFrame == cls.framecount ) {
		re->RenderScene( fd );
		return;
	}
	mm.viewFrame = cls.framecount;

	view = *fd;
	VectorCopy( mm.camOrg, view.vieworg );
	VectorCopy( mm.camAng, view.viewangles );
	AnglesToAxis( mm.camAng, view.viewaxis );
	mm.fovX = view.fov_x;
	mm.fovY = view.fov_y;
	view.viewContents = CM_PointContents( mm.camOrg, 0 );
	// the snapshot's area mask is from the player's position, the camera can be anywhere
	memset( view.areamask, 0, sizeof( view.areamask ) );

	if ( mm.state == MM_CAMERA )
		MM_AddPieces();

	re->RenderScene( &view );
}

/*
===============================================================================

2D

===============================================================================
*/

static void MM_Fill( float x, float y, float w, float h, const float *color ) {
	re->SetColor( color );
	re->DrawStretchPic( x, y, w, h, 0, 0, 0, 0, cls.whiteShader );
	re->SetColor( NULL );
}

static void MM_Box( float x, float y, float w, float h, const float *fill ) {
	MM_Fill( x, y, w, h, fill );
	MM_Fill( x, y, w, 1, mmBorder );
	MM_Fill( x, y + h - 1, w, 1, mmBorder );
	MM_Fill( x, y, 1, h, mmBorder );
	MM_Fill( x + w - 1, y, 1, h, mmBorder );
}

static void MM_Text( float x, float y, const char *text, const float *color ) {
	re->Font_DrawString( (int)x, (int)y, text, color, mm.font | STYLE_DROPSHADOW, -1, MM_TEXT_SCALE );
}

static void MM_TextClipped( float x, float y, float w, const char *text, const float *color ) {
	char buf[MAX_STRING_CHARS];
	int len;

	Q_strncpyz( buf, text, sizeof( buf ) );
	len = strlen( buf );
	while ( len > 3 && re->Font_StrLenPixels( buf, mm.font, MM_TEXT_SCALE ) > w ) {
		len--;
		buf[len - 3] = '.';
		buf[len - 2] = '.';
		buf[len - 1] = '.';
		buf[len] = '\0';
	}
	MM_Text( x, y, buf, color );
}

static void MM_FieldStyle( wgStyle_t *style ) {
	style->font = mm.font;
	style->scale = MM_TEXT_SCALE;
	style->lineH = MM_ROW_H;
	style->padX = 4.0f;
	style->padY = 3.0f;
	style->fill = mmPanelOpaque;
	style->border = mmBorder;
	style->text = mmWhite;
	style->dim = mmDim;
}

// clicks on it are handled in MM_BrowseClick
static void MM_Button( float x, float y, float w, const char *label ) {
	MM_Box( x, y, w, MM_BUTTON_H, MM_InRect( x, y, w, MM_BUTTON_H ) ? mmHighlight : mmPanelLight );
	MM_Text( x + 8, y + 5, label, mmWhite );
}

// models spinning in the preview box, laid out as given
static void MM_DrawPreviewOf( const mmSavedPiece_t *pieces, int count ) {
	refdef_t refdef;
	refEntity_t ent;
	vec3_t mins, maxs, bmins, bmaxs, corner, center, forward, angles, offset;
	matrix3_t axis;
	float radius, fovY, dist, xScale, yScale, spin;
	qhandle_t h;
	int drawn = 0;

	// the bounds of all of it
	ClearBounds( mins, maxs );
	for ( int i = 0; i < count; i++ ) {
		h = MM_RegisterModel( MM_FindOrAddModel( pieces[i].name ) );
		if ( !h )
			continue;
		MM_ModelBounds( h, bmins, bmaxs );
		MM_BuildAxis( pieces[i].angles, pieces[i].scale, axis );
		for ( int c = 0; c < 8; c++ ) {
			MM_Corner( bmins, bmaxs, axis, c, corner );
			VectorAdd( corner, pieces[i].offset, corner );
			AddPointToBounds( corner, mins, maxs );
		}
		drawn++;
	}
	if ( !drawn ) {
		MM_Text( MM_PREVIEW_X + 8, MM_PREVIEW_Y + 8, "Can't load model", mmRed );
		return;
	}
	VectorAdd( mins, maxs, center );
	VectorScale( center, 0.5f, center );
	radius = Q_max( 1.0f, Distance( mins, maxs ) * 0.5f );

	xScale = cls.glconfig.vidWidth / (float)SCREEN_WIDTH;
	yScale = cls.glconfig.vidHeight / (float)SCREEN_HEIGHT;

	memset( &refdef, 0, sizeof( refdef ) );
	refdef.x = (int)( ( MM_PREVIEW_X + 1 ) * xScale );
	refdef.y = (int)( ( MM_PREVIEW_Y + 1 ) * yScale );
	refdef.width = (int)( ( MM_PREVIEW_W - 2 ) * xScale );
	refdef.height = (int)( ( MM_PREVIEW_H - 2 ) * yScale );
	fovY = 40.0f;
	refdef.fov_y = fovY;
	refdef.fov_x = RAD2DEG( 2.0f * atanf( tanf( DEG2RAD( fovY * 0.5f ) ) * refdef.width / (float)refdef.height ) );
	if ( refdef.fov_x < fovY )
		fovY = refdef.fov_x;
	dist = radius / sinf( DEG2RAD( fovY * 0.5f ) );

	VectorSet( angles, 20.0f, 180.0f, 0 );
	AngleVectors( angles, forward, NULL, NULL );
	VectorMA( vec3_origin, -dist, forward, refdef.vieworg );
	AnglesToAxis( angles, refdef.viewaxis );
	refdef.rdflags = RDF_NOWORLDMODEL;
	refdef.time = cl.serverTime;

	// one turn every 10 seconds, about the middle; yaw is the outermost turn, so it just adds
	spin = (float)( cls.realtime % 10000 ) * 0.036f;
	re->ClearScene();
	for ( int i = 0; i < count; i++ ) {
		h = MM_RegisterModel( MM_FindOrAddModel( pieces[i].name ) );
		if ( !h )
			continue;
		memset( &ent, 0, sizeof( ent ) );
		ent.hModel = h;
		ent.renderfx = RF_NOSHADOW;
		VectorSubtract( pieces[i].offset, center, offset );
		RotatePointAroundVector( ent.origin, axisDefault[2], offset, spin );
		VectorCopy( pieces[i].angles, angles );
		angles[YAW] += spin;
		MM_BuildAxis( angles, pieces[i].scale, ent.axis );
		ent.nonNormalizedAxes = (qboolean)( pieces[i].scale != 100 );
		VectorCopy( ent.origin, ent.oldorigin );
		VectorCopy( ent.origin, ent.lightingOrigin );
		re->AddRefEntityToScene( &ent );
	}
	re->RenderScene( &refdef );
}

static void MM_DrawPreview( int sel ) {
	mmSavedPiece_t one;

	MM_Box( MM_PREVIEW_X, MM_PREVIEW_Y, MM_PREVIEW_W, MM_PREVIEW_H, mmPanelLight );
	if ( sel < 0 )
		return;

	if ( mm.viewStates ) {
		char buf[MAX_STRING_CHARS], *line, *next;
		float y = MM_PREVIEW_Y + 6;

		MM_ReadPreviewState( sel );
		Q_strncpyz( buf, mm.stateSummary, sizeof( buf ) );
		for ( line = buf; line && *line; line = next, y += MM_ROW_H ) {
			next = strchr( line, '\n' );
			if ( next )
				*next++ = '\0';
			MM_TextClipped( MM_PREVIEW_X + 8, y, MM_PREVIEW_W - 16, line, mmWhite );
		}
		return;
	}

	if ( mm.viewSaves ) {
		MM_ReadPreviewSave( sel );
		if ( mm.numSavePieces <= 0 )
			MM_Text( MM_PREVIEW_X + 8, MM_PREVIEW_Y + 8, "No models in this file", mmRed );
		else
			MM_DrawPreviewOf( mm.savePieces, mm.numSavePieces );
		return;
	}

	if ( !MM_RegisterModel( sel ) ) {
		MM_Text( MM_PREVIEW_X + 8, MM_PREVIEW_Y + 8, "Can't load model", mmRed );
		if ( mm.models[sel].error )
			MM_Text( MM_PREVIEW_X + 8, MM_PREVIEW_Y + 8 + MM_ROW_H, mm.models[sel].error, mmRed );
		return;
	}
	memset( &one, 0, sizeof( one ) );
	Q_strncpyz( one.name, mm.models[sel].path, sizeof( one.name ) );
	one.scale = 100;
	MM_DrawPreviewOf( &one, 1 );
}

static void MM_DrawRows( void ) {
	float rw = MM_FOLDER_W;

	MM_Box( MM_FOLDER_X, MM_LIST_Y, MM_FOLDER_W, MM_LIST_H, mmPanelLight );
	if ( WG_Scrollbar( MM_FOLDER_X + MM_FOLDER_W - WG_SCROLL_W - 1, MM_LIST_Y + 1, MM_LIST_H - 2, mm.numRows, MM_VISIBLE_ROWS, &mm.rowScroll,
		mm.cursorX, mm.cursorY, &mm.barClick, mm.barClickX, mm.barClickY ) )
		rw -= WG_SCROLL_W + 1;
	for ( int i = 0; i < MM_VISIBLE_ROWS; i++ ) {
		const int row = mm.rowScroll + i;
		const mmRow_t *r;
		const float y = MM_LIST_Y + i * MM_ROW_H;
		float indent = 0;
		const char *label;

		if ( row >= mm.numRows )
			break;
		r = &mm.rows[row];
		if ( row == mm.row && !mm.search[0] )
			MM_Fill( MM_FOLDER_X + 1, y, rw - 2, MM_ROW_H, mmHighlight );
		else if ( MM_InRect( MM_FOLDER_X, y, rw, MM_ROW_H ) )
			MM_Fill( MM_FOLDER_X + 1, y, rw - 2, MM_ROW_H, mmHover );

		switch ( r->type ) {
		case MM_ROW_SAVED:
		case MM_ROW_STATES:
			label = va( S_COLOR_CYAN "%s " S_COLOR_GREY "(%i)", r->name, r->count );
			break;
		case MM_ROW_FAVES:
			label = va( S_COLOR_YELLOW "* " S_COLOR_WHITE "%s " S_COLOR_GREY "(%i)", r->name, r->count );
			break;
		case MM_ROW_CATEGORY:
			label = va( S_COLOR_YELLOW "%s " S_COLOR_GREY "(%i)", r->name, r->count );
			break;
		case MM_ROW_SOURCE: {
			const char *slash = strrchr( r->name, '/' );
			int len = strlen( r->name );

			indent = 8;
			if ( len > 4 && !Q_stricmp( r->name + len - 4, ".pk3" ) )
				label = va( S_COLOR_GREEN "%s " S_COLOR_GREY "(%i)", slash ? slash + 1 : r->name, r->count );
			else
				label = va( S_COLOR_GREEN "%s folder " S_COLOR_GREY "(%i)", r->name, r->count );
			break;
		}
		default:
			// under a pk3 when it's a custom one
			indent = ( r->first < mm.numListed && mm.models[r->first].custom ) ? 16 : 8;
			label = va( "%s " S_COLOR_GREY "(%i)", MM_FolderLabel( r->name ), r->count );
			break;
		}
		MM_TextClipped( MM_FOLDER_X + 4 + indent, y + 1, rw - 8 - indent, label, mm.search[0] ? mmDim : mmWhite );
	}
}

static void MM_DrawBrowser( void ) {
	char buf[MAX_STRING_CHARS];
	int row, sel = MM_Selected(), added, moved, deleted;
	float y;

	MM_Fill( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, mmPanel );

	MM_Text( MM_FOLDER_X, 16, va( "Model Manager  " S_COLOR_GREY "%i models", mm.numListed ), mmWhite );
	MM_Pending( &added, &moved, &deleted );
	if ( added + moved + deleted )
		MM_Text( MM_FOLDER_X, 28, va( S_COLOR_YELLOW "Not sent yet: %i new, %i moved, %i deleted. Tab to the free camera, Enter sends them", added, moved, deleted ), mmWhite );
	else
		MM_Text( MM_FOLDER_X, 28, S_COLOR_GREY "Type to search everything. Enter places the model, Tab edits the map's models.", mmWhite );

	// search box
	{
		const float lw = MM_SearchLabelW();
		wgStyle_t style;

		MM_FieldStyle( &style );
		MM_Box( MM_FOLDER_X, MM_SEARCH_Y, MM_SEARCH_W, 16, mmPanelLight );
		MM_Text( MM_FOLDER_X + 4, MM_SEARCH_Y + 3, "Search:", mm.search[0] ? mmWhite : mmDim );
		WG_Field( &style, MM_FOLDER_X + lw - 4, MM_SEARCH_Y, MM_SEARCH_W - lw + 4, 16, mm.search, qtrue, "", mm.cursorX, mm.cursorY );
	}

	// filter bar
	for ( int i = 0; i < MM_FILTER_COUNT; i++ ) {
		const float bx = MM_FOLDER_X + MM_SEARCH_W + MM_FILTER_GAP + i * ( MM_FILTER_W + MM_FILTER_GAP );
		const char *label = mmFilterNames[i];

		MM_Box( bx, MM_SEARCH_Y, MM_FILTER_W, 16, i == mm.filter ? mmHighlight : MM_InRect( bx, MM_SEARCH_Y, MM_FILTER_W, 16 ) ? mmButtonHover : mmPanelLight );
		MM_Text( bx + ( MM_FILTER_W - re->Font_StrLenPixels( label, mm.font, MM_TEXT_SCALE ) ) * 0.5f, MM_SEARCH_Y + 3, label, i == mm.filter ? mmWhite : mmDim );
	}

	MM_DrawRows();

	// models, or saved constructs
	float mw = MM_MODEL_W;

	MM_Box( MM_MODEL_X, MM_LIST_Y, MM_MODEL_W, MM_LIST_H, mmPanelLight );
	if ( WG_Scrollbar( MM_MODEL_X + MM_MODEL_W - WG_SCROLL_W - 1, MM_LIST_Y + 1, MM_LIST_H - 2, mm.numView, MM_VISIBLE_ROWS, &mm.modelScroll,
		mm.cursorX, mm.cursorY, &mm.barClick, mm.barClickX, mm.barClickY ) )
		mw -= WG_SCROLL_W + 1;
	mm.barClick = qfalse;
	if ( !mm.numView )
		MM_Text( MM_MODEL_X + 4, MM_LIST_Y + 1, mm.viewSaves ? "None yet: select models, press K"
			: mm.viewStates ? "None yet: Shift+K in the free camera"
			: !mm.search[0] && mm.row < mm.numRows && mm.rows[mm.row].type == MM_ROW_FAVES ? "None yet: click the * by a model"
			: "No models match", mmDim );
	for ( int i = 0; i < MM_VISIBLE_ROWS; i++ ) {
		const char *label;
		qboolean failed = qfalse;

		row = mm.modelScroll + i;
		if ( row >= mm.numView )
			break;
		y = MM_LIST_Y + i * MM_ROW_H;
		if ( row == mm.selRow )
			MM_Fill( MM_MODEL_X + 1, y, mw - 2, MM_ROW_H, mmHighlight );
		else if ( MM_InRect( MM_MODEL_X, y, mw, MM_ROW_H ) )
			MM_Fill( MM_MODEL_X + 1, y, mw - 2, MM_ROW_H, mmHover );
		if ( mm.viewSaves ) {
			label = mm.saves[mm.view[row]];
		} else if ( mm.viewStates ) {
			label = mm.states[mm.view[row]];
		} else {
			mmModel_t *m = &mm.models[mm.view[row]];
			const mmRow_t *r = &mm.rows[mm.row];

			MM_CheckModel( m );
			failed = m->failed;
			// a folder lists names, anything wider lists where they are
			label = ( mm.search[0] || r->type != MM_ROW_FOLDER ) ? MM_FolderLabel( m->path ) : m->base;
			// the star: click it for a favorite
			MM_Text( MM_MODEL_X + 4, y + 1, CL_Fav_Star( CL_Fav_Has( MM_FAV_LIST, m->path ) ), mmWhite );
			MM_TextClipped( MM_MODEL_X + 4 + MM_STAR_W, y + 1, mw - 8 - MM_STAR_W, label, failed ? mmRed : mmWhite );
			continue;
		}
		MM_TextClipped( MM_MODEL_X + 4, y + 1, mw - 8, label, failed ? mmRed : mmWhite );
	}

	// preview and details
	MM_DrawPreview( sel );
	y = MM_PREVIEW_Y + MM_PREVIEW_H + 6;
	if ( sel >= 0 && mm.viewStates ) {
		MM_TextClipped( MM_PREVIEW_X, y, MM_PREVIEW_W, mm.states[sel], mmWhite );
		MM_TextClipped( MM_PREVIEW_X, y + MM_ROW_H, MM_PREVIEW_W, S_COLOR_GREY "Gives the models back their names after rpdump load", mmWhite );
		MM_TextClipped( MM_PREVIEW_X, y + MM_ROW_H * 2, MM_PREVIEW_W, va( S_COLOR_GREY "%s/%s%s", MM_STATE_DIR, mm.states[sel], MM_SAVE_EXT ), mmWhite );
		MM_Button( MM_PREVIEW_X, MM_BUTTON_Y, MM_PREVIEW_W, "Reload this state  (Enter)" );
	} else if ( sel >= 0 && mm.viewSaves ) {
		MM_TextClipped( MM_PREVIEW_X, y, MM_PREVIEW_W, mm.saves[sel], mmWhite );
		MM_Text( MM_PREVIEW_X, y + MM_ROW_H, va( S_COLOR_GREY "%i models", Q_max( 0, mm.numSavePieces ) ), mmWhite );
		MM_TextClipped( MM_PREVIEW_X, y + MM_ROW_H * 2, MM_PREVIEW_W, va( S_COLOR_GREY "%s/%s%s", MM_SAVE_DIR, mm.saves[sel], MM_SAVE_EXT ), mmWhite );
		MM_Button( MM_PREVIEW_X, MM_BUTTON_Y, MM_PREVIEW_W, "Place this construct  (Enter)" );
	} else if ( sel >= 0 ) {
		const mmModel_t *m = &mm.models[sel];
		qhandle_t h = MM_RegisterModel( sel );

		MM_TextClipped( MM_PREVIEW_X, y, MM_PREVIEW_W, m->path, mmWhite );
		MM_TextClipped( MM_PREVIEW_X, y + MM_ROW_H, MM_PREVIEW_W, va( S_COLOR_GREY "%s, from %s", m->custom ? "Custom" : "Base", mm.sources[m->source] ), mmWhite );
		if ( h ) {
			vec3_t mins, maxs;

			re->ModelBounds( h, mins, maxs );
			MM_Text( MM_PREVIEW_X, y + MM_ROW_H * 2, va( S_COLOR_GREY "Size %i x %i x %i units, scale %i%%, solid %s",
				(int)( maxs[0] - mins[0] ), (int)( maxs[1] - mins[1] ), (int)( maxs[2] - mins[2] ), mm.defScale, mm.defSolid ? "yes" : "no" ), mmWhite );
		}
		MM_Button( MM_PREVIEW_X, MM_BUTTON_Y, MM_PREVIEW_W, "Place this model  (Enter)" );
	}
	MM_Button( MM_PREVIEW_X, MM_BUTTON_Y + MM_BUTTON_H + 4, MM_PREVIEW_W, "Edit the map's models  (Tab)" );

	MM_Text( MM_FOLDER_X, 440, S_COLOR_GREY "Up/Down select   Left/Right folder   Wheel scroll   Enter or double-click place", mmWhite );
	Com_sprintf( buf, sizeof( buf ), S_COLOR_GREY "Tab free camera   Esc close   %s",
		Key_GetKey( MM_TOGGLE_CMD ) >= 0 || Key_GetKey( MM_OLD_TOGGLE_CMD ) >= 0 ? "Your modelmanager bind closes" : "Tip: bind a key to modelmanager" );
	MM_Text( MM_FOLDER_X, 452, buf, mmWhite );

	WG_DrawOverlay();
	re->DrawStretchPic( mm.cursorX, mm.cursorY, 32, 32, 0, 0, 1, 1, cls.cursorShader );
}

static const char *MM_PieceLabel( const mmPiece_t *p ) {
	const char *tag = p->tag[0] ? va( S_COLOR_GREEN "%s " S_COLOR_WHITE, p->tag ) : "";

	if ( MM_IsNew( p ) )
		return va( "%s%s  " S_COLOR_GREY "(new)", tag, p->name );
	return va( S_COLOR_CYAN "%i %s" S_COLOR_WHITE "%s", p->num, tag, p->name );
}

static void MM_DrawCamera( void ) {
	static const char *help[] = {
		"LMB            select (Shift adds), or put down",
		"Enter          put down, send all changes",
		"Shift+Enter    name it, then put down and send",
		"RMB / F        pick up; lock or follow",
		"P / Ctrl+Wheel follow in front / how far",
		"Wheel Z X Q E  turn: yaw, pitch, roll",
		"R / Backspace  reset angles / and scale",
		"[ ] or - +     scale (Shift x5)",
		"T              toggle solid",
		"Arrows PgUp/Dn nudge",
		"G              cycle snap",
		"B              outlines on / off",
		"hold O or MMB  outline of the one aimed at",
		"V              copy the selection",
		"Del            delete the selection",
		"U              undo (selection, or all)",
		"K / Shift+K    save the selection / the state",
		"I              send as you go / on Enter",
		"L              read the map's models again",
		"WASD Space C   fly (Shift fast, Ctrl slow)",
		"Esc / Tab      cancel a move / browser",
		"J              move arrows (drag X Y Z; RMB looks)",
		"H              hide this help",
	};
	const float x = 10, w = 270, cx = SCREEN_WIDTH * 0.5f, cy = SCREEN_HEIGHT * 0.5f;
	const int count = MM_NumSelected();
	const mmPiece_t *first = MM_FirstSelected();
	int added, moved, deleted, lines;
	float y = 10;
	const char *status;

	// crosshair
	MM_Fill( cx - 6, cy - 0.5f, 12, 1, mmWhite );
	MM_Fill( cx - 0.5f, cy - 6, 1, 12, mmWhite );

	// the move arrows over the selection, and the mouse
	if ( mm.gizmo ) {
		gzView_t v;
		vec3_t pivot;

		if ( count ) {
			MM_GzView( &v );
			MM_Pivot( pivot );
			GZ_Draw( &v, pivot, mm.gzHot );
		}
		if ( !mm.held[A_MOUSE2] && !mm.naming )
			GZ_DrawCursor( mm.cursorX, mm.cursorY );
	}

	if ( mm.carrying )
		status = va( "%s %s", MM_CarryingTemplate() ? S_COLOR_GREEN "PLACING" : S_COLOR_MAGENTA "MOVING",
			mm.locked ? S_COLOR_YELLOW "LOCKED" : cl_modelFollowFront->integer ? va( S_COLOR_GREEN "IN FRONT %i", (int)MM_HoldDistance() )
			: S_COLOR_GREEN "FOLLOWING" );
	else if ( count )
		status = va( S_COLOR_CYAN "%i selected", count );
	else
		status = S_COLOR_GREY "nothing selected";

	MM_Pending( &added, &moved, &deleted );
	lines = 4 + ( added + moved + deleted ? 1 : 0 ) + ( count == 1 ? 5 : count ? 1 : 0 );
	MM_Box( x, y, w, lines * MM_ROW_H + 8, mmPanel );
	y += 4;
	MM_Text( x + 6, y, va( "Model Manager  %s", status ), mmWhite ); y += MM_ROW_H;
	MM_Text( x + 6, y, mm.instant ? "Sends each change as it's made  " S_COLOR_GREY "(I)" : S_COLOR_YELLOW "Sends the changes on Enter  " S_COLOR_GREY "(I)", mmWhite ); y += MM_ROW_H;
	if ( added + moved + deleted ) {
		MM_Text( x + 6, y, va( S_COLOR_YELLOW "Not sent: %i new, %i moved, %i deleted", added, moved, deleted ), mmWhite );
		y += MM_ROW_H;
	}
	if ( count == 1 ) {
		MM_TextClipped( x + 6, y, w - 12, MM_PieceLabel( first ), mmAccent ); y += MM_ROW_H;
		if ( !MM_IsNew( first ) && !first->known ) {
			MM_TextClipped( x + 6, y, w - 12, S_COLOR_YELLOW "Too far from your character to see", mmWhite ); y += MM_ROW_H;
			MM_TextClipped( x + 6, y, w - 12, S_COLOR_YELLOW "how it's turned: it can only be deleted", mmWhite ); y += MM_ROW_H;
			MM_Text( x + 6, y, va( "Origin  %i %i %i", MM_Round( first->origin[0] ), MM_Round( first->origin[1] ), MM_Round( first->origin[2] ) ), mmWhite ); y += MM_ROW_H;
			y += MM_ROW_H;
		} else {
			MM_Text( x + 6, y, va( "Origin  %i %i %i", MM_Round( first->origin[0] ), MM_Round( first->origin[1] ), MM_Round( first->origin[2] ) ), mmWhite ); y += MM_ROW_H;
			MM_Text( x + 6, y, va( "Angles  %i %i %i", MM_NormalizeAngle( first->angles[PITCH] ), MM_NormalizeAngle( first->angles[YAW] ), MM_NormalizeAngle( first->angles[ROLL] ) ), mmWhite ); y += MM_ROW_H;
			MM_Text( x + 6, y, va( "Scale   %i%%", first->scale ), mmWhite ); y += MM_ROW_H;
			MM_Text( x + 6, y, va( "Solid   %s%s", first->solid ? S_COLOR_GREEN "yes" : S_COLOR_YELLOW "no", first->deleted ? S_COLOR_RED "   DELETED" : "" ), mmWhite ); y += MM_ROW_H;
		}
	} else if ( count ) {
		int onServer = 0;

		for ( int i = 0; i < mm.numPieces; i++ ) {
			if ( MM_Sel( &mm.pieces[i] ) && !MM_IsNew( &mm.pieces[i] ) )
				onServer++;
		}
		MM_Text( x + 6, y, va( "%i models: %i on the server, %i new", count, onServer, count - onServer ), mmAccent ); y += MM_ROW_H;
	}
	MM_Text( x + 6, y, va( "Snap    %i deg / %i units%s", mmSnaps[mm.snap].angle, mmSnaps[mm.snap].grid,
		mm.hideBoxes ? S_COLOR_YELLOW "   outlines off (B)" : "" ), mmWhite ); y += MM_ROW_H;
	MM_Text( x + 6, y, va( S_COLOR_GREY "%i models in the map", mm.numPieces ), mmWhite ); y += MM_ROW_H;

	if ( !mm.hideHelp ) {
		y += 6;
		MM_Box( x, y, w, ARRAY_LEN( help ) * MM_ROW_H + 8, mmPanel );
		y += 4;
		for ( size_t i = 0; i < ARRAY_LEN( help ); i++, y += MM_ROW_H )
			MM_Text( x + 6, y, help[i], mmDim );
	}

	// what is under the crosshair, what is going out, and what came back
	MM_Box( 10, SCREEN_HEIGHT - 46, SCREEN_WIDTH - 20, 36, mmPanel );
	if ( mm.hover >= 0 && mm.hover < mm.numPieces ) {
		const mmPiece_t *p = &mm.pieces[mm.hover];

		MM_TextClipped( 16, SCREEN_HEIGHT - 44, SCREEN_WIDTH - 32, va( "%s%s", MM_PieceLabel( p ),
			p->deleted ? S_COLOR_RED "  deleted" : MM_Moved( p ) ? S_COLOR_MAGENTA "  moved" : "" ), mmWhite );
	} else if ( mm.carrying && first ) {
		MM_TextClipped( 16, SCREEN_HEIGHT - 44, SCREEN_WIDTH - 32, MM_CarryingTemplate() && count == 1
			? MM_AddCommand( first ) : va( "%i models in hand", count ), mmWhite );
	}
	if ( mm.numQueue ) {
		MM_TextClipped( 16, SCREEN_HEIGHT - 33, SCREEN_WIDTH - 32,
			va( S_COLOR_YELLOW "Sending, %i commands to go (the server takes about one a second)", mm.numQueue ), mmWhite );
	} else if ( mm.lastCmd[0] ) {
		MM_TextClipped( 16, SCREEN_HEIGHT - 33, SCREEN_WIDTH - 32,
			va( "%sSent: %s", cls.realtime - mm.sentTime < 1500 ? S_COLOR_GREEN : S_COLOR_GREY, mm.lastCmd ), mmWhite );
	}
	if ( mm.message[0] && cls.realtime - mm.messageTime < MM_MESSAGE_MS )
		MM_TextClipped( 16, SCREEN_HEIGHT - 22, SCREEN_WIDTH - 32, mm.message, mmWhite );
	else if ( mm.reply[0] && cls.realtime - mm.replyTime < 8000 )
		MM_TextClipped( 16, SCREEN_HEIGHT - 22, SCREEN_WIDTH - 32, va( S_COLOR_YELLOW "Server: " S_COLOR_WHITE "%s", mm.reply ), mmWhite );

	// typing a name
	if ( mm.naming ) {
		const float bw = 340, bx = ( SCREEN_WIDTH - bw ) * 0.5f, by = SCREEN_HEIGHT * 0.5f + 30;
		const char *title, *hint, *note = NULL;
		int others;

		switch ( mm.naming ) {
		case MM_NAMING_TAG:
			title = va( MM_CarryingTemplate() ? "Name the %i models you're putting down:" : "Name the %i selected models:", count );
			hint = S_COLOR_GREY "Enter names, puts down and sends; empty takes the name off; Esc cancels";
			// the same name again makes them one group
			others = mm.saveName[0] ? MM_CountTagged( mm.saveName ) : 0;
			for ( int i = 0; i < mm.numPieces && others; i++ ) {
				if ( MM_Sel( &mm.pieces[i] ) && !mm.pieces[i].tmpl && !Q_stricmp( mm.pieces[i].tag, mm.saveName ) )
					others--;
			}
			if ( others )
				note = va( S_COLOR_YELLOW "%i other models are named %s; these join them", others, mm.saveName );
			break;
		case MM_NAMING_STATE:
			title = va( "Save the state of %i models placed or named as:", MM_StateCount() );
			hint = S_COLOR_GREY "Enter saves, Esc cancels";
			if ( mm.saveName[0] && FS_FileExists( va( "%s/%s%s", MM_STATE_DIR, mm.saveName, MM_SAVE_EXT ) ) )
				note = S_COLOR_YELLOW "A state with this name is there already; Enter replaces it";
			break;
		default:
			title = va( "Save %i models as a construct named:", count );
			hint = S_COLOR_GREY "Enter saves, Esc cancels";
			break;
		}

		MM_Box( bx, by, bw, note ? 58 : 46, mmPanelOpaque );
		MM_TextClipped( bx + 8, by + 4, bw - 16, title, mmWhite );
		wgStyle_t style;

		MM_FieldStyle( &style );
		MM_Box( bx + 8, by + 18, bw - 16, 16, mmPanelLight );
		WG_Field( &style, bx + 8, by + 18, bw - 16, 16, mm.saveName, qtrue, "", mm.cursorX, mm.cursorY );
		MM_TextClipped( bx + 8, by + 36, bw - 16, hint, mmWhite );
		if ( note )
			MM_TextClipped( bx + 8, by + 48, bw - 16, note, mmWhite );
		WG_DrawOverlay();
	}
}

// drawn over the cgame, under the UI and console
void CL_ModelManager_Draw( void ) {
	if ( mm.state == MM_BROWSE )
		MM_DrawBrowser();
	else if ( mm.state == MM_CAMERA )
		MM_DrawCamera();
}
