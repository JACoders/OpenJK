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

// cl_storyteller.cpp -- assemble a mission out of steps, then play it
//
// A story is a list of steps run one after the other: waits, a stop until the
// storyteller says go on, chat, console commands, and the RPMod commands the
// managers send (npc spawn/tele/kill/..., rpmodel add, rpeffect, rpshader remap and
// overlay, the shader manager's sky look).
// The panel has the actions to pick from, the story, and the selected step's
// settings. Commands go out paced by the server's flood protection, and a story
// keeps playing with the panel closed. Stories are saved in storyteller/*.story,
// one step a line: its action, 1 or 0 for whether it runs, and its settings.
//
// "When someone comes" waits for a player or NPC to come into a map trigger, or near a
// spot or an effect, or for a door to move. A story starting with it can be
// armed: a copy of it waits in the background and plays when that happens, once or
// every time, several at once. The server only sends this client who is near it, so the
// rest of the map is watched two more ways (see FAR AWAY): players with this client are
// told where the stories wait and say when they come or go, and the others' positions are
// asked of the server with rporigin, one a command. NPCs out of sight still can't be seen.
// It can also wait for someone to use one of the map editor's use triggers, by its name.
//
// Several stories can be open at once, each in a numbered tab, and steps copied between
// them: Ctrl+click and Shift+click pick several, Ctrl+C, Ctrl+X, Ctrl+V and Del act on
// them. The story playing keeps playing from its own tab whichever is shown.
//
// Where the stories armed wait, and the "when someone comes" of the one shown, are drawn
// in the world for the storyteller only: a ring round a spot or an effect, a box round a
// map trigger or a door, and the story's name over it (storyteller markers on|off).
//
// This lives in the engine rather than cgame so it works whatever cgame the
// server's mod ships (RPMod loads its own).

#include "client.h"
#include "qcommon/cm_public.h"
#include "cl_widgets.h"

#define ST_TOGGLE_CMD		"storyteller"
#define ST_SAVE_DIR			"storyteller"		// in the game folder
#define ST_SAVE_EXT			".story"

#define ST_MAX_STEPS		512
#define ST_MAX_FIELDS		10
#define ST_VALUE_LEN		192
#define ST_MAX_STORIES		512
#define ST_MESSAGE_MS		5000
#define ST_CONFIRM_MS		3000
#define ST_DOUBLECLICK_MS	400
#define ST_TRACE_DIST		8192.0f
#define ST_GROUND_HEIGHT	25.0f	// NPC origins sit this far above the floor

// layout, in 640x480
#define ST_ROW_H			13.0f
#define ST_CTRL_H			15.0f
#define ST_TEXT_SCALE		0.8f
#define ST_TOP_Y			46.0f
#define ST_ACTIONS_X		16.0f
#define ST_ACTIONS_W		150.0f
#define ST_ACTIONS_H		250.0f
#define ST_STEPS_X			176.0f
#define ST_STEPS_W			218.0f
#define ST_STEPS_H			330.0f
#define ST_EDIT_X			404.0f
#define ST_EDIT_W			220.0f
#define ST_EDIT_LABEL_W		64.0f
#define ST_BUTTON_Y			410.0f
#define ST_POPUP_X			170.0f
#define ST_POPUP_W			300.0f

typedef enum {
	ST_K_TEXT,
	ST_K_NUMBER,
	ST_K_CHOICE		// cycles through choices, '|' separated
} stKind_t;

typedef struct {
	const char	*label;
	stKind_t	kind;
	const char	*def;
	const char	*choices;
} stFieldDef_t;

typedef enum {
	ST_A_WAIT,
	ST_A_PAUSE,
	ST_A_TRIGGER,
	ST_A_IF,
	ST_A_ELSE,
	ST_A_ENDIF,
	ST_A_COMMAND,
	ST_A_SAY,
	ST_A_CENTER,
	ST_A_NOTIFY,
	ST_A_NPC_TALK,
	ST_A_OBJECTIVE,
	ST_A_NPC_SPAWN,
	ST_A_NPC_TELE,
	ST_A_NPC_CMD,
	ST_A_NPC_WALK,
	ST_A_MODEL,
	ST_A_EFFECT,
	ST_A_SCREEN,
	ST_A_SHADER,
	ST_A_OVERLAY,
	ST_A_SKY,
	ST_A_LOCK,
	ST_A_SOUND,
	ST_A_MUSIC,
	ST_A_CINEMATIC,
	ST_A_CIN_START,
	ST_A_CAM_SHOT,
	ST_A_CAM_FOV,
	ST_A_CIN_BARS,
	ST_A_CIN_END,
	ST_NUM_ACTIONS
} stAction_t;

typedef struct {
	const char		*key;			// in story files
	const char		*name;
	const char		*category;
	const char		*desc;
	int				posField;		// first of X Y Z, -1 if it has no spot
	int				yawField;		// -1 if it has none
	int				numFields;
	stFieldDef_t	fields[ST_MAX_FIELDS];
} stActionDef_t;

#define ST_NPC_COMMANDS		"kill|freeze|unfreeze|emote|stop emote|follow me|stop following|dialog|no dialog|hologram|projector|no hologram"

static const stActionDef_t stActions[ST_NUM_ACTIONS] = {
	{ "wait", "Wait", "Flow", "Waits this many seconds before the next step.", -1, -1, 1, {
		{ "Seconds", ST_K_NUMBER, "2", NULL } } },
	{ "pause", "Wait for me", "Flow", "Stops until you press Continue, or run /storyteller continue (bind it to a key). The note shows while it waits.", -1, -1, 1, {
		{ "Note", ST_K_TEXT, "", NULL } } },
	{ "trigger", "When someone comes", "Flow", "Waits until someone comes into a map trigger or near a spot or an effect, a door moves, someone says one of the words (| between them), or someone uses one of the map editor's use triggers (by its name). With For, they have to stay in that many seconds. A story starting with it can be armed. Players out of sight are found too: those with this client say where they are (with experimental on), the others are asked for with rporigin.", 5, -1, 9, {
		{ "Where", ST_K_CHOICE, "map trigger", "map trigger|near a spot|near an effect|door|someone says|someone uses" },
		{ "Which", ST_K_TEXT, "", NULL },
		{ "Who", ST_K_CHOICE, "any player", "any player|any NPC|anyone|me|named" },
		{ "Name", ST_K_TEXT, "", NULL },
		{ "Radius", ST_K_NUMBER, "96", NULL },
		{ "X", ST_K_NUMBER, "", NULL },
		{ "Y", ST_K_NUMBER, "", NULL },
		{ "Z", ST_K_NUMBER, "", NULL },
		{ "For (s)", ST_K_NUMBER, "0", NULL } } },
	{ "if", "If", "Flow", "Goes under a When step, and looks once, when the story gets to it: how many are in a map trigger or near a spot or an effect, or whether someone has said one of the words (| between them), or used the use trigger by that name, since the story began. True, the steps after it play; false, the ones after its Else, or after its End if.", 5, -1, 10, {
		{ "Where", ST_K_CHOICE, "map trigger", "map trigger|near a spot|near an effect|someone says|someone uses" },
		{ "Which", ST_K_TEXT, "", NULL },
		{ "Who", ST_K_CHOICE, "any player", "any player|any NPC|anyone|me|named" },
		{ "Name", ST_K_TEXT, "", NULL },
		{ "Radius", ST_K_NUMBER, "96", NULL },
		{ "X", ST_K_NUMBER, "", NULL },
		{ "Y", ST_K_NUMBER, "", NULL },
		{ "Z", ST_K_NUMBER, "", NULL },
		{ "How many", ST_K_CHOICE, "at least", "at least|at most|exactly" },
		{ "Count", ST_K_NUMBER, "1", NULL } } },
	{ "else", "Else", "Flow", "The steps after it play when the If before it was false, up to its End if.", -1, -1, 0, { } },
	{ "endif", "End if", "Flow", "Where an If's steps end: the story goes on from here either way.", -1, -1, 0, { } },
	{ "command", "Run a command", "Flow", "Runs it as if typed in the console, e.g. rpcinematic intro, or exec mission.cfg. Separate several with ;", -1, -1, 1, {
		{ "Command", ST_K_TEXT, "", NULL } } },
	{ "say", "Say", "Speech", "Says this in chat, as you, or as one of the players on the server: by their number or a part of their name. As a player it shows in chat as theirs for everyone with this client (sent hidden in an rpnotify; others see the raw line in their console).", -1, -1, 3, {
		{ "Text", ST_K_TEXT, "", NULL },
		{ "As", ST_K_CHOICE, "me", "me|a player" },
		{ "Player", ST_K_TEXT, "", NULL } } },
	{ "center", "Center message", "Speech", "Shows the text in the middle of the screen with rpcentersay, for everyone (all), or one player. 0 seconds keeps each player's own setting.", -1, -1, 3, {
		{ "Who", ST_K_TEXT, "all", NULL },
		{ "Seconds", ST_K_NUMBER, "3", NULL },
		{ "Text", ST_K_TEXT, "", NULL } } },
	{ "notify", "Notification", "Speech", "Shows the text in the top left corner with rpnotify, for everyone (all) or one player, this many seconds. A countdown is a live count of the seconds left (rpnotify's <timer>), on each player's screen; or type <timer> in the text where it goes.", -1, -1, 4, {
		{ "Who", ST_K_TEXT, "all", NULL },
		{ "Seconds", ST_K_NUMBER, "10", NULL },
		{ "Text", ST_K_TEXT, "", NULL },
		{ "Countdown", ST_K_CHOICE, "no", "no|after the text|before the text" } } },
	{ "npctalk", "NPC talk", "Speech", "The NPC says it in chat, as a player would, for everyone with this client (sent hidden in an rpnotify; others see the raw line in their console). The emote is played on the NPC with npc emote.", -1, -1, 5, {
		{ "NPC", ST_K_TEXT, "", NULL },
		{ "Name", ST_K_TEXT, "", NULL },
		{ "Name color", ST_K_TEXT, "^7", NULL },
		{ "Emote", ST_K_TEXT, "none", NULL },
		{ "Text", ST_K_TEXT, "", NULL } } },
	{ "objective", "Objective", "Speech", "Adds to the mission's objectives, listed on the left while Esc is up, for everyone with this client (sent hidden in an rpnotify). Done strikes one through; it and undone and remove take its number or its text. Clear takes them all, or with done only the struck ones.", -1, -1, 2, {
		{ "Do", ST_K_CHOICE, "add", "add|done|undone|remove|clear|title" },
		{ "Text", ST_K_TEXT, "", NULL } } },
	{ "npcspawn", "Spawn NPC", "NPCs", "Spawns an NPC at the spot, as the NPC manager does. Give it a targetname to command it in later steps.", 6, 9, 10, {
		{ "Type", ST_K_TEXT, "", NULL },
		{ "Targetname", ST_K_TEXT, "", NULL },
		{ "Team", ST_K_CHOICE, "default", "default|player|enemy" },
		{ "Vehicle", ST_K_CHOICE, "no", "no|yes" },
		{ "Model", ST_K_TEXT, "", NULL },
		{ "Scale", ST_K_NUMBER, "", NULL },
		{ "X", ST_K_NUMBER, "", NULL },
		{ "Y", ST_K_NUMBER, "", NULL },
		{ "Z", ST_K_NUMBER, "", NULL },
		{ "Yaw", ST_K_NUMBER, "0", NULL } } },
	{ "npctele", "Teleport NPC", "NPCs", "Teleports the NPCs with this targetname (or number, or all) to the spot.", 1, 4, 5, {
		{ "Target", ST_K_TEXT, "", NULL },
		{ "X", ST_K_NUMBER, "", NULL },
		{ "Y", ST_K_NUMBER, "", NULL },
		{ "Z", ST_K_NUMBER, "", NULL },
		{ "Yaw", ST_K_NUMBER, "0", NULL } } },
	{ "npccmd", "NPC command", "NPCs", "Runs an npc command on the NPCs with this targetname (or number, or all). Emote and dialog take a value.", -1, -1, 3, {
		{ "Target", ST_K_TEXT, "", NULL },
		{ "Command", ST_K_CHOICE, "kill", ST_NPC_COMMANDS },
		{ "Value", ST_K_TEXT, "", NULL } } },
	{ "npcwalk", "Walk NPC", "NPCs", "Walks the NPCs with this targetname (or number, or several with commas between, keeping their places) along a route made in the NPC manager's Walk row, once or round and round. Everyone with this client sees them walk; the others see them hop from spot to spot.", -1, -1, 4, {
		{ "Target", ST_K_TEXT, "", NULL },
		{ "Route", ST_K_TEXT, "", NULL },
		{ "Pace", ST_K_CHOICE, "walk", "walk|run" },
		{ "Loop", ST_K_CHOICE, "no", "no|yes" } } },
	{ "model", "Place model", "World", "Places a model at the spot with rpmodel add, as the model manager does. Scale is in percent.", 1, 5, 9, {
		{ "Model", ST_K_TEXT, "", NULL },
		{ "X", ST_K_NUMBER, "", NULL },
		{ "Y", ST_K_NUMBER, "", NULL },
		{ "Z", ST_K_NUMBER, "", NULL },
		{ "Pitch", ST_K_NUMBER, "0", NULL },
		{ "Yaw", ST_K_NUMBER, "0", NULL },
		{ "Roll", ST_K_NUMBER, "0", NULL },
		{ "Scale", ST_K_NUMBER, "100", NULL },
		{ "Solid", ST_K_CHOICE, "yes", "yes|no" } } },
	{ "effect", "Play effect", "World", "Plays an effect at the spot with rpeffect, once or looping.", 2, 6, 8, {
		{ "Effect", ST_K_TEXT, "", NULL },
		{ "Play", ST_K_CHOICE, "once", "once|loop" },
		{ "X", ST_K_NUMBER, "", NULL },
		{ "Y", ST_K_NUMBER, "", NULL },
		{ "Z", ST_K_NUMBER, "", NULL },
		{ "Pitch", ST_K_NUMBER, "0", NULL },
		{ "Yaw", ST_K_NUMBER, "0", NULL },
		{ "Roll", ST_K_NUMBER, "0", NULL } } },
	{ "screen", "Screen effect", "World", "RPMod's alarm, fade out and camera shake, for everyone. Intensity and duration are for the shake.", -1, -1, 3, {
		{ "Effect", ST_K_CHOICE, "fade out", "fade out|fade in|alarm|stop alarm|camera shake" },
		{ "Intensity", ST_K_NUMBER, "5", NULL },
		{ "Duration", ST_K_NUMBER, "2", NULL } } },
	{ "shader", "Remap shader", "World", "Swaps a shader for another with rpshader remap, as the shader manager does.", -1, -1, 3, {
		{ "From", ST_K_TEXT, "", NULL },
		{ "To", ST_K_TEXT, "", NULL },
		{ "Time offset", ST_K_NUMBER, "", NULL } } },
	{ "overlay", "Screen overlay", "World", "Draws a shader over everyone's screen with rpshader overlay, or takes it away (by its shader or its number in the list). X, Y, width and height are on a 640x480 screen; empty is the whole screen.", -1, -1, 7, {
		{ "Do", ST_K_CHOICE, "add", "add|remove|remove all" },
		{ "Shader", ST_K_TEXT, "", NULL },
		{ "Fade", ST_K_NUMBER, "", NULL },
		{ "X", ST_K_NUMBER, "", NULL },
		{ "Y", ST_K_NUMBER, "", NULL },
		{ "Width", ST_K_NUMBER, "", NULL },
		{ "Height", ST_K_NUMBER, "", NULL } } },
	{ "sky", "Sky look", "World", "Tints the map's sky (red green blue, 255 as it is), spins it round (degrees a second) and turns it, as the shader manager's Sky tab does; a 3D skybox's view also moves. For everyone with this client, until the map changes (sent hidden in an rpnotify).", -1, -1, 7, {
		{ "Do", ST_K_CHOICE, "set", "set|reset" },
		{ "Colour", ST_K_TEXT, "255 255 255", NULL },
		{ "Spin", ST_K_NUMBER, "0", NULL },
		{ "Around", ST_K_NUMBER, "0", NULL },
		{ "Tilt", ST_K_NUMBER, "0", NULL },
		{ "Roll", ST_K_NUMBER, "0", NULL },
		{ "Move", ST_K_TEXT, "0 0 0", NULL } } },
	{ "lock", "Lock", "World", "Locks a door or a trigger_multiple with rplock, for that rank and up (-1 no one, 0 anyone with an account), or unlocks it. The target is its targetname or entity number; the map editor's Locks tab shows both. The name is shown to whoever it refuses.", -1, -1, 4, {
		{ "Do", ST_K_CHOICE, "lock", "lock|unlock|unlock all" },
		{ "Target", ST_K_TEXT, "", NULL },
		{ "Rank", ST_K_NUMBER, "0", NULL },
		{ "Name", ST_K_TEXT, "", NULL } } },
	{ "sound", "Play sound", "World", "Plays a sound with rpsound: once, looping (until removed) or local, to everyone, you, a player or at a spot. Pick it in the sound manager to hear it first.", 4, -1, 7, {
		{ "Sound", ST_K_TEXT, "", NULL },
		{ "Play", ST_K_CHOICE, "once", "once|loop|local" },
		{ "To", ST_K_CHOICE, "everyone", "everyone|me|player|spot" },
		{ "Player", ST_K_TEXT, "", NULL },
		{ "X", ST_K_NUMBER, "", NULL },
		{ "Y", ST_K_NUMBER, "", NULL },
		{ "Z", ST_K_NUMBER, "", NULL } } },
	{ "music", "Change music", "World", "Changes the music for everyone with rpmusic: a file from the music folder, the map's own again, or none. Pick it in the music manager to hear it first.", -1, -1, 2, {
		{ "Do", ST_K_CHOICE, "play", "play|map default|none" },
		{ "Music", ST_K_TEXT, "", NULL } } },
	{ "cinematic", "Play cinematic", "Cinematic", "Plays a cinematic made in the cinematic manager for everyone with this client: its camera shots, and what the players and NPCs in it do. Edit opens it there (or /cinematicmanager <name>). It goes out a part a second first, then plays; the story waits for it to end unless told not to.", -1, -1, 2, {
		{ "Cinematic", ST_K_TEXT, "", NULL },
		{ "Wait", ST_K_CHOICE, "until it ends", "until it ends|no" } } },
	{ "cinstart", "Start cinematic", "Cinematic", "Takes everyone with this client from their own view to the cinematic camera, starting where each of them looks from. Camera shots start one too. Players can leave it with /cincam stop.", -1, -1, 2, {
		{ "HUD", ST_K_CHOICE, "hide", "hide|show" },
		{ "Bars", ST_K_CHOICE, "on", "on|off" } } },
	{ "camshot", "Camera shot", "Cinematic", "Plays a shot made in the camera editor: Edit shot opens it there (or /cincam edit <name>). Fly where it should go, add spots, see the trail and preview it; its focus, fades and the NPCs walking in it are there too. Waits for the shot to end unless told not to.", -1, -1, 2, {
		{ "Shot", ST_K_TEXT, "", NULL },
		{ "Wait", ST_K_CHOICE, "until it ends", "until it ends|no" } } },
	{ "camfov", "Camera zoom", "Cinematic", "Zooms to the FOV (like cg_fov: smaller is closer) over the seconds; 0 seconds is at once.", -1, -1, 2, {
		{ "FOV", ST_K_NUMBER, "40", NULL },
		{ "Seconds", ST_K_NUMBER, "2", NULL } } },
	{ "cinbars", "Black bars", "Cinematic", "Slides the black bars in at the top and the bottom, or out. They work without the camera too.", -1, -1, 2, {
		{ "Bars", ST_K_CHOICE, "on", "on|off" },
		{ "Seconds", ST_K_NUMBER, "1", NULL } } },
	{ "cinend", "End cinematic", "Cinematic", "Everyone is back in their own view, and the bars slide out.", -1, -1, 0, {
		{ NULL, ST_K_TEXT, NULL, NULL } } },
};

typedef struct {
	stAction_t	action;
	qboolean	enabled;
	char		values[ST_MAX_FIELDS][ST_VALUE_LEN];
} stStep_t;

typedef struct {
	int			sel, scroll;
	int			shownSel;		// the selection last brought into view
	int			lastClickTime, lastClickRow;
} stList_t;

typedef enum {
	ST_POPUP_NONE,
	ST_POPUP_SAVE,
	ST_POPUP_LOAD,
	ST_POPUP_ARMED,
	ST_POPUP_CHOOSE,
	ST_POPUP_EXPERIMENTAL	// the warning before turning them on
} stPopup_t;

// what the choose popup lists
typedef enum {
	ST_CHOOSE_TRIGGER,
	ST_CHOOSE_DOOR,
	ST_CHOOSE_EFFECT,
	ST_CHOOSE_ROUTE
} stChoose_t;

#define ST_MAX_PEOPLE	128

// a story playing: the one being edited, or an armed one
typedef struct {
	qboolean	running;
	int			pc;				// the step being run
	int			stepStart;		// when a wait began, 0 = not yet
	qboolean	waiting;		// at a "wait for me"
	qboolean	goOn;			// told to go on past it
	int			sent;			// commands sent this run
	qboolean	extraDue;		// the step's own command is out, its second one isn't
	int			shotUntil;		// a camera shot plays until then

	// at a "when someone comes": who was in at the last look, so only someone new sets it off
	qboolean	watching;
	int			inside[ST_MAX_PEOPLE];
	int			numInside;
	int			moverTime, moverATime;	// the door's last move
	char		by[MAX_QPATH];			// who set it off
	int			since[ST_MAX_PEOPLE];	// when each one inside came in, to stay a while
	int			chatFrom;				// the chat heard since then counts
	int			started;				// cls.realtime the story began, for an If's chat
} stRun_t;

typedef enum {
	ST_PICK_NONE,
	ST_PICK_MODEL,
	ST_PICK_EFFECT,
	ST_PICK_SHADER,
	ST_PICK_OVERLAY,
	ST_PICK_SKY,
	ST_PICK_NPC_SPAWN,
	ST_PICK_NPC_MANAGE,
	ST_PICK_SOUND,
	ST_PICK_MUSIC,
	ST_PICK_LOCK,
	ST_PICK_TRIGGER		// a map trigger, a door or a use trigger, clicked in the map editor
} stPick_t;

#define ST_MAX_PICKED	64

#define ST_F_NONE		-1
#define ST_F_SAVE_NAME	-2

// the actions list: a category's header, then its actions
typedef struct {
	const char	*header;		// NULL for an action
	stAction_t	action;
	int			kind;			// a "when": which of stWhenKinds, else -1
} stActionRow_t;

// the kinds of "when", each its own action in the list; its Where, in the story file
#define ST_WHEN_ARMED	" A story starting with it can be armed: it plays each time it's set off."
#define ST_WHEN_FAR		" Players out of sight are found too: those with this client say where they are (with experimental on), the others are asked for with rporigin."
static const struct {
	const char	*where;
	const char	*name;
	const char	*desc;
} stWhenKinds[] = {
	{ "map trigger", "When someone enters a map trigger", "Waits until someone comes into one of the map's own triggers: stand in it and press I stand in, or pick it from the list. With For, they have to stay in that many seconds." ST_WHEN_ARMED ST_WHEN_FAR },
	{ "near a spot", "When someone comes near a spot", "Waits until someone comes within the radius of a spot, where you stand when it's added. With For, they have to stay that many seconds." ST_WHEN_ARMED ST_WHEN_FAR },
	{ "near an effect", "When someone comes near an effect", "Waits until someone comes within the radius of an effect placed with the effect manager. With For, they have to stay that many seconds." ST_WHEN_ARMED ST_WHEN_FAR },
	{ "door", "When a door moves", "Waits until one of the map's doors opens or closes: aim at it, or pick it from the list. Your game only sees a door while it's near you." ST_WHEN_ARMED },
	{ "someone says", "When someone says", "Waits until someone says one of the words in chat, | between them." ST_WHEN_ARMED },
	{ "someone uses", "When someone uses a trigger", "Waits until someone uses one of the map editor's use triggers, by its name: one with Hold use only once it's been held that long." ST_WHEN_ARMED },
};

static int ST_WhenKind( const char *where ) {
	if ( !Q_stricmp( where, "door or button" ) )
		where = "door";
	for ( int i = 0; i < (int)ARRAY_LEN( stWhenKinds ); i++ ) {
		if ( !Q_stricmp( where, stWhenKinds[i].where ) )
			return i;
	}
	return 0;
}

static struct {
	qboolean	open;
	int			font;

	// input; clicks and the wheel are handled where the control under them is drawn
	float		cursorX, cursorY;
	qboolean	click;
	float		clickX, clickY;
	int			wheel;
	int			focus;			// field of the selected step, or ST_F_*

	// the story
	stStep_t	steps[ST_MAX_STEPS];
	int			numSteps;
	char		name[MAX_QPATH];	// "" = not saved yet
	qboolean	dirty;
	stList_t	stepList;
	byte		marked[ST_MAX_STEPS];	// picked with Ctrl or Shift, with the selected one
	int			anchor;				// where a Shift+click picks from

	stActionRow_t	actionRows[ST_NUM_ACTIONS * 2 + 8];
	int				numActionRows;
	stList_t		actionList;

	// save and load
	stPopup_t	popup;
	char		saveName[MAX_QPATH];
	char		stories[ST_MAX_STORIES][MAX_QPATH];
	int			numStories;
	stList_t	storyList;
	qboolean	confirmNew;
	int			confirmTime;

	// playing
	stRun_t		run;
	int			runTab;			// the tab the story playing is in
	int			nextSend;		// for every story: the server's flood protection is per client
	int			confirmClose;	// cls.realtime a tab with changes was asked to close, 0 none

	// the armed stories, and the list a trigger step picks from
	stList_t	armedList;
	int			armTimes;			// the "Arm: N times" button's
	stChoose_t	chooseKind;
	stList_t	chooseList;

	// a manager opened from a step: what it sends comes back as steps
	stPick_t	pick;
	int			pickStep;		// the step it was opened from
	char		picked[ST_MAX_PICKED][MAX_STRING_CHARS];
	int			numPicked;
	char		pickedTrigger[ST_VALUE_LEN];	// what the map editor's click gave

	char		message[MAX_STRING_CHARS];
	int			messageTime;
} st;

static void ST_OpenArmed( void );
static void ST_OpenChoose( stChoose_t kind );
static qboolean ST_MarkersOn( void );

// the stories open, one in each tab; the one shown is in st, the others kept here
#define ST_MAX_TABS		8
typedef struct {
	stStep_t	*steps;			// while another tab is shown
	int			numSteps;
	char		name[MAX_QPATH];
	qboolean	dirty;
	stList_t	stepList;
	byte		marked[ST_MAX_STEPS];
} stTab_t;

static stTab_t	stTabs[ST_MAX_TABS];
static int		stNumTabs = 1, stActiveTab;

// steps copied, to paste in this story or another
static stStep_t	*stClip;
static int		stNumClip;

// the story playing is the one shown
static qboolean ST_RunHere( void ) {
	return (qboolean)( st.run.running && st.runTab == stActiveTab );
}

// the steps of the story playing, wherever its tab is
static stStep_t *ST_RunSteps( int *numSteps ) {
	if ( st.runTab == stActiveTab || st.runTab < 0 || st.runTab >= stNumTabs || !stTabs[st.runTab].steps ) {
		*numSteps = st.numSteps;
		return st.steps;
	}
	*numSteps = stTabs[st.runTab].numSteps;
	return stTabs[st.runTab].steps;
}

static vec4_t stWhite		= { 1.0f, 1.0f, 1.0f, 1.0f };
static vec4_t stPanel		= { 0.0f, 0.0f, 0.0f, 0.65f };
static vec4_t stPanelOpaque	= { 0.05f, 0.05f, 0.05f, 0.95f };
static vec4_t stPanelLight	= { 0.15f, 0.15f, 0.15f, 0.8f };
static vec4_t stPanelFocus	= { 0.1f, 0.2f, 0.3f, 0.9f };
static vec4_t stHighlight	= { 0.2f, 0.45f, 0.8f, 0.8f };
static vec4_t stHover		= { 1.0f, 1.0f, 1.0f, 0.12f };
static vec4_t stHoverBox	= { 0.25f, 0.25f, 0.25f, 0.9f };
static vec4_t stDanger		= { 0.6f, 0.15f, 0.15f, 0.9f };
static vec4_t stBorder		= { 0.5f, 0.5f, 0.5f, 0.8f };
static vec4_t stDim			= { 0.7f, 0.7f, 0.7f, 1.0f };
static vec4_t stAccent		= { 1.0f, 0.8f, 0.3f, 1.0f };
static vec4_t stRunning		= { 0.2f, 0.6f, 0.25f, 0.8f };
static vec4_t stMarked		= { 0.2f, 0.45f, 0.8f, 0.4f };

/*
===============================================================================

STEPS

===============================================================================
*/

static void ST_Message( const char *text ) {
	Q_strncpyz( st.message, text, sizeof( st.message ) );
	st.messageTime = cls.realtime;
}

static const stActionDef_t *ST_Def( const stStep_t *s ) {
	return &stActions[s->action];
}

static stStep_t *ST_SelectedStep( void ) {
	if ( st.stepList.sel < 0 || st.stepList.sel >= st.numSteps )
		return NULL;
	return &st.steps[st.stepList.sel];
}

// a choice field's choices, one at a time
static int ST_NumChoices( const char *choices ) {
	int n = 1;

	for ( ; *choices; choices++ ) {
		if ( *choices == '|' )
			n++;
	}
	return n;
}

static const char *ST_Choice( const char *choices, int index ) {
	static char buf[64];
	const char *end;

	while ( index-- > 0 ) {
		choices = strchr( choices, '|' );
		if ( !choices )
			return "";
		choices++;
	}
	end = strchr( choices, '|' );
	Q_strncpyz( buf, choices, end ? Q_min( (int)sizeof( buf ), (int)( end - choices ) + 1 ) : (int)sizeof( buf ) );
	return buf;
}

static int ST_ChoiceIndex( const char *choices, const char *value ) {
	const int n = ST_NumChoices( choices );

	for ( int i = 0; i < n; i++ ) {
		if ( !Q_stricmp( ST_Choice( choices, i ), value ) )
			return i;
	}
	return 0;
}

static float ST_Number( const stStep_t *s, int field ) {
	return (float)atof( s->values[field] );
}

static int ST_Round( float v ) {
	return (int)floorf( v + 0.5f );
}

static void ST_SetDefaults( stStep_t *s ) {
	const stActionDef_t *def = ST_Def( s );

	memset( s->values, 0, sizeof( s->values ) );
	for ( int i = 0; i < def->numFields; i++ )
		Q_strncpyz( s->values[i], def->fields[i].def, ST_VALUE_LEN );
}

// where the player stands, and the yaw they look along
static void ST_FillHere( stStep_t *s ) {
	const stActionDef_t *def = ST_Def( s );

	if ( def->posField < 0 || !cl.snap.valid )
		return;
	for ( int i = 0; i < 3; i++ )
		Com_sprintf( s->values[def->posField + i], ST_VALUE_LEN, "%i", ST_Round( cl.snap.ps.origin[i] ) );
	if ( def->yawField >= 0 )
		Com_sprintf( s->values[def->yawField], ST_VALUE_LEN, "%i", ST_Round( AngleNormalize360( cl.viewangles[YAW] ) ) );
}

// where the crosshair points, facing the player
static qboolean ST_FillAim( stStep_t *s ) {
	const stActionDef_t *def = ST_Def( s );
	vec3_t start, forward, end, pos;
	trace_t tr;

	if ( def->posField < 0 || !cl.snap.valid )
		return qfalse;
	VectorCopy( cl.snap.ps.origin, start );
	start[2] += cl.snap.ps.viewheight;
	AngleVectors( cl.viewangles, forward, NULL, NULL );
	VectorMA( start, ST_TRACE_DIST, forward, end );
	CM_BoxTrace( &tr, start, end, vec3_origin, vec3_origin, 0, CONTENTS_SOLID | CONTENTS_TERRAIN, qfalse );
	if ( tr.fraction >= 1.0f || tr.allsolid )
		return qfalse;

	VectorCopy( tr.endpos, pos );
	// an NPC stands on the floor, the rest sits on the surface
	if ( s->action == ST_A_NPC_SPAWN || s->action == ST_A_NPC_TELE ) {
		VectorMA( pos, 16.0f, tr.plane.normal, pos );
		if ( tr.plane.normal[2] > 0.7f )
			pos[2] = tr.endpos[2] + ST_GROUND_HEIGHT;
	}
	for ( int i = 0; i < 3; i++ )
		Com_sprintf( s->values[def->posField + i], ST_VALUE_LEN, "%i", ST_Round( pos[i] ) );
	if ( def->yawField >= 0 )
		Com_sprintf( s->values[def->yawField], ST_VALUE_LEN, "%i", ST_Round( AngleNormalize360( cl.viewangles[YAW] + 180.0f ) ) );
	return qtrue;
}

static qboolean ST_TriggerNear( const stStep_t *s ) {
	return (qboolean)!Q_stricmpn( s->values[0], "near", 4 );
}

static qboolean ST_TriggerEffect( const stStep_t *s ) {
	return (qboolean)!Q_stricmp( s->values[0], "near an effect" );
}

static qboolean ST_TriggerDoor( const stStep_t *s ) {
	return (qboolean)( !Q_stricmp( s->values[0], "door" ) || !Q_stricmp( s->values[0], "door or button" ) );
}

static qboolean ST_TriggerChat( const stStep_t *s ) {
	return (qboolean)!Q_stricmp( s->values[0], "someone says" );
}

// one of the map editor's use triggers used, by its name
static qboolean ST_TriggerUse( const stStep_t *s ) {
	return (qboolean)!Q_stricmp( s->values[0], "someone uses" );
}

// an area people are in, not words, a use or a door
static qboolean ST_TriggerArea( const stStep_t *s ) {
	return (qboolean)( !ST_TriggerChat( s ) && !ST_TriggerUse( s ) && !ST_TriggerDoor( s ) );
}

// a "when someone comes" or an If: an area, or what's said
static qboolean ST_Watches( const stStep_t *s ) {
	return (qboolean)( s->action == ST_A_TRIGGER || s->action == ST_A_IF );
}

// a field's label: an area's Which is the words, for what's said
static const char *ST_FieldLabel( const stStep_t *s, int field, const char *label ) {
	if ( field == 1 && ST_Watches( s ) && ST_TriggerChat( s ) )
		return "Words";
	if ( field == 1 && ST_Watches( s ) && ST_TriggerUse( s ) )
		return "Trigger";
	return label;
}

static qboolean ST_ChatSaid( const stStep_t *s, int since, char *by, int bySize );
static qboolean ST_UsedSince( const stStep_t *s, int since, char *by, int bySize );
static void ST_AddFar( struct stPerson_s *out, int *n, int max, const qboolean *seen );

// "*12" as the map's inline model 12, 0 if it isn't one
static int ST_ModelNum( const char *which ) {
	const int m = atoi( which[0] == '*' ? which + 1 : which );

	return ( m > 0 && m < CM_NumInlineModels() ) ? m : 0;
}

// a "when someone comes" step shows only what its kind of trigger uses
static qboolean ST_FieldShown( const stStep_t *s, int field ) {
	if ( s->action == ST_A_SOUND ) {
		if ( field == 3 )
			return (qboolean)!Q_stricmp( s->values[2], "player" );
		if ( field >= 4 )
			return (qboolean)!Q_stricmp( s->values[2], "spot" );
		return qtrue;
	}
	if ( s->action == ST_A_MUSIC )
		return (qboolean)( field != 1 || !Q_stricmp( s->values[0], "play" ) );
	if ( s->action == ST_A_SAY )
		return (qboolean)( field != 2 || !Q_stricmp( s->values[1], "a player" ) );
	if ( s->action == ST_A_OVERLAY )
		return (qboolean)( field == 0 || ( field == 1 && Q_stricmp( s->values[0], "remove all" ) ) || !Q_stricmp( s->values[0], "add" ) );
	if ( s->action == ST_A_SKY )
		return (qboolean)( field == 0 || Q_stricmp( s->values[0], "reset" ) );
	if ( s->action == ST_A_LOCK )
		return (qboolean)( field == 0 || ( field == 1 && Q_stricmp( s->values[0], "unlock all" ) ) || !Q_stricmp( s->values[0], "lock" ) );
	if ( s->action == ST_A_NOTIFY )
		return qtrue;
	if ( !ST_Watches( s ) )
		return qtrue;
	if ( s->action == ST_A_TRIGGER && field == 0 )
		return qfalse;
	if ( ST_TriggerChat( s ) || ST_TriggerUse( s ) )
		return (qboolean)( field <= 2 || ( field == 3 && !Q_stricmp( s->values[2], "named" ) ) );
	switch ( field ) {
	case 1:		return (qboolean)( !ST_TriggerNear( s ) || ST_TriggerEffect( s ) );
	case 2:		return (qboolean)!ST_TriggerDoor( s );
	case 3:		return (qboolean)( !ST_TriggerDoor( s ) && !Q_stricmp( s->values[2], "named" ) );
	case 4:
	case 5:
	case 6:
	case 7:		return ST_TriggerNear( s );
	case 8:		return (qboolean)( s->action == ST_A_IF || !ST_TriggerDoor( s ) );
	default:	return qtrue;
	}
}

static qboolean ST_HavePos( const stStep_t *s ) {
	const stActionDef_t *def = ST_Def( s );

	if ( ST_Watches( s ) && !ST_TriggerNear( s ) )
		return qtrue;
	if ( s->action == ST_A_SOUND && Q_stricmp( s->values[2], "spot" ) )
		return qtrue;

	if ( def->posField < 0 )
		return qtrue;
	for ( int i = 0; i < 3; i++ ) {
		if ( !s->values[def->posField + i][0] )
			return qfalse;
	}
	return qtrue;
}

static void ST_GetPos( const stStep_t *s, vec3_t pos ) {
	const stActionDef_t *def = ST_Def( s );

	for ( int i = 0; i < 3; i++ )
		pos[i] = ST_Number( s, def->posField + i );
}

static const char *ST_PosArgs( const stStep_t *s ) {
	vec3_t pos;

	ST_GetPos( s, pos );
	return va( "%i %i %i", ST_Round( pos[0] ), ST_Round( pos[1] ), ST_Round( pos[2] ) );
}

// a part of a hook's header: its separators would cut it up
// RPMod's server crashes on a centre print word longer than a line: the storyteller keeps them
// well under the client's own limit
#define ST_MAX_WORD		30

// the server breaks centre prints into lines and a line loses the colour before it, so each word
// gets the colour it's in: "^5this is ^7a test" goes as "^5this ^5is ^7a ^7test". Long words
// are broken up with spaces
static const char *ST_CenterText( const char *in ) {
	static char out[MAX_STRING_CHARS];
	char color[2] = { 0, 0 };
	int len = 0, run = 0;
	qboolean wordStart = qtrue;

	for ( ; *in && len < (int)sizeof( out ) - 5; in++ ) {
		if ( *in == ' ' ) {
			out[len++] = ' ';
			run = 0;
			wordStart = qtrue;
			continue;
		}
		if ( run >= ST_MAX_WORD ) {
			out[len++] = ' ';
			run = 0;
			wordStart = qtrue;
		}
		if ( wordStart && color[0] && !Q_IsColorString( in ) ) {
			out[len++] = color[0];
			out[len++] = color[1];
			run += 2;
		}
		wordStart = qfalse;
		if ( Q_IsColorString( in ) ) {
			color[0] = in[0];
			color[1] = in[1];
			out[len++] = *in++;
			run++;
		}
		out[len++] = *in;
		run++;
	}
	out[len] = '\0';
	return out;
}

static qboolean ST_HasEmote( const stStep_t *s ) {
	return (qboolean)( s->values[3][0] && Q_stricmp( s->values[3], "none" ) );
}

// sent as cmd: hooks, which only players with this client see
static qboolean ST_Experimental( stAction_t action ) {
	switch ( action ) {
	case ST_A_SKY:
	case ST_A_NPC_TALK:
	case ST_A_NPC_WALK:
	case ST_A_OBJECTIVE:
	case ST_A_CINEMATIC:
	case ST_A_CIN_START:
	case ST_A_CAM_SHOT:
	case ST_A_CAM_FOV:
	case ST_A_CIN_BARS:
	case ST_A_CIN_END:
		return qtrue;
	default:
		return qfalse;
	}
}

// the camera steps from before the cinematic manager: stories that have them still play them,
// but they're no longer in the list to add
static qboolean ST_OldAction( int action ) {
	return (qboolean)( action == ST_A_CIN_START || action == ST_A_CAM_SHOT || action == ST_A_CAM_FOV
		|| action == ST_A_CIN_BARS || action == ST_A_CIN_END );
}

// an action from the list can go in the story
static qboolean ST_CanAdd( stAction_t action ) {
	if ( !ST_Experimental( action ) || CL_RpHooks_Experimental() )
		return qtrue;
	ST_Message( S_COLOR_YELLOW RP_EXPERIMENTAL_OFF ", top right" );
	return qfalse;
}

// the NPC manager's walk along a route, or why it can't; NULL either way when nothing walks
static const char *ST_WalkCommand( const char *target, const char *route, const char *pace, qboolean loop, const char **error ) {
	static char cmd[MAX_STRING_CHARS], why[MAX_STRING_CHARS];
	vec3_t pts[NW_MAX_ROUTE_POINTS];
	float speed;

	*error = NULL;
	if ( !target[0] ) {
		*error = "Type the NPC's targetname or number";
		return NULL;
	}
	if ( !route[0] ) {
		*error = "Pick the route it walks";
		return NULL;
	}
	if ( CL_NpcWalk_LoadRoute( route, pts, NW_MAX_ROUTE_POINTS, &speed ) <= 0 ) {
		Com_sprintf( why, sizeof( why ), "No route called %s: make it in the NPC manager's Walk row", route );
		*error = why;
		return NULL;
	}
	Com_sprintf( cmd, sizeof( cmd ), "npcmanager walk %s %s %s%s", target, route, pace, loop ? " loop" : "" );
	return cmd;
}

// what a step sends after its command, NULL if nothing; *local if it's a console command
static const char *ST_StepExtra( const stStep_t *s, qboolean *local ) {
	static char cmd[MAX_STRING_CHARS];

	*local = qfalse;
	if ( s->action != ST_A_NPC_TALK || !ST_HasEmote( s ) )
		return NULL;
	Com_sprintf( cmd, sizeof( cmd ), "npc emote %s %s", s->values[3], s->values[0] );
	return cmd;
}

// a connected player's name, colours and all
static const char *ST_PlayerName( int num ) {
	const char *info;

	if ( num < 0 || num >= MAX_CLIENTS )
		return "";
	info = cl.gameState.stringData + cl.gameState.stringOffsets[CS_PLAYERS + num];
	return info[0] ? Info_ValueForKey( info, "n" ) : "";
}

// a player by their number, or by their name or a part of it, colours left out; -1 if no one
static int ST_FindPlayer( const char *who ) {
	char want[MAX_STRING_CHARS], name[MAX_STRING_CHARS];
	int found = -1;

	if ( Q_isanumber( who ) && !strchr( who, '.' ) ) {
		const int num = atoi( who );

		return ST_PlayerName( num )[0] ? num : -1;
	}
	Q_strncpyz( want, who, sizeof( want ) );
	Q_StripColor( want );
	Q_strlwr( want );
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		Q_strncpyz( name, ST_PlayerName( i ), sizeof( name ) );
		if ( !name[0] )
			continue;
		Q_StripColor( name );
		Q_strlwr( name );
		// the whole name first, else the first it's part of
		if ( !strcmp( name, want ) )
			return i;
		if ( found < 0 && strstr( name, want ) )
			found = i;
	}
	return found;
}

// what a step sends: a server command, or with *local a console command; NULL and why if it can't
static const char *ST_StepCommand( const stStep_t *s, qboolean *local, const char **error ) {
	static char cmd[MAX_STRING_CHARS];
	const char ( *v )[ST_VALUE_LEN] = s->values;
	const char *value;
	vec3_t pos;

	*local = qfalse;
	*error = NULL;
	cmd[0] = '\0';

	if ( ST_Experimental( s->action ) && !CL_RpHooks_Experimental() ) {
		*error = RP_EXPERIMENTAL_OFF;
		return NULL;
	}
	if ( !ST_HavePos( s ) ) {
		*error = "Set the spot: X, Y and Z";
		return NULL;
	}

	switch ( s->action ) {
	case ST_A_WAIT:
	case ST_A_PAUSE:
		return NULL;

	case ST_A_ELSE:
	case ST_A_ENDIF:
		return NULL;

	case ST_A_IF:
	case ST_A_TRIGGER:
		if ( ST_TriggerChat( s ) ) {
			if ( !v[1][0] ) {
				*error = "Type the words to listen for, | between them";
				return NULL;
			}
		} else if ( ST_TriggerUse( s ) ) {
			if ( !v[1][0] ) {
				*error = "Type the name of the use trigger, as the map editor's Triggers tab has it";
				return NULL;
			}
		} else if ( !ST_TriggerNear( s ) && !ST_ModelNum( v[1] ) ) {
			*error = ST_TriggerDoor( s ) ? "Pick the door: aim at it, or pick it from the list"
				: "Pick the trigger: stand in it, or pick it from the list";
			return NULL;
		}
		if ( !ST_TriggerDoor( s ) && !Q_stricmp( v[2], "named" ) && !v[3][0] ) {
			*error = "Type the player's or the NPC's name, or the NPC's number";
			return NULL;
		}
		return NULL;

	case ST_A_NOTIFY: {
		// rpnotify <player|all|-1> <seconds> <message>, <timer> counting down in it
		char text[ST_VALUE_LEN + 16];

		if ( ST_Number( s, 1 ) <= 0 ) {
			*error = "Give it some seconds";
			return NULL;
		}
		if ( !Q_stricmpn( v[2], "cmd:", 4 ) ) {
			*error = "Text starting with cmd: is a hook, not a notification";
			return NULL;
		}
		Q_strncpyz( text, v[2], sizeof( text ) );
		if ( Q_stricmp( v[3], "no" ) && !Q_stristr( text, "<timer>" ) ) {
			if ( !text[0] )
				Q_strncpyz( text, "<timer>", sizeof( text ) );
			else if ( !Q_stricmp( v[3], "before the text" ) )
				Com_sprintf( text, sizeof( text ), "<timer> %s", v[2] );
			else
				Q_strcat( text, sizeof( text ), " <timer>" );
		}
		if ( !text[0] ) {
			*error = "Type the text, or have it count down";
			return NULL;
		}
		Com_sprintf( cmd, sizeof( cmd ), "rpnotify %s %g %s", v[0][0] ? v[0] : "all", ST_Number( s, 1 ), text );
		return cmd;
	}

	case ST_A_SAY:
		if ( !v[0][0] ) {
			*error = "Type what to say";
			return NULL;
		}
		if ( !Q_stricmp( v[1], "a player" ) ) {
			static char why[MAX_STRING_CHARS];
			int num;

			// shown as their chat, which only this client's hook can do
			if ( !CL_RpHooks_Experimental() ) {
				*error = RP_EXPERIMENTAL_OFF;
				return NULL;
			}
			if ( !v[2][0] ) {
				*error = "Type the player's number or a part of their name";
				return NULL;
			}
			if ( ( num = ST_FindPlayer( v[2] ) ) < 0 ) {
				Com_sprintf( why, sizeof( why ), "No player %s on the server", v[2] );
				*error = why;
				return NULL;
			}
			return CL_RpHooks_NpcTalkCommand( va( "%i", num ), ST_PlayerName( num ), "^7", "", v[0] );
		}
		Com_sprintf( cmd, sizeof( cmd ), "say %s", v[0] );
		return cmd;

	case ST_A_CENTER:
		if ( !v[2][0] ) {
			*error = "Type the text to show";
			return NULL;
		}
		Com_sprintf( cmd, sizeof( cmd ), "rpcentersay %s %i %s", v[0][0] ? v[0] : "all", ST_Round( ST_Number( s, 1 ) ), ST_CenterText( v[2] ) );
		return cmd;

	case ST_A_NPC_TALK:
		if ( !v[0][0] ) {
			*error = "Type the NPC's targetname";
			return NULL;
		}
		if ( !v[4][0] ) {
			*error = "Type what the NPC says";
			return NULL;
		}
		return CL_RpHooks_NpcTalkCommand( v[0], v[1], v[2], ST_HasEmote( s ) ? v[3] : "", v[4] );

	case ST_A_OBJECTIVE:
		if ( !v[1][0] && Q_stricmp( v[0], "clear" ) && Q_stricmp( v[0], "title" ) ) {
			*error = !Q_stricmp( v[0], "add" ) ? "Type the objective" : "Type the objective's number or text";
			return NULL;
		}
		if ( v[1][0] )
			Com_sprintf( cmd, sizeof( cmd ), "rpnotify -1 1 cmd:objective/%s: %s", v[0], v[1] );
		else
			Com_sprintf( cmd, sizeof( cmd ), "rpnotify -1 1 cmd:objective/%s", v[0] );
		return cmd;

	case ST_A_CINEMATIC: {
		static char why[MAX_STRING_CHARS];

		if ( !v[0][0] ) {
			*error = "Name the cinematic, then Edit to make it";
			return NULL;
		}
		if ( CL_Cinematic_PlayMs( v[0] ) < 0 ) {
			Com_sprintf( why, sizeof( why ), "No cinematic called %s yet: Edit makes it", v[0] );
			*error = why;
			return NULL;
		}
		// it sends itself, a part at a time
		*local = qtrue;
		Com_sprintf( cmd, sizeof( cmd ), "cinematicmanager send %s", v[0] );
		return cmd;
	}

	case ST_A_CIN_START:
		Com_sprintf( cmd, sizeof( cmd ), "rpnotify -1 1 cmd:cin/start%s%s", !Q_stricmp( v[0], "show" ) ? "/hud" : "",
			!Q_stricmp( v[1], "on" ) ? "/bars" : "" );
		return cmd;

	case ST_A_CAM_SHOT: {
		static char why[MAX_STRING_CHARS];
		char hook[MAX_STRING_CHARS];

		if ( !v[0][0] ) {
			*error = "Name the shot, then Edit shot to make it";
			return NULL;
		}

		if ( !CL_RpCinematic_ShotHook( v[0], hook, sizeof( hook ), NULL ) ) {
			Com_sprintf( why, sizeof( why ), "No shot called %s yet: Edit shot makes it", v[0] );
			*error = why;
			return NULL;
		}
		Com_sprintf( cmd, sizeof( cmd ), "rpnotify -1 1 cmd:%s", hook );
		return cmd;
	}

	case ST_A_CAM_FOV:
		if ( ST_Number( s, 0 ) <= 0 ) {
			*error = "Set the FOV, e.g. 40 to zoom in, 80 for the usual";
			return NULL;
		}
		Com_sprintf( cmd, sizeof( cmd ), "rpnotify -1 1 cmd:cin/fov/%g/%g", ST_Number( s, 0 ), ST_Number( s, 1 ) );
		return cmd;

	case ST_A_CIN_BARS:
		Com_sprintf( cmd, sizeof( cmd ), "rpnotify -1 1 cmd:cin/bars/%s/%g", !Q_stricmp( v[0], "off" ) ? "off" : "on", ST_Number( s, 1 ) );
		return cmd;

	case ST_A_CIN_END:
		return "rpnotify -1 1 cmd:cin/end";

	case ST_A_COMMAND:
		if ( !v[0][0] ) {
			*error = "Type the command";
			return NULL;
		}
		*local = qtrue;
		Q_strncpyz( cmd, v[0], sizeof( cmd ) );
		return cmd;

	case ST_A_NPC_SPAWN:
		if ( !v[0][0] ) {
			*error = "Type the NPC type, e.g. stormtrooper";
			return NULL;
		}
		ST_GetPos( s, pos );
		return CL_NpcManager_SpawnCommand( v[0], (qboolean)!Q_stricmp( v[3], "yes" ), v[1], v[2], v[4], v[5], pos,
			ST_Round( AngleNormalize360( ST_Number( s, 9 ) ) ) );

	case ST_A_NPC_TELE:
		if ( !v[0][0] ) {
			*error = "Type the target: a targetname, a number or all";
			return NULL;
		}
		Com_sprintf( cmd, sizeof( cmd ), "npc tele %s %i %s", ST_PosArgs( s ), ST_Round( AngleNormalize360( ST_Number( s, 4 ) ) ), v[0] );
		return cmd;

	case ST_A_NPC_WALK:
		value = ST_WalkCommand( v[0], v[1], v[2], (qboolean)!Q_stricmp( v[3], "yes" ), error );
		if ( !value )
			return NULL;
		*local = qtrue;
		Q_strncpyz( cmd, value, sizeof( cmd ) );
		return cmd;

	case ST_A_NPC_CMD:
		if ( !v[0][0] ) {
			*error = "Type the target: a targetname, a number or all";
			return NULL;
		}
		value = v[1];
		if ( ( !Q_stricmp( value, "emote" ) || !Q_stricmp( value, "dialog" ) ) && !v[2][0] ) {
			*error = !Q_stricmp( value, "emote" ) ? "Type the emote, e.g. emsit" : "Type the dialog";
			return NULL;
		}
		if ( !Q_stricmp( value, "kill" ) )					Com_sprintf( cmd, sizeof( cmd ), "npc kill %s", v[0] );
		else if ( !Q_stricmp( value, "freeze" ) )			Com_sprintf( cmd, sizeof( cmd ), "npc freeze %s 1", v[0] );
		else if ( !Q_stricmp( value, "unfreeze" ) )			Com_sprintf( cmd, sizeof( cmd ), "npc freeze %s 0", v[0] );
		else if ( !Q_stricmp( value, "emote" ) )			Com_sprintf( cmd, sizeof( cmd ), "npc emote %s %s", v[2], v[0] );
		else if ( !Q_stricmp( value, "stop emote" ) )		Com_sprintf( cmd, sizeof( cmd ), "npc emote none %s", v[0] );
		else if ( !Q_stricmp( value, "follow me" ) )		Com_sprintf( cmd, sizeof( cmd ), "npc follow %i %s", clc.clientNum, v[0] );
		else if ( !Q_stricmp( value, "stop following" ) )	Com_sprintf( cmd, sizeof( cmd ), "npc follow none %s", v[0] );
		else if ( !Q_stricmp( value, "dialog" ) )			Com_sprintf( cmd, sizeof( cmd ), "npc dialog %s %s", v[2], v[0] );
		else if ( !Q_stricmp( value, "no dialog" ) )		Com_sprintf( cmd, sizeof( cmd ), "npc dialog none %s", v[0] );
		else if ( !Q_stricmp( value, "hologram" ) )			Com_sprintf( cmd, sizeof( cmd ), "npc hologram normal %s", v[0] );
		else if ( !Q_stricmp( value, "projector" ) )		Com_sprintf( cmd, sizeof( cmd ), "npc hologram projector %s", v[0] );
		else												Com_sprintf( cmd, sizeof( cmd ), "npc hologram off %s", v[0] );
		return cmd;

	case ST_A_MODEL:
		if ( !v[0][0] ) {
			*error = "Type the model, e.g. models/map_objects/imperial/crate_banded.md3";
			return NULL;
		}
		Com_sprintf( cmd, sizeof( cmd ), "rpmodel add %s %s %i %i %i %i %i", v[0], ST_PosArgs( s ),
			ST_Round( AngleNormalize360( ST_Number( s, 4 ) ) ), ST_Round( AngleNormalize360( ST_Number( s, 5 ) ) ),
			ST_Round( AngleNormalize360( ST_Number( s, 6 ) ) ), v[7][0] ? ST_Round( ST_Number( s, 7 ) ) : 100,
			Q_stricmp( v[8], "no" ) ? 1 : 0 );
		return cmd;

	case ST_A_EFFECT:
		if ( !v[0][0] ) {
			*error = "Type the effect, e.g. env/fire";
			return NULL;
		}
		Com_sprintf( cmd, sizeof( cmd ), "rpeffect %s %s %s %i %i %i", !Q_stricmp( v[1], "loop" ) ? "add" : "once", v[0], ST_PosArgs( s ),
			ST_Round( AngleNormalize360( ST_Number( s, 5 ) ) ), ST_Round( AngleNormalize360( ST_Number( s, 6 ) ) ),
			ST_Round( AngleNormalize360( ST_Number( s, 7 ) ) ) );
		return cmd;

	case ST_A_SCREEN:
		value = v[0];
		if ( !Q_stricmp( value, "fade out" ) )			Q_strncpyz( cmd, "rpeffect fadeout", sizeof( cmd ) );
		else if ( !Q_stricmp( value, "fade in" ) )		Q_strncpyz( cmd, "rpeffect remove fadeout", sizeof( cmd ) );
		else if ( !Q_stricmp( value, "alarm" ) )		Q_strncpyz( cmd, "rpeffect alarm", sizeof( cmd ) );
		else if ( !Q_stricmp( value, "stop alarm" ) )	Q_strncpyz( cmd, "rpeffect remove alarm", sizeof( cmd ) );
		else {
			if ( !v[1][0] || !v[2][0] ) {
				*error = "Set the shake's intensity and duration";
				return NULL;
			}
			Com_sprintf( cmd, sizeof( cmd ), "rpeffect camerashake %s %s", v[1], v[2] );
		}
		return cmd;

	case ST_A_SOUND: {
		const char *verb = !Q_stricmp( v[1], "loop" ) ? "add" : !Q_stricmp( v[1], "local" ) ? "local" : "once";
		const int len = strlen( v[0] );

		if ( !v[0][0] ) {
			*error = "Type the sound, e.g. ambience/thunder1.wav, or pick it in the sound manager";
			return NULL;
		}
		// the engine plays an mp3 sound once and no more, so a looping one would go quiet
		if ( !strcmp( verb, "add" ) && len > 4 && !Q_stricmp( v[0] + len - 4, ".mp3" ) ) {
			*error = "An mp3 only plays once in this engine: loop a .wav, or play this once";
			return NULL;
		}
		if ( !Q_stricmp( v[2], "player" ) ) {
			if ( !v[3][0] ) {
				*error = "Type the player's number";
				return NULL;
			}
			Com_sprintf( cmd, sizeof( cmd ), "rpsound %s %s %s", verb, v[0], v[3] );
		} else if ( !Q_stricmp( v[2], "spot" ) ) {
			if ( strcmp( verb, "add" ) ) {
				*error = "Only a looping sound plays at a spot";
				return NULL;
			}
			Com_sprintf( cmd, sizeof( cmd ), "rpsound add %s %s", v[0], ST_PosArgs( s ) );
		} else if ( !Q_stricmp( v[2], "me" ) ) {
			if ( !strcmp( verb, "add" ) ) {
				*error = "A looping sound goes to everyone, a player or a spot";
				return NULL;
			}
			Com_sprintf( cmd, sizeof( cmd ), "rpsound %s %s", verb, v[0] );
		} else {
			if ( !strcmp( verb, "local" ) ) {
				*error = "A local sound goes to you or a player";
				return NULL;
			}
			Com_sprintf( cmd, sizeof( cmd ), "rpsound %s %s all", verb, v[0] );
		}
		return cmd;
	}

	case ST_A_MUSIC:
		if ( !Q_stricmp( v[0], "map default" ) )
			return "rpmusic default";
		if ( !Q_stricmp( v[0], "none" ) )
			return "rpmusic none";
		if ( !v[1][0] ) {
			*error = "Type the music, e.g. kejim/impbaseb_explore.mp3, or pick it in the music manager";
			return NULL;
		}
		Com_sprintf( cmd, sizeof( cmd ), "rpmusic %s", v[1] );
		return cmd;

	case ST_A_SHADER:
		if ( !v[0][0] || !v[1][0] ) {
			*error = "Type the shader to swap, and the one to put instead";
			return NULL;
		}
		if ( v[2][0] )
			Com_sprintf( cmd, sizeof( cmd ), "rpshader remap %s %s %s", v[0], v[1], v[2] );
		else
			Com_sprintf( cmd, sizeof( cmd ), "rpshader remap %s %s", v[0], v[1] );
		return cmd;

	case ST_A_LOCK:
		// rplock add <target> <rank> [name], rplock remove <target|all>
		if ( !Q_stricmp( v[0], "unlock all" ) )
			return "rplock remove all";
		if ( !v[1][0] || strchr( v[1], ' ' ) ) {
			*error = "Type the door's or trigger's targetname or entity number, or pick it in the map editor";
			return NULL;
		}
		if ( !Q_stricmp( v[0], "unlock" ) ) {
			Com_sprintf( cmd, sizeof( cmd ), "rplock remove %s", v[1] );
			return cmd;
		}
		Com_sprintf( cmd, sizeof( cmd ), "rplock add %s %i%s%s", v[1], Com_Clampi( -1, 15, (int)ST_Number( s, 2 ) ), v[3][0] ? " " : "", v[3] );
		return cmd;

	case ST_A_OVERLAY: {
		// rpshader overlay add <shader> [fade] [x] [y] [width] [height], up to the last one given
		static const char *defaults[] = { "0", "0", "0", "640", "480" };
		int last = -1;

		if ( !Q_stricmp( v[0], "remove all" ) )
			return "rpshader overlay remove all";
		if ( !v[1][0] ) {
			*error = !Q_stricmp( v[0], "remove" ) ? "Type the overlay's shader, or its number in the list" : "Type the shader, or pick it in the shader manager";
			return NULL;
		}
		if ( !Q_stricmp( v[0], "remove" ) ) {
			Com_sprintf( cmd, sizeof( cmd ), "rpshader overlay remove %s", v[1] );
			return cmd;
		}
		for ( int i = 0; i < 5; i++ ) {
			if ( v[2 + i][0] )
				last = i;
		}
		Com_sprintf( cmd, sizeof( cmd ), "rpshader overlay add %s", v[1] );
		for ( int i = 0; i <= last; i++ )
			Q_strcat( cmd, sizeof( cmd ), va( " %s", v[2 + i][0] ? v[2 + i] : defaults[i] ) );
		return cmd;
	}

	case ST_A_SKY: {
		vec3_t tint = { 255, 255, 255 }, shift = { 0, 0, 0 };

		if ( !Q_stricmp( v[0], "reset" ) )
			return "rpnotify -1 1 cmd:sky/off";
		if ( v[1][0] && sscanf( v[1], "%f %f %f", &tint[0], &tint[1], &tint[2] ) != 3 ) {
			*error = "Type the colour as red green blue, 0 to 255, e.g. 255 180 120";
			return NULL;
		}
		if ( v[6][0] && sscanf( v[6], "%f %f %f", &shift[0], &shift[1], &shift[2] ) != 3 ) {
			*error = "Type the move as x y z, e.g. 0 0 128";
			return NULL;
		}
		Com_sprintf( cmd, sizeof( cmd ), "rpnotify -1 1 cmd:sky/%g %g %g/%g/%g %g %g/%g %g %g", Com_Clamp( 0, 255, tint[0] ),
			Com_Clamp( 0, 255, tint[1] ), Com_Clamp( 0, 255, tint[2] ), ST_Number( s, 2 ), ST_Number( s, 4 ), ST_Number( s, 3 ),
			ST_Number( s, 5 ), shift[0], shift[1], shift[2] );
		return cmd;
	}

	default:
		return NULL;
	}
}

/*
===============================================================================

When someone comes

===============================================================================
*/

#define ST_MAX_MAPENTS	1024
#define ST_MAX_ARMED	16
#define ST_MAX_ARM_TIMES	99

// the map's brush entities, read from its entity string: triggers, doors, buttons...
typedef struct {
	int			model;
	char		classname[48];
	char		targetname[48];
	qboolean	trigger;
	vec3_t		mins, maxs;
} stMapEnt_t;

static struct {
	char		map[MAX_QPATH];
	stMapEnt_t	ents[ST_MAX_MAPENTS];
	int			count;
} stMap;

// a story armed: a copy of it plays each time its first step is set off
typedef struct {
	char		name[MAX_QPATH];
	char		map[MAX_QPATH];		// it goes with the map
	stStep_t	*steps;
	int			numSteps;
	int			limit;				// times it plays, 0 = every time
	int			times;				// set off so far
	stRun_t		run;
} stArmed_t;

static stArmed_t	stArmed[ST_MAX_ARMED];
static int			stNumArmed;

// a player or an NPC the snapshot has, or a player out of sight we know of
typedef struct stPerson_s {
	int			num;
	qboolean	npc;
	vec3_t		org;
} stPerson_t;

static void ST_ScanMap( void ) {
	char classname[48], targetname[48], model[16], key[64];
	const char *p, *token;

	if ( !cl.mapname[0] || !Q_stricmp( stMap.map, cl.mapname ) )
		return;
	Q_strncpyz( stMap.map, cl.mapname, sizeof( stMap.map ) );
	stMap.count = 0;

	p = CM_EntityString();
	while ( p ) {
		token = COM_Parse( &p );
		if ( !token[0] )
			break;
		if ( strcmp( token, "{" ) )
			continue;
		classname[0] = targetname[0] = model[0] = '\0';
		for ( ;; ) {
			token = COM_Parse( &p );
			if ( !token[0] || !strcmp( token, "}" ) )
				break;
			Q_strncpyz( key, token, sizeof( key ) );
			token = COM_Parse( &p );
			if ( !Q_stricmp( key, "classname" ) )
				Q_strncpyz( classname, token, sizeof( classname ) );
			else if ( !Q_stricmp( key, "targetname" ) )
				Q_strncpyz( targetname, token, sizeof( targetname ) );
			else if ( !Q_stricmp( key, "model" ) )
				Q_strncpyz( model, token, sizeof( model ) );
		}
		if ( model[0] != '*' || !ST_ModelNum( model ) || stMap.count >= ST_MAX_MAPENTS )
			continue;

		stMapEnt_t *e = &stMap.ents[stMap.count++];
		e->model = ST_ModelNum( model );
		Q_strncpyz( e->classname, classname, sizeof( e->classname ) );
		Q_strncpyz( e->targetname, targetname, sizeof( e->targetname ) );
		e->trigger = (qboolean)!Q_stricmpn( classname, "trigger_", 8 );
		CM_ModelBounds( CM_InlineModel( e->model ), e->mins, e->maxs );
	}
}

static const stMapEnt_t *ST_FindEnt( int model ) {
	ST_ScanMap();
	for ( int i = 0; i < stMap.count; i++ ) {
		if ( stMap.ents[i].model == model )
			return &stMap.ents[i];
	}
	return NULL;
}

// "trigger_multiple door1 (*12)"
static const char *ST_EntLabel( const char *which ) {
	const int m = ST_ModelNum( which );
	const stMapEnt_t *e = m ? ST_FindEnt( m ) : NULL;

	if ( !e )
		return which[0] ? which : "?";
	return va( "%s%s%s (*%i)", e->classname, e->targetname[0] ? " " : "", e->targetname, e->model );
}

static const char *ST_WhoText( const stStep_t *s ) {
	const char *who = s->values[2];

	if ( !Q_stricmp( who, "any NPC" ) )		return "an NPC";
	if ( !Q_stricmp( who, "anyone" ) )		return "anyone";
	if ( !Q_stricmp( who, "me" ) )			return "you";
	if ( !Q_stricmp( who, "named" ) )		return s->values[3][0] ? s->values[3] : "someone named";
	return "a player";
}

// more than one of who's counted
static const char *ST_WhoPlural( const stStep_t *s ) {
	const char *who = s->values[2];

	if ( !Q_stricmp( who, "any NPC" ) )		return "NPCs";
	if ( !Q_stricmp( who, "anyone" ) )		return "people";
	if ( !Q_stricmp( who, "me" ) )			return "of you";
	if ( !Q_stricmp( who, "named" ) )		return va( "named %s", s->values[3][0] ? s->values[3] : "?" );
	return "players";
}

// where an area is, for a summary
static const char *ST_AreaText( const stStep_t *s ) {
	if ( ST_TriggerEffect( s ) )
		return va( "near %s", s->values[1][0] ? s->values[1] : "an effect" );
	if ( ST_TriggerNear( s ) )
		return va( "within %g of %s %s %s", ST_Number( s, 4 ) > 0 ? ST_Number( s, 4 ) : 96.0f, s->values[5], s->values[6], s->values[7] );
	return va( "in %s", ST_EntLabel( s->values[1] ) );
}

static const char *ST_IfSummary( const stStep_t *s ) {
	if ( ST_TriggerChat( s ) )
		return va( "If %s said \"%s\"", ST_WhoText( s ), s->values[1] );
	if ( ST_TriggerUse( s ) )
		return va( "If %s used %s", ST_WhoText( s ), s->values[1][0] ? s->values[1] : "a use trigger" );
	return va( "If %s %i %s %s", s->values[8], (int)ST_Number( s, 9 ), ST_WhoPlural( s ), ST_AreaText( s ) );
}

static const char *ST_TriggerSummary( const stStep_t *s ) {
	const float stay = ST_Number( s, 8 );

	if ( ST_TriggerDoor( s ) )
		return va( "When %s moves", ST_EntLabel( s->values[1] ) );
	if ( ST_TriggerChat( s ) )
		return va( "When %s says \"%s\"", ST_WhoText( s ), s->values[1] );
	if ( ST_TriggerUse( s ) )
		return va( "When %s uses %s", ST_WhoText( s ), s->values[1][0] ? s->values[1] : "a use trigger" );
	if ( stay > 0 )
		return va( "When %s stays %s for %g s", ST_WhoText( s ), ST_AreaText( s ), stay );
	if ( ST_TriggerEffect( s ) )
		return va( "When %s comes near %s", ST_WhoText( s ), s->values[1][0] ? s->values[1] : "an effect" );
	if ( ST_TriggerNear( s ) )
		return va( "When %s comes within %g of %s %s %s", ST_WhoText( s ), ST_Number( s, 4 ) > 0 ? ST_Number( s, 4 ) : 96.0f,
			s->values[5], s->values[6], s->values[7] );
	return va( "When %s comes into %s", ST_WhoText( s ), ST_EntLabel( s->values[1] ) );
}

// a player's box, near enough for an NPC's too
static const vec3_t stPersonMins = { -15, -15, -24 };
static const vec3_t stPersonMaxs = { 15, 15, 40 };

static qboolean ST_InTrigger( int model, const vec3_t org ) {
	trace_t tr;

	CM_BoxTrace( &tr, org, org, stPersonMins, stPersonMaxs, CM_InlineModel( model ), -1, qfalse );
	return (qboolean)( tr.startsolid || tr.allsolid );
}

// the smallest of the map's triggers the player stands in, 0 if none
static int ST_TriggerHere( void ) {
	float best = 0;
	int model = 0;

	ST_ScanMap();
	for ( int i = 0; i < stMap.count; i++ ) {
		const stMapEnt_t *e = &stMap.ents[i];
		vec3_t size;
		float volume;

		if ( !e->trigger || !ST_InTrigger( e->model, cl.snap.ps.origin ) )
			continue;
		VectorSubtract( e->maxs, e->mins, size );
		volume = size[0] * size[1] * size[2];
		if ( !model || volume < best ) {
			best = volume;
			model = e->model;
		}
	}
	return model;
}

static const entityState_t *ST_SnapEntity( int i ) {
	return &cl.parseEntities[( cl.snap.parseEntitiesNum + i ) & ( MAX_PARSE_ENTITIES - 1 )];
}

// the door, button or other brush entity the crosshair is on, 0 if none
static int ST_AimedMover( void ) {
	vec3_t start, end, forward;
	trace_t tr;
	float best;
	int model = 0;

	VectorCopy( cl.snap.ps.origin, start );
	start[2] += cl.snap.ps.viewheight;
	AngleVectors( cl.viewangles, forward, NULL, NULL );
	VectorMA( start, 8192, forward, end );
	CM_BoxTrace( &tr, start, end, vec3_origin, vec3_origin, 0, MASK_SOLID, qfalse );
	best = tr.fraction;

	ST_ScanMap();
	for ( int i = 0; i < cl.snap.numEntities; i++ ) {
		const entityState_t *es = ST_SnapEntity( i );
		const stMapEnt_t *e;

		if ( es->solid != SOLID_BMODEL || !ST_ModelNum( va( "%i", es->modelindex ) ) )
			continue;
		e = ST_FindEnt( es->modelindex );
		if ( e && e->trigger )
			continue;
		CM_TransformedBoxTrace( &tr, start, end, vec3_origin, vec3_origin, CM_InlineModel( es->modelindex ), -1,
			es->pos.trBase, es->apos.trBase, qfalse );
		if ( tr.fraction <= best && tr.fraction < 1.0f ) {
			best = tr.fraction;
			model = es->modelindex;
		}
	}
	return model;
}

// the players and NPCs this client can see, the player first, then the players out of sight
// whose place we've been told or asked for
static int ST_People( stPerson_t *out, int max ) {
	qboolean seen[MAX_CLIENTS] = { qfalse };
	int n = 0;

	if ( !cl.snap.valid )
		return 0;
	seen[cl.snap.ps.clientNum] = qtrue;
	if ( cl.snap.ps.pm_type != PM_SPECTATOR ) {
		out[n].num = cl.snap.ps.clientNum;
		out[n].npc = qfalse;
		VectorCopy( cl.snap.ps.origin, out[n].org );
		n++;
	}
	for ( int i = 0; i < cl.snap.numEntities && n < max; i++ ) {
		const entityState_t *es = ST_SnapEntity( i );

		if ( es->number == cl.snap.ps.clientNum )
			continue;
		if ( es->eType == ET_PLAYER && es->number < MAX_CLIENTS ) {
			out[n].npc = qfalse;
			seen[es->number] = qtrue;
		} else if ( es->eType == ET_NPC ) {
			out[n].npc = qtrue;
		} else {
			continue;
		}
		out[n].num = es->number;
		VectorCopy( es->pos.trBase, out[n].org );
		n++;
	}
	ST_AddFar( out, &n, max, seen );
	return n;
}

// a player's name, an NPC's targetname or type
static void ST_PersonName( const stPerson_t *p, char *out, int size ) {
	if ( !p->npc ) {
		const char *cs = cl.gameState.stringData + cl.gameState.stringOffsets[CS_PLAYERS + p->num];
		Q_strncpyz( out, Info_ValueForKey( cs, "n" ), size );
		return;
	}
	for ( int i = 0; i < CL_NpcManager_NumNpcs(); i++ ) {
		const char *type, *name;
		int num;

		if ( CL_NpcManager_GetNpc( i, &num, &type, &name ) && num == p->num ) {
			Q_strncpyz( out, name[0] ? name : type, size );
			return;
		}
	}
	Com_sprintf( out, size, "NPC %i", p->num );
}

// a name as typed: a number is an NPC's, a name is matched without colours, in part from 3 letters
static qboolean ST_NameIs( const stPerson_t *p, const char *want ) {
	char name[MAX_QPATH], typed[MAX_QPATH];

	if ( !want[0] )
		return qfalse;
	if ( Q_isanumber( want ) )
		return (qboolean)( p->npc && p->num == atoi( want ) );
	ST_PersonName( p, name, sizeof( name ) );
	Q_StripColor( name );
	Q_strncpyz( typed, want, sizeof( typed ) );
	Q_StripColor( typed );
	if ( !Q_stricmp( name, typed ) )
		return qtrue;
	return (qboolean)( strlen( typed ) >= 3 && Q_stristr( name, typed ) );
}

static qboolean ST_Counts( const stStep_t *s, const stPerson_t *p ) {
	const char *who = s->values[2];

	if ( !Q_stricmp( who, "any NPC" ) )		return p->npc;
	if ( !Q_stricmp( who, "anyone" ) )		return qtrue;
	if ( !Q_stricmp( who, "me" ) )			return (qboolean)( !p->npc && p->num == cl.snap.ps.clientNum );
	if ( !Q_stricmp( who, "named" ) )		return ST_NameIs( p, s->values[3] );
	return (qboolean)!p->npc;
}

static qboolean ST_Inside( const stStep_t *s, const vec3_t org ) {
	if ( ST_TriggerNear( s ) ) {
		vec3_t pos;
		const float radius = ST_Number( s, 4 ) > 0 ? ST_Number( s, 4 ) : 96.0f;

		ST_GetPos( s, pos );
		return (qboolean)( Distance( org, pos ) <= radius );
	}
	return ST_InTrigger( ST_ModelNum( s->values[1] ), org );
}

static const entityState_t *ST_FindMover( int model ) {
	for ( int i = 0; i < cl.snap.numEntities; i++ ) {
		const entityState_t *es = ST_SnapEntity( i );

		if ( es->solid == SOLID_BMODEL && es->modelindex == model )
			return es;
	}
	return NULL;
}

static int ST_CommandGap( void );

// a "when someone comes" step, looked at once a frame: qtrue when someone it counts has
// come in since the last look, or the door started a move. The first look only sees
// who is in already
static qboolean ST_TriggerFired( stRun_t *r, const stStep_t *s ) {
	stPerson_t people[ST_MAX_PEOPLE];
	int now[ST_MAX_PEOPLE], nowSince[ST_MAX_PEOPLE], numNow = 0, n, stay;
	qboolean fired = qfalse;

	if ( !cl.snap.valid )
		return qfalse;
	r->by[0] = '\0';

	// what's said from when it began listening
	if ( ST_TriggerChat( s ) ) {
		if ( !r->watching ) {
			r->watching = qtrue;
			r->chatFrom = cls.realtime;
			return qfalse;
		}
		return ST_ChatSaid( s, r->chatFrom, r->by, sizeof( r->by ) );
	}
	// a use, from when it began listening
	if ( ST_TriggerUse( s ) ) {
		if ( !r->watching ) {
			r->watching = qtrue;
			r->chatFrom = cls.realtime;
			return qfalse;
		}
		return ST_UsedSince( s, r->chatFrom, r->by, sizeof( r->by ) );
	}

	if ( ST_TriggerDoor( s ) ) {
		const entityState_t *es = ST_FindMover( ST_ModelNum( s->values[1] ) );

		// out of sight: it's looked at afresh when it's back
		if ( !es ) {
			r->watching = qfalse;
			return qfalse;
		}
		fired = (qboolean)( r->watching && ( es->pos.trTime != r->moverTime || es->apos.trTime != r->moverATime )
			&& ( es->pos.trType != TR_STATIONARY || es->apos.trType != TR_STATIONARY ) );
		r->moverTime = es->pos.trTime;
		r->moverATime = es->apos.trTime;
		r->watching = qtrue;
		return fired;
	}

	// a named NPC is found in the NPC list, asked for once as it starts watching
	if ( !r->watching && !Q_stricmp( s->values[2], "named" ) && !Q_isanumber( s->values[3] ) ) {
		CL_NpcManager_RequestList();
		st.nextSend = Q_max( st.nextSend, cls.realtime + ST_CommandGap() );
	}

	// to stay in a while: from when they came, or from the first look for the ones in already
	stay = (int)( ST_Number( s, 8 ) * 1000.0f );
	n = ST_People( people, ST_MAX_PEOPLE );
	for ( int i = 0; i < n; i++ ) {
		qboolean was = qfalse;
		int since = cls.realtime;

		if ( !ST_Counts( s, &people[i] ) || !ST_Inside( s, people[i].org ) )
			continue;
		for ( int j = 0; j < r->numInside; j++ ) {
			if ( r->inside[j] == people[i].num ) {
				was = qtrue;
				since = r->since[j];
			}
		}
		nowSince[numNow] = since;
		now[numNow++] = people[i].num;
		if ( fired || !r->watching )
			continue;
		if ( stay > 0 ? cls.realtime - since >= stay : !was ) {
			fired = qtrue;
			ST_PersonName( &people[i], r->by, sizeof( r->by ) );
		}
	}
	memcpy( r->inside, now, numNow * sizeof( now[0] ) );
	memcpy( r->since, nowSince, numNow * sizeof( nowSince[0] ) );
	r->numInside = numNow;
	r->watching = qtrue;
	return fired;
}

// an If, looked at once: how many of who it counts are in the area, or whether they said it
// since the story began
static qboolean ST_IfTrue( const stRun_t *r, const stStep_t *s ) {
	stPerson_t people[ST_MAX_PEOPLE];
	int n, count = 0;
	const int want = (int)ST_Number( s, 9 );

	if ( ST_TriggerChat( s ) )
		return ST_ChatSaid( s, r->started, NULL, 0 );
	if ( ST_TriggerUse( s ) )
		return ST_UsedSince( s, r->started, NULL, 0 );
	n = ST_People( people, ST_MAX_PEOPLE );
	for ( int i = 0; i < n; i++ ) {
		if ( ST_Counts( s, &people[i] ) && ST_Inside( s, people[i].org ) )
			count++;
	}
	if ( !Q_stricmp( s->values[8], "at most" ) )
		return (qboolean)( count <= want );
	if ( !Q_stricmp( s->values[8], "exactly" ) )
		return (qboolean)( count == want );
	return (qboolean)( count >= want );
}

// from an If or an Else, the step after its Else (toElse) or its End if, Ifs inside it skipped
// whole; the end of the story when there's none
static int ST_SkipBranch( const stStep_t *steps, int numSteps, int pc, qboolean toElse ) {
	int depth = 0;

	for ( int i = pc + 1; i < numSteps; i++ ) {
		if ( !steps[i].enabled )
			continue;
		if ( steps[i].action == ST_A_IF ) {
			depth++;
		} else if ( steps[i].action == ST_A_ENDIF ) {
			if ( !depth )
				return i + 1;
			depth--;
		} else if ( steps[i].action == ST_A_ELSE && !depth && toElse ) {
			return i + 1;
		}
	}
	return numSteps;
}

// how deep in Ifs a step is, for the list
static int ST_Depth( const stStep_t *steps, int row ) {
	int depth = 0;

	for ( int i = 0; i < row; i++ ) {
		if ( !steps[i].enabled )
			continue;
		if ( steps[i].action == ST_A_IF )
			depth++;
		else if ( steps[i].action == ST_A_ENDIF && depth )
			depth--;
	}
	if ( steps[row].enabled && ( steps[row].action == ST_A_ELSE || steps[row].action == ST_A_ENDIF ) && depth )
		depth--;
	return depth;
}

// one line about a step, for the story list
static const char *ST_StepSummary( const stStep_t *s ) {
	static char buf[MAX_STRING_CHARS];
	const stActionDef_t *def = ST_Def( s );

	switch ( s->action ) {
	case ST_A_WAIT:
		Com_sprintf( buf, sizeof( buf ), "Wait %s s", s->values[0][0] ? s->values[0] : "0" );
		break;
	case ST_A_PAUSE:
		Com_sprintf( buf, sizeof( buf ), "Wait for me%s%s", s->values[0][0] ? ": " : "", s->values[0] );
		break;
	case ST_A_TRIGGER:
		Q_strncpyz( buf, ST_TriggerSummary( s ), sizeof( buf ) );
		break;
	case ST_A_IF:
		Q_strncpyz( buf, ST_IfSummary( s ), sizeof( buf ) );
		break;
	case ST_A_ELSE:
		Q_strncpyz( buf, "Else", sizeof( buf ) );
		break;
	case ST_A_ENDIF:
		Q_strncpyz( buf, "End if", sizeof( buf ) );
		break;
	case ST_A_NOTIFY:
		Com_sprintf( buf, sizeof( buf ), "Notify %s, %s s%s: %s", s->values[0][0] ? s->values[0] : "all", s->values[1],
			Q_stricmp( s->values[3], "no" ) || Q_stristr( s->values[2], "<timer>" ) ? ", counting down" : "", s->values[2] );
		break;
	case ST_A_SOUND:
		Com_sprintf( buf, sizeof( buf ), "Sound %s (%s, %s%s%s)", s->values[0][0] ? s->values[0] : "?", s->values[1], s->values[2],
			!Q_stricmp( s->values[2], "player" ) ? " " : "", !Q_stricmp( s->values[2], "player" ) ? s->values[3] : "" );
		break;
	case ST_A_MUSIC:
		Com_sprintf( buf, sizeof( buf ), "Music: %s", !Q_stricmp( s->values[0], "play" ) ? ( s->values[1][0] ? s->values[1] : "?" ) : s->values[0] );
		break;
	case ST_A_NPC_SPAWN:
		Com_sprintf( buf, sizeof( buf ), "Spawn %s%s%s", s->values[0][0] ? s->values[0] : "?",
			s->values[1][0] ? " as " : "", s->values[1] );
		break;
	case ST_A_NPC_CMD:
		Com_sprintf( buf, sizeof( buf ), "NPC %s %s%s%s", s->values[0][0] ? s->values[0] : "?", s->values[1],
			s->values[2][0] ? " " : "", s->values[2] );
		break;
	case ST_A_NPC_TELE:
		Com_sprintf( buf, sizeof( buf ), "Teleport %s", s->values[0][0] ? s->values[0] : "?" );
		break;
	case ST_A_NPC_TALK:
		Com_sprintf( buf, sizeof( buf ), "%s: %s", s->values[1][0] ? s->values[1] : s->values[0][0] ? s->values[0] : "?", s->values[4] );
		break;
	case ST_A_OBJECTIVE:
		Com_sprintf( buf, sizeof( buf ), "Objective %s%s%s", s->values[0], s->values[1][0] ? ": " : "", s->values[1] );
		break;
	case ST_A_CINEMATIC:
		Com_sprintf( buf, sizeof( buf ), "Cinematic %s%s", s->values[0][0] ? s->values[0] : "?",
			!Q_stricmp( s->values[1], "no" ) ? " (goes on)" : "" );
		break;
	case ST_A_CIN_START:
		Com_sprintf( buf, sizeof( buf ), "Start cinematic (HUD %s, bars %s)", s->values[0], s->values[1] );
		break;
	case ST_A_CAM_SHOT:
		Com_sprintf( buf, sizeof( buf ), "Camera shot %s%s", s->values[0][0] ? s->values[0] : "?",
			!Q_stricmp( s->values[1], "no" ) ? " (goes on)" : "" );
		break;
	case ST_A_CAM_FOV:
		Com_sprintf( buf, sizeof( buf ), "Camera zoom to %s in %s s", s->values[0], s->values[1] );
		break;
	case ST_A_CIN_BARS:
		Com_sprintf( buf, sizeof( buf ), "Black bars %s", s->values[0] );
		break;
	case ST_A_CENTER:
		Com_sprintf( buf, sizeof( buf ), "Center: %s", s->values[2] );
		break;
	case ST_A_EFFECT:
		Com_sprintf( buf, sizeof( buf ), "Effect %s (%s)", s->values[0][0] ? s->values[0] : "?", s->values[1] );
		break;
	case ST_A_SCREEN:
		Com_sprintf( buf, sizeof( buf ), "Screen: %s", s->values[0] );
		break;
	case ST_A_SHADER:
		Com_sprintf( buf, sizeof( buf ), "Remap %s", s->values[0][0] ? s->values[0] : "?" );
		break;
	case ST_A_SAY:
		if ( !Q_stricmp( s->values[1], "a player" ) )
			Com_sprintf( buf, sizeof( buf ), "Say as %s" S_COLOR_WHITE ": %s", s->values[2][0] ? s->values[2] : "?", s->values[0] );
		else
			Com_sprintf( buf, sizeof( buf ), "Say: %s", s->values[0] );
		break;
	case ST_A_LOCK:
		if ( !Q_stricmp( s->values[0], "unlock all" ) )
			Q_strncpyz( buf, "Unlock every lock", sizeof( buf ) );
		else if ( !Q_stricmp( s->values[0], "unlock" ) )
			Com_sprintf( buf, sizeof( buf ), "Unlock %s", s->values[1][0] ? s->values[1] : "?" );
		else
			Com_sprintf( buf, sizeof( buf ), "Lock %s: %s%s%s", s->values[1][0] ? s->values[1] : "?",
				atoi( s->values[2] ) < 0 ? "no one" : va( "rank %s", s->values[2][0] ? s->values[2] : "0" ), s->values[3][0] ? ", " : "", s->values[3] );
		break;
	case ST_A_OVERLAY:
		if ( !Q_stricmp( s->values[0], "remove all" ) )
			Q_strncpyz( buf, "Overlays: remove all", sizeof( buf ) );
		else
			Com_sprintf( buf, sizeof( buf ), "Overlay %s %s", s->values[0], s->values[1][0] ? s->values[1] : "?" );
		break;
	case ST_A_SKY:
		if ( !Q_stricmp( s->values[0], "reset" ) )
			Q_strncpyz( buf, "Sky look: as the map has it", sizeof( buf ) );
		else
			Com_sprintf( buf, sizeof( buf ), "Sky look: %s, spin %s", s->values[1], s->values[2][0] ? s->values[2] : "0" );
		break;
	default:
		Com_sprintf( buf, sizeof( buf ), "%s%s%s", def->name, s->values[0][0] ? ": " : "", s->values[0] );
		break;
	}
	return buf;
}

// an action's name, a "when" by its kind
static const char *ST_StepName( const stStep_t *s ) {
	if ( s->action == ST_A_TRIGGER )
		return stWhenKinds[ST_WhenKind( s->values[0] )].name;
	return ST_Def( s )->name;
}

static const char *ST_RowName( const stActionRow_t *r ) {
	return r->kind >= 0 ? stWhenKinds[r->kind].name : stActions[r->action].name;
}

static const char *ST_RowDesc( const stActionRow_t *r ) {
	return r->kind >= 0 ? stWhenKinds[r->kind].desc : stActions[r->action].desc;
}

// an If, an Else or an End if: they go under a "when", and look when it's set off
static qboolean ST_IsCondition( stAction_t action ) {
	return (qboolean)( action == ST_A_IF || action == ST_A_ELSE || action == ST_A_ENDIF );
}

static qboolean ST_UnderWhen( const stStep_t *steps, int row ) {
	for ( int i = 0; i < row; i++ ) {
		if ( steps[i].action == ST_A_TRIGGER )
			return qtrue;
	}
	return qfalse;
}

static qboolean ST_StepUsable( const stStep_t *s ) {
	qboolean local;
	const char *error;

	if ( s->action == ST_A_WAIT || s->action == ST_A_PAUSE )
		return qtrue;
	ST_StepCommand( s, &local, &error );
	return (qboolean)!error;
}

static void ST_Changed( void ) {
	st.dirty = qtrue;
}

static void ST_SelectStep( int index ) {
	st.stepList.sel = index;
	st.focus = ST_F_NONE;
}

// where a new step goes: after the selected one
static int ST_InsertAt( void ) {
	return ( st.stepList.sel >= 0 && st.stepList.sel < st.numSteps ) ? st.stepList.sel + 1 : st.numSteps;
}

// a new step of this action, after the selected one
static void ST_AddStep( stAction_t action ) {
	int at;
	stStep_t *s;

	if ( st.numSteps >= ST_MAX_STEPS ) {
		ST_Message( S_COLOR_RED "The story is full" );
		return;
	}
	at = ST_InsertAt();
	if ( ST_IsCondition( action ) && !ST_UnderWhen( st.steps, at ) ) {
		ST_Message( S_COLOR_YELLOW "An If, Else or End if goes under a When step: add the When first, then select it" );
		return;
	}
	memmove( &st.steps[at + 1], &st.steps[at], ( st.numSteps - at ) * sizeof( st.steps[0] ) );
	st.numSteps++;
	if ( ST_RunHere() && st.run.pc >= at )
		st.run.pc++;

	s = &st.steps[at];
	s->action = action;
	s->enabled = qtrue;
	ST_SetDefaults( s );
	// most spots are near where the storyteller stands
	ST_FillHere( s );
	ST_SelectStep( at );
	ST_Changed();
}

// a row of the actions list put in the story: a "when" of its kind
static void ST_AddRow( const stActionRow_t *r ) {
	const int before = st.numSteps;

	ST_AddStep( r->action );
	if ( r->kind >= 0 && st.numSteps > before && st.stepList.sel >= 0 ) {
		Q_strncpyz( st.steps[st.stepList.sel].values[0], stWhenKinds[r->kind].where, ST_VALUE_LEN );
		ST_FillHere( &st.steps[st.stepList.sel] );
	}
}

// the steps picked, in order: the marked ones, or the selected one; how many
static int ST_Picked( int *rows ) {
	int count = 0;

	for ( int i = 0; i < st.numSteps; i++ ) {
		if ( st.marked[i] )
			rows[count++] = i;
	}
	if ( !count && st.stepList.sel >= 0 && st.stepList.sel < st.numSteps )
		rows[count++] = st.stepList.sel;
	return count;
}

static void ST_ClearMarks( void ) {
	memset( st.marked, 0, sizeof( st.marked ) );
}

static void ST_DeleteStep( void ) {
	int rows[ST_MAX_STEPS];
	const int count = ST_Picked( rows );

	if ( !count )
		return;
	// from the last up, so the ones before stay where they are
	for ( int i = count - 1; i >= 0; i-- ) {
		const int at = rows[i];

		memmove( &st.steps[at], &st.steps[at + 1], ( st.numSteps - at - 1 ) * sizeof( st.steps[0] ) );
		st.numSteps--;
		if ( ST_RunHere() && st.run.pc > at )
			st.run.pc--;
	}
	ST_ClearMarks();
	ST_SelectStep( rows[0] < st.numSteps ? rows[0] : st.numSteps - 1 );
	ST_Changed();
	if ( count > 1 )
		ST_Message( va( "Deleted %i steps", count ) );
}

// the picked steps to the clipboard
static void ST_CopySteps( qboolean cut ) {
	int rows[ST_MAX_STEPS];
	const int count = ST_Picked( rows );

	if ( !count )
		return;
	if ( !stClip )
		stClip = (stStep_t *)Z_Malloc( ST_MAX_STEPS * sizeof( stStep_t ), TAG_GENERAL, qfalse );
	for ( int i = 0; i < count; i++ )
		stClip[i] = st.steps[rows[i]];
	stNumClip = count;
	if ( cut )
		ST_DeleteStep();
	ST_Message( va( "%s %i step%s: Paste puts %s after the selected one, in any tab", cut ? "Cut" : "Copied", count, count == 1 ? "" : "s",
		count == 1 ? "it" : "them" ) );
}

// the clipboard after the selected step, picked once there
static void ST_PasteSteps( void ) {
	int at;

	if ( !stNumClip )
		return;
	if ( st.numSteps + stNumClip > ST_MAX_STEPS ) {
		ST_Message( S_COLOR_RED "The story would be too long" );
		return;
	}
	at = ( st.stepList.sel >= 0 && st.stepList.sel < st.numSteps ) ? st.stepList.sel + 1 : st.numSteps;
	memmove( &st.steps[at + stNumClip], &st.steps[at], ( st.numSteps - at ) * sizeof( st.steps[0] ) );
	memcpy( &st.steps[at], stClip, stNumClip * sizeof( st.steps[0] ) );
	st.numSteps += stNumClip;
	if ( ST_RunHere() && st.run.pc >= at )
		st.run.pc += stNumClip;
	ST_ClearMarks();
	for ( int i = 0; i < stNumClip; i++ )
		st.marked[at + i] = 1;
	ST_SelectStep( at + stNumClip - 1 );
	st.anchor = at;
	ST_Changed();
}

static void ST_DuplicateStep( void ) {
	const int at = st.stepList.sel;

	if ( at < 0 || at >= st.numSteps || st.numSteps >= ST_MAX_STEPS )
		return;
	memmove( &st.steps[at + 1], &st.steps[at], ( st.numSteps - at ) * sizeof( st.steps[0] ) );
	st.numSteps++;
	if ( ST_RunHere() && st.run.pc > at )
		st.run.pc++;
	ST_SelectStep( at + 1 );
	ST_Changed();
}

static void ST_MoveStep( int delta ) {
	const int at = st.stepList.sel, to = at + delta;
	stStep_t tmp;

	if ( at < 0 || at >= st.numSteps || to < 0 || to >= st.numSteps )
		return;
	tmp = st.steps[at];
	st.steps[at] = st.steps[to];
	st.steps[to] = tmp;
	{
		const byte mark = st.marked[at];

		st.marked[at] = st.marked[to];
		st.marked[to] = mark;
	}
	if ( ST_RunHere() ) {
		if ( st.run.pc == at )
			st.run.pc = to;
		else if ( st.run.pc == to )
			st.run.pc = at;
	}
	ST_SelectStep( to );
	ST_Changed();
}

static void ST_New( void ) {
	if ( ST_RunHere() )
		st.run.running = qfalse;
	ST_ClearMarks();
	st.numSteps = 0;
	st.name[0] = '\0';
	st.dirty = qfalse;
	memset( &st.stepList, 0, sizeof( st.stepList ) );
	st.stepList.sel = -1;
	st.focus = ST_F_NONE;
}

/*
===============================================================================

PLAYING

===============================================================================
*/

static int ST_CommandGap( void ) {
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

static void ST_Stop( const char *why ) {
	if ( !st.run.running )
		return;
	st.run.running = qfalse;
	st.run.waiting = qfalse;
	if ( why ) {
		ST_Message( why );
		Com_Printf( "Storyteller: %s\n", why );
	}
}

static void ST_StartRun( stRun_t *r, int from ) {
	memset( r, 0, sizeof( *r ) );
	r->running = qtrue;
	r->pc = from;
	r->started = cls.realtime;
}

static void ST_Run( int from ) {
	if ( cls.state != CA_ACTIVE ) {
		ST_Message( S_COLOR_RED "Join a server first" );
		return;
	}
	if ( from < 0 || from >= st.numSteps ) {
		ST_Message( S_COLOR_YELLOW "Nothing to play: add steps first" );
		return;
	}
	ST_StartRun( &st.run, from );
	st.runTab = stActiveTab;
	ST_Message( from ? va( S_COLOR_GREEN "Playing from step %i", from + 1 ) : S_COLOR_GREEN "Playing" );
}

static qboolean ST_AnyWaiting( void ) {
	if ( st.run.running && st.run.waiting )
		return qtrue;
	for ( int i = 0; i < stNumArmed; i++ ) {
		if ( stArmed[i].run.running && stArmed[i].run.waiting )
			return qtrue;
	}
	return qfalse;
}

// every story at a "wait for me" goes on
static void ST_Continue( void ) {
	if ( st.run.running && st.run.waiting )
		st.run.goOn = qtrue;
	for ( int i = 0; i < stNumArmed; i++ ) {
		if ( stArmed[i].run.running && stArmed[i].run.waiting )
			stArmed[i].run.goOn = qtrue;
	}
}

// a frame of a story playing; qtrue when it got to its end. An armed one is named in the console
static qboolean ST_Play( stRun_t *r, stStep_t *steps, int numSteps, const char *armed ) {
	char prefix[MAX_QPATH + 32];
	const char *cmd, *error;
	qboolean local;
	stStep_t *s;

	if ( armed )
		Com_sprintf( prefix, sizeof( prefix ), "Storyteller (%s): ", armed );
	else
		Q_strncpyz( prefix, "Storyteller: ", sizeof( prefix ) );

	while ( r->running ) {
		// a shot plays out, though what goes with it still goes
		if ( cls.realtime < r->shotUntil && !r->extraDue )
			return qfalse;
		if ( r->pc >= numSteps )
			return qtrue;
		s = &steps[r->pc];
		if ( !s->enabled ) {
			r->pc++;
			continue;
		}
		if ( ST_IsCondition( s->action ) && !ST_UnderWhen( steps, r->pc ) ) {
			Com_Printf( S_COLOR_YELLOW "%sstep %i: an If, Else or End if goes under a When step; the story stops here\n", prefix, r->pc + 1 );
			r->pc = numSteps;
			return qtrue;
		}

		switch ( s->action ) {
		case ST_A_WAIT:
			if ( !r->stepStart )
				r->stepStart = cls.realtime;
			if ( cls.realtime - r->stepStart < (int)( ST_Number( s, 0 ) * 1000.0f ) )
				return qfalse;
			r->stepStart = 0;
			r->pc++;
			continue;

		case ST_A_PAUSE:
			r->waiting = qtrue;
			if ( !r->goOn )
				return qfalse;
			r->waiting = r->goOn = qfalse;
			r->pc++;
			continue;

		case ST_A_TRIGGER:
			ST_StepCommand( s, &local, &error );
			if ( error ) {
				Com_Printf( S_COLOR_YELLOW "%sstep %i skipped: %s\n", prefix, r->pc + 1, error );
				r->pc++;
				continue;
			}
			if ( !ST_TriggerFired( r, s ) )
				return qfalse;
			Com_Printf( S_COLOR_GREEN "%s" S_COLOR_WHITE "step %i set off%s%s\n", prefix, r->pc + 1, r->by[0] ? " by " : "", r->by );
			r->watching = qfalse;
			r->numInside = 0;
			r->pc++;
			continue;

		case ST_A_IF: {
			qboolean yes;

			ST_StepCommand( s, &local, &error );
			if ( error )
				Com_Printf( S_COLOR_YELLOW "%sstep %i taken as false: %s\n", prefix, r->pc + 1, error );
			yes = (qboolean)( !error && ST_IfTrue( r, s ) );
			Com_Printf( S_COLOR_GREEN "%s" S_COLOR_WHITE "step %i: %s, %s\n", prefix, r->pc + 1, ST_IfSummary( s ), yes ? "yes" : "no" );
			r->pc = yes ? r->pc + 1 : ST_SkipBranch( steps, numSteps, r->pc, qtrue );
			continue;
		}

		case ST_A_ELSE:
			// the If's steps played: the Else's don't
			r->pc = ST_SkipBranch( steps, numSteps, r->pc, qfalse );
			continue;

		case ST_A_ENDIF:
			r->pc++;
			continue;

		default:
			// the server's flood protection: any command this client sent counts, an NPC
			// walk's or an impersonation's too
			if ( cls.realtime < st.nextSend || cls.realtime - CL_LastCommandTime() < ST_CommandGap() )
				return qfalse;
			if ( r->extraDue ) {
				r->extraDue = qfalse;
				cmd = ST_StepExtra( s, &local );
				if ( cmd ) {
					Com_Printf( S_COLOR_GREEN "%s" S_COLOR_WHITE "%s\n", prefix, cmd );
					if ( local ) {
						Cbuf_AddText( cmd );
						Cbuf_AddText( "\n" );
					} else {
						CL_AddReliableCommand( cmd, qfalse );
					}
					r->sent++;
					st.nextSend = cls.realtime + ST_CommandGap();
				}
				r->pc++;
				return qfalse;
			}
			cmd = ST_StepCommand( s, &local, &error );
			if ( !cmd ) {
				Com_Printf( S_COLOR_YELLOW "%sstep %i skipped: %s\n", prefix, r->pc + 1, error ? error : "nothing to send" );
				r->pc++;
				continue;
			}
			Com_Printf( S_COLOR_GREEN "%s" S_COLOR_WHITE "%s\n", prefix, cmd );
			if ( local ) {
				Cbuf_AddText( cmd );
				Cbuf_AddText( "\n" );
			} else {
				CL_AddReliableCommand( cmd, qfalse );
			}
			r->sent++;
			// a console command may well send one too
			st.nextSend = cls.realtime + ST_CommandGap();
			// a cinematic goes out and plays out
			if ( s->action == ST_A_CINEMATIC && Q_stricmp( s->values[1], "no" ) ) {
				const int ms = CL_Cinematic_PlayMs( s->values[0] );

				if ( ms > 0 )
					r->shotUntil = cls.realtime + ms;
			}
			if ( s->action == ST_A_CAM_SHOT ) {
				char hook[MAX_STRING_CHARS];
				int ms;

				// the NPCs walking in it set off, each when the shot says
				CL_RpCinematic_ShotWalks( s->values[0], qfalse );
				if ( Q_stricmp( s->values[1], "no" ) && CL_RpCinematic_ShotHook( s->values[0], hook, sizeof( hook ), &ms ) )
					r->shotUntil = cls.realtime + ms;
			}
			if ( ST_StepExtra( s, &local ) )
				r->extraDue = qtrue;
			else
				r->pc++;
			return qfalse;
		}
	}
	return qfalse;
}

// what keeps a story from being armed, NULL if nothing
static const char *ST_ArmProblem( const stStep_t *steps, int numSteps ) {
	const char *error;
	qboolean local;

	for ( int i = 0; i < numSteps; i++ ) {
		if ( !steps[i].enabled )
			continue;
		if ( steps[i].action != ST_A_TRIGGER )
			return "its first step has to be \"When someone comes\"";
		ST_StepCommand( &steps[i], &local, &error );
		return error;
	}
	return "it has no steps";
}

static void ST_Disarm( int index ) {
	Z_Free( stArmed[index].steps );
	stNumArmed--;
	memmove( &stArmed[index], &stArmed[index + 1], ( stNumArmed - index ) * sizeof( stArmed[0] ) );
}

static int ST_FindArmed( const char *name ) {
	for ( int i = 0; i < stNumArmed; i++ ) {
		if ( !Q_stricmp( stArmed[i].name, name ) )
			return i;
	}
	return -1;
}

// "once", "every time" or "3 times"
static const char *ST_ArmedHow( int limit ) {
	static char out[32];

	if ( !limit )
		return "every time";
	if ( limit == 1 )
		return "once";
	Com_sprintf( out, sizeof( out ), "%i times", limit );
	return out;
}

// an armed one, its plays and how many there were
static const char *ST_ArmedState( const stArmed_t *a ) {
	static char out[64];

	if ( a->limit > 1 )
		Com_sprintf( out, sizeof( out ), "%s, %i left", ST_ArmedHow( a->limit ), a->limit - a->times );
	else
		Com_sprintf( out, sizeof( out ), "%s, set off %i time%s", ST_ArmedHow( a->limit ), a->times, a->times == 1 ? "" : "s" );
	return out;
}

// a copy of the story waits for its first step, to play limit times (0 every time); one by that
// name already armed is replaced
static qboolean ST_Arm( const char *name, const stStep_t *steps, int numSteps, int limit ) {
	const char *why = ST_ArmProblem( steps, numSteps );
	int i = ST_FindArmed( name );
	stArmed_t *a;

	if ( cls.state != CA_ACTIVE )
		why = "join a server first";
	if ( why ) {
		ST_Message( va( S_COLOR_YELLOW "Can't arm %s: %s", name, why ) );
		Com_Printf( S_COLOR_YELLOW "Storyteller: can't arm %s: %s\n", name, why );
		return qfalse;
	}
	if ( i >= 0 ) {
		Z_Free( stArmed[i].steps );
	} else {
		if ( stNumArmed >= ST_MAX_ARMED ) {
			ST_Message( va( S_COLOR_YELLOW "%i stories are armed already: disarm one first", ST_MAX_ARMED ) );
			Com_Printf( S_COLOR_YELLOW "Storyteller: %i stories are armed already\n", ST_MAX_ARMED );
			return qfalse;
		}
		i = stNumArmed++;
	}
	a = &stArmed[i];
	memset( a, 0, sizeof( *a ) );
	Q_strncpyz( a->name, name, sizeof( a->name ) );
	Q_strncpyz( a->map, cl.mapname, sizeof( a->map ) );
	a->steps = (stStep_t *)Z_Malloc( numSteps * sizeof( stStep_t ), TAG_GENERAL, qfalse );
	memcpy( a->steps, steps, numSteps * sizeof( stStep_t ) );
	a->numSteps = numSteps;
	a->limit = limit;
	ST_StartRun( &a->run, 0 );
	ST_Message( va( S_COLOR_GREEN "Armed %s: it plays %s", name, !limit ? "every time it's set off" : limit == 1 ? "the next time it's set off"
		: va( "the next %i times it's set off", limit ) ) );
	Com_Printf( S_COLOR_GREEN "Storyteller: " S_COLOR_WHITE "armed %s (%s)\n", name, ST_ArmedHow( limit ) );
	return qtrue;
}

static qboolean ST_AtTrigger( const stRun_t *r, const stStep_t *steps, int numSteps ) {
	return (qboolean)( r->pc < numSteps && steps[r->pc].action == ST_A_TRIGGER && cls.realtime >= r->shotUntil );
}

static void ST_RunArmed( void ) {
	for ( int i = 0; i < stNumArmed; i++ ) {
		stArmed_t *a = &stArmed[i];

		// the triggers are the map's
		if ( Q_stricmp( a->map, cl.mapname ) ) {
			Com_Printf( S_COLOR_YELLOW "Storyteller: %s disarmed: the map changed\n", a->name );
			ST_Disarm( i-- );
			continue;
		}
		if ( !a->run.running )
			ST_StartRun( &a->run, 0 );
		if ( !ST_Play( &a->run, a->steps, a->numSteps, a->name ) )
			continue;
		a->times++;
		if ( !a->limit ) {
			Com_Printf( S_COLOR_GREEN "Storyteller (%s): " S_COLOR_WHITE "played, armed again\n", a->name );
			ST_StartRun( &a->run, 0 );
		} else if ( a->times < a->limit ) {
			Com_Printf( S_COLOR_GREEN "Storyteller (%s): " S_COLOR_WHITE "played %i of %i times, armed again\n", a->name, a->times, a->limit );
			ST_StartRun( &a->run, 0 );
		} else {
			Com_Printf( S_COLOR_GREEN "Storyteller (%s): " S_COLOR_WHITE "played, disarmed\n", a->name );
			ST_Disarm( i-- );
		}
	}
}

// every frame in game, open or not
static void ST_RunFrame( void ) {
	int runNum;
	stStep_t *runSteps = ST_RunSteps( &runNum );

	if ( st.run.running && ST_Play( &st.run, runSteps, runNum, NULL ) )
		ST_Stop( va( S_COLOR_GREEN "The story is over (%i command%s sent)", st.run.sent, st.run.sent == 1 ? "" : "s" ) );
	ST_RunArmed();
}

/*
===============================================================================

FILES

===============================================================================
*/

static qboolean ST_NameChar( int ch ) {
	return (qboolean)( isalnum( ch ) || ch == '_' || ch == '-' );
}

static const char *ST_StoryPath( const char *name ) {
	return va( "%s/%s%s", ST_SAVE_DIR, name, ST_SAVE_EXT );
}

static void ST_WriteLine( fileHandle_t f, const char *text ) {
	FS_Write( text, strlen( text ), f );
}

static qboolean ST_Save( const char *name ) {
	fileHandle_t f;

	if ( !name[0] )
		return qfalse;
	f = FS_FOpenFileWrite( ST_StoryPath( name ) );
	if ( !f ) {
		ST_Message( va( S_COLOR_RED "Couldn't write %s", ST_StoryPath( name ) ) );
		return qfalse;
	}
	ST_WriteLine( f, "// Storyteller story: action, 1 if it runs, then its settings\n" );
	for ( int i = 0; i < st.numSteps; i++ ) {
		const stStep_t *s = &st.steps[i];
		const stActionDef_t *def = ST_Def( s );

		ST_WriteLine( f, va( "%s %i", def->key, s->enabled ? 1 : 0 ) );
		for ( int j = 0; j < def->numFields; j++ )
			ST_WriteLine( f, va( " \"%s\"", s->values[j] ) );
		ST_WriteLine( f, "\n" );
	}
	FS_FCloseFile( f );
	Q_strncpyz( st.name, name, sizeof( st.name ) );
	st.dirty = qfalse;
	ST_Message( va( S_COLOR_GREEN "Saved %s (%i step%s)", name, st.numSteps, st.numSteps == 1 ? "" : "s" ) );
	return qtrue;
}

// splits a line into words, "quoted" ones whole; the line is cut up in place
static int ST_SplitLine( char *line, char **argv, int max ) {
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

// a saved story's steps; how many, -1 if there is none
static int ST_ReadStory( const char *name, stStep_t *steps, int max, int *skipped ) {
	char *buf, *line, *next, *argv[ST_MAX_FIELDS + 2];
	int len, argc, count = 0;
	stStep_t *s;

	*skipped = 0;
	len = FS_ReadFile( ST_StoryPath( name ), (void **)&buf );
	if ( len < 0 || !buf )
		return -1;

	for ( line = buf; line && *line; line = next ) {
		next = strchr( line, '\n' );
		if ( next )
			*next++ = '\0';
		len = strlen( line );
		if ( len && line[len - 1] == '\r' )
			line[len - 1] = '\0';
		if ( !line[0] || ( line[0] == '/' && line[1] == '/' ) )
			continue;

		argc = ST_SplitLine( line, argv, ARRAY_LEN( argv ) );
		if ( argc < 2 )
			continue;
		int action = -1;
		for ( int i = 0; i < ST_NUM_ACTIONS; i++ ) {
			if ( !Q_stricmp( argv[0], stActions[i].key ) )
				action = i;
		}
		if ( action < 0 || count >= max ) {
			( *skipped )++;
			continue;
		}
		s = &steps[count++];
		memset( s, 0, sizeof( *s ) );
		s->action = (stAction_t)action;
		s->enabled = (qboolean)( atoi( argv[1] ) != 0 );
		ST_SetDefaults( s );
		for ( int i = 2; i < argc && i - 2 < ST_Def( s )->numFields; i++ )
			Q_strncpyz( s->values[i - 2], argv[i], ST_VALUE_LEN );
		// as older stories have it
		if ( s->action == ST_A_TRIGGER && !Q_stricmp( s->values[0], "door or button" ) )
			Q_strncpyz( s->values[0], "door", ST_VALUE_LEN );
	}
	FS_FreeFile( buf );
	return count;
}

static qboolean ST_Load( const char *name ) {
	int skipped;

	if ( FS_ReadFile( ST_StoryPath( name ), NULL ) < 0 ) {
		ST_Message( va( S_COLOR_RED "No story called %s", name ) );
		return qfalse;
	}
	ST_New();
	st.numSteps = Q_max( 0, ST_ReadStory( name, st.steps, ST_MAX_STEPS, &skipped ) );

	Q_strncpyz( st.name, name, sizeof( st.name ) );
	st.dirty = qfalse;
	st.stepList.sel = st.numSteps ? 0 : -1;
	if ( skipped )
		ST_Message( va( S_COLOR_YELLOW "Loaded %s; %i line%s not understood", name, skipped, skipped == 1 ? "" : "s" ) );
	else
		ST_Message( va( S_COLOR_GREEN "Loaded %s (%i step%s)", name, st.numSteps, st.numSteps == 1 ? "" : "s" ) );
	return qtrue;
}

// a saved story armed, by the console
static void ST_ArmSaved( const char *name, int limit ) {
	stStep_t *steps = (stStep_t *)Z_Malloc( ST_MAX_STEPS * sizeof( stStep_t ), TAG_GENERAL, qfalse );
	int skipped;
	const int count = ST_ReadStory( name, steps, ST_MAX_STEPS, &skipped );

	if ( count < 0 )
		Com_Printf( "Storyteller: no story called %s\n", name );
	else
		ST_Arm( name, steps, count, limit );
	Z_Free( steps );
}
static int QDECL ST_CompareNames( const void *a, const void *b ) {
	return Q_stricmp( (const char *)a, (const char *)b );
}

/*
===============================================================================

TABS

===============================================================================
*/

// the story shown, kept in its tab
static void ST_StoreTab( void ) {
	stTab_t *t = &stTabs[stActiveTab];

	if ( !t->steps )
		t->steps = (stStep_t *)Z_Malloc( ST_MAX_STEPS * sizeof( stStep_t ), TAG_GENERAL, qfalse );
	memcpy( t->steps, st.steps, st.numSteps * sizeof( stStep_t ) );
	t->numSteps = st.numSteps;
	Q_strncpyz( t->name, st.name, sizeof( t->name ) );
	t->dirty = st.dirty;
	t->stepList = st.stepList;
	memcpy( t->marked, st.marked, sizeof( t->marked ) );
}

// a tab's story shown, the one shown before not kept
static void ST_FetchTab( int index ) {
	stTab_t *t = &stTabs[index];

	stActiveTab = index;
	st.numSteps = t->steps ? t->numSteps : 0;
	if ( st.numSteps )
		memcpy( st.steps, t->steps, st.numSteps * sizeof( stStep_t ) );
	Q_strncpyz( st.name, t->name, sizeof( st.name ) );
	st.dirty = t->dirty;
	st.stepList = t->stepList;
	memcpy( st.marked, t->marked, sizeof( st.marked ) );
	st.anchor = Q_max( 0, st.stepList.sel );
	st.focus = ST_F_NONE;
	st.confirmClose = 0;
}

static void ST_ShowTab( int index ) {
	if ( index == stActiveTab || index < 0 || index >= stNumTabs )
		return;
	ST_StoreTab();
	ST_FetchTab( index );
}

// a new, empty tab, shown; qfalse if there's no room for one
static qboolean ST_NewTab( void ) {
	if ( stNumTabs >= ST_MAX_TABS ) {
		ST_Message( va( S_COLOR_YELLOW "At most %i stories open at once: close one first", ST_MAX_TABS ) );
		return qfalse;
	}
	ST_StoreTab();
	memset( &stTabs[stNumTabs], 0, sizeof( stTabs[0] ) );
	stTabs[stNumTabs].stepList.sel = -1;
	ST_FetchTab( stNumTabs++ );
	return qtrue;
}

// the tab shown closed, its story with it; the one playing in it stops
static void ST_CloseTab( void ) {
	const int index = stActiveTab;

	if ( ST_RunHere() )
		ST_Stop( S_COLOR_YELLOW "Stopped: its tab was closed" );
	if ( stNumTabs == 1 ) {
		ST_New();
		return;
	}
	if ( stTabs[index].steps )
		Z_Free( stTabs[index].steps );
	stNumTabs--;
	memmove( &stTabs[index], &stTabs[index + 1], ( stNumTabs - index ) * sizeof( stTabs[0] ) );
	memset( &stTabs[stNumTabs], 0, sizeof( stTabs[0] ) );
	if ( st.runTab > index )
		st.runTab--;
	ST_FetchTab( index < stNumTabs ? index : stNumTabs - 1 );
}

static void ST_RefreshStories( void ) {
	char list[16384], *name;
	int count;

	st.numStories = 0;
	count = FS_GetFileList( ST_SAVE_DIR, ST_SAVE_EXT, list, sizeof( list ) );
	name = list;
	for ( int i = 0; i < count && st.numStories < ST_MAX_STORIES; i++ ) {
		const int len = strlen( name );
		char *ext;

		Q_strncpyz( st.stories[st.numStories], name, MAX_QPATH );
		ext = strstr( st.stories[st.numStories], ST_SAVE_EXT );
		if ( ext )
			*ext = '\0';
		if ( st.stories[st.numStories][0] )
			st.numStories++;
		name += len + 1;
	}
	qsort( st.stories, st.numStories, sizeof( st.stories[0] ), ST_CompareNames );
}

static void ST_OpenSave( void ) {
	st.popup = ST_POPUP_SAVE;
	Q_strncpyz( st.saveName, st.name, sizeof( st.saveName ) );
	st.focus = ST_F_SAVE_NAME;
}

// a saved story opened from the list: in a tab of its own, unless the one shown is empty,
// or the story is open already
static qboolean ST_LoadInTab( const char *name ) {
	if ( FS_ReadFile( ST_StoryPath( name ), NULL ) < 0 ) {
		ST_Message( va( S_COLOR_RED "No story called %s", name ) );
		return qfalse;
	}
	ST_StoreTab();
	for ( int i = 0; i < stNumTabs; i++ ) {
		if ( !Q_stricmp( stTabs[i].name, name ) ) {
			ST_ShowTab( i );
			ST_Message( va( "%s is open in tab %i", name, i + 1 ) );
			return qtrue;
		}
	}
	if ( ( st.numSteps || st.name[0] || st.dirty ) && !ST_NewTab() )
		return qfalse;
	return ST_Load( name );
}

static void ST_OpenLoad( void ) {
	ST_RefreshStories();
	st.popup = ST_POPUP_LOAD;
	memset( &st.storyList, 0, sizeof( st.storyList ) );
	st.storyList.lastClickRow = -1;
	st.focus = ST_F_NONE;
}

static void ST_DoSave( void ) {
	if ( !st.saveName[0] ) {
		ST_Message( S_COLOR_YELLOW "Type a name" );
		return;
	}
	if ( ST_Save( st.saveName ) ) {
		st.popup = ST_POPUP_NONE;
		st.focus = ST_F_NONE;
	}
}

/*
===============================================================================

OPEN / CLOSE

===============================================================================
*/

static void ST_BuildActionRows( void ) {
	const char *category = NULL;

	st.numActionRows = 0;
	for ( int i = 0; i < ST_NUM_ACTIONS; i++ ) {
		if ( ST_OldAction( i ) )
			continue;
		if ( !category || Q_stricmp( category, stActions[i].category ) ) {
			category = stActions[i].category;
			st.actionRows[st.numActionRows].header = category;
			st.numActionRows++;
		}
		// a "when", once for each kind
		for ( int k = 0; k < ( i == ST_A_TRIGGER ? (int)ARRAY_LEN( stWhenKinds ) : 1 ); k++ ) {
			st.actionRows[st.numActionRows].header = NULL;
			st.actionRows[st.numActionRows].action = (stAction_t)i;
			st.actionRows[st.numActionRows].kind = i == ST_A_TRIGGER ? k : -1;
			st.numActionRows++;
		}
	}
}

static qboolean ST_Open( void ) {
	if ( cls.state != CA_ACTIVE || !cls.cgameStarted ) {
		Com_Printf( "Storyteller: join a server first\n" );
		return qfalse;
	}
	// for RPMod's admins: whoever else can't run what it makes
	if ( !CL_RpAdmin_Allow( "Storyteller", "storyteller" ) )
		return qfalse;
	CL_ModelManager_Close();
	CL_NpcManager_Close();
	CL_ShaderManager_Close();
	CL_EffectManager_Close();
	CL_RpCinematic_EditorClose();
	CL_SoundManager_Close();
	CL_MapEditor_Close();

	st.font = re->RegisterFont( "arialnb" );
	if ( !st.font )
		st.font = cls.menuFont;
	if ( !st.numActionRows ) {
		ST_BuildActionRows();
		st.actionList.sel = 1;
		st.actionList.lastClickRow = -1;
	}

	st.open = qtrue;
	st.popup = ST_POPUP_NONE;
	st.focus = ST_F_NONE;
	st.cursorX = SCREEN_WIDTH * 0.5f;
	st.cursorY = SCREEN_HEIGHT * 0.5f;
	st.click = qfalse;
	st.wheel = 0;
	Key_SetCatcher( Key_GetCatcher() | KEYCATCH_STORYTELLER );
	return qtrue;
}

void CL_Storyteller_Close( void ) {
	if ( !st.open )
		return;
	st.open = qfalse;
	st.popup = ST_POPUP_NONE;
	st.focus = ST_F_NONE;
	Key_SetCatcher( Key_GetCatcher() & ~KEYCATCH_STORYTELLER );
}

void CL_Storyteller_Open( void ) {
	if ( !st.open )
		ST_Open();
}

// the cinematic manager saved what it was opened on: the step takes the name it went under; an
// old Camera shot step plays it as a cinematic from now on, waiting for it or not as it did
void CL_Storyteller_ShotSaved( const char *name ) {
	stStep_t *s = ST_SelectedStep();

	if ( !s || ( s->action != ST_A_CINEMATIC && s->action != ST_A_CAM_SHOT ) )
		return;
	if ( s->action == ST_A_CAM_SHOT ) {
		s->action = ST_A_CINEMATIC;
		ST_Changed();
	}
	if ( Q_stricmp( s->values[0], name ) ) {
		Q_strncpyz( s->values[0], name, ST_VALUE_LEN );
		ST_Changed();
	}
}


/*
===============================================================================

Chat heard, for "someone says"

===============================================================================
*/

#define ST_MAX_CHAT		64

static struct {
	int		time;					// cls.realtime it came
	char	name[MAX_QPATH];		// without colours
	char	text[MAX_STRING_CHARS / 2];
} stChat[ST_MAX_CHAT];
static int stChatNext;

static qboolean ST_FarMessage( const char *text );

// a chat command from the server, before a language turns it into gibberish: kept a while.
// "chat" "name\x19: message", "lchat" "name" "location" "colour" "message". qtrue for one of
// the hidden ones between storyteller and players, which never shows
qboolean CL_Storyteller_ChatLine( void ) {
	const char *cmd = Cmd_Argv( 0 );
	char name[MAX_STRING_CHARS], text[MAX_STRING_CHARS];

	if ( !strcmp( cmd, "chat" ) || !strcmp( cmd, "tchat" ) ) {
		const char *line = Cmd_Argv( 1 ), *sep = strstr( line, "\x19:" );

		if ( sep ) {
			Q_strncpyz( name, line, Q_min( (int)sizeof( name ), (int)( sep - line ) + 1 ) );
			line = sep + 2;
		} else {
			name[0] = '\0';
		}
		Q_strncpyz( text, line, sizeof( text ) );
	} else if ( !strcmp( cmd, "lchat" ) || !strcmp( cmd, "ltchat" ) ) {
		Q_strncpyz( name, Cmd_Argv( 1 ), sizeof( name ) );
		Q_strncpyz( text, Cmd_Argv( 4 ), sizeof( text ) );
	} else {
		return qfalse;
	}
	Q_StripColor( name );
	Q_StripColor( text );
	if ( ST_FarMessage( text ) )
		return qtrue;
	stChat[stChatNext].time = cls.realtime;
	Q_strncpyz( stChat[stChatNext].name, name, sizeof( stChat[0].name ) );
	Q_strncpyz( stChat[stChatNext].text, text, sizeof( stChat[0].text ) );
	stChatNext = ( stChatNext + 1 ) % ST_MAX_CHAT;
	return qfalse;
}

// whether one of the words, | between them, is in the text
static qboolean ST_HasWords( const char *text, const char *words ) {
	char word[ST_VALUE_LEN];

	while ( *words ) {
		int len = 0;

		while ( *words == '|' || *words == ' ' )
			words++;
		while ( *words && *words != '|' && len < (int)sizeof( word ) - 1 )
			word[len++] = *words++;
		while ( len && word[len - 1] == ' ' )
			len--;
		word[len] = '\0';
		if ( len && Q_stristr( text, word ) )
			return qtrue;
	}
	return qfalse;
}

// someone the step counts said its words since then; who, in by when it's given
static qboolean ST_ChatSaid( const stStep_t *s, int since, char *by, int bySize ) {
	const char *who = s->values[2];
	char mine[MAX_QPATH];

	Q_strncpyz( mine, Info_ValueForKey( cl.gameState.stringData + cl.gameState.stringOffsets[CS_PLAYERS + clc.clientNum], "n" ), sizeof( mine ) );
	Q_StripColor( mine );
	for ( int n = 0; n < ST_MAX_CHAT; n++ ) {
		const int i = ( stChatNext + n ) % ST_MAX_CHAT;

		if ( !stChat[i].time || stChat[i].time < since )
			continue;
		if ( !Q_stricmp( who, "me" ) && Q_stricmp( stChat[i].name, mine ) )
			continue;
		if ( !Q_stricmp( who, "named" ) ) {
			char typed[MAX_QPATH];

			Q_strncpyz( typed, s->values[3], sizeof( typed ) );
			Q_StripColor( typed );
			if ( !typed[0] || ( Q_stricmp( stChat[i].name, typed ) && ( strlen( typed ) < 3 || !Q_stristr( stChat[i].name, typed ) ) ) )
				continue;
		}
		if ( !ST_HasWords( stChat[i].text, s->values[1] ) )
			continue;
		if ( by )
			Q_strncpyz( by, stChat[i].name, bySize );
		return qtrue;
	}
	return qfalse;
}

/*
===============================================================================

FAR AWAY: who's out of sight, and the use triggers used

The server sends this client only who is near it. A storyteller whose stories wait on
areas or use triggers (armed, or the one playing) tells everyone with this client where and
which, in a hidden rpnotify, again every while for who joins later:
	cmd:story/watch/<its client number>: <map> | m <inline model> | s <x y z> <radius> | u <name> | ...
	cmd:story/unwatch/<its client number>
Their client says, in a tell hidden from both of them, where its player is each time they go
in or out of one, and every while they're in one; and that they used a named use trigger:
	tell <storyteller> ~st <client number> pos <x y z> | gone | use <name>
The players whose client doesn't say are asked of the server with rporigin, one a command,
when the storyteller's an admin that can. NPCs out of sight can't be found either way.

===============================================================================
*/

#define ST_FAR_MARKER		"~st "
#define ST_FAR_MS			90000	// someone out of sight is known this long after we're told
#define ST_WATCH_RESEND		30000	// the areas go out again this often
#define ST_WATCH_MS			75000	// a storyteller's areas last this long after they last came
#define ST_HEARTBEAT_MS		8000	// in an area, a player says so again this often
#define ST_POLL_REPLY_MS	3000
#define ST_MAX_AREAS		24
#define ST_MAX_WATCHERS		4
#define ST_MAX_USES			32

typedef struct {
	qboolean	sphere;				// near a spot; else a map trigger
	int			model;
	vec3_t		pos;
	float		radius;
} stArea_t;

// as a storyteller: the players out of sight, the areas sent, the server asked
static struct {
	vec3_t		org[MAX_CLIENTS];
	int			time[MAX_CLIENTS];		// 0 not known
	qboolean	reports[MAX_CLIENTS];	// their client says itself
	unsigned	sentSum;
	int			sentTime;
	qboolean	watchSent;
	int			pollNext, pollAsked, pollTime;
	qboolean	noPoll;					// rporigin isn't ours to use
} stFar = { { { 0 } }, { 0 }, { qfalse }, 0, 0, qfalse, 0, -1 };

// as a player: the storytellers watching, and their areas
static struct {
	qboolean	on;
	int			num;
	char		map[MAX_QPATH];
	stArea_t	areas[ST_MAX_AREAS];
	int			numAreas;
	int			time;					// when they last came
	unsigned	inMask;
	qboolean	told;					// it's been told where we are since they came
	int			lastSend;
} stWatchers[ST_MAX_WATCHERS];

// use triggers used, here or told
static struct {
	int		time;
	char	name[48];
	char	by[MAX_QPATH];
} stUses[ST_MAX_USES];
static int stUseNext;

static qboolean ST_Connected( int c ) {
	return (qboolean)( c >= 0 && c < MAX_CLIENTS && cl.gameState.stringOffsets[CS_PLAYERS + c]
		&& cl.gameState.stringData[cl.gameState.stringOffsets[CS_PLAYERS + c]] );
}

static qboolean ST_Spectating( int c ) {
	return (qboolean)( atoi( Info_ValueForKey( cl.gameState.stringData + cl.gameState.stringOffsets[CS_PLAYERS + c], "t" ) ) == TEAM_SPECTATOR );
}

// the players out of sight we know of, after the ones seen
static void ST_AddFar( stPerson_t *out, int *n, int max, const qboolean *seen ) {
	for ( int c = 0; c < MAX_CLIENTS && *n < max; c++ ) {
		if ( seen[c] || !stFar.time[c] || cls.realtime - stFar.time[c] > ST_FAR_MS || !ST_Connected( c ) || ST_Spectating( c ) )
			continue;
		out[*n].num = c;
		out[*n].npc = qfalse;
		VectorCopy( stFar.org[c], out[*n].org );
		( *n )++;
	}
}

// the areas a story's steps wait on
static void ST_StepAreas( const stStep_t *steps, int numSteps, stArea_t *out, int *n ) {
	for ( int i = 0; i < numSteps && *n < ST_MAX_AREAS; i++ ) {
		const stStep_t *s = &steps[i];
		stArea_t a;
		qboolean dup = qfalse;

		if ( !s->enabled || ( s->action != ST_A_TRIGGER && s->action != ST_A_IF ) || !ST_TriggerArea( s ) )
			continue;
		memset( &a, 0, sizeof( a ) );
		if ( ST_TriggerNear( s ) ) {
			if ( !ST_HavePos( s ) )
				continue;
			a.sphere = qtrue;
			ST_GetPos( s, a.pos );
			a.radius = ST_Number( s, 4 ) > 0 ? ST_Number( s, 4 ) : 96.0f;
		} else {
			a.model = ST_ModelNum( s->values[1] );
			if ( !a.model )
				continue;
		}
		for ( int k = 0; k < *n; k++ ) {
			if ( out[k].sphere == a.sphere && out[k].model == a.model && VectorCompare( out[k].pos, a.pos ) && out[k].radius == a.radius )
				dup = qtrue;
		}
		if ( !dup )
			out[( *n )++] = a;
	}
}

// the use triggers a story's steps wait on, by name
static void ST_StepUses( const stStep_t *steps, int numSteps, char ( *out )[48], int *n ) {
	for ( int i = 0; i < numSteps && *n < ST_MAX_AREAS; i++ ) {
		const stStep_t *s = &steps[i];
		qboolean dup = qfalse;

		if ( !s->enabled || ( s->action != ST_A_TRIGGER && s->action != ST_A_IF ) || !ST_TriggerUse( s ) || !s->values[1][0] )
			continue;
		for ( int k = 0; k < *n; k++ ) {
			if ( !Q_stricmp( out[k], s->values[1] ) )
				dup = qtrue;
		}
		if ( !dup )
			Q_strncpyz( out[( *n )++], s->values[1], sizeof( out[0] ) );
	}
}

static int ST_WatchedUses( char ( *out )[48] ) {
	int n = 0, runNum;

	for ( int i = 0; i < stNumArmed; i++ ) {
		if ( !Q_stricmp( stArmed[i].map, cl.mapname ) )
			ST_StepUses( stArmed[i].steps, stArmed[i].numSteps, out, &n );
	}
	if ( st.run.running )
		ST_StepUses( ST_RunSteps( &runNum ), runNum, out, &n );
	return n;
}

// every area waited on now: the stories armed for this map, and the one playing
static int ST_WatchedAreas( stArea_t *out ) {
	int n = 0, runNum;
	stStep_t *runSteps;

	for ( int i = 0; i < stNumArmed; i++ ) {
		if ( !Q_stricmp( stArmed[i].map, cl.mapname ) )
			ST_StepAreas( stArmed[i].steps, stArmed[i].numSteps, out, &n );
	}
	if ( st.run.running ) {
		runSteps = ST_RunSteps( &runNum );
		ST_StepAreas( runSteps, runNum, out, &n );
	}
	return n;
}

static unsigned ST_AreasSum( const stArea_t *areas, int n ) {
	unsigned sum = (unsigned)n;

	for ( int i = 0; i < n; i++ ) {
		sum = sum * 31 + ( areas[i].sphere ? 1 : 2 ) + areas[i].model * 7;
		for ( int k = 0; k < 3; k++ )
			sum = sum * 31 + (unsigned)ST_Round( areas[i].pos[k] );
		sum = sum * 31 + (unsigned)ST_Round( areas[i].radius );
	}
	return sum;
}

// as a storyteller, each frame: the areas out to everyone, the server asked about the rest
static void ST_FarFrame( void ) {
	stArea_t areas[ST_MAX_AREAS];
	char uses[ST_MAX_AREAS][48];
	const int n = cls.state == CA_ACTIVE ? ST_WatchedAreas( areas ) : 0;
	const int nu = cls.state == CA_ACTIVE ? ST_WatchedUses( uses ) : 0;
	unsigned sum = ST_AreasSum( areas, n );

	for ( int i = 0; i < nu; i++ ) {
		for ( const char *c = uses[i]; *c; c++ )
			sum = sum * 31 + (unsigned)tolower( *c );
		sum = sum * 31 + 1;
	}

	if ( cls.state != CA_ACTIVE || cls.realtime < st.nextSend )
		return;

	// where to watch, to everyone with this client
	if ( CL_RpHooks_Experimental() ) {
		if ( ( n || nu ) && ( !stFar.watchSent || sum != stFar.sentSum || cls.realtime - stFar.sentTime > ST_WATCH_RESEND ) ) {
			char text[MAX_STRING_CHARS];

			Com_sprintf( text, sizeof( text ), "rpnotify -1 1 cmd:story/watch/%i: %s", clc.clientNum, cl.mapname );
			for ( int i = 0; i < n; i++ ) {
				if ( areas[i].sphere )
					Q_strcat( text, sizeof( text ), va( " | s %i %i %i %i", ST_Round( areas[i].pos[0] ), ST_Round( areas[i].pos[1] ),
						ST_Round( areas[i].pos[2] ), ST_Round( areas[i].radius ) ) );
				else
					Q_strcat( text, sizeof( text ), va( " | m %i", areas[i].model ) );
			}
			for ( int i = 0; i < nu; i++ )
				Q_strcat( text, sizeof( text ), va( " | u %s", uses[i] ) );
			CL_AddReliableCommand( text, qfalse );
			stFar.watchSent = qtrue;
			stFar.sentSum = sum;
			stFar.sentTime = cls.realtime;
			st.nextSend = cls.realtime + ST_CommandGap();
			return;
		}
		if ( !n && !nu && stFar.watchSent ) {
			CL_AddReliableCommand( va( "rpnotify -1 1 cmd:story/unwatch/%i", clc.clientNum ), qfalse );
			stFar.watchSent = qfalse;
			st.nextSend = cls.realtime + ST_CommandGap();
			return;
		}
	}

	// the players out of sight that don't say where they are, one at a time
	if ( !n || stFar.noPoll || ( stFar.pollAsked >= 0 && cls.realtime - stFar.pollTime < ST_POLL_REPLY_MS ) )
		return;
	stFar.pollAsked = -1;
	for ( int k = 0; k < MAX_CLIENTS; k++ ) {
		const int c = ( stFar.pollNext + k ) % MAX_CLIENTS;
		qboolean inSight = qfalse;

		if ( c == clc.clientNum || !ST_Connected( c ) || ST_Spectating( c ) || stFar.reports[c] )
			continue;
		for ( int i = 0; i < cl.snap.numEntities; i++ ) {
			if ( ST_SnapEntity( i )->number == c )
				inSight = qtrue;
		}
		if ( inSight )
			continue;
		CL_AddReliableCommand( va( "rporigin %i", c ), qfalse );
		stFar.pollAsked = c;
		stFar.pollTime = cls.realtime;
		stFar.pollNext = c + 1;
		st.nextSend = cls.realtime + ST_CommandGap();
		return;
	}
}

// rporigin's answer, "Name: (x y z, yaw)", for the player asked about: kept, and out of sight
qboolean CL_Storyteller_ServerPrint( const char *text ) {
	char name[MAX_STRING_CHARS], line[MAX_STRING_CHARS];
	const char *paren;
	float x, y, z, yaw;
	const int c = stFar.pollAsked;

	if ( c < 0 || cls.realtime - stFar.pollTime > ST_POLL_REPLY_MS )
		return qfalse;
	Q_strncpyz( line, text, sizeof( line ) );
	Q_StripColor( line );
	// no right to: no more asking this map
	if ( Q_stristr( line, "permission" ) || Q_stristr( line, "not allowed" ) || Q_stristr( line, "unknown command" ) ) {
		stFar.noPoll = qtrue;
		stFar.pollAsked = -1;
		Com_Printf( S_COLOR_YELLOW "Storyteller: players out of sight can't be asked for (rporigin isn't allowed)\n" );
		return qtrue;
	}
	paren = strrchr( line, '(' );
	if ( !paren || sscanf( paren, "(%f %f %f, %f)", &x, &y, &z, &yaw ) != 4 )
		return qfalse;
	Q_strncpyz( name, Info_ValueForKey( cl.gameState.stringData + cl.gameState.stringOffsets[CS_PLAYERS + c], "n" ), sizeof( name ) );
	Q_StripColor( name );
	if ( Q_stricmpn( line, name, strlen( name ) ) )
		return qfalse;
	VectorSet( stFar.org[c], x, y, z );
	stFar.time[c] = cls.realtime;
	stFar.pollAsked = -1;
	return qtrue;
}

// a use trigger used, here or told
static void ST_LogUse( const char *name, const char *by ) {
	stUses[stUseNext].time = cls.realtime;
	Q_strncpyz( stUses[stUseNext].name, name, sizeof( stUses[0].name ) );
	Q_strncpyz( stUses[stUseNext].by, by, sizeof( stUses[0].by ) );
	stUseNext = ( stUseNext + 1 ) % ST_MAX_USES;
}

static const char *ST_ClientName( int c ) {
	static char name[MAX_QPATH];

	Q_strncpyz( name, Info_ValueForKey( cl.gameState.stringData + cl.gameState.stringOffsets[CS_PLAYERS + c], "n" ), sizeof( name ) );
	Q_StripColor( name );
	return name;
}

// someone the step counts used its trigger since then
static qboolean ST_UsedSince( const stStep_t *s, int since, char *by, int bySize ) {
	const char *who = s->values[2];

	for ( int n = 0; n < ST_MAX_USES; n++ ) {
		const int i = ( stUseNext + n ) % ST_MAX_USES;

		if ( !stUses[i].time || stUses[i].time < since || Q_stricmp( stUses[i].name, s->values[1] ) )
			continue;
		if ( !Q_stricmp( who, "any NPC" ) )
			continue;
		if ( !Q_stricmp( who, "me" ) && Q_stricmp( stUses[i].by, ST_ClientName( clc.clientNum ) ) )
			continue;
		if ( !Q_stricmp( who, "named" ) ) {
			char typed[MAX_QPATH];

			Q_strncpyz( typed, s->values[3], sizeof( typed ) );
			Q_StripColor( typed );
			if ( !typed[0] || ( Q_stricmp( stUses[i].by, typed ) && ( strlen( typed ) < 3 || !Q_stristr( stUses[i].by, typed ) ) ) )
				continue;
		}
		if ( by )
			Q_strncpyz( by, stUses[i].by, bySize );
		return qtrue;
	}
	return qfalse;
}

// a hidden message, without its colours: "~st <client> pos x y z | gone | use <name>"; qtrue if
// it was one, which never shows (the sender gets it back too)
static qboolean ST_FarMessage( const char *text ) {
	const char *p = text;
	char word[16];
	int c, used = 0;

	while ( *p == ' ' )
		p++;
	if ( Q_stricmpn( p, ST_FAR_MARKER, strlen( ST_FAR_MARKER ) ) )
		return qfalse;
	p += strlen( ST_FAR_MARKER );
	if ( sscanf( p, "%i %15s %n", &c, word, &used ) < 2 || c < 0 || c >= MAX_CLIENTS || c == clc.clientNum )
		return qtrue;
	p += used;
	if ( !Q_stricmp( word, "pos" ) ) {
		vec3_t org;

		if ( sscanf( p, "%f %f %f", &org[0], &org[1], &org[2] ) == 3 ) {
			VectorCopy( org, stFar.org[c] );
			stFar.time[c] = cls.realtime;
			stFar.reports[c] = qtrue;
		}
	} else if ( !Q_stricmp( word, "gone" ) ) {
		stFar.time[c] = 0;
		stFar.reports[c] = qtrue;
	} else if ( !Q_stricmp( word, "use" ) && *p ) {
		ST_LogUse( p, ST_ClientName( c ) );
	}
	return qtrue;
}

// cmd:story/watch/<num>: <map> | m <model> | s <x y z> <radius> | u <use trigger> | ...  and
// cmd:story/unwatch/<num>. The uses say only that it listens: every use is told
void CL_Storyteller_Hook( char **argv, int argc, const char *message ) {
	char copy[MAX_STRING_CHARS], *part, *next;
	int num, slot = -1;

	if ( argc < 2 )
		return;
	num = atoi( argv[1] );
	if ( num < 0 || num >= MAX_CLIENTS )
		return;
	for ( int i = 0; i < ST_MAX_WATCHERS; i++ ) {
		if ( stWatchers[i].on && stWatchers[i].num == num )
			slot = i;
	}
	if ( !Q_stricmp( argv[0], "unwatch" ) ) {
		if ( slot >= 0 )
			stWatchers[slot].on = qfalse;
		return;
	}
	if ( Q_stricmp( argv[0], "watch" ) )
		return;
	if ( slot < 0 ) {
		// a free one, or the oldest
		slot = 0;
		for ( int i = 0; i < ST_MAX_WATCHERS; i++ ) {
			if ( !stWatchers[i].on ) {
				slot = i;
				break;
			}
			if ( stWatchers[i].time < stWatchers[slot].time )
				slot = i;
		}
		memset( &stWatchers[slot], 0, sizeof( stWatchers[slot] ) );
	}
	stWatchers[slot].on = qtrue;
	stWatchers[slot].num = num;
	stWatchers[slot].time = cls.realtime;
	stWatchers[slot].numAreas = 0;
	Q_strncpyz( copy, message, sizeof( copy ) );
	part = copy;
	next = strchr( part, '|' );
	if ( next )
		*next++ = '\0';
	while ( *part == ' ' )
		part++;
	for ( char *t = part + strlen( part ); t > part && t[-1] == ' '; )
		*--t = '\0';
	Q_strncpyz( stWatchers[slot].map, part, sizeof( stWatchers[slot].map ) );
	for ( part = next; part && stWatchers[slot].numAreas < ST_MAX_AREAS; part = next ) {
		stArea_t *a = &stWatchers[slot].areas[stWatchers[slot].numAreas];
		char kind = 0;

		next = strchr( part, '|' );
		if ( next )
			*next++ = '\0';
		memset( a, 0, sizeof( *a ) );
		if ( sscanf( part, " %c %f %f %f %f", &kind, &a->pos[0], &a->pos[1], &a->pos[2], &a->radius ) == 5 && kind == 's' ) {
			a->sphere = qtrue;
			stWatchers[slot].numAreas++;
		} else if ( sscanf( part, " %c %i", &kind, &a->model ) == 2 && kind == 'm' && a->model > 0 && a->model < CM_NumInlineModels() ) {
			stWatchers[slot].numAreas++;
		}
	}
}

// the areas a player is in, a bit each
static unsigned ST_AreasHere( const stArea_t *areas, int n, const vec3_t org ) {
	unsigned mask = 0;

	for ( int i = 0; i < n; i++ ) {
		if ( areas[i].sphere ? Distance( org, areas[i].pos ) <= areas[i].radius : ST_InTrigger( areas[i].model, org ) )
			mask |= 1u << i;
	}
	return mask;
}

static void ST_TellWatcher( int num, const char *payload ) {
	CL_AddReliableCommand( va( "tell %i " ST_FAR_MARKER "%i %s", num, clc.clientNum, payload ), qfalse );
}

// as a player, each frame: where we are, to each storyteller watching, when we go in or out of
// one of their areas, and every while inside
static void ST_ReportFrame( void ) {
	const qboolean away = (qboolean)( !cl.snap.valid || cl.snap.ps.pm_type == PM_SPECTATOR || cl.snap.ps.stats[STAT_HEALTH] <= 0
		|| ( cl.snap.ps.pm_flags & PMF_FOLLOW ) );

	if ( cls.state != CA_ACTIVE )
		return;
	for ( int i = 0; i < ST_MAX_WATCHERS; i++ ) {
		qboolean sent = qfalse;
		unsigned mask;

		if ( !stWatchers[i].on )
			continue;
		if ( cls.realtime - stWatchers[i].time > ST_WATCH_MS || !ST_Connected( stWatchers[i].num ) ) {
			stWatchers[i].on = qfalse;
			continue;
		}
		if ( stWatchers[i].num == clc.clientNum || Q_stricmp( stWatchers[i].map, cl.mapname ) )
			continue;
		mask = away ? 0 : ST_AreasHere( stWatchers[i].areas, stWatchers[i].numAreas, cl.snap.ps.origin );
		if ( cls.realtime - stWatchers[i].lastSend < ST_CommandGap() )
			continue;
		if ( !stWatchers[i].told || mask != stWatchers[i].inMask || ( mask && cls.realtime - stWatchers[i].lastSend > ST_HEARTBEAT_MS ) ) {
			if ( away )
				ST_TellWatcher( stWatchers[i].num, "gone" );
			else
				ST_TellWatcher( stWatchers[i].num, va( "pos %i %i %i", ST_Round( cl.snap.ps.origin[0] ), ST_Round( cl.snap.ps.origin[1] ),
					ST_Round( cl.snap.ps.origin[2] ) ) );
			sent = qtrue;
		}
		if ( sent ) {
			stWatchers[i].told = qtrue;
			stWatchers[i].inMask = mask;
			stWatchers[i].lastSend = cls.realtime;
			// one a frame: the server's flood protection
			return;
		}
	}
}

// the map editor's use trigger by that name was used here: heard by our own stories, and told to
// the storytellers watching
void CL_Storyteller_TriggerUsed( const char *name ) {
	if ( !name || !name[0] )
		return;
	ST_LogUse( name, ST_ClientName( clc.clientNum ) );
	for ( int i = 0; i < ST_MAX_WATCHERS; i++ ) {
		if ( stWatchers[i].on && stWatchers[i].num != clc.clientNum && cls.realtime - stWatchers[i].time <= ST_WATCH_MS )
			ST_TellWatcher( stWatchers[i].num, va( "use %s", name ) );
	}
}

/*
===============================================================================

Picking with the managers

===============================================================================
*/

// a step's manager, opened on the world; what it would send the server comes back here
static void ST_StartPick( stPick_t pick ) {
	st.pick = pick;
	st.pickStep = st.stepList.sel;
	st.numPicked = 0;
	if ( pick == ST_PICK_MODEL )
		CL_ModelManager_OpenForStory();
	else if ( pick == ST_PICK_EFFECT )
		CL_EffectManager_OpenForStory();
	else if ( pick == ST_PICK_SHADER || pick == ST_PICK_OVERLAY || pick == ST_PICK_SKY )
		CL_ShaderManager_OpenForStory( pick == ST_PICK_OVERLAY ? 2 : pick == ST_PICK_SKY ? 3 : 0 );
	else if ( pick == ST_PICK_SOUND )
		CL_SoundManager_OpenForStory();
	else if ( pick == ST_PICK_MUSIC )
		CL_MusicManager_OpenForStory();
	else if ( pick == ST_PICK_LOCK )
		CL_MapEditor_OpenForStory();
	else
		CL_NpcManager_OpenForStory( (qboolean)( pick == ST_PICK_NPC_MANAGE ) );
}

static qboolean ST_PickOpen( void ) {
	switch ( st.pick ) {
	case ST_PICK_MODEL:		return CL_ModelManager_Active();
	case ST_PICK_EFFECT:	return CL_EffectManager_Active();
	case ST_PICK_SHADER:
	case ST_PICK_OVERLAY:
	case ST_PICK_SKY:		return CL_ShaderManager_Active();
	case ST_PICK_NPC_SPAWN:
	case ST_PICK_NPC_MANAGE:	return CL_NpcManager_Active();
	case ST_PICK_SOUND:		return CL_SoundManager_Active();
	case ST_PICK_MUSIC:		return CL_MusicManager_Active();
	case ST_PICK_LOCK:
	case ST_PICK_TRIGGER:	return CL_MapEditor_Active();
	default:				return qfalse;
	}
}

// the step's trigger, clicked in the map editor's free camera: 0 a map trigger, 1 a door or
// button, 2 a use trigger
static void ST_StartTriggerPick( int kind ) {
	st.pick = ST_PICK_TRIGGER;
	st.pickStep = st.stepList.sel;
	st.numPicked = 0;
	st.pickedTrigger[0] = '\0';
	CL_MapEditor_PickForStory( kind );
}

// the map editor's click: "*12" for a map trigger or a door, the name of a use trigger
void CL_Storyteller_TriggerPicked( const char *value ) {
	if ( st.pick != ST_PICK_TRIGGER )
		return;
	Q_strncpyz( st.pickedTrigger, value, sizeof( st.pickedTrigger ) );
	st.numPicked = 1;
}

// a manager opened from a step: what it sends goes in the story
qboolean CL_Storyteller_Picking( void ) {
	return (qboolean)( st.pick != ST_PICK_NONE && ST_PickOpen() );
}

// a manager about to send a command: qtrue keeps it for the story. Lists only read, they go
qboolean CL_Storyteller_Capture( const char *cmd ) {
	const char *last;

	if ( st.pick == ST_PICK_NONE || !ST_PickOpen() )
		return qfalse;
	last = strrchr( cmd, ' ' );
	if ( last && !Q_stricmp( last, " list" ) )
		return qfalse;
	if ( st.numPicked < ST_MAX_PICKED )
		Q_strncpyz( st.picked[st.numPicked++], cmd, sizeof( st.picked[0] ) );
	return qtrue;
}

static qboolean ST_IsNumber( const char *s ) {
	if ( *s == '-' || *s == '+' )
		s++;
	if ( !*s )
		return qfalse;
	for ( ; *s; s++ ) {
		if ( !isdigit( (unsigned char)*s ) && *s != '.' )
			return qfalse;
	}
	return qtrue;
}

static qboolean ST_AllNumbers( char **argv, int from, int to ) {
	for ( int i = from; i < to; i++ ) {
		if ( !ST_IsNumber( argv[i] ) )
			return qfalse;
	}
	return qtrue;
}

static void ST_Join( char **argv, int from, int to, char *out, int size ) {
	out[0] = '\0';
	for ( int i = from; i < to; i++ )
		Q_strcat( out, size, va( i > from ? " %s" : "%s", argv[i] ) );
}

// the NPC manager's commands: npc spawn, tele, and the ones the NPC command step runs
static qboolean ST_NpcStep( char **argv, int argc, stStep_t *s ) {
	const char *verb = argv[1], *target = argv[argc - 1], *command = NULL;
	char value[ST_VALUE_LEN] = "";

	// npc spawn [vehicle] <type> [<targetname> [<team> [<model> [<x y z yaw> [<scale>]]]]]
	if ( !Q_stricmp( verb, "spawn" ) ) {
		int at = 2;
		qboolean vehicle = qfalse;

		if ( !Q_stricmp( argv[at], "vehicle" ) && argc > 3 ) {
			vehicle = qtrue;
			at++;
		}
		if ( argc - at > 9 || ( argc - at >= 5 && !ST_AllNumbers( argv, at + 4, argc ) ) )
			return qfalse;
		s->action = ST_A_NPC_SPAWN;
		ST_SetDefaults( s );
		Q_strncpyz( s->values[0], argv[at], ST_VALUE_LEN );
		Q_strncpyz( s->values[3], vehicle ? "yes" : "no", ST_VALUE_LEN );
		if ( argc > at + 1 )
			Q_strncpyz( s->values[1], argv[at + 1], ST_VALUE_LEN );
		if ( argc > at + 2 )
			Q_strncpyz( s->values[2], !Q_stricmp( argv[at + 2], "player" ) || !Q_stricmp( argv[at + 2], "enemy" ) ? argv[at + 2] : "default", ST_VALUE_LEN );
		if ( argc > at + 3 )
			Q_strncpyz( s->values[4], argv[at + 3], ST_VALUE_LEN );
		if ( argc >= at + 8 ) {
			for ( int i = 0; i < 4; i++ )
				Q_strncpyz( s->values[6 + i], argv[at + 4 + i], ST_VALUE_LEN );
		} else {
			// in front of the player: where the storyteller stands now
			ST_FillHere( s );
		}
		if ( argc == at + 9 )
			Q_strncpyz( s->values[5], argv[at + 8], ST_VALUE_LEN );
		return qtrue;
	}

	// npc tele <x y z yaw> <target>
	if ( !Q_stricmp( verb, "tele" ) ) {
		if ( argc != 7 || !ST_AllNumbers( argv, 2, 6 ) )
			return qfalse;
		s->action = ST_A_NPC_TELE;
		Q_strncpyz( s->values[0], target, ST_VALUE_LEN );
		for ( int i = 0; i < 4; i++ )
			Q_strncpyz( s->values[1 + i], argv[2 + i], ST_VALUE_LEN );
		return qtrue;
	}

	if ( !Q_stricmp( verb, "kill" ) && argc == 3 && Q_stricmp( target, "spawners" ) ) {
		command = "kill";
	} else if ( !Q_stricmp( verb, "freeze" ) && argc == 4 && ( !strcmp( argv[3], "1" ) || !strcmp( argv[3], "0" ) ) ) {
		command = !strcmp( argv[3], "1" ) ? "freeze" : "unfreeze";
		target = argv[2];
	} else if ( ( !Q_stricmp( verb, "emote" ) || !Q_stricmp( verb, "dialog" ) ) && argc >= 4 ) {
		// the emote or dialog, then the target
		ST_Join( argv, 2, argc - 1, value, sizeof( value ) );
		if ( !Q_stricmp( value, "none" ) ) {
			command = !Q_stricmp( verb, "emote" ) ? "stop emote" : "no dialog";
			value[0] = '\0';
		} else {
			command = !Q_stricmp( verb, "emote" ) ? "emote" : "dialog";
		}
	} else if ( !Q_stricmp( verb, "follow" ) && argc == 4 ) {
		if ( !Q_stricmp( argv[2], "none" ) )
			command = "stop following";
		else if ( atoi( argv[2] ) == clc.clientNum && ST_IsNumber( argv[2] ) )
			command = "follow me";
	} else if ( !Q_stricmp( verb, "hologram" ) && argc == 4 ) {
		if ( !Q_stricmp( argv[2], "normal" ) )			command = "hologram";
		else if ( !Q_stricmp( argv[2], "projector" ) )	command = "projector";
		else if ( !Q_stricmp( argv[2], "off" ) )		command = "no hologram";
	}
	if ( !command )
		return qfalse;
	s->action = ST_A_NPC_CMD;
	Q_strncpyz( s->values[0], target, ST_VALUE_LEN );
	Q_strncpyz( s->values[1], command, ST_VALUE_LEN );
	Q_strncpyz( s->values[2], value, ST_VALUE_LEN );
	return qtrue;
}

// rpnotify -1 1 cmd:npc/talk/<npc>/<name>/<colour>/<emote>: <text>, as NPC talk sends it
static qboolean ST_TalkStep( const char *cmd, stStep_t *s ) {
	static const char prefix[] = "rpnotify -1 1 cmd:npc/talk/";
	const char *p = cmd + strlen( prefix ), *colon;

	if ( Q_stricmpn( cmd, prefix, strlen( prefix ) ) || !( colon = strchr( p, ':' ) ) )
		return qfalse;
	s->action = ST_A_NPC_TALK;
	ST_SetDefaults( s );
	// NPC, name, colour and emote, then the text
	for ( int part = 0; part < 4 && p < colon; part++ ) {
		const char *end = p;

		while ( end < colon && *end != '/' )
			end++;
		Q_strncpyz( s->values[part], p, Q_min( ST_VALUE_LEN, (int)( end - p ) + 1 ) );
		CL_RpHooks_Unescape( s->values[part] );
		p = end < colon ? end + 1 : end;
	}
	for ( p = colon + 1; *p == ' '; p++ )
		;
	Q_strncpyz( s->values[4], p, ST_VALUE_LEN );
	return qtrue;
}

// a manager's command as a step of its own kind, or one that runs it as it is
static void ST_StepFromCommand( const char *cmd, stStep_t *s ) {
	char line[MAX_STRING_CHARS];
	char *argv[16];
	int argc;

	memset( s, 0, sizeof( *s ) );
	s->enabled = qtrue;

	// rpnotify -1 1 cmd:sky/<r g b>/<spin>/<pitch yaw roll>/<x y z>, or cmd:sky/off: the shader manager's
	// sky look; its parts have spaces in them
	if ( !Q_stricmpn( cmd, "rpnotify -1 1 cmd:sky/", 22 ) ) {
		vec3_t tint = { 255, 255, 255 }, angles = { 0, 0, 0 }, shift = { 0, 0, 0 };
		float spin = 0.0f;

		s->action = ST_A_SKY;
		ST_SetDefaults( s );
		if ( !Q_stricmpn( cmd + 22, "off", 3 ) || !Q_stricmpn( cmd + 22, "reset", 5 ) ) {
			Q_strncpyz( s->values[0], "reset", ST_VALUE_LEN );
			return;
		}
		sscanf( cmd + 22, "%f %f %f/%f/%f %f %f/%f %f %f", &tint[0], &tint[1], &tint[2], &spin, &angles[0], &angles[1], &angles[2],
			&shift[0], &shift[1], &shift[2] );
		Com_sprintf( s->values[1], ST_VALUE_LEN, "%g %g %g", tint[0], tint[1], tint[2] );
		Com_sprintf( s->values[2], ST_VALUE_LEN, "%g", spin );
		Com_sprintf( s->values[3], ST_VALUE_LEN, "%g", angles[YAW] );
		Com_sprintf( s->values[4], ST_VALUE_LEN, "%g", angles[PITCH] );
		Com_sprintf( s->values[5], ST_VALUE_LEN, "%g", angles[ROLL] );
		Com_sprintf( s->values[6], ST_VALUE_LEN, "%g %g %g", shift[0], shift[1], shift[2] );
		return;
	}

	Q_strncpyz( line, cmd, sizeof( line ) );
	argc = ST_SplitLine( line, argv, ARRAY_LEN( argv ) );

	// rpnotify <who> <seconds> <text>, not a hook
	if ( argc >= 4 && !Q_stricmp( argv[0], "rpnotify" ) && ST_IsNumber( argv[2] ) && !Q_stristr( cmd, "cmd:" ) ) {
		s->action = ST_A_NOTIFY;
		ST_SetDefaults( s );
		Q_strncpyz( s->values[0], argv[1], ST_VALUE_LEN );
		Q_strncpyz( s->values[1], argv[2], ST_VALUE_LEN );
		s->values[2][0] = '\0';
		for ( int i = 3; i < argc; i++ ) {
			if ( i > 3 )
				Q_strcat( s->values[2], ST_VALUE_LEN, " " );
			Q_strcat( s->values[2], ST_VALUE_LEN, argv[i] );
		}
		return;
	}

	// rplock add <target> <rank> [name], rplock remove <target|all>
	if ( argc >= 3 && !Q_stricmp( argv[0], "rplock" ) && ( ( !Q_stricmp( argv[1], "add" ) && argc >= 4 ) || !Q_stricmp( argv[1], "remove" ) ) ) {
		s->action = ST_A_LOCK;
		ST_SetDefaults( s );
		if ( !Q_stricmp( argv[1], "remove" ) ) {
			Q_strncpyz( s->values[0], !Q_stricmp( argv[2], "all" ) ? "unlock all" : "unlock", ST_VALUE_LEN );
			if ( Q_stricmp( argv[2], "all" ) )
				Q_strncpyz( s->values[1], argv[2], ST_VALUE_LEN );
			return;
		}
		Q_strncpyz( s->values[0], "lock", ST_VALUE_LEN );
		Q_strncpyz( s->values[1], argv[2], ST_VALUE_LEN );
		Q_strncpyz( s->values[2], argv[3], ST_VALUE_LEN );
		for ( int i = 4; i < argc; i++ ) {
			if ( i > 4 )
				Q_strcat( s->values[3], ST_VALUE_LEN, " " );
			Q_strcat( s->values[3], ST_VALUE_LEN, argv[i] );
		}
		return;
	}

	// rpshader overlay add <shader> [fade x y width height], rpshader overlay remove <shader|number|all>
	if ( argc >= 4 && argc <= 9 && !Q_stricmp( argv[0], "rpshader" ) && !Q_stricmp( argv[1], "overlay" )
		&& ( !Q_stricmp( argv[2], "add" ) || !Q_stricmp( argv[2], "remove" ) ) ) {
		s->action = ST_A_OVERLAY;
		ST_SetDefaults( s );
		if ( !Q_stricmp( argv[2], "remove" ) && ( !Q_stricmp( argv[3], "all" ) || !strcmp( argv[3], "-1" ) ) ) {
			Q_strncpyz( s->values[0], "remove all", ST_VALUE_LEN );
			return;
		}
		Q_strncpyz( s->values[0], argv[2], ST_VALUE_LEN );
		for ( int i = 3; i < argc; i++ )
			Q_strncpyz( s->values[i - 2], argv[i], ST_VALUE_LEN );
		return;
	}

	// rpmodel add <model> <x y z> <pitch yaw roll> <scale> <solid>
	if ( argc == 10 && !Q_stricmp( argv[0], "rpmodel" ) && !Q_stricmp( argv[1], "add" ) && ST_AllNumbers( argv, 3, 10 ) ) {
		s->action = ST_A_MODEL;
		for ( int i = 0; i < 8; i++ )
			Q_strncpyz( s->values[i], argv[i + 2], ST_VALUE_LEN );
		Q_strncpyz( s->values[8], atoi( argv[9] ) ? "yes" : "no", ST_VALUE_LEN );
		return;
	}
	// rpeffect once|add <effect> [<x y z> [<pitch yaw roll>]]; one at the player, with no spot or
	// 0 0 0, is where the storyteller stands now
	if ( ( argc == 3 || argc == 6 || argc == 9 ) && !Q_stricmp( argv[0], "rpeffect" )
		&& ( !Q_stricmp( argv[1], "once" ) || !Q_stricmp( argv[1], "add" ) ) && ST_AllNumbers( argv, 3, argc ) ) {
		s->action = ST_A_EFFECT;
		Q_strncpyz( s->values[0], argv[2], ST_VALUE_LEN );
		Q_strncpyz( s->values[1], !Q_stricmp( argv[1], "add" ) ? "loop" : "once", ST_VALUE_LEN );
		for ( int i = 0; i < 6; i++ )
			Q_strncpyz( s->values[i + 2], i + 3 < argc ? argv[i + 3] : "0", ST_VALUE_LEN );
		if ( argc == 3 || ( !atof( argv[3] ) && !atof( argv[4] ) && !atof( argv[5] ) ) ) {
			char yaw[ST_VALUE_LEN];

			// its own angles, not the way the storyteller looks
			Q_strncpyz( yaw, s->values[6], sizeof( yaw ) );
			ST_FillHere( s );
			Q_strncpyz( s->values[6], yaw, ST_VALUE_LEN );
		}
		return;
	}
	// the effect manager's presets
	if ( argc >= 2 && !Q_stricmp( argv[0], "rpeffect" ) ) {
		const char *screen = NULL;

		if ( argc == 2 && !Q_stricmp( argv[1], "fadeout" ) )										screen = "fade out";
		else if ( argc == 3 && !Q_stricmp( argv[1], "remove" ) && !Q_stricmp( argv[2], "fadeout" ) )	screen = "fade in";
		else if ( argc == 2 && !Q_stricmp( argv[1], "alarm" ) )										screen = "alarm";
		else if ( argc == 3 && !Q_stricmp( argv[1], "remove" ) && !Q_stricmp( argv[2], "alarm" ) )	screen = "stop alarm";
		else if ( argc == 4 && !Q_stricmp( argv[1], "camerashake" ) )								screen = "camera shake";
		if ( screen ) {
			s->action = ST_A_SCREEN;
			ST_SetDefaults( s );
			Q_strncpyz( s->values[0], screen, ST_VALUE_LEN );
			if ( argc == 4 ) {
				Q_strncpyz( s->values[1], argv[2], ST_VALUE_LEN );
				Q_strncpyz( s->values[2], argv[3], ST_VALUE_LEN );
			}
			return;
		}
	}
	if ( argc >= 3 && !Q_stricmp( argv[0], "npc" ) && ST_NpcStep( argv, argc, s ) )
		return;
	// npcmanager walk <target> <route> [walk|run] [loop]
	if ( argc >= 4 && argc <= 6 && !Q_stricmp( argv[0], "npcmanager" ) && !Q_stricmp( argv[1], "walk" ) && Q_stricmp( argv[3], "stop" ) ) {
		s->action = ST_A_NPC_WALK;
		ST_SetDefaults( s );
		Q_strncpyz( s->values[0], argv[2], ST_VALUE_LEN );
		Q_strncpyz( s->values[1], argv[3], ST_VALUE_LEN );
		for ( int i = 4; i < argc; i++ ) {
			if ( !Q_stricmp( argv[i], "run" ) )
				Q_strncpyz( s->values[2], "run", ST_VALUE_LEN );
			else if ( !Q_stricmp( argv[i], "loop" ) )
				Q_strncpyz( s->values[3], "yes", ST_VALUE_LEN );
		}
		return;
	}
	if ( ST_TalkStep( cmd, s ) )
		return;
	// rpsound once|add|local <sound> [all | -1 | <player> | <x y z>]
	if ( argc >= 3 && !Q_stricmp( argv[0], "rpsound" )
		&& ( !Q_stricmp( argv[1], "once" ) || !Q_stricmp( argv[1], "add" ) || !Q_stricmp( argv[1], "local" ) )
		&& ( argc <= 4 || ( argc == 6 && ST_AllNumbers( argv, 3, 6 ) ) ) ) {
		s->action = ST_A_SOUND;
		ST_SetDefaults( s );
		Q_strncpyz( s->values[0], argv[2], ST_VALUE_LEN );
		Q_strncpyz( s->values[1], !Q_stricmp( argv[1], "add" ) ? "loop" : !Q_stricmp( argv[1], "local" ) ? "local" : "once", ST_VALUE_LEN );
		if ( argc == 3 ) {
			Q_strncpyz( s->values[2], !Q_stricmp( argv[1], "add" ) ? "everyone" : "me", ST_VALUE_LEN );
		} else if ( argc == 4 && ( !Q_stricmp( argv[3], "all" ) || !strcmp( argv[3], "-1" ) ) ) {
			Q_strncpyz( s->values[2], "everyone", ST_VALUE_LEN );
		} else if ( argc == 4 ) {
			Q_strncpyz( s->values[2], "player", ST_VALUE_LEN );
			Q_strncpyz( s->values[3], argv[3], ST_VALUE_LEN );
		} else {
			Q_strncpyz( s->values[2], "spot", ST_VALUE_LEN );
			for ( int i = 0; i < 3; i++ )
				Q_strncpyz( s->values[4 + i], argv[3 + i], ST_VALUE_LEN );
		}
		return;
	}
	// rpmusic <music> | default | reset | none
	if ( argc == 2 && !Q_stricmp( argv[0], "rpmusic" ) ) {
		s->action = ST_A_MUSIC;
		ST_SetDefaults( s );
		if ( !Q_stricmp( argv[1], "default" ) || !Q_stricmp( argv[1], "reset" ) ) {
			Q_strncpyz( s->values[0], "map default", ST_VALUE_LEN );
		} else if ( !Q_stricmp( argv[1], "none" ) ) {
			Q_strncpyz( s->values[0], "none", ST_VALUE_LEN );
		} else {
			Q_strncpyz( s->values[0], "play", ST_VALUE_LEN );
			Q_strncpyz( s->values[1], argv[1], ST_VALUE_LEN );
		}
		return;
	}
	// rpshader remap <from> <to> [<time offset>]
	if ( ( argc == 4 || argc == 5 ) && !Q_stricmp( argv[0], "rpshader" ) && !Q_stricmp( argv[1], "remap" ) ) {
		s->action = ST_A_SHADER;
		Q_strncpyz( s->values[0], argv[2], ST_VALUE_LEN );
		Q_strncpyz( s->values[1], argv[3], ST_VALUE_LEN );
		if ( argc == 5 )
			Q_strncpyz( s->values[2], argv[4], ST_VALUE_LEN );
		return;
	}
	s->action = ST_A_COMMAND;
	Q_strncpyz( s->values[0], cmd, ST_VALUE_LEN );
}

// what the manager sent, into the story: the first fills the step it was opened from if it's
// that kind, the rest go after it
static void ST_TakePicked( void ) {
	int from = st.pickStep;

	if ( from < 0 || from >= st.numSteps )
		from = st.numSteps - 1;
	ST_SelectStep( from );
	for ( int i = 0; i < st.numPicked; i++ ) {
		stStep_t step;

		ST_StepFromCommand( st.picked[i], &step );
		if ( i == 0 && from >= 0 && st.steps[from].action == step.action ) {
			step.enabled = st.steps[from].enabled;
			st.steps[from] = step;
			ST_Changed();
			continue;
		}
		if ( st.numSteps >= ST_MAX_STEPS ) {
			ST_Message( S_COLOR_RED "The story is full" );
			break;
		}
		ST_AddStep( step.action );
		st.steps[st.stepList.sel] = step;
	}
	ST_Message( va( S_COLOR_GREEN "%i step%s from the manager", st.numPicked, st.numPicked == 1 ? "" : "s" ) );
}

// every frame: once the manager has sent something, or was closed, back to the story
static void ST_PickFrame( void ) {
	if ( st.pick == ST_PICK_NONE )
		return;
	if ( ST_PickOpen() && !st.numPicked )
		return;
	CL_ModelManager_Close();
	CL_EffectManager_Close();
	CL_ShaderManager_Close();
	CL_NpcManager_Close();
	CL_SoundManager_Close();
	if ( st.pick == ST_PICK_LOCK || st.pick == ST_PICK_TRIGGER )
		CL_MapEditor_Close();
	// a trigger clicked: the step's
	if ( st.pick == ST_PICK_TRIGGER ) {
		st.pick = ST_PICK_NONE;
		if ( ST_Open() && st.numPicked && st.pickStep >= 0 && st.pickStep < st.numSteps && ST_Watches( &st.steps[st.pickStep] ) ) {
			ST_SelectStep( st.pickStep );
			Q_strncpyz( st.steps[st.pickStep].values[1], st.pickedTrigger, ST_VALUE_LEN );
			ST_Changed();
			ST_Message( va( S_COLOR_GREEN "Picked %s", st.pickedTrigger ) );
		}
		st.numPicked = 0;
		return;
	}
	st.pick = ST_PICK_NONE;
	// another tool opened from it stays
	if ( !( Key_GetCatcher() & KEYCATCH_ENGINETOOLS ) && ST_Open() && st.numPicked )
		ST_TakePicked();
	st.numPicked = 0;
}

void CL_Storyteller_Init( void ) {
	memset( &st, 0, sizeof( st ) );
	st.stepList.sel = -1;
	st.stepList.lastClickRow = -1;
	st.focus = ST_F_NONE;
}

// cgame is going away: map change, disconnect or vid_restart; the story stays, the play doesn't
void CL_Storyteller_Shutdown( void ) {
	CL_Storyteller_Close();
	ST_Stop( S_COLOR_YELLOW "Stopped: left the map" );
	// armed ones wait on their first step again, if the map is still theirs
	for ( int i = 0; i < stNumArmed; i++ )
		stArmed[i].run.running = qfalse;
	// who's where, and who watches, are the map's
	memset( &stFar, 0, sizeof( stFar ) );
	stFar.pollAsked = -1;
	memset( stWatchers, 0, sizeof( stWatchers ) );
	memset( stUses, 0, sizeof( stUses ) );
}

qboolean CL_Storyteller_Active( void ) {
	return st.open;
}

void CL_Storyteller_Frame( void ) {
	ST_PickFrame();
	ST_RunFrame();
	ST_FarFrame();
	ST_ReportFrame();
}

/*
===============================================================================

CONSOLE COMMAND

===============================================================================
*/

void CL_Storyteller_f( void ) {
	const char *arg = Cmd_Argv( 1 );

	if ( !arg[0] ) {
		if ( st.open )
			CL_Storyteller_Close();
		else
			ST_Open();
	} else if ( !Q_stricmp( arg, "close" ) ) {
		CL_Storyteller_Close();
	} else if ( !Q_stricmp( arg, "markers" ) ) {
		const char *to = Cmd_Argv( 2 );

		Cvar_SetValue( "cl_storyMarkers", !Q_stricmp( to, "on" ) ? 1 : !Q_stricmp( to, "off" ) ? 0 : !ST_MarkersOn() );
		Com_Printf( "Storyteller: markers %s\n", ST_MarkersOn() ? "on" : "off" );
	} else if ( !Q_stricmp( arg, "continue" ) ) {
		if ( !ST_AnyWaiting() )
			Com_Printf( "Storyteller: nothing is waiting for you\n" );
		ST_Continue();
	} else if ( !Q_stricmp( arg, "play" ) ) {
		if ( Cmd_Argc() >= 3 && !ST_Load( Cmd_Argv( 2 ) ) ) {
			Com_Printf( "Storyteller: no story called %s\n", Cmd_Argv( 2 ) );
			return;
		}
		ST_Run( 0 );
	} else if ( !Q_stricmp( arg, "stop" ) ) {
		if ( st.run.running )
			ST_Stop( S_COLOR_YELLOW "Stopped" );
	} else if ( !Q_stricmp( arg, "load" ) && Cmd_Argc() >= 3 ) {
		if ( ST_Load( Cmd_Argv( 2 ) ) )
			Com_Printf( "Storyteller: loaded %s, %i steps\n", st.name, st.numSteps );
		else
			Com_Printf( "Storyteller: no story called %s\n", Cmd_Argv( 2 ) );
	} else if ( !Q_stricmp( arg, "save" ) && Cmd_Argc() >= 3 ) {
		const char *name = Cmd_Argv( 2 );
		for ( const char *c = name; *c; c++ ) {
			if ( !ST_NameChar( *c ) ) {
				Com_Printf( "Storyteller: names take letters, digits, _ and -\n" );
				return;
			}
		}
		if ( ST_Save( name ) )
			Com_Printf( "Storyteller: saved %s\n", name );
	} else if ( !Q_stricmp( arg, "open" ) && Cmd_Argc() >= 3 ) {
		// in a tab of its own, as the Load list does
		if ( !ST_LoadInTab( Cmd_Argv( 2 ) ) )
			Com_Printf( "Storyteller: couldn't open %s\n", Cmd_Argv( 2 ) );
	} else if ( !Q_stricmp( arg, "arm" ) && Cmd_Argc() >= 3 ) {
		const char *how = Cmd_Argv( 3 );

		// once, again for every time, or a number of times
		if ( !how[0] )
			ST_ArmSaved( Cmd_Argv( 2 ), 1 );
		else if ( !Q_stricmp( how, "again" ) )
			ST_ArmSaved( Cmd_Argv( 2 ), 0 );
		else if ( ST_IsNumber( how ) && atoi( how ) >= 1 )
			ST_ArmSaved( Cmd_Argv( 2 ), atoi( how ) );
		else
			Com_Printf( "Storyteller: arm it once, again (every time), or a number of times\n" );
	} else if ( !Q_stricmp( arg, "disarm" ) ) {
		const char *name = Cmd_Argv( 2 );
		const int i = ST_FindArmed( name );

		if ( !name[0] || !Q_stricmp( name, "all" ) ) {
			while ( stNumArmed )
				ST_Disarm( 0 );
			Com_Printf( "Storyteller: nothing is armed now\n" );
		} else if ( i < 0 ) {
			Com_Printf( "Storyteller: %s isn't armed\n", name );
		} else {
			ST_Disarm( i );
			Com_Printf( "Storyteller: %s disarmed\n", name );
		}
	} else if ( !Q_stricmp( arg, "armed" ) ) {
		Com_Printf( "Storyteller: %i armed\n", stNumArmed );
		for ( int i = 0; i < stNumArmed; i++ ) {
			const stArmed_t *a = &stArmed[i];
			Com_Printf( "  %s: %s, %s\n", a->name, ST_ArmedState( a ),
				ST_AtTrigger( &a->run, a->steps, a->numSteps ) ? "waiting" : "playing" );
		}
	} else if ( !Q_stricmp( arg, "list" ) ) {
		ST_RefreshStories();
		Com_Printf( "Storyteller: %i stor%s\n", st.numStories, st.numStories == 1 ? "y" : "ies" );
		for ( int i = 0; i < st.numStories; i++ )
			Com_Printf( "  %s\n", st.stories[i] );
	} else {
		Com_Printf( "usage: storyteller                  toggle the storyteller (bind a key to it)\n" );
		Com_Printf( "       storyteller play [name]      play the story, or load that one and play it\n" );
		Com_Printf( "       storyteller continue         go on past a \"wait for me\" step (bind a key to it)\n" );
		Com_Printf( "       storyteller stop             stop playing\n" );
		Com_Printf( "       storyteller load <name>      load a saved story\n" );
		Com_Printf( "       storyteller open <name>      open a saved story in a tab of its own\n" );
		Com_Printf( "       storyteller save <name>      save the story\n" );
		Com_Printf( "       storyteller list             list the saved stories\n" );
		Com_Printf( "       storyteller arm <name> [again|<times>]  arm a saved story: it plays when its first step\n" );
		Com_Printf( "                                    is set off, once, every time with again, or that many times\n" );
		Com_Printf( "       storyteller disarm [name|all]  disarm one, or all\n" );
		Com_Printf( "       storyteller armed            list the armed stories\n" );
		Com_Printf( "       storyteller markers [on|off] where the armed stories wait, drawn on your screen\n" );
		Com_Printf( "       storyteller close            close the storyteller\n" );
	}
}

/*
===============================================================================

INPUT

===============================================================================
*/

static qboolean ST_InRect( float x, float y, float w, float h ) {
	return (qboolean)( st.cursorX >= x && st.cursorX < x + w && st.cursorY >= y && st.cursorY < y + h );
}

// binds don't run while the storyteller holds the keys, so honour the user's toggle bind here
static qboolean ST_IsToggleKey( int key ) {
	const char *binding;

	// a printable key belongs to the text box
	if ( st.focus != ST_F_NONE && key >= A_SPACE && key <= A_TILDE )
		return qfalse;
	binding = Key_GetBinding( key );
	return (qboolean)( VALIDSTRING( binding ) && !Q_stricmp( binding, ST_TOGGLE_CMD ) );
}

// the text being typed in, NULL if none
static char *ST_FocusText( int *size, stKind_t *kind ) {
	stStep_t *s;

	*kind = ST_K_TEXT;
	if ( st.focus == ST_F_SAVE_NAME ) {
		*size = sizeof( st.saveName );
		return st.saveName;
	}
	s = ST_SelectedStep();
	if ( st.focus < 0 || !s || st.focus >= ST_Def( s )->numFields || ST_Def( s )->fields[st.focus].kind == ST_K_CHOICE )
		return NULL;
	*size = ST_VALUE_LEN;
	*kind = ST_Def( s )->fields[st.focus].kind;
	return s->values[st.focus];
}

// the next text field of the step, or none past the last
static void ST_NextField( void ) {
	stStep_t *s = ST_SelectedStep();

	if ( !s || st.focus < 0 )
		return;
	for ( int i = st.focus + 1; i < ST_Def( s )->numFields; i++ ) {
		if ( ST_Def( s )->fields[i].kind != ST_K_CHOICE && ST_FieldShown( s, i ) ) {
			st.focus = i;
			return;
		}
	}
	st.focus = ST_F_NONE;
}

// a character the focused box takes
static qboolean ST_AllowChar( int ch ) {
	int size;
	stKind_t kind;

	if ( !ST_FocusText( &size, &kind ) || ch < ' ' || ch == '"' || ch > 126 )
		return qfalse;
	if ( st.focus == ST_F_SAVE_NAME )
		return ST_NameChar( ch );
	if ( kind == ST_K_NUMBER && !isdigit( ch ) && ch != '-' && ch != '.' )
		return qfalse;
	return qtrue;
}

static void ST_ListMove( stList_t *list, int count, int delta ) {
	if ( !count )
		return;
	list->sel = Com_Clampi( 0, count - 1, list->sel + delta );
}

void CL_Storyteller_KeyEvent( int key, qboolean down ) {
	int size;
	stKind_t kind;
	char *text;

	if ( !st.open || !down )
		return;

	if ( ST_IsToggleKey( key ) ) {
		CL_Storyteller_Close();
		return;
	}
	if ( st.popup == ST_POPUP_EXPERIMENTAL && key != A_MOUSE1 )
		return;

	switch ( key ) {
	case A_MOUSE1:
		st.click = qtrue;
		st.clickX = st.cursorX;
		st.clickY = st.cursorY;
		return;
	case A_MWHEELUP:
		st.wheel -= 3;
		return;
	case A_MWHEELDOWN:
		st.wheel += 3;
		return;
	default:
		break;
	}

	text = ST_FocusText( &size, &kind );
	if ( text ) {
		const int edit = WG_EditKey( text, size, key, ST_AllowChar );

		if ( edit ) {
			if ( ( edit & WG_EDIT_CHANGED ) && st.focus != ST_F_SAVE_NAME )
				ST_Changed();
			return;
		}
		switch ( key ) {
		case A_ENTER:
		case A_KP_ENTER:
			if ( st.focus == ST_F_SAVE_NAME )
				ST_DoSave();
			else
				st.focus = ST_F_NONE;
			return;
		case A_TAB:
			if ( st.focus != ST_F_SAVE_NAME )
				ST_NextField();
			return;
		default:
			return;
		}
	}

	if ( st.popup == ST_POPUP_LOAD ) {
		switch ( key ) {
		case A_CURSOR_UP:
			ST_ListMove( &st.storyList, st.numStories, -1 );
			break;
		case A_CURSOR_DOWN:
			ST_ListMove( &st.storyList, st.numStories, 1 );
			break;
		case A_ENTER:
		case A_KP_ENTER:
			if ( st.storyList.sel >= 0 && st.storyList.sel < st.numStories && ST_LoadInTab( st.stories[st.storyList.sel] ) )
				st.popup = ST_POPUP_NONE;
			break;
		default:
			break;
		}
		return;
	}
	if ( st.popup != ST_POPUP_NONE )
		return;

	switch ( key ) {
	case A_CURSOR_UP:
		if ( Key_IsDown( A_CTRL ) ) {
			ST_MoveStep( -1 );
		} else {
			ST_ClearMarks();
			ST_SelectStep( Com_Clampi( 0, st.numSteps - 1, st.stepList.sel - 1 ) );
		}
		break;
	case A_CURSOR_DOWN:
		if ( Key_IsDown( A_CTRL ) ) {
			ST_MoveStep( 1 );
		} else if ( st.numSteps ) {
			ST_ClearMarks();
			ST_SelectStep( Com_Clampi( 0, st.numSteps - 1, st.stepList.sel + 1 ) );
		}
		break;
	case A_DELETE:
		ST_DeleteStep();
		break;
	case A_LOW_C:
	case A_CAP_C:
		if ( Key_IsDown( A_CTRL ) )
			ST_CopySteps( qfalse );
		break;
	case A_LOW_X:
	case A_CAP_X:
		if ( Key_IsDown( A_CTRL ) )
			ST_CopySteps( qtrue );
		break;
	case A_LOW_V:
	case A_CAP_V:
		if ( Key_IsDown( A_CTRL ) )
			ST_PasteSteps();
		break;
	case A_LOW_A:
	case A_CAP_A:
		if ( Key_IsDown( A_CTRL ) ) {
			for ( int i = 0; i < st.numSteps; i++ )
				st.marked[i] = 1;
		}
		break;
	case A_ENTER:
	case A_KP_ENTER:
		if ( st.actionList.sel >= 0 && st.actionList.sel < st.numActionRows && !st.actionRows[st.actionList.sel].header
			&& ST_CanAdd( st.actionRows[st.actionList.sel].action ) )
			ST_AddRow( &st.actionRows[st.actionList.sel] );
		break;
	default:
		break;
	}
}

void CL_Storyteller_CharEvent( int ch ) {
	int size;
	stKind_t kind;
	char *text;

	if ( !st.open )
		return;
	text = ST_FocusText( &size, &kind );
	if ( text && WG_EditChar( text, size, ch, ST_AllowChar ) && st.focus != ST_F_SAVE_NAME )
		ST_Changed();
}

void CL_Storyteller_MouseEvent( int dx, int dy ) {
	st.cursorX = Com_Clamp( 0, SCREEN_WIDTH, st.cursorX + dx );
	st.cursorY = Com_Clamp( 0, SCREEN_HEIGHT, st.cursorY + dy );
}

// escape backs out one level: a popup, a text box, then the storyteller
void CL_Storyteller_Escape( void ) {
	if ( st.popup != ST_POPUP_NONE ) {
		st.popup = ST_POPUP_NONE;
		st.focus = ST_F_NONE;
	} else if ( st.focus != ST_F_NONE ) {
		st.focus = ST_F_NONE;
	} else {
		CL_Storyteller_Close();
	}
}

/*
===============================================================================

2D

===============================================================================
*/

static void ST_Fill( float x, float y, float w, float h, const float *color ) {
	re->SetColor( color );
	re->DrawStretchPic( x, y, w, h, 0, 0, 0, 0, cls.whiteShader );
	re->SetColor( NULL );
}

static void ST_Box( float x, float y, float w, float h, const float *fill ) {
	ST_Fill( x, y, w, h, fill );
	ST_Fill( x, y, w, 1, stBorder );
	ST_Fill( x, y + h - 1, w, 1, stBorder );
	ST_Fill( x, y, 1, h, stBorder );
	ST_Fill( x + w - 1, y, 1, h, stBorder );
}

static void ST_Text( float x, float y, const char *text, const float *color ) {
	re->Font_DrawString( (int)x, (int)y, text, color, st.font | STYLE_DROPSHADOW, -1, ST_TEXT_SCALE );
}

static float ST_TextWidth( const char *text ) {
	return (float)re->Font_StrLenPixels( text, st.font, ST_TEXT_SCALE );
}

static void ST_TextClipped( float x, float y, float w, const char *text, const float *color ) {
	char buf[MAX_STRING_CHARS];
	int len;

	Q_strncpyz( buf, text, sizeof( buf ) );
	len = strlen( buf );
	while ( len > 3 && ST_TextWidth( buf ) > w ) {
		len--;
		buf[len - 3] = '.';
		buf[len - 2] = '.';
		buf[len - 1] = '.';
		buf[len] = '\0';
	}
	ST_Text( x, y, buf, color );
}

// what was typed, colour codes shown rather than applied: each ^ is drawn on its own
static float ST_RawWidth( const char *text ) {
	char seg[MAX_STRING_CHARS];
	float w = 0;

	while ( *text ) {
		int len = 0;
		if ( *text == Q_COLOR_ESCAPE ) {
			w += ST_TextWidth( "^" );
			text++;
			continue;
		}
		while ( text[len] && text[len] != Q_COLOR_ESCAPE && len < (int)sizeof( seg ) - 1 ) {
			seg[len] = text[len];
			len++;
		}
		seg[len] = '\0';
		w += ST_TextWidth( seg );
		text += len;
	}
	return w;
}

static void ST_RawText( float x, float y, const char *text, const float *color ) {
	char seg[MAX_STRING_CHARS];

	while ( *text ) {
		int len = 0;
		if ( *text == Q_COLOR_ESCAPE ) {
			ST_Text( x, y, "^", color );
			x += ST_TextWidth( "^" );
			text++;
			continue;
		}
		while ( text[len] && text[len] != Q_COLOR_ESCAPE && len < (int)sizeof( seg ) - 1 ) {
			seg[len] = text[len];
			len++;
		}
		seg[len] = '\0';
		ST_Text( x, y, seg, color );
		x += ST_TextWidth( seg );
		text += len;
	}
}

// text broken into lines at spaces; returns how many lines it took. raw shows colour codes
static int ST_TextWrappedEx( float x, float y, float w, const char *text, const float *color, int maxLines, qboolean raw ) {
	char line[MAX_STRING_CHARS];
	int lines = 0;

	while ( *text && lines < maxLines ) {
		int len = 0, fit = 0;

		while ( text[len] && len < (int)sizeof( line ) - 1 ) {
			line[len] = text[len];
			line[len + 1] = '\0';
			if ( ( raw ? ST_RawWidth( line ) : ST_TextWidth( line ) ) > w )
				break;
			if ( text[len] == ' ' || !text[len + 1] )
				fit = len + 1;
			len++;
		}
		if ( !text[len] )
			fit = len;
		if ( !fit )
			fit = Q_max( len, 1 );
		Q_strncpyz( line, text, fit + 1 );
		if ( raw )
			ST_RawText( x, y + lines * ST_ROW_H, line, color );
		else
			ST_Text( x, y + lines * ST_ROW_H, line, color );
		lines++;
		text += fit;
		while ( *text == ' ' )
			text++;
	}
	return lines;
}

static int ST_TextWrapped( float x, float y, float w, const char *text, const float *color, int maxLines ) {
	return ST_TextWrappedEx( x, y, w, text, color, maxLines, qfalse );
}

// eats this frame's click if it landed in the rect
static qboolean ST_Clicked( float x, float y, float w, float h ) {
	if ( !st.click || st.clickX < x || st.clickX >= x + w || st.clickY < y || st.clickY >= y + h )
		return qfalse;
	st.click = qfalse;
	return qtrue;
}

static qboolean ST_Button( float x, float y, float w, const char *label, qboolean selected, qboolean enabled ) {
	const float *fill = selected ? stHighlight : ( enabled && ST_InRect( x, y, w, ST_CTRL_H ) ) ? stHoverBox : stPanelLight;

	ST_Box( x, y, w, ST_CTRL_H, fill );
	ST_Text( x + ( w - ST_TextWidth( label ) ) * 0.5f, y + 2, label, enabled ? stWhite : stDim );
	return (qboolean)( ST_Clicked( x, y, w, ST_CTRL_H ) && enabled );
}

static void ST_FieldStyle( wgStyle_t *style ) {
	style->font = st.font;
	style->scale = ST_TEXT_SCALE;
	style->lineH = ST_ROW_H;
	style->padX = 4.0f;
	style->padY = 2.0f;
	style->fill = stPanelFocus;
	style->border = stBorder;
	style->text = stWhite;
	style->dim = stDim;
}

// a text box; its height now, grown down while it's typed in and long
static float ST_TextBox( float x, float y, float w, char *text, qboolean focused, const char *placeholder ) {
	wgStyle_t style;

	ST_FieldStyle( &style );
	ST_Box( x, y, w, ST_CTRL_H, focused ? stPanelFocus : stPanelLight );
	WG_Field( &style, x, y, w, ST_CTRL_H, text, focused, placeholder, st.cursorX, st.cursorY );
	return WG_FieldHeight( &style, w, ST_CTRL_H, text, focused );
}

// a click in a text box: it's typed in, the cursor where it was clicked
static qboolean ST_TextBoxClicked( float x, float y, float w, float h, char *text ) {
	if ( !ST_Clicked( x, y, w, h ) )
		return qfalse;
	WG_FieldClick( text, st.clickX, st.clickY );
	return qtrue;
}

typedef void ( *stRowFunc_t )( int row, float x, float y, float w );

// returns the row double-clicked, -1 if none
static int ST_List( stList_t *list, int count, float x, float y, float w, float h, stRowFunc_t drawRow, const float *rowFill( int row ) ) {
	const int visible = (int)( h / ST_ROW_H );
	int row, picked = -1;

	if ( st.wheel && ST_InRect( x, y, w, h ) ) {
		list->scroll += st.wheel;
		st.wheel = 0;
	}
	// the selection brought into view when it moves, not again while the list is scrolled
	if ( list->sel != list->shownSel ) {
		list->shownSel = list->sel;
		if ( list->sel >= 0 && list->sel < list->scroll )
			list->scroll = list->sel;
		else if ( list->sel >= list->scroll + visible )
			list->scroll = list->sel - visible + 1;
	}
	list->scroll = Com_Clampi( 0, Q_max( 0, count - visible ), list->scroll );
	if ( count > visible ) {
		ST_Box( x, y, w, h, stPanelLight );
		WG_Scrollbar( x + w - WG_SCROLL_W - 1, y + 1, h - 2, count, visible, &list->scroll, st.cursorX, st.cursorY, &st.click, st.clickX, st.clickY );
		w -= WG_SCROLL_W + 1;
	}

	if ( ST_Clicked( x, y, w, h ) ) {
		row = list->scroll + (int)( ( st.clickY - y ) / ST_ROW_H );
		if ( row < count ) {
			if ( row == list->lastClickRow && cls.realtime - list->lastClickTime < ST_DOUBLECLICK_MS ) {
				list->lastClickTime = 0;
				picked = row;
			} else {
				list->lastClickRow = row;
				list->lastClickTime = cls.realtime;
			}
			list->sel = row;
		}
	}

	if ( count <= visible )
		ST_Box( x, y, w, h, stPanelLight );
	for ( int i = 0; i < visible; i++ ) {
		const float ry = y + i * ST_ROW_H;
		const float *fill;

		row = list->scroll + i;
		if ( row >= count )
			break;
		fill = rowFill ? rowFill( row ) : NULL;
		if ( row == list->sel )
			ST_Fill( x + 1, ry, w - 2, ST_ROW_H, stHighlight );
		else if ( fill )
			ST_Fill( x + 1, ry, w - 2, ST_ROW_H, fill );
		else if ( ST_InRect( x, ry, w, ST_ROW_H ) )
			ST_Fill( x + 1, ry, w - 2, ST_ROW_H, stHover );
		drawRow( row, x + 4, ry + 1, w - 8 );
	}
	return picked;
}

static void ST_DrawActionRow( int row, float x, float y, float w ) {
	const stActionRow_t *r = &st.actionRows[row];

	if ( r->header )
		ST_Text( x, y, va( S_COLOR_YELLOW "%s", r->header ), stWhite );
	else if ( ( ST_Experimental( r->action ) && !CL_RpHooks_Experimental() )
		|| ( ST_IsCondition( r->action ) && !ST_UnderWhen( st.steps, ST_InsertAt() ) ) )
		ST_TextClipped( x + 8, y, w - 8, va( S_COLOR_GREY "%s", ST_RowName( r ) ), stWhite );
	else
		ST_TextClipped( x + 8, y, w - 8, ST_RowName( r ), stWhite );
}

static void ST_DrawStepRow( int row, float x, float y, float w ) {
	const stStep_t *s = &st.steps[row];
	const float *color = !s->enabled ? stDim : ST_StepUsable( s ) ? stWhite : stAccent;
	char num[16];

	// not va(): building the step's command goes through it too
	Com_sprintf( num, sizeof( num ), "%i", row + 1 );
	const char *summary = ST_StepSummary( s );

	// inside an If, indented
	const float indent = 24.0f + Q_min( 6, ST_Depth( st.steps, row ) ) * 10.0f;

	ST_Text( x + 18 - ST_TextWidth( num ), y, num, stDim );
	if ( s->enabled && ST_IsCondition( s->action ) && !ST_UnderWhen( st.steps, row ) ) {
		ST_TextClipped( x + indent, y, w - indent, va( S_COLOR_RED "needs a When above: " S_COLOR_WHITE "%s", summary ), stWhite );
		return;
	}
	ST_TextClipped( x + indent, y, w - indent, s->enabled ? summary : va( "(off) %s", summary ),
		s->enabled && ( s->action == ST_A_IF || s->action == ST_A_ELSE || s->action == ST_A_ENDIF ) && ST_StepUsable( s ) ? stAccent : color );
}

static const float *ST_StepFill( int row ) {
	if ( ST_RunHere() && row == st.run.pc )
		return stRunning;
	return st.marked[row] ? stMarked : NULL;
}

static void ST_DrawActions( void ) {
	const float listY = ST_TOP_Y + 14;
	const stActionRow_t *r;
	int picked;

	ST_Text( ST_ACTIONS_X, ST_TOP_Y, "Actions", stDim );
	picked = ST_List( &st.actionList, st.numActionRows, ST_ACTIONS_X, listY, ST_ACTIONS_W, ST_ACTIONS_H, ST_DrawActionRow, NULL );
	// headers aren't picked
	if ( st.actionList.sel >= 0 && st.actionList.sel < st.numActionRows && st.actionRows[st.actionList.sel].header
		&& st.actionList.sel + 1 < st.numActionRows )
		st.actionList.sel++;
	if ( picked >= 0 && !st.actionRows[picked].header && ST_CanAdd( st.actionRows[picked].action ) )
		ST_AddRow( &st.actionRows[picked] );

	if ( st.actionList.sel < 0 || st.actionList.sel >= st.numActionRows )
		return;
	r = &st.actionRows[st.actionList.sel];
	if ( r->header )
		return;
	const qboolean off = (qboolean)( ST_Experimental( r->action ) && !CL_RpHooks_Experimental() );

	const qboolean noWhen = (qboolean)( ST_IsCondition( r->action ) && !ST_UnderWhen( st.steps, ST_InsertAt() ) );

	if ( ST_Button( ST_ACTIONS_X, listY + ST_ACTIONS_H + 4, ST_ACTIONS_W, va( "Add \"%s\"", ST_RowName( r ) ), qfalse, (qboolean)( !off && !noWhen ) ) )
		ST_AddRow( r );
	ST_TextWrapped( ST_ACTIONS_X, listY + ST_ACTIONS_H + 24, ST_ACTIONS_W,
		off ? va( S_COLOR_YELLOW "Experimental, off: " RP_EXPERIMENTAL_LABEL " turns it on. " S_COLOR_GREY "%s", ST_RowDesc( r ) )
		: noWhen ? va( S_COLOR_YELLOW "Goes under a When step: add one, then select it. " S_COLOR_GREY "%s", ST_RowDesc( r ) )
		: ST_RowDesc( r ), stDim, 7 );
}

static void ST_DrawSteps( void ) {
	const float listY = ST_TOP_Y + 14, by = listY + ST_STEPS_H + 4, bw = ( ST_STEPS_W - 20 ) / 6;
	stStep_t *s = ST_SelectedStep();
	int picked;

	ST_Text( ST_STEPS_X, ST_TOP_Y, va( "Story" S_COLOR_GREY "  %i step%s", st.numSteps, st.numSteps == 1 ? "" : "s" ), stWhite );
	if ( st.numSteps ) {
		const int before = st.stepList.sel;
		const qboolean ctrl = (qboolean)( Key_IsDown( A_CTRL ) != 0 ), shift = (qboolean)( Key_IsDown( A_SHIFT ) != 0 );
		int row = -1;

		// the row a click is on, before the list takes it
		if ( st.click && ST_InRect( ST_STEPS_X, listY, ST_STEPS_W - ( st.numSteps > (int)( ST_STEPS_H / ST_ROW_H ) ? WG_SCROLL_W + 1 : 0 ), ST_STEPS_H ) ) {
			row = st.stepList.scroll + (int)( ( st.clickY - listY ) / ST_ROW_H );
			if ( row >= st.numSteps )
				row = -1;
		}
		picked = ST_List( &st.stepList, st.numSteps, ST_STEPS_X, listY, ST_STEPS_W, ST_STEPS_H, ST_DrawStepRow, ST_StepFill );
		if ( st.stepList.sel != before )
			st.focus = ST_F_NONE;
		if ( row >= 0 && ctrl ) {
			// one more, or one less; the one selected before is one of them
			if ( before >= 0 && before < st.numSteps && before != row ) {
				qboolean any = qfalse;

				for ( int i = 0; i < st.numSteps && !any; i++ )
					any = (qboolean)( st.marked[i] != 0 );
				if ( !any )
					st.marked[before] = 1;
			}
			st.marked[row] = (byte)!st.marked[row];
			picked = -1;
		} else if ( row >= 0 && shift ) {
			// all of them from the last one clicked
			ST_ClearMarks();
			for ( int i = Q_min( st.anchor, row ); i <= Q_max( st.anchor, row ) && i < st.numSteps; i++ )
				st.marked[i] = 1;
			picked = -1;
		} else if ( row >= 0 ) {
			ST_ClearMarks();
			st.anchor = row;
		}
		// double-click plays from there
		if ( picked >= 0 )
			ST_Run( picked );
	} else {
		ST_Box( ST_STEPS_X, listY, ST_STEPS_W, ST_STEPS_H, stPanelLight );
		ST_TextWrapped( ST_STEPS_X + 8, listY + 8, ST_STEPS_W - 16,
			"A blank slate. Pick an action on the left and click Add, or double-click it: steps go after the selected one, and play from top to bottom.", stDim, 6 );
	}

	if ( ST_Button( ST_STEPS_X, by, bw, "Up", qfalse, (qboolean)( s && st.stepList.sel > 0 ) ) )
		ST_MoveStep( -1 );
	if ( ST_Button( ST_STEPS_X + bw + 4, by, bw, "Down", qfalse, (qboolean)( s && st.stepList.sel < st.numSteps - 1 ) ) )
		ST_MoveStep( 1 );
	if ( ST_Button( ST_STEPS_X + ( bw + 4 ) * 2, by, bw, "Copy", qfalse, (qboolean)( s != NULL ) ) )
		ST_CopySteps( qfalse );
	if ( ST_Button( ST_STEPS_X + ( bw + 4 ) * 3, by, bw, "Cut", qfalse, (qboolean)( s != NULL ) ) )
		ST_CopySteps( qtrue );
	if ( ST_Button( ST_STEPS_X + ( bw + 4 ) * 4, by, bw, stNumClip ? va( "Paste %i", stNumClip ) : "Paste", qfalse, (qboolean)( stNumClip > 0 ) ) )
		ST_PasteSteps();
	if ( ST_Button( ST_STEPS_X + ( bw + 4 ) * 5, by, bw, "Delete", qfalse, (qboolean)( s != NULL ) ) )
		ST_DeleteStep();
}

static void ST_DrawEdit( void ) {
	stStep_t *s = ST_SelectedStep();
	const stActionDef_t *def;
	const float fx = ST_EDIT_X + ST_EDIT_LABEL_W, fw = ST_EDIT_W - ST_EDIT_LABEL_W;
	float y = ST_TOP_Y + 14;
	const char *cmd, *error;
	qboolean local;

	if ( !s ) {
		ST_Text( ST_EDIT_X, ST_TOP_Y, "Step", stDim );
		return;
	}
	def = ST_Def( s );
	ST_Text( ST_EDIT_X, ST_TOP_Y, va( "Step %i" S_COLOR_GREY "  %s", st.stepList.sel + 1, ST_StepName( s ) ), stWhite );

	for ( int i = 0; i < def->numFields; y += ST_FieldShown( s, i ) ? ST_CTRL_H + 3 : 0, i++ ) {
		const stFieldDef_t *f = &def->fields[i];

		if ( !ST_FieldShown( s, i ) )
			continue;

		ST_Text( ST_EDIT_X, y + 2, ST_FieldLabel( s, i, f->label ), stDim );
		if ( f->kind == ST_K_CHOICE ) {
			// click for the next one, right side for the one before
			const int n = ST_NumChoices( f->choices ), at = ST_ChoiceIndex( f->choices, s->values[i] );
			if ( ST_Button( fx, y, fw - 20, s->values[i], qfalse, qtrue ) ) {
				Q_strncpyz( s->values[i], ST_Choice( f->choices, ( at + 1 ) % n ), ST_VALUE_LEN );
				ST_Changed();
			}
			if ( ST_Button( fx + fw - 18, y, 18, "<", qfalse, qtrue ) ) {
				Q_strncpyz( s->values[i], ST_Choice( f->choices, ( at + n - 1 ) % n ), ST_VALUE_LEN );
				ST_Changed();
			}
			continue;
		}
		const float fh = ST_TextBox( fx, y, fw, s->values[i], (qboolean)( st.focus == i ), f->kind == ST_K_NUMBER ? "number" : "type here" );
		if ( ST_TextBoxClicked( fx, y, fw, fh, s->values[i] ) )
			st.focus = i;
	}

	// a use trigger: the one looked at
	if ( ST_Watches( s ) && ST_TriggerUse( s ) ) {
		const float bw = ( ST_EDIT_W - 4 ) * 0.5f;

		if ( ST_Button( ST_EDIT_X + bw + 4, y, bw, "Pick in the map", qfalse, qtrue ) )
			ST_StartTriggerPick( 2 );
		if ( ST_Button( ST_EDIT_X, y, bw, "The one I look at", qfalse, qtrue ) ) {
			const char *name = CL_MapEditor_AimedUseTrigger();

			if ( name && name[0] ) {
				Q_strncpyz( s->values[1], name, ST_VALUE_LEN );
				ST_Changed();
			} else {
				ST_Message( name ? S_COLOR_YELLOW "That use trigger has no name: give it one in the map editor's Triggers tab"
					: S_COLOR_YELLOW "Look at one of the map editor's use triggers first, close enough to use it" );
			}
		}
		y += ST_CTRL_H + 4;
	}
	if ( ST_Watches( s ) && !ST_TriggerNear( s ) && !ST_TriggerChat( s ) && !ST_TriggerUse( s ) ) {
		const qboolean door = ST_TriggerDoor( s );
		const float bw = ( ST_EDIT_W - 8 ) / 3.0f;

		if ( ST_Button( ST_EDIT_X + bw + 4, y, bw, "In the map", qfalse, qtrue ) )
			ST_StartTriggerPick( door ? 1 : 0 );
		if ( ST_Button( ST_EDIT_X, y, bw, door ? "I aim at" : "I stand in", qfalse, qtrue ) ) {
			const int m = door ? ST_AimedMover() : ST_TriggerHere();

			if ( m ) {
				Com_sprintf( s->values[1], ST_VALUE_LEN, "*%i", m );
				ST_Changed();
			} else {
				ST_Message( door ? S_COLOR_YELLOW "Aim at a door first" : S_COLOR_YELLOW "Stand in one of the map's triggers first" );
			}
		}
		if ( ST_Button( ST_EDIT_X + ( bw + 4 ) * 2, y, bw, "From a list", qfalse, qtrue ) )
			ST_OpenChoose( door ? ST_CHOOSE_DOOR : ST_CHOOSE_TRIGGER );
		y += ST_CTRL_H + 6;
	}
	if ( ST_Watches( s ) && ST_TriggerEffect( s ) ) {
		if ( ST_Button( ST_EDIT_X, y, ST_EDIT_W, "Pick an effect in the map", qfalse, qtrue ) )
			ST_OpenChoose( ST_CHOOSE_EFFECT );
		y += ST_CTRL_H + 6;
	}

	if ( def->posField >= 0 && ST_FieldShown( s, def->posField ) ) {
		const float bw = ( ST_EDIT_W - 4 ) * 0.5f;
		if ( ST_Button( ST_EDIT_X, y, bw, "Where I stand", qfalse, qtrue ) ) {
			ST_FillHere( s );
			ST_Changed();
		}
		if ( ST_Button( ST_EDIT_X + bw + 4, y, bw, "Where I aim", qfalse, qtrue ) ) {
			if ( ST_FillAim( s ) )
				ST_Changed();
			else
				ST_Message( S_COLOR_YELLOW "Aim at a surface first" );
		}
		y += ST_CTRL_H + 6;
	}

	if ( s->action == ST_A_MODEL || s->action == ST_A_EFFECT || s->action == ST_A_SCREEN || s->action == ST_A_SHADER ) {
		const char *label = s->action == ST_A_MODEL ? "Place it with the model manager"
			: s->action == ST_A_SHADER ? "Pick it with the shader manager" : "Pick it in the effect manager";

		if ( ST_Button( ST_EDIT_X, y, ST_EDIT_W, label, qfalse, qtrue ) ) {
			ST_StartPick( s->action == ST_A_MODEL ? ST_PICK_MODEL : s->action == ST_A_SHADER ? ST_PICK_SHADER : ST_PICK_EFFECT );
			return;
		}
		y += ST_CTRL_H + 6;
	}

	if ( s->action == ST_A_OVERLAY || s->action == ST_A_SKY ) {
		if ( ST_Button( ST_EDIT_X, y, ST_EDIT_W, s->action == ST_A_OVERLAY ? "Pick it in the shader manager's Overlays" : "Set it in the shader manager's Sky tab", qfalse, qtrue ) ) {
			ST_StartPick( s->action == ST_A_OVERLAY ? ST_PICK_OVERLAY : ST_PICK_SKY );
			return;
		}
		y += ST_CTRL_H + 6;
	}

	if ( s->action == ST_A_LOCK ) {
		if ( ST_Button( ST_EDIT_X, y, ST_EDIT_W, "Pick it in the map editor's Locks tab", qfalse, qtrue ) ) {
			ST_StartPick( ST_PICK_LOCK );
			return;
		}
		y += ST_CTRL_H + 6;
	}

	if ( s->action == ST_A_SOUND || s->action == ST_A_MUSIC ) {
		if ( ST_Button( ST_EDIT_X, y, ST_EDIT_W, s->action == ST_A_SOUND ? "Pick it in the sound manager" : "Pick it in the music manager", qfalse, qtrue ) ) {
			ST_StartPick( s->action == ST_A_SOUND ? ST_PICK_SOUND : ST_PICK_MUSIC );
			return;
		}
		y += ST_CTRL_H + 6;
	}

	if ( s->action == ST_A_NPC_WALK ) {
		if ( ST_Button( ST_EDIT_X, y, ST_EDIT_W, "Pick the route", qfalse, qtrue ) )
			ST_OpenChoose( ST_CHOOSE_ROUTE );
		y += ST_CTRL_H + 6;
	}

	if ( s->action == ST_A_NPC_SPAWN || s->action == ST_A_NPC_TELE || s->action == ST_A_NPC_CMD || s->action == ST_A_NPC_WALK ) {
		if ( ST_Button( ST_EDIT_X, y, ST_EDIT_W, s->action == ST_A_NPC_SPAWN ? "Place it with the NPC manager" : "Pick it in the NPC manager", qfalse, qtrue ) ) {
			ST_StartPick( s->action == ST_A_NPC_SPAWN ? ST_PICK_NPC_SPAWN : ST_PICK_NPC_MANAGE );
			return;
		}
		y += ST_CTRL_H + 6;
	}

	if ( s->action == ST_A_CINEMATIC || s->action == ST_A_CAM_SHOT ) {
		if ( ST_Button( ST_EDIT_X, y, ST_EDIT_W, s->action == ST_A_CAM_SHOT ? "Open in the cinematic manager" : "Edit in the cinematic manager", qfalse, qtrue ) ) {
			// a new one gets a name of its own
			if ( !s->values[0][0] ) {
				Com_sprintf( s->values[0], ST_VALUE_LEN, "cinematic%i", st.stepList.sel + 1 );
				ST_Changed();
			}
			CL_RpCinematic_Edit( s->values[0], qtrue );
			return;
		}
		y += ST_CTRL_H + 6;
	}

	if ( ST_Button( ST_EDIT_X, y, ST_EDIT_W, s->enabled ? "Runs  (click to skip it)" : "Skipped  (click to run it)", (qboolean)!s->enabled, qtrue ) ) {
		s->enabled = (qboolean)!s->enabled;
		ST_Changed();
	}
	y += ST_CTRL_H + 8;

	// what it will do
	cmd = ST_StepCommand( s, &local, &error );
	if ( error ) {
		ST_TextWrapped( ST_EDIT_X, y, ST_EDIT_W, va( S_COLOR_YELLOW "%s", error ), stWhite, 3 );
	} else if ( cmd ) {
		int lines;
		ST_Text( ST_EDIT_X, y, local ? "Runs:" : "Sends:", stDim );
		lines = ST_TextWrappedEx( ST_EDIT_X, y + ST_ROW_H, ST_EDIT_W, cmd, stWhite, 5, qtrue );
		cmd = ST_StepExtra( s, &local );
		if ( cmd ) {
			y += ST_ROW_H * ( lines + 1 ) + 4;
			ST_Text( ST_EDIT_X, y, "Then:", stDim );
			ST_TextWrappedEx( ST_EDIT_X, y + ST_ROW_H, ST_EDIT_W, cmd, stWhite, 2, qtrue );
		}
	} else if ( s->action == ST_A_TRIGGER ) {
		const int lines = ST_TextWrapped( ST_EDIT_X, y, ST_EDIT_W, va( "%s, then goes on.", ST_TriggerSummary( s ) ), stDim, 3 );
		ST_TextWrapped( ST_EDIT_X, y + ( lines + 1 ) * ST_ROW_H, ST_EDIT_W, ST_TriggerDoor( s )
			? "Whoever opens it: your game can't see who used it."
			: ST_TriggerChat( s ) ? "Said in chat from when this step starts waiting, any of the words, | between them."
			: ST_TriggerUse( s ) ? "Used from when this step starts waiting: by you, or by anyone with this client with experimental on."
			: ST_Number( s, 8 ) > 0 ? "Someone in already counts from when the step starts. Players out of sight count too; NPCs only near you."
			: "Someone already in has to leave and come back. Players out of sight count too; NPCs only near you.", stDim, 3 );
	} else if ( s->action == ST_A_IF && ST_UnderWhen( st.steps, st.stepList.sel ) ) {
		const int lines = ST_TextWrapped( ST_EDIT_X, y, ST_EDIT_W, va( "%s: the steps after it play, else the ones after its Else.", ST_IfSummary( s ) ), stDim, 3 );
		ST_TextWrapped( ST_EDIT_X, y + ( lines + 1 ) * ST_ROW_H, ST_EDIT_W, ST_TriggerChat( s )
			? "Said in chat since the story began playing. End the If's steps with End if."
			: ST_TriggerUse( s ) ? "Used since the story began playing. End the If's steps with End if."
			: "Looked at once, when the story gets to it; players out of sight count too, NPCs only near you. End the If's steps with End if.", stDim, 3 );
	} else if ( ST_IsCondition( s->action ) && !ST_UnderWhen( st.steps, st.stepList.sel ) ) {
		ST_TextWrapped( ST_EDIT_X, y, ST_EDIT_W, S_COLOR_YELLOW "It goes under a When step: move it below one (Ctrl+Down), or add a When above it. Played as it is, the story stops here.", stWhite, 3 );
	} else if ( s->action == ST_A_ELSE || s->action == ST_A_ENDIF ) {
		ST_TextWrapped( ST_EDIT_X, y, ST_EDIT_W, s->action == ST_A_ELSE ? "The steps from here to the End if play when the If was false."
			: "The If's steps end here; the story goes on either way.", stDim, 3 );
	} else if ( s->action == ST_A_WAIT ) {
		ST_TextWrapped( ST_EDIT_X, y, ST_EDIT_W, va( "Waits %.1f seconds, then goes on.", ST_Number( s, 0 ) ), stDim, 2 );
	} else if ( s->action == ST_A_PAUSE ) {
		ST_TextWrapped( ST_EDIT_X, y, ST_EDIT_W, "Waits until you press Continue, or run /storyteller continue.", stDim, 3 );
	}
}

// the stories open: a tab each, the one shown with its close box, and one more
static void ST_DrawTabs( void ) {
	const float y = 29, w = 72;
	float x = ST_ACTIONS_X;

	for ( int i = 0; i < stNumTabs; i++ ) {
		const qboolean shown = (qboolean)( i == stActiveTab );
		const char *name = shown ? st.name : stTabs[i].name;
		const qboolean dirty = shown ? st.dirty : stTabs[i].dirty;
		const qboolean playing = (qboolean)( st.run.running && st.runTab == i );
		const char *label = va( "%s%i %s%s", playing ? S_COLOR_GREEN : "", i + 1, name[0] ? name : "new", dirty ? S_COLOR_YELLOW "*" : "" );

		if ( ST_Button( x, y, shown ? w - 14 : w, label, shown, qtrue ) && !shown ) {
			ST_ShowTab( i );
			return;
		}
		if ( shown ) {
			const qboolean asked = (qboolean)( st.confirmClose && cls.realtime - st.confirmClose < ST_CONFIRM_MS );

			if ( ST_Button( x + w - 12, y, 12, asked ? "?" : "x", asked, qtrue ) ) {
				if ( !st.dirty || asked ) {
					ST_CloseTab();
					return;
				}
				st.confirmClose = cls.realtime;
				ST_Message( S_COLOR_YELLOW "Not saved: click x again to close it anyway" );
			}
		}
		x += w + 4;
	}
	if ( stNumTabs < ST_MAX_TABS && ST_Button( x, y, 16, "+", qfalse, qtrue ) )
		ST_NewTab();
}

static void ST_DrawBar( void ) {
	const float y = ST_BUTTON_Y + 22;
	stStep_t *s = ST_SelectedStep();

	if ( ST_Button( ST_ACTIONS_X, y, 48, st.confirmNew && cls.realtime - st.confirmTime < ST_CONFIRM_MS ? "Sure?" : "New", qfalse, (qboolean)( st.numSteps > 0 ) ) ) {
		if ( !st.dirty || ( st.confirmNew && cls.realtime - st.confirmTime < ST_CONFIRM_MS ) ) {
			ST_New();
			st.confirmNew = qfalse;
			ST_Message( "New story" );
		} else {
			st.confirmNew = qtrue;
			st.confirmTime = cls.realtime;
			ST_Message( S_COLOR_YELLOW "Not saved: click again to start over anyway" );
		}
	}
	if ( ST_Button( ST_ACTIONS_X + 52, y, 48, "Save", qfalse, (qboolean)( st.numSteps > 0 ) ) )
		ST_OpenSave();
	if ( ST_Button( ST_ACTIONS_X + 104, y, 46, "Load", qfalse, qtrue ) )
		ST_OpenLoad();

	if ( !st.run.running ) {
		if ( ST_Button( ST_STEPS_X, y, 106, "Play", qfalse, (qboolean)( st.numSteps > 0 ) ) )
			ST_Run( 0 );
		if ( ST_Button( ST_STEPS_X + 112, y, 106, "Play from here", qfalse, (qboolean)( s != NULL ) ) )
			ST_Run( st.stepList.sel );
	} else {
		if ( ST_Button( ST_STEPS_X, y, 106, "Stop", qfalse, qtrue ) )
			ST_Stop( S_COLOR_YELLOW "Stopped" );
		if ( ST_Button( ST_STEPS_X + 112, y, 106, "Continue", (qboolean)st.run.waiting, (qboolean)st.run.waiting ) )
			ST_Continue();
	}
}

static void ST_DrawStoryRow( int row, float x, float y, float w ) {
	ST_TextClipped( x, y, w, st.stories[row], stWhite );
}

static void ST_OpenArmed( void ) {
	st.popup = ST_POPUP_ARMED;
	st.focus = ST_F_NONE;
	st.armedList.sel = stNumArmed ? 0 : -1;
	st.armedList.scroll = 0;
	st.armedList.lastClickRow = -1;
	if ( st.armTimes < 2 )
		st.armTimes = 2;
}

#define ST_MAX_ROUTES		256
static char stRoutes[ST_MAX_ROUTES][MAX_QPATH];
static int stNumRoutes;

static void ST_OpenChoose( stChoose_t kind ) {
	ST_ScanMap();
	st.chooseKind = kind;
	st.chooseList.sel = -1;
	st.chooseList.scroll = 0;
	st.chooseList.lastClickRow = -1;
	if ( kind == ST_CHOOSE_EFFECT ) {
		CL_EffectManager_RequestList();
		st.nextSend = Q_max( st.nextSend, cls.realtime + ST_CommandGap() );
	}
	// the routes saved, read once for the popup
	if ( kind == ST_CHOOSE_ROUTE ) {
		char **files;
		int count;

		files = FS_ListFiles( "npcmanager/routes", ".route", &count );
		stNumRoutes = 0;
		for ( int i = 0; i < count && stNumRoutes < ST_MAX_ROUTES; i++ )
			COM_StripExtension( files[i], stRoutes[stNumRoutes++], sizeof( stRoutes[0] ) );
		FS_FreeFileList( files );
	}
	st.popup = ST_POPUP_CHOOSE;
	st.focus = ST_F_NONE;
}

static int stChooseRows[ST_MAX_MAPENTS];

// what the choose popup lists now: indexes into the map's entities, or into RPMod's effect list
static int ST_ChooseRows( void ) {
	int count = 0;

	if ( st.chooseKind == ST_CHOOSE_EFFECT ) {
		for ( int i = 0; i < CL_EffectManager_NumActive() && count < ST_MAX_MAPENTS; i++ )
			stChooseRows[count++] = i;
		return count;
	}
	if ( st.chooseKind == ST_CHOOSE_ROUTE ) {
		for ( int i = 0; i < stNumRoutes && count < ST_MAX_MAPENTS; i++ )
			stChooseRows[count++] = i;
		return count;
	}
	for ( int i = 0; i < stMap.count; i++ ) {
		if ( stMap.ents[i].trigger == (qboolean)( st.chooseKind == ST_CHOOSE_TRIGGER ) )
			stChooseRows[count++] = i;
	}
	return count;
}

static void ST_DrawChooseRow( int row, float x, float y, float w ) {
	if ( st.chooseKind == ST_CHOOSE_ROUTE ) {
		ST_TextClipped( x, y, w, stRoutes[stChooseRows[row]], stWhite );
		return;
	}
	if ( st.chooseKind == ST_CHOOSE_EFFECT ) {
		const char *path;
		vec3_t org;
		int num;

		if ( CL_EffectManager_GetActive( stChooseRows[row], &num, &path, org ) )
			ST_TextClipped( x, y, w, va( "%i  %s" S_COLOR_GREY "  %i %i %i", num, path, ST_Round( org[0] ), ST_Round( org[1] ), ST_Round( org[2] ) ), stWhite );
		return;
	}
	const stMapEnt_t *e = &stMap.ents[stChooseRows[row]];
	vec3_t mid;

	VectorAdd( e->mins, e->maxs, mid );
	VectorScale( mid, 0.5f, mid );
	ST_TextClipped( x, y, w, va( "*%i  %s%s%s" S_COLOR_GREY "  at %i %i %i", e->model, e->classname, e->targetname[0] ? " " : "", e->targetname,
		ST_Round( mid[0] ), ST_Round( mid[1] ), ST_Round( mid[2] ) ), stWhite );
}

// the selected step takes the row picked
static void ST_ChooseApply( int row ) {
	stStep_t *s = ST_SelectedStep();

	// a route, for the walk
	if ( s && st.chooseKind == ST_CHOOSE_ROUTE ) {
		if ( s->action == ST_A_NPC_WALK ) {
			Q_strncpyz( s->values[1], stRoutes[stChooseRows[row]], ST_VALUE_LEN );
			ST_Changed();
		}
		return;
	}
	if ( !s || !ST_Watches( s ) )
		return;
	if ( st.chooseKind == ST_CHOOSE_EFFECT ) {
		const char *path;
		vec3_t org;
		int num;

		if ( !CL_EffectManager_GetActive( stChooseRows[row], &num, &path, org ) )
			return;
		Q_strncpyz( s->values[1], path, ST_VALUE_LEN );
		for ( int i = 0; i < 3; i++ )
			Com_sprintf( s->values[5 + i], ST_VALUE_LEN, "%i", ST_Round( org[i] ) );
	} else {
		Com_sprintf( s->values[1], ST_VALUE_LEN, "*%i", stMap.ents[stChooseRows[row]].model );
	}
	ST_Changed();
}

static void ST_DrawChoosePopup( void ) {
	static const char *titles[] = { "Pick a trigger", "Pick a door", "Pick an effect", "Pick a route" };
	const float x = ST_POPUP_X, y = 80, w = ST_POPUP_W, h = 320;
	const int count = ST_ChooseRows();
	int picked = -1;

	ST_Box( x, y, w, h, stPanelOpaque );
	ST_Text( x + 8, y + 8, va( "%s" S_COLOR_GREY "  %i", titles[st.chooseKind], count ), stWhite );
	if ( !count ) {
		ST_TextWrapped( x + 8, y + 30, w - 16, st.chooseKind == ST_CHOOSE_ROUTE
			? "No routes saved yet: make one in the NPC manager, Manage tab, Walk row, Route on map."
			: st.chooseKind == ST_CHOOSE_EFFECT
			? "Asking the server for the map's effects... If none come, put some there with the effect manager."
			: st.chooseKind == ST_CHOOSE_TRIGGER ? "This map has no triggers." : "This map has no doors.", stDim, 3 );
	} else {
		picked = ST_List( &st.chooseList, count, x + 8, y + 26, w - 16, h - 60, ST_DrawChooseRow, NULL );
	}
	if ( ST_Button( x + 8, y + h - 22, 80, "Pick", qfalse, (qboolean)( st.chooseList.sel >= 0 && st.chooseList.sel < count ) ) )
		picked = st.chooseList.sel;
	if ( ST_Button( x + w - 88, y + h - 22, 80, "Cancel", qfalse, qtrue ) )
		st.popup = ST_POPUP_NONE;
	if ( picked >= 0 && picked < count ) {
		ST_ChooseApply( picked );
		st.popup = ST_POPUP_NONE;
	}
}

static void ST_DrawArmedRow( int row, float x, float y, float w ) {
	const stArmed_t *a = &stArmed[row];

	ST_TextClipped( x, y, w, va( "%s" S_COLOR_GREY "  %s, %s", a->name, ST_ArmedState( a ), ST_AtTrigger( &a->run, a->steps, a->numSteps ) ? "waiting" : S_COLOR_GREEN "playing" ), stWhite );
}

static void ST_DrawArmedPopup( void ) {
	const float x = ST_POPUP_X, y = 80, w = ST_POPUP_W, h = 320;
	const char *why = ST_ArmProblem( st.steps, st.numSteps );
	const char *name = st.name[0] ? st.name : "unsaved story";
	const qboolean haveSel = (qboolean)( st.armedList.sel >= 0 && st.armedList.sel < stNumArmed );

	ST_Box( x, y, w, h, stPanelOpaque );
	ST_Text( x + 8, y + 8, va( "Armed stories" S_COLOR_GREY "  %i", stNumArmed ), stWhite );
	ST_TextWrapped( x + 8, y + 26, w - 16, va( "Arm %s: a copy of it plays by itself when its first step is set off.", name ), stDim, 2 );
	if ( why ) {
		ST_TextWrapped( x + 8, y + 54, w - 16, va( S_COLOR_YELLOW "Can't: %s", why ), stWhite, 2 );
	} else {
		const float bw = ( w - 20 ) * 0.5f;

		if ( ST_Button( x + 8, y + 54, bw, "Arm: once", qfalse, qtrue ) )
			ST_Arm( name, st.steps, st.numSteps, 1 );
		if ( ST_Button( x + 12 + bw, y + 54, bw, "Arm: every time", qfalse, qtrue ) )
			ST_Arm( name, st.steps, st.numSteps, 0 );
		// a number of times, set with - and +
		if ( ST_Button( x + 8, y + 76, 22, "-", qfalse, (qboolean)( st.armTimes > 2 ) ) )
			st.armTimes--;
		ST_Text( x + 8 + ( bw - ST_TextWidth( va( "%i", st.armTimes ) ) ) * 0.5f, y + 78, va( "%i", st.armTimes ), stWhite );
		if ( ST_Button( x + bw - 14, y + 76, 22, "+", qfalse, (qboolean)( st.armTimes < ST_MAX_ARM_TIMES ) ) )
			st.armTimes++;
		if ( ST_Button( x + 12 + bw, y + 76, bw, va( "Arm: %i times", st.armTimes ), qfalse, qtrue ) )
			ST_Arm( name, st.steps, st.numSteps, st.armTimes );
	}

	if ( !stNumArmed )
		ST_Text( x + 8, y + 106, S_COLOR_GREY "Nothing armed yet", stWhite );
	else
		ST_List( &st.armedList, stNumArmed, x + 8, y + 104, w - 16, h - 162, ST_DrawArmedRow, NULL );
	ST_TextWrapped( x + 8, y + h - 52, w - 16, S_COLOR_GREY "A copy: arm it again after changing it. Disarmed when the map changes.", stWhite, 2 );

	if ( ST_Button( x + 8, y + h - 22, 80, "Disarm", qfalse, haveSel ) ) {
		ST_Message( va( "Disarmed %s", stArmed[st.armedList.sel].name ) );
		ST_Disarm( st.armedList.sel );
		if ( st.armedList.sel >= stNumArmed )
			st.armedList.sel = stNumArmed - 1;
	}
	if ( ST_Button( x + 92, y + h - 22, 80, "Disarm all", qfalse, (qboolean)( stNumArmed > 0 ) ) ) {
		while ( stNumArmed )
			ST_Disarm( 0 );
		st.armedList.sel = -1;
		ST_Message( "Nothing is armed now" );
	}
	if ( ST_Button( x + w - 88, y + h - 22, 80, "Close", qfalse, qtrue ) )
		st.popup = ST_POPUP_NONE;
}

static void ST_DrawExperimentalPopup( void ) {
	const float x = ST_POPUP_X, y = 160, w = ST_POPUP_W, h = 120;

	ST_Box( x, y, w, h, stPanelOpaque );
	ST_Text( x + 8, y + 8, S_COLOR_YELLOW "Experimental features", stWhite );
	ST_TextWrapped( x + 8, y + 26, w - 16, RP_EXPERIMENTAL_WARNING, stWhite, 5 );
	if ( ST_Button( x + 8, y + h - 22, 80, "OK", qfalse, qtrue ) ) {
		CL_RpHooks_SetExperimental( qtrue );
		ST_Message( S_COLOR_YELLOW "Experimental features on, for this session" );
		st.popup = ST_POPUP_NONE;
	}
	if ( ST_Button( x + w - 88, y + h - 22, 80, "Cancel", qfalse, qtrue ) )
		st.popup = ST_POPUP_NONE;
}

static void ST_DrawPopup( void ) {
	const float x = ST_POPUP_X, w = ST_POPUP_W;

	if ( st.popup == ST_POPUP_EXPERIMENTAL ) {
		ST_DrawExperimentalPopup();
		return;
	}

	if ( st.popup == ST_POPUP_ARMED ) {
		ST_DrawArmedPopup();
		return;
	}
	if ( st.popup == ST_POPUP_CHOOSE ) {
		ST_DrawChoosePopup();
		return;
	}

	if ( st.popup == ST_POPUP_SAVE ) {
		const float y = 180, h = 90;
		ST_Box( x, y, w, h, stPanelOpaque );
		ST_Text( x + 8, y + 8, "Save the story as", stWhite );
		const float fh = ST_TextBox( x + 8, y + 26, w - 16, st.saveName, (qboolean)( st.focus == ST_F_SAVE_NAME ), "name" );
		if ( ST_TextBoxClicked( x + 8, y + 26, w - 16, fh, st.saveName ) )
			st.focus = ST_F_SAVE_NAME;
		ST_Text( x + 8, y + 46, va( S_COLOR_GREY "In %s/, letters, digits, _ and -", ST_SAVE_DIR ), stWhite );
		if ( ST_Button( x + 8, y + h - 22, 80, "Save", qfalse, (qboolean)( st.saveName[0] != '\0' ) ) )
			ST_DoSave();
		if ( ST_Button( x + w - 88, y + h - 22, 80, "Cancel", qfalse, qtrue ) ) {
			st.popup = ST_POPUP_NONE;
			st.focus = ST_F_NONE;
		}
		return;
	}

	const float y = 80, h = 320;
	int picked;
	ST_Box( x, y, w, h, stPanelOpaque );
	ST_Text( x + 8, y + 8, va( "Load a story" S_COLOR_GREY "  %i saved", st.numStories ), stWhite );
	if ( !st.numStories ) {
		ST_TextWrapped( x + 8, y + 30, w - 16, va( "No stories saved yet: they go in %s/ in your game folder.", ST_SAVE_DIR ), stDim, 3 );
		picked = -1;
	} else {
		picked = ST_List( &st.storyList, st.numStories, x + 8, y + 26, w - 16, h - 60, ST_DrawStoryRow, NULL );
	}
	if ( st.dirty )
		ST_Text( x + 8, y + h - 44, S_COLOR_YELLOW "The story open now isn't saved", stWhite );
	if ( ST_Button( x + 8, y + h - 22, 80, "Load", qfalse, (qboolean)( st.storyList.sel >= 0 && st.storyList.sel < st.numStories ) ) )
		picked = st.storyList.sel;
	if ( ST_Button( x + w - 88, y + h - 22, 80, "Cancel", qfalse, qtrue ) )
		st.popup = ST_POPUP_NONE;
	if ( picked >= 0 && picked < st.numStories && ST_LoadInTab( st.stories[picked] ) )
		st.popup = ST_POPUP_NONE;
}

static const char *ST_RunStatus( const stRun_t *r, const stStep_t *steps, int numSteps ) {
	const stStep_t *s;

	if ( !r->running )
		return NULL;
	if ( cls.realtime < r->shotUntil )
		return va( S_COLOR_GREEN "Step %i/%i: the shot plays, %.1f s left", r->pc, numSteps, ( r->shotUntil - cls.realtime ) / 1000.0f );
	if ( r->pc >= numSteps )
		return S_COLOR_GREEN "Finishing";
	s = &steps[r->pc];
	if ( r->waiting )
		return va( S_COLOR_YELLOW "Waiting for you at step %i%s%s", r->pc + 1, s->values[0][0] ? ": " : "", s->values[0] );
	if ( s->action == ST_A_WAIT && r->stepStart ) {
		const float left = ST_Number( s, 0 ) - ( cls.realtime - r->stepStart ) / 1000.0f;
		return va( S_COLOR_GREEN "Step %i/%i: waiting %.1f s", r->pc + 1, numSteps, left > 0 ? left : 0.0f );
	}
	return va( S_COLOR_GREEN "Step %i/%i: %s", r->pc + 1, numSteps, ST_StepSummary( s ) );
}

static const char *ST_Status( void ) {
	int num;
	const stStep_t *steps = ST_RunSteps( &num );

	return ST_RunStatus( &st.run, steps, num );
}

/*
===============================================================================

MARKERS

Where the stories armed wait, and the shown story's "when someone comes": drawn in the
world on this screen only, with the story's name over each.

===============================================================================
*/

#define ST_MAX_MARKERS		48
#define ST_MAX_MARKER_LINES	800
#define ST_MARKER_HEIGHT	72.0f

static const byte stArmedColor[4]	= { 255, 170, 40, 255 };
static const byte stFiredColor[4]	= { 60, 230, 90, 255 };
static const byte stShownColor[4]	= { 70, 200, 255, 255 };

static struct {
	qboolean	haveView;
	vec3_t		viewOrg, viewAxis[3];
	float		fovX, fovY;
	int			numLines;
	int			count;
	vec3_t		top[ST_MAX_MARKERS];
	char		text[ST_MAX_MARKERS][MAX_QPATH * 2];
	byte		color[ST_MAX_MARKERS][4];
} stMark;

static qboolean ST_MarkersOn( void ) {
	static cvar_t *markers;

	if ( !markers )
		markers = Cvar_Get( "cl_storyMarkers", "1", CVAR_ARCHIVE_ND, "Draw where the stories armed wait, on the storyteller's screen" );
	return (qboolean)( markers->integer != 0 );
}

static void ST_Line( const vec3_t a, const vec3_t b, const byte *color ) {
	refEntity_t ent;
	vec3_t mid;

	if ( stMark.numLines >= ST_MAX_MARKER_LINES )
		return;
	stMark.numLines++;
	VectorAdd( a, b, mid );
	VectorScale( mid, 0.5f, mid );
	memset( &ent, 0, sizeof( ent ) );
	ent.reType = RT_LINE;
	VectorCopy( a, ent.origin );
	VectorCopy( b, ent.oldorigin );
	ent.radius = 0.5f + Distance( mid, stMark.viewOrg ) * 0.002f;
	ent.customShader = cls.whiteShader;
	memcpy( ent.shaderRGBA, color, sizeof( ent.shaderRGBA ) );
	ent.renderfx = RF_NOSHADOW;
	re->AddRefEntityToScene( &ent );
}

// the trigger of a step drawn, its name to go at *top; qfalse if it can't be told where
static qboolean ST_DrawTrigger( const stStep_t *s, const byte *color, vec3_t top ) {
	vec3_t mins, maxs;

	// what's said, or a use, is nowhere
	if ( ST_TriggerChat( s ) || ST_TriggerUse( s ) )
		return qfalse;

	if ( ST_TriggerNear( s ) ) {
		// a ring on the floor round the spot, and one over it, posts between
		const float radius = ST_Number( s, 4 ) > 0 ? ST_Number( s, 4 ) : 96.0f;
		vec3_t pos, prev[2];

		if ( !ST_HavePos( s ) || !s->values[5][0] )
			return qfalse;
		ST_GetPos( s, pos );
		pos[2] -= ST_GROUND_HEIGHT;
		for ( int i = 0; i <= 32; i++ ) {
			const float a = i * ( M_PI * 2.0f / 32.0f );
			vec3_t p[2];

			for ( int h = 0; h < 2; h++ ) {
				VectorSet( p[h], pos[0] + cosf( a ) * radius, pos[1] + sinf( a ) * radius, pos[2] + 2.0f + h * ST_MARKER_HEIGHT );
				if ( i )
					ST_Line( prev[h], p[h], color );
				VectorCopy( p[h], prev[h] );
			}
			if ( i % 8 == 0 )
				ST_Line( p[0], p[1], color );
		}
		VectorSet( top, pos[0], pos[1], pos[2] + ST_MARKER_HEIGHT + 8.0f );
		return qtrue;
	}

	// a box round the map's trigger, door or button
	{
		const int model = ST_ModelNum( s->values[1] );
		qboolean found = qfalse;

		for ( int i = 0; i < stMap.count && !found; i++ ) {
			if ( stMap.ents[i].model == model ) {
				VectorCopy( stMap.ents[i].mins, mins );
				VectorCopy( stMap.ents[i].maxs, maxs );
				found = qtrue;
			}
		}
		if ( !found )
			return qfalse;
	}
	{
		// its twelve edges, between its eight corners
		static const int edges[12][2] = { { 0, 1 }, { 2, 3 }, { 4, 5 }, { 6, 7 }, { 0, 2 }, { 1, 3 }, { 4, 6 }, { 5, 7 },
			{ 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 } };
		vec3_t corners[8];

		for ( int i = 0; i < 8; i++ )
			VectorSet( corners[i], ( i & 1 ) ? maxs[0] : mins[0], ( i & 2 ) ? maxs[1] : mins[1], ( i & 4 ) ? maxs[2] : mins[2] );
		for ( int i = 0; i < 12; i++ )
			ST_Line( corners[edges[i][0]], corners[edges[i][1]], color );
	}
	VectorSet( top, ( mins[0] + maxs[0] ) * 0.5f, ( mins[1] + maxs[1] ) * 0.5f, maxs[2] + 8.0f );
	return qtrue;
}

static void ST_AddMarker( const stStep_t *s, const byte *color, const char *text ) {
	if ( stMark.count >= ST_MAX_MARKERS || !ST_DrawTrigger( s, color, stMark.top[stMark.count] ) )
		return;
	Q_strncpyz( stMark.text[stMark.count], text, sizeof( stMark.text[0] ) );
	memcpy( stMark.color[stMark.count], color, 4 );
	stMark.count++;
}

// the cgame's main view, about to be drawn: the markers go in it
void CL_Storyteller_AddToScene( const refdef_t *fd ) {
	if ( fd->rdflags & ( RDF_NOWORLDMODEL | RDF_SKYBOXPORTAL ) )
		return;
	stMark.haveView = qfalse;
	stMark.count = 0;
	stMark.numLines = 0;
	if ( !ST_MarkersOn() || cls.state != CA_ACTIVE || CL_RpCinematic_Active() )
		return;
	stMark.haveView = qtrue;
	VectorCopy( fd->vieworg, stMark.viewOrg );
	for ( int i = 0; i < 3; i++ )
		VectorCopy( fd->viewaxis[i], stMark.viewAxis[i] );
	stMark.fovX = fd->fov_x;
	stMark.fovY = fd->fov_y;
	ST_ScanMap();

	// the stories armed: where they wait, or have been set off
	for ( int i = 0; i < stNumArmed; i++ ) {
		const stArmed_t *a = &stArmed[i];
		const qboolean fired = (qboolean)( a->run.running && !ST_AtTrigger( &a->run, a->steps, a->numSteps ) );

		if ( Q_stricmp( a->map, cl.mapname ) )
			continue;
		for ( int j = 0; j < a->numSteps; j++ ) {
			if ( !a->steps[j].enabled )
				continue;
			if ( a->steps[j].action == ST_A_TRIGGER ) {
				ST_AddMarker( &a->steps[j], fired ? stFiredColor : stArmedColor, va( "%s%s" S_COLOR_GREY "  %s%s", fired ? S_COLOR_GREEN : S_COLOR_YELLOW,
					a->name, fired ? "playing" : "waiting", a->limit ? va( ", %i of %i played", a->times, a->limit ) : "" ) );
			}
			break;
		}
	}
	// the story shown, while it's being made or played
	if ( st.open || ST_RunHere() ) {
		for ( int i = 0; i < st.numSteps; i++ ) {
			if ( st.steps[i].enabled && ST_Watches( &st.steps[i] ) )
				ST_AddMarker( &st.steps[i], stShownColor, va( S_COLOR_CYAN "%s" S_COLOR_GREY "  step %i", st.name[0] ? st.name : "this story", i + 1 ) );
		}
	}
}

static qboolean ST_Project( const vec3_t point, float *x, float *y ) {
	vec3_t d;
	float forward;

	VectorSubtract( point, stMark.viewOrg, d );
	forward = DotProduct( d, stMark.viewAxis[0] );
	if ( forward < 4.0f )
		return qfalse;
	*x = SCREEN_WIDTH * 0.5f * ( 1.0f - DotProduct( d, stMark.viewAxis[1] ) / forward / tanf( DEG2RAD( stMark.fovX * 0.5f ) ) );
	*y = SCREEN_HEIGHT * 0.5f * ( 1.0f - DotProduct( d, stMark.viewAxis[2] ) / forward / tanf( DEG2RAD( stMark.fovY * 0.5f ) ) );
	return qtrue;
}

// the stories' names over their markers
static void ST_DrawMarkerLabels( void ) {
	if ( !stMark.haveView || ( Key_GetCatcher() & ( KEYCATCH_UI | KEYCATCH_CONSOLE ) ) )
		return;
	for ( int i = 0; i < stMark.count; i++ ) {
		float x, y, w;

		if ( !ST_Project( stMark.top[i], &x, &y ) || x < 0 || x > SCREEN_WIDTH || y < 0 || y > SCREEN_HEIGHT )
			continue;
		w = ST_TextWidth( stMark.text[i] );
		ST_Fill( x - w * 0.5f - 3, y - 7, w + 6, 14, stPanel );
		ST_Text( x - w * 0.5f, y - 6, stMark.text[i], stWhite );
	}
}

// while playing with the panel closed: where the story is, and how to go on
// a story's line at the top; how tall it was
static float ST_DrawRunLine( float y, const char *label, const char *status, qboolean waiting ) {
	ST_Fill( 160, y, 320, waiting ? 30 : 17, stPanel );
	ST_TextClipped( 166, y + 2, 308, va( "%s: %s", label, status ), stWhite );
	if ( waiting ) {
		const int key = Key_GetKey( ST_TOGGLE_CMD " continue" );
		ST_TextClipped( 166, y + 15, 308, key >= 0 ? va( S_COLOR_GREY "Press %s to go on", Key_KeynumToString( key ) )
			: S_COLOR_GREY "/storyteller continue goes on (bind a key to it)", stWhite );
	}
	return waiting ? 32.0f : 19.0f;
}

// while playing with the panel closed: where the stories are, and how to go on. Armed ones
// show once set off
static void ST_DrawPlaying( void ) {
	const char *status = ST_Status();
	float y = 4;

	if ( Key_GetCatcher() & ( KEYCATCH_UI | KEYCATCH_CONSOLE ) )
		return;
	if ( status )
		y += ST_DrawRunLine( y, "Storyteller", status, st.run.waiting );
	for ( int i = 0; i < stNumArmed; i++ ) {
		const stArmed_t *a = &stArmed[i];

		if ( !a->run.running || ST_AtTrigger( &a->run, a->steps, a->numSteps ) )
			continue;
		status = ST_RunStatus( &a->run, a->steps, a->numSteps );
		if ( status )
			y += ST_DrawRunLine( y, a->name, status, a->run.waiting );
	}
}

void CL_Storyteller_Draw( void ) {
	char buf[MAX_STRING_CHARS];
	const char *status;

	if ( !st.font ) {
		st.font = re->RegisterFont( "arialnb" );
		if ( !st.font )
			st.font = cls.menuFont;
	}
	ST_DrawMarkerLabels();
	if ( !st.open ) {
		ST_DrawPlaying();
		return;
	}

	// the popup sits on top and gets the clicks first, but is drawn last
	const qboolean popupOpen = (qboolean)( st.popup != ST_POPUP_NONE );
	const qboolean click = st.click;
	const int wheel = st.wheel;
	if ( popupOpen )
		st.click = qfalse, st.wheel = 0;

	ST_Fill( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, stPanel );
	ST_Text( ST_ACTIONS_X, 14, "Storyteller", stWhite );
	ST_TextClipped( ST_ACTIONS_X + 80, 14, 200, st.name[0] ? va( "%s%s", st.name, st.dirty ? S_COLOR_YELLOW " *" : "" )
		: va( S_COLOR_GREY "unsaved story%s", st.dirty ? S_COLOR_YELLOW " *" : "" ), stWhite );
	status = ST_Status();
	if ( status )
		ST_TextClipped( ST_EDIT_X - 100, 14, ST_EDIT_W + 100 - 76 - 124 - 64, status, stWhite );
	if ( ST_Button( ST_EDIT_X + ST_EDIT_W - 74 - 124 - 60, 12, 60, "Markers", ST_MarkersOn(), qtrue ) ) {
		Cvar_SetValue( "cl_storyMarkers", ST_MarkersOn() ? 0 : 1 );
		ST_Message( ST_MarkersOn() ? "Markers on: where the stories wait shows on your screen" : "Markers off" );
	}
	if ( ST_Button( ST_EDIT_X + ST_EDIT_W - 74 - 120, 12, 120, RP_EXPERIMENTAL_LABEL, CL_RpHooks_Experimental(), qtrue ) ) {
		if ( CL_RpHooks_Experimental() ) {
			CL_RpHooks_SetExperimental( qfalse );
			ST_Message( "Experimental features off" );
		} else {
			st.popup = ST_POPUP_EXPERIMENTAL;
			st.focus = ST_F_NONE;
		}
	}
	if ( ST_Button( ST_EDIT_X + ST_EDIT_W - 70, 12, 70, va( "Armed (%i)", stNumArmed ), (qboolean)( stNumArmed > 0 ), qtrue ) )
		ST_OpenArmed();
	if ( st.message[0] && cls.realtime - st.messageTime < ST_MESSAGE_MS )
		ST_TextClipped( ST_ACTIONS_X, 454, ST_EDIT_X - ST_ACTIONS_X - 8, st.message, stWhite );

	ST_DrawTabs();
	ST_DrawActions();
	ST_DrawSteps();
	ST_DrawEdit();
	ST_DrawBar();

	ST_Text( ST_EDIT_X, 418, S_COLOR_GREY "Up/Down select   Ctrl+Up/Down move", stWhite );
	ST_Text( ST_EDIT_X, 430, S_COLOR_GREY "Ctrl/Shift+click several   Ctrl+A all", stWhite );
	ST_Text( ST_EDIT_X, 442, S_COLOR_GREY "Ctrl+C/X/V copy, cut, paste   Del delete", stWhite );
	Com_sprintf( buf, sizeof( buf ), S_COLOR_GREY "Tab next box   Esc back / close%s", Key_GetKey( ST_TOGGLE_CMD ) >= 0 ? "" : "   Bind a key to storyteller" );
	ST_Text( ST_EDIT_X, 454, buf, stWhite );

	if ( popupOpen && st.popup != ST_POPUP_NONE ) {
		st.click = click;
		st.wheel = wheel;
		ST_DrawPopup();
	}

	// a click on nothing leaves the text box
	if ( st.click && st.focus != ST_F_SAVE_NAME )
		st.focus = ST_F_NONE;
	st.click = qfalse;
	st.wheel = 0;

	WG_DrawOverlay();
	re->DrawStretchPic( st.cursorX, st.cursorY, 32, 32, 0, 0, 1, 1, cls.cursorShader );
}
