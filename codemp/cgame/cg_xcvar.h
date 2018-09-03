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

#ifdef XCVAR_PROTO
	#define XCVAR_DEF( name, defVal, update, flags ) extern vmCvar_t name;
#endif

#ifdef XCVAR_DECL
	#define XCVAR_DEF( name, defVal, update, flags ) vmCvar_t name;
#endif

#ifdef XCVAR_LIST
	#define XCVAR_DEF( name, defVal, update, flags ) { & name , #name , defVal , update , flags },
#endif

		//name				default value	update function			flag(s)
XCVAR_DEF( g_forceRegenTime,		"0",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cl_currentServerAddress,	"0",	NULL,					CVAR_ROM )

//JAPRO HUD / DISPLAY
XCVAR_DEF( cg_movementKeys,			"0",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_movementKeysX,		"465",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_movementKeysY,		"432",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_movementKeysSize,		"1.0",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_speedometerSettings,	"0",	NULL,					CVAR_ARCHIVE ) //bitvalue
XCVAR_DEF( cg_speedometerX,			"132",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_speedometerY,			"459",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_speedometerSize,		"0.75",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_drawTeamOverlay,		"0",	CG_TeamOverlayChange,	CVAR_ARCHIVE )
XCVAR_DEF( cg_drawTeamOverlayX,		"640",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_drawTeamOverlayY,		"0",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_raceTimer,			"2",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_raceTimerSize,		"0.75",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_raceTimerX,			"5",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_raceTimerY,			"280",	NULL,					CVAR_ARCHIVE ) 
XCVAR_DEF( cg_smallScoreboard,		"0",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_scoreDeaths,			"1",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_killMessage,			"1",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_newFont,				"0",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_chatBox,				"10000",NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_chatBoxFontSize,		"1",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_chatBoxHeight,		"350",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_chatBoxX,				"30",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_chatBoxCutOffLength,	"350",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_crosshairRed,			"0",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_crosshairGreen,		"0",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_crosshairBlue,		"0",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_crosshairAlpha,		"255",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_hudColors,			"0",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_tintHud,				"1",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_drawScore,			"2",	NULL,					CVAR_ARCHIVE ) //score counter on HUD
XCVAR_DEF( cg_drawScores,			"1",	NULL,					CVAR_ARCHIVE ) //team score counter in top right
XCVAR_DEF( cg_drawVote,				"1",	NULL,					CVAR_ARCHIVE )

//Strafehelper
XCVAR_DEF( cg_strafeHelper,						"3008",	NULL,		CVAR_ARCHIVE )
XCVAR_DEF( cg_strafeHelper_FPS,					"0",	NULL,		CVAR_ARCHIVE ) //fats _ syntax to follow smod ;s
XCVAR_DEF( cg_strafeHelperOffset,				"75",	NULL,		CVAR_ARCHIVE )
XCVAR_DEF( cg_strafeHelperInvertOffset,			"75",	NULL,		CVAR_ARCHIVE )
XCVAR_DEF( cg_strafeHelperLineWidth,			"1",	NULL,		CVAR_ARCHIVE )

//Sounds
XCVAR_DEF( cg_rollSounds,						"1",	NULL,		CVAR_ARCHIVE )
XCVAR_DEF( cg_jumpSounds,						"0",	NULL,		CVAR_ARCHIVE )
XCVAR_DEF( cg_chatSounds,						"1",	NULL,		CVAR_ARCHIVE )
XCVAR_DEF( cg_hitsounds,						"0",	NULL,		CVAR_ARCHIVE )
XCVAR_DEF( cg_raceSounds,						"1",	NULL,		CVAR_ARCHIVE ) //Bitvalue, but so far we just have RS_TIMER_START set up

//Visuals
XCVAR_DEF( cg_remaps,							"1",	NULL,		CVAR_ARCHIVE|CVAR_LATCH )
XCVAR_DEF( cg_screenShake,						"2",	NULL,		CVAR_ARCHIVE )
XCVAR_DEF( cg_drawScreenTints,					"1",	NULL,		CVAR_ARCHIVE )
XCVAR_DEF( cg_smoothCamera,						"0",	NULL,		CVAR_ARCHIVE )
XCVAR_DEF( cg_blood,							"0",	NULL,		CVAR_ARCHIVE ) //JAPRO - Clientside - re add cg_blood
XCVAR_DEF( cg_thirdPersonFlagAlpha,				"1",	NULL,		CVAR_ARCHIVE )
XCVAR_DEF( cg_stylePlayer,						"0",	NULL,		CVAR_ARCHIVE )

XCVAR_DEF( cg_alwaysShowAbsorb,					"0",	NULL,		CVAR_ARCHIVE )
XCVAR_DEF( cg_zoomFov,							"30.0",	NULL,		CVAR_ARCHIVE )					
XCVAR_DEF( cg_fleshSparks,						"7",	NULL,		CVAR_ARCHIVE )
XCVAR_DEF( cg_noFX,								"0",	NULL,		CVAR_ARCHIVE )
XCVAR_DEF( cg_noTeleFX,							"0",	NULL,		CVAR_ARCHIVE )
XCVAR_DEF( cl_ratioFix,							"1",	CG_Set2DRatio,	CVAR_ARCHIVE ) // Shared with UI module

//Features
XCVAR_DEF( cg_simulatedProjectiles,				"0",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_simulatedHitscan,					"1",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_spectatorCameraDamp,				"0",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_scopeSensitivity,					"1",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_defaultModelRandom,				"1",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_defaultModel,						"kyle",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_defaultFemaleModel,				"jan",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_forceModel,						"0",	CG_ForceModelChange,	CVAR_ARCHIVE )
XCVAR_DEF( cg_forceAllyModel,					"none",	CG_ForceModelChange,	CVAR_ARCHIVE )
XCVAR_DEF( cg_forceEnemyModel,					"none",	CG_ForceModelChange,	CVAR_ARCHIVE )
XCVAR_DEF( cg_jumpHeight,						"0",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_zoomSensitivity,					"0",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_leadIndicator,					"0",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_drawHitBox,						"0",	NULL,					0 )
XCVAR_DEF( cg_drawPlayerNames,					"0",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_drawPlayerNamesScale,				"0.5",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_autoScreenshot,					"0",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_autoRecordDemo,					"0",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_autoRecordRaceDemo,				"0",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_autoKillWhenFalling,				"0",	NULL,					CVAR_ARCHIVE )
#ifdef WIN32
XCVAR_DEF( cg_engineModifications,				"1", CG_MemoryPatchChange,		CVAR_ARCHIVE ) //should remove
#endif


//Auto login
XCVAR_DEF( cg_autoLoginServer1,					"0",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_autoLoginPass1,					"0",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_autoLoginServer2,					"0",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_autoLoginPass2,					"0",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_autoLoginServer3,					"0",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_autoLoginPass3,					"0",	NULL,					CVAR_ARCHIVE )

//Logging
XCVAR_DEF( cg_logChat,							"1",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_logFormat,						"1",	NULL,					CVAR_ARCHIVE )

//BETA
XCVAR_DEF( cg_specHud,							"0",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_minimapScale,						"1",	NULL,					CVAR_NONE )
XCVAR_DEF( cg_duelMusic,						"1",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_drawHud,							"1",	NULL,					CVAR_ARCHIVE )

XCVAR_DEF( cg_strafeHelperPrecision,			"256",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_strafeHelperCutoff,				"0",	NULL,					CVAR_ARCHIVE )

XCVAR_DEF( cg_predictKnockback,					"0",	NULL,					0 )

XCVAR_DEF( cg_strafeHelperActiveColor,	"0 255 0 200", CG_StrafeHelperActiveColorChange,	CVAR_ARCHIVE )
XCVAR_DEF( cg_strafeHelperInactiveAlpha,		"200",	NULL,					CVAR_ARCHIVE )

XCVAR_DEF( cp_pluginDisable,					"1536",	NULL,					CVAR_ARCHIVE|CVAR_USERINFO ) //'enable' holstered saber (512) and ledge grab (1536) by default, to avoid missing JA+ animations
XCVAR_DEF( com_maxFPS,							"125",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_displayCameraPosition,		"1 80 16",	NULL,					CVAR_ROM|CVAR_USERINFO )
XCVAR_DEF( cg_displayNetSettings,			"125 0 125",NULL,					CVAR_ROM|CVAR_USERINFO )

//group timenudge, maxpackets, maxfps into cg_displaynetSettings ?
XCVAR_DEF( cl_timeNudge,						"0",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cl_maxPackets,						"125",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cjp_client,						"1.4JAPRO",	NULL,					CVAR_USERINFO|CVAR_ROM )
XCVAR_DEF( cp_clanPwd,							"none",	NULL,					CVAR_USERINFO )
XCVAR_DEF( cp_sbRGB1,							"0",	NULL,					CVAR_ARCHIVE | CVAR_USERINFO )
XCVAR_DEF( cp_sbRGB2,							"0",	NULL,					CVAR_ARCHIVE | CVAR_USERINFO )

//XCVAR_DEF( cg_predictRacemode,				"0",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_legstuck,							"0",	NULL,					CVAR_NONE )
XCVAR_DEF( cg_specCameraMode,					"1",	NULL,					CVAR_ARCHIVE ) //ethan wants this archive

XCVAR_DEF( cg_centerHeight,						"0",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_centerSize,						"1",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_fkDuration,						"50",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_fkFirstJumpDuration,				"0",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_fkSecondJumpDelay,				"0",	NULL,					CVAR_ARCHIVE )

XCVAR_DEF( cl_commandsize,						"64",	NULL,					CVAR_ARCHIVE )

XCVAR_DEF( cg_strafeTrailPlayers,				"0",	NULL,					0 )
XCVAR_DEF( cg_strafeTrailLife,					"5",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_strafeTrailRacersOnly,			"0",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_strafeTrailRadius,				"2",	NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_strafeTrailFPS,					"40",	NULL,					0 )
XCVAR_DEF( cg_strafeTrailGhost,					"0",	NULL,					CVAR_ARCHIVE )

XCVAR_DEF( cg_drainFX,							"1",	NULL,					CVAR_NONE )
//Make maxpackets userinfo maybe idk

#if 1
XCVAR_DEF( cg_logStrafeTrail,					"0",	NULL,					0 )
#endif

#define _DRAWTRIGGERS 0
#if _DRAWTRIGGERS
XCVAR_DEF( cg_drawTriggers,						"0",	NULL,					CVAR_NONE ) //CVAR_ARCHIVE
#endif

XCVAR_DEF( cg_cleanChatbox,						"0",	NULL,					CVAR_ARCHIVE )

#define _DEBUGTIMENUDGE 1
#ifdef _DEBUGTIMENUDGE
//XCVAR_DEF( cg_smoothTimenudge,				"0",							0 )
XCVAR_DEF( cl_timenudgeDuration,				"0",	NULL,					0 )
#endif

XCVAR_DEF( cg_drawTrajectory,					"0",	NULL,					0 )

XCVAR_DEF( bg_fighterAltControl,				"0",					NULL,					CVAR_SYSTEMINFO )
XCVAR_DEF( broadsword,							"0",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_animBlend,						"1",					NULL,					CVAR_NONE )
XCVAR_DEF( cg_animSpeed,						"1",					NULL,					CVAR_CHEAT )
XCVAR_DEF( cg_auraShell,						"1",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_autoSwitch,						"1",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_bobPitch,							"0",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_bobRoll,							"0",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_bobUp,							"0",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_cameraOrbit,						"0",					NULL,					CVAR_CHEAT )
XCVAR_DEF( cg_cameraOrbitDelay,					"50",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_centerTime,						"3",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_crosshairHealth,					"0",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_crosshairSize,					"24",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_crosshairSizeScale,				"1",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_crosshairX,						"0",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_crosshairY,						"0",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_currentSelectedPlayer,			"0",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_draw2D,							"1",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_draw3DIcons,						"1",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_drawAmmoWarning,					"0",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_drawCrosshair,					"1",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_drawCrosshairNames,				"1",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_drawEnemyInfo,					"1",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_drawFPS,							"0",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_drawFriend,						"1",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_drawGun,							"1",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_gunAlpha,							"1",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_drawIcons,						"1",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_drawRadar,						"1",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_drawRewards,						"1",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_drawSnapshot,						"0",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_drawStatus,						"1",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_drawTimer,						"0",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_drawTimerMsec,					"1",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_drawUpperRight,					"1",					NULL,					CVAR_NONE )
XCVAR_DEF( cg_drawVehLeadIndicator,				"1",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_dynamicCrosshair,					"1",					NULL,					CVAR_ARCHIVE )
//Enables ghoul2 traces for crosshair traces.. more precise when pointing at others, but slower.
//And if the server doesn't have g2 col enabled, it won't match up the same.
XCVAR_DEF( cg_dynamicCrosshairPrecision,		"1",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_debugAnim,						"0",					NULL,					CVAR_CHEAT )
XCVAR_DEF( cg_debugGun,							"0",					NULL,					CVAR_CHEAT )
XCVAR_DEF( cg_debugSaber,						"0",					NULL,					CVAR_CHEAT )
XCVAR_DEF( cg_debugPosition,					"0",					NULL,					CVAR_CHEAT )
XCVAR_DEF( cg_debugEvents,						"0",					NULL,					CVAR_CHEAT )
XCVAR_DEF( cg_dismember,						"0",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_deferPlayers,						"1",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_errorDecay,						"100",					NULL,					CVAR_NONE )
XCVAR_DEF( cg_footsteps,						"3",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_fov,								"90",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_fovAspectAdjust,					"1",					NULL,					CVAR_ARCHIVE ) //fixed skyportal issue
XCVAR_DEF( cg_fovViewmodel,						"80",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_fovViewmodelAdjust,				"1",					NULL,					CVAR_ARCHIVE ) // shifts viewmodels down above cg_fov 90
XCVAR_DEF( cg_headTurn,							"1",					NULL,					CVAR_NONE )
XCVAR_DEF( cg_fpls,								"0",					NULL,					CVAR_NONE )
XCVAR_DEF( cg_g2TraceLod,						"2",					NULL,					CVAR_NONE )
XCVAR_DEF( cg_ghoul2Marks,						"16",					NULL,					CVAR_NONE )
XCVAR_DEF( cg_gunX,								"0",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_gunY,								"0",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_gunZ,								"0",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_hudFiles,							"0",					CG_UpdateHUD,					CVAR_ARCHIVE )
XCVAR_DEF( cg_jumpSounds,						"0",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_instantDuck,						"0",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_lagometer,						"0",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_lagometerX,						"48",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_lagometerY,						"144",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_marks,							"1",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_noPlayerAnims,					"0",					NULL,					CVAR_CHEAT )
XCVAR_DEF( cg_noPredict,						"0",					NULL,					CVAR_NONE )
XCVAR_DEF( cg_noProjectileTrail,				"0",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_noTaunt,							"0",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_oldPainSounds,					"0",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_predictItems,						"1",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_renderToTextureFX,				"1",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_repeaterOrb,						"0",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_runPitch,							"0",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_runRoll,							"0",					NULL,					CVAR_ARCHIVE )
//allows us to trace between server frames on the client to see if we're visually
//hitting the last entity we detected a hit on from the server.
XCVAR_DEF( cg_saberClientVisualCompensation,	"1",					NULL,					CVAR_NONE )
XCVAR_DEF( cg_saberContact,						"1",					NULL,					CVAR_NONE )
XCVAR_DEF( cg_saberDynamicMarks,				"0",					NULL,					CVAR_NONE )
XCVAR_DEF( cg_saberDynamicMarkTime,				"60000",				NULL,					CVAR_NONE )
XCVAR_DEF( cg_saberModelTraceEffect,			"0",					NULL,					CVAR_NONE )
XCVAR_DEF( cg_saberTrail,						"1",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_saberClash,						"1",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_shaderSaberCore,					"0.8",					NULL,					CVAR_NONE )
XCVAR_DEF( cg_shaderSaberGlow,					"0.625",				NULL,					CVAR_NONE )//1.25?
XCVAR_DEF( cg_saberTeamColors,					"1",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_noRGBSabers,						"0",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( disco,								"0",					NULL,					CVAR_INTERNAL ) //xD
XCVAR_DEF( cg_scorePlums,						"0",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_stereoSeparation,					"0.4",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_shadows,							"1",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_simpleItems,						"0",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_showMiss,							"0",					NULL,					CVAR_NONE )
XCVAR_DEF( cg_showVehBounds,					"0",					NULL,					CVAR_NONE )
XCVAR_DEF( cg_showVehMiss,						"0",					NULL,					CVAR_NONE )
//XCVAR_DEF( cg_smoothClients,					"0",					NULL,					CVAR_ARCHIVE ) //breaks interp on non-JAPRO servers
XCVAR_DEF( cg_snapshotTimeout,					"10",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_speedTrail,						"1",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_stats,							"0",					NULL,					CVAR_NONE )
XCVAR_DEF( cg_teamChatsOnly,					"0",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_thirdPerson,						"0",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_thirdPersonAlpha,					"1.0",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_thirdPersonAngle,					"0",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_thirdPersonCameraDamp,			"0.3",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_thirdPersonHorzOffset,			"0",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_thirdPersonPitchOffset,			"0",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_thirdPersonRange,					"80",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_thirdPersonSpecialCam,			"0",					NULL,					CVAR_NONE )
XCVAR_DEF( cg_thirdPersonTargetDamp,			"0.5",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_thirdPersonVertOffset,			"16",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_thirdPersonCrosshairCenter,		"0",					NULL,					CVAR_NONE ) //1 is old third person static crosshair behavior
XCVAR_DEF( cg_timescaleFadeEnd,					"1",					NULL,					CVAR_NONE )
XCVAR_DEF( cg_timescaleFadeSpeed,				"0",					NULL,					CVAR_NONE )
XCVAR_DEF( cg_viewsize,							"100",					NULL,					CVAR_INTERNAL ) //inherited bloat
XCVAR_DEF( cl_paused,							"0",					NULL,					CVAR_ROM )
XCVAR_DEF( com_buildScript,						"0",					NULL,					CVAR_NONE )
XCVAR_DEF( com_cameraMode,						"0",					NULL,					CVAR_CHEAT )
XCVAR_DEF( com_optvehtrace,						"0",					NULL,					CVAR_NONE )
XCVAR_DEF( debugBB,								"0",					NULL,					CVAR_NONE )
XCVAR_DEF( forcepowers,							DEFAULT_FORCEPOWERS,	NULL,					CVAR_USERINFO|CVAR_ARCHIVE )
XCVAR_DEF( g_synchronousClients,				"0",					NULL,					CVAR_SYSTEMINFO )
XCVAR_DEF( model,								DEFAULT_MODEL,			NULL,					CVAR_USERINFO|CVAR_ARCHIVE )
XCVAR_DEF( pmove_fixed,							"0",					NULL,					CVAR_SYSTEMINFO )
XCVAR_DEF( pmove_float,							"0",					NULL,					CVAR_SYSTEMINFO )
XCVAR_DEF( pmove_msec,							"8",					NULL,					CVAR_SYSTEMINFO )
XCVAR_DEF( r_autoMap,							"0",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( r_autoMapX,							"496",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( r_autoMapY,							"32",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( r_autoMapW,							"128",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( r_autoMapH,							"128",					NULL,					CVAR_ARCHIVE )
XCVAR_DEF( sv_running,							"0",					CG_SVRunningChange,		CVAR_ROM )
XCVAR_DEF( teamoverlay,							"0",					NULL,					CVAR_ROM|CVAR_USERINFO )
XCVAR_DEF( timescale,							"1",					NULL,					CVAR_NONE )
XCVAR_DEF( ui_about_gametype,					"0",					NULL,					CVAR_ROM|CVAR_INTERNAL )
XCVAR_DEF( ui_about_fraglimit,					"0",					NULL,					CVAR_ROM|CVAR_INTERNAL )
XCVAR_DEF( ui_about_capturelimit,				"0",					NULL,					CVAR_ROM|CVAR_INTERNAL )
XCVAR_DEF( ui_about_duellimit,					"0",					NULL,					CVAR_ROM|CVAR_INTERNAL )
XCVAR_DEF( ui_about_timelimit,					"0",					NULL,					CVAR_ROM|CVAR_INTERNAL )
XCVAR_DEF( ui_about_maxclients,					"0",					NULL,					CVAR_ROM|CVAR_INTERNAL )
XCVAR_DEF( ui_about_dmflags,					"0",					NULL,					CVAR_ROM|CVAR_INTERNAL )
XCVAR_DEF( ui_about_mapname,					"0",					NULL,					CVAR_ROM|CVAR_INTERNAL )
XCVAR_DEF( ui_about_hostname,					"0",					NULL,					CVAR_ROM|CVAR_INTERNAL )
XCVAR_DEF( ui_about_needpass,					"0",					NULL,					CVAR_ROM|CVAR_INTERNAL )
XCVAR_DEF( ui_about_botminplayers,				"0",					NULL,					CVAR_ROM|CVAR_INTERNAL )
XCVAR_DEF( ui_myteam,							"0",					NULL,					CVAR_ROM|CVAR_INTERNAL )
XCVAR_DEF( ui_tm1_c0_cnt,						"0",					NULL,					CVAR_ROM|CVAR_INTERNAL )
XCVAR_DEF( ui_tm1_c1_cnt,						"0",					NULL,					CVAR_ROM|CVAR_INTERNAL )
XCVAR_DEF( ui_tm1_c2_cnt,						"0",					NULL,					CVAR_ROM|CVAR_INTERNAL )
XCVAR_DEF( ui_tm1_c3_cnt,						"0",					NULL,					CVAR_ROM|CVAR_INTERNAL )
XCVAR_DEF( ui_tm1_c4_cnt,						"0",					NULL,					CVAR_ROM|CVAR_INTERNAL )
XCVAR_DEF( ui_tm1_c5_cnt,						"0",					NULL,					CVAR_ROM|CVAR_INTERNAL )
XCVAR_DEF( ui_tm1_cnt,							"0",					NULL,					CVAR_ROM|CVAR_INTERNAL )
XCVAR_DEF( ui_tm2_c0_cnt,						"0",					NULL,					CVAR_ROM|CVAR_INTERNAL )
XCVAR_DEF( ui_tm2_c1_cnt,						"0",					NULL,					CVAR_ROM|CVAR_INTERNAL )
XCVAR_DEF( ui_tm2_c2_cnt,						"0",					NULL,					CVAR_ROM|CVAR_INTERNAL )
XCVAR_DEF( ui_tm2_c3_cnt,						"0",					NULL,					CVAR_ROM|CVAR_INTERNAL )
XCVAR_DEF( ui_tm2_c4_cnt,						"0",					NULL,					CVAR_ROM|CVAR_INTERNAL )
XCVAR_DEF( ui_tm2_c5_cnt,						"0",					NULL,					CVAR_ROM|CVAR_INTERNAL )
XCVAR_DEF( ui_tm2_cnt,							"0",					NULL,					CVAR_ROM|CVAR_INTERNAL )
XCVAR_DEF( ui_tm3_cnt,							"0",					NULL,					CVAR_ROM|CVAR_INTERNAL )


#undef XCVAR_DEF
