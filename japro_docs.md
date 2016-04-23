## Server Cvars ##

//JAPRO CTF
g_flagDrag		"0"	
g_fixFlagSuicide	"0"	
g_allowFlagThrow	"0"	
g_fixCTFScores		"0"	
g_fixFlagHitbox		"0"	
g_rabbit		"0"	

//JAPRO Saber
g_tweakSaber		"0"	//Configured with /tweakSaber command
g_backslashDamageScale	"1"	
g_maxSaberDefense	"0"		
g_saberTouchDmg		"0"	//Configure saber touch damage for MP dmgs.  Can be > 1 for more touch damage like in JK2.
g_fixGroundStab		"0"	
g_saberDuelSPDamage	"1"	 
g_forceDuelSPDamage	"0"	 
g_saberDisable		"0"	
g_blueDamageScale	"1"	 
g_yellowDamageScale	"1"	 
g_redDamageScale	"1"	 
g_redDFADamageScale	"1"	 

//JAPRO FORCE
g_tweakForce		"0"	//Configured with /tweakForce command
g_fixSaberInGrip	"0"	 
g_fixLightning		"0"	
g_fixGetups		"0"	
g_teamAbsorbScale	"1"	 
g_teamHealScale		"1"	
g_teamEnergizeScale	"1"	

//JAPRO GUNS
g_tweakWeapons		"0"	//Configured with /tweakWeapons command	
g_startingWeapons	"8"	//Configured with /startingWeapons command	
g_weaponDamageScale	"1"	
g_projectileVelocityScale	"1"	
g_selfDamageScale	"0.5"	
g_projectileInheritance	"0"	
g_fullInheritance	"0"	

//JAPRO MOVEMENT
g_slideOnPlayer		"0"	
g_flipKick		"0"	
g_nonRandomKnockdown	"0"	
g_fixRoll		"0"		
g_onlyBhop		"0"		
g_tweakJetpack		"0"	
g_movementStyle		"1"	
g_LegDangle		"1"	
g_fixHighFPSAbuse	"0"	
g_fixSlidePhysics	"0"	
g_fixRedDFA		"0"	
g_fixGlitchKickDamage	"0"	

//JAPRO DUELING
g_duelStartHealth	"0"	
g_duelStartArmor	"0"	
g_duelDistanceLimit	"0"	
g_allowUseInDuel	"1"	
g_allowGunDuel		"1"	
g_saberDuelForceRegenTime	"200"	
g_forceDuelForceRegenTime	"200"	
g_duelRespawn		"0"	

//JAPRO ADMIN
g_juniorAdminLevel	"0"	
g_fullAdminLevel	"0"	
g_juniorAdminPass	""	
g_fullAdminPass		""	
g_juniorAdminMsg	""	
g_fullAdminMsg		""	
g_allowNoFollow		"0"	 

//JAPRO OTHER Gameplay
g_flipKickDamageScale	"1"	
g_maxFallDmg		"0"	
g_startingItems		"0"	//Configured with /startingItems command.
g_quakeStyleTeleport	"0"	
g_screenShake		"0"	
g_unlagged		"0"			
g_allowSaberSwitch	"0"	
g_allowTeamSuicide	"0"	
g_godChat		"0"	
g_showHealth		"0"		
g_damageNumbers		"0"	
g_fixKillCredit		"0"	
g_stopHealthESP		"0"	
g_blockDuelHealthSpec	"0"	
g_antiWallhack		"0"	



//JAPRO Other
g_emotesDisable		"0"	
g_corpseRemovalTime	"30"	
g_removeSpectatorPortals	"0"	
g_consoleMOTD		""	
g_centerMOTDTime	"5"	
g_centerMOTD		""	
g_fakeClients		"0"	
g_lagIcon		"0"	
g_allowSamePlayerNames	"0"	
sv_maxTeamSize		"0"	
g_tweakVote		"0"	//Configured with /tweakVote command.
g_allowSpotting		"0"	
g_allowTargetLaser	"0"	
g_voteTimeout		"180"	//Time in seconds to lockout callvote after a failed vote.
g_allowVGS		"0"	
g_pauseTime		"120"		
g_unpauseTime		"5"		
restricts		"0"	
g_mercyRule		"0"	 //If the score difference is greater than X percent of the frag limit, end the match.

//JAPRO RACE / ACCOUNTS
g_raceMode		"0"	//0 = No racemode, 1 = forced racemode, 2 = client can toggle racemode with /race command.
g_allowRaceTele		"0"	
g_allowRegistration	"1"	
sv_pluginKey		"0"	
g_forceLogin		"0"	


//JAPRO LOGGING/RECORDING
g_duelLog		"0"	
g_raceLog		"0"	
g_playerLog		"0"	
sv_autoRaceDemo		"0"	

//JAPRO BOTS
bot_nochat		"0"	
bot_strafeOffset	"0"	
bot_frameTime		"0.008"	
g_newBotAI		"0"	
g_newBotAITarget	"-1"	//-2 = Target closest excluding other bots.  -1 = target closest.  0-31 = target clientnum.
bot_maxbots		"0"	

//DEBUG / TOOLS
g_showJumpSpot		"0"	//Draws a red line where a player touches ground as they land.  Useful with the /nudge command on entities to easily space jumps.


## Server RCON Commands: ##

accountInfo							
			
amban											
amgrantadmin							
amkick											
				
changepassword												
clearIP									
DBInfo									
deleteAccount							
				
forceteam										
gametype	//Instantly change the gametype of the server without having to reload the map											
			

pause		//Pause / unpause the game											

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

### Saber tweaks ###
	Skip saber interpolate for MP dmgs	//1
	JK2 1.02 Style Damage System	//2
	Reduced saberblock for MP damages	//3
	Reduce saberdrops for MP damages	//4
	Allow rollcancel for saber swings	//5
	Remove chainable swings from red stance	//6
	Fixed saberswitch	//7
	No aim backslash	//8
	JK2 red DFA	//9
	Fix yellow DFA	//10
	Spin red DFA	//11
	Spin backslash	//12
	JK2 Lunge	//13

### Force Tweaks ###

	No forcepower drain for crouch attack	//1
	Fix projectile force push dir	//2
	Push pull knockdowns	//3
	Fix grip absorb	//4
	Allow force combo	//5
	Fix pull strength	//6
	JK2 grip	//7
	Fast grip runspeed	//8
	Push/pull items	//9
	Smaller Drain COF	//10

### Weapon Tweaks ###

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
Stun baton lightning gun	//11
Stun baton shocklance	//12
Projectile gravity	//13
Allow center muzzle	//14
Pseudo random weapon spread	//15
Rocket alt fire mortar	//16
Rocket alt fire redeemer	//17
Infinite ammo	//18
Stun baton heal gun	//19
Weapons can damage vehicles	//20
Allow gunroll	//21
Fast weaponswitch	//22
Impact nitrons	//23
Flechette stake gun	//24
Fix dropped mine ammo count	//25
JK2 Style Alt Tripmine	//26
Projectile Sniper	//27
No Spread	//28

### Vote Tweaks ###

	Allow spec callvote in siege gametype	//1
	Allow spec callvote in CTF/TFFA gametypes	//2
	Clear vote when going to spectate	//3
	Dont allow callvote for 30s after mapload	//4
	Floodprotect callvotes by IP	//5
	Dont allow map callvotes for 10 minutes at start of each map	//6
	Add vote delay for map callvotes only	//7
	Allow voting from spectate	//8
	Show votes in console	//9
	Only count voters in pass/fail calculation	//10
	Fix mapchange after gametype vote	//11

## Server Game Commands: ##	

	addbot				Cmd_AddBot_f				
	amban				Cmd_Amban_f		
		
	ambeg				Cmd_EmoteBeg_f				 //EMOTE
	ambeg2				Cmd_EmoteBeg2_f			 //EMOTE
	ambernie			Cmd_EmoteBernie_f			 //EMOTE
	ambreakdance		Cmd_EmoteBreakdance_f		 //EMOTE
	ambreakdance2		Cmd_EmoteBreakdance2_f		 //EMOTE
	ambreakdance3		Cmd_EmoteBreakdance3_f		 //EMOTE
	ambreakdance4		Cmd_EmoteBreakdance4_f		 //EMOTE

	amcheer			Cmd_EmoteCheer_f			 //EMOTE
	amcower						 //EMOTE
	amdance						 //EMOTE

	amflip						 //EMOTE

	amforceteam					 
	amfreeze						 
	amgrantadmin					
	amhug							 //EMOTE
	aminfo								
	amkick							
	amkillvote						
	amlistmaps					 
	amlockteam						 
	amlogin						
	amlogout							
	amlookup						
	ammap								 
	ammotd					
			 
	amnoisy						 //EMOTE
	ampoint						 //EMOTE 
	amrage						 //EMOTE
	amrename						 
	amrun								//EMOTE
	amsay								
	amsignal						 //EMOTE
	amsignal2					 //EMOTE
	amsignal3						 //EMOTE
	amsignal4						 //EMOTE
	amsit								 //EMOTE
	amsit2						 //EMOTE
	amsit3						 //EMOTE
	amsit4							 //EMOTE
	amsit5						 //EMOTE
	amslap						 //EMOTE
	amsleep						 //EMOTE
	amsmack						 //EMOTE	
	amsurrender				 //EMOTE
	amtaunt						 //EMOTE
	amtaunt2						 //EMOTE
	amtele							 
	amtelemark						 
	amvictory						 //EMOTE

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

## Admin Commands: ##


## Client Cvars: ##

	//JAPRO HUD / DISPLAY
	cg_movementKeys	0	
	cg_movementKeysX	451	
	cg_movementKeysY	430	
	cg_movementKeysSize	1	
	cg_speedometer	0	
	cg_speedometerX	132	
	cg_speedometerY	459	
	cg_speedometerSize	0.75	
	cg_tintHud	1	
	cg_drawTeamOverlayX	640	
	cg_drawTeamOverlayY	0	
	cg_drawJumpHeight	0	
	cg_raceTimer	2	
	cg_raceTimerSize	0.75	
	cg_raceTimerX	5	
	cg_raceTimerY	280	 
	cg_groundspeedometer	0	
	cg_smallScoreboard	0	
	cg_chatBoxFontSize	1	
	cg_killMessage	1	
	cg_newFont	0	
	cg_crosshairRed	0	
	cg_crosshairGreen	0	
	cg_crosshairBlue	0	
	cg_crosshairAlpha	255	
	cg_hudColors	0	
	cg_drawScore	1	 //eh
	cg_drawVote	1	 //eh

	//Strafehelper
	cg_strafeHelper	0	
	cg_strafeHelper_FPS	0	
	cg_strafeHelperOffset	75	
	cg_strafeHelperInvertOffset	75	
	cg_strafeHelperLineWidth	1	

	//Sounds
	cg_rollSounds	1	
	cg_jumpSounds	0	
	cg_chatSounds	1	
	cg_hitsounds	0	

	//Visuals
	cg_remaps	1	
	cg_screenShake	2	
	cg_smoothCamera	0	
	cg_corpseEffects	1	
	cg_thirdPersonFlagAlpha	1	
	cg_drawDuelers	1	
	cg_drawRacers	2	
	cg_brightSkins	0	
	cg_alwaysShowAbsorb	0	
	cg_gunFov	0	
	cg_fleshSparks	7	
	cg_noFX	0	
	cg_drawYsalShell	1	
	cg_noTeleFX	0	

	//Features
	cg_simulatedProjectiles	0	
	cg_simulatedHitscan	1	
	cg_spectatorCameraDamp	0	
	cg_scopeSensitivity	1	
	cg_defaultModel	kyle	
	cg_defaultFemaleModel	jan	
	cg_forceAllyModel	none	
	cg_forceEnemyModel	none	
	cg_jumpHeight	0	
	cg_autoRecordDemo	0	
	cg_zoomSensitivity	2.5	
	cg_leadIndicator	0	
	cg_cosbyBotTarget	-1	
	cg_cosbyBotLS	0	
	cg_drawHitBox	0	
	cg_drawPlayerNames	0	
	cg_drawPlayerNamesScale	0.5	
	cg_autoScreenshot	0	
	cg_autoRecordRaceDemo	0	
	cg_autoKillWhenFalling	0	
	cg_engineModifications	0	

	//Auto login
	cg_autoLoginServer1	0	
	cg_autoLoginPass1	0	
	cg_autoLoginServer2	0	
	cg_autoLoginPass2	0	
	cg_autoLoginServer3	0	
	cg_autoLoginPass3	0	

	//Logging
	cg_logChat	1	
	cg_logFormat	1	

	//BETA
	cg_accelerometer	0	
	cg_speedGraph	0	
	cg_specHud	0	
	cg_duelMusic	1	
	cg_drawHud	1	

	cg_strafeHelperPrecision	256	
	cg_strafeHelperCutoff	0	

	cg_drawJumpDistance	0	
	

	cg_predictKnockback	0	

	cg_strafeHelperActiveColor	0 255 0 200	
	cg_strafeHelperInactiveAlpha200	

	cp_pluginDisable	0	 
	
	 
//unlocked - default
	cl_timeNudge	0	
	cl_maxPackets	30	


	 
	cp_clanPwd	none	 
	cp_sbRGB1	0	   
	cp_sbRGB2	0	   

	cg_predictRacemode	0	

	cg_centerHeight	0	
	cg_centerSize	1	
	cg_fkDuration	50	
	cg_fkFirstJumpDuration	0	
	cg_fkSecondJumpDelay	0	


	cg_strafeTrailPlayers	0	
	cg_strafeTrailLife	5	
	cg_strafeTrailRacersOnly	0	
	cg_strafeTrailRadius	2	
	cg_strafeTrailFPS	40	

	cg_newDrainFX	0	



	cg_logStrafeTrail	0	

	
	cl_timenudgeDuration	0	


## Client game commands: ##
	
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


