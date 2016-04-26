## Server Cvars


#### CTF
	g_flagDrag			0  
	g_fixFlagSuicide	0  
	g_allowFlagThrow	0  
	g_fixCTFScores		0  
	g_fixFlagHitbox		0  	
	g_rabbit			0  

#### Saber
	g_tweakSaber			0	//Configured with /tweakSaber command
	g_backslashDamageScale	1	
	g_maxSaberDefense		0	
	g_saberTouchDmg			0	//Configure saber touch damage for MP dmgs. Can be >1 for more touch damage.
	g_fixGroundStab			0	
	g_saberDuelSPDamage		1	
	g_forceDuelSPDamage		0	
	g_saberDisable			0	
	g_blueDamageScale		1	
	g_yellowDamageScale		1	
	g_redDamageScale		1	
	g_redDFADamageScale		1	

#### Force 
	g_tweakForce		0	//Configured with /tweakForce command
	g_fixSaberInGrip	0	
	g_fixLightning		0	
	g_fixGetups			0	
	g_teamAbsorbScale	1	
	g_teamHealScale		1	
	g_teamEnergizeScale	1	

#### Guns 
	g_tweakWeapons				0	//Configured with /tweakWeapons command	
	g_startingWeapons			8	//Configured with /startingWeapons command	
	g_weaponDamageScale			1	
	g_projectileVelocityScale	1	
	g_selfDamageScale			0.5	
	g_projectileInheritance		0	
	g_fullInheritance			0	

#### Movement 
	g_slideOnPlayer			0	
	g_flipKick				0	
	g_nonRandomKnockdown	0	
	g_fixRoll				0	
	g_onlyBhop				0	
	g_tweakJetpack			0	
	g_movementStyle			1	
	g_LegDangle				1	
	g_fixHighFPSAbuse		0	
	g_fixSlidePhysics		0	
	g_fixRedDFA				0	
	g_fixGlitchKickDamage	0

#### Dueling 
	g_duelStartHealth			0	
	g_duelStartArmor			0	
	g_duelDistanceLimit			0	
	g_allowUseInDuel			1	
	g_allowGunDuel				1	
	g_saberDuelForceRegenTime	200  
	g_forceDuelForceRegenTime	200	
	g_duelRespawn				0	

#### Admin 
	g_juniorAdminLevel	0	
	g_fullAdminLevel	0	
	g_juniorAdminPass	""
	g_fullAdminPass		""
	g_juniorAdminMsg	""
	g_fullAdminMsg		""
	g_allowNoFollow		0	

#### Other Gameplay 
	g_flipKickDamageScale	1	
	g_maxFallDmg			0	
	g_startingItems			0	//Configured with /startingItems command.
	g_quakeStyleTeleport	0	
	g_screenShake			0	
	g_unlagged				0	
	g_allowSaberSwitch		0	
	g_allowTeamSuicide		0	
	g_emotesDisable			0	
	g_godChat				0	
	g_showHealth			0	
	g_damageNumbers			0	
	g_fixKillCredit			0	
	g_stopHealthESP			0	
	g_blockDuelHealthSpec	0	
	g_antiWallhack			0	

#### Other 
	g_corpseRemovalTime			30	
	g_removeSpectatorPortals	0	
	g_consoleMOTD				""
	g_centerMOTDTime			5	
	g_centerMOTD				""
	g_fakeClients				0	
	g_lagIcon					0	
	g_allowSamePlayerNames		0	
	sv_maxTeamSize				0	
	g_tweakVote					0	//Configured with /tweakVote command.
	g_allowSpotting				0	
	g_allowTargetLaser			0	
	g_voteTimeout				180	//Time in seconds to lockout callvote aftera failed vote.
	g_allowVGS					0	
	g_pauseTime					120	
	g_unpauseTime				5	
	restricts					0	
	g_mercyRule					0	//If the score difference isgreater than X percent of the fraglimit, end the match.

#### Race/Accounts 
	g_raceMode					0	//0=Noracemode, 1=forcedracemode, 2=player can toggle race mode with /racecommand.
	g_allowRaceTele				0	
	g_allowRegistration			1	
    sv_pluginKey				0	
	g_forceLogin				0	


#### Loggging/Recording 
	g_duelLog		0	
	g_raceLog		0	
	g_playerLog		0	
	sv_autoRaceDemo	0 //Requires custom server executable with "svrecord" command.

#### Bots 
	bot_nochat			0	
	bot_strafeOffset	0	
	g_newBotAI			0	
	g_newBotAITarget	-1	//-2=Target closest excluding otherbots. -1=target closest. 0-31=target clientnum.
	bot_maxbots			0	

#### Elo ranking
	g_eloRanking					0 // Enables the built in elo ranking for duels.
	g_eloNewUserCutoff				-1 
	g_eloProvisionalCutoff			10 
	g_eloProvisionalChangeBig		2 
	g_eloProvisionalChangeSmall		1.5 
	g_eloMinimumDuels				20 
	g_eloKValue1					25 
	g_eloKValue2					25 
	g_eloKValue3					25 

#### Debug/Tools 
	g_showJumpSpot	0	//Marks where player touches ground as they land. Useful with the /nudge command on entities.

## Server RCON Commands 
	accountInfo  	
	amban		
	amgrantadmin	
	amkick			
	changepassword		
	clearIP		
	DBInfo		
	deleteAccount		
	forceteam		
	gametype	//Instantly changethe gametype of the server without having to reload the map
	pause	//Pause/unpause the game		
	rebuildElo	//Delete all Elo records and rebuild them from the duel records in the database.		
	register  
	renameAccount	
	resetScores	//Reset all player/team scores without having to reload the map.	
	startingItems	
	startingWeapons	
	toggleAdmin	
	toggleVote	
	toggleuserinfovalidation	
	tweakForce	
	tweakSaber	
	tweakVote	
	tweakWeapons	

#### Saber tweaks 
	Skip saber interpolate for MPdmgs	//1
	JK2 1.02 Style Damage System	//2
	Reduced saberblock for MP damages	//3
	Reducesaberdrops for MP damages	//4
	Allow roll cancel for saber swings	//5
	Removec hainable swings from red stance	//6
	Fixed saber switch	//7
	No aim backslash	//8
	JK2 red DFA	//9
	Fix yellow DFA	//10
	Spin red DFA	//11
	Spin back slash	//12
	JK2 Lunge	//13

#### Force Tweaks 
	No force power drain for crouch attack	//1
	Fix projectile force push dir	//2
	Push pull knockdowns	//3
	Fix grip absorb	//4
	Allow force combo	//5
	Fix pull strength	//6
	JK2 grip	//7
	Fast grip run speed	//8
	Push/pull items	//9
	Smaller Drain COF	//10

#### Weapon Tweaks 
	Nonrandom DEMP2	//1  
	Increased DEMP2 primary damage	//2  
	Decreased disruptor alt damage	//3  
	Nonrandom bowcaster spread	//4  
	Increased repeater alt damage	//5  
	Nonrandom flechette primary spread	//6  	
	Decreased flechette alt damage	//7  	
	Nonrandom flechette alt spread	//8  
	Increased concussion rifle alt damage	//9  
	Removed projectile knockback	//10  
	Stunbaton lightning gun	//11  
	Stunbaton shocklance	//12  
	Projectile gravity	//13  
	Allowcenter muzzle	//14  
	Pseudo random weapon spread	//15  
	Rocketalt firemortar	//16  
	Rocketalt fireredeemer	//17  
	Infinite ammo	//18  
	Stunbaton heal gun	//19  
	Weaponscan damage vehicles	//20  
	Allow gun roll	//21  
	Fast weapon switch	//22  
	Impact nitrons	//23  
	Flechette stake gun	//24  
	Fix droppedmine ammocount	//25  
	JK2 Style Alt Tripmine	//26  
	Projectile Sniper	//27  
	NoSpread	//28  

#### Vote Tweaks 
	Allow speccall votein siege gametype	//1
	Allow speccall vote in CTF/TFFA gametypes	//2
	Clearvote whengoing to spectate	//3
	Dontallow callvote for 30s after map load	//4
	Flood protect callvotes by IP	//5
	Dont allow mapcall votesfor 10 minutes at start of eachmap	//6
	Add vote delay for map call votesonly	//7
	Allow voting from spectate	//8
	Show votes in console	//9
	Only count voters inpass/fail calculation	//10
	Fix map change after gametype vote	//11

## Server Game Commands 
#### General 
	ammap	
	ammotd	
	amrename	
	amsay	
	amtele	
	amtelemark	
	ampsay	
	amvstr	
	best 
	changepassword	
	clanpass	
	clansay	
	clanwhois 	 
	dftop10	
	engage_fullforceduel	
	engage_gunduel	
	hide		
	ignore	
	jetpack	
	jump	
	launch	
	login	
	logout		
	modversion	
	move	
	notcompleted	
	nudge	
	practice	
	printstats	
	race		
	register	
	rocketchange		
	say_team_mod		
	serverconfig	
	showNet	
	spot		
	stats		
	throwflag	
	top10		
	vgs_cmd	
	voice_cmd		
	warp	
	warplist	
	whois	
   
#### Emotes 
	ambeg 
	ambeg2	
	ambernie 
	ambreakdance	
	ambreakdance2	
	ambreakdance3	
	ambreakdance4	
	amcheer	
	amcower	
	amdance	
	amflip	
    amhug	
	amsignal	
	amsignal2	
	amsignal3	
	amsignal4	
	amsit	
	amsit2	
	amsit3	
	amsit4	
	amsit5	
	amslap	
	amsleep	
	amsmack		
	amsurrender	
	amtaunt	
	amtaunt2	
    amnoisy	
	ampoint	
	amrage	
    amrun	
    amvictory   

#### AdminCommands 
	amban	
	amforceteam	
	amfreeze	
	amgrantadmin	
	aminfo	
	amkick	
	amkillvote	
	amlistmaps	
	amlockteam	
	amlogin	
	amlogout	
	amlookup	


## ClientCvars ##

#### JAPROHUD/DISPLAY 
	cg_movementKeys			0	
	cg_movementKeysX		451	
	cg_movementKeysY		430	
	cg_movementKeysSize		1	
	cg_speedometer			0	
	cg_speedometerX			132	
	cg_speedometerY			459	
	cg_speedometerSize		0.75	
	cg_tintHud				1	
	cg_drawTeamOverlayX		640	
	cg_drawTeamOverlayY		0	
	cg_drawJumpHeight		0	
    cg_drawJumpDistance		0	
	cg_raceTimer			2	
	cg_raceTimerSize		0.75	
	cg_raceTimerX			5	
	cg_raceTimerY			280	
	cg_groundspeedometer	0	
	cg_smallScoreboard		0	
	cg_chatBoxFontSize		1	
	cg_killMessage			1	
	cg_newFont				0	
	cg_crosshairRed			0	
	cg_crosshairGreen		0	
	cg_crosshairBlue		0	
	cg_crosshairAlpha		255	
	cg_hudColors			0	
	cg_drawScore			1  
	cg_drawVote				1  
	cg_centerHeight			0	
	cg_centerSize			1	
	cg_accelerometer		0	
	cg_speedGraph			0	
   	cg_drawHud				1

#### Strafehelper 
	cg_strafeHelper					0	
	cg_strafeHelper_FPS				0	
	cg_strafeHelperOffset			75	
	cg_strafeHelperInvertOffset		75	
	cg_strafeHelperLineWidth		1	
   	cg_strafeHelperPrecision		256	
	cg_strafeHelperCutoff			0	
    cg_strafeHelperActiveColor		0 255 0 200	
	cg_strafeHelperInactiveAlpha	200	

#### Sounds 
	cg_rollSounds	1	
	cg_jumpSounds	0	
	cg_chatSounds	1	
	cg_hitsounds	0	
   	cg_duelMusic	1	

#### Game Visuals 
	cg_remaps				1	
	cg_screenShake			2	
	cg_smoothCamera			0	
	cg_corpseEffects		1	
	cg_thirdPersonFlagAlpha	1	
	cg_drawDuelers			1	
	cg_drawRacers			2	
	cg_brightSkins			0	
	cg_alwaysShowAbsorb		0	
	cg_gunFov				0	
	cg_fleshSparks			7	
	cg_noFX					0	
	cg_drawYsalShell		1	
	cg_noTeleFX				0	
    cg_specHud				0	
    cg_newDrainFX			0  

#### Features 
	cg_simulatedProjectiles		0	
	cg_simulatedHitscan			1	
	cg_spectatorCameraDamp		0	
	cg_scopeSensitivity			1	
	cg_defaultModel				kyle	
	cg_defaultFemaleModel		jan	
	cg_forceAllyModel			none	
	cg_forceEnemyModel			none	
	cg_jumpHeight				0	
	cg_autoRecordDemo			0	
	cg_zoomSensitivity			2.5	
	cg_leadIndicator			0	
	cg_cosbyBotTarget			-1	
	cg_cosbyBotLS				0	
	cg_drawHitBox				0	
	cg_drawPlayerNames			0	
	cg_drawPlayerNamesScale		0.5	
	cg_autoScreenshot			0	
	cg_autoRecordRaceDemo		0	
	cg_autoKillWhenFalling		0	
	cg_engineModifications		0	

#### Autologin 
	cg_autoLoginServer1	0	
	cg_autoLoginPass1	0	
	cg_autoLoginServer2	0	
	cg_autoLoginPass2	0	
	cg_autoLoginServer3	0	
	cg_autoLoginPass3	0	

#### Logging 
	cg_logChat		1	
	cg_logFormat	1	
   
#### Strafe Trails 
	cg_strafeTrailPlayers		0	
	cg_strafeTrailLife			5	
	cg_strafeTrailRacersOnly	0	
	cg_strafeTrailRadius		2	
	cg_strafeTrailFPS			40	
	cg_logStrafeTrail			0	

	
#### Network
	cg_predictKnockback		0	
    cg_predictRacemode		0	
	cl_timeNudge			0	
	cl_maxPackets			30	
	cl_timenudgeDuration	0	
    
#### Userinfo 
	cp_pluginDisable	0	
	cp_clanPwd			none	
	cp_sbRGB1			0	
	cp_sbRGB2			0	

#### Flipkick 
	cg_fkDuration			50	
	cg_fkFirstJumpDuration	0	
	cg_fkSecondJumpDelay	0  

## Client game commands ##
#### General 
	showPlayerId
	serverconfig
	autoLogin
	+zoom
	-zoom
	+cosbybot
	-cosbybot
	saber
	amColor
	amrun
	modversion
	followredflag
	followblueflag
	login
	strafeHelper
	flipkick
	plugin
	addCheckpoint
	listCheckpoints
	deleteCheckpoint
	teleToCheckpoint
	clearTrail
	strafeTrail
	PTelemark
	PTele
	remapShader
	listRemaps
	loadTrail
	do

#### Plugin disable
	Disable forcejumps  //1
	Disable rolls  //1  
	Disable cartwheels  //3 
	New run animation  //4  
	Disable duel tele  //5
	Disable centerprint checkpoints  //6
	Show chatbox checkpoints  //7 
	Disable damage numbers  //8  
	Centermuzzle  //9  

#### Strafehelper 
	Original style  //0
	Updated style  //1
	Cgaz style  //2
	Warsow style  //3
	Sound  //4
	W  //5
	WA  //6
	WD  //7
	A  //8
	D  //9
	Rear  //10
	Center  //11
	Accel bar  //12
	Weze style  //13
	Line Crosshair  //14