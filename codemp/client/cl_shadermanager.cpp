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

// cl_shadermanager.cpp -- finds the shaders of what you look at, and remaps them
//
// "shadermanager crosshair" names the shader under the crosshair. "shadermanager"
// opens a free camera: click something to list its shaders, pick a replacement
// from every shader the game can find, see it on this screen first, then send
// RPMod's "rpshader remap" so the server applies it for everyone.
//
// Collision traces only know brushes and surface flags, so the drawn surfaces are
// traced instead: the map's own surfaces read out of its .bsp (planar, patches,
// triangle soups such as baked misc_models), and the entities cgame hands the
// renderer each frame (md3s and doors/movers by their geometry, Ghoul2 models
// through the Ghoul2 collision code). The shader is worked out the way the
// renderer picks it: custom shader, then skin, then the model's own.
//
// The model, skin and remap names come from watching cgame's register and remap
// calls, since the renderer can't turn those handles back into names.
//
// Effects have no geometry to trace: the effects system reports each piece it
// draws (sprite, line, poly...), the pieces close together are taken as one
// effect, and the box around them is what a click finds.
//
// The Overlays tab sends RPMod's "rpshader overlay add/remove/list": a shader drawn
// over everyone's screen. The Sky tab tints and turns the map's sky, and spins it
// round, on this screen as it's changed; a 3D skybox's view is turned and moved
// instead. Shared, it goes to everyone with this client in an rpnotify, cmd:sky. A sky
// portal shows the map seen from a spot in place of the sky: the map's own 3D skybox,
// or a spot picked with the camera, kept per map in shadermanager/skyportals/<map>.txt.
//
// The Mine tab changes player shaders (force powers, sabers, shells) on this player's
// own only, on this screen, with no command sent: someone else's force lightning stays
// standard here, and everyone else sees this player's standard unless they change it too.
// Whose an effect is: bolted to this player, or started nearer them than anyone else,
// as force powers come out of the caster's hand; the same for sabers, shells and trails
// cgame draws itself. They are kept in shadermanager/mine.txt and put back on every map.
// Shared (cl_shaderShareMine), they go to everyone with this client through rpnotify,
// cmd:mine, who draw them on this player's own the same way; see SHARED PLAYER SHADERS.

#include <algorithm>
#include <map>
#include <set>
#include <vector>
#include <string>
#include "client.h"
#include "qcommon/cm_public.h"
#include "ghoul2/G2.h"
#include "cl_widgets.h"

extern IHeapAllocator *G2VertSpaceClient;

#define SM_TOGGLE_CMD		"shadermanager"
#define SM_TRACE_DIST		16384.0f
#define SM_PATCH_STEPS		6		// samples per 3x3 patch piece, each way
#define SM_MAX_ENTITIES		2048
#define SM_MAX_HANDLES		8192
#define SM_SELF_RADIUS		48.0f	// entities this close to the player are taken as the player
#define SM_TEXT_SCALE		0.8f
#define SM_FLY_SPEED		400.0f
#define SM_REPLY_MS			3000	// server prints this soon after a command are shown as its reply
#define SM_MAX_HISTORY		64
#define SM_MAX_OUTLINE		2048	// triangles of a surface worth outlining
#define SM_FIELD_LEN		64
#define SM_MAX_FX_BITS		4096	// effect pieces kept a frame
#define SM_MAX_FX_GROUPS	256
#define SM_MAX_FX_SHADERS	16		// per effect
#define SM_FX_GAP			24.0f	// pieces whose centres come this close are one effect
#define SM_FX_CORE			8.0f	// how much of a piece's size counts for that
#define SM_FX_BOX			32.0f	// and for its box: glows fade out well before their edge
#define SM_FX_WINDOW		300		// ms: a box holds what its effect covered this long, so flickering pieces don't shrink it

// the manager's panel, docked on the right; the view shrinks to the left of it
#define SM_PANEL_X			368.0f
#define SM_PANEL_W			( SCREEN_WIDTH - SM_PANEL_X - 8.0f )
#define SM_ROW_H			13.0f
#define SM_CTRL_H			15.0f

#define SM_OVERLAY_ROWS		8
#define SM_PORTAL_MS		1000	// a 3D skybox's view rendered this lately: the map has one
#define SM_MINE_FILE		"shadermanager/mine.txt"
#define SM_PORTAL_DIR		"shadermanager/skyportals"
#define SM_PORTAL_ROWS		3
#define SM_MINE_ROWS		7
#define SM_MINE_REACH		96.0f	// what starts this close to the player, and nearer them than anyone else, is theirs
#define SM_MINE_SELF		32.0f	// a player model this close to where the player is taken to be is them
#define SM_SHARE_LEN		700		// a hook's player shaders, well inside what RPMod passes on
#define SM_SHARE_WORD		32		// longer words go in pieces
#define SM_SHARE_GAP_MS		400		// between hooks, for the server's flood protection
#define SM_SHARE_START_MS	5000	// shared this long into a map
#define SM_SHARE_JOIN_MS	4000	// and this long after someone joins
#define SM_MAX_SHARED		64		// player shaders taken from one player

typedef enum {
	SM_HIT_NONE,
	SM_HIT_WORLD,
	SM_HIT_BRUSH,		// a door, mover or other brush entity
	SM_HIT_MODEL,		// an md3
	SM_HIT_GHOUL2,
	SM_HIT_EFFECT		// pieces of the effects system
} smHitKind_t;

typedef struct smHit_s {
	smHitKind_t	kind;
	float		dist;
	vec3_t		pos;
	char		shader[MAX_QPATH];
	char		model[MAX_QPATH];		// the model, "*n" for brush entities
	char		surface[MAX_QPATH];		// the model's surface, "" for the world
	int			surf;					// map surface hit, world and brush entities, -1 otherwise
	int			fx;						// effect hit, into fxGroups, -1 otherwise
	refEntity_t	ent;					// the entity hit, all but the world
} smHit_t;

typedef enum {
	SM_OFF,
	SM_CAMERA,		// flying around, the crosshair picks
	SM_EDIT			// the panel is open on what was picked
} smState_t;

typedef enum {
	SM_TAB_OBJECT,
	SM_TAB_REMAPS,
	SM_TAB_OVERLAYS,
	SM_TAB_SKY,
	SM_TAB_MINE,		// last: the storyteller opens the others by number
	SM_NUM_TABS
} smTab_t;

// the Mine tab's groups of player shaders
typedef enum {
	SM_MINE_ALL,
	SM_MINE_FORCE,
	SM_MINE_SABERS,
	SM_MINE_SHELLS,
	SM_MINE_CHANGED,
	SM_MINE_NUM_GROUPS
} smMineGroup_t;

typedef enum {
	SM_FILTER_ALL,
	SM_FILTER_TEXTURES,
	SM_FILTER_MODELS,
	SM_FILTER_GFX,
	SM_FILTER_FAVES		// the starred ones
} smFilter_t;

#define SM_FAV_LIST			"shaders"
#define SM_STAR_W			11.0f	// the favorite star a shader row starts with

typedef enum {
	SM_F_NONE,			// typing goes to the search box
	SM_F_SEARCH,
	SM_F_OFFSET,
	SM_F_PATH,			// a replacement typed in, instead of picked from the list
	SM_F_OV_SEARCH,		// the overlays tab's shader search
	SM_F_OV_FADE,		// and the overlay's numbers, in the order the command takes them
	SM_F_OV_X,
	SM_F_OV_Y,
	SM_F_OV_W,
	SM_F_OV_H,
	SM_NUM_FIELDS
} smField_t;

typedef struct smList_s {
	int			sel, scroll;
} smList_t;

// the map's drawn geometry
typedef struct smSurf_s {
	int			shader;				// into bspShaders
	int			firstTri, numTris;	// into tris, three vertex numbers each
	vec3_t		mins, maxs;
} smSurf_t;

typedef struct smBModel_s {
	int			firstSurf, numSurfs;
	vec3_t		mins, maxs;
} smBModel_t;

// an md3, kept for tracing
typedef struct smMd3Surf_s {
	std::string	name;
	std::vector<std::string> shaders;
	int			numVerts;
	std::vector<int> tris;
	std::vector<float> xyz;			// numFrames * numVerts * 3
} smMd3Surf_t;

typedef struct smMd3_s {
	bool		loaded, valid;
	int			numFrames;
	std::vector<smMd3Surf_t> surfs;
} smMd3_t;

// a .skin: surface -> shader
typedef struct smSkin_s {
	bool		loaded;
	std::vector<std::string> surfaces, shaders;
} smSkin_t;

// one piece the effects system drew, and the pieces of one effect together
typedef struct smFxBit_s {
	vec3_t		coreMins, coreMaxs;		// what joins it to its neighbours
	vec3_t		mins, maxs;				// what it covers on screen
	qhandle_t	shader;
} smFxBit_t;

typedef struct smFxGroup_s {
	vec3_t		coreMins, coreMaxs;
	vec3_t		mins, maxs;				// covered in this window
	vec3_t		prevMins, prevMaxs;		// and in the one before; the box is both
	int			windowStart, lastSeen;
	int			numBits;
	int			numShaders;
	qhandle_t	shaders[SM_MAX_FX_SHADERS];
} smFxGroup_t;

// a .glm: surface number -> name and shader
typedef struct smGlm_s {
	std::vector<std::string> surfaces, shaders;
} smGlm_t;

// an overlay on the server, as its list printed it, or as this client added it
typedef struct smOverlay_s {
	std::string	line;
	std::string	shader;			// "" if the line doesn't name one
	int			index;			// the number the list gave it, -1 if none
} smOverlay_t;

// the sky's look; all as the map has it is no change
typedef struct smSky_s {
	float		tint[3];		// 0-255 a colour, 255 as it is
	float		spin;			// degrees a second, round the up axis
	vec3_t		angles;			// turned this far: pitch, yaw, roll
	vec3_t		shift;			// a 3D skybox's view moved this far
	qboolean	portalOn;		// the map seen from portalOrg in place of the sky
	vec3_t		portalOrg;
	float		zoom;			// the sky drawn this much nearer, 1 as it is
} smSky_t;

// a spot a sky portal can show the map from
typedef struct smPortal_s {
	vec3_t		org;
	qboolean	map;			// the map's own 3D skybox, else a spot kept for it
} smPortal_t;

// a shader the player's own things draw with, for the Mine tab
typedef struct smPlayerShader_s {
	std::string	name;			// lowercase, no extension
	int			group;			// smMineGroup_t
} smPlayerShader_t;

static struct {
	qboolean	initialized;
	int			font;

	// the map
	char		mapName[MAX_QPATH];	// "" until read
	qboolean	mapLoaded;
	std::vector<std::string> bspShaders;
	std::vector<float> verts;		// xyz
	std::vector<int> tris;
	std::vector<smSurf_t> surfs;
	std::vector<smBModel_t> bmodels;

	// what cgame registered
	std::string	modelNames[SM_MAX_HANDLES];
	std::string	skinNames[SM_MAX_HANDLES];
	smMd3_t		md3s[SM_MAX_HANDLES];
	smSkin_t	skins[SM_MAX_HANDLES];
	std::vector<std::string> glmNames;
	std::vector<smGlm_t> glms;
	std::vector<std::string> remapFrom, remapTo;

	// this frame's main view and the entities in it
	qboolean	haveView;
	int			viewFrame;
	vec3_t		viewOrg;
	matrix3_t	viewAxis;
	float		viewFovX, viewFovY;
	float		viewX, viewW;		// the part of the virtual screen it covers
	refEntity_t	pending[SM_MAX_ENTITIES];
	int			numPending;
	refEntity_t	entities[SM_MAX_ENTITIES];
	int			numEntities;
	smFxBit_t	fxPending[SM_MAX_FX_BITS];
	int			numFxPending;
	smFxGroup_t	fxFrame[SM_MAX_FX_GROUPS];	// this frame's pieces grouped
	int			numFxFrame;
	smFxGroup_t	fxGroups[SM_MAX_FX_GROUPS];	// the effects, kept across frames
	int			numFxGroups;
	qboolean	hideEffects;		// no boxes, so clicks reach what is behind them

	// the last thing found, refreshed once a frame
	int			hitFrame;
	qboolean	hitValid;			// there was a ray to trace
	smHit_t		hit;

	// the manager
	smState_t	state;
	smTab_t		tab;
	int			renderedFrame;
	vec3_t		camOrg, camAng;
	qboolean	held[MAX_KEYS];
	qboolean	looking;			// right mouse held over the panel: the camera moves
	int			lastFrameTime;
	float		cursorX, cursorY;
	qboolean	click;
	float		clickX, clickY;
	int			wheel;
	smField_t	focus;
	char		fields[SM_NUM_FIELDS][SM_FIELD_LEN];

	// what was clicked
	qboolean	haveSel;
	smHitKind_t	selKind;
	char		selWhat[MAX_QPATH * 2];
	std::vector<std::string> selShaders;
	smList_t	selList;				// its rows: selShaders, or with selFaves only the starred ones
	qboolean	selFaves;
	int			selSurf;			// map surface to outline, -1 for none
	char		selBModel[MAX_QPATH];	// "*n" when that surface belongs to a brush entity
	qboolean	selFx;					// an effect, in this box when it was picked
	vec3_t		selFxMins, selFxMaxs;

	// every shader the game can find
	qboolean	indexed;
	std::vector<std::string> shaderNames;
	std::set<std::string> skyShaders;			// the ones with skyParms, by lowercase name
	std::map<std::string, qhandle_t> previews;	// 2D handles, by lowercase name
	std::vector<int> shaderView;
	qboolean	viewSky;					// shaderView only holds skies, for a sky
	smFilter_t	filter;
	smList_t	shaderList;
	qboolean	typingPath;					// the replacement is typed in SM_F_PATH, not picked from the list
	char		pathShader[MAX_QPATH];		// what it is typed as, cleaned up
	qboolean	showPreview;
	qboolean	livePreview;
	char		liveFrom[MAX_QPATH], liveTo[MAX_QPATH];	// remapped on this screen only

	// remaps tab
	smList_t	remapList;

	// overlays tab
	std::vector<int> overlayView;				// the shaders matching its search
	smList_t	overlayShaders;
	qboolean	overlayPreview;					// the overlay drawn where it would go, on this screen
	std::vector<smOverlay_t> overlays;
	smList_t	overlayList;
	int			overlayListTime;				// "overlay list" sent; its reply's lines are the list

	// sky tab
	smSky_t		sky;
	qboolean	skySet;							// the renderer has a sky look to put back
	int			portalTime;						// a 3D skybox's view last rendered
	float		*drag;							// the slider the mouse holds
	char		portalMap[MAX_QPATH];			// the map the portals were read for
	std::vector<smPortal_t> portals;
	smList_t	portalList;						// row 0 is none

	// mine tab: player shaders changed on this screen only
	qboolean	playerIndexed;
	std::vector<smPlayerShader_t> playerShaders;
	std::vector<int> mineView;
	smList_t	mineList;
	int			mineGroup;						// smMineGroup_t
	qboolean	mineLoaded;						// the file was read
	qboolean	mineApplied;					// on this cgame's shaders
	std::vector<std::string> mineFrom, mineTo;	// by lowercase name
	std::map<qhandle_t, qhandle_t> mineHandles;	// the same as the renderer's shaders
	qhandle_t	liveMineFrom, liveMineTo;		// the Mine tab's live preview, on this player's own too
	char		liveMineName[2][MAX_QPATH];
	vec3_t		myOrg;							// where this player's model was last drawn
	int			myOrgFrame;
	std::set<qhandle_t> mapped;					// every shader someone's player shaders replace
	// the player shaders others with this client shared, by client number
	std::vector<std::string> otherFrom[MAX_CLIENTS], otherTo[MAX_CLIENTS];
	std::map<qhandle_t, qhandle_t> otherHandles[MAX_CLIENTS];
	// this player's, shared
	std::vector<std::string> shareQueue;
	int			shareAt;						// due then, 0 not
	int			shareNextTime;
	int			sharePlayers;					// players on the server, to see someone join
	qboolean	shareWarned;
	qboolean	experimentalBox;				// Shared clicked with experimental features off: the warning is up

	// what was sent, for undo: the shader and what it was remapped to before
	std::vector<std::string> undoShader, undoPrevious;

	// messages
	char		status[MAX_STRING_CHARS];
	int			sentTime;
	char		reply[MAX_STRING_CHARS];
	int			replyTime;
} sm;

static cvar_t *cl_shaderCrosshair;
static cvar_t *cl_shaderShareMine;

static vec4_t smWhite	= { 1.0f, 1.0f, 1.0f, 1.0f };
static vec4_t smPanel	= { 0.0f, 0.0f, 0.0f, 0.55f };

/*
===============================================================================

NAMES

===============================================================================
*/

// shader names are compared without extension, as the renderer does
static void SM_ShaderKey( const char *name, char *out, int size ) {
	COM_StripExtension( name, out, size );
	Q_strlwr( out );
	for ( char *p = out; *p; p++ ) {
		if ( *p == '\\' )
			*p = '/';
	}
}

static const char *SM_RemappedTo( const char *shader ) {
	char key[MAX_QPATH];

	SM_ShaderKey( shader, key, sizeof( key ) );
	for ( size_t i = 0; i < sm.remapFrom.size(); i++ ) {
		if ( sm.remapFrom[i] == key )
			return sm.remapTo[i].c_str();
	}
	return NULL;
}

static void SM_NoteRemap( const char *oldShader, const char *newShader ) {
	char from[MAX_QPATH], to[MAX_QPATH];

	if ( !oldShader || !newShader )
		return;
	SM_ShaderKey( oldShader, from, sizeof( from ) );
	SM_ShaderKey( newShader, to, sizeof( to ) );
	for ( size_t i = 0; i < sm.remapFrom.size(); i++ ) {
		if ( sm.remapFrom[i] == from ) {
			sm.remapFrom.erase( sm.remapFrom.begin() + i );
			sm.remapTo.erase( sm.remapTo.begin() + i );
			break;
		}
	}
	// a shader remapped to itself is back to normal
	if ( strcmp( from, to ) ) {
		sm.remapFrom.push_back( from );
		sm.remapTo.push_back( to );
	}
}

// the player shaders changed on this screen only
static int SM_MineIndex( const char *shader ) {
	char key[MAX_QPATH];

	if ( !shader || !shader[0] )
		return -1;
	SM_ShaderKey( shader, key, sizeof( key ) );
	for ( size_t i = 0; i < sm.mineFrom.size(); i++ ) {
		if ( sm.mineFrom[i] == key )
			return (int)i;
	}
	return -1;
}

static const char *SM_MineTo( const char *shader ) {
	const int i = SM_MineIndex( shader );

	return i >= 0 ? sm.mineTo[i].c_str() : NULL;
}

static void SM_LoadMine( void ) {
	char *buf, from[MAX_QPATH], to[MAX_QPATH];
	const char *p, *token;

	sm.mineLoaded = qtrue;
	sm.mineFrom.clear();
	sm.mineTo.clear();
	if ( FS_ReadFile( SM_MINE_FILE, (void **)&buf ) <= 0 || !buf )
		return;
	p = buf;
	COM_BeginParseSession( SM_MINE_FILE );
	while ( 1 ) {
		token = COM_ParseExt( &p, qtrue );
		if ( !token[0] )
			break;
		SM_ShaderKey( token, from, sizeof( from ) );
		token = COM_ParseExt( &p, qfalse );
		if ( !token[0] )
			continue;
		SM_ShaderKey( token, to, sizeof( to ) );
		if ( strcmp( from, to ) && SM_MineIndex( from ) < 0 ) {
			sm.mineFrom.push_back( from );
			sm.mineTo.push_back( to );
		}
	}
	FS_FreeFile( buf );
}

static void SM_SaveMine( void ) {
	std::string out = "// the shader manager's player shaders, changed on this screen only: original replacement\n";

	for ( size_t i = 0; i < sm.mineFrom.size(); i++ )
		out += sm.mineFrom[i] + " " + sm.mineTo[i] + "\n";
	FS_WriteFile( SM_MINE_FILE, out.c_str(), (int)out.size() );
}

// player shaders as the renderer's handles: what cgame and the effects draw with
static void SM_BuildHandles( const std::vector<std::string> &from, const std::vector<std::string> &to, std::map<qhandle_t, qhandle_t> &out ) {
	out.clear();
	for ( size_t i = 0; i < from.size(); i++ ) {
		const qhandle_t f = re->RegisterShader( from[i].c_str() ), t = re->RegisterShader( to[i].c_str() );

		if ( f > 0 && t > 0 && f != t )
			out[f] = t;
	}
}

// every shader someone's player shaders replace, to pass the rest by quickly
static void SM_RebuildMapped( void ) {
	std::map<qhandle_t, qhandle_t>::const_iterator it;

	sm.mapped.clear();
	for ( it = sm.mineHandles.begin(); it != sm.mineHandles.end(); ++it )
		sm.mapped.insert( it->first );
	for ( int c = 0; c < MAX_CLIENTS; c++ ) {
		for ( it = sm.otherHandles[c].begin(); it != sm.otherHandles[c].end(); ++it )
			sm.mapped.insert( it->first );
	}
	if ( sm.liveMineFrom )
		sm.mapped.insert( sm.liveMineFrom );
}

static void SM_BuildMineHandles( void ) {
	SM_BuildHandles( sm.mineFrom, sm.mineTo, sm.mineHandles );
	SM_RebuildMapped();
}

// once cgame has registered its shaders: each map, and after a vid_restart
static void SM_ApplyMine( void ) {
	if ( !sm.mineLoaded )
		SM_LoadMine();
	sm.mineApplied = qtrue;
	SM_BuildHandles( sm.mineFrom, sm.mineTo, sm.mineHandles );
	for ( int c = 0; c < MAX_CLIENTS; c++ )
		SM_BuildHandles( sm.otherFrom[c], sm.otherTo[c], sm.otherHandles[c] );
	SM_RebuildMapped();
	// the others with this client get them once the map is under way
	if ( !sm.mineFrom.empty() )
		sm.shareAt = cls.realtime + SM_SHARE_START_MS;
}

// the Mine tab's live preview: from drawn as to on this player's own; NULL for none
static void SM_SetLiveMine( const char *from, const char *to ) {
	if ( !from || !to ) {
		if ( sm.liveMineFrom ) {
			sm.liveMineFrom = sm.liveMineTo = 0;
			SM_RebuildMapped();
		}
		sm.liveMineName[0][0] = sm.liveMineName[1][0] = '\0';
		return;
	}
	if ( !Q_stricmp( from, sm.liveMineName[0] ) && !Q_stricmp( to, sm.liveMineName[1] ) )
		return;
	Q_strncpyz( sm.liveMineName[0], from, sizeof( sm.liveMineName[0] ) );
	Q_strncpyz( sm.liveMineName[1], to, sizeof( sm.liveMineName[1] ) );
	sm.liveMineFrom = re->RegisterShader( from );
	sm.liveMineTo = re->RegisterShader( to );
	if ( sm.liveMineFrom <= 0 || sm.liveMineTo <= 0 )
		sm.liveMineFrom = sm.liveMineTo = 0;
	SM_RebuildMapped();
}

// where this player is: their model as last drawn, else the snapshot carried ahead as cgame predicts
static void SM_MyOrigin( vec3_t out ) {
	float dt;

	if ( sm.myOrgFrame >= cls.framecount - 1 ) {
		VectorCopy( sm.myOrg, out );
		return;
	}
	dt = Com_Clamp( 0.0f, 0.25f, ( cl.serverTime - cl.snap.serverTime + cl.snap.ping ) * 0.001f );
	VectorMA( cl.snap.ps.origin, dt, cl.snap.ps.velocity, out );
}

// the player whose own what is at org, or bolted to entity entNum, is: the player entity, else
// the player it started within reach of and nearer than any other player or NPC; -1 for none
int CL_ShaderManager_Owner( const vec3_t org, int entNum ) {
	if ( sm.mapped.empty() || !cl.snap.valid )
		return -1;
	if ( entNum >= 0 && entNum < ENTITYNUM_WORLD )
		return entNum < MAX_CLIENTS ? entNum : -1;
	return CL_ShaderManager_NearestPlayer( org );
}

// the player what's at org is nearest, within reach and nearer than any other player or NPC;
// -1 for none
int CL_ShaderManager_NearestPlayer( const vec3_t org ) {
	vec3_t me;
	float best = SM_MINE_REACH, dist;
	int owner = -1;

	if ( !org || !cl.snap.valid )
		return -1;
	// the player the view is of: this one where cgame predicts them, or the one followed
	if ( cl.snap.ps.pm_type != PM_SPECTATOR ) {
		if ( cl.snap.ps.clientNum == clc.clientNum )
			SM_MyOrigin( me );
		else
			VectorCopy( cl.snap.ps.origin, me );
		dist = Distance( org, me );
		if ( dist < best ) {
			best = dist;
			owner = cl.snap.ps.clientNum;
		}
	}
	for ( int i = 0; i < cl.snap.numEntities; i++ ) {
		const entityState_t *es = &cl.parseEntities[( cl.snap.parseEntitiesNum + i ) & ( MAX_PARSE_ENTITIES - 1 )];
		const qboolean player = (qboolean)( es->eType == ET_PLAYER && es->number < MAX_CLIENTS );

		if ( es->number == cl.snap.ps.clientNum || ( !player && es->eType != ET_NPC ) )
			continue;
		dist = Distance( org, es->pos.trBase );
		if ( dist < best ) {
			best = dist;
			owner = player ? es->number : -1;	// an NPC's is nobody's
		}
	}
	return owner;
}

// a shader of a player's own: their player shader in its place, this player's own or one they shared
qhandle_t CL_ShaderManager_OwnerShader( int owner, qhandle_t shader ) {
	const std::map<qhandle_t, qhandle_t> *handles;
	std::map<qhandle_t, qhandle_t>::const_iterator it;

	if ( owner < 0 || owner >= MAX_CLIENTS || shader <= 0 )
		return shader;
	if ( owner == clc.clientNum ) {
		if ( shader == sm.liveMineFrom )
			return sm.liveMineTo;
		handles = &sm.mineHandles;
	} else {
		handles = &sm.otherHandles[owner];
	}
	if ( handles->empty() )
		return shader;
	it = handles->find( shader );
	return it == handles->end() ? shader : it->second;
}

// a shader cgame draws something at org with: the player shader of the player whose own it is
qhandle_t CL_ShaderManager_MineFor( qhandle_t shader, const vec3_t org, int entNum ) {
	if ( shader <= 0 || sm.mapped.find( shader ) == sm.mapped.end() )
		return shader;
	return CL_ShaderManager_OwnerShader( CL_ShaderManager_Owner( org, entNum ), shader );
}

// an entity cgame draws: each player's own sabers and shells with their player shaders.
// This player's model, drawn where cgame predicts them, says where they are
const refEntity_t *CL_ShaderManager_MineEntity( const refEntity_t *ent, refEntity_t *copy ) {
	qhandle_t shader;

	// where this player is, for the saber light too
	if ( ent->reType == RT_MODEL && ent->ghoul2 && cl.snap.valid && cl.snap.ps.clientNum == clc.clientNum ) {
		vec3_t me;

		SM_MyOrigin( me );
		if ( ( ent->renderfx & RF_THIRD_PERSON ) || Distance( ent->origin, me ) < SM_MINE_SELF ) {
			VectorCopy( ent->origin, sm.myOrg );
			sm.myOrgFrame = cls.framecount;
		}
	}
	if ( sm.mapped.empty() )
		return ent;
	shader = CL_ShaderManager_MineFor( ent->customShader, ent->origin, -1 );
	if ( shader == ent->customShader )
		return ent;
	if ( copy != ent )
		*copy = *ent;
	copy->customShader = shader;
	return copy;
}

/*
===============================================================================

SHARED PLAYER SHADERS

Through RPMod's rpnotify, to everyone with this client: cmd:mine/<client>/<part>/<parts>:
<shader>><shader>|..., the words split for the server and joined again; cmd:mine/<client>/clear.

===============================================================================
*/

// a word, a space put in after each / and every SM_SHARE_WORD characters: RPMod chokes on long ones
static void SM_AppendSplit( std::string &out, const char *word ) {
	int run = 0;

	for ( ; *word; word++ ) {
		out += *word;
		run++;
		if ( word[1] && ( *word == '/' || run >= SM_SHARE_WORD ) ) {
			out += ' ';
			run = 0;
		}
	}
}

static void SM_QueueShare( const char *text ) {
	sm.shareQueue.push_back( va( "rpnotify -1 1 %s", text ) );
}

// this player's player shaders, in as few hooks as fit
static void SM_ShareMine( void ) {
	std::vector<std::string> parts;
	std::string part, piece;

	if ( sm.mineFrom.empty() ) {
		SM_QueueShare( va( "cmd:mine/%i/clear", clc.clientNum ) );
		return;
	}
	for ( size_t i = 0; i < sm.mineFrom.size(); i++ ) {
		piece.clear();
		SM_AppendSplit( piece, ( sm.mineFrom[i] + ">" + sm.mineTo[i] ).c_str() );
		if ( !part.empty() && part.size() + piece.size() + 3 > SM_SHARE_LEN ) {
			parts.push_back( part );
			part.clear();
		}
		if ( !part.empty() )
			part += " | ";
		part += piece;
	}
	parts.push_back( part );
	for ( size_t i = 0; i < parts.size(); i++ )
		SM_QueueShare( va( "cmd:mine/%i/%i/%i: %s", clc.clientNum, (int)i + 1, (int)parts.size(), parts[i].c_str() ) );
}

static qboolean SM_ShareOn( void ) {
	return (qboolean)( cl_shaderShareMine && cl_shaderShareMine->integer );
}

// once a frame: shared when due, a hook at a time so the server's flood protection lets them by;
// someone joining gets them a little later, and a player gone takes theirs with them
static void SM_MineFrame( void ) {
	int players = 0;

	if ( cls.state != CA_ACTIVE )
		return;
	for ( int c = 0; c < MAX_CLIENTS; c++ ) {
		const char *info = cl.gameState.stringData + cl.gameState.stringOffsets[CS_PLAYERS + c];

		if ( info[0] ) {
			players++;
		} else if ( !sm.otherFrom[c].empty() ) {
			sm.otherFrom[c].clear();
			sm.otherTo[c].clear();
			sm.otherHandles[c].clear();
			SM_RebuildMapped();
		}
	}
	if ( players > sm.sharePlayers && sm.sharePlayers && !sm.mineFrom.empty() )
		sm.shareAt = cls.realtime + SM_SHARE_JOIN_MS;
	sm.sharePlayers = players;

	if ( sm.shareAt && cls.realtime >= sm.shareAt ) {
		sm.shareAt = 0;
		if ( !SM_ShareOn() )
			Com_DPrintf( "Shader manager: player shaders not shared: Shared is off in the Mine tab\n" );
		if ( SM_ShareOn() ) {
			if ( CL_RpHooks_Experimental() )
				SM_ShareMine();
			else if ( !sm.shareWarned ) {
				sm.shareWarned = qtrue;
				Com_Printf( S_COLOR_YELLOW "Shader manager: your player shaders aren't shared: TOGGLE EXPERIMENTAL in the npcmanager first\n" );
			}
		}
	}
	if ( !sm.shareQueue.empty() && cls.realtime >= sm.shareNextTime ) {
		Com_Printf( S_COLOR_GREEN "Shader manager: " S_COLOR_WHITE "sharing your player shaders: %s\n", sm.shareQueue.front().c_str() );
		CL_AddReliableCommand( sm.shareQueue.front().c_str(), qfalse );
		sm.shareQueue.erase( sm.shareQueue.begin() );
		sm.shareNextTime = cls.realtime + SM_SHARE_GAP_MS;
	}
}

// a shader name as a player sent it: a path, nothing that could be read another way
static qboolean SM_SharedName( const char *name ) {
	if ( !name[0] || strlen( name ) >= MAX_QPATH )
		return qfalse;
	for ( ; *name; name++ ) {
		if ( !isalnum( (unsigned char)*name ) && !strchr( "_-./", *name ) )
			return qfalse;
	}
	return qtrue;
}

// cmd:mine/<client>/<part>/<parts>: <shader>><shader>|...; cmd:mine/<client>/clear
void CL_ShaderManager_MineHook( char **argv, int argc, const char *message ) {
	char buf[MAX_STRING_CHARS], *pair, *next, *to, key[MAX_QPATH];
	int client, len = 0;

	if ( argc < 2 )
		return;
	client = atoi( argv[0] );
	if ( client < 0 || client >= MAX_CLIENTS || client == clc.clientNum )
		return;
	if ( !Q_stricmp( argv[1], "clear" ) || atoi( argv[1] ) <= 1 ) {
		sm.otherFrom[client].clear();
		sm.otherTo[client].clear();
	}
	if ( Q_stricmp( argv[1], "clear" ) ) {
		// the words came split for the server: the spaces go again
		for ( const char *c = message; *c && len < (int)sizeof( buf ) - 1; c++ ) {
			if ( *c != ' ' )
				buf[len++] = *c;
		}
		buf[len] = '\0';
		for ( pair = buf; pair && *pair; pair = next ) {
			next = strchr( pair, '|' );
			if ( next )
				*next++ = '\0';
			to = strchr( pair, '>' );
			if ( !to )
				continue;
			*to++ = '\0';
			if ( !SM_SharedName( pair ) || !SM_SharedName( to ) || sm.otherFrom[client].size() >= SM_MAX_SHARED )
				continue;
			SM_ShaderKey( pair, key, sizeof( key ) );
			sm.otherFrom[client].push_back( key );
			SM_ShaderKey( to, key, sizeof( key ) );
			sm.otherTo[client].push_back( key );
		}
	}
	Com_Printf( S_COLOR_GREEN "Shader manager: " S_COLOR_WHITE "%s shared %i player shader%s\n",
		Info_ValueForKey( cl.gameState.stringData + cl.gameState.stringOffsets[CS_PLAYERS + client], "n" ),
		(int)sm.otherFrom[client].size(), sm.otherFrom[client].size() == 1 ? "" : "s" );
	// the renderer's handles now if cgame is up, else once it is
	if ( sm.mineApplied )
		SM_BuildHandles( sm.otherFrom[client], sm.otherTo[client], sm.otherHandles[client] );
	else
		sm.otherHandles[client].clear();
	SM_RebuildMapped();
}

qhandle_t CL_ShaderManager_RegisterModel( const char *name ) {
	qhandle_t h = re->RegisterModel( name );

	if ( h > 0 && h < SM_MAX_HANDLES && name ) {
		if ( Q_stricmp( sm.modelNames[h].c_str(), name ) ) {
			sm.modelNames[h] = name;
			sm.md3s[h] = smMd3_t();
		}
	}
	return h;
}

qhandle_t CL_ShaderManager_RegisterSkin( const char *name ) {
	qhandle_t h = re->RegisterSkin( name );

	if ( h > 0 && h < SM_MAX_HANDLES && name ) {
		if ( Q_stricmp( sm.skinNames[h].c_str(), name ) ) {
			sm.skinNames[h] = name;
			sm.skins[h] = smSkin_t();
		}
	}
	return h;
}

void CL_ShaderManager_RemapShader( const char *oldShader, const char *newShader, const char *timeOffset ) {
	re->RemapShader( oldShader, newShader, timeOffset );
	SM_NoteRemap( oldShader, newShader );
}

static const char *SM_ShaderName( qhandle_t h ) {
	const char *name;

	if ( h <= 0 )
		return NULL;
	name = re->ShaderNameFromIndex( h );
	return name && name[0] ? name : NULL;
}

/*
===============================================================================

SKINS AND MODELS

===============================================================================
*/

// one .skin file's "surface,shader" lines, the way the renderer reads them
static void SM_ParseSkinFile( const char *path, smSkin_t *skin ) {
	char *buf, *p, *line, *comma;

	if ( FS_ReadFile( path, (void **)&buf ) <= 0 || !buf )
		return;
	for ( p = buf; *p; ) {
		line = p;
		while ( *p && *p != '\n' && *p != '\r' )
			p++;
		if ( *p )
			*p++ = '\0';
		comma = strchr( line, ',' );
		if ( !comma )
			continue;
		*comma = '\0';
		char surf[MAX_QPATH], shader[MAX_QPATH];
		Q_strncpyz( surf, line, sizeof( surf ) );
		Q_strncpyz( shader, comma + 1, sizeof( shader ) );
		Q_strlwr( surf );
		// trim
		for ( char *s : { surf, shader } ) {
			int len = strlen( s ), start = 0;

			while ( len && ( s[len - 1] == ' ' || s[len - 1] == '\t' || s[len - 1] == '"' ) )
				s[--len] = '\0';
			while ( s[start] == ' ' || s[start] == '\t' || s[start] == '"' )
				start++;
			memmove( s, s + start, len - start + 1 );
		}
		if ( !surf[0] || !shader[0] || !strncmp( surf, "tag_", 4 ) )
			continue;
		skin->surfaces.push_back( surf );
		skin->shaders.push_back( shader );
	}
	FS_FreeFile( buf );
}

// the file a skin handle was registered from, NULL if cgame didn't register it
const char *CL_ShaderManager_SkinName( qhandle_t h ) {
	if ( h <= 0 || h >= SM_MAX_HANDLES || sm.skinNames[h].empty() )
		return NULL;
	return sm.skinNames[h].c_str();
}

static smSkin_t *SM_GetSkin( qhandle_t h ) {
	smSkin_t *skin;
	const char *name;

	if ( h <= 0 || h >= SM_MAX_HANDLES || sm.skinNames[h].empty() )
		return NULL;
	skin = &sm.skins[h];
	if ( skin->loaded )
		return skin;
	skin->loaded = true;

	// "models/players/x/|head|torso|lower" is three files, as RE_SplitSkins does it
	name = sm.skinNames[h].c_str();
	if ( strchr( name, '|' ) ) {
		char base[MAX_QPATH], *part, *next;

		Q_strncpyz( base, name, sizeof( base ) );
		part = strchr( base, '|' );
		*part++ = '\0';
		while ( part && *part ) {
			next = strchr( part, '|' );
			if ( next )
				*next++ = '\0';
			SM_ParseSkinFile( va( "%s%s.skin", base, part ), skin );
			part = next;
		}
	} else {
		SM_ParseSkinFile( name, skin );
	}
	return skin;
}

static const char *SM_SkinShader( qhandle_t h, const char *surface ) {
	smSkin_t *skin = SM_GetSkin( h );

	if ( !skin )
		return NULL;
	for ( size_t i = 0; i < skin->surfaces.size(); i++ ) {
		if ( !Q_stricmp( skin->surfaces[i].c_str(), surface ) )
			return skin->shaders[i].c_str();
	}
	return NULL;
}

static smMd3_t *SM_GetMd3( qhandle_t h ) {
	smMd3_t *m;
	byte *buf;
	int len;
	const md3Header_t *header;
	const char *name;

	if ( h <= 0 || h >= SM_MAX_HANDLES || sm.modelNames[h].empty() )
		return NULL;
	m = &sm.md3s[h];
	if ( m->loaded )
		return m->valid ? m : NULL;
	m->loaded = true;

	name = sm.modelNames[h].c_str();
	len = FS_ReadFile( name, (void **)&buf );
	if ( len <= 0 || !buf )
		return NULL;
	header = (const md3Header_t *)buf;
	if ( len >= (int)sizeof( *header ) && LittleLong( header->ident ) == MD3_IDENT ) {
		int numFrames = LittleLong( header->numFrames ), numSurfaces = LittleLong( header->numSurfaces );
		int ofs = LittleLong( header->ofsSurfaces );
		bool ok = numFrames > 0 && numSurfaces >= 0;

		m->numFrames = numFrames;
		for ( int i = 0; i < numSurfaces && ok; i++ ) {
			const md3Surface_t *s;
			smMd3Surf_t out;
			int numVerts, numTris, numShaders;

			if ( ofs < 0 || ofs > len - (int)sizeof( md3Surface_t ) ) {
				ok = false;
				break;
			}
			s = (const md3Surface_t *)( buf + ofs );
			numVerts = LittleLong( s->numVerts );
			numTris = LittleLong( s->numTriangles );
			numShaders = LittleLong( s->numShaders );
			if ( numVerts < 0 || numTris < 0 || numShaders < 0
				|| LittleLong( s->numFrames ) != numFrames
				|| ofs + LittleLong( s->ofsTriangles ) + numTris * (int)sizeof( md3Triangle_t ) > len
				|| ofs + LittleLong( s->ofsShaders ) + numShaders * (int)sizeof( md3Shader_t ) > len
				|| (int64_t)ofs + LittleLong( s->ofsXyzNormals ) + (int64_t)numVerts * numFrames * (int)sizeof( md3XyzNormal_t ) > len ) {
				ok = false;
				break;
			}

			out.name = s->name;
			Q_strlwr( &out.name[0] );
			out.numVerts = numVerts;
			for ( int j = 0; j < numShaders; j++ ) {
				const md3Shader_t *sh = (const md3Shader_t *)( buf + ofs + LittleLong( s->ofsShaders ) ) + j;
				char shader[MAX_QPATH];

				Q_strncpyz( shader, sh->name, sizeof( shader ) );
				out.shaders.push_back( shader );
			}
			const md3Triangle_t *tri = (const md3Triangle_t *)( buf + ofs + LittleLong( s->ofsTriangles ) );
			for ( int j = 0; j < numTris; j++ ) {
				for ( int k = 0; k < 3; k++ ) {
					int v = LittleLong( tri[j].indexes[k] );
					out.tris.push_back( v >= 0 && v < numVerts ? v : 0 );
				}
			}
			const md3XyzNormal_t *xyz = (const md3XyzNormal_t *)( buf + ofs + LittleLong( s->ofsXyzNormals ) );
			out.xyz.resize( (size_t)numVerts * numFrames * 3 );
			for ( int j = 0; j < numVerts * numFrames; j++ ) {
				for ( int k = 0; k < 3; k++ )
					out.xyz[j * 3 + k] = LittleShort( xyz[j].xyz[k] ) * (float)MD3_XYZ_SCALE;
			}
			m->surfs.push_back( out );
			ofs += LittleLong( s->ofsEnd );
		}
		m->valid = ok;
	}
	FS_FreeFile( buf );
	return m->valid ? m : NULL;
}

// a .glm's surface names and shaders, by surface number
static const smGlm_t *SM_GetGlm( const char *name ) {
	smGlm_t glm;
	byte *buf;
	int len;
	const mdxmHeader_t *header;

	for ( size_t i = 0; i < sm.glmNames.size(); i++ ) {
		if ( !Q_stricmp( sm.glmNames[i].c_str(), name ) )
			return &sm.glms[i];
	}

	len = FS_ReadFile( name, (void **)&buf );
	if ( len > 0 && buf ) {
		header = (const mdxmHeader_t *)buf;
		if ( len >= (int)sizeof( *header ) && LittleLong( header->ident ) == MDXM_IDENT ) {
			int numSurfaces = LittleLong( header->numSurfaces );
			int ofsTable = (int)sizeof( mdxmHeader_t );

			// the hierarchy offsets follow the header and count from there, as G2_API.cpp reads them
			for ( int i = 0; i < numSurfaces; i++ ) {
				const mdxmSurfHierarchy_t *s;
				int at;

				if ( ofsTable + ( i + 1 ) * 4 > len )
					break;
				at = ofsTable + LittleLong( ( (const int *)( buf + ofsTable ) )[i] );
				if ( at < 0 || at > len - (int)sizeof( mdxmSurfHierarchy_t ) )
					break;
				s = (const mdxmSurfHierarchy_t *)( buf + at );
				char surf[MAX_QPATH], shader[MAX_QPATH];
				Q_strncpyz( surf, s->name, sizeof( surf ) );
				Q_strncpyz( shader, s->shader, sizeof( shader ) );
				Q_strlwr( surf );
				glm.surfaces.push_back( surf );
				glm.shaders.push_back( shader );
			}
		}
		FS_FreeFile( buf );
	}
	sm.glmNames.push_back( name );
	sm.glms.push_back( glm );
	return &sm.glms.back();
}

/*
===============================================================================

THE MAP

===============================================================================
*/

static void SM_FreeMap( void ) {
	sm.mapName[0] = '\0';
	sm.mapLoaded = qfalse;
	std::vector<std::string>().swap( sm.bspShaders );
	std::vector<float>().swap( sm.verts );
	std::vector<int>().swap( sm.tris );
	std::vector<smSurf_t>().swap( sm.surfs );
	std::vector<smBModel_t>().swap( sm.bmodels );
}

static int SM_AddVert( const vec3_t v ) {
	sm.verts.push_back( v[0] );
	sm.verts.push_back( v[1] );
	sm.verts.push_back( v[2] );
	return (int)( sm.verts.size() / 3 ) - 1;
}

static void SM_SurfBounds( smSurf_t *s ) {
	ClearBounds( s->mins, s->maxs );
	for ( int i = s->firstTri * 3; i < ( s->firstTri + s->numTris ) * 3; i++ )
		AddPointToBounds( &sm.verts[sm.tris[i] * 3], s->mins, s->maxs );
}

// a patch's surface, sampled from its 3x3 bezier pieces
static void SM_AddPatch( const mapVert_t *verts, int width, int height, smSurf_t *s ) {
	for ( int py = 0; py + 2 < height; py += 2 ) {
		for ( int px = 0; px + 2 < width; px += 2 ) {
			int base = (int)( sm.verts.size() / 3 );

			for ( int j = 0; j <= SM_PATCH_STEPS; j++ ) {
				float v = j / (float)SM_PATCH_STEPS, bv[3] = { ( 1 - v ) * ( 1 - v ), 2 * v * ( 1 - v ), v * v };

				for ( int i = 0; i <= SM_PATCH_STEPS; i++ ) {
					float u = i / (float)SM_PATCH_STEPS, bu[3] = { ( 1 - u ) * ( 1 - u ), 2 * u * ( 1 - u ), u * u };
					vec3_t p = { 0, 0, 0 };

					for ( int b = 0; b < 3; b++ ) {
						for ( int a = 0; a < 3; a++ )
							VectorMA( p, bu[a] * bv[b], verts[( py + b ) * width + px + a].xyz, p );
					}
					SM_AddVert( p );
				}
			}
			for ( int j = 0; j < SM_PATCH_STEPS; j++ ) {
				for ( int i = 0; i < SM_PATCH_STEPS; i++ ) {
					int a = base + j * ( SM_PATCH_STEPS + 1 ) + i, b = a + 1, c = a + SM_PATCH_STEPS + 1, d = c + 1;

					sm.tris.push_back( a ); sm.tris.push_back( b ); sm.tris.push_back( c );
					sm.tris.push_back( b ); sm.tris.push_back( d ); sm.tris.push_back( c );
					s->numTris += 2;
				}
			}
		}
	}
}

static qboolean SM_LumpOk( const dheader_t *h, int lump, int size, int len ) {
	int ofs = LittleLong( h->lumps[lump].fileofs ), n = LittleLong( h->lumps[lump].filelen );

	return (qboolean)( ofs >= 0 && n >= 0 && ofs <= len - n && n % size == 0 );
}

// reads the drawn surfaces out of the map's .bsp
static void SM_LoadMap( const char *mapName ) {
	char path[MAX_QPATH];
	byte *buf;
	int len;
	const dheader_t *h;

	SM_FreeMap();
	Q_strncpyz( sm.mapName, mapName, sizeof( sm.mapName ) );
	sm.mapLoaded = qtrue;	// even if it fails, don't read it every frame

	Com_sprintf( path, sizeof( path ), "maps/%s.bsp", mapName );
	len = FS_ReadFile( path, (void **)&buf );
	if ( len <= 0 || !buf )
		return;
	h = (const dheader_t *)buf;
	if ( len < (int)sizeof( *h ) || LittleLong( h->ident ) != BSP_IDENT || LittleLong( h->version ) != BSP_VERSION
		|| !SM_LumpOk( h, LUMP_SHADERS, sizeof( dshader_t ), len ) || !SM_LumpOk( h, LUMP_MODELS, sizeof( dmodel_t ), len )
		|| !SM_LumpOk( h, LUMP_DRAWVERTS, sizeof( mapVert_t ), len ) || !SM_LumpOk( h, LUMP_DRAWINDEXES, sizeof( int ), len )
		|| !SM_LumpOk( h, LUMP_SURFACES, sizeof( dsurface_t ), len ) ) {
		Com_Printf( S_COLOR_YELLOW "Shader manager: can't read %s\n", path );
		FS_FreeFile( buf );
		return;
	}

	const dshader_t *shaders = (const dshader_t *)( buf + LittleLong( h->lumps[LUMP_SHADERS].fileofs ) );
	const dmodel_t *models = (const dmodel_t *)( buf + LittleLong( h->lumps[LUMP_MODELS].fileofs ) );
	const mapVert_t *verts = (const mapVert_t *)( buf + LittleLong( h->lumps[LUMP_DRAWVERTS].fileofs ) );
	const int *indexes = (const int *)( buf + LittleLong( h->lumps[LUMP_DRAWINDEXES].fileofs ) );
	const dsurface_t *surfs = (const dsurface_t *)( buf + LittleLong( h->lumps[LUMP_SURFACES].fileofs ) );
	int numShaders = LittleLong( h->lumps[LUMP_SHADERS].filelen ) / sizeof( dshader_t );
	int numModels = LittleLong( h->lumps[LUMP_MODELS].filelen ) / sizeof( dmodel_t );
	int numVerts = LittleLong( h->lumps[LUMP_DRAWVERTS].filelen ) / sizeof( mapVert_t );
	int numIndexes = LittleLong( h->lumps[LUMP_DRAWINDEXES].filelen ) / sizeof( int );
	int numSurfs = LittleLong( h->lumps[LUMP_SURFACES].filelen ) / sizeof( dsurface_t );

	for ( int i = 0; i < numShaders; i++ ) {
		char name[MAX_QPATH];

		Q_strncpyz( name, shaders[i].shader, sizeof( name ) );
		sm.bspShaders.push_back( name );
	}

	// every surface belongs to one model: 0 is the world, the rest are brush entities
	for ( int m = 0; m < numModels; m++ ) {
		smBModel_t bm;
		int first = LittleLong( models[m].firstSurface ), count = LittleLong( models[m].numSurfaces );

		bm.firstSurf = (int)sm.surfs.size();
		bm.numSurfs = 0;
		for ( int k = 0; k < 3; k++ ) {
			bm.mins[k] = LittleFloat( models[m].mins[k] );
			bm.maxs[k] = LittleFloat( models[m].maxs[k] );
		}
		for ( int i = first; i < first + count && i >= 0 && i < numSurfs; i++ ) {
			const dsurface_t *in = &surfs[i];
			int type = LittleLong( in->surfaceType ), shader = LittleLong( in->shaderNum );
			int fv = LittleLong( in->firstVert ), nv = LittleLong( in->numVerts );
			int fi = LittleLong( in->firstIndex ), ni = LittleLong( in->numIndexes );
			smSurf_t s;

			if ( shader < 0 || shader >= numShaders || fv < 0 || nv <= 0 || fv > numVerts - nv )
				continue;
			s.shader = shader;
			s.firstTri = (int)( sm.tris.size() / 3 );
			s.numTris = 0;

			if ( type == MST_PLANAR || type == MST_TRIANGLE_SOUP ) {
				int base = (int)( sm.verts.size() / 3 );

				if ( fi < 0 || ni < 3 || fi > numIndexes - ni )
					continue;
				for ( int v = 0; v < nv; v++ ) {
					vec3_t p;

					for ( int k = 0; k < 3; k++ )
						p[k] = LittleFloat( verts[fv + v].xyz[k] );
					SM_AddVert( p );
				}
				for ( int t = 0; t + 2 < ni; t += 3 ) {
					int a = LittleLong( indexes[fi + t] ), b = LittleLong( indexes[fi + t + 1] ), c = LittleLong( indexes[fi + t + 2] );

					if ( a < 0 || b < 0 || c < 0 || a >= nv || b >= nv || c >= nv )
						continue;
					sm.tris.push_back( base + a ); sm.tris.push_back( base + b ); sm.tris.push_back( base + c );
					s.numTris++;
				}
			} else if ( type == MST_PATCH ) {
				int w = LittleLong( in->patchWidth ), ht = LittleLong( in->patchHeight );
				std::vector<mapVert_t> control( nv );

				if ( w < 3 || ht < 3 || w * ht != nv )
					continue;
				for ( int v = 0; v < nv; v++ ) {
					for ( int k = 0; k < 3; k++ )
						control[v].xyz[k] = LittleFloat( verts[fv + v].xyz[k] );
				}
				SM_AddPatch( control.data(), w, ht, &s );
			}

			if ( s.numTris ) {
				SM_SurfBounds( &s );
				sm.surfs.push_back( s );
				bm.numSurfs++;
			}
		}
		sm.bmodels.push_back( bm );
	}

	FS_FreeFile( buf );
	Com_DPrintf( "Shader manager: %s has %i surfaces, %i triangles\n", path, (int)sm.surfs.size(), (int)( sm.tris.size() / 3 ) );
}

static void SM_CheckMap( void ) {
	const char *info, *mapName;

	if ( cl.gameState.stringOffsets[CS_SERVERINFO] <= 0 && !cl.gameState.dataCount )
		return;
	info = cl.gameState.stringData + cl.gameState.stringOffsets[CS_SERVERINFO];
	mapName = Info_ValueForKey( info, "mapname" );
	if ( !mapName[0] )
		return;
	if ( !sm.mapLoaded || Q_stricmp( mapName, sm.mapName ) )
		SM_LoadMap( mapName );
}

/*
===============================================================================

TRACING

===============================================================================
*/

// distance along dir to the triangle, both sides count; -1 if missed
static float SM_RayTriangle( const vec3_t org, const vec3_t dir, const float *a, const float *b, const float *c ) {
	vec3_t e1, e2, p, t, q;
	float det, inv, u, v, d;

	VectorSubtract( b, a, e1 );
	VectorSubtract( c, a, e2 );
	CrossProduct( dir, e2, p );
	det = DotProduct( e1, p );
	if ( fabsf( det ) < 1e-8f )
		return -1.0f;
	inv = 1.0f / det;
	VectorSubtract( org, a, t );
	u = DotProduct( t, p ) * inv;
	if ( u < 0.0f || u > 1.0f )
		return -1.0f;
	CrossProduct( t, e1, q );
	v = DotProduct( dir, q ) * inv;
	if ( v < 0.0f || u + v > 1.0f )
		return -1.0f;
	d = DotProduct( e2, q ) * inv;
	return d > 0.0f ? d : -1.0f;
}

// does the ray reach the box before maxDist
static qboolean SM_RayBox( const vec3_t org, const vec3_t dir, const vec3_t mins, const vec3_t maxs, float maxDist ) {
	float tmin = 0.0f, tmax = maxDist;

	for ( int i = 0; i < 3; i++ ) {
		if ( fabsf( dir[i] ) < 1e-8f ) {
			if ( org[i] < mins[i] - 1.0f || org[i] > maxs[i] + 1.0f )
				return qfalse;
			continue;
		}
		float t1 = ( mins[i] - 1.0f - org[i] ) / dir[i], t2 = ( maxs[i] + 1.0f - org[i] ) / dir[i];

		if ( t1 > t2 ) {
			float tmp = t1;
			t1 = t2;
			t2 = tmp;
		}
		tmin = Q_max( tmin, t1 );
		tmax = Q_min( tmax, t2 );
		if ( tmin > tmax )
			return qfalse;
	}
	return qtrue;
}

static void SM_FxBox( const smFxGroup_t *g, vec3_t mins, vec3_t maxs );

// how far along the ray it enters the box; -1 if it misses or starts inside
static float SM_RayBoxEntry( const vec3_t org, const vec3_t dir, const vec3_t mins, const vec3_t maxs ) {
	float tmin = 0.0f, tmax = SM_TRACE_DIST;
	qboolean inside = qtrue;

	for ( int i = 0; i < 3; i++ ) {
		if ( org[i] < mins[i] || org[i] > maxs[i] )
			inside = qfalse;
		if ( fabsf( dir[i] ) < 1e-8f ) {
			if ( org[i] < mins[i] || org[i] > maxs[i] )
				return -1.0f;
			continue;
		}
		float t1 = ( mins[i] - org[i] ) / dir[i], t2 = ( maxs[i] - org[i] ) / dir[i];

		if ( t1 > t2 ) {
			float tmp = t1;
			t1 = t2;
			t2 = tmp;
		}
		tmin = Q_max( tmin, t1 );
		tmax = Q_min( tmax, t2 );
		if ( tmin > tmax )
			return -1.0f;
	}
	return inside ? -1.0f : tmin;
}

// the nearest surface of one map model, in that model's space; -1 if none
static int SM_TraceBModel( int model, const vec3_t org, const vec3_t dir, float *best ) {
	const smBModel_t *bm;
	int found = -1;

	if ( model < 0 || model >= (int)sm.bmodels.size() )
		return -1;
	bm = &sm.bmodels[model];
	for ( int i = bm->firstSurf; i < bm->firstSurf + bm->numSurfs; i++ ) {
		const smSurf_t *s = &sm.surfs[i];

		if ( !SM_RayBox( org, dir, s->mins, s->maxs, *best ) )
			continue;
		for ( int t = s->firstTri; t < s->firstTri + s->numTris; t++ ) {
			const int *tri = &sm.tris[t * 3];
			float d = SM_RayTriangle( org, dir, &sm.verts[tri[0] * 3], &sm.verts[tri[1] * 3], &sm.verts[tri[2] * 3] );

			if ( d > 0.0f && d < *best ) {
				*best = d;
				found = i;
			}
		}
	}
	return found;
}

// the ray in an entity's space; the axes may be scaled
static void SM_ToLocal( const refEntity_t *ent, const vec3_t org, const vec3_t dir, vec3_t lorg, vec3_t ldir ) {
	vec3_t delta;

	VectorSubtract( org, ent->origin, delta );
	for ( int i = 0; i < 3; i++ ) {
		float len2 = DotProduct( ent->axis[i], ent->axis[i] );

		if ( len2 < 1e-8f )
			len2 = 1.0f;
		lorg[i] = DotProduct( delta, ent->axis[i] ) / len2;
		ldir[i] = DotProduct( dir, ent->axis[i] ) / len2;
	}
}

static void SM_SetHit( smHit_t *hit, smHitKind_t kind, float dist, const char *shader, const char *model, const char *surface, const refEntity_t *ent, int surf ) {
	hit->kind = kind;
	hit->dist = dist;
	Q_strncpyz( hit->shader, shader ? shader : "", sizeof( hit->shader ) );
	Q_strncpyz( hit->model, model ? model : "", sizeof( hit->model ) );
	Q_strncpyz( hit->surface, surface ? surface : "", sizeof( hit->surface ) );
	hit->surf = surf;
	if ( ent )
		hit->ent = *ent;
	else
		memset( &hit->ent, 0, sizeof( hit->ent ) );
}

// an md3 or a brush entity
static void SM_TraceModelEntity( const refEntity_t *ent, const vec3_t org, const vec3_t dir, smHit_t *hit ) {
	const char *name;
	vec3_t lorg, ldir;
	float best;

	if ( ent->hModel <= 0 || ent->hModel >= SM_MAX_HANDLES || sm.modelNames[ent->hModel].empty() )
		return;
	name = sm.modelNames[ent->hModel].c_str();
	SM_ToLocal( ent, org, dir, lorg, ldir );

	// the local direction has the axis scale folded in, so distances stay in world units
	if ( name[0] == '*' ) {
		int model = atoi( name + 1 ), surf;

		if ( model <= 0 || model >= (int)sm.bmodels.size() )
			return;
		best = hit->dist;
		surf = SM_TraceBModel( model, lorg, ldir, &best );
		if ( surf >= 0 ) {
			const char *shader = SM_ShaderName( ent->customShader );

			SM_SetHit( hit, SM_HIT_BRUSH, best, shader ? shader : sm.bspShaders[sm.surfs[surf].shader].c_str(), name, NULL, ent, surf );
		}
		return;
	}

	smMd3_t *m = SM_GetMd3( ent->hModel );
	if ( !m )
		return;
	int frame = Com_Clampi( 0, m->numFrames - 1, ent->frame );
	for ( size_t s = 0; s < m->surfs.size(); s++ ) {
		const smMd3Surf_t *surf = &m->surfs[s];
		const float *xyz = &surf->xyz[(size_t)frame * surf->numVerts * 3];
		qboolean hitSurf = qfalse;

		best = hit->dist;
		for ( size_t t = 0; t + 2 < surf->tris.size(); t += 3 ) {
			float d = SM_RayTriangle( lorg, ldir, &xyz[surf->tris[t] * 3], &xyz[surf->tris[t + 1] * 3], &xyz[surf->tris[t + 2] * 3] );

			if ( d > 0.0f && d < best ) {
				best = d;
				hitSurf = qtrue;
			}
		}
		if ( !hitSurf )
			continue;

		// the way R_AddMD3Surfaces picks it
		const char *shader = SM_ShaderName( ent->customShader );
		if ( !shader && ent->customSkin )
			shader = SM_SkinShader( ent->customSkin, surf->name.c_str() );
		if ( !shader && !surf->shaders.empty() )
			shader = surf->shaders[ent->skinNum % surf->shaders.size()].c_str();
		SM_SetHit( hit, SM_HIT_MODEL, best, shader, name, surf->name.c_str(), ent, -1 );
	}
}

static void SM_TraceGhoul2Entity( const refEntity_t *ent, const vec3_t org, const vec3_t dir, smHit_t *hit ) {
	CGhoul2Info_v &ghoul2 = *(CGhoul2Info_v *)ent->ghoul2;
	CollisionRecord_t records[MAX_G2_COLLISIONS];
	vec3_t start, end;
	const CollisionRecord_t *nearest = NULL;

	VectorCopy( org, start );
	VectorMA( org, hit->dist, dir, end );
	memset( records, 0, sizeof( records ) );
	for ( int i = 0; i < MAX_G2_COLLISIONS; i++ )
		records[i].mEntityNum = -1;
	re->G2API_CollisionDetect( records, ghoul2, ent->angles, ent->origin, re->G2API_GetTime( cl.serverTime ), 0,
		start, end, (float *)ent->modelScale, G2VertSpaceClient, G2_FRONTFACE, 0, 0.0f );

	for ( int i = 0; i < MAX_G2_COLLISIONS; i++ ) {
		if ( records[i].mEntityNum == -1 )
			break;
		if ( !nearest || records[i].mDistance < nearest->mDistance )
			nearest = &records[i];
	}
	if ( !nearest || nearest->mDistance >= hit->dist || nearest->mModelIndex < 0 || nearest->mModelIndex >= re->G2API_Ghoul2Size( ghoul2 ) )
		return;

	// the way R_AddGhoulSurfaces picks it
	const char *file = re->G2API_GetModelName( ghoul2, nearest->mModelIndex );
	if ( !file || !file[0] )
		return;
	char fileName[MAX_QPATH];
	Q_strncpyz( fileName, file, sizeof( fileName ) );
	const smGlm_t *glm = SM_GetGlm( fileName );
	const char *surface = nearest->mSurfaceIndex >= 0 && nearest->mSurfaceIndex < (int)glm->surfaces.size() ? glm->surfaces[nearest->mSurfaceIndex].c_str() : NULL;
	const char *shader = SM_ShaderName( ent->customShader );
	qhandle_t customSkin = 0, ownSkin = 0, skin;

	if ( re->ext.G2API_GetSkins )
		re->ext.G2API_GetSkins( ghoul2, nearest->mModelIndex, &customSkin, &ownSkin );
	skin = customSkin ? customSkin : ent->customSkin ? ent->customSkin : ownSkin;
	if ( !shader && skin && surface )
		shader = SM_SkinShader( skin, surface );
	if ( !shader && surface )
		shader = glm->shaders[nearest->mSurfaceIndex].c_str();
	SM_SetHit( hit, SM_HIT_GHOUL2, nearest->mDistance, shader, fileName, surface, ent, -1 );
}

// the player's own model, seen from behind in third person
static qboolean SM_IsSelf( const refEntity_t *ent ) {
	vec3_t delta;

	if ( !cl.snap.valid )
		return qfalse;
	VectorSubtract( ent->origin, cl.snap.ps.origin, delta );
	return (qboolean)( VectorLength( delta ) < SM_SELF_RADIUS );
}

static void SM_Trace( const vec3_t org, const vec3_t dir, smHit_t *hit ) {
	int surf;

	memset( hit, 0, sizeof( *hit ) );
	hit->dist = SM_TRACE_DIST;
	hit->surf = -1;
	hit->fx = -1;

	SM_CheckMap();
	surf = SM_TraceBModel( 0, org, dir, &hit->dist );
	if ( surf >= 0 )
		SM_SetHit( hit, SM_HIT_WORLD, hit->dist, sm.bspShaders[sm.surfs[surf].shader].c_str(), NULL, NULL, NULL, surf );

	if ( sm.viewFrame == cls.framecount ) {
		for ( int i = 0; i < sm.numEntities; i++ ) {
			const refEntity_t *ent = &sm.entities[i];

			if ( ent->reType != RT_MODEL || ( ent->renderfx & ( RF_FIRST_PERSON | RF_DEPTHHACK | RF_THIRD_PERSON ) ) )
				continue;
			if ( ent->ghoul2 && re->G2API_HaveWeGhoul2Models( *(CGhoul2Info_v *)ent->ghoul2 ) ) {
				// the free camera can look at the player, the normal view only ever sees its back
				if ( sm.state != SM_OFF || !SM_IsSelf( ent ) )
					SM_TraceGhoul2Entity( ent, org, dir, hit );
			} else {
				SM_TraceModelEntity( ent, org, dir, hit );
			}
		}
	}

	// an effect is see-through: its box is found before what it sits on
	if ( sm.viewFrame == cls.framecount && !sm.hideEffects ) {
		for ( int i = 0; i < sm.numFxGroups; i++ ) {
			const smFxGroup_t *g = &sm.fxGroups[i];
			vec3_t mins, maxs;
			float d;

			SM_FxBox( g, mins, maxs );
			d = SM_RayBoxEntry( org, dir, mins, maxs );

			if ( d >= 0.0f && d < hit->dist ) {
				SM_SetHit( hit, SM_HIT_EFFECT, d, SM_ShaderName( g->shaders[0] ), NULL, NULL, NULL, -1 );
				hit->fx = i;
			}
		}
	}
	VectorMA( org, hit->dist, dir, hit->pos );
}

/*
===============================================================================

REMAPS

===============================================================================
*/

static void SM_SetStatus( const char *text ) {
	Q_strncpyz( sm.status, text, sizeof( sm.status ) );
}

static void SM_Send( const char *cmd ) {
	// opened from the storyteller, it goes in the story instead
	if ( CL_Storyteller_Capture( cmd ) ) {
		SM_SetStatus( va( "In the story: %s", cmd ) );
		return;
	}
	CL_AddReliableCommand( cmd, qfalse );
	sm.sentTime = cls.realtime;
	sm.reply[0] = '\0';
	Com_Printf( S_COLOR_GREEN "Shader manager: " S_COLOR_WHITE "%s\n", cmd );
	SM_SetStatus( va( "Sent: %s", cmd ) );
}

static qboolean SM_RemapAllowed( const char *from, const char *to );

// "rpshader remap"; record keeps what it was before, for undo
static void SM_SendRemap( const char *from, const char *to, qboolean record ) {
	const char *offset = sm.fields[SM_F_OFFSET], *previous;
	char cmd[MAX_STRING_CHARS];

	if ( !from[0] || !to[0] )
		return;
	if ( !SM_RemapAllowed( from, to ) ) {
		SM_SetStatus( S_COLOR_RED "A sky can only be remapped to another sky" );
		return;
	}
	if ( record ) {
		previous = SM_RemappedTo( from );
		sm.undoShader.push_back( from );
		sm.undoPrevious.push_back( previous ? previous : from );
		if ( (int)sm.undoShader.size() > SM_MAX_HISTORY ) {
			sm.undoShader.erase( sm.undoShader.begin() );
			sm.undoPrevious.erase( sm.undoPrevious.begin() );
		}
	}
	if ( offset[0] && Q_stricmp( from, to ) )
		Com_sprintf( cmd, sizeof( cmd ), "rpshader remap %s %s %s", from, to, offset );
	else
		Com_sprintf( cmd, sizeof( cmd ), "rpshader remap %s %s", from, to );
	SM_Send( cmd );
}

static void SM_Undo( void ) {
	std::string shader, previous;

	if ( sm.undoShader.empty() )
		return;
	shader = sm.undoShader.back();
	previous = sm.undoPrevious.back();
	sm.undoShader.pop_back();
	sm.undoPrevious.pop_back();
	SM_SendRemap( shader.c_str(), previous.c_str(), qfalse );
}

// the remap shown on this screen only, undone by putting back what the server has
static void SM_ClearLive( void ) {
	const char *target;

	if ( !sm.liveFrom[0] )
		return;
	target = SM_RemappedTo( sm.liveFrom );
	re->RemapShader( sm.liveFrom, target ? target : sm.liveFrom, NULL );
	sm.liveFrom[0] = sm.liveTo[0] = '\0';
}

static void SM_SetLive( const char *from, const char *to ) {
	if ( from && to && !Q_stricmp( from, sm.liveFrom ) && !Q_stricmp( to, sm.liveTo ) )
		return;
	SM_ClearLive();
	if ( !from || !to || !from[0] || !to[0] || !SM_RemapAllowed( from, to ) )
		return;
	re->RemapShader( from, to, sm.fields[SM_F_OFFSET][0] ? sm.fields[SM_F_OFFSET] : NULL );
	Q_strncpyz( sm.liveFrom, from, sizeof( sm.liveFrom ) );
	Q_strncpyz( sm.liveTo, to, sizeof( sm.liveTo ) );
}

// a line of "rpshader overlay list"'s reply: one that names a shader or starts with a number
// is an overlay; the others, its heading or that there are none, aren't
static void SM_OverlayListPrint( const char *text ) {
	char buf[MAX_STRING_CHARS], *line, *next;

	Q_strncpyz( buf, text, sizeof( buf ) );
	Q_StripColor( buf );
	for ( line = buf; line; line = next ) {
		char word[MAX_QPATH];
		const char *p;
		smOverlay_t o;
		int n = 0;

		next = strchr( line, '\n' );
		if ( next )
			*next++ = '\0';
		while ( *line == ' ' || *line == '\t' )
			line++;
		for ( int len = strlen( line ); len && ( line[len - 1] == ' ' || line[len - 1] == '\r' ); )
			line[--len] = '\0';
		if ( !line[0] )
			continue;
		o.line = line;
		o.index = -1;
		for ( p = line; *p; n++ ) {
			int len = 0;
			char *w = word;

			while ( *p == ' ' || *p == '\t' )
				p++;
			while ( *p && *p != ' ' && *p != '\t' ) {
				if ( len < (int)sizeof( word ) - 1 )
					word[len++] = *p;
				p++;
			}
			word[len] = '\0';
			// [1] #1 1: 1) 1. 1, "name",
			while ( *w == '[' || *w == '#' || *w == '(' || *w == '"' || *w == '\'' )
				w++;
			for ( len = strlen( w ); len && strchr( "]):.,\"'", w[len - 1] ); )
				w[--len] = '\0';
			if ( n == 0 && w[0] && ( Q_isanumber( w ) || ( w[0] == '-' && Q_isanumber( w + 1 ) ) ) && !strchr( w, '.' ) )
				o.index = atoi( w );
			else if ( o.shader.empty() && strchr( w, '/' ) )
				o.shader = w;
		}
		if ( o.index >= 0 || !o.shader.empty() )
			sm.overlays.push_back( o );
	}
}

// a print from the server soon after a command is taken as its answer
void CL_ShaderManager_ServerPrint( const char *text ) {
	char line[MAX_STRING_CHARS];
	int len;

	if ( sm.state == SM_OFF || !sm.sentTime || cls.realtime - sm.sentTime > SM_REPLY_MS )
		return;
	if ( sm.overlayListTime && cls.realtime - sm.overlayListTime < SM_REPLY_MS )
		SM_OverlayListPrint( text );
	Q_strncpyz( line, text, sizeof( line ) );
	Q_StripColor( line );
	len = strlen( line );
	while ( len && ( line[len - 1] == '\n' || line[len - 1] == '\r' || line[len - 1] == ' ' ) )
		line[--len] = '\0';
	if ( !line[0] )
		return;
	Q_strncpyz( sm.reply, line, sizeof( sm.reply ) );
	sm.replyTime = cls.realtime;
}

/*
===============================================================================

SHADER INDEX

===============================================================================
*/

static void SM_AddShaderName( const char *name ) {
	char key[MAX_QPATH];

	COM_StripExtension( name, key, sizeof( key ) );
	for ( char *p = key; *p; p++ ) {
		if ( *p == '\\' )
			*p = '/';
	}
	if ( key[0] )
		sm.shaderNames.push_back( key );
}

static void SM_AddImage( const char *name, void *ctx ) {
	SM_AddShaderName( name );
}

// the names in one .shader file
static void SM_ParseShaderFile( const char *path ) {
	char *buf;
	const char *p, *token;
	char name[MAX_QPATH], key[MAX_QPATH];
	int depth;
	qboolean sky;

	if ( FS_ReadFile( path, (void **)&buf ) <= 0 || !buf )
		return;
	p = buf;
	COM_BeginParseSession( path );
	while ( 1 ) {
		token = COM_ParseExt( &p, qtrue );
		if ( !token[0] )
			break;
		Q_strncpyz( name, token, sizeof( name ) );
		token = COM_ParseExt( &p, qtrue );
		if ( Q_stricmp( token, "{" ) )
			break;
		// skip the body, noting a skyParms among its own keywords
		sky = qfalse;
		for ( depth = 1; depth > 0; ) {
			token = COM_ParseExt( &p, qtrue );
			if ( !token[0] )
				break;
			if ( !Q_stricmp( token, "{" ) )
				depth++;
			else if ( !Q_stricmp( token, "}" ) )
				depth--;
			else if ( depth == 1 && !Q_stricmp( token, "skyParms" ) )
				sky = qtrue;
		}
		SM_AddShaderName( name );
		if ( sky ) {
			SM_ShaderKey( name, key, sizeof( key ) );
			sm.skyShaders.insert( key );
		}
	}
	FS_FreeFile( buf );
}

static bool SM_NameLess( const std::string &a, const std::string &b ) {
	return Q_stricmp( a.c_str(), b.c_str() ) < 0;
}

// every shader in shaders/*.shader (where JKA keeps them, as the renderer reads), and every image a shader can be made from
static void SM_BuildIndex( void ) {
	static const char *dirs[] = { "textures", "models", "gfx", "effects" };
	static const char *exts[] = { ".jpg", ".tga", ".png" };
	int start = Sys_Milliseconds(), numFiles, out = 0;
	char **files;

	sm.shaderNames.clear();
	sm.skyShaders.clear();
	files = FS_ListFiles( "shaders", ".shader", &numFiles );
	for ( int i = 0; i < numFiles; i++ )
		SM_ParseShaderFile( va( "shaders/%s", files[i] ) );
	FS_FreeFileList( files );
	for ( size_t d = 0; d < ARRAY_LEN( dirs ); d++ ) {
		for ( size_t e = 0; e < ARRAY_LEN( exts ); e++ )
			FS_ListFilesRecursive( dirs[d], exts[e], SM_AddImage, NULL );
	}

	std::sort( sm.shaderNames.begin(), sm.shaderNames.end(), SM_NameLess );
	for ( size_t i = 0; i < sm.shaderNames.size(); i++ ) {
		if ( !out || Q_stricmp( sm.shaderNames[out - 1].c_str(), sm.shaderNames[i].c_str() ) )
			sm.shaderNames[out++] = sm.shaderNames[i];
	}
	sm.shaderNames.resize( out );
	sm.indexed = qtrue;
	sm.shaderList.sel = sm.shaderList.scroll = 0;
	Com_Printf( "Shader manager: indexed %i shaders (%i ms)\n", out, Sys_Milliseconds() - start );
}

static qboolean SM_ContainsNoCase( const char *haystack, const char *needle ) {
	int len = strlen( needle );

	if ( !len )
		return qtrue;
	for ( ; *haystack; haystack++ ) {
		if ( !Q_stricmpn( haystack, needle, len ) )
			return qtrue;
	}
	return qfalse;
}

// a sky's surfaces are drawn by the sky code, which needs a sky shader: remapping one to anything
// else crashes clients without the renderer fix, so skies are only offered other skies
static qboolean SM_IsSky( const char *name ) {
	char key[MAX_QPATH];

	if ( !name || !name[0] )
		return qfalse;
	SM_ShaderKey( name, key, sizeof( key ) );
	return (qboolean)( sm.skyShaders.find( key ) != sm.skyShaders.end() );
}

static qboolean SM_RemapAllowed( const char *from, const char *to ) {
	return (qboolean)( !SM_IsSky( from ) || SM_IsSky( to ) || !Q_stricmp( from, to ) );
}

static const char *SM_SelectedOriginal( void );

static void SM_RebuildShaderView( void ) {
	static const char *prefixes[] = { "", "textures/", "models/", "gfx/", "" };
	const char *search = sm.fields[SM_F_SEARCH], *prefix = prefixes[sm.filter];
	int prefixLen = strlen( prefix ), oldSel = -1;

	if ( sm.shaderList.sel >= 0 && sm.shaderList.sel < (int)sm.shaderView.size() )
		oldSel = sm.shaderView[sm.shaderList.sel];
	sm.viewSky = (qboolean)( sm.tab != SM_TAB_MINE && SM_IsSky( SM_SelectedOriginal() ) );
	sm.shaderView.clear();
	for ( size_t i = 0; i < sm.shaderNames.size(); i++ ) {
		const char *name = sm.shaderNames[i].c_str();

		if ( prefixLen && Q_stricmpn( name, prefix, prefixLen ) )
			continue;
		if ( sm.viewSky && !SM_IsSky( name ) )
			continue;
		if ( sm.filter == SM_FILTER_FAVES && !CL_Fav_Has( SM_FAV_LIST, name ) )
			continue;
		if ( SM_ContainsNoCase( name, search ) )
			sm.shaderView.push_back( (int)i );
	}
	// keep the highlighted shader if it is still there
	sm.shaderList.sel = 0;
	for ( size_t i = 0; i < sm.shaderView.size(); i++ ) {
		if ( sm.shaderView[i] == oldSel ) {
			sm.shaderList.sel = (int)i;
			break;
		}
	}
}

// the overlays tab's list: every shader, by its own search
static void SM_RebuildOverlayView( void ) {
	const char *search = sm.fields[SM_F_OV_SEARCH];

	sm.overlayView.clear();
	for ( size_t i = 0; i < sm.shaderNames.size(); i++ ) {
		if ( SM_ContainsNoCase( sm.shaderNames[i].c_str(), search ) )
			sm.overlayView.push_back( (int)i );
	}
	sm.overlayShaders.sel = Com_Clampi( 0, Q_max( (int)sm.overlayView.size() - 1, 0 ), sm.overlayShaders.sel );
}

static const char *SM_OverlayShader( void ) {
	if ( sm.overlayShaders.sel < 0 || sm.overlayShaders.sel >= (int)sm.overlayView.size() )
		return NULL;
	return sm.shaderNames[sm.overlayView[sm.overlayShaders.sel]].c_str();
}

// the shader picked in the list, or with a path typed in, that path
static const char *SM_HighlightedShader( void ) {
	if ( sm.typingPath )
		return sm.pathShader[0] ? sm.pathShader : NULL;
	if ( sm.shaderList.sel < 0 || sm.shaderList.sel >= (int)sm.shaderView.size() )
		return NULL;
	return sm.shaderNames[sm.shaderView[sm.shaderList.sel]].c_str();
}

// the path box as a shader name: forward slashes, no extension, as the renderer names them
static void SM_PathChanged( void ) {
	COM_StripExtension( sm.fields[SM_F_PATH], sm.pathShader, sizeof( sm.pathShader ) );
	for ( char *p = sm.pathShader; *p; p++ ) {
		if ( *p == '\\' )
			*p = '/';
	}
}

// the typed path is one of the shaders or images the game has
static qboolean SM_PathKnown( void ) {
	std::vector<std::string>::const_iterator it;

	if ( !sm.pathShader[0] )
		return qfalse;
	it = std::lower_bound( sm.shaderNames.begin(), sm.shaderNames.end(), std::string( sm.pathShader ), SM_NameLess );
	return (qboolean)( it != sm.shaderNames.end() && !Q_stricmp( it->c_str(), sm.pathShader ) );
}

// where typing goes with no box clicked: the path box while typing a path, else the search
static smField_t SM_TypingField( void ) {
	if ( sm.focus != SM_F_NONE )
		return sm.focus;
	if ( sm.tab == SM_TAB_OVERLAYS )
		return SM_F_OV_SEARCH;
	return sm.typingPath && sm.tab == SM_TAB_OBJECT ? SM_F_PATH : SM_F_SEARCH;
}

// the shaders on what was clicked as listed: all, or with selFaves only the starred ones
static int SM_SelCount( void ) {
	int count = 0;

	if ( !sm.selFaves )
		return (int)sm.selShaders.size();
	for ( size_t i = 0; i < sm.selShaders.size(); i++ ) {
		if ( CL_Fav_Has( SM_FAV_LIST, sm.selShaders[i].c_str() ) )
			count++;
	}
	return count;
}

// the shader on a row of that list, NULL past its end
static const char *SM_SelShader( int row ) {
	if ( row < 0 )
		return NULL;
	for ( size_t i = 0; i < sm.selShaders.size(); i++ ) {
		if ( sm.selFaves && !CL_Fav_Has( SM_FAV_LIST, sm.selShaders[i].c_str() ) )
			continue;
		if ( !row-- )
			return sm.selShaders[i].c_str();
	}
	return NULL;
}

static const char *SM_SelectedOriginal( void ) {
	if ( !sm.haveSel )
		return NULL;
	return SM_SelShader( sm.selList.sel );
}

// a 2D handle to preview a shader by, registered the first time it is shown
static qhandle_t SM_PreviewHandle( const char *name ) {
	char key[MAX_QPATH];
	std::map<std::string, qhandle_t>::iterator it;
	qhandle_t h;

	SM_ShaderKey( name, key, sizeof( key ) );
	it = sm.previews.find( key );
	if ( it != sm.previews.end() )
		return it->second;
	h = re->RegisterShaderNoMip( name );
	sm.previews[key] = h;
	return h;
}

/*
===============================================================================

SELECTION

===============================================================================
*/

static void SM_AddSelShader( const char *shader ) {
	if ( !shader || !shader[0] || shader[0] == '*' )
		return;	// "*off" in a skin
	for ( size_t i = 0; i < sm.selShaders.size(); i++ ) {
		if ( !Q_stricmp( sm.selShaders[i].c_str(), shader ) )
			return;
	}
	sm.selShaders.push_back( shader );
}

// every shader on what was hit, the hit one selected
static void SM_Select( const smHit_t *hit ) {
	const refEntity_t *ent = &hit->ent;
	const char *custom = SM_ShaderName( ent->customShader );

	sm.haveSel = qtrue;
	sm.selKind = hit->kind;
	sm.selShaders.clear();
	sm.selList.sel = sm.selList.scroll = 0;
	sm.selSurf = hit->surf;
	sm.selBModel[0] = '\0';
	sm.selFx = qfalse;

	switch ( hit->kind ) {
	case SM_HIT_WORLD:
		Q_strncpyz( sm.selWhat, "Map surface", sizeof( sm.selWhat ) );
		SM_AddSelShader( hit->shader );
		break;

	case SM_HIT_BRUSH: {
		int model = atoi( hit->model + 1 );

		Com_sprintf( sm.selWhat, sizeof( sm.selWhat ), "Brush entity %s", hit->model );
		Q_strncpyz( sm.selBModel, hit->model, sizeof( sm.selBModel ) );
		SM_AddSelShader( hit->shader );
		if ( !custom && model > 0 && model < (int)sm.bmodels.size() ) {
			const smBModel_t *bm = &sm.bmodels[model];

			for ( int i = bm->firstSurf; i < bm->firstSurf + bm->numSurfs; i++ )
				SM_AddSelShader( sm.bspShaders[sm.surfs[i].shader].c_str() );
		}
		break;
	}

	case SM_HIT_MODEL: {
		smMd3_t *m = SM_GetMd3( ent->hModel );

		Q_strncpyz( sm.selWhat, hit->model, sizeof( sm.selWhat ) );
		SM_AddSelShader( hit->shader );
		for ( size_t s = 0; m && !custom && s < m->surfs.size(); s++ ) {
			const smMd3Surf_t *surf = &m->surfs[s];
			const char *shader = ent->customSkin ? SM_SkinShader( ent->customSkin, surf->name.c_str() ) : NULL;

			if ( !shader && !surf->shaders.empty() )
				shader = surf->shaders[ent->skinNum % surf->shaders.size()].c_str();
			SM_AddSelShader( shader );
		}
		break;
	}

	case SM_HIT_GHOUL2: {
		CGhoul2Info_v &ghoul2 = *(CGhoul2Info_v *)ent->ghoul2;
		int numModels = re->G2API_Ghoul2Size( ghoul2 );

		Q_strncpyz( sm.selWhat, hit->model, sizeof( sm.selWhat ) );
		SM_AddSelShader( hit->shader );
		for ( int m = 0; !custom && m < numModels; m++ ) {
			const char *file = re->G2API_GetModelName( ghoul2, m );
			qhandle_t customSkin = 0, ownSkin = 0, skin;
			char fileName[MAX_QPATH];

			if ( !file || !file[0] )
				continue;
			Q_strncpyz( fileName, file, sizeof( fileName ) );
			const smGlm_t *glm = SM_GetGlm( fileName );
			if ( re->ext.G2API_GetSkins )
				re->ext.G2API_GetSkins( ghoul2, m, &customSkin, &ownSkin );
			skin = customSkin ? customSkin : ent->customSkin ? ent->customSkin : ownSkin;
			for ( size_t s = 0; s < glm->surfaces.size(); s++ ) {
				const char *surface = glm->surfaces[s].c_str(), *shader = NULL;
				int status = re->G2API_GetSurfaceRenderStatus( ghoul2, m, surface );

				if ( status < 0 || ( status & ( G2SURFACEFLAG_OFF | G2SURFACEFLAG_NODESCENDANTS ) ) )
					continue;
				if ( skin )
					shader = SM_SkinShader( skin, surface );
				if ( !shader )
					shader = glm->shaders[s].c_str();
				SM_AddSelShader( shader );
			}
		}
		break;
	}

	case SM_HIT_EFFECT: {
		const smFxGroup_t *g;

		if ( hit->fx < 0 || hit->fx >= sm.numFxGroups ) {
			sm.haveSel = qfalse;
			return;
		}
		g = &sm.fxGroups[hit->fx];
		for ( int i = 0; i < g->numShaders; i++ )
			SM_AddSelShader( SM_ShaderName( g->shaders[i] ) );
		Com_sprintf( sm.selWhat, sizeof( sm.selWhat ), "Effect, %i shader%s", (int)sm.selShaders.size(), sm.selShaders.size() == 1 ? "" : "s" );
		sm.selFx = qtrue;
		SM_FxBox( g, sm.selFxMins, sm.selFxMaxs );
		break;
	}

	default:
		sm.haveSel = qfalse;
		return;
	}
	if ( custom )
		Q_strcat( sm.selWhat, sizeof( sm.selWhat ), "  (forced shader)" );
}

// a shader from the remaps tab, edited without an object
static void SM_SelectShader( const char *shader ) {
	sm.haveSel = qtrue;
	sm.selKind = SM_HIT_NONE;
	sm.selShaders.clear();
	sm.selShaders.push_back( shader );
	sm.selList.sel = sm.selList.scroll = 0;
	sm.selFaves = qfalse;	// the one asked for shows, starred or not
	sm.selSurf = -1;
	sm.selBModel[0] = '\0';
	sm.selFx = qfalse;
	Q_strncpyz( sm.selWhat, "From the remap list", sizeof( sm.selWhat ) );
}

/*
===============================================================================

STATE CHANGES

===============================================================================
*/

static void SM_ReleaseKeys( void ) {
	memset( sm.held, 0, sizeof( sm.held ) );
	sm.looking = qfalse;
	sm.drag = NULL;
}

static void SM_EnterCamera( void ) {
	SM_ClearLive();
	sm.state = SM_CAMERA;
	sm.focus = SM_F_NONE;
	SM_ReleaseKeys();
}

static void SM_BuildPlayerShaders( void );
static void SM_RebuildMineView( void );

static void SM_EnterEdit( smTab_t tab ) {
	if ( !sm.indexed )
		SM_BuildIndex();
	if ( !sm.playerIndexed ) {
		SM_BuildPlayerShaders();
		SM_RebuildMineView();
	}
	sm.state = SM_EDIT;
	sm.tab = tab;
	sm.focus = SM_F_NONE;
	sm.cursorX = SM_PANEL_X * 0.5f;
	sm.cursorY = SCREEN_HEIGHT * 0.5f;
	sm.click = qfalse;
	sm.wheel = 0;
	SM_ReleaseKeys();
	SM_RebuildShaderView();
	SM_RebuildOverlayView();
}

static void SM_Open( void ) {
	if ( cls.state != CA_ACTIVE || !cls.cgameStarted ) {
		Com_Printf( "Shader manager: join a server first\n" );
		return;
	}
	// for RPMod's admins: whoever else can't run what it makes
	if ( !CL_RpAdmin_Allow( "Shader manager", "shadermanager" ) )
		return;
	CL_ModelManager_Close();
	CL_NpcManager_Close();
	CL_EffectManager_Close();
	CL_MapEditor_Close();
	CL_Storyteller_Close();
	CL_RpCinematic_EditorClose();
	CL_SoundManager_Close();

	sm.font = re->RegisterFont( "arialnb" );
	if ( !sm.font )
		sm.font = cls.menuFont;
	if ( sm.haveView ) {
		VectorCopy( sm.viewOrg, sm.camOrg );
		vectoangles( sm.viewAxis[0], sm.camAng );
	} else {
		VectorCopy( cl.snap.ps.origin, sm.camOrg );
		sm.camOrg[2] += cl.snap.ps.viewheight;
		VectorCopy( cl.viewangles, sm.camAng );
	}
	sm.camAng[ROLL] = 0;
	if ( sm.camAng[PITCH] > 180.0f )
		sm.camAng[PITCH] -= 360.0f;
	sm.lastFrameTime = cls.realtime;
	SM_EnterCamera();
	Key_SetCatcher( Key_GetCatcher() | KEYCATCH_SHADERMANAGER );
}

void CL_ShaderManager_Close( void ) {
	if ( sm.state == SM_OFF )
		return;
	SM_ClearLive();
	SM_SetLiveMine( NULL, NULL );
	sm.state = SM_OFF;
	SM_ReleaseKeys();
	Key_SetCatcher( Key_GetCatcher() & ~KEYCATCH_SHADERMANAGER );
}

qboolean CL_ShaderManager_Active( void ) {
	return (qboolean)( sm.state != SM_OFF );
}

// opened from a storyteller step: on the world for a remap, else on its tab (an smTab_t)
void CL_ShaderManager_OpenForStory( int tab ) {
	SM_Open();
	if ( sm.state == SM_OFF )
		return;
	if ( tab > SM_TAB_OBJECT && tab < SM_NUM_TABS )
		SM_EnterEdit( (smTab_t)tab );
	SM_SetStatus( tab == SM_TAB_SKY ? S_COLOR_GREEN "For the story: Share with all puts the sky look in it as a step"
		: S_COLOR_GREEN "For the story: what you send here goes in it as steps, not to the server" );
}

/*
===============================================================================

INPUT

===============================================================================
*/

// binds don't run while the manager holds the keys, so honour the user's toggle bind here
static qboolean SM_IsToggleKey( int key ) {
	const char *binding;

	// a printable key belongs to the search box while editing
	if ( sm.state == SM_EDIT && !sm.looking && key >= A_SPACE && key <= A_TILDE )
		return qfalse;
	binding = Key_GetBinding( key );
	return (qboolean)( VALIDSTRING( binding ) && !Q_stricmp( binding, SM_TOGGLE_CMD ) );
}

static void SM_ListMove( smList_t *list, int count, int visible, int delta ) {
	list->sel = Com_Clampi( 0, Q_max( count - 1, 0 ), list->sel + delta );
	if ( list->sel < list->scroll )
		list->scroll = list->sel;
	else if ( list->sel >= list->scroll + visible )
		list->scroll = list->sel - visible + 1;
}

static int SM_ShaderRows( void ) {
	return sm.showPreview ? 11 : 17;
}

static void SM_AddOverlay( void );
static void SM_MineEnter( void );

static void SM_FieldChanged( smField_t f ) {
	if ( f == SM_F_SEARCH ) {
		sm.shaderList.scroll = 0;
		SM_RebuildShaderView();
	} else if ( f == SM_F_PATH ) {
		SM_PathChanged();
	} else if ( f == SM_F_OV_SEARCH ) {
		sm.overlayShaders.scroll = 0;
		SM_RebuildOverlayView();
	}
}

static qboolean SM_AllowChar( int ch ) {
	const smField_t f = SM_TypingField();

	if ( ch < ' ' || ch > '~' )
		return qfalse;
	if ( ( f == SM_F_OFFSET || ( f >= SM_F_OV_FADE && f <= SM_F_OV_H ) ) && !( isdigit( ch ) || ch == '.' || ch == '-' ) )
		return qfalse;
	// goes on the command line as one word
	if ( f == SM_F_PATH && ( ch == ' ' || ch == '"' || ch == ';' ) )
		return qfalse;
	return qtrue;
}

static void SM_EditKey( int key ) {
	smList_t *list;
	int count, visible, delta;
	smField_t f = SM_TypingField();

	switch ( key ) {
	case A_MOUSE1:
		sm.click = qtrue;
		sm.clickX = sm.cursorX;
		sm.clickY = sm.cursorY;
		return;
	case A_MOUSE2:
		sm.looking = qtrue;
		return;
	case A_MWHEELUP:
		sm.wheel -= 3;
		return;
	case A_MWHEELDOWN:
		sm.wheel += 3;
		return;
	case A_TAB:
		sm.tab = (smTab_t)( ( sm.tab + 1 ) % SM_NUM_TABS );
		sm.focus = SM_F_NONE;
		return;
	case A_ENTER:
	case A_KP_ENTER:
		if ( sm.tab == SM_TAB_OBJECT && SM_SelectedOriginal() && SM_HighlightedShader() )
			SM_SendRemap( SM_SelectedOriginal(), SM_HighlightedShader(), qtrue );
		else if ( sm.tab == SM_TAB_OVERLAYS )
			SM_AddOverlay();
		else if ( sm.tab == SM_TAB_MINE )
			SM_MineEnter();
		return;
	default:
		break;
	}

	// a box clicked takes the editing keys; typed in without a click, the arrows, home and end
	// are the lists' and delete empties it
	if ( sm.focus != SM_F_NONE || !( key == A_CURSOR_LEFT || key == A_CURSOR_RIGHT || key == A_CURSOR_UP
		|| key == A_CURSOR_DOWN || key == A_HOME || key == A_END || key == A_DELETE ) ) {
		const int edit = WG_EditKey( sm.fields[f], SM_FIELD_LEN, key, SM_AllowChar );

		if ( edit & WG_EDIT_CHANGED )
			SM_FieldChanged( f );
		if ( edit )
			return;
	} else if ( key == A_DELETE ) {
		sm.fields[f][0] = '\0';
		SM_FieldChanged( f );
		return;
	}

	if ( sm.tab == SM_TAB_OBJECT || sm.tab == SM_TAB_MINE ) {
		list = &sm.shaderList;
		count = (int)sm.shaderView.size();
		visible = sm.tab == SM_TAB_MINE ? ( sm.showPreview ? 8 : 14 ) : SM_ShaderRows();
	} else if ( sm.tab == SM_TAB_REMAPS ) {
		list = &sm.remapList;
		count = (int)sm.remapFrom.size();
		visible = 26;
	} else if ( sm.tab == SM_TAB_OVERLAYS ) {
		list = &sm.overlayShaders;
		count = (int)sm.overlayView.size();
		visible = SM_OVERLAY_ROWS;
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
	SM_ListMove( list, count, visible, delta );
}

static void SM_SelectUnderRay( void );

void CL_ShaderManager_KeyEvent( int key, qboolean down ) {
	if ( sm.state == SM_OFF )
		return;

	if ( key >= 0 && key < MAX_KEYS )
		sm.held[key] = down;
	if ( !down ) {
		if ( key == A_MOUSE2 )
			sm.looking = qfalse;
		if ( key == A_MOUSE1 )
			sm.drag = NULL;
		return;
	}

	if ( SM_IsToggleKey( key ) ) {
		CL_ShaderManager_Close();
		return;
	}

	if ( sm.state == SM_CAMERA ) {
		if ( key == A_MOUSE1 )
			sm.click = qtrue;	// picked in Draw, where this frame's trace is
		else if ( key == A_TAB )
			SM_EnterEdit( SM_TAB_REMAPS );
		else if ( key == A_CAP_F || key == A_LOW_F )
			sm.hideEffects = (qboolean)!sm.hideEffects;
		return;
	}
	if ( sm.looking && key != A_MOUSE1 )
		return;	// flying keys
	SM_EditKey( key );
}

void CL_ShaderManager_CharEvent( int ch ) {
	smField_t f;

	if ( sm.state != SM_EDIT || sm.looking )
		return;
	f = SM_TypingField();
	if ( WG_EditChar( sm.fields[f], SM_FIELD_LEN, ch, SM_AllowChar ) )
		SM_FieldChanged( f );
}

void CL_ShaderManager_MouseEvent( int dx, int dy ) {
	if ( sm.state == SM_CAMERA || ( sm.state == SM_EDIT && sm.looking ) ) {
		sm.camAng[YAW] -= dx * cl_sensitivity->value * m_yaw->value;
		sm.camAng[PITCH] = Com_Clamp( -89.0f, 89.0f, sm.camAng[PITCH] + dy * cl_sensitivity->value * m_pitch->value );
	} else if ( sm.state == SM_EDIT ) {
		sm.cursorX = Com_Clamp( 0, SCREEN_WIDTH, sm.cursorX + dx );
		sm.cursorY = Com_Clamp( 0, SCREEN_HEIGHT, sm.cursorY + dy );
	}
}

// escape backs out one level: a text box, the panel, then the manager
void CL_ShaderManager_Escape( void ) {
	if ( sm.state == SM_EDIT && sm.focus != SM_F_NONE )
		sm.focus = SM_F_NONE;
	else if ( sm.state == SM_EDIT )
		SM_EnterCamera();
	else
		CL_ShaderManager_Close();
}

/*
===============================================================================

CAMERA AND SCENE

===============================================================================
*/

static qboolean SM_ShiftDown( void ) {
	return (qboolean)( sm.held[A_SHIFT] || sm.held[A_SHIFT2] );
}

static qboolean SM_CtrlDown( void ) {
	return (qboolean)( sm.held[A_CTRL] || sm.held[A_CTRL2] );
}

// called every frame before the cgame draws
void CL_ShaderManager_Frame( void ) {
	vec3_t forward, right, move;
	float dt, speed;

	if ( sm.state == SM_OFF )
		return;
	if ( !( Key_GetCatcher() & KEYCATCH_SHADERMANAGER ) ) {
		// something else cleared the catchers
		SM_ClearLive();
		SM_SetLiveMine( NULL, NULL );
		sm.state = SM_OFF;
		SM_ReleaseKeys();
		return;
	}

	dt = Com_Clamp( 0.0f, 0.1f, ( cls.realtime - sm.lastFrameTime ) / 1000.0f );
	sm.lastFrameTime = cls.realtime;
	if ( sm.state == SM_EDIT && !sm.looking )
		return;

	AngleVectors( sm.camAng, forward, right, NULL );
	VectorClear( move );
	if ( sm.held[A_CAP_W] )		VectorAdd( move, forward, move );
	if ( sm.held[A_CAP_S] )		VectorSubtract( move, forward, move );
	if ( sm.held[A_CAP_D] )		VectorAdd( move, right, move );
	if ( sm.held[A_CAP_A] )		VectorSubtract( move, right, move );
	if ( sm.held[A_SPACE] )		move[2] += 1.0f;
	if ( sm.held[A_CAP_C] )		move[2] -= 1.0f;
	if ( VectorNormalize( move ) > 0.0f ) {
		speed = SM_FLY_SPEED;
		if ( SM_ShiftDown() )
			speed *= 3.0f;
		if ( SM_CtrlDown() )
			speed *= 0.25f;
		VectorMA( sm.camOrg, speed * dt, move, sm.camOrg );
	}
}

// every entity cgame adds; kept for the scene that follows
void CL_ShaderManager_AddEntity( const refEntity_t *ent ) {
	if ( sm.state == SM_OFF && !cl_shaderCrosshair->integer )
		return;
	if ( sm.numPending < SM_MAX_ENTITIES )
		sm.pending[sm.numPending++] = *ent;
}

// a piece of an effect the effects system drew; radius is the piece's size
void CL_ShaderManager_AddEffect( qhandle_t shader, const vec3_t mins, const vec3_t maxs, float radius ) {
	smFxBit_t *bit;
	float core;

	if ( !sm.initialized || ( sm.state == SM_OFF && !cl_shaderCrosshair->integer ) )
		return;
	if ( shader <= 0 || sm.numFxPending >= SM_MAX_FX_BITS )
		return;
	core = Com_Clamp( 1.0f, SM_FX_CORE, radius );
	radius = Com_Clamp( 1.0f, SM_FX_BOX, radius * 0.5f );
	bit = &sm.fxPending[sm.numFxPending++];
	for ( int i = 0; i < 3; i++ ) {
		bit->coreMins[i] = mins[i] - core;
		bit->coreMaxs[i] = maxs[i] + core;
		bit->mins[i] = mins[i] - radius;
		bit->maxs[i] = maxs[i] + radius;
	}
	bit->shader = shader;
}

static qboolean SM_BoxesTouch( const vec3_t amins, const vec3_t amaxs, const vec3_t bmins, const vec3_t bmaxs ) {
	for ( int i = 0; i < 3; i++ ) {
		if ( amins[i] > bmaxs[i] + SM_FX_GAP || bmins[i] > amaxs[i] + SM_FX_GAP )
			return qfalse;
	}
	return qtrue;
}

static void SM_AddGroupShader( smFxGroup_t *g, qhandle_t shader ) {
	for ( int i = 0; i < g->numShaders; i++ ) {
		if ( g->shaders[i] == shader )
			return;
	}
	if ( g->numShaders < SM_MAX_FX_SHADERS )
		g->shaders[g->numShaders++] = shader;
}

static void SM_MergeGroup( smFxGroup_t *into, const smFxGroup_t *from ) {
	AddPointToBounds( from->coreMins, into->coreMins, into->coreMaxs );
	AddPointToBounds( from->coreMaxs, into->coreMins, into->coreMaxs );
	AddPointToBounds( from->mins, into->mins, into->maxs );
	AddPointToBounds( from->maxs, into->mins, into->maxs );
	into->numBits += from->numBits;
	for ( int i = 0; i < from->numShaders; i++ )
		SM_AddGroupShader( into, from->shaders[i] );
}

// the box an effect covers on screen
static void SM_FxBox( const smFxGroup_t *g, vec3_t mins, vec3_t maxs ) {
	VectorCopy( g->prevMins, mins );
	VectorCopy( g->prevMaxs, maxs );
	if ( g->mins[0] <= g->maxs[0] ) {
		AddPointToBounds( g->mins, mins, maxs );
		AddPointToBounds( g->maxs, mins, maxs );
	}
}

// this frame's groups into the effects kept from the frames before
static void SM_KeepEffects( void ) {
	int now = cls.realtime;

	for ( int i = 0; i < sm.numFxGroups; i++ ) {
		smFxGroup_t *g = &sm.fxGroups[i];

		if ( now - g->windowStart >= SM_FX_WINDOW ) {
			if ( g->mins[0] <= g->maxs[0] ) {
				VectorCopy( g->mins, g->prevMins );
				VectorCopy( g->maxs, g->prevMaxs );
			}
			ClearBounds( g->mins, g->maxs );
			g->windowStart = now;
		}
	}
	for ( int f = 0; f < sm.numFxFrame; f++ ) {
		const smFxGroup_t *in = &sm.fxFrame[f];
		smFxGroup_t *g = NULL;

		for ( int i = 0; i < sm.numFxGroups && !g; i++ ) {
			vec3_t mins, maxs;

			SM_FxBox( &sm.fxGroups[i], mins, maxs );
			if ( SM_BoxesTouch( mins, maxs, in->coreMins, in->coreMaxs ) )
				g = &sm.fxGroups[i];
		}
		if ( !g ) {
			if ( sm.numFxGroups >= SM_MAX_FX_GROUPS )
				continue;
			g = &sm.fxGroups[sm.numFxGroups++];
			*g = *in;
			VectorCopy( in->mins, g->prevMins );
			VectorCopy( in->maxs, g->prevMaxs );
			g->windowStart = now;
		} else {
			AddPointToBounds( in->coreMins, g->coreMins, g->coreMaxs );
			AddPointToBounds( in->coreMaxs, g->coreMins, g->coreMaxs );
			AddPointToBounds( in->mins, g->mins, g->maxs );
			AddPointToBounds( in->maxs, g->mins, g->maxs );
			for ( int i = 0; i < in->numShaders; i++ )
				SM_AddGroupShader( g, in->shaders[i] );
			g->numBits = in->numBits;
		}
		g->lastSeen = now;
	}
	// gone for a while: the effect ended
	for ( int i = 0; i < sm.numFxGroups; ) {
		if ( now - sm.fxGroups[i].lastSeen > SM_FX_WINDOW * 2 )
			sm.fxGroups[i] = sm.fxGroups[--sm.numFxGroups];
		else
			i++;
	}
}

// this frame's pieces, one group per effect
static void SM_GroupEffects( const vec3_t viewOrg ) {
	sm.numFxFrame = 0;
	for ( int b = 0; b < sm.numFxPending; b++ ) {
		const smFxBit_t *bit = &sm.fxPending[b];
		smFxGroup_t piece, *g = NULL;

		// screen flashes are drawn on the camera
		if ( viewOrg[0] > bit->mins[0] && viewOrg[0] < bit->maxs[0] && viewOrg[1] > bit->mins[1] && viewOrg[1] < bit->maxs[1]
			&& viewOrg[2] > bit->mins[2] && viewOrg[2] < bit->maxs[2] )
			continue;
		VectorCopy( bit->coreMins, piece.coreMins );
		VectorCopy( bit->coreMaxs, piece.coreMaxs );
		VectorCopy( bit->mins, piece.mins );
		VectorCopy( bit->maxs, piece.maxs );
		piece.numBits = 1;
		piece.numShaders = 1;
		piece.shaders[0] = bit->shader;
		for ( int i = 0; i < sm.numFxFrame && !g; i++ ) {
			if ( SM_BoxesTouch( sm.fxFrame[i].coreMins, sm.fxFrame[i].coreMaxs, piece.coreMins, piece.coreMaxs ) )
				g = &sm.fxFrame[i];
		}
		if ( g )
			SM_MergeGroup( g, &piece );
		else if ( sm.numFxFrame < SM_MAX_FX_GROUPS )
			sm.fxFrame[sm.numFxFrame++] = piece;
	}

	// groups that grew into each other are one effect
	for ( int i = 0; i < sm.numFxFrame; i++ ) {
		for ( int j = i + 1; j < sm.numFxFrame; ) {
			if ( SM_BoxesTouch( sm.fxFrame[i].coreMins, sm.fxFrame[i].coreMaxs, sm.fxFrame[j].coreMins, sm.fxFrame[j].coreMaxs ) ) {
				SM_MergeGroup( &sm.fxFrame[i], &sm.fxFrame[j] );
				sm.fxFrame[j] = sm.fxFrame[--sm.numFxFrame];
				j = i + 1;
			} else {
				j++;
			}
		}
	}
	SM_KeepEffects();
}

// the free camera, for the effects system to cull against
qboolean CL_ShaderManager_Camera( vec3_t origin, vec3_t angles ) {
	if ( sm.state == SM_OFF )
		return qfalse;
	VectorCopy( sm.camOrg, origin );
	VectorCopy( sm.camAng, angles );
	return qtrue;
}

// the view weapon would float where the player stands
qboolean CL_ShaderManager_FilterEntity( const refEntity_t *ent ) {
	return (qboolean)( sm.state != SM_OFF && ( ent->renderfx & RF_FIRST_PERSON ) );
}

static void SM_KeepView( const refdef_t *view, float x, float w ) {
	sm.haveView = qtrue;
	sm.viewFrame = cls.framecount;
	VectorCopy( view->vieworg, sm.viewOrg );
	VectorCopy( view->viewaxis[0], sm.viewAxis[0] );
	VectorCopy( view->viewaxis[1], sm.viewAxis[1] );
	VectorCopy( view->viewaxis[2], sm.viewAxis[2] );
	sm.viewFovX = view->fov_x;
	sm.viewFovY = view->fov_y;
	sm.viewX = x;
	sm.viewW = w;
	memcpy( sm.entities, sm.pending, sm.numPending * sizeof( sm.pending[0] ) );
	sm.numEntities = sm.numPending;
	SM_GroupEffects( view->vieworg );
}

// the camera, and with the panel open only the part of the screen left of it
static void SM_AimView( refdef_t *view ) {
	AnglesToAxis( sm.camAng, view->viewaxis );
	VectorCopy( sm.camAng, view->viewangles );
	if ( sm.state == SM_EDIT ) {
		view->width = (int)( view->width * SM_PANEL_X / SCREEN_WIDTH );
		view->fov_x = RAD2DEG( 2.0f * atanf( tanf( DEG2RAD( view->fov_y * 0.5f ) ) * view->width / (float)view->height ) );
	}
}

// a thin bar both ways round, as the model placer draws its box
static void SM_AddEdge( const vec3_t a, const vec3_t b, const byte *color ) {
	polyVert_t verts[4], back[4];
	vec3_t dir, toCam, side, mid;
	float width;

	VectorSubtract( b, a, dir );
	VectorAdd( a, b, mid );
	VectorScale( mid, 0.5f, mid );
	VectorSubtract( sm.camOrg, mid, toCam );
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

static bool SM_EdgeLess( const std::pair<int, int> &a, const std::pair<int, int> &b ) {
	return a.first != b.first ? a.first < b.first : a.second < b.second;
}

// the border of one map surface; a brush entity's moves with it
static void SM_AddOutline( int surfIndex, const char *bmodel, const byte *color ) {
	const refEntity_t *ent = NULL;
	std::vector<std::pair<int, int> > edges;
	const smSurf_t *s;

	if ( surfIndex < 0 || surfIndex >= (int)sm.surfs.size() )
		return;
	s = &sm.surfs[surfIndex];
	if ( s->numTris > SM_MAX_OUTLINE )
		return;
	if ( bmodel && bmodel[0] ) {
		for ( int i = 0; i < sm.numPending && !ent; i++ ) {
			const refEntity_t *e = &sm.pending[i];

			if ( e->hModel > 0 && e->hModel < SM_MAX_HANDLES && !Q_stricmp( sm.modelNames[e->hModel].c_str(), bmodel ) )
				ent = e;
		}
		if ( !ent )
			return;
	}

	// edges used by one triangle only are the border
	for ( int t = s->firstTri; t < s->firstTri + s->numTris; t++ ) {
		for ( int k = 0; k < 3; k++ ) {
			int a = sm.tris[t * 3 + k], b = sm.tris[t * 3 + ( k + 1 ) % 3];

			edges.push_back( std::make_pair( Q_min( a, b ), Q_max( a, b ) ) );
		}
	}
	std::sort( edges.begin(), edges.end(), SM_EdgeLess );
	for ( size_t i = 0; i < edges.size(); ) {
		size_t j = i;
		vec3_t p[2];

		while ( j < edges.size() && edges[j] == edges[i] )
			j++;
		if ( j - i == 1 ) {
			for ( int k = 0; k < 2; k++ ) {
				const float *v = &sm.verts[( k ? edges[i].second : edges[i].first ) * 3];

				if ( ent ) {
					VectorCopy( ent->origin, p[k] );
					for ( int a = 0; a < 3; a++ )
						VectorMA( p[k], v[a], ent->axis[a], p[k] );
				} else {
					VectorCopy( v, p[k] );
				}
			}
			SM_AddEdge( p[0], p[1], color );
		}
		i = j;
	}
}

// every cgame scene passes through here; qfalse leaves it to the model placer
qboolean CL_ShaderManager_RenderScene( const refdef_t *fd ) {
	static const byte hoverColor[4] = { 255, 220, 64, 255 };
	static const byte selColor[4] = { 64, 220, 255, 255 };
	refdef_t view;

	if ( fd->rdflags & RDF_SKYBOXPORTAL )
		sm.portalTime = cls.realtime;
	if ( fd->rdflags & ( RDF_NOWORLDMODEL | RDF_AUTOMAP ) ) {
		sm.numPending = 0;
		sm.numFxPending = 0;
		return qfalse;
	}

	if ( sm.state == SM_OFF ) {
		if ( !( fd->rdflags & RDF_SKYBOXPORTAL ) )
			SM_KeepView( fd, 0.0f, SCREEN_WIDTH );
		sm.numPending = 0;
		sm.numFxPending = 0;
		return qfalse;
	}

	view = *fd;
	SM_AimView( &view );
	if ( fd->rdflags & RDF_SKYBOXPORTAL ) {
		// the sky portal keeps its own origin but looks the way the camera does
		re->RenderScene( &view );
		sm.numPending = 0;
		sm.numFxPending = 0;
		return qtrue;
	}

	// the main view, once a frame
	if ( sm.renderedFrame == cls.framecount ) {
		re->RenderScene( &view );
		sm.numPending = 0;
		sm.numFxPending = 0;
		return qtrue;
	}
	sm.renderedFrame = cls.framecount;

	VectorCopy( sm.camOrg, view.vieworg );
	view.viewContents = CM_PointContents( sm.camOrg, 0 );
	// the snapshot's area mask is from the player's position, the camera can be anywhere
	memset( view.areamask, 0, sizeof( view.areamask ) );

	if ( sm.hitValid && sm.hitFrame >= cls.framecount - 1 && sm.hit.surf >= 0 )
		SM_AddOutline( sm.hit.surf, sm.hit.kind == SM_HIT_BRUSH ? sm.hit.model : NULL, hoverColor );
	if ( sm.state == SM_EDIT && sm.haveSel && sm.selSurf >= 0 && ( cls.realtime / 400 ) % 2 )
		SM_AddOutline( sm.selSurf, sm.selBModel, selColor );

	SM_KeepView( &view, 0.0f, sm.state == SM_EDIT ? SM_PANEL_X : SCREEN_WIDTH );
	re->RenderScene( &view );
	sm.numPending = 0;
	sm.numFxPending = 0;
	return qtrue;
}

// the ray under the crosshair, or under the mouse when the panel is open
static qboolean SM_PickRay( vec3_t org, vec3_t dir ) {
	float nx, ny, centerY = SCREEN_HEIGHT * 0.5f;

	if ( !sm.haveView )
		return qfalse;
	VectorCopy( sm.viewOrg, org );
	if ( sm.state != SM_EDIT ) {
		VectorCopy( sm.viewAxis[0], dir );
		return qtrue;
	}
	if ( sm.cursorX >= sm.viewX + sm.viewW || sm.cursorX < sm.viewX || sm.looking )
		return qfalse;
	nx = ( sm.cursorX - ( sm.viewX + sm.viewW * 0.5f ) ) / ( sm.viewW * 0.5f );
	ny = ( sm.cursorY - centerY ) / centerY;
	VectorCopy( sm.viewAxis[0], dir );
	VectorMA( dir, -nx * tanf( DEG2RAD( sm.viewFovX * 0.5f ) ), sm.viewAxis[1], dir );
	VectorMA( dir, -ny * tanf( DEG2RAD( sm.viewFovY * 0.5f ) ), sm.viewAxis[2], dir );
	VectorNormalize( dir );
	return qtrue;
}

static void SM_UpdateHit( void ) {
	vec3_t org, dir;

	if ( sm.hitFrame == cls.framecount )
		return;
	sm.hitFrame = cls.framecount;
	sm.hitValid = SM_PickRay( org, dir );
	if ( sm.hitValid )
		SM_Trace( org, dir, &sm.hit );
}

static void SM_SelectUnderRay( void ) {
	SM_UpdateHit();
	if ( !sm.hitValid || sm.hit.kind == SM_HIT_NONE || !sm.hit.shader[0] )
		return;
	SM_Select( &sm.hit );
	if ( !sm.haveSel )
		return;
	if ( sm.state != SM_EDIT )
		SM_EnterEdit( SM_TAB_OBJECT );
	sm.tab = SM_TAB_OBJECT;
	SM_SetStatus( "" );
}

/*
===============================================================================

2D

===============================================================================
*/

static vec4_t smRed			= { 1.0f, 0.3f, 0.3f, 1.0f };
static vec4_t smPanelLight	= { 0.15f, 0.15f, 0.15f, 0.85f };
static vec4_t smPanelDark	= { 0.0f, 0.0f, 0.0f, 0.85f };
static vec4_t smPanelFocus	= { 0.1f, 0.2f, 0.3f, 0.9f };
static vec4_t smHighlight	= { 0.2f, 0.45f, 0.8f, 0.8f };
static vec4_t smHover		= { 1.0f, 1.0f, 1.0f, 0.12f };
static vec4_t smHoverBox	= { 0.25f, 0.25f, 0.25f, 0.9f };
static vec4_t smBorder		= { 0.5f, 0.5f, 0.5f, 0.8f };
static vec4_t smDim			= { 0.7f, 0.7f, 0.7f, 1.0f };
static vec4_t smChecker		= { 0.3f, 0.3f, 0.3f, 1.0f };

static void SM_Fill( float x, float y, float w, float h, const float *color ) {
	re->SetColor( color );
	re->DrawStretchPic( x, y, w, h, 0, 0, 0, 0, cls.whiteShader );
	re->SetColor( NULL );
}

static void SM_Box( float x, float y, float w, float h, const float *fill ) {
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

static qboolean SM_InRect( float x, float y, float w, float h ) {
	return (qboolean)( sm.cursorX >= x && sm.cursorX < x + w && sm.cursorY >= y && sm.cursorY < y + h );
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

static void SM_Field( smField_t f, float x, float y, float w, const char *placeholder ) {
	const qboolean active = (qboolean)( SM_TypingField() == f );
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
	SM_Box( x, y, w, SM_CTRL_H, active ? smPanelFocus : smPanelLight );
	WG_Field( &style, x, y, w, SM_CTRL_H, sm.fields[f], active, placeholder, sm.cursorX, sm.cursorY );
	// a click: typed in, the cursor where it was clicked
	if ( SM_Clicked( x, y, w, WG_FieldHeight( &style, w, SM_CTRL_H, sm.fields[f], active ) ) ) {
		sm.focus = f;
		WG_FieldClick( sm.fields[f], sm.clickX, sm.clickY );
	}
}

typedef void ( *smRowFunc_t )( int row, float x, float y, float w );

// returns the row clicked, -1 if none
static int SM_List( smList_t *list, int count, float x, float y, float w, int rows, smRowFunc_t drawRow ) {
	const float h = rows * SM_ROW_H;
	int row, clicked = -1;

	if ( sm.wheel && SM_InRect( x, y, w, h ) ) {
		list->scroll += sm.wheel;
		sm.wheel = 0;
	}
	list->scroll = Com_Clampi( 0, Q_max( count - rows, 0 ), list->scroll );
	list->sel = Com_Clampi( 0, Q_max( count - 1, 0 ), list->sel );
	SM_Box( x, y, w, h, smPanelLight );
	if ( count > rows ) {
		WG_Scrollbar( x + w - WG_SCROLL_W - 1, y + 1, h - 2, count, rows, &list->scroll, sm.cursorX, sm.cursorY, &sm.click, sm.clickX, sm.clickY );
		w -= WG_SCROLL_W + 1;
	}

	if ( SM_Clicked( x, y, w, h ) ) {
		row = list->scroll + (int)( ( sm.clickY - y ) / SM_ROW_H );
		if ( row < count ) {
			list->sel = row;
			clicked = row;
		}
	}

	for ( int i = 0; i < rows; i++ ) {
		float ry = y + i * SM_ROW_H;

		row = list->scroll + i;
		if ( row >= count )
			break;
		if ( row == list->sel )
			SM_Fill( x + 1, ry, w - 2, SM_ROW_H, smHighlight );
		else if ( SM_InRect( x, ry, w, SM_ROW_H ) )
			SM_Fill( x + 1, ry, w - 2, SM_ROW_H, smHover );
		drawRow( row, x + 4, ry + 1, w - 8 );
	}
	return clicked;
}

// the star a shader row starts with: click it for a favorite
static void SM_DrawStar( float x, float y, const char *shader ) {
	SM_Text( x, y, CL_Fav_Star( CL_Fav_Has( SM_FAV_LIST, shader ) ), smWhite );
}

static void SM_DrawSelRow( int row, float x, float y, float w ) {
	const char *shader = SM_SelShader( row ), *remap;

	if ( !shader )
		return;
	remap = SM_RemappedTo( shader );
	SM_DrawStar( x, y, shader );
	if ( remap )
		SM_TextClipped( x + SM_STAR_W, y, w - SM_STAR_W, va( "%s " S_COLOR_CYAN "-> %s", shader, remap ), smWhite );
	else
		SM_TextClipped( x + SM_STAR_W, y, w - SM_STAR_W, shader, smWhite );
}

static void SM_DrawShaderRow( int row, float x, float y, float w ) {
	const char *shader = sm.shaderNames[sm.shaderView[row]].c_str();

	SM_DrawStar( x, y, shader );
	SM_TextClipped( x + SM_STAR_W, y, w - SM_STAR_W, shader, smWhite );
}

// a row's star clicked: that shader a favorite, or not any more. The lists showing only the
// starred ones are made again; returns whether it was the star
static qboolean SM_StarClicked( int clicked, float x, const char *shader ) {
	qboolean on;

	if ( clicked < 0 || !shader || sm.clickX >= x + 4 + SM_STAR_W )
		return qfalse;
	on = CL_Fav_Toggle( SM_FAV_LIST, shader );
	SM_SetStatus( va( on ? "Favorite: %s" : "No longer a favorite: %s", shader ) );
	if ( sm.filter == SM_FILTER_FAVES )
		SM_RebuildShaderView();
	if ( sm.selFaves )
		sm.selList.sel = Com_Clampi( 0, Q_max( SM_SelCount() - 1, 0 ), sm.selList.sel );
	return qtrue;
}

static void SM_DrawRemapRow( int row, float x, float y, float w ) {
	SM_TextClipped( x, y, w, va( "%s " S_COLOR_CYAN "-> %s", sm.remapFrom[row].c_str(), sm.remapTo[row].c_str() ), smWhite );
}

// a shader drawn flat in a box, on a checkerboard so see-through ones show
static void SM_DrawSwatch( float x, float y, float size, const char *label, const char *shader ) {
	qhandle_t h = shader ? SM_PreviewHandle( shader ) : 0;
	float cell = size / 8.0f;

	SM_Text( x, y, label, smDim );
	y += 12;
	SM_Box( x, y, size, size, smPanelDark );
	for ( int j = 0; j < 8; j++ ) {
		for ( int i = ( j & 1 ); i < 8; i += 2 )
			SM_Fill( x + i * cell, y + j * cell, cell, cell, smChecker );
	}
	if ( h ) {
		re->SetColor( NULL );
		re->DrawStretchPic( x + 1, y + 1, size - 2, size - 2, 0, 0, 1, 1, h );
	} else if ( shader ) {
		SM_Text( x + 4, y + size * 0.5f - 6, "No preview", smRed );
	}
}

// the readout box: under the crosshair, or under the mouse with the panel open
static void SM_DrawReadout( float cx, float cy ) {
	const char *lines[3], *remap;
	int numLines = 0;
	float w = 0.0f, h, x;

	switch ( sm.hit.kind ) {
	case SM_HIT_NONE:
		lines[numLines++] = S_COLOR_GREY "No surface";
		break;
	case SM_HIT_WORLD:
		lines[numLines++] = va( S_COLOR_YELLOW "%s", sm.hit.shader[0] ? sm.hit.shader : "(no shader)" );
		lines[numLines++] = S_COLOR_GREY "Map surface";
		break;
	case SM_HIT_BRUSH:
		lines[numLines++] = va( S_COLOR_YELLOW "%s", sm.hit.shader[0] ? sm.hit.shader : "(no shader)" );
		lines[numLines++] = va( S_COLOR_GREY "Brush entity %s", sm.hit.model );
		break;
	case SM_HIT_EFFECT: {
		int numShaders = sm.hit.fx >= 0 && sm.hit.fx < sm.numFxGroups ? sm.fxGroups[sm.hit.fx].numShaders : 1;

		lines[numLines++] = va( S_COLOR_YELLOW "%s", sm.hit.shader[0] ? sm.hit.shader : "(no shader)" );
		lines[numLines++] = numShaders > 1 ? va( S_COLOR_GREY "Effect, %i more shaders", numShaders - 1 ) : S_COLOR_GREY "Effect";
		break;
	}
	default:
		lines[numLines++] = va( S_COLOR_YELLOW "%s", sm.hit.shader[0] ? sm.hit.shader : "(no shader)" );
		lines[numLines++] = va( S_COLOR_GREY "%s%s%s", sm.hit.model, sm.hit.surface[0] ? "  surface " : "", sm.hit.surface );
		break;
	}
	remap = sm.hit.shader[0] ? SM_RemappedTo( sm.hit.shader ) : NULL;
	if ( remap )
		lines[numLines++] = va( S_COLOR_CYAN "remapped to %s", remap );

	for ( int i = 0; i < numLines; i++ )
		w = Q_max( w, SM_TextWidth( lines[i] ) );
	h = numLines * 12.0f + 6.0f;
	x = Com_Clamp( 8.0f, SCREEN_WIDTH - w - 8.0f, cx - w * 0.5f );

	SM_Fill( x - 6, cy - 3, w + 12, h, smPanel );
	for ( int i = 0; i < numLines; i++ )
		SM_Text( x, cy + i * 12.0f, lines[i], smWhite );
}

// where a point lands on the virtual screen; qfalse behind the camera
static qboolean SM_Project( const vec3_t p, float *x, float *y ) {
	vec3_t d;
	float fwd;

	VectorSubtract( p, sm.viewOrg, d );
	fwd = DotProduct( d, sm.viewAxis[0] );
	if ( fwd < 1.0f )
		return qfalse;
	*x = sm.viewX + sm.viewW * 0.5f * ( 1.0f - DotProduct( d, sm.viewAxis[1] ) / ( fwd * tanf( DEG2RAD( sm.viewFovX * 0.5f ) ) ) );
	*y = SCREEN_HEIGHT * 0.5f * ( 1.0f - DotProduct( d, sm.viewAxis[2] ) / ( fwd * tanf( DEG2RAD( sm.viewFovY * 0.5f ) ) ) );
	return qtrue;
}

// can the camera see some of the box: its middle, top or bottom
static qboolean SM_FxVisible( const vec3_t mins, const vec3_t maxs ) {
	vec3_t p;
	trace_t tr;

	for ( int i = 0; i < 3; i++ ) {
		p[0] = ( mins[0] + maxs[0] ) * 0.5f;
		p[1] = ( mins[1] + maxs[1] ) * 0.5f;
		p[2] = i == 0 ? ( mins[2] + maxs[2] ) * 0.5f : i == 1 ? maxs[2] - 1.0f : mins[2] + 1.0f;
		CM_BoxTrace( &tr, sm.viewOrg, p, vec3_origin, vec3_origin, 0, CONTENTS_SOLID, qfalse );
		if ( tr.fraction >= 1.0f || ( tr.endpos[0] >= mins[0] && tr.endpos[0] <= maxs[0] && tr.endpos[1] >= mins[1]
			&& tr.endpos[1] <= maxs[1] && tr.endpos[2] >= mins[2] && tr.endpos[2] <= maxs[2] ) )
			return qtrue;
	}
	return qfalse;
}

// a frame around where the box shows on screen; behind the camera or off the view draws nothing
static void SM_DrawFxBox( const vec3_t mins, const vec3_t maxs, const float *color, float thick ) {
	float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f, x, y;
	vec3_t corner;

	for ( int i = 0; i < 8; i++ ) {
		corner[0] = ( i & 1 ) ? maxs[0] : mins[0];
		corner[1] = ( i & 2 ) ? maxs[1] : mins[1];
		corner[2] = ( i & 4 ) ? maxs[2] : mins[2];
		if ( !SM_Project( corner, &x, &y ) )
			return;
		x0 = Q_min( x0, x );
		y0 = Q_min( y0, y );
		x1 = Q_max( x1, x );
		y1 = Q_max( y1, y );
	}
	x0 = Q_max( x0, sm.viewX );
	x1 = Q_min( x1, sm.viewX + sm.viewW );
	y0 = Q_max( y0, 0.0f );
	y1 = Q_min( y1, (float)SCREEN_HEIGHT );
	if ( x1 - x0 < 2.0f || y1 - y0 < 2.0f )
		return;
	SM_Fill( x0, y0, x1 - x0, thick, color );
	SM_Fill( x0, y1 - thick, x1 - x0, thick, color );
	SM_Fill( x0, y0, thick, y1 - y0, color );
	SM_Fill( x1 - thick, y0, thick, y1 - y0, color );
}

static void SM_DrawEffects( void ) {
	static const vec4_t boxColor = { 0.75f, 0.5f, 1.0f, 0.45f };
	static const vec4_t hoverColor = { 1.0f, 0.86f, 0.25f, 1.0f };
	static const vec4_t selColor = { 0.25f, 0.86f, 1.0f, 1.0f };

	if ( sm.hideEffects || sm.viewFrame != cls.framecount )
		return;
	for ( int i = 0; i < sm.numFxGroups; i++ ) {
		qboolean hovered = (qboolean)( sm.hitValid && sm.hit.kind == SM_HIT_EFFECT && sm.hit.fx == i );
		vec3_t mins, maxs;

		SM_FxBox( &sm.fxGroups[i], mins, maxs );
		// ones behind walls would only clutter; the one under the cursor shows wherever it is
		if ( hovered || SM_FxVisible( mins, maxs ) )
			SM_DrawFxBox( mins, maxs, hovered ? hoverColor : boxColor, hovered ? 1.5f : 1.0f );
	}
	if ( sm.state == SM_EDIT && sm.haveSel && sm.selFx && ( cls.realtime / 400 ) % 2 )
		SM_DrawFxBox( sm.selFxMins, sm.selFxMaxs, selColor, 1.5f );
}

static qboolean SM_IsPlayerShader( const char *name );
static void SM_MineSelect( const char *shader );

static void SM_DrawObjectTab( float x, float y, float w ) {
	const char *original, *remap, *replacement;
	int rows = SM_ShaderRows(), clicked;

	SM_TextClipped( x, y, w, va( S_COLOR_GREY "%s", sm.haveSel ? sm.selWhat : "Nothing selected: click something in the view" ), smWhite );
	y += 14;
	SM_Text( x, y, "Shaders on it", smDim );
	// only the starred ones, or all
	if ( SM_Button( x + w - 52, y - 3, 52, "Faves", sm.selFaves, qtrue ) ) {
		sm.selFaves = (qboolean)!sm.selFaves;
		sm.selList.sel = sm.selList.scroll = 0;
		SM_RebuildShaderView();
	}
	y += 14;
	if ( sm.haveSel ) {
		const int selRows = SM_SelCount();
		const int before = sm.selList.sel;
		const int clickedSel = SM_List( &sm.selList, selRows, x, y, w, 5, SM_DrawSelRow );

		if ( clickedSel >= 0 && SM_StarClicked( clickedSel, x, SM_SelShader( clickedSel ) ) )
			sm.selList.sel = Com_Clampi( 0, Q_max( SM_SelCount() - 1, 0 ), before );
		if ( !selRows )
			SM_Text( x + 4, y + 1, sm.selFaves ? "No favorites on it: Faves shows all again" : "None", smDim );
	} else {
		SM_Box( x, y, w, 5 * SM_ROW_H, smPanelLight );
	}
	y += 5 * SM_ROW_H + 4;
	original = SM_SelectedOriginal();
	remap = original ? SM_RemappedTo( original ) : NULL;
	if ( SM_IsSky( original ) ) {
		// a sky has its own look to change
		SM_TextClipped( x, y, w - 84, remap ? va( S_COLOR_CYAN "Now remapped to %s", remap ) : S_COLOR_GREY "Not remapped", smWhite );
		if ( SM_Button( x + w - 80, y - 2, 80, "Sky look...", qfalse, qtrue ) )
			sm.tab = SM_TAB_SKY;
	} else if ( SM_IsPlayerShader( original ) ) {
		// a player shader: on this screen only, in the Mine tab
		SM_TextClipped( x, y, w - 84, remap ? va( S_COLOR_CYAN "Now remapped to %s", remap ) : S_COLOR_GREY "Not remapped", smWhite );
		if ( SM_Button( x + w - 80, y - 2, 80, "Only mine...", qfalse, qtrue ) )
			SM_MineSelect( original );
	} else {
		SM_TextClipped( x, y, w, remap ? va( S_COLOR_CYAN "Now remapped to %s", remap ) : original ? S_COLOR_GREY "Not remapped" : "", smWhite );
	}
	y += 16;

	SM_Text( x, y + 2, "With", smDim );
	{
		// the list, filtered by where the shaders are, or a path typed in; as wide as their labels, from the right
		static const char *labels[] = { "All", "textures", "models", "gfx", "Faves", "Path..." };
		float widths[6], bx = x + w + 2;

		for ( int i = 5; i >= 0; i-- ) {
			widths[i] = SM_TextWidth( labels[i] ) + 8;
			bx -= widths[i] + 2;
		}
		for ( int i = 0; i < 6; bx += widths[i] + 2, i++ ) {
			const qboolean path = (qboolean)( i == 5 );

			if ( !SM_Button( bx, y, widths[i], labels[i], path ? sm.typingPath : (qboolean)( !sm.typingPath && sm.filter == i ), qtrue ) )
				continue;
			if ( path ) {
				sm.typingPath = qtrue;
				sm.focus = SM_F_PATH;
				SM_PathChanged();
				continue;
			}
			sm.typingPath = qfalse;
			sm.focus = SM_F_NONE;
			sm.filter = (smFilter_t)i;
			sm.shaderList.scroll = 0;
			SM_RebuildShaderView();
		}
	}
	y += 18;
	if ( sm.typingPath ) {
		// what it's typed as, and whether the game has it
		SM_Field( SM_F_PATH, x, y, w, "Type a shader or image path, e.g. effects/gfx/force2" );
		y += 18;
		SM_Box( x, y, w, rows * SM_ROW_H, smPanelLight );
		if ( !sm.pathShader[0] ) {
			SM_TextClipped( x + 4, y + 2, w - 8, S_COLOR_GREY "Any shader the game can load, or an image without", smWhite );
			SM_TextClipped( x + 4, y + 2 + SM_ROW_H, w - 8, S_COLOR_GREY "its extension. Enter applies it, as from the list.", smWhite );
		} else if ( SM_PathKnown() ) {
			SM_TextClipped( x + 4, y + 2, w - 8, va( S_COLOR_GREEN "Found: %s", sm.pathShader ), smWhite );
		} else {
			SM_TextClipped( x + 4, y + 2, w - 8, S_COLOR_YELLOW "Not a shader or image in your game files", smWhite );
			SM_TextClipped( x + 4, y + 2 + SM_ROW_H, w - 8, S_COLOR_GREY "The server may still have it; else it shows as the", smWhite );
			SM_TextClipped( x + 4, y + 2 + SM_ROW_H * 2, w - 8, S_COLOR_GREY "default, checkered shader. Live shows it here first.", smWhite );
		}
		if ( original && !SM_RemapAllowed( original, sm.pathShader[0] ? sm.pathShader : original ) )
			SM_TextClipped( x + 4, y + 2 + SM_ROW_H * 4, w - 8, S_COLOR_RED "A sky can only be remapped to another sky", smWhite );
	} else {
		if ( SM_IsSky( original ) != sm.viewSky )
			SM_RebuildShaderView();
		if ( sm.viewSky )
			SM_Field( SM_F_SEARCH, x, y, w, va( "A sky: search %i sky shaders", (int)sm.skyShaders.size() ) );
		else
			SM_Field( SM_F_SEARCH, x, y, w, va( "Type to search %i shaders", (int)sm.shaderNames.size() ) );
		y += 18;
		clicked = SM_List( &sm.shaderList, (int)sm.shaderView.size(), x, y, w, rows, SM_DrawShaderRow );
		if ( clicked >= 0 && clicked < (int)sm.shaderView.size() )
			SM_StarClicked( clicked, x, sm.shaderNames[sm.shaderView[clicked]].c_str() );
		if ( sm.shaderView.empty() )
			SM_Text( x + 4, y + 1, sm.filter == SM_FILTER_FAVES ? ( sm.viewSky ? "No favorite skies: click the * by one" : "No favorites: click the * by a shader" )
				: sm.viewSky ? "No sky shaders match" : "No shaders match", smDim );
	}
	y += rows * SM_ROW_H + 4;
	replacement = SM_HighlightedShader();

	if ( sm.showPreview ) {
		SM_DrawSwatch( x, y, 72, "Current", original ? ( remap ? remap : original ) : NULL );
		SM_DrawSwatch( x + 84, y, 72, "New", replacement );
		SM_TextClipped( x + 168, y + 12, w - 168, S_COLOR_GREY "Flat preview;", smWhite );
		SM_TextClipped( x + 168, y + 24, w - 168, S_COLOR_GREY "Live shows it", smWhite );
		SM_TextClipped( x + 168, y + 36, w - 168, S_COLOR_GREY "on the object.", smWhite );
		y += 90;
	}

	SM_Text( x, y + 2, "Time offset", smDim );
	SM_Field( SM_F_OFFSET, x + 62, y, 40, "none" );
	if ( SM_Button( x + 108, y, 74, sm.livePreview ? "Live: on" : "Live: off", sm.livePreview, qtrue ) )
		sm.livePreview = (qboolean)!sm.livePreview;
	if ( SM_Button( x + 186, y, w - 186, sm.showPreview ? "Hide preview" : "Show preview", qfalse, qtrue ) )
		sm.showPreview = (qboolean)!sm.showPreview;
	y += 19;

	if ( SM_Button( x, y, 96, "Apply  (Enter)", qfalse, (qboolean)( original && replacement ) ) )
		SM_SendRemap( original, replacement, qtrue );
	if ( SM_Button( x + 100, y, 90, "Reset original", qfalse, (qboolean)( remap != NULL ) ) )
		SM_SendRemap( original, original, qtrue );
	if ( SM_Button( x + 194, y, w - 194, "Undo last", qfalse, (qboolean)!sm.undoShader.empty() ) )
		SM_Undo();

	// shown on this screen only, while it is being picked
	if ( sm.livePreview && original && replacement )
		SM_SetLive( original, replacement );
	else
		SM_ClearLive();
}

static void SM_DrawRemapsTab( float x, float y, float w ) {
	const char *from;
	int count = (int)sm.remapFrom.size();

	SM_ClearLive();
	SM_Text( x, y, va( S_COLOR_GREY "%i shader%s remapped in this map", count, count == 1 ? "" : "s" ), smWhite );
	y += 16;
	SM_List( &sm.remapList, count, x, y, w, 26, SM_DrawRemapRow );
	if ( !count )
		SM_Text( x + 4, y + 1, "None", smDim );
	y += 26 * SM_ROW_H + 6;
	from = sm.remapList.sel < count ? sm.remapFrom[sm.remapList.sel].c_str() : NULL;
	if ( SM_Button( x, y, 90, "Reset selected", qfalse, (qboolean)( from != NULL ) ) )
		SM_SendRemap( from, from, qtrue );
	if ( SM_Button( x + 94, y, 80, "Edit it...", qfalse, (qboolean)( from != NULL ) ) ) {
		SM_SelectShader( from );
		sm.tab = SM_TAB_OBJECT;
	}
	if ( SM_Button( x + 178, y, w - 178, "Undo last", qfalse, (qboolean)!sm.undoShader.empty() ) )
		SM_Undo();
}

/*
===============================================================================

OVERLAYS

===============================================================================
*/

static void SM_DrawOverlayShaderRow( int row, float x, float y, float w ) {
	SM_TextClipped( x, y, w, sm.shaderNames[sm.overlayView[row]].c_str(), smWhite );
}

static void SM_DrawOverlayRow( int row, float x, float y, float w ) {
	SM_TextClipped( x, y, w, sm.overlays[row].line.c_str(), smWhite );
}

// the overlay's numbers as the command takes them, those left empty as RPMod would have them
static const char *SM_OverlayValue( int field ) {
	static const char *defaults[] = { "0", "0", "0", "640", "480" };
	const char *v = sm.fields[SM_F_OV_FADE + field];

	return v[0] ? v : defaults[field];
}

// rpshader overlay add <shader> [fade] [x] [y] [width] [height], up to the last number given
static void SM_AddOverlay( void ) {
	const char *shader = SM_OverlayShader();
	char cmd[MAX_STRING_CHARS];
	smOverlay_t o;
	int last = -1;

	if ( !shader )
		return;
	for ( int i = 0; i < 5; i++ ) {
		if ( sm.fields[SM_F_OV_FADE + i][0] )
			last = i;
	}
	Com_sprintf( cmd, sizeof( cmd ), "rpshader overlay add %s", shader );
	for ( int i = 0; i <= last; i++ )
		Q_strcat( cmd, sizeof( cmd ), va( " %s", SM_OverlayValue( i ) ) );
	SM_Send( cmd );
	o.shader = shader;
	o.index = -1;
	o.line = va( "%s  " S_COLOR_GREY "%s %s %sx%s", shader, SM_OverlayValue( 1 ), SM_OverlayValue( 2 ), SM_OverlayValue( 3 ), SM_OverlayValue( 4 ) );
	sm.overlays.push_back( o );
}

static void SM_RemoveOverlay( int i ) {
	if ( i < 0 || i >= (int)sm.overlays.size() )
		return;
	if ( !sm.overlays[i].shader.empty() )
		SM_Send( va( "rpshader overlay remove %s", sm.overlays[i].shader.c_str() ) );
	else
		SM_Send( va( "rpshader overlay remove %i", sm.overlays[i].index ) );
	sm.overlays.erase( sm.overlays.begin() + i );
}

// where the picked shader would show, on this screen only
static void SM_DrawOverlayPreview( void ) {
	const char *shader = SM_OverlayShader();
	float x, y, w, h;
	qhandle_t hShader;

	if ( sm.tab != SM_TAB_OVERLAYS || !sm.overlayPreview || !shader || !( hShader = SM_PreviewHandle( shader ) ) )
		return;
	x = (float)atof( SM_OverlayValue( 1 ) );
	y = (float)atof( SM_OverlayValue( 2 ) );
	w = (float)atof( SM_OverlayValue( 3 ) );
	h = (float)atof( SM_OverlayValue( 4 ) );
	re->SetColor( NULL );
	re->DrawStretchPic( x, y, w, h, 0, 0, 1, 1, hShader );
	SM_Fill( x, y, w, 1, smBorder );
	SM_Fill( x, y + h - 1, w, 1, smBorder );
	SM_Fill( x, y, 1, h, smBorder );
	SM_Fill( x + w - 1, y, 1, h, smBorder );
}

static void SM_DrawOverlaysTab( float x, float y, float w ) {
	static const char *labels[] = { "Fade", "X", "Y", "Width", "Height" };
	const char *shader;
	const float cw = ( w - 16 ) / 5;
	int count = (int)sm.overlays.size(), sel;

	SM_ClearLive();
	SM_Text( x, y, "Shader drawn over everyone's screen", smDim );
	y += 12;
	SM_Field( SM_F_OV_SEARCH, x, y, w, va( "Type to search %i shaders", (int)sm.shaderNames.size() ) );
	y += 18;
	SM_List( &sm.overlayShaders, (int)sm.overlayView.size(), x, y, w, SM_OVERLAY_ROWS, SM_DrawOverlayShaderRow );
	if ( sm.overlayView.empty() )
		SM_Text( x + 4, y + 1, "No shaders match", smDim );
	y += SM_OVERLAY_ROWS * SM_ROW_H + 4;
	shader = SM_OverlayShader();

	// the numbers, empty ones left out or as RPMod has them
	for ( int i = 0; i < 5; i++ ) {
		static const char *placeholders[] = { "0", "0", "0", "640", "480" };

		SM_Text( x + i * ( cw + 4 ), y, labels[i], smDim );
		SM_Field( (smField_t)( SM_F_OV_FADE + i ), x + i * ( cw + 4 ), y + 12, cw, placeholders[i] );
	}
	y += 31;
	if ( SM_Button( x, y, 70, "Full screen", qfalse, qtrue ) ) {
		for ( int i = 1; i < 5; i++ )
			sm.fields[SM_F_OV_FADE + i][0] = '\0';
	}
	if ( SM_Button( x + 74, y, 82, sm.overlayPreview ? "Preview: on" : "Preview: off", sm.overlayPreview, qtrue ) )
		sm.overlayPreview = (qboolean)!sm.overlayPreview;
	if ( SM_Button( x + 160, y, w - 160, "Add  (Enter)", qfalse, (qboolean)( shader != NULL ) ) )
		SM_AddOverlay();
	y += 24;

	SM_Text( x, y + 2, va( S_COLOR_GREY "%i overlay%s", count, count == 1 ? "" : "s" ), smWhite );
	if ( SM_Button( x + w - 90, y, 90, "List from server", qfalse, qtrue ) ) {
		// the reply's lines make the list
		sm.overlays.clear();
		sm.overlayListTime = cls.realtime;
		SM_Send( "rpshader overlay list" );
	}
	y += 18;
	SM_List( &sm.overlayList, count, x, y, w, SM_OVERLAY_ROWS, SM_DrawOverlayRow );
	if ( !count )
		SM_Text( x + 4, y + 1, "None added here; List asks the server", smDim );
	y += SM_OVERLAY_ROWS * SM_ROW_H + 4;
	sel = sm.overlayList.sel < count ? sm.overlayList.sel : -1;
	if ( SM_Button( x, y, 110, "Remove selected", qfalse, (qboolean)( sel >= 0 ) ) )
		SM_RemoveOverlay( sel );
	if ( SM_Button( x + 114, y, w - 114, "Remove all", qfalse, qtrue ) ) {
		SM_Send( "rpshader overlay remove all" );
		sm.overlays.clear();
	}
	y += 22;
	SM_TextClipped( x, y, w, S_COLOR_GREY "Place on a 640x480 screen. Overlays others added show", smWhite );
	SM_TextClipped( x, y + 12, w, S_COLOR_GREY "here once listed from the server.", smWhite );
}

/*
===============================================================================

SKY

===============================================================================
*/

static void SM_SkyDefault( smSky_t *sky ) {
	memset( sky, 0, sizeof( *sky ) );
	sky->tint[0] = sky->tint[1] = sky->tint[2] = 255.0f;
	sky->zoom = 1.0f;
}

static qboolean SM_SkyAltered( const smSky_t *sky ) {
	smSky_t plain;

	SM_SkyDefault( &plain );
	return (qboolean)( memcmp( sky, &plain, sizeof( plain ) ) != 0 );
}

// before each scene: the sky's look to the renderer, the spin by the server's clock so
// everyone's sky has turned as far
void CL_ShaderManager_SkyLook( void ) {
	vec3_t tint, angles;

	if ( !re )
		return;
	// the player shaders, once cgame has registered its shaders, and shared as they're due
	if ( !sm.mineApplied && cls.cgameStarted )
		SM_ApplyMine();
	SM_MineFrame();
	if ( re->ext.SetSkyPortal )
		re->ext.SetSkyPortal( sm.sky.portalOn ? sm.sky.portalOrg : NULL );
	if ( re->ext.SetSkyZoom )
		re->ext.SetSkyZoom( sm.sky.zoom );
	if ( !re->ext.SetSkyLook )
		return;
	if ( !SM_SkyAltered( &sm.sky ) ) {
		if ( sm.skySet )
			re->ext.SetSkyLook( NULL, NULL, NULL );
		sm.skySet = qfalse;
		return;
	}
	VectorScale( sm.sky.tint, 1.0f / 255.0f, tint );
	VectorCopy( sm.sky.angles, angles );
	angles[YAW] = AngleNormalize360( angles[YAW] + (float)fmod( sm.sky.spin * ( cl.serverTime / 1000.0 ), 360.0 ) );
	re->ext.SetSkyLook( tint, angles, sm.sky.shift );
	sm.skySet = qtrue;
}

// cmd:sky/<r g b>/<spin>/<pitch yaw roll>/<x y z>[/<portal x y z>|-[/<zoom>]]: the sky look an admin shared; cmd:sky/off
void CL_ShaderManager_SkyHook( char **argv, int argc ) {
	smSky_t sky;

	SM_SkyDefault( &sky );
	if ( argc >= 1 && Q_stricmp( argv[0], "off" ) && Q_stricmp( argv[0], "reset" ) ) {
		sscanf( argv[0], "%f %f %f", &sky.tint[0], &sky.tint[1], &sky.tint[2] );
		if ( argc >= 2 )
			sky.spin = (float)atof( argv[1] );
		if ( argc >= 3 )
			sscanf( argv[2], "%f %f %f", &sky.angles[0], &sky.angles[1], &sky.angles[2] );
		if ( argc >= 4 )
			sscanf( argv[3], "%f %f %f", &sky.shift[0], &sky.shift[1], &sky.shift[2] );
		if ( argc >= 5 && sscanf( argv[4], "%f %f %f", &sky.portalOrg[0], &sky.portalOrg[1], &sky.portalOrg[2] ) == 3 )
			sky.portalOn = qtrue;
		if ( argc >= 6 && atof( argv[5] ) > 0.0f )
			sky.zoom = Com_Clamp( 0.25f, 4.0f, (float)atof( argv[5] ) );
		for ( int i = 0; i < 3; i++ )
			sky.tint[i] = Com_Clamp( 0.0f, 255.0f, sky.tint[i] );
	}
	sm.sky = sky;
}

// a value dragged along a bar, or stepped with the wheel over it
static void SM_Slider( float x, float y, float w, const char *label, float *value, float min, float max, float step, const char *fmt, qboolean enabled ) {
	const float labelW = 46.0f, valueW = 44.0f, bx = x + labelW, bw = w - labelW - valueW;
	float old = *value, frac;

	SM_Text( x, y + 1, label, smDim );
	if ( enabled && SM_Clicked( bx - 3, y, bw + 6, SM_CTRL_H ) )
		sm.drag = value;
	if ( sm.drag == value ) {
		if ( sm.held[A_MOUSE1] && enabled )
			*value = min + ( max - min ) * Com_Clamp( 0.0f, 1.0f, ( sm.cursorX - bx ) / bw );
		else
			sm.drag = NULL;
	}
	if ( enabled && sm.wheel && SM_InRect( bx - 3, y, bw + 6, SM_CTRL_H ) ) {
		*value -= ( sm.wheel / 3 ) * step;
		sm.wheel = 0;
	}
	if ( *value != old )
		*value = Com_Clamp( min, max, step * floorf( *value / step + 0.5f ) );
	frac = Com_Clamp( 0.0f, 1.0f, ( *value - min ) / ( max - min ) );
	SM_Box( bx, y + 5, bw, 5, smPanelLight );
	SM_Fill( bx + 1, y + 6, ( bw - 2 ) * frac, 3, enabled ? smHighlight : smPanelLight );
	SM_Fill( bx + ( bw - 4 ) * frac, y + 1, 4, SM_CTRL_H - 2, enabled ? smWhite : smDim );
	SM_Text( bx + bw + 6, y + 1, va( fmt, *value ), enabled ? smWhite : smDim );
}

// the sky the map's surfaces show, remapped or not
static const char *SM_MapSky( void ) {
	for ( size_t i = 0; i < sm.bspShaders.size(); i++ ) {
		if ( SM_IsSky( sm.bspShaders[i].c_str() ) )
			return sm.bspShaders[i].c_str();
	}
	return NULL;
}

static float SM_DrawPortals( float x, float y, float w, qboolean mapPortal );

static void SM_DrawSkyTab( float x, float y, float w ) {
	qboolean portal = (qboolean)( sm.portalTime && cls.realtime - sm.portalTime < SM_PORTAL_MS );
	const qboolean renderer = (qboolean)( re->ext.SetSkyLook != NULL );
	const char *sky = SM_MapSky(), *remap = sky ? SM_RemappedTo( sky ) : NULL;
	vec4_t swatch;

	SM_ClearLive();
	SM_TextClipped( x, y, w, sky ? va( S_COLOR_GREY "Sky: " S_COLOR_WHITE "%s%s%s", sky, remap ? S_COLOR_CYAN " -> " : "", remap ? remap : "" )
		: S_COLOR_GREY "No sky shader found in this map", smWhite );
	y += 12;
	SM_TextClipped( x, y, w, sm.sky.portalOn ? S_COLOR_GREY "A sky portal: its whole view is turned and moved"
		: portal ? S_COLOR_GREY "A 3D skybox: its whole view is turned and moved"
		: S_COLOR_GREY "Shown on your screen as you change it", smWhite );
	y += 18;
	if ( !renderer ) {
		SM_TextClipped( x, y, w, S_COLOR_RED "This renderer can't change the sky's look", smWhite );
		return;
	}

	SM_Text( x, y, "Colour", smWhite );
	y += 13;
	SM_Slider( x, y, w - 30, "Red", &sm.sky.tint[0], 0, 255, 1, "%.0f", qtrue );
	SM_Slider( x, y + 17, w - 30, "Green", &sm.sky.tint[1], 0, 255, 1, "%.0f", qtrue );
	SM_Slider( x, y + 34, w - 30, "Blue", &sm.sky.tint[2], 0, 255, 1, "%.0f", qtrue );
	VectorScale( sm.sky.tint, 1.0f / 255.0f, swatch );
	swatch[3] = 1.0f;
	SM_Box( x + w - 24, y, 24, 49, swatch );
	y += 54;

	SM_Text( x, y, "Spin", smWhite );
	y += 13;
	SM_Slider( x, y, w, "Speed", &sm.sky.spin, -20, 20, 0.1f, "%.1f/s", qtrue );
	y += 22;

	SM_Text( x, y, "Turn and zoom", smWhite );
	y += 13;
	SM_Slider( x, y, w, "Around", &sm.sky.angles[YAW], 0, 360, 1, "%.0f", qtrue );
	SM_Slider( x, y + 17, w, "Tilt", &sm.sky.angles[PITCH], -90, 90, 1, "%.0f", qtrue );
	SM_Slider( x, y + 34, w, "Roll", &sm.sky.angles[ROLL], -180, 180, 1, "%.0f", qtrue );
	// above 1 nearer, below 1 farther; a 3D skybox or sky portal's view zooms
	SM_Slider( x, y + 51, w, "Zoom", &sm.sky.zoom, 0.25f, 4.0f, 0.05f, "%.2fx", qtrue );
	if ( fabsf( sm.sky.zoom - 1.0f ) < 0.01f )
		sm.sky.zoom = 1.0f;	// back to the map's own, not a step's rounding off it
	y += 73;

	y = SM_DrawPortals( x, y, w, portal );

	portal = (qboolean)( portal || sm.sky.portalOn );
	SM_Text( x, y, portal ? ( sm.sky.portalOn ? "Move the sky portal" : "Move the 3D skybox" ) : S_COLOR_GREY "Move: only a 3D skybox or sky portal", smWhite );
	y += 13;
	SM_Slider( x, y, w, "X", &sm.sky.shift[0], -2048, 2048, 8, "%.0f", portal );
	SM_Slider( x, y + 17, w, "Y", &sm.sky.shift[1], -2048, 2048, 8, "%.0f", portal );
	SM_Slider( x, y + 34, w, "Z", &sm.sky.shift[2], -2048, 2048, 8, "%.0f", portal );
	y += 56;

	if ( SM_Button( x, y, 60, "Reset", qfalse, SM_SkyAltered( &sm.sky ) ) )
		SM_SkyDefault( &sm.sky );
	if ( SM_Button( x + 64, y, 96, "Share with all", qfalse, qtrue ) ) {
		if ( !CL_RpHooks_Experimental() )
			SM_SetStatus( S_COLOR_YELLOW "Sharing the sky is experimental: TOGGLE EXPERIMENTAL in the npcmanager first" );
		else if ( !SM_SkyAltered( &sm.sky ) )
			SM_Send( "rpnotify -1 1 cmd:sky/off" );
		else
			SM_Send( va( "rpnotify -1 1 cmd:sky/%g %g %g/%g/%g %g %g/%g %g %g%s%s", sm.sky.tint[0], sm.sky.tint[1], sm.sky.tint[2], sm.sky.spin,
				sm.sky.angles[0], sm.sky.angles[1], sm.sky.angles[2], sm.sky.shift[0], sm.sky.shift[1], sm.sky.shift[2],
				sm.sky.portalOn ? va( "/%g %g %g", sm.sky.portalOrg[0], sm.sky.portalOrg[1], sm.sky.portalOrg[2] ) : sm.sky.zoom != 1.0f ? "/-" : "",
				sm.sky.zoom != 1.0f ? va( "/%g", sm.sky.zoom ) : "" ) );
	}
	if ( SM_Button( x + 164, y, w - 164, "Reset for all", qfalse, qtrue ) ) {
		if ( !CL_RpHooks_Experimental() ) {
			SM_SetStatus( S_COLOR_YELLOW "Sharing the sky is experimental: TOGGLE EXPERIMENTAL in the npcmanager first" );
		} else {
			SM_SkyDefault( &sm.sky );
			SM_Send( "rpnotify -1 1 cmd:sky/off" );
		}
	}
	y += 22;
	SM_TextClipped( x, y, w, S_COLOR_GREY "Shared, it shows for everyone with this client until", smWhite );
	SM_TextClipped( x, y + 12, w, S_COLOR_GREY "the map changes; those who join later don't get it.", smWhite );
}

/*
===============================================================================

SKY PORTALS

===============================================================================
*/

// "maps/ffa3.bsp" as "ffa3"
static void SM_MapBase( char *out, int size ) {
	COM_StripExtension( COM_SkipPath( cl.mapname ), out, size );
}

static const char *SM_PortalFile( void ) {
	char base[MAX_QPATH];

	SM_MapBase( base, sizeof( base ) );
	return va( SM_PORTAL_DIR "/%s.txt", base );
}

// the map's own 3D skyboxes, from its entities, and the spots kept for it
static void SM_LoadPortals( void ) {
	char base[MAX_QPATH], key[MAX_TOKEN_CHARS], classname[MAX_QPATH], origin[MAX_QPATH], *buf;
	const char *p, *token;
	smPortal_t portal;

	SM_MapBase( base, sizeof( base ) );
	if ( !Q_stricmp( base, sm.portalMap ) )
		return;
	Q_strncpyz( sm.portalMap, base, sizeof( sm.portalMap ) );
	sm.portals.clear();
	sm.portalList.sel = sm.portalList.scroll = 0;

	p = CM_EntityString();
	COM_BeginParseSession( "skyportals" );
	while ( p ) {
		token = COM_ParseExt( &p, qtrue );
		if ( !token[0] )
			break;
		if ( strcmp( token, "{" ) )
			continue;
		classname[0] = origin[0] = '\0';
		while ( 1 ) {
			token = COM_ParseExt( &p, qtrue );
			if ( !token[0] || !strcmp( token, "}" ) )
				break;
			Q_strncpyz( key, token, sizeof( key ) );
			token = COM_ParseExt( &p, qtrue );
			if ( !Q_stricmp( key, "classname" ) )
				Q_strncpyz( classname, token, sizeof( classname ) );
			else if ( !Q_stricmp( key, "origin" ) )
				Q_strncpyz( origin, token, sizeof( origin ) );
		}
		if ( !Q_stricmp( classname, "misc_skyportal" ) && sscanf( origin, "%f %f %f", &portal.org[0], &portal.org[1], &portal.org[2] ) == 3 ) {
			portal.map = qtrue;
			sm.portals.push_back( portal );
		}
	}

	if ( FS_ReadFile( SM_PortalFile(), (void **)&buf ) <= 0 || !buf )
		return;
	p = buf;
	COM_BeginParseSession( SM_PortalFile() );
	while ( 1 ) {
		int i;

		for ( i = 0; i < 3; i++ ) {
			token = COM_ParseExt( &p, qtrue );
			if ( !token[0] )
				break;
			portal.org[i] = (float)atof( token );
		}
		if ( i < 3 )
			break;
		portal.map = qfalse;
		sm.portals.push_back( portal );
	}
	FS_FreeFile( buf );
}

static void SM_SavePortals( void ) {
	std::string out = "// the shader manager's sky portal spots for this map: x y z\n";

	for ( size_t i = 0; i < sm.portals.size(); i++ ) {
		if ( !sm.portals[i].map )
			out += va( "%g %g %g\n", sm.portals[i].org[0], sm.portals[i].org[1], sm.portals[i].org[2] );
	}
	FS_WriteFile( SM_PortalFile(), out.c_str(), (int)out.size() );
}

static qboolean SM_PortalShown( int i ) {
	return (qboolean)( sm.sky.portalOn && VectorCompare( sm.portals[i].org, sm.sky.portalOrg ) );
}

static void SM_DrawPortalRow( int row, float x, float y, float w ) {
	const smPortal_t *portal;
	int spot = 0;

	if ( !row ) {
		SM_TextClipped( x, y, w, va( "%sNone: the sky as the map has it", sm.sky.portalOn ? "" : S_COLOR_GREEN "> " S_COLOR_WHITE ), smWhite );
		return;
	}
	portal = &sm.portals[row - 1];
	for ( int i = 0; i < row - 1; i++ ) {
		if ( !sm.portals[i].map )
			spot++;
	}
	SM_TextClipped( x, y, w, va( "%s%s " S_COLOR_GREY "(%.0f %.0f %.0f)", SM_PortalShown( row - 1 ) ? S_COLOR_GREEN "> " S_COLOR_WHITE : "",
		portal->map ? "The map's 3D skybox" : va( "Spot %i", spot + 1 ), portal->org[0], portal->org[1], portal->org[2] ), smWhite );
}

// the sky portal section of the sky tab; returns where the next one goes
static float SM_DrawPortals( float x, float y, float w, qboolean mapPortal ) {
	const int count = (int)sm.portals.size() + 1;
	int clicked, sel;

	SM_LoadPortals();
	SM_Text( x, y, "Sky portal", smWhite );
	SM_TextClipped( x + 58, y, w - 58, S_COLOR_GREY "the map seen from a spot, as the sky", smWhite );
	y += 13;
	clicked = SM_List( &sm.portalList, count, x, y, w, SM_PORTAL_ROWS, SM_DrawPortalRow );
	if ( clicked == 0 ) {
		sm.sky.portalOn = qfalse;
	} else if ( clicked > 0 ) {
		sm.sky.portalOn = qtrue;
		VectorCopy( sm.portals[clicked - 1].org, sm.sky.portalOrg );
	}
	y += SM_PORTAL_ROWS * SM_ROW_H + 3;
	sel = sm.portalList.sel - 1;
	if ( sel >= (int)sm.portals.size() )
		sel = -1;

	// the camera's spot, kept for this map and shown
	if ( SM_Button( x, y, 100, "Add camera spot", qfalse, qtrue ) ) {
		smPortal_t portal;

		VectorCopy( sm.camOrg, portal.org );
		portal.map = qfalse;
		sm.portals.push_back( portal );
		SM_SavePortals();
		sm.portalList.sel = (int)sm.portals.size();
		SM_ListMove( &sm.portalList, count + 1, SM_PORTAL_ROWS, 0 );
		sm.sky.portalOn = qtrue;
		VectorCopy( portal.org, sm.sky.portalOrg );
		SM_SetStatus( mapPortal ? "Sky portal: the map's 3D skybox now shows from the camera's spot"
			: "Sky portal: the sky now shows the map from the camera's spot" );
	}
	// the camera to the spot, to look around it
	if ( SM_Button( x + 104, y, 64, "Go there", qfalse, (qboolean)( sel >= 0 ) ) )
		VectorCopy( sm.portals[sel].org, sm.camOrg );
	if ( SM_Button( x + 172, y, w - 172, "Remove", qfalse, (qboolean)( sel >= 0 && !sm.portals[sel].map ) ) ) {
		if ( SM_PortalShown( sel ) )
			sm.sky.portalOn = qfalse;
		sm.portals.erase( sm.portals.begin() + sel );
		SM_SavePortals();
	}
	return y + 20;
}

/*
===============================================================================

MINE

===============================================================================
*/

static void SM_AddPlayerShader( const char *name, int group ) {
	char key[MAX_QPATH];
	smPlayerShader_t ps;

	if ( !name || !name[0] || name[0] == '{' || name[0] == '}' || name[0] == '[' || name[0] == ']' )
		return;
	SM_ShaderKey( name, key, sizeof( key ) );
	for ( size_t i = 0; i < sm.playerShaders.size(); i++ ) {
		if ( sm.playerShaders[i].name == key )
			return;
	}
	ps.name = key;
	ps.group = group;
	sm.playerShaders.push_back( ps );
}

// the shaders an effects file draws with: one after "shader", a [ ] list after "shaders"
static void SM_ParseEfxShaders( const char *path, int group ) {
	char *buf;
	const char *p, *token;

	if ( FS_ReadFile( path, (void **)&buf ) <= 0 || !buf )
		return;
	p = buf;
	COM_BeginParseSession( path );
	while ( 1 ) {
		token = COM_ParseExt( &p, qtrue );
		if ( !token[0] )
			break;
		if ( Q_stricmp( token, "shader" ) && Q_stricmp( token, "shaders" ) )
			continue;
		token = COM_ParseExt( &p, qtrue );
		if ( Q_stricmp( token, "[" ) ) {
			SM_AddPlayerShader( token, group );
			continue;
		}
		while ( 1 ) {
			token = COM_ParseExt( &p, qtrue );
			if ( !token[0] || !Q_stricmp( token, "]" ) )
				break;
			SM_AddPlayerShader( token, group );
		}
	}
	FS_FreeFile( buf );
}

static void SM_ParseEfxDir( const char *dir, const char *prefix, int group ) {
	char **files;
	int numFiles;

	files = FS_ListFiles( dir, ".efx", &numFiles );
	for ( int i = 0; i < numFiles; i++ ) {
		if ( !prefix || !Q_stricmpn( files[i], prefix, strlen( prefix ) ) )
			SM_ParseEfxShaders( va( "%s/%s", dir, files[i] ), group );
	}
	FS_FreeFileList( files );
}

static bool SM_PlayerShaderLess( const smPlayerShader_t &a, const smPlayerShader_t &b ) {
	if ( a.group != b.group )
		return a.group < b.group;
	return a.name < b.name;
}

// what the player's own things draw with: force powers and their effects, sabers, shells
static void SM_BuildPlayerShaders( void ) {
	static const char *shells[] = { "gfx/misc/personalshield", "gfx/misc/forceprotect", "gfx/misc/sightbubble" };

	sm.playerShaders.clear();
	for ( size_t i = 0; i < ARRAY_LEN( shells ); i++ )
		SM_AddPlayerShader( shells[i], SM_MINE_SHELLS );
	for ( size_t i = 0; i < sm.shaderNames.size(); i++ ) {
		const char *name = sm.shaderNames[i].c_str();

		if ( !Q_stricmpn( name, "powerups/", 9 ) && SM_ContainsNoCase( name, "shell" ) )
			SM_AddPlayerShader( name, SM_MINE_SHELLS );
		else if ( !Q_stricmpn( name, "gfx/effects/sabers/", 19 ) )
			SM_AddPlayerShader( name, SM_MINE_SABERS );
		else if ( !Q_stricmpn( name, "gfx/effects/force", 17 ) || !Q_stricmpn( name, "gfx/misc/force", 14 ) )
			SM_AddPlayerShader( name, SM_MINE_FORCE );
	}
	SM_ParseEfxDir( "effects/force", NULL, SM_MINE_FORCE );
	SM_ParseEfxDir( "effects/mp", "drain", SM_MINE_FORCE );
	SM_ParseEfxDir( "effects/saber", NULL, SM_MINE_SABERS );
	// and whatever was changed before, should it no longer be found
	for ( size_t i = 0; i < sm.mineFrom.size(); i++ )
		SM_AddPlayerShader( sm.mineFrom[i].c_str(), SM_MINE_ALL );
	std::sort( sm.playerShaders.begin(), sm.playerShaders.end(), SM_PlayerShaderLess );
	sm.playerIndexed = qtrue;
}

static qboolean SM_IsPlayerShader( const char *name ) {
	char key[MAX_QPATH];

	if ( !name || !name[0] )
		return qfalse;
	SM_ShaderKey( name, key, sizeof( key ) );
	for ( size_t i = 0; i < sm.playerShaders.size(); i++ ) {
		if ( sm.playerShaders[i].name == key )
			return qtrue;
	}
	return qfalse;
}

static void SM_RebuildMineView( void ) {
	sm.mineView.clear();
	for ( size_t i = 0; i < sm.playerShaders.size(); i++ ) {
		const smPlayerShader_t *ps = &sm.playerShaders[i];

		if ( sm.mineGroup == SM_MINE_CHANGED ? SM_MineIndex( ps->name.c_str() ) >= 0
			: ( sm.mineGroup == SM_MINE_ALL || ps->group == sm.mineGroup ) )
			sm.mineView.push_back( (int)i );
	}
	sm.mineList.sel = Com_Clampi( 0, Q_max( (int)sm.mineView.size() - 1, 0 ), sm.mineList.sel );
}

static const char *SM_MineSelected( void ) {
	if ( sm.mineList.sel < 0 || sm.mineList.sel >= (int)sm.mineView.size() )
		return NULL;
	return sm.playerShaders[sm.mineView[sm.mineList.sel]].name.c_str();
}

// the replacement highlighted in the list; the Mine tab has no typed path
static const char *SM_MineReplacement( void ) {
	if ( sm.shaderList.sel < 0 || sm.shaderList.sel >= (int)sm.shaderView.size() )
		return NULL;
	return sm.shaderNames[sm.shaderView[sm.shaderList.sel]].c_str();
}

// a player shader on this player's own: to another, or back to the standard (NULL)
static void SM_MineSet( const char *shader, const char *to ) {
	char from[MAX_QPATH], key[MAX_QPATH];
	const int i = SM_MineIndex( shader );

	SM_ShaderKey( shader, from, sizeof( from ) );
	if ( i >= 0 ) {
		sm.mineFrom.erase( sm.mineFrom.begin() + i );
		sm.mineTo.erase( sm.mineTo.begin() + i );
	}
	if ( to ) {
		SM_ShaderKey( to, key, sizeof( key ) );
		if ( !strcmp( key, from ) )
			to = NULL;
	}
	if ( to ) {
		sm.mineFrom.push_back( from );
		sm.mineTo.push_back( key );
		SM_SetStatus( va( "Yours only: %s -> %s", from, key ) );
	} else {
		SM_SetStatus( va( "Back to the standard: %s", from ) );
	}
	SM_BuildMineHandles();
	SM_SaveMine();
	if ( SM_ShareOn() )
		sm.shareAt = cls.realtime;
	if ( sm.mineGroup == SM_MINE_CHANGED )
		SM_RebuildMineView();
}

static void SM_MineClearAll( void ) {
	sm.mineFrom.clear();
	sm.mineTo.clear();
	sm.mineHandles.clear();
	SM_RebuildMapped();
	SM_SaveMine();
	if ( SM_ShareOn() )
		sm.shareAt = cls.realtime;
	SM_RebuildMineView();
	SM_SetStatus( "Every player shader back to the standard" );
}

static void SM_MineEnter( void ) {
	const char *shader = SM_MineSelected(), *to = SM_MineReplacement();

	if ( shader && to )
		SM_MineSet( shader, to );
}

// from the object tab: that shader in the Mine tab
static void SM_MineSelect( const char *shader ) {
	char key[MAX_QPATH];

	SM_ShaderKey( shader, key, sizeof( key ) );
	sm.mineGroup = SM_MINE_ALL;
	SM_RebuildMineView();
	for ( size_t i = 0; i < sm.mineView.size(); i++ ) {
		if ( sm.playerShaders[sm.mineView[i]].name == key ) {
			sm.mineList.sel = (int)i;
			sm.mineList.scroll = Q_max( 0, (int)i - SM_MINE_ROWS / 2 );
			break;
		}
	}
	SM_ClearLive();
	sm.tab = SM_TAB_MINE;
	sm.focus = SM_F_NONE;
	SM_RebuildShaderView();
}

static void SM_DrawMineRow( int row, float x, float y, float w ) {
	const char *name = sm.playerShaders[sm.mineView[row]].name.c_str(), *mine = SM_MineTo( name );

	if ( mine )
		SM_TextClipped( x, y, w, va( "%s " S_COLOR_CYAN "-> %s", name, mine ), smWhite );
	else
		SM_TextClipped( x, y, w, name, smWhite );
}

static void SM_StartSharing( void ) {
	Cvar_Set( "cl_shaderShareMine", "1" );
	sm.shareAt = cls.realtime;
	sm.shareWarned = qfalse;
	SM_SetStatus( "Shared: everyone with this client sees yours on you" );
}

// the warning, over the view: OK turns experimental features on for the session and shares
static void SM_DrawExperimentalBox( void ) {
	const float w = 320, h = 132, x = ( SM_PANEL_X - w ) * 0.5f, y = 150;
	char text[MAX_STRING_CHARS], *line = text;
	float ly = y + 24;

	SM_Box( x, y, w, h, smPanelDark );
	SM_Box( x, y, w, h, smPanelDark );
	SM_Text( x + 8, y + 6, S_COLOR_YELLOW "Experimental features", smWhite );
	// wrapped by words
	Q_strncpyz( text, RP_EXPERIMENTAL_WARNING, sizeof( text ) );
	while ( *line && ly < y + h - 46 ) {
		char *end = line, *lastSpace = NULL, keep;

		while ( *end ) {
			if ( *end == ' ' ) {
				*end = '\0';
				const qboolean fits = (qboolean)( SM_TextWidth( line ) <= w - 16 );
				*end = ' ';
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
		SM_Text( x + 8, ly, line, smWhite );
		*end = keep;
		line = *end ? end + 1 : end;
		ly += SM_ROW_H;
	}
	SM_TextClipped( x + 8, y + h - 40, w - 16, S_COLOR_GREY "Sharing your player shaders needs them. rpexperimental keeps them on.", smWhite );
	if ( SM_Button( x + 8, y + h - 22, 80, "OK", qfalse, qtrue ) ) {
		CL_RpHooks_SetExperimental( qtrue );
		sm.experimentalBox = qfalse;
		SM_StartSharing();
	} else if ( SM_Button( x + w - 88, y + h - 22, 80, "Cancel", qfalse, qtrue ) ) {
		sm.experimentalBox = qfalse;
		SM_SetStatus( "Not shared" );
	}
	// a click elsewhere on it isn't a pick in the view
	if ( SM_Clicked( x, y, w, h ) )
		return;
}

static void SM_DrawMineTab( float x, float y, float w ) {
	static const char *groups[SM_MINE_NUM_GROUPS] = { "All", "Force", "Sabers", "Shells", "Changed" };
	const int rows = sm.showPreview ? 8 : 14;
	const char *original, *mine, *replacement;
	float bx = x;

	if ( !sm.playerIndexed ) {
		SM_BuildPlayerShaders();
		SM_RebuildMineView();
	}
	if ( sm.viewSky )
		SM_RebuildShaderView();

	SM_TextClipped( x, y, w, S_COLOR_GREY "On your own only. Shared, others with this client see them", smWhite );
	y += 14;
	for ( int i = 0; i < SM_MINE_NUM_GROUPS; i++ ) {
		const float bw = SM_TextWidth( groups[i] ) + 10;

		if ( SM_Button( bx, y, bw, groups[i], (qboolean)( sm.mineGroup == i ), qtrue ) ) {
			sm.mineGroup = i;
			sm.mineList.sel = sm.mineList.scroll = 0;
			SM_RebuildMineView();
		}
		bx += bw + 2;
	}
	y += 18;
	SM_List( &sm.mineList, (int)sm.mineView.size(), x, y, w, SM_MINE_ROWS, SM_DrawMineRow );
	if ( sm.mineView.empty() )
		SM_Text( x + 4, y + 1, sm.mineGroup == SM_MINE_CHANGED ? "None changed yet" : "None found", smDim );
	y += SM_MINE_ROWS * SM_ROW_H + 4;

	original = SM_MineSelected();
	mine = SM_MineTo( original );
	SM_TextClipped( x, y, w, mine ? va( S_COLOR_CYAN "Yours: %s", mine ) : original ? S_COLOR_GREY "The standard one" : "", smWhite );
	y += 14;
	SM_Field( SM_F_SEARCH, x, y, w, va( "With: type to search %i shaders", (int)sm.shaderNames.size() ) );
	y += 18;
	{
		const int clickedWith = SM_List( &sm.shaderList, (int)sm.shaderView.size(), x, y, w, rows, SM_DrawShaderRow );

		if ( clickedWith >= 0 && clickedWith < (int)sm.shaderView.size() )
			SM_StarClicked( clickedWith, x, sm.shaderNames[sm.shaderView[clickedWith]].c_str() );
	}
	if ( sm.shaderView.empty() )
		SM_Text( x + 4, y + 1, "No shaders match", smDim );
	y += rows * SM_ROW_H + 4;
	replacement = SM_MineReplacement();

	if ( sm.showPreview ) {
		SM_DrawSwatch( x, y, 72, "Current", original ? ( mine ? mine : original ) : NULL );
		SM_DrawSwatch( x + 84, y, 72, "New", replacement );
		SM_TextClipped( x + 168, y + 12, w - 168, S_COLOR_GREY "Flat preview;", smWhite );
		SM_TextClipped( x + 168, y + 24, w - 168, S_COLOR_GREY "Live shows it", smWhite );
		SM_TextClipped( x + 168, y + 36, w - 168, S_COLOR_GREY "on yours, in", smWhite );
		SM_TextClipped( x + 168, y + 48, w - 168, S_COLOR_GREY "the game.", smWhite );
		y += 90;
	}

	if ( SM_Button( x, y, 74, sm.livePreview ? "Live: on" : "Live: off", sm.livePreview, qtrue ) )
		sm.livePreview = (qboolean)!sm.livePreview;
	// to the others with this client, who see them on this player's own
	if ( SM_Button( x + 78, y, 84, SM_ShareOn() ? "Shared: on" : "Shared: off", SM_ShareOn(), qtrue ) ) {
		if ( SM_ShareOn() ) {
			Cvar_Set( "cl_shaderShareMine", "0" );
			SM_QueueShare( va( "cmd:mine/%i/clear", clc.clientNum ) );
			SM_SetStatus( "Not shared: the others see your standard ones again" );
		} else if ( !CL_RpHooks_Experimental() ) {
			// the warning first; its OK turns them on and shares
			sm.experimentalBox = qtrue;
		} else {
			SM_StartSharing();
		}
	}
	if ( SM_Button( x + 166, y, w - 166, sm.showPreview ? "Hide preview" : "Show preview", qfalse, qtrue ) )
		sm.showPreview = (qboolean)!sm.showPreview;
	y += 19;

	if ( SM_Button( x, y, 96, "Use it  (Enter)", qfalse, (qboolean)( original && replacement ) ) )
		SM_MineSet( original, replacement );
	if ( SM_Button( x + 100, y, 70, "Standard", qfalse, (qboolean)( mine != NULL ) ) )
		SM_MineSet( original, NULL );
	if ( SM_Button( x + 174, y, w - 174, "All standard", qfalse, (qboolean)!sm.mineFrom.empty() ) )
		SM_MineClearAll();

	// shown on this player's own while it is being picked
	SM_ClearLive();
	if ( sm.livePreview && original && replacement )
		SM_SetLiveMine( original, replacement );
	else
		SM_SetLiveMine( NULL, NULL );
}

static void SM_DrawPanel( void ) {
	static const char *tabs[SM_NUM_TABS] = { "Object", "Remaps", "Overlays", "Sky", "Mine" };
	const float px = SM_PANEL_X, pw = SM_PANEL_W, x = px + 8, w = pw - 16;
	qboolean click;
	float bx = x + w;

	SM_Box( px, 8, pw, SCREEN_HEIGHT - 16, smPanelDark );
	// the tabs from the right, as wide as their names
	for ( int i = SM_NUM_TABS - 1; i >= 0; i-- ) {
		const float bw = SM_TextWidth( tabs[i] ) + 10;

		bx -= bw;
		if ( SM_Button( bx, 12, bw, tabs[i], (qboolean)( sm.tab == i ), qtrue ) ) {
			sm.tab = (smTab_t)i;
			sm.focus = SM_F_NONE;
		}
		bx -= 2;
	}
	if ( sm.tab != SM_TAB_MINE )
		SM_SetLiveMine( NULL, NULL );
	// the title where the tabs leave room for it
	if ( SM_TextWidth( "Shader Manager" ) + 4 < bx - x )
		SM_Text( x, 14, "Shader Manager", smWhite );
	else if ( SM_TextWidth( "Shaders" ) + 4 < bx - x )
		SM_Text( x, 14, "Shaders", smWhite );

	if ( sm.tab == SM_TAB_OBJECT )
		SM_DrawObjectTab( x, 34, w );
	else if ( sm.tab == SM_TAB_REMAPS )
		SM_DrawRemapsTab( x, 34, w );
	else if ( sm.tab == SM_TAB_OVERLAYS )
		SM_DrawOverlaysTab( x, 34, w );
	else if ( sm.tab == SM_TAB_MINE )
		SM_DrawMineTab( x, 34, w );
	else
		SM_DrawSkyTab( x, 34, w );

	// the server's answer, else what was sent
	if ( sm.reply[0] && cls.realtime - sm.replyTime < 8000 )
		SM_TextClipped( x, SCREEN_HEIGHT - 24, w, va( S_COLOR_YELLOW "Server: " S_COLOR_WHITE "%s", sm.reply ), smWhite );
	else if ( sm.status[0] )
		SM_TextClipped( x, SCREEN_HEIGHT - 24, w, va( "%s%s", cls.realtime - sm.sentTime < 1500 ? S_COLOR_GREEN : S_COLOR_GREY, sm.status ), smWhite );

	if ( sm.experimentalBox ) {
		if ( sm.tab == SM_TAB_MINE )
			SM_DrawExperimentalBox();
		else
			sm.experimentalBox = qfalse;
	}

	// a click in the view picks what is under it
	click = sm.click;
	if ( click && sm.clickX < SM_PANEL_X ) {
		sm.click = qfalse;
		sm.focus = SM_F_NONE;
		SM_SelectUnderRay();
	}
}

void CL_ShaderManager_Draw( void ) {
	if ( !sm.initialized )
		return;
	if ( Key_GetCatcher() & ( KEYCATCH_UI | KEYCATCH_NPCMANAGER ) )
		return;
	if ( sm.state == SM_OFF && !cl_shaderCrosshair->integer )
		return;
	if ( !sm.font ) {
		sm.font = re->RegisterFont( "arialnb" );
		if ( !sm.font )
			sm.font = cls.menuFont;
	}

	SM_UpdateHit();
	if ( sm.state == SM_OFF ) {
		if ( sm.hitValid )
			SM_DrawReadout( SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f + 24.0f );
		return;
	}

	if ( sm.state == SM_CAMERA ) {
		if ( sm.click ) {
			sm.click = qfalse;
			SM_SelectUnderRay();
			if ( sm.state == SM_EDIT )
				return;
		}
		SM_DrawEffects();
		SM_Fill( SCREEN_WIDTH * 0.5f - 1, SCREEN_HEIGHT * 0.5f - 6, 2, 12, smWhite );
		SM_Fill( SCREEN_WIDTH * 0.5f - 6, SCREEN_HEIGHT * 0.5f - 1, 12, 2, smWhite );
		if ( sm.hitValid )
			SM_DrawReadout( SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f + 24.0f );
		SM_Text( 16, SCREEN_HEIGHT - 28, S_COLOR_GREY "WASD fly   Space/C up/down   Shift faster   Ctrl slower", smWhite );
		SM_Text( 16, SCREEN_HEIGHT - 16, va( S_COLOR_GREY "Click: pick what's under the crosshair   F: effect boxes %s   Tab: remap list   Esc: close",
			sm.hideEffects ? "(off)" : "(on)" ), smWhite );
		return;
	}

	SM_DrawEffects();
	SM_DrawOverlayPreview();
	// before the panel, which takes clicks in the view as picks
	if ( SM_Button( 16, SCREEN_HEIGHT - 48, 110, sm.hideEffects ? "Effect boxes: off" : "Effect boxes: on", (qboolean)!sm.hideEffects, qtrue ) )
		sm.hideEffects = (qboolean)!sm.hideEffects;
	SM_DrawPanel();
	// not over the experimental features warning
	if ( sm.hitValid && !sm.experimentalBox )
		SM_DrawReadout( sm.cursorX, sm.cursorY + 20.0f );
	SM_Text( 16, SCREEN_HEIGHT - 28, S_COLOR_GREY "Hold right mouse: look and fly (WASD)", smWhite );
	SM_Text( 16, SCREEN_HEIGHT - 16, S_COLOR_GREY "Click in the view: pick   Tab: tabs   Esc: back", smWhite );

	// a click on nothing leaves the text box
	if ( sm.click )
		sm.focus = SM_F_NONE;
	sm.click = qfalse;
	sm.wheel = 0;
	WG_DrawOverlay();
	if ( !sm.looking )
		re->DrawStretchPic( sm.cursorX, sm.cursorY, 32, 32, 0, 0, 1, 1, cls.cursorShader );
}

/*
===============================================================================

SETUP

===============================================================================
*/

void CL_ShaderManager_f( void ) {
	const char *arg = Cmd_Argv( 1 );

	if ( Cmd_Argc() < 2 ) {
		if ( sm.state != SM_OFF )
			CL_ShaderManager_Close();
		else
			SM_Open();
	} else if ( !Q_stricmp( arg, "crosshair" ) ) {
		Cvar_SetValue( "cl_shaderCrosshair", cl_shaderCrosshair->integer ? 0 : 1 );
		Com_Printf( "Shader under the crosshair: %s\n", cl_shaderCrosshair->integer ? "shown" : "hidden" );
	} else if ( !Q_stricmp( arg, "remaps" ) ) {
		if ( sm.remapFrom.empty() )
			Com_Printf( "No shaders are remapped\n" );
		for ( size_t i = 0; i < sm.remapFrom.size(); i++ )
			Com_Printf( "%s -> %s\n", sm.remapFrom[i].c_str(), sm.remapTo[i].c_str() );
	} else if ( !Q_stricmp( arg, "mine" ) ) {
		if ( !sm.mineLoaded )
			SM_LoadMine();
		if ( !Q_stricmp( Cmd_Argv( 2 ), "reset" ) ) {
			SM_MineClearAll();
			Com_Printf( "Every player shader is back to the standard\n" );
		} else if ( !Q_stricmp( Cmd_Argv( 2 ), "others" ) ) {
			int shown = 0;

			for ( int c = 0; c < MAX_CLIENTS; c++ ) {
				for ( size_t i = 0; i < sm.otherFrom[c].size(); i++, shown++ )
					Com_Printf( "%i %s: %s -> %s\n", c, Info_ValueForKey( cl.gameState.stringData + cl.gameState.stringOffsets[CS_PLAYERS + c], "n" ),
						sm.otherFrom[c][i].c_str(), sm.otherTo[c][i].c_str() );
			}
			if ( !shown )
				Com_Printf( "Nobody shared their player shaders\n" );
		} else if ( !Q_stricmp( Cmd_Argv( 2 ), "share" ) ) {
			sm.shareAt = cls.realtime;
			sm.shareWarned = qfalse;
		} else {
			if ( sm.mineFrom.empty() )
				Com_Printf( "No player shaders changed on this screen\n" );
			for ( size_t i = 0; i < sm.mineFrom.size(); i++ )
				Com_Printf( "%s -> %s\n", sm.mineFrom[i].c_str(), sm.mineTo[i].c_str() );
		}
	} else if ( !Q_stricmp( arg, "reindex" ) ) {
		SM_BuildIndex();
		SM_RebuildShaderView();
		SM_BuildPlayerShaders();
		SM_RebuildMineView();
	} else if ( !Q_stricmp( arg, "close" ) ) {
		CL_ShaderManager_Close();
	} else {
		Com_Printf( "usage: " SM_TOGGLE_CMD "              toggle the shader manager (bind a key to it)\n" );
		Com_Printf( "       " SM_TOGGLE_CMD " crosshair    show/hide the shader under the crosshair\n" );
		Com_Printf( "       " SM_TOGGLE_CMD " remaps       list the shaders remapped in this map\n" );
		Com_Printf( "       " SM_TOGGLE_CMD " mine [reset] list your player shaders, or put them back\n" );
		Com_Printf( "       " SM_TOGGLE_CMD " mine others  the player shaders others shared\n" );
		Com_Printf( "       " SM_TOGGLE_CMD " mine share   send yours again (with Shared on in the Mine tab)\n" );
		Com_Printf( "       " SM_TOGGLE_CMD " reindex      rescan the filesystem for shaders\n" );
		Com_Printf( "       " SM_TOGGLE_CMD " close        close the shader manager\n" );
	}
}

void CL_ShaderManager_Init( void ) {
	cl_shaderCrosshair = Cvar_Get( "cl_shaderCrosshair", "0", CVAR_ARCHIVE, "Show the shader under the crosshair" );
	cl_shaderShareMine = Cvar_Get( "cl_shaderShareMine", "0", CVAR_ARCHIVE, "Share your Mine player shaders with others who have this client" );
	sm.initialized = qtrue;
	sm.showPreview = qtrue;
	sm.livePreview = qtrue;
	sm.overlayPreview = qtrue;
	sm.selSurf = -1;
	SM_SkyDefault( &sm.sky );
}

// cgame is going away: map change, disconnect or vid_restart
void CL_ShaderManager_Shutdown( void ) {
	CL_ShaderManager_Close();
	SM_FreeMap();
	for ( int i = 0; i < SM_MAX_HANDLES; i++ ) {
		sm.modelNames[i].clear();
		sm.skinNames[i].clear();
		sm.md3s[i] = smMd3_t();
		sm.skins[i] = smSkin_t();
	}
	sm.glmNames.clear();
	sm.glms.clear();
	sm.remapFrom.clear();
	sm.remapTo.clear();
	sm.undoShader.clear();
	sm.undoPrevious.clear();
	sm.haveSel = qfalse;
	sm.selShaders.clear();
	sm.haveView = qfalse;
	sm.hitValid = qfalse;
	sm.numFxGroups = 0;
	sm.selFx = qfalse;
	SM_SkyDefault( &sm.sky );
	sm.overlays.clear();
	sm.overlayView.clear();
	sm.portalTime = 0;
	sm.portalMap[0] = '\0';
	sm.portals.clear();
	// the renderer's shaders go: the player shaders' handles are found again with the next cgame's
	sm.mineApplied = qfalse;
	sm.mineHandles.clear();
	// the others' come again as the next map starts; the client numbers may change
	for ( int c = 0; c < MAX_CLIENTS; c++ ) {
		sm.otherFrom[c].clear();
		sm.otherTo[c].clear();
		sm.otherHandles[c].clear();
	}
	sm.mapped.clear();
	sm.shareQueue.clear();
	sm.shareAt = 0;
	sm.sharePlayers = 0;
	SM_SetLiveMine( NULL, NULL );
	sm.myOrgFrame = 0;
	sm.playerIndexed = qfalse;
	sm.playerShaders.clear();
	sm.mineView.clear();
	sm.liveFrom[0] = sm.liveTo[0] = '\0';
	sm.numPending = sm.numEntities = 0;
	sm.font = 0;
	// the pure pk3 list can change with the next map or server, and the renderer drops its shaders
	sm.indexed = qfalse;
	std::vector<std::string>().swap( sm.shaderNames );
	std::set<std::string>().swap( sm.skyShaders );
	sm.previews.clear();
	sm.shaderView.clear();
}
