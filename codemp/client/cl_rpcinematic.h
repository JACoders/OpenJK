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

// cl_rpcinematic.h -- what the cinematic camera (cl_rpcinematic.cpp) shares with the
// cinematic manager (cl_cinematic.cpp)

#pragma once

#define RC_MAX_KEYS			16
#define RC_BAR_FRAC			0.12f		// of the screen's height, each
#define RC_DEFAULT_FOV		80.0f

#define RC_SHOT_DIR			"camshots"	// in the game folder: shots from before cinematics
#define RC_SHOT_EXT			".cam"
#define RC_MAX_HOOK			900			// RPMod passes the line on in a print
#define RC_MAX_WALKS		4			// NPCs walking in an old shot
#define RC_MAX_ANIMS		8			// animations in an old shot

#define RC_DOF_RANGE		128.0f		// what a new depth of field keeps sharp, either side
#define RC_DOF_BLUR			4.0f
#define RC_DOF_MAX_BLUR		10.0f
#define RC_DOF_POINT		-2.0f		// a shot's focus on a spot that moves from key to key
#define RC_MAX_FOCUS		8
#define RC_TRANS_MS			1000

// an NPC walking a route while an old shot plays
typedef struct {
	char		npc[MAX_QPATH];		// targetname or number, commas between several
	char		route[MAX_QPATH];
	qboolean	run;
	int			delay;				// ms into the shot it sets off
} rcWalk_t;

typedef struct {
	vec3_t	org;
	vec3_t	ang;
	float	fov;		// cg_fov's; 0 is as the key before
	int		ms;			// from the key before

	// its own depth of field, or the one before's
	qboolean	dofSet;
	float		dofFocus, dofRange, dofBlur;
} rcKey_t;

// how a shot comes in or goes: cut, faded from or to a colour, or wiped across from a side
typedef enum {
	RC_TR_CUT,
	RC_TR_FADE,
	RC_TR_LEFT,
	RC_TR_RIGHT,
	RC_TR_TOP,
	RC_TR_BOTTOM,
	RC_NUM_TRANSITIONS
} rcTransType_t;

// a filter over the picture
typedef enum {
	RC_FI_NONE,
	RC_FI_BW,
	RC_FI_NOIR,
	RC_FI_SEPIA,
	RC_FI_OLDFILM,
	RC_FI_NIGHT,
	RC_FI_THERMAL,
	RC_FI_CCTV,
	RC_FI_VHS,
	RC_FI_HOLOGRAM,
	RC_FI_WARM,
	RC_FI_COLD,
	RC_FI_FADED,
	RC_FI_NEGATIVE,
	RC_NUM_FILTERS
} rcFilter_t;

typedef struct {
	int			type;			// rcTransType_t
	int			ms;
	byte		color[3];
} rcTrans_t;

// where a moving focus is ms into its shot, and how far either side of it is sharp
typedef struct {
	int			ms;
	vec3_t		org;
	float		range;
} rcFocus_t;

// a camera shot: a path through spots, one spot held (a still, its ms long), or an orbit
typedef struct {
	qboolean	orbit;
	qboolean	linear;

	// depth of field
	qboolean	dof;
	float		dofFocus;		// 0 is what the camera looks at, -1 dofWho, RC_DOF_POINT the focus keys
	float		dofRange, dofBlur;
	char		dofWho[MAX_QPATH];
	rcFocus_t	focus[RC_MAX_FOCUS];	// by when
	int			numFocus;

	// a filter over it all, rcFilter_t
	int			filter;
	int			filterStrength;	// percent

	// coming in, going out
	rcTrans_t	in, out;

	// a path
	rcKey_t		keys[RC_MAX_KEYS + 1];		// a move's keys[0] is where it set off from
	int			numKeys;
	qboolean	look;
	vec3_t		lookAt;

	// an orbit
	vec3_t		center;
	float		radius, height, fromYaw, degrees;
	float		fov, endFov;
	int			ms;				// an orbit's, or a still's
} rcShot_t;

typedef enum {
	RC_AM_PLAY,
	RC_AM_REVERSE,
	RC_AM_HOLD,
	RC_NUM_AMODES
} rcAnimMode_t;

// one of JKA's animations forced on someone
typedef struct {
	char		who[MAX_QPATH];		// numbers, me, a player's name or an NPC's targetname, commas between
	char		anim[48];			// BOTH_STAND1 and the like
	int			from, to;			// frames into it; to -1 is its last
	int			mode;				// rcAnimMode_t
	qboolean	loop;
	int			speed;				// percent
	int			delay, ms;			// into the shot; how long, 0 to the shot's end
} rcAnim_t;

// an animation in a skeleton's animation.cfg
typedef struct {
	char		name[48];
	int			first, num;
	float		fps;
} rcAnimDef_t;

// the camera's sums
float RC_Ease( float t, qboolean linear );
float RC_FovY( float fovX );
float RC_FovX( float fovY, float width, float height );
int RC_Round( float v );
int RC_SecondsMs( float seconds );
float RC_Seconds( int ms );

// shots
int RC_ShotMs( const rcShot_t *s );
void RC_EvalShot( const rcShot_t *s, int ms, vec3_t org, vec3_t ang, float *fov );
void RC_FillFov( rcShot_t *s, float fov );
qboolean RC_ParseShot( char **argv, int argc, rcShot_t *s, const vec3_t ang );
void RC_WriteShot( const rcShot_t *s, char *out, int size );
int RC_SplitHook( char *hook, char **argv, int max );
qboolean RC_ReadShot( const char *name, char *hook, int size, rcShot_t *shot );
int RC_ReadWalks( const char *name, rcWalk_t *walks, int max );
int RC_ReadAnims( const char *name, rcAnim_t *anims, int max );

// how a shot looks
void RC_KeyDof( const rcShot_t *s, int key, float *focus, float *range, float *blur );
qboolean RC_DepthOfField( const rcShot_t *s, int prev, int next, float frac, const vec3_t org, const vec3_t ang, float *now, int *time );
qboolean RC_ShotDepthOfField( const rcShot_t *s, int ms, const vec3_t org, const vec3_t ang, float *now, int *time );
void RC_NoDepthOfField( void );
void RC_FocusPoint( const rcShot_t *s, int ms, vec3_t at, float *range );
qboolean RC_FocusTarget( const char *who, vec3_t out );
qboolean RC_Filter( int type, int strength );
void RC_NoFilter( void );
const char *RC_FilterName( int type );
void RC_DrawTransitions( const rcShot_t *shot, int ms );

// the camera, for a cinematic: the HUD and the bars as it has them, then each of its shots, msIn
// ms into it
void RC_CineBegin( qboolean hud, qboolean bars );
void RC_CineShot( const rcShot_t *shot, int msIn );
void RC_SyncThirdPerson( void );
qboolean RC_CinematicsOn( void );
qboolean RC_PlayerView( vec3_t org, vec3_t ang, float *fovY );

// animations
int RC_AnimTargets( const char *who, int *nums, int max );
int RC_AnimMs( const rcAnim_t *a, int shotMs );
int RC_EntCfg( int num );
const rcAnimDef_t *RC_FindAnim( int cfg, const char *name );
const rcAnimDef_t *RC_AnimDefs( int cfg, int *count );
// a cinematic's animations now: each of nums on its animation, elapsed ms into it; the others it
// had let go
void RC_TimelineAnims( const int *nums, const rcAnim_t *const *anims, const int *elapsed, int count );
void RC_TimelineClear( void );

// the cinematic manager's, for the camera
qboolean CM_EditorOpen( void );
qboolean CM_EditorViewing( void );
void CM_EditorRenderScene( const refdef_t *fd );
qboolean CM_EditorCamera( vec3_t org, vec3_t ang, float *fov );
int CM_EditorFilter( void );
void CM_Frame( void );
void CM_Draw( void );
void CM_Init( void );
void CM_Shutdown( void );
void CM_Reset( void );
// recording someone else: the view where they are, your own body out of it; the blades seen,
// for the clashes
qboolean CM_RecordView( const refdef_t *fd );
qboolean CM_RecordCamera( vec3_t origin, vec3_t angles );
qboolean CM_RecordHides( const vec3_t origin );
void CM_NoteBlade( const vec3_t origin, const vec3_t dir, float length );
