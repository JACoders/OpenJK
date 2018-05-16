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