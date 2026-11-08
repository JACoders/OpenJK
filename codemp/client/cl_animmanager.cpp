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

// cl_animmanager.cpp -- the animation manager (/animationmanager)
//
// Animations: every animation of every skeleton the game has loaded, the base pk3s' and custom
// ones', from the animation.cfg beside each .gla. Picked, it plays on a model beside the list:
// paused, stepped and dragged through frame by frame, sped up or slowed down. Each says where it
// comes from: the base game, or the pk3 that added it or changed it (its animation.cfg against
// the base game's own copy).
//
// Emote chains: one command for several emotes one after the other: /emgreet doing /emsalute,
// then /emyes when its animation is over (or after the seconds given). They're the server's own
// emotes, sent as typed, so everyone sees them.
//
// Custom emotes (experimental): an animation, and bones of it following other animations, or
// turned. The server knows nothing of them: they're drawn on the model by this client. An
// admin's, with experimental on, go to everyone with this client hidden in an rpnotify:
//
//   rpnotify -1 1 cmd:emote/<client number>/<part>/<parts>: <the emote, see AM_Encode>
//   rpnotify -1 1 cmd:emote/<client number>/stop
//
// anyone else's show on their own screen only. An RPMod emote can play under one, for everyone.
//
// One playing is drawn on a copy of the player's model, made each frame from their own and drawn
// in its place: their own model's bones are never touched, so once it's over their own model is
// drawn again, in whatever animation the game has it in by then. Each emote has its player's
// body: of the bodies drawn, the nearest to where they are, one body to one emote.
//
// Both kinds are saved in emotes/, <name>.chain and <name>.emote, and each is a command of its
// own, its name starting with em as the server's emotes do.
//
// A custom emote can have keys: its bones posed at times, gone through one after the other (see
// KEYS below). The first is its bones' own values, so a client without keys shows that pose.
//
// In the custom emote's preview the picked bone has rings to turn it (pitch, yaw, roll) and arrows
// to move it (X, Y, Z), dragged with the mouse. Where each ring and arrow goes is measured, not
// worked out: the bone is nudged each way and where it went read back, so they're right however
// the bone is turned. The rings stay square to each other, where the angles are with the bone
// unturned: the angles' own axes lean into each other as it turns (at a pitch of 90 yaw and roll
// are the same turn), so a ring turns the bone round its axis and the three angles are worked out
// from where that leaves it.

#include "client.h"
#include "ghoul2/G2.h"
#include "qcommon/matcomp.h"
#include "cl_widgets.h"

#define AM_CMD				"animationmanager"
#define AM_DIR				"emotes"
#define AM_CHAIN_EXT		".chain"
#define AM_CUSTOM_EXT		".emote"
#define AM_HUMANOID			"models/players/_humanoid/_humanoid"

#define AM_MAX_SKELS		256		// every .gla found counts, vehicles and props too
#define AM_MAX_ANIMS		4096
#define AM_MAX_SKEL_BONES	256
#define AM_MAX_CHAINS		64
#define AM_MAX_STEPS		24
#define AM_MAX_CUSTOMS		64
#define AM_MAX_CBONES		32
#define AM_MAX_KEYS			10		// a custom emote's poses over time, the first its bones' own
#define AM_MAX_PLAYING		32
#define AM_NAME_LEN			32
#define AM_FIELD_LEN		48
#define AM_NUM_LEN			12
#define AM_STEP_DEFAULT_MS	2000	// a step whose animation isn't known, or loops
#define AM_NEAR				64.0f	// a model drawn this near an entity is its
#define AM_SEND_PART		400		// of an emote, in one rpnotify
#define AM_RECV_LEN			4096
#define AM_SEND_LEN			( AM_RECV_LEN / AM_SEND_PART * AM_SEND_PART )	// the most one takes
#define AM_MOVE_SPEED		40.0f	// moving this fast ends your own emote
#define AM_MOVE_GRACE_MS	500		// not in its first moments
#define AM_HOLD_MS			400		// one that's over holds its last pose this long
#define AM_PREVIEW_HOLD_MS	1200	// the preview holds the end of one that doesn't loop, then starts over

#define AM_ROW_H			13.0f
#define AM_CTRL_H			15.0f
#define AM_TEXT_SCALE		0.8f
#define AM_MESSAGE_MS		5000
#define AM_CONFIRM_MS		3000

#define AM_LEFT_X			16.0f
#define AM_TOP_Y			56.0f
#define AM_MID_X			146.0f
#define AM_MID_W			276.0f
#define AM_PV_X				432.0f
#define AM_PV_Y				56.0f
#define AM_PV_W				192.0f
#define AM_PV_H				282.0f
#define AM_BUTTON_Y			424.0f

typedef enum {
	AM_TAB_ANIMS,
	AM_TAB_CHAINS,
	AM_TAB_CUSTOM,
	AM_NUM_TABS
} amTab_t;

typedef enum {
	AM_K_TEXT,		// a search
	AM_K_NAME,		// an emote's or an animation's name
	AM_K_NUMBER
} amKind_t;

typedef enum {
	AM_O_BASE,			// as the base game has it
	AM_O_ADDED,			// not in the base game's
	AM_O_CHANGED		// in it, with other frames or speed
} amOrigin_t;

typedef struct {
	char		name[48];
	int			first, num, loop;	// loop: -1 it doesn't
	float		fps;
	byte		origin;
	int			baseNum;			// a changed one's, in the base game
	float		baseFps;
} amAnim_t;

typedef enum {
	AM_F_ALL,
	AM_F_BASE,
	AM_F_CUSTOM,
	AM_NUM_FILTERS
} amFilter_t;

typedef struct {
	char		name[MAX_QPATH];
	int			depth;
} amBoneInfo_t;

typedef struct {
	char		gla[MAX_QPATH];		// no extension
	char		label[MAX_QPATH];
	char		source[MAX_QPATH];	// the pk3 its animation.cfg is read from, "gamedir/name.pk3", or a folder
	qboolean	custom;				// not the base game's
	amAnim_t	*anims;
	int			numAnims;
	amBoneInfo_t *bones;
	int			numBones;
	qboolean	bonesRead;
} amSkel_t;

typedef struct {
	char		cmd[AM_FIELD_LEN];
	char		secs[AM_NUM_LEN];	// "" its animation's length
} amStep_t;

typedef struct {
	char		name[AM_NAME_LEN];
	amStep_t	steps[AM_MAX_STEPS];
	int			numSteps;
} amChain_t;

typedef struct {
	char		bone[MAX_QPATH];
	char		anim[48];			// "" the emote's own
	char		twist[3][AM_NUM_LEN];	// pitch, yaw, roll
	char		offset[3][AM_NUM_LEN];	// moved, in the model's space
} amCBone_t;

// the bones posed at a time: each one's pitch, yaw, roll and X, Y, Z, in the order of its bones
typedef struct {
	char		secs[AM_NUM_LEN];	// seconds into it; the first key's 0
	float		v[AM_MAX_CBONES][6];
} amKey_t;

typedef struct {
	char		name[AM_NAME_LEN];
	char		anim[48];
	char		from[AM_NUM_LEN], to[AM_NUM_LEN], speed[AM_NUM_LEN];
	qboolean	loop;
	qboolean	untilMove;			// holds its last pose until you move, else ends with the animation
	char		under[AM_FIELD_LEN];	// an RPMod emote sent with it
	amCBone_t	bones[AM_MAX_CBONES];	// turned and moved as at its first key (in the editor, the picked key)
	int			numBones;
	int			numKeys;			// 0 it holds the one pose; else 2 or more, the bones going from one to the next
	amKey_t		keys[AM_MAX_KEYS];
} amCustom_t;

// one playing on someone
typedef struct {
	qboolean	active;
	qboolean	stopping;			// over: its bones let go the next time it's drawn
	qboolean	own;				// ours, played here: moving ends it, and the end is sent
	qboolean	shared;
	int			num;
	int			start, stopTime;
	amCustom_t	def;
	void		*body;				// their body's ghoul2, as the cgame draws it (last frame's pick)
	CGhoul2Info_v	*copy;			// drawn in its place, playing the emote
	int			copyFrame;			// the frame it was made for
} amPlay_t;

// the bodies drawn this frame, for the emotes to find theirs next frame
#define AM_MAX_BODIES		128
static struct {
	void		*ghoul2;
	vec3_t		origin;
} amBodies[AM_MAX_BODIES];
static int			amNumBodies;
static int			amFrameNum;

static amSkel_t		amSkels[AM_MAX_SKELS];
static int			amNumSkels;
static qboolean		amSkelsListed;

static amChain_t	amChains[AM_MAX_CHAINS];
static int			amNumChains;
static amCustom_t	amCustoms[AM_MAX_CUSTOMS];
static int			amNumCustoms;

static amPlay_t		amPlaying[AM_MAX_PLAYING];

// what's come of each sender's emote so far
static struct {
	char		text[AM_RECV_LEN];
	int			parts, got;
} amRecv[MAX_CLIENTS];

// a chain being done
static struct {
	qboolean	on;
	amChain_t	chain;
	int			step, next, start;
} amRun;

static cvar_t *cl_customEmotes;

static struct {
	qboolean	open;
	int			tab;
	qhandle_t	font;
	float		cursorX, cursorY;
	qboolean	click;
	float		clickX, clickY;
	qboolean	rclick;				// the right button, where the left one was
	float		rclickX, rclickY;
	int			wheel;
	char		*focus;
	int			focusSize;
	amKind_t	focusKind;
	char		message[256];
	int			messageTime;
	int			confirmTime;

	// Animations
	int			skel;
	char		search[AM_FIELD_LEN];
	int			view[AM_MAX_ANIMS];
	int			numView;
	int			animSel;			// in the skeleton's animations, -1 none
	int			filter;
	int			animScroll;
	qboolean	skelList;			// the skeletons listed to pick from, in the animations' place
	int			skelListView[AM_MAX_SKELS];	// the ones with animations
	int			skelListScroll;
	char		speedText[AM_NUM_LEN];

	// the preview
	CGhoul2Info_v *g2;
	char		g2Key[MAX_QPATH * 2];
	const char	*g2Error;
	vec3_t		mins, maxs;
	float		lookZ;
	int			pelvis;
	float		yaw;
	qboolean	dragging;
	float		dragX;
	qboolean	paused;
	float		ms;					// into what it shows
	int			lastTime;
	float		frame;				// the frame shown, for the readout
	int			firstFrame, lastFrame;
	qboolean	scrubbing;
	refdef_t	refdef;				// the last drawn, to mark a bone in
	refEntity_t	ent;

	// Emote chains
	int			chainSel;			// -1 a new one
	amChain_t	chain;
	int			chainScroll;
	int			stepSel, stepScroll;
	char		pickSearch[AM_FIELD_LEN];
	int			pickView[512];
	int			numPick;
	int			pickSel, pickScroll;

	// Custom emotes
	int			customSel;			// -1 a new one
	amCustom_t	custom;
	int			customScroll;
	int			boneSel, boneScroll;	// in the skeleton's bones
	int			keySel;				// the key its bones' fields show
	int			keyStep;			// the preview's < or >: the key before or after, picked next frame

	// the picked bone's rings and arrows in the preview
	struct {
		qboolean	valid;
		vec3_t		pos;
		vec3_t		rotAxis[3];		// each ring's axis in the world: the angle's axis with the bone unturned
		vec3_t		moveDir[3];		// where a unit of each offset moves it
		float		radius;
		int			hot;			// 0-2 a ring, 3-5 an arrow, -1 none
		qboolean	dragging;
		int			part;
		float		startX, startY, startValue;
		float		startAngle, sign;
		float		turned;			// a ring: how far round it's been dragged, in radians
		vec3_t		startTwist;		// a ring: the bone's angles when grabbed
		float		sx, sy;			// an arrow: screen pixels a unit
	} gz;
} am;

static vec4_t amWhite		= { 1.0f, 1.0f, 1.0f, 1.0f };
static vec4_t amPanel		= { 0.0f, 0.0f, 0.0f, 0.7f };
static vec4_t amPanelLight	= { 0.15f, 0.15f, 0.15f, 0.8f };
static vec4_t amPanelFocus	= { 0.1f, 0.2f, 0.3f, 0.9f };
static vec4_t amHighlight	= { 0.2f, 0.45f, 0.8f, 0.8f };
static vec4_t amHover		= { 1.0f, 1.0f, 1.0f, 0.12f };
static vec4_t amHoverBox	= { 0.25f, 0.25f, 0.25f, 0.9f };
static vec4_t amDanger		= { 0.6f, 0.15f, 0.15f, 0.9f };
static vec4_t amBorder		= { 0.5f, 0.5f, 0.5f, 0.8f };
static vec4_t amDim			= { 0.7f, 0.7f, 0.7f, 1.0f };
static vec4_t amAccent		= { 1.0f, 0.8f, 0.3f, 1.0f };
static vec4_t amPreviewBg	= { 0.08f, 0.08f, 0.1f, 0.95f };
static vec4_t amAdded		= { 0.45f, 0.9f, 0.5f, 1.0f };
static vec4_t amChanged		= { 0.45f, 0.8f, 1.0f, 1.0f };

typedef struct cmd_function_s cmd_function_t;
cmd_function_t *Cmd_FindCommand( const char *cmd_name );

static void AM_Message( const char *text ) {
	Q_strncpyz( am.message, text, sizeof( am.message ) );
	am.messageTime = cls.realtime;
}

/*
===============================================================================

SKELETONS AND THEIR ANIMATIONS

===============================================================================
*/

static void AM_StripExt( const char *in, char *out, int size ) {
	Q_strncpyz( out, in, size );
	if ( strlen( out ) > 4 && !Q_stricmp( out + strlen( out ) - 4, ".gla" ) )
		out[strlen( out ) - 4] = '\0';
}

// an animation.cfg's animations, up to max; how many
static int AM_ParseCfg( const char *buf, const char *path, amAnim_t *out, int max ) {
	const char *p = buf;
	int n = 0;

	COM_BeginParseSession( path );
	while ( n < max ) {
		amAnim_t *a = &out[n];
		const char *token = COM_ParseExt( &p, qtrue );

		if ( !token[0] )
			break;
		if ( !isalpha( (unsigned char)token[0] ) ) {
			SkipRestOfLine( &p );
			continue;
		}
		memset( a, 0, sizeof( *a ) );
		Q_strncpyz( a->name, token, sizeof( a->name ) );
		a->first = atoi( COM_ParseExt( &p, qfalse ) );
		a->num = atoi( COM_ParseExt( &p, qfalse ) );
		a->loop = atoi( COM_ParseExt( &p, qfalse ) );
		a->fps = (float)atof( COM_ParseExt( &p, qfalse ) );
		SkipRestOfLine( &p );
		if ( a->num > 0 && a->first >= 0 && a->fps != 0.0f )
			n++;
	}
	return n;
}

static qboolean AM_IsBaseSource( const char *source ) {
	return (qboolean)!Q_stricmpn( source, BASEGAME "/assets", strlen( BASEGAME "/assets" ) );
}

// every copy of the animation.cfg there is: the one in use first, then the base game's
typedef struct {
	const char	*path;
	char		inUse[MAX_QPATH];
	char		base[MAX_QPATH];
} amCopies_t;

static void AM_CfgCopy( const char *name, const char *source, void *ctx ) {
	amCopies_t *c = (amCopies_t *)ctx;

	if ( Q_stricmp( name, c->path ) )
		return;
	if ( !c->inUse[0] )
		Q_strncpyz( c->inUse, source, sizeof( c->inUse ) );
	if ( !c->base[0] && AM_IsBaseSource( source ) )
		Q_strncpyz( c->base, source, sizeof( c->base ) );
}

// where each animation comes from: the base game's, or added or changed by the pk3 in use
static void AM_ReadOrigins( amSkel_t *s, const char *path ) {
	amCopies_t copies;
	char dir[MAX_QPATH], *slash;
	amAnim_t *base;
	void *buf;
	int numBase;

	memset( &copies, 0, sizeof( copies ) );
	copies.path = path;
	Q_strncpyz( dir, path, sizeof( dir ) );
	slash = strrchr( dir, '/' );
	if ( slash )
		*slash = '\0';
	FS_ListFilesRecursiveFrom( dir, "animation.cfg", AM_CfgCopy, &copies );
	Q_strncpyz( s->source, copies.inUse[0] ? copies.inUse : "a folder", sizeof( s->source ) );
	s->custom = (qboolean)!AM_IsBaseSource( s->source );
	for ( int i = 0; i < s->numAnims; i++ )
		s->anims[i].origin = s->custom ? AM_O_ADDED : AM_O_BASE;
	if ( !s->custom || !copies.base[0] || FS_ReadFileFromPak( copies.base, path, &buf ) <= 0 )
		return;

	// against the base game's own
	base = (amAnim_t *)Z_Malloc( AM_MAX_ANIMS * sizeof( amAnim_t ), TAG_GENERAL, qfalse );
	numBase = AM_ParseCfg( (const char *)buf, path, base, AM_MAX_ANIMS );
	Z_Free( buf );
	for ( int i = 0; i < s->numAnims; i++ ) {
		amAnim_t *a = &s->anims[i];

		for ( int j = 0; j < numBase; j++ ) {
			const amAnim_t *b = &base[j];

			if ( Q_stricmp( a->name, b->name ) )
				continue;
			if ( a->first == b->first && a->num == b->num && a->loop == b->loop && a->fps == b->fps ) {
				a->origin = AM_O_BASE;
			} else {
				a->origin = AM_O_CHANGED;
				a->baseNum = b->num;
				a->baseFps = b->fps;
			}
			break;
		}
	}
	Z_Free( base );
}

// the animation.cfg beside a skeleton; qfalse if it has none
static qboolean AM_ReadAnims( amSkel_t *s ) {
	char path[MAX_QPATH], *buf, *slash;
	amAnim_t *tmp;
	int n;

	Q_strncpyz( path, s->gla, sizeof( path ) );
	slash = strrchr( path, '/' );
	if ( !slash )
		return qfalse;
	slash[1] = '\0';
	Q_strcat( path, sizeof( path ), "animation.cfg" );
	if ( FS_ReadFile( path, (void **)&buf ) <= 0 || !buf )
		return qfalse;
	tmp = (amAnim_t *)Z_Malloc( AM_MAX_ANIMS * sizeof( amAnim_t ), TAG_GENERAL, qtrue );
	n = AM_ParseCfg( buf, path, tmp, AM_MAX_ANIMS );
	FS_FreeFile( buf );
	if ( !n ) {
		Z_Free( tmp );
		return qfalse;
	}
	s->anims = (amAnim_t *)Z_Malloc( n * sizeof( amAnim_t ), TAG_GENERAL, qfalse );
	memcpy( s->anims, tmp, n * sizeof( amAnim_t ) );
	s->numAnims = n;
	Z_Free( tmp );
	AM_ReadOrigins( s, path );
	return qtrue;
}

// a skeleton by its .gla, read the first time it's asked for; -1 if it has no animations
static int AM_Skel( const char *glaIn ) {
	char gla[MAX_QPATH];
	amSkel_t *s;

	if ( !glaIn || !glaIn[0] )
		return -1;
	AM_StripExt( glaIn, gla, sizeof( gla ) );
	for ( int i = 0; i < amNumSkels; i++ ) {
		if ( !Q_stricmp( amSkels[i].gla, gla ) )
			return amSkels[i].numAnims ? i : -1;
	}
	if ( amNumSkels >= AM_MAX_SKELS )
		return -1;
	s = &amSkels[amNumSkels];
	memset( s, 0, sizeof( *s ) );
	Q_strncpyz( s->gla, gla, sizeof( s->gla ) );
	Q_strncpyz( s->label, COM_SkipPath( gla ), sizeof( s->label ) );
	{
		char folder[MAX_QPATH], *slash;

		Q_strncpyz( folder, gla, sizeof( folder ) );
		slash = strrchr( folder, '/' );
		if ( slash ) {
			*slash = '\0';
			if ( Q_stricmp( COM_SkipPath( folder ), s->label ) )
				Com_sprintf( s->label, sizeof( s->label ), "%s/%s", COM_SkipPath( folder ), COM_SkipPath( gla ) );
		}
	}
	AM_ReadAnims( s );
	amNumSkels++;
	return s->numAnims ? amNumSkels - 1 : -1;
}

static void AM_ListSkel( const char *name, void *ctx ) {
	AM_Skel( name );
}

// every skeleton in the game's files, the humanoid first
static void AM_ListSkels( void ) {
	if ( amSkelsListed )
		return;
	amSkelsListed = qtrue;
	AM_Skel( AM_HUMANOID );
	FS_ListFilesRecursive( "models", ".gla", AM_ListSkel, NULL );
}

static void AM_GlaFree( void );

static void AM_FreeSkels( void ) {
	AM_GlaFree();
	for ( int i = 0; i < amNumSkels; i++ ) {
		if ( amSkels[i].anims )
			Z_Free( amSkels[i].anims );
		if ( amSkels[i].bones )
			Z_Free( amSkels[i].bones );
	}
	memset( amSkels, 0, sizeof( amSkels ) );
	amNumSkels = 0;
	amSkelsListed = qfalse;
}

static const amAnim_t *AM_FindAnim( int skel, const char *name ) {
	if ( skel < 0 || !name || !name[0] )
		return NULL;
	for ( int i = 0; i < amSkels[skel].numAnims; i++ ) {
		if ( !Q_stricmp( amSkels[skel].anims[i].name, name ) )
			return &amSkels[skel].anims[i];
	}
	return NULL;
}

// the bones of a skeleton, from its .gla, each with how deep it is under the root
static void AM_ReadBones( amSkel_t *s ) {
	const mdxaHeader_t *header;
	const mdxaSkelOffsets_t *offsets;
	int parents[AM_MAX_SKEL_BONES];
	byte *buf;
	int len, numBones;

	if ( s->bonesRead )
		return;
	s->bonesRead = qtrue;
	len = FS_ReadFile( va( "%s.gla", s->gla ), (void **)&buf );
	if ( len <= 0 || !buf )
		return;
	header = (const mdxaHeader_t *)buf;
	numBones = len >= (int)sizeof( *header ) ? LittleLong( header->numBones ) : 0;
	if ( LittleLong( header->ident ) == MDXA_IDENT && numBones > 0 && numBones <= AM_MAX_SKEL_BONES
		&& (int)sizeof( *header ) + numBones * 4 <= len ) {
		s->bones = (amBoneInfo_t *)Z_Malloc( numBones * sizeof( amBoneInfo_t ), TAG_GENERAL, qtrue );
		offsets = (const mdxaSkelOffsets_t *)( buf + sizeof( *header ) );
		for ( int i = 0; i < numBones; i++ ) {
			const int ofs = (int)sizeof( *header ) + LittleLong( offsets->offsets[i] );
			const mdxaSkel_t *bone;

			if ( ofs < 0 || ofs > len - (int)sizeof( mdxaSkel_t ) )
				break;
			bone = (const mdxaSkel_t *)( buf + ofs );
			Q_strncpyz( s->bones[i].name, bone->name, sizeof( s->bones[i].name ) );
			parents[i] = LittleLong( bone->parent );
			s->numBones = i + 1;
		}
		for ( int i = 0; i < s->numBones; i++ ) {
			int d = 0;

			for ( int p = parents[i]; p >= 0 && p < s->numBones && d < 64; p = parents[p] )
				d++;
			s->bones[i].depth = d;
		}
	}
	FS_FreeFile( buf );
}

// a step's or an emote's length: its animation once through, AM_STEP_DEFAULT_MS when it loops
// or isn't known; *known whether it was
static int AM_AnimMs( const char *anim, float speed, qboolean *known ) {
	const amAnim_t *a = AM_FindAnim( AM_Skel( AM_HUMANOID ), anim );

	if ( known )
		*known = qfalse;
	if ( !a || a->loop != -1 )
		return AM_STEP_DEFAULT_MS;
	if ( known )
		*known = qtrue;
	return (int)( a->num * 1000.0f / fabsf( a->fps ) / Q_max( 0.05f, speed ) );
}

/*
===============================================================================

FRAMES ON BONES

===============================================================================
*/

// the frame an animation is on, ms into it: from..to of its frames (to -1 its last), at speed,
// round and round or held at its end; *last the last frame it goes to
static float AM_Frame( const amAnim_t *a, int from, int to, float speed, qboolean loop, float ms, int *last ) {
	const int end = a->num - 1;
	float f, pos;
	int span;

	from = Com_Clampi( 0, end, from );
	to = to < 0 ? end : Com_Clampi( from, end, to );
	span = to - from;
	f = (float)from;
	if ( span > 0 ) {
		pos = Q_max( 0.0f, ms ) / 1000.0f * fabsf( a->fps ) * Q_max( 0.05f, speed );
		pos = loop ? fmodf( pos, (float)span ) : Q_min( pos, (float)span );
		// one whose frames run backwards (a negative fps) plays them backwards already
		f = a->fps < 0.0f ? to - pos : from + pos;
	}
	*last = a->first + to;
	return a->first + f;
}

// how long it takes once through
static float AM_LengthMs( const amAnim_t *a, int from, int to, float speed ) {
	const int end = a->num - 1;

	from = Com_Clampi( 0, end, from );
	to = to < 0 ? end : Com_Clampi( from, end, to );
	return ( to - from ) * 1000.0f / fabsf( a->fps ) / Q_max( 0.05f, speed );
}

// a bone, and the ones under it, on that frame
static void AM_SetBoneFrame( CGhoul2Info_v &g2, const char *bone, float frame, int last ) {
	const int s = (int)floorf( frame ), e = s + 1 <= last ? s + 2 : s + 1;
	const float set = e == s + 2 ? frame : (float)s;

	re->G2API_SetBoneAnim( g2, 0, bone, s, e, BONE_ANIM_OVERRIDE_FREEZE, 0.001f, re->G2API_GetTime( 0 ), set, 0 );
}

static float AM_Num( const char *text, float def ) {
	return text[0] ? (float)atof( text ) : def;
}

static int AM_CustomFrom( const amCustom_t *c ) {
	return (int)AM_Num( c->from, 0 );
}

static int AM_CustomTo( const amCustom_t *c ) {
	return (int)AM_Num( c->to, -1 );
}

static float AM_CustomSpeed( const amCustom_t *c ) {
	return AM_Num( c->speed, 100 ) / 100.0f;
}

static qboolean AM_Zero3( const char ( *v )[AM_NUM_LEN] ) {
	return (qboolean)( AM_Num( v[0], 0 ) == 0.0f && AM_Num( v[1], 0 ) == 0.0f && AM_Num( v[2], 0 ) == 0.0f );
}

// a bone turned (pitch, yaw, roll) and moved; with the cgame's orientations the second angle G2
// takes is the roll and the third the yaw, so they're handed over that way round
static void AM_BoneSet( CGhoul2Info_v &g2, const char *bone, const vec3_t twist, const vec3_t offset ) {
	vec3_t angles;

	VectorSet( angles, twist[0], twist[2], twist[1] );
	if ( offset && VectorLength( offset ) > 0.001f && re->ext.G2API_SetBoneAnglesOffset )
		re->ext.G2API_SetBoneAnglesOffset( g2, 0, bone, angles, offset, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0,
			re->G2API_GetTime( 0 ) );
	else
		re->G2API_SetBoneAngles( g2, 0, bone, angles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, re->G2API_GetTime( 0 ) );
}

// a bone let go: Ghoul2's RemoveBone leaves one alone while it's still turned or animated, so its
// turn (and its animation, when it's ours) are taken off first
static void AM_FreeBone( CGhoul2Info_v &g2, const char *bone, qboolean anim ) {
	if ( !bone || !bone[0] )
		return;
	if ( re->ext.G2API_FreeBone )
		re->ext.G2API_FreeBone( g2, 0, bone, anim );
	else
		re->G2API_RemoveBone( g2, 0, bone );
}

static void AM_BoneValues( const amCBone_t *b, vec3_t twist, vec3_t offset ) {
	for ( int k = 0; k < 3; k++ ) {
		twist[k] = b ? AM_Num( b->twist[k], 0 ) : 0.0f;
		offset[k] = b ? AM_Num( b->offset[k], 0 ) : 0.0f;
	}
}

/*
===============================================================================

KEYS: THE BONES POSED AT TIMES

===============================================================================
*/

// A custom emote with keys goes through their poses: each bone turned and moved as the key before
// has it, eased into the key after's. The first key, at 0 seconds, is its bones' own values, what
// it was before it had keys (and all a client without keys sees of it); the others are numbers in
// the order of its bones. The turns go the short way round, as one turn from the one to the other.

static void AM_AxisAngles( const matrix3_t axis, vec3_t angles );

static float AM_Round( float v, float scale ) {
	return floorf( v * scale + 0.5f ) / scale + 0.0f;
}

static void AM_NumText( char *out, float v ) {
	if ( v == 0.0f )
		out[0] = '\0';
	else
		Com_sprintf( out, AM_NUM_LEN, "%g", v + 0.0f );
}

static float AM_KeySecs( const amCustom_t *c, int k ) {
	return k > 0 ? Q_max( 0.0f, AM_Num( c->keys[k].secs, 0 ) ) : 0.0f;
}

// the last key's time
static float AM_KeyEnd( const amCustom_t *c ) {
	float end = 0.0f;

	for ( int k = 0; k < c->numKeys; k++ )
		end = Q_max( end, AM_KeySecs( c, k ) );
	return end;
}

// its bones' fields into key k, and back out
static void AM_KeyStore( amCustom_t *c, int k ) {
	for ( int i = 0; i < c->numBones; i++ )
		AM_BoneValues( &c->bones[i], c->keys[k].v[i], c->keys[k].v[i] + 3 );
}

static void AM_KeyLoad( amCustom_t *c, int k ) {
	for ( int i = 0; i < c->numBones; i++ ) {
		for ( int j = 0; j < 3; j++ ) {
			AM_NumText( c->bones[i].twist[j], c->keys[k].v[i][j] );
			AM_NumText( c->bones[i].offset[j], c->keys[k].v[i][3 + j] );
		}
	}
}

// a bone turned or moved in any key
static qboolean AM_KeyBoneUsed( const amCustom_t *c, int i ) {
	for ( int k = 0; k < c->numKeys; k++ ) {
		for ( int j = 0; j < 6; j++ ) {
			if ( c->keys[k].v[i][j] != 0.0f )
				return qtrue;
		}
	}
	return qfalse;
}

static void AM_AxisQuat( const matrix3_t m, vec4_t q ) {
	const float tr = m[0][0] + m[1][1] + m[2][2];
	float s;

	if ( tr > 0.0f ) {
		s = 0.5f / sqrtf( tr + 1.0f );
		q[3] = 0.25f / s;
		q[0] = ( m[2][1] - m[1][2] ) * s;
		q[1] = ( m[0][2] - m[2][0] ) * s;
		q[2] = ( m[1][0] - m[0][1] ) * s;
	} else if ( m[0][0] > m[1][1] && m[0][0] > m[2][2] ) {
		s = 2.0f * sqrtf( 1.0f + m[0][0] - m[1][1] - m[2][2] );
		q[3] = ( m[2][1] - m[1][2] ) / s;
		q[0] = 0.25f * s;
		q[1] = ( m[0][1] + m[1][0] ) / s;
		q[2] = ( m[0][2] + m[2][0] ) / s;
	} else if ( m[1][1] > m[2][2] ) {
		s = 2.0f * sqrtf( 1.0f + m[1][1] - m[0][0] - m[2][2] );
		q[3] = ( m[0][2] - m[2][0] ) / s;
		q[0] = ( m[0][1] + m[1][0] ) / s;
		q[1] = 0.25f * s;
		q[2] = ( m[1][2] + m[2][1] ) / s;
	} else {
		s = 2.0f * sqrtf( 1.0f + m[2][2] - m[0][0] - m[1][1] );
		q[3] = ( m[1][0] - m[0][1] ) / s;
		q[0] = ( m[0][2] + m[2][0] ) / s;
		q[1] = ( m[1][2] + m[2][1] ) / s;
		q[2] = 0.25f * s;
	}
}

static void AM_QuatAxis( const vec4_t q, matrix3_t m ) {
	const float x = q[0], y = q[1], z = q[2], w = q[3];

	m[0][0] = 1 - 2 * ( y * y + z * z );	m[0][1] = 2 * ( x * y - z * w );		m[0][2] = 2 * ( x * z + y * w );
	m[1][0] = 2 * ( x * y + z * w );		m[1][1] = 1 - 2 * ( x * x + z * z );	m[1][2] = 2 * ( y * z - x * w );
	m[2][0] = 2 * ( x * z - y * w );		m[2][1] = 2 * ( y * z + x * w );		m[2][2] = 1 - 2 * ( x * x + y * y );
}

// angles f of the way from a to b, the short way round
static void AM_SlerpAngles( const vec3_t a, const vec3_t b, float f, vec3_t out ) {
	matrix3_t m;
	vec4_t qa, qb, q;
	float dot, s0, s1, len;

	AnglesToAxis( a, m );
	AM_AxisQuat( m, qa );
	AnglesToAxis( b, m );
	AM_AxisQuat( m, qb );
	dot = qa[0] * qb[0] + qa[1] * qb[1] + qa[2] * qb[2] + qa[3] * qb[3];
	if ( dot < 0.0f ) {
		for ( int i = 0; i < 4; i++ )
			qb[i] = -qb[i];
		dot = -dot;
	}
	if ( dot > 0.9995f ) {
		s0 = 1.0f - f;
		s1 = f;
	} else {
		const float th = acosf( dot ), sn = sinf( th );

		s0 = sinf( ( 1.0f - f ) * th ) / sn;
		s1 = sinf( f * th ) / sn;
	}
	for ( int i = 0; i < 4; i++ )
		q[i] = qa[i] * s0 + qb[i] * s1;
	len = sqrtf( q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3] );
	for ( int i = 0; i < 4 && len > 1e-6f; i++ )
		q[i] /= len;
	AM_QuatAxis( q, m );
	AM_AxisAngles( m, out );
}

// where the keys have bone i, secs into them
static void AM_KeyValues( const amCustom_t *c, int i, float secs, vec3_t twist, vec3_t offset ) {
	int prev = -1, next = -1;
	float f;

	for ( int k = 0; k < c->numKeys; k++ ) {
		const float t = AM_KeySecs( c, k );

		if ( t <= secs && ( prev < 0 || t >= AM_KeySecs( c, prev ) ) )
			prev = k;
		if ( t > secs && ( next < 0 || t < AM_KeySecs( c, next ) ) )
			next = k;
	}
	if ( prev < 0 ) {
		prev = next;
		next = -1;
	}
	if ( prev < 0 ) {
		VectorClear( twist );
		VectorClear( offset );
		return;
	}
	VectorCopy( c->keys[prev].v[i], twist );
	VectorCopy( c->keys[prev].v[i] + 3, offset );
	if ( next < 0 )
		return;
	f = ( secs - AM_KeySecs( c, prev ) ) / Q_max( 0.001f, AM_KeySecs( c, next ) - AM_KeySecs( c, prev ) );
	f = f * f * ( 3.0f - 2.0f * f );	// eased in and out
	if ( !VectorCompare( c->keys[prev].v[i], c->keys[next].v[i] ) )
		AM_SlerpAngles( c->keys[prev].v[i], c->keys[next].v[i], f, twist );
	for ( int j = 0; j < 3; j++ )
		offset[j] = c->keys[prev].v[i][3 + j] + f * ( c->keys[next].v[i][3 + j] - c->keys[prev].v[i][3 + j] );
}

// the keys' clock ms into it: looping, round again after the last
static float AM_KeyClock( const amCustom_t *c, float ms ) {
	const float end = AM_KeyEnd( c );
	float secs = Q_max( 0.0f, ms ) / 1000.0f;

	if ( c->loop && end > 0.0f && secs > end )
		secs = fmodf( secs, end );
	return secs;
}

// a custom emote ms into it, on a model of that skeleton
static void AM_ApplyCustom( CGhoul2Info_v &g2, int skel, const amCustom_t *c, float ms ) {
	static const char *body[] = { "model_root", "lower_lumbar", "Motion" };
	const amAnim_t *base = AM_FindAnim( skel, c->anim );
	const float speed = AM_CustomSpeed( c );
	int last;

	if ( base ) {
		const float frame = AM_Frame( base, AM_CustomFrom( c ), AM_CustomTo( c ), speed, c->loop, ms, &last );

		for ( size_t i = 0; i < ARRAY_LEN( body ); i++ )
			AM_SetBoneFrame( g2, body[i], frame, last );
	}
	// bones following another animation, then the turned ones
	for ( int i = 0; i < c->numBones; i++ ) {
		const amCBone_t *b = &c->bones[i];
		const amAnim_t *a = AM_FindAnim( skel, b->anim );

		if ( a )
			AM_SetBoneFrame( g2, b->bone, AM_Frame( a, 0, -1, speed, c->loop, ms, &last ), last );
	}
	if ( c->numKeys ) {
		const float secs = AM_KeyClock( c, ms );

		// each set every frame, unturned too, so none keeps the last key's turn
		for ( int i = 0; i < c->numBones; i++ ) {
			vec3_t twist, offset;

			if ( !AM_KeyBoneUsed( c, i ) )
				continue;
			AM_KeyValues( c, i, secs, twist, offset );
			AM_BoneSet( g2, c->bones[i].bone, twist, offset );
		}
		return;
	}
	for ( int i = 0; i < c->numBones; i++ ) {
		const amCBone_t *b = &c->bones[i];
		vec3_t twist, offset;

		if ( AM_Zero3( b->twist ) && AM_Zero3( b->offset ) )
			continue;
		AM_BoneValues( b, twist, offset );
		AM_BoneSet( g2, b->bone, twist, offset );
	}
}

// how long one plays before it's over, 0 never (it loops, or holds until you move)
static float AM_CustomMs( const amCustom_t *c ) {
	const amAnim_t *a = AM_FindAnim( AM_Skel( AM_HUMANOID ), c->anim );

	if ( !a || c->loop || c->untilMove )
		return 0.0f;
	// the animation or the keys, whichever goes on longer
	return Q_max( AM_LengthMs( a, AM_CustomFrom( c ), AM_CustomTo( c ), AM_CustomSpeed( c ) ), AM_KeyEnd( c ) * 1000.0f ) + AM_HOLD_MS;
}

/*
===============================================================================

PLAYING ON SOMEONE

===============================================================================
*/

static amPlay_t *AM_PlayingOn( int num ) {
	for ( int i = 0; i < AM_MAX_PLAYING; i++ ) {
		if ( amPlaying[i].active && !amPlaying[i].stopping && amPlaying[i].num == num )
			return &amPlaying[i];
	}
	return NULL;
}

static void AM_StopOn( int num ) {
	for ( int i = 0; i < AM_MAX_PLAYING; i++ ) {
		amPlay_t *p = &amPlaying[i];

		if ( p->active && !p->stopping && p->num == num ) {
			p->stopping = qtrue;
			p->stopTime = cls.realtime;
		}
	}
}

// a copy is let go between frames only: the renderer reads it until the frame it was drawn in ends
static void AM_FreeCopy( amPlay_t *p ) {
	if ( p->copy )
		re->G2API_CleanGhoul2Models( &p->copy );
	p->copy = NULL;
}

static amPlay_t *AM_StartOn( int num, const amCustom_t *def ) {
	amPlay_t *p = NULL;

	if ( num < 0 || num >= MAX_GENTITIES )
		return NULL;
	AM_StopOn( num );
	for ( int i = 0; i < AM_MAX_PLAYING && !p; i++ ) {
		if ( !amPlaying[i].active )
			p = &amPlaying[i];
	}
	if ( !p )
		return NULL;
	AM_FreeCopy( p );
	memset( p, 0, sizeof( *p ) );
	p->active = qtrue;
	p->num = num;
	p->start = cls.realtime;
	p->def = *def;
	return p;
}

// where someone is now, near enough
static qboolean AM_EntityOrigin( int num, vec3_t out ) {
	if ( num == clc.clientNum ) {
		VectorCopy( cl.snap.ps.origin, out );
		return qtrue;
	}
	for ( int i = 0; i < cl.snap.numEntities; i++ ) {
		const entityState_t *es = &cl.parseEntities[( cl.snap.parseEntitiesNum + i ) & ( MAX_PARSE_ENTITIES - 1 )];

		if ( es->number == num ) {
			VectorCopy( es->pos.trBase, out );
			return qtrue;
		}
	}
	return qfalse;
}

// each frame, before it's drawn: every emote's body found among last frame's, the nearest pairs
// first, one body to one emote; the copies of the frame before let go
static void AM_BodiesFrame( void ) {
	qboolean bodyTaken[AM_MAX_BODIES] = { qfalse }, playDone[AM_MAX_PLAYING] = { qfalse };
	vec3_t org[AM_MAX_PLAYING];
	qboolean known[AM_MAX_PLAYING];

	amFrameNum++;
	for ( int i = 0; i < AM_MAX_PLAYING; i++ ) {
		amPlay_t *p = &amPlaying[i];

		if ( p->copy && ( !p->active || p->stopping || p->copyFrame != amFrameNum - 1 ) )
			AM_FreeCopy( p );
		// over: their own body's drawn again, as it is
		if ( p->stopping )
			p->active = qfalse;
		known[i] = (qboolean)( p->active && !p->stopping && AM_EntityOrigin( p->num, org[i] ) );
		if ( !known[i] )
			p->body = NULL;
	}
	for ( ;; ) {
		float best = AM_NEAR;
		int bp = -1, bb = -1;

		for ( int i = 0; i < AM_MAX_PLAYING; i++ ) {
			if ( !known[i] || playDone[i] )
				continue;
			for ( int b = 0; b < amNumBodies; b++ ) {
				const float d = Distance( org[i], amBodies[b].origin );

				if ( !bodyTaken[b] && d < best ) {
					best = d;
					bp = i;
					bb = b;
				}
			}
		}
		if ( bp < 0 )
			break;
		playDone[bp] = qtrue;
		bodyTaken[bb] = qtrue;
		amPlaying[bp].body = amBodies[bb].ghoul2;
	}
	for ( int i = 0; i < AM_MAX_PLAYING; i++ ) {
		if ( known[i] && !playDone[i] )
			amPlaying[i].body = NULL;
	}
	amNumBodies = 0;
}

// a model on its way to the renderer: a body playing a custom emote is swapped for a copy of
// itself playing it (into *copy); NULL when it's left as it is
const refEntity_t *CL_AnimManager_Entity( const refEntity_t *ent, refEntity_t *copy ) {
	amPlay_t *pick = NULL;
	const char *gla;
	int skel;

	if ( !ent || ent->reType != RT_MODEL || !ent->ghoul2 )
		return NULL;
	CGhoul2Info_v &g2 = *(CGhoul2Info_v *)ent->ghoul2;
	if ( !re->G2API_HaveWeGhoul2Models( g2 ) )
		return NULL;
	gla = re->G2API_GetGLAName( g2, 0 );
	skel = AM_Skel( gla );
	if ( skel < 0 )
		return NULL;
	// a body: noted for next frame's pairing (a weapon or a saber has no such skeleton)
	if ( amNumBodies < AM_MAX_BODIES ) {
		qboolean seen = qfalse;

		for ( int b = 0; b < amNumBodies && !seen; b++ )
			seen = (qboolean)( amBodies[b].ghoul2 == ent->ghoul2 );
		if ( !seen ) {
			amBodies[amNumBodies].ghoul2 = ent->ghoul2;
			VectorCopy( ent->origin, amBodies[amNumBodies].origin );
			amNumBodies++;
		}
	}
	for ( int i = 0; i < AM_MAX_PLAYING && !pick; i++ ) {
		amPlay_t *p = &amPlaying[i];

		if ( p->active && !p->stopping && p->body == ent->ghoul2 )
			pick = p;
	}
	if ( !pick || !AM_FindAnim( skel, pick->def.anim ) )
		return NULL;
	// the copy, made once a frame from the body as the cgame has it now (drawn twice, as with a
	// shell around it, both are the copy)
	if ( !pick->copy || pick->copyFrame != amFrameNum ) {
		AM_FreeCopy( pick );
		re->G2API_DuplicateGhoul2Instance( g2, &pick->copy );
		if ( !pick->copy || !re->G2API_HaveWeGhoul2Models( *pick->copy ) ) {
			AM_FreeCopy( pick );
			return NULL;
		}
		pick->copyFrame = amFrameNum;
		AM_ApplyCustom( *pick->copy, skel, &pick->def, (float)( cls.realtime - pick->start ) );
	}
	*copy = *ent;
	copy->ghoul2 = pick->copy;
	return copy;
}

/*
===============================================================================

SHARING

===============================================================================
*/

static void AM_Clean( const char *in, char *out, int size ) {
	int len = 0;

	for ( ; *in && len < size - 1; in++ ) {
		if ( *in == '|' || *in == ';' || *in == '"' || *in == ':' || *in == '/' || *in == '\n' || (unsigned char)*in < ' ' )
			continue;
		out[len++] = *in == ' ' ? '_' : *in;
	}
	out[len] = '\0';
}

// a <anim> <from> <to> <speed> <loop> <until moved>|b <bone> <anim or -> <pitch> <yaw> <roll> <x> <y> <z>|...
// then each key after the first: |k <seconds> <pitch> <yaw> <roll> [<x> <y> <z>],... a bone at a time,
// in the order of the b's. Keys that don't fit are left off; the number left off is returned
static int AM_Encode( const amCustom_t *c, char *out, int size ) {
	char anim[48], bone[MAX_QPATH], banim[48];

	AM_Clean( c->anim, anim, sizeof( anim ) );
	Com_sprintf( out, size, "a %s %i %i %g %i %i", anim, AM_CustomFrom( c ), AM_CustomTo( c ), AM_Num( c->speed, 100 ),
		c->loop ? 1 : 0, c->untilMove ? 1 : 0 );
	for ( int i = 0; i < c->numBones; i++ ) {
		const amCBone_t *b = &c->bones[i];

		AM_Clean( b->bone, bone, sizeof( bone ) );
		AM_Clean( b->anim, banim, sizeof( banim ) );
		Q_strcat( out, size, va( "|b %s %s %g %g %g %g %g %g", bone, banim[0] ? banim : "-",
			AM_Num( b->twist[0], 0 ), AM_Num( b->twist[1], 0 ), AM_Num( b->twist[2], 0 ),
			AM_Num( b->offset[0], 0 ), AM_Num( b->offset[1], 0 ), AM_Num( b->offset[2], 0 ) ) );
	}
	for ( int k = 1; k < c->numKeys; k++ ) {
		char seg[2048];

		Com_sprintf( seg, sizeof( seg ), "|k %g ", AM_Round( AM_KeySecs( c, k ), 100.0f ) );
		for ( int i = 0; i < c->numBones; i++ ) {
			const float *v = c->keys[k].v[i];

			Q_strcat( seg, sizeof( seg ), va( "%s%g %g %g", i ? "," : "", AM_Round( v[0], 10.0f ), AM_Round( v[1], 10.0f ), AM_Round( v[2], 10.0f ) ) );
			if ( v[3] != 0.0f || v[4] != 0.0f || v[5] != 0.0f )
				Q_strcat( seg, sizeof( seg ), va( " %g %g %g", AM_Round( v[3], 100.0f ), AM_Round( v[4], 100.0f ), AM_Round( v[5], 100.0f ) ) );
		}
		if ( (int)( strlen( out ) + strlen( seg ) ) >= size )
			return c->numKeys - k;
		Q_strcat( out, size, seg );
	}
	return 0;
}

static qboolean AM_Decode( const char *text, amCustom_t *c ) {
	char buf[AM_RECV_LEN], *part, *next;

	memset( c, 0, sizeof( *c ) );
	Q_strncpyz( buf, text, sizeof( buf ) );
	for ( part = buf; part && *part; part = next ) {
		char a[64], b[64];
		float f1, f2, f3, o1 = 0, o2 = 0, o3 = 0;
		int i1, i2, i3, i4;

		next = strchr( part, '|' );
		if ( next )
			*next++ = '\0';
		while ( *part == ' ' )
			part++;
		if ( part[0] == 'a' && sscanf( part, "a %47s %d %d %f %d %d", a, &i1, &i2, &f1, &i3, &i4 ) == 6 ) {
			Q_strncpyz( c->anim, a, sizeof( c->anim ) );
			Com_sprintf( c->from, sizeof( c->from ), "%i", i1 );
			Com_sprintf( c->to, sizeof( c->to ), "%i", i2 );
			Com_sprintf( c->speed, sizeof( c->speed ), "%g", f1 );
			c->loop = (qboolean)( i3 != 0 );
			c->untilMove = (qboolean)( i4 != 0 );
		} else if ( part[0] == 'b' && c->numBones < AM_MAX_CBONES && sscanf( part, "b %63s %63s %f %f %f %f %f %f", a, b, &f1, &f2, &f3, &o1, &o2, &o3 ) >= 5 ) {
			amCBone_t *bone = &c->bones[c->numBones++];

			Com_sprintf( bone->offset[0], AM_NUM_LEN, "%g", o1 );
			Com_sprintf( bone->offset[1], AM_NUM_LEN, "%g", o2 );
			Com_sprintf( bone->offset[2], AM_NUM_LEN, "%g", o3 );

			Q_strncpyz( bone->bone, a, sizeof( bone->bone ) );
			Q_strncpyz( bone->anim, strcmp( b, "-" ) ? b : "", sizeof( bone->anim ) );
			Com_sprintf( bone->twist[0], AM_NUM_LEN, "%g", f1 );
			Com_sprintf( bone->twist[1], AM_NUM_LEN, "%g", f2 );
			Com_sprintf( bone->twist[2], AM_NUM_LEN, "%g", f3 );
		} else if ( part[0] == 'k' && part[1] == ' ' && c->numKeys < AM_MAX_KEYS ) {
			amKey_t *key;
			char *s = part + 2;

			if ( !c->numKeys )
				c->numKeys = 1;
			key = &c->keys[c->numKeys++];
			memset( key, 0, sizeof( *key ) );
			Com_sprintf( key->secs, sizeof( key->secs ), "%g", Q_max( 0.0f, (float)strtod( s, &s ) ) );
			for ( int i = 0; i < AM_MAX_CBONES && s && *s; i++ ) {
				char *end = strchr( s, ',' );
				float *v = key->v[i];

				if ( end )
					*end++ = '\0';
				sscanf( s, "%f %f %f %f %f %f", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5] );
				s = end;
			}
		}
	}
	// the first key is its bones as they are
	if ( c->numKeys )
		AM_KeyStore( c, 0 );
	return (qboolean)( c->anim[0] != '\0' );
}

// ours, to everyone with this client: only an admin's, with experimental on
static qboolean AM_CanShare( void ) {
	return (qboolean)( CL_RpHooks_Experimental() && CL_RpAdmin_IsAdmin() && !com_sv_running->integer );
}

// the number of keys too many to send is returned
static int AM_Share( const amCustom_t *c ) {
	char text[AM_RECV_LEN];
	int start[AM_RECV_LEN / AM_SEND_PART], size[AM_RECV_LEN / AM_SEND_PART];
	int len, parts = 0, left;

	// a little short of the most, as the parts can come out a little short
	left = AM_Encode( c, text, AM_SEND_LEN - 30 );
	len = strlen( text );
	// cut away from spaces: one at the start of a part is lost on the way
	for ( int pos = 0; pos < len && parts < (int)ARRAY_LEN( start ); parts++ ) {
		int end = Q_min( len, pos + AM_SEND_PART );

		while ( end < len && end > pos + 1 && ( text[end] == ' ' || text[end - 1] == ' ' ) )
			end--;
		start[parts] = pos;
		size[parts] = end - pos;
		pos = end;
	}
	for ( int i = 0; i < parts; i++ ) {
		CL_AddReliableCommand( va( "rpnotify -1 1 cmd:emote/%i/%i/%i: %.*s", clc.clientNum, i + 1, parts, size[i], text + start[i] ), qfalse );
	}
	return left;
}

static void AM_ShareStop( void ) {
	CL_AddReliableCommand( va( "rpnotify -1 1 cmd:emote/%i/stop", clc.clientNum ), qfalse );
}

// cmd:emote/<num>/<part>/<parts>: <text>, cmd:emote/<num>/stop
void CL_AnimManager_Hook( char **argv, int argc, const char *message ) {
	amCustom_t def;
	int num, part, parts;

	if ( argc < 2 || !cl_customEmotes || !cl_customEmotes->integer )
		return;
	num = atoi( argv[0] );
	if ( num < 0 || num >= MAX_CLIENTS || num == clc.clientNum )
		return;
	if ( !Q_stricmp( argv[1], "stop" ) ) {
		AM_StopOn( num );
		return;
	}
	if ( argc < 3 )
		return;
	part = atoi( argv[1] );
	parts = atoi( argv[2] );
	if ( part < 1 || parts < 1 || part > parts || parts > AM_RECV_LEN / AM_SEND_PART )
		return;
	if ( part == 1 ) {
		amRecv[num].text[0] = '\0';
		amRecv[num].parts = parts;
		amRecv[num].got = 0;
	} else if ( amRecv[num].parts != parts || amRecv[num].got != part - 1 ) {
		return;
	}
	Q_strcat( amRecv[num].text, sizeof( amRecv[num].text ), message ? message : "" );
	amRecv[num].got = part;
	if ( part == parts && AM_Decode( amRecv[num].text, &def ) )
		AM_StartOn( num, &def );
}

/*
===============================================================================

PLAYING YOUR OWN

===============================================================================
*/

static void AM_StopOwn( qboolean say ) {
	amPlay_t *p = AM_PlayingOn( clc.clientNum );

	if ( !p )
		return;
	if ( p->shared )
		AM_ShareStop();
	AM_StopOn( clc.clientNum );
	if ( say )
		Com_Printf( "Custom emote stopped\n" );
}

static void AM_PlayOwn( const amCustom_t *c, qboolean share ) {
	amPlay_t *p;

	if ( cls.state != CA_ACTIVE )
		return;
	if ( !AM_FindAnim( AM_Skel( AM_HUMANOID ), c->anim ) ) {
		Com_Printf( S_COLOR_YELLOW "%s: its animation %s isn't one the game has\n", c->name[0] ? c->name : "Custom emote", c->anim );
		return;
	}
	AM_StopOwn( qfalse );
	p = AM_StartOn( clc.clientNum, c );
	if ( !p )
		return;
	p->own = qtrue;
	if ( share && AM_CanShare() ) {
		const int left = AM_Share( c );

		if ( left )
			Com_Printf( S_COLOR_YELLOW "%s: its last %i key%s too much to send, the others see it without\n", c->name[0] ? c->name : "Custom emote",
				left, left == 1 ? " is" : "s are" );
		p->shared = qtrue;
	}
	// what everyone else sees under it
	if ( share && c->under[0] ) {
		Cbuf_AddText( c->under );
		Cbuf_AddText( "\n" );
	}
}

// moving, or its time over, ends your own
static void AM_OwnFrame( void ) {
	amPlay_t *p = AM_PlayingOn( clc.clientNum );
	float ms;

	if ( !p || !p->own )
		return;
	ms = AM_CustomMs( &p->def );
	if ( cls.realtime - p->start > AM_MOVE_GRACE_MS
		&& sqrtf( cl.snap.ps.velocity[0] * cl.snap.ps.velocity[0] + cl.snap.ps.velocity[1] * cl.snap.ps.velocity[1] ) > AM_MOVE_SPEED ) {
		AM_StopOwn( qfalse );
	} else if ( ms > 0.0f && cls.realtime - p->start > ms ) {
		AM_StopOwn( qfalse );
	}
}

/*
===============================================================================

CHAINS

===============================================================================
*/

static int AM_FindChain( const char *name ) {
	for ( int i = 0; i < amNumChains; i++ ) {
		if ( !Q_stricmp( amChains[i].name, name ) )
			return i;
	}
	return -1;
}

static int AM_FindCustom( const char *name ) {
	for ( int i = 0; i < amNumCustoms; i++ ) {
		if ( !Q_stricmp( amCustoms[i].name, name ) )
			return i;
	}
	return -1;
}

// a step's time: its seconds, else its emote's animation
static int AM_StepMs( const amStep_t *s, qboolean *known ) {
	const char *anim;
	const int custom = AM_FindCustom( s->cmd );

	if ( s->secs[0] ) {
		if ( known )
			*known = qtrue;
		return (int)( Q_max( 0.0f, (float)atof( s->secs ) ) * 1000.0f );
	}
	if ( custom >= 0 ) {
		const float ms = AM_CustomMs( &amCustoms[custom] );

		if ( known )
			*known = (qboolean)( ms > 0.0f );
		return ms > 0.0f ? (int)ms : AM_STEP_DEFAULT_MS;
	}
	anim = CL_EmotePreview_Anim( s->cmd );
	return AM_AnimMs( anim, 1.0f, known );
}

static void AM_StopChain( const char *why ) {
	if ( !amRun.on )
		return;
	amRun.on = qfalse;
	if ( why )
		Com_Printf( "%s: %s\n", amRun.chain.name, why );
}

static void AM_StartChain( const amChain_t *c ) {
	if ( cls.state != CA_ACTIVE || !c->numSteps )
		return;
	amRun.on = qtrue;
	amRun.chain = *c;
	amRun.step = 0;
	amRun.next = cls.realtime;
	amRun.start = cls.realtime;
}

static void AM_ChainFrame( void ) {
	const amStep_t *s;

	if ( !amRun.on )
		return;
	if ( cls.state != CA_ACTIVE ) {
		amRun.on = qfalse;
		return;
	}
	// walking off ends it, as it does the emotes
	if ( amRun.step > 0 && cls.realtime - amRun.start > AM_MOVE_GRACE_MS
		&& sqrtf( cl.snap.ps.velocity[0] * cl.snap.ps.velocity[0] + cl.snap.ps.velocity[1] * cl.snap.ps.velocity[1] ) > AM_MOVE_SPEED * 3.0f ) {
		AM_StopChain( "stopped, you moved" );
		return;
	}
	if ( cls.realtime < amRun.next )
		return;
	if ( amRun.step >= amRun.chain.numSteps ) {
		amRun.on = qfalse;
		return;
	}
	s = &amRun.chain.steps[amRun.step++];
	if ( s->cmd[0] ) {
		Cbuf_AddText( s->cmd );
		Cbuf_AddText( "\n" );
	}
	amRun.next = cls.realtime + AM_StepMs( s, NULL );
}

/*
===============================================================================

FILES AND COMMANDS

===============================================================================
*/

static qboolean AM_NameChar( int ch ) {
	return (qboolean)( isalnum( ch ) || ch == '_' || ch == '-' );
}

static void AM_EmoteCmd_f( void );

// an emote's name: em and letters, digits, _ and -; NULL if fine, else why not. For one of ours
// being saved again, ourName is its old name
static const char *AM_NameProblem( const char *name, const char *ourName ) {
	if ( Q_stricmpn( name, "em", 2 ) || strlen( name ) < 3 )
		return "an emote's name starts with em, and goes on";
	for ( const char *c = name; *c; c++ ) {
		if ( !AM_NameChar( *c ) )
			return "a name takes letters, digits, _ and -";
	}
	for ( int i = 0; i < CL_EmotePreview_Count(); i++ ) {
		if ( !Q_stricmp( CL_EmotePreview_Name( i ), name ) )
			return "that's one of the server's emotes";
	}
	if ( ( ourName == NULL || Q_stricmp( ourName, name ) ) && ( AM_FindChain( name ) >= 0 || AM_FindCustom( name ) >= 0 ) )
		return "there's an emote by that name already";
	if ( AM_FindChain( name ) < 0 && AM_FindCustom( name ) < 0 && Cmd_FindCommand( name ) )
		return "that's a command already";
	return NULL;
}

static void AM_AddCommand( const char *name ) {
	if ( !Cmd_FindCommand( name ) )
		Cmd_AddCommand( name, AM_EmoteCmd_f, "An emote made in the animation manager" );
}

static void AM_RemoveCommand( const char *name ) {
	if ( AM_FindChain( name ) < 0 && AM_FindCustom( name ) < 0 )
		Cmd_RemoveCommand( name );
}

static void AM_WriteLine( fileHandle_t f, const char *text ) {
	FS_Write( text, strlen( text ), f );
}

static qboolean AM_SaveChain( const amChain_t *c ) {
	const fileHandle_t f = FS_FOpenFileWrite( va( AM_DIR "/%s" AM_CHAIN_EXT, c->name ) );

	if ( !f )
		return qfalse;
	AM_WriteLine( f, "// Animation manager emote chain: a command, and its seconds (\"\" its animation's), a line\n" );
	for ( int i = 0; i < c->numSteps; i++ )
		AM_WriteLine( f, va( "step \"%s\" \"%s\"\n", c->steps[i].cmd, c->steps[i].secs ) );
	FS_FCloseFile( f );
	return qtrue;
}

static qboolean AM_SaveCustom( const amCustom_t *c ) {
	const fileHandle_t f = FS_FOpenFileWrite( va( AM_DIR "/%s" AM_CUSTOM_EXT, c->name ) );

	if ( !f )
		return qfalse;
	AM_WriteLine( f, "// Animation manager custom emote\n" );
	AM_WriteLine( f, va( "anim \"%s\" \"%s\" \"%s\" \"%s\" %i %i\n", c->anim, c->from, c->to, c->speed, c->loop ? 1 : 0, c->untilMove ? 1 : 0 ) );
	if ( c->under[0] )
		AM_WriteLine( f, va( "under \"%s\"\n", c->under ) );
	for ( int i = 0; i < c->numBones; i++ ) {
		const amCBone_t *b = &c->bones[i];

		AM_WriteLine( f, va( "bone \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\"\n", b->bone, b->anim, b->twist[0], b->twist[1], b->twist[2],
			b->offset[0], b->offset[1], b->offset[2] ) );
	}
	// the keys after the first: its seconds, then each bone's pitch, yaw, roll, X, Y, Z
	for ( int k = 1; k < c->numKeys; k++ ) {
		AM_WriteLine( f, va( "key \"%g\"", AM_KeySecs( c, k ) ) );
		for ( int i = 0; i < c->numBones; i++ ) {
			const float *v = c->keys[k].v[i];

			AM_WriteLine( f, va( "  %g %g %g %g %g %g", v[0] + 0.0f, v[1] + 0.0f, v[2] + 0.0f, v[3] + 0.0f, v[4] + 0.0f, v[5] + 0.0f ) );
		}
		AM_WriteLine( f, "\n" );
	}
	FS_FCloseFile( f );
	return qtrue;
}

static void AM_LoadChain( const char *name ) {
	amChain_t *c;
	char *buf;
	const char *p;

	if ( amNumChains >= AM_MAX_CHAINS || FS_ReadFile( va( AM_DIR "/%s" AM_CHAIN_EXT, name ), (void **)&buf ) <= 0 || !buf )
		return;
	c = &amChains[amNumChains];
	memset( c, 0, sizeof( *c ) );
	Q_strncpyz( c->name, name, sizeof( c->name ) );
	p = buf;
	COM_BeginParseSession( name );
	while ( 1 ) {
		const char *token = COM_ParseExt( &p, qtrue );

		if ( !token[0] )
			break;
		if ( !Q_stricmp( token, "step" ) && c->numSteps < AM_MAX_STEPS ) {
			amStep_t *s = &c->steps[c->numSteps++];

			Q_strncpyz( s->cmd, COM_ParseExt( &p, qfalse ), sizeof( s->cmd ) );
			Q_strncpyz( s->secs, COM_ParseExt( &p, qfalse ), sizeof( s->secs ) );
		}
		SkipRestOfLine( &p );
	}
	FS_FreeFile( buf );
	amNumChains++;
	AM_AddCommand( c->name );
}

static void AM_LoadCustom( const char *name ) {
	amCustom_t *c;
	char *buf;
	const char *p;

	if ( amNumCustoms >= AM_MAX_CUSTOMS || FS_ReadFile( va( AM_DIR "/%s" AM_CUSTOM_EXT, name ), (void **)&buf ) <= 0 || !buf )
		return;
	c = &amCustoms[amNumCustoms];
	memset( c, 0, sizeof( *c ) );
	Q_strncpyz( c->name, name, sizeof( c->name ) );
	p = buf;
	COM_BeginParseSession( name );
	while ( 1 ) {
		const char *token = COM_ParseExt( &p, qtrue );
		qboolean endOfLine = qfalse;

		if ( !token[0] )
			break;
		if ( !Q_stricmp( token, "anim" ) ) {
			Q_strncpyz( c->anim, COM_ParseExt( &p, qfalse ), sizeof( c->anim ) );
			Q_strncpyz( c->from, COM_ParseExt( &p, qfalse ), sizeof( c->from ) );
			Q_strncpyz( c->to, COM_ParseExt( &p, qfalse ), sizeof( c->to ) );
			Q_strncpyz( c->speed, COM_ParseExt( &p, qfalse ), sizeof( c->speed ) );
			c->loop = (qboolean)( atoi( COM_ParseExt( &p, qfalse ) ) != 0 );
			c->untilMove = (qboolean)( atoi( COM_ParseExt( &p, qfalse ) ) != 0 );
		} else if ( !Q_stricmp( token, "under" ) ) {
			Q_strncpyz( c->under, COM_ParseExt( &p, qfalse ), sizeof( c->under ) );
		} else if ( !Q_stricmp( token, "bone" ) && c->numBones < AM_MAX_CBONES ) {
			amCBone_t *b = &c->bones[c->numBones++];

			Q_strncpyz( b->bone, COM_ParseExt( &p, qfalse ), sizeof( b->bone ) );
			Q_strncpyz( b->anim, COM_ParseExt( &p, qfalse ), sizeof( b->anim ) );
			for ( int k = 0; k < 3; k++ )
				Q_strncpyz( b->twist[k], COM_ParseExt( &p, qfalse ), sizeof( b->twist[k] ) );
			for ( int k = 0; k < 3; k++ )
				Q_strncpyz( b->offset[k], COM_ParseExt( &p, qfalse ), sizeof( b->offset[k] ) );
		} else if ( !Q_stricmp( token, "key" ) && c->numKeys < AM_MAX_KEYS ) {
			amKey_t *key;

			if ( !c->numKeys )
				c->numKeys = 1;
			key = &c->keys[c->numKeys++];
			Q_strncpyz( key->secs, COM_ParseExt( &p, qfalse ), sizeof( key->secs ) );
			for ( int n = 0; n < AM_MAX_CBONES * 6 && endOfLine == qfalse; n++ ) {
				const char *value = COM_ParseExt( &p, qfalse );

				// the line's end, gone past already
				if ( !value[0] )
					endOfLine = qtrue;
				else
					key->v[n / 6][n % 6] = (float)atof( value );
			}
		}
		if ( !endOfLine )
			SkipRestOfLine( &p );
	}
	FS_FreeFile( buf );
	// the first key is its bones as they are
	if ( c->numKeys )
		AM_KeyStore( c, 0 );
	amNumCustoms++;
	AM_AddCommand( c->name );
}

static int QDECL AM_CompareChains( const void *a, const void *b ) {
	return Q_stricmp( ( (const amChain_t *)a )->name, ( (const amChain_t *)b )->name );
}

static int QDECL AM_CompareCustoms( const void *a, const void *b ) {
	return Q_stricmp( ( (const amCustom_t *)a )->name, ( (const amCustom_t *)b )->name );
}

static void AM_LoadAll( void ) {
	char **list;
	int n;

	amNumChains = amNumCustoms = 0;
	list = FS_ListFiles( AM_DIR, AM_CHAIN_EXT, &n );
	for ( int i = 0; i < n; i++ ) {
		char name[AM_NAME_LEN];

		COM_StripExtension( list[i], name, sizeof( name ) );
		if ( !AM_NameProblem( name, NULL ) )
			AM_LoadChain( name );
	}
	FS_FreeFileList( list );
	list = FS_ListFiles( AM_DIR, AM_CUSTOM_EXT, &n );
	for ( int i = 0; i < n; i++ ) {
		char name[AM_NAME_LEN];

		COM_StripExtension( list[i], name, sizeof( name ) );
		if ( !AM_NameProblem( name, NULL ) )
			AM_LoadCustom( name );
	}
	FS_FreeFileList( list );
	qsort( amChains, amNumChains, sizeof( amChains[0] ), AM_CompareChains );
	qsort( amCustoms, amNumCustoms, sizeof( amCustoms[0] ), AM_CompareCustoms );
}

// /em<name>: a chain done, a custom emote played (again: stopped)
static void AM_EmoteCmd_f( void ) {
	const char *name = Cmd_Argv( 0 );
	const int chain = AM_FindChain( name ), custom = AM_FindCustom( name );

	if ( cls.state != CA_ACTIVE ) {
		Com_Printf( "%s: join a server first\n", name );
		return;
	}
	if ( chain >= 0 ) {
		if ( amRun.on && !Q_stricmp( amRun.chain.name, name ) )
			AM_StopChain( "stopped" );
		else
			AM_StartChain( &amChains[chain] );
		return;
	}
	if ( custom >= 0 ) {
		const amPlay_t *p = AM_PlayingOn( clc.clientNum );

		if ( p && !Q_stricmp( p->def.name, name ) )
			AM_StopOwn( qtrue );
		else
			AM_PlayOwn( &amCustoms[custom], qtrue );
	}
}

/*
===============================================================================

OPENING AND CLOSING

===============================================================================
*/

static void AM_FreePreview( void ) {
	if ( am.g2 )
		re->G2API_CleanGhoul2Models( &am.g2 );
	am.g2 = NULL;
	am.g2Key[0] = '\0';
}

static void AM_Close( void ) {
	if ( !am.open )
		return;
	AM_GlaFree();
	am.open = qfalse;
	am.focus = NULL;
	am.dragging = am.scrubbing = qfalse;
	Key_SetCatcher( Key_GetCatcher() & ~KEYCATCH_ANIMMANAGER );
}

void CL_AnimManager_Close( void ) {
	AM_Close();
}

qboolean CL_AnimManager_Active( void ) {
	return am.open;
}

static void AM_RebuildView( void );
static void AM_SeeRow( int row, int *scroll, float h );
static void AM_PickAnim( int index );
static void AM_NewChain( void );
static void AM_NewCustom( void );

static qboolean AM_Open( void ) {
	if ( cls.state != CA_ACTIVE || !cls.cgameStarted ) {
		Com_Printf( "Animation manager: join a server first\n" );
		return qfalse;
	}
	if ( am.open )
		return qtrue;
	CL_ModelManager_Close();
	CL_NpcManager_Close();
	CL_ShaderManager_Close();
	CL_EffectManager_Close();
	CL_Storyteller_Close();
	CL_RpCinematic_EditorClose();
	CL_MapEditor_Close();
	CL_SoundManager_Close();
	CL_Presets_Close();
	CL_SpecialMoves_Close();
	CL_ChatLog_Close();

	am.font = re->RegisterFont( "arialnb" );
	if ( !am.font )
		am.font = cls.menuFont;
	AM_ListSkels();
	if ( am.skel < 0 || am.skel >= amNumSkels || !amSkels[am.skel].numAnims )
		am.skel = AM_Skel( AM_HUMANOID );
	if ( am.skel < 0 )
		am.skel = 0;
	AM_RebuildView();
	am.open = qtrue;
	am.focus = NULL;
	am.cursorX = SCREEN_WIDTH * 0.5f;
	am.cursorY = SCREEN_HEIGHT * 0.5f;
	am.click = am.rclick = qfalse;
	am.skelList = qfalse;
	am.wheel = 0;
	am.lastTime = cls.realtime;
	Key_SetCatcher( Key_GetCatcher() | KEYCATCH_ANIMMANAGER );
	return qtrue;
}

void CL_AnimManager_f( void ) {
	const char *arg = Cmd_Argv( 1 );

	if ( !arg[0] ) {
		if ( am.open )
			AM_Close();
		else
			AM_Open();
	} else if ( !Q_stricmp( arg, "close" ) ) {
		AM_Close();
	} else if ( !Q_stricmp( arg, "stop" ) ) {
		AM_StopChain( "stopped" );
		AM_StopOwn( qtrue );
	} else if ( !Q_stricmp( arg, "list" ) ) {
		Com_Printf( "Emote chains:\n" );
		for ( int i = 0; i < amNumChains; i++ )
			Com_Printf( "  %s (%i steps)\n", amChains[i].name, amChains[i].numSteps );
		Com_Printf( "Custom emotes:\n" );
		for ( int i = 0; i < amNumCustoms; i++ )
			Com_Printf( "  %s (%s)\n", amCustoms[i].name, amCustoms[i].anim );
	} else if ( !Q_stricmp( arg, "anim" ) || !Q_stricmp( arg, "animations" ) ) {
		// on the Animations tab, that one picked
		if ( AM_Open() ) {
			const int skel = AM_Skel( AM_HUMANOID );
			const amAnim_t *a = AM_FindAnim( skel, Cmd_Argv( 2 ) );

			am.tab = AM_TAB_ANIMS;
			if ( a ) {
				am.skel = skel;
				am.search[0] = '\0';
				AM_RebuildView();
				am.filter = AM_F_ALL;
				AM_RebuildView();
				AM_PickAnim( (int)( a - amSkels[skel].anims ) );
				for ( int row = 0; row < am.numView; row++ ) {
					if ( am.view[row] == am.animSel ) {
						AM_SeeRow( row, &am.animScroll, AM_BUTTON_Y - 6 - AM_TOP_Y - 22 );
						break;
					}
				}
			}
		}
	} else if ( !Q_stricmp( arg, "chains" ) ) {
		if ( AM_Open() ) {
			const int i = AM_FindChain( Cmd_Argv( 2 ) );

			am.tab = AM_TAB_CHAINS;
			if ( i >= 0 ) {
				am.chainSel = i;
				am.chain = amChains[i];
				am.stepSel = am.chain.numSteps ? 0 : -1;
			}
		}
	} else if ( !Q_stricmp( arg, "custom" ) ) {
		if ( AM_Open() ) {
			const int i = AM_FindCustom( Cmd_Argv( 2 ) );

			am.tab = AM_TAB_CUSTOM;
			if ( i >= 0 ) {
				am.customSel = i;
				am.custom = amCustoms[i];
				if ( am.custom.numBones ) {
					const int skel = AM_Skel( AM_HUMANOID );

					AM_ReadBones( &amSkels[skel] );
					for ( int b = 0; b < amSkels[skel].numBones; b++ ) {
						if ( !Q_stricmp( amSkels[skel].bones[b].name, am.custom.bones[0].bone ) )
							am.boneSel = b;
					}
					AM_SeeRow( am.boneSel, &am.boneScroll, 7 * AM_ROW_H + 2 );
				}
			}
		}
	} else if ( !Q_stricmp( arg, "reload" ) ) {
		AM_FreeSkels();
		AM_ListSkels();
		am.skel = AM_Skel( AM_HUMANOID );
		AM_RebuildView();
		// each, where its list is from, and what of it the base game hasn't
		for ( int i = 0; i < amNumSkels; i++ ) {
			const amSkel_t *sk = &amSkels[i];
			int added = 0, changed = 0;

			if ( !sk->numAnims )
				continue;
			for ( int j = 0; j < sk->numAnims; j++ ) {
				added += sk->anims[j].origin == AM_O_ADDED;
				changed += sk->anims[j].origin == AM_O_CHANGED;
			}
			Com_Printf( "  %s: %i animations, from %s%s\n", sk->label, sk->numAnims, sk->source,
				sk->custom ? va( " (%i added, %i changed)", added, changed ) : "" );
		}
	} else {
		Com_Printf( "usage: " AM_CMD "          open or close it (bind a key to it)\n" );
		Com_Printf( "       " AM_CMD " anim <ANIMATION>    open it on that animation\n" );
		Com_Printf( "       " AM_CMD " chains [name]       open it on the emote chains, or that one\n" );
		Com_Printf( "       " AM_CMD " custom [name]       open it on the custom emotes, or that one\n" );
		Com_Printf( "       " AM_CMD " stop     stop the chain and the custom emote playing\n" );
		Com_Printf( "       " AM_CMD " list     the emotes made\n" );
		Com_Printf( "       " AM_CMD " reload   read the animations again\n" );
	}
}

void CL_AnimManager_Init( void ) {
	memset( &am, 0, sizeof( am ) );
	am.skel = -1;
	am.animSel = -1;
	am.chainSel = -1;
	am.customSel = -1;
	am.stepSel = am.pickSel = am.boneSel = -1;
	cl_customEmotes = Cvar_Get( "cl_customEmotes", "1", CVAR_ARCHIVE_ND, "Show the custom emotes admins share" );
	Cmd_AddCommand( AM_CMD, CL_AnimManager_f, "Preview animations and make emotes" );
	AM_LoadAll();
	AM_NewChain();
	AM_NewCustom();
}

// cgame is going away: the models it drew go with it
void CL_AnimManager_Shutdown( void ) {
	AM_Close();
	AM_FreePreview();
	for ( int i = 0; i < AM_MAX_PLAYING; i++ )
		AM_FreeCopy( &amPlaying[i] );
	memset( amPlaying, 0, sizeof( amPlaying ) );
	memset( amRecv, 0, sizeof( amRecv ) );
	amNumBodies = 0;
	amRun.on = qfalse;
	// the next server's files may differ
	AM_FreeSkels();
}

void CL_AnimManager_Frame( void ) {
	AM_BodiesFrame();
	AM_ChainFrame();
	AM_OwnFrame();
}

/*
===============================================================================

INPUT

===============================================================================
*/

static qboolean AM_InRect( float x, float y, float w, float h ) {
	return (qboolean)( am.cursorX >= x && am.cursorX < x + w && am.cursorY >= y && am.cursorY < y + h );
}

static qboolean AM_AllowChar( int ch ) {
	if ( ch < ' ' || ch >= 127 )
		return qfalse;
	if ( am.focusKind == AM_K_NUMBER )
		return (qboolean)( ( ch >= '0' && ch <= '9' ) || ch == '-' || ch == '.' );
	if ( am.focusKind == AM_K_NAME )
		return (qboolean)( AM_NameChar( ch ) || ch == ' ' || ch == '.' );
	return qtrue;
}

static void AM_FieldChanged( void ) {
	if ( am.focus == am.search ) {
		am.animScroll = 0;
		AM_RebuildView();
	}
}

// binds don't run while the manager holds the keys, so honour the user's toggle bind here
static qboolean AM_IsToggleKey( int key ) {
	const char *binding;

	if ( am.focus && key >= A_SPACE && key <= A_TILDE )
		return qfalse;
	binding = Key_GetBinding( key );
	return (qboolean)( VALIDSTRING( binding ) && !Q_stricmp( binding, AM_CMD ) );
}

static void AM_ListKey( int key );

void CL_AnimManager_KeyEvent( int key, qboolean down ) {
	if ( !am.open )
		return;
	if ( !down ) {
		if ( key == A_MOUSE1 )
			am.dragging = am.scrubbing = am.gz.dragging = qfalse;
		return;
	}
	if ( AM_IsToggleKey( key ) ) {
		AM_Close();
		return;
	}
	switch ( key ) {
	case A_MOUSE1:
		am.click = qtrue;
		am.clickX = am.cursorX;
		am.clickY = am.cursorY;
		return;
	case A_MOUSE2:
		am.rclick = qtrue;
		am.rclickX = am.cursorX;
		am.rclickY = am.cursorY;
		return;
	case A_MWHEELUP:
		am.wheel -= 3;
		return;
	case A_MWHEELDOWN:
		am.wheel += 3;
		return;
	default:
		break;
	}
	if ( am.focus ) {
		const int edit = WG_EditKey( am.focus, am.focusSize, key, AM_AllowChar );

		if ( edit & WG_EDIT_CHANGED )
			AM_FieldChanged();
		if ( edit )
			return;
		if ( key == A_ENTER || key == A_KP_ENTER || key == A_TAB ) {
			// the search keeps the arrows for its list
			if ( am.focus != am.search )
				am.focus = NULL;
			return;
		}
	}
	AM_ListKey( key );
}

void CL_AnimManager_CharEvent( int ch ) {
	if ( !am.open || !am.focus )
		return;
	if ( WG_EditChar( am.focus, am.focusSize, ch, AM_AllowChar ) )
		AM_FieldChanged();
}

void CL_AnimManager_MouseEvent( int dx, int dy ) {
	am.cursorX = Com_Clamp( 0, SCREEN_WIDTH, am.cursorX + dx );
	am.cursorY = Com_Clamp( 0, SCREEN_HEIGHT, am.cursorY + dy );
}

void CL_AnimManager_Escape( void ) {
	if ( am.focus )
		am.focus = NULL;
	else if ( am.skelList )
		am.skelList = qfalse;
	else
		AM_Close();
}

/*
===============================================================================

DRAWING HELPERS

===============================================================================
*/

static void AM_Fill( float x, float y, float w, float h, const float *color ) {
	re->SetColor( color );
	re->DrawStretchPic( x, y, w, h, 0, 0, 0, 0, cls.whiteShader );
	re->SetColor( NULL );
}

static void AM_Box( float x, float y, float w, float h, const float *fill ) {
	AM_Fill( x, y, w, h, fill );
	AM_Fill( x, y, w, 1, amBorder );
	AM_Fill( x, y + h - 1, w, 1, amBorder );
	AM_Fill( x, y, 1, h, amBorder );
	AM_Fill( x + w - 1, y, 1, h, amBorder );
}

static void AM_Text( float x, float y, const char *text, const float *color ) {
	re->Font_DrawString( (int)x, (int)y, text, color, am.font | STYLE_DROPSHADOW, -1, AM_TEXT_SCALE );
}

static float AM_TextWidth( const char *text ) {
	return (float)re->Font_StrLenPixels( text, am.font, AM_TEXT_SCALE );
}

static void AM_TextClipped( float x, float y, float w, const char *text, const float *color ) {
	char buf[MAX_STRING_CHARS];
	int len;

	Q_strncpyz( buf, text, sizeof( buf ) );
	len = strlen( buf );
	while ( len > 3 && AM_TextWidth( buf ) > w ) {
		len--;
		buf[len - 3] = '.';
		buf[len - 2] = '.';
		buf[len - 1] = '.';
		buf[len] = '\0';
	}
	AM_Text( x, y, buf, color );
}

// a pk3 by its own name, the folder it's in left off
static const char *AM_PakName( const char *source ) {
	const char *slash = strrchr( source, '/' );

	return slash ? slash + 1 : source;
}

// smaller, cut to fit
static void AM_TextSmall( float x, float y, float w, const char *text, const float *color ) {
	char buf[MAX_QPATH * 2];
	int len;

	Q_strncpyz( buf, text, sizeof( buf ) );
	len = strlen( buf );
	while ( len > 3 && re->Font_StrLenPixels( buf, am.font, AM_TEXT_SCALE * 0.75f ) > w ) {
		len--;
		buf[len - 3] = buf[len - 2] = buf[len - 1] = '.';
		buf[len] = '\0';
	}
	re->Font_DrawString( (int)x, (int)y, buf, color, am.font | STYLE_DROPSHADOW, -1, AM_TEXT_SCALE * 0.75f );
}

static void AM_Label( float x, float y, const char *text ) {
	AM_Text( x, y + 2, text, amDim );
}

static qboolean AM_Clicked( float x, float y, float w, float h ) {
	if ( !am.click || am.clickX < x || am.clickX >= x + w || am.clickY < y || am.clickY >= y + h )
		return qfalse;
	am.click = qfalse;
	return qtrue;
}

static qboolean AM_RightClicked( float x, float y, float w, float h ) {
	if ( !am.rclick || am.rclickX < x || am.rclickX >= x + w || am.rclickY < y || am.rclickY >= y + h )
		return qfalse;
	am.rclick = qfalse;
	return qtrue;
}

static qboolean AM_Button( float x, float y, float w, const char *label, qboolean selected, qboolean enabled ) {
	const float *fill = selected ? amHighlight : ( enabled && AM_InRect( x, y, w, AM_CTRL_H ) ) ? amHoverBox : amPanelLight;

	AM_Box( x, y, w, AM_CTRL_H, fill );
	AM_Text( x + ( w - AM_TextWidth( label ) ) * 0.5f, y + 2, label, enabled ? amWhite : amDim );
	return (qboolean)( AM_Clicked( x, y, w, AM_CTRL_H ) && enabled );
}

// one that has to be clicked twice
static qboolean AM_DangerButton( float x, float y, float w, const char *label, qboolean enabled ) {
	const qboolean armed = (qboolean)( am.confirmTime && cls.realtime - am.confirmTime < AM_CONFIRM_MS );
	const char *text = armed ? "Click again" : label;

	AM_Box( x, y, w, AM_CTRL_H, armed ? amDanger : ( enabled && AM_InRect( x, y, w, AM_CTRL_H ) ) ? amHoverBox : amPanelLight );
	AM_Text( x + ( w - AM_TextWidth( text ) ) * 0.5f, y + 2, text, enabled ? amWhite : amDim );
	if ( !AM_Clicked( x, y, w, AM_CTRL_H ) || !enabled )
		return qfalse;
	if ( armed ) {
		am.confirmTime = 0;
		return qtrue;
	}
	am.confirmTime = cls.realtime;
	return qfalse;
}

// a text box; qtrue when it was typed in this frame is up to the caller to see
static void AM_Field( float x, float y, float w, char *buf, int size, amKind_t kind, const char *placeholder ) {
	const qboolean focused = (qboolean)( am.focus == buf );
	wgStyle_t style;

	style.font = am.font;
	style.scale = AM_TEXT_SCALE;
	style.lineH = AM_ROW_H;
	style.padX = 4.0f;
	style.padY = 2.0f;
	style.fill = amPanelFocus;
	style.border = amBorder;
	style.text = amWhite;
	style.dim = amDim;
	AM_Box( x, y, w, AM_CTRL_H, focused ? amPanelFocus : amPanelLight );
	WG_Field( &style, x, y, w, AM_CTRL_H, buf, focused, placeholder, am.cursorX, am.cursorY );
	if ( AM_Clicked( x, y, w, WG_FieldHeight( &style, w, AM_CTRL_H, buf, focused ) ) ) {
		if ( !focused )
			WG_EditReset();
		am.focus = buf;
		am.focusSize = size;
		am.focusKind = kind;
		WG_FieldClick( buf, am.clickX, am.clickY );
	}
}

// a list of rows; the row clicked, -1 none. Scrolled by the wheel over it and its bar
typedef void ( *amRowFunc_t )( int row, float x, float y, float w, qboolean selected );

static int AM_List( float x, float y, float w, float h, int count, int sel, int *scroll, amRowFunc_t draw ) {
	const int visible = (int)( h / AM_ROW_H );
	int clicked = -1;

	AM_Box( x, y, w, h, amPanelLight );
	if ( am.wheel && AM_InRect( x, y, w, h ) ) {
		*scroll += am.wheel;
		am.wheel = 0;
	}
	*scroll = Com_Clampi( 0, Q_max( 0, count - visible ), *scroll );
	if ( WG_Scrollbar( x + w - WG_SCROLL_W - 1, y + 1, h - 2, count, visible, scroll, am.cursorX, am.cursorY, &am.click, am.clickX, am.clickY ) )
		w -= WG_SCROLL_W;
	for ( int i = 0; i < visible && *scroll + i < count; i++ ) {
		const int row = *scroll + i;
		const float ry = y + 1 + i * AM_ROW_H;

		if ( row == sel )
			AM_Fill( x + 1, ry, w - 2, AM_ROW_H, amHighlight );
		else if ( AM_InRect( x + 1, ry, w - 2, AM_ROW_H ) )
			AM_Fill( x + 1, ry, w - 2, AM_ROW_H, amHover );
		draw( row, x + 4, ry, w - 8, (qboolean)( row == sel ) );
		if ( AM_Clicked( x + 1, ry, w - 2, AM_ROW_H ) )
			clicked = row;
	}
	return clicked;
}

// a row kept in sight
static void AM_SeeRow( int row, int *scroll, float h ) {
	const int visible = (int)( h / AM_ROW_H );

	if ( row < 0 )
		return;
	if ( row < *scroll )
		*scroll = row;
	else if ( row >= *scroll + visible )
		*scroll = row - visible + 1;
}

/*
===============================================================================

THE PREVIEW

===============================================================================
*/

// the model a skeleton's animations are shown on: for the humanoid the player's own, else the
// model.glm beside the .gla
static void AM_PreviewModel( int skel, char *glm, int glmSize, char *skin, int skinSize ) {
	glm[0] = skin[0] = '\0';
	if ( skel < 0 )
		return;
	if ( !Q_stricmp( amSkels[skel].gla, AM_HUMANOID ) ) {
		const int index = cl.gameState.stringOffsets[CS_PLAYERS + cl.snap.ps.clientNum];
		const char *model = index ? Info_ValueForKey( cl.gameState.stringData + index, "model" ) : "";
		char name[MAX_QPATH], *slash;
		const char *skinName = "default";

		Q_strncpyz( name, model[0] ? model : "kyle", sizeof( name ) );
		slash = strchr( name, '/' );
		if ( slash ) {
			*slash = '\0';
			if ( slash[1] )
				skinName = slash + 1;
		}
		Com_sprintf( glm, glmSize, "models/players/%s/model.glm", name );
		if ( strchr( skinName, '|' ) ) {
			Com_sprintf( skin, skinSize, "models/players/%s/|%s", name, skinName );
		} else {
			Com_sprintf( skin, skinSize, "models/players/%s/model_%s.skin", name, skinName );
			if ( FS_ReadFile( skin, NULL ) <= 0 )
				Com_sprintf( skin, skinSize, "models/players/%s/model_default.skin", name );
		}
		return;
	} else {
		char dir[MAX_QPATH], *slash;

		Q_strncpyz( dir, amSkels[skel].gla, sizeof( dir ) );
		slash = strrchr( dir, '/' );
		if ( !slash )
			return;
		*slash = '\0';
		Com_sprintf( glm, glmSize, "%s/model.glm", dir );
		Com_sprintf( skin, skinSize, "%s/model_default.skin", dir );
		if ( FS_ReadFile( skin, NULL ) <= 0 )
			skin[0] = '\0';
	}
}

// (re)built when the skeleton or the player's model changed
static void AM_UpdatePreview( int skel ) {
	char glm[MAX_QPATH], skin[MAX_QPATH], key[sizeof( am.g2Key )];
	qhandle_t hSkin;

	AM_PreviewModel( skel, glm, sizeof( glm ), skin, sizeof( skin ) );
	Com_sprintf( key, sizeof( key ), "%s|%s", glm, skin );
	if ( !Q_stricmp( key, am.g2Key ) )
		return;
	AM_FreePreview();
	Q_strncpyz( am.g2Key, key, sizeof( am.g2Key ) );
	am.g2Error = NULL;
	if ( !glm[0] ) {
		am.g2Error = "No model to show it on";
		return;
	}
	am.g2Error = CL_GlmBounds( glm, am.mins, am.maxs );
	if ( am.g2Error ) {
		am.g2Error = "No model to show it on";
		return;
	}
	hSkin = skin[0] ? re->RegisterSkin( skin ) : 0;
	if ( re->G2API_InitGhoul2Model( &am.g2, glm, 0, hSkin, 0, 0, 0 ) < 0 || !am.g2 ) {
		AM_FreePreview();
		Q_strncpyz( am.g2Key, key, sizeof( am.g2Key ) );
		am.g2Error = "Couldn't load the model";
		return;
	}
	if ( hSkin )
		re->G2API_SetSkin( *am.g2, 0, hSkin, hSkin );
	am.pelvis = re->G2API_AddBolt( *am.g2, 0, "pelvis" );
	am.lookZ = ( am.mins[2] + am.maxs[2] ) * 0.5f;
}

// the bones a custom emote touched let go on the preview, so the next starts clean
static void AM_ResetPreviewBones( int skel ) {
	if ( !am.g2 || skel < 0 )
		return;
	AM_ReadBones( &amSkels[skel] );
	for ( int i = 0; i < amSkels[skel].numBones; i++ )
		AM_FreeBone( *am.g2, amSkels[skel].bones[i].name, qtrue );
}

// a point in the world on the preview's screen, in 640x480; qfalse behind it
static qboolean AM_Project( const vec3_t point, float *x, float *y ) {
	const refdef_t *rd = &am.refdef;
	const float xScale = cls.glconfig.vidWidth / (float)SCREEN_WIDTH, yScale = cls.glconfig.vidHeight / (float)SCREEN_HEIGHT;
	vec3_t d;
	float fwd, right, up;

	VectorSubtract( point, rd->vieworg, d );
	fwd = DotProduct( d, rd->viewaxis[0] );
	if ( fwd < 1.0f )
		return qfalse;
	right = -DotProduct( d, rd->viewaxis[1] );
	up = DotProduct( d, rd->viewaxis[2] );
	*x = ( rd->x + rd->width * 0.5f * ( 1.0f + right / fwd / tanf( DEG2RAD( rd->fov_x * 0.5f ) ) ) ) / xScale;
	*y = ( rd->y + rd->height * 0.5f * ( 1.0f - up / fwd / tanf( DEG2RAD( rd->fov_y * 0.5f ) ) ) ) / yScale;
	return qtrue;
}

static void AM_RenderPreview( float x, float y, float w, float h ) {
	refdef_t *refdef = &am.refdef;
	refEntity_t *ent = &am.ent;
	vec3_t center, forward, angles;
	const float xScale = cls.glconfig.vidWidth / (float)SCREEN_WIDTH, yScale = cls.glconfig.vidHeight / (float)SCREEN_HEIGHT;
	float fovX, fovY, halfH, dist;

	memset( ent, 0, sizeof( *ent ) );
	halfH = ( am.maxs[2] - am.mins[2] ) * 0.5f;
	VectorSet( ent->angles, 0, am.yaw, 0 );

	// the camera follows the pelvis down when it sits, kneels or lies
	if ( am.pelvis >= 0 ) {
		mdxaBone_t bolt;
		vec3_t zero = { 0, 0, 0 };

		if ( re->G2API_GetBoltMatrix( *am.g2, 0, am.pelvis, &bolt, ent->angles, zero, cl.serverTime, NULL, zero ) )
			am.lookZ += ( bolt.matrix[2][3] + halfH * 0.15f - am.lookZ ) * 0.15f;
	}
	VectorSet( center, 0, 0, am.lookZ );

	memset( refdef, 0, sizeof( *refdef ) );
	refdef->x = (int)( x * xScale );
	refdef->y = (int)( y * yScale );
	refdef->width = (int)( w * xScale );
	refdef->height = (int)( h * yScale );
	fovY = 30.0f;
	fovX = RAD2DEG( 2.0f * atanf( tanf( DEG2RAD( fovY * 0.5f ) ) * refdef->width / (float)refdef->height ) );
	refdef->fov_x = fovX;
	refdef->fov_y = fovY;
	dist = Q_max( halfH / tanf( DEG2RAD( fovY * 0.5f ) ), halfH / tanf( DEG2RAD( fovX * 0.5f ) ) ) * 1.1f;
	VectorSet( angles, 10.0f, 180.0f, 0 );
	AngleVectors( angles, forward, NULL, NULL );
	VectorMA( center, -dist, forward, refdef->vieworg );
	AnglesToAxis( angles, refdef->viewaxis );
	refdef->rdflags = RDF_NOWORLDMODEL;
	refdef->time = cl.serverTime;

	ent->reType = RT_MODEL;
	ent->ghoul2 = am.g2;
	ent->renderfx = RF_NOSHADOW;
	AnglesToAxis( ent->angles, ent->axis );
	ent->shaderRGBA[0] = ent->shaderRGBA[1] = ent->shaderRGBA[2] = ent->shaderRGBA[3] = 255;
	ent->radius = halfH * 2.0f;

	re->ClearScene();
	re->AddRefEntityToScene( ent );
	re->RenderScene( refdef );
}

// a bone marked on the preview
static void AM_MarkBone( const char *bone ) {
	mdxaBone_t m;
	vec3_t zero = { 0, 0, 0 }, point;
	float sx, sy;
	int bolt;

	if ( !am.g2 || !bone || !bone[0] )
		return;
	bolt = re->G2API_AddBolt( *am.g2, 0, bone );
	if ( bolt < 0 || !re->G2API_GetBoltMatrix( *am.g2, 0, bolt, &m, am.ent.angles, zero, cl.serverTime, NULL, zero ) )
		return;
	VectorSet( point, m.matrix[0][3], m.matrix[1][3], m.matrix[2][3] );
	if ( !AM_Project( point, &sx, &sy ) )
		return;
	AM_Fill( sx - 3, sy - 3, 7, 7, amBorder );
	AM_Fill( sx - 2, sy - 2, 5, 5, amAccent );
}

/*
===============================================================================

THE PICKED BONE'S RINGS AND ARROWS

===============================================================================
*/

static amCBone_t *AM_CustomBone( amCustom_t *c, const char *bone, qboolean make );

static const vec4_t amGzColors[6] = {
	{ 1.0f, 0.45f, 0.45f, 1.0f },	// pitch
	{ 0.45f, 1.0f, 0.45f, 1.0f },	// yaw
	{ 0.45f, 0.6f, 1.0f, 1.0f },	// roll
	{ 1.0f, 0.25f, 0.25f, 1.0f },	// X
	{ 0.3f, 0.9f, 0.3f, 1.0f },		// Y
	{ 0.3f, 0.45f, 1.0f, 1.0f },	// Z
};
static const vec4_t amGzHot = { 1.0f, 0.9f, 0.2f, 1.0f };
static const char *amGzNames[6] = { "P", "Y", "R", "X", "Y", "Z" };

// the bone where the preview has it, and turned how
static qboolean AM_BoneNow( const char *bone, vec3_t pos, float m[3][3] ) {
	mdxaBone_t b;
	vec3_t zero = { 0, 0, 0 };
	const int bolt = re->G2API_AddBolt( *am.g2, 0, bone );

	if ( bolt < 0 || !re->G2API_GetBoltMatrix( *am.g2, 0, bolt, &b, am.ent.angles, zero, cl.serverTime, NULL, zero ) )
		return qfalse;
	for ( int r = 0; r < 3; r++ ) {
		pos[r] = b.matrix[r][3];
		for ( int c = 0; c < 3; c++ )
			m[r][c] = b.matrix[r][c];
	}
	return qtrue;
}

// each ring's axis and each arrow's way, measured by nudging the bone and seeing where it goes
static void AM_MeasureGizmo( const char *bone, const amCBone_t *cb ) {
	vec3_t twist, offset, pos, t2, o2, p2;
	float m[3][3], m2[3][3];

	am.gz.valid = qfalse;
	if ( !am.g2 || !AM_BoneNow( bone, pos, m ) )
		return;
	AM_BoneValues( cb, twist, offset );
	VectorCopy( pos, am.gz.pos );
	am.gz.radius = ( am.maxs[2] - am.mins[2] ) * 0.09f;
	// the rings from the bone unturned, so they stay square to each other
	VectorClear( t2 );
	AM_BoneSet( *am.g2, bone, t2, offset );
	if ( !AM_BoneNow( bone, p2, m ) )
		return;
	for ( int k = 0; k < 3; k++ ) {
		float r[3][3];
		vec3_t axis;

		VectorClear( t2 );
		t2[k] = 3.0f;
		AM_BoneSet( *am.g2, bone, t2, offset );
		if ( !AM_BoneNow( bone, p2, m2 ) )
			return;
		// the turn between the two, and its axis
		for ( int i = 0; i < 3; i++ )
			for ( int j = 0; j < 3; j++ )
				r[i][j] = m2[i][0] * m[j][0] + m2[i][1] * m[j][1] + m2[i][2] * m[j][2];
		VectorSet( axis, r[2][1] - r[1][2], r[0][2] - r[2][0], r[1][0] - r[0][1] );
		if ( VectorNormalize( axis ) < 1e-5f )
			VectorSet( axis, k == 1, k == 2, k == 0 );
		VectorCopy( axis, am.gz.rotAxis[k] );

		VectorCopy( offset, o2 );
		o2[k] += 2.0f;
		AM_BoneSet( *am.g2, bone, twist, o2 );
		if ( !AM_BoneNow( bone, p2, m2 ) )
			return;
		VectorSubtract( p2, pos, am.gz.moveDir[k] );
		VectorScale( am.gz.moveDir[k], 0.5f, am.gz.moveDir[k] );
	}
	// as it was
	if ( cb )
		AM_BoneSet( *am.g2, bone, twist, offset );
	else
		AM_FreeBone( *am.g2, bone, qfalse );
	am.gz.valid = qtrue;
}

static void AM_GzBasis( const vec3_t axis, vec3_t u, vec3_t v ) {
	vec3_t other;

	VectorSet( other, fabsf( axis[2] ) < 0.9f ? 0 : 1, 0, fabsf( axis[2] ) < 0.9f ? 1 : 0 );
	CrossProduct( axis, other, u );
	VectorNormalize( u );
	CrossProduct( axis, u, v );
}

static void AM_GzLine( float x0, float y0, float x1, float y1, const float *color ) {
	const float len = sqrtf( ( x1 - x0 ) * ( x1 - x0 ) + ( y1 - y0 ) * ( y1 - y0 ) );
	const int steps = Com_Clampi( 1, 400, (int)( len / 0.8f ) );

	re->SetColor( color );
	for ( int i = 0; i <= steps; i++ ) {
		const float f = (float)i / steps;

		re->DrawStretchPic( x0 + ( x1 - x0 ) * f - 0.8f, y0 + ( y1 - y0 ) * f - 0.8f, 1.6f, 1.6f, 0, 0, 0, 0, cls.whiteShader );
	}
	re->SetColor( NULL );
}

static float AM_SegDist( float px, float py, float x0, float y0, float x1, float y1 ) {
	const float dx = x1 - x0, dy = y1 - y0, len2 = dx * dx + dy * dy;
	const float f = len2 > 0.0001f ? Com_Clamp( 0.0f, 1.0f, ( ( px - x0 ) * dx + ( py - y0 ) * dy ) / len2 ) : 0.0f;

	return sqrtf( Square( px - ( x0 + dx * f ) ) + Square( py - ( y0 + dy * f ) ) );
}

#define AM_GZ_SEGS		48
#define AM_GZ_GRAB		6.0f

// a ring's points on the screen; qfalse if it can't be seen
static qboolean AM_GzRing( int k, float *xs, float *ys ) {
	vec3_t u, v, p;

	AM_GzBasis( am.gz.rotAxis[k], u, v );
	for ( int i = 0; i <= AM_GZ_SEGS; i++ ) {
		const float a = i * 2.0f * M_PI / AM_GZ_SEGS;

		VectorMA( am.gz.pos, am.gz.radius * cosf( a ), u, p );
		VectorMA( p, am.gz.radius * sinf( a ), v, p );
		if ( !AM_Project( p, &xs[i], &ys[i] ) )
			return qfalse;
	}
	return qtrue;
}

// an arrow's ends on the screen
static qboolean AM_GzArrow( int k, float *x0, float *y0, float *x1, float *y1 ) {
	vec3_t dir, tip;

	VectorCopy( am.gz.moveDir[k], dir );
	if ( VectorNormalize( dir ) < 1e-4f )
		return qfalse;
	VectorMA( am.gz.pos, am.gz.radius * 1.7f, dir, tip );
	return (qboolean)( AM_Project( am.gz.pos, x0, y0 ) && AM_Project( tip, x1, y1 ) );
}

// what's under the mouse: 0-2 a ring, 3-5 an arrow, -1 nothing
static int AM_GzHit( float mx, float my ) {
	float xs[AM_GZ_SEGS + 1], ys[AM_GZ_SEGS + 1], best = AM_GZ_GRAB;
	int found = -1;

	for ( int k = 0; k < 3; k++ ) {
		float x0, y0, x1, y1, d;

		if ( !AM_GzArrow( k, &x0, &y0, &x1, &y1 ) )
			continue;
		d = AM_SegDist( mx, my, x0 + ( x1 - x0 ) * 0.6f, y0 + ( y1 - y0 ) * 0.6f, x1, y1 );
		if ( d < best ) {
			best = d;
			found = 3 + k;
		}
	}
	if ( found >= 0 )
		return found;
	for ( int k = 0; k < 3; k++ ) {
		if ( !AM_GzRing( k, xs, ys ) )
			continue;
		for ( int i = 0; i < AM_GZ_SEGS; i++ ) {
			const float d = AM_SegDist( mx, my, xs[i], ys[i], xs[i + 1], ys[i + 1] );

			if ( d < best ) {
				best = d;
				found = k;
			}
		}
	}
	return found;
}

static void AM_DrawGizmo( void ) {
	float xs[AM_GZ_SEGS + 1], ys[AM_GZ_SEGS + 1], cx, cy;
	const int lit = am.gz.dragging ? am.gz.part : am.gz.hot;

	if ( !am.gz.valid || !AM_Project( am.gz.pos, &cx, &cy ) )
		return;
	for ( int k = 0; k < 3; k++ ) {
		const float *color = lit == k ? amGzHot : amGzColors[k];

		if ( !AM_GzRing( k, xs, ys ) )
			continue;
		for ( int i = 0; i < AM_GZ_SEGS; i++ )
			AM_GzLine( xs[i], ys[i], xs[i + 1], ys[i + 1], color );
		re->Font_DrawString( (int)xs[AM_GZ_SEGS / 8] + 2, (int)ys[AM_GZ_SEGS / 8] - 5, amGzNames[k], color, am.font | STYLE_DROPSHADOW, -1, 0.6f );
	}
	for ( int k = 0; k < 3; k++ ) {
		const float *color = lit == 3 + k ? amGzHot : amGzColors[3 + k];
		float x0, y0, x1, y1;

		if ( !AM_GzArrow( k, &x0, &y0, &x1, &y1 ) )
			continue;
		AM_GzLine( x0, y0, x1, y1, color );
		AM_Fill( x1 - 2, y1 - 2, 5, 5, color );
		re->Font_DrawString( (int)x1 + 4, (int)y1 - 5, amGzNames[3 + k], color, am.font | STYLE_DROPSHADOW, -1, 0.6f );
	}
}

// the screen angle of the mouse round the bone
static float AM_GzAngle( float mx, float my ) {
	float cx, cy;

	AM_Project( am.gz.pos, &cx, &cy );
	return atan2f( my - cy, mx - cx );
}

// a ring or an arrow grabbed
static void AM_GzBegin( int part, const amCBone_t *cb ) {
	vec3_t twist, offset;

	AM_BoneValues( cb, twist, offset );
	am.gz.dragging = qtrue;
	am.gz.part = part;
	am.gz.startX = am.cursorX;
	am.gz.startY = am.cursorY;
	if ( part < 3 ) {
		vec3_t u, v, q0, q1;
		float x0, y0, x1, y1, cx, cy;

		VectorCopy( twist, am.gz.startTwist );
		am.gz.turned = 0.0f;
		am.gz.startAngle = AM_GzAngle( am.cursorX, am.cursorY );
		// which way round on the screen turns it positive
		AM_GzBasis( am.gz.rotAxis[part], u, v );
		VectorMA( am.gz.pos, am.gz.radius, u, q0 );
		VectorMA( am.gz.pos, am.gz.radius * cosf( 0.2f ), u, q1 );
		VectorMA( q1, am.gz.radius * sinf( 0.2f ), v, q1 );
		am.gz.sign = 1.0f;
		if ( AM_Project( am.gz.pos, &cx, &cy ) && AM_Project( q0, &x0, &y0 ) && AM_Project( q1, &x1, &y1 ) )
			am.gz.sign = ( ( x0 - cx ) * ( y1 - cy ) - ( y0 - cy ) * ( x1 - cx ) ) >= 0.0f ? 1.0f : -1.0f;
	} else {
		const int k = part - 3;
		vec3_t tip;
		float x0, y0, x1, y1;

		am.gz.startValue = offset[k];
		am.gz.sx = am.gz.sy = 0.0f;
		VectorAdd( am.gz.pos, am.gz.moveDir[k], tip );
		if ( AM_Project( am.gz.pos, &x0, &y0 ) && AM_Project( tip, &x1, &y1 ) ) {
			am.gz.sx = x1 - x0;
			am.gz.sy = y1 - y0;
		}
	}
}

// AnglesToAxis the other way round; at a pitch of 90 the roll is left at 0 and the yaw does it all
static void AM_AxisAngles( const matrix3_t axis, vec3_t angles ) {
	const float pitch = asinf( Com_Clamp( -1.0f, 1.0f, -axis[0][2] ) );

	angles[PITCH] = RAD2DEG( pitch );
	if ( cosf( pitch ) > 1e-4f ) {
		angles[YAW] = RAD2DEG( atan2f( axis[0][1], axis[0][0] ) );
		angles[ROLL] = RAD2DEG( atan2f( axis[1][2], axis[2][2] ) );
	} else {
		angles[YAW] = RAD2DEG( atan2f( -axis[1][0], axis[1][1] ) );
		angles[ROLL] = 0.0f;
	}
}

// as the mouse goes: the turn or the move into the bone's fields
static void AM_GzDrag( const char *bone ) {
	amCBone_t *cb = AM_CustomBone( &am.custom, bone, qtrue );
	float value;

	if ( !cb )
		return;
	if ( am.gz.part < 3 ) {
		const float now = AM_GzAngle( am.cursorX, am.cursorY );
		float delta = now - am.gz.startAngle;
		vec3_t turn = { 0, 0, 0 }, angles;
		matrix3_t q, r, out;

		// added up a frame at a time, so it goes on round past half a turn
		while ( delta > M_PI )
			delta -= 2.0f * M_PI;
		while ( delta < -M_PI )
			delta += 2.0f * M_PI;
		am.gz.turned += delta;
		am.gz.startAngle = now;
		turn[am.gz.part] = floorf( am.gz.sign * RAD2DEG( am.gz.turned ) + 0.5f );
		if ( turn[am.gz.part] == 0.0f )
			VectorCopy( am.gz.startTwist, angles );
		else {
			// the turn round the ring's axis, then the bone as it was: the angles' matrices take
			// their axes as columns, so out = r q in these rows
			AnglesToAxis( turn, q );
			AnglesToAxis( am.gz.startTwist, r );
			for ( int j = 0; j < 3; j++ )
				for ( int i = 0; i < 3; i++ )
					out[j][i] = r[j][0] * q[0][i] + r[j][1] * q[1][i] + r[j][2] * q[2][i];
			AM_AxisAngles( out, angles );
		}
		for ( int k = 0; k < 3; k++ ) {
			value = turn[am.gz.part] == 0.0f ? angles[k] : floorf( angles[k] * 10.0f + 0.5f ) / 10.0f + 0.0f;
			while ( value > 180.0f )
				value -= 360.0f;
			while ( value < -180.0f )
				value += 360.0f;
			Com_sprintf( cb->twist[k], AM_NUM_LEN, "%g", value );
		}
	} else {
		const float s2 = am.gz.sx * am.gz.sx + am.gz.sy * am.gz.sy;

		if ( s2 < 1e-6f )
			return;
		value = am.gz.startValue + ( ( am.cursorX - am.gz.startX ) * am.gz.sx + ( am.cursorY - am.gz.startY ) * am.gz.sy ) / s2;
		value = floorf( value * 10.0f + 0.5f ) / 10.0f;
		Com_sprintf( cb->offset[am.gz.part - 3], AM_NUM_LEN, "%g", value );
	}
	am.focus = NULL;
}

// the gizmo on the picked bone: measured, drawn, grabbed and dragged; qtrue if it took the click
static qboolean AM_Gizmo( const char *bone, float x, float y, float w, float h ) {
	amCBone_t *cb = AM_CustomBone( &am.custom, bone, qfalse );

	AM_MeasureGizmo( bone, cb );
	if ( !am.gz.valid ) {
		am.gz.dragging = qfalse;
		return qfalse;
	}
	if ( am.gz.dragging ) {
		if ( !Key_IsDown( A_MOUSE1 ) )
			am.gz.dragging = qfalse;
		else
			AM_GzDrag( bone );
	}
	am.gz.hot = AM_InRect( x, y, w, h ) ? AM_GzHit( am.cursorX, am.cursorY ) : -1;
	AM_DrawGizmo();
	if ( am.click && am.gz.hot >= 0 && am.clickX >= x && am.clickX < x + w && am.clickY >= y && am.clickY < y + h ) {
		am.click = qfalse;
		AM_GzBegin( am.gz.hot, cb );
		return qtrue;
	}
	return qfalse;
}

/*
===============================================================================

A POSE COPIED FROM ANOTHER SKELETON

===============================================================================
*/

// A frame of any skeleton's animation (a mod's .gla on the humanoid's bones, or the humanoid's own)
// copied into a custom emote as turned and moved bones, so it shows for those without that .gla.
// Where each bone is on a frame is worked out from the file, as Ghoul2 does: its parent's, times
// its own for that frame. One turned and moved is its parent's, times its frame, times the turn
// round its unposed place (base * turn * base inverse) with the move added; so for each bone, the
// parent first, the turn and move that take the humanoid's, on the emote's animation, to where
// the other frame has it. Bones with no match in the humanoid are left as they are.

#define AM_POSE_MIN_DEG		0.5f	// turned less than this, and moved less than
#define AM_POSE_MIN_MOVE	0.05f	// this, a bone's left alone

static void AM_TidyBones( amCustom_t *c );
static void AM_KeyPick( int k );
static int AM_KeyTakePose( const amCustom_t *pose );

typedef struct {
	char		gla[MAX_QPATH];
	byte		*buf;
	int			len;
	int			lastUsed;
	int			numBones, numFrames;
	int			ofsFrames, ofsPool;
	int			parent[AM_MAX_SKEL_BONES];
	char		name[AM_MAX_SKEL_BONES][MAX_QPATH];
	mdxaBone_t	base[AM_MAX_SKEL_BONES], baseInv[AM_MAX_SKEL_BONES];
	int			order[AM_MAX_SKEL_BONES];	// parents before their children
} amGla_t;

static amGla_t		amGlas[2];		// the humanoid's and the one copied from, kept while it's open
static int			amGlaUse;
static int			amPoseKey;		// the preview's copied pose made again when it changes

static void AM_GlaFree( void ) {
	for ( size_t i = 0; i < ARRAY_LEN( amGlas ); i++ ) {
		if ( amGlas[i].buf )
			FS_FreeFile( amGlas[i].buf );
		memset( &amGlas[i], 0, sizeof( amGlas[i] ) );
	}
	amPoseKey++;
}

static qboolean AM_GlaRead( amGla_t *g, const char *gla ) {
	const mdxaHeader_t *h;
	const mdxaSkelOffsets_t *offsets;
	int done[AM_MAX_SKEL_BONES], numDone = 0;

	Q_strncpyz( g->gla, gla, sizeof( g->gla ) );
	g->len = FS_ReadFile( va( "%s.gla", gla ), (void **)&g->buf );
	if ( g->len < (int)sizeof( mdxaHeader_t ) || !g->buf )
		return qfalse;
	h = (const mdxaHeader_t *)g->buf;
	g->numBones = LittleLong( h->numBones );
	g->numFrames = LittleLong( h->numFrames );
	g->ofsFrames = LittleLong( h->ofsFrames );
	g->ofsPool = LittleLong( h->ofsCompBonePool );
	if ( LittleLong( h->ident ) != MDXA_IDENT || LittleLong( h->version ) != MDXA_VERSION || g->numBones < 1
		|| g->numBones > AM_MAX_SKEL_BONES || g->numFrames < 1 || (int)sizeof( *h ) + g->numBones * 4 > g->len
		|| g->ofsFrames < 0 || g->ofsFrames + g->numFrames * g->numBones * 3 > g->len || g->ofsPool < 0 || g->ofsPool > g->len )
		return qfalse;
	offsets = (const mdxaSkelOffsets_t *)( g->buf + sizeof( *h ) );
	for ( int i = 0; i < g->numBones; i++ ) {
		const int ofs = (int)sizeof( *h ) + LittleLong( offsets->offsets[i] );
		const mdxaSkel_t *bone;

		if ( ofs < 0 || ofs > g->len - (int)sizeof( mdxaSkel_t ) )
			return qfalse;
		bone = (const mdxaSkel_t *)( g->buf + ofs );
		Q_strncpyz( g->name[i], bone->name, sizeof( g->name[i] ) );
		g->parent[i] = LittleLong( bone->parent );
		if ( g->parent[i] < 0 || g->parent[i] >= g->numBones || g->parent[i] == i )
			g->parent[i] = -1;
		g->base[i] = bone->BasePoseMat;
		g->baseInv[i] = bone->BasePoseMatInv;
		done[i] = 0;
	}
	// parents first, whatever order the file has them in
	while ( numDone < g->numBones ) {
		const int before = numDone;

		for ( int i = 0; i < g->numBones; i++ ) {
			if ( !done[i] && ( g->parent[i] < 0 || done[g->parent[i]] ) ) {
				done[i] = 1;
				g->order[numDone++] = i;
			}
		}
		if ( numDone == before )
			return qfalse;	// round in a circle
	}
	return qtrue;
}

// a .gla read, or the one read already
static const amGla_t *AM_Gla( const char *gla ) {
	amGla_t *g = &amGlas[0];

	for ( size_t i = 0; i < ARRAY_LEN( amGlas ); i++ ) {
		if ( amGlas[i].buf && !Q_stricmp( amGlas[i].gla, gla ) ) {
			amGlas[i].lastUsed = ++amGlaUse;
			return &amGlas[i];
		}
		if ( amGlas[i].lastUsed < g->lastUsed )
			g = &amGlas[i];
	}
	if ( g->buf )
		FS_FreeFile( g->buf );
	memset( g, 0, sizeof( *g ) );
	g->lastUsed = ++amGlaUse;
	if ( !AM_GlaRead( g, gla ) ) {
		if ( g->buf )
			FS_FreeFile( g->buf );
		g->buf = NULL;
		return NULL;
	}
	return g;
}

static void AM_MatIdentity( mdxaBone_t *m ) {
	memset( m, 0, sizeof( *m ) );
	m->matrix[0][0] = m->matrix[1][1] = m->matrix[2][2] = 1.0f;
}

// out = a * b, as Ghoul2 puts them together
static void AM_MatMul( const mdxaBone_t *a, const mdxaBone_t *b, mdxaBone_t *out ) {
	mdxaBone_t t;

	for ( int i = 0; i < 3; i++ ) {
		for ( int j = 0; j < 4; j++ )
			t.matrix[i][j] = a->matrix[i][0] * b->matrix[0][j] + a->matrix[i][1] * b->matrix[1][j] + a->matrix[i][2] * b->matrix[2][j];
		t.matrix[i][3] += a->matrix[i][3];
	}
	*out = t;
}

static void AM_MatInverse( const mdxaBone_t *m, mdxaBone_t *out ) {
	const float ( *a )[4] = m->matrix;
	float det, c[3][3];
	mdxaBone_t t;

	c[0][0] = a[1][1] * a[2][2] - a[1][2] * a[2][1];
	c[0][1] = a[0][2] * a[2][1] - a[0][1] * a[2][2];
	c[0][2] = a[0][1] * a[1][2] - a[0][2] * a[1][1];
	c[1][0] = a[1][2] * a[2][0] - a[1][0] * a[2][2];
	c[1][1] = a[0][0] * a[2][2] - a[0][2] * a[2][0];
	c[1][2] = a[0][2] * a[1][0] - a[0][0] * a[1][2];
	c[2][0] = a[1][0] * a[2][1] - a[1][1] * a[2][0];
	c[2][1] = a[0][1] * a[2][0] - a[0][0] * a[2][1];
	c[2][2] = a[0][0] * a[1][1] - a[0][1] * a[1][0];
	det = a[0][0] * c[0][0] + a[0][1] * c[1][0] + a[0][2] * c[2][0];
	if ( fabsf( det ) < 1e-8f ) {
		AM_MatIdentity( out );
		return;
	}
	for ( int i = 0; i < 3; i++ ) {
		for ( int j = 0; j < 3; j++ )
			t.matrix[i][j] = c[i][j] / det;
		t.matrix[i][3] = -( t.matrix[i][0] * a[0][3] + t.matrix[i][1] * a[1][3] + t.matrix[i][2] * a[2][3] );
	}
	*out = t;
}

// a bone on a frame, against its parent
static qboolean AM_GlaBone( const amGla_t *g, int frame, int bone, mdxaBone_t *out ) {
	const byte *index = g->buf + g->ofsFrames + ( frame * g->numBones + bone ) * 3;
	const int ofs = g->ofsPool + ( index[0] | ( index[1] << 8 ) | ( index[2] << 16 ) ) * (int)sizeof( mdxaCompQuatBone_t );

	if ( ofs < 0 || ofs > g->len - (int)sizeof( mdxaCompQuatBone_t ) )
		return qfalse;
	MC_UnCompressQuat( out->matrix, ( (const mdxaCompQuatBone_t *)( g->buf + ofs ) )->Comp );
	return qtrue;
}

// the turn and move that take a bone, on its frame (local) under its parent where it is (parent),
// to where it's to be; *out the matrix they make, as rounded. How far it's turned, in degrees
static float AM_PoseBone( const amGla_t *g, int bone, const mdxaBone_t *local, const mdxaBone_t *parent,
	const mdxaBone_t *to, vec3_t twist, vec3_t offset, mdxaBone_t *out ) {
	mdxaBone_t inv, o, r;
	matrix3_t axis;
	float cosine;

	// o = local^-1 * parent^-1 * to; and o's turn round the unposed bone, base^-1 * o * base
	AM_MatInverse( parent, &inv );
	AM_MatMul( &inv, to, &o );
	AM_MatInverse( local, &inv );
	AM_MatMul( &inv, &o, &o );
	AM_MatMul( &g->baseInv[bone], &o, &r );
	AM_MatMul( &r, &g->base[bone], &r );
	for ( int i = 0; i < 3; i++ )
		for ( int j = 0; j < 3; j++ )
			axis[j][i] = r.matrix[i][j];
	AM_AxisAngles( axis, twist );
	for ( int k = 0; k < 3; k++ ) {
		twist[k] = floorf( twist[k] * 10.0f + 0.5f ) / 10.0f + 0.0f;
		if ( twist[k] > 180.0f )
			twist[k] -= 360.0f;
		else if ( twist[k] < -180.0f )
			twist[k] += 360.0f;
	}
	// the turn as Ghoul2 will make it, and what's left over of the move
	AnglesToAxis( twist, axis );
	memset( &r, 0, sizeof( r ) );
	for ( int i = 0; i < 3; i++ )
		for ( int j = 0; j < 3; j++ )
			r.matrix[i][j] = axis[j][i];
	AM_MatMul( &g->base[bone], &r, out );
	AM_MatMul( out, &g->baseInv[bone], out );
	for ( int i = 0; i < 3; i++ ) {
		offset[i] = floorf( ( o.matrix[i][3] - out->matrix[i][3] ) * 100.0f + 0.5f ) / 100.0f + 0.0f;
		out->matrix[i][3] += offset[i];
	}
	cosine = ( axis[0][0] + axis[1][1] + axis[2][2] - 1.0f ) * 0.5f;
	return RAD2DEG( acosf( Com_Clamp( -1.0f, 1.0f, cosine ) ) );
}

// c's bones set so that the humanoid, on the first frame of c's animation (BOTH_STAND1 when it
// has none), takes the pose of that frame of anim, on skel; it's then held on that frame. How many
// bones it took, or -1 with *why
static int AM_CopyPose( amCustom_t *c, int skel, const amAnim_t *anim, int frame, const char **why ) {
	static mdxaBone_t srcPose[AM_MAX_SKEL_BONES], local[AM_MAX_SKEL_BONES], to[AM_MAX_SKEL_BONES], got[AM_MAX_SKEL_BONES];
	int map[AM_MAX_SKEL_BONES], matched = 0, numPicked = 0, srcFrame, humFrame;
	float score[AM_MAX_SKEL_BONES];
	qboolean picked[AM_MAX_SKEL_BONES];
	const int humSkel = AM_Skel( AM_HUMANOID );
	const amGla_t *hum, *src;
	const amAnim_t *base;
	mdxaBone_t ident, o;
	vec3_t twist, offset;

	*why = NULL;
	if ( humSkel < 0 || skel < 0 || skel >= amNumSkels || !anim ) {
		*why = "Nothing to copy";
		return -1;
	}
	if ( !c->anim[0] )
		Q_strncpyz( c->anim, "BOTH_STAND1", sizeof( c->anim ) );
	base = AM_FindAnim( humSkel, c->anim );
	if ( !base ) {
		*why = "The emote's animation isn't one of the humanoid's";
		return -1;
	}
	hum = AM_Gla( amSkels[humSkel].gla );
	src = hum && skel != humSkel ? AM_Gla( amSkels[skel].gla ) : hum;
	if ( !hum || !src ) {
		*why = "Couldn't read the skeleton's .gla";
		return -1;
	}
	srcFrame = anim->first + Com_Clampi( 0, Q_max( 0, anim->num - 1 ), frame );
	humFrame = base->first + Com_Clampi( 0, Q_max( 0, base->num - 1 ), AM_CustomFrom( c ) );
	if ( srcFrame < 0 || srcFrame >= src->numFrames || humFrame < 0 || humFrame >= hum->numFrames ) {
		*why = "That frame isn't in the .gla";
		return -1;
	}
	AM_MatIdentity( &ident );

	// the other skeleton on its frame
	for ( int k = 0; k < src->numBones; k++ ) {
		const int b = src->order[k], p = src->parent[b];

		if ( !AM_GlaBone( src, srcFrame, b, &srcPose[b] ) ) {
			*why = "The .gla is damaged";
			return -1;
		}
		if ( p >= 0 )
			AM_MatMul( &srcPose[p], &srcPose[b], &srcPose[b] );
	}
	// the humanoid's bones in it, by name
	for ( int b = 0; b < hum->numBones; b++ ) {
		map[b] = -1;
		for ( int s = 0; s < src->numBones; s++ ) {
			if ( !Q_stricmp( hum->name[b], src->name[s] ) ) {
				map[b] = s;
				matched++;
				break;
			}
		}
		if ( !AM_GlaBone( hum, humFrame, b, &local[b] ) ) {
			*why = "The humanoid's .gla is damaged";
			return -1;
		}
	}
	if ( matched * 2 < hum->numBones ) {
		*why = "Its bones aren't the humanoid's";
		return -1;
	}
	// where each is to be: where the other has it, through each one's unposed place; one with no
	// match goes along with its parent
	for ( int k = 0; k < hum->numBones; k++ ) {
		const int b = hum->order[k], p = hum->parent[b], s = map[b];

		if ( s >= 0 ) {
			AM_MatMul( &srcPose[s], &src->base[s], &to[b] );
			AM_MatMul( &to[b], &hum->baseInv[b], &to[b] );
		} else
			AM_MatMul( p >= 0 ? &to[p] : &ident, &local[b], &to[b] );
	}
	// how far each is from it, its parent there, and the most turned or moved ones picked
	for ( int b = 0; b < hum->numBones; b++ ) {
		const int p = hum->parent[b];

		score[b] = 0.0f;
		picked[b] = qfalse;
		if ( map[b] < 0 )
			continue;
		score[b] = AM_PoseBone( hum, b, &local[b], p >= 0 ? &to[p] : &ident, &to[b], twist, offset, &o );
		if ( score[b] < AM_POSE_MIN_DEG && VectorLength( offset ) < AM_POSE_MIN_MOVE )
			score[b] = 0.0f;
		else
			score[b] += VectorLength( offset ) + 0.001f;
	}
	while ( numPicked < AM_MAX_CBONES ) {
		int best = -1;

		for ( int b = 0; b < hum->numBones; b++ ) {
			if ( !picked[b] && score[b] > 0.0f && ( best < 0 || score[b] > score[best] ) )
				best = b;
		}
		if ( best < 0 )
			break;
		picked[best] = qtrue;
		numPicked++;
	}
	// again, each from where its parent did get to, so what one's rounding or leaving out did is
	// made up below it
	c->numBones = 0;
	for ( int k = 0; k < hum->numBones; k++ ) {
		const int b = hum->order[k], p = hum->parent[b];
		const mdxaBone_t *parent = p >= 0 ? &got[p] : &ident;
		amCBone_t *cb;

		if ( !picked[b] ) {
			AM_MatMul( parent, &local[b], &got[b] );
			continue;
		}
		AM_PoseBone( hum, b, &local[b], parent, &to[b], twist, offset, &o );
		AM_MatMul( parent, &local[b], &got[b] );
		AM_MatMul( &got[b], &o, &got[b] );
		cb = &c->bones[c->numBones++];
		memset( cb, 0, sizeof( *cb ) );
		Q_strncpyz( cb->bone, hum->name[b], sizeof( cb->bone ) );
		for ( int i = 0; i < 3; i++ ) {
			if ( twist[i] != 0.0f )
				Com_sprintf( cb->twist[i], sizeof( cb->twist[i] ), "%g", twist[i] );
			if ( VectorLength( offset ) >= AM_POSE_MIN_MOVE && offset[i] != 0.0f )
				Com_sprintf( cb->offset[i], sizeof( cb->offset[i] ), "%g", offset[i] );
		}
	}
	AM_TidyBones( c );
	// held on the frame it was worked out on
	Com_sprintf( c->to, sizeof( c->to ), "%i", AM_CustomFrom( c ) );
	return c->numBones;
}

// a skeleton with a model of its own to show it on
static qboolean AM_SkelHasModel( int skel ) {
	static char lastGla[MAX_QPATH];
	static qboolean has;

	if ( Q_stricmp( lastGla, amSkels[skel].gla ) ) {
		char glm[MAX_QPATH], skin[MAX_QPATH];

		Q_strncpyz( lastGla, amSkels[skel].gla, sizeof( lastGla ) );
		AM_PreviewModel( skel, glm, sizeof( glm ), skin, sizeof( skin ) );
		has = (qboolean)( glm[0] && FS_ReadFile( glm, NULL ) > 0 );
	}
	return has;
}

// the preview of another skeleton's animation, with no model of its own: the frame shown copied
// onto the humanoid, as Copy pose would make it; NULL if it can't be
static const amCustom_t *AM_PreviewPose( int skel, const amAnim_t *a, int frame ) {
	static amCustom_t pose;
	static int lastKey = -1, lastSkel = -1, lastFrame = -1;
	static const amAnim_t *lastAnim;
	static qboolean ok;

	if ( lastKey != amPoseKey || lastSkel != skel || lastAnim != a || lastFrame != frame ) {
		const char *why;

		lastKey = amPoseKey;
		lastSkel = skel;
		lastAnim = a;
		lastFrame = frame;
		memset( &pose, 0, sizeof( pose ) );
		ok = (qboolean)( AM_CopyPose( &pose, skel, a, frame, &why ) >= 0 );
	}
	return ok ? &pose : NULL;
}

// what the preview plays, set each frame by the tab: one animation, or a custom emote
typedef struct {
	int				skel;
	const amAnim_t	*anim;			// the Animations tab's
	const amCustom_t *custom;
	const char		*markBone;
	const char		*caption;
} amShow_t;

static void AM_DrawPreview( const amShow_t *show ) {
	const float x = AM_PV_X, y = AM_PV_Y, w = AM_PV_W, h = AM_PV_H;
	const amAnim_t *a = show->anim;
	const amCustom_t *posed;
	const qboolean keyed = (qboolean)( show->custom && show->custom->numKeys );
	float lengthMs = 0.0f, speed = 1.0f;
	qboolean loop = qfalse;
	int last = 0;

	AM_Box( x, y, w, h, amPreviewBg );
	if ( show->custom ) {
		a = AM_FindAnim( show->skel, show->custom->anim );
		speed = AM_CustomSpeed( show->custom );
		loop = show->custom->loop;
		if ( a )
			lengthMs = AM_LengthMs( a, AM_CustomFrom( show->custom ), AM_CustomTo( show->custom ), speed );
		if ( keyed )
			lengthMs = Q_max( lengthMs, AM_KeyEnd( show->custom ) * 1000.0f );
	} else if ( a ) {
		speed = AM_Num( am.speedText, 100 ) / 100.0f;
		loop = (qboolean)( a->loop != -1 );
		lengthMs = AM_LengthMs( a, 0, -1, speed );
	}

	// the clock: paused, scrubbed, or going, and round again at the end
	if ( !am.paused && !am.scrubbing )
		am.ms += (float)( cls.realtime - am.lastTime );
	am.lastTime = cls.realtime;
	if ( !am.paused && !am.scrubbing && lengthMs > 0.0f ) {
		if ( loop )
			am.ms = fmodf( am.ms, lengthMs );
		else if ( am.ms > lengthMs + AM_PREVIEW_HOLD_MS )
			am.ms = 0.0f;
	}
	// with keys, paused it's on the picked one, to pose it there
	if ( keyed && am.paused && !am.scrubbing )
		am.ms = AM_KeySecs( show->custom, am.keySel ) * 1000.0f;

	// another skeleton's animation with no model of its own: the frame shown copied onto the humanoid
	posed = NULL;
	if ( !show->custom && a ) {
		am.frame = AM_Frame( a, 0, -1, speed, loop, Q_min( am.ms, lengthMs ), &last );
		if ( show->skel != AM_Skel( AM_HUMANOID ) && !AM_SkelHasModel( show->skel ) )
			posed = AM_PreviewPose( show->skel, a, (int)am.frame - a->first );
	}

	AM_UpdatePreview( posed ? AM_Skel( AM_HUMANOID ) : show->skel );
	if ( !am.g2 || !a ) {
		AM_TextClipped( x + 8, y + h * 0.5f - 6, w - 16, am.g2Error ? am.g2Error : show->custom ? "Pick its animation" : "Pick an animation", amDim );
	} else {
		if ( posed ) {
			AM_ResetPreviewBones( AM_Skel( AM_HUMANOID ) );
			AM_ApplyCustom( *am.g2, AM_Skel( AM_HUMANOID ), posed, 0.0f );
		} else if ( show->custom ) {
			AM_ResetPreviewBones( show->skel );
			AM_ApplyCustom( *am.g2, show->skel, show->custom, Q_min( am.ms, lengthMs ) );
			am.frame = AM_Frame( a, AM_CustomFrom( show->custom ), AM_CustomTo( show->custom ), speed, loop, Q_min( am.ms, lengthMs ), &last );
		} else {
			static const char *body[] = { "model_root", "lower_lumbar", "Motion" };

			AM_ResetPreviewBones( show->skel );
			am.frame = AM_Frame( a, 0, -1, speed, loop, Q_min( am.ms, lengthMs ), &last );
			for ( size_t i = 0; i < ARRAY_LEN( body ); i++ )
				AM_SetBoneFrame( *am.g2, body[i], am.frame, last );
		}
		am.firstFrame = a->first;
		am.lastFrame = a->first + a->num - 1;
		AM_RenderPreview( x + 1, y + 1, w - 2, h - 2 );
		// with keys, posed only while held on one
		if ( show->custom && show->markBone && am.tab == AM_TAB_CUSTOM && ( !keyed || ( am.paused && !am.scrubbing ) ) )
			AM_Gizmo( show->markBone, x, y, w, h );
		else
			AM_MarkBone( show->markBone );
	}

	// dragged sideways in the box, it turns
	if ( am.click && AM_InRect( x, y, w, h ) && am.clickX >= x && am.clickY >= y && am.clickX < x + w && am.clickY < y + h ) {
		am.click = qfalse;
		am.dragging = qtrue;
		am.dragX = am.cursorX;
	}
	if ( am.dragging ) {
		am.yaw += ( am.cursorX - am.dragX ) * 1.5f;
		am.dragX = am.cursorX;
	}
	if ( show->caption )
		AM_TextClipped( x + 4, y + 3, w - 8, show->caption, amDim );
	if ( posed && am.g2 )
		AM_TextClipped( x + 4, y + h - 14, w - 8, "Copied onto your model", amDim );

	// under it: play or pause, a frame back or on, the frame dragged to
	{
		const float cy = y + h + 6;
		const float barY = cy + 20;
		const float span = lengthMs > 0.0f ? lengthMs : 1.0f;

		if ( AM_Button( x, cy, 50, am.paused ? "Play" : "Pause", qfalse, (qboolean)( a != NULL ) ) )
			am.paused = (qboolean)!am.paused;
		if ( keyed ) {
			// a key back or on, the frames being the keys'
			if ( AM_Button( x + 54, cy, 22, "<", qfalse, (qboolean)( am.keySel > 0 ) ) )
				am.keyStep = -1;
			if ( AM_Button( x + 78, cy, 22, ">", qfalse, (qboolean)( am.keySel < show->custom->numKeys - 1 ) ) )
				am.keyStep = 1;
			AM_TextClipped( x + 106, cy + 2, w - 106, va( "%.1fs, key %i of %i", am.ms / 1000.0f, am.keySel + 1, show->custom->numKeys ), amDim );
		} else {
			if ( AM_Button( x + 54, cy, 22, "<", qfalse, (qboolean)( a != NULL ) ) && a ) {
				am.paused = qtrue;
				am.ms = Q_max( 0.0f, am.ms - 1000.0f / fabsf( a->fps ) / Q_max( 0.05f, speed ) );
			}
			if ( AM_Button( x + 78, cy, 22, ">", qfalse, (qboolean)( a != NULL ) ) && a ) {
				am.paused = qtrue;
				am.ms = Q_min( lengthMs, am.ms + 1000.0f / fabsf( a->fps ) / Q_max( 0.05f, speed ) );
			}
			if ( a )
				AM_TextClipped( x + 106, cy + 2, w - 106, va( "frame %i of %i", (int)am.frame - a->first, a->num - 1 ), amDim );
		}

		AM_Box( x, barY, w, 10, amPanelLight );
		if ( a && lengthMs > 0.0f )
			AM_Fill( x + 1, barY + 1, ( w - 2 ) * Com_Clamp( 0, 1, am.ms / span ), 8, amHighlight );
		for ( int k = 0; keyed && k < show->custom->numKeys; k++ ) {
			const float kx = x + 1 + ( w - 3 ) * Com_Clamp( 0, 1, AM_KeySecs( show->custom, k ) * 1000.0f / span );

			AM_Fill( kx, barY - 2, 2, 14, k == am.keySel ? amAccent : amWhite );
		}
		if ( am.click && a && am.clickX >= x && am.clickX < x + w && am.clickY >= barY && am.clickY < barY + 10 ) {
			am.click = qfalse;
			am.scrubbing = qtrue;
		}
		if ( am.scrubbing && a )
			am.ms = Com_Clamp( 0, 1, ( am.cursorX - x ) / w ) * span;
	}
}

/*
===============================================================================

THE ANIMATIONS TAB

===============================================================================
*/

static void AM_RebuildView( void ) {
	const amSkel_t *s;

	am.numView = 0;
	if ( am.skel < 0 || am.skel >= amNumSkels )
		return;
	s = &amSkels[am.skel];
	for ( int i = 0; i < s->numAnims && am.numView < AM_MAX_ANIMS; i++ ) {
		if ( am.search[0] && !Q_stristr( s->anims[i].name, am.search ) )
			continue;
		if ( ( am.filter == AM_F_BASE && s->anims[i].origin != AM_O_BASE ) || ( am.filter == AM_F_CUSTOM && s->anims[i].origin == AM_O_BASE ) )
			continue;
		am.view[am.numView++] = i;
	}
}

static const amAnim_t *AM_SelectedAnim( void ) {
	if ( am.skel < 0 || am.skel >= amNumSkels || am.animSel < 0 || am.animSel >= amSkels[am.skel].numAnims )
		return NULL;
	return &amSkels[am.skel].anims[am.animSel];
}

static void AM_PickAnim( int index ) {
	am.animSel = index;
	am.ms = 0.0f;
	am.paused = qfalse;
}

// the next skeleton with animations, dir 1 or -1 for the one before
static void AM_StepSkel( int dir ) {
	for ( int k = 1; k <= amNumSkels; k++ ) {
		const int next = ( ( am.skel + dir * k ) % amNumSkels + amNumSkels ) % amNumSkels;

		if ( amSkels[next].numAnims ) {
			am.skel = next;
			break;
		}
	}
	am.animSel = -1;
	am.animScroll = 0;
	AM_RebuildView();
}

static void AM_SkelRow( int row, float x, float y, float w, qboolean selected ) {
	const amSkel_t *s = &amSkels[am.skelListView[row]];
	const char *count = va( "%i", s->numAnims );

	AM_TextClipped( x, y, w - 34, s->label, s->custom ? amAdded : amWhite );
	AM_Text( x + w - AM_TextWidth( count ), y, count, amDim );
}

static void AM_AnimRow( int row, float x, float y, float w, qboolean selected ) {
	const amAnim_t *a = &amSkels[am.skel].anims[am.view[row]];
	const char *len = va( "%.1fs", AM_LengthMs( a, 0, -1, 1.0f ) / 1000.0f );

	AM_TextClipped( x, y, w - 34, a->name, a->origin == AM_O_ADDED ? amAdded : a->origin == AM_O_CHANGED ? amChanged : amWhite );
	AM_Text( x + w - AM_TextWidth( len ), y, len, a->loop != -1 ? amAccent : amDim );
}

static void AM_DrawAnimsTab( amShow_t *show ) {
	const float listY = AM_TOP_Y + 22, listW = 262, listH = AM_BUTTON_Y - 6 - listY;
	const amAnim_t *a;
	int clicked;

	// the skeleton, round the ones there are
	{
		static const char *filters[AM_NUM_FILTERS] = { "All", "Base", "Custom" };

		if ( AM_Button( AM_LEFT_X + 124, AM_TOP_Y, 52, filters[am.filter], (qboolean)( am.filter != AM_F_ALL ), qtrue ) ) {
			am.filter = ( am.filter + 1 ) % AM_NUM_FILTERS;
			am.animScroll = 0;
			AM_RebuildView();
		}
	}
	// the skeleton: a click the next, a right click the one before, the arrow all of them in a list
	{
		const qboolean several = (qboolean)( amNumSkels > 1 );

		if ( several && AM_RightClicked( AM_LEFT_X, AM_TOP_Y, 100, AM_CTRL_H ) )
			AM_StepSkel( -1 );
		if ( AM_Button( AM_LEFT_X, AM_TOP_Y, 100, am.skel >= 0 && am.skel < amNumSkels
			? va( "%s (%i)", amSkels[am.skel].label, amSkels[am.skel].numAnims ) : "No skeletons", qfalse, several ) )
			AM_StepSkel( 1 );
		if ( AM_Button( AM_LEFT_X + 102, AM_TOP_Y, 18, am.skelList ? "^" : "v", am.skelList, several ) ) {
			am.skelList = (qboolean)!am.skelList;
			am.skelListScroll = 0;
		}
	}
	AM_Field( AM_LEFT_X + 180, AM_TOP_Y, listW - 180, am.search, sizeof( am.search ), AM_K_TEXT, "search" );

	if ( am.skelList ) {
		int numSkels = 0, sel = -1;

		for ( int i = 0; i < amNumSkels; i++ ) {
			if ( !amSkels[i].numAnims )
				continue;
			if ( i == am.skel )
				sel = numSkels;
			am.skelListView[numSkels++] = i;
		}
		clicked = AM_List( AM_LEFT_X, listY, listW, listH, numSkels, sel, &am.skelListScroll, AM_SkelRow );
		if ( clicked >= 0 ) {
			am.skel = am.skelListView[clicked];
			am.skelList = qfalse;
			am.animSel = -1;
			am.animScroll = 0;
			AM_RebuildView();
		}
		clicked = -1;
	} else
		clicked = AM_List( AM_LEFT_X, listY, listW, listH, am.numView, -1, &am.animScroll, AM_AnimRow );
	// the picked one's row lit
	for ( int i = 0; i < am.numView && !am.skelList; i++ ) {
		if ( am.view[i] == am.animSel ) {
			const int visible = (int)( listH / AM_ROW_H );

			if ( i >= am.animScroll && i < am.animScroll + visible )
				AM_Fill( AM_LEFT_X + 1, listY + 1 + ( i - am.animScroll ) * AM_ROW_H, 3, AM_ROW_H, amAccent );
			break;
		}
	}
	if ( clicked >= 0 )
		AM_PickAnim( am.view[clicked] );

	// about the picked one
	a = AM_SelectedAnim();
	{
		const float x = AM_LEFT_X + listW + 8, w = AM_PV_X - 8 - x;
		float y = listY;

		if ( !a ) {
			const amSkel_t *sk = am.skel >= 0 && am.skel < amNumSkels ? &amSkels[am.skel] : NULL;

			AM_TextClipped( x, y, w, "Pick one from the list", amDim );
			if ( sk ) {
				AM_TextClipped( x, y + AM_ROW_H * 2, w, "This skeleton's list is from", amDim );
				AM_TextSmall( x, y + AM_ROW_H * 3 + 2, w, AM_PakName( sk->source ), sk->custom ? amAdded : amWhite );
				AM_TextClipped( x, y + AM_ROW_H * 5, w, S_COLOR_GREEN "Green" S_COLOR_GREY ": added by it", amWhite );
				AM_TextClipped( x, y + AM_ROW_H * 6, w, "^5Blue" S_COLOR_GREY ": changed by it", amWhite );
			}
		} else {
			const amSkel_t *sk = &amSkels[am.skel];

			AM_TextClipped( x, y, w, a->name, amAccent );
			y += AM_ROW_H + 4;
			if ( a->origin == AM_O_BASE ) {
				AM_TextClipped( x, y, w, "Base game", amDim );
				y += AM_ROW_H;
				AM_TextSmall( x, y + 1, w, sk->custom ? va( "kept by %s", AM_PakName( sk->source ) ) : AM_PakName( sk->source ), amDim );
			} else {
				AM_TextClipped( x, y, w, a->origin == AM_O_ADDED ? "Added by" : "Changed by", a->origin == AM_O_ADDED ? amAdded : amChanged );
				y += AM_ROW_H;
				AM_TextSmall( x, y + 1, w, AM_PakName( sk->source ), a->origin == AM_O_ADDED ? amAdded : amChanged );
				if ( a->origin == AM_O_CHANGED ) {
					y += AM_ROW_H;
					AM_TextClipped( x, y, w, va( "base: %i frames at %g fps", a->baseNum, fabsf( a->baseFps ) ), amDim );
				}
			}
			y += AM_ROW_H + 4;
			AM_TextClipped( x, y, w, va( "%i frames at %g fps", a->num, fabsf( a->fps ) ), amWhite );
			y += AM_ROW_H;
			AM_TextClipped( x, y, w, va( "%.2f seconds%s", AM_LengthMs( a, 0, -1, 1.0f ) / 1000.0f, a->fps < 0 ? ", backwards" : "" ), amWhite );
			y += AM_ROW_H;
			AM_TextClipped( x, y, w, a->loop == -1 ? "Plays once" : "Loops", amWhite );
			y += AM_ROW_H;
			AM_TextClipped( x, y, w, va( "Frames %i to %i in the file", a->first, a->first + a->num - 1 ), amDim );
			y += AM_ROW_H + 8;
			AM_Label( x, y, "Speed %" );
			AM_Field( x + 52, y, w - 52, am.speedText, sizeof( am.speedText ), AM_K_NUMBER, "100" );
			y += AM_CTRL_H + 10;
			if ( AM_Button( x, y, w, "Play on me", qfalse, (qboolean)( am.skel == AM_Skel( AM_HUMANOID ) ) ) ) {
				amCustom_t c;

				memset( &c, 0, sizeof( c ) );
				Q_strncpyz( c.anim, a->name, sizeof( c.anim ) );
				Q_strncpyz( c.speed, am.speedText, sizeof( c.speed ) );
				c.untilMove = qtrue;
				AM_PlayOwn( &c, qfalse );
				AM_Message( "Playing on you, on your screen only: move to end it" );
			}
			y += AM_CTRL_H + 4;
			if ( AM_Button( x, y, w, "Use in a custom emote", qfalse, qtrue ) ) {
				Q_strncpyz( am.custom.anim, a->name, sizeof( am.custom.anim ) );
				am.tab = AM_TAB_CUSTOM;
				am.ms = 0.0f;
				AM_Message( va( "The custom emote plays %s", a->name ) );
			}
			y += AM_CTRL_H + 4;
			// the frame shown, as bones turned on the custom emote's animation: it shows for those
			// without this skeleton too
			if ( AM_Button( x, y, w, "Copy this pose to it", qfalse, qtrue ) ) {
				const int frame = (int)am.frame - a->first;
				const char *why;
				static amCustom_t c;
				int n;

				c = am.custom;
				c.numKeys = 0;
				n = AM_CopyPose( &c, am.skel, a, frame, &why );
				if ( n < 0 )
					AM_Message( va( S_COLOR_YELLOW "Can't copy it: %s", why ) );
				else if ( am.custom.numKeys ) {
					// with keys, into the picked one: a frame a key, and it goes through them
					const int left = AM_KeyTakePose( &c );

					am.tab = AM_TAB_CUSTOM;
					AM_KeyPick( am.keySel );
					AM_Message( va( "Frame %i of %s copied into key %i: %i bones turned%s", frame, a->name, am.keySel + 1, n - left,
						left ? va( S_COLOR_YELLOW ", %i left out (an emote turns %i bones in all)", left, AM_MAX_CBONES ) : "" ) );
				} else {
					am.custom = c;
					am.tab = AM_TAB_CUSTOM;
					am.ms = 0.0f;
					AM_Message( va( "Frame %i of %s copied: %i bones turned on %s, held on its frame %i", frame, a->name, n,
						am.custom.anim, AM_CustomFrom( &am.custom ) ) );
				}
			}
			y += AM_CTRL_H + 10;
			AM_TextClipped( x, y, w, "Orange lengths loop.", amDim );
			y += AM_ROW_H;
			AM_TextClipped( x, y, w, "Drag the model to turn it,", amDim );
			y += AM_ROW_H;
			AM_TextClipped( x, y, w, "the bar to go through it.", amDim );
		}
	}
	show->skel = am.skel;
	show->anim = a;
}

/*
===============================================================================

THE EMOTE CHAINS TAB

===============================================================================
*/

static void AM_NewChain( void ) {
	memset( &am.chain, 0, sizeof( am.chain ) );
	Q_strncpyz( am.chain.name, "em", sizeof( am.chain.name ) );
	am.chainSel = -1;
	am.stepSel = -1;
}

static void AM_ChainRow( int row, float x, float y, float w, qboolean selected ) {
	if ( row == amNumChains )
		AM_Text( x, y, "+ New", amAccent );
	else
		AM_TextClipped( x, y, w, amChains[row].name, amWhite );
}

static void AM_StepRow( int row, float x, float y, float w, qboolean selected ) {
	const amStep_t *s = &am.chain.steps[row];
	qboolean known;
	const int ms = AM_StepMs( s, &known );
	const char *time = s->secs[0] ? va( "%gs", (float)atof( s->secs ) ) : known ? va( "%.1fs", ms / 1000.0f ) : "2s ?";

	AM_TextClipped( x, y, w - 50, va( "%i  %s", row + 1, s->cmd ), amWhite );
	AM_Text( x + w - AM_TextWidth( time ), y, time, s->secs[0] ? amWhite : known ? amDim : amAccent );
}

// the emotes there are to pick from: the server's known ones, and the custom ones
static int AM_PickCount( void ) {
	return CL_EmotePreview_Count() + amNumCustoms;
}

static const char *AM_PickName( int i ) {
	const int n = CL_EmotePreview_Count();

	return i < n ? CL_EmotePreview_Name( i ) : amCustoms[i - n].name;
}

static void AM_RebuildPick( void ) {
	am.numPick = 0;
	for ( int i = 0; i < AM_PickCount() && am.numPick < (int)ARRAY_LEN( am.pickView ); i++ ) {
		const char *name = AM_PickName( i );

		if ( name && ( !am.pickSearch[0] || Q_stristr( name, am.pickSearch ) ) )
			am.pickView[am.numPick++] = i;
	}
}

static void AM_PickRow( int row, float x, float y, float w, qboolean selected ) {
	const int i = am.pickView[row];
	const char *name = AM_PickName( i );
	const char *anim = i < CL_EmotePreview_Count() ? CL_EmotePreview_Anim( name ) : "custom";

	AM_TextClipped( x, y, w * 0.5f, name, amWhite );
	AM_TextClipped( x + w * 0.5f, y, w * 0.5f, anim ? anim : "", amDim );
}

static void AM_AddStep( const char *cmd ) {
	if ( am.chain.numSteps >= AM_MAX_STEPS ) {
		AM_Message( S_COLOR_YELLOW "A chain takes no more steps" );
		return;
	}
	memset( &am.chain.steps[am.chain.numSteps], 0, sizeof( amStep_t ) );
	Q_strncpyz( am.chain.steps[am.chain.numSteps].cmd, cmd, sizeof( am.chain.steps[0].cmd ) );
	am.stepSel = am.chain.numSteps++;
	am.ms = 0.0f;
}

static void AM_SaveChainEdit( void ) {
	const char *old = am.chainSel >= 0 && am.chainSel < amNumChains ? amChains[am.chainSel].name : NULL;
	const char *why = AM_NameProblem( am.chain.name, old );
	int index;

	if ( why ) {
		AM_Message( va( S_COLOR_YELLOW "Can't save it: %s", why ) );
		return;
	}
	if ( !am.chain.numSteps ) {
		AM_Message( S_COLOR_YELLOW "Can't save it: it has no steps" );
		return;
	}
	for ( int i = 0; i < am.chain.numSteps; i++ ) {
		if ( AM_FindChain( am.chain.steps[i].cmd ) >= 0 ) {
			AM_Message( S_COLOR_YELLOW "Can't save it: a chain can't have another chain in it" );
			return;
		}
	}
	if ( !AM_SaveChain( &am.chain ) ) {
		AM_Message( S_COLOR_YELLOW "Couldn't write the file" );
		return;
	}
	// renamed: the old file and command go
	if ( old && Q_stricmp( old, am.chain.name ) ) {
		char oldName[AM_NAME_LEN];

		Q_strncpyz( oldName, old, sizeof( oldName ) );
		FS_HomeRemove( va( AM_DIR "/%s" AM_CHAIN_EXT, oldName ) );
		amChains[am.chainSel].name[0] = '\0';
		AM_RemoveCommand( oldName );
	}
	index = am.chainSel >= 0 && am.chainSel < amNumChains ? am.chainSel : amNumChains < AM_MAX_CHAINS ? amNumChains++ : -1;
	if ( index < 0 ) {
		AM_Message( S_COLOR_YELLOW "There are as many chains as there can be" );
		return;
	}
	amChains[index] = am.chain;
	AM_AddCommand( am.chain.name );
	qsort( amChains, amNumChains, sizeof( amChains[0] ), AM_CompareChains );
	am.chainSel = AM_FindChain( am.chain.name );
	AM_Message( va( S_COLOR_GREEN "Saved: /%s does it", am.chain.name ) );
}

static void AM_DeleteChain( void ) {
	char name[AM_NAME_LEN];

	if ( am.chainSel < 0 || am.chainSel >= amNumChains )
		return;
	Q_strncpyz( name, amChains[am.chainSel].name, sizeof( name ) );
	FS_HomeRemove( va( AM_DIR "/%s" AM_CHAIN_EXT, name ) );
	memmove( &amChains[am.chainSel], &amChains[am.chainSel + 1], ( amNumChains - am.chainSel - 1 ) * sizeof( amChains[0] ) );
	amNumChains--;
	AM_RemoveCommand( name );
	AM_NewChain();
	AM_Message( va( "Deleted %s", name ) );
}

static void AM_DrawChainsTab( amShow_t *show ) {
	const float listH = AM_BUTTON_Y - 6 - AM_TOP_Y;
	const float x = AM_MID_X, w = AM_MID_W;
	const float stepsY = AM_TOP_Y + 22, stepsH = 13 * AM_ROW_H + 2;
	float y;
	int clicked;
	amStep_t *s;

	clicked = AM_List( AM_LEFT_X, AM_TOP_Y, AM_MID_X - 6 - AM_LEFT_X, listH, amNumChains + 1, am.chainSel < 0 ? amNumChains : am.chainSel,
		&am.chainScroll, AM_ChainRow );
	if ( clicked >= 0 ) {
		if ( clicked >= amNumChains ) {
			AM_NewChain();
		} else {
			am.chainSel = clicked;
			am.chain = amChains[clicked];
			am.stepSel = am.chain.numSteps ? 0 : -1;
		}
		am.focus = NULL;
		am.ms = 0.0f;
	}

	AM_Label( x, AM_TOP_Y, "Command /" );
	AM_Field( x + 58, AM_TOP_Y, w - 58, am.chain.name, sizeof( am.chain.name ), AM_K_NAME, "emname" );

	clicked = AM_List( x, stepsY, w, stepsH, am.chain.numSteps, am.stepSel, &am.stepScroll, AM_StepRow );
	if ( clicked >= 0 ) {
		am.stepSel = clicked;
		am.ms = 0.0f;
	}
	if ( !am.chain.numSteps )
		AM_TextClipped( x + 6, stepsY + 4, w - 12, "Add the emotes it does, from the list below", amDim );

	// the step picked: its seconds, moved, taken out
	y = stepsY + stepsH + 4;
	s = am.stepSel >= 0 && am.stepSel < am.chain.numSteps ? &am.chain.steps[am.stepSel] : NULL;
	if ( s ) {
		AM_Label( x, y, "Seconds" );
		AM_Field( x + 48, y, 56, s->secs, sizeof( s->secs ), AM_K_NUMBER, "auto" );
		if ( AM_Button( x + 110, y, 40, "Up", qfalse, (qboolean)( am.stepSel > 0 ) ) ) {
			const amStep_t t = am.chain.steps[am.stepSel - 1];

			am.chain.steps[am.stepSel - 1] = *s;
			am.chain.steps[am.stepSel] = t;
			am.stepSel--;
		} else if ( AM_Button( x + 154, y, 46, "Down", qfalse, (qboolean)( am.stepSel < am.chain.numSteps - 1 ) ) ) {
			const amStep_t t = am.chain.steps[am.stepSel + 1];

			am.chain.steps[am.stepSel + 1] = *s;
			am.chain.steps[am.stepSel] = t;
			am.stepSel++;
		} else if ( AM_Button( x + 204, y, w - 204, "Remove", qfalse, qtrue ) ) {
			memmove( s, s + 1, ( am.chain.numSteps - am.stepSel - 1 ) * sizeof( *s ) );
			am.chain.numSteps--;
			am.stepSel = Q_min( am.stepSel, am.chain.numSteps - 1 );
		}
	}
	y += AM_CTRL_H + 8;

	// the emotes to add
	AM_Label( x, y, "Add" );
	AM_Field( x + 30, y, w - 30, am.pickSearch, sizeof( am.pickSearch ), AM_K_TEXT, "search the emotes, or type one and press Add" );
	y += AM_CTRL_H + 2;
	AM_RebuildPick();
	{
		const float pickH = AM_BUTTON_Y - 6 - y - AM_CTRL_H - 4;

		clicked = AM_List( x, y, w, pickH, am.numPick, am.pickSel, &am.pickScroll, AM_PickRow );
		if ( clicked >= 0 ) {
			if ( clicked == am.pickSel )
				AM_AddStep( AM_PickName( am.pickView[clicked] ) );
			am.pickSel = clicked;
		}
		y += pickH + 4;
		if ( AM_Button( x, y, w, am.pickSel >= 0 && am.pickSel < am.numPick ? va( "Add %s (or click it again)", AM_PickName( am.pickView[am.pickSel] ) )
			: am.pickSearch[0] ? va( "Add %s", am.pickSearch ) : "Add", qfalse, (qboolean)( ( am.pickSel >= 0 && am.pickSel < am.numPick ) || am.pickSearch[0] ) ) ) {
			if ( am.pickSel >= 0 && am.pickSel < am.numPick )
				AM_AddStep( AM_PickName( am.pickView[am.pickSel] ) );
			else
				AM_AddStep( am.pickSearch );
		}
	}

	if ( AM_Button( x, AM_BUTTON_Y, 60, "Save", qfalse, qtrue ) )
		AM_SaveChainEdit();
	if ( AM_Button( x + 64, AM_BUTTON_Y, 70, amRun.on ? "Stop" : "Try it", qfalse, (qboolean)( am.chain.numSteps > 0 ) ) ) {
		if ( amRun.on ) {
			AM_StopChain( "stopped" );
		} else {
			AM_StartChain( &am.chain );
			AM_Close();
			return;
		}
	}
	if ( AM_DangerButton( x + 138, AM_BUTTON_Y, 60, "Delete", (qboolean)( am.chainSel >= 0 ) ) )
		AM_DeleteChain();

	// the preview plays the picked step's emote, or the one picked to add
	show->skel = AM_Skel( AM_HUMANOID );
	{
		const char *emote = s ? s->cmd : am.pickSel >= 0 && am.pickSel < am.numPick ? AM_PickName( am.pickView[am.pickSel] ) : NULL;
		const int custom = emote ? AM_FindCustom( emote ) : -1;

		if ( custom >= 0 ) {
			show->custom = &amCustoms[custom];
		} else if ( emote ) {
			show->anim = AM_FindAnim( show->skel, CL_EmotePreview_Anim( emote ) );
			if ( !show->anim )
				show->caption = "Its animation isn't known yet: do the emote once";
		}
		if ( emote && !show->caption )
			show->caption = emote;
	}
}

/*
===============================================================================

THE CUSTOM EMOTES TAB

===============================================================================
*/

static void AM_NewCustom( void ) {
	memset( &am.custom, 0, sizeof( am.custom ) );
	Q_strncpyz( am.custom.name, "em", sizeof( am.custom.name ) );
	am.custom.untilMove = qtrue;
	am.customSel = -1;
	am.keySel = 0;
}

static void AM_CustomRow( int row, float x, float y, float w, qboolean selected ) {
	if ( row == amNumCustoms )
		AM_Text( x, y, "+ New", amAccent );
	else
		AM_TextClipped( x, y, w, amCustoms[row].name, amWhite );
}

static amCBone_t *AM_CustomBone( amCustom_t *c, const char *bone, qboolean make ) {
	for ( int i = 0; i < c->numBones; i++ ) {
		if ( !Q_stricmp( c->bones[i].bone, bone ) )
			return &c->bones[i];
	}
	if ( !make || c->numBones >= AM_MAX_CBONES )
		return NULL;
	memset( &c->bones[c->numBones], 0, sizeof( amCBone_t ) );
	Q_strncpyz( c->bones[c->numBones].bone, bone, sizeof( c->bones[0].bone ) );
	// unturned in every key
	for ( int k = 0; k < AM_MAX_KEYS; k++ )
		memset( c->keys[k].v[c->numBones], 0, sizeof( c->keys[k].v[0] ) );
	return &c->bones[c->numBones++];
}

// a bone with nothing on it any more, in any key, is taken out
static void AM_TidyBones( amCustom_t *c ) {
	for ( int i = 0; i < c->numBones; i++ ) {
		const amCBone_t *b = &c->bones[i];

		if ( !b->anim[0] && AM_Zero3( b->twist ) && AM_Zero3( b->offset ) && !AM_KeyBoneUsed( c, i ) ) {
			memmove( &c->bones[i], &c->bones[i + 1], ( c->numBones - i - 1 ) * sizeof( c->bones[0] ) );
			for ( int k = 0; k < AM_MAX_KEYS; k++ )
				memmove( c->keys[k].v[i], c->keys[k].v[i + 1], ( c->numBones - i - 1 ) * sizeof( c->keys[k].v[0] ) );
			c->numBones--;
			i--;
		}
	}
}

// the fields over to key k, the ones shown kept in theirs
static void AM_KeySwap( int k ) {
	amCustom_t *c = &am.custom;

	if ( !c->numKeys || k == am.keySel || k < 0 || k >= c->numKeys )
		return;
	AM_KeyStore( c, am.keySel );
	am.keySel = k;
	AM_KeyLoad( c, k );
	am.focus = NULL;
}

// key k picked: the preview held on it
static void AM_KeyPick( int k ) {
	if ( !am.custom.numKeys )
		return;
	AM_KeySwap( Com_Clampi( 0, am.custom.numKeys - 1, k ) );
	am.ms = AM_KeySecs( &am.custom, am.keySel ) * 1000.0f;
	am.paused = qtrue;
}

// a key where the preview is, or a second after the last, posed as it is there
static void AM_KeyAdd( void ) {
	amCustom_t *c = &am.custom;
	float secs, v[AM_MAX_CBONES][6];
	int at;

	if ( c->numKeys >= AM_MAX_KEYS ) {
		AM_Message( va( S_COLOR_YELLOW "An emote takes %i keys", AM_MAX_KEYS ) );
		return;
	}
	if ( !c->numKeys ) {
		c->numKeys = 1;
		c->keys[0].secs[0] = '\0';
		am.keySel = 0;
	}
	AM_KeyStore( c, am.keySel );
	secs = AM_Round( Q_max( 0.0f, am.ms ) / 1000.0f, 10.0f );
	for ( int k = 0; k < c->numKeys; k++ ) {
		if ( fabsf( AM_KeySecs( c, k ) - secs ) < 0.05f ) {
			secs = AM_KeyEnd( c ) + 1.0f;
			break;
		}
	}
	for ( int i = 0; i < c->numBones; i++ ) {
		AM_KeyValues( c, i, secs, v[i], v[i] + 3 );
		for ( int j = 0; j < 6; j++ )
			v[i][j] = AM_Round( v[i][j], j < 3 ? 10.0f : 100.0f );
	}
	// in its place by time
	for ( at = 1; at < c->numKeys && AM_KeySecs( c, at ) <= secs; at++ )
		;
	memmove( &c->keys[at + 1], &c->keys[at], ( c->numKeys - at ) * sizeof( c->keys[0] ) );
	c->numKeys++;
	memset( &c->keys[at], 0, sizeof( c->keys[at] ) );
	Com_sprintf( c->keys[at].secs, sizeof( c->keys[at].secs ), "%g", secs );
	memcpy( c->keys[at].v, v, c->numBones * sizeof( v[0] ) );
	am.keySel = at;
	AM_KeyLoad( c, at );
	am.ms = secs * 1000.0f;
	am.paused = qtrue;
	am.focus = NULL;
	AM_Message( va( "Key %i at %g seconds: pose it, and the bones go there from the key before", at + 1, secs ) );
}

static void AM_KeyRemove( void ) {
	amCustom_t *c = &am.custom;
	const int k = am.keySel;

	if ( k <= 0 || k >= c->numKeys )
		return;
	memmove( &c->keys[k], &c->keys[k + 1], ( c->numKeys - k - 1 ) * sizeof( c->keys[0] ) );
	c->numKeys--;
	am.keySel = k - 1;
	// one left: it holds that pose, as one with no keys
	if ( c->numKeys == 1 ) {
		c->numKeys = 0;
		am.keySel = 0;
	}
	AM_KeyLoad( c, am.keySel );
	AM_TidyBones( c );
	am.ms = AM_KeySecs( c, am.keySel ) * 1000.0f;
	am.focus = NULL;
}

// a pose copied into the picked key: the bones it turns as it has them, the others unturned in
// it; the number that didn't fit is returned
static int AM_KeyTakePose( const amCustom_t *pose ) {
	amCustom_t *c = &am.custom;
	int left = 0;

	Q_strncpyz( c->anim, pose->anim, sizeof( c->anim ) );
	Q_strncpyz( c->to, pose->to, sizeof( c->to ) );
	for ( int i = 0; i < c->numBones; i++ ) {
		for ( int j = 0; j < 3; j++ )
			c->bones[i].twist[j][0] = c->bones[i].offset[j][0] = '\0';
	}
	for ( int i = 0; i < pose->numBones; i++ ) {
		amCBone_t *cb = AM_CustomBone( c, pose->bones[i].bone, qtrue );

		if ( !cb ) {
			left++;
			continue;
		}
		memcpy( cb->twist, pose->bones[i].twist, sizeof( cb->twist ) );
		memcpy( cb->offset, pose->bones[i].offset, sizeof( cb->offset ) );
	}
	AM_KeyStore( c, am.keySel );
	AM_TidyBones( c );
	return left;
}

// the one being made as it's kept and played: its bones' fields as at its first key
static void AM_EditedCustom( amCustom_t *out ) {
	if ( am.custom.numKeys )
		AM_KeyStore( &am.custom, am.keySel );
	AM_TidyBones( &am.custom );
	*out = am.custom;
	if ( out->numKeys )
		AM_KeyLoad( out, 0 );
}

static int amBoneSkel;

static void AM_BoneRow( int row, float x, float y, float w, qboolean selected ) {
	const amBoneInfo_t *b = &amSkels[amBoneSkel].bones[row];
	const amCBone_t *cb = AM_CustomBone( &am.custom, b->name, qfalse );
	const float indent = Q_min( 10, b->depth ) * 6.0f;

	AM_TextClipped( x + indent, y, w - indent - 30, b->name, cb ? amAccent : amWhite );
	if ( cb )
		AM_Text( x + w - 26, y, va( "%s%s%s", cb->anim[0] ? "A" : "", !AM_Zero3( cb->twist ) ? "T" : "", !AM_Zero3( cb->offset ) ? "M" : "" ), amAccent );
}

static void AM_SaveCustomEdit( void ) {
	const char *old = am.customSel >= 0 && am.customSel < amNumCustoms ? amCustoms[am.customSel].name : NULL;
	const char *why = AM_NameProblem( am.custom.name, old );
	static amCustom_t saved;
	int index;

	if ( why ) {
		AM_Message( va( S_COLOR_YELLOW "Can't save it: %s", why ) );
		return;
	}
	if ( !AM_FindAnim( AM_Skel( AM_HUMANOID ), am.custom.anim ) ) {
		AM_Message( S_COLOR_YELLOW "Can't save it: pick its animation first" );
		return;
	}
	AM_EditedCustom( &saved );
	if ( !AM_SaveCustom( &saved ) ) {
		AM_Message( S_COLOR_YELLOW "Couldn't write the file" );
		return;
	}
	if ( old && Q_stricmp( old, am.custom.name ) ) {
		char oldName[AM_NAME_LEN];

		Q_strncpyz( oldName, old, sizeof( oldName ) );
		FS_HomeRemove( va( AM_DIR "/%s" AM_CUSTOM_EXT, oldName ) );
		amCustoms[am.customSel].name[0] = '\0';
		AM_RemoveCommand( oldName );
	}
	index = am.customSel >= 0 && am.customSel < amNumCustoms ? am.customSel : amNumCustoms < AM_MAX_CUSTOMS ? amNumCustoms++ : -1;
	if ( index < 0 ) {
		AM_Message( S_COLOR_YELLOW "There are as many custom emotes as there can be" );
		return;
	}
	amCustoms[index] = saved;
	AM_AddCommand( am.custom.name );
	qsort( amCustoms, amNumCustoms, sizeof( amCustoms[0] ), AM_CompareCustoms );
	am.customSel = AM_FindCustom( am.custom.name );
	AM_Message( va( S_COLOR_GREEN "Saved: /%s plays it%s", am.custom.name, AM_CanShare() ? ", for everyone with this client"
		: ", on your screen (only an admin's with experimental on are shared)" ) );
}

static void AM_DeleteCustom( void ) {
	char name[AM_NAME_LEN];

	if ( am.customSel < 0 || am.customSel >= amNumCustoms )
		return;
	Q_strncpyz( name, amCustoms[am.customSel].name, sizeof( name ) );
	FS_HomeRemove( va( AM_DIR "/%s" AM_CUSTOM_EXT, name ) );
	memmove( &amCustoms[am.customSel], &amCustoms[am.customSel + 1], ( amNumCustoms - am.customSel - 1 ) * sizeof( amCustoms[0] ) );
	amNumCustoms--;
	AM_RemoveCommand( name );
	AM_NewCustom();
	AM_Message( va( "Deleted %s", name ) );
}

static void AM_DrawCustomTab( amShow_t *show ) {
	const float listH = AM_BUTTON_Y - 6 - AM_TOP_Y;
	const float x = AM_MID_X, w = AM_MID_W;
	const int skel = AM_Skel( AM_HUMANOID );
	const amAnim_t *picked = AM_SelectedAnim();
	amSkel_t *s = skel >= 0 ? &amSkels[skel] : NULL;
	float y = AM_TOP_Y;
	int clicked;

	// the preview's < and > step through the keys; its bar, dragged, picks the nearest
	if ( am.custom.numKeys ) {
		if ( am.keyStep )
			AM_KeyPick( am.keySel + am.keyStep );
		else if ( am.scrubbing ) {
			int nearest = 0;

			for ( int k = 1; k < am.custom.numKeys; k++ ) {
				if ( fabsf( AM_KeySecs( &am.custom, k ) * 1000.0f - am.ms ) < fabsf( AM_KeySecs( &am.custom, nearest ) * 1000.0f - am.ms ) )
					nearest = k;
			}
			AM_KeySwap( nearest );
		}
	}
	am.keyStep = 0;

	clicked = AM_List( AM_LEFT_X, AM_TOP_Y, AM_MID_X - 6 - AM_LEFT_X, listH, amNumCustoms + 1, am.customSel < 0 ? amNumCustoms : am.customSel,
		&am.customScroll, AM_CustomRow );
	if ( clicked >= 0 ) {
		if ( clicked >= amNumCustoms ) {
			AM_NewCustom();
		} else {
			am.customSel = clicked;
			am.custom = amCustoms[clicked];
			am.keySel = 0;
		}
		am.focus = NULL;
		am.ms = 0.0f;
	}

	AM_Label( x, y, "Command /" );
	AM_Field( x + 58, y, w - 58, am.custom.name, sizeof( am.custom.name ), AM_K_NAME, "emname" );
	y += AM_CTRL_H + 4;

	// its animation: typed, or the one picked in Animations
	AM_Label( x, y, "Animation" );
	AM_Field( x + 58, y, w - 58 - 64, am.custom.anim, sizeof( am.custom.anim ), AM_K_NAME, "BOTH_..." );
	if ( AM_Button( x + w - 60, y, 60, "Picked", qfalse, (qboolean)( picked && am.skel == skel ) ) ) {
		Q_strncpyz( am.custom.anim, picked->name, sizeof( am.custom.anim ) );
		am.ms = 0.0f;
	}
	y += AM_CTRL_H + 4;
	AM_Label( x, y, "Frames" );
	AM_Field( x + 58, y, 40, am.custom.from, sizeof( am.custom.from ), AM_K_NUMBER, "0" );
	AM_Label( x + 102, y, "to" );
	AM_Field( x + 116, y, 40, am.custom.to, sizeof( am.custom.to ), AM_K_NUMBER, "end" );
	AM_Label( x + 162, y, "Speed %" );
	AM_Field( x + 210, y, w - 210, am.custom.speed, sizeof( am.custom.speed ), AM_K_NUMBER, "100" );
	y += AM_CTRL_H + 4;
	if ( AM_Button( x, y, 66, am.custom.loop ? "Loops" : "Once", am.custom.loop, qtrue ) )
		am.custom.loop = (qboolean)!am.custom.loop;
	if ( AM_Button( x + 70, y, w - 70, am.custom.untilMove ? "Holds its end until you move" : "Ends with the animation", qfalse, (qboolean)!am.custom.loop ) )
		am.custom.untilMove = (qboolean)!am.custom.untilMove;
	y += AM_CTRL_H + 4;
	AM_Label( x, y, "Under it" );
	AM_Field( x + 58, y, w - 58, am.custom.under, sizeof( am.custom.under ), AM_K_NAME, "an RPMod emote everyone sees, or none" );
	y += AM_CTRL_H + 6;

	// the bones: one picked follows another animation, or turns
	if ( s ) {
		const float bonesH = 7 * AM_ROW_H + 2;
		amCBone_t *cb;

		AM_ReadBones( s );
		amBoneSkel = skel;
		clicked = AM_List( x, y, w, bonesH, s->numBones, am.boneSel, &am.boneScroll, AM_BoneRow );
		if ( clicked >= 0 )
			am.boneSel = clicked;
		y += bonesH + 4;
		if ( am.boneSel >= 0 && am.boneSel < s->numBones ) {
			const char *bone = s->bones[am.boneSel].name;
			static const char *axes[3] = { "Pitch", "Yaw", "Roll" };

			show->markBone = bone;
			cb = AM_CustomBone( &am.custom, bone, qfalse );
			AM_TextClipped( x, y + 2, 70, bone, amAccent );
			AM_TextClipped( x + 72, y + 2, w - 72 - 128, cb && cb->anim[0] ? va( "follows %s", cb->anim ) : "follows the emote", amDim );
			if ( AM_Button( x + w - 124, y, 64, "Picked", qfalse, (qboolean)( picked && am.skel == skel ) ) ) {
				cb = AM_CustomBone( &am.custom, bone, qtrue );
				if ( cb )
					Q_strncpyz( cb->anim, picked->name, sizeof( cb->anim ) );
			}
			if ( AM_Button( x + w - 56, y, 56, "Its own", qfalse, (qboolean)( cb && cb->anim[0] ) ) && cb ) {
				cb->anim[0] = '\0';
				AM_TidyBones( &am.custom );
			}
			y += AM_CTRL_H + 4;
			for ( int k = 0; k < 3; k++ ) {
				const float cx = x + k * ( w / 3.0f ), cw = w / 3.0f - 4;

				cb = AM_CustomBone( &am.custom, bone, qfalse );
				AM_Label( cx, y, axes[k] );
				if ( AM_Button( cx + 30, y, 16, "-", qfalse, qtrue ) || AM_Button( cx + cw - 16, y, 16, "+", qfalse, qtrue ) ) {
					const qboolean minus = (qboolean)( am.clickX < cx + 46 );

					cb = AM_CustomBone( &am.custom, bone, qtrue );
					if ( cb ) {
						Com_sprintf( cb->twist[k], AM_NUM_LEN, "%g", AM_Num( cb->twist[k], 0 ) + ( minus ? -10.0f : 10.0f ) );
						AM_TidyBones( &am.custom );
					}
				}
				cb = AM_CustomBone( &am.custom, bone, qfalse );
				if ( cb ) {
					AM_Field( cx + 48, y, cw - 66, cb->twist[k], AM_NUM_LEN, AM_K_NUMBER, "0" );
				} else {
					// typed in, the bone's taken on
					static char blank[3][AM_NUM_LEN];

					AM_Field( cx + 48, y, cw - 66, blank[k], AM_NUM_LEN, AM_K_NUMBER, "0" );
					if ( blank[k][0] && strcmp( blank[k], "-" ) && strcmp( blank[k], "." ) ) {
						cb = AM_CustomBone( &am.custom, bone, qtrue );
						if ( cb ) {
							Q_strncpyz( cb->twist[k], blank[k], AM_NUM_LEN );
							if ( am.focus == blank[k] )
								am.focus = cb->twist[k];
						}
						blank[k][0] = '\0';
					}
				}
			}
			y += AM_CTRL_H + 4;
			{
				static const char *moves[3] = { "Move X", "Y", "Z" };
				static char blankMove[3][AM_NUM_LEN];

				for ( int k = 0; k < 3; k++ ) {
					const float cx = x + k * ( w / 3.0f ), cw = w / 3.0f - 4;
					const float lw = k == 0 ? 40.0f : 14.0f;

					cb = AM_CustomBone( &am.custom, bone, qfalse );
					AM_Label( cx, y, moves[k] );
					if ( cb ) {
						AM_Field( cx + lw, y, cw - lw, cb->offset[k], AM_NUM_LEN, AM_K_NUMBER, "0" );
					} else {
						AM_Field( cx + lw, y, cw - lw, blankMove[k], AM_NUM_LEN, AM_K_NUMBER, "0" );
						if ( blankMove[k][0] && strcmp( blankMove[k], "-" ) && strcmp( blankMove[k], "." ) ) {
							cb = AM_CustomBone( &am.custom, bone, qtrue );
							if ( cb ) {
								Q_strncpyz( cb->offset[k], blankMove[k], AM_NUM_LEN );
								if ( am.focus == blankMove[k] )
									am.focus = cb->offset[k];
							}
							blankMove[k][0] = '\0';
						}
					}
				}
			}
			y += AM_CTRL_H + 4;
			if ( AM_Button( x, y, w, "Reset this bone", qfalse, (qboolean)( AM_CustomBone( &am.custom, bone, qfalse ) != NULL ) ) ) {
				cb = AM_CustomBone( &am.custom, bone, qfalse );
				if ( cb ) {
					cb->anim[0] = '\0';
					for ( int k = 0; k < 3; k++ )
						cb->twist[k][0] = cb->offset[k][0] = '\0';
					AM_TidyBones( &am.custom );
				}
			}
			AM_TextClipped( x, AM_BUTTON_Y - 14, w, S_COLOR_GREY "Drag the rings to turn the bone, the arrows to move it", amWhite );
		} else {
			AM_TextClipped( x, y + 2, w, "Pick a bone: it can follow the animation picked in Animations, turn and move. Tags move with their bones.", amDim );
		}
	}

	// its keys: the bones posed at times, gone through one after the other
	{
		float ky = AM_BUTTON_Y - 16 - 2 * ( AM_CTRL_H + 4 );
		amCustom_t *c = &am.custom;

		AM_Label( x, ky, "Keys" );
		if ( !c->numKeys )
			AM_TextClipped( x + 32, ky + 2, w - 32, "none: it holds the one pose", amDim );
		for ( int k = 0; k < c->numKeys; k++ ) {
			if ( AM_Button( x + 32 + k * 22, ky, 20, va( "%i", k + 1 ), (qboolean)( k == am.keySel ), qtrue ) )
				AM_KeyPick( k );
		}
		ky += AM_CTRL_H + 4;
		if ( c->numKeys ) {
			AM_Label( x, ky, va( "Key %i at", am.keySel + 1 ) );
			if ( am.keySel > 0 )
				AM_Field( x + 50, ky, 40, c->keys[am.keySel].secs, sizeof( c->keys[0].secs ), AM_K_NUMBER, "0" );
			else
				AM_Text( x + 54, ky + 2, "0", amDim );
			AM_Label( x + 94, ky, "seconds" );
		}
		if ( AM_Button( x + w - 128, ky, 70, c->numKeys ? "Add key" : "Add keys", qfalse, (qboolean)( c->numKeys < AM_MAX_KEYS ) ) )
			AM_KeyAdd();
		if ( AM_Button( x + w - 54, ky, 54, "Remove", qfalse, (qboolean)( am.keySel > 0 && c->numKeys ) ) )
			AM_KeyRemove();
	}

	if ( AM_Button( x, AM_BUTTON_Y, 56, "Save", qfalse, qtrue ) )
		AM_SaveCustomEdit();
	{
		const amPlay_t *p = AM_PlayingOn( clc.clientNum );

		if ( AM_Button( x + 60, AM_BUTTON_Y, 74, p ? "Stop on me" : "Try on me", qfalse, (qboolean)( p || AM_FindAnim( skel, am.custom.anim ) ) ) ) {
			if ( p ) {
				AM_StopOwn( qfalse );
			} else {
				static amCustom_t edited;

				AM_EditedCustom( &edited );
				AM_PlayOwn( &edited, qtrue );
				AM_Close();
				return;
			}
		}
	}
	if ( AM_DangerButton( x + 138, AM_BUTTON_Y, 56, "Delete", (qboolean)( am.customSel >= 0 ) ) )
		AM_DeleteCustom();
	AM_TextClipped( x + 198, AM_BUTTON_Y + 2, w - 198, S_COLOR_YELLOW "experimental", amWhite );

	// the picked key as its fields have it, for the preview
	if ( am.custom.numKeys )
		AM_KeyStore( &am.custom, am.keySel );
	show->skel = skel;
	show->custom = &am.custom;
	show->caption = am.custom.anim[0] ? am.custom.anim : NULL;
}

/*
===============================================================================

LIST KEYS AND THE FRAME

===============================================================================
*/

static void AM_ListKey( int key ) {
	int delta;

	switch ( key ) {
	case A_CURSOR_UP:	delta = -1; break;
	case A_CURSOR_DOWN:	delta = 1; break;
	case A_PAGE_UP:		delta = -20; break;
	case A_PAGE_DOWN:	delta = 20; break;
	case A_SPACE:
		am.paused = (qboolean)!am.paused;
		return;
	default:			return;
	}
	if ( am.tab == AM_TAB_ANIMS && am.numView ) {
		int row = 0;

		for ( int i = 0; i < am.numView; i++ ) {
			if ( am.view[i] == am.animSel ) {
				row = i + delta;
				break;
			}
		}
		row = Com_Clampi( 0, am.numView - 1, row );
		AM_PickAnim( am.view[row] );
		AM_SeeRow( row, &am.animScroll, AM_BUTTON_Y - 6 - AM_TOP_Y - 22 );
	} else if ( am.tab == AM_TAB_CHAINS && am.chain.numSteps ) {
		am.stepSel = Com_Clampi( 0, am.chain.numSteps - 1, am.stepSel + delta );
		am.ms = 0.0f;
	} else if ( am.tab == AM_TAB_CUSTOM ) {
		const int skel = AM_Skel( AM_HUMANOID );

		if ( skel >= 0 && amSkels[skel].numBones ) {
			am.boneSel = Com_Clampi( 0, amSkels[skel].numBones - 1, am.boneSel + delta );
			AM_SeeRow( am.boneSel, &am.boneScroll, 7 * AM_ROW_H + 2 );
		}
	}
}

void CL_AnimManager_Draw( void ) {
	static const char *tabs[AM_NUM_TABS] = { "Animations", "Emote chains", "Custom emotes" };
	amShow_t show;
	float tx = AM_LEFT_X;

	if ( !am.open )
		return;
	memset( &show, 0, sizeof( show ) );
	show.skel = -1;

	AM_Fill( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, amPanel );
	AM_Text( AM_LEFT_X, 12, "Animation Manager", amWhite );
	if ( amRun.on )
		AM_TextClipped( 200, 12, 220, va( S_COLOR_GREEN "doing %s: step %i of %i", amRun.chain.name, amRun.step, amRun.chain.numSteps ), amWhite );
	for ( int i = 0; i < AM_NUM_TABS; i++ ) {
		const float tw = AM_TextWidth( tabs[i] ) + 20;

		if ( AM_Button( tx, 32, tw, tabs[i], (qboolean)( am.tab == i ), qtrue ) && am.tab != i ) {
			am.tab = i;
			am.focus = NULL;
			am.ms = 0.0f;
			am.paused = qfalse;
		}
		tx += tw + 4;
	}
	if ( AM_Button( SCREEN_WIDTH - 16 - 50, 12, 50, "Close", qfalse, qtrue ) ) {
		AM_Close();
		return;
	}

	// the preview takes its clicks first: it sits apart from the rest
	switch ( am.tab ) {
	case AM_TAB_ANIMS:		AM_DrawAnimsTab( &show ); break;
	case AM_TAB_CHAINS:		AM_DrawChainsTab( &show ); break;
	default:				AM_DrawCustomTab( &show ); break;
	}
	if ( !am.open )
		return;
	AM_DrawPreview( &show );

	if ( am.message[0] && cls.realtime - am.messageTime < AM_MESSAGE_MS )
		AM_TextClipped( AM_LEFT_X, 456, SCREEN_WIDTH - 32, am.message, amWhite );
	WG_DrawOverlay();
	am.click = am.rclick = qfalse;
	am.wheel = 0;
	re->SetColor( NULL );
	re->DrawStretchPic( am.cursorX, am.cursorY, 32, 32, 0, 0, 1, 1, cls.cursorShader );
}
