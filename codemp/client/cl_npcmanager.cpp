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

// cl_npcmanager.cpp -- engine-side front end for RPMod's /npc command
//
// Spawn tab: every NPC type in ext_data/npcs/*.npc the filesystem can see, Base (the
// game's own pk3s) or Custom, with targetname, team, model, scale and a spot picked on
// the map, sent as "npc spawn". Picking on the map flies a free camera, so the spot can
// be anywhere, not just in view. Several can be put down there and spawned together,
// one a second as the server's flood protection lets them through, and saved as an NPC
// construct to put down again. The form's settings can be saved as an NPC of their own.
// Manage tab: the NPCs in the map, read from the server's "npc list" reply, and the
// kill/freeze/emote/follow/dialog/tele/hologram/score commands to run on them.
//
// This lives in the engine rather than cgame so it works whatever cgame the
// server's mod ships (RPMod loads its own). The list is read off the "print"
// server commands on their way to the cgame, and kept out of the console when
// the manager asked for it.

#include "client.h"
#include "qcommon/cm_public.h"
#include "ghoul2/G2.h"
#include "cl_widgets.h"

#define NM_TOGGLE_CMD		"npcmanager"
#define NM_LIST_HEADER		"List of all NPCs in the map"

#define NM_SAVE_DIR			"npcmanager"		// in the game folder
#define NM_NPC_DIR			NM_SAVE_DIR "/npcs"
#define NM_CONSTRUCT_DIR	NM_SAVE_DIR "/constructs"
#define NM_ROUTE_DIR		NM_SAVE_DIR "/routes"		// cl_npcwalk.cpp reads them
#define NM_WALK_SPEED		90.0f
#define NM_RUN_SPEED		250.0f
#define NM_SAVE_EXT			".cfg"
#define NM_BASE_PAKS		BASEGAME "/assets"	// base/assets0.pk3 to assets3.pk3 are the game's own

#define NM_MAX_TYPES		4096
#define NM_MAX_SKINS		8192
#define NM_MAX_NPCS			1024
#define NM_MAX_FILES		1024
#define NM_MAX_SOURCES		256
#define NM_MAX_SAVED		512
#define NM_MAX_PLACED		256		// put down, not spawned yet; also a construct's NPCs
#define NM_MAX_QUEUE		512
#define NM_CMD_LEN			256
#define NM_POOL_SIZE		( 512 * 1024 )
#define NM_FIELD_LEN		160		// long enough for a line an NPC says
#define NM_MESSAGE_MS		5000

#define NM_LIST_TIMEOUT		2500	// ms without the list header before giving up on it
#define NM_LIST_LINE_GAP	1500	// ms between list lines before the list is over
#define NM_REPLY_MS			3000	// prints this soon after a command are shown as its reply
#define NM_REFRESH_DELAY	400		// ms after a kill or tele before reading the list again
#define NM_CONFIRM_MS		3000
#define NM_DOUBLECLICK_MS	400
#define NM_TRACE_MASK		(CONTENTS_SOLID|CONTENTS_TERRAIN)
#define NM_TRACE_DIST		8192.0f
#define NM_GROUND_HEIGHT	25.0f	// NPC origins sit this far above the floor
#define NM_MAX_MARKED		16		// NPCs marked to walk together, as many as can walk at once
#define NM_MAX_ROUTES		128		// saved routes listed in the Routes tab
#define NM_MAX_WALKS		32		// walks going on, listed and drawn
#define NM_ROUTE_LIST_Y		132.0f	// the Routes tab: the saved routes
#define NM_ROUTE_LIST_H		106.0f
#define NM_WALK_LIST_Y		318.0f	// and the walks going on
#define NM_WALK_LIST_H		60.0f
#define NM_FRONT_DIST		96.0f	// "in front of me"
#define NM_YAW_STEP			15
#define NM_FLY_SPEED		400.0f	// free camera, units a second
#define NM_LABEL_RANGE		32.0f	// a label this close to the crosshair is the one picked

// layout, in 640x480
#define NM_ROW_H			13.0f
#define NM_CTRL_H			15.0f
#define NM_TEXT_SCALE		0.8f
#define NM_LEFT_X			16.0f
#define NM_LEFT_W			250.0f
#define NM_RIGHT_X			276.0f
#define NM_RIGHT_W			348.0f
#define NM_CTRL_X			( NM_RIGHT_X + 68.0f )
#define NM_CTRL_W			( NM_RIGHT_X + NM_RIGHT_W - NM_CTRL_X )
#define NM_SEARCH_Y			46.0f
#define NM_LIST_Y			66.0f
#define NM_LIST_H			364.0f
#define NM_TYPE_LIST_Y		( NM_LIST_Y + 20.0f )	// under the All / Base / Custom buttons
#define NM_TYPE_LIST_H		( NM_LIST_H - 20.0f )
#define NM_TREE_W			128.0f					// Saved, Base, Custom > pk3 > .npc file
#define NM_TYPES_X			( NM_LEFT_X + NM_TREE_W + 4.0f )	// and the NPCs of the one selected
#define NM_TYPES_W			( NM_LEFT_W - NM_TREE_W - 4.0f )
#define NM_MAX_ROWS			2048
#define NM_PLACED_Y			222.0f
#define NM_PLACED_ROWS		7
#define NM_BUTTON_Y			410.0f
#define NM_PICKER_X			150.0f
#define NM_PICKER_Y			50.0f
#define NM_PICKER_W			340.0f
#define NM_PICKER_H			380.0f
#define NM_PICKER_MODELS_X	60.0f	// the model picker, with its preview
#define NM_PICKER_PREVIEW_W	200.0f
#define NM_PREVIEW_X		170.0f
#define NM_PREVIEW_Y		40.0f
#define NM_PREVIEW_W		300.0f
#define NM_PREVIEW_H		400.0f

typedef struct nmType_s {
	const char	*name;		// what "npc spawn" takes
	const char	*model;		// its playerModel[/customSkin], "" if it has none
	qboolean	vehicle;
	// what else changes its look, for the preview
	const char	*surfOff, *surfOn;	// comma separated
	byte		rgba[4];			// customRGBA, white if it has none or a random one
	int			scale;				// percent
	const char	*weapon;			// WP_ name, "" for none
	const char	*saber, *saber2;	// .sab names, "" for none
	int			source;				// where its .npc file is read from, index into sources
	qboolean	custom;				// not from the game's own pk3s
	const char	*file;				// that .npc file, in ext_data/npcs
} nmType_t;

// the left of the spawn tab, as the model manager has it: what was saved, then Base with its
// .npc files, then Custom with a row for each pk3 and that pk3's .npc files under it
typedef enum {
	NM_ROW_SAVED,
	NM_ROW_CATEGORY,	// Base or Custom
	NM_ROW_SOURCE,		// a custom pk3
	NM_ROW_FILE			// a .npc file
} nmRowType_t;

typedef struct nmRow_s {
	nmRowType_t	type;
	char		name[MAX_QPATH];
	int			first, end;			// range in the sorted types, end not in it
	int			count;				// of them, the filters let through
	qboolean	custom;
} nmRow_t;

typedef struct nmNpc_s {
	int			num;
	vec3_t		origin;
	int			yaw;
	char		type[MAX_QPATH];
	char		name[MAX_QPATH];	// targetname, "" if it has none
} nmNpc_t;

typedef struct nmList_s {
	int			sel, scroll;
	int			lastClickTime, lastClickRow;
} nmList_t;

typedef enum {
	NM_TAB_SPAWN,
	NM_TAB_MANAGE,
	NM_TAB_ROUTES
} nmTab_t;

// the Routes tab's NPC list: every NPC, the ones walking a route, or the others
typedef enum {
	NM_RSHOW_ALL,
	NM_RSHOW_WALKING,
	NM_RSHOW_IDLE
} nmRouteShow_t;

typedef enum {
	NM_PICK_NONE,
	NM_PICK_SPAWN,		// where to spawn
	NM_PICK_PLACE,		// where to put down what is in hand, as many times as wanted
	NM_PICK_TELE,		// where to teleport the selection to
	NM_PICK_SELECT,		// which NPC to select
	NM_PICK_ROUTE,		// the spots of a walk, one after the other
	NM_PICK_ROUTES		// looking at the routes walked, to pick an NPC by its route
} nmPick_t;

typedef enum {
	NM_SRC_ALL,
	NM_SRC_BASE,
	NM_SRC_CUSTOM,
	NM_SRC_COUNT
} nmSource_t;

static const char *nmSourceLabels[NM_SRC_COUNT] = { "All", "Base", "Custom" };

typedef enum {
	NM_SAVING_NONE,
	NM_SAVING_NPC,			// the form's settings
	NM_SAVING_CONSTRUCT,	// the NPCs put down
	NM_SAVING_ROUTE			// the walk's spots, walked once saved
} nmSaving_t;

typedef enum {
	NM_PICKER_NONE,
	NM_PICKER_MODEL,
	NM_PICKER_EMOTE,
	NM_PICKER_PLAYER
} nmPicker_t;

typedef enum {
	NM_SHOW_ALL,
	NM_SHOW_CHARACTERS,
	NM_SHOW_VEHICLES
} nmShow_t;

typedef enum {
	NM_TEAM_DEFAULT,
	NM_TEAM_PLAYER,
	NM_TEAM_ENEMY
} nmTeam_t;

typedef enum {
	NM_APPLY_ONE,		// the selected NPC, by number
	NM_APPLY_NAME,		// every NPC sharing its targetname
	NM_APPLY_ALL,
	NM_APPLY_MARKED		// the NPCs marked with Ctrl+click: they walk together, nothing else takes them
} nmApply_t;

typedef enum {
	NM_F_NONE,			// typing goes to the search box of whatever is showing
	NM_F_TYPE_SEARCH,
	NM_F_TARGETNAME,
	NM_F_MODEL,
	NM_F_SCALE,
	NM_F_NPC_SEARCH,
	NM_F_EMOTE,
	NM_F_DIALOG,
	NM_F_TALK_TEXT,		// what the NPC says, and as who
	NM_F_TALK_NAME,
	NM_F_PICKER_SEARCH,
	NM_F_SAVE_NAME,
	NM_F_TELE_X,		// the teleport box, in this order
	NM_F_TELE_Y,
	NM_F_TELE_Z,
	NM_F_TELE_YAW,
	NM_NUM_FIELDS
} nmField_t;

typedef enum {
	NM_CONFIRM_NONE,
	NM_CONFIRM_KILL,
	NM_CONFIRM_SPAWNERS,
	NM_CONFIRM_CLEAR,
	NM_CONFIRM_DELETE_ROUTE,
	NM_CONFIRM_STOP_WALKS
} nmConfirm_t;

// what one spawn takes: the form's, a saved NPC's, or one of a construct
typedef struct nmSpawn_s {
	char		type[MAX_QPATH];
	qboolean	vehicle;
	char		targetname[NM_FIELD_LEN];
	nmTeam_t	team;
	char		model[NM_FIELD_LEN];	// "" = its own
	char		scale[NM_FIELD_LEN];	// "" = its own
	vec3_t		pos;					// in the map; in a construct, from its middle
	int			yaw;
} nmSpawn_t;

// a file in NM_NPC_DIR or NM_CONSTRUCT_DIR
typedef struct nmSaved_s {
	char		name[MAX_QPATH];
	qboolean	construct;
	int			count;		// a construct's NPCs
	nmSpawn_t	npc;		// a saved NPC's settings
} nmSaved_t;

// the emotes in RPMod's help menu
static const char *nmEmotes[] = {
	"emtalk",
	"emtalk2",
	"emcomm",
	"emknockdown",
	"emknockdown2",
	"emknockdown3",
	"emkneel",
	"emkneel2",
	"emkneel3",
	"emsurrender",
	"emraisehand",
	"emsleep",
	"emconsole",
	"emconsolecomm",
	"emyes",
	"emno",
	"emcomeon",
	"emcomeon2",
	"emhello",
	"embeg",
	"emcower",
	"emnoisy",
	"emwait",
	"ematease",
	"emhips",
	"emreach",
	"emreach2",
	"emlotus",
	"emsit",
	"emsit2",
	"emsit3",
	"emsit4",
	"emsit5",
	"emsit6",
	"emsit7",
	"emsit8",
	"emsit9",
	"emsit10",
	"emsit11",
	"emsit12",
	"emsitconsole",
	"emliedown",
	"emhandsignal1",
	"emhandsignal2",
	"emhandsignal3",
	"emhandsignal4",
	"emhandsignall",
	"emhandsignalr",
	"emgrip",
	"emgrip2",
	"emgrip3",
	"emgrip4",
	"emthrow",
	"embutton",
	"embutton2",
	"emsniper",
	"emknockback",
	"emhurt",
	"emgrab",
	"emgrabbed",
	"emheadtiltl",
	"emshy",
	"emshuffle",
	"embalance",
	"emcheer",
	"emdeath",
	"emdeath2",
	"emdeath3",
	"emdeath4",
	"emdead",
	"emdead2",
	"emdead3",
	"emdodgel",
	"emdodger",
	"embackflip",
	"emsaberkick",
	"emsaberpose",
	"emsaberpose2",
	"emsaberpose3",
	"emsaberpose4",
	"emsaberpose5",
	"emsaberpose6",
	"emsaberfinish",
	"emsaberthrow",
	"emsaberthrow2",
	"empoint",
	"empoint2",
	"emwindy",
	"emsabertaunt",
	"emsabertaunt2",
	"emsabertaunt3",
	"emsabertaunt4",
	"emsaberlock1",
	"emsaberlock2",
	"emcrossarms",
	"emlift",
	"emlift2",
	"empose",
	"empose2",
	"empose3",
	"empose4",
	"emsalute",
	"emlean",
	"emgun",
	"ematease2",
};

// the name's colour codes, white first
static const char nmTalkColors[] = "7123456890";

static const char *nmTeamArgs[] = { "0", "player", "enemy" };
static const char *nmTeamLabels[] = { "Default", "Player (blue)", "Enemy (red)" };

static struct {
	qboolean	open;
	int			font;
	nmTab_t		tab;
	nmPick_t	pick;
	nmPicker_t	picker;

	// input; clicks and the wheel are handled where the control under them is drawn
	float		cursorX, cursorY;
	qboolean	click;
	float		clickX, clickY;
	int			wheel;
	nmField_t	focus;
	char		fields[NM_NUM_FIELDS][NM_FIELD_LEN];

	// index, rebuilt the first time the manager opens after cgame (re)starts
	qboolean	indexed;
	char		pool[NM_POOL_SIZE];
	int			poolUsed;
	nmType_t	types[NM_MAX_TYPES];
	int			numTypes;
	const char	*skins[NM_MAX_SKINS];
	int			numSkins;
	const char	*sources[NM_MAX_SOURCES];	// "gamedir/name.pk3", or "gamedir" for loose files
	int			numSources;
	struct {
		const char	*file;					// in ext_data/npcs
		int			source;					// the one in use
	} files[NM_MAX_FILES];
	int			numFiles;

	// saved NPCs and constructs
	nmSaved_t	saved[NM_MAX_SAVED];
	int			numSaved;
	char		appliedSaved[MAX_QPATH];	// the saved NPC whose settings are in the form, "" = none
	char		formBefore[3][NM_FIELD_LEN];	// targetname, model and scale from before it
	nmTeam_t	teamBefore;
	char		constructShown[MAX_QPATH];	// the construct read into constructNpcs
	nmSpawn_t	constructNpcs[NM_MAX_PLACED];
	int			numConstructNpcs;
	nmSaving_t	saving;						// the name box is open
	qboolean	teleBox;					// the box to teleport to typed coordinates is open
	qboolean	experimentalBox;			// the warning before turning experimental features on

	// spawn tab
	nmShow_t	show;
	nmSource_t	source;
	nmTeam_t	team;
	nmRow_t		rows[NM_MAX_ROWS];		// of the types the filters let through
	int			numRows;
	int			row;					// the one selected, its NPCs listed beside it
	nmList_t	rowList;
	int			typeView[NM_MAX_TYPES + NM_MAX_SAVED];	// type index, or -1 - saved index
	int			numTypeView;
	nmList_t	typeList;
	qboolean	havePos;		// qfalse = in front of the player
	vec3_t		pos;
	int			yaw;

	// put down on the map, waiting for "Spawn all"
	nmSpawn_t	placed[NM_MAX_PLACED];
	int			numPlaced;
	int			placedScroll;
	// what each click puts down: the form's NPC, facing the camera, or a construct
	nmSpawn_t	hand[NM_MAX_PLACED];
	int			numHand;
	qboolean	handConstruct;
	char		handName[MAX_QPATH];

	// spawns go out one at a time, as the server's flood protection lets them through
	char		queue[NM_MAX_QUEUE][NM_CMD_LEN];
	int			numQueue, queueTotal;
	int			nextSend;

	// manage tab
	nmNpc_t		npcs[NM_MAX_NPCS];
	int			numNpcs;
	int			npcView[NM_MAX_NPCS];
	int			numNpcView;
	nmList_t	npcList;
	int			selNum;			// entity number of the selected NPC, -1 = none
	int			marked[NM_MAX_MARKED];	// entity numbers marked with Ctrl+click
	int			numMarked;
	nmApply_t	apply;
	int			talkColor;		// of the name, in nmTalkColors
	qboolean	talkEmote;		// talking plays emtalk on it
	vec3_t		route[NW_MAX_ROUTE_POINTS];	// the walk being made
	int			numRoute;
	qboolean	routeRun;		// the route is run, not walked
	qboolean	routeLoop;		// walked round and round until stopped
	char		routeName[MAX_QPATH];	// the last one saved, to walk again
	qboolean	routeSaveOnly;	// the route being made is saved, not walked: the Routes tab's

	// routes tab
	struct {
		char		name[MAX_QPATH];
		vec3_t		pts[NW_MAX_ROUTE_POINTS];
		int			numPts;
		float		speed;
	} routes[NM_MAX_ROUTES];			// the saved ones
	int			numRoutes;
	nmList_t	routeList;
	npcWalkInfo_t	walks[NM_MAX_WALKS];	// going on, read each frame the tab or the map view shows
	int			numWalks;
	nmList_t	walkList;
	nmRouteShow_t	routeShow;
	qboolean	routeOutlines;	// the map view draws the routes' lines and spots
	qboolean	routeFocus;		// and only the one hovered, or the selected NPC's
	nmConfirm_t	confirm;
	int			confirmTime;

	// reading the list
	int			listRequestTime;	// asked for it, waiting for the header, 0 = not waiting
	qboolean	listOurs;			// the list being read was asked for by the manager
	int			listLineTime;		// last list line read, 0 = not reading a list
	int			listTime;			// when a list was last read, 0 = never
	qboolean	listDone;			// all of it read, the walks told
	int			refreshTime;		// read the list again at this time, 0 = no

	// picker
	int			pickerView[NM_MAX_SKINS];
	int			numPickerView;
	nmList_t	pickerList;

	// preview popup
	qboolean	preview;
	char		previewKey[MAX_QPATH * 2];	// type and model the instance below was built for
	CGhoul2Info_v *previewG2;
	CGhoul2Info_v *previewHeld[2];			// what was copied into its slots 1 and 2, kept like cgame does
	qboolean	previewOwnModel;			// the type's own model, not one from the model box
	vec3_t		previewMins, previewMaxs;	// unscaled, from the .glm
	char		previewModel[MAX_QPATH];	// model/skin shown
	const char	*previewError;
	qboolean	previewSkinMissing;
	float		previewYaw;					// added by the wheel

	// picking on the map, with a free camera
	int			pickYaw;		// added to facing the camera
	vec3_t		camOrg, camAng;
	qboolean	held[MAX_KEYS];
	int			lastFrameTime;
	int			viewFrame;		// cls.framecount the camera last moved on
	float		pickCursorX, pickCursorY;	// where the cursor was, back when picking ends

	// the main view last frame: the cgame's, or the free camera's
	qboolean	haveView;
	vec3_t		viewOrg;
	matrix3_t	viewAxis;
	float		fovX, fovY;

	// messages
	char		message[MAX_STRING_CHARS];
	int			messageTime;
	char		lastCmd[MAX_STRING_CHARS];
	int			sentTime;
	char		reply[MAX_STRING_CHARS];
	int			replyTime;
} nm;

static vec4_t nmWhite		= { 1.0f, 1.0f, 1.0f, 1.0f };
static vec4_t nmRed			= { 1.0f, 0.3f, 0.3f, 1.0f };
static vec4_t nmPanel		= { 0.0f, 0.0f, 0.0f, 0.65f };
static vec4_t nmPanelOpaque	= { 0.05f, 0.05f, 0.05f, 0.95f };	// a box over the panel, which would show through
static vec4_t nmPanelLight	= { 0.15f, 0.15f, 0.15f, 0.8f };
static vec4_t nmPanelFocus	= { 0.1f, 0.2f, 0.3f, 0.9f };
static vec4_t nmHighlight	= { 0.2f, 0.45f, 0.8f, 0.8f };
static vec4_t nmHover		= { 1.0f, 1.0f, 1.0f, 0.12f };
static vec4_t nmHoverBox	= { 0.25f, 0.25f, 0.25f, 0.9f };
static vec4_t nmDanger		= { 0.6f, 0.15f, 0.15f, 0.9f };
static vec4_t nmBorder		= { 0.5f, 0.5f, 0.5f, 0.8f };
static vec4_t nmDim			= { 0.7f, 0.7f, 0.7f, 1.0f };
static vec4_t nmAccent		= { 1.0f, 0.8f, 0.3f, 1.0f };
static vec4_t nmPlacedColor	= { 0.4f, 1.0f, 0.5f, 1.0f };
static vec4_t nmCustom		= { 0.6f, 1.0f, 0.6f, 1.0f };		// custom NPC types in the list
static vec4_t nmRouteColors[] = {				// each walk's route in the map view
	{ 0.4f, 1.0f, 0.5f, 1.0f },
	{ 0.4f, 0.8f, 1.0f, 1.0f },
	{ 1.0f, 0.5f, 0.9f, 1.0f },
	{ 1.0f, 0.6f, 0.3f, 1.0f },
	{ 0.8f, 0.8f, 0.3f, 1.0f },
	{ 0.6f, 0.6f, 1.0f, 1.0f }
};

static void NM_RequestList( void );
static void NM_BuildRows( void );
static void NM_RefreshRoutes( void );

/*
===============================================================================

INDEX

===============================================================================
*/

static const char *NM_PoolString( const char *s ) {
	int len = strlen( s ) + 1;
	char *out;

	if ( nm.poolUsed + len > NM_POOL_SIZE )
		return NULL;
	out = nm.pool + nm.poolUsed;
	memcpy( out, s, len );
	nm.poolUsed += len;
	return out;
}

// "base/assets<digits>.pk3"; anything else people named assets_something isn't
static qboolean NM_IsBaseSource( const char *source ) {
	const int prefixLen = strlen( NM_BASE_PAKS );
	const char *c = source + prefixLen;

	if ( Q_stricmpn( source, NM_BASE_PAKS, prefixLen ) || !isdigit( (unsigned char)*c ) )
		return qfalse;
	while ( isdigit( (unsigned char)*c ) )
		c++;
	return (qboolean)!Q_stricmp( c, ".pk3" );
}

static int NM_SourceIndex( const char *source ) {
	const char *stored;

	for ( int i = 0; i < nm.numSources; i++ ) {
		if ( !Q_stricmp( nm.sources[i], source ) )
			return i;
	}
	if ( nm.numSources >= NM_MAX_SOURCES || !( stored = NM_PoolString( source ) ) )
		return 0;
	nm.sources[nm.numSources] = stored;
	return nm.numSources++;
}

// name is a full game path, e.g. "ext_data/npcs/jedi.npc"; the first one seen is the one in use
static void NM_AddNpcFile( const char *name, const char *source, void *ctx ) {
	const char *file = name + strlen( "ext_data/npcs/" );

	// the game only reads the folder itself
	if ( strchr( file, '/' ) || strchr( file, '\\' ) || nm.numFiles >= NM_MAX_FILES )
		return;
	for ( int i = 0; i < nm.numFiles; i++ ) {
		if ( !Q_stricmp( nm.files[i].file, file ) )
			return;
	}
	nm.files[nm.numFiles].file = NM_PoolString( file );
	nm.files[nm.numFiles].source = NM_SourceIndex( source );
	if ( nm.files[nm.numFiles].file )
		nm.numFiles++;
}

// 0, "unknown", if it wasn't seen
static int NM_FileSource( const char *file ) {
	for ( int i = 0; i < nm.numFiles; i++ ) {
		if ( !Q_stricmp( nm.files[i].file, file ) )
			return nm.files[i].source;
	}
	return 0;
}

// looks holds what NM_ParseNpcFile read besides the model
static void NM_AddType( const char *name, const char *model, const char *skin, const char *surfOff, const char *surfOn, const nmType_t *looks ) {
	char full[MAX_QPATH];
	nmType_t *t;

	// the game takes the first block with the name, in file list order
	for ( int i = 0; i < nm.numTypes; i++ ) {
		if ( !Q_stricmp( nm.types[i].name, name ) )
			return;
	}
	if ( nm.numTypes >= NM_MAX_TYPES )
		return;

	if ( model[0] && skin[0] && Q_stricmp( skin, "default" ) )
		Com_sprintf( full, sizeof( full ), "%s/%s", model, skin );
	else
		Q_strncpyz( full, model, sizeof( full ) );

	t = &nm.types[nm.numTypes];
	*t = *looks;
	t->name = NM_PoolString( name );
	t->model = NM_PoolString( full );
	t->surfOff = NM_PoolString( surfOff );
	t->surfOn = NM_PoolString( surfOn );
	if ( t->name && t->model && t->surfOff && t->surfOn )
		nm.numTypes++;
}

// adds to a comma separated list, the way NPC_ParseParms collects surfOff/surfOn
static void NM_AddToList( char *list, int size, const char *value ) {
	if ( list[0] )
		Q_strcat( list, size, "," );
	Q_strcat( list, size, value );
}

// reads every block out of one .npc file, the way NPC_ParseParms finds them
static void NM_ParseNpcFile( const char *path, const char *fileName, int source ) {
	char *buf;
	const char *p, *token, *file;
	char name[MAX_QPATH], key[64], model[MAX_QPATH], skin[MAX_QPATH];
	char surfOff[1024], surfOn[1024], weapon[64], saber[64], saber2[64];
	nmType_t looks;

	if ( FS_ReadFile( path, (void **)&buf ) < 0 || !buf )
		return;
	file = NM_PoolString( fileName );
	if ( !file ) {
		FS_FreeFile( buf );
		return;
	}

	p = buf;
	COM_BeginParseSession( path );
	while ( 1 ) {
		token = COM_ParseExt( &p, qtrue );
		if ( !token[0] )
			break;
		Q_strncpyz( name, token, sizeof( name ) );
		if ( Q_stricmp( COM_ParseExt( &p, qtrue ), "{" ) )
			break;

		model[0] = skin[0] = surfOff[0] = surfOn[0] = weapon[0] = saber[0] = saber2[0] = '\0';
		memset( &looks, 0, sizeof( looks ) );
		looks.rgba[0] = looks.rgba[1] = looks.rgba[2] = looks.rgba[3] = 255;
		looks.scale = 100;
		looks.source = source;
		looks.custom = (qboolean)!NM_IsBaseSource( nm.sources[source] );
		looks.file = file;
		while ( 1 ) {
			token = COM_ParseExt( &p, qtrue );
			if ( !token[0] || !Q_stricmp( token, "}" ) )
				break;
			if ( !Q_stricmp( token, "{" ) ) {
				SkipBracedSection( &p, 1 );
				continue;
			}
			Q_strncpyz( key, token, sizeof( key ) );
			token = COM_ParseExt( &p, qfalse );
			if ( !Q_stricmp( key, "playerModel" ) )
				Q_strncpyz( model, token, sizeof( model ) );
			else if ( !Q_stricmp( key, "customSkin" ) )
				Q_strncpyz( skin, token, sizeof( skin ) );
			else if ( !Q_stricmp( key, "class" ) )
				looks.vehicle = (qboolean)!Q_stricmp( token, "CLASS_VEHICLE" );
			else if ( !Q_stricmp( key, "surfOff" ) )
				NM_AddToList( surfOff, sizeof( surfOff ), token );
			else if ( !Q_stricmp( key, "surfOn" ) )
				NM_AddToList( surfOn, sizeof( surfOn ), token );
			else if ( !Q_stricmp( key, "weapon" ) )
				Q_strncpyz( weapon, token, sizeof( weapon ) );
			else if ( !Q_stricmp( key, "saber" ) )
				Q_strncpyz( saber, token, sizeof( saber ) );
			else if ( !Q_stricmp( key, "saber2" ) )
				Q_strncpyz( saber2, token, sizeof( saber2 ) );
			else if ( !Q_stricmp( key, "scale" ) ) {
				if ( atoi( token ) > 0 )
					looks.scale = atoi( token );
			} else if ( !Q_stricmp( key, "customRGBA" ) && ( isdigit( (unsigned char)token[0] ) ) ) {
				// "r g b [a]"; the named and random ones differ per spawn, so stay white
				looks.rgba[0] = (byte)Com_Clampi( 0, 255, atoi( token ) );
				for ( int i = 1; i < 4; i++ ) {
					token = COM_ParseExt( &p, qfalse );
					if ( !token[0] )
						break;
					looks.rgba[i] = (byte)Com_Clampi( 0, 255, atoi( token ) );
				}
			}
			SkipRestOfLine( &p );
		}
		looks.weapon = NM_PoolString( weapon );
		looks.saber = NM_PoolString( saber );
		looks.saber2 = NM_PoolString( saber2 );
		if ( looks.weapon && looks.saber && looks.saber2 )
			NM_AddType( name, model, skin, surfOff, surfOn, &looks );
	}

	FS_FreeFile( buf );
}

// name is a full game path, e.g. "models/players/kyle/model_default.skin"
static void NM_AddSkin( const char *name, void *ctx ) {
	static const char prefix[] = "models/players/";
	char path[MAX_QPATH], out[MAX_QPATH];
	char *model, *file, *variant, *ext;

	if ( nm.numSkins >= NM_MAX_SKINS || Q_stricmpn( name, prefix, sizeof( prefix ) - 1 ) )
		return;
	Q_strncpyz( path, name + sizeof( prefix ) - 1, sizeof( path ) );
	for ( char *c = path; *c; c++ ) {
		if ( *c == '\\' )
			*c = '/';
	}

	// only <model>/model_<variant>.skin, the ones the game can put on a player
	model = path;
	file = strchr( path, '/' );
	if ( !file || strchr( file + 1, '/' ) )
		return;
	*file++ = '\0';
	if ( Q_stricmpn( file, "model_", 6 ) )
		return;
	variant = file + 6;
	ext = strrchr( variant, '.' );
	if ( ext )
		*ext = '\0';
	if ( !variant[0] )
		return;

	if ( !Q_stricmp( variant, "default" ) )
		Q_strncpyz( out, model, sizeof( out ) );
	else
		Com_sprintf( out, sizeof( out ), "%s/%s", model, variant );
	nm.skins[nm.numSkins] = NM_PoolString( out );
	if ( nm.skins[nm.numSkins] )
		nm.numSkins++;
}

// Base first, then Custom pk3 by pk3, then by .npc file: every row is one contiguous range
static int QDECL NM_CompareTypes( const void *a, const void *b ) {
	const nmType_t *ta = (const nmType_t *)a, *tb = (const nmType_t *)b;
	int cmp;

	if ( ta->custom != tb->custom )
		return ta->custom ? 1 : -1;
	if ( ta->custom && ta->source != tb->source && ( cmp = Q_stricmp( nm.sources[ta->source], nm.sources[tb->source] ) ) )
		return cmp;
	if ( ( cmp = Q_stricmp( ta->file, tb->file ) ) != 0 )
		return cmp;
	return Q_stricmp( ta->name, tb->name );
}

static int QDECL NM_CompareStrings( const void *a, const void *b ) {
	return Q_stricmp( *(const char **)a, *(const char **)b );
}

static void NM_BuildIndex( void ) {
	int start = Sys_Milliseconds(), numFiles, out;
	char **files;

	nm.poolUsed = 0;
	nm.numTypes = 0;
	nm.numSkins = 0;
	nm.numSources = 0;
	nm.numFiles = 0;
	NM_SourceIndex( "unknown" );	// 0

	// where each file is read from; the game's own list sets the order the types are taken in
	FS_ListFilesRecursiveFrom( "ext_data/npcs", ".npc", NM_AddNpcFile, NULL );
	files = FS_ListFiles( "ext_data/npcs", ".npc", &numFiles );
	for ( int i = 0; i < numFiles; i++ )
		NM_ParseNpcFile( va( "ext_data/npcs/%s", files[i] ), files[i], NM_FileSource( files[i] ) );
	FS_FreeFileList( files );
	qsort( nm.types, nm.numTypes, sizeof( nm.types[0] ), NM_CompareTypes );

	// the same skin can be in several pk3s and folders
	FS_ListFilesRecursive( "models/players", ".skin", NM_AddSkin, NULL );
	qsort( nm.skins, nm.numSkins, sizeof( nm.skins[0] ), NM_CompareStrings );
	for ( int i = out = 0; i < nm.numSkins; i++ ) {
		if ( !out || Q_stricmp( nm.skins[out - 1], nm.skins[i] ) )
			nm.skins[out++] = nm.skins[i];
	}
	nm.numSkins = out;

	nm.indexed = qtrue;
	nm.typeList.sel = nm.typeList.scroll = 0;
	nm.row = -1;	// the first file, once the rows are built
	NM_BuildRows();
	out = 0;
	for ( int i = 0; i < nm.numTypes; i++ ) {
		if ( nm.types[i].custom )
			out++;
	}
	Com_Printf( "NPC manager: indexed %i NPC types, %i of them custom, and %i player models (%i ms)\n",
		nm.numTypes, out, nm.numSkins, Sys_Milliseconds() - start );
}

/*
===============================================================================

HELPERS

===============================================================================
*/

static qboolean NM_ContainsNoCase( const char *haystack, const char *needle ) {
	int nlen = strlen( needle );

	if ( !nlen )
		return qtrue;
	for ( ; *haystack; haystack++ ) {
		if ( !Q_stricmpn( haystack, needle, nlen ) )
			return qtrue;
	}
	return qfalse;
}

static const char *NM_PlayerName( int clientNum ) {
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

static void NM_Message( const char *text ) {
	Q_strncpyz( nm.message, text, sizeof( nm.message ) );
	nm.messageTime = cls.realtime;
}

// the type with the name; one only the server may know, as saved, stands in for a missing one
static const nmType_t *NM_FindType( const char *name, qboolean vehicle ) {
	static nmType_t stub;
	static char stubName[MAX_QPATH];

	for ( int i = 0; i < nm.numTypes; i++ ) {
		if ( !Q_stricmp( nm.types[i].name, name ) )
			return &nm.types[i];
	}
	Q_strncpyz( stubName, name, sizeof( stubName ) );
	memset( &stub, 0, sizeof( stub ) );
	stub.name = stubName;
	stub.model = stub.surfOff = stub.surfOn = stub.weapon = stub.saber = stub.saber2 = "";
	stub.vehicle = vehicle;
	stub.rgba[0] = stub.rgba[1] = stub.rgba[2] = stub.rgba[3] = 255;
	stub.scale = 100;
	stub.custom = qtrue;
	return &stub;
}

// the saved NPC or construct on the selected row, NULL if it's a type
static const nmSaved_t *NM_SelectedSaved( void ) {
	int v;

	if ( nm.typeList.sel < 0 || nm.typeList.sel >= nm.numTypeView )
		return NULL;
	v = nm.typeView[nm.typeList.sel];
	return v < 0 ? &nm.saved[-1 - v] : NULL;
}

// a saved NPC's type for one; NULL on a construct
static const nmType_t *NM_SelectedType( void ) {
	const nmSaved_t *saved = NM_SelectedSaved();

	if ( nm.typeList.sel < 0 || nm.typeList.sel >= nm.numTypeView )
		return NULL;
	if ( !saved )
		return &nm.types[nm.typeView[nm.typeList.sel]];
	return saved->construct ? NULL : NM_FindType( saved->npc.type, saved->npc.vehicle );
}

static const nmNpc_t *NM_FindNpc( int num ) {
	for ( int i = 0; i < nm.numNpcs; i++ ) {
		if ( nm.npcs[i].num == num )
			return &nm.npcs[i];
	}
	return NULL;
}

static const nmNpc_t *NM_SelectedNpc( void ) {
	return nm.selNum >= 0 ? NM_FindNpc( nm.selNum ) : NULL;
}

static int NM_CountNamed( const char *name ) {
	int count = 0;

	for ( int i = 0; i < nm.numNpcs; i++ ) {
		if ( !Q_stricmp( nm.npcs[i].name, name ) )
			count++;
	}
	return count;
}

static qboolean NM_IsMarked( int num ) {
	for ( int i = 0; i < nm.numMarked; i++ ) {
		if ( nm.marked[i] == num )
			return qtrue;
	}
	return qfalse;
}

// the marked ones still in the list
static int NM_CountMarked( void ) {
	int count = 0;

	for ( int i = 0; i < nm.numMarked; i++ ) {
		if ( NM_FindNpc( nm.marked[i] ) )
			count++;
	}
	return count;
}

// Ctrl+click: marked, or not any more; the commands then go to the marked ones, or back to one
static void NM_ToggleMark( int num ) {
	for ( int i = 0; i < nm.numMarked; i++ ) {
		if ( nm.marked[i] == num ) {
			memmove( &nm.marked[i], &nm.marked[i + 1], ( nm.numMarked - i - 1 ) * sizeof( nm.marked[0] ) );
			nm.numMarked--;
			if ( !NM_CountMarked() && nm.apply == NM_APPLY_MARKED )
				nm.apply = NM_APPLY_ONE;
			return;
		}
	}
	// the ones gone from the map make room
	for ( int i = 0; i < nm.numMarked; ) {
		if ( !NM_FindNpc( nm.marked[i] ) )
			nm.marked[i] = nm.marked[--nm.numMarked];
		else
			i++;
	}
	if ( nm.numMarked >= NM_MAX_MARKED ) {
		NM_Message( va( S_COLOR_YELLOW "At most %i NPCs walk together", NM_MAX_MARKED ) );
		return;
	}
	nm.marked[nm.numMarked++] = num;
	nm.apply = NM_APPLY_MARKED;
}

// where the NPC is now if it's in the snapshot, else where the list saw it
static void NM_NpcOrigin( const nmNpc_t *npc, vec3_t out ) {
	for ( int i = 0; i < cl.snap.numEntities; i++ ) {
		const entityState_t *es = &cl.parseEntities[( cl.snap.parseEntitiesNum + i ) & ( MAX_PARSE_ENTITIES - 1 )];
		if ( es->number == npc->num ) {
			VectorCopy( es->pos.trBase, out );
			return;
		}
	}
	VectorCopy( npc->origin, out );
}

static int NM_NormalizeYaw( float a ) {
	int i = (int)floorf( a + 0.5f ) % 360;
	return i < 0 ? i + 360 : i;
}

// yaw facing from one spot to the next
static float NM_LegYaw( const vec3_t from, const vec3_t to ) {
	vec3_t dir, angles;

	VectorSubtract( to, from, dir );
	dir[2] = 0;
	vectoangles( dir, angles );
	return angles[YAW];
}

// yaw for something at pos to face the camera
static int NM_YawToView( const vec3_t pos ) {
	vec3_t dir, angles;

	VectorSubtract( nm.haveView ? nm.viewOrg : cl.snap.ps.origin, pos, dir );
	dir[2] = 0;
	vectoangles( dir, angles );
	return NM_NormalizeYaw( angles[YAW] );
}

static void NM_InFrontOfPlayer( vec3_t pos, int *yaw ) {
	vec3_t angles, forward;

	VectorSet( angles, 0, cl.viewangles[YAW], 0 );
	AngleVectors( angles, forward, NULL, NULL );
	VectorMA( cl.snap.ps.origin, NM_FRONT_DIST, forward, pos );
	*yaw = NM_NormalizeYaw( cl.viewangles[YAW] + 180.0f );
}

static void NM_ListClamp( nmList_t *list, int count, int visible ) {
	list->sel = Com_Clampi( 0, count ? count - 1 : 0, list->sel );
	list->scroll = Com_Clampi( 0, count > visible ? count - visible : 0, list->scroll );
}

static void NM_ListMove( nmList_t *list, int count, int visible, int delta ) {
	if ( !count )
		return;
	list->sel = Com_Clampi( 0, count - 1, list->sel + delta );
	if ( list->sel < list->scroll )
		list->scroll = list->sel;
	else if ( list->sel >= list->scroll + visible )
		list->scroll = list->sel - visible + 1;
}

// the field typing goes to
static nmField_t NM_ActiveField( void ) {
	if ( nm.picker != NM_PICKER_NONE )
		return NM_F_PICKER_SEARCH;
	if ( nm.saving != NM_SAVING_NONE )
		return NM_F_SAVE_NAME;
	if ( nm.teleBox )
		return nm.focus >= NM_F_TELE_X && nm.focus <= NM_F_TELE_YAW ? nm.focus : NM_F_TELE_X;
	if ( nm.focus != NM_F_NONE )
		return nm.focus;
	return nm.tab == NM_TAB_SPAWN ? NM_F_TYPE_SEARCH : NM_F_NPC_SEARCH;
}

/*
===============================================================================

VIEWS

===============================================================================
*/

static qboolean NM_ShowPasses( qboolean vehicle ) {
	return (qboolean)!( ( nm.show == NM_SHOW_CHARACTERS && vehicle ) || ( nm.show == NM_SHOW_VEHICLES && !vehicle ) );
}

static qboolean NM_TypePasses( const nmType_t *t ) {
	return (qboolean)( NM_ShowPasses( t->vehicle ) && !( nm.source == NM_SRC_BASE && t->custom ) && !( nm.source == NM_SRC_CUSTOM && !t->custom ) );
}

// a construct can hold both kinds
static qboolean NM_SavedPasses( const nmSaved_t *s ) {
	return (qboolean)( s->construct || NM_ShowPasses( s->npc.vehicle ) );
}

static int NM_AddRow( nmRowType_t type, const char *name, int first, qboolean custom ) {
	nmRow_t *r;

	if ( nm.numRows >= NM_MAX_ROWS )
		return -1;
	r = &nm.rows[nm.numRows];
	r->type = type;
	Q_strncpyz( r->name, name, sizeof( r->name ) );
	r->first = r->end = first;
	r->count = 0;
	r->custom = custom;
	return nm.numRows++;
}

// what was saved, then Base and its .npc files, then Custom with each pk3 and its files, of
// the types the filters let through. The row selected stays, if it is still there
static void NM_BuildRows( void ) {
	nmRow_t keep;
	int category = -1, source = -1, catRow = -1, srcRow = -1, fileRow = -1;
	const qboolean haveKeep = (qboolean)( nm.row >= 0 && nm.row < nm.numRows );

	if ( haveKeep )
		keep = nm.rows[nm.row];
	nm.numRows = 0;
	NM_AddRow( NM_ROW_SAVED, "Saved", 0, qfalse );
	for ( int i = 0; i < nm.numSaved; i++ ) {
		if ( NM_SavedPasses( &nm.saved[i] ) )
			nm.rows[0].count++;
	}
	for ( int i = 0; i < nm.numTypes; i++ ) {
		const nmType_t *t = &nm.types[i];

		if ( !NM_TypePasses( t ) )
			continue;
		if ( t->custom != category ) {
			category = t->custom;
			catRow = NM_AddRow( NM_ROW_CATEGORY, t->custom ? "Custom" : "Base", i, t->custom );
			source = srcRow = fileRow = -1;
		}
		if ( t->custom && t->source != source ) {
			source = t->source;
			srcRow = NM_AddRow( NM_ROW_SOURCE, nm.sources[source], i, qtrue );
			fileRow = -1;
		}
		if ( fileRow < 0 || Q_stricmp( nm.rows[fileRow].name, t->file ) )
			fileRow = NM_AddRow( NM_ROW_FILE, t->file, i, t->custom );
		if ( catRow < 0 || fileRow < 0 )
			break;	// out of rows
		nm.rows[catRow].end = nm.rows[fileRow].end = i + 1;
		nm.rows[catRow].count++;
		nm.rows[fileRow].count++;
		if ( srcRow >= 0 && t->custom ) {
			nm.rows[srcRow].end = i + 1;
			nm.rows[srcRow].count++;
		}
	}

	// the same row again, else the first file
	nm.row = -1;
	for ( int i = 0; i < nm.numRows && haveKeep; i++ ) {
		if ( nm.rows[i].type == keep.type && nm.rows[i].custom == keep.custom && !Q_stricmp( nm.rows[i].name, keep.name ) ) {
			nm.row = i;
			break;
		}
	}
	for ( int i = 0; i < nm.numRows && nm.row < 0; i++ ) {
		if ( nm.rows[i].type == NM_ROW_FILE )
			nm.row = i;
	}
	if ( nm.row < 0 )
		nm.row = 0;
	nm.rowList.sel = nm.row;
}

// the row on the left, its NPCs listed beside it
static void NM_SetRow( int row ) {
	if ( !nm.numRows )
		return;
	nm.row = Com_Clampi( 0, nm.numRows - 1, row );
	nm.rowList.sel = nm.row;
	NM_ListMove( &nm.rowList, nm.numRows, (int)( NM_TYPE_LIST_H / NM_ROW_H ), 0 );
	nm.fields[NM_F_TYPE_SEARCH][0] = '\0';
	nm.typeList.sel = nm.typeList.scroll = 0;
}

// the NPCs of the selected row, or with a search everything that matches, saved ones first
static void NM_RebuildTypeView( void ) {
	const char *search = nm.fields[NM_F_TYPE_SEARCH];
	const nmRow_t *r = nm.row >= 0 && nm.row < nm.numRows ? &nm.rows[nm.row] : NULL;
	int first = 0, end = nm.numTypes;

	nm.numTypeView = 0;
	if ( search[0] || ( r && r->type == NM_ROW_SAVED ) ) {
		for ( int i = 0; i < nm.numSaved && ( !search[0] || nm.source == NM_SRC_ALL ); i++ ) {
			const nmSaved_t *s = &nm.saved[i];

			if ( !NM_SavedPasses( s ) )
				continue;
			if ( search[0] && !NM_ContainsNoCase( s->name, search ) && ( s->construct || !NM_ContainsNoCase( s->npc.type, search ) ) )
				continue;
			nm.typeView[nm.numTypeView++] = -1 - i;
		}
		if ( !search[0] ) {
			NM_ListClamp( &nm.typeList, nm.numTypeView, (int)( NM_TYPE_LIST_H / NM_ROW_H ) );
			return;
		}
	} else if ( r ) {
		first = r->first;
		end = r->end;
	} else {
		end = 0;
	}
	for ( int i = first; i < end; i++ ) {
		const nmType_t *t = &nm.types[i];

		if ( !NM_TypePasses( t ) )
			continue;
		if ( search[0] && !NM_ContainsNoCase( t->name, search ) && !NM_ContainsNoCase( t->model, search ) )
			continue;
		nm.typeView[nm.numTypeView++] = i;
	}
	NM_ListClamp( &nm.typeList, nm.numTypeView, (int)( NM_TYPE_LIST_H / NM_ROW_H ) );
}

// the NPC list's rows: the Routes tab has its filter buttons over it
static int NM_NpcListRows( void ) {
	return (int)( ( nm.tab == NM_TAB_ROUTES ? NM_TYPE_LIST_H : NM_LIST_H ) / NM_ROW_H );
}

static void NM_RebuildNpcView( void ) {
	const char *search = nm.fields[NM_F_NPC_SEARCH];

	nm.numNpcView = 0;
	for ( int i = 0; i < nm.numNpcs; i++ ) {
		const nmNpc_t *n = &nm.npcs[i];

		if ( search[0] && !NM_ContainsNoCase( n->type, search ) && !NM_ContainsNoCase( n->name, search )
			&& !NM_ContainsNoCase( va( "%i", n->num ), search ) )
			continue;
		// the Routes tab: walking, or not
		if ( nm.tab == NM_TAB_ROUTES && nm.routeShow != NM_RSHOW_ALL
			&& ( CL_NpcWalk_RouteOf( n->num ) != NULL ) != ( nm.routeShow == NM_RSHOW_WALKING ) )
			continue;
		if ( n->num == nm.selNum )
			nm.npcList.sel = nm.numNpcView;
		nm.npcView[nm.numNpcView++] = i;
	}
	NM_ListClamp( &nm.npcList, nm.numNpcView, NM_NpcListRows() );
}

static int NM_PickerCount( void ) {
	switch ( nm.picker ) {
	case NM_PICKER_MODEL:	return nm.numSkins;
	case NM_PICKER_EMOTE:	return ARRAY_LEN( nmEmotes );
	case NM_PICKER_PLAYER:	return MAX_CLIENTS;
	default:				return 0;
	}
}

static const char *NM_PickerItem( int index ) {
	switch ( nm.picker ) {
	case NM_PICKER_MODEL:	return nm.skins[index];
	case NM_PICKER_EMOTE:	return nmEmotes[index];
	case NM_PICKER_PLAYER:	return NM_PlayerName( index );
	default:				return "";
	}
}

static void NM_RebuildPickerView( void ) {
	const char *search = nm.fields[NM_F_PICKER_SEARCH];
	int count = NM_PickerCount();

	nm.numPickerView = 0;
	for ( int i = 0; i < count; i++ ) {
		const char *item = NM_PickerItem( i );

		if ( item[0] && NM_ContainsNoCase( item, search ) )
			nm.pickerView[nm.numPickerView++] = i;
	}
	NM_ListClamp( &nm.pickerList, nm.numPickerView, (int)( ( NM_PICKER_H - 74 ) / NM_ROW_H ) );
}

/*
===============================================================================

THE NPC LIST

===============================================================================
*/

// "776: (-1934 -5523  104,   6) my57, named my-57", colors already stripped
static qboolean NM_ParseNpcLine( const char *line ) {
	nmNpc_t *n;
	float x, y, z, yaw;
	int num, used = 0;
	const char *rest, *named;

	if ( sscanf( line, "%d: (%f %f %f, %f)%n", &num, &x, &y, &z, &yaw, &used ) != 5 || !used )
		return qfalse;
	if ( nm.numNpcs >= NM_MAX_NPCS )
		return qtrue;

	n = &nm.npcs[nm.numNpcs];
	n->num = num;
	VectorSet( n->origin, x, y, z );
	n->yaw = NM_NormalizeYaw( yaw );

	rest = line + used;
	while ( *rest == ' ' )
		rest++;
	named = strstr( rest, ", named " );
	if ( named ) {
		Q_strncpyz( n->type, rest, Com_Clampi( 1, sizeof( n->type ), (int)( named - rest ) + 1 ) );
		Q_strncpyz( n->name, named + 8, sizeof( n->name ) );
	} else {
		Q_strncpyz( n->type, rest, sizeof( n->type ) );
		n->name[0] = '\0';
	}
	nm.numNpcs++;
	return qtrue;
}

// every server print goes through here on its way to the cgame; qtrue keeps it out of the console
qboolean CL_NpcManager_ServerPrint( const char *text ) {
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

		if ( !Q_stricmpn( line, NM_LIST_HEADER, strlen( NM_LIST_HEADER ) ) ) {
			// also read a list someone typed /npc list for, just don't hide it
			nm.listOurs = (qboolean)( nm.listRequestTime && cls.realtime - nm.listRequestTime < NM_LIST_TIMEOUT );
			nm.listRequestTime = 0;
			nm.listLineTime = nm.listTime = cls.realtime;
			nm.listDone = qfalse;
			nm.numNpcs = 0;
			if ( !nm.listOurs )
				swallow = qfalse;
		} else if ( nm.listLineTime && cls.realtime - nm.listLineTime < NM_LIST_LINE_GAP && NM_ParseNpcLine( line ) ) {
			nm.listLineTime = cls.realtime;
			if ( !nm.listOurs )
				swallow = qfalse;
		} else {
			swallow = qfalse;
			if ( nm.open && cls.realtime - nm.sentTime < NM_REPLY_MS ) {
				Q_strncpyz( nm.reply, line, sizeof( nm.reply ) );
				nm.replyTime = cls.realtime;
			}
		}
	}

	if ( nm.open )
		NM_RebuildNpcView();
	return (qboolean)( any && swallow );
}

/*
===============================================================================

COMMANDS

===============================================================================
*/

// ms the server's flood protection wants between commands
static int NM_CommandGap( void ) {
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

static void NM_Send( const char *cmd, qboolean echo ) {
	// opened from the storyteller, it goes in the story instead
	if ( CL_Storyteller_Capture( cmd ) )
		return;
	CL_AddReliableCommand( cmd, qfalse );
	// the queue waits for this one too
	nm.nextSend = cls.realtime + NM_CommandGap();
	if ( echo ) {
		// the list asked for behind the user's back has no reply to show
		nm.sentTime = cls.realtime;
		Q_strncpyz( nm.lastCmd, cmd, sizeof( nm.lastCmd ) );
		Com_Printf( S_COLOR_GREEN "NPC manager: " S_COLOR_WHITE "%s\n", cmd );
	}
}

static void NM_RequestList( void ) {
	// it would take a spawn's place; read it once they're all out
	if ( nm.numQueue ) {
		nm.refreshTime = cls.realtime;
		return;
	}
	nm.listRequestTime = cls.realtime;
	nm.refreshTime = 0;
	NM_Send( "npc list", qfalse );
}

// the list again once what was just sent has had time to happen, and the server takes commands
static void NM_RefreshSoon( void ) {
	nm.refreshTime = Q_max( cls.realtime + NM_REFRESH_DELAY, nm.nextSend );
}

static void NM_Queue( const char *cmd ) {
	// opened from the storyteller, it goes in the story instead; the queue would only
	// send it after the manager closed
	if ( CL_Storyteller_Capture( cmd ) )
		return;
	if ( nm.numQueue >= NM_MAX_QUEUE ) {
		Com_Printf( S_COLOR_YELLOW "NPC manager: too many spawns waiting, dropped %s\n", cmd );
		return;
	}
	Q_strncpyz( nm.queue[nm.numQueue++], cmd, sizeof( nm.queue[0] ) );
	nm.queueTotal++;
}

static void NM_RunQueue( void ) {
	if ( !nm.numQueue ) {
		nm.queueTotal = 0;
		return;
	}
	if ( cls.realtime < nm.nextSend || cls.state != CA_ACTIVE )
		return;
	NM_Send( nm.queue[0], qtrue );
	nm.numQueue--;
	memmove( nm.queue[0], nm.queue[1], nm.numQueue * sizeof( nm.queue[0] ) );
	if ( !nm.numQueue )
		NM_RefreshSoon();
}

// the NPC list for the other tools, read quietly
void CL_NpcManager_RequestList( void ) {
	if ( cls.state == CA_ACTIVE )
		NM_RequestList();
}

qboolean CL_NpcManager_Waiting( void ) {
	return (qboolean)( nm.listRequestTime && cls.realtime - nm.listRequestTime < NM_LIST_TIMEOUT );
}

// when the list was last read, 0 never
int CL_NpcManager_ListTime( void ) {
	return nm.listTime;
}

int CL_NpcManager_NumNpcs( void ) {
	return nm.numNpcs;
}

// where the NPC is now, as the manager sees it; qfalse if it isn't in the list
qboolean CL_NpcManager_NpcOrigin( int num, vec3_t out ) {
	const nmNpc_t *npc = NM_FindNpc( num );

	if ( !npc )
		return qfalse;
	NM_NpcOrigin( npc, out );
	return qtrue;
}

qboolean CL_NpcManager_GetNpc( int index, int *num, const char **type, const char **name ) {
	if ( index < 0 || index >= nm.numNpcs )
		return qfalse;
	*num = nm.npcs[index].num;
	*type = nm.npcs[index].type;
	*name = nm.npcs[index].name;
	return qtrue;
}

// "npc spawn" for one NPC; at its pos and yaw with atPos, else in front of the player
static const char *NM_SpawnCommand( const nmSpawn_t *s, qboolean atPos ) {
	static char cmd[MAX_STRING_CHARS];
	const nmType_t *t = NM_FindType( s->type, s->vehicle );
	qboolean coords = (qboolean)( atPos || s->scale[0] );
	int need, yaw;
	vec3_t pos;

	// the arguments are positional, so everything before the last one given has to be there
	if ( coords )
		need = 3;
	else if ( s->model[0] )
		need = 3;
	else if ( s->team != NM_TEAM_DEFAULT )
		need = 2;
	else if ( s->targetname[0] )
		need = 1;
	else
		need = 0;

	Com_sprintf( cmd, sizeof( cmd ), "npc spawn %s%s", s->vehicle ? "vehicle " : "", s->type );
	if ( need >= 1 )
		Q_strcat( cmd, sizeof( cmd ), va( " %s", s->targetname[0] ? s->targetname : s->type ) );
	if ( need >= 2 )
		Q_strcat( cmd, sizeof( cmd ), va( " %s", nmTeamArgs[s->team] ) );
	if ( need >= 3 ) {
		// no custom model: the NPC's own, so it looks the way it would without one
		Q_strcat( cmd, sizeof( cmd ), va( " %s", s->model[0] ? s->model : t->model[0] ? t->model : s->type ) );
	}
	if ( coords ) {
		if ( atPos ) {
			VectorCopy( s->pos, pos );
			yaw = s->yaw;
		} else {
			NM_InFrontOfPlayer( pos, &yaw );
		}
		Q_strcat( cmd, sizeof( cmd ), va( " %i %i %i %i", (int)floorf( pos[0] + 0.5f ), (int)floorf( pos[1] + 0.5f ), (int)floorf( pos[2] + 0.5f ), yaw ) );
	}
	if ( s->scale[0] )
		Q_strcat( cmd, sizeof( cmd ), va( " %s", s->scale ) );
	return cmd;
}

// "npc spawn" for the storyteller: a type set up as the form would, at a spot
const char *CL_NpcManager_SpawnCommand( const char *type, qboolean vehicle, const char *targetname, const char *team,
	const char *model, const char *scale, const vec3_t pos, int yaw ) {
	nmSpawn_t s;

	if ( !nm.indexed )
		NM_BuildIndex();
	memset( &s, 0, sizeof( s ) );
	Q_strncpyz( s.type, type, sizeof( s.type ) );
	s.vehicle = vehicle;
	Q_strncpyz( s.targetname, targetname, sizeof( s.targetname ) );
	for ( int i = 0; i < (int)ARRAY_LEN( nmTeamArgs ); i++ ) {
		if ( !Q_stricmp( team, nmTeamLabels[i] ) || !Q_stricmp( team, nmTeamArgs[i] ) )
			s.team = (nmTeam_t)i;
	}
	// a type the index doesn't know has no model of its own to fall back on
	Q_strncpyz( s.model, model[0] || NM_FindType( type, vehicle ) ? model : type, sizeof( s.model ) );
	Q_strncpyz( s.scale, scale, sizeof( s.scale ) );
	VectorCopy( pos, s.pos );
	s.yaw = yaw;
	return NM_SpawnCommand( &s, qtrue );
}

// the NPC the form describes, at the spot picked for it
static qboolean NM_FormSpawn( nmSpawn_t *out ) {
	const nmType_t *t = NM_SelectedType();

	memset( out, 0, sizeof( *out ) );
	if ( !t )
		return qfalse;
	Q_strncpyz( out->type, t->name, sizeof( out->type ) );
	out->vehicle = t->vehicle;
	Q_strncpyz( out->targetname, nm.fields[NM_F_TARGETNAME], sizeof( out->targetname ) );
	out->team = nm.team;
	Q_strncpyz( out->model, nm.fields[NM_F_MODEL], sizeof( out->model ) );
	Q_strncpyz( out->scale, nm.fields[NM_F_SCALE], sizeof( out->scale ) );
	VectorCopy( nm.pos, out->pos );
	out->yaw = nm.yaw;
	return qtrue;
}

static const char *NM_BuildSpawn( void ) {
	nmSpawn_t s;

	if ( !NM_FormSpawn( &s ) )
		return NULL;
	return NM_SpawnCommand( &s, nm.havePos );
}

static void NM_Spawn( void ) {
	const char *cmd = NM_BuildSpawn();

	if ( !cmd )
		return;
	NM_Queue( cmd );
	NM_RunQueue();
}

static void NM_SpawnPlaced( void ) {
	if ( !nm.numPlaced )
		return;
	for ( int i = 0; i < nm.numPlaced; i++ )
		NM_Queue( NM_SpawnCommand( &nm.placed[i], qtrue ) );
	NM_Message( va( S_COLOR_GREEN "Spawning %i NPCs, about one a second", nm.numPlaced ) );
	nm.numPlaced = nm.placedScroll = 0;
	NM_RunQueue();
}

/*
===============================================================================

PREVIEW

===============================================================================
*/

static void NM_PreviewFree( void ) {
	if ( nm.previewG2 )
		re->G2API_CleanGhoul2Models( &nm.previewG2 );
	nm.previewG2 = NULL;
	for ( int i = 0; i < 2; i++ ) {
		if ( nm.previewHeld[i] )
			re->G2API_CleanGhoul2Models( &nm.previewHeld[i] );
		nm.previewHeld[i] = NULL;
	}
	nm.previewKey[0] = '\0';
}

// the model is freed when the next frame starts: the popup's buttons close it
// mid-frame, after its scene went to the renderer still pointing at the model
static void NM_ClosePreview( void ) {
	nm.preview = qfalse;
}

// the scale box, or the type's own scale, in percent. The .npc scale is a
// percentage; small numbers in the box are taken as a multiplier (1.5 = 150%)
static int NM_PreviewScale( const nmType_t *t ) {
	const char *s = nm.fields[NM_F_SCALE];
	float f = (float)atof( s );

	if ( !s[0] || f <= 0.0f )
		return t->scale;
	return (int)( f <= 10.0f ? f * 100.0f + 0.5f : f );
}

// the bind pose bounds of a .glm's first LOD, or why it can't be shown
const char *CL_GlmBounds( const char *path, vec3_t mins, vec3_t maxs ) {
	byte *buf;
	const mdxmHeader_t *header;
	const mdxmLODSurfOffset_t *indexes;
	const char *error = NULL;
	int len, numSurfaces, ofsIndexes;

	len = FS_ReadFile( path, (void **)&buf );
	if ( len <= 0 || !buf )
		return "Model not found";

	ClearBounds( mins, maxs );
	header = (const mdxmHeader_t *)buf;
	numSurfaces = len >= (int)sizeof( *header ) ? LittleLong( header->numSurfaces ) : 0;
	ofsIndexes = len >= (int)sizeof( *header ) ? LittleLong( header->ofsLODs ) + (int)sizeof( mdxmLOD_t ) : 0;
	if ( len < (int)sizeof( *header ) || LittleLong( header->ident ) != MDXM_IDENT || LittleLong( header->numLODs ) < 1
		|| numSurfaces < 1 || numSurfaces > len / 4 || ofsIndexes < (int)sizeof( *header ) || ofsIndexes > len - numSurfaces * 4 ) {
		error = "Not a Ghoul2 model";
	} else {
		indexes = (const mdxmLODSurfOffset_t *)( buf + ofsIndexes );
		for ( int i = 0; i < numSurfaces && !error; i++ ) {
			const mdxmSurface_t *surf;
			const mdxmVertex_t *verts;
			int ofsSurf = ofsIndexes + LittleLong( indexes->offsets[i] ), numVerts, ofsVerts;

			if ( ofsSurf < 0 || ofsSurf > len - (int)sizeof( mdxmSurface_t ) ) {
				error = "Damaged model file";
				break;
			}
			surf = (const mdxmSurface_t *)( buf + ofsSurf );
			numVerts = LittleLong( surf->numVerts );
			ofsVerts = ofsSurf + LittleLong( surf->ofsVerts );
			if ( numVerts < 0 || numVerts > len / (int)sizeof( mdxmVertex_t ) || ofsVerts < 0 || ofsVerts > len - numVerts * (int)sizeof( mdxmVertex_t ) ) {
				error = "Damaged model file";
				break;
			}
			verts = (const mdxmVertex_t *)( buf + ofsVerts );
			for ( int j = 0; j < numVerts; j++ ) {
				vec3_t p;

				for ( int k = 0; k < 3; k++ )
					p[k] = LittleFloat( verts[j].vertCoords[k] );
				AddPointToBounds( p, mins, maxs );
			}
		}
		if ( !error && mins[0] > maxs[0] )
			error = "Model has no vertices";
	}

	FS_FreeFile( buf );
	return error;
}

// turns a comma separated list of surfaces on or off
static void NM_PreviewSurfaces( const char *list, int flags ) {
	char name[MAX_QPATH];
	const char *p = list;

	while ( *p ) {
		int len = 0;

		while ( *p == ',' || *p == ' ' || *p == '\t' )
			p++;
		while ( *p && *p != ',' ) {
			if ( len < (int)sizeof( name ) - 1 )
				name[len++] = *p;
			p++;
		}
		while ( len && ( name[len - 1] == ' ' || name[len - 1] == '\t' ) )
			len--;
		name[len] = '\0';
		if ( name[0] )
			re->G2API_SetSurfaceOnOff( *nm.previewG2, name, flags );
	}
}

// loops BOTH_STAND1 from the animation.cfg next to the model's .gla, if it has one
static void NM_PreviewAnimate( void ) {
	char path[MAX_QPATH], *buf, *slash;
	const char *gla = re->G2API_GetGLAName( *nm.previewG2, 0 ), *p, *token;

	if ( !gla || !gla[0] )
		return;
	Q_strncpyz( path, gla, sizeof( path ) );
	slash = strrchr( path, '/' );
	if ( !slash )
		return;
	slash[1] = '\0';
	Q_strcat( path, sizeof( path ), "animation.cfg" );
	if ( FS_ReadFile( path, (void **)&buf ) <= 0 || !buf )
		return;

	p = buf;
	COM_BeginParseSession( path );
	while ( 1 ) {
		int first, num;
		float fps;

		token = COM_ParseExt( &p, qtrue );
		if ( !token[0] )
			break;
		if ( Q_stricmp( token, "BOTH_STAND1" ) ) {
			SkipRestOfLine( &p );
			continue;
		}
		first = atoi( COM_ParseExt( &p, qfalse ) );
		num = atoi( COM_ParseExt( &p, qfalse ) );
		COM_ParseExt( &p, qfalse );	// loop frames
		fps = fabsf( (float)atof( COM_ParseExt( &p, qfalse ) ) );
		// the same speed cgame plays it at: 50 / ( 1000 / fps )
		if ( num > 0 && fps > 0.0f ) {
			re->G2API_SetBoneAnim( *nm.previewG2, 0, "model_root", first, first + num, BONE_ANIM_OVERRIDE_LOOP,
				fps / 20.0f, re->G2API_GetTime( cl.serverTime ), -1, -1 );
		}
		break;
	}
	FS_FreeFile( buf );
}

// what NPCs hold, as bg_misc.c's items give cgame its weapon models
static const struct {
	const char	*weapon, *model;
} nmWeaponModels[] = {
	{ "WP_STUN_BATON",		"models/weapons2/stun_baton/baton_w.glm" },
	{ "WP_BRYAR_PISTOL",	"models/weapons2/blaster_pistol/blaster_pistol_w.glm" },
	{ "WP_CONCUSSION",		"models/weapons2/concussion/c_rifle_w.glm" },
	{ "WP_BRYAR_OLD",		"models/weapons2/briar_pistol/briar_pistol_w.glm" },
	{ "WP_BLASTER",			"models/weapons2/blaster_r/blaster_w.glm" },
	{ "WP_DISRUPTOR",		"models/weapons2/disruptor/disruptor_w.glm" },
	{ "WP_BOWCASTER",		"models/weapons2/bowcaster/bowcaster_w.glm" },
	{ "WP_REPEATER",		"models/weapons2/heavy_repeater/heavy_repeater_w.glm" },
	{ "WP_DEMP2",			"models/weapons2/demp2/demp2_w.glm" },
	{ "WP_FLECHETTE",		"models/weapons2/golan_arms/golan_arms_w.glm" },
	{ "WP_ROCKET_LAUNCHER",	"models/weapons2/merr_sonn/merr_sonn_w.glm" },
	{ "WP_THERMAL",			"models/weapons2/thermal/thermal_w.glm" },
	{ "WP_TRIP_MINE",		"models/weapons2/laser_trap/laser_trap_w.glm" },
	{ "WP_DET_PACK",		"models/weapons2/detpack/det_pack_proj.glm" },
};

// a saber's hilt, from the first block with its name in ext_data/sabers/*.sab
static void NM_SaberModel( const char *saber, char *out, int size ) {
	char **files, *buf, name[MAX_QPATH], key[64];
	const char *p, *token;
	int numFiles;
	qboolean found = qfalse;

	Q_strncpyz( out, "models/weapons2/saber/saber_w.glm", size );	// DEFAULT_SABER_MODEL
	files = FS_ListFiles( "ext_data/sabers", ".sab", &numFiles );
	for ( int i = 0; i < numFiles && !found; i++ ) {
		const char *path = va( "ext_data/sabers/%s", files[i] );

		if ( FS_ReadFile( path, (void **)&buf ) <= 0 || !buf )
			continue;
		p = buf;
		COM_BeginParseSession( path );
		while ( !found ) {
			token = COM_ParseExt( &p, qtrue );
			if ( !token[0] )
				break;
			Q_strncpyz( name, token, sizeof( name ) );
			if ( Q_stricmp( COM_ParseExt( &p, qtrue ), "{" ) )
				break;
			if ( Q_stricmp( name, saber ) ) {
				SkipBracedSection( &p, 1 );
				continue;
			}
			found = qtrue;
			while ( 1 ) {
				token = COM_ParseExt( &p, qtrue );
				if ( !token[0] || !Q_stricmp( token, "}" ) )
					break;
				Q_strncpyz( key, token, sizeof( key ) );
				token = COM_ParseExt( &p, qfalse );
				if ( !Q_stricmp( key, "saberModel" ) && token[0] )
					Q_strncpyz( out, token, size );
				SkipRestOfLine( &p );
			}
		}
		FS_FreeFile( buf );
	}
	FS_FreeFileList( files );
}

// puts a weapon model in the preview's slot 1 or 2, held by the given hand bolt
static void NM_PreviewHold( const char *model, int slot, int bolt ) {
	CGhoul2Info_v **weapon = &nm.previewHeld[slot - 1];

	if ( bolt < 0 || FS_ReadFile( model, NULL ) <= 0 )
		return;
	if ( re->G2API_InitGhoul2Model( weapon, model, 0, 0, 0, 0, 0 ) >= 0 && *weapon ) {
		re->G2API_SetBoltInfo( **weapon, 0, bolt );	// bolt on model 0, the body
		re->G2API_CopySpecificG2Model( **weapon, 0, *nm.previewG2, slot );
	}
}

// the .npc's weapon, or its sabers, in the NPC's hands
static void NM_PreviewWeapons( const nmType_t *t ) {
	char saber[MAX_QPATH];

	if ( !Q_stricmp( t->weapon, "WP_SABER" ) ) {
		NM_SaberModel( t->saber[0] ? t->saber : "Kyle", saber, sizeof( saber ) );	// DEFAULT_SABER
		NM_PreviewHold( saber, 1, re->G2API_AddBolt( *nm.previewG2, 0, "*r_hand" ) );
		if ( t->saber2[0] ) {
			NM_SaberModel( t->saber2, saber, sizeof( saber ) );
			NM_PreviewHold( saber, 2, re->G2API_AddBolt( *nm.previewG2, 0, "*l_hand" ) );
		}
		return;
	}
	for ( size_t i = 0; i < ARRAY_LEN( nmWeaponModels ); i++ ) {
		if ( !Q_stricmp( t->weapon, nmWeaponModels[i].weapon ) ) {
			NM_PreviewHold( nmWeaponModels[i].model, 1, re->G2API_AddBolt( *nm.previewG2, 0, "*r_hand" ) );
			return;
		}
	}
}

// (re)builds the model when the type or the model box changed
// t's model as the spawn would send it, or pick, a model/skin from the model picker
static void NM_PreviewUpdate( const nmType_t *t, const char *pick ) {
	char key[sizeof( nm.previewKey )], name[MAX_QPATH], glm[MAX_QPATH], skinPath[MAX_QPATH];
	const char *model = nm.fields[NM_F_MODEL], *skin;
	char *slash;
	qhandle_t hSkin;

	if ( pick ) {
		model = pick;
		nm.previewOwnModel = qfalse;
	} else if ( !t ) {
		NM_PreviewFree();
		nm.previewError = "Pick an NPC type on the left";
		return;
	} else {
		// what the spawn sends: vehicles always use their own
		nm.previewOwnModel = (qboolean)( t->vehicle || !model[0] );
		if ( nm.previewOwnModel )
			model = t->model[0] ? t->model : t->name;
	}
	Com_sprintf( key, sizeof( key ), "%s|%s", t ? t->name : "", model );
	if ( !Q_stricmp( key, nm.previewKey ) )
		return;

	NM_PreviewFree();
	Q_strncpyz( nm.previewKey, key, sizeof( nm.previewKey ) );
	Q_strncpyz( nm.previewModel, model, sizeof( nm.previewModel ) );
	nm.previewError = NULL;
	nm.previewSkinMissing = qfalse;

	// "model" or "model/skin"
	Q_strncpyz( name, model, sizeof( name ) );
	slash = strchr( name, '/' );
	skin = "default";
	if ( slash ) {
		*slash = '\0';
		if ( slash[1] )
			skin = slash + 1;
	}

	Com_sprintf( glm, sizeof( glm ), "models/players/%s/model.glm", name );
	nm.previewError = CL_GlmBounds( glm, nm.previewMins, nm.previewMaxs );
	if ( nm.previewError )
		return;

	if ( strchr( skin, '|' ) ) {
		Com_sprintf( skinPath, sizeof( skinPath ), "models/players/%s/|%s", name, skin );
	} else {
		Com_sprintf( skinPath, sizeof( skinPath ), "models/players/%s/model_%s.skin", name, skin );
		if ( FS_ReadFile( skinPath, NULL ) <= 0 ) {
			nm.previewSkinMissing = qtrue;
			Com_sprintf( skinPath, sizeof( skinPath ), "models/players/%s/model_default.skin", name );
		}
	}
	hSkin = re->RegisterSkin( skinPath );

	if ( re->G2API_InitGhoul2Model( &nm.previewG2, glm, 0, hSkin, 0, 0, 0 ) < 0 || !nm.previewG2 ) {
		NM_PreviewFree();
		Q_strncpyz( nm.previewKey, key, sizeof( nm.previewKey ) );	// don't retry every frame
		nm.previewError = "Couldn't load the model";
		return;
	}
	re->G2API_SetSkin( *nm.previewG2, 0, hSkin, hSkin );
	// the .npc's surfaces are for its own model
	if ( nm.previewOwnModel ) {
		NM_PreviewSurfaces( t->surfOff, G2SURFACEFLAG_OFF );
		NM_PreviewSurfaces( t->surfOn, 0 );
	}
	NM_PreviewAnimate();
	if ( t && !t->vehicle )
		NM_PreviewWeapons( t );
}

static void NM_RenderPreview( const nmType_t *t, float x, float y, float w, float h ) {
	refdef_t refdef;
	refEntity_t ent;
	vec3_t mins, maxs, center, forward, angles;
	float s = NM_PreviewScale( t ) / 100.0f, lift, xScale, yScale, fovX, fovY, halfH, radius, dist;

	// as cgame does: scaled characters are lifted so their feet stay on the ground
	lift = t->vehicle ? 0.0f : 24.0f * ( s - 1.0f );

	// frame the bigger of the scaled and unscaled model, so a smaller NPC shows smaller
	for ( int i = 0; i < 3; i++ ) {
		float off = i == 2 ? lift : 0.0f;

		mins[i] = Q_min( nm.previewMins[i], nm.previewMins[i] * s + off );
		maxs[i] = Q_max( nm.previewMaxs[i], nm.previewMaxs[i] * s + off );
	}
	// it turns around its origin
	radius = sqrtf( Square( Q_max( fabsf( mins[0] ), fabsf( maxs[0] ) ) ) + Square( Q_max( fabsf( mins[1] ), fabsf( maxs[1] ) ) ) );
	halfH = ( maxs[2] - mins[2] ) * 0.5f;
	VectorSet( center, 0, 0, ( mins[2] + maxs[2] ) * 0.5f );

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
	ent.ghoul2 = nm.previewG2;
	ent.renderfx = RF_NOSHADOW;
	VectorSet( ent.angles, 0, (float)( cls.realtime % 12000 ) * 0.03f + nm.previewYaw, 0 );	// a turn every 12 seconds
	AnglesToAxis( ent.angles, ent.axis );
	ent.origin[2] = lift;
	if ( s != 1.0f )
		VectorSet( ent.modelScale, s, s, s );
	memcpy( ent.shaderRGBA, t->rgba, sizeof( ent.shaderRGBA ) );
	ent.radius = radius + halfH;

	re->ClearScene();
	re->AddRefEntityToScene( &ent );
	re->RenderScene( &refdef );
}

// what the manage commands are run on, NULL if it can't be. Not va(): the
// manage tab keeps both answers across a frame of drawing
static const char *NM_ApplyTarget( qboolean allowAll ) {
	static char target[2][MAX_QPATH];
	const nmNpc_t *npc = NM_SelectedNpc();
	char *out = target[allowAll ? 1 : 0];

	switch ( nm.apply ) {
	case NM_APPLY_MARKED:
		return NULL;
	case NM_APPLY_ALL:
		return allowAll ? "all" : NULL;
	case NM_APPLY_NAME:
		if ( !npc || !npc->name[0] )
			return NULL;
		Q_strncpyz( out, npc->name, sizeof( target[0] ) );
		return out;
	default:
		if ( !npc )
			return NULL;
		Com_sprintf( out, sizeof( target[0] ), "%i", npc->num );
		return out;
	}
}

static void NM_SendManage( const char *cmd, qboolean refresh ) {
	NM_Send( cmd, qtrue );
	if ( refresh )
		NM_RefreshSoon();
}

// the name it talks as when none is typed: its targetname, or its type
static const char *NM_TalkName( const nmNpc_t *npc ) {
	if ( nm.fields[NM_F_TALK_NAME][0] )
		return nm.fields[NM_F_TALK_NAME];
	return npc ? ( npc->name[0] ? npc->name : npc->type ) : "";
}

// the selection says it in chat, as the storyteller's NPC talk step does
static void NM_Talk( void ) {
	const nmNpc_t *npc = NM_SelectedNpc();
	const char *target = NM_ApplyTarget( qfalse ), *emote = nm.talkEmote ? "emtalk" : "";

	if ( !target || !nm.fields[NM_F_TALK_TEXT][0] || !CL_RpHooks_Experimental() )
		return;
	NM_SendManage( CL_RpHooks_NpcTalkCommand( target, NM_TalkName( npc ), va( "^%c", nmTalkColors[nm.talkColor] ), emote, nm.fields[NM_F_TALK_TEXT] ), qfalse );
	// a story's NPC talk step plays the emote itself; it waits its turn behind the line
	if ( emote[0] && !CL_Storyteller_Picking() )
		NM_Queue( va( "npc emote %s %s", emote, target ) );
	nm.fields[NM_F_TALK_TEXT][0] = '\0';
}

// npcmanager select: what you say in chat said by an NPC instead, until unselected or the map
// changes (its NPCs gone with it)
static struct {
	char		target[MAX_QPATH];		// its targetname or entity number, "" for none
	char		name[MAX_QPATH];		// what it's called in chat, colour codes and all
	char		map[MAX_QPATH];			// cl.mapname it was chosen in
} nmSpeak;

// talking through an NPC is NPC talk: an admin's, with experimental features on
static qboolean NM_TalkAllowed( void ) {
	if ( cls.state != CA_ACTIVE ) {
		Com_Printf( "NPC manager: not in game\n" );
		return qfalse;
	}
	if ( !CL_RpAdmin_Allow( "NPC manager", NM_TOGGLE_CMD ) )
		return qfalse;
	if ( !CL_RpHooks_Experimental() ) {
		Com_Printf( S_COLOR_YELLOW "NPC talk is experimental: press TOGGLE EXPERIMENTAL in the npcmanager's Manage tab first\n" );
		return qfalse;
	}
	return qtrue;
}

// the name an NPC talks as when none is given: for an entity number, its targetname or type
// if the list has it
static const char *NM_DefaultTalkName( const char *target ) {
	if ( Q_isanumber( target ) ) {
		const nmNpc_t *npc = NM_FindNpc( atoi( target ) );

		if ( npc )
			return npc->name[0] ? npc->name : npc->type;
	}
	return target;
}

static void NM_SpeakOff( const char *why ) {
	if ( !nmSpeak.target[0] )
		return;
	Com_Printf( "NPC manager: you talk as yourself again%s\n", why );
	nmSpeak.target[0] = '\0';
}

// a command on its way to the server: say, with an NPC selected, is that NPC talking
const char *CL_NpcManager_OutgoingChat( const char *cmd ) {
	char text[MAX_STRING_CHARS];
	const char *said;
	int len;

	// a bind or a script can leave spaces before it
	while ( *cmd == ' ' )
		cmd++;
	if ( !nmSpeak.target[0] || Q_stricmpn( cmd, "say ", 4 ) )
		return cmd;
	if ( strcmp( nmSpeak.map, cl.mapname ) ) {
		NM_SpeakOff( " (the map changed)" );
		return cmd;
	}
	// the console's say may have it in quotes, the chat box's doesn't
	for ( said = cmd + 4; *said == ' '; )
		said++;
	Q_strncpyz( text, said, sizeof( text ) );
	len = strlen( text );
	if ( len >= 2 && text[0] == '"' && text[len - 1] == '"' ) {
		text[len - 1] = '\0';
		memmove( text, text + 1, len - 1 );
	}
	if ( !text[0] )
		return cmd;
	return CL_RpHooks_NpcTalkCommand( nmSpeak.target, nmSpeak.name, "", "", text );
}

// who walks: the selection, or the marked ones by number, commas between; NULL if none
static const char *NM_WalkTarget( void ) {
	static char target[MAX_STRING_CHARS];

	if ( nm.apply != NM_APPLY_MARKED )
		return NM_ApplyTarget( qfalse );
	target[0] = '\0';
	for ( int i = 0; i < nm.numMarked; i++ ) {
		if ( NM_FindNpc( nm.marked[i] ) )
			Q_strcat( target, sizeof( target ), va( target[0] ? ",%i" : "%i", nm.marked[i] ) );
	}
	return target[0] ? target : NULL;
}

// where the marked ones stand, walking together; how many, 0 when it isn't them who walk
static int NM_GroupStarts( vec3_t *starts ) {
	int count = 0;

	if ( nm.apply != NM_APPLY_MARKED )
		return 0;
	for ( int i = 0; i < nm.numMarked; i++ ) {
		const nmNpc_t *npc = NM_FindNpc( nm.marked[i] );

		if ( npc )
			NM_NpcOrigin( npc, starts[count++] );
	}
	return count;
}

// the selection walks a saved route; opened from the storyteller, it goes in the story
static void NM_Walk( const char *route ) {
	const char *target = NM_WalkTarget(), *cmd;

	if ( !target || !route[0] )
		return;
	cmd = va( "npcmanager walk %s %s %s%s", target, route, nm.routeRun ? "run" : "walk", nm.routeLoop ? " loop" : "" );
	if ( CL_Storyteller_Capture( cmd ) )
		return;
	Com_Printf( S_COLOR_GREEN "NPC manager: " S_COLOR_WHITE "%s\n", cmd );
	Cbuf_AddText( va( "%s\n", cmd ) );
}

static void NM_Teleport( const vec3_t pos, int yaw ) {
	const char *target = NM_ApplyTarget( qfalse );

	if ( !target )
		return;
	NM_SendManage( va( "npc tele %i %i %i %i %s", (int)floorf( pos[0] + 0.5f ), (int)floorf( pos[1] + 0.5f ), (int)floorf( pos[2] + 0.5f ), yaw, target ), qtrue );
}

static void NM_SetTeleFields( const vec3_t pos, int yaw ) {
	for ( int i = 0; i < 3; i++ )
		Com_sprintf( nm.fields[NM_F_TELE_X + i], NM_FIELD_LEN, "%i", (int)floorf( pos[i] + 0.5f ) );
	Com_sprintf( nm.fields[NM_F_TELE_YAW], NM_FIELD_LEN, "%i", NM_NormalizeYaw( (float)yaw ) );
}

// the box to type where to teleport to, starting from where the selected NPC is
static void NM_OpenTeleBox( void ) {
	const nmNpc_t *npc = NM_SelectedNpc();
	vec3_t org;

	if ( npc ) {
		NM_NpcOrigin( npc, org );
		NM_SetTeleFields( org, npc->yaw );
	}
	nm.teleBox = qtrue;
	nm.focus = NM_F_TELE_X;
}

static void NM_TeleportToFields( void ) {
	vec3_t pos;

	for ( int i = 0; i < 3; i++ ) {
		if ( !nm.fields[NM_F_TELE_X + i][0] ) {
			NM_Message( S_COLOR_YELLOW "Fill in X, Y and Z" );
			return;
		}
		pos[i] = (float)atof( nm.fields[NM_F_TELE_X + i] );
	}
	NM_Teleport( pos, NM_NormalizeYaw( (float)atof( nm.fields[NM_F_TELE_YAW] ) ) );
	nm.teleBox = qfalse;
	nm.focus = NM_F_NONE;
}

/*
===============================================================================

SAVED NPCS AND CONSTRUCTS

Plain .cfg files in the game folder. Exec'd, the first line hands the file to the
manager, which reads it whole; the lines after it do nothing on their own.

===============================================================================
*/

static qboolean NM_SaveNameChar( int ch ) {
	return (qboolean)( ( ch >= 'a' && ch <= 'z' ) || ( ch >= 'A' && ch <= 'Z' ) || ( ch >= '0' && ch <= '9' ) || ch == '_' || ch == '-' );
}

static const char *NM_SavePath( qboolean construct, const char *name ) {
	return va( "%s/%s%s", construct ? NM_CONSTRUCT_DIR : NM_NPC_DIR, name, NM_SAVE_EXT );
}

static void NM_WriteLine( fileHandle_t f, const char *text ) {
	FS_Write( text, strlen( text ), f );
}

// splits a line into words, "quoted" ones whole; the line is cut up in place
static int NM_SplitLine( char *line, char **argv, int max ) {
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

typedef qboolean ( *nmLineParser_t )( char **argv, int argc, void *out );

// the lines of a saved file parse turns into entries of out, each size big; -1 if there's no file
static int NM_ReadLines( const char *path, nmLineParser_t parse, void *out, size_t size, int max ) {
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
		argc = NM_SplitLine( line, argv, ARRAY_LEN( argv ) );
		if ( parse( argv, argc, (byte *)out + count * size ) )
			count++;
	}
	FS_FreeFile( buf );
	return count;
}

// the six words of a spawn: <type> <vehicle> <targetname> <team> <model> <scale>, "-" for none
static const char *NM_SpawnArgs( const nmSpawn_t *s ) {
	return va( "%s %i %s %s %s %s", s->type, s->vehicle ? 1 : 0, s->targetname[0] ? s->targetname : "-",
		nmTeamArgs[s->team], s->model[0] ? s->model : "-", s->scale[0] ? s->scale : "-" );
}

static void NM_ParseSpawnArgs( char **argv, nmSpawn_t *out ) {
	memset( out, 0, sizeof( *out ) );
	Q_strncpyz( out->type, argv[0], sizeof( out->type ) );
	out->vehicle = (qboolean)( atoi( argv[1] ) != 0 );
	Q_strncpyz( out->targetname, strcmp( argv[2], "-" ) ? argv[2] : "", sizeof( out->targetname ) );
	for ( int i = 0; i < (int)ARRAY_LEN( nmTeamArgs ); i++ ) {
		if ( !Q_stricmp( argv[3], nmTeamArgs[i] ) )
			out->team = (nmTeam_t)i;
	}
	Q_strncpyz( out->model, strcmp( argv[4], "-" ) ? argv[4] : "", sizeof( out->model ) );
	Q_strncpyz( out->scale, strcmp( argv[5], "-" ) ? argv[5] : "", sizeof( out->scale ) );
}

static qboolean NM_ParseSavedNpc( char **argv, int argc, void *out ) {
	// npcmanager savednpc <six words>
	if ( argc < 8 || Q_stricmp( argv[0], NM_TOGGLE_CMD ) || Q_stricmp( argv[1], "savednpc" ) )
		return qfalse;
	NM_ParseSpawnArgs( argv + 2, (nmSpawn_t *)out );
	return qtrue;
}

static qboolean NM_ParseConstructNpc( char **argv, int argc, void *to ) {
	nmSpawn_t *out = (nmSpawn_t *)to;

	// npcmanager constructnpc <six words> <x y z from the middle> <yaw>
	if ( argc < 12 || Q_stricmp( argv[0], NM_TOGGLE_CMD ) || Q_stricmp( argv[1], "constructnpc" ) )
		return qfalse;
	NM_ParseSpawnArgs( argv + 2, out );
	VectorSet( out->pos, atof( argv[8] ), atof( argv[9] ), atof( argv[10] ) );
	out->yaw = NM_NormalizeYaw( atof( argv[11] ) );
	return qtrue;
}

static int NM_ReadConstruct( const char *name, nmSpawn_t *out ) {
	return NM_ReadLines( NM_SavePath( qtrue, name ), NM_ParseConstructNpc, out, sizeof( *out ), NM_MAX_PLACED );
}

static int QDECL NM_CompareSaved( const void *a, const void *b ) {
	return Q_stricmp( ( (const nmSaved_t *)a )->name, ( (const nmSaved_t *)b )->name );
}

static void NM_RefreshSaved( void ) {
	nmSpawn_t *scratch = (nmSpawn_t *)Z_Malloc( sizeof( nmSpawn_t ) * NM_MAX_PLACED, TAG_TEMP_WORKSPACE, qfalse );

	nm.numSaved = 0;
	for ( int construct = 0; construct < 2; construct++ ) {
		char **list;
		int count;

		list = FS_ListFiles( construct ? NM_CONSTRUCT_DIR : NM_NPC_DIR, NM_SAVE_EXT, &count );
		for ( int i = 0; i < count && nm.numSaved < NM_MAX_SAVED; i++ ) {
			nmSaved_t *s = &nm.saved[nm.numSaved];

			memset( s, 0, sizeof( *s ) );
			Q_strncpyz( s->name, list[i], sizeof( s->name ) );
			if ( strlen( s->name ) > strlen( NM_SAVE_EXT ) )
				s->name[strlen( s->name ) - strlen( NM_SAVE_EXT )] = '\0';
			s->construct = (qboolean)construct;
			if ( construct )
				s->count = NM_ReadConstruct( s->name, scratch );
			else if ( NM_ReadLines( NM_SavePath( qfalse, s->name ), NM_ParseSavedNpc, &s->npc, sizeof( s->npc ), 1 ) < 1 )
				continue;	// not one of ours
			nm.numSaved++;
		}
		FS_FreeFileList( list );
	}
	Z_Free( scratch );
	qsort( nm.saved, nm.numSaved, sizeof( nm.saved[0] ), NM_CompareSaved );
	// appliedSaved stays: it's by name, and the form still holds it
	nm.constructShown[0] = '\0';
	// the Saved row's count
	if ( nm.indexed )
		NM_BuildRows();
}

// the selected construct's NPCs, read once per construct
static void NM_ReadShownConstruct( const nmSaved_t *s ) {
	if ( !Q_stricmp( nm.constructShown, s->name ) )
		return;
	Q_strncpyz( nm.constructShown, s->name, sizeof( nm.constructShown ) );
	nm.numConstructNpcs = Q_max( 0, NM_ReadConstruct( s->name, nm.constructNpcs ) );
}

static int NM_FindSaved( qboolean construct, const char *name ) {
	for ( int i = 0; i < nm.numSaved; i++ ) {
		if ( nm.saved[i].construct == construct && !Q_stricmp( nm.saved[i].name, name ) )
			return i;
	}
	return -1;
}

// a saved NPC's settings go in the form while its row is selected; leaving it puts back what was there
static void NM_ApplySelection( void ) {
	const nmSaved_t *s = NM_SelectedSaved();
	const char *name = s && !s->construct ? s->name : "";

	if ( !Q_stricmp( name, nm.appliedSaved ) )
		return;
	if ( nm.appliedSaved[0] ) {
		Q_strncpyz( nm.fields[NM_F_TARGETNAME], nm.formBefore[0], NM_FIELD_LEN );
		Q_strncpyz( nm.fields[NM_F_MODEL], nm.formBefore[1], NM_FIELD_LEN );
		Q_strncpyz( nm.fields[NM_F_SCALE], nm.formBefore[2], NM_FIELD_LEN );
		nm.team = nm.teamBefore;
	}
	if ( name[0] ) {
		Q_strncpyz( nm.formBefore[0], nm.fields[NM_F_TARGETNAME], NM_FIELD_LEN );
		Q_strncpyz( nm.formBefore[1], nm.fields[NM_F_MODEL], NM_FIELD_LEN );
		Q_strncpyz( nm.formBefore[2], nm.fields[NM_F_SCALE], NM_FIELD_LEN );
		nm.teamBefore = nm.team;
		Q_strncpyz( nm.fields[NM_F_TARGETNAME], s->npc.targetname, NM_FIELD_LEN );
		Q_strncpyz( nm.fields[NM_F_MODEL], s->npc.model, NM_FIELD_LEN );
		Q_strncpyz( nm.fields[NM_F_SCALE], s->npc.scale, NM_FIELD_LEN );
		nm.team = s->npc.team;
	}
	Q_strncpyz( nm.appliedSaved, name, sizeof( nm.appliedSaved ) );
}

// shows a saved NPC or construct in the list, selected
static void NM_SelectSaved( qboolean construct, const char *name ) {
	const int index = NM_FindSaved( construct, name );

	if ( index < 0 )
		return;
	// on the Saved row, with nothing hiding it
	nm.show = NM_SHOW_ALL;
	NM_BuildRows();
	NM_SetRow( 0 );
	NM_RebuildTypeView();
	for ( int i = 0; i < nm.numTypeView; i++ ) {
		if ( nm.typeView[i] == -1 - index ) {
			nm.typeList.sel = i;
			NM_ListMove( &nm.typeList, nm.numTypeView, (int)( NM_TYPE_LIST_H / NM_ROW_H ), 0 );
			break;
		}
	}
	NM_ApplySelection();
}

static qboolean NM_SaveNpc( const char *name ) {
	const char *path;
	fileHandle_t f;
	nmSpawn_t s;

	if ( !NM_FormSpawn( &s ) )
		return qfalse;
	path = NM_SavePath( qfalse, name );
	f = FS_FOpenFileWrite( path );
	if ( !f ) {
		NM_Message( va( S_COLOR_RED "Couldn't write %s", path ) );
		return qfalse;
	}
	NM_WriteLine( f, va( "// NPC manager saved NPC: %s\n", name ) );
	NM_WriteLine( f, "// Shown under Saved in the NPC manager's spawn tab; exec this file to put it in the form\n" );
	NM_WriteLine( f, "// savednpc <type> <vehicle> <targetname> <team> <model or model/skin> <scale>, - for none\n" );
	NM_WriteLine( f, va( "npcmanager loadnpc %s\n", name ) );
	NM_WriteLine( f, va( "npcmanager savednpc %s\n", NM_SpawnArgs( &s ) ) );
	FS_FCloseFile( f );
	NM_RefreshSaved();
	NM_Message( va( S_COLOR_GREEN "Saved %s as %s", s.type, NM_SavePath( qfalse, name ) ) );
	return qtrue;
}

// what was put down, relative to the middle of its floor
static qboolean NM_SaveConstruct( const char *name ) {
	vec3_t mins, maxs, anchor;
	const char *path;
	fileHandle_t f;

	if ( !nm.numPlaced )
		return qfalse;
	ClearBounds( mins, maxs );
	for ( int i = 0; i < nm.numPlaced; i++ )
		AddPointToBounds( nm.placed[i].pos, mins, maxs );
	VectorSet( anchor, floorf( ( mins[0] + maxs[0] ) * 0.5f + 0.5f ), floorf( ( mins[1] + maxs[1] ) * 0.5f + 0.5f ), floorf( mins[2] + 0.5f ) );

	path = NM_SavePath( qtrue, name );
	f = FS_FOpenFileWrite( path );
	if ( !f ) {
		NM_Message( va( S_COLOR_RED "Couldn't write %s", path ) );
		return qfalse;
	}
	NM_WriteLine( f, va( "// NPC manager construct: %i NPCs, made on %s\n", nm.numPlaced,
		Info_ValueForKey( cl.gameState.stringData + cl.gameState.stringOffsets[CS_SERVERINFO], "mapname" ) ) );
	NM_WriteLine( f, "// Shown under Saved in the NPC manager's spawn tab; exec this file to pick it up and put it down\n" );
	NM_WriteLine( f, "// constructnpc <type> <vehicle> <targetname> <team> <model> <scale> <x y z from the middle> <yaw>\n" );
	NM_WriteLine( f, va( "npcmanager loadconstruct %s\n", name ) );
	for ( int i = 0; i < nm.numPlaced; i++ ) {
		const nmSpawn_t *s = &nm.placed[i];

		NM_WriteLine( f, va( "npcmanager constructnpc %s %i %i %i %i\n", NM_SpawnArgs( s ),
			(int)floorf( s->pos[0] - anchor[0] + 0.5f ), (int)floorf( s->pos[1] - anchor[1] + 0.5f ), (int)floorf( s->pos[2] - anchor[2] + 0.5f ), s->yaw ) );
	}
	FS_FCloseFile( f );
	NM_RefreshSaved();
	NM_Message( va( S_COLOR_GREEN "Saved %i NPCs as %s", nm.numPlaced, NM_SavePath( qtrue, name ) ) );
	return qtrue;
}

static void NM_StartSaving( nmSaving_t saving ) {
	const nmType_t *t = NM_SelectedType();
	const char *offer = "";

	if ( saving == NM_SAVING_NPC ) {
		if ( !t )
			return;
		offer = nm.fields[NM_F_TARGETNAME][0] ? nm.fields[NM_F_TARGETNAME] : t->name;
	} else if ( saving == NM_SAVING_ROUTE ) {
		if ( !nm.numRoute )
			return;
		offer = nm.routeName[0] ? nm.routeName : "route";
	} else if ( !nm.numPlaced ) {
		return;
	}
	Q_strncpyz( nm.fields[NM_F_SAVE_NAME], offer, NM_FIELD_LEN );
	for ( char *c = nm.fields[NM_F_SAVE_NAME]; *c; c++ ) {
		if ( !NM_SaveNameChar( *c ) )
			*c = '_';
	}
	nm.saving = saving;
	nm.focus = NM_F_NONE;
}

static void NM_DoSave( void ) {
	const char *name = nm.fields[NM_F_SAVE_NAME];

	if ( nm.saving == NM_SAVING_ROUTE ) {
		if ( !name[0] || !CL_NpcWalk_SaveRoute( name, nm.route, nm.numRoute, nm.routeRun ? NM_RUN_SPEED : NM_WALK_SPEED ) )
			return;
		Q_strncpyz( nm.routeName, name, sizeof( nm.routeName ) );
		nm.saving = NM_SAVING_NONE;
		NM_Message( va( S_COLOR_GREEN "Saved the route as %s/%s.route", NM_ROUTE_DIR, name ) );
		NM_RefreshRoutes();
		if ( nm.routeSaveOnly ) {
			// selected in the Routes tab, to give to NPCs
			for ( int i = 0; i < nm.numRoutes; i++ ) {
				if ( !Q_stricmp( nm.routes[i].name, name ) )
					NM_ListMove( &nm.routeList, nm.numRoutes, (int)( NM_ROUTE_LIST_H / NM_ROW_H ), i - nm.routeList.sel );
			}
			return;
		}
		NM_Walk( name );
		return;
	}

	if ( name[0] && ( nm.saving == NM_SAVING_NPC ? NM_SaveNpc( name ) : NM_SaveConstruct( name ) ) )
		nm.saving = NM_SAVING_NONE;
}

/*
===============================================================================

STATE CHANGES

===============================================================================
*/

// the saved routes, for the Routes tab
static void NM_RefreshRoutes( void ) {
	char **files;
	int count;

	files = FS_ListFiles( NM_ROUTE_DIR, ".route", &count );
	nm.numRoutes = 0;
	for ( int i = 0; i < count && nm.numRoutes < NM_MAX_ROUTES; i++ ) {
		char name[MAX_QPATH];
		int n;

		COM_StripExtension( files[i], name, sizeof( name ) );
		n = CL_NpcWalk_LoadRoute( name, nm.routes[nm.numRoutes].pts, NW_MAX_ROUTE_POINTS, &nm.routes[nm.numRoutes].speed );
		if ( n < 0 )
			continue;
		Q_strncpyz( nm.routes[nm.numRoutes].name, name, sizeof( nm.routes[0].name ) );
		nm.routes[nm.numRoutes].numPts = n;
		nm.numRoutes++;
	}
	FS_FreeFileList( files );
	NM_ListClamp( &nm.routeList, nm.numRoutes, (int)( NM_ROUTE_LIST_H / NM_ROW_H ) );
}

// the saved route selected in the Routes tab, -1 none
static int NM_SelectedRoute( void ) {
	return nm.routeList.sel < nm.numRoutes ? nm.routeList.sel : -1;
}

// the walks going on, as they are this frame
static void NM_ReadWalks( void ) {
	nm.numWalks = 0;
	while ( nm.numWalks < NM_MAX_WALKS && CL_NpcWalk_GetWalk( nm.numWalks, &nm.walks[nm.numWalks] ) )
		nm.numWalks++;
}

static const npcWalkInfo_t *NM_FindWalk( int num ) {
	for ( int i = 0; i < nm.numWalks; i++ ) {
		if ( nm.walks[i].num == num )
			return &nm.walks[i];
	}
	return NULL;
}

static void NM_SetTab( nmTab_t tab ) {
	nm.tab = tab;
	nm.focus = NM_F_NONE;
	nm.teleBox = qfalse;
	if ( tab != NM_TAB_SPAWN )
		NM_ClosePreview();
	nm.confirm = NM_CONFIRM_NONE;
	if ( tab == NM_TAB_ROUTES ) {
		// a route is walked by one, those named alike or the marked ones, not all
		if ( nm.apply == NM_APPLY_ALL )
			nm.apply = NM_APPLY_ONE;
		NM_RefreshRoutes();
	}
	if ( tab == NM_TAB_MANAGE || tab == NM_TAB_ROUTES )
		NM_RequestList();
	NM_RebuildNpcView();
}

static void NM_OpenPicker( nmPicker_t picker ) {
	nm.picker = picker;
	nm.previewYaw = 0.0f;
	nm.fields[NM_F_PICKER_SEARCH][0] = '\0';
	nm.pickerList.sel = nm.pickerList.scroll = 0;
	NM_RebuildPickerView();
}

static void NM_ChoosePicker( int index ) {
	const char *item = NM_PickerItem( index ), *target;

	switch ( nm.picker ) {
	case NM_PICKER_MODEL:
		Q_strncpyz( nm.fields[NM_F_MODEL], item, sizeof( nm.fields[NM_F_MODEL] ) );
		break;
	case NM_PICKER_EMOTE:
		Q_strncpyz( nm.fields[NM_F_EMOTE], item, sizeof( nm.fields[NM_F_EMOTE] ) );
		target = NM_ApplyTarget( qtrue );
		if ( target )
			NM_SendManage( va( "npc emote %s %s", item, target ), qfalse );
		break;
	case NM_PICKER_PLAYER:
		target = NM_ApplyTarget( qtrue );
		if ( target )
			NM_SendManage( va( "npc follow %i %s", index, target ), qfalse );
		break;
	default:
		break;
	}
	nm.picker = NM_PICKER_NONE;
}

static void NM_ReleaseKeys( void ) {
	memset( nm.held, 0, sizeof( nm.held ) );
}

// fly from where the player looks; the crosshair in the middle is what gets picked
static void NM_StartPick( nmPick_t pick ) {
	nm.pick = pick;
	nm.pickYaw = 0;
	nm.focus = NM_F_NONE;
	if ( nm.haveView ) {
		VectorCopy( nm.viewOrg, nm.camOrg );
		vectoangles( nm.viewAxis[0], nm.camAng );
	} else {
		VectorCopy( cl.snap.ps.origin, nm.camOrg );
		nm.camOrg[2] += cl.snap.ps.viewheight;
		VectorCopy( cl.viewangles, nm.camAng );
	}
	if ( nm.camAng[PITCH] > 180.0f )
		nm.camAng[PITCH] -= 360.0f;
	nm.camAng[ROLL] = 0;
	nm.lastFrameTime = cls.realtime;
	nm.pickCursorX = nm.cursorX;
	nm.pickCursorY = nm.cursorY;
	nm.cursorX = SCREEN_WIDTH * 0.5f;
	nm.cursorY = SCREEN_HEIGHT * 0.5f;
	NM_ReleaseKeys();
}

// back to the panel, the cursor where it was
static void NM_EndPick( void ) {
	if ( nm.pick == NM_PICK_NONE )
		return;
	nm.pick = NM_PICK_NONE;
	nm.cursorX = nm.pickCursorX;
	nm.cursorY = nm.pickCursorY;
	NM_ReleaseKeys();
}

// the form's NPC in hand, to put down as many as wanted
static void NM_StartPlace( void ) {
	if ( !NM_FormSpawn( &nm.hand[0] ) )
		return;
	VectorClear( nm.hand[0].pos );
	nm.numHand = 1;
	nm.handConstruct = qfalse;
	Q_strncpyz( nm.handName, nm.hand[0].type, sizeof( nm.handName ) );
	NM_StartPick( NM_PICK_PLACE );
}

// a saved construct in hand
static void NM_StartConstruct( const char *name ) {
	int count = NM_ReadConstruct( name, nm.hand );

	if ( count <= 0 ) {
		NM_Message( va( S_COLOR_YELLOW "%s %s", count < 0 ? "No construct" : "No NPCs in", NM_SavePath( qtrue, name ) ) );
		return;
	}
	nm.numHand = count;
	nm.handConstruct = qtrue;
	Q_strncpyz( nm.handName, name, sizeof( nm.handName ) );
	NM_StartPick( NM_PICK_PLACE );
}

// the Routes tab: a new route, or the selected one's spots to change, saved without walking it
static void NM_StartRouteEdit( int route ) {
	nm.numRoute = 0;
	nm.routeName[0] = '\0';
	if ( route >= 0 && route < nm.numRoutes ) {
		nm.numRoute = nm.routes[route].numPts;
		memcpy( nm.route, nm.routes[route].pts, nm.numRoute * sizeof( nm.route[0] ) );
		Q_strncpyz( nm.routeName, nm.routes[route].name, sizeof( nm.routeName ) );
		nm.routeRun = (qboolean)( nm.routes[route].speed > ( NM_WALK_SPEED + NM_RUN_SPEED ) * 0.5f );
	}
	nm.routeSaveOnly = qtrue;
	NM_StartPick( NM_PICK_ROUTE );
}

static void NM_DeleteRoute( int route ) {
	if ( route < 0 || route >= nm.numRoutes )
		return;
	FS_HomeRemove( va( "%s/%s.route", NM_ROUTE_DIR, nm.routes[route].name ) );
	NM_Message( va( "Deleted the route %s", nm.routes[route].name ) );
	if ( !Q_stricmp( nm.routeName, nm.routes[route].name ) )
		nm.routeName[0] = '\0';
	NM_RefreshRoutes();
}

// npcmanager walk ... stop for these, or into the story
static void NM_StopWalks( const char *target ) {
	const char *cmd;

	if ( !target || !target[0] )
		return;
	cmd = va( "npcmanager walk %s stop", target );
	if ( !CL_Storyteller_Capture( cmd ) )
		Cbuf_AddText( va( "%s\n", cmd ) );
}

// where an NPC in hand goes with the hand at anchor: one alone faces the camera, a
// construct turns about its middle by the wheel
static void NM_HandSpot( int i, const vec3_t anchor, vec3_t pos, int *yaw ) {
	const nmSpawn_t *h = &nm.hand[i];
	float a, c, s;

	if ( !nm.handConstruct ) {
		VectorCopy( anchor, pos );
		*yaw = NM_NormalizeYaw( NM_YawToView( anchor ) + nm.pickYaw );
		return;
	}
	a = DEG2RAD( (float)nm.pickYaw );
	c = cosf( a );
	s = sinf( a );
	pos[0] = anchor[0] + h->pos[0] * c - h->pos[1] * s;
	pos[1] = anchor[1] + h->pos[0] * s + h->pos[1] * c;
	pos[2] = anchor[2] + h->pos[2];
	*yaw = NM_NormalizeYaw( (float)( h->yaw + nm.pickYaw ) );
}

// what is in hand goes down where anchor is, waiting to be spawned
static void NM_PutDown( const vec3_t anchor ) {
	for ( int i = 0; i < nm.numHand; i++ ) {
		nmSpawn_t *p;

		if ( nm.numPlaced >= NM_MAX_PLACED ) {
			NM_Message( S_COLOR_YELLOW "That's as many as can wait at once; spawn them first" );
			break;
		}
		p = &nm.placed[nm.numPlaced++];
		*p = nm.hand[i];
		NM_HandSpot( i, anchor, p->pos, &p->yaw );
	}
}

static qboolean NM_Open( nmTab_t tab ) {
	if ( cls.state != CA_ACTIVE || !cls.cgameStarted ) {
		Com_Printf( "NPC manager: join a server first\n" );
		return qfalse;
	}
	// for RPMod's admins: whoever else can't run what it makes
	if ( !CL_RpAdmin_Allow( "NPC manager", "npcmanager" ) )
		return qfalse;
	CL_ModelManager_Close();
	CL_ShaderManager_Close();
	CL_EffectManager_Close();
	CL_MapEditor_Close();
	CL_Storyteller_Close();
	CL_RpCinematic_EditorClose();
	CL_SoundManager_Close();
	if ( !nm.indexed )
		NM_BuildIndex();

	nm.font = re->RegisterFont( "arialnb" );
	if ( !nm.font )
		nm.font = cls.menuFont;

	nm.open = qtrue;
	nm.pick = NM_PICK_NONE;
	nm.picker = NM_PICKER_NONE;
	nm.saving = NM_SAVING_NONE;
	nm.preview = qfalse;
	nm.cursorX = SCREEN_WIDTH * 0.5f;
	nm.cursorY = SCREEN_HEIGHT * 0.5f;
	nm.click = qfalse;
	nm.wheel = 0;
	NM_RefreshSaved();
	NM_RebuildTypeView();
	NM_SetTab( tab );
	if ( tab != NM_TAB_MANAGE )
		NM_RequestList();	// for the map labels
	Key_SetCatcher( Key_GetCatcher() | KEYCATCH_NPCMANAGER );
	return qtrue;
}

void CL_NpcManager_Close( void ) {
	if ( !nm.open )
		return;
	nm.open = qfalse;
	nm.pick = NM_PICK_NONE;
	nm.picker = NM_PICKER_NONE;
	nm.saving = NM_SAVING_NONE;
	nm.teleBox = qfalse;
	nm.preview = qfalse;
	NM_ReleaseKeys();
	NM_PreviewFree();	// never called while drawing
	Key_SetCatcher( Key_GetCatcher() & ~KEYCATCH_NPCMANAGER );
}

// opened from a storyteller step, on spawning or on the map's NPCs
void CL_NpcManager_OpenForStory( qboolean manage ) {
	if ( NM_Open( manage ? NM_TAB_MANAGE : NM_TAB_SPAWN ) )
		NM_Message( S_COLOR_GREEN "For the story: what you send here goes in it as steps, not to the server" );
}

void CL_NpcManager_Init( void ) {
	memset( &nm, 0, sizeof( nm ) );
	nm.selNum = -1;
	nm.routeOutlines = qtrue;
}

// cgame is going away: map change, disconnect or vid_restart
void CL_NpcManager_Shutdown( void ) {
	CL_NpcManager_Close();
	NM_PreviewFree();	// the renderer may be going too
	// the pure pk3 list can change with the next map or server
	nm.indexed = qfalse;
	nm.haveView = qfalse;
	nm.numNpcs = nm.numNpcView = 0;
	nm.listRequestTime = nm.listLineTime = nm.listTime = nm.refreshTime = 0;
	nm.listDone = qfalse;
	nm.numWalks = 0;
	nm.havePos = qfalse;
	nm.selNum = -1;
	nm.numMarked = 0;
	if ( nm.apply == NM_APPLY_MARKED )
		nm.apply = NM_APPLY_ONE;
	// spots in this map, for this server
	nm.numPlaced = nm.placedScroll = 0;
	nm.numQueue = nm.queueTotal = 0;
}

qboolean CL_NpcManager_Active( void ) {
	return nm.open;
}

// every frame in game, open or not: the spawns still to go, and the list
void CL_NpcManager_Frame( void ) {
	NM_RunQueue();

	// a list we asked for that never came, or one that is due
	if ( nm.listRequestTime && cls.realtime - nm.listRequestTime > NM_LIST_TIMEOUT ) {
		nm.listRequestTime = 0;
		nm.numNpcs = 0;
		NM_RebuildNpcView();
	}
	if ( nm.refreshTime && cls.realtime >= nm.refreshTime && !nm.numQueue && cls.realtime >= nm.nextSend )
		NM_RequestList();
	// the whole list read: the walks of NPCs gone from it are unhooked
	if ( nm.listTime && !nm.listDone && cls.realtime - nm.listLineTime >= NM_LIST_LINE_GAP ) {
		nm.listDone = qtrue;
		CL_NpcWalk_ListRead( nm.listTime );
		if ( nm.open )
			NM_RebuildNpcView();
	}
}

/*
===============================================================================

CONSOLE COMMAND

===============================================================================
*/

void CL_NpcManager_f( void ) {
	const char *arg = Cmd_Argv( 1 );

	if ( Cmd_Argc() < 2 ) {
		if ( nm.open )
			CL_NpcManager_Close();
		else
			NM_Open( nm.tab );
		return;
	}

	if ( !Q_stricmp( arg, "spawn" ) ) {
		if ( nm.open )
			NM_SetTab( NM_TAB_SPAWN );
		else
			NM_Open( NM_TAB_SPAWN );
	} else if ( !Q_stricmp( arg, "manage" ) ) {
		if ( nm.open )
			NM_SetTab( NM_TAB_MANAGE );
		else
			NM_Open( NM_TAB_MANAGE );
	} else if ( !Q_stricmp( arg, "close" ) ) {
		CL_NpcManager_Close();
	} else if ( !Q_stricmp( arg, "reindex" ) ) {
		NM_BuildIndex();
		NM_RefreshSaved();
		NM_RebuildTypeView();
	} else if ( !Q_stricmp( arg, "loadnpc" ) && Cmd_Argc() >= 3 ) {
		char name[MAX_QPATH];

		Q_strncpyz( name, Cmd_Argv( 2 ), sizeof( name ) );
		COM_StripExtension( name, name, sizeof( name ) );
		if ( !nm.open && !NM_Open( NM_TAB_SPAWN ) )
			return;
		NM_SetTab( NM_TAB_SPAWN );
		NM_RefreshSaved();
		if ( NM_FindSaved( qfalse, name ) < 0 )
			Com_Printf( S_COLOR_YELLOW "NPC manager: no saved NPC %s\n", NM_SavePath( qfalse, name ) );
		NM_SelectSaved( qfalse, name );
	} else if ( !Q_stricmp( arg, "loadconstruct" ) && Cmd_Argc() >= 3 ) {
		char name[MAX_QPATH];

		Q_strncpyz( name, Cmd_Argv( 2 ), sizeof( name ) );
		COM_StripExtension( name, name, sizeof( name ) );
		if ( !nm.open && !NM_Open( NM_TAB_SPAWN ) )
			return;
		NM_SetTab( NM_TAB_SPAWN );
		NM_StartConstruct( name );
		if ( nm.pick == NM_PICK_NONE )
			Com_Printf( "%s\n", nm.message );
	} else if ( !Q_stricmp( arg, "routing" ) ) {
		if ( nm.open )
			NM_SetTab( NM_TAB_ROUTES );
		else
			NM_Open( NM_TAB_ROUTES );
	} else if ( !Q_stricmp( arg, "walk" ) || !Q_stricmp( arg, "routes" ) ) {
		CL_NpcWalk_Command();
	} else if ( !Q_stricmp( arg, "talk" ) && Cmd_Argc() >= 4 ) {
		const char *target = Cmd_Argv( 2 );

		if ( !NM_TalkAllowed() )
			return;
		// as the selected one's name, if it's the one selected
		NM_Send( CL_RpHooks_NpcTalkCommand( target, !Q_stricmp( target, nmSpeak.target ) ? nmSpeak.name : NM_DefaultTalkName( target ),
			"", "", Cmd_ArgsFrom( 3 ) ), qfalse );
	} else if ( !Q_stricmp( arg, "select" ) || !Q_stricmp( arg, "unselect" ) ) {
		if ( !Q_stricmp( arg, "unselect" ) || !Q_stricmp( Cmd_Argv( 2 ), "off" ) ) {
			if ( !nmSpeak.target[0] )
				Com_Printf( "NPC manager: no NPC selected to talk through\n" );
			NM_SpeakOff( "" );
			return;
		}
		if ( Cmd_Argc() < 3 ) {
			if ( nmSpeak.target[0] )
				Com_Printf( "NPC manager: your chat goes through NPC %s, as %s" S_COLOR_WHITE "\n", nmSpeak.target, nmSpeak.name );
			else
				Com_Printf( "NPC manager: no NPC selected to talk through; npcmanager select <npc> [display name]\n" );
			return;
		}
		if ( !NM_TalkAllowed() )
			return;
		Q_strncpyz( nmSpeak.target, Cmd_Argv( 2 ), sizeof( nmSpeak.target ) );
		Q_strncpyz( nmSpeak.name, Cmd_Argc() >= 4 ? Cmd_ArgsFrom( 3 ) : NM_DefaultTalkName( nmSpeak.target ), sizeof( nmSpeak.name ) );
		Q_strncpyz( nmSpeak.map, cl.mapname, sizeof( nmSpeak.map ) );
		Com_Printf( "NPC manager: what you say in chat is now said by NPC %s, as %s" S_COLOR_WHITE "; npcmanager select off to talk as yourself\n",
			nmSpeak.target, nmSpeak.name );
	} else if ( !Q_stricmp( arg, "savednpc" ) || !Q_stricmp( arg, "constructnpc" ) ) {
		// the lines of a saved file being exec'd; its first line read them all
	} else {
		Com_Printf( "usage: npcmanager                       toggle the NPC manager (bind a key to it)\n" );
		Com_Printf( "       npcmanager spawn                 open on the spawn tab\n" );
		Com_Printf( "       npcmanager manage                open on the NPCs in the map\n" );
		Com_Printf( "       npcmanager loadnpc <name>        put a saved NPC in the spawn form\n" );
		Com_Printf( "       npcmanager loadconstruct <name>  pick up an NPC construct to put down\n" );
		Com_Printf( "       npcmanager talk <npc> <message>  the NPC (targetname or number) says it in chat\n" );
		Com_Printf( "       npcmanager select <npc> [name]   what you say in chat is said by the NPC, as name\n" );
		Com_Printf( "       npcmanager select off            talk as yourself again (unselect does too)\n" );
		Com_Printf( "       npcmanager walk <npc>[,<npc>...] <route> [walk|run|<speed>] [loop] [local]   the NPC walks a saved route, or round and round;\n" );
		Com_Printf( "                                        several walk it together, keeping their places\n" );
		Com_Printf( "       npcmanager walk <npc>[,<npc>...] stop   they stop where they are\n" );
		Com_Printf( "       npcmanager routes                list the saved routes\n" );
		Com_Printf( "       npcmanager routing               open on the Routes tab: who walks which route\n" );
		Com_Printf( "       npcmanager reindex               rescan the filesystem for NPC types and models\n" );
		Com_Printf( "       npcmanager close                 close the NPC manager\n" );
		Com_Printf( "Saved NPCs are in the %s folder of the game folder, constructs in %s; both can be exec'd\n", NM_NPC_DIR, NM_CONSTRUCT_DIR );
	}
}

/*
===============================================================================

MAP VIEW

===============================================================================
*/

static qboolean NM_Flying( void ) {
	return (qboolean)( nm.open && nm.pick != NM_PICK_NONE && ( Key_GetCatcher() & KEYCATCH_NPCMANAGER ) );
}

static void NM_UpdateCamera( void ) {
	vec3_t forward, right, move;
	float dt, speed;

	dt = Com_Clamp( 0.0f, 0.1f, ( cls.realtime - nm.lastFrameTime ) / 1000.0f );
	nm.lastFrameTime = cls.realtime;

	AngleVectors( nm.camAng, forward, right, NULL );
	VectorClear( move );
	if ( nm.held[A_CAP_W] )		VectorAdd( move, forward, move );
	if ( nm.held[A_CAP_S] )		VectorSubtract( move, forward, move );
	if ( nm.held[A_CAP_D] )		VectorAdd( move, right, move );
	if ( nm.held[A_CAP_A] )		VectorSubtract( move, right, move );
	if ( nm.held[A_SPACE] )		move[2] += 1.0f;
	if ( nm.held[A_CAP_C] )		move[2] -= 1.0f;

	if ( VectorNormalize( move ) > 0.0f ) {
		speed = NM_FLY_SPEED;
		if ( nm.held[A_SHIFT] || nm.held[A_SHIFT2] )
			speed *= 3.0f;
		if ( nm.held[A_CTRL] || nm.held[A_CTRL2] )
			speed *= 0.25f;
		VectorMA( nm.camOrg, speed * dt, move, nm.camOrg );
	}
}

static void NM_KeepView( const refdef_t *fd ) {
	VectorCopy( fd->vieworg, nm.viewOrg );
	VectorCopy( fd->viewaxis[0], nm.viewAxis[0] );
	VectorCopy( fd->viewaxis[1], nm.viewAxis[1] );
	VectorCopy( fd->viewaxis[2], nm.viewAxis[2] );
	nm.fovX = fd->fov_x;
	nm.fovY = fd->fov_y;
	nm.haveView = qtrue;
}

// every cgame scene passes through here before reaching the renderer; qtrue if it was rendered
qboolean CL_NpcManager_RenderScene( const refdef_t *fd ) {
	refdef_t view;

	if ( fd->rdflags & ( RDF_NOWORLDMODEL | RDF_AUTOMAP ) )
		return qfalse;

	if ( fd->rdflags & RDF_SKYBOXPORTAL ) {
		if ( !NM_Flying() )
			return qfalse;
		// the sky portal keeps its own origin but looks the way the camera does
		view = *fd;
		AnglesToAxis( nm.camAng, view.viewaxis );
		VectorCopy( nm.camAng, view.viewangles );
		re->RenderScene( &view );
		return qtrue;
	}

	if ( !NM_Flying() ) {
		NM_KeepView( fd );
		return qfalse;
	}

	if ( nm.viewFrame != cls.framecount ) {
		nm.viewFrame = cls.framecount;
		NM_UpdateCamera();
	}
	view = *fd;
	VectorCopy( nm.camOrg, view.vieworg );
	VectorCopy( nm.camAng, view.viewangles );
	AnglesToAxis( nm.camAng, view.viewaxis );
	view.viewContents = CM_PointContents( nm.camOrg, 0 );
	// the snapshot's area mask is from the player's position, the camera can be anywhere
	memset( view.areamask, 0, sizeof( view.areamask ) );
	NM_KeepView( &view );
	re->RenderScene( &view );
	return qtrue;
}

// the free camera, for the effects system to cull against
qboolean CL_NpcManager_Camera( vec3_t origin, vec3_t angles ) {
	if ( !NM_Flying() )
		return qfalse;
	VectorCopy( nm.camOrg, origin );
	VectorCopy( nm.camAng, angles );
	return qtrue;
}

// the view weapon would float where the player stands
qboolean CL_NpcManager_FilterEntity( const refEntity_t *ent ) {
	return (qboolean)( NM_Flying() && ( ent->renderfx & RF_FIRST_PERSON ) );
}

static qboolean NM_Project( const vec3_t point, float *x, float *y ) {
	vec3_t d;
	float forward;

	if ( !nm.haveView )
		return qfalse;
	VectorSubtract( point, nm.viewOrg, d );
	forward = DotProduct( d, nm.viewAxis[0] );
	if ( forward < 4.0f )
		return qfalse;
	*x = SCREEN_WIDTH * 0.5f * ( 1.0f - DotProduct( d, nm.viewAxis[1] ) / forward / tanf( DEG2RAD( nm.fovX * 0.5f ) ) );
	*y = SCREEN_HEIGHT * 0.5f * ( 1.0f - DotProduct( d, nm.viewAxis[2] ) / forward / tanf( DEG2RAD( nm.fovY * 0.5f ) ) );
	return qtrue;
}

// the world under the cursor; out is where an NPC's origin would go
static qboolean NM_TraceCursor( vec3_t out ) {
	vec3_t dir, end;
	float x, y;
	trace_t tr;

	if ( !nm.haveView )
		return qfalse;
	x = ( nm.cursorX / ( SCREEN_WIDTH * 0.5f ) - 1.0f ) * tanf( DEG2RAD( nm.fovX * 0.5f ) );
	y = ( nm.cursorY / ( SCREEN_HEIGHT * 0.5f ) - 1.0f ) * tanf( DEG2RAD( nm.fovY * 0.5f ) );
	VectorCopy( nm.viewAxis[0], dir );
	VectorMA( dir, -x, nm.viewAxis[1], dir );
	VectorMA( dir, -y, nm.viewAxis[2], dir );
	VectorNormalize( dir );
	VectorMA( nm.viewOrg, NM_TRACE_DIST, dir, end );

	CM_BoxTrace( &tr, nm.viewOrg, end, vec3_origin, vec3_origin, 0, NM_TRACE_MASK, qfalse );
	if ( tr.fraction >= 1.0f || tr.allsolid )
		return qfalse;

	// off the floor by its own height, out of walls by a bit
	VectorMA( tr.endpos, 16.0f, tr.plane.normal, out );
	if ( tr.plane.normal[2] > 0.7f )
		out[2] = tr.endpos[2] + NM_GROUND_HEIGHT;
	return qtrue;
}

/*
===============================================================================

INPUT

===============================================================================
*/

static qboolean NM_InRect( float x, float y, float w, float h ) {
	return (qboolean)( nm.cursorX >= x && nm.cursorX < x + w && nm.cursorY >= y && nm.cursorY < y + h );
}

// binds don't run while the manager holds the keys, so honour the user's toggle bind here
static qboolean NM_IsToggleKey( int key ) {
	const char *binding;

	// a printable key belongs to the text box
	if ( nm.pick == NM_PICK_NONE && key >= A_SPACE && key <= A_TILDE )
		return qfalse;
	binding = Key_GetBinding( key );
	return (qboolean)( VALIDSTRING( binding ) && !Q_stricmp( binding, NM_TOGGLE_CMD ) );
}

// the NPC whose label is under the cursor, -1 = none
static int NM_NpcAtCursor( void ) {
	float best = NM_LABEL_RANGE * NM_LABEL_RANGE, x, y;
	vec3_t org;
	int found = -1;

	for ( int i = 0; i < nm.numNpcs; i++ ) {
		NM_NpcOrigin( &nm.npcs[i], org );
		org[2] += 40.0f;
		if ( !NM_Project( org, &x, &y ) )
			continue;
		x -= nm.cursorX;
		y -= nm.cursorY;
		if ( x * x + y * y < best ) {
			best = x * x + y * y;
			found = i;
		}
	}
	return found;
}

// how far the cursor is from a line on screen
static float NM_CursorToSegment( float x1, float y1, float x2, float y2 ) {
	const float dx = x2 - x1, dy = y2 - y1, len = dx * dx + dy * dy;
	float t = len > 0.0f ? ( ( nm.cursorX - x1 ) * dx + ( nm.cursorY - y1 ) * dy ) / len : 0.0f;

	t = Com_Clamp( 0.0f, 1.0f, t );
	return sqrtf( Square( x1 + t * dx - nm.cursorX ) + Square( y1 + t * dy - nm.cursorY ) );
}

// the walk under the crosshair in the routes view: near its NPC or a line of its route; -1 none
static int NM_RouteHover( void ) {
	float best = NM_LABEL_RANGE * 0.5f;
	int found = -1;

	for ( int i = 0; i < nm.numWalks; i++ ) {
		const npcWalkInfo_t *w = &nm.walks[i];
		vec3_t head;
		float x, y, px, py;

		VectorCopy( w->pos, head );
		head[2] += 40.0f;
		if ( NM_Project( head, &x, &y ) && NM_CursorToSegment( x, y, x, y ) < best ) {
			best = NM_CursorToSegment( x, y, x, y );
			found = i;
		}
		if ( !nm.routeOutlines )
			continue;
		for ( int j = 1; j < w->numPts; j++ ) {
			if ( NM_Project( w->pts[j - 1], &px, &py ) && NM_Project( w->pts[j], &x, &y ) && NM_CursorToSegment( px, py, x, y ) < best ) {
				best = NM_CursorToSegment( px, py, x, y );
				found = i;
			}
		}
	}
	return found;
}

// the selected NPC by its label on the map
static void NM_SelectAtCursor( void ) {
	int i = NM_NpcAtCursor();

	if ( i < 0 )
		return;
	// a filter could hide it in the list
	nm.fields[NM_F_NPC_SEARCH][0] = '\0';
	nm.selNum = nm.npcs[i].num;
	NM_RebuildNpcView();
	NM_ListMove( &nm.npcList, nm.numNpcView, NM_NpcListRows(), 0 );
}

static void NM_PickClick( void ) {
	vec3_t pos;

	if ( nm.pick == NM_PICK_ROUTES ) {
		// an NPC by its route or its label; Ctrl marks it. Stays in the view, for more
		const int walk = NM_RouteHover();
		const int i = walk >= 0 ? -1 : NM_NpcAtCursor();
		const int num = walk >= 0 ? nm.walks[walk].num : i >= 0 ? nm.npcs[i].num : -1;

		if ( num < 0 )
			return;
		if ( nm.held[A_CTRL] || nm.held[A_CTRL2] ) {
			NM_ToggleMark( num );
			return;
		}
		nm.fields[NM_F_NPC_SEARCH][0] = '\0';
		nm.routeShow = NM_RSHOW_ALL;
		nm.selNum = num;
		NM_RebuildNpcView();
		NM_ListMove( &nm.npcList, nm.numNpcView, NM_NpcListRows(), 0 );
		return;
	}

	if ( nm.pick == NM_PICK_SELECT ) {
		// Ctrl marks one, or unmarks it, and goes on picking
		if ( ( nm.held[A_CTRL] || nm.held[A_CTRL2] ) && NM_NpcAtCursor() >= 0 ) {
			NM_ToggleMark( nm.npcs[NM_NpcAtCursor()].num );
			return;
		}
		// a miss keeps flying, to look for it elsewhere
		if ( NM_NpcAtCursor() >= 0 ) {
			NM_SelectAtCursor();
			NM_EndPick();
		}
		return;
	}
	if ( !NM_TraceCursor( pos ) )
		return;
	if ( nm.pick == NM_PICK_ROUTE ) {
		// one more spot, then the next
		if ( nm.numRoute < NW_MAX_ROUTE_POINTS )
			VectorCopy( pos, nm.route[nm.numRoute++] );
		return;
	}
	if ( nm.pick == NM_PICK_PLACE ) {
		// stays in hand for the next one
		NM_PutDown( pos );
		return;
	}
	if ( nm.pick == NM_PICK_SPAWN ) {
		VectorCopy( pos, nm.pos );
		nm.yaw = NM_NormalizeYaw( NM_YawToView( pos ) + nm.pickYaw );
		nm.havePos = qtrue;
	} else if ( nm.pick == NM_PICK_TELE ) {
		NM_Teleport( pos, NM_NormalizeYaw( NM_YawToView( pos ) + nm.pickYaw ) );
	}
	NM_EndPick();
}

static qboolean NM_FieldAllows( nmField_t f, int ch ) {
	if ( f == NM_F_SAVE_NAME )
		return NM_SaveNameChar( ch );
	if ( f >= NM_F_TELE_X && f <= NM_F_TELE_YAW )
		return (qboolean)( ( ch >= '0' && ch <= '9' ) || ch == '-' || ch == '.' );
	if ( f == NM_F_SCALE )
		return (qboolean)( ( ch >= '0' && ch <= '9' ) || ch == '.' );
	// these go on the command line as single arguments
	if ( f == NM_F_TARGETNAME || f == NM_F_MODEL || f == NM_F_EMOTE || f == NM_F_DIALOG )
		return (qboolean)( ch > ' ' && ch < 127 && ch != '"' && ch != ';' );
	if ( f == NM_F_TALK_TEXT || f == NM_F_TALK_NAME )
		return (qboolean)( ch >= ' ' && ch < 127 && ch != '"' );
	return (qboolean)( ch >= ' ' && ch < 127 );
}

static qboolean NM_AllowChar( int ch ) {
	return NM_FieldAllows( NM_ActiveField(), ch );
}

static void NM_FieldChanged( nmField_t f ) {
	switch ( f ) {
	case NM_F_TYPE_SEARCH:
		nm.typeList.sel = nm.typeList.scroll = 0;
		NM_RebuildTypeView();
		break;
	case NM_F_NPC_SEARCH:
		nm.npcList.scroll = 0;
		NM_RebuildNpcView();
		break;
	case NM_F_PICKER_SEARCH:
		nm.pickerList.sel = nm.pickerList.scroll = 0;
		NM_RebuildPickerView();
		break;
	default:
		break;
	}
}

static void NM_ListKey( int key ) {
	nmList_t *list;
	int count, visible, delta;

	if ( nm.picker != NM_PICKER_NONE ) {
		list = &nm.pickerList;
		count = nm.numPickerView;
		visible = (int)( ( NM_PICKER_H - 74 ) / NM_ROW_H );
	} else if ( nm.tab == NM_TAB_SPAWN ) {
		// the rows on the left
		if ( key == A_CURSOR_LEFT || key == A_CURSOR_RIGHT ) {
			if ( !nm.fields[NM_F_TYPE_SEARCH][0] )
				NM_SetRow( nm.row + ( key == A_CURSOR_LEFT ? -1 : 1 ) );
			return;
		}
		list = &nm.typeList;
		count = nm.numTypeView;
		visible = (int)( NM_TYPE_LIST_H / NM_ROW_H );
	} else {
		list = &nm.npcList;
		count = nm.numNpcView;
		visible = NM_NpcListRows();
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
	NM_ListMove( list, count, visible, delta );
	if ( list == &nm.npcList && list->sel < nm.numNpcView )
		nm.selNum = nm.npcs[nm.npcView[list->sel]].num;
}

// Enter or a double-click on the list: a construct is picked up, anything else spawns
static void NM_SpawnOrPlace( void ) {
	const nmSaved_t *s = NM_SelectedSaved();

	if ( s && s->construct )
		NM_StartConstruct( s->name );
	else
		NM_Spawn();
}

void CL_NpcManager_KeyEvent( int key, qboolean down ) {
	nmField_t f;

	if ( !nm.open )
		return;
	// held keys fly the camera
	if ( key >= 0 && key < MAX_KEYS )
		nm.held[key] = down;
	if ( !down )
		return;

	if ( NM_IsToggleKey( key ) ) {
		CL_NpcManager_Close();
		return;
	}

	if ( nm.pick != NM_PICK_NONE ) {
		switch ( key ) {
		case A_MOUSE1:
			NM_PickClick();
			break;
		case A_MOUSE2:
			NM_EndPick();
			break;
		case A_MWHEELUP:
			nm.pickYaw += NM_YAW_STEP;
			break;
		case A_MWHEELDOWN:
			nm.pickYaw -= NM_YAW_STEP;
			break;
		case A_BACKSPACE:
			// takes back the last one put down
			if ( nm.pick == NM_PICK_PLACE && nm.numPlaced )
				nm.numPlaced--;
			if ( nm.pick == NM_PICK_ROUTE && nm.numRoute )
				nm.numRoute--;
			break;
		case A_DELETE:
			if ( nm.pick == NM_PICK_ROUTE )
				nm.numRoute = 0;
			break;
		case A_CAP_R:
			if ( nm.pick == NM_PICK_ROUTE )
				nm.routeRun = (qboolean)!nm.routeRun;
			break;
		case A_CAP_O:
			if ( nm.pick == NM_PICK_ROUTES )
				nm.routeOutlines = (qboolean)!nm.routeOutlines;
			break;
		case A_CAP_F:
			if ( nm.pick == NM_PICK_ROUTES )
				nm.routeFocus = (qboolean)!nm.routeFocus;
			break;
		case A_ENTER:
		case A_KP_ENTER:
			if ( nm.pick == NM_PICK_PLACE ) {
				NM_SpawnPlaced();
				NM_EndPick();
			} else if ( nm.pick == NM_PICK_ROUTE && nm.numRoute ) {
				NM_EndPick();
				NM_StartSaving( NM_SAVING_ROUTE );
			}
			break;
		default:
			break;
		}
		return;
	}

	if ( nm.experimentalBox && key != A_MOUSE1 )
		return;
	f = NM_ActiveField();
	if ( key != A_MOUSE1 && key != A_MWHEELUP && key != A_MWHEELDOWN && !( nm.preview && nm.picker == NM_PICKER_NONE )
		&& !( !nm.fields[f][0] && ( key == A_CURSOR_LEFT || key == A_CURSOR_RIGHT || key == A_HOME || key == A_END ) ) ) {
		const int edit = WG_EditKey( nm.fields[f], NM_FIELD_LEN, key, NM_AllowChar );

		if ( edit ) {
			if ( edit & WG_EDIT_CHANGED )
				NM_FieldChanged( f );
			return;
		}
	}
	if ( nm.saving != NM_SAVING_NONE && nm.picker == NM_PICKER_NONE ) {
		// only the name box takes keys
		switch ( key ) {
		case A_ENTER:
		case A_KP_ENTER:
			NM_DoSave();
			return;
		case A_MOUSE1:
		case A_BACKSPACE:
		case A_DELETE:
			break;
		default:
			return;
		}
	}
	if ( nm.teleBox && nm.picker == NM_PICKER_NONE ) {
		// only the teleport box takes keys
		switch ( key ) {
		case A_ENTER:
		case A_KP_ENTER:
			NM_TeleportToFields();
			return;
		case A_TAB:
			// the next box, Shift the one before
			nm.focus = (nmField_t)( NM_F_TELE_X + ( f - NM_F_TELE_X + ( nm.held[A_SHIFT] || nm.held[A_SHIFT2] ? 3 : 1 ) ) % 4 );
			return;
		case A_MOUSE1:
		case A_BACKSPACE:
		case A_DELETE:
			break;
		default:
			return;
		}
	}
	if ( nm.preview && nm.picker == NM_PICKER_NONE ) {
		// nothing to type in; the keys go to the popup and the type list behind it
		switch ( key ) {
		case A_BACKSPACE:
		case A_DELETE:
		case A_TAB:
			return;
		case A_ENTER:
		case A_KP_ENTER:
			NM_Spawn();
			NM_ClosePreview();
			return;
		default:
			break;
		}
	}
	switch ( key ) {
	case A_MOUSE1:
		nm.click = qtrue;
		nm.clickX = nm.cursorX;
		nm.clickY = nm.cursorY;
		break;
	case A_MWHEELUP:
		nm.wheel -= 3;
		break;
	case A_MWHEELDOWN:
		nm.wheel += 3;
		break;
	case A_TAB:
		if ( nm.picker == NM_PICKER_NONE )
			NM_SetTab( (nmTab_t)( ( nm.tab + 1 ) % ( NM_TAB_ROUTES + 1 ) ) );
		break;
	case A_ENTER:
	case A_KP_ENTER:
		if ( nm.picker != NM_PICKER_NONE ) {
			if ( nm.pickerList.sel < nm.numPickerView )
				NM_ChoosePicker( nm.pickerView[nm.pickerList.sel] );
		} else if ( f == NM_F_TALK_TEXT || f == NM_F_TALK_NAME ) {
			NM_Talk();
		} else if ( f == NM_F_EMOTE || f == NM_F_DIALOG ) {
			const char *target = NM_ApplyTarget( qtrue );

			if ( target && nm.fields[f][0] )
				NM_SendManage( va( "npc %s %s %s", f == NM_F_EMOTE ? "emote" : "dialog", nm.fields[f], target ), qfalse );
		} else if ( nm.tab == NM_TAB_SPAWN ) {
			NM_SpawnOrPlace();
		}
		break;
	default:
		NM_ListKey( key );
		break;
	}
}

void CL_NpcManager_CharEvent( int ch ) {
	nmField_t f;

	if ( !nm.open || nm.pick != NM_PICK_NONE || nm.experimentalBox || ( nm.preview && nm.picker == NM_PICKER_NONE ) )
		return;
	f = NM_ActiveField();
	if ( WG_EditChar( nm.fields[f], NM_FIELD_LEN, ch, NM_AllowChar ) )
		NM_FieldChanged( f );
}

void CL_NpcManager_MouseEvent( int dx, int dy ) {
	if ( nm.pick != NM_PICK_NONE ) {
		nm.camAng[YAW] -= dx * cl_sensitivity->value * m_yaw->value;
		nm.camAng[PITCH] = Com_Clamp( -89.0f, 89.0f, nm.camAng[PITCH] + dy * cl_sensitivity->value * m_pitch->value );
		return;
	}
	nm.cursorX = Com_Clamp( 0, SCREEN_WIDTH, nm.cursorX + dx );
	nm.cursorY = Com_Clamp( 0, SCREEN_HEIGHT, nm.cursorY + dy );
}

// escape backs out one level: picker, picking, a text box, then the manager
void CL_NpcManager_Escape( void ) {
	if ( nm.experimentalBox )
		nm.experimentalBox = qfalse;
	else if ( nm.picker != NM_PICKER_NONE )
		nm.picker = NM_PICKER_NONE;
	else if ( nm.saving != NM_SAVING_NONE )
		nm.saving = NM_SAVING_NONE;
	else if ( nm.teleBox )
		nm.teleBox = qfalse, nm.focus = NM_F_NONE;
	else if ( nm.preview )
		NM_ClosePreview();
	else if ( nm.pick != NM_PICK_NONE )
		NM_EndPick();
	else if ( nm.focus != NM_F_NONE )
		nm.focus = NM_F_NONE;
	else
		CL_NpcManager_Close();
}

/*
===============================================================================

2D

===============================================================================
*/

static void NM_Fill( float x, float y, float w, float h, const float *color ) {
	re->SetColor( color );
	re->DrawStretchPic( x, y, w, h, 0, 0, 0, 0, cls.whiteShader );
	re->SetColor( NULL );
}

static void NM_Box( float x, float y, float w, float h, const float *fill ) {
	NM_Fill( x, y, w, h, fill );
	NM_Fill( x, y, w, 1, nmBorder );
	NM_Fill( x, y + h - 1, w, 1, nmBorder );
	NM_Fill( x, y, 1, h, nmBorder );
	NM_Fill( x + w - 1, y, 1, h, nmBorder );
}

static void NM_Text( float x, float y, const char *text, const float *color ) {
	re->Font_DrawString( (int)x, (int)y, text, color, nm.font | STYLE_DROPSHADOW, -1, NM_TEXT_SCALE );
}

static float NM_TextWidth( const char *text ) {
	return (float)re->Font_StrLenPixels( text, nm.font, NM_TEXT_SCALE );
}

static void NM_TextClipped( float x, float y, float w, const char *text, const float *color ) {
	char buf[MAX_STRING_CHARS];
	int len;

	Q_strncpyz( buf, text, sizeof( buf ) );
	len = strlen( buf );
	while ( len > 3 && NM_TextWidth( buf ) > w ) {
		len--;
		buf[len - 3] = '.';
		buf[len - 2] = '.';
		buf[len - 1] = '.';
		buf[len] = '\0';
	}
	NM_Text( x, y, buf, color );
}

// eats this frame's click if it landed in the rect
static qboolean NM_Clicked( float x, float y, float w, float h ) {
	if ( !nm.click || nm.clickX < x || nm.clickX >= x + w || nm.clickY < y || nm.clickY >= y + h )
		return qfalse;
	nm.click = qfalse;
	return qtrue;
}

static qboolean NM_Button( float x, float y, float w, const char *label, qboolean selected, qboolean enabled ) {
	const float *fill = selected ? nmHighlight : ( enabled && NM_InRect( x, y, w, NM_CTRL_H ) ) ? nmHoverBox : nmPanelLight;

	NM_Box( x, y, w, NM_CTRL_H, fill );
	NM_Text( x + ( w - NM_TextWidth( label ) ) * 0.5f, y + 2, label, enabled ? nmWhite : nmDim );
	return (qboolean)( NM_Clicked( x, y, w, NM_CTRL_H ) && enabled );
}

// a button that has to be clicked twice
static qboolean NM_DangerButton( float x, float y, float w, const char *label, nmConfirm_t id, qboolean enabled ) {
	qboolean armed = (qboolean)( nm.confirm == id && cls.realtime - nm.confirmTime < NM_CONFIRM_MS );
	const char *text = armed ? "Click again" : label;

	NM_Box( x, y, w, NM_CTRL_H, armed ? nmDanger : ( enabled && NM_InRect( x, y, w, NM_CTRL_H ) ) ? nmHoverBox : nmPanelLight );
	NM_Text( x + ( w - NM_TextWidth( text ) ) * 0.5f, y + 2, text, enabled ? nmWhite : nmDim );
	if ( !NM_Clicked( x, y, w, NM_CTRL_H ) || !enabled )
		return qfalse;
	if ( armed ) {
		nm.confirm = NM_CONFIRM_NONE;
		return qtrue;
	}
	nm.confirm = id;
	nm.confirmTime = cls.realtime;
	return qfalse;
}

static void NM_FieldStyle( wgStyle_t *style ) {
	style->font = nm.font;
	style->scale = NM_TEXT_SCALE;
	style->lineH = NM_ROW_H;
	style->padX = 4.0f;
	style->padY = 2.0f;
	style->fill = nmPanelFocus;
	style->border = nmBorder;
	style->text = nmWhite;
	style->dim = nmDim;
}

// a text box, grown down while it's typed in and long; a click puts the cursor there
static void NM_Field( nmField_t f, float x, float y, float w, const char *placeholder ) {
	const qboolean focused = (qboolean)( NM_ActiveField() == f );
	wgStyle_t style;

	NM_FieldStyle( &style );
	NM_Box( x, y, w, NM_CTRL_H, focused ? nmPanelFocus : nmPanelLight );
	WG_Field( &style, x, y, w, NM_CTRL_H, nm.fields[f], focused, placeholder, nm.cursorX, nm.cursorY );
	if ( NM_Clicked( x, y, w, WG_FieldHeight( &style, w, NM_CTRL_H, nm.fields[f], focused ) ) ) {
		nm.focus = ( f == NM_F_TYPE_SEARCH || f == NM_F_NPC_SEARCH || f == NM_F_PICKER_SEARCH || f == NM_F_SAVE_NAME ) ? NM_F_NONE : f;
		WG_FieldClick( nm.fields[f], nm.clickX, nm.clickY );
	}
}

static void NM_Label( float y, const char *label ) {
	NM_Text( NM_RIGHT_X, y + 2, label, nmDim );
}

typedef void ( *nmRowFunc_t )( int row, float x, float y, float w );

// the room a list's scrollbar takes, 0 when all its rows fit
static float NM_ListBarW( int count, float h ) {
	return count > (int)( h / NM_ROW_H ) ? WG_SCROLL_W + 1 : 0.0f;
}

// returns the row double-clicked, -1 if none
static int NM_List( nmList_t *list, int count, float x, float y, float w, float h, nmRowFunc_t drawRow ) {
	const int visible = (int)( h / NM_ROW_H );
	int row, picked = -1;

	if ( nm.wheel && NM_InRect( x, y, w, h ) ) {
		list->scroll += nm.wheel;
		nm.wheel = 0;
	}
	NM_ListClamp( list, count, visible );
	NM_Box( x, y, w, h, nmPanelLight );
	if ( WG_Scrollbar( x + w - WG_SCROLL_W - 1, y + 1, h - 2, count, visible, &list->scroll, nm.cursorX, nm.cursorY, &nm.click, nm.clickX, nm.clickY ) )
		w -= WG_SCROLL_W + 1;

	if ( NM_Clicked( x, y, w, h ) ) {
		row = list->scroll + (int)( ( nm.clickY - y ) / NM_ROW_H );
		if ( row < count ) {
			if ( row == list->lastClickRow && cls.realtime - list->lastClickTime < NM_DOUBLECLICK_MS ) {
				list->lastClickTime = 0;
				picked = row;
			} else {
				list->lastClickRow = row;
				list->lastClickTime = cls.realtime;
			}
			list->sel = row;
		}
	}

	for ( int i = 0; i < visible; i++ ) {
		float ry = y + i * NM_ROW_H;

		row = list->scroll + i;
		if ( row >= count )
			break;
		if ( row == list->sel )
			NM_Fill( x + 1, ry, w - 2, NM_ROW_H, nmHighlight );
		else if ( NM_InRect( x, ry, w, NM_ROW_H ) )
			NM_Fill( x + 1, ry, w - 2, NM_ROW_H, nmHover );
		drawRow( row, x + 4, ry + 1, w - 8 );
	}
	return picked;
}

static void NM_DrawTypeRow( int row, float x, float y, float w ) {
	const int v = nm.typeView[row];
	const nmType_t *t;
	const nmSaved_t *s;

	if ( v < 0 ) {
		s = &nm.saved[-1 - v];
		if ( s->construct )
			NM_TextClipped( x, y, w, va( S_COLOR_CYAN "%s " S_COLOR_GREY "(construct, %i NPCs)", s->name, s->count ), nmWhite );
		else
			NM_TextClipped( x, y, w, va( S_COLOR_CYAN "%s " S_COLOR_GREY "(saved %s)", s->name, s->npc.type ), nmWhite );
		return;
	}
	// custom ones in green
	t = &nm.types[v];
	NM_TextClipped( x, y, w, t->vehicle ? va( "%s " S_COLOR_GREY "(vehicle)", t->name ) : t->name, t->custom ? nmCustom : nmWhite );
}

// what was saved, Base and its .npc files, Custom with its pk3s and theirs indented under them
static void NM_DrawTreeRow( int row, float x, float y, float w ) {
	const nmRow_t *r = &nm.rows[row];
	const float *color = nm.fields[NM_F_TYPE_SEARCH][0] ? nmDim : nmWhite;
	char file[MAX_QPATH];
	const char *slash;
	float indent;

	switch ( r->type ) {
	case NM_ROW_SAVED:
		NM_TextClipped( x, y, w, va( S_COLOR_CYAN "%s " S_COLOR_GREY "(%i)", r->name, r->count ), color );
		break;
	case NM_ROW_CATEGORY:
		NM_TextClipped( x, y, w, va( S_COLOR_YELLOW "%s " S_COLOR_GREY "(%i)", r->name, r->count ), color );
		break;
	case NM_ROW_SOURCE:
		// "base/name.pk3" as "name.pk3", a loose folder as itself
		slash = strrchr( r->name, '/' );
		indent = 6;
		NM_TextClipped( x + indent, y, w - indent, strlen( r->name ) > 4 && !Q_stricmp( r->name + strlen( r->name ) - 4, ".pk3" )
			? va( S_COLOR_GREEN "%s " S_COLOR_GREY "(%i)", slash ? slash + 1 : r->name, r->count )
			: va( S_COLOR_GREEN "%s folder " S_COLOR_GREY "(%i)", r->name, r->count ), color );
		break;
	default:
		indent = r->custom ? 12 : 6;
		COM_StripExtension( r->name, file, sizeof( file ) );
		NM_TextClipped( x + indent, y, w - indent, va( "%s " S_COLOR_GREY "(%i)", file, r->count ), color );
		break;
	}
}

// "Base" or "Custom, from <pk3>"
static const char *NM_SourceLabel( const nmType_t *t ) {
	if ( !t->custom )
		return va( "Base, from %s", nm.sources[t->source] );
	return t->source ? va( "Custom, from %s", nm.sources[t->source] ) : "Custom, not in your files";
}

static void NM_DrawNpcRow( int row, float x, float y, float w ) {
	const nmNpc_t *n = &nm.npcs[nm.npcView[row]];

	// marked with Ctrl+click
	if ( NM_IsMarked( n->num ) )
		NM_Fill( x - 3, y, 2, NM_ROW_H - 2, nmAccent );
	NM_Text( x, y, va( S_COLOR_CYAN "%i", n->num ), nmWhite );
	NM_TextClipped( x + 34, y, w - 34, n->name[0] ? va( "%s " S_COLOR_GREY "%s", n->name, n->type ) : va( S_COLOR_GREY "%s", n->type ), nmWhite );
}

static void NM_DrawPickerRow( int row, float x, float y, float w ) {
	const int index = nm.pickerView[row];

	if ( nm.picker == NM_PICKER_PLAYER )
		NM_TextClipped( x, y, w, va( S_COLOR_CYAN "%i " S_COLOR_WHITE "%s%s", index, NM_PickerItem( index ), index == clc.clientNum ? S_COLOR_GREY " (you)" : "" ), nmWhite );
	else
		NM_TextClipped( x, y, w, NM_PickerItem( index ), nmWhite );
}

static void NM_DrawCommandLines( const char *preview ) {
	if ( nm.numQueue ) {
		NM_TextClipped( NM_RIGHT_X, NM_BUTTON_Y - 30, NM_RIGHT_W, va( S_COLOR_YELLOW "Sending, %i of %i to go (the server takes about one a second)",
			nm.numQueue, nm.queueTotal ), nmWhite );
	} else if ( preview ) {
		NM_TextClipped( NM_RIGHT_X, NM_BUTTON_Y - 30, NM_RIGHT_W, va( S_COLOR_GREY "%s", preview ), nmWhite );
	}
	if ( nm.lastCmd[0] ) {
		NM_TextClipped( NM_RIGHT_X, NM_BUTTON_Y - 17, NM_RIGHT_W,
			va( "%sSent: %s", cls.realtime - nm.sentTime < 1500 ? S_COLOR_GREEN : S_COLOR_GREY, nm.lastCmd ), nmWhite );
	}
}

// the NPCs put down and waiting, and what can be done with them
static void NM_DrawPlaced( void ) {
	const float y = NM_PLACED_Y, by = y + 18, bh = NM_PLACED_ROWS * NM_ROW_H + 4, ay = by + bh + 4;
	const qboolean any = (qboolean)( nm.numPlaced > 0 );
	int remove = -1;

	NM_Label( y, "Put down" );
	if ( NM_Button( NM_CTRL_X, y, 140, "Place several on map...", qfalse, (qboolean)( NM_SelectedType() != NULL ) ) )
		NM_StartPlace();
	NM_TextClipped( NM_CTRL_X + 146, y + 2, NM_CTRL_W - 146,
		any ? va( S_COLOR_YELLOW "%i waiting to spawn", nm.numPlaced ) : S_COLOR_GREY "none waiting", nmWhite );

	NM_Box( NM_RIGHT_X, by, NM_RIGHT_W, bh, nmPanelLight );
	if ( nm.wheel && NM_InRect( NM_RIGHT_X, by, NM_RIGHT_W, bh ) ) {
		nm.placedScroll += nm.wheel;
		nm.wheel = 0;
	}
	nm.placedScroll = Com_Clampi( 0, Q_max( 0, nm.numPlaced - NM_PLACED_ROWS ), nm.placedScroll );
	const float placedBar = WG_Scrollbar( NM_RIGHT_X + NM_RIGHT_W - WG_SCROLL_W - 1, by + 1, bh - 2, nm.numPlaced, NM_PLACED_ROWS,
		&nm.placedScroll, nm.cursorX, nm.cursorY, &nm.click, nm.clickX, nm.clickY ) ? WG_SCROLL_W + 1 : 0.0f;
	if ( !any ) {
		NM_TextClipped( NM_RIGHT_X + 4, by + 3, NM_RIGHT_W - 8, S_COLOR_GREY "Put NPCs down on the map, then spawn them all", nmWhite );
		NM_TextClipped( NM_RIGHT_X + 4, by + 3 + NM_ROW_H, NM_RIGHT_W - 8, S_COLOR_GREY "at once, or save them as a construct", nmWhite );
	}
	for ( int i = 0; i < NM_PLACED_ROWS; i++ ) {
		const int row = nm.placedScroll + i;
		const float ry = by + 2 + i * NM_ROW_H, xx = NM_RIGHT_X + NM_RIGHT_W - 20 - placedBar;
		const nmSpawn_t *p;

		if ( row >= nm.numPlaced )
			break;
		p = &nm.placed[row];
		NM_TextClipped( NM_RIGHT_X + 4, ry, NM_RIGHT_W - 28 - placedBar, va( "%s %s" S_COLOR_GREY "at %i %i %i, yaw %i", p->type,
			p->targetname[0] ? va( S_COLOR_CYAN "%s ", p->targetname ) : "", (int)p->pos[0], (int)p->pos[1], (int)p->pos[2], p->yaw ), nmWhite );
		// takes it back
		if ( NM_InRect( xx, ry, 16, NM_ROW_H ) )
			NM_Fill( xx, ry, 16, NM_ROW_H, nmHoverBox );
		NM_Text( xx + 5, ry, S_COLOR_RED "x", nmWhite );
		if ( NM_Clicked( xx, ry, 16, NM_ROW_H ) )
			remove = row;
	}
	if ( remove >= 0 ) {
		memmove( &nm.placed[remove], &nm.placed[remove + 1], ( nm.numPlaced - remove - 1 ) * sizeof( nm.placed[0] ) );
		nm.numPlaced--;
	}

	if ( NM_Button( NM_RIGHT_X, ay, 130, any ? va( "Spawn all %i", nm.numPlaced ) : "Spawn all", qfalse, any ) )
		NM_SpawnPlaced();
	if ( NM_DangerButton( NM_RIGHT_X + 134, ay, 70, "Clear", NM_CONFIRM_CLEAR, any ) )
		nm.numPlaced = nm.placedScroll = 0;
	if ( NM_Button( NM_RIGHT_X + 208, ay, NM_RIGHT_W - 208, "Save as construct...", qfalse, any ) )
		NM_StartSaving( NM_SAVING_CONSTRUCT );
}

// a saved construct's NPCs, in place of the form
static void NM_DrawConstruct( const nmSaved_t *s ) {
	const float lh = NM_PLACED_Y - 6 - ( NM_LIST_Y + 42 );
	const int rows = (int)( ( lh - 4 ) / NM_ROW_H );
	float y = NM_LIST_Y;

	NM_ReadShownConstruct( s );
	NM_Box( NM_RIGHT_X, y, NM_RIGHT_W, 34, nmPanelLight );
	NM_TextClipped( NM_RIGHT_X + 6, y + 4, NM_RIGHT_W - 12, va( "%s " S_COLOR_GREY "NPC construct", s->name ), nmAccent );
	NM_TextClipped( NM_RIGHT_X + 6, y + 18, NM_RIGHT_W - 12, va( S_COLOR_GREY "%i NPCs, from %s", nm.numConstructNpcs, NM_SavePath( qtrue, s->name ) ), nmWhite );

	y += 42;
	NM_Box( NM_RIGHT_X, y, NM_RIGHT_W, lh, nmPanelLight );
	for ( int i = 0; i < nm.numConstructNpcs && i < rows; i++ ) {
		const nmSpawn_t *n = &nm.constructNpcs[i];

		if ( i == rows - 1 && nm.numConstructNpcs > rows ) {
			NM_Text( NM_RIGHT_X + 4, y + 2 + i * NM_ROW_H, va( S_COLOR_GREY "and %i more", nm.numConstructNpcs - i ), nmWhite );
			break;
		}
		NM_TextClipped( NM_RIGHT_X + 4, y + 2 + i * NM_ROW_H, NM_RIGHT_W - 8, va( "%s %s" S_COLOR_GREY "%s%s%s", n->type,
			n->targetname[0] ? va( S_COLOR_CYAN "%s ", n->targetname ) : "", nmTeamLabels[n->team],
			n->model[0] ? ", model " : "", n->model ), nmWhite );
	}
}

static void NM_DrawSpawn( void ) {
	const float bw = ( NM_LEFT_W - 4.0f * ( NM_SRC_COUNT - 1 ) ) / NM_SRC_COUNT;
	const nmType_t *t;
	const nmSaved_t *saved;
	const char *cmd;
	float y, x;
	int picked;

	NM_Field( NM_F_TYPE_SEARCH, NM_LEFT_X, NM_SEARCH_Y, NM_LEFT_W, "Type to search NPC types" );
	// where they come from
	for ( int i = 0; i < NM_SRC_COUNT; i++ ) {
		if ( NM_Button( NM_LEFT_X + i * ( bw + 4 ), NM_LIST_Y, bw, nmSourceLabels[i], (qboolean)( nm.source == i ), qtrue ) && nm.source != i ) {
			nm.source = (nmSource_t)i;
			nm.typeList.sel = nm.typeList.scroll = 0;
			NM_BuildRows();
		}
	}

	// the rows, and the NPCs of the one selected beside them
	nm.rowList.sel = nm.fields[NM_F_TYPE_SEARCH][0] ? -1 : nm.row;
	{
		const int before = nm.rowList.sel;
		// the list puts "none" on the first row: only a click picks one, and leaves the search
		const qboolean clicked = (qboolean)( nm.click && nm.clickX >= NM_LEFT_X && nm.clickX < NM_LEFT_X + NM_TREE_W - NM_ListBarW( nm.numRows, NM_TYPE_LIST_H )
			&& nm.clickY >= NM_TYPE_LIST_Y && nm.clickY < NM_TYPE_LIST_Y + NM_TYPE_LIST_H );

		NM_List( &nm.rowList, nm.numRows, NM_LEFT_X, NM_TYPE_LIST_Y, NM_TREE_W, NM_TYPE_LIST_H, NM_DrawTreeRow );
		if ( nm.rowList.sel != before && nm.rowList.sel >= 0 && ( before >= 0 || clicked ) )
			NM_SetRow( nm.rowList.sel );
	}
	NM_RebuildTypeView();
	picked = NM_List( &nm.typeList, nm.numTypeView, NM_TYPES_X, NM_TYPE_LIST_Y, NM_TYPES_W, NM_TYPE_LIST_H, NM_DrawTypeRow );
	if ( !nm.numTypeView ) {
		NM_Text( NM_TYPES_X + 4, NM_TYPE_LIST_Y + 1, nm.row == 0 && !nm.fields[NM_F_TYPE_SEARCH][0]
			? "Nothing saved yet" : "None match", nmDim );
	}
	NM_ApplySelection();
	t = NM_SelectedType();
	saved = NM_SelectedSaved();

	y = NM_SEARCH_Y;
	NM_Label( y, "Show" );
	for ( int i = 0; i < 3; i++ ) {
		static const char *labels[] = { "All", "Characters", "Vehicles" };

		if ( NM_Button( NM_CTRL_X + i * 84, y, 80, labels[i], (qboolean)( nm.show == i ), qtrue ) && nm.show != i ) {
			nm.show = (nmShow_t)i;
			NM_BuildRows();
		}
	}

	if ( saved && saved->construct ) {
		NM_DrawConstruct( saved );
		NM_DrawPlaced();
		NM_DrawCommandLines( NULL );
		if ( NM_Button( NM_RIGHT_X, NM_BUTTON_Y, NM_RIGHT_W, "Pick it up to put it down...  (Enter)", qfalse, (qboolean)( saved->count > 0 ) ) || picked >= 0 )
			NM_StartConstruct( saved->name );
		return;
	}

	y = NM_LIST_Y;
	NM_Box( NM_RIGHT_X, y, NM_RIGHT_W, 34, nmPanelLight );
	if ( t && saved ) {
		NM_TextClipped( NM_RIGHT_X + 6, y + 4, NM_RIGHT_W - 12, va( "%s " S_COLOR_GREY "saved NPC", saved->name ), nmAccent );
		NM_TextClipped( NM_RIGHT_X + 6, y + 18, NM_RIGHT_W - 12,
			va( S_COLOR_GREY "%s %s   %s", t->name, t->vehicle ? "vehicle" : "character", NM_SourceLabel( t ) ), nmWhite );
	} else if ( t ) {
		NM_TextClipped( NM_RIGHT_X + 6, y + 4, NM_RIGHT_W - 12, t->name, nmAccent );
		NM_TextClipped( NM_RIGHT_X + 6, y + 18, NM_RIGHT_W - 12,
			va( S_COLOR_GREY "%s   model %s   %s", t->vehicle ? "Vehicle" : "Character", t->model[0] ? t->model : "(none)", NM_SourceLabel( t ) ), nmWhite );
	} else {
		NM_Text( NM_RIGHT_X + 6, y + 4, "Pick an NPC type on the left", nmDim );
	}

	y += 44;
	NM_Label( y, "Targetname" );
	NM_Field( NM_F_TARGETNAME, NM_CTRL_X, y, NM_CTRL_W, "optional, names it for later commands" );

	y += 22;
	NM_Label( y, "Team" );
	for ( int i = 0; i < 3; i++ ) {
		if ( NM_Button( NM_CTRL_X + i * 94, y, 90, nmTeamLabels[i], (qboolean)( nm.team == i ), qtrue ) )
			nm.team = (nmTeam_t)i;
	}

	y += 22;
	NM_Label( y, "Model" );
	NM_Field( NM_F_MODEL, NM_CTRL_X, y, NM_CTRL_W - 74, t && t->vehicle ? "ignored on vehicles" : "optional, model or model/skin" );
	if ( NM_Button( NM_CTRL_X + NM_CTRL_W - 70, y, 70, "Browse...", qfalse, (qboolean)( nm.numSkins > 0 ) ) )
		NM_OpenPicker( NM_PICKER_MODEL );

	y += 22;
	NM_Label( y, "Scale" );
	NM_Field( NM_F_SCALE, NM_CTRL_X, y, 80, "default" );

	y += 22;
	NM_Label( y, "Location" );
	if ( NM_Button( NM_CTRL_X, y, 100, "In front of me", (qboolean)!nm.havePos, qtrue ) )
		nm.havePos = qfalse;
	if ( NM_Button( NM_CTRL_X + 104, y, 100, "Pick on map...", nm.havePos, qtrue ) )
		NM_StartPick( NM_PICK_SPAWN );
	if ( nm.havePos ) {
		x = NM_CTRL_X + 210;
		NM_TextClipped( x, y + 2, NM_RIGHT_X + NM_RIGHT_W - x, va( S_COLOR_GREY "%i %i %i, yaw %i",
			(int)nm.pos[0], (int)nm.pos[1], (int)nm.pos[2], nm.yaw ), nmWhite );
	}

	NM_DrawPlaced();

	cmd = NM_BuildSpawn();
	NM_DrawCommandLines( cmd ? va( "Will send: %s", cmd ) : NULL );
	if ( NM_Button( NM_RIGHT_X, NM_BUTTON_Y, NM_RIGHT_W - 188, "Spawn  (Enter)", qfalse, (qboolean)( t != NULL ) ) || picked >= 0 )
		NM_Spawn();
	if ( NM_Button( NM_RIGHT_X + NM_RIGHT_W - 184, NM_BUTTON_Y, 90, "Save NPC...", qfalse, (qboolean)( t != NULL ) ) )
		NM_StartSaving( NM_SAVING_NPC );
	if ( NM_Button( NM_RIGHT_X + NM_RIGHT_W - 90, NM_BUTTON_Y, 90, "Preview...", qfalse, (qboolean)( t != NULL ) ) ) {
		nm.preview = qtrue;
		nm.previewYaw = 0.0f;
		nm.focus = NM_F_NONE;
	}
}

// typing where to teleport to
static void NM_DrawExperimentalBox( void ) {
	const float w = 320, h = 120, x = ( SCREEN_WIDTH - w ) * 0.5f, y = 160;
	char text[MAX_STRING_CHARS], *line = text;
	float ly = y + 24;

	NM_Box( x, y, w, h, nmPanelOpaque );
	NM_Text( x + 8, y + 6, S_COLOR_YELLOW "Experimental features", nmWhite );
	// wrapped by words
	Q_strncpyz( text, RP_EXPERIMENTAL_WARNING, sizeof( text ) );
	while ( *line && ly < y + h - 34 ) {
		char *end = line, *lastSpace = NULL, keep;

		while ( *end ) {
			if ( *end == ' ' ) {
				keep = *end;
				*end = '\0';
				const qboolean fits = (qboolean)( NM_TextWidth( line ) <= w - 16 );
				*end = keep;
				if ( !fits )
					break;
				lastSpace = end;
			}
			end++;
		}
		if ( *end && lastSpace )
			end = lastSpace;
		keep = *end;
		*end = '\0';
		NM_Text( x + 8, ly, line, nmWhite );
		*end = keep;
		line = *end ? end + 1 : end;
		ly += 13;
	}
	if ( NM_Button( x + 8, y + h - 24, 80, "OK", qfalse, qtrue ) ) {
		CL_RpHooks_SetExperimental( qtrue );
		NM_Message( S_COLOR_YELLOW "Experimental features on, for this session" );
		nm.experimentalBox = qfalse;
		return;
	}
	if ( NM_Button( x + w - 88, y + h - 24, 80, "Cancel", qfalse, qtrue ) )
		nm.experimentalBox = qfalse;
}

static void NM_DrawTeleBox( void ) {
	static const char *labels[] = { "X", "Y", "Z", "Yaw" };
	const float w = 300, h = 118, x = ( SCREEN_WIDTH - w ) * 0.5f, y = 160, fw = ( w - 16 - 3 * 6 ) / 4;
	const nmNpc_t *npc = NM_SelectedNpc();
	const char *target = NM_ApplyTarget( qfalse );
	qboolean filled = qtrue;

	NM_Box( x, y, w, h, nmPanelOpaque );
	if ( !target )
		NM_Text( x + 8, y + 6, "Teleport to:", nmWhite );
	else if ( nm.apply == NM_APPLY_NAME )
		NM_TextClipped( x + 8, y + 6, w - 16, va( "Teleport the NPCs named " S_COLOR_YELLOW "%s" S_COLOR_WHITE " to:", target ), nmWhite );
	else
		NM_TextClipped( x + 8, y + 6, w - 16, va( "Teleport NPC " S_COLOR_CYAN "%s" S_COLOR_WHITE "%s to:", target, npc ? va( " (%s)", npc->type ) : "" ), nmWhite );

	for ( int i = 0; i < 4; i++ ) {
		const float fx = x + 8 + i * ( fw + 6 );

		NM_Text( fx, y + 22, labels[i], nmDim );
		NM_Field( (nmField_t)( NM_F_TELE_X + i ), fx, y + 34, fw, i == 3 ? "0" : "" );
		if ( i < 3 && !nm.fields[NM_F_TELE_X + i][0] )
			filled = qfalse;
	}

	// starting points
	if ( NM_Button( x + 8, y + 56, 138, "Where it is now", qfalse, (qboolean)( npc != NULL ) ) ) {
		vec3_t org;

		NM_NpcOrigin( npc, org );
		NM_SetTeleFields( org, npc->yaw );
	}
	if ( NM_Button( x + w - 146, y + 56, 138, "Where I am", qfalse, qtrue ) )
		NM_SetTeleFields( cl.snap.ps.origin, (int)cl.viewangles[YAW] );
	NM_Text( x + 8, y + 76, S_COLOR_GREY "Tab next box   Enter teleports   Esc cancels", nmWhite );

	if ( NM_Button( x + 8, y + h - 24, 120, "Teleport  (Enter)", qfalse, (qboolean)( target && filled ) ) ) {
		NM_TeleportToFields();
		return;
	}
	if ( NM_Button( x + w - 108, y + h - 24, 100, "Cancel  (Esc)", qfalse, qtrue ) ) {
		nm.teleBox = qfalse;
		nm.focus = NM_F_NONE;
		return;
	}
	if ( NM_Clicked( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT ) && !NM_InRect( x, y, w, h ) ) {
		nm.teleBox = qfalse;
		nm.focus = NM_F_NONE;
	}
}

// the name box for saving the form's NPC or what was put down
static void NM_DrawSaving( void ) {
	const float w = 340, h = 88, x = ( SCREEN_WIDTH - w ) * 0.5f, y = 170;
	const qboolean construct = (qboolean)( nm.saving == NM_SAVING_CONSTRUCT );
	const qboolean route = (qboolean)( nm.saving == NM_SAVING_ROUTE );
	const char *name = nm.fields[NM_F_SAVE_NAME];
	char path[MAX_QPATH];

	if ( route )
		Com_sprintf( path, sizeof( path ), "%s/%s.route", NM_ROUTE_DIR, name );
	else
		Q_strncpyz( path, NM_SavePath( construct, name ), sizeof( path ) );
	NM_Box( x, y, w, h, nmPanelOpaque );
	NM_TextClipped( x + 8, y + 6, w - 16, route ? va( nm.routeSaveOnly ? "Save the route of %i spots as:" : "Save the route of %i spots, then walk it, as:", nm.numRoute )
		: construct ? va( "Save the %i NPCs put down as a construct named:", nm.numPlaced )
		: "Save the form's NPC, with its settings, as:", nmWhite );
	NM_Field( NM_F_SAVE_NAME, x + 8, y + 22, w - 16, "letters, numbers, - and _" );
	if ( name[0] && FS_FileExists( path ) )
		NM_TextClipped( x + 8, y + 42, w - 16, S_COLOR_YELLOW "One with this name is there already; saving replaces it", nmWhite );
	else if ( route )
		NM_TextClipped( x + 8, y + 42, w - 16, va( S_COLOR_GREY "Goes in %s; npcmanager walk <npc> <name> walks it", NM_ROUTE_DIR ), nmWhite );
	else
		NM_TextClipped( x + 8, y + 42, w - 16, va( S_COLOR_GREY "Goes in %s, shown under Saved", construct ? NM_CONSTRUCT_DIR : NM_NPC_DIR ), nmWhite );
	if ( NM_Button( x + 8, y + h - 24, 100, route && !nm.routeSaveOnly ? "Save, walk (Enter)" : "Save  (Enter)", qfalse, (qboolean)( name[0] != '\0' ) ) ) {
		NM_DoSave();
		return;
	}
	if ( NM_Button( x + w - 108, y + h - 24, 100, "Cancel  (Esc)", qfalse, qtrue ) ) {
		nm.saving = NM_SAVING_NONE;
		return;
	}
	if ( NM_Clicked( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT ) && !NM_InRect( x, y, w, h ) )
		nm.saving = NM_SAVING_NONE;
}

// the NPC as it would spawn; Up/Down go through the types behind it
static void NM_DrawPreview( void ) {
	const float x = NM_PREVIEW_X, y = NM_PREVIEW_Y, w = NM_PREVIEW_W, h = NM_PREVIEW_H;
	const float vx = x + 8, vy = y + 22, vw = w - 16, vh = h - 136;
	const nmType_t *t = NM_SelectedType();
	float ly = vy + vh + 6;

	NM_PreviewUpdate( t, NULL );

	NM_Box( x, y, w, h, nmPanel );
	NM_Fill( x + 1, y + 1, w - 2, h - 2, nmPanel );
	NM_TextClipped( x + 8, y + 6, w - 16, t ? va( "Preview: " S_COLOR_YELLOW "%s", t->name ) : "Preview", nmWhite );
	NM_Box( vx, vy, vw, vh, nmPanelLight );
	if ( nm.wheel && NM_InRect( vx, vy, vw, vh ) ) {
		nm.previewYaw += nm.wheel * 5.0f;	// 15 degrees a notch
		nm.wheel = 0;
	}
	if ( t && nm.previewG2 )
		NM_RenderPreview( t, vx + 1, vy + 1, vw - 2, vh - 2 );
	else if ( nm.previewError )
		NM_TextClipped( vx + 8, vy + 8, vw - 16, nm.previewError, nmRed );

	if ( t ) {
		int scale = NM_PreviewScale( t );

		NM_TextClipped( vx, ly, vw, va( S_COLOR_GREY "Model  " S_COLOR_WHITE "%s%s", nm.previewModel,
			nm.previewOwnModel ? S_COLOR_GREY "  (its own)" : "" ), nmWhite );
		ly += NM_ROW_H;
		if ( nm.previewSkinMissing ) {
			NM_TextClipped( vx, ly, vw, "Skin not found, showing the default one", nmRed );
			ly += NM_ROW_H;
		}
		NM_TextClipped( vx, ly, vw, va( S_COLOR_GREY "Scale  " S_COLOR_WHITE "%i%%", scale ), nmWhite );
		ly += NM_ROW_H;
		if ( t->rgba[0] != 255 || t->rgba[1] != 255 || t->rgba[2] != 255 ) {
			NM_TextClipped( vx, ly, vw, va( S_COLOR_GREY "Tint   " S_COLOR_WHITE "%i %i %i", t->rgba[0], t->rgba[1], t->rgba[2] ), nmWhite );
			ly += NM_ROW_H;
		}
		if ( !t->vehicle && t->weapon[0] && Q_stricmp( t->weapon, "WP_NONE" ) ) {
			const char *held = t->weapon;

			if ( !Q_stricmp( t->weapon, "WP_SABER" ) )
				held = va( "WP_SABER  %s%s%s", t->saber[0] ? t->saber : "Kyle", t->saber2[0] ? " + " : "", t->saber2 );
			NM_TextClipped( vx, ly, vw, va( S_COLOR_GREY "Weapon " S_COLOR_WHITE "%s", held ), nmWhite );
			ly += NM_ROW_H;
		}
	}
	NM_Text( vx, y + h - 40, S_COLOR_GREY "Wheel turns it   Up/Down change type", nmWhite );

	if ( NM_Button( x + 8, y + h - 24, 120, "Spawn  (Enter)", qfalse, (qboolean)( t != NULL ) ) ) {
		NM_Spawn();
		NM_ClosePreview();
		return;
	}
	if ( NM_Button( x + w - 108, y + h - 24, 100, "Close  (Esc)", qfalse, qtrue ) ) {
		NM_ClosePreview();
		return;
	}
	if ( NM_Clicked( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT ) && !NM_InRect( x, y, w, h ) )
		NM_ClosePreview();
}

static void NM_DrawManage( void ) {
	const qboolean experimental = CL_RpHooks_Experimental();
	const nmNpc_t *npc;
	const char *target, *targetAll, *walker;
	float y, w;
	int named, marked;
	vec3_t org;

	NM_Field( NM_F_NPC_SEARCH, NM_LEFT_X, NM_SEARCH_Y, NM_LEFT_W, "Type to filter by number, type or name" );
	// Ctrl+click marks several, to walk them together
	if ( ( nm.held[A_CTRL] || nm.held[A_CTRL2] ) && NM_Clicked( NM_LEFT_X, NM_LIST_Y, NM_LEFT_W - NM_ListBarW( nm.numNpcView, NM_LIST_H ), NM_LIST_H ) ) {
		const int row = nm.npcList.scroll + (int)( ( nm.clickY - NM_LIST_Y ) / NM_ROW_H );

		if ( row < nm.numNpcView ) {
			nm.npcList.sel = row;
			NM_ToggleMark( nm.npcs[nm.npcView[row]].num );
		}
	}
	NM_List( &nm.npcList, nm.numNpcView, NM_LEFT_X, NM_LIST_Y, NM_LEFT_W, NM_LIST_H, NM_DrawNpcRow );
	if ( nm.npcList.sel < nm.numNpcView )
		nm.selNum = nm.npcs[nm.npcView[nm.npcList.sel]].num;
	if ( !nm.numNpcView ) {
		NM_Text( NM_LEFT_X + 4, NM_LIST_Y + 1,
			nm.listRequestTime ? "Asking the server..." : nm.numNpcs ? "No NPCs match" : "No NPCs in the map", nmDim );
	}
	npc = NM_SelectedNpc();

	y = NM_SEARCH_Y;
	if ( NM_Button( NM_RIGHT_X, y, 80, "Refresh", qfalse, qtrue ) )
		NM_RequestList();
	if ( NM_Button( NM_RIGHT_X + 84, y, 100, "Select on map...", qfalse, (qboolean)( nm.numNpcs > 0 ) ) )
		NM_StartPick( NM_PICK_SELECT );
	if ( nm.listTime ) {
		NM_TextClipped( NM_RIGHT_X + 192, y + 2, NM_RIGHT_W - 192,
			va( S_COLOR_GREY "%i NPCs, %is ago", nm.numNpcs, ( cls.realtime - nm.listTime ) / 1000 ), nmWhite );
	}

	y = NM_LIST_Y;
	NM_Box( NM_RIGHT_X, y, NM_RIGHT_W, 34, nmPanelLight );
	if ( nm.apply == NM_APPLY_MARKED ) {
		const char *marked = NM_WalkTarget();

		NM_TextClipped( NM_RIGHT_X + 6, y + 4, NM_RIGHT_W - 12, va( S_COLOR_CYAN "Marked  " S_COLOR_WHITE "%s", marked ? marked : "none in the list" ), nmWhite );
		NM_TextClipped( NM_RIGHT_X + 6, y + 18, NM_RIGHT_W - 12, S_COLOR_GREY "Ctrl+click marks more. They walk together, keeping their places", nmWhite );
	} else if ( npc ) {
		NM_NpcOrigin( npc, org );
		NM_TextClipped( NM_RIGHT_X + 6, y + 4, NM_RIGHT_W - 12,
			va( S_COLOR_CYAN "%i  " S_COLOR_YELLOW "%s  " S_COLOR_WHITE "%s", npc->num, npc->type, npc->name[0] ? npc->name : S_COLOR_GREY "(no targetname)" ), nmWhite );
		NM_TextClipped( NM_RIGHT_X + 6, y + 18, NM_RIGHT_W - 12, va( S_COLOR_GREY "at %i %i %i, yaw %i, %i units from you",
			(int)org[0], (int)org[1], (int)org[2], npc->yaw, (int)Distance( org, cl.snap.ps.origin ) ), nmWhite );
	} else {
		NM_Text( NM_RIGHT_X + 6, y + 4, "Pick an NPC on the left, or on the map", nmDim );
	}

	// what the commands run on
	y += 42;
	named = npc && npc->name[0] ? NM_CountNamed( npc->name ) : 0;
	NM_Label( y, "Apply to" );
	marked = NM_CountMarked();
	if ( NM_Button( NM_CTRL_X, y, 60, "This NPC", (qboolean)( nm.apply == NM_APPLY_ONE ), (qboolean)( npc != NULL ) ) )
		nm.apply = NM_APPLY_ONE;
	if ( NM_Button( NM_CTRL_X + 64, y, 100, named ? va( "Named alike (%i)", named ) : "Named alike", (qboolean)( nm.apply == NM_APPLY_NAME ), (qboolean)( named > 0 ) ) )
		nm.apply = NM_APPLY_NAME;
	// Ctrl+click in the list or on the map marks them
	if ( NM_Button( NM_CTRL_X + 168, y, 66, marked ? va( "Marked (%i)", marked ) : "Marked", (qboolean)( nm.apply == NM_APPLY_MARKED ), (qboolean)( marked > 0 ) ) )
		nm.apply = NM_APPLY_MARKED;
	if ( NM_Button( NM_CTRL_X + 238, y, 42, "All", (qboolean)( nm.apply == NM_APPLY_ALL ), qtrue ) )
		nm.apply = NM_APPLY_ALL;
	target = NM_ApplyTarget( qfalse );
	targetAll = NM_ApplyTarget( qtrue );
	walker = NM_WalkTarget();

	w = 66;
	y += 24;
	NM_Label( y, "Kill" );
	if ( NM_DangerButton( NM_CTRL_X, y, w * 2 + 4, nm.apply == NM_APPLY_ALL ? "Kill all NPCs" : "Kill", NM_CONFIRM_KILL, (qboolean)( targetAll != NULL ) ) )
		NM_SendManage( va( "npc kill %s", targetAll ), qtrue );
	if ( NM_DangerButton( NM_CTRL_X + ( w + 4 ) * 2, y, w * 2 + 4, "Kill all spawners", NM_CONFIRM_SPAWNERS, qtrue ) )
		NM_SendManage( "npc kill spawners", qtrue );

	y += 20;
	NM_Label( y, "Freeze" );
	if ( NM_Button( NM_CTRL_X, y, w, "Freeze", qfalse, (qboolean)( targetAll != NULL ) ) )
		NM_SendManage( va( "npc freeze %s 1", targetAll ), qfalse );
	if ( NM_Button( NM_CTRL_X + w + 4, y, w, "Unfreeze", qfalse, (qboolean)( targetAll != NULL ) ) )
		NM_SendManage( va( "npc freeze %s 0", targetAll ), qfalse );
	if ( NM_Button( NM_CTRL_X + ( w + 4 ) * 2, y, w, "Toggle", qfalse, (qboolean)( targetAll != NULL ) ) )
		NM_SendManage( va( "npc freeze %s !", targetAll ), qfalse );

	y += 20;
	NM_Label( y, "Emote" );
	NM_Field( NM_F_EMOTE, NM_CTRL_X, y, w * 2 + 4, "emote name" );
	if ( NM_Button( NM_CTRL_X + ( w + 4 ) * 2, y, w, "List...", qfalse, qtrue ) )
		NM_OpenPicker( NM_PICKER_EMOTE );
	if ( NM_Button( NM_CTRL_X + ( w + 4 ) * 3, y, 40, "Set", qfalse, (qboolean)( targetAll != NULL && nm.fields[NM_F_EMOTE][0] ) ) )
		NM_SendManage( va( "npc emote %s %s", nm.fields[NM_F_EMOTE], targetAll ), qfalse );
	if ( NM_Button( NM_CTRL_X + ( w + 4 ) * 3 + 44, y, 40, "Stop", qfalse, (qboolean)( targetAll != NULL ) ) )
		NM_SendManage( va( "npc emote none %s", targetAll ), qfalse );

	y += 20;
	NM_Label( y, "Follow" );
	if ( NM_Button( NM_CTRL_X, y, w, "Me", qfalse, (qboolean)( targetAll != NULL ) ) )
		NM_SendManage( va( "npc follow %i %s", clc.clientNum, targetAll ), qfalse );
	if ( NM_Button( NM_CTRL_X + w + 4, y, w, "Player...", qfalse, (qboolean)( targetAll != NULL ) ) )
		NM_OpenPicker( NM_PICKER_PLAYER );
	if ( NM_Button( NM_CTRL_X + ( w + 4 ) * 2, y, w, "Stop", qfalse, (qboolean)( targetAll != NULL ) ) )
		NM_SendManage( va( "npc follow none %s", targetAll ), qfalse );

	y += 20;
	NM_Label( y, "Dialog" );
	NM_Field( NM_F_DIALOG, NM_CTRL_X, y, w * 2 + 4, "dialog name" );
	if ( NM_Button( NM_CTRL_X + ( w + 4 ) * 2, y, w, "Set", qfalse, (qboolean)( targetAll != NULL && nm.fields[NM_F_DIALOG][0] ) ) )
		NM_SendManage( va( "npc dialog %s %s", nm.fields[NM_F_DIALOG], targetAll ), qfalse );
	if ( NM_Button( NM_CTRL_X + ( w + 4 ) * 3, y, w, "None", qfalse, (qboolean)( targetAll != NULL ) ) )
		NM_SendManage( va( "npc dialog none %s", targetAll ), qfalse );

	y += 20;
	NM_Label( y, "Teleport" );
	if ( NM_Button( NM_CTRL_X, y, 88, "Pick on map...", qfalse, (qboolean)( target != NULL ) ) )
		NM_StartPick( NM_PICK_TELE );
	if ( NM_Button( NM_CTRL_X + 92, y, 88, "In front of me", qfalse, (qboolean)( target != NULL ) ) ) {
		vec3_t pos;
		int yaw;

		NM_InFrontOfPlayer( pos, &yaw );
		NM_Teleport( pos, yaw );
	}
	if ( NM_Button( NM_CTRL_X + 184, y, NM_CTRL_W - 184, "Coordinates...", qfalse, (qboolean)( target != NULL ) ) )
		NM_OpenTeleBox();
	if ( nm.apply == NM_APPLY_ALL )
		NM_Text( NM_CTRL_X + ( w + 4 ) * 4, y + 2, S_COLOR_GREY "not on all", nmWhite );

	y += 20;
	NM_Label( y, "Walk" );
	if ( NM_Button( NM_CTRL_X, y, 88, "Route on map...", qfalse, (qboolean)( experimental && walker != NULL ) ) ) {
		nm.routeSaveOnly = qfalse;
		NM_StartPick( NM_PICK_ROUTE );
	}
	// a click runs it instead, and back
	if ( NM_Button( NM_CTRL_X + 92, y, 40, nm.routeRun ? "Run" : "Walk", qfalse, experimental ) )
		nm.routeRun = (qboolean)!nm.routeRun;
	// and round and round, until Stop
	if ( NM_Button( NM_CTRL_X + 136, y, 40, nm.routeLoop ? "Loop" : "Once", qfalse, experimental ) )
		nm.routeLoop = (qboolean)!nm.routeLoop;
	if ( NM_Button( NM_CTRL_X + 180, y, 48, "Again", qfalse, (qboolean)( experimental && walker != NULL && nm.routeName[0] ) ) )
		NM_Walk( nm.routeName );
	if ( NM_Button( NM_CTRL_X + 232, y, NM_CTRL_W - 232, "Stop", qfalse, (qboolean)( experimental && walker != NULL ) ) )
		NM_StopWalks( walker );

	y += 20;
	NM_Label( y, "Hologram" );
	if ( NM_Button( NM_CTRL_X, y, w, "Normal", qfalse, (qboolean)( targetAll != NULL ) ) )
		NM_SendManage( va( "npc hologram normal %s", targetAll ), qfalse );
	if ( NM_Button( NM_CTRL_X + w + 4, y, w, "Projector", qfalse, (qboolean)( targetAll != NULL ) ) )
		NM_SendManage( va( "npc hologram projector %s", targetAll ), qfalse );
	if ( NM_Button( NM_CTRL_X + ( w + 4 ) * 2, y, w, "Off", qfalse, (qboolean)( targetAll != NULL ) ) )
		NM_SendManage( va( "npc hologram off %s", targetAll ), qfalse );

	y += 20;
	NM_Label( y, "Talk" );
	if ( experimental ) {
		NM_Field( NM_F_TALK_TEXT, NM_CTRL_X, y, NM_CTRL_W - 44, "what it says, Enter to say it" );
	} else {
		NM_Box( NM_CTRL_X, y, NM_CTRL_W - 44, NM_CTRL_H, nmPanelLight );
		NM_TextClipped( NM_CTRL_X + 4, y + 2, NM_CTRL_W - 52, RP_EXPERIMENTAL_OFF ", top right", nmDim );
	}
	if ( NM_Button( NM_CTRL_X + NM_CTRL_W - 40, y, 40, "Say", qfalse, (qboolean)( experimental && target != NULL && nm.fields[NM_F_TALK_TEXT][0] ) ) )
		NM_Talk();
	y += 20;
	NM_Label( y, "  as" );
	if ( experimental ) {
		NM_Field( NM_F_TALK_NAME, NM_CTRL_X, y, w * 2 + 4, npc ? NM_TalkName( npc ) : "name" );
	} else {
		NM_Box( NM_CTRL_X, y, w * 2 + 4, NM_CTRL_H, nmPanelLight );
		NM_TextClipped( NM_CTRL_X + 4, y + 2, w * 2 - 4, npc ? NM_TalkName( npc ) : "name", nmDim );
	}
	// a click takes the next colour
	if ( NM_Button( NM_CTRL_X + ( w + 4 ) * 2, y, 40, experimental ? va( "^%cColor", nmTalkColors[nm.talkColor] ) : "Color", qfalse, experimental ) )
		nm.talkColor = ( nm.talkColor + 1 ) % ( ARRAY_LEN( nmTalkColors ) - 1 );
	if ( NM_Button( NM_CTRL_X + ( w + 4 ) * 2 + 44, y, w, "emtalk", (qboolean)( experimental && nm.talkEmote ), experimental ) )
		nm.talkEmote = (qboolean)!nm.talkEmote;

	y += 20;
	NM_Label( y, "Score" );
	if ( NM_Button( NM_CTRL_X, y, w * 2 + 4, "Show kills", qfalse, qtrue ) ) {
		// only takes a targetname, everything otherwise
		if ( nm.apply == NM_APPLY_NAME && npc && npc->name[0] )
			NM_SendManage( va( "npc score %s", npc->name ), qfalse );
		else
			NM_SendManage( "npc score", qfalse );
	}

	NM_DrawCommandLines( NULL );
}

// the Routes tab's NPC rows: who, and the route it walks
static void NM_DrawRouteNpcRow( int row, float x, float y, float w ) {
	const nmNpc_t *n = &nm.npcs[nm.npcView[row]];
	const char *route = CL_NpcWalk_RouteOf( n->num );
	const char *right = !route ? S_COLOR_GREY "-" : route[0] ? va( S_COLOR_GREEN "%s", route ) : S_COLOR_GREEN "walking";
	const float rw = Q_min( 90.0f, NM_TextWidth( right ) );

	if ( NM_IsMarked( n->num ) )
		NM_Fill( x - 3, y, 2, NM_ROW_H - 2, nmAccent );
	NM_Text( x, y, va( S_COLOR_CYAN "%i", n->num ), nmWhite );
	NM_TextClipped( x + 34, y, w - 40 - rw, n->name[0] ? va( "%s " S_COLOR_GREY "%s", n->name, n->type ) : va( S_COLOR_GREY "%s", n->type ), nmWhite );
	NM_TextClipped( x + w - rw, y, rw, right, nmWhite );
}

static void NM_DrawRouteRow( int row, float x, float y, float w ) {
	int walking = 0;

	for ( int i = 0; i < nm.numWalks; i++ ) {
		if ( !Q_stricmp( nm.walks[i].route, nm.routes[row].name ) )
			walking++;
	}
	NM_TextClipped( x, y, w, va( "%s " S_COLOR_GREY "%i spots, %s%s", nm.routes[row].name, nm.routes[row].numPts,
		nm.routes[row].speed > ( NM_WALK_SPEED + NM_RUN_SPEED ) * 0.5f ? "run" : "walk",
		walking ? va( S_COLOR_GREEN ", %i walking", walking ) : "" ), nmWhite );
}

static void NM_DrawWalkRow( int row, float x, float y, float w ) {
	const npcWalkInfo_t *wk = &nm.walks[row];
	const nmNpc_t *n = NM_FindNpc( wk->num );

	NM_TextClipped( x, y, w, va( S_COLOR_CYAN "%i " S_COLOR_WHITE "%s  " S_COLOR_GREEN "%s  " S_COLOR_GREY "%s, %s%s", wk->num,
		n ? ( n->name[0] ? n->name : n->type ) : "",
		wk->route[0] ? wk->route : "(no name)", wk->loop ? "round and round" : "once",
		wk->speed >= ( NM_WALK_SPEED + NM_RUN_SPEED ) * 0.5f ? "running" : "walking", wk->driver ? ", you send it" : "" ), nmWhite );
}

// every NPC, with the route it walks or none; routes given to one, those named alike, or the
// marked ones together
static void NM_DrawRoutes( void ) {
	static const char *showLabels[] = { "All", "Walking", "Not walking" };
	const qboolean experimental = CL_RpHooks_Experimental();
	const nmNpc_t *npc;
	const char *walker, *route;
	int named, marked, sel, picked, counts[3] = { 0, 0, 0 };
	float y;

	NM_ReadWalks();
	NM_RebuildNpcView();	// walks start and end

	// left: the NPCs, filtered by whether they walk
	for ( int i = 0; i < nm.numNpcs; i++ ) {
		counts[NM_RSHOW_ALL]++;
		counts[CL_NpcWalk_RouteOf( nm.npcs[i].num ) ? NM_RSHOW_WALKING : NM_RSHOW_IDLE]++;
	}
	NM_Field( NM_F_NPC_SEARCH, NM_LEFT_X, NM_SEARCH_Y, NM_LEFT_W, "Type to filter by number, type or name" );
	for ( int i = 0; i < 3; i++ ) {
		const float bw = ( NM_LEFT_W - 8 ) / 3.0f;

		if ( NM_Button( NM_LEFT_X + i * ( bw + 4 ), NM_LIST_Y, bw, va( "%s (%i)", showLabels[i], counts[i] ), (qboolean)( nm.routeShow == i ), qtrue ) ) {
			nm.routeShow = (nmRouteShow_t)i;
			NM_RebuildNpcView();
		}
	}
	if ( ( nm.held[A_CTRL] || nm.held[A_CTRL2] ) && NM_Clicked( NM_LEFT_X, NM_TYPE_LIST_Y, NM_LEFT_W - NM_ListBarW( nm.numNpcView, NM_TYPE_LIST_H ), NM_TYPE_LIST_H ) ) {
		const int row = nm.npcList.scroll + (int)( ( nm.clickY - NM_TYPE_LIST_Y ) / NM_ROW_H );

		if ( row < nm.numNpcView ) {
			nm.npcList.sel = row;
			NM_ToggleMark( nm.npcs[nm.npcView[row]].num );
		}
	}
	NM_List( &nm.npcList, nm.numNpcView, NM_LEFT_X, NM_TYPE_LIST_Y, NM_LEFT_W, NM_TYPE_LIST_H, NM_DrawRouteNpcRow );
	if ( nm.npcList.sel < nm.numNpcView )
		nm.selNum = nm.npcs[nm.npcView[nm.npcList.sel]].num;
	if ( !nm.numNpcView ) {
		NM_Text( NM_LEFT_X + 4, NM_TYPE_LIST_Y + 1, nm.listRequestTime ? "Asking the server..."
			: !nm.numNpcs ? "No NPCs in the map" : nm.routeShow == NM_RSHOW_WALKING ? "None walks a route" : "No NPCs match", nmDim );
	}
	npc = NM_SelectedNpc();

	// right: the selection, and what it walks
	y = NM_SEARCH_Y;
	if ( NM_Button( NM_RIGHT_X, y, 60, "Refresh", qfalse, qtrue ) ) {
		NM_RequestList();
		NM_RefreshRoutes();
	}
	if ( NM_Button( NM_RIGHT_X + 64, y, 110, "Routes on map...", qfalse, qtrue ) )
		NM_StartPick( NM_PICK_ROUTES );
	NM_TextClipped( NM_RIGHT_X + 182, y + 2, NM_RIGHT_W - 182, va( S_COLOR_GREY "%i walking, %i routes saved", nm.numWalks, nm.numRoutes ), nmWhite );

	y = NM_LIST_Y;
	NM_Box( NM_RIGHT_X, y, NM_RIGHT_W, 34, nmPanelLight );
	if ( nm.apply == NM_APPLY_MARKED ) {
		const char *list = NM_WalkTarget();

		NM_TextClipped( NM_RIGHT_X + 6, y + 4, NM_RIGHT_W - 12, va( S_COLOR_CYAN "Marked  " S_COLOR_WHITE "%s", list ? list : "none in the list" ), nmWhite );
		NM_TextClipped( NM_RIGHT_X + 6, y + 18, NM_RIGHT_W - 12, S_COLOR_GREY "Ctrl+click marks more. They walk a route together, keeping their places", nmWhite );
	} else if ( npc ) {
		const char *walks = CL_NpcWalk_RouteOf( npc->num );
		const npcWalkInfo_t *wk = NM_FindWalk( npc->num );

		NM_TextClipped( NM_RIGHT_X + 6, y + 4, NM_RIGHT_W - 12,
			va( S_COLOR_CYAN "%i  " S_COLOR_YELLOW "%s  " S_COLOR_WHITE "%s", npc->num, npc->type, npc->name[0] ? npc->name : S_COLOR_GREY "(no targetname)" ), nmWhite );
		NM_TextClipped( NM_RIGHT_X + 6, y + 18, NM_RIGHT_W - 12, !walks ? S_COLOR_GREY "Walks no route"
			: va( S_COLOR_GREEN "Walks %s" S_COLOR_GREY ", %s", walks[0] ? walks : "a route", wk && wk->loop ? "round and round" : "once" ), nmWhite );
	} else {
		NM_Text( NM_RIGHT_X + 6, y + 4, "Pick an NPC on the left, or on the map", nmDim );
	}

	y += 42;
	named = npc && npc->name[0] ? NM_CountNamed( npc->name ) : 0;
	marked = NM_CountMarked();
	NM_Label( y, "Give to" );
	if ( NM_Button( NM_CTRL_X, y, 60, "This NPC", (qboolean)( nm.apply == NM_APPLY_ONE ), (qboolean)( npc != NULL ) ) )
		nm.apply = NM_APPLY_ONE;
	if ( NM_Button( NM_CTRL_X + 64, y, 100, named ? va( "Named alike (%i)", named ) : "Named alike", (qboolean)( nm.apply == NM_APPLY_NAME ), (qboolean)( named > 0 ) ) )
		nm.apply = NM_APPLY_NAME;
	if ( NM_Button( NM_CTRL_X + 168, y, 80, marked ? va( "Marked (%i)", marked ) : "Marked", (qboolean)( nm.apply == NM_APPLY_MARKED ), (qboolean)( marked > 0 ) ) )
		nm.apply = NM_APPLY_MARKED;
	walker = NM_WalkTarget();

	// the saved routes
	y = NM_ROUTE_LIST_Y;
	NM_Label( y, "Routes" );
	sel = NM_SelectedRoute();
	picked = NM_List( &nm.routeList, nm.numRoutes, NM_CTRL_X, y, NM_CTRL_W, NM_ROUTE_LIST_H, NM_DrawRouteRow );
	if ( !nm.numRoutes )
		NM_Text( NM_CTRL_X + 4, y + 1, "None saved yet: New on map...", nmDim );
	if ( NM_SelectedRoute() != sel )
		nm.confirm = NM_CONFIRM_NONE;
	sel = NM_SelectedRoute();
	route = sel >= 0 ? nm.routes[sel].name : NULL;
	// a double-click gives it to the selection
	if ( picked >= 0 && experimental && walker )
		NM_Walk( nm.routes[picked].name );

	y = NM_ROUTE_LIST_Y + NM_ROUTE_LIST_H + 6;
	NM_Label( y, "Walk" );
	if ( NM_Button( NM_CTRL_X, y, 64, "Give it", qfalse, (qboolean)( experimental && walker && route ) ) )
		NM_Walk( route );
	if ( NM_Button( NM_CTRL_X + 68, y, 40, nm.routeRun ? "Run" : "Walk", qfalse, experimental ) )
		nm.routeRun = (qboolean)!nm.routeRun;
	if ( NM_Button( NM_CTRL_X + 112, y, 40, nm.routeLoop ? "Loop" : "Once", qfalse, experimental ) )
		nm.routeLoop = (qboolean)!nm.routeLoop;
	if ( NM_Button( NM_CTRL_X + 156, y, 44, "Stop", qfalse, (qboolean)( experimental && walker ) ) )
		NM_StopWalks( walker );
	if ( NM_DangerButton( NM_CTRL_X + 204, y, NM_CTRL_W - 204, "Stop all", NM_CONFIRM_STOP_WALKS, (qboolean)( experimental && nm.numWalks > 0 ) ) ) {
		char all[MAX_STRING_CHARS] = "";

		for ( int i = 0; i < nm.numWalks; i++ )
			Q_strcat( all, sizeof( all ), va( all[0] ? ",%i" : "%i", nm.walks[i].num ) );
		NM_StopWalks( all );
	}

	y += 20;
	NM_Label( y, "Route" );
	if ( NM_Button( NM_CTRL_X, y, 88, "New on map...", qfalse, qtrue ) )
		NM_StartRouteEdit( -1 );
	if ( NM_Button( NM_CTRL_X + 92, y, 88, "Change on map...", qfalse, (qboolean)( route != NULL ) ) )
		NM_StartRouteEdit( sel );
	if ( NM_DangerButton( NM_CTRL_X + 184, y, NM_CTRL_W - 184, "Delete", NM_CONFIRM_DELETE_ROUTE, (qboolean)( route != NULL ) ) )
		NM_DeleteRoute( sel );

	y += 20;
	NM_Label( y, "Map view" );
	if ( NM_Button( NM_CTRL_X, y, 110, nm.routeOutlines ? "Outlines shown" : "Outlines hidden", nm.routeOutlines, qtrue ) )
		nm.routeOutlines = (qboolean)!nm.routeOutlines;
	if ( NM_Button( NM_CTRL_X + 114, y, NM_CTRL_W - 114, nm.routeFocus ? "Only the one hovered" : "Every route", nm.routeFocus, qtrue ) )
		nm.routeFocus = (qboolean)!nm.routeFocus;

	// the walks going on; a click selects the NPC
	y = NM_WALK_LIST_Y;
	NM_Label( y, "Walking" );
	sel = nm.click && nm.clickX >= NM_CTRL_X && nm.clickX < NM_CTRL_X + NM_CTRL_W - NM_ListBarW( nm.numWalks, NM_WALK_LIST_H )
		&& nm.clickY >= y && nm.clickY < y + NM_WALK_LIST_H;
	NM_List( &nm.walkList, nm.numWalks, NM_CTRL_X, y, NM_CTRL_W, NM_WALK_LIST_H, NM_DrawWalkRow );
	if ( !nm.numWalks )
		NM_Text( NM_CTRL_X + 4, y + 1, "No NPC walks a route", nmDim );
	else if ( sel && nm.walkList.sel < nm.numWalks ) {
		nm.selNum = nm.walks[nm.walkList.sel].num;
		nm.routeShow = NM_RSHOW_ALL;
		if ( nm.apply == NM_APPLY_MARKED )
			nm.apply = NM_APPLY_ONE;
		NM_RebuildNpcView();
		NM_ListMove( &nm.npcList, nm.numNpcView, NM_NpcListRows(), 0 );
	}
	if ( !experimental )
		NM_TextClipped( NM_RIGHT_X, NM_BUTTON_Y - 30, NM_RIGHT_W, S_COLOR_YELLOW RP_EXPERIMENTAL_OFF ", top right, to walk them", nmWhite );
	else
		NM_DrawCommandLines( NULL );
}

// the model highlighted in the model picker, with the selected type's scale and tint
static void NM_DrawPickerPreview( float x, float y, float w, float h ) {
	static nmType_t plain;
	const nmType_t *t = NM_SelectedType(), *looks = t;
	const char *pick = nm.pickerList.sel < nm.numPickerView ? NM_PickerItem( nm.pickerView[nm.pickerList.sel] ) : NULL;

	// vehicles ignore the model box, so show the model as a character would wear it
	if ( !looks || looks->vehicle ) {
		plain.name = "";
		plain.rgba[0] = plain.rgba[1] = plain.rgba[2] = plain.rgba[3] = 255;
		plain.scale = 100;
		plain.weapon = plain.saber = plain.saber2 = "";
		looks = &plain;
	}

	NM_Box( x, y, w, h, nmPanelLight );
	if ( !pick ) {
		NM_PreviewFree();
		return;
	}
	NM_PreviewUpdate( looks, pick );
	if ( nm.wheel && NM_InRect( x, y, w, h ) ) {
		nm.previewYaw += nm.wheel * 5.0f;	// 15 degrees a notch
		nm.wheel = 0;
	}
	if ( nm.previewG2 )
		NM_RenderPreview( looks, x + 1, y + 1, w - 2, h - 2 );
	else if ( nm.previewError )
		NM_TextClipped( x + 6, y + 6, w - 12, nm.previewError, nmRed );
	if ( nm.previewSkinMissing )
		NM_TextClipped( x + 6, y + h - 16, w - 12, "Skin not found, showing default", nmRed );
}

static void NM_DrawPicker( void ) {
	static const char *titles[] = { "", "Pick a model", "Pick an emote", "Pick a player to follow" };
	// the model picker is wider, with a preview of the highlighted model on the right
	const qboolean models = (qboolean)( nm.picker == NM_PICKER_MODEL );
	const float x = models ? NM_PICKER_MODELS_X : NM_PICKER_X, y = NM_PICKER_Y, h = NM_PICKER_H;
	const float w = NM_PICKER_W + ( models ? NM_PICKER_PREVIEW_W : 0.0f ), lw = NM_PICKER_W - 16;
	int picked;

	NM_Box( x, y, w, h, nmPanel );
	NM_Fill( x + 1, y + 1, w - 2, h - 2, nmPanel );
	NM_Text( x + 8, y + 6, titles[nm.picker], nmWhite );
	NM_Field( NM_F_PICKER_SEARCH, x + 8, y + 22, lw, "Type to search" );
	if ( nm.picker == NM_PICKER_PLAYER )
		NM_RebuildPickerView();	// people come and go
	picked = NM_List( &nm.pickerList, nm.numPickerView, x + 8, y + 42, lw, h - 74, NM_DrawPickerRow );
	if ( models )
		NM_DrawPickerPreview( x + lw + 16, y + 22, NM_PICKER_PREVIEW_W - 8, h - 54 );
	if ( NM_Button( x + 8, y + h - 24, 100, "Choose  (Enter)", qfalse, (qboolean)( nm.numPickerView > 0 ) ) )
		picked = nm.pickerList.sel;
	if ( NM_Button( x + w - 108, y + h - 24, 100, "Cancel  (Esc)", qfalse, qtrue ) ) {
		nm.picker = NM_PICKER_NONE;
		return;
	}
	if ( picked >= 0 && picked < nm.numPickerView )
		NM_ChoosePicker( nm.pickerView[picked] );
	else if ( NM_Clicked( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT ) && !NM_InRect( x, y, w, h ) )
		nm.picker = NM_PICKER_NONE;
}

// numbers and names over the NPCs in view
static void NM_DrawLabels( void ) {
	// picking one, the label under the crosshair lights up
	const int hovered = nm.pick == NM_PICK_SELECT ? NM_NpcAtCursor() : -1;
	float x, y, tw;
	vec3_t org;

	for ( int i = 0; i < nm.numNpcs; i++ ) {
		const nmNpc_t *n = &nm.npcs[i];
		const char *label = n->name[0] ? va( "%i %s", n->num, n->name ) : va( "%i %s", n->num, n->type );
		qboolean sel = (qboolean)( n->num == nm.selNum || i == hovered || NM_IsMarked( n->num ) );

		NM_NpcOrigin( n, org );
		org[2] += 40.0f;
		if ( !NM_Project( org, &x, &y ) || x < 0 || x > SCREEN_WIDTH || y < 0 || y > SCREEN_HEIGHT )
			continue;
		tw = NM_TextWidth( label );
		NM_Box( x - tw * 0.5f - 3, y - 7, tw + 6, 14, sel ? nmHighlight : nmPanel );
		NM_Text( x - tw * 0.5f, y - 6, label, sel ? nmAccent : nmWhite );
	}
}

// a dotted line on screen, the dots as close however long it is
static void NM_DottedLine( float x1, float y1, float x2, float y2, const float *color ) {
	const int dots = Com_Clampi( 2, 120, (int)( sqrtf( Square( x2 - x1 ) + Square( y2 - y1 ) ) / 4.0f ) );

	for ( int i = 0; i <= dots; i++ )
		NM_Fill( x1 + ( x2 - x1 ) * i / dots - 1, y1 + ( y2 - y1 ) * i / dots - 1, 2, 2, color );
}

// a spot an NPC goes: a pole from its feet to its head, which way it faces, and a label
static void NM_DrawMarker( const vec3_t pos, int yaw, const char *label, const float *color ) {
	vec3_t feet, head, angles, forward, end;
	float fx, fy, hx, hy, x, y, ex, ey, tw;

	VectorCopy( pos, feet );
	feet[2] -= NM_GROUND_HEIGHT;
	VectorCopy( pos, head );
	head[2] += 40.0f;
	if ( !NM_Project( feet, &fx, &fy ) || !NM_Project( head, &hx, &hy ) || !NM_Project( pos, &x, &y ) )
		return;
	NM_DottedLine( fx, fy, hx, hy, color );
	VectorSet( angles, 0, (float)yaw, 0 );
	AngleVectors( angles, forward, NULL, NULL );
	VectorMA( pos, 32.0f, forward, end );
	if ( NM_Project( end, &ex, &ey ) )
		NM_DottedLine( x, y, ex, ey, color );
	tw = NM_TextWidth( label );
	NM_Box( hx - tw * 0.5f - 3, hy - 16, tw + 6, 14, nmPanel );
	NM_Text( hx - tw * 0.5f, hy - 15, label, color );
}

// where each of a group walking together stands at a spot: a dot, its number by it
static void NM_DrawGroupSpots( const vec3_t spot, const vec3_t *starts, int count, const vec3_t middle, const float *color ) {
	for ( int m = 0; m < count; m++ ) {
		vec3_t offset, at;
		float x, y;

		VectorSubtract( starts[m], middle, offset );
		offset[2] = 0.0f;
		CL_NpcWalk_GroupSpot( spot, offset, at );
		if ( NM_Project( at, &x, &y ) )
			NM_Fill( x - 2, y - 2, 4, 4, color );
	}
}

static const char *NM_MarkerLabel( const nmSpawn_t *s ) {
	return s->targetname[0] ? va( "%s %s", s->type, s->targetname ) : s->type;
}

// the routes walked now: each its colour, its spots numbered, its NPC labelled where it is; the
// selected saved route too, dimmer. Only the one hovered, or the selected NPC's, when asked
static void NM_DrawRoutesView( void ) {
	const int hovered = NM_RouteHover();
	int only = -1;
	float x, y, px, py, tw;

	if ( nm.routeFocus ) {
		only = hovered;
		for ( int i = 0; i < nm.numWalks && only < 0; i++ ) {
			if ( nm.walks[i].num == nm.selNum )
				only = i;
		}
	}
	// the NPCs not walking, by their labels, to pick them too
	for ( int i = 0; i < nm.numNpcs; i++ ) {
		const nmNpc_t *n = &nm.npcs[i];
		const char *label;
		vec3_t org;

		if ( NM_FindWalk( n->num ) || nm.routeFocus )
			continue;
		NM_NpcOrigin( n, org );
		org[2] += 40.0f;
		if ( !NM_Project( org, &x, &y ) || x < 0 || x > SCREEN_WIDTH || y < 0 || y > SCREEN_HEIGHT )
			continue;
		label = va( "%i %s", n->num, n->name[0] ? n->name : n->type );
		tw = NM_TextWidth( label );
		NM_Box( x - tw * 0.5f - 3, y - 7, tw + 6, 14, n->num == nm.selNum || NM_IsMarked( n->num ) ? nmHighlight : nmPanel );
		NM_Text( x - tw * 0.5f, y - 6, label, nmDim );
	}
	// the saved route selected in the tab, where it would be walked
	if ( nm.routeOutlines && !nm.routeFocus && NM_SelectedRoute() >= 0 ) {
		const int r = NM_SelectedRoute();

		for ( int j = 0; j < nm.routes[r].numPts; j++ ) {
			if ( !NM_Project( nm.routes[r].pts[j], &x, &y ) )
				continue;
			if ( j && NM_Project( nm.routes[r].pts[j - 1], &px, &py ) )
				NM_DottedLine( px, py, x, y, nmDim );
			NM_Fill( x - 2, y - 2, 4, 4, nmDim );
			if ( !j ) {
				tw = NM_TextWidth( nm.routes[r].name );
				NM_Text( x - tw * 0.5f, y - 18, nm.routes[r].name, nmDim );
			}
		}
	}
	for ( int i = 0; i < nm.numWalks; i++ ) {
		const npcWalkInfo_t *w = &nm.walks[i];
		const nmNpc_t *n = NM_FindNpc( w->num );
		const float *color = nmRouteColors[i % ARRAY_LEN( nmRouteColors )];
		const qboolean lit = (qboolean)( i == hovered || w->num == nm.selNum || NM_IsMarked( w->num ) );
		const char *label;
		vec3_t head;

		if ( only >= 0 ? i != only : nm.routeFocus )
			continue;
		if ( nm.routeOutlines ) {
			// from where it set off, through the spots
			for ( int j = 0; j < w->numPts; j++ ) {
				if ( !NM_Project( w->pts[j], &x, &y ) )
					continue;
				if ( j && NM_Project( w->pts[j - 1], &px, &py ) )
					NM_DottedLine( px, py, x, y, lit ? nmAccent : color );
				if ( j ) {
					label = va( "%i", j );
					NM_Fill( x - 2, y - 2, 4, 4, color );
					NM_Text( x + 4, y - 6, label, color );
				}
			}
			// round and round: back from the last to the first spot
			if ( w->loop && w->numPts > 2 && NM_Project( w->pts[w->numPts - 1], &px, &py ) && NM_Project( w->pts[1], &x, &y ) )
				NM_DottedLine( px, py, x, y, nmDim );
		}
		VectorCopy( w->pos, head );
		head[2] += 40.0f;
		if ( !NM_Project( head, &x, &y ) || x < 0 || x > SCREEN_WIDTH || y < 0 || y > SCREEN_HEIGHT )
			continue;
		label = va( "%i %s " S_COLOR_GREY "%s", w->num, n ? ( n->name[0] ? n->name : n->type ) : "", w->route[0] ? w->route : "" );
		tw = NM_TextWidth( label );
		NM_Box( x - tw * 0.5f - 3, y - 7, tw + 6, 14, lit ? nmHighlight : nmPanel );
		NM_Text( x - tw * 0.5f, y - 6, label, color );
	}
}

static void NM_DrawPick( void ) {
	const char *what;
	vec3_t pos, end, angles, forward;
	float x, y, ex, ey;
	int yaw;

	if ( nm.pick == NM_PICK_ROUTES )
		NM_ReadWalks();	// they go on walking, and their labels are drawn where they are
	else
		NM_DrawLabels();
	// the ones put down, waiting
	for ( int i = 0; i < nm.numPlaced; i++ )
		NM_DrawMarker( nm.placed[i].pos, nm.placed[i].yaw, NM_MarkerLabel( &nm.placed[i] ), nmPlacedColor );

	if ( nm.pick == NM_PICK_SELECT ) {
		what = "Aim at an NPC's label and click to select it, Ctrl+click to mark several to walk together.   Right-click or Esc cancels";
	} else if ( nm.pick == NM_PICK_ROUTE ) {
		const nmNpc_t *npc = nm.routeSaveOnly ? NULL : NM_SelectedNpc();
		qboolean have = (qboolean)( npc != NULL );
		vec3_t from, starts[NM_MAX_MARKED], middle;
		const int group = nm.routeSaveOnly ? 0 : NM_GroupStarts( starts );

		// from the NPC, or the middle of the marked ones, through the spots, then to where the
		// next would go
		if ( group ) {
			CL_NpcWalk_GroupMiddle( starts, group, middle );
			VectorCopy( middle, from );
			have = qtrue;
		} else if ( have ) {
			NM_NpcOrigin( npc, from );
		}
		for ( int i = 0; i < nm.numRoute; i++ ) {
			if ( have && NM_Project( from, &x, &y ) && NM_Project( nm.route[i], &ex, &ey ) )
				NM_DottedLine( x, y, ex, ey, nmPlacedColor );
			yaw = have ? NM_NormalizeYaw( NM_LegYaw( from, nm.route[i] ) ) : 0;
			NM_DrawMarker( nm.route[i], yaw, va( "%i", i + 1 ), nmPlacedColor );
			NM_DrawGroupSpots( nm.route[i], starts, group, middle, nmPlacedColor );
			VectorCopy( nm.route[i], from );
			have = qtrue;
		}
		if ( NM_TraceCursor( pos ) && NM_Project( pos, &ex, &ey ) ) {
			NM_Fill( ex - 3, ey - 3, 6, 6, nmAccent );
			if ( have && NM_Project( from, &x, &y ) )
				NM_DottedLine( x, y, ex, ey, nmAccent );
			NM_DrawGroupSpots( pos, starts, group, middle, nmAccent );
		}
		what = nm.routeSaveOnly ? va( "Click the spots of the route%s, one after the other.   Backspace takes the last back, Del clears them",
			nm.routeName[0] ? va( " %s", nm.routeName ) : "" )
			: group > 1 ? va( "Click the spots the %i marked NPCs %s to, keeping their places.   Backspace takes the last back, Del clears them",
			group, nm.routeRun ? "run" : "walk" )
			: va( "Click the spots it %s to, one after the other.   Backspace takes the last back, Del clears them",
			nm.routeRun ? "runs" : "walks" );
	} else if ( nm.pick == NM_PICK_ROUTES ) {
		NM_DrawRoutesView();
		what = "The routes walked now.   Click an NPC or its route to select it, Ctrl+click marks several.   Right-click or Esc goes back";
	} else if ( nm.pick == NM_PICK_PLACE ) {
		// what a click puts down, and where
		if ( NM_TraceCursor( pos ) ) {
			for ( int i = 0; i < nm.numHand; i++ ) {
				vec3_t spot;

				NM_HandSpot( i, pos, spot, &yaw );
				NM_DrawMarker( spot, yaw, NM_MarkerLabel( &nm.hand[i] ), nmAccent );
			}
		}
		what = va( "Click to put down %s%s, again for more.   Wheel turns it.   Backspace takes the last back",
			nm.handConstruct ? "the construct " : "", nm.handName );
	} else {
		if ( NM_TraceCursor( pos ) && NM_Project( pos, &x, &y ) ) {
			// where it goes and which way it will face
			yaw = NM_NormalizeYaw( NM_YawToView( pos ) + nm.pickYaw );
			VectorSet( angles, 0, (float)yaw, 0 );
			AngleVectors( angles, forward, NULL, NULL );
			VectorMA( pos, 32.0f, forward, end );
			NM_Fill( x - 3, y - 3, 6, 6, nmAccent );
			if ( NM_Project( end, &ex, &ey ) ) {
				for ( int i = 1; i <= 8; i++ )
					NM_Fill( x + ( ex - x ) * i / 8.0f - 1, y + ( ey - y ) * i / 8.0f - 1, 2, 2, nmAccent );
			}
		}
		what = nm.pick == NM_PICK_SPAWN
			? "Aim and click where to spawn it.   Wheel turns it.   Right-click or Esc cancels"
			: "Aim and click where to teleport to.   Wheel turns it.   Right-click or Esc cancels";
	}

	NM_Box( 10, 10, SCREEN_WIDTH - 20, nm.pick == NM_PICK_PLACE || nm.pick == NM_PICK_ROUTE || nm.pick == NM_PICK_ROUTES ? 43 : 30, nmPanel );
	NM_TextClipped( 16, 13, SCREEN_WIDTH - 32, what, nmWhite );
	NM_TextClipped( 16, 26, SCREEN_WIDTH - 32, S_COLOR_GREY "Mouse looks   WASD fly   Space/C up/down   Shift faster   Ctrl slower", nmWhite );
	if ( nm.pick == NM_PICK_PLACE ) {
		NM_TextClipped( 16, 39, SCREEN_WIDTH - 32, va( "%s" S_COLOR_WHITE "   Enter spawns them all   Right-click or Esc goes back, keeping them",
			nm.numPlaced ? va( S_COLOR_GREEN "%i put down", nm.numPlaced ) : S_COLOR_GREY "None put down yet" ), nmWhite );
	} else if ( nm.pick == NM_PICK_ROUTES ) {
		NM_TextClipped( 16, 39, SCREEN_WIDTH - 32, va( S_COLOR_GREEN "%i walking" S_COLOR_WHITE "   O: outlines %s   F: %s",
			nm.numWalks, nm.routeOutlines ? S_COLOR_YELLOW "shown" S_COLOR_WHITE : S_COLOR_YELLOW "hidden" S_COLOR_WHITE,
			nm.routeFocus ? S_COLOR_YELLOW "only the one hovered" S_COLOR_WHITE " (every route)" : S_COLOR_YELLOW "every route" S_COLOR_WHITE " (only the one hovered)" ), nmWhite );
	} else if ( nm.pick == NM_PICK_ROUTE ) {
		NM_TextClipped( 16, 39, SCREEN_WIDTH - 32, va( "%s" S_COLOR_WHITE "   R: %s   Enter saves it%s   Right-click or Esc goes back, keeping the spots",
			nm.numRoute ? va( S_COLOR_GREEN "%i of %i spots", nm.numRoute, NW_MAX_ROUTE_POINTS ) : S_COLOR_GREY "No spots yet",
			nm.routeSaveOnly ? "" : " and walks it",
			nm.routeRun ? S_COLOR_YELLOW "running" S_COLOR_WHITE " (walk)" : S_COLOR_YELLOW "walking" S_COLOR_WHITE " (run)" ), nmWhite );
	}

	// the crosshair, what a click picks
	NM_Fill( nm.cursorX - 8, nm.cursorY - 0.5f, 6, 1, nmWhite );
	NM_Fill( nm.cursorX + 2, nm.cursorY - 0.5f, 6, 1, nmWhite );
	NM_Fill( nm.cursorX - 0.5f, nm.cursorY - 8, 1, 6, nmWhite );
	NM_Fill( nm.cursorX - 0.5f, nm.cursorY + 2, 1, 6, nmWhite );
}

// drawn over the cgame, under the UI and console
void CL_NpcManager_Draw( void ) {
	char buf[MAX_STRING_CHARS];

	if ( !nm.open )
		return;

	if ( !nm.preview && nm.picker != NM_PICKER_MODEL && nm.previewG2 )
		NM_PreviewFree();

	if ( nm.pick != NM_PICK_NONE ) {
		NM_DrawPick();
		return;
	}

	// the warning, the picker, the name box and the preview sit on top and get the clicks first,
	// but are drawn last
	qboolean experimentalOpen = nm.experimentalBox;
	qboolean pickerOpen = (qboolean)( nm.picker != NM_PICKER_NONE );
	qboolean savingOpen = (qboolean)( nm.saving != NM_SAVING_NONE && !pickerOpen );
	qboolean teleOpen = (qboolean)( nm.teleBox && !pickerOpen && !savingOpen );
	qboolean previewOpen = (qboolean)( nm.preview && !pickerOpen && !savingOpen && !teleOpen );
	qboolean click = nm.click;
	int wheel = nm.wheel;
	if ( experimentalOpen || pickerOpen || savingOpen || teleOpen || previewOpen )
		nm.click = qfalse, nm.wheel = 0;

	NM_Fill( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, nmPanel );
	NM_Text( NM_LEFT_X, 14, "NPC Manager", nmWhite );
	if ( NM_Button( 104, 12, 54, "Spawn", (qboolean)( nm.tab == NM_TAB_SPAWN ), qtrue ) )
		NM_SetTab( NM_TAB_SPAWN );
	if ( NM_Button( 162, 12, 56, "Manage", (qboolean)( nm.tab == NM_TAB_MANAGE ), qtrue ) )
		NM_SetTab( NM_TAB_MANAGE );
	if ( NM_Button( 222, 12, 50, "Routes", (qboolean)( nm.tab == NM_TAB_ROUTES ), qtrue ) )
		NM_SetTab( NM_TAB_ROUTES );
	if ( NM_Button( NM_RIGHT_X + NM_RIGHT_W - 120, 12, 120, RP_EXPERIMENTAL_LABEL, CL_RpHooks_Experimental(), qtrue ) ) {
		if ( CL_RpHooks_Experimental() ) {
			CL_RpHooks_SetExperimental( qfalse );
			NM_Message( "Experimental features off" );
		} else {
			nm.experimentalBox = qtrue;
			nm.focus = NM_F_NONE;
		}
	}

	// the server's answer to the last command
	if ( nm.reply[0] && cls.realtime - nm.replyTime < 8000 )
		NM_TextClipped( NM_RIGHT_X, 14, NM_RIGHT_W - 124, va( S_COLOR_YELLOW "Server: " S_COLOR_WHITE "%s", nm.reply ), nmWhite );
	// and the manager's own
	if ( nm.message[0] && cls.realtime - nm.messageTime < NM_MESSAGE_MS )
		NM_TextClipped( NM_RIGHT_X, 29, NM_RIGHT_W, nm.message, nmWhite );

	if ( nm.tab == NM_TAB_SPAWN )
		NM_DrawSpawn();
	else if ( nm.tab == NM_TAB_ROUTES )
		NM_DrawRoutes();
	else
		NM_DrawManage();

	NM_Text( NM_LEFT_X, 440, S_COLOR_GREY "Up/Down select   Wheel scroll   Tab switch tab   Click a box to type in it", nmWhite );
	Com_sprintf( buf, sizeof( buf ), S_COLOR_GREY "Esc back / close   %s",
		Key_GetKey( NM_TOGGLE_CMD ) >= 0 ? "Your npcmanager bind closes" : "Tip: bind a key to npcmanager" );
	NM_Text( NM_LEFT_X, 452, buf, nmWhite );

	if ( experimentalOpen ) {
		nm.click = click;
		nm.wheel = wheel;
		NM_DrawExperimentalBox();
	} else if ( pickerOpen && nm.picker != NM_PICKER_NONE ) {
		nm.click = click;
		nm.wheel = wheel;
		NM_DrawPicker();
	} else if ( savingOpen && nm.saving != NM_SAVING_NONE ) {
		nm.click = click;
		nm.wheel = wheel;
		NM_DrawSaving();
	} else if ( teleOpen && nm.teleBox && nm.tab == NM_TAB_MANAGE ) {
		nm.click = click;
		nm.wheel = wheel;
		NM_DrawTeleBox();
	} else if ( previewOpen && nm.preview && nm.tab == NM_TAB_SPAWN ) {
		nm.click = click;
		nm.wheel = wheel;
		NM_DrawPreview();
	}

	// a click on nothing leaves the text box
	if ( nm.click )
		nm.focus = NM_F_NONE;
	nm.click = qfalse;
	nm.wheel = 0;

	WG_DrawOverlay();
	re->DrawStretchPic( nm.cursorX, nm.cursorY, 32, 32, 0, 0, 1, 1, cls.cursorShader );
}
