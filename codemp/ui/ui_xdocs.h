#ifndef DEFAULT_FORMAT_CALLBACK
#error Please define a default format callback before including this file
#endif

#ifndef SIMPLE_FORMAT_CALLBACK
#error Please define a default format callback before including this file
#endif

#define NL "\n"
#define LINE( tabbing, color, prefix, separator, desc ) tabbing color prefix S_COLOR_WHITE separator desc
#define PADGROUP( specialChar, text ) specialChar text specialChar

// describes one possible setting. use their aliases when possible
#define DESCLINE( color, prefix, desc )		LINE( "    ", color, prefix, " "S_COLOR_GREY"-"S_COLOR_WHITE" ", desc )
#define EMPTYLINE( prefix, moreDesc )		LINE( "    ", "", prefix, "   ", moreDesc )
#define EXAMPLE( example, desc )			S_COLOR_WHITE "Example: "S_COLOR_GREY"\"" S_COLOR_WHITE example S_COLOR_GREY "\" -> " S_COLOR_WHITE desc ""

// aliases for DESCLINE
#define SETTING( setting, desc )			DESCLINE( S_COLOR_WHITE, PADGROUP( "\x1f", setting ), desc )
#define SPECIAL( special, desc )			DESCLINE( S_COLOR_YELLOW, PADGROUP( "\x1f", special ), desc )

// aliases for EMPTYLINE to continue the previous DESCLINE alias
#define SETTING_NEXTLINE( moreDesc )		EMPTYLINE( PADGROUP( "\x1f", "" ), moreDesc )
#define SPECIAL_NEXTLINE( moreDesc )		EMPTYLINE( PADGROUP( "\x1f", "" ), moreDesc )

#ifdef XDOCS_CVAR_HELP
#define XDOCS_CVAR_DEF(name, shortDesc, longDesc) { name, shortDesc, longDesc },
#define XDOCS_CVAR_BITFLAG_DEF( name, shortDesc, longDesc ) { name, shortDesc, longDesc },
#define XDOCS_CVAR_KEYVALUE_DEF( name, shortDesc, longDesc ) { name, shortDesc, longDesc },
#define XDOCS_CMD_DEF( name, shortDesc ) { name, shortDesc, DEFAULT_FORMAT_CALLBACK },
#endif

/* --------------------------------------------------- */
/* CVARS */

// EternalJK HUD cvars:

XDOCS_CVAR_DEF("cg_movementKeys", "Show the movement keys onscreen",
	SETTING("0", "Movement keys are hidden") NL
	SETTING("1", "Movement keys are shown")
)

XDOCS_CVAR_DEF("cg_movementKeysX", "Horizontal location of the movement keys onscreen",""
)

XDOCS_CVAR_DEF("cg_movementKeysY", "Vertical location of the movement keys onscreen",""
)

XDOCS_CVAR_DEF("cg_movementKeysSize", "Scale of the movement keys",""
)

XDOCS_CVAR_DEF("cg_speedometerSettings", "Configure with the /speedometer command",""
)

XDOCS_CVAR_DEF("cg_speedometerX", "Horizontal location of the speedometer onscreen",""
)

XDOCS_CVAR_DEF("cg_soeedometerY", "Vertical location of the speedometer onscreen",""
)

XDOCS_CVAR_DEF("cg_speedometerSize", "Scale of the speedometer",""
)

XDOCS_CVAR_DEF("cg_drawTeamOverlay", "Draw the team overlay for team-based gametypes",
	SETTING("0", "The team overlay is not drawn") NL
	SETTING("1", "The team overlay is drawn")
)

XDOCS_CVAR_DEF("cg_drawTeamOverlayX", "Horizontal location of the team overlay",
)

XDOCS_CVAR_DEF("cg_drawTeamOverlayY", "Vertical location of the team overlay",
)

XDOCS_CVAR_DEF("cg_raceTimer", "Show the race timer onscreen",
	SETTING("0", "Race timer is hidden") NL
	SETTING("1", "Race timer is shown")
)

XDOCS_CVAR_DEF("cg_raceTimerX", "Horizontal location of the race timer onscreen",""
)

XDOCS_CVAR_DEF("cg_raceTimerY", "Vertical location of the race timer onscreen",""
)

XDOCS_CVAR_DEF("cg_raceTimerSize", "Scale of the race timer",""
)

XDOCS_CVAR_DEF("cg_smallScoreboard", "Always use the small version of the scoreboard",
	SETTING("0", "Scoreboard scales depending on number of players (Base behavior)") NL
	SETTING("1", "Scoreboard is always small")
)

XDOCS_CVAR_DEF("cg_scoreDeaths", "Display score AND deaths on the scoreboard",
	"This does not work on base." NL
	SETTING("0", "Scoreboard only shows score (Base behavior)") NL
	SETTING("1", "Scoreboard shows score and deaths")
)

XDOCS_CVAR_DEF("cg_killMessage", "Print a kill message on the screen when you kill someone",
	SETTING("0", "Kill messages won't be printed") NL
	SETTING("1", "Kill messages will be printed")
)

XDOCS_CVAR_DEF("cg_newFont", "Uses a different font for the chat",
	SETTING("0", "Use the base font") NL
	SETTING("1", "Use the new font")
)

XDOCS_CVAR_DEF("cg_chatBoxFontSize", "Scale of the chat box font",""
)

XDOCS_CVAR_DEF("cg_chatBoxCutOffLength", "Length of chat box before starting a new line",""
)

XDOCS_CVAR_DEF("cg_crossHairRed", "Custom red color of the crosshair",""
)

XDOCS_CVAR_DEF("cg_crossHairGreen", "Custom green color of the crosshair",""
)

XDOCS_CVAR_DEF("cg_crossHairBlue", "Custom blue color of the crosshair",""
)

XDOCS_CVAR_DEF("cg_crossHairAlpha", "Custom transparency of the crosshair",""
)

XDOCS_CVAR_DEF("cg_hudColors", "Changes the colors of the HUD based on saber style and force",
	"This only applies to the simple HUD." NL
	SETTING("0", "Style and force colors will not change (Base behavior)") NL
	SETTING("1", "Style and force colors will change")
)

XDOCS_CVAR_DEF("cg_tintHud", "Changes the color of the HUD based on team color",
	"This does not apply to the simple HUD." NL
	SETTING("0", "HUD colors will not change") NL
	SETTING("1", "HUD colors will change")
)

XDOCS_CVAR_DEF("cg_drawScore", "Displays your score in the lower right",
	"This does not apply to the simple HUD." NL
	SETTING("0", "Score is hidden") NL
	SETTING("1", "Score is drawn beside the HUD") NL
	SETTING("2", "Score, team score, and bias is drawn")
)

XDOCS_CVAR_DEF("cg_drawScores", "Displays team scores in the upper right",
	SETTING("0", "Team scores are hidden") NL
	SETTING("1", "Team scores are drawn") NL
	SETTING("2", "Team scores are drawn and colored based on team colors")
)

XDOCS_CVAR_DEF("cg_drawVote", "Displays votecalls in the upper left",
	"If turned off, votecalls are still shown in the console." NL
	SETTING("0", "Callvotes are hidden") NL
	SETTING("1", "Callvotes are shown")
)

//Strafehelper

XDOCS_CVAR_DEF("cg_strafeHelper", "Configure with the /strafehelper command",""
)

//Sounds

XDOCS_CVAR_DEF("cg_rollSounds", "Play sound when players roll",
	SETTING("0", "Don't play roll sounds") NL
	SETTING("1", "Play roll sounds from all clients") NL
	SETTING("2", "Play roll sounds from other clients") NL
	SETTING("3", "Only play roll sounds from local client")
)

XDOCS_CVAR_DEF("cg_jumpSounds", "Play sound when players jump",
	SETTING("0", "Don't play jump sounds") NL
	SETTING("1", "Play all jump sounds") NL
	SETTING("2", "Play only when other clients jump") NL
	SETTING("3", "Play only when local client jumps")
)

XDOCS_CVAR_DEF("cg_chatSounds", "Play sound when chat messages appear",
	SETTING("0", "Don't play chat sounds") NL
	SETTING("1", "Play chat sounds")
)

XDOCS_CVAR_DEF("cg_hitSounds", "Play sound when you hit someone",
	SETTING("0", "Don't play the hit sound (Base behavior)") NL
	SETTING("1", "Play the hit sound")
)

XDOCS_CVAR_DEF("cg_raceSounds", "Play sound when race is started",
	SETTING("0", "Don't play race sound") NL
	SETTING("1", "Play race sounds")
)

//Visuals

XDOCS_CVAR_DEF("cg_remaps", "Show or hide serverside remaps",
	"This requires a vid_restart." NL
	SETTING("0", "Don't show remaps") NL
	SETTING("1", "Show remaps")
)

XDOCS_CVAR_DEF("cg_screenShake", "Shake screen when hit or while charging weapons",
	SETTING("0", "Screen doesn't shake") NL
	SETTING("1", "Screen shakes only when taking damage") NL
	SETTING("2", "Screen shakes when charging weaspons and taking damage")
)

XDOCS_CVAR_DEF("cg_drawScreenTints", "Turn the tint from water on or off",
	SETTING("0", "Removes the screen tint") NL
	SETTING("1", "Shows the screen tint")
)

XDOCS_CVAR_DEF("cg_smoothCamera", "Smooth camera movement",
	"This works best with low cameradamp." NL
	SETTING("0", "No smooth camera movement") NL
	SETTING("1", "Camera movements are smooth")
)

XDOCS_CVAR_DEF("cg_blood", "Show blood when shot by guns",
	"Blood only shows from guns; gibs only show on JAPRO servers." NL
	SETTING("0", "No blood and no gibs") NL
	SETTING("1", "Only show blood") NL
	SETTING("2", "Show blood and gibs")
)

XDOCS_CVAR_DEF("cg_thirdPersonFlagAlpha", "Custom transparency of the CTF flag",""
)

XDOCS_CVAR_DEF("cg_stylePlayer", "Configure with the /stylePlayer command",""
)

XDOCS_CVAR_DEF("cg_alwaysShowAbsorb", "Show absorb when it's in use",
	SETTING("0", "Absorb shows only when used against another power (Base behavior)") NL
	SETTING("1", "Absorb shows always when in use")
)

XDOCS_CVAR_DEF("cg_zoomFov", "The field of view when using +zoom",""
)

XDOCS_CVAR_DEF("cg_fleshSparks", "Maximum number of sparks from a saber hit",""
)

XDOCS_CVAR_DEF("cg_noFX", "Determines if effects and map models are shown",
	SETTING("0", "No effects are removed") NL
	SETTING("1", "Removes effects") NL
	SETTING("2", "Removes effects and speakers") NL
	SETTING("3", "Removes above and replaces misc map models") NL
	SETTING("4", "Removes effects, speakers, and misc map models")
)

XDOCS_CVAR_DEF("cg_noTeleFX", "Disables the teleportation effect",
	"This is also used for the spawn effect." NL
	SETTING("0", "Teleport effect is on") NL
	SETTING("1", "Teleport effect is off")
)


// ...

/* --------------------------------------------------- */
/* COMMANDS */

// EternalJK console commands:

XDOCS_CMD_DEF("clientlist", "Displays a list of all connected clients and their real client numbers")

// ...

#undef NL
#undef LINE
#undef PADGROUP

#undef DESCLINE
#undef EMPTYLINE
#undef EXAMPLE

#undef SETTING
#undef SPECIAL

#undef SETTING_NEXTLINE
#undef SPECIAL_NEXTLINE

#undef XDOCS_CVAR_DEF
#undef XDOCS_CMD_DEF