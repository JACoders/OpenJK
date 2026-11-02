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

// cl_effectmanager.cpp -- engine-side front end for RPMod's /rpeffect command
//
// Effects tab: every effects/*.efx the filesystem can see, Base (the game's own pk3s)
// or Custom, played live in a window onto the world, then sent as "rpeffect once|add"
// at the player, at a spot picked with a free camera, or bolted to a tag or bone of a
// player or NPC. Bolted ones are shown on the real player or NPC before they are sent.
// Effects can wait to be sent together, and looping ones can be given a name.
// Active tab: the effects playing in the map, read from the server's "rpeffect list"
// reply, to remove one, several, all, or the ones with a name. RPMod doesn't keep the
// names, so a state does: saved before rpdump, reloaded after, it finds the effects
// again by what and where they are. Presets tab: alarm, fadeout and camerashake.
//
// Previews are played by this client's own effects system, so nobody else sees
// them. This lives in the engine rather than cgame so it works whatever cgame the
// server's mod ships (RPMod loads its own).

#include "client.h"
#include "qcommon/cm_public.h"
#include "ghoul2/G2.h"
#include "cl_cgameapi.h"
#include "FXExport.h"
#include "FxScheduler.h"
#include "cl_gizmo.h"
#include "cl_widgets.h"

#define EM_ROOT				"effects"
#define EM_EXT				".efx"
#define EM_TOGGLE_CMD		"effectmanager"
#define EM_LIST_HEADER		"List of all effects currently playing"
#define EM_SAVE_DIR			"effectmanager"			// in the game folder
#define EM_STATE_DIR		EM_SAVE_DIR "/states"
#define EM_CONSTRUCT_DIR	EM_SAVE_DIR "/constructs"
#define EM_MAX_CPIECES		128		// effects in one construct
#define EM_SAVE_EXT			".cfg"
#define EM_BASE_PAKS		BASEGAME "/assets"		// base/assets0.pk3 to assets3.pk3 are the game's own

#define EM_MAX_SOURCES		1024
#define EM_MAX_WAITING		128		// effects put down, not sent yet
#define EM_MAX_EXPECT		128		// looping effects sent, not in a list yet
#define EM_MAX_NAMES		256
#define EM_MAX_STATE		EM_MAX_ACTIVE
#define EM_NAME_LEN			48
#define EM_EXPECT_MS		30000	// a sent effect not in a list by then isn't coming
#define EM_MATCH_DIST		8.0f	// an effect this close to where it was sent is that one
#define EM_STATE_MATCH_DIST	4.0f	// and this close to where a state had it, after rpdump load
#define EM_MESSAGE_MS		5000
#define EM_FILTER_W			44.0f	// the All / Base / Custom buttons, right of the search box

#define EM_MAX_EFFECTS		8192
#define EM_MAX_ROWS			2048
#define EM_NAME_POOL		( EM_MAX_EFFECTS * 48 )
#define EM_HASH_SIZE		( EM_MAX_EFFECTS * 2 )
#define EM_MAX_ACTIVE		1024
#define EM_MAX_BOLTS		384
#define EM_MAX_CAPTURED		512
#define EM_MAX_QUEUE		256
#define EM_MAX_RETIRED		16
#define EM_FIELD_LEN		64

#define EM_LIST_TIMEOUT		2500	// ms without the list header before giving up on it
#define EM_LIST_LINE_GAP	1500	// ms between list lines before the list is over
#define EM_REPLY_MS			3000	// prints this soon after a command are shown as its reply
#define EM_CONFIRM_MS		3000
#define EM_DOUBLECLICK_MS	400
#define EM_SETTLE_MS		250		// the selection rests this long before the effect is loaded
#define EM_RETIRE_MS		60000	// a model copy outlives the effects bolted to it by this much
#define EM_ONCE_GAP			1000	// pause between plays of a "once" effect in the preview
#define EM_MIN_PERIOD		100
#define EM_TRACE_MASK		(CONTENTS_SOLID|CONTENTS_TERRAIN)
#define EM_TRACE_DIST		8192.0f
#define EM_NO_HIT_DIST		256.0f
#define EM_FLY_SPEED		400.0f
#define EM_TARGET_XY		40.0f	// a model this close to the entity's origin is taken as its own
#define EM_TARGET_Z			80.0f
#define EM_MAX_BOLT_INDEX	127		// the effects system keeps bolt numbers in a char

// layout, in 640x480
#define EM_ROW_H			13.0f
#define EM_STAR_W			11.0f	// the favorite star an effect row starts with
#define EM_FAV_LIST			"effects"
#define EM_CTRL_H			15.0f
#define EM_TEXT_SCALE		0.8f
#define EM_SEARCH_Y			46.0f
#define EM_LIST_Y			66.0f
#define EM_LIST_H			364.0f
#define EM_FOLDER_X			16.0f
#define EM_FOLDER_W			150.0f
#define EM_EFFECT_X			170.0f
#define EM_EFFECT_W			160.0f
#define EM_RIGHT_X			338.0f
#define EM_RIGHT_W			286.0f
#define EM_CTRL_X			( EM_RIGHT_X + 52.0f )
#define EM_CTRL_W			( EM_RIGHT_X + EM_RIGHT_W - EM_CTRL_X )
#define EM_VIEW_Y			46.0f	// the preview window
#define EM_VIEW_H			150.0f
#define EM_BUTTON_Y			410.0f
#define EM_ACTIVE_W			310.0f	// the active effects list
#define EM_PICKER_X			150.0f
#define EM_PICKER_Y			50.0f
#define EM_PICKER_W			340.0f
#define EM_PICKER_H			380.0f
#define EM_PICKER_BOLT_X	60.0f	// the bolt picker, with the model on its right
#define EM_PICKER_PREVIEW_W	200.0f

typedef enum {
	EM_OFF,
	EM_BROWSE,
	EM_PLACE,		// free camera, the effect at the crosshair or on its bolt
	EM_PICK			// active effects labelled on the map, click to check them
} emState_t;

typedef enum {
	EM_TAB_EFFECTS,
	EM_TAB_ACTIVE,
	EM_TAB_PRESETS
} emTab_t;

typedef enum {
	EM_WHERE_ME,	// no coordinates: the server uses the player's origin
	EM_WHERE_POS,
	EM_WHERE_BOLT
} emWhere_t;

typedef enum {
	EM_TARGET_ME,
	EM_TARGET_PLAYER,
	EM_TARGET_NPC
} emTarget_t;

typedef enum {
	EM_PICKER_NONE,
	EM_PICKER_PLAYER,
	EM_PICKER_NPC,
	EM_PICKER_BOLT,
	EM_PICKER_REMOVE_PLAYER,	// "remove bolted" on a player
	EM_PICKER_REMOVE_NPC,
	EM_PICKER_REMOVE_NAME,		// the effects with a name
	EM_PICKER_STATE				// a saved state to reload
} emPicker_t;

typedef enum {
	EM_FILTER_ALL,
	EM_FILTER_BASE,
	EM_FILTER_CUSTOM,
	EM_FILTER_COUNT
} emFilter_t;

static const char *emFilterNames[EM_FILTER_COUNT] = { "All", "Base", "Custom" };

typedef enum {
	EM_NAMING_NONE,
	EM_NAMING_TAG,		// naming what is sent, Shift+Enter in the free camera
	EM_NAMING_STATE,	// saving the state
	EM_NAMING_CONSTRUCT	// saving the selection, or what waits, as a construct
} emNaming_t;

typedef enum {
	EM_F_NONE,			// typing goes to the search box of whatever is showing
	EM_F_SEARCH,
	EM_F_PITCH,
	EM_F_YAW,
	EM_F_ROLL,
	EM_F_BOLT,
	EM_F_ACTIVE_SEARCH,
	EM_F_INTENSITY,
	EM_F_DURATION,
	EM_F_PICKER_SEARCH,
	EM_F_NAME,			// what the effects sent are named, "" = nothing
	EM_F_STATE_NAME,
	EM_F_CONSTRUCT_NAME,
	EM_NUM_FIELDS
} emField_t;

typedef enum {
	EM_CONFIRM_NONE,
	EM_CONFIRM_REMOVE,
	EM_CONFIRM_REMOVE_ALL,
	EM_CONFIRM_CLEAR
} emConfirm_t;

typedef struct emEffect_s {
	const char	*path;		// relative to EM_ROOT, no extension, e.g. "scepter/invincibility"
	const char	*base;		// file name part of path
	int			id;			// in the effects system, 0 = not loaded yet
	qboolean	failed;		// the effects system couldn't load it
	int			source;		// where it is read from, index into sources
	qboolean	custom;		// not from the game's own pk3s
} emEffect_t;

// the left column, as the model manager has it: the saved constructs, then Base with its
// folders, then Custom with a row for each pk3 and that pk3's folders under it
typedef enum {
	EM_ROW_SAVED,
	EM_ROW_FAVES,		// the starred effects
	EM_ROW_CATEGORY,	// Base or Custom
	EM_ROW_SOURCE,		// a custom pk3
	EM_ROW_FOLDER
} emRowType_t;

typedef struct emRow_s {
	emRowType_t	type;
	char		name[MAX_QPATH];	// a folder's path relative to EM_ROOT, "" for the root itself
	int			first, count;		// range in the sorted effect list
	qboolean	custom;				// under Custom
} emRow_t;

typedef struct emActive_s {
	int			num;
	vec3_t		origin;
	char		path[MAX_QPATH];
	qboolean	checked;
	char		tag[EM_NAME_LEN];	// the name it was sent with, "" = none
	qboolean	placed;				// sent by this manager, so it goes in the state
	qboolean	bolted;				// sent bolted, so where it is says little
	qboolean	fresh;				// not in the list before this one

	// moving it in the free camera: sent as a remove and an add
	vec3_t		pos;				// where it's going, origin if nowhere
	vec3_t		angles;
	qboolean	haveAngles;			// else straight up
	qboolean	srvHaveAngles;		// what it was sent with, if this manager sent it
	vec3_t		srvAngles;
	qboolean	selected;
	qboolean	deleted;			// to be removed
	qboolean	sent;				// its commands are on their way; the next list shows how it went
	vec3_t		carryFrom, carryOffset, carryAngles;
	qboolean	carryHaveAngles;
	int			nextPlay;			// its preview where it's going
} emActive_t;

// one effect as it is sent: the form's, or one waiting
typedef struct emItem_s {
	char		path[MAX_QPATH];
	qboolean	loop;
	qboolean	bolted;
	qboolean	havePos;			// else at the player
	vec3_t		pos;
	qboolean	haveAngles;			// else straight up
	vec3_t		angles;
	char		bolt[EM_FIELD_LEN];
	emTarget_t	target;
	int			targetClient, targetNpc;
	char		targetName[MAX_QPATH];
	char		tag[EM_NAME_LEN];
	float		scale;				// how big it's drawn, 0 or 1 as made
	int			nextPlay;			// its preview, waiting
	qboolean	selected;			// in the free camera
	vec3_t		carryFrom, carryOffset, carryAngles;
	qboolean	carryHaveAngles;
} emItem_t;

// a looping effect sent, to be named when the list shows it
typedef struct emExpect_s {
	char		path[MAX_QPATH];
	qboolean	anyPos;				// bolted or at the player: where it is wasn't known
	vec3_t		pos;
	qboolean	bolted;
	qboolean	haveAngles;
	vec3_t		angles;
	char		tag[EM_NAME_LEN];
	qboolean	selected;			// a moved one, selected when it went
	int			time;
} emExpect_t;

// an effect of a construct: at a place from its middle, or bolted to a tag or bone of
// whoever it is put on, which is picked when it is used
typedef struct emCPiece_s {
	char		path[MAX_QPATH];
	qboolean	loop;
	qboolean	bolted;
	vec3_t		offset;
	qboolean	haveAngles;
	vec3_t		angles;
	char		bolt[EM_FIELD_LEN];
	int			nextPlay;			// its preview, in hand
} emCPiece_t;

// an effect of a saved state, where the server had it
typedef struct emStateEffect_s {
	char		tag[EM_NAME_LEN];
	char		path[MAX_QPATH];
	vec3_t		origin;
	qboolean	bolted;
} emStateEffect_t;

typedef struct emList_s {
	int			sel, scroll;
	int			lastClickTime, lastClickRow;
} emList_t;

typedef struct emCaptured_s {
	CGhoul2Info_v	*ghoul2;
	vec3_t			origin;
	vec3_t			scale;
} emCaptured_t;

typedef struct emBolt_s {
	char		name[MAX_QPATH];
	qboolean	tag;		// a *tag surface, not a bone
} emBolt_t;

typedef struct emSnap_s {
	int			angle;		// degrees per rotation step
	int			grid;		// units per nudge / follow snap
} emSnap_t;

static const emSnap_t emSnaps[] = {
	{ 1, 1 },
	{ 5, 2 },
	{ 15, 4 },
	{ 45, 8 },
	{ 90, 16 },
};

static struct {
	emState_t	state;
	emTab_t		tab;
	emPicker_t	picker;
	int			font;

	// input; clicks and the wheel are handled where the control under them is drawn
	float		cursorX, cursorY;
	qboolean	click;
	float		clickX, clickY;
	int			wheel;
	emField_t	focus;
	char		fields[EM_NUM_FIELDS][EM_FIELD_LEN];
	qboolean	held[MAX_KEYS];

	// index, rebuilt the first time the manager opens after cgame (re)starts
	qboolean	indexed;
	int			numEffects;
	emEffect_t	effects[EM_MAX_EFFECTS];
	int			numCustom;
	char		pool[EM_NAME_POOL];
	int			poolUsed;
	int			hash[EM_HASH_SIZE];	// effect index + 1, 0 = empty
	const char	*sources[EM_MAX_SOURCES];	// "gamedir/name.pk3", or "gamedir" for loose files
	int			numSources;

	// effects tab
	emFilter_t	filter;					// which effects the rows and the search show
	emRow_t		rows[EM_MAX_ROWS];		// of the effects the filter lets through
	int			numRows;
	int			row;					// the one selected, its effects listed on the right
	int			view[EM_MAX_EFFECTS];	// effect indices currently listed
	int			numView;
	emList_t	folderList, effectList;
	int			effect;					// selected effect, -1 = none
	int			selTime;				// when it was selected
	qboolean	loop;					// "add" rather than "once"
	emWhere_t	where;
	qboolean	havePos;
	vec3_t		pos;
	float		previewDist;			// how far into the preview window the effect plays

	// put down or added, waiting to be sent together
	emItem_t	waiting[EM_MAX_WAITING];
	int			numWaiting;
	qboolean	instant;				// the free camera sends each one as it is put down, else on Enter

	// constructs
	char		constructs[EM_MAX_NAMES][MAX_QPATH];	// saved ones, sorted
	int			constructCounts[EM_MAX_NAMES];
	int			numConstructs;
	int			construct;				// the one selected on their row, -1 = none
	emCPiece_t	cpieces[EM_MAX_CPIECES];	// what it holds
	int			numCPieces;
	qboolean	constructFromSelection;	// saving the free camera's selection, else what waits
	emCPiece_t	hand[EM_MAX_CPIECES];	// a construct's placed effects, in hand in the free camera
	int			numHand;
	qboolean	handConstruct;			// the free camera puts the hand down, not the form's effect
	int			handYaw;				// it's turned by
	char		handName[MAX_QPATH];

	// selecting and moving effects in the free camera, waiting ones and the map's
	qboolean	selecting;				// the free camera selects, rather than puts the form's effect down
	qboolean	carrying;				// the selection is in hand
	qboolean	carryLocked;			// in hand, but not following the crosshair
	int			hover;					// under the crosshair: a handle, -1 = none

	// names and states
	emExpect_t	expect[EM_MAX_EXPECT];
	int			numExpect;
	emNaming_t	naming;					// the name box is open, and what for
	char		names[EM_MAX_NAMES][EM_NAME_LEN];	// for the remove-by-name picker
	int			numNames;
	char		stateFiles[EM_MAX_NAMES][MAX_QPATH];	// for the reload picker
	int			numStateFiles;
	char		activeMap[MAX_STRING_CHARS];	// the server and map the active list is for
	qboolean	reloading;				// a state waits for the list it asked for
	int			reloadTime;
	char		reloadName[MAX_QPATH];
	emStateEffect_t stateEffects[EM_MAX_STATE];
	int			numStateEffects;
	char		message[MAX_STRING_CHARS];
	int			messageTime;

	// bolting
	emTarget_t	target;
	int			targetClient;
	int			targetNpc;				// entity number
	char		targetName[MAX_QPATH];	// NPC targetname
	qboolean	targetSeen;				// its model was in the last frame
	CGhoul2Info_v *g2;					// our copy of the target's model, bolted effects hold on to it
	CGhoul2Info_v *g2View;				// another, for the model view: looking at bolts adds them, and
										// effects can only take the first EM_MAX_BOLT_INDEX of them
	char		g2Key[MAX_QPATH * 2];	// what it was copied from
	int			g2Ent;
	vec3_t		g2Scale;
	char		g2Model[MAX_QPATH];		// model/skin, for showing
	vec3_t		g2Mins, g2Maxs;			// around its skeleton
	emBolt_t	bolts[EM_MAX_BOLTS];
	int			numBolts;
	float		modelYaw;				// added by the wheel in the model view
	CGhoul2Info_v *retired[EM_MAX_RETIRED];
	int			retiredTime[EM_MAX_RETIRED];

	// what the cgame drew last frame, for finding the target's model
	emCaptured_t pending[EM_MAX_CAPTURED];
	int			numPending;

	// preview playback
	char		playKey[MAX_QPATH * 3];
	int			nextPlay;
	const char	*playError;

	// active tab
	emActive_t	active[EM_MAX_ACTIVE];
	int			numActive;
	int			activeView[EM_MAX_ACTIVE];
	int			numActiveView;
	emList_t	activeList;
	emConfirm_t	confirm;
	int			confirmTime;
	int			listRequestTime;	// asked for it, waiting for the header, 0 = not waiting
	qboolean	listOurs;			// the list being read was asked for by the manager
	qboolean	listQueued;			// a request is waiting in the queue
	int			listLineTime;		// last list line read, 0 = not reading a list
	int			listTime;			// when a list was last read, 0 = never
	int			listTimeoutTime;	// when a list asked for last didn't come
	int			refreshTime;		// read the list again at this time, 0 = no

	// picker
	int			pickerView[EM_MAX_NAMES > EM_MAX_BOLTS ? EM_MAX_NAMES : EM_MAX_BOLTS];
	int			numPickerView;
	emList_t	pickerList;

	// placing
	vec3_t		camOrg, camAng;
	vec3_t		origin;				// where the effect goes
	qboolean	locked;				// qfalse = it follows the crosshair
	qboolean	align;				// face away from the surface under the crosshair
	qboolean	hideHelp;
	int			snap;
	int			lastFrameTime;
	int			viewFrame;			// cls.framecount the main view was last re-aimed on

	// the main view last frame: the cgame's, or the free camera's
	qboolean	haveView;
	vec3_t		viewOrg;
	matrix3_t	viewAxis;
	float		fovX, fovY;

	// commands, spaced out for the server's flood protection
	char		queue[EM_MAX_QUEUE][MAX_STRING_CHARS];
	qboolean	queueEcho[EM_MAX_QUEUE];
	int			numQueue;
	int			nextSend;
	char		lastCmd[MAX_STRING_CHARS];
	int			sentTime;
	char		reply[MAX_STRING_CHARS];
	int			replyTime;

	// how big the form's effect goes down, 1 as made (experimental: for everyone with this client)
	float		scale;
	int			sharePlayers;		// players on the server, to see someone join
	int			shareAt;			// the sizes this client set go out again then, 0 not
	qboolean	shareWarned;

	// the move arrows in the free camera: the mouse a cursor, an arrow dragged moves along it
	qboolean	gizmo;
	qboolean	gzDragging;
	gzDrag_t	gzDrag;
	int			gzHot;
} em;

static vec4_t emWhite		= { 1.0f, 1.0f, 1.0f, 1.0f };
static vec4_t emRed			= { 1.0f, 0.3f, 0.3f, 1.0f };
static vec4_t emPanel		= { 0.0f, 0.0f, 0.0f, 0.65f };
static vec4_t emPanelLight	= { 0.15f, 0.15f, 0.15f, 0.8f };
static vec4_t emPanelFocus	= { 0.1f, 0.2f, 0.3f, 0.9f };
static vec4_t emHighlight	= { 0.2f, 0.45f, 0.8f, 0.8f };
static vec4_t emHover		= { 1.0f, 1.0f, 1.0f, 0.12f };
static vec4_t emHoverBox	= { 0.25f, 0.25f, 0.25f, 0.9f };
static vec4_t emDanger		= { 0.6f, 0.15f, 0.15f, 0.9f };
static vec4_t emBorder		= { 0.5f, 0.5f, 0.5f, 0.8f };
static vec4_t emDim			= { 0.7f, 0.7f, 0.7f, 1.0f };
static vec4_t emAccent		= { 1.0f, 0.8f, 0.3f, 1.0f };
static vec4_t emCustom		= { 0.6f, 1.0f, 0.6f, 1.0f };		// custom effects in the list
static vec4_t emPanelOpaque	= { 0.05f, 0.05f, 0.05f, 0.95f };	// a box over the panel, which would show through

static void EM_RequestList( void );
static void EM_FieldChanged( int f );
static void EM_BuildRows( void );
static int EM_FirstFolderRow( void );
static qboolean EM_ActiveEdited( const emActive_t *a );
static void EM_NormPath( const char *in, char *out, int size );
static int EM_ReadConstruct( const char *name, emCPiece_t *out, int max );
static void EM_ListConstructs( void );
static void EM_HandItem( int i, const vec3_t anchor, emItem_t *out );
static void EM_StartNaming( emNaming_t naming );
static float EM_ClampScale( float scale );
static float EM_ScaleAt( const char *path, const vec3_t org );

/*
===============================================================================

INDEX

===============================================================================
*/

static int EM_HashName( const char *s ) {
	unsigned int h = 5381;

	while ( *s ) {
		h = h * 33 + (unsigned int)tolower( (unsigned char)*s );
		s++;
	}
	return (int)( h % EM_HASH_SIZE );
}

static const char *EM_PoolString( const char *s ) {
	int len = strlen( s ) + 1;
	char *out;

	if ( em.poolUsed + len > EM_NAME_POOL )
		return NULL;
	out = em.pool + em.poolUsed;
	memcpy( out, s, len );
	em.poolUsed += len;
	return out;
}

// "base/assets<digits>.pk3"; anything else people named assets_something isn't
static qboolean EM_IsBaseSource( const char *source ) {
	const int prefixLen = strlen( EM_BASE_PAKS );
	const char *c = source + prefixLen;

	if ( Q_stricmpn( source, EM_BASE_PAKS, prefixLen ) || !isdigit( (unsigned char)*c ) )
		return qfalse;
	while ( isdigit( (unsigned char)*c ) )
		c++;
	return (qboolean)!Q_stricmp( c, ".pk3" );
}

static int EM_SourceIndex( const char *source ) {
	const char *stored;

	// the files of one pk3 come together
	for ( int i = em.numSources - 1; i >= 0; i-- ) {
		if ( !Q_stricmp( em.sources[i], source ) )
			return i;
	}
	if ( em.numSources >= EM_MAX_SOURCES || !( stored = EM_PoolString( source ) ) )
		return 0;
	em.sources[em.numSources] = stored;
	return em.numSources++;
}

// name is a full game path, e.g. "effects/scepter/invincibility.efx"; the first one seen is the one in use
static void EM_AddEffect( const char *name, const char *source, void *ctx ) {
	char path[MAX_QPATH];
	const int rootLen = strlen( EM_ROOT ), extLen = strlen( EM_EXT );
	const char *stored, *slash;
	int h, len;

	len = strlen( name );
	if ( len <= rootLen + 1 + extLen || Q_stricmpn( name, EM_ROOT "/", rootLen + 1 ) )
		return;
	Q_strncpyz( path, name + rootLen + 1, sizeof( path ) );
	path[strlen( path ) - extLen] = '\0';
	for ( char *c = path; *c; c++ ) {
		if ( *c == '\\' )
			*c = '/';
	}

	// the same file can be in several pk3s and folders
	h = EM_HashName( path );
	while ( em.hash[h] ) {
		if ( !Q_stricmp( em.effects[em.hash[h] - 1].path, path ) )
			return;
		h = ( h + 1 ) % EM_HASH_SIZE;
	}

	if ( em.numEffects >= EM_MAX_EFFECTS )
		return;
	stored = EM_PoolString( path );
	if ( !stored )
		return;

	slash = strrchr( stored, '/' );
	memset( &em.effects[em.numEffects], 0, sizeof( em.effects[0] ) );
	em.effects[em.numEffects].path = stored;
	em.effects[em.numEffects].base = slash ? slash + 1 : stored;
	em.effects[em.numEffects].source = EM_SourceIndex( source );
	em.effects[em.numEffects].custom = (qboolean)!EM_IsBaseSource( source );
	em.hash[h] = ++em.numEffects;
}

// the effect with the path relative to EM_ROOT, -1 if there is none
static int EM_FindEffect( const char *path ) {
	int h = EM_HashName( path );

	while ( em.hash[h] ) {
		if ( !Q_stricmp( em.effects[em.hash[h] - 1].path, path ) )
			return em.hash[h] - 1;
		h = ( h + 1 ) % EM_HASH_SIZE;
	}
	return -1;
}

static int EM_FolderLen( const emEffect_t *e ) {
	return (int)( e->base - e->path ) - ( e->base != e->path ? 1 : 0 );
}

// Base first, then Custom pk3 by pk3, then by folder: every row is one contiguous range
static int QDECL EM_CompareEffects( const void *a, const void *b ) {
	const emEffect_t *ea = (const emEffect_t *)a, *eb = (const emEffect_t *)b;
	int la = EM_FolderLen( ea ), lb = EM_FolderLen( eb ), cmp;

	if ( ea->custom != eb->custom )
		return ea->custom ? 1 : -1;
	if ( ea->custom && ea->source != eb->source && ( cmp = Q_stricmp( em.sources[ea->source], em.sources[eb->source] ) ) )
		return cmp;
	cmp = Q_stricmpn( ea->path, eb->path, la < lb ? la : lb );
	if ( cmp )
		return cmp;
	if ( la != lb )
		return la - lb;
	return Q_stricmp( ea->base, eb->base );
}

static void EM_BuildIndex( void ) {
	int start = Sys_Milliseconds();

	em.numEffects = 0;
	em.numCustom = 0;
	em.poolUsed = 0;
	em.numSources = 0;
	memset( em.hash, 0, sizeof( em.hash ) );

	FS_ListFilesRecursiveFrom( EM_ROOT, EM_EXT, EM_AddEffect, NULL );

	qsort( em.effects, em.numEffects, sizeof( em.effects[0] ), EM_CompareEffects );
	// sorting moved them; the hash finds them by path again
	memset( em.hash, 0, sizeof( em.hash ) );
	for ( int i = 0; i < em.numEffects; i++ ) {
		int h = EM_HashName( em.effects[i].path );

		while ( em.hash[h] )
			h = ( h + 1 ) % EM_HASH_SIZE;
		em.hash[h] = i + 1;
	}

	for ( int i = 0; i < em.numEffects; i++ ) {
		if ( em.effects[i].custom )
			em.numCustom++;
	}

	em.indexed = qtrue;
	em.effect = -1;
	memset( &em.folderList, 0, sizeof( em.folderList ) );
	memset( &em.effectList, 0, sizeof( em.effectList ) );
	EM_BuildRows();
	em.row = em.folderList.sel = EM_FirstFolderRow();
	Com_Printf( "Effect manager: indexed %i effects, %i of them custom, in %i rows (%i ms)\n",
		em.numEffects, em.numCustom, em.numRows, Sys_Milliseconds() - start );
}

/*
===============================================================================

HELPERS

===============================================================================
*/

static qboolean EM_ContainsNoCase( const char *haystack, const char *needle ) {
	int nlen = strlen( needle );

	if ( !nlen )
		return qtrue;
	for ( ; *haystack; haystack++ ) {
		if ( !Q_stricmpn( haystack, needle, nlen ) )
			return qtrue;
	}
	return qfalse;
}

static const char *EM_PlayerName( int clientNum ) {
	static char name[MAX_QPATH];
	const char *cs;

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS )
		return "";
	cs = cl.gameState.stringData + cl.gameState.stringOffsets[CS_PLAYERS + clientNum];
	if ( !cs[0] )
		return "";
	Q_strncpyz( name, Info_ValueForKey( cs, "n" ), sizeof( name ) );
	Q_StripColor( name );
	return name;
}

static int EM_NormalizeAngle( float a ) {
	int i = (int)floorf( a + 0.5f ) % 360;
	return i < 0 ? i + 360 : i;
}

static float EM_SnapTo( float v, int grid ) {
	return grid > 1 ? floorf( v / grid + 0.5f ) * grid : floorf( v + 0.5f );
}

// a selection of -1 (none) stays none
static void EM_ListClamp( emList_t *list, int count, int visible ) {
	if ( list->sel >= 0 )
		list->sel = Com_Clampi( 0, count ? count - 1 : 0, list->sel );
	list->scroll = Com_Clampi( 0, count > visible ? count - visible : 0, list->scroll );
}

static void EM_ListMove( emList_t *list, int count, int visible, int delta ) {
	if ( !count )
		return;
	list->sel = Com_Clampi( 0, count - 1, list->sel + delta );
	if ( list->sel < list->scroll )
		list->scroll = list->sel;
	else if ( list->sel >= list->scroll + visible )
		list->scroll = list->sel - visible + 1;
}

static const emEffect_t *EM_Effect( void ) {
	return em.effect >= 0 && em.effect < em.numEffects ? &em.effects[em.effect] : NULL;
}

// the angle boxes; empty boxes are 0
static void EM_GetAngles( vec3_t angles ) {
	angles[PITCH] = (float)atof( em.fields[EM_F_PITCH] );
	angles[YAW] = (float)atof( em.fields[EM_F_YAW] );
	angles[ROLL] = (float)atof( em.fields[EM_F_ROLL] );
}

static void EM_SetAngles( const vec3_t angles ) {
	Com_sprintf( em.fields[EM_F_PITCH], EM_FIELD_LEN, "%i", EM_NormalizeAngle( angles[PITCH] ) );
	Com_sprintf( em.fields[EM_F_YAW], EM_FIELD_LEN, "%i", EM_NormalizeAngle( angles[YAW] ) );
	Com_sprintf( em.fields[EM_F_ROLL], EM_FIELD_LEN, "%i", EM_NormalizeAngle( angles[ROLL] ) );
}

static qboolean EM_HaveAngles( void ) {
	vec3_t angles;

	EM_GetAngles( angles );
	return (qboolean)( EM_NormalizeAngle( angles[PITCH] ) || EM_NormalizeAngle( angles[YAW] ) || EM_NormalizeAngle( angles[ROLL] ) );
}

// as RPMod plays it: no angles is straight up
static void EM_Direction( vec3_t dir ) {
	vec3_t angles;

	if ( !EM_HaveAngles() ) {
		VectorSet( dir, 0, 0, 1 );
		return;
	}
	EM_GetAngles( angles );
	AngleVectors( angles, dir, NULL, NULL );
}

static int EM_TargetEntity( void ) {
	switch ( em.target ) {
	case EM_TARGET_PLAYER:	return em.targetClient;
	case EM_TARGET_NPC:		return em.targetNpc;
	default:				return clc.clientNum;
	}
}

static const char *EM_TargetLabel( void ) {
	switch ( em.target ) {
	case EM_TARGET_PLAYER:	return va( "%s " S_COLOR_GREY "(player %i)", EM_PlayerName( em.targetClient ), em.targetClient );
	case EM_TARGET_NPC:		return va( "%s " S_COLOR_GREY "(NPC %i)", em.targetName, em.targetNpc );
	default:				return va( "%s " S_COLOR_GREY "(you)", EM_PlayerName( clc.clientNum ) );
	}
}

// where cgame has the entity this frame
static void EM_LerpData( int entNum, vec3_t origin, vec3_t angles ) {
	TCGGetBoltData *data = (TCGGetBoltData *)cl.mSharedMemory;

	data->mEntityNum = entNum;
	CGVM_GetLerpData();
	VectorCopy( data->mOrigin, origin );
	if ( angles )
		VectorCopy( data->mAngles, angles );
}

// "1000" as sv_floodProtect gives it, how long to wait between commands
static int EM_CommandGap( void ) {
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

/*
===============================================================================

VIEWS

===============================================================================
*/

static qboolean EM_PassesFilter( const emEffect_t *e ) {
	return (qboolean)( em.filter == EM_FILTER_ALL || ( em.filter == EM_FILTER_CUSTOM ) == ( e->custom != qfalse ) );
}

static int EM_AddRow( emRowType_t type, const char *name, int first, qboolean custom ) {
	emRow_t *r;

	if ( em.numRows >= EM_MAX_ROWS )
		return -1;
	r = &em.rows[em.numRows];
	r->type = type;
	Q_strncpyz( r->name, name, sizeof( r->name ) );
	r->first = first;
	r->count = 0;
	r->custom = custom;
	return em.numRows++;
}

// the saved constructs, then Base and its folders, then Custom with each pk3 and its folders,
// of the effects the filter lets through
static void EM_BuildRows( void ) {
	int category = -1, source = -1, catRow = -1, srcRow = -1, folderRow = -1;

	em.numRows = 0;
	EM_AddRow( EM_ROW_SAVED, "Saved constructs", 0, qfalse );
	em.rows[0].count = em.numConstructs;
	EM_AddRow( EM_ROW_FAVES, "Favorites", 0, qfalse );
	for ( int i = 0; i < em.numEffects; i++ ) {
		if ( EM_PassesFilter( &em.effects[i] ) && CL_Fav_Has( EM_FAV_LIST, em.effects[i].path ) )
			em.rows[1].count++;
	}
	for ( int i = 0; i < em.numEffects; i++ ) {
		const emEffect_t *e = &em.effects[i];
		const int len = EM_FolderLen( e );

		// base and custom are each one block, so a range stays whole
		if ( !EM_PassesFilter( e ) )
			continue;
		if ( e->custom != category ) {
			category = e->custom;
			catRow = EM_AddRow( EM_ROW_CATEGORY, e->custom ? "Custom" : "Base", i, e->custom );
			source = -1;
			folderRow = -1;
		}
		if ( e->custom && e->source != source ) {
			source = e->source;
			srcRow = EM_AddRow( EM_ROW_SOURCE, em.sources[source], i, qtrue );
			folderRow = -1;
		}
		if ( folderRow < 0 || (int)strlen( em.rows[folderRow].name ) != len || Q_stricmpn( em.rows[folderRow].name, e->path, len ) ) {
			char folder[MAX_QPATH];

			Q_strncpyz( folder, e->path, Q_min( len + 1, (int)sizeof( folder ) ) );
			folderRow = EM_AddRow( EM_ROW_FOLDER, folder, i, e->custom );
		}
		if ( catRow < 0 || folderRow < 0 )
			break;	// out of rows
		em.rows[catRow].count++;
		if ( e->custom && srcRow >= 0 )
			em.rows[srcRow].count++;
		em.rows[folderRow].count++;
	}
}

static int EM_FirstFolderRow( void ) {
	for ( int i = 0; i < em.numRows; i++ ) {
		if ( em.rows[i].type == EM_ROW_FOLDER )
			return i;
	}
	return 0;
}

// the effect list shows the saved constructs
static qboolean EM_ConstructShown( void ) {
	return (qboolean)( em.row == 0 && !em.fields[EM_F_SEARCH][0] );
}

static void EM_RebuildEffectView( void ) {
	const char *search = em.fields[EM_F_SEARCH];

	em.numView = 0;
	if ( EM_ConstructShown() ) {
		for ( int i = 0; i < em.numConstructs; i++ )
			em.view[em.numView++] = i;
		em.effectList.sel = em.construct >= 0 && em.construct < em.numView ? em.construct : -1;
		EM_ListClamp( &em.effectList, em.numView, (int)( EM_LIST_H / EM_ROW_H ) );
		return;
	}
	if ( search[0] ) {
		for ( int i = 0; i < em.numEffects; i++ ) {
			if ( EM_PassesFilter( &em.effects[i] ) && EM_ContainsNoCase( em.effects[i].path, search ) )
				em.view[em.numView++] = i;
		}
	} else if ( em.row > 0 && em.row < em.numRows && em.rows[em.row].type == EM_ROW_FAVES ) {
		for ( int i = 0; i < em.numEffects && em.numView < EM_MAX_EFFECTS; i++ ) {
			if ( EM_PassesFilter( &em.effects[i] ) && CL_Fav_Has( EM_FAV_LIST, em.effects[i].path ) )
				em.view[em.numView++] = i;
		}
	} else if ( em.row > 0 && em.row < em.numRows ) {
		// a folder, a pk3 or all of Base or Custom
		const emRow_t *r = &em.rows[em.row];

		for ( int i = 0; i < r->count; i++ )
			em.view[em.numView++] = r->first + i;
	}

	// keep the selected effect selected if it is still listed
	em.effectList.sel = -1;
	for ( int i = 0; i < em.numView; i++ ) {
		if ( em.view[i] == em.effect ) {
			em.effectList.sel = i;
			break;
		}
	}
	EM_ListClamp( &em.effectList, em.numView, (int)( EM_LIST_H / EM_ROW_H ) );
}

static void EM_SelectConstruct( int index ) {
	if ( index < 0 || index >= em.numConstructs )
		return;
	if ( index != em.construct || !em.numCPieces ) {
		em.construct = index;
		em.numCPieces = Q_max( 0, EM_ReadConstruct( em.constructs[index], em.cpieces, EM_MAX_CPIECES ) );
	}
	for ( int i = 0; i < em.numView; i++ ) {
		if ( em.view[i] == index ) {
			EM_ListMove( &em.effectList, em.numView, (int)( EM_LIST_H / EM_ROW_H ), i - em.effectList.sel );
			break;
		}
	}
}

// the row on the left, its effects listed on the right; the saved constructs, the first selected
static void EM_SetRow( int row ) {
	if ( !em.numRows )
		return;
	em.row = Com_Clampi( 0, em.numRows - 1, row );
	em.folderList.sel = em.row;
	em.fields[EM_F_SEARCH][0] = '\0';
	em.effectList.sel = em.effectList.scroll = 0;
	EM_ListMove( &em.folderList, em.numRows, (int)( EM_LIST_H / EM_ROW_H ), 0 );
	if ( em.row == 0 ) {
		EM_ListConstructs();
		em.construct = -1;
	}
	EM_RebuildEffectView();
	if ( em.row == 0 && em.numConstructs )
		EM_SelectConstruct( 0 );
}

static void EM_ShowConstructs( void ) {
	EM_SetRow( 0 );
}

static void EM_StepFolder( int delta ) {
	EM_SetRow( em.row + delta );
}

static void EM_SetFilter( emFilter_t filter ) {
	const qboolean onSaved = (qboolean)( em.row == 0 );
	const qboolean onFaves = (qboolean)( em.row < em.numRows && em.rows[em.row].type == EM_ROW_FAVES );

	if ( filter == em.filter )
		return;
	em.filter = filter;
	EM_BuildRows();
	// the rows moved, but the saved constructs and the favorites stay first; a search stays
	if ( em.fields[EM_F_SEARCH][0] ) {
		em.row = onSaved ? 0 : onFaves ? 1 : EM_FirstFolderRow();
		em.effectList.sel = em.effectList.scroll = 0;
		EM_RebuildEffectView();
	} else {
		EM_SetRow( onSaved ? 0 : onFaves ? 1 : EM_FirstFolderRow() );
	}
}

// an effect starred or not any more: the Favorites row counts it, and lists it if it's the one shown
static void EM_ToggleFavorite( int index ) {
	const emEffect_t *e = &em.effects[index];
	const qboolean on = CL_Fav_Toggle( EM_FAV_LIST, e->path );

	for ( int i = 0; i < em.numRows; i++ ) {
		if ( em.rows[i].type == EM_ROW_FAVES )
			em.rows[i].count += on ? 1 : -1;
	}
	if ( !em.fields[EM_F_SEARCH][0] && em.row < em.numRows && em.rows[em.row].type == EM_ROW_FAVES )
		EM_RebuildEffectView();
}

static void EM_SelectEffect( int index ) {
	for ( int i = 0; i < em.numView; i++ ) {
		if ( em.view[i] == index ) {
			EM_ListMove( &em.effectList, em.numView, (int)( EM_LIST_H / EM_ROW_H ), i - em.effectList.sel );
			break;
		}
	}
	if ( index == em.effect )
		return;
	em.effect = index;
	em.selTime = cls.realtime;
}

// what is on the effect list's row: an effect, or a saved construct
static void EM_SelectRow( int row ) {
	if ( row < 0 || row >= em.numView )
		return;
	if ( EM_ConstructShown() )
		EM_SelectConstruct( em.view[row] );
	else
		EM_SelectEffect( em.view[row] );
}

static void EM_RebuildActiveView( void ) {
	const char *search = em.fields[EM_F_ACTIVE_SEARCH];

	em.numActiveView = 0;
	for ( int i = 0; i < em.numActive; i++ ) {
		const emActive_t *a = &em.active[i];

		if ( search[0] && !EM_ContainsNoCase( a->path, search ) && !EM_ContainsNoCase( va( "%i", a->num ), search ) )
			continue;
		em.activeView[em.numActiveView++] = i;
	}
	EM_ListClamp( &em.activeList, em.numActiveView, (int)( EM_LIST_H / EM_ROW_H ) );
}

static int EM_CountChecked( void ) {
	int count = 0;

	for ( int i = 0; i < em.numActive; i++ ) {
		if ( em.active[i].checked )
			count++;
	}
	return count;
}

static int EM_PickerCount( void ) {
	switch ( em.picker ) {
	case EM_PICKER_PLAYER:
	case EM_PICKER_REMOVE_PLAYER:	return MAX_CLIENTS;
	case EM_PICKER_NPC:
	case EM_PICKER_REMOVE_NPC:		return CL_NpcManager_NumNpcs();
	case EM_PICKER_BOLT:			return em.numBolts;
	case EM_PICKER_REMOVE_NAME:		return em.numNames;
	case EM_PICKER_STATE:			return em.numStateFiles;
	default:						return 0;
	}
}

// what the row shows and is searched by
static const char *EM_PickerItem( int index ) {
	int num;
	const char *type, *name;

	switch ( em.picker ) {
	case EM_PICKER_PLAYER:
	case EM_PICKER_REMOVE_PLAYER:
		return EM_PlayerName( index );
	case EM_PICKER_NPC:
	case EM_PICKER_REMOVE_NPC:
		if ( !CL_NpcManager_GetNpc( index, &num, &type, &name ) )
			return "";
		return va( "%i %s %s", num, name, type );
	case EM_PICKER_BOLT:
		return em.bolts[index].name;
	case EM_PICKER_REMOVE_NAME:
		return em.names[index];
	case EM_PICKER_STATE:
		return em.stateFiles[index];
	default:
		return "";
	}
}

static void EM_RebuildPickerView( void ) {
	const char *search = em.fields[EM_F_PICKER_SEARCH];
	int count = EM_PickerCount();

	em.numPickerView = 0;
	for ( int i = 0; i < count && em.numPickerView < (int)ARRAY_LEN( em.pickerView ); i++ ) {
		const char *item = EM_PickerItem( i );

		if ( item[0] && EM_ContainsNoCase( item, search ) )
			em.pickerView[em.numPickerView++] = i;
	}
	EM_ListClamp( &em.pickerList, em.numPickerView, (int)( ( EM_PICKER_H - 74 ) / EM_ROW_H ) );
}

/*
===============================================================================

THE TARGET'S MODEL

===============================================================================
*/

static void EM_AddBolt( const char *name, qboolean tag ) {
	if ( em.numBolts >= EM_MAX_BOLTS || !name[0] )
		return;
	for ( int i = 0; i < em.numBolts; i++ ) {
		if ( !Q_stricmp( em.bolts[i].name, name ) )
			return;
	}
	Q_strncpyz( em.bolts[em.numBolts].name, name, sizeof( em.bolts[0].name ) );
	em.bolts[em.numBolts].tag = tag;
	em.numBolts++;
}

// the *tag surfaces of a .glm
static void EM_ReadTags( const char *path ) {
	const mdxmHeader_t *header;
	const mdxmHierarchyOffsets_t *offsets;
	byte *buf;
	int len, numSurfaces;

	len = FS_ReadFile( path, (void **)&buf );
	if ( len <= 0 || !buf )
		return;
	header = (const mdxmHeader_t *)buf;
	numSurfaces = len >= (int)sizeof( *header ) ? LittleLong( header->numSurfaces ) : 0;
	if ( LittleLong( header->ident ) == MDXM_IDENT && numSurfaces > 0 && numSurfaces < len / 4
		&& (int)sizeof( *header ) + numSurfaces * 4 <= len ) {
		offsets = (const mdxmHierarchyOffsets_t *)( buf + sizeof( *header ) );
		for ( int i = 0; i < numSurfaces; i++ ) {
			int ofs = (int)sizeof( *header ) + LittleLong( offsets->offsets[i] );
			const mdxmSurfHierarchy_t *surf;

			if ( ofs < 0 || ofs > len - (int)sizeof( mdxmSurfHierarchy_t ) )
				break;
			surf = (const mdxmSurfHierarchy_t *)( buf + ofs );
			if ( surf->name[0] == '*' )
				EM_AddBolt( va( "%.*s", MAX_QPATH - 1, surf->name ), qtrue );
		}
	}
	FS_FreeFile( buf );
}

// the bones of a .gla
static void EM_ReadBones( const char *path ) {
	const mdxaHeader_t *header;
	const mdxaSkelOffsets_t *offsets;
	byte *buf;
	int len, numBones;

	len = FS_ReadFile( path, (void **)&buf );
	if ( len <= 0 || !buf )
		return;
	header = (const mdxaHeader_t *)buf;
	numBones = len >= (int)sizeof( *header ) ? LittleLong( header->numBones ) : 0;
	if ( LittleLong( header->ident ) == MDXA_IDENT && numBones > 0 && numBones < len / 4
		&& (int)sizeof( *header ) + numBones * 4 <= len ) {
		offsets = (const mdxaSkelOffsets_t *)( buf + sizeof( *header ) );
		for ( int i = 0; i < numBones; i++ ) {
			int ofs = (int)sizeof( *header ) + LittleLong( offsets->offsets[i] );
			const mdxaSkel_t *bone;

			if ( ofs < 0 || ofs > len - (int)sizeof( mdxaSkel_t ) )
				break;
			bone = (const mdxaSkel_t *)( buf + ofs );
			EM_AddBolt( va( "%.*s", MAX_QPATH - 1, bone->name ), qfalse );
		}
	}
	FS_FreeFile( buf );
}

static int QDECL EM_CompareBolts( const void *a, const void *b ) {
	const emBolt_t *ba = (const emBolt_t *)a, *bb = (const emBolt_t *)b;

	// tags first, the way RPMod's help lists them
	if ( ba->tag != bb->tag )
		return ba->tag ? -1 : 1;
	return ba->tag ? Q_stricmp( ba->name, bb->name ) : 0;
}

// the point a bolt is at, with the model at the origin facing yaw
static qboolean EM_BoltPointOn( CGhoul2Info_v &ghoul2, const char *name, float yaw, vec3_t out ) {
	mdxaBone_t matrix;
	vec3_t angles;
	int bolt;

	if ( !name[0] )
		return qfalse;
	bolt = re->G2API_AddBolt( ghoul2, 0, name );
	if ( bolt < 0 )
		return qfalse;
	VectorSet( angles, 0, yaw, 0 );
	if ( !re->G2API_GetBoltMatrix( ghoul2, 0, bolt, &matrix, angles, vec3_origin, cl.serverTime, NULL, em.g2Scale ) )
		return qfalse;
	VectorSet( out, matrix.matrix[0][3], matrix.matrix[1][3], matrix.matrix[2][3] );
	return qtrue;
}

static qboolean EM_BoltPoint( const char *name, float yaw, vec3_t out ) {
	return (qboolean)( em.g2View && EM_BoltPointOn( *em.g2View, name, yaw, out ) );
}

// the bolt box names something on the model
static qboolean EM_BoltExists( void ) {
	return (qboolean)( em.g2View && em.fields[EM_F_BOLT][0] && re->G2API_AddBolt( *em.g2View, 0, em.fields[EM_F_BOLT] ) >= 0 );
}

// the tags and bones it can be bolted to, and how big it is
static void EM_LoadBolts( void ) {
	char gla[MAX_QPATH];
	const char *glm, *anim;
	int numTags = 0;
	vec3_t p;

	em.numBolts = 0;
	if ( !em.g2 )
		return;
	glm = re->G2API_GetModelName( *em.g2, 0 );
	if ( glm && glm[0] )
		EM_ReadTags( glm );
	anim = re->G2API_GetGLAName( *em.g2, 0 );
	if ( anim && anim[0] ) {
		Q_strncpyz( gla, anim, sizeof( gla ) );
		if ( !strchr( gla, '.' ) )
			Q_strcat( gla, sizeof( gla ), ".gla" );
		EM_ReadBones( gla );
	}
	for ( int i = 0; i < em.numBolts; i++ ) {
		if ( em.bolts[i].tag )
			numTags++;
	}
	qsort( em.bolts, em.numBolts, sizeof( em.bolts[0] ), EM_CompareBolts );

	// frame the model view around its bones
	ClearBounds( em.g2Mins, em.g2Maxs );
	for ( int i = numTags; i < em.numBolts; i++ ) {
		if ( EM_BoltPoint( em.bolts[i].name, 0, p ) )
			AddPointToBounds( p, em.g2Mins, em.g2Maxs );
	}
	if ( em.g2Mins[0] > em.g2Maxs[0] ) {
		VectorSet( em.g2Mins, -16, -16, -24 );
		VectorSet( em.g2Maxs, 16, 16, 40 );
	}
	for ( int i = 0; i < 3; i++ ) {
		em.g2Mins[i] -= 8.0f;
		em.g2Maxs[i] += 8.0f;
	}
}

// "kyle/default" out of the model and skin file names
static void EM_DescribeModel( CGhoul2Info_v &ghoul2, char *out, int size ) {
	char model[MAX_QPATH], skin[MAX_QPATH];
	const char *glm = re->G2API_GetModelName( ghoul2, 0 ), *skinName = NULL, *p;
	qhandle_t customSkin = 0, ownSkin = 0;

	Q_strncpyz( model, glm ? glm : "", sizeof( model ) );
	if ( !Q_stricmpn( model, "models/players/", 15 ) ) {
		memmove( model, model + 15, strlen( model + 15 ) + 1 );
		if ( ( p = strchr( model, '/' ) ) != NULL )
			model[p - model] = '\0';
	}

	if ( re->ext.G2API_GetSkins )
		re->ext.G2API_GetSkins( ghoul2, 0, &customSkin, &ownSkin );
	skinName = CL_ShaderManager_SkinName( customSkin ? customSkin : ownSkin );
	skin[0] = '\0';
	if ( skinName ) {
		p = strrchr( skinName, '/' );
		Q_strncpyz( skin, p ? p + 1 : skinName, sizeof( skin ) );
		if ( !Q_stricmpn( skin, "model_", 6 ) )
			memmove( skin, skin + 6, strlen( skin + 6 ) + 1 );
		COM_StripExtension( skin, skin, sizeof( skin ) );
		if ( skin[0] == '|' )
			memmove( skin, skin + 1, strlen( skin + 1 ) + 1 );
	}
	Com_sprintf( out, size, "%s%s%s", model, skin[0] ? "/" : "", skin );
}

static void EM_RetireModel( void ) {
	int slot = 0;

	if ( !em.g2 )
		return;
	// effects already bolted to it keep using it for a while
	for ( int i = 0; i < EM_MAX_RETIRED; i++ ) {
		if ( !em.retired[i] ) {
			slot = i;
			break;
		}
		if ( em.retiredTime[i] < em.retiredTime[slot] )
			slot = i;
	}
	if ( em.retired[slot] )
		re->G2API_CleanGhoul2Models( &em.retired[slot] );
	em.retired[slot] = em.g2;
	em.retiredTime[slot] = cls.realtime;
	em.g2 = NULL;
	em.g2Key[0] = '\0';
	em.numBolts = 0;
	// no effect uses the view's copy
	if ( em.g2View )
		re->G2API_CleanGhoul2Models( &em.g2View );
	em.g2View = NULL;
}

static void EM_FreeModels( void ) {
	if ( em.g2 )
		re->G2API_CleanGhoul2Models( &em.g2 );
	em.g2 = NULL;
	if ( em.g2View )
		re->G2API_CleanGhoul2Models( &em.g2View );
	em.g2View = NULL;
	em.g2Key[0] = '\0';
	em.numBolts = 0;
	for ( int i = 0; i < EM_MAX_RETIRED; i++ ) {
		if ( em.retired[i] )
			re->G2API_CleanGhoul2Models( &em.retired[i] );
		em.retired[i] = NULL;
	}
}

static qboolean EM_WantTarget( void ) {
	if ( em.state == EM_OFF || em.tab != EM_TAB_EFFECTS )
		return qfalse;
	return (qboolean)( em.where == EM_WHERE_BOLT || em.picker == EM_PICKER_BOLT );
}

// finds the target among the models cgame drew this frame, and copies it when it changed
static void EM_UpdateTarget( void ) {
	const emCaptured_t *best = NULL;
	float bestDist = EM_TARGET_XY * EM_TARGET_XY;
	vec3_t org;
	char key[sizeof( em.g2Key )];
	qhandle_t customSkin = 0, ownSkin = 0;
	const char *glm;
	int ent;

	if ( !EM_WantTarget() || !cls.cgameStarted )
		return;
	ent = EM_TargetEntity();
	EM_LerpData( ent, org, NULL );

	for ( int i = 0; i < em.numPending; i++ ) {
		const emCaptured_t *c = &em.pending[i];
		float dx = c->origin[0] - org[0], dy = c->origin[1] - org[1];

		if ( fabsf( c->origin[2] - org[2] ) > EM_TARGET_Z || dx * dx + dy * dy >= bestDist )
			continue;
		bestDist = dx * dx + dy * dy;
		best = c;
	}
	em.targetSeen = (qboolean)( best != NULL );
	if ( !best ) {
		// don't bolt to the last target's copy
		if ( em.g2 && em.g2Ent != ent )
			EM_RetireModel();
		return;
	}

	glm = re->G2API_GetModelName( *best->ghoul2, 0 );
	if ( re->ext.G2API_GetSkins )
		re->ext.G2API_GetSkins( *best->ghoul2, 0, &customSkin, &ownSkin );
	Com_sprintf( key, sizeof( key ), "%i|%s|%i|%i", ent, glm ? glm : "", customSkin, ownSkin );
	if ( em.g2 && !strcmp( key, em.g2Key ) )
		return;

	EM_RetireModel();
	re->G2API_DuplicateGhoul2Instance( *best->ghoul2, &em.g2 );
	re->G2API_DuplicateGhoul2Instance( *best->ghoul2, &em.g2View );
	if ( !em.g2 || !em.g2View ) {
		EM_FreeModels();
		return;
	}
	Q_strncpyz( em.g2Key, key, sizeof( em.g2Key ) );
	em.g2Ent = ent;
	VectorCopy( best->scale, em.g2Scale );
	EM_DescribeModel( *em.g2, em.g2Model, sizeof( em.g2Model ) );
	EM_LoadBolts();
}

/*
===============================================================================

PREVIEW

===============================================================================
*/

// loads the effect once the selection rests on it; 0 if it can't be played (yet)
static int EM_EffectId( emEffect_t *e ) {
	if ( !e || e->failed )
		return 0;
	if ( !e->id ) {
		if ( em.state == EM_BROWSE && cls.realtime - em.selTime < EM_SETTLE_MS )
			return 0;
		e->id = FX_RegisterEffect( va( "%s/%s", EM_ROOT, e->path ) );
		if ( !e->id )
			e->failed = qtrue;
	}
	return e->id;
}

static int EM_PlayPeriod( int id ) {
	int length = 0, repeatDelay = 0;

	if ( !theFxScheduler.GetEffectTiming( id, &length, &repeatDelay ) )
		return 1000;
	if ( !em.loop )
		length += EM_ONCE_GAP;
	return length < EM_MIN_PERIOD ? EM_MIN_PERIOD : length;
}

// the ray through a point of the 2D screen, from the main view last frame
static qboolean EM_ScreenRay( float sx, float sy, vec3_t dir ) {
	float x, y;

	if ( !em.haveView )
		return qfalse;
	x = ( sx / ( SCREEN_WIDTH * 0.5f ) - 1.0f ) * tanf( DEG2RAD( em.fovX * 0.5f ) );
	y = ( sy / ( SCREEN_HEIGHT * 0.5f ) - 1.0f ) * tanf( DEG2RAD( em.fovY * 0.5f ) );
	VectorCopy( em.viewAxis[0], dir );
	VectorMA( dir, -x, em.viewAxis[1], dir );
	VectorMA( dir, -y, em.viewAxis[2], dir );
	VectorNormalize( dir );
	return qtrue;
}

static qboolean EM_Project( const vec3_t point, float *x, float *y ) {
	vec3_t d;
	float forward;

	if ( !em.haveView )
		return qfalse;
	VectorSubtract( point, em.viewOrg, d );
	forward = DotProduct( d, em.viewAxis[0] );
	if ( forward < 4.0f )
		return qfalse;
	*x = SCREEN_WIDTH * 0.5f * ( 1.0f - DotProduct( d, em.viewAxis[1] ) / forward / tanf( DEG2RAD( em.fovX * 0.5f ) ) );
	*y = SCREEN_HEIGHT * 0.5f * ( 1.0f - DotProduct( d, em.viewAxis[2] ) / forward / tanf( DEG2RAD( em.fovY * 0.5f ) ) );
	return qtrue;
}

// the middle of the preview window, out in the world, short of any wall
static qboolean EM_WindowPoint( vec3_t out ) {
	vec3_t dir, end;
	trace_t tr;

	if ( !EM_ScreenRay( EM_RIGHT_X + EM_RIGHT_W * 0.5f, EM_VIEW_Y + EM_VIEW_H * 0.5f, dir ) )
		return qfalse;
	VectorMA( em.viewOrg, em.previewDist, dir, end );
	CM_BoxTrace( &tr, em.viewOrg, end, vec3_origin, vec3_origin, 0, EM_TRACE_MASK, qfalse );
	VectorMA( em.viewOrg, Q_max( 8.0f, em.previewDist * tr.fraction - 8.0f ), dir, out );
	return qtrue;
}

static qboolean EM_BoltExists( void );

static int EM_BoltIndex( void ) {
	if ( !em.g2 || !EM_BoltExists() )
		return -1;
	return re->G2API_AddBolt( *em.g2, 0, em.fields[EM_F_BOLT] );
}

static void EM_ItemDirection( const emItem_t *it, vec3_t dir ) {
	if ( it->haveAngles )
		AngleVectors( it->angles, dir, NULL, NULL );
	else
		VectorSet( dir, 0, 0, 1 );
}

// the effects waiting at a position, playing where they will be, on this client only
static void EM_PreviewWaiting( void ) {
	vec3_t dir;
	int index, id, length, repeatDelay;

	for ( int i = 0; i < em.numWaiting; i++ ) {
		emItem_t *it = &em.waiting[i];

		if ( it->bolted || !it->havePos || cls.realtime < it->nextPlay )
			continue;
		index = EM_FindEffect( it->path );
		id = index >= 0 ? EM_EffectId( &em.effects[index] ) : 0;
		if ( !id ) {
			it->nextPlay = cls.realtime + 1000;
			continue;
		}
		length = repeatDelay = 0;
		if ( !theFxScheduler.GetEffectTiming( id, &length, &repeatDelay ) )
			length = 1000;
		if ( !it->loop )
			length += EM_ONCE_GAP;
		it->nextPlay = cls.realtime + Q_max( EM_MIN_PERIOD, length );
		EM_ItemDirection( it, dir );
		FX_NextScale( EM_ClampScale( it->scale ) );
		FX_PlayEffectID( id, it->pos, dir, -1, -1 );
	}

	// the map's moved ones where they're going; the server's copy still plays where it was
	for ( int i = 0; i < em.numActive; i++ ) {
		emActive_t *a = &em.active[i];
		char path[MAX_QPATH];

		if ( a->sent || a->deleted || a->bolted || cls.realtime < a->nextPlay || !EM_ActiveEdited( a ) )
			continue;
		EM_NormPath( a->path, path, sizeof( path ) );
		index = EM_FindEffect( path );
		id = index >= 0 ? EM_EffectId( &em.effects[index] ) : 0;
		if ( !id ) {
			a->nextPlay = cls.realtime + 1000;
			continue;
		}
		length = repeatDelay = 0;
		if ( !theFxScheduler.GetEffectTiming( id, &length, &repeatDelay ) )
			length = 1000;
		a->nextPlay = cls.realtime + Q_max( EM_MIN_PERIOD, length );
		if ( a->haveAngles )
			AngleVectors( a->angles, dir, NULL, NULL );
		else
			VectorSet( dir, 0, 0, 1 );
		FX_NextScale( EM_ScaleAt( a->path, a->origin ) );
		FX_PlayEffectID( id, a->pos, dir, -1, -1 );
	}
}

// plays the selected effect where it would go, again and again, on this client only
static void EM_PreviewFrame( void ) {
	emEffect_t *e = em.effect >= 0 && em.effect < em.numEffects ? &em.effects[em.effect] : NULL;
	qboolean bolted = (qboolean)( em.where == EM_WHERE_BOLT );
	char key[sizeof( em.playKey )];
	vec3_t org, dir;
	int id, bolt = -1;

	em.playError = NULL;
	if ( em.state == EM_PLACE )
		EM_PreviewWaiting();
	// a construct in hand plays where each of its effects would go
	if ( em.state == EM_PLACE && em.handConstruct ) {
		for ( int i = 0; i < em.numHand; i++ ) {
			emCPiece_t *p = &em.hand[i];
			emItem_t it;
			int index, length = 0, repeatDelay = 0;

			if ( cls.realtime < p->nextPlay )
				continue;
			index = EM_FindEffect( p->path );
			id = index >= 0 ? EM_EffectId( &em.effects[index] ) : 0;
			if ( !id ) {
				p->nextPlay = cls.realtime + 1000;
				continue;
			}
			if ( !theFxScheduler.GetEffectTiming( id, &length, &repeatDelay ) )
				length = 1000;
			if ( !p->loop )
				length += EM_ONCE_GAP;
			p->nextPlay = cls.realtime + Q_max( EM_MIN_PERIOD, length );
			EM_HandItem( i, em.origin, &it );
			EM_ItemDirection( &it, dir );
			FX_PlayEffectID( id, it.pos, dir, -1, -1 );
		}
		return;
	}
	// selecting, nothing is in hand to play
	if ( !e || em.picker != EM_PICKER_NONE || ( em.state == EM_PLACE && em.selecting ) )
		return;
	if ( em.state == EM_BROWSE ) {
		// bolted effects are shown on the model in the window, and played in the world from "View in world"
		if ( em.tab != EM_TAB_EFFECTS || bolted || EM_ConstructShown() )
			return;
	} else if ( em.state != EM_PLACE ) {
		return;
	}

	id = EM_EffectId( e );
	if ( !id ) {
		if ( e->failed )
			em.playError = "The effects system couldn't load it";
		return;
	}

	if ( bolted ) {
		bolt = EM_BoltIndex();
		if ( !em.g2 ) {
			em.playError = "Target not in view";
			return;
		}
		if ( bolt < 0 ) {
			em.playError = "Its model has no such bolt";
			return;
		}
		if ( bolt > EM_MAX_BOLT_INDEX ) {
			em.playError = "Too many bolts on this model to show it here";
			return;
		}
	}

	Com_sprintf( key, sizeof( key ), "%i|%i|%i|%i|%s|%s", em.effect, em.loop, bolted, em.state, bolted ? em.g2Key : "", bolted ? em.fields[EM_F_BOLT] : "" );
	if ( strcmp( key, em.playKey ) ) {
		Q_strncpyz( em.playKey, key, sizeof( em.playKey ) );
		em.nextPlay = cls.realtime;
	}
	if ( cls.realtime < em.nextPlay )
		return;
	em.nextPlay = cls.realtime + EM_PlayPeriod( id );

	if ( bolted ) {
		int boltInfo = 0;

		EM_LerpData( em.g2Ent, org, NULL );
		if ( re->G2API_AttachEnt( &boltInfo, *em.g2, 0, bolt, em.g2Ent, 0 ) )
			FX_PlayBoltedEffectID( id, org, boltInfo, em.g2, 0, qtrue );
		return;
	}

	if ( em.state == EM_PLACE )
		VectorCopy( em.origin, org );
	else if ( !EM_WindowPoint( org ) )
		return;
	EM_Direction( dir );
	FX_NextScale( EM_ClampScale( em.scale ) );
	FX_PlayEffectID( id, org, dir, -1, -1 );
}

/*
===============================================================================

COMMANDS

===============================================================================
*/

static void EM_Queue( const char *cmd, qboolean echo ) {
	// opened from the cinematic manager, an effect put down goes in the cinematic
	if ( CL_Cinematic_CaptureEffect( cmd, em.scale ) )
		return;
	// opened from the storyteller, it goes in the story instead
	if ( CL_Storyteller_Capture( cmd ) )
		return;
	if ( em.numQueue >= EM_MAX_QUEUE ) {
		Com_Printf( S_COLOR_YELLOW "Effect manager: too many commands waiting, dropped %s\n", cmd );
		return;
	}
	Q_strncpyz( em.queue[em.numQueue], cmd, sizeof( em.queue[0] ) );
	em.queueEcho[em.numQueue] = echo;
	em.numQueue++;
}

// one command at a time, as fast as the server's flood protection lets them through
static void EM_RunQueue( void ) {
	if ( !em.numQueue || cls.realtime < em.nextSend || cls.state != CA_ACTIVE )
		return;

	CL_AddReliableCommand( em.queue[0], qfalse );
	if ( em.queueEcho[0] ) {
		em.sentTime = cls.realtime;
		Q_strncpyz( em.lastCmd, em.queue[0], sizeof( em.lastCmd ) );
		Com_Printf( S_COLOR_GREEN "Effect manager: " S_COLOR_WHITE "%s\n", em.queue[0] );
	} else if ( !Q_stricmp( em.queue[0], "rpeffect list" ) ) {
		em.listRequestTime = cls.realtime;
		em.listQueued = qfalse;
	}
	em.numQueue--;
	memmove( em.queue[0], em.queue[1], em.numQueue * sizeof( em.queue[0] ) );
	memmove( &em.queueEcho[0], &em.queueEcho[1], em.numQueue * sizeof( em.queueEcho[0] ) );
	em.nextSend = cls.realtime + EM_CommandGap();
}

// the list once the commands before it went through; one already waiting moves behind
// what was queued since, so it shows that too
static void EM_RequestList( void ) {
	em.refreshTime = 0;
	for ( int i = 0; i < em.numQueue && em.listQueued; i++ ) {
		if ( em.queueEcho[i] || Q_stricmp( em.queue[i], "rpeffect list" ) )
			continue;
		if ( i == em.numQueue - 1 )
			return;
		em.numQueue--;
		memmove( em.queue[i], em.queue[i + 1], ( em.numQueue - i ) * sizeof( em.queue[0] ) );
		memmove( &em.queueEcho[i], &em.queueEcho[i + 1], ( em.numQueue - i ) * sizeof( em.queueEcho[0] ) );
		break;
	}
	em.listQueued = qtrue;
	EM_Queue( "rpeffect list", qfalse );
}

// the list again once the commands before it went through
static void EM_SendAndRefresh( const char *cmd ) {
	EM_Queue( cmd, qtrue );
	EM_RequestList();
}

static void EM_Message( const char *text ) {
	Q_strncpyz( em.message, text, sizeof( em.message ) );
	em.messageTime = cls.realtime;
}

/*
===============================================================================

SIZES (experimental)

An effect can be drawn bigger or smaller than it was made, on this screen: the effects system
asks CL_EffectManager_FxScale how big one is as it plays, and finds it here by where it is.
Shared, the sizes go to everyone with this client in an rpnotify:
	cmd:fxscale/<x y z>/<size>[/once]: <effect>
a size of 1 putting it back; once, for an effect that only plays once, lasts a moment.
They're kept until the map changes, and sent again a little after someone joins.

===============================================================================
*/

#define EM_MAX_SCALED		128
#define EM_SCALE_NEAR		4.0f		// an effect this near where a size was set is that one
#define EM_SCALE_MIN		0.1f
#define EM_SCALE_MAX		8.0f
#define EM_SCALE_STEP		1.25f
#define EM_SCALE_ONCE_MS	4000		// a one-off effect's size lasts this long
#define EM_SCALE_JOIN_MS	4000
#define EM_SCALE_WORD		30			// RPMod chokes on long words: a space every so often

typedef struct {
	char		key[MAX_QPATH];		// the effect, as EM_ScaleKey has it
	vec3_t		org;
	float		scale;
	int			until;				// cls.realtime it goes, 0 never
	qboolean	mine;				// set here, sent again when someone joins
} emScaled_t;

static emScaled_t emScaled[EM_MAX_SCALED];
static int emNumScaled;
static char emScaledMap[MAX_QPATH];

static float EM_ClampScale( float scale ) {
	return scale > 0.0f ? Com_Clamp( EM_SCALE_MIN, EM_SCALE_MAX, scale ) : 1.0f;
}

// an effect's name as the effects system keys it: no effects/ in front, no .efx, lower case
static void EM_ScaleKey( const char *in, char *out, int size ) {
	char *c;

	while ( *in == '/' || *in == '\\' )
		in++;
	if ( !Q_stricmpn( in, "effects/", 8 ) || !Q_stricmpn( in, "effects\\", 8 ) )
		in += 8;
	COM_StripExtension( in, out, size );
	for ( c = out; *c; c++ ) {
		if ( *c == '\\' )
			*c = '/';
	}
	Q_strlwr( out );
}

// a new map starts with none; one-offs go once played
static void EM_ScaleTidy( void ) {
	int keep = 0;

	if ( Q_stricmp( emScaledMap, cl.mapname ) ) {
		Q_strncpyz( emScaledMap, cl.mapname, sizeof( emScaledMap ) );
		emNumScaled = 0;
		return;
	}
	for ( int i = 0; i < emNumScaled; i++ ) {
		if ( emScaled[i].until && cls.realtime > emScaled[i].until )
			continue;
		emScaled[keep++] = emScaled[i];
	}
	emNumScaled = keep;
}

static emScaled_t *EM_FindScaled( const char *key, const vec3_t org ) {
	emScaled_t *best = NULL;
	float bestDist = EM_SCALE_NEAR;

	for ( int i = 0; i < emNumScaled; i++ ) {
		const float d = Distance( emScaled[i].org, org );

		if ( d <= bestDist && !strcmp( emScaled[i].key, key ) ) {
			bestDist = d;
			best = &emScaled[i];
		}
	}
	return best;
}

// how big the effect playing here is drawn, 1 as made
float CL_EffectManager_FxScale( const char *effect, const vec3_t org ) {
	char key[MAX_QPATH];
	const emScaled_t *e;

	if ( !emNumScaled || !org || !effect )
		return 1.0f;
	EM_ScaleTidy();
	EM_ScaleKey( effect, key, sizeof( key ) );
	e = EM_FindScaled( key, org );
	return e && ( !e->until || cls.realtime <= e->until ) ? e->scale : 1.0f;
}

static float EM_ScaleAt( const char *path, const vec3_t org ) {
	return CL_EffectManager_FxScale( path, org );
}

static void EM_StoreScale( const char *path, const vec3_t org, float scale, qboolean once, qboolean mine ) {
	char key[MAX_QPATH];
	emScaled_t *e;

	EM_ScaleTidy();
	EM_ScaleKey( path, key, sizeof( key ) );
	e = EM_FindScaled( key, org );
	scale = EM_ClampScale( scale );
	// as made: forgotten
	if ( fabsf( scale - 1.0f ) < 0.001f ) {
		if ( e ) {
			*e = emScaled[--emNumScaled];
		}
		return;
	}
	if ( !e ) {
		if ( emNumScaled >= EM_MAX_SCALED )
			memmove( &emScaled[0], &emScaled[1], ( --emNumScaled ) * sizeof( emScaled[0] ) );
		e = &emScaled[emNumScaled++];
		Q_strncpyz( e->key, key, sizeof( e->key ) );
	}
	VectorCopy( org, e->org );
	e->scale = scale;
	e->until = once ? cls.realtime + EM_SCALE_ONCE_MS : 0;
	e->mine = mine;
}

static void EM_Queue( const char *cmd, qboolean echo );

// to everyone with this client, if it can be
static void EM_ShareScale( const char *path, const vec3_t org, float scale, qboolean once ) {
	char split[MAX_QPATH * 2];
	int run = 0, n = 0;

	if ( !CL_RpHooks_Experimental() ) {
		if ( !em.shareWarned ) {
			em.shareWarned = qtrue;
			EM_Message( S_COLOR_YELLOW "Sizes show on your screen only: TOGGLE EXPERIMENTAL in the npcmanager to share them" );
		}
		return;
	}
	// the name with a space now and then, taken out again where it's read
	for ( const char *c = path; *c && n < (int)sizeof( split ) - 2; c++ ) {
		split[n++] = *c;
		if ( ++run >= EM_SCALE_WORD || *c == '/' ) {
			split[n++] = ' ';
			run = 0;
		}
	}
	split[n] = '\0';
	EM_Queue( va( "rpnotify -1 1 cmd:fxscale/%i %i %i/%g%s: %s", (int)floorf( org[0] + 0.5f ), (int)floorf( org[1] + 0.5f ), (int)floorf( org[2] + 0.5f ),
		floorf( EM_ClampScale( scale ) * 100.0f + 0.5f ) / 100.0f, once ? "/once" : "", split ), qfalse );
}

// a size set here: shown now, and shared
static void EM_SetScale( const char *path, const vec3_t org, float scale, qboolean once ) {
	vec3_t at;

	for ( int i = 0; i < 3; i++ )
		at[i] = floorf( org[i] + 0.5f );
	EM_StoreScale( path, at, scale, once, qtrue );
	EM_ShareScale( path, at, scale, once );
}

// an effect sent at a place, as big as it was asked to be
static void EM_SentScale( const emItem_t *it ) {
	if ( CL_Cinematic_PickingEffect() || it->bolted || !it->havePos || fabsf( EM_ClampScale( it->scale ) - 1.0f ) < 0.001f )
		return;
	EM_SetScale( it->path, it->pos, it->scale, (qboolean)!it->loop );
}

// cmd:fxscale/<x y z>/<size>[/once]: <effect>
void CL_EffectManager_ScaleHook( char **argv, int argc, const char *message ) {
	char path[MAX_QPATH];
	vec3_t org;
	int n = 0;

	if ( argc < 2 || sscanf( argv[0], "%f %f %f", &org[0], &org[1], &org[2] ) != 3 )
		return;
	for ( const char *c = message; *c && n < (int)sizeof( path ) - 1; c++ ) {
		if ( *c != ' ' )
			path[n++] = *c;
	}
	path[n] = '\0';
	if ( !path[0] )
		return;
	EM_StoreScale( path, org, (float)atof( argv[1] ), (qboolean)( argc > 2 && !Q_stricmp( argv[2], "once" ) ), qfalse );
}

// someone joining gets the sizes set here a little later
static void EM_ScaleShareFrame( void ) {
	int players = 0, mine = 0;

	if ( cls.state != CA_ACTIVE )
		return;
	for ( int c = 0; c < MAX_CLIENTS; c++ ) {
		if ( cl.gameState.stringData[cl.gameState.stringOffsets[CS_PLAYERS + c]] )
			players++;
	}
	for ( int i = 0; i < emNumScaled; i++ ) {
		if ( emScaled[i].mine && !emScaled[i].until )
			mine++;
	}
	if ( players > em.sharePlayers && em.sharePlayers && mine )
		em.shareAt = cls.realtime + EM_SCALE_JOIN_MS;
	em.sharePlayers = players;
	if ( em.shareAt && cls.realtime >= em.shareAt ) {
		em.shareAt = 0;
		for ( int i = 0; i < emNumScaled; i++ ) {
			if ( emScaled[i].mine && !emScaled[i].until )
				EM_ShareScale( emScaled[i].key, emScaled[i].org, emScaled[i].scale, qfalse );
		}
	}
}

// the effect the form describes, where it would go now; qfalse if something is missing
static qboolean EM_FormItem( emItem_t *out ) {
	const emEffect_t *e = EM_Effect();

	memset( out, 0, sizeof( *out ) );
	if ( !e )
		return qfalse;
	Q_strncpyz( out->path, e->path, sizeof( out->path ) );
	out->loop = em.loop;
	out->scale = EM_ClampScale( em.scale );
	Q_strncpyz( out->tag, em.fields[EM_F_NAME], sizeof( out->tag ) );

	if ( em.where == EM_WHERE_BOLT ) {
		if ( !em.fields[EM_F_BOLT][0] || ( em.target == EM_TARGET_NPC && !em.targetName[0] ) )
			return qfalse;
		out->bolted = qtrue;
		Q_strncpyz( out->bolt, em.fields[EM_F_BOLT], sizeof( out->bolt ) );
		out->target = em.target;
		out->targetClient = em.targetClient;
		out->targetNpc = em.targetNpc;
		Q_strncpyz( out->targetName, em.targetName, sizeof( out->targetName ) );
		return qtrue;
	}

	if ( em.state == EM_PLACE ) {
		out->havePos = qtrue;
		VectorCopy( em.origin, out->pos );
	} else if ( em.where == EM_WHERE_POS && em.havePos ) {
		out->havePos = qtrue;
		VectorCopy( em.pos, out->pos );
	}
	out->haveAngles = EM_HaveAngles();
	EM_GetAngles( out->angles );
	return qtrue;
}

static const char *EM_ItemCommand( const emItem_t *it ) {
	static char cmd[MAX_STRING_CHARS];

	Com_sprintf( cmd, sizeof( cmd ), "rpeffect %s %s", it->loop ? "add" : "once", it->path );
	if ( it->bolted ) {
		Q_strcat( cmd, sizeof( cmd ), va( " bolted %s", it->bolt ) );
		if ( it->target == EM_TARGET_PLAYER )
			Q_strcat( cmd, sizeof( cmd ), va( " %i", it->targetClient ) );
		else if ( it->target == EM_TARGET_NPC )
			Q_strcat( cmd, sizeof( cmd ), va( " npc %s", it->targetName ) );
		return cmd;
	}

	// positional: angles need the coordinates before them, which 0 0 0 leaves to the server
	if ( it->havePos )
		Q_strcat( cmd, sizeof( cmd ), va( " %i %i %i", (int)floorf( it->pos[0] + 0.5f ), (int)floorf( it->pos[1] + 0.5f ), (int)floorf( it->pos[2] + 0.5f ) ) );
	if ( it->haveAngles ) {
		if ( !it->havePos )
			Q_strcat( cmd, sizeof( cmd ), " 0 0 0" );
		Q_strcat( cmd, sizeof( cmd ), va( " %i %i %i", EM_NormalizeAngle( it->angles[PITCH] ), EM_NormalizeAngle( it->angles[YAW] ), EM_NormalizeAngle( it->angles[ROLL] ) ) );
	}
	return cmd;
}

// the command for the selected effect, NULL if something is missing
static const char *EM_BuildCommand( void ) {
	emItem_t it;

	if ( !EM_FormItem( &it ) )
		return NULL;
	return EM_ItemCommand( &it );
}

// a looping one stays on the map, to be found in the next list
static void EM_Expect( const emItem_t *it ) {
	emExpect_t *x;

	if ( !it->loop )
		return;
	if ( em.numExpect >= EM_MAX_EXPECT ) {
		memmove( &em.expect[0], &em.expect[1], ( EM_MAX_EXPECT - 1 ) * sizeof( em.expect[0] ) );
		em.numExpect--;
	}
	x = &em.expect[em.numExpect++];
	Q_strncpyz( x->path, it->path, sizeof( x->path ) );
	x->anyPos = (qboolean)( it->bolted || !it->havePos );
	VectorCopy( it->pos, x->pos );
	x->bolted = it->bolted;
	x->haveAngles = it->haveAngles;
	VectorCopy( it->angles, x->angles );
	Q_strncpyz( x->tag, it->tag, sizeof( x->tag ) );
	x->selected = it->selected;
	x->time = cls.realtime;
}

static void EM_SendItem( const emItem_t *it ) {
	EM_Expect( it );
	if ( it->loop )
		EM_SendAndRefresh( EM_ItemCommand( it ) );
	else
		EM_Queue( EM_ItemCommand( it ), qtrue );
	EM_SentScale( it );
}

static void EM_Send( void ) {
	emItem_t it;

	if ( !EM_FormItem( &it ) )
		return;
	if ( em.state == EM_PLACE && em.where != EM_WHERE_BOLT ) {
		// what was placed is the position from now on
		VectorCopy( em.origin, em.pos );
		em.havePos = qtrue;
		em.where = EM_WHERE_POS;
	}
	EM_SendItem( &it );
}

// the form's effect waits, to go with the others
static void EM_AddWaiting( void ) {
	if ( em.numWaiting >= EM_MAX_WAITING ) {
		EM_Message( S_COLOR_YELLOW "That's as many as can wait at once; send them first" );
		return;
	}
	if ( !EM_FormItem( &em.waiting[em.numWaiting] ) )
		return;
	em.waiting[em.numWaiting].nextPlay = cls.realtime;
	em.numWaiting++;
}

static void EM_SendWaiting( void ) {
	const int count = em.numWaiting;

	if ( !count )
		return;
	// one list after them all
	for ( int i = 0; i < count; i++ ) {
		EM_Expect( &em.waiting[i] );
		EM_Queue( EM_ItemCommand( &em.waiting[i] ), qtrue );
		EM_SentScale( &em.waiting[i] );
	}
	EM_RequestList();
	em.numWaiting = 0;
	EM_Message( va( S_COLOR_GREEN "Sending %i effects, about one a second", count ) );
}

/*
===============================================================================

THE ACTIVE LIST

===============================================================================
*/

// "109: ( 2233 3289 -23) effects/scepter/invincibility", colors already stripped
static qboolean EM_ParseActiveLine( const char *line, const emActive_t *old, int numOld ) {
	emActive_t *a;
	float x, y, z;
	int num, used = 0;
	const char *rest;

	if ( sscanf( line, "%d: (%f %f %f)%n", &num, &x, &y, &z, &used ) != 4 || !used )
		return qfalse;
	if ( em.numActive >= EM_MAX_ACTIVE )
		return qtrue;

	a = &em.active[em.numActive];
	a->num = num;
	VectorSet( a->origin, x, y, z );
	rest = line + used;
	while ( *rest == ' ' )
		rest++;
	Q_strncpyz( a->path, rest, sizeof( a->path ) );
	a->checked = qfalse;
	a->tag[0] = '\0';
	a->placed = a->bolted = qfalse;
	a->fresh = qtrue;
	VectorCopy( a->origin, a->pos );
	VectorClear( a->angles );
	VectorClear( a->srvAngles );
	a->haveAngles = a->srvHaveAngles = qfalse;
	a->selected = a->deleted = a->sent = qfalse;
	// the same effect as before keeps what it had
	for ( int i = 0; i < numOld; i++ ) {
		const emActive_t *o = &old[i];

		if ( o->num != num || Q_stricmp( o->path, a->path ) )
			continue;
		a->checked = o->checked;
		Q_strncpyz( a->tag, o->tag, sizeof( a->tag ) );
		a->placed = o->placed;
		a->bolted = o->bolted;
		a->fresh = qfalse;
		a->haveAngles = a->srvHaveAngles = o->srvHaveAngles;
		VectorCopy( o->srvAngles, a->srvAngles );
		VectorCopy( o->srvAngles, a->angles );
		// what was done to it and not sent yet; something sent is as the list has it
		if ( !o->sent ) {
			VectorCopy( o->pos, a->pos );
			VectorCopy( o->angles, a->angles );
			a->haveAngles = o->haveAngles;
			a->selected = o->selected;
			a->deleted = o->deleted;
			// in hand while the list came
			VectorCopy( o->carryFrom, a->carryFrom );
			VectorCopy( o->carryOffset, a->carryOffset );
			VectorCopy( o->carryAngles, a->carryAngles );
			a->carryHaveAngles = o->carryHaveAngles;
		}
		break;
	}
	em.numActive++;
	return qtrue;
}

// "effects/scepter/invincibility.efx" and the like as "scepter/invincibility"
static void EM_NormPath( const char *in, char *out, int size ) {
	const int rootLen = strlen( EM_ROOT ), extLen = strlen( EM_EXT );
	int len;

	if ( !Q_stricmpn( in, EM_ROOT "/", rootLen + 1 ) || !Q_stricmpn( in, EM_ROOT "\\", rootLen + 1 ) )
		in += rootLen + 1;
	Q_strncpyz( out, in, size );
	len = strlen( out );
	if ( len > extLen && !Q_stricmp( out + len - extLen, EM_EXT ) )
		out[len - extLen] = '\0';
	for ( char *c = out; *c; c++ ) {
		if ( *c == '\\' )
			*c = '/';
	}
}

static qboolean EM_SamePath( const char *a, const char *b ) {
	char na[MAX_QPATH], nb[MAX_QPATH];

	EM_NormPath( a, na, sizeof( na ) );
	EM_NormPath( b, nb, sizeof( nb ) );
	return (qboolean)!Q_stricmp( na, nb );
}

// the active list is for one map of one server; a vid_restart keeps it, and its names
static void EM_CheckMap( void ) {
	char key[MAX_STRING_CHARS];

	Com_sprintf( key, sizeof( key ), "%s %s", cls.servername,
		Info_ValueForKey( cl.gameState.stringData + cl.gameState.stringOffsets[CS_SERVERINFO], "mapname" ) );
	if ( !Q_stricmp( key, em.activeMap ) )
		return;
	Q_strncpyz( em.activeMap, key, sizeof( em.activeMap ) );
	em.numActive = em.numActiveView = 0;
	em.numWaiting = em.numExpect = 0;
	em.reloading = qfalse;
}

// the effects new in the list that this manager sent take what they were sent with
static void EM_MatchExpected( void ) {
	for ( int i = 0; i < em.numExpect; ) {
		if ( cls.realtime - em.expect[i].time > EM_EXPECT_MS ) {
			em.numExpect--;
			memmove( &em.expect[i], &em.expect[i + 1], ( em.numExpect - i ) * sizeof( em.expect[0] ) );
			continue;
		}
		i++;
	}

	for ( int i = 0; i < em.numActive; i++ ) {
		emActive_t *a = &em.active[i];
		int best, bestPos = -1, bestAny = -1;
		float bestDist = EM_MATCH_DIST, d;

		if ( !a->fresh )
			continue;
		a->fresh = qfalse;
		for ( int j = 0; j < em.numExpect; j++ ) {
			const emExpect_t *x = &em.expect[j];

			if ( !EM_SamePath( x->path, a->path ) )
				continue;
			if ( x->anyPos ) {
				// the oldest sent first, the list goes in the order they came
				if ( bestAny < 0 )
					bestAny = j;
				continue;
			}
			d = Distance( x->pos, a->origin );
			if ( d <= bestDist ) {
				bestPos = j;
				bestDist = d;
			}
		}
		// one sent to this very spot is surer than one sent anywhere
		best = bestPos >= 0 ? bestPos : bestAny;
		if ( best < 0 )
			continue;
		Q_strncpyz( a->tag, em.expect[best].tag, sizeof( a->tag ) );
		a->placed = qtrue;
		a->bolted = em.expect[best].bolted;
		// it was sent turned this way
		a->haveAngles = a->srvHaveAngles = em.expect[best].haveAngles;
		VectorCopy( em.expect[best].angles, a->angles );
		VectorCopy( em.expect[best].angles, a->srvAngles );
		// one moved stays selected under its new number
		a->selected = em.expect[best].selected;
		em.numExpect--;
		memmove( &em.expect[best], &em.expect[best + 1], ( em.numExpect - best ) * sizeof( em.expect[0] ) );
	}
}

// every server print goes through here on its way to the cgame; qtrue keeps it out of the console
qboolean CL_EffectManager_ServerPrint( const char *text ) {
	static emActive_t old[EM_MAX_ACTIVE];
	static int numOld;
	char buf[MAX_STRING_CHARS], *line, *next;
	qboolean swallow = qtrue, any = qfalse;

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

		if ( !Q_stricmpn( line, EM_LIST_HEADER, strlen( EM_LIST_HEADER ) ) ) {
			// also read a list someone typed /rpeffect list for, just don't hide it
			em.listOurs = (qboolean)( em.listRequestTime && cls.realtime - em.listRequestTime < EM_LIST_TIMEOUT );
			em.listRequestTime = 0;
			EM_CheckMap();
			em.listLineTime = em.listTime = cls.realtime;
			// the checks stay on the effects that are still there
			memcpy( old, em.active, em.numActive * sizeof( em.active[0] ) );
			numOld = em.numActive;
			em.numActive = 0;
			if ( !em.listOurs )
				swallow = qfalse;
		} else if ( em.listLineTime && cls.realtime - em.listLineTime < EM_LIST_LINE_GAP && EM_ParseActiveLine( line, old, numOld ) ) {
			em.listLineTime = cls.realtime;
			if ( !em.listOurs )
				swallow = qfalse;
		} else {
			swallow = qfalse;
			if ( em.state != EM_OFF && cls.realtime - em.sentTime < EM_REPLY_MS ) {
				Q_strncpyz( em.reply, line, sizeof( em.reply ) );
				em.replyTime = cls.realtime;
			}
		}
	}

	if ( em.state != EM_OFF )
		EM_RebuildActiveView();
	return (qboolean)( any && swallow );
}

static void EM_RemoveChecked( void ) {
	int count = 0;

	for ( int i = 0; i < em.numActive; i++ ) {
		if ( em.active[i].checked ) {
			EM_Queue( va( "rpeffect remove %i", em.active[i].num ), qtrue );
			count++;
		}
	}
	if ( count )
		EM_RequestList();
}

/*
===============================================================================

NAMES AND STATES

A name goes with the looping effects sent while it is in the form, and sticks to
them once the next list shows them. RPMod doesn't keep it, so a state remembers it:
the effects sent with the manager and the named ones, where the server has them.
Once rpdump load has brought them back, reloading the state finds each one by its
effect and position in a fresh list, and gives it its name again.

===============================================================================
*/

static qboolean EM_NameChar( int ch ) {
	return (qboolean)( ( ch >= 'a' && ch <= 'z' ) || ( ch >= 'A' && ch <= 'Z' ) || ( ch >= '0' && ch <= '9' ) || ch == '_' || ch == '-' );
}

static qboolean EM_CheckName( const char *name, const char *what ) {
	if ( !name[0] ) {
		EM_Message( va( S_COLOR_YELLOW "Give the %s a name", what ) );
		return qfalse;
	}
	for ( const char *c = name; *c; c++ ) {
		if ( !EM_NameChar( *c ) ) {
			EM_Message( S_COLOR_YELLOW "Use only letters, numbers, - and _ in the name" );
			return qfalse;
		}
	}
	return qtrue;
}

// both said, for the console commands that work with the manager closed
static void EM_Report( const char *text ) {
	EM_Message( text );
	Com_Printf( "Effect manager: %s\n", text );
}

static const char *EM_MapName( void ) {
	return Info_ValueForKey( cl.gameState.stringData + cl.gameState.stringOffsets[CS_SERVERINFO], "mapname" );
}

static int EM_CountTagged( const char *tag ) {
	int count = 0;

	for ( int i = 0; i < em.numActive; i++ ) {
		if ( !Q_stricmp( em.active[i].tag, tag ) )
			count++;
	}
	return count;
}

// the names in the active list, once each
static void EM_CollectNames( void ) {
	em.numNames = 0;
	for ( int i = 0; i < em.numActive && em.numNames < EM_MAX_NAMES; i++ ) {
		const char *tag = em.active[i].tag;
		qboolean seen = qfalse;

		if ( !tag[0] )
			continue;
		for ( int j = 0; j < em.numNames && !seen; j++ )
			seen = (qboolean)!Q_stricmp( em.names[j], tag );
		if ( !seen )
			Q_strncpyz( em.names[em.numNames++], tag, EM_NAME_LEN );
	}
}

static void EM_RemoveNamed( const char *name ) {
	int count = 0;

	for ( int i = 0; i < em.numActive; i++ ) {
		if ( Q_stricmp( em.active[i].tag, name ) )
			continue;
		EM_Queue( va( "rpeffect remove %i", em.active[i].num ), qtrue );
		count++;
	}
	if ( !count ) {
		EM_Report( va( S_COLOR_YELLOW "No effects are named %s; effectmanager names lists the names", name ) );
		return;
	}
	EM_RequestList();
	EM_Report( va( "Removing %i effects named %s", count, name ) );
}

static void EM_PrintNames( void ) {
	EM_CollectNames();
	for ( int i = 0; i < em.numNames; i++ )
		Com_Printf( "%s: %i effects\n", em.names[i], EM_CountTagged( em.names[i] ) );
	if ( !em.numNames )
		Com_Printf( "Effect manager: no named effects; a name in the effects form goes with the looping effects sent, and reloading a state brings names back\n" );
}

// what a state remembers: the effects sent with the manager, and the named ones
static qboolean EM_InState( const emActive_t *a ) {
	return (qboolean)( a->placed || a->tag[0] );
}

static int EM_StateCount( void ) {
	int count = 0;

	for ( int i = 0; i < em.numActive; i++ ) {
		if ( EM_InState( &em.active[i] ) )
			count++;
	}
	return count;
}

static void EM_WriteLine( fileHandle_t f, const char *text ) {
	FS_Write( text, strlen( text ), f );
}

static qboolean EM_SaveState( const char *name ) {
	char path[MAX_QPATH], effect[MAX_QPATH];
	fileHandle_t f;
	int count = EM_StateCount(), named = 0;

	if ( !EM_CheckName( name, "state" ) )
		return qfalse;
	if ( !count ) {
		EM_Message( S_COLOR_YELLOW "Nothing to save: no looping effects were sent or named yet" );
		return qfalse;
	}
	Com_sprintf( path, sizeof( path ), "%s/%s%s", EM_STATE_DIR, name, EM_SAVE_EXT );
	f = FS_FOpenFileWrite( path );
	if ( !f ) {
		EM_Message( va( S_COLOR_RED "Couldn't write %s", path ) );
		return qfalse;
	}
	EM_WriteLine( f, va( "// Effect manager state: %i effects on %s\n", count, EM_MapName() ) );
	EM_WriteLine( f, "// Once the effects are back on the map, after rpdump load, reload it from the effect manager's\n" );
	EM_WriteLine( f, "// Active tab, or exec this file, and they get their names back\n" );
	EM_WriteLine( f, "// stated <name, - for none> <effect> <x y z> <bolted>\n" );
	EM_WriteLine( f, va( "effectmanager reloadstate %s\n", name ) );
	for ( int i = 0; i < em.numActive; i++ ) {
		const emActive_t *a = &em.active[i];

		if ( !EM_InState( a ) )
			continue;
		if ( a->tag[0] )
			named++;
		EM_NormPath( a->path, effect, sizeof( effect ) );
		EM_WriteLine( f, va( "effectmanager stated %s %s %i %i %i %i\n", a->tag[0] ? a->tag : "-", effect,
			(int)floorf( a->origin[0] + 0.5f ), (int)floorf( a->origin[1] + 0.5f ), (int)floorf( a->origin[2] + 0.5f ), a->bolted ? 1 : 0 ) );
	}
	FS_FCloseFile( f );
	EM_Report( va( S_COLOR_GREEN "Saved the state of %i effects, %i of them named, as %s", count, named, path ) );
	return qtrue;
}

// splits a line into words; the line is cut up in place
static int EM_SplitLine( char *line, char **argv, int max ) {
	int argc = 0;

	while ( *line && argc < max ) {
		while ( *line == ' ' || *line == '\t' )
			line++;
		if ( !*line )
			break;
		argv[argc++] = line;
		while ( *line && *line != ' ' && *line != '\t' )
			line++;
		if ( *line )
			*line++ = '\0';
	}
	return argc;
}

// the effects of a saved state; -1 if there is no such state
static int EM_ReadState( const char *name, emStateEffect_t *out, int max ) {
	char *buf, *line, *next, *argv[16];
	int count = 0, argc;

	if ( FS_ReadFile( va( "%s/%s%s", EM_STATE_DIR, name, EM_SAVE_EXT ), (void **)&buf ) < 0 || !buf )
		return -1;
	for ( line = buf; line && count < max; line = next ) {
		next = strchr( line, '\n' );
		if ( next )
			*next++ = '\0';
		for ( char *c = line; *c; c++ ) {
			if ( *c == '\r' )
				*c = '\0';
		}
		// effectmanager stated <name> <effect> <x y z> <bolted>
		argc = EM_SplitLine( line, argv, ARRAY_LEN( argv ) );
		if ( argc < 8 || Q_stricmp( argv[0], EM_TOGGLE_CMD ) || Q_stricmp( argv[1], "stated" ) )
			continue;
		Q_strncpyz( out[count].tag, strcmp( argv[2], "-" ) ? argv[2] : "", sizeof( out[0].tag ) );
		Q_strncpyz( out[count].path, argv[3], sizeof( out[0].path ) );
		VectorSet( out[count].origin, atof( argv[4] ), atof( argv[5] ), atof( argv[6] ) );
		out[count].bolted = (qboolean)( atoi( argv[7] ) != 0 );
		count++;
	}
	FS_FreeFile( buf );
	return count;
}

static int QDECL EM_CompareStateFiles( const void *a, const void *b ) {
	return Q_stricmp( (const char *)a, (const char *)b );
}

static void EM_ListStates( void ) {
	char **list;
	int count;

	em.numStateFiles = 0;
	list = FS_ListFiles( EM_STATE_DIR, EM_SAVE_EXT, &count );
	for ( int i = 0; i < count && em.numStateFiles < EM_MAX_NAMES; i++ ) {
		char *name = em.stateFiles[em.numStateFiles++];

		Q_strncpyz( name, list[i], MAX_QPATH );
		if ( strlen( name ) > strlen( EM_SAVE_EXT ) )
			name[strlen( name ) - strlen( EM_SAVE_EXT )] = '\0';
	}
	FS_FreeFileList( list );
	qsort( em.stateFiles, em.numStateFiles, sizeof( em.stateFiles[0] ), EM_CompareStateFiles );
}

// the list asked for is in: each effect of the state, the closest of its kind where it was
static void EM_MatchState( void ) {
	static qboolean taken[EM_MAX_ACTIVE];
	int matched = 0, named = 0;

	em.reloading = qfalse;
	memset( taken, 0, sizeof( taken ) );
	for ( int i = 0; i < em.numStateEffects; i++ ) {
		const emStateEffect_t *s = &em.stateEffects[i];
		int best = -1;
		float bestDist = EM_STATE_MATCH_DIST, d;

		for ( int j = 0; j < em.numActive; j++ ) {
			const emActive_t *a = &em.active[j];

			if ( taken[j] || !EM_SamePath( a->path, s->path ) )
				continue;
			// a bolted one moves with what it's on; the first of its kind will do
			if ( s->bolted ) {
				best = j;
				break;
			}
			d = Distance( a->origin, s->origin );
			if ( d <= bestDist ) {
				best = j;
				bestDist = d;
			}
		}
		if ( best < 0 )
			continue;
		taken[best] = qtrue;
		em.active[best].placed = qtrue;
		em.active[best].bolted = s->bolted;
		if ( s->tag[0] ) {
			Q_strncpyz( em.active[best].tag, s->tag, sizeof( em.active[best].tag ) );
			named++;
		}
		matched++;
	}

	if ( em.state != EM_OFF )
		EM_RebuildActiveView();
	if ( matched == em.numStateEffects )
		EM_Report( va( S_COLOR_GREEN "Reloaded state %s: found all %i effects, %i of them named", em.reloadName, matched, named ) );
	else
		EM_Report( va( S_COLOR_YELLOW "Reloaded state %s: found %i of %i effects, %i of them named; the rest aren't on the map, or not where the state had them",
			em.reloadName, matched, em.numStateEffects, named ) );
}

static void EM_ReloadState( const char *name ) {
	int count;

	if ( cls.state != CA_ACTIVE ) {
		Com_Printf( "Effect manager: join a server first\n" );
		return;
	}
	count = EM_ReadState( name, em.stateEffects, EM_MAX_STATE );
	if ( count <= 0 ) {
		EM_Report( va( S_COLOR_YELLOW "%s %s/%s%s", count < 0 ? "No state" : "No effects in", EM_STATE_DIR, name, EM_SAVE_EXT ) );
		return;
	}
	em.numStateEffects = count;
	Q_strncpyz( em.reloadName, name, sizeof( em.reloadName ) );
	em.reloading = qtrue;
	em.reloadTime = cls.realtime;
	EM_RequestList();
	EM_Report( va( "Reloading state %s: reading the map's effects from the server", name ) );
}

/*
===============================================================================

SELECTING AND MOVING

In the free camera, V selects instead of putting the form's effect down: the effects
waiting at a position, and the map's own. RPMod can't move an effect, so one of the
map's that was moved is removed and added again where it went, as the model manager
does it, and the next list gives it its name back under its new number. The list
has no angles: one this manager sent keeps the ones it was sent with, any other goes
back straight up unless it is turned.

===============================================================================
*/

#define EM_HANDLE_ACTIVE	EM_MAX_WAITING	// handles from here on are the map's effects
#define EM_HOVER_PIXELS		18.0f			// how near the crosshair an effect is to be under it

// what the selection code moves, for one waiting at a position or one on the map
typedef struct emHandle_s {
	float		*pos, *angles;
	qboolean	*haveAngles, *selected;
	float		*carryFrom, *carryOffset, *carryAngles;
	qboolean	*carryHaveAngles;
	const char	*path;
	emActive_t	*active;	// NULL for one waiting
} emHandle_t;

static int EM_FirstHandle( void ) {
	return em.numWaiting ? 0 : EM_HANDLE_ACTIVE;
}

static int EM_NextHandle( int h ) {
	return h + 1 == em.numWaiting ? EM_HANDLE_ACTIVE : h + 1;
}

static int EM_EndHandle( void ) {
	return EM_HANDLE_ACTIVE + em.numActive;
}

// the effect behind handle h, if it can be selected: bolted ones go with what they're on
static qboolean EM_GetHandle( int h, emHandle_t *out ) {
	if ( h >= 0 && h < em.numWaiting ) {
		emItem_t *it = &em.waiting[h];

		if ( it->bolted || !it->havePos )
			return qfalse;
		out->pos = it->pos;
		out->angles = it->angles;
		out->haveAngles = &it->haveAngles;
		out->selected = &it->selected;
		out->carryFrom = it->carryFrom;
		out->carryOffset = it->carryOffset;
		out->carryAngles = it->carryAngles;
		out->carryHaveAngles = &it->carryHaveAngles;
		out->path = it->path;
		out->active = NULL;
		return qtrue;
	}
	h -= EM_HANDLE_ACTIVE;
	if ( h >= 0 && h < em.numActive ) {
		emActive_t *a = &em.active[h];

		if ( a->bolted || a->sent )
			return qfalse;
		out->pos = a->pos;
		out->angles = a->angles;
		out->haveAngles = &a->haveAngles;
		out->selected = &a->selected;
		out->carryFrom = a->carryFrom;
		out->carryOffset = a->carryOffset;
		out->carryAngles = a->carryAngles;
		out->carryHaveAngles = &a->carryHaveAngles;
		out->path = a->path;
		out->active = a;
		return qtrue;
	}
	return qfalse;
}

// one of the map's that isn't where, or turned the way, the server has it
static qboolean EM_ActiveEdited( const emActive_t *a ) {
	if ( Distance( a->pos, a->origin ) > 0.5f || a->haveAngles != a->srvHaveAngles )
		return qtrue;
	for ( int i = 0; i < 3 && a->haveAngles; i++ ) {
		if ( EM_NormalizeAngle( a->angles[i] ) != EM_NormalizeAngle( a->srvAngles[i] ) )
			return qtrue;
	}
	return qfalse;
}

static int EM_NumSelected( void ) {
	emHandle_t hd;
	int count = 0;

	for ( int h = EM_FirstHandle(); h < EM_EndHandle(); h = EM_NextHandle( h ) ) {
		if ( EM_GetHandle( h, &hd ) && *hd.selected )
			count++;
	}
	return count;
}

static void EM_ClearSelection( void ) {
	for ( int i = 0; i < em.numWaiting; i++ )
		em.waiting[i].selected = qfalse;
	for ( int i = 0; i < em.numActive; i++ )
		em.active[i].selected = qfalse;
}

// the one nearest the crosshair on screen, or the mouse with the arrows up; -1 if none is near enough
static int EM_HoverAtCrosshair( void ) {
	float best = EM_HOVER_PIXELS * EM_HOVER_PIXELS, x, y, d;
	const float ax = em.gizmo ? em.cursorX : SCREEN_WIDTH * 0.5f, ay = em.gizmo ? em.cursorY : SCREEN_HEIGHT * 0.5f;
	emHandle_t hd;
	int found = -1;

	for ( int h = EM_FirstHandle(); h < EM_EndHandle(); h = EM_NextHandle( h ) ) {
		if ( !EM_GetHandle( h, &hd ) || !EM_Project( hd.pos, &x, &y ) )
			continue;
		d = Square( x - ax ) + Square( y - ay );
		if ( d < best ) {
			best = d;
			found = h;
		}
	}
	return found;
}

// the middle of the selection, which it turns about
static void EM_Pivot( vec3_t pivot ) {
	vec3_t mins, maxs;
	emHandle_t hd;

	ClearBounds( mins, maxs );
	for ( int h = EM_FirstHandle(); h < EM_EndHandle(); h = EM_NextHandle( h ) ) {
		if ( EM_GetHandle( h, &hd ) && *hd.selected )
			AddPointToBounds( hd.pos, mins, maxs );
	}
	if ( mins[0] > maxs[0] ) {
		VectorClear( pivot );
		return;
	}
	VectorAdd( mins, maxs, pivot );
	VectorScale( pivot, 0.5f, pivot );
}

static void EM_ClickSelect( void ) {
	emHandle_t hd;

	const qboolean shift = (qboolean)( em.held[A_SHIFT] || em.held[A_SHIFT2] );

	if ( !EM_GetHandle( em.hover, &hd ) ) {
		if ( !shift )
			EM_ClearSelection();
		return;
	}
	if ( shift ) {
		*hd.selected = (qboolean)!*hd.selected;
	} else {
		EM_ClearSelection();
		*hd.selected = qtrue;
	}
}

// the selection in hand, to follow the crosshair; where each was stays, to put back
static qboolean EM_PickUp( void ) {
	vec3_t pivot;
	emHandle_t hd;

	if ( em.carrying )
		return qtrue;
	if ( !EM_NumSelected() ) {
		EM_Message( S_COLOR_YELLOW "Nothing selected: aim at an effect and click it" );
		return qfalse;
	}
	EM_Pivot( pivot );
	for ( int h = EM_FirstHandle(); h < EM_EndHandle(); h = EM_NextHandle( h ) ) {
		if ( !EM_GetHandle( h, &hd ) || !*hd.selected )
			continue;
		VectorCopy( hd.pos, hd.carryFrom );
		VectorSubtract( hd.pos, pivot, hd.carryOffset );
		VectorCopy( hd.angles, hd.carryAngles );
		*hd.carryHaveAngles = *hd.haveAngles;
	}
	em.carrying = qtrue;
	em.carryLocked = qfalse;
	return qtrue;
}

// back where they were picked up
static void EM_CancelCarry( void ) {
	emHandle_t hd;

	if ( !em.carrying )
		return;
	em.carrying = qfalse;
	for ( int h = EM_FirstHandle(); h < EM_EndHandle(); h = EM_NextHandle( h ) ) {
		if ( !EM_GetHandle( h, &hd ) || !*hd.selected )
			continue;
		VectorCopy( hd.carryFrom, hd.pos );
		VectorCopy( hd.carryAngles, hd.angles );
		*hd.haveAngles = *hd.carryHaveAngles;
	}
}

// where the crosshair meets the world, just off that surface
static void EM_CrosshairPoint( vec3_t out ) {
	vec3_t forward, end;
	trace_t tr;
	int grid = emSnaps[em.snap].grid;

	AngleVectors( em.camAng, forward, NULL, NULL );
	VectorMA( em.camOrg, EM_TRACE_DIST, forward, end );
	CM_BoxTrace( &tr, em.camOrg, end, vec3_origin, vec3_origin, 0, EM_TRACE_MASK, qfalse );
	if ( tr.fraction < 1.0f && !tr.startsolid && !( tr.surfaceFlags & SURF_SKY ) ) {
		VectorMA( tr.endpos, 1.0f, tr.plane.normal, out );
		for ( int i = 0; i < 3; i++ ) {
			if ( fabsf( tr.plane.normal[i] ) < 0.7f )
				out[i] = EM_SnapTo( out[i], grid );
		}
	} else {
		VectorMA( em.camOrg, EM_NO_HIT_DIST, forward, out );
		for ( int i = 0; i < 3; i++ )
			out[i] = EM_SnapTo( out[i], grid );
	}
}

static void EM_FollowSelection( void ) {
	vec3_t point;
	emHandle_t hd;

	EM_CrosshairPoint( point );
	for ( int h = EM_FirstHandle(); h < EM_EndHandle(); h = EM_NextHandle( h ) ) {
		if ( EM_GetHandle( h, &hd ) && *hd.selected )
			VectorAdd( point, hd.carryOffset, hd.pos );
	}
}

// the changes to the map's effects, removes first while the numbers still name the
// effects they were read for; the selected ones, or all of them
static int EM_SendActiveChanges( qboolean selectedOnly ) {
	int count = 0;

	for ( int i = 0; i < em.numActive; i++ ) {
		const emActive_t *a = &em.active[i];

		if ( a->sent || a->bolted || ( selectedOnly && !a->selected ) )
			continue;
		if ( a->deleted || EM_ActiveEdited( a ) )
			EM_Queue( va( "rpeffect remove %i", a->num ), qtrue );
	}
	for ( int i = 0; i < em.numActive; i++ ) {
		emActive_t *a = &em.active[i];
		emItem_t it;

		if ( a->sent || a->bolted || ( selectedOnly && !a->selected ) )
			continue;
		if ( a->deleted ) {
			a->sent = qtrue;
			count++;
			continue;
		}
		if ( !EM_ActiveEdited( a ) )
			continue;
		// back where it went, looping as all the map's are, with its name
		memset( &it, 0, sizeof( it ) );
		EM_NormPath( a->path, it.path, sizeof( it.path ) );
		it.loop = qtrue;
		it.havePos = qtrue;
		VectorCopy( a->pos, it.pos );
		it.haveAngles = a->haveAngles;
		VectorCopy( a->angles, it.angles );
		Q_strncpyz( it.tag, a->tag, sizeof( it.tag ) );
		it.selected = a->selected;
		it.scale = EM_ScaleAt( a->path, a->origin );
		EM_Expect( &it );
		EM_Queue( EM_ItemCommand( &it ), qtrue );
		if ( fabsf( it.scale - 1.0f ) > 0.001f ) {
			EM_SetScale( a->path, a->origin, 1.0f, qfalse );
			EM_SentScale( &it );
		}
		a->sent = qtrue;
		count++;
	}
	if ( count )
		EM_RequestList();
	return count;
}

// the map's effects moved, removed or still to send, and those waiting
static void EM_PendingChanges( int *moved, int *removed ) {
	*moved = *removed = 0;
	for ( int i = 0; i < em.numActive; i++ ) {
		const emActive_t *a = &em.active[i];

		if ( a->sent || a->bolted )
			continue;
		if ( a->deleted )
			( *removed )++;
		else if ( EM_ActiveEdited( a ) )
			( *moved )++;
	}
}

static void EM_SendAllChanges( void ) {
	const int changed = EM_SendActiveChanges( qfalse ), waiting = em.numWaiting;

	EM_SendWaiting();
	if ( changed + waiting )
		EM_Message( va( S_COLOR_GREEN "Sending %i changes and %i new effects, about one a second", changed, waiting ) );
	else
		EM_Message( "Nothing to send" );
}

static void EM_DropSelection( void ) {
	if ( !em.carrying )
		return;
	em.carrying = qfalse;
	if ( em.instant )
		EM_SendActiveChanges( qtrue );
}

static void EM_RotateSelection( int axis, int sign ) {
	const int step = emSnaps[em.snap].angle;
	const float a = DEG2RAD( (float)( sign * step ) );
	const qboolean group = (qboolean)( EM_NumSelected() > 1 );
	vec3_t pivot, d;
	emHandle_t hd;

	if ( !EM_NumSelected() ) {
		EM_Message( S_COLOR_YELLOW "Nothing selected: aim at an effect and click it" );
		return;
	}
	EM_Pivot( pivot );
	for ( int h = EM_FirstHandle(); h < EM_EndHandle(); h = EM_NextHandle( h ) ) {
		if ( !EM_GetHandle( h, &hd ) || !*hd.selected )
			continue;
		if ( !*hd.haveAngles ) {
			VectorClear( hd.angles );
			*hd.haveAngles = qtrue;
		}
		hd.angles[axis] = (float)EM_NormalizeAngle( EM_SnapTo( hd.angles[axis] + sign * step, step ) );
		// a group turns about its middle
		if ( group && axis == YAW ) {
			VectorSubtract( hd.pos, pivot, d );
			hd.pos[0] = pivot[0] + d[0] * cosf( a ) - d[1] * sinf( a );
			hd.pos[1] = pivot[1] + d[0] * sinf( a ) + d[1] * cosf( a );
			if ( em.carrying )
				VectorSubtract( hd.pos, pivot, hd.carryOffset );
		}
	}
}

static void EM_StraightenSelection( void ) {
	emHandle_t hd;

	for ( int h = EM_FirstHandle(); h < EM_EndHandle(); h = EM_NextHandle( h ) ) {
		if ( EM_GetHandle( h, &hd ) && *hd.selected ) {
			VectorClear( hd.angles );
			*hd.haveAngles = qfalse;
		}
	}
}

// along the world axis closest to the camera direction, so nudges stay on the grid
static void EM_NudgeSelection( int dx, int dy, int dz ) {
	const float yaw = DEG2RAD( floorf( em.camAng[YAW] / 90.0f + 0.5f ) * 90.0f );
	const int grid = emSnaps[em.snap].grid;
	vec3_t forward, right, delta;
	emHandle_t hd;

	if ( !EM_NumSelected() ) {
		EM_Message( S_COLOR_YELLOW "Nothing selected: aim at an effect and click it" );
		return;
	}
	if ( em.carrying )
		em.carryLocked = qtrue;
	VectorSet( forward, cosf( yaw ), sinf( yaw ), 0 );
	VectorSet( right, sinf( yaw ), -cosf( yaw ), 0 );
	VectorScale( forward, (float)( dx * grid ), delta );
	VectorMA( delta, (float)( dy * grid ), right, delta );
	delta[0] = floorf( delta[0] + 0.5f );
	delta[1] = floorf( delta[1] + 0.5f );
	delta[2] = (float)( dz * grid );
	for ( int h = EM_FirstHandle(); h < EM_EndHandle(); h = EM_NextHandle( h ) ) {
		if ( EM_GetHandle( h, &hd ) && *hd.selected )
			VectorAdd( hd.pos, delta, hd.pos );
	}
}

// waiting ones just go, the map's are removed
static void EM_DeleteSelection( void ) {
	int keep = 0, removed = 0;

	EM_CancelCarry();
	for ( int i = 0; i < em.numWaiting; i++ ) {
		const emItem_t *it = &em.waiting[i];

		if ( it->selected && !it->bolted && it->havePos )
			continue;
		em.waiting[keep++] = em.waiting[i];
	}
	em.numWaiting = keep;
	for ( int i = 0; i < em.numActive; i++ ) {
		emActive_t *a = &em.active[i];

		if ( !a->selected || a->bolted || a->sent )
			continue;
		a->deleted = qtrue;
		removed++;
	}
	em.hover = -1;
	if ( removed && em.instant )
		EM_SendActiveChanges( qtrue );
}

// the map's effects back as the server has them: the selected ones, or all
static void EM_UndoChanges( void ) {
	const qboolean all = (qboolean)!EM_NumSelected();

	EM_CancelCarry();
	for ( int i = 0; i < em.numActive; i++ ) {
		emActive_t *a = &em.active[i];

		if ( a->sent || ( !all && !a->selected ) )
			continue;
		VectorCopy( a->origin, a->pos );
		VectorCopy( a->srvAngles, a->angles );
		a->haveAngles = a->srvHaveAngles;
		a->deleted = qfalse;
	}
}

/*
===============================================================================

CONSTRUCTS

Effects saved together: the free camera's selection, or what waits. Placed ones are
kept from their middle, to put down anywhere and turn; bolted ones keep their tag or
bone, and go on whoever is picked when the construct is used.

===============================================================================
*/

static int QDECL EM_CompareNames( const void *a, const void *b ) {
	return Q_stricmp( (const char *)a, (const char *)b );
}

static qboolean EM_ParseCPiece( char **argv, int argc, emCPiece_t *out ) {
	// effectmanager cpiece <effect> <loop> pos <x y z> <angles seen> <pitch yaw roll>
	// effectmanager cpiece <effect> <loop> bolt <tag or bone>
	if ( argc < 6 || Q_stricmp( argv[0], EM_TOGGLE_CMD ) || Q_stricmp( argv[1], "cpiece" ) )
		return qfalse;
	memset( out, 0, sizeof( *out ) );
	Q_strncpyz( out->path, argv[2], sizeof( out->path ) );
	out->loop = (qboolean)( atoi( argv[3] ) != 0 );
	if ( !Q_stricmp( argv[4], "bolt" ) ) {
		out->bolted = qtrue;
		Q_strncpyz( out->bolt, argv[5], sizeof( out->bolt ) );
		return qtrue;
	}
	if ( Q_stricmp( argv[4], "pos" ) || argc < 12 )
		return qfalse;
	VectorSet( out->offset, atof( argv[5] ), atof( argv[6] ), atof( argv[7] ) );
	out->haveAngles = (qboolean)( atoi( argv[8] ) != 0 );
	VectorSet( out->angles, atof( argv[9] ), atof( argv[10] ), atof( argv[11] ) );
	return qtrue;
}

// the effects of a saved construct; -1 if there is no such construct
static int EM_ReadConstruct( const char *name, emCPiece_t *out, int max ) {
	char *buf, *line, *next, *argv[16];
	int count = 0;

	if ( FS_ReadFile( va( "%s/%s%s", EM_CONSTRUCT_DIR, name, EM_SAVE_EXT ), (void **)&buf ) < 0 || !buf )
		return -1;
	for ( line = buf; line && count < max; line = next ) {
		next = strchr( line, '\n' );
		if ( next )
			*next++ = '\0';
		for ( char *c = line; *c; c++ ) {
			if ( *c == '\r' )
				*c = '\0';
		}
		if ( EM_ParseCPiece( argv, EM_SplitLine( line, argv, ARRAY_LEN( argv ) ), &out[count] ) )
			count++;
	}
	FS_FreeFile( buf );
	return count;
}

static void EM_ListConstructs( void ) {
	static emCPiece_t scratch[EM_MAX_CPIECES];
	char **list;
	int count;

	em.numConstructs = 0;
	list = FS_ListFiles( EM_CONSTRUCT_DIR, EM_SAVE_EXT, &count );
	for ( int i = 0; i < count && em.numConstructs < EM_MAX_NAMES; i++ ) {
		char *name = em.constructs[em.numConstructs++];

		Q_strncpyz( name, list[i], MAX_QPATH );
		if ( strlen( name ) > strlen( EM_SAVE_EXT ) )
			name[strlen( name ) - strlen( EM_SAVE_EXT )] = '\0';
	}
	FS_FreeFileList( list );
	qsort( em.constructs, em.numConstructs, sizeof( em.constructs[0] ), EM_CompareNames );
	for ( int i = 0; i < em.numConstructs; i++ )
		em.constructCounts[i] = Q_max( 0, EM_ReadConstruct( em.constructs[i], scratch, EM_MAX_CPIECES ) );
	em.numCPieces = 0;
	if ( em.numRows )
		em.rows[0].count = em.numConstructs;
}

static void EM_CountCPieces( const emCPiece_t *pieces, int count, int *placed, int *bolted ) {
	*placed = *bolted = 0;
	for ( int i = 0; i < count; i++ ) {
		if ( pieces[i].bolted )
			( *bolted )++;
		else
			( *placed )++;
	}
}

// what saving a construct would take: the free camera's selection, or what waits
static int EM_ConstructSource( emCPiece_t *out, int max ) {
	vec3_t mins, maxs, anchor;
	emHandle_t hd;
	int count = 0;

	if ( em.constructFromSelection ) {
		for ( int h = EM_FirstHandle(); h < EM_EndHandle() && count < max; h = EM_NextHandle( h ) ) {
			emCPiece_t *p = &out[count];

			if ( !EM_GetHandle( h, &hd ) || !*hd.selected )
				continue;
			memset( p, 0, sizeof( *p ) );
			EM_NormPath( hd.path, p->path, sizeof( p->path ) );
			p->loop = hd.active ? qtrue : em.waiting[h].loop;	// the map's all loop
			VectorCopy( hd.pos, p->offset );
			p->haveAngles = *hd.haveAngles;
			VectorCopy( hd.angles, p->angles );
			count++;
		}
	} else {
		for ( int i = 0; i < em.numWaiting && count < max; i++ ) {
			const emItem_t *it = &em.waiting[i];
			emCPiece_t *p = &out[count];

			// one at the player has no place of its own
			if ( !it->bolted && !it->havePos )
				continue;
			memset( p, 0, sizeof( *p ) );
			Q_strncpyz( p->path, it->path, sizeof( p->path ) );
			p->loop = it->loop;
			p->bolted = it->bolted;
			Q_strncpyz( p->bolt, it->bolt, sizeof( p->bolt ) );
			VectorCopy( it->pos, p->offset );
			p->haveAngles = it->haveAngles;
			VectorCopy( it->angles, p->angles );
			count++;
		}
	}

	// placed ones from the middle of their floor
	ClearBounds( mins, maxs );
	for ( int i = 0; i < count; i++ ) {
		if ( !out[i].bolted )
			AddPointToBounds( out[i].offset, mins, maxs );
	}
	if ( mins[0] <= maxs[0] ) {
		VectorSet( anchor, floorf( ( mins[0] + maxs[0] ) * 0.5f + 0.5f ), floorf( ( mins[1] + maxs[1] ) * 0.5f + 0.5f ), floorf( mins[2] + 0.5f ) );
		for ( int i = 0; i < count; i++ ) {
			if ( !out[i].bolted )
				VectorSubtract( out[i].offset, anchor, out[i].offset );
		}
	}
	return count;
}

static qboolean EM_SaveConstruct( const char *name ) {
	static emCPiece_t pieces[EM_MAX_CPIECES];
	char path[MAX_QPATH];
	fileHandle_t f;
	int count, placed, bolted;

	if ( !EM_CheckName( name, "construct" ) )
		return qfalse;
	count = EM_ConstructSource( pieces, EM_MAX_CPIECES );
	if ( !count ) {
		EM_Message( S_COLOR_YELLOW "Nothing to save: select effects, or add some to waiting" );
		return qfalse;
	}
	Com_sprintf( path, sizeof( path ), "%s/%s%s", EM_CONSTRUCT_DIR, name, EM_SAVE_EXT );
	f = FS_FOpenFileWrite( path );
	if ( !f ) {
		EM_Message( va( S_COLOR_RED "Couldn't write %s", path ) );
		return qfalse;
	}
	EM_CountCPieces( pieces, count, &placed, &bolted );
	EM_WriteLine( f, va( "// Effect manager construct: %i effects, %i placed and %i bolted, made on %s\n", count, placed, bolted, EM_MapName() ) );
	EM_WriteLine( f, "// Shown under Saved constructs in the effect manager; exec this file to pick it\n" );
	EM_WriteLine( f, "// cpiece <effect> <loop> pos <x y z from the middle> <angles seen> <pitch yaw roll>\n" );
	EM_WriteLine( f, "// cpiece <effect> <loop> bolt <tag or bone>, on whoever it is put on\n" );
	EM_WriteLine( f, va( "effectmanager loadconstruct %s\n", name ) );
	for ( int i = 0; i < count; i++ ) {
		const emCPiece_t *p = &pieces[i];

		if ( p->bolted ) {
			EM_WriteLine( f, va( "effectmanager cpiece %s %i bolt %s\n", p->path, p->loop ? 1 : 0, p->bolt ) );
			continue;
		}
		EM_WriteLine( f, va( "effectmanager cpiece %s %i pos %i %i %i %i %i %i %i\n", p->path, p->loop ? 1 : 0,
			(int)floorf( p->offset[0] + 0.5f ), (int)floorf( p->offset[1] + 0.5f ), (int)floorf( p->offset[2] + 0.5f ),
			p->haveAngles ? 1 : 0, EM_NormalizeAngle( p->angles[PITCH] ), EM_NormalizeAngle( p->angles[YAW] ), EM_NormalizeAngle( p->angles[ROLL] ) ) );
	}
	FS_FCloseFile( f );
	EM_ListConstructs();
	EM_Report( va( S_COLOR_GREEN "Saved %i effects as %s", count, path ) );
	return qtrue;
}

// a bolted piece on whoever the form has picked
static qboolean EM_BoltedItem( const emCPiece_t *p, emItem_t *out ) {
	memset( out, 0, sizeof( *out ) );
	if ( em.target == EM_TARGET_NPC && !em.targetName[0] )
		return qfalse;
	Q_strncpyz( out->path, p->path, sizeof( out->path ) );
	out->loop = p->loop;
	out->bolted = qtrue;
	Q_strncpyz( out->bolt, p->bolt, sizeof( out->bolt ) );
	out->target = em.target;
	out->targetClient = em.targetClient;
	out->targetNpc = em.targetNpc;
	Q_strncpyz( out->targetName, em.targetName, sizeof( out->targetName ) );
	Q_strncpyz( out->tag, em.fields[EM_F_NAME], sizeof( out->tag ) );
	return qtrue;
}

// the selected construct's bolted effects on the target, sent or waiting
static void EM_ApplyConstructBolts( qboolean toWaiting ) {
	int count = 0;

	for ( int i = 0; i < em.numCPieces; i++ ) {
		emItem_t it;

		if ( !em.cpieces[i].bolted || !EM_BoltedItem( &em.cpieces[i], &it ) )
			continue;
		if ( toWaiting ) {
			if ( em.numWaiting >= EM_MAX_WAITING )
				break;
			em.waiting[em.numWaiting++] = it;
		} else {
			EM_Expect( &it );
			EM_Queue( EM_ItemCommand( &it ), qtrue );
		}
		count++;
	}
	if ( !count )
		return;
	if ( !toWaiting )
		EM_RequestList();
	EM_Message( va( toWaiting ? S_COLOR_GREEN "%i bolted effects added to waiting, on %s" : S_COLOR_GREEN "Sending %i bolted effects, on %s",
		count, EM_TargetLabel() ) );
}

// a placed piece in hand, with the hand at anchor and turned
static void EM_HandItem( int i, const vec3_t anchor, emItem_t *out ) {
	const emCPiece_t *p = &em.hand[i];
	const float a = DEG2RAD( (float)em.handYaw );

	memset( out, 0, sizeof( *out ) );
	Q_strncpyz( out->path, p->path, sizeof( out->path ) );
	out->loop = p->loop;
	out->havePos = qtrue;
	out->pos[0] = anchor[0] + p->offset[0] * cosf( a ) - p->offset[1] * sinf( a );
	out->pos[1] = anchor[1] + p->offset[0] * sinf( a ) + p->offset[1] * cosf( a );
	out->pos[2] = anchor[2] + p->offset[2];
	// straight up stays straight up, whichever way it's turned
	out->haveAngles = p->haveAngles;
	VectorCopy( p->angles, out->angles );
	if ( p->haveAngles )
		out->angles[YAW] = (float)EM_NormalizeAngle( out->angles[YAW] + em.handYaw );
	Q_strncpyz( out->tag, em.fields[EM_F_NAME], sizeof( out->tag ) );
}

// what is in hand goes down where the crosshair is, sent or waiting; it stays in hand
static void EM_PutDownHand( qboolean send ) {
	int count = 0;

	for ( int i = 0; i < em.numHand; i++ ) {
		emItem_t it;

		EM_HandItem( i, em.origin, &it );
		if ( send ) {
			EM_Expect( &it );
			EM_Queue( EM_ItemCommand( &it ), qtrue );
		} else {
			if ( em.numWaiting >= EM_MAX_WAITING ) {
				EM_Message( S_COLOR_YELLOW "That's as many as can wait at once; send them first" );
				break;
			}
			it.nextPlay = cls.realtime;
			em.waiting[em.numWaiting++] = it;
		}
		count++;
	}
	if ( send && count )
		EM_RequestList();
}

// a list was read whole
static void EM_ListDone( void ) {
	EM_MatchExpected();
	if ( em.reloading && em.listTime >= em.reloadTime )
		EM_MatchState();
	if ( em.state != EM_OFF )
		EM_RebuildActiveView();
}

/*
===============================================================================

STATE CHANGES

===============================================================================
*/

static void EM_ReleaseKeys( void ) {
	memset( em.held, 0, sizeof( em.held ) );
}

static void EM_SetTab( emTab_t tab ) {
	em.tab = tab;
	em.focus = EM_F_NONE;
	em.picker = EM_PICKER_NONE;
	em.confirm = EM_CONFIRM_NONE;
	if ( tab == EM_TAB_ACTIVE )
		EM_RequestList();
}

static void EM_OpenPicker( emPicker_t picker ) {
	em.picker = picker;
	em.fields[EM_F_PICKER_SEARCH][0] = '\0';
	em.pickerList.sel = em.pickerList.scroll = 0;
	em.modelYaw = 0.0f;
	if ( picker == EM_PICKER_NPC || picker == EM_PICKER_REMOVE_NPC )
		CL_NpcManager_RequestList();
	if ( picker == EM_PICKER_REMOVE_NAME )
		EM_CollectNames();
	if ( picker == EM_PICKER_STATE )
		EM_ListStates();
	EM_RebuildPickerView();
	// start on the bolt already chosen
	if ( picker == EM_PICKER_BOLT ) {
		for ( int i = 0; i < em.numPickerView; i++ ) {
			if ( !Q_stricmp( em.bolts[em.pickerView[i]].name, em.fields[EM_F_BOLT] ) ) {
				EM_ListMove( &em.pickerList, em.numPickerView, (int)( ( EM_PICKER_H - 74 ) / EM_ROW_H ), i );
				break;
			}
		}
	}
}

static void EM_ChoosePicker( int index ) {
	int num;
	const char *type, *name;

	switch ( em.picker ) {
	case EM_PICKER_PLAYER:
		em.target = EM_TARGET_PLAYER;
		em.targetClient = index;
		break;
	case EM_PICKER_NPC:
		if ( !CL_NpcManager_GetNpc( index, &num, &type, &name ) || !name[0] )
			return;	// RPMod finds NPCs by targetname
		em.target = EM_TARGET_NPC;
		em.targetNpc = num;
		Q_strncpyz( em.targetName, name, sizeof( em.targetName ) );
		break;
	case EM_PICKER_BOLT:
		Q_strncpyz( em.fields[EM_F_BOLT], em.bolts[index].name, EM_FIELD_LEN );
		break;
	case EM_PICKER_REMOVE_PLAYER:
		EM_SendAndRefresh( va( "rpeffect remove bolted %i", index ) );
		break;
	case EM_PICKER_REMOVE_NPC:
		if ( !CL_NpcManager_GetNpc( index, &num, &type, &name ) || !name[0] )
			return;
		EM_SendAndRefresh( va( "rpeffect remove bolted npc %s", name ) );
		break;
	case EM_PICKER_REMOVE_NAME:
		EM_RemoveNamed( em.names[index] );
		break;
	case EM_PICKER_STATE:
		EM_ReloadState( em.stateFiles[index] );
		break;
	default:
		break;
	}
	em.picker = EM_PICKER_NONE;
}

static void EM_EnterBrowse( void ) {
	EM_CancelCarry();
	em.selecting = qfalse;
	em.handConstruct = qfalse;
	em.state = EM_BROWSE;
	em.focus = EM_F_NONE;
	EM_ReleaseKeys();
}

// the free camera starts where the player looks from
static void EM_CameraFromView( void ) {
	if ( em.haveView ) {
		VectorCopy( em.viewOrg, em.camOrg );
		vectoangles( em.viewAxis[0], em.camAng );
	} else {
		VectorCopy( cl.snap.ps.origin, em.camOrg );
		em.camOrg[2] += cl.snap.ps.viewheight;
		VectorCopy( cl.viewangles, em.camAng );
	}
	if ( em.camAng[PITCH] > 180.0f )
		em.camAng[PITCH] -= 360.0f;
	em.camAng[ROLL] = 0;
}

// the free camera, selecting the effects waiting and the map's, to move them
static void EM_EnterSelect( void ) {
	if ( em.state != EM_PLACE ) {
		em.picker = EM_PICKER_NONE;
		em.focus = EM_F_NONE;
		EM_CameraFromView();
		em.state = EM_PLACE;
		em.lastFrameTime = cls.realtime;
		EM_ReleaseKeys();
	}
	em.selecting = qtrue;
	em.handConstruct = qfalse;
	em.carrying = qfalse;
	em.hover = -1;
	em.playKey[0] = '\0';
	// what is on the map now
	EM_RequestList();
}

static void EM_EnterPlace( void ) {
	if ( !EM_Effect() )
		return;

	em.picker = EM_PICKER_NONE;
	em.focus = EM_F_NONE;
	EM_CancelCarry();
	em.selecting = qfalse;
	em.handConstruct = qfalse;
	EM_CameraFromView();

	if ( em.where == EM_WHERE_BOLT ) {
		// in front of the target, looking at it
		vec3_t org, angles, forward, dir;

		EM_LerpData( EM_TargetEntity(), org, angles );
		angles[PITCH] = angles[ROLL] = 0;
		AngleVectors( angles, forward, NULL, NULL );
		VectorMA( org, 110.0f, forward, em.camOrg );
		em.camOrg[2] += 20.0f;
		VectorSubtract( org, em.camOrg, dir );
		vectoangles( dir, em.camAng );
	} else if ( em.where == EM_WHERE_POS && em.havePos ) {
		VectorCopy( em.pos, em.origin );
		em.locked = qtrue;
	} else {
		em.locked = qfalse;
	}
	if ( em.camAng[PITCH] > 180.0f )
		em.camAng[PITCH] -= 360.0f;
	em.camAng[ROLL] = 0;
	em.state = EM_PLACE;
	em.lastFrameTime = cls.realtime;
	em.playKey[0] = '\0';
	EM_ReleaseKeys();
}

// the selected construct's placed effects in hand, in the free camera, to put down
static void EM_StartConstructPlace( void ) {
	em.numHand = 0;
	for ( int i = 0; i < em.numCPieces && em.construct >= 0; i++ ) {
		if ( em.cpieces[i].bolted )
			continue;
		em.hand[em.numHand] = em.cpieces[i];
		em.hand[em.numHand].nextPlay = cls.realtime;
		em.numHand++;
	}
	if ( !em.numHand ) {
		EM_Message( S_COLOR_YELLOW "This construct has only bolted effects: pick who they go on, then Send or Add to waiting" );
		return;
	}
	Q_strncpyz( em.handName, em.constructs[em.construct], sizeof( em.handName ) );
	em.handYaw = 0;
	em.picker = EM_PICKER_NONE;
	em.focus = EM_F_NONE;
	EM_CancelCarry();
	em.selecting = qfalse;
	em.handConstruct = qtrue;
	em.locked = qfalse;
	EM_CameraFromView();
	em.state = EM_PLACE;
	em.lastFrameTime = cls.realtime;
	em.playKey[0] = '\0';
	EM_ReleaseKeys();
}

static void EM_Open( void ) {
	if ( cls.state != CA_ACTIVE || !cls.cgameStarted ) {
		Com_Printf( "Effect manager: join a server first\n" );
		return;
	}
	// for RPMod's admins: whoever else can't run what it makes
	if ( !CL_RpAdmin_Allow( "Effect manager", "effectmanager" ) )
		return;
	CL_ModelManager_Close();
	CL_NpcManager_Close();
	CL_ShaderManager_Close();
	CL_MapEditor_Close();
	CL_Storyteller_Close();
	CL_RpCinematic_EditorClose();
	CL_SoundManager_Close();
	EM_CheckMap();
	EM_ListConstructs();
	if ( !em.indexed ) {
		EM_BuildIndex();
		EM_RebuildEffectView();
	}

	em.font = re->RegisterFont( "arialnb" );
	if ( !em.font )
		em.font = cls.menuFont;

	em.cursorX = SCREEN_WIDTH * 0.5f;
	em.cursorY = SCREEN_HEIGHT * 0.5f;
	em.click = qfalse;
	em.wheel = 0;
	em.picker = EM_PICKER_NONE;
	em.playKey[0] = '\0';
	EM_RebuildEffectView();
	EM_EnterBrowse();
	EM_SetTab( em.tab );
	Key_SetCatcher( Key_GetCatcher() | KEYCATCH_EFFECTMANAGER );
}

static void EM_Close( void ) {
	em.state = EM_OFF;
	em.picker = EM_PICKER_NONE;
	em.naming = EM_NAMING_NONE;
	em.numPending = 0;
	EM_ReleaseKeys();
	Key_SetCatcher( Key_GetCatcher() & ~KEYCATCH_EFFECTMANAGER );
}

void CL_EffectManager_Close( void ) {
	if ( em.state != EM_OFF )
		EM_Close();
}

void CL_EffectManager_Init( void ) {
	memset( &em, 0, sizeof( em ) );
	em.effect = -1;
	em.previewDist = 160.0f;
	em.snap = 2;
	em.instant = qtrue;
	em.construct = -1;
	Q_strncpyz( em.fields[EM_F_BOLT], "*r_hand", EM_FIELD_LEN );
	Q_strncpyz( em.fields[EM_F_INTENSITY], "5", EM_FIELD_LEN );
	Q_strncpyz( em.fields[EM_F_DURATION], "2", EM_FIELD_LEN );
}

// cgame is going away: map change, disconnect or vid_restart
void CL_EffectManager_Shutdown( void ) {
	CL_EffectManager_Close();
	// effects bolted to our copies die with the cgame's effects system
	EM_FreeModels();
	for ( int i = 0; i < em.numEffects; i++ ) {
		em.effects[i].id = 0;
		em.effects[i].failed = qfalse;
	}
	// the pure pk3 list can change with the next map or server
	em.indexed = qfalse;
	em.haveView = qfalse;
	// the active list stays for its names, until EM_CheckMap finds another map or server
	em.numQueue = 0;
	em.numExpect = 0;
	em.reloading = qfalse;
	em.listRequestTime = em.listLineTime = em.listTime = em.refreshTime = 0;
	em.listQueued = qfalse;
	em.havePos = qfalse;
	em.target = EM_TARGET_ME;
}

qboolean CL_EffectManager_Active( void ) {
	return (qboolean)( em.state != EM_OFF );
}

// the effects RPMod listed last, for the storyteller; the list is read with the manager closed too
void CL_EffectManager_RequestList( void ) {
	if ( cls.state == CA_ACTIVE )
		EM_RequestList();
}

int CL_EffectManager_NumActive( void ) {
	return em.numActive;
}

qboolean CL_EffectManager_GetActive( int index, int *num, const char **path, vec3_t origin ) {
	if ( index < 0 || index >= em.numActive )
		return qfalse;
	*num = em.active[index].num;
	*path = em.active[index].path;
	VectorCopy( em.active[index].origin, origin );
	return qtrue;
}

// opened from a storyteller step
// for the cinematic manager: what's put down goes in the cinematic
void CL_EffectManager_OpenForCinematic( void ) {
	EM_Open();
	if ( em.state != EM_OFF )
		EM_Message( S_COLOR_GREEN "For the cinematic: pick an effect and put it down; it goes in the cinematic, not to the server" );
}

void CL_EffectManager_OpenForStory( void ) {
	EM_Open();
	if ( em.state != EM_OFF )
		EM_Message( S_COLOR_GREEN "For the story: what you send here goes in it as steps, not to the server" );
}

/*
===============================================================================

CONSOLE COMMAND

===============================================================================
*/

void CL_EffectManager_f( void ) {
	const char *arg = Cmd_Argv( 1 );

	if ( Cmd_Argc() < 2 ) {
		if ( em.state == EM_OFF )
			EM_Open();
		else
			EM_Close();
		return;
	}

	if ( !Q_stricmp( arg, "active" ) || !Q_stricmp( arg, "presets" ) || !Q_stricmp( arg, "effects" ) ) {
		emTab_t tab = !Q_stricmp( arg, "active" ) ? EM_TAB_ACTIVE : !Q_stricmp( arg, "presets" ) ? EM_TAB_PRESETS : EM_TAB_EFFECTS;

		if ( em.state == EM_OFF ) {
			em.tab = tab;
			EM_Open();
		} else {
			EM_EnterBrowse();
			EM_SetTab( tab );
		}
	} else if ( !Q_stricmp( arg, "close" ) ) {
		CL_EffectManager_Close();
	} else if ( !Q_stricmp( arg, "place" ) ) {
		if ( em.state == EM_OFF )
			EM_Open();
		if ( em.state != EM_OFF && EM_Effect() ) {
			EM_SetTab( EM_TAB_EFFECTS );
			EM_EnterPlace();
		}
	} else if ( !Q_stricmp( arg, "edit" ) ) {
		if ( em.state == EM_OFF )
			EM_Open();
		if ( em.state != EM_OFF )
			EM_EnterSelect();
	} else if ( !Q_stricmp( arg, "bolt" ) && Cmd_Argc() >= 3 ) {
		em.where = EM_WHERE_BOLT;
		em.target = EM_TARGET_ME;
		Q_strncpyz( em.fields[EM_F_BOLT], Cmd_Argv( 2 ), EM_FIELD_LEN );
	} else if ( !Q_stricmp( arg, "reindex" ) ) {
		if ( em.state == EM_PLACE )
			EM_EnterBrowse();
		EM_BuildIndex();
		EM_RebuildEffectView();
	} else if ( !Q_stricmp( arg, "remove" ) && Cmd_Argc() >= 3 ) {
		if ( cls.state == CA_ACTIVE )
			EM_RemoveNamed( Cmd_Argv( 2 ) );
	} else if ( !Q_stricmp( arg, "names" ) ) {
		EM_PrintNames();
	} else if ( !Q_stricmp( arg, "savestate" ) && Cmd_Argc() >= 3 ) {
		if ( cls.state == CA_ACTIVE && !EM_SaveState( Cmd_Argv( 2 ) ) )
			Com_Printf( "%s\n", em.message );
	} else if ( !Q_stricmp( arg, "reloadstate" ) && Cmd_Argc() >= 3 ) {
		char name[MAX_QPATH];

		Q_strncpyz( name, Cmd_Argv( 2 ), sizeof( name ) );
		COM_StripExtension( name, name, sizeof( name ) );
		EM_ReloadState( name );
	} else if ( !Q_stricmp( arg, "loadconstruct" ) && Cmd_Argc() >= 3 ) {
		char name[MAX_QPATH];

		Q_strncpyz( name, Cmd_Argv( 2 ), sizeof( name ) );
		COM_StripExtension( name, name, sizeof( name ) );
		if ( em.state == EM_OFF ) {
			em.tab = EM_TAB_EFFECTS;
			EM_Open();
		}
		if ( em.state == EM_OFF )
			return;
		EM_EnterBrowse();
		EM_SetTab( EM_TAB_EFFECTS );
		EM_ShowConstructs();
		for ( int i = 0; i < em.numConstructs; i++ ) {
			if ( !Q_stricmp( em.constructs[i], name ) )
				EM_SelectConstruct( i );
		}
		if ( em.construct < 0 || Q_stricmp( em.constructs[em.construct], name ) )
			Com_Printf( S_COLOR_YELLOW "Effect manager: no construct %s/%s%s\n", EM_CONSTRUCT_DIR, name, EM_SAVE_EXT );
	} else if ( !Q_stricmp( arg, "cpiece" ) ) {
		// an effect of a construct being exec'd; its loadconstruct line read them all
	} else if ( !Q_stricmp( arg, "stated" ) ) {
		// an effect of a state being exec'd; its reloadstate line read them all
	} else if ( !Q_stricmp( arg, "help" ) || !Q_stricmp( arg, "?" ) ) {
		Com_Printf( "usage: effectmanager              toggle the effect manager (bind a key to it)\n" );
		Com_Printf( "       effectmanager <search>     open the effects filtered by <search>\n" );
		Com_Printf( "       effectmanager active       open on the effects playing in the map\n" );
		Com_Printf( "       effectmanager presets      open on alarm, fadeout and camerashake\n" );
		Com_Printf( "       effectmanager place        place the selected effect in the world\n" );
		Com_Printf( "       effectmanager edit         select and move the map's effects, and those waiting\n" );
		Com_Printf( "       effectmanager loadconstruct <name>  show a saved construct, to use\n" );
		Com_Printf( "       effectmanager bolt <tag>   bolt the selected effect to your own tag or bone\n" );
		Com_Printf( "       effectmanager names        list the names given to looping effects\n" );
		Com_Printf( "       effectmanager remove <name>  remove the effects with that name from the map\n" );
		Com_Printf( "       effectmanager savestate <name>    save the effects sent or named, and their names\n" );
		Com_Printf( "       effectmanager reloadstate <name>  give the effects of a state their names back, after rpdump load\n" );
		Com_Printf( "       effectmanager reindex      rescan the filesystem for effects\n" );
		Com_Printf( "       effectmanager close        close the effect manager\n" );
		Com_Printf( "States are in the %s folder of the game folder, and can be exec'd\n", EM_STATE_DIR );
	} else {
		// anything else is a search
		if ( em.state == EM_OFF ) {
			em.tab = EM_TAB_EFFECTS;
			EM_Open();
		}
		if ( em.state == EM_OFF )
			return;
		EM_EnterBrowse();
		EM_SetTab( EM_TAB_EFFECTS );
		Q_strncpyz( em.fields[EM_F_SEARCH], arg, EM_FIELD_LEN );
		EM_FieldChanged( EM_F_SEARCH );
	}
}

/*
===============================================================================

INPUT

===============================================================================
*/

static qboolean EM_InRect( float x, float y, float w, float h ) {
	return (qboolean)( em.cursorX >= x && em.cursorX < x + w && em.cursorY >= y && em.cursorY < y + h );
}

static qboolean EM_ShiftDown( void ) {
	return (qboolean)( em.held[A_SHIFT] || em.held[A_SHIFT2] );
}

static qboolean EM_CtrlDown( void ) {
	return (qboolean)( em.held[A_CTRL] || em.held[A_CTRL2] );
}

// the field typing goes to
static emField_t EM_ActiveField( void ) {
	if ( em.naming != EM_NAMING_NONE )
		return em.naming == EM_NAMING_TAG ? EM_F_NAME : em.naming == EM_NAMING_STATE ? EM_F_STATE_NAME : EM_F_CONSTRUCT_NAME;
	if ( em.picker != EM_PICKER_NONE )
		return EM_F_PICKER_SEARCH;
	if ( em.focus != EM_F_NONE )
		return em.focus;
	if ( em.tab == EM_TAB_ACTIVE )
		return EM_F_ACTIVE_SEARCH;
	if ( em.tab == EM_TAB_PRESETS )
		return EM_F_NONE;
	return EM_F_SEARCH;
}

// binds don't run while the manager holds the keys, so honour the user's toggle bind here
static qboolean EM_IsToggleKey( int key ) {
	const char *binding;

	// a printable key belongs to the text box
	if ( ( em.state == EM_BROWSE || em.naming != EM_NAMING_NONE ) && EM_ActiveField() != EM_F_NONE && key >= A_SPACE && key <= A_TILDE )
		return qfalse;
	binding = Key_GetBinding( key );
	return (qboolean)( VALIDSTRING( binding ) && !Q_stricmp( binding, EM_TOGGLE_CMD ) );
}

static qboolean EM_FieldAllows( emField_t f, int ch ) {
	if ( f == EM_F_NAME || f == EM_F_STATE_NAME || f == EM_F_CONSTRUCT_NAME )
		return EM_NameChar( ch );
	if ( f == EM_F_PITCH || f == EM_F_YAW || f == EM_F_ROLL )
		return (qboolean)( ( ch >= '0' && ch <= '9' ) || ch == '-' || ch == '.' );
	if ( f == EM_F_INTENSITY || f == EM_F_DURATION )
		return (qboolean)( ( ch >= '0' && ch <= '9' ) || ch == '.' );
	// goes on the command line as a single argument
	if ( f == EM_F_BOLT )
		return (qboolean)( ch > ' ' && ch < 127 && ch != '"' && ch != ';' );
	return (qboolean)( ch >= ' ' && ch < 127 );
}

// a character the field typed in takes
static qboolean EM_AllowChar( int ch ) {
	const emField_t f = EM_ActiveField();

	return (qboolean)( f != EM_F_NONE && EM_FieldAllows( f, ch ) );
}

// a search box typed in without a click: the list keeps its keys
static qboolean EM_ImplicitField( void ) {
	return (qboolean)( em.naming == EM_NAMING_NONE && ( em.picker != EM_PICKER_NONE || em.focus == EM_F_NONE ) );
}

static void EM_FieldChanged( int f ) {
	switch ( f ) {
	case EM_F_SEARCH:
		em.effectList.sel = em.effectList.scroll = 0;
		EM_RebuildEffectView();
		if ( em.numView )
			EM_SelectEffect( em.view[0] );
		break;
	case EM_F_ACTIVE_SEARCH:
		em.activeList.scroll = 0;
		EM_RebuildActiveView();
		break;
	case EM_F_PICKER_SEARCH:
		em.pickerList.sel = em.pickerList.scroll = 0;
		EM_RebuildPickerView();
		break;
	default:
		break;
	}
}

static void EM_BrowseListKey( int key ) {
	emList_t *list;
	int count, visible, delta;

	if ( em.picker != EM_PICKER_NONE ) {
		list = &em.pickerList;
		count = em.numPickerView;
		visible = (int)( ( EM_PICKER_H - 74 ) / EM_ROW_H );
	} else if ( em.tab == EM_TAB_EFFECTS ) {
		if ( key == A_CURSOR_LEFT || key == A_CURSOR_RIGHT ) {
			if ( !em.fields[EM_F_SEARCH][0] ) {
				EM_StepFolder( key == A_CURSOR_LEFT ? -1 : 1 );
				EM_SelectRow( 0 );
			}
			return;
		}
		list = &em.effectList;
		count = em.numView;
		visible = (int)( EM_LIST_H / EM_ROW_H );
	} else if ( em.tab == EM_TAB_ACTIVE ) {
		list = &em.activeList;
		count = em.numActiveView;
		visible = (int)( EM_LIST_H / EM_ROW_H );
	} else {
		return;
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
	EM_ListMove( list, count, visible, delta );
	if ( list == &em.effectList )
		EM_SelectRow( list->sel );
}

static void EM_BrowseKey( int key ) {
	emField_t f = EM_ActiveField();

	if ( f != EM_F_NONE && key != A_MOUSE1 && key != A_MWHEELUP && key != A_MWHEELDOWN ) {
		qboolean toField = qtrue;

		// a search box the list shares: up, down, page, home and end are the list's, left and
		// right too while it's empty
		if ( EM_ImplicitField() ) {
			if ( key == A_CURSOR_UP || key == A_CURSOR_DOWN || key == A_PAGE_UP || key == A_PAGE_DOWN || key == A_HOME || key == A_END )
				toField = qfalse;
			else if ( ( key == A_CURSOR_LEFT || key == A_CURSOR_RIGHT ) && !em.fields[f][0] )
				toField = qfalse;
		}
		if ( toField ) {
			const int edit = WG_EditKey( em.fields[f], EM_FIELD_LEN, key, EM_AllowChar );

			if ( edit ) {
				if ( edit & WG_EDIT_CHANGED )
					EM_FieldChanged( f );
				return;
			}
		}
	}

	switch ( key ) {
	case A_MOUSE1:
		em.click = qtrue;
		em.clickX = em.cursorX;
		em.clickY = em.cursorY;
		break;
	case A_MWHEELUP:
		em.wheel -= 3;
		break;
	case A_MWHEELDOWN:
		em.wheel += 3;
		break;
	case A_TAB:
		if ( em.picker == EM_PICKER_NONE )
			EM_SetTab( (emTab_t)( ( em.tab + 1 ) % 3 ) );
		break;
	case A_ENTER:
	case A_KP_ENTER:
		if ( em.picker != EM_PICKER_NONE ) {
			if ( em.pickerList.sel < em.numPickerView )
				EM_ChoosePicker( em.pickerView[em.pickerList.sel] );
		} else if ( em.tab == EM_TAB_EFFECTS ) {
			if ( EM_ConstructShown() )
				EM_ApplyConstructBolts( qfalse );
			else
				EM_Send();
		}
		break;
	default:
		EM_BrowseListKey( key );
		break;
	}
}

static void EM_GzView( gzView_t *v ) {
	VectorCopy( em.viewOrg, v->org );
	AxisCopy( em.viewAxis, v->axis );
	v->fovX = em.fovX;
	v->fovY = em.fovY;
}

// what the arrows move: the selection, or the effect being put down; qfalse if nothing
static qboolean EM_GzPivot( vec3_t pivot ) {
	if ( em.selecting ) {
		if ( !EM_NumSelected() )
			return qfalse;
		EM_Pivot( pivot );
		return qtrue;
	}
	if ( em.where == EM_WHERE_BOLT && !em.handConstruct )
		return qfalse;
	VectorCopy( em.origin, pivot );
	return qtrue;
}

// the arrow held: what it moves goes where it says along its axis
static void EM_GizmoFrame( void ) {
	gzView_t v;
	vec3_t pivot, to, delta;
	emHandle_t hd;

	em.gzHot = -1;
	if ( !em.gizmo || !em.haveView || !EM_GzPivot( pivot ) )
		return;
	EM_GzView( &v );
	if ( !em.gzDragging ) {
		em.gzHot = GZ_Hit( &v, pivot, em.cursorX, em.cursorY );
		return;
	}
	em.gzHot = em.gzDrag.axis;
	GZ_Target( &em.gzDrag, em.cursorX, em.cursorY, emSnaps[em.snap].grid, to );
	if ( !em.selecting ) {
		VectorCopy( to, em.origin );
		return;
	}
	VectorSubtract( to, pivot, delta );
	for ( int h = EM_FirstHandle(); h < EM_EndHandle(); h = EM_NextHandle( h ) ) {
		if ( EM_GetHandle( h, &hd ) && *hd.selected )
			VectorAdd( hd.pos, delta, hd.pos );
	}
}

// the arrows' key and mouse: qtrue if they took it
static qboolean EM_GizmoKey( int key ) {
	vec3_t pivot;
	gzView_t v;

	if ( key == GZ_KEY ) {
		em.gizmo = (qboolean)!em.gizmo;
		em.gzDragging = qfalse;
		em.cursorX = SCREEN_WIDTH * 0.5f;
		em.cursorY = SCREEN_HEIGHT * 0.5f;
		EM_Message( em.gizmo ? "Move arrows: drag X, Y or Z to move it along; hold the right mouse button to look; J hides them" : "Move arrows hidden" );
		return qtrue;
	}
	if ( !em.gizmo )
		return qfalse;
	if ( key == A_MOUSE2 )
		return qtrue;
	if ( key != A_MOUSE1 || em.gzHot < 0 || !EM_GzPivot( pivot ) )
		return qfalse;
	EM_GzView( &v );
	GZ_Begin( &em.gzDrag, &v, pivot, em.gzHot, em.cursorX, em.cursorY );
	em.gzDragging = em.gzDrag.on;
	// held where the arrow puts it, not where the crosshair is
	if ( em.selecting )
		em.carryLocked = qtrue;
	else
		em.locked = qtrue;
	return qtrue;
}

// how big: the effect being put down, or the selection
static void EM_ScaleKeyPressed( int sign ) {
	const float f = sign > 0 ? EM_SCALE_STEP : 1.0f / EM_SCALE_STEP;
	emHandle_t hd;

	if ( !em.selecting ) {
		em.scale = EM_ClampScale( EM_ClampScale( em.scale ) * f );
		EM_Message( va( "Size x%.2g " S_COLOR_GREY "(experimental: everyone with this client sees it)", em.scale ) );
		return;
	}
	for ( int h = EM_FirstHandle(); h < EM_EndHandle(); h = EM_NextHandle( h ) ) {
		if ( !EM_GetHandle( h, &hd ) || !*hd.selected )
			continue;
		if ( hd.active )
			EM_SetScale( hd.active->path, hd.active->origin, EM_ScaleAt( hd.active->path, hd.active->origin ) * f, qfalse );
		else
			em.waiting[h].scale = EM_ClampScale( EM_ClampScale( em.waiting[h].scale ) * f );
	}
}

static void EM_Rotate( int axis, int sign ) {
	vec3_t angles;
	int step = emSnaps[em.snap].angle;

	EM_GetAngles( angles );
	angles[axis] = (float)EM_NormalizeAngle( EM_SnapTo( angles[axis] + sign * step, step ) );
	EM_SetAngles( angles );
	em.align = qfalse;
}

// moves along the world axis closest to the camera direction, so nudges stay on the grid
static void EM_Nudge( qboolean forwardAxis, int sign ) {
	vec3_t forward, right;
	float yaw = DEG2RAD( floorf( em.camAng[YAW] / 90.0f + 0.5f ) * 90.0f );
	int grid = emSnaps[em.snap].grid;

	VectorSet( forward, cosf( yaw ), sinf( yaw ), 0 );
	VectorSet( right, sinf( yaw ), -cosf( yaw ), 0 );
	em.locked = qtrue;
	VectorMA( em.origin, (float)( sign * grid ), forwardAxis ? forward : right, em.origin );
	em.origin[0] = floorf( em.origin[0] + 0.5f );
	em.origin[1] = floorf( em.origin[1] + 0.5f );
}

// the free camera selecting: the effects waiting and the map's, to move or remove
static void EM_SelectKey( int key ) {
	switch ( key ) {
	case A_MOUSE1:
		if ( em.carrying )
			EM_DropSelection();
		else
			EM_ClickSelect();
		break;
	case A_MOUSE2:
	case A_CAP_F:
		if ( em.carrying )
			em.carryLocked = (qboolean)!em.carryLocked;
		else
			EM_PickUp();
		break;
	case A_ENTER:
	case A_KP_ENTER:
		em.carrying = qfalse;
		EM_SendAllChanges();
		break;
	case A_MWHEELUP:	EM_RotateSelection( YAW, 1 );	break;
	case A_MWHEELDOWN:	EM_RotateSelection( YAW, -1 );	break;
	case A_CAP_Z:		EM_RotateSelection( PITCH, -1 );	break;
	case A_CAP_X:		EM_RotateSelection( PITCH, 1 );	break;
	case A_CAP_Q:		EM_RotateSelection( ROLL, -1 );	break;
	case A_CAP_E:		EM_RotateSelection( ROLL, 1 );	break;
	case A_CAP_R:
		EM_StraightenSelection();
		break;
	case A_CURSOR_UP:		EM_NudgeSelection( 1, 0, 0 );	break;
	case A_CURSOR_DOWN:		EM_NudgeSelection( -1, 0, 0 );	break;
	case A_CURSOR_RIGHT:	EM_NudgeSelection( 0, 1, 0 );	break;
	case A_CURSOR_LEFT:		EM_NudgeSelection( 0, -1, 0 );	break;
	case A_PAGE_UP:			EM_NudgeSelection( 0, 0, 1 );	break;
	case A_PAGE_DOWN:		EM_NudgeSelection( 0, 0, -1 );	break;
	case A_DELETE:
		EM_DeleteSelection();
		break;
	case A_CAP_U:
		EM_UndoChanges();
		break;
	case A_CAP_I:
		em.instant = (qboolean)!em.instant;
		break;
	case A_CAP_G:
		em.snap = ( em.snap + 1 ) % ARRAY_LEN( emSnaps );
		break;
	case A_CAP_H:
		em.hideHelp = (qboolean)!em.hideHelp;
		break;
	case A_CAP_L:
		EM_RequestList();
		break;
	case A_CAP_K:
		EM_StartNaming( EM_NAMING_CONSTRUCT );
		break;
	case A_CAP_V:
		// back to putting the form's effect down, if there is one
		EM_CancelCarry();
		if ( EM_Effect() ) {
			em.selecting = qfalse;
			em.playKey[0] = '\0';
		} else {
			EM_Message( S_COLOR_YELLOW "Pick an effect in the manager first to put one down" );
		}
		break;
	case A_TAB:
		EM_EnterBrowse();
		break;
	default:
		break;
	}
}

static void EM_PlaceKey( int key ) {
	const qboolean bolted = (qboolean)( em.where == EM_WHERE_BOLT );

	if ( EM_GizmoKey( key ) )
		return;
	if ( ( key == A_OPEN_SQUARE || key == A_CLOSE_SQUARE || key == A_MINUS || key == A_EQUALS || key == A_KP_MINUS || key == A_KP_PLUS )
		&& !em.handConstruct && ( em.selecting || !bolted ) ) {
		EM_ScaleKeyPressed( key == A_CLOSE_SQUARE || key == A_EQUALS || key == A_KP_PLUS ? 1 : -1 );
		return;
	}
	if ( em.selecting ) {
		EM_SelectKey( key );
		return;
	}
	if ( key == A_CAP_V ) {
		EM_EnterSelect();
		return;
	}

	// a construct in hand: a copy goes down with each click
	if ( em.handConstruct ) {
		switch ( key ) {
		case A_MOUSE1:
			EM_PutDownHand( em.instant );
			return;
		case A_ENTER:
		case A_KP_ENTER:
			if ( em.numWaiting )
				EM_SendWaiting();
			else
				EM_PutDownHand( qtrue );
			return;
		case A_MWHEELUP:
		case A_MWHEELDOWN:
			em.handYaw = EM_NormalizeAngle( (float)( em.handYaw + ( key == A_MWHEELUP ? 1 : -1 ) * emSnaps[em.snap].angle ) );
			return;
		case A_CAP_R:
		case A_BACKSPACE:
			em.handYaw = 0;
			return;
		case A_CAP_Z:
		case A_CAP_X:
		case A_CAP_Q:
		case A_CAP_E:
		case A_CAP_N:
		case A_CAP_M:
			// its effects keep the angles and looping they were saved with
			return;
		default:
			break;
		}
	}

	switch ( key ) {
	case A_MOUSE1:
		// right away, or to wait for the others
		if ( em.instant )
			EM_Send();
		else
			EM_AddWaiting();
		break;
	case A_ENTER:
	case A_KP_ENTER:
		if ( em.numWaiting )
			EM_SendWaiting();
		else
			EM_Send();
		break;
	case A_CAP_I:
		em.instant = (qboolean)!em.instant;
		break;
	case A_CAP_U:
		// takes back the last one put down
		if ( em.numWaiting )
			em.numWaiting--;
		break;
	case A_CAP_M:
		em.loop = (qboolean)!em.loop;
		break;
	case A_CAP_H:
		em.hideHelp = (qboolean)!em.hideHelp;
		break;
	case A_TAB:
		EM_EnterBrowse();
		break;
	default:
		break;
	}
	if ( bolted && !em.handConstruct )
		return;

	switch ( key ) {
	case A_MOUSE2:
	case A_CAP_F:
		em.locked = (qboolean)!em.locked;
		break;
	case A_MWHEELUP:	EM_Rotate( YAW, 1 );	break;
	case A_MWHEELDOWN:	EM_Rotate( YAW, -1 );	break;
	case A_CAP_Z:		EM_Rotate( PITCH, -1 );	break;
	case A_CAP_X:		EM_Rotate( PITCH, 1 );	break;
	case A_CAP_Q:		EM_Rotate( ROLL, -1 );	break;
	case A_CAP_E:		EM_Rotate( ROLL, 1 );	break;
	case A_CAP_R:
	case A_BACKSPACE:
		em.fields[EM_F_PITCH][0] = em.fields[EM_F_YAW][0] = em.fields[EM_F_ROLL][0] = '\0';
		em.align = qfalse;
		break;
	case A_CAP_N:
		em.align = (qboolean)!em.align;
		break;
	case A_CAP_G:
		em.snap = ( em.snap + 1 ) % ARRAY_LEN( emSnaps );
		break;
	case A_CURSOR_UP:		EM_Nudge( qtrue, 1 );	break;
	case A_CURSOR_DOWN:		EM_Nudge( qtrue, -1 );	break;
	case A_CURSOR_RIGHT:	EM_Nudge( qfalse, 1 );	break;
	case A_CURSOR_LEFT:		EM_Nudge( qfalse, -1 );	break;
	case A_PAGE_UP:
		em.locked = qtrue;
		em.origin[2] += emSnaps[em.snap].grid;
		break;
	case A_PAGE_DOWN:
		em.locked = qtrue;
		em.origin[2] -= emSnaps[em.snap].grid;
		break;
	default:
		break;
	}
}

// the active effect whose label is under the cursor gets checked or unchecked
static void EM_PickClick( void ) {
	float best = 24.0f * 24.0f, x, y;
	int found = -1;

	for ( int i = 0; i < em.numActive; i++ ) {
		if ( !EM_Project( em.active[i].origin, &x, &y ) )
			continue;
		x -= em.cursorX;
		y -= em.cursorY;
		if ( x * x + y * y < best ) {
			best = x * x + y * y;
			found = i;
		}
	}
	if ( found >= 0 )
		em.active[found].checked = (qboolean)!em.active[found].checked;
}

// the name box: what is sent named, or the state saved
static void EM_StartNaming( emNaming_t naming ) {
	emItem_t it;

	if ( naming == EM_NAMING_STATE ) {
		if ( !EM_StateCount() ) {
			EM_Message( S_COLOR_YELLOW "Nothing to save in a state: no looping effects were sent or named yet" );
			return;
		}
		Q_strncpyz( em.fields[EM_F_STATE_NAME], COM_SkipPath( (char *)EM_MapName() ), EM_FIELD_LEN );
		for ( char *c = em.fields[EM_F_STATE_NAME]; *c; c++ ) {
			if ( !EM_NameChar( *c ) )
				*c = '_';
		}
	} else if ( naming == EM_NAMING_CONSTRUCT ) {
		// the free camera's selection, or what waits
		em.constructFromSelection = (qboolean)( em.state == EM_PLACE && em.selecting );
		if ( em.constructFromSelection ? !EM_NumSelected() : !em.numWaiting ) {
			EM_Message( em.constructFromSelection ? S_COLOR_YELLOW "Select the effects to save first" : S_COLOR_YELLOW "Add effects to waiting first" );
			return;
		}
	} else if ( em.state == EM_PLACE && em.selecting ) {
		if ( !EM_NumSelected() ) {
			EM_Message( S_COLOR_YELLOW "Select the effects to name first" );
			return;
		}
	} else if ( !em.numWaiting && !EM_FormItem( &it ) ) {
		return;
	}
	em.naming = naming;
	em.focus = EM_F_NONE;
	EM_ReleaseKeys();
}

static void EM_ConfirmNaming( void ) {
	const char *name = em.fields[EM_F_NAME];

	if ( em.naming == EM_NAMING_STATE ) {
		if ( EM_SaveState( em.fields[EM_F_STATE_NAME] ) )
			em.naming = EM_NAMING_NONE;
		return;
	}
	if ( em.naming == EM_NAMING_CONSTRUCT ) {
		if ( EM_SaveConstruct( em.fields[EM_F_CONSTRUCT_NAME] ) )
			em.naming = EM_NAMING_NONE;
		return;
	}
	// selecting: the selected ones are named, the map's here only, as RPMod keeps no names
	if ( em.state == EM_PLACE && em.selecting ) {
		emHandle_t hd;
		int count = 0;

		em.naming = EM_NAMING_NONE;
		for ( int h = EM_FirstHandle(); h < EM_EndHandle(); h = EM_NextHandle( h ) ) {
			if ( !EM_GetHandle( h, &hd ) || !*hd.selected )
				continue;
			if ( hd.active )
				Q_strncpyz( hd.active->tag, name, sizeof( hd.active->tag ) );
			else
				Q_strncpyz( em.waiting[h].tag, name, sizeof( em.waiting[h].tag ) );
			count++;
		}
		EM_Message( name[0] ? va( S_COLOR_GREEN "Named %i effects %s", count, name ) : va( "Took the name off %i effects", count ) );
		return;
	}

	// what waits, or the form's one
	qboolean looping = (qboolean)( !em.numWaiting && em.loop );

	em.naming = EM_NAMING_NONE;
	for ( int i = 0; i < em.numWaiting; i++ ) {
		Q_strncpyz( em.waiting[i].tag, name, sizeof( em.waiting[i].tag ) );
		if ( em.waiting[i].loop )
			looping = qtrue;
	}
	if ( em.numWaiting )
		EM_SendWaiting();
	else
		EM_Send();
	if ( name[0] && !looping )
		EM_Message( S_COLOR_YELLOW "Sent, but only looping effects stay on the map to keep a name" );
	else if ( name[0] )
		EM_Message( va( S_COLOR_GREEN "Sent, named %s", name ) );
}

// the keys the name box takes; qtrue if it took this one
static qboolean EM_NamingKey( int key ) {
	const emField_t f = EM_ActiveField();

	if ( WG_EditKey( em.fields[f], EM_FIELD_LEN, key, EM_AllowChar ) )
		return qtrue;
	if ( key == A_ENTER || key == A_KP_ENTER )
		EM_ConfirmNaming();
	return qtrue;
}

void CL_EffectManager_KeyEvent( int key, qboolean down ) {
	if ( em.state == EM_OFF )
		return;

	// letters typed into the name box don't fly the camera
	if ( key >= 0 && key < MAX_KEYS )
		em.held[key] = (qboolean)( down && em.naming == EM_NAMING_NONE );
	// an arrow let go: moved ones of the map's go out as they would on a click
	if ( !down && key == A_MOUSE1 && em.gzDragging ) {
		em.gzDragging = qfalse;
		if ( em.selecting && em.instant )
			EM_SendActiveChanges( qtrue );
		return;
	}
	if ( !down )
		return;

	if ( EM_IsToggleKey( key ) ) {
		EM_Close();
		return;
	}

	if ( em.naming != EM_NAMING_NONE && EM_NamingKey( key ) )
		return;
	if ( ( key == A_ENTER || key == A_KP_ENTER ) && EM_ShiftDown() && em.state != EM_PICK
		&& ( em.state == EM_PLACE || ( em.tab == EM_TAB_EFFECTS && em.picker == EM_PICKER_NONE ) ) ) {
		EM_StartNaming( EM_NAMING_TAG );
		return;
	}

	switch ( em.state ) {
	case EM_BROWSE:
		EM_BrowseKey( key );
		break;
	case EM_PLACE:
		EM_PlaceKey( key );
		break;
	case EM_PICK:
		if ( key == A_MOUSE1 )
			EM_PickClick();
		else if ( key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER )
			EM_EnterBrowse();
		break;
	default:
		break;
	}
}

void CL_EffectManager_CharEvent( int ch ) {
	emField_t f;

	if ( em.state != EM_BROWSE && em.naming == EM_NAMING_NONE )
		return;
	f = EM_ActiveField();
	if ( f == EM_F_NONE )
		return;
	if ( WG_EditChar( em.fields[f], EM_FIELD_LEN, ch, EM_AllowChar ) )
		EM_FieldChanged( f );
}

void CL_EffectManager_MouseEvent( int dx, int dy ) {
	if ( em.state == EM_PLACE && !( em.gizmo && !em.held[A_MOUSE2] ) ) {
		em.camAng[YAW] -= dx * cl_sensitivity->value * m_yaw->value;
		em.camAng[PITCH] = Com_Clamp( -89.0f, 89.0f, em.camAng[PITCH] + dy * cl_sensitivity->value * m_pitch->value );
		return;
	}
	em.cursorX = Com_Clamp( 0, SCREEN_WIDTH, em.cursorX + dx );
	em.cursorY = Com_Clamp( 0, SCREEN_HEIGHT, em.cursorY + dy );
}

// escape backs out one level: picker, a text box, the world, then the manager
void CL_EffectManager_Escape( void ) {
	if ( em.naming != EM_NAMING_NONE )
		em.naming = EM_NAMING_NONE;
	else if ( em.state == EM_PLACE && em.carrying )
		EM_CancelCarry();
	else if ( em.state == EM_PLACE || em.state == EM_PICK )
		EM_EnterBrowse();
	else if ( em.picker != EM_PICKER_NONE )
		em.picker = EM_PICKER_NONE;
	else if ( em.focus != EM_F_NONE )
		em.focus = EM_F_NONE;
	else
		EM_Close();
}

/*
===============================================================================

3D VIEW

===============================================================================
*/

static void EM_UpdateCamera( float dt ) {
	vec3_t forward, right, move;
	float speed;

	AngleVectors( em.camAng, forward, right, NULL );
	VectorClear( move );
	if ( em.held[A_CAP_W] )		VectorAdd( move, forward, move );
	if ( em.held[A_CAP_S] )		VectorSubtract( move, forward, move );
	if ( em.held[A_CAP_D] )		VectorAdd( move, right, move );
	if ( em.held[A_CAP_A] )		VectorSubtract( move, right, move );
	if ( em.held[A_SPACE] )		move[2] += 1.0f;
	if ( em.held[A_CAP_C] )		move[2] -= 1.0f;

	if ( VectorNormalize( move ) > 0.0f ) {
		speed = EM_FLY_SPEED;
		if ( EM_ShiftDown() )
			speed *= 3.0f;
		if ( EM_CtrlDown() )
			speed *= 0.25f;
		VectorMA( em.camOrg, speed * dt, move, em.camOrg );
	}
}

// put the effect where the crosshair meets the world, just off that surface
static void EM_FollowCrosshair( void ) {
	vec3_t forward, end, angles;
	trace_t tr;
	int grid = emSnaps[em.snap].grid;

	AngleVectors( em.camAng, forward, NULL, NULL );
	VectorMA( em.camOrg, EM_TRACE_DIST, forward, end );
	CM_BoxTrace( &tr, em.camOrg, end, vec3_origin, vec3_origin, 0, EM_TRACE_MASK, qfalse );

	if ( tr.fraction < 1.0f && !tr.startsolid && !( tr.surfaceFlags & SURF_SKY ) ) {
		VectorMA( tr.endpos, 1.0f, tr.plane.normal, em.origin );
		// snap along the surface, not off it
		for ( int i = 0; i < 3; i++ ) {
			if ( fabsf( tr.plane.normal[i] ) < 0.7f )
				em.origin[i] = EM_SnapTo( em.origin[i], grid );
		}
		if ( em.align ) {
			vectoangles( tr.plane.normal, angles );
			EM_SetAngles( angles );
		}
	} else {
		VectorMA( em.camOrg, EM_NO_HIT_DIST, forward, em.origin );
		for ( int i = 0; i < 3; i++ )
			em.origin[i] = EM_SnapTo( em.origin[i], grid );
	}
}

static void EM_ExpireModels( void ) {
	for ( int i = 0; i < EM_MAX_RETIRED; i++ ) {
		if ( em.retired[i] && cls.realtime - em.retiredTime[i] > EM_RETIRE_MS ) {
			re->G2API_CleanGhoul2Models( &em.retired[i] );
			em.retired[i] = NULL;
		}
	}
}

// called every frame before the cgame draws
void CL_EffectManager_Frame( void ) {
	float dt;

	// queued commands go out, and lists are read, even with the manager closed
	EM_RunQueue();
	EM_ExpireModels();
	EM_ScaleShareFrame();

	// a list read whole
	if ( em.listLineTime && cls.realtime - em.listLineTime >= EM_LIST_LINE_GAP ) {
		em.listLineTime = 0;
		EM_ListDone();
	}
	// a list we asked for that never came
	if ( em.listRequestTime && cls.realtime - em.listRequestTime > EM_LIST_TIMEOUT ) {
		em.listRequestTime = 0;
		em.listTimeoutTime = cls.realtime;
		if ( em.state != EM_OFF )
			EM_RebuildActiveView();
		if ( em.reloading && !em.listQueued ) {
			em.reloading = qfalse;
			EM_Report( va( S_COLOR_YELLOW "Couldn't reload state %s: the server didn't send its list of effects", em.reloadName ) );
		}
	}

	if ( em.state == EM_OFF )
		return;
	if ( !( Key_GetCatcher() & KEYCATCH_EFFECTMANAGER ) ) {
		// something else cleared the catchers
		em.state = EM_OFF;
		em.naming = EM_NAMING_NONE;
		EM_ReleaseKeys();
		return;
	}

	// a list that is due
	if ( em.refreshTime && cls.realtime >= em.refreshTime )
		EM_RequestList();

	dt = Com_Clamp( 0.0f, 0.1f, ( cls.realtime - em.lastFrameTime ) / 1000.0f );
	em.lastFrameTime = cls.realtime;
	if ( em.state == EM_PLACE ) {
		EM_UpdateCamera( dt );
		EM_GizmoFrame();
		if ( em.selecting ) {
			if ( em.carrying && !em.carryLocked )
				EM_FollowSelection();
			em.hover = em.carrying ? -1 : EM_HoverAtCrosshair();
		} else if ( ( em.handConstruct || em.where != EM_WHERE_BOLT ) && !em.locked ) {
			EM_FollowCrosshair();
		}
	}

	EM_PreviewFrame();
}

// a thin bar both ways round, as the model placer draws its box
static void EM_AddEdge( const vec3_t a, const vec3_t b, const byte *color ) {
	polyVert_t verts[4], back[4];
	vec3_t dir, toCam, side, mid;
	float width;

	VectorSubtract( b, a, dir );
	VectorAdd( a, b, mid );
	VectorScale( mid, 0.5f, mid );
	VectorSubtract( em.camOrg, mid, toCam );
	width = 0.3f + VectorLength( toCam ) * 0.001f;
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
	for ( int i = 0; i < 4; i++ )
		back[i] = verts[3 - i];
	re->AddPolyToScene( cls.whiteShader, 4, verts, 1 );
	re->AddPolyToScene( cls.whiteShader, 4, back, 1 );
}

static void EM_AddCross( const vec3_t origin, const vec3_t dir, const byte *crossColor, const byte *dirColor ) {
	vec3_t a, b;

	for ( int i = 0; i < 3; i++ ) {
		VectorCopy( origin, a );
		VectorCopy( origin, b );
		a[i] -= 6.0f;
		b[i] += 6.0f;
		EM_AddEdge( a, b, crossColor );
	}
	VectorMA( origin, 32.0f, dir, b );
	EM_AddEdge( origin, b, dirColor );
}

// a small cross where the effect goes, and which way it faces; green ones wait there
static void EM_AddMarker( void ) {
	static const byte crossColor[4] = { 255, 255, 255, 255 };
	static const byte dirColor[4] = { 255, 200, 64, 255 };
	static const byte sentColor[4] = { 64, 255, 96, 255 };
	static const byte selColor[4] = { 64, 220, 255, 255 };
	static const byte hoverColor[4] = { 255, 255, 64, 255 };
	static const byte movedColor[4] = { 255, 96, 255, 255 };
	static const byte deletedColor[4] = { 255, 64, 64, 255 };
	static const byte mapColor[4] = { 170, 170, 170, 255 };
	vec3_t dir;

	if ( em.selecting ) {
		emHandle_t hd;

		for ( int h = EM_FirstHandle(); h < EM_EndHandle(); h = EM_NextHandle( h ) ) {
			const byte *color;

			if ( !EM_GetHandle( h, &hd ) )
				continue;
			if ( h == em.hover )
				color = hoverColor;
			else if ( *hd.selected )
				color = selColor;
			else if ( !hd.active )
				color = sentColor;
			else if ( hd.active->deleted )
				color = deletedColor;
			else if ( EM_ActiveEdited( hd.active ) )
				color = movedColor;
			else
				color = mapColor;
			if ( *hd.haveAngles )
				AngleVectors( hd.angles, dir, NULL, NULL );
			else
				VectorSet( dir, 0, 0, 1 );
			EM_AddCross( hd.pos, dir, color, color );
			// a moved one of the map's: a small cross where the server still has it
			if ( hd.active && Distance( hd.active->origin, hd.pos ) > 0.5f ) {
				vec3_t a, b;

				for ( int i = 0; i < 2; i++ ) {
					VectorCopy( hd.active->origin, a );
					VectorCopy( hd.active->origin, b );
					a[i] -= 4.0f;
					b[i] += 4.0f;
					EM_AddEdge( a, b, movedColor );
				}
			}
		}
		return;
	}

	for ( int i = 0; i < em.numWaiting; i++ ) {
		if ( em.waiting[i].bolted || !em.waiting[i].havePos )
			continue;
		EM_ItemDirection( &em.waiting[i], dir );
		EM_AddCross( em.waiting[i].pos, dir, sentColor, sentColor );
	}
	// a construct in hand, each of its effects
	if ( em.handConstruct ) {
		for ( int i = 0; i < em.numHand; i++ ) {
			emItem_t it;

			EM_HandItem( i, em.origin, &it );
			EM_ItemDirection( &it, dir );
			EM_AddCross( it.pos, dir, crossColor, dirColor );
		}
		return;
	}
	if ( em.where == EM_WHERE_BOLT )
		return;
	EM_Direction( dir );
	EM_AddCross( em.origin, dir, crossColor, cls.realtime - em.sentTime < 300 ? sentColor : dirColor );
}

// every entity cgame adds; models are kept to find the bolt target among them
void CL_EffectManager_AddEntity( const refEntity_t *ent ) {
	emCaptured_t *c;

	if ( em.state == EM_OFF || !ent->ghoul2 || ent->reType != RT_MODEL || em.numPending >= EM_MAX_CAPTURED )
		return;
	if ( !re->G2API_HaveWeGhoul2Models( *(CGhoul2Info_v *)ent->ghoul2 ) )
		return;
	c = &em.pending[em.numPending++];
	c->ghoul2 = (CGhoul2Info_v *)ent->ghoul2;
	VectorCopy( ent->origin, c->origin );
	VectorCopy( ent->modelScale, c->scale );
}

// the view weapon would float where the player stands
qboolean CL_EffectManager_FilterEntity( const refEntity_t *ent ) {
	return (qboolean)( em.state == EM_PLACE && ( ent->renderfx & RF_FIRST_PERSON ) );
}

// the free camera, for the effects system to cull against
qboolean CL_EffectManager_Camera( vec3_t origin, vec3_t angles ) {
	if ( em.state != EM_PLACE )
		return qfalse;
	VectorCopy( em.camOrg, origin );
	VectorCopy( em.camAng, angles );
	return qtrue;
}

static void EM_KeepView( const refdef_t *fd ) {
	VectorCopy( fd->vieworg, em.viewOrg );
	VectorCopy( fd->viewaxis[0], em.viewAxis[0] );
	VectorCopy( fd->viewaxis[1], em.viewAxis[1] );
	VectorCopy( fd->viewaxis[2], em.viewAxis[2] );
	em.fovX = fd->fov_x;
	em.fovY = fd->fov_y;
	em.haveView = qtrue;
}

// every cgame scene passes through here before reaching the renderer; qtrue if it was rendered
qboolean CL_EffectManager_RenderScene( const refdef_t *fd ) {
	refdef_t view;

	if ( em.state == EM_OFF ) {
		em.numPending = 0;
		return qfalse;
	}
	if ( fd->rdflags & ( RDF_NOWORLDMODEL | RDF_AUTOMAP ) ) {
		em.numPending = 0;
		return qfalse;
	}

	if ( fd->rdflags & RDF_SKYBOXPORTAL ) {
		em.numPending = 0;
		if ( em.state != EM_PLACE )
			return qfalse;
		// the sky portal keeps its own origin but looks the way the camera does
		view = *fd;
		AnglesToAxis( em.camAng, view.viewaxis );
		VectorCopy( em.camAng, view.viewangles );
		re->RenderScene( &view );
		return qtrue;
	}

	// the main view: the models in it are all in, and still alive
	EM_UpdateTarget();
	em.numPending = 0;

	if ( em.state != EM_PLACE ) {
		EM_KeepView( fd );
		return qfalse;
	}

	view = *fd;
	if ( em.viewFrame != cls.framecount ) {
		em.viewFrame = cls.framecount;
		EM_AddMarker();
	}
	VectorCopy( em.camOrg, view.vieworg );
	VectorCopy( em.camAng, view.viewangles );
	AnglesToAxis( em.camAng, view.viewaxis );
	view.viewContents = CM_PointContents( em.camOrg, 0 );
	// the snapshot's area mask is from the player's position, the camera can be anywhere
	memset( view.areamask, 0, sizeof( view.areamask ) );
	EM_KeepView( &view );
	re->RenderScene( &view );
	return qtrue;
}

/*
===============================================================================

2D

===============================================================================
*/

static void EM_Fill( float x, float y, float w, float h, const float *color ) {
	re->SetColor( color );
	re->DrawStretchPic( x, y, w, h, 0, 0, 0, 0, cls.whiteShader );
	re->SetColor( NULL );
}

static void EM_Box( float x, float y, float w, float h, const float *fill ) {
	if ( fill )
		EM_Fill( x, y, w, h, fill );
	EM_Fill( x, y, w, 1, emBorder );
	EM_Fill( x, y + h - 1, w, 1, emBorder );
	EM_Fill( x, y, 1, h, emBorder );
	EM_Fill( x + w - 1, y, 1, h, emBorder );
}

static void EM_Text( float x, float y, const char *text, const float *color ) {
	re->Font_DrawString( (int)x, (int)y, text, color, em.font | STYLE_DROPSHADOW, -1, EM_TEXT_SCALE );
}

static float EM_TextWidth( const char *text ) {
	return (float)re->Font_StrLenPixels( text, em.font, EM_TEXT_SCALE );
}

static void EM_TextClipped( float x, float y, float w, const char *text, const float *color ) {
	char buf[MAX_STRING_CHARS];
	int len;

	Q_strncpyz( buf, text, sizeof( buf ) );
	len = strlen( buf );
	while ( len > 3 && EM_TextWidth( buf ) > w ) {
		len--;
		buf[len - 3] = '.';
		buf[len - 2] = '.';
		buf[len - 1] = '.';
		buf[len] = '\0';
	}
	EM_Text( x, y, buf, color );
}

// eats this frame's click if it landed in the rect
static qboolean EM_Clicked( float x, float y, float w, float h ) {
	if ( !em.click || em.clickX < x || em.clickX >= x + w || em.clickY < y || em.clickY >= y + h )
		return qfalse;
	em.click = qfalse;
	return qtrue;
}

static qboolean EM_Button( float x, float y, float w, const char *label, qboolean selected, qboolean enabled ) {
	const float *fill = selected ? emHighlight : ( enabled && EM_InRect( x, y, w, EM_CTRL_H ) ) ? emHoverBox : emPanelLight;

	EM_Box( x, y, w, EM_CTRL_H, fill );
	EM_Text( x + ( w - EM_TextWidth( label ) ) * 0.5f, y + 2, label, enabled ? emWhite : emDim );
	return (qboolean)( EM_Clicked( x, y, w, EM_CTRL_H ) && enabled );
}

// a button that has to be clicked twice
static qboolean EM_DangerButton( float x, float y, float w, const char *label, emConfirm_t id, qboolean enabled ) {
	qboolean armed = (qboolean)( em.confirm == id && cls.realtime - em.confirmTime < EM_CONFIRM_MS );
	const char *text = armed ? "Click again" : label;

	EM_Box( x, y, w, EM_CTRL_H, armed ? emDanger : ( enabled && EM_InRect( x, y, w, EM_CTRL_H ) ) ? emHoverBox : emPanelLight );
	EM_Text( x + ( w - EM_TextWidth( text ) ) * 0.5f, y + 2, text, enabled ? emWhite : emDim );
	if ( !EM_Clicked( x, y, w, EM_CTRL_H ) || !enabled )
		return qfalse;
	if ( armed ) {
		em.confirm = EM_CONFIRM_NONE;
		return qtrue;
	}
	em.confirm = id;
	em.confirmTime = cls.realtime;
	return qfalse;
}

static void EM_FieldStyle( wgStyle_t *style ) {
	style->font = em.font;
	style->scale = EM_TEXT_SCALE;
	style->lineH = EM_ROW_H;
	style->padX = 4.0f;
	style->padY = 2.0f;
	style->fill = emPanelFocus;
	style->border = emBorder;
	style->text = emWhite;
	style->dim = emDim;
}

static void EM_Field( emField_t f, float x, float y, float w, const char *placeholder ) {
	const qboolean focused = (qboolean)( EM_ActiveField() == f );
	wgStyle_t style;
	float h;

	EM_FieldStyle( &style );
	EM_Box( x, y, w, EM_CTRL_H, focused ? emPanelFocus : emPanelLight );
	WG_Field( &style, x, y, w, EM_CTRL_H, em.fields[f], focused, placeholder, em.cursorX, em.cursorY );
	h = WG_FieldHeight( &style, w, EM_CTRL_H, em.fields[f], focused );
	if ( EM_Clicked( x, y, w, h ) ) {
		em.focus = ( f == EM_F_SEARCH || f == EM_F_ACTIVE_SEARCH || f == EM_F_PICKER_SEARCH ) ? EM_F_NONE : f;
		WG_FieldClick( em.fields[f], em.clickX, em.clickY );
	}
}

static void EM_Label( float x, float y, const char *label ) {
	EM_Text( x, y + 2, label, emDim );
}

typedef void ( *emRowFunc_t )( int row, float x, float y, float w );

// returns the row double-clicked, -1 if none; *clicked gets the row clicked once
static int EM_List( emList_t *list, int count, float x, float y, float w, float h, emRowFunc_t drawRow, int *clicked ) {
	const int visible = (int)( h / EM_ROW_H );
	int row, picked = -1;

	if ( clicked )
		*clicked = -1;
	if ( em.wheel && EM_InRect( x, y, w, h ) ) {
		list->scroll += em.wheel;
		em.wheel = 0;
	}
	EM_ListClamp( list, count, visible );
	if ( count > visible ) {
		EM_Box( x, y, w, h, emPanelLight );
		WG_Scrollbar( x + w - WG_SCROLL_W - 1, y + 1, h - 2, count, visible, &list->scroll, em.cursorX, em.cursorY, &em.click, em.clickX, em.clickY );
		w -= WG_SCROLL_W + 1;
	}

	if ( EM_Clicked( x, y, w, h ) ) {
		row = list->scroll + (int)( ( em.clickY - y ) / EM_ROW_H );
		if ( row < count ) {
			if ( row == list->lastClickRow && cls.realtime - list->lastClickTime < EM_DOUBLECLICK_MS ) {
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

	if ( count <= visible )
		EM_Box( x, y, w, h, emPanelLight );
	for ( int i = 0; i < visible; i++ ) {
		float ry = y + i * EM_ROW_H;

		row = list->scroll + i;
		if ( row >= count )
			break;
		if ( row == list->sel )
			EM_Fill( x + 1, ry, w - 2, EM_ROW_H, emHighlight );
		else if ( EM_InRect( x, ry, w, EM_ROW_H ) )
			EM_Fill( x + 1, ry, w - 2, EM_ROW_H, emHover );
		drawRow( row, x + 4, ry + 1, w - 8 );
	}
	return picked;
}

// the saved constructs, Base and its folders, Custom with its pk3s and theirs indented under them
static void EM_DrawFolderRow( int row, float x, float y, float w ) {
	const emRow_t *r = &em.rows[row];
	const float *color = em.fields[EM_F_SEARCH][0] ? emDim : emWhite;
	const char *slash;
	float indent = 0;

	switch ( r->type ) {
	case EM_ROW_SAVED:
		EM_TextClipped( x, y, w, va( S_COLOR_CYAN "%s " S_COLOR_GREY "(%i)", r->name, r->count ), color );
		return;
	case EM_ROW_FAVES:
		EM_TextClipped( x, y, w, va( S_COLOR_YELLOW "* " S_COLOR_WHITE "%s " S_COLOR_GREY "(%i)", r->name, r->count ), color );
		return;
	case EM_ROW_CATEGORY:
		EM_TextClipped( x, y, w, va( S_COLOR_YELLOW "%s " S_COLOR_GREY "(%i)", r->name, r->count ), color );
		return;
	case EM_ROW_SOURCE:
		// "base/name.pk3" as "name.pk3", a loose folder as itself
		slash = strrchr( r->name, '/' );
		indent = 6;
		EM_TextClipped( x + indent, y, w - indent, strlen( r->name ) > 4 && !Q_stricmp( r->name + strlen( r->name ) - 4, ".pk3" )
			? va( S_COLOR_GREEN "%s " S_COLOR_GREY "(%i)", slash ? slash + 1 : r->name, r->count )
			: va( S_COLOR_GREEN "%s folder " S_COLOR_GREY "(%i)", r->name, r->count ), color );
		return;
	default:
		indent = r->custom ? 12 : 6;
		EM_TextClipped( x + indent, y, w - indent, va( "%s " S_COLOR_GREY "(%i)", r->name[0] ? r->name : "(root)", r->count ), color );
		return;
	}
}

// custom ones in green
static void EM_DrawEffectRow( int row, float x, float y, float w ) {
	const emEffect_t *e;

	if ( EM_ConstructShown() ) {
		EM_TextClipped( x, y, w, va( S_COLOR_CYAN "%s " S_COLOR_GREY "(%i)", em.constructs[em.view[row]], em.constructCounts[em.view[row]] ), emWhite );
		return;
	}
	// a folder lists names, anything wider lists where they are; the star first, click it for a favorite
	e = &em.effects[em.view[row]];
	EM_Text( x, y, CL_Fav_Star( CL_Fav_Has( EM_FAV_LIST, e->path ) ), emWhite );
	EM_TextClipped( x + EM_STAR_W, y, w - EM_STAR_W, em.fields[EM_F_SEARCH][0] || em.rows[em.row].type != EM_ROW_FOLDER ? e->path : e->base,
		e->failed ? emRed : e->custom ? emCustom : emWhite );
}

// "Base, from base/assets1.pk3" or "Custom, from ..."
static const char *EM_SourceLabel( const emEffect_t *e ) {
	return va( "%s, from %s", e->custom ? "Custom" : "Base", em.sources[e->source] );
}

static void EM_DrawCheck( float x, float y, qboolean checked ) {
	EM_Box( x, y + 1, 9, 9, emPanel );
	if ( checked )
		EM_Fill( x + 2, y + 3, 5, 5, emAccent );
}

static void EM_DrawActiveRow( int row, float x, float y, float w ) {
	const emActive_t *a = &em.active[em.activeView[row]];

	EM_DrawCheck( x, y, a->checked );
	EM_Text( x + 14, y, va( S_COLOR_CYAN "%i", a->num ), emWhite );
	EM_TextClipped( x + 44, y, w - 44, va( "%s%s " S_COLOR_GREY "%i u", a->tag[0] ? va( S_COLOR_GREEN "%s " S_COLOR_WHITE, a->tag ) : "",
		a->path, (int)Distance( a->origin, cl.snap.ps.origin ) ), emWhite );
}

static void EM_DrawPickerRow( int row, float x, float y, float w ) {
	const int index = em.pickerView[row];
	int num;
	const char *type, *name;

	switch ( em.picker ) {
	case EM_PICKER_PLAYER:
	case EM_PICKER_REMOVE_PLAYER:
		EM_TextClipped( x, y, w, va( S_COLOR_CYAN "%i " S_COLOR_WHITE "%s%s", index, EM_PlayerName( index ), index == clc.clientNum ? S_COLOR_GREY " (you)" : "" ), emWhite );
		break;
	case EM_PICKER_NPC:
	case EM_PICKER_REMOVE_NPC:
		if ( !CL_NpcManager_GetNpc( index, &num, &type, &name ) )
			break;
		if ( name[0] )
			EM_TextClipped( x, y, w, va( S_COLOR_CYAN "%i " S_COLOR_WHITE "%s " S_COLOR_GREY "%s", num, name, type ), emWhite );
		else
			EM_TextClipped( x, y, w, va( "%i %s  (no targetname, can't be named)", num, type ), emDim );
		break;
	case EM_PICKER_BOLT:
		EM_TextClipped( x, y, w, va( "%s%s", em.bolts[index].name, em.bolts[index].tag ? "" : S_COLOR_GREY "  bone" ), emWhite );
		break;
	case EM_PICKER_REMOVE_NAME:
		EM_TextClipped( x, y, w, va( S_COLOR_GREEN "%s " S_COLOR_GREY "(%i effects)", em.names[index], EM_CountTagged( em.names[index] ) ), emWhite );
		break;
	case EM_PICKER_STATE:
		EM_TextClipped( x, y, w, em.stateFiles[index], emWhite );
		break;
	default:
		break;
	}
}

static void EM_DrawCommandLines( const char *preview ) {
	if ( preview )
		EM_TextClipped( EM_RIGHT_X, EM_BUTTON_Y - 30, EM_RIGHT_W, va( S_COLOR_GREY "%s", preview ), emWhite );
	if ( em.lastCmd[0] ) {
		EM_TextClipped( EM_RIGHT_X, EM_BUTTON_Y - 17, EM_RIGHT_W,
			va( "%sSent: %s", cls.realtime - em.sentTime < 1500 ? S_COLOR_GREEN : S_COLOR_GREY, em.lastCmd ), emWhite );
	} else if ( em.numQueue ) {
		EM_TextClipped( EM_RIGHT_X, EM_BUTTON_Y - 17, EM_RIGHT_W, va( S_COLOR_GREY "%i commands waiting for the server's flood protection", em.numQueue ), emWhite );
	}
}

// the target's model in a box, turning, with the bolt marked on it
static void EM_RenderModel( float x, float y, float w, float h, const char *bolt ) {
	refdef_t refdef;
	refEntity_t ent;
	vec3_t center, forward, angles, p, d;
	float xScale, yScale, fovX, fovY, radius, halfH, dist;
	const float yaw = (float)( cls.realtime % 12000 ) * 0.03f + em.modelYaw;	// a turn every 12 seconds

	if ( !em.g2View )
		return;

	radius = sqrtf( Square( Q_max( fabsf( em.g2Mins[0] ), fabsf( em.g2Maxs[0] ) ) ) + Square( Q_max( fabsf( em.g2Mins[1] ), fabsf( em.g2Maxs[1] ) ) ) );
	halfH = ( em.g2Maxs[2] - em.g2Mins[2] ) * 0.5f;
	VectorSet( center, 0, 0, ( em.g2Mins[2] + em.g2Maxs[2] ) * 0.5f );

	xScale = cls.glconfig.vidWidth / (float)SCREEN_WIDTH;
	yScale = cls.glconfig.vidHeight / (float)SCREEN_HEIGHT;
	memset( &refdef, 0, sizeof( refdef ) );
	refdef.x = (int)( x * xScale );
	refdef.y = (int)( y * yScale );
	refdef.width = (int)( w * xScale );
	refdef.height = (int)( h * yScale );
	fovY = 30.0f;
	fovX = RAD2DEG( 2.0f * atanf( tanf( DEG2RAD( fovY * 0.5f ) ) * refdef.width / (float)refdef.height ) );
	refdef.fov_x = fovX;
	refdef.fov_y = fovY;
	dist = Q_max( halfH / tanf( DEG2RAD( fovY * 0.5f ) ), radius / tanf( DEG2RAD( fovX * 0.5f ) ) ) * 1.1f + radius;

	VectorSet( angles, 8.0f, 180.0f, 0 );
	AngleVectors( angles, forward, NULL, NULL );
	VectorMA( center, -dist, forward, refdef.vieworg );
	AnglesToAxis( angles, refdef.viewaxis );
	refdef.rdflags = RDF_NOWORLDMODEL;
	refdef.time = cl.serverTime;

	memset( &ent, 0, sizeof( ent ) );
	ent.reType = RT_MODEL;
	ent.ghoul2 = em.g2View;
	ent.renderfx = RF_NOSHADOW;
	VectorSet( ent.angles, 0, yaw, 0 );
	AnglesToAxis( ent.angles, ent.axis );
	VectorCopy( em.g2Scale, ent.modelScale );
	ent.radius = radius + halfH;

	re->ClearScene();
	re->AddRefEntityToScene( &ent );
	re->RenderScene( &refdef );

	// the bolt, drawn over the model so the body can't hide it
	if ( bolt && EM_BoltPoint( bolt, yaw, p ) ) {
		float sx, sy, fwd;

		VectorSubtract( p, refdef.vieworg, d );
		fwd = DotProduct( d, refdef.viewaxis[0] );
		if ( fwd > 1.0f ) {
			sx = x + w * 0.5f * ( 1.0f - DotProduct( d, refdef.viewaxis[1] ) / fwd / tanf( DEG2RAD( fovX * 0.5f ) ) );
			sy = y + h * 0.5f * ( 1.0f - DotProduct( d, refdef.viewaxis[2] ) / fwd / tanf( DEG2RAD( fovY * 0.5f ) ) );
			if ( sx > x && sx < x + w && sy > y && sy < y + h ) {
				const float s = ( cls.realtime / 300 ) % 2 ? 5.0f : 4.0f;

				EM_Fill( sx - s - 1, sy - s - 1, s * 2 + 2, s * 2 + 2, emPanel );
				EM_Fill( sx - s, sy - s, s * 2, s * 2, emAccent );
			}
		}
	}
}

// the window: the world with the effect playing in it, or the target with its bolt
static void EM_DrawWindow( void ) {
	const float x = EM_RIGHT_X, y = EM_VIEW_Y, w = EM_RIGHT_W, h = EM_VIEW_H;
	const emEffect_t *e = EM_Effect();

	if ( em.where == EM_WHERE_BOLT ) {
		EM_Box( x, y, w, h, emPanelLight );
		if ( em.wheel && EM_InRect( x, y, w, h ) ) {
			em.modelYaw += em.wheel * 5.0f;	// 15 degrees a notch
			em.wheel = 0;
		}
		if ( em.g2 && em.targetSeen ) {
			EM_RenderModel( x + 1, y + 1, w - 2, h - 2, em.fields[EM_F_BOLT] );
			EM_TextClipped( x + 6, y + h - 16, w - 12, va( S_COLOR_GREY "%s", em.g2Model ), emWhite );
			if ( e )
				EM_TextClipped( x + 6, y + 4, w - 12, va( S_COLOR_GREY "%s", EM_SourceLabel( e ) ), emWhite );
		} else {
			EM_TextClipped( x + 8, y + 8, w - 16, "The target isn't in view.", emRed );
			EM_TextClipped( x + 8, y + 8 + EM_ROW_H, w - 16, "Its model shows here once it is drawn:", emDim );
			EM_TextClipped( x + 8, y + 8 + EM_ROW_H * 2, w - 16, "get it on screen, or use third person for yourself.", emDim );
		}
		return;
	}

	// left clear, the effect plays in the world behind it
	EM_Box( x, y, w, h, NULL );
	if ( em.wheel && EM_InRect( x, y, w, h ) ) {
		em.previewDist = Com_Clamp( 32.0f, 2048.0f, em.previewDist * ( em.wheel > 0 ? 1.25f : 0.8f ) );
		em.wheel = 0;
	}
	if ( !e )
		EM_TextClipped( x + 6, y + 4, w - 12, "Pick an effect on the left", emDim );
	else if ( em.playError )
		EM_TextClipped( x + 6, y + 4, w - 12, em.playError, emRed );
	else
		EM_TextClipped( x + 6, y + 4, w - 12, va( S_COLOR_GREY "%s", EM_SourceLabel( e ) ), emWhite );
	EM_TextClipped( x + 6, y + h - 16, w - 12, va( S_COLOR_GREY "Live, %i units away   Wheel nearer/farther", (int)em.previewDist ), emWhite );
}

// what the looping ones sent are called, and what waits to go together
static void EM_DrawNameAndWaiting( void ) {
	float y = 328;

	EM_Label( EM_RIGHT_X, y, "Name" );
	EM_Field( EM_F_NAME, EM_CTRL_X, y, EM_CTRL_W, "optional, looping effects keep it" );

	y += 20;
	EM_Label( EM_RIGHT_X, y, "Waiting" );
	if ( EM_Button( EM_CTRL_X, y, 86, em.numWaiting ? va( "Send all %i", em.numWaiting ) : "Send all", qfalse, (qboolean)( em.numWaiting > 0 ) ) )
		EM_SendWaiting();
	if ( EM_DangerButton( EM_CTRL_X + 90, y, 50, "Clear", EM_CONFIRM_CLEAR, (qboolean)( em.numWaiting > 0 ) ) )
		em.numWaiting = 0;
	if ( EM_Button( EM_CTRL_X + 144, y, EM_CTRL_W - 144, "Save construct...", qfalse, (qboolean)( em.numWaiting > 0 ) ) )
		EM_StartNaming( EM_NAMING_CONSTRUCT );
}

// a saved construct: what it holds, who its bolted effects go on, and where its placed ones go
static void EM_DrawConstructPanel( int picked ) {
	const float x = EM_RIGHT_X, w = EM_RIGHT_W;
	const int rows = (int)( ( EM_VIEW_H - 8 ) / EM_ROW_H );
	float y = EM_VIEW_Y;
	int placed, bolted;
	qboolean targetOk = (qboolean)( em.target != EM_TARGET_NPC || em.targetName[0] );

	EM_CountCPieces( em.cpieces, em.numCPieces, &placed, &bolted );
	EM_Box( x, y, w, EM_VIEW_H, emPanelLight );
	if ( em.construct < 0 || !em.numConstructs ) {
		EM_TextClipped( x + 6, y + 4, w - 12, "None saved yet. Save one from:", emDim );
		EM_TextClipped( x + 6, y + 4 + EM_ROW_H, w - 12, S_COLOR_GREY "- K in the free camera, selecting (V): the selection", emWhite );
		EM_TextClipped( x + 6, y + 4 + EM_ROW_H * 2, w - 12, S_COLOR_GREY "- Save construct... under Waiting: what waits,", emWhite );
		EM_TextClipped( x + 6, y + 4 + EM_ROW_H * 3, w - 12, S_COLOR_GREY "  bolted ones too, without who they're on", emWhite );
		EM_DrawNameAndWaiting();
		return;
	}
	for ( int i = 0; i < em.numCPieces && i < rows; i++ ) {
		const emCPiece_t *p = &em.cpieces[i];
		const float ry = y + 4 + i * EM_ROW_H;

		if ( i == rows - 1 && em.numCPieces > rows ) {
			EM_Text( x + 6, ry, va( S_COLOR_GREY "and %i more", em.numCPieces - i ), emWhite );
			break;
		}
		EM_TextClipped( x + 6, ry, w - 12, p->bolted
			? va( "%s " S_COLOR_CYAN "on %s%s", p->path, p->bolt, p->loop ? "" : S_COLOR_GREY "  once" )
			: va( "%s " S_COLOR_GREY "at %+i %+i %+i%s", p->path, (int)p->offset[0], (int)p->offset[1], (int)p->offset[2], p->loop ? "" : "  once" ), emWhite );
	}

	y += EM_VIEW_H + 4;
	EM_TextClipped( x, y, w, va( S_COLOR_YELLOW "%s " S_COLOR_GREY "%i effects: %i placed, %i bolted", em.constructs[em.construct], em.numCPieces, placed, bolted ), emWhite );

	// who the bolted ones go on, picked now as for one effect
	y += 16;
	if ( bolted ) {
		EM_Label( x, y, "On" );
		if ( EM_Button( EM_CTRL_X, y, 74, "Me", (qboolean)( em.target == EM_TARGET_ME ), qtrue ) )
			em.target = EM_TARGET_ME;
		if ( EM_Button( EM_CTRL_X + 78, y, 74, "Player...", (qboolean)( em.target == EM_TARGET_PLAYER ), qtrue ) )
			EM_OpenPicker( EM_PICKER_PLAYER );
		if ( EM_Button( EM_CTRL_X + 156, y, 72, "NPC...", (qboolean)( em.target == EM_TARGET_NPC ), qtrue ) )
			EM_OpenPicker( EM_PICKER_NPC );
		y += 18;
		EM_TextClipped( EM_CTRL_X, y, EM_CTRL_W, EM_TargetLabel(), emWhite );
	}
	if ( placed ) {
		y += 20;
		EM_TextClipped( x, y, w, S_COLOR_GREY "The placed ones go where you click in the free camera, the wheel turns them", emWhite );
	}

	EM_DrawNameAndWaiting();
	EM_DrawCommandLines( bolted && !targetOk ? "Pick an NPC with a targetname" : NULL );
	if ( EM_Button( x, EM_BUTTON_Y, 80, "Send  (Enter)", qfalse, (qboolean)( bolted && targetOk ) ) )
		EM_ApplyConstructBolts( qfalse );
	if ( EM_Button( x + 84, EM_BUTTON_Y, 82, "Add to waiting", qfalse, (qboolean)( bolted && targetOk ) ) )
		EM_ApplyConstructBolts( qtrue );
	if ( EM_Button( x + w - 116, EM_BUTTON_Y, 116, "Place in world...", qfalse, (qboolean)( placed > 0 ) ) || ( picked >= 0 && placed ) )
		EM_StartConstructPlace();
}

static void EM_DrawEffects( void ) {
	const emEffect_t *e;
	const char *cmd;
	float y, x;
	int picked, clicked;
	int length = 0, repeatDelay = 0;

	// folders and effects, of the kind the filter bar lets through
	const float searchW = EM_FOLDER_W + EM_EFFECT_W + 4 - EM_FILTER_COUNT * ( EM_FILTER_W + 4 );

	EM_Field( EM_F_SEARCH, EM_FOLDER_X, EM_SEARCH_Y, searchW, "Type to search all effects" );
	for ( int i = 0; i < EM_FILTER_COUNT; i++ ) {
		if ( EM_Button( EM_FOLDER_X + searchW + 4 + i * ( EM_FILTER_W + 4 ), EM_SEARCH_Y, EM_FILTER_W, emFilterNames[i], (qboolean)( em.filter == i ), qtrue ) ) {
			EM_SetFilter( (emFilter_t)i );
			EM_SelectRow( 0 );
		}
	}
	em.folderList.sel = em.fields[EM_F_SEARCH][0] ? -1 : em.row;
	EM_List( &em.folderList, em.numRows, EM_FOLDER_X, EM_LIST_Y, EM_FOLDER_W, EM_LIST_H, EM_DrawFolderRow, &clicked );
	if ( clicked >= 0 && clicked < em.numRows ) {
		EM_SetRow( clicked );
		EM_SelectRow( 0 );
	}
	picked = EM_List( &em.effectList, em.numView, EM_EFFECT_X, EM_LIST_Y, EM_EFFECT_W, EM_LIST_H, EM_DrawEffectRow, &clicked );
	// the star: a favorite, or not any more
	if ( clicked >= 0 && clicked < em.numView && !EM_ConstructShown() && em.clickX < EM_EFFECT_X + 4 + EM_STAR_W ) {
		EM_ToggleFavorite( em.view[clicked] );
		clicked = -1;
		picked = -1;
	}
	if ( clicked >= 0 && clicked < em.numView )
		EM_SelectRow( clicked );
	if ( !em.numView ) {
		EM_Text( EM_EFFECT_X + 4, EM_LIST_Y + 1, EM_ConstructShown() ? "None yet: see the right"
			: !em.fields[EM_F_SEARCH][0] && em.row < em.numRows && em.rows[em.row].type == EM_ROW_FAVES ? "None yet: click the * by an effect"
			: em.numEffects ? "No effects match" : "No effects found", emDim );
	}
	if ( EM_ConstructShown() ) {
		EM_DrawConstructPanel( picked );
		return;
	}
	e = EM_Effect();

	EM_DrawWindow();

	y = EM_VIEW_Y + EM_VIEW_H + 4;
	if ( e ) {
		const char *timing = "";

		if ( e->id && theFxScheduler.GetEffectTiming( e->id, &length, &repeatDelay ) )
			timing = va( S_COLOR_GREY "   lasts %.1f s", length / 1000.0f );
		EM_TextClipped( EM_RIGHT_X, y, EM_RIGHT_W, va( S_COLOR_YELLOW "%s%s", e->path, timing ), emWhite );
	}

	y += 16;
	EM_Label( EM_RIGHT_X, y, "Play" );
	if ( EM_Button( EM_CTRL_X, y, 74, "Once", (qboolean)!em.loop, qtrue ) )
		em.loop = qfalse;
	if ( EM_Button( EM_CTRL_X + 78, y, 74, "Loop (add)", em.loop, qtrue ) )
		em.loop = qtrue;
	// how big, experimental
	if ( EM_Button( EM_CTRL_X + 158, y, 16, "-", qfalse, (qboolean)( em.where != EM_WHERE_BOLT ) ) )
		em.scale = EM_ClampScale( EM_ClampScale( em.scale ) / EM_SCALE_STEP );
	EM_TextClipped( EM_CTRL_X + 176, y + 2, 36, em.where == EM_WHERE_BOLT ? S_COLOR_GREY "size" : va( "%sx%.2g", fabsf( EM_ClampScale( em.scale ) - 1.0f ) > 0.001f ? S_COLOR_CYAN : S_COLOR_GREY, EM_ClampScale( em.scale ) ), emWhite );
	if ( EM_Button( EM_CTRL_X + 212, y, 16, "+", qfalse, (qboolean)( em.where != EM_WHERE_BOLT ) ) )
		em.scale = EM_ClampScale( EM_ClampScale( em.scale ) * EM_SCALE_STEP );

	y += 20;
	EM_Label( EM_RIGHT_X, y, "Where" );
	if ( EM_Button( EM_CTRL_X, y, 74, "At me", (qboolean)( em.where == EM_WHERE_ME ), qtrue ) )
		em.where = EM_WHERE_ME;
	if ( EM_Button( EM_CTRL_X + 78, y, 74, "Position", (qboolean)( em.where == EM_WHERE_POS ), qtrue ) ) {
		em.where = EM_WHERE_POS;
		if ( !em.havePos )
			EM_EnterPlace();
	}
	if ( EM_Button( EM_CTRL_X + 156, y, 72, "Bolted", (qboolean)( em.where == EM_WHERE_BOLT ), qtrue ) )
		em.where = EM_WHERE_BOLT;

	y += 20;
	if ( em.where == EM_WHERE_BOLT ) {
		EM_Label( EM_RIGHT_X, y, "On" );
		if ( EM_Button( EM_CTRL_X, y, 74, "Me", (qboolean)( em.target == EM_TARGET_ME ), qtrue ) )
			em.target = EM_TARGET_ME;
		if ( EM_Button( EM_CTRL_X + 78, y, 74, "Player...", (qboolean)( em.target == EM_TARGET_PLAYER ), qtrue ) )
			EM_OpenPicker( EM_PICKER_PLAYER );
		if ( EM_Button( EM_CTRL_X + 156, y, 72, "NPC...", (qboolean)( em.target == EM_TARGET_NPC ), qtrue ) )
			EM_OpenPicker( EM_PICKER_NPC );
		y += 18;
		EM_TextClipped( EM_CTRL_X, y, EM_CTRL_W, EM_TargetLabel(), emWhite );

		y += 16;
		EM_Label( EM_RIGHT_X, y, "Bolt" );
		EM_Field( EM_F_BOLT, EM_CTRL_X, y, EM_CTRL_W - 104, "*tag or bone" );
		if ( EM_Button( EM_CTRL_X + EM_CTRL_W - 100, y, 100, "Tags & bones...", qfalse, (qboolean)( em.g2 != NULL && em.numBolts > 0 ) ) )
			EM_OpenPicker( EM_PICKER_BOLT );
		y += 18;
		if ( em.g2View && em.fields[EM_F_BOLT][0] && !EM_BoltExists() )
			EM_TextClipped( EM_CTRL_X, y, EM_CTRL_W, va( "%s has no %s", em.g2Model, em.fields[EM_F_BOLT] ), emRed );
	} else {
		if ( em.where == EM_WHERE_POS ) {
			EM_Label( EM_RIGHT_X, y, "At" );
			if ( EM_Button( EM_CTRL_X, y, 110, em.havePos ? "Pick again..." : "Pick in world...", qfalse, (qboolean)( e != NULL ) ) )
				EM_EnterPlace();
			x = EM_CTRL_X + 116;
			EM_TextClipped( x, y + 2, EM_RIGHT_X + EM_RIGHT_W - x, em.havePos
				? va( S_COLOR_GREY "%i %i %i", (int)em.pos[0], (int)em.pos[1], (int)em.pos[2] )
				: S_COLOR_GREY "not picked yet", emWhite );
		} else {
			EM_TextClipped( EM_CTRL_X, y + 2, EM_CTRL_W, S_COLOR_GREY "Where you stand when it is sent", emWhite );
		}

		y += 20;
		EM_Label( EM_RIGHT_X, y, "Angles" );
		EM_Field( EM_F_PITCH, EM_CTRL_X, y, 44, "pitch" );
		EM_Field( EM_F_YAW, EM_CTRL_X + 48, y, 44, "yaw" );
		EM_Field( EM_F_ROLL, EM_CTRL_X + 96, y, 44, "roll" );
		if ( EM_Button( EM_CTRL_X + 144, y, 84, "Straight up", qfalse, EM_HaveAngles() ) )
			em.fields[EM_F_PITCH][0] = em.fields[EM_F_YAW][0] = em.fields[EM_F_ROLL][0] = '\0';
		y += 18;
		EM_TextClipped( EM_CTRL_X, y, EM_CTRL_W, S_COLOR_GREY "Empty or 0 0 0 points it straight up", emWhite );
	}

	EM_DrawNameAndWaiting();

	cmd = EM_BuildCommand();
	EM_DrawCommandLines( cmd ? va( "Will send: %s", cmd ) : em.where == EM_WHERE_BOLT && em.target == EM_TARGET_NPC && !em.targetName[0] ? "Pick an NPC with a targetname" : NULL );
	if ( EM_Button( EM_RIGHT_X, EM_BUTTON_Y, 80, "Send  (Enter)", qfalse, (qboolean)( cmd != NULL ) ) )
		EM_Send();
	if ( EM_Button( EM_RIGHT_X + 84, EM_BUTTON_Y, 82, "Add to waiting", qfalse, (qboolean)( cmd != NULL ) ) )
		EM_AddWaiting();
	if ( EM_Button( EM_RIGHT_X + EM_RIGHT_W - 116, EM_BUTTON_Y, 116, em.where == EM_WHERE_BOLT ? "View in world..." : "Place in world...", qfalse, (qboolean)( e != NULL ) )
		|| ( picked >= 0 && e ) )
		EM_EnterPlace();
}

static void EM_DrawActive( void ) {
	const emActive_t *a = NULL;
	float y, w;
	int checked = EM_CountChecked(), clicked;

	EM_Field( EM_F_ACTIVE_SEARCH, EM_FOLDER_X, EM_SEARCH_Y, EM_ACTIVE_W, "Type to filter by number or effect" );
	EM_List( &em.activeList, em.numActiveView, EM_FOLDER_X, EM_LIST_Y, EM_ACTIVE_W, EM_LIST_H, EM_DrawActiveRow, &clicked );
	if ( clicked >= 0 && clicked < em.numActiveView )
		em.active[em.activeView[clicked]].checked = (qboolean)!em.active[em.activeView[clicked]].checked;
	if ( !em.numActiveView ) {
		EM_Text( EM_FOLDER_X + 4, EM_LIST_Y + 1,
			em.listRequestTime || em.listQueued ? "Asking the server..." : em.numActive ? "No effects match" : "No effects playing", emDim );
	}
	if ( em.activeList.sel < em.numActiveView )
		a = &em.active[em.activeView[em.activeList.sel]];

	const float rx = EM_FOLDER_X + EM_ACTIVE_W + 10, rw = SCREEN_WIDTH - 16 - rx, cx = rx + 66;

	y = EM_SEARCH_Y;
	if ( EM_Button( rx, y, 58, "Refresh", qfalse, qtrue ) )
		EM_RequestList();
	if ( EM_Button( rx + 62, y, 92, "Check on map...", qfalse, (qboolean)( em.numActive > 0 ) ) ) {
		em.state = EM_PICK;
		em.focus = EM_F_NONE;
	}
	if ( EM_Button( rx + 158, y, 86, "Edit on map...", qfalse, qtrue ) )
		EM_EnterSelect();
	if ( em.listTime )
		EM_TextClipped( rx + 248, y + 2, rw - 248, va( S_COLOR_GREY "%i, %is", em.numActive, ( cls.realtime - em.listTime ) / 1000 ), emWhite );

	y = EM_LIST_Y;
	EM_Box( rx, y, rw, 34, emPanelLight );
	if ( a ) {
		EM_TextClipped( rx + 6, y + 4, rw - 12, va( S_COLOR_CYAN "%i  " S_COLOR_YELLOW "%s", a->num, a->path ), emWhite );
		EM_TextClipped( rx + 6, y + 18, rw - 12, va( S_COLOR_GREY "at %i %i %i, %i units from you",
			(int)a->origin[0], (int)a->origin[1], (int)a->origin[2], (int)Distance( a->origin, cl.snap.ps.origin ) ), emWhite );
	} else {
		EM_Text( rx + 6, y + 4, "Click effects to check them", emDim );
	}

	w = ( rw - 66 - 8 ) / 3.0f;
	y += 42;
	EM_Label( rx, y, "Check" );
	if ( EM_Button( cx, y, w, "All", qfalse, (qboolean)( em.numActiveView > 0 ) ) ) {
		for ( int i = 0; i < em.numActiveView; i++ )
			em.active[em.activeView[i]].checked = qtrue;
	}
	if ( EM_Button( cx + w + 4, y, w, "None", qfalse, (qboolean)( checked > 0 ) ) ) {
		for ( int i = 0; i < em.numActive; i++ )
			em.active[i].checked = qfalse;
	}
	if ( EM_Button( cx + ( w + 4 ) * 2, y, w, "Invert", qfalse, (qboolean)( em.numActiveView > 0 ) ) ) {
		for ( int i = 0; i < em.numActiveView; i++ )
			em.active[em.activeView[i]].checked = (qboolean)!em.active[em.activeView[i]].checked;
	}

	// how big the checked ones are drawn, or the one picked; experimental, shared
	y += 24;
	EM_Label( rx, y, "Size" );
	{
		const float shown = a ? EM_ScaleAt( a->path, a->origin ) : 1.0f;
		int dir = 0;

		if ( EM_Button( cx, y, 20, "-", qfalse, (qboolean)( checked > 0 || a ) ) )
			dir = -1;
		EM_TextClipped( cx + 24, y + 2, w * 2 - 28, va( "x%.2g  " S_COLOR_GREY "%s", shown, checked ? va( "the %i checked", checked ) : a ? "this one" : "check some" ), emWhite );
		if ( EM_Button( cx + w * 2, y, 20, "+", qfalse, (qboolean)( checked > 0 || a ) ) )
			dir = 1;
		if ( EM_Button( cx + w * 2 + 24, y, w - 20, "x1", qfalse, (qboolean)( checked > 0 || a ) ) )
			dir = 2;
		for ( int i = 0; dir && i < em.numActive; i++ ) {
			emActive_t *t = &em.active[i];
			float now;

			if ( checked ? !t->checked : t != a )
				continue;
			now = EM_ScaleAt( t->path, t->origin );
			EM_SetScale( t->path, t->origin, dir == 2 ? 1.0f : now * ( dir > 0 ? EM_SCALE_STEP : 1.0f / EM_SCALE_STEP ), qfalse );
		}
	}

	y += 24;
	EM_Label( rx, y, "Remove" );
	if ( EM_DangerButton( cx, y, rw - 66, checked ? va( "Remove the %i checked", checked ) : "Remove the checked", EM_CONFIRM_REMOVE, (qboolean)( checked > 0 ) ) )
		EM_RemoveChecked();
	y += 20;
	if ( EM_DangerButton( cx, y, rw - 66, "Remove every effect", EM_CONFIRM_REMOVE_ALL, qtrue ) )
		EM_SendAndRefresh( "rpeffect remove all" );

	y += 26;
	EM_Label( rx, y, "Bolted on" );
	if ( EM_Button( cx, y, w, "Me", qfalse, qtrue ) )
		EM_SendAndRefresh( "rpeffect remove bolted" );
	if ( EM_Button( cx + w + 4, y, w, "Player...", qfalse, qtrue ) )
		EM_OpenPicker( EM_PICKER_REMOVE_PLAYER );
	if ( EM_Button( cx + ( w + 4 ) * 2, y, w, "NPC...", qfalse, qtrue ) )
		EM_OpenPicker( EM_PICKER_REMOVE_NPC );
	y += 18;
	EM_TextClipped( cx, y, rw - 66, S_COLOR_GREY "removes the effects bolted to them", emWhite );

	// names, and states that remember them through rpdump
	EM_CollectNames();
	y += 26;
	EM_Label( rx, y, "Named" );
	if ( EM_Button( cx, y, w * 2 + 4, "Remove by name...", qfalse, (qboolean)( em.numNames > 0 ) ) )
		EM_OpenPicker( EM_PICKER_REMOVE_NAME );
	EM_TextClipped( cx + ( w + 4 ) * 2, y + 2, w, va( S_COLOR_GREY "%i name%s", em.numNames, em.numNames == 1 ? "" : "s" ), emWhite );
	y += 20;
	EM_Label( rx, y, "State" );
	if ( EM_Button( cx, y, w * 1.5f + 2, "Save...", qfalse, (qboolean)( EM_StateCount() > 0 ) ) )
		EM_StartNaming( EM_NAMING_STATE );
	if ( EM_Button( cx + w * 1.5f + 6, y, w * 1.5f + 2, "Reload...", qfalse, qtrue ) )
		EM_OpenPicker( EM_PICKER_STATE );
	y += 18;
	EM_TextClipped( cx, y, rw - 66, S_COLOR_GREY "Save before rpdump, reload after: names come back", emWhite );

	if ( em.lastCmd[0] ) {
		EM_TextClipped( rx, EM_BUTTON_Y - 17, rw,
			va( "%sSent: %s", cls.realtime - em.sentTime < 1500 ? S_COLOR_GREEN : S_COLOR_GREY, em.lastCmd ), emWhite );
	}
	if ( em.numQueue )
		EM_TextClipped( rx, EM_BUTTON_Y, rw, va( S_COLOR_GREY "%i commands waiting for the server's flood protection", em.numQueue ), emWhite );
}

static void EM_DrawPresets( void ) {
	const float x = EM_FOLDER_X, cx = x + 90;
	float y = EM_SEARCH_Y + 4;

	EM_Text( x, y, S_COLOR_GREY "RPMod's ready-made effects, for everyone on the server.", emWhite );

	y += 24;
	EM_Label( x, y, "Alarm" );
	if ( EM_Button( cx, y, 100, "Start", qfalse, qtrue ) )
		EM_Queue( "rpeffect alarm", qtrue );
	if ( EM_Button( cx + 104, y, 100, "Stop", qfalse, qtrue ) )
		EM_Queue( "rpeffect remove alarm", qtrue );
	EM_Text( cx + 214, y + 2, S_COLOR_GREY "full screen red alert, with two background sounds", emWhite );

	y += 22;
	EM_Label( x, y, "Fade out" );
	if ( EM_Button( cx, y, 100, "Fade out", qfalse, qtrue ) )
		EM_Queue( "rpeffect fadeout", qtrue );
	if ( EM_Button( cx + 104, y, 100, "Fade back in", qfalse, qtrue ) )
		EM_Queue( "rpeffect remove fadeout", qtrue );
	EM_Text( cx + 214, y + 2, S_COLOR_GREY "the screen fades to black until it is removed", emWhite );

	y += 22;
	EM_Label( x, y, "Camera shake" );
	EM_Field( EM_F_INTENSITY, cx, y, 60, "intensity" );
	EM_Field( EM_F_DURATION, cx + 64, y, 60, "seconds" );
	if ( EM_Button( cx + 128, y, 76, "Shake", qfalse, (qboolean)( em.fields[EM_F_INTENSITY][0] && em.fields[EM_F_DURATION][0] ) ) )
		EM_Queue( va( "rpeffect camerashake %s %s", em.fields[EM_F_INTENSITY], em.fields[EM_F_DURATION] ), qtrue );
	EM_Text( cx + 214, y + 2, S_COLOR_GREY "intensity, then how long in seconds", emWhite );

	if ( em.lastCmd[0] ) {
		EM_TextClipped( x, y + 34, SCREEN_WIDTH - 32,
			va( "%sSent: %s", cls.realtime - em.sentTime < 1500 ? S_COLOR_GREEN : S_COLOR_GREY, em.lastCmd ), emWhite );
	}
}

// the bolt highlighted in the bolt picker, on the target's model
static void EM_DrawPickerModel( float x, float y, float w, float h ) {
	const char *bolt = em.pickerList.sel < em.numPickerView ? em.bolts[em.pickerView[em.pickerList.sel]].name : NULL;

	EM_Box( x, y, w, h, emPanelLight );
	if ( em.wheel && EM_InRect( x, y, w, h ) ) {
		em.modelYaw += em.wheel * 5.0f;
		em.wheel = 0;
	}
	if ( em.g2 )
		EM_RenderModel( x + 1, y + 1, w - 2, h - 2, bolt );
	EM_TextClipped( x + 6, y + h - 16, w - 12, va( S_COLOR_GREY "%s", em.g2Model ), emWhite );
}

static void EM_DrawPicker( void ) {
	static const char *titles[] = { "", "Bolt to which player?", "Bolt to which NPC?", "Pick a tag or bone",
		"Remove the effects bolted to which player?", "Remove the effects bolted to which NPC?",
		"Remove the effects with which name?", "Reload which state?" };
	const qboolean bolts = (qboolean)( em.picker == EM_PICKER_BOLT );
	const float x = bolts ? EM_PICKER_BOLT_X : EM_PICKER_X, y = EM_PICKER_Y, h = EM_PICKER_H;
	const float w = EM_PICKER_W + ( bolts ? EM_PICKER_PREVIEW_W : 0.0f ), lw = EM_PICKER_W - 16;
	int picked;

	EM_Box( x, y, w, h, emPanel );
	EM_Fill( x + 1, y + 1, w - 2, h - 2, emPanel );
	EM_Text( x + 8, y + 6, titles[em.picker], emWhite );
	EM_Field( EM_F_PICKER_SEARCH, x + 8, y + 22, lw, "Type to search" );
	if ( !bolts )
		EM_RebuildPickerView();	// people and NPCs come and go
	picked = EM_List( &em.pickerList, em.numPickerView, x + 8, y + 42, lw, h - 74, EM_DrawPickerRow, NULL );
	if ( !em.numPickerView ) {
		EM_Text( x + 12, y + 43, ( em.picker == EM_PICKER_NPC || em.picker == EM_PICKER_REMOVE_NPC ) && CL_NpcManager_Waiting()
			? "Asking the server..." : "Nothing to pick", emDim );
	}
	if ( bolts )
		EM_DrawPickerModel( x + lw + 16, y + 22, EM_PICKER_PREVIEW_W - 8, h - 54 );
	if ( EM_Button( x + 8, y + h - 24, 100, "Choose  (Enter)", qfalse, (qboolean)( em.numPickerView > 0 ) ) )
		picked = em.pickerList.sel;
	if ( EM_Button( x + w - 108, y + h - 24, 100, "Cancel  (Esc)", qfalse, qtrue ) ) {
		em.picker = EM_PICKER_NONE;
		return;
	}
	if ( picked >= 0 && picked < em.numPickerView )
		EM_ChoosePicker( em.pickerView[picked] );
	else if ( EM_Clicked( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT ) && !EM_InRect( x, y, w, h ) )
		em.picker = EM_PICKER_NONE;
}

// typing a name, over whatever shows
static void EM_DrawNaming( void ) {
	const float bw = 360, bx = ( SCREEN_WIDTH - bw ) * 0.5f, by = SCREEN_HEIGHT * 0.5f + 30;
	const qboolean state = (qboolean)( em.naming == EM_NAMING_STATE );
	const char *field = em.fields[EM_ActiveField()];
	const char *title, *hint, *note = NULL;

	if ( em.naming == EM_NAMING_CONSTRUCT ) {
		title = em.constructFromSelection ? va( "Save the %i selected effects as a construct named:", EM_NumSelected() )
			: va( "Save the %i effects waiting as a construct named:", em.numWaiting );
		hint = S_COLOR_GREY "Enter saves, Esc cancels; bolted ones go on whoever is picked when it's used";
		if ( field[0] && FS_FileExists( va( "%s/%s%s", EM_CONSTRUCT_DIR, field, EM_SAVE_EXT ) ) )
			note = S_COLOR_YELLOW "A construct with this name is there already; Enter replaces it";
	} else if ( state ) {
		title = va( "Save the state of %i effects sent or named as:", EM_StateCount() );
		hint = S_COLOR_GREY "Enter saves, Esc cancels";
		if ( field[0] && FS_FileExists( va( "%s/%s%s", EM_STATE_DIR, field, EM_SAVE_EXT ) ) )
			note = S_COLOR_YELLOW "A state with this name is there already; Enter replaces it";
	} else {
		const int others = field[0] ? EM_CountTagged( field ) : 0;

		if ( em.state == EM_PLACE && em.selecting ) {
			title = va( "Name the %i selected effects:", EM_NumSelected() );
			hint = S_COLOR_GREY "Enter names them; empty takes the name off; Esc cancels";
		} else {
			title = em.numWaiting ? va( "Name the %i effects waiting, then send them:", em.numWaiting ) : "Name the effect, then send it:";
			hint = S_COLOR_GREY "Enter names and sends; empty sends with no name; Esc cancels";
		}
		if ( others )
			note = va( S_COLOR_YELLOW "%i effects on the map are named %s; these join them", others, field );
	}

	EM_Box( bx, by, bw, note ? 58 : 46, emPanelOpaque );
	EM_TextClipped( bx + 8, by + 4, bw - 16, title, emWhite );
	wgStyle_t style;

	EM_FieldStyle( &style );
	EM_Box( bx + 8, by + 18, bw - 16, 16, emPanelLight );
	WG_Field( &style, bx + 8, by + 18, bw - 16, 16, em.fields[EM_ActiveField()], qtrue, "", em.cursorX, em.cursorY );
	EM_TextClipped( bx + 8, by + 36, bw - 16, hint, emWhite );
	if ( note )
		EM_TextClipped( bx + 8, by + 48, bw - 16, note, emWhite );
}

static void EM_DrawBrowse( void ) {
	char buf[MAX_STRING_CHARS];
	const qboolean window = (qboolean)( em.tab == EM_TAB_EFFECTS && em.where != EM_WHERE_BOLT );

	// the picker sits on top and gets the clicks first, but is drawn last
	qboolean pickerOpen = (qboolean)( em.picker != EM_PICKER_NONE );
	qboolean click = em.click;
	int wheel = em.wheel;
	if ( pickerOpen )
		em.click = qfalse, em.wheel = 0;

	// dim everything but the preview window, where the world shows through
	if ( window ) {
		EM_Fill( 0, 0, SCREEN_WIDTH, EM_VIEW_Y, emPanel );
		EM_Fill( 0, EM_VIEW_Y + EM_VIEW_H, SCREEN_WIDTH, SCREEN_HEIGHT - EM_VIEW_Y - EM_VIEW_H, emPanel );
		EM_Fill( 0, EM_VIEW_Y, EM_RIGHT_X, EM_VIEW_H, emPanel );
		EM_Fill( EM_RIGHT_X + EM_RIGHT_W, EM_VIEW_Y, SCREEN_WIDTH - EM_RIGHT_X - EM_RIGHT_W, EM_VIEW_H, emPanel );
	} else {
		EM_Fill( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, emPanel );
	}

	EM_Text( EM_FOLDER_X, 14, "Effect Manager", emWhite );
	if ( EM_Button( 112, 12, 64, "Effects", (qboolean)( em.tab == EM_TAB_EFFECTS ), qtrue ) )
		EM_SetTab( EM_TAB_EFFECTS );
	if ( EM_Button( 180, 12, 64, "Active", (qboolean)( em.tab == EM_TAB_ACTIVE ), qtrue ) )
		EM_SetTab( EM_TAB_ACTIVE );
	if ( EM_Button( 248, 12, 64, "Presets", (qboolean)( em.tab == EM_TAB_PRESETS ), qtrue ) )
		EM_SetTab( EM_TAB_PRESETS );

	// the server's answer to the last command
	if ( em.reply[0] && cls.realtime - em.replyTime < 8000 )
		EM_TextClipped( EM_RIGHT_X, 14, EM_RIGHT_W, va( S_COLOR_YELLOW "Server: " S_COLOR_WHITE "%s", em.reply ), emWhite );
	// and the manager's own
	if ( em.message[0] && cls.realtime - em.messageTime < EM_MESSAGE_MS )
		EM_TextClipped( EM_FOLDER_X, 29, SCREEN_WIDTH - EM_FOLDER_X * 2, em.message, emWhite );

	switch ( em.tab ) {
	case EM_TAB_EFFECTS:	EM_DrawEffects();	break;
	case EM_TAB_ACTIVE:		EM_DrawActive();	break;
	default:				EM_DrawPresets();	break;
	}

	if ( em.tab == EM_TAB_EFFECTS )
		EM_Text( EM_FOLDER_X, 440, S_COLOR_GREY "Up/Down effect   Left/Right folder   Enter send   Double-click place/view in world   Tab switch tab", emWhite );
	else
		EM_Text( EM_FOLDER_X, 440, S_COLOR_GREY "Up/Down select   Wheel scroll   Tab switch tab   Click a box to type in it", emWhite );
	Com_sprintf( buf, sizeof( buf ), S_COLOR_GREY "Esc back / close   %s",
		Key_GetKey( EM_TOGGLE_CMD ) >= 0 ? "Your effectmanager bind closes" : "Tip: bind a key to effectmanager" );
	EM_Text( EM_FOLDER_X, 452, buf, emWhite );

	if ( pickerOpen && em.picker != EM_PICKER_NONE ) {
		em.click = click;
		em.wheel = wheel;
		EM_DrawPicker();
	}
	if ( em.naming != EM_NAMING_NONE )
		EM_DrawNaming();

	// a click on nothing leaves the text box
	if ( em.click )
		em.focus = EM_F_NONE;
	em.click = qfalse;
	em.wheel = 0;

	WG_DrawOverlay();
	re->DrawStretchPic( em.cursorX, em.cursorY, 32, 32, 0, 0, 1, 1, cls.cursorShader );
}

// what a handle is, for the panel
static const char *EM_HandleLabel( int h ) {
	emHandle_t hd;

	if ( !EM_GetHandle( h, &hd ) )
		return "";
	if ( !hd.active ) {
		return va( "%s%s  " S_COLOR_GREY "(waiting)", em.waiting[h].path,
			em.waiting[h].tag[0] ? va( S_COLOR_GREEN "  %s" S_COLOR_WHITE, em.waiting[h].tag ) : "" );
	}
	return va( S_COLOR_CYAN "%i " S_COLOR_WHITE "%s%s%s", hd.active->num, hd.active->path,
		hd.active->tag[0] ? va( S_COLOR_GREEN "  %s" S_COLOR_WHITE, hd.active->tag ) : "",
		hd.active->deleted ? S_COLOR_RED "  removed" : EM_ActiveEdited( hd.active ) ? S_COLOR_MAGENTA "  moved" : "" );
}

static void EM_DrawSelect( void ) {
	static const char *help[] = {
		"LMB            select (Shift adds), or put down",
		"RMB / F        pick up; lock or follow",
		"Wheel Z X Q E  turn: yaw, pitch, roll",
		"R              straight up (no angles)",
		"Arrows PgUp/Dn nudge",
		"Del            remove",
		"U              undo (selection, or all)",
		"Enter          send all changes",
		"Shift+Enter    name the selection",
		"K              save the selection as a construct",
		"I              send on putting down / on Enter",
		"L              read the map's effects again",
		"G              cycle snap",
		"V              back to putting effects down",
		"WASD Space C   fly (Shift fast, Ctrl slow)",
		"Tab / Esc      cancel a move, manager",
		"H              hide this help",
	};
	const float x = 10, w = 280, cx = SCREEN_WIDTH * 0.5f, cy = SCREEN_HEIGHT * 0.5f;
	const int count = EM_NumSelected();
	int moved, removed, lines, first = -1;
	float y = 10;
	const char *status;
	emHandle_t hd;

	// crosshair
	EM_Fill( cx - 6, cy - 0.5f, 12, 1, emWhite );
	EM_Fill( cx - 0.5f, cy - 6, 1, 12, emWhite );

	for ( int h = EM_FirstHandle(); h < EM_EndHandle() && first < 0; h = EM_NextHandle( h ) ) {
		if ( EM_GetHandle( h, &hd ) && *hd.selected )
			first = h;
	}
	if ( em.carrying )
		status = va( S_COLOR_MAGENTA "MOVING %s", em.carryLocked ? S_COLOR_YELLOW "LOCKED" : S_COLOR_GREEN "FOLLOWING" );
	else if ( count )
		status = va( S_COLOR_CYAN "%i selected", count );
	else
		status = S_COLOR_GREY "nothing selected";

	EM_PendingChanges( &moved, &removed );
	lines = 4 + ( moved + removed + em.numWaiting ? 1 : 0 ) + ( count == 1 ? 1 : 0 );
	EM_Box( x, y, w, lines * EM_ROW_H + 8, emPanel );
	y += 4;
	EM_Text( x + 6, y, va( "Effect Manager  " S_COLOR_WHITE "SELECT  %s", status ), emWhite ); y += EM_ROW_H;
	EM_TextClipped( x + 6, y, w - 12, em.instant ? "Sends each move as it's put down  " S_COLOR_GREY "(I)"
		: S_COLOR_YELLOW "Sends the changes on Enter  " S_COLOR_GREY "(I)", emWhite ); y += EM_ROW_H;
	if ( moved + removed + em.numWaiting ) {
		EM_TextClipped( x + 6, y, w - 12, va( S_COLOR_YELLOW "Not sent: %i moved, %i removed, %i new", moved, removed, em.numWaiting ), emWhite );
		y += EM_ROW_H;
	}
	if ( first >= 0 ) {
		EM_TextClipped( x + 6, y, w - 12, count == 1 ? EM_HandleLabel( first ) : va( "%i effects", count ), emAccent );
		y += EM_ROW_H;
		if ( count == 1 && EM_GetHandle( first, &hd ) ) {
			EM_TextClipped( x + 6, y, w - 12, va( "At %i %i %i, %s", (int)floorf( hd.pos[0] + 0.5f ), (int)floorf( hd.pos[1] + 0.5f ), (int)floorf( hd.pos[2] + 0.5f ),
				*hd.haveAngles ? va( "angles %i %i %i", EM_NormalizeAngle( hd.angles[PITCH] ), EM_NormalizeAngle( hd.angles[YAW] ), EM_NormalizeAngle( hd.angles[ROLL] ) )
				: hd.active && !hd.active->placed ? S_COLOR_GREY "angles unknown, goes back straight up" : "straight up" ), emWhite );
			y += EM_ROW_H;
		}
	} else {
		EM_TextClipped( x + 6, y, w - 12, S_COLOR_GREY "Aim at an effect's marker and click it", emWhite );
		y += EM_ROW_H;
	}
	EM_Text( x + 6, y, va( "Snap    %i deg / %i units", emSnaps[em.snap].angle, emSnaps[em.snap].grid ), emWhite ); y += EM_ROW_H;
	EM_Text( x + 6, y, va( S_COLOR_GREY "%i on the map, %i waiting", em.numActive, em.numWaiting ), emWhite ); y += EM_ROW_H;

	if ( !em.hideHelp ) {
		y += 8;
		EM_Box( x, y, w, ARRAY_LEN( help ) * EM_ROW_H + 8, emPanel );
		y += 4;
		for ( size_t i = 0; i < ARRAY_LEN( help ); i++, y += EM_ROW_H )
			EM_Text( x + 6, y, help[i], emDim );
	}

	// what is under the crosshair, and what happened last
	EM_Box( 10, SCREEN_HEIGHT - 40, SCREEN_WIDTH - 20, 30, emPanel );
	if ( em.hover >= 0 )
		EM_TextClipped( 16, SCREEN_HEIGHT - 37, SCREEN_WIDTH - 32, EM_HandleLabel( em.hover ), emWhite );
	else if ( em.numQueue )
		EM_TextClipped( 16, SCREEN_HEIGHT - 37, SCREEN_WIDTH - 32, va( S_COLOR_YELLOW "Sending, %i commands to go (the server takes about one a second)", em.numQueue ), emWhite );
	if ( em.message[0] && cls.realtime - em.messageTime < EM_MESSAGE_MS ) {
		EM_TextClipped( 16, SCREEN_HEIGHT - 24, SCREEN_WIDTH - 32, em.message, emWhite );
	} else if ( em.lastCmd[0] ) {
		EM_TextClipped( 16, SCREEN_HEIGHT - 24, SCREEN_WIDTH - 32,
			va( "%sSent: %s", cls.realtime - em.sentTime < 1500 ? S_COLOR_GREEN : S_COLOR_GREY, em.lastCmd ), emWhite );
	}

	if ( em.naming != EM_NAMING_NONE )
		EM_DrawNaming();
}

static void EM_DrawPlace( void ) {
	static const char *help[] = {
		"LMB            send, or put down to wait (I)",
		"Enter          send what waits, or this one",
		"Shift+Enter    name it, then send",
		"U              take back the last put down",
		"RMB / F        lock or follow crosshair",
		"Wheel          yaw",
		"Z / X          pitch",
		"Q / E          roll",
		"R              straight up (no angles)",
		"N              face away from surfaces",
		"M              once / loop",
		"G              cycle snap",
		"Arrows PgUp/Dn nudge (locks)",
		"V              select and move effects",
		"[ ] or - +     size (experimental, shared)",
		"J              move arrows (drag X Y Z; RMB looks)",
		"WASD Space C   fly (Shift fast, Ctrl slow)",
		"Tab / Esc      back to the manager",
		"H              hide this help",
	};
	static const char *boltHelp[] = {
		"LMB            send, or keep to wait (I)",
		"Enter          send what waits, or this one",
		"Shift+Enter    name it, then send",
		"U              take back the last kept",
		"M              once / loop",
		"V              select and move effects",
		"WASD Space C   fly (Shift fast, Ctrl slow)",
		"Tab / Esc      back to the manager",
		"H              hide this help",
	};
	const qboolean bolted = (qboolean)( em.where == EM_WHERE_BOLT );

	if ( em.selecting ) {
		EM_DrawSelect();
		return;
	}
	const float x = 10, w = 260, cx = SCREEN_WIDTH * 0.5f, cy = SCREEN_HEIGHT * 0.5f;
	const emEffect_t *e = EM_Effect();
	const char *cmd = em.handConstruct ? NULL : EM_BuildCommand();
	float y = 10;
	vec3_t angles;
	int lines = ( bolted ? 5 : 6 ) + 2;

	if ( !bolted ) {
		EM_Fill( cx - 6, cy - 0.5f, 12, 1, emWhite );
		EM_Fill( cx - 0.5f, cy - 6, 1, 12, emWhite );
	}

	EM_Box( x, y, w, lines * EM_ROW_H + 8, emPanel );
	y += 4;
	if ( bolted )
		EM_Text( x + 6, y, va( "Effect Manager  " S_COLOR_CYAN "BOLTED  %s", em.loop ? "loop" : "once" ), emWhite );
	else
		EM_Text( x + 6, y, va( "Effect Manager  %s  " S_COLOR_WHITE "%s", em.locked ? S_COLOR_YELLOW "LOCKED" : S_COLOR_GREEN "FOLLOWING", em.loop ? "loop" : "once" ), emWhite );
	y += EM_ROW_H;
	EM_TextClipped( x + 6, y, w - 12, em.instant ? "A click sends it right away  " S_COLOR_GREY "(I)"
		: S_COLOR_YELLOW "A click keeps it, Enter sends  " S_COLOR_GREY "(I)", emWhite ); y += EM_ROW_H;
	EM_TextClipped( x + 6, y, w - 12, em.numWaiting ? va( S_COLOR_GREEN "%i waiting to be sent", em.numWaiting ) : S_COLOR_GREY "none waiting", emWhite ); y += EM_ROW_H;
	if ( em.handConstruct )
		EM_TextClipped( x + 6, y, w - 12, va( "construct %s, %i effects, turned %i", em.handName, em.numHand, em.handYaw ), emAccent );
	else
		EM_TextClipped( x + 6, y, w - 12, e ? va( "%s%s", e->path, em.fields[EM_F_NAME][0] ? va( S_COLOR_GREEN "  %s", em.fields[EM_F_NAME] ) : "" ) : "", emAccent );
	y += EM_ROW_H;
	if ( bolted ) {
		EM_TextClipped( x + 6, y, w - 12, va( "On   %s", EM_TargetLabel() ), emWhite ); y += EM_ROW_H;
		EM_TextClipped( x + 6, y, w - 12, va( "Bolt %s", em.fields[EM_F_BOLT] ), emWhite ); y += EM_ROW_H;
		EM_TextClipped( x + 6, y, w - 12, em.playError ? va( S_COLOR_RED "%s", em.playError ) : va( S_COLOR_GREY "%s", em.g2Model ), emWhite ); y += EM_ROW_H;
	} else {
		EM_GetAngles( angles );
		EM_Text( x + 6, y, va( "Origin  %i %i %i", (int)floorf( em.origin[0] + 0.5f ), (int)floorf( em.origin[1] + 0.5f ), (int)floorf( em.origin[2] + 0.5f ) ), emWhite ); y += EM_ROW_H;
		EM_Text( x + 6, y, EM_HaveAngles() ? va( "Angles  %i %i %i%s", EM_NormalizeAngle( angles[PITCH] ), EM_NormalizeAngle( angles[YAW] ), EM_NormalizeAngle( angles[ROLL] ), em.align ? S_COLOR_GREY "  (surface)" : "" )
			: "Angles  straight up", emWhite ); y += EM_ROW_H;
		EM_Text( x + 6, y, va( "Snap    %i deg / %i units%s", emSnaps[em.snap].angle, emSnaps[em.snap].grid,
			fabsf( EM_ClampScale( em.scale ) - 1.0f ) > 0.001f ? va( S_COLOR_CYAN "   size x%.2g", EM_ClampScale( em.scale ) ) : "" ), emWhite ); y += EM_ROW_H;
		if ( em.playError )
			EM_TextClipped( x + 6, y, w - 12, em.playError, emRed );
		y += EM_ROW_H;
	}

	if ( !em.hideHelp ) {
		const char **lines2 = bolted ? boltHelp : help;
		const int count = bolted ? (int)ARRAY_LEN( boltHelp ) : (int)ARRAY_LEN( help );

		y += 8;
		EM_Box( x, y, w, count * EM_ROW_H + 8, emPanel );
		y += 4;
		for ( int i = 0; i < count; i++, y += EM_ROW_H )
			EM_Text( x + 6, y, lines2[i], emDim );
	}

	// what will be sent, and what was sent last
	EM_Box( 10, SCREEN_HEIGHT - 40, SCREEN_WIDTH - 20, 30, emPanel );
	EM_TextClipped( 16, SCREEN_HEIGHT - 37, SCREEN_WIDTH - 32, cmd ? cmd : em.handConstruct
		? va( S_COLOR_GREY "A click puts construct %s down, the wheel turns it, R straightens it", em.handName ) : S_COLOR_GREY "Nothing to send yet", emWhite );
	if ( em.message[0] && cls.realtime - em.messageTime < EM_MESSAGE_MS ) {
		EM_TextClipped( 16, SCREEN_HEIGHT - 24, SCREEN_WIDTH - 32, em.message, emWhite );
	} else if ( em.lastCmd[0] ) {
		EM_TextClipped( 16, SCREEN_HEIGHT - 24, SCREEN_WIDTH - 32,
			va( "%sSent: %s%s", cls.realtime - em.sentTime < 1500 ? S_COLOR_GREEN : S_COLOR_GREY, em.lastCmd,
				em.numQueue ? va( S_COLOR_YELLOW "   %i more to go", em.numQueue ) : "" ), emWhite );
	}

	if ( em.naming != EM_NAMING_NONE ) {
		EM_DrawNaming();
		WG_DrawOverlay();
	}
}

// numbers over the active effects in view, checked ones highlighted
static void EM_DrawPick( void ) {
	float x, y, tw;

	for ( int i = 0; i < em.numActive; i++ ) {
		const emActive_t *a = &em.active[i];
		const char *label = va( "%i %s", a->num, a->path );

		if ( !EM_Project( a->origin, &x, &y ) || x < 0 || x > SCREEN_WIDTH || y < 0 || y > SCREEN_HEIGHT )
			continue;
		tw = EM_TextWidth( label );
		EM_Fill( x - 2, y - 2, 4, 4, a->checked ? emAccent : emWhite );
		EM_Box( x - tw * 0.5f - 3, y - 20, tw + 6, 14, a->checked ? emHighlight : emPanel );
		EM_Text( x - tw * 0.5f, y - 19, label, a->checked ? emAccent : emWhite );
	}

	EM_Box( 10, 10, SCREEN_WIDTH - 20, 18, emPanel );
	EM_TextClipped( 16, 13, SCREEN_WIDTH - 32, va( "Click an effect to check or uncheck it (%i checked).   Right-click, Enter or Esc goes back", EM_CountChecked() ), emWhite );
	re->DrawStretchPic( em.cursorX, em.cursorY, 32, 32, 0, 0, 1, 1, cls.cursorShader );
}

// the move arrows over what they move, and the mouse
static void EM_DrawGizmo( void ) {
	gzView_t v;
	vec3_t pivot;

	if ( !em.gizmo )
		return;
	if ( em.haveView && EM_GzPivot( pivot ) ) {
		EM_GzView( &v );
		GZ_Draw( &v, pivot, em.gzHot );
	}
	if ( !em.held[A_MOUSE2] && em.naming == EM_NAMING_NONE )
		GZ_DrawCursor( em.cursorX, em.cursorY );
}

// drawn over the cgame, under the UI and console
void CL_EffectManager_Draw( void ) {
	switch ( em.state ) {
	case EM_BROWSE:	EM_DrawBrowse();	break;
	case EM_PLACE:	EM_DrawPlace();	EM_DrawGizmo();	break;
	case EM_PICK:	EM_DrawPick();		break;
	default:		break;
	}
}
