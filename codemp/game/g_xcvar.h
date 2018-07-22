
#ifdef XCVAR_PROTO
	#define XCVAR_DEF( name, defVal, update, flags, announce ) extern vmCvar_t name;
#endif

#ifdef XCVAR_DECL
	#define XCVAR_DEF( name, defVal, update, flags, announce ) vmCvar_t name;
#endif

#ifdef XCVAR_LIST
	#define XCVAR_DEF( name, defVal, update, flags, announce ) { & name , #name , defVal , update , flags , announce },
#endif

XCVAR_DEF( bg_fighterAltControl,		"0",			NULL,				CVAR_SYSTEMINFO,								qtrue )
XCVAR_DEF( capturelimit,				"8",			NULL,				CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_NORESTART,	qtrue )
XCVAR_DEF( com_optvehtrace,				"0",			NULL,				CVAR_NONE,										qtrue )
XCVAR_DEF( d_altRoutes,					"0",			NULL,				CVAR_CHEAT,										qfalse )
XCVAR_DEF( d_asynchronousGroupAI,		"0",			NULL,				CVAR_CHEAT,										qfalse )
XCVAR_DEF( d_break,						"0",			NULL,				CVAR_CHEAT,										qfalse )
XCVAR_DEF( d_JediAI,					"0",			NULL,				CVAR_CHEAT,										qfalse )
XCVAR_DEF( d_noGroupAI,					"0",			NULL,				CVAR_CHEAT,										qfalse )
XCVAR_DEF( d_noroam,					"0",			NULL,				CVAR_CHEAT,										qfalse )
XCVAR_DEF( d_npcai,						"0",			NULL,				CVAR_CHEAT,										qfalse )
XCVAR_DEF( d_npcaiming,					"0",			NULL,				CVAR_CHEAT,										qfalse )
XCVAR_DEF( d_npcfreeze,					"0",			NULL,				CVAR_CHEAT,										qfalse )
XCVAR_DEF( d_noIntermissionWait,		"0",			NULL,				CVAR_NONE,										qfalse )
XCVAR_DEF( d_patched,					"0",			NULL,				CVAR_CHEAT,										qfalse )
XCVAR_DEF( d_perPlayerGhoul2,			"0",			NULL,				CVAR_CHEAT,										qtrue )
XCVAR_DEF( d_powerDuelPrint,			"0",			NULL,				CVAR_NONE,										qtrue )
XCVAR_DEF( d_projectileGhoul2Collision,	"1",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( d_saberAlwaysBoxTrace,		"0",			NULL,				CVAR_CHEAT,										qtrue )
XCVAR_DEF( d_saberBoxTraceSize,			"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( d_saberCombat,				"0",			NULL,				CVAR_CHEAT,										qfalse )
XCVAR_DEF( d_saberGhoul2Collision,		"1",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( d_saberInterpolate,			"0",			NULL,				CVAR_CHEAT,										qtrue )
XCVAR_DEF( d_saberKickTweak,			"1",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( d_saberSPStyleDamage,		"1",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( d_saberStanceDebug,			"0",			NULL,				CVAR_NONE,										qfalse )
XCVAR_DEF( d_siegeSeekerNPC,			"0",			NULL,				CVAR_CHEAT,										qtrue )
XCVAR_DEF( dedicated,					"0",			NULL,				CVAR_NONE,										qfalse )
XCVAR_DEF( developer,					"0",			NULL,				CVAR_NONE,										qfalse )
XCVAR_DEF( dmflags,						"0",			NULL,				CVAR_SERVERINFO|CVAR_ARCHIVE,					qtrue )
XCVAR_DEF( duel_fraglimit,				"10",			NULL,				CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_NORESTART,	qtrue )
XCVAR_DEF( fraglimit,					"20",			NULL,				CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_NORESTART,	qtrue )
XCVAR_DEF( g_adaptRespawn,				"1",			NULL,				CVAR_NONE,										qtrue )
XCVAR_DEF( g_allowDuelSuicide,			"1",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_allowHighPingDuelist,		"1",			NULL,				CVAR_NONE,										qtrue )
XCVAR_DEF( g_allowNPC,					"1",			NULL,				CVAR_CHEAT,										qtrue )
XCVAR_DEF( g_allowTeamVote,				"1",			NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( g_allowVote,					"-1",			NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( g_antiFakePlayer,			"1",			NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( g_armBreakage,				"0",			NULL,				CVAR_NONE,										qtrue )
XCVAR_DEF( g_austrian,					"0",			NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( g_autoMapCycle,				"0",			NULL,				CVAR_ARCHIVE|CVAR_NORESTART,					qtrue )
XCVAR_DEF( g_banIPs,					"",				NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( g_charRestrictRGB,			"1",			NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( g_duelWeaponDisable,			"1",			NULL,				CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_LATCH,		qtrue )
XCVAR_DEF( g_debugAlloc,				"0",			NULL,				CVAR_NONE,										qfalse )
#ifndef FINAL_BUILD
XCVAR_DEF( g_debugDamage,				"0",			NULL,				CVAR_NONE,										qfalse )
#endif
XCVAR_DEF( g_debugMelee,				"0",			NULL,				CVAR_SERVERINFO,								qtrue )
XCVAR_DEF( g_debugMove,					"0",			NULL,				CVAR_NONE,										qfalse )
XCVAR_DEF( g_debugSaberLocks,			"0",			NULL,				CVAR_CHEAT,										qfalse )
XCVAR_DEF( g_debugServerSkel,			"0",			NULL,				CVAR_CHEAT,										qfalse )
#ifdef _DEBUG
XCVAR_DEF( g_disableServerG2,			"0",			NULL,				CVAR_NONE,										qtrue )
#endif
XCVAR_DEF( g_dismember,					"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_doWarmup,					"0",			NULL,				CVAR_NONE,										qtrue )
//XCVAR_DEF( g_engineModifications,		"1",			NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( g_ff_objectives,				"0",			NULL,				CVAR_CHEAT|CVAR_NORESTART,						qtrue )
XCVAR_DEF( g_filterBan,					"1",			NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( g_forceBasedTeams,			"0",			NULL,				CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_LATCH,		qfalse )
XCVAR_DEF( g_forceClientUpdateRate,		"250",			NULL,				CVAR_NONE,										qfalse )
XCVAR_DEF( g_forceDodge,				"1",			NULL,				CVAR_NONE,										qtrue )
XCVAR_DEF( g_forcePowerDisable,			"0",			CVU_ForceDisable,	CVAR_SERVERINFO|CVAR_ARCHIVE/*|CVAR_LATCH*/,		qtrue )
XCVAR_DEF( g_forcePowerDisableFFA,		"0",			NULL,				CVAR_ARCHIVE/*|CVAR_LATCH*/,					qtrue )
XCVAR_DEF( g_forceRegenTime,			"200",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_forceRespawn,				"60",			NULL,				CVAR_NONE,										qtrue )
XCVAR_DEF( g_fraglimitVoteCorrection,	"1",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_friendlyFire,				"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_friendlySaber,				"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_g2TraceLod,				"3",			NULL,				CVAR_NONE,										qtrue )
XCVAR_DEF( g_gametype,					"0",			NULL,				CVAR_SERVERINFO|CVAR_LATCH,						qfalse )
XCVAR_DEF( g_gravity,					"800",			NULL,				CVAR_NONE,										qtrue )
XCVAR_DEF( g_inactivity,				"0",			NULL,				CVAR_NONE,										qtrue )
XCVAR_DEF( g_jediVmerc,					"0",			NULL,				CVAR_SERVERINFO|CVAR_LATCH|CVAR_ARCHIVE,		qtrue )
XCVAR_DEF( g_knockback,					"1000",			NULL,				CVAR_NONE,										qtrue )
XCVAR_DEF( g_locationBasedDamage,		"1",			NULL,				CVAR_NONE,										qtrue )
XCVAR_DEF( g_log,						"games.log",	NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( g_logClientInfo,				"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_logSync,					"0",			NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( g_maxConnPerIP,				"3",			NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( g_maxForceRank,				"7",			CVU_ForceDisable,	CVAR_SERVERINFO|CVAR_ARCHIVE/*|CVAR_LATCH*/,	qtrue )
XCVAR_DEF( g_maxGameClients,			"0",			NULL,				CVAR_SERVERINFO|CVAR_LATCH|CVAR_ARCHIVE,		qfalse )
XCVAR_DEF( g_maxHolocronCarry,			"3",			NULL,				CVAR_LATCH,										qfalse )
XCVAR_DEF( g_motd,						"",				NULL,				CVAR_NONE,										qfalse )
XCVAR_DEF( g_needpass,					"0",			NULL,				CVAR_SERVERINFO|CVAR_ROM,						qfalse )
XCVAR_DEF( g_noSpecMove,				"0",			NULL,				CVAR_SERVERINFO,								qtrue )
XCVAR_DEF( g_npcspskill,				"0",			NULL,				CVAR_ARCHIVE|CVAR_INTERNAL,						qfalse )
XCVAR_DEF( g_password,					"",				NULL,				CVAR_NONE,										qfalse )
XCVAR_DEF( g_powerDuelEndHealth,		"90",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_powerDuelStartHealth,		"150",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_privateDuel,				"1",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_randFix,					"1",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_restarted,					"0",			NULL,				CVAR_ROM,										qfalse )
XCVAR_DEF( g_saberBladeFaces,			"1",			NULL,				CVAR_NONE,										qtrue )
XCVAR_DEF( g_saberDamageScale,			"1",			NULL,				CVAR_ARCHIVE,									qtrue )
#ifdef DEBUG_SABER_BOX
XCVAR_DEF( g_saberDebugBox,				"0",			NULL,				CVAR_CHEAT,										qfalse )
#endif
#ifndef FINAL_BUILD
XCVAR_DEF( g_saberDebugPrint,			"0",			NULL,				CVAR_CHEAT,										qfalse )
#endif
XCVAR_DEF( g_saberDmgDelay_Idle,		"350",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_saberDmgDelay_Wound,		"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_saberDmgVelocityScale,		"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_saberLockFactor,			"2",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_saberLocking,				"1",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_saberLockRandomNess,		"2",			NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( g_saberRealisticCombat,		"0",			NULL,				CVAR_CHEAT,										qfalse )
XCVAR_DEF( g_saberRestrictForce,		"0",			NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( g_saberTraceSaberFirst,		"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_saberWallDamageScale,		"0.4",			NULL,				CVAR_NONE,										qfalse )
XCVAR_DEF( g_securityLog,				"1",			NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( g_showDuelHealths,			"0",			NULL,				CVAR_SERVERINFO,								qfalse )
XCVAR_DEF( g_siegeRespawn,				"20",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_siegeTeam1,				"none",			NULL,				CVAR_ARCHIVE|CVAR_SERVERINFO,					qfalse )
XCVAR_DEF( g_siegeTeam2,				"none",			NULL,				CVAR_ARCHIVE|CVAR_SERVERINFO,					qfalse )
XCVAR_DEF( g_siegeTeamSwitch,			"1",			NULL,				CVAR_SERVERINFO|CVAR_ARCHIVE,					qfalse )
XCVAR_DEF( g_slowmoDuelEnd,				"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_smoothClients,				"1",			NULL,				CVAR_NONE,										qfalse )
XCVAR_DEF( g_spawnInvulnerability,		"3000",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_speed,						"250",			NULL,				CVAR_NONE,										qtrue )
XCVAR_DEF( g_statLog,					"0",			NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( g_statLogFile,				"statlog.log",	NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( g_stepSlideFix,				"1",			NULL,				CVAR_SERVERINFO,								qtrue )
XCVAR_DEF( g_synchronousClients,		"0",			NULL,				CVAR_SYSTEMINFO,								qfalse )
XCVAR_DEF( g_teamAutoJoin,				"0",			NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( g_teamForceBalance,			"0",			NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( g_timeouttospec,				"70",			NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( g_userinfoValidate,			"0",		NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( g_useWhileThrowing,			"1",			NULL,				CVAR_NONE,										qtrue )
XCVAR_DEF( g_voteDelay,					"3000",			NULL,				CVAR_NONE,										qfalse )
XCVAR_DEF( g_warmup,					"20",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_weaponDisable,				"0",			CVU_WeaponDisable,	CVAR_SERVERINFO|CVAR_ARCHIVE/*|CVAR_LATCH*/,		qtrue )
XCVAR_DEF( g_weaponRespawn,				"5",			NULL,				CVAR_NONE,										qtrue )
XCVAR_DEF( gamedate,					__DATE__,		NULL,				CVAR_ROM,										qfalse )
XCVAR_DEF( gamename,					GAMEVERSION,	NULL,				CVAR_SERVERINFO|CVAR_ROM,						qfalse )
XCVAR_DEF( pmove_fixed,					"0",			NULL,				CVAR_SYSTEMINFO|CVAR_ARCHIVE,					qtrue )
XCVAR_DEF( pmove_float,					"0",			NULL,				CVAR_SYSTEMINFO|CVAR_ARCHIVE,					qtrue )
XCVAR_DEF( pmove_msec,					"8",			NULL,				CVAR_SYSTEMINFO|CVAR_ARCHIVE,					qtrue )
XCVAR_DEF( RMG,							"0",			NULL,				CVAR_NONE,										qtrue )
XCVAR_DEF( sv_cheats,					"1",			NULL,				CVAR_NONE,										qfalse )
XCVAR_DEF( sv_fps,						"40",			NULL,				CVAR_ARCHIVE|CVAR_SERVERINFO,					qtrue )
XCVAR_DEF( sv_maxRate,					"7000",			NULL,				CVAR_ARCHIVE|CVAR_SERVERINFO,					qtrue )
XCVAR_DEF( sv_maxclients,				"8",			NULL,				CVAR_SERVERINFO|CVAR_LATCH|CVAR_ARCHIVE,		qfalse )
XCVAR_DEF( timelimit,					"0",			NULL,				CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_NORESTART,	qtrue )

//JAPRO CTF
XCVAR_DEF( g_flagDrag,					"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_fixFlagSuicide,			"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_allowFlagThrow,			"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_fixCTFScores,				"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_fixFlagHitbox,				"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_rabbit,					"0",			CVU_Rabbit,	CVAR_ARCHIVE,									qtrue )

//JAPRO Saber
XCVAR_DEF( g_tweakSaber,				"0",			CVU_TweakSaber,		CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_backslashDamageScale,		"1",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_maxSaberDefense,			"0",			NULL,				CVAR_ARCHIVE|CVAR_LATCH,						qtrue )
XCVAR_DEF( g_saberTouchDmg,				"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_fixGroundStab,				"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_saberDuelSPDamage,			"1",			NULL,				CVAR_ARCHIVE,									qtrue ) //s ?
XCVAR_DEF( g_forceDuelSPDamage,			"0",			NULL,				CVAR_ARCHIVE,									qtrue ) //s ?
XCVAR_DEF( g_saberDisable,				"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_blueDamageScale,			"1",			NULL,				CVAR_ARCHIVE,									qtrue ) //sad hack
XCVAR_DEF( g_yellowDamageScale,			"1",			NULL,				CVAR_ARCHIVE,									qtrue ) //sad hack
XCVAR_DEF( g_redDamageScale,			"1",			NULL,				CVAR_ARCHIVE,									qtrue ) //sad hack
XCVAR_DEF( g_redDFADamageScale,			"1",			NULL,				CVAR_ARCHIVE,									qtrue ) 
XCVAR_DEF( g_saberDmgDelay_Hit,			"0",			NULL,				CVAR_ARCHIVE,									qtrue )

//JAPRO FORCE
XCVAR_DEF( g_tweakForce,				"0",			CVU_TweakForce,		CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_fixSaberInGrip,			"0",			NULL,				CVAR_ARCHIVE,									qtrue ) 
XCVAR_DEF( g_fixLightning,				"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_fixGetups,					"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_teamAbsorbScale,			"1",			NULL,				CVAR_ARCHIVE,									qtrue ) 
XCVAR_DEF( g_teamHealScale,				"1",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_teamEnergizeScale,			"1",			NULL,				CVAR_ARCHIVE,									qtrue )

//JAPRO GUNS
XCVAR_DEF( g_tweakWeapons,				"0",			CVU_TweakWeapons,	CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_startingWeapons,			"8",			CVU_StartingWeapons,	CVAR_ARCHIVE,									qtrue ) //Start with saber only default, fall back to melee if no saberattack
XCVAR_DEF( g_weaponDamageScale,			"1",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_projectileVelocityScale,	"1",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_selfDamageScale,			"0.5",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_projectileInheritance,		"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_fullInheritance,			"0",			NULL,				CVAR_ARCHIVE,									qtrue )

//JAPRO MOVEMENT
XCVAR_DEF( g_flipKick,					"0",			CVU_Flipkick,		CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_nonRandomKnockdown,		"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_fixRoll,					"0",			CVU_Roll,			CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_onlyBhop,					"0",			CVU_Bhop,			CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_movementStyle,				"1",			NULL,				CVAR_ARCHIVE,									qtrue )

//JAPRO Movement to be replaced with tweakmovement?
XCVAR_DEF( g_fixHighFPSAbuse,			"0",			CVU_HighFPS,		CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_LegDangle,					"1",			CVU_LegDangle,		CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_tweakJetpack,				"0",			CVU_TweakJetpack,	CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_slideOnPlayer,				"0",			CVU_Headslide,		CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_fixSlidePhysics,			"0",			NULL,				CVAR_ARCHIVE,									qtrue )

XCVAR_DEF( g_allowGrapple,				"0",			CVU_Grapple,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_hookSpeed,					"2400",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_hookStrength,				"800",			NULL,				CVAR_ARCHIVE|CVAR_SERVERINFO,					qtrue )
XCVAR_DEF( g_hookStrength1,				"20",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_hookStrength2,				"40",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_hookInheritance,			"0.5",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_hookFloodProtect,			"600",			NULL,				CVAR_ARCHIVE,									qtrue )

//JAPRO DUELING
XCVAR_DEF( g_duelStartHealth,			"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_duelStartArmor,			"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_duelDistanceLimit,			"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_allowUseInDuel,			"1",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_allowGunDuel,				"1",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_saberDuelForceRegenTime,	"200",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_forceDuelForceRegenTime,	"200",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_duelRespawn,				"0",			NULL,				CVAR_ARCHIVE,									qtrue )

//JAPRO ADMIN
XCVAR_DEF( g_juniorAdminLevel,			"0",			NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( g_fullAdminLevel,			"0",			NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( g_juniorAdminPass,			"",				NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( g_fullAdminPass,				"",				NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( g_juniorAdminMsg,			"",				NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( g_fullAdminMsg,				"",				NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( g_allowNoFollow,				"0",			NULL,				CVAR_ARCHIVE,									qtrue ) //race also

//JAPRO OTHER Gameplay
XCVAR_DEF( g_flipKickDamageScale,		"1",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_glitchKickDamage,			"-1",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_maxFallDmg,				"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_startingItems,				"0",			CVU_StartingItems,	CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_screenShake,				"0",			CVU_ScreenShake,	CVAR_ARCHIVE,									qtrue ) //should be g_forceScreenShake
XCVAR_DEF( g_unlagged,					"0",			CVU_Unlagged,		CVAR_ARCHIVE|CVAR_LATCH,						qtrue )
XCVAR_DEF( g_allowSaberSwitch,			"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_allowTeamSuicide,			"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_godChat,					"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_showHealth,				"0",			NULL,				CVAR_ARCHIVE|CVAR_LATCH,						qtrue )
XCVAR_DEF( g_damageNumbers,				"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_fixKillCredit,				"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_stopHealthESP,				"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_blockDuelHealthSpec,		"0",			NULL,				CVAR_ARCHIVE,									qtrue )

#define _ANTIWALLHACK 1
#if _ANTIWALLHACK
XCVAR_DEF( g_antiWallhack,				"0",			NULL,				CVAR_ARCHIVE,									qtrue )
#endif


//JAPRO Other
XCVAR_DEF( jcinfo,						"0",			NULL,				CVAR_SERVERINFO|CVAR_ROM,						qtrue ) //not a cvar, dont change it
XCVAR_DEF( g_emotesDisable,				"0",			CVU_Jawarun,		CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_corpseRemovalTime,			"30",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_removeSpectatorPortals,	"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_consoleMOTD,				"",				NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( g_centerMOTDTime,			"5",			NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( g_centerMOTD,				"",				NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( g_fakeClients,				"0",			NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( g_lagIcon,					"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_allowSamePlayerNames,		"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( sv_maxTeamSize,				"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_tweakVote,					"0",			NULL,				CVAR_ARCHIVE|CVAR_LATCH,						qtrue )//latch cuz of calculateRanks? not sure man
XCVAR_DEF( g_allowSpotting,				"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_allowTargetLaser,			"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_voteTimeout,				"180",			NULL,				CVAR_ARCHIVE,									qfalse )//Time in seconds to lockout callvote after a failed vote
XCVAR_DEF( g_allowVGS,					"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_pauseTime,					"120",			NULL,				CVAR_NONE,										qfalse )
XCVAR_DEF( g_unpauseTime,				"5",			NULL,				CVAR_NONE,										qfalse )
XCVAR_DEF( restricts,					"0",			NULL,				CVAR_ARCHIVE|CVAR_SERVERINFO,					qfalse )
XCVAR_DEF( g_mercyRule,					"0",			NULL,				CVAR_ARCHIVE,									qtrue ) //If the difference is greater than X percent of the frag limit... then end match.

//JAPRO RACE / ACCOUNTS
XCVAR_DEF( g_raceMode,					"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_allowRaceTele,				"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_allowRegistration,			"1",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( sv_pluginKey,				"0",			NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( g_forceLogin,				"0",			NULL,				CVAR_ARCHIVE,									qfalse )
//XCVAR_DEF( sv_globalDBPath,			"",				NULL,				CVAR_ARCHIVE|CVAR_LATCH,						qfalse )
//XCVAR_DEF( sv_webServerPath,			"",				NULL,				CVAR_ARCHIVE|CVAR_LATCH,						qfalse )
//XCVAR_DEF( sv_webServerPassword,		"",				NULL,				CVAR_ARCHIVE,									qfalse )

//JAPRO LOGGING/RECORDING
XCVAR_DEF( g_duelLog,					"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_raceLog,					"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_playerLog,					"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( sv_autoRaceDemo,				"0",			NULL,				CVAR_ARCHIVE,									qtrue )

//JAPRO BOTS
XCVAR_DEF( bot_nochat,					"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( bot_strafeOffset,			"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_newBotAI,					"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_newBotAITarget,			"-1",			NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( bot_maxbots,					"0",			NULL,				CVAR_ARCHIVE,									qfalse )

//testing

XCVAR_DEF( bot_s1,						"16",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( bot_s2,						"24",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( bot_s3,						"48",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( bot_s4,						"0.5",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( bot_s5,						"0",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( bot_s6,						"64",			NULL,				CVAR_ARCHIVE,									qtrue )


//DEBUG / TOOLS
XCVAR_DEF( g_showJumpSpot,				"0",			NULL,				CVAR_ARCHIVE,									qtrue )

#define _ELORANKING 1
#if _ELORANKING
XCVAR_DEF( g_eloRanking,					"0",			NULL,				CVAR_ARCHIVE|CVAR_LATCH,						qtrue )
XCVAR_DEF( g_eloNewUserCutoff,				"-1",			NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( g_eloProvisionalCutoff,			"10",			NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( g_eloProvisionalChangeBig,		"2",			NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( g_eloProvisionalChangeSmall,		"1.5",			NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( g_eloMinimumDuels,				"20",			NULL,				CVAR_ARCHIVE,									qfalse )
XCVAR_DEF( g_eloKValue1,						"25",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_eloKValue2,						"25",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_eloKValue3,						"25",			NULL,				CVAR_ARCHIVE,									qtrue )
#endif

#define _NEWRACERANKING 1
#define _STATLOG 0
#define _TESTBSP 0

//XCVAR_DEF( cl_yawspeed,				"0",			NULL,				CVAR_SYSTEMINFO,								qfalse )
//XCVAR_DEF( cl_allowDownload,			"0",			NULL,				CVAR_SYSTEMINFO,								qfalse )
//XCVAR_DEF( r_primitives,				"0",			NULL,				CVAR_SYSTEMINFO,								qtrue ) //not needed anymore, RIP jaiko
//XCVAR_DEF( g_fixFlipKick,				"0",			NULL,				CVAR_ARCHIVE,									qtrue )

//drain fuck around cvars
#define _draintest 0
#if _draintest
XCVAR_DEF( g_forceDrainDamage,				"4",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_forceDrainTargetRegenDelay,	"800",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_forceDrainSelfRegenDelay,		"500",			NULL,				CVAR_ARCHIVE,									qtrue )
XCVAR_DEF( g_forceDrainRestartDelay,		"1500",			NULL,				CVAR_ARCHIVE,									qtrue )
#endif

#define _retardedsabertest 0 
#if _retardedsabertest
XCVAR_DEF( sv_saberFPS,					"0",			NULL,				CVAR_ARCHIVE,									qtrue )
#endif

#undef XCVAR_DEF
