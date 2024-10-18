/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

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

#ifndef	__CG_LOCAL_H__
#define	__CG_LOCAL_H__

#include "../qcommon/q_shared.h"

// define GAME_INCLUDE so that g_public.h does not define the
// short, server-visible gclient_t and gentity_t structures,
// because we define the full size ones in this file
#define	GAME_INCLUDE
#include "../game/g_shared.h"
#include "cg_camera.h"

// The entire cgame module is unloaded and reloaded on each level change,
// so there is NO persistant data between levels on the client side.
// If you absolutely need something stored, it can either be kept
// by the server in the server stored userinfos, or stashed in a cvar.

#define	POWERUP_BLINKS		5
#define	POWERUP_BLINK_TIME	1000
#define	FADE_TIME			200
#define	PULSE_TIME			200
#define	DAMAGE_DEFLECT_TIME	100
#define	DAMAGE_RETURN_TIME	400
#define DAMAGE_TIME			500
#define	LAND_DEFLECT_TIME	150
#define	LAND_RETURN_TIME	300
#define	STEP_TIME			200
#define	DUCK_TIME			100
#define	PAIN_TWITCH_TIME	200
#define	WEAPON_SELECT_TIME	1400
#define	ITEM_SCALEUP_TIME	1000
// Zoom vars
#define	ZOOM_TIME			150		// not currently used?
#define MAX_ZOOM_FOV		3.0f
#define ZOOM_IN_TIME		1500.0f
#define ZOOM_OUT_TIME		100.0f
#define ZOOM_START_PERCENT	0.3f

#define	ITEM_BLOB_TIME		200
#define	MUZZLE_FLASH_TIME	20
#define	FRAG_FADE_TIME		1000		// time for fragments to fade away

#define	PULSE_SCALE			1.5			// amount to scale up the icons when activating

#define	MAX_STEP_CHANGE		32

#define	MAX_VERTS_ON_POLY	10
#define	MAX_MARK_POLYS		256

#define STAT_MINUS			10	// num frame for '-' stats digit

#define	ICON_SIZE			48
#define	TEXT_ICON_SPACE		4

#define	CHARSMALL_WIDTH		16
#define	CHARSMALL_HEIGHT	32

// very large characters
#define	GIANT_WIDTH			32
#define	GIANT_HEIGHT		48

#define MAX_PRINTTEXT		128
#define MAX_CAPTIONTEXT		 32	// we don't need 64 now since we don't use this for scroll text, and I needed to change a hardwired 128 to 256, so...
#define MAX_LCARSTEXT		128


#define NUM_FONT_BIG	1
#define NUM_FONT_SMALL	2
#define NUM_FONT_CHUNKY	3

#define CS_BASIC	0
#define CS_COMBAT	1
#define CS_EXTRA	2
#define CS_JEDI		3
#define CS_TRY_ALL	4

#define	WAVE_AMPLITUDE	1
#define	WAVE_FREQUENCY	0.4

//=================================================

// player entities need to track more information
// than any other type of entity.

// note that not every player entity is a client entity,
// because corpses after respawn are outside the normal
// client numbering range

// when changing animation, set animationTime to frameTime + lerping time
// The current lerp will finish out, then it will lerp to the new animation
typedef struct {
	int			oldFrame;
	int			oldFrameTime;		// time when ->oldFrame was exactly on

	int			frame;
	int			frameTime;			// time when ->frame will be exactly on

	float		backlerp;

	float		yawAngle;
	qboolean	yawing;
	float		pitchAngle;
	qboolean	pitching;

	int			animationNumber;
	animation_t	*animation;
	int			animationTime;		// time when the first frame of the animation will be exact
} lerpFrame_t;

typedef struct {
	lerpFrame_t		legs, torso;
	int				painTime;
	int				painDirection;	// flip from 0 to 1

	// For persistent beam weapons, so they don't play their start sound more than once
	qboolean		lightningFiring;

	// machinegun spinning
//	float			barrelAngle;
//	int				barrelTime;
//	qboolean		barrelSpinning;
} playerEntity_t;

//=================================================

// centity_t have a direct corespondence with gentity_t in the game, but
// only the entityState_t is directly communicated to the cgame
struct centity_s
{
	entityState_t	currentState;	// from cg.frame
	const entityState_t	*nextState;		// from cg.nextFrame, if available
	qboolean		interpolate;	// true if next is valid to interpolate to
	qboolean		currentValid;	// true if cg.frame holds this entity

	int				muzzleFlashTime;	// move to playerEntity?
	qboolean		altFire;			// move to playerEntity?

	int				previousEvent;
//	int				teleportFlag;

//	int				trailTime;		// so missile trails can handle dropped initial packets
	int				miscTime;

	playerEntity_t	pe;

//	int				errorTime;		// decay the error from this time
//	vec3_t			errorOrigin;
//	vec3_t			errorAngles;

//	qboolean		extrapolated;	// false if origin / angles is an interpolation
//	vec3_t			rawOrigin;
//	vec3_t			rawAngles;

//	vec3_t			beamEnd;

	// exact interpolated position of entity on this frame
	vec3_t			lerpOrigin;
	vec3_t			lerpAngles;
	vec3_t			renderAngles;	//for ET_PLAYERS, the actual angles it was rendered at- should be used by any getboltmatrix calls after CG_Player

	float			rotValue; //rotation increment for repeater effect

	int				snapShotTime;

	//Pointer to corresponding gentity
	gentity_t		*gent;
};

typedef centity_s centity_t;


//======================================================================

// local entities are created as a result of events or predicted actions,
// and live independently from all server transmitted entities

typedef struct markPoly_s {
	struct markPoly_s	*prevMark, *nextMark;
	int			time;
	qhandle_t	markShader;
	qboolean	alphaFade;		// fade alpha instead of rgb
	float		color[4];
	poly_t		poly;
	polyVert_t	verts[MAX_VERTS_ON_POLY];
} markPoly_t;


typedef enum {
	LE_MARK,
	LE_FADE_MODEL,
	LE_FADE_SCALE_MODEL, // currently only for Demp2 shock sphere
	LE_FRAGMENT,
	LE_PUFF,
	LE_FADE_RGB,
	LE_LIGHT,
	LE_LINE,
	LE_QUAD,
	LE_SPRITE,
} leType_t;

typedef enum {
	LEF_PUFF_DONT_SCALE = 0x0001,			// do not scale size over time
	LEF_TUMBLE			= 0x0002,			// tumble over time, used for ejecting shells
	LEF_FADE_RGB		= 0x0004,			// explicitly fade
	LEF_NO_RANDOM_ROTATE= 0x0008			// MakeExplosion adds random rotate which could be bad in some cases
} leFlag_t;

typedef enum
{
	LEBS_NONE,
	LEBS_METAL,
	LEBS_ROCK
} leBounceSound_t;	// fragment local entities can make sounds on impacts

typedef struct localEntity_s {
	struct localEntity_s	*prev, *next;
	leType_t		leType;
	int				leFlags;

	int				startTime;
	int				endTime;

	float			lifeRate;			// 1.0 / (endTime - startTime)

	trajectory_t	pos;
	trajectory_t	angles;

	float			bounceFactor;		// 0.0 = no bounce, 1.0 = perfect

	float			color[4];

	float			radius;

	float			light;
	vec3_t			lightColor;

	leBounceSound_t	leBounceSoundType;

	refEntity_t		refEntity;
	int				ownerGentNum;
} localEntity_t;

//======================================================================


// each IT_* item has an associated itemInfo_t
// that constains media references necessary to present the
// item and its effects
typedef struct {
	qboolean		registered;
	qhandle_t		models;
	qhandle_t		icon;
} itemInfo_t;


typedef struct {
	int				itemNum;
} powerupInfo_t;


#define	CG_OVERRIDE_3RD_PERSON_ENT	0x00000001
#define	CG_OVERRIDE_3RD_PERSON_RNG	0x00000002
#define	CG_OVERRIDE_3RD_PERSON_ANG	0x00000004
#define	CG_OVERRIDE_3RD_PERSON_VOF	0x00000008
#define	CG_OVERRIDE_3RD_PERSON_POF	0x00000010
#define	CG_OVERRIDE_3RD_PERSON_CDP	0x00000020
#define	CG_OVERRIDE_3RD_PERSON_APH	0x00000040
#define	CG_OVERRIDE_FOV				0x00000080

typedef struct {
	//NOTE: these probably get cleared in save/load!!!
	int				active;	//bit-flag field of which overrides are active
	int				thirdPersonEntity;	//who to center on
	float			thirdPersonRange;	//how far to be from them
	float			thirdPersonAngle;	//what angle to look at them from
	float			thirdPersonVertOffset;	//how high to be above them
	float			thirdPersonPitchOffset;	//what offset pitch to apply the the camera view
	float			thirdPersonCameraDamp;	//how tightly to move the camera pos behind the player
	float			thirdPersonAlpha;	//how tightly to move the camera pos behind the player
	float			fov;				//what fov to use
	//NOTE: could put Alpha and HorzOffset and the target & camera damps, but no-one is trying to override those, so...
} overrides_t;

//======================================================================


// all cg.stepTime, cg.duckTime, cg.landTime, etc are set to cg.time when the action
// occurs, and they will have visible effects for #define STEP_TIME or whatever msec after

typedef struct {
	int			clientFrame;		// incremented each frame

	qboolean	levelShot;			// taking a level menu screenshot

	// there are only one or two snapshot_t that are relevent at a time
	int			latestSnapshotNum;	// the number of snapshots the client system has received
	int			latestSnapshotTime;	// the time from latestSnapshotNum, so we don't need to read the snapshot yet
	int			processedSnapshotNum;// the number of snapshots cgame has requested
	snapshot_t	*snap;				// cg.snap->serverTime <= cg.time
	snapshot_t	*nextSnap;			// cg.nextSnap->serverTime > cg.time, or NULL

	float		frameInterpolation;	// (float)( cg.time - cg.frame->serverTime ) / (cg.nextFrame->serverTime - cg.frame->serverTime)

	qboolean	thisFrameTeleport;
	qboolean	nextFrameTeleport;

	int			frametime;		// cg.time - cg.oldTime

	int			time;			// this is the time value that the client
								// is rendering at.
	int			oldTime;		// time at last frame, used for missile trails and prediction checking

	int			timelimitWarnings;	// 5 min, 1 min, overtime

	qboolean	renderingThirdPerson;		// during deaths, chasecams, etc

	// prediction state
	qboolean	hyperspace;				// true if prediction has hit a trigger_teleport
	playerState_t	predicted_player_state;
	qboolean	validPPS;				// clear until the first call to CG_PredictPlayerState
	int			predictedErrorTime;
	vec3_t		predictedError;

	float		stepChange;				// for stair up smoothing
	int			stepTime;

	float		duckChange;				// for duck viewheight smoothing
	int			duckTime;

	float		landChange;				// for landing hard
	int			landTime;

	// input state sent to server
	int			weaponSelect;
	int			saberAnimLevelPending;

	// auto rotating items
	vec3_t		autoAngles;
	vec3_t		autoAxis[3];
	vec3_t		autoAnglesFast;
	vec3_t		autoAxisFast[3];

	// view rendering
	refdef_t	refdef;
	vec3_t		refdefViewAngles;		// will be converted to refdef.viewaxis

	// zoom key
	int			zoomMode;		// 0 - not zoomed, 1 - binoculars, 2 - disruptor weapon
	int			zoomDir;		// -1, 1
	int			zoomTime;
	qboolean	zoomLocked;

	// gonk use
	int			batteryChargeTime;

	// FIXME:
	int			forceCrosshairStartTime;
	int			forceCrosshairEndTime;

	// information screen text during loading
	char		infoScreenText[MAX_STRING_CHARS];

	// centerprinting
	int			centerPrintTime;
	int			centerPrintY;
	char		centerPrint[1024];
	int			centerPrintLines;

	// Scrolling text, caption text and LCARS text use this
	char		printText[MAX_PRINTTEXT][128];
	int			printTextY;

	char		captionText[MAX_CAPTIONTEXT][256/*128*/];	// bosted for taiwanese squealy radio static speech in kejim post
	int			captionTextY;

	int			scrollTextLines;	// Number of lines being printed
	int			scrollTextTime;

	int			captionNextTextTime;
	int			captionTextCurrentLine;
	int			captionTextTime;
	int			captionLetterTime;

	// For flashing health armor counter
	int			oldhealth;
	int			oldHealthTime;
	int			oldarmor;
	int			oldArmorTime;
	int			oldammo;
	int			oldAmmoTime;

	// low ammo warning state
	int			lowAmmoWarning;		// 1 = low, 2 = empty

	// crosshair client ID
	int			crosshairClientNum;		//who you're looking at
	int			crosshairClientTime;	//last time you looked at them

	// powerup active flashing
	int			powerupActive;
	int			powerupTime;

	//==========================
	int			creditsStart;

	int			itemPickup;
	int			itemPickupTime;
	int			itemPickupBlendTime;	// the pulse around the crosshair is timed seperately

	float		iconHUDPercent;			// How far into opening sequence the icon HUD is
	int			iconSelectTime;			// How long the Icon HUD has been active
	qboolean	iconHUDActive;

	int			DataPadInventorySelect;		// Current inventory item chosen on Data Pad
	int			DataPadWeaponSelect;		// Current weapon item chosen on Data Pad
	int			DataPadforcepowerSelect;	// Current force power chosen on Data Pad

	qboolean	messageLitActive;			// Flag to show of message lite is active

	int			weaponSelectTime;
	int			weaponAnimation;
	int			weaponAnimationTime;

	int			inventorySelect;		// Current inventory item chosen
	int			inventorySelectTime;

	int			forcepowerSelect;		// Current force power chosen
	int			forcepowerSelectTime;

	// blend blobs
	float		damageTime;
	float		damageX, damageY, damageValue;

	// status bar head
	float		headYaw;
	float		headEndPitch;
	float		headEndYaw;
	int			headEndTime;
	float		headStartPitch;
	float		headStartYaw;
	int			headStartTime;

	int			loadLCARSStage;

	int			missionInfoFlashTime;
	qboolean	missionStatusShow;
	int			missionStatusDeadTime;

	int			forceHUDTotalFlashTime;
	int			forceHUDNextFlashTime;
	qboolean	forceHUDActive;				// Flag to show force hud is off/on

	qboolean	missionFailedScreen;	// qtrue if opened

	int			weaponPickupTextTime;

	int			VHUDFlashTime;
	qboolean	VHUDTurboFlag;
	int			HUDTickFlashTime;
	qboolean	HUDArmorFlag;
	qboolean	HUDHealthFlag;

	// view movement
	float		v_dmg_time;
	float		v_dmg_pitch;
	float		v_dmg_roll;

	int			wonkyTime;		// when interrogator gets you, wonky time controls "drugged" camera view.

	vec3_t		kick_angles;	// weapon kicks
	int			kick_time;		// when the kick happened, so it gets reduced over time

	// temp working variables for player view
	float		bobfracsin;
	int			bobcycle;
	float		xyspeed;

	// development tool
	char			testModelName[MAX_QPATH];
/*
Ghoul2 Insert Start
*/
	int				testModel;
	// had to be moved so we wouldn't wipe these out with the memset - these have STL in them and shouldn't be cleared that way
	snapshot_t	activeSnapshots[2];
	refEntity_t		testModelEntity;
/*
Ghoul2 Insert End
*/
	overrides_t	overrides;	//for overriding certain third-person camera properties

} cg_t;


#define MAX_SHOWPOWERS 12
extern int showPowers[MAX_SHOWPOWERS];
extern const char *showPowersName[MAX_SHOWPOWERS];
extern int force_icons[NUM_FORCE_POWERS];
#define MAX_DPSHOWPOWERS 16

//==============================================================================


#define SG_OFF		0
#define SG_STRING	1
#define SG_GRAPHIC	2
#define SG_NUMBER	3
#define SG_VAR		4

typedef struct
{
	int				type;		// STRING or GRAPHIC
	float			timer;		// When it changes
	int				x;			// X position
	int				y;			// Y positon
	int				width;		// Graphic width
	int				height;		// Graphic height
	char			*file;		// File name of graphic/ text if STRING
	int				ingameEnum;	// Index to ingame_text[]
	qhandle_t		graphic;	// Handle of graphic if GRAPHIC
	int				min;		//
	int				max;
	int				target;		// Final value
	int				inc;
	int				style;
	int				color;		// Normal color
	void			*pointer;		// To an address
} screengraphics_s;


extern	cg_t			cg;
extern	centity_t		cg_entities[MAX_GENTITIES];

extern	centity_t		*cg_permanents[MAX_GENTITIES];
extern	int				cg_numpermanents;

extern	weaponInfo_t	cg_weapons[MAX_WEAPONS];
extern	itemInfo_t		cg_items[MAX_ITEMS];
extern	markPoly_t		cg_markPolys[MAX_MARK_POLYS];


extern	vmCvar_t		cg_runpitch;
extern	vmCvar_t		cg_runroll;
extern	vmCvar_t		cg_bobup;
extern	vmCvar_t		cg_bobpitch;
extern	vmCvar_t		cg_bobroll;
extern	vmCvar_t		cg_shadows;
extern	vmCvar_t		cg_renderToTextureFX;
extern	vmCvar_t		cg_shadowCullDistance;
extern	vmCvar_t		cg_paused;
extern	vmCvar_t		cg_drawTimer;
extern	vmCvar_t		cg_drawFPS;
extern	vmCvar_t		cg_drawSnapshot;
extern	vmCvar_t		cg_drawAmmoWarning;
extern	vmCvar_t		cg_drawCrosshair;
extern	vmCvar_t		cg_dynamicCrosshair;
extern	vmCvar_t		cg_crosshairForceHint;
extern	vmCvar_t		cg_crosshairIdentifyTarget;
extern	vmCvar_t		cg_crosshairX;
extern	vmCvar_t		cg_crosshairY;
extern	vmCvar_t		cg_crosshairSize;
extern	vmCvar_t		cg_drawStatus;
extern	vmCvar_t		cg_drawHUD;
extern	vmCvar_t		cg_draw2D;
extern	vmCvar_t		cg_debugAnim;
#ifndef FINAL_BUILD
extern	vmCvar_t		cg_debugAnimTarget;
extern	vmCvar_t		cg_gun_frame;
#endif
extern	vmCvar_t		cg_gun_x;
extern	vmCvar_t		cg_gun_y;
extern	vmCvar_t		cg_gun_z;
extern	vmCvar_t		cg_debugSaber;
extern	vmCvar_t		cg_debugEvents;
extern	vmCvar_t		cg_errorDecay;
extern	vmCvar_t		cg_footsteps;
extern	vmCvar_t		cg_addMarks;
extern	vmCvar_t		cg_drawGun;
extern	vmCvar_t		cg_autoswitch;
extern	vmCvar_t		cg_simpleItems;
extern	vmCvar_t		cg_fov;
extern	vmCvar_t		cg_fovAspectAdjust;
extern	vmCvar_t		cg_endcredits;
extern	vmCvar_t		cg_updatedDataPadForcePower1;
extern	vmCvar_t		cg_updatedDataPadForcePower2;
extern	vmCvar_t		cg_updatedDataPadForcePower3;
extern	vmCvar_t		cg_updatedDataPadObjective;
extern	vmCvar_t		cg_drawBreath;
extern	vmCvar_t		cg_roffdebug;
#ifndef FINAL_BUILD
extern	vmCvar_t		cg_roffval1;
extern	vmCvar_t		cg_roffval2;
extern	vmCvar_t		cg_roffval3;
extern	vmCvar_t		cg_roffval4;
#endif
extern	vmCvar_t		cg_thirdPerson;
extern	vmCvar_t		cg_thirdPersonRange;
extern	vmCvar_t		cg_thirdPersonMaxRange;
extern	vmCvar_t		cg_thirdPersonAngle;
extern	vmCvar_t		cg_thirdPersonPitchOffset;
extern	vmCvar_t		cg_thirdPersonVertOffset;
extern	vmCvar_t		cg_thirdPersonCameraDamp;
extern	vmCvar_t		cg_thirdPersonTargetDamp;
extern	vmCvar_t		cg_gunAutoFirst;

extern	vmCvar_t		cg_stereoSeparation;
extern	vmCvar_t		cg_developer;
extern	vmCvar_t		cg_timescale;
extern	vmCvar_t		cg_skippingcin;

extern	vmCvar_t		cg_pano;
extern	vmCvar_t		cg_panoNumShots;

extern	vmCvar_t		fx_freeze;
extern	vmCvar_t		fx_debug;

extern	vmCvar_t		cg_missionInfoFlashTime;
extern	vmCvar_t		cg_hudFiles;

extern	vmCvar_t		cg_turnAnims;
extern	vmCvar_t		cg_motionBoneComp;
extern	vmCvar_t		cg_reliableAnimSounds;

extern	vmCvar_t		cg_smoothPlayerPos;
extern	vmCvar_t		cg_smoothPlayerPlat;
extern	vmCvar_t		cg_smoothPlayerPlatAccel;

extern	vmCvar_t		cg_smoothCamera;
extern	vmCvar_t		cg_speedTrail;
extern	vmCvar_t		cg_fovViewmodel;
extern	vmCvar_t		cg_fovViewmodelAdjust;

extern	vmCvar_t		cg_scaleVehicleSensitivity;

void CG_NewClientinfo( int clientNum );
//
// cg_main.c
//
const char *CG_ConfigString( int index );
const char *CG_Argv( int arg );

void QDECL CG_Printf( const char *msg, ... );
NORETURN void QDECL CG_Error( const char *msg, ... );

void CG_StartMusic( qboolean bForceStart );

void CG_UpdateCvars( void );

int CG_CrosshairPlayer( void );
void CG_LoadMenus(const char *menuFile);

//
// cg_view.c
//
void CG_TestModel_f (void);
void CG_TestModelNextFrame_f (void);
void CG_TestModelPrevFrame_f (void);
void CG_TestModelNextSkin_f (void);
void CG_TestModelPrevSkin_f (void);

void CG_ZoomDown_f( void );
void CG_ZoomUp_f( void );

void CG_DrawActiveFrame( int serverTime, stereoFrame_t stereoView );
/*
Ghoul2 Insert Start
*/

void CG_TestG2Model_f (void);
void CG_TestModelSurfaceOnOff_f(void);
void CG_ListModelSurfaces_f (void);
void CG_ListModelBones_f (void);
void CG_TestModelSetAnglespre_f(void);
void CG_TestModelSetAnglespost_f(void);
void CG_TestModelAnimate_f(void);
/*
Ghoul2 Insert End
*/


//
// cg_drawtools.c
//

#define CG_LEFT			0x00000000	// default
#define CG_CENTER		0x00000001
#define CG_RIGHT		0x00000002
#define CG_FORMATMASK	0x00000007
#define CG_SMALLFONT	0x00000010
#define CG_BIGFONT		0x00000020	// default

#define CG_DROPSHADOW	0x00000800
#define CG_BLINK		0x00001000
#define CG_INVERSE		0x00002000
#define CG_PULSE		0x00004000


void CG_DrawRect( float x, float y, float width, float height, float size, const float *color );
void CG_FillRect( float x, float y, float width, float height, const float *color );
void CG_Scissor( float x, float y, float width, float height);
void CG_DrawPic( float x, float y, float width, float height, qhandle_t hShader );
void CG_DrawPic2( float x, float y, float width, float height, float s1, float t1, float s2, float t2, qhandle_t hShader );
void CG_DrawRotatePic( float x, float y, float width, float height,float angle, qhandle_t hShader );
void CG_DrawRotatePic2( float x, float y, float width, float height,float angle, qhandle_t hShader );
void CG_DrawString( float x, float y, const char *string,
				   float charWidth, float charHeight, const float *modulate );
void CG_PrintInterfaceGraphics(int min,int max);
void CG_DrawNumField (int x, int y, int width, int value,int charWidth,int charHeight,int style,qboolean zeroFill);
void CG_DrawProportionalString( int x, int y, const char* str, int style, vec4_t color );


void CG_DrawStringExt( int x, int y, const char *string, const float *setColor,
		qboolean forceColor, qboolean shadow, int charWidth, int charHeight );
void CG_DrawSmallStringColor( int x, int y, const char *s, vec4_t color );

int CG_DrawStrlen( const char *str );

float	*CG_FadeColor( int startMsec, int totalMsec );
void CG_TileClear( void );


//
// cg_draw.c
//
void CG_CenterPrint( const char *str, int y );
void CG_DrawActive( stereoFrame_t stereoView );
void CG_ScrollText( const char *str, int iPixelWidth );
void CG_CaptionText( const char *str, int sound );
void CG_CaptionTextStop( void );

//
// cg_text.c
//
void CG_DrawScrollText( void );
void CG_DrawCaptionText( void );
void CG_DrawCenterString( void );


//
// cg_player.c
//
void CG_AddGhoul2Mark(int type, float size, vec3_t hitloc, vec3_t hitdirection,
				int entnum, vec3_t entposition, float entangle, CGhoul2Info_v &ghoul2, vec3_t modelScale, int lifeTime = 0, int firstModel = 0, vec3_t uaxis = 0);
void CG_Player( centity_t *cent );
void CG_ResetPlayerEntity( centity_t *cent );
void CG_AddRefEntityWithPowerups( refEntity_t *ent, int powerups, centity_t *cent );
void CG_GetTagWorldPosition( refEntity_t *model, char *tag, vec3_t pos, vec3_t axis[3] );

//
// cg_predict.c
//
int	CG_PointContents( const vec3_t point, int passEntityNum );
void CG_Trace( trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
					 const int skipNumber, const int mask, const EG2_Collision eG2TraceType=G2_NOCOLLIDE, const int useLod=0 );
void CG_PredictPlayerState( void );

//
// cg_events.c
//
void CG_CheckEvents( centity_t *cent );
const char	*CG_PlaceString( int rank );
void CG_EntityEvent( centity_t *cent, vec3_t position );


//
// cg_ents.c
//
vec3_t *CG_SetEntitySoundPosition( centity_t *cent );
void CG_AddPacketEntities( qboolean isPortal );
void CG_Beam( centity_t *cent, int color );
void CG_AdjustPositionForMover( const vec3_t in, int moverNum, int atTime, vec3_t out );

void CG_PositionEntityOnTag( refEntity_t *entity, const refEntity_t *parent,
							qhandle_t parentModel, char *tagName );
void CG_PositionRotatedEntityOnTag( refEntity_t *entity, const refEntity_t *parent,
							qhandle_t parentModel, char *tagName, orientation_t *tagOrient );

/*
Ghoul2 Insert Start
*/
void ScaleModelAxis(refEntity_t	*ent);
/*
Ghoul2 Insert End
*/


//
// cg_weapons.c
//
void CG_NextWeapon_f( void );
void CG_PrevWeapon_f( void );
void CG_Weapon_f( void );
void CG_DPNextWeapon_f( void );
void CG_DPPrevWeapon_f( void );
void CG_DPNextInventory_f( void );
void CG_DPPrevInventory_f( void );
void CG_DPNextForcePower_f( void );
void CG_DPPrevForcePower_f( void );


void CG_RegisterWeapon( int weaponNum );
void CG_RegisterItemVisuals( int itemNum );
void CG_RegisterItemSounds( int itemNum );

void CG_FireWeapon( centity_t *cent, qboolean alt_fire );

void CG_AddViewWeapon (playerState_t *ps);
void CG_DrawWeaponSelect( void );

void CG_OutOfAmmoChange( void );	// should this be in pmove?

//
// cg_marks.c
//
void	CG_InitMarkPolys( void );
void	CG_AddMarks( void );
void	CG_ImpactMark( qhandle_t markShader,
				    const vec3_t origin, const vec3_t dir,
					float orientation,
				    float r, float g, float b, float a,
					qboolean alphaFade,
					float radius, qboolean temporary );

//
// cg_localents.c
//
void	CG_InitLocalEntities( void );
localEntity_t	*CG_AllocLocalEntity( void );
void	CG_AddLocalEntities( void );

//
// cg_effects.c
//

/*localEntity_t *CG_MakeExplosion( vec3_t origin, vec3_t dir,
								qhandle_t hModel, int numframes, qhandle_t shader, int msec,
								qboolean isSprite, float scale = 1.0f );// Overloaded

localEntity_t *CG_MakeExplosion( vec3_t origin, vec3_t dir,
								qhandle_t hModel, int numframes, qhandle_t shader, int msec,
								qboolean isSprite, float scale, int flags );// Overloaded
*/
localEntity_t *CG_AddTempLight( vec3_t origin, float scale, vec3_t color, int msec );

void CG_TestLine( vec3_t start, vec3_t end, int time, unsigned int color, int radius);

//
// cg_snapshot.c
//
void CG_ProcessSnapshots( void );

//
// cg_info.c
//
void CG_DrawInformation( void );

//
// cg_scoreboard.c
//
qboolean CG_DrawScoreboard( void );
extern void CG_MissionCompletion(void);

//
// cg_consolecmds.c
//
qboolean CG_ConsoleCommand( void );
void CG_InitConsoleCommands( void );

//
// cg_servercmds.c
//
void CG_ExecuteNewServerCommands( int latestSequence );
void CG_ParseServerinfo( void );

//
// cg_playerstate.c
//
void CG_Respawn( void );
void CG_TransitionPlayerState( playerState_t *ps, playerState_t *ops );

// cg_credits.cpp
//
void CG_Credits_Init( const char *psStripReference, vec4_t *pv4Color);
qboolean CG_Credits_Running( void );
qboolean CG_Credits_Draw( void );


//===============================================

//
// system calls
// These functions are how the cgame communicates with the main game system
//

// print message on the local console
void	cgi_Printf( const char *fmt );

// abort the game
NORETURN void	cgi_Error( const char *fmt );

// milliseconds should only be used for performance tuning, never
// for anything game related.  Get time from the CG_DrawActiveFrame parameter
int		cgi_Milliseconds( void );

// console variable interaction
void	cgi_Cvar_Register( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags );
void	cgi_Cvar_Update( vmCvar_t *vmCvar );
void	cgi_Cvar_Set( const char *var_name, const char *value );


// ServerCommand and ConsoleCommand parameter access
int		cgi_Argc( void );
void	cgi_Argv( int n, char *buffer, int bufferLength );
void	cgi_Args( char *buffer, int bufferLength );

// filesystem access
// returns length of file
int		cgi_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
int		cgi_FS_Read( void *buffer, int len, fileHandle_t f );
int		cgi_FS_Write( const void *buffer, int len, fileHandle_t f );
void	cgi_FS_FCloseFile( fileHandle_t f );

// add commands to the local console as if they were typed in
// for map changing, etc.  The command is not executed immediately,
// but will be executed in order the next time console commands
// are processed
void	cgi_SendConsoleCommand( const char *text );

// register a command name so the console can perform command completion.
// FIXME: replace this with a normal console command "defineCommand"?
void	cgi_AddCommand( const char *cmdName );

// send a string to the server over the network
void	cgi_SendClientCommand( const char *s );

// force a screen update, only used during gamestate load
void	cgi_UpdateScreen( void );

//RMG
void	cgi_RMG_Init(int terrainID, const char *terrainInfo);
int		cgi_CM_RegisterTerrain(const char *terrainInfo);
void	cgi_RE_InitRendererTerrain( const char *terrainInfo );

// model collision
void	cgi_CM_LoadMap( const char *mapname, qboolean subBSP );
int		cgi_CM_NumInlineModels( void );
clipHandle_t cgi_CM_InlineModel( int index );		// 0 = world, 1+ = bmodels
clipHandle_t cgi_CM_TempBoxModel( const vec3_t mins, const vec3_t maxs );//, const int contents );
int		cgi_CM_PointContents( const vec3_t p, clipHandle_t model );
int		cgi_CM_TransformedPointContents( const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles );
void	cgi_CM_BoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
						  const vec3_t mins, const vec3_t maxs,
						  clipHandle_t model, int brushmask );
void	cgi_CM_TransformedBoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
						  const vec3_t mins, const vec3_t maxs,
						  clipHandle_t model, int brushmask,
						  const vec3_t origin, const vec3_t angles );

// Returns the projection of a polygon onto the solid brushes in the world
int		cgi_CM_MarkFragments( int numPoints, const vec3_t *points,
				const vec3_t projection,
				int maxPoints, vec3_t pointBuffer,
				int maxFragments, markFragment_t *fragmentBuffer );

// normal sounds will have their volume dynamically changed as their entity
// moves and the listener moves
void	cgi_S_StartSound( const vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx );
void	cgi_S_StopSounds( void );

// a local sound is always played full volume
void	cgi_S_StartLocalSound( sfxHandle_t sfx, int channelNum );
void	cgi_S_ClearLoopingSounds( void );
void	cgi_S_AddLoopingSound(int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx, soundChannel_t chan = CHAN_AUTO );
void	cgi_S_UpdateEntityPosition( int entityNum, const vec3_t origin );

// repatialize recalculates the volumes of sound as they should be heard by the
// given entityNum and position
void	cgi_S_Respatialize( int entityNum, const vec3_t origin, vec3_t axis[3], qboolean inwater );
sfxHandle_t	cgi_S_RegisterSound( const char *sample );		// returns buzz if not found
void	cgi_S_StartBackgroundTrack( const char *intro, const char *loop, qboolean bForceStart );	// empty name stops music
float	cgi_S_GetSampleLength( sfxHandle_t sfx);

void	cgi_R_LoadWorldMap( const char *mapname );

// all media should be registered during level startup to prevent
// hitches during gameplay
qhandle_t	cgi_R_RegisterModel( const char *name );			// returns rgb axis if not found
qhandle_t	cgi_R_RegisterSkin( const char *name );
qhandle_t	cgi_R_RegisterShader( const char *name );			// returns default shader if not found
qhandle_t	cgi_R_RegisterShaderNoMip( const char *name );			// returns all white if not found
qhandle_t	cgi_R_RegisterFont( const char *name );
int			cgi_R_Font_StrLenPixels(const char *text, const int iFontIndex, const float scale = 1.0f);
int			cgi_R_Font_StrLenChars(const char *text);
int			cgi_R_Font_HeightPixels(const int iFontIndex, const float scale = 1.0f);
void		cgi_R_Font_DrawString(int ox, int oy, const char *text, const float *rgba, const int setIndex, int iMaxPixelWidth, const float scale = 1.0f);
qboolean	cgi_Language_IsAsian(void);
qboolean	cgi_Language_UsesSpaces(void);
unsigned	cgi_AnyLanguage_ReadCharFromString( const char *psText, int *iAdvanceCount, qboolean *pbIsTrailingPunctuation = NULL );

void	cgi_R_SetRefractProp(float alpha, float stretch, qboolean prepost, qboolean negate);

// a scene is built up by calls to R_ClearScene and the various R_Add functions.
// Nothing is drawn until R_RenderScene is called.
void	cgi_R_ClearScene( void );
void	cgi_R_AddRefEntityToScene( const refEntity_t *re );
void	cgi_R_GetLighting( const vec3_t origin, vec3_t ambientLight, vec3_t directedLight, vec3_t ligthDir );

//used by miscents
qboolean	cgi_R_inPVS( vec3_t p1, vec3_t p2 );

// polys are intended for simple wall marks, not really for doing
// significant construction
void	cgi_R_AddPolyToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts );
void	cgi_R_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b );
void	cgi_R_RenderScene( const refdef_t *fd );
void	cgi_R_SetColor( const float *rgba );	// NULL = 1,1,1,1
void	cgi_R_DrawStretchPic( float x, float y, float w, float h,
	float s1, float t1, float s2, float t2, qhandle_t hShader );

void	cgi_R_ModelBounds( qhandle_t model, vec3_t mins, vec3_t maxs );
void	cgi_R_LerpTag( orientation_t *tag, qhandle_t mod, int startFrame, int endFrame,
					 float frac, const char *tagName );
// Does weird, barely controllable rotation behaviour
void	cgi_R_DrawRotatePic( float x, float y, float w, float h,
	float s1, float t1, float s2, float t2,float a, qhandle_t hShader );
// rotates image around exact center point of passed in coords
void	cgi_R_DrawRotatePic2( float x, float y, float w, float h,
	float s1, float t1, float s2, float t2,float a, qhandle_t hShader );
void	cgi_R_SetRangeFog(float range);
void	cgi_R_LAGoggles( void );
void	cgi_R_Scissor( float x, float y, float w, float h);

// The glconfig_t will not change during the life of a cgame.
// If it needs to change, the entire cgame will be restarted, because
// all the qhandle_t are then invalid.
void		cgi_GetGlconfig( glconfig_t *glconfig );

// the gamestate should be grabbed at startup, and whenever a
// configstring changes
void		cgi_GetGameState( gameState_t *gamestate );

// cgame will poll each frame to see if a newer snapshot has arrived
// that it is interested in.  The time is returned seperately so that
// snapshot latency can be calculated.
void		cgi_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime );

// a snapshot get can fail if the snapshot (or the entties it holds) is so
// old that it has fallen out of the client system queue
qboolean	cgi_GetSnapshot( int snapshotNumber, snapshot_t *snapshot );

qboolean	cgi_GetDefaultState(int entityIndex, entityState_t *state );

// retrieve a text command from the server stream
// the current snapshot will hold the number of the most recent command
// qfalse can be returned if the client system handled the command
// argc() / argv() can be used to examine the parameters of the command
qboolean	cgi_GetServerCommand( int serverCommandNumber );

// returns the most recent command number that can be passed to GetUserCmd
// this will always be at least one higher than the number in the current
// snapshot, and it may be quite a few higher if it is a fast computer on
// a lagged connection
int			cgi_GetCurrentCmdNumber( void );
qboolean	cgi_GetUserCmd( int cmdNumber, usercmd_t *ucmd );

// used for the weapon select and zoom
void		cgi_SetUserCmdValue( int stateValue, float sensitivityScale, float mPitchOverride, float mYawOverride );
void		cgi_SetUserCmdAngles( float pitchOverride, float yawOverride, float rollOverride );

void		cgi_S_UpdateAmbientSet( const char *name, vec3_t origin );
void		cgi_AS_ParseSets( void );
void		cgi_AS_AddPrecacheEntry( const char *name );
int			cgi_S_AddLocalSet( const char *name, vec3_t listener_origin, vec3_t origin, int entID, int time );
sfxHandle_t	cgi_AS_GetBModelSound( const char *name, int stage );


void CG_DrawMiscEnts(void);


//-----------------------------
// Effects related prototypes
//-----------------------------

// Weapon prototypes
void FX_Saber( vec3_t start, vec3_t normal, float height, float radius, saber_colors_t color );

void FX_BryarHitWall( vec3_t origin, vec3_t normal );
void FX_BryarAltHitWall( vec3_t origin, vec3_t normal, int power );
void FX_BryarHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid );
void FX_BryarAltHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid );

void FX_BlasterProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BlasterAltFireThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BlasterWeaponHitWall( vec3_t origin, vec3_t normal );
void FX_BlasterWeaponHitPlayer( gentity_t *hit, vec3_t origin, vec3_t normal, qboolean humanoid );

void FX_DisruptorMainShot( vec3_t start, vec3_t end );
void FX_DisruptorAltShot( vec3_t start, vec3_t end, qboolean full );
void FX_DisruptorAltMiss( vec3_t origin, vec3_t normal );

void FX_BowcasterHitWall( vec3_t origin, vec3_t normal );
void FX_BowcasterHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid );

void FX_RepeaterHitWall( vec3_t origin, vec3_t normal );
void FX_RepeaterAltHitWall( vec3_t origin, vec3_t normal );
void FX_RepeaterHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid );
void FX_RepeaterAltHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid );

void FX_DEMP2_HitWall( vec3_t origin, vec3_t normal );
void FX_DEMP2_HitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid );
void FX_DEMP2_AltDetonate( vec3_t org, float size );

void FX_FlechetteProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_FlechetteWeaponHitWall( vec3_t origin, vec3_t normal );
void FX_FlechetteWeaponHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid );

void FX_RocketHitWall( vec3_t origin, vec3_t normal );
void FX_RocketHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid );

void FX_ConcProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_ConcHitWall( vec3_t origin, vec3_t normal );
void FX_ConcHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid );
void FX_ConcAltShot( vec3_t start, vec3_t end );
void FX_ConcAltMiss( vec3_t origin, vec3_t normal );

void FX_EmplacedHitWall( vec3_t origin, vec3_t normal, qboolean eweb );
void FX_EmplacedHitPlayer( vec3_t origin, vec3_t normal, qboolean eweb );
void FX_EmplacedProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );

void FX_ATSTMainHitWall( vec3_t origin, vec3_t normal );
void FX_ATSTMainHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid );
void FX_ATSTMainProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );

void FX_TuskenShotProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_TuskenShotWeaponHitWall( vec3_t origin, vec3_t normal );
void FX_TuskenShotWeaponHitPlayer( gentity_t *hit, vec3_t origin, vec3_t normal, qboolean humanoid );

void FX_NoghriShotProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_NoghriShotWeaponHitWall( vec3_t origin, vec3_t normal );
void FX_NoghriShotWeaponHitPlayer( gentity_t *hit, vec3_t origin, vec3_t normal, qboolean humanoid );

void CG_BounceEffect( centity_t *cent, int weapon, vec3_t origin, vec3_t normal );
void CG_MissileStick( centity_t *cent, int weapon, vec3_t origin );

void CG_MissileHitPlayer( centity_t *cent, int weapon, vec3_t origin, vec3_t dir, qboolean altFire );
void CG_MissileHitWall( centity_t *cent, int weapon, vec3_t origin, vec3_t dir, qboolean altFire );

void CG_DrawTargetBeam( vec3_t start, vec3_t end, vec3_t norm, const char *beamFx, const char *impactFx );

qboolean CG_VehicleWeaponImpact( centity_t *cent );


/*
Ghoul2 Insert Start
*/
// CG specific API access
void		trap_G2_SetGhoul2ModelIndexes(CGhoul2Info_v &ghoul2, qhandle_t *modelList, qhandle_t *skinList);
void		CG_Init_CG(void);

void CG_SetGhoul2Info( refEntity_t *ent, centity_t *cent);

/*
Ghoul2 Insert End
*/
void	trap_Com_SetOrgAngles(vec3_t org,vec3_t angles);
void	trap_R_GetLightStyle(int style, color4ub_t color);
void	trap_R_SetLightStyle(int style, int color);

int		trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits, const char *psAudioFile = NULL);
e_status trap_CIN_StopCinematic(int handle);
e_status trap_CIN_RunCinematic (int handle);
void	trap_CIN_DrawCinematic (int handle);
void	trap_CIN_SetExtents (int handle, int x, int y, int w, int h);
void	*cgi_Z_Malloc( int size, int tag );
void	cgi_Z_Free( void *ptr );

int		cgi_SP_GetStringTextString(const char *text, char *buf, int bufferlength);


void	cgi_UI_Menu_Reset( void );
void	cgi_UI_Menu_New(char *buf );
void	cgi_UI_Menu_OpenByName(char *buf);
void	cgi_UI_SetActive_Menu(char *name);
void	cgi_UI_Parse_Int(int *value);
void	cgi_UI_Parse_String(char *buf);
void	cgi_UI_Parse_Float(float *value);
int		cgi_UI_StartParseSession(char *menuFile,char **buf);
void	cgi_UI_ParseExt(char **token);
void	cgi_UI_MenuPaintAll(void);
void	cgi_UI_MenuCloseAll(void);
void	cgi_UI_String_Init(void);
int		cgi_UI_GetMenuItemInfo(const char *menuFile,const char *itemName,int *x,int *y,int *w,int *h,vec4_t color,qhandle_t *background);
int		cgi_UI_GetMenuInfo(char *menuFile,int *x,int *y,int *w,int *h);
void	cgi_UI_Menu_Paint( void *menu, qboolean force );
void	*cgi_UI_GetMenuByName( const char *menu );


void	SetWeaponSelectTime(void);

void CG_PlayEffectBolted( const char *fxName, const int modelIndex, const int boltIndex, const int entNum, vec3_t origin, int iLoopTime=0, const bool isRelative=false );
void CG_PlayEffectIDBolted( const int fxID, const int modelIndex, const int boltIndex, const int entNum, vec3_t origin, int iLoopTime=0, const bool isRelative=false );
void CG_PlayEffectOnEnt( const char *fxName, const int clientNum, vec3_t origin, const vec3_t fwd );
void CG_PlayEffectIDOnEnt( const int fxID, const int clientNum, vec3_t origin, const vec3_t fwd );
void CG_PlayEffect( const char *fxName, vec3_t origin, const vec3_t fwd );
void CG_PlayEffectID( const int fxID, vec3_t origin, const vec3_t fwd );

void	CG_ClearLightStyles( void );
void	CG_RunLightStyles( void );
void	CG_SetLightstyle( int i );

int CG_MagicFontToReal( int menuFontIndex );

#endif	//__CG_LOCAL_H__
