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

/*
=======================================================================

USER INTERFACE MAIN

=======================================================================
*/

#include <algorithm>
#include <vector>

#include "../server/exe_headers.h"

#include "ui_local.h"

#include "menudef.h"

#include "ui_shared.h"

#include "../ghoul2/G2.h"

#include "../game/bg_public.h"
#include "../game/anims.h"
extern stringID_table_t animTable [MAX_ANIMATIONS+1];

#include "../qcommon/stringed_ingame.h"
#include "../qcommon/stv_version.h"
#include "../qcommon/q_shared.h"

extern qboolean ItemParse_model_g2anim_go( itemDef_t *item, const char *animName );
extern qboolean ItemParse_asset_model_go( itemDef_t *item, const char *name );
extern qboolean ItemParse_model_g2skin_go( itemDef_t *item, const char *skinName );
extern qboolean UI_SaberModelForSaber( const char *saberName, char *saberModel );
extern qboolean UI_SaberSkinForSaber( const char *saberName, char *saberSkin );
extern void UI_SaberAttachToChar( itemDef_t *item );

extern qboolean PC_Script_Parse(const char **out);

#define LISTBUFSIZE 10240

static struct
{
	char	listBuf[LISTBUFSIZE];			//	The list of file names read in

	// For scrolling through file names
	int				currentLine;		//	Index to currentSaveFileComments[] currently highlighted
	int				saveFileCnt;		//	Number of save files read in

	int				awaitingSave;		//	Flag to see if user wants to overwrite a game.

	char			*savegameMap;
	int				savegameFromFlag;
} s_savegame;

#define MAX_SAVELOADFILES	100
#define MAX_SAVELOADNAME	32

#ifdef JK2_MODE
byte screenShotBuf[SG_SCR_WIDTH * SG_SCR_HEIGHT * 4];
#endif

typedef struct
{
	char *currentSaveFileName;						// file name of savegame
	char currentSaveFileComments[iSG_COMMENT_SIZE];	// file comment
	char currentSaveFileDateTimeString[iSG_COMMENT_SIZE];	// file time and date
	time_t currentSaveFileDateTime;
	char currentSaveFileMap[MAX_TOKEN_CHARS];			// map save game is from
} savedata_t;

static savedata_t s_savedata[MAX_SAVELOADFILES];
void UI_SetActiveMenu( const char* menuname,const char *menuID );
void ReadSaveDirectory (void);
void Item_RunScript(itemDef_t *item, const char *s);
qboolean Item_SetFocus(itemDef_t *item, float x, float y);

qboolean		Asset_Parse(char **buffer);
menuDef_t		*Menus_FindByName(const char *p);
void			Menus_HideItems(const char *menuName);
int				Text_Height(const char *text, float scale, int iFontIndex );
int				Text_Width(const char *text, float scale, int iFontIndex );
void			_UI_DrawTopBottom(float x, float y, float w, float h, float size);
void			_UI_DrawSides(float x, float y, float w, float h, float size);
void			UI_CheckVid1Data(const char *menuTo,const char *warningMenuName);
void			UI_GetVideoSetup ( void );
void			UI_UpdateVideoSetup ( void );
static void		UI_UpdateCharacterCvars ( void );
static void		UI_GetCharacterCvars ( void );
static void		UI_UpdateSaberCvars ( void );
static void		UI_GetSaberCvars ( void );
static void		UI_ResetSaberCvars ( void );
static void		UI_InitAllocForcePowers ( const char *forceName );
static void		UI_AffectForcePowerLevel ( const char *forceName );
static void		UI_ShowForceLevelDesc ( const char *forceName );
static void		UI_ResetForceLevels ( void );
static void		UI_ClearWeapons ( void );
static void		UI_GiveWeapon ( const int weaponIndex );
static void		UI_EquipWeapon ( const int weaponIndex );
static void		UI_LoadMissionSelectMenu( const char *cvarName );
static void		UI_AddWeaponSelection ( const int weaponIndex, const int ammoIndex, const int ammoAmount, const char *iconItemName,const char *litIconItemName, const char *hexBackground, const char *soundfile );
static void		UI_AddThrowWeaponSelection ( const int weaponIndex, const int ammoIndex, const int ammoAmount, const char *iconItemName,const char *litIconItemName, const char *hexBackground, const char *soundfile );
static void		UI_RemoveWeaponSelection ( const int weaponIndex );
static void		UI_RemoveThrowWeaponSelection ( void );
static void		UI_HighLightWeaponSelection ( const int selectionslot );
static void		UI_NormalWeaponSelection ( const int selectionslot );
static void		UI_NormalThrowSelection ( void );
static void		UI_HighLightThrowSelection( void );
static void		UI_ClearInventory ( void );
static void		UI_GiveInventory ( const int itemIndex, const int amount );
static void		UI_ForcePowerWeaponsButton(qboolean activeFlag);
static void		UI_UpdateCharacterSkin( void );
static void		UI_UpdateCharacter( qboolean changedModel );
static void		UI_UpdateSaberType( void );
static void		UI_UpdateSaberHilt( qboolean secondSaber );
//static void		UI_UpdateSaberColor( qboolean secondSaber );
static void		UI_InitWeaponSelect( void );
static void		UI_WeaponHelpActive( void );

#ifndef JK2_MODE
static void		UI_UpdateFightingStyle ( void );
static void		UI_UpdateFightingStyleChoices ( void );
static void		UI_CalcForceStatus(void);
#endif // !JK2_MODE

static void		UI_DecrementForcePowerLevel( void );
static void		UI_DecrementCurrentForcePower ( void );
static void		UI_ShutdownForceHelp( void );
static void		UI_ForceHelpActive( void );

#ifndef JK2_MODE
static void		UI_DemoSetForceLevels( void );
#endif // !JK2_MODE

static void		UI_RecordForceLevels( void );
static void		UI_RecordWeapons( void );
static void		UI_ResetCharacterListBoxes( void );


void		UI_LoadMenus(const char *menuFile, qboolean reset);
static void		UI_OwnerDraw(float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float special, float scale, vec4_t color, qhandle_t shader, int textStyle, int iFontIndex);
static qboolean UI_OwnerDrawVisible(int flags);
int				UI_OwnerDrawWidth(int ownerDraw, float scale);
static void		UI_Update(const char *name);
void			UI_UpdateCvars( void );
void			UI_ResetDefaults( void );
void			UI_AdjustSaveGameListBox( int currentLine );

void			Menus_CloseByName(const char *p);

// Movedata Sounds
enum
{
	MDS_NONE = 0,
	MDS_FORCE_JUMP,
	MDS_ROLL,
	MDS_SABER,
	MDS_MOVE_SOUNDS_MAX
};

enum
{
	MD_ACROBATICS = 0,
	MD_SINGLE_FAST,
	MD_SINGLE_MEDIUM,
	MD_SINGLE_STRONG,
	MD_DUAL_SABERS,
	MD_SABER_STAFF,
	MD_MOVE_TITLE_MAX
};

// Some hard coded badness
// At some point maybe this should be externalized to a .dat file
const char *datapadMoveTitleData[MD_MOVE_TITLE_MAX] =
{
"@MENUS_ACROBATICS",
"@MENUS_SINGLE_FAST",
"@MENUS_SINGLE_MEDIUM",
"@MENUS_SINGLE_STRONG",
"@MENUS_DUAL_SABERS",
"@MENUS_SABER_STAFF",
};

const char *datapadMoveTitleBaseAnims[MD_MOVE_TITLE_MAX] =
{
"BOTH_RUN1",
"BOTH_SABERFAST_STANCE",
"BOTH_STAND2",
"BOTH_SABERSLOW_STANCE",
"BOTH_SABERDUAL_STANCE",
"BOTH_SABERSTAFF_STANCE",
};

#define MAX_MOVES 16

typedef struct
{
	const char	*title;
	const char	*desc;
	const char	*anim;
	short	sound;
} datpadmovedata_t;

static datpadmovedata_t datapadMoveData[MD_MOVE_TITLE_MAX][MAX_MOVES] =
{
{
// Acrobatics
{ "@MENUS_FORCE_JUMP1",				"@MENUS_FORCE_JUMP1_DESC",			"BOTH_FORCEJUMP1",				MDS_FORCE_JUMP },
{ "@MENUS_FORCE_FLIP",				"@MENUS_FORCE_FLIP_DESC",			"BOTH_FLIP_F",					MDS_FORCE_JUMP },
{ "@MENUS_ROLL",						"@MENUS_ROLL_DESC",					"BOTH_ROLL_F",					MDS_ROLL },
{ "@MENUS_BACKFLIP_OFF_WALL",			"@MENUS_BACKFLIP_OFF_WALL_DESC",	"BOTH_WALL_FLIP_BACK1",			MDS_FORCE_JUMP },
{ "@MENUS_SIDEFLIP_OFF_WALL",			"@MENUS_SIDEFLIP_OFF_WALL_DESC",	"BOTH_WALL_FLIP_RIGHT",			MDS_FORCE_JUMP },
{ "@MENUS_WALL_RUN",					"@MENUS_WALL_RUN_DESC",				"BOTH_WALL_RUN_RIGHT",			MDS_FORCE_JUMP },
{ "@MENUS_LONG_JUMP",					"@MENUS_LONG_JUMP_DESC",			"BOTH_FORCELONGLEAP_START",		MDS_FORCE_JUMP },
{ "@MENUS_WALL_GRAB_JUMP",			"@MENUS_WALL_GRAB_JUMP_DESC",		"BOTH_FORCEWALLREBOUND_FORWARD",MDS_FORCE_JUMP },
{ "@MENUS_RUN_UP_WALL_BACKFLIP",		"@MENUS_RUN_UP_WALL_BACKFLIP_DESC",	"BOTH_FORCEWALLRUNFLIP_START",	MDS_FORCE_JUMP },
{ "@MENUS_JUMPUP_FROM_KNOCKDOWN",		"@MENUS_JUMPUP_FROM_KNOCKDOWN_DESC","BOTH_KNOCKDOWN3",				MDS_NONE },
{ "@MENUS_JUMPKICK_FROM_KNOCKDOWN",	"@MENUS_JUMPKICK_FROM_KNOCKDOWN_DESC","BOTH_KNOCKDOWN2",			MDS_NONE },
{ "@MENUS_ROLL_FROM_KNOCKDOWN",		"@MENUS_ROLL_FROM_KNOCKDOWN_DESC",	"BOTH_KNOCKDOWN1",				MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
},

{
//Single Saber, Fast Style
{ "@MENUS_STAB_BACK",					"@MENUS_STAB_BACK_DESC",			"BOTH_A2_STABBACK1",			MDS_SABER },
{ "@MENUS_LUNGE_ATTACK",				"@MENUS_LUNGE_ATTACK_DESC",			"BOTH_LUNGE2_B__T_",			MDS_SABER },
{ "@MENUS_FORCE_PULL_IMPALE",			"@MENUS_FORCE_PULL_IMPALE_DESC",	"BOTH_PULL_IMPALE_STAB",		MDS_SABER },
{ "@MENUS_FAST_ATTACK_KATA",			"@MENUS_FAST_ATTACK_KATA_DESC",		"BOTH_A1_SPECIAL",				MDS_SABER },
{ "@MENUS_ATTACK_ENEMYONGROUND",		"@MENUS_ATTACK_ENEMYONGROUND_DESC", "BOTH_STABDOWN",				MDS_FORCE_JUMP },
{ "@MENUS_CARTWHEEL",					"@MENUS_CARTWHEEL_DESC",			"BOTH_ARIAL_RIGHT",				MDS_FORCE_JUMP },
{ "@MENUS_BOTH_ROLL_STAB",			"@MENUS_BOTH_ROLL_STAB2_DESC",		"BOTH_ROLL_STAB",				MDS_SABER },
{ NULL, NULL, 0,	MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
},

{
//Single Saber, Medium Style
{ "@MENUS_SLASH_BACK",				"@MENUS_SLASH_BACK_DESC",			"BOTH_ATTACK_BACK",				MDS_SABER },
{ "@MENUS_FLIP_ATTACK",				"@MENUS_FLIP_ATTACK_DESC",			"BOTH_JUMPFLIPSLASHDOWN1",		MDS_FORCE_JUMP },
{ "@MENUS_FORCE_PULL_SLASH",			"@MENUS_FORCE_PULL_SLASH_DESC",		"BOTH_PULL_IMPALE_SWING",		MDS_SABER },
{ "@MENUS_MEDIUM_ATTACK_KATA",		"@MENUS_MEDIUM_ATTACK_KATA_DESC",	"BOTH_A2_SPECIAL",				MDS_SABER },
{ "@MENUS_ATTACK_ENEMYONGROUND",		"@MENUS_ATTACK_ENEMYONGROUND_DESC", "BOTH_STABDOWN",				MDS_FORCE_JUMP },
{ "@MENUS_CARTWHEEL",					"@MENUS_CARTWHEEL_DESC",			"BOTH_ARIAL_RIGHT",				MDS_FORCE_JUMP },
{ "@MENUS_BOTH_ROLL_STAB",			"@MENUS_BOTH_ROLL_STAB2_DESC",		"BOTH_ROLL_STAB",				MDS_SABER },
{ NULL, NULL, 0,	MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
},

{
//Single Saber, Strong Style
{ "@MENUS_SLASH_BACK",				"@MENUS_SLASH_BACK_DESC",			"BOTH_ATTACK_BACK",				MDS_SABER },
{ "@MENUS_JUMP_ATTACK",				"@MENUS_JUMP_ATTACK_DESC",			"BOTH_FORCELEAP2_T__B_",		MDS_FORCE_JUMP },
{ "@MENUS_FORCE_PULL_SLASH",			"@MENUS_FORCE_PULL_SLASH_DESC",		"BOTH_PULL_IMPALE_SWING",		MDS_SABER },
{ "@MENUS_STRONG_ATTACK_KATA",		"@MENUS_STRONG_ATTACK_KATA_DESC",	"BOTH_A3_SPECIAL",				MDS_SABER },
{ "@MENUS_ATTACK_ENEMYONGROUND",		"@MENUS_ATTACK_ENEMYONGROUND_DESC", "BOTH_STABDOWN",				MDS_FORCE_JUMP },
{ "@MENUS_CARTWHEEL",					"@MENUS_CARTWHEEL_DESC",			"BOTH_ARIAL_RIGHT",				MDS_FORCE_JUMP },
{ "@MENUS_BOTH_ROLL_STAB",			"@MENUS_BOTH_ROLL_STAB2_DESC",		"BOTH_ROLL_STAB",				MDS_SABER },
{ NULL, NULL, 0,	MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
},

{
//Dual Sabers
{ "@MENUS_SLASH_BACK",				"@MENUS_SLASH_BACK_DESC",			"BOTH_ATTACK_BACK",				MDS_SABER },
{ "@MENUS_FLIP_FORWARD_ATTACK",		"@MENUS_FLIP_FORWARD_ATTACK_DESC",	"BOTH_JUMPATTACK6",				MDS_FORCE_JUMP },
{ "@MENUS_DUAL_SABERS_TWIRL",			"@MENUS_DUAL_SABERS_TWIRL_DESC",	"BOTH_SPINATTACK6",				MDS_SABER },
{ "@MENUS_ATTACK_ENEMYONGROUND",		"@MENUS_ATTACK_ENEMYONGROUND_DESC", "BOTH_STABDOWN_DUAL",			MDS_FORCE_JUMP },
{ "@MENUS_DUAL_SABER_BARRIER",		"@MENUS_DUAL_SABER_BARRIER_DESC",	"BOTH_A6_SABERPROTECT",			MDS_SABER },
{ "@MENUS_DUAL_STAB_FRONT_BACK",		"@MENUS_DUAL_STAB_FRONT_BACK_DESC", "BOTH_A6_FB",					MDS_SABER },
{ "@MENUS_DUAL_STAB_LEFT_RIGHT",		"@MENUS_DUAL_STAB_LEFT_RIGHT_DESC", "BOTH_A6_LR",					MDS_SABER },
{ "@MENUS_CARTWHEEL",					"@MENUS_CARTWHEEL_DESC",			"BOTH_ARIAL_RIGHT",				MDS_FORCE_JUMP },
{ "@MENUS_BOTH_ROLL_STAB",			"@MENUS_BOTH_ROLL_STAB_DESC",		"BOTH_ROLL_STAB",				MDS_SABER },
{ NULL, NULL, 0,	MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
},

{
// Saber Staff
{ "@MENUS_STAB_BACK",					"@MENUS_STAB_BACK_DESC",			"BOTH_A2_STABBACK1",			MDS_SABER },
{ "@MENUS_BACK_FLIP_ATTACK",			"@MENUS_BACK_FLIP_ATTACK_DESC",		"BOTH_JUMPATTACK7",				MDS_FORCE_JUMP },
{ "@MENUS_SABER_STAFF_TWIRL",			"@MENUS_SABER_STAFF_TWIRL_DESC",	"BOTH_SPINATTACK7",				MDS_SABER },
{ "@MENUS_ATTACK_ENEMYONGROUND",		"@MENUS_ATTACK_ENEMYONGROUND_DESC", "BOTH_STABDOWN_STAFF",			MDS_FORCE_JUMP },
{ "@MENUS_SPINNING_KATA",				"@MENUS_SPINNING_KATA_DESC",		"BOTH_A7_SOULCAL",				MDS_SABER },
{ "@MENUS_KICK1",						"@MENUS_KICK1_DESC",				"BOTH_A7_KICK_F",				MDS_FORCE_JUMP },
{ "@MENUS_JUMP_KICK",					"@MENUS_JUMP_KICK_DESC",			"BOTH_A7_KICK_F_AIR",			MDS_FORCE_JUMP },
{ "@MENUS_SPLIT_KICK",				"@MENUS_SPLIT_KICK_DESC",			"BOTH_A7_KICK_RL",				MDS_FORCE_JUMP },
{ "@MENUS_SPIN_KICK",					"@MENUS_SPIN_KICK_DESC",			"BOTH_A7_KICK_S",				MDS_FORCE_JUMP },
{ "@MENUS_FLIP_KICK",					"@MENUS_FLIP_KICK_DESC",			"BOTH_A7_KICK_BF",				MDS_FORCE_JUMP },
{ "@MENUS_BUTTERFLY_ATTACK",			"@MENUS_BUTTERFLY_ATTACK_DESC",		"BOTH_BUTTERFLY_FR1",			MDS_SABER },
{ "@MENUS_BOTH_ROLL_STAB",			"@MENUS_BOTH_ROLL_STAB2_DESC",		"BOTH_ROLL_STAB",				MDS_SABER },
{ NULL, NULL, 0,	MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
{ NULL, NULL, 0,	MDS_NONE },
}
};


static int gamecodetoui[] = {4,2,3,0,5,1,6};

uiInfo_t uiInfo;

static void UI_RegisterCvars( void );
void UI_Load(void);

static int UI_GetScreenshotFormatForString( const char *str ) {
	if ( !Q_stricmp(str, "jpg") || !Q_stricmp(str, "jpeg") )
		return SSF_JPEG;
	else if ( !Q_stricmp(str, "tga") )
		return SSF_TGA;
	else if ( !Q_stricmp(str, "png") )
		return SSF_PNG;
	else
		return -1;
}

static const char *UI_GetScreenshotFormatString( int format )
{
	switch ( format )
	{
	default:
	case SSF_JPEG:
		return "jpg";
	case SSF_TGA:
		return "tga";
	case SSF_PNG:
		return "png";
	}
}

typedef struct cvarTable_s {
	vmCvar_t	*vmCvar;
	const char		*cvarName;
	const char		*defaultString;
	void		(*update)( void );
	uint32_t	cvarFlags;
} cvarTable_t;

vmCvar_t	ui_menuFiles;
vmCvar_t	ui_hudFiles;

vmCvar_t	ui_char_anim;
vmCvar_t	ui_char_model;
vmCvar_t	ui_char_skin_head;
vmCvar_t	ui_char_skin_torso;
vmCvar_t	ui_char_skin_legs;
vmCvar_t	ui_saber_type;
vmCvar_t	ui_saber;
vmCvar_t	ui_saber2;
vmCvar_t	ui_saber_color;
vmCvar_t	ui_saber2_color;
vmCvar_t	ui_char_color_red;
vmCvar_t	ui_char_color_green;
vmCvar_t	ui_char_color_blue;
vmCvar_t	ui_PrecacheModels;
vmCvar_t	ui_screenshotType;

static void UI_UpdateScreenshot( void )
{
	qboolean changed = qfalse;
	// check some things
	if ( ui_screenshotType.string[0] && isalpha( ui_screenshotType.string[0] ) )
	{
		int ssf = UI_GetScreenshotFormatForString( ui_screenshotType.string );
		if ( ssf == -1 )
		{
			ui.Printf( "UI Screenshot Format Type '%s' unrecognised, defaulting to JPEG\n", ui_screenshotType.string );
			uiInfo.uiDC.screenshotFormat = SSF_JPEG;
			changed = qtrue;
		}
		else
			uiInfo.uiDC.screenshotFormat = ssf;
	}
	else if ( ui_screenshotType.integer < SSF_JPEG || ui_screenshotType.integer > SSF_PNG )
	{
		ui.Printf( "ui_screenshotType %i is out of range, defaulting to 0 (JPEG)\n", ui_screenshotType.integer );
		uiInfo.uiDC.screenshotFormat = SSF_JPEG;
		changed = qtrue;
	}
	else {
		uiInfo.uiDC.screenshotFormat = atoi( ui_screenshotType.string );
		changed = qtrue;
	}

	if ( changed ) {
		Cvar_Set( "ui_screenshotType", UI_GetScreenshotFormatString( uiInfo.uiDC.screenshotFormat ) );
		Cvar_Update( &ui_screenshotType );
	}
}

static cvarTable_t cvarTable[] =
{
	{ &ui_menuFiles,			"ui_menuFiles",			"ui/menus.txt", NULL, CVAR_ARCHIVE },
#ifdef JK2_MODE
	{ &ui_hudFiles,				"cg_hudFiles",			"ui/jk2hud.txt", NULL, CVAR_ARCHIVE},
#else
	{ &ui_hudFiles,				"cg_hudFiles",			"ui/jahud.txt", NULL, CVAR_ARCHIVE},
#endif

	{ &ui_char_anim,			"ui_char_anim",			"BOTH_WALK1", NULL, 0},

	{ &ui_char_model,			"ui_char_model",		"", NULL, 0},	//these are filled in by the "g_*" versions on load
	{ &ui_char_skin_head,		"ui_char_skin_head",	"", NULL, 0},	//the "g_*" versions are initialized in UI_Init, ui_atoms.cpp
	{ &ui_char_skin_torso,		"ui_char_skin_torso",	"", NULL, 0},
	{ &ui_char_skin_legs,		"ui_char_skin_legs",	"", NULL, 0},

	{ &ui_saber_type,			"ui_saber_type",		"", NULL, 0},
	{ &ui_saber,				"ui_saber",				"", NULL, 0},
	{ &ui_saber2,				"ui_saber2",			"", NULL, 0},
	{ &ui_saber_color,			"ui_saber_color",		"", NULL, 0},
	{ &ui_saber2_color,			"ui_saber2_color",		"", NULL, 0},

	{ &ui_char_color_red,		"ui_char_color_red",	"", NULL, 0},
	{ &ui_char_color_green,		"ui_char_color_green",	"", NULL, 0},
	{ &ui_char_color_blue,		"ui_char_color_blue",	"", NULL, 0},

	{ &ui_PrecacheModels,		"ui_PrecacheModels",	"1", NULL, CVAR_ARCHIVE},

	{ &ui_screenshotType,		"ui_screenshotType",	"jpg", UI_UpdateScreenshot, CVAR_ARCHIVE }
};

#define FP_UPDATED_NONE -1
#define NOWEAPON -1

static const size_t cvarTableSize = ARRAY_LEN( cvarTable );

void Text_Paint(float x, float y, float scale, vec4_t color, const char *text, int iMaxPixelWidth, int style, int iFontIndex);
int Key_GetCatcher( void );

#define	UI_FPS_FRAMES	4
void _UI_Refresh( int realtime )
{
	static int index;
	static int	previousTimes[UI_FPS_FRAMES];

	if ( !( Key_GetCatcher() & KEYCATCH_UI ) )
	{
		return;
	}

	extern void SE_CheckForLanguageUpdates(void);
	SE_CheckForLanguageUpdates();

	if ( Menus_AnyFullScreenVisible() )
	{//if not in full screen, don't mess with ghoul2
		//rww - ghoul2 needs to know what time it is even if the client/server are not running
		//FIXME: this screws up the game when you go back to the game...
		re.G2API_SetTime(realtime, 0);
		re.G2API_SetTime(realtime, 1);
	}

	uiInfo.uiDC.frameTime = realtime - uiInfo.uiDC.realTime;
	uiInfo.uiDC.realTime = realtime;

	previousTimes[index % UI_FPS_FRAMES] = uiInfo.uiDC.frameTime;
	index++;
	if ( index > UI_FPS_FRAMES )
	{
		int i, total;
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for ( i = 0 ; i < UI_FPS_FRAMES ; i++ )
		{
			total += previousTimes[i];
		}
		if ( !total )
		{
			total = 1;
		}
		uiInfo.uiDC.FPS = 1000 * UI_FPS_FRAMES / total;
	}

	UI_UpdateCvars();

	if (Menu_Count() > 0)
	{
		// paint all the menus
		Menu_PaintAll();
		// refresh server browser list
//		UI_DoServerRefresh();
		// refresh server status
//		UI_BuildServerStatus(qfalse);
		// refresh find player list
//		UI_BuildFindPlayerList(qfalse);
	}

	// draw cursor
	UI_SetColor( NULL );
	if (Menu_Count() > 0)
	{
		if (uiInfo.uiDC.cursorShow == qtrue)
		{
			UI_DrawHandlePic( uiInfo.uiDC.cursorx, uiInfo.uiDC.cursory, 48, 48, uiInfo.uiDC.Assets.cursor);
		}
	}
}

#define MODSBUFSIZE (MAX_MODS * MAX_QPATH)

/*
===============
UI_LoadMods
===============
*/
static void UI_LoadMods() {
	int		numdirs;
	char	dirlist[MODSBUFSIZE];
	char	*dirptr;
	char	*descptr;
	int		i;
	int		dirlen;

	uiInfo.modCount = 0;

	numdirs = FS_GetFileList( "$modlist", "", dirlist, sizeof(dirlist) );
	dirptr  = dirlist;
	for( i = 0; i < numdirs; i++ ) {
		dirlen = strlen( dirptr ) + 1;
		descptr = dirptr + dirlen;
		uiInfo.modList[uiInfo.modCount].modName = String_Alloc(dirptr);
		uiInfo.modList[uiInfo.modCount].modDescr = String_Alloc(descptr);
		dirptr += dirlen + strlen(descptr) + 1;
		uiInfo.modCount++;
		if (uiInfo.modCount >= MAX_MODS) {
			break;
		}
	}
}

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .qvm file
================
*/
extern "C" Q_EXPORT intptr_t QDECL vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11  )
{
	return 0;
}



/*
================
Text_PaintChar
================
*/
/*
static void Text_PaintChar(float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader)
{
	float w, h;

	w = width * scale;
	h = height * scale;
	ui.R_DrawStretchPic((int)x, (int)y, w, h, s, t, s2, t2, hShader );	//make the coords (int) or else the chars bleed
}
*/

/*
================
Text_Paint
================
*/
// iMaxPixelWidth is 0 here for no limit (but gets converted to -1), else max printable pixel width relative to start pos
//
void Text_Paint(float x, float y, float scale, vec4_t color, const char *text, int iMaxPixelWidth, int style, int iFontIndex)
{
	if (iFontIndex == 0)
	{
		iFontIndex = uiInfo.uiDC.Assets.qhMediumFont;
	}
	// kludge.. convert JK2 menu styles to SOF2 printstring ctrl codes...
	//
	int iStyleOR = 0;
	switch (style)
	{
//	case  ITEM_TEXTSTYLE_NORMAL:			iStyleOR = 0;break;					// JK2 normal text
//	case  ITEM_TEXTSTYLE_BLINK:				iStyleOR = STYLE_BLINK;break;		// JK2 fast blinking
	case  ITEM_TEXTSTYLE_PULSE:				iStyleOR = STYLE_BLINK;break;		// JK2 slow pulsing
	case  ITEM_TEXTSTYLE_SHADOWED:			iStyleOR = STYLE_DROPSHADOW;break;	// JK2 drop shadow ( need a color for this )
	case  ITEM_TEXTSTYLE_OUTLINED:			iStyleOR = STYLE_DROPSHADOW;break;	// JK2 drop shadow ( need a color for this )
	case  ITEM_TEXTSTYLE_OUTLINESHADOWED:	iStyleOR = STYLE_DROPSHADOW;break;	// JK2 drop shadow ( need a color for this )
	case  ITEM_TEXTSTYLE_SHADOWEDMORE:		iStyleOR = STYLE_DROPSHADOW;break;	// JK2 drop shadow ( need a color for this )
	}

	ui.R_Font_DrawString(	x,		// int ox
							y,		// int oy
							text,	// const char *text
							color,	// paletteRGBA_c c
							iStyleOR | iFontIndex,	// const int iFontHandle
							!iMaxPixelWidth?-1:iMaxPixelWidth,	// iMaxPixelWidth (-1 = none)
							scale	// const float scale = 1.0f
							);
}


/*
================
Text_PaintWithCursor
================
*/
// iMaxPixelWidth is 0 here for no-limit
void Text_PaintWithCursor(float x, float y, float scale, vec4_t color, const char *text, int cursorPos, char cursor, int iMaxPixelWidth, int style, int iFontIndex)
{
	Text_Paint(x, y, scale, color, text, iMaxPixelWidth, style, iFontIndex);

	// now print the cursor as well...
	//
	char sTemp[1024];
	int iCopyCount = iMaxPixelWidth > 0 ? Q_min( (int)strlen( text ), iMaxPixelWidth ) : (int)strlen( text );
		iCopyCount = Q_min(iCopyCount, cursorPos);
		iCopyCount = Q_min(iCopyCount,(int)sizeof(sTemp));

	// copy text into temp buffer for pixel measure...
	//
	strncpy(sTemp,text,iCopyCount);
			sTemp[iCopyCount] = '\0';

	int iNextXpos  = ui.R_Font_StrLenPixels(sTemp, iFontIndex, scale );

	Text_Paint(x+iNextXpos, y, scale, color, va("%c",cursor), iMaxPixelWidth, style|ITEM_TEXTSTYLE_BLINK, iFontIndex);
}


const char *UI_FeederItemText(float feederID, int index, int column, qhandle_t *handle)
{
	*handle = -1;

	if (feederID == FEEDER_SAVEGAMES)
	{
		if (column==0)
		{
			return s_savedata[index].currentSaveFileComments;
		}
		else
		{
			return s_savedata[index].currentSaveFileDateTimeString;
		}
	}
	else if (feederID == FEEDER_MOVES)
	{
		return datapadMoveData[uiInfo.movesTitleIndex][index].title;
	}
	else if (feederID == FEEDER_MOVES_TITLES)
	{
		return datapadMoveTitleData[index];
	}
	else if (feederID == FEEDER_PLAYER_SPECIES)
	{
		if (index >= 0 && index < uiInfo.playerSpeciesCount)
		{
			return uiInfo.playerSpecies[index].Name;
		}
	}
	else if (feederID == FEEDER_LANGUAGES)
	{
#ifdef JK2_MODE
		// FIXME
		return NULL;
#else
		return SE_GetLanguageName( index );
#endif
	}
	else if (feederID == FEEDER_PLAYER_SKIN_HEAD)
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHeadCount)
		{
			*handle = ui.R_RegisterShaderNoMip(va("models/players/%s/icon_%s.jpg", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Name, uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHead[index].name));
			return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHead[index].name;
		}
	}
	else if (feederID == FEEDER_PLAYER_SKIN_TORSO)
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorsoCount)
		{
			*handle = ui.R_RegisterShaderNoMip(va("models/players/%s/icon_%s.jpg", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Name, uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorso[index].name));
			return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorso[index].name;
		}
	}
	else if (feederID == FEEDER_PLAYER_SKIN_LEGS)
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLegCount)
		{
			*handle = ui.R_RegisterShaderNoMip(va("models/players/%s/icon_%s.jpg", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Name, uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLeg[index].name));
			return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLeg[index].name;
		}
	}
	else if (feederID == FEEDER_COLORCHOICES)
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].ColorCount)
		{
			*handle = ui.R_RegisterShaderNoMip( uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Color[index].shader);
			return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Color[index].shader;
		}
	}
	else if (feederID == FEEDER_MODS)
	{
		if (index >= 0 && index < uiInfo.modCount)
		{
			if (uiInfo.modList[index].modDescr && *uiInfo.modList[index].modDescr)
			{
				return uiInfo.modList[index].modDescr;
			}
			else
			{
				return uiInfo.modList[index].modName;
			}
		}
	}

	return "";
}

qhandle_t UI_FeederItemImage(float feederID, int index)
{
	if (feederID == FEEDER_PLAYER_SKIN_HEAD)
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHeadCount)
		{
			//return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHeadIcons[index];
			return ui.R_RegisterShaderNoMip(va("models/players/%s/icon_%s.jpg", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Name, uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHead[index].name));
		}
	}
	else if (feederID == FEEDER_PLAYER_SKIN_TORSO)
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorsoCount)
		{
			//return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorsoIcons[index];
			return ui.R_RegisterShaderNoMip(va("models/players/%s/icon_%s.jpg", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Name, uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorso[index].name));
		}
	}
	else if (feederID == FEEDER_PLAYER_SKIN_LEGS)
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLegCount)
		{
			//return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLegIcons[index];
			return ui.R_RegisterShaderNoMip(va("models/players/%s/icon_%s.jpg", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Name, uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLeg[index].name));
		}
	}
	else if (feederID == FEEDER_COLORCHOICES)
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].ColorCount)
		{
			return ui.R_RegisterShaderNoMip( uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Color[index].shader);
		}
	}
/*	else if (feederID == FEEDER_ALLMAPS || feederID == FEEDER_MAPS)
	{
		int actual;
		UI_SelectedMap(index, &actual);
		index = actual;
		if (index >= 0 && index < uiInfo.mapCount)
		{
			if (uiInfo.mapList[index].levelShot == -1)
			{
				uiInfo.mapList[index].levelShot = trap_R_RegisterShaderNoMip(uiInfo.mapList[index].imageName);
			}
			return uiInfo.mapList[index].levelShot;
		}
	}
*/
	return 0;
}


/*
=================
CreateNextSaveName
=================
*/
static int CreateNextSaveName(char *fileName)
{
	int i;

	// Loop through all the save games and look for the first open name
	for (i=0;i<MAX_SAVELOADFILES;i++)
	{
#ifdef JK2_MODE
		Com_sprintf( fileName, MAX_SAVELOADNAME, "jkii%02d", i );
#else
		Com_sprintf( fileName, MAX_SAVELOADNAME, "jedi_%02d", i );
#endif

		if (!ui.SG_GetSaveGameComment(fileName, NULL, NULL))
		{
			return qtrue;
		}
	}

	return qfalse;
}

/*
===============
UI_DeferMenuScript

Return true if the menu script should be deferred for later
===============
*/
static qboolean UI_DeferMenuScript ( const char **args )
{
	const char* name;

	// Whats the reason for being deferred?
	if (!String_Parse(args, &name))
	{
		return qfalse;
	}

	// Handle the custom cases
	if ( !Q_stricmp ( name, "VideoSetup" ) )
	{
		const char* warningMenuName;
		qboolean	deferred;

		// No warning menu specified
		if ( !String_Parse(args, &warningMenuName) )
		{
			return qfalse;
		}

		// Defer if the video options were modified
		deferred = Cvar_VariableIntegerValue( "ui_r_modified" ) ? qtrue : qfalse;

		if ( deferred )
		{
			// Open the warning menu
			Menus_OpenByName(warningMenuName);
		}

		return deferred;
	}

	return qfalse;
}

/*
===============
UI_RunMenuScript
===============
*/
static qboolean UI_RunMenuScript ( const char **args )
{
	const char *name, *name2,*mapName,*menuName,*warningMenuName;

	if (String_Parse(args, &name))
	{
		if (Q_stricmp(name, "resetdefaults") == 0)
		{
			UI_ResetDefaults();
		}
		else if (Q_stricmp(name, "saveControls") == 0)
		{
			Controls_SetConfig();
		}
		else if (Q_stricmp(name, "loadControls") == 0)
		{
			Controls_GetConfig();
		}
		else if (Q_stricmp(name, "clearError") == 0)
		{
			Cvar_Set("com_errorMessage", "");
		}
		else if (Q_stricmp(name, "ReadSaveDirectory") == 0)
		{
			s_savegame.saveFileCnt = -1;	//force a refresh at drawtime
//			ReadSaveDirectory();
		}
		else if (Q_stricmp(name, "loadAuto") == 0)
		{
			Menus_CloseAll();
			ui.Cmd_ExecuteText( EXEC_APPEND, "load auto\n");	//load game menu
		}
		else if (Q_stricmp(name, "loadgame") == 0)
		{
			if (s_savedata[s_savegame.currentLine].currentSaveFileName)// && (*s_file_desc_field.field.buffer))
			{
				Menus_CloseAll();
				ui.Cmd_ExecuteText( EXEC_APPEND, va("load %s\n", s_savedata[s_savegame.currentLine].currentSaveFileName));
			}
			// after loading a game, the list box (and it's highlight) get's reset back to 0, but currentLine sticks around, so set it to 0 here
			s_savegame.currentLine = 0;

		}
		else if (Q_stricmp(name, "deletegame") == 0)
		{
			if (s_savedata[s_savegame.currentLine].currentSaveFileName)	// A line was chosen
			{
#ifndef FINAL_BUILD
				ui.Printf( va("%s\n","Attempting to delete game"));
#endif

				ui.Cmd_ExecuteText( EXEC_NOW, va("wipe %s\n", s_savedata[s_savegame.currentLine].currentSaveFileName));

				if( (s_savegame.currentLine>0) && ((s_savegame.currentLine+1) == s_savegame.saveFileCnt) )
				{
					s_savegame.currentLine--;
					// yeah this is a pretty bad hack
					// adjust cursor position of listbox so correct item is highlighted
					UI_AdjustSaveGameListBox( s_savegame.currentLine );
				}

//				ReadSaveDirectory();	//refresh
				s_savegame.saveFileCnt = -1;	//force a refresh at drawtime

			}
		}
		else if (Q_stricmp(name, "savegame") == 0)
		{
			char fileName[MAX_SAVELOADNAME];
			char description[64];
			// Create a new save game
//			if ( !s_savedata[s_savegame.currentLine].currentSaveFileName)	// No line was chosen
			{
				CreateNextSaveName(fileName);	// Get a name to save to
			}
//			else	// Overwrite a current save game? Ask first.
			{
//				s_savegame.yes.generic.flags	= QMF_HIGHLIGHT_IF_FOCUS;
//				s_savegame.no.generic.flags		= QMF_HIGHLIGHT_IF_FOCUS;

//				strcpy(fileName,s_savedata[s_savegame.currentLine].currentSaveFileName);
//				s_savegame.awaitingSave = qtrue;
//				s_savegame.deletegame.generic.flags	= QMF_GRAYED;	// Turn off delete button
//				break;
			}

			// Save description line
			ui.Cvar_VariableStringBuffer("ui_gameDesc",description,sizeof(description));
			ui.SG_StoreSaveGameComment(description);

			ui.Cmd_ExecuteText( EXEC_APPEND, va("save %s\n", fileName));
			s_savegame.saveFileCnt = -1;	//force a refresh the next time around
		}
		else if (Q_stricmp(name, "LoadMods") == 0)
		{
			UI_LoadMods();
		}
		else if (Q_stricmp(name, "RunMod") == 0)
		{
			if (uiInfo.modList[uiInfo.modIndex].modName)
			{
				Cvar_Set( "fs_game", uiInfo.modList[uiInfo.modIndex].modName);
				extern	void FS_Restart( qboolean inPlace = qfalse );
				FS_Restart();
				Cbuf_ExecuteText( EXEC_APPEND, "vid_restart;" );
			}
		}
		else if (Q_stricmp(name, "Quit") == 0)
		{
			Cbuf_ExecuteText( EXEC_NOW, "quit");
		}
		else if (Q_stricmp(name, "Controls") == 0)
		{
			Cvar_Set( "cl_paused", "1" );
			trap_Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName("setup_menu2");
		}
		else if (Q_stricmp(name, "Leave") == 0)
		{
			Cbuf_ExecuteText( EXEC_APPEND, "disconnect\n" );
			trap_Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			//Menus_ActivateByName("mainMenu");
		}
		else if (Q_stricmp(name, "getvideosetup") == 0)
		{
			UI_GetVideoSetup ( );
		}
		else if (Q_stricmp(name, "updatevideosetup") == 0)
		{
			UI_UpdateVideoSetup ( );
		}
		else if (Q_stricmp(name, "nextDataPadForcePower") == 0)
		{
			ui.Cmd_ExecuteText( EXEC_APPEND, "dpforcenext\n");
		}
		else if (Q_stricmp(name, "prevDataPadForcePower") == 0)
		{
			ui.Cmd_ExecuteText( EXEC_APPEND, "dpforceprev\n");
		}
		else if (Q_stricmp(name, "nextDataPadWeapon") == 0)
		{
			ui.Cmd_ExecuteText( EXEC_APPEND, "dpweapnext\n");
		}
		else if (Q_stricmp(name, "prevDataPadWeapon") == 0)
		{
			ui.Cmd_ExecuteText( EXEC_APPEND, "dpweapprev\n");
		}
		else if (Q_stricmp(name, "nextDataPadInventory") == 0)
		{
			ui.Cmd_ExecuteText( EXEC_APPEND, "dpinvnext\n");
		}
		else if (Q_stricmp(name, "prevDataPadInventory") == 0)
		{
			ui.Cmd_ExecuteText( EXEC_APPEND, "dpinvprev\n");
		}
		else if (Q_stricmp(name, "checkvid1data") == 0)		// Warn user data has changed before leaving screen?
		{
			String_Parse(args, &menuName);

			String_Parse(args, &warningMenuName);

			UI_CheckVid1Data(menuName,warningMenuName);
		}
		else if (Q_stricmp(name, "startgame") == 0)
		{
			Menus_CloseAll();
#ifdef JK2_MODE
			ui.Cmd_ExecuteText( EXEC_APPEND, "map kejim_post\n" );
#else
			ui.Cmd_ExecuteText( EXEC_APPEND, "map yavin1\n");
#endif
		}
		else if (Q_stricmp(name, "startmap") == 0)
		{
			Menus_CloseAll();

			String_Parse(args, &mapName);

			ui.Cmd_ExecuteText( EXEC_APPEND, va("maptransition %s\n",mapName));
		}
		else if (Q_stricmp(name, "closeingame") == 0)
		{
			trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
			trap_Key_ClearStates();
			Cvar_Set( "cl_paused", "0" );
			Menus_CloseAll();

			if (1 == Cvar_VariableIntegerValue("ui_missionfailed"))
			{
				Menus_ActivateByName("missionfailed_menu");
				ui.Key_SetCatcher( KEYCATCH_UI );
			}
			else
			{
				Menus_ActivateByName("mainhud");
			}
		}
		else if (Q_stricmp(name, "closedatapad") == 0)
		{
			trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
			trap_Key_ClearStates();
			Cvar_Set( "cl_paused", "0" );
			Menus_CloseAll();
			Menus_ActivateByName("mainhud");

			Cvar_Set( "cg_updatedDataPadForcePower1", "0" );
			Cvar_Set( "cg_updatedDataPadForcePower2", "0" );
			Cvar_Set( "cg_updatedDataPadForcePower3", "0" );
			Cvar_Set( "cg_updatedDataPadObjective", "0" );
		}
		else if (Q_stricmp(name, "closesabermenu") == 0)
		{
			// if we're in the saber menu when creating a character, close this down
			if( !Cvar_VariableIntegerValue( "saber_menu" ) )
			{
				Menus_CloseByName( "saberMenu" );
				Menus_OpenByName( "characterMenu" );
			}
		}
		else if (Q_stricmp(name, "clearmouseover") == 0)
		{
			itemDef_t *item;
			menuDef_t *menu = Menu_GetFocused();

			if (menu)
			{
				const char *itemName;
				String_Parse(args, &itemName);
				item = (itemDef_s *) Menu_FindItemByName((menuDef_t *) menu, itemName);
				if (item)
				{
					item->window.flags &= ~WINDOW_MOUSEOVER;
				}
			}
		}
		else if (Q_stricmp(name, "setMovesListDefault") == 0)
		{
			uiInfo.movesTitleIndex = 2;
		}
		else if (Q_stricmp(name, "resetMovesDesc") == 0)
		{
			menuDef_t *menu = Menu_GetFocused();
			itemDef_t *item;

			if (menu)
			{
				item = (itemDef_s *) Menu_FindItemByName(menu, "item_desc");
				if (item)
				{
					listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;
					if( listPtr )
					{
						listPtr->cursorPos = 0;
						listPtr->startPos = 0;
					}
					item->cursorPos = 0;
				}
			}

		}
		else if (Q_stricmp(name, "resetMovesList") == 0)
		{
			menuDef_t *menu;
			menu = Menus_FindByName("datapadMovesMenu");
			//update saber models
			if (menu)
			{
				itemDef_t *item;
				item = (itemDef_s *) Menu_FindItemByName((menuDef_t *) menu, "character");
				if (item)
				{
					UI_SaberAttachToChar( item );
				}
			}

			Cvar_Set( "ui_move_desc", " " );
		}
//		else if (Q_stricmp(name, "setanisotropicmax") == 0)
//		{
//			r_ext_texture_filter_anisotropic->value;
//		}
		else if (Q_stricmp(name, "setMoveCharacter") == 0)
		{
			itemDef_t *item;
			menuDef_t *menu;
			modelDef_t *modelPtr;
			char skin[MAX_QPATH];

			UI_GetCharacterCvars();
			UI_GetSaberCvars();

			uiInfo.movesTitleIndex = 0;

			menu = Menus_FindByName("datapadMovesMenu");

			if (menu)
			{
				item = (itemDef_s *) Menu_FindItemByName((menuDef_t *) menu, "character");
				if (item)
				{
					modelPtr = (modelDef_t*)item->typeData;
					if (modelPtr)
					{
						uiInfo.movesBaseAnim = datapadMoveTitleBaseAnims[uiInfo.movesTitleIndex];
						ItemParse_model_g2anim_go( item, uiInfo.movesBaseAnim );

						uiInfo.moveAnimTime = 0 ;
						DC->g2hilev_SetAnim(&item->ghoul2[0], "model_root", modelPtr->g2anim, qtrue);
						Com_sprintf( skin, sizeof( skin ), "models/players/%s/|%s|%s|%s",
															Cvar_VariableString ( "g_char_model"),
															Cvar_VariableString ( "g_char_skin_head"),
															Cvar_VariableString ( "g_char_skin_torso"),
															Cvar_VariableString ( "g_char_skin_legs")
									);

						ItemParse_model_g2skin_go( item, skin );
						UI_SaberAttachToChar( item );
					}
				}
			}
		}
		else if (Q_stricmp(name, "glCustom") == 0)
		{
			Cvar_Set("ui_r_glCustom", "4");
		}
		else if (Q_stricmp(name, "character") == 0)
		{
			UI_UpdateCharacter( qfalse );
		}
		else if (Q_stricmp(name, "characterchanged") == 0)
		{
			UI_UpdateCharacter( qtrue );
		}
		else if (Q_stricmp(name, "char_skin") == 0)
		{
			UI_UpdateCharacterSkin();
		}
		else if (Q_stricmp(name, "saber_type") == 0)
		{
			UI_UpdateSaberType();
		}
		else if (Q_stricmp(name, "saber_hilt") == 0)
		{
			UI_UpdateSaberHilt( qfalse );
		}
		else if (Q_stricmp(name, "saber_color") == 0)
		{
//			UI_UpdateSaberColor( qfalse );
		}
		else if (Q_stricmp(name, "saber2_hilt") == 0)
		{
			UI_UpdateSaberHilt( qtrue );
		}
		else if (Q_stricmp(name, "saber2_color") == 0)
		{
//			UI_UpdateSaberColor( qtrue );
		}
		else if (Q_stricmp(name, "updatecharcvars") == 0)
		{
			UI_UpdateCharacterCvars();
		}
		else if (Q_stricmp(name, "getcharcvars") == 0)
		{
			UI_GetCharacterCvars();
		}
		else if (Q_stricmp(name, "updatesabercvars") == 0)
		{
			UI_UpdateSaberCvars();
		}
		else if (Q_stricmp(name, "getsabercvars") == 0)
		{
			UI_GetSaberCvars();
		}
		else if (Q_stricmp(name, "resetsabercvardefaults") == 0)
		{
			// NOTE : ONLY do this if saber menu is set properly (ie. first time we enter this menu)
			if( !Cvar_VariableIntegerValue( "saber_menu" ) )
			{
				UI_ResetSaberCvars();
			}
    	}
#ifndef JK2_MODE
		else if (Q_stricmp(name, "updatefightingstylechoices") == 0)
		{
			UI_UpdateFightingStyleChoices();
		}
#endif // !JK2_MODE
		else if (Q_stricmp(name, "initallocforcepower") == 0)
		{
			const char *forceName;
			String_Parse(args, &forceName);

			UI_InitAllocForcePowers(forceName);
		}
		else if (Q_stricmp(name, "affectforcepowerlevel") == 0)
		{
			const char *forceName;
			String_Parse(args, &forceName);

			UI_AffectForcePowerLevel(forceName);
		}
		else if (Q_stricmp(name, "decrementcurrentforcepower") == 0)
		{
			UI_DecrementCurrentForcePower();
		}
		else if (Q_stricmp(name, "shutdownforcehelp") == 0)
		{
			UI_ShutdownForceHelp();
		}
		else if (Q_stricmp(name, "forcehelpactive") == 0)
		{
			UI_ForceHelpActive();
		}
#ifndef JK2_MODE
		else if (Q_stricmp(name, "demosetforcelevels") == 0)
		{
			UI_DemoSetForceLevels();
		}
#endif // !JK2_MODE
		else if (Q_stricmp(name, "recordforcelevels") == 0)
		{
			UI_RecordForceLevels();
		}
		else if (Q_stricmp(name, "recordweapons") == 0)
		{
			UI_RecordWeapons();
		}
		else if (Q_stricmp(name, "showforceleveldesc") == 0)
		{
			const char *forceName;
			String_Parse(args, &forceName);

			UI_ShowForceLevelDesc(forceName);
		}
		else if (Q_stricmp(name, "resetforcelevels") == 0)
		{
			UI_ResetForceLevels();
		}
		else if (Q_stricmp(name, "weaponhelpactive") == 0)
		{
			UI_WeaponHelpActive();
		}
		// initialize weapon selection screen
		else if (Q_stricmp(name, "initweaponselect") == 0)
		{
			UI_InitWeaponSelect();
		}
		else if (Q_stricmp(name, "clearweapons") == 0)
		{
			UI_ClearWeapons();
		}
		else if (Q_stricmp(name, "stopgamesounds") == 0)
		{
			trap_S_StopSounds();
		}
		else if (Q_stricmp(name, "loadmissionselectmenu") == 0)
		{
			const char *cvarName;
			String_Parse(args, &cvarName);

			if (cvarName)
			{
				UI_LoadMissionSelectMenu(cvarName);
			}
		}
#ifndef JK2_MODE
		else if (Q_stricmp(name, "calcforcestatus") == 0)
		{
			UI_CalcForceStatus();
		}
#endif // !JK2_MODE
		else if (Q_stricmp(name, "giveweapon") == 0)
		{
			const char *weaponIndex;
			String_Parse(args, &weaponIndex);
			UI_GiveWeapon(atoi(weaponIndex));
		}
		else if (Q_stricmp(name, "equipweapon") == 0)
		{
			const char *weaponIndex;
			String_Parse(args, &weaponIndex);
			UI_EquipWeapon(atoi(weaponIndex));
		}
		else if (Q_stricmp(name, "addweaponselection") == 0)
		{
			const char *weaponIndex;
			String_Parse(args, &weaponIndex);
			if (!weaponIndex)
			{
				return qfalse;
			}

			const char *ammoIndex;
			String_Parse(args, &ammoIndex);
			if (!ammoIndex)
			{
				return qfalse;
			}

			const char *ammoAmount;
			String_Parse(args, &ammoAmount);
			if (!ammoAmount)
			{
				return qfalse;
			}

			const char *itemName;
			String_Parse(args, &itemName);
			if (!itemName)
			{
				return qfalse;
			}

			const char *litItemName;
			String_Parse(args, &litItemName);
			if (!litItemName)
			{
				return qfalse;
			}

			const char *backgroundName;
			String_Parse(args, &backgroundName);
			if (!backgroundName)
			{
				return qfalse;
			}

			const char *soundfile = NULL;
			String_Parse(args, &soundfile);

			UI_AddWeaponSelection(atoi(weaponIndex),atoi(ammoIndex),atoi(ammoAmount),itemName,litItemName, backgroundName, soundfile);
		}
		else if (Q_stricmp(name, "addthrowweaponselection") == 0)
		{
			const char *weaponIndex;
			String_Parse(args, &weaponIndex);
			if (!weaponIndex)
			{
				return qfalse;
			}

			const char *ammoIndex;
			String_Parse(args, &ammoIndex);
			if (!ammoIndex)
			{
				return qfalse;
			}

			const char *ammoAmount;
			String_Parse(args, &ammoAmount);
			if (!ammoAmount)
			{
				return qfalse;
			}

			const char *itemName;
			String_Parse(args, &itemName);
			if (!itemName)
			{
				return qfalse;
			}

			const char *litItemName;
			String_Parse(args, &litItemName);
			if (!litItemName)
			{
				return qfalse;
			}

			const char *backgroundName;
			String_Parse(args, &backgroundName);
			if (!backgroundName)
			{
				return qfalse;
			}

			const char *soundfile;
			String_Parse(args, &soundfile);

			UI_AddThrowWeaponSelection(atoi(weaponIndex),atoi(ammoIndex),atoi(ammoAmount),itemName,litItemName,backgroundName, soundfile);
		}
		else if (Q_stricmp(name, "removeweaponselection") == 0)
		{
			const char *weaponIndex;
			String_Parse(args, &weaponIndex);
			if (weaponIndex)
			{
				UI_RemoveWeaponSelection(atoi(weaponIndex));
			}
		}
		else if (Q_stricmp(name, "removethrowweaponselection") == 0)
		{
			UI_RemoveThrowWeaponSelection();
		}
		else if (Q_stricmp(name, "normalthrowselection") == 0)
		{
			UI_NormalThrowSelection();
		}
		else if (Q_stricmp(name, "highlightthrowselection") == 0)
		{
			UI_HighLightThrowSelection();
		}
		else if (Q_stricmp(name, "normalweaponselection") == 0)
		{
			const char *slotIndex;
			String_Parse(args, &slotIndex);
			if (!slotIndex)
			{
				return qfalse;
			}

			UI_NormalWeaponSelection(atoi(slotIndex));
		}
		else if (Q_stricmp(name, "highlightweaponselection") == 0)
		{
			const char *slotIndex;
			String_Parse(args, &slotIndex);
			if (!slotIndex)
			{
				return qfalse;
			}

			UI_HighLightWeaponSelection(atoi(slotIndex));
		}
		else if (Q_stricmp(name, "clearinventory") == 0)
		{
			UI_ClearInventory();
		}
		else if (Q_stricmp(name, "giveinventory") == 0)
		{
			const char *inventoryIndex,*amount;
			String_Parse(args, &inventoryIndex);
			String_Parse(args, &amount);
			UI_GiveInventory(atoi(inventoryIndex),atoi(amount));
		}
#ifndef JK2_MODE
		else if (Q_stricmp(name, "updatefightingstyle") == 0)
		{
			UI_UpdateFightingStyle();
		}
#endif // !JK2_MODE
		else if (Q_stricmp(name, "update") == 0)
		{
			if (String_Parse(args, &name2))
			{
				UI_Update(name2);
			}
			else
			{
				Com_Printf("update missing cmd\n");
			}
		}
		else if (Q_stricmp(name, "load_quick") == 0)
		{
#ifdef JK2_MODE
			ui.Cmd_ExecuteText(EXEC_APPEND,"load quik\n");
#else
			ui.Cmd_ExecuteText(EXEC_APPEND,"load quick\n");
#endif
		}
		else if (Q_stricmp(name, "load_auto") == 0)
		{
			ui.Cmd_ExecuteText(EXEC_APPEND,"load *respawn\n");	//death menu, might load a saved game instead if they just loaded on this map
		}
		else if (Q_stricmp(name, "decrementforcepowerlevel") == 0)
		{
			UI_DecrementForcePowerLevel();
		}
		else if (Q_stricmp(name, "getmousepitch") == 0)
		{
			Cvar_Set("ui_mousePitch", (trap_Cvar_VariableValue("m_pitch") >= 0) ? "0" : "1");
		}
		else if (Q_stricmp(name, "resetcharacterlistboxes") == 0)
		{
			UI_ResetCharacterListBoxes();
		}
		else if ( Q_stricmp( name, "LaunchMP" ) == 0 )
		{
			// TODO for MAC_PORT, will only be valid for non-JK2 mode
		}
		else
		{
			Com_Printf("unknown UI script %s\n", name);
		}
	}

	return qtrue;
}

/*
=================
UI_GetValue
=================
*/
static float UI_GetValue(int ownerDraw)
{
  return 0;
}

//Force Warnings
enum
{
	FW_VERY_LIGHT = 0,
	FW_SEMI_LIGHT,
	FW_NEUTRAL,
	FW_SEMI_DARK,
	FW_VERY_DARK
};

const char *lukeForceStatusSounds[] =
{
"sound/chars/luke/misc/MLUK_03.mp3",	// Very Light
"sound/chars/luke/misc/MLUK_04.mp3",	// Semi Light
"sound/chars/luke/misc/MLUK_05.mp3",	// Neutral
"sound/chars/luke/misc/MLUK_01.mp3",	// Semi dark
"sound/chars/luke/misc/MLUK_02.mp3",	// Very dark
};

const char *kyleForceStatusSounds[] =
{
"sound/chars/kyle/misc/MKYK_05.mp3",	// Very Light
"sound/chars/kyle/misc/MKYK_04.mp3",	// Semi Light
"sound/chars/kyle/misc/MKYK_03.mp3",	// Neutral
"sound/chars/kyle/misc/MKYK_01.mp3",	// Semi dark
"sound/chars/kyle/misc/MKYK_02.mp3",	// Very dark
};


#ifndef JK2_MODE
static void UI_CalcForceStatus(void)
{
	float		lightSide,darkSide,total;
	short		who, index=FW_VERY_LIGHT;
	qboolean	lukeFlag=qtrue;
	float		percent;
	client_t*	cl = &svs.clients[0];	// 0 because only ever us as a player
	char		value[256];

	if (!cl)
	{
		return;
	}
	playerState_t*		pState = cl->gentity->client;

	if (!cl->gentity || !cl->gentity->client)
	{
		return;
	}

	memset(value, 0, sizeof(value));

	lightSide = pState->forcePowerLevel[FP_HEAL] +
		pState->forcePowerLevel[FP_TELEPATHY] +
		pState->forcePowerLevel[FP_PROTECT] +
		pState->forcePowerLevel[FP_ABSORB];

	darkSide = pState->forcePowerLevel[FP_GRIP] +
		pState->forcePowerLevel[FP_LIGHTNING] +
		pState->forcePowerLevel[FP_RAGE] +
		pState->forcePowerLevel[FP_DRAIN];

	total = lightSide + darkSide;

	percent = lightSide / total;

	who = Q_irand( 0, 100 );
	if (percent >= 0.90f)	//  90 - 100%
	{
		index = FW_VERY_LIGHT;
		if (who <50)
		{
			strcpy(value,"vlk");	// Very light Kyle
			lukeFlag = qfalse;
		}
		else
		{
			strcpy(value,"vll");	// Very light Luke
		}

	}
	else if (percent > 0.60f )
	{
		index = FW_SEMI_LIGHT;
		if ( who<50 )
		{
			strcpy(value,"slk");	// Semi-light Kyle
			lukeFlag = qfalse;
		}
		else
		{
			strcpy(value,"sll");	// Semi-light light Luke
		}
	}
	else if (percent > 0.40f )
	{
		index = FW_NEUTRAL;
		if ( who<50 )
		{
			strcpy(value,"ntk");	// Neutral Kyle
			lukeFlag = qfalse;
		}
		else
		{
			strcpy(value,"ntl");	// Netural Luke
		}
	}
	else if (percent > 0.10f )
	{
		index = FW_SEMI_DARK;
		if ( who<50 )
		{
			strcpy(value,"sdk");	// Semi-dark Kyle
			lukeFlag = qfalse;
		}
		else
		{
			strcpy(value,"sdl");	// Semi-Dark Luke
		}
	}
	else
	{
		index = FW_VERY_DARK;
		if ( who<50 )
		{
			strcpy(value,"vdk");	// Very dark Kyle
			lukeFlag = qfalse;
		}
		else
		{
			strcpy(value,"vdl");	// Very Dark Luke
		}
	}

	Cvar_Set("ui_forcestatus", value );

	if (lukeFlag)
	{
		DC->startLocalSound(DC->registerSound(lukeForceStatusSounds[index], qfalse), CHAN_VOICE );
	}
	else
	{
		DC->startLocalSound(DC->registerSound(kyleForceStatusSounds[index], qfalse), CHAN_VOICE );
	}
}
#endif // !JK2_MODE

/*
=================
UI_StopCinematic
=================
*/
static void UI_StopCinematic(int handle)
{
	if (handle >= 0)
	{
		trap_CIN_StopCinematic(handle);
	}
	else
	{
		handle = abs(handle);
		if (handle == UI_MAPCINEMATIC)
		{
			// FIXME - BOB do we need this?
//			if (uiInfo.mapList[ui_currentMap.integer].cinematic >= 0)
//			{
//				trap_CIN_StopCinematic(uiInfo.mapList[ui_currentMap.integer].cinematic);
//				uiInfo.mapList[ui_currentMap.integer].cinematic = -1;
//			}
		}
		else if (handle == UI_NETMAPCINEMATIC)
		{
			// FIXME - BOB do we need this?
//			if (uiInfo.serverStatus.currentServerCinematic >= 0)
//			{
//				trap_CIN_StopCinematic(uiInfo.serverStatus.currentServerCinematic);
//				uiInfo.serverStatus.currentServerCinematic = -1;
//			}
		}
		else if (handle == UI_CLANCINEMATIC)
		{
			// FIXME - BOB do we need this?
//			int i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_teamName"));
//			if (i >= 0 && i < uiInfo.teamCount)
//			{
//				if (uiInfo.teamList[i].cinematic >= 0)
//				{
//					trap_CIN_StopCinematic(uiInfo.teamList[i].cinematic);
//					uiInfo.teamList[i].cinematic = -1;
//				}
//			}
		}
	}
}
static void UI_HandleLoadSelection()
{
	Cvar_Set("ui_SelectionOK", va("%d",(s_savegame.currentLine < s_savegame.saveFileCnt)) );
	if (s_savegame.currentLine >= s_savegame.saveFileCnt)
		return;
#ifdef JK2_MODE
	Cvar_Set("ui_gameDesc", s_savedata[s_savegame.currentLine].currentSaveFileComments );	// set comment

	if (!ui.SG_GetSaveImage(s_savedata[s_savegame.currentLine].currentSaveFileName, &screenShotBuf))
	{
		memset( screenShotBuf,0,(SG_SCR_WIDTH * SG_SCR_HEIGHT * 4));
	}
#endif
}

/*
=================
UI_FeederCount
=================
*/
static int UI_FeederCount(float feederID)
{
	if (feederID == FEEDER_SAVEGAMES )
	{
		if (s_savegame.saveFileCnt == -1)
		{
			ReadSaveDirectory();	//refresh
			UI_HandleLoadSelection();
#ifndef JK2_MODE
			UI_AdjustSaveGameListBox(s_savegame.currentLine);
#endif
		}
		return s_savegame.saveFileCnt;
	}
	// count number of moves for the current title
	else if (feederID == FEEDER_MOVES)
	{
		int count=0,i;

		for (i=0;i<MAX_MOVES;i++)
		{
			if (datapadMoveData[uiInfo.movesTitleIndex][i].title)
			{
				count++;
			}
		}

		return count;
	}
	else if (feederID == FEEDER_MOVES_TITLES)
	{
		return (MD_MOVE_TITLE_MAX);
	}
	else if (feederID == FEEDER_MODS)
	{
		return uiInfo.modCount;
	}
	else if (feederID == FEEDER_LANGUAGES)
	{
		return uiInfo.languageCount;
	}
	else if (feederID == FEEDER_PLAYER_SPECIES)
	{
		return uiInfo.playerSpeciesCount;
	}
	else if (feederID == FEEDER_PLAYER_SKIN_HEAD)
	{
		return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHeadCount;
	}
	else if (feederID == FEEDER_PLAYER_SKIN_TORSO)
	{
		return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorsoCount;
	}
	else if (feederID == FEEDER_PLAYER_SKIN_LEGS)
	{
		return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLegCount;
	}
	else if (feederID == FEEDER_COLORCHOICES)
	{
		return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].ColorCount;
	}

	return 0;
}

/*
=================
UI_FeederSelection
=================
*/
static void UI_FeederSelection(float feederID, int index, itemDef_t *item)
{
	if (feederID == FEEDER_SAVEGAMES)
	{
		s_savegame.currentLine = index;
		UI_HandleLoadSelection();
	}
	else if (feederID == FEEDER_MOVES)
	{
		itemDef_t *item;
		menuDef_t *menu;
		modelDef_t *modelPtr;
		char skin[MAX_QPATH];

		menu = Menus_FindByName("datapadMovesMenu");

		if (menu)
		{
			item = (itemDef_s *) Menu_FindItemByName((menuDef_t *) menu, "character");
			if (item)
			{
				modelPtr = (modelDef_t*)item->typeData;
				if (modelPtr)
				{
					if (datapadMoveData[uiInfo.movesTitleIndex][index].anim)
					{
						ItemParse_model_g2anim_go( item, datapadMoveData[uiInfo.movesTitleIndex][index].anim );
						uiInfo.moveAnimTime = DC->g2hilev_SetAnim(&item->ghoul2[0], "model_root", modelPtr->g2anim, qtrue);

						uiInfo.moveAnimTime += uiInfo.uiDC.realTime;

						// Play sound for anim
						if (datapadMoveData[uiInfo.movesTitleIndex][index].sound == MDS_FORCE_JUMP)
						{
							DC->startLocalSound(uiInfo.uiDC.Assets.datapadmoveJumpSound, CHAN_LOCAL );
						}
						else if (datapadMoveData[uiInfo.movesTitleIndex][index].sound == MDS_ROLL)
						{
							DC->startLocalSound(uiInfo.uiDC.Assets.datapadmoveRollSound, CHAN_LOCAL );
						}
						else if (datapadMoveData[uiInfo.movesTitleIndex][index].sound == MDS_SABER)
						{
							// Randomly choose one sound
							int soundI = Q_irand( 1, 6 );
							sfxHandle_t *soundPtr;
							soundPtr = &uiInfo.uiDC.Assets.datapadmoveSaberSound1;
							if (soundI == 2)
							{
								soundPtr = &uiInfo.uiDC.Assets.datapadmoveSaberSound2;
							}
							else if (soundI == 3)
							{
								soundPtr = &uiInfo.uiDC.Assets.datapadmoveSaberSound3;
							}
							else if (soundI == 4)
							{
								soundPtr = &uiInfo.uiDC.Assets.datapadmoveSaberSound4;
							}
							else if (soundI == 5)
							{
								soundPtr = &uiInfo.uiDC.Assets.datapadmoveSaberSound5;
							}
							else if (soundI == 6)
							{
								soundPtr = &uiInfo.uiDC.Assets.datapadmoveSaberSound6;
							}

							DC->startLocalSound(*soundPtr, CHAN_LOCAL );
						}

						if (datapadMoveData[uiInfo.movesTitleIndex][index].desc)
						{
							Cvar_Set( "ui_move_desc", datapadMoveData[uiInfo.movesTitleIndex][index].desc);
						}

						Com_sprintf( skin, sizeof( skin ), "models/players/%s/|%s|%s|%s",
															Cvar_VariableString ( "g_char_model"),
															Cvar_VariableString ( "g_char_skin_head"),
															Cvar_VariableString ( "g_char_skin_torso"),
															Cvar_VariableString ( "g_char_skin_legs")
									);

						ItemParse_model_g2skin_go( item, skin );

					}
				}
			}
		}
	}
	else if (feederID == FEEDER_MOVES_TITLES)
	{
		itemDef_t *item;
		menuDef_t *menu;
		modelDef_t *modelPtr;

		uiInfo.movesTitleIndex = index;
		uiInfo.movesBaseAnim = datapadMoveTitleBaseAnims[uiInfo.movesTitleIndex];
		menu = Menus_FindByName("datapadMovesMenu");

		if (menu)
		{
			item = (itemDef_s *) Menu_FindItemByName((menuDef_t *) menu, "character");
			if (item)
			{
				modelPtr = (modelDef_t*)item->typeData;
				if (modelPtr)
				{
					ItemParse_model_g2anim_go( item, uiInfo.movesBaseAnim );
					uiInfo.moveAnimTime = DC->g2hilev_SetAnim(&item->ghoul2[0], "model_root", modelPtr->g2anim, qtrue);
				}
			}
		}
	}
	else if (feederID == FEEDER_MODS)
	{
		uiInfo.modIndex = index;
	}
	else if (feederID == FEEDER_PLAYER_SPECIES)
	{
		if (index >= 0 && index < uiInfo.playerSpeciesCount)
		{
			uiInfo.playerSpeciesIndex = index;
		}
	}
	else if (feederID == FEEDER_LANGUAGES)
	{
		uiInfo.languageCountIndex = index;
	}
	else if (feederID == FEEDER_PLAYER_SKIN_HEAD)
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHeadCount)
		{
			Cvar_Set("ui_char_skin_head", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHead[index].name);
		}
	}
	else if (feederID == FEEDER_PLAYER_SKIN_TORSO)
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorsoCount)
		{
			Cvar_Set("ui_char_skin_torso", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorso[index].name);
		}
	}
	else if (feederID == FEEDER_PLAYER_SKIN_LEGS)
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLegCount)
		{
			Cvar_Set("ui_char_skin_legs", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLeg[index].name);
		}
	}
	else if (feederID == FEEDER_COLORCHOICES)
	{
extern void	Item_RunScript(itemDef_t *item, const char *s);		//from ui_shared;
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].ColorCount)
		{
			Item_RunScript(item, uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Color[index].actionText);
		}
	}
/*	else if (feederID == FEEDER_CINEMATICS)
	{
		uiInfo.movieIndex = index;
		if (uiInfo.previewMovie >= 0)
		{
			trap_CIN_StopCinematic(uiInfo.previewMovie);
		}
		uiInfo.previewMovie = -1;
	}
	else if (feederID == FEEDER_DEMOS)
	{
		uiInfo.demoIndex = index;
	}
*/
}

void Key_KeynumToStringBuf( int keynum, char *buf, int buflen );
void Key_GetBindingBuf( int keynum, char *buf, int buflen );

static qboolean UI_Crosshair_HandleKey(int flags, float *special, int key)
{
  if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER)
  {
		if (key == A_MOUSE2)
		{
			uiInfo.currentCrosshair--;
		} else {
			uiInfo.currentCrosshair++;
		}

		if (uiInfo.currentCrosshair >= NUM_CROSSHAIRS) {
			uiInfo.currentCrosshair = 0;
		} else if (uiInfo.currentCrosshair < 0) {
			uiInfo.currentCrosshair = NUM_CROSSHAIRS - 1;
		}
		Cvar_Set("cg_drawCrosshair", va("%d", uiInfo.currentCrosshair));
		return qtrue;
	}
	return qfalse;
}


static qboolean UI_OwnerDrawHandleKey(int ownerDraw, int flags, float *special, int key)
{

	switch (ownerDraw)
	{
		case UI_CROSSHAIR:
			UI_Crosshair_HandleKey(flags, special, key);
			break;
		default:
			break;
	}

  return qfalse;
}

//unfortunately we cannot rely on any game/cgame module code to do our animation stuff,
//because the ui can be loaded while the game/cgame are not loaded. So we're going to recreate what we need here.
#undef MAX_ANIM_FILES
#define MAX_ANIM_FILES 4
class ui_animFileSet_t
{
public:
	char			filename[MAX_QPATH];
	animation_t		animations[MAX_ANIMATIONS];
}; // ui_animFileSet_t
static ui_animFileSet_t	ui_knownAnimFileSets[MAX_ANIM_FILES];

int				ui_numKnownAnimFileSets;

qboolean UI_ParseAnimationFile( const char *af_filename )
{
	const char		*text_p;
	int			len;
	int			i;
	const char		*token;
	float		fps;
	char		text[80000];
	int			animNum;
	animation_t	*animations = ui_knownAnimFileSets[ui_numKnownAnimFileSets].animations;

	len = re.GetAnimationCFG(af_filename, text, sizeof(text));
	if ( len <= 0 )
	{
		return qfalse;
	}
	if ( len >= (int)(sizeof( text ) - 1) )
	{
		Com_Error( ERR_FATAL, "UI_ParseAnimationFile: File %s too long\n (%d > %d)", af_filename, len, sizeof( text ) - 1);
		return qfalse;
	}

	// parse the text
	text_p = text;

	//FIXME: have some way of playing anims backwards... negative numFrames?

	//initialize anim array so that from 0 to MAX_ANIMATIONS, set default values of 0 1 0 100
	for(i = 0; i < MAX_ANIMATIONS; i++)
	{
		animations[i].firstFrame = 0;
		animations[i].numFrames = 0;
		animations[i].loopFrames = -1;
		animations[i].frameLerp = 100;
//		animations[i].initialLerp = 100;
	}

	// read information for each frame
	COM_BeginParseSession();
	while(1)
	{
		token = COM_Parse( &text_p );

		if ( !token || !token[0])
		{
			break;
		}

		animNum = GetIDForString(animTable, token);
		if(animNum == -1)
		{
//#ifndef FINAL_BUILD
#ifdef _DEBUG
			if (strcmp(token,"ROOT"))
			{
				Com_Printf(S_COLOR_RED"WARNING: Unknown token %s in %s\n", token, af_filename);
			}
#endif
			while (token[0])
			{
				token = COM_ParseExt( &text_p, qfalse );	//returns empty string when next token is EOL
			}
			continue;
		}

		token = COM_Parse( &text_p );
		if ( !token )
		{
			break;
		}
		animations[animNum].firstFrame = atoi( token );

		token = COM_Parse( &text_p );
		if ( !token )
		{
			break;
		}
		animations[animNum].numFrames = atoi( token );

		token = COM_Parse( &text_p );
		if ( !token )
		{
			break;
		}
		animations[animNum].loopFrames = atoi( token );

		token = COM_Parse( &text_p );
		if ( !token )
		{
			break;
		}
		fps = atof( token );
		if ( fps == 0 )
		{
			fps = 1;//Don't allow divide by zero error
		}
		if ( fps < 0 )
		{//backwards
			animations[animNum].frameLerp = floor(1000.0f / fps);
		}
		else
		{
			animations[animNum].frameLerp = ceil(1000.0f / fps);
		}

//		animations[animNum].initialLerp = ceil(1000.0f / fabs(fps));
	}
	COM_EndParseSession();

	return qtrue;
}

qboolean UI_ParseAnimFileSet( const char *animCFG, int *animFileIndex )
{ //Not going to bother parsing the sound config here.
	char		afilename[MAX_QPATH];
	char		strippedName[MAX_QPATH];
	int			i;
	char		*slash;

	Q_strncpyz( strippedName, animCFG, sizeof(strippedName));
	slash = strrchr( strippedName, '/' );
	if ( slash )
	{
		// truncate modelName to find just the dir not the extension
		*slash = 0;
	}

	//if this anims file was loaded before, don't parse it again, just point to the correct table of info
	for ( i = 0; i < ui_numKnownAnimFileSets; i++ )
	{
		if ( Q_stricmp(ui_knownAnimFileSets[i].filename, strippedName ) == 0 )
		{
			*animFileIndex = i;
			return qtrue;
		}
	}

	if ( ui_numKnownAnimFileSets == MAX_ANIM_FILES )
	{//TOO MANY!
		for (i = 0; i < MAX_ANIM_FILES; i++)
		{
			Com_Printf("animfile[%d]: %s\n", i, ui_knownAnimFileSets[i].filename);
		}
		Com_Error( ERR_FATAL, "UI_ParseAnimFileSet: %d == MAX_ANIM_FILES == %d", ui_numKnownAnimFileSets, MAX_ANIM_FILES);
	}

	//Okay, time to parse in a new one
	Q_strncpyz( ui_knownAnimFileSets[ui_numKnownAnimFileSets].filename, strippedName, sizeof( ui_knownAnimFileSets[ui_numKnownAnimFileSets].filename ) );

	// Load and parse animations.cfg file
	Com_sprintf( afilename, sizeof( afilename ), "%s/animation.cfg", strippedName );
	if ( !UI_ParseAnimationFile( afilename ) )
	{
		*animFileIndex = -1;
		return qfalse;
	}

	//set index and increment
	*animFileIndex = ui_numKnownAnimFileSets++;

	return qtrue;
}

int UI_G2SetAnim(CGhoul2Info *ghlInfo, const char *boneName, int animNum, const qboolean freeze)
{
	int animIndex,blendTime;
	char *GLAName;

	GLAName = re.G2API_GetGLAName(ghlInfo);

	if (!GLAName || !GLAName[0])
	{
		return 0;
	}

	UI_ParseAnimFileSet(GLAName, &animIndex);

	if (animIndex != -1)
	{
		animation_t *anim = &ui_knownAnimFileSets[animIndex].animations[animNum];
		if (anim->numFrames <= 0)
		{
			return 0;
		}
		int sFrame = anim->firstFrame;
		int eFrame = anim->firstFrame + anim->numFrames;
		int flags = BONE_ANIM_OVERRIDE;
		int time = uiInfo.uiDC.realTime;
		float animSpeed = (50.0f / anim->frameLerp);

		blendTime = 150;

		// Freeze anim if it's not looping, special hack for datapad moves menu
		if (freeze)
		{
			if (anim->loopFrames == -1)
			{
				flags = BONE_ANIM_OVERRIDE_FREEZE;
			}
			else
			{
				flags = BONE_ANIM_OVERRIDE_LOOP;
			}
		}
		else if (anim->loopFrames != -1)
		{
			flags = BONE_ANIM_OVERRIDE_LOOP;
		}
		flags |= BONE_ANIM_BLEND;
		blendTime = 150;


		re.G2API_SetBoneAnim(ghlInfo, boneName, sFrame, eFrame, flags, animSpeed, time, -1, blendTime);

		return ((anim->frameLerp * (anim->numFrames-2)));
	}

	return 0;
}

static qboolean UI_ParseColorData(char* buf, playerSpeciesInfo_t &species)
{
	const char	*token;
	const char	*p;

	p = buf;
	COM_BeginParseSession();
	species.ColorCount = 0;
	species.ColorMax = 16;
	species.Color = (playerColor_t *)malloc(species.ColorMax * sizeof(playerColor_t));

	while ( p )
	{
		token = COM_ParseExt( &p, qtrue );	//looking for the shader
		if ( token[0] == 0 )
		{
			COM_EndParseSession(  );
			return (qboolean)(species.ColorCount != 0);
		}

		if (species.ColorCount >= species.ColorMax)
		{
			species.ColorMax *= 2;
			species.Color = (playerColor_t *)realloc(species.Color, species.ColorMax * sizeof(playerColor_t));
		}

		memset(&species.Color[species.ColorCount], 0, sizeof(playerColor_t));

		Q_strncpyz( species.Color[species.ColorCount].shader, token, MAX_QPATH );

		token = COM_ParseExt( &p, qtrue );	//looking for action block {
		if ( token[0] != '{' )
		{
			COM_EndParseSession(  );
			return qfalse;
		}

		token = COM_ParseExt( &p, qtrue );	//looking for action commands
		while (token[0] != '}')
		{
			if ( token[0] == 0)
			{	//EOF
				COM_EndParseSession(  );
				return qfalse;
			}
			Q_strcat(species.Color[species.ColorCount].actionText, ACTION_BUFFER_SIZE, token);
			Q_strcat(species.Color[species.ColorCount].actionText, ACTION_BUFFER_SIZE, " ");
			token = COM_ParseExt( &p, qtrue );	//looking for action commands or final }
		}
		species.ColorCount++;	//next color please
	}
	COM_EndParseSession(  );
	return qtrue;//never get here
}

/*
=================
bIsImageFile
builds path and scans for valid image extentions
=================
*/
static qboolean IsImageFile(const char* dirptr, const char* skinname, qboolean building)
{
	char fpath[MAX_QPATH];
	int f;


	Com_sprintf(fpath, MAX_QPATH, "models/players/%s/icon_%s.jpg", dirptr, skinname);
	ui.FS_FOpenFile(fpath, &f, FS_READ);
	if (!f)
	{ //not there, try png
		Com_sprintf(fpath, MAX_QPATH, "models/players/%s/icon_%s.png", dirptr, skinname);
		ui.FS_FOpenFile(fpath, &f, FS_READ);
	}
	if (!f)
	{ //not there, try tga
		Com_sprintf(fpath, MAX_QPATH, "models/players/%s/icon_%s.tga", dirptr, skinname);
		ui.FS_FOpenFile(fpath, &f, FS_READ);
	}
	if (f)
	{
		ui.FS_FCloseFile(f);
		if ( building ) ui.R_RegisterShaderNoMip(fpath);
		return qtrue;
	}

	return qfalse;
}

static void UI_FreeSpecies( playerSpeciesInfo_t *species )
{
	free(species->SkinHead);
	free(species->SkinTorso);
	free(species->SkinLeg);
	free(species->Color);
	memset(species, 0, sizeof(playerSpeciesInfo_t));
}

void UI_FreeAllSpecies( void )
{
	int i;

	for (i = 0; i < uiInfo.playerSpeciesCount; i++)
	{
		UI_FreeSpecies(&uiInfo.playerSpecies[i]);
	}
	free(uiInfo.playerSpecies);

	uiInfo.playerSpeciesCount = 0;
	uiInfo.playerSpecies = NULL;
}

/*
=================
PlayerModel_BuildList
=================
*/
static void UI_BuildPlayerModel_List( qboolean inGameLoad )
{
	static const size_t DIR_LIST_SIZE = 16384;

	int		numdirs;
	size_t	dirListSize = DIR_LIST_SIZE;
	char	stackDirList[8192];
	char	*dirlist;
	char*	dirptr;
	int		dirlen;
	int		i;
	const int building = Cvar_VariableIntegerValue("com_buildscript");

	dirlist = (char *)malloc(DIR_LIST_SIZE);
	if ( !dirlist )
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: Failed to allocate %u bytes of memory for player model "
			"directory list. Using stack allocated buffer of %u bytes instead.",
			DIR_LIST_SIZE, sizeof(stackDirList));

		dirlist = stackDirList;
		dirListSize = sizeof(stackDirList);
	}

	uiInfo.playerSpeciesCount = 0;
	uiInfo.playerSpeciesIndex = 0;
	uiInfo.playerSpeciesMax = 8;
	uiInfo.playerSpecies = (playerSpeciesInfo_t *)malloc(uiInfo.playerSpeciesMax * sizeof(playerSpeciesInfo_t));

	// iterate directory of all player models
	numdirs = ui.FS_GetFileList("models/players", "/", dirlist, dirListSize );
	dirptr  = dirlist;
	for (i=0; i<numdirs; i++,dirptr+=dirlen+1)
	{
		char*	fileptr;
		int		filelen;
		int f = 0;
		char fpath[MAX_QPATH];

		dirlen = strlen(dirptr);

		if (dirlen)
		{
			if (dirptr[dirlen-1]=='/')
				dirptr[dirlen-1]='\0';
		}
		else
		{
			continue;
		}

		if (!strcmp(dirptr,".") || !strcmp(dirptr,".."))
			continue;

		Com_sprintf(fpath, sizeof(fpath), "models/players/%s/PlayerChoice.txt", dirptr);
		filelen = ui.FS_FOpenFile(fpath, &f, FS_READ);

		if (f)
		{
			char filelist[2048];
			playerSpeciesInfo_t *species = NULL;

			std::vector<char> buffer(filelen + 1);
			ui.FS_Read(&buffer[0], filelen, f);
			ui.FS_FCloseFile(f);

			buffer[filelen] = 0;

			//record this species
			if (uiInfo.playerSpeciesCount >= uiInfo.playerSpeciesMax)
			{
				uiInfo.playerSpeciesMax *= 2;
				uiInfo.playerSpecies = (playerSpeciesInfo_t *)realloc(uiInfo.playerSpecies, uiInfo.playerSpeciesMax*sizeof(playerSpeciesInfo_t));
			}
			species = &uiInfo.playerSpecies[uiInfo.playerSpeciesCount];
			memset(species, 0, sizeof(playerSpeciesInfo_t));
			Q_strncpyz( species->Name, dirptr, MAX_QPATH );

			if (!UI_ParseColorData(buffer.data(),*species))
			{
				ui.Printf( "UI_BuildPlayerModel_List: Errors parsing '%s'\n", fpath );
			}

			species->SkinHeadMax = 8;
			species->SkinTorsoMax = 8;
			species->SkinLegMax = 8;

			species->SkinHead = (skinName_t *)malloc(species->SkinHeadMax * sizeof(skinName_t));
			species->SkinTorso = (skinName_t *)malloc(species->SkinTorsoMax * sizeof(skinName_t));
			species->SkinLeg = (skinName_t *)malloc(species->SkinLegMax * sizeof(skinName_t));

			int		j;
			char	skinname[64];
			int		numfiles;
			int		iSkinParts=0;

			numfiles = ui.FS_GetFileList( va("models/players/%s",dirptr), ".skin", filelist, sizeof(filelist) );
			fileptr  = filelist;
			for (j=0; j<numfiles; j++,fileptr+=filelen+1)
			{
				if ( building )
				{
					ui.FS_FOpenFile(va("models/players/%s/%s",dirptr,fileptr), &f, FS_READ);
					if (f) ui.FS_FCloseFile(f);
					ui.FS_FOpenFile(va("models/players/%s/sounds.cfg", dirptr), &f, FS_READ);
					if (f) ui.FS_FCloseFile(f);
					ui.FS_FOpenFile(va("models/players/%s/animevents.cfg", dirptr), &f, FS_READ);
					if (f) ui.FS_FCloseFile(f);
				}

				filelen = strlen(fileptr);
				COM_StripExtension(fileptr,skinname, sizeof(skinname));

				if (IsImageFile(dirptr, skinname, (qboolean)(building != 0)))
				{ //if it exists
					if (Q_stricmpn(skinname,"head_",5) == 0)
					{
						if (species->SkinHeadCount >= species->SkinHeadMax)
						{
							species->SkinHeadMax *= 2;
							species->SkinHead = (skinName_t *)realloc(species->SkinHead, species->SkinHeadMax*sizeof(skinName_t));
						}
						Q_strncpyz(species->SkinHead[species->SkinHeadCount++].name, skinname, SKIN_LENGTH);
						iSkinParts |= 1<<0;
					} else
					if (Q_stricmpn(skinname,"torso_",6) == 0)
					{
						if (species->SkinTorsoCount >= species->SkinTorsoMax)
						{
							species->SkinTorsoMax *= 2;
							species->SkinTorso = (skinName_t *)realloc(species->SkinTorso, species->SkinTorsoMax*sizeof(skinName_t));
						}
						Q_strncpyz(species->SkinTorso[species->SkinTorsoCount++].name, skinname, SKIN_LENGTH);
						iSkinParts |= 1<<1;
					} else
					if (Q_stricmpn(skinname,"lower_",6) == 0)
					{
						if (species->SkinLegCount >= species->SkinLegMax)
						{
							species->SkinLegMax *= 2;
							species->SkinLeg = (skinName_t *)realloc(species->SkinLeg, species->SkinLegMax*sizeof(skinName_t));
						}
						Q_strncpyz(species->SkinLeg[species->SkinLegCount++].name, skinname, SKIN_LENGTH);
						iSkinParts |= 1<<2;
					}

				}
			}
			if (iSkinParts != 7)
			{	//didn't get a skin for each, then skip this model.
				UI_FreeSpecies(species);
				continue;
			}
			uiInfo.playerSpeciesCount++;
			if (!inGameLoad && ui_PrecacheModels.integer)
			{
				CGhoul2Info_v ghoul2;
				Com_sprintf( fpath, sizeof( fpath ), "models/players/%s/model.glm", dirptr );
				int g2Model = DC->g2_InitGhoul2Model(ghoul2, fpath, 0, 0, 0, 0, 0);
				if (g2Model >= 0)
				{
					DC->g2_RemoveGhoul2Model( ghoul2, 0 );
				}
			}
		}
	}

	if ( dirlist != stackDirList )
	{
		free(dirlist);
	}
}

/*
================
UI_Shutdown
=================
*/
void UI_Shutdown( void )
{
	UI_FreeAllSpecies();
}

/*
=================
UI_Init
=================
*/
void _UI_Init( qboolean inGameLoad )
{
	// Get the list of possible languages
#ifndef JK2_MODE
	uiInfo.languageCount = SE_GetNumLanguages();	// this does a dir scan, so use carefully
#else
	// sod it, parse every menu strip file until we find a gap in the sequence...
	//
	for (int i=0; i<10; i++)
	{
		if (!ui.SP_Register(va("menus%d",i), /*SP_REGISTER_REQUIRED|*/SP_REGISTER_MENU))
			break;
	}
#endif

	uiInfo.inGameLoad = inGameLoad;

	UI_RegisterCvars();

	UI_InitMemory();

	// cache redundant calulations
	trap_GetGlconfig( &uiInfo.uiDC.glconfig );

	// for 640x480 virtualized screen
	uiInfo.uiDC.yscale = uiInfo.uiDC.glconfig.vidHeight * (1.0/480.0);
	uiInfo.uiDC.xscale = uiInfo.uiDC.glconfig.vidWidth * (1.0/640.0);
	if ( uiInfo.uiDC.glconfig.vidWidth * 480 > uiInfo.uiDC.glconfig.vidHeight * 640 )
	{
		// wide screen
		uiInfo.uiDC.bias = 0.5 * ( uiInfo.uiDC.glconfig.vidWidth - ( uiInfo.uiDC.glconfig.vidHeight * (640.0/480.0) ) );
	}
	else
	{
		// no wide screen
		uiInfo.uiDC.bias = 0;
	}

	Init_Display(&uiInfo.uiDC);

	uiInfo.uiDC.drawText			= &Text_Paint;
	uiInfo.uiDC.drawHandlePic		= &UI_DrawHandlePic;
	uiInfo.uiDC.drawRect			= &_UI_DrawRect;
	uiInfo.uiDC.drawSides			= &_UI_DrawSides;
	uiInfo.uiDC.drawTextWithCursor	= &Text_PaintWithCursor;
	uiInfo.uiDC.executeText			= &Cbuf_ExecuteText;
	uiInfo.uiDC.drawTopBottom		= &_UI_DrawTopBottom;
	uiInfo.uiDC.feederCount			= &UI_FeederCount;
	uiInfo.uiDC.feederSelection		= &UI_FeederSelection;
	uiInfo.uiDC.fillRect			= &UI_FillRect;
	uiInfo.uiDC.getBindingBuf		= &Key_GetBindingBuf;
	uiInfo.uiDC.getCVarString		= Cvar_VariableStringBuffer;
	uiInfo.uiDC.getCVarValue		= trap_Cvar_VariableValue;
	uiInfo.uiDC.getOverstrikeMode	= &trap_Key_GetOverstrikeMode;
	uiInfo.uiDC.getValue			= &UI_GetValue;
	uiInfo.uiDC.keynumToStringBuf	= &Key_KeynumToStringBuf;
	uiInfo.uiDC.modelBounds			= &trap_R_ModelBounds;
	uiInfo.uiDC.ownerDrawVisible	= &UI_OwnerDrawVisible;
	uiInfo.uiDC.ownerDrawWidth		= &UI_OwnerDrawWidth;
	uiInfo.uiDC.ownerDrawItem		= &UI_OwnerDraw;
	uiInfo.uiDC.Print				= &Com_Printf;
	uiInfo.uiDC.registerSound		= &trap_S_RegisterSound;
	uiInfo.uiDC.registerModel		= ui.R_RegisterModel;
	uiInfo.uiDC.clearScene			= &trap_R_ClearScene;
	uiInfo.uiDC.addRefEntityToScene = &trap_R_AddRefEntityToScene;
	uiInfo.uiDC.renderScene			= &trap_R_RenderScene;
	uiInfo.uiDC.runScript			= &UI_RunMenuScript;
	uiInfo.uiDC.deferScript			= &UI_DeferMenuScript;
	uiInfo.uiDC.setBinding			= &trap_Key_SetBinding;
	uiInfo.uiDC.setColor			= &UI_SetColor;
	uiInfo.uiDC.setCVar				= Cvar_Set;
	uiInfo.uiDC.setOverstrikeMode	= &trap_Key_SetOverstrikeMode;
	uiInfo.uiDC.startLocalSound		= &trap_S_StartLocalSound;
	uiInfo.uiDC.stopCinematic		= &UI_StopCinematic;
	uiInfo.uiDC.textHeight			= &Text_Height;
	uiInfo.uiDC.textWidth			= &Text_Width;
	uiInfo.uiDC.feederItemImage		= &UI_FeederItemImage;
	uiInfo.uiDC.feederItemText		= &UI_FeederItemText;
	uiInfo.uiDC.ownerDrawHandleKey	= &UI_OwnerDrawHandleKey;

	uiInfo.uiDC.registerSkin		= re.RegisterSkin;

	uiInfo.uiDC.g2_SetSkin = re.G2API_SetSkin;
	uiInfo.uiDC.g2_SetBoneAnim = re.G2API_SetBoneAnim;
	uiInfo.uiDC.g2_RemoveGhoul2Model = re.G2API_RemoveGhoul2Model;
	uiInfo.uiDC.g2_InitGhoul2Model = re.G2API_InitGhoul2Model;
	uiInfo.uiDC.g2_CleanGhoul2Models = re.G2API_CleanGhoul2Models;
	uiInfo.uiDC.g2_AddBolt = re.G2API_AddBolt;
	uiInfo.uiDC.g2_GetBoltMatrix = re.G2API_GetBoltMatrix;
	uiInfo.uiDC.g2_GiveMeVectorFromMatrix = re.G2API_GiveMeVectorFromMatrix;

	uiInfo.uiDC.g2hilev_SetAnim = UI_G2SetAnim;

	UI_BuildPlayerModel_List(inGameLoad);

	String_Init();

	const char *menuSet = UI_Cvar_VariableString("ui_menuFiles");

	if (menuSet == NULL || menuSet[0] == '\0')
	{
		menuSet = "ui/menus.txt";
	}

#ifndef JK2_MODE
	if (inGameLoad)
	{
		UI_LoadMenus("ui/ingame.txt", qtrue);
	}
	else
#endif
	{
		UI_LoadMenus(menuSet, qtrue);
	}

	Menus_CloseAll();

	uiInfo.uiDC.whiteShader = ui.R_RegisterShaderNoMip( "white" );

	AssetCache();

	uis.debugMode = qfalse;

	// sets defaults for ui temp cvars
	uiInfo.effectsColor = (int)trap_Cvar_VariableValue("color")-1;
	if (uiInfo.effectsColor < 0)
	{
		uiInfo.effectsColor = 0;
	}
	uiInfo.effectsColor = gamecodetoui[uiInfo.effectsColor];
	uiInfo.currentCrosshair = (int)trap_Cvar_VariableValue("cg_drawCrosshair");
	Cvar_Set("ui_mousePitch", (trap_Cvar_VariableValue("m_pitch") >= 0) ? "0" : "1");

	Cvar_Set("cg_endcredits", "0");	// Reset value
	Cvar_Set("ui_missionfailed","0"); // reset

	uiInfo.forcePowerUpdated = FP_UPDATED_NONE;
	uiInfo.selectedWeapon1 = NOWEAPON;
	uiInfo.selectedWeapon2 = NOWEAPON;
	uiInfo.selectedThrowWeapon = NOWEAPON;

	uiInfo.uiDC.Assets.nullSound = trap_S_RegisterSound("sound/null", qfalse);

#ifndef JK2_MODE
	//FIXME hack to prevent error in jk2 by disabling
	trap_S_RegisterSound("sound/interface/weapon_deselect", qfalse);
#endif

}

/*
=================
UI_RegisterCvars
=================
*/
static void UI_RegisterCvars( void )
{
	size_t i = 0;
	const cvarTable_t *cv = NULL;

	for ( i=0, cv=cvarTable; i<cvarTableSize; i++, cv++ ) {
		Cvar_Register( cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags );
		if ( cv->update )
			cv->update();
	}
}

/*
=================
UI_ParseMenu
=================
*/
void UI_ParseMenu(const char *menuFile)
{
	char	*buffer,*holdBuffer,*token2;
	int len;
//	pc_token_t token;

	//Com_DPrintf("Parsing menu file: %s\n", menuFile);
	len = PC_StartParseSession(menuFile,&buffer);

	holdBuffer = buffer;

	if (len<=0)
	{
		Com_Printf("UI_ParseMenu: Unable to load menu %s\n", menuFile);
		return;
	}

	while ( 1 )
	{

		token2 = PC_ParseExt();

		if (!*token2)
		{
			break;
		}
/*
		if ( menuCount == MAX_MENUS )
		{
			PC_ParseWarning("Too many menus!");
			break;
		}
*/
		if ( *token2 == '{')
		{
			continue;
		}
		else if ( *token2 == '}' )
		{
			break;
		}
		else if (Q_stricmp(token2, "assetGlobalDef") == 0)
		{
			if (Asset_Parse(&holdBuffer))
			{
				continue;
			}
			else
			{
				break;
			}
		}
		else if (Q_stricmp(token2, "menudef") == 0)
		{
			// start a new menu
			Menu_New(holdBuffer);
			continue;
		}

		PC_ParseWarning(va("Invalid keyword '%s'",token2));
	}

	PC_EndParseSession(buffer);

}

/*
=================
Load_Menu
	Load current menu file
=================
*/
qboolean Load_Menu(const char **holdBuffer)
{
	const char	*token2;

	token2 = COM_ParseExt( holdBuffer, qtrue );

	if (!token2[0])
	{
		return qfalse;
	}

	if (*token2 != '{')
	{
		return qfalse;
	}

	while ( 1 )
	{
		token2 = COM_ParseExt( holdBuffer, qtrue );

		if ((!token2) || (token2 == 0))
		{
			return qfalse;
		}

		if ( *token2 == '}' )
		{
			return qtrue;
		}

//#ifdef _DEBUG
//		extern void UI_Debug_AddMenuFilePath(const char *);
//		UI_Debug_AddMenuFilePath(token2);
//#endif
		UI_ParseMenu(token2);

	}
	return qfalse;
}

/*
=================
UI_LoadMenus
	Load all menus based on the files listed in the data file in menuFile (default "ui/menus.txt")
=================
*/
void UI_LoadMenus(const char *menuFile, qboolean reset)
{
//	pc_token_t token;
//	int handle;
	int start;

	char *buffer;
	const char *holdBuffer;
	int len;

	start = Sys_Milliseconds();

	len = ui.FS_ReadFile(menuFile,(void **) &buffer);

	if (len<1)
	{
		Com_Printf( va( S_COLOR_YELLOW "menu file not found: %s, using default\n", menuFile ) );
		len = ui.FS_ReadFile("ui/menus.txt",(void **) &buffer);

		if (len<1)
		{
			Com_Error( ERR_FATAL, "%s", va("default menu file not found: ui/menus.txt, unable to continue!\n", menuFile ));
			return;
		}
	}

	if (reset)
	{
		Menu_Reset();
	}

	const char	*token2;
	holdBuffer = buffer;
	COM_BeginParseSession();
	while ( 1 )
	{
		token2 = COM_ParseExt( &holdBuffer, qtrue );
		if (!*token2)
		{
			break;
		}

		if( *token2 == 0 || *token2 == '}')			// End of the menus file
		{
			break;
		}

		if (*token2 == '{')
		{
				continue;
		}
		else if (Q_stricmp(token2, "loadmenu") == 0)
		{
			if (Load_Menu(&holdBuffer))
			{
				continue;
			}
			else
			{
				break;
			}
		}
		else
		{
			Com_Printf("Unknown keyword '%s' in menus file %s\n", token2, menuFile);
		}
	}
	COM_EndParseSession();

	Com_Printf("UI menu load time = %d milli seconds\n", Sys_Milliseconds() - start);

	ui.FS_FreeFile( buffer );	//let go of the buffer
}

/*
=================
UI_Load
=================
*/
void UI_Load(void)
{
	const char *menuSet;
	char lastName[1024];
	menuDef_t *menu = Menu_GetFocused();

	if (menu && menu->window.name)
	{
		strcpy(lastName, menu->window.name);
	}
	else
	{
		lastName[0] = 0;
	}

#ifndef JK2_MODE
	if (uiInfo.inGameLoad)
	{
		menuSet= "ui/ingame.txt";
	}
	else
#endif
	{
		menuSet= UI_Cvar_VariableString("ui_menuFiles");
	}
	if (menuSet == NULL || menuSet[0] == '\0')
	{
		menuSet = "ui/menus.txt";
	}

	String_Init();

	UI_LoadMenus(menuSet, qtrue);
	Menus_CloseAll();
	Menus_ActivateByName(lastName);
}

/*
=================
Asset_Parse
=================
*/
qboolean Asset_Parse(char **buffer)
{
	char		*token;
	const char	*tempStr;
	int			pointSize;

	token = PC_ParseExt();

	if (!token)
	{
		return qfalse;
	}

	if (*token != '{')
	{
		return qfalse;
	}

	while ( 1 )
	{
		token = PC_ParseExt();

		if (!token)
		{
			return qfalse;
		}

		if (*token == '}')
		{
			return qtrue;
		}

		// fonts
		if (Q_stricmp(token, "smallFont") == 0)		//legacy, really it only matters which order they are registered
		{
			if (PC_ParseString(&tempStr))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'smallFont'");
				return qfalse;
			}

			UI_RegisterFont(tempStr);

			//not used anymore
			if (PC_ParseInt(&pointSize))
			{
//				PC_ParseWarning("Bad 2nd parameter for keyword 'smallFont'");
			}

			continue;
		}

		if (Q_stricmp(token, "mediumFont") == 0)
		{
			if (PC_ParseString(&tempStr))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'font'");
				return qfalse;
			}

			uiInfo.uiDC.Assets.qhMediumFont = UI_RegisterFont(tempStr);
			uiInfo.uiDC.Assets.fontRegistered = qtrue;

			//not used
			if (PC_ParseInt(&pointSize))
			{
//				PC_ParseWarning("Bad 2nd parameter for keyword 'font'");
			}
			continue;
		}

		if (Q_stricmp(token, "bigFont") == 0) //legacy
		{
			if (PC_ParseString(&tempStr))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'bigFont'");
				return qfalse;
			}

			UI_RegisterFont(tempStr);

			if (PC_ParseInt(&pointSize))
			{
//				PC_ParseWarning("Bad 2nd parameter for keyword 'bigFont'");
			}

			continue;
		}

#ifdef JK2_MODE
		if (Q_stricmp(token, "stripedFile") == 0)
		{
			if (!PC_ParseStringMem((const char **) &tempStr))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'stripedFile'");
				return qfalse;
			}

			char sTemp[1024];
			Q_strncpyz( sTemp, tempStr,  sizeof(sTemp) );
			if (!ui.SP_Register(sTemp, /*SP_REGISTER_REQUIRED|*/SP_REGISTER_MENU))
			{
				PC_ParseWarning(va("(.SP file \"%s\" not found)",sTemp));
				//return qfalse;	// hmmm... dunno about this, don't want to break scripts for just missing subtitles
			}
			else
			{
//				extern void AddMenuPackageRetryKey(const char *);
//				AddMenuPackageRetryKey(sTemp);
			}

			continue;
		}
#endif

		// gradientbar
		if (Q_stricmp(token, "gradientbar") == 0)
		{
			if (PC_ParseString(&tempStr))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'gradientbar'");
				return qfalse;
			}
			uiInfo.uiDC.Assets.gradientBar = ui.R_RegisterShaderNoMip(tempStr);
			continue;
		}

		// enterMenuSound
		if (Q_stricmp(token, "menuEnterSound") == 0)
		{
			if (PC_ParseString(&tempStr))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'menuEnterSound'");
				return qfalse;
			}

			uiInfo.uiDC.Assets.menuEnterSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// exitMenuSound
		if (Q_stricmp(token, "menuExitSound") == 0)
		{
			if (PC_ParseString(&tempStr))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'menuExitSound'");
				return qfalse;
			}
			uiInfo.uiDC.Assets.menuExitSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// itemFocusSound
		if (Q_stricmp(token, "itemFocusSound") == 0)
		{
			if (PC_ParseString(&tempStr))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'itemFocusSound'");
				return qfalse;
			}
			uiInfo.uiDC.Assets.itemFocusSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// menuBuzzSound
		if (Q_stricmp(token, "menuBuzzSound") == 0)
		{
			if (PC_ParseString(&tempStr))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'menuBuzzSound'");
				return qfalse;
			}
			uiInfo.uiDC.Assets.menuBuzzSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// Chose a force power from the ingame force allocation screen (the one where you get to allocate a force power point)
		if (Q_stricmp(token, "forceChosenSound") == 0)
		{
			if (PC_ParseString(&tempStr))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'forceChosenSound'");
				return qfalse;
			}

			uiInfo.uiDC.Assets.forceChosenSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}


		// Unchose a force power from the ingame force allocation screen (the one where you get to allocate a force power point)
		if (Q_stricmp(token, "forceUnchosenSound") == 0)
		{
			if (PC_ParseString(&tempStr))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'forceUnchosenSound'");
				return qfalse;
			}

			uiInfo.uiDC.Assets.forceUnchosenSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		if (Q_stricmp(token, "datapadmoveRollSound") == 0)
		{
			if (PC_ParseString(&tempStr))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'datapadmoveRollSound'");
				return qfalse;
			}

			uiInfo.uiDC.Assets.datapadmoveRollSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		if (Q_stricmp(token, "datapadmoveJumpSound") == 0)
		{
			if (PC_ParseString(&tempStr))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'datapadmoveRoll'");
				return qfalse;
			}

			uiInfo.uiDC.Assets.datapadmoveJumpSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		if (Q_stricmp(token, "datapadmoveSaberSound1") == 0)
		{
			if (PC_ParseString(&tempStr))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'datapadmoveSaberSound1'");
				return qfalse;
			}

			uiInfo.uiDC.Assets.datapadmoveSaberSound1 = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		if (Q_stricmp(token, "datapadmoveSaberSound2") == 0)
		{
			if (PC_ParseString(&tempStr))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'datapadmoveSaberSound2'");
				return qfalse;
			}

			uiInfo.uiDC.Assets.datapadmoveSaberSound2 = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		if (Q_stricmp(token, "datapadmoveSaberSound3") == 0)
		{
			if (PC_ParseString(&tempStr))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'datapadmoveSaberSound3'");
				return qfalse;
			}

			uiInfo.uiDC.Assets.datapadmoveSaberSound3 = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		if (Q_stricmp(token, "datapadmoveSaberSound4") == 0)
		{
			if (PC_ParseString(&tempStr))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'datapadmoveSaberSound4'");
				return qfalse;
			}

			uiInfo.uiDC.Assets.datapadmoveSaberSound4 = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		if (Q_stricmp(token, "datapadmoveSaberSound5") == 0)
		{
			if (PC_ParseString(&tempStr))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'datapadmoveSaberSound5'");
				return qfalse;
			}

			uiInfo.uiDC.Assets.datapadmoveSaberSound5 = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		if (Q_stricmp(token, "datapadmoveSaberSound6") == 0)
		{
			if (PC_ParseString(&tempStr))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'datapadmoveSaberSound6'");
				return qfalse;
			}

			uiInfo.uiDC.Assets.datapadmoveSaberSound6 = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		if (Q_stricmp(token, "cursor") == 0)
		{
			if (PC_ParseString(&tempStr))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'cursor'");
				return qfalse;
			}
			uiInfo.uiDC.Assets.cursor = ui.R_RegisterShaderNoMip( tempStr);
			continue;
		}

		if (Q_stricmp(token, "fadeClamp") == 0)
		{
			if (PC_ParseFloat(&uiInfo.uiDC.Assets.fadeClamp))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'fadeClamp'");
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token, "fadeCycle") == 0)
		{
			if (PC_ParseInt(&uiInfo.uiDC.Assets.fadeCycle))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'fadeCycle'");
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token, "fadeAmount") == 0)
		{
			if (PC_ParseFloat(&uiInfo.uiDC.Assets.fadeAmount))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'fadeAmount'");
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token, "shadowX") == 0)
		{
			if (PC_ParseFloat(&uiInfo.uiDC.Assets.shadowX))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'shadowX'");
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token, "shadowY") == 0)
		{
			if (PC_ParseFloat(&uiInfo.uiDC.Assets.shadowY))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'shadowY'");
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token, "shadowColor") == 0)
		{
			if (PC_ParseColor(&uiInfo.uiDC.Assets.shadowColor))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'shadowColor'");
				return qfalse;
			}
			uiInfo.uiDC.Assets.shadowFadeClamp = uiInfo.uiDC.Assets.shadowColor[3];
			continue;
		}

		// precaching various sound files used in the menus
		if (Q_stricmp(token, "precacheSound") == 0)
		{
			if (PC_Script_Parse(&tempStr))
			{
				char *soundFile;
				do
				{
					soundFile = COM_ParseExt(&tempStr, qfalse);
					if (soundFile[0] != 0 && soundFile[0] != ';') {
						if (!trap_S_RegisterSound( soundFile, qfalse ))
						{
							PC_ParseWarning("Can't locate precache sound");
						}
					}
				} while (soundFile[0]);
			}
			continue;
		}
	}

	PC_ParseWarning(va("Invalid keyword '%s'",token));
	return qfalse;
}

/*
=================
UI_Update
=================
*/
static void UI_Update(const char *name)
{
	int	val = trap_Cvar_VariableValue(name);


	if (Q_stricmp(name, "s_khz") == 0)
	{
		ui.Cmd_ExecuteText( EXEC_APPEND, "snd_restart\n" );
		return;
	}

	if (Q_stricmp(name, "ui_SetName") == 0)
	{
		Cvar_Set( "name", UI_Cvar_VariableString("ui_Name"));
 	}
	else if (Q_stricmp(name, "ui_GetName") == 0)
	{
		Cvar_Set( "ui_Name", UI_Cvar_VariableString("name"));
 	}
	else if (Q_stricmp(name, "ui_r_colorbits") == 0)
	{
		switch (val)
		{
			case 0:
				Cvar_SetValue( "ui_r_depthbits", 0 );
				break;

			case 16:
				Cvar_SetValue( "ui_r_depthbits", 16 );
				break;

			case 32:
				Cvar_SetValue( "ui_r_depthbits", 24 );
				break;
		}
	}
	else if (Q_stricmp(name, "ui_r_lodbias") == 0)
	{
		switch (val)
		{
			case 0:
				Cvar_SetValue( "ui_r_subdivisions", 4 );
				break;
			case 1:
				Cvar_SetValue( "ui_r_subdivisions", 12 );
				break;

			case 2:
				Cvar_SetValue( "ui_r_subdivisions", 20 );
				break;
		}
	}
	else if (Q_stricmp(name, "ui_r_glCustom") == 0)
	{
		switch (val)
		{
			case 0:	// high quality

				Cvar_SetValue( "ui_r_fullScreen", 1 );
				Cvar_SetValue( "ui_r_subdivisions", 4 );
				Cvar_SetValue( "ui_r_lodbias", 0 );
				Cvar_SetValue( "ui_r_colorbits", 32 );
				Cvar_SetValue( "ui_r_depthbits", 24 );
				Cvar_SetValue( "ui_r_picmip", 0 );
				Cvar_SetValue( "ui_r_mode", 4 );
				Cvar_SetValue( "ui_r_texturebits", 32 );
				Cvar_SetValue( "ui_r_fastSky", 0 );
				Cvar_SetValue( "ui_r_inGameVideo", 1 );
				//Cvar_SetValue( "ui_cg_shadows", 2 );//stencil
				Cvar_Set( "ui_r_texturemode", "GL_LINEAR_MIPMAP_LINEAR" );
				break;

			case 1: // normal
				Cvar_SetValue( "ui_r_fullScreen", 1 );
				Cvar_SetValue( "ui_r_subdivisions", 4 );
				Cvar_SetValue( "ui_r_lodbias", 0 );
				Cvar_SetValue( "ui_r_colorbits", 0 );
				Cvar_SetValue( "ui_r_depthbits", 24 );
				Cvar_SetValue( "ui_r_picmip", 1 );
				Cvar_SetValue( "ui_r_mode", 3 );
				Cvar_SetValue( "ui_r_texturebits", 0 );
				Cvar_SetValue( "ui_r_fastSky", 0 );
				Cvar_SetValue( "ui_r_inGameVideo", 1 );
				//Cvar_SetValue( "ui_cg_shadows", 2 );
				Cvar_Set( "ui_r_texturemode", "GL_LINEAR_MIPMAP_LINEAR" );
				break;

			case 2: // fast

				Cvar_SetValue( "ui_r_fullScreen", 1 );
				Cvar_SetValue( "ui_r_subdivisions", 12 );
				Cvar_SetValue( "ui_r_lodbias", 1 );
				Cvar_SetValue( "ui_r_colorbits", 0 );
				Cvar_SetValue( "ui_r_depthbits", 0 );
				Cvar_SetValue( "ui_r_picmip", 2 );
				Cvar_SetValue( "ui_r_mode", 3 );
				Cvar_SetValue( "ui_r_texturebits", 0 );
				Cvar_SetValue( "ui_r_fastSky", 1 );
				Cvar_SetValue( "ui_r_inGameVideo", 0 );
				//Cvar_SetValue( "ui_cg_shadows", 1 );
				Cvar_Set( "ui_r_texturemode", "GL_LINEAR_MIPMAP_NEAREST" );
				break;

			case 3: // fastest

				Cvar_SetValue( "ui_r_fullScreen", 1 );
				Cvar_SetValue( "ui_r_subdivisions", 20 );
				Cvar_SetValue( "ui_r_lodbias", 2 );
				Cvar_SetValue( "ui_r_colorbits", 16 );
				Cvar_SetValue( "ui_r_depthbits", 16 );
				Cvar_SetValue( "ui_r_mode", 3 );
				Cvar_SetValue( "ui_r_picmip", 3 );
				Cvar_SetValue( "ui_r_texturebits", 16 );
				Cvar_SetValue( "ui_r_fastSky", 1 );
				Cvar_SetValue( "ui_r_inGameVideo", 0 );
				//Cvar_SetValue( "ui_cg_shadows", 0 );
				Cvar_Set( "ui_r_texturemode", "GL_LINEAR_MIPMAP_NEAREST" );
			break;
		}
	}
	else if (Q_stricmp(name, "ui_mousePitch") == 0)
	{
		if (val == 0)
		{
			Cvar_SetValue( "m_pitch", 0.022f );
		}
		else
		{
			Cvar_SetValue( "m_pitch", -0.022f );
		}
	}
	else
	{//failure!!
		Com_Printf("unknown UI script UPDATE %s\n", name);
	}
}

#define ASSET_SCROLLBAR             "gfx/menus/scrollbar.tga"
#define ASSET_SCROLLBAR_ARROWDOWN   "gfx/menus/scrollbar_arrow_dwn_a.tga"
#define ASSET_SCROLLBAR_ARROWUP     "gfx/menus/scrollbar_arrow_up_a.tga"
#define ASSET_SCROLLBAR_ARROWLEFT   "gfx/menus/scrollbar_arrow_left.tga"
#define ASSET_SCROLLBAR_ARROWRIGHT  "gfx/menus/scrollbar_arrow_right.tga"
#define ASSET_SCROLL_THUMB          "gfx/menus/scrollbar_thumb.tga"


/*
=================
AssetCache
=================
*/
void AssetCache(void)
{
//	int n;
	uiInfo.uiDC.Assets.scrollBar = ui.R_RegisterShaderNoMip( ASSET_SCROLLBAR );
	uiInfo.uiDC.Assets.scrollBarArrowDown = ui.R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWDOWN );
	uiInfo.uiDC.Assets.scrollBarArrowUp = ui.R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWUP );
	uiInfo.uiDC.Assets.scrollBarArrowLeft = ui.R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWLEFT );
	uiInfo.uiDC.Assets.scrollBarArrowRight = ui.R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWRIGHT );
	uiInfo.uiDC.Assets.scrollBarThumb = ui.R_RegisterShaderNoMip( ASSET_SCROLL_THUMB );

	uiInfo.uiDC.Assets.sliderBar = ui.R_RegisterShaderNoMip( "menu/new/slider" );
	uiInfo.uiDC.Assets.sliderThumb = ui.R_RegisterShaderNoMip( "menu/new/sliderthumb");


	/*
	for( n = 0; n < NUM_CROSSHAIRS; n++ )
	{
		uiInfo.uiDC.Assets.crosshairShader[n] = ui.R_RegisterShaderNoMip( va("gfx/2d/crosshair%c", 'a' + n ) );
	}
	*/
}

/*
================
_UI_DrawSides
=================
*/
void _UI_DrawSides(float x, float y, float w, float h, float size)
{
	size *= uiInfo.uiDC.xscale;
	trap_R_DrawStretchPic( x, y, size, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap_R_DrawStretchPic( x + w - size, y, size, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
}

/*
================
_UI_DrawTopBottom
=================
*/
void _UI_DrawTopBottom(float x, float y, float w, float h, float size)
{
	size *= uiInfo.uiDC.yscale;
	trap_R_DrawStretchPic( x, y, w, size, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap_R_DrawStretchPic( x, y + h - size, w, size, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
}
/*
================
UI_DrawRect

Coordinates are 640*480 virtual values
=================
*/
void _UI_DrawRect( float x, float y, float width, float height, float size, const float *color )
{
	trap_R_SetColor( color );

	_UI_DrawTopBottom(x, y, width, height, size);
	_UI_DrawSides(x, y, width, height, size);

	trap_R_SetColor( NULL );
}

/*
=================
UI_UpdateCvars
=================
*/
void UI_UpdateCvars( void )
{
	size_t i = 0;
	const cvarTable_t *cv = NULL;

	for ( i=0, cv=cvarTable; i<cvarTableSize; i++, cv++ ) {
		if ( cv->vmCvar ) {
			int modCount = cv->vmCvar->modificationCount;
			Cvar_Update( cv->vmCvar );
			if ( cv->vmCvar->modificationCount != modCount ) {
				if ( cv->update )
					cv->update();
			}
		}
	}
}

/*
=================
UI_DrawEffects
=================
*/
static void UI_DrawEffects(rectDef_t *rect, float scale, vec4_t color)
{
	UI_DrawHandlePic( rect->x, rect->y - 14, 128, 8, 0/*uiInfo.uiDC.Assets.fxBasePic*/ );
	UI_DrawHandlePic( rect->x + uiInfo.effectsColor * 16 + 8, rect->y - 16, 16, 12, 0/*uiInfo.uiDC.Assets.fxPic[uiInfo.effectsColor]*/ );
}

/*
=================
UI_Version
=================
*/
static void UI_Version(rectDef_t *rect, float scale, vec4_t color, int iFontIndex)
{
	int width;

	width = DC->textWidth(Q3_VERSION, scale, 0);

	DC->drawText(rect->x - width, rect->y, scale, color, Q3_VERSION, 0, ITEM_TEXTSTYLE_SHADOWED, iFontIndex);
}

/*
=================
UI_DrawKeyBindStatus
=================
*/
static void UI_DrawKeyBindStatus(rectDef_t *rect, float scale, vec4_t color, int textStyle, int iFontIndex)
{
	if (Display_KeyBindPending())
	{
#ifdef JK2_MODE
		Text_Paint(rect->x, rect->y, scale, color, ui.SP_GetStringTextString("MENUS_WAITINGFORKEY"), 0, textStyle, iFontIndex);
#else
		Text_Paint(rect->x, rect->y, scale, color, SE_GetString("MENUS_WAITINGFORKEY"), 0, textStyle, iFontIndex);
#endif
	}
	else
	{
//		Text_Paint(rect->x, rect->y, scale, color, ui.SP_GetStringTextString("MENUS_ENTERTOCHANGE"), 0, textStyle, iFontIndex);
	}
}

/*
=================
UI_DrawKeyBindStatus
=================
*/
static void UI_DrawGLInfo(rectDef_t *rect, float scale, vec4_t color, int textStyle, int iFontIndex)
{
#define MAX_LINES 64
	char buff[4096];
	char * eptr = buff;
	const char *lines[MAX_LINES];
	int y, numLines=0, i=0;

	y = rect->y;
	Text_Paint(rect->x, y, scale, color, va("GL_VENDOR: %s",uiInfo.uiDC.glconfig.vendor_string), rect->w, textStyle, iFontIndex);
	y += 15;
	Text_Paint(rect->x, y, scale, color, va("GL_VERSION: %s: %s", uiInfo.uiDC.glconfig.version_string,uiInfo.uiDC.glconfig.renderer_string), rect->w, textStyle, iFontIndex);
	y += 15;
	Text_Paint(rect->x, y, scale, color, "GL_PIXELFORMAT:", rect->w, textStyle, iFontIndex);
	y += 15;
	Text_Paint(rect->x, y, scale, color, va ("Color(%d-bits) Z(%d-bits) stencil(%d-bits)",uiInfo.uiDC.glconfig.colorBits, uiInfo.uiDC.glconfig.depthBits, uiInfo.uiDC.glconfig.stencilBits), rect->w, textStyle, iFontIndex);
	y += 15;
	// build null terminated extension strings
	Q_strncpyz(buff, uiInfo.uiDC.glconfig.extensions_string, sizeof(buff));
	int testy=y-16;
	while ( testy <= rect->y + rect->h && *eptr && (numLines < MAX_LINES) )
	{
		while ( *eptr && *eptr == ' ' )
			*eptr++ = '\0';

		// track start of valid string
		if (*eptr && *eptr != ' ')
		{
			lines[numLines++] = eptr;
			testy+=16;
		}

		while ( *eptr && *eptr != ' ' )
			eptr++;
	}

	numLines--;
	while (i < numLines)
	{
		Text_Paint(rect->x, y, scale, color, lines[i++], rect->w, textStyle, iFontIndex);
		y += 16;
	}
}

/*
=================
UI_DataPad_Inventory
=================
*/
/*
static void UI_DataPad_Inventory(rectDef_t *rect, float scale, vec4_t color, int iFontIndex)
{
	Text_Paint(rect->x, rect->y, scale, color, "INVENTORY", 0, 1, iFontIndex);
}
*/
/*
=================
UI_DataPad_ForcePowers
=================
*/
/*
static void UI_DataPad_ForcePowers(rectDef_t *rect, float scale, vec4_t color, int iFontIndex)
{
	Text_Paint(rect->x, rect->y, scale, color, "FORCE POWERS", 0, 1, iFontIndex);
}
*/

static void UI_DrawCrosshair(rectDef_t *rect, float scale, vec4_t color) {
 	trap_R_SetColor( color );
	if (uiInfo.currentCrosshair < 0 || uiInfo.currentCrosshair >= NUM_CROSSHAIRS) {
		uiInfo.currentCrosshair = 0;
	}
	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.uiDC.Assets.crosshairShader[uiInfo.currentCrosshair]);
 	trap_R_SetColor( NULL );
}


/*
=================
UI_OwnerDraw
=================
*/
static void UI_OwnerDraw(float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float special, float scale, vec4_t color, qhandle_t shader, int textStyle, int iFontIndex)
{
	rectDef_t rect;

	rect.x = x + text_x;
	rect.y = y + text_y;
	rect.w = w;
	rect.h = h;

	switch (ownerDraw)
	{
		case UI_EFFECTS:
			UI_DrawEffects(&rect, scale, color);
			break;
		case UI_VERSION:
			UI_Version(&rect, scale, color, iFontIndex);
			break;

		case UI_DATAPAD_MISSION:
			ui.Draw_DataPad(DP_HUD);
			ui.Draw_DataPad(DP_OBJECTIVES);
			break;

		case UI_DATAPAD_WEAPONS:
			ui.Draw_DataPad(DP_HUD);
			ui.Draw_DataPad(DP_WEAPONS);
			break;

		case UI_DATAPAD_INVENTORY:
			ui.Draw_DataPad(DP_HUD);
			ui.Draw_DataPad(DP_INVENTORY);
			break;

		case UI_DATAPAD_FORCEPOWERS:
			ui.Draw_DataPad(DP_HUD);
			ui.Draw_DataPad(DP_FORCEPOWERS);
			break;

		case UI_ALLMAPS_SELECTION://saved game thumbnail

			int levelshot;
			levelshot = ui.R_RegisterShaderNoMip( va( "levelshots/%s", s_savedata[s_savegame.currentLine].currentSaveFileMap ) );
#ifdef JK2_MODE
			if (screenShotBuf[0])
			{
				ui.DrawStretchRaw( x, y, w, h, SG_SCR_WIDTH, SG_SCR_HEIGHT, screenShotBuf, 0, qtrue );
			}
			else
#endif
			if (levelshot)
			{
				ui.R_DrawStretchPic( x, y, w, h, 0, 0, 1, 1, levelshot );
			}
			else
			{
				UI_DrawHandlePic(x, y, w, h, uis.menuBackShader);
			}

			ui.R_Font_DrawString(	x,		// int ox
									y+h,	// int oy
									s_savedata[s_savegame.currentLine].currentSaveFileMap,	// const char *text
									color,	// paletteRGBA_c c
									iFontIndex,	// const int iFontHandle
									w,//-1,		// iMaxPixelWidth (-1 = none)
									scale	// const float scale = 1.0f
									);
			break;
		case UI_PREVIEWCINEMATIC:
			// FIXME BOB - make this work?
//			UI_DrawPreviewCinematic(&rect, scale, color);
			break;
		case UI_CROSSHAIR:
			UI_DrawCrosshair(&rect, scale, color);
			break;
		case UI_GLINFO:
			UI_DrawGLInfo(&rect,scale, color, textStyle, iFontIndex);
			break;
		case UI_KEYBINDSTATUS:
			UI_DrawKeyBindStatus(&rect,scale, color, textStyle, iFontIndex);
			break;
		default:
		  break;
	}

}

/*
=================
UI_OwnerDrawVisible
=================
*/
static qboolean UI_OwnerDrawVisible(int flags)
{
	qboolean vis = qtrue;

	while (flags)
	{
/*		if (flags & UI_SHOW_DEMOAVAILABLE)
		{
			if (!uiInfo.demoAvailable)
			{
				vis = qfalse;
			}
			flags &= ~UI_SHOW_DEMOAVAILABLE;
		}
		else
*/		{
			flags = 0;
		}
	}
	return vis;
}

/*
=================
Text_Width
=================
*/
int Text_Width(const char *text, float scale, int iFontIndex)
{
	// temp code until Bob retro-fits all menus to have font specifiers...
	//
	if ( iFontIndex == 0 )
	{
		iFontIndex = uiInfo.uiDC.Assets.qhMediumFont;
	}
	return ui.R_Font_StrLenPixels(text, iFontIndex, scale);
}

/*
=================
UI_OwnerDrawWidth
=================
*/
int UI_OwnerDrawWidth(int ownerDraw, float scale)
{
//	int i, h, value;
//	const char *text;
	const char *s = NULL;


	switch (ownerDraw)
	{
	case UI_KEYBINDSTATUS:
		if (Display_KeyBindPending())
		{
#ifdef JK2_MODE
			s = ui.SP_GetStringTextString("MENUS_WAITINGFORKEY");
#else
			s = SE_GetString("MENUS_WAITINGFORKEY");
#endif
		}
		else
		{
//			s = ui.SP_GetStringTextString("MENUS_ENTERTOCHANGE");
		}
		break;

	// FIXME BOB
//	case UI_SERVERREFRESHDATE:
//		s = UI_Cvar_VariableString(va("ui_lastServerRefresh_%i", ui_netSource.integer));
//		break;
    default:
      break;
	}

	if (s)
	{
		return Text_Width(s, scale, 0);
	}
	return 0;
}

/*
=================
Text_Height
=================
*/
int Text_Height(const char *text, float scale, int iFontIndex)
{
	// temp until Bob retro-fits all menu files with font specifiers...
	//
	if ( iFontIndex == 0 )
	{
		iFontIndex = uiInfo.uiDC.Assets.qhMediumFont;
	}
	return ui.R_Font_HeightPixels(iFontIndex, scale);
}


/*
=================
UI_MouseEvent
=================
*/
//JLFMOUSE  CALLED EACH FRAME IN UI
void _UI_MouseEvent( int dx, int dy )
{
	// update mouse screen position
	uiInfo.uiDC.cursorx += dx;
	if (uiInfo.uiDC.cursorx < 0)
	{
		uiInfo.uiDC.cursorx = 0;
	}
	else if (uiInfo.uiDC.cursorx > SCREEN_WIDTH)
	{
		uiInfo.uiDC.cursorx = SCREEN_WIDTH;
	}

	uiInfo.uiDC.cursory += dy;
	if (uiInfo.uiDC.cursory < 0)
	{
		uiInfo.uiDC.cursory = 0;
	}
	else if (uiInfo.uiDC.cursory > SCREEN_HEIGHT)
	{
		uiInfo.uiDC.cursory = SCREEN_HEIGHT;
	}

	if (Menu_Count() > 0)
	{
    //menuDef_t *menu = Menu_GetFocused();
    //Menu_HandleMouseMove(menu, uiInfo.uiDC.cursorx, uiInfo.uiDC.cursory);
		Display_MouseMove(NULL, uiInfo.uiDC.cursorx, uiInfo.uiDC.cursory);
	}

}

/*
=================
UI_KeyEvent
=================
*/
void _UI_KeyEvent( int key, qboolean down )
{
/*	extern qboolean SwallowBadNumLockedKPKey( int iKey );
	if (SwallowBadNumLockedKPKey(key)){
		return;
	}
*/

	if (Menu_Count() > 0)
	{
		menuDef_t *menu = Menu_GetFocused();
		if (menu)
		{
			//DemoEnd();
			if (key == A_ESCAPE && down && !Menus_AnyFullScreenVisible() && !(menu->window.flags & WINDOW_IGNORE_ESCAPE))
			{
				Menus_CloseAll();
			}
			else
			{
				Menu_HandleKey(menu, key, down );
			}
		}
		else
		{
			trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
			trap_Key_ClearStates();
			Cvar_Set( "cl_paused", "0" );
		}
	}
}

/*
=================
UI_Report
=================
*/
void UI_Report(void)
{
  String_Report();
}



/*
=================
UI_DataPadMenu
=================
*/
void UI_DataPadMenu(void)
{
	int	newForcePower,newObjective;

	Menus_CloseByName("mainhud");

	newForcePower = (int)trap_Cvar_VariableValue("cg_updatedDataPadForcePower1");
	newObjective = (int)trap_Cvar_VariableValue("cg_updatedDataPadObjective");

	if (newForcePower)
	{
		Menus_ActivateByName("datapadForcePowersMenu");
	}
	else if (newObjective)
	{
		Menus_ActivateByName("datapadMissionMenu");
	}
	else
	{
		Menus_ActivateByName("datapadMissionMenu");
	}
	ui.Key_SetCatcher( KEYCATCH_UI );

}

/*
=================
UI_InGameMenu
=================
*/
void UI_InGameMenu(const char*menuID)
{
#ifdef JK2_MODE
	ui.PrecacheScreenshot();
#endif
	Menus_CloseByName("mainhud");

	if (menuID)
	{
		Menus_ActivateByName(menuID);
	}
	else
	{
		Menus_ActivateByName("ingameMainMenu");
	}
	ui.Key_SetCatcher( KEYCATCH_UI );
}

qboolean _UI_IsFullscreen( void )
{
	return Menus_AnyFullScreenVisible();
}

/*
=======================================================================

MAIN MENU

=======================================================================
*/


/*
===============
UI_MainMenu

The main menu only comes up when not in a game,
so make sure that the attract loop server is down
and that local cinematics are killed
===============
*/
void UI_MainMenu(void)
{
	char buf[256];
	ui.Cvar_Set("sv_killserver", "1");	// let the demo server know it should shut down

	ui.Key_SetCatcher( KEYCATCH_UI );

	menuDef_t *m = Menus_ActivateByName("mainMenu");
	if (!m)
	{	//wha? try again
		UI_LoadMenus("ui/menus.txt",qfalse);
	}
	ui.Cvar_VariableStringBuffer("com_errorMessage", buf, sizeof(buf));
	if (strlen(buf)) {
		Menus_ActivateByName("error_popmenu");
	}
}


/*
=================
Menu_Cache
=================
*/
void Menu_Cache( void )
{
	uis.cursor		= ui.R_RegisterShaderNoMip( "menu/new/crosshairb");
	// Common menu graphics
	uis.whiteShader = ui.R_RegisterShader( "white" );
	uis.menuBackShader = ui.R_RegisterShaderNoMip( "menu/art/unknownmap" );
}

/*
=================
UI_UpdateVideoSetup

Copies the temporary user interface version of the video cvars into
their real counterparts.  This is to create a interface which allows
you to discard your changes if you did something you didnt want
=================
*/
void UI_UpdateVideoSetup ( void )
{
	Cvar_Set ( "r_mode", Cvar_VariableString ( "ui_r_mode" ) );
	Cvar_Set ( "r_fullscreen", Cvar_VariableString ( "ui_r_fullscreen" ) );
	Cvar_Set ( "r_colorbits", Cvar_VariableString ( "ui_r_colorbits" ) );
	Cvar_Set ( "r_lodbias", Cvar_VariableString ( "ui_r_lodbias" ) );
	Cvar_Set ( "r_picmip", Cvar_VariableString ( "ui_r_picmip" ) );
	Cvar_Set ( "r_texturebits", Cvar_VariableString ( "ui_r_texturebits" ) );
	Cvar_Set ( "r_texturemode", Cvar_VariableString ( "ui_r_texturemode" ) );
	Cvar_Set ( "r_detailtextures", Cvar_VariableString ( "ui_r_detailtextures" ) );
	Cvar_Set ( "r_ext_compress_textures", Cvar_VariableString ( "ui_r_ext_compress_textures" ) );
	Cvar_Set ( "r_depthbits", Cvar_VariableString ( "ui_r_depthbits" ) );
	Cvar_Set ( "r_subdivisions", Cvar_VariableString ( "ui_r_subdivisions" ) );
	Cvar_Set ( "r_fastSky", Cvar_VariableString ( "ui_r_fastSky" ) );
	Cvar_Set ( "r_inGameVideo", Cvar_VariableString ( "ui_r_inGameVideo" ) );
	Cvar_Set ( "r_allowExtensions", Cvar_VariableString ( "ui_r_allowExtensions" ) );
//	Cvar_Set ( "cg_shadows", Cvar_VariableString ( "ui_cg_shadows" ) );
	Cvar_Set ( "ui_r_modified", "0" );

	Cbuf_ExecuteText( EXEC_APPEND, "vid_restart;" );
}

/*
=================
UI_GetVideoSetup

Retrieves the current actual video settings into the temporary user
interface versions of the cvars.
=================
*/
void UI_GetVideoSetup ( void )
{
	Cvar_Register ( NULL, "ui_r_glCustom",				"4", CVAR_ARCHIVE );

	// Make sure the cvars are registered as read only.
	Cvar_Register ( NULL, "ui_r_mode",					"0", CVAR_ROM );
	Cvar_Register ( NULL, "ui_r_fullscreen",			"0", CVAR_ROM );
	Cvar_Register ( NULL, "ui_r_colorbits",				"0", CVAR_ROM );
	Cvar_Register ( NULL, "ui_r_lodbias",				"0", CVAR_ROM );
	Cvar_Register ( NULL, "ui_r_picmip",				"0", CVAR_ROM );
	Cvar_Register ( NULL, "ui_r_texturebits",			"0", CVAR_ROM );
	Cvar_Register ( NULL, "ui_r_texturemode",			"0", CVAR_ROM );
	Cvar_Register ( NULL, "ui_r_detailtextures",		"0", CVAR_ROM );
	Cvar_Register ( NULL, "ui_r_ext_compress_textures",	"0", CVAR_ROM );
	Cvar_Register ( NULL, "ui_r_depthbits",				"0", CVAR_ROM );
	Cvar_Register ( NULL, "ui_r_subdivisions",			"0", CVAR_ROM );
	Cvar_Register ( NULL, "ui_r_fastSky",				"0", CVAR_ROM );
	Cvar_Register ( NULL, "ui_r_inGameVideo",			"0", CVAR_ROM );
	Cvar_Register ( NULL, "ui_r_allowExtensions",		"0", CVAR_ROM );
//	Cvar_Register ( NULL, "ui_cg_shadows",				"0", CVAR_ROM );
	Cvar_Register ( NULL, "ui_r_modified",				"0", CVAR_ROM );

	// Copy over the real video cvars into their temporary counterparts
	Cvar_Set ( "ui_r_mode", Cvar_VariableString ( "r_mode" ) );
	Cvar_Set ( "ui_r_colorbits", Cvar_VariableString ( "r_colorbits" ) );
	Cvar_Set ( "ui_r_fullscreen", Cvar_VariableString ( "r_fullscreen" ) );
	Cvar_Set ( "ui_r_lodbias", Cvar_VariableString ( "r_lodbias" ) );
	Cvar_Set ( "ui_r_picmip", Cvar_VariableString ( "r_picmip" ) );
	Cvar_Set ( "ui_r_texturebits", Cvar_VariableString ( "r_texturebits" ) );
	Cvar_Set ( "ui_r_texturemode", Cvar_VariableString ( "r_texturemode" ) );
	Cvar_Set ( "ui_r_detailtextures", Cvar_VariableString ( "r_detailtextures" ) );
	Cvar_Set ( "ui_r_ext_compress_textures", Cvar_VariableString ( "r_ext_compress_textures" ) );
	Cvar_Set ( "ui_r_depthbits", Cvar_VariableString ( "r_depthbits" ) );
	Cvar_Set ( "ui_r_subdivisions", Cvar_VariableString ( "r_subdivisions" ) );
	Cvar_Set ( "ui_r_fastSky", Cvar_VariableString ( "r_fastSky" ) );
	Cvar_Set ( "ui_r_inGameVideo", Cvar_VariableString ( "r_inGameVideo" ) );
	Cvar_Set ( "ui_r_allowExtensions", Cvar_VariableString ( "r_allowExtensions" ) );
//	Cvar_Set ( "ui_cg_shadows", Cvar_VariableString ( "cg_shadows" ) );
	Cvar_Set ( "ui_r_modified", "0" );
}

static void UI_SetSexandSoundForModel(const char* char_model)
{
	int			f,i;
	char		soundpath[MAX_QPATH];
	qboolean	isFemale = qfalse;

	i = ui.FS_FOpenFile(va("models/players/%s/sounds.cfg", char_model), &f, FS_READ);
	if ( !f )
	{//no?  oh bother.
		Cvar_Reset("snd");
		Cvar_Reset("sex");
		return;
	}

	soundpath[0] = 0;

	ui.FS_Read(&soundpath, i, f);

	while (i >= 0 && soundpath[i] != '\n')
	{
		if (soundpath[i] == 'f')
		{
			isFemale = qtrue;
			soundpath[i] = 0;
		}

		i--;
	}

	i = 0;

	while (soundpath[i] && soundpath[i] != '\r' && soundpath[i] != '\n')
	{
		i++;
	}
	soundpath[i] = 0;

	ui.FS_FCloseFile(f);

	Cvar_Set ( "snd", soundpath);
	if (isFemale)
	{
		Cvar_Set ( "sex", "f");
	}
	else
	{
		Cvar_Set ( "sex", "m");
	}
}
static void UI_UpdateCharacterCvars ( void )
{
	const char *char_model = Cvar_VariableString ( "ui_char_model" );
	UI_SetSexandSoundForModel(char_model);
	Cvar_Set ( "g_char_model", char_model );
	Cvar_Set ( "g_char_skin_head", Cvar_VariableString ( "ui_char_skin_head" ) );
	Cvar_Set ( "g_char_skin_torso", Cvar_VariableString ( "ui_char_skin_torso" ) );
	Cvar_Set ( "g_char_skin_legs", Cvar_VariableString ( "ui_char_skin_legs" ) );
	Cvar_Set ( "g_char_color_red", Cvar_VariableString ( "ui_char_color_red" ) );
	Cvar_Set ( "g_char_color_green", Cvar_VariableString ( "ui_char_color_green" ) );
	Cvar_Set ( "g_char_color_blue", Cvar_VariableString ( "ui_char_color_blue" ) );
}

static void UI_GetCharacterCvars ( void )
{
	Cvar_Set ( "ui_char_skin_head", Cvar_VariableString ( "g_char_skin_head" ) );
	Cvar_Set ( "ui_char_skin_torso", Cvar_VariableString ( "g_char_skin_torso" ) );
	Cvar_Set ( "ui_char_skin_legs", Cvar_VariableString ( "g_char_skin_legs" ) );
	Cvar_Set ( "ui_char_color_red", Cvar_VariableString ( "g_char_color_red" ) );
	Cvar_Set ( "ui_char_color_green", Cvar_VariableString ( "g_char_color_green" ) );
	Cvar_Set ( "ui_char_color_blue", Cvar_VariableString ( "g_char_color_blue" ) );

	const char* model = Cvar_VariableString ( "g_char_model" );
	Cvar_Set ( "ui_char_model", model );

	for (int i = 0; i < uiInfo.playerSpeciesCount; i++)
	{
		if ( !Q_stricmp(model, uiInfo.playerSpecies[i].Name) )
		{
			uiInfo.playerSpeciesIndex = i;
		}
	}
}

static void UI_UpdateSaberCvars ( void )
{
	Cvar_Set ( "g_saber_type", Cvar_VariableString ( "ui_saber_type" ) );
	Cvar_Set ( "g_saber", Cvar_VariableString ( "ui_saber" ) );
	Cvar_Set ( "g_saber2", Cvar_VariableString ( "ui_saber2" ) );
	Cvar_Set ( "g_saber_color", Cvar_VariableString ( "ui_saber_color" ) );
	Cvar_Set ( "g_saber2_color", Cvar_VariableString ( "ui_saber2_color" ) );
}

#ifndef JK2_MODE
static void UI_UpdateFightingStyleChoices ( void )
{
	//
	if (!Q_stricmp("staff",Cvar_VariableString ( "ui_saber_type" )))
	{
		Cvar_Set ( "ui_fightingstylesallowed", "0" );
		Cvar_Set ( "ui_newfightingstyle", "4" );		// SS_STAFF
	}
	else if (!Q_stricmp("dual",Cvar_VariableString ( "ui_saber_type" )))
	{
		Cvar_Set ( "ui_fightingstylesallowed", "0" );
		Cvar_Set ( "ui_newfightingstyle", "3" );		// SS_DUAL
	}
	else
	{
		// Get player state
		client_t	*cl = &svs.clients[0];	// 0 because only ever us as a player
		playerState_t	*pState;

		if (cl && cl->gentity && cl->gentity->client)
		{
			pState = cl->gentity->client;


			// Knows Fast style?
			if (pState->saberStylesKnown & (1<<SS_FAST))
			{
				// And Medium?
				if (pState->saberStylesKnown & (1<<SS_MEDIUM))
				{
					Cvar_Set ( "ui_fightingstylesallowed", "6" );	// Has FAST and MEDIUM, so can only choose STRONG
					Cvar_Set ( "ui_newfightingstyle", "2" );		// STRONG
				}
				else
				{
					Cvar_Set ( "ui_fightingstylesallowed", "1" );	// Has FAST, so can choose from MEDIUM and STRONG
					Cvar_Set ( "ui_newfightingstyle", "1" );		// MEDIUM
				}
			}
			// Knows Medium style?
			else if (pState->saberStylesKnown & (1<<SS_MEDIUM))
			{
				// And Strong?
				if (pState->saberStylesKnown & (1<<SS_STRONG))
				{
					Cvar_Set ( "ui_fightingstylesallowed", "4" );	// Has MEDIUM and STRONG, so can only choose FAST
					Cvar_Set ( "ui_newfightingstyle", "0" );		// FAST
				}
				else
				{
					Cvar_Set ( "ui_fightingstylesallowed", "2" );	// Has MEDIUM, so can choose from FAST and STRONG
					Cvar_Set ( "ui_newfightingstyle", "0" );		// FAST
				}
			}
			// Knows Strong style?
			else if (pState->saberStylesKnown & (1<<SS_STRONG))
			{
				// And Fast
				if (pState->saberStylesKnown & (1<<SS_FAST))
				{
					Cvar_Set ( "ui_fightingstylesallowed", "5" );	// Has STRONG and FAST, so can only take MEDIUM
					Cvar_Set ( "ui_newfightingstyle", "1" );		// MEDIUM
				}
				else
				{
					Cvar_Set ( "ui_fightingstylesallowed", "3" );	// Has STRONG, so can choose from FAST and MEDIUM
					Cvar_Set ( "ui_newfightingstyle", "1" );		// MEDIUM
				}
			}
			else		// They have nothing, which should not happen
			{
				Cvar_Set ( "ui_currentfightingstyle", "1" );		// Default MEDIUM
				Cvar_Set ( "ui_newfightingstyle", "0" );			// FAST??
				Cvar_Set ( "ui_fightingstylesallowed", "0" );		// Default to no new styles allowed
			}

			// Determine current style
			if (pState->saberAnimLevel == SS_FAST)
			{
				Cvar_Set ( "ui_currentfightingstyle", "0" );			// FAST
			}
			else if (pState->saberAnimLevel == SS_STRONG)
			{
				Cvar_Set ( "ui_currentfightingstyle", "2" );			// STRONG
			}
			else
			{
				Cvar_Set ( "ui_currentfightingstyle", "1" );			// default MEDIUM
			}
		}
		else	// No client so this must be first time
		{
			Cvar_Set ( "ui_currentfightingstyle", "1" );		// Default to MEDIUM
			Cvar_Set ( "ui_fightingstylesallowed", "0" );		// Default to no new styles allowed
			Cvar_Set ( "ui_newfightingstyle", "1" );			// MEDIUM
		}
	}
}
#endif // !JK2_MODE

#define MAX_POWER_ENUMS 16

typedef struct {
	const char	*title;
	short	powerEnum;
} powerEnum_t;

static powerEnum_t powerEnums[MAX_POWER_ENUMS] =
{
#ifndef JK2_MODE
	{ "absorb",		FP_ABSORB },
#endif // !JK2_MODE

	{ "heal",			FP_HEAL },
	{ "mindtrick",	FP_TELEPATHY },

#ifndef JK2_MODE
	{ "protect",		FP_PROTECT },
#endif // !JK2_MODE

				// Core powers
	{ "jump",			FP_LEVITATION },
	{ "pull",			FP_PULL },
	{ "push",			FP_PUSH },

#ifndef JK2_MODE
	{ "sense",		FP_SEE },
#endif // !JK2_MODE

	{ "speed",		FP_SPEED },
	{ "sabdef",		FP_SABER_DEFENSE },
	{ "saboff",		FP_SABER_OFFENSE },
	{ "sabthrow",		FP_SABERTHROW },

				// Dark powers
#ifndef JK2_MODE
	{ "drain",		FP_DRAIN },
#endif // !JK2_MODE

	{ "grip",			FP_GRIP },
	{ "lightning",	FP_LIGHTNING },

#ifndef JK2_MODE
	{ "rage",			FP_RAGE },
#endif // !JK2_MODE
};


// Find the index to the Force Power in powerEnum array
static qboolean UI_GetForcePowerIndex ( const char *forceName, short *forcePowerI )
{
	int i;

	// Find a match for the forceName passed in
	for (i=0;i<MAX_POWER_ENUMS;i++)
	{
		if ( !Q_stricmp(forceName, powerEnums[i].title ) )
		{
			*forcePowerI = i;
			return(qtrue);
		}
	}

	*forcePowerI = FP_UPDATED_NONE;	// Didn't find it

	return(qfalse);
}

// Set the fields for the allocation of force powers (Used by Force Power Allocation screen)
static void UI_InitAllocForcePowers ( const char *forceName )
{
	menuDef_t	*menu;
	itemDef_t	*item;
	short		forcePowerI=0;
	int			forcelevel;

	menu = Menu_GetFocused();	// Get current menu

	if (!menu)
	{
		return;
	}

	if (!UI_GetForcePowerIndex ( forceName, &forcePowerI ))
	{
		return;
	}

	client_t* cl = &svs.clients[0];	// 0 because only ever us as a player

	// NOTE: this UIScript can be called outside the running game now, so handle that case
	// by getting info frim UIInfo instead of PlayerState
	if( cl )
	{
		playerState_t*		pState = cl->gentity->client;
		forcelevel = pState->forcePowerLevel[powerEnums[forcePowerI].powerEnum];
	}
	else
	{
		forcelevel = uiInfo.forcePowerLevel[powerEnums[forcePowerI].powerEnum];
	}

	char itemName[128];
	Com_sprintf (itemName, sizeof(itemName), "%s_hexpic", powerEnums[forcePowerI].title);
	item = (itemDef_s *) Menu_FindItemByName(menu, itemName);

	if (item)
	{
		char itemGraphic[128];
		Com_sprintf (itemGraphic, sizeof(itemGraphic), "gfx/menus/hex_pattern_%d",forcelevel >= 4 ? 3 : forcelevel);
		item->window.background = ui.R_RegisterShaderNoMip(itemGraphic);

		// If maxed out on power - don't allow update
		if (forcelevel>=3)
		{
			Com_sprintf (itemName, sizeof(itemName), "%s_fbutton", powerEnums[forcePowerI].title);
			item = (itemDef_s *) Menu_FindItemByName(menu, itemName);
			if (item)
			{
				item->action = 0;	//you are bad, no action for you!
				item->descText = 0; //no desc either!
			}
		}
	}

	// Set weapons button to inactive
	UI_ForcePowerWeaponsButton(qfalse);
}

// Flip flop between being able to see the text showing the Force Point has or hasn't been allocated (Used by Force Power Allocation screen)
static void UI_SetPowerTitleText ( qboolean showAllocated )
{
	menuDef_t	*menu;
	itemDef_t	*item;

	menu = Menu_GetFocused();	// Get current menu

	if (!menu)
	{
		return;
	}

	if (showAllocated)
	{
		// Show the text saying the force point has been allocated
		item = (itemDef_s *) Menu_FindItemByName(menu, "allocated_text");
		if (item)
		{
			item->window.flags |= WINDOW_VISIBLE;
		}

		// Hide text saying the force point needs to be allocated
		item = (itemDef_s *) Menu_FindItemByName(menu, "allocate_text");
		if (item)
		{
			item->window.flags &= ~WINDOW_VISIBLE;
		}
	}
	else
	{
		// Hide the text saying the force point has been allocated
		item = (itemDef_s *) Menu_FindItemByName(menu, "allocated_text");
		if (item)
		{
			item->window.flags &= ~WINDOW_VISIBLE;
		}

		// Show text saying the force point needs to be allocated
		item = (itemDef_s *) Menu_FindItemByName(menu, "allocate_text");
		if (item)
		{
			item->window.flags |= WINDOW_VISIBLE;
		}
	}
}

#ifndef JK2_MODE
static int UI_CountForcePowers( void ) {
	const client_t *cl = &svs.clients[0];

	if ( cl && cl->gentity ) {
		const playerState_t *ps = cl->gentity->client;
		return		ps->forcePowerLevel[FP_HEAL] +
					ps->forcePowerLevel[FP_TELEPATHY] +
					ps->forcePowerLevel[FP_PROTECT] +
					ps->forcePowerLevel[FP_ABSORB] +
					ps->forcePowerLevel[FP_GRIP] +
					ps->forcePowerLevel[FP_LIGHTNING] +
					ps->forcePowerLevel[FP_RAGE] +
					ps->forcePowerLevel[FP_DRAIN];
	}
	else {
		return		uiInfo.forcePowerLevel[FP_HEAL] +
					uiInfo.forcePowerLevel[FP_TELEPATHY] +
					uiInfo.forcePowerLevel[FP_PROTECT] +
					uiInfo.forcePowerLevel[FP_ABSORB] +
					uiInfo.forcePowerLevel[FP_GRIP] +
					uiInfo.forcePowerLevel[FP_LIGHTNING] +
					uiInfo.forcePowerLevel[FP_RAGE] +
					uiInfo.forcePowerLevel[FP_DRAIN];
	}
}
#endif

//. Find weapons button and make active/inactive  (Used by Force Power Allocation screen)
static void UI_ForcePowerWeaponsButton(qboolean activeFlag)
{
	menuDef_t	*menu;
	menu = Menu_GetFocused();	// Get current menu

	if (!menu)
	{
		return;
	}

#ifndef JK2_MODE
	if (!activeFlag) {
		// total light and dark powers are at maximum level 3    ( 3 levels * ( 4ls + 4ds ) = 24 )
		if ( UI_CountForcePowers() >= 24 )
			activeFlag = qtrue;
	}
#endif

	// Find weaponsbutton
	itemDef_t	*item;
	item = (itemDef_s *) Menu_FindItemByName(menu, "weaponbutton");
	if (item)
	{
		// Make it active
		if (activeFlag)
		{
			item->window.flags &= ~WINDOW_INACTIVE;
		}
		else
		{
			item->window.flags |= WINDOW_INACTIVE;
		}
	}
}

void UI_SetItemColor(itemDef_t *item,const char *itemname,const char *name,vec4_t color);

static void UI_SetHexPicLevel( const menuDef_t	*menu,const int forcePowerI,const int powerLevel, const qboolean	goldFlag )
{
	char itemName[128];
	itemDef_t	*item;

	// Find proper hex picture on menu
	Com_sprintf (itemName, sizeof(itemName), "%s_hexpic", powerEnums[forcePowerI].title);
	item = (itemDef_s *) Menu_FindItemByName((menuDef_t	*) menu, itemName);

	// Now give it the proper hex graphic
	if (item)
	{
		char itemGraphic[128];
		if (goldFlag)
		{
			Com_sprintf (itemGraphic, sizeof(itemGraphic), "gfx/menus/hex_pattern_%d_gold",powerLevel >= 4 ? 3 : powerLevel);
		}
		else
		{
			Com_sprintf (itemGraphic, sizeof(itemGraphic),  "gfx/menus/hex_pattern_%d",powerLevel >= 4 ? 3 : powerLevel);
		}

		item->window.background = ui.R_RegisterShaderNoMip(itemGraphic);

		Com_sprintf (itemName, sizeof(itemName), "%s_fbutton", powerEnums[forcePowerI].title);
		item = (itemDef_s *) Menu_FindItemByName((menuDef_t	*)menu, itemName);
		if (item)
		{
			if (goldFlag)
			{
				// Change description text to tell player they can decrement the force point
				item->descText = "@MENUS_REMOVEFP";
			}
			else
			{
				// Change description text to tell player they can increment the force point
				item->descText = "@MENUS_ADDFP";
			}
		}
	}
}

void UI_SetItemVisible(menuDef_t *menu,const char *itemname,qboolean visible);

// if this is the first time into the force power allocation screen, show the INSTRUCTION screen
static void	UI_ForceHelpActive( void )
{
	int	tier_storyinfo = Cvar_VariableIntegerValue( "tier_storyinfo" );

	// First time, show instructions
	if (tier_storyinfo==1)
	{
//		Menus_OpenByName("ingameForceHelp");
		Menus_ActivateByName("ingameForceHelp");
	}
}

#ifndef JK2_MODE
// Set the force levels depending on the level chosen
static void	UI_DemoSetForceLevels( void )
{
	menuDef_t	*menu;

	menu = Menu_GetFocused();	// Get current menu

	if (!menu)
	{
		return;
	}

	char	buffer[MAX_STRING_CHARS];

	client_t* cl = &svs.clients[0];	// 0 because only ever us as a player
	playerState_t*		pState = NULL;
	if( cl )
	{
		pState = cl->gentity->client;
	}

	ui.Cvar_VariableStringBuffer( "ui_demo_level", buffer, sizeof(buffer));
	if( Q_stricmp( buffer, "t1_sour")==0 )
	{// NOTE : always set the uiInfo powers
		// level 1 in all core powers
		uiInfo.forcePowerLevel[FP_LEVITATION]=1;
		uiInfo.forcePowerLevel[FP_SPEED]=1;
		uiInfo.forcePowerLevel[FP_PUSH]=1;
		uiInfo.forcePowerLevel[FP_PULL]=1;
		uiInfo.forcePowerLevel[FP_SEE]=1;
		uiInfo.forcePowerLevel[FP_SABER_OFFENSE]=1;
		uiInfo.forcePowerLevel[FP_SABER_DEFENSE]=1;
		uiInfo.forcePowerLevel[FP_SABERTHROW]=1;
		// plus these extras
		uiInfo.forcePowerLevel[FP_HEAL]=1;
		uiInfo.forcePowerLevel[FP_TELEPATHY]=1;
		uiInfo.forcePowerLevel[FP_GRIP]=1;

		// and set the rest to zero
		uiInfo.forcePowerLevel[FP_ABSORB]=0;
		uiInfo.forcePowerLevel[FP_PROTECT]=0;
		uiInfo.forcePowerLevel[FP_DRAIN]=0;
		uiInfo.forcePowerLevel[FP_LIGHTNING]=0;
		uiInfo.forcePowerLevel[FP_RAGE]=0;
	}
	else
	{
		// level 3 in all core powers
		uiInfo.forcePowerLevel[FP_LEVITATION]=3;
		uiInfo.forcePowerLevel[FP_SPEED]=3;
		uiInfo.forcePowerLevel[FP_PUSH]=3;
		uiInfo.forcePowerLevel[FP_PULL]=3;
		uiInfo.forcePowerLevel[FP_SEE]=3;
		uiInfo.forcePowerLevel[FP_SABER_OFFENSE]=3;
		uiInfo.forcePowerLevel[FP_SABER_DEFENSE]=3;
		uiInfo.forcePowerLevel[FP_SABERTHROW]=3;

		// plus these extras
		uiInfo.forcePowerLevel[FP_HEAL]=1;
		uiInfo.forcePowerLevel[FP_TELEPATHY]=1;
		uiInfo.forcePowerLevel[FP_GRIP]=2;
		uiInfo.forcePowerLevel[FP_LIGHTNING]=1;
		uiInfo.forcePowerLevel[FP_PROTECT]=1;

		// and set the rest to zero

		uiInfo.forcePowerLevel[FP_ABSORB]=0;
		uiInfo.forcePowerLevel[FP_DRAIN]=0;
		uiInfo.forcePowerLevel[FP_RAGE]=0;
	}

	if (pState)
	{//i am carrying over from a previous level, so get the increased power! (non-core only)
		uiInfo.forcePowerLevel[FP_HEAL] = Q_max(pState->forcePowerLevel[FP_HEAL], uiInfo.forcePowerLevel[FP_HEAL]);
		uiInfo.forcePowerLevel[FP_TELEPATHY]=Q_max(pState->forcePowerLevel[FP_TELEPATHY], uiInfo.forcePowerLevel[FP_TELEPATHY]);
		uiInfo.forcePowerLevel[FP_GRIP]=Q_max(pState->forcePowerLevel[FP_GRIP], uiInfo.forcePowerLevel[FP_GRIP]);
		uiInfo.forcePowerLevel[FP_LIGHTNING]=Q_max(pState->forcePowerLevel[FP_LIGHTNING], uiInfo.forcePowerLevel[FP_LIGHTNING]);
		uiInfo.forcePowerLevel[FP_PROTECT]=Q_max(pState->forcePowerLevel[FP_PROTECT], uiInfo.forcePowerLevel[FP_PROTECT]);

		uiInfo.forcePowerLevel[FP_ABSORB]=Q_max(pState->forcePowerLevel[FP_ABSORB], uiInfo.forcePowerLevel[FP_ABSORB]);
		uiInfo.forcePowerLevel[FP_DRAIN]=Q_max(pState->forcePowerLevel[FP_DRAIN], uiInfo.forcePowerLevel[FP_DRAIN]);
		uiInfo.forcePowerLevel[FP_RAGE]=Q_max(pState->forcePowerLevel[FP_RAGE], uiInfo.forcePowerLevel[FP_RAGE]);
	}
}
#endif // !JK2_MODE

// record the force levels into a cvar so when restoring player from map transition
// the force levels are set up correctly
static void	UI_RecordForceLevels( void )
{
	menuDef_t	*menu;

	menu = Menu_GetFocused();	// Get current menu

	if (!menu)
	{
		return;
	}

	const char *s2 = "";
	int i;
	for (i=0;i< NUM_FORCE_POWERS; i++)
	{
		s2 = va("%s %i",s2, uiInfo.forcePowerLevel[i]);
	}
	Cvar_Set( "demo_playerfplvl", s2 );

}

// record the weapons into a cvar so when restoring player from map transition
// the force levels are set up correctly
static void	UI_RecordWeapons( void )
{
	menuDef_t	*menu;

	menu = Menu_GetFocused();	// Get current menu

	if (!menu)
	{
		return;
	}

	const char *s2 = "";

	int wpns = 0;
	// always add blaster and saber
	wpns |= (1<<WP_SABER);
	wpns |= (1<<WP_BLASTER_PISTOL);
	wpns |= (1<< uiInfo.selectedWeapon1);
	wpns |= (1<< uiInfo.selectedWeapon2);
	wpns |= (1<< uiInfo.selectedThrowWeapon);
	s2 = va("%i", wpns );

	Cvar_Set( "demo_playerwpns", s2 );

}

// Shut down the help screen in the force power allocation screen
static void UI_ShutdownForceHelp( void )
{
	int i;
	char itemName[128];
	menuDef_t	*menu;
	itemDef_t	*item;
	vec4_t color = { 0.65f, 0.65f, 0.65f, 1.0f};

	menu = Menu_GetFocused();	// Get current menu

	if (!menu)
	{
		return;
	}

	// Not in upgrade mode so turn on all the force buttons, the big forceicon and description text
	if (uiInfo.forcePowerUpdated == FP_UPDATED_NONE)
	{
		// We just decremented a field so turn all buttons back on
		// Make it so all  buttons can be clicked
		for (i=0;i<MAX_POWER_ENUMS;i++)
		{
			Com_sprintf (itemName, sizeof(itemName), "%s_fbutton", powerEnums[i].title);

			UI_SetItemVisible(menu,itemName,qtrue);
		}

		UI_SetItemVisible(menu,"force_icon",qtrue);
		UI_SetItemVisible(menu,"force_desc",qtrue);
		UI_SetItemVisible(menu,"leveltext",qtrue);

		// Set focus on the top left button
		item = (itemDef_s *) Menu_FindItemByName(menu, "absorb_fbutton");
		item->window.flags |= WINDOW_HASFOCUS;

		if (item->onFocus)
		{
			Item_RunScript(item, item->onFocus);
		}

	}
	// In upgrade mode so just turn the deallocate button on
	else
	{
		UI_SetItemVisible(menu,"force_icon",qtrue);
		UI_SetItemVisible(menu,"force_desc",qtrue);
		UI_SetItemVisible(menu,"deallocate_fbutton",qtrue);

		item = (itemDef_s *) Menu_FindItemByName(menu, va("%s_fbutton",powerEnums[uiInfo.forcePowerUpdated].title));
		if (item)
		{
			item->window.flags |= WINDOW_HASFOCUS;
		}

		// Get player state
		client_t* cl = &svs.clients[0];	// 0 because only ever us as a player

		if (!cl)	// No client, get out
		{
			return;
		}

		playerState_t*		pState = cl->gentity->client;


		if (uiInfo.forcePowerUpdated == FP_UPDATED_NONE)
		{
			return;
		}

		// Update level description
		Com_sprintf (
			itemName,
			sizeof(itemName),
			"%s_level%ddesc",
			powerEnums[uiInfo.forcePowerUpdated].title,
			pState->forcePowerLevel[powerEnums[uiInfo.forcePowerUpdated].powerEnum]
			);

		item = (itemDef_s *) Menu_FindItemByName(menu, itemName);
		if (item)
		{
			item->window.flags |= WINDOW_VISIBLE;
		}
	}

	// If one was a chosen force power, high light it again.
	if (uiInfo.forcePowerUpdated>FP_UPDATED_NONE)
	{
		char itemhexName[128];
		char itemiconName[128];
		vec4_t color2 = { 1.0f, 1.0f, 1.0f, 1.0f};

		Com_sprintf (itemhexName, sizeof(itemhexName), "%s_hexpic", powerEnums[uiInfo.forcePowerUpdated].title);
		Com_sprintf (itemiconName, sizeof(itemiconName), "%s_iconpic", powerEnums[uiInfo.forcePowerUpdated].title);

		UI_SetItemColor(item,itemhexName,"forecolor",color2);
		UI_SetItemColor(item,itemiconName,"forecolor",color2);
	}
	else
	{
		// Un-grey-out all icons
		UI_SetItemColor(item,"hexpic","forecolor",color);
		UI_SetItemColor(item,"iconpic","forecolor",color);
	}
}

// Decrement force power level (Used by Force Power Allocation screen)
static void UI_DecrementCurrentForcePower ( void )
{
	menuDef_t	*menu;
	itemDef_t	*item;
	short i;
	vec4_t color = { 0.65f, 0.65f, 0.65f, 1.0f};
	char itemName[128];

	menu = Menu_GetFocused();	// Get current menu

	if (!menu)
	{
		return;
	}

	// Get player state
	client_t* cl = &svs.clients[0];	// 0 because only ever us as a player
	playerState_t*		pState = NULL;
	int forcelevel;

	if( cl )
	{
		pState = cl->gentity->client;
		forcelevel = pState->forcePowerLevel[powerEnums[uiInfo.forcePowerUpdated].powerEnum];
	}
	else
	{
		forcelevel = uiInfo.forcePowerLevel[powerEnums[uiInfo.forcePowerUpdated].powerEnum];
	}

	if (uiInfo.forcePowerUpdated == FP_UPDATED_NONE)
	{
		return;
	}

	DC->startLocalSound(uiInfo.uiDC.Assets.forceUnchosenSound, CHAN_AUTO );

	if (forcelevel>0)
	{
		if( pState )
		{
			pState->forcePowerLevel[powerEnums[uiInfo.forcePowerUpdated].powerEnum]--;	// Decrement it
			forcelevel = pState->forcePowerLevel[powerEnums[uiInfo.forcePowerUpdated].powerEnum];
			// Turn off power if level is 0
			if (pState->forcePowerLevel[powerEnums[uiInfo.forcePowerUpdated].powerEnum]<1)
			{
				pState->forcePowersKnown &= ~( 1 << powerEnums[uiInfo.forcePowerUpdated].powerEnum );
			}
		}
		else
		{
			uiInfo.forcePowerLevel[powerEnums[uiInfo.forcePowerUpdated].powerEnum]--;	// Decrement it
			forcelevel = uiInfo.forcePowerLevel[powerEnums[uiInfo.forcePowerUpdated].powerEnum];
		}
	}

	UI_SetHexPicLevel( menu,uiInfo.forcePowerUpdated,forcelevel,qfalse );

	UI_ShowForceLevelDesc ( powerEnums[uiInfo.forcePowerUpdated].title );

	// We just decremented a field so turn all buttons back on
	// Make it so all  buttons can be clicked
	for (i=0;i<MAX_POWER_ENUMS;i++)
	{
		Com_sprintf (itemName, sizeof(itemName), "%s_fbutton", powerEnums[i].title);
		item = (itemDef_s *) Menu_FindItemByName(menu, itemName);
		if (item)		// This is okay, because core powers don't have a hex button
		{
			item->window.flags |= WINDOW_VISIBLE;
		}
	}

	// Show point has not been allocated
	UI_SetPowerTitleText( qfalse);

	// Make weapons button inactive
	UI_ForcePowerWeaponsButton(qfalse);

	// Hide the deallocate button
	item = (itemDef_s *) Menu_FindItemByName(menu, "deallocate_fbutton");
	if (item)
	{
		item->window.flags &= ~WINDOW_VISIBLE;	//

		// Un-grey-out all icons
		UI_SetItemColor(item,"hexpic","forecolor",color);
		UI_SetItemColor(item,"iconpic","forecolor",color);
	}

	item = (itemDef_s *) Menu_FindItemByName(menu, va("%s_fbutton",powerEnums[uiInfo.forcePowerUpdated].title));
	if (item)
	{
		item->window.flags |= WINDOW_HASFOCUS;
	}

	uiInfo.forcePowerUpdated = FP_UPDATED_NONE;			// It's as if nothing happened.
}


void Item_MouseEnter(itemDef_t *item, float x, float y);

// Try to increment force power level (Used by Force Power Allocation screen)
static void UI_AffectForcePowerLevel ( const char *forceName )
{
	short forcePowerI=0,i;
	menuDef_t	*menu;
	itemDef_t	*item;

	menu = Menu_GetFocused();	// Get current menu

	if (!menu)
	{
		return;
	}

	if (!UI_GetForcePowerIndex ( forceName, &forcePowerI ))
	{
		return;
	}

	// Get player state
	client_t* cl = &svs.clients[0];	// 0 because only ever us as a player
	playerState_t*		pState = NULL;
	int	forcelevel;
	if( cl )
	{
		pState = cl->gentity->client;
		forcelevel = pState->forcePowerLevel[powerEnums[forcePowerI].powerEnum];
	}
	else
	{
		forcelevel = uiInfo.forcePowerLevel[powerEnums[forcePowerI].powerEnum];
	}


	if (forcelevel>2)
	{	// Too big, can't be incremented
		return;
	}

	// Increment power level.
	DC->startLocalSound(uiInfo.uiDC.Assets.forceChosenSound, CHAN_AUTO );

	uiInfo.forcePowerUpdated = forcePowerI;	// Remember which power was updated

	if( pState )
	{
		pState->forcePowerLevel[powerEnums[forcePowerI].powerEnum]++;	// Increment it
		pState->forcePowersKnown |= ( 1 << powerEnums[forcePowerI].powerEnum );
		forcelevel = pState->forcePowerLevel[powerEnums[forcePowerI].powerEnum];
	}
	else
	{
		uiInfo.forcePowerLevel[powerEnums[forcePowerI].powerEnum]++;	// Increment it
		forcelevel = uiInfo.forcePowerLevel[powerEnums[forcePowerI].powerEnum];
	}

	UI_SetHexPicLevel( menu,uiInfo.forcePowerUpdated,forcelevel,qtrue );

	UI_ShowForceLevelDesc ( forceName );

	// A field was updated, so make it so others can't be
	if (uiInfo.forcePowerUpdated>FP_UPDATED_NONE)
	{
		vec4_t color = { 0.25f, 0.25f, 0.25f, 1.0f};
		char itemName[128];

		// Make it so none of the other buttons can be clicked
		for (i=0;i<MAX_POWER_ENUMS;i++)
		{
			if (i==uiInfo.forcePowerUpdated)
			{
				continue;
			}

			Com_sprintf (itemName, sizeof(itemName), "%s_fbutton", powerEnums[i].title);
			item = (itemDef_s *) Menu_FindItemByName(menu, itemName);
			if (item)		// This is okay, because core powers don't have a hex button
			{
				item->window.flags &= ~WINDOW_VISIBLE;
			}
		}

		// Show point has been allocated
		UI_SetPowerTitleText ( qtrue );

		// Make weapons button active
		UI_ForcePowerWeaponsButton(qtrue);

		// Make user_info
		Cvar_Set ( "ui_forcepower_inc", va("%d",uiInfo.forcePowerUpdated) );

		// Just grab an item to hand it to the function.
		item = (itemDef_s *) Menu_FindItemByName(menu, "deallocate_fbutton");
		if (item)
		{
			// Show all icons as greyed-out
			UI_SetItemColor(item,"hexpic","forecolor",color);
			UI_SetItemColor(item,"iconpic","forecolor",color);

			item->window.flags |= WINDOW_HASFOCUS;
		}
	}

}

static void UI_DecrementForcePowerLevel( void )
{
	int	forcePowerI = Cvar_VariableIntegerValue( "ui_forcepower_inc" );
	// Get player state
	client_t* cl = &svs.clients[0];	// 0 because only ever us as a player

	if (!cl)	// No client, get out
	{
		return;
	}

	playerState_t*		pState = cl->gentity->client;

	pState->forcePowerLevel[powerEnums[forcePowerI].powerEnum]--;	// Decrement it

}

// Show force level description that matches current player level (Used by Force Power Allocation screen)
static void UI_ShowForceLevelDesc ( const char *forceName )
{
	short forcePowerI=0;
	menuDef_t	*menu;
	itemDef_t	*item;
	menu = Menu_GetFocused();	// Get current menu

	if (!menu)
	{
		return;
	}


	if (!UI_GetForcePowerIndex ( forceName, &forcePowerI ))
	{
		return;
	}

	// Get player state
	client_t* cl = &svs.clients[0];	// 0 because only ever us as a player

	if (!cl)	// No client, get out
	{
		return;
	}
	playerState_t*		pState = cl->gentity->client;

	char itemName[128];

	// Update level description
	Com_sprintf (
		itemName,
		sizeof(itemName),
		"%s_level%ddesc",
		powerEnums[forcePowerI].title,
		pState->forcePowerLevel[powerEnums[forcePowerI].powerEnum]
		);

	item = (itemDef_s *) Menu_FindItemByName(menu, itemName);
	if (item)
	{
		item->window.flags |= WINDOW_VISIBLE;
	}

}

// Reset force level powers screen to what it was before player upgraded them (Used by Force Power Allocation screen)
static void UI_ResetForceLevels ( void )
{

	// What force ppower had the point added to it?
	if (uiInfo.forcePowerUpdated!=FP_UPDATED_NONE)
	{
		// Get player state
		client_t* cl = &svs.clients[0];	// 0 because only ever us as a player

		if (!cl)	// No client, get out
		{
			return;
		}
		playerState_t*		pState = cl->gentity->client;

		// Decrement that power
		pState->forcePowerLevel[powerEnums[uiInfo.forcePowerUpdated].powerEnum]--;

		menuDef_t	*menu;
		itemDef_t	*item;

		menu = Menu_GetFocused();	// Get current menu

		if (!menu)
		{
			return;
		}

		int i;
		char itemName[128];

		// Make it so all  buttons can be clicked
		for (i=0;i<MAX_POWER_ENUMS;i++)
		{
			Com_sprintf (itemName, sizeof(itemName), "%s_fbutton", powerEnums[i].title);
			item = (itemDef_s *) Menu_FindItemByName(menu, itemName);
			if (item)		// This is okay, because core powers don't have a hex button
			{
				item->window.flags |= WINDOW_VISIBLE;
			}
		}

		UI_SetPowerTitleText( qfalse );

		Com_sprintf (itemName, sizeof(itemName), "%s_fbutton", powerEnums[uiInfo.forcePowerUpdated].title);
		item = (itemDef_s *) Menu_FindItemByName(menu, itemName);
		if (item)
		{
			// Change description text to tell player they can increment the force point
			item->descText = "@MENUS_ADDFP";
		}

		uiInfo.forcePowerUpdated = FP_UPDATED_NONE;
	}

	UI_ForcePowerWeaponsButton(qfalse);
}


#ifndef JK2_MODE
// Set the Players known saber style
static void UI_UpdateFightingStyle ( void )
{
	playerState_t	*pState;
	int				fightingStyle,saberStyle;


	fightingStyle = Cvar_VariableIntegerValue( "ui_newfightingstyle" );

	if (fightingStyle == 1)
	{
		saberStyle = SS_MEDIUM;
	}
	else if (fightingStyle == 2)
	{
		saberStyle = SS_STRONG;
	}
	else if (fightingStyle == 3)
	{
		saberStyle = SS_DUAL;
	}
	else if (fightingStyle == 4)
	{
		saberStyle = SS_STAFF;
	}
	else // 0 is Fast
	{
		saberStyle = SS_FAST;
	}

	// Get player state
	client_t	*cl = &svs.clients[0];	// 0 because only ever us as a player

	// No client, get out
	if (cl && cl->gentity && cl->gentity->client)
	{
		pState = cl->gentity->client;
		pState->saberStylesKnown |= (1<<saberStyle);
	}
	else	// Must be at the beginning of the game so the client hasn't been created, shove data in a cvar
	{
		Cvar_Set ( "g_fighting_style", va("%d",saberStyle) );
	}
}
#endif // !JK2_MODE

static void UI_ResetCharacterListBoxes( void )
{

	itemDef_t *item;
	menuDef_t *menu;
	listBoxDef_t *listPtr;

	menu = Menus_FindByName("characterMenu");


	if (menu)
	{
		item = (itemDef_s *) Menu_FindItemByName((menuDef_t *) menu, "headlistbox");
		if (item)
		{
			listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;
			if( listPtr )
			{
				listPtr->cursorPos = 0;
			}
			item->cursorPos = 0;
		}

		item = (itemDef_s *) Menu_FindItemByName((menuDef_t *) menu, "torsolistbox");
		if (item)
		{
			listPtr = (listBoxDef_t*)item->typeData;
			if( listPtr )
			{
				listPtr->cursorPos = 0;
			}
			item->cursorPos = 0;
		}

		item = (itemDef_s *) Menu_FindItemByName((menuDef_t *) menu, "lowerlistbox");
		if (item)
		{
			listPtr = (listBoxDef_t*)item->typeData;
			if( listPtr )
			{
				listPtr->cursorPos = 0;
			}
			item->cursorPos = 0;
		}

		item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "colorbox");
		if (item)
		{
			listPtr = (listBoxDef_t*)item->typeData;
			if( listPtr )
			{
				listPtr->cursorPos = 0;
			}
			item->cursorPos = 0;
		}
	}
}

static void UI_ClearInventory ( void )
{
	// Get player state
	client_t* cl = &svs.clients[0];	// 0 because only ever us as a player

	if (!cl)	// No client, get out
	{
		return;
	}

	if (cl->gentity && cl->gentity->client)
	{
		playerState_t*		pState = cl->gentity->client;

		// Clear out inventory for the player
		int i;
		for (i=0;i<MAX_INVENTORY;i++)
		{
			pState->inventory[i] = 0;
		}
	}
}

static void UI_GiveInventory ( const int itemIndex, const int amount )
{
	// Get player state
	client_t* cl = &svs.clients[0];	// 0 because only ever us as a player

	if (!cl)	// No client, get out
	{
		return;
	}

	if (cl->gentity && cl->gentity->client)
	{
		playerState_t*	pState = cl->gentity->client;

		if (itemIndex < MAX_INVENTORY)
		{
			pState->inventory[itemIndex]=amount;
		}
	}

}

//. Find weapons allocation screen BEGIN button and make active/inactive
static void UI_WeaponAllocBeginButton(qboolean activeFlag)
{
	menuDef_t	*menu;
	menu = Menu_GetFocused();	// Get current menu

	if (!menu)
	{
		return;
	}

	int weap =	Cvar_VariableIntegerValue( "weapon_menu" );

	// Find begin button
	itemDef_t	*item;
	item = Menu_GetMatchingItemByNumber(menu, weap, "beginmission");

	if (item)
	{
		// Make it active
		if (activeFlag)
		{
			item->window.flags &= ~WINDOW_INACTIVE;
		}
		else
		{
			item->window.flags |= WINDOW_INACTIVE;
		}
	}
}

// If we have both weapons and the throwable weapon, turn on the begin mission button,
// otherwise, turn it off
static void UI_WeaponsSelectionsComplete( void )
{
	// We need two weapons and one throwable
	if (( uiInfo.selectedWeapon1 != NOWEAPON ) &&
		( uiInfo.selectedWeapon2 != NOWEAPON ) &&
		( uiInfo.selectedThrowWeapon != NOWEAPON ))
	{
		UI_WeaponAllocBeginButton(qtrue);	// Turn it on
	}
	else
	{
		UI_WeaponAllocBeginButton(qfalse);	// Turn it off
	}
}

// if this is the first time into the weapon allocation screen, show the INSTRUCTION screen
static void	UI_WeaponHelpActive( void )
{
	int	tier_storyinfo = Cvar_VariableIntegerValue( "tier_storyinfo" );
	menuDef_t	*menu;

	menu = Menu_GetFocused();	// Get current menu

	if (!menu)
	{
		return;
	}

	// First time, show instructions
	if (tier_storyinfo==1)
	{
		UI_SetItemVisible(menu,"weapon_button",qfalse);

		UI_SetItemVisible(menu,"inst_stuff",qtrue);

	}
	// just act like normal
	else
	{
		UI_SetItemVisible(menu,"weapon_button",qtrue);

		UI_SetItemVisible(menu,"inst_stuff",qfalse);
	}
}

static void UI_InitWeaponSelect( void )
{
	UI_WeaponAllocBeginButton(qfalse);
	uiInfo.selectedWeapon1 = NOWEAPON;
	uiInfo.selectedWeapon2 = NOWEAPON;
	uiInfo.selectedThrowWeapon = NOWEAPON;
}

static void UI_ClearWeapons ( void )
{
	// Get player state
	client_t* cl = &svs.clients[0];	// 0 because only ever us as a player

	if (!cl)	// No client, get out
	{
		return;
	}

	if (cl->gentity && cl->gentity->client)
	{
		playerState_t*		pState = cl->gentity->client;

		// Clear out any weapons for the player
		pState->stats[ STAT_WEAPONS ] = 0;

		pState->weapon = WP_NONE;

	}

}

static void UI_GiveWeapon ( const int weaponIndex )
{
	// Get player state
	client_t* cl = &svs.clients[0];	// 0 because only ever us as a player

	if (!cl)	// No client, get out
	{
		return;
	}

	if (cl->gentity && cl->gentity->client)
	{
		playerState_t*	pState = cl->gentity->client;

		if (weaponIndex<WP_NUM_WEAPONS)
		{
			pState->stats[ STAT_WEAPONS ] |= ( 1 << weaponIndex );
		}
	}
}

static void UI_EquipWeapon ( const int weaponIndex )
{
	// Get player state
	client_t* cl = &svs.clients[0];	// 0 because only ever us as a player

	if (!cl)	// No client, get out
	{
		return;
	}

	if (cl->gentity && cl->gentity->client)
	{
		playerState_t*	pState = cl->gentity->client;

		if (weaponIndex<WP_NUM_WEAPONS)
		{
			pState->weapon = weaponIndex;
			//force it to change
			//CG_ChangeWeapon( wp );
		}
	}
}

static void	UI_LoadMissionSelectMenu( const char *cvarName )
{
	int holdLevel = (int)trap_Cvar_VariableValue(cvarName);

	// Figure out which tier menu to load
	if ((holdLevel > 0) && (holdLevel < 5))
	{
		UI_LoadMenus("ui/tier1.txt",qfalse);

		Menus_CloseByName("ingameMissionSelect1");
	}
	else if ((holdLevel > 6) && (holdLevel < 10))
	{
		UI_LoadMenus("ui/tier2.txt",qfalse);

		Menus_CloseByName("ingameMissionSelect2");
	}
	else if ((holdLevel > 11) && (holdLevel < 15))
	{
		UI_LoadMenus("ui/tier3.txt",qfalse);

		Menus_CloseByName("ingameMissionSelect3");
	}

}

// Update the player weapons with the chosen weapon
static void	UI_AddWeaponSelection ( const int weaponIndex, const int ammoIndex, const int ammoAmount, const char *iconItemName,const char *litIconItemName, const char *hexBackground, const char *soundfile )
{
	itemDef_s  *item, *iconItem,*litIconItem;
	menuDef_t	*menu;

	menu = Menu_GetFocused();	// Get current menu

	if (!menu)
	{
		return;
	}

	iconItem = (itemDef_s *) Menu_FindItemByName(menu, iconItemName );
	litIconItem = (itemDef_s *) Menu_FindItemByName(menu, litIconItemName );

	const char *chosenItemName, *chosenButtonName;

	// has this weapon already been chosen?
	if (weaponIndex == uiInfo.selectedWeapon1)
	{
		UI_RemoveWeaponSelection ( 1 );
		return;
	}
	else if (weaponIndex == uiInfo.selectedWeapon2)
	{
		UI_RemoveWeaponSelection ( 2 );
		return;
	}

	// See if either slot is empty
	if ( uiInfo.selectedWeapon1 == NOWEAPON )
	{
		chosenItemName = "chosenweapon1_icon";
		chosenButtonName = "chosenweapon1_button";
		uiInfo.selectedWeapon1 = weaponIndex;
		uiInfo.selectedWeapon1AmmoIndex = ammoIndex;

		memcpy( uiInfo.selectedWeapon1ItemName,hexBackground,sizeof(uiInfo.selectedWeapon1ItemName));

		//Save the lit and unlit icons for the selected weapon slot
		uiInfo.litWeapon1Icon = litIconItem->window.background;
		uiInfo.unlitWeapon1Icon = iconItem->window.background;

		uiInfo.weapon1ItemButton = uiInfo.runScriptItem;
		uiInfo.weapon1ItemButton->descText = "@MENUS_CLICKREMOVE";
	}
	else if ( uiInfo.selectedWeapon2 == NOWEAPON )
	{
		chosenItemName = "chosenweapon2_icon";
		chosenButtonName = "chosenweapon2_button";
		uiInfo.selectedWeapon2 = weaponIndex;
		uiInfo.selectedWeapon2AmmoIndex = ammoIndex;

		memcpy( uiInfo.selectedWeapon2ItemName,hexBackground,sizeof(uiInfo.selectedWeapon2ItemName));

		//Save the lit and unlit icons for the selected weapon slot
		uiInfo.litWeapon2Icon = litIconItem->window.background;
		uiInfo.unlitWeapon2Icon = iconItem->window.background;

		uiInfo.weapon2ItemButton = uiInfo.runScriptItem;
		uiInfo.weapon2ItemButton->descText = "@MENUS_CLICKREMOVE";
	}
	else	// Both slots are used, can't add it.
	{
		return;
	}

	item = (itemDef_s *) Menu_FindItemByName(menu, chosenItemName );
	if ((item) && (iconItem))
	{
		item->window.background = iconItem->window.background;
		item->window.flags |= WINDOW_VISIBLE;
	}

	// Turn on chosenweapon button so player can unchoose the weapon
	item = (itemDef_s *) Menu_FindItemByName(menu, chosenButtonName );
	if (item)
	{
		item->window.background = iconItem->window.background;
		item->window.flags |= WINDOW_VISIBLE;
	}

	// Switch hex background to be 'on'
	item = (itemDef_s *) Menu_FindItemByName(menu, hexBackground );
	if (item)
	{
		item->window.foreColor[0] = 0;
		item->window.foreColor[1] = 1;
		item->window.foreColor[2] = 0;
		item->window.foreColor[3] = 1;

	}

	// Get player state
	client_t* cl = &svs.clients[0];	// 0 because only ever us as a player

	// NOTE : this UIScript can now be run from outside the game, so don't
	// return out here, just skip this part
	if (cl)
	{
		// Add weapon
		if (cl->gentity && cl->gentity->client)
		{
			playerState_t*	pState = cl->gentity->client;

			if ((weaponIndex>0) && (weaponIndex<WP_NUM_WEAPONS))
			{
				pState->stats[ STAT_WEAPONS ] |= ( 1 << weaponIndex );
			}

			// Give them ammo too
			if ((ammoIndex>0) && (ammoIndex<AMMO_MAX))
			{
				pState->ammo[ ammoIndex ] = ammoAmount;
			}
		}
	}

	if( soundfile )
	{
		DC->startLocalSound(DC->registerSound(soundfile, qfalse), CHAN_LOCAL );
	}

	UI_WeaponsSelectionsComplete();	// Test to see if the mission begin button should turn on or off


}


// Update the player weapons with the chosen weapon
static void UI_RemoveWeaponSelection ( const int weaponSelectionIndex )
{
	itemDef_s  *item;
	menuDef_t	*menu;
	const char *chosenItemName, *chosenButtonName,*background;
	int		ammoIndex,weaponIndex;

	menu = Menu_GetFocused();	// Get current menu

	// Which item has it?
	if ( weaponSelectionIndex == 1 )
	{
		chosenItemName = "chosenweapon1_icon";
		chosenButtonName = "chosenweapon1_button";
		background = uiInfo.selectedWeapon1ItemName;
		ammoIndex = uiInfo.selectedWeapon1AmmoIndex;
		weaponIndex = uiInfo.selectedWeapon1;

		if (uiInfo.weapon1ItemButton)
		{
			uiInfo.weapon1ItemButton->descText = "@MENUS_CLICKSELECT";
			uiInfo.weapon1ItemButton = NULL;
		}
	}
	else if ( weaponSelectionIndex == 2 )
	{
		chosenItemName = "chosenweapon2_icon";
		chosenButtonName = "chosenweapon2_button";
		background = uiInfo.selectedWeapon2ItemName;
		ammoIndex = uiInfo.selectedWeapon2AmmoIndex;
		weaponIndex = uiInfo.selectedWeapon2;

		if (uiInfo.weapon2ItemButton)
		{
			uiInfo.weapon2ItemButton->descText = "@MENUS_CLICKSELECT";
			uiInfo.weapon2ItemButton = NULL;
		}
	}
	else
	{
		return;
	}

	// Reset background of upper icon
	item = (itemDef_s *) Menu_FindItemByName( menu, background );
	if ( item )
	{
		item->window.foreColor[0] = 0.0f;
		item->window.foreColor[1] = 0.5f;
		item->window.foreColor[2] = 0.0f;
		item->window.foreColor[3] = 1.0f;
	}

	// Hide it icon
	item = (itemDef_s *) Menu_FindItemByName( menu, chosenItemName );
	if ( item )
	{
		item->window.flags &= ~WINDOW_VISIBLE;
	}

	// Hide button
	item = (itemDef_s *) Menu_FindItemByName( menu, chosenButtonName );
	if ( item )
	{
		item->window.flags &= ~WINDOW_VISIBLE;
	}

	// Get player state
	client_t* cl = &svs.clients[0];	// 0 because only ever us as a player

	// NOTE : this UIScript can now be run from outside the game, so don't
	// return out here, just skip this part
	if (cl)	// No client, get out
	{

		// Remove weapon
		if (cl->gentity && cl->gentity->client)
		{
			playerState_t*	pState = cl->gentity->client;

			if ((weaponIndex>0) && (weaponIndex<WP_NUM_WEAPONS))
			{
				pState->stats[ STAT_WEAPONS ]  &= ~( 1 << weaponIndex );
			}

			// Remove ammo too
			if ((ammoIndex>0) && (ammoIndex<AMMO_MAX))
			{	// But don't take it away if the other weapon is using that ammo
				if ( uiInfo.selectedWeapon1AmmoIndex != uiInfo.selectedWeapon2AmmoIndex )
				{
					pState->ammo[ ammoIndex ] = 0;
				}
			}
		}

	}

	// Now do a little clean up
	if ( weaponSelectionIndex == 1 )
	{
		uiInfo.selectedWeapon1 = NOWEAPON;
		memset(uiInfo.selectedWeapon1ItemName,0,sizeof(uiInfo.selectedWeapon1ItemName));
		uiInfo.selectedWeapon1AmmoIndex = 0;
	}
	else if ( weaponSelectionIndex == 2 )
	{
		uiInfo.selectedWeapon2 = NOWEAPON;
		memset(uiInfo.selectedWeapon2ItemName,0,sizeof(uiInfo.selectedWeapon2ItemName));
		uiInfo.selectedWeapon2AmmoIndex = 0;
	}

#ifndef JK2_MODE
	//FIXME hack to prevent error in jk2 by disabling
	DC->startLocalSound(DC->registerSound("sound/interface/weapon_deselect.mp3", qfalse), CHAN_LOCAL );
#endif

	UI_WeaponsSelectionsComplete();	// Test to see if the mission begin button should turn on or off


}

static void	UI_NormalWeaponSelection ( const int selectionslot )
{
	itemDef_s  *item;
	menuDef_t	*menu;

	menu = Menu_GetFocused();	// Get current menu
	if (!menu)
	{
		return;
	}

	if (selectionslot == 1)
	{
		item = (itemDef_s *) Menu_FindItemByName( menu, "chosenweapon1_icon" );
		if (item)
		{
			item->window.background = uiInfo.unlitWeapon1Icon;
		}
	}

	if (selectionslot == 2)
	{
		item = (itemDef_s *) Menu_FindItemByName( menu, "chosenweapon2_icon" );
		if (item)
		{
			item->window.background = uiInfo.unlitWeapon2Icon;
		}
	}
}

static void	UI_HighLightWeaponSelection ( const int selectionslot )
{
	itemDef_s  *item;
	menuDef_t	*menu;

	menu = Menu_GetFocused();	// Get current menu
	if (!menu)
	{
		return;
	}

	if (selectionslot == 1)
	{
		item = (itemDef_s *) Menu_FindItemByName( menu, "chosenweapon1_icon" );
		if (item)
		{
			item->window.background = uiInfo.litWeapon1Icon;
		}
	}

	if (selectionslot == 2)
	{
		item = (itemDef_s *) Menu_FindItemByName( menu, "chosenweapon2_icon" );
		if (item)
		{
			item->window.background = uiInfo.litWeapon2Icon;
		}
	}
}

// Update the player throwable weapons (okay it's a bad description) with the chosen weapon
static void	UI_AddThrowWeaponSelection ( const int weaponIndex, const int ammoIndex, const int ammoAmount, const char *iconItemName,const char *litIconItemName, const char *hexBackground, const char *soundfile )
{
	itemDef_s  *item, *iconItem,*litIconItem;
	menuDef_t	*menu;

	menu = Menu_GetFocused();	// Get current menu

	if (!menu)
	{
		return;
	}

	iconItem = (itemDef_s *) Menu_FindItemByName(menu, iconItemName );
	litIconItem = (itemDef_s *) Menu_FindItemByName(menu, litIconItemName );

	const char *chosenItemName, *chosenButtonName;

	// Has a throw weapon already been chosen?
	if (uiInfo.selectedThrowWeapon!=NOWEAPON)
	{
		// Clicked on the selected throwable weapon
		if (uiInfo.selectedThrowWeapon==weaponIndex)
		{	// Deselect it
			UI_RemoveThrowWeaponSelection();
		}
		return;
	}

	chosenItemName = "chosenthrowweapon_icon";
	chosenButtonName = "chosenthrowweapon_button";
	uiInfo.selectedThrowWeapon = weaponIndex;
	uiInfo.selectedThrowWeaponAmmoIndex = ammoIndex;
	uiInfo.weaponThrowButton = uiInfo.runScriptItem;

	if (uiInfo.weaponThrowButton)
	{
		uiInfo.weaponThrowButton->descText = "@MENUS_CLICKREMOVE";
	}

	memcpy( uiInfo.selectedThrowWeaponItemName,hexBackground,sizeof(uiInfo.selectedWeapon1ItemName));

	//Save the lit and unlit icons for the selected weapon slot
	uiInfo.litThrowableIcon = litIconItem->window.background;
	uiInfo.unlitThrowableIcon = iconItem->window.background;

	item = (itemDef_s *) Menu_FindItemByName(menu, chosenItemName );
	if ((item) && (iconItem))
	{
		item->window.background = iconItem->window.background;
		item->window.flags |= WINDOW_VISIBLE;
	}

	// Turn on throwchosenweapon button so player can unchoose the weapon
	item = (itemDef_s *) Menu_FindItemByName(menu, chosenButtonName );
	if (item)
	{
		item->window.background = iconItem->window.background;
		item->window.flags |= WINDOW_VISIBLE;
	}

	// Switch hex background to be 'on'
	item = (itemDef_s *) Menu_FindItemByName(menu, hexBackground );
	if (item)
	{
		item->window.foreColor[0] = 0.0f;
		item->window.foreColor[1] = 0.0f;
		item->window.foreColor[2] = 1.0f;
		item->window.foreColor[3] = 1.0f;

	}

	// Get player state

	client_t* cl = &svs.clients[0];	// 0 because only ever us as a player

	// NOTE : this UIScript can now be run from outside the game, so don't
	// return out here, just skip this part
	if (cl)	// No client, get out
	{
		// Add weapon
		if (cl->gentity && cl->gentity->client)
		{
			playerState_t*	pState = cl->gentity->client;

			if ((weaponIndex>0) && (weaponIndex<WP_NUM_WEAPONS))
			{
				pState->stats[ STAT_WEAPONS ] |= ( 1 << weaponIndex );
			}

			// Give them ammo too
			if ((ammoIndex>0) && (ammoIndex<AMMO_MAX))
			{
				pState->ammo[ ammoIndex ] = ammoAmount;
			}
		}
	}

	if( soundfile )
	{
		DC->startLocalSound(DC->registerSound(soundfile, qfalse), CHAN_LOCAL );
	}

	UI_WeaponsSelectionsComplete();	// Test to see if the mission begin button should turn on or off

}


// Update the player weapons with the chosen throw weapon
static void UI_RemoveThrowWeaponSelection ( void )
{
	itemDef_s  *item;
	menuDef_t	*menu;
	const char *chosenItemName, *chosenButtonName,*background;

	menu = Menu_GetFocused();	// Get current menu

	// Weapon not chosen
	if ( uiInfo.selectedThrowWeapon == NOWEAPON )
	{
		return;
	}

	chosenItemName = "chosenthrowweapon_icon";
	chosenButtonName = "chosenthrowweapon_button";
	background = uiInfo.selectedThrowWeaponItemName;

	// Reset background of upper icon
	item = (itemDef_s *) Menu_FindItemByName( menu, background );
	if ( item )
	{
		item->window.foreColor[0] = 0.0f;
		item->window.foreColor[1] = 0.0f;
		item->window.foreColor[2] = 0.5f;
		item->window.foreColor[3] = 1.0f;
	}

	// Hide it icon
	item = (itemDef_s *) Menu_FindItemByName( menu, chosenItemName );
	if ( item )
	{
		item->window.flags &= ~WINDOW_VISIBLE;
	}

	// Hide button
	item = (itemDef_s *) Menu_FindItemByName( menu, chosenButtonName );
	if ( item )
	{
		item->window.flags &= ~WINDOW_VISIBLE;
	}

	// Get player state

	client_t* cl = &svs.clients[0];	// 0 because only ever us as a player

	// NOTE : this UIScript can now be run from outside the game, so don't
	// return out here, just skip this part
	if (cl)	// No client, get out
	{
		// Remove weapon
		if (cl->gentity && cl->gentity->client)
		{
			playerState_t*	pState = cl->gentity->client;

			if ((uiInfo.selectedThrowWeapon>0) && (uiInfo.selectedThrowWeapon<WP_NUM_WEAPONS))
			{
				pState->stats[ STAT_WEAPONS ]  &= ~( 1 << uiInfo.selectedThrowWeapon );
			}

			// Remove ammo too
			if ((uiInfo.selectedThrowWeaponAmmoIndex>0) && (uiInfo.selectedThrowWeaponAmmoIndex<AMMO_MAX))
			{
				pState->ammo[ uiInfo.selectedThrowWeaponAmmoIndex ] = 0;
			}

		}
	}

	// Now do a little clean up
	uiInfo.selectedThrowWeapon = NOWEAPON;
	memset(uiInfo.selectedThrowWeaponItemName,0,sizeof(uiInfo.selectedThrowWeaponItemName));
	uiInfo.selectedThrowWeaponAmmoIndex = 0;

	if (uiInfo.weaponThrowButton)
	{
		uiInfo.weaponThrowButton->descText = "@MENUS_CLICKSELECT";
		uiInfo.weaponThrowButton = NULL;
	}

#ifndef JK2_MODE
	//FIXME hack to prevent error in jk2 by disabling
	DC->startLocalSound(DC->registerSound("sound/interface/weapon_deselect.mp3", qfalse), CHAN_LOCAL );
#endif

	UI_WeaponsSelectionsComplete();	// Test to see if the mission begin button should turn on or off

}

static void	UI_NormalThrowSelection ( void )
{
	itemDef_s  *item;
	menuDef_t	*menu;

	menu = Menu_GetFocused();	// Get current menu
	if (!menu)
	{
		return;
	}

	item = (itemDef_s *) Menu_FindItemByName( menu, "chosenthrowweapon_icon" );
	item->window.background = uiInfo.unlitThrowableIcon;
}

static void	UI_HighLightThrowSelection ( void )
{
	itemDef_s  *item;
	menuDef_t	*menu;

	menu = Menu_GetFocused();	// Get current menu
	if (!menu)
	{
		return;
	}

	item = (itemDef_s *) Menu_FindItemByName( menu, "chosenthrowweapon_icon" );
	item->window.background = uiInfo.litThrowableIcon;
}

static void UI_GetSaberCvars ( void )
{
	Cvar_Set ( "ui_saber_type", Cvar_VariableString ( "g_saber_type" ) );
	Cvar_Set ( "ui_saber", Cvar_VariableString ( "g_saber" ) );
	Cvar_Set ( "ui_saber2", Cvar_VariableString ( "g_saber2" ) );
	Cvar_Set ( "ui_saber_color", Cvar_VariableString ( "g_saber_color" ) );
	Cvar_Set ( "ui_saber2_color", Cvar_VariableString ( "g_saber2_color" ) );

	Cvar_Set ( "ui_newfightingstyle", "0");

}

static void UI_ResetSaberCvars ( void )
{
	Cvar_Set ( "g_saber_type", "single" );
	Cvar_Set ( "g_saber", "single_1" );
	Cvar_Set ( "g_saber2", "" );

	Cvar_Set ( "ui_saber_type", "single" );
	Cvar_Set ( "ui_saber", "single_1" );
	Cvar_Set ( "ui_saber2", "" );
}

extern qboolean ItemParse_asset_model_go( itemDef_t *item, const char *name );
extern qboolean ItemParse_model_g2skin_go( itemDef_t *item, const char *skinName );
static void UI_UpdateCharacterSkin( void )
{
	menuDef_t *menu;
	itemDef_t *item;
	char skin[MAX_QPATH];

	menu = Menu_GetFocused();	// Get current menu

	if (!menu)
	{
		return;
	}

	item = (itemDef_s *) Menu_FindItemByName(menu, "character");

	if (!item)
	{
		Com_Error( ERR_FATAL, "UI_UpdateCharacterSkin: Could not find item (character) in menu (%s)", menu->window.name);
	}

	Com_sprintf( skin, sizeof( skin ), "models/players/%s/|%s|%s|%s",
										Cvar_VariableString ( "ui_char_model"),
										Cvar_VariableString ( "ui_char_skin_head"),
										Cvar_VariableString ( "ui_char_skin_torso"),
										Cvar_VariableString ( "ui_char_skin_legs")
				);

	ItemParse_model_g2skin_go( item, skin );
}

static void UI_UpdateCharacter( qboolean changedModel )
{
	menuDef_t *menu;
	itemDef_t *item;
	char modelPath[MAX_QPATH];

	menu = Menu_GetFocused();	// Get current menu

	if (!menu)
	{
		return;
	}

	item = (itemDef_s *) Menu_FindItemByName(menu, "character");

	if (!item)
	{
		Com_Error( ERR_FATAL, "UI_UpdateCharacter: Could not find item (character) in menu (%s)", menu->window.name);
	}

	ItemParse_model_g2anim_go( item, ui_char_anim.string );

	Com_sprintf( modelPath, sizeof( modelPath ), "models/players/%s/model.glm", Cvar_VariableString ( "ui_char_model" ) );
	ItemParse_asset_model_go( item, modelPath );

	if ( changedModel )
	{//set all skins to first skin since we don't know you always have all skins
		//FIXME: could try to keep the same spot in each list as you swtich models
		UI_FeederSelection(FEEDER_PLAYER_SKIN_HEAD, 0, item);	//fixme, this is not really the right item!!
		UI_FeederSelection(FEEDER_PLAYER_SKIN_TORSO, 0, item);
		UI_FeederSelection(FEEDER_PLAYER_SKIN_LEGS, 0, item);
		UI_FeederSelection(FEEDER_COLORCHOICES, 0, item);
	}
	UI_UpdateCharacterSkin();
}

void UI_UpdateSaberType( void )
{
	char sType[MAX_QPATH];
	DC->getCVarString( "ui_saber_type", sType, sizeof(sType) );
	if ( Q_stricmp( "single", sType ) == 0 ||
		Q_stricmp( "staff", sType ) == 0 )
	{
		DC->setCVar( "ui_saber2", "" );
	}
}

static void UI_UpdateSaberHilt( qboolean secondSaber )
{
	menuDef_t *menu;
	itemDef_t *item;
	char model[MAX_QPATH];
	char modelPath[MAX_QPATH];
	char skinPath[MAX_QPATH];
	menu = Menu_GetFocused();	// Get current menu (either video or ingame video, I would assume)

	if (!menu)
	{
		return;
	}

	const char *itemName;
	const char *saberCvarName;
	if ( secondSaber )
	{
		itemName = "saber2";
		saberCvarName = "ui_saber2";
	}
	else
	{
		itemName = "saber";
		saberCvarName = "ui_saber";
	}

	item = (itemDef_s *) Menu_FindItemByName(menu, itemName );

	if(!item)
	{
		Com_Error( ERR_FATAL, "UI_UpdateSaberHilt: Could not find item (%s) in menu (%s)", itemName, menu->window.name);
	}
	DC->getCVarString( saberCvarName, model, sizeof(model) );
	//read this from the sabers.cfg
	if ( UI_SaberModelForSaber( model, modelPath ) )
	{//successfully found a model
		ItemParse_asset_model_go( item, modelPath );//set the model
		//get the customSkin, if any
		//COM_StripExtension( modelPath, skinPath, sizeof(skinPath) );
		//COM_DefaultExtension( skinPath, sizeof( skinPath ), ".skin" );
		if ( UI_SaberSkinForSaber( model, skinPath ) )
		{
			ItemParse_model_g2skin_go( item, skinPath );//apply the skin
		}
		else
		{
			ItemParse_model_g2skin_go( item, NULL );//apply the skin
		}
	}
}

/*
static void UI_UpdateSaberColor( qboolean secondSaber )
{
	int sabernumber;
	if (secondSaber)
		sabernumber = 2;
	else
		sabernumber = 1;

	ui.Cmd_ExecuteText( EXEC_APPEND, va("sabercolor %i %s\n",sabernumber, Cvar_VariableString("g_saber_color")));
}
*/
char GoToMenu[1024];

/*
=================
Menus_SaveGoToMenu
=================
*/
void Menus_SaveGoToMenu(const char *menuTo)
{
	memcpy(GoToMenu, menuTo, sizeof(GoToMenu));
}

/*
=================
UI_CheckVid1Data
=================
*/
void UI_CheckVid1Data(const char *menuTo,const char *warningMenuName)
{
	menuDef_t *menu;
	itemDef_t *applyChanges;

	menu = Menu_GetFocused();	// Get current menu (either video or ingame video, I would assume)

	if (!menu)
	{
		Com_Printf(S_COLOR_YELLOW"WARNING: No videoMenu was found. Video data could not be checked\n");
		return;
	}

	applyChanges = (itemDef_s *) Menu_FindItemByName(menu, "applyChanges");

	if (!applyChanges)
	{
//		Menus_CloseAll();
		Menus_OpenByName(menuTo);
		return;
	}

	if ((applyChanges->window.flags & WINDOW_VISIBLE))	// Is the APPLY CHANGES button active?
	{
//		Menus_SaveGoToMenu(menuTo);							// Save menu you're going to
//		Menus_HideItems(menu->window.name);					// HIDE videMenu in case you have to come back
		Menus_OpenByName(warningMenuName);				// Give warning
	}
	else
	{
//		Menus_CloseAll();
//		Menus_OpenByName(menuTo);
	}
}

/*
=================
UI_ResetDefaults
=================
*/
void UI_ResetDefaults( void )
{
	ui.Cmd_ExecuteText( EXEC_APPEND, "cvar_restart\n");
	Controls_SetDefaults();
	ui.Cmd_ExecuteText( EXEC_APPEND, "exec default.cfg\n");
	ui.Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );
}

/*
=======================
UI_SortSaveGames
=======================
*/
static int UI_SortSaveGames( const void *A, const void *B )
{

	const int &a = ((savedata_t*)A)->currentSaveFileDateTime;
	const int &b = ((savedata_t*)B)->currentSaveFileDateTime;

	if (a > b)
	{
		return -1;
	}
	else
	{
		return (a < b);
	}
}

/*
=======================
UI_AdjustSaveGameListBox
=======================
*/
// Yeah I could get fired for this... in a world of good and bad, this is bad
// I wish we passed in the menu item to RunScript(), oh well...
void UI_AdjustSaveGameListBox( int currentLine )
{
	menuDef_t *menu;
	itemDef_t *item;

	// could be in either the ingame or shell load menu (I know, I know it's bad)
	menu = Menus_FindByName("loadgameMenu");
	if( !menu )
	{
		menu = Menus_FindByName("ingameloadMenu");
	}

	if (menu)
	{
		item = (itemDef_s *) Menu_FindItemByName((menuDef_t *) menu, "loadgamelist");
		if (item)
		{
			listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;
			if( listPtr )
			{
				listPtr->cursorPos = currentLine;
			}

			item->cursorPos = currentLine;
		}
	}

}

/*
=================
ReadSaveDirectory
=================
*/
//JLFSAVEGAME MPNOTUSED
void ReadSaveDirectory (void)
{
	int		i;
	char	*holdChar;
	int		len;
	int		fileCnt;
	// Clear out save data
	memset(s_savedata,0,sizeof(s_savedata));
	s_savegame.saveFileCnt = 0;
	Cvar_Set("ui_gameDesc", "" );	// Blank out comment
	Cvar_Set("ui_SelectionOK", "0" );
#ifdef JK2_MODE
	memset( screenShotBuf,0,(SG_SCR_WIDTH * SG_SCR_HEIGHT * 4)); //blank out sshot
#endif


	// Get everything in saves directory
	fileCnt = ui.FS_GetFileList("saves", ".sav", s_savegame.listBuf, LISTBUFSIZE );

	Cvar_Set("ui_ResumeOK", "0" );
	holdChar = s_savegame.listBuf;
	for ( i = 0; i < fileCnt; i++ )
	{
		// strip extension
		len = strlen( holdChar );
		holdChar[len-4] = '\0';

		if	( Q_stricmp("current",holdChar)!=0 )
		{
			time_t result;
			if (Q_stricmp("auto",holdChar)==0)
			{
				Cvar_Set("ui_ResumeOK", "1" );
			}
			else
			{	// Is this a valid file??? & Get comment of file
				result = ui.SG_GetSaveGameComment(holdChar, s_savedata[s_savegame.saveFileCnt].currentSaveFileComments, s_savedata[s_savegame.saveFileCnt].currentSaveFileMap);
				if (result != 0) // ignore Bad save game
				{
					s_savedata[s_savegame.saveFileCnt].currentSaveFileName = holdChar;
					s_savedata[s_savegame.saveFileCnt].currentSaveFileDateTime = result;

					struct tm *localTime;
					localTime = localtime( &result );
					strcpy(s_savedata[s_savegame.saveFileCnt].currentSaveFileDateTimeString,asctime( localTime ) );
					s_savegame.saveFileCnt++;
					if (s_savegame.saveFileCnt == MAX_SAVELOADFILES)
					{
						break;
					}
				}
			}
		}

		holdChar += len + 1;	//move to next item
	}

	qsort( s_savedata, s_savegame.saveFileCnt, sizeof(savedata_t), UI_SortSaveGames );

}
