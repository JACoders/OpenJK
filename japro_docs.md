## Server Cvars


#### CTF
	g_flagDrag			0  //Add air friction to flag carrier.
	g_fixFlagSuicide	0  //Don't auto return flag when flag carrier suicides.
	g_allowFlagThrow	0  //Allow use of /throwflag command.
	g_fixCTFScores		0  //Tweak CTF score amounts.
	g_fixFlagHitbox		0  	
	g_rabbit			0 //1=normal rabit, 2=sniper rabbit

#### Saber
	g_tweakSaber			0	//Configured with /tweakSaber command.
	g_backslashDamageScale	1	
	g_maxSaberDefense		0	
	g_saberTouchDmg			0	//Configure saber touch damage for MP dmgs. Can be >1 for more touch damage.
	g_fixGroundStab			0	//1=Groundstabs damage players on ground. 2=Groundstabs damage players on ground but with reduced damage.
	g_saberDuelSPDamage		1	//Toggle use of SP style damage in saber duels.
	g_forceDuelSPDamage		0	//Toggle use of SP style damage in force duels.
	g_saberDisable			0	//Configured with /saberDisable command.
	g_blueDamageScale		1	
	g_yellowDamageScale		1	
	g_redDamageScale		1	
	g_redDFADamageScale		1	

#### Force 
	g_tweakForce		0	//Configured with /tweakForce command.
	g_fixSaberInGrip	0	//1=Grip does not turn off lightsaber. 2=Same as 1 and also target can toggle lightsaber in grip. 3=Same as 2 and also target can switch saberstyle in grip.
	g_fixLightning		0	//1=Lightning gives forcepoints to target. 2=Same as 1 with reduced damage. 3=Same as 2 with no melee bonus.
	g_fixGetups			0  //1=Allow grip during knockdown recovery. 2=Allow grip/push/pull during knockdown recovery.
	g_teamAbsorbScale	1	//Scale forcepoints gained from teampush/teampull etc.
	g_teamHealScale		1	//Scale health given from team heal.
	g_teamEnergizeScale	1	//Scale forcepoints given from team energize.

#### Guns 
	g_tweakWeapons				0	//Configured with /tweakWeapons command	
	g_startingWeapons			8	//Configured with /startingWeapons command	
	g_weaponDamageScale			1	
	g_projectileVelocityScale	1	
	g_selfDamageScale			0.5	
	g_projectileInheritance		0//Scale of forward player momentum gained by projectiles.
	g_fullInheritance			0//Scale of all player momentum gained by projectiles.

#### Movement 
	g_slideOnPlayer			0	
	g_flipKick				0//1=JA+ style.  2=Floodprotected to one kick every 50ms.  3=JK2 style.
	g_nonRandomKnockdown	0	//1=Nonrandom knockdowns based on forcepoints.  2=Pseudorandom with less variance. 3=Nonrandom based on viewangle of target. 4=Random based on viewangle of target. 
	g_fixRoll				0	//1=JA+ style base roll. //2=Chainable roll. //3=JK2 style roll.
	g_onlyBhop				0	//1=Disable forcejumps for all players. 2=Let players choose if they want to disable forcejumps.
	g_tweakJetpack			0	
	g_movementStyle			1	//Force movement style for players. 0=SIEGE 1=JKA 2=QW 3=CPM 4=Q3 5=PJK 6=WSW
	g_LegDangle				1	//Toggle the leg dangle animation which is not predicted and results in jerkyness on ledges with high ping.
	g_fixHighFPSAbuse		0	//Make players who have more than 250fps behave at 250fps physics.
	g_fixSlidePhysics		0	//1=Fixed slide physics for NPCS.  2=Fixed slide physics for NPCs and players.
	g_fixGlitchKickDamage	0
	g_allowGrapple			0//1=Tarzan style. 2=JA+ Style.
	g_hookSpeed			2400//Speed that grapple hook travels at
	g_hookStrength			800//Speed that grapple pulls you at
	g_hookFloodProtect		600//Milliseconds between hook shots

#### Dueling 
	g_duelStartHealth			0	
	g_duelStartArmor			0	
	g_duelDistanceLimit			0	
	g_allowUseInDuel			1	
	g_allowGunDuel				1	
	g_saberDuelForceRegenTime	200  
	g_forceDuelForceRegenTime	200	
	g_duelRespawn				0	//Automatically respawn players to where they died after losing a duel.  

#### Admin 
	g_juniorAdminLevel	0	
	g_fullAdminLevel	0	
	g_juniorAdminPass	""
	g_fullAdminPass		""
	g_juniorAdminMsg	""
	g_fullAdminMsg		""
	g_allowNoFollow		0	//Allow players to hide themselves and not be spectated/seen in racemode.

#### Other Gameplay 
	g_flipKickDamageScale	1	
	g_maxFallDmg			0	
	g_startingItems			0	//Configured with /startingItems command.
	g_quakeStyleTeleport	0	//Preserve momentum during teleports.
	g_screenShake			0 //Force screenshake so players can't disable it with jaPRO.
	g_unlagged				0//Bitvalue. 1=Unlagged projectiles. 2=Unlagged hitscan.  4=Unlagged push/pull.
	g_allowSaberSwitch		0	
	g_allowTeamSuicide		0	
	g_emotesDisable			0//Configured with /toggleEmotes command
	g_godChat				0	
	g_showHealth			0 //Show healthbars above players heads when aimed at.  Requires map restart.	
	g_damageNumbers			0 //1-7.  Controls different types of damagenumber printouts.
	g_fixKillCredit			0 //1=Award kill credit after target suicides/spectates. 2=Also for disconnects/reconnects.
	g_stopHealthESP			0 //Don't send health info to other players (pain sounds can't be used to count health now).
	g_blockDuelHealthSpec	0 //Don't show duelers health to people in spectate.
	g_antiWallhack			0 //Experimental anti wallhack code.  Use the client plugin so it can tell the server where your camera position is, and if you use 3rd person.

#### Other 
	g_corpseRemovalTime			30	
	g_removeSpectatorPortals	0 //Network all entity data to spectators, useful if you have a recorder in spectate so you can record all POV's.
	g_consoleMOTD				""
	g_centerMOTDTime			5	
	g_centerMOTD				""
	g_fakeClients				0	
	g_lagIcon					0	
	g_allowSamePlayerNames		0	
	sv_maxTeamSize				0	
	g_tweakVote					0	//Configured with /tweakVote command.
	g_allowSpotting				0	
	g_allowTargetLaser			0	//Target laser used with +button14
	g_voteTimeout				180	//Time in seconds to lockout callvote aftera failed vote.
	g_allowVGS					0	
	g_pauseTime					120	
	g_unpauseTime				5	
	restricts					0	
	g_mercyRule					0	//If the score difference isgreater than X percent of the fraglimit, end the match.

#### Race/Accounts 
	g_raceMode					0	//0=Noracemode, 1=forcedracemode, 2=player can toggle race mode with /racecommand.
	g_allowRaceTele				0//1=Allow amtele in racemode. 2=Also allow noclip.
	g_allowRegistration			1=Allow registration. 2=also allow clan joining. 3=also allow clan creation
	sv_pluginKey				0	
	g_forceLogin				0//Force players to login in order to be ingame.


#### Loggging/Recording 
	g_duelLog		0//Log to duels.log.
	g_raceLog		0//Log to races.log (incase database gets messed up).	
	g_playerLog		0//Used by /amlookup
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
	saberDisable
	startingItems	
	startingWeapons	
	toggleAdmin	
	toggleEmotes  
	toggleVote	
	toggleuserinfovalidation	
	tweakForce	
	tweakSaber	
	tweakVote	
	tweakWeapons	

#### Saber Disables
	SABERSTYLE_BLUE //1
	SABERSTYLE_YELLOW //2
	SABERSTYLE_RED //3
	SABERSTYLE_DUAL //4
	SABERSTYLE_STAFF //5
	SABERSTYLE_DESANN //6
	SABERSTYLE_TAVION //7

#### Saber Tweaks 
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
	Fix red DFA Boost//14
	Make red DFA cost 0 forcepoints//15

#### Force Tweaks 
	No force power drain for crouch attack	//1
	Fix projectile force push dir	//2
	Knocked down players can be pushed/pulled//3
	No grip absorb	//4
	Allow force combo	//5
	Fix pull strength	//6
	JK2 grip	//7
	Fast grip run speed	//8
	Push/pull items	//9
	Smaller Drain COF	//10
	Push/pull can knockdown players like in JK2//11
	JK2 style knockdown getups//12
	JK2 push/pull in roll//13
	No drain absorb//14

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
	No Spread	//28  

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

#### Emote bitvalue order
	BEG
	BEG2
	BREAKDANCE
	CHEER
	COWER
	DANCE
	HUG
	NOISY
	POINT
	RAGE
	SIT
	SURRENDER
	SMACK
	TAUNT
	VICTORY
	JAWARUN
	BERNIE
	SLEEP
	SABERFLIP
	SLAP
	SIGNAL

## Server Game Commands 
#### General 	
	ammotd		
	amsay	
	best 
	changepassword	
	clanpass	
	clansay	
	clanwhois 	 
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
	rTop
	rLatest
	rWorst
	rRank
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
	ammap
	amrename
	amtele	
	amtelemark	
	ampsay	
	amvstr	
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
	cg_tintHud				1//Hud color tinting based on team.	
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
	cg_cleanChatbox			0  //1=Remove all colors from chat msgs, 2=Only remove color at begining of message

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
	cg_rollSounds	1//0=no sounds. 1=sounds for all rolls. 2=sounds for all rolls but your own.
	cg_jumpSounds	0//same as rollsounds but for jump.
	cg_chatSounds	1
	cg_hitsounds	0//0-4	
   	cg_duelMusic	1	

#### Game Visuals 
	cg_remaps				1//Toggle serverside texture changes.	
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
	cg_draw2D				1 //2=No tinted overlay during rage/ysal use.

#### Features 
	cg_simulatedProjectiles		0 //Wether to use simulated projectiles so bullets appear instantly.  Values > 1 don't draw the projectile right away.
	cg_simulatedHitscan			1 //Toggle predicted hitscan weapon effects.	
	cg_spectatorCameraDamp		0	
	cg_scopeSensitivity			1	
	cg_defaultModel				kyle	
	cg_defaultFemaleModel		jan	
	cg_forceAllyModel			none	
	cg_forceEnemyModel			none	
	cg_jumpHeight				0 //Specify jumpheight before jump is automaticaly canceled.
	cg_autoRecordDemo			0	
	cg_zoomSensitivity			2.5	
	cg_leadIndicator			0	
	cg_cosbyBotTarget			-1 //-1 = closest.  0-31 = clientnum.
	cg_cosbyBotLS				0	
	cg_drawHitBox				0	
	cg_drawPlayerNames			0	
	cg_drawPlayerNamesScale		0.5	
	cg_autoScreenshot			0 //Automatically take a screenshot at end of round.
	cg_autoRecordRaceDemo		0	
	cg_autoKillWhenFalling		0	
	cg_engineModifications		0	

#### Autologin 
	cg_autoLoginServer1	0 //IP of server to try to autologin on, used with /autologin command. If you are not on the right server, the password will not be sent to the server.
	cg_autoLoginPass1	0 //password to use.
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
	cg_strafeTrailFPS			40 //SV_FPS the strafetrail was recorded at.
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
	followRedFlag
	followBlueFlag
	followFastest
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

#### Strafehelper Options
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

## Map entity changes  
### df_trigger_finish  
#### Spawnflags  
JUMP1_ONLY JUMP2_ONLY JUMP3_ONLY ALLOW_HASTE ALLOW_JETPACK FFA_ONLY  
#### Keys  
noise - sound it makes  
awesomenoise - sound it makes when time is below 'speed' key (only in jka style)  
speed - max time in ms to play awesomenoise  
target  

### df_trigger_checkpoint
noise - sound it makes  

### trigger_newpush   
#### Spawnflags  
XVEL YVEL ZVEL PLAYERONLY NPCONLY  
#### Keys  
noise - sound it makes  
speed - scale to use  

### target_restrict  
#### Spawnflags  
REMOVE_RESTRICTIONS HASTE REMOVE_NEUTRALFLAG  
#### Keys  
REMOVE_RESTRICTIONS - Removes the specified restrictions (default only bhop)  
HASTE - Replaces the default onlybhop restriction with haste.  

### func_static 
Added spawnflag "NOT_IN_CTF" (512) to stop entity from spawning in CTF mode.  

### trigger_multiple
Added spawnflag "NO_RACEMODE" (4096) to prevent people in racemode from using it.  
Added spawnflag "NOT_IN_CTF" (8192) to prevent it from spawning in CTF.  

### trigger_teleport
Added spawnflag "KEEP_VELOCITY" (2) to preserve momentum after teleport.  

### target_teleporter
Added spawnflag "KEEP_VELOCITY" (1) to preserve momentum after teleport.  

