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

// cl_mapeditor.cpp -- a map editor on this screen
//
// A free camera over the map, the panel on the right in tabs:
// Map: the map's own entities, triggers boxed in orange and brush entities in blue, with
// all their keys; a brush entity can be hidden.
// Models: models placed here, carried to the crosshair, turned and scaled; other maps too,
// whole ("#name", as misc_bsp has them), to fuse with this one; and the map's own props. Those were melted into the map when it was built, so a click finds the
// triangles under the cursor and the ones joined to them; lifted out, they're hidden in
// the world and drawn as a model of their own, with the light they had. What's joined to
// the whole map is taken a brush at a time instead: the faces lying on the brush clicked.
// What's chosen has red, green and blue arrows to drag it along X, Y and Z.
// Lights: dynamic lights, their colour and reach, and a light style they can follow.
// Triggers: boxes that run a console command when the player walks in, or presses use
// looking at one within reach.
// Sounds: speakers playing a sound or a piece of music round and round, heard near them
// or everywhere on the map, as a target_speaker would; saved as one.
// Styles: the map's light styles, the switchable lights it was built with, and style 0,
// the light of the whole map, given a pattern and a colour of our own.
// Locks: RPMod's rplock on the map's doors and trigger_multiples, by rank, with a name; the
// server keeps those, not this screen.
// Fog: the fog the map was built with, over all of it or in volumes, given a colour and a
// thickness of our own, or taken away; and fog volumes of our own, boxes of fog anywhere.
//
// RPMod has no server side for any of it yet, so only this screen sees the edits. A map
// always starts without any: they're kept only by name as layouts, in
// mapedits/<map>/<name>.ent (entities a server could spawn later), loaded when wanted and
// shared, so no one's edits come back on their own over someone else's.
//
// Shared, an admin's edits go to everyone on the map with this client, hidden in rpnotify
// prints (see cl_rphooks.cpp):
//   cmd:map/begin/<id>/<parts>: <map>      a batch of edits for that map
//   cmd:map/add/<id>/<part>: <edit> | <edit> | ...
//   cmd:map/end/<id>/<parts>               shown in place of the player's own until
//                                          cmd:map/clear: <map>, or another map
// An edit is m (a model), p (a map prop), l (a light), a (a speaker), s (a light style) or
// h (a hidden brush entity), then its numbers; a model's path and a style's letters come last, with
// spaces put in so no word is long, and taken out again. Triggers stay with whoever made
// them: a print never runs a command. Only what's drawn changes: walls hidden still block,
// props moved don't, as the server knows nothing of it. Players who join later see the
// edits once they're shared again.

#include "client.h"
#include "qcommon/cm_public.h"
#include "cl_widgets.h"

#define ME_TOGGLE_CMD		"mapedit"
#define ME_DIR				"mapedits"
#define ME_EXT				".ent"
#define ME_HEADER			"// map edits, made with /mapedit; only this screen sees them for now\n"
#define ME_MAX_LAYOUTS		64
#define ME_SHARE_HOOK		"map"
#define ME_SHARE_LEN		700			// a hook's edits, well inside what RPMod passes on
#define ME_SHARE_WORD		32			// longer words go in pieces: RPMod chokes on long ones
#define ME_MAX_SHARE		96			// hooks waiting to go
#define ME_SHARE_BUF		( ME_MAX_SHARE * ( ME_SHARE_LEN + 4 ) )

#define ME_MAX_ENTS			2048
#define ME_MAX_KV			64
#define ME_MAX_ITEMS		256
#define ME_MAX_HIDDEN		64
#define ME_MAX_NAMES		16384
#define ME_NAME_POOL		( ME_MAX_NAMES * 40 )
#define ME_CMD_LEN			256

#define ME_FLY_SPEED		400.0f
#define ME_PANEL_W			236.0f
#define ME_PANEL_X			( SCREEN_WIDTH - ME_PANEL_W )
#define ME_ROW_H			12.0f
#define ME_CTRL_H			15.0f
#define ME_GAP				3.0f
#define ME_TEXT_SCALE		0.7f
#define ME_MESSAGE_MS		4000
#define ME_TRACE_DIST		8192.0f
#define ME_DRAW_DIST		3000.0f		// map entities further than this aren't drawn
#define ME_LABEL_DIST		700.0f		// nor named
#define ME_MAX_LINES		1100
#define ME_MAX_LABELS		48
#define ME_POINT_SIZE		8.0f		// a point entity's cross, and what a click finds it by
#define ME_FLASH_MS			3000
#define ME_MAX_SEEDS		32			// pieces a map prop is found from
#define ME_PROP_MAX_TRIS	20000		// bigger than this it's the map, not a prop
#define ME_PROP_MAX_SIZE	1024.0f
#define ME_MAX_SOUNDS		8192		// sounds and music to pick from
#define ME_SOUND_POOL		( ME_MAX_SOUNDS * 48 )
#define ME_SOUND_NEAR		256.0f		// a speaker's sound is at its loudest this near
#define ME_GIZMO_PX			6.0f		// an arrow is pointed at this near, on the 640x480 screen
#define ME_USE_REACH		96.0f		// a use trigger is used from this near
#define ME_MAX_USETIME		60000		// ms a use trigger can be held for
#define ME_CROUCH_MS		120			// a held use trigger's emote ended with a crouch this long
#define ME_CROUCH_KEY		"1023"		// the crouch's own key number: the player's C isn't let go with it
#define ME_MAX_FOGS			64
#define ME_FOG_NONE			65536.0f	// a fog this thin isn't seen

typedef enum {
	ME_TAB_MAP,
	ME_TAB_MODELS,
	ME_TAB_LIGHTS,
	ME_TAB_TRIGGERS,
	ME_TAB_SOUNDS,
	ME_TAB_STYLES,
	ME_TAB_LOCKS,
	ME_TAB_FOG,
	ME_NUM_TABS
} meTab_t;

static const char *meTabNames[ME_NUM_TABS] = { "Map", "Models", "Lights", "Triggers", "Sounds", "Styles", "Locks", "Fog" };
#define ME_TABS_PER_ROW		4

// what can be chosen
typedef enum {
	ME_SEL_NONE,
	ME_SEL_ENT,			// one of the map's own
	ME_SEL_ITEM,		// one of ours
	ME_SEL_STYLE,
	ME_SEL_PROP,		// a prop of the map's, not lifted yet
	ME_SEL_FOG
} meSelKind_t;

typedef enum {
	ME_ENT_POINT,
	ME_ENT_TRIGGER,
	ME_ENT_BRUSH
} meEntKind_t;

// the map's entities, from the bsp's entity string
typedef struct {
	int			ofs;				// where its { is in the string
	char		classname[48];
	char		targetname[48];
	int			kind;
	int			bmodel;				// its *N, 0 for none
	qboolean	hasOrigin;
	vec3_t		origin;
	vec3_t		mins, maxs;			// in the world, for a brush entity
	qboolean	hidden;
} meEnt_t;

typedef enum {
	ME_ITEM_MODEL,
	ME_ITEM_LIGHT,
	ME_ITEM_TRIGGER,
	ME_ITEM_PROP,		// a prop of the map's, lifted out
	ME_ITEM_SPEAKER,	// a sound playing round and round
	ME_ITEM_FOG			// a fog volume of ours: a box, its colour, how thick
} meItemKind_t;

typedef struct {
	int			kind;
	vec3_t		org;

	// a model
	char		model[MAX_QPATH];	// "models/....md3"
	vec3_t		ang;
	float		scale;
	qhandle_t	h;
	qboolean	registered;

	// a light; it follows a light style unless that's -1
	vec3_t		color;
	float		radius;
	int			style;

	// a speaker: its sound, heard everywhere or fading with distance, playing or not
	char		noise[MAX_QPATH];
	qboolean	global;
	qboolean	off;
	sfxHandle_t	sfx;
	qboolean	sfxRegistered;

	// a trigger: a box that size either way from org, walked into or used, with a name
	vec3_t		size;
	char		command[ME_CMD_LEN];
	qboolean	once;
	qboolean	inside, fired;
	qboolean	use;
	char		name[48];			// what the storyteller knows it by
	char		display[48];		// what a use trigger is shown as, looked at
	int			usetime;			// a use trigger: the use key held this many ms, 0 a press (JKA's usetime)

	// a fog volume: the box as a trigger's, its colour as a light's, and how deep it takes to
	// see nothing through it
	float		fogDepth;

	// a map prop: the pieces it's found from, surface and triangle, where it was, and
	// whether it's gone; org, ang and scale are a model's
	int			seeds[ME_MAX_SEEDS][2];
	int			numSeeds;
	vec3_t		pivot;
	qboolean	removed;
} meItem_t;

// a triangle of the world: its surface, which of the surface's it is, and its corners
typedef struct {
	int			surf, local;
	int			v[3];
} meTri_t;

// a corner where it is, to find the triangles that share it
typedef struct {
	int			x, y, z;
	int			tri;
} meVertKey_t;

typedef struct {
	qboolean	override;
	char		pattern[MAX_QPATH];	// a dark .. z full, one letter each 50 ms
	vec3_t		color;

	// the map's surfaces lit by it
	int			surfaces;
	vec3_t		mins, maxs;
} meStyle_t;

// a fog of the map's: as the map has it, and ours over it
typedef struct {
	vec3_t		mapColor;
	float		mapDepth;
	vec3_t		mins, maxs;
	qboolean	override;
	vec3_t		color;
	float		depth;				// how deep it takes to see nothing through it
} meFog_t;

typedef struct {
	const char	*name;
	const char	*pattern;
} meStylePreset_t;

static const meStylePreset_t meStylePresets[] = {
	{ "Steady",		"z" },
	{ "Off",		"a" },
	{ "Half",		"m" },
	{ "Flicker",	"mmnmmommommnonmmonqnmmo" },
	{ "Candle",		"mmmmmaaaaammmmmaaaaaabcdefgabcdefg" },
	{ "Pulse",		"abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcb" },
	{ "Strobe",		"mamamamamama" },
	{ "Blink",		"zzzzzzzzzzaaaaaaaaaa" },
};

// a style's colour and pattern together, for the whole map's light most of all
typedef struct {
	const char	*name;
	vec3_t		color;
	const char	*pattern;
} meStyleLook_t;

static const meStyleLook_t meStyleLooks[] = {
	{ "Normal",		{ 1.0f, 1.0f, 1.0f },		"z" },
	{ "Dim",		{ 0.55f, 0.55f, 0.55f },	"z" },
	{ "Night",		{ 0.3f, 0.36f, 0.6f },		"z" },
	{ "Dusk",		{ 1.0f, 0.62f, 0.42f },		"z" },
	{ "Alarm",		{ 1.0f, 0.22f, 0.18f },		"mnopqrstuvwxyzyxwvutsrqpon" },
	{ "Blackout",	{ 1.0f, 1.0f, 1.0f },		"b" },
	{ "Storm",		{ 0.45f, 0.5f, 0.65f },		"jjjjjjjjjjjjjjjjjjjjzjzjjjjjjjjjjjjjjjj" },
	{ "Eerie",		{ 0.45f, 0.9f, 0.6f },		"kklmnmlkkjjkllmnnmlk" },
};

static const vec3_t meLightColors[] = {
	{ 1.0f, 1.0f, 1.0f },
	{ 1.0f, 0.8f, 0.55f },
	{ 0.6f, 0.75f, 1.0f },
	{ 1.0f, 0.2f, 0.15f },
	{ 0.2f, 1.0f, 0.3f },
	{ 0.2f, 0.4f, 1.0f },
	{ 1.0f, 0.4f, 1.0f },
};

// a text box being typed in
typedef enum {
	ME_FIELD_NONE,
	ME_FIELD_FILTER,		// the map's entities, by class or name
	ME_FIELD_SEARCH,		// the models
	ME_FIELD_COMMAND,		// a trigger's
	ME_FIELD_PATTERN,		// a light style's
	ME_FIELD_LAYOUT,		// a layout's name
	ME_FIELD_SOUND,			// a speaker's sound
	ME_FIELD_LOCKNAME,		// a lock's name
	ME_FIELD_TRIGGERNAME,	// a trigger's name, the storyteller's
	ME_FIELD_TRIGGERDISPLAY	// what a use trigger is shown as
} meField_t;

typedef struct {
	qboolean	open;
	int			font;
	int			tab;

	// the map the edits are for, "" before one is loaded
	char		map[MAX_QPATH];
	qboolean	reparse;			// the cgame restarted: the map's own are read again
	const char	*entString;
	meEnt_t		ents[ME_MAX_ENTS];
	int			numEnts;
	qboolean	showTriggers, showBrushes, showPoints;
	char		filter[32];
	int			entScroll;
	char		kv[ME_MAX_KV][2][MAX_QPATH];
	int			numKv, kvScroll, kvEnt;

	// ours
	meItem_t	items[ME_MAX_ITEMS];
	int			numItems;
	int			itemScroll[ME_NUM_TABS];
	meStyle_t	styles[MAX_LIGHT_STYLES];
	qboolean	allStyles;
	int			styleScroll;
	int			flashStyle, flashUntil;
	meFog_t		fogs[ME_MAX_FOGS];	// 1 on, as the renderer counts them
	int			numFogs, globalFog;
	qboolean	fogsRead, fogsDirty, fogsShown;
	int			fogScroll;
	qboolean	fogAdded;			// the tab shows ours, not the map's
	unsigned	fogItemsSum;		// ours as they were last applied
	qhandle_t	hidden[ME_MAX_HIDDEN];
	int			numHidden;
	qboolean	dirty;

	int			selKind, sel;
	int			hoverKind, hover;
	qboolean	grabbing;
	vec3_t		grabFrom;

	// the free camera
	vec3_t		camOrg, camAng;
	float		camFov;
	qboolean	flying;
	qboolean	held[MAX_KEYS];
	int			lastFrameTime;
	qboolean	haveAim;
	vec3_t		aim, aimNormal;

	// the player's view, to start from
	qboolean	havePlayerView;
	vec3_t		playerOrg, playerAng;

	// input; clicks and the wheel go to the control drawn under them
	float		cursorX, cursorY;
	qboolean	click;
	float		clickX, clickY;
	int			wheel;
	int			field;

	// models to choose from
	qboolean	indexed;
	char		*names[ME_MAX_NAMES];
	int			numNames;
	int			poolUsed;
	char		search[48];
	int			searchScroll;
	char		pickModel[MAX_QPATH];

	// sounds and music to choose from
	qboolean	soundsIndexed;
	char		*sounds[ME_MAX_SOUNDS];
	int			numSounds;
	int			soundPoolUsed;
	int			soundScroll;

	// the view drawn, for picking and labels
	qboolean	haveView;
	vec3_t		viewOrg;
	vec3_t		viewAxis[3];
	float		viewFovX, viewFovY;

	// the world's triangles, for its props
	vec3_t		*bspVerts;
	int			numBspVerts;
	meTri_t		*tris;
	int			numTris;
	int			*surfFirstTri;		// -1 for a surface that isn't in it
	int			numBspSurfs;
	meVertKey_t	*keys;
	int			numKeys;
	int			*triStamp;
	int			stamp;
	byte		*triTaken;			// by a lifted prop
	int			*compTris;			// what was found last
	int			numCompTris;
	qboolean	propsDirty, propsShown;

	// the world's solid brushes, to take one whole: their planes, and boxes to skip them by
	float		( *brushPlanes )[4];
	int			*brushFirst, *brushNum;
	vec3_t		*brushMins, *brushMaxs;
	int			numBrushes;
	int			candBrush;			// the brush under the last click, -1 none

	// the arrows on what's chosen: the one pointed at, the one dragged and from where
	int			gizmoAxis, dragAxis;
	vec3_t		dragFrom;
	float		dragT0;

	// use triggers: the use key as it was, and the one looked at
	qboolean	useDown;
	int			useAim;
	int			holdItem;			// held on with the use key, -1 none
	int			holdStart;
	qboolean	holdDone;			// used: not again until the key is let go
	qboolean	holdEmote;			// emconsole played while it's held
	int			crouchUntil;		// the crouch ending it, let go then; 0 none

	// opened from the storyteller to click a trigger for a step: 0 a map trigger, 1 a door or
	// button, 2 a use trigger; -1 not
	int			storyPick;

	// a prop chosen in the map, not lifted yet
	int			candSeeds[ME_MAX_SEEDS][2];
	int			numCandSeeds;
	int			candTris;
	vec3_t		candMins, candMaxs;

	int			numLines;
	char		message[MAX_STRING_CHARS];
	int			messageTime;

	// layouts: edits kept by name, the panel showing them in place of a tab
	qboolean	showLayouts;
	char		layoutName[MAX_QPATH];
	char		layouts[ME_MAX_LAYOUTS][MAX_QPATH];
	int			numLayouts, layoutScroll, layoutSel;
	int			confirmTime;
	qboolean	experimentalBox;	// the warning before turning experimental features on

	// shared: ours going out, a paced hook at a time; someone's coming in
	char		share[ME_MAX_SHARE][ME_SHARE_LEN + 64];
	int			numShare, shareSent, nextShare;
	int			shareId;			// ours, left alone when it comes back
	qboolean	shared;				// what's shown is someone's, not ours
	int			rxId, rxParts, rxGot;
	char		rxBuf[ME_SHARE_BUF];
	int			rxLen;
} meState_t;

static meState_t me;
static char meNamePool[ME_NAME_POOL];
static char meSoundPool[ME_SOUND_POOL];
static cvar_t *rpc_mapEdits;
static cvar_t *rpc_mapShare;

static vec4_t meWhite		= { 1.0f, 1.0f, 1.0f, 1.0f };
static vec4_t mePanel		= { 0.0f, 0.0f, 0.0f, 0.7f };
static vec4_t mePanelLight	= { 0.15f, 0.15f, 0.15f, 0.85f };
static vec4_t meFocus		= { 0.1f, 0.2f, 0.3f, 0.9f };
static vec4_t meHighlight	= { 0.2f, 0.45f, 0.8f, 0.85f };
static vec4_t meHoverBox	= { 0.28f, 0.28f, 0.28f, 0.9f };
static vec4_t meBorder		= { 0.5f, 0.5f, 0.5f, 0.8f };
static vec4_t meDim			= { 0.65f, 0.65f, 0.65f, 1.0f };
static vec4_t mePanelOpaque	= { 0.05f, 0.05f, 0.05f, 0.95f };	// a box over the rest, which would show through

static const byte meTriggerColor[4]	= { 255, 140, 30, 255 };
static const byte meBrushColor[4]	= { 70, 140, 255, 255 };
static const byte mePointColor[4]	= { 90, 230, 110, 255 };
static const byte meHiddenColor[4]	= { 120, 120, 120, 160 };
static const byte meItemColor[4]	= { 60, 220, 255, 255 };
static const byte meOurTrigger[4]	= { 255, 230, 40, 255 };
static const byte meSelColor[4]		= { 255, 255, 255, 255 };
static const byte meAimColor[4]		= { 255, 140, 40, 255 };
static const byte meReachColor[4]	= { 255, 255, 255, 70 };
static const byte meCandColor[4]	= { 255, 90, 200, 255 };
static const byte meSpeakerColor[4]	= { 190, 120, 255, 255 };
static const byte meLockedColor[4]	= { 255, 50, 50, 255 };
static const byte meUseTrigger[4]	= { 80, 230, 255, 255 };
static const byte meAxisColors[3][4] = { { 255, 60, 60, 255 }, { 60, 230, 60, 255 }, { 70, 120, 255, 255 } };
static const byte meAxisLit[4]		= { 255, 230, 40, 255 };
static const vec3_t meAxes[3]		= { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };

static int ME_Round( float f ) {
	return (int)( f < 0 ? f - 0.5f : f + 0.5f );
}

static void ME_Message( const char *text ) {
	Q_strncpyz( me.message, text, sizeof( me.message ) );
	me.messageTime = cls.realtime;
}

static void ME_Changed( void ) {
	me.dirty = qtrue;
}

// the edits show and run outside the editor too, unless rpc_mapEdits is off
static qboolean ME_Shown( void ) {
	return (qboolean)( me.open || ( rpc_mapEdits && rpc_mapEdits->integer ) );
}

static meItem_t *ME_SelectedItem( void ) {
	if ( me.selKind != ME_SEL_ITEM || me.sel < 0 || me.sel >= me.numItems )
		return NULL;
	return &me.items[me.sel];
}

static meEnt_t *ME_SelectedEnt( void ) {
	if ( me.selKind != ME_SEL_ENT || me.sel < 0 || me.sel >= me.numEnts )
		return NULL;
	return &me.ents[me.sel];
}

/*
===============================================================================

Locks: RPMod's rplock on the map's doors and triggers

Only a func_door or a trigger_multiple takes one. It's added by its targetname when every
entity with that name can take it (a pair of doors locks together), else by its entity
number, read from the baselines the server sent with the map. "rplock list" is read, its
reply kept out of the console, to show what's locked.

===============================================================================
*/

#define ME_MAX_LOCKS		256
#define ME_LOCK_HEADER		"List of all current locks in this map:"
#define ME_LOCK_LINE_GAP	1500	// ms between the list's lines before it's over
#define ME_LOCK_ASK_MS		3000	// the list asked for comes this soon
#define ME_LOCK_REPLY_MS	4000	// prints this soon after a command are its reply
#define ME_LOCK_QUEUE		8
#define ME_LOCK_NAME_LEN	64

typedef struct {
	int		num;
	int		rank;
	char	rankName[48];
	char	name[ME_LOCK_NAME_LEN];
	char	type[32];
	char	targetname[64];		// "" for none
} meLock_t;

static struct {
	meLock_t	locks[ME_MAX_LOCKS];
	int			numLocks;
	qboolean	haveList;			// read since the map came up
	int			askTime;			// the list asked for, its reply hidden; 0 not waiting
	qboolean	ours;				// the list being read was asked for here
	int			lineTime;			// the last of its lines, 0 not reading one
	char		queue[ME_LOCK_QUEUE][MAX_STRING_CHARS];	// out one at a time, as the server takes them
	int			numQueue;
	char		reply[MAX_STRING_CHARS];
	int			replyTime, sentTime;
	int			rank;				// what a lock is added with
	char		name[ME_LOCK_NAME_LEN];
	qboolean	showDoors, showTriggers, onlyLocked;
	int			scroll;
	int			confirmTime;		// "Remove every lock" armed
} ml;

static qboolean ME_LocksTab( void ) {
	return (qboolean)( me.open && me.tab == ME_TAB_LOCKS && !me.showLayouts );
}

static qboolean ME_IsDoor( const meEnt_t *e ) {
	return (qboolean)!Q_stricmp( e->classname, "func_door" );
}

static qboolean ME_Lockable( const meEnt_t *e ) {
	return (qboolean)( ME_IsDoor( e ) || !Q_stricmp( e->classname, "trigger_multiple" ) );
}

// its number on the server, from the baselines or what's in view; -1 not known
static int ME_EntNumber( const meEnt_t *e ) {
	if ( !e->bmodel )
		return -1;
	for ( int i = MAX_CLIENTS; i < MAX_GENTITIES; i++ ) {
		const entityState_t *b = &cl.entityBaselines[i];

		if ( b->number == i && b->solid == SOLID_BMODEL && b->modelindex == e->bmodel )
			return i;
	}
	for ( int i = 0; i < cl.snap.numEntities; i++ ) {
		const entityState_t *es = &cl.parseEntities[( cl.snap.parseEntitiesNum + i ) & ( MAX_PARSE_ENTITIES - 1 )];

		if ( es->solid == SOLID_BMODEL && es->modelindex == e->bmodel )
			return es->number;
	}
	return -1;
}

// how many share its targetname, and whether every one of them takes a lock
static int ME_NamedAlike( const meEnt_t *e, qboolean *allLockable ) {
	int count = 0;

	*allLockable = qtrue;
	if ( !e->targetname[0] )
		return 0;
	for ( int i = 0; i < me.numEnts; i++ ) {
		if ( Q_stricmp( me.ents[i].targetname, e->targetname ) )
			continue;
		count++;
		if ( !ME_Lockable( &me.ents[i] ) )
			*allLockable = qfalse;
	}
	return count;
}

// what rplock is given for it: its targetname, or its number; NULL when neither will do
static const char *ME_LockTarget( const meEnt_t *e ) {
	qboolean allLockable;
	int num;

	if ( ME_NamedAlike( e, &allLockable ) && allLockable )
		return va( "%s", e->targetname );
	num = ME_EntNumber( e );
	return num >= 0 ? va( "%i", num ) : NULL;
}

// its lock in the list read, NULL none
static const meLock_t *ME_FindLock( const meEnt_t *e ) {
	const int num = ME_EntNumber( e );

	for ( int i = 0; i < ml.numLocks; i++ ) {
		const meLock_t *l = &ml.locks[i];

		if ( num >= 0 ? l->num == num : ( e->targetname[0] && !Q_stricmp( l->targetname, e->targetname ) && !Q_stricmp( l->type, e->classname ) ) )
			return l;
	}
	return NULL;
}

static const char *ME_RankLabel( int rank, const char *rankName ) {
	if ( rank < 0 )
		return "no one";
	if ( rankName && rankName[0] )
		return va( "rank %i, %s", rank, rankName );
	return rank ? va( "rank %i", rank ) : "rank 0, no rank";
}

// a command for the server, after the ones waiting; opened from the storyteller, a lock goes
// in the story instead. The list and trace are never a step
static void ME_LockSend( const char *cmd ) {
	const qboolean story = (qboolean)( Q_stricmpn( cmd, "rplock list", 11 ) && Q_stricmpn( cmd, "rplock trace", 12 ) );

	if ( story && CL_Storyteller_Capture( cmd ) ) {
		ME_Message( S_COLOR_GREEN "Put in the story" );
		return;
	}
	for ( int i = 0; i < ml.numQueue; i++ ) {
		if ( !Q_stricmp( ml.queue[i], cmd ) )
			return;
	}
	if ( ml.numQueue >= ME_LOCK_QUEUE ) {
		ME_Message( S_COLOR_YELLOW "Still sending the last ones" );
		return;
	}
	Q_strncpyz( ml.queue[ml.numQueue++], cmd, sizeof( ml.queue[0] ) );
	// what's locked now, once it's done
	if ( story && ml.numQueue < ME_LOCK_QUEUE )
		Q_strncpyz( ml.queue[ml.numQueue++], "rplock list", sizeof( ml.queue[0] ) );
}

// every frame: the next command once the server's flood protection takes it
static void ME_RunLocks( void ) {
	if ( ml.askTime && cls.realtime - ml.askTime > ME_LOCK_ASK_MS )
		ml.askTime = 0;
	if ( !ml.numQueue || cls.state != CA_ACTIVE || !CL_NpcWalk_CanSend() )
		return;
	CL_AddReliableCommand( ml.queue[0], qfalse );
	if ( !Q_stricmp( ml.queue[0], "rplock list" ) ) {
		ml.askTime = cls.realtime;
	} else {
		ml.sentTime = cls.realtime;
		Com_Printf( S_COLOR_GREEN "Map editor: " S_COLOR_WHITE "%s\n", ml.queue[0] );
	}
	ml.numQueue--;
	memmove( ml.queue[0], ml.queue[1], ml.numQueue * sizeof( ml.queue[0] ) );
}

// " 123: rank: 3 (Captain), name: Bridge, type: func_door, target name: bridgedoor"
static qboolean ME_ParseLock( const char *line ) {
	const char *open, *name, *type, *target, *p;
	meLock_t *l;
	int num, rank;

	if ( sscanf( line, "%d: rank: %d", &num, &rank ) != 2 || !( open = strchr( line, '(' ) ) || !( name = strstr( open, "), name: " ) ) )
		return qfalse;
	// the last ones: a lock's name could have them in it
	for ( type = NULL, p = name; ( p = strstr( p, ", type: " ) ) != NULL; p++ )
		type = p;
	for ( target = NULL, p = name; ( p = strstr( p, ", target name: " ) ) != NULL; p++ )
		target = p;
	if ( !type || !target || target < type || ml.numLocks >= ME_MAX_LOCKS )
		return qfalse;
	l = &ml.locks[ml.numLocks++];
	l->num = num;
	l->rank = rank;
	Q_strncpyz( l->rankName, open + 1, Com_Clampi( 1, sizeof( l->rankName ), (int)( name - open ) ) );
	Q_strncpyz( l->name, name + 9, Com_Clampi( 1, sizeof( l->name ), (int)( type - name - 9 ) + 1 ) );
	Q_strncpyz( l->type, type + 8, Com_Clampi( 1, sizeof( l->type ), (int)( target - type - 8 ) + 1 ) );
	Q_strncpyz( l->targetname, target + 15, sizeof( l->targetname ) );
	if ( !Q_stricmp( l->name, "<unnamed>" ) )
		l->name[0] = '\0';
	if ( !Q_stricmp( l->targetname, "<none>" ) )
		l->targetname[0] = '\0';
	return qtrue;
}

// every server print on its way to the cgame: rplock list read, and hidden when asked for here;
// qtrue keeps it out of the console
qboolean CL_MapEditor_ServerPrint( const char *text ) {
	char buf[MAX_STRING_CHARS], *line, *next;
	qboolean swallow = qtrue, any = qfalse;

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
		if ( !Q_stricmpn( line, ME_LOCK_HEADER, strlen( ME_LOCK_HEADER ) ) ) {
			// a list someone typed for is read too, and shown
			ml.ours = (qboolean)( ml.askTime != 0 );
			ml.askTime = 0;
			ml.lineTime = cls.realtime;
			ml.numLocks = 0;
			ml.haveList = qtrue;
		} else if ( ml.lineTime && cls.realtime - ml.lineTime < ME_LOCK_LINE_GAP && ( ME_ParseLock( line ) || !Q_stricmp( line, "None." ) ) ) {
			ml.lineTime = cls.realtime;
		} else {
			ml.lineTime = 0;
			swallow = qfalse;
			if ( cls.realtime - ml.sentTime < ME_LOCK_REPLY_MS ) {
				Q_strncpyz( ml.reply, line, sizeof( ml.reply ) );
				ml.replyTime = cls.realtime;
			}
			continue;
		}
		if ( !ml.ours )
			swallow = qfalse;
	}
	return (qboolean)( any && swallow );
}


// the Locks tab shows what takes a lock, doors and triggers as asked
static qboolean ME_LockShown( const meEnt_t *e ) {
	if ( !ME_Lockable( e ) )
		return qfalse;
	return ME_IsDoor( e ) ? ml.showDoors : ml.showTriggers;
}

static char *ME_LockNameBuffer( int *size ) {
	*size = sizeof( ml.name );
	return ml.name;
}

// opened from a storyteller step to click its trigger: 0 a map trigger, 1 a door or button, 2 a
// use trigger
void CL_MapEditor_PickForStory( int kind ) {
	CL_MapEditor_Open();
	if ( !me.open )
		return;
	me.storyPick = kind;
	me.tab = kind == 2 ? ME_TAB_TRIGGERS : ME_TAB_MAP;
	me.showLayouts = qfalse;
	me.showTriggers = me.showBrushes = qtrue;
	me.selKind = ME_SEL_NONE;
	me.sel = -1;
	me.grabbing = qfalse;
}

// opened from a storyteller step: Lock and Unlock go in the story, not to the server
void CL_MapEditor_OpenForStory( void ) {
	CL_MapEditor_Open();
	if ( !me.open )
		return;
	me.tab = ME_TAB_LOCKS;
	me.showLayouts = qfalse;
	ME_Message( S_COLOR_GREEN "For the story: Lock and Unlock go in it as steps, not to the server" );
}


static int ME_ItemTab( int kind ) {
	return kind == ME_ITEM_MODEL || kind == ME_ITEM_PROP ? ME_TAB_MODELS : kind == ME_ITEM_LIGHT ? ME_TAB_LIGHTS
		: kind == ME_ITEM_SPEAKER ? ME_TAB_SOUNDS : kind == ME_ITEM_FOG ? ME_TAB_FOG : ME_TAB_TRIGGERS;
}

static int ME_TabItemKind( int tab ) {
	return tab == ME_TAB_MODELS ? ME_ITEM_MODEL : tab == ME_TAB_LIGHTS ? ME_ITEM_LIGHT : tab == ME_TAB_SOUNDS ? ME_ITEM_SPEAKER
		: tab == ME_TAB_FOG ? ME_ITEM_FOG : ME_ITEM_TRIGGER;
}

/*
===============================================================================

The map's own entities

===============================================================================
*/

static void ME_ParseVector( const char *s, vec3_t v ) {
	VectorClear( v );
	sscanf( s, "%f %f %f", &v[0], &v[1], &v[2] );
}

static void ME_ParseEnts( void ) {
	const char *p = CM_EntityString(), *start;
	const int numModels = CM_NumInlineModels();
	char key[MAX_TOKEN_CHARS], *token;

	me.entString = p;
	me.numEnts = 0;
	me.kvEnt = -1;
	if ( !p )
		return;
	COM_BeginParseSession( "mapedit" );
	while ( me.numEnts < ME_MAX_ENTS ) {
		meEnt_t *e = &me.ents[me.numEnts];

		start = p;
		token = COM_ParseExt( &p, qtrue );
		if ( !token[0] || strcmp( token, "{" ) )
			break;
		memset( e, 0, sizeof( *e ) );
		while ( *start && *start != '{' )
			start++;
		e->ofs = start - me.entString;
		for ( ;; ) {
			token = COM_ParseExt( &p, qtrue );
			if ( !token[0] || !strcmp( token, "}" ) )
				break;
			Q_strncpyz( key, token, sizeof( key ) );
			token = COM_ParseExt( &p, qtrue );
			if ( !Q_stricmp( key, "classname" ) )
				Q_strncpyz( e->classname, token, sizeof( e->classname ) );
			else if ( !Q_stricmp( key, "targetname" ) )
				Q_strncpyz( e->targetname, token, sizeof( e->targetname ) );
			else if ( !Q_stricmp( key, "origin" ) ) {
				ME_ParseVector( token, e->origin );
				e->hasOrigin = qtrue;
			} else if ( !Q_stricmp( key, "model" ) && token[0] == '*' )
				e->bmodel = atoi( token + 1 );
		}
		if ( e->bmodel > 0 && e->bmodel < numModels ) {
			CM_ModelBounds( CM_InlineModel( e->bmodel ), e->mins, e->maxs );
			// with an origin the brushes are around it
			if ( e->hasOrigin ) {
				VectorAdd( e->mins, e->origin, e->mins );
				VectorAdd( e->maxs, e->origin, e->maxs );
			}
			e->kind = Q_stricmpn( e->classname, "trigger_", 8 ) ? ME_ENT_BRUSH : ME_ENT_TRIGGER;
		} else {
			e->bmodel = 0;
			e->kind = Q_stricmpn( e->classname, "trigger_", 8 ) ? ME_ENT_POINT : ME_ENT_TRIGGER;
		}
		me.numEnts++;
	}
}

// the chosen entity's keys, read again from the string
static void ME_ReadKeys( int index ) {
	const char *p;
	char *token;

	me.numKv = 0;
	me.kvScroll = 0;
	me.kvEnt = index;
	if ( index < 0 || index >= me.numEnts || me.entString != CM_EntityString() )
		return;
	p = me.entString + me.ents[index].ofs;
	COM_BeginParseSession( "mapedit" );
	token = COM_ParseExt( &p, qtrue );
	if ( strcmp( token, "{" ) )
		return;
	while ( me.numKv < ME_MAX_KV ) {
		token = COM_ParseExt( &p, qtrue );
		if ( !token[0] || !strcmp( token, "}" ) )
			break;
		Q_strncpyz( me.kv[me.numKv][0], token, MAX_QPATH );
		token = COM_ParseExt( &p, qtrue );
		Q_strncpyz( me.kv[me.numKv][1], token, MAX_QPATH );
		me.numKv++;
	}
}

static void ME_EntCenter( const meEnt_t *e, vec3_t out ) {
	if ( e->bmodel ) {
		VectorAdd( e->mins, e->maxs, out );
		VectorScale( out, 0.5f, out );
	} else {
		VectorCopy( e->origin, out );
	}
}

static qboolean ME_EntDrawn( const meEnt_t *e ) {
	if ( !e->bmodel && !e->hasOrigin )
		return qfalse;
	// the Locks tab: what takes a lock, nothing else
	if ( ME_LocksTab() )
		return ME_LockShown( e );
	if ( e->kind == ME_ENT_TRIGGER )
		return me.showTriggers;
	if ( e->kind == ME_ENT_BRUSH )
		return me.showBrushes;
	return me.showPoints;
}

static qboolean ME_EntMatches( const meEnt_t *e ) {
	char lower[64], name[96];

	if ( !me.filter[0] )
		return qtrue;
	Q_strncpyz( lower, me.filter, sizeof( lower ) );
	Q_strlwr( lower );
	Com_sprintf( name, sizeof( name ), "%s %s", e->classname, e->targetname );
	Q_strlwr( name );
	return (qboolean)( strstr( name, lower ) != NULL );
}

// a brush entity hidden here: the cgame's drawing of it is left out
static void ME_RebuildHidden( void ) {
	me.numHidden = 0;
	for ( int i = 0; i < me.numEnts && me.numHidden < ME_MAX_HIDDEN; i++ ) {
		if ( me.ents[i].hidden && me.ents[i].bmodel )
			me.hidden[me.numHidden++] = re->RegisterModel( va( "*%i", me.ents[i].bmodel ) );
	}
}

/*
===============================================================================

The map's props: the world's triangles, and which are joined

===============================================================================
*/

static void ME_FreeTris( void ) {
	void **mem[] = { (void **)&me.bspVerts, (void **)&me.tris, (void **)&me.surfFirstTri, (void **)&me.keys,
		(void **)&me.triStamp, (void **)&me.triTaken, (void **)&me.compTris, (void **)&me.brushPlanes, (void **)&me.brushFirst,
		(void **)&me.brushNum, (void **)&me.brushMins, (void **)&me.brushMaxs };

	for ( int i = 0; i < (int)ARRAY_LEN( mem ); i++ ) {
		if ( *mem[i] )
			Z_Free( *mem[i] );
		*mem[i] = NULL;
	}
	me.numBspVerts = me.numTris = me.numBspSurfs = me.numKeys = me.numCompTris = 0;
	me.numBrushes = 0;
	me.numCandSeeds = 0;
	me.candBrush = -1;
}

static void ME_KeyFor( const vec3_t p, meVertKey_t *k ) {
	// an eighth of a unit apart is the same corner
	k->x = ME_Round( p[0] * 8.0f );
	k->y = ME_Round( p[1] * 8.0f );
	k->z = ME_Round( p[2] * 8.0f );
}

static int ME_CompareKey( const meVertKey_t *a, const meVertKey_t *b ) {
	if ( a->x != b->x )
		return a->x < b->x ? -1 : 1;
	if ( a->y != b->y )
		return a->y < b->y ? -1 : 1;
	if ( a->z != b->z )
		return a->z < b->z ? -1 : 1;
	return 0;
}

static int QDECL ME_CompareKeys( const void *a, const void *b ) {
	return ME_CompareKey( (const meVertKey_t *)a, (const meVertKey_t *)b );
}

static int QDECL ME_CompareInts( const void *a, const void *b ) {
	return *(const int *)a - *(const int *)b;
}

static qboolean ME_LumpOk( const dheader_t *header, int lump, int len, int size ) {
	const int ofs = LittleLong( header->lumps[lump].fileofs ), filelen = LittleLong( header->lumps[lump].filelen );

	return (qboolean)( ofs >= 0 && filelen >= 0 && ofs + filelen <= len && filelen % size == 0 );
}

// the world's flat and model triangles; curves and the sky aren't props
static void ME_BuildTris( const byte *buf, int len ) {
	const dheader_t *header = (const dheader_t *)buf;
	const dsurface_t *surfs;
	const drawVert_t *verts;
	const int *indexes;
	const dshader_t *shaders;
	const dmodel_t *world;
	int numIndexes, numShaders, first, last, count = 0;

	ME_FreeTris();
	if ( !ME_LumpOk( header, LUMP_SURFACES, len, sizeof( dsurface_t ) ) || !ME_LumpOk( header, LUMP_DRAWVERTS, len, sizeof( drawVert_t ) )
		|| !ME_LumpOk( header, LUMP_DRAWINDEXES, len, sizeof( int ) ) || !ME_LumpOk( header, LUMP_SHADERS, len, sizeof( dshader_t ) )
		|| !ME_LumpOk( header, LUMP_MODELS, len, sizeof( dmodel_t ) ) || !LittleLong( header->lumps[LUMP_MODELS].filelen ) )
		return;
	surfs = (const dsurface_t *)( buf + LittleLong( header->lumps[LUMP_SURFACES].fileofs ) );
	me.numBspSurfs = LittleLong( header->lumps[LUMP_SURFACES].filelen ) / sizeof( dsurface_t );
	verts = (const drawVert_t *)( buf + LittleLong( header->lumps[LUMP_DRAWVERTS].fileofs ) );
	me.numBspVerts = LittleLong( header->lumps[LUMP_DRAWVERTS].filelen ) / sizeof( drawVert_t );
	indexes = (const int *)( buf + LittleLong( header->lumps[LUMP_DRAWINDEXES].fileofs ) );
	numIndexes = LittleLong( header->lumps[LUMP_DRAWINDEXES].filelen ) / sizeof( int );
	shaders = (const dshader_t *)( buf + LittleLong( header->lumps[LUMP_SHADERS].fileofs ) );
	numShaders = LittleLong( header->lumps[LUMP_SHADERS].filelen ) / sizeof( dshader_t );
	world = (const dmodel_t *)( buf + LittleLong( header->lumps[LUMP_MODELS].fileofs ) );
	first = Com_Clampi( 0, me.numBspSurfs, LittleLong( world->firstSurface ) );
	last = Com_Clampi( first, me.numBspSurfs, first + LittleLong( world->numSurfaces ) );

	// which surfaces, and how many triangles
	me.surfFirstTri = (int *)Z_Malloc( Q_max( 1, me.numBspSurfs ) * sizeof( int ), TAG_GENERAL, qfalse );
	for ( int i = 0; i < me.numBspSurfs; i++ ) {
		const dsurface_t *ds = &surfs[i];
		const int type = LittleLong( ds->surfaceType ), shader = LittleLong( ds->shaderNum );
		const int fi = LittleLong( ds->firstIndex ), ni = LittleLong( ds->numIndexes );
		const int fv = LittleLong( ds->firstVert ), nv = LittleLong( ds->numVerts );

		me.surfFirstTri[i] = -1;
		if ( i < first || i >= last || ( type != MST_PLANAR && type != MST_TRIANGLE_SOUP ) )
			continue;
		if ( shader < 0 || shader >= numShaders || ( LittleLong( shaders[shader].surfaceFlags ) & ( SURF_SKY | SURF_NODRAW ) ) )
			continue;
		if ( fi < 0 || ni < 3 || fi + ni > numIndexes || fv < 0 || nv <= 0 || fv + nv > me.numBspVerts )
			continue;
		me.surfFirstTri[i] = count;
		count += ni / 3;
	}
	if ( !count ) {
		ME_FreeTris();
		return;
	}

	me.bspVerts = (vec3_t *)Z_Malloc( me.numBspVerts * sizeof( vec3_t ), TAG_GENERAL, qfalse );
	for ( int i = 0; i < me.numBspVerts; i++ ) {
		me.bspVerts[i][0] = LittleFloat( verts[i].xyz[0] );
		me.bspVerts[i][1] = LittleFloat( verts[i].xyz[1] );
		me.bspVerts[i][2] = LittleFloat( verts[i].xyz[2] );
	}
	me.tris = (meTri_t *)Z_Malloc( count * sizeof( meTri_t ), TAG_GENERAL, qfalse );
	me.keys = (meVertKey_t *)Z_Malloc( count * 3 * sizeof( meVertKey_t ), TAG_GENERAL, qfalse );
	me.triStamp = (int *)Z_Malloc( count * sizeof( int ), TAG_GENERAL, qtrue );
	me.triTaken = (byte *)Z_Malloc( count, TAG_GENERAL, qtrue );
	me.compTris = (int *)Z_Malloc( count * sizeof( int ), TAG_GENERAL, qfalse );
	me.stamp = 0;

	for ( int i = 0; i < me.numBspSurfs; i++ ) {
		const dsurface_t *ds = &surfs[i];
		const int fi = LittleLong( ds->firstIndex ), ni = LittleLong( ds->numIndexes );
		const int fv = LittleLong( ds->firstVert ), nv = LittleLong( ds->numVerts );

		if ( me.surfFirstTri[i] < 0 )
			continue;
		for ( int t = 0; t < ni / 3; t++ ) {
			meTri_t *tri = &me.tris[me.numTris];

			tri->surf = i;
			tri->local = t;
			for ( int c = 0; c < 3; c++ ) {
				const int v = LittleLong( indexes[fi + t * 3 + c] );

				tri->v[c] = fv + ( v >= 0 && v < nv ? v : 0 );
				ME_KeyFor( me.bspVerts[tri->v[c]], &me.keys[me.numKeys] );
				me.keys[me.numKeys++].tri = me.numTris;
			}
			me.numTris++;
		}
	}
	qsort( me.keys, me.numKeys, sizeof( me.keys[0] ), ME_CompareKeys );

	// the world's solid brushes, their planes and, from the planes along the axes, their boxes
	if ( ME_LumpOk( header, LUMP_PLANES, len, sizeof( dplane_t ) ) && ME_LumpOk( header, LUMP_BRUSHES, len, sizeof( dbrush_t ) )
		&& ME_LumpOk( header, LUMP_BRUSHSIDES, len, sizeof( dbrushside_t ) ) ) {
		const dplane_t *planes = (const dplane_t *)( buf + LittleLong( header->lumps[LUMP_PLANES].fileofs ) );
		const int numPlanes = LittleLong( header->lumps[LUMP_PLANES].filelen ) / sizeof( dplane_t );
		const dbrush_t *brushes = (const dbrush_t *)( buf + LittleLong( header->lumps[LUMP_BRUSHES].fileofs ) );
		const int numBrushLump = LittleLong( header->lumps[LUMP_BRUSHES].filelen ) / sizeof( dbrush_t );
		const dbrushside_t *sides = (const dbrushside_t *)( buf + LittleLong( header->lumps[LUMP_BRUSHSIDES].fileofs ) );
		const int numSides = LittleLong( header->lumps[LUMP_BRUSHSIDES].filelen ) / sizeof( dbrushside_t );
		const int fb = Com_Clampi( 0, numBrushLump, LittleLong( world->firstBrush ) );
		const int nb = Com_Clampi( 0, numBrushLump - fb, LittleLong( world->numBrushes ) );
		int total = 0, out = 0;

		for ( int i = 0; i < nb; i++ )
			total += Q_max( 0, LittleLong( brushes[fb + i].numSides ) );
		if ( nb > 0 && total > 0 ) {
			me.numBrushes = nb;
			me.brushPlanes = (float ( * )[4])Z_Malloc( total * sizeof( me.brushPlanes[0] ), TAG_GENERAL, qfalse );
			me.brushFirst = (int *)Z_Malloc( nb * sizeof( int ), TAG_GENERAL, qfalse );
			me.brushNum = (int *)Z_Malloc( nb * sizeof( int ), TAG_GENERAL, qtrue );
			me.brushMins = (vec3_t *)Z_Malloc( nb * sizeof( vec3_t ), TAG_GENERAL, qfalse );
			me.brushMaxs = (vec3_t *)Z_Malloc( nb * sizeof( vec3_t ), TAG_GENERAL, qfalse );
			for ( int i = 0; i < nb; i++ ) {
				const dbrush_t *db = &brushes[fb + i];
				const int shader = LittleLong( db->shaderNum ), first = LittleLong( db->firstSide ), n = LittleLong( db->numSides );
				const int contents = shader >= 0 && shader < numShaders ? LittleLong( shaders[shader].contentFlags ) : 0;

				me.brushFirst[i] = out;
				VectorSet( me.brushMins[i], -MAX_WORLD_COORD, -MAX_WORLD_COORD, -MAX_WORLD_COORD );
				VectorSet( me.brushMaxs[i], MAX_WORLD_COORD, MAX_WORLD_COORD, MAX_WORLD_COORD );
				if ( !( contents & CONTENTS_SOLID ) || first < 0 || n <= 0 || first + n > numSides )
					continue;
				for ( int k = 0; k < n && out < total; k++ ) {
					const int pn = LittleLong( sides[first + k].planeNum );
					float *pl;

					if ( pn < 0 || pn >= numPlanes )
						continue;
					pl = me.brushPlanes[out++];
					pl[0] = LittleFloat( planes[pn].normal[0] );
					pl[1] = LittleFloat( planes[pn].normal[1] );
					pl[2] = LittleFloat( planes[pn].normal[2] );
					pl[3] = LittleFloat( planes[pn].dist );
					me.brushNum[i]++;
					for ( int a = 0; a < 3; a++ ) {
						if ( pl[a] == 1.0f )
							me.brushMaxs[i][a] = pl[3];
						else if ( pl[a] == -1.0f )
							me.brushMins[i][a] = -pl[3];
					}
				}
			}
		}
	}
}

// a point inside a brush, give or take eps
static qboolean ME_PointInBrush( const vec3_t p, int b, float eps ) {
	for ( int i = 0; i < 3; i++ ) {
		if ( p[i] < me.brushMins[b][i] - eps || p[i] > me.brushMaxs[b][i] + eps )
			return qfalse;
	}
	for ( int k = 0; k < me.brushNum[b]; k++ ) {
		const float *pl = me.brushPlanes[me.brushFirst[b] + k];

		if ( DotProduct( p, pl ) - pl[3] > eps )
			return qfalse;
	}
	return qtrue;
}

// a triangle lying on a brush: one of its faces, or part of one
static qboolean ME_TriOnBrush( int tri, int b ) {
	for ( int c = 0; c < 3; c++ ) {
		if ( !ME_PointInBrush( me.bspVerts[me.tris[tri].v[c]], b, 0.5f ) )
			return qfalse;
	}
	return qtrue;
}

// the first key at a corner
static int ME_FindKey( const meVertKey_t *k ) {
	int lo = 0, hi = me.numKeys;

	while ( lo < hi ) {
		const int mid = ( lo + hi ) / 2;

		if ( ME_CompareKey( &me.keys[mid], k ) < 0 )
			lo = mid + 1;
		else
			hi = mid;
	}
	return lo;
}

static int ME_SeedTri( int surf, int local ) {
	int tri;

	if ( !me.tris || surf < 0 || surf >= me.numBspSurfs || me.surfFirstTri[surf] < 0 )
		return -1;
	tri = me.surfFirstTri[surf] + local;
	if ( local < 0 || tri >= me.numTris || me.tris[tri].surf != surf )
		return -1;
	return tri;
}

static qboolean ME_TooBig( const vec3_t mins, const vec3_t maxs ) {
	for ( int i = 0; i < 3; i++ ) {
		if ( maxs[i] - mins[i] > ME_PROP_MAX_SIZE )
			return qtrue;
	}
	return qfalse;
}

// a triangle and all joined to it by corners, onto compTris; qfalse if it's more than a prop
static qboolean ME_Grow( int seed, qboolean skipTaken, vec3_t mins, vec3_t maxs ) {
	int head = me.numCompTris;
	const int start = head;

	if ( me.triStamp[seed] == me.stamp )
		return qtrue;
	me.triStamp[seed] = me.stamp;
	me.compTris[me.numCompTris++] = seed;
	while ( head < me.numCompTris ) {
		const meTri_t *t = &me.tris[me.compTris[head++]];

		for ( int c = 0; c < 3; c++ ) {
			meVertKey_t key;

			AddPointToBounds( me.bspVerts[t->v[c]], mins, maxs );
			ME_KeyFor( me.bspVerts[t->v[c]], &key );
			for ( int k = ME_FindKey( &key ); k < me.numKeys && !ME_CompareKey( &me.keys[k], &key ); k++ ) {
				const int n = me.keys[k].tri;

				if ( me.triStamp[n] == me.stamp || ( skipTaken && me.triTaken[n] ) )
					continue;
				me.triStamp[n] = me.stamp;
				me.compTris[me.numCompTris++] = n;
			}
		}
		if ( me.numCompTris - start > ME_PROP_MAX_TRIS || ME_TooBig( mins, maxs ) )
			return qfalse;
	}
	return qtrue;
}

// a brush's faces, the triangles lying on it, onto compTris; qfalse if it has none
static qboolean ME_AddBrush( int b, qboolean skipTaken, vec3_t mins, vec3_t maxs ) {
	int found = 0;

	if ( b < 0 || b >= me.numBrushes || !me.brushNum[b] )
		return qfalse;
	for ( int i = 0; i < me.numTris; i++ ) {
		if ( me.triStamp[i] == me.stamp || ( skipTaken && me.triTaken[i] ) || !ME_TriOnBrush( i, b ) )
			continue;
		me.triStamp[i] = me.stamp;
		me.compTris[me.numCompTris++] = i;
		for ( int c = 0; c < 3; c++ )
			AddPointToBounds( me.bspVerts[me.tris[i].v[c]], mins, maxs );
		found++;
	}
	return (qboolean)( found > 0 );
}

// the triangles of a prop found from its pieces, in order, onto compTris; a piece of -1:<n>
// is brush n's faces
static qboolean ME_CollectSeeds( const int ( *seeds )[2], int numSeeds, qboolean skipTaken, vec3_t mins, vec3_t maxs ) {
	me.stamp++;
	me.numCompTris = 0;
	ClearBounds( mins, maxs );
	if ( !me.tris )
		return qfalse;
	for ( int i = 0; i < numSeeds; i++ ) {
		if ( seeds[i][0] == -1 ) {
			ME_AddBrush( seeds[i][1], skipTaken, mins, maxs );
			continue;
		}
		const int tri = ME_SeedTri( seeds[i][0], seeds[i][1] );

		if ( tri >= 0 && !ME_Grow( tri, skipTaken, mins, maxs ) )
			return qfalse;
	}
	qsort( me.compTris, me.numCompTris, sizeof( me.compTris[0] ), ME_CompareInts );
	return (qboolean)( me.numCompTris > 0 );
}

// the nearest of the world's triangles along the ray, -1 none; what's lifted is gone
static int ME_TriHit( const vec3_t org, const vec3_t dir, float *dist ) {
	float best = ME_TRACE_DIST;
	int hit = -1;

	for ( int i = 0; i < me.numTris; i++ ) {
		const meTri_t *t = &me.tris[i];
		vec3_t e1, e2, pv, tv, qv;
		float det, u, v, d;

		if ( me.triTaken[i] )
			continue;
		VectorSubtract( me.bspVerts[t->v[1]], me.bspVerts[t->v[0]], e1 );
		VectorSubtract( me.bspVerts[t->v[2]], me.bspVerts[t->v[0]], e2 );
		CrossProduct( dir, e2, pv );
		det = DotProduct( e1, pv );
		if ( fabsf( det ) < 0.000001f )
			continue;
		VectorSubtract( org, me.bspVerts[t->v[0]], tv );
		u = DotProduct( tv, pv ) / det;
		if ( u < 0.0f || u > 1.0f )
			continue;
		CrossProduct( tv, e1, qv );
		v = DotProduct( dir, qv ) / det;
		if ( v < 0.0f || u + v > 1.0f )
			continue;
		d = DotProduct( e2, qv ) / det;
		if ( d > 0.1f && d < best ) {
			best = d;
			hit = i;
		}
	}
	*dist = best;
	return hit;
}

// what's lifted out is hidden in the world: all of them, each time one changes
static void ME_ApplyProps( void ) {
	int *surfs, *locals, n = 0;

	me.propsDirty = qfalse;
	me.propsShown = ME_Shown();
	if ( !me.tris )
		return;
	memset( me.triTaken, 0, me.numTris );
	if ( me.propsShown ) {
		for ( int i = 0; i < me.numItems; i++ ) {
			const meItem_t *it = &me.items[i];
			vec3_t mins, maxs;

			if ( it->kind != ME_ITEM_PROP || !ME_CollectSeeds( it->seeds, it->numSeeds, qfalse, mins, maxs ) )
				continue;
			for ( int j = 0; j < me.numCompTris; j++ )
				me.triTaken[me.compTris[j]] = 1;
		}
	}
	if ( !re->ext.HideWorldTriangles )
		return;
	for ( int i = 0; i < me.numTris; i++ ) {
		if ( me.triTaken[i] )
			n++;
	}
	surfs = (int *)Z_Malloc( ( n + 1 ) * sizeof( int ) * 2, TAG_GENERAL, qfalse );
	locals = surfs + n + 1;
	n = 0;
	for ( int i = 0; i < me.numTris; i++ ) {
		if ( me.triTaken[i] ) {
			surfs[n] = me.tris[i].surf;
			locals[n++] = me.tris[i].local;
		}
	}
	re->ext.HideWorldTriangles( surfs, locals, n );
	Z_Free( surfs );
}

// a lifted prop's model: the triangles it was, around where it stood
static void ME_BuildProp( meItem_t *it ) {
	vec3_t mins, maxs;
	int *surfs, *locals;

	it->registered = qtrue;
	it->h = 0;
	if ( !re->ext.WorldPieceModel || !ME_CollectSeeds( it->seeds, it->numSeeds, qfalse, mins, maxs ) )
		return;
	VectorSet( it->pivot, ( mins[0] + maxs[0] ) * 0.5f, ( mins[1] + maxs[1] ) * 0.5f, mins[2] );
	surfs = (int *)Z_Malloc( me.numCompTris * sizeof( int ) * 2, TAG_GENERAL, qfalse );
	locals = surfs + me.numCompTris;
	for ( int i = 0; i < me.numCompTris; i++ ) {
		surfs[i] = me.tris[me.compTris[i]].surf;
		locals[i] = me.tris[me.compTris[i]].local;
	}
	it->h = re->ext.WorldPieceModel( surfs, locals, me.numCompTris, it->pivot );
	Z_Free( surfs );
}

/*
===============================================================================

Light styles: which the map's surfaces use, and ours over the server's

===============================================================================
*/

// the bsp's surfaces say which styles light them
static void ME_ScanStyles( void ) {
	const dheader_t *header;
	const dsurface_t *surfs;
	const drawVert_t *verts;
	int len, numSurfs, numVerts;
	byte *buf;

	for ( int i = 0; i < MAX_LIGHT_STYLES; i++ ) {
		me.styles[i].surfaces = 0;
		ClearBounds( me.styles[i].mins, me.styles[i].maxs );
	}
	ME_FreeTris();
	len = FS_ReadFile( cl.mapname, (void **)&buf );
	if ( len < (int)sizeof( dheader_t ) || !buf ) {
		if ( buf )
			FS_FreeFile( buf );
		return;
	}
	header = (const dheader_t *)buf;
	if ( LittleLong( header->ident ) != BSP_IDENT || LittleLong( header->version ) != BSP_VERSION
		|| LittleLong( header->lumps[LUMP_SURFACES].fileofs ) + LittleLong( header->lumps[LUMP_SURFACES].filelen ) > len
		|| LittleLong( header->lumps[LUMP_DRAWVERTS].fileofs ) + LittleLong( header->lumps[LUMP_DRAWVERTS].filelen ) > len ) {
		FS_FreeFile( buf );
		return;
	}
	surfs = (const dsurface_t *)( buf + LittleLong( header->lumps[LUMP_SURFACES].fileofs ) );
	numSurfs = LittleLong( header->lumps[LUMP_SURFACES].filelen ) / sizeof( dsurface_t );
	verts = (const drawVert_t *)( buf + LittleLong( header->lumps[LUMP_DRAWVERTS].fileofs ) );
	numVerts = LittleLong( header->lumps[LUMP_DRAWVERTS].filelen ) / sizeof( drawVert_t );

	for ( int i = 0; i < numSurfs; i++ ) {
		const dsurface_t *s = &surfs[i];
		const int first = LittleLong( s->firstVert ), count = LittleLong( s->numVerts );
		qboolean seen[MAX_LIGHT_STYLES] = { qfalse };

		for ( int j = 0; j < MAXLIGHTMAPS * 2; j++ ) {
			const int style = j < MAXLIGHTMAPS ? s->lightmapStyles[j] : s->vertexStyles[j - MAXLIGHTMAPS];
			meStyle_t *st;

			// 0 is everything's normal light
			if ( style <= LS_NORMAL || style >= MAX_LIGHT_STYLES || seen[style] )
				continue;
			seen[style] = qtrue;
			st = &me.styles[style];
			st->surfaces++;
			if ( first < 0 || count < 0 || first + count > numVerts )
				continue;
			for ( int v = 0; v < count; v++ ) {
				vec3_t p;

				p[0] = LittleFloat( verts[first + v].xyz[0] );
				p[1] = LittleFloat( verts[first + v].xyz[1] );
				p[2] = LittleFloat( verts[first + v].xyz[2] );
				AddPointToBounds( p, st->mins, st->maxs );
			}
		}
	}
	// and its triangles, for the props
	ME_BuildTris( buf, len );
	FS_FreeFile( buf );
}

// the server's pattern for a style, its red one
static const char *ME_ServerPattern( int style ) {
	const int index = CS_LIGHT_STYLES + style * 3;
	const int ofs = cl.gameState.stringOffsets[index];

	return ofs ? cl.gameState.stringData + ofs : "";
}

static int ME_PatternValue( const char *pattern ) {
	const int len = strlen( pattern );
	int c;

	if ( !len )
		return 255;
	c = pattern[( Q_max( 0, cl.serverTime ) / 50 ) % len];
	return Com_Clampi( 0, 255, ( c - 'a' ) * 255 / ( 'z' - 'a' ) );
}

static int ME_PackColor( int r, int g, int b ) {
	byteAlias_t ba;

	ba.b[0] = (byte)Com_Clampi( 0, 255, r );
	ba.b[1] = (byte)Com_Clampi( 0, 255, g );
	ba.b[2] = (byte)Com_Clampi( 0, 255, b );
	ba.b[3] = 255;
	return ba.i;
}

// what the cgame sets a style to each frame, ours instead when it has one
int CL_MapEditor_LightStyle( int style, int color ) {
	const meStyle_t *s;
	int v;

	if ( style < 0 || style >= MAX_LIGHT_STYLES )
		return color;
	// found by blinking
	if ( me.open && style == me.flashStyle && cls.realtime < me.flashUntil ) {
		v = ( ( cls.realtime / 150 ) & 1 ) ? 255 : 0;
		return ME_PackColor( v, v, v );
	}
	s = &me.styles[style];
	if ( !ME_Shown() || !s->override )
		return color;
	v = ME_PatternValue( s->pattern );
	return ME_PackColor( ME_Round( v * s->color[0] ), ME_Round( v * s->color[1] ), ME_Round( v * s->color[2] ) );
}

/*
===============================================================================

Fog: the map's, ours over it

===============================================================================
*/

// what the map has, once it's up; ours kept
static void ME_ReadFogs( void ) {
	static vec3_t colors[ME_MAX_FOGS], mins[ME_MAX_FOGS], maxs[ME_MAX_FOGS];
	static float depths[ME_MAX_FOGS];

	me.fogsRead = qtrue;
	me.numFogs = 0;
	me.globalFog = -1;
	if ( !re->ext.GetFogs || cls.state != CA_ACTIVE )
		return;
	me.numFogs = re->ext.GetFogs( colors, depths, mins, maxs, &me.globalFog, ME_MAX_FOGS );
	for ( int i = 1; i < me.numFogs; i++ ) {
		meFog_t *f = &me.fogs[i];

		VectorCopy( colors[i], f->mapColor );
		f->mapDepth = depths[i];
		VectorCopy( mins[i], f->mins );
		VectorCopy( maxs[i], f->maxs );
		if ( !f->override ) {
			VectorCopy( f->mapColor, f->color );
			f->depth = f->mapDepth;
		}
	}
	me.fogsDirty = qtrue;
}

// our fog volumes as they are, to tell when one's changed
static unsigned ME_FogItemsSum( void ) {
	unsigned sum = 0;

	for ( int i = 0; i < me.numItems; i++ ) {
		const meItem_t *it = &me.items[i];
		const float v[10] = { it->org[0], it->org[1], it->org[2], it->size[0], it->size[1], it->size[2], it->color[0], it->color[1], it->color[2], it->fogDepth };

		if ( it->kind != ME_ITEM_FOG )
			continue;
		sum = sum * 31 + i;
		for ( int k = 0; k < 10; k++ )
			sum = sum * 31 + (unsigned)ME_Round( v[k] * 64.0f );
	}
	return sum;
}

// ours to the renderer, or the map's back: its fogs as we have them, and our volumes
static void ME_ApplyFogs( void ) {
	me.fogsDirty = qfalse;
	me.fogsShown = ME_Shown();
	me.fogItemsSum = ME_FogItemsSum();
	if ( re->ext.SetFog ) {
		for ( int i = 1; i < me.numFogs; i++ ) {
			const meFog_t *f = &me.fogs[i];

			re->ext.SetFog( i, me.fogsShown && f->override ? f->color : NULL, f->depth );
		}
	}
	if ( !re->ext.AddFog || !re->ext.ClearAddedFogs || cls.state != CA_ACTIVE )
		return;
	re->ext.ClearAddedFogs();
	if ( !me.fogsShown )
		return;
	for ( int i = 0; i < me.numItems; i++ ) {
		const meItem_t *it = &me.items[i];
		vec3_t mins, maxs;

		if ( it->kind != ME_ITEM_FOG )
			continue;
		VectorSubtract( it->org, it->size, mins );
		VectorAdd( it->org, it->size, maxs );
		re->ext.AddFog( mins, maxs, it->color, it->fogDepth );
	}
}

/*
===============================================================================

Our things

===============================================================================
*/

static void ME_RegisterItem( meItem_t *it ) {
	if ( it->kind == ME_ITEM_PROP && !it->registered ) {
		ME_BuildProp( it );
		return;
	}
	if ( it->kind != ME_ITEM_MODEL || it->registered )
		return;
	it->registered = qtrue;
	it->h = it->model[0] ? re->RegisterModel( it->model ) : 0;
}

static void ME_ForgetHandles( void ) {
	for ( int i = 0; i < me.numItems; i++ ) {
		me.items[i].registered = qfalse;
		me.items[i].h = 0;
		me.items[i].sfxRegistered = qfalse;
		me.items[i].sfx = 0;
	}
	me.numHidden = 0;
	// the renderer's world comes back whole
	me.propsDirty = qtrue;
	me.fogsRead = qfalse;
}

static void ME_Clear( void ) {
	me.numItems = 0;
	for ( int i = 0; i < MAX_LIGHT_STYLES; i++ ) {
		me.styles[i].override = qfalse;
		me.styles[i].pattern[0] = '\0';
		VectorSet( me.styles[i].color, 1, 1, 1 );
	}
	for ( int i = 0; i < me.numEnts; i++ )
		me.ents[i].hidden = qfalse;
	for ( int i = 0; i < ME_MAX_FOGS; i++ ) {
		me.fogs[i].override = qfalse;
		VectorCopy( me.fogs[i].mapColor, me.fogs[i].color );
		me.fogs[i].depth = me.fogs[i].mapDepth;
	}
	me.fogsDirty = qtrue;
	me.numHidden = 0;
	me.selKind = ME_SEL_NONE;
	me.sel = -1;
	me.grabbing = qfalse;
	me.dirty = qfalse;
	me.propsDirty = qtrue;
}

static const char *ME_ServerPattern( int style );

static void ME_Select( int kind, int index ) {
	me.grabbing = qfalse;
	me.dragAxis = -1;
	me.selKind = kind;
	me.sel = index;
	if ( kind == ME_SEL_ENT )
		ME_ReadKeys( index );
	// a style not ours yet shows the server's, ready to change
	if ( kind == ME_SEL_STYLE && index >= 0 && index < MAX_LIGHT_STYLES && !me.styles[index].override ) {
		Q_strncpyz( me.styles[index].pattern, ME_ServerPattern( index )[0] ? ME_ServerPattern( index ) : "z", sizeof( me.styles[index].pattern ) );
		VectorSet( me.styles[index].color, 1, 1, 1 );
	}
}

static meItem_t *ME_NewItem( int kind ) {
	meItem_t *it;

	if ( me.numItems >= ME_MAX_ITEMS ) {
		ME_Message( S_COLOR_YELLOW "That's as many as there can be" );
		return NULL;
	}
	it = &me.items[me.numItems];
	memset( it, 0, sizeof( *it ) );
	it->kind = kind;
	it->scale = 1.0f;
	it->style = -1;
	it->radius = 200.0f;
	VectorSet( it->color, 1.0f, 0.8f, 0.55f );
	VectorSet( it->size, 32, 32, 32 );
	it->once = qfalse;
	return it;
}

// where a new thing goes: on what the crosshair is on, or in front of the camera
static const char *ME_ShownName( const char *name );
static void ME_ItemAxis( const meItem_t *it, matrix3_t axis );

// a placed map ("#name") turns and scales around its bottom middle, the point it was put down by,
// not its origin, which can be far from it. That point in its own space; qfalse if it isn't a map
static qboolean ME_MapPivot( const meItem_t *it, vec3_t pivot ) {
	vec3_t mins, maxs;

	if ( it->kind != ME_ITEM_MODEL || it->model[0] != '#' || !it->h )
		return qfalse;
	re->ModelBounds( it->h, mins, maxs );
	pivot[0] = ( mins[0] + maxs[0] ) * 0.5f;
	pivot[1] = ( mins[1] + maxs[1] ) * 0.5f;
	pivot[2] = mins[2];
	return qtrue;
}

// a point in the item's own space, where it is in the world
static void ME_ItemPoint( const meItem_t *it, const vec3_t local, vec3_t out ) {
	matrix3_t axis;

	ME_ItemAxis( it, axis );
	VectorCopy( it->org, out );
	for ( int i = 0; i < 3; i++ )
		VectorMA( out, local[i], axis[i], out );
}

// the item moved so a point in its own space is at that place in the world
static void ME_PutPointAt( meItem_t *it, const vec3_t local, const vec3_t world ) {
	matrix3_t axis;

	ME_ItemAxis( it, axis );
	VectorCopy( world, it->org );
	for ( int i = 0; i < 3; i++ )
		VectorMA( it->org, -local[i], axis[i], it->org );
}

static void ME_PlaceAt( meItem_t *it, const vec3_t at, const vec3_t normal ) {
	switch ( it->kind ) {
	case ME_ITEM_MODEL:
	case ME_ITEM_PROP:
		VectorCopy( at, it->org );
		break;
	case ME_ITEM_LIGHT:
	case ME_ITEM_SPEAKER:
		VectorMA( at, 16.0f, normal, it->org );
		break;
	default:
		// a box sits on a floor, stands out from a wall
		if ( normal[2] > 0.7f ) {
			VectorCopy( at, it->org );
			it->org[2] += it->size[2];
		} else {
			VectorMA( at, Q_max( it->size[0], it->size[1] ), normal, it->org );
		}
		break;
	}
}

static void ME_AddItem( int kind ) {
	meItem_t *it;
	vec3_t forward, at, normal;
	trace_t tr;

	if ( kind == ME_ITEM_MODEL && !me.pickModel[0] ) {
		ME_Message( S_COLOR_YELLOW "Pick a model from the list first" );
		return;
	}
	if ( !( it = ME_NewItem( kind ) ) )
		return;
	if ( kind == ME_ITEM_FOG ) {
		VectorSet( it->size, 128, 128, 64 );
		VectorSet( it->color, 0.6f, 0.62f, 0.66f );
		it->fogDepth = 768.0f;
	}
	if ( kind == ME_ITEM_MODEL ) {
		Q_strncpyz( it->model, me.pickModel, sizeof( it->model ) );
		if ( it->model[0] != '#' )
			it->ang[YAW] = AngleNormalize360( ME_Round( ( me.camAng[YAW] + 180.0f ) / 15.0f ) * 15.0f );
	}
	// what the middle of the screen is on
	AngleVectors( me.camAng, forward, NULL, NULL );
	VectorMA( me.camOrg, ME_TRACE_DIST, forward, at );
	CM_BoxTrace( &tr, me.camOrg, at, vec3_origin, vec3_origin, 0, CONTENTS_SOLID | CONTENTS_TERRAIN, qfalse );
	if ( tr.fraction < 1.0f && !tr.allsolid ) {
		VectorCopy( tr.endpos, at );
		VectorCopy( tr.plane.normal, normal );
	} else {
		VectorMA( me.camOrg, 128.0f, forward, at );
		VectorSet( normal, 0, 0, 1 );
	}
	ME_PlaceAt( it, at, normal );
	if ( kind == ME_ITEM_MODEL && it->model[0] == '#' ) {
		vec3_t pivot;

		ME_RegisterItem( it );
		if ( !ME_MapPivot( it, pivot ) ) {
			ME_Message( va( S_COLOR_YELLOW "%s couldn't be loaded", ME_ShownName( it->model ) ) );
			return;
		}
		ME_PutPointAt( it, pivot, at );
	}
	me.numItems++;
	ME_Select( ME_SEL_ITEM, me.numItems - 1 );
	me.tab = ME_ItemTab( kind );
	ME_Changed();
}

static void ME_CopySelected( void ) {
	meItem_t *src = ME_SelectedItem(), *it;

	if ( !src || !( it = ME_NewItem( src->kind ) ) )
		return;
	*it = *src;
	it->org[0] += 16.0f;
	it->org[1] += 16.0f;
	it->inside = it->fired = qfalse;
	me.numItems++;
	ME_Select( ME_SEL_ITEM, me.numItems - 1 );
	ME_Changed();
}

static void ME_DeleteSelected( void ) {
	const meItem_t *it = ME_SelectedItem();

	if ( !it )
		return;
	if ( it->kind == ME_ITEM_PROP )
		me.propsDirty = qtrue;
	memmove( &me.items[me.sel], &me.items[me.sel + 1], ( me.numItems - me.sel - 1 ) * sizeof( me.items[0] ) );
	me.numItems--;
	me.selKind = ME_SEL_NONE;
	me.sel = -1;
	me.grabbing = qfalse;
	ME_Changed();
}

static void ME_Grab( void ) {
	meItem_t *it = ME_SelectedItem();

	if ( me.grabbing ) {
		me.grabbing = qfalse;
		return;
	}
	if ( !it ) {
		ME_Message( S_COLOR_YELLOW "Only what was placed here can be moved" );
		return;
	}
	VectorCopy( it->org, me.grabFrom );
	me.grabbing = qtrue;
}

static void ME_CancelGrab( void ) {
	meItem_t *it = ME_SelectedItem();

	if ( me.grabbing && it )
		VectorCopy( me.grabFrom, it->org );
	me.grabbing = qfalse;
}

// a click on the world found a prop of the map's
static qboolean ME_ChooseProp( int tri ) {
	if ( tri < 0 )
		return qfalse;
	me.candSeeds[0][0] = me.tris[tri].surf;
	me.candSeeds[0][1] = me.tris[tri].local;
	if ( !ME_CollectSeeds( me.candSeeds, 1, qtrue, me.candMins, me.candMaxs ) )
		return qfalse;
	me.numCandSeeds = 1;
	me.candTris = me.numCompTris;
	me.grabbing = qfalse;
	me.selKind = ME_SEL_PROP;
	me.sel = 0;
	me.tab = ME_TAB_MODELS;
	return qtrue;
}

// the brush a triangle hit lies on: the one just behind where the ray met it, the smallest if
// there are more; -1 none
static int ME_BrushAt( int tri, const vec3_t org, const vec3_t dir, float dist ) {
	vec3_t e1, e2, n, p;
	float best = 0.0f;
	int found = -1;

	if ( tri < 0 || tri >= me.numTris || !me.numBrushes )
		return -1;
	VectorSubtract( me.bspVerts[me.tris[tri].v[1]], me.bspVerts[me.tris[tri].v[0]], e1 );
	VectorSubtract( me.bspVerts[me.tris[tri].v[2]], me.bspVerts[me.tris[tri].v[0]], e2 );
	CrossProduct( e1, e2, n );
	if ( VectorNormalize( n ) <= 0.0f )
		return -1;
	// facing the camera, and a unit behind the face
	if ( DotProduct( n, dir ) > 0.0f )
		VectorNegate( n, n );
	VectorMA( org, dist, dir, p );
	VectorMA( p, -1.0f, n, p );
	for ( int b = 0; b < me.numBrushes; b++ ) {
		float size = 1.0f;

		if ( !me.brushNum[b] || !ME_PointInBrush( p, b, 0.0f ) || !ME_TriOnBrush( tri, b ) )
			continue;
		for ( int a = 0; a < 3; a++ )
			size *= Q_max( 1.0f, me.brushMaxs[b][a] - me.brushMins[b][a] );
		if ( found < 0 || size < best ) {
			best = size;
			found = b;
		}
	}
	return found;
}

static qboolean ME_CandIsBrush( void ) {
	return (qboolean)( me.numCandSeeds == 1 && me.candSeeds[0][0] == -1 );
}

static qboolean ME_ItemIsBrush( const meItem_t *it ) {
	return (qboolean)( it->kind == ME_ITEM_PROP && it->numSeeds >= 1 && it->seeds[0][0] == -1 );
}

// a click on the world found a brush of the map's: its faces, to lift like a prop
static qboolean ME_ChooseBrush( int b ) {
	const int seeds[1][2] = { { -1, b } };

	if ( b < 0 || !ME_CollectSeeds( seeds, 1, qtrue, me.candMins, me.candMaxs ) )
		return qfalse;
	me.candSeeds[0][0] = -1;
	me.candSeeds[0][1] = b;
	me.numCandSeeds = 1;
	me.candTris = me.numCompTris;
	me.grabbing = qfalse;
	me.selKind = ME_SEL_PROP;
	me.sel = 0;
	me.tab = ME_TAB_MODELS;
	return qtrue;
}

// pieces next to the prop that are inside what it covers: a lamp's glass, a table's legs
static void ME_TakeTouching( void ) {
	meItem_t *it = ME_SelectedItem();
	int ( *seeds )[2] = me.candSeeds, *numSeeds = &me.numCandSeeds, added = 0;
	vec3_t mins, maxs, size;

	if ( it && it->kind == ME_ITEM_PROP ) {
		seeds = it->seeds;
		numSeeds = &it->numSeeds;
	} else if ( me.selKind != ME_SEL_PROP ) {
		return;
	}
	if ( !ME_CollectSeeds( seeds, *numSeeds, qfalse, mins, maxs ) )
		return;
	VectorSubtract( maxs, mins, size );
	for ( int i = 0; i < 3; i++ ) {
		mins[i] -= 8.0f + size[i] * 0.25f;
		maxs[i] += 8.0f + size[i] * 0.25f;
	}
	for ( int i = 0; i < me.numTris && *numSeeds < ME_MAX_SEEDS; i++ ) {
		const meTri_t *t = &me.tris[i];
		const int start = me.numCompTris;
		vec3_t pmins, pmaxs;
		qboolean inside = qtrue;

		// not ours already, not another's
		if ( me.triStamp[i] == me.stamp || me.triTaken[i] )
			continue;
		for ( int c = 0; c < 3 && inside; c++ ) {
			for ( int a = 0; a < 3; a++ ) {
				if ( me.bspVerts[t->v[c]][a] < mins[a] || me.bspVerts[t->v[c]][a] > maxs[a] )
					inside = qfalse;
			}
		}
		if ( !inside )
			continue;
		ClearBounds( pmins, pmaxs );
		if ( ME_Grow( i, qtrue, pmins, pmaxs ) ) {
			for ( int a = 0; a < 3; a++ ) {
				if ( pmins[a] < mins[a] || pmaxs[a] > maxs[a] )
					inside = qfalse;
			}
		} else {
			inside = qfalse;
		}
		if ( !inside ) {
			// stays stamped, so it isn't tried again
			me.numCompTris = start;
			continue;
		}
		seeds[*numSeeds][0] = t->surf;
		seeds[*numSeeds][1] = t->local;
		( *numSeeds )++;
		added++;
	}
	if ( me.selKind == ME_SEL_PROP ) {
		ME_CollectSeeds( me.candSeeds, me.numCandSeeds, qtrue, me.candMins, me.candMaxs );
		me.candTris = me.numCompTris;
	}
	if ( it ) {
		it->registered = qfalse;
		me.propsDirty = qtrue;
		ME_Changed();
	}
	ME_Message( added ? va( "Took %i more piece%s", added, added == 1 ? "" : "s" ) : "Nothing else close enough to take" );
}

// the chosen prop out of the map, a model of its own: where it was, or gone
static void ME_LiftProp( qboolean removed, qboolean grab ) {
	meItem_t *it;

	if ( me.selKind != ME_SEL_PROP )
		return;
	if ( !re->ext.WorldPieceModel || !re->ext.HideWorldTriangles ) {
		ME_Message( S_COLOR_YELLOW "Lifting the map's props needs the vanilla renderer" );
		return;
	}
	if ( !( it = ME_NewItem( ME_ITEM_PROP ) ) )
		return;
	memcpy( it->seeds, me.candSeeds, sizeof( it->seeds ) );
	it->numSeeds = me.numCandSeeds;
	ME_BuildProp( it );
	if ( !it->h ) {
		ME_Message( S_COLOR_YELLOW "Couldn't make a model of it" );
		return;
	}
	VectorCopy( it->pivot, it->org );
	it->removed = removed;
	me.numItems++;
	ME_Select( ME_SEL_ITEM, me.numItems - 1 );
	me.tab = ME_TAB_MODELS;
	me.propsDirty = qtrue;
	ME_Changed();
	ME_Message( removed ? "Hidden; Shown brings it back" : "Lifted out: drag its arrows, or G, to move it" );
	if ( grab )
		ME_Grab();
}

/*
===============================================================================

Saving and loading

===============================================================================
*/

// maps/mp/ffa3.bsp: mp/ffa3
static const char *ME_MapBase( void ) {
	static char name[MAX_QPATH];

	Q_strncpyz( name, !Q_stricmpn( cl.mapname, "maps/", 5 ) ? cl.mapname + 5 : cl.mapname, sizeof( name ) );
	COM_StripExtension( name, name, sizeof( name ) );
	return name;
}

// mapedits/mp/ffa3.ent
static const char *ME_Path( void ) {
	return va( ME_DIR "/%s" ME_EXT, ME_MapBase() );
}

// mapedits/mp/ffa3/<name>.ent
static const char *ME_LayoutDir( void ) {
	return va( ME_DIR "/%s", ME_MapBase() );
}

// the edits a map used to load by itself, mapedits/<map>.ent, now a layout like the others
#define ME_OLD_LAYOUT		"old-saved-edits"

static const char *ME_LayoutPath( const char *name ) {
	if ( !Q_stricmp( name, ME_OLD_LAYOUT ) )
		return ME_Path();
	return va( ME_DIR "/%s/%s" ME_EXT, ME_MapBase(), name );
}

static qboolean ME_LayoutChar( int ch ) {
	return (qboolean)( isalnum( ch ) || ch == '_' || ch == '-' );
}

static qboolean ME_LayoutNameOk( const char *name ) {
	if ( !name[0] )
		return qfalse;
	for ( ; *name; name++ ) {
		if ( !ME_LayoutChar( *name ) )
			return qfalse;
	}
	return qtrue;
}

static void ME_Print( fileHandle_t f, const char *key, const char *value ) {
	FS_Printf( f, "\"%s\" \"%s\"\n", key, value );
}

static const char *ME_Vec( const vec3_t v ) {
	return va( "%g %g %g", ME_Round( v[0] * 10.0f ) / 10.0f, ME_Round( v[1] * 10.0f ) / 10.0f, ME_Round( v[2] * 10.0f ) / 10.0f );
}

// the edits written to the file; how many, -1 if it couldn't be
static int ME_SaveTo( const char *path ) {
	fileHandle_t f;
	int count = 0;

	if ( !me.map[0] )
		return -1;
	f = FS_FOpenFileWrite( path );
	if ( !f ) {
		ME_Message( S_COLOR_RED "Couldn't write the file" );
		return -1;
	}
	FS_Printf( f, "%s", ME_HEADER );
	for ( int i = 0; i < me.numItems; i++ ) {
		const meItem_t *it = &me.items[i];

		FS_Printf( f, "{\n" );
		if ( it->kind == ME_ITEM_MODEL ) {
			// a misc_model_static is drawn by the cgame from the entity string
			ME_Print( f, "classname", "misc_model_static" );
			ME_Print( f, "model", it->model );
			ME_Print( f, "origin", ME_Vec( it->org ) );
			ME_Print( f, "angles", ME_Vec( it->ang ) );
			if ( it->scale != 1.0f )
				ME_Print( f, "modelscale", va( "%g", it->scale ) );
		} else if ( it->kind == ME_ITEM_PROP ) {
			char seeds[MAX_STRING_CHARS] = "";

			for ( int j = 0; j < it->numSeeds; j++ )
				Q_strcat( seeds, sizeof( seeds ), va( "%s%i:%i", j ? " " : "", it->seeds[j][0], it->seeds[j][1] ) );
			// the map's own, found again by its pieces
			ME_Print( f, "classname", "rpm_prop" );
			ME_Print( f, "seeds", seeds );
			ME_Print( f, "from", ME_Vec( it->pivot ) );
			ME_Print( f, "origin", ME_Vec( it->org ) );
			ME_Print( f, "angles", ME_Vec( it->ang ) );
			if ( it->scale != 1.0f )
				ME_Print( f, "modelscale", va( "%g", it->scale ) );
			if ( it->removed )
				ME_Print( f, "removed", "1" );
		} else if ( it->kind == ME_ITEM_SPEAKER ) {
			// looped on or off, and global: a target_speaker's spawnflags
			ME_Print( f, "classname", "target_speaker" );
			ME_Print( f, "origin", ME_Vec( it->org ) );
			ME_Print( f, "noise", it->noise );
			ME_Print( f, "spawnflags", va( "%i", ( it->off ? 2 : 1 ) | ( it->global ? 4 : 0 ) ) );
		} else if ( it->kind == ME_ITEM_FOG ) {
			ME_Print( f, "classname", "rpm_fogvolume" );
			ME_Print( f, "origin", ME_Vec( it->org ) );
			ME_Print( f, "size", ME_Vec( it->size ) );
			ME_Print( f, "_color", va( "%g %g %g", it->color[0], it->color[1], it->color[2] ) );
			ME_Print( f, "depth", va( "%i", ME_Round( it->fogDepth ) ) );
		} else if ( it->kind == ME_ITEM_LIGHT ) {
			ME_Print( f, "classname", "rpm_light" );
			ME_Print( f, "origin", ME_Vec( it->org ) );
			ME_Print( f, "radius", va( "%i", ME_Round( it->radius ) ) );
			ME_Print( f, "_color", va( "%g %g %g", it->color[0], it->color[1], it->color[2] ) );
			if ( it->style >= 0 )
				ME_Print( f, "style", va( "%i", it->style ) );
		} else {
			ME_Print( f, "classname", "rpm_trigger" );
			ME_Print( f, "origin", ME_Vec( it->org ) );
			ME_Print( f, "size", ME_Vec( it->size ) );
			ME_Print( f, "command", it->command );
			if ( it->once )
				ME_Print( f, "once", "1" );
			if ( it->use )
				ME_Print( f, "use", "1" );
			if ( it->name[0] )
				ME_Print( f, "targetname", it->name );
			if ( it->display[0] )
				ME_Print( f, "displayname", it->display );
			if ( it->usetime > 0 )
				ME_Print( f, "usetime", va( "%i", it->usetime ) );
		}
		FS_Printf( f, "}\n" );
		count++;
	}
	for ( int i = 0; i < MAX_LIGHT_STYLES; i++ ) {
		const meStyle_t *s = &me.styles[i];

		if ( !s->override )
			continue;
		FS_Printf( f, "{\n" );
		ME_Print( f, "classname", "rpm_lightstyle" );
		ME_Print( f, "style", va( "%i", i ) );
		ME_Print( f, "value", s->pattern );
		ME_Print( f, "_color", va( "%g %g %g", s->color[0], s->color[1], s->color[2] ) );
		FS_Printf( f, "}\n" );
		count++;
	}
	for ( int i = 1; i < ME_MAX_FOGS; i++ ) {
		const meFog_t *fog = &me.fogs[i];

		if ( !fog->override )
			continue;
		FS_Printf( f, "{\n" );
		ME_Print( f, "classname", "rpm_fog" );
		ME_Print( f, "fog", va( "%i", i ) );
		ME_Print( f, "_color", va( "%g %g %g", fog->color[0], fog->color[1], fog->color[2] ) );
		ME_Print( f, "depth", va( "%i", ME_Round( fog->depth ) ) );
		FS_Printf( f, "}\n" );
		count++;
	}
	for ( int i = 0; i < me.numEnts; i++ ) {
		if ( !me.ents[i].hidden )
			continue;
		FS_Printf( f, "{\n" );
		ME_Print( f, "classname", "rpm_hide" );
		ME_Print( f, "model", va( "*%i", me.ents[i].bmodel ) );
		FS_Printf( f, "}\n" );
		count++;
	}
	FS_FCloseFile( f );
	return count;
}

static qboolean ME_LayoutNameOk( const char *name );
static void ME_SaveLayout( const char *name );

// edits are kept only as layouts, never by the map itself: the layout named, or the
// Layouts panel to name one
static qboolean ME_Save( void ) {
	if ( ME_LayoutNameOk( me.layoutName ) ) {
		ME_SaveLayout( me.layoutName );
		me.dirty = qfalse;
		return qtrue;
	}
	me.showLayouts = qtrue;
	me.field = ME_FIELD_LAYOUT;
	ME_Message( S_COLOR_YELLOW "Name a layout to keep the edits in" );
	return qfalse;
}

typedef struct {
	char	key[MAX_QPATH];
	char	value[ME_CMD_LEN];
} meKv_t;

static const char *ME_Value( const meKv_t *kv, int num, const char *key, const char *def ) {
	for ( int i = 0; i < num; i++ ) {
		if ( !Q_stricmp( kv[i].key, key ) )
			return kv[i].value;
	}
	return def;
}

static void ME_LoadEntity( const meKv_t *kv, int num ) {
	const char *classname = ME_Value( kv, num, "classname", "" );
	meItem_t *it;

	if ( !Q_stricmp( classname, "misc_model_static" ) ) {
		if ( !( it = ME_NewItem( ME_ITEM_MODEL ) ) )
			return;
		Q_strncpyz( it->model, ME_Value( kv, num, "model", "" ), sizeof( it->model ) );
		ME_ParseVector( ME_Value( kv, num, "origin", "0 0 0" ), it->org );
		ME_ParseVector( ME_Value( kv, num, "angles", "0 0 0" ), it->ang );
		it->scale = Com_Clamp( 0.05f, 50.0f, atof( ME_Value( kv, num, "modelscale", "1" ) ) );
		me.numItems++;
	} else if ( !Q_stricmp( classname, "rpm_prop" ) ) {
		const char *p = ME_Value( kv, num, "seeds", "" );

		if ( !( it = ME_NewItem( ME_ITEM_PROP ) ) )
			return;
		while ( *p && it->numSeeds < ME_MAX_SEEDS ) {
			int surf, local;

			while ( *p == ' ' )
				p++;
			if ( sscanf( p, "%i:%i", &surf, &local ) != 2 )
				break;
			it->seeds[it->numSeeds][0] = surf;
			it->seeds[it->numSeeds][1] = local;
			it->numSeeds++;
			while ( *p && *p != ' ' )
				p++;
		}
		ME_ParseVector( ME_Value( kv, num, "from", "0 0 0" ), it->pivot );
		ME_ParseVector( ME_Value( kv, num, "origin", "0 0 0" ), it->org );
		ME_ParseVector( ME_Value( kv, num, "angles", "0 0 0" ), it->ang );
		it->scale = Com_Clamp( 0.05f, 50.0f, atof( ME_Value( kv, num, "modelscale", "1" ) ) );
		it->removed = (qboolean)( atoi( ME_Value( kv, num, "removed", "0" ) ) != 0 );
		if ( it->numSeeds )
			me.numItems++;
	} else if ( !Q_stricmp( classname, "target_speaker" ) ) {
		const int flags = atoi( ME_Value( kv, num, "spawnflags", "1" ) );

		if ( !( it = ME_NewItem( ME_ITEM_SPEAKER ) ) )
			return;
		ME_ParseVector( ME_Value( kv, num, "origin", "0 0 0" ), it->org );
		Q_strncpyz( it->noise, ME_Value( kv, num, "noise", "" ), sizeof( it->noise ) );
		it->off = (qboolean)( ( flags & 2 ) && !( flags & 1 ) );
		it->global = (qboolean)( ( flags & 4 ) != 0 );
		me.numItems++;
	} else if ( !Q_stricmp( classname, "rpm_light" ) ) {
		if ( !( it = ME_NewItem( ME_ITEM_LIGHT ) ) )
			return;
		ME_ParseVector( ME_Value( kv, num, "origin", "0 0 0" ), it->org );
		ME_ParseVector( ME_Value( kv, num, "_color", "1 1 1" ), it->color );
		it->radius = Com_Clamp( 8.0f, 2000.0f, atof( ME_Value( kv, num, "radius", "200" ) ) );
		it->style = Com_Clampi( -1, MAX_LIGHT_STYLES - 1, atoi( ME_Value( kv, num, "style", "-1" ) ) );
		me.numItems++;
	} else if ( !Q_stricmp( classname, "rpm_trigger" ) ) {
		if ( !( it = ME_NewItem( ME_ITEM_TRIGGER ) ) )
			return;
		ME_ParseVector( ME_Value( kv, num, "origin", "0 0 0" ), it->org );
		ME_ParseVector( ME_Value( kv, num, "size", "32 32 32" ), it->size );
		for ( int i = 0; i < 3; i++ )
			it->size[i] = Com_Clamp( 1.0f, 4096.0f, it->size[i] );
		Q_strncpyz( it->command, ME_Value( kv, num, "command", "" ), sizeof( it->command ) );
		it->once = (qboolean)( atoi( ME_Value( kv, num, "once", "0" ) ) != 0 );
		it->use = (qboolean)( atoi( ME_Value( kv, num, "use", "0" ) ) != 0 );
		Q_strncpyz( it->name, ME_Value( kv, num, "targetname", "" ), sizeof( it->name ) );
		Q_strncpyz( it->display, ME_Value( kv, num, "displayname", "" ), sizeof( it->display ) );
		it->usetime = Com_Clampi( 0, ME_MAX_USETIME, atoi( ME_Value( kv, num, "usetime", "0" ) ) );
		me.numItems++;
	} else if ( !Q_stricmp( classname, "rpm_lightstyle" ) ) {
		const int style = atoi( ME_Value( kv, num, "style", "-1" ) );

		if ( style < 0 || style >= MAX_LIGHT_STYLES )
			return;
		me.styles[style].override = qtrue;
		Q_strncpyz( me.styles[style].pattern, ME_Value( kv, num, "value", "z" ), sizeof( me.styles[style].pattern ) );
		ME_ParseVector( ME_Value( kv, num, "_color", "1 1 1" ), me.styles[style].color );
	} else if ( !Q_stricmp( classname, "rpm_fogvolume" ) ) {
		if ( !( it = ME_NewItem( ME_ITEM_FOG ) ) )
			return;
		ME_ParseVector( ME_Value( kv, num, "origin", "0 0 0" ), it->org );
		ME_ParseVector( ME_Value( kv, num, "size", "128 128 64" ), it->size );
		for ( int i = 0; i < 3; i++ )
			it->size[i] = Com_Clamp( 1.0f, 16384.0f, it->size[i] );
		ME_ParseVector( ME_Value( kv, num, "_color", "0.6 0.6 0.6" ), it->color );
		it->fogDepth = Com_Clamp( 16.0f, ME_FOG_NONE, atof( ME_Value( kv, num, "depth", "768" ) ) );
		me.numItems++;
	} else if ( !Q_stricmp( classname, "rpm_fog" ) ) {
		const int fog = atoi( ME_Value( kv, num, "fog", "0" ) );

		if ( fog <= 0 || fog >= ME_MAX_FOGS )
			return;
		me.fogs[fog].override = qtrue;
		ME_ParseVector( ME_Value( kv, num, "_color", "0.5 0.5 0.5" ), me.fogs[fog].color );
		me.fogs[fog].depth = Com_Clamp( 16.0f, ME_FOG_NONE, atof( ME_Value( kv, num, "depth", "1024" ) ) );
		me.fogsDirty = qtrue;
	} else if ( !Q_stricmp( classname, "rpm_hide" ) ) {
		const char *model = ME_Value( kv, num, "model", "" );
		const int bmodel = model[0] == '*' ? atoi( model + 1 ) : 0;

		for ( int i = 0; i < me.numEnts; i++ ) {
			if ( bmodel && me.ents[i].bmodel == bmodel )
				me.ents[i].hidden = qtrue;
		}
	}
}

// the edits in the file in place of the ones shown; qfalse if there's no file
static qboolean ME_LoadFrom( const char *path ) {
	static meKv_t kv[32];
	const char *p;
	char *buf, *token;
	int num;

	ME_Clear();
	me.shared = qfalse;
	if ( FS_ReadFile( path, (void **)&buf ) <= 0 || !buf ) {
		ME_RebuildHidden();
		return qfalse;
	}
	p = buf;
	COM_BeginParseSession( "mapedit" );
	for ( ;; ) {
		token = COM_ParseExt( &p, qtrue );
		if ( !token[0] || strcmp( token, "{" ) )
			break;
		num = 0;
		for ( ;; ) {
			token = COM_ParseExt( &p, qtrue );
			if ( !token[0] || !strcmp( token, "}" ) )
				break;
			if ( num < (int)ARRAY_LEN( kv ) )
				Q_strncpyz( kv[num].key, token, sizeof( kv[num].key ) );
			token = COM_ParseExt( &p, qtrue );
			if ( num < (int)ARRAY_LEN( kv ) ) {
				Q_strncpyz( kv[num].value, token, sizeof( kv[num].value ) );
				num++;
			}
		}
		ME_LoadEntity( kv, num );
	}
	FS_FreeFile( buf );
	ME_RebuildHidden();
	return qtrue;
}

// no edits: a map always starts without any, until a layout is loaded or someone shares theirs
static void ME_Load( void ) {
	ME_Clear();
	me.shared = qfalse;
	ME_RebuildHidden();
}

/*
===============================================================================

Layouts

===============================================================================
*/

static void ME_ListLayouts( void ) {
	char list[16384], *p;
	int count;

	me.numLayouts = 0;
	if ( !me.map[0] )
		return;
	count = FS_GetFileList( ME_LayoutDir(), ME_EXT, list, sizeof( list ) );
	for ( p = list; count-- > 0 && me.numLayouts < ME_MAX_LAYOUTS; p += strlen( p ) + 1 ) {
		Q_strncpyz( me.layouts[me.numLayouts], p, sizeof( me.layouts[0] ) );
		COM_StripExtension( me.layouts[me.numLayouts], me.layouts[me.numLayouts], sizeof( me.layouts[0] ) );
		if ( me.layouts[me.numLayouts][0] )
			me.numLayouts++;
	}
	if ( FS_FileExists( ME_Path() ) && me.numLayouts < ME_MAX_LAYOUTS )
		Q_strncpyz( me.layouts[me.numLayouts++], ME_OLD_LAYOUT, sizeof( me.layouts[0] ) );
	if ( me.layoutSel >= me.numLayouts )
		me.layoutSel = -1;
}

static void ME_SaveLayout( const char *name ) {
	int count;

	if ( !ME_LayoutNameOk( name ) ) {
		ME_Message( S_COLOR_YELLOW "Name it with letters, numbers, - and _" );
		return;
	}
	count = ME_SaveTo( ME_LayoutPath( name ) );
	if ( count < 0 )
		return;
	ME_Message( va( S_COLOR_GREEN "Saved %i edits as %s", count, name ) );
	ME_ListLayouts();
}

static void ME_LoadLayout( const char *name ) {
	if ( !ME_LayoutNameOk( name ) || !ME_LoadFrom( ME_LayoutPath( name ) ) ) {
		ME_Message( va( S_COLOR_YELLOW "No layout %s", name ) );
		ME_Load();
		return;
	}
	me.dirty = qfalse;
	Q_strncpyz( me.layoutName, name, sizeof( me.layoutName ) );
	ME_Message( va( S_COLOR_GREEN "Loaded %s: Share shows it to everyone", name ) );
}

static void ME_DeleteLayout( const char *name ) {
	if ( !ME_LayoutNameOk( name ) )
		return;
	FS_HomeRemove( ME_LayoutPath( name ) );
	ME_Message( va( "Deleted %s", name ) );
	ME_ListLayouts();
}

/*
===============================================================================

Sharing: our edits to everyone on the map with this client, theirs to us

===============================================================================
*/

static void ME_CheckMap( void );

// what the server lets through: one command a second, unless it says otherwise
static int ME_CommandGap( void ) {
	const char *info = cl.gameState.stringData + cl.gameState.stringOffsets[CS_SERVERINFO];
	const char *flood = Info_ValueForKey( info, "sv_floodProtect" );
	int value;

	if ( !flood[0] )
		return 1100;
	value = atoi( flood );
	if ( value <= 0 )
		return 0;
	return ( value == 1 ? 1000 : value ) + 100;
}

// a word, a space put in after each / and every ME_SHARE_WORD characters
static void ME_AppendSplit( char *out, int size, const char *word ) {
	int len = strlen( out ), run = 0;

	for ( ; *word && len < size - 2; word++ ) {
		out[len++] = *word;
		run++;
		if ( word[1] && ( *word == '/' || run >= ME_SHARE_WORD ) ) {
			out[len++] = ' ';
			run = 0;
		}
	}
	out[len] = '\0';
}

static void ME_QueueShare( const char *hook ) {
	if ( me.numShare < ME_MAX_SHARE )
		Q_strncpyz( me.share[me.numShare++], hook, sizeof( me.share[0] ) );
}

// one edit for a hook; "" if it isn't shared
static const char *ME_ShareItem( const meItem_t *it ) {
	static char out[ME_SHARE_LEN];

	out[0] = '\0';
	if ( it->kind == ME_ITEM_MODEL ) {
		Q_strcat( out, sizeof( out ), va( "m %s", ME_Vec( it->org ) ) );
		Q_strcat( out, sizeof( out ), va( " %s %g ", ME_Vec( it->ang ), it->scale ) );
		ME_AppendSplit( out, sizeof( out ), it->model );
	} else if ( it->kind == ME_ITEM_PROP ) {
		Q_strcat( out, sizeof( out ), va( "p %s", ME_Vec( it->pivot ) ) );
		Q_strcat( out, sizeof( out ), va( " %s", ME_Vec( it->org ) ) );
		Q_strcat( out, sizeof( out ), va( " %s %g %i", ME_Vec( it->ang ), it->scale, it->removed ? 1 : 0 ) );
		for ( int j = 0; j < it->numSeeds; j++ )
			Q_strcat( out, sizeof( out ), va( " %i:%i", it->seeds[j][0], it->seeds[j][1] ) );
	} else if ( it->kind == ME_ITEM_LIGHT ) {
		Q_strcat( out, sizeof( out ), va( "l %s %i %g %g %g %i", ME_Vec( it->org ), ME_Round( it->radius ),
			it->color[0], it->color[1], it->color[2], it->style ) );
	} else if ( it->kind == ME_ITEM_TRIGGER && it->use ) {
		char words[sizeof( it->name ) + sizeof( it->display ) + 16];

		// the name and what it's shown as, ~ between, spaces as `: the edit's spaces are taken out.
		// Held, the ms after another ~
		if ( it->usetime > 0 )
			Com_sprintf( words, sizeof( words ), "%s~%s~%i", it->name, it->display, it->usetime );
		else
			Com_sprintf( words, sizeof( words ), "%s~%s", it->name, it->display );
		for ( char *c = words; *c; c++ ) {
			if ( *c == ' ' )
				*c = '`';
		}
		Q_strcat( out, sizeof( out ), va( "u %s", ME_Vec( it->org ) ) );
		Q_strcat( out, sizeof( out ), va( " %s ", ME_Vec( it->size ) ) );
		ME_AppendSplit( out, sizeof( out ), words );
	} else if ( it->kind == ME_ITEM_FOG ) {
		Q_strcat( out, sizeof( out ), va( "v %s", ME_Vec( it->org ) ) );
		Q_strcat( out, sizeof( out ), va( " %s %g %g %g %i", ME_Vec( it->size ), it->color[0], it->color[1], it->color[2], ME_Round( it->fogDepth ) ) );
	} else if ( it->kind == ME_ITEM_SPEAKER && it->noise[0] ) {
		Q_strcat( out, sizeof( out ), va( "a %s %i ", ME_Vec( it->org ), ( it->off ? 2 : 1 ) | ( it->global ? 4 : 0 ) ) );
		ME_AppendSplit( out, sizeof( out ), it->noise );
	}
	return out;
}

// everything shown, to everyone: begin, the edits a hook at a time, end
static void ME_Share( void ) {
	char part[ME_SHARE_LEN + 1] = "";
	const char *edits[ME_MAX_ITEMS + MAX_LIGHT_STYLES + ME_MAX_HIDDEN + ME_MAX_FOGS];
	static char styleText[MAX_LIGHT_STYLES][MAX_QPATH * 2 + 32];
	static char fogText[ME_MAX_FOGS][64];
	static char itemText[ME_MAX_ITEMS][ME_SHARE_LEN];
	static char hideText[ME_MAX_HIDDEN][16];
	int numHide = 0;
	int numEdits = 0, numParts = 0, first;

	if ( cls.state != CA_ACTIVE || !me.map[0] )
		return;
	if ( !CL_RpHooks_Experimental() ) {
		ME_Message( S_COLOR_YELLOW RP_EXPERIMENTAL_OFF ", in Layouts" );
		return;
	}
	if ( me.shareSent < me.numShare ) {
		ME_Message( S_COLOR_YELLOW "Still sending the last share" );
		return;
	}

	for ( int i = 0; i < me.numItems; i++ ) {
		const char *edit = ME_ShareItem( &me.items[i] );

		if ( edit[0] ) {
			Q_strncpyz( itemText[i], edit, sizeof( itemText[i] ) );
			edits[numEdits++] = itemText[i];
		}
	}
	for ( int i = 0; i < MAX_LIGHT_STYLES; i++ ) {
		const meStyle_t *s = &me.styles[i];

		if ( !s->override )
			continue;
		Com_sprintf( styleText[i], sizeof( styleText[i] ), "s %i %g %g %g ", i, s->color[0], s->color[1], s->color[2] );
		ME_AppendSplit( styleText[i], sizeof( styleText[i] ), s->pattern[0] ? s->pattern : "z" );
		edits[numEdits++] = styleText[i];
	}
	for ( int i = 1; i < ME_MAX_FOGS; i++ ) {
		const meFog_t *fog = &me.fogs[i];

		if ( !fog->override )
			continue;
		Com_sprintf( fogText[i], sizeof( fogText[i] ), "f %i %g %g %g %i", i, fog->color[0], fog->color[1], fog->color[2], ME_Round( fog->depth ) );
		edits[numEdits++] = fogText[i];
	}
	for ( int i = 0; i < me.numEnts && numHide < ME_MAX_HIDDEN; i++ ) {
		if ( me.ents[i].hidden && me.ents[i].bmodel ) {
			Com_sprintf( hideText[numHide], sizeof( hideText[0] ), "h %i", me.ents[i].bmodel );
			edits[numEdits++] = hideText[numHide++];
		}
	}

	me.numShare = me.shareSent = 0;
	me.shareId = 1 + ( ( cls.realtime ^ ( rand() << 8 ) ) & 0xffff );
	first = me.numShare;
	ME_QueueShare( "" );		// the begin, once the parts are counted
	for ( int i = 0; i < numEdits; i++ ) {
		if ( part[0] && strlen( part ) + strlen( edits[i] ) + 3 > ME_SHARE_LEN ) {
			ME_QueueShare( va( "cmd:" ME_SHARE_HOOK "/add/%i/%i: %s", me.shareId, ++numParts, part ) );
			part[0] = '\0';
		}
		Q_strcat( part, sizeof( part ), va( "%s%s", part[0] ? " | " : "", edits[i] ) );
	}
	if ( part[0] )
		ME_QueueShare( va( "cmd:" ME_SHARE_HOOK "/add/%i/%i: %s", me.shareId, ++numParts, part ) );
	ME_QueueShare( va( "cmd:" ME_SHARE_HOOK "/end/%i/%i", me.shareId, numParts ) );
	Com_sprintf( me.share[first], sizeof( me.share[0] ), "cmd:" ME_SHARE_HOOK "/begin/%i/%i: %s", me.shareId, numParts, me.map );
	if ( me.numShare == ME_MAX_SHARE ) {
		me.numShare = 0;
		ME_Message( S_COLOR_RED "Too many edits to share at once" );
		return;
	}
	me.nextShare = 0;
	ME_Message( va( S_COLOR_GREEN "Sharing %i edits in %i hooks", numEdits, me.numShare ) );
}

// everyone back to their own edits
static void ME_Unshare( void ) {
	if ( cls.state != CA_ACTIVE || !me.map[0] )
		return;
	if ( !CL_RpHooks_Experimental() ) {
		ME_Message( S_COLOR_YELLOW RP_EXPERIMENTAL_OFF ", in Layouts" );
		return;
	}
	// what's still going isn't wanted now
	me.numShare = me.shareSent = 0;
	ME_QueueShare( va( "cmd:" ME_SHARE_HOOK "/clear: %s", me.map ) );
	me.nextShare = 0;
	ME_Message( "Taking the shared edits away from everyone" );
}

static void ME_RunShare( void ) {
	if ( me.shareSent >= me.numShare || cls.realtime < me.nextShare )
		return;
	CL_AddReliableCommand( va( "rpnotify -1 1 %s", me.share[me.shareSent++] ), qfalse );
	me.nextShare = cls.realtime + ME_CommandGap();
	if ( me.shareSent == me.numShare )
		ME_Message( S_COLOR_GREEN "Shared with everyone on the map with this client" );
}

// the edit's words put back together: spaces were put in to keep them short
static void ME_Unsplit( const char *in, char *out, int size ) {
	int len = 0;

	for ( ; *in && len < size - 1; in++ ) {
		if ( *in != ' ' )
			out[len++] = *in;
	}
	out[len] = '\0';
}

// one shared edit, as an entity from the file
static void ME_ApplyShared( const char *edit ) {
	meKv_t kv[8];
	float f[12];
	int n = 0, used = 0;
	char rest[MAX_QPATH * 2];

	while ( *edit == ' ' )
		edit++;
	memset( kv, 0, sizeof( kv ) );
#define ME_KV( k, v ) ( Q_strncpyz( kv[n].key, k, sizeof( kv[n].key ) ), Q_strncpyz( kv[n].value, v, sizeof( kv[n].value ) ), n++ )
	switch ( *edit ) {
	case 'm':
		if ( sscanf( edit + 1, "%f %f %f %f %f %f %f %n", &f[0], &f[1], &f[2], &f[3], &f[4], &f[5], &f[6], &used ) < 7 || !used )
			return;
		ME_Unsplit( edit + 1 + used, rest, sizeof( rest ) );
		ME_KV( "classname", "misc_model_static" );
		ME_KV( "model", rest );
		ME_KV( "origin", va( "%g %g %g", f[0], f[1], f[2] ) );
		ME_KV( "angles", va( "%g %g %g", f[3], f[4], f[5] ) );
		ME_KV( "modelscale", va( "%g", f[6] ) );
		break;
	case 'p':
		if ( sscanf( edit + 1, "%f %f %f %f %f %f %f %f %f %f %f %n", &f[0], &f[1], &f[2], &f[3], &f[4], &f[5], &f[6], &f[7], &f[8],
			&f[9], &f[10], &used ) < 11 || !used )
			return;
		ME_KV( "classname", "rpm_prop" );
		ME_KV( "from", va( "%g %g %g", f[0], f[1], f[2] ) );
		ME_KV( "origin", va( "%g %g %g", f[3], f[4], f[5] ) );
		ME_KV( "angles", va( "%g %g %g", f[6], f[7], f[8] ) );
		ME_KV( "modelscale", va( "%g", f[9] ) );
		ME_KV( "removed", f[10] != 0.0f ? "1" : "0" );
		ME_KV( "seeds", edit + 1 + used );
		break;
	case 'l':
		if ( sscanf( edit + 1, "%f %f %f %f %f %f %f %f", &f[0], &f[1], &f[2], &f[3], &f[4], &f[5], &f[6], &f[7] ) < 8 )
			return;
		ME_KV( "classname", "rpm_light" );
		ME_KV( "origin", va( "%g %g %g", f[0], f[1], f[2] ) );
		ME_KV( "radius", va( "%g", f[3] ) );
		ME_KV( "_color", va( "%g %g %g", f[4], f[5], f[6] ) );
		ME_KV( "style", va( "%i", (int)f[7] ) );
		break;
	case 'a':
		if ( sscanf( edit + 1, "%f %f %f %f %n", &f[0], &f[1], &f[2], &f[3], &used ) < 4 || !used )
			return;
		ME_Unsplit( edit + 1 + used, rest, sizeof( rest ) );
		ME_KV( "classname", "target_speaker" );
		ME_KV( "origin", va( "%g %g %g", f[0], f[1], f[2] ) );
		ME_KV( "spawnflags", va( "%i", (int)f[3] ) );
		ME_KV( "noise", rest );
		break;
	case 's':
		if ( sscanf( edit + 1, "%f %f %f %f %n", &f[0], &f[1], &f[2], &f[3], &used ) < 4 || !used )
			return;
		ME_Unsplit( edit + 1 + used, rest, sizeof( rest ) );
		ME_KV( "classname", "rpm_lightstyle" );
		ME_KV( "style", va( "%i", (int)f[0] ) );
		ME_KV( "_color", va( "%g %g %g", f[1], f[2], f[3] ) );
		ME_KV( "value", rest );
		break;
	case 'u':
		if ( sscanf( edit + 1, "%f %f %f %f %f %f %n", &f[0], &f[1], &f[2], &f[3], &f[4], &f[5], &used ) < 6 || !used )
			return;
		ME_Unsplit( edit + 1 + used, rest, sizeof( rest ) );
		for ( char *c = rest; *c; c++ ) {
			if ( *c == '`' )
				*c = ' ';
		}
		{
			char *bar = strchr( rest, '~' ), *held = NULL;

			if ( bar ) {
				*bar++ = '\0';
				held = strchr( bar, '~' );
				if ( held )
					*held++ = '\0';
			}
			ME_KV( "classname", "rpm_trigger" );
			ME_KV( "origin", va( "%g %g %g", f[0], f[1], f[2] ) );
			ME_KV( "size", va( "%g %g %g", f[3], f[4], f[5] ) );
			ME_KV( "use", "1" );
			ME_KV( "targetname", rest );
			ME_KV( "displayname", bar ? bar : "" );
			if ( held )
				ME_KV( "usetime", held );
		}
		break;
	case 'v':
		if ( sscanf( edit + 1, "%f %f %f %f %f %f %f %f %f %f", &f[0], &f[1], &f[2], &f[3], &f[4], &f[5], &f[6], &f[7], &f[8], &f[9] ) < 10 )
			return;
		ME_KV( "classname", "rpm_fogvolume" );
		ME_KV( "origin", va( "%g %g %g", f[0], f[1], f[2] ) );
		ME_KV( "size", va( "%g %g %g", f[3], f[4], f[5] ) );
		ME_KV( "_color", va( "%g %g %g", f[6], f[7], f[8] ) );
		ME_KV( "depth", va( "%g", f[9] ) );
		break;
	case 'f':
		if ( sscanf( edit + 1, "%f %f %f %f %f", &f[0], &f[1], &f[2], &f[3], &f[4] ) < 5 )
			return;
		ME_KV( "classname", "rpm_fog" );
		ME_KV( "fog", va( "%i", (int)f[0] ) );
		ME_KV( "_color", va( "%g %g %g", f[1], f[2], f[3] ) );
		ME_KV( "depth", va( "%g", f[4] ) );
		break;
	case 'h':
		if ( sscanf( edit + 1, "%f", &f[0] ) < 1 )
			return;
		ME_KV( "classname", "rpm_hide" );
		ME_KV( "model", va( "*%i", (int)f[0] ) );
		break;
	default:
		return;
	}
#undef ME_KV
	ME_LoadEntity( kv, n );
}

// the batch in, in place of what's shown
static void ME_ApplyBatch( void ) {
	char *p = me.rxBuf, *end;
	int before;

	// ours aren't lost under theirs
	if ( me.dirty && !me.shared && me.numItems ) {
		const char *name = ME_LayoutNameOk( me.layoutName ) ? me.layoutName : "before-shared";

		ME_SaveLayout( name );
		Com_Printf( "Map editor: your edits were kept as the layout %s before the shared ones came in\n", name );
	}
	ME_Clear();
	for ( ; *p; p = end ) {
		end = strchr( p, '|' );
		if ( end )
			*end++ = '\0';
		else
			end = p + strlen( p );
		ME_ApplyShared( p );
	}
	ME_RebuildHidden();
	me.shared = qtrue;
	me.dirty = qfalse;
	before = me.rxParts - me.rxGot;
	ME_Message( va( S_COLOR_GREEN "Map edits shared with you: %i%s", me.numItems, before > 0 ? va( S_COLOR_YELLOW ", %i parts missing", before ) : "" ) );
	Com_Printf( "%s\n", me.message );
	me.rxId = 0;
}

// cmd:map/<verb>/<id>/<n>: <text>
void CL_MapEditor_Hook( char **argv, int argc, const char *message ) {
	const char *verb = argc >= 1 ? argv[0] : "";
	const int id = argc >= 2 ? atoi( argv[1] ) : 0;

	if ( !rpc_mapShare->integer || cls.state != CA_ACTIVE )
		return;
	ME_CheckMap();
	if ( !me.map[0] )
		return;

	if ( !Q_stricmp( verb, "clear" ) ) {
		if ( me.shared && !Q_stricmp( message, me.map ) ) {
			ME_Load();
			ME_Message( "The shared map edits were taken away" );
			Com_Printf( "%s\n", me.message );
		}
		return;
	}
	// ours, back from the server
	if ( !id || id == me.shareId )
		return;
	if ( !Q_stricmp( verb, "begin" ) ) {
		me.rxId = Q_stricmp( message, me.map ) ? 0 : id;
		me.rxParts = argc >= 3 ? atoi( argv[2] ) : 0;
		me.rxGot = 0;
		me.rxLen = 0;
		me.rxBuf[0] = '\0';
	} else if ( !Q_stricmp( verb, "add" ) && id == me.rxId ) {
		const int len = strlen( message );

		if ( me.rxLen + len + 2 < (int)sizeof( me.rxBuf ) ) {
			memcpy( me.rxBuf + me.rxLen, message, len );
			me.rxLen += len;
			me.rxBuf[me.rxLen++] = '|';
			me.rxBuf[me.rxLen] = '\0';
		}
		me.rxGot++;
	} else if ( !Q_stricmp( verb, "end" ) && id == me.rxId ) {
		ME_ApplyBatch();
	}
}

// a new map: its entities, its styles, and the edits made for it
static void ME_CheckMap( void ) {
	if ( !cl.mapname[0] )
		return;
	if ( !Q_stricmp( me.map, cl.mapname ) ) {
		// the same map back, after a vid_restart or a map_restart
		if ( me.reparse ) {
			qboolean hidden[ME_MAX_ENTS];
			const int num = me.numEnts;

			for ( int i = 0; i < num; i++ )
				hidden[i] = me.ents[i].hidden;
			ME_ParseEnts();
			for ( int i = 0; i < num && i < me.numEnts; i++ )
				me.ents[i].hidden = hidden[i];
			ME_RebuildHidden();
			me.reparse = qfalse;
		}
		return;
	}
	if ( me.dirty && me.map[0] )
		Com_Printf( S_COLOR_YELLOW "Map editor: the edits for %s weren't saved\n", me.map );
	me.reparse = qfalse;
	Q_strncpyz( me.map, cl.mapname, sizeof( me.map ) );
	me.numShare = me.shareSent = 0;
	me.rxId = 0;
	me.layoutSel = -1;
	ME_ParseEnts();
	ME_ScanStyles();
	ME_Load();
	ME_ListLayouts();
	me.entScroll = me.styleScroll = 0;
	memset( me.itemScroll, 0, sizeof( me.itemScroll ) );
}

/*
===============================================================================

Models to choose from

===============================================================================
*/

static void ME_AddName( const char *name, void *ctx ) {
	const int len = strlen( name ) + 1;
	char *stored;

	if ( me.numNames >= ME_MAX_NAMES || me.poolUsed + len > ME_NAME_POOL || !Q_stricmpn( name, "models/players/", 15 ) )
		return;
	stored = meNamePool + me.poolUsed;
	me.poolUsed += len;
	Q_strncpyz( stored, name, len );
	for ( char *c = stored; *c; c++ )
		*c = ( *c == '\\' ) ? '/' : tolower( *c );
	me.names[me.numNames++] = stored;
}

// another map, by its name in maps/ without .bsp: "#mp/ffa3", as the renderer takes it; not
// the one loaded
static void ME_AddMapName( const char *name, void *ctx ) {
	char map[MAX_QPATH], current[MAX_QPATH];

	if ( Q_stricmpn( name, "maps/", 5 ) )
		return;
	COM_StripExtension( name + 5, map, sizeof( map ) );
	COM_StripExtension( cl.mapname[0] ? cl.mapname + 5 : "", current, sizeof( current ) );
	for ( char *c = map; *c; c++ ) {
		if ( *c == '\\' )
			*c = '/';
	}
	if ( !map[0] || !Q_stricmp( map, current ) )
		return;
	ME_AddName( va( "#%s", map ), ctx );
}

// a name in the list as it's shown: a model's without "models/", a map's as one
static const char *ME_ShownName( const char *name ) {
	if ( name[0] == '#' )
		return va( "map: %s", name + 1 );
	return !Q_stricmpn( name, "models/", 7 ) ? name + 7 : name;
}

static int QDECL ME_CompareNames( const void *a, const void *b ) {
	return strcmp( *(const char **)a, *(const char **)b );
}

static void ME_IndexModels( void ) {
	int kept = 0;

	me.numNames = me.poolUsed = 0;
	FS_ListFilesRecursive( "models", ".md3", ME_AddName, NULL );
	FS_ListFilesRecursive( "maps", ".bsp", ME_AddMapName, NULL );
	qsort( me.names, me.numNames, sizeof( me.names[0] ), ME_CompareNames );
	// a model in a pk3 and loose shows once
	for ( int i = 0; i < me.numNames; i++ ) {
		if ( kept && !strcmp( me.names[kept - 1], me.names[i] ) )
			continue;
		me.names[kept++] = me.names[i];
	}
	me.numNames = kept;
	me.indexed = qtrue;
}

/*
===============================================================================

Opening and closing

===============================================================================
*/

static void ME_ReleaseKeys( void ) {
	memset( me.held, 0, sizeof( me.held ) );
	me.flying = qfalse;
}

void CL_MapEditor_Open( void ) {
	if ( cls.state != CA_ACTIVE || !cls.cgameStarted ) {
		Com_Printf( "Map editor: join a server first\n" );
		return;
	}
	// for RPMod's admins: whoever else can't run what it makes
	if ( !CL_RpAdmin_Allow( "Map editor", "mapedit" ) )
		return;
	CL_ModelManager_Close();
	CL_NpcManager_Close();
	CL_ShaderManager_Close();
	CL_EffectManager_Close();
	CL_Storyteller_Close();
	CL_RpCinematic_EditorClose();
	CL_SoundManager_Close();
	ME_CheckMap();
	if ( !me.indexed )
		ME_IndexModels();

	me.font = re->RegisterFont( "arialnb" );
	if ( !me.font )
		me.font = cls.menuFont;
	if ( me.havePlayerView ) {
		VectorCopy( me.playerOrg, me.camOrg );
		VectorCopy( me.playerAng, me.camAng );
	} else {
		VectorCopy( cl.snap.ps.origin, me.camOrg );
		me.camOrg[2] += cl.snap.ps.viewheight;
		VectorCopy( cl.viewangles, me.camAng );
	}
	me.camAng[ROLL] = 0;
	me.camAng[PITCH] = Com_Clamp( -89.0f, 89.0f, AngleNormalize180( me.camAng[PITCH] ) );

	me.open = qtrue;
	me.field = ME_FIELD_NONE;
	me.grabbing = qfalse;
	me.cursorX = ME_PANEL_X - 120;
	me.cursorY = SCREEN_HEIGHT * 0.5f;
	me.click = qfalse;
	me.wheel = 0;
	me.lastFrameTime = cls.realtime;
	ME_ReleaseKeys();
	Key_SetCatcher( Key_GetCatcher() | KEYCATCH_MAPEDITOR );
}

void CL_MapEditor_Close( void ) {
	if ( !me.open )
		return;
	ME_CancelGrab();
	me.open = qfalse;
	me.field = ME_FIELD_NONE;
	me.storyPick = -1;
	ME_ReleaseKeys();
	Key_SetCatcher( Key_GetCatcher() & ~KEYCATCH_MAPEDITOR );
	if ( me.dirty )
		Com_Printf( S_COLOR_YELLOW "Map editor: the edits aren't saved yet; /mapedit save keeps them\n" );
}

qboolean CL_MapEditor_Active( void ) {
	return me.open;
}

/*
===============================================================================

Picking: what's under the cursor or the crosshair

===============================================================================
*/

// from the camera through the cursor, or straight ahead while flying
static void ME_CursorRay( float x, float y, vec3_t dir ) {
	if ( me.flying || !me.haveView ) {
		AngleVectors( me.camAng, dir, NULL, NULL );
		return;
	}
	VectorCopy( me.viewAxis[0], dir );
	VectorMA( dir, ( 1.0f - 2.0f * x / SCREEN_WIDTH ) * tanf( DEG2RAD( me.viewFovX * 0.5f ) ), me.viewAxis[1], dir );
	VectorMA( dir, ( 1.0f - 2.0f * y / SCREEN_HEIGHT ) * tanf( DEG2RAD( me.viewFovY * 0.5f ) ), me.viewAxis[2], dir );
	VectorNormalize( dir );
}

// how far along the ray it enters the box, -1 if it misses
static float ME_RayBox( const vec3_t org, const vec3_t dir, const vec3_t mins, const vec3_t maxs ) {
	float tmin = 0.0f, tmax = ME_TRACE_DIST;

	for ( int i = 0; i < 3; i++ ) {
		if ( fabsf( dir[i] ) < 0.0001f ) {
			if ( org[i] < mins[i] || org[i] > maxs[i] )
				return -1.0f;
			continue;
		}
		{
			float t1 = ( mins[i] - org[i] ) / dir[i], t2 = ( maxs[i] - org[i] ) / dir[i];

			if ( t1 > t2 ) {
				const float t = t1;
				t1 = t2;
				t2 = t;
			}
			tmin = Q_max( tmin, t1 );
			tmax = Q_min( tmax, t2 );
			if ( tmin > tmax )
				return -1.0f;
		}
	}
	return tmin;
}

static float ME_RayPoint( const vec3_t org, const vec3_t dir, const vec3_t p, float size ) {
	vec3_t mins, maxs;

	VectorSet( mins, p[0] - size, p[1] - size, p[2] - size );
	VectorSet( maxs, p[0] + size, p[1] + size, p[2] + size );
	return ME_RayBox( org, dir, mins, maxs );
}

static void ME_ItemBounds( meItem_t *it, vec3_t mins, vec3_t maxs ) {
	if ( it->kind == ME_ITEM_TRIGGER || it->kind == ME_ITEM_FOG ) {
		VectorSubtract( it->org, it->size, mins );
		VectorAdd( it->org, it->size, maxs );
		return;
	}
	if ( it->kind == ME_ITEM_MODEL || it->kind == ME_ITEM_PROP ) {
		ME_RegisterItem( it );
		if ( it->h ) {
			vec3_t m0, m1, axis[3];

			// its own box, turned: the box around that
			re->ModelBounds( it->h, m0, m1 );
			AnglesToAxis( it->ang, axis );
			ClearBounds( mins, maxs );
			for ( int i = 0; i < 8; i++ ) {
				vec3_t p;

				VectorCopy( it->org, p );
				VectorMA( p, ( ( i & 1 ) ? m1[0] : m0[0] ) * it->scale, axis[0], p );
				VectorMA( p, ( ( i & 2 ) ? m1[1] : m0[1] ) * it->scale, axis[1], p );
				VectorMA( p, ( ( i & 4 ) ? m1[2] : m0[2] ) * it->scale, axis[2], p );
				AddPointToBounds( p, mins, maxs );
			}
			if ( maxs[0] - mins[0] > 1.0f )
				return;
		}
	}
	VectorSet( mins, it->org[0] - ME_POINT_SIZE, it->org[1] - ME_POINT_SIZE, it->org[2] - ME_POINT_SIZE );
	VectorSet( maxs, it->org[0] + ME_POINT_SIZE, it->org[1] + ME_POINT_SIZE, it->org[2] + ME_POINT_SIZE );
}

// where the ray meets one of ours; a model by its own box, turned as it is
static float ME_ItemRay( meItem_t *it, const vec3_t org, const vec3_t dir ) {
	vec3_t mins, maxs;

	ME_RegisterItem( it );
	if ( ( it->kind == ME_ITEM_MODEL || it->kind == ME_ITEM_PROP ) && it->h ) {
		vec3_t axis[3], d, localOrg, localDir;

		re->ModelBounds( it->h, mins, maxs );
		AnglesToAxis( it->ang, axis );
		VectorSubtract( org, it->org, d );
		for ( int i = 0; i < 3; i++ ) {
			localOrg[i] = DotProduct( d, axis[i] ) / it->scale;
			localDir[i] = DotProduct( dir, axis[i] ) / it->scale;
		}
		return ME_RayBox( localOrg, localDir, mins, maxs );
	}
	ME_ItemBounds( it, mins, maxs );
	return ME_RayBox( org, dir, mins, maxs );
}

// the nearest thing the ray meets: ours first when they overlap, they're smaller
static qboolean ME_Pick( const vec3_t org, const vec3_t dir, int *kind, int *index ) {
	float best = ME_TRACE_DIST;
	trace_t tr;
	vec3_t end;

	*kind = ME_SEL_NONE;
	*index = -1;
	// not through walls
	VectorMA( org, ME_TRACE_DIST, dir, end );
	CM_BoxTrace( &tr, org, end, vec3_origin, vec3_origin, 0, CONTENTS_SOLID | CONTENTS_TERRAIN, qfalse );
	best = tr.fraction * ME_TRACE_DIST + 8.0f;

	for ( int i = 0; i < me.numItems && !ME_LocksTab(); i++ ) {
		float t;

		// a removed prop is found in the list only
		if ( me.items[i].kind == ME_ITEM_PROP && me.items[i].removed )
			continue;
		t = ME_ItemRay( &me.items[i], org, dir );
		if ( t >= 0.0f && t < best ) {
			best = t;
			*kind = ME_SEL_ITEM;
			*index = i;
		}
	}
	if ( *kind != ME_SEL_NONE )
		return qtrue;
	for ( int i = 0; i < me.numEnts; i++ ) {
		const meEnt_t *e = &me.ents[i];
		float t;

		if ( !ME_EntDrawn( e ) )
			continue;
		if ( e->bmodel ) {
			t = ME_RayBox( org, dir, e->mins, e->maxs );
			// the box we're in is found only by its walls
			if ( t == 0.0f )
				continue;
		} else {
			t = ME_RayPoint( org, dir, e->origin, ME_POINT_SIZE );
		}
		if ( t >= 0.0f && t < best ) {
			best = t;
			*kind = ME_SEL_ENT;
			*index = i;
		}
	}
	return (qboolean)( *kind != ME_SEL_NONE );
}

static void ME_ScrollToSelection( void ) {
	if ( me.selKind == ME_SEL_ENT ) {
		int row = 0;

		for ( int i = 0; i < me.sel; i++ ) {
			if ( ME_EntMatches( &me.ents[i] ) )
				row++;
		}
		me.entScroll = Q_max( 0, row - 4 );
	}
}

// for the storyteller, the trigger of the kind it wants the ray meets: kind ME_SEL_ENT or
// ME_SEL_ITEM; qfalse if none
static qboolean ME_StoryPickAt( const vec3_t org, const vec3_t dir, int *kind, int *index ) {
	float best;
	trace_t tr;
	vec3_t end;

	*kind = ME_SEL_NONE;
	*index = -1;
	VectorMA( org, ME_TRACE_DIST, dir, end );
	CM_BoxTrace( &tr, org, end, vec3_origin, vec3_origin, 0, CONTENTS_SOLID | CONTENTS_TERRAIN, qfalse );
	best = tr.fraction * ME_TRACE_DIST + 8.0f;
	if ( me.storyPick == 2 ) {
		for ( int i = 0; i < me.numItems; i++ ) {
			float t;

			if ( me.items[i].kind != ME_ITEM_TRIGGER || !me.items[i].use )
				continue;
			t = ME_ItemRay( &me.items[i], org, dir );
			if ( t >= 0.0f && t < best ) {
				best = t;
				*kind = ME_SEL_ITEM;
				*index = i;
			}
		}
	} else {
		for ( int i = 0; i < me.numEnts; i++ ) {
			const meEnt_t *e = &me.ents[i];
			float t;

			if ( !e->bmodel || e->kind != ( me.storyPick == 0 ? ME_ENT_TRIGGER : ME_ENT_BRUSH ) )
				continue;
			t = ME_RayBox( org, dir, e->mins, e->maxs );
			// a door's box we're in is found by its walls; a trigger's we stand in counts
			if ( t < 0.0f || ( t == 0.0f && me.storyPick == 1 ) || t >= best )
				continue;
			best = t;
			*kind = ME_SEL_ENT;
			*index = i;
		}
	}
	return (qboolean)( *kind != ME_SEL_NONE );
}

static void ME_ClickWorld( float x, float y ) {
	vec3_t dir;
	int kind, index;

	// for the storyteller: the trigger clicked goes in its step
	if ( me.storyPick >= 0 ) {
		ME_CursorRay( x, y, dir );
		if ( !ME_StoryPickAt( me.camOrg, dir, &kind, &index ) ) {
			ME_Message( S_COLOR_YELLOW "Nothing of that kind there" );
			return;
		}
		if ( kind == ME_SEL_ITEM ) {
			if ( !me.items[index].name[0] ) {
				ME_Select( kind, index );
				ME_Message( S_COLOR_YELLOW "Give it a name first, in the Triggers tab, then click it again" );
				return;
			}
			CL_Storyteller_TriggerPicked( me.items[index].name );
		} else {
			CL_Storyteller_TriggerPicked( va( "*%i", me.ents[index].bmodel ) );
		}
		return;
	}

	if ( me.grabbing ) {
		me.grabbing = qfalse;
		ME_Changed();
		return;
	}
	ME_CursorRay( x, y, dir );
	// the Locks tab picks a door or a trigger, never a prop
	if ( ME_LocksTab() ) {
		if ( ME_Pick( me.camOrg, dir, &kind, &index ) && kind == ME_SEL_ENT )
			ME_Select( kind, index );
		return;
	}
	if ( !ME_Pick( me.camOrg, dir, &kind, &index ) || ( kind == ME_SEL_ENT && me.ents[index].kind == ME_ENT_TRIGGER ) ) {
		float dist;
		const int tri = ME_TriHit( me.camOrg, dir, &dist );

		// a prop of the map's before a trigger it's standing in; joined to the whole map, the
		// brush clicked
		me.candBrush = ME_BrushAt( tri, me.camOrg, dir, dist );
		if ( ME_ChooseProp( tri ) || ME_ChooseBrush( me.candBrush ) )
			return;
		if ( kind == ME_SEL_NONE ) {
			me.selKind = ME_SEL_NONE;
			me.sel = -1;
			return;
		}
	}
	ME_Select( kind, index );
	if ( kind == ME_SEL_ITEM )
		me.tab = ME_ItemTab( me.items[index].kind );
	else
		me.tab = ME_TAB_MAP;
	ME_ScrollToSelection();
}

/*
===============================================================================

The arrows: what's chosen dragged along X, Y or Z

===============================================================================
*/

static qboolean ME_ProjectAny( const vec3_t point, float *x, float *y );

// what they move: ours, not while carried, not a map prop that's hidden
static meItem_t *ME_GizmoItem( void ) {
	meItem_t *it = ME_SelectedItem();

	if ( me.storyPick >= 0 )
		return NULL;
	if ( !it || me.grabbing || ( it->kind == ME_ITEM_PROP && it->removed ) )
		return NULL;
	return it;
}

// about as long on the screen however far it is
static float ME_GizmoLen( const meItem_t *it ) {
	return Com_Clamp( 12.0f, 512.0f, Distance( it->org, me.camOrg ) * 0.15f );
}

// the arrow under the cursor, -1 none
static int ME_GizmoHover( void ) {
	const meItem_t *it = ME_GizmoItem();
	float ox, oy, best = ME_GIZMO_PX;
	int found = -1;

	if ( !it || me.flying || me.cursorX >= ME_PANEL_X || !ME_ProjectAny( it->org, &ox, &oy ) )
		return -1;
	for ( int a = 0; a < 3; a++ ) {
		vec3_t tip;
		float tx, ty, dx, dy, len2, u, d;

		VectorMA( it->org, ME_GizmoLen( it ), meAxes[a], tip );
		if ( !ME_ProjectAny( tip, &tx, &ty ) )
			continue;
		// the cursor to the arrow, on the screen
		dx = tx - ox;
		dy = ty - oy;
		len2 = dx * dx + dy * dy;
		u = len2 > 0.01f ? Com_Clamp( 0.0f, 1.0f, ( ( me.cursorX - ox ) * dx + ( me.cursorY - oy ) * dy ) / len2 ) : 0.0f;
		d = sqrtf( Square( ox + u * dx - me.cursorX ) + Square( oy + u * dy - me.cursorY ) );
		if ( d < best ) {
			best = d;
			found = a;
		}
	}
	return found;
}

// how far along the arrow's line from from the cursor's ray passes nearest; qfalse when the
// camera looks along it
static qboolean ME_AxisParam( const vec3_t from, int axis, float *t ) {
	vec3_t dir, w;
	float b, d, e, denom;

	ME_CursorRay( me.cursorX, me.cursorY, dir );
	VectorSubtract( from, me.camOrg, w );
	b = DotProduct( meAxes[axis], dir );
	d = DotProduct( meAxes[axis], w );
	e = DotProduct( dir, w );
	denom = 1.0f - b * b;
	if ( denom < 0.001f )
		return qfalse;
	*t = ( b * e - d ) / denom;
	return qtrue;
}

// the camera in front of a spot, looking at it
static void ME_GoTo( const vec3_t spot, float size ) {
	vec3_t forward;

	AngleVectors( me.camAng, forward, NULL, NULL );
	VectorMA( spot, -( 96.0f + Q_min( size, 1024.0f ) ), forward, me.camOrg );
}

/*
===============================================================================

Editor input

===============================================================================
*/

static qboolean ME_Held( int upper ) {
	return (qboolean)( me.held[upper] || me.held[upper + ( 'a' - 'A' )] );
}

static qboolean ME_ShiftDown( void ) {
	return (qboolean)( me.held[A_SHIFT] || me.held[A_SHIFT2] );
}

static qboolean ME_CtrlDown( void ) {
	return (qboolean)( me.held[A_CTRL] || me.held[A_CTRL2] );
}

static qboolean ME_IsToggleKey( int key ) {
	const char *binding;

	if ( me.field && key >= A_SPACE && key <= A_TILDE )
		return qfalse;
	binding = Key_GetBinding( key );
	return (qboolean)( VALIDSTRING( binding ) && !Q_stricmp( binding, ME_TOGGLE_CMD ) );
}

static char *ME_FieldBuffer( int *size ) {
	meItem_t *it = ME_SelectedItem();

	switch ( me.field ) {
	case ME_FIELD_FILTER:
		*size = sizeof( me.filter );
		return me.filter;
	case ME_FIELD_SEARCH:
		*size = sizeof( me.search );
		return me.search;
	case ME_FIELD_COMMAND:
		if ( !it || it->kind != ME_ITEM_TRIGGER )
			return NULL;
		*size = sizeof( it->command );
		return it->command;
	case ME_FIELD_TRIGGERNAME:
		if ( !it || it->kind != ME_ITEM_TRIGGER )
			return NULL;
		*size = sizeof( it->name );
		return it->name;
	case ME_FIELD_TRIGGERDISPLAY:
		if ( !it || it->kind != ME_ITEM_TRIGGER )
			return NULL;
		*size = sizeof( it->display );
		return it->display;
	case ME_FIELD_PATTERN:
		if ( me.selKind != ME_SEL_STYLE || me.sel < 0 || me.sel >= MAX_LIGHT_STYLES )
			return NULL;
		*size = sizeof( me.styles[me.sel].pattern );
		return me.styles[me.sel].pattern;
	case ME_FIELD_LAYOUT:
		*size = sizeof( me.layoutName );
		return me.layoutName;
	case ME_FIELD_SOUND:
		if ( !it || it->kind != ME_ITEM_SPEAKER )
			return NULL;
		*size = sizeof( it->noise );
		return it->noise;
	case ME_FIELD_LOCKNAME:
		return ME_LockNameBuffer( size );
	default:
		return NULL;
	}
}

static void ME_FieldEdited( void ) {
	if ( me.field == ME_FIELD_FILTER )
		me.entScroll = 0;
	else if ( me.field == ME_FIELD_SEARCH )
		me.searchScroll = 0;
	else if ( me.field != ME_FIELD_LAYOUT && me.field != ME_FIELD_LOCKNAME )
		ME_Changed();
	if ( me.field == ME_FIELD_PATTERN && me.selKind == ME_SEL_STYLE && me.sel >= 0 && me.sel < MAX_LIGHT_STYLES )
		me.styles[me.sel].override = qtrue;
	// another sound: heard as typed, once it's one
	if ( me.field == ME_FIELD_SOUND && ME_SelectedItem() ) {
		ME_SelectedItem()->sfxRegistered = qfalse;
		me.soundScroll = 0;
	}
}

static qboolean ME_AllowChar( int ch );

void CL_MapEditor_KeyEvent( int key, qboolean down ) {
	if ( !me.open )
		return;

	if ( key >= 0 && key < MAX_KEYS )
		me.held[key] = down;
	if ( !down ) {
		if ( key == A_MOUSE2 )
			me.flying = qfalse;
		if ( key == A_MOUSE1 && me.dragAxis >= 0 ) {
			me.dragAxis = -1;
			ME_Changed();
		}
		return;
	}
	if ( ME_IsToggleKey( key ) ) {
		CL_MapEditor_Close();
		return;
	}
	// the warning only takes clicks
	if ( me.experimentalBox ) {
		if ( key == A_MOUSE1 ) {
			me.click = qtrue;
			me.clickX = me.cursorX;
			me.clickY = me.cursorY;
		}
		return;
	}

	switch ( key ) {
	case A_MOUSE1:
		// an arrow pointed at: dragged
		if ( me.gizmoAxis >= 0 && ME_GizmoItem() && ME_AxisParam( ME_GizmoItem()->org, me.gizmoAxis, &me.dragT0 ) ) {
			me.dragAxis = me.gizmoAxis;
			VectorCopy( ME_GizmoItem()->org, me.dragFrom );
			me.field = ME_FIELD_NONE;
			return;
		}
		me.click = qtrue;
		me.clickX = me.cursorX;
		me.clickY = me.cursorY;
		return;
	case A_MOUSE2:
		me.flying = qtrue;
		me.field = ME_FIELD_NONE;
		me.dragAxis = -1;
		return;
	case A_MWHEELUP:
		if ( me.flying )
			me.camFov = Com_Clamp( 10.0f, 130.0f, me.camFov - 5.0f );
		else
			me.wheel--;
		return;
	case A_MWHEELDOWN:
		if ( me.flying )
			me.camFov = Com_Clamp( 10.0f, 130.0f, me.camFov + 5.0f );
		else
			me.wheel++;
		return;
	default:
		break;
	}
	if ( me.flying ) {
		// carrying something while flying: G drops it
		if ( ( key == A_CAP_G || key == A_LOW_G ) && me.grabbing ) {
			me.grabbing = qfalse;
			ME_Changed();
		}
		return;
	}

	if ( me.field ) {
		int size;
		char *buf = ME_FieldBuffer( &size );
		const int edit = buf ? WG_EditKey( buf, size, key, ME_AllowChar ) : WG_EDIT_NONE;

		if ( edit ) {
			if ( edit & WG_EDIT_CHANGED )
				ME_FieldEdited();
		} else if ( key == A_ENTER || key == A_KP_ENTER ) {
			if ( me.field == ME_FIELD_LAYOUT )
				ME_SaveLayout( me.layoutName );
			me.field = ME_FIELD_NONE;
		}
		return;
	}

	switch ( key ) {
	case A_CAP_G:
	case A_LOW_G:
		if ( me.selKind == ME_SEL_PROP ) {
			ME_LiftProp( qfalse, qtrue );
			break;
		}
		ME_Grab();
		if ( !me.grabbing )
			ME_Changed();
		break;
	case A_DELETE:
	case A_BACKSPACE:
		if ( me.selKind == ME_SEL_PROP ) {
			ME_LiftProp( qtrue, qfalse );
		} else if ( ME_SelectedItem() && ME_SelectedItem()->kind == ME_ITEM_PROP ) {
			// a map prop is removed, Put back undoes it
			ME_SelectedItem()->removed = qtrue;
			ME_Changed();
		} else {
			ME_DeleteSelected();
		}
		break;
	case A_CAP_S:
	case A_LOW_S:
		if ( ME_CtrlDown() )
			ME_Save();
		break;
	case A_CAP_D:
	case A_LOW_D:
		if ( ME_CtrlDown() )
			ME_CopySelected();
		break;
	default:
		break;
	}
}

static qboolean ME_FieldChar( int field, int ch ) {
	switch ( field ) {
	case ME_FIELD_PATTERN:
		return (qboolean)( ch >= 'a' && ch <= 'z' );
	case ME_FIELD_LAYOUT:
		return ME_LayoutChar( ch );
	case ME_FIELD_COMMAND:
		// an entity's value can't hold a quote
		return (qboolean)( ch >= ' ' && ch < 127 && ch != '"' );
	case ME_FIELD_TRIGGERNAME:
	case ME_FIELD_TRIGGERDISPLAY:
		// shared, they can't hold what splits the edits up
		return (qboolean)( ch >= ( me.field == ME_FIELD_TRIGGERNAME ? '!' : ' ' ) && ch < 127 && !strchr( "\"|~`", ch ) );
	case ME_FIELD_LOCKNAME:
		// a word or more, nothing that would end the command
		return (qboolean)( ch >= ' ' && ch < 127 && ch != '"' && ch != ';' );
	default:
		return (qboolean)( ch > ' ' && ch < 127 && ch != '"' );
	}
}

// a character the box typed in takes
static qboolean ME_AllowChar( int ch ) {
	return ME_FieldChar( me.field, ch );
}

void CL_MapEditor_CharEvent( int ch ) {
	int size;
	char *buf;

	if ( !me.open || !me.field || me.flying || me.experimentalBox )
		return;
	if ( me.field == ME_FIELD_PATTERN && ch >= 'A' && ch <= 'Z' )
		ch += 'a' - 'A';
	if ( !( buf = ME_FieldBuffer( &size ) ) )
		return;
	if ( WG_EditChar( buf, size, ch, ME_AllowChar ) )
		ME_FieldEdited();
}

void CL_MapEditor_MouseEvent( int dx, int dy ) {
	if ( !me.open )
		return;
	if ( me.flying ) {
		me.camAng[YAW] -= dx * cl_sensitivity->value * m_yaw->value;
		me.camAng[PITCH] = Com_Clamp( -89.0f, 89.0f, me.camAng[PITCH] + dy * cl_sensitivity->value * m_pitch->value );
	} else {
		me.cursorX = Com_Clamp( 0, SCREEN_WIDTH, me.cursorX + dx );
		me.cursorY = Com_Clamp( 0, SCREEN_HEIGHT, me.cursorY + dy );
	}
}

// escape backs out one level: the text box, carrying, the selection, then the editor
void CL_MapEditor_Escape( void ) {
	// picking for the storyteller: straight back to it
	if ( me.storyPick >= 0 && !me.field && !me.experimentalBox ) {
		CL_MapEditor_Close();
		return;
	}
	if ( me.experimentalBox ) {
		me.experimentalBox = qfalse;
	} else if ( me.field ) {
		me.field = ME_FIELD_NONE;
	} else if ( me.grabbing ) {
		ME_CancelGrab();
	} else if ( me.selKind != ME_SEL_NONE ) {
		me.selKind = ME_SEL_NONE;
		me.sel = -1;
	} else {
		CL_MapEditor_Close();
	}
}

/*
===============================================================================

Every frame

===============================================================================
*/

// the use trigger the player looks at, within reach and not behind a wall; -1 none
static int ME_UseAimed( void ) {
	vec3_t eye, ang, dir, end, mins, maxs;
	trace_t tr;
	float best;
	int found = -1;

	VectorCopy( cl.snap.ps.origin, eye );
	eye[2] += cl.snap.ps.viewheight;
	for ( int i = 0; i < 3; i++ )
		ang[i] = cl.viewangles[i] + SHORT2ANGLE( cl.snap.ps.delta_angles[i] );
	AngleVectors( ang, dir, NULL, NULL );
	VectorMA( eye, ME_USE_REACH, dir, end );
	CM_BoxTrace( &tr, eye, end, vec3_origin, vec3_origin, 0, CONTENTS_SOLID | CONTENTS_TERRAIN, qfalse );
	best = tr.fraction * ME_USE_REACH + 1.0f;
	for ( int i = 0; i < me.numItems; i++ ) {
		meItem_t *it = &me.items[i];
		float t;

		if ( it->kind != ME_ITEM_TRIGGER || !it->use )
			continue;
		ME_ItemBounds( it, mins, maxs );
		t = ME_RayBox( eye, dir, mins, maxs );
		if ( t >= 0.0f && t < best ) {
			best = t;
			found = i;
		}
	}
	return found;
}

// the storyteller's: the name of the use trigger looked at, "" if it has none, NULL if none's looked at
const char *CL_MapEditor_AimedUseTrigger( void ) {
	if ( me.useAim < 0 || me.useAim >= me.numItems )
		return NULL;
	return me.items[me.useAim].name;
}

// a use trigger does nothing of its own: the player reaches out to it (RPMod's emote), and the
// storytellers hear it was used; their stories decide what it does. A held one has its own emote
static void ME_UseTrigger( meItem_t *it, qboolean reach ) {
	if ( reach && cls.state == CA_ACTIVE )
		CL_AddReliableCommand( "emreach", qfalse );
	CL_Storyteller_TriggerUsed( it->name );
}

// held: the player works the console (RPMod's emconsole) until it's used or let go
static void ME_HoldEmote( qboolean on ) {
	if ( on == me.holdEmote )
		return;
	me.holdEmote = on;
	if ( cls.state != CA_ACTIVE )
		return;
	if ( on ) {
		CL_AddReliableCommand( "emconsole", qfalse );
		return;
	}
	// ended as the player would: a tap of crouch
	Cbuf_AddText( "+movedown " ME_CROUCH_KEY "\n" );
	me.crouchUntil = cls.realtime + ME_CROUCH_MS;
}

// our triggers: walking into one runs its command, and pressing use looking at a use one
static void ME_RunTriggers( void ) {
	vec3_t mins, maxs;
	qboolean useDown = (qboolean)( ( cl.cmds[cl.cmdNumber & CMD_MASK].buttons & BUTTON_USE ) != 0 );

	// the crouch that ended a held emote, let go
	if ( me.crouchUntil && cls.realtime >= me.crouchUntil ) {
		me.crouchUntil = 0;
		Cbuf_AddText( "-movedown " ME_CROUCH_KEY "\n" );
	}
	if ( me.open || !ME_Shown() || cl.snap.ps.pm_type == PM_SPECTATOR || cl.snap.ps.stats[STAT_HEALTH] <= 0 ) {
		for ( int i = 0; i < me.numItems; i++ )
			me.items[i].inside = qfalse;
		me.useAim = -1;
		me.holdItem = -1;
		ME_HoldEmote( qfalse );
		me.useDown = useDown;
		return;
	}
	me.useAim = ME_UseAimed();
	// a press uses it; one with a usetime only once it's been held that long on it
	if ( useDown && me.useAim >= 0 ) {
		meItem_t *it = &me.items[me.useAim];

		if ( !me.useDown ) {
			me.holdItem = me.useAim;
			me.holdStart = cls.realtime;
			me.holdDone = qfalse;
			if ( it->usetime <= 0 ) {
				ME_UseTrigger( it, qtrue );
				me.holdDone = qtrue;
			} else {
				ME_HoldEmote( qtrue );
			}
		} else if ( me.holdItem != me.useAim ) {
			me.holdItem = -1;	// looked away: let go
		}
		if ( me.holdItem == me.useAim && !me.holdDone && it->usetime > 0 && cls.realtime - me.holdStart >= it->usetime ) {
			ME_UseTrigger( it, qfalse );
			me.holdDone = qtrue;
		}
	} else {
		me.holdItem = -1;
	}
	// used, let go or looked away: the emote ends
	if ( me.holdEmote && ( me.holdItem < 0 || me.holdDone ) )
		ME_HoldEmote( qfalse );
	me.useDown = useDown;
	// the player's box
	VectorSet( mins, cl.snap.ps.origin[0] - 15, cl.snap.ps.origin[1] - 15, cl.snap.ps.origin[2] - 24 );
	VectorSet( maxs, cl.snap.ps.origin[0] + 15, cl.snap.ps.origin[1] + 15, cl.snap.ps.origin[2] + 40 );
	for ( int i = 0; i < me.numItems; i++ ) {
		meItem_t *it = &me.items[i];
		qboolean inside = qtrue;

		if ( it->kind != ME_ITEM_TRIGGER || it->use )
			continue;
		for ( int j = 0; j < 3; j++ ) {
			if ( maxs[j] < it->org[j] - it->size[j] || mins[j] > it->org[j] + it->size[j] )
				inside = qfalse;
		}
		if ( inside && !it->inside && it->command[0] && !( it->once && it->fired ) ) {
			it->fired = qtrue;
			Cbuf_AddText( va( "%s\n", it->command ) );
		}
		it->inside = inside;
	}
}

static void ME_EditorFrame( void ) {
	vec3_t forward, right, move, end, dir;
	float dt, speed;
	trace_t tr;

	if ( !( Key_GetCatcher() & KEYCATCH_MAPEDITOR ) ) {
		// something else cleared the catchers
		ME_CancelGrab();
		me.open = qfalse;
		me.storyPick = -1;
		me.field = ME_FIELD_NONE;
		ME_ReleaseKeys();
		return;
	}

	dt = Com_Clamp( 0.0f, 0.1f, ( cls.realtime - me.lastFrameTime ) / 1000.0f );
	me.lastFrameTime = cls.realtime;

	if ( me.flying ) {
		AngleVectors( me.camAng, forward, right, NULL );
		VectorClear( move );
		if ( ME_Held( A_CAP_W ) )	VectorAdd( move, forward, move );
		if ( ME_Held( A_CAP_S ) )	VectorSubtract( move, forward, move );
		if ( ME_Held( A_CAP_D ) )	VectorAdd( move, right, move );
		if ( ME_Held( A_CAP_A ) )	VectorSubtract( move, right, move );
		if ( me.held[A_SPACE] )		move[2] += 1.0f;
		if ( ME_Held( A_CAP_C ) )	move[2] -= 1.0f;
		speed = ME_FLY_SPEED;
		if ( ME_ShiftDown() )
			speed *= 3.0f;
		if ( ME_CtrlDown() )
			speed *= 0.25f;
		if ( VectorNormalize( move ) > 0.0f )
			VectorMA( me.camOrg, speed * dt, move, me.camOrg );
	}

	// what the crosshair, or the cursor, is on
	ME_CursorRay( me.cursorX, me.cursorY, dir );
	if ( me.flying || me.cursorX < ME_PANEL_X ) {
		VectorMA( me.camOrg, ME_TRACE_DIST, dir, end );
		CM_BoxTrace( &tr, me.camOrg, end, vec3_origin, vec3_origin, 0, CONTENTS_SOLID | CONTENTS_TERRAIN, qfalse );
		me.haveAim = (qboolean)( tr.fraction < 1.0f && !tr.allsolid );
		VectorCopy( tr.endpos, me.aim );
		VectorCopy( tr.plane.normal, me.aimNormal );
	}
	// an arrow dragged moves what's chosen along it, in whole units (Shift 16, Ctrl a quarter)
	if ( me.dragAxis >= 0 ) {
		meItem_t *it = ME_GizmoItem();
		float t;

		if ( !it || !me.held[A_MOUSE1] ) {
			me.dragAxis = -1;
		} else if ( ME_AxisParam( me.dragFrom, me.dragAxis, &t ) ) {
			const float snap = ME_ShiftDown() ? 16.0f : ME_CtrlDown() ? 0.25f : 1.0f;
			const float move = floorf( ( t - me.dragT0 ) / snap + 0.5f ) * snap;

			VectorCopy( me.dragFrom, it->org );
			it->org[me.dragAxis] += move;
		}
		me.gizmoAxis = me.dragAxis;
	} else {
		me.gizmoAxis = ME_GizmoHover();
	}
	if ( !me.flying && me.cursorX < ME_PANEL_X && me.storyPick >= 0 )
		ME_StoryPickAt( me.camOrg, dir, &me.hoverKind, &me.hover );
	else if ( !me.flying && me.cursorX < ME_PANEL_X && !me.grabbing && me.gizmoAxis < 0 )
		ME_Pick( me.camOrg, dir, &me.hoverKind, &me.hover );
	else
		me.hoverKind = ME_SEL_NONE;

	// carried to the crosshair
	if ( me.grabbing ) {
		meItem_t *it = ME_SelectedItem();

		if ( !it )
			me.grabbing = qfalse;
		else if ( me.haveAim )
			ME_PlaceAt( it, me.aim, me.aimNormal );
	}
}

void CL_MapEditor_Frame( void ) {
	if ( cls.state != CA_ACTIVE || !cls.cgameStarted )
		return;
	ME_CheckMap();
	if ( me.numHidden == 0 ) {
		for ( int i = 0; i < me.numEnts; i++ ) {
			if ( me.ents[i].hidden ) {
				ME_RebuildHidden();
				break;
			}
		}
	}
	if ( me.propsDirty || me.propsShown != ME_Shown() )
		ME_ApplyProps();
	if ( !me.fogsRead )
		ME_ReadFogs();
	if ( me.fogsDirty || me.fogsShown != ME_Shown() || me.fogItemsSum != ME_FogItemsSum() )
		ME_ApplyFogs();
	ME_RunTriggers();
	ME_RunShare();
	ME_RunLocks();
	if ( me.open )
		ME_EditorFrame();
}

/*
===============================================================================

The scene: our things always, the editor's lines while it's open

===============================================================================
*/

// a line; fx RF_DEPTHHACK draws it over what's in front of it, thicker
static void ME_LineFx( const vec3_t a, const vec3_t b, const byte *color, int fx ) {
	refEntity_t ent;
	vec3_t mid, toCam;

	if ( me.numLines >= ME_MAX_LINES )
		return;
	me.numLines++;
	VectorAdd( a, b, mid );
	VectorScale( mid, 0.5f, mid );
	VectorSubtract( mid, me.camOrg, toCam );

	memset( &ent, 0, sizeof( ent ) );
	ent.reType = RT_LINE;
	VectorCopy( a, ent.origin );
	VectorCopy( b, ent.oldorigin );
	ent.radius = ( 0.3f + VectorLength( toCam ) * 0.0015f ) * ( ( fx & RF_DEPTHHACK ) ? 2.0f : 1.0f );
	ent.customShader = cls.whiteShader;
	memcpy( ent.shaderRGBA, color, sizeof( ent.shaderRGBA ) );
	ent.renderfx = RF_NOSHADOW | fx;
	re->AddRefEntityToScene( &ent );
}

static void ME_Line( const vec3_t a, const vec3_t b, const byte *color ) {
	ME_LineFx( a, b, color, 0 );
}

static void ME_Cross( const vec3_t p, float size, const byte *color ) {
	for ( int i = 0; i < 3; i++ ) {
		vec3_t a, b;

		VectorCopy( p, a );
		VectorCopy( p, b );
		a[i] -= size;
		b[i] += size;
		ME_Line( a, b, color );
	}
}

static void ME_BoxLines( const vec3_t mins, const vec3_t maxs, const byte *color ) {
	vec3_t c[8];

	for ( int i = 0; i < 8; i++ )
		VectorSet( c[i], ( i & 1 ) ? maxs[0] : mins[0], ( i & 2 ) ? maxs[1] : mins[1], ( i & 4 ) ? maxs[2] : mins[2] );
	for ( int i = 0; i < 8; i++ ) {
		for ( int bit = 1; bit < 8; bit <<= 1 ) {
			if ( !( i & bit ) )
				ME_Line( c[i], c[i | bit], color );
		}
	}
}

static void ME_Circle( const vec3_t center, float radius, int axis, const byte *color ) {
	vec3_t prev, p;

	for ( int i = 0; i <= 32; i++ ) {
		const float a = DEG2RAD( i * 11.25f );
		const int u = ( axis + 1 ) % 3, v = ( axis + 2 ) % 3;

		VectorCopy( center, p );
		p[u] += cosf( a ) * radius;
		p[v] += sinf( a ) * radius;
		if ( i )
			ME_Line( prev, p, color );
		VectorCopy( p, prev );
	}
}

static void ME_ItemAxis( const meItem_t *it, matrix3_t axis ) {
	AnglesToAxis( it->ang, axis );
	if ( it->scale != 1.0f ) {
		for ( int i = 0; i < 3; i++ )
			VectorScale( axis[i], it->scale, axis[i] );
	}
}

// a light's colour now, with its style
static void ME_LightNow( const meItem_t *it, vec3_t color ) {
	VectorCopy( it->color, color );
	if ( it->style >= 0 && it->style < MAX_LIGHT_STYLES ) {
		color4ub_t c;

		re->GetLightStyle( it->style, c );
		color[0] *= c[0] / 255.0f;
		color[1] *= c[1] / 255.0f;
		color[2] *= c[2] / 255.0f;
	}
}

// a speaker's sound, registered when first wanted; 0 if there's none to play. The game keeps
// MP3s of sound/chars to stream, and a stream can't loop
static sfxHandle_t ME_SpeakerSound( meItem_t *it ) {
	if ( !it->sfxRegistered ) {
		it->sfxRegistered = qtrue;
		it->sfx = 0;
		if ( it->noise[0] && Q_stricmpn( it->noise, "sound/chars/", 12 ) && FS_ReadFile( it->noise, NULL ) > 0 )
			it->sfx = S_RegisterSound( it->noise );
	}
	return it->sfx;
}

// the speakers, round and round: near them, or as loud everywhere, where the listener's ear
// is. Each frame, as the cgame hands its own looping sounds to the mixer
void CL_MapEditor_Sounds( const vec3_t ear ) {
	if ( cls.state != CA_ACTIVE || !ME_Shown() || !me.map[0] || Q_stricmp( me.map, cl.mapname ) )
		return;
	for ( int i = 0; i < me.numItems; i++ ) {
		meItem_t *it = &me.items[i];
		sfxHandle_t sfx;

		if ( it->kind != ME_ITEM_SPEAKER || it->off || !( sfx = ME_SpeakerSound( it ) ) )
			continue;
		S_AddLoopingSound( ENTITYNUM_WORLD, it->global ? ear : it->org, vec3_origin, sfx );
	}
}

// the models and lights, in every main view
static void ME_AddItems( void ) {
	for ( int i = 0; i < me.numItems; i++ ) {
		meItem_t *it = &me.items[i];

		if ( it->kind == ME_ITEM_MODEL || it->kind == ME_ITEM_PROP ) {
			refEntity_t ent;

			ME_RegisterItem( it );
			if ( !it->h || ( it->kind == ME_ITEM_PROP && it->removed ) )
				continue;
			memset( &ent, 0, sizeof( ent ) );
			ent.hModel = it->h;
			VectorCopy( it->org, ent.origin );
			VectorCopy( it->org, ent.oldorigin );
			VectorCopy( it->org, ent.lightingOrigin );
			ME_ItemAxis( it, ent.axis );
			ent.nonNormalizedAxes = (qboolean)( it->scale != 1.0f );
			re->AddRefEntityToScene( &ent );
		} else if ( it->kind == ME_ITEM_LIGHT ) {
			vec3_t color;

			ME_LightNow( it, color );
			if ( color[0] + color[1] + color[2] > 0.01f )
				re->AddLightToScene( it->org, it->radius, color[0], color[1], color[2] );
		}
	}
}

typedef struct {
	int		index;
	float	dist;
} meNear_t;

static int QDECL ME_CompareNear( const void *a, const void *b ) {
	const float d = ( (const meNear_t *)a )->dist - ( (const meNear_t *)b )->dist;

	return d < 0 ? -1 : d > 0 ? 1 : 0;
}

static qboolean ME_IsSel( int kind, int index ) {
	return (qboolean)( me.selKind == kind && me.sel == index );
}

static qboolean ME_IsHover( int kind, int index ) {
	return (qboolean)( me.hoverKind == kind && me.hover == index );
}

static void ME_AddEditorLines( void ) {
	static meNear_t near_[ME_MAX_ENTS];
	int numNear = 0;

	me.numLines = 0;
	if ( me.haveAim && ( me.flying || me.grabbing ) )
		ME_Cross( me.aim, 4.0f, meAimColor );

	// ours first, they're what's worked on
	for ( int i = 0; i < me.numItems; i++ ) {
		meItem_t *it = &me.items[i];
		const qboolean sel = ME_IsSel( ME_SEL_ITEM, i ), hover = ME_IsHover( ME_SEL_ITEM, i );
		vec3_t mins, maxs;

		if ( it->kind == ME_ITEM_FOG ) {
			byte color[4] = { (byte)Com_Clampi( 0, 255, ME_Round( it->color[0] * 255 ) ), (byte)Com_Clampi( 0, 255, ME_Round( it->color[1] * 255 ) ),
				(byte)Com_Clampi( 0, 255, ME_Round( it->color[2] * 255 ) ), 255 };

			ME_ItemBounds( it, mins, maxs );
			ME_BoxLines( mins, maxs, sel || hover ? meSelColor : color );
		} else if ( it->kind == ME_ITEM_TRIGGER ) {
			ME_ItemBounds( it, mins, maxs );
			ME_BoxLines( mins, maxs, sel || hover ? meSelColor : it->use ? meUseTrigger : meOurTrigger );
		} else if ( it->kind == ME_ITEM_SPEAKER ) {
			ME_Cross( it->org, sel || hover ? 10.0f : 6.0f, sel || hover ? meSelColor : it->off ? meHiddenColor : meSpeakerColor );
			if ( sel && !it->global )
				ME_Circle( it->org, ME_SOUND_NEAR, 2, meReachColor );
		} else if ( it->kind == ME_ITEM_LIGHT ) {
			byte color[4];
			vec3_t c;

			ME_LightNow( it, c );
			color[0] = (byte)Com_Clampi( 0, 255, ME_Round( it->color[0] * 255 ) );
			color[1] = (byte)Com_Clampi( 0, 255, ME_Round( it->color[1] * 255 ) );
			color[2] = (byte)Com_Clampi( 0, 255, ME_Round( it->color[2] * 255 ) );
			color[3] = 255;
			ME_Cross( it->org, sel || hover ? 10.0f : 6.0f, sel || hover ? meSelColor : color );
			if ( sel ) {
				// how far it reaches
				for ( int a = 0; a < 3; a++ )
					ME_Circle( it->org, it->radius, a, meReachColor );
			}
		} else {
			ME_Cross( it->org, 4.0f, it->kind == ME_ITEM_PROP && it->removed ? meHiddenColor : meItemColor );
			if ( sel || hover ) {
				ME_ItemBounds( it, mins, maxs );
				if ( it->h ) {
					vec3_t m0, m1;

					// its own box, unturned
					re->ModelBounds( it->h, m0, m1 );
					VectorMA( it->org, it->scale, m0, mins );
					VectorMA( it->org, it->scale, m1, maxs );
				}
				ME_BoxLines( mins, maxs, meSelColor );
			}
		}
	}

	// a prop of the map's, chosen
	if ( me.selKind == ME_SEL_PROP )
		ME_BoxLines( me.candMins, me.candMaxs, meCandColor );
	// a fog volume of the map's, chosen
	if ( me.selKind == ME_SEL_FOG && me.sel >= 1 && me.sel < me.numFogs && me.sel != me.globalFog )
		ME_BoxLines( me.fogs[me.sel].mins, me.fogs[me.sel].maxs, meCandColor );

	// the arrows on what's chosen, over what's in front of them
	if ( ME_GizmoItem() ) {
		const meItem_t *it = ME_GizmoItem();
		const float len = ME_GizmoLen( it );

		for ( int a = 0; a < 3; a++ ) {
			const byte *color = a == me.gizmoAxis ? meAxisLit : meAxisColors[a];
			vec3_t tip, back, side;

			VectorMA( it->org, len, meAxes[a], tip );
			ME_LineFx( it->org, tip, color, RF_DEPTHHACK );
			VectorMA( tip, -len * 0.2f, meAxes[a], back );
			for ( int k = 1; k <= 2; k++ ) {
				VectorMA( back, len * 0.08f, meAxes[( a + k ) % 3], side );
				ME_LineFx( tip, side, color, RF_DEPTHHACK );
				VectorMA( back, -len * 0.08f, meAxes[( a + k ) % 3], side );
				ME_LineFx( tip, side, color, RF_DEPTHHACK );
			}
		}
	}

	// the map's, nearest first while there are lines to spare
	for ( int i = 0; i < me.numEnts; i++ ) {
		const meEnt_t *e = &me.ents[i];
		vec3_t center;
		float dist;

		if ( !ME_EntDrawn( e ) )
			continue;
		ME_EntCenter( e, center );
		dist = Distance( center, me.camOrg );
		if ( e->bmodel ) {
			// a big box is seen from further
			vec3_t size;

			VectorSubtract( e->maxs, e->mins, size );
			dist = Q_max( 0.0f, dist - VectorLength( size ) * 0.5f );
		}
		if ( dist > ME_DRAW_DIST && !ME_IsSel( ME_SEL_ENT, i ) )
			continue;
		near_[numNear].index = i;
		near_[numNear].dist = ME_IsSel( ME_SEL_ENT, i ) ? -1.0f : dist;
		numNear++;
	}
	qsort( near_, numNear, sizeof( near_[0] ), ME_CompareNear );
	for ( int n = 0; n < numNear && me.numLines < ME_MAX_LINES; n++ ) {
		const int i = near_[n].index;
		const meEnt_t *e = &me.ents[i];
		const qboolean mark = (qboolean)( ME_IsSel( ME_SEL_ENT, i ) || ME_IsHover( ME_SEL_ENT, i ) );
		const byte *color = mark ? meSelColor : ME_LocksTab() && ME_FindLock( e ) ? meLockedColor : e->hidden ? meHiddenColor
			: e->kind == ME_ENT_TRIGGER ? meTriggerColor : e->kind == ME_ENT_BRUSH ? meBrushColor : mePointColor;

		if ( e->bmodel )
			ME_BoxLines( e->mins, e->maxs, color );
		else
			ME_Cross( e->origin, ME_POINT_SIZE, color );
	}
}

// the cgame's main view, before it goes to the renderer: our things are added to it
void CL_MapEditor_AddToScene( const refdef_t *fd ) {
	if ( fd->rdflags & ( RDF_NOWORLDMODEL | RDF_SKYBOXPORTAL | RDF_AUTOMAP ) )
		return;
	if ( !me.open ) {
		me.havePlayerView = qtrue;
		VectorCopy( fd->vieworg, me.playerOrg );
		VectorCopy( fd->viewangles, me.playerAng );
	}
	if ( !ME_Shown() || !me.map[0] || Q_stricmp( me.map, cl.mapname ) )
		return;
	ME_AddItems();
	if ( me.open )
		ME_AddEditorLines();
}

// the editor's camera in place of the player's. qtrue if drawn here or left out
qboolean CL_MapEditor_RenderScene( const refdef_t *fd ) {
	refdef_t view;

	if ( !me.open )
		return qfalse;
	// the HUD's 3D bits, heads and items
	if ( fd->rdflags & RDF_NOWORLDMODEL )
		return qtrue;
	if ( fd->rdflags & RDF_AUTOMAP )
		return qfalse;

	view = *fd;
	view.fov_x = me.camFov;
	view.fov_y = RAD2DEG( 2.0f * atanf( tanf( DEG2RAD( me.camFov * 0.5f ) ) * fd->height / (float)fd->width ) );
	VectorCopy( me.camAng, view.viewangles );
	AnglesToAxis( me.camAng, view.viewaxis );
	// the sky portal keeps its own origin but looks the way the camera does
	if ( !( fd->rdflags & RDF_SKYBOXPORTAL ) ) {
		VectorCopy( me.camOrg, view.vieworg );
		view.viewContents = CM_PointContents( me.camOrg, 0 );
		// the snapshot's area mask is from the player's position, the camera can be anywhere
		memset( view.areamask, 0, sizeof( view.areamask ) );
		me.haveView = qtrue;
		VectorCopy( view.vieworg, me.viewOrg );
		VectorCopy( view.viewaxis[0], me.viewAxis[0] );
		VectorCopy( view.viewaxis[1], me.viewAxis[1] );
		VectorCopy( view.viewaxis[2], me.viewAxis[2] );
		me.viewFovX = view.fov_x;
		me.viewFovY = view.fov_y;
	}
	re->RenderScene( &view );
	return qtrue;
}

// the camera, for the effects system to cull against
qboolean CL_MapEditor_Camera( vec3_t origin, vec3_t angles ) {
	if ( !me.open )
		return qfalse;
	VectorCopy( me.camOrg, origin );
	VectorCopy( me.camAng, angles );
	return qtrue;
}

// what the cgame adds that's left out: a hidden brush entity, and the view weapon
// floating where the player stands while the camera is elsewhere
qboolean CL_MapEditor_FilterEntity( const refEntity_t *ent ) {
	if ( me.open && ( ent->renderfx & RF_FIRST_PERSON ) )
		return qtrue;
	if ( ME_Shown() && ent->reType == RT_MODEL ) {
		for ( int i = 0; i < me.numHidden; i++ ) {
			if ( me.hidden[i] && ent->hModel == me.hidden[i] )
				return qtrue;
		}
	}
	return qfalse;
}

// the cgame's HUD is left out while editing, but what covers the whole screen still shows
qboolean CL_MapEditor_Hide2D( float x, float y, float w, float h ) {
	if ( !me.open )
		return qfalse;
	return (qboolean)!( x <= 0.0f && y <= 0.0f && x + w >= SCREEN_WIDTH && y + h >= SCREEN_HEIGHT );
}

/*
===============================================================================

The panel

===============================================================================
*/

static void ME_Fill( float x, float y, float w, float h, const float *color ) {
	re->SetColor( color );
	re->DrawStretchPic( x, y, w, h, 0, 0, 0, 0, cls.whiteShader );
	re->SetColor( NULL );
}

static void ME_Box( float x, float y, float w, float h, const float *fill ) {
	ME_Fill( x, y, w, h, fill );
	ME_Fill( x, y, w, 1, meBorder );
	ME_Fill( x, y + h - 1, w, 1, meBorder );
	ME_Fill( x, y, 1, h, meBorder );
	ME_Fill( x + w - 1, y, 1, h, meBorder );
}

static float ME_TextWidth( const char *text ) {
	return (float)re->Font_StrLenPixels( text, me.font, ME_TEXT_SCALE );
}

static void ME_Text( float x, float y, const char *text, const float *color ) {
	re->Font_DrawString( (int)x, (int)y, text, color, me.font | STYLE_DROPSHADOW, -1, ME_TEXT_SCALE );
}

static void ME_TextClipped( float x, float y, float w, const char *text, const float *color ) {
	char buf[MAX_STRING_CHARS];
	int len;

	Q_strncpyz( buf, text, sizeof( buf ) );
	len = strlen( buf );
	while ( len > 3 && ME_TextWidth( buf ) > w ) {
		len--;
		buf[len - 3] = '.';
		buf[len - 2] = '.';
		buf[len - 1] = '.';
		buf[len] = '\0';
	}
	ME_Text( x, y, buf, color );
}

// the end of a long name, which is the part that tells it apart
static void ME_TextTail( float x, float y, float w, const char *text, const float *color ) {
	const char *p = text;

	while ( *p && ME_TextWidth( p ) > w )
		p++;
	if ( p != text && p[0] && p[1] && p[2] )
		ME_Text( x, y, va( "...%s", p + 3 ), color );
	else
		ME_Text( x, y, p, color );
}

static qboolean ME_InRect( float x, float y, float w, float h ) {
	return (qboolean)( !me.flying && me.cursorX >= x && me.cursorX < x + w && me.cursorY >= y && me.cursorY < y + h );
}

// eats this frame's click if it landed in the rect
static qboolean ME_Clicked( float x, float y, float w, float h ) {
	if ( !me.click || me.clickX < x || me.clickX >= x + w || me.clickY < y || me.clickY >= y + h )
		return qfalse;
	me.click = qfalse;
	return qtrue;
}

static qboolean ME_Button( float x, float y, float w, const char *label, qboolean selected, qboolean enabled ) {
	const float *fill = selected ? meHighlight : ( enabled && ME_InRect( x, y, w, ME_CTRL_H ) ) ? meHoverBox : mePanelLight;

	ME_Box( x, y, w, ME_CTRL_H, fill );
	ME_Text( x + ( w - ME_TextWidth( label ) ) * 0.5f, y + 2, label, enabled ? meWhite : meDim );
	return (qboolean)( ME_Clicked( x, y, w, ME_CTRL_H ) && enabled );
}

// a value with - and + either side, the wheel over it too; how many steps it went
static int ME_Stepper( float x, float y, float w, const char *label, const char *value ) {
	const float lw = 62.0f, vx = x + lw + 17, vw = w - lw - 34;
	int steps = 0;

	ME_Text( x, y + 2, label, meDim );
	if ( ME_Button( x + lw, y, 16, "-", qfalse, qtrue ) )
		steps--;
	ME_Box( vx, y, vw, ME_CTRL_H, mePanelLight );
	ME_Text( vx + ( vw - ME_TextWidth( value ) ) * 0.5f, y + 2, value, meWhite );
	if ( ME_Button( x + w - 16, y, 16, "+", qfalse, qtrue ) )
		steps++;
	if ( me.wheel && ME_InRect( x, y, w, ME_CTRL_H ) ) {
		steps -= me.wheel;
		me.wheel = 0;
	}
	return steps;
}

// Shift goes in big steps, Ctrl in small ones
static float ME_Step( float step ) {
	if ( ME_ShiftDown() )
		return step * 5.0f;
	if ( ME_CtrlDown() )
		return step * 0.2f;
	return step;
}

static void ME_Adjust( float *value, int steps, float step, float min, float max ) {
	if ( !steps )
		return;
	*value = Com_Clamp( min, max, *value + steps * ME_Step( step ) );
	ME_Changed();
}

static void ME_AdjustAngle( float *value, int steps, float step ) {
	if ( !steps )
		return;
	*value = AngleNormalize360( *value + steps * ME_Step( step ) );
	ME_Changed();
}

static void ME_FieldStyle( wgStyle_t *style ) {
	style->font = me.font;
	style->scale = ME_TEXT_SCALE;
	style->lineH = ME_ROW_H;
	style->padX = 4.0f;
	style->padY = 2.0f;
	style->fill = meFocus;
	style->border = meBorder;
	style->text = meWhite;
	style->dim = meDim;
}

// a text box; a click on it starts typing, the cursor where it was clicked. Typed in and long,
// it grows down over what's below
static void ME_TextBox( float x, float y, float w, int field, char *text, const char *empty ) {
	const qboolean typing = (qboolean)( me.field == field );
	wgStyle_t style;

	ME_FieldStyle( &style );
	ME_Box( x, y, w, ME_CTRL_H, typing ? meFocus : mePanelLight );
	WG_Field( &style, x, y, w, ME_CTRL_H, text, typing, empty, me.cursorX, me.cursorY );
	if ( ME_Clicked( x, y, w, WG_FieldHeight( &style, w, ME_CTRL_H, text, typing ) ) ) {
		me.field = field;
		WG_FieldClick( text, me.clickX, me.clickY );
	}
}

// a scrolling list, with a scrollbar at its side when it's longer than shown; returns the row
// clicked, -1 none. *rowW is the rows' width, less the scrollbar's
static int ME_ListRows( float x, float y, float w, int rows, int count, int *scroll, float *rowW ) {
	if ( me.wheel && ME_InRect( x, y, w, rows * ME_ROW_H ) ) {
		*scroll += me.wheel;
		me.wheel = 0;
	}
	*scroll = Com_Clampi( 0, Q_max( 0, count - rows ), *scroll );
	ME_Box( x, y, w, rows * ME_ROW_H + 2, mePanelLight );
	if ( WG_Scrollbar( x + w - WG_SCROLL_W - 1, y + 1, rows * ME_ROW_H, count, rows, scroll, me.cursorX, me.cursorY,
		&me.click, me.clickX, me.clickY ) )
		w -= WG_SCROLL_W + 1;
	*rowW = w;
	for ( int i = 0; i < rows && *scroll + i < count; i++ ) {
		const float ry = y + 1 + i * ME_ROW_H;

		if ( ME_InRect( x, ry, w, ME_ROW_H ) )
			ME_Fill( x + 1, ry, w - 2, ME_ROW_H, meHoverBox );
	}
	if ( me.click && me.clickX >= x && me.clickX < x + w && me.clickY >= y + 1 && me.clickY < y + 1 + rows * ME_ROW_H ) {
		const int row = (int)( ( me.clickY - y - 1 ) / ME_ROW_H );

		if ( *scroll + row < count ) {
			me.click = qfalse;
			return *scroll + row;
		}
	}
	return -1;
}

static void ME_RowHighlight( float x, float y, float w, int row, int scroll, int rows ) {
	if ( row >= scroll && row < scroll + rows )
		ME_Fill( x + 1, y + 1 + ( row - scroll ) * ME_ROW_H, w - 2, ME_ROW_H, meHighlight );
}

static const char *ME_EntName( const meEnt_t *e ) {
	if ( e->targetname[0] )
		return va( "%s " S_COLOR_GREY "%s", e->classname, e->targetname );
	return e->classname[0] ? e->classname : "(no class)";
}

static void ME_DrawLockRow( float x, float y, float w, const meEnt_t *e ) {
	const meLock_t *l = ME_FindLock( e );
	const int num = ME_EntNumber( e );
	const char *right = !l ? S_COLOR_GREY "-" : l->rank < 0 ? S_COLOR_RED "no one" : va( S_COLOR_YELLOW "rank %i", l->rank );
	const float rw = ME_TextWidth( right );

	ME_TextClipped( x, y, w - rw - 6, va( "%s%s " S_COLOR_WHITE "%s", ME_IsDoor( e ) ? S_COLOR_CYAN : S_COLOR_ORANGE, ME_IsDoor( e ) ? "door" : "trigger",
		e->targetname[0] ? e->targetname : num >= 0 ? va( S_COLOR_GREY "#%i", num ) : S_COLOR_GREY "(no name)" ), meWhite );
	ME_Text( x + w - rw, y, right, meWhite );
}

static void ME_DrawLocksTab( float x, float y, float w ) {
	const float third = ( w - 8 ) / 3.0f, half = ( w - 4 ) * 0.5f;
	const int rows = 12;
	static int matches[ME_MAX_ENTS];
	int numMatches = 0, doors = 0, triggers = 0, locked = 0, clicked, selRow = -1;
	meEnt_t *e;

	if ( !ml.haveList && !ml.askTime && !ml.numQueue )
		ME_LockSend( "rplock list" );
	if ( ME_Button( x, y, third, "Refresh", qfalse, qtrue ) )
		ME_LockSend( "rplock list" );
	// the server tells the numbers and names of what's used
	if ( ME_Button( x + third + 4, y, third, "Trace on", qfalse, qtrue ) )
		ME_LockSend( "rplock trace on" );
	if ( ME_Button( x + ( third + 4 ) * 2, y, third, "Trace off", qfalse, qtrue ) )
		ME_LockSend( "rplock trace off" );
	y += ME_CTRL_H + ME_GAP;

	for ( int i = 0; i < me.numEnts; i++ ) {
		const meEnt_t *ent = &me.ents[i];

		if ( !ME_Lockable( ent ) )
			continue;
		if ( ME_IsDoor( ent ) )
			doors++;
		else
			triggers++;
		if ( ME_FindLock( ent ) )
			locked++;
		if ( !ME_EntDrawn( ent ) || !ME_EntMatches( ent ) || ( ml.onlyLocked && !ME_FindLock( ent ) ) )
			continue;
		if ( ME_IsSel( ME_SEL_ENT, i ) )
			selRow = numMatches;
		matches[numMatches++] = i;
	}
	if ( ME_Button( x, y, third, va( "Doors (%i)", doors ), ml.showDoors, qtrue ) )
		ml.showDoors = (qboolean)!ml.showDoors;
	if ( ME_Button( x + third + 4, y, third, va( "Triggers (%i)", triggers ), ml.showTriggers, qtrue ) )
		ml.showTriggers = (qboolean)!ml.showTriggers;
	if ( ME_Button( x + ( third + 4 ) * 2, y, third, va( "Locked (%i)", locked ), ml.onlyLocked, qtrue ) )
		ml.onlyLocked = (qboolean)!ml.onlyLocked;
	y += ME_CTRL_H + ME_GAP;
	ME_TextBox( x, y, w, ME_FIELD_FILTER, me.filter, "type to find a class or name" );
	y += ME_CTRL_H + ME_GAP;

	float rowW;

	clicked = ME_ListRows( x, y, w, rows, numMatches, &ml.scroll, &rowW );
	if ( selRow >= 0 )
		ME_RowHighlight( x, y, rowW, selRow, ml.scroll, rows );
	for ( int r = 0; r < rows && ml.scroll + r < numMatches; r++ )
		ME_DrawLockRow( x + 4, y + 1 + r * ME_ROW_H, rowW - 8, &me.ents[matches[ml.scroll + r]] );
	if ( !numMatches )
		ME_Text( x + 4, y + 1, S_COLOR_GREY "No doors or triggers to lock", meWhite );
	if ( clicked >= 0 ) {
		ME_Select( ME_SEL_ENT, matches[clicked] );
		ml.confirmTime = 0;
	}
	y += rows * ME_ROW_H + 8;

	e = ME_SelectedEnt();
	if ( !e || !ME_Lockable( e ) ) {
		ME_TextClipped( x, y, w, S_COLOR_GREY "Click a door or a trigger, in the world or the list", meWhite );
		ME_TextClipped( x, y + ME_ROW_H, w, S_COLOR_GREY "RPMod locks func_door and trigger_multiple", meWhite );
	} else {
		const meLock_t *l = ME_FindLock( e );
		const char *target = ME_LockTarget( e );
		qboolean allLockable;
		const int alike = ME_NamedAlike( e, &allLockable );
		vec3_t center, size;
		int steps;

		ME_TextClipped( x, y, w, va( "%s " S_COLOR_GREY "%s", e->classname, !target ? "number not known: Trace on, then use it"
			: alike > 1 && allLockable ? va( "%s, with the %i named alike", target, alike - 1 ) : Q_isanumber( target ) ? va( "entity %s", target ) : target ), meWhite );
		y += ME_ROW_H;
		if ( l )
			ME_TextClipped( x, y, w, va( S_COLOR_YELLOW "Locked" S_COLOR_WHITE ": %s%s", ME_RankLabel( l->rank, l->rankName ), l->name[0] ? va( ", \"%s" S_COLOR_WHITE "\"", l->name ) : "" ), meWhite );
		else
			ME_TextClipped( x, y, w, ml.haveList ? S_COLOR_GREEN "Not locked" : S_COLOR_GREY "Asking the server...", meWhite );
		y += ME_ROW_H + 2;

		steps = ME_Stepper( x, y, w, "Rank", ml.rank < 0 ? "-1: no one" : ml.rank ? va( "%i", ml.rank ) : "0: no rank" );
		if ( steps )
			ml.rank = Com_Clampi( -1, 15, ml.rank + steps );
		y += ME_CTRL_H + ME_GAP;
		ME_TextBox( x, y, w, ME_FIELD_LOCKNAME, ml.name, "lock name, shown when it's refused" );
		y += ME_CTRL_H + ME_GAP;
		if ( ME_Button( x, y, third, l ? "Change lock" : "Lock", qfalse, (qboolean)( target != NULL ) ) )
			ME_LockSend( va( "rplock add %s %i%s%s", target, ml.rank, ml.name[0] ? " " : "", ml.name ) );
		if ( ME_Button( x + third + 4, y, third, "Unlock", qfalse, (qboolean)( target != NULL && ( l || !ml.haveList ) ) ) )
			ME_LockSend( va( "rplock remove %s", target ) );
		ME_EntCenter( e, center );
		VectorSubtract( e->maxs, e->mins, size );
		if ( ME_Button( x + ( third + 4 ) * 2, y, third, "Go to it", qfalse, qtrue ) )
			ME_GoTo( center, VectorLength( size ) * 0.5f );
		y += ME_CTRL_H + ME_GAP;
		if ( target )
			ME_TextClipped( x, y, w, va( S_COLOR_GREY "rplock add %s %i%s%s", target, ml.rank, ml.name[0] ? " " : "", ml.name ), meWhite );
	}
	y += ME_ROW_H + 6;

	// every lock in the map, a click to arm it and one to do it
	{
		const qboolean armed = (qboolean)( ml.confirmTime && cls.realtime - ml.confirmTime < 3000 );

		if ( ME_Button( x, y, w, armed ? "Click again to remove every lock" : "Remove every lock in the map", armed, (qboolean)( ml.numLocks > 0 || !ml.haveList ) ) ) {
			if ( armed ) {
				ml.confirmTime = 0;
				ME_LockSend( "rplock remove all" );
			} else {
				ml.confirmTime = cls.realtime;
			}
		}
		y += ME_CTRL_H + ME_GAP;
	}
	if ( ml.numQueue )
		ME_TextClipped( x, y, w, va( S_COLOR_YELLOW "Sending, %i to go", ml.numQueue ), meWhite );
	else if ( ml.reply[0] && cls.realtime - ml.replyTime < 8000 )
		ME_TextClipped( x, y, w, va( S_COLOR_YELLOW "Server: " S_COLOR_WHITE "%s", ml.reply ), meWhite );
	else
		ME_TextClipped( x, y, w, S_COLOR_GREY "RPMod's rp_locks has to be 2 to add or remove", meWhite );
}

static void ME_DrawMapTab( float x, float y, float w ) {
	const float third = ( w - 8 ) / 3.0f;
	const int rows = 11, kvRows = 12;
	static int matches[ME_MAX_ENTS];
	meEnt_t *e;
	int numMatches = 0, clicked, selRow = -1;

	if ( ME_Button( x, y, third, "Triggers", me.showTriggers, qtrue ) )
		me.showTriggers = (qboolean)!me.showTriggers;
	if ( ME_Button( x + third + 4, y, third, "Brushes", me.showBrushes, qtrue ) )
		me.showBrushes = (qboolean)!me.showBrushes;
	if ( ME_Button( x + ( third + 4 ) * 2, y, third, "Points", me.showPoints, qtrue ) )
		me.showPoints = (qboolean)!me.showPoints;
	y += ME_CTRL_H + ME_GAP;
	ME_TextBox( x, y, w, ME_FIELD_FILTER, me.filter, "type to find a class or name" );
	y += ME_CTRL_H + ME_GAP;

	for ( int i = 0; i < me.numEnts; i++ ) {
		if ( ME_EntMatches( &me.ents[i] ) ) {
			if ( ME_IsSel( ME_SEL_ENT, i ) )
				selRow = numMatches;
			matches[numMatches++] = i;
		}
	}
	ME_Text( x, y, va( "Entities (%i of %i)", numMatches, me.numEnts ), meWhite );
	y += ME_ROW_H + 2;
	float rowW;

	clicked = ME_ListRows( x, y, w, rows, numMatches, &me.entScroll, &rowW );
	if ( selRow >= 0 )
		ME_RowHighlight( x, y, rowW, selRow, me.entScroll, rows );
	for ( int r = 0; r < rows && me.entScroll + r < numMatches; r++ ) {
		const meEnt_t *row = &me.ents[matches[me.entScroll + r]];

		ME_TextClipped( x + 4, y + 1 + r * ME_ROW_H, rowW - 8, va( "%s%i " S_COLOR_WHITE "%s",
			row->kind == ME_ENT_TRIGGER ? S_COLOR_ORANGE : row->kind == ME_ENT_BRUSH ? S_COLOR_CYAN : S_COLOR_GREEN,
			matches[me.entScroll + r], ME_EntName( row ) ), meWhite );
	}
	if ( clicked >= 0 )
		ME_Select( ME_SEL_ENT, matches[clicked] );
	y += rows * ME_ROW_H + 8;

	e = ME_SelectedEnt();
	if ( !e ) {
		ME_Text( x, y, S_COLOR_GREY "Click one in the world or the list", meWhite );
		return;
	}
	{
		const float half = ( w - 4 ) * 0.5f;
		vec3_t center, size;

		ME_EntCenter( e, center );
		VectorSubtract( e->maxs, e->mins, size );
		if ( ME_Button( x, y, half, "Go to it", qfalse, (qboolean)( e->bmodel || e->hasOrigin ) ) )
			ME_GoTo( center, e->bmodel ? VectorLength( size ) * 0.5f : 0.0f );
		if ( ME_Button( x + half + 4, y, half, e->hidden ? "Hidden here" : "Hide here", e->hidden, (qboolean)( e->bmodel != 0 ) ) ) {
			e->hidden = (qboolean)!e->hidden;
			ME_RebuildHidden();
			ME_Changed();
		}
		y += ME_CTRL_H + ME_GAP;
	}
	if ( me.kvEnt != me.sel )
		ME_ReadKeys( me.sel );
	float kvW;

	clicked = ME_ListRows( x, y, w, kvRows, me.numKv, &me.kvScroll, &kvW );
	for ( int r = 0; r < kvRows && me.kvScroll + r < me.numKv; r++ ) {
		const float ry = y + 1 + r * ME_ROW_H;

		ME_TextClipped( x + 4, ry, 70, me.kv[me.kvScroll + r][0], meDim );
		ME_TextClipped( x + 78, ry, kvW - 82, me.kv[me.kvScroll + r][1], meWhite );
	}
	// a click on a target finds what it targets
	if ( clicked >= 0 && ( !Q_stricmp( me.kv[clicked][0], "target" ) || !Q_stricmp( me.kv[clicked][0], "target2" ) ) ) {
		for ( int i = 0; i < me.numEnts; i++ ) {
			if ( me.ents[i].targetname[0] && !Q_stricmp( me.ents[i].targetname, me.kv[clicked][1] ) ) {
				ME_Select( ME_SEL_ENT, i );
				ME_ScrollToSelection();
				break;
			}
		}
	}
	y += kvRows * ME_ROW_H + 4;
	ME_Text( x, y, S_COLOR_GREY "The server runs these; a click on a target finds it", meWhite );
}

// the list of ours of a kind, and add, copy, delete; the item chosen, NULL none
static meItem_t *ME_DrawItemList( float *py, float x, float w, int kind, const char *addLabel ) {
	const int tab = ME_ItemTab( kind ), rows = 6;
	const float third = ( w - 8 ) / 3.0f;
	static int matches[ME_MAX_ITEMS];
	int numMatches = 0, clicked, selRow = -1;
	float y = *py;
	meItem_t *it;

	for ( int i = 0; i < me.numItems; i++ ) {
		if ( me.items[i].kind == kind || ( kind == ME_ITEM_MODEL && me.items[i].kind == ME_ITEM_PROP ) ) {
			if ( ME_IsSel( ME_SEL_ITEM, i ) )
				selRow = numMatches;
			matches[numMatches++] = i;
		}
	}
	float rowW;

	clicked = ME_ListRows( x, y, w, rows, numMatches, &me.itemScroll[tab], &rowW );
	if ( selRow >= 0 )
		ME_RowHighlight( x, y, rowW, selRow, me.itemScroll[tab], rows );
	for ( int r = 0; r < rows && me.itemScroll[tab] + r < numMatches; r++ ) {
		const meItem_t *row = &me.items[matches[me.itemScroll[tab] + r]];
		const float ry = y + 1 + r * ME_ROW_H;
		const char *text;

		if ( row->kind == ME_ITEM_PROP )
			text = va( "%i  map %s%s", r + me.itemScroll[tab] + 1, ME_ItemIsBrush( row ) ? "brush" : "prop", row->removed ? S_COLOR_GREY " (hidden)" : "" );
		else if ( kind == ME_ITEM_MODEL )
			text = va( "%i  %s", r + me.itemScroll[tab] + 1, COM_SkipPath( (char *)row->model ) );
		else if ( kind == ME_ITEM_LIGHT )
			text = va( "%i  reach %i%s", r + me.itemScroll[tab] + 1, ME_Round( row->radius ), row->style >= 0 ? va( ", style %i", row->style ) : "" );
		else if ( kind == ME_ITEM_FOG )
			text = va( "%i  fog volume" S_COLOR_GREY "  %i x %i x %i, opaque at %i", r + me.itemScroll[tab] + 1, ME_Round( row->size[0] * 2 ),
				ME_Round( row->size[1] * 2 ), ME_Round( row->size[2] * 2 ), ME_Round( row->fogDepth ) );
		else if ( kind == ME_ITEM_SPEAKER )
			text = va( "%i  %s" S_COLOR_GREY "  %s%s", r + me.itemScroll[tab] + 1, row->noise[0] ? COM_SkipPath( (char *)row->noise ) : S_COLOR_YELLOW "no sound",
				row->global ? "everywhere" : "here", row->off ? ", stopped" : "" );
		else
			text = row->use ? va( "%i  " S_COLOR_CYAN "use " S_COLOR_WHITE "%s" S_COLOR_GREY "  %s", r + me.itemScroll[tab] + 1,
				row->name[0] ? row->name : S_COLOR_YELLOW "no name", row->display )
				: va( "%i  %s%s", r + me.itemScroll[tab] + 1, row->name[0] ? va( "%s: ", row->name ) : "", row->command[0] ? row->command : S_COLOR_GREY "no command" );
		ME_TextClipped( x + 4, ry, rowW - 8, text, meWhite );
	}
	if ( !numMatches )
		ME_Text( x + 4, y + 2, S_COLOR_GREY "None yet", meWhite );
	if ( clicked >= 0 )
		ME_Select( ME_SEL_ITEM, matches[clicked] );
	y += rows * ME_ROW_H + 6;

	it = ME_SelectedItem();
	if ( it && it->kind != kind && !( kind == ME_ITEM_MODEL && it->kind == ME_ITEM_PROP ) )
		it = NULL;
	if ( ME_Button( x, y, third, addLabel, qfalse, qtrue ) ) {
		ME_AddItem( kind );
		it = ME_SelectedItem();
	}
	if ( ME_Button( x + third + 4, y, third, "Copy", qfalse, (qboolean)( it != NULL ) ) ) {
		ME_CopySelected();
		it = ME_SelectedItem();
	}
	// a map prop goes back where it was
	if ( ME_Button( x + ( third + 4 ) * 2, y, third, it && it->kind == ME_ITEM_PROP ? "Put back" : "Delete", qfalse, (qboolean)( it != NULL ) ) ) {
		ME_DeleteSelected();
		it = NULL;
	}
	y += ME_CTRL_H + ME_GAP;
	if ( it ) {
		const float half = ( w - 4 ) * 0.5f;
		vec3_t mins, maxs, size;

		if ( ME_Button( x, y, half, me.grabbing ? "Carrying (G)" : "Move (G)", me.grabbing, qtrue ) ) {
			ME_Grab();
			if ( !me.grabbing )
				ME_Changed();
		}
		ME_ItemBounds( it, mins, maxs );
		VectorSubtract( maxs, mins, size );
		if ( ME_Button( x + half + 4, y, half, "Go to it", qfalse, qtrue ) )
			ME_GoTo( it->org, VectorLength( size ) * 0.5f );
		y += ME_CTRL_H + ME_GAP;
		{
			const int steps = ME_Stepper( x, y, w, "Height", va( "%i", ME_Round( it->org[2] ) ) );

			ME_Adjust( &it->org[2], steps, 4.0f, -65536.0f, 65536.0f );
		}
		y += ME_CTRL_H + ME_GAP;
	}
	*py = y;
	return it;
}

// a prop of the map's, chosen with a click: lift it out, or take it away
static void ME_DrawPropChoice( float *py, float x, float w ) {
	const float third = ( w - 8 ) / 3.0f;
	vec3_t size;
	float y = *py;

	VectorSubtract( me.candMaxs, me.candMins, size );
	ME_Text( x, y, va( "%s" S_COLOR_GREY "   %i triangles, %i x %i x %i", ME_CandIsBrush() ? "Map brush" : "Map prop", me.candTris,
		ME_Round( size[0] ), ME_Round( size[1] ), ME_Round( size[2] ) ), meWhite );
	y += ME_ROW_H + 2;
	if ( ME_Button( x, y, third, "Lift out (G)", qfalse, qtrue ) )
		ME_LiftProp( qfalse, qfalse );
	if ( ME_Button( x + third + 4, y, third, "Hide (Del)", qfalse, qtrue ) )
		ME_LiftProp( qtrue, qfalse );
	if ( ME_Button( x + ( third + 4 ) * 2, y, third, "Take more", qfalse, (qboolean)( me.numCandSeeds < ME_MAX_SEEDS ) ) )
		ME_TakeTouching();
	y += ME_CTRL_H + ME_GAP;
	// a prop made of brushes: just the one clicked
	if ( !ME_CandIsBrush() && me.candBrush >= 0 ) {
		if ( ME_Button( x, y, w, "Just the brush clicked", qfalse, qtrue ) )
			ME_ChooseBrush( me.candBrush );
		y += ME_CTRL_H + ME_GAP;
	}
	ME_Text( x, y, ME_CandIsBrush() ? S_COLOR_GREY "One brush of the map's: the faces on it" : S_COLOR_GREY "Take more adds loose pieces inside it", meWhite );
	y += ME_ROW_H + 6;
	*py = y;
}

static void ME_DrawModelsTab( float x, float y, float w ) {
	meItem_t *it;
	const int rows = 9;
	static int matches[ME_MAX_NAMES];
	int numMatches = 0, clicked;
	char lower[sizeof( me.search )];

	if ( me.selKind == ME_SEL_PROP )
		ME_DrawPropChoice( &y, x, w );
	it = ME_DrawItemList( &y, x, w, ME_ITEM_MODEL, "Add" );
	if ( it && it->kind == ME_ITEM_PROP ) {
		const float half = ( w - 4 ) * 0.5f;

		if ( ME_Button( x, y, half, it->removed ? "Hidden" : "Shown", (qboolean)!it->removed, qtrue ) ) {
			it->removed = (qboolean)!it->removed;
			ME_Changed();
		}
		if ( ME_Button( x + half + 4, y, half, "Take more", qfalse, (qboolean)( it->numSeeds < ME_MAX_SEEDS ) ) )
			ME_TakeTouching();
		y += ME_CTRL_H + ME_GAP;
	}

	if ( it ) {
		vec3_t pivot, pivotAt;
		const qboolean isMap = ME_MapPivot( it, pivot );
		int steps;

		if ( isMap )
			ME_ItemPoint( it, pivot, pivotAt );
		steps = ME_Stepper( x, y, w, "Yaw", va( "%i", ME_Round( it->ang[YAW] ) ) );
		ME_AdjustAngle( &it->ang[YAW], steps, 15.0f );
		y += ME_CTRL_H + ME_GAP;
		steps = ME_Stepper( x, y, w, "Pitch", va( "%i", ME_Round( AngleNormalize180( it->ang[PITCH] ) ) ) );
		ME_AdjustAngle( &it->ang[PITCH], steps, 15.0f );
		y += ME_CTRL_H + ME_GAP;
		steps = ME_Stepper( x, y, w, "Roll", va( "%i", ME_Round( AngleNormalize180( it->ang[ROLL] ) ) ) );
		ME_AdjustAngle( &it->ang[ROLL], steps, 15.0f );
		y += ME_CTRL_H + ME_GAP;
		steps = ME_Stepper( x, y, w, "Scale", va( "%.2f", it->scale ) );
		ME_Adjust( &it->scale, steps, 0.1f, 0.05f, 50.0f );
		y += ME_CTRL_H + ME_GAP + 4;
		// a map turned or scaled stays where it was put
		if ( isMap )
			ME_PutPointAt( it, pivot, pivotAt );
	} else {
		y += 4;
	}

	// the models there are
	ME_TextBox( x, y, w, ME_FIELD_SEARCH, me.search, "type to find a model" );
	y += ME_CTRL_H + ME_GAP;
	Q_strncpyz( lower, me.search, sizeof( lower ) );
	Q_strlwr( lower );
	for ( int i = 0; i < me.numNames; i++ ) {
		if ( !lower[0] || Q_stristr( ME_ShownName( me.names[i] ), lower ) )
			matches[numMatches++] = i;
	}
	float rowW;

	clicked = ME_ListRows( x, y, w, rows, numMatches, &me.searchScroll, &rowW );
	for ( int r = 0; r < rows && me.searchScroll + r < numMatches; r++ ) {
		const char *name = me.names[matches[me.searchScroll + r]];

		if ( !Q_stricmp( name, me.pickModel ) )
			ME_Fill( x + 1, y + 1 + r * ME_ROW_H, rowW - 2, ME_ROW_H, meHighlight );
		ME_TextTail( x + 4, y + 1 + r * ME_ROW_H, rowW - 8, ME_ShownName( name ), meWhite );
	}
	if ( clicked >= 0 ) {
		Q_strncpyz( me.pickModel, me.names[matches[clicked]], sizeof( me.pickModel ) );
		// the one chosen takes it
		if ( it && it->kind == ME_ITEM_MODEL ) {
			Q_strncpyz( it->model, me.pickModel, sizeof( it->model ) );
			it->registered = qfalse;
			ME_Changed();
		}
	}
	y += rows * ME_ROW_H + 4;
	ME_TextTail( x, y, w, me.pickModel[0] ? va( "Adds %s", ME_ShownName( me.pickModel ) ) : "Pick a model or a map (type map), then Add puts it at the crosshair", meDim );
}

static void ME_DrawLightsTab( float x, float y, float w ) {
	meItem_t *it = ME_DrawItemList( &y, x, w, ME_ITEM_LIGHT, "Add" );
	const float sw = ( w - 4.0f * ( ARRAY_LEN( meLightColors ) - 1 ) ) / ARRAY_LEN( meLightColors );
	int steps;

	if ( !it ) {
		ME_Text( x, y + 4, S_COLOR_GREY "Add puts a light at the crosshair", meWhite );
		ME_Text( x, y + 4 + ME_ROW_H, S_COLOR_GREY "Only dynamic lights can be added at runtime", meWhite );
		return;
	}
	steps = ME_Stepper( x, y, w, "Reach", va( "%i", ME_Round( it->radius ) ) );
	ME_Adjust( &it->radius, steps, 25.0f, 8.0f, 2000.0f );
	y += ME_CTRL_H + ME_GAP;
	steps = ME_Stepper( x, y, w, "Red", va( "%i", ME_Round( it->color[0] * 255 ) ) );
	ME_Adjust( &it->color[0], steps, 0.1f, 0.0f, 1.0f );
	y += ME_CTRL_H + ME_GAP;
	steps = ME_Stepper( x, y, w, "Green", va( "%i", ME_Round( it->color[1] * 255 ) ) );
	ME_Adjust( &it->color[1], steps, 0.1f, 0.0f, 1.0f );
	y += ME_CTRL_H + ME_GAP;
	steps = ME_Stepper( x, y, w, "Blue", va( "%i", ME_Round( it->color[2] * 255 ) ) );
	ME_Adjust( &it->color[2], steps, 0.1f, 0.0f, 1.0f );
	y += ME_CTRL_H + ME_GAP;
	// the colours most lights are
	for ( int i = 0; i < (int)ARRAY_LEN( meLightColors ); i++ ) {
		const float sx = x + i * ( sw + 4 );
		vec4_t c;

		VectorCopy( meLightColors[i], c );
		c[3] = 1.0f;
		ME_Box( sx, y, sw, ME_CTRL_H, ME_InRect( sx, y, sw, ME_CTRL_H ) ? meHoverBox : mePanelLight );
		ME_Fill( sx + 3, y + 3, sw - 6, ME_CTRL_H - 6, c );
		if ( ME_Clicked( sx, y, sw, ME_CTRL_H ) ) {
			VectorCopy( meLightColors[i], it->color );
			ME_Changed();
		}
	}
	y += ME_CTRL_H + ME_GAP;
	steps = ME_Stepper( x, y, w, "Style", it->style >= 0 ? va( "%i", it->style ) : "none" );
	if ( steps ) {
		it->style = Com_Clampi( -1, MAX_LIGHT_STYLES - 1, it->style + steps );
		ME_Changed();
	}
	y += ME_CTRL_H + ME_GAP;
	ME_Text( x, y, S_COLOR_GREY "With a style it flickers or pulses as that does;", meWhite );
	ME_Text( x, y + ME_ROW_H, S_COLOR_GREY "set the style's pattern in Styles", meWhite );
}

static void ME_DrawTriggersTab( float x, float y, float w ) {
	meItem_t *it = ME_DrawItemList( &y, x, w, ME_ITEM_TRIGGER, "Add" );
	const float half = ( w - 4 ) * 0.5f;
	int steps;

	if ( !it ) {
		ME_Text( x, y + 4, S_COLOR_GREY "A box that runs a console command when you", meWhite );
		ME_Text( x, y + 4 + ME_ROW_H, S_COLOR_GREY "walk into it; or one the storyteller's stories", meWhite );
		ME_Text( x, y + 4 + ME_ROW_H * 2, S_COLOR_GREY "act on, used with the use key looking at it", meWhite );
		return;
	}
	// walked into, or used: the use key, looking at it, within reach
	if ( ME_Button( x, y, half, "Walked into", (qboolean)!it->use, qtrue ) && it->use ) {
		it->use = qfalse;
		ME_Changed();
	}
	if ( ME_Button( x + half + 4, y, half, "Used (+use)", it->use, qtrue ) && !it->use ) {
		it->use = qtrue;
		ME_Changed();
	}
	y += ME_CTRL_H + ME_GAP;
	ME_Text( x, y + 2, "Name", meDim );
	ME_TextBox( x + 52, y, w - 52, ME_FIELD_TRIGGERNAME, it->name, "the storyteller's, optional" );
	y += ME_CTRL_H + ME_GAP;
	if ( it->use ) {
		ME_Text( x, y + 2, "Shown as", meDim );
		ME_TextBox( x + 52, y, w - 52, ME_FIELD_TRIGGERDISPLAY, it->display, "what players see: [key] ..." );
		y += ME_CTRL_H + ME_GAP;
		// JKA's usetime, in milliseconds
		steps = ME_Stepper( x, y, w, "Hold use", it->usetime > 0 ? va( "%i ms (%.2g s)", it->usetime, it->usetime / 1000.0f ) : "no, a press" );
		if ( steps ) {
			float held = (float)it->usetime;

			ME_Adjust( &held, steps, 250.0f, 0.0f, (float)ME_MAX_USETIME );
			it->usetime = (int)( held + 0.5f );
		}
		y += ME_CTRL_H + ME_GAP;
	}
	steps = ME_Stepper( x, y, w, "Width", va( "%i", ME_Round( it->size[0] * 2 ) ) );
	ME_Adjust( &it->size[0], steps, 8.0f, 1.0f, 4096.0f );
	y += ME_CTRL_H + ME_GAP;
	steps = ME_Stepper( x, y, w, "Depth", va( "%i", ME_Round( it->size[1] * 2 ) ) );
	ME_Adjust( &it->size[1], steps, 8.0f, 1.0f, 4096.0f );
	y += ME_CTRL_H + ME_GAP;
	steps = ME_Stepper( x, y, w, "Tall", va( "%i", ME_Round( it->size[2] * 2 ) ) );
	ME_Adjust( &it->size[2], steps, 8.0f, 1.0f, 4096.0f );
	y += ME_CTRL_H + ME_GAP + 4;
	// a use trigger's doing is the storyteller's
	if ( it->use ) {
		ME_Text( x, y, S_COLOR_GREY "What it does is up to the storyteller: a story's", meWhite );
		ME_Text( x, y + ME_ROW_H, S_COLOR_GREY "\"When someone uses\" with its name. Using it", meWhite );
		ME_Text( x, y + ME_ROW_H * 2, S_COLOR_GREY "plays the reach emote.", meWhite );
		return;
	}
	ME_Text( x, y, "Runs", meDim );
	y += ME_ROW_H;
	ME_TextBox( x, y, w, ME_FIELD_COMMAND, it->command, "a console command, e.g. play sound/..." );
	y += ME_CTRL_H + ME_GAP;
	if ( ME_Button( x, y, half, it->once ? "Once" : "Every time", it->once, qtrue ) ) {
		it->once = (qboolean)!it->once;
		it->fired = qfalse;
		ME_Changed();
	}
	if ( ME_Button( x + half + 4, y, half, "Try it now", qfalse, (qboolean)( it->command[0] != '\0' ) ) )
		Cbuf_AddText( va( "%s\n", it->command ) );
	y += ME_CTRL_H + ME_GAP;
	if ( it->once && it->fired ) {
		if ( ME_Button( x, y, w, "Done; let it run again", qfalse, qtrue ) )
			it->fired = qfalse;
	}
}

static void ME_AddSound( const char *name, const char *source, void *ctx ) {
	char path[MAX_QPATH];
	const int len = strlen( name );

	if ( me.numSounds >= ME_MAX_SOUNDS || me.soundPoolUsed + len + 1 > ME_SOUND_POOL || len >= MAX_QPATH )
		return;
	// streamed, these can't loop
	if ( !Q_stricmpn( name, "sound/chars/", 12 ) )
		return;
	Q_strncpyz( path, name, sizeof( path ) );
	for ( char *c = path; *c; c++ ) {
		if ( *c == '\\' )
			*c = '/';
	}
	me.sounds[me.numSounds++] = strcpy( meSoundPool + me.soundPoolUsed, path );
	me.soundPoolUsed += len + 1;
}

static int QDECL ME_CompareSounds( const void *a, const void *b ) {
	return Q_stricmp( *(const char * const *)a, *(const char * const *)b );
}

// the music first, then the sounds, each found once
static void ME_IndexSounds( void ) {
	static const char *roots[] = { "music", "sound" };
	static const char *exts[] = { ".mp3", ".wav" };
	int count = 0;

	me.numSounds = me.soundPoolUsed = 0;
	for ( int r = 0; r < (int)ARRAY_LEN( roots ); r++ ) {
		const int from = me.numSounds;

		for ( int e = 0; e < (int)ARRAY_LEN( exts ); e++ )
			FS_ListFilesRecursiveFrom( roots[r], exts[e], ME_AddSound, NULL );
		qsort( me.sounds + from, me.numSounds - from, sizeof( me.sounds[0] ), ME_CompareSounds );
		// the same file in several pk3s
		count = from;
		for ( int i = from; i < me.numSounds; i++ ) {
			if ( count == from || Q_stricmp( me.sounds[i], me.sounds[count - 1] ) )
				me.sounds[count++] = me.sounds[i];
		}
		me.numSounds = count;
	}
	me.soundsIndexed = qtrue;
}

static void ME_DrawSoundsTab( float x, float y, float w ) {
	meItem_t *it = ME_DrawItemList( &y, x, w, ME_ITEM_SPEAKER, "Add" );
	const float half = ( w - 4 ) * 0.5f;
	static int matches[ME_MAX_SOUNDS];
	int numMatches = 0, clicked;
	const int rows = 10;

	if ( !it ) {
		ME_Text( x, y + 4, S_COLOR_GREY "A speaker at the crosshair, playing a sound", meWhite );
		ME_Text( x, y + 4 + ME_ROW_H, S_COLOR_GREY "or music round and round, as a target_speaker", meWhite );
		return;
	}
	if ( ME_Button( x, y, half, it->global ? "Heard everywhere" : "Heard near it", it->global, qtrue ) ) {
		it->global = (qboolean)!it->global;
		ME_Changed();
	}
	if ( ME_Button( x + half + 4, y, half, it->off ? "Stopped" : "Playing", (qboolean)!it->off, qtrue ) ) {
		it->off = (qboolean)!it->off;
		ME_Changed();
	}
	y += ME_CTRL_H + ME_GAP + 4;

	// the sound: typed, or picked from the ones that match what's typed
	ME_Text( x, y, va( "Sound%s", it->noise[0] && !ME_SpeakerSound( it ) ? S_COLOR_RED "  not found" : "" ), meDim );
	y += ME_ROW_H;
	ME_TextBox( x, y, w, ME_FIELD_SOUND, it->noise, "type to look for one, e.g. music/" );
	y += ME_CTRL_H + ME_GAP;
	if ( !me.soundsIndexed )
		ME_IndexSounds();
	for ( int i = 0; i < me.numSounds; i++ ) {
		if ( !it->noise[0] || Q_stristr( me.sounds[i], it->noise ) )
			matches[numMatches++] = i;
	}
	float rowW;

	clicked = ME_ListRows( x, y, w, rows, numMatches, &me.soundScroll, &rowW );
	for ( int r = 0; r < rows && me.soundScroll + r < numMatches; r++ ) {
		const char *name = me.sounds[matches[me.soundScroll + r]];

		if ( !Q_stricmp( name, it->noise ) )
			ME_RowHighlight( x, y, rowW, r + me.soundScroll, me.soundScroll, rows );
		ME_TextClipped( x + 4, y + 1 + r * ME_ROW_H, rowW - 8, name, meWhite );
	}
	if ( !numMatches )
		ME_Text( x + 4, y + 2, S_COLOR_GREY "None match", meWhite );
	if ( clicked >= 0 ) {
		Q_strncpyz( it->noise, me.sounds[matches[clicked]], sizeof( it->noise ) );
		it->sfxRegistered = qfalse;
		me.field = ME_FIELD_NONE;
		ME_Changed();
	}
	y += rows * ME_ROW_H + 6;
	ME_Text( x, y, va( S_COLOR_GREY "%i of %i: the music, then the sounds", numMatches, me.numSounds ), meWhite );
	ME_Text( x, y + ME_ROW_H, S_COLOR_GREY "Shared, everyone with this client hears it", meWhite );
}

// a pattern as bars, dark to full, the letter now marked
static void ME_DrawPattern( float x, float y, float w, float h, const char *pattern ) {
	const int len = strlen( pattern ), now = len ? ( Q_max( 0, cl.serverTime ) / 50 ) % len : 0;
	const int shown = Q_min( len, (int)w );
	const float bw = shown ? w / shown : w;
	static vec4_t bar = { 0.75f, 0.75f, 0.4f, 0.9f }, lit = { 1.0f, 1.0f, 1.0f, 1.0f };

	ME_Box( x, y, w, h, mePanelLight );
	for ( int i = 0; i < shown; i++ ) {
		const float v = Com_Clamp( 0.0f, 1.0f, ( pattern[i] - 'a' ) / (float)( 'z' - 'a' ) );

		ME_Fill( x + i * bw, y + h - 1 - v * ( h - 2 ), Q_max( 1.0f, bw - 1.0f ), v * ( h - 2 ), i == now ? lit : bar );
	}
}

static void ME_FogEdited( meFog_t *f ) {
	f->override = qtrue;
	me.fogsDirty = qtrue;
	ME_Changed();
}

// a fog's thickness and colour; qtrue if either changed. Where the next row goes in *py
static qboolean ME_DrawFogLook( float x, float *py, float w, vec3_t color, float *depth ) {
	const float sw = ( w - 4.0f * ( ARRAY_LEN( meLightColors ) ) ) / ( ARRAY_LEN( meLightColors ) + 1 );
	static const char *thick[] = { "Thick", "Misty", "Thin", "Haze" };
	static const float thickDepth[] = { 256.0f, 1024.0f, 3072.0f, 8192.0f };
	const float quarter = ( w - 12 ) / 4.0f;
	qboolean changed = qfalse;
	float y = *py;
	int steps;

	// how deep it takes to see nothing through it, in steps a tenth of it
	steps = ME_Stepper( x, y, w, "Opaque at", *depth >= ME_FOG_NONE ? "none" : va( "%i", ME_Round( *depth ) ) );
	if ( steps ) {
		*depth = Com_Clamp( 16.0f, ME_FOG_NONE, *depth * powf( ME_ShiftDown() ? 1.5f : 1.1f, (float)steps ) );
		changed = qtrue;
	}
	y += ME_CTRL_H + ME_GAP;
	for ( int i = 0; i < 4; i++ ) {
		if ( ME_Button( x + i * ( quarter + 4 ), y, quarter, thick[i], (qboolean)( ME_Round( *depth ) == ME_Round( thickDepth[i] ) ), qtrue ) ) {
			*depth = thickDepth[i];
			changed = qtrue;
		}
	}
	y += ME_CTRL_H + ME_GAP + 4;
	{
		static const char *names[3] = { "Red", "Green", "Blue" };

		for ( int c = 0; c < 3; c++ ) {
			steps = ME_Stepper( x, y, w, names[c], va( "%i", ME_Round( color[c] * 255 ) ) );
			if ( steps ) {
				ME_Adjust( &color[c], steps, 0.05f, 0.0f, 1.0f );
				changed = qtrue;
			}
			y += ME_CTRL_H + ME_GAP;
		}
	}
	// the colours most fogs are, and black
	for ( int i = 0; i <= (int)ARRAY_LEN( meLightColors ); i++ ) {
		const float sx = x + i * ( sw + 4 );
		vec4_t c = { 0.0f, 0.0f, 0.0f, 1.0f };

		if ( i < (int)ARRAY_LEN( meLightColors ) )
			VectorScale( meLightColors[i], 0.75f, c );
		ME_Box( sx, y, sw, ME_CTRL_H, ME_InRect( sx, y, sw, ME_CTRL_H ) ? meHoverBox : mePanelLight );
		ME_Fill( sx + 3, y + 3, sw - 6, ME_CTRL_H - 6, c );
		if ( ME_Clicked( sx, y, sw, ME_CTRL_H ) ) {
			VectorCopy( c, color );
			changed = qtrue;
		}
	}
	*py = y + ME_CTRL_H + ME_GAP + 4;
	return changed;
}

// the map's own fogs: its list, and the one chosen
static void ME_DrawMapFogs( float x, float y, float w ) {
	const int rows = 6;
	const float half = ( w - 4 ) * 0.5f;
	int clicked, selRow = -1;
	meFog_t *f;

	if ( me.numFogs <= 1 ) {
		ME_Text( x, y, S_COLOR_GREY "This map was built without fog; Added", meWhite );
		ME_Text( x, y + ME_ROW_H, S_COLOR_GREY "volumes puts boxes of fog of our own", meWhite );
		return;
	}
	if ( me.selKind == ME_SEL_FOG && me.sel >= 1 && me.sel < me.numFogs )
		selRow = me.sel - 1;
	float rowW;

	clicked = ME_ListRows( x, y, w, rows, me.numFogs - 1, &me.fogScroll, &rowW );
	if ( selRow >= 0 )
		ME_RowHighlight( x, y, rowW, selRow, me.fogScroll, rows );
	for ( int r = 0; r < rows && me.fogScroll + r < me.numFogs - 1; r++ ) {
		const int i = me.fogScroll + r + 1;
		const meFog_t *row = &me.fogs[i];
		const float ry = y + 1 + r * ME_ROW_H;
		const qboolean none = (qboolean)( row->override && row->depth >= ME_FOG_NONE );
		vec4_t c;
		vec3_t size;

		VectorCopy( row->override ? row->color : row->mapColor, c );
		c[3] = 1.0f;
		ME_Fill( x + 4, ry + 2, 8, ME_ROW_H - 4, c );
		VectorSubtract( row->maxs, row->mins, size );
		ME_TextClipped( x + 16, ry, rowW - 20, va( "%s%s" S_COLOR_GREY "  %s", row->override ? S_COLOR_YELLOW : "",
			i == me.globalFog ? "The whole map" : va( "Fog volume %i", i ),
			none ? "taken away" : i == me.globalFog ? va( "opaque at %i", ME_Round( row->override ? row->depth : row->mapDepth ) )
			: va( "%i x %i x %i", ME_Round( size[0] ), ME_Round( size[1] ), ME_Round( size[2] ) ) ), meWhite );
	}
	if ( clicked >= 0 )
		ME_Select( ME_SEL_FOG, clicked + 1 );
	y += rows * ME_ROW_H + 8;

	if ( me.selKind != ME_SEL_FOG || me.sel < 1 || me.sel >= me.numFogs ) {
		ME_Text( x, y, S_COLOR_GREY "Pick one to change its colour and how thick", meWhite );
		ME_Text( x, y + ME_ROW_H, S_COLOR_GREY "it is, or to take it away", meWhite );
		return;
	}
	f = &me.fogs[me.sel];
	ME_TextClipped( x, y, w, me.sel == me.globalFog ? "The fog over the whole map" : va( "Fog volume %i", me.sel ), meWhite );
	y += ME_ROW_H + 2;
	if ( ME_Button( x, y, half, f->override ? "Ours: back to the map's" : "The map's", f->override, f->override ) ) {
		f->override = qfalse;
		VectorCopy( f->mapColor, f->color );
		f->depth = f->mapDepth;
		me.fogsDirty = qtrue;
		ME_Changed();
	}
	if ( ME_Button( x + half + 4, y, half, "No fog", (qboolean)( f->override && f->depth >= ME_FOG_NONE ), qtrue ) ) {
		f->depth = ME_FOG_NONE;
		ME_FogEdited( f );
	}
	y += ME_CTRL_H + ME_GAP;
	if ( me.sel != me.globalFog ) {
		if ( ME_Button( x, y, w, "Go to it", qfalse, qtrue ) ) {
			vec3_t center, size;

			VectorAdd( f->mins, f->maxs, center );
			VectorScale( center, 0.5f, center );
			VectorSubtract( f->maxs, f->mins, size );
			ME_GoTo( center, VectorLength( size ) * 0.5f );
		}
		y += ME_CTRL_H + ME_GAP;
	}
	y += 4;
	if ( ME_DrawFogLook( x, &y, w, f->color, &f->depth ) )
		ME_FogEdited( f );
	ME_Text( x, y, S_COLOR_GREY "Everyone with this client sees it once shared", meWhite );
}

// our own fog volumes: boxes of fog anywhere
static void ME_DrawAddedFogs( float x, float y, float w ) {
	meItem_t *it = ME_DrawItemList( &y, x, w, ME_ITEM_FOG, "Add" );

	if ( !it ) {
		ME_Text( x, y + 4, S_COLOR_GREY "Add puts a box of fog at the crosshair;", meWhite );
		ME_Text( x, y + 4 + ME_ROW_H, S_COLOR_GREY "what's in it is fogged, models too", meWhite );
		return;
	}
	y += 2;
	if ( ME_DrawFogLook( x, &y, w, it->color, &it->fogDepth ) )
		ME_Changed();
	{
		static const char *names[3] = { "Width", "Depth", "Tall" };

		for ( int a = 0; a < 3; a++ ) {
			const int steps = ME_Stepper( x, y, w, names[a], va( "%i", ME_Round( it->size[a] * 2 ) ) );

			ME_Adjust( &it->size[a], steps, 16.0f, 4.0f, 16384.0f );
			y += ME_CTRL_H + ME_GAP;
		}
	}
}

static void ME_DrawFogTab( float x, float y, float w ) {
	const float half = ( w - 4 ) * 0.5f;
	const meItem_t *sel = ME_SelectedItem();
	int added = 0;

	if ( !re->ext.GetFogs || !re->ext.SetFog ) {
		ME_Text( x, y, S_COLOR_GREY "Fog can be changed with the vanilla renderer", meWhite );
		return;
	}
	// what's chosen picks which
	if ( sel && sel->kind == ME_ITEM_FOG )
		me.fogAdded = qtrue;
	else if ( me.selKind == ME_SEL_FOG )
		me.fogAdded = qfalse;
	for ( int i = 0; i < me.numItems; i++ ) {
		if ( me.items[i].kind == ME_ITEM_FOG )
			added++;
	}
	// the other page: what's chosen on this one is let go, its changes kept, or it would pull
	// the tab straight back
	if ( ME_Button( x, y, half, va( "The map's (%i)", Q_max( 0, me.numFogs - 1 ) ), (qboolean)!me.fogAdded, qtrue ) && me.fogAdded ) {
		me.fogAdded = qfalse;
		if ( sel && sel->kind == ME_ITEM_FOG )
			ME_Select( ME_SEL_NONE, -1 );
	}
	if ( ME_Button( x + half + 4, y, half, va( "Added volumes (%i)", added ), me.fogAdded, qtrue ) && !me.fogAdded ) {
		me.fogAdded = qtrue;
		if ( me.selKind == ME_SEL_FOG )
			ME_Select( ME_SEL_NONE, -1 );
	}
	y += ME_CTRL_H + ME_GAP + 4;
	if ( me.fogAdded )
		ME_DrawAddedFogs( x, y, w );
	else
		ME_DrawMapFogs( x, y, w );
}

static void ME_DrawStylesTab( float x, float y, float w ) {
	const int rows = 8;
	const float half = ( w - 4 ) * 0.5f, quarter = ( w - 12 ) / 4.0f;
	static int matches[MAX_LIGHT_STYLES];
	int numMatches = 0, clicked, selRow = -1;
	meStyle_t *s;

	if ( ME_Button( x, y, w, me.allStyles ? "Showing all 64" : "Showing the map's", me.allStyles, qtrue ) )
		me.allStyles = (qboolean)!me.allStyles;
	y += ME_CTRL_H + ME_GAP;
	// 0, the whole map's light, first and always
	for ( int i = 0; i < MAX_LIGHT_STYLES; i++ ) {
		if ( !i || me.allStyles || me.styles[i].surfaces || me.styles[i].override ) {
			if ( ME_IsSel( ME_SEL_STYLE, i ) )
				selRow = numMatches;
			matches[numMatches++] = i;
		}
	}
	float rowW;

	clicked = ME_ListRows( x, y, w, rows, numMatches, &me.styleScroll, &rowW );
	if ( selRow >= 0 )
		ME_RowHighlight( x, y, rowW, selRow, me.styleScroll, rows );
	for ( int r = 0; r < rows && me.styleScroll + r < numMatches; r++ ) {
		const int i = matches[me.styleScroll + r];
		const meStyle_t *row = &me.styles[i];
		const char *pattern = row->override ? row->pattern : ME_ServerPattern( i );

		ME_TextClipped( x + 4, y + 1 + r * ME_ROW_H, rowW - 8, va( "%s%i  " S_COLOR_GREY "%s  %s%s", row->override ? S_COLOR_YELLOW : S_COLOR_WHITE, i,
			i ? va( "%i surfaces", row->surfaces ) : "the whole map", row->override ? S_COLOR_YELLOW : S_COLOR_WHITE,
			pattern[0] ? pattern : "steady" ), meWhite );
	}
	if ( clicked >= 0 )
		ME_Select( ME_SEL_STYLE, matches[clicked] );
	y += rows * ME_ROW_H + 6;

	if ( me.selKind != ME_SEL_STYLE || me.sel < 0 || me.sel >= MAX_LIGHT_STYLES ) {
		ME_Text( x, y, S_COLOR_GREY "0 is the whole map's light; the others the", meWhite );
		ME_Text( x, y + ME_ROW_H, S_COLOR_GREY "switchable lights it was built with, and", meWhite );
		ME_Text( x, y + ME_ROW_H * 2, S_COLOR_GREY "our lights set to follow them", meWhite );
		return;
	}
	s = &me.styles[me.sel];
	ME_TextClipped( x, y, w, va( "Style %i%s" S_COLOR_GREY "   the server's: %s", me.sel, me.sel ? "" : ", the whole map",
		ME_ServerPattern( me.sel )[0] ? ME_ServerPattern( me.sel ) : "steady" ), meWhite );
	y += ME_ROW_H + 2;
	// as the server has it, or ours: changing anything makes it ours
	if ( ME_Button( x, y, half, s->override ? "Ours: back to server's" : "The server's", s->override, s->override ) ) {
		s->override = qfalse;
		ME_Select( ME_SEL_STYLE, me.sel );
		ME_Changed();
	}
	if ( ME_Button( x + half + 4, y, half, "Blink it", (qboolean)( me.flashStyle == me.sel && cls.realtime < me.flashUntil ), qtrue ) ) {
		me.flashStyle = me.sel;
		me.flashUntil = cls.realtime + ME_FLASH_MS;
	}
	y += ME_CTRL_H + ME_GAP;
	if ( me.sel ) {
		if ( ME_Button( x, y, w, "Go to its lights", qfalse, (qboolean)( s->surfaces > 0 ) ) ) {
			vec3_t center, size;

			VectorAdd( s->mins, s->maxs, center );
			VectorScale( center, 0.5f, center );
			VectorSubtract( s->maxs, s->mins, size );
			ME_GoTo( center, VectorLength( size ) * 0.5f );
		}
		y += ME_CTRL_H + ME_GAP;
	}
	y += 2;

	// its pattern, typed or picked, and how it goes
	ME_TextBox( x, y, w, ME_FIELD_PATTERN, s->pattern, "letters a (dark) to z (full)" );
	y += ME_CTRL_H + ME_GAP;
	ME_DrawPattern( x, y, w, 14, s->pattern[0] ? s->pattern : "z" );
	y += 14 + ME_GAP;
	for ( int i = 0; i < (int)ARRAY_LEN( meStylePresets ); i++ ) {
		const float bx = x + ( i % 4 ) * ( quarter + 4 ), by = y + ( i / 4 ) * ( ME_CTRL_H + ME_GAP );

		if ( ME_Button( bx, by, quarter, meStylePresets[i].name, (qboolean)!strcmp( s->pattern, meStylePresets[i].pattern ), qtrue ) ) {
			Q_strncpyz( s->pattern, meStylePresets[i].pattern, sizeof( s->pattern ) );
			s->override = qtrue;
			ME_Changed();
		}
	}
	y += ( ME_CTRL_H + ME_GAP ) * 2 + 2;

	// a colour and pattern at once
	ME_Text( x, y, "Looks", meDim );
	y += ME_ROW_H;
	for ( int i = 0; i < (int)ARRAY_LEN( meStyleLooks ); i++ ) {
		const meStyleLook_t *l = &meStyleLooks[i];
		const float bx = x + ( i % 4 ) * ( quarter + 4 ), by = y + ( i / 4 ) * ( ME_CTRL_H + ME_GAP );
		const qboolean on = (qboolean)( s->override && VectorCompare( s->color, l->color ) && !strcmp( s->pattern, l->pattern ) );

		if ( ME_Button( bx, by, quarter, l->name, on, qtrue ) ) {
			VectorCopy( l->color, s->color );
			Q_strncpyz( s->pattern, l->pattern, sizeof( s->pattern ) );
			s->override = qtrue;
			ME_Changed();
		}
	}
	y += ( ME_CTRL_H + ME_GAP ) * 2 + 2;
	{
		static const char *names[3] = { "Red", "Green", "Blue" };

		for ( int c = 0; c < 3; c++ ) {
			const int steps = ME_Stepper( x, y, w, names[c], va( "%i%%", ME_Round( s->color[c] * 100 ) ) );

			if ( steps ) {
				ME_Adjust( &s->color[c], steps, 0.05f, 0.0f, 1.0f );
				s->override = qtrue;
			}
			y += ME_CTRL_H + ME_GAP;
		}
	}
	if ( !me.sel )
		ME_TextClipped( x, y, w, S_COLOR_GREY "The world follows it with the vanilla renderer", meWhite );
}

// where a point is on the screen, anywhere on it or off it; qfalse behind the camera
static qboolean ME_ProjectAny( const vec3_t point, float *x, float *y ) {
	vec3_t d;
	float forward;

	if ( !me.haveView )
		return qfalse;
	VectorSubtract( point, me.viewOrg, d );
	forward = DotProduct( d, me.viewAxis[0] );
	if ( forward < 4.0f )
		return qfalse;
	*x = SCREEN_WIDTH * 0.5f * ( 1.0f - DotProduct( d, me.viewAxis[1] ) / forward / tanf( DEG2RAD( me.viewFovX * 0.5f ) ) );
	*y = SCREEN_HEIGHT * 0.5f * ( 1.0f - DotProduct( d, me.viewAxis[2] ) / forward / tanf( DEG2RAD( me.viewFovY * 0.5f ) ) );
	return qtrue;
}

// the same, qfalse off the world's part of the screen
static qboolean ME_Project( const vec3_t point, float *x, float *y ) {
	if ( !ME_ProjectAny( point, x, y ) )
		return qfalse;
	return (qboolean)( *x >= 0 && *x <= ME_PANEL_X && *y >= 0 && *y <= SCREEN_HEIGHT );
}

// names over what's near, and over what's chosen or under the cursor
static void ME_DrawLabels( void ) {
	int labels = 0;
	float sx, sy;

	for ( int i = 0; i < me.numEnts && labels < ME_MAX_LABELS; i++ ) {
		const meEnt_t *e = &me.ents[i];
		const qboolean mark = (qboolean)( ME_IsSel( ME_SEL_ENT, i ) || ME_IsHover( ME_SEL_ENT, i ) );
		vec3_t center;

		if ( !ME_EntDrawn( e ) )
			continue;
		ME_EntCenter( e, center );
		if ( !mark && Distance( center, me.camOrg ) > ME_LABEL_DIST )
			continue;
		if ( !ME_Project( center, &sx, &sy ) )
			continue;
		if ( ME_LocksTab() && ME_FindLock( e ) ) {
			const meLock_t *l = ME_FindLock( e );

			ME_Text( sx + 4, sy - 12, va( "%s%s " S_COLOR_RED "[%s%s]", mark ? S_COLOR_WHITE : S_COLOR_GREY, ME_EntName( e ),
				l->rank < 0 ? "no one" : va( "rank %i", l->rank ), l->name[0] ? va( " %s", l->name ) : "" ), meWhite );
		} else {
			ME_Text( sx + 4, sy - 12, va( "%s%s", mark ? S_COLOR_WHITE : S_COLOR_GREY, ME_EntName( e ) ), meWhite );
		}
		labels++;
	}
	for ( int i = 0; i < me.numItems; i++ ) {
		const meItem_t *it = &me.items[i];
		const qboolean mark = (qboolean)( ME_IsSel( ME_SEL_ITEM, i ) || ME_IsHover( ME_SEL_ITEM, i ) );

		if ( !mark || !ME_Project( it->org, &sx, &sy ) )
			continue;
		if ( it->kind == ME_ITEM_MODEL )
			ME_Text( sx + 4, sy - 12, COM_SkipPath( (char *)it->model ), meWhite );
		else if ( it->kind == ME_ITEM_PROP )
			ME_Text( sx + 4, sy - 12, va( "map %s%s", ME_ItemIsBrush( it ) ? "brush" : "prop", it->removed ? " (hidden)" : "" ), meWhite );
		else if ( it->kind == ME_ITEM_LIGHT )
			ME_Text( sx + 4, sy - 12, "light", meWhite );
		else if ( it->kind == ME_ITEM_TRIGGER )
			ME_Text( sx + 4, sy - 12, va( "%s%s%s", it->use ? "use trigger" : "trigger", it->name[0] ? va( " %s", it->name ) : "",
				it->inside ? " (you're in it)" : "" ), meWhite );
		else if ( it->kind == ME_ITEM_FOG )
			ME_Text( sx + 4, sy - 12, "fog volume", meWhite );
		else
			ME_Text( sx + 4, sy - 12, "speaker", meWhite );
	}
	// the arrows' letters
	if ( ME_GizmoItem() ) {
		static const char *letters[3] = { S_COLOR_RED "X", S_COLOR_GREEN "Y", S_COLOR_BLUE "Z" };
		const meItem_t *it = ME_GizmoItem();

		for ( int a = 0; a < 3; a++ ) {
			vec3_t tip;

			VectorMA( it->org, ME_GizmoLen( it ) * 1.12f, meAxes[a], tip );
			if ( ME_Project( tip, &sx, &sy ) )
				ME_Text( sx - 3, sy - 6, a == me.gizmoAxis ? va( S_COLOR_YELLOW "%s", letters[a] + 2 ) : letters[a], meWhite );
		}
	}
}

// the edits kept by name, and shared with everyone
static void ME_DrawLayouts( float x, float y, float w ) {
	const float half = ( w - 4 ) * 0.5f;
	const qboolean armed = (qboolean)( me.confirmTime && cls.realtime - me.confirmTime < 3000 );
	const qboolean experimental = CL_RpHooks_Experimental();
	const qboolean sending = (qboolean)( me.shareSent < me.numShare );
	const int rows = 10;
	int clicked;

	ME_Text( x, y, "Layouts, kept by name", meWhite );
	y += ME_ROW_H + 2;
	ME_TextBox( x, y, w - 64, ME_FIELD_LAYOUT, me.layoutName, "a name for these edits" );
	if ( ME_Button( x + w - 60, y, 60, "Save as", qfalse, ME_LayoutNameOk( me.layoutName ) ) )
		ME_SaveLayout( me.layoutName );
	y += ME_CTRL_H + ME_GAP;
	float rowW;

	clicked = ME_ListRows( x, y, w, rows, me.numLayouts, &me.layoutScroll, &rowW );
	if ( me.layoutSel >= 0 )
		ME_RowHighlight( x, y, rowW, me.layoutSel, me.layoutScroll, rows );
	for ( int r = 0; r < rows && me.layoutScroll + r < me.numLayouts; r++ )
		ME_TextClipped( x + 4, y + 1 + r * ME_ROW_H, rowW - 8, me.layouts[me.layoutScroll + r], meWhite );
	if ( !me.numLayouts )
		ME_Text( x + 4, y + 1, S_COLOR_GREY "None for this map yet", meWhite );
	if ( clicked >= 0 ) {
		me.layoutSel = clicked;
		me.confirmTime = 0;
		Q_strncpyz( me.layoutName, me.layouts[clicked], sizeof( me.layoutName ) );
	}
	y += rows * ME_ROW_H + 6;
	if ( ME_Button( x, y, half, "Load", qfalse, (qboolean)( me.layoutSel >= 0 ) ) )
		ME_LoadLayout( me.layouts[me.layoutSel] );
	if ( ME_Button( x + half + 4, y, half, armed ? "Click again" : "Delete", armed, (qboolean)( me.layoutSel >= 0 ) ) ) {
		if ( armed ) {
			me.confirmTime = 0;
			ME_DeleteLayout( me.layouts[me.layoutSel] );
		} else {
			me.confirmTime = cls.realtime;
		}
	}
	y += ME_CTRL_H + ME_GAP;
	if ( ME_Button( x, y, w, "Clear: no edits", qfalse, qtrue ) ) {
		ME_Load();
		ME_Message( "No edits now; a layout loads them again" );
	}
	y += ME_CTRL_H + ME_GAP + 10;

	ME_Text( x, y, "Everyone on the map with this client", meWhite );
	y += ME_ROW_H + 2;
	if ( ME_Button( x, y, w, RP_EXPERIMENTAL_LABEL, experimental, qtrue ) ) {
		if ( experimental ) {
			CL_RpHooks_SetExperimental( qfalse );
			ME_Message( "Experimental features off" );
		} else {
			me.experimentalBox = qtrue;
			me.field = ME_FIELD_NONE;
		}
	}
	y += ME_CTRL_H + ME_GAP;
	// off, either of them asks first
	if ( ME_Button( x, y, half, sending ? va( "Sending %i/%i", me.shareSent, me.numShare ) : "Share", sending, (qboolean)!sending ) ) {
		if ( experimental )
			ME_Share();
		else
			me.experimentalBox = qtrue;
	}
	if ( ME_Button( x + half + 4, y, half, "Unshare", qfalse, qtrue ) ) {
		if ( experimental )
			ME_Unshare();
		else
			me.experimentalBox = qtrue;
	}
	y += ME_CTRL_H + ME_GAP;
	if ( me.shared ) {
		ME_TextClipped( x, y, w, S_COLOR_CYAN "Showing edits shared with you", meWhite );
		y += ME_ROW_H + 2;
	}
	ME_Text( x, y, S_COLOR_GREY "Models, props, lights, styles, fog, hidden", meWhite );
	ME_Text( x, y + ME_ROW_H, S_COLOR_GREY "brushes and use triggers go (not commands).", meWhite );
	ME_Text( x, y + ME_ROW_H * 2, S_COLOR_GREY "Only the look changes: no collision.", meWhite );
	ME_Text( x, y + ME_ROW_H * 3, S_COLOR_GREY "Who joins later needs it shared again.", meWhite );
}

// the warning, over the world: OK turns experimental features on for the session
static void ME_DrawExperimentalBox( void ) {
	const float w = 320, h = 120, x = ( ME_PANEL_X - w ) * 0.5f, y = 160;
	char text[MAX_STRING_CHARS], *line = text;
	float ly = y + 24;

	ME_Box( x, y, w, h, mePanelOpaque );
	ME_Text( x + 8, y + 6, S_COLOR_YELLOW "Experimental features", meWhite );
	// wrapped by words
	Q_strncpyz( text, RP_EXPERIMENTAL_WARNING, sizeof( text ) );
	while ( *line && ly < y + h - 34 ) {
		char *end = line, *lastSpace = NULL, keep;

		while ( *end ) {
			if ( *end == ' ' ) {
				keep = *end;
				*end = '\0';
				const qboolean fits = (qboolean)( ME_TextWidth( line ) <= w - 16 );
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
		ME_Text( x + 8, ly, line, meWhite );
		*end = keep;
		line = *end ? end + 1 : end;
		ly += ME_ROW_H;
	}
	if ( ME_Button( x + 8, y + h - 24, 80, "OK", qfalse, qtrue ) ) {
		CL_RpHooks_SetExperimental( qtrue );
		ME_Message( S_COLOR_YELLOW "Experimental features on, for this session" );
		me.experimentalBox = qfalse;
		return;
	}
	if ( ME_Button( x + w - 88, y + h - 24, 80, "Cancel", qfalse, qtrue ) )
		me.experimentalBox = qfalse;
}

static void ME_DrawEditor( void ) {
	const float x = ME_PANEL_X + 6, w = ME_PANEL_W - 12;
	const float third = ( w - 8 ) / 3.0f, tabW = ( w - 4.0f * ( ME_TABS_PER_ROW - 1 ) ) / ME_TABS_PER_ROW;
	// the warning sits on top and gets the clicks first, but is drawn last
	const qboolean popup = me.experimentalBox, click = me.click;
	const int wheel = me.wheel;
	float y = 8;

	if ( popup )
		me.click = qfalse, me.wheel = 0;

	ME_DrawLabels();

	// top left: what this is, how it went
	ME_Text( 8, 40, va( "Map editor" S_COLOR_GREY "   %s   fov %i", me.map, ME_Round( me.camFov ) ), meWhite );
	if ( me.storyPick >= 0 )
		ME_Text( 8, 40 + ME_ROW_H, va( S_COLOR_CYAN "For the story: click the %s it waits on   " S_COLOR_GREY "Esc goes back",
			me.storyPick == 0 ? "map trigger" : me.storyPick == 1 ? "door" : "use trigger" ), meWhite );
	else if ( me.grabbing )
		ME_Text( 8, 40 + ME_ROW_H, S_COLOR_CYAN "Carrying: it goes where you point; click or G puts it down, Esc puts it back", meWhite );
	if ( me.message[0] && cls.realtime - me.messageTime < ME_MESSAGE_MS )
		ME_TextClipped( 8, 40 + ME_ROW_H * 2, ME_PANEL_X - 16, me.message, meWhite );
	ME_Fill( ME_PANEL_X * 0.5f - 6, SCREEN_HEIGHT * 0.5f, 13, 1, meDim );
	ME_Fill( ME_PANEL_X * 0.5f, SCREEN_HEIGHT * 0.5f - 6, 1, 13, meDim );

	// bottom left: the keys
	{
		static const char *help[] = {
			"Hold right mouse: look and fly   WASD  Space/C up/down",
			"Shift fast  Ctrl slow  wheel zooms while flying",
			"Click: pick, a map prop or brush too   G: carry it   Del: delete   Ctrl+D: copy",
			"Drag an arrow: along X, Y or Z (Shift 16, Ctrl 1/4)   Ctrl+S: save   Esc: back"
		};
		for ( int i = 0; i < (int)ARRAY_LEN( help ); i++ )
			ME_Text( 8, SCREEN_HEIGHT - 12 - ( ARRAY_LEN( help ) - 1 - i ) * ME_ROW_H, va( S_COLOR_GREY "%s", help[i] ), meWhite );
	}

	ME_Fill( ME_PANEL_X, 0, ME_PANEL_W, SCREEN_HEIGHT, mePanel );
	ME_Text( x, y, va( "Edits%s", me.shared ? S_COLOR_CYAN " (shared with you)" : me.dirty ? S_COLOR_YELLOW " (not saved)" : "" ), meWhite );
	y += ME_ROW_H + 2;
	if ( ME_Button( x, y, third, "Save", qfalse, qtrue ) )
		ME_Save();
	if ( ME_Button( x + third + 4, y, third, "Layouts", me.showLayouts, qtrue ) ) {
		me.showLayouts = (qboolean)!me.showLayouts;
		me.field = ME_FIELD_NONE;
		if ( me.showLayouts )
			ME_ListLayouts();
	}
	if ( ME_Button( x + ( third + 4 ) * 2, y, third, rpc_mapEdits->integer ? "Always on" : "Editor only", (qboolean)( rpc_mapEdits->integer != 0 ), qtrue ) )
		Cvar_SetValue( "rpc_mapEdits", rpc_mapEdits->integer ? 0 : 1 );
	y += ME_CTRL_H + ME_GAP + 4;

	for ( int i = 0; i < ME_NUM_TABS; i++ ) {
		const float tx = x + ( i % ME_TABS_PER_ROW ) * ( tabW + 4 ), ty = y + ( i / ME_TABS_PER_ROW ) * ( ME_CTRL_H + 2 );

		if ( ME_Button( tx, ty, tabW, meTabNames[i], (qboolean)( me.tab == i && !me.showLayouts ), qtrue ) ) {
			me.tab = i;
			me.showLayouts = qfalse;
			me.field = ME_FIELD_NONE;
		}
	}
	y += ( ME_CTRL_H + 2 ) * ( ( ME_NUM_TABS + ME_TABS_PER_ROW - 1 ) / ME_TABS_PER_ROW ) + ME_GAP + 4;

	if ( me.showLayouts )
		ME_DrawLayouts( x, y, w );
	else switch ( me.tab ) {
	case ME_TAB_MAP:		ME_DrawMapTab( x, y, w );		break;
	case ME_TAB_MODELS:		ME_DrawModelsTab( x, y, w );	break;
	case ME_TAB_LIGHTS:		ME_DrawLightsTab( x, y, w );	break;
	case ME_TAB_TRIGGERS:	ME_DrawTriggersTab( x, y, w );	break;
	case ME_TAB_SOUNDS:		ME_DrawSoundsTab( x, y, w );	break;
	case ME_TAB_LOCKS:		ME_DrawLocksTab( x, y, w );		break;
	case ME_TAB_FOG:		ME_DrawFogTab( x, y, w );		break;
	default:				ME_DrawStylesTab( x, y, w );	break;
	}

	WG_DrawOverlay();

	if ( popup && me.experimentalBox ) {
		me.click = click;
		me.wheel = wheel;
		ME_DrawExperimentalBox();
		me.click = qfalse;
	}

	// a click that's left: on the world it picks, on the panel it leaves the text box
	if ( me.click ) {
		me.field = ME_FIELD_NONE;
		if ( me.clickX < ME_PANEL_X )
			ME_ClickWorld( me.clickX, me.clickY );
	}
	me.click = qfalse;
	me.wheel = 0;

	if ( !me.flying )
		re->DrawStretchPic( me.cursorX, me.cursorY, 32, 32, 0, 0, 1, 1, cls.cursorShader );
}

void CL_MapEditor_Draw( void ) {
	if ( me.open ) {
		ME_DrawEditor();
		return;
	}
	// a use trigger looked at: which key uses it
	if ( me.useAim >= 0 && me.useAim < me.numItems && ME_Shown() && !Key_GetCatcher() ) {
		const meItem_t *it = &me.items[me.useAim];
		const int key = Key_GetKey( "+use" );
		const char *text = va( S_COLOR_YELLOW "[%s]" S_COLOR_WHITE " %s", key >= 0 ? Key_KeynumToString( key ) : "use",
			it->display[0] ? it->display : it->name[0] ? it->name : "Use" );

		if ( !me.font ) {
			me.font = re->RegisterFont( "arialnb" );
			if ( !me.font )
				me.font = cls.menuFont;
		}
		ME_Text( ( SCREEN_WIDTH - ME_TextWidth( text ) ) * 0.5f, SCREEN_HEIGHT * 0.5f + 20, text, meWhite );
		// held: how far along
		if ( it->usetime > 0 && me.holdItem == me.useAim && !me.holdDone ) {
			const float bw = 120.0f, frac = Com_Clamp( 0.0f, 1.0f, (float)( cls.realtime - me.holdStart ) / it->usetime );

			ME_Fill( ( SCREEN_WIDTH - bw ) * 0.5f, SCREEN_HEIGHT * 0.5f + 36, bw, 5, mePanel );
			ME_Fill( ( SCREEN_WIDTH - bw ) * 0.5f + 1, SCREEN_HEIGHT * 0.5f + 37, ( bw - 2 ) * frac, 3, meHighlight );
		} else if ( it->usetime > 0 ) {
			const char *hold = va( S_COLOR_GREY "hold %.2g s", it->usetime / 1000.0f );

			ME_Text( ( SCREEN_WIDTH - ME_TextWidth( hold ) ) * 0.5f, SCREEN_HEIGHT * 0.5f + 34, hold, meWhite );
		}
	}
}

/*
===============================================================================

Commands

===============================================================================
*/

// mapedit: the editor; mapedit save|load|clear: the edits; mapedit on|off: shown outside it
void CL_MapEditor_f( void ) {
	const char *arg = Cmd_Argv( 1 );

	if ( !arg[0] ) {
		if ( me.open )
			CL_MapEditor_Close();
		else
			CL_MapEditor_Open();
		return;
	}
	if ( !Q_stricmp( arg, "on" ) || !Q_stricmp( arg, "off" ) ) {
		Cvar_Set( "rpc_mapEdits", Q_stricmp( arg, "on" ) ? "0" : "1" );
		return;
	}
	if ( cls.state != CA_ACTIVE ) {
		Com_Printf( "Not on a map.\n" );
		return;
	}
	ME_CheckMap();
	if ( !Q_stricmp( arg, "save" ) ) {
		if ( Cmd_Argv( 2 )[0] )
			ME_SaveLayout( Cmd_Argv( 2 ) );
		else
			ME_Save();
		Com_Printf( "%s\n", me.message );
	} else if ( !Q_stricmp( arg, "load" ) && Cmd_Argv( 2 )[0] ) {
		ME_LoadLayout( Cmd_Argv( 2 ) );
		Com_Printf( "%s\n", me.message );
	} else if ( !Q_stricmp( arg, "clear" ) || !Q_stricmp( arg, "reload" ) ) {
		ME_Load();
		Com_Printf( "Map edits cleared: a layout loads them again\n" );
	} else if ( !Q_stricmp( arg, "layouts" ) ) {
		ME_ListLayouts();
		Com_Printf( "Layouts for %s, in %s:\n", me.map, ME_LayoutDir() );
		for ( int i = 0; i < me.numLayouts; i++ )
			Com_Printf( "  %s\n", me.layouts[i] );
		if ( !me.numLayouts )
			Com_Printf( "  none\n" );
	} else if ( !Q_stricmp( arg, "share" ) ) {
		ME_Share();
		Com_Printf( "%s\n", me.message );
	} else if ( !Q_stricmp( arg, "unshare" ) ) {
		ME_Unshare();
		Com_Printf( "%s\n", me.message );
	} else {
		Com_Printf( "mapedit : open or close the map editor\n"
			"mapedit save [name] : keep the edits as a layout: the one loaded, or of that name\n"
			"mapedit load <name> : a layout in place of the edits shown\n"
			"mapedit layouts : the layouts kept for this map, in %s\n"
			"mapedit clear : no edits; a map always starts without any\n"
			"mapedit share : the edits to everyone on the map with this client (experimental)\n"
			"mapedit unshare : everyone back to their own\n"
			"mapedit on|off : show them outside the editor too, or not\n", ME_LayoutDir() );
	}
}

void CL_MapEditor_Init( void ) {
	memset( &me, 0, sizeof( me ) );
	me.sel = -1;
	me.camFov = 90.0f;
	me.flashStyle = -1;
	me.kvEnt = -1;
	me.showTriggers = me.showBrushes = qtrue;
	me.showPoints = qtrue;
	for ( int i = 0; i < MAX_LIGHT_STYLES; i++ )
		VectorSet( me.styles[i].color, 1, 1, 1 );
	rpc_mapEdits = Cvar_Get( "rpc_mapEdits", "1", CVAR_ARCHIVE_ND, "Show the map edits made with /mapedit outside the editor too" );
	rpc_mapShare = Cvar_Get( "rpc_mapShare", "1", CVAR_ARCHIVE_ND, "Show the map edits an admin shares with everyone" );
	me.layoutSel = -1;
	me.candBrush = -1;
	me.gizmoAxis = me.dragAxis = -1;
	me.useAim = -1;
	me.holdItem = -1;
	me.storyPick = -1;
	memset( &ml, 0, sizeof( ml ) );
	ml.showDoors = ml.showTriggers = qtrue;
}

// the cgame is going, maybe to another map: the renderer's handles go with it
void CL_MapEditor_Shutdown( void ) {
	CL_MapEditor_Close();
	ME_ForgetHandles();
	me.haveView = qfalse;
	me.havePlayerView = qfalse;
	// the pure pk3 list can change with the next map or server
	me.indexed = qfalse;
	// the map's own are read again when it's up; ours stay if it's this map again
	me.reparse = qtrue;
	me.numCandSeeds = 0;
	if ( me.selKind == ME_SEL_PROP )
		me.selKind = ME_SEL_NONE;
	// the next map's locks are its own
	ml.numLocks = ml.numQueue = 0;
	ml.haveList = qfalse;
	ml.askTime = ml.lineTime = 0;
	ml.scroll = 0;
}
