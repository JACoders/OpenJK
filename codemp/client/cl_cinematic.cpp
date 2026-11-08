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

// cl_cinematic.cpp -- the cinematic manager
//
// A cinematic is a scene on one timeline: the camera's shots one after the other, and what
// the players and NPCs in it do meanwhile, each on their own rows: moving along spots,
// playing animations and saying lines. /cinematicmanager [name] (or /cincam edit) opens the manager: a free camera
// over the map, the panel on the right (the Camera tab for the shots, the Entities tab for
// who's in it and what they do) and the timeline under the view, where a moment picked shows
// the cinematic as it is then and the bars drag to retime things.
//
// Cinematics are saved in cinematics/<name>.cin, a line each:
//   look <bars 0|1> <hud 0|1> <only those in it seen 0|1> <bars 0 thin|1|2 wide> <the game's sounds %>
//   mark <seconds in> <name|->                      a moment marked on the timeline
//   speed <seconds in> <seconds> <percent>          slowed or sped up meanwhile
//   maplook <seconds in> <seconds, 0 to the end> <fade seconds> <light %> <r g b> <sky r g b> <fog depth, 0 none> <fog r g b> <map editor layout|->
//   force <ent> <seconds in> <seconds, 0 to the end> <cmForces> <at ent, -1 no one> <acted 0|1>
//   shot <hold seconds> <hook>      a shot as the cin/path or cin/orbit hook has it (see
//                                   cl_rpcinematic.cpp); a path of one spot is held that long
//   ent <who>                       a player or an NPC: me, a number, a player's name, a targetname
//   move <ent> <seconds in> walk|run <x y z>|<x y z>...
//   anim <ent> <seconds in> <seconds, 0 to the end> <ANIMATION> <from> <to> play|reverse|hold loop|once <speed %>
//   say <ent> <seconds in> <NPC's name shown>|<text>
//   emote <ent> <seconds in> <seconds, 0 to the end> <emote> <loop 0|1> <for real 0|1>
//   place <ent> <seconds in> <smooth 0|1> <ms x y z pitch yaw roll>|...
//   rec <ent> <seconds in> <recording>, the recording's moments in
//   tele <ent> <seconds in> <x y z yaw>             a teleport: a position of one key, at once
//   npccmd <ent> <seconds in> <command> [value]     an npc command, spaces in it as _
//   spawn <ent> <seconds in> <x y z yaw> <team> <NPC type>   its targetname the ent's who
//   remove <ent> <seconds in>                       taken out with npc kill
//   appear <ent> <seconds in> <seconds, 0 to the end> <hidden 0|1> <size %> <cmShells> <cmSaberLooks> <blade %>
//          <hand 0 right|1 left> <pitch yaw roll> <model held|-> <cmFaces>
//   music <seconds in> <seconds, 0 to the end> <music/...>   a track in place of the map's
//   prop <seconds in> <seconds, 0 to the end> <smooth 0|1> <scale> <ms,x,y,z,pitch,yaw,roll;...> <model>
//   light <seconds in> <seconds, 0 to the end> <x y z> <radius> <r g b 0-1> <steady|flicker|pulse|strobe|flash 0-4> <fade seconds> <who|->
//   screen <seconds in> <seconds> <0 flash|1 tint|2 dark edges> <r g b 0-1> <strength %> <fade seconds>
//   txt <seconds in> <seconds> <where 0 bottom bar|1 top bar|2 center|3 notify> <size 0-2> <typed 0|1> <who>|<text>
//   extra <ent> <x y z yaw> <saber 0 none|1|2 staff|3 two> <colour 0-5> <model[/skin]>   the ent "+name"
//   rs <recording> <ms> <x y z> <pitch> <yaw> <legs> <torso> <flips> <holstered> <force powers> <saber move>
// The shots of the camera editor from before (camshots/*.cam) open as a cinematic of one shot.
//
// A cinematic plays on each screen by itself (see Playing): the camera, the animations and
// where everyone is are worked out there every frame from the time it's at. Moves are seen
// on screens with this client only; a player moved goes back where they are when it ends.
// Played for everyone it goes out whole first, hidden in rpnotify hooks (see Sending), and
// whoever sent it teleports its NPCs where their moves end, for the server and the players
// without this client; its NPC commands, spawns and removals it sends at their moment.
//
// This lives in the engine rather than cgame so it works whatever cgame the server's mod
// ships (RPMod loads its own).

#include "client.h"
#include "cl_rpcinematic.h"
#include "FXExport.h"
#include "FxScheduler.h"
#include "qcommon/cm_public.h"
#include "cl_gizmo.h"
#include "cl_widgets.h"
#include <vector>
#include <string>
#include <algorithm>

#define CM_DIR				"cinematics"	// in the game folder
#define CM_EXT				".cin"
#define CM_HEADER			"// cinematic, made in the cinematic manager (/cinematicmanager)\n"
#define CM_MAX_SHOTS		16
#define CM_MAX_ENTS			12
#define CM_MAX_CLIPS		64
#define CM_MAX_PTS			16
#define CM_MAX_GROUP		8			// NPCs sharing a targetname, moving together
#define CM_HOLD_MS			3000		// a new shot holds this long
#define CM_KEY_MS			2000		// a new spot is this far from the one before
#define CM_ANIM_MS			2000
#define CM_SAY_MS			2500		// how long a line takes on the timeline
#define CM_SAY_LATE_MS		1500		// a line this late, starting from the middle, isn't said
#define CM_SAY_LEN			150
#define CM_TEXT_MAX			65536		// a cinematic as text
#define CM_PART_LEN			700			// of it encoded, in each hook
#define CM_MAX_PARTS		64
#define CM_WORD				30			// a space in every so often, RPMod chokes on long words
#define CM_MAX_OUT			96			// commands waiting for the server to take them
#define CM_MAX_PLACE		12			// a position's keys
#define CM_MAX_RECS			6			// recordings in a cinematic
#define CM_MAX_SAMPLES		320			// in a recording
#define CM_REC_MAX_MS		30000
#define CM_REC_EVERY		100			// a recording keeps where you are this often, and every change besides
#define CM_REC_COUNTDOWN	3000
#define CM_EMOTE_MS			3000
#define CM_SEND_GAP_MS		1100		// one a second, the server's flood protection
#define CM_MAX_FX			24			// effects in a cinematic
#define CM_FX_MS			4000		// a new looping one plays this long
#define CM_FX_ONCE_MS		600			// how long a one-off shows on the timeline
#define CM_MAX_SND			24			// sounds in a cinematic
#define CM_SND_MS			4000		// a new looping one plays this long
#define CM_MAX_TXT			32			// texts in a cinematic
#define CM_TXT_LEN			256
#define CM_TXT_WHO_LEN		48
#define CM_TXT_MS			3000		// a new one shows this long
#define CM_TXT_FADE_MS		250
#define CM_TXT_TYPE_MS		35			// typed out, a letter this often
#define CM_TXT_MAX_LINES	8
#define CM_MAX_PROPS		16			// props in a cinematic
#define CM_PROP_KEYS		8
#define CM_PROP_MS			4000		// a new one shows this long
#define CM_MAX_LIGHTS		16
#define CM_LIGHT_MS			3000
#define CM_MAX_SCREENS		16
#define CM_MAX_MARKS		16
#define CM_MAX_MAPLOOKS		16
#define CM_MAX_SPEEDS		16
#define CM_SPEED_MS			2000
#define CM_MAPLOOK_MS		4000
#define CM_FOG_NONE			20000.0f	// fog this thin is none
#define CM_CAMREC_MAX		600			// a camera flown and kept: a tenth of a second each, a minute
#define CM_CAMREC_EVERY		100
#define CM_SCREEN_MS		1500

typedef enum {
	CM_CLIP_MOVE,
	CM_CLIP_ANIM,
	CM_CLIP_SAY,
	CM_CLIP_EMOTE,
	CM_CLIP_PLACE,
	CM_CLIP_RECORD,
	CM_CLIP_NPCCMD,		// an npc command at its moment: freeze, follow, dialog, hologram...
	CM_CLIP_SPAWN,		// the NPC spawned at its moment, its targetname the ent's who
	CM_CLIP_REMOVE,		// and taken out
	CM_CLIP_LOOK,		// how they look meanwhile: hidden, their size, drawn as a hologram, their saber
	CM_CLIP_FORCE,		// a force power at someone: its effect from their hands, and both of them acting it
	CM_NUM_CLIPS
} cmClipKind_t;

// a force power: its effect, played again this often, from the hands to the one it's at or round
// the one using it, and how each of them acts it
typedef struct {
	const char	*name;
	const char	*effect;
	int			every;				// ms
	qboolean	atTarget;			// played on the one it's at, else from the hands toward them
	qboolean	onSelf;				// played on the one using it
	const char	*anim;				// the one using it
	const char	*victim;			// the one it's at
} cmForce_t;

static const cmForce_t cmForces[] = {
	{ "lightning", "force/lightning", 60, qfalse, qfalse, "BOTH_FORCELIGHTNING_HOLD", "BOTH_SONICPAIN_HOLD" },
	{ "lightning, both hands", "force/lightningwide", 60, qfalse, qfalse, "BOTH_FORCE_2HANDEDLIGHTNING_HOLD", "BOTH_SONICPAIN_HOLD" },
	{ "drain", "force/drain_hand", 80, qfalse, qfalse, "BOTH_FORCE_DRAIN_HOLD", "BOTH_SONICPAIN_HOLD" },
	{ "grip", "force/force_touch", 400, qtrue, qfalse, "BOTH_FORCEGRIP_HOLD", "BOTH_CHOKE1" },
	{ "push", "force/pushblur", 250, qfalse, qfalse, "BOTH_FORCEPUSH", NULL },
	{ "pull", "force/pushblur", 250, qfalse, qfalse, "BOTH_FORCEPULL", NULL },
	{ "heal", "force/heal", 1000, qfalse, qtrue, "BOTH_FORCEHEAL_START", NULL },
	{ "rage", "force/rage2", 500, qfalse, qtrue, "BOTH_FORCE_RAGE", NULL }
};
#define CM_FORCE_MS			2000

// what a look can draw someone as: in place of their body, or over it
typedef struct {
	const char	*name;
	const char	*shader;
	qboolean	over;
} cmShell_t;

static const cmShell_t cmShells[] = {
	{ "as they are", NULL, qfalse },
	{ "a hologram", "powerups/ysaliblueshell", qfalse },
	{ "cloaked", "gfx/effects/cloakedShader", qfalse },
	{ "force glow", "powerups/forceshell", qtrue },
	{ "light side", "powerups/enlightenmentshell", qtrue },
	{ "dark side", "powerups/endarkenmentshell", qtrue },
	{ "electric", "gfx/effects/demp2shell", qtrue },
	{ "red glow", "powerups/ysaliredshell", qtrue },
	{ "yellow glow", "powerups/ysalimarishell", qtrue },
	{ "force protect", "gfx/misc/forceprotect", qtrue }
};

// a look's face: as it is, talking, or one of the game's faces held
static const struct {
	const char	*name;
	const char	*anim;
} cmFaces[] = {
	{ "as it is", NULL },
	{ "talking", "FACE_TALK1" },
	{ "smiling", "FACE_SMILE" },
	{ "frowning", "FACE_FROWN" },
	{ "alert", "FACE_ALERT" },
	{ "eyes closed", "FACE_DEAD" }
};
#define CM_FACE_TALK		1

// a look's saber: as it is, its blades off, or blades of a colour
static const char *cmSaberLooks[] = { "as it is", "off", "red", "orange", "yellow", "green", "blue", "purple" };
#define CM_SABER_OFF		1
#define CM_SABER_COLORS		2		// the first of the colours
#define CM_LOOK_UP			24.0f	// a player's origin is this far over their feet

// what an NPC command can be
static const char *cmNpcCmds[] = { "freeze", "unfreeze", "follow me", "stop following", "dialog", "no dialog",
	"hologram", "projector", "no hologram", "kill" };
static const char *cmTeams[] = { "default", "player", "enemy" };
#define CM_ACT_MS			600			// how long an NPC command, a spawn or a removal shows on the timeline

// a position's key: where they are and how they're turned, ms into it
typedef struct {
	int			ms;
	vec3_t		org;
	vec3_t		ang;
} cmPlace_t;

// a moment of a recording: where you were from where it began, how you were turned and what
// you were doing
typedef struct {
	int			ms;
	vec3_t		org;
	float		pitch, yaw;
	int			legs, torso;
	int			flags;				// 1 legs started again, 2 torso
	int			holstered;
	int			force;				// the force powers on
	int			saberMove;
} cmSample_t;

// something heard of as an event while it was recorded: a swing, a jump...
#define CM_MAX_REC_EVENTS	200
typedef struct {
	int			ms, event, parm;
} cmRecEvent_t;

typedef struct {
	int			count;
	cmSample_t	s[CM_MAX_SAMPLES];
	int			numEvents;
	cmRecEvent_t	ev[CM_MAX_REC_EVENTS];
} cmRec_t;

// something someone does in it
typedef struct {
	int			kind;				// cmClipKind_t
	int			ent;
	int			start;				// ms into the cinematic

	// a move: from where they are through the spots
	qboolean	run;
	int			numPts;
	vec3_t		pts[CM_MAX_PTS];

	// an animation or an emote's: its who and delay aren't used, its ms is how long, 0 to the end
	rcAnim_t	a;

	// a line said: as a player's chat, or an NPC's talk with the name shown ("" its own)
	char		text[CM_SAY_LEN];
	char		name[MAX_QPATH];

	// an emote, its animation shown, and done for real by an NPC or you when it's played for all
	char		emote[48];
	qboolean	realEmote;

	// a position: keys anywhere, turned any way, from one to the next
	int			numKeys;
	cmPlace_t	keys[CM_MAX_PLACE];
	qboolean	smooth;

	// a recording: which of the cinematic's
	int			rec;

	// a position of one key that's a teleport: there at once, as a spot and a yaw
	qboolean	tele;

	// an NPC command: which (cmNpcCmds), its dialog in text
	char		cmd[24];

	// a spawn: the NPC type and team, where in keys[0]
	char		npcType[MAX_QPATH];
	char		team[16];

	// a look, for its a.ms (0 to the end): hidden, their size and blades' length in percent,
	// cmShells, cmSaberLooks
	qboolean	hidden;
	int			size;
	int			shell;
	int			saber;
	int			blade;
	// and a model held: in the right hand or the left, turned in it
	char		hold[MAX_QPATH];
	int			holdHand;			// 0 right, 1 left
	vec3_t		holdAng;
	int			face;				// cmFaces

	// a force power, for its a.ms (0 to the end): which (cmForces), at whom (an ent, -1 no one), and
	// whether both act it
	int			power;
	int			target;
	qboolean	forceAnims;
} cmClip_t;

// someone in it: a player or an NPC by who, or an extra, "+" and its name, that only screens
// with this client draw, with its model, saber and spot
typedef struct {
	char		who[MAX_QPATH];
	char		model[MAX_QPATH];	// an extra's: "model" or "model/skin"
	int			saber;				// clExtraSaber_t
	int			color;				// its blades', red to purple
	vec3_t		home;				// where it stands until moved: an entity's origin, over the floor
	float		yaw;
} cmEnt_t;

static qboolean CM_IsExtra( const char *who ) {
	return (qboolean)( who[0] == '+' );
}

// an effect played at its moment: once, or again and again while it lasts
typedef struct {
	int			start;				// ms into the cinematic
	int			ms;					// how long it goes on, 0 once
	char		path[MAX_QPATH];	// as the effect manager has it
	vec3_t		org;
	qboolean	haveAngles;			// else straight up
	vec3_t		ang;
	float		scale;				// how big, 1 as made
} cmFx_t;

// a sound played at its moment: once, or again and again while it lasts; heard everywhere,
// or from a spot
typedef struct {
	int			start;				// ms into the cinematic
	int			ms;					// how long it goes on, 0 once
	char		path[MAX_QPATH];	// under sound/, as the sound manager has it
	qboolean	spot;				// heard from org, else everywhere alike
	vec3_t		org;
	qboolean	music;				// a track: path under music/, in place of the map's, ms 0 to the end
} cmSnd_t;

// a text shown from its moment for as long as it lasts: on the bottom bar (a line said, who
// says it before it), on the top bar, in the middle of the screen, or in the top left as a
// notification; typed out a letter at a time or not. Drawn on the screens with this client
typedef enum {
	CM_TXT_BOTTOM,
	CM_TXT_TOP,
	CM_TXT_CENTER,
	CM_TXT_NOTIFY,
	CM_NUM_TXT_KINDS
} cmTxtKind_t;

static const char *cmTxtKinds[CM_NUM_TXT_KINDS] = { "Bottom bar", "Top bar", "Center", "Notify" };
static const char *cmTxtSizes[] = { "small", "normal", "large" };

typedef struct {
	int			start;				// ms into the cinematic
	int			ms;					// how long it shows
	int			kind;				// cmTxtKind_t
	int			size;				// 0 small, 1 normal, 2 large
	qboolean	typed;				// a letter at a time
	char		who[CM_TXT_WHO_LEN];	// who says it, shown before it; "" none
	char		text[CM_TXT_LEN];
} cmTxt_t;

// a model in the scene from its moment for as long as it lasts, through its keys (ms into it,
// where, turned how), smoothly or straight from one to the next: a ship setting down, a door
typedef struct {
	int			start;
	int			ms;					// 0 to the end
	char		model[MAX_QPATH];	// models/....md3
	float		scale;
	qboolean	smooth;
	int			numKeys;
	cmPlace_t	keys[CM_PROP_KEYS];
} cmProp_t;

// a light from its moment for as long as it lasts: at a spot or on someone, steady or flickering,
// pulsing, strobing or a flash dying away, faded in and out
typedef enum {
	CM_LIGHT_STEADY,
	CM_LIGHT_FLICKER,
	CM_LIGHT_PULSE,
	CM_LIGHT_STROBE,
	CM_LIGHT_FLASH,
	CM_NUM_LIGHT_FX
} cmLightFx_t;

static const char *cmLightFx[CM_NUM_LIGHT_FX] = { "steady", "flickering", "pulsing", "strobe", "a flash, dying away" };

typedef struct {
	int			start;
	int			ms;					// 0 to the end
	vec3_t		org;
	char		who[MAX_QPATH];		// on someone, "" at org
	float		radius;
	vec3_t		color;
	int			fx;					// cmLightFx_t
	int			fade;				// ms in and out
} cmLight_t;

// over the whole picture from its moment for as long as it lasts: a flash dying away, a tint of
// a colour faded in and out, or the edges darkened
typedef enum {
	CM_SCREEN_FLASH,
	CM_SCREEN_TINT,
	CM_SCREEN_VIGNETTE,
	CM_NUM_SCREENS
} cmScreenKind_t;

static const char *cmScreenKinds[CM_NUM_SCREENS] = { "a flash", "a tint", "dark edges" };

typedef struct {
	int			start;
	int			ms;
	int			kind;				// cmScreenKind_t
	vec3_t		color;
	int			strength;			// percent
	int			fade;				// ms in and out, a tint's and the edges'
} cmScreen_t;

// the map's look for a while, faded in and out: its light, how bright and what colour, the sky's
// colour, and fog; one over another blends into it
typedef struct {
	int			start;
	int			ms;					// 0 to the end
	int			fade;
	int			light;				// percent
	vec3_t		lightColor;
	vec3_t		skyColor;
	float		fogDepth;			// how deep it takes to see nothing through it, 0 no fog
	vec3_t		fogColor;
	char		layout[MAX_QPATH];	// a map editor layout shown meanwhile: its props hidden or moved, textures...
} cmMapLook_t;

// a stretch of it played slower or faster: what the cinematic moves, the camera, those it poses,
// its extras, props, lights and words; not what the server moves
typedef struct {
	int			start;				// ms of the cinematic's own time
	int			ms;
	int			speed;				// percent
} cmSpeed_t;

typedef struct {
	qboolean	bars, hud;
	qboolean	onlyCast;			// no one but those in it seen while it plays
	int			barSize;			// 0 thin, 1 as they were, 2 wide
	int			numMarks;			// moments marked on the timeline, things snap to them
	int			marks[CM_MAX_MARKS];
	char		markNames[CM_MAX_MARKS][32];
	int			numShots;
	rcShot_t	shots[CM_MAX_SHOTS];
	int			numEnts;
	cmEnt_t		ents[CM_MAX_ENTS];
	int			numClips;
	cmClip_t	clips[CM_MAX_CLIPS];
	cmRec_t		recs[CM_MAX_RECS];
	int			numFx;
	cmFx_t		fx[CM_MAX_FX];
	int			numSnd;
	cmSnd_t		snd[CM_MAX_SND];
	int			numTxt;
	cmTxt_t		txt[CM_MAX_TXT];
	int			numProps;
	cmProp_t	props[CM_MAX_PROPS];
	int			numLights;
	cmLight_t	lights[CM_MAX_LIGHTS];
	int			numScreens;
	cmScreen_t	screens[CM_MAX_SCREENS];
	int			numMapLooks;
	cmMapLook_t	mapLooks[CM_MAX_MAPLOOKS];
	int			worldVolume;		// the game's own sounds while it plays, percent; its music as it is
	int			numSpeeds;
	cmSpeed_t	speeds[CM_MAX_SPEEDS];
} cmCine_t;

// how much of the screen's height each of a cinematic's bars takes
static float CM_BarFrac( const cmCine_t *c ) {
	static const float fracs[3] = { 0.07f, RC_BAR_FRAC, 0.18f };

	return fracs[Com_Clampi( 0, 2, c->barSize )];
}

// a move as it goes: each of them from where they are then through the spots, a group
// keeping its shape
typedef struct {
	int			count;
	int			nums[CM_MAX_GROUP];
	vec3_t		pts[CM_MAX_GROUP][CM_MAX_PTS + 1];
	int			numPts;
	float		speed;
	int			ms;					// until the last of them is there
} cmPlan_t;

static const char *cmModeWords[RC_NUM_AMODES] = { "play", "reverse", "hold" };

/*
===============================================================================

Cinematics

===============================================================================
*/

static int CM_ShotLen( const rcShot_t *s ) {
	return RC_ShotMs( s );
}

static int CM_ShotStart( const cmCine_t *c, int shot ) {
	int ms = 0;

	for ( int i = 0; i < shot && i < c->numShots; i++ )
		ms += CM_ShotLen( &c->shots[i] );
	return ms;
}

static int CM_Total( const cmCine_t *c ) {
	return CM_ShotStart( c, c->numShots );
}

// the shot playing t ms in, and how far into it; past the end, the last at its end
static int CM_ShotAt( const cmCine_t *c, int t, int *local ) {
	int from = 0;

	for ( int i = 0; i < c->numShots; i++ ) {
		const int len = CM_ShotLen( &c->shots[i] );

		if ( t < from + len || i == c->numShots - 1 ) {
			*local = Com_Clampi( 0, len, t - from );
			return i;
		}
		from += len;
	}
	*local = 0;
	return -1;
}

// how far into the shot a spot is
static int CM_KeyMs( const rcShot_t *s, int key ) {
	int ms = 0;

	for ( int i = 1; i <= key && i < s->numKeys; i++ )
		ms += s->keys[i].ms;
	return ms;
}

static qboolean CM_NameChar( int ch ) {
	return (qboolean)( isalnum( ch ) || ch == '_' || ch == '-' );
}

// a name without its colour codes: ^ and whatever follows it, RPMod's letters as well as digits
static void CM_StripName( const char *in, char *out, int size ) {
	int len = 0;

	for ( ; *in && len < size - 1; in++ ) {
		if ( *in == Q_COLOR_ESCAPE && in[1] ) {
			in++;
			continue;
		}
		out[len++] = *in;
	}
	out[len] = '\0';
}

// the player who goes by that name, colours left out; -1 if nobody does
static int CM_FindPlayer( const char *name ) {
	for ( int c = 0; c < MAX_CLIENTS; c++ ) {
		const int ofs = cl.gameState.stringOffsets[CS_PLAYERS + c];
		char plain[MAX_QPATH];

		if ( !ofs )
			continue;
		CM_StripName( Info_ValueForKey( cl.gameState.stringData + ofs, "n" ), plain, sizeof( plain ) );
		if ( plain[0] && !Q_stricmp( plain, name ) )
			return c;
	}
	return -1;
}

// everyone a who names: me, numbers, players by their name, every NPC with a targetname
static int CM_Resolve( const char *who, int *nums, int max ) {
	int count = 0;

	for ( const char *p = who; *p && count < max; ) {
		char item[MAX_QPATH];
		int len = 0, found[CM_MAX_GROUP], n = 0;

		while ( *p && *p != ',' ) {
			if ( len < (int)sizeof( item ) - 1 )
				item[len++] = *p;
			p++;
		}
		item[len] = '\0';
		if ( *p )
			p++;
		if ( !item[0] )
			continue;
		if ( CM_IsExtra( item ) && ( found[0] = CL_Extras_Find( item ) ) >= 0 )
			n = 1;
		else if ( CM_IsExtra( item ) )
			continue;
		if ( n <= 0 && Q_stricmp( item, "me" ) && !Q_isanumber( item ) && ( found[0] = CM_FindPlayer( item ) ) >= 0 )
			n = 1;
		if ( n <= 0 && Q_stricmp( item, "me" ) && !Q_isanumber( item ) )
			n = CL_NpcWalk_Targets( item, found, CM_MAX_GROUP );
		if ( n <= 0 )
			n = RC_AnimTargets( item, found, 1 );
		for ( int i = 0; i < n && count < max; i++ ) {
			int j = 0;

			while ( j < count && nums[j] != found[i] )
				j++;
			if ( j == count )
				nums[count++] = found[i];
		}
	}
	return count;
}

static qboolean CM_EntOrigin( int num, vec3_t out ) {
	if ( num == clc.clientNum ) {
		VectorCopy( cl.snap.ps.origin, out );
		return qtrue;
	}
	return CL_NpcWalk_NpcOrigin( num, out );
}

// how long an animation or an emote lasts: its own, or to the end
static int CM_AnimLen( const cmCine_t *c, const cmClip_t *clip ) {
	return clip->a.ms > 0 ? clip->a.ms : Q_max( 0, CM_Total( c ) - clip->start );
}

// a move, a position or a recording: where someone is
static qboolean CM_Moves( int kind ) {
	return (qboolean)( kind == CM_CLIP_MOVE || kind == CM_CLIP_PLACE || kind == CM_CLIP_RECORD );
}

// an NPC command, a spawn or a removal: done at its moment, at once
static qboolean CM_Instant( int kind ) {
	return (qboolean)( kind == CM_CLIP_NPCCMD || kind == CM_CLIP_SPAWN || kind == CM_CLIP_REMOVE );
}

// someone taken out by then: not posed after
static qboolean CM_GoneAt( const cmCine_t *c, int ent, int t ) {
	for ( int i = 0; i < c->numClips; i++ ) {
		if ( c->clips[i].ent == ent && c->clips[i].kind == CM_CLIP_REMOVE && t >= c->clips[i].start )
			return qtrue;
	}
	return qfalse;
}

static const cmRec_t *CM_Rec( const cmCine_t *c, const cmClip_t *clip ) {
	return clip->rec >= 0 && clip->rec < CM_MAX_RECS && c->recs[clip->rec].count ? &c->recs[clip->rec] : NULL;
}

static int CM_RecLen( const cmCine_t *c, const cmClip_t *clip ) {
	const cmRec_t *r = CM_Rec( c, clip );

	return r ? r->s[r->count - 1].ms : 0;
}

static int CM_PlaceLen( const cmClip_t *clip ) {
	return clip->numKeys ? clip->keys[clip->numKeys - 1].ms : 0;
}

static float CM_CatmullRom( float p0, float p1, float p2, float p3, float t ) {
	return 0.5f * ( 2.0f * p1 + ( p2 - p0 ) * t + ( 2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3 ) * t * t
		+ ( 3.0f * p1 - p0 - 3.0f * p2 + p3 ) * t * t * t );
}

// where keys have something ms in, and how it's turned: smoothly through them, or straight
static void CM_KeysAt( const cmPlace_t *keys, int n, qboolean smooth, int ms, vec3_t org, vec3_t ang ) {
	int i;

	if ( !n ) {
		VectorClear( org );
		VectorClear( ang );
		return;
	}
	if ( n == 1 || ms <= keys[0].ms || ms >= keys[n - 1].ms ) {
		const cmPlace_t *k = ms <= keys[0].ms ? &keys[0] : &keys[n - 1];

		VectorCopy( k->org, org );
		VectorCopy( k->ang, ang );
		return;
	}
	for ( i = 1; i < n - 1 && ms > keys[i].ms; i++ )
		;
	{
		const cmPlace_t *k0 = &keys[Q_max( i - 2, 0 )], *k1 = &keys[i - 1], *k2 = &keys[i], *k3 = &keys[Q_min( i + 1, n - 1 )];
		const int span = k2->ms - k1->ms;
		const float f = span > 0 ? Com_Clamp( 0.0f, 1.0f, (float)( ms - k1->ms ) / span ) : 1.0f;

		for ( int j = 0; j < 3; j++ ) {
			org[j] = smooth ? CM_CatmullRom( k0->org[j], k1->org[j], k2->org[j], k3->org[j], f ) : k1->org[j] + ( k2->org[j] - k1->org[j] ) * f;
			ang[j] = LerpAngle( k1->ang[j], k2->ang[j], smooth ? RC_Ease( f, qfalse ) : f );
		}
	}
}

// where a position has them ms into it, and how they're turned
static void CM_PlaceAt( const cmClip_t *clip, int ms, vec3_t org, vec3_t ang ) {
	CM_KeysAt( clip->keys, clip->numKeys, clip->smooth, ms, org, ang );
}

// a prop and a light: when they go, where they are, how bright
static int CM_MapLookEnd( const cmCine_t *c, const cmMapLook_t *m ) {
	return m->ms > 0 ? m->start + m->ms : Q_max( m->start + 1, CM_Total( c ) );
}

static int CM_PropEnd( const cmCine_t *c, const cmProp_t *pr ) {
	return pr->ms > 0 ? pr->start + pr->ms : Q_max( pr->start + 1, CM_Total( c ) );
}

static int CM_LightEnd( const cmCine_t *c, const cmLight_t *l ) {
	return l->ms > 0 ? l->start + l->ms : Q_max( l->start + 1, CM_Total( c ) );
}

// where a prop is t ms into the cinematic, qfalse if it isn't there then; t < 0 at its first key
static qboolean CM_PropAt( const cmCine_t *c, const cmProp_t *pr, int t, vec3_t org, vec3_t ang ) {
	if ( !pr->numKeys )
		return qfalse;
	if ( t < 0 ) {
		VectorCopy( pr->keys[0].org, org );
		VectorCopy( pr->keys[0].ang, ang );
		return qtrue;
	}
	if ( t < pr->start || t >= CM_PropEnd( c, pr ) )
		return qfalse;
	CM_KeysAt( pr->keys, pr->numKeys, pr->smooth, t - pr->start, org, ang );
	return qtrue;
}

// how bright a light is in ms into it, of len: faded in and out, and as it flickers
static float CM_LightNow( const cmLight_t *l, int in, int len ) {
	float a = 1.0f;

	if ( l->fade > 0 ) {
		a = Q_min( a, (float)in / l->fade );
		a = Q_min( a, (float)( len - in ) / l->fade );
	}
	switch ( l->fx ) {
	case CM_LIGHT_FLICKER: {
		// the same flicker on every screen: from the time, not chance
		const unsigned h = (unsigned)( in / 60 + 1 ) * 2654435761u;

		a *= 0.35f + 0.65f * ( ( h >> 16 ) & 255 ) / 255.0f;
		break;
	}
	case CM_LIGHT_PULSE:
		a *= 0.55f + 0.45f * sinf( in * 0.00628f );
		break;
	case CM_LIGHT_STROBE:
		a *= ( in % 250 ) < 70 ? 1.0f : 0.0f;
		break;
	case CM_LIGHT_FLASH: {
		const float f = 1.0f - (float)in / Q_max( 1, len );

		a *= f * f;
		break;
	}
	default:
		break;
	}
	return Com_Clamp( 0.0f, 1.0f, a );
}

// where a light is: on whoever it follows, a little over their middle, else its spot
static void CM_LightOrg( const cmLight_t *l, vec3_t org ) {
	int num;

	if ( l->who[0] && CM_Resolve( l->who, &num, 1 ) && RC_EntityOrigin( num, org ) ) {
		org[2] += 16.0f;
		return;
	}
	VectorCopy( l->org, org );
}


// what a recording has ms into it: where from where it began, how they're turned; the moment's
// doings, NULL if it has none
static const cmSample_t *CM_SampleAt( const cmRec_t *r, int ms, vec3_t org, vec3_t ang ) {
	const cmSample_t *a;
	int i;

	VectorClear( ang );
	if ( !r || !r->count ) {
		VectorClear( org );
		return NULL;
	}
	for ( i = 0; i < r->count - 1 && r->s[i + 1].ms <= ms; i++ )
		;
	a = &r->s[i];
	VectorCopy( a->org, org );
	ang[PITCH] = a->pitch;
	ang[YAW] = a->yaw;
	if ( i + 1 < r->count && ms > a->ms && r->s[i + 1].ms > a->ms ) {
		const cmSample_t *b = &r->s[i + 1];
		const float f = Com_Clamp( 0.0f, 1.0f, (float)( ms - a->ms ) / ( b->ms - a->ms ) );

		for ( int j = 0; j < 3; j++ )
			org[j] = a->org[j] + ( b->org[j] - a->org[j] ) * f;
		ang[PITCH] = a->pitch + ( b->pitch - a->pitch ) * f;
		ang[YAW] = LerpAngle( a->yaw, b->yaw, f );
	}
	return a;
}

// where one of them is t ms into a move, a position or a recording, and how they're turned;
// qtrue while it's still under way
static qboolean CM_PlanAt( const cmCine_t *c, const cmPlan_t *p, const cmClip_t *clip, int m, int t, vec3_t pos, vec3_t ang ) {
	vec3_t org;

	VectorClear( ang );
	switch ( clip->kind ) {
	case CM_CLIP_PLACE:
		CM_PlaceAt( clip, t, org, ang );
		VectorAdd( org, p->pts[m][1], pos );	// a group keeps its shape
		return (qboolean)( t < CM_PlaceLen( clip ) );
	case CM_CLIP_RECORD:
		CM_SampleAt( CM_Rec( c, clip ), t, org, ang );
		VectorAdd( p->pts[m][0], org, pos );
		return (qboolean)( t < CM_RecLen( c, clip ) );
	default: {
		float yaw;
		const qboolean moving = CL_NpcWalk_RouteAt( p->pts[m], p->numPts, p->speed, t, pos, &yaw );

		ang[YAW] = yaw;
		return moving;
	}
	}
}

// how long one of them takes over a move, a position or a recording
static int CM_PlanLen( const cmCine_t *c, const cmPlan_t *p, const cmClip_t *clip, int m ) {
	if ( clip->kind == CM_CLIP_PLACE )
		return CM_PlaceLen( clip );
	if ( clip->kind == CM_CLIP_RECORD )
		return CM_RecLen( c, clip );
	return CL_NpcWalk_RouteMs( p->pts[m], p->numPts, p->speed );
}

// one entity's moves, positions and recordings worked out: each from where it stands now, or
// where the one before has it then
static void CM_PlanEnt( const cmCine_t *c, cmPlan_t *plans, int e ) {
	int order[CM_MAX_CLIPS], n = 0, nums[CM_MAX_GROUP], count, prev = -1;

	for ( int i = 0; i < c->numClips; i++ ) {
		if ( c->clips[i].ent == e ) {
			plans[i].count = 0;
			plans[i].ms = 0;
		}
	}

	for ( int i = 0; i < c->numClips; i++ ) {
		const cmClip_t *clip = &c->clips[i];
		int j = n;

		if ( clip->ent != e || !CM_Moves( clip->kind ) || ( clip->kind == CM_CLIP_MOVE && !clip->numPts )
			|| ( clip->kind == CM_CLIP_PLACE && !clip->numKeys ) || ( clip->kind == CM_CLIP_RECORD && !CM_Rec( c, clip ) ) )
			continue;
		for ( ; j > 0 && c->clips[order[j - 1]].start > clip->start; j-- )
			order[j] = order[j - 1];
		order[j] = i;
		n++;
	}
	if ( !n )
		return;
	count = CM_Resolve( c->ents[e].who, nums, CM_MAX_GROUP );
	for ( int k = 0; k < n; k++ ) {
		const cmClip_t *clip = &c->clips[order[k]];
		cmPlan_t *p = &plans[order[k]];
		vec3_t starts[CM_MAX_GROUP], middle;
		int members = 0;

		if ( prev < 0 ) {
			for ( int m = 0; m < count; m++ ) {
				if ( CM_EntOrigin( nums[m], starts[members] ) )
					p->nums[members++] = nums[m];
			}
			// out of sight, from the first spot or key
			if ( !members && count == 1 && clip->kind != CM_CLIP_RECORD ) {
				p->nums[0] = nums[0];
				VectorCopy( clip->kind == CM_CLIP_PLACE ? clip->keys[0].org : clip->pts[0], starts[0] );
				members = 1;
			}
		} else {
			const cmPlan_t *q = &plans[prev];

			members = q->count;
			for ( int m = 0; m < members; m++ ) {
				vec3_t ang;

				p->nums[m] = q->nums[m];
				CM_PlanAt( c, q, &c->clips[prev], m, clip->start - c->clips[prev].start, starts[m], ang );
			}
		}
		if ( !members )
			continue;
		CL_NpcWalk_GroupMiddle( starts, members, middle );
		for ( int m = 0; m < members; m++ ) {
			vec3_t offset;

			VectorCopy( starts[m], p->pts[m][0] );
			VectorSubtract( starts[m], middle, offset );
			offset[2] = 0.0f;
			if ( clip->kind == CM_CLIP_PLACE ) {
				if ( members == 1 )
					VectorClear( offset );
				VectorCopy( offset, p->pts[m][1] );
			}
			for ( int j = 0; clip->kind == CM_CLIP_MOVE && j < clip->numPts; j++ ) {
				if ( members == 1 )
					VectorCopy( clip->pts[j], p->pts[m][j + 1] );
				else
					CL_NpcWalk_GroupSpot( clip->pts[j], offset, p->pts[m][j + 1] );
			}
		}
		p->count = members;
		p->numPts = clip->kind == CM_CLIP_MOVE ? clip->numPts + 1 : 1;
		p->speed = CL_NpcWalk_PaceSpeed( clip->run );
		for ( int m = 0; m < members; m++ )
			p->ms = Q_max( p->ms, CM_PlanLen( c, p, clip, m ) );
		prev = order[k];
	}
}

// everyone's
static void CM_Plan( const cmCine_t *c, cmPlan_t *plans ) {
	for ( int i = 0; i < c->numClips; i++ ) {
		plans[i].count = 0;
		plans[i].ms = 0;
	}
	for ( int e = 0; e < c->numEnts; e++ )
		CM_PlanEnt( c, plans, e );
}

static cvar_t *rpc_cinNoPredict;		// 1 while cg_noPredict is ours to put back to 0, a crash included

// your own player moved by a cinematic: the cgame draws you from the snapshot, where you're
// moved to, not from where it predicts you'd be
static void CM_HoldSelf( qboolean on ) {
	if ( !rpc_cinNoPredict )
		return;
	if ( on ) {
		if ( Cvar_VariableIntegerValue( "cg_noPredict" ) || rpc_cinNoPredict->integer )
			return;
		Cvar_Set( "rpc_cinNoPredict", "1" );
		Cvar_Set( "cg_noPredict", "1" );
	} else if ( rpc_cinNoPredict->integer ) {
		Cvar_Set( "cg_noPredict", "0" );
		Cvar_Set( "rpc_cinNoPredict", "0" );
	}
}

// the recordings' events heard from the moment t was last at, while it plays on
static const cmCine_t *cmEvCine;
static int cmEvT[CM_MAX_CLIPS];

static void CM_RecEvents( const cmRec_t *r, const cmClip_t *clip, int from, int t, const int *nums, int count ) {
	if ( !r || t <= from || t - from > 400 )
		return;
	for ( int e = 0; e < r->numEvents; e++ ) {
		const int at = clip->start + r->ev[e].ms;

		if ( at <= from || at > t )
			continue;
		for ( int m = 0; m < count; m++ )
			CL_NpcWalk_PoseEvent( nums[m], r->ev[e].event, r->ev[e].parm );
	}
}

static qboolean CM_Playing( const cmCine_t *c );
static void CM_WorldVolume( const cmCine_t *c );
static void CM_SyncExtras( void );

// everyone where the cinematic has them t ms in, on their animation's frame
static void CM_Apply( const cmCine_t *c, const cmPlan_t *plans, int t ) {
	static rcAnim_t emoteAnims[CM_MAX_CLIPS];
	int order[CM_MAX_CLIPS], n = 0;
	int nums[32], elapsed[32], starts[32], count = 0;
	const rcAnim_t *anims[32];
	const qboolean heard = CM_Playing( c );

	if ( c != cmEvCine ) {
		cmEvCine = c;
		for ( int i = 0; i < CM_MAX_CLIPS; i++ )
			cmEvT[i] = t;
	}

	// where they are: the moves, positions and recordings by when they start, a later one over an
	// earlier
	CL_NpcWalk_ClearPoses();
	for ( int i = 0; i < c->numClips; i++ ) {
		int j = n;

		if ( !CM_Moves( c->clips[i].kind ) )
			continue;
		for ( ; j > 0 && c->clips[order[j - 1]].start > c->clips[i].start; j-- )
			order[j] = order[j - 1];
		order[j] = i;
		n++;
	}
	for ( int k = 0; k < n; k++ ) {
		const cmClip_t *clip = &c->clips[order[k]];
		const cmPlan_t *p = &plans[order[k]];
		const int from = cmEvT[order[k]];

		cmEvT[order[k]] = t;
		if ( t <= clip->start || CM_GoneAt( c, clip->ent, t ) )
			continue;
		if ( heard && clip->kind == CM_CLIP_RECORD )
			CM_RecEvents( CM_Rec( c, clip ), clip, from, t, p->nums, p->count );
		for ( int m = 0; m < p->count; m++ ) {
			vec3_t pos, ang;
			const qboolean going = CM_PlanAt( c, p, clip, m, t - clip->start, pos, ang );

			if ( clip->kind == CM_CLIP_PLACE ) {
				CL_NpcWalk_PoseEx( p->nums[m], pos, ang, 0, qtrue, NULL, qtrue );
			} else if ( clip->kind == CM_CLIP_RECORD ) {
				vec3_t org, sampleAng;
				const cmSample_t *sample = going ? CM_SampleAt( CM_Rec( c, clip ), t - clip->start, org, sampleAng ) : NULL;
				npcPoseState_t st;

				memset( &st, 0, sizeof( st ) );
				if ( sample ) {
					st.legsAnim = sample->legs;
					st.torsoAnim = sample->torso;
					st.legsFlip = (qboolean)( ( sample->flags & 1 ) != 0 );
					st.torsoFlip = (qboolean)( ( sample->flags & 2 ) != 0 );
					st.forcePowersActive = sample->force;
					st.saberHolstered = sample->holstered;
					st.saberMove = sample->saberMove;
				}
				CL_NpcWalk_PoseEx( p->nums[m], pos, ang, 0, qtrue, sample ? &st : NULL, qfalse );
			} else {
				CL_NpcWalk_PoseEx( p->nums[m], pos, ang, going ? ( clip->run ? 2 : 1 ) : 0, qfalse, NULL, qfalse );
			}
		}
	}

	// on each one, the animation or emote that started last, while it lasts; a force power's on the
	// one using it and the one it's at
	for ( int i = 0; i < c->numClips; i++ ) {
		const cmClip_t *clip = &c->clips[i];
		const int len = clip->kind == CM_CLIP_ANIM || clip->kind == CM_CLIP_EMOTE || clip->kind == CM_CLIP_FORCE ? CM_AnimLen( c, clip ) : 0;
		const rcAnim_t *a = &clip->a;
		int targets[CM_MAX_GROUP], numTargets;

		if ( clip->kind == CM_CLIP_FORCE && clip->forceAnims && t >= clip->start && ( len <= 0 || t < clip->start + len ) ) {
			static rcAnim_t forceAnims[CM_MAX_CLIPS][2];
			const cmForce_t *f = &cmForces[clip->power];

			for ( int side = 0; side < 2; side++ ) {
				const char *anim = side ? f->victim : f->anim;
				const int who = side ? clip->target : clip->ent;
				rcAnim_t *fa = &forceAnims[i][side];

				if ( !anim || who < 0 || who >= c->numEnts || CM_GoneAt( c, who, t ) )
					continue;
				memset( fa, 0, sizeof( *fa ) );
				Q_strncpyz( fa->anim, anim, sizeof( fa->anim ) );
				fa->to = -1;
				fa->loop = qtrue;
				fa->speed = 100;
				numTargets = CM_Resolve( c->ents[who].who, targets, CM_MAX_GROUP );
				for ( int m = 0; m < numTargets; m++ ) {
					int j = 0;

					while ( j < count && nums[j] != targets[m] )
						j++;
					if ( j == count ) {
						if ( count == (int)ARRAY_LEN( nums ) )
							continue;
						count++;
					} else if ( starts[j] > clip->start ) {
						continue;
					}
					nums[j] = targets[m];
					anims[j] = fa;
					starts[j] = clip->start;
					elapsed[j] = t - clip->start;
				}
			}
			continue;
		}
		if ( ( clip->kind != CM_CLIP_ANIM && clip->kind != CM_CLIP_EMOTE ) || t < clip->start || ( len > 0 && t >= clip->start + len )
			|| CM_GoneAt( c, clip->ent, t ) )
			continue;
		// an emote shows the animation it plays
		if ( clip->kind == CM_CLIP_EMOTE ) {
			const char *anim = CL_EmotePreview_Anim( clip->emote );

			if ( !anim )
				continue;
			emoteAnims[i] = clip->a;
			Q_strncpyz( emoteAnims[i].anim, anim, sizeof( emoteAnims[i].anim ) );
			emoteAnims[i].from = 0;
			emoteAnims[i].to = -1;
			a = &emoteAnims[i];
		}
		numTargets = CM_Resolve( c->ents[clip->ent].who, targets, CM_MAX_GROUP );
		for ( int m = 0; m < numTargets; m++ ) {
			int j = 0;

			while ( j < count && nums[j] != targets[m] )
				j++;
			if ( j == count ) {
				if ( count == (int)ARRAY_LEN( nums ) )
					continue;
				count++;
			} else if ( starts[j] > clip->start ) {
				continue;
			}
			nums[j] = targets[m];
			anims[j] = a;
			starts[j] = clip->start;
			elapsed[j] = t - clip->start;
		}
	}
	RC_TimelineAnims( nums, anims, elapsed, count );
	{
		vec3_t pos;

		CM_HoldSelf( CL_NpcWalk_PoseOf( clc.clientNum, pos ) );
	}
}

static void CM_Unapply( void ) {
	CL_NpcWalk_ClearPoses();
	RC_TimelineClear();
	CM_HoldSelf( qfalse );
}

/*
===============================================================================

Cinematic files

===============================================================================
*/

static const char *CM_Path( const char *name ) {
	return va( "%s/%s%s", CM_DIR, name, CM_EXT );
}

static qboolean CM_ValidName( const char *name ) {
	if ( !name[0] )
		return qfalse;
	for ( const char *c = name; *c; c++ ) {
		if ( !CM_NameChar( *c ) )
			return qfalse;
	}
	return qtrue;
}

static int CM_FindEnt( cmCine_t *c, const char *who, qboolean add ) {
	for ( int i = 0; i < c->numEnts; i++ ) {
		if ( !Q_stricmp( c->ents[i].who, who ) )
			return i;
	}
	if ( !add || c->numEnts >= CM_MAX_ENTS )
		return -1;
	Q_strncpyz( c->ents[c->numEnts].who, who, sizeof( c->ents[0].who ) );
	return c->numEnts++;
}

static void CM_Default( cmCine_t *c ) {
	memset( c, 0, sizeof( *c ) );
	c->bars = qtrue;
	c->barSize = 1;
	c->worldVolume = 100;
}

static void CM_WriteLine( fileHandle_t f, const char *line ) {
	FS_Write( line, strlen( line ), f );
}

// the cinematic as its file has it, a line each; how long it is
static int CM_ToText( const cmCine_t *c, char *out, int size ) {
	char hook[MAX_STRING_CHARS * 2], line[MAX_STRING_CHARS * 2];
	int len = 0;

	out[0] = '\0';
#define CM_ADD( text ) do { const char *t_ = ( text ); const int l_ = strlen( t_ ); if ( len + l_ < size ) { memcpy( out + len, t_, l_ + 1 ); len += l_; } } while ( 0 )
	CM_ADD( va( "look %i %i %i %i %i\n", c->bars ? 1 : 0, c->hud ? 1 : 0, c->onlyCast ? 1 : 0, c->barSize, c->worldVolume ) );
	// maplook <start> <seconds> <fade> <light %> <r g b> <sky r g b> <fog depth, 0 none> <fog r g b>
	for ( int i = 0; i < c->numMapLooks; i++ ) {
		const cmMapLook_t *m = &c->mapLooks[i];

		CM_ADD( va( "maplook %g %g %g %i %g %g %g %g %g %g %i %g %g %g %s\n", RC_Seconds( m->start ), RC_Seconds( m->ms ), RC_Seconds( m->fade ), m->light,
			m->lightColor[0], m->lightColor[1], m->lightColor[2], m->skyColor[0], m->skyColor[1], m->skyColor[2], RC_Round( m->fogDepth ),
			m->fogColor[0], m->fogColor[1], m->fogColor[2], m->layout[0] ? m->layout : "-" ) );
	}
	for ( int i = 0; i < c->numMarks; i++ )
		CM_ADD( va( "mark %g %s\n", RC_Seconds( c->marks[i] ), c->markNames[i][0] ? c->markNames[i] : "-" ) );
	for ( int i = 0; i < c->numSpeeds; i++ )
		CM_ADD( va( "speed %g %g %i\n", RC_Seconds( c->speeds[i].start ), RC_Seconds( c->speeds[i].ms ), c->speeds[i].speed ) );
	// fx <start> <seconds, 0 once> <size> <x y z> <angles 0|1> <pitch yaw roll> <effect>
	for ( int i = 0; i < c->numFx; i++ ) {
		const cmFx_t *f = &c->fx[i];

		CM_ADD( va( "fx %g %g %g %i %i %i %i %i %i %i %s\n", RC_Seconds( f->start ), RC_Seconds( f->ms ), f->scale > 0.0f ? f->scale : 1.0f,
			RC_Round( f->org[0] ), RC_Round( f->org[1] ), RC_Round( f->org[2] ), f->haveAngles ? 1 : 0,
			RC_Round( AngleNormalize180( f->ang[PITCH] ) ), RC_Round( AngleNormalize360( f->ang[YAW] ) ), RC_Round( AngleNormalize180( f->ang[ROLL] ) ), f->path ) );
	}
	// snd <start> <seconds, 0 once> <spot 0|1> <x y z> <sound>
	for ( int i = 0; i < c->numSnd; i++ ) {
		const cmSnd_t *s = &c->snd[i];

		if ( s->music )
			CM_ADD( va( "music %g %g %s\n", RC_Seconds( s->start ), RC_Seconds( s->ms ), s->path ) );
		else
			CM_ADD( va( "snd %g %g %i %i %i %i %s\n", RC_Seconds( s->start ), RC_Seconds( s->ms ), s->spot ? 1 : 0,
				RC_Round( s->org[0] ), RC_Round( s->org[1] ), RC_Round( s->org[2] ), s->path ) );
	}
	// prop <start> <seconds, 0 to the end> <smooth 0|1> <scale> <ms,x,y,z,pitch,yaw,roll;...> <model>
	for ( int i = 0; i < c->numProps; i++ ) {
		const cmProp_t *pr = &c->props[i];
		char keys[CM_PROP_KEYS * 64] = "";

		for ( int k = 0; k < pr->numKeys; k++ ) {
			const cmPlace_t *pk = &pr->keys[k];

			Q_strcat( keys, sizeof( keys ), va( "%s%i,%i,%i,%i,%i,%i,%i", k ? ";" : "", pk->ms, RC_Round( pk->org[0] ), RC_Round( pk->org[1] ), RC_Round( pk->org[2] ),
				RC_Round( AngleNormalize180( pk->ang[PITCH] ) ), RC_Round( AngleNormalize360( pk->ang[YAW] ) ), RC_Round( AngleNormalize180( pk->ang[ROLL] ) ) ) );
		}
		if ( pr->numKeys && pr->model[0] )
			CM_ADD( va( "prop %g %g %i %g %s %s\n", RC_Seconds( pr->start ), RC_Seconds( pr->ms ), pr->smooth ? 1 : 0, pr->scale, keys, pr->model ) );
	}
	// light <start> <seconds, 0 to the end> <x y z> <radius> <r g b> <cmLightFx> <fade seconds> <who|->
	for ( int i = 0; i < c->numLights; i++ ) {
		const cmLight_t *l = &c->lights[i];

		CM_ADD( va( "light %g %g %i %i %i %i %g %g %g %i %g %s\n", RC_Seconds( l->start ), RC_Seconds( l->ms ), RC_Round( l->org[0] ), RC_Round( l->org[1] ),
			RC_Round( l->org[2] ), RC_Round( l->radius ), l->color[0], l->color[1], l->color[2], l->fx, RC_Seconds( l->fade ), l->who[0] ? l->who : "-" ) );
	}
	// screen <start> <seconds> <kind> <r g b> <strength %> <fade seconds>
	for ( int i = 0; i < c->numScreens; i++ ) {
		const cmScreen_t *sc = &c->screens[i];

		CM_ADD( va( "screen %g %g %i %g %g %g %i %g\n", RC_Seconds( sc->start ), RC_Seconds( sc->ms ), sc->kind, sc->color[0], sc->color[1], sc->color[2],
			sc->strength, RC_Seconds( sc->fade ) ) );
	}
	// txt <start> <seconds> <kind> <size> <typed 0|1> <who>|<text>
	for ( int i = 0; i < c->numTxt; i++ ) {
		const cmTxt_t *x = &c->txt[i];

		CM_ADD( va( "txt %g %g %i %i %i %s|%s\n", RC_Seconds( x->start ), RC_Seconds( x->ms ), x->kind, x->size, x->typed ? 1 : 0, x->who, x->text ) );
	}
	for ( int i = 0; i < c->numShots; i++ ) {
		RC_WriteShot( &c->shots[i], hook, sizeof( hook ) );
		Com_sprintf( line, sizeof( line ), "shot %g %s\n", c->shots[i].numKeys == 1 && !c->shots[i].orbit ? RC_Seconds( c->shots[i].ms ) : 0.0f, hook );
		CM_ADD( line );
	}
	for ( int i = 0; i < c->numEnts; i++ )
		CM_ADD( va( "ent %s\n", c->ents[i].who[0] ? c->ents[i].who : "-" ) );
	for ( int i = 0; i < c->numEnts; i++ ) {
		const cmEnt_t *e = &c->ents[i];

		if ( CM_IsExtra( e->who ) )
			CM_ADD( va( "extra %i %i %i %i %i %i %i %s\n", i, RC_Round( e->home[0] ), RC_Round( e->home[1] ), RC_Round( e->home[2] ),
				RC_Round( AngleNormalize360( e->yaw ) ), e->saber, e->color, e->model[0] ? e->model : "kyle" ) );
	}
	for ( int i = 0; i < c->numClips; i++ ) {
		const cmClip_t *clip = &c->clips[i];

		if ( clip->kind == CM_CLIP_MOVE ) {
			Com_sprintf( line, sizeof( line ), "move %i %g %s", clip->ent, RC_Seconds( clip->start ), clip->run ? "run" : "walk" );
			for ( int j = 0; j < clip->numPts; j++ )
				Q_strcat( line, sizeof( line ), va( "%s%i %i %i", j ? "|" : " ", RC_Round( clip->pts[j][0] ), RC_Round( clip->pts[j][1] ), RC_Round( clip->pts[j][2] ) ) );
			Q_strcat( line, sizeof( line ), "\n" );
			CM_ADD( line );
		} else if ( clip->kind == CM_CLIP_ANIM ) {
			const rcAnim_t *a = &clip->a;

			Com_sprintf( line, sizeof( line ), "anim %i %g %g %s %i %i %s %s %i\n", clip->ent, RC_Seconds( clip->start ), RC_Seconds( a->ms ), a->anim[0] ? a->anim : "BOTH_STAND1",
				a->from, a->to, cmModeWords[Com_Clampi( 0, RC_NUM_AMODES - 1, a->mode )], a->loop ? "loop" : "once", a->speed );
			CM_ADD( line );
		} else if ( clip->kind == CM_CLIP_SAY ) {
			Com_sprintf( line, sizeof( line ), "say %i %g %s|%s\n", clip->ent, RC_Seconds( clip->start ), clip->name, clip->text );
			CM_ADD( line );
		} else if ( clip->kind == CM_CLIP_FORCE ) {
			CM_ADD( va( "force %i %g %g %i %i %i\n", clip->ent, RC_Seconds( clip->start ), RC_Seconds( clip->a.ms ), clip->power, clip->target,
				clip->forceAnims ? 1 : 0 ) );
		} else if ( clip->kind == CM_CLIP_LOOK ) {
			CM_ADD( va( "appear %i %g %g %i %i %i %i %i %i %i %i %i %s %i\n", clip->ent, RC_Seconds( clip->start ), RC_Seconds( clip->a.ms ), clip->hidden ? 1 : 0,
				clip->size, clip->shell, clip->saber, clip->blade, clip->holdHand, RC_Round( clip->holdAng[PITCH] ), RC_Round( clip->holdAng[YAW] ),
				RC_Round( clip->holdAng[ROLL] ), clip->hold[0] ? clip->hold : "-", clip->face ) );
		} else if ( clip->kind == CM_CLIP_EMOTE ) {
			Com_sprintf( line, sizeof( line ), "emote %i %g %g %s %i %i\n", clip->ent, RC_Seconds( clip->start ), RC_Seconds( clip->a.ms ),
				clip->emote[0] ? clip->emote : "-", clip->a.loop ? 1 : 0, clip->realEmote ? 1 : 0 );
			CM_ADD( line );
		} else if ( clip->kind == CM_CLIP_PLACE && clip->tele && clip->numKeys ) {
			const cmPlace_t *k = &clip->keys[0];

			CM_ADD( va( "tele %i %g %i %i %i %i\n", clip->ent, RC_Seconds( clip->start ), RC_Round( k->org[0] ), RC_Round( k->org[1] ),
				RC_Round( k->org[2] ), RC_Round( AngleNormalize360( k->ang[YAW] ) ) ) );
		} else if ( clip->kind == CM_CLIP_NPCCMD ) {
			char word[sizeof( clip->cmd )];

			Q_strncpyz( word, clip->cmd[0] ? clip->cmd : cmNpcCmds[0], sizeof( word ) );
			for ( char *w = word; *w; w++ ) {
				if ( *w == ' ' )
					*w = '_';
			}
			CM_ADD( va( "npccmd %i %g %s%s%s\n", clip->ent, RC_Seconds( clip->start ), word, clip->text[0] ? " " : "", clip->text ) );
		} else if ( clip->kind == CM_CLIP_SPAWN ) {
			const cmPlace_t *k = &clip->keys[0];

			CM_ADD( va( "spawn %i %g %i %i %i %i %s %s\n", clip->ent, RC_Seconds( clip->start ), RC_Round( k->org[0] ), RC_Round( k->org[1] ),
				RC_Round( k->org[2] ), RC_Round( AngleNormalize360( k->ang[YAW] ) ), clip->team[0] ? clip->team : "default",
				clip->npcType[0] ? clip->npcType : "-" ) );
		} else if ( clip->kind == CM_CLIP_REMOVE ) {
			CM_ADD( va( "remove %i %g\n", clip->ent, RC_Seconds( clip->start ) ) );
		} else if ( clip->kind == CM_CLIP_PLACE ) {
			Com_sprintf( line, sizeof( line ), "place %i %g %i", clip->ent, RC_Seconds( clip->start ), clip->smooth ? 1 : 0 );
			for ( int j = 0; j < clip->numKeys; j++ ) {
				const cmPlace_t *k = &clip->keys[j];

				Q_strcat( line, sizeof( line ), va( "%s%i %i %i %i %i %i %i", j ? "|" : " ", k->ms, RC_Round( k->org[0] ), RC_Round( k->org[1] ), RC_Round( k->org[2] ),
					RC_Round( AngleNormalize180( k->ang[PITCH] ) ), RC_Round( AngleNormalize360( k->ang[YAW] ) ), RC_Round( AngleNormalize180( k->ang[ROLL] ) ) ) );
			}
			Q_strcat( line, sizeof( line ), "\n" );
			CM_ADD( line );
		} else if ( clip->kind == CM_CLIP_RECORD && CM_Rec( c, clip ) ) {
			CM_ADD( va( "rec %i %g %i\n", clip->ent, RC_Seconds( clip->start ), clip->rec ) );
		}
	}
	// the recordings' moments
	for ( int r = 0; r < CM_MAX_RECS; r++ ) {
		qboolean used = qfalse;

		for ( int i = 0; i < c->numClips && !used; i++ )
			used = (qboolean)( c->clips[i].kind == CM_CLIP_RECORD && c->clips[i].rec == r );
		for ( int j = 0; used && j < c->recs[r].count; j++ ) {
			const cmSample_t *sm = &c->recs[r].s[j];

			Com_sprintf( line, sizeof( line ), "rs %i %i %i %i %i %i %i %i %i %i %i %i %i\n", r, sm->ms, RC_Round( sm->org[0] ), RC_Round( sm->org[1] ),
				RC_Round( sm->org[2] ), RC_Round( sm->pitch ), RC_Round( AngleNormalize360( sm->yaw ) ), sm->legs, sm->torso, sm->flags, sm->holstered, sm->force, sm->saberMove );
			CM_ADD( line );
		}
		for ( int j = 0; used && j < c->recs[r].numEvents; j++ ) {
			const cmRecEvent_t *e = &c->recs[r].ev[j];

			CM_ADD( va( "re %i %i %i %i\n", r, e->ms, e->event, e->parm ) );
		}
	}
#undef CM_ADD
	return len;
}

static qboolean CM_Save( const cmCine_t *c, const char *name ) {
	fileHandle_t f = FS_FOpenFileWrite( CM_Path( name ) );
	char *text;

	if ( !f )
		return qfalse;
	text = (char *)Z_Malloc( CM_TEXT_MAX, TAG_GENERAL, qfalse );
	CM_ToText( c, text, CM_TEXT_MAX );
	CM_WriteLine( f, CM_HEADER );
	CM_WriteLine( f, text );
	Z_Free( text );
	FS_FCloseFile( f );
	return qtrue;
}

// a cinematic's text, a line at a time, into c
static void CM_Parse( char *text, cmCine_t *c ) {
	char *line, *next;

	CM_Default( c );
	for ( line = text; line && *line; line = next ) {
		char *end;

		next = strchr( line, '\n' );
		if ( next )
			*next++ = '\0';
		if ( ( end = strchr( line, '\r' ) ) != NULL )
			*end = '\0';
		if ( !Q_stricmpn( line, "look ", 5 ) ) {
			int bars = 1, hud = 0;

			int only = 0, size = 1, volume = 100;

			sscanf( line + 5, "%i %i %i %i %i", &bars, &hud, &only, &size, &volume );
			c->worldVolume = Com_Clampi( 0, 100, volume );
			c->onlyCast = (qboolean)( only != 0 );
			c->barSize = Com_Clampi( 0, 2, size );
			c->bars = (qboolean)( bars != 0 );
			c->hud = (qboolean)( hud != 0 );
		} else if ( !Q_stricmpn( line, "shot ", 5 ) && c->numShots < CM_MAX_SHOTS ) {
			char *hook, *argv[RC_MAX_KEYS * 2 + RC_MAX_FOCUS + 8];
			float hold = (float)atof( line + 5 );
			int argc;

			hook = strchr( line + 5, ' ' );
			if ( !hook )
				continue;
			argc = RC_SplitHook( hook + 1, argv, ARRAY_LEN( argv ) );
			if ( argc < 2 || !RC_ParseShot( argv + 1, argc - 1, &c->shots[c->numShots], vec3_origin ) )
				continue;
			RC_FillFov( &c->shots[c->numShots], RC_DEFAULT_FOV );
			if ( !c->shots[c->numShots].orbit && c->shots[c->numShots].numKeys == 1 )
				c->shots[c->numShots].ms = Q_max( 100, RC_SecondsMs( hold ) );
			c->numShots++;
		} else if ( !Q_stricmpn( line, "fx ", 3 ) && c->numFx < CM_MAX_FX ) {
			cmFx_t *f = &c->fx[c->numFx];
			float start, secs, scale;
			int have = 0, n = 0;

			memset( f, 0, sizeof( *f ) );
			if ( sscanf( line + 3, "%f %f %f %f %f %f %i %f %f %f %n", &start, &secs, &scale, &f->org[0], &f->org[1], &f->org[2], &have,
				&f->ang[PITCH], &f->ang[YAW], &f->ang[ROLL], &n ) < 10 || n <= 0 || !line[3 + n] )
				continue;
			f->start = RC_SecondsMs( start );
			f->ms = RC_SecondsMs( secs );
			f->scale = scale > 0.0f ? Com_Clamp( 0.1f, 8.0f, scale ) : 1.0f;
			f->haveAngles = (qboolean)( have != 0 );
			Q_strncpyz( f->path, line + 3 + n, sizeof( f->path ) );
			c->numFx++;
		} else if ( !Q_stricmpn( line, "snd ", 4 ) && c->numSnd < CM_MAX_SND ) {
			cmSnd_t *s = &c->snd[c->numSnd];
			float start, secs;
			int spot = 0, n = 0;

			memset( s, 0, sizeof( *s ) );
			if ( sscanf( line + 4, "%f %f %i %f %f %f %n", &start, &secs, &spot, &s->org[0], &s->org[1], &s->org[2], &n ) < 6 || n <= 0 || !line[4 + n] )
				continue;
			s->start = RC_SecondsMs( start );
			s->ms = RC_SecondsMs( secs );
			s->spot = (qboolean)( spot != 0 );
			Q_strncpyz( s->path, line + 4 + n, sizeof( s->path ) );
			c->numSnd++;
		} else if ( !Q_stricmpn( line, "maplook ", 8 ) && c->numMapLooks < CM_MAX_MAPLOOKS ) {
			cmMapLook_t *m = &c->mapLooks[c->numMapLooks];
			float start, secs, fade, fog;
			char layout[MAX_QPATH] = "-";

			memset( m, 0, sizeof( *m ) );
			if ( sscanf( line + 8, "%f %f %f %i %f %f %f %f %f %f %f %f %f %f %63s", &start, &secs, &fade, &m->light, &m->lightColor[0], &m->lightColor[1],
				&m->lightColor[2], &m->skyColor[0], &m->skyColor[1], &m->skyColor[2], &fog, &m->fogColor[0], &m->fogColor[1], &m->fogColor[2], layout ) < 14 )
				continue;
			Q_strncpyz( m->layout, strcmp( layout, "-" ) ? layout : "", sizeof( m->layout ) );
			m->start = RC_SecondsMs( start );
			m->ms = RC_SecondsMs( secs );
			m->fade = RC_SecondsMs( fade );
			m->light = Com_Clampi( 0, 300, m->light );
			m->fogDepth = fog > 0.0f ? Com_Clamp( 64.0f, CM_FOG_NONE, fog ) : 0.0f;
			c->numMapLooks++;
		} else if ( !Q_stricmpn( line, "speed ", 6 ) && c->numSpeeds < CM_MAX_SPEEDS ) {
			cmSpeed_t *sp = &c->speeds[c->numSpeeds];
			float start, secs;
			int speed = 100;

			if ( sscanf( line + 6, "%f %f %i", &start, &secs, &speed ) < 3 )
				continue;
			sp->start = RC_SecondsMs( start );
			sp->ms = Q_max( 100, RC_SecondsMs( secs ) );
			sp->speed = Com_Clampi( 5, 400, speed );
			c->numSpeeds++;
		} else if ( !Q_stricmpn( line, "mark ", 5 ) && c->numMarks < CM_MAX_MARKS ) {
			float at = 0.0f;
			char name[32] = "-";

			if ( sscanf( line + 5, "%f %31s", &at, name ) < 1 )
				continue;
			c->marks[c->numMarks] = RC_SecondsMs( at );
			Q_strncpyz( c->markNames[c->numMarks], strcmp( name, "-" ) ? name : "", sizeof( c->markNames[0] ) );
			c->numMarks++;
		} else if ( !Q_stricmpn( line, "force ", 6 ) && c->numClips < CM_MAX_CLIPS ) {
			cmClip_t *clip = &c->clips[c->numClips];
			float start, seconds;
			int ent, power = 0, target = -1, anims = 1;

			memset( clip, 0, sizeof( *clip ) );
			if ( sscanf( line + 6, "%i %f %f %i %i %i", &ent, &start, &seconds, &power, &target, &anims ) < 4 || ent < 0 || ent >= CM_MAX_ENTS )
				continue;
			clip->kind = CM_CLIP_FORCE;
			clip->ent = ent;
			clip->start = RC_SecondsMs( start );
			clip->a.ms = RC_SecondsMs( seconds );
			clip->power = Com_Clampi( 0, (int)ARRAY_LEN( cmForces ) - 1, power );
			clip->target = target >= 0 && target < CM_MAX_ENTS ? target : -1;
			clip->forceAnims = (qboolean)( anims != 0 );
			c->numClips++;
		} else if ( !Q_stricmpn( line, "music ", 6 ) && c->numSnd < CM_MAX_SND ) {
			cmSnd_t *s = &c->snd[c->numSnd];
			float start, secs;
			int n = 0;

			memset( s, 0, sizeof( *s ) );
			if ( sscanf( line + 6, "%f %f %n", &start, &secs, &n ) < 2 || n <= 0 || !line[6 + n] )
				continue;
			s->start = RC_SecondsMs( start );
			s->ms = RC_SecondsMs( secs );
			s->music = qtrue;
			Q_strncpyz( s->path, line + 6 + n, sizeof( s->path ) );
			c->numSnd++;
		} else if ( !Q_stricmpn( line, "prop ", 5 ) && c->numProps < CM_MAX_PROPS ) {
			cmProp_t *pr = &c->props[c->numProps];
			char keys[CM_PROP_KEYS * 64];
			float start, secs, scale;
			int smooth = 1, n = 0;

			memset( pr, 0, sizeof( *pr ) );
			if ( sscanf( line + 5, "%f %f %i %f %511s %n", &start, &secs, &smooth, &scale, keys, &n ) < 5 || n <= 0 || !line[5 + n] )
				continue;
			pr->start = RC_SecondsMs( start );
			pr->ms = RC_SecondsMs( secs );
			pr->smooth = (qboolean)( smooth != 0 );
			pr->scale = Com_Clamp( 0.05f, 50.0f, scale );
			Q_strncpyz( pr->model, line + 5 + n, sizeof( pr->model ) );
			for ( const char *k = keys; *k && pr->numKeys < CM_PROP_KEYS; ) {
				cmPlace_t *pk = &pr->keys[pr->numKeys];

				if ( sscanf( k, "%i,%f,%f,%f,%f,%f,%f", &pk->ms, &pk->org[0], &pk->org[1], &pk->org[2], &pk->ang[PITCH], &pk->ang[YAW], &pk->ang[ROLL] ) == 7 )
					pr->numKeys++;
				k = strchr( k, ';' );
				if ( !k )
					break;
				k++;
			}
			if ( pr->numKeys )
				c->numProps++;
		} else if ( !Q_stricmpn( line, "light ", 6 ) && c->numLights < CM_MAX_LIGHTS ) {
			cmLight_t *l = &c->lights[c->numLights];
			float start, secs, fade;
			char who[MAX_QPATH];
			int fx = 0;

			memset( l, 0, sizeof( *l ) );
			if ( sscanf( line + 6, "%f %f %f %f %f %f %f %f %f %i %f %63s", &start, &secs, &l->org[0], &l->org[1], &l->org[2], &l->radius,
				&l->color[0], &l->color[1], &l->color[2], &fx, &fade, who ) < 11 )
				continue;
			l->start = RC_SecondsMs( start );
			l->ms = RC_SecondsMs( secs );
			l->radius = Com_Clamp( 8.0f, 2000.0f, l->radius );
			l->fx = Com_Clampi( 0, CM_NUM_LIGHT_FX - 1, fx );
			l->fade = RC_SecondsMs( fade );
			Q_strncpyz( l->who, strcmp( who, "-" ) ? who : "", sizeof( l->who ) );
			c->numLights++;
		} else if ( !Q_stricmpn( line, "screen ", 7 ) && c->numScreens < CM_MAX_SCREENS ) {
			cmScreen_t *sc = &c->screens[c->numScreens];
			float start, secs, fade = 0.0f;
			int kind = 0, strength = 100;

			memset( sc, 0, sizeof( *sc ) );
			if ( sscanf( line + 7, "%f %f %i %f %f %f %i %f", &start, &secs, &kind, &sc->color[0], &sc->color[1], &sc->color[2], &strength, &fade ) < 7 )
				continue;
			sc->start = RC_SecondsMs( start );
			sc->ms = Q_max( 100, RC_SecondsMs( secs ) );
			sc->kind = Com_Clampi( 0, CM_NUM_SCREENS - 1, kind );
			sc->strength = Com_Clampi( 0, 100, strength );
			sc->fade = RC_SecondsMs( fade );
			c->numScreens++;
		} else if ( !Q_stricmpn( line, "txt ", 4 ) && c->numTxt < CM_MAX_TXT ) {
			cmTxt_t *x = &c->txt[c->numTxt];
			float start, secs;
			int kind = 0, size = 1, typed = 0, n = 0;
			const char *who, *bar;

			memset( x, 0, sizeof( *x ) );
			if ( sscanf( line + 4, "%f %f %i %i %i %n", &start, &secs, &kind, &size, &typed, &n ) < 5 || n <= 0 )
				continue;
			who = line + 4 + n;
			if ( !( bar = strchr( who, '|' ) ) || !bar[1] )
				continue;
			x->start = RC_SecondsMs( start );
			x->ms = Q_max( 100, RC_SecondsMs( secs ) );
			x->kind = Com_Clampi( 0, CM_NUM_TXT_KINDS - 1, kind );
			x->size = Com_Clampi( 0, (int)ARRAY_LEN( cmTxtSizes ) - 1, size );
			x->typed = (qboolean)( typed != 0 );
			Q_strncpyz( x->who, who, Q_min( (int)sizeof( x->who ), (int)( bar - who ) + 1 ) );
			Q_strncpyz( x->text, bar + 1, sizeof( x->text ) );
			c->numTxt++;
		} else if ( !Q_stricmpn( line, "ent ", 4 ) && c->numEnts < CM_MAX_ENTS ) {
			// a player's name can have spaces in it
			const char *who = line + 4;

			while ( *who == ' ' )
				who++;
			Q_strncpyz( c->ents[c->numEnts++].who, strcmp( who, "-" ) ? who : "", sizeof( c->ents[0].who ) );
		} else if ( !Q_stricmpn( line, "extra ", 6 ) ) {
			float v[4];
			int ent, saber = 0, color = 0, n = 0;

			if ( sscanf( line + 6, "%i %f %f %f %f %i %i %n", &ent, &v[0], &v[1], &v[2], &v[3], &saber, &color, &n ) < 7 || n <= 0
				|| ent < 0 || ent >= c->numEnts )
				continue;
			VectorCopy( v, c->ents[ent].home );
			c->ents[ent].yaw = v[3];
			c->ents[ent].saber = Com_Clampi( 0, CL_EXTRA_SABER_TWO, saber );
			c->ents[ent].color = Com_Clampi( 0, CL_EXTRA_NUM_COLORS - 1, color );
			Q_strncpyz( c->ents[ent].model, line + 6 + n, sizeof( c->ents[ent].model ) );
		} else if ( !Q_stricmpn( line, "appear ", 7 ) && c->numClips < CM_MAX_CLIPS ) {
			cmClip_t *clip = &c->clips[c->numClips];
			float start, seconds;
			int ent, hidden = 0, size = 100, shell = 0, saber = 0, blade = 100, hand = 0, face = 0;
			char hold[MAX_QPATH] = "-";

			memset( clip, 0, sizeof( *clip ) );
			if ( sscanf( line + 7, "%i %f %f %i %i %i %i %i %i %f %f %f %63s %i", &ent, &start, &seconds, &hidden, &size, &shell, &saber, &blade, &hand,
				&clip->holdAng[PITCH], &clip->holdAng[YAW], &clip->holdAng[ROLL], hold, &face ) < 3 || ent < 0 || ent >= CM_MAX_ENTS )
				continue;
			clip->face = Com_Clampi( 0, (int)ARRAY_LEN( cmFaces ) - 1, face );
			clip->holdHand = hand ? 1 : 0;
			Q_strncpyz( clip->hold, strcmp( hold, "-" ) ? hold : "", sizeof( clip->hold ) );
			clip->kind = CM_CLIP_LOOK;
			clip->ent = ent;
			clip->start = RC_SecondsMs( start );
			clip->a.ms = RC_SecondsMs( seconds );
			clip->hidden = (qboolean)( hidden != 0 );
			clip->size = Com_Clampi( 25, 400, size );
			clip->shell = Com_Clampi( 0, (int)ARRAY_LEN( cmShells ) - 1, shell );
			clip->saber = Com_Clampi( 0, (int)ARRAY_LEN( cmSaberLooks ) - 1, saber );
			clip->blade = Com_Clampi( 25, 300, blade );
			c->numClips++;
		} else if ( !Q_stricmpn( line, "emote ", 6 ) && c->numClips < CM_MAX_CLIPS ) {
			cmClip_t *clip = &c->clips[c->numClips];
			char emote[48];
			float start, seconds;
			int ent, loop = 0, real = 1;

			memset( clip, 0, sizeof( *clip ) );
			if ( sscanf( line + 6, "%i %f %f %47s %i %i", &ent, &start, &seconds, emote, &loop, &real ) < 4 || ent < 0 || ent >= CM_MAX_ENTS )
				continue;
			clip->kind = CM_CLIP_EMOTE;
			clip->ent = ent;
			clip->start = RC_SecondsMs( start );
			Q_strncpyz( clip->emote, strcmp( emote, "-" ) ? emote : "", sizeof( clip->emote ) );
			clip->a.ms = RC_SecondsMs( seconds );
			clip->a.loop = (qboolean)( loop != 0 );
			clip->a.speed = 100;
			clip->a.to = -1;
			clip->realEmote = (qboolean)( real != 0 );
			c->numClips++;
		} else if ( !Q_stricmpn( line, "place ", 6 ) && c->numClips < CM_MAX_CLIPS ) {
			cmClip_t *clip = &c->clips[c->numClips];
			float start;
			int ent, smooth = 1, n = 0;

			memset( clip, 0, sizeof( *clip ) );
			if ( sscanf( line + 6, "%i %f %i %n", &ent, &start, &smooth, &n ) < 3 || n <= 0 || ent < 0 || ent >= CM_MAX_ENTS )
				continue;
			clip->kind = CM_CLIP_PLACE;
			clip->ent = ent;
			clip->start = RC_SecondsMs( start );
			clip->smooth = (qboolean)( smooth != 0 );
			for ( char *k = line + 6 + n; k && *k && clip->numKeys < CM_MAX_PLACE; ) {
				cmPlace_t *key = &clip->keys[clip->numKeys];

				if ( sscanf( k, "%i %f %f %f %f %f %f", &key->ms, &key->org[0], &key->org[1], &key->org[2], &key->ang[0], &key->ang[1], &key->ang[2] ) == 7 )
					clip->numKeys++;
				k = strchr( k, '|' );
				if ( k )
					k++;
			}
			c->numClips++;
		} else if ( !Q_stricmpn( line, "tele ", 5 ) && c->numClips < CM_MAX_CLIPS ) {
			cmClip_t *clip = &c->clips[c->numClips];
			float start;
			int ent;

			memset( clip, 0, sizeof( *clip ) );
			if ( sscanf( line + 5, "%i %f %f %f %f %f", &ent, &start, &clip->keys[0].org[0], &clip->keys[0].org[1], &clip->keys[0].org[2],
				&clip->keys[0].ang[YAW] ) < 6 || ent < 0 || ent >= CM_MAX_ENTS )
				continue;
			clip->kind = CM_CLIP_PLACE;
			clip->tele = qtrue;
			clip->ent = ent;
			clip->start = RC_SecondsMs( start );
			clip->numKeys = 1;
			c->numClips++;
		} else if ( !Q_stricmpn( line, "npccmd ", 7 ) && c->numClips < CM_MAX_CLIPS ) {
			cmClip_t *clip = &c->clips[c->numClips];
			char word[sizeof( clip->cmd )];
			float start;
			int ent, n = 0;

			memset( clip, 0, sizeof( *clip ) );
			if ( sscanf( line + 7, "%i %f %23s %n", &ent, &start, word, &n ) < 3 || ent < 0 || ent >= CM_MAX_ENTS )
				continue;
			for ( char *w = word; *w; w++ ) {
				if ( *w == '_' )
					*w = ' ';
			}
			clip->kind = CM_CLIP_NPCCMD;
			clip->ent = ent;
			clip->start = RC_SecondsMs( start );
			Q_strncpyz( clip->cmd, word, sizeof( clip->cmd ) );
			if ( n > 0 )
				Q_strncpyz( clip->text, line + 7 + n, sizeof( clip->text ) );
			c->numClips++;
		} else if ( !Q_stricmpn( line, "spawn ", 6 ) && c->numClips < CM_MAX_CLIPS ) {
			cmClip_t *clip = &c->clips[c->numClips];
			char team[16], type[MAX_QPATH];
			float start;
			int ent;

			memset( clip, 0, sizeof( *clip ) );
			if ( sscanf( line + 6, "%i %f %f %f %f %f %15s %63s", &ent, &start, &clip->keys[0].org[0], &clip->keys[0].org[1],
				&clip->keys[0].org[2], &clip->keys[0].ang[YAW], team, type ) < 8 || ent < 0 || ent >= CM_MAX_ENTS )
				continue;
			clip->kind = CM_CLIP_SPAWN;
			clip->ent = ent;
			clip->start = RC_SecondsMs( start );
			clip->numKeys = 1;
			Q_strncpyz( clip->team, team, sizeof( clip->team ) );
			Q_strncpyz( clip->npcType, strcmp( type, "-" ) ? type : "", sizeof( clip->npcType ) );
			c->numClips++;
		} else if ( !Q_stricmpn( line, "remove ", 7 ) && c->numClips < CM_MAX_CLIPS ) {
			cmClip_t *clip = &c->clips[c->numClips];
			float start;
			int ent;

			memset( clip, 0, sizeof( *clip ) );
			if ( sscanf( line + 7, "%i %f", &ent, &start ) < 2 || ent < 0 || ent >= CM_MAX_ENTS )
				continue;
			clip->kind = CM_CLIP_REMOVE;
			clip->ent = ent;
			clip->start = RC_SecondsMs( start );
			c->numClips++;
		} else if ( !Q_stricmpn( line, "rec ", 4 ) && c->numClips < CM_MAX_CLIPS ) {
			cmClip_t *clip = &c->clips[c->numClips];
			float start;
			int ent, rec;

			memset( clip, 0, sizeof( *clip ) );
			if ( sscanf( line + 4, "%i %f %i", &ent, &start, &rec ) < 3 || ent < 0 || ent >= CM_MAX_ENTS || rec < 0 || rec >= CM_MAX_RECS )
				continue;
			clip->kind = CM_CLIP_RECORD;
			clip->ent = ent;
			clip->start = RC_SecondsMs( start );
			clip->rec = rec;
			c->numClips++;
		} else if ( !Q_stricmpn( line, "rs ", 3 ) ) {
			cmSample_t sm;
			int r;

			memset( &sm, 0, sizeof( sm ) );
			if ( sscanf( line + 3, "%i %i %f %f %f %f %f %i %i %i %i %i %i", &r, &sm.ms, &sm.org[0], &sm.org[1], &sm.org[2], &sm.pitch, &sm.yaw,
				&sm.legs, &sm.torso, &sm.flags, &sm.holstered, &sm.force, &sm.saberMove ) == 13 && r >= 0 && r < CM_MAX_RECS && c->recs[r].count < CM_MAX_SAMPLES )
				c->recs[r].s[c->recs[r].count++] = sm;
		} else if ( !Q_stricmpn( line, "re ", 3 ) ) {
			cmRecEvent_t e;
			int r;

			if ( sscanf( line + 3, "%i %i %i %i", &r, &e.ms, &e.event, &e.parm ) == 4 && r >= 0 && r < CM_MAX_RECS && c->recs[r].numEvents < CM_MAX_REC_EVENTS )
				c->recs[r].ev[c->recs[r].numEvents++] = e;
		} else if ( !Q_stricmpn( line, "say ", 4 ) && c->numClips < CM_MAX_CLIPS ) {
			cmClip_t *clip = &c->clips[c->numClips];
			float start;
			int ent, n = 0;
			const char *bar;

			memset( clip, 0, sizeof( *clip ) );
			if ( sscanf( line + 4, "%i %f %n", &ent, &start, &n ) < 2 || n <= 0 || ent < 0 || ent >= CM_MAX_ENTS || !( bar = strchr( line + 4 + n, '|' ) ) )
				continue;
			clip->kind = CM_CLIP_SAY;
			clip->ent = ent;
			clip->start = RC_SecondsMs( start );
			Q_strncpyz( clip->name, line + 4 + n, Q_min( (int)sizeof( clip->name ), (int)( bar - ( line + 4 + n ) ) + 1 ) );
			Q_strncpyz( clip->text, bar + 1, sizeof( clip->text ) );
			c->numClips++;
		} else if ( !Q_stricmpn( line, "move ", 5 ) && c->numClips < CM_MAX_CLIPS ) {
			cmClip_t *clip = &c->clips[c->numClips];
			char pace[16];
			float start;
			int ent, n = 0;
			char *p;

			memset( clip, 0, sizeof( *clip ) );
			if ( sscanf( line + 5, "%i %f %15s %n", &ent, &start, pace, &n ) < 3 || n <= 0 || ent < 0 || ent >= CM_MAX_ENTS )
				continue;
			clip->kind = CM_CLIP_MOVE;
			clip->ent = ent;
			clip->start = RC_SecondsMs( start );
			clip->run = (qboolean)!Q_stricmp( pace, "run" );
			for ( p = line + 5 + n; p && *p && clip->numPts < CM_MAX_PTS; ) {
				if ( sscanf( p, "%f %f %f", &clip->pts[clip->numPts][0], &clip->pts[clip->numPts][1], &clip->pts[clip->numPts][2] ) == 3 )
					clip->numPts++;
				p = strchr( p, '|' );
				if ( p )
					p++;
			}
			c->numClips++;
		} else if ( !Q_stricmpn( line, "anim ", 5 ) && c->numClips < CM_MAX_CLIPS ) {
			cmClip_t *clip = &c->clips[c->numClips];
			char anim[48], mode[16] = "play", loop[16] = "once";
			float start, seconds;
			int ent, from = 0, to = -1, speed = 100;

			memset( clip, 0, sizeof( *clip ) );
			if ( sscanf( line + 5, "%i %f %f %47s %i %i %15s %15s %i", &ent, &start, &seconds, anim, &from, &to, mode, loop, &speed ) < 4
				|| ent < 0 || ent >= CM_MAX_ENTS )
				continue;
			clip->kind = CM_CLIP_ANIM;
			clip->ent = ent;
			clip->start = RC_SecondsMs( start );
			Q_strncpyz( clip->a.anim, anim, sizeof( clip->a.anim ) );
			clip->a.from = from;
			clip->a.to = to;
			clip->a.mode = !Q_stricmp( mode, "reverse" ) ? RC_AM_REVERSE : !Q_stricmp( mode, "hold" ) ? RC_AM_HOLD : RC_AM_PLAY;
			clip->a.loop = (qboolean)!Q_stricmp( loop, "loop" );
			clip->a.speed = Com_Clampi( 5, 1000, speed );
			clip->a.ms = RC_SecondsMs( seconds );
			c->numClips++;
		}
	}
	// clips of someone not in it go
	for ( int i = 0; i < c->numClips; ) {
		if ( c->clips[i].ent < c->numEnts ) {
			i++;
			continue;
		}
		c->numClips--;
		memmove( &c->clips[i], &c->clips[i + 1], ( c->numClips - i ) * sizeof( c->clips[0] ) );
	}
}

static qboolean CM_Load( const char *name, cmCine_t *c ) {
	char *buf, *text;
	int len;

	if ( !CM_ValidName( name ) )
		return qfalse;
	len = FS_ReadFile( CM_Path( name ), (void **)&buf );
	if ( len <= 0 || !buf )
		return qfalse;
	text = (char *)Z_Malloc( len + 1, TAG_GENERAL, qfalse );
	memcpy( text, buf, len );
	text[len] = '\0';
	FS_FreeFile( buf );
	CM_Parse( text, c );
	Z_Free( text );
	return qtrue;
}

// a shot from the camera editor before cinematics, as a cinematic of one shot: its walks and
// animations its people's moves and animations
static qboolean CM_ImportShot( const char *name, cmCine_t *c ) {
	char hook[MAX_STRING_CHARS];
	rcWalk_t walks[RC_MAX_WALKS];
	rcAnim_t anims[RC_MAX_ANIMS];
	int numWalks, numAnims;

	CM_Default( c );
	if ( !RC_ReadShot( name, hook, sizeof( hook ), &c->shots[0] ) )
		return qfalse;
	RC_FillFov( &c->shots[0], RC_DEFAULT_FOV );
	if ( !c->shots[0].orbit && c->shots[0].numKeys == 1 )
		c->shots[0].ms = CM_HOLD_MS;
	c->numShots = 1;
	numWalks = RC_ReadWalks( name, walks, RC_MAX_WALKS );
	for ( int i = 0; i < numWalks && c->numClips < CM_MAX_CLIPS; i++ ) {
		cmClip_t *clip = &c->clips[c->numClips];
		vec3_t pts[NW_MAX_ROUTE_POINTS];
		float speed;
		const int ent = CM_FindEnt( c, walks[i].npc, qtrue );
		const int n = CL_NpcWalk_LoadRoute( walks[i].route, pts, NW_MAX_ROUTE_POINTS, &speed );

		if ( ent < 0 || n <= 0 )
			continue;
		memset( clip, 0, sizeof( *clip ) );
		clip->kind = CM_CLIP_MOVE;
		clip->ent = ent;
		clip->start = walks[i].delay;
		clip->run = walks[i].run;
		clip->numPts = Q_min( n, CM_MAX_PTS );
		memcpy( clip->pts, pts, clip->numPts * sizeof( vec3_t ) );
		c->numClips++;
	}
	numAnims = RC_ReadAnims( name, anims, RC_MAX_ANIMS );
	for ( int i = 0; i < numAnims && c->numClips < CM_MAX_CLIPS; i++ ) {
		cmClip_t *clip = &c->clips[c->numClips];
		const int ent = CM_FindEnt( c, anims[i].who, qtrue );

		if ( ent < 0 )
			continue;
		memset( clip, 0, sizeof( *clip ) );
		clip->kind = CM_CLIP_ANIM;
		clip->ent = ent;
		clip->start = anims[i].delay;
		clip->a = anims[i];
		clip->a.who[0] = '\0';
		clip->a.delay = 0;
		c->numClips++;
	}
	return qtrue;
}

/*
===============================================================================

Playing

The cinematic is copied and its moves worked out as it starts, from where everyone is then;
each frame the camera is put on the shot it's at, everyone where they are then.

===============================================================================
*/

static struct {
	qboolean	playing;
	cmCine_t	cine;
	int			start;				// cls.realtime of its beginning
	int			total;
	int			shot;				// the camera's on, -1 none yet
	qboolean	driver;				// this client sent it: it teleports the NPCs on the server
	qboolean	local;				// on this screen only: a preview, or /cinematicmanager play
	qboolean	teleSent[CM_MAX_CLIPS][CM_MAX_GROUP];
	qboolean	said[CM_MAX_CLIPS];
	qboolean	emoted[CM_MAX_CLIPS];
	qboolean	acted[CM_MAX_CLIPS];	// its NPC command, spawn or removal sent, or let go
	qboolean	entSeen[CM_MAX_ENTS];	// here when its moves were worked out: one spawned later, then
	int			seenNext;
	int			fxNext[CM_MAX_FX];		// when each effect plays again; a one-off, 1 once it has
	int			sndNext[CM_MAX_SND];	// the same for each sound
	int			forceNext[CM_MAX_CLIPS];	// and each force power's effect
	int			t;					// how far in, by its own clock: slowed or sped up where it says
	double		tExact;
	int			lastReal;
} cmp;

static sfxHandle_t CM_SndHandle( const char *path ) {
	if ( !path[0] )
		return 0;
	return S_RegisterSound( !Q_stricmpn( path, "sound/", 6 ) ? path : va( "sound/%s", path ) );
}

// how long it sounds, once
static int CM_SndLen( const char *path ) {
	const sfxHandle_t h = CM_SndHandle( path );

	return h ? Q_max( 300, (int)S_GetSampleLengthInMilliSeconds( h ) ) : 1000;
}

static int CM_SndEnd( const cmSnd_t *s ) {
	// a track with no length plays on, shown as a while on the timeline
	if ( s->music )
		return s->start + ( s->ms > 0 ? s->ms : 10000 );
	return s->start + ( s->ms > 0 ? s->ms : CM_SndLen( s->path ) );
}

static void CM_PlaySnd( const cmSnd_t *s, sfxHandle_t h ) {
	if ( s->spot )
		S_StartSound( s->org, ENTITYNUM_WORLD, CHAN_AUTO, h );
	else
		S_StartLocalSound( h, CHAN_AUTO );
}

// t ms in: the one-offs at their moment, the others again each time they end while they last
static qboolean cmMusicPlayed;		// a cinematic's track took over: the map's back after it

static void CM_RunSnd( const cmCine_t *c, int t, int *next ) {
	for ( int i = 0; i < c->numSnd; i++ ) {
		const cmSnd_t *s = &c->snd[i];
		sfxHandle_t h;

		if ( t < s->start )
			continue;
		// a track: started once, from its start however late, and the map's back at its end
		if ( s->music ) {
			if ( !next[i] ) {
				S_StopBackgroundTrack();
				S_StartBackgroundTrack( s->path, s->path, qfalse );
				cmMusicPlayed = qtrue;
				next[i] = 1;
			}
			if ( next[i] == 1 && s->ms > 0 && t >= s->start + s->ms ) {
				CL_SoundManager_RestoreMusic();
				next[i] = 2;
			}
			continue;
		}
		if ( s->ms <= 0 ) {
			// one played from the middle that's long past isn't
			if ( !next[i] && t - s->start < 500 && ( h = CM_SndHandle( s->path ) ) != 0 )
				CM_PlaySnd( s, h );
			next[i] = 1;
			continue;
		}
		if ( t >= s->start + s->ms || cls.realtime < next[i] )
			continue;
		h = CM_SndHandle( s->path );
		if ( !h ) {
			next[i] = cls.realtime + 1000;
			continue;
		}
		CM_PlaySnd( s, h );
		next[i] = cls.realtime + Q_max( 200, (int)S_GetSampleLengthInMilliSeconds( h ) );
	}
}

// a colour code, RPMod's letters too
static qboolean CM_TxtCode( const char *p ) {
	return (qboolean)( p[0] == Q_COLOR_ESCAPE && p[1] && isalnum( (unsigned char)p[1] ) );
}

// the text broken at spaces into lines that fit the width, each starting in the colour the one
// before ended in; how many
static int CM_TxtLines( const char *text, int font, float scale, float w, char lines[][CM_TXT_LEN + CM_TXT_WHO_LEN + 16], int max ) {
	const int size = CM_TXT_LEN + CM_TXT_WHO_LEN + 16;
	char code[3] = "";
	int n = 0;

	while ( *text && n < max ) {
		char *line = lines[n];
		const int start = strlen( code );
		int len = 0, fit = 0;

		Q_strncpyz( line, code, size );
		while ( text[len] && start + len < size - 1 ) {
			line[start + len] = text[len];
			line[start + len + 1] = '\0';
			if ( CL_RpHooks_TextWidth( line, font, scale ) > w )
				break;
			if ( text[len] == ' ' || !text[len + 1] )
				fit = len + 1;
			len++;
		}
		if ( !text[len] )
			fit = len;
		if ( !fit )
			fit = Q_max( len, 1 );
		line[start + fit] = '\0';
		for ( int i = 0; i < fit; i++ ) {
			if ( CM_TxtCode( text + i ) ) {
				code[0] = text[i];
				code[1] = text[i + 1];
			}
		}
		n++;
		text += fit;
		while ( *text == ' ' )
			text++;
	}
	return n;
}

// a line up to so many letters, its colour codes not counted; the letters it had
static int CM_TxtTyped( const char *line, int letters, char *out, int size ) {
	int len = 0, used = 0;

	while ( *line && len < size - 2 ) {
		if ( CM_TxtCode( line ) ) {
			out[len++] = *line++;
			out[len++] = *line++;
			continue;
		}
		if ( used >= letters )
			break;
		out[len++] = *line++;
		used++;
	}
	out[len] = '\0';
	while ( *line ) {
		if ( CM_TxtCode( line ) )
			line += 2;
		else
			line++, used++;
	}
	return used;
}

// the texts showing t ms in: over the bars, in the middle, in the top left; a few of one kind at
// once go one under the other
static void CM_DrawTexts( const cmCine_t *c, int t ) {
	static const float kindScale[CM_NUM_TXT_KINDS] = { 1.0f, 1.0f, 1.5f, 0.85f };
	static const float sizeScale[] = { 0.8f, 1.0f, 1.3f };
	static char lines[CM_TXT_MAX_LINES][CM_TXT_LEN + CM_TXT_WHO_LEN + 16];
	const float barH = CM_BarFrac( c ) * SCREEN_HEIGHT;
	float used[CM_NUM_TXT_KINDS] = { 0 };
	int font = 0;

	for ( int i = 0; i < c->numTxt; i++ ) {
		const cmTxt_t *x = &c->txt[i];
		const int in = t - x->start;
		char full[CM_TXT_LEN + CM_TXT_WHO_LEN + 16], part[sizeof( full )];
		vec4_t color = { 1.0f, 1.0f, 1.0f, 1.0f };
		float scale, rowH, w, left, top, blockH;
		int numLines, letters;

		if ( in < 0 || in >= x->ms || !x->text[0] )
			continue;
		if ( !font )
			font = CL_RpHooks_Font();
		// typed out it comes in a letter at a time, else it fades in; either fades out
		color[3] = Com_Clamp( 0.0f, 1.0f, (float)Q_min( x->typed ? CM_TXT_FADE_MS : in, x->ms - in ) / CM_TXT_FADE_MS );
		if ( x->who[0] )
			Com_sprintf( full, sizeof( full ), "%s%s" S_COLOR_WHITE ": %s", CM_TxtCode( x->who ) ? "" : S_COLOR_YELLOW, x->who, x->text );
		else
			Q_strncpyz( full, x->text, sizeof( full ) );
		scale = kindScale[x->kind] * sizeScale[x->size];
		rowH = re->Font_HeightPixels( font, scale ) + 2.0f;
		w = x->kind == CM_TXT_NOTIFY ? 300.0f : x->kind == CM_TXT_CENTER ? 520.0f : 560.0f;
		numLines = CM_TxtLines( full, font, scale, w, lines, CM_TXT_MAX_LINES );
		blockH = numLines * rowH;
		switch ( x->kind ) {
		case CM_TXT_BOTTOM:
			// the newest lowest, in the middle of the bar
			top = SCREEN_HEIGHT - barH * 0.5f - blockH * 0.5f - used[x->kind];
			break;
		case CM_TXT_TOP:
			top = barH * 0.5f - blockH * 0.5f + used[x->kind];
			break;
		case CM_TXT_CENTER:
			top = SCREEN_HEIGHT * 0.42f - blockH * 0.5f + used[x->kind];
			break;
		default:
			top = barH + 10.0f + used[x->kind];
			break;
		}
		used[x->kind] += blockH + 4.0f;
		// who says it is there at once, what's said typed after
		letters = x->typed ? ( x->who[0] ? CM_TxtTyped( full, 0, part, sizeof( part ) ) - CM_TxtTyped( x->text, 0, part, sizeof( part ) ) : 0 )
			+ in / CM_TXT_TYPE_MS + 1 : 1 << 20;
		for ( int l = 0; l < numLines && letters > 0; l++ ) {
			const float lw = (float)CL_RpHooks_TextWidth( lines[l], font, scale );

			left = x->kind == CM_TXT_NOTIFY ? 16.0f : ( SCREEN_WIDTH - w ) * 0.5f + ( w - lw ) * 0.5f;
			letters -= CM_TxtTyped( lines[l], letters, part, sizeof( part ) );
			CL_RpHooks_DrawText( left, top + l * rowH, part, color, font, scale );
		}
	}
}

// what's over the whole picture t ms in: flashes, tints, dark edges
static void CM_DrawScreens( const cmCine_t *c, int t ) {
	for ( int i = 0; i < c->numScreens; i++ ) {
		const cmScreen_t *sc = &c->screens[i];
		const int in = t - sc->start;
		vec4_t color;
		float a = sc->strength / 100.0f;

		if ( in < 0 || in >= sc->ms || a <= 0.0f )
			continue;
		if ( sc->kind == CM_SCREEN_FLASH ) {
			const float f = 1.0f - (float)in / sc->ms;

			a *= f * f;
		} else if ( sc->fade > 0 ) {
			a *= Com_Clamp( 0.0f, 1.0f, Q_min( (float)in, (float)( sc->ms - in ) ) / sc->fade );
		}
		VectorCopy( sc->color, color );
		if ( sc->kind != CM_SCREEN_VIGNETTE ) {
			color[3] = a;
			re->SetColor( color );
			re->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 0, 0, cls.whiteShader );
			continue;
		}
		// the edges: rings in from them, each fainter, darker where they meet at the corners
		for ( int r = 0; r < 20; r++ ) {
			const float band = 8.0f, inset = r * band, f = 1.0f - r / 20.0f;

			color[3] = a * f * f * 0.85f;
			re->SetColor( color );
			re->DrawStretchPic( inset, inset, SCREEN_WIDTH - inset * 2, band, 0, 0, 0, 0, cls.whiteShader );
			re->DrawStretchPic( inset, SCREEN_HEIGHT - inset - band, SCREEN_WIDTH - inset * 2, band, 0, 0, 0, 0, cls.whiteShader );
			re->DrawStretchPic( inset, inset + band, band, SCREEN_HEIGHT - ( inset + band ) * 2, 0, 0, 0, 0, cls.whiteShader );
			re->DrawStretchPic( SCREEN_WIDTH - inset - band, inset + band, band, SCREEN_HEIGHT - ( inset + band ) * 2, 0, 0, 0, 0, cls.whiteShader );
		}
	}
	re->SetColor( NULL );
}

// while it plays, preview too: what's over the picture, then the texts over the bars and the scene
void CM_DrawPlayingTexts( void ) {
	if ( !cmp.playing )
		return;
	CM_DrawScreens( &cmp.cine, cmp.t );
	CM_DrawTexts( &cmp.cine, cmp.t );
}

static int CM_FxId( const char *path ) {
	if ( !path[0] )
		return 0;
	return FX_RegisterEffect( !Q_stricmpn( path, "effects/", 8 ) ? path : va( "effects/%s", path ) );
}

// how often a looping one goes again: as long as it lasts
static int CM_FxPeriod( int id ) {
	int length = 0, repeatDelay = 0;

	if ( !id || !theFxScheduler.GetEffectTiming( id, &length, &repeatDelay ) || length <= 0 )
		length = 1000;
	return Q_max( 200, length );
}

static void CM_PlayFx( const cmFx_t *f, int id ) {
	vec3_t org, dir;

	VectorCopy( f->org, org );
	if ( f->haveAngles )
		AngleVectors( f->ang, dir, NULL, NULL );
	else
		VectorSet( dir, 0, 0, 1 );
	FX_NextScale( f->scale > 0.0f ? f->scale : 1.0f );
	FX_PlayEffectID( id, org, dir, -1, -1 );
}

static int CM_FxEnd( const cmFx_t *f ) {
	return f->start + ( f->ms > 0 ? f->ms : CM_FX_ONCE_MS );
}

// where someone's hands are, near enough, turned toward a point: in front of their chest
static qboolean CM_HandsToward( const cmCine_t *c, int ent, const vec3_t to, vec3_t org, vec3_t dir ) {
	vec3_t flat;
	int num;

	if ( ent < 0 || ent >= c->numEnts || !CM_Resolve( c->ents[ent].who, &num, 1 ) || !RC_EntityOrigin( num, org ) )
		return qfalse;
	org[2] += 14.0f;
	VectorSubtract( to, org, flat );
	flat[2] = 0.0f;
	if ( VectorNormalize( flat ) > 1.0f )
		VectorMA( org, 14.0f, flat, org );
	VectorSubtract( to, org, dir );
	if ( VectorNormalize( dir ) < 1.0f )
		VectorCopy( flat, dir );
	return qtrue;
}

// the force powers going on t ms in: their effects again each while
static void CM_RunForce( const cmCine_t *c, int t, int *next ) {
	for ( int i = 0; i < c->numClips; i++ ) {
		const cmClip_t *clip = &c->clips[i];
		const cmForce_t *f;
		const int len = clip->kind == CM_CLIP_FORCE ? CM_AnimLen( c, clip ) : 0;
		vec3_t org, dir, at;
		int id, num;

		if ( clip->kind != CM_CLIP_FORCE || t < clip->start || ( len > 0 && t >= clip->start + len ) || cls.realtime < next[i] )
			continue;
		f = &cmForces[clip->power];
		next[i] = cls.realtime + f->every;
		if ( !( id = CM_FxId( f->effect ) ) )
			continue;
		// the one it's at, over their middle; with no one, in front of the one using it
		if ( clip->target >= 0 && clip->target < c->numEnts && CM_Resolve( c->ents[clip->target].who, &num, 1 ) && RC_EntityOrigin( num, at ) ) {
			at[2] += 12.0f;
		} else {
			vec3_t ang = { 0, 0, 0 }, fwd;

			if ( clip->ent >= c->numEnts || !RC_TrackTarget( c->ents[clip->ent].who, at, &ang[YAW] ) )
				continue;
			AngleVectors( ang, fwd, NULL, NULL );
			VectorMA( at, 128.0f, fwd, at );
		}
		if ( f->atTarget ) {
			VectorSet( dir, 0, 0, 1 );
			FX_PlayEffectID( id, at, dir, -1, -1 );
		} else if ( CM_HandsToward( c, clip->ent, at, org, dir ) ) {
			if ( f->onSelf ) {
				org[2] -= 14.0f;
				VectorSet( dir, 0, 0, 1 );
			}
			FX_PlayEffectID( id, org, dir, -1, -1 );
		}
	}
}

// t ms in: the one-offs at their moment, the others again and again while they last
static void CM_RunFx( const cmCine_t *c, int t, int *next ) {
	for ( int i = 0; i < c->numFx; i++ ) {
		const cmFx_t *f = &c->fx[i];
		int id;

		if ( t < f->start )
			continue;
		if ( f->ms <= 0 ) {
			// one played from the middle that's long past isn't
			if ( !next[i] && t - f->start < 500 && ( id = CM_FxId( f->path ) ) != 0 )
				CM_PlayFx( f, id );
			next[i] = 1;
			continue;
		}
		if ( t >= f->start + f->ms || cls.realtime < next[i] )
			continue;
		id = CM_FxId( f->path );
		if ( !id ) {
			next[i] = cls.realtime + 1000;
			continue;
		}
		CM_PlayFx( f, id );
		next[i] = cls.realtime + CM_FxPeriod( id );
	}
}

static cmPlan_t cmPlayPlans[CM_MAX_CLIPS];

static void CM_Queue( const char *cmd );
static qboolean cm_EditorOpenForPlay( void );
static void cm_EditorPlayStopped( void );

// whoever sent it brings the NPCs along on the server: each where its move ends, once it's
// there; now, every NPC under way where it is
static void CM_DriverTeles( int t, qboolean now ) {
	for ( int i = 0; i < cmp.cine.numClips; i++ ) {
		const cmClip_t *clip = &cmp.cine.clips[i];
		const cmPlan_t *p = &cmPlayPlans[i];

		if ( !CM_Moves( clip->kind ) || t <= clip->start )
			continue;
		for ( int m = 0; m < p->count; m++ ) {
			const int ms = CM_PlanLen( &cmp.cine, p, clip, m );
			vec3_t pos, ang;

			if ( p->nums[m] < MAX_CLIENTS || CL_Extras_IsNum( p->nums[m] ) || cmp.teleSent[i][m] || ( !now && t < clip->start + ms ) )
				continue;
			CM_PlanAt( &cmp.cine, p, clip, m, Q_min( t - clip->start, ms ), pos, ang );
			CM_Queue( va( "npc tele %i %i %i %i %i", RC_Round( pos[0] ), RC_Round( pos[1] ), RC_Round( pos[2] ),
				RC_Round( AngleNormalize360( ang[YAW] ) ), p->nums[m] ) );
			cmp.teleSent[i][m] = qtrue;
		}
	}
}

// the player's name, colours left out
static const char *CM_PlayerName( int num ) {
	static char name[MAX_QPATH];
	const int ofs = num >= 0 && num < MAX_CLIENTS ? cl.gameState.stringOffsets[CS_PLAYERS + num] : 0;

	CM_StripName( ofs ? Info_ValueForKey( cl.gameState.stringData + ofs, "n" ) : "", name, sizeof( name ) );
	return name;
}

// a line said as the Storyteller says it: your own in chat, another player's or an NPC's shown as
// theirs for everyone with this client (an rpnotify hook, see cl_rphooks.cpp); NULL if there's
// nobody to say it
static const char *CM_SayCommand( const cmCine_t *c, const cmClip_t *clip, qboolean preview ) {
	int nums[CM_MAX_GROUP];
	char text[CM_SAY_LEN];

	if ( !clip->text[0] || clip->ent >= c->numEnts || !CM_Resolve( c->ents[clip->ent].who, nums, CM_MAX_GROUP ) )
		return NULL;
	// quotes would cut the line short
	Q_strncpyz( text, clip->text, sizeof( text ) );
	for ( char *t = text; *t; t++ ) {
		if ( *t == '"' )
			*t = '\'';
	}
	if ( nums[0] == clc.clientNum && !preview )
		return va( "say %s", text );
	if ( nums[0] < MAX_CLIENTS )
		return CL_RpHooks_NpcTalkCommand( va( "%i", nums[0] ), CM_PlayerName( nums[0] ), "^7", "", text );
	return CL_RpHooks_NpcTalkCommand( c->ents[clip->ent].who, clip->name[0] ? clip->name : CM_IsExtra( c->ents[clip->ent].who ) ? c->ents[clip->ent].who + 1
		: c->ents[clip->ent].who, "^7", "", text );
}

// a hook shown on this screen alone, as if the server had sent it: a preview's lines
static void CM_ShowHook( const char *cmd ) {
	qboolean replaced;

	if ( Q_stricmpn( cmd, "rpnotify -1 1 ", 14 ) )
		return;
	Cmd_TokenizeString( va( "print \"%s\n\"", cmd + 14 ) );
	CL_RpHooks_ServerCommand( &replaced );
}

// the lines whose time has come: said by whoever sent it, shown on a preview's screen
static void CM_RunSays( int t ) {
	for ( int i = 0; i < cmp.cine.numClips; i++ ) {
		const cmClip_t *clip = &cmp.cine.clips[i];
		const char *cmd;

		if ( clip->kind != CM_CLIP_SAY || cmp.said[i] || t < clip->start )
			continue;
		cmp.said[i] = qtrue;
		if ( t - clip->start > CM_SAY_LATE_MS || !( cmd = CM_SayCommand( &cmp.cine, clip, cmp.local ) ) )
			continue;
		if ( cmp.local )
			CM_ShowHook( cmd );
		else if ( cmp.driver )
			CM_Queue( cmd );
	}
}

// the emotes done for real when their time comes, by whoever sent it: an NPC's with npc emote,
// their own by its sender; other players' are only seen
static void CM_RunEmotes( int t ) {
	for ( int i = 0; i < cmp.cine.numClips; i++ ) {
		const cmClip_t *clip = &cmp.cine.clips[i];
		int nums[CM_MAX_GROUP];

		if ( clip->kind != CM_CLIP_EMOTE || cmp.emoted[i] || t < clip->start )
			continue;
		cmp.emoted[i] = qtrue;
		if ( !cmp.driver || !clip->realEmote || !clip->emote[0] || t - clip->start > CM_SAY_LATE_MS
			|| !CM_Resolve( cmp.cine.ents[clip->ent].who, nums, CM_MAX_GROUP ) )
			continue;
		if ( nums[0] >= MAX_CLIENTS )
			CM_Queue( va( "npc emote %s %s", clip->emote, cmp.cine.ents[clip->ent].who ) );
		else if ( nums[0] == clc.clientNum )
			CM_Queue( clip->emote );
	}
}

// an NPC command, a spawn or a removal as the server takes it; NULL if it has nothing to go on
static const char *CM_ActCommand( const cmCine_t *c, const cmClip_t *clip ) {
	const char *who = c->ents[clip->ent].who;

	// an extra is this client's alone: nothing goes to the server
	if ( !who[0] || strchr( who, ',' ) || CM_IsExtra( who ) )
		return NULL;
	if ( clip->kind == CM_CLIP_REMOVE )
		return va( "npc kill %s", who );
	if ( clip->kind == CM_CLIP_SPAWN ) {
		if ( !clip->npcType[0] || Q_isanumber( who ) )
			return NULL;
		return CL_NpcManager_SpawnCommand( clip->npcType, qfalse, who, clip->team[0] ? clip->team : "default", "", "", clip->keys[0].org,
			RC_Round( AngleNormalize360( clip->keys[0].ang[YAW] ) ) );
	}
	if ( clip->kind != CM_CLIP_NPCCMD )
		return NULL;
	if ( !Q_stricmp( clip->cmd, "freeze" ) )			return va( "npc freeze %s 1", who );
	if ( !Q_stricmp( clip->cmd, "unfreeze" ) )			return va( "npc freeze %s 0", who );
	if ( !Q_stricmp( clip->cmd, "follow me" ) )			return va( "npc follow %i %s", clc.clientNum, who );
	if ( !Q_stricmp( clip->cmd, "stop following" ) )	return va( "npc follow none %s", who );
	if ( !Q_stricmp( clip->cmd, "dialog" ) )			return clip->text[0] ? va( "npc dialog %s %s", clip->text, who ) : NULL;
	if ( !Q_stricmp( clip->cmd, "no dialog" ) )			return va( "npc dialog none %s", who );
	if ( !Q_stricmp( clip->cmd, "hologram" ) )			return va( "npc hologram normal %s", who );
	if ( !Q_stricmp( clip->cmd, "projector" ) )			return va( "npc hologram projector %s", who );
	if ( !Q_stricmp( clip->cmd, "no hologram" ) )		return va( "npc hologram off %s", who );
	if ( !Q_stricmp( clip->cmd, "kill" ) )				return va( "npc kill %s", who );
	return NULL;
}

// the NPC commands, spawns and removals whose time has come: sent by whoever sent it, one played
// from the middle that's long past not
static void CM_RunActs( int t ) {
	for ( int i = 0; i < cmp.cine.numClips; i++ ) {
		const cmClip_t *clip = &cmp.cine.clips[i];
		const char *cmd;

		if ( !CM_Instant( clip->kind ) || cmp.acted[i] || t < clip->start )
			continue;
		cmp.acted[i] = qtrue;
		if ( !cmp.driver || t - clip->start > CM_SAY_LATE_MS || !( cmd = CM_ActCommand( &cmp.cine, clip ) ) )
			continue;
		CM_Queue( cmd );
	}
}

// someone spawned since it began: their moves worked out now they're here, on every screen
static void CM_PlanSpawned( void ) {
	if ( cls.realtime < cmp.seenNext )
		return;
	cmp.seenNext = cls.realtime + 250;
	for ( int e = 0; e < cmp.cine.numEnts; e++ ) {
		int nums[CM_MAX_GROUP];

		if ( cmp.entSeen[e] || !CM_Resolve( cmp.cine.ents[e].who, nums, CM_MAX_GROUP ) )
			continue;
		cmp.entSeen[e] = qtrue;
		CM_PlanEnt( &cmp.cine, cmPlayPlans, e );
	}
}

static void CM_VideoDone( void );

static void CM_StopPlaying( void ) {
	if ( !cmp.playing )
		return;
	cmp.playing = qfalse;
	CM_VideoDone();
	CM_WorldVolume( NULL );
	if ( cmMusicPlayed ) {
		cmMusicPlayed = qfalse;
		CL_SoundManager_RestoreMusic();
	}
	if ( cmp.driver )
		CM_DriverTeles( cmp.t, qtrue );
	CM_Unapply();
	if ( CL_RpCinematic_Active() )
		CL_RpCinematic_Stop();
}

static void CM_PlayerFrame( void ) {
	int t, local, shot;

	if ( !cmp.playing )
		return;
	// its own clock on, as fast as the stretch it's in plays
	{
		float speed = 1.0f;
		int latest = -1;

		for ( int i = 0; i < cmp.cine.numSpeeds; i++ ) {
			const cmSpeed_t *sp = &cmp.cine.speeds[i];

			if ( cmp.t >= sp->start && cmp.t < sp->start + sp->ms && sp->start >= latest ) {
				latest = sp->start;
				speed = sp->speed / 100.0f;
			}
		}
		cmp.tExact += ( cls.realtime - cmp.lastReal ) * speed;
		cmp.lastReal = cls.realtime;
		cmp.t = (int)cmp.tExact;
	}
	t = cmp.t;
	// over, or left with /cincam stop
	if ( cls.state != CA_ACTIVE || t >= cmp.total || ( cmp.shot >= 0 && !CL_RpCinematic_Active() ) ) {
		CM_StopPlaying();
		return;
	}
	shot = CM_ShotAt( &cmp.cine, t, &local );
	if ( shot != cmp.shot && shot >= 0 ) {
		RC_CineShot( &cmp.cine.shots[shot], local );
		cmp.shot = shot;
	} else if ( shot >= 0 ) {
		// the camera where the cinematic's own clock has it
		RC_SetShotTime( local );
	}
	CM_Apply( &cmp.cine, cmPlayPlans, t );
	CM_RunFx( &cmp.cine, t, cmp.fxNext );
	CM_RunSnd( &cmp.cine, t, cmp.sndNext );
	CM_RunForce( &cmp.cine, t, cmp.forceNext );
	if ( cmp.driver )
		CM_DriverTeles( t, qfalse );
	CM_RunSays( t );
	CM_RunEmotes( t );
	CM_RunActs( t );
	CM_PlanSpawned();
}

// from fromMs in, on this screen; local, it's on this screen alone
static qboolean CM_Play( const cmCine_t *c, int fromMs, qboolean local ) {
	if ( !c->numShots || CM_Total( c ) <= 0 )
		return qfalse;
	CM_StopPlaying();
	cmp.cine = *c;
	cmp.total = CM_Total( c );
	cmp.start = cls.realtime - Com_Clampi( 0, cmp.total - 1, fromMs );
	cmp.t = Com_Clampi( 0, cmp.total - 1, fromMs );
	cmp.tExact = cmp.t;
	cmp.lastReal = cls.realtime;
	cmp.shot = -1;
	cmp.driver = qfalse;
	cmp.local = local;
	// its extras there before anything is worked out from where everyone is
	cmp.playing = qtrue;
	CM_SyncExtras();
	memset( cmp.teleSent, 0, sizeof( cmp.teleSent ) );
	memset( cmp.said, 0, sizeof( cmp.said ) );
	memset( cmp.emoted, 0, sizeof( cmp.emoted ) );
	memset( cmp.acted, 0, sizeof( cmp.acted ) );
	cmp.seenNext = 0;
	for ( int e = 0; e < c->numEnts; e++ ) {
		int nums[CM_MAX_GROUP];

		cmp.entSeen[e] = (qboolean)( CM_Resolve( c->ents[e].who, nums, CM_MAX_GROUP ) > 0 );
	}
	memset( cmp.fxNext, 0, sizeof( cmp.fxNext ) );
	memset( cmp.sndNext, 0, sizeof( cmp.sndNext ) );
	memset( cmp.forceNext, 0, sizeof( cmp.forceNext ) );
	CM_Plan( &cmp.cine, cmPlayPlans );
	cmp.playing = qtrue;
	RC_CineBegin( c->hud, c->bars );
	RC_SetBarFrac( CM_BarFrac( c ) );
	CM_WorldVolume( c );
	CM_PlayerFrame();
	return qtrue;
}

/*
===============================================================================

Sending

Played for everyone, the cinematic goes out as its file's text in rpnotify hooks,
  cmd:cine/<id>/<part>/<parts>: <text>
a part a second for the server's flood protection; everyone with this client plays it once the
last part is in, its sender too, so they start together. In the text a space is +, a line's end
*, anything else RPMod or the server could take another way ~ and its hex, and spaces are put in
for RPMod, which chokes on long words, and taken out again. cmd:cine/stop ends it.

===============================================================================
*/

static struct {
	char		cmds[CM_MAX_OUT][MAX_STRING_CHARS];	// going out, a command at a time
	int			count;
	int			seq;
	char		sentId[32];
} cms;

// what arrives, a cinematic at a time
static struct {
	char		id[32];
	int			parts, got;
	qboolean	have[CM_MAX_PARTS];
	char		data[CM_MAX_PARTS][CM_PART_LEN + CM_PART_LEN / CM_WORD + 8];
} cmr;

static void CM_Queue( const char *cmd ) {
	if ( cms.count >= CM_MAX_OUT ) {
		Com_Printf( S_COLOR_YELLOW "Cinematic: too much to send at once, \"%s\" left out\n", cmd );
		return;
	}
	Q_strncpyz( cms.cmds[cms.count++], cmd, sizeof( cms.cmds[0] ) );
}

// a command a second, as the server takes them
static void CM_OutFrame( void ) {
	if ( !cms.count || cls.state != CA_ACTIVE || !CL_NpcWalk_CanSend() )
		return;
	CL_AddReliableCommand( cms.cmds[0], qfalse );
	cms.count--;
	memmove( cms.cmds[0], cms.cmds[1], cms.count * sizeof( cms.cmds[0] ) );
}

// what's left of a cinematic being sent; 0 when none
static int CM_PartsLeft( void ) {
	const char *prefix = va( "rpnotify -1 1 cmd:cine/%s/", cms.sentId );
	int left = 0;

	for ( int i = 0; i < cms.count && cms.sentId[0]; i++ ) {
		if ( !Q_stricmpn( cms.cmds[i], prefix, strlen( prefix ) ) )
			left++;
	}
	return left;
}

static int CM_Encode( const char *in, char *out, int size ) {
	int len = 0;

	for ( ; *in && len < size - 4; in++ ) {
		const unsigned char ch = (unsigned char)*in;

		if ( isalnum( ch ) || strchr( ".,_|-", ch ) )
			out[len++] = ch;
		else if ( ch == ' ' )
			out[len++] = '+';
		else if ( ch == '\n' )
			out[len++] = '*';
		else
			len += Com_sprintf( out + len, size - len, "~%02X", ch );
	}
	out[len] = '\0';
	return len;
}

// the spaces put in left out
static void CM_Decode( const char *in, char *out, int size ) {
	int len = 0;

	for ( ; *in && len < size - 1; in++ ) {
		if ( *in == '+' ) {
			out[len++] = ' ';
		} else if ( *in == '*' ) {
			out[len++] = '\n';
		} else if ( *in == '~' && isxdigit( (unsigned char)in[1] ) && isxdigit( (unsigned char)in[2] ) ) {
			const char hex[3] = { in[1], in[2], '\0' };

			out[len++] = (char)strtol( hex, NULL, 16 );
			in += 2;
		} else {
			out[len++] = *in;
		}
	}
	out[len] = '\0';
}

// how many parts it goes in, the text encoded into enc; 0 if too long
static int CM_EncodeAll( const cmCine_t *c, char *enc, int size, int *encLen ) {
	char *text = (char *)Z_Malloc( CM_TEXT_MAX, TAG_GENERAL, qfalse );
	int parts;

	CM_ToText( c, text, CM_TEXT_MAX );
	*encLen = CM_Encode( text, enc, size );
	Z_Free( text );
	parts = ( *encLen + CM_PART_LEN - 1 ) / CM_PART_LEN;
	return parts <= CM_MAX_PARTS ? parts : 0;
}

// played for everyone: sent, a part a second
static qboolean CM_Send( const cmCine_t *c ) {
	const int size = CM_TEXT_MAX * 3;
	char *enc;
	int len, parts;

	if ( cls.state != CA_ACTIVE )
		return qfalse;
	if ( !CL_RpHooks_Experimental() ) {
		Com_Printf( S_COLOR_YELLOW "Cinematic not sent: " RP_EXPERIMENTAL_OFF " in the storyteller or NPC manager\n" );
		return qfalse;
	}
	if ( !c->numShots ) {
		Com_Printf( S_COLOR_YELLOW "Cinematic not sent: it has no shots\n" );
		return qfalse;
	}
	enc = (char *)Z_Malloc( size, TAG_GENERAL, qfalse );
	parts = CM_EncodeAll( c, enc, size, &len );
	if ( !parts || cms.count + parts + 1 > CM_MAX_OUT ) {
		Com_Printf( S_COLOR_YELLOW "Cinematic not sent: %s\n", !parts ? "too big, take some spots or moves out" : "still sending the last one" );
		Z_Free( enc );
		return qfalse;
	}
	Com_sprintf( cms.sentId, sizeof( cms.sentId ), "%i-%i", clc.clientNum, ++cms.seq );
	for ( int i = 0; i < parts; i++ ) {
		char part[CM_PART_LEN + CM_PART_LEN / CM_WORD + 8];
		const char *from = enc + i * CM_PART_LEN;
		const int n = Q_min( CM_PART_LEN, len - i * CM_PART_LEN );
		int out = 0;

		for ( int j = 0; j < n; j++ ) {
			if ( j && !( j % CM_WORD ) )
				part[out++] = ' ';
			part[out++] = from[j];
		}
		part[out] = '\0';
		CM_Queue( va( "rpnotify -1 1 cmd:cine/%s/%i/%i: %s", cms.sentId, i + 1, parts, part ) );
	}
	Z_Free( enc );
	Com_Printf( S_COLOR_GREEN "Cinematic: " S_COLOR_WHITE "sending it in %i part%s, it plays for everyone once they're in\n", parts, parts == 1 ? "" : "s" );
	return qtrue;
}

// ended for everyone, the rest of it unsent
static void CM_SendStop( void ) {
	const char *prefix = va( "rpnotify -1 1 cmd:cine/%s/", cms.sentId );

	for ( int i = 0; i < cms.count; ) {
		if ( cms.sentId[0] && !Q_stricmpn( cms.cmds[i], prefix, strlen( prefix ) ) ) {
			cms.count--;
			memmove( cms.cmds[i], cms.cmds[i + 1], ( cms.count - i ) * sizeof( cms.cmds[0] ) );
		} else {
			i++;
		}
	}
	CM_Queue( "rpnotify -1 1 cmd:cine/stop" );
}

static void CM_Received( void ) {
	const int size = CM_MAX_PARTS * ( CM_PART_LEN + 8 );
	char *joined = (char *)Z_Malloc( size, TAG_GENERAL, qfalse ), *text = (char *)Z_Malloc( CM_TEXT_MAX, TAG_GENERAL, qfalse );
	cmCine_t *c = (cmCine_t *)Z_Malloc( sizeof( cmCine_t ), TAG_GENERAL, qtrue );
	const qboolean mine = (qboolean)( cms.sentId[0] && !Q_stricmp( cmr.id, cms.sentId ) );
	int len = 0;

	for ( int i = 0; i < cmr.parts; i++ ) {
		for ( const char *p = cmr.data[i]; *p && len < size - 1; p++ ) {
			if ( *p != ' ' )
				joined[len++] = *p;
		}
	}
	joined[len] = '\0';
	CM_Decode( joined, text, CM_TEXT_MAX );
	CM_Parse( text, c );
	// its sender plays it whatever, the NPCs are theirs to bring along
	if ( mine || RC_CinematicsOn() ) {
		if ( CM_Play( c, 0, qfalse ) ) {
			cmp.driver = mine;
			// the manager's open: it's shown, as a preview is
			if ( cm_EditorOpenForPlay() )
				Com_DPrintf( "Cinematic: shown over the manager\n" );
		}
	}
	Z_Free( c );
	Z_Free( text );
	Z_Free( joined );
}

// cmd:cine/<id>/<part>/<parts>: <text>, cmd:cine/stop
void CL_Cinematic_Hook( char **argv, int argc, const char *message ) {
	int n, parts;

	if ( cls.state != CA_ACTIVE )
		return;
	if ( argc >= 1 && !Q_stricmp( argv[0], "stop" ) ) {
		cmr.id[0] = '\0';
		if ( cmp.playing ) {
			CM_StopPlaying();
			cm_EditorPlayStopped();
		}
		return;
	}
	if ( argc < 3 )
		return;
	n = atoi( argv[1] );
	parts = atoi( argv[2] );
	if ( n < 1 || parts < 1 || parts > CM_MAX_PARTS || n > parts || strlen( argv[0] ) >= sizeof( cmr.id ) )
		return;
	if ( Q_stricmp( argv[0], cmr.id ) || cmr.parts != parts ) {
		memset( &cmr, 0, sizeof( cmr ) );
		Q_strncpyz( cmr.id, argv[0], sizeof( cmr.id ) );
		cmr.parts = parts;
	}
	if ( !cmr.have[n - 1] ) {
		Q_strncpyz( cmr.data[n - 1], message, sizeof( cmr.data[0] ) );
		cmr.have[n - 1] = qtrue;
		cmr.got++;
	}
	if ( cmr.got == cmr.parts ) {
		CM_Received();
		cmr.id[0] = '\0';
	}
}

// how long a saved cinematic takes from when it's sent to its end; -1 if there's none
int CL_Cinematic_PlayMs( const char *name ) {
	cmCine_t *c = (cmCine_t *)Z_Malloc( sizeof( cmCine_t ), TAG_GENERAL, qtrue );
	char *enc = (char *)Z_Malloc( CM_TEXT_MAX * 3, TAG_GENERAL, qfalse );
	int ms = -1, len;

	if ( CM_Load( name, c ) && c->numShots ) {
		const int parts = CM_EncodeAll( c, enc, CM_TEXT_MAX * 3, &len );

		ms = parts * CM_SEND_GAP_MS + CM_Total( c ) + 500;
	}
	Z_Free( enc );
	Z_Free( c );
	return ms;
}

/*
===============================================================================

The manager

A free camera over the map: right mouse held looks and flies, the panel on the right has the
cinematic, the timeline under the view its time.

===============================================================================
*/

#define CM_TOGGLE_CMD		"cinematicmanager"
#define CM_FLY_SPEED		400.0f
#define CM_PANEL_X			446.0f
#define CM_PANEL_W			186.0f
#define CM_ROW_H			12.0f
#define CM_CTRL_H			15.0f
#define CM_GAP				3.0f
#define CM_TEXT_SCALE		0.7f
#define CM_KEY_ROWS			6
#define CM_ENT_ROWS			4
#define CM_PICK_ROWS		4
#define CM_MAX_FILES		256
#define CM_MESSAGE_MS		4000
#define CM_FRUSTUM			20.0f		// how far out a camera's frame is drawn
#define CM_TRACE_DIST		8192.0f
#define CM_MAX_LINES		1200
#define CM_INFO_Y			( RC_BAR_FRAC * SCREEN_HEIGHT + 4 )
#define CM_PERSON_NEAR		96.0f		// the crosshair this near someone picks them

typedef enum {
	CM_TAB_CAMERA,
	CM_TAB_ENTITIES,
	CM_TAB_EFFECTS,
	CM_TAB_SOUNDS,
	CM_TAB_TEXT,
	CM_TAB_WORLD		// props and lights
} cmTab_t;

typedef enum {
	CM_PAGE_SPOTS,
	CM_PAGE_FOCUS,
	CM_PAGE_LOOK,
	CM_PAGE_TRACK		// who it keeps in frame, and its shaking
} cmPage_t;

typedef enum {
	CM_TYPE_NONE,
	CM_TYPE_NAME,
	CM_TYPE_WHO,
	CM_TYPE_ANIM,
	CM_TYPE_FOCUS,
	CM_TYPE_SAY,
	CM_TYPE_SAYNAME,
	CM_TYPE_EMOTE,
	CM_TYPE_NEWNPC,		// a targetname for an NPC to spawn
	CM_TYPE_NPCTYPE,	// a spawn's NPC type
	CM_TYPE_DIALOG,		// an NPC command's dialog
	CM_TYPE_TXT,		// a text's words
	CM_TYPE_TXTWHO,		// and who says them
	CM_TYPE_TRACK,		// who a shot keeps in frame
	CM_TYPE_EXTRANAME,	// an extra's name, + first
	CM_TYPE_EXTRAMODEL,	// and its model
	CM_TYPE_PROPMODEL,	// a prop's model, and the search for one
	CM_TYPE_LIGHTWHO,	// who a light follows
	CM_TYPE_HOLD		// the model a look has someone hold
} cmTyping_t;

typedef enum {
	CM_DRAG_NONE,
	CM_DRAG_HEAD,		// the moment
	CM_DRAG_KEY,		// a spot of a shot
	CM_DRAG_SHOT_END,	// how long a held spot or an orbit lasts
	CM_DRAG_CLIP,
	CM_DRAG_CLIP_END,
	CM_DRAG_FOCUS,		// a key of a moving focus
	CM_DRAG_FX,			// an effect, when it starts
	CM_DRAG_FX_END,		// how long it goes on
	CM_DRAG_SND,		// a sound, when it starts
	CM_DRAG_SND_END,	// how long it goes on
	CM_DRAG_TXT,		// a text, when it shows
	CM_DRAG_TXT_END,	// how long it shows
	CM_DRAG_PROP,		// a prop, when it comes
	CM_DRAG_PROP_END,	// how long it stays
	CM_DRAG_LIGHT,		// a light, when it comes on
	CM_DRAG_LIGHT_END,	// how long it stays on
	CM_DRAG_SCREEN,		// a flash, tint or dark edges, when
	CM_DRAG_SCREEN_END,	// and how long
	CM_DRAG_MAPLOOK,	// the map's look, when
	CM_DRAG_MAPLOOK_END,	// and how long
	CM_DRAG_SPEED,		// a stretch slowed or sped up, when
	CM_DRAG_SPEED_END	// and how long
} cmDrag_t;

static struct {
	qboolean	open;
	int			font;
	qboolean	fromStoryteller;

	cmCine_t	cine;
	char		name[MAX_QPATH];
	qboolean	dirty;

	int			tab;				// cmTab_t
	int			page;				// cmPage_t, of the Camera tab
	int			cur;				// the shot shown on the Camera tab
	int			sel;				// its spot, -1 none
	int			keyScroll;
	int			ent;				// who's picked on the Entities tab, in it; -1 not
	int			pickNum;			// who's picked: their entity number, -1 none
	char		pickWho[MAX_QPATH];	// how they go in it: a player's name, an NPC's targetname or number
	int			clip;				// what they do that's picked, -1 none
	int			spot;				// the move's spot picked, -1 none
	qboolean	spotDrag;			// held and dragged over the floor

	// a moving focus: its key picked, its circle in the view held and dragged at a moment of the shot
	int			focusSel;
	qboolean	focusDrag;
	int			focusDragMs;
	qboolean	focusShown;			// the circle's drawn, here on the screen
	float		focusX, focusY, focusRX, focusRY;

	// the Effects tab: the one picked, held and dragged over what's under the mouse; played
	// where it is while it's looked at
	int			fx;
	int			fxScroll;
	qboolean	fxDrag;
	int			fxPreviewNext;
	// picked in the effect manager: the editor closed meanwhile, back when it's put down
	qboolean	fxPicking;
	int			fxPickIndex;		// the one it replaces, -1 a new one
	int			fxPickStart;
	qboolean	fxPicked;
	cmFx_t		fxPickedFx;
	qboolean	keepCam;			// opened again where the camera was
	qboolean	fxPickPinned;

	// the Sounds tab: the one picked, a spot's held and dragged as an effect is; picked in the
	// sound manager, the editor closed meanwhile
	int			snd;
	int			sndScroll;
	qboolean	sndDrag;
	qboolean	sndPicking;
	int			sndPickIndex;		// the one it replaces, -1 a new one
	int			sndPickStart;
	qboolean	sndPickPinned;
	qboolean	sndPicked;
	cmSnd_t		sndPickedSnd;

	// the Text tab: the one picked
	int			txt;
	int			txtScroll;

	// the Track page: the jolt picked
	int			shakeSel;

	// the World tab: its page, the prop and its key picked, the light picked; one held in the view
	int			worldPage;			// 0 props, 1 lights
	int			prop, propKey, propScroll, propPickScroll;
	int			light, lightScroll;
	int			screen, screenScroll;
	int			mapLook, mapLookScroll;
	int			speed, speedScroll;

	// bars picked together with Ctrl+click, moved together: their kind, which, and where they
	// started when the drag began
	int			numMulti;
	int			multiKind[64], multiIndex[64], multiBase[64];
	qboolean	worldDrag;

	// the move arrows over the effect picked: J; one dragged moves it along
	qboolean	gizmo;
	qboolean	gzDragging;
	gzDrag_t	gzDrag;
	int			gzHot;
	qboolean	addOpen;			// the actions to add are listed, in a popup over the panel
	qboolean	addPlaced;			// the popup placed this frame, where addPop says
	float		addPop[4];			// x y w h
	float		addBtn[4];			// the button that opens it
	qboolean	popClick;			// this frame's click, landed in the popup
	qboolean	popHover;			// the mouse over the popup: nothing under it lights up
	qboolean	extraOpen;			// an extra's settings shown

	// a preview going round between the marks either side of the moment
	qboolean	loop;
	int			loopFrom, loopTo;
	// a picture of the moment: the manager hidden a few frames for it
	int			pictureFrames;

	// the free camera flown and kept as a shot: where it was, every tenth of a second
	qboolean	camRec;
	int			camRecStart, camRecNext, camRecCount;
	vec3_t		camRecOrg[CM_CAMREC_MAX], camRecAng[CM_CAMREC_MAX];
	float		camRecFov[CM_CAMREC_MAX];
	int			camRecMs[CM_CAMREC_MAX];
	char		newNpc[MAX_QPATH];	// an NPC to spawn in it, by its targetname, being typed
	int			listAsked;			// cls.realtime the NPC list was last asked for
	int			entScroll, pickScroll;
	int			typing;				// cmTyping_t
	qboolean	animLive;			// the animation picked shown on its model while it's made
	int			liveStart;
	rcAnim_t	liveShown;

	// the free camera; its fov is cg_fov's
	vec3_t		camOrg, camAng;
	float		camFov;
	qboolean	flying;				// right mouse held
	qboolean	through;			// the camera is the spot: flying moves it
	qboolean	held[MAX_KEYS];
	int			lastFrameTime;

	// where the camera's crosshair is
	qboolean	haveAim;
	vec3_t		aim;

	// input; clicks and the wheel go to the control drawn under them
	float		cursorX, cursorY;
	qboolean	click;
	float		clickX, clickY;
	int			wheel;

	// saved cinematics and old shots, while the list is up
	qboolean	listing;
	char		files[CM_MAX_FILES][MAX_QPATH];
	qboolean	fileOld[CM_MAX_FILES];
	int			numFiles, fileScroll;

	// a preview on this screen
	qboolean	preview;

	// the view drawn, for the numbers over the spots
	qboolean	haveView;
	vec3_t		viewOrg;
	vec3_t		viewAxis[3];
	float		viewFovX, viewFovY;
	float		dofNow;
	int			dofTime;
	int			numLines;

	// the timeline
	qboolean	tlHidden;
	qboolean	tlPinned;			// a moment picked: everything shown as it is then
	int			tlMs;
	qboolean	tlCam;				// seen through the cinematic's camera then, not the free one
	int			tlScroll;
	int			tlDrag;				// cmDrag_t
	int			tlDragShot, tlDragIndex;
	int			tlDragGrab;			// ms from where the bar starts to where it was grabbed
	int			tlDragSpan;			// the timeline's length when it was grabbed, kept while dragging
	qboolean	applied;			// the timeline's poses and animations are on

	// the shots' lengths as last seen: a shot made longer or shorter takes what's in it and after
	// it along
	int			seenShots;
	int			seenLens[CM_MAX_SHOTS];

	// the moves worked out, again when they change and now and then for where everyone is
	int			planTime;
	cmEnt_t		plannedEnts[CM_MAX_ENTS];
	cmClip_t	plannedClips[CM_MAX_CLIPS];
	int			plannedEntCount, plannedClipCount;

	char		message[MAX_STRING_CHARS];
	int			messageTime;
} cm;

static cmPlan_t cmEditPlans[CM_MAX_CLIPS];

// a cinematic played for everyone came in with the manager open: shown as a preview is
static qboolean cm_EditorOpenForPlay( void ) {
	if ( !cm.open )
		return qfalse;
	cm.preview = qtrue;
	cm.flying = qfalse;
	cm.through = qfalse;
	return qtrue;
}

static void cm_EditorPlayStopped( void ) {
	cm.preview = qfalse;
}

static vec4_t cmWhite		= { 1.0f, 1.0f, 1.0f, 1.0f };
static vec4_t cmPanel		= { 0.0f, 0.0f, 0.0f, 0.7f };
static vec4_t cmPanelLight	= { 0.15f, 0.15f, 0.15f, 0.85f };
static vec4_t cmFocusBox	= { 0.1f, 0.2f, 0.3f, 0.9f };
static vec4_t cmHighlight	= { 0.2f, 0.45f, 0.8f, 0.85f };
static vec4_t cmHoverBox	= { 0.28f, 0.28f, 0.28f, 0.9f };
static vec4_t cmBorder		= { 0.5f, 0.5f, 0.5f, 0.8f };
static vec4_t cmDim			= { 0.65f, 0.65f, 0.65f, 1.0f };
static vec4_t cmGuide		= { 1.0f, 1.0f, 1.0f, 0.25f };
static vec4_t cmBarGuide	= { 0.0f, 0.0f, 0.0f, 0.45f };
static vec4_t cmTabLine		= { 0.2f, 0.45f, 0.8f, 1.0f };

static const byte cmTrailColor[4]	= { 255, 200, 40, 255 };
static const byte cmFaintTrail[4]	= { 255, 200, 40, 90 };
static const byte cmKeyColor[4]		= { 235, 235, 235, 255 };
static const byte cmSelColor[4]		= { 60, 220, 255, 255 };
static const byte cmGhostColor[4]	= { 80, 255, 120, 255 };
static const byte cmLookColor[4]	= { 255, 80, 220, 255 };
static const byte cmAimColor[4]		= { 255, 140, 40, 255 };
static const byte cmRayColor[4]		= { 60, 220, 255, 110 };
static const byte cmCircleColor[4]	= { 255, 200, 40, 90 };
static const byte cmMoveColor[4]	= { 190, 110, 255, 255 };
static const byte cmMoveFaint[4]	= { 190, 110, 255, 110 };

qboolean CM_EditorOpen( void ) {
	return cm.open;
}

qboolean CM_EditorViewing( void ) {
	return (qboolean)( cm.open && !cm.preview );
}

static void CM_Message( const char *text ) {
	Q_strncpyz( cm.message, text, sizeof( cm.message ) );
	cm.messageTime = cls.realtime;
}

// undo and redo: the cinematic as its file's text, kept once a change has settled (the mouse let
// go and nothing else changed for a moment), so a drag or a run of clicks goes back in one
#define CM_UNDO_MAX			60
#define CM_UNDO_SETTLE_MS	400

static std::vector<std::string> cmUndo, cmRedo;
static std::string cmUndoBase;			// as it was when last kept
static qboolean cmUndoPending;
static int cmUndoTime;

static void CM_Changed( void ) {
	cm.dirty = qtrue;
	cmUndoPending = qtrue;
	cmUndoTime = cls.realtime;
}

static void CM_CamRecStart( void );
static void CM_CamRecStop( void );
static void CM_UndoReset( void );
static void CM_UndoFrame( void );
static void CM_Undo( qboolean redo );
static void CM_Copy( void );
static void CM_Paste( void );

static rcShot_t *CM_Shot( void ) {
	if ( cm.cur < 0 || cm.cur >= cm.cine.numShots )
		return NULL;
	return &cm.cine.shots[cm.cur];
}

static rcKey_t *CM_SelectedKey( void ) {
	rcShot_t *s = CM_Shot();

	if ( !s || s->orbit || cm.sel < 0 || cm.sel >= s->numKeys )
		return NULL;
	return &s->keys[cm.sel];
}

static cmClip_t *CM_Clip( void ) {
	if ( cm.clip < 0 || cm.clip >= cm.cine.numClips )
		return NULL;
	return &cm.cine.clips[cm.clip];
}

static void CM_KeyFromCamera( rcKey_t *k ) {
	VectorCopy( cm.camOrg, k->org );
	VectorCopy( cm.camAng, k->ang );
	k->fov = cm.camFov;
}

static void CM_CameraFromKey( const rcKey_t *k ) {
	VectorCopy( k->org, cm.camOrg );
	VectorCopy( k->ang, cm.camAng );
	cm.camAng[PITCH] = Com_Clamp( -89.0f, 89.0f, AngleNormalize180( cm.camAng[PITCH] ) );
	cm.camFov = k->fov > 0 ? k->fov : cm.camFov;
}

static void CM_SelectKey( int index ) {
	rcShot_t *s = CM_Shot();
	rcKey_t *k;

	cm.sel = s && s->numKeys ? Com_Clampi( 0, s->numKeys - 1, index ) : -1;
	if ( cm.sel < cm.keyScroll )
		cm.keyScroll = Q_max( 0, cm.sel );
	else if ( cm.sel >= cm.keyScroll + CM_KEY_ROWS )
		cm.keyScroll = cm.sel - CM_KEY_ROWS + 1;
	k = CM_SelectedKey();
	if ( cm.through && k )
		CM_CameraFromKey( k );
	if ( !k )
		cm.through = qfalse;
}

static void CM_SelectShot( int shot ) {
	if ( !cm.cine.numShots ) {
		cm.cur = 0;
		cm.sel = -1;
		return;
	}
	shot = Com_Clampi( 0, cm.cine.numShots - 1, shot );
	if ( shot != cm.cur ) {
		cm.cur = shot;
		cm.keyScroll = 0;
		cm.through = qfalse;
		CM_SelectKey( 0 );
	}
}

// the shots' lengths as they are now, for CM_Ripple
static void CM_SeeShots( void ) {
	cm.seenShots = cm.cine.numShots;
	for ( int i = 0; i < cm.cine.numShots; i++ )
		cm.seenLens[i] = CM_ShotLen( &cm.cine.shots[i] );
}

// a shot longer or shorter: what starts in it stays where it was in it, stretched or squeezed
// with it, what comes after it moves by as much; the timeline's moment too
static void CM_Ripple( void ) {
	cmCine_t *c = &cm.cine;
	int oldStarts[CM_MAX_SHOTS + 1], newStarts[CM_MAX_SHOTS + 1];
	qboolean changed = qfalse;

	if ( cm.seenShots != c->numShots ) {
		CM_SeeShots();
		return;
	}
	oldStarts[0] = newStarts[0] = 0;
	for ( int i = 0; i < c->numShots; i++ ) {
		const int len = CM_ShotLen( &c->shots[i] );

		changed = (qboolean)( changed || len != cm.seenLens[i] );
		oldStarts[i + 1] = oldStarts[i] + cm.seenLens[i];
		newStarts[i + 1] = newStarts[i] + len;
	}
	if ( !changed )
		return;
#define CM_REMAP( t ) do { \
		int i_ = 0; \
		while ( i_ < c->numShots - 1 && ( t ) >= oldStarts[i_ + 1] ) \
			i_++; \
		if ( ( t ) >= oldStarts[c->numShots] ) \
			( t ) += newStarts[c->numShots] - oldStarts[c->numShots]; \
		else if ( cm.seenLens[i_] > 0 ) \
			( t ) = newStarts[i_] + (int)( (float)( ( t ) - oldStarts[i_] ) * ( newStarts[i_ + 1] - newStarts[i_] ) / cm.seenLens[i_] + 0.5f ); \
		else \
			( t ) += newStarts[i_] - oldStarts[i_]; \
	} while ( 0 )
	for ( int i = 0; i < c->numClips; i++ )
		CM_REMAP( c->clips[i].start );
	for ( int i = 0; i < c->numFx; i++ )
		CM_REMAP( c->fx[i].start );
	for ( int i = 0; i < c->numSnd; i++ )
		CM_REMAP( c->snd[i].start );
	for ( int i = 0; i < c->numTxt; i++ )
		CM_REMAP( c->txt[i].start );
	for ( int i = 0; i < c->numProps; i++ )
		CM_REMAP( c->props[i].start );
	for ( int i = 0; i < c->numLights; i++ )
		CM_REMAP( c->lights[i].start );
	for ( int i = 0; i < c->numScreens; i++ )
		CM_REMAP( c->screens[i].start );
	for ( int i = 0; i < c->numMapLooks; i++ )
		CM_REMAP( c->mapLooks[i].start );
	for ( int i = 0; i < c->numSpeeds; i++ )
		CM_REMAP( c->speeds[i].start );
	if ( cm.tlPinned && !cm.tlDrag )
		CM_REMAP( cm.tlMs );
#undef CM_REMAP
	CM_SeeShots();
}

// what starts at or after ms moves by delta, what's between ms and ms - delta back to ms
static void CM_ShiftClips( int ms, int delta ) {
	const int clips = cm.cine.numClips, fx = clips + cm.cine.numFx, snd = fx + cm.cine.numSnd, txt = snd + cm.cine.numTxt;
	const int props = txt + cm.cine.numProps, lights = props + cm.cine.numLights, screens = lights + cm.cine.numScreens;
	const int maps = screens + cm.cine.numMapLooks;

	for ( int i = 0; i < maps + cm.cine.numSpeeds; i++ ) {
		int *start = i < clips ? &cm.cine.clips[i].start : i < fx ? &cm.cine.fx[i - clips].start
			: i < snd ? &cm.cine.snd[i - fx].start : i < txt ? &cm.cine.txt[i - snd].start
			: i < props ? &cm.cine.props[i - txt].start : i < lights ? &cm.cine.lights[i - props].start
			: i < screens ? &cm.cine.screens[i - lights].start : i < maps ? &cm.cine.mapLooks[i - screens].start : &cm.cine.speeds[i - maps].start;

		if ( delta < 0 && *start >= ms + delta && *start < ms )
			*start = ms + delta;
		else if ( *start >= ms )
			*start = Q_max( 0, *start + delta );
	}
}

// a new shot after the one shown, held on where the camera is
static void CM_AddShot( void ) {
	cmCine_t *c = &cm.cine;
	const int at = c->numShots ? cm.cur + 1 : 0;
	rcShot_t *s;

	if ( c->numShots >= CM_MAX_SHOTS ) {
		CM_Message( S_COLOR_YELLOW "A cinematic has 16 shots at most" );
		return;
	}
	// what comes after it waits for it
	CM_ShiftClips( CM_ShotStart( c, at ), CM_HOLD_MS );
	memmove( &c->shots[at + 1], &c->shots[at], ( c->numShots - at ) * sizeof( c->shots[0] ) );
	c->numShots++;
	s = &c->shots[at];
	memset( s, 0, sizeof( *s ) );
	CM_KeyFromCamera( &s->keys[0] );
	s->numKeys = 1;
	s->ms = CM_HOLD_MS;
	CM_SeeShots();
	cm.cur = -1;
	CM_SelectShot( at );
	CM_Changed();
	CM_Message( va( "Shot %i: held where the camera is. Enter adds spots for it to move through", at + 1 ) );
}

static void CM_DeleteShot( void ) {
	cmCine_t *c = &cm.cine;

	if ( !CM_Shot() )
		return;
	// what was in it goes to where it began, what came after it closes up
	CM_ShiftClips( CM_ShotStart( c, cm.cur + 1 ), -CM_ShotLen( &c->shots[cm.cur] ) );
	memmove( &c->shots[cm.cur], &c->shots[cm.cur + 1], ( c->numShots - cm.cur - 1 ) * sizeof( c->shots[0] ) );
	c->numShots--;
	CM_SeeShots();
	cm.through = qfalse;
	{
		const int was = cm.cur;

		cm.cur = -1;
		CM_SelectShot( Q_min( was, c->numShots - 1 ) );
	}
	CM_Changed();
}

// a spot where the camera is, after the selected one; the first makes the first shot
static void CM_AddKey( void ) {
	rcShot_t *s = CM_Shot();
	int at;

	if ( !s || s->orbit ) {
		if ( !s )
			CM_AddShot();
		return;
	}
	if ( s->numKeys >= RC_MAX_KEYS ) {
		CM_Message( S_COLOR_YELLOW "A shot has 16 spots at most: make another shot" );
		return;
	}
	at = ( cm.sel >= 0 && cm.sel < s->numKeys ) ? cm.sel + 1 : s->numKeys;
	memmove( &s->keys[at + 1], &s->keys[at], ( s->numKeys - at ) * sizeof( s->keys[0] ) );
	s->numKeys++;
	memset( &s->keys[at], 0, sizeof( s->keys[0] ) );
	CM_KeyFromCamera( &s->keys[at] );
	if ( !at ) {
		s->keys[0].ms = 0;
		s->keys[1].ms = CM_KEY_MS;
	} else if ( s->numKeys == 2 ) {
		// what it was held for, it moves in
		s->keys[1].ms = s->ms > 0 ? s->ms : CM_KEY_MS;
	} else {
		s->keys[at].ms = s->keys[at - 1].ms > 0 && at > 1 ? s->keys[at - 1].ms : CM_KEY_MS;
	}
	cm.through = qfalse;
	CM_SelectKey( at );
	CM_Changed();
}

static void CM_DeleteKey( void ) {
	rcShot_t *s = CM_Shot();
	const int at = cm.sel;

	if ( !CM_SelectedKey() )
		return;
	if ( s->numKeys == 1 ) {
		CM_DeleteShot();
		CM_Message( "Its only spot gone, the shot is too" );
		return;
	}
	// one left is held for as long as the shot was
	if ( s->numKeys == 2 )
		s->ms = Q_max( 500, RC_ShotMs( s ) );
	memmove( &s->keys[at], &s->keys[at + 1], ( s->numKeys - at - 1 ) * sizeof( s->keys[0] ) );
	s->numKeys--;
	s->keys[0].ms = 0;
	cm.through = qfalse;
	CM_SelectKey( at < s->numKeys ? at : s->numKeys - 1 );
	CM_Changed();
}

static void CM_SetKeyHere( void ) {
	rcKey_t *k = CM_SelectedKey();

	if ( !k )
		return;
	CM_KeyFromCamera( k );
	CM_Changed();
}

static void CM_ToggleThrough( void ) {
	rcKey_t *k = CM_SelectedKey();

	if ( !k ) {
		cm.through = qfalse;
		return;
	}
	cm.through = (qboolean)!cm.through;
	if ( cm.through ) {
		CM_CameraFromKey( k );
		cm.tlCam = qfalse;
	}
}

static void CM_SetOrbit( qboolean orbit ) {
	rcShot_t *s = CM_Shot();

	if ( !s || s->orbit == orbit )
		return;
	s->orbit = orbit;
	cm.through = qfalse;
	if ( orbit ) {
		// round what the crosshair is on, from where the camera is
		if ( cm.haveAim ) {
			VectorCopy( cm.aim, s->center );
		} else {
			vec3_t forward;

			AngleVectors( cm.camAng, forward, NULL, NULL );
			VectorMA( cm.camOrg, 300.0f, forward, s->center );
		}
		if ( !s->radius ) {
			s->radius = 200;
			s->height = 64;
			s->degrees = 360;
			s->linear = qtrue;
		}
		s->ms = Q_max( s->ms, 4000 );
		s->fov = s->endFov = cm.camFov;
		s->fromYaw = RAD2DEG( atan2f( cm.camOrg[1] - s->center[1], cm.camOrg[0] - s->center[0] ) );
	} else if ( !s->numKeys ) {
		CM_KeyFromCamera( &s->keys[0] );
		s->numKeys = 1;
	}
	CM_SelectKey( 0 );
	CM_Changed();
}

static void CM_OrbitFromHere( void ) {
	rcShot_t *s = CM_Shot();
	float dx, dy;

	if ( !s )
		return;
	dx = cm.camOrg[0] - s->center[0];
	dy = cm.camOrg[1] - s->center[1];
	s->radius = Q_max( 16.0f, sqrtf( dx * dx + dy * dy ) );
	s->height = cm.camOrg[2] - s->center[2];
	s->fromYaw = RAD2DEG( atan2f( dy, dx ) );
	s->fov = s->endFov = cm.camFov;
	CM_Changed();
}

/*
===============================================================================

Who's in it, and what they do

===============================================================================
*/

// the player or NPC whose middle the crosshair is nearest: me, a player's number, an NPC's
// targetname or number; NULL if none near
static const char *CM_PersonAtCrosshair( void ) {
	float best = CM_PERSON_NEAR;
	int found = -1;

	if ( !cm.haveAim )
		return NULL;
	if ( Distance( cl.snap.ps.origin, cm.aim ) < best ) {
		best = Distance( cl.snap.ps.origin, cm.aim );
		found = clc.clientNum;
	}
	for ( int i = 0; i < cl.snap.numEntities; i++ ) {
		const entityState_t *es = &cl.parseEntities[( cl.snap.parseEntitiesNum + i ) & ( MAX_PARSE_ENTITIES - 1 )];

		if ( es->eType != ET_NPC && es->eType != ET_PLAYER )
			continue;
		if ( Distance( es->pos.trBase, cm.aim ) < best ) {
			best = Distance( es->pos.trBase, cm.aim );
			found = es->number;
		}
	}
	if ( found < 0 )
		return NULL;
	if ( found == clc.clientNum )
		return "me";
	if ( found < MAX_CLIENTS )
		return va( "%i", found );
	for ( int i = 0; i < CL_NpcManager_NumNpcs(); i++ ) {
		const char *type, *name;
		int num;

		if ( CL_NpcManager_GetNpc( i, &num, &type, &name ) && num == found && name[0] )
			return name;
	}
	return va( "%i", found );
}

static int CM_AddEnt( const char *who ) {
	const int existing = who[0] ? CM_FindEnt( &cm.cine, who, qfalse ) : -1;

	if ( existing >= 0 ) {
		cm.ent = existing;
		return existing;
	}
	if ( cm.cine.numEnts >= CM_MAX_ENTS ) {
		CM_Message( S_COLOR_YELLOW "12 players and NPCs at most" );
		return -1;
	}
	Q_strncpyz( cm.cine.ents[cm.cine.numEnts].who, who, sizeof( cm.cine.ents[0].who ) );
	cm.ent = cm.cine.numEnts++;
	cm.clip = -1;
	CM_Changed();
	return cm.ent;
}

static void CM_RemoveEnt( void ) {
	cmCine_t *c = &cm.cine;
	const int e = cm.ent;

	if ( e < 0 || e >= c->numEnts )
		return;
	for ( int i = 0; i < c->numClips; ) {
		if ( c->clips[i].ent == e ) {
			c->numClips--;
			memmove( &c->clips[i], &c->clips[i + 1], ( c->numClips - i ) * sizeof( c->clips[0] ) );
			continue;
		}
		if ( c->clips[i].ent > e )
			c->clips[i].ent--;
		// a force power at them is at no one now
		if ( c->clips[i].kind == CM_CLIP_FORCE && c->clips[i].target == e )
			c->clips[i].target = -1;
		else if ( c->clips[i].kind == CM_CLIP_FORCE && c->clips[i].target > e )
			c->clips[i].target--;
		i++;
	}
	c->numEnts--;
	memmove( &c->ents[e], &c->ents[e + 1], ( c->numEnts - e ) * sizeof( c->ents[0] ) );
	cm.ent = Q_min( e, c->numEnts - 1 );
	cm.clip = -1;
	cm.typing = CM_TYPE_NONE;
	CM_Changed();
}

// the moment new things start: the timeline's, or after what they do already
static int CM_AddTime( int kind ) {
	int at = 0;

	if ( cm.tlPinned )
		return cm.tlMs;
	for ( int i = 0; i < cm.cine.numClips; i++ ) {
		const cmClip_t *clip = &cm.cine.clips[i];

		if ( clip->ent != cm.ent || CM_Moves( clip->kind ) != CM_Moves( kind ) )
			continue;
		at = Q_max( at, CM_Moves( clip->kind ) ? clip->start + ( cmEditPlans[i].count ? cmEditPlans[i].ms : 1000 )
			: clip->kind == CM_CLIP_SAY ? clip->start + CM_SAY_MS : CM_Instant( clip->kind ) ? clip->start + CM_ACT_MS
			: clip->start + CM_AnimLen( &cm.cine, clip ) );
	}
	return Q_min( at, Q_max( 0, CM_Total( &cm.cine ) - 500 ) );
}

static void CM_AddPlaceKey( cmClip_t *clip );
static void CM_StartRecording( int clip );

static void CM_AddClip( int kind, qboolean tele ) {
	cmCine_t *c = &cm.cine;
	cmClip_t *clip;

	if ( ( cm.ent < 0 || cm.ent >= c->numEnts ) && cm.pickWho[0] )
		CM_AddEnt( cm.pickWho );
	if ( cm.ent < 0 || cm.ent >= c->numEnts )
		return;
	if ( c->numClips >= CM_MAX_CLIPS ) {
		CM_Message( S_COLOR_YELLOW "64 actions at most" );
		return;
	}
	clip = &c->clips[c->numClips];
	memset( clip, 0, sizeof( *clip ) );
	clip->kind = kind;
	clip->ent = cm.ent;
	clip->start = CM_AddTime( kind );
	if ( kind == CM_CLIP_ANIM ) {
		Q_strncpyz( clip->a.anim, "BOTH_STAND1", sizeof( clip->a.anim ) );
		clip->a.to = -1;
		clip->a.speed = 100;
		clip->a.ms = CM_ANIM_MS;
		CM_Message( "Pick the animation from the list, or type its name" );
	} else if ( kind == CM_CLIP_SAY ) {
		cm.typing = CM_TYPE_SAY;
		CM_Message( "Type what they say" );
	} else if ( kind == CM_CLIP_FORCE ) {
		clip->a.ms = CM_FORCE_MS;
		clip->target = -1;
		clip->forceAnims = qtrue;
		// at the first one in it who isn't them
		for ( int e = 0; e < c->numEnts && clip->target < 0; e++ ) {
			if ( e != cm.ent )
				clip->target = e;
		}
		CM_Message( "A force power: pick it and who it's at; its end on the timeline is when it stops" );
	} else if ( kind == CM_CLIP_LOOK ) {
		clip->a.ms = CM_EMOTE_MS;
		clip->size = 100;
		clip->blade = 100;
		CM_Message( "How they look meanwhile: drag its end on the timeline for how long" );
	} else if ( kind == CM_CLIP_EMOTE ) {
		clip->a.to = -1;
		clip->a.speed = 100;
		clip->a.ms = CM_EMOTE_MS;
		clip->realEmote = qtrue;
		cm.typing = CM_TYPE_EMOTE;
		CM_Message( "Pick the emote from the list, or type its name" );
	} else if ( kind == CM_CLIP_PLACE && tele ) {
		clip->tele = qtrue;
		CM_Message( "Where they're teleported to: where you aim, or drag it in the view" );
	} else if ( kind == CM_CLIP_PLACE ) {
		clip->smooth = qtrue;
		CM_Message( "Set where they are and how they're turned; move the moment and + Key for the next" );
	} else if ( kind == CM_CLIP_NPCCMD ) {
		Q_strncpyz( clip->cmd, cmNpcCmds[0], sizeof( clip->cmd ) );
		CM_Message( "Pick the command" );
	} else if ( kind == CM_CLIP_SPAWN ) {
		Q_strncpyz( clip->team, "default", sizeof( clip->team ) );
		clip->numKeys = 1;
		if ( cm.haveAim )
			VectorCopy( cm.aim, clip->keys[0].org );
		clip->keys[0].ang[YAW] = cm.camAng[YAW] + 180.0f;
		cm.typing = CM_TYPE_NPCTYPE;
		CM_Message( "Type the NPC type, e.g. stormtrooper; it spawns where you aim" );
	} else if ( kind == CM_CLIP_REMOVE ) {
		CM_Message( "Taken out of the game at its moment: drag it on the timeline" );
	} else if ( kind == CM_CLIP_RECORD ) {
		clip->rec = -1;
	} else {
		CM_Message( "Aim where they go and press Enter, a spot at a time" );
	}
	cm.clip = c->numClips++;
	cm.spot = -1;
	cm.pickScroll = 0;
	CM_Changed();
	// a position starts where they are, a recording at once
	if ( kind == CM_CLIP_PLACE )
		CM_AddPlaceKey( &c->clips[cm.clip] );
	else if ( kind == CM_CLIP_RECORD )
		CM_StartRecording( cm.clip );
}

// where someone is now, as drawn, and which way they face
static void CM_EntityNow( int ent, vec3_t org, vec3_t ang ) {
	int nums[4];

	VectorClear( ang );
	if ( ent >= 0 && ent < cm.cine.numEnts && CM_Resolve( cm.cine.ents[ent].who, nums, ARRAY_LEN( nums ) ) ) {
		if ( CL_NpcWalk_PoseOf( nums[0], org ) )
			return;
		if ( nums[0] == clc.clientNum ) {
			VectorCopy( cl.snap.ps.origin, org );
			ang[YAW] = cl.snap.ps.viewangles[YAW];
			return;
		}
		for ( int i = 0; i < cl.snap.numEntities; i++ ) {
			const entityState_t *es = &cl.parseEntities[( cl.snap.parseEntitiesNum + i ) & ( MAX_PARSE_ENTITIES - 1 )];

			if ( es->number == nums[0] ) {
				VectorCopy( es->pos.trBase, org );
				ang[YAW] = es->apos.trBase[YAW];
				return;
			}
		}
		if ( CM_EntOrigin( nums[0], org ) )
			return;
	}
	VectorCopy( cm.aim, org );
	org[2] += 25.0f;
}

// a position's keys by when they come, the one picked kept picked
static void CM_SortKeys( cmClip_t *clip ) {
	for ( int i = 1; i < clip->numKeys; i++ ) {
		for ( int j = i; j > 0 && clip->keys[j - 1].ms > clip->keys[j].ms; j-- ) {
			const cmPlace_t k = clip->keys[j];

			clip->keys[j] = clip->keys[j - 1];
			clip->keys[j - 1] = k;
			if ( cm.spot == j )
				cm.spot = j - 1;
			else if ( cm.spot == j - 1 )
				cm.spot = j;
		}
	}
}

// a key at the timeline's moment, or a second after the last: where they'd be then
static void CM_AddPlaceKey( cmClip_t *clip ) {
	vec3_t org, ang;
	int ms, at;

	if ( clip->numKeys >= CM_MAX_PLACE ) {
		CM_Message( S_COLOR_YELLOW "12 keys at most: add another position after it" );
		return;
	}
	ms = cm.tlPinned ? Q_max( 0, cm.tlMs - clip->start ) : clip->numKeys ? clip->keys[clip->numKeys - 1].ms + 1000 : 0;
	for ( int i = 0; i < clip->numKeys; i++ ) {
		if ( clip->keys[i].ms == ms ) {
			cm.spot = i;
			return;
		}
	}
	if ( clip->numKeys )
		CM_PlaceAt( clip, ms, org, ang );
	else
		CM_EntityNow( clip->ent, org, ang );
	for ( at = 0; at < clip->numKeys && clip->keys[at].ms < ms; at++ )
		;
	memmove( &clip->keys[at + 1], &clip->keys[at], ( clip->numKeys - at ) * sizeof( clip->keys[0] ) );
	clip->keys[at].ms = ms;
	VectorCopy( org, clip->keys[at].org );
	VectorCopy( ang, clip->keys[at].ang );
	clip->numKeys++;
	cm.spot = at;
	CM_Changed();
}

static void CM_DeletePlaceKey( cmClip_t *clip ) {
	if ( cm.spot < 0 || cm.spot >= clip->numKeys )
		return;
	memmove( &clip->keys[cm.spot], &clip->keys[cm.spot + 1], ( clip->numKeys - cm.spot - 1 ) * sizeof( clip->keys[0] ) );
	clip->numKeys--;
	cm.spot = Q_min( cm.spot, clip->numKeys - 1 );
	CM_Changed();
}

static void CM_DeleteClip( void ) {
	cmCine_t *c = &cm.cine;

	if ( !CM_Clip() )
		return;
	// its recording goes with it
	if ( c->clips[cm.clip].kind == CM_CLIP_RECORD && c->clips[cm.clip].rec >= 0 && c->clips[cm.clip].rec < CM_MAX_RECS )
		c->recs[c->clips[cm.clip].rec].count = 0;
	c->numClips--;
	memmove( &c->clips[cm.clip], &c->clips[cm.clip + 1], ( c->numClips - cm.clip ) * sizeof( c->clips[0] ) );
	cm.clip = -1;
	cm.typing = CM_TYPE_NONE;
	CM_Changed();
}

// a spot where the crosshair is, for the move picked
static void CM_AddMoveSpot( void ) {
	cmClip_t *clip = CM_Clip();

	if ( !clip || clip->kind != CM_CLIP_MOVE )
		return;
	if ( !cm.haveAim ) {
		CM_Message( S_COLOR_YELLOW "Aim at the floor where they go" );
		return;
	}
	if ( clip->numPts >= CM_MAX_PTS ) {
		CM_Message( S_COLOR_YELLOW "16 spots at most: add another move after it" );
		return;
	}
	{
		// after the one picked, or at the end
		const int at = cm.spot >= 0 && cm.spot < clip->numPts ? cm.spot + 1 : clip->numPts;

		memmove( &clip->pts[at + 1], &clip->pts[at], ( clip->numPts - at ) * sizeof( clip->pts[0] ) );
		VectorCopy( cm.aim, clip->pts[at] );
		clip->pts[at][2] += 25.0f;	// an origin over the floor
		clip->numPts++;
		cm.spot = at;
	}
	CM_Changed();
}

// the spot picked, or the last
static void CM_RemoveMoveSpot( void ) {
	cmClip_t *clip = CM_Clip();
	int at;

	if ( !clip || clip->kind != CM_CLIP_MOVE || !clip->numPts )
		return;
	at = cm.spot >= 0 && cm.spot < clip->numPts ? cm.spot : clip->numPts - 1;
	memmove( &clip->pts[at], &clip->pts[at + 1], ( clip->numPts - at - 1 ) * sizeof( clip->pts[0] ) );
	clip->numPts--;
	cm.spot = Q_min( at, clip->numPts - 1 );
	CM_Changed();
}

/*
===============================================================================

Opening, saving

===============================================================================
*/

static void CM_New( void ) {
	CM_Default( &cm.cine );
	cm.name[0] = '\0';
	cm.dirty = qfalse;
	cm.cur = 0;
	cm.sel = -1;
	cm.keyScroll = 0;
	cm.ent = -1;
	cm.pickNum = -1;
	cm.pickWho[0] = '\0';
	cm.clip = -1;
	cm.spot = -1;
	cm.spotDrag = qfalse;
	cm.addOpen = qfalse;
	cm.entScroll = 0;
	cm.through = qfalse;
	cm.tlPinned = qfalse;
	cm.tlDrag = CM_DRAG_NONE;
	cm.tlMs = 0;
	cm.tlScroll = 0;
	cm.planTime = 0;
	cm.seenShots = -1;
	cm.numMulti = 0;
	CM_UndoReset();
}

static void CM_AfterLoad( const char *name ) {
	const cmCine_t c = cm.cine;

	CM_New();
	cm.cine = c;
	CM_UndoReset();
	Q_strncpyz( cm.name, name, sizeof( cm.name ) );
	cm.cur = -1;
	CM_SelectShot( 0 );
	cm.ent = -1;
	// the camera a step behind where it starts, to see it from
	if ( cm.cine.numShots ) {
		const rcShot_t *s = &cm.cine.shots[0];
		vec3_t org, ang, forward;
		float fov;

		RC_EvalShot( s, 0, org, ang, &fov );
		VectorCopy( org, cm.camOrg );
		VectorCopy( ang, cm.camAng );
		cm.camAng[PITCH] = Com_Clamp( -89.0f, 89.0f, AngleNormalize180( cm.camAng[PITCH] ) );
		cm.camAng[ROLL] = 0;
		AngleVectors( cm.camAng, forward, NULL, NULL );
		VectorMA( cm.camOrg, -80.0f, forward, cm.camOrg );
		cm.camOrg[2] += 16.0f;
	}
}

static qboolean CM_Open( const char *name, qboolean old ) {
	if ( old ? !CM_ImportShot( name, &cm.cine ) : !CM_Load( name, &cm.cine ) ) {
		CM_Message( va( S_COLOR_RED "No cinematic called %s", name ) );
		return qfalse;
	}
	CM_AfterLoad( name );
	CM_Message( old ? va( S_COLOR_GREEN "Opened the old shot %s: Save keeps it as a cinematic", name ) : va( S_COLOR_GREEN "Opened %s", name ) );
	return qtrue;
}

static qboolean CM_SaveCurrent( void ) {
	if ( !cm.name[0] ) {
		cm.typing = CM_TYPE_NAME;
		CM_Message( S_COLOR_YELLOW "Type a name for it first" );
		return qfalse;
	}
	if ( !cm.cine.numShots ) {
		CM_Message( S_COLOR_YELLOW "Nothing to save: fly where it starts and press Enter" );
		return qfalse;
	}
	if ( !CM_Save( &cm.cine, cm.name ) ) {
		CM_Message( va( S_COLOR_RED "Couldn't write %s", CM_Path( cm.name ) ) );
		return qfalse;
	}
	cm.dirty = qfalse;
	cm.typing = CM_TYPE_NONE;
	CM_Message( va( S_COLOR_GREEN "Saved %s", cm.name ) );
	if ( cm.fromStoryteller )
		CL_Storyteller_ShotSaved( cm.name );
	return qtrue;
}

static int QDECL CM_CompareFiles( const void *a, const void *b ) {
	return Q_stricmp( (const char *)a, (const char *)b );
}

static void CM_ListFiles( void ) {
	char **files;
	int count;

	cm.numFiles = 0;
	cm.fileScroll = 0;
	files = FS_ListFiles( CM_DIR, CM_EXT, &count );
	for ( int i = 0; i < count && cm.numFiles < CM_MAX_FILES; i++ ) {
		COM_StripExtension( files[i], cm.files[cm.numFiles], sizeof( cm.files[0] ) );
		if ( cm.files[cm.numFiles][0] )
			cm.numFiles++;
	}
	FS_FreeFileList( files );
	qsort( cm.files, cm.numFiles, sizeof( cm.files[0] ), CM_CompareFiles );
	memset( cm.fileOld, 0, sizeof( cm.fileOld ) );
	// the old shots after them
	files = FS_ListFiles( RC_SHOT_DIR, RC_SHOT_EXT, &count );
	{
		const int first = cm.numFiles;

		for ( int i = 0; i < count && cm.numFiles < CM_MAX_FILES; i++ ) {
			COM_StripExtension( files[i], cm.files[cm.numFiles], sizeof( cm.files[0] ) );
			cm.fileOld[cm.numFiles] = qtrue;
			if ( cm.files[cm.numFiles][0] )
				cm.numFiles++;
		}
		qsort( cm.files[first], cm.numFiles - first, sizeof( cm.files[0] ), CM_CompareFiles );
	}
	FS_FreeFileList( files );
	cm.listing = qtrue;
}

static void CM_ReleaseKeys( void ) {
	memset( cm.held, 0, sizeof( cm.held ) );
	cm.flying = qfalse;
}

/*
===============================================================================

The timeline's moment, and the preview

===============================================================================
*/

// the moves worked out again when they change, and now and then for where everyone is
static void CM_EditPlans( qboolean force ) {
	cmCine_t *c = &cm.cine;

	if ( !force && cm.plannedEntCount == c->numEnts && cm.plannedClipCount == c->numClips
		&& !memcmp( cm.plannedEnts, c->ents, c->numEnts * sizeof( c->ents[0] ) )
		&& !memcmp( cm.plannedClips, c->clips, c->numClips * sizeof( c->clips[0] ) )
		&& cls.realtime >= cm.planTime && cls.realtime - cm.planTime < 1000 )
		return;
	CM_Plan( c, cmEditPlans );
	memcpy( cm.plannedEnts, c->ents, sizeof( cm.plannedEnts ) );
	memcpy( cm.plannedClips, c->clips, sizeof( cm.plannedClips ) );
	cm.plannedEntCount = c->numEnts;
	cm.plannedClipCount = c->numClips;
	cm.planTime = cls.realtime;
}

// the view is the cinematic's camera at the timeline's moment
static qboolean CM_TlLooking( void ) {
	return (qboolean)( cm.open && !cm.preview && cm.tlPinned && cm.tlCam && cm.cine.numShots );
}

// where the manager's view looks from: the free camera, or the cinematic's at the timeline's
// moment; qtrue for the cinematic's
static qboolean CM_ViewCam( vec3_t org, vec3_t ang, float *fov ) {
	if ( CM_TlLooking() ) {
		int local;
		const int shot = CM_ShotAt( &cm.cine, cm.tlMs, &local );

		RC_EvalShotLive( &cm.cine.shots[shot], local, org, ang, fov, qfalse );
		return qtrue;
	}
	VectorCopy( cm.camOrg, org );
	VectorCopy( cm.camAng, ang );
	*fov = cm.camFov;
	return qfalse;
}

static void CM_TlPin( void ) {
	if ( cm.tlPinned )
		return;
	cm.tlPinned = qtrue;
	if ( cm.tlCam )
		cm.through = qfalse;
}

static void CM_TlLetGo( void ) {
	cm.tlPinned = qfalse;
	cm.tlDrag = CM_DRAG_NONE;
}

// a video of it: played from the start on this screen, every frame kept at cl_aviFrameRate, into
// videos/<name>-<n>.avi in the game folder, until it ends or is stopped
static qboolean cmVideo;
static char cmVideoPath[MAX_QPATH];

static qboolean CM_VideoPlay( const cmCine_t *c, const char *name ) {
	char path[MAX_QPATH];
	int n;

	if ( CL_VideoRecording() || !c->numShots )
		return qfalse;
	for ( n = 1; n < 1000; n++ ) {
		Com_sprintf( path, sizeof( path ), "videos/%s-%03i.avi", name[0] ? name : "cinematic", n );
		if ( !FS_FileExists( path ) )
			break;
	}
	if ( !CM_Play( c, 0, qtrue ) )
		return qfalse;
	if ( !CL_OpenAVIForWriting( path ) ) {
		CM_StopPlaying();
		Com_Printf( S_COLOR_RED "Couldn't make the video file %s\n", path );
		return qfalse;
	}
	cmVideo = qtrue;
	Q_strncpyz( cmVideoPath, path, sizeof( cmVideoPath ) );
	return qtrue;
}

static void CM_RecordVideo( void ) {
	if ( cm.preview || CL_VideoRecording() )
		return;
	if ( !cm.cine.numShots ) {
		CM_Message( S_COLOR_YELLOW "Add a shot first: fly where it starts and press Enter" );
		return;
	}
	CM_Unapply();
	cm.applied = qfalse;
	cm.flying = qfalse;
	cm.through = qfalse;
	cm.typing = CM_TYPE_NONE;
	if ( CM_VideoPlay( &cm.cine, cm.name ) )
		cm.preview = qtrue;
}

static void CM_VideoDone( void ) {
	if ( !cmVideo )
		return;
	cmVideo = qfalse;
	// closed at the end of the frame: the renderer still has the last one to fill
	CL_CloseAVILater();
	Com_Printf( "Cinematic video saved: %s\n", cmVideoPath );
	CM_Message( va( S_COLOR_GREEN "Saved %s in the game folder", cmVideoPath ) );
}

static void CM_StopPreview( void ) {
	if ( !cm.preview )
		return;
	cm.preview = qfalse;
	// the timeline's moment where it got to
	if ( cm.tlPinned && cmp.playing )
		cm.tlMs = Q_max( 0, cmp.t );
	CM_StopPlaying();
}

// the cinematic on this screen, from the timeline's moment or the start
static void CM_LoopSpan( int *from, int *to );

static void CM_Preview( void ) {
	const int total = CM_Total( &cm.cine );
	int from = cm.tlPinned && cm.tlMs < total ? cm.tlMs : 0;

	if ( cm.preview )
		return;
	if ( !cm.cine.numShots ) {
		CM_Message( S_COLOR_YELLOW "Add a shot first: fly where it starts and press Enter" );
		return;
	}
	// going round between the marks either side of the moment
	if ( cm.loop ) {
		CM_LoopSpan( &cm.loopFrom, &cm.loopTo );
		from = cm.loopFrom;
	}
	CM_Unapply();
	cm.applied = qfalse;
	cm.flying = qfalse;
	cm.through = qfalse;
	if ( CM_Play( &cm.cine, from, qtrue ) )
		cm.preview = qtrue;
}

// a picture of the moment as the players see it, through the cinematic's camera, into
// screenshots/<name>-<ms>.jpg, nothing of the manager's on it
static void CM_Picture( void ) {
	if ( !cm.cine.numShots ) {
		CM_Message( S_COLOR_YELLOW "Add a shot first: fly where it starts and press Enter" );
		return;
	}
	if ( !cm.tlPinned ) {
		cm.tlMs = 0;
		CM_TlPin();
	}
	cm.tlCam = qtrue;
	cm.through = qfalse;
	cm.typing = CM_TYPE_NONE;
	cm.pictureFrames = 4;
	Cbuf_AddText( va( "screenshot %s-%06i\n", cm.name[0] ? cm.name : "cinematic", cm.tlMs ) );
	CM_Message( va( S_COLOR_GREEN "Picture of %.1f s in screenshots/", cm.tlMs / 1000.0f ) );
}

// every frame: the timeline's moment shown, or the animation being made on its model
static void CM_EditorApply( void ) {
	cmClip_t *clip = CM_Clip();

	if ( cmp.playing )
		return;
	if ( cm.open && cm.tlPinned ) {
		CM_EditPlans( qfalse );
		CM_Apply( &cm.cine, cmEditPlans, cm.tlMs );
		cm.applied = qtrue;
		return;
	}
	// the animation picked, as it's made, round and round
	if ( cm.open && cm.tab == CM_TAB_ENTITIES && cm.animLive && clip && ( clip->kind == CM_CLIP_ANIM || clip->kind == CM_CLIP_EMOTE ) && clip->ent < cm.cine.numEnts ) {
		int nums[CM_MAX_GROUP], elapsed[CM_MAX_GROUP];
		const rcAnim_t *anims[CM_MAX_GROUP];
		const int n = CM_Resolve( cm.cine.ents[clip->ent].who, nums, CM_MAX_GROUP );
		static rcAnim_t shown;

		if ( memcmp( &cm.liveShown, &clip->a, sizeof( clip->a ) ) ) {
			cm.liveShown = clip->a;
			cm.liveStart = cls.realtime;
		}
		shown = clip->a;
		shown.loop = (qboolean)( shown.mode != RC_AM_HOLD );
		if ( clip->kind == CM_CLIP_EMOTE ) {
			const char *anim = clip->emote[0] ? CL_EmotePreview_Anim( clip->emote ) : NULL;

			Q_strncpyz( shown.anim, anim ? anim : "", sizeof( shown.anim ) );
			shown.from = 0;
			shown.to = -1;
		}
		for ( int i = 0; i < n; i++ ) {
			anims[i] = &shown;
			elapsed[i] = cls.realtime - cm.liveStart;
		}
		CL_NpcWalk_ClearPoses();
		CM_HoldSelf( qfalse );
		RC_TimelineAnims( nums, anims, elapsed, n );
		cm.applied = qtrue;
		return;
	}
	if ( cm.applied ) {
		CM_Unapply();
		cm.applied = qfalse;
	}
}

/*
===============================================================================

Opening and closing the manager, input

===============================================================================
*/

// the manager opens on the named cinematic, or the one it had
void CL_RpCinematic_Edit( const char *name, qboolean fromStoryteller ) {
	vec3_t org, ang;
	float fovY;

	if ( cls.state != CA_ACTIVE || !cls.cgameStarted ) {
		Com_Printf( "Cinematic manager: join a server first\n" );
		return;
	}
	CL_ModelManager_Close();
	CL_NpcManager_Close();
	CL_ShaderManager_Close();
	CL_EffectManager_Close();
	CL_Storyteller_Close();
	CL_SoundManager_Close();
	CL_AnimManager_Close();
	CL_Presets_Close();
	CL_SpecialMoves_Close();
	CL_ChatLog_Close();
	CL_MapEditor_Close();

	cm.font = re->RegisterFont( "arialnb" );
	if ( !cm.font )
		cm.font = cls.menuFont;
	if ( !cm.camFov )
		cm.camFov = RC_DEFAULT_FOV;
	// from where the player looks, unless it's back from picking an effect
	if ( cm.keepCam ) {
		cm.keepCam = qfalse;
	} else if ( RC_PlayerView( org, ang, &fovY ) ) {
		VectorCopy( org, cm.camOrg );
		VectorCopy( ang, cm.camAng );
	} else {
		VectorCopy( cl.snap.ps.origin, cm.camOrg );
		cm.camOrg[2] += cl.snap.ps.viewheight;
		VectorCopy( cl.viewangles, cm.camAng );
	}
	cm.camAng[ROLL] = 0;
	cm.camAng[PITCH] = Com_Clamp( -89.0f, 89.0f, AngleNormalize180( cm.camAng[PITCH] ) );

	cm.fromStoryteller = fromStoryteller;
	if ( name && name[0] && Q_stricmp( name, cm.name ) ) {
		// a cinematic, an old shot, or a new one by that name
		if ( !CM_Open( name, qfalse ) && !CM_Open( name, qtrue ) ) {
			CM_New();
			if ( CM_ValidName( name ) )
				Q_strncpyz( cm.name, name, sizeof( cm.name ) );
			CM_Message( va( "New cinematic %s: fly to where it starts and press Enter", cm.name ) );
		}
	}
	CM_StopPlaying();
	cm.open = qtrue;
	cm.preview = qfalse;
	cm.listing = qfalse;
	cm.typing = CM_TYPE_NONE;
	cm.through = qfalse;
	cm.cursorX = CM_PANEL_X - 120;
	cm.cursorY = SCREEN_HEIGHT * 0.5f;
	cm.click = qfalse;
	cm.wheel = 0;
	cm.lastFrameTime = cls.realtime;
	CM_ReleaseKeys();
	Key_SetCatcher( Key_GetCatcher() | KEYCATCH_CINEDITOR );
	RC_SyncThirdPerson();
}

void CL_RpCinematic_EditorClose( void ) {
	if ( !cm.open )
		return;
	CM_StopPreview();
	CM_TlLetGo();
	cm.open = qfalse;
	cm.typing = CM_TYPE_NONE;
	// a track listened to: the map's music back
	if ( cmMusicPlayed && !cmp.playing ) {
		cmMusicPlayed = qfalse;
		CL_SoundManager_RestoreMusic();
	}
	cm.listing = qfalse;
	cm.through = qfalse;
	CM_ReleaseKeys();
	Key_SetCatcher( Key_GetCatcher() & ~KEYCATCH_CINEDITOR );
	CM_EditorApply();
	RC_SyncThirdPerson();
}

qboolean CL_RpCinematic_EditorActive( void ) {
	return cm.open;
}

static qboolean CM_Held( int upper ) {
	return (qboolean)( cm.held[upper] || cm.held[upper + ( 'a' - 'A' )] );
}

static qboolean CM_ShiftDown( void ) {
	return (qboolean)( cm.held[A_SHIFT] || cm.held[A_SHIFT2] );
}

static qboolean CM_CtrlDown( void ) {
	return (qboolean)( cm.held[A_CTRL] || cm.held[A_CTRL2] );
}

static qboolean CM_IsToggleKey( int key ) {
	const char *binding;

	if ( cm.typing && key >= A_SPACE && key <= A_TILDE )
		return qfalse;
	binding = Key_GetBinding( key );
	return (qboolean)( VALIDSTRING( binding ) && ( !Q_stricmp( binding, CM_TOGGLE_CMD ) || !Q_stricmp( binding, "cincam edit" ) ) );
}

// what's being typed in; NULL if nothing
static char *CM_TypedText( int *size ) {
	rcShot_t *s = CM_Shot();
	cmClip_t *clip = CM_Clip();

	switch ( cm.typing ) {
	case CM_TYPE_NAME:
		*size = sizeof( cm.name );
		return cm.name;
	case CM_TYPE_WHO:
		if ( cm.ent < 0 || cm.ent >= cm.cine.numEnts )
			return NULL;
		*size = sizeof( cm.cine.ents[0].who );
		return cm.cine.ents[cm.ent].who;
	case CM_TYPE_ANIM:
		if ( !clip || clip->kind != CM_CLIP_ANIM )
			return NULL;
		*size = sizeof( clip->a.anim );
		return clip->a.anim;
	case CM_TYPE_FOCUS:
		if ( !s )
			return NULL;
		*size = sizeof( s->dofWho );
		return s->dofWho;
	case CM_TYPE_EXTRANAME:
	case CM_TYPE_EXTRAMODEL:
		if ( cm.ent < 0 || cm.ent >= cm.cine.numEnts || !CM_IsExtra( cm.cine.ents[cm.ent].who ) )
			return NULL;
		*size = cm.typing == CM_TYPE_EXTRANAME ? sizeof( cm.cine.ents[0].who ) : sizeof( cm.cine.ents[0].model );
		return cm.typing == CM_TYPE_EXTRANAME ? cm.cine.ents[cm.ent].who : cm.cine.ents[cm.ent].model;
	case CM_TYPE_HOLD:
		if ( !clip || clip->kind != CM_CLIP_LOOK )
			return NULL;
		*size = sizeof( clip->hold );
		return clip->hold;
	case CM_TYPE_PROPMODEL:
		if ( cm.prop < 0 || cm.prop >= cm.cine.numProps )
			return NULL;
		*size = sizeof( cm.cine.props[0].model );
		return cm.cine.props[cm.prop].model;
	case CM_TYPE_LIGHTWHO:
		if ( cm.light < 0 || cm.light >= cm.cine.numLights )
			return NULL;
		*size = sizeof( cm.cine.lights[0].who );
		return cm.cine.lights[cm.light].who;
	case CM_TYPE_TRACK:
		if ( !s )
			return NULL;
		*size = sizeof( s->trackWho );
		return s->trackWho;
	case CM_TYPE_EMOTE:
		if ( !clip || clip->kind != CM_CLIP_EMOTE )
			return NULL;
		*size = sizeof( clip->emote );
		return clip->emote;
	case CM_TYPE_NEWNPC:
		*size = sizeof( cm.newNpc );
		return cm.newNpc;
	case CM_TYPE_TXT:
	case CM_TYPE_TXTWHO:
		if ( cm.txt < 0 || cm.txt >= cm.cine.numTxt )
			return NULL;
		*size = cm.typing == CM_TYPE_TXT ? sizeof( cm.cine.txt[0].text ) : sizeof( cm.cine.txt[0].who );
		return cm.typing == CM_TYPE_TXT ? cm.cine.txt[cm.txt].text : cm.cine.txt[cm.txt].who;
	case CM_TYPE_NPCTYPE:
		if ( !clip || clip->kind != CM_CLIP_SPAWN )
			return NULL;
		*size = sizeof( clip->npcType );
		return clip->npcType;
	case CM_TYPE_DIALOG:
		if ( !clip || clip->kind != CM_CLIP_NPCCMD )
			return NULL;
		*size = sizeof( clip->text );
		return clip->text;
	case CM_TYPE_SAY:
	case CM_TYPE_SAYNAME:
		if ( !clip || clip->kind != CM_CLIP_SAY )
			return NULL;
		*size = cm.typing == CM_TYPE_SAY ? sizeof( clip->text ) : sizeof( clip->name );
		return cm.typing == CM_TYPE_SAY ? clip->text : clip->name;
	default:
		return NULL;
	}
}

// a character the box typed in takes: a line anything, a name shown anything but the | that
// ends it in the file; the rest names, who can be several with commas between
static qboolean CM_AllowChar( int ch ) {
	if ( cm.typing == CM_TYPE_SAY || cm.typing == CM_TYPE_SAYNAME || cm.typing == CM_TYPE_DIALOG || cm.typing == CM_TYPE_TXT || cm.typing == CM_TYPE_TXTWHO )
		return (qboolean)!( ch < ' ' || ch > '~' || ( cm.typing != CM_TYPE_SAY && cm.typing != CM_TYPE_TXT && ch == '|' ) || ( cm.typing == CM_TYPE_DIALOG && ch == ';' ) );
	if ( cm.typing == CM_TYPE_EXTRAMODEL )
		return (qboolean)( CM_NameChar( ch ) || ch == '/' || ch == '|' );
	if ( cm.typing == CM_TYPE_PROPMODEL || cm.typing == CM_TYPE_HOLD )
		return (qboolean)( CM_NameChar( ch ) || ch == '/' || ch == '.' );
	if ( cm.typing == CM_TYPE_LIGHTWHO )
		return (qboolean)( CM_NameChar( ch ) || ch == ',' || ch == '+' );
	if ( cm.typing == CM_TYPE_EXTRANAME )
		return (qboolean)( CM_NameChar( ch ) || ch == '+' );
	return (qboolean)( CM_NameChar( ch ) || ( ( cm.typing == CM_TYPE_WHO || cm.typing == CM_TYPE_FOCUS || cm.typing == CM_TYPE_TRACK ) && ch == ',' ) );
}

// what was typed changed: the animations' names are in capitals, the emotes' small, and their
// lists start again from the top
static void CM_TypedChanged( char *text ) {
	// an extra's name always starts with its +
	if ( cm.typing == CM_TYPE_EXTRANAME && text[0] != '+' ) {
		const int len = (int)strlen( text );

		if ( len < MAX_QPATH - 1 ) {
			memmove( text + 1, text, len + 1 );
			text[0] = '+';
		}
	}
	if ( cm.typing == CM_TYPE_ANIM ) {
		Q_strupr( text );
		cm.pickScroll = 0;
	} else if ( cm.typing == CM_TYPE_EMOTE ) {
		Q_strlwr( text );
		cm.pickScroll = 0;
	}
	CM_Changed();
}

static void CM_AddPlaceKey( cmClip_t *clip );
static void CM_DeletePlaceKey( cmClip_t *clip );

static void CM_DeleteFx( void );
static void CM_DeleteSnd( void );
static void CM_DeleteTxt( void );
static void CM_DeleteWorld( void );

void CL_RpCinematic_EditorKeyEvent( int key, qboolean down ) {
	cmClip_t *clip;

	if ( !cm.open )
		return;
	if ( key >= 0 && key < MAX_KEYS )
		cm.held[key] = down;
	if ( !down ) {
		if ( key == A_MOUSE2 )
			cm.flying = qfalse;
		if ( key == A_MOUSE1 )
			cm.gzDragging = qfalse;
		return;
	}

	if ( cm.preview ) {
		if ( key == A_MOUSE1 || key == A_MOUSE2 || key == A_CAP_P || key == A_LOW_P || key == A_SPACE || key == A_ENTER )
			CM_StopPreview();
		return;
	}
	if ( CM_IsToggleKey( key ) ) {
		CL_RpCinematic_EditorClose();
		return;
	}

	switch ( key ) {
	case A_MOUSE1:
		cm.click = qtrue;
		cm.clickX = cm.cursorX;
		cm.clickY = cm.cursorY;
		return;
	case A_MOUSE2:
		// from the cinematic's camera, the free camera flies on from there
		if ( CM_TlLooking() ) {
			CM_ViewCam( cm.camOrg, cm.camAng, &cm.camFov );
			cm.camAng[PITCH] = Com_Clamp( -89.0f, 89.0f, AngleNormalize180( cm.camAng[PITCH] ) );
			cm.tlCam = qfalse;
		}
		cm.flying = qtrue;
		cm.typing = CM_TYPE_NONE;
		return;
	case A_MWHEELUP:
		if ( cm.flying )
			cm.camFov = Com_Clamp( 10.0f, 130.0f, cm.camFov - ( CM_CtrlDown() ? 1.0f : 5.0f ) );
		else
			cm.wheel--;
		return;
	case A_MWHEELDOWN:
		if ( cm.flying )
			cm.camFov = Com_Clamp( 10.0f, 130.0f, cm.camFov + ( CM_CtrlDown() ? 1.0f : 5.0f ) );
		else
			cm.wheel++;
		return;
	default:
		break;
	}
	if ( cm.flying )
		return;		// flying keys

	if ( cm.typing ) {
		int size;
		char *text = CM_TypedText( &size );

		if ( text ) {
			const int edit = WG_EditKey( text, size, key, CM_AllowChar );

			if ( edit ) {
				if ( edit & WG_EDIT_CHANGED )
					CM_TypedChanged( text );
				return;
			}
		}
		if ( key == A_ENTER || key == A_KP_ENTER ) {
			if ( cm.typing == CM_TYPE_NAME )
				CM_SaveCurrent();
			cm.typing = CM_TYPE_NONE;
		}
		return;
	}

	// Ctrl+Z, Ctrl+Y or Ctrl+Shift+Z, Ctrl+C, Ctrl+V
	if ( CM_CtrlDown() ) {
		switch ( key ) {
		case A_CAP_Z:
		case A_LOW_Z:
			CM_Undo( (qboolean)( cm.held[A_SHIFT] || cm.held[A_SHIFT2] ) );
			return;
		case A_CAP_Y:
		case A_LOW_Y:
			CM_Undo( qtrue );
			return;
		case A_CAP_C:
		case A_LOW_C:
			CM_Copy();
			return;
		case A_CAP_V:
		case A_LOW_V:
			CM_Paste();
			return;
		default:
			break;
		}
	}

	clip = CM_Clip();
	if ( cm.camRec && ( key == A_ENTER || key == A_KP_ENTER ) ) {
		CM_CamRecStop();
		return;
	}
	switch ( key ) {
	case A_ENTER:
	case A_KP_ENTER:
		// a spot: the move's on the Entities tab, the camera's on the Camera tab
		if ( cm.tab == CM_TAB_ENTITIES ) {
			if ( clip && clip->kind == CM_CLIP_MOVE )
				CM_AddMoveSpot();
			else if ( clip && clip->kind == CM_CLIP_PLACE )
				CM_AddPlaceKey( clip );
			else
				CM_Message( S_COLOR_YELLOW "Pick a move to add spots to, or add one: + Add action" );
		} else {
			CM_AddKey();
		}
		break;
	case A_DELETE:
	case A_BACKSPACE:
		if ( cm.tab == CM_TAB_EFFECTS )
			CM_DeleteFx();
		else if ( cm.tab == CM_TAB_SOUNDS )
			CM_DeleteSnd();
		else if ( cm.tab == CM_TAB_TEXT )
			CM_DeleteTxt();
		else if ( cm.tab == CM_TAB_WORLD )
			CM_DeleteWorld();
		else if ( cm.tab == CM_TAB_ENTITIES && clip && clip->kind == CM_CLIP_PLACE )
			CM_DeletePlaceKey( clip );
		else if ( cm.tab == CM_TAB_ENTITIES )
			CM_RemoveMoveSpot();
		else
			CM_DeleteKey();
		break;
	case A_CURSOR_UP:
		CM_SelectKey( cm.sel - 1 );
		break;
	case A_CURSOR_DOWN:
		CM_SelectKey( cm.sel + 1 );
		break;
	case A_CAP_F:
	case A_LOW_F:
		CM_ToggleThrough();
		break;
	case A_CAP_P:
	case A_LOW_P:
	case A_SPACE:
		CM_Preview();
		break;
	case A_CAP_J:
	case A_LOW_J:
		cm.gizmo = (qboolean)!cm.gizmo;
		cm.gzDragging = qfalse;
		CM_Message( cm.gizmo ? "Move arrows on the effect or sound picked: drag X, Y or Z to move it along; J hides them" : "Move arrows hidden" );
		break;
	case A_CAP_T:
	case A_LOW_T:
		cm.tlHidden = (qboolean)!cm.tlHidden;
		if ( cm.tlHidden )
			CM_TlLetGo();
		break;
	default:
		break;
	}
}

void CL_RpCinematic_EditorCharEvent( int ch ) {
	int size;
	char *text;

	if ( !cm.open || cm.flying || !( text = CM_TypedText( &size ) ) )
		return;
	// the animations' names are in capitals, the emotes' small
	if ( cm.typing == CM_TYPE_ANIM )
		ch = toupper( ch );
	else if ( cm.typing == CM_TYPE_EMOTE )
		ch = tolower( ch );
	if ( WG_EditChar( text, size, ch, CM_AllowChar ) )
		CM_TypedChanged( text );
}

void CL_RpCinematic_EditorMouseEvent( int dx, int dy ) {
	if ( !cm.open || cm.preview )
		return;
	if ( cm.flying ) {
		cm.camAng[YAW] -= dx * cl_sensitivity->value * m_yaw->value;
		cm.camAng[PITCH] = Com_Clamp( -89.0f, 89.0f, cm.camAng[PITCH] + dy * cl_sensitivity->value * m_pitch->value );
	} else {
		cm.cursorX = Com_Clamp( 0, SCREEN_WIDTH, cm.cursorX + dx );
		cm.cursorY = Com_Clamp( 0, SCREEN_HEIGHT, cm.cursorY + dy );
	}
}

// escape backs out one level: the preview, the typing, the list, the moment, then the manager
void CL_RpCinematic_EditorEscape( void ) {
	if ( cm.preview ) {
		CM_StopPreview();
	} else if ( cm.typing ) {
		cm.typing = CM_TYPE_NONE;
	} else if ( cm.listing ) {
		cm.listing = qfalse;
	} else if ( cm.tlPinned ) {
		CM_TlLetGo();
	} else {
		const qboolean back = cm.fromStoryteller;

		CL_RpCinematic_EditorClose();
		if ( back )
			CL_Storyteller_Open();
	}
}

// every frame before the cgame draws: flying, and the spot that goes with the camera
static void CM_EditorFrame( void ) {
	vec3_t forward, right, move, end;
	float dt, speed;
	trace_t tr;

	if ( !cm.open )
		return;
	if ( !( Key_GetCatcher() & KEYCATCH_CINEDITOR ) ) {
		// something else cleared the catchers
		CM_StopPreview();
		cm.open = qfalse;
		CM_ReleaseKeys();
		RC_SyncThirdPerson();
		return;
	}

	dt = Com_Clamp( 0.0f, 0.1f, ( cls.realtime - cm.lastFrameTime ) / 1000.0f );
	cm.lastFrameTime = cls.realtime;
	CM_Ripple();

	if ( cm.preview ) {
		// round again from the first mark, at the second or the end
		if ( cm.loop && !cmVideo && ( !cmp.playing || cmp.t >= cm.loopTo ) ) {
			if ( !CM_Play( &cm.cine, cm.loopFrom, qtrue ) )
				cm.preview = qfalse;
			return;
		}
		if ( !cmp.playing )
			cm.preview = qfalse;
		return;
	}

	if ( cm.flying ) {
		AngleVectors( cm.camAng, forward, right, NULL );
		VectorClear( move );
		if ( CM_Held( A_CAP_W ) )	VectorAdd( move, forward, move );
		if ( CM_Held( A_CAP_S ) )	VectorSubtract( move, forward, move );
		if ( CM_Held( A_CAP_D ) )	VectorAdd( move, right, move );
		if ( CM_Held( A_CAP_A ) )	VectorSubtract( move, right, move );
		if ( cm.held[A_SPACE] )		move[2] += 1.0f;
		if ( CM_Held( A_CAP_C ) )	move[2] -= 1.0f;
		speed = CM_FLY_SPEED;
		if ( CM_ShiftDown() )
			speed *= 3.0f;
		if ( CM_CtrlDown() )
			speed *= 0.25f;
		if ( VectorNormalize( move ) > 0.0f )
			VectorMA( cm.camOrg, speed * dt, move, cm.camOrg );
		// roll, and R levels it
		if ( CM_Held( A_CAP_Q ) )
			cm.camAng[ROLL] -= 45.0f * dt;
		if ( CM_Held( A_CAP_E ) )
			cm.camAng[ROLL] += 45.0f * dt;
		if ( CM_Held( A_CAP_R ) )
			cm.camAng[ROLL] = 0;
	}

	if ( cm.through ) {
		rcKey_t *k = CM_SelectedKey();

		if ( !k ) {
			cm.through = qfalse;
		} else if ( !VectorCompare( k->org, cm.camOrg ) || !VectorCompare( k->ang, cm.camAng ) || k->fov != cm.camFov ) {
			CM_KeyFromCamera( k );
			CM_Changed();
		}
	}

	// what the crosshair is on
	AngleVectors( cm.camAng, forward, NULL, NULL );
	VectorMA( cm.camOrg, CM_TRACE_DIST, forward, end );
	CM_BoxTrace( &tr, cm.camOrg, end, vec3_origin, vec3_origin, 0, CONTENTS_SOLID | CONTENTS_TERRAIN, qfalse );
	cm.haveAim = (qboolean)( tr.fraction < 1.0f && !tr.allsolid );
	VectorCopy( tr.endpos, cm.aim );
}

/*
===============================================================================

The manager's scene: the camera's trails, the moves, and what the cameras look at

===============================================================================
*/

static void CM_Line( const vec3_t a, const vec3_t b, const byte *color ) {
	refEntity_t ent;
	vec3_t mid, toCam;

	if ( cm.numLines >= CM_MAX_LINES )
		return;
	cm.numLines++;
	VectorAdd( a, b, mid );
	VectorScale( mid, 0.5f, mid );
	VectorSubtract( mid, cm.camOrg, toCam );

	memset( &ent, 0, sizeof( ent ) );
	ent.reType = RT_LINE;
	VectorCopy( a, ent.origin );
	VectorCopy( b, ent.oldorigin );
	ent.radius = 0.3f + VectorLength( toCam ) * 0.0015f;
	ent.customShader = cls.whiteShader;
	memcpy( ent.shaderRGBA, color, sizeof( ent.shaderRGBA ) );
	ent.renderfx = RF_NOSHADOW;
	re->AddRefEntityToScene( &ent );
}

static void CM_Cross( const vec3_t p, float size, const byte *color ) {
	for ( int i = 0; i < 3; i++ ) {
		vec3_t a, b;

		VectorCopy( p, a );
		VectorCopy( p, b );
		a[i] -= size;
		b[i] += size;
		CM_Line( a, b, color );
	}
}

// a camera: its frame as the screen is shaped, lines to it, and a mark on top
static void CM_Camera( const vec3_t org, const vec3_t ang, float fov, const byte *color ) {
	const float fovY = RC_FovY( fov );
	const float aspect = cls.glconfig.vidHeight ? (float)cls.glconfig.vidWidth / cls.glconfig.vidHeight : 4.0f / 3.0f;
	const float h = CM_FRUSTUM * tanf( DEG2RAD( fovY * 0.5f ) ), w = h * aspect;
	vec3_t axis[3], center, c[4], top;

	AnglesToAxis( ang, axis );
	VectorMA( org, CM_FRUSTUM, axis[0], center );
	for ( int i = 0; i < 4; i++ ) {
		VectorMA( center, ( i == 0 || i == 3 ) ? w : -w, axis[1], c[i] );
		VectorMA( c[i], i < 2 ? h : -h, axis[2], c[i] );
	}
	for ( int i = 0; i < 4; i++ ) {
		CM_Line( org, c[i], color );
		CM_Line( c[i], c[( i + 1 ) & 3], color );
	}
	VectorMA( center, h * 1.5f, axis[2], top );
	CM_Line( c[0], top, color );
	CM_Line( c[1], top, color );
}

// where a camera looks to, as far as the first wall
static void CM_Ray( const vec3_t org, const vec3_t ang, const byte *color ) {
	vec3_t forward, end;
	trace_t tr;

	AngleVectors( ang, forward, NULL, NULL );
	VectorMA( org, CM_TRACE_DIST, forward, end );
	CM_BoxTrace( &tr, org, end, vec3_origin, vec3_origin, 0, CONTENTS_SOLID | CONTENTS_TERRAIN, qfalse );
	if ( tr.allsolid )
		return;
	VectorMA( org, CM_FRUSTUM, forward, end );
	CM_Line( end, tr.endpos, color );
	if ( tr.fraction < 1.0f )
		CM_Cross( tr.endpos, 6.0f, color );
}

// a shot's trail, a tick each second along it
static void CM_Trail( const rcShot_t *s, const byte *color, qboolean ticks ) {
	const int total = RC_ShotMs( s );
	vec3_t org, ang, prev;
	float fov;
	int samples;

	if ( !s->orbit && s->numKeys < 2 )
		return;
	samples = Com_Clampi( 24, 240, total / 40 );
	RC_EvalShot( s, 0, prev, ang, &fov );
	for ( int i = 1; i <= samples; i++ ) {
		RC_EvalShot( s, total * i / samples, org, ang, &fov );
		CM_Line( prev, org, color );
		VectorCopy( org, prev );
	}
	for ( int ms = 1000; ticks && ms < total; ms += 1000 ) {
		vec3_t a, b;

		RC_EvalShot( s, ms, org, ang, &fov );
		VectorCopy( org, a );
		VectorCopy( org, b );
		a[2] -= 3.0f;
		b[2] += 3.0f;
		CM_Line( a, b, color );
	}
}

static void CM_AddScene( void ) {
	const rcShot_t *s = CM_Shot();
	const int total = CM_Total( &cm.cine );
	vec3_t org, ang;
	float fov;

	cm.numLines = 0;
	if ( cm.haveAim && !cm.flying )
		CM_Cross( cm.aim, 4.0f, cmAimColor );
	CM_EditPlans( qfalse );

	// the moves: from where each starts through its spots
	for ( int i = 0; i < cm.cine.numClips; i++ ) {
		const cmClip_t *clip = &cm.cine.clips[i];
		const cmPlan_t *p = &cmEditPlans[i];
		const qboolean sel = (qboolean)( i == cm.clip && cm.tab == CM_TAB_ENTITIES );
		const byte *color = sel ? cmSelColor : clip->ent == cm.ent ? cmMoveColor : cmMoveFaint;

		if ( clip->kind != CM_CLIP_MOVE )
			continue;
		if ( p->count ) {
			for ( int m = 0; m < p->count; m++ ) {
				for ( int j = 1; j < p->numPts; j++ )
					CM_Line( p->pts[m][j - 1], p->pts[m][j], color );
			}
		} else {
			for ( int j = 1; j < clip->numPts; j++ )
				CM_Line( clip->pts[j - 1], clip->pts[j], color );
		}
		for ( int j = 0; j < clip->numPts; j++ )
			CM_Cross( clip->pts[j], sel && j == cm.spot ? 8.0f : sel ? 5.0f : 3.0f, sel && j == cm.spot ? cmAimColor : color );
	}

	// the positions: from key to key, a tick the way each faces; the recordings: where they go
	for ( int i = 0; i < cm.cine.numClips; i++ ) {
		const cmClip_t *clip = &cm.cine.clips[i];
		const cmPlan_t *p = &cmEditPlans[i];
		const qboolean sel = (qboolean)( i == cm.clip && cm.tab == CM_TAB_ENTITIES );
		static const byte place[4] = { 60, 200, 210, 255 }, placeFaint[4] = { 60, 200, 210, 110 };
		static const byte recColor[4] = { 230, 70, 70, 255 }, recFaint[4] = { 230, 70, 70, 110 };

		if ( clip->kind == CM_CLIP_SPAWN ) {
			static const byte spawn[4] = { 90, 115, 190, 255 }, spawnFaint[4] = { 90, 115, 190, 110 };
			const byte *color = sel ? cmSelColor : clip->ent == cm.ent ? spawn : spawnFaint;
			vec3_t forward, tip, top;

			CM_Cross( clip->keys[0].org, sel ? 10.0f : 6.0f, color );
			AngleVectors( clip->keys[0].ang, forward, NULL, NULL );
			VectorMA( clip->keys[0].org, 24.0f, forward, tip );
			CM_Line( clip->keys[0].org, tip, color );
			VectorCopy( clip->keys[0].org, top );
			top[2] += 64.0f;
			CM_Line( clip->keys[0].org, top, color );
			continue;
		}
		if ( clip->kind == CM_CLIP_PLACE ) {
			const byte *color = sel ? cmSelColor : clip->ent == cm.ent ? place : placeFaint;

			for ( int j = 0; j < clip->numKeys; j++ ) {
				vec3_t forward, tip;

				if ( j )
					CM_Line( clip->keys[j - 1].org, clip->keys[j].org, color );
				CM_Cross( clip->keys[j].org, sel && j == cm.spot ? 8.0f : 4.0f, sel && j == cm.spot ? cmAimColor : color );
				AngleVectors( clip->keys[j].ang, forward, NULL, NULL );
				VectorMA( clip->keys[j].org, 20.0f, forward, tip );
				CM_Line( clip->keys[j].org, tip, color );
			}
		} else if ( clip->kind == CM_CLIP_RECORD && p->count ) {
			const cmRec_t *r = CM_Rec( &cm.cine, clip );
			const byte *color = sel ? cmSelColor : clip->ent == cm.ent ? recColor : recFaint;

			for ( int j = 1; r && j < r->count; j++ ) {
				vec3_t a, b;

				VectorAdd( p->pts[0][0], r->s[j - 1].org, a );
				VectorAdd( p->pts[0][0], r->s[j].org, b );
				CM_Line( a, b, color );
			}
		}
	}

	// the effects, the one picked brighter
	for ( int i = 0; i < cm.cine.numFx; i++ ) {
		static const byte fxColor[4] = { 255, 140, 60, 255 }, fxFaint[4] = { 255, 140, 60, 110 };
		const qboolean sel = (qboolean)( i == cm.fx && cm.tab == CM_TAB_EFFECTS );

		CM_Cross( cm.cine.fx[i].org, sel ? 8.0f : 4.0f, sel ? cmSelColor : cm.tab == CM_TAB_EFFECTS ? fxColor : fxFaint );
	}
	// the sounds heard from a spot
	for ( int i = 0; i < cm.cine.numSnd; i++ ) {
		static const byte sndColor[4] = { 120, 200, 255, 255 }, sndFaint[4] = { 120, 200, 255, 110 };
		const qboolean sel = (qboolean)( i == cm.snd && cm.tab == CM_TAB_SOUNDS );

		if ( cm.cine.snd[i].spot )
			CM_Cross( cm.cine.snd[i].org, sel ? 8.0f : 4.0f, sel ? cmSelColor : cm.tab == CM_TAB_SOUNDS ? sndColor : sndFaint );
	}
	// the props' paths through their keys, the one picked brighter; the lights, in their colour
	if ( cm.tab == CM_TAB_WORLD ) {
		static const byte propColor[4] = { 230, 200, 120, 255 }, propFaint[4] = { 230, 200, 120, 90 };

		for ( int i = 0; i < cm.cine.numProps && cm.worldPage == 0; i++ ) {
			const cmProp_t *pr = &cm.cine.props[i];
			const qboolean sel = (qboolean)( i == cm.prop );

			for ( int k = 0; k < pr->numKeys; k++ ) {
				CM_Cross( pr->keys[k].org, sel && k == cm.propKey ? 8.0f : 4.0f, sel && k == cm.propKey ? cmSelColor : sel ? propColor : propFaint );
				if ( k )
					CM_Line( pr->keys[k - 1].org, pr->keys[k].org, sel ? propColor : propFaint );
			}
		}
		for ( int i = 0; i < cm.cine.numLights && cm.worldPage == 1; i++ ) {
			const cmLight_t *l = &cm.cine.lights[i];
			const byte color[4] = { (byte)( l->color[0] * 255 ), (byte)( l->color[1] * 255 ), (byte)( l->color[2] * 255 ), 255 };
			vec3_t org;

			CM_LightOrg( l, org );
			CM_Cross( org, i == cm.light ? 8.0f : 5.0f, i == cm.light ? cmSelColor : color );
		}
	}

	// the shots' trails, the one shown brighter
	for ( int i = 0; i < cm.cine.numShots; i++ )
		CM_Trail( &cm.cine.shots[i], i == cm.cur ? cmTrailColor : cmFaintTrail, (qboolean)( i == cm.cur ) );

	if ( s && s->orbit ) {
		// the whole circle faintly, the centre, and where it starts
		for ( int i = 0; i < 48; i++ ) {
			vec3_t a, b;
			const float y0 = DEG2RAD( i * 7.5f ), y1 = DEG2RAD( ( i + 1 ) * 7.5f );

			VectorSet( a, s->center[0] + cosf( y0 ) * s->radius, s->center[1] + sinf( y0 ) * s->radius, s->center[2] + s->height );
			VectorSet( b, s->center[0] + cosf( y1 ) * s->radius, s->center[1] + sinf( y1 ) * s->radius, s->center[2] + s->height );
			CM_Line( a, b, cmCircleColor );
		}
		CM_Cross( s->center, 10.0f, cmLookColor );
		RC_EvalShot( s, 0, org, ang, &fov );
		CM_Camera( org, ang, fov, cmSelColor );
		CM_Line( org, s->center, cmRayColor );
	} else if ( s ) {
		for ( int i = 0; i < s->numKeys; i++ ) {
			const rcKey_t *k = &s->keys[i];
			vec3_t look;

			if ( ( cm.through && i == cm.sel ) || Distance( k->org, cm.camOrg ) < CM_FRUSTUM )
				continue;	// the camera is in it
			VectorCopy( k->ang, look );
			if ( s->look ) {
				vec3_t dir;

				VectorSubtract( s->lookAt, k->org, dir );
				vectoangles( dir, look );
				look[ROLL] = k->ang[ROLL];
			}
			CM_Camera( k->org, look, k->fov, i == cm.sel ? cmSelColor : cmKeyColor );
			if ( i == cm.sel ) {
				if ( s->look )
					CM_Line( k->org, s->lookAt, cmRayColor );
				else
					CM_Ray( k->org, look, cmRayColor );
			}
		}
		if ( s->look )
			CM_Cross( s->lookAt, 10.0f, cmLookColor );
	}

	// a camera running along the cinematic, as fast as it will; held at the timeline's moment
	if ( total > 0 ) {
		int local;
		const int ms = cm.tlPinned ? cm.tlMs : cls.realtime % ( total + 1000 );
		const int shot = CM_ShotAt( &cm.cine, Q_min( ms, total ), &local );

		RC_EvalShotLive( &cm.cine.shots[shot], local, org, ang, &fov, qfalse );
		CM_Camera( org, ang, fov, cmGhostColor );
	}

	// where the moment has everyone moving: a post over each, seen even when they aren't
	if ( cm.tlPinned ) {
		for ( int i = 0; i < cm.cine.numClips; i++ ) {
			const cmPlan_t *p = &cmEditPlans[i];

			for ( int m = 0; cm.cine.clips[i].kind == CM_CLIP_MOVE && m < p->count; m++ ) {
				vec3_t pos, top;

				if ( !CL_NpcWalk_PoseOf( p->nums[m], pos ) )
					continue;
				VectorCopy( pos, top );
				pos[2] -= 24.0f;
				top[2] += 48.0f;
				CM_Line( pos, top, cmMoveColor );
				CM_Cross( top, 4.0f, cmMoveColor );
			}
		}
	}
}

static qboolean CM_FocusMoves( const rcShot_t *s );
static int CM_FocusMs( const rcShot_t *s );

void CM_EditorRenderScene( const refdef_t *fd ) {
	refdef_t view = *fd;
	vec3_t org, ang;
	float fov, fovY;
	const qboolean looking = CM_ViewCam( org, ang, &fov );
	qboolean dof = qfalse, filter = qfalse;

	fovY = RC_FovY( fov );
	view.fov_y = fovY;
	view.fov_x = RC_FovX( fovY, (float)fd->width, (float)fd->height );
	VectorCopy( ang, view.viewangles );
	AnglesToAxis( ang, view.viewaxis );
	// the sky portal keeps its own origin but looks the way the camera does
	if ( fd->rdflags & RDF_SKYBOXPORTAL ) {
		re->RenderScene( &view );
		return;
	}
	VectorCopy( org, view.vieworg );
	view.viewContents = CM_PointContents( org, 0 );
	memset( view.areamask, 0, sizeof( view.areamask ) );
	cm.haveView = qtrue;
	VectorCopy( view.vieworg, cm.viewOrg );
	VectorCopy( view.viewaxis[0], cm.viewAxis[0] );
	VectorCopy( view.viewaxis[1], cm.viewAxis[1] );
	VectorCopy( view.viewaxis[2], cm.viewAxis[2] );
	cm.viewFovX = view.fov_x;
	cm.viewFovY = view.fov_y;

	if ( looking ) {
		// through the cinematic's camera it's as the players will see it, nothing of the manager's
		int local;
		const rcShot_t *s = &cm.cine.shots[CM_ShotAt( &cm.cine, cm.tlMs, &local )];

		dof = RC_ShotDepthOfField( s, local, org, ang, &cm.dofNow, &cm.dofTime );
		filter = RC_Filter( s->filter, s->filterStrength );
	} else {
		const rcShot_t *s = CM_Shot();

		CM_AddScene();
		if ( s && cm.tab == CM_TAB_CAMERA && !cm.listing ) {
			const int at = cm.sel >= 0 ? cm.sel : 0;

			if ( ( cm.page == CM_PAGE_FOCUS || cm.through ) && CM_FocusMoves( s ) )
				dof = RC_ShotDepthOfField( s, CM_FocusMs( s ), cm.camOrg, cm.camAng, &cm.dofNow, &cm.dofTime );
			else if ( cm.page == CM_PAGE_FOCUS || cm.through )
				dof = RC_DepthOfField( s, at, at, 0.0f, cm.camOrg, cm.camAng, &cm.dofNow, &cm.dofTime );
			if ( cm.page == CM_PAGE_LOOK || cm.through )
				filter = RC_Filter( s->filter, s->filterStrength );
		}
	}
	re->RenderScene( &view );
	if ( dof )
		RC_NoDepthOfField();
	if ( filter )
		RC_NoFilter();
}

qboolean CM_EditorCamera( vec3_t org, vec3_t ang, float *fov ) {
	if ( !cm.open )
		return qfalse;
	CM_ViewCam( org, ang, fov );
	return qtrue;
}

// the filter over the picture now, for the thermal look
int CM_EditorFilter( void ) {
	const rcShot_t *s;

	if ( CM_TlLooking() ) {
		int local;

		s = &cm.cine.shots[CM_ShotAt( &cm.cine, cm.tlMs, &local )];
		return s->filterStrength > 0 ? s->filter : RC_FI_NONE;
	}
	s = CM_Shot();
	if ( s && cm.tab == CM_TAB_CAMERA && ( cm.page == CM_PAGE_LOOK || cm.through ) && !cm.listing && s->filterStrength > 0 )
		return s->filter;
	return RC_FI_NONE;
}

/*
===============================================================================

The panel's controls

===============================================================================
*/

static void CM_Fill( float x, float y, float w, float h, const float *color ) {
	re->SetColor( color );
	re->DrawStretchPic( x, y, w, h, 0, 0, 0, 0, cls.whiteShader );
	re->SetColor( NULL );
}

static void CM_Box( float x, float y, float w, float h, const float *fill ) {
	CM_Fill( x, y, w, h, fill );
	CM_Fill( x, y, w, 1, cmBorder );
	CM_Fill( x, y + h - 1, w, 1, cmBorder );
	CM_Fill( x, y, 1, h, cmBorder );
	CM_Fill( x + w - 1, y, 1, h, cmBorder );
}

static float CM_TextWidth( const char *text ) {
	return (float)re->Font_StrLenPixels( text, cm.font, CM_TEXT_SCALE );
}

static void CM_Text( float x, float y, const char *text, const float *color ) {
	re->Font_DrawString( (int)x, (int)y, text, color, cm.font | STYLE_DROPSHADOW, -1, CM_TEXT_SCALE );
}

static void CM_TextClipped( float x, float y, float w, const char *text, const float *color ) {
	char buf[MAX_STRING_CHARS];
	int len;

	Q_strncpyz( buf, text, sizeof( buf ) );
	len = strlen( buf );
	while ( len > 3 && CM_TextWidth( buf ) > w ) {
		len--;
		buf[len - 3] = '.';
		buf[len - 2] = '.';
		buf[len - 1] = '.';
		buf[len] = '\0';
	}
	CM_Text( x, y, buf, color );
}

static qboolean CM_InRect( float x, float y, float w, float h ) {
	return (qboolean)( !cm.flying && !cm.popHover && cm.cursorX >= x && cm.cursorX < x + w && cm.cursorY >= y && cm.cursorY < y + h );
}

// eats this frame's click if it landed in the rect
static qboolean CM_Clicked( float x, float y, float w, float h ) {
	if ( !cm.click || cm.clickX < x || cm.clickX >= x + w || cm.clickY < y || cm.clickY >= y + h )
		return qfalse;
	cm.click = qfalse;
	return qtrue;
}

static qboolean CM_Button( float x, float y, float w, const char *label, qboolean selected, qboolean enabled ) {
	const float *fill = selected ? cmHighlight : ( enabled && CM_InRect( x, y, w, CM_CTRL_H ) ) ? cmHoverBox : cmPanelLight;

	CM_Box( x, y, w, CM_CTRL_H, fill );
	CM_Text( x + ( w - CM_TextWidth( label ) ) * 0.5f, y + 2, label, enabled ? cmWhite : cmDim );
	return (qboolean)( CM_Clicked( x, y, w, CM_CTRL_H ) && enabled );
}

// a setting that's one of two: its name, then the value, a click for the other
static qboolean CM_Switch( float x, float y, float w, const char *label, const char *value ) {
	return CM_Button( x, y, w, va( "%s" S_COLOR_GREY ": " S_COLOR_WHITE "%s", label, value ), qfalse, qtrue );
}

// a value with - and + either side, the wheel over it too; how many steps it went
static int CM_Stepper( float x, float y, float w, const char *label, const char *value ) {
	const float lw = 62.0f, vx = x + lw + 17, vw = w - lw - 34;
	int steps = 0;

	CM_Text( x, y + 2, label, cmDim );
	if ( CM_Button( x + lw, y, 16, "-", qfalse, qtrue ) )
		steps--;
	CM_Box( vx, y, vw, CM_CTRL_H, cmPanelLight );
	CM_Text( vx + ( vw - CM_TextWidth( value ) ) * 0.5f, y + 2, value, cmWhite );
	if ( CM_Button( x + w - 16, y, 16, "+", qfalse, qtrue ) )
		steps++;
	if ( cm.wheel && CM_InRect( x, y, w, CM_CTRL_H ) ) {
		steps -= cm.wheel;
		cm.wheel = 0;
	}
	return steps;
}

// Shift goes in big steps, Ctrl in small ones
static float CM_Step( float step ) {
	if ( CM_ShiftDown() )
		return step * 5.0f;
	if ( CM_CtrlDown() )
		return step * 0.2f;
	return step;
}

static void CM_Adjust( float *value, int steps, float step, float min, float max ) {
	if ( !steps )
		return;
	*value = Com_Clamp( min, max, *value + steps * CM_Step( step ) );
	CM_Changed();
}

static void CM_AdjustMs( int *ms, int steps, float seconds, int min ) {
	if ( !steps )
		return;
	*ms = Q_max( min, RC_SecondsMs( RC_Seconds( *ms ) + steps * CM_Step( seconds ) ) );
	CM_Changed();
}

static vec4_t cmUnknown		= { 1.0f, 1.0f, 0.0f, 1.0f };

static void CM_FieldStyle( wgStyle_t *style, const float *color ) {
	style->font = cm.font;
	style->scale = CM_TEXT_SCALE;
	style->lineH = CM_ROW_H;
	style->padX = 4.0f;
	style->padY = 2.0f;
	style->fill = cmFocusBox;
	style->border = cmBorder;
	style->text = color ? color : cmWhite;
	style->dim = cmDim;
}

// a box typed in, its text in color (NULL white); a click starts typing in it, the cursor where
// it was clicked. Its height now: grown down while it's typed in and long
static float CM_TextBox( float x, float y, float w, char *text, const char *empty, int typing, const float *color ) {
	const qboolean on = (qboolean)( cm.typing == typing );
	wgStyle_t style;
	float h;

	CM_FieldStyle( &style, color );
	CM_Box( x, y, w, CM_CTRL_H, on ? cmFocusBox : cmPanelLight );
	WG_Field( &style, x, y, w, CM_CTRL_H, text, on, empty, cm.cursorX, cm.cursorY );
	h = WG_FieldHeight( &style, w, CM_CTRL_H, text, on );
	if ( CM_Clicked( x, y, w, h ) ) {
		if ( !on )
			cm.pickScroll = 0;
		cm.typing = typing;
		WG_FieldClick( text, cm.clickX, cm.clickY );
	}
	return h;
}

// a list's scrollbar along its right side (the box x, y, w, h) when there's more than fits;
// the rows' width beside it
static float CM_ListBar( float x, float y, float w, float h, int count, int rows, int *scroll ) {
	if ( count <= rows )
		return w;
	WG_Scrollbar( x + w - WG_SCROLL_W - 1, y + 1, h - 2, count, rows, scroll, cm.cursorX, cm.cursorY, &cm.click, cm.clickX, cm.clickY );
	return w - WG_SCROLL_W - 1;
}

// tabs, joined, the one open lit with a line under them all
static int CM_Tabs( float x, float y, float w, const char **labels, int count, int open ) {
	const float tw = w / count;
	int picked = open;

	for ( int i = 0; i < count; i++ ) {
		const float tx = x + i * tw;
		const qboolean on = (qboolean)( i == open );

		CM_Fill( tx, y, tw - 1, CM_CTRL_H, on ? cmHighlight : CM_InRect( tx, y, tw, CM_CTRL_H ) ? cmHoverBox : cmPanelLight );
		CM_Text( tx + ( tw - CM_TextWidth( labels[i] ) ) * 0.5f, y + 2, labels[i], on ? cmWhite : cmDim );
		if ( CM_Clicked( tx, y, tw, CM_CTRL_H ) )
			picked = i;
	}
	CM_Fill( x, y + CM_CTRL_H, w, 2, cmTabLine );
	return picked;
}

/*
===============================================================================

The Camera tab

===============================================================================
*/

static void CM_DrawSpotsPage( float x, float y, float w ) {
	rcShot_t *s = CM_Shot();
	const float half = ( w - 4 ) * 0.5f;
	rcKey_t *k;

	// looking at a point the whole way
	CM_Text( x, y + 2, "Look at", cmDim );
	if ( CM_Button( x + 62, y, 36, s->look ? "on" : "off", s->look, qtrue ) ) {
		s->look = (qboolean)!s->look;
		if ( s->look && VectorCompare( s->lookAt, vec3_origin ) && cm.haveAim )
			VectorCopy( cm.aim, s->lookAt );
		CM_Changed();
	}
	if ( CM_Button( x + 102, y, w - 102, "Set to crosshair", qfalse, cm.haveAim ) ) {
		VectorCopy( cm.aim, s->lookAt );
		s->look = qtrue;
		CM_Changed();
	}
	y += CM_CTRL_H + CM_GAP + 2;

	CM_Text( x, y, va( "Spots (%i)" S_COLOR_GREY "   %s", s->numKeys, s->numKeys == 1 ? "held still" : "moves through them" ), cmWhite );
	y += CM_ROW_H + 2;
	if ( cm.wheel && CM_InRect( x, y, w, CM_KEY_ROWS * CM_ROW_H ) ) {
		cm.keyScroll += cm.wheel;
		cm.wheel = 0;
	}
	cm.keyScroll = Com_Clampi( 0, Q_max( 0, s->numKeys - CM_KEY_ROWS ), cm.keyScroll );
	CM_Box( x, y, w, CM_KEY_ROWS * CM_ROW_H + 2, cmPanelLight );
	const float lw = CM_ListBar( x, y, w, CM_KEY_ROWS * CM_ROW_H + 2, s->numKeys, CM_KEY_ROWS, &cm.keyScroll );
	for ( int i = 0; i < CM_KEY_ROWS && cm.keyScroll + i < s->numKeys; i++ ) {
		const int index = cm.keyScroll + i;
		const rcKey_t *key = &s->keys[index];
		const float ry = y + 1 + i * CM_ROW_H;
		char row[64];

		if ( index == cm.sel )
			CM_Fill( x + 1, ry, lw - 2, CM_ROW_H, cmHighlight );
		else if ( CM_InRect( x, ry, lw, CM_ROW_H ) )
			CM_Fill( x + 1, ry, lw - 2, CM_ROW_H, cmHoverBox );
		if ( index )
			Com_sprintf( row, sizeof( row ), "%i   after %.1f s", index + 1, RC_Seconds( key->ms ) );
		else
			Com_sprintf( row, sizeof( row ), "1   start" );
		CM_Text( x + 4, ry, row, cmWhite );
		Com_sprintf( row, sizeof( row ), "fov %i%s", RC_Round( key->fov ),
			RC_Round( AngleNormalize180( key->ang[ROLL] ) ) ? va( "  roll %i", RC_Round( AngleNormalize180( key->ang[ROLL] ) ) ) : "" );
		CM_Text( x + lw - 4 - CM_TextWidth( row ), ry, row, cmDim );
		if ( CM_Clicked( x, ry, lw, CM_ROW_H ) )
			CM_SelectKey( index );
	}
	y += CM_KEY_ROWS * CM_ROW_H + 6;

	if ( CM_Button( x, y, half, "Add spot (Enter)", qfalse, (qboolean)( s->numKeys < RC_MAX_KEYS ) ) )
		CM_AddKey();
	k = CM_SelectedKey();
	if ( CM_Button( x + half + 4, y, half, "Move spot here", qfalse, (qboolean)( k && !cm.through ) ) )
		CM_SetKeyHere();
	y += CM_CTRL_H + CM_GAP;
	if ( CM_Button( x, y, half, "Delete spot", qfalse, (qboolean)( k != NULL ) ) )
		CM_DeleteKey();
	if ( CM_Button( x + half + 4, y, half, cm.through ? "Through it (F)" : "Look through (F)", cm.through, (qboolean)( k != NULL ) ) )
		CM_ToggleThrough();
	y += CM_CTRL_H + CM_GAP + 2;

	if ( !k )
		return;
	if ( s->numKeys == 1 ) {
		const int steps = CM_Stepper( x, y, w, "Held for", va( "%.1f s", RC_Seconds( s->ms ) ) );

		CM_AdjustMs( &s->ms, steps, 0.5f, 100 );
	} else if ( cm.sel ) {
		const int steps = CM_Stepper( x, y, w, "Seconds", va( "%.1f after", RC_Seconds( k->ms ) ) );

		CM_AdjustMs( &k->ms, steps, 0.5f, 100 );
	} else {
		CM_Text( x, y + 2, S_COLOR_GREY "The shot starts here", cmWhite );
	}
	y += CM_CTRL_H + CM_GAP;
	{
		const int steps = CM_Stepper( x, y, w, "FOV", va( "%i", RC_Round( k->fov ) ) );

		CM_Adjust( &k->fov, steps, 5.0f, 10.0f, 130.0f );
		if ( steps && cm.through )
			cm.camFov = k->fov;
	}
	y += CM_CTRL_H + CM_GAP;
	{
		float roll = AngleNormalize180( k->ang[ROLL] );
		const int steps = CM_Stepper( x, y, w, "Roll", va( "%i", RC_Round( roll ) ) );

		if ( steps ) {
			CM_Adjust( &roll, steps, 5.0f, -180.0f, 180.0f );
			k->ang[ROLL] = roll;
			if ( cm.through )
				cm.camAng[ROLL] = roll;
		}
	}
	// slowing into it and out of it, as a camera that stops there
	if ( s->numKeys > 1 ) {
		y += CM_CTRL_H + CM_GAP;
		if ( CM_Switch( x, y, w, "Stops here", k->stops ? "yes, eases in and out" : "no, goes through" ) ) {
			k->stops = (qboolean)!k->stops;
			CM_Changed();
		}
	}
}

static void CM_DrawOrbitPage( float x, float y, float w ) {
	rcShot_t *s = CM_Shot();
	int steps;

	CM_Text( x, y + 2, "Round", cmDim );
	if ( CM_Button( x + 62, y, w - 62, "Set to crosshair", qfalse, cm.haveAim ) ) {
		VectorCopy( cm.aim, s->center );
		CM_Changed();
	}
	y += CM_CTRL_H + CM_GAP;
	steps = CM_Stepper( x, y, w, "Seconds", va( "%.1f", RC_Seconds( s->ms ) ) );
	CM_AdjustMs( &s->ms, steps, 1.0f, 100 );
	y += CM_CTRL_H + CM_GAP;
	steps = CM_Stepper( x, y, w, "Degrees", va( "%i", RC_Round( s->degrees ) ) );
	CM_Adjust( &s->degrees, steps, 45.0f, -3600.0f, 3600.0f );
	y += CM_CTRL_H + CM_GAP;
	steps = CM_Stepper( x, y, w, "Radius", va( "%i", RC_Round( s->radius ) ) );
	CM_Adjust( &s->radius, steps, 16.0f, 16.0f, 8192.0f );
	y += CM_CTRL_H + CM_GAP;
	steps = CM_Stepper( x, y, w, "Height", va( "%i", RC_Round( s->height ) ) );
	CM_Adjust( &s->height, steps, 16.0f, -4096.0f, 4096.0f );
	y += CM_CTRL_H + CM_GAP;
	steps = CM_Stepper( x, y, w, "From yaw", va( "%i", RC_Round( AngleNormalize360( s->fromYaw ) ) ) );
	CM_Adjust( &s->fromYaw, steps, 15.0f, -3600.0f, 3600.0f );
	y += CM_CTRL_H + CM_GAP;
	steps = CM_Stepper( x, y, w, "Start FOV", va( "%i", RC_Round( s->fov ) ) );
	CM_Adjust( &s->fov, steps, 5.0f, 10.0f, 130.0f );
	y += CM_CTRL_H + CM_GAP;
	steps = CM_Stepper( x, y, w, "End FOV", va( "%i", RC_Round( s->endFov ) ) );
	CM_Adjust( &s->endFov, steps, 5.0f, 10.0f, 130.0f );
	y += CM_CTRL_H + CM_GAP + 2;
	if ( CM_Button( x, y, w, "Start from the camera", qfalse, qtrue ) )
		CM_OrbitFromHere();
}

static const char *cmTransNames[RC_NUM_TRANSITIONS] = { "Cut", "Fade", "Wipe from the left", "Wipe from the right", "Wipe from the top", "Wipe from the bottom" };

static const struct {
	const char	*name;
	byte		rgb[3];
} cmTransColors[] = {
	{ "Black", { 0, 0, 0 } },
	{ "White", { 255, 255, 255 } },
	{ "Red", { 190, 25, 25 } },
	{ "Orange", { 235, 130, 20 } },
	{ "Yellow", { 240, 215, 40 } },
	{ "Green", { 35, 170, 60 } },
	{ "Cyan", { 40, 200, 220 } },
	{ "Blue", { 30, 70, 210 } },
	{ "Purple", { 140, 50, 190 } }
};

// a transition's kind, length and colour; where the next row goes
static float CM_DrawTransition( float x, float y, float w, const char *label, rcTrans_t *tr ) {
	int color = -1, steps;

	CM_Text( x, y + 2, label, cmDim );
	if ( CM_Button( x + 62, y, w - 62, cmTransNames[tr->type], (qboolean)( tr->type != RC_TR_CUT ), qtrue ) ) {
		tr->type = ( tr->type + 1 ) % RC_NUM_TRANSITIONS;
		if ( tr->type != RC_TR_CUT && !tr->ms )
			tr->ms = RC_TRANS_MS;
		CM_Changed();
	}
	y += CM_CTRL_H + CM_GAP;
	if ( tr->type == RC_TR_CUT )
		return y;
	steps = CM_Stepper( x, y, w, "Seconds", va( "%.1f", RC_Seconds( tr->ms ) ) );
	CM_AdjustMs( &tr->ms, steps, 0.25f, 100 );
	y += CM_CTRL_H + CM_GAP;
	for ( int i = 0; i < (int)ARRAY_LEN( cmTransColors ); i++ ) {
		if ( !memcmp( cmTransColors[i].rgb, tr->color, 3 ) )
			color = i;
	}
	CM_Text( x, y + 2, "Color", cmDim );
	{
		const vec4_t swatch = { tr->color[0] / 255.0f, tr->color[1] / 255.0f, tr->color[2] / 255.0f, 1.0f };

		CM_Box( x + 62, y, 18, CM_CTRL_H, cmBorder );
		CM_Fill( x + 63, y + 1, 16, CM_CTRL_H - 2, swatch );
	}
	if ( CM_Button( x + 84, y, w - 84, color >= 0 ? cmTransColors[color].name : va( "%i %i %i", tr->color[0], tr->color[1], tr->color[2] ), qfalse, qtrue ) ) {
		color = ( color + 1 ) % ARRAY_LEN( cmTransColors );
		memcpy( tr->color, cmTransColors[color].rgb, 3 );
		CM_Changed();
	}
	return y + CM_CTRL_H + CM_GAP;
}

static qboolean CM_FocusMoves( const rcShot_t *s ) {
	return (qboolean)( s && s->dof && s->dofFocus == RC_DOF_POINT );
}

// the moment a moving focus is worked on, into the shot shown: the timeline's while it's on the
// shot, the key picked's otherwise
static int CM_FocusMs( const rcShot_t *s ) {
	if ( cm.tlPinned && cm.cine.numShots ) {
		int local;

		if ( CM_ShotAt( &cm.cine, cm.tlMs, &local ) == cm.cur )
			return local;
	}
	if ( cm.focusSel >= 0 && cm.focusSel < s->numFocus )
		return s->focus[cm.focusSel].ms;
	return 0;
}

// the timeline at a key of the shot shown
static void CM_FocusSeeKey( const rcShot_t *s, int k ) {
	if ( k < 0 || k >= s->numFocus )
		return;
	cm.focusSel = k;
	cm.tlMs = CM_ShotStart( &cm.cine, cm.cur ) + s->focus[k].ms;
	CM_TlPin();
}

// the focus key at ms, made there as the focus is then if there's none; -1 if there's no room
static int CM_FocusKeyAt( rcShot_t *s, int ms ) {
	rcFocus_t f;
	int i;

	for ( i = 0; i < s->numFocus; i++ ) {
		if ( abs( s->focus[i].ms - ms ) <= 50 )
			return i;
	}
	if ( s->numFocus >= RC_MAX_FOCUS ) {
		CM_Message( S_COLOR_YELLOW "8 focus keys at most in a shot" );
		return -1;
	}
	f.ms = ms;
	if ( s->numFocus ) {
		RC_FocusPoint( s, ms, f.org, &f.range );
	} else {
		vec3_t fwd;

		AngleVectors( cm.camAng, fwd, NULL, NULL );
		if ( cm.haveAim )
			VectorCopy( cm.aim, f.org );
		else
			VectorMA( cm.camOrg, 256.0f, fwd, f.org );
		f.range = s->dofRange > 0.0f ? s->dofRange : RC_DOF_RANGE;
	}
	for ( i = s->numFocus++; i > 0 && s->focus[i - 1].ms > ms; i-- )
		s->focus[i] = s->focus[i - 1];
	s->focus[i] = f;
	CM_Changed();
	return i;
}

// a key retimed: back in order of when, the one followed kept track of
static int CM_SortFocus( rcShot_t *s, int k ) {
	while ( k > 0 && s->focus[k - 1].ms > s->focus[k].ms ) {
		const rcFocus_t tmp = s->focus[k];

		s->focus[k] = s->focus[k - 1];
		s->focus[k - 1] = tmp;
		k--;
	}
	while ( k + 1 < s->numFocus && s->focus[k + 1].ms < s->focus[k].ms ) {
		const rcFocus_t tmp = s->focus[k];

		s->focus[k] = s->focus[k + 1];
		s->focus[k + 1] = tmp;
		k++;
	}
	return k;
}

// a moving focus: its keys, where the one picked is and how far it's sharp
static float CM_FocusKeyControls( float x, float y, float w, rcShot_t *s ) {
	const float half = ( w - 4 ) * 0.5f;
	int steps, k;

	if ( !s->numFocus )
		CM_FocusSeeKey( s, CM_FocusKeyAt( s, CM_FocusMs( s ) ) );
	cm.focusSel = Com_Clampi( 0, Q_max( 0, s->numFocus - 1 ), cm.focusSel );
	CM_Text( x, y, S_COLOR_GREY "Drag the circle in the view to what's to be sharp.", cmWhite );
	y += CM_ROW_H;
	CM_Text( x, y, S_COLOR_GREY "At another moment it's another key: the focus", cmWhite );
	y += CM_ROW_H;
	CM_Text( x, y, S_COLOR_GREY "goes from one to the next. Wheel on it: how deep", cmWhite );
	y += CM_ROW_H + CM_GAP;
	steps = CM_Stepper( x, y, w, "Key", s->numFocus ? va( "%i of %i, at %.1f s", cm.focusSel + 1, s->numFocus, s->focus[cm.focusSel].ms / 1000.0f ) : "none" );
	if ( steps && s->numFocus )
		CM_FocusSeeKey( s, Com_Clampi( 0, s->numFocus - 1, cm.focusSel + steps ) );
	y += CM_CTRL_H + CM_GAP;
	if ( CM_Button( x, y, half, "Key now, crosshair", qfalse, cm.haveAim ) && ( k = CM_FocusKeyAt( s, CM_FocusMs( s ) ) ) >= 0 ) {
		VectorCopy( cm.aim, s->focus[k].org );
		cm.focusSel = k;
		CM_Changed();
	}
	if ( CM_Button( x + half + 4, y, half, "Delete key", qfalse, (qboolean)( s->numFocus > 1 ) ) && s->numFocus > 1 ) {
		s->numFocus--;
		memmove( &s->focus[cm.focusSel], &s->focus[cm.focusSel + 1], ( s->numFocus - cm.focusSel ) * sizeof( s->focus[0] ) );
		cm.focusSel = Q_min( cm.focusSel, s->numFocus - 1 );
		CM_Changed();
	}
	y += CM_CTRL_H + CM_GAP;
	if ( s->numFocus ) {
		steps = CM_Stepper( x, y, w, "Sharp", va( "%i either side", RC_Round( s->focus[cm.focusSel].range ) ) );
		CM_Adjust( &s->focus[cm.focusSel].range, steps, 16.0f, 8.0f, 4096.0f );
		y += CM_CTRL_H + CM_GAP;
	}
	return y;
}

// a focus: what's in the middle, a distance, someone, or a spot that moves; how far it's sharp and
// how blurred the rest is. Where the next row goes
static float CM_FocusControls( float x, float y, float w, float *focus, float *range, float *blur, qboolean whoBox ) {
	rcShot_t *s = CM_Shot();
	const float third = ( w - 8 ) / 3.0f;
	int steps;

	CM_Text( x, y + 2, "Focus on", cmDim );
	y += CM_ROW_H + 1;
	{
		// the shot's has a fourth, two rows of two
		const float bw = whoBox ? ( w - 4 ) * 0.5f : third;
		float bx[4], by[4];

		for ( int i = 0; i < 4; i++ ) {
			bx[i] = whoBox ? x + ( i & 1 ) * ( bw + 4 ) : x + i * ( third + 4 );
			by[i] = whoBox ? y + ( i >> 1 ) * ( CM_CTRL_H + CM_GAP ) : y;
		}
		if ( CM_Button( bx[0], by[0], bw, "The middle", (qboolean)( *focus == 0.0f ), qtrue ) ) {
			*focus = 0.0f;
			CM_Changed();
		}
		if ( CM_Button( bx[1], by[1], bw, "A distance", (qboolean)( *focus > 0.0f ), qtrue ) ) {
			if ( *focus <= 0.0f )
				*focus = cm.haveAim ? Q_max( 1.0f, Distance( cm.camOrg, cm.aim ) ) : 256.0f;
			CM_Changed();
		}
		if ( CM_Button( bx[2], by[2], bw, "Someone", (qboolean)( *focus == -1.0f ), qtrue ) ) {
			*focus = -1.0f;
			if ( !s->dofWho[0] && CM_PersonAtCrosshair() )
				Q_strncpyz( s->dofWho, CM_PersonAtCrosshair(), sizeof( s->dofWho ) );
			CM_Changed();
		}
		if ( whoBox && CM_Button( bx[3], by[3], bw, "A moving spot", (qboolean)( *focus == RC_DOF_POINT ), qtrue ) ) {
			*focus = RC_DOF_POINT;
			CM_Changed();
		}
		y = by[whoBox ? 3 : 2] + CM_CTRL_H + CM_GAP;
	}

	if ( *focus == RC_DOF_POINT ) {
		y = CM_FocusKeyControls( x, y, w, s );
		steps = CM_Stepper( x, y, w, "Blur", *blur > 0.0f ? va( "%i", RC_Round( *blur ) ) : "none, sharp" );
		CM_Adjust( blur, steps, 1.0f, 1.0f, RC_DOF_MAX_BLUR );
		return y + CM_CTRL_H + CM_GAP;
	}
	if ( *focus > 0.0f ) {
		steps = CM_Stepper( x, y, w, "Distance", va( "%i away", RC_Round( *focus ) ) );
		CM_Adjust( focus, steps, 16.0f, 1.0f, 8192.0f );
		y += CM_CTRL_H + CM_GAP;
		if ( CM_Button( x, y, w, "Set to the crosshair", qfalse, cm.haveAim ) ) {
			*focus = Q_max( 1.0f, Distance( cm.camOrg, cm.aim ) );
			CM_Changed();
		}
		y += CM_CTRL_H + CM_GAP;
	} else if ( *focus < 0.0f ) {
		vec3_t at;

		if ( whoBox ) {
			CM_Text( x, y + 2, "Who", cmDim );
			CM_TextBox( x + 62, y, w - 62 - 34, s->dofWho, "me, a number or a name", CM_TYPE_FOCUS, NULL );
			if ( CM_Button( x + w - 30, y, 30, "Aim", qfalse, (qboolean)( CM_PersonAtCrosshair() != NULL ) ) ) {
				Q_strncpyz( s->dofWho, CM_PersonAtCrosshair(), sizeof( s->dofWho ) );
				CM_Changed();
			}
			y += CM_CTRL_H + CM_GAP;
		}
		if ( !s->dofWho[0] )
			CM_Text( x, y, S_COLOR_YELLOW "Nobody yet: the middle instead", cmWhite );
		else if ( RC_FocusTarget( s->dofWho, at ) )
			CM_TextClipped( x, y, w, va( S_COLOR_GREEN "%s" S_COLOR_GREY ", %i away from here", s->dofWho, RC_Round( Distance( cm.camOrg, at ) ) ), cmWhite );
		else
			CM_TextClipped( x, y, w, va( S_COLOR_YELLOW "%s isn't here: the middle meanwhile", s->dofWho ), cmWhite );
		y += CM_ROW_H + CM_GAP;
	}

	steps = CM_Stepper( x, y, w, "Sharp", va( "%i either side", RC_Round( *range ) ) );
	CM_Adjust( range, steps, 16.0f, 8.0f, 4096.0f );
	y += CM_CTRL_H + CM_GAP;
	steps = CM_Stepper( x, y, w, "Blur", *blur > 0.0f ? va( "%i", RC_Round( *blur ) ) : "none, sharp" );
	CM_Adjust( blur, steps, 1.0f, whoBox ? 1.0f : 0.0f, RC_DOF_MAX_BLUR );
	return y + CM_CTRL_H + CM_GAP;
}

// the shot's depth of field, and how it changes along the shot from spot to spot
static void CM_DrawFocusPage( float x, float y, float w ) {
	rcShot_t *s = CM_Shot();
	const float half = ( w - 4 ) * 0.5f;
	rcKey_t *k;

	if ( !re->ext.SetDepthOfField ) {
		CM_Text( x, y, S_COLOR_GREY "This renderer has no depth of field", cmWhite );
		return;
	}
	if ( CM_Switch( x, y, w, "Depth of field", s->dof ? "on" : "off, all sharp" ) ) {
		s->dof = (qboolean)!s->dof;
		if ( s->dof && s->dofRange <= 0.0f )
			s->dofRange = RC_DOF_RANGE;
		if ( s->dof && s->dofBlur <= 0.0f )
			s->dofBlur = RC_DOF_BLUR;
		CM_Changed();
	}
	y += CM_CTRL_H + CM_GAP;
	if ( !s->dof )
		return;
	y = CM_FocusControls( x, y, w, &s->dofFocus, &s->dofRange, &s->dofBlur, qtrue );

	// along the shot: a spot can have its own, the shot going from one to the next
	if ( s->orbit || s->numKeys < 2 || CM_FocusMoves( s ) )
		return;
	y += 2;
	{
		const int steps = CM_Stepper( x, y, w, "At spot", va( "%i of %i", cm.sel + 1, s->numKeys ) );

		if ( steps )
			CM_SelectKey( cm.sel + steps );
	}
	y += CM_CTRL_H + CM_GAP;
	k = CM_SelectedKey();
	if ( !k )
		return;
	if ( CM_Button( x, y, half, cm.sel ? "As the one before" : "As the shot's", (qboolean)!k->dofSet, qtrue ) && k->dofSet ) {
		k->dofSet = qfalse;
		CM_Changed();
	}
	if ( CM_Button( x + half + 4, y, half, "Its own", k->dofSet, qtrue ) && !k->dofSet ) {
		RC_KeyDof( s, cm.sel, &k->dofFocus, &k->dofRange, &k->dofBlur );
		k->dofSet = qtrue;
		CM_Changed();
	}
	y += CM_CTRL_H + CM_GAP;
	if ( k->dofSet )
		CM_FocusControls( x, y, w, &k->dofFocus, &k->dofRange, &k->dofBlur, qfalse );
}

// the filter over the picture, and how the shot comes in and goes out
static void CM_DrawLookPage( float x, float y, float w ) {
	rcShot_t *s = CM_Shot();
	int steps;

	if ( re->ext.SetScreenFilter ) {
		steps = CM_Stepper( x, y, w, "Filter", RC_FilterName( s->filter ) );
		if ( steps ) {
			s->filter = ( ( s->filter + steps ) % RC_NUM_FILTERS + RC_NUM_FILTERS ) % RC_NUM_FILTERS;
			if ( s->filter && !s->filterStrength )
				s->filterStrength = 100;
			CM_Changed();
		}
		y += CM_CTRL_H + CM_GAP;
		if ( s->filter ) {
			float strength = (float)s->filterStrength;

			steps = CM_Stepper( x, y, w, "Strength", va( "%i%%", s->filterStrength ) );
			CM_Adjust( &strength, steps, 10.0f, 10.0f, 100.0f );
			s->filterStrength = RC_Round( strength );
			y += CM_CTRL_H + CM_GAP;
		}
		y += 4;
	}
	y = CM_DrawTransition( x, y, w, "Coming in", &s->in );
	y = CM_DrawTransition( x, y, w, "Going out", &s->out );
}

// someone kept in frame, a path turning to them or an orbit circling them; and the camera
// shaking, held by hand and jolted
static void CM_DrawTrackPage( float x, float y, float w ) {
	static const char *handheld[RC_MAX_HANDHELD + 1] = { "off, steady", "light", "medium", "heavy" };
	rcShot_t *s = CM_Shot();
	const float half = ( w - 4 ) * 0.5f;
	int steps, local = 0;
	vec3_t at;
	float yaw;

	CM_Text( x, y + 2, "Keep in frame", cmWhite );
	y += CM_ROW_H + 2;
	CM_Text( x, y + 2, "Who", cmDim );
	CM_TextBox( x + 30, y, w - 30 - 34, s->trackWho, "nobody: as set", CM_TYPE_TRACK, NULL );
	if ( CM_Button( x + w - 30, y, 30, "Aim", qfalse, (qboolean)( CM_PersonAtCrosshair() != NULL ) ) ) {
		Q_strncpyz( s->trackWho, CM_PersonAtCrosshair(), sizeof( s->trackWho ) );
		CM_Changed();
	}
	y += CM_CTRL_H + CM_GAP;
	if ( s->trackWho[0] ) {
		if ( RC_TrackTarget( s->trackWho, at, &yaw ) )
			CM_TextClipped( x, y, w, va( S_COLOR_GREEN "%s" S_COLOR_GREY ": the camera %s", s->trackWho, s->orbit ? "circles them" : "turns to them" ), cmWhite );
		else
			CM_TextClipped( x, y, w, va( S_COLOR_YELLOW "%s isn't here: as set meanwhile", s->trackWho ), cmWhite );
		y += CM_ROW_H + 2;
		if ( s->orbit ) {
			if ( CM_Switch( x, y, w, "From yaw", s->trackTurn ? "the way they face" : "the map's" ) ) {
				s->trackTurn = (qboolean)!s->trackTurn;
				CM_Changed();
			}
			y += CM_CTRL_H + CM_GAP;
			CM_Text( x, y, S_COLOR_GREY "Their way: from yaw 180 behind them,", cmWhite );
			CM_Text( x, y + CM_ROW_H, S_COLOR_GREY "90 or 270 beside, 0 in front; degrees 0", cmWhite );
			CM_Text( x, y + CM_ROW_H * 2, S_COLOR_GREY "stays there as they go (Spots page)", cmWhite );
			y += CM_ROW_H * 3 + 2;
		} else {
			CM_Text( x, y, S_COLOR_GREY "The spots stay where they are; it turns", cmWhite );
			CM_Text( x, y + CM_ROW_H, S_COLOR_GREY "to them. An orbit goes round them", cmWhite );
			y += CM_ROW_H * 2 + 2;
		}
	} else {
		CM_Text( x, y, S_COLOR_GREY "A player or NPC: a path turns to them", cmWhite );
		CM_Text( x, y + CM_ROW_H, S_COLOR_GREY "wherever they go, an orbit circles them", cmWhite );
		y += CM_ROW_H * 2 + 2;
	}

	// the vertigo shot: zoomed as the camera moves so what it looks at stays as big
	if ( !s->orbit && ( s->trackWho[0] || s->look ) ) {
		if ( CM_Switch( x, y, w, "Dolly zoom", s->dolly ? "on: it stays as big" : "off" ) ) {
			s->dolly = (qboolean)!s->dolly;
			CM_Changed();
		}
		y += CM_CTRL_H + CM_GAP;
	}
	y += 4;
	CM_Fill( x, y - 3, w, 1, cmBorder );
	CM_Text( x, y + 2, "Shake", cmWhite );
	y += CM_ROW_H + 2;
	steps = CM_Stepper( x, y, w, "Handheld", handheld[s->handheld] );
	if ( steps ) {
		s->handheld = Com_Clampi( 0, RC_MAX_HANDHELD, s->handheld + steps );
		CM_Changed();
	}
	y += CM_CTRL_H + CM_GAP;
	// a jolt at the moment picked, if it's in this shot
	if ( cm.tlPinned && CM_ShotAt( &cm.cine, cm.tlMs, &local ) != cm.cur )
		local = 0;
	if ( CM_Button( x, y, w, va( "+ Jolt at %.1f s into the shot", local / 1000.0f ), qfalse, (qboolean)( s->numShakes < RC_MAX_SHAKES ) ) ) {
		int i = s->numShakes++;

		for ( ; i > 0 && s->shakes[i - 1].ms > local; i-- )
			s->shakes[i] = s->shakes[i - 1];
		s->shakes[i].ms = local;
		s->shakes[i].strength = 5;
		s->shakes[i].len = 1000;
		cm.shakeSel = i;
		CM_Changed();
	}
	y += CM_CTRL_H + CM_GAP;
	for ( int i = 0; i < s->numShakes; i++ ) {
		const rcShake_t *k = &s->shakes[i];

		if ( CM_Button( x, y, w - 20, va( "%.1f s: strength %i, %.1f s", k->ms / 1000.0f, k->strength, k->len / 1000.0f ), (qboolean)( i == cm.shakeSel ), qtrue ) )
			cm.shakeSel = i;
		if ( CM_Button( x + w - 16, y, 16, "x", qfalse, qtrue ) ) {
			memmove( &s->shakes[i], &s->shakes[i + 1], ( s->numShakes - i - 1 ) * sizeof( s->shakes[0] ) );
			s->numShakes--;
			CM_Changed();
			return;
		}
		y += CM_CTRL_H + 2;
	}
	if ( cm.shakeSel >= 0 && cm.shakeSel < s->numShakes ) {
		rcShake_t *k = &s->shakes[cm.shakeSel];

		y += CM_GAP;
		steps = CM_Stepper( x, y, w, "Strength", va( "%i", k->strength ) );
		if ( steps ) {
			k->strength = Com_Clampi( 1, 10, k->strength + steps );
			CM_Changed();
		}
		y += CM_CTRL_H + CM_GAP;
		steps = CM_Stepper( x, y, w, "Dies away in", va( "%.1f s", k->len / 1000.0f ) );
		CM_AdjustMs( &k->len, steps, 0.25f, 100 );
		y += CM_CTRL_H + CM_GAP;
		steps = CM_Stepper( x, y, half * 2 + 4, "At", va( "%.1f s", k->ms / 1000.0f ) );
		CM_AdjustMs( &k->ms, steps, 0.1f, 0 );
	} else if ( !s->numShakes ) {
		CM_Text( x, y, S_COLOR_GREY "A jolt: an explosion, a ship landing", cmWhite );
	}
}

static void CM_DrawCameraTab( float x, float y, float w ) {
	rcShot_t *s = CM_Shot();
	const float half = ( w - 4 ) * 0.5f;
	static const char *pages[] = { "Spots", "Focus", "Filter", "Track" };

	if ( !s ) {
		CM_Text( x, y, "No shots yet", cmWhite );
		CM_Text( x, y + CM_ROW_H, S_COLOR_GREY "Fly where it starts (hold right mouse)", cmWhite );
		CM_Text( x, y + CM_ROW_H * 2, S_COLOR_GREY "and press Enter", cmWhite );
		if ( CM_Button( x, y + CM_ROW_H * 3 + 4, w, "+ Shot here", qfalse, qtrue ) )
			CM_AddShot();
		return;
	}

	// which shot, and more of them
	if ( CM_Button( x, y, 16, "<", qfalse, (qboolean)( cm.cur > 0 ) ) )
		CM_SelectShot( cm.cur - 1 );
	CM_Text( x + 22, y + 2, va( "Shot %i of %i" S_COLOR_GREY "   %.1f s", cm.cur + 1, cm.cine.numShots, CM_ShotLen( s ) / 1000.0f ), cmWhite );
	if ( CM_Button( x + w - 16, y, 16, ">", qfalse, (qboolean)( cm.cur < cm.cine.numShots - 1 ) ) )
		CM_SelectShot( cm.cur + 1 );
	y += CM_CTRL_H + CM_GAP;
	if ( CM_Button( x, y, half, "+ Shot after it", qfalse, (qboolean)( cm.cine.numShots < CM_MAX_SHOTS ) ) )
		CM_AddShot();
	if ( CM_Button( x + half + 4, y, half, "Delete shot", qfalse, qtrue ) ) {
		CM_DeleteShot();
		return;
	}
	y += CM_CTRL_H + CM_GAP;
	// a shot flown by hand
	if ( CM_Button( x, y, w, cm.camRec ? va( S_COLOR_RED "Recording %.1f s: Stop (Enter)", ( cls.realtime - cm.camRecStart ) / 1000.0f ) : "Record a shot: fly the camera",
		cm.camRec, (qboolean)( cm.camRec || cm.cine.numShots < CM_MAX_SHOTS ) ) ) {
		if ( cm.camRec )
			CM_CamRecStop();
		else
			CM_CamRecStart();
	}
	y += CM_CTRL_H + CM_GAP;
	if ( CM_Button( x, y, half, "Path", (qboolean)!s->orbit, qtrue ) )
		CM_SetOrbit( qfalse );
	if ( CM_Button( x + half + 4, y, half, "Orbit", s->orbit, qtrue ) )
		CM_SetOrbit( qtrue );
	y += CM_CTRL_H + CM_GAP;
	if ( s->orbit || s->numKeys > 1 ) {
		if ( CM_Switch( x, y, w, "Speed", s->linear ? "steady" : "smooth, eases in and out" ) ) {
			s->linear = (qboolean)!s->linear;
			CM_Changed();
		}
		y += CM_CTRL_H + CM_GAP;
	}
	y += 2;
	cm.page = CM_Tabs( x, y, w, pages, ARRAY_LEN( pages ), cm.page );
	if ( ( cm.page != CM_PAGE_FOCUS && cm.typing == CM_TYPE_FOCUS ) || ( cm.page != CM_PAGE_TRACK && cm.typing == CM_TYPE_TRACK ) )
		cm.typing = CM_TYPE_NONE;
	y += CM_CTRL_H + 6;
	if ( cm.page == CM_PAGE_FOCUS )
		CM_DrawFocusPage( x, y, w );
	else if ( cm.page == CM_PAGE_LOOK )
		CM_DrawLookPage( x, y, w );
	else if ( cm.page == CM_PAGE_TRACK )
		CM_DrawTrackPage( x, y, w );
	else if ( s->orbit )
		CM_DrawOrbitPage( x, y, w );
	else
		CM_DrawSpotsPage( x, y, w );
}

/*
===============================================================================

The Entities tab

===============================================================================
*/

static void CM_DrawMoveClip( float x, float y, float w, cmClip_t *clip ) {
	const float half = ( w - 4 ) * 0.5f;
	const cmPlan_t *p = &cmEditPlans[cm.clip];

	if ( CM_Switch( x, y, w, "Pace", clip->run ? "runs" : "walks" ) ) {
		clip->run = (qboolean)!clip->run;
		CM_Changed();
	}
	y += CM_CTRL_H + CM_GAP;
	CM_Text( x, y, va( "Spots (%i)", clip->numPts ), cmWhite );
	if ( p->count )
		CM_Text( x + 62, y, va( S_COLOR_GREY "%.1f s to %.1f s", clip->start / 1000.0f, ( clip->start + p->ms ) / 1000.0f ), cmWhite );
	y += CM_ROW_H + 2;
	if ( CM_Button( x, y, half, "Add at crosshair", qfalse, (qboolean)( cm.haveAim && clip->numPts < CM_MAX_PTS ) ) )
		CM_AddMoveSpot();
	if ( CM_Button( x + half + 4, y, half, cm.spot >= 0 && cm.spot < clip->numPts ? va( "Remove spot %i", cm.spot + 1 ) : "Remove last", qfalse, (qboolean)( clip->numPts > 0 ) ) )
		CM_RemoveMoveSpot();
	y += CM_CTRL_H + CM_GAP + 2;
	if ( !clip->numPts ) {
		CM_Text( x, y, S_COLOR_GREY "Aim at the floor where they go", cmWhite );
		CM_Text( x, y + CM_ROW_H, S_COLOR_GREY "and press Enter, a spot at a time", cmWhite );
	} else {
		CM_Text( x, y, S_COLOR_GREY "Click a spot in the view to pick it,", cmWhite );
		CM_Text( x, y + CM_ROW_H, S_COLOR_GREY "drag it to move it. Drag the move", cmWhite );
		CM_Text( x, y + CM_ROW_H * 2, S_COLOR_GREY "on the timeline for when they set off", cmWhite );
	}
}

static void CM_DrawAnimClip( float x, float y, float w, cmClip_t *clip ) {
	const float half = ( w - 4 ) * 0.5f;
	rcAnim_t *a = &clip->a;
	int nums[4], n, cfg, count, numPicks = 0, steps;
	const rcAnimDef_t *defs, *def;
	static int picks[2048];

	n = CM_Resolve( cm.cine.ents[clip->ent].who, nums, ARRAY_LEN( nums ) );
	cfg = RC_EntCfg( n ? nums[0] : -1 );
	def = RC_FindAnim( cfg, a->anim );
	defs = RC_AnimDefs( cfg, &count );

	// which: typed, the list under it the ones with that in their name
	CM_TextBox( x, y, w, a->anim, "type to look for one", CM_TYPE_ANIM, def || cm.typing == CM_TYPE_ANIM ? NULL : cmUnknown );
	y += CM_CTRL_H + 1;
	for ( int i = 0; i < count && numPicks < (int)ARRAY_LEN( picks ); i++ ) {
		if ( cm.typing != CM_TYPE_ANIM || !a->anim[0] || Q_stristr( defs[i].name, a->anim ) )
			picks[numPicks++] = i;
	}
	CM_Box( x, y, w, CM_PICK_ROWS * CM_ROW_H + 2, cmPanelLight );
	if ( cm.wheel && CM_InRect( x, y, w, CM_PICK_ROWS * CM_ROW_H ) ) {
		cm.pickScroll += cm.wheel;
		cm.wheel = 0;
	}
	// the picked one in view when not looking for one
	if ( cm.typing != CM_TYPE_ANIM && def && cm.pickScroll == 0 && defs ) {
		for ( int i = 0; i < numPicks; i++ ) {
			if ( &defs[picks[i]] == def && i >= CM_PICK_ROWS )
				cm.pickScroll = i - 1;
		}
	}
	cm.pickScroll = Com_Clampi( 0, Q_max( 0, numPicks - CM_PICK_ROWS ), cm.pickScroll );
	const float lw = CM_ListBar( x, y, w, CM_PICK_ROWS * CM_ROW_H + 2, numPicks, CM_PICK_ROWS, &cm.pickScroll );
	for ( int r = 0; r < CM_PICK_ROWS && cm.pickScroll + r < numPicks; r++ ) {
		const rcAnimDef_t *d = &defs[picks[cm.pickScroll + r]];
		const float ry = y + 1 + r * CM_ROW_H;

		if ( def == d )
			CM_Fill( x + 1, ry, lw - 2, CM_ROW_H, cmHighlight );
		else if ( CM_InRect( x, ry, lw, CM_ROW_H ) )
			CM_Fill( x + 1, ry, lw - 2, CM_ROW_H, cmHoverBox );
		CM_TextClipped( x + 4, ry, lw - 8, va( "%s" S_COLOR_GREY " %i", d->name, d->num ), cmWhite );
		if ( CM_Clicked( x, ry, lw, CM_ROW_H ) ) {
			Q_strncpyz( a->anim, d->name, sizeof( a->anim ) );
			a->from = 0;
			a->to = -1;
			cm.typing = CM_TYPE_NONE;
			CM_Changed();
		}
	}
	if ( !numPicks )
		CM_Text( x + 4, y + 2, S_COLOR_GREY "No animation by that name", cmWhite );
	y += CM_PICK_ROWS * CM_ROW_H + 6;

	// the frames: a range, played forward or back, or one held
	if ( CM_Button( x, y, half, a->mode == RC_AM_HOLD ? "Holds a frame" : a->mode == RC_AM_REVERSE ? "Plays backwards" : "Plays", qfalse, qtrue ) ) {
		a->mode = ( a->mode + 1 ) % RC_NUM_AMODES;
		CM_Changed();
	}
	if ( CM_Button( x + half + 4, y, half, a->loop ? "Loops" : "Once", a->loop, (qboolean)( a->mode != RC_AM_HOLD ) ) ) {
		a->loop = (qboolean)!a->loop;
		CM_Changed();
	}
	y += CM_CTRL_H + CM_GAP;
	{
		const int last = def ? def->num - 1 : 999;
		const int to = a->to < 0 ? last : a->to;

		steps = CM_Stepper( x, y, w, a->mode == RC_AM_HOLD ? "Frame" : "From", def ? va( "%i of %i", Com_Clampi( 0, last, a->from ), last ) : va( "%i", a->from ) );
		if ( steps ) {
			a->from = Com_Clampi( 0, last, a->from + steps * ( CM_ShiftDown() ? 10 : 1 ) );
			if ( a->to >= 0 && a->to < a->from )
				a->to = a->from;
			CM_Changed();
		}
		y += CM_CTRL_H + CM_GAP;
		if ( a->mode != RC_AM_HOLD ) {
			steps = CM_Stepper( x, y, w, "To", a->to < 0 ? "its last" : va( "%i", to ) );
			if ( steps ) {
				const int now = Com_Clampi( a->from, last, to + steps * ( CM_ShiftDown() ? 10 : 1 ) );

				a->to = now >= last && def ? -1 : now;
				CM_Changed();
			}
			y += CM_CTRL_H + CM_GAP;
			steps = CM_Stepper( x, y, w, "Speed", va( "%i%%", a->speed ) );
			if ( steps ) {
				a->speed = Com_Clampi( 5, 1000, a->speed + steps * ( CM_ShiftDown() ? 50 : 10 ) );
				CM_Changed();
			}
			y += CM_CTRL_H + CM_GAP;
		}
	}
	CM_Text( x, y + 1, va( S_COLOR_GREY "%.1f s to %s: drag it on the timeline", clip->start / 1000.0f,
		a->ms ? va( "%.1f s", ( clip->start + a->ms ) / 1000.0f ) : "the end" ), cmWhite );
	y += CM_ROW_H + 2;
	if ( CM_Switch( x, y, w, "On the model", cm.animLive ? "shown as it's made" : "not shown" ) )
		cm.animLive = (qboolean)!cm.animLive;
}

static qboolean CM_Project( const vec3_t point, float *x, float *y );

static void CM_StartRecording( int clip );

// someone the Entities tab lists: you, a player, an NPC, or someone in it who isn't here
typedef struct {
	int		num;				// their entity number, -1 not here
	int		ent;				// in the cinematic, -1 not
	char	who[MAX_QPATH];		// how they go in it: a player's name, an NPC's targetname or number
	char	label[MAX_QPATH * 2];
} cmPerson_t;

#define CM_MAX_PEOPLE		( MAX_CLIENTS + 512 + CM_MAX_ENTS )
static cmPerson_t cmPeople[CM_MAX_PEOPLE];
static int cmNumPeople;

static int CM_EntOfNum( int num, int entNums[][CM_MAX_GROUP], const int *entCounts ) {
	for ( int e = 0; e < cm.cine.numEnts; e++ ) {
		for ( int k = 0; k < entCounts[e]; k++ ) {
			if ( entNums[e][k] == num )
				return e;
		}
	}
	return -1;
}

// you, the other players, the NPCs in the NPC list, then whoever's in it but not here
static void CM_ListPeople( void ) {
	int entNums[CM_MAX_ENTS][CM_MAX_GROUP], entCounts[CM_MAX_ENTS];
	qboolean listed[CM_MAX_ENTS];

	cmNumPeople = 0;
	for ( int e = 0; e < cm.cine.numEnts; e++ ) {
		entCounts[e] = CM_Resolve( cm.cine.ents[e].who, entNums[e], CM_MAX_GROUP );
		listed[e] = qfalse;
	}
	for ( int pass = 0; pass < 2; pass++ ) {
		for ( int c = 0; c < MAX_CLIENTS && cmNumPeople < CM_MAX_PEOPLE; c++ ) {
			cmPerson_t *p = &cmPeople[cmNumPeople];
			const char *name;

			if ( ( c == clc.clientNum ) != ( pass == 0 ) || !cl.gameState.stringOffsets[CS_PLAYERS + c] )
				continue;
			name = CM_PlayerName( c );
			p->num = c;
			Q_strncpyz( p->who, name[0] ? name : va( "%i", c ), sizeof( p->who ) );
			Com_sprintf( p->label, sizeof( p->label ), "%s" S_COLOR_GREY "  %s", p->who, c == clc.clientNum ? "you" : "player" );
			p->ent = CM_EntOfNum( c, entNums, entCounts );
			if ( p->ent >= 0 )
				listed[p->ent] = qtrue;
			cmNumPeople++;
		}
	}
	for ( int i = 0; i < CL_NpcManager_NumNpcs() && cmNumPeople < CM_MAX_PEOPLE; i++ ) {
		cmPerson_t *p = &cmPeople[cmNumPeople];
		const char *type, *name;
		int num;

		if ( !CL_NpcManager_GetNpc( i, &num, &type, &name ) )
			continue;
		p->num = num;
		Q_strncpyz( p->who, name[0] ? name : va( "%i", num ), sizeof( p->who ) );
		Com_sprintf( p->label, sizeof( p->label ), "%s" S_COLOR_GREY "  %s", p->who, type );
		p->ent = CM_EntOfNum( num, entNums, entCounts );
		if ( p->ent >= 0 )
			listed[p->ent] = qtrue;
		cmNumPeople++;
	}
	for ( int e = 0; e < cm.cine.numEnts && cmNumPeople < CM_MAX_PEOPLE; e++ ) {
		cmPerson_t *p = &cmPeople[cmNumPeople];

		if ( listed[e] )
			continue;
		p->num = -1;
		p->ent = e;
		Q_strncpyz( p->who, cm.cine.ents[e].who, sizeof( p->who ) );
		if ( CM_IsExtra( p->who ) )
			Com_sprintf( p->label, sizeof( p->label ), "%s" S_COLOR_GREY "  extra, %s", p->who + 1, cm.cine.ents[e].model[0] ? cm.cine.ents[e].model : "kyle" );
		else
			Com_sprintf( p->label, sizeof( p->label ), "%s" S_COLOR_YELLOW "  not here", p->who[0] ? p->who : "nobody" );
		cmNumPeople++;
	}
}

static qboolean CM_Picked( const cmPerson_t *p ) {
	if ( p->ent >= 0 )
		return (qboolean)( p->ent == cm.ent );
	return (qboolean)( cm.ent < 0 && p->num >= 0 && p->num == cm.pickNum && cm.pickWho[0] && !Q_stricmp( p->who, cm.pickWho ) );
}

static void CM_Pick( const cmPerson_t *p ) {
	if ( CM_Picked( p ) )
		return;
	cm.pickNum = p->num;
	Q_strncpyz( cm.pickWho, p->who, sizeof( cm.pickWho ) );
	cm.ent = p->ent;
	cm.clip = -1;
	cm.spot = -1;
	cm.addOpen = qfalse;
	cm.typing = CM_TYPE_NONE;
}

// someone by their number, picked as if clicked in the list
static void CM_PickNum( int num ) {
	CM_ListPeople();
	for ( int i = 0; i < cmNumPeople; i++ ) {
		if ( cmPeople[i].num == num ) {
			CM_Pick( &cmPeople[i] );
			return;
		}
	}
}

static void CM_DrawSayClip( float x, float y, float w, cmClip_t *clip ) {
	int nums[4];
	const int n = CM_Resolve( cm.cine.ents[clip->ent].who, nums, ARRAY_LEN( nums ) );
	const qboolean npc = (qboolean)( n && nums[0] >= MAX_CLIENTS );

	CM_TextBox( x, y, w, clip->text, "type what they say", CM_TYPE_SAY, NULL );
	y += CM_CTRL_H + CM_GAP;
	if ( npc ) {
		CM_Text( x, y + 2, "Name shown", cmDim );
		CM_TextBox( x + 70, y, w - 70, clip->name, cm.cine.ents[clip->ent].who, CM_TYPE_SAYNAME, NULL );
		y += CM_CTRL_H + CM_GAP;
	}
	CM_Text( x, y + 2, va( S_COLOR_GREY "At %.1f s: drag it on the timeline", clip->start / 1000.0f ), cmWhite );
	y += CM_ROW_H + 4;
	if ( !n )
		CM_Text( x, y, S_COLOR_YELLOW "They aren't here to say it", cmWhite );
	else if ( nums[0] == clc.clientNum )
		CM_Text( x, y, S_COLOR_GREY "You say it in chat", cmWhite );
	else
		CM_Text( x, y, va( S_COLOR_GREY "Shown as their %s, for everyone", npc ? "talk" : "chat" ), cmWhite );
	CM_Text( x, y + CM_ROW_H, S_COLOR_GREY "with this client; a subtitle in a preview", cmWhite );
}

// a value with - and + either side, small, two to a row
static int CM_MiniStepper( float x, float y, float w, const char *label, const char *value ) {
	const float lw = 30.0f, vx = x + lw + 15, vw = w - lw - 30;
	int steps = 0;

	CM_Text( x, y + 2, label, cmDim );
	if ( CM_Button( x + lw, y, 14, "-", qfalse, qtrue ) )
		steps--;
	CM_Box( vx, y, vw, CM_CTRL_H, cmPanelLight );
	CM_Text( vx + ( vw - CM_TextWidth( value ) ) * 0.5f, y + 2, value, cmWhite );
	if ( CM_Button( x + w - 14, y, 14, "+", qfalse, qtrue ) )
		steps++;
	if ( cm.wheel && CM_InRect( x, y, w, CM_CTRL_H ) ) {
		steps -= cm.wheel;
		cm.wheel = 0;
	}
	return steps;
}

static void CM_DrawEmoteClip( float x, float y, float w, cmClip_t *clip ) {
	const float half = ( w - 4 ) * 0.5f;
	const int count = CL_EmotePreview_Count();
	const char *anim = clip->emote[0] ? CL_EmotePreview_Anim( clip->emote ) : NULL;
	static int picks[512];
	int numPicks = 0, nums[4], n;

	CM_TextBox( x, y, w, clip->emote, "type to look for one", CM_TYPE_EMOTE, anim || cm.typing == CM_TYPE_EMOTE ? NULL : cmUnknown );
	y += CM_CTRL_H + 1;
	for ( int i = 0; i < count && numPicks < (int)ARRAY_LEN( picks ); i++ ) {
		if ( cm.typing != CM_TYPE_EMOTE || !clip->emote[0] || Q_stristr( CL_EmotePreview_Name( i ), clip->emote ) )
			picks[numPicks++] = i;
	}
	CM_Box( x, y, w, CM_PICK_ROWS * CM_ROW_H + 2, cmPanelLight );
	if ( cm.wheel && CM_InRect( x, y, w, CM_PICK_ROWS * CM_ROW_H ) ) {
		cm.pickScroll += cm.wheel;
		cm.wheel = 0;
	}
	cm.pickScroll = Com_Clampi( 0, Q_max( 0, numPicks - CM_PICK_ROWS ), cm.pickScroll );
	const float lw = CM_ListBar( x, y, w, CM_PICK_ROWS * CM_ROW_H + 2, numPicks, CM_PICK_ROWS, &cm.pickScroll );
	for ( int r = 0; r < CM_PICK_ROWS && cm.pickScroll + r < numPicks; r++ ) {
		const char *name = CL_EmotePreview_Name( picks[cm.pickScroll + r] );
		const float ry = y + 1 + r * CM_ROW_H;

		if ( !Q_stricmp( name, clip->emote ) )
			CM_Fill( x + 1, ry, lw - 2, CM_ROW_H, cmHighlight );
		else if ( CM_InRect( x, ry, lw, CM_ROW_H ) )
			CM_Fill( x + 1, ry, lw - 2, CM_ROW_H, cmHoverBox );
		CM_TextClipped( x + 4, ry, lw - 8, va( "%s" S_COLOR_GREY "  %s", name, CL_EmotePreview_Anim( name ) ), cmWhite );
		if ( CM_Clicked( x, ry, lw, CM_ROW_H ) ) {
			Q_strncpyz( clip->emote, name, sizeof( clip->emote ) );
			cm.typing = CM_TYPE_NONE;
			CM_Changed();
		}
	}
	if ( !numPicks )
		CM_Text( x + 4, y + 2, S_COLOR_GREY "None known: see /emotepreview", cmWhite );
	y += CM_PICK_ROWS * CM_ROW_H + 6;

	n = CM_Resolve( cm.cine.ents[clip->ent].who, nums, ARRAY_LEN( nums ) );
	if ( CM_Button( x, y, half, clip->a.loop ? "Loops" : "Once", clip->a.loop, qtrue ) ) {
		clip->a.loop = (qboolean)!clip->a.loop;
		CM_Changed();
	}
	// an NPC's, or yours, done for real too when it's played for everyone
	if ( n && ( nums[0] >= MAX_CLIENTS || nums[0] == clc.clientNum ) ) {
		if ( CM_Button( x + half + 4, y, half, clip->realEmote ? "Done for real too" : "Only shown", clip->realEmote, qtrue ) ) {
			clip->realEmote = (qboolean)!clip->realEmote;
			CM_Changed();
		}
	} else {
		CM_Text( x + half + 8, y + 2, S_COLOR_GREY "only shown", cmWhite );
	}
	y += CM_CTRL_H + CM_GAP;
	CM_Text( x, y + 1, va( S_COLOR_GREY "%.1f s to %s: drag it on the timeline", clip->start / 1000.0f,
		clip->a.ms ? va( "%.1f s", ( clip->start + clip->a.ms ) / 1000.0f ) : "the end" ), cmWhite );
	y += CM_ROW_H + 2;
	if ( clip->emote[0] && !anim )
		CM_Text( x, y, S_COLOR_YELLOW "Not known: do it once in game", cmWhite );
	else if ( CM_Switch( x, y, w, "On the model", cm.animLive ? "shown as it's made" : "not shown" ) )
		cm.animLive = (qboolean)!cm.animLive;
}

static void CM_DrawPlaceClip( float x, float y, float w, cmClip_t *clip ) {
	const float half = ( w - 4 ) * 0.5f;
	const int rows = 3;
	cmPlace_t *k;
	int steps;

	if ( CM_Switch( x, y, w, "From key to key", clip->smooth ? "smooth" : "straight" ) ) {
		clip->smooth = (qboolean)!clip->smooth;
		CM_Changed();
	}
	y += CM_CTRL_H + CM_GAP;
	CM_Box( x, y, w, rows * CM_ROW_H + 2, cmPanelLight );
	if ( cm.wheel && CM_InRect( x, y, w, rows * CM_ROW_H ) ) {
		cm.pickScroll += cm.wheel;
		cm.wheel = 0;
	}
	cm.pickScroll = Com_Clampi( 0, Q_max( 0, clip->numKeys - rows ), cm.pickScroll );
	const float lw = CM_ListBar( x, y, w, rows * CM_ROW_H + 2, clip->numKeys, rows, &cm.pickScroll );
	for ( int r = 0; r < rows && cm.pickScroll + r < clip->numKeys; r++ ) {
		const int i = cm.pickScroll + r;
		const cmPlace_t *key = &clip->keys[i];
		const float ry = y + 1 + r * CM_ROW_H;

		if ( i == cm.spot )
			CM_Fill( x + 1, ry, lw - 2, CM_ROW_H, cmHighlight );
		else if ( CM_InRect( x, ry, lw, CM_ROW_H ) )
			CM_Fill( x + 1, ry, lw - 2, CM_ROW_H, cmHoverBox );
		CM_TextClipped( x + 4, ry, lw - 8, va( "%i  at %.1f s" S_COLOR_GREY "   %i %i %i", i + 1, ( clip->start + key->ms ) / 1000.0f,
			RC_Round( key->org[0] ), RC_Round( key->org[1] ), RC_Round( key->org[2] ) ), cmWhite );
		if ( CM_Clicked( x, ry, lw, CM_ROW_H ) )
			cm.spot = i;
	}
	y += rows * CM_ROW_H + 5;
	if ( CM_Button( x, y, half, cm.tlPinned ? va( "+ Key at %.1f s", cm.tlMs / 1000.0f ) : "+ Key", qfalse, (qboolean)( clip->numKeys < CM_MAX_PLACE ) ) )
		CM_AddPlaceKey( clip );
	if ( CM_Button( x + half + 4, y, half, "Delete key", qfalse, (qboolean)( cm.spot >= 0 && cm.spot < clip->numKeys ) ) )
		CM_DeletePlaceKey( clip );
	y += CM_CTRL_H + CM_GAP + 2;
	if ( cm.spot < 0 || cm.spot >= clip->numKeys ) {
		CM_Text( x, y, S_COLOR_GREY "Pick a key here or click it in the view", cmWhite );
		return;
	}
	k = &clip->keys[cm.spot];
	// where it is and how it's turned, side by side
	{
		static const char *axes[3] = { "X", "Y", "Z" }, *turns[3] = { "Pitch", "Yaw", "Roll" };
		static const int turnAxis[3] = { PITCH, YAW, ROLL };

		for ( int j = 0; j < 3; j++ ) {
			steps = CM_MiniStepper( x, y, half, axes[j], va( "%i", RC_Round( k->org[j] ) ) );
			CM_Adjust( &k->org[j], steps, 8.0f, -65536.0f, 65536.0f );
			steps = CM_MiniStepper( x + half + 4, y, half, turns[j], va( "%i", RC_Round( turnAxis[j] == YAW ? AngleNormalize360( k->ang[YAW] ) : AngleNormalize180( k->ang[turnAxis[j]] ) ) ) );
			CM_Adjust( &k->ang[turnAxis[j]], steps, 15.0f, -3600.0f, 3600.0f );
			y += CM_CTRL_H + CM_GAP;
		}
	}
	steps = CM_Stepper( x, y, w, "At", va( "%.1f s", ( clip->start + k->ms ) / 1000.0f ) );
	if ( steps ) {
		CM_AdjustMs( &k->ms, steps, 0.1f, 0 );
		CM_SortKeys( clip );
	}
	y += CM_CTRL_H + CM_GAP;
	CM_Text( x, y, S_COLOR_GREY "Drag a key in the view; pitch and roll tip", cmWhite );
	CM_Text( x, y + CM_ROW_H, S_COLOR_GREY "the whole body, as one thrown would be", cmWhite );
}

// a spot and a yaw, for a teleport or a spawn: where you aim, or nudged
static float CM_SpotControls( float x, float y, float w, cmPlace_t *k ) {
	const float half = ( w - 4 ) * 0.5f;
	int steps;

	if ( CM_Button( x, y, w, "Where you aim", qfalse, cm.haveAim ) ) {
		VectorCopy( cm.aim, k->org );
		CM_Changed();
	}
	y += CM_CTRL_H + CM_GAP;
	steps = CM_MiniStepper( x, y, half, "X", va( "%i", RC_Round( k->org[0] ) ) );
	CM_Adjust( &k->org[0], steps, 8.0f, -65536.0f, 65536.0f );
	steps = CM_MiniStepper( x + half + 4, y, half, "Y", va( "%i", RC_Round( k->org[1] ) ) );
	CM_Adjust( &k->org[1], steps, 8.0f, -65536.0f, 65536.0f );
	y += CM_CTRL_H + CM_GAP;
	steps = CM_MiniStepper( x, y, half, "Z", va( "%i", RC_Round( k->org[2] ) ) );
	CM_Adjust( &k->org[2], steps, 8.0f, -65536.0f, 65536.0f );
	steps = CM_MiniStepper( x + half + 4, y, half, "Yaw", va( "%i", RC_Round( AngleNormalize360( k->ang[YAW] ) ) ) );
	CM_Adjust( &k->ang[YAW], steps, 15.0f, -3600.0f, 3600.0f );
	return y + CM_CTRL_H + CM_GAP;
}

static void CM_DrawTeleClip( float x, float y, float w, cmClip_t *clip ) {
	int nums[4];
	const int n = CM_Resolve( cm.cine.ents[clip->ent].who, nums, ARRAY_LEN( nums ) );

	if ( !clip->numKeys ) {
		clip->numKeys = 1;
		CM_EntityNow( clip->ent, clip->keys[0].org, clip->keys[0].ang );
	}
	cm.spot = 0;
	y = CM_SpotControls( x, y, w, &clip->keys[0] );
	CM_Text( x, y, va( S_COLOR_GREY "At %.1f s: drag it on the timeline,", clip->start / 1000.0f ), cmWhite );
	CM_Text( x, y + CM_ROW_H, S_COLOR_GREY "the spot in the view", cmWhite );
	y += CM_ROW_H * 2 + 4;
	if ( n && nums[0] < MAX_CLIENTS ) {
		CM_Text( x, y, S_COLOR_GREY "A player is only seen there, on screens", cmWhite );
		CM_Text( x, y + CM_ROW_H, S_COLOR_GREY "with this client, till it ends", cmWhite );
	} else {
		CM_Text( x, y, S_COLOR_GREY "Teleported for real (npc tele) when it's", cmWhite );
		CM_Text( x, y + CM_ROW_H, S_COLOR_GREY "played for everyone", cmWhite );
	}
}

// an NPC's, or several with one targetname: someone who isn't a player
static qboolean CM_NpcEnt( int ent ) {
	int nums[4];
	const char *who = cm.cine.ents[ent].who;

	if ( !who[0] || !Q_stricmp( who, "me" ) || CM_FindPlayer( who ) >= 0 )
		return qfalse;
	return (qboolean)( !CM_Resolve( who, nums, ARRAY_LEN( nums ) ) || nums[0] >= MAX_CLIENTS );
}

static void CM_DrawNpcCmdClip( float x, float y, float w, cmClip_t *clip ) {
	int which = 0, steps;

	for ( int i = 0; i < (int)ARRAY_LEN( cmNpcCmds ); i++ ) {
		if ( !Q_stricmp( clip->cmd, cmNpcCmds[i] ) )
			which = i;
	}
	steps = CM_Stepper( x, y, w, "Command", cmNpcCmds[which] );
	if ( steps ) {
		which = ( ( which + steps ) % (int)ARRAY_LEN( cmNpcCmds ) + (int)ARRAY_LEN( cmNpcCmds ) ) % (int)ARRAY_LEN( cmNpcCmds );
		Q_strncpyz( clip->cmd, cmNpcCmds[which], sizeof( clip->cmd ) );
		CM_Changed();
	}
	y += CM_CTRL_H + CM_GAP;
	if ( !Q_stricmp( clip->cmd, "dialog" ) ) {
		CM_TextBox( x, y, w, clip->text, "type the dialog", CM_TYPE_DIALOG, NULL );
		y += CM_CTRL_H + CM_GAP;
	}
	CM_Text( x, y + 2, va( S_COLOR_GREY "At %.1f s: drag it on the timeline", clip->start / 1000.0f ), cmWhite );
	y += CM_ROW_H + 4;
	if ( !CM_NpcEnt( clip->ent ) )
		CM_Text( x, y, S_COLOR_YELLOW "Only NPCs take it: this is a player", cmWhite );
	else
		CM_Text( x, y, S_COLOR_GREY "Sent when it's played for everyone", cmWhite );
}

static void CM_DrawSpawnClip( float x, float y, float w, cmClip_t *clip ) {
	const char *who = cm.cine.ents[clip->ent].who;
	int team = 0, steps;

	CM_TextBox( x, y, w, clip->npcType, "the NPC type, e.g. stormtrooper", CM_TYPE_NPCTYPE, NULL );
	y += CM_CTRL_H + CM_GAP;
	for ( int i = 0; i < (int)ARRAY_LEN( cmTeams ); i++ ) {
		if ( !Q_stricmp( clip->team, cmTeams[i] ) )
			team = i;
	}
	steps = CM_Stepper( x, y, w, "Team", cmTeams[team] );
	if ( steps ) {
		team = ( ( team + steps ) % (int)ARRAY_LEN( cmTeams ) + (int)ARRAY_LEN( cmTeams ) ) % (int)ARRAY_LEN( cmTeams );
		Q_strncpyz( clip->team, cmTeams[team], sizeof( clip->team ) );
		CM_Changed();
	}
	y += CM_CTRL_H + CM_GAP;
	clip->numKeys = 1;
	y = CM_SpotControls( x, y, w, &clip->keys[0] );
	CM_Text( x, y, va( S_COLOR_GREY "At %.1f s, its targetname %s", clip->start / 1000.0f, who[0] ? who : "?" ), cmWhite );
	y += CM_ROW_H + 2;
	if ( !CM_NpcEnt( clip->ent ) || !who[0] || Q_isanumber( who ) || strchr( who, ',' ) )
		CM_Text( x, y, S_COLOR_YELLOW "Spawns need an NPC's targetname here", cmWhite );
	else
		CM_Text( x, y, S_COLOR_GREY "Spawned when it's played for everyone", cmWhite );
}

static void CM_DrawRemoveClip( float x, float y, float w, cmClip_t *clip ) {
	CM_Text( x, y + 2, va( S_COLOR_GREY "At %.1f s: drag it on the timeline", clip->start / 1000.0f ), cmWhite );
	y += CM_ROW_H + 4;
	if ( !CM_NpcEnt( clip->ent ) ) {
		CM_Text( x, y, S_COLOR_YELLOW "Only NPCs can be taken out: this is a player", cmWhite );
		return;
	}
	CM_Text( x, y, S_COLOR_GREY "Taken out of the game (npc kill) when", cmWhite );
	CM_Text( x, y + CM_ROW_H, S_COLOR_GREY "it's played for everyone", cmWhite );
}

static int CM_RecordStopKey( void );

static void CM_DrawRecordClip( float x, float y, float w, cmClip_t *clip ) {
	const cmRec_t *r = CM_Rec( &cm.cine, clip );

	CM_Text( x, y, r ? va( "Recorded, %.1f s" S_COLOR_GREY "  %i moments", CM_RecLen( &cm.cine, clip ) / 1000.0f, r->count ) : S_COLOR_YELLOW "Not recorded yet", cmWhite );
	y += CM_ROW_H + 4;
	if ( CM_Button( x, y, w, r ? "Record it again" : "Record it", qfalse, qtrue ) ) {
		CM_StartRecording( cm.clip );
		return;
	}
	y += CM_CTRL_H + CM_GAP + 2;
	CM_Text( x, y, S_COLOR_GREY "You act it out yourself after a 3-2-1:", cmWhite );
	CM_Text( x, y + CM_ROW_H, S_COLOR_GREY "moving, jumping, your saber, the force.", cmWhite );
	CM_Text( x, y + CM_ROW_H * 2, va( S_COLOR_GREY "%s ends it. They do it from where", Key_KeynumToString( CM_RecordStopKey() ) ), cmWhite );
	CM_Text( x, y + CM_ROW_H * 3, S_COLOR_GREY "they are then; drag it on the timeline", cmWhite );
}

static const char *cmExtraSabers[] = { "none", "a saber", "a staff", "two sabers" };
static const char *cmExtraColors[CL_EXTRA_NUM_COLORS] = { "red", "orange", "yellow", "green", "blue", "purple" };

// the model you wear, as everyone sees it
static const char *CM_MyModel( void ) {
	const int index = cl.gameState.stringOffsets[CS_PLAYERS + clc.clientNum];
	const char *model = index ? Info_ValueForKey( cl.gameState.stringData + index, "model" ) : "";

	return model[0] ? model : "kyle";
}

// a new extra where the crosshair is, facing the camera, in your model, a blue saber in hand
static void CM_AddExtra( void ) {
	char who[MAX_QPATH];
	cmEnt_t *e;
	int n = 1;

	do {
		Com_sprintf( who, sizeof( who ), "+Extra%i", n++ );
	} while ( CM_FindEnt( &cm.cine, who, qfalse ) >= 0 );
	if ( CM_AddEnt( who ) < 0 )
		return;
	e = &cm.cine.ents[cm.ent];
	Q_strncpyz( e->model, CM_MyModel(), sizeof( e->model ) );
	e->saber = CL_EXTRA_SABER_SINGLE;
	e->color = 4;
	if ( cm.haveAim ) {
		VectorCopy( cm.aim, e->home );
	} else {
		vec3_t forward;

		AngleVectors( cm.camAng, forward, NULL, NULL );
		VectorMA( cm.camOrg, 128.0f, forward, e->home );
	}
	e->home[2] += CM_LOOK_UP;
	e->yaw = AngleNormalize360( cm.camAng[YAW] + 180.0f );
	cm.pickNum = -1;
	Q_strncpyz( cm.pickWho, who, sizeof( cm.pickWho ) );
	cm.clip = -1;
	cm.extraOpen = qtrue;
	CM_Changed();
	CM_Message( "An extra: only screens with this client see it. Give it a name, a model, and actions" );
}

// an extra's name, model, saber and spot; where the next row goes
static float CM_DrawExtra( float x, float y, float w, cmEnt_t *e ) {
	const float half = ( w - 4 ) * 0.5f;
	int steps;

	if ( CM_Button( x, y, w, va( "Extra: %s, %s   %s", e->model[0] ? e->model : "kyle", cmExtraSabers[e->saber], cm.extraOpen ? "^" : "v" ),
		cm.extraOpen, qtrue ) )
		cm.extraOpen = (qboolean)!cm.extraOpen;
	y += CM_CTRL_H + CM_GAP;
	if ( !cm.extraOpen )
		return y;
	CM_Text( x, y + 2, "Name", cmDim );
	CM_TextBox( x + 40, y, w - 40, e->who, "+ and a name", CM_TYPE_EXTRANAME, NULL );
	y += CM_CTRL_H + CM_GAP;
	CM_Text( x, y + 2, "Model", cmDim );
	CM_TextBox( x + 40, y, w - 40 - 40, e->model, "model/skin", CM_TYPE_EXTRAMODEL, NULL );
	if ( CM_Button( x + w - 36, y, 36, "Mine", qfalse, qtrue ) ) {
		Q_strncpyz( e->model, CM_MyModel(), sizeof( e->model ) );
		CM_Changed();
	}
	y += CM_CTRL_H + CM_GAP;
	steps = CM_Stepper( x, y, w, "Holds", cmExtraSabers[e->saber] );
	if ( steps ) {
		e->saber = ( e->saber + steps + ARRAY_LEN( cmExtraSabers ) ) % ARRAY_LEN( cmExtraSabers );
		CM_Changed();
	}
	y += CM_CTRL_H + CM_GAP;
	if ( e->saber ) {
		steps = CM_Stepper( x, y, w, "Blades", cmExtraColors[e->color] );
		if ( steps ) {
			e->color = ( e->color + steps + CL_EXTRA_NUM_COLORS ) % CL_EXTRA_NUM_COLORS;
			CM_Changed();
		}
		y += CM_CTRL_H + CM_GAP;
	}
	if ( CM_Button( x, y, half, "Stand at crosshair", qfalse, cm.haveAim ) ) {
		VectorCopy( cm.aim, e->home );
		e->home[2] += CM_LOOK_UP;
		CM_Changed();
	}
	steps = CM_MiniStepper( x + half + 4, y, half, "Yaw", va( "%i", RC_Round( AngleNormalize360( e->yaw ) ) ) );
	if ( steps ) {
		e->yaw = AngleNormalize360( e->yaw + steps * 15.0f );
		CM_Changed();
	}
	y += CM_CTRL_H + CM_GAP;
	CM_Text( x, y, S_COLOR_GREY "Stands there until an action moves it", cmWhite );
	return y + CM_ROW_H + 4;
}

static void CM_DrawForceClip( float x, float y, float w, cmClip_t *clip ) {
	const cmCine_t *c = &cm.cine;
	int steps;

	steps = CM_Stepper( x, y, w, "Power", cmForces[clip->power].name );
	if ( steps ) {
		clip->power = ( clip->power + steps + ARRAY_LEN( cmForces ) ) % ARRAY_LEN( cmForces );
		CM_Changed();
	}
	y += CM_CTRL_H + CM_GAP;
	// at someone else in it, or at no one: in front of them
	steps = CM_Stepper( x, y, w, "At", clip->target >= 0 && clip->target < c->numEnts ? c->ents[clip->target].who : "no one, ahead" );
	if ( steps ) {
		int t = clip->target;

		for ( int n = 0; n <= c->numEnts; n++ ) {
			t += steps > 0 ? 1 : -1;
			if ( t >= c->numEnts )
				t = -1;
			else if ( t < -1 )
				t = c->numEnts - 1;
			if ( t != clip->ent )
				break;
		}
		clip->target = t;
		CM_Changed();
	}
	y += CM_CTRL_H + CM_GAP;
	if ( CM_Switch( x, y, w, "Acted", clip->forceAnims ? "both of them" : "no, the effect only" ) ) {
		clip->forceAnims = (qboolean)!clip->forceAnims;
		CM_Changed();
	}
	y += CM_CTRL_H + CM_GAP;
	CM_Text( x, y + 1, va( S_COLOR_GREY "%.1f s to %s: drag it on the timeline", clip->start / 1000.0f,
		clip->a.ms ? va( "%.1f s", ( clip->start + clip->a.ms ) / 1000.0f ) : "the end" ), cmWhite );
	CM_Text( x, y + 1 + CM_ROW_H, S_COLOR_GREY "Only seen, on screens with this client;", cmWhite );
	CM_Text( x, y + 1 + CM_ROW_H * 2, S_COLOR_GREY "no one is hurt. Seen when it plays", cmWhite );
}

// a look in a few words, for its bar
static const char *CM_LookSummary( const cmClip_t *clip ) {
	static char out[96];

	if ( clip->hidden )
		return "look: hidden";
	Q_strncpyz( out, "look:", sizeof( out ) );
	if ( clip->size != 100 )
		Q_strcat( out, sizeof( out ), va( " %i%%", clip->size ) );
	if ( clip->shell )
		Q_strcat( out, sizeof( out ), va( " %s", cmShells[clip->shell].name ) );
	if ( clip->saber )
		Q_strcat( out, sizeof( out ), va( " saber %s", cmSaberLooks[clip->saber] ) );
	if ( clip->saber != CM_SABER_OFF && clip->blade != 100 )
		Q_strcat( out, sizeof( out ), va( " blade %i%%", clip->blade ) );
	if ( clip->hold[0] )
		Q_strcat( out, sizeof( out ), va( " holds %s", COM_SkipPath( (char *)clip->hold ) ) );
	if ( clip->face )
		Q_strcat( out, sizeof( out ), va( " %s", cmFaces[clip->face].name ) );
	return !strcmp( out, "look:" ) ? "look: as they are" : out;
}

static float CM_ModelPicks( float x, float y, float w, char *model, int size, qboolean shown );

static void CM_DrawLookClip( float x, float y, float w, cmClip_t *clip ) {
	const float half = ( w - 4 ) * 0.5f;
	int steps;

	if ( CM_Switch( x, y, w, "Shown", clip->hidden ? "no, hidden" : "yes" ) ) {
		clip->hidden = (qboolean)!clip->hidden;
		CM_Changed();
	}
	y += CM_CTRL_H + CM_GAP;
	if ( !clip->hidden ) {
		steps = CM_Stepper( x, y, w, "Size", va( "%i%%", clip->size ) );
		if ( steps ) {
			clip->size = Com_Clampi( 25, 400, clip->size + steps * ( clip->size > 100 || ( clip->size == 100 && steps > 0 ) ? 10 : 5 ) );
			CM_Changed();
		}
		y += CM_CTRL_H + CM_GAP;
		steps = CM_Stepper( x, y, w, "Drawn", cmShells[clip->shell].name );
		if ( steps ) {
			clip->shell = ( clip->shell + steps + ARRAY_LEN( cmShells ) ) % ARRAY_LEN( cmShells );
			CM_Changed();
		}
		y += CM_CTRL_H + CM_GAP;
		steps = CM_Stepper( x, y, w, "Saber", cmSaberLooks[clip->saber] );
		if ( steps ) {
			clip->saber = ( clip->saber + steps + ARRAY_LEN( cmSaberLooks ) ) % ARRAY_LEN( cmSaberLooks );
			CM_Changed();
		}
		y += CM_CTRL_H + CM_GAP;
		if ( clip->saber != CM_SABER_OFF ) {
			steps = CM_Stepper( x, y, w, "Blades", va( "%i%% long", clip->blade ) );
			if ( steps ) {
				clip->blade = Com_Clampi( 25, 300, clip->blade + steps * 10 );
				CM_Changed();
			}
			y += CM_CTRL_H + CM_GAP;
		}
		steps = CM_Stepper( x, y, w, "Face", cmFaces[clip->face].name );
		if ( steps ) {
			clip->face = ( clip->face + steps + ARRAY_LEN( cmFaces ) ) % ARRAY_LEN( cmFaces );
			CM_Changed();
		}
		y += CM_CTRL_H + CM_GAP;
		// a model in a hand: a blaster, a datapad, a cup
		CM_Text( x, y + 2, "Holds", cmDim );
		CM_TextBox( x + 36, y, w - 36, clip->hold, "nothing: type to find a model", CM_TYPE_HOLD, NULL );
		y += CM_CTRL_H + 1;
		y = CM_ModelPicks( x, y, w, clip->hold, sizeof( clip->hold ), (qboolean)( cm.typing == CM_TYPE_HOLD ) );
		if ( clip->hold[0] ) {
			y += CM_GAP;
			if ( CM_Button( x, y, half, clip->holdHand ? "In the left hand" : "In the right hand", qfalse, qtrue ) ) {
				clip->holdHand = !clip->holdHand;
				CM_Changed();
			}
			steps = CM_MiniStepper( x + half + 4, y, half, "Pitch", va( "%i", RC_Round( clip->holdAng[PITCH] ) ) );
			CM_Adjust( &clip->holdAng[PITCH], steps, 15.0f, -180.0f, 180.0f );
			y += CM_CTRL_H + CM_GAP;
			steps = CM_MiniStepper( x, y, half, "Yaw", va( "%i", RC_Round( clip->holdAng[YAW] ) ) );
			CM_Adjust( &clip->holdAng[YAW], steps, 15.0f, -180.0f, 180.0f );
			steps = CM_MiniStepper( x + half + 4, y, half, "Roll", va( "%i", RC_Round( clip->holdAng[ROLL] ) ) );
			CM_Adjust( &clip->holdAng[ROLL], steps, 15.0f, -180.0f, 180.0f );
			y += CM_CTRL_H + CM_GAP;
		}
	}
	CM_Text( x, y + 1, va( S_COLOR_GREY "%.1f s to %s: drag it on the timeline", clip->start / 1000.0f,
		clip->a.ms ? va( "%.1f s", ( clip->start + clip->a.ms ) / 1000.0f ) : "the end" ), cmWhite );
	CM_Text( x, y + 1 + CM_ROW_H, S_COLOR_GREY "Seen on screens with this client; a saber", cmWhite );
	CM_Text( x, y + 1 + CM_ROW_H * 2, S_COLOR_GREY "that's off can't be lit here", cmWhite );
}

// the actions to add: a teleport is a position of one key
static const struct {
	const char	*label;
	int			kind;
	qboolean	tele;
} cmAdds[] = {
	{ "Move", CM_CLIP_MOVE, qfalse }, { "Position", CM_CLIP_PLACE, qfalse }, { "Teleport", CM_CLIP_PLACE, qtrue },
	{ "Record", CM_CLIP_RECORD, qfalse }, { "Animation", CM_CLIP_ANIM, qfalse }, { "Emote", CM_CLIP_EMOTE, qfalse },
	{ "Say", CM_CLIP_SAY, qfalse }, { "Look", CM_CLIP_LOOK, qfalse }, { "Force", CM_CLIP_FORCE, qfalse },
	{ "NPC command", CM_CLIP_NPCCMD, qfalse }, { "Spawn", CM_CLIP_SPAWN, qfalse }, { "Remove", CM_CLIP_REMOVE, qfalse } };
#define CM_ADD_LINES		( ( (int)ARRAY_LEN( cmAdds ) + 2 ) / 3 )
// the top of the panel's bottom part: Preview, Video, Play for everyone
#define CM_BOTTOM_Y			( SCREEN_HEIGHT - 4 - ( CM_CTRL_H + CM_GAP ) * 2 - CM_ROW_H )

static void CM_DrawEntitiesTab( float x, float y, float w ) {
	static const char *kinds[CM_NUM_CLIPS] = { "Move", "Animation", "Say", "Emote", "Position", "Record", "NPC command", "Spawn", "Remove", "Look", "Force" };
	cmCine_t *c = &cm.cine;
	const int rows = CM_Clip() && cm.ent >= 0 && CM_Clip()->ent == cm.ent ? 4 : 6;
	const cmPerson_t *picked = NULL;
	cmClip_t *clip;

	// the NPC list read now and then while it's looked at
	if ( !CL_NpcManager_Waiting() && cls.realtime - cm.listAsked > 30000 ) {
		cm.listAsked = cls.realtime;
		CL_NpcManager_RequestList();
	}
	CM_ListPeople();

	CM_Text( x, y + 2, va( "Players and NPCs (%i)", cmNumPeople ), cmWhite );
	if ( CM_Button( x + w - 56, y, 56, CL_NpcManager_Waiting() ? "Reading" : "Refresh", qfalse, (qboolean)!CL_NpcManager_Waiting() ) ) {
		cm.listAsked = cls.realtime;
		CL_NpcManager_RequestList();
	}
	y += CM_CTRL_H + 2;
	if ( cm.wheel && CM_InRect( x, y, w, rows * CM_ROW_H ) ) {
		cm.entScroll += cm.wheel;
		cm.wheel = 0;
	}
	cm.entScroll = Com_Clampi( 0, Q_max( 0, cmNumPeople - rows ), cm.entScroll );
	CM_Box( x, y, w, rows * CM_ROW_H + 2, cmPanelLight );
	const float lw = CM_ListBar( x, y, w, rows * CM_ROW_H + 2, cmNumPeople, rows, &cm.entScroll );
	for ( int i = 0; i < cmNumPeople; i++ ) {
		const cmPerson_t *p = &cmPeople[i];
		const int r = i - cm.entScroll;
		const float ry = y + 1 + r * CM_ROW_H;

		if ( CM_Picked( p ) )
			picked = p;
		if ( r < 0 || r >= rows )
			continue;
		if ( CM_Picked( p ) )
			CM_Fill( x + 1, ry, lw - 2, CM_ROW_H, cmHighlight );
		else if ( CM_InRect( x, ry, lw, CM_ROW_H ) )
			CM_Fill( x + 1, ry, lw - 2, CM_ROW_H, cmHoverBox );
		// who's in it already, marked
		if ( p->ent >= 0 )
			CM_Fill( x + 2, ry + 3, 3, CM_ROW_H - 6, cmTabLine );
		CM_TextClipped( x + 8, ry, lw - 12, p->label, cmWhite );
		if ( CM_Clicked( x, ry, lw, CM_ROW_H ) )
			CM_Pick( p );
	}
	if ( !cmNumPeople )
		CM_Text( x + 4, y + 2, S_COLOR_GREY "Nobody here", cmWhite );
	y += rows * CM_ROW_H + 4;
	// an NPC that isn't here yet, spawned in it: by the targetname it'll have
	CM_TextBox( x, y, w - 74, cm.newNpc, "an NPC to spawn: its targetname", CM_TYPE_NEWNPC, NULL );
	if ( CM_Button( x + w - 70, y, 70, "+ To spawn", qfalse, (qboolean)( cm.newNpc[0] && !Q_isanumber( cm.newNpc ) ) ) ) {
		if ( CM_AddEnt( cm.newNpc ) >= 0 ) {
			cm.newNpc[0] = '\0';
			cm.typing = CM_TYPE_NONE;
			CM_AddClip( CM_CLIP_SPAWN, qfalse );
		}
		return;
	}
	y += CM_CTRL_H + 4;
	// someone only this client draws, any model, where the crosshair is
	if ( CM_Button( x, y, w, "+ Extra: anyone, drawn by this client", qfalse, (qboolean)( c->numEnts < CM_MAX_ENTS ) ) ) {
		CM_AddExtra();
		return;
	}
	y += CM_CTRL_H + 4;
	if ( !picked ) {
		CM_Text( x, y, S_COLOR_GREY "Pick someone here, or click them", cmWhite );
		CM_Text( x, y + CM_ROW_H, S_COLOR_GREY "in the view", cmWhite );
		return;
	}

	// what they do: a new action at the moment picked, or after theirs
	if ( picked->ent >= 0 && CM_Button( x + w - 50, y, 50, "Take out", qfalse, qtrue ) ) {
		CM_RemoveEnt();
		return;
	}
	CM_TextClipped( x, y + 2, w - 56, picked->label, cmWhite );
	y += CM_CTRL_H + CM_GAP;
	if ( picked->ent >= 0 && CM_IsExtra( c->ents[picked->ent].who ) )
		y = CM_DrawExtra( x, y, w, &c->ents[picked->ent] );
	if ( CM_Button( x, y, w, va( "+ Add action at %.1f s   %s", CM_AddTime( CM_CLIP_MOVE ) / 1000.0f, cm.addOpen ? "^" : "v" ), cm.addOpen,
		(qboolean)( c->numClips < CM_MAX_CLIPS ) ) )
		cm.addOpen = (qboolean)!cm.addOpen;
	cm.addBtn[0] = x;
	cm.addBtn[1] = y;
	cm.addBtn[2] = w;
	cm.addBtn[3] = CM_CTRL_H;
	// the actions in a popup drawn over the rest: under the button, or over it if the panel's
	// bottom is in the way
	if ( cm.addOpen ) {
		const float h = CM_ADD_LINES * ( CM_CTRL_H + 1 ) + 7;
		float py = y + CM_CTRL_H + 1;

		if ( py + h > CM_BOTTOM_Y - 6 )
			py = y - 1 - h;
		cm.addPop[0] = x - 3;
		cm.addPop[1] = Q_max( 4.0f, py );
		cm.addPop[2] = w + 6;
		cm.addPop[3] = h;
		cm.addPlaced = qtrue;
	}
	y += CM_CTRL_H + 1 + CM_GAP + 3;

	clip = CM_Clip();
	if ( !clip || clip->ent != cm.ent || cm.ent < 0 ) {
		CM_Text( x, y, S_COLOR_GREY "Their actions are on the timeline:", cmWhite );
		CM_Text( x, y + CM_ROW_H, S_COLOR_GREY "click one to change it", cmWhite );
		return;
	}
	CM_Fill( x, y - 2, w, 1, cmBorder );
	CM_Text( x, y + 2, clip->kind == CM_CLIP_PLACE && clip->tele ? "Teleport" : kinds[Com_Clampi( 0, CM_NUM_CLIPS - 1, clip->kind )], cmWhite );
	if ( CM_Button( x + w - 50, y, 50, "Delete", qfalse, qtrue ) ) {
		CM_DeleteClip();
		return;
	}
	y += CM_CTRL_H + CM_GAP;
	if ( clip->kind == CM_CLIP_MOVE )
		CM_DrawMoveClip( x, y, w, clip );
	else if ( clip->kind == CM_CLIP_SAY )
		CM_DrawSayClip( x, y, w, clip );
	else if ( clip->kind == CM_CLIP_EMOTE )
		CM_DrawEmoteClip( x, y, w, clip );
	else if ( clip->kind == CM_CLIP_PLACE && clip->tele )
		CM_DrawTeleClip( x, y, w, clip );
	else if ( clip->kind == CM_CLIP_PLACE )
		CM_DrawPlaceClip( x, y, w, clip );
	else if ( clip->kind == CM_CLIP_RECORD )
		CM_DrawRecordClip( x, y, w, clip );
	else if ( clip->kind == CM_CLIP_NPCCMD )
		CM_DrawNpcCmdClip( x, y, w, clip );
	else if ( clip->kind == CM_CLIP_SPAWN )
		CM_DrawSpawnClip( x, y, w, clip );
	else if ( clip->kind == CM_CLIP_REMOVE )
		CM_DrawRemoveClip( x, y, w, clip );
	else if ( clip->kind == CM_CLIP_LOOK )
		CM_DrawLookClip( x, y, w, clip );
	else if ( clip->kind == CM_CLIP_FORCE )
		CM_DrawForceClip( x, y, w, clip );
	else
		CM_DrawAnimClip( x, y, w, clip );
}

/*
===============================================================================

The Effects tab: effects played at their moment, picked in the effect manager, moved in the
view; for everyone the cinematic plays for, as big as they're set

===============================================================================
*/

static cmFx_t *CM_Fx( void ) {
	return cm.fx >= 0 && cm.fx < cm.cine.numFx ? &cm.cine.fx[cm.fx] : NULL;
}

// the effect manager opened on the world; what it would send comes back here (CL_Cinematic_CaptureEffect)
static void CM_PickFx( int replace ) {
	cm.fxPicking = qtrue;
	cm.fxPicked = qfalse;
	cm.fxPickIndex = replace;
	cm.fxPickStart = cm.tlPinned ? cm.tlMs : 0;
	cm.fxPickPinned = cm.tlPinned;
	CL_RpCinematic_EditorClose();
	CL_EffectManager_OpenForCinematic();
	if ( !CL_EffectManager_Active() ) {
		cm.fxPicking = qfalse;
		cm.keepCam = qtrue;
		CL_RpCinematic_Edit( NULL, cm.fromStoryteller );
	}
}

qboolean CL_Cinematic_PickingEffect( void ) {
	return (qboolean)( cm.fxPicking && CL_EffectManager_Active() );
}

// the effect manager about to send: an effect put down is the cinematic's instead
qboolean CL_Cinematic_CaptureEffect( const char *cmd, float scale ) {
	char buf[MAX_STRING_CHARS], *argv[16];
	int argc = 0;
	cmFx_t *f = &cm.fxPickedFx;

	if ( !CL_Cinematic_PickingEffect() )
		return qfalse;
	// its size goes with it, not to the server
	if ( !Q_stricmpn( cmd, "rpnotify ", 9 ) && Q_stristr( cmd, "cmd:fxscale" ) )
		return qtrue;
	if ( Q_stricmpn( cmd, "rpeffect add ", 13 ) && Q_stricmpn( cmd, "rpeffect once ", 14 ) )
		return qfalse;
	if ( cm.fxPicked )
		return qtrue;	// one is enough
	Q_strncpyz( buf, cmd, sizeof( buf ) );
	for ( char *p = buf; *p && argc < (int)ARRAY_LEN( argv ); ) {
		while ( *p == ' ' )
			*p++ = '\0';
		if ( !*p )
			break;
		argv[argc++] = p;
		while ( *p && *p != ' ' )
			p++;
	}
	if ( argc < 3 || ( argc > 3 && !Q_stricmp( argv[3], "bolted" ) ) ) {
		Com_Printf( S_COLOR_YELLOW "Cinematic manager: an effect bolted to someone can't go in a cinematic; put one down at a place\n" );
		return qtrue;
	}
	memset( f, 0, sizeof( *f ) );
	Q_strncpyz( f->path, argv[2], sizeof( f->path ) );
	if ( argc >= 6 ) {
		VectorSet( f->org, (float)atof( argv[3] ), (float)atof( argv[4] ), (float)atof( argv[5] ) );
	} else {
		VectorCopy( cl.snap.ps.origin, f->org );
	}
	if ( argc >= 9 ) {
		VectorSet( f->ang, (float)atof( argv[6] ), (float)atof( argv[7] ), (float)atof( argv[8] ) );
		f->haveAngles = (qboolean)( f->ang[PITCH] || f->ang[YAW] || f->ang[ROLL] );
	}
	f->ms = !Q_stricmp( argv[1], "add" ) ? CM_FX_MS : 0;
	f->scale = scale > 0.0f ? Com_Clamp( 0.1f, 8.0f, scale ) : 1.0f;
	cm.fxPicked = qtrue;
	return qtrue;
}

// every frame: back from the effect manager once it's put one down, or was closed
static void CM_PickFxFrame( void ) {
	cmCine_t *c = &cm.cine;

	if ( !cm.fxPicking || ( CL_EffectManager_Active() && !cm.fxPicked ) )
		return;
	cm.fxPicking = qfalse;
	CL_EffectManager_Close();
	cm.keepCam = qtrue;
	CL_RpCinematic_Edit( NULL, cm.fromStoryteller );
	cm.tab = CM_TAB_EFFECTS;
	if ( cm.fxPickPinned ) {
		cm.tlMs = cm.fxPickStart;
		CM_TlPin();
	}
	if ( !cm.fxPicked )
		return;
	if ( cm.fxPickIndex >= 0 && cm.fxPickIndex < c->numFx ) {
		cmFx_t *f = &c->fx[cm.fxPickIndex];

		Q_strncpyz( f->path, cm.fxPickedFx.path, sizeof( f->path ) );
		VectorCopy( cm.fxPickedFx.org, f->org );
		VectorCopy( cm.fxPickedFx.ang, f->ang );
		f->haveAngles = cm.fxPickedFx.haveAngles;
		f->scale = cm.fxPickedFx.scale;
		cm.fx = cm.fxPickIndex;
	} else if ( c->numFx < CM_MAX_FX ) {
		cm.fxPickedFx.start = cm.fxPickStart;
		c->fx[c->numFx] = cm.fxPickedFx;
		cm.fx = c->numFx++;
	}
	CM_Changed();
	CM_Message( va( S_COLOR_GREEN "Effect %s at %.1f s: drag it in the view, J for the move arrows", COM_SkipPath( cm.fxPickedFx.path ), cm.fxPickStart / 1000.0f ) );
}

static void CM_DeleteFx( void ) {
	cmCine_t *c = &cm.cine;

	if ( !CM_Fx() )
		return;
	c->numFx--;
	memmove( &c->fx[cm.fx], &c->fx[cm.fx + 1], ( c->numFx - cm.fx ) * sizeof( c->fx[0] ) );
	cm.fx = Q_min( cm.fx, c->numFx - 1 );
	CM_Changed();
}

// the one picked, playing where it is while the tab's looked at
static void CM_PreviewFx( void ) {
	const cmFx_t *f = CM_Fx();
	int id;

	if ( !f || !cm.open || cm.tab != CM_TAB_EFFECTS || cmp.playing || cls.realtime < cm.fxPreviewNext )
		return;
	id = CM_FxId( f->path );
	if ( !id ) {
		cm.fxPreviewNext = cls.realtime + 1000;
		return;
	}
	CM_PlayFx( f, id );
	cm.fxPreviewNext = cls.realtime + Q_max( f->ms > 0 ? 0 : 1500, CM_FxPeriod( id ) );
}

static void CM_DrawEffectsTab( float x, float y, float w ) {
	cmCine_t *c = &cm.cine;
	const float half = ( w - 4 ) * 0.5f;
	const int rows = 6;
	cmFx_t *f;
	int steps;

	CM_Text( x, y + 2, va( "Effects (%i)", c->numFx ), cmWhite );
	y += CM_CTRL_H + 2;
	if ( cm.wheel && CM_InRect( x, y, w, rows * CM_ROW_H ) ) {
		cm.fxScroll += cm.wheel;
		cm.wheel = 0;
	}
	cm.fxScroll = Com_Clampi( 0, Q_max( 0, c->numFx - rows ), cm.fxScroll );
	CM_Box( x, y, w, rows * CM_ROW_H + 2, cmPanelLight );
	const float lw = CM_ListBar( x, y, w, rows * CM_ROW_H + 2, c->numFx, rows, &cm.fxScroll );
	for ( int i = 0; i < c->numFx; i++ ) {
		const cmFx_t *e = &c->fx[i];
		const int r = i - cm.fxScroll;
		const float ry = y + 1 + r * CM_ROW_H;

		if ( r < 0 || r >= rows )
			continue;
		if ( i == cm.fx )
			CM_Fill( x + 1, ry, lw - 2, CM_ROW_H, cmHighlight );
		else if ( CM_InRect( x, ry, lw, CM_ROW_H ) )
			CM_Fill( x + 1, ry, lw - 2, CM_ROW_H, cmHoverBox );
		CM_TextClipped( x + 4, ry, lw - 8, va( S_COLOR_CYAN "%.1fs " S_COLOR_WHITE "%s" S_COLOR_GREY "  %s", e->start / 1000.0f, COM_SkipPath( (char *)e->path ),
			e->ms > 0 ? va( "%.1f s", e->ms / 1000.0f ) : "once" ), cmWhite );
		if ( CM_Clicked( x, ry, lw, CM_ROW_H ) ) {
			cm.fx = i;
			cm.fxPreviewNext = 0;
		}
	}
	if ( !c->numFx )
		CM_Text( x + 4, y + 2, S_COLOR_GREY "None yet", cmWhite );
	y += rows * CM_ROW_H + 6;
	if ( CM_Button( x, y, w, va( "+ Pick an effect, at %.1f s...", ( cm.tlPinned ? cm.tlMs : 0 ) / 1000.0f ), qfalse, (qboolean)( c->numFx < CM_MAX_FX ) ) ) {
		CM_PickFx( -1 );
		return;
	}
	y += CM_CTRL_H + CM_GAP + 3;

	f = CM_Fx();
	if ( !f ) {
		CM_Text( x, y, S_COLOR_GREY "The effect manager opens: put one down", cmWhite );
		CM_Text( x, y + CM_ROW_H, S_COLOR_GREY "and it comes back here, at the moment", cmWhite );
		CM_Text( x, y + CM_ROW_H * 2, S_COLOR_GREY "picked on the timeline", cmWhite );
		return;
	}
	CM_Fill( x, y - 2, w, 1, cmBorder );
	CM_TextClipped( x, y + 2, w - 56, f->path, cmWhite );
	if ( CM_Button( x + w - 50, y, 50, "Delete", qfalse, qtrue ) ) {
		CM_DeleteFx();
		return;
	}
	y += CM_CTRL_H + CM_GAP;
	if ( CM_Button( x, y, half, "Pick again...", qfalse, qtrue ) ) {
		CM_PickFx( cm.fx );
		return;
	}
	if ( CM_Button( x + half + 4, y, half, "To the crosshair", qfalse, cm.haveAim ) ) {
		VectorCopy( cm.aim, f->org );
		CM_Changed();
	}
	y += CM_CTRL_H + CM_GAP;
	steps = CM_Stepper( x, y, w, "Starts", va( "%.1f s", f->start / 1000.0f ) );
	CM_AdjustMs( &f->start, steps, 0.1f, 0 );
	y += CM_CTRL_H + CM_GAP;
	steps = CM_Stepper( x, y, w, "Plays", f->ms > 0 ? va( "%.1f s, over and over", f->ms / 1000.0f ) : "once" );
	CM_AdjustMs( &f->ms, steps, 0.5f, 0 );
	y += CM_CTRL_H + CM_GAP;
	steps = CM_Stepper( x, y, w, "Size", va( "x%.2g", f->scale > 0.0f ? f->scale : 1.0f ) );
	if ( steps ) {
		f->scale = Com_Clamp( 0.1f, 8.0f, ( f->scale > 0.0f ? f->scale : 1.0f ) * powf( 1.25f, (float)steps ) );
		CM_Changed();
	}
	y += CM_CTRL_H + CM_GAP;
	if ( CM_Button( x, y, half, "Straight up", (qboolean)!f->haveAngles, qtrue ) && f->haveAngles ) {
		f->haveAngles = qfalse;
		VectorClear( f->ang );
		CM_Changed();
	}
	if ( CM_Button( x + half + 4, y, half, "Turned", f->haveAngles, qtrue ) && !f->haveAngles ) {
		f->haveAngles = qtrue;
		CM_Changed();
	}
	y += CM_CTRL_H + CM_GAP;
	if ( f->haveAngles ) {
		steps = CM_Stepper( x, y, w, "Pitch", va( "%i", RC_Round( AngleNormalize180( f->ang[PITCH] ) ) ) );
		CM_Adjust( &f->ang[PITCH], steps, 15.0f, -90.0f, 90.0f );
		y += CM_CTRL_H + CM_GAP;
		steps = CM_Stepper( x, y, w, "Yaw", va( "%i", RC_Round( AngleNormalize360( f->ang[YAW] ) ) ) );
		if ( steps ) {
			f->ang[YAW] = AngleNormalize360( f->ang[YAW] + steps * CM_Step( 15.0f ) );
			CM_Changed();
		}
		y += CM_CTRL_H + CM_GAP;
	}
	CM_Text( x, y, S_COLOR_GREY "Drag it in the view, or J: move arrows", cmWhite );
	CM_Text( x, y + CM_ROW_H, S_COLOR_GREY "Its bar on the timeline: drag it, or its end", cmWhite );
}

// what's under the mouse: a wall or the floor
static qboolean CM_CursorSurface( vec3_t out ) {
	const float nx = cm.cursorX / SCREEN_WIDTH * 2.0f - 1.0f, ny = cm.cursorY / SCREEN_HEIGHT * 2.0f - 1.0f;
	vec3_t dir, end;
	trace_t tr;

	if ( !cm.haveView )
		return qfalse;
	VectorCopy( cm.viewAxis[0], dir );
	VectorMA( dir, -nx * tanf( DEG2RAD( cm.viewFovX * 0.5f ) ), cm.viewAxis[1], dir );
	VectorMA( dir, -ny * tanf( DEG2RAD( cm.viewFovY * 0.5f ) ), cm.viewAxis[2], dir );
	VectorNormalize( dir );
	VectorMA( cm.viewOrg, CM_TRACE_DIST, dir, end );
	CM_BoxTrace( &tr, cm.viewOrg, end, vec3_origin, vec3_origin, 0, CONTENTS_SOLID | CONTENTS_TERRAIN, qfalse );
	if ( tr.allsolid || tr.fraction >= 1.0f )
		return qfalse;
	VectorMA( tr.endpos, 1.0f, tr.plane.normal, out );
	return qtrue;
}

// the effect held follows what's under the mouse
static void CM_FxDrag( void ) {
	cmFx_t *f = CM_Fx();
	vec3_t at;

	if ( !cm.fxDrag )
		return;
	if ( !cm.held[A_MOUSE1] || !f || cm.tab != CM_TAB_EFFECTS ) {
		cm.fxDrag = qfalse;
		return;
	}
	if ( CM_CursorSurface( at ) && !VectorCompare( at, f->org ) ) {
		VectorCopy( at, f->org );
		CM_Changed();
	}
}

/*
===============================================================================

The Sounds tab: sounds played at their moment, picked in the sound manager; heard everywhere,
or from a spot moved in the view; for everyone the cinematic plays for

===============================================================================
*/

static cmSnd_t *CM_Snd( void ) {
	return cm.snd >= 0 && cm.snd < cm.cine.numSnd ? &cm.cine.snd[cm.snd] : NULL;
}

// the sound manager opened; what it would send comes back here (CL_Cinematic_CaptureSound)
static void CM_PickSnd( int replace, qboolean music ) {
	cm.sndPicking = qtrue;
	cm.sndPicked = qfalse;
	cm.sndPickIndex = replace;
	cm.sndPickStart = cm.tlPinned ? cm.tlMs : 0;
	cm.sndPickPinned = cm.tlPinned;
	CL_RpCinematic_EditorClose();
	if ( music )
		CL_MusicManager_OpenForCinematic();
	else
		CL_SoundManager_OpenForCinematic();
	if ( !CL_SoundManager_Active() ) {
		cm.sndPicking = qfalse;
		cm.keepCam = qtrue;
		CL_RpCinematic_Edit( NULL, cm.fromStoryteller );
	}
}

// the sound manager about to send: the sound is the cinematic's instead, nothing goes out.
// Played once it's heard everywhere; looping at a spot, from there
qboolean CL_Cinematic_CaptureSound( const char *cmd ) {
	char buf[MAX_STRING_CHARS], *argv[8];
	int argc = 0;
	cmSnd_t *s = &cm.sndPickedSnd;

	if ( !cm.sndPicking || !CL_SoundManager_Active() )
		return qfalse;
	if ( cm.sndPicked )
		return qtrue;
	// a track from the music manager
	if ( !Q_stricmpn( cmd, "rpmusic ", 8 ) ) {
		memset( s, 0, sizeof( *s ) );
		Com_sprintf( s->path, sizeof( s->path ), "music/%s", cmd + 8 );
		s->music = qtrue;
		cm.sndPicked = qtrue;
		return qtrue;
	}
	if ( Q_stricmpn( cmd, "rpsound ", 8 ) )
		return qtrue;
	Q_strncpyz( buf, cmd, sizeof( buf ) );
	for ( char *p = buf; *p && argc < (int)ARRAY_LEN( argv ); ) {
		while ( *p == ' ' )
			*p++ = '\0';
		if ( !*p )
			break;
		argv[argc++] = p;
		while ( *p && *p != ' ' )
			p++;
	}
	if ( argc < 3 || ( Q_stricmp( argv[1], "once" ) && Q_stricmp( argv[1], "add" ) && Q_stricmp( argv[1], "local" ) ) )
		return qtrue;
	memset( s, 0, sizeof( *s ) );
	Q_strncpyz( s->path, argv[2], sizeof( s->path ) );
	s->ms = !Q_stricmp( argv[1], "add" ) ? CM_SND_MS : 0;
	if ( argc >= 6 ) {
		s->spot = qtrue;
		VectorSet( s->org, (float)atof( argv[3] ), (float)atof( argv[4] ), (float)atof( argv[5] ) );
	} else {
		VectorCopy( cl.snap.ps.origin, s->org );
	}
	cm.sndPicked = qtrue;
	return qtrue;
}

// every frame: back from the sound manager once a sound's sent, or it was closed
static void CM_PickSndFrame( void ) {
	cmCine_t *c = &cm.cine;

	if ( !cm.sndPicking || ( CL_SoundManager_Active() && !cm.sndPicked ) )
		return;
	cm.sndPicking = qfalse;
	CL_SoundManager_Close();
	CL_AnimManager_Close();
	CL_Presets_Close();
	CL_SpecialMoves_Close();
	CL_ChatLog_Close();
	cm.keepCam = qtrue;
	CL_RpCinematic_Edit( NULL, cm.fromStoryteller );
	cm.tab = CM_TAB_SOUNDS;
	if ( cm.sndPickPinned ) {
		cm.tlMs = cm.sndPickStart;
		CM_TlPin();
	}
	if ( !cm.sndPicked )
		return;
	if ( cm.sndPickIndex >= 0 && cm.sndPickIndex < c->numSnd ) {
		// picked again: the sound changes, when and where it plays stay
		Q_strncpyz( c->snd[cm.sndPickIndex].path, cm.sndPickedSnd.path, sizeof( c->snd[0].path ) );
		c->snd[cm.sndPickIndex].music = cm.sndPickedSnd.music;
		cm.snd = cm.sndPickIndex;
	} else if ( c->numSnd < CM_MAX_SND ) {
		cm.sndPickedSnd.start = cm.sndPickStart;
		c->snd[c->numSnd] = cm.sndPickedSnd;
		cm.snd = c->numSnd++;
	}
	CM_Changed();
	CM_Message( va( S_COLOR_GREEN "Sound %s at %.1f s", COM_SkipPath( cm.sndPickedSnd.path ), cm.sndPickStart / 1000.0f ) );
}

static void CM_DeleteSnd( void ) {
	cmCine_t *c = &cm.cine;

	if ( !CM_Snd() )
		return;
	c->numSnd--;
	memmove( &c->snd[cm.snd], &c->snd[cm.snd + 1], ( c->numSnd - cm.snd ) * sizeof( c->snd[0] ) );
	cm.snd = Q_min( cm.snd, c->numSnd - 1 );
	CM_Changed();
}

// the one picked, heard here alone, as it would be
static void CM_ListenSnd( const cmSnd_t *s ) {
	const sfxHandle_t h = s->music ? 0 : CM_SndHandle( s->path );

	if ( s->music ) {
		S_StopBackgroundTrack();
		S_StartBackgroundTrack( s->path, s->path, qfalse );
		cmMusicPlayed = qtrue;
		return;
	}
	if ( !h ) {
		CM_Message( va( S_COLOR_YELLOW "Can't play %s", s->path ) );
		return;
	}
	CM_PlaySnd( s, h );
}

static void CM_DrawSoundsTab( float x, float y, float w ) {
	cmCine_t *c = &cm.cine;
	const float half = ( w - 4 ) * 0.5f;
	const int rows = 6;
	cmSnd_t *s;
	int steps;

	CM_Text( x, y + 2, va( "Sounds (%i)", c->numSnd ), cmWhite );
	y += CM_CTRL_H + 2;
	if ( cm.wheel && CM_InRect( x, y, w, rows * CM_ROW_H ) ) {
		cm.sndScroll += cm.wheel;
		cm.wheel = 0;
	}
	cm.sndScroll = Com_Clampi( 0, Q_max( 0, c->numSnd - rows ), cm.sndScroll );
	CM_Box( x, y, w, rows * CM_ROW_H + 2, cmPanelLight );
	const float lw = CM_ListBar( x, y, w, rows * CM_ROW_H + 2, c->numSnd, rows, &cm.sndScroll );
	for ( int i = 0; i < c->numSnd; i++ ) {
		const cmSnd_t *e = &c->snd[i];
		const int r = i - cm.sndScroll;
		const float ry = y + 1 + r * CM_ROW_H;

		if ( r < 0 || r >= rows )
			continue;
		if ( i == cm.snd )
			CM_Fill( x + 1, ry, lw - 2, CM_ROW_H, cmHighlight );
		else if ( CM_InRect( x, ry, lw, CM_ROW_H ) )
			CM_Fill( x + 1, ry, lw - 2, CM_ROW_H, cmHoverBox );
		CM_TextClipped( x + 4, ry, lw - 8, va( S_COLOR_CYAN "%.1fs " S_COLOR_WHITE "%s" S_COLOR_GREY "  %s", e->start / 1000.0f, COM_SkipPath( (char *)e->path ),
			e->music ? "music" : e->ms > 0 ? va( "%.1f s", e->ms / 1000.0f ) : "once" ), cmWhite );
		if ( CM_Clicked( x, ry, lw, CM_ROW_H ) )
			cm.snd = i;
	}
	if ( !c->numSnd )
		CM_Text( x + 4, y + 2, S_COLOR_GREY "None yet", cmWhite );
	y += rows * CM_ROW_H + 6;
	if ( CM_Button( x, y, half, va( "+ Sound at %.1f s...", ( cm.tlPinned ? cm.tlMs : 0 ) / 1000.0f ), qfalse, (qboolean)( c->numSnd < CM_MAX_SND ) ) ) {
		CM_PickSnd( -1, qfalse );
		return;
	}
	if ( CM_Button( x + half + 4, y, half, va( "+ Music at %.1f s...", ( cm.tlPinned ? cm.tlMs : 0 ) / 1000.0f ), qfalse, (qboolean)( c->numSnd < CM_MAX_SND ) ) ) {
		CM_PickSnd( -1, qtrue );
		return;
	}
	y += CM_CTRL_H + CM_GAP + 3;

	s = CM_Snd();
	if ( !s ) {
		CM_Text( x, y, S_COLOR_GREY "The sound manager opens: pick one", cmWhite );
		CM_Text( x, y + CM_ROW_H, S_COLOR_GREY "and add it; it comes back here, at", cmWhite );
		CM_Text( x, y + CM_ROW_H * 2, S_COLOR_GREY "the moment picked on the timeline", cmWhite );
		return;
	}
	CM_Fill( x, y - 2, w, 1, cmBorder );
	CM_TextClipped( x, y + 2, w - 56, s->path, cmWhite );
	if ( CM_Button( x + w - 50, y, 50, "Delete", qfalse, qtrue ) ) {
		CM_DeleteSnd();
		return;
	}
	y += CM_CTRL_H + CM_GAP;
	if ( CM_Button( x, y, half, "Pick again...", qfalse, qtrue ) ) {
		CM_PickSnd( cm.snd, s->music );
		return;
	}
	if ( CM_Button( x + half + 4, y, half, "Listen", qfalse, qtrue ) )
		CM_ListenSnd( s );
	y += CM_CTRL_H + CM_GAP;
	steps = CM_Stepper( x, y, w, "Starts", va( "%.1f s", s->start / 1000.0f ) );
	CM_AdjustMs( &s->start, steps, 0.1f, 0 );
	y += CM_CTRL_H + CM_GAP;
	// a track: in place of the map's music, for a while or on to the end
	if ( s->music ) {
		steps = CM_Stepper( x, y, w, "Plays", s->ms > 0 ? va( "%.1f s, then the map's", s->ms / 1000.0f ) : "on, to the end" );
		CM_AdjustMs( &s->ms, steps, 1.0f, 0 );
		y += CM_CTRL_H + CM_GAP;
		CM_Text( x, y, S_COLOR_GREY "Music in place of the map's; the map's", cmWhite );
		CM_Text( x, y + CM_ROW_H, S_COLOR_GREY "comes back when it ends, or the cinematic does", cmWhite );
		return;
	}
	steps = CM_Stepper( x, y, w, "Plays", s->ms > 0 ? va( "%.1f s, over and over", s->ms / 1000.0f ) : "once" );
	CM_AdjustMs( &s->ms, steps, 0.5f, 0 );
	y += CM_CTRL_H + CM_GAP;
	if ( CM_Button( x, y, half, "Everywhere", (qboolean)!s->spot, qtrue ) && s->spot ) {
		s->spot = qfalse;
		CM_Changed();
	}
	if ( CM_Button( x + half + 4, y, half, "From a spot", s->spot, qtrue ) && !s->spot ) {
		s->spot = qtrue;
		if ( cm.haveAim )
			VectorCopy( cm.aim, s->org );
		CM_Changed();
	}
	y += CM_CTRL_H + CM_GAP;
	if ( s->spot ) {
		if ( CM_Button( x, y, w, "To the crosshair", qfalse, cm.haveAim ) ) {
			VectorCopy( cm.aim, s->org );
			CM_Changed();
		}
		y += CM_CTRL_H + CM_GAP;
		CM_Text( x, y, S_COLOR_GREY "Heard louder nearer it. Drag it", cmWhite );
		CM_Text( x, y + CM_ROW_H, S_COLOR_GREY "in the view, or J: move arrows", cmWhite );
		y += CM_ROW_H * 2;
	} else {
		CM_Text( x, y, S_COLOR_GREY "Heard the same wherever you are", cmWhite );
		y += CM_ROW_H;
	}
	CM_Text( x, y, S_COLOR_GREY "Its bar on the timeline: drag it, or its end", cmWhite );
}

/*
===============================================================================

Text: what's written on the screen at its moment

===============================================================================
*/

static cmTxt_t *CM_Txt( void ) {
	return cm.txt >= 0 && cm.txt < cm.cine.numTxt ? &cm.cine.txt[cm.txt] : NULL;
}

// a new one at the moment picked, of the kind of the one picked, its words typed next
static void CM_AddTxt( void ) {
	cmCine_t *c = &cm.cine;
	const cmTxt_t *like = CM_Txt();
	cmTxt_t *x;

	if ( c->numTxt >= CM_MAX_TXT ) {
		CM_Message( S_COLOR_YELLOW "A cinematic has 32 texts at most" );
		return;
	}
	x = &c->txt[c->numTxt];
	memset( x, 0, sizeof( *x ) );
	x->start = cm.tlPinned ? cm.tlMs : 0;
	x->ms = CM_TXT_MS;
	x->kind = like ? like->kind : CM_TXT_BOTTOM;
	x->size = like ? like->size : 1;
	x->typed = like ? like->typed : qfalse;
	if ( like && like->kind == CM_TXT_BOTTOM )
		Q_strncpyz( x->who, like->who, sizeof( x->who ) );
	cm.txt = c->numTxt++;
	cm.txtScroll = Q_max( 0, cm.txt - 3 );
	cm.typing = CM_TYPE_TXT;
	CM_Changed();
}

static void CM_DeleteTxt( void ) {
	cmCine_t *c = &cm.cine;

	if ( !CM_Txt() )
		return;
	cm.typing = CM_TYPE_NONE;
	c->numTxt--;
	memmove( &c->txt[cm.txt], &c->txt[cm.txt + 1], ( c->numTxt - cm.txt ) * sizeof( c->txt[0] ) );
	cm.txt = Q_min( cm.txt, c->numTxt - 1 );
	CM_Changed();
}

static void CM_DrawTextTab( float x, float y, float w ) {
	cmCine_t *c = &cm.cine;
	const float half = ( w - 4 ) * 0.5f;
	const int rows = 6;
	cmTxt_t *t;
	int steps;

	CM_Text( x, y + 2, va( "Texts (%i)", c->numTxt ), cmWhite );
	y += CM_CTRL_H + 2;
	if ( cm.wheel && CM_InRect( x, y, w, rows * CM_ROW_H ) ) {
		cm.txtScroll += cm.wheel;
		cm.wheel = 0;
	}
	cm.txtScroll = Com_Clampi( 0, Q_max( 0, c->numTxt - rows ), cm.txtScroll );
	CM_Box( x, y, w, rows * CM_ROW_H + 2, cmPanelLight );
	const float lw = CM_ListBar( x, y, w, rows * CM_ROW_H + 2, c->numTxt, rows, &cm.txtScroll );
	for ( int i = 0; i < c->numTxt; i++ ) {
		const cmTxt_t *e = &c->txt[i];
		const int r = i - cm.txtScroll;
		const float ry = y + 1 + r * CM_ROW_H;

		if ( r < 0 || r >= rows )
			continue;
		if ( i == cm.txt )
			CM_Fill( x + 1, ry, lw - 2, CM_ROW_H, cmHighlight );
		else if ( CM_InRect( x, ry, lw, CM_ROW_H ) )
			CM_Fill( x + 1, ry, lw - 2, CM_ROW_H, cmHoverBox );
		CM_TextClipped( x + 4, ry, lw - 8, va( S_COLOR_CYAN "%.1fs " S_COLOR_GREY "%s  " S_COLOR_WHITE "%s%s%s", e->start / 1000.0f, cmTxtKinds[e->kind],
			e->who[0] ? e->who : "", e->who[0] ? S_COLOR_WHITE ": " : "", e->text[0] ? e->text : S_COLOR_YELLOW "nothing yet" ), cmWhite );
		if ( CM_Clicked( x, ry, lw, CM_ROW_H ) ) {
			cm.txt = i;
			cm.typing = CM_TYPE_NONE;
		}
	}
	if ( !c->numTxt )
		CM_Text( x + 4, y + 2, S_COLOR_GREY "None yet", cmWhite );
	y += rows * CM_ROW_H + 6;
	if ( CM_Button( x, y, w, va( "+ Add text, at %.1f s", ( cm.tlPinned ? cm.tlMs : 0 ) / 1000.0f ), qfalse, (qboolean)( c->numTxt < CM_MAX_TXT ) ) )
		CM_AddTxt();
	y += CM_CTRL_H + CM_GAP + 3;

	t = CM_Txt();
	if ( !t ) {
		CM_Text( x, y, S_COLOR_GREY "Words on the screen at a moment: a line", cmWhite );
		CM_Text( x, y + CM_ROW_H, S_COLOR_GREY "said on the bottom bar, a title on the", cmWhite );
		CM_Text( x, y + CM_ROW_H * 2, S_COLOR_GREY "top one, a centre message, a notification", cmWhite );
		return;
	}
	CM_Fill( x, y - 2, w, 1, cmBorder );
	CM_TextBox( x, y, w - 56, t->text, "type it: ^1 colours work", CM_TYPE_TXT, NULL );
	if ( CM_Button( x + w - 50, y, 50, "Delete", qfalse, qtrue ) ) {
		CM_DeleteTxt();
		return;
	}
	y += CM_CTRL_H + CM_GAP;
	CM_Text( x, y + 2, "Said by", cmDim );
	CM_TextBox( x + 52, y, w - 52, t->who, "no one, or a name shown before it", CM_TYPE_TXTWHO, NULL );
	y += CM_CTRL_H + CM_GAP;
	// where it shows
	for ( int k = 0; k < CM_NUM_TXT_KINDS; k++ ) {
		const float bx = x + ( k & 1 ) * ( half + 4 ), by = y + ( k >> 1 ) * ( CM_CTRL_H + CM_GAP );

		if ( CM_Button( bx, by, half, cmTxtKinds[k], (qboolean)( t->kind == k ), qtrue ) && t->kind != k ) {
			t->kind = k;
			CM_Changed();
		}
	}
	y += ( CM_CTRL_H + CM_GAP ) * 2;
	steps = CM_Stepper( x, y, w, "Shows at", va( "%.1f s", t->start / 1000.0f ) );
	CM_AdjustMs( &t->start, steps, 0.1f, 0 );
	y += CM_CTRL_H + CM_GAP;
	steps = CM_Stepper( x, y, w, "For", va( "%.1f s", t->ms / 1000.0f ) );
	CM_AdjustMs( &t->ms, steps, 0.5f, 100 );
	y += CM_CTRL_H + CM_GAP;
	steps = CM_Stepper( x, y, w, "Size", cmTxtSizes[t->size] );
	if ( steps ) {
		t->size = Com_Clampi( 0, (int)ARRAY_LEN( cmTxtSizes ) - 1, t->size + steps );
		CM_Changed();
	}
	y += CM_CTRL_H + CM_GAP;
	if ( CM_Switch( x, y, w, "Typed out", t->typed ? "a letter at a time" : "no, all at once" ) ) {
		t->typed = (qboolean)!t->typed;
		CM_Changed();
	}
	y += CM_CTRL_H + CM_GAP;
	CM_Text( x, y, S_COLOR_GREY "Its bar on the timeline: drag it, or its end", cmWhite );
	CM_Text( x, y + CM_ROW_H, S_COLOR_GREY "Seen on screens with this client", cmWhite );
}

/*
===============================================================================

The World tab: props, models through keys while they last; and lights

===============================================================================
*/

static const struct {
	const char	*name;
	vec3_t		rgb;
} cmLightColors[] = {
	{ "White", { 1.0f, 1.0f, 1.0f } },
	{ "Warm", { 1.0f, 0.8f, 0.55f } },
	{ "Red", { 1.0f, 0.15f, 0.1f } },
	{ "Orange", { 1.0f, 0.5f, 0.1f } },
	{ "Yellow", { 1.0f, 0.9f, 0.25f } },
	{ "Green", { 0.2f, 1.0f, 0.3f } },
	{ "Cyan", { 0.2f, 0.9f, 1.0f } },
	{ "Blue", { 0.2f, 0.35f, 1.0f } },
	{ "Purple", { 0.7f, 0.25f, 1.0f } }
};

static std::vector<std::string> cmModels;	// every models/**/*.md3 but the players', to pick a prop's
static qboolean cmModelsIndexed;

static void CM_AddModelName( const char *name, void *ctx ) {
	std::string n( name );

	if ( !Q_stricmpn( name, "models/players/", 15 ) )
		return;
	for ( size_t i = 0; i < n.size(); i++ )
		n[i] = n[i] == '\\' ? '/' : (char)tolower( (unsigned char)n[i] );
	cmModels.push_back( n );
}

static void CM_IndexModels( void ) {
	if ( cmModelsIndexed )
		return;
	cmModelsIndexed = qtrue;
	cmModels.clear();
	FS_ListFilesRecursive( "models", ".md3", CM_AddModelName, NULL );
	std::sort( cmModels.begin(), cmModels.end() );
	cmModels.erase( std::unique( cmModels.begin(), cmModels.end() ), cmModels.end() );
}

// every model the filter matches, under a box being typed in: a click puts it in the box; where
// the next row goes
static float CM_ModelPicks( float x, float y, float w, char *model, int size, qboolean shown ) {
	static std::vector<int> picks;
	const int rows = 6;

	if ( !shown )
		return y;
	CM_IndexModels();
	picks.clear();
	for ( size_t i = 0; i < cmModels.size(); i++ ) {
		if ( !model[0] || Q_stristr( cmModels[i].c_str(), model ) )
			picks.push_back( (int)i );
	}
	if ( cm.wheel && CM_InRect( x, y, w, rows * CM_ROW_H ) ) {
		cm.propPickScroll += cm.wheel;
		cm.wheel = 0;
	}
	cm.propPickScroll = Com_Clampi( 0, Q_max( 0, (int)picks.size() - rows ), cm.propPickScroll );
	CM_Box( x, y, w, rows * CM_ROW_H + 2, cmPanelLight );
	const float lw = CM_ListBar( x, y, w, rows * CM_ROW_H + 2, (int)picks.size(), rows, &cm.propPickScroll );
	for ( int r = 0; r < rows && cm.propPickScroll + r < (int)picks.size(); r++ ) {
		const char *name = cmModels[picks[cm.propPickScroll + r]].c_str();
		const float ry = y + 1 + r * CM_ROW_H;

		if ( CM_InRect( x, ry, lw, CM_ROW_H ) )
			CM_Fill( x + 1, ry, lw - 2, CM_ROW_H, cmHoverBox );
		CM_TextClipped( x + 4, ry, lw - 8, name + 7, cmWhite );
		if ( CM_Clicked( x, ry, lw, CM_ROW_H ) ) {
			Q_strncpyz( model, name, size );
			cm.typing = CM_TYPE_NONE;
			CM_Changed();
		}
	}
	if ( picks.empty() )
		CM_Text( x + 4, y + 2, S_COLOR_GREY "No model has that in its name", cmWhite );
	return y + rows * CM_ROW_H + 6;
}

static cmProp_t *CM_Prop( void ) {
	return cm.prop >= 0 && cm.prop < cm.cine.numProps ? &cm.cine.props[cm.prop] : NULL;
}

static cmPlace_t *CM_PropKey( void ) {
	cmProp_t *pr = CM_Prop();

	return pr && cm.propKey >= 0 && cm.propKey < pr->numKeys ? &pr->keys[cm.propKey] : NULL;
}

static cmLight_t *CM_Light( void ) {
	return cm.light >= 0 && cm.light < cm.cine.numLights ? &cm.cine.lights[cm.light] : NULL;
}

// what the move arrows and a drag in the view move on the World tab: the prop's key picked, or
// the light picked, unless it follows someone
static float *CM_WorldOrg( void ) {
	if ( cm.worldPage == 0 )
		return CM_PropKey() ? CM_PropKey()->org : NULL;
	if ( cm.worldPage == 1 )
		return CM_Light() && !CM_Light()->who[0] ? CM_Light()->org : NULL;
	return NULL;
}

// where something new goes: on what the crosshair is on, else in front of the camera
static void CM_NewSpot( vec3_t out ) {
	vec3_t forward;

	if ( cm.haveAim ) {
		VectorCopy( cm.aim, out );
		return;
	}
	AngleVectors( cm.camAng, forward, NULL, NULL );
	VectorMA( cm.camOrg, 128.0f, forward, out );
}

// a key kept in order of when, and still the one picked
static void CM_SortPropKey( cmProp_t *pr ) {
	int i = cm.propKey;

	while ( i > 0 && pr->keys[i - 1].ms > pr->keys[i].ms ) {
		const cmPlace_t k = pr->keys[i];

		pr->keys[i] = pr->keys[i - 1];
		pr->keys[i - 1] = k;
		i--;
	}
	while ( i < pr->numKeys - 1 && pr->keys[i + 1].ms < pr->keys[i].ms ) {
		const cmPlace_t k = pr->keys[i];

		pr->keys[i] = pr->keys[i + 1];
		pr->keys[i + 1] = k;
		i++;
	}
	cm.propKey = i;
}

static void CM_AddProp( void ) {
	cmCine_t *c = &cm.cine;
	const cmProp_t *like = CM_Prop();
	cmProp_t *pr;

	if ( c->numProps >= CM_MAX_PROPS ) {
		CM_Message( S_COLOR_YELLOW "A cinematic has 16 props at most" );
		return;
	}
	pr = &c->props[c->numProps];
	memset( pr, 0, sizeof( *pr ) );
	pr->start = cm.tlPinned ? cm.tlMs : 0;
	pr->ms = CM_PROP_MS;
	pr->scale = 1.0f;
	pr->smooth = qtrue;
	if ( like )
		Q_strncpyz( pr->model, like->model, sizeof( pr->model ) );
	pr->numKeys = 1;
	CM_NewSpot( pr->keys[0].org );
	pr->keys[0].ang[YAW] = AngleNormalize360( cm.camAng[YAW] + 180.0f );
	cm.prop = c->numProps++;
	cm.propKey = 0;
	cm.propScroll = Q_max( 0, cm.prop - 3 );
	cm.typing = like ? CM_TYPE_NONE : CM_TYPE_PROPMODEL;
	CM_Changed();
	CM_Message( "A prop: pick its model, then add keys at other moments for where it goes" );
}

static void CM_AddLight( void ) {
	cmCine_t *c = &cm.cine;
	const cmLight_t *like = CM_Light();
	cmLight_t *l;

	if ( c->numLights >= CM_MAX_LIGHTS ) {
		CM_Message( S_COLOR_YELLOW "A cinematic has 16 lights at most" );
		return;
	}
	l = &c->lights[c->numLights];
	memset( l, 0, sizeof( *l ) );
	l->start = cm.tlPinned ? cm.tlMs : 0;
	l->ms = CM_LIGHT_MS;
	CM_NewSpot( l->org );
	l->org[2] += 32.0f;
	l->radius = like ? like->radius : 300.0f;
	if ( like )
		VectorCopy( like->color, l->color );
	else
		VectorCopy( cmLightColors[1].rgb, l->color );
	l->fx = like ? like->fx : CM_LIGHT_STEADY;
	cm.light = c->numLights++;
	cm.lightScroll = Q_max( 0, cm.light - 3 );
	CM_Changed();
}

static void CM_DeleteWorld( void ) {
	cmCine_t *c = &cm.cine;

	cm.typing = CM_TYPE_NONE;
	if ( cm.worldPage == 0 && CM_Prop() ) {
		c->numProps--;
		memmove( &c->props[cm.prop], &c->props[cm.prop + 1], ( c->numProps - cm.prop ) * sizeof( c->props[0] ) );
		cm.prop = Q_min( cm.prop, c->numProps - 1 );
		cm.propKey = 0;
		CM_Changed();
	} else if ( cm.worldPage == 1 && CM_Light() ) {
		c->numLights--;
		memmove( &c->lights[cm.light], &c->lights[cm.light + 1], ( c->numLights - cm.light ) * sizeof( c->lights[0] ) );
		cm.light = Q_min( cm.light, c->numLights - 1 );
		CM_Changed();
	} else if ( cm.worldPage == 2 && cm.screen >= 0 && cm.screen < c->numScreens ) {
		c->numScreens--;
		memmove( &c->screens[cm.screen], &c->screens[cm.screen + 1], ( c->numScreens - cm.screen ) * sizeof( c->screens[0] ) );
		cm.screen = Q_min( cm.screen, c->numScreens - 1 );
		CM_Changed();
	} else if ( cm.worldPage == 4 && cm.speed >= 0 && cm.speed < c->numSpeeds ) {
		c->numSpeeds--;
		memmove( &c->speeds[cm.speed], &c->speeds[cm.speed + 1], ( c->numSpeeds - cm.speed ) * sizeof( c->speeds[0] ) );
		cm.speed = Q_min( cm.speed, c->numSpeeds - 1 );
		CM_Changed();
	} else if ( cm.worldPage == 3 && cm.mapLook >= 0 && cm.mapLook < c->numMapLooks ) {
		c->numMapLooks--;
		memmove( &c->mapLooks[cm.mapLook], &c->mapLooks[cm.mapLook + 1], ( c->numMapLooks - cm.mapLook ) * sizeof( c->mapLooks[0] ) );
		cm.mapLook = Q_min( cm.mapLook, c->numMapLooks - 1 );
		CM_Changed();
	}
}

// a list's rows, the one picked lit; the one clicked, -1 none
static int CM_WorldList( float x, float y, float w, int rows, int count, int sel, int *scroll, const char *( *label )( int ) ) {
	int clicked = -1;

	if ( cm.wheel && CM_InRect( x, y, w, rows * CM_ROW_H ) ) {
		*scroll += cm.wheel;
		cm.wheel = 0;
	}
	*scroll = Com_Clampi( 0, Q_max( 0, count - rows ), *scroll );
	CM_Box( x, y, w, rows * CM_ROW_H + 2, cmPanelLight );
	const float lw = CM_ListBar( x, y, w, rows * CM_ROW_H + 2, count, rows, scroll );
	for ( int i = *scroll; i < count && i < *scroll + rows; i++ ) {
		const float ry = y + 1 + ( i - *scroll ) * CM_ROW_H;

		if ( i == sel )
			CM_Fill( x + 1, ry, lw - 2, CM_ROW_H, cmHighlight );
		else if ( CM_InRect( x, ry, lw, CM_ROW_H ) )
			CM_Fill( x + 1, ry, lw - 2, CM_ROW_H, cmHoverBox );
		CM_TextClipped( x + 4, ry, lw - 8, label( i ), cmWhite );
		if ( CM_Clicked( x, ry, lw, CM_ROW_H ) )
			clicked = i;
	}
	if ( !count )
		CM_Text( x + 4, y + 2, S_COLOR_GREY "None yet", cmWhite );
	return clicked;
}

static const char *CM_PropLabel( int i ) {
	const cmProp_t *pr = &cm.cine.props[i];

	return va( S_COLOR_CYAN "%.1fs " S_COLOR_WHITE "%s" S_COLOR_GREY "  %i key%s", pr->start / 1000.0f,
		pr->model[0] ? COM_SkipPath( (char *)pr->model ) : S_COLOR_YELLOW "no model yet", pr->numKeys, pr->numKeys == 1 ? "" : "s" );
}

static const char *CM_LightLabel( int i ) {
	const cmLight_t *l = &cm.cine.lights[i];

	return va( S_COLOR_CYAN "%.1fs " S_COLOR_WHITE "%s" S_COLOR_GREY "  reach %i%s", l->start / 1000.0f, cmLightFx[l->fx], RC_Round( l->radius ),
		l->who[0] ? va( ", on %s", l->who ) : "" );
}

static void CM_DrawPropsPage( float x, float y, float w ) {
	cmCine_t *c = &cm.cine;
	const float half = ( w - 4 ) * 0.5f;
	cmProp_t *pr;
	cmPlace_t *k;
	int clicked, steps;

	clicked = CM_WorldList( x, y, w, 4, c->numProps, cm.prop, &cm.propScroll, CM_PropLabel );
	if ( clicked >= 0 && clicked != cm.prop ) {
		cm.prop = clicked;
		cm.propKey = 0;
		cm.typing = CM_TYPE_NONE;
	}
	y += 4 * CM_ROW_H + 6;
	if ( CM_Button( x, y, w, va( "+ Prop at %.1f s, at the crosshair", ( cm.tlPinned ? cm.tlMs : 0 ) / 1000.0f ), qfalse, (qboolean)( c->numProps < CM_MAX_PROPS ) ) )
		CM_AddProp();
	y += CM_CTRL_H + CM_GAP + 3;
	if ( !( pr = CM_Prop() ) ) {
		CM_Text( x, y, S_COLOR_GREY "A model in the scene for a while: a ship", cmWhite );
		CM_Text( x, y + CM_ROW_H, S_COLOR_GREY "setting down, a speeder going by, a door", cmWhite );
		return;
	}
	CM_Fill( x, y - 2, w, 1, cmBorder );
	CM_TextBox( x, y, w - 54, pr->model, "type to find a model", CM_TYPE_PROPMODEL, NULL );
	if ( CM_Button( x + w - 50, y, 50, "Delete", qfalse, qtrue ) ) {
		CM_DeleteWorld();
		return;
	}
	y += CM_CTRL_H + 1;
	// the models there are, while it's typed in or has none
	y = CM_ModelPicks( x, y, w, pr->model, sizeof( pr->model ), (qboolean)( cm.typing == CM_TYPE_PROPMODEL || !pr->model[0] ) );
	steps = CM_Stepper( x, y, w, "Shows", pr->ms > 0 ? va( "%.1f s", pr->ms / 1000.0f ) : "to the end" );
	CM_AdjustMs( &pr->ms, steps, 0.5f, 0 );
	y += CM_CTRL_H + CM_GAP;
	steps = CM_MiniStepper( x, y, half, "Size", va( "%.2f", pr->scale ) );
	CM_Adjust( &pr->scale, steps, 0.1f, 0.05f, 50.0f );
	if ( CM_Button( x + half + 4, y, half, pr->smooth ? "Moves smoothly" : "Moves straight", pr->smooth, qtrue ) ) {
		pr->smooth = (qboolean)!pr->smooth;
		CM_Changed();
	}
	y += CM_CTRL_H + CM_GAP + 2;

	// its keys: where it is, turned how, at moments of it
	cm.propKey = Com_Clampi( 0, pr->numKeys - 1, cm.propKey );
	steps = CM_Stepper( x, y, w, "Key", va( "%i of %i, at %.1f s", cm.propKey + 1, pr->numKeys, ( pr->start + pr->keys[cm.propKey].ms ) / 1000.0f ) );
	if ( steps )
		cm.propKey = Com_Clampi( 0, pr->numKeys - 1, cm.propKey + steps );
	y += CM_CTRL_H + CM_GAP;
	{
		const int at = cm.tlPinned ? Q_max( 0, cm.tlMs - pr->start ) : pr->keys[pr->numKeys - 1].ms + 1000;

		if ( CM_Button( x, y, half, va( "+ Key at %.1f s", ( pr->start + at ) / 1000.0f ), qfalse, (qboolean)( pr->numKeys < CM_PROP_KEYS ) ) ) {
			cmPlace_t nk;

			nk.ms = at;
			CM_KeysAt( pr->keys, pr->numKeys, pr->smooth, at, nk.org, nk.ang );
			pr->keys[pr->numKeys] = nk;
			cm.propKey = pr->numKeys++;
			CM_SortPropKey( pr );
			CM_Changed();
		}
	}
	if ( CM_Button( x + half + 4, y, half, "Delete key", qfalse, (qboolean)( pr->numKeys > 1 ) ) ) {
		memmove( &pr->keys[cm.propKey], &pr->keys[cm.propKey + 1], ( pr->numKeys - cm.propKey - 1 ) * sizeof( pr->keys[0] ) );
		pr->numKeys--;
		cm.propKey = Q_min( cm.propKey, pr->numKeys - 1 );
		CM_Changed();
		return;
	}
	y += CM_CTRL_H + CM_GAP;
	k = &pr->keys[cm.propKey];
	if ( CM_Button( x, y, half, "To the crosshair", qfalse, cm.haveAim ) ) {
		VectorCopy( cm.aim, k->org );
		CM_Changed();
	}
	if ( cm.propKey > 0 ) {
		steps = CM_MiniStepper( x + half + 4, y, half, "At", va( "%.1f", ( pr->start + k->ms ) / 1000.0f ) );
		if ( steps ) {
			CM_AdjustMs( &k->ms, steps, 0.1f, 0 );
			CM_SortPropKey( pr );
			k = &pr->keys[cm.propKey];
		}
	}
	y += CM_CTRL_H + CM_GAP;
	steps = CM_MiniStepper( x, y, half, "Yaw", va( "%i", RC_Round( AngleNormalize360( k->ang[YAW] ) ) ) );
	CM_Adjust( &k->ang[YAW], steps, 15.0f, -3600.0f, 3600.0f );
	steps = CM_MiniStepper( x + half + 4, y, half, "Pitch", va( "%i", RC_Round( AngleNormalize180( k->ang[PITCH] ) ) ) );
	CM_Adjust( &k->ang[PITCH], steps, 15.0f, -3600.0f, 3600.0f );
	y += CM_CTRL_H + CM_GAP;
	steps = CM_MiniStepper( x, y, half, "Roll", va( "%i", RC_Round( AngleNormalize180( k->ang[ROLL] ) ) ) );
	CM_Adjust( &k->ang[ROLL], steps, 15.0f, -3600.0f, 3600.0f );
	steps = CM_MiniStepper( x + half + 4, y, half, "Up", va( "%i", RC_Round( k->org[2] ) ) );
	CM_Adjust( &k->org[2], steps, 8.0f, -65536.0f, 65536.0f );
	y += CM_CTRL_H + CM_GAP;
	CM_Text( x, y, S_COLOR_GREY "Drag the key in the view, or J for arrows;", cmWhite );
	CM_Text( x, y + CM_ROW_H, S_COLOR_GREY "its bar on the timeline: drag it, or its end", cmWhite );
}

static void CM_DrawLightsPage( float x, float y, float w ) {
	cmCine_t *c = &cm.cine;
	const float sw = ( w - 4.0f * ( ARRAY_LEN( cmLightColors ) - 1 ) ) / ARRAY_LEN( cmLightColors );
	cmLight_t *l;
	int clicked, steps;
	vec3_t at;

	clicked = CM_WorldList( x, y, w, 4, c->numLights, cm.light, &cm.lightScroll, CM_LightLabel );
	if ( clicked >= 0 && clicked != cm.light ) {
		cm.light = clicked;
		cm.typing = CM_TYPE_NONE;
	}
	y += 4 * CM_ROW_H + 6;
	if ( CM_Button( x, y, w, va( "+ Light at %.1f s, at the crosshair", ( cm.tlPinned ? cm.tlMs : 0 ) / 1000.0f ), qfalse, (qboolean)( c->numLights < CM_MAX_LIGHTS ) ) )
		CM_AddLight();
	y += CM_CTRL_H + CM_GAP + 3;
	if ( !( l = CM_Light() ) ) {
		CM_Text( x, y, S_COLOR_GREY "A light for a while: blaster fire, a fire,", cmWhite );
		CM_Text( x, y + CM_ROW_H, S_COLOR_GREY "an alarm, lightning, a torch someone holds", cmWhite );
		return;
	}
	CM_Fill( x, y - 2, w, 1, cmBorder );
	CM_Text( x, y + 2, "Light", cmWhite );
	if ( CM_Button( x + w - 50, y, 50, "Delete", qfalse, qtrue ) ) {
		CM_DeleteWorld();
		return;
	}
	y += CM_CTRL_H + CM_GAP;
	for ( int i = 0; i < (int)ARRAY_LEN( cmLightColors ); i++ ) {
		const float sx = x + i * ( sw + 4 );
		vec4_t col;

		VectorCopy( cmLightColors[i].rgb, col );
		col[3] = 1.0f;
		CM_Box( sx, y, sw, CM_CTRL_H, VectorCompare( cmLightColors[i].rgb, l->color ) ? cmHighlight : CM_InRect( sx, y, sw, CM_CTRL_H ) ? cmHoverBox : cmPanelLight );
		CM_Fill( sx + 3, y + 3, sw - 6, CM_CTRL_H - 6, col );
		if ( CM_Clicked( sx, y, sw, CM_CTRL_H ) ) {
			VectorCopy( cmLightColors[i].rgb, l->color );
			CM_Changed();
		}
	}
	y += CM_CTRL_H + CM_GAP;
	steps = CM_Stepper( x, y, w, "Reach", va( "%i", RC_Round( l->radius ) ) );
	CM_Adjust( &l->radius, steps, 25.0f, 8.0f, 2000.0f );
	y += CM_CTRL_H + CM_GAP;
	steps = CM_Stepper( x, y, w, "It's", cmLightFx[l->fx] );
	if ( steps ) {
		l->fx = ( l->fx + steps + CM_NUM_LIGHT_FX ) % CM_NUM_LIGHT_FX;
		CM_Changed();
	}
	y += CM_CTRL_H + CM_GAP;
	steps = CM_Stepper( x, y, w, "On for", l->ms > 0 ? va( "%.1f s", l->ms / 1000.0f ) : "to the end" );
	CM_AdjustMs( &l->ms, steps, 0.5f, 0 );
	y += CM_CTRL_H + CM_GAP;
	steps = CM_Stepper( x, y, w, "Fades", l->fade > 0 ? va( "%.2f s in and out", l->fade / 1000.0f ) : "no, at once" );
	CM_AdjustMs( &l->fade, steps, 0.25f, 0 );
	y += CM_CTRL_H + CM_GAP;
	CM_Text( x, y + 2, "On", cmDim );
	CM_TextBox( x + 30, y, w - 30 - 34, l->who, "nobody: at its spot", CM_TYPE_LIGHTWHO, NULL );
	if ( CM_Button( x + w - 30, y, 30, "Aim", qfalse, (qboolean)( CM_PersonAtCrosshair() != NULL ) ) ) {
		Q_strncpyz( l->who, CM_PersonAtCrosshair(), sizeof( l->who ) );
		CM_Changed();
	}
	y += CM_CTRL_H + CM_GAP;
	if ( l->who[0] ) {
		int num;

		if ( CM_Resolve( l->who, &num, 1 ) && RC_EntityOrigin( num, at ) )
			CM_TextClipped( x, y, w, va( S_COLOR_GREEN "%s" S_COLOR_GREY ": it goes where they go", l->who ), cmWhite );
		else
			CM_TextClipped( x, y, w, va( S_COLOR_YELLOW "%s isn't here: at its spot meanwhile", l->who ), cmWhite );
		y += CM_ROW_H + 2;
	}
	if ( CM_Button( x, y, w, "To the crosshair", qfalse, cm.haveAim ) ) {
		VectorCopy( cm.aim, l->org );
		l->org[2] += 32.0f;
		CM_Changed();
	}
	y += CM_CTRL_H + CM_GAP;
	CM_Text( x, y, S_COLOR_GREY "Drag it in the view, or J for arrows", cmWhite );
}

static cmScreen_t *CM_Screen( void ) {
	return cm.screen >= 0 && cm.screen < cm.cine.numScreens ? &cm.cine.screens[cm.screen] : NULL;
}

static const char *CM_ScreenLabel( int i ) {
	const cmScreen_t *sc = &cm.cine.screens[i];

	return va( S_COLOR_CYAN "%.1fs " S_COLOR_WHITE "%s" S_COLOR_GREY "  %i%%, %.1f s", sc->start / 1000.0f, cmScreenKinds[sc->kind], sc->strength, sc->ms / 1000.0f );
}

static void CM_DrawScreenPage( float x, float y, float w ) {
	cmCine_t *c = &cm.cine;
	const int numColors = (int)ARRAY_LEN( cmLightColors ) + 1;
	const float sw = ( w - 4.0f * ( numColors - 1 ) ) / numColors;
	cmScreen_t *sc;
	int clicked, steps;

	clicked = CM_WorldList( x, y, w, 4, c->numScreens, cm.screen, &cm.screenScroll, CM_ScreenLabel );
	if ( clicked >= 0 )
		cm.screen = clicked;
	y += 4 * CM_ROW_H + 6;
	if ( CM_Button( x, y, w, va( "+ Over the picture at %.1f s", ( cm.tlPinned ? cm.tlMs : 0 ) / 1000.0f ), qfalse, (qboolean)( c->numScreens < CM_MAX_SCREENS ) ) ) {
		const cmScreen_t *like = CM_Screen();

		sc = &c->screens[c->numScreens];
		memset( sc, 0, sizeof( *sc ) );
		sc->start = cm.tlPinned ? cm.tlMs : 0;
		sc->ms = CM_SCREEN_MS;
		sc->kind = like ? like->kind : CM_SCREEN_FLASH;
		if ( like )
			VectorCopy( like->color, sc->color );
		else
			VectorSet( sc->color, 1, 1, 1 );
		sc->strength = like ? like->strength : 100;
		sc->fade = like ? like->fade : 250;
		cm.screen = c->numScreens++;
		cm.screenScroll = Q_max( 0, cm.screen - 3 );
		CM_Changed();
	}
	y += CM_CTRL_H + CM_GAP + 3;
	if ( !( sc = CM_Screen() ) ) {
		CM_Text( x, y, S_COLOR_GREY "Over the whole picture: a flash dying", cmWhite );
		CM_Text( x, y + CM_ROW_H, S_COLOR_GREY "away, a colour over it, the edges dark;", cmWhite );
		CM_Text( x, y + CM_ROW_H * 2, S_COLOR_GREY "black, full: a fade out and back", cmWhite );
		return;
	}
	CM_Fill( x, y - 2, w, 1, cmBorder );
	steps = CM_Stepper( x, y, w - 54, "It's", cmScreenKinds[sc->kind] );
	if ( steps ) {
		sc->kind = ( sc->kind + steps + CM_NUM_SCREENS ) % CM_NUM_SCREENS;
		if ( sc->kind == CM_SCREEN_VIGNETTE )
			VectorClear( sc->color );
		CM_Changed();
	}
	if ( CM_Button( x + w - 50, y, 50, "Delete", qfalse, qtrue ) ) {
		CM_DeleteWorld();
		return;
	}
	y += CM_CTRL_H + CM_GAP;
	// black, then the lights' colours
	for ( int i = 0; i < numColors; i++ ) {
		const float sx = x + i * ( sw + 4 );
		vec3_t rgb;
		vec4_t col;

		if ( i )
			VectorCopy( cmLightColors[i - 1].rgb, rgb );
		else
			VectorClear( rgb );
		VectorCopy( rgb, col );
		col[3] = 1.0f;
		CM_Box( sx, y, sw, CM_CTRL_H, VectorCompare( rgb, sc->color ) ? cmHighlight : CM_InRect( sx, y, sw, CM_CTRL_H ) ? cmHoverBox : cmPanelLight );
		CM_Fill( sx + 3, y + 3, sw - 6, CM_CTRL_H - 6, col );
		if ( CM_Clicked( sx, y, sw, CM_CTRL_H ) ) {
			VectorCopy( rgb, sc->color );
			CM_Changed();
		}
	}
	y += CM_CTRL_H + CM_GAP;
	steps = CM_Stepper( x, y, w, "Strength", va( "%i%%", sc->strength ) );
	if ( steps ) {
		sc->strength = Com_Clampi( 5, 100, sc->strength + steps * 5 );
		CM_Changed();
	}
	y += CM_CTRL_H + CM_GAP;
	steps = CM_Stepper( x, y, w, "Lasts", va( "%.1f s", sc->ms / 1000.0f ) );
	CM_AdjustMs( &sc->ms, steps, 0.25f, 100 );
	y += CM_CTRL_H + CM_GAP;
	if ( sc->kind != CM_SCREEN_FLASH ) {
		steps = CM_Stepper( x, y, w, "Fades", sc->fade > 0 ? va( "%.2f s in and out", sc->fade / 1000.0f ) : "no, at once" );
		CM_AdjustMs( &sc->fade, steps, 0.25f, 0 );
		y += CM_CTRL_H + CM_GAP;
	}
	CM_Text( x, y, S_COLOR_GREY "Seen at the moment picked on the timeline;", cmWhite );
	CM_Text( x, y + CM_ROW_H, S_COLOR_GREY "its bar there: drag it, or its end", cmWhite );
}

// a row of colour swatches, black first if asked; qtrue if one's clicked into rgb
static qboolean CM_Swatches( float x, float y, float w, float *rgb, qboolean black ) {
	const int first = black ? 0 : 1, num = (int)ARRAY_LEN( cmLightColors ) + 1 - first;
	const float sw = ( w - 4.0f * ( num - 1 ) ) / num;
	qboolean clicked = qfalse;

	for ( int i = 0; i < num; i++ ) {
		const float sx = x + i * ( sw + 4 );
		vec3_t c;
		vec4_t col;

		if ( i + first )
			VectorCopy( cmLightColors[i + first - 1].rgb, c );
		else
			VectorClear( c );
		VectorCopy( c, col );
		col[3] = 1.0f;
		CM_Box( sx, y, sw, CM_CTRL_H, VectorCompare( c, rgb ) ? cmHighlight : CM_InRect( sx, y, sw, CM_CTRL_H ) ? cmHoverBox : cmPanelLight );
		CM_Fill( sx + 3, y + 3, sw - 6, CM_CTRL_H - 6, col );
		if ( CM_Clicked( sx, y, sw, CM_CTRL_H ) ) {
			VectorCopy( c, rgb );
			clicked = qtrue;
		}
	}
	return clicked;
}

static cmMapLook_t *CM_MapLook( void ) {
	return cm.mapLook >= 0 && cm.mapLook < cm.cine.numMapLooks ? &cm.cine.mapLooks[cm.mapLook] : NULL;
}

static const char *CM_MapLookLabel( int i ) {
	const cmMapLook_t *m = &cm.cine.mapLooks[i];

	return va( S_COLOR_CYAN "%.1fs " S_COLOR_WHITE "light %i%%" S_COLOR_GREY "%s", m->start / 1000.0f, m->light, m->fogDepth > 0.0f ? ", fog" : "" );
}

static void CM_DrawMapPage( float x, float y, float w ) {
	static const int volumes[] = { 100, 50, 25, 0 };
	cmCine_t *c = &cm.cine;
	cmMapLook_t *m;
	int clicked, steps;

	clicked = CM_WorldList( x, y, w, 3, c->numMapLooks, cm.mapLook, &cm.mapLookScroll, CM_MapLookLabel );
	if ( clicked >= 0 )
		cm.mapLook = clicked;
	y += 3 * CM_ROW_H + 6;
	if ( CM_Button( x, y, w, va( "+ The map's look at %.1f s", ( cm.tlPinned ? cm.tlMs : 0 ) / 1000.0f ), qfalse, (qboolean)( c->numMapLooks < CM_MAX_MAPLOOKS ) ) ) {
		const cmMapLook_t *like = CM_MapLook();

		m = &c->mapLooks[c->numMapLooks];
		if ( like ) {
			*m = *like;
		} else {
			memset( m, 0, sizeof( *m ) );
			m->light = 40;
			VectorSet( m->lightColor, 0.55f, 0.65f, 1.0f );
			VectorSet( m->skyColor, 0.3f, 0.35f, 0.6f );
			VectorSet( m->fogColor, 0.5f, 0.5f, 0.5f );
		}
		m->start = cm.tlPinned ? cm.tlMs : 0;
		m->ms = like ? like->ms : CM_MAPLOOK_MS;
		m->fade = like ? like->fade : 1500;
		cm.mapLook = c->numMapLooks++;
		cm.mapLookScroll = Q_max( 0, cm.mapLook - 2 );
		CM_Changed();
	}
	y += CM_CTRL_H + CM_GAP + 3;
	if ( ( m = CM_MapLook() ) != NULL ) {
		CM_Fill( x, y - 2, w, 1, cmBorder );
		steps = CM_Stepper( x, y, w - 54, "Light", va( "%i%%", m->light ) );
		if ( steps ) {
			m->light = Com_Clampi( 0, 300, m->light + steps * 10 );
			CM_Changed();
		}
		if ( CM_Button( x + w - 50, y, 50, "Delete", qfalse, qtrue ) ) {
			CM_DeleteWorld();
			return;
		}
		y += CM_CTRL_H + CM_GAP;
		if ( CM_Swatches( x, y, w, m->lightColor, qfalse ) )
			CM_Changed();
		y += CM_CTRL_H + CM_GAP;
		CM_Text( x, y + 2, "Sky", cmDim );
		if ( CM_Swatches( x + 30, y, w - 30, m->skyColor, qtrue ) )
			CM_Changed();
		y += CM_CTRL_H + CM_GAP;
		// none, then thicker and thicker
		{
			static const float depths[] = { 0.0f, 8000.0f, 5000.0f, 3000.0f, 2000.0f, 1400.0f, 1000.0f, 700.0f, 450.0f, 250.0f };
			int d = 0;

			while ( d < (int)ARRAY_LEN( depths ) - 1 && m->fogDepth > 0.0f && depths[d] != m->fogDepth && ( d == 0 || depths[d] > m->fogDepth ) )
				d++;
			steps = CM_Stepper( x, y, w, "Fog", m->fogDepth > 0.0f ? va( "see %i through it", RC_Round( m->fogDepth ) ) : "none" );
			if ( steps ) {
				m->fogDepth = depths[Com_Clampi( 0, (int)ARRAY_LEN( depths ) - 1, d + steps )];
				CM_Changed();
			}
		}
		y += CM_CTRL_H + CM_GAP;
		if ( m->fogDepth > 0.0f ) {
			if ( CM_Swatches( x, y, w, m->fogColor, qtrue ) )
				CM_Changed();
			y += CM_CTRL_H + CM_GAP;
		}
		steps = CM_Stepper( x, y, w, "Lasts", m->ms > 0 ? va( "%.1f s", m->ms / 1000.0f ) : "to the end" );
		CM_AdjustMs( &m->ms, steps, 0.5f, 0 );
		y += CM_CTRL_H + CM_GAP;
		steps = CM_Stepper( x, y, w, "Fades", m->fade > 0 ? va( "%.1f s in and out", m->fade / 1000.0f ) : "no, at once" );
		CM_AdjustMs( &m->fade, steps, 0.5f, 0 );
		y += CM_CTRL_H + CM_GAP;
		// a map editor layout of this map, shown while it lasts: props hidden or moved, textures
		{
			static char layouts[64][MAX_QPATH];
			static int numLayouts, nextList;
			int cur = -1;

			if ( cls.realtime >= nextList ) {
				numLayouts = CL_MapEditor_Layouts( layouts, ARRAY_LEN( layouts ) );
				nextList = cls.realtime + 2000;
			}
			for ( int i = 0; i < numLayouts && cur < 0; i++ ) {
				if ( !Q_stricmp( layouts[i], m->layout ) )
					cur = i;
			}
			steps = CM_Stepper( x, y, w, "Map edits", m->layout[0] ? m->layout : numLayouts ? "none" : "none: make a layout" );
			if ( steps && numLayouts ) {
				cur += steps;
				if ( cur < -1 )
					cur = numLayouts - 1;
				else if ( cur >= numLayouts )
					cur = -1;
				Q_strncpyz( m->layout, cur >= 0 ? layouts[cur] : "", sizeof( m->layout ) );
				CM_Changed();
			}
			y += CM_CTRL_H + CM_GAP;
		}
	} else {
		CM_Text( x, y, S_COLOR_GREY "The map's light, its sky and fog for a while:", cmWhite );
		CM_Text( x, y + CM_ROW_H, S_COLOR_GREY "night falling, an alarm's red, a haze", cmWhite );
		y += CM_ROW_H * 2 + 4;
	}
	// for all of it
	CM_Fill( x, y - 2, w, 1, cmBorder );
	{
		int v = 0;

		while ( v < (int)ARRAY_LEN( volumes ) - 1 && volumes[v] > c->worldVolume )
			v++;
		steps = CM_Stepper( x, y + 2, w, "Game sounds", c->worldVolume >= 100 ? "as they are" : c->worldVolume ? va( "%i%%, music as it is", c->worldVolume ) : "none, music only" );
		if ( steps ) {
			c->worldVolume = volumes[Com_Clampi( 0, (int)ARRAY_LEN( volumes ) - 1, v + steps )];
			CM_Changed();
		}
	}
}

static const char *CM_SpeedLabel( int i ) {
	const cmSpeed_t *sp = &cm.cine.speeds[i];

	return va( S_COLOR_CYAN "%.1fs " S_COLOR_WHITE "%i%%" S_COLOR_GREY "  for %.1f s", sp->start / 1000.0f, sp->speed, sp->ms / 1000.0f );
}

// stretches of it played slower or faster
static void CM_DrawTimePage( float x, float y, float w ) {
	static const int speeds[] = { 10, 25, 50, 75, 150, 200, 400 };
	cmCine_t *c = &cm.cine;
	cmSpeed_t *sp;
	int clicked, steps;

	clicked = CM_WorldList( x, y, w, 4, c->numSpeeds, cm.speed, &cm.speedScroll, CM_SpeedLabel );
	if ( clicked >= 0 )
		cm.speed = clicked;
	y += 4 * CM_ROW_H + 6;
	if ( CM_Button( x, y, w, va( "+ Slow motion at %.1f s", ( cm.tlPinned ? cm.tlMs : 0 ) / 1000.0f ), qfalse, (qboolean)( c->numSpeeds < CM_MAX_SPEEDS ) ) ) {
		sp = &c->speeds[c->numSpeeds];
		sp->start = cm.tlPinned ? cm.tlMs : 0;
		sp->ms = CM_SPEED_MS;
		sp->speed = 25;
		cm.speed = c->numSpeeds++;
		CM_Changed();
	}
	y += CM_CTRL_H + CM_GAP + 3;
	sp = cm.speed >= 0 && cm.speed < c->numSpeeds ? &c->speeds[cm.speed] : NULL;
	if ( !sp ) {
		CM_Text( x, y, S_COLOR_GREY "A stretch played slower, or faster: the", cmWhite );
		CM_Text( x, y + CM_ROW_H, S_COLOR_GREY "camera, those it moves, its extras, props,", cmWhite );
		CM_Text( x, y + CM_ROW_H * 2, S_COLOR_GREY "lights and words; not what the server moves", cmWhite );
		return;
	}
	CM_Fill( x, y - 2, w, 1, cmBorder );
	{
		int v = 0;

		while ( v < (int)ARRAY_LEN( speeds ) - 1 && speeds[v] < sp->speed )
			v++;
		steps = CM_Stepper( x, y, w - 54, "Speed", sp->speed < 100 ? va( "%i%%, slower", sp->speed ) : va( "%i%%, faster", sp->speed ) );
		if ( steps ) {
			sp->speed = speeds[Com_Clampi( 0, (int)ARRAY_LEN( speeds ) - 1, v + steps )];
			CM_Changed();
		}
	}
	if ( CM_Button( x + w - 50, y, 50, "Delete", qfalse, qtrue ) ) {
		CM_DeleteWorld();
		return;
	}
	y += CM_CTRL_H + CM_GAP;
	steps = CM_Stepper( x, y, w, "Lasts", va( "%.1f s of it", sp->ms / 1000.0f ) );
	CM_AdjustMs( &sp->ms, steps, 0.5f, 100 );
	y += CM_CTRL_H + CM_GAP;
	CM_Text( x, y, va( S_COLOR_GREY "Takes %.1f s to play", sp->ms * 100.0f / sp->speed / 1000.0f ), cmWhite );
}

static void CM_DrawWorldTab( float x, float y, float w ) {
	static const char *pages[] = { "Props", "Lights", "Screen", "Map", "Time" };
	const int page = CM_Tabs( x, y, w, pages, ARRAY_LEN( pages ), cm.worldPage );

	if ( page != cm.worldPage ) {
		cm.worldPage = page;
		cm.typing = CM_TYPE_NONE;
	}
	y += CM_CTRL_H + 6;
	if ( cm.worldPage == 0 )
		CM_DrawPropsPage( x, y, w );
	else if ( cm.worldPage == 1 )
		CM_DrawLightsPage( x, y, w );
	else if ( cm.worldPage == 2 )
		CM_DrawScreenPage( x, y, w );
	else if ( cm.worldPage == 3 )
		CM_DrawMapPage( x, y, w );
	else
		CM_DrawTimePage( x, y, w );
}

// the key or light held follows what's under the mouse
static void CM_WorldDrag( void ) {
	float *org = CM_WorldOrg();
	vec3_t at;

	if ( !cm.worldDrag )
		return;
	if ( !cm.held[A_MOUSE1] || !org || cm.tab != CM_TAB_WORLD ) {
		cm.worldDrag = qfalse;
		return;
	}
	if ( CM_CursorSurface( at ) ) {
		if ( cm.worldPage == 1 )
			at[2] += 32.0f;
		if ( !VectorCompare( at, org ) ) {
			VectorCopy( at, org );
			CM_Changed();
		}
	}
}

// the sound held follows what's under the mouse
static void CM_SndDrag( void ) {
	cmSnd_t *s = CM_Snd();
	vec3_t at;

	if ( !cm.sndDrag )
		return;
	if ( !cm.held[A_MOUSE1] || !s || !s->spot || cm.tab != CM_TAB_SOUNDS ) {
		cm.sndDrag = qfalse;
		return;
	}
	if ( CM_CursorSurface( at ) && !VectorCompare( at, s->org ) ) {
		VectorCopy( at, s->org );
		CM_Changed();
	}
}

// where the move arrows are: the effect or the spot sound picked on its tab
static float *CM_GzOrg( void ) {
	if ( cm.tab == CM_TAB_EFFECTS && CM_Fx() )
		return CM_Fx()->org;
	if ( cm.tab == CM_TAB_SOUNDS && CM_Snd() && CM_Snd()->spot )
		return CM_Snd()->org;
	if ( cm.tab == CM_TAB_WORLD )
		return CM_WorldOrg();
	return NULL;
}

static void CM_GzView( gzView_t *v ) {
	VectorCopy( cm.viewOrg, v->org );
	VectorCopy( cm.viewAxis[0], v->axis[0] );
	VectorCopy( cm.viewAxis[1], v->axis[1] );
	VectorCopy( cm.viewAxis[2], v->axis[2] );
	v->fovX = cm.viewFovX;
	v->fovY = cm.viewFovY;
}

// the arrows over the effect picked: the one under the mouse lit, the one held moving it
static void CM_GizmoFrame( void ) {
	float *org = CM_GzOrg();
	gzView_t v;

	cm.gzHot = -1;
	if ( !cm.gizmo || !org || !cm.haveView || cm.listing ) {
		cm.gzDragging = qfalse;
		return;
	}
	CM_GzView( &v );
	if ( cm.gzDragging && !cm.held[A_MOUSE1] )
		cm.gzDragging = qfalse;
	if ( cm.gzDragging ) {
		vec3_t to;

		GZ_Target( &cm.gzDrag, cm.cursorX, cm.cursorY, 1, to );
		if ( !VectorCompare( to, org ) ) {
			VectorCopy( to, org );
			CM_Changed();
		}
		cm.gzHot = cm.gzDrag.axis;
	} else {
		cm.gzHot = cm.cursorX < CM_PANEL_X ? GZ_Hit( &v, org, cm.cursorX, cm.cursorY ) : -1;
	}
	GZ_Draw( &v, org, cm.gzHot );
}

// the floor under the mouse, as the view sees it
static qboolean CM_CursorGround( vec3_t out ) {
	const float nx = cm.cursorX / SCREEN_WIDTH * 2.0f - 1.0f, ny = cm.cursorY / SCREEN_HEIGHT * 2.0f - 1.0f;
	vec3_t dir, end;
	trace_t tr;

	if ( !cm.haveView )
		return qfalse;
	VectorCopy( cm.viewAxis[0], dir );
	VectorMA( dir, -nx * tanf( DEG2RAD( cm.viewFovX * 0.5f ) ), cm.viewAxis[1], dir );
	VectorMA( dir, -ny * tanf( DEG2RAD( cm.viewFovY * 0.5f ) ), cm.viewAxis[2], dir );
	VectorNormalize( dir );
	VectorMA( cm.viewOrg, CM_TRACE_DIST, dir, end );
	CM_BoxTrace( &tr, cm.viewOrg, end, vec3_origin, vec3_origin, 0, CONTENTS_SOLID | CONTENTS_TERRAIN, qfalse );
	if ( tr.allsolid || tr.fraction >= 1.0f )
		return qfalse;
	VectorCopy( tr.endpos, out );
	// a wall hit: down it to the floor, from a step out from it
	if ( tr.plane.normal[2] < 0.7f ) {
		vec3_t from;
		trace_t down;

		VectorMA( tr.endpos, 16.0f, tr.plane.normal, from );
		VectorCopy( from, end );
		end[2] -= 4096.0f;
		CM_BoxTrace( &down, from, end, vec3_origin, vec3_origin, 0, CONTENTS_SOLID | CONTENTS_TERRAIN, qfalse );
		if ( !down.allsolid && !down.startsolid && down.fraction < 1.0f )
			VectorCopy( down.endpos, out );
	}
	return qtrue;
}

// what's under the mouse to focus on: someone, a little over their middle, or the wall or floor
static qboolean CM_CursorFocus( vec3_t out ) {
	const float nx = cm.cursorX / SCREEN_WIDTH * 2.0f - 1.0f, ny = cm.cursorY / SCREEN_HEIGHT * 2.0f - 1.0f;
	float best = 24.0f, sx, sy;
	qboolean found = qfalse;
	vec3_t dir, end;
	trace_t tr;

	if ( !cm.haveView )
		return qfalse;
	for ( int i = -1; i < cl.snap.numEntities; i++ ) {
		const entityState_t *es = i >= 0 ? &cl.parseEntities[( cl.snap.parseEntitiesNum + i ) & ( MAX_PARSE_ENTITIES - 1 )] : NULL;
		const int num = es ? es->number : clc.clientNum;
		vec3_t pos;

		if ( es && es->eType != ET_NPC && es->eType != ET_PLAYER )
			continue;
		if ( !CL_NpcWalk_PoseOf( num, pos ) )
			VectorCopy( es ? es->pos.trBase : cl.snap.ps.origin, pos );
		pos[2] += 16.0f;
		if ( CM_Project( pos, &sx, &sy ) && sqrtf( ( sx - cm.cursorX ) * ( sx - cm.cursorX ) + ( sy - cm.cursorY ) * ( sy - cm.cursorY ) ) < best ) {
			best = sqrtf( ( sx - cm.cursorX ) * ( sx - cm.cursorX ) + ( sy - cm.cursorY ) * ( sy - cm.cursorY ) );
			VectorCopy( pos, out );
			found = qtrue;
		}
	}
	if ( found )
		return qtrue;
	VectorCopy( cm.viewAxis[0], dir );
	VectorMA( dir, -nx * tanf( DEG2RAD( cm.viewFovX * 0.5f ) ), cm.viewAxis[1], dir );
	VectorMA( dir, -ny * tanf( DEG2RAD( cm.viewFovY * 0.5f ) ), cm.viewAxis[2], dir );
	VectorNormalize( dir );
	VectorMA( cm.viewOrg, CM_TRACE_DIST, dir, end );
	CM_BoxTrace( &tr, cm.viewOrg, end, vec3_origin, vec3_origin, 0, CONTENTS_SOLID | CONTENTS_TERRAIN, qfalse );
	if ( tr.allsolid || tr.fraction >= 1.0f )
		return qfalse;
	VectorCopy( tr.endpos, out );
	return qtrue;
}

// the moving focus's circle held: the key at the moment follows the mouse, made if there's none
static void CM_FocusDrag( void ) {
	rcShot_t *s = CM_Shot();
	vec3_t at;
	int k;

	if ( !cm.focusDrag )
		return;
	if ( !cm.held[A_MOUSE1] || !CM_FocusMoves( s ) ) {
		cm.focusDrag = qfalse;
		return;
	}
	if ( !CM_CursorFocus( at ) )
		return;
	k = CM_FocusKeyAt( s, cm.focusDragMs );
	if ( k < 0 ) {
		cm.focusDrag = qfalse;
		return;
	}
	cm.focusSel = k;
	if ( !VectorCompare( at, s->focus[k].org ) ) {
		VectorCopy( at, s->focus[k].org );
		CM_Changed();
	}
}

// the mouse on the circle: how far it's from the middle, 1 at its edge
static float CM_OnFocusCircle( float px, float py ) {
	const float dx = ( px - cm.focusX ) / Q_max( 4.0f, cm.focusRX ), dy = ( py - cm.focusY ) / Q_max( 4.0f, cm.focusRY );

	if ( !cm.focusShown )
		return 99.0f;
	// the middle can always be had
	if ( fabsf( px - cm.focusX ) < 8.0f && fabsf( py - cm.focusY ) < 8.0f )
		return 0.0f;
	return sqrtf( dx * dx + dy * dy );
}

// the moving focus in the view: a circle as wide as it's sharp, either side of it, where it is at
// the moment; its keys marked. The wheel on it makes it deeper or shallower
static void CM_DrawFocusCircle( void ) {
	static const vec4_t ring = { 0.3f, 0.9f, 1.0f, 0.9f }, keyCol = { 1.0f, 1.0f, 1.0f, 0.7f }, keySel = { 0.3f, 0.9f, 1.0f, 1.0f };
	rcShot_t *s = CM_Shot();
	const int ms = s ? CM_FocusMs( s ) : 0;
	vec3_t at, side, up;
	float range, sx, sy, ex, ey, ux, uy;

	cm.focusShown = qfalse;
	if ( cm.tab != CM_TAB_CAMERA || cm.page != CM_PAGE_FOCUS || cm.listing || !CM_FocusMoves( s ) || !s->numFocus || !cm.haveView )
		return;
	for ( int i = 0; i < s->numFocus; i++ ) {
		if ( !CM_Project( s->focus[i].org, &sx, &sy ) )
			continue;
		CM_Fill( sx - 2, sy - 2, 5, 5, i == cm.focusSel ? keySel : keyCol );
		CM_Text( sx + 4, sy - 14, va( "%s%.1fs", i == cm.focusSel ? S_COLOR_CYAN : "", s->focus[i].ms / 1000.0f ), cmWhite );
	}
	RC_FocusPoint( s, ms, at, &range );
	if ( !CM_Project( at, &sx, &sy ) )
		return;
	VectorMA( at, range, cm.viewAxis[1], side );
	VectorMA( at, range, cm.viewAxis[2], up );
	CM_Project( side, &ex, &ey );
	CM_Project( up, &ux, &uy );
	cm.focusShown = qtrue;
	cm.focusX = sx;
	cm.focusY = sy;
	cm.focusRX = Com_Clamp( 4.0f, SCREEN_WIDTH, sqrtf( ( ex - sx ) * ( ex - sx ) + ( ey - sy ) * ( ey - sy ) ) );
	cm.focusRY = Com_Clamp( 4.0f, SCREEN_HEIGHT, sqrtf( ( ux - sx ) * ( ux - sx ) + ( uy - sy ) * ( uy - sy ) ) );
	for ( int i = 0; i < 72; i++ ) {
		const float a = DEG2RAD( i * 5.0f ), px = sx + cosf( a ) * cm.focusRX;

		if ( px < CM_PANEL_X - 2 )
			CM_Fill( px - 0.5f, sy + sinf( a ) * cm.focusRY - 0.5f, 1.5f, 1.5f, ring );
	}
	CM_Fill( sx - 5, sy, 11, 1, ring );
	CM_Fill( sx, sy - 5, 1, 11, ring );
	CM_Text( sx + cm.focusRX * 0.72f + 3, sy - cm.focusRY * 0.72f - 12, va( S_COLOR_CYAN "sharp %i either side", RC_Round( range ) ), cmWhite );

	// the wheel on it: how deep, on the key at the moment
	if ( cm.wheel && !cm.flying && CM_OnFocusCircle( cm.cursorX, cm.cursorY ) <= 1.0f ) {
		const int k = CM_FocusKeyAt( s, ms );

		if ( k >= 0 ) {
			cm.focusSel = k;
			s->focus[k].range = Com_Clamp( 8.0f, 4096.0f, s->focus[k].range * ( cm.wheel > 0 ? 1.15f : 1.0f / 1.15f ) );
			CM_Changed();
		}
		cm.wheel = 0;
	}
}

// a click in the view: the moving focus's circle, held to drag; one of the move's spots, held to
// drag, or someone, picked
static void CM_ViewClick( void ) {
	cmClip_t *clip = CM_Clip();
	float best = 12.0f, sx, sy;
	int found = -1;

	// the move arrows, then the effects
	if ( cm.gizmo && cm.gzHot >= 0 && CM_GzOrg() ) {
		gzView_t v;

		CM_GzView( &v );
		GZ_Begin( &cm.gzDrag, &v, CM_GzOrg(), cm.gzHot, cm.clickX, cm.clickY );
		cm.gzDragging = cm.gzDrag.on;
		cm.click = qfalse;
		return;
	}
	if ( cm.tab == CM_TAB_EFFECTS ) {
		for ( int i = 0; i < cm.cine.numFx; i++ ) {
			if ( !CM_Project( cm.cine.fx[i].org, &sx, &sy ) || sqrtf( Square( sx - cm.clickX ) + Square( sy - cm.clickY ) ) > best )
				continue;
			best = sqrtf( Square( sx - cm.clickX ) + Square( sy - cm.clickY ) );
			found = i;
		}
		if ( found >= 0 ) {
			cm.fx = found;
			cm.fxDrag = qtrue;
			cm.fxPreviewNext = 0;
			cm.click = qfalse;
			return;
		}
	}
	if ( cm.tab == CM_TAB_SOUNDS ) {
		for ( int i = 0; i < cm.cine.numSnd; i++ ) {
			if ( !cm.cine.snd[i].spot || !CM_Project( cm.cine.snd[i].org, &sx, &sy ) || sqrtf( Square( sx - cm.clickX ) + Square( sy - cm.clickY ) ) > best )
				continue;
			best = sqrtf( Square( sx - cm.clickX ) + Square( sy - cm.clickY ) );
			found = i;
		}
		if ( found >= 0 ) {
			cm.snd = found;
			cm.sndDrag = qtrue;
			cm.click = qfalse;
			return;
		}
	}
	if ( cm.tab == CM_TAB_WORLD && cm.worldPage == 0 ) {
		const cmProp_t *pr = CM_Prop();
		int prop = -1, key = -1;

		// the keys of the one picked first, then where each prop is
		for ( int k = 0; pr && k < pr->numKeys; k++ ) {
			if ( !CM_Project( pr->keys[k].org, &sx, &sy ) || sqrtf( Square( sx - cm.clickX ) + Square( sy - cm.clickY ) ) > best )
				continue;
			best = sqrtf( Square( sx - cm.clickX ) + Square( sy - cm.clickY ) );
			prop = cm.prop;
			key = k;
		}
		for ( int i = 0; key < 0 && i < cm.cine.numProps; i++ ) {
			vec3_t org, ang;

			if ( !CM_PropAt( &cm.cine, &cm.cine.props[i], cm.tlPinned ? cm.tlMs : -1, org, ang ) || !CM_Project( org, &sx, &sy )
				|| sqrtf( Square( sx - cm.clickX ) + Square( sy - cm.clickY ) ) > best )
				continue;
			best = sqrtf( Square( sx - cm.clickX ) + Square( sy - cm.clickY ) );
			prop = i;
		}
		if ( prop >= 0 ) {
			if ( prop != cm.prop )
				cm.propKey = 0;
			cm.prop = prop;
			if ( key >= 0 ) {
				cm.propKey = key;
				cm.worldDrag = qtrue;
			}
			cm.click = qfalse;
			return;
		}
	}
	if ( cm.tab == CM_TAB_WORLD && cm.worldPage == 1 ) {
		for ( int i = 0; i < cm.cine.numLights; i++ ) {
			vec3_t org;

			CM_LightOrg( &cm.cine.lights[i], org );
			if ( !CM_Project( org, &sx, &sy ) || sqrtf( Square( sx - cm.clickX ) + Square( sy - cm.clickY ) ) > best )
				continue;
			best = sqrtf( Square( sx - cm.clickX ) + Square( sy - cm.clickY ) );
			found = i;
		}
		if ( found >= 0 ) {
			cm.light = found;
			cm.worldDrag = (qboolean)!cm.cine.lights[found].who[0];
			cm.click = qfalse;
			return;
		}
	}
	if ( CM_OnFocusCircle( cm.clickX, cm.clickY ) <= 1.0f ) {
		cm.focusDrag = qtrue;
		cm.focusDragMs = CM_FocusMs( CM_Shot() );
		cm.click = qfalse;
		return;
	}

	if ( cm.tab == CM_TAB_ENTITIES && clip && ( clip->kind == CM_CLIP_MOVE || clip->kind == CM_CLIP_PLACE ) ) {
		const int count = clip->kind == CM_CLIP_MOVE ? clip->numPts : clip->numKeys;

		for ( int j = 0; j < count; j++ ) {
			if ( CM_Project( clip->kind == CM_CLIP_MOVE ? clip->pts[j] : clip->keys[j].org, &sx, &sy ) && sqrtf( ( sx - cm.clickX ) * ( sx - cm.clickX ) + ( sy - cm.clickY ) * ( sy - cm.clickY ) ) < best ) {
				best = sqrtf( ( sx - cm.clickX ) * ( sx - cm.clickX ) + ( sy - cm.clickY ) * ( sy - cm.clickY ) );
				found = j;
			}
		}
		if ( found >= 0 ) {
			cm.spot = found;
			cm.spotDrag = qtrue;
			cm.click = qfalse;
			return;
		}
	}
	// someone, where they're drawn
	best = 28.0f;
	for ( int i = -1; i < cl.snap.numEntities; i++ ) {
		const entityState_t *es = i >= 0 ? &cl.parseEntities[( cl.snap.parseEntitiesNum + i ) & ( MAX_PARSE_ENTITIES - 1 )] : NULL;
		const int num = es ? es->number : clc.clientNum;
		vec3_t pos;

		if ( es && es->eType != ET_NPC && es->eType != ET_PLAYER )
			continue;
		if ( !CL_NpcWalk_PoseOf( num, pos ) )
			VectorCopy( es ? es->pos.trBase : cl.snap.ps.origin, pos );
		pos[2] += 8.0f;
		if ( CM_Project( pos, &sx, &sy ) && sqrtf( ( sx - cm.clickX ) * ( sx - cm.clickX ) + ( sy - cm.clickY ) * ( sy - cm.clickY ) ) < best ) {
			best = sqrtf( ( sx - cm.clickX ) * ( sx - cm.clickX ) + ( sy - cm.clickY ) * ( sy - cm.clickY ) );
			found = num;
		}
	}
	if ( found >= 0 ) {
		CM_PickNum( found );
		cm.tab = CM_TAB_ENTITIES;
		cm.click = qfalse;
	}
}

// where the mouse points at the height z
static qboolean CM_CursorPlane( float z, vec3_t out ) {
	const float nx = cm.cursorX / SCREEN_WIDTH * 2.0f - 1.0f, ny = cm.cursorY / SCREEN_HEIGHT * 2.0f - 1.0f;
	vec3_t dir;
	float t;

	if ( !cm.haveView )
		return qfalse;
	VectorCopy( cm.viewAxis[0], dir );
	VectorMA( dir, -nx * tanf( DEG2RAD( cm.viewFovX * 0.5f ) ), cm.viewAxis[1], dir );
	VectorMA( dir, -ny * tanf( DEG2RAD( cm.viewFovY * 0.5f ) ), cm.viewAxis[2], dir );
	if ( fabsf( dir[2] ) < 0.01f )
		return qfalse;
	t = ( z - cm.viewOrg[2] ) / dir[2];
	if ( t <= 0.0f || t > CM_TRACE_DIST )
		return qfalse;
	VectorMA( cm.viewOrg, t, dir, out );
	return qtrue;
}

// the spot held follows the floor under the mouse; a position's key stays at its height
static void CM_SpotDrag( void ) {
	cmClip_t *clip = CM_Clip();
	const int count = clip ? clip->kind == CM_CLIP_MOVE ? clip->numPts : clip->kind == CM_CLIP_PLACE ? clip->numKeys : 0 : 0;
	float *pt;
	vec3_t at;

	if ( !cm.spotDrag )
		return;
	if ( !cm.held[A_MOUSE1] || cm.spot < 0 || cm.spot >= count ) {
		cm.spotDrag = qfalse;
		return;
	}
	if ( clip->kind == CM_CLIP_PLACE ) {
		pt = clip->keys[cm.spot].org;
		if ( !CM_CursorPlane( pt[2], at ) )
			return;
	} else {
		pt = clip->pts[cm.spot];
		if ( !CM_CursorGround( at ) )
			return;
		at[2] += 25.0f;
	}
	if ( !VectorCompare( at, pt ) ) {
		VectorCopy( at, pt );
		CM_Changed();
	}
}

/*
===============================================================================

The timeline, under the view: the cinematic from its start to its end, a row for the
camera's shots and spots, and two for each one in it, their moves and their animations.
A click or a drag picks a moment; a shot's spots and end, and what people do, drag to
retime them.

===============================================================================
*/

#define CM_HELP_LINES		5
#define CM_TL_X				8.0f
#define CM_TL_W				( CM_PANEL_X - 16.0f )
#define CM_TL_LABEL_W		78.0f
#define CM_TL_RULER_H		11.0f
#define CM_TL_ROWS			7
#define CM_TL_BOTTOM		( SCREEN_HEIGHT - 12 - ( CM_HELP_LINES - 1 ) * CM_ROW_H - 4 )
#define CM_TL_GRAB			4.0f		// a spot or an end is grabbed this near
#define CM_TL_SNAP_MS		100
#define CM_TL_MARK_SNAP		150		// ms either side of a mark that snaps to it
#define CM_TL_MIN_MS		100			// the least between two spots

static vec4_t cmTlStripe	= { 1.0f, 1.0f, 1.0f, 0.04f };
static vec4_t cmTlShot		= { 0.85f, 0.7f, 0.15f, 0.45f };
static vec4_t cmTlShotOn	= { 0.95f, 0.78f, 0.2f, 0.75f };
static vec4_t cmTlTrans		= { 0.0f, 0.0f, 0.0f, 0.6f };
static vec4_t cmTlMove		= { 0.55f, 0.3f, 0.9f, 0.85f };
static vec4_t cmTlAnim		= { 0.9f, 0.5f, 0.12f, 0.85f };
static vec4_t cmTlSay		= { 0.2f, 0.6f, 0.35f, 0.85f };
static vec4_t cmTlPlace		= { 0.15f, 0.6f, 0.68f, 0.85f };
static vec4_t cmTlRec		= { 0.8f, 0.22f, 0.22f, 0.85f };
static vec4_t cmTlEmote		= { 0.8f, 0.35f, 0.6f, 0.85f };
static vec4_t cmTlAct		= { 0.35f, 0.45f, 0.75f, 0.85f };	// an NPC command, a spawn or a removal
static vec4_t cmTlMissing	= { 0.5f, 0.5f, 0.5f, 0.6f };
static vec4_t cmTlSel		= { 0.25f, 0.85f, 1.0f, 1.0f };
static vec4_t cmTlHead		= { 1.0f, 0.3f, 0.25f, 1.0f };
static vec4_t cmTlGhost		= { 0.3f, 1.0f, 0.45f, 0.5f };
static vec4_t cmTlLine		= { 1.0f, 1.0f, 1.0f, 0.12f };

typedef struct {
	float	x, w;			// the track
	int		span;			// ms across it
} cmTrack_t;

static float CM_TlX( const cmTrack_t *t, int ms ) {
	return t->x + t->w * Com_Clamp( 0.0f, 1.0f, (float)ms / t->span );
}

// the ms under x, past the end too when it's let
static int CM_TlMsAt( const cmTrack_t *t, float x, qboolean pastEnd ) {
	const float f = ( x - t->x ) / t->w;

	return (int)( Com_Clamp( 0.0f, pastEnd ? 4.0f : 1.0f, f ) * t->span + 0.5f );
}

// the marked moment within range of ms, -1 none
static int CM_MarkNear( int ms, int range ) {
	int best = -1;

	for ( int i = 0; i < cm.cine.numMarks; i++ ) {
		const int d = abs( cm.cine.marks[i] - ms );

		if ( d <= range && ( best < 0 || d < abs( cm.cine.marks[best] - ms ) ) )
			best = i;
	}
	return best;
}

// a moment dragged to: on a marked one near it, else on the tenth of a second
static int CM_TlSnap( int ms ) {
	const int m = CM_MarkNear( ms, CM_TL_MARK_SNAP );

	if ( m >= 0 )
		return cm.cine.marks[m];
	return Q_max( 0, ( ms + CM_TL_SNAP_MS / 2 ) / CM_TL_SNAP_MS * CM_TL_SNAP_MS );
}

// a second more at the moment picked, or a second less from it: the shot it's in longer or shorter
// there, and all after it along with it
static void CM_ChangeTime( int delta ) {
	cmCine_t *c = &cm.cine;
	rcShot_t *s;
	int local, shot;

	if ( !cm.tlPinned || !c->numShots || ( shot = CM_ShotAt( c, cm.tlMs, &local ) ) < 0 )
		return;
	s = &c->shots[shot];
	if ( s->orbit || s->numKeys == 1 ) {
		if ( s->ms + delta < 500 ) {
			CM_Message( S_COLOR_YELLOW "The shot's too short to take a second from" );
			return;
		}
		s->ms += delta;
	} else {
		int from = 0, i;

		// the stretch between two spots the moment is in
		for ( i = 1; i < s->numKeys - 1 && local > from + s->keys[i].ms; i++ )
			from += s->keys[i].ms;
		if ( s->keys[i].ms + delta < 100 ) {
			CM_Message( S_COLOR_YELLOW "Too short between these spots to take a second from" );
			return;
		}
		s->keys[i].ms += delta;
	}
	if ( delta > 0 )
		CM_ShiftClips( cm.tlMs, delta );
	else
		CM_ShiftClips( cm.tlMs - delta, delta );
	// the shot's new length is ours, not one to stretch what's in it over
	CM_SeeShots();
	CM_Changed();
	CM_Message( delta > 0 ? va( "A second more at %.1f s", cm.tlMs / 1000.0f ) : va( "A second less from %.1f s", cm.tlMs / 1000.0f ) );
}

// the marks either side of the moment, or the start and the end
static void CM_LoopSpan( int *from, int *to ) {
	const int total = CM_Total( &cm.cine ), at = cm.tlPinned ? cm.tlMs : 0;

	*from = 0;
	*to = total;
	for ( int i = 0; i < cm.cine.numMarks; i++ ) {
		const int m = cm.cine.marks[i];

		if ( m <= at && m > *from )
			*from = m;
		if ( m > at && m < *to )
			*to = m;
	}
}

// a moment marked where the timeline's at, or the mark there taken away
static void CM_ToggleMark( void ) {
	cmCine_t *c = &cm.cine;
	const int m = CM_MarkNear( cm.tlMs, 0 );

	if ( m >= 0 ) {
		c->numMarks--;
		memmove( &c->marks[m], &c->marks[m + 1], ( c->numMarks - m ) * sizeof( c->marks[0] ) );
		memmove( c->markNames[m], c->markNames[m + 1], ( c->numMarks - m ) * sizeof( c->markNames[0] ) );
		CM_Changed();
		return;
	}
	if ( c->numMarks >= CM_MAX_MARKS ) {
		CM_Message( S_COLOR_YELLOW "16 marks at most" );
		return;
	}
	c->marks[c->numMarks] = cm.tlMs;
	c->markNames[c->numMarks][0] = '\0';
	c->numMarks++;
	CM_Changed();
	CM_Message( va( "Marked %.1f s: what's dragged near it snaps to it", cm.tlMs / 1000.0f ) );
}

static int CM_ClipEnd( int i, int span ) {
	const cmClip_t *clip = &cm.cine.clips[i];

	if ( CM_Moves( clip->kind ) )
		return clip->start + ( cmEditPlans[i].count ? Q_max( 300, cmEditPlans[i].ms ) : 1000 );
	if ( clip->kind == CM_CLIP_SAY )
		return clip->start + CM_SAY_MS;
	if ( CM_Instant( clip->kind ) )
		return clip->start + CM_ACT_MS;
	return clip->a.ms > 0 ? clip->start + clip->a.ms : span;
}

static void CM_TlBar( float x0, float x1, float ry, const float *color, const char *label, qboolean sel ) {
	const float w = Q_max( 3.0f, x1 - x0 ), y = ry + 1, h = CM_ROW_H - 2;

	CM_Fill( x0, y, w, h, color );
	if ( sel ) {
		CM_Fill( x0, y, w, 1, cmTlSel );
		CM_Fill( x0, y + h - 1, w, 1, cmTlSel );
		CM_Fill( x0, y, 1, h, cmTlSel );
		CM_Fill( x0 + w - 1, y, 1, h, cmTlSel );
	}
	if ( label && w > 16.0f )
		CM_TextClipped( x0 + 3, ry, w - 5, label, cmWhite );
}

// a shot's end grabbed: the time a held spot or an orbit lasts, the last spot of a path
static qboolean CM_TlGrabShotEnd( int shot, int localEnd ) {
	rcShot_t *s = &cm.cine.shots[shot];

	if ( s->orbit || s->numKeys == 1 ) {
		cm.tlDrag = CM_DRAG_SHOT_END;
		cm.tlDragShot = shot;
		return qtrue;
	}
	cm.tlDrag = CM_DRAG_KEY;
	cm.tlDragShot = shot;
	cm.tlDragIndex = s->numKeys - 1;
	return (qboolean)( localEnd > 0 );
}

// a click on the timeline: a row's name picks it, a spot, an end or a bar is grabbed, anywhere
// else is the moment
static void CM_TlClickOne( const cmTrack_t *t, float rowsY, int numRows ) {
	cmCine_t *c = &cm.cine;
	const float cx = cm.clickX;
	const int r = cm.clickY >= rowsY ? (int)( ( cm.clickY - rowsY ) / CM_ROW_H ) : -1;
	const int row = r > 0 ? cm.tlScroll + r : r;
	const int at = CM_TlMsAt( t, cx, qfalse );
	const int fxRow = 1 + c->numEnts * 2, sndRow = fxRow + 1, txtRow = sndRow + 1, propRow = txtRow + 1, lightRow = propRow + 1, screenRow = lightRow + 1;
	const int mapRow = screenRow + 1, speedRow = mapRow + 1;

	cm.tlDragSpan = t->span;
	cm.typing = CM_TYPE_NONE;
	if ( row >= numRows )
		return;
	// the names
	if ( cx < t->x - 2 ) {
		if ( row == 0 ) {
			cm.tab = CM_TAB_CAMERA;
		} else if ( row == fxRow ) {
			cm.tab = CM_TAB_EFFECTS;
		} else if ( row == sndRow ) {
			cm.tab = CM_TAB_SOUNDS;
		} else if ( row == txtRow ) {
			cm.tab = CM_TAB_TEXT;
		} else if ( row == propRow || row == lightRow || row == screenRow || row == mapRow || row == speedRow ) {
			cm.tab = CM_TAB_WORLD;
			cm.worldPage = row == propRow ? 0 : row == lightRow ? 1 : row == screenRow ? 2 : row == mapRow ? 3 : 4;
		} else if ( row > 0 ) {
			cm.ent = ( row - 1 ) / 2;
			cm.clip = -1;
			cm.pickNum = -1;
			Q_strncpyz( cm.pickWho, c->ents[cm.ent].who, sizeof( cm.pickWho ) );
			cm.tab = CM_TAB_ENTITIES;
		}
		return;
	}
	if ( row == 0 && cm.clickY >= rowsY + CM_ROW_H * 0.5f ) {
		// a moving focus's keys, along the bottom of the row
		for ( int i = 0; i < c->numShots; i++ ) {
			const rcShot_t *s = &c->shots[i];
			const int from = CM_ShotStart( c, i );

			for ( int k = 0; CM_FocusMoves( s ) && k < s->numFocus; k++ ) {
				if ( fabsf( cx - CM_TlX( t, from + s->focus[k].ms ) ) > CM_TL_GRAB )
					continue;
				CM_SelectShot( i );
				cm.tab = CM_TAB_CAMERA;
				cm.page = CM_PAGE_FOCUS;
				CM_FocusSeeKey( s, k );
				cm.tlDrag = CM_DRAG_FOCUS;
				cm.tlDragShot = i;
				cm.tlDragIndex = k;
				return;
			}
		}
	}
	if ( row == 0 ) {
		// the spots and ends of the shots, the one shown first
		for ( int pass = 0; pass < 2; pass++ ) {
			for ( int i = 0; i < c->numShots; i++ ) {
				const rcShot_t *s = &c->shots[i];
				const int from = CM_ShotStart( c, i ), len = CM_ShotLen( s );

				if ( ( pass == 0 ) != ( i == cm.cur ) )
					continue;
				if ( fabsf( cx - CM_TlX( t, from + len ) ) <= CM_TL_GRAB ) {
					CM_SelectShot( i );
					cm.tab = CM_TAB_CAMERA;
					CM_TlGrabShotEnd( i, len );
					if ( cm.tlDrag == CM_DRAG_KEY )
						CM_SelectKey( cm.tlDragIndex );
					return;
				}
				for ( int k = 1; !s->orbit && k < s->numKeys - 1; k++ ) {
					if ( fabsf( cx - CM_TlX( t, from + CM_KeyMs( s, k ) ) ) > CM_TL_GRAB )
						continue;
					CM_SelectShot( i );
					cm.tab = CM_TAB_CAMERA;
					CM_SelectKey( k );
					cm.tlDrag = CM_DRAG_KEY;
					cm.tlDragShot = i;
					cm.tlDragIndex = k;
					return;
				}
			}
		}
	} else if ( row == fxRow ) {
		// an effect: its bar, or its end to make it last longer
		for ( int i = c->numFx - 1; i >= 0; i-- ) {
			const cmFx_t *f = &c->fx[i];
			const float x0 = CM_TlX( t, f->start ), x1 = CM_TlX( t, CM_FxEnd( f ) );

			if ( cx < x0 - 2 || cx > x1 + CM_TL_GRAB )
				continue;
			cm.fx = i;
			cm.tab = CM_TAB_EFFECTS;
			cm.tlDragIndex = i;
			if ( f->ms > 0 && cx >= x1 - CM_TL_GRAB && x1 - x0 > CM_TL_GRAB * 2 ) {
				cm.tlDrag = CM_DRAG_FX_END;
			} else {
				cm.tlDrag = CM_DRAG_FX;
				cm.tlDragGrab = at - f->start;
			}
			return;
		}
	} else if ( row == sndRow ) {
		// a sound: its bar, or its end to make it last longer
		for ( int i = c->numSnd - 1; i >= 0; i-- ) {
			const cmSnd_t *s = &c->snd[i];
			const float x0 = CM_TlX( t, s->start ), x1 = CM_TlX( t, CM_SndEnd( s ) );

			if ( cx < x0 - 2 || cx > x1 + CM_TL_GRAB )
				continue;
			cm.snd = i;
			cm.tab = CM_TAB_SOUNDS;
			cm.tlDragIndex = i;
			if ( s->ms > 0 && cx >= x1 - CM_TL_GRAB && x1 - x0 > CM_TL_GRAB * 2 ) {
				cm.tlDrag = CM_DRAG_SND_END;
			} else {
				cm.tlDrag = CM_DRAG_SND;
				cm.tlDragGrab = at - s->start;
			}
			return;
		}
	} else if ( row == txtRow ) {
		// a text: its bar, or its end to make it show longer
		for ( int i = c->numTxt - 1; i >= 0; i-- ) {
			const cmTxt_t *x = &c->txt[i];
			const float x0 = CM_TlX( t, x->start ), x1 = CM_TlX( t, x->start + x->ms );

			if ( cx < x0 - 2 || cx > x1 + CM_TL_GRAB )
				continue;
			cm.txt = i;
			cm.tab = CM_TAB_TEXT;
			cm.tlDragIndex = i;
			if ( cx >= x1 - CM_TL_GRAB && x1 - x0 > CM_TL_GRAB * 2 ) {
				cm.tlDrag = CM_DRAG_TXT_END;
			} else {
				cm.tlDrag = CM_DRAG_TXT;
				cm.tlDragGrab = at - x->start;
			}
			return;
		}
	} else if ( row == speedRow ) {
		for ( int i = c->numSpeeds - 1; i >= 0; i-- ) {
			const cmSpeed_t *sp = &c->speeds[i];
			const float x0 = CM_TlX( t, sp->start ), x1 = CM_TlX( t, sp->start + sp->ms );

			if ( cx < x0 - 2 || cx > x1 + CM_TL_GRAB )
				continue;
			cm.tab = CM_TAB_WORLD;
			cm.worldPage = 4;
			cm.speed = i;
			cm.tlDragIndex = i;
			if ( cx >= x1 - CM_TL_GRAB && x1 - x0 > CM_TL_GRAB * 2 ) {
				cm.tlDrag = CM_DRAG_SPEED_END;
			} else {
				cm.tlDrag = CM_DRAG_SPEED;
				cm.tlDragGrab = at - sp->start;
			}
			return;
		}
	} else if ( row == mapRow ) {
		for ( int i = c->numMapLooks - 1; i >= 0; i-- ) {
			const cmMapLook_t *m = &c->mapLooks[i];
			const float x0 = CM_TlX( t, m->start ), x1 = CM_TlX( t, CM_MapLookEnd( c, m ) );

			if ( cx < x0 - 2 || cx > x1 + CM_TL_GRAB )
				continue;
			cm.tab = CM_TAB_WORLD;
			cm.worldPage = 3;
			cm.mapLook = i;
			cm.tlDragIndex = i;
			if ( cx >= x1 - CM_TL_GRAB && x1 - x0 > CM_TL_GRAB * 2 ) {
				cm.tlDrag = CM_DRAG_MAPLOOK_END;
			} else {
				cm.tlDrag = CM_DRAG_MAPLOOK;
				cm.tlDragGrab = at - m->start;
			}
			return;
		}
	} else if ( row == screenRow ) {
		for ( int i = c->numScreens - 1; i >= 0; i-- ) {
			const cmScreen_t *sc = &c->screens[i];
			const float x0 = CM_TlX( t, sc->start ), x1 = CM_TlX( t, sc->start + sc->ms );

			if ( cx < x0 - 2 || cx > x1 + CM_TL_GRAB )
				continue;
			cm.tab = CM_TAB_WORLD;
			cm.worldPage = 2;
			cm.screen = i;
			cm.tlDragIndex = i;
			if ( cx >= x1 - CM_TL_GRAB && x1 - x0 > CM_TL_GRAB * 2 ) {
				cm.tlDrag = CM_DRAG_SCREEN_END;
			} else {
				cm.tlDrag = CM_DRAG_SCREEN;
				cm.tlDragGrab = at - sc->start;
			}
			return;
		}
	} else if ( row == propRow || row == lightRow ) {
		// a prop or a light: its bar, or its end to make it last longer
		const qboolean props = (qboolean)( row == propRow );

		for ( int i = ( props ? c->numProps : c->numLights ) - 1; i >= 0; i-- ) {
			const int start = props ? c->props[i].start : c->lights[i].start;
			const int end = props ? CM_PropEnd( c, &c->props[i] ) : CM_LightEnd( c, &c->lights[i] );
			const float x0 = CM_TlX( t, start ), x1 = CM_TlX( t, end );

			if ( cx < x0 - 2 || cx > x1 + CM_TL_GRAB )
				continue;
			cm.tab = CM_TAB_WORLD;
			cm.worldPage = props ? 0 : 1;
			if ( props ) {
				if ( cm.prop != i )
					cm.propKey = 0;
				cm.prop = i;
			} else {
				cm.light = i;
			}
			cm.tlDragIndex = i;
			if ( cx >= x1 - CM_TL_GRAB && x1 - x0 > CM_TL_GRAB * 2 ) {
				cm.tlDrag = props ? CM_DRAG_PROP_END : CM_DRAG_LIGHT_END;
			} else {
				cm.tlDrag = props ? CM_DRAG_PROP : CM_DRAG_LIGHT;
				cm.tlDragGrab = at - start;
			}
			return;
		}
	} else {
		// what someone does: the last drawn is on top
		const int e = ( row - 1 ) / 2, lane = ( row - 1 ) & 1;

		for ( int i = c->numClips - 1; i >= 0; i-- ) {
			const cmClip_t *clip = &c->clips[i];
			float x0, x1;

			if ( clip->ent != e || CM_Moves( clip->kind ) != ( lane == 0 ) )
				continue;
			x0 = CM_TlX( t, clip->start );
			x1 = CM_TlX( t, CM_ClipEnd( i, t->span ) );
			if ( cx < x0 - 2 || cx > x1 + CM_TL_GRAB )
				continue;
			cm.ent = e;
			cm.clip = i;
			cm.spot = -1;
			cm.pickNum = -1;
			Q_strncpyz( cm.pickWho, c->ents[e].who, sizeof( cm.pickWho ) );
			cm.tab = CM_TAB_ENTITIES;
			cm.tlDragIndex = i;
			if ( ( clip->kind == CM_CLIP_ANIM || clip->kind == CM_CLIP_EMOTE || clip->kind == CM_CLIP_LOOK || clip->kind == CM_CLIP_FORCE ) && cx >= x1 - CM_TL_GRAB
				&& x1 - x0 > CM_TL_GRAB * 2 ) {
				cm.tlDrag = CM_DRAG_CLIP_END;
			} else {
				cm.tlDrag = CM_DRAG_CLIP;
				cm.tlDragGrab = at - clip->start;
			}
			return;
		}
		if ( e < c->numEnts && cm.ent != e ) {
			cm.ent = e;
			cm.clip = -1;
			cm.pickNum = -1;
			Q_strncpyz( cm.pickWho, c->ents[e].who, sizeof( cm.pickWho ) );
		}
	}
	// the camera's row picks the shot there too
	if ( row == 0 && c->numShots ) {
		int local;

		cm.tab = CM_TAB_CAMERA;
		CM_SelectShot( CM_ShotAt( c, at, &local ) );
	}
	cm.tlMs = at;
	CM_TlPin();
	cm.tlDrag = CM_DRAG_HEAD;
}

// what's grabbed follows the mouse, until the button's let go
// the kinds of bars that can be picked together, as their drags have them
enum { CM_MK_CLIP, CM_MK_FX, CM_MK_SND, CM_MK_TXT, CM_MK_PROP, CM_MK_LIGHT, CM_MK_SCREEN, CM_MK_MAPLOOK, CM_MK_SPEED, CM_NUM_MK };

// where a bar starts, NULL if it's gone
static int *CM_MultiStart( int kind, int i ) {
	cmCine_t *c = &cm.cine;

	switch ( kind ) {
	case CM_MK_CLIP:	return i < c->numClips ? &c->clips[i].start : NULL;
	case CM_MK_FX:		return i < c->numFx ? &c->fx[i].start : NULL;
	case CM_MK_SND:		return i < c->numSnd ? &c->snd[i].start : NULL;
	case CM_MK_TXT:		return i < c->numTxt ? &c->txt[i].start : NULL;
	case CM_MK_PROP:	return i < c->numProps ? &c->props[i].start : NULL;
	case CM_MK_LIGHT:	return i < c->numLights ? &c->lights[i].start : NULL;
	case CM_MK_SCREEN:	return i < c->numScreens ? &c->screens[i].start : NULL;
	case CM_MK_MAPLOOK:	return i < c->numMapLooks ? &c->mapLooks[i].start : NULL;
	case CM_MK_SPEED:	return i < c->numSpeeds ? &c->speeds[i].start : NULL;
	default:			return NULL;
	}
}

// the kind of bar a drag moves by its start, -1 one that isn't
static int CM_DragKind( int drag ) {
	switch ( drag ) {
	case CM_DRAG_CLIP:		return CM_MK_CLIP;
	case CM_DRAG_FX:		return CM_MK_FX;
	case CM_DRAG_SND:		return CM_MK_SND;
	case CM_DRAG_TXT:		return CM_MK_TXT;
	case CM_DRAG_PROP:		return CM_MK_PROP;
	case CM_DRAG_LIGHT:		return CM_MK_LIGHT;
	case CM_DRAG_SCREEN:	return CM_MK_SCREEN;
	case CM_DRAG_MAPLOOK:	return CM_MK_MAPLOOK;
	case CM_DRAG_SPEED:		return CM_MK_SPEED;
	default:				return -1;
	}
}

static int CM_MultiFind( int kind, int i ) {
	for ( int m = 0; m < cm.numMulti; m++ ) {
		if ( cm.multiKind[m] == kind && cm.multiIndex[m] == i )
			return m;
	}
	return -1;
}

// a click on the timeline: Ctrl+click puts a bar with the others picked or takes it out; a drag
// of one of them takes them all
static void CM_TlClick( const cmTrack_t *t, float rowsY, int numRows ) {
	int kind, m;

	CM_TlClickOne( t, rowsY, numRows );
	kind = CM_DragKind( cm.tlDrag );
	if ( kind < 0 ) {
		if ( !CM_CtrlDown() && cm.tlDrag != CM_DRAG_HEAD )
			cm.numMulti = 0;
		return;
	}
	m = CM_MultiFind( kind, cm.tlDragIndex );
	if ( CM_CtrlDown() ) {
		if ( m >= 0 ) {
			cm.numMulti--;
			cm.multiKind[m] = cm.multiKind[cm.numMulti];
			cm.multiIndex[m] = cm.multiIndex[cm.numMulti];
		} else if ( cm.numMulti < (int)ARRAY_LEN( cm.multiKind ) ) {
			cm.multiKind[cm.numMulti] = kind;
			cm.multiIndex[cm.numMulti++] = cm.tlDragIndex;
		}
		cm.tlDrag = CM_DRAG_NONE;
		CM_Message( va( "%i picked: drag one to move them all; a click elsewhere lets them go", cm.numMulti ) );
		return;
	}
	if ( m < 0 ) {
		cm.numMulti = 0;
		return;
	}
	// where each started, to move them all by as much as the one dragged
	for ( int j = 0; j < cm.numMulti; j++ ) {
		const int *start = CM_MultiStart( cm.multiKind[j], cm.multiIndex[j] );

		cm.multiBase[j] = start ? *start : 0;
	}
}

// the others picked with the one dragged, moved by as much
static void CM_MultiDrag( void ) {
	const int kind = CM_DragKind( cm.tlDrag ), m = kind >= 0 ? CM_MultiFind( kind, cm.tlDragIndex ) : -1;
	const int *dragged = m >= 0 ? CM_MultiStart( kind, cm.tlDragIndex ) : NULL;
	int delta;

	if ( !dragged || cm.numMulti < 2 )
		return;
	delta = *dragged - cm.multiBase[m];
	for ( int j = 0; j < cm.numMulti; j++ ) {
		int *start = CM_MultiStart( cm.multiKind[j], cm.multiIndex[j] );

		if ( j != m && start && *start != Q_max( 0, cm.multiBase[j] + delta ) ) {
			*start = Q_max( 0, cm.multiBase[j] + delta );
			CM_Changed();
		}
	}
}

// the bars picked together, outlined where they're drawn
static void CM_MultiOutline( const cmTrack_t *t, float rowsY, int visRows ) {
	cmCine_t *c = &cm.cine;
	const int fxRow = 1 + c->numEnts * 2;
	static const vec4_t outline = { 0.3f, 0.95f, 1.0f, 1.0f };

	for ( int j = 0; j < cm.numMulti; j++ ) {
		const int i = cm.multiIndex[j];
		int start, end, row, r;
		float x0, x1, y;

		switch ( cm.multiKind[j] ) {
		case CM_MK_CLIP:
			if ( i >= c->numClips ) continue;
			start = c->clips[i].start; end = CM_ClipEnd( i, t->span ); row = 1 + c->clips[i].ent * 2 + ( CM_Moves( c->clips[i].kind ) ? 0 : 1 ); break;
		case CM_MK_FX:
			if ( i >= c->numFx ) continue;
			start = c->fx[i].start; end = CM_FxEnd( &c->fx[i] ); row = fxRow; break;
		case CM_MK_SND:
			if ( i >= c->numSnd ) continue;
			start = c->snd[i].start; end = CM_SndEnd( &c->snd[i] ); row = fxRow + 1; break;
		case CM_MK_TXT:
			if ( i >= c->numTxt ) continue;
			start = c->txt[i].start; end = start + c->txt[i].ms; row = fxRow + 2; break;
		case CM_MK_PROP:
			if ( i >= c->numProps ) continue;
			start = c->props[i].start; end = CM_PropEnd( c, &c->props[i] ); row = fxRow + 3; break;
		case CM_MK_LIGHT:
			if ( i >= c->numLights ) continue;
			start = c->lights[i].start; end = CM_LightEnd( c, &c->lights[i] ); row = fxRow + 4; break;
		case CM_MK_SCREEN:
			if ( i >= c->numScreens ) continue;
			start = c->screens[i].start; end = start + c->screens[i].ms; row = fxRow + 5; break;
		case CM_MK_MAPLOOK:
			if ( i >= c->numMapLooks ) continue;
			start = c->mapLooks[i].start; end = CM_MapLookEnd( c, &c->mapLooks[i] ); row = fxRow + 6; break;
		case CM_MK_SPEED:
			if ( i >= c->numSpeeds ) continue;
			start = c->speeds[i].start; end = start + c->speeds[i].ms; row = fxRow + 7; break;
		default:
			continue;
		}
		r = row - cm.tlScroll;
		if ( r < 1 || r >= visRows )
			continue;
		x0 = CM_TlX( t, start );
		x1 = Q_max( x0 + 3.0f, CM_TlX( t, end ) );
		y = rowsY + r * CM_ROW_H;
		CM_Fill( x0, y, x1 - x0, 2, outline );
		CM_Fill( x0, y + CM_ROW_H - 2, x1 - x0, 2, outline );
		CM_Fill( x0, y, 2, CM_ROW_H, outline );
		CM_Fill( x1 - 2, y, 2, CM_ROW_H, outline );
	}
}

static void CM_TlDrag( const cmTrack_t *t ) {
	cmCine_t *c = &cm.cine;
	const int at = CM_TlMsAt( t, cm.cursorX, (qboolean)( cm.tlDrag != CM_DRAG_HEAD ) );

	if ( !cm.held[A_MOUSE1] )
		cm.tlDrag = CM_DRAG_NONE;
	switch ( cm.tlDrag ) {
	case CM_DRAG_HEAD: {
		const int m = CM_MarkNear( at, (int)( 6.0f / t->w * t->span ) );
		int local;

		// on a mark when it's dragged near one
		cm.tlMs = m >= 0 ? c->marks[m] : at;
		// the camera tab follows the shot it's in
		if ( c->numShots && cm.tab == CM_TAB_CAMERA )
			CM_SelectShot( CM_ShotAt( c, at, &local ) );
		break;
	}
	case CM_DRAG_SHOT_END:
		if ( cm.tlDragShot < c->numShots ) {
			rcShot_t *s = &c->shots[cm.tlDragShot];
			const int ms = Q_max( 500, CM_TlSnap( at - CM_ShotStart( c, cm.tlDragShot ) ) );

			if ( ms != s->ms ) {
				s->ms = ms;
				CM_Changed();
			}
		}
		break;
	case CM_DRAG_KEY:
		if ( cm.tlDragShot < c->numShots ) {
			rcShot_t *s = &c->shots[cm.tlDragShot];
			const int k = cm.tlDragIndex, from = CM_ShotStart( c, cm.tlDragShot );

			if ( k > 0 && k < s->numKeys ) {
				// between the spots either side; the one after keeps where it is
				const int prev = CM_KeyMs( s, k - 1 ), next = k + 1 < s->numKeys ? CM_KeyMs( s, k + 1 ) : -1;
				int ms = Q_max( prev + CM_TL_MIN_MS, CM_TlSnap( at - from ) );

				if ( next >= 0 )
					ms = Q_min( next - CM_TL_MIN_MS, ms );
				if ( ms - prev != s->keys[k].ms ) {
					s->keys[k].ms = ms - prev;
					if ( next >= 0 )
						s->keys[k + 1].ms = next - ms;
					CM_Changed();
				}
			}
		}
		break;
	case CM_DRAG_FOCUS:
		if ( cm.tlDragShot < c->numShots && cm.tlDragIndex < c->shots[cm.tlDragShot].numFocus ) {
			rcShot_t *s = &c->shots[cm.tlDragShot];
			const int from = CM_ShotStart( c, cm.tlDragShot );
			const int ms = Com_Clampi( 0, CM_ShotLen( s ), CM_TlSnap( at - from ) );

			if ( ms != s->focus[cm.tlDragIndex].ms ) {
				s->focus[cm.tlDragIndex].ms = ms;
				cm.tlDragIndex = CM_SortFocus( s, cm.tlDragIndex );
				CM_Changed();
			}
			cm.focusSel = cm.tlDragIndex;
			cm.tlMs = from + ms;
		}
		break;
	case CM_DRAG_FX:
	case CM_DRAG_FX_END:
		if ( cm.tlDragIndex < c->numFx ) {
			cmFx_t *f = &c->fx[cm.tlDragIndex];
			const int ms = cm.tlDrag == CM_DRAG_FX ? CM_TlSnap( at - cm.tlDragGrab ) : Q_max( CM_TL_MIN_MS, CM_TlSnap( at - f->start ) );
			int *field = cm.tlDrag == CM_DRAG_FX ? &f->start : &f->ms;

			if ( ms != *field ) {
				*field = ms;
				CM_Changed();
			}
		}
		break;
	case CM_DRAG_SND:
	case CM_DRAG_SND_END:
		if ( cm.tlDragIndex < c->numSnd ) {
			cmSnd_t *s = &c->snd[cm.tlDragIndex];
			const int ms = cm.tlDrag == CM_DRAG_SND ? CM_TlSnap( at - cm.tlDragGrab ) : Q_max( CM_TL_MIN_MS, CM_TlSnap( at - s->start ) );
			int *field = cm.tlDrag == CM_DRAG_SND ? &s->start : &s->ms;

			if ( ms != *field ) {
				*field = ms;
				CM_Changed();
			}
		}
		break;
	case CM_DRAG_PROP:
	case CM_DRAG_PROP_END:
	case CM_DRAG_LIGHT:
	case CM_DRAG_LIGHT_END: {
		const qboolean props = (qboolean)( cm.tlDrag == CM_DRAG_PROP || cm.tlDrag == CM_DRAG_PROP_END );
		const qboolean end = (qboolean)( cm.tlDrag == CM_DRAG_PROP_END || cm.tlDrag == CM_DRAG_LIGHT_END );

		if ( cm.tlDragIndex < ( props ? c->numProps : c->numLights ) ) {
			int *startField = props ? &c->props[cm.tlDragIndex].start : &c->lights[cm.tlDragIndex].start;
			int *msField = props ? &c->props[cm.tlDragIndex].ms : &c->lights[cm.tlDragIndex].ms;
			const int ms = !end ? CM_TlSnap( at - cm.tlDragGrab ) : Q_max( CM_TL_MIN_MS, CM_TlSnap( at - *startField ) );
			int *field = end ? msField : startField;

			if ( ms != *field ) {
				*field = ms;
				CM_Changed();
			}
		}
		break;
	}
	case CM_DRAG_SPEED:
	case CM_DRAG_SPEED_END:
		if ( cm.tlDragIndex < c->numSpeeds ) {
			cmSpeed_t *sp = &c->speeds[cm.tlDragIndex];
			const int ms = cm.tlDrag == CM_DRAG_SPEED ? CM_TlSnap( at - cm.tlDragGrab ) : Q_max( CM_TL_MIN_MS, CM_TlSnap( at - sp->start ) );
			int *field = cm.tlDrag == CM_DRAG_SPEED ? &sp->start : &sp->ms;

			if ( ms != *field ) {
				*field = ms;
				CM_Changed();
			}
		}
		break;
	case CM_DRAG_MAPLOOK:
	case CM_DRAG_MAPLOOK_END:
		if ( cm.tlDragIndex < c->numMapLooks ) {
			cmMapLook_t *m = &c->mapLooks[cm.tlDragIndex];
			const int ms = cm.tlDrag == CM_DRAG_MAPLOOK ? CM_TlSnap( at - cm.tlDragGrab ) : Q_max( CM_TL_MIN_MS, CM_TlSnap( at - m->start ) );
			int *field = cm.tlDrag == CM_DRAG_MAPLOOK ? &m->start : &m->ms;

			if ( ms != *field ) {
				*field = ms;
				CM_Changed();
			}
		}
		break;
	case CM_DRAG_SCREEN:
	case CM_DRAG_SCREEN_END:
		if ( cm.tlDragIndex < c->numScreens ) {
			cmScreen_t *sc = &c->screens[cm.tlDragIndex];
			const int ms = cm.tlDrag == CM_DRAG_SCREEN ? CM_TlSnap( at - cm.tlDragGrab ) : Q_max( CM_TL_MIN_MS, CM_TlSnap( at - sc->start ) );
			int *field = cm.tlDrag == CM_DRAG_SCREEN ? &sc->start : &sc->ms;

			if ( ms != *field ) {
				*field = ms;
				CM_Changed();
			}
		}
		break;
	case CM_DRAG_TXT:
	case CM_DRAG_TXT_END:
		if ( cm.tlDragIndex < c->numTxt ) {
			cmTxt_t *x = &c->txt[cm.tlDragIndex];
			const int ms = cm.tlDrag == CM_DRAG_TXT ? CM_TlSnap( at - cm.tlDragGrab ) : Q_max( CM_TL_MIN_MS, CM_TlSnap( at - x->start ) );
			int *field = cm.tlDrag == CM_DRAG_TXT ? &x->start : &x->ms;

			if ( ms != *field ) {
				*field = ms;
				CM_Changed();
			}
		}
		break;
	case CM_DRAG_CLIP:
	case CM_DRAG_CLIP_END:
		if ( cm.tlDragIndex < c->numClips ) {
			cmClip_t *clip = &c->clips[cm.tlDragIndex];
			const int ms = cm.tlDrag == CM_DRAG_CLIP ? CM_TlSnap( at - cm.tlDragGrab ) : Q_max( CM_TL_MIN_MS, CM_TlSnap( at - clip->start ) );
			int *field = cm.tlDrag == CM_DRAG_CLIP ? &clip->start : &clip->a.ms;

			if ( ms != *field ) {
				*field = ms;
				CM_Changed();
			}
		}
		break;
	default:
		break;
	}
}

static void CM_DrawTimeline( void ) {
	cmCine_t *c = &cm.cine;
	const int fxRow = 1 + c->numEnts * 2, sndRow = fxRow + 1, txtRow = sndRow + 1, propRow = txtRow + 1, lightRow = propRow + 1, screenRow = lightRow + 1;
	const int mapRow = screenRow + 1, speedRow = mapRow + 1, numRows = speedRow + 1, visRows = Q_min( CM_TL_ROWS, numRows );
	const float x = CM_TL_X, w = CM_TL_W;
	const float boxH = CM_TL_RULER_H + Q_max( 2, visRows ) * CM_ROW_H + 3, boxY = CM_TL_BOTTOM - boxH, headY = boxY - CM_CTRL_H - 2;
	const float rowsY = boxY + 1 + CM_TL_RULER_H;
	const int total = CM_Total( c );
	cmTrack_t t;

	if ( cm.tlHidden )
		return;
	t.x = x + CM_TL_LABEL_W;
	t.w = w - CM_TL_LABEL_W - 6;
	t.span = cm.tlDrag ? cm.tlDragSpan : Q_max( 1000, total );
	CM_EditPlans( qfalse );
	CM_TlDrag( &t );
	CM_MultiDrag();
	if ( !cm.tlDrag )
		cm.tlMs = Com_Clampi( 0, t.span, cm.tlMs );

	// what it's at, and what to do with it
	if ( cm.tlPinned )
		CM_Text( x, headY + 2, va( "Timeline" S_COLOR_CYAN "  %.1f s" S_COLOR_GREY " of %.1f", cm.tlMs / 1000.0f, total / 1000.0f ), cmWhite );
	else
		CM_Text( x, headY + 2, va( "Timeline" S_COLOR_GREY "  %.1f s: click a moment to see it", total / 1000.0f ), cmWhite );
	if ( CM_Button( x + w - 44, headY, 44, "Let go", qfalse, cm.tlPinned ) )
		CM_TlLetGo();
	if ( CM_Button( x + w - 44 - 4 - 104 - 4 - 50, headY, 50, cm.tlPinned && CM_MarkNear( cm.tlMs, 0 ) >= 0 ? "Unmark" : "+ Mark", qfalse, cm.tlPinned ) )
		CM_ToggleMark();
	// a second more or less at the moment, and previews going round between the marks
	if ( CM_Button( x + w - 44 - 4 - 104 - 4 - 50 - 4 - 30, headY, 30, "-1 s", qfalse, cm.tlPinned ) )
		CM_ChangeTime( -1000 );
	if ( CM_Button( x + w - 44 - 4 - 104 - 4 - 50 - 4 - 30 - 4 - 30, headY, 30, "+1 s", qfalse, cm.tlPinned ) )
		CM_ChangeTime( 1000 );
	if ( CM_Button( x + w - 44 - 4 - 104 - 4 - 50 - 4 - 30 - 4 - 30 - 4 - 34, headY, 34, "Loop", cm.loop, qtrue ) )
		cm.loop = (qboolean)!cm.loop;
	if ( CM_Button( x + w - 44 - 4 - 104, headY, 104, cm.tlCam ? "Seen: the cinematic's" : "Seen: free camera", qfalse, cm.tlPinned ) ) {
		cm.tlCam = (qboolean)!cm.tlCam;
		if ( cm.tlCam )
			cm.through = qfalse;
	}

	CM_Box( x, boxY, w, boxH, cmPanel );

	// a second or so between the marks, whatever its length
	{
		static const int steps[] = { 250, 500, 1000, 2000, 5000, 10000, 30000, 60000 };
		int step = steps[ARRAY_LEN( steps ) - 1];

		for ( size_t i = 0; i < ARRAY_LEN( steps ); i++ ) {
			if ( t.w * steps[i] / t.span >= 30.0f ) {
				step = steps[i];
				break;
			}
		}
		for ( int ms = 0; ms <= t.span; ms += step ) {
			const float mx = CM_TlX( &t, ms );
			const char *label = va( step < 500 ? "%.2f" : step < 1000 ? "%.1f" : "%.0f", ms / 1000.0f );

			CM_Fill( mx, boxY + 1, 1, CM_TL_RULER_H, cmBorder );
			if ( mx + 2 + CM_TextWidth( label ) < t.x + t.w )
				CM_Text( mx + 2, boxY, label, cmDim );
		}
	}

	// the wheel over the names scrolls them, over the rest steps the moment
	if ( cm.wheel && CM_InRect( x, boxY, CM_TL_LABEL_W, boxH ) && numRows > visRows ) {
		cm.tlScroll += cm.wheel * 2;
		cm.wheel = 0;
	} else if ( cm.wheel && CM_InRect( t.x - 4, headY, t.w + 10, CM_TL_BOTTOM - headY ) ) {
		cm.tlMs = Com_Clampi( 0, t.span, cm.tlMs + cm.wheel * ( CM_ShiftDown() ? 1000 : CM_CtrlDown() ? 20 : 100 ) );
		CM_TlPin();
		cm.wheel = 0;
	}
	// the camera's row stays, the others scroll a person at a time
	cm.tlScroll = Com_Clampi( 0, Q_max( 0, numRows - visRows ), cm.tlScroll ) & ~1;

	for ( int r = 0; r < visRows; r++ ) {
		const int row = r ? cm.tlScroll + r : 0;
		const float ry = rowsY + r * CM_ROW_H;

		if ( row >= numRows )
			break;
		if ( row && ( row & 1 ) )
			CM_Fill( x + 1, ry, w - 2, 1, cmTlLine );
		if ( row == 0 ) {
			CM_TextClipped( x + 3, ry, CM_TL_LABEL_W - 6, "Camera", cm.tab == CM_TAB_CAMERA ? cmTlSel : cmWhite );
			if ( !c->numShots ) {
				CM_Text( t.x + 3, ry, S_COLOR_GREY "no shots yet", cmWhite );
				continue;
			}
			for ( int i = 0; i < c->numShots; i++ ) {
				const rcShot_t *s = &c->shots[i];
				const int from = CM_ShotStart( c, i ), len = CM_ShotLen( s );
				const float x0 = CM_TlX( &t, from ), x1 = CM_TlX( &t, from + len );

				CM_Fill( x0 + 1, ry + 1, Q_max( 1.0f, x1 - x0 - 1 ), CM_ROW_H - 2, i == cm.cur ? cmTlShotOn : cmTlShot );
				if ( s->in.type != RC_TR_CUT && s->in.ms > 0 )
					CM_Fill( x0 + 1, ry + 1, CM_TlX( &t, from + s->in.ms ) - x0, CM_ROW_H - 2, cmTlTrans );
				if ( s->out.type != RC_TR_CUT && s->out.ms > 0 )
					CM_Fill( CM_TlX( &t, from + len - s->out.ms ), ry + 1, x1 - CM_TlX( &t, from + len - s->out.ms ), CM_ROW_H - 2, cmTlTrans );
				for ( int k = 1; !s->orbit && k < s->numKeys - 1; k++ ) {
					const float mx = CM_TlX( &t, from + CM_KeyMs( s, k ) );

					CM_Fill( mx - 1, ry + 2, 3, CM_ROW_H - 4, i == cm.cur && k == cm.sel ? cmTlSel : cmWhite );
				}
				// a moving focus's keys, along the bottom
				for ( int k = 0; CM_FocusMoves( s ) && k < s->numFocus; k++ ) {
					static const vec4_t focusKey = { 0.3f, 0.75f, 1.0f, 1.0f };
					const float mx = CM_TlX( &t, from + s->focus[k].ms );

					CM_Fill( mx - 2, ry + CM_ROW_H - 6, 5, 5, i == cm.cur && k == cm.focusSel ? cmTlHead : focusKey );
				}
				// the end, where the next starts
				CM_Fill( x1 - 1, ry, 3, CM_ROW_H, cmWhite );
				if ( x1 - x0 > 30.0f )
					CM_TextClipped( x0 + 4, ry, x1 - x0 - 8, va( "%s%i%s", i == cm.cur ? "" : S_COLOR_GREY, i + 1, s->orbit ? " orbit" : s->numKeys == 1 ? " still" : "" ), cmWhite );
			}
		} else if ( row == fxRow ) {
			static const vec4_t fxBar = { 0.85f, 0.45f, 0.15f, 0.85f };

			CM_TextClipped( x + 3, ry, CM_TL_LABEL_W - 6, "Effects", cm.tab == CM_TAB_EFFECTS ? cmTlSel : cmWhite );
			for ( int i = 0; i < c->numFx; i++ ) {
				const cmFx_t *f = &c->fx[i];

				CM_TlBar( CM_TlX( &t, f->start ), CM_TlX( &t, CM_FxEnd( f ) ), ry, fxBar, COM_SkipPath( (char *)f->path ), (qboolean)( i == cm.fx && cm.tab == CM_TAB_EFFECTS ) );
			}
		} else if ( row == sndRow ) {
			static const vec4_t sndBar = { 0.25f, 0.55f, 0.85f, 0.85f };

			CM_TextClipped( x + 3, ry, CM_TL_LABEL_W - 6, "Sounds", cm.tab == CM_TAB_SOUNDS ? cmTlSel : cmWhite );
			for ( int i = 0; i < c->numSnd; i++ ) {
				const cmSnd_t *s = &c->snd[i];

				CM_TlBar( CM_TlX( &t, s->start ), CM_TlX( &t, CM_SndEnd( s ) ), ry, sndBar, COM_SkipPath( (char *)s->path ), (qboolean)( i == cm.snd && cm.tab == CM_TAB_SOUNDS ) );
			}
		} else if ( row == txtRow ) {
			static const vec4_t txtBar = { 0.6f, 0.4f, 0.8f, 0.85f };

			CM_TextClipped( x + 3, ry, CM_TL_LABEL_W - 6, "Text", cm.tab == CM_TAB_TEXT ? cmTlSel : cmWhite );
			for ( int i = 0; i < c->numTxt; i++ ) {
				const cmTxt_t *e = &c->txt[i];

				CM_TlBar( CM_TlX( &t, e->start ), CM_TlX( &t, e->start + e->ms ), ry, txtBar, e->text[0] ? e->text : "nothing yet",
					(qboolean)( i == cm.txt && cm.tab == CM_TAB_TEXT ) );
			}
		} else if ( row == propRow ) {
			static const vec4_t propBar = { 0.55f, 0.5f, 0.35f, 0.85f };

			CM_TextClipped( x + 3, ry, CM_TL_LABEL_W - 6, "Props", cm.tab == CM_TAB_WORLD && cm.worldPage == 0 ? cmTlSel : cmWhite );
			for ( int i = 0; i < c->numProps; i++ ) {
				const cmProp_t *pr = &c->props[i];

				CM_TlBar( CM_TlX( &t, pr->start ), CM_TlX( &t, CM_PropEnd( c, pr ) ), ry, propBar, pr->model[0] ? COM_SkipPath( (char *)pr->model ) : "no model yet",
					(qboolean)( i == cm.prop && cm.tab == CM_TAB_WORLD && cm.worldPage == 0 ) );
				// its keys, along the bottom
				for ( int k = 0; k < pr->numKeys; k++ )
					CM_Fill( CM_TlX( &t, pr->start + pr->keys[k].ms ) - 2, ry + CM_ROW_H - 6, 5, 5, i == cm.prop && k == cm.propKey ? cmTlHead : cmWhite );
			}
		} else if ( row == speedRow ) {
			static const vec4_t slowBar = { 0.3f, 0.45f, 0.6f, 0.85f }, fastBar = { 0.65f, 0.4f, 0.25f, 0.85f };

			CM_TextClipped( x + 3, ry, CM_TL_LABEL_W - 6, "Time", cm.tab == CM_TAB_WORLD && cm.worldPage == 4 ? cmTlSel : cmWhite );
			for ( int i = 0; i < c->numSpeeds; i++ ) {
				const cmSpeed_t *sp = &c->speeds[i];

				CM_TlBar( CM_TlX( &t, sp->start ), CM_TlX( &t, sp->start + sp->ms ), ry, sp->speed < 100 ? slowBar : fastBar, va( "%i%%", sp->speed ),
					(qboolean)( i == cm.speed && cm.tab == CM_TAB_WORLD && cm.worldPage == 4 ) );
			}
		} else if ( row == mapRow ) {
			CM_TextClipped( x + 3, ry, CM_TL_LABEL_W - 6, "Map", cm.tab == CM_TAB_WORLD && cm.worldPage == 3 ? cmTlSel : cmWhite );
			for ( int i = 0; i < c->numMapLooks; i++ ) {
				const cmMapLook_t *m = &c->mapLooks[i];
				const vec4_t bar = { 0.2f + m->lightColor[0] * 0.3f * m->light / 100.0f, 0.2f + m->lightColor[1] * 0.3f * m->light / 100.0f,
					0.2f + m->lightColor[2] * 0.3f * m->light / 100.0f, 0.85f };

				CM_TlBar( CM_TlX( &t, m->start ), CM_TlX( &t, CM_MapLookEnd( c, m ) ), ry, bar, va( "light %i%%%s", m->light, m->fogDepth > 0.0f ? ", fog" : "" ),
					(qboolean)( i == cm.mapLook && cm.tab == CM_TAB_WORLD && cm.worldPage == 3 ) );
			}
		} else if ( row == screenRow ) {
			CM_TextClipped( x + 3, ry, CM_TL_LABEL_W - 6, "Screen", cm.tab == CM_TAB_WORLD && cm.worldPage == 2 ? cmTlSel : cmWhite );
			for ( int i = 0; i < c->numScreens; i++ ) {
				const cmScreen_t *sc = &c->screens[i];
				const vec4_t bar = { 0.25f + sc->color[0] * 0.5f, 0.25f + sc->color[1] * 0.5f, 0.25f + sc->color[2] * 0.5f, 0.85f };

				CM_TlBar( CM_TlX( &t, sc->start ), CM_TlX( &t, sc->start + sc->ms ), ry, bar, cmScreenKinds[sc->kind],
					(qboolean)( i == cm.screen && cm.tab == CM_TAB_WORLD && cm.worldPage == 2 ) );
			}
		} else if ( row == lightRow ) {
			CM_TextClipped( x + 3, ry, CM_TL_LABEL_W - 6, "Lights", cm.tab == CM_TAB_WORLD && cm.worldPage == 1 ? cmTlSel : cmWhite );
			for ( int i = 0; i < c->numLights; i++ ) {
				const cmLight_t *l = &c->lights[i];
				const vec4_t bar = { l->color[0] * 0.75f, l->color[1] * 0.75f, l->color[2] * 0.75f, 0.85f };

				CM_TlBar( CM_TlX( &t, l->start ), CM_TlX( &t, CM_LightEnd( c, l ) ), ry, bar, cmLightFx[l->fx],
					(qboolean)( i == cm.light && cm.tab == CM_TAB_WORLD && cm.worldPage == 1 ) );
			}
		} else {
			const int e = ( row - 1 ) / 2, lane = ( row - 1 ) & 1;

			if ( lane == 0 )
				CM_TextClipped( x + 3, ry, CM_TL_LABEL_W - 6, c->ents[e].who[0] ? c->ents[e].who : S_COLOR_YELLOW "nobody",
					cm.tab == CM_TAB_ENTITIES && e == cm.ent ? cmTlSel : cmWhite );
			else
				CM_Text( x + 10, ry, S_COLOR_GREY "acts", cmWhite );
			for ( int i = 0; i < c->numClips; i++ ) {
				const cmClip_t *clip = &c->clips[i];
				const qboolean sel = (qboolean)( i == cm.clip );

				if ( clip->ent != e || CM_Moves( clip->kind ) != ( lane == 0 ) )
					continue;
				if ( clip->kind == CM_CLIP_MOVE )
					CM_TlBar( CM_TlX( &t, clip->start ), CM_TlX( &t, CM_ClipEnd( i, t.span ) ), ry, cmEditPlans[i].count ? cmTlMove : cmTlMissing,
						cmEditPlans[i].count ? va( "%s, %i spot%s", clip->run ? "runs" : "walks", clip->numPts, clip->numPts == 1 ? "" : "s" )
						: !clip->numPts ? "no spots yet" : "not here", sel );
				else if ( clip->kind == CM_CLIP_PLACE )
					CM_TlBar( CM_TlX( &t, clip->start ), CM_TlX( &t, CM_ClipEnd( i, t.span ) ), ry, cmEditPlans[i].count ? cmTlPlace : cmTlMissing,
						clip->tele ? "teleport" : va( "position, %i key%s", clip->numKeys, clip->numKeys == 1 ? "" : "s" ), sel );
				else if ( clip->kind == CM_CLIP_NPCCMD )
					CM_TlBar( CM_TlX( &t, clip->start ), CM_TlX( &t, CM_ClipEnd( i, t.span ) ), ry, cmTlAct, clip->cmd[0] ? clip->cmd : "command", sel );
				else if ( clip->kind == CM_CLIP_SPAWN )
					CM_TlBar( CM_TlX( &t, clip->start ), CM_TlX( &t, CM_ClipEnd( i, t.span ) ), ry, clip->npcType[0] ? cmTlAct : cmTlMissing,
						clip->npcType[0] ? va( "spawn %s", clip->npcType ) : "spawn: no type yet", sel );
				else if ( clip->kind == CM_CLIP_REMOVE )
					CM_TlBar( CM_TlX( &t, clip->start ), CM_TlX( &t, CM_ClipEnd( i, t.span ) ), ry, cmTlAct, "remove", sel );
				else if ( clip->kind == CM_CLIP_RECORD )
					CM_TlBar( CM_TlX( &t, clip->start ), CM_TlX( &t, CM_ClipEnd( i, t.span ) ), ry, CM_Rec( c, clip ) ? cmTlRec : cmTlMissing,
						CM_Rec( c, clip ) ? va( "recorded, %.1f s", CM_RecLen( c, clip ) / 1000.0f ) : "not recorded yet", sel );
				else if ( clip->kind == CM_CLIP_EMOTE )
					CM_TlBar( CM_TlX( &t, clip->start ), CM_TlX( &t, CM_ClipEnd( i, t.span ) ), ry, cmTlEmote,
						clip->emote[0] ? va( "/%s", clip->emote ) : "no emote yet", sel );
				else if ( clip->kind == CM_CLIP_LOOK )
					CM_TlBar( CM_TlX( &t, clip->start ), CM_TlX( &t, CM_ClipEnd( i, t.span ) ), ry, cmTlEmote, CM_LookSummary( clip ), sel );
				else if ( clip->kind == CM_CLIP_FORCE )
					CM_TlBar( CM_TlX( &t, clip->start ), CM_TlX( &t, CM_ClipEnd( i, t.span ) ), ry, cmTlAct,
						clip->target >= 0 && clip->target < c->numEnts ? va( "%s at %s", cmForces[clip->power].name, c->ents[clip->target].who )
						: cmForces[clip->power].name, sel );
				else if ( clip->kind == CM_CLIP_SAY )
					CM_TlBar( CM_TlX( &t, clip->start ), CM_TlX( &t, CM_ClipEnd( i, t.span ) ), ry, cmTlSay,
						clip->text[0] ? va( "\"%s\"", clip->text ) : "says nothing yet", sel );
				else
					CM_TlBar( CM_TlX( &t, clip->start ), CM_TlX( &t, CM_ClipEnd( i, t.span ) ), ry, cmTlAnim, clip->a.anim, sel );
			}
		}
	}
	if ( !c->numEnts && !c->numFx && !c->numSnd && !c->numTxt && !c->numProps && !c->numLights && !c->numScreens && !c->numMapLooks )
		CM_Text( t.x + 3, rowsY + CM_ROW_H, S_COLOR_GREY "Who's in it: the Entities tab. Effects, sounds, text, props and lights: their tabs", cmWhite );
	else if ( numRows > visRows )
		CM_Text( x + 3, boxY, va( S_COLOR_GREY "%i of %i", cm.tlScroll / 2 + 1, c->numEnts ), cmWhite );

	CM_MultiOutline( &t, rowsY, visRows );

	// the marked moments, a line down the rows and their number on the ruler
	for ( int i = 0; i < c->numMarks; i++ ) {
		static const vec4_t markColor = { 1.0f, 0.8f, 0.2f, 0.7f };
		const float mx = CM_TlX( &t, c->marks[i] );

		CM_Fill( mx, boxY + 1, 1, boxH - 2, markColor );
		CM_Fill( mx - 2, boxY + 1, 5, 4, markColor );
		CM_Text( mx + 3, boxY, va( S_COLOR_YELLOW "%i", i + 1 ), cmWhite );
	}

	// the moment, or the green camera's in the view running along
	if ( cm.tlPinned ) {
		const float mx = CM_TlX( &t, cm.tlMs );

		CM_Fill( mx, boxY + 1, 1, boxH - 2, cmTlHead );
		CM_Fill( mx - 2, boxY + 1, 5, 3, cmTlHead );
	} else if ( total > 0 ) {
		CM_Fill( CM_TlX( &t, Q_min( cls.realtime % ( total + 1000 ), total ) ), rowsY, 1, boxH - CM_TL_RULER_H - 3, cmTlGhost );
	}

	if ( cm.click && cm.clickX >= x && cm.clickX < x + w && cm.clickY >= boxY && cm.clickY < boxY + boxH ) {
		cm.click = qfalse;
		CM_TlClick( &t, rowsY, numRows );
	}
}

/*
===============================================================================

The manager's screen

===============================================================================
*/

static qboolean CM_Project( const vec3_t point, float *x, float *y ) {
	vec3_t d;
	float forward;

	if ( !cm.haveView )
		return qfalse;
	VectorSubtract( point, cm.viewOrg, d );
	forward = DotProduct( d, cm.viewAxis[0] );
	if ( forward < 4.0f )
		return qfalse;
	*x = SCREEN_WIDTH * 0.5f * ( 1.0f - DotProduct( d, cm.viewAxis[1] ) / forward / tanf( DEG2RAD( cm.viewFovX * 0.5f ) ) );
	*y = SCREEN_HEIGHT * 0.5f * ( 1.0f - DotProduct( d, cm.viewAxis[2] ) / forward / tanf( DEG2RAD( cm.viewFovY * 0.5f ) ) );
	return (qboolean)( *x >= 0 && *x <= SCREEN_WIDTH && *y >= 0 && *y <= SCREEN_HEIGHT );
}

// the frame every screen shares, and where the bars will cover
static void CM_DrawGuides( void ) {
	const float aspect = cls.glconfig.vidHeight ? (float)cls.glconfig.vidWidth / cls.glconfig.vidHeight : 4.0f / 3.0f;
	const float cx = SCREEN_WIDTH * 0.5f, cy = SCREEN_HEIGHT * 0.5f;

	if ( cm.cine.bars ) {
		const float h = CM_BarFrac( &cm.cine ) * SCREEN_HEIGHT;

		CM_Fill( 0, 0, SCREEN_WIDTH, h, cmBarGuide );
		CM_Fill( 0, SCREEN_HEIGHT - h, SCREEN_WIDTH, h, cmBarGuide );
	}
	// a wide screen sees more at the sides than a 4:3 one
	if ( aspect > 4.0f / 3.0f + 0.01f ) {
		const float w = SCREEN_WIDTH * ( 4.0f / 3.0f ) / aspect;

		CM_Fill( cx - w * 0.5f, 0, 1, SCREEN_HEIGHT, cmGuide );
		CM_Fill( cx + w * 0.5f, 0, 1, SCREEN_HEIGHT, cmGuide );
	}
	CM_Fill( cx - 6, cy, 13, 1, cmWhite );
	CM_Fill( cx, cy - 6, 1, 13, cmWhite );
}

static void CM_DrawFileList( float x, float y, float w ) {
	const int rows = 24;

	CM_Text( x, y, va( "Saved (%i)", cm.numFiles ), cmWhite );
	y += CM_ROW_H + 2;
	if ( cm.wheel && CM_InRect( x, y, w, rows * CM_ROW_H ) ) {
		cm.fileScroll += cm.wheel;
		cm.wheel = 0;
	}
	cm.fileScroll = Com_Clampi( 0, Q_max( 0, cm.numFiles - rows ), cm.fileScroll );
	CM_Box( x, y, w, rows * CM_ROW_H + 2, cmPanelLight );
	const float lw = CM_ListBar( x, y, w, rows * CM_ROW_H + 2, cm.numFiles, rows, &cm.fileScroll );
	for ( int i = 0; i < rows && cm.fileScroll + i < cm.numFiles; i++ ) {
		const int f = cm.fileScroll + i;
		const float ry = y + 1 + i * CM_ROW_H;

		if ( CM_InRect( x, ry, lw, CM_ROW_H ) )
			CM_Fill( x + 1, ry, lw - 2, CM_ROW_H, cmHoverBox );
		if ( !cm.fileOld[f] && !Q_stricmp( cm.files[f], cm.name ) )
			CM_Fill( x + 1, ry, lw - 2, CM_ROW_H, cmHighlight );
		CM_TextClipped( x + 4, ry, lw - 8, cm.fileOld[f] ? va( "%s" S_COLOR_GREY "  old shot", cm.files[f] ) : cm.files[f], cmWhite );
		if ( CM_Clicked( x, ry, lw, CM_ROW_H ) && CM_Open( cm.files[f], cm.fileOld[f] ) )
			cm.listing = qfalse;
	}
	if ( !cm.numFiles )
		CM_Text( x + 4, y + 2, S_COLOR_GREY "none yet", cmWhite );
	y += rows * CM_ROW_H + 8;
	if ( CM_Button( x, y, w, "Back", qfalse, qtrue ) )
		cm.listing = qfalse;
}

// the actions to add, over the panel: it takes the click it was given
static void CM_DrawAddPopup( void ) {
	const float *r = cm.addPop;
	const float third = ( r[2] - 6 - 8 ) / 3.0f;

	cm.click = cm.popClick;
	cm.popHover = qfalse;
	CM_Fill( r[0] + 2, r[1] + 2, r[2], r[3], cmPanel );
	CM_Box( r[0], r[1], r[2], r[3], cmPanelLight );
	CM_Fill( r[0] + 1, r[1] + 1, r[2] - 2, r[3] - 2, cmPanel );
	for ( int k = 0; k < (int)ARRAY_LEN( cmAdds ); k++ ) {
		if ( CM_Button( r[0] + 3 + ( k % 3 ) * ( third + 4 ), r[1] + 3 + ( k / 3 ) * ( CM_CTRL_H + 1 ), third, cmAdds[k].label, qfalse, qtrue ) ) {
			cm.addOpen = qfalse;
			CM_AddClip( cmAdds[k].kind, cmAdds[k].tele );
			break;
		}
	}
	cm.click = qfalse;
}

static void CM_DrawEditor( void ) {
	const float x = CM_PANEL_X + 6, w = CM_PANEL_W - 12;
	const float half = ( w - 4 ) * 0.5f, third = ( w - 8 ) / 3.0f;
	const int total = CM_Total( &cm.cine );
	const rcShot_t *s = CM_Shot();
	cmClip_t *clip = CM_Clip();
	float y = 8;

	if ( cm.preview ) {
		// nothing of the manager's in a video
		if ( !cmVideo )
			CM_Text( 8, SCREEN_HEIGHT - 16, S_COLOR_GREY "Preview: Esc, P or a click stops it", cmWhite );
		return;
	}

	// the actions' popup, placed last frame: a click in it is its own, and nothing under it lights
	// up; a click anywhere else but its button closes it
	cm.popClick = qfalse;
	cm.popHover = qfalse;
	if ( cm.addOpen && cm.addPlaced ) {
		const float *r = cm.addPop, *b = cm.addBtn;

		cm.popHover = (qboolean)( !cm.flying && cm.cursorX >= r[0] && cm.cursorX < r[0] + r[2] && cm.cursorY >= r[1] && cm.cursorY < r[1] + r[3] );
		if ( cm.popHover )
			cm.wheel = 0;
		if ( cm.click && cm.clickX >= r[0] && cm.clickX < r[0] + r[2] && cm.clickY >= r[1] && cm.clickY < r[1] + r[3] ) {
			cm.popClick = qtrue;
			cm.click = qfalse;
		} else if ( cm.click && !( cm.clickX >= b[0] && cm.clickX < b[0] + b[2] && cm.clickY >= b[1] && cm.clickY < b[1] + b[3] ) ) {
			cm.addOpen = qfalse;
		}
	}
	cm.addPlaced = qfalse;

	// through the cinematic's camera at the timeline's moment: its fade or wipe then
	if ( CM_TlLooking() ) {
		int local;
		const int shot = CM_ShotAt( &cm.cine, cm.tlMs, &local );

		RC_DrawTransitions( &cm.cine.shots[shot], local );
	}
	// the texts then, seen through its camera or written on the Text tab
	if ( cm.tlPinned && ( CM_TlLooking() || cm.tab == CM_TAB_TEXT || cm.tab == CM_TAB_WORLD ) ) {
		CM_DrawScreens( &cm.cine, cm.tlMs );
		CM_DrawTexts( &cm.cine, cm.tlMs );
	}
	// a picture being taken: the bars as they'll be, and nothing else
	if ( cm.pictureFrames > 0 ) {
		if ( cm.cine.bars ) {
			static const vec4_t black = { 0, 0, 0, 1 };
			const float h = CM_BarFrac( &cm.cine ) * SCREEN_HEIGHT;

			CM_Fill( 0, 0, SCREEN_WIDTH, h, black );
			CM_Fill( 0, SCREEN_HEIGHT - h, SCREEN_WIDTH, h, black );
			// the texts over the bars
			CM_DrawTexts( &cm.cine, cm.tlMs );
		}
		return;
	}
	CM_DrawGuides();
	CM_DrawFocusCircle();
	CM_GizmoFrame();

	// numbers over the effects
	if ( cm.tab == CM_TAB_EFFECTS && !cm.listing ) {
		for ( int i = 0; i < cm.cine.numFx; i++ ) {
			float sx, sy;

			if ( !CM_Project( cm.cine.fx[i].org, &sx, &sy ) )
				continue;
			CM_Fill( sx - 2, sy - 2, 5, 5, i == cm.fx ? cmTlSel : cmWhite );
			CM_Text( sx + 4, sy - 14, va( "%s%.1fs %s", i == cm.fx ? S_COLOR_CYAN : "", cm.cine.fx[i].start / 1000.0f, COM_SkipPath( cm.cine.fx[i].path ) ), cmWhite );
		}
	}
	if ( cm.tab == CM_TAB_SOUNDS && !cm.listing ) {
		for ( int i = 0; i < cm.cine.numSnd; i++ ) {
			float sx, sy;

			if ( !cm.cine.snd[i].spot || !CM_Project( cm.cine.snd[i].org, &sx, &sy ) )
				continue;
			CM_Fill( sx - 2, sy - 2, 5, 5, i == cm.snd ? cmTlSel : cmWhite );
			CM_Text( sx + 4, sy - 14, va( "%s%.1fs %s", i == cm.snd ? S_COLOR_CYAN : "", cm.cine.snd[i].start / 1000.0f, COM_SkipPath( cm.cine.snd[i].path ) ), cmWhite );
		}
	}

	// numbers over the spots: the shot's, or the move's
	if ( !CM_TlLooking() ) {
		if ( cm.tab == CM_TAB_CAMERA && s && !s->orbit ) {
			for ( int i = 0; i < s->numKeys; i++ ) {
				float sx, sy;

				if ( ( cm.through && i == cm.sel ) || !CM_Project( s->keys[i].org, &sx, &sy ) )
					continue;
				CM_Text( sx + 4, sy - 14, va( "%s%i", i == cm.sel ? S_COLOR_CYAN : "", i + 1 ), cmWhite );
			}
		} else if ( cm.tab == CM_TAB_ENTITIES && clip && ( clip->kind == CM_CLIP_MOVE || clip->kind == CM_CLIP_PLACE ) ) {
			for ( int i = 0; i < ( clip->kind == CM_CLIP_MOVE ? clip->numPts : clip->numKeys ); i++ ) {
				float sx, sy;

				if ( CM_Project( clip->kind == CM_CLIP_MOVE ? clip->pts[i] : clip->keys[i].org, &sx, &sy ) )
					CM_Text( sx + 4, sy - 14, va( "%s%i", i == cm.spot ? S_COLOR_YELLOW : S_COLOR_CYAN, i + 1 ), cmWhite );
			}
		}
	}

	// top left, under the console's lines: what this is, how it went
	if ( cm.through )
		CM_Text( 8, CM_INFO_Y, va( "Cinematic manager" S_COLOR_CYAN "   looking through spot %i: flying moves it", cm.sel + 1 ), cmWhite );
	else if ( CM_TlLooking() )
		CM_Text( 8, CM_INFO_Y, va( "Cinematic manager" S_COLOR_CYAN "   at %.1f s, as it's seen: right mouse flies from here", cm.tlMs / 1000.0f ), cmWhite );
	else
		CM_Text( 8, CM_INFO_Y, "Cinematic manager", cmWhite );
	CM_Text( 8, CM_INFO_Y + CM_ROW_H, va( S_COLOR_GREY "FOV %i%s", RC_Round( cm.camFov ),
		RC_Round( AngleNormalize180( cm.camAng[ROLL] ) ) ? va( "   roll %i", RC_Round( AngleNormalize180( cm.camAng[ROLL] ) ) ) : "" ), cmWhite );
	if ( cm.message[0] && cls.realtime - cm.messageTime < CM_MESSAGE_MS )
		CM_TextClipped( 8, CM_INFO_Y + CM_ROW_H * 2, CM_PANEL_X - 16, cm.message, cmWhite );

	// bottom left: the keys
	{
		const char *help[CM_HELP_LINES] = {
			"Hold right mouse: look and fly   WASD  Space/C up/down  Shift fast  Ctrl slow",
			"While flying: wheel zooms   Q/E roll   R levels",
			cm.tab == CM_TAB_ENTITIES ? "Click someone to pick them   Enter adds a move's spot or a position's key, a click picks one, drag moves it   Del takes it off"
				: cm.tab == CM_TAB_EFFECTS ? "Click an effect to pick it, drag it over walls and floors   J move arrows   Del deletes it   P preview"
				: cm.tab == CM_TAB_SOUNDS ? "Click a sound's spot to pick it, drag it over walls and floors   J move arrows   Del deletes it   P preview"
				: cm.tab == CM_TAB_TEXT ? "Pick a moment on the timeline to see the texts then   Del deletes the one picked   P preview"
				: cm.tab == CM_TAB_WORLD ? "Click a prop's key or a light to pick it, drag it over walls and floors   J move arrows   Del deletes it   P preview"
				: "Enter add a spot   Del delete   Up/Down pick   F look through it   P preview",
			cm.tlHidden ? "T shows the timeline" : "Timeline: click or drag a moment, drag bars to retime   P plays from it   T hides it",
			cm.tlPinned ? "Ctrl+Z undo   Ctrl+Y redo   Ctrl+C copy what's picked   Ctrl+V paste it at the moment   Esc lets go of the moment"
				: "Ctrl+Z undo   Ctrl+Y redo   Ctrl+C copy what's picked   Ctrl+V paste it   Esc back"
		};

		for ( int i = 0; i < CM_HELP_LINES; i++ )
			CM_Text( 8, SCREEN_HEIGHT - 12 - ( CM_HELP_LINES - 1 - i ) * CM_ROW_H, va( S_COLOR_GREY "%s", help[i] ), cmWhite );
	}

	CM_DrawTimeline();

	CM_Fill( CM_PANEL_X, 0, CM_PANEL_W, SCREEN_HEIGHT, cmPanel );

	// the name and the files
	CM_Text( x, y, "Name", cmWhite );
	CM_TextBox( x + 36, y - 2, w - 36, cm.name, "click to name it", CM_TYPE_NAME, NULL );
	if ( cm.dirty && cm.typing != CM_TYPE_NAME )
		CM_Text( x + w - 4 - CM_TextWidth( "*" ), y, S_COLOR_YELLOW "*", cmWhite );
	y += CM_CTRL_H + CM_GAP;
	if ( CM_Button( x, y, third, "Save", qfalse, qtrue ) )
		CM_SaveCurrent();
	if ( CM_Button( x + third + 4, y, third, "Open", cm.listing, qtrue ) ) {
		if ( cm.listing )
			cm.listing = qfalse;
		else
			CM_ListFiles();
	}
	if ( CM_Button( x + ( third + 4 ) * 2, y, third, "New", qfalse, qtrue ) ) {
		CM_New();
		CM_Message( "New cinematic: fly to where it starts and press Enter" );
	}
	y += CM_CTRL_H + CM_GAP;

	if ( cm.listing ) {
		CM_DrawFileList( x, y + 6, w );
	} else {
		static const char *tabs[] = { "Camera", "Entities", "Effects", "Sounds", "Text", "World" };

		// how it looks to everyone
		// off, thin, as they were, wide
		if ( CM_Switch( x, y, half, "Bars", !cm.cine.bars ? "off" : cm.cine.barSize == 0 ? "thin" : cm.cine.barSize == 2 ? "wide" : "on" ) ) {
			if ( !cm.cine.bars ) {
				cm.cine.bars = qtrue;
				cm.cine.barSize = 0;
			} else if ( cm.cine.barSize < 2 ) {
				cm.cine.barSize++;
			} else {
				cm.cine.bars = qfalse;
				cm.cine.barSize = 1;
			}
			CM_Changed();
		}
		if ( CM_Switch( x + half + 4, y, half, "HUD", cm.cine.hud ? "shown" : "hidden" ) ) {
			cm.cine.hud = (qboolean)!cm.cine.hud;
			CM_Changed();
		}
		y += CM_CTRL_H + CM_GAP;
		if ( CM_Switch( x, y, w, "Seen", cm.cine.onlyCast ? "only those in it" : "everyone" ) ) {
			cm.cine.onlyCast = (qboolean)!cm.cine.onlyCast;
			CM_Changed();
		}
		y += CM_CTRL_H + CM_GAP + 6;
		{
			const int tab = CM_Tabs( x, y, w, tabs, ARRAY_LEN( tabs ), cm.tab );

			if ( tab != cm.tab ) {
				cm.tab = tab;
				cm.typing = CM_TYPE_NONE;
				cm.through = qfalse;
			}
		}
		y += CM_CTRL_H + 8;
		if ( cm.tab == CM_TAB_ENTITIES )
			CM_DrawEntitiesTab( x, y, w );
		else if ( cm.tab == CM_TAB_EFFECTS )
			CM_DrawEffectsTab( x, y, w );
		else if ( cm.tab == CM_TAB_SOUNDS )
			CM_DrawSoundsTab( x, y, w );
		else if ( cm.tab == CM_TAB_TEXT )
			CM_DrawTextTab( x, y, w );
		else if ( cm.tab == CM_TAB_WORLD )
			CM_DrawWorldTab( x, y, w );
		else
			CM_DrawCameraTab( x, y, w );

		// try it
		y = CM_BOTTOM_Y;
		CM_Fill( x, y - 4, w, 1, cmBorder );
		CM_Text( x, y, CL_RpHooks_Experimental() ? va( S_COLOR_GREY "%.1f s, %i shot%s, %i in it", total / 1000.0f, cm.cine.numShots, cm.cine.numShots == 1 ? "" : "s", cm.cine.numEnts )
			: S_COLOR_GREY "For everyone: " RP_EXPERIMENTAL_LABEL, cmWhite );
		y += CM_ROW_H;
		if ( CM_Button( x, y, third, cm.loop ? "Preview, loop" : cm.tlPinned && cm.tlMs < total ? va( "Preview %.1f s", cm.tlMs / 1000.0f ) : "Preview (P)", qfalse,
			(qboolean)( total > 0 ) ) )
			CM_Preview();
		if ( CM_Button( x + third + 4, y, third, va( "Video %i fps", cl_aviFrameRate->integer ), qfalse, (qboolean)( total > 0 && !CL_VideoRecording() ) ) )
			CM_RecordVideo();
		if ( CM_Button( x + ( third + 4 ) * 2, y, third, "Picture", qfalse, (qboolean)( total > 0 ) ) )
			CM_Picture();
		y += CM_CTRL_H + CM_GAP;
		if ( CM_PartsLeft() ) {
			CM_Text( x, y + 2, va( S_COLOR_GREEN "Sending it, %i part%s to go", CM_PartsLeft(), CM_PartsLeft() == 1 ? "" : "s" ), cmWhite );
		} else {
			if ( CM_Button( x, y, half, "Play for everyone", qfalse, (qboolean)( total > 0 && CL_RpHooks_Experimental() ) ) && CM_Send( &cm.cine ) )
				CM_Message( S_COLOR_GREEN "Sending it to everyone: it plays once it's all there" );
			if ( CM_Button( x + half + 4, y, half, "End for everyone", qfalse, CL_RpHooks_Experimental() ) ) {
				CM_SendStop();
				CM_Message( S_COLOR_GREEN "Ending it for everyone" );
			}
		}
	}

	// over everything else
	if ( cm.addOpen && cm.addPlaced )
		CM_DrawAddPopup();
	else
		cm.addOpen = qfalse;
	cm.popHover = qfalse;

	// a click in the view picks a spot or someone; on nothing it leaves the box typed in
	if ( cm.click && cm.clickX < CM_PANEL_X && !cm.listing )
		CM_ViewClick();
	CM_SpotDrag();
	CM_FocusDrag();
	CM_FxDrag();
	CM_SndDrag();
	CM_WorldDrag();
	if ( cm.click )
		cm.typing = CM_TYPE_NONE;
	cm.click = qfalse;
	cm.wheel = 0;

	WG_DrawOverlay();
	if ( !cm.flying )
		re->DrawStretchPic( cm.cursorX, cm.cursorY, 32, 32, 0, 0, 1, 1, cls.cursorShader );
}

/*
===============================================================================

Recording: you act it out as yourself, and what you do is kept, a moment a server frame, for
someone in it to do from where they are then

===============================================================================
*/

static cvar_t *cl_cinRecordKey;

static struct {
	int			phase;				// 0 not, 1 counting down, 2 recording
	int			clip;				// the recording action it fills
	int			start;				// cls.realtime the countdown or the recording began
	int			snapStart;			// cl.snap.serverTime the recording began
	int			lastSnap;
	vec3_t		base;				// where you were then
	char		who[MAX_QPATH];
	cmRec_t		rec;

	// who you're acting as, shown where they are: the view goes there, they move as you do
	int			clipStart;			// in the cinematic
	int			target;				// their number, -1 nobody to show
	vec3_t		targetStart;		// where they are when it begins
	float		targetYaw;			// and the way they face
	vec3_t		pivot;				// where you are, turned and moved to them
	float		yawDelta;			// you turned to face their way
	vec3_t		offset;				// from you to them
	qboolean	away;				// far enough for your own body to be left out

	// what the server has you heard doing, taken from where it's got to
	int			eventSeq, externalEvent;
} cmRecording;

// a cinematic's player, or a recording, playing c on: its events heard, its force shown
static qboolean CM_Playing( const cmCine_t *c ) {
	return (qboolean)( ( cmp.playing && c == &cmp.cine ) || ( cmRecording.phase == 2 && c != &cmp.cine && c != &cm.cine ) );
}

// what the server's had you heard doing since last time
static void CM_RecordEvents( const playerState_t *ps, int ms ) {
	static const int kept[] = { EV_FALL, EV_JUMP, EV_ROLL, EV_WATER_TOUCH, EV_WATER_LEAVE, EV_WATER_UNDER, EV_WATER_CLEAR, EV_SABER_ATTACK,
		EV_SABER_UNHOLSTER, EV_SWIM, EV_FOOTWADE };
	int evs[MAX_PS_EVENTS + 1], parms[MAX_PS_EVENTS + 1], n = 0;

	for ( int i = Q_max( cmRecording.eventSeq, ps->eventSequence - MAX_PS_EVENTS ); i < ps->eventSequence; i++ ) {
		evs[n] = ps->events[i & ( MAX_PS_EVENTS - 1 )] & ~EV_EVENT_BITS;
		parms[n++] = ps->eventParms[i & ( MAX_PS_EVENTS - 1 )];
	}
	cmRecording.eventSeq = ps->eventSequence;
	if ( ps->externalEvent && ps->externalEvent != cmRecording.externalEvent ) {
		evs[n] = ps->externalEvent & ~EV_EVENT_BITS;
		parms[n++] = ps->externalEventParm;
	}
	cmRecording.externalEvent = ps->externalEvent;
	for ( int i = 0; i < n; i++ ) {
		size_t k = 0;

		while ( k < ARRAY_LEN( kept ) && kept[k] != evs[i] )
			k++;
		if ( k == ARRAY_LEN( kept ) )
			continue;
		if ( cmRecording.rec.numEvents < CM_MAX_REC_EVENTS ) {
			cmRecEvent_t *e = &cmRecording.rec.ev[cmRecording.rec.numEvents++];

			e->ms = ms;
			e->event = evs[i];
			e->parm = parms[i];
		}
	}
}

// a point of yours where it is around them: turned their way about pivot, moved to them
static void CM_ToTarget( const vec3_t in, vec3_t out ) {
	const float r = DEG2RAD( cmRecording.yawDelta ), c = cosf( r ), sn = sinf( r );
	vec3_t d;

	VectorSubtract( in, cmRecording.pivot, d );
	out[0] = cmRecording.targetStart[0] + d[0] * c - d[1] * sn;
	out[1] = cmRecording.targetStart[1] + d[0] * sn + d[1] * c;
	out[2] = cmRecording.targetStart[2] + d[2];
}

// a point around them back where it is around you, where the game hears from
static void CM_FromTarget( const vec3_t in, vec3_t out ) {
	const float r = DEG2RAD( -cmRecording.yawDelta ), c = cosf( r ), sn = sinf( r );
	vec3_t d;

	VectorSubtract( in, cmRecording.targetStart, d );
	out[0] = cmRecording.pivot[0] + d[0] * c - d[1] * sn;
	out[1] = cmRecording.pivot[1] + d[0] * sn + d[1] * c;
	out[2] = cmRecording.pivot[2] + d[2];
}

// a sound where it's seen
static void CM_SoundAt( const vec3_t pos, const char *name ) {
	vec3_t at;

	if ( cmRecording.phase && cmRecording.target >= 0 )
		CM_FromTarget( pos, at );
	else
		VectorCopy( pos, at );
	S_StartSound( at, ENTITYNUM_WORLD, CHAN_AUTO, S_RegisterSound( name ) );
}

// a direction of yours turned their way
static void CM_TurnToTarget( const vec3_t in, vec3_t out ) {
	const float r = DEG2RAD( cmRecording.yawDelta ), c = cosf( r ), sn = sinf( r );
	const float x = in[0] * c - in[1] * sn, y = in[0] * sn + in[1] * c;

	out[0] = x;
	out[1] = y;
	out[2] = in[2];
}

// the rest of the cinematic, played along while you record: without the recording, and without
// what the one recorded, or you, would otherwise do
static cmCine_t cmRecCine;
static cmPlan_t cmRecPlans[CM_MAX_CLIPS];

static void CM_RecordCine( int clip ) {
	const int target = cmRecording.target;
	cmCine_t *c = &cmRecCine;

	*c = cm.cine;
	for ( int i = c->numClips - 1; i >= 0; i-- ) {
		const cmClip_t *cl = &c->clips[i];
		int nums[CM_MAX_GROUP];
		const int n = CM_Resolve( c->ents[cl->ent].who, nums, CM_MAX_GROUP );
		qboolean drop = (qboolean)( i == clip );

		for ( int m = 0; m < n && !drop; m++ ) {
			// you act; who you act as keeps only where they've got to before it
			if ( nums[m] == clc.clientNum )
				drop = qtrue;
			else if ( nums[m] == target )
				drop = (qboolean)( !CM_Moves( cl->kind ) || cl->start >= cm.cine.clips[clip].start );
		}
		if ( drop ) {
			c->numClips--;
			memmove( &c->clips[i], &c->clips[i + 1], ( c->numClips - i ) * sizeof( c->clips[0] ) );
		}
	}
	CM_Plan( c, cmRecPlans );
}

// the cinematic's view while you record someone else: from where they are
qboolean CM_RecordView( const refdef_t *fd ) {
	refdef_t view;

	if ( !cmRecording.phase || cmRecording.target < 0 || ( fd->rdflags & RDF_SKYBOXPORTAL ) )
		return qfalse;
	view = *fd;
	CM_ToTarget( fd->vieworg, view.vieworg );
	for ( int i = 0; i < 3; i++ )
		CM_TurnToTarget( fd->viewaxis[i], view.viewaxis[i] );
	view.viewangles[YAW] += cmRecording.yawDelta;
	view.viewContents = CM_PointContents( view.vieworg, 0 );
	memset( view.areamask, 0, sizeof( view.areamask ) );
	re->RenderScene( &view );
	return qtrue;
}

qboolean CM_RecordCamera( vec3_t origin, vec3_t angles ) {
	float fovY;

	vec3_t org;

	if ( !cmRecording.phase || cmRecording.target < 0 || !RC_PlayerView( org, angles, &fovY ) )
		return qfalse;
	CM_ToTarget( org, origin );
	angles[YAW] += cmRecording.yawDelta;
	return qtrue;
}

// your own body, and what's on it, out of the view while you act as someone away from you
qboolean CM_RecordHides( const vec3_t origin ) {
	return (qboolean)( cmRecording.phase && cmRecording.target >= 0 && cmRecording.away && Distance( origin, cl.snap.ps.origin ) < 72.0f
		&& Distance( origin, cmRecording.targetStart ) > 72.0f );
}

static int CM_RecordStopKey( void ) {
	const int key = cl_cinRecordKey ? Key_StringToKeynum( cl_cinRecordKey->string ) : -1;

	return key > 0 ? key : A_F10;
}

static void CM_StartRecording( int clip ) {
	if ( cls.state != CA_ACTIVE || clip < 0 || clip >= cm.cine.numClips || cm.cine.clips[clip].kind != CM_CLIP_RECORD )
		return;
	memset( &cmRecording, 0, sizeof( cmRecording ) );
	cmRecording.phase = 1;
	cmRecording.clip = clip;
	cmRecording.start = cls.realtime;
	cmRecording.clipStart = cm.cine.clips[clip].start;
	Q_strncpyz( cmRecording.who, cm.cine.ents[cm.cine.clips[clip].ent].who, sizeof( cmRecording.who ) );
	{
		int nums[CM_MAX_GROUP];

		cmRecording.target = CM_Resolve( cmRecording.who, nums, CM_MAX_GROUP ) && nums[0] != clc.clientNum ? nums[0] : -1;
	}
	CM_RecordCine( clip );
	// where who you act as is when it begins
	if ( cmRecording.target >= 0 ) {
		CM_Apply( &cmRecCine, cmRecPlans, cmRecording.clipStart );
		if ( !CL_NpcWalk_PoseOf( cmRecording.target, cmRecording.targetStart ) && !CM_EntOrigin( cmRecording.target, cmRecording.targetStart ) )
			cmRecording.target = -1;
		// the way they face then
		if ( !CL_NpcWalk_PoseYaw( cmRecording.target, &cmRecording.targetYaw ) ) {
			cmRecording.targetYaw = cl.snap.ps.viewangles[YAW];
			for ( int i = 0; i < cl.snap.numEntities; i++ ) {
				const entityState_t *es = &cl.parseEntities[( cl.snap.parseEntitiesNum + i ) & ( MAX_PARSE_ENTITIES - 1 )];

				if ( es->number == cmRecording.target )
					cmRecording.targetYaw = es->apos.trBase[YAW];
			}
		}
	}
	// you play
	CL_RpCinematic_EditorClose();
}

// kept in the cinematic, or let go; the manager back either way
static void CM_StopRecording( qboolean keep ) {
	cmClip_t *clip;

	if ( !cmRecording.phase )
		return;
	cmRecording.phase = 0;
	CM_Unapply();
	clip = cmRecording.clip < cm.cine.numClips ? &cm.cine.clips[cmRecording.clip] : NULL;
	if ( keep && clip && clip->kind == CM_CLIP_RECORD && cmRecording.rec.count >= 2 ) {
		int slot = clip->rec >= 0 && clip->rec < CM_MAX_RECS ? clip->rec : -1;

		// a recording no one else has
		for ( int r = 0; r < CM_MAX_RECS && slot < 0; r++ ) {
			qboolean used = qfalse;

			for ( int i = 0; i < cm.cine.numClips; i++ )
				used = (qboolean)( used || ( cm.cine.clips[i].kind == CM_CLIP_RECORD && cm.cine.clips[i].rec == r && &cm.cine.clips[i] != clip ) );
			if ( !used )
				slot = r;
		}
		if ( slot < 0 ) {
			CM_Message( S_COLOR_YELLOW "6 recordings at most in a cinematic" );
		} else {
			cm.cine.recs[slot] = cmRecording.rec;
			clip->rec = slot;
			CM_Changed();
			CM_Message( va( S_COLOR_GREEN "Recorded %.1f s for %s", cmRecording.rec.s[cmRecording.rec.count - 1].ms / 1000.0f, cmRecording.who ) );
		}
	} else {
		CM_Message( S_COLOR_YELLOW "Nothing recorded" );
	}
	CL_RpCinematic_Edit( NULL, cm.fromStoryteller );
	cm.tab = CM_TAB_ENTITIES;
	if ( clip ) {
		cm.clip = cmRecording.clip;
		cm.ent = clip->ent;
	}
}

// every frame: the countdown, then a moment kept each server frame, as often as you change and
// every CM_REC_EVERY besides
static void CM_RecordFrame( void ) {
	const playerState_t *ps = &cl.snap.ps;
	cmSample_t sm, *last;

	if ( !cmRecording.phase )
		return;
	if ( cls.state != CA_ACTIVE ) {
		cmRecording.phase = 0;
		return;
	}
	// you're them from where you stand and the way you face as it begins: while it counts down
	// that's wherever you are, then it stays
	if ( cmRecording.phase == 1 ) {
		VectorCopy( ps->origin, cmRecording.pivot );
		cmRecording.yawDelta = cmRecording.target >= 0 ? AngleNormalize180( cmRecording.targetYaw - ps->viewangles[YAW] ) : 0.0f;
	}
	if ( cmRecording.phase == 1 && cls.realtime - cmRecording.start >= CM_REC_COUNTDOWN ) {
		cmRecording.phase = 2;
		cmRecording.start = cls.realtime;
		cmRecording.snapStart = cl.snap.serverTime;
		cmRecording.lastSnap = -1;
		VectorCopy( ps->origin, cmRecording.base );
		VectorCopy( ps->origin, cmRecording.pivot );
		cmRecording.eventSeq = ps->eventSequence;
		cmRecording.externalEvent = ps->externalEvent;
	}

	// the others as they'd be, held where it starts while it counts down; who you act as where
	// they are, doing what you do
	CM_Apply( &cmRecCine, cmRecPlans, cmRecording.clipStart + ( cmRecording.phase == 2 ? cls.realtime - cmRecording.start : 0 ) );
	CM_HoldSelf( qfalse );
	if ( cmRecording.target >= 0 ) {
		npcPoseState_t st;
		vec3_t pos, ang;

		VectorSubtract( cmRecording.targetStart, cmRecording.pivot, cmRecording.offset );
		cmRecording.away = (qboolean)( VectorLength( cmRecording.offset ) > 96.0f || fabsf( cmRecording.yawDelta ) > 20.0f );
		CM_ToTarget( ps->origin, pos );
		VectorSet( ang, ps->viewangles[PITCH], ps->viewangles[YAW] + cmRecording.yawDelta, 0 );
		memset( &st, 0, sizeof( st ) );
		st.legsAnim = ps->legsAnim;
		st.torsoAnim = ps->torsoAnim;
		st.legsFlip = ps->legsFlip;
		st.torsoFlip = ps->torsoFlip;
		st.forcePowersActive = ps->fd.forcePowersActive;
		st.saberHolstered = ps->saberHolstered;
		st.saberMove = ps->saberMove;
		CL_NpcWalk_PoseEx( cmRecording.target, pos, ang, 0, qtrue, &st, qfalse );
	}
	if ( cmRecording.phase == 1 )
		return;
	if ( cl.snap.serverTime == cmRecording.lastSnap )
		return;
	cmRecording.lastSnap = cl.snap.serverTime;
	memset( &sm, 0, sizeof( sm ) );
	sm.ms = cl.snap.serverTime - cmRecording.snapStart;
	{
		vec3_t d;

		VectorSubtract( ps->origin, cmRecording.base, d );
		CM_TurnToTarget( d, sm.org );
	}
	sm.pitch = AngleNormalize180( ps->viewangles[PITCH] );
	sm.yaw = AngleNormalize360( ps->viewangles[YAW] + cmRecording.yawDelta );
	sm.legs = ps->legsAnim;
	sm.torso = ps->torsoAnim;
	sm.flags = ( ps->legsFlip ? 1 : 0 ) | ( ps->torsoFlip ? 2 : 0 );
	sm.holstered = ps->saberHolstered;
	sm.force = ps->fd.forcePowersActive;
	sm.saberMove = ps->saberMove;
	CM_RecordEvents( ps, sm.ms );
	last = cmRecording.rec.count ? &cmRecording.rec.s[cmRecording.rec.count - 1] : NULL;
	if ( !last || sm.ms - last->ms >= CM_REC_EVERY || sm.legs != last->legs || sm.torso != last->torso || sm.flags != last->flags
		|| sm.holstered != last->holstered || sm.force != last->force || sm.saberMove != last->saberMove )
		cmRecording.rec.s[cmRecording.rec.count++] = sm;
	if ( sm.ms >= CM_REC_MAX_MS || cmRecording.rec.count >= CM_MAX_SAMPLES ) {
		Com_Printf( S_COLOR_YELLOW "Cinematic manager: a recording is %i s at most\n", CM_REC_MAX_MS / 1000 );
		CM_StopRecording( qtrue );
	}
}

// its key ends it, whatever the key is bound to
qboolean CL_Cinematic_RecordKey( int key ) {
	if ( !cmRecording.phase || key != CM_RecordStopKey() )
		return qfalse;
	CM_StopRecording( (qboolean)( cmRecording.phase == 2 ) );
	return qtrue;
}

static void CM_DrawRecording( void ) {
	static const vec4_t red = { 1.0f, 0.3f, 0.25f, 1.0f };
	const char *text;

	if ( cmRecording.phase == 1 && cmRecording.target >= 0 && cmRecording.away )
		text = va( "Recording for %s in %i...   you see from where they are, walls are where you are; %s ends it", cmRecording.who,
			1 + ( CM_REC_COUNTDOWN - ( cls.realtime - cmRecording.start ) ) / 1000, Key_KeynumToString( CM_RecordStopKey() ) );
	else if ( cmRecording.phase == 1 )
		text = va( "Recording for %s in %i...   act it out, %s ends it", cmRecording.who,
			1 + ( CM_REC_COUNTDOWN - ( cls.realtime - cmRecording.start ) ) / 1000, Key_KeynumToString( CM_RecordStopKey() ) );
	else
		text = va( "%s  Recording for %s   %.1f s   %s ends it", ( ( cls.realtime >> 9 ) & 1 ) ? "(*)" : "( )", cmRecording.who,
			( cls.realtime - cmRecording.start ) / 1000.0f, Key_KeynumToString( CM_RecordStopKey() ) );
	re->Font_DrawString( (int)( ( SCREEN_WIDTH - re->Font_StrLenPixels( text, cm.font ? cm.font : cls.menuFont, 0.9f ) ) * 0.5f ), 60, text, red,
		( cm.font ? cm.font : cls.menuFont ) | STYLE_DROPSHADOW, -1, 0.9f );
}

/*
===============================================================================

Clashes: while a cinematic plays, or one is recorded, blades that cross throw sparks and ring,
as the server would have them clash; worked out from where the cgame draws them

===============================================================================
*/

#define CM_MAX_BLADES		32
#define CM_CLASH_NEAR		16.0f		// blades this near meet anywhere along them, as generous as the server's blocks
#define CM_CLASH_SAME		20.0f		// hilts this near are one saber's, when whose they are isn't known
#define CM_OWNER_NEAR		48.0f		// a hilt this near someone is in their hand
#define CM_CLASH_MS			220			// a clash there again no sooner than this
#define CM_HIT_NEAR			9.0f		// a blade this near someone's middle cuts them
#define CM_HIT_OWN			40.0f		// a hilt this near is their own saber
#define CM_HIT_MS			350			// someone cut again no sooner than this

typedef struct {
	vec3_t		base, tip;
	int			owner;				// whose hand it's in, -1 not known
} cmBlade_t;

static cmBlade_t cmBlades[CM_MAX_BLADES], cmLastBlades[CM_MAX_BLADES];
static int cmNumBlades, cmNumLastBlades;
static int cmLastHit[MAX_GENTITIES];

static struct {
	vec3_t		at;
	int			time;
} cmClashes[8];

void CM_NoteBlade( const vec3_t origin, const vec3_t dir, float length ) {
	if ( cmNumBlades >= CM_MAX_BLADES || length < 4.0f || ( !cmp.playing && cmRecording.phase != 2 ) )
		return;
	// the same blade drawn twice in a frame counts once
	for ( int i = 0; i < cmNumBlades; i++ ) {
		if ( VectorCompare( cmBlades[i].base, origin ) )
			return;
	}
	VectorCopy( origin, cmBlades[cmNumBlades].base );
	VectorMA( origin, length, dir, cmBlades[cmNumBlades].tip );
	cmBlades[cmNumBlades].owner = -1;
	cmNumBlades++;
}

// the players and NPCs about, where they're shown
typedef struct {
	int			num;
	vec3_t		pos;
} cmBody_t;

#define CM_MAX_BODIES		64

static int CM_Bodies( cmBody_t *out, int max ) {
	int n = 0;

	for ( int i = -1; i < cl.snap.numEntities && n < max; i++ ) {
		const entityState_t *es = i >= 0 ? &cl.parseEntities[( cl.snap.parseEntitiesNum + i ) & ( MAX_PARSE_ENTITIES - 1 )] : NULL;
		cmBody_t *b = &out[n];

		if ( es && ( ( es->eType != ET_PLAYER && es->eType != ET_NPC ) || ( es->eFlags & EF_DEAD ) || es->number == clc.clientNum ) )
			continue;
		b->num = es ? es->number : clc.clientNum;
		if ( b->num < 0 || b->num >= MAX_GENTITIES )
			continue;
		if ( !CL_NpcWalk_PoseOf( b->num, b->pos ) )
			VectorCopy( es ? es->pos.trBase : cl.snap.ps.origin, b->pos );
		n++;
	}
	return n;
}

// the nearest the segments a-b and c-d come, and the point midway
static float CM_SegmentsNear( const vec3_t a, const vec3_t b, const vec3_t c, const vec3_t d, vec3_t mid ) {
	vec3_t u, v, w, p, q;
	float uu, vv, uv, den, sc, tc, uw, vw;

	VectorSubtract( b, a, u );
	VectorSubtract( d, c, v );
	uu = DotProduct( u, u );
	vv = DotProduct( v, v );
	uv = DotProduct( u, v );
	den = uu * vv - uv * uv;
	VectorSubtract( a, c, w );
	uw = DotProduct( u, w );
	vw = DotProduct( v, w );
	sc = den > 0.0001f ? Com_Clamp( 0.0f, 1.0f, ( uv * vw - vv * uw ) / den ) : 0.0f;
	tc = vv > 0.0001f ? Com_Clamp( 0.0f, 1.0f, ( uv * sc + vw ) / vv ) : 0.0f;
	sc = uu > 0.0001f ? Com_Clamp( 0.0f, 1.0f, ( uv * tc - uw ) / uu ) : 0.0f;
	VectorMA( a, sc, u, p );
	VectorMA( c, tc, v, q );
	VectorAdd( p, q, mid );
	VectorScale( mid, 0.5f, mid );
	return Distance( p, q );
}

// a blade as it was the frame before, found by its hilt; NULL if it's new
static const cmBlade_t *CM_LastBlade( const cmBlade_t *b ) {
	const cmBlade_t *best = NULL;
	float bestDist = 32.0f;

	for ( int i = 0; i < cmNumLastBlades; i++ ) {
		const float d = Distance( cmLastBlades[i].base, b->base );

		if ( b->owner >= 0 && cmLastBlades[i].owner >= 0 && cmLastBlades[i].owner != b->owner )
			continue;
		if ( d < bestDist ) {
			bestDist = d;
			best = &cmLastBlades[i];
		}
	}
	return best;
}

// a blade frac of the way from where it was the frame before to where it is
static void CM_BladeBetween( const cmBlade_t *b, const cmBlade_t *last, float frac, cmBlade_t *out ) {
	for ( int k = 0; k < 3; k++ ) {
		out->base[k] = last->base[k] + ( b->base[k] - last->base[k] ) * frac;
		out->tip[k] = last->tip[k] + ( b->tip[k] - last->tip[k] ) * frac;
	}
}

// two blades met this frame or swept through each other since the last: where; qfalse if not
static qboolean CM_BladesMet( const cmBlade_t *a, const cmBlade_t *b, vec3_t at ) {
	const cmBlade_t *la = CM_LastBlade( a ), *lb = CM_LastBlade( b );

	if ( CM_SegmentsNear( a->base, a->tip, b->base, b->tip, at ) <= CM_CLASH_NEAR )
		return qtrue;
	// a fast swing goes through in less than a frame
	for ( int s = 1; la && lb && s < 4; s++ ) {
		cmBlade_t ia, ib;

		CM_BladeBetween( a, la, s / 4.0f, &ia );
		CM_BladeBetween( b, lb, s / 4.0f, &ib );
		if ( CM_SegmentsNear( ia.base, ia.tip, ib.base, ib.tip, at ) <= CM_CLASH_NEAR )
			return qtrue;
	}
	return qfalse;
}

// whose hand each blade's in
static void CM_BladeOwners( const cmBody_t *bodies, int numBodies ) {
	for ( int b = 0; b < cmNumBlades; b++ ) {
		float best = CM_OWNER_NEAR;

		for ( int i = 0; i < numBodies; i++ ) {
			vec3_t low, high, at;
			float d;

			VectorCopy( bodies[i].pos, low );
			VectorCopy( bodies[i].pos, high );
			low[2] -= 20.0f;
			high[2] += 34.0f;
			d = CM_SegmentsNear( low, high, cmBlades[b].base, cmBlades[b].base, at );
			if ( d < best ) {
				best = d;
				cmBlades[b].owner = bodies[i].num;
			}
		}
	}
}

// someone a blade cuts: whoever's middle it goes through, not whoever holds it
static void CM_Hits( const cmBody_t *bodies, int numBodies ) {
	for ( int i = 0; i < numBodies; i++ ) {
		const int num = bodies[i].num;
		vec3_t low, high;

		if ( num < 0 || num >= MAX_GENTITIES || cls.realtime - cmLastHit[num] < CM_HIT_MS )
			continue;
		VectorCopy( bodies[i].pos, low );
		VectorCopy( bodies[i].pos, high );
		low[2] -= 20.0f;
		high[2] += 34.0f;
		for ( int b = 0; b < cmNumBlades; b++ ) {
			vec3_t at, hilt, dir;

			// their own, from their hand
			if ( cmBlades[b].owner == num || ( cmBlades[b].owner < 0 && CM_SegmentsNear( low, high, cmBlades[b].base, cmBlades[b].base, hilt ) < CM_HIT_OWN )
				|| CM_SegmentsNear( low, high, cmBlades[b].base, cmBlades[b].tip, at ) > CM_HIT_NEAR )
				continue;
			cmLastHit[num] = cls.realtime;
			VectorSubtract( cmBlades[b].tip, cmBlades[b].base, dir );
			VectorNormalize( dir );
			FX_PlayEffect( "saber/blood_sparks_mp.efx", at, dir, 0, 0 );
			CM_SoundAt( at, va( "sound/weapons/saber/saberhit%i.wav", Q_irand( 1, 3 ) ) );
			break;
		}
	}
}

// last frame's blades, crossed: sparks and a ring where they meet; someone cut, sparks and a hiss
static void CM_Clashes( void ) {
	if ( cmNumBlades ) {
		static cmBody_t bodies[CM_MAX_BODIES];
		const int numBodies = CM_Bodies( bodies, CM_MAX_BODIES );

		CM_BladeOwners( bodies, numBodies );
		CM_Hits( bodies, numBodies );
	}
	for ( int i = 0; i < cmNumBlades; i++ ) {
		for ( int j = i + 1; j < cmNumBlades; j++ ) {
			vec3_t mid, dir;
			int k, oldest = 0;

			// two hands' blades, wherever along them they meet
			if ( cmBlades[i].owner >= 0 || cmBlades[j].owner >= 0 ) {
				if ( cmBlades[i].owner == cmBlades[j].owner )
					continue;
			} else if ( Distance( cmBlades[i].base, cmBlades[j].base ) < CM_CLASH_SAME ) {
				continue;
			}
			if ( !CM_BladesMet( &cmBlades[i], &cmBlades[j], mid ) )
				continue;
			for ( k = 0; k < (int)ARRAY_LEN( cmClashes ); k++ ) {
				if ( cls.realtime - cmClashes[k].time < CM_CLASH_MS && Distance( cmClashes[k].at, mid ) < 48.0f )
					break;
				if ( cmClashes[k].time < cmClashes[oldest].time )
					oldest = k;
			}
			if ( k < (int)ARRAY_LEN( cmClashes ) )
				continue;
			VectorCopy( mid, cmClashes[oldest].at );
			cmClashes[oldest].time = cls.realtime;
			VectorSubtract( cmBlades[i].tip, cmBlades[i].base, dir );
			VectorNormalize( dir );
			FX_PlayEffect( "saber/saber_block.efx", mid, dir, 0, 0 );
			CM_SoundAt( mid, va( "sound/weapons/saber/saberblock%i.wav", Q_irand( 1, 9 ) ) );
		}
	}
	// kept, to see how they swept by the next frame
	memcpy( cmLastBlades, cmBlades, cmNumBlades * sizeof( cmBlades[0] ) );
	cmNumLastBlades = cmNumBlades;
	cmNumBlades = 0;
}

/*
===============================================================================

Looks: how people are drawn while a look of theirs lasts, on this screen

===============================================================================
*/

#define CM_MAX_LOOKS		32
#define CM_LOOK_BODIES		( MAX_CLIENTS + 128 )
#define CM_BODY_REACH		48.0f	// a body drawn this near someone is theirs
#define CM_BLADE_REACH		96.0f	// and a blade, from its hilt

typedef struct {
	int				num;
	const cmClip_t	*clip;
	vec3_t			org;
} cmLookNow_t;

static cmLookNow_t	cmLooks[CM_MAX_LOOKS];
static int			cmNumLooks, cmLooksFrame = -1;
static qboolean		cmOnlyCast;			// everyone else left out
static int			cmTalk[CM_MAX_CLIPS], cmNumTalk;	// saying a line now, their mouths going
static int			cmFaced[64], cmNumFaced;			// given a face, to stop it after
static int			cmCast[CM_MAX_ENTS * CM_MAX_GROUP], cmNumCast;
static int			cmBodyNums[CM_LOOK_BODIES];
static vec3_t		cmBodyOrgs[CM_LOOK_BODIES];
static int			cmNumBodies;

// the looks going on now, and where everyone is, once a frame
static void CM_LooksFrame( void ) {
	const cmCine_t *c = NULL;
	int t = 0;

	if ( cmLooksFrame == cls.framecount )
		return;
	cmLooksFrame = cls.framecount;
	cmNumLooks = cmNumBodies = cmNumCast = cmNumTalk = 0;
	cmOnlyCast = qfalse;
	if ( cmp.playing ) {
		c = &cmp.cine;
		t = cmp.t;
	} else if ( cm.open && cm.tlPinned ) {
		c = &cm.cine;
		t = cm.tlMs;
	}
	if ( !c || cls.state != CA_ACTIVE )
		return;
	// those in it, when no one else is to be seen
	if ( c->onlyCast ) {
		cmOnlyCast = qtrue;
		for ( int e = 0; e < c->numEnts; e++ )
			cmNumCast += CM_Resolve( c->ents[e].who, cmCast + cmNumCast, CM_MAX_GROUP );
	}
	// those saying a line now
	for ( int i = 0; i < c->numClips && cmNumTalk < CM_MAX_CLIPS; i++ ) {
		const cmClip_t *clip = &c->clips[i];

		if ( clip->kind == CM_CLIP_SAY && clip->text[0] && t >= clip->start && t < clip->start + CM_SAY_MS )
			cmNumTalk += CM_Resolve( c->ents[clip->ent].who, cmTalk + cmNumTalk, Q_min( CM_MAX_GROUP, CM_MAX_CLIPS - cmNumTalk ) );
	}
	// the one that started last, for each of them
	for ( int i = 0; i < c->numClips; i++ ) {
		const cmClip_t *clip = &c->clips[i];
		int nums[CM_MAX_GROUP], n;

		if ( clip->kind != CM_CLIP_LOOK || t < clip->start || t >= clip->start + ( clip->a.ms > 0 ? clip->a.ms : CM_Total( c ) - clip->start + 1 )
			|| CM_GoneAt( c, clip->ent, t ) )
			continue;
		n = CM_Resolve( c->ents[clip->ent].who, nums, CM_MAX_GROUP );
		for ( int m = 0; m < n; m++ ) {
			int j;

			for ( j = 0; j < cmNumLooks && cmLooks[j].num != nums[m]; j++ )
				;
			if ( j < cmNumLooks ) {
				if ( clip->start >= cmLooks[j].clip->start )
					cmLooks[j].clip = clip;
			} else if ( cmNumLooks < CM_MAX_LOOKS && RC_EntityOrigin( nums[m], cmLooks[j].org ) ) {
				cmLooks[j].num = nums[m];
				cmLooks[j].clip = clip;
				cmNumLooks++;
			}
		}
	}
	if ( !cmNumLooks && !cmOnlyCast && !cmNumTalk && !cmNumFaced )
		return;
	// everyone drawn, to tell whose a body or a blade is
	if ( RC_EntityOrigin( clc.clientNum, cmBodyOrgs[0] ) )
		cmBodyNums[cmNumBodies++] = clc.clientNum;
	for ( int i = 0; i < cl.snap.numEntities && cmNumBodies < CM_LOOK_BODIES; i++ ) {
		const entityState_t *es = &cl.parseEntities[( cl.snap.parseEntitiesNum + i ) & ( MAX_PARSE_ENTITIES - 1 )];

		if ( ( es->eType == ET_PLAYER || es->eType == ET_NPC ) && es->number != clc.clientNum && RC_EntityOrigin( es->number, cmBodyOrgs[cmNumBodies] ) )
			cmBodyNums[cmNumBodies++] = es->number;
	}
	cmNumBodies += CL_Extras_Bodies( cmBodyNums + cmNumBodies, cmBodyOrgs + cmNumBodies, CM_LOOK_BODIES - cmNumBodies );
}

// the look of whoever is nearest the point, if they're that near and have one
static const cmLookNow_t *CM_LookAt( const vec3_t p, float reach, qboolean *leaveOut, int *who ) {
	float best = reach;
	int num = -1;

	*leaveOut = qfalse;
	*who = -1;
	for ( int i = 0; i < cmNumBodies; i++ ) {
		const float d = Distance( p, cmBodyOrgs[i] );

		if ( d < best ) {
			best = d;
			num = cmBodyNums[i];
		}
	}
	*who = num;
	// someone not in it, when only those in it are seen
	if ( cmOnlyCast && num >= 0 ) {
		int i = 0;

		while ( i < cmNumCast && cmCast[i] != num )
			i++;
		if ( i == cmNumCast ) {
			*leaveOut = qtrue;
			return NULL;
		}
	}
	for ( int i = 0; num >= 0 && i < cmNumLooks; i++ ) {
		if ( cmLooks[i].num == num )
			return &cmLooks[i];
	}
	return NULL;
}

static void CM_LookScalePoint( vec3_t p, const vec3_t feet, float k ) {
	vec3_t d;

	VectorSubtract( p, feet, d );
	VectorMA( feet, k, d, p );
}

// someone's face as it's drawn: one of the game's faces, or talking while they say a line; one
// given a face before goes back to the cgame's when it's done
static void CM_Face( const refEntity_t *body, int num, int face ) {
	CGhoul2Info_v &g2 = *(CGhoul2Info_v *)body->ghoul2;
	const rcAnimDef_t *d;
	const char *gla, *anim;
	int faced = 0;

	if ( !face ) {
		for ( int i = 0; i < cmNumTalk && !face; i++ ) {
			if ( cmTalk[i] == num )
				face = CM_FACE_TALK;
		}
	}
	while ( faced < cmNumFaced && cmFaced[faced] != num )
		faced++;
	if ( !re->G2API_HaveWeGhoul2Models( g2 ) )
		return;
	if ( !face ) {
		// done with: a face at rest, until the cgame gives its own
		if ( faced < cmNumFaced ) {
			cmFaced[faced] = cmFaced[--cmNumFaced];
			face = -1;
		} else {
			return;
		}
	} else if ( faced == cmNumFaced && cmNumFaced < (int)ARRAY_LEN( cmFaced ) ) {
		cmFaced[cmNumFaced++] = num;
	}
	// talking: one mouth shape after another
	anim = face < 0 ? "FACE_TALK0" : cmFaces[face].anim;
	if ( face == CM_FACE_TALK )
		anim = va( "FACE_TALK%i", 1 + ( cls.realtime / 220 + num ) % 4 );
	gla = re->G2API_GetGLAName( g2, 0 );
	if ( !gla || !gla[0] || !( d = RC_FindAnim( RC_AnimCfg( gla ), anim ) ) || d->num <= 0 || !d->fps )
		return;
	re->G2API_SetBoneAnim( g2, 0, "face", d->first, d->first + d->num, face == 5 ? BONE_ANIM_OVERRIDE_FREEZE : BONE_ANIM_OVERRIDE_LOOP,
		fabsf( d->fps ) / 20.0f, re->G2API_GetTime( cl.serverTime ), -1, 100 );
}

// a model in someone's hand, where the hand's bolt is on the body as it's drawn, turned in it
static void CM_LookHold( const refEntity_t *body, const cmClip_t *clip, float k ) {
	CGhoul2Info_v &g2 = *(CGhoul2Info_v *)body->ghoul2;
	refEntity_t held;
	matrix3_t boltAxis, turn;
	mdxaBone_t m;
	vec3_t angles;
	qhandle_t h;
	int bolt;

	if ( !re->G2API_HaveWeGhoul2Models( g2 ) || !( h = re->RegisterModel( clip->hold ) ) )
		return;
	if ( ( bolt = re->G2API_AddBolt( g2, 0, clip->holdHand ? "*l_hand" : "*r_hand" ) ) < 0 )
		return;
	vectoangles( body->axis[0], angles );
	if ( !re->G2API_GetBoltMatrix( g2, 0, bolt, &m, angles, body->origin, cl.serverTime, NULL, (float *)body->modelScale ) )
		return;
	memset( &held, 0, sizeof( held ) );
	held.reType = RT_MODEL;
	held.hModel = h;
	VectorSet( held.origin, m.matrix[0][3], m.matrix[1][3], m.matrix[2][3] );
	VectorCopy( held.origin, held.oldorigin );
	VectorCopy( held.origin, held.lightingOrigin );
	for ( int i = 0; i < 3; i++ ) {
		VectorSet( boltAxis[i], m.matrix[0][i], m.matrix[1][i], m.matrix[2][i] );
		VectorNormalize( boltAxis[i] );
	}
	AnglesToAxis( clip->holdAng, turn );
	MatrixMultiply( turn, boltAxis, held.axis );
	if ( k != 1.0f ) {
		for ( int i = 0; i < 3; i++ )
			VectorScale( held.axis[i], k, held.axis[i] );
		held.nonNormalizedAxes = qtrue;
	}
	held.shaderRGBA[0] = held.shaderRGBA[1] = held.shaderRGBA[2] = held.shaderRGBA[3] = 255;
	re->AddRefEntityToScene( &held );
}

// what cgame draws, as a look has it: a body hidden, scaled, drawn as a hologram or with a shell
// over it; a blade hidden, coloured, longer or shorter, and where a scaled hand holds it. NULL
// leaves it out
const refEntity_t *CM_LookEntity( const refEntity_t *ent, refEntity_t *copy ) {
	const qboolean body = (qboolean)( ent->ghoul2 && ent->reType == RT_MODEL );
	const qboolean glow = (qboolean)( ent->reType == RT_SABER_GLOW ), line = (qboolean)( ent->reType == RT_LINE );
	const cmLookNow_t *l;
	const cmClip_t *clip;
	float k;

	if ( !body && !glow && !line )
		return ent;
	CM_LooksFrame();
	if ( !cmNumLooks && !cmOnlyCast && !cmNumTalk && !cmNumFaced )
		return ent;
	{
		qboolean leaveOut;
		int who;

		l = CM_LookAt( line ? ent->oldorigin : ent->origin, body ? CM_BODY_REACH : CM_BLADE_REACH, &leaveOut, &who );
		if ( leaveOut )
			return NULL;
		// a face: the look's, or talking while they say a line
		if ( body && who >= 0 )
			CM_Face( ent, who, l && !l->clip->hidden ? l->clip->face : 0 );
		if ( !l )
			return ent;
	}
	clip = l->clip;
	if ( clip->hidden || ( !body && clip->saber == CM_SABER_OFF ) )
		return NULL;
	if ( copy != ent )
		*copy = *ent;
	k = clip->size / 100.0f;
	if ( body ) {
		if ( clip->size != 100 ) {
			const float was = ent->modelScale[2] > 0.0f ? ent->modelScale[2] : 1.0f;

			// as cgame scales a model: its bones' places, and its axes
			for ( int i = 0; i < 3; i++ ) {
				copy->modelScale[i] = ( ent->modelScale[i] > 0.0f ? ent->modelScale[i] : 1.0f ) * k;
				VectorScale( copy->axis[i], k, copy->axis[i] );
			}
			copy->nonNormalizedAxes = qtrue;
			// standing on the same floor
			copy->origin[2] += CM_LOOK_UP * was * ( k - 1.0f );
		}
		if ( clip->hold[0] )
			CM_LookHold( copy, clip, k );
		if ( clip->shell ) {
			const cmShell_t *sh = &cmShells[clip->shell];
			const qhandle_t h = re->RegisterShader( sh->shader );

			if ( h && sh->over ) {
				refEntity_t over = *copy;

				over.customShader = h;
				over.renderfx &= ~RF_RGB_TINT;
				over.shaderRGBA[0] = over.shaderRGBA[1] = over.shaderRGBA[2] = over.shaderRGBA[3] = 255;
				re->AddRefEntityToScene( &over );
			} else if ( h ) {
				copy->customShader = h;
				copy->renderfx &= ~RF_RGB_TINT;
				copy->shaderRGBA[0] = copy->shaderRGBA[1] = copy->shaderRGBA[2] = copy->shaderRGBA[3] = 255;
			}
		}
		return copy;
	}

	// a blade: its length, its colour, then where it is in a hand scaled about their feet
	if ( glow )
		copy->saberLength *= clip->blade / 100.0f;
	else if ( clip->blade != 100 ) {
		vec3_t d;

		// the core: origin its tip, oldorigin the hilt
		VectorSubtract( copy->origin, copy->oldorigin, d );
		VectorMA( copy->oldorigin, clip->blade / 100.0f, d, copy->origin );
	}
	if ( clip->saber >= CM_SABER_COLORS ) {
		const qhandle_t h = re->RegisterShader( va( "gfx/effects/sabers/%s_%s", cmSaberLooks[clip->saber], glow ? "glow" : "line" ) );

		if ( h ) {
			copy->customShader = h;
			copy->shaderRGBA[0] = copy->shaderRGBA[1] = copy->shaderRGBA[2] = 255;
		}
	}
	if ( clip->size != 100 ) {
		vec3_t feet;

		VectorCopy( l->org, feet );
		feet[2] -= CM_LOOK_UP;
		CM_LookScalePoint( copy->origin, feet, k );
		if ( line )
			CM_LookScalePoint( copy->oldorigin, feet, k );
		else
			copy->saberLength *= k;
		copy->radius *= k;
	}
	return copy;
}

/*
===============================================================================

Every frame, and the command

===============================================================================
*/

/*
===============================================================================

The camera flown and kept as a shot

===============================================================================
*/

static void CM_CamRecStart( void ) {
	cm.camRec = qtrue;
	cm.camRecStart = cls.realtime;
	cm.camRecNext = 0;
	cm.camRecCount = 0;
	cm.through = qfalse;
	CM_Message( "Recording the camera: fly it (hold right mouse), then Stop or Enter; a minute at most" );
}

// the take as a shot after the one shown: its spots spread over it, each as far in as it was flown
static void CM_CamRecStop( void ) {
	cmCine_t *c = &cm.cine;
	const int n = cm.camRecCount;
	rcShot_t *s;
	int at, keys, prev = 0;

	cm.camRec = qfalse;
	if ( n < 2 ) {
		CM_Message( S_COLOR_YELLOW "Too short to keep" );
		return;
	}
	if ( c->numShots >= CM_MAX_SHOTS ) {
		CM_Message( S_COLOR_YELLOW "A cinematic has 16 shots at most" );
		return;
	}
	at = c->numShots ? cm.cur + 1 : 0;
	CM_ShiftClips( CM_ShotStart( c, at ), cm.camRecMs[n - 1] );
	memmove( &c->shots[at + 1], &c->shots[at], ( c->numShots - at ) * sizeof( c->shots[0] ) );
	c->numShots++;
	s = &c->shots[at];
	memset( s, 0, sizeof( *s ) );
	keys = Q_min( RC_MAX_KEYS, n );
	for ( int k = 0; k < keys; k++ ) {
		const int i = k * ( n - 1 ) / ( keys - 1 );
		rcKey_t *key = &s->keys[k];

		VectorCopy( cm.camRecOrg[i], key->org );
		VectorCopy( cm.camRecAng[i], key->ang );
		key->fov = cm.camRecFov[i];
		key->ms = k ? cm.camRecMs[i] - prev : 0;
		prev = cm.camRecMs[i];
		// turned the short way from the one before, so the curve doesn't spin
		if ( k ) {
			for ( int j = 0; j < 3; j++ )
				key->ang[j] = s->keys[k - 1].ang[j] + AngleNormalize180( key->ang[j] - s->keys[k - 1].ang[j] );
		}
	}
	s->numKeys = keys;
	s->linear = qtrue;		// as fast as it was flown
	CM_SeeShots();
	cm.cur = -1;
	CM_SelectShot( at );
	CM_Changed();
	CM_Message( va( S_COLOR_GREEN "Kept %.1f s as shot %i, %i spots; Track page: Handheld for a camera held by hand", cm.camRecMs[n - 1] / 1000.0f, at + 1, keys ) );
}

static void CM_CamRecFrame( void ) {
	const int ms = cls.realtime - cm.camRecStart;

	if ( !cm.camRec || cls.realtime < cm.camRecNext )
		return;
	if ( cm.camRecCount >= CM_CAMREC_MAX ) {
		CM_CamRecStop();
		return;
	}
	cm.camRecNext = cls.realtime + CM_CAMREC_EVERY;
	VectorCopy( cm.camOrg, cm.camRecOrg[cm.camRecCount] );
	VectorCopy( cm.camAng, cm.camRecAng[cm.camRecCount] );
	cm.camRecFov[cm.camRecCount] = cm.camFov;
	cm.camRecMs[cm.camRecCount] = ms;
	cm.camRecCount++;
}

/*
===============================================================================

Undo, redo; copy and paste

===============================================================================
*/

static std::string CM_CineText( void ) {
	char *buf = (char *)Z_Malloc( CM_TEXT_MAX, TAG_GENERAL, qfalse );
	std::string text;

	CM_ToText( &cm.cine, buf, CM_TEXT_MAX );
	text = buf;
	Z_Free( buf );
	return text;
}

// a cinematic opened or begun: nothing to go back to
static void CM_UndoReset( void ) {
	cmUndo.clear();
	cmRedo.clear();
	cmUndoBase = CM_CineText();
	cmUndoPending = qfalse;
}

// a change kept once it's settled, or at once with force
static void CM_UndoKeep( qboolean force ) {
	std::string now;

	if ( !cmUndoPending )
		return;
	if ( !force && ( cm.held[A_MOUSE1] || cls.realtime - cmUndoTime < CM_UNDO_SETTLE_MS ) )
		return;
	cmUndoPending = qfalse;
	now = CM_CineText();
	if ( now == cmUndoBase )
		return;
	cmUndo.push_back( cmUndoBase );
	if ( (int)cmUndo.size() > CM_UNDO_MAX )
		cmUndo.erase( cmUndo.begin() );
	cmRedo.clear();
	cmUndoBase = now;
}

static void CM_UndoFrame( void ) {
	CM_UndoKeep( qfalse );
}

// the cinematic as it was, or as it was again; what's picked kept where it still can be
static void CM_Undo( qboolean redo ) {
	std::vector<std::string> &from = redo ? cmRedo : cmUndo, &to = redo ? cmUndo : cmRedo;
	char *buf;

	CM_UndoKeep( qtrue );
	if ( from.empty() ) {
		CM_Message( redo ? S_COLOR_YELLOW "Nothing to redo" : S_COLOR_YELLOW "Nothing to undo" );
		return;
	}
	to.push_back( cmUndoBase );
	cmUndoBase = from.back();
	from.pop_back();
	buf = (char *)Z_Malloc( cmUndoBase.size() + 1, TAG_GENERAL, qfalse );
	memcpy( buf, cmUndoBase.c_str(), cmUndoBase.size() + 1 );
	CM_Parse( buf, &cm.cine );
	Z_Free( buf );
	cmUndoPending = qfalse;
	cm.dirty = qtrue;
	cm.typing = CM_TYPE_NONE;
	cm.tlDrag = CM_DRAG_NONE;
	cm.numMulti = 0;
	cm.cur = Com_Clampi( 0, Q_max( 0, cm.cine.numShots - 1 ), cm.cur );
	cm.sel = -1;
	if ( cm.ent >= cm.cine.numEnts )
		cm.ent = -1;
	if ( cm.clip >= cm.cine.numClips )
		cm.clip = -1;
	cm.seenShots = -1;
	CM_SeeShots();
	CM_Message( va( "%s: %i more to %s, %i to %s", redo ? "Redone" : "Undone", (int)from.size(), redo ? "redo" : "undo", (int)to.size(), redo ? "undo" : "redo" ) );
}

// what Ctrl+C keeps
typedef enum {
	CM_BOARD_NONE,
	CM_BOARD_SHOT,
	CM_BOARD_CLIPS,			// an action, or all someone's
	CM_BOARD_FX,
	CM_BOARD_SND,
	CM_BOARD_TXT,
	CM_BOARD_PROP,
	CM_BOARD_LIGHT,
	CM_BOARD_SCREEN,
	CM_BOARD_MAPLOOK
} cmBoardKind_t;

static struct {
	int			kind;
	char		what[64];
	rcShot_t	shot;
	int			numClips;
	cmClip_t	clips[CM_MAX_CLIPS];
	int			recOf[CM_MAX_CLIPS];	// a recording's in recs, -1 none
	cmRec_t		recs[CM_MAX_RECS];
	int			numRecs;
	cmFx_t		fx;
	cmSnd_t		snd;
	cmTxt_t		txt;
	cmProp_t	prop;
	cmLight_t	light;
	cmScreen_t	screen;
	cmMapLook_t	mapLook;
} cmBoard;

static void CM_BoardClip( const cmClip_t *clip ) {
	const cmRec_t *r;

	if ( cmBoard.numClips >= CM_MAX_CLIPS )
		return;
	cmBoard.clips[cmBoard.numClips] = *clip;
	cmBoard.recOf[cmBoard.numClips] = -1;
	// a recording goes with its moments, for another cinematic too
	if ( clip->kind == CM_CLIP_RECORD && ( r = CM_Rec( &cm.cine, clip ) ) != NULL && cmBoard.numRecs < CM_MAX_RECS ) {
		cmBoard.recs[cmBoard.numRecs] = *r;
		cmBoard.recOf[cmBoard.numClips] = cmBoard.numRecs++;
	}
	cmBoard.numClips++;
}

// the shot, the action (or all of someone's), the effect, sound, text, prop or light picked
static void CM_Copy( void ) {
	cmClip_t *clip = CM_Clip();

	cmBoard.kind = CM_BOARD_NONE;
	cmBoard.numClips = cmBoard.numRecs = 0;
	switch ( cm.tab ) {
	case CM_TAB_CAMERA:
		if ( CM_Shot() ) {
			cmBoard.kind = CM_BOARD_SHOT;
			cmBoard.shot = *CM_Shot();
			Com_sprintf( cmBoard.what, sizeof( cmBoard.what ), "shot %i", cm.cur + 1 );
		}
		break;
	case CM_TAB_ENTITIES:
		if ( clip && cm.ent >= 0 && clip->ent == cm.ent ) {
			CM_BoardClip( clip );
			Q_strncpyz( cmBoard.what, "the action", sizeof( cmBoard.what ) );
		} else if ( cm.ent >= 0 && cm.ent < cm.cine.numEnts ) {
			for ( int i = 0; i < cm.cine.numClips; i++ ) {
				if ( cm.cine.clips[i].ent == cm.ent )
					CM_BoardClip( &cm.cine.clips[i] );
			}
			Com_sprintf( cmBoard.what, sizeof( cmBoard.what ), "all %s does", cm.cine.ents[cm.ent].who );
		}
		if ( cmBoard.numClips )
			cmBoard.kind = CM_BOARD_CLIPS;
		break;
	case CM_TAB_EFFECTS:
		if ( CM_Fx() ) {
			cmBoard.kind = CM_BOARD_FX;
			cmBoard.fx = *CM_Fx();
			Q_strncpyz( cmBoard.what, "the effect", sizeof( cmBoard.what ) );
		}
		break;
	case CM_TAB_SOUNDS:
		if ( CM_Snd() ) {
			cmBoard.kind = CM_BOARD_SND;
			cmBoard.snd = *CM_Snd();
			Q_strncpyz( cmBoard.what, "the sound", sizeof( cmBoard.what ) );
		}
		break;
	case CM_TAB_TEXT:
		if ( CM_Txt() ) {
			cmBoard.kind = CM_BOARD_TXT;
			cmBoard.txt = *CM_Txt();
			Q_strncpyz( cmBoard.what, "the text", sizeof( cmBoard.what ) );
		}
		break;
	case CM_TAB_WORLD:
		if ( cm.worldPage == 0 && CM_Prop() ) {
			cmBoard.kind = CM_BOARD_PROP;
			cmBoard.prop = *CM_Prop();
			Q_strncpyz( cmBoard.what, "the prop", sizeof( cmBoard.what ) );
		} else if ( cm.worldPage == 1 && CM_Light() ) {
			cmBoard.kind = CM_BOARD_LIGHT;
			cmBoard.light = *CM_Light();
			Q_strncpyz( cmBoard.what, "the light", sizeof( cmBoard.what ) );
		} else if ( cm.worldPage == 3 && CM_MapLook() ) {
			cmBoard.kind = CM_BOARD_MAPLOOK;
			cmBoard.mapLook = *CM_MapLook();
			Q_strncpyz( cmBoard.what, "the map's look", sizeof( cmBoard.what ) );
		} else if ( cm.worldPage == 2 && CM_Screen() ) {
			cmBoard.kind = CM_BOARD_SCREEN;
			cmBoard.screen = *CM_Screen();
			Q_strncpyz( cmBoard.what, va( "%s", cmScreenKinds[cmBoard.screen.kind] ), sizeof( cmBoard.what ) );
		}
		break;
	default:
		break;
	}
	if ( cmBoard.kind == CM_BOARD_NONE )
		CM_Message( S_COLOR_YELLOW "Nothing picked to copy here" );
	else
		CM_Message( va( "Copied %s: Ctrl+V puts it in at the moment picked", cmBoard.what ) );
}

// a recording slot no action uses, -1 none
static int CM_FreeRec( const cmCine_t *c ) {
	for ( int r = 0; r < CM_MAX_RECS; r++ ) {
		qboolean used = qfalse;

		for ( int i = 0; i < c->numClips && !used; i++ )
			used = (qboolean)( c->clips[i].kind == CM_CLIP_RECORD && c->clips[i].rec == r );
		if ( !used )
			return r;
	}
	return -1;
}

// a moment for something pasted: the one picked on the timeline, else when it was
static int CM_PasteAt( int was ) {
	return cm.tlPinned ? cm.tlMs : was;
}

static void CM_Paste( void ) {
	cmCine_t *c = &cm.cine;

	switch ( cmBoard.kind ) {
	case CM_BOARD_SHOT: {
		const int at = c->numShots ? cm.cur + 1 : 0;

		if ( c->numShots >= CM_MAX_SHOTS ) {
			CM_Message( S_COLOR_YELLOW "A cinematic has 16 shots at most" );
			return;
		}
		CM_ShiftClips( CM_ShotStart( c, at ), CM_ShotLen( &cmBoard.shot ) );
		memmove( &c->shots[at + 1], &c->shots[at], ( c->numShots - at ) * sizeof( c->shots[0] ) );
		c->shots[at] = cmBoard.shot;
		c->numShots++;
		CM_SeeShots();
		cm.cur = -1;
		CM_SelectShot( at );
		cm.tab = CM_TAB_CAMERA;
		break;
	}
	case CM_BOARD_CLIPS: {
		int first = cmBoard.clips[0].start, pasted = 0;

		if ( cm.ent < 0 || cm.ent >= c->numEnts ) {
			CM_Message( S_COLOR_YELLOW "Pick who it's for first, someone in the cinematic" );
			return;
		}
		for ( int i = 1; i < cmBoard.numClips; i++ )
			first = Q_min( first, cmBoard.clips[i].start );
		for ( int i = 0; i < cmBoard.numClips && c->numClips < CM_MAX_CLIPS; i++ ) {
			cmClip_t *clip = &c->clips[c->numClips];

			*clip = cmBoard.clips[i];
			clip->ent = cm.ent;
			clip->start = Q_max( 0, clip->start - first + CM_PasteAt( first ) );
			if ( clip->kind == CM_CLIP_RECORD ) {
				const int r = cmBoard.recOf[i] >= 0 ? CM_FreeRec( c ) : -1;

				if ( r < 0 )
					continue;
				c->recs[r] = cmBoard.recs[cmBoard.recOf[i]];
				clip->rec = r;
			}
			cm.clip = c->numClips++;
			pasted++;
		}
		if ( pasted > 1 )
			cm.clip = -1;
		cm.tab = CM_TAB_ENTITIES;
		if ( pasted < cmBoard.numClips )
			CM_Message( S_COLOR_YELLOW "Not all of it fitted: 64 actions and 6 recordings at most" );
		break;
	}
	case CM_BOARD_FX:
		if ( c->numFx >= CM_MAX_FX )
			return;
		c->fx[c->numFx] = cmBoard.fx;
		c->fx[c->numFx].start = CM_PasteAt( cmBoard.fx.start );
		cm.fx = c->numFx++;
		cm.tab = CM_TAB_EFFECTS;
		break;
	case CM_BOARD_SND:
		if ( c->numSnd >= CM_MAX_SND )
			return;
		c->snd[c->numSnd] = cmBoard.snd;
		c->snd[c->numSnd].start = CM_PasteAt( cmBoard.snd.start );
		cm.snd = c->numSnd++;
		cm.tab = CM_TAB_SOUNDS;
		break;
	case CM_BOARD_TXT:
		if ( c->numTxt >= CM_MAX_TXT )
			return;
		c->txt[c->numTxt] = cmBoard.txt;
		c->txt[c->numTxt].start = CM_PasteAt( cmBoard.txt.start );
		cm.txt = c->numTxt++;
		cm.tab = CM_TAB_TEXT;
		break;
	case CM_BOARD_PROP:
		if ( c->numProps >= CM_MAX_PROPS )
			return;
		c->props[c->numProps] = cmBoard.prop;
		c->props[c->numProps].start = CM_PasteAt( cmBoard.prop.start );
		cm.prop = c->numProps++;
		cm.propKey = 0;
		cm.tab = CM_TAB_WORLD;
		cm.worldPage = 0;
		break;
	case CM_BOARD_LIGHT:
		if ( c->numLights >= CM_MAX_LIGHTS )
			return;
		c->lights[c->numLights] = cmBoard.light;
		c->lights[c->numLights].start = CM_PasteAt( cmBoard.light.start );
		cm.light = c->numLights++;
		cm.tab = CM_TAB_WORLD;
		cm.worldPage = 1;
		break;
	case CM_BOARD_MAPLOOK:
		if ( c->numMapLooks >= CM_MAX_MAPLOOKS )
			return;
		c->mapLooks[c->numMapLooks] = cmBoard.mapLook;
		c->mapLooks[c->numMapLooks].start = CM_PasteAt( cmBoard.mapLook.start );
		cm.mapLook = c->numMapLooks++;
		cm.tab = CM_TAB_WORLD;
		cm.worldPage = 3;
		break;
	case CM_BOARD_SCREEN:
		if ( c->numScreens >= CM_MAX_SCREENS )
			return;
		c->screens[c->numScreens] = cmBoard.screen;
		c->screens[c->numScreens].start = CM_PasteAt( cmBoard.screen.start );
		cm.screen = c->numScreens++;
		cm.tab = CM_TAB_WORLD;
		cm.worldPage = 2;
		break;
	default:
		CM_Message( S_COLOR_YELLOW "Nothing copied yet: Ctrl+C copies what's picked" );
		return;
	}
	CM_Changed();
	CM_Message( va( "Pasted %s%s", cmBoard.what, cm.tlPinned ? va( " at %.1f s", cm.tlMs / 1000.0f ) : "" ) );
}

/*
===============================================================================

Props and lights: in the scene while they last, on this screen

===============================================================================
*/

// the props and lights of the cinematic playing, or of the one being made at the moment picked
// (all of them, the props at their first key, with no moment picked)
void CL_Cinematic_AddToScene( const refdef_t *fd ) {
	const cmCine_t *c;
	int t;

	if ( ( fd->rdflags & ( RDF_NOWORLDMODEL | RDF_SKYBOXPORTAL | RDF_AUTOMAP ) ) || cls.state != CA_ACTIVE )
		return;
	if ( cmp.playing ) {
		c = &cmp.cine;
		t = cmp.t;
	} else if ( cm.open ) {
		c = &cm.cine;
		t = cm.tlPinned ? cm.tlMs : -1;
	} else {
		return;
	}
	for ( int i = 0; i < c->numProps; i++ ) {
		const cmProp_t *pr = &c->props[i];
		refEntity_t ent;
		vec3_t org, ang;
		qhandle_t h;

		if ( !pr->model[0] || !CM_PropAt( c, pr, t, org, ang ) || !( h = re->RegisterModel( pr->model ) ) )
			continue;
		memset( &ent, 0, sizeof( ent ) );
		ent.reType = RT_MODEL;
		ent.hModel = h;
		VectorCopy( org, ent.origin );
		VectorCopy( org, ent.oldorigin );
		VectorCopy( org, ent.lightingOrigin );
		AnglesToAxis( ang, ent.axis );
		if ( pr->scale != 1.0f ) {
			for ( int j = 0; j < 3; j++ )
				VectorScale( ent.axis[j], pr->scale, ent.axis[j] );
			ent.nonNormalizedAxes = qtrue;
		}
		ent.shaderRGBA[0] = ent.shaderRGBA[1] = ent.shaderRGBA[2] = ent.shaderRGBA[3] = 255;
		re->AddRefEntityToScene( &ent );
	}
	for ( int i = 0; i < c->numLights; i++ ) {
		const cmLight_t *l = &c->lights[i];
		const int end = CM_LightEnd( c, l );
		float a;
		vec3_t org;

		if ( t >= 0 && ( t < l->start || t >= end ) )
			continue;
		a = t < 0 ? 1.0f : CM_LightNow( l, t - l->start, end - l->start );
		if ( a < 0.01f )
			continue;
		CM_LightOrg( l, org );
		re->AddLightToScene( org, l->radius, l->color[0] * a, l->color[1] * a, l->color[2] * a );
	}
}

/*
===============================================================================

The map's look: its light, the sky, fog; the game's own sounds

===============================================================================
*/

static struct {
	int			frame;
	qboolean	any;				// a look going on
	float		light;
	vec3_t		lightColor, skyColor, fogColor;
	float		fogDepth;
	const char	*layout;			// the map editor layout shown now, NULL none
} cmMapNow = { -1 };

static float CM_Lerp( float a, float b, float w ) {
	return a + ( b - a ) * w;
}

// the map's look now, once a frame: the cinematic playing, or the one being made at the moment
// picked; each look going on blended over the ones before it, as far as it's faded in
static void CM_MapNow( void ) {
	const cmCine_t *c = NULL;
	int t = 0, order[CM_MAX_MAPLOOKS], n = 0;

	if ( cmMapNow.frame == cls.framecount )
		return;
	cmMapNow.frame = cls.framecount;
	cmMapNow.any = qfalse;
	cmMapNow.light = 1.0f;
	VectorSet( cmMapNow.lightColor, 1, 1, 1 );
	VectorSet( cmMapNow.skyColor, 1, 1, 1 );
	VectorSet( cmMapNow.fogColor, 0.5f, 0.5f, 0.5f );
	cmMapNow.fogDepth = CM_FOG_NONE;
	cmMapNow.layout = NULL;
	if ( cmp.playing ) {
		c = &cmp.cine;
		t = cmp.t;
	} else if ( cm.open && cm.tlPinned ) {
		c = &cm.cine;
		t = cm.tlMs;
	}
	if ( !c || cls.state != CA_ACTIVE )
		return;
	for ( int i = 0; i < c->numMapLooks; i++ ) {
		int j = n++;

		for ( ; j > 0 && c->mapLooks[order[j - 1]].start > c->mapLooks[i].start; j-- )
			order[j] = order[j - 1];
		order[j] = i;
	}
	for ( int k = 0; k < n; k++ ) {
		const cmMapLook_t *m = &c->mapLooks[order[k]];
		const int in = t - m->start, len = CM_MapLookEnd( c, m ) - m->start;
		float w = 1.0f;

		if ( in < 0 || in >= len )
			continue;
		if ( m->fade > 0 ) {
			w = Q_min( w, (float)in / m->fade );
			if ( m->ms > 0 )
				w = Q_min( w, (float)( len - in ) / m->fade );
		}
		cmMapNow.any = qtrue;
		if ( m->layout[0] )
			cmMapNow.layout = m->layout;
		cmMapNow.light = CM_Lerp( cmMapNow.light, m->light / 100.0f, w );
		for ( int j = 0; j < 3; j++ ) {
			cmMapNow.lightColor[j] = CM_Lerp( cmMapNow.lightColor[j], m->lightColor[j], w );
			cmMapNow.skyColor[j] = CM_Lerp( cmMapNow.skyColor[j], m->skyColor[j], w );
			if ( m->fogDepth > 0.0f )
				cmMapNow.fogColor[j] = CM_Lerp( cmMapNow.fogColor[j], m->fogColor[j], w );
		}
		cmMapNow.fogDepth = CM_Lerp( cmMapNow.fogDepth, m->fogDepth > 0.0f ? m->fogDepth : CM_FOG_NONE, w );
	}
}

// a light style as the cgame sets it, darker, brighter or coloured as the map's look has it
int CL_Cinematic_LightStyle( int style, int color ) {
	byteAlias_t ba;

	CM_MapNow();
	if ( !cmMapNow.any )
		return color;
	ba.i = color;
	for ( int j = 0; j < 3; j++ )
		ba.b[j] = (byte)Com_Clampi( 0, 255, (int)( ba.b[j] * cmMapNow.light * cmMapNow.lightColor[j] + 0.5f ) );
	return ba.i;
}

// the sky's colour as the map's look has it; qfalse if it's as it is
qboolean CL_Cinematic_SkyTint( vec3_t tint ) {
	CM_MapNow();
	if ( !cmMapNow.any || ( fabsf( cmMapNow.skyColor[0] - 1.0f ) < 0.01f && fabsf( cmMapNow.skyColor[1] - 1.0f ) < 0.01f && fabsf( cmMapNow.skyColor[2] - 1.0f ) < 0.01f ) )
		return qfalse;
	VectorCopy( cmMapNow.skyColor, tint );
	return qtrue;
}

// fog over everything as the map's look has it; qfalse if there's none
qboolean CL_Cinematic_Fog( vec3_t color, float *depth ) {
	CM_MapNow();
	if ( !cmMapNow.any || cmMapNow.fogDepth >= CM_FOG_NONE - 1.0f )
		return qfalse;
	VectorCopy( cmMapNow.fogColor, color );
	*depth = cmMapNow.fogDepth;
	return qtrue;
}

// the game's own sounds quieter while a cinematic plays, s_volume put back after; kept in a cvar
// too, so a crash meanwhile doesn't leave it low
static cvar_t *rpc_cinVolume;

static void CM_WorldVolume( const cmCine_t *c ) {
	if ( !rpc_cinVolume )
		rpc_cinVolume = Cvar_Get( "rpc_cinVolume", "", CVAR_ARCHIVE_ND | CVAR_INTERNAL, "s_volume while a cinematic has it quieter" );
	if ( c && c->worldVolume < 100 ) {
		if ( !rpc_cinVolume->string[0] )
			Cvar_Set( "rpc_cinVolume", Cvar_VariableString( "s_volume" ) );
		Cvar_SetValue( "s_volume", (float)atof( rpc_cinVolume->string ) * c->worldVolume / 100.0f );
	} else if ( rpc_cinVolume->string[0] ) {
		Cvar_Set( "s_volume", rpc_cinVolume->string );
		Cvar_Set( "rpc_cinVolume", "" );
	}
}

// the extras of the cinematic playing, or being made: defined for cl_extras, shown from their
// spawn, if they have one, until they're taken out
static void CM_SyncExtras( void ) {
	const cmCine_t *c = cmp.playing ? &cmp.cine : cm.open ? &cm.cine : NULL;
	const int t = cmp.playing ? cmp.t : cm.tlPinned ? cm.tlMs : -1;
	qboolean keep[CL_MAX_EXTRAS] = { qfalse };

	if ( c && cls.state == CA_ACTIVE ) {
		for ( int e = 0; e < c->numEnts && e < CL_MAX_EXTRAS; e++ ) {
			const cmEnt_t *ent = &c->ents[e];
			const cmClip_t *spawn = NULL;
			vec3_t home;
			float yaw = ent->yaw;

			if ( !CM_IsExtra( ent->who ) )
				continue;
			VectorCopy( ent->home, home );
			for ( int i = 0; i < c->numClips && !spawn; i++ ) {
				if ( c->clips[i].ent == e && c->clips[i].kind == CM_CLIP_SPAWN && c->clips[i].numKeys )
					spawn = &c->clips[i];
			}
			if ( spawn ) {
				VectorCopy( spawn->keys[0].org, home );
				home[2] += CM_LOOK_UP;
				yaw = spawn->keys[0].ang[YAW];
			}
			// standing on the floor under its spot
			{
				trace_t tr;
				vec3_t from, to;

				VectorCopy( home, from );
				VectorCopy( home, to );
				from[2] += 16.0f;
				to[2] -= 1024.0f;
				CM_BoxTrace( &tr, from, to, vec3_origin, vec3_origin, 0, CONTENTS_SOLID | CONTENTS_TERRAIN, qfalse );
				if ( !tr.startsolid && tr.fraction < 1.0f )
					home[2] = tr.endpos[2] + CM_LOOK_UP;
			}
			CL_Extras_Define( e, ent->who, ent->model, ent->saber, ent->color, home, yaw,
				(qboolean)( t < 0 || ( ( !spawn || t >= spawn->start ) && !CM_GoneAt( c, e, t ) ) ) );
			keep[e] = qtrue;
		}
	}
	CL_Extras_Keep( keep );
}

void CM_Frame( void ) {
	CM_SyncExtras();
	// a look's map edits shown while it lasts, what was there back after
	CM_MapNow();
	CL_MapEditor_CineLayout( cmMapNow.layout );
	if ( cm.open ) {
		CM_UndoFrame();
		CM_CamRecFrame();
		if ( cm.pictureFrames > 0 )
			cm.pictureFrames--;
	}
	CM_PickFxFrame();
	CM_PickSndFrame();
	CM_PreviewFx();
	CM_Clashes();
	CM_RecordFrame();
	CM_OutFrame();
	CM_PlayerFrame();
	CM_EditorFrame();
	CM_EditorApply();
}

void CM_Draw( void ) {
	if ( cm.open )
		CM_DrawEditor();
	else if ( cmRecording.phase )
		CM_DrawRecording();
}

// cinematicmanager [name]: the manager; play <name>: on this screen; video <name>: on this screen
// into a video; send <name>: for everyone;
// stop [all]
static void CM_Cinematic_f( void ) {
	const char *arg = Cmd_Argv( 1 );

	if ( !Q_stricmp( arg, "send" ) ) {
		cmCine_t *c = (cmCine_t *)Z_Malloc( sizeof( cmCine_t ), TAG_GENERAL, qtrue );

		if ( !CM_Load( Cmd_Argv( 2 ), c ) )
			Com_Printf( "No cinematic called \"%s\" in %s/\n", Cmd_Argv( 2 ), CM_DIR );
		else
			CM_Send( c );
		Z_Free( c );
		return;
	}
	if ( !Q_stricmp( arg, "video" ) ) {
		cmCine_t *c = (cmCine_t *)Z_Malloc( sizeof( cmCine_t ), TAG_GENERAL, qtrue );

		if ( cls.state != CA_ACTIVE )
			Com_Printf( "Not connected to a server.\n" );
		else if ( !CM_Load( Cmd_Argv( 2 ), c ) )
			Com_Printf( "No cinematic called \"%s\" in %s/\n", Cmd_Argv( 2 ), CM_DIR );
		else if ( !CM_VideoPlay( c, Cmd_Argv( 2 ) ) )
			Com_Printf( "Couldn't record it: already recording, or it has no shots\n" );
		Z_Free( c );
		return;
	}
	if ( !Q_stricmp( arg, "play" ) ) {
		cmCine_t *c = (cmCine_t *)Z_Malloc( sizeof( cmCine_t ), TAG_GENERAL, qtrue );

		if ( cls.state != CA_ACTIVE ) {
			Com_Printf( "Not connected to a server.\n" );
		} else if ( !CM_Load( Cmd_Argv( 2 ), c ) ) {
			Com_Printf( "No cinematic called \"%s\" in %s/\n", Cmd_Argv( 2 ), CM_DIR );
		} else if ( !CM_Play( c, 0, qtrue ) ) {
			Com_Printf( "%s has no shots\n", Cmd_Argv( 2 ) );
		}
		Z_Free( c );
		return;
	}
	if ( !Q_stricmp( arg, "stop" ) ) {
		if ( !Q_stricmp( Cmd_Argv( 2 ), "all" ) )
			CM_SendStop();
		else
			CM_StopPlaying();
		return;
	}
	if ( !Q_stricmp( arg, "help" ) || !Q_stricmp( arg, "?" ) ) {
		Com_Printf( "cinematicmanager [name] : the cinematic manager, to make cinematics in\n"
			"cinematicmanager play <name> : plays one on your screen only\n"
			"cinematicmanager video <name> : plays one on your screen, kept as videos/<name>-<n>.avi (cl_aviFrameRate)\n"
			"cinematicmanager send <name> : plays one for everyone with this client\n"
			"cinematicmanager stop [all] : stops it on your screen, or for everyone\n" );
		return;
	}
	if ( cm.open && !arg[0] )
		CL_RpCinematic_EditorClose();
	else
		CL_RpCinematic_Edit( arg, qfalse );
}

void CM_Init( void ) {
	memset( &cm, 0, sizeof( cm ) );
	memset( &cmp, 0, sizeof( cmp ) );
	CM_Default( &cm.cine );
	cm.sel = -1;
	cm.ent = -1;
	cm.pickNum = -1;
	cm.clip = -1;
	cm.spot = -1;
	cm.camFov = RC_DEFAULT_FOV;
	cm.animLive = qtrue;
	cm.tlCam = qtrue;
	rpc_cinNoPredict = Cvar_Get( "rpc_cinNoPredict", "0", CVAR_ARCHIVE_ND | CVAR_INTERNAL, "" );
	cl_cinRecordKey = Cvar_Get( "cl_cinRecordKey", "F10", CVAR_ARCHIVE_ND, "The key that ends a recording in the cinematic manager" );
	// a cinematic moving you was cut short by a crash: prediction back, and the game's sounds
	CM_HoldSelf( qfalse );
	CM_WorldVolume( NULL );
	Cmd_AddCommand( CM_TOGGLE_CMD, CM_Cinematic_f, "The cinematic manager: make cinematics of camera shots and what players and NPCs do, and play them" );
}

void CM_Shutdown( void ) {
	Cmd_RemoveCommand( CM_TOGGLE_CMD );
}

// leaving the map or the server: nothing plays
void CM_Reset( void ) {
	cmp.playing = qfalse;
	cmRecording.phase = 0;
	cms.count = 0;
	cmr.id[0] = '\0';
	cm.applied = qfalse;
	cm.preview = qfalse;
	cm.planTime = 0;
	CL_NpcWalk_ClearPoses();
	CM_HoldSelf( qfalse );
}
