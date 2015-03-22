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

#ifndef __CG_MEDIA_H_
#define __CG_MEDIA_H_

#define	NUM_CROSSHAIRS		9

typedef enum {
	FOOTSTEP_NORMAL,
	FOOTSTEP_METAL,
	FOOTSTEP_SPLASH,
	FOOTSTEP_WADE,
	FOOTSTEP_SWIM,

	FOOTSTEP_TOTAL
} footstep_t;

#define ICON_WEAPONS	0
#define ICON_FORCE		1
#define ICON_INVENTORY	2

#define MAX_TICS	14

typedef struct forceTicPos_s
{
	int				x;
	int				y;
	int				width;
	int				height;
	char			*file;
	qhandle_t		tic;
} forceTicPos_t;
extern forceTicPos_t forceTicPos[];
extern forceTicPos_t ammoTicPos[];

#define NUM_CHUNK_MODELS	4

enum
{
	CHUNK_METAL1 = 0,
	CHUNK_METAL2,
	CHUNK_ROCK1,
	CHUNK_ROCK2,
	CHUNK_ROCK3,
	CHUNK_CRATE1,
	CHUNK_CRATE2,
	CHUNK_WHITE_METAL,
	NUM_CHUNK_TYPES
};

// all of the model, shader, and sound references that are
// loaded at gamestate time are stored in cgMedia_t
// Other media that can be tied to clients, weapons, or items are
// stored in the clientInfo_t, itemInfo_t, weaponInfo_t, and powerupInfo_t
typedef struct {
	qhandle_t	charsetShader;
	qhandle_t	whiteShader;

	qhandle_t	selectShader;
	qhandle_t	crosshairShader[NUM_CROSSHAIRS];
	qhandle_t	backTileShader;
	qhandle_t	noammoShader;

	qhandle_t	numberShaders[11];
	qhandle_t	smallnumberShaders[11];
	qhandle_t	chunkyNumberShaders[11];

	qhandle_t	loadTick;
	qhandle_t	loadTickCap;

	//			HUD artwork
	int			currentBackground;
	qhandle_t	weaponbox;
	qhandle_t	weaponIconBackground;
	qhandle_t	weaponProngsOff;
	qhandle_t	weaponProngsOn;
	qhandle_t	forceIconBackground;
	qhandle_t	forceProngsOn;
	qhandle_t	inventoryIconBackground;
	qhandle_t	inventoryProngsOn;
	qhandle_t	ladyLuckHealthShader;
	qhandle_t	turretComputerOverlayShader;
	qhandle_t	turretCrossHairShader;

	int			currentDataPadIconBackground;

//Chunks
	qhandle_t	chunkModels[NUM_CHUNK_TYPES][4];
	sfxHandle_t	chunkSound;
	sfxHandle_t	grateSound;
	sfxHandle_t	rockBreakSound;
	sfxHandle_t	rockBounceSound[2];
	sfxHandle_t	metalBounceSound[2];
	sfxHandle_t	glassChunkSound;
	sfxHandle_t	crateBreakSound[2];

	// Saber shaders
	//-----------------------------
	qhandle_t	forceCoronaShader;
	qhandle_t	saberBlurShader;
	qhandle_t	yellowDroppedSaberShader; // glow

	qhandle_t	redSaberGlowShader;
	qhandle_t	redSaberCoreShader;
	qhandle_t	orangeSaberGlowShader;
	qhandle_t	orangeSaberCoreShader;
	qhandle_t	yellowSaberGlowShader;
	qhandle_t	yellowSaberCoreShader;
	qhandle_t	greenSaberGlowShader;
	qhandle_t	greenSaberCoreShader;
	qhandle_t	blueSaberGlowShader;
	qhandle_t	blueSaberCoreShader;
	qhandle_t	purpleSaberGlowShader;
	qhandle_t	purpleSaberCoreShader;

	qhandle_t	explosionModel;
	qhandle_t	surfaceExplosionShader;

	qhandle_t	solidWhiteShader;
	qhandle_t	electricBodyShader;
	qhandle_t	electricBody2Shader;
	qhandle_t	shieldShader;
	qhandle_t	boltShader;

	// Disruptor zoom graphics
	qhandle_t	disruptorMask;
	qhandle_t	disruptorInsert;
	qhandle_t	disruptorLight;
	qhandle_t	disruptorInsertTick;

	// Binocular graphics
	qhandle_t	binocularCircle;
	qhandle_t	binocularMask;
	qhandle_t	binocularArrow;
	qhandle_t	binocularTri;
	qhandle_t	binocularStatic;
	qhandle_t	binocularOverlay;

	// LA Goggles graphics
	qhandle_t	laGogglesStatic;
	qhandle_t	laGogglesMask;
	qhandle_t	laGogglesSideBit;
	qhandle_t	laGogglesBracket;
	qhandle_t	laGogglesArrow;

	// wall mark shaders
	qhandle_t	phaserMarkShader;
	qhandle_t	scavMarkShader;
	qhandle_t	bulletmarksShader;
	qhandle_t	rivetMarkShader;

	qhandle_t	shadowMarkShader;
	qhandle_t	wakeMarkShader;

	qhandle_t	damageBlendBlobShader;

	// fonts...
	//
	qhandle_t	qhFontSmall;
	qhandle_t	qhFontMedium;

	// special effects models / etc.
	qhandle_t	personalShieldShader;
	qhandle_t	cloakedShader;

	// Mission objectives
	qhandle_t	objcorner1;
	qhandle_t	objcorner2;
	qhandle_t	objcorner3;
	qhandle_t	pending;
	qhandle_t	notpending;

	// Interface media
	qhandle_t	ammoweapon;
	qhandle_t	ammoslider;
	qhandle_t	emplacedHealthBarShader;

	qhandle_t	HUDLeftFrame;
	qhandle_t	HUDArmor1;
	qhandle_t	HUDArmor2;
	qhandle_t	HUDHealth;
	qhandle_t	HUDHealthTic;
	qhandle_t	HUDArmorTic;
	qhandle_t	HUDInnerLeft;

	qhandle_t	HUDSaberStyleFast;
	qhandle_t	HUDSaberStyleMed;
	qhandle_t	HUDSaberStyleStrong;

	qhandle_t	HUDRightFrame;
	qhandle_t	HUDInnerRight;

	qhandle_t	dataPadFrame;
	qhandle_t	DPForcePowerOverlay;

	qhandle_t	talkingtop;
	qhandle_t	talkingbot;

	qhandle_t	bracketlu;
	qhandle_t	bracketld;
	qhandle_t	bracketru;
	qhandle_t	bracketrd;

	qhandle_t	messageLitOn;
	qhandle_t	messageLitOff;
	qhandle_t	messageObjCircle;

	qhandle_t	batteryChargeShader;

	qhandle_t	levelLoad;

	// sounds
	sfxHandle_t disintegrateSound;
	sfxHandle_t disintegrate2Sound;

	sfxHandle_t	grenadeBounce1;
	sfxHandle_t	grenadeBounce2;

	sfxHandle_t	flechetteStickSound;
	sfxHandle_t	detPackStickSound;
	sfxHandle_t	tripMineStickSound;

	sfxHandle_t	selectSound;
	sfxHandle_t	selectSound2;
	sfxHandle_t	overchargeSlowSound;
	sfxHandle_t overchargeFastSound;
	sfxHandle_t	overchargeLoopSound;
	sfxHandle_t	overchargeEndSound;

	sfxHandle_t	useNothingSound;
	sfxHandle_t	footsteps[FOOTSTEP_TOTAL][4];

	sfxHandle_t talkSound;
	sfxHandle_t	noAmmoSound;

	sfxHandle_t landSound;
	sfxHandle_t rollSound;
	sfxHandle_t messageLitSound;

	sfxHandle_t interfaceSnd1;
	sfxHandle_t interfaceSnd2;
	sfxHandle_t interfaceSnd3;

	sfxHandle_t	batteryChargeSound;

	sfxHandle_t watrInSound;
	sfxHandle_t watrOutSound;
	sfxHandle_t watrUnSound;

	// Zoom
	sfxHandle_t	zoomStart;
	sfxHandle_t	zoomLoop;
	sfxHandle_t	zoomEnd;
	sfxHandle_t	disruptorZoomLoop;

	//blaster reflection sounds
	sfxHandle_t blasterReflectSound[3];

} cgMedia_t;


// Stored FX handles
//--------------------
typedef struct
{
	// BRYAR PISTOL
	fxHandle_t	bryarShotEffect;
	fxHandle_t	bryarPowerupShotEffect;
	fxHandle_t	bryarWallImpactEffect;
	fxHandle_t	bryarWallImpactEffect2;
	fxHandle_t	bryarWallImpactEffect3;
	fxHandle_t	bryarFleshImpactEffect;

	// BLASTER
	fxHandle_t	blasterShotEffect;
	fxHandle_t	blasterOverchargeEffect;
	fxHandle_t	blasterWallImpactEffect;
	fxHandle_t	blasterFleshImpactEffect;

	// BOWCASTER
	fxHandle_t	bowcasterShotEffect;
	fxHandle_t	bowcasterBounceEffect;
	fxHandle_t	bowcasterImpactEffect;

	// FLECHETTE
	fxHandle_t	flechetteShotEffect;
	fxHandle_t	flechetteAltShotEffect;
	fxHandle_t	flechetteShotDeathEffect;
	fxHandle_t	flechetteFleshImpactEffect;
	fxHandle_t	flechetteRicochetEffect;

	//FORCE
	fxHandle_t	forceConfusion;
	fxHandle_t	forceLightning;
	fxHandle_t	forceLightningWide;
	fxHandle_t	forceInvincibility;
	fxHandle_t	forceHeal;

} cgEffects_t;


// The client game static (cgs) structure hold everything
// loaded or calculated from the gamestate.  It will NOT
// be cleared when a tournement restart is done, allowing
// all clients to begin playing instantly
#define STRIPED_LEVELNAME_VARIATIONS 3	// sigh, to cope with levels that use text from >1 SP file (plus 1 for common)
typedef struct {
	gameState_t		gameState;			// gamestate from server
	glconfig_t		glconfig;			// rendering configuration

	int				serverCommandSequence;	// reliable command stream counter

	// parsed from serverinfo
	int				dmflags;
	int				teamflags;
	int				timelimit;
	int				maxclients;
	char			mapname[MAX_QPATH];
	char			stripLevelName[STRIPED_LEVELNAME_VARIATIONS][MAX_QPATH];

	//
	// locally derived information from gamestate
	//
	qhandle_t		model_draw[MAX_MODELS];
	sfxHandle_t		sound_precache[MAX_SOUNDS];
// Ghoul2 start
	qhandle_t		skins[MAX_CHARSKINS];

// Ghoul2 end

	int				numInlineModels;
	qhandle_t		inlineDrawModel[MAX_SUBMODELS];
	vec3_t			inlineModelMidpoints[MAX_SUBMODELS];

	clientInfo_t	clientinfo[MAX_CLIENTS];

	// media
	cgMedia_t		media;

	// effects
	cgEffects_t		effects;

} cgs_t;

extern	cgs_t			cgs;

#endif //__CG_MEDIA_H_
