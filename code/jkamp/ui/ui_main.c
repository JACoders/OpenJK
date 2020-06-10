/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2005 - 2015, ioquake3 contributors
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

// use this to get a demo build without an explicit demo build, i.e. to get the demo ui files to build
//#define PRE_RELEASE_TADEMO

#include <stdlib.h>

#include "ghoul2/G2.h"
#include "ui_local.h"
#include "qcommon/qfiles.h"
#include "qcommon/game_version.h"
#include "ui_force.h"
#include "cgame/animtable.h" //we want this to be compiled into the module because we access it in the shared module.
#include "game/bg_saga.h"
#include "ui_shared.h"

NORETURN_PTR void (*Com_Error)( int level, const char *error, ... );
void (*Com_Printf)( const char *msg, ... );

extern void UI_SaberAttachToChar( itemDef_t *item );

const char *forcepowerDesc[NUM_FORCE_POWERS] =
{
	"@MENUS_OF_EFFECT_JEDI_ONLY_NEFFECT",
	"@MENUS_DURATION_IMMEDIATE_NAREA",
	"@MENUS_DURATION_5_SECONDS_NAREA",
	"@MENUS_DURATION_INSTANTANEOUS",
	"@MENUS_INSTANTANEOUS_EFFECT_NAREA",
	"@MENUS_DURATION_VARIABLE_20",
	"@MENUS_DURATION_INSTANTANEOUS_NAREA",
	"@MENUS_OF_EFFECT_LIVING_PERSONS",
	"@MENUS_DURATION_VARIABLE_10",
	"@MENUS_DURATION_VARIABLE_NAREA",
	"@MENUS_DURATION_CONTINUOUS_NAREA",
	"@MENUS_OF_EFFECT_JEDI_ALLIES_NEFFECT",
	"@MENUS_EFFECT_JEDI_ALLIES_NEFFECT",
	"@MENUS_VARIABLE_NAREA_OF_EFFECT",
	"@MENUS_EFFECT_NAREA_OF_EFFECT",
	"@SP_INGAME_FORCE_SABER_OFFENSE_DESC",
	"@SP_INGAME_FORCE_SABER_DEFENSE_DESC",
	"@SP_INGAME_FORCE_SABER_THROW_DESC"
};

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

typedef struct datpadmovedata_s
{
	const char	*title;
	const char	*desc;
	const char	*anim;
	short	sound;
} datpadmovedata_t;

static datpadmovedata_t datapadMoveData[MD_MOVE_TITLE_MAX][MAX_MOVES] = {
	{// Acrobatics
		{ "@MENUS_FORCE_JUMP1",				"@MENUS_FORCE_JUMP1_DESC",				"BOTH_FORCEJUMP1",				MDS_FORCE_JUMP },
		{ "@MENUS_FORCE_FLIP",				"@MENUS_FORCE_FLIP_DESC",				"BOTH_FLIP_F",					MDS_FORCE_JUMP },
		{ "@MENUS_ROLL",					"@MENUS_ROLL_DESC",						"BOTH_ROLL_F",					MDS_ROLL },
		{ "@MENUS_BACKFLIP_OFF_WALL",		"@MENUS_BACKFLIP_OFF_WALL_DESC",		"BOTH_WALL_FLIP_BACK1",			MDS_FORCE_JUMP },
		{ "@MENUS_SIDEFLIP_OFF_WALL",		"@MENUS_SIDEFLIP_OFF_WALL_DESC",		"BOTH_WALL_FLIP_RIGHT",			MDS_FORCE_JUMP },
		{ "@MENUS_WALL_RUN",				"@MENUS_WALL_RUN_DESC",					"BOTH_WALL_RUN_RIGHT",			MDS_FORCE_JUMP },
		{ "@MENUS_WALL_GRAB_JUMP",			"@MENUS_WALL_GRAB_JUMP_DESC",			"BOTH_FORCEWALLREBOUND_FORWARD",MDS_FORCE_JUMP },
		{ "@MENUS_RUN_UP_WALL_BACKFLIP",	"@MENUS_RUN_UP_WALL_BACKFLIP_DESC",		"BOTH_FORCEWALLRUNFLIP_START",	MDS_FORCE_JUMP },
		{ "@MENUS_JUMPUP_FROM_KNOCKDOWN",	"@MENUS_JUMPUP_FROM_KNOCKDOWN_DESC",	"BOTH_KNOCKDOWN3",				MDS_NONE },
		{ "@MENUS_JUMPKICK_FROM_KNOCKDOWN",	"@MENUS_JUMPKICK_FROM_KNOCKDOWN_DESC",	"BOTH_KNOCKDOWN2",				MDS_NONE },
		{ "@MENUS_ROLL_FROM_KNOCKDOWN",		"@MENUS_ROLL_FROM_KNOCKDOWN_DESC",		"BOTH_KNOCKDOWN1",				MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
	},
	{//Single Saber, Fast Style
		{ "@MENUS_STAB_BACK",				"@MENUS_STAB_BACK_DESC",				"BOTH_A2_STABBACK1",			MDS_SABER },
		{ "@MENUS_LUNGE_ATTACK",			"@MENUS_LUNGE_ATTACK_DESC",				"BOTH_LUNGE2_B__T_",			MDS_SABER },
		{ "@MENUS_FAST_ATTACK_KATA",		"@MENUS_FAST_ATTACK_KATA_DESC",			"BOTH_A1_SPECIAL",				MDS_SABER },
		{ "@MENUS_ATTACK_ENEMYONGROUND",	"@MENUS_ATTACK_ENEMYONGROUND_DESC",		"BOTH_STABDOWN",				MDS_FORCE_JUMP },
		{ "@MENUS_CARTWHEEL",				"@MENUS_CARTWHEEL_DESC",				"BOTH_ARIAL_RIGHT",				MDS_FORCE_JUMP },
		{ "@MENUS_BOTH_ROLL_STAB",			"@MENUS_BOTH_ROLL_STAB2_DESC",			"BOTH_ROLL_STAB",				MDS_SABER },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
	},
	{//Single Saber, Medium Style
		{ "@MENUS_SLASH_BACK",				"@MENUS_SLASH_BACK_DESC",				"BOTH_ATTACK_BACK",				MDS_SABER },
		{ "@MENUS_FLIP_ATTACK",				"@MENUS_FLIP_ATTACK_DESC",				"BOTH_JUMPFLIPSLASHDOWN1",		MDS_FORCE_JUMP },
		{ "@MENUS_MEDIUM_ATTACK_KATA",		"@MENUS_MEDIUM_ATTACK_KATA_DESC",		"BOTH_A2_SPECIAL",				MDS_SABER },
		{ "@MENUS_ATTACK_ENEMYONGROUND",	"@MENUS_ATTACK_ENEMYONGROUND_DESC",		"BOTH_STABDOWN",				MDS_FORCE_JUMP },
		{ "@MENUS_CARTWHEEL",				"@MENUS_CARTWHEEL_DESC",				"BOTH_ARIAL_RIGHT",				MDS_FORCE_JUMP },
		{ "@MENUS_BOTH_ROLL_STAB",			"@MENUS_BOTH_ROLL_STAB2_DESC",			"BOTH_ROLL_STAB",				MDS_SABER },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
	},
	{//Single Saber, Strong Style
		{ "@MENUS_SLASH_BACK",				"@MENUS_SLASH_BACK_DESC",				"BOTH_ATTACK_BACK",				MDS_SABER },
		{ "@MENUS_JUMP_ATTACK",				"@MENUS_JUMP_ATTACK_DESC",				"BOTH_FORCELEAP2_T__B_",		MDS_FORCE_JUMP },
		{ "@MENUS_STRONG_ATTACK_KATA",		"@MENUS_STRONG_ATTACK_KATA_DESC",		"BOTH_A3_SPECIAL",				MDS_SABER },
		{ "@MENUS_ATTACK_ENEMYONGROUND",	"@MENUS_ATTACK_ENEMYONGROUND_DESC",		"BOTH_STABDOWN",				MDS_FORCE_JUMP },
		{ "@MENUS_CARTWHEEL",				"@MENUS_CARTWHEEL_DESC",				"BOTH_ARIAL_RIGHT",				MDS_FORCE_JUMP },
		{ "@MENUS_BOTH_ROLL_STAB",			"@MENUS_BOTH_ROLL_STAB2_DESC",			"BOTH_ROLL_STAB",				MDS_SABER },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
	},
	{//Dual Sabers
		{ "@MENUS_SLASH_BACK",				"@MENUS_SLASH_BACK_DESC",				"BOTH_ATTACK_BACK",				MDS_SABER },
		{ "@MENUS_FLIP_FORWARD_ATTACK",		"@MENUS_FLIP_FORWARD_ATTACK_DESC",		"BOTH_JUMPATTACK6",				MDS_FORCE_JUMP },
		{ "@MENUS_DUAL_SABERS_TWIRL",		"@MENUS_DUAL_SABERS_TWIRL_DESC",		"BOTH_SPINATTACK6",				MDS_SABER },
		{ "@MENUS_ATTACK_ENEMYONGROUND",	"@MENUS_ATTACK_ENEMYONGROUND_DESC",		"BOTH_STABDOWN_DUAL",			MDS_FORCE_JUMP },
		{ "@MENUS_DUAL_SABER_BARRIER",		"@MENUS_DUAL_SABER_BARRIER_DESC",		"BOTH_A6_SABERPROTECT",			MDS_SABER },
		{ "@MENUS_DUAL_STAB_FRONT_BACK",	"@MENUS_DUAL_STAB_FRONT_BACK_DESC",		"BOTH_A6_FB",					MDS_SABER },
		{ "@MENUS_DUAL_STAB_LEFT_RIGHT",	"@MENUS_DUAL_STAB_LEFT_RIGHT_DESC",		"BOTH_A6_LR",					MDS_SABER },
		{ "@MENUS_CARTWHEEL",				"@MENUS_CARTWHEEL_DESC",				"BOTH_ARIAL_RIGHT",				MDS_FORCE_JUMP },
		{ "@MENUS_BOTH_ROLL_STAB",			"@MENUS_BOTH_ROLL_STAB_DESC",			"BOTH_ROLL_STAB",				MDS_SABER },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
	},
	{// Saber Staff
		{ "@MENUS_STAB_BACK",				"@MENUS_STAB_BACK_DESC",				"BOTH_A2_STABBACK1",			MDS_SABER },
		{ "@MENUS_BACK_FLIP_ATTACK",		"@MENUS_BACK_FLIP_ATTACK_DESC",			"BOTH_JUMPATTACK7",				MDS_FORCE_JUMP },
		{ "@MENUS_SABER_STAFF_TWIRL",		"@MENUS_SABER_STAFF_TWIRL_DESC",		"BOTH_SPINATTACK7",				MDS_SABER },
		{ "@MENUS_ATTACK_ENEMYONGROUND",	"@MENUS_ATTACK_ENEMYONGROUND_DESC",		"BOTH_STABDOWN_STAFF",			MDS_FORCE_JUMP },
		{ "@MENUS_SPINNING_KATA",			"@MENUS_SPINNING_KATA_DESC",			"BOTH_A7_SOULCAL",				MDS_SABER },
		{ "@MENUS_KICK1",					"@MENUS_KICK1_DESC",					"BOTH_A7_KICK_F",				MDS_FORCE_JUMP },
		{ "@MENUS_JUMP_KICK",				"@MENUS_JUMP_KICK_DESC",				"BOTH_A7_KICK_F_AIR",			MDS_FORCE_JUMP },
		{ "@MENUS_BUTTERFLY_ATTACK",		"@MENUS_BUTTERFLY_ATTACK_DESC",			"BOTH_BUTTERFLY_FR1",			MDS_SABER },
		{ "@MENUS_BOTH_ROLL_STAB",			"@MENUS_BOTH_ROLL_STAB2_DESC",			"BOTH_ROLL_STAB",				MDS_SABER },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE },
		{ NULL,								NULL,									NULL,							MDS_NONE }
	}
};

static siegeClassDesc_t g_UIClassDescriptions[MAX_SIEGE_CLASSES];
static siegeTeam_t *siegeTeam1 = NULL, *siegeTeam2 = NULL;
static int g_UIGloballySelectedSiegeClass = -1;

//Cut down version of the stuff used in the game code
//This is just the bare essentials of what we need to load animations properly for ui ghoul2 models.
//This function doesn't need to be sync'd with the BG_ version in bg_panimate.c unless some sort of fundamental change
//is made. Just make sure the variables/functions accessed in ui_shared.c exist in both modules.
qboolean	UIPAFtextLoaded = qfalse;
animation_t	uiHumanoidAnimations[MAX_TOTALANIMATIONS]; //humanoid animations are the only ones that are statically allocated.

bgLoadedAnim_t bgAllAnims[MAX_ANIM_FILES];
int uiNumAllAnims = 1; //start off at 0, because 0 will always be assigned to humanoid.

animation_t *UI_AnimsetAlloc(void)
{
	assert (uiNumAllAnims < MAX_ANIM_FILES);
	bgAllAnims[uiNumAllAnims].anims = (animation_t *) BG_Alloc(sizeof(animation_t)*MAX_TOTALANIMATIONS);

	return bgAllAnims[uiNumAllAnims].anims;
}

/*
======================
UI_ParseAnimationFile

Read a configuration file containing animation counts and rates
models/players/visor/animation.cfg, etc

======================
*/
static char UIPAFtext[60000];
int UI_ParseAnimationFile(const char *filename, animation_t *animset, qboolean isHumanoid)
{
	char		*text_p;
	int			len;
	int			i;
	char		*token;
	float		fps;
	int			usedIndex = -1;
	int			nextIndex = uiNumAllAnims;

	fileHandle_t	f;
	int				animNum;

	if (!isHumanoid)
	{
		i = 1;
		while (i < uiNumAllAnims)
		{ //see if it's been loaded already
			if (!Q_stricmp(bgAllAnims[i].filename, filename))
			{
				animset = bgAllAnims[i].anims;
				return i; //alright, we already have it.
			}
			i++;
		}

		//Looks like it has not yet been loaded. Allocate space for the anim set if we need to, and continue along.
		if (!animset)
		{
			if (strstr(filename, "players/_humanoid/"))
			{ //then use the static humanoid set.
				animset = uiHumanoidAnimations;
				isHumanoid = qtrue;
				nextIndex = 0;
			}
			else
			{
				animset = UI_AnimsetAlloc();

				if (!animset)
				{
					assert(!"Anim set alloc failed!");
					return -1;
				}
			}
		}
	}
#ifdef _DEBUG
	else
	{
		assert(animset);
	}
#endif

	// load the file
	if (!UIPAFtextLoaded || !isHumanoid)
	{ //rww - We are always using the same animation config now. So only load it once.
		len = trap->FS_Open( filename, &f, FS_READ );
		if ( !f ) {
			return -1;
		}
		if ( len >= sizeof( UIPAFtext ) - 1 ) {
			trap->FS_Close( f );
			Com_Error(ERR_DROP, "%s exceeds the allowed ui-side animation buffer!", filename);
		}

		trap->FS_Read( UIPAFtext, len, f );
		UIPAFtext[len] = 0;
		trap->FS_Close( f );
	}
	else
	{
		return 0; //humanoid index
	}

	// parse the text
	text_p = UIPAFtext;

	//FIXME: have some way of playing anims backwards... negative numFrames?

	//initialize anim array so that from 0 to MAX_ANIMATIONS, set default values of 0 1 0 100
	for(i = 0; i < MAX_ANIMATIONS; i++)
	{
		animset[i].firstFrame = 0;
		animset[i].numFrames = 0;
		animset[i].loopFrames = -1;
		animset[i].frameLerp = 100;
//		animset[i].initialLerp = 100;
	}

	COM_BeginParseSession ("UI_ParseAnimationFile");

	// read information for each frame
	while(1)
	{
		token = COM_Parse( (const char **)(&text_p) );

		if ( !token || !token[0])
		{
			break;
		}

		animNum = GetIDForString(animTable, token);
		if(animNum == -1)
		{
//#ifndef FINAL_BUILD
#ifdef _DEBUG
			//Com_Printf(S_COLOR_RED"WARNING: Unknown token %s in %s\n", token, filename);
#endif
			continue;
		}

		token = COM_Parse( (const char **)(&text_p) );
		if ( !token )
		{
			break;
		}
		animset[animNum].firstFrame = atoi( token );

		token = COM_Parse( (const char **)(&text_p) );
		if ( !token )
		{
			break;
		}
		animset[animNum].numFrames = atoi( token );

		token = COM_Parse( (const char **)(&text_p) );
		if ( !token )
		{
			break;
		}
		animset[animNum].loopFrames = atoi( token );

		token = COM_Parse( (const char **)(&text_p) );
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
			animset[animNum].frameLerp = floor(1000.0f / fps);
		}
		else
		{
			animset[animNum].frameLerp = ceil(1000.0f / fps);
		}

//		animset[animNum].initialLerp = ceil(1000.0f / fabs(fps));
	}

#ifdef _DEBUG
	//Check the array, and print the ones that have nothing in them.
	/*
	for(i = 0; i < MAX_ANIMATIONS; i++)
	{
		if (animTable[i].name != NULL)		// This animation reference exists.
		{
			if (animset[i].firstFrame <= 0 && animset[i].numFrames <=0)
			{	// This is an empty animation reference.
				Com_Printf("***ANIMTABLE reference #%d (%s) is empty!\n", i, animTable[i].name);
			}
		}
	}
	*/
#endif // _DEBUG

	if (isHumanoid)
	{
		bgAllAnims[0].anims = animset;
		Q_strncpyz(bgAllAnims[0].filename, filename, sizeof(bgAllAnims[0].filename));
		UIPAFtextLoaded = qtrue;

		usedIndex = 0;
	}
	else
	{
		bgAllAnims[nextIndex].anims = animset;
		Q_strncpyz(bgAllAnims[nextIndex].filename, filename, sizeof(bgAllAnims[nextIndex].filename));

		usedIndex = nextIndex;

		if (nextIndex)
		{ //don't bother increasing the number if this ended up as a humanoid load.
			uiNumAllAnims++;
		}
		else
		{
			UIPAFtextLoaded = qtrue;
			usedIndex = 0;
		}
	}

	return usedIndex;
}

//menuDef_t *Menus_FindByName(const char *p);
void Menu_ShowItemByName(menuDef_t *menu, const char *p, qboolean bShow);


void UpdateForceUsed();

char holdSPString[MAX_STRING_CHARS]={0};

uiInfo_t uiInfo;

static void UI_StartServerRefresh(qboolean full);
static void UI_StopServerRefresh( void );
static void UI_DoServerRefresh( void );
static void UI_BuildServerDisplayList(int force);
static void UI_BuildServerStatus(qboolean force);
static void UI_BuildFindPlayerList(qboolean force);
static int QDECL UI_ServersQsortCompare( const void *arg1, const void *arg2 );
static int UI_MapCountByGameType(qboolean singlePlayer);
static int UI_HeadCountByColor( void );
static void UI_ParseGameInfo(const char *teamFile);
static const char *UI_SelectedMap(int index, int *actual);
static int UI_GetIndexFromSelection(int actual);
static void UI_SiegeClassCnt( const int team );

int	uiSkinColor=TEAM_FREE;
int	uiHoldSkinColor=TEAM_FREE;	// Stores the skin color so that in non-team games, the player screen remembers the team you chose, in case you're coming back from the force powers screen.

static const char *skillLevels[] = {
	"SKILL1", // "Initiate"
	"SKILL2", // "Padawan"
	"SKILL3", // "Jedi"
	"SKILL4", // "Jedi Knight"
	"SKILL5" // "Jedi Master"
};
static const size_t numSkillLevels = ARRAY_LEN( skillLevels );

static const char *gameTypes[GT_MAX_GAME_TYPE] = {
	"FFA",
	"Holocron",
	"JediMaster",
	"Duel",
	"PowerDuel",
	"SP",
	"Team FFA",
	"Siege",
	"CTF",
	"CTY",
};
static const int numGameTypes = ARRAY_LEN( gameTypes );

static char* netNames[] = {
	"???",
	"UDP",
	NULL
};

static const int numNetNames = ARRAY_LEN( netNames ) - 1;

const char *UI_GetStringEdString(const char *refSection, const char *refName);

const char *UI_TeamName(int team) {
	if (team==TEAM_RED)
		return "RED";
	else if (team==TEAM_BLUE)
		return "BLUE";
	else if (team==TEAM_SPECTATOR)
		return "SPECTATOR";
	return "FREE";
}

// returns either string or NULL for OOR...
//
static const char *GetCRDelineatedString( const char *psStripFileRef, const char *psStripStringRef, int iIndex)
{
	static char sTemp[256];
	const char *psList = UI_GetStringEdString(psStripFileRef, psStripStringRef);
	char *p;

	while (iIndex--)
	{
		psList = strchr(psList,'\n');
		if (!psList){
			return NULL;	// OOR
		}
		psList++;
	}

	Q_strncpyz(sTemp,psList, sizeof(sTemp));
	p = strchr(sTemp,'\n');
	if (p) {
		*p = '\0';
	}

	return sTemp;
}

static const char *GetMonthAbbrevString( int iMonth )
{
	const char *p = GetCRDelineatedString("MP_INGAME","MONTHS", iMonth);

	return p ? p : "Jan";	// sanity
}

#define UIAS_LOCAL				0
#define UIAS_GLOBAL1			1
#define UIAS_GLOBAL2			2
#define UIAS_GLOBAL3			3
#define UIAS_GLOBAL4			4
#define UIAS_GLOBAL5			5
#define UIAS_FAVORITES			6

#define UI_MAX_MASTER_SERVERS	5

// Convert ui's net source to AS_* used by trap calls.
int UI_SourceForLAN( void ) {
	switch ( ui_netSource.integer ) {
	default:
	case UIAS_LOCAL:
		return AS_LOCAL;
	case UIAS_GLOBAL1:
	case UIAS_GLOBAL2:
	case UIAS_GLOBAL3:
	case UIAS_GLOBAL4:
	case UIAS_GLOBAL5:
		return AS_GLOBAL;
	case UIAS_FAVORITES:
		return AS_FAVORITES;
	}
}

/*
static const char *netSources[] = {
	"Local",
	"Internet",
	"Favorites"
//	"Mplayer"
};
static const int numNetSources = ARRAY_LEN(netSources);
*/
static const int numNetSources = 7;	// now hard-entered in StringEd file
static const char *GetNetSourceString(int iSource)
{
	static char result[256] = {0};

	Q_strncpyz( result, GetCRDelineatedString( "MP_INGAME", "NET_SOURCES", UI_SourceForLAN() ), sizeof(result) );

	if ( iSource >= UIAS_GLOBAL1 && iSource <= UIAS_GLOBAL5 ) {
		Q_strcat( result, sizeof(result), va( " %d", iSource ) );
	}

	return result;
}

void AssetCache(void) {
	int n;
	//if (Assets.textFont == NULL) {
	//}
	//Com_Printf("Menu Size: %i bytes\n", sizeof(Menus));
	uiInfo.uiDC.Assets.gradientBar			= trap->R_RegisterShaderNoMip( ASSET_GRADIENTBAR );
	uiInfo.uiDC.Assets.fxBasePic			= trap->R_RegisterShaderNoMip( ART_FX_BASE );
	uiInfo.uiDC.Assets.fxPic[0]				= trap->R_RegisterShaderNoMip( ART_FX_RED );
	uiInfo.uiDC.Assets.fxPic[1]				= trap->R_RegisterShaderNoMip( ART_FX_ORANGE );//trap->R_RegisterShaderNoMip( ART_FX_YELLOW );
	uiInfo.uiDC.Assets.fxPic[2]				= trap->R_RegisterShaderNoMip( ART_FX_YELLOW );//trap->R_RegisterShaderNoMip( ART_FX_GREEN );
	uiInfo.uiDC.Assets.fxPic[3]				= trap->R_RegisterShaderNoMip( ART_FX_GREEN );//trap->R_RegisterShaderNoMip( ART_FX_TEAL );
	uiInfo.uiDC.Assets.fxPic[4]				= trap->R_RegisterShaderNoMip( ART_FX_BLUE );
	uiInfo.uiDC.Assets.fxPic[5]				= trap->R_RegisterShaderNoMip( ART_FX_PURPLE );//trap->R_RegisterShaderNoMip( ART_FX_CYAN );
	uiInfo.uiDC.Assets.fxPic[6]				= trap->R_RegisterShaderNoMip( ART_FX_WHITE );
	uiInfo.uiDC.Assets.scrollBar			= trap->R_RegisterShaderNoMip( ASSET_SCROLLBAR );
	uiInfo.uiDC.Assets.scrollBarArrowDown	= trap->R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWDOWN );
	uiInfo.uiDC.Assets.scrollBarArrowUp		= trap->R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWUP );
	uiInfo.uiDC.Assets.scrollBarArrowLeft	= trap->R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWLEFT );
	uiInfo.uiDC.Assets.scrollBarArrowRight	= trap->R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWRIGHT );
	uiInfo.uiDC.Assets.scrollBarThumb		= trap->R_RegisterShaderNoMip( ASSET_SCROLL_THUMB );
	uiInfo.uiDC.Assets.sliderBar			= trap->R_RegisterShaderNoMip( ASSET_SLIDER_BAR );
	uiInfo.uiDC.Assets.sliderThumb			= trap->R_RegisterShaderNoMip( ASSET_SLIDER_THUMB );

	// Icons for various server settings.
	uiInfo.uiDC.Assets.needPass			= trap->R_RegisterShaderNoMip( "gfx/menus/needpass" );
	uiInfo.uiDC.Assets.noForce			= trap->R_RegisterShaderNoMip( "gfx/menus/noforce" );
	uiInfo.uiDC.Assets.forceRestrict	= trap->R_RegisterShaderNoMip( "gfx/menus/forcerestrict" );
	uiInfo.uiDC.Assets.saberOnly		= trap->R_RegisterShaderNoMip( "gfx/menus/saberonly" );
	uiInfo.uiDC.Assets.trueJedi			= trap->R_RegisterShaderNoMip( "gfx/menus/truejedi" );

	for( n = 0; n < NUM_CROSSHAIRS; n++ ) {
		uiInfo.uiDC.Assets.crosshairShader[n] = trap->R_RegisterShaderNoMip( va("gfx/2d/crosshair%c", 'a' + n ) );
	}
}

void _UI_DrawSides(float x, float y, float w, float h, float size) {
	size *= uiInfo.uiDC.xscale;
	trap->R_DrawStretchPic( x, y, size, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap->R_DrawStretchPic( x + w - size, y, size, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
}

void _UI_DrawTopBottom(float x, float y, float w, float h, float size) {
	size *= uiInfo.uiDC.yscale;
	trap->R_DrawStretchPic( x, y, w, size, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap->R_DrawStretchPic( x, y + h - size, w, size, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
}
/*
================
UI_DrawRect

Coordinates are 640*480 virtual values
=================
*/
void _UI_DrawRect( float x, float y, float width, float height, float size, const float *color ) {
	trap->R_SetColor( color );

	_UI_DrawTopBottom(x, y, width, height, size);
	_UI_DrawSides(x, y, width, height, size);

	trap->R_SetColor( NULL );
}

int MenuFontToHandle(int iMenuFont)
{
	switch (iMenuFont)
	{
		case 1: return uiInfo.uiDC.Assets.qhSmallFont;
		case 2: return uiInfo.uiDC.Assets.qhMediumFont;
		case 3: return uiInfo.uiDC.Assets.qhBigFont;
		case 4: return uiInfo.uiDC.Assets.qhSmall2Font;
	}

	return uiInfo.uiDC.Assets.qhMediumFont;	// 0;
}

int Text_Width(const char *text, float scale, int iMenuFont)
{
	int iFontIndex = MenuFontToHandle(iMenuFont);

	return trap->R_Font_StrLenPixels(text, iFontIndex, scale);
}

int Text_Height(const char *text, float scale, int iMenuFont)
{
	int iFontIndex = MenuFontToHandle(iMenuFont);

	return trap->R_Font_HeightPixels(iFontIndex, scale);
}

void Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style, int iMenuFont)
{
	int iStyleOR = 0;

	int iFontIndex = MenuFontToHandle(iMenuFont);
	//
	// kludge.. convert JK2 menu styles to SOF2 printstring ctrl codes...
	//
	switch (style)
	{
	case  ITEM_TEXTSTYLE_NORMAL:			iStyleOR = 0;break;					// JK2 normal text
	case  ITEM_TEXTSTYLE_BLINK:				iStyleOR = (int)STYLE_BLINK;break;		// JK2 fast blinking
	case  ITEM_TEXTSTYLE_PULSE:				iStyleOR = (int)STYLE_BLINK;break;		// JK2 slow pulsing
	case  ITEM_TEXTSTYLE_SHADOWED:			iStyleOR = (int)STYLE_DROPSHADOW;break;	// JK2 drop shadow
	case  ITEM_TEXTSTYLE_OUTLINED:			iStyleOR = (int)STYLE_DROPSHADOW;break;	// JK2 drop shadow
	case  ITEM_TEXTSTYLE_OUTLINESHADOWED:	iStyleOR = (int)STYLE_DROPSHADOW;break;	// JK2 drop shadow
	case  ITEM_TEXTSTYLE_SHADOWEDMORE:		iStyleOR = (int)STYLE_DROPSHADOW;break;	// JK2 drop shadow
	}

	trap->R_Font_DrawString(	x,		// int ox
							y,		// int oy
							text,	// const char *text
							color,	// paletteRGBA_c c
							iStyleOR | iFontIndex,	// const int iFontHandle
							!limit?-1:limit,		// iCharLimit (-1 = none)
							scale	// const float scale = 1.0f
							);
}


void Text_PaintWithCursor(float x, float y, float scale, vec4_t color, const char *text, int cursorPos, char cursor, int limit, int style, int iMenuFont)
{
	Text_Paint(x, y, scale, color, text, 0, limit, style, iMenuFont);

	// now print the cursor as well...  (excuse the braces, it's for porting C++ to C)
	//
	{
		char sTemp[1024];
		int iCopyCount = limit > 0 ? Q_min( (int)strlen( text ), limit ) : (int)strlen( text );
			iCopyCount = Q_min( iCopyCount, cursorPos );
			iCopyCount = Q_min( iCopyCount, (int)sizeof( sTemp )-1 );

			// copy text into temp buffer for pixel measure...
			//
			strncpy(sTemp,text,iCopyCount);
					sTemp[iCopyCount] = '\0';

			{
				int iFontIndex = MenuFontToHandle( iMenuFont );
				int iNextXpos  = trap->R_Font_StrLenPixels(sTemp, iFontIndex, scale );

				Text_Paint(x+iNextXpos, y, scale, color, va("%c",cursor), 0, limit, style|ITEM_TEXTSTYLE_BLINK, iMenuFont);
			}
	}
}


// maxX param is initially an X limit, but is also used as feedback. 0 = text was clipped to fit within, else maxX = next pos
//
static void Text_Paint_Limit(float *maxX, float x, float y, float scale, vec4_t color, const char* text, float adjust, int limit, int iMenuFont)
{
	// this is kinda dirty, but...
	//
	int iFontIndex = MenuFontToHandle(iMenuFont);

	//float fMax = *maxX;
	int iPixelLen = trap->R_Font_StrLenPixels(text, iFontIndex, scale);
	if (x + iPixelLen > *maxX)
	{
		// whole text won't fit, so we need to print just the amount that does...
		//  Ok, this is slow and tacky, but only called occasionally, and it works...
		//
		char sTemp[4096]={0};	// lazy assumption
		const char *psText = text;
		char *psOut = &sTemp[0];
		char *psOutLastGood = psOut;
		unsigned int uiLetter;

		while (*psText && (x + trap->R_Font_StrLenPixels(sTemp, iFontIndex, scale)<=*maxX)
			   && psOut < &sTemp[sizeof(sTemp)-1]	// sanity
				)
		{
			int iAdvanceCount;
			psOutLastGood = psOut;

			uiLetter = trap->R_AnyLanguage_ReadCharFromString(psText, &iAdvanceCount, NULL);
			psText += iAdvanceCount;

			if (uiLetter > 255)
			{
				*psOut++ = uiLetter>>8;
				*psOut++ = uiLetter&0xFF;
			}
			else
			{
				*psOut++ = uiLetter&0xFF;
			}
		}
		*psOutLastGood = '\0';

		*maxX = 0;	// feedback
		Text_Paint(x, y, scale, color, sTemp, adjust, limit, ITEM_TEXTSTYLE_NORMAL, iMenuFont);
	}
	else
	{
		// whole text fits fine, so print it all...
		//
		*maxX = x + iPixelLen;	// feedback the next position, as the caller expects
		Text_Paint(x, y, scale, color, text, adjust, limit, ITEM_TEXTSTYLE_NORMAL, iMenuFont);
	}
}

void UI_LoadNonIngame() {
	const char *menuSet = UI_Cvar_VariableString("ui_menuFilesMP");
	if (menuSet == NULL || menuSet[0] == '\0') {
		menuSet = "ui/jampmenus.txt";
	}
	UI_LoadMenus(menuSet, qfalse);
	uiInfo.inGameLoad = qfalse;
}

/*
===============
UI_BuildPlayerList
===============
*/
static void UI_BuildPlayerList() {
	uiClientState_t	cs;
	int		n, count, team, team2, playerTeamNumber;
	char	info[MAX_INFO_STRING];

	trap->GetClientState( &cs );
	trap->GetConfigString( CS_PLAYERS + cs.clientNum, info, MAX_INFO_STRING );
	uiInfo.playerNumber = cs.clientNum;
	uiInfo.teamLeader = atoi(Info_ValueForKey(info, "tl"));
	team = atoi(Info_ValueForKey(info, "t"));
	trap->GetConfigString( CS_SERVERINFO, info, sizeof(info) );
	count = atoi( Info_ValueForKey( info, "sv_maxclients" ) );
	uiInfo.playerCount = 0;
	uiInfo.myTeamCount = 0;
	playerTeamNumber = 0;
	for( n = 0; n < count; n++ ) {
		trap->GetConfigString( CS_PLAYERS + n, info, MAX_INFO_STRING );

		if (info[0]) {
			Q_strncpyz( uiInfo.playerNames[uiInfo.playerCount], Info_ValueForKey( info, "n" ), MAX_NETNAME );
			Q_StripColor( uiInfo.playerNames[uiInfo.playerCount] );
			uiInfo.playerIndexes[uiInfo.playerCount] = n;
			uiInfo.playerCount++;
			team2 = atoi(Info_ValueForKey(info, "t"));
			if (team2 == team && n != uiInfo.playerNumber) {
				Q_strncpyz( uiInfo.teamNames[uiInfo.myTeamCount], Info_ValueForKey( info, "n" ), MAX_NETNAME );
				Q_StripColor( uiInfo.teamNames[uiInfo.myTeamCount] );
				uiInfo.teamClientNums[uiInfo.myTeamCount] = n;
				if (uiInfo.playerNumber == n) {
					playerTeamNumber = uiInfo.myTeamCount;
				}
				uiInfo.myTeamCount++;
			}
		}
	}

	if (!uiInfo.teamLeader) {
		trap->Cvar_Set("cg_selectedPlayer", va("%d", playerTeamNumber));
	}

	n = trap->Cvar_VariableValue("cg_selectedPlayer");
	if (n < 0 || n > uiInfo.myTeamCount) {
		n = 0;
	}


	if (n < uiInfo.myTeamCount) {
		trap->Cvar_Set("cg_selectedPlayerName", uiInfo.teamNames[n]);
	}
	else
	{
		trap->Cvar_Set("cg_selectedPlayerName", "Everyone");
	}

	if (!team || team == TEAM_SPECTATOR || !uiInfo.teamLeader)
	{
		n = uiInfo.myTeamCount;
		trap->Cvar_Set("cg_selectedPlayer", va("%d", n));
		trap->Cvar_Set("cg_selectedPlayerName", "N/A");
	}
}

void UI_SetActiveMenu( uiMenuCommand_t menu ) {
	char buf[256];

	// this should be the ONLY way the menu system is brought up
	// enusure minumum menu data is cached
	if (Menu_Count() > 0) {
		vec3_t v;
		v[0] = v[1] = v[2] = 0;
		switch ( menu ) {
		case UIMENU_NONE:
			trap->Key_SetCatcher( trap->Key_GetCatcher() & ~KEYCATCH_UI );
			trap->Key_ClearStates();
			trap->Cvar_Set( "cl_paused", "0" );
			Menus_CloseAll();

			return;
		case UIMENU_MAIN:
			{
				//	trap->Cvar_Set( "sv_killserver", "1" );
				trap->Key_SetCatcher( KEYCATCH_UI );
				//	trap->S_StartLocalSound( trap_S_RegisterSound("sound/misc/menu_background.wav", qfalse) , CHAN_LOCAL_SOUND );
				//	trap->S_StartBackgroundTrack("sound/misc/menu_background.wav", NULL);
				if (uiInfo.inGameLoad)
					UI_LoadNonIngame();

				Menus_CloseAll();
				Menus_ActivateByName("main");
				trap->Cvar_VariableStringBuffer("com_errorMessage", buf, sizeof(buf));

				if (buf[0])
				{
					if (!ui_singlePlayerActive.integer)
					{
						Menus_ActivateByName("error_popmenu");
					}
					else
					{
						trap->Cvar_Set("com_errorMessage", "");
					}
				}
				return;
			}

		case UIMENU_TEAM:
			trap->Key_SetCatcher( KEYCATCH_UI );
			Menus_ActivateByName("team");
			return;
		case UIMENU_POSTGAME:
			//trap->Cvar_Set( "sv_killserver", "1" );
			trap->Key_SetCatcher( KEYCATCH_UI );
			if (uiInfo.inGameLoad)
				UI_LoadNonIngame();
			Menus_CloseAll();
			Menus_ActivateByName("endofgame");
			return;
		case UIMENU_INGAME:
			trap->Cvar_Set( "cl_paused", "1" );
			trap->Key_SetCatcher( KEYCATCH_UI );
			UI_BuildPlayerList();
			Menus_CloseAll();
			Menus_ActivateByName("ingame");
			return;
		case UIMENU_PLAYERCONFIG:
			// trap->Cvar_Set( "cl_paused", "1" );
			trap->Key_SetCatcher( KEYCATCH_UI );
			UI_BuildPlayerList();
			Menus_CloseAll();
			Menus_ActivateByName("ingame_player");
			UpdateForceUsed();
			return;
		case UIMENU_PLAYERFORCE:
			// trap->Cvar_Set( "cl_paused", "1" );
			trap->Key_SetCatcher( KEYCATCH_UI );
			UI_BuildPlayerList();
			Menus_CloseAll();
			Menus_ActivateByName("ingame_playerforce");
			UpdateForceUsed();
			return;
		case UIMENU_SIEGEMESSAGE:
			// trap->Cvar_Set( "cl_paused", "1" );
			trap->Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName("siege_popmenu");
			return;
		case UIMENU_SIEGEOBJECTIVES:
			// trap->Cvar_Set( "cl_paused", "1" );
			trap->Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName("ingame_siegeobjectives");
			return;
		case UIMENU_VOICECHAT:
			// trap->Cvar_Set( "cl_paused", "1" );
			// No chatin non-siege games.

			if (trap->Cvar_VariableValue( "g_gametype" ) < GT_TEAM)
			{
				return;
			}

			trap->Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName("ingame_voicechat");
			return;
		case UIMENU_CLOSEALL:
			Menus_CloseAll();
			return;
		case UIMENU_CLASSSEL:
			trap->Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName("ingame_siegeclass");
			return;
		}
	}
}

void UI_DrawCenteredPic(qhandle_t image, int w, int h) {
	int x, y;
	x = (SCREEN_WIDTH - w) / 2;
	y = (SCREEN_HEIGHT - h) / 2;
	UI_DrawHandlePic(x, y, w, h, image);
}

int frameCount = 0;
int startTime;

static void UI_BuildPlayerList();
char parsedFPMessage[1024];

extern int FPMessageTime;

void Text_PaintCenter(float x, float y, float scale, vec4_t color, const char *text, float adjust, int iMenuFont);

const char *UI_GetStringEdString(const char *refSection, const char *refName)
{
	static char text[1024]={0};

	trap->SE_GetStringTextString(va("%s_%s", refSection, refName), text, sizeof(text));
	return text;
}

void UI_SetColor( const float *rgba ) {
	trap->R_SetColor( rgba );
}

/*
=================
_UI_Shutdown
=================
*/
void UI_CleanupGhoul2(void);
void UI_FreeAllSpecies(void);

void UI_Shutdown( void ) {
	trap->LAN_SaveCachedServers();
	UI_CleanupGhoul2();
	UI_FreeAllSpecies();
}

char *defaultMenu = NULL;

char *GetMenuBuffer(const char *filename) {
	int	len;
	fileHandle_t	f;
	static char buf[MAX_MENUFILE];

	len = trap->FS_Open( filename, &f, FS_READ );
	if ( !f ) {
		trap->Print( S_COLOR_RED "menu file not found: %s, using default\n", filename );
		return defaultMenu;
	}
	if ( len >= MAX_MENUFILE ) {
		trap->Print( S_COLOR_RED "menu file too large: %s is %i, max allowed is %i\n", filename, len, MAX_MENUFILE );
		trap->FS_Close( f );
		return defaultMenu;
	}

	trap->FS_Read( buf, len, f );
	buf[len] = 0;
	trap->FS_Close( f );
	//COM_Compress(buf);
	return buf;
}

qboolean Asset_Parse(int handle) {
	pc_token_t token;

	if (!trap->PC_ReadToken(handle, &token))
		return qfalse;
	if (Q_stricmp(token.string, "{") != 0) {
		return qfalse;
	}

	while ( 1 ) {
		memset(&token, 0, sizeof(pc_token_t));

		if (!trap->PC_ReadToken(handle, &token))
			return qfalse;

		if (Q_stricmp(token.string, "}") == 0) {
			return qtrue;
		}

		// font
		if (Q_stricmp(token.string, "font") == 0) {
			int pointSize;
			if (!trap->PC_ReadToken(handle, &token) || !PC_Int_Parse(handle,&pointSize)) {
				return qfalse;
			}
			//trap->R_RegisterFont(tempStr, pointSize, &uiInfo.uiDC.Assets.textFont);
			uiInfo.uiDC.Assets.qhMediumFont = trap->R_RegisterFont(token.string);
			uiInfo.uiDC.Assets.fontRegistered = qtrue;
			continue;
		}

		if (Q_stricmp(token.string, "smallFont") == 0) {
			int pointSize;
			if (!trap->PC_ReadToken(handle, &token) || !PC_Int_Parse(handle,&pointSize)) {
				return qfalse;
			}
			//trap->R_RegisterFont(token, pointSize, &uiInfo.uiDC.Assets.smallFont);
			uiInfo.uiDC.Assets.qhSmallFont = trap->R_RegisterFont(token.string);
			continue;
		}

		if (Q_stricmp(token.string, "small2Font") == 0) {
			int pointSize;
			if (!trap->PC_ReadToken(handle, &token) || !PC_Int_Parse(handle,&pointSize)) {
				return qfalse;
			}
			//trap->R_RegisterFont(token, pointSize, &uiInfo.uiDC.Assets.smallFont);
			uiInfo.uiDC.Assets.qhSmall2Font = trap->R_RegisterFont(token.string);
			continue;
		}

		if (Q_stricmp(token.string, "bigFont") == 0) {
			int pointSize;
			if (!trap->PC_ReadToken(handle, &token) || !PC_Int_Parse(handle,&pointSize)) {
				return qfalse;
			}
			//trap->R_RegisterFont(token, pointSize, &uiInfo.uiDC.Assets.bigFont);
			uiInfo.uiDC.Assets.qhBigFont = trap->R_RegisterFont(token.string);
			continue;
		}

		if (Q_stricmp(token.string, "cursor") == 0)
		{
			if (!PC_String_Parse(handle, &uiInfo.uiDC.Assets.cursorStr))
			{
				Com_Printf(S_COLOR_YELLOW,"Bad 1st parameter for keyword 'cursor'");
				return qfalse;
			}
			uiInfo.uiDC.Assets.cursor = trap->R_RegisterShaderNoMip( uiInfo.uiDC.Assets.cursorStr);
			continue;
		}

		// gradientbar
		if (Q_stricmp(token.string, "gradientbar") == 0) {
			if (!trap->PC_ReadToken(handle, &token)) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.gradientBar = trap->R_RegisterShaderNoMip(token.string);
			continue;
		}

		// enterMenuSound
		if (Q_stricmp(token.string, "menuEnterSound") == 0) {
			if (!trap->PC_ReadToken(handle, &token)) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.menuEnterSound = trap->S_RegisterSound( token.string );
			continue;
		}

		// exitMenuSound
		if (Q_stricmp(token.string, "menuExitSound") == 0) {
			if (!trap->PC_ReadToken(handle, &token)) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.menuExitSound = trap->S_RegisterSound( token.string );
			continue;
		}

		// itemFocusSound
		if (Q_stricmp(token.string, "itemFocusSound") == 0) {
			if (!trap->PC_ReadToken(handle, &token)) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.itemFocusSound = trap->S_RegisterSound( token.string );
			continue;
		}

		// menuBuzzSound
		if (Q_stricmp(token.string, "menuBuzzSound") == 0) {
			if (!trap->PC_ReadToken(handle, &token)) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.menuBuzzSound = trap->S_RegisterSound( token.string );
			continue;
		}

		if (Q_stricmp(token.string, "fadeClamp") == 0) {
			if (!PC_Float_Parse(handle, &uiInfo.uiDC.Assets.fadeClamp)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "fadeCycle") == 0) {
			if (!PC_Int_Parse(handle, &uiInfo.uiDC.Assets.fadeCycle)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "fadeAmount") == 0) {
			if (!PC_Float_Parse(handle, &uiInfo.uiDC.Assets.fadeAmount)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowX") == 0) {
			if (!PC_Float_Parse(handle, &uiInfo.uiDC.Assets.shadowX)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowY") == 0) {
			if (!PC_Float_Parse(handle, &uiInfo.uiDC.Assets.shadowY)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowColor") == 0) {
			if (!PC_Color_Parse(handle, &uiInfo.uiDC.Assets.shadowColor)) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.shadowFadeClamp = uiInfo.uiDC.Assets.shadowColor[3];
			continue;
		}

		if (Q_stricmp(token.string, "moveRollSound") == 0)
		{
			if (trap->PC_ReadToken(handle,&token))
			{
				uiInfo.uiDC.Assets.moveRollSound = trap->S_RegisterSound( token.string );
			}
			continue;
		}

		if (Q_stricmp(token.string, "moveJumpSound") == 0)
		{
			if (trap->PC_ReadToken(handle,&token))
			{
				uiInfo.uiDC.Assets.moveJumpSound = trap->S_RegisterSound( token.string );
			}

			continue;
		}
		if (Q_stricmp(token.string, "datapadmoveSaberSound1") == 0)
		{
			if (trap->PC_ReadToken(handle,&token))
			{
				uiInfo.uiDC.Assets.datapadmoveSaberSound1 = trap->S_RegisterSound( token.string );
			}

			continue;
		}

		if (Q_stricmp(token.string, "datapadmoveSaberSound2") == 0)
		{
			if (trap->PC_ReadToken(handle,&token))
			{
				uiInfo.uiDC.Assets.datapadmoveSaberSound2 = trap->S_RegisterSound( token.string );
			}

			continue;
		}

		if (Q_stricmp(token.string, "datapadmoveSaberSound3") == 0)
		{
			if (trap->PC_ReadToken(handle,&token))
			{
				uiInfo.uiDC.Assets.datapadmoveSaberSound3 = trap->S_RegisterSound( token.string );
			}

			continue;
		}

		if (Q_stricmp(token.string, "datapadmoveSaberSound4") == 0)
		{
			if (trap->PC_ReadToken(handle,&token))
			{
				uiInfo.uiDC.Assets.datapadmoveSaberSound4 = trap->S_RegisterSound( token.string );
			}

			continue;
		}

		if (Q_stricmp(token.string, "datapadmoveSaberSound5") == 0)
		{
			if (trap->PC_ReadToken(handle,&token))
			{
				uiInfo.uiDC.Assets.datapadmoveSaberSound5 = trap->S_RegisterSound( token.string );
			}

			continue;
		}

		if (Q_stricmp(token.string, "datapadmoveSaberSound6") == 0)
		{
			if (trap->PC_ReadToken(handle,&token))
			{
				uiInfo.uiDC.Assets.datapadmoveSaberSound6 = trap->S_RegisterSound( token.string );
			}

			continue;
		}

		// precaching various sound files used in the menus
		if (Q_stricmp(token.string, "precacheSound") == 0)
		{
			const char *tempStr;
			if (PC_Script_Parse(handle, &tempStr))
			{
				char *soundFile;
				do
				{
					soundFile = COM_ParseExt(&tempStr, qfalse);
					if (soundFile[0] != 0 && soundFile[0] != ';') {
						trap->S_RegisterSound( soundFile);
					}
				} while (soundFile[0]);
			}
			continue;
		}
	}
	return qfalse;
}

void UI_Report( void ) {
	String_Report();
	//Font_Report();
}

void UI_ParseMenu(const char *menuFile) {
	int handle;
	pc_token_t token;

	//Com_Printf("Parsing menu file: %s\n", menuFile);

	handle = trap->PC_LoadSource(menuFile);
	if (!handle) {
		return;
	}

	while ( 1 ) {
		memset(&token, 0, sizeof(pc_token_t));
		if (!trap->PC_ReadToken( handle, &token )) {
			break;
		}

		//if ( Q_stricmp( token, "{" ) ) {
		//	Com_Printf( "Missing { in menu file\n" );
		//	break;
		//}

		//if ( menuCount == MAX_MENUS ) {
		//	Com_Printf( "Too many menus!\n" );
		//	break;
		//}

		if ( token.string[0] == '}' ) {
			break;
		}

		if (Q_stricmp(token.string, "assetGlobalDef") == 0) {
			if (Asset_Parse(handle)) {
				continue;
			} else {
				break;
			}
		}

		if (Q_stricmp(token.string, "menudef") == 0) {
			// start a new menu
			Menu_New(handle);
		}
	}
	trap->PC_FreeSource(handle);
}

qboolean Load_Menu(int handle) {
	pc_token_t token;

	if (!trap->PC_ReadToken(handle, &token))
		return qfalse;
	if (token.string[0] != '{') {
		return qfalse;
	}

	while ( 1 ) {

		if (!trap->PC_ReadToken(handle, &token))
			return qfalse;

		if ( token.string[0] == 0 ) {
			return qfalse;
		}

		if ( token.string[0] == '}' ) {
			return qtrue;
		}

		UI_ParseMenu(token.string);
	}
	return qfalse;
}

void UI_LoadMenus(const char *menuFile, qboolean reset) {
	pc_token_t token;
	int handle;
//	int start = trap->Milliseconds();

	trap->PC_LoadGlobalDefines ( "ui/jamp/menudef.h" );

	handle = trap->PC_LoadSource( menuFile );
	if (!handle) {
		Com_Printf( S_COLOR_YELLOW "menu file not found: %s, using default\n", menuFile );
		handle = trap->PC_LoadSource( "ui/jampmenus.txt" );
		if (!handle) {
			trap->Error( ERR_DROP, S_COLOR_RED "default menu file not found: ui/jampmenus.txt, unable to continue!\n" );
		}
	}

	if (reset) {
		Menu_Reset();
	}

	while ( 1 ) {
		if (!trap->PC_ReadToken(handle, &token))
			break;
		if( token.string[0] == 0 || token.string[0] == '}') {
			break;
		}

		if ( token.string[0] == '}' ) {
			break;
		}

		if (Q_stricmp(token.string, "loadmenu") == 0) {
			if (Load_Menu(handle)) {
				continue;
			} else {
				break;
			}
		}
	}

//	Com_Printf("UI menu load time = %d milli seconds\n", trap->Milliseconds() - start);

	trap->PC_FreeSource( handle );

	trap->PC_RemoveAllGlobalDefines ( );
}

void UI_Load( void ) {
	char *menuSet;
	char lastName[1024];
	menuDef_t *menu = Menu_GetFocused();

	if (menu && menu->window.name) {
		Q_strncpyz(lastName, menu->window.name, sizeof(lastName));
	}
	else
	{
		lastName[0] = 0;
	}

	if (uiInfo.inGameLoad)
	{
		menuSet = "ui/jampingame.txt";
	}
	else
	{
		menuSet = UI_Cvar_VariableString("ui_menuFilesMP");
	}
	if (menuSet == NULL || menuSet[0] == '\0') {
		menuSet = "ui/jampmenus.txt";
	}

	String_Init();

#ifdef PRE_RELEASE_TADEMO
	UI_ParseGameInfo("demogameinfo.txt");
#else
	UI_ParseGameInfo("ui/jamp/gameinfo.txt");
#endif
	UI_LoadArenas();
	UI_LoadBots();

	UI_LoadMenus(menuSet, qtrue);
	Menus_CloseAll();
	Menus_ActivateByName(lastName);
}

char	sAll[15] = {0};
char	sJediAcademy[30] = {0};
const char *UI_FilterDescription( int value ) {
	if ( value <= 0 || value > uiInfo.modCount ) {
		return sAll;
	}

	return uiInfo.modList[value - 1].modDescr;
}

const char *UI_FilterDir( int value ) {
	if ( value <= 0 || value > uiInfo.modCount ) {
		return "";
	}

	return uiInfo.modList[value - 1].modName;
}

static const char *handicapValues[] = {"None","95","90","85","80","75","70","65","60","55","50","45","40","35","30","25","20","15","10","5",NULL};

static void UI_DrawHandicap(rectDef_t *rect, float scale, vec4_t color, int textStyle, int iMenuFont) {
	int i, h;

	h = Com_Clamp(5, 100, trap->Cvar_VariableValue("handicap"));
	i = 20 - h / 5;

	Text_Paint(rect->x, rect->y, scale, color, handicapValues[i], 0, 0, textStyle, iMenuFont);
}

static void UI_DrawClanName(rectDef_t *rect, float scale, vec4_t color, int textStyle, int iMenuFont) {
	Text_Paint(rect->x, rect->y, scale, color, UI_Cvar_VariableString("ui_teamName"), 0, 0, textStyle, iMenuFont);
}

static void UI_SetCapFragLimits(qboolean uiVars) {
	int cap = 5;
	int frag = 10;

	if (uiVars) {
		trap->Cvar_Set("ui_captureLimit", va("%d", cap));
		trap->Cvar_Set("ui_fragLimit", va("%d", frag));
	} else {
		trap->Cvar_Set("capturelimit", va("%d", cap));
		trap->Cvar_Set("fraglimit", va("%d", frag));
	}
}

static const char* UI_GetGameTypeName(int gtEnum)
{
	switch ( gtEnum )
	{
	case GT_FFA:
		return UI_GetStringEdString("MENUS", "FREE_FOR_ALL");//"Free For All";
	case GT_HOLOCRON:
		return UI_GetStringEdString("MENUS", "HOLOCRON_FFA");//"Holocron FFA";
	case GT_JEDIMASTER:
		return UI_GetStringEdString("MENUS", "SAGA");//"Jedi Master";??
	case GT_SINGLE_PLAYER:
		return UI_GetStringEdString("MENUS", "SAGA");//"Team FFA";
	case GT_DUEL:
		return UI_GetStringEdString("MENUS", "DUEL");//"Team FFA";
	case GT_POWERDUEL:
		return UI_GetStringEdString("MENUS", "POWERDUEL");//"Team FFA";
	case GT_TEAM:
		return UI_GetStringEdString("MENUS", "TEAM_FFA");//"Team FFA";
	case GT_SIEGE:
		return UI_GetStringEdString("MENUS", "SIEGE");//"Siege";
	case GT_CTF:
		return UI_GetStringEdString("MENUS", "CAPTURE_THE_FLAG");//"Capture the Flag";
	case GT_CTY:
		return UI_GetStringEdString("MENUS", "CAPTURE_THE_YSALIMARI");//"Capture the Ysalamiri";
	}
	return UI_GetStringEdString("MENUS", "SAGA");//"Team FFA";
}



// ui_gameType assumes gametype 0 is -1 ALL and will not show
static void UI_DrawGameType(rectDef_t *rect, float scale, vec4_t color, int textStyle, int iMenuFont)
{
	Text_Paint(rect->x, rect->y, scale, color, UI_GetGameTypeName(uiInfo.gameTypes[ui_gametype.integer].gtEnum), 0, 0, textStyle, iMenuFont);
}

static void UI_DrawNetGameType(rectDef_t *rect, float scale, vec4_t color, int textStyle, int iMenuFont)
{
	if (ui_netGametype.integer < 0 || ui_netGametype.integer >= uiInfo.numGameTypes)
	{
		trap->Cvar_Set("ui_netGametype", "0");
		trap->Cvar_Update(&ui_netGametype);
		trap->Cvar_Set("ui_actualNetGametype", "0");
		trap->Cvar_Update(&ui_actualNetGametype);
	}
	Text_Paint(rect->x, rect->y, scale, color, UI_GetGameTypeName(uiInfo.gameTypes[ui_netGametype.integer].gtEnum) , 0, 0, textStyle, iMenuFont);
}

static void UI_DrawAutoSwitch(rectDef_t *rect, float scale, vec4_t color, int textStyle, int iMenuFont) {
	int switchVal = trap->Cvar_VariableValue("cg_autoswitch");
	const char *switchString = "AUTOSWITCH1";
	const char *stripString = NULL;

	switch(switchVal)
	{
	case 2:
		switchString = "AUTOSWITCH2";
		break;
	case 3:
		switchString = "AUTOSWITCH3";
		break;
	case 0:
		switchString = "AUTOSWITCH0";
		break;
	default:
		break;
	}

	stripString = UI_GetStringEdString("MP_INGAME", (char *)switchString);

	if (stripString)
	{
		Text_Paint(rect->x, rect->y, scale, color, stripString, 0, 0, textStyle, iMenuFont);
	}
}

static void UI_DrawJoinGameType(rectDef_t *rect, float scale, vec4_t color, int textStyle, int iMenuFont)
{
	if (ui_joinGametype.integer < 0 || ui_joinGametype.integer > uiInfo.numJoinGameTypes)
	{
		trap->Cvar_Set("ui_joinGametype", "0");
		trap->Cvar_Update(&ui_joinGametype);
	}

	Text_Paint(rect->x, rect->y, scale, color, UI_GetGameTypeName(uiInfo.joinGameTypes[ui_joinGametype.integer].gtEnum) , 0, 0, textStyle, iMenuFont);
}

static int UI_TeamIndexFromName(const char *name) {
	int i;

	if (name && *name) {
		for (i = 0; i < uiInfo.teamCount; i++) {
			if (Q_stricmp(name, uiInfo.teamList[i].teamName) == 0) {
				return i;
			}
		}
	}

	return 0;
}

static void UI_DrawClanLogo(rectDef_t *rect, float scale, vec4_t color) {
	int i;
	i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_teamName"));
	if (i >= 0 && i < uiInfo.teamCount) {
		trap->R_SetColor( color );

		if (uiInfo.teamList[i].teamIcon == -1) {
			uiInfo.teamList[i].teamIcon = trap->R_RegisterShaderNoMip(uiInfo.teamList[i].imageName);
			uiInfo.teamList[i].teamIcon_Metal = trap->R_RegisterShaderNoMip(va("%s_metal",uiInfo.teamList[i].imageName));
			uiInfo.teamList[i].teamIcon_Name = trap->R_RegisterShaderNoMip(va("%s_name", uiInfo.teamList[i].imageName));
		}

		UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[i].teamIcon);
		trap->R_SetColor(NULL);
	}
}

static void UI_DrawClanCinematic(rectDef_t *rect, float scale, vec4_t color) {
	int i;
	i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_teamName"));
	if (i >= 0 && i < uiInfo.teamCount) {
		if (uiInfo.teamList[i].cinematic >= -2) {
			if (uiInfo.teamList[i].cinematic == -1) {
				uiInfo.teamList[i].cinematic = trap->CIN_PlayCinematic(va("%s.roq", uiInfo.teamList[i].imageName), 0, 0, 0, 0, (CIN_loop | CIN_silent) );
			}
			if (uiInfo.teamList[i].cinematic >= 0) {
				trap->CIN_RunCinematic(uiInfo.teamList[i].cinematic);
				trap->CIN_SetExtents(uiInfo.teamList[i].cinematic, rect->x, rect->y, rect->w, rect->h);
				trap->CIN_DrawCinematic(uiInfo.teamList[i].cinematic);
			} else {
				trap->R_SetColor( color );
				UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[i].teamIcon_Metal);
				trap->R_SetColor(NULL);
				uiInfo.teamList[i].cinematic = -2;
			}
		} else {
			trap->R_SetColor( color );
			UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[i].teamIcon);
			trap->R_SetColor(NULL);
		}
	}
}

static void UI_DrawPreviewCinematic(rectDef_t *rect, float scale, vec4_t color) {
	if (uiInfo.previewMovie > -2) {
		uiInfo.previewMovie = trap->CIN_PlayCinematic(va("%s.roq", uiInfo.movieList[uiInfo.movieIndex]), 0, 0, 0, 0, (CIN_loop | CIN_silent) );
		if (uiInfo.previewMovie >= 0) {
			trap->CIN_RunCinematic(uiInfo.previewMovie);
			trap->CIN_SetExtents(uiInfo.previewMovie, rect->x, rect->y, rect->w, rect->h);
			trap->CIN_DrawCinematic(uiInfo.previewMovie);
		} else {
			uiInfo.previewMovie = -2;
		}
	}
}

static void UI_DrawSkill(rectDef_t *rect, float scale, vec4_t color, int textStyle, int iMenuFont) {
	int i;
	i = trap->Cvar_VariableValue( "g_spSkill" );
	if (i < 1 || i > numSkillLevels) {
		i = 1;
	}
	Text_Paint(rect->x, rect->y, scale, color, (char *)UI_GetStringEdString("MP_INGAME", (char *)skillLevels[i-1]),0, 0, textStyle, iMenuFont);
}

static void UI_DrawGenericNum(rectDef_t *rect, float scale, vec4_t color, int textStyle, int val, int min, int max, int type,int iMenuFont)
{
	int i;
	char s[256];

	i = val;
	if (i < min || i > max)
	{
		i = min;
	}

	Com_sprintf(s, sizeof(s), "%i\0", val);
	Text_Paint(rect->x, rect->y, scale, color, s,0, 0, textStyle, iMenuFont);
}

static void UI_DrawForceMastery(rectDef_t *rect, float scale, vec4_t color, int textStyle, int val, int min, int max, int iMenuFont)
{
	int i;
	char *s;

	i = val;
	if (i < min)
	{
		i = min;
	}
	if (i > max)
	{
		i = max;
	}

	s = (char *)UI_GetStringEdString("MP_INGAME", forceMasteryLevels[i]);
	Text_Paint(rect->x, rect->y, scale, color, s, 0, 0, textStyle, iMenuFont);
}

static void UI_DrawSkinColor(rectDef_t *rect, float scale, vec4_t color, int textStyle, int val, int min, int max, int iMenuFont)
{
	char s[256];

	switch(val)
	{
	case TEAM_RED:
		trap->SE_GetStringTextString("MENUS_TEAM_RED", s, sizeof(s));
//		Com_sprintf(s, sizeof(s), "Red\0");
		break;
	case TEAM_BLUE:
		trap->SE_GetStringTextString("MENUS_TEAM_BLUE", s, sizeof(s));
//		Com_sprintf(s, sizeof(s), "Blue\0");
		break;
	default:
		trap->SE_GetStringTextString("MENUS_DEFAULT", s, sizeof(s));
//		Com_sprintf(s, sizeof(s), "Default\0");
		break;
	}

	Text_Paint(rect->x, rect->y, scale, color, s, 0, 0, textStyle, iMenuFont);
}

static void UI_DrawForceSide(rectDef_t *rect, float scale, vec4_t color, int textStyle, int val, int min, int max, int iMenuFont)
{
	char s[256];
	menuDef_t *menu;

	char info[MAX_INFO_VALUE];

	info[0] = '\0';
	trap->GetConfigString(CS_SERVERINFO, info, sizeof(info));

	if (atoi( Info_ValueForKey( info, "g_forceBasedTeams" ) ))
	{
		switch((int)(trap->Cvar_VariableValue("ui_myteam")))
		{
		case TEAM_RED:
			uiForceSide = FORCE_DARKSIDE;
			color[0] = 0.2f;
			color[1] = 0.2f;
			color[2] = 0.2f;
			break;
		case TEAM_BLUE:
			uiForceSide = FORCE_LIGHTSIDE;
			color[0] = 0.2f;
			color[1] = 0.2f;
			color[2] = 0.2f;
			break;
		default:
			break;
		}
	}

	if (val == FORCE_LIGHTSIDE)
	{
		trap->SE_GetStringTextString("MENUS_FORCEDESC_LIGHT",s, sizeof(s));
		menu = Menus_FindByName("forcealloc");
		if (menu)
		{
			Menu_ShowItemByName(menu, "lightpowers", qtrue);
			Menu_ShowItemByName(menu, "darkpowers", qfalse);
			Menu_ShowItemByName(menu, "darkpowers_team", qfalse);

			Menu_ShowItemByName(menu, "lightpowers_team", qtrue);//(ui_gameType.integer >= GT_TEAM));

		}
		menu = Menus_FindByName("ingame_playerforce");
		if (menu)
		{
			Menu_ShowItemByName(menu, "lightpowers", qtrue);
			Menu_ShowItemByName(menu, "darkpowers", qfalse);
			Menu_ShowItemByName(menu, "darkpowers_team", qfalse);

			Menu_ShowItemByName(menu, "lightpowers_team", qtrue);//(ui_gameType.integer >= GT_TEAM));
		}
	}
	else
	{
		trap->SE_GetStringTextString("MENUS_FORCEDESC_DARK",s, sizeof(s));
		menu = Menus_FindByName("forcealloc");
		if (menu)
		{
			Menu_ShowItemByName(menu, "lightpowers", qfalse);
			Menu_ShowItemByName(menu, "lightpowers_team", qfalse);
			Menu_ShowItemByName(menu, "darkpowers", qtrue);

			Menu_ShowItemByName(menu, "darkpowers_team", qtrue);//(ui_gameType.integer >= GT_TEAM));
		}
		menu = Menus_FindByName("ingame_playerforce");
		if (menu)
		{
			Menu_ShowItemByName(menu, "lightpowers", qfalse);
			Menu_ShowItemByName(menu, "lightpowers_team", qfalse);
			Menu_ShowItemByName(menu, "darkpowers", qtrue);

			Menu_ShowItemByName(menu, "darkpowers_team", qtrue);//(ui_gameType.integer >= GT_TEAM));
		}
	}

	Text_Paint(rect->x, rect->y, scale, color, s,0, 0, textStyle, iMenuFont);
}

qboolean UI_HasSetSaberOnly( const char *info, const int gametype )
{
	int i = 0;
	int wDisable = 0;

	if ( gametype == GT_JEDIMASTER )
	{ //set to 0
		return qfalse;
	}

	if (gametype == GT_DUEL || gametype == GT_POWERDUEL)
	{
		wDisable = atoi(Info_ValueForKey(info, "g_duelWeaponDisable"));
	}
	else
	{
		wDisable = atoi(Info_ValueForKey(info, "g_weaponDisable"));
	}

	while (i < WP_NUM_WEAPONS)
	{
		if (!(wDisable & (1 << i)) &&
			i != WP_SABER && i != WP_NONE)
		{
			return qfalse;
		}

		i++;
	}

	return qtrue;
}

static qboolean UI_AllForceDisabled(int force)
{
	int i;

	if (force)
	{
		for (i=0;i<NUM_FORCE_POWERS;i++)
		{
			if (!(force & (1<<i)))
			{
				return qfalse;
			}
		}

		return qtrue;
	}

	return qfalse;
}

qboolean UI_TrueJediEnabled( void )
{
	char	info[MAX_INFO_STRING] = {0};
	int		gametype = 0, disabledForce = 0, trueJedi = 0;
	qboolean saberOnly = qfalse, allForceDisabled = qfalse;

	trap->GetConfigString( CS_SERVERINFO, info, sizeof(info) );

	//already have serverinfo at this point for stuff below. Don't bother trying to use ui_forcePowerDisable.
	//if (ui_forcePowerDisable.integer)
	//if (atoi(Info_ValueForKey(info, "g_forcePowerDisable")))
	disabledForce = atoi(Info_ValueForKey(info, "g_forcePowerDisable"));
	allForceDisabled = UI_AllForceDisabled(disabledForce);
	gametype = atoi(Info_ValueForKey(info, "g_gametype"));
	saberOnly = UI_HasSetSaberOnly(info, gametype);

	if ( gametype == GT_HOLOCRON
		|| gametype == GT_JEDIMASTER
		|| saberOnly
		|| allForceDisabled )
	{
		trueJedi = 0;
	}
	else
	{
		trueJedi = atoi( Info_ValueForKey( info, "g_jediVmerc" ) );
	}
	return (trueJedi != 0);
}

static void UI_DrawJediNonJedi(rectDef_t *rect, float scale, vec4_t color, int textStyle, int val, int min, int max, int iMenuFont)
{
	int i;
	char s[256];
	//menuDef_t *menu;

	char info[MAX_INFO_VALUE];

	i = val;
	if (i < min || i > max)
	{
		i = min;
	}

	info[0] = '\0';
	trap->GetConfigString(CS_SERVERINFO, info, sizeof(info));

	if ( !UI_TrueJediEnabled() )
	{//true jedi mode is not on, do not draw this button type
		return;
	}

	if ( val == FORCE_NONJEDI )
		trap->SE_GetStringTextString("MENUS_NO",s, sizeof(s));
	else
		trap->SE_GetStringTextString("MENUS_YES",s, sizeof(s));

	Text_Paint(rect->x, rect->y, scale, color, s,0, 0, textStyle, iMenuFont);
}

static void UI_DrawTeamName(rectDef_t *rect, float scale, vec4_t color, qboolean blue, int textStyle, int iMenuFont) {
	int i;
	i = UI_TeamIndexFromName(UI_Cvar_VariableString((blue) ? "ui_blueTeam" : "ui_redTeam"));
	if (i >= 0 && i < uiInfo.teamCount) {
		Text_Paint(rect->x, rect->y, scale, color, va("%s: %s", (blue) ? "Blue" : "Red", uiInfo.teamList[i].teamName),0, 0, textStyle, iMenuFont);
	}
}

static void UI_DrawTeamMember(rectDef_t *rect, float scale, vec4_t color, qboolean blue, int num, int textStyle, int iMenuFont)
{
	// 0 - None
	// 1 - Human
	// 2..NumCharacters - Bot
	int value = trap->Cvar_VariableValue(va(blue ? "ui_blueteam%i" : "ui_redteam%i", num));
	const char *text;
	int maxcl = trap->Cvar_VariableValue( "sv_maxClients" );
	vec4_t finalColor;
	int numval = num;

	numval *= 2;

	if (blue)
	{
		numval -= 1;
	}

	finalColor[0] = color[0];
	finalColor[1] = color[1];
	finalColor[2] = color[2];
	finalColor[3] = color[3];

	if (numval > maxcl)
	{
		finalColor[0] *= 0.5;
		finalColor[1] *= 0.5;
		finalColor[2] *= 0.5;

		value = -1;
	}

	if (uiInfo.gameTypes[ui_netGametype.integer].gtEnum == GT_SIEGE)
	{
		if (value > 1 )
		{
			value = 1;
		}
	}

	if (value <= 1) {
		if (value == -1)
		{
			//text = "Closed";
			text = UI_GetStringEdString("MENUS", "CLOSED");
		}
		else
		{
			//text = "Human";
			text = UI_GetStringEdString("MENUS", "HUMAN");
		}
	} else {
		value -= 2;
		if (value >= UI_GetNumBots()) {
			value = 1;
		}
		text = UI_GetBotNameByNumber(value);
	}

	Text_Paint(rect->x, rect->y, scale, finalColor, text, 0, 0, textStyle, iMenuFont);
}

static void UI_DrawMapPreview(rectDef_t *rect, float scale, vec4_t color, qboolean net) {
	int map = (net) ? ui_currentNetMap.integer : ui_currentMap.integer;
	if (map < 0 || map > uiInfo.mapCount) {
		if (net) {
			trap->Cvar_Set("ui_currentNetMap", "0");
			trap->Cvar_Update(&ui_currentNetMap);
		} else {
			trap->Cvar_Set("ui_currentMap", "0");
			trap->Cvar_Update(&ui_currentMap);
		}
		map = 0;
	}

	if (uiInfo.mapList[map].levelShot == -1) {
		uiInfo.mapList[map].levelShot = trap->R_RegisterShaderNoMip(uiInfo.mapList[map].imageName);
	}

	if (uiInfo.mapList[map].levelShot > 0) {
		UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.mapList[map].levelShot);
	} else {
		UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, trap->R_RegisterShaderNoMip("menu/art/unknownmap_mp"));
	}
}

static void UI_DrawMapCinematic(rectDef_t *rect, float scale, vec4_t color, qboolean net) {
	int map = (net) ? ui_currentNetMap.integer : ui_currentMap.integer;
	if (map < 0 || map > uiInfo.mapCount) {
		if (net) {
			trap->Cvar_Set("ui_currentNetMap", "0");
			trap->Cvar_Update(&ui_currentNetMap);
		} else {
			trap->Cvar_Set("ui_currentMap", "0");
			trap->Cvar_Update(&ui_currentMap);
		}
		map = 0;
	}

	if (uiInfo.mapList[map].cinematic >= -1) {
		if (uiInfo.mapList[map].cinematic == -1) {
			uiInfo.mapList[map].cinematic = trap->CIN_PlayCinematic(va("%s.roq", uiInfo.mapList[map].mapLoadName), 0, 0, 0, 0, (CIN_loop | CIN_silent) );
		}
		if (uiInfo.mapList[map].cinematic >= 0) {
			trap->CIN_RunCinematic(uiInfo.mapList[map].cinematic);
			trap->CIN_SetExtents(uiInfo.mapList[map].cinematic, rect->x, rect->y, rect->w, rect->h);
			trap->CIN_DrawCinematic(uiInfo.mapList[map].cinematic);
		} else {
			uiInfo.mapList[map].cinematic = -2;
		}
	} else {
		UI_DrawMapPreview(rect, scale, color, net);
	}
}

static void UI_SetForceDisabled(int force)
{
	int i = 0;

	if (force)
	{
		while (i < NUM_FORCE_POWERS)
		{
			if (force & (1 << i))
			{
				uiForcePowersDisabled[i] = qtrue;

				if (i != FP_LEVITATION && i != FP_SABER_OFFENSE && i != FP_SABER_DEFENSE)
				{
					uiForcePowersRank[i] = 0;
				}
				else
				{
					if (i == FP_LEVITATION)
					{
						uiForcePowersRank[i] = 1;
					}
					else
					{
						uiForcePowersRank[i] = 3;
					}
				}
			}
			else
			{
				uiForcePowersDisabled[i] = qfalse;
			}
			i++;
		}
	}
	else
	{
		i = 0;

		while (i < NUM_FORCE_POWERS)
		{
			uiForcePowersDisabled[i] = qfalse;
			i++;
		}
	}
}
// The game type on create server has changed - make the HUMAN/BOTS fields active
void UpdateBotButtons(void)
{
	menuDef_t *menu;

	menu = Menu_GetFocused();

	if (!menu)
	{
		return;
	}

	if (uiInfo.gameTypes[ui_netGametype.integer].gtEnum == GT_SIEGE)
	{
		Menu_ShowItemByName(menu, "humanbotfield", qfalse);
		Menu_ShowItemByName(menu, "humanbotnonfield", qtrue);
	}
	else
	{
		Menu_ShowItemByName(menu, "humanbotfield", qtrue);
		Menu_ShowItemByName(menu, "humanbotnonfield", qfalse);
	}

}

void UpdateForceStatus()
{
	menuDef_t *menu;

	// Currently we don't make a distinction between those that wish to play Jedi of lower than maximum skill.
/*	if (ui_forcePowerDisable.integer)
	{
		uiForceRank = 0;
		uiForceAvailable = 0;
		uiForceUsed = 0;
	}
	else
	{
		uiForceRank = uiMaxRank;
		uiForceUsed = 0;
		uiForceAvailable = forceMasteryPoints[uiForceRank];
	}
*/
	menu = Menus_FindByName("ingame_player");
	if (menu)
	{
		char	info[MAX_INFO_STRING];
		int		disabledForce = 0;
		qboolean trueJedi = qfalse, allForceDisabled = qfalse;

		trap->GetConfigString( CS_SERVERINFO, info, sizeof(info) );

		//already have serverinfo at this point for stuff below. Don't bother trying to use ui_forcePowerDisable.
		//if (ui_forcePowerDisable.integer)
		//if (atoi(Info_ValueForKey(info, "g_forcePowerDisable")))
		disabledForce = atoi(Info_ValueForKey(info, "g_forcePowerDisable"));
		allForceDisabled = UI_AllForceDisabled(disabledForce);
		trueJedi = UI_TrueJediEnabled();

		if ( !trueJedi || allForceDisabled )
		{
			Menu_ShowItemByName(menu, "jedinonjedi", qfalse);
		}
		else
		{
			Menu_ShowItemByName(menu, "jedinonjedi", qtrue);
		}
		if ( allForceDisabled == qtrue || (trueJedi && uiJediNonJedi == FORCE_NONJEDI) )
		{	// No force stuff
			Menu_ShowItemByName(menu, "noforce", qtrue);
			Menu_ShowItemByName(menu, "yesforce", qfalse);
			// We don't want the saber explanation to say "configure saber attack 1" since we can't.
			Menu_ShowItemByName(menu, "sabernoneconfigme", qfalse);
		}
		else
		{
			UI_SetForceDisabled(disabledForce);
			Menu_ShowItemByName(menu, "noforce", qfalse);
			Menu_ShowItemByName(menu, "yesforce", qtrue);
		}

		//Moved this to happen after it's done with force power disabling stuff
		if (uiForcePowersRank[FP_SABER_OFFENSE] > 0 || ui_freeSaber.integer)
		{	// Show lightsaber stuff.
			Menu_ShowItemByName(menu, "nosaber", qfalse);
			Menu_ShowItemByName(menu, "yessaber", qtrue);
		}
		else
		{
			Menu_ShowItemByName(menu, "nosaber", qtrue);
			Menu_ShowItemByName(menu, "yessaber", qfalse);
		}

		// The leftmost button should be "apply" unless you are in spectator, where you can join any team.
		if ((int)(trap->Cvar_VariableValue("ui_myteam")) != TEAM_SPECTATOR)
		{
			Menu_ShowItemByName(menu, "playerapply", qtrue);
			Menu_ShowItemByName(menu, "playerforcejoin", qfalse);
			Menu_ShowItemByName(menu, "playerforcered", qtrue);
			Menu_ShowItemByName(menu, "playerforceblue", qtrue);
			Menu_ShowItemByName(menu, "playerforcespectate", qtrue);
		}
		else
		{
			// Set or reset buttons based on choices
			if (atoi(Info_ValueForKey(info, "g_gametype")) >= GT_TEAM)
			{	// This is a team-based game.
				Menu_ShowItemByName(menu, "playerforcespectate", qtrue);

				// This is disabled, always show both sides from spectator.
				if ( 0 && atoi(Info_ValueForKey(info, "g_forceBasedTeams")))
				{	// Show red or blue based on what side is chosen.
					if (uiForceSide==FORCE_LIGHTSIDE)
					{
						Menu_ShowItemByName(menu, "playerforcered", qfalse);
						Menu_ShowItemByName(menu, "playerforceblue", qtrue);
					}
					else if (uiForceSide==FORCE_DARKSIDE)
					{
						Menu_ShowItemByName(menu, "playerforcered", qtrue);
						Menu_ShowItemByName(menu, "playerforceblue", qfalse);
					}
					else
					{
						Menu_ShowItemByName(menu, "playerforcered", qtrue);
						Menu_ShowItemByName(menu, "playerforceblue", qtrue);
					}
				}
				else
				{
					Menu_ShowItemByName(menu, "playerforcered", qtrue);
					Menu_ShowItemByName(menu, "playerforceblue", qtrue);
				}
			}
			else
			{
				Menu_ShowItemByName(menu, "playerforcered", qfalse);
				Menu_ShowItemByName(menu, "playerforceblue", qfalse);
			}

			Menu_ShowItemByName(menu, "playerapply", qfalse);
			Menu_ShowItemByName(menu, "playerforcejoin", qtrue);
			Menu_ShowItemByName(menu, "playerforcespectate", qtrue);
		}
	}

	if ( !UI_TrueJediEnabled() )
	{// Take the current team and force a skin color based on it.
		char	info[MAX_INFO_STRING];

		switch((int)(trap->Cvar_VariableValue("ui_myteam")))
		{
		case TEAM_RED:
			uiSkinColor = TEAM_RED;
			break;
		case TEAM_BLUE:
			uiSkinColor = TEAM_BLUE;
			break;
		default:
			trap->GetConfigString( CS_SERVERINFO, info, sizeof(info) );

			if (atoi(Info_ValueForKey(info, "g_gametype")) >= GT_TEAM)
			{
				uiSkinColor = TEAM_FREE;
			}
			else	// A bit of a hack so non-team games will remember which skin set you chose in the player menu
			{
				uiSkinColor = uiHoldSkinColor;
			}
			break;
		}
	}
}

static void UI_DrawNetSource(rectDef_t *rect, float scale, vec4_t color, int textStyle, int iMenuFont)
{
	if (ui_netSource.integer < 0 || ui_netSource.integer >= numNetSources) {
		trap->Cvar_Set("ui_netSource", "0");
		trap->Cvar_Update(&ui_netSource);
	}

	trap->SE_GetStringTextString("MENUS_SOURCE", holdSPString, sizeof(holdSPString) );
	Text_Paint(rect->x, rect->y, scale, color, va("%s %s",holdSPString,
		GetNetSourceString(ui_netSource.integer)), 0, 0, textStyle, iMenuFont);
}

static void UI_DrawNetMapPreview(rectDef_t *rect, float scale, vec4_t color) {
	if (uiInfo.serverStatus.currentServerPreview > 0) {
		UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.serverStatus.currentServerPreview);
	} else {
		UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, trap->R_RegisterShaderNoMip("menu/art/unknownmap_mp"));
	}
}

static void UI_DrawNetMapCinematic(rectDef_t *rect, float scale, vec4_t color) {
	if (ui_currentNetMap.integer < 0 || ui_currentNetMap.integer > uiInfo.mapCount) {
		trap->Cvar_Set("ui_currentNetMap", "0");
		trap->Cvar_Update(&ui_currentNetMap);
	}

	if (uiInfo.serverStatus.currentServerCinematic >= 0) {
		trap->CIN_RunCinematic(uiInfo.serverStatus.currentServerCinematic);
		trap->CIN_SetExtents(uiInfo.serverStatus.currentServerCinematic, rect->x, rect->y, rect->w, rect->h);
		trap->CIN_DrawCinematic(uiInfo.serverStatus.currentServerCinematic);
	} else {
		UI_DrawNetMapPreview(rect, scale, color);
	}
}

static void UI_DrawNetFilter(rectDef_t *rect, float scale, vec4_t color, int textStyle, int iMenuFont)
{
	trap->SE_GetStringTextString("MENUS_GAME", holdSPString, sizeof(holdSPString));

	Text_Paint(rect->x, rect->y, scale, color, va("%s %s",holdSPString, UI_FilterDescription( ui_serverFilterType.integer )), 0, 0, textStyle, iMenuFont);
}

static void UI_DrawTier(rectDef_t *rect, float scale, vec4_t color, int textStyle, int iMenuFont) {
	int i;
	i = trap->Cvar_VariableValue( "ui_currentTier" );
	if (i < 0 || i >= uiInfo.tierCount) {
		i = 0;
	}
	Text_Paint(rect->x, rect->y, scale, color, va("Tier: %s", uiInfo.tierList[i].tierName),0, 0, textStyle, iMenuFont);
}

static void UI_DrawTierMap(rectDef_t *rect, int index) {
	int i;
	i = trap->Cvar_VariableValue( "ui_currentTier" );
	if (i < 0 || i >= uiInfo.tierCount) {
		i = 0;
	}

	if (uiInfo.tierList[i].mapHandles[index] == -1) {
		uiInfo.tierList[i].mapHandles[index] = trap->R_RegisterShaderNoMip(va("levelshots/%s", uiInfo.tierList[i].maps[index]));
	}

	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.tierList[i].mapHandles[index]);
}

static const char *UI_EnglishMapName(const char *map) {
	int i;
	for (i = 0; i < uiInfo.mapCount; i++) {
		if (Q_stricmp(map, uiInfo.mapList[i].mapLoadName) == 0) {
			return uiInfo.mapList[i].mapName;
		}
	}
	return "";
}

static void UI_DrawTierMapName(rectDef_t *rect, float scale, vec4_t color, int textStyle, int iMenuFont) {
	int i, j;
	i = trap->Cvar_VariableValue( "ui_currentTier" );
	if (i < 0 || i >= uiInfo.tierCount) {
		i = 0;
	}
	j = trap->Cvar_VariableValue("ui_currentMap");
	if (j < 0 || j >= MAPS_PER_TIER) {
		j = 0;
	}

	Text_Paint(rect->x, rect->y, scale, color, UI_EnglishMapName(uiInfo.tierList[i].maps[j]), 0, 0, textStyle, iMenuFont);
}

static void UI_DrawTierGameType(rectDef_t *rect, float scale, vec4_t color, int textStyle, int iMenuFont) {
	int i, j;
	i = trap->Cvar_VariableValue( "ui_currentTier" );
	if (i < 0 || i >= uiInfo.tierCount) {
		i = 0;
	}
	j = trap->Cvar_VariableValue("ui_currentMap");
	if (j < 0 || j >= MAPS_PER_TIER) {
		j = 0;
	}

	Text_Paint(rect->x, rect->y, scale, color, uiInfo.gameTypes[uiInfo.tierList[i].gameTypes[j]].gameType , 0, 0, textStyle,iMenuFont);
}

static const char *UI_AIFromName(const char *name) {
	int j;
	for (j = 0; j < uiInfo.aliasCount; j++) {
		if (Q_stricmp(uiInfo.aliasList[j].name, name) == 0) {
			return uiInfo.aliasList[j].ai;
		}
	}
	return "Kyle";
}

/*
static qboolean updateOpponentModel = qtrue;
static void UI_DrawOpponent(rectDef_t *rect) {
  static playerInfo_t info2;
  char model[MAX_QPATH];
  char headmodel[MAX_QPATH];
  char team[256];
	vec3_t	viewangles;
	vec3_t	moveangles;

	if (updateOpponentModel) {

		strcpy(model, UI_Cvar_VariableString("ui_opponentModel"));
	  strcpy(headmodel, UI_Cvar_VariableString("ui_opponentModel"));
		team[0] = '\0';

  	memset( &info2, 0, sizeof(playerInfo_t) );
  	viewangles[YAW]   = 180 - 10;
  	viewangles[PITCH] = 0;
  	viewangles[ROLL]  = 0;
  	VectorClear( moveangles );
    UI_PlayerInfo_SetModel( &info2, model, headmodel, "");
    UI_PlayerInfo_SetInfo( &info2, TORSO_WEAPONREADY3, TORSO_WEAPONREADY3, viewangles, vec3_origin, WP_BRYAR_PISTOL, qfalse );
		UI_RegisterClientModelname( &info2, model, headmodel, team);
    updateOpponentModel = qfalse;
  }

  UI_DrawPlayer( rect->x, rect->y, rect->w, rect->h, &info2, uiInfo.uiDC.realTime / 2);

}
*/
static void UI_NextOpponent() {
	int i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_opponentName"));
	int j = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_teamName"));
	i++;
	if (i >= uiInfo.teamCount) {
		i = 0;
	}
	if (i == j) {
		i++;
		if ( i >= uiInfo.teamCount) {
			i = 0;
		}
	}
 	trap->Cvar_Set( "ui_opponentName", uiInfo.teamList[i].teamName );
}

static void UI_PriorOpponent() {
	int i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_opponentName"));
	int j = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_teamName"));
	i--;
	if (i < 0) {
		i = uiInfo.teamCount - 1;
	}
	if (i == j) {
		i--;
		if ( i < 0) {
			i = uiInfo.teamCount - 1;
		}
	}
 	trap->Cvar_Set( "ui_opponentName", uiInfo.teamList[i].teamName );
}

static void	UI_DrawPlayerLogo(rectDef_t *rect, vec3_t color) {
	int i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_teamName"));
	if (uiInfo.teamList[i].teamIcon == -1) {
		uiInfo.teamList[i].teamIcon = trap->R_RegisterShaderNoMip(uiInfo.teamList[i].imageName);
		uiInfo.teamList[i].teamIcon_Metal = trap->R_RegisterShaderNoMip(va("%s_metal", uiInfo.teamList[i].imageName));
		uiInfo.teamList[i].teamIcon_Name = trap->R_RegisterShaderNoMip(va("%s_name", uiInfo.teamList[i].imageName));
	}

	trap->R_SetColor(color);
	UI_DrawHandlePic(rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[i].teamIcon);
	trap->R_SetColor(NULL);
}

static void	UI_DrawPlayerLogoMetal(rectDef_t *rect, vec3_t color) {
	int i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_teamName"));
	if (uiInfo.teamList[i].teamIcon == -1) {
		uiInfo.teamList[i].teamIcon = trap->R_RegisterShaderNoMip(uiInfo.teamList[i].imageName);
		uiInfo.teamList[i].teamIcon_Metal = trap->R_RegisterShaderNoMip(va("%s_metal", uiInfo.teamList[i].imageName));
		uiInfo.teamList[i].teamIcon_Name = trap->R_RegisterShaderNoMip(va("%s_name", uiInfo.teamList[i].imageName));
	}

	trap->R_SetColor(color);
	UI_DrawHandlePic(rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[i].teamIcon_Metal);
	trap->R_SetColor(NULL);
}

static void	UI_DrawPlayerLogoName(rectDef_t *rect, vec3_t color) {
	int i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_teamName"));
	if (uiInfo.teamList[i].teamIcon == -1) {
		uiInfo.teamList[i].teamIcon = trap->R_RegisterShaderNoMip(uiInfo.teamList[i].imageName);
		uiInfo.teamList[i].teamIcon_Metal = trap->R_RegisterShaderNoMip(va("%s_metal", uiInfo.teamList[i].imageName));
		uiInfo.teamList[i].teamIcon_Name = trap->R_RegisterShaderNoMip(va("%s_name", uiInfo.teamList[i].imageName));
	}

	trap->R_SetColor(color);
	UI_DrawHandlePic(rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[i].teamIcon_Name);
	trap->R_SetColor(NULL);
}

static void	UI_DrawOpponentLogo(rectDef_t *rect, vec3_t color) {
	int i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_opponentName"));
	if (uiInfo.teamList[i].teamIcon == -1) {
		uiInfo.teamList[i].teamIcon = trap->R_RegisterShaderNoMip(uiInfo.teamList[i].imageName);
		uiInfo.teamList[i].teamIcon_Metal = trap->R_RegisterShaderNoMip(va("%s_metal", uiInfo.teamList[i].imageName));
		uiInfo.teamList[i].teamIcon_Name = trap->R_RegisterShaderNoMip(va("%s_name", uiInfo.teamList[i].imageName));
	}

	trap->R_SetColor(color);
	UI_DrawHandlePic(rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[i].teamIcon);
	trap->R_SetColor(NULL);
}

static void	UI_DrawOpponentLogoMetal(rectDef_t *rect, vec3_t color) {
	int i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_opponentName"));
	if (uiInfo.teamList[i].teamIcon == -1) {
		uiInfo.teamList[i].teamIcon = trap->R_RegisterShaderNoMip(uiInfo.teamList[i].imageName);
		uiInfo.teamList[i].teamIcon_Metal = trap->R_RegisterShaderNoMip(va("%s_metal", uiInfo.teamList[i].imageName));
		uiInfo.teamList[i].teamIcon_Name = trap->R_RegisterShaderNoMip(va("%s_name", uiInfo.teamList[i].imageName));
	}

	trap->R_SetColor(color);
	UI_DrawHandlePic(rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[i].teamIcon_Metal);
	trap->R_SetColor(NULL);
}

static void	UI_DrawOpponentLogoName(rectDef_t *rect, vec3_t color) {
	int i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_opponentName"));
	if (uiInfo.teamList[i].teamIcon == -1) {
		uiInfo.teamList[i].teamIcon = trap->R_RegisterShaderNoMip(uiInfo.teamList[i].imageName);
		uiInfo.teamList[i].teamIcon_Metal = trap->R_RegisterShaderNoMip(va("%s_metal", uiInfo.teamList[i].imageName));
		uiInfo.teamList[i].teamIcon_Name = trap->R_RegisterShaderNoMip(va("%s_name", uiInfo.teamList[i].imageName));
	}

	trap->R_SetColor(color);
	UI_DrawHandlePic(rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[i].teamIcon_Name);
	trap->R_SetColor(NULL);
}

static void UI_DrawAllMapsSelection(rectDef_t *rect, float scale, vec4_t color, int textStyle, qboolean net, int iMenuFont) {
	int map = (net) ? ui_currentNetMap.integer : ui_currentMap.integer;
	if (map >= 0 && map < uiInfo.mapCount) {
		Text_Paint(rect->x, rect->y, scale, color, uiInfo.mapList[map].mapName, 0, 0, textStyle, iMenuFont);
	}
}

static void UI_DrawOpponentName(rectDef_t *rect, float scale, vec4_t color, int textStyle, int iMenuFont) {
	Text_Paint(rect->x, rect->y, scale, color, UI_Cvar_VariableString("ui_opponentName"), 0, 0, textStyle, iMenuFont);
}

static int UI_OwnerDrawWidth(int ownerDraw, float scale) {
	int i, h, value, findex, iUse = 0;
	const char *text;
	const char *s = NULL;

	switch (ownerDraw) {
    case UI_HANDICAP:
			  h = Com_Clamp( 5, 100, trap->Cvar_VariableValue("handicap") );
				i = 20 - h / 5;
				s = handicapValues[i];
      break;
    case UI_SKIN_COLOR:
		switch(uiSkinColor)
		{
		case TEAM_RED:
//			s = "Red";
			s = (char *)UI_GetStringEdString("MENUS", "TEAM_RED");
			break;
		case TEAM_BLUE:
//			s = "Blue";
			s = (char *)UI_GetStringEdString("MENUS", "TEAM_BLUE");
			break;
		default:
//			s = "Default";
			s = (char *)UI_GetStringEdString("MENUS", "DEFAULT");
			break;
		}
		break;
    case UI_FORCE_SIDE:
		i = uiForceSide;
		if (i < 1 || i > 2) {
			i = 1;
		}

		if (i == FORCE_LIGHTSIDE)
		{
//			s = "Light";
			s = (char *)UI_GetStringEdString("MENUS", "FORCEDESC_LIGHT");
		}
		else
		{
//			s = "Dark";
			s = (char *)UI_GetStringEdString("MENUS", "FORCEDESC_DARK");
		}
		break;
    case UI_JEDI_NONJEDI:
		i = uiJediNonJedi;
		if (i < 0 || i > 1)
		{
			i = 0;
		}

		if (i == FORCE_NONJEDI)
		{
//			s = "Non-Jedi";
			s = (char *)UI_GetStringEdString("MENUS", "NO");
		}
		else
		{
//			s = "Jedi";
			s = (char *)UI_GetStringEdString("MENUS", "YES");
		}
		break;
    case UI_FORCE_RANK:
		i = uiForceRank;
		if (i < 1 || i > MAX_FORCE_RANK) {
			i = 1;
		}

		s = (char *)UI_GetStringEdString("MP_INGAME", forceMasteryLevels[i]);
		break;
	case UI_FORCE_RANK_HEAL:
	case UI_FORCE_RANK_LEVITATION:
	case UI_FORCE_RANK_SPEED:
	case UI_FORCE_RANK_PUSH:
	case UI_FORCE_RANK_PULL:
	case UI_FORCE_RANK_TELEPATHY:
	case UI_FORCE_RANK_GRIP:
	case UI_FORCE_RANK_LIGHTNING:
	case UI_FORCE_RANK_RAGE:
	case UI_FORCE_RANK_PROTECT:
	case UI_FORCE_RANK_ABSORB:
	case UI_FORCE_RANK_TEAM_HEAL:
	case UI_FORCE_RANK_TEAM_FORCE:
	case UI_FORCE_RANK_DRAIN:
	case UI_FORCE_RANK_SEE:
	case UI_FORCE_RANK_SABERATTACK:
	case UI_FORCE_RANK_SABERDEFEND:
	case UI_FORCE_RANK_SABERTHROW:
		findex = (ownerDraw - UI_FORCE_RANK)-1;
		//this will give us the index as long as UI_FORCE_RANK is always one below the first force rank index
		i = uiForcePowersRank[findex];

		if (i < 0 || i > NUM_FORCE_POWER_LEVELS-1)
		{
			i = 0;
		}

		s = va("%i", uiForcePowersRank[findex]);
		break;
    case UI_CLANNAME:
		s = UI_Cvar_VariableString("ui_teamName");
      break;
    case UI_GAMETYPE:
		s = uiInfo.gameTypes[ui_gametype.integer].gameType;
      break;
    case UI_SKILL:
		i = trap->Cvar_VariableValue( "g_spSkill" );
		if (i < 1 || i > numSkillLevels) {
			i = 1;
		}
		s = (char *)UI_GetStringEdString("MP_INGAME", (char *)skillLevels[i-1]);
      break;
    case UI_BLUETEAMNAME:
		i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_blueTeam"));
		if (i >= 0 && i < uiInfo.teamCount) {
			s = va("%s: %s", (char *)UI_GetStringEdString("MENUS", "TEAM_BLUE"), uiInfo.teamList[i].teamName);
		}
      break;
    case UI_REDTEAMNAME:
		i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_redTeam"));
		if (i >= 0 && i < uiInfo.teamCount) {
			s = va("%s: %s",  (char *)UI_GetStringEdString("MENUS", "TEAM_RED"), uiInfo.teamList[i].teamName);
		}
      break;
    case UI_BLUETEAM1:
		case UI_BLUETEAM2:
		case UI_BLUETEAM3:
		case UI_BLUETEAM4:
		case UI_BLUETEAM5:
		case UI_BLUETEAM6:
		case UI_BLUETEAM7:
		case UI_BLUETEAM8:
			if (ownerDraw <= UI_BLUETEAM5)
			{
			  iUse = ownerDraw-UI_BLUETEAM1 + 1;
			}
			else
			{
			  iUse = ownerDraw-274; //unpleasent hack because I don't want to move up all the UI_BLAHTEAM# defines
			}

			value = trap->Cvar_VariableValue(va("ui_blueteam%i", iUse));
			if (value <= 1) {
				text = "Human";
			} else {
				value -= 2;
				if (value >= uiInfo.aliasCount) {
					value = 1;
				}
				text = uiInfo.aliasList[value].name;
			}
			s = va("%i. %s", iUse, text);
      break;
    case UI_REDTEAM1:
		case UI_REDTEAM2:
		case UI_REDTEAM3:
		case UI_REDTEAM4:
		case UI_REDTEAM5:
		case UI_REDTEAM6:
		case UI_REDTEAM7:
		case UI_REDTEAM8:
			if (ownerDraw <= UI_REDTEAM5)
			{
			  iUse = ownerDraw-UI_REDTEAM1 + 1;
			}
			else
			{
			  iUse = ownerDraw-277; //unpleasent hack because I don't want to move up all the UI_BLAHTEAM# defines
			}

			value = trap->Cvar_VariableValue(va("ui_redteam%i", iUse));
			if (value <= 1) {
				text = "Human";
			} else {
				value -= 2;
				if (value >= uiInfo.aliasCount) {
					value = 1;
				}
				text = uiInfo.aliasList[value].name;
			}
			s = va("%i. %s", iUse, text);
      break;
		case UI_NETSOURCE:
			if (ui_netSource.integer < 0 || ui_netSource.integer >= numNetSources) {
				trap->Cvar_Set("ui_netSource", "0");
				trap->Cvar_Update(&ui_netSource);
			}
			trap->SE_GetStringTextString("MENUS_SOURCE", holdSPString, sizeof(holdSPString));
			s = va("%s %s", holdSPString, GetNetSourceString(ui_netSource.integer));
			break;
		case UI_NETFILTER:
			trap->SE_GetStringTextString("MENUS_GAME", holdSPString, sizeof(holdSPString));
			s = va("%s %s", holdSPString, UI_FilterDescription( ui_serverFilterType.integer ) );
			break;
		case UI_TIER:
			break;
		case UI_TIER_MAPNAME:
			break;
		case UI_TIER_GAMETYPE:
			break;
		case UI_ALLMAPS_SELECTION:
			break;
		case UI_OPPONENT_NAME:
			break;
		case UI_KEYBINDSTATUS:
			if (Display_KeyBindPending()) {
				s = UI_GetStringEdString("MP_INGAME", "WAITING_FOR_NEW_KEY");
			} else {
			//	s = "Press ENTER or CLICK to change, Press BACKSPACE to clear";
			}
			break;
		case UI_SERVERREFRESHDATE:
			s = UI_Cvar_VariableString(va("ui_lastServerRefresh_%i", ui_netSource.integer));
			break;
    default:
      break;
  }

	if (s) {
		return Text_Width(s, scale, 0);
	}
	return 0;
}

static void UI_DrawBotName(rectDef_t *rect, float scale, vec4_t color, int textStyle,int iMenuFont)
{
	int value = uiInfo.botIndex;
	const char *text = "";
	if (value >= UI_GetNumBots()) {
		value = 0;
	}
	text = UI_GetBotNameByNumber(value);
	Text_Paint(rect->x, rect->y, scale, color, text, 0, 0, textStyle,iMenuFont);
}

static void UI_DrawBotSkill(rectDef_t *rect, float scale, vec4_t color, int textStyle,int iMenuFont)
{
	if (uiInfo.skillIndex >= 0 && uiInfo.skillIndex < numSkillLevels)
	{
		Text_Paint(rect->x, rect->y, scale, color, (char *)UI_GetStringEdString("MP_INGAME", (char *)skillLevels[uiInfo.skillIndex]), 0, 0, textStyle,iMenuFont);
	}
}

static void UI_DrawRedBlue(rectDef_t *rect, float scale, vec4_t color, int textStyle,int iMenuFont)
{
	Text_Paint(rect->x, rect->y, scale, color, (uiInfo.redBlue == 0) ? UI_GetStringEdString("MP_INGAME","RED") : UI_GetStringEdString("MP_INGAME","BLUE"), 0, 0, textStyle,iMenuFont);
}

static void UI_DrawCrosshair(rectDef_t *rect, float scale, vec4_t color) {
	float size = 32.0f;

 	trap->R_SetColor( color );
	if (uiInfo.currentCrosshair < 0 || uiInfo.currentCrosshair >= NUM_CROSSHAIRS) {
		uiInfo.currentCrosshair = 0;
	}

	size = Q_min( rect->w, rect->h );
	UI_DrawHandlePic( rect->x, rect->y, size, size, uiInfo.uiDC.Assets.crosshairShader[uiInfo.currentCrosshair]);
 	trap->R_SetColor( NULL );
}

static void UI_DrawSelectedPlayer(rectDef_t *rect, float scale, vec4_t color, int textStyle, int iMenuFont) {
	if (uiInfo.uiDC.realTime > uiInfo.playerRefresh) {
		uiInfo.playerRefresh = uiInfo.uiDC.realTime + 3000;
		UI_BuildPlayerList();
	}
	Text_Paint(rect->x, rect->y, scale, color, UI_Cvar_VariableString("cg_selectedPlayerName"), 0, 0, textStyle, iMenuFont);
}

static void UI_DrawServerRefreshDate(rectDef_t *rect, float scale, vec4_t color, int textStyle, int iMenuFont)
{
	if (uiInfo.serverStatus.refreshActive)
	{
		vec4_t lowLight, newColor;
		lowLight[0] = 0.8 * color[0];
		lowLight[1] = 0.8 * color[1];
		lowLight[2] = 0.8 * color[2];
		lowLight[3] = 0.8 * color[3];
		LerpColor(color,lowLight,newColor,0.5+0.5*sin((float)(uiInfo.uiDC.realTime / PULSE_DIVISOR)));

		trap->SE_GetStringTextString("MP_INGAME_GETTINGINFOFORSERVERS", holdSPString, sizeof(holdSPString));
		Text_Paint(rect->x, rect->y, scale, newColor, va((char *) holdSPString, trap->LAN_GetServerCount(UI_SourceForLAN())), 0, 0, textStyle, iMenuFont);
	}
	else
	{
		char buff[64];
		Q_strncpyz(buff, UI_Cvar_VariableString(va("ui_lastServerRefresh_%i", ui_netSource.integer)), sizeof(buff));
		trap->SE_GetStringTextString("MP_INGAME_SERVER_REFRESHTIME", holdSPString, sizeof(holdSPString));

		Text_Paint(rect->x, rect->y, scale, color, va("%s: %s", holdSPString, buff), 0, 0, textStyle, iMenuFont);
	}
}

static void UI_DrawServerMOTD(rectDef_t *rect, float scale, vec4_t color, int iMenuFont) {
	if (uiInfo.serverStatus.motdLen) {
		float maxX;

		if (uiInfo.serverStatus.motdWidth == -1) {
			uiInfo.serverStatus.motdWidth = 0;
			uiInfo.serverStatus.motdPaintX = rect->x + 1;
			uiInfo.serverStatus.motdPaintX2 = -1;
		}

		if (uiInfo.serverStatus.motdOffset > uiInfo.serverStatus.motdLen) {
			uiInfo.serverStatus.motdOffset = 0;
			uiInfo.serverStatus.motdPaintX = rect->x + 1;
			uiInfo.serverStatus.motdPaintX2 = -1;
		}

		if (uiInfo.uiDC.realTime > uiInfo.serverStatus.motdTime) {
			uiInfo.serverStatus.motdTime = uiInfo.uiDC.realTime + 10;
			if (uiInfo.serverStatus.motdPaintX <= rect->x + 2) {
				if (uiInfo.serverStatus.motdOffset < uiInfo.serverStatus.motdLen) {
					uiInfo.serverStatus.motdPaintX += Text_Width(&uiInfo.serverStatus.motd[uiInfo.serverStatus.motdOffset], scale, 1) - 1;
					uiInfo.serverStatus.motdOffset++;
				} else {
					uiInfo.serverStatus.motdOffset = 0;
					if (uiInfo.serverStatus.motdPaintX2 >= 0) {
						uiInfo.serverStatus.motdPaintX = uiInfo.serverStatus.motdPaintX2;
					} else {
						uiInfo.serverStatus.motdPaintX = rect->x + rect->w - 2;
					}
					uiInfo.serverStatus.motdPaintX2 = -1;
				}
			} else {
				//serverStatus.motdPaintX--;
				uiInfo.serverStatus.motdPaintX -= 2;
				if (uiInfo.serverStatus.motdPaintX2 >= 0) {
					//serverStatus.motdPaintX2--;
					uiInfo.serverStatus.motdPaintX2 -= 2;
				}
			}
		}

		maxX = rect->x + rect->w - 2;
		Text_Paint_Limit(&maxX, uiInfo.serverStatus.motdPaintX, rect->y + rect->h - 3, scale, color, &uiInfo.serverStatus.motd[uiInfo.serverStatus.motdOffset], 0, 0, iMenuFont);
		if (uiInfo.serverStatus.motdPaintX2 >= 0) {
			float maxX2 = rect->x + rect->w - 2;
			Text_Paint_Limit(&maxX2, uiInfo.serverStatus.motdPaintX2, rect->y + rect->h - 3, scale, color, uiInfo.serverStatus.motd, 0, uiInfo.serverStatus.motdOffset, iMenuFont);
		}
		if (uiInfo.serverStatus.motdOffset && maxX > 0) {
			// if we have an offset ( we are skipping the first part of the string ) and we fit the string
			if (uiInfo.serverStatus.motdPaintX2 == -1) {
						uiInfo.serverStatus.motdPaintX2 = rect->x + rect->w - 2;
			}
		} else {
			uiInfo.serverStatus.motdPaintX2 = -1;
		}
	}
}

static void UI_DrawKeyBindStatus(rectDef_t *rect, float scale, vec4_t color, int textStyle,int iMenuFont) {
	if (Display_KeyBindPending()) {
		Text_Paint(rect->x, rect->y, scale, color, UI_GetStringEdString("MP_INGAME", "WAITING_FOR_NEW_KEY"), 0, 0, textStyle,iMenuFont);
	} else {
//		Text_Paint(rect->x, rect->y, scale, color, "Press ENTER or CLICK to change, Press BACKSPACE to clear", 0, 0, textStyle,iMenuFont);
	}
}

static void UI_DrawGLInfo(rectDef_t *rect, float scale, vec4_t color, int textStyle,int iMenuFont)
{
	char buff[4096] = {0};
	char *extensionName;
	int y, i=0;

	Text_Paint(rect->x + 2, rect->y, scale, color, va("GL_VENDOR: %s", uiInfo.uiDC.glconfig.vendor_string), 0, rect->w, textStyle,iMenuFont);
	Text_Paint(rect->x + 2, rect->y + 15, scale, color, va("GL_VERSION: %s: %s", uiInfo.uiDC.glconfig.version_string,uiInfo.uiDC.glconfig.renderer_string), 0, rect->w, textStyle,iMenuFont);
	Text_Paint(rect->x + 2, rect->y + 30, scale, color, va ("GL_PIXELFORMAT: color(%d-bits) Z(%d-bits) stencil(%d-bits)", uiInfo.uiDC.glconfig.colorBits, uiInfo.uiDC.glconfig.depthBits, uiInfo.uiDC.glconfig.stencilBits), 0, rect->w, textStyle,iMenuFont);

	// build null terminated extension strings
	Q_strncpyz(buff, uiInfo.uiDC.glconfig.extensions_string, sizeof(buff));
	y = rect->y + 45;

	extensionName = strtok (buff, " ");
	while ( y < rect->y + rect->h && extensionName != NULL )
	{
		if ( (i % 2) == 0 )
		{
			Text_Paint (rect->x + 2, y, scale, color, extensionName, 0, (rect->w / 2), textStyle, iMenuFont);
		}
		else
		{
			Text_Paint (rect->x + rect->w / 2, y, scale, color, extensionName, 0, (rect->w / 2), textStyle, iMenuFont);
			y += 11;
		}

		extensionName = strtok (NULL, " ");
		i++;
	}
}

/*
=================
UI_Version
=================
*/
static void UI_Version(rectDef_t *rect, float scale, vec4_t color, int iMenuFont)
{
	int width;

	width = uiInfo.uiDC.textWidth(JK_VERSION, scale, iMenuFont);

	uiInfo.uiDC.drawText(rect->x - width, rect->y, scale, color, JK_VERSION, 0, 0, 0, iMenuFont);
}

/*
=================
UI_OwnerDraw
=================
*/
// FIXME: table drive
//
static void UI_OwnerDraw(float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float special, float scale, vec4_t color, qhandle_t shader, int textStyle,int iMenuFont)
{
	rectDef_t rect;
	int findex;
	int drawRank = 0, iUse = 0;

	rect.x = x + text_x;
	rect.y = y + text_y;
	rect.w = w;
	rect.h = h;

  switch (ownerDraw)
  {
    case UI_HANDICAP:
      UI_DrawHandicap(&rect, scale, color, textStyle, iMenuFont);
      break;
    case UI_SKIN_COLOR:
      UI_DrawSkinColor(&rect, scale, color, textStyle, uiSkinColor, TEAM_FREE, TEAM_BLUE, iMenuFont);
      break;
	case UI_FORCE_SIDE:
      UI_DrawForceSide(&rect, scale, color, textStyle, uiForceSide, 1, 2, iMenuFont);
      break;
	case UI_JEDI_NONJEDI:
      UI_DrawJediNonJedi(&rect, scale, color, textStyle, uiJediNonJedi, 0, 1, iMenuFont);
      break;
    case UI_FORCE_POINTS:
      UI_DrawGenericNum(&rect, scale, color, textStyle, uiForceAvailable, 1, forceMasteryPoints[MAX_FORCE_RANK], ownerDraw,iMenuFont);
      break;
	case UI_FORCE_MASTERY_SET:
      UI_DrawForceMastery(&rect, scale, color, textStyle, uiForceRank, 0, MAX_FORCE_RANK, iMenuFont);
      break;
    case UI_FORCE_RANK:
      UI_DrawForceMastery(&rect, scale, color, textStyle, uiForceRank, 0, MAX_FORCE_RANK, iMenuFont);
      break;
	case UI_FORCE_RANK_HEAL:
	case UI_FORCE_RANK_LEVITATION:
	case UI_FORCE_RANK_SPEED:
	case UI_FORCE_RANK_PUSH:
	case UI_FORCE_RANK_PULL:
	case UI_FORCE_RANK_TELEPATHY:
	case UI_FORCE_RANK_GRIP:
	case UI_FORCE_RANK_LIGHTNING:
	case UI_FORCE_RANK_RAGE:
	case UI_FORCE_RANK_PROTECT:
	case UI_FORCE_RANK_ABSORB:
	case UI_FORCE_RANK_TEAM_HEAL:
	case UI_FORCE_RANK_TEAM_FORCE:
	case UI_FORCE_RANK_DRAIN:
	case UI_FORCE_RANK_SEE:
	case UI_FORCE_RANK_SABERATTACK:
	case UI_FORCE_RANK_SABERDEFEND:
	case UI_FORCE_RANK_SABERTHROW:

//		uiForceRank
/*
		uiForceUsed
		// Only fields for white stars
		if (uiForceUsed<3)
		{
		    Menu_ShowItemByName(menu, "lightpowers_team", qtrue);
		}
		else if (uiForceUsed<6)
		{
		    Menu_ShowItemByName(menu, "lightpowers_team", qtrue);
		}
*/

		findex = (ownerDraw - UI_FORCE_RANK)-1;
		//this will give us the index as long as UI_FORCE_RANK is always one below the first force rank index
		if (uiForcePowerDarkLight[findex] && uiForceSide != uiForcePowerDarkLight[findex])
		{
			color[0] *= 0.5;
			color[1] *= 0.5;
			color[2] *= 0.5;
		}
/*		else if (uiForceRank < UI_ForceColorMinRank[bgForcePowerCost[findex][FORCE_LEVEL_1]])
		{
			color[0] *= 0.5;
			color[1] *= 0.5;
			color[2] *= 0.5;
		}
*/		drawRank = uiForcePowersRank[findex];

		UI_DrawForceStars(&rect, scale, color, textStyle, findex, drawRank, 0, NUM_FORCE_POWER_LEVELS-1);
		break;
    case UI_EFFECTS:
      break;
    case UI_PLAYERMODEL:
      //UI_DrawPlayerModel(&rect);
      break;
    case UI_CLANNAME:
      UI_DrawClanName(&rect, scale, color, textStyle, iMenuFont);
      break;
    case UI_CLANLOGO:
      UI_DrawClanLogo(&rect, scale, color);
      break;
    case UI_CLANCINEMATIC:
      UI_DrawClanCinematic(&rect, scale, color);
      break;
    case UI_PREVIEWCINEMATIC:
      UI_DrawPreviewCinematic(&rect, scale, color);
      break;
    case UI_GAMETYPE:
      UI_DrawGameType(&rect, scale, color, textStyle, iMenuFont);
      break;
    case UI_NETGAMETYPE:
      UI_DrawNetGameType(&rect, scale, color, textStyle, iMenuFont);
      break;
    case UI_AUTOSWITCHLIST:
      UI_DrawAutoSwitch(&rect, scale, color, textStyle, iMenuFont);
      break;
    case UI_JOINGAMETYPE:
	  UI_DrawJoinGameType(&rect, scale, color, textStyle, iMenuFont);
	  break;
    case UI_MAPPREVIEW:
      UI_DrawMapPreview(&rect, scale, color, qtrue);
      break;
    case UI_MAP_TIMETOBEAT:
      break;
    case UI_MAPCINEMATIC:
      UI_DrawMapCinematic(&rect, scale, color, qfalse);
      break;
    case UI_STARTMAPCINEMATIC:
      UI_DrawMapCinematic(&rect, scale, color, qtrue);
      break;
    case UI_SKILL:
      UI_DrawSkill(&rect, scale, color, textStyle, iMenuFont);
      break;
    case UI_TOTALFORCESTARS:
//      UI_DrawTotalForceStars(&rect, scale, color, textStyle);
      break;
    case UI_BLUETEAMNAME:
      UI_DrawTeamName(&rect, scale, color, qtrue, textStyle, iMenuFont);
      break;
    case UI_REDTEAMNAME:
      UI_DrawTeamName(&rect, scale, color, qfalse, textStyle, iMenuFont);
      break;
    case UI_BLUETEAM1:
		case UI_BLUETEAM2:
		case UI_BLUETEAM3:
		case UI_BLUETEAM4:
		case UI_BLUETEAM5:
		case UI_BLUETEAM6:
		case UI_BLUETEAM7:
		case UI_BLUETEAM8:
	if (ownerDraw <= UI_BLUETEAM5)
	{
	  iUse = ownerDraw-UI_BLUETEAM1 + 1;
	}
	else
	{
	  iUse = ownerDraw-274; //unpleasent hack because I don't want to move up all the UI_BLAHTEAM# defines
	}
      UI_DrawTeamMember(&rect, scale, color, qtrue, iUse, textStyle, iMenuFont);
      break;
    case UI_REDTEAM1:
		case UI_REDTEAM2:
		case UI_REDTEAM3:
		case UI_REDTEAM4:
		case UI_REDTEAM5:
		case UI_REDTEAM6:
		case UI_REDTEAM7:
		case UI_REDTEAM8:
	if (ownerDraw <= UI_REDTEAM5)
	{
	  iUse = ownerDraw-UI_REDTEAM1 + 1;
	}
	else
	{
	  iUse = ownerDraw-277; //unpleasent hack because I don't want to move up all the UI_BLAHTEAM# defines
	}
      UI_DrawTeamMember(&rect, scale, color, qfalse, iUse, textStyle, iMenuFont);
      break;
		case UI_NETSOURCE:
      UI_DrawNetSource(&rect, scale, color, textStyle, iMenuFont);
			break;
    case UI_NETMAPPREVIEW:
      UI_DrawNetMapPreview(&rect, scale, color);
      break;
    case UI_NETMAPCINEMATIC:
      UI_DrawNetMapCinematic(&rect, scale, color);
      break;
		case UI_NETFILTER:
      UI_DrawNetFilter(&rect, scale, color, textStyle, iMenuFont);
			break;
		case UI_TIER:
			UI_DrawTier(&rect, scale, color, textStyle, iMenuFont);
			break;
		case UI_OPPONENTMODEL:
			//UI_DrawOpponent(&rect);
			break;
		case UI_TIERMAP1:
			UI_DrawTierMap(&rect, 0);
			break;
		case UI_TIERMAP2:
			UI_DrawTierMap(&rect, 1);
			break;
		case UI_TIERMAP3:
			UI_DrawTierMap(&rect, 2);
			break;
		case UI_PLAYERLOGO:
			UI_DrawPlayerLogo(&rect, color);
			break;
		case UI_PLAYERLOGO_METAL:
			UI_DrawPlayerLogoMetal(&rect, color);
			break;
		case UI_PLAYERLOGO_NAME:
			UI_DrawPlayerLogoName(&rect, color);
			break;
		case UI_OPPONENTLOGO:
			UI_DrawOpponentLogo(&rect, color);
			break;
		case UI_OPPONENTLOGO_METAL:
			UI_DrawOpponentLogoMetal(&rect, color);
			break;
		case UI_OPPONENTLOGO_NAME:
			UI_DrawOpponentLogoName(&rect, color);
			break;
		case UI_TIER_MAPNAME:
			UI_DrawTierMapName(&rect, scale, color, textStyle, iMenuFont);
			break;
		case UI_TIER_GAMETYPE:
			UI_DrawTierGameType(&rect, scale, color, textStyle, iMenuFont);
			break;
		case UI_ALLMAPS_SELECTION:
			UI_DrawAllMapsSelection(&rect, scale, color, textStyle, qtrue, iMenuFont);
			break;
		case UI_MAPS_SELECTION:
			UI_DrawAllMapsSelection(&rect, scale, color, textStyle, qfalse, iMenuFont);
			break;
		case UI_OPPONENT_NAME:
			UI_DrawOpponentName(&rect, scale, color, textStyle, iMenuFont);
			break;
		case UI_BOTNAME:
			UI_DrawBotName(&rect, scale, color, textStyle,iMenuFont);
			break;
		case UI_BOTSKILL:
			UI_DrawBotSkill(&rect, scale, color, textStyle,iMenuFont);
			break;
		case UI_REDBLUE:
			UI_DrawRedBlue(&rect, scale, color, textStyle,iMenuFont);
			break;
		case UI_CROSSHAIR:
			UI_DrawCrosshair(&rect, scale, color);
			break;
		case UI_SELECTEDPLAYER:
			UI_DrawSelectedPlayer(&rect, scale, color, textStyle, iMenuFont);
			break;
		case UI_SERVERREFRESHDATE:
			UI_DrawServerRefreshDate(&rect, scale, color, textStyle, iMenuFont);
			break;
		case UI_SERVERMOTD:
			UI_DrawServerMOTD(&rect, scale, color, iMenuFont);
			break;
		case UI_GLINFO:
			UI_DrawGLInfo(&rect,scale, color, textStyle, iMenuFont);
			break;
		case UI_KEYBINDSTATUS:
			UI_DrawKeyBindStatus(&rect,scale, color, textStyle,iMenuFont);
			break;
		case UI_VERSION:
			UI_Version(&rect, scale, color, iMenuFont);
			break;
    default:
      break;
  }
}

static qboolean UI_OwnerDrawVisible(int flags) {
	qboolean vis = qtrue;

	while (flags) {
		if (flags & UI_SHOW_FFA) {
			if (trap->Cvar_VariableValue("g_gametype") != GT_FFA &&
				trap->Cvar_VariableValue("g_gametype") != GT_HOLOCRON &&
				trap->Cvar_VariableValue("g_gametype") != GT_JEDIMASTER) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_FFA;
		}
		if (flags & UI_SHOW_NOTFFA) {
			if (trap->Cvar_VariableValue("g_gametype") == GT_FFA ||
				trap->Cvar_VariableValue("g_gametype") == GT_HOLOCRON ||
				trap->Cvar_VariableValue("g_gametype") != GT_JEDIMASTER) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_NOTFFA;
		}
		if (flags & UI_SHOW_LEADER) {
			// these need to show when this client can give orders to a player or a group
			if (!uiInfo.teamLeader) {
				vis = qfalse;
			} else {
				// if showing yourself
				if (cg_selectedPlayer.integer < uiInfo.myTeamCount && uiInfo.teamClientNums[cg_selectedPlayer.integer] == uiInfo.playerNumber) {
					vis = qfalse;
				}
			}
			flags &= ~UI_SHOW_LEADER;
		}
		if (flags & UI_SHOW_NOTLEADER) {
			// these need to show when this client is assigning their own status or they are NOT the leader
			if (uiInfo.teamLeader) {
				// if not showing yourself
				if (!(cg_selectedPlayer.integer < uiInfo.myTeamCount && uiInfo.teamClientNums[cg_selectedPlayer.integer] == uiInfo.playerNumber)) {
					vis = qfalse;
				}
				// these need to show when this client can give orders to a player or a group
			}
			flags &= ~UI_SHOW_NOTLEADER;
		}
		if (flags & UI_SHOW_FAVORITESERVERS) {
			// this assumes you only put this type of display flag on something showing in the proper context
			if (ui_netSource.integer != UIAS_FAVORITES) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_FAVORITESERVERS;
		}
		if (flags & UI_SHOW_NOTFAVORITESERVERS) {
			// this assumes you only put this type of display flag on something showing in the proper context
			if (ui_netSource.integer == UIAS_FAVORITES) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_NOTFAVORITESERVERS;
		}
		if (flags & UI_SHOW_ANYTEAMGAME) {
			if (uiInfo.gameTypes[ui_gametype.integer].gtEnum <= GT_TEAM ) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_ANYTEAMGAME;
		}
		if (flags & UI_SHOW_ANYNONTEAMGAME) {
			if (uiInfo.gameTypes[ui_gametype.integer].gtEnum > GT_TEAM ) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_ANYNONTEAMGAME;
		}
		if (flags & UI_SHOW_NETANYTEAMGAME) {
			if (uiInfo.gameTypes[ui_netGametype.integer].gtEnum <= GT_TEAM ) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_NETANYTEAMGAME;
		}
		if (flags & UI_SHOW_NETANYNONTEAMGAME) {
			if (uiInfo.gameTypes[ui_netGametype.integer].gtEnum > GT_TEAM ) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_NETANYNONTEAMGAME;
		} else {
			flags = 0;
		}
	}
	return vis;
}

static qboolean UI_Handicap_HandleKey(int flags, float *special, int key) {
	if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER) {
		int h;
		h = Com_Clamp( 5, 100, trap->Cvar_VariableValue("handicap") );
		if (key == A_MOUSE2) {
			h -= 5;
		} else {
			h += 5;
		}
		if (h > 100) {
			h = 5;
		} else if (h < 5) {
			h = 100;
		}
		trap->Cvar_Set( "handicap", va( "%i", h) );
		return qtrue;
	}
	return qfalse;
}

extern void	Item_RunScript(itemDef_t *item, const char *s);		//from ui_shared;

// For hot keys on the chat main menu.
static qboolean UI_Chat_Main_HandleKey(int key)
{
	menuDef_t *menu;
	itemDef_t *item;

	menu = Menu_GetFocused();

	if (!menu)
	{
		return (qfalse);
	}

	if ((key == A_1) || ( key == A_PLING))
	{
		item = Menu_FindItemByName(menu, "attack");
	}
	else if ((key == A_2) || ( key == A_AT))
	{
		item = Menu_FindItemByName(menu, "defend");
	}
	else if ((key == A_3) || ( key == A_HASH))
	{
		item = Menu_FindItemByName(menu, "request");
	}
	else if ((key == A_4) || ( key == A_STRING))
	{
		item = Menu_FindItemByName(menu, "reply");
	}
	else if ((key == A_5) || ( key == A_PERCENT))
	{
		item = Menu_FindItemByName(menu, "spot");
	}
	else if ((key == A_6) || ( key == A_CARET))
	{
		item = Menu_FindItemByName(menu, "tactics");
	}
	else
	{
		return (qfalse);
	}

	if (item)
	{
	    Item_RunScript(item, item->action);
	}

	return (qtrue);
}

// For hot keys on the chat main menu.
static qboolean UI_Chat_Attack_HandleKey(int key)
{
	menuDef_t *menu;
	itemDef_t *item;

	menu = Menu_GetFocused();

	if (!menu)
	{
		return (qfalse);
	}

	if ((key == A_1) || ( key == A_PLING))
	{
		item = Menu_FindItemByName(menu, "att_01");
	}
	else if ((key == A_2) || ( key == A_AT))
	{
		item = Menu_FindItemByName(menu, "att_02");
	}
	else if ((key == A_3) || ( key == A_HASH))
	{
		item = Menu_FindItemByName(menu, "att_03");
	}
	else
	{
		return (qfalse);
	}

	if (item)
	{
	    Item_RunScript(item, item->action);
	}

	return (qtrue);
}

// For hot keys on the chat main menu.
static qboolean UI_Chat_Defend_HandleKey(int key)
{
	menuDef_t *menu;
	itemDef_t *item;

	menu = Menu_GetFocused();

	if (!menu)
	{
		return (qfalse);
	}

	if ((key == A_1) || ( key == A_PLING))
	{
		item = Menu_FindItemByName(menu, "def_01");
	}
	else if ((key == A_2) || ( key == A_AT))
	{
		item = Menu_FindItemByName(menu, "def_02");
	}
	else if ((key == A_3) || ( key == A_HASH))
	{
		item = Menu_FindItemByName(menu, "def_03");
	}
	else if ((key == A_4) || ( key == A_STRING))
	{
		item = Menu_FindItemByName(menu, "def_04");
	}
	else
	{
		return (qfalse);
	}

	if (item)
	{
	    Item_RunScript(item, item->action);
	}

	return (qtrue);
}

// For hot keys on the chat main menu.
static qboolean UI_Chat_Request_HandleKey(int key)
{
	menuDef_t *menu;
	itemDef_t *item;

	menu = Menu_GetFocused();

	if (!menu)
	{
		return (qfalse);
	}

	if ((key == A_1) || ( key == A_PLING))
	{
		item = Menu_FindItemByName(menu, "req_01");
	}
	else if ((key == A_2) || ( key == A_AT))
	{
		item = Menu_FindItemByName(menu, "req_02");
	}
	else if ((key == A_3) || ( key == A_HASH))
	{
		item = Menu_FindItemByName(menu, "req_03");
	}
	else if ((key == A_4) || ( key == A_STRING))
	{
		item = Menu_FindItemByName(menu, "req_04");
	}
	else if ((key == A_5) || ( key == A_PERCENT))
	{
		item = Menu_FindItemByName(menu, "req_05");
	}
	else if ((key == A_6) || ( key == A_CARET))
	{
		item = Menu_FindItemByName(menu, "req_06");
	}
	else
	{
		return (qfalse);
	}

	if (item)
	{
	    Item_RunScript(item, item->action);
	}

	return (qtrue);
}

// For hot keys on the chat main menu.
static qboolean UI_Chat_Reply_HandleKey(int key)
{
	menuDef_t *menu;
	itemDef_t *item;

	menu = Menu_GetFocused();

	if (!menu)
	{
		return (qfalse);
	}

	if ((key == A_1) || ( key == A_PLING))
	{
		item = Menu_FindItemByName(menu, "rep_01");
	}
	else if ((key == A_2) || ( key == A_AT))
	{
		item = Menu_FindItemByName(menu, "rep_02");
	}
	else if ((key == A_3) || ( key == A_HASH))
	{
		item = Menu_FindItemByName(menu, "rep_03");
	}
	else if ((key == A_4) || ( key == A_STRING))
	{
		item = Menu_FindItemByName(menu, "rep_04");
	}
	else if ((key == A_5) || ( key == A_PERCENT))
	{
		item = Menu_FindItemByName(menu, "rep_05");
	}
	else
	{
		return (qfalse);
	}

	if (item)
	{
	    Item_RunScript(item, item->action);
	}

	return (qtrue);
}

// For hot keys on the chat main menu.
static qboolean UI_Chat_Spot_HandleKey(int key)
{
	menuDef_t *menu;
	itemDef_t *item;

	menu = Menu_GetFocused();

	if (!menu)
	{
		return (qfalse);
	}

	if ((key == A_1) || ( key == A_PLING))
	{
		item = Menu_FindItemByName(menu, "spot_01");
	}
	else if ((key == A_2) || ( key == A_AT))
	{
		item = Menu_FindItemByName(menu, "spot_02");
	}
	else if ((key == A_3) || ( key == A_HASH))
	{
		item = Menu_FindItemByName(menu, "spot_03");
	}
	else if ((key == A_4) || ( key == A_STRING))
	{
		item = Menu_FindItemByName(menu, "spot_04");
	}
	else if ((key == A_5) || (key == A_PERCENT))
	{
		item = Menu_FindItemByName(menu, "spot_05");
	}
	else
	{
		return (qfalse);
	}

	if (item)
	{
	    Item_RunScript(item, item->action);
	}

	return (qtrue);
}

// For hot keys on the chat main menu.
static qboolean UI_Chat_Tactical_HandleKey(int key)
{
	menuDef_t *menu;
	itemDef_t *item;

	menu = Menu_GetFocused();

	if (!menu)
	{
		return (qfalse);
	}

	if ((key == A_1) || ( key == A_PLING))
	{
		item = Menu_FindItemByName(menu, "tac_01");
	}
	else if ((key == A_2) || ( key == A_AT))
	{
		item = Menu_FindItemByName(menu, "tac_02");
	}
	else if ((key == A_3) || ( key == A_HASH))
	{
		item = Menu_FindItemByName(menu, "tac_03");
	}
	else if ((key == A_4) || ( key == A_STRING))
	{
		item = Menu_FindItemByName(menu, "tac_04");
	}
	else if ((key == A_5) || ( key == A_PERCENT))
	{
		item = Menu_FindItemByName(menu, "tac_05");
	}
	else if ((key == A_6) || ( key == A_CARET))
	{
		item = Menu_FindItemByName(menu, "tac_06");
	}
	else
	{
		return (qfalse);
	}

	if (item)
	{
	    Item_RunScript(item, item->action);
	}

	return (qtrue);
}

static qboolean UI_GameType_HandleKey(int flags, float *special, int key, qboolean resetMap) {
	if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER) {
		int oldCount = UI_MapCountByGameType(qtrue);
		int value = ui_gametype.integer;

		// hard coded mess here
		if (key == A_MOUSE2) {
			value--;
			if (value == 2) {
				value = 1;
			} else if (value < 2) {
				value = uiInfo.numGameTypes - 1;
			}
		} else {
			value++;
			if (value >= uiInfo.numGameTypes) {
				value = 1;
			} else if (value == 2) {
				value = 3;
			}
		}

		trap->Cvar_Set("ui_gametype", va("%d", value));
		trap->Cvar_Update(&ui_gametype);
		UI_SetCapFragLimits(qtrue);
		if (resetMap && oldCount != UI_MapCountByGameType(qtrue)) {
			trap->Cvar_Set( "ui_currentMap", "0");
			trap->Cvar_Update(&ui_currentMap);
			Menu_SetFeederSelection(NULL, FEEDER_MAPS, 0, NULL);
		}
		return qtrue;
	}
	return qfalse;
}

// If we're in the solo menu, don't let them see siege maps.
static qboolean UI_InSoloMenu( void )
{
	menuDef_t *menu;
	itemDef_t *item;
	char *name = "solo_gametypefield";

	menu = Menu_GetFocused();	// Get current menu (either video or ingame video, I would assume)

	if (!menu)
	{
		return (qfalse);
	}

	item = Menu_FindItemByName(menu, name);
	if (item)
	{
		return qtrue;
	}

	return (qfalse);
}

static qboolean UI_NetGameType_HandleKey(int flags, float *special, int key)
{
	if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER)
	{
		int value = ui_netGametype.integer;

		if (key == A_MOUSE2)
		{
			value--;
			if (UI_InSoloMenu())
			{
				if (uiInfo.gameTypes[value].gtEnum == GT_SIEGE)
				{
					value--;
				}
			}
		}
		else
		{
			value++;
			if (UI_InSoloMenu())
			{
				if (uiInfo.gameTypes[value].gtEnum == GT_SIEGE)
				{
					value++;
				}
			}
		}

		if (value < 0)
		{
			value = uiInfo.numGameTypes - 1;
		}
		else if (value >= uiInfo.numGameTypes)
		{
			value = 0;
		}

		trap->Cvar_Set( "ui_netGametype", va("%d", value));
		trap->Cvar_Update(&ui_netGametype);
		trap->Cvar_Set( "ui_actualNetGametype", va("%d", uiInfo.gameTypes[ui_netGametype.integer].gtEnum));
		trap->Cvar_Update(&ui_actualNetGametype);
		trap->Cvar_Set( "ui_currentNetMap", "0");
		trap->Cvar_Update(&ui_currentNetMap);
		UI_MapCountByGameType(qfalse);
		Menu_SetFeederSelection(NULL, FEEDER_ALLMAPS, 0, NULL);
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_AutoSwitch_HandleKey(int flags, float *special, int key) {
	if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER) {
		int switchVal = trap->Cvar_VariableValue("cg_autoswitch");

		if (key == A_MOUSE2) {
			switchVal--;
		} else {
			switchVal++;
		}

		if (switchVal < 0)
		{
			switchVal = 2;
		}
		else if (switchVal >= 3)
		{
			switchVal = 0;
		}

		trap->Cvar_Set( "cg_autoswitch", va("%i", switchVal));
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_JoinGameType_HandleKey(int flags, float *special, int key) {
	if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER) {
		int value = ui_joinGametype.integer;

		if (key == A_MOUSE2) {
			value--;
		} else {
			value++;
		}

		if (value < 0) {
			value = uiInfo.numJoinGameTypes - 1;
		} else if (value >= uiInfo.numJoinGameTypes) {
			value = 0;
		}

		trap->Cvar_Set( "ui_joinGametype", va("%d", value));
		trap->Cvar_Update(&ui_joinGametype);
		UI_BuildServerDisplayList(qtrue);
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_Skill_HandleKey(int flags, float *special, int key) {
	if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER) {
		int i = trap->Cvar_VariableValue( "g_spSkill" );

		if (key == A_MOUSE2) {
			i--;
		} else {
			i++;
		}

		if (i < 1) {
			i = numSkillLevels;
		} else if (i > numSkillLevels) {
			i = 1;
		}

		trap->Cvar_Set("g_spSkill", va("%i", i));
		trap->Cvar_Update(&g_spSkill);
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_TeamName_HandleKey(int flags, float *special, int key, qboolean blue) {
	if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER) {
		int i;
		i = UI_TeamIndexFromName(UI_Cvar_VariableString((blue) ? "ui_blueTeam" : "ui_redTeam"));

		if (key == A_MOUSE2) {
			i--;
		} else {
			i++;
		}

		if (i >= uiInfo.teamCount) {
			i = 0;
		} else if (i < 0) {
			i = uiInfo.teamCount - 1;
		}

		trap->Cvar_Set( (blue) ? "ui_blueTeam" : "ui_redTeam", uiInfo.teamList[i].teamName);

		return qtrue;
	}
	return qfalse;
}

static qboolean UI_TeamMember_HandleKey(int flags, float *special, int key, qboolean blue, int num) {
	if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER) {
		// 0 - None
		// 1 - Human
		// 2..NumCharacters - Bot
		char *cvar = va(blue ? "ui_blueteam%i" : "ui_redteam%i", num);
		int value = trap->Cvar_VariableValue(cvar);
		int maxcl = trap->Cvar_VariableValue( "sv_maxClients" );
		int numval = num;

		numval *= 2;

		if (blue)
		{
			numval -= 1;
		}

		if (numval > maxcl)
		{
			return qfalse;
		}

		if (value < 1)
		{
			value = 1;
		}

		if (key == A_MOUSE2) {
			value--;
		} else {
			value++;
		}

		/*if (ui_actualNetGameType.integer >= GT_TEAM) {
		if (value >= uiInfo.characterCount + 2) {
		value = 0;
		} else if (value < 0) {
		value = uiInfo.characterCount + 2 - 1;
		}
		} else {*/
		if (value >= UI_GetNumBots() + 2) {
			value = 1;
		} else if (value < 1) {
			value = UI_GetNumBots() + 2 - 1;
		}
		//}

		trap->Cvar_Set(cvar, va("%i", value));
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_NetSource_HandleKey(int flags, float *special, int key) {
	if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER) {
		int value = ui_netSource.integer;

		if (key == A_MOUSE2) {
			value--;
		} else {
			value++;
		}

		if(value >= UIAS_GLOBAL1 && value <= UIAS_GLOBAL5)
		{
			char masterstr[2], cvarname[sizeof("sv_master1")];

			while(value >= UIAS_GLOBAL1 && value <= UIAS_GLOBAL5)
			{
				Com_sprintf(cvarname, sizeof(cvarname), "sv_master%d", value);
				trap->Cvar_VariableStringBuffer(cvarname, masterstr, sizeof(masterstr));
				if(*masterstr)
					break;

				if (key == A_MOUSE2) {
					value--;
				} else {
					value++;
				}
			}
		}

		if (value >= numNetSources) {
			value = 0;
		} else if (value < 0) {
			value = numNetSources - 1;
		}

		trap->Cvar_Set( "ui_netSource", va("%d", value));
		trap->Cvar_Update(&ui_netSource);

		UI_BuildServerDisplayList(qtrue);
		if (!(ui_netSource.integer >= UIAS_GLOBAL1 && ui_netSource.integer <= UIAS_GLOBAL5)) {
			UI_StartServerRefresh(qtrue);
		}
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_NetFilter_HandleKey(int flags, float *special, int key) {
	if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER) {
		int value = ui_serverFilterType.integer;

		if (key == A_MOUSE2) {
			value--;
		} else {
			value++;
		}

		if (value > uiInfo.modCount) {
			value = 0;
		} else if (value < 0) {
			value = uiInfo.modCount;
		}

		trap->Cvar_Set( "ui_serverFilterType", va("%d", value));
		trap->Cvar_Update(&ui_serverFilterType);

		UI_BuildServerDisplayList(qtrue);
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_OpponentName_HandleKey(int flags, float *special, int key) {
	if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER) {
		if (key == A_MOUSE2) {
			UI_PriorOpponent();
		} else {
			UI_NextOpponent();
		}
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_BotName_HandleKey(int flags, float *special, int key) {
	if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER) {
//		int game = trap->Cvar_VariableValue("g_gametype");
		int value = uiInfo.botIndex;

		if (key == A_MOUSE2) {
			value--;
		} else {
			value++;
		}

		/*
		if (game >= GT_TEAM) {
		if (value >= uiInfo.characterCount + 2) {
		value = 0;
		} else if (value < 0) {
		value = uiInfo.characterCount + 2 - 1;
		}
		} else {
		*/
		if (value >= UI_GetNumBots()/* + 2*/) {
			value = 0;
		} else if (value < 0) {
			value = UI_GetNumBots()/* + 2*/ - 1;
		}
		//}
		uiInfo.botIndex = value;
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_BotSkill_HandleKey(int flags, float *special, int key) {
	if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER) {
		if (key == A_MOUSE2) {
			uiInfo.skillIndex--;
		} else {
			uiInfo.skillIndex++;
		}
		if (uiInfo.skillIndex >= numSkillLevels) {
			uiInfo.skillIndex = 0;
		} else if (uiInfo.skillIndex < 0) {
			uiInfo.skillIndex = numSkillLevels-1;
		}
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_RedBlue_HandleKey(int flags, float *special, int key) {
	if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER) {
		uiInfo.redBlue ^= 1;
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_Crosshair_HandleKey(int flags, float *special, int key) {
	if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER) {
		if (key == A_MOUSE2) {
			uiInfo.currentCrosshair--;
		} else {
			uiInfo.currentCrosshair++;
		}

		if (uiInfo.currentCrosshair >= NUM_CROSSHAIRS) {
			uiInfo.currentCrosshair = 0;
		} else if (uiInfo.currentCrosshair < 0) {
			uiInfo.currentCrosshair = NUM_CROSSHAIRS - 1;
		}
		trap->Cvar_Set("cg_drawCrosshair", va("%d", uiInfo.currentCrosshair));
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_SelectedPlayer_HandleKey(int flags, float *special, int key) {
	if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER) {
		int selected;

		UI_BuildPlayerList();
		if (!uiInfo.teamLeader) {
			return qfalse;
		}
		selected = trap->Cvar_VariableValue("cg_selectedPlayer");

		if (key == A_MOUSE2) {
			selected--;
		} else {
			selected++;
		}

		if (selected > uiInfo.myTeamCount) {
			selected = 0;
		} else if (selected < 0) {
			selected = uiInfo.myTeamCount;
		}

		if (selected == uiInfo.myTeamCount) {
		 	trap->Cvar_Set( "cg_selectedPlayerName", "Everyone");
		} else {
		 	trap->Cvar_Set( "cg_selectedPlayerName", uiInfo.teamNames[selected]);
		}
	 	trap->Cvar_Set( "cg_selectedPlayer", va("%d", selected));
	}
	return qfalse;
}

/*
static qboolean UI_VoiceChat_HandleKey(int flags, float *special, int key)
{

	qboolean ret = qfalse;

	switch(key)
	{
		case A_1:
		case A_KP_1:
			ret = qtrue;
			break;
		case A_2:
		case A_KP_2:
			ret = qtrue;
			break;

	}

	return ret;
}
*/

static qboolean UI_OwnerDrawHandleKey(int ownerDraw, int flags, float *special, int key) {
	int findex, iUse = 0;

  switch (ownerDraw) {
    case UI_HANDICAP:
      return UI_Handicap_HandleKey(flags, special, key);
      break;
    case UI_SKIN_COLOR:
      return UI_SkinColor_HandleKey(flags, special, key, uiSkinColor, TEAM_FREE, TEAM_BLUE, ownerDraw);
      break;
    case UI_FORCE_SIDE:
      return UI_ForceSide_HandleKey(flags, special, key, uiForceSide, 1, 2, ownerDraw);
      break;
    case UI_JEDI_NONJEDI:
      return UI_JediNonJedi_HandleKey(flags, special, key, uiJediNonJedi, 0, 1, ownerDraw);
      break;
	case UI_FORCE_MASTERY_SET:
      return UI_ForceMaxRank_HandleKey(flags, special, key, uiForceRank, 1, MAX_FORCE_RANK, ownerDraw);
      break;
    case UI_FORCE_RANK:
		break;
	case UI_CHAT_MAIN:
		return UI_Chat_Main_HandleKey(key);
		break;
	case UI_CHAT_ATTACK:
		return UI_Chat_Attack_HandleKey(key);
		break;
	case UI_CHAT_DEFEND:
		return UI_Chat_Defend_HandleKey(key);
		break;
	case UI_CHAT_REQUEST:
		return UI_Chat_Request_HandleKey(key);
		break;
	case UI_CHAT_REPLY:
		return UI_Chat_Reply_HandleKey(key);
		break;
	case UI_CHAT_SPOT:
		return UI_Chat_Spot_HandleKey(key);
		break;
	case UI_CHAT_TACTICAL:
		return UI_Chat_Tactical_HandleKey(key);
		break;
	case UI_FORCE_RANK_HEAL:
	case UI_FORCE_RANK_LEVITATION:
	case UI_FORCE_RANK_SPEED:
	case UI_FORCE_RANK_PUSH:
	case UI_FORCE_RANK_PULL:
	case UI_FORCE_RANK_TELEPATHY:
	case UI_FORCE_RANK_GRIP:
	case UI_FORCE_RANK_LIGHTNING:
	case UI_FORCE_RANK_RAGE:
	case UI_FORCE_RANK_PROTECT:
	case UI_FORCE_RANK_ABSORB:
	case UI_FORCE_RANK_TEAM_HEAL:
	case UI_FORCE_RANK_TEAM_FORCE:
	case UI_FORCE_RANK_DRAIN:
	case UI_FORCE_RANK_SEE:
	case UI_FORCE_RANK_SABERATTACK:
	case UI_FORCE_RANK_SABERDEFEND:
	case UI_FORCE_RANK_SABERTHROW:
		findex = (ownerDraw - UI_FORCE_RANK)-1;
		//this will give us the index as long as UI_FORCE_RANK is always one below the first force rank index
		return UI_ForcePowerRank_HandleKey(flags, special, key, uiForcePowersRank[findex], 0, NUM_FORCE_POWER_LEVELS-1, ownerDraw);
		break;
    case UI_EFFECTS:
      break;
    case UI_GAMETYPE:
      return UI_GameType_HandleKey(flags, special, key, qtrue);
      break;
    case UI_NETGAMETYPE:
      return UI_NetGameType_HandleKey(flags, special, key);
      break;
    case UI_AUTOSWITCHLIST:
      return UI_AutoSwitch_HandleKey(flags, special, key);
      break;
    case UI_JOINGAMETYPE:
      return UI_JoinGameType_HandleKey(flags, special, key);
      break;
    case UI_SKILL:
      return UI_Skill_HandleKey(flags, special, key);
      break;
    case UI_BLUETEAMNAME:
      return UI_TeamName_HandleKey(flags, special, key, qtrue);
      break;
    case UI_REDTEAMNAME:
      return UI_TeamName_HandleKey(flags, special, key, qfalse);
      break;
    case UI_BLUETEAM1:
		case UI_BLUETEAM2:
		case UI_BLUETEAM3:
		case UI_BLUETEAM4:
		case UI_BLUETEAM5:
		case UI_BLUETEAM6:
		case UI_BLUETEAM7:
		case UI_BLUETEAM8:
	if (ownerDraw <= UI_BLUETEAM5)
	{
	  iUse = ownerDraw-UI_BLUETEAM1 + 1;
	}
	else
	{
	  iUse = ownerDraw-274; //unpleasent hack because I don't want to move up all the UI_BLAHTEAM# defines
	}

      UI_TeamMember_HandleKey(flags, special, key, qtrue, iUse);
      break;
    case UI_REDTEAM1:
		case UI_REDTEAM2:
		case UI_REDTEAM3:
		case UI_REDTEAM4:
		case UI_REDTEAM5:
		case UI_REDTEAM6:
		case UI_REDTEAM7:
		case UI_REDTEAM8:
	if (ownerDraw <= UI_REDTEAM5)
	{
	  iUse = ownerDraw-UI_REDTEAM1 + 1;
	}
	else
	{
	  iUse = ownerDraw-277; //unpleasent hack because I don't want to move up all the UI_BLAHTEAM# defines
	}
      UI_TeamMember_HandleKey(flags, special, key, qfalse, iUse);
      break;
		case UI_NETSOURCE:
      UI_NetSource_HandleKey(flags, special, key);
			break;
		case UI_NETFILTER:
      UI_NetFilter_HandleKey(flags, special, key);
			break;
		case UI_OPPONENT_NAME:
			UI_OpponentName_HandleKey(flags, special, key);
			break;
		case UI_BOTNAME:
			return UI_BotName_HandleKey(flags, special, key);
			break;
		case UI_BOTSKILL:
			return UI_BotSkill_HandleKey(flags, special, key);
			break;
		case UI_REDBLUE:
			UI_RedBlue_HandleKey(flags, special, key);
			break;
		case UI_CROSSHAIR:
			UI_Crosshair_HandleKey(flags, special, key);
			break;
		case UI_SELECTEDPLAYER:
			UI_SelectedPlayer_HandleKey(flags, special, key);
			break;
	//	case UI_VOICECHAT:
	//		UI_VoiceChat_HandleKey(flags, special, key);
	//		break;
    default:
      break;
  }

  return qfalse;
}

static float UI_GetValue(int ownerDraw) {
	return 0;
}

/*
=================
UI_ServersQsortCompare
=================
*/
static int QDECL UI_ServersQsortCompare( const void *arg1, const void *arg2 ) {
	return trap->LAN_CompareServers( UI_SourceForLAN(), uiInfo.serverStatus.sortKey, uiInfo.serverStatus.sortDir, *(int*)arg1, *(int*)arg2);
}

/*
=================
UI_ServersSort
=================
*/
void UI_ServersSort(int column, qboolean force) {
	if ( !force ) {
		if ( uiInfo.serverStatus.sortKey == column ) {
			return;
		}
	}

	uiInfo.serverStatus.sortKey = column;
	qsort( &uiInfo.serverStatus.displayServers[0], uiInfo.serverStatus.numDisplayServers, sizeof(int), UI_ServersQsortCompare);
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
	char	version[MAX_CVAR_VALUE_STRING] = {0};

	trap->SE_GetStringTextString("MENUS_ALL", sAll, sizeof(sAll));

	// To still display base game with old engine
	Q_strncpyz( version, UI_Cvar_VariableString( "version" ), sizeof(version) );
	if ( strstr( version, "2003" ) ) {
		trap->SE_GetStringTextString("MENUS_JEDI_ACADEMY", sJediAcademy, sizeof(sJediAcademy));
		uiInfo.modList[0].modName = String_Alloc("");
		uiInfo.modList[0].modDescr = String_Alloc(sJediAcademy);
		uiInfo.modCount = 1;
	}
	else
		uiInfo.modCount = 0;

	numdirs = trap->FS_GetFileList( "$modlist", "", dirlist, sizeof(dirlist) );
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
===============
UI_LoadMovies
===============
*/
static void UI_LoadMovies() {
	char	movielist[4096];
	char	*moviename;
	int		i, len;

	uiInfo.movieCount = trap->FS_GetFileList( "video", "roq", movielist, 4096 );

	if (uiInfo.movieCount) {
		if (uiInfo.movieCount > MAX_MOVIES) {
			uiInfo.movieCount = MAX_MOVIES;
		}
		moviename = movielist;
		for ( i = 0; i < uiInfo.movieCount; i++ ) {
			len = strlen( moviename );
			if (!Q_stricmp(moviename + len - 4, ".roq")) {
				moviename[len-4] = '\0';
			}
			Q_strupr(moviename);
			uiInfo.movieList[i] = String_Alloc(moviename);
			moviename += len + 1;
		}
	}
}

/*
===============
UI_LoadDemos
===============
*/
#define MAX_DEMO_FOLDER_DEPTH (8)
typedef struct loadDemoContext_s
{
	int depth;
	qboolean warned;
	char demoList[MAX_DEMOLIST];
	char directoryList[MAX_DEMOLIST];
	char *dirListHead;
} loadDemoContext_t;

static void UI_LoadDemosInDirectory( loadDemoContext_t *ctx, const char *directory )
{
	char *demoname = NULL;
	char demoExt[32] = {0};
	int protocol = trap->Cvar_VariableValue( "com_protocol" );
	int protocolLegacy = trap->Cvar_VariableValue( "com_legacyprotocol" );
	char *dirListEnd;
	int j;

	if ( ctx->depth > MAX_DEMO_FOLDER_DEPTH )
	{
		if ( !ctx->warned )
		{
			ctx->warned = qtrue;
			Com_Printf( S_COLOR_YELLOW "WARNING: Maximum demo folder depth (%d) was reached.\n", MAX_DEMO_FOLDER_DEPTH );
		}

		return;
	}

	ctx->depth++;

	if ( !protocol )
		protocol = trap->Cvar_VariableValue( "protocol" );
	if ( protocolLegacy == protocol )
		protocolLegacy = 0;

	Com_sprintf( demoExt, sizeof( demoExt ), ".%s%d", DEMO_EXTENSION, protocol);

	uiInfo.demoCount += trap->FS_GetFileList( directory, demoExt, ctx->demoList, sizeof( ctx->demoList ) );

	demoname = ctx->demoList;

	for ( j = 0; j < 2; j++ )
	{
		if ( uiInfo.demoCount > MAX_DEMOS )
			uiInfo.demoCount = MAX_DEMOS;

		for( ; uiInfo.loadedDemos<uiInfo.demoCount; uiInfo.loadedDemos++)
		{
			char dirPath[MAX_QPATH];
			size_t len;

			Q_strncpyz( dirPath, directory + strlen( DEMO_DIRECTORY ), sizeof( dirPath ) );
			Q_strcat( dirPath, sizeof( dirPath ), "/" );
			len = strlen( demoname );
			Com_sprintf( uiInfo.demoList[uiInfo.loadedDemos], sizeof( uiInfo.demoList[0] ), "%s%s", dirPath + 1, demoname );
			demoname += len + 1;
		}

		if ( !j )
		{
			if ( protocolLegacy > 0 && uiInfo.demoCount < MAX_DEMOS )
			{
				Com_sprintf( demoExt, sizeof( demoExt ), ".%s%d", DEMO_EXTENSION, protocolLegacy );
				uiInfo.demoCount += trap->FS_GetFileList( directory, demoExt, ctx->demoList, sizeof( ctx->demoList ) );
				demoname = ctx->demoList;
			}
			else
				break;
		}
	}

	dirListEnd = ctx->directoryList + sizeof( ctx->directoryList );
	if ( ctx->dirListHead < dirListEnd )
	{
		int i;
		int dirListSpaceRemaining = dirListEnd - ctx->dirListHead;
		int numFiles = trap->FS_GetFileList( directory, "/", ctx->dirListHead, dirListSpaceRemaining );
		char *dirList;
		char *childDirListBase;
		char *fileName;

		// Find end of this list so we have a base pointer for the child folders to use
		dirList = ctx->dirListHead;
		for ( i = 0; i < numFiles; i++ )
		{
			ctx->dirListHead += strlen( ctx->dirListHead ) + 1;
		}
		ctx->dirListHead++;

		// Iterate through child directories
		childDirListBase = ctx->dirListHead;
		fileName = dirList;
		for ( i = 0; i < numFiles; i++ )
		{
			size_t len = strlen( fileName );

			if ( Q_stricmp( fileName, "." ) && Q_stricmp( fileName, ".." ) && len )
				UI_LoadDemosInDirectory( ctx, va( "%s/%s", directory, fileName ) );

			ctx->dirListHead = childDirListBase;
			fileName += len+1;
		}

		assert( (fileName + 1) == childDirListBase );
	}

	ctx->depth--;
}

static void InitLoadDemoContext( loadDemoContext_t *ctx )
{
	ctx->warned = qfalse;
	ctx->depth = 0;
	ctx->dirListHead = ctx->directoryList;
}

static void UI_LoadDemos( void )
{
	loadDemoContext_t loadDemoContext;
	InitLoadDemoContext( &loadDemoContext );

	uiInfo.demoCount = 0;
	uiInfo.loadedDemos = 0;
	memset( uiInfo.demoList, 0, sizeof( uiInfo.demoList ) );
	UI_LoadDemosInDirectory( &loadDemoContext, DEMO_DIRECTORY );
}

static qboolean UI_SetNextMap(int actual, int index) {
	int i;
	for (i = actual + 1; i < uiInfo.mapCount; i++) {
		if (uiInfo.mapList[i].active) {
			Menu_SetFeederSelection(NULL, FEEDER_MAPS, index + 1, "skirmish");
			return qtrue;
		}
	}
	return qfalse;
}

static void UI_StartSkirmish(qboolean next) {
	int i, k, g, delay, temp;
	float skill;
	char buff[MAX_STRING_CHARS];

	temp = trap->Cvar_VariableValue( "g_gametype" );
	trap->Cvar_Set("ui_gameType", va("%i", temp));

	if (next) {
		int actual;
		int index = trap->Cvar_VariableValue("ui_mapIndex");
		UI_MapCountByGameType(qtrue);
		UI_SelectedMap(index, &actual);
		if (UI_SetNextMap(actual, index)) {
		} else {
			UI_GameType_HandleKey(0, 0, A_MOUSE1, qfalse);
			UI_MapCountByGameType(qtrue);
			Menu_SetFeederSelection(NULL, FEEDER_MAPS, 0, "skirmish");
		}
	}

	g = uiInfo.gameTypes[ui_gametype.integer].gtEnum;
	trap->Cvar_SetValue( "g_gametype", g );
	trap->Cmd_ExecuteText( EXEC_APPEND, va( "wait ; wait ; map %s\n", uiInfo.mapList[ui_currentMap.integer].mapLoadName) );
	skill = trap->Cvar_VariableValue( "g_spSkill" );
	trap->Cvar_Set("ui_scoreMap", uiInfo.mapList[ui_currentMap.integer].mapName);

	k = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_opponentName"));

	trap->Cvar_Set("ui_singlePlayerActive", "1");

	// set up sp overrides, will be replaced on postgame
	temp = trap->Cvar_VariableValue( "capturelimit" );	trap->Cvar_Set("ui_saveCaptureLimit", va("%i", temp));
	temp = trap->Cvar_VariableValue( "fraglimit" );		trap->Cvar_Set("ui_saveFragLimit", va("%i", temp));
	temp = trap->Cvar_VariableValue( "duel_fraglimit" );	trap->Cvar_Set("ui_saveDuelLimit", va("%i", temp));

	UI_SetCapFragLimits(qfalse);

	temp = trap->Cvar_VariableValue( "cg_drawTimer" );	trap->Cvar_Set("ui_drawTimer", va("%i", temp));
	temp = trap->Cvar_VariableValue( "g_doWarmup" );		trap->Cvar_Set("ui_doWarmup", va("%i", temp));
	temp = trap->Cvar_VariableValue( "g_friendlyFire" );	trap->Cvar_Set("ui_friendlyFire", va("%i", temp));
	temp = trap->Cvar_VariableValue( "sv_maxClients" );	trap->Cvar_Set("ui_maxClients", va("%i", temp));
	temp = trap->Cvar_VariableValue( "g_warmup" );		trap->Cvar_Set("ui_Warmup", va("%i", temp));
	temp = trap->Cvar_VariableValue( "sv_pure" );			trap->Cvar_Set("ui_pure", va("%i", temp));

	trap->Cvar_Set("cg_cameraOrbit", "0");
//	trap->Cvar_Set("cg_thirdPerson", "0");
	trap->Cvar_Set("cg_drawTimer", "1");
	trap->Cvar_Set("g_doWarmup", "1");
	trap->Cvar_Set("g_warmup", "15");
	trap->Cvar_Set("sv_pure", "0");
	trap->Cvar_Set("g_friendlyFire", "0");
//	trap->Cvar_Set("g_redTeam", UI_Cvar_VariableString("ui_teamName"));
//	trap->Cvar_Set("g_blueTeam", UI_Cvar_VariableString("ui_opponentName"));

	if (trap->Cvar_VariableValue("ui_recordSPDemo")) {
		Com_sprintf(buff, MAX_STRING_CHARS, "%s_%i", uiInfo.mapList[ui_currentMap.integer].mapLoadName, g);
		trap->Cvar_Set("ui_recordSPDemoName", buff);
	}

	delay = 500;

	if (g == GT_DUEL || g == GT_POWERDUEL) {
		temp = uiInfo.mapList[ui_currentMap.integer].teamMembers * 2;
		trap->Cvar_Set("sv_maxClients", va("%d", temp));
		Com_sprintf( buff, sizeof(buff), "wait ; addbot %s %f "", %i \n", uiInfo.mapList[ui_currentMap.integer].opponentName, skill, delay);
		trap->Cmd_ExecuteText( EXEC_APPEND, buff );
	} else if (g == GT_HOLOCRON || g == GT_JEDIMASTER) {
		temp = uiInfo.mapList[ui_currentMap.integer].teamMembers * 2;
		trap->Cvar_Set("sv_maxClients", va("%d", temp));
		for (i =0; i < uiInfo.mapList[ui_currentMap.integer].teamMembers; i++) {
			Com_sprintf( buff, sizeof(buff), "addbot \"%s\" %f %s %i %s\n", UI_AIFromName(uiInfo.teamList[k].teamMembers[i]), skill, (g == GT_HOLOCRON) ? "" : "Blue", delay, uiInfo.teamList[k].teamMembers[i]);
			trap->Cmd_ExecuteText( EXEC_APPEND, buff );
			delay += 500;
		}
		k = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_teamName"));
		for (i =0; i < uiInfo.mapList[ui_currentMap.integer].teamMembers-1; i++) {
			Com_sprintf( buff, sizeof(buff), "addbot \"%s\" %f %s %i %s\n", UI_AIFromName(uiInfo.teamList[k].teamMembers[i]), skill, (g == GT_HOLOCRON) ? "" : "Red", delay, uiInfo.teamList[k].teamMembers[i]);
			trap->Cmd_ExecuteText( EXEC_APPEND, buff );
			delay += 500;
		}
	} else {
		temp = uiInfo.mapList[ui_currentMap.integer].teamMembers * 2;
		trap->Cvar_Set("sv_maxClients", va("%d", temp));
		for (i =0; i < uiInfo.mapList[ui_currentMap.integer].teamMembers; i++) {
			Com_sprintf( buff, sizeof(buff), "addbot \"%s\" %f %s %i %s\n", UI_AIFromName(uiInfo.teamList[k].teamMembers[i]), skill, (g == GT_FFA) ? "" : "Blue", delay, uiInfo.teamList[k].teamMembers[i]);
			trap->Cmd_ExecuteText( EXEC_APPEND, buff );
			delay += 500;
		}
		k = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_teamName"));
		for (i =0; i < uiInfo.mapList[ui_currentMap.integer].teamMembers-1; i++) {
			Com_sprintf( buff, sizeof(buff), "addbot \"%s\" %f %s %i %s\n", UI_AIFromName(uiInfo.teamList[k].teamMembers[i]), skill, (g == GT_FFA) ? "" : "Red", delay, uiInfo.teamList[k].teamMembers[i]);
			trap->Cmd_ExecuteText( EXEC_APPEND, buff );
			delay += 500;
		}
	}
	if (g >= GT_TEAM ) {
		trap->Cmd_ExecuteText( EXEC_APPEND, "wait 5; team Red\n" );
	}
}

static void UI_Update(const char *name) {
	int	val = trap->Cvar_VariableValue(name);

	if (Q_stricmp(name, "s_khz") == 0)
	{
		trap->Cmd_ExecuteText( EXEC_APPEND, "snd_restart\n" );
		return;
	}

	if ( !Q_stricmp( name, "ui_SetName" ) ) {
		char buf[MAX_NETNAME] = {0};
		Q_strncpyz( buf, UI_Cvar_VariableString( "ui_Name" ), sizeof( buf ) );
		trap->Cvar_Set( "name", buf );
	}
	else if (Q_stricmp(name, "ui_setRate") == 0) {
		float rate = trap->Cvar_VariableValue("rate");
		if (rate >= 5000) {
			trap->Cvar_Set("cl_maxpackets", "30");
			trap->Cvar_Set("cl_packetdup", "1");
		} else if (rate >= 4000) {
			trap->Cvar_Set("cl_maxpackets", "15");
			trap->Cvar_Set("cl_packetdup", "2");		// favor less prediction errors when there's packet loss
		} else {
			trap->Cvar_Set("cl_maxpackets", "15");
			trap->Cvar_Set("cl_packetdup", "1");		// favor lower bandwidth
		}
	}
	else if ( !Q_stricmp( name, "ui_GetName" ) ) {
		char buf[MAX_NETNAME] = {0};
		Q_strncpyz( buf, UI_Cvar_VariableString( "name" ), sizeof( buf ) );
		trap->Cvar_Set( "ui_Name", buf );
	}
	else if (Q_stricmp(name, "ui_r_colorbits") == 0)
	{
		switch (val)
		{
			case 0:
				trap->Cvar_SetValue( "ui_r_depthbits", 0 );
				break;

			case 16:
				trap->Cvar_SetValue( "ui_r_depthbits", 16 );
				break;

			case 32:
				trap->Cvar_SetValue( "ui_r_depthbits", 24 );
				break;
		}
	}
	else if (Q_stricmp(name, "ui_r_lodbias") == 0)
	{
		switch (val)
		{
			case 0:
				trap->Cvar_SetValue( "ui_r_subdivisions", 4 );
				break;
			case 1:
				trap->Cvar_SetValue( "ui_r_subdivisions", 12 );
				break;

			case 2:
				trap->Cvar_SetValue( "ui_r_subdivisions", 20 );
				break;
		}
	}
	else if (Q_stricmp(name, "ui_r_glCustom") == 0)
	{
		switch (val)
		{
		case 0:	// high quality

			trap->Cvar_SetValue( "ui_r_fullScreen", 1 );
			trap->Cvar_SetValue( "ui_r_subdivisions", 4 );
			trap->Cvar_SetValue( "ui_r_lodbias", 0 );
			trap->Cvar_SetValue( "ui_r_colorbits", 32 );
			trap->Cvar_SetValue( "ui_r_depthbits", 24 );
			trap->Cvar_SetValue( "ui_r_picmip", 0 );
			trap->Cvar_SetValue( "ui_r_mode", 4 );
			trap->Cvar_SetValue( "ui_r_texturebits", 32 );
			trap->Cvar_SetValue( "ui_r_fastSky", 0 );
			trap->Cvar_SetValue( "ui_r_inGameVideo", 1 );
		//	trap->Cvar_SetValue( "ui_cg_shadows", 2 );//stencil
			trap->Cvar_Set( "ui_r_texturemode", "GL_LINEAR_MIPMAP_LINEAR" );
			break;

		case 1: // normal
			trap->Cvar_SetValue( "ui_r_fullScreen", 1 );
			trap->Cvar_SetValue( "ui_r_subdivisions", 4 );
			trap->Cvar_SetValue( "ui_r_lodbias", 0 );
			trap->Cvar_SetValue( "ui_r_colorbits", 0 );
			trap->Cvar_SetValue( "ui_r_depthbits", 24 );
			trap->Cvar_SetValue( "ui_r_picmip", 1 );
			trap->Cvar_SetValue( "ui_r_mode", 3 );
			trap->Cvar_SetValue( "ui_r_texturebits", 0 );
			trap->Cvar_SetValue( "ui_r_fastSky", 0 );
			trap->Cvar_SetValue( "ui_r_inGameVideo", 1 );
		//	trap->Cvar_SetValue( "ui_cg_shadows", 2 );
			trap->Cvar_Set( "ui_r_texturemode", "GL_LINEAR_MIPMAP_LINEAR" );
			break;

		case 2: // fast

			trap->Cvar_SetValue( "ui_r_fullScreen", 1 );
			trap->Cvar_SetValue( "ui_r_subdivisions", 12 );
			trap->Cvar_SetValue( "ui_r_lodbias", 1 );
			trap->Cvar_SetValue( "ui_r_colorbits", 0 );
			trap->Cvar_SetValue( "ui_r_depthbits", 0 );
			trap->Cvar_SetValue( "ui_r_picmip", 2 );
			trap->Cvar_SetValue( "ui_r_mode", 3 );
			trap->Cvar_SetValue( "ui_r_texturebits", 0 );
			trap->Cvar_SetValue( "ui_r_fastSky", 1 );
			trap->Cvar_SetValue( "ui_r_inGameVideo", 0 );
		//	trap->Cvar_SetValue( "ui_cg_shadows", 1 );
			trap->Cvar_Set( "ui_r_texturemode", "GL_LINEAR_MIPMAP_NEAREST" );
			break;

		case 3: // fastest

			trap->Cvar_SetValue( "ui_r_fullScreen", 1 );
			trap->Cvar_SetValue( "ui_r_subdivisions", 20 );
			trap->Cvar_SetValue( "ui_r_lodbias", 2 );
			trap->Cvar_SetValue( "ui_r_colorbits", 16 );
			trap->Cvar_SetValue( "ui_r_depthbits", 16 );
			trap->Cvar_SetValue( "ui_r_mode", 3 );
			trap->Cvar_SetValue( "ui_r_picmip", 3 );
			trap->Cvar_SetValue( "ui_r_texturebits", 16 );
			trap->Cvar_SetValue( "ui_r_fastSky", 1 );
			trap->Cvar_SetValue( "ui_r_inGameVideo", 0 );
		//	trap->Cvar_SetValue( "ui_cg_shadows", 0 );
			trap->Cvar_Set( "ui_r_texturemode", "GL_LINEAR_MIPMAP_NEAREST" );
			break;
		}
	}
	else if (Q_stricmp(name, "ui_mousePitch") == 0)
	{
		if (val == 0)	trap->Cvar_SetValue( "m_pitch", 0.022f );
		else			trap->Cvar_SetValue( "m_pitch", -0.022f );
	}
	else if (Q_stricmp(name, "ui_mousePitchVeh") == 0)
	{
		if (val == 0)	trap->Cvar_SetValue( "m_pitchVeh", 0.022f );
		else 			trap->Cvar_SetValue( "m_pitchVeh", -0.022f );
	}
}

int gUISelectedMap = 0;

/*
===============
UI_DeferMenuScript

Return true if the menu script should be deferred for later
===============
*/
static qboolean UI_DeferMenuScript ( char **args )
{
	const char* name;

	// Whats the reason for being deferred?
	if (!String_Parse( (char**)args, &name))
	{
		return qfalse;
	}

	// Handle the custom cases
	if ( !Q_stricmp ( name, "VideoSetup" ) )
	{
		const char* warningMenuName;
		qboolean	deferred;

		// No warning menu specified
		if ( !String_Parse( (char**)args, &warningMenuName) )
		{
			return qfalse;
		}

		// Defer if the video options were modified
		deferred = trap->Cvar_VariableValue ( "ui_r_modified" ) ? qtrue : qfalse;

		if ( deferred )
		{
			// Open the warning menu
			Menus_OpenByName(warningMenuName);
		}

		return deferred;
	}
	else if ( !Q_stricmp ( name, "RulesBackout" ) )
	{
		qboolean deferred;

		deferred = trap->Cvar_VariableValue ( "ui_rules_backout" ) ? qtrue : qfalse ;

		trap->Cvar_Set ( "ui_rules_backout", "0" );

		return deferred;
	}

	return qfalse;
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
	trap->Cvar_Set ( "r_mode", UI_Cvar_VariableString ( "ui_r_mode" ) );
	trap->Cvar_Set ( "r_fullscreen", UI_Cvar_VariableString ( "ui_r_fullscreen" ) );
	trap->Cvar_Set ( "r_colorbits", UI_Cvar_VariableString ( "ui_r_colorbits" ) );
	trap->Cvar_Set ( "r_lodbias", UI_Cvar_VariableString ( "ui_r_lodbias" ) );
	trap->Cvar_Set ( "r_picmip", UI_Cvar_VariableString ( "ui_r_picmip" ) );
	trap->Cvar_Set ( "r_texturebits", UI_Cvar_VariableString ( "ui_r_texturebits" ) );
	trap->Cvar_Set ( "r_texturemode", UI_Cvar_VariableString ( "ui_r_texturemode" ) );
	trap->Cvar_Set ( "r_detailtextures", UI_Cvar_VariableString ( "ui_r_detailtextures" ) );
	trap->Cvar_Set ( "r_ext_compress_textures", UI_Cvar_VariableString ( "ui_r_ext_compress_textures" ) );
	trap->Cvar_Set ( "r_depthbits", UI_Cvar_VariableString ( "ui_r_depthbits" ) );
	trap->Cvar_Set ( "r_subdivisions", UI_Cvar_VariableString ( "ui_r_subdivisions" ) );
	trap->Cvar_Set ( "r_fastSky", UI_Cvar_VariableString ( "ui_r_fastSky" ) );
	trap->Cvar_Set ( "r_inGameVideo", UI_Cvar_VariableString ( "ui_r_inGameVideo" ) );
	trap->Cvar_Set ( "r_allowExtensions", UI_Cvar_VariableString ( "ui_r_allowExtensions" ) );
	trap->Cvar_Set ( "cg_shadows", UI_Cvar_VariableString ( "ui_cg_shadows" ) );
	trap->Cvar_Set ( "ui_r_modified", "0" );

	trap->Cmd_ExecuteText( EXEC_APPEND, "vid_restart;" );
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
	trap->Cvar_Register ( NULL, "ui_r_glCustom",				"4", CVAR_INTERNAL|CVAR_ARCHIVE );

	// Make sure the cvars are registered as read only.
	trap->Cvar_Register ( NULL, "ui_r_mode",					"0", CVAR_ROM|CVAR_INTERNAL );
	trap->Cvar_Register ( NULL, "ui_r_fullscreen",			"0", CVAR_ROM|CVAR_INTERNAL );
	trap->Cvar_Register ( NULL, "ui_r_colorbits",				"0", CVAR_ROM|CVAR_INTERNAL );
	trap->Cvar_Register ( NULL, "ui_r_lodbias",				"0", CVAR_ROM|CVAR_INTERNAL );
	trap->Cvar_Register ( NULL, "ui_r_picmip",				"0", CVAR_ROM|CVAR_INTERNAL );
	trap->Cvar_Register ( NULL, "ui_r_texturebits",			"0", CVAR_ROM|CVAR_INTERNAL );
	trap->Cvar_Register ( NULL, "ui_r_texturemode",			"0", CVAR_ROM|CVAR_INTERNAL );
	trap->Cvar_Register ( NULL, "ui_r_detailtextures",		"0", CVAR_ROM|CVAR_INTERNAL );
	trap->Cvar_Register ( NULL, "ui_r_ext_compress_textures",	"0", CVAR_ROM|CVAR_INTERNAL );
	trap->Cvar_Register ( NULL, "ui_r_depthbits",				"0", CVAR_ROM|CVAR_INTERNAL );
	trap->Cvar_Register ( NULL, "ui_r_subdivisions",			"0", CVAR_ROM|CVAR_INTERNAL );
	trap->Cvar_Register ( NULL, "ui_r_fastSky",				"0", CVAR_ROM|CVAR_INTERNAL );
	trap->Cvar_Register ( NULL, "ui_r_inGameVideo",			"0", CVAR_ROM|CVAR_INTERNAL );
	trap->Cvar_Register ( NULL, "ui_r_allowExtensions",		"0", CVAR_ROM|CVAR_INTERNAL );
	trap->Cvar_Register ( NULL, "ui_cg_shadows",				"0", CVAR_ROM|CVAR_INTERNAL );
	trap->Cvar_Register ( NULL, "ui_r_modified",				"0", CVAR_ROM|CVAR_INTERNAL );

	// Copy over the real video cvars into their temporary counterparts
	trap->Cvar_Set ( "ui_r_mode",						UI_Cvar_VariableString ( "r_mode" ) );
	trap->Cvar_Set ( "ui_r_colorbits",				UI_Cvar_VariableString ( "r_colorbits" ) );
	trap->Cvar_Set ( "ui_r_fullscreen",				UI_Cvar_VariableString ( "r_fullscreen" ) );
	trap->Cvar_Set ( "ui_r_lodbias",					UI_Cvar_VariableString ( "r_lodbias" ) );
	trap->Cvar_Set ( "ui_r_picmip",					UI_Cvar_VariableString ( "r_picmip" ) );
	trap->Cvar_Set ( "ui_r_texturebits",				UI_Cvar_VariableString ( "r_texturebits" ) );
	trap->Cvar_Set ( "ui_r_texturemode",				UI_Cvar_VariableString ( "r_texturemode" ) );
	trap->Cvar_Set ( "ui_r_detailtextures",			UI_Cvar_VariableString ( "r_detailtextures" ) );
	trap->Cvar_Set ( "ui_r_ext_compress_textures",	UI_Cvar_VariableString ( "r_ext_compress_textures" ) );
	trap->Cvar_Set ( "ui_r_depthbits",				UI_Cvar_VariableString ( "r_depthbits" ) );
	trap->Cvar_Set ( "ui_r_subdivisions",				UI_Cvar_VariableString ( "r_subdivisions" ) );
	trap->Cvar_Set ( "ui_r_fastSky",					UI_Cvar_VariableString ( "r_fastSky" ) );
	trap->Cvar_Set ( "ui_r_inGameVideo",				UI_Cvar_VariableString ( "r_inGameVideo" ) );
	trap->Cvar_Set ( "ui_r_allowExtensions",			UI_Cvar_VariableString ( "r_allowExtensions" ) );
	trap->Cvar_Set ( "ui_cg_shadows",					UI_Cvar_VariableString ( "cg_shadows" ) );
	trap->Cvar_Set ( "ui_r_modified",					"0" );
}

// If the game type is siege, hide the addbot button. I would have done a cvar text on that item,
// but it already had one on it.
static void UI_SetBotButton ( void )
{
	int gameType = trap->Cvar_VariableValue( "g_gametype" );
	int server;
	menuDef_t *menu;
	itemDef_t *item;
	char *name = "addBot";

	server = trap->Cvar_VariableValue( "sv_running" );

	// If in siege or a client, don't show add bot button
	if ((gameType==GT_SIEGE) || (server==0))	// If it's not siege, don't worry about it
	{
		menu = Menu_GetFocused();	// Get current menu (either video or ingame video, I would assume)

		if (!menu)
		{
			return;
		}

		item = Menu_FindItemByName(menu, name);
		if (item)
		{
			Menu_ShowItemByName(menu, name, qfalse);
		}
	}
}

// Update the model cvar and everything is good.
static void UI_UpdateCharacterCvars ( void )
{
	char skin[MAX_QPATH];
	char model[MAX_QPATH];
	char head[MAX_QPATH];
	char torso[MAX_QPATH];
	char legs[MAX_QPATH];

	trap->Cvar_VariableStringBuffer("ui_char_model", model, sizeof(model));
	trap->Cvar_VariableStringBuffer("ui_char_skin_head", head, sizeof(head));
	trap->Cvar_VariableStringBuffer("ui_char_skin_torso", torso, sizeof(torso));
	trap->Cvar_VariableStringBuffer("ui_char_skin_legs", legs, sizeof(legs));

	Com_sprintf( skin, sizeof( skin ), "%s/%s|%s|%s",
										model,
										head,
										torso,
										legs
				);

	trap->Cvar_Set ( "model", skin );

	trap->Cvar_Set ( "char_color_red", UI_Cvar_VariableString ( "ui_char_color_red" ) );
	trap->Cvar_Set ( "char_color_green", UI_Cvar_VariableString ( "ui_char_color_green" ) );
	trap->Cvar_Set ( "char_color_blue", UI_Cvar_VariableString ( "ui_char_color_blue" ) );
	trap->Cvar_Set ( "ui_selectedModelIndex", "-1");

}

static void UI_GetCharacterCvars ( void )
{
	char *model;
	char *skin;
	int i;

	trap->Cvar_Set ( "ui_char_color_red", UI_Cvar_VariableString ( "char_color_red" ) );
	trap->Cvar_Set ( "ui_char_color_green", UI_Cvar_VariableString ( "char_color_green" ) );
	trap->Cvar_Set ( "ui_char_color_blue", UI_Cvar_VariableString ( "char_color_blue" ) );

	model = UI_Cvar_VariableString ( "model" );
	skin = strrchr(model,'/');
	if (skin && strchr(model,'|'))	//we have a multipart custom jedi
	{
		char skinhead[MAX_QPATH];
		char skintorso[MAX_QPATH];
		char skinlower[MAX_QPATH];
		char *p2;

		*skin=0;
		skin++;
		//now get the the individual files

		//advance to second
		p2 = strchr(skin, '|');
		assert(p2);
		*p2=0;
		p2++;
		Q_strncpyz (skinhead, skin, sizeof(skinhead));


		//advance to third
		skin = strchr(p2, '|');
		assert(skin);
		*skin=0;
		skin++;
		Q_strncpyz (skintorso,p2, sizeof(skintorso));

		Q_strncpyz (skinlower,skin, sizeof(skinlower));



		trap->Cvar_Set("ui_char_model", model);
		trap->Cvar_Set("ui_char_skin_head", skinhead);
		trap->Cvar_Set("ui_char_skin_torso", skintorso);
		trap->Cvar_Set("ui_char_skin_legs", skinlower);

		for (i = 0; i < uiInfo.playerSpeciesCount; i++)
		{
			if ( !Q_stricmp(model, uiInfo.playerSpecies[i].Name) )
			{
				uiInfo.playerSpeciesIndex = i;
				break;
			}
		}
	}
	else
	{
		model = UI_Cvar_VariableString ( "ui_char_model" );
		for (i = 0; i < uiInfo.playerSpeciesCount; i++)
		{
			if ( !Q_stricmp(model, uiInfo.playerSpecies[i].Name) )
			{
				uiInfo.playerSpeciesIndex = i;
				return;	//FOUND IT, don't fall through
			}
		}
		//nope, didn't find it.
		uiInfo.playerSpeciesIndex = 0;//jic
		trap->Cvar_Set("ui_char_model", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Name);
		trap->Cvar_Set("ui_char_skin_head", "head_a1");
		trap->Cvar_Set("ui_char_skin_torso","torso_a1");
		trap->Cvar_Set("ui_char_skin_legs", "lower_a1");
	}
}

void UI_SetSiegeObjectiveGraphicPos(menuDef_t *menu,const char *itemName,const char *cvarName)
{
	itemDef_t	*item;
	char		cvarBuf[1024];
	const char	*holdVal;
	char		*holdBuf;

	item = Menu_FindItemByName(menu, itemName);

	if (item)
	{
		// get cvar data
		trap->Cvar_VariableStringBuffer(cvarName, cvarBuf, sizeof(cvarBuf));

		holdBuf = cvarBuf;
		if (String_Parse(&holdBuf,&holdVal))
		{
			item->window.rectClient.x = atof(holdVal);
			if (String_Parse(&holdBuf,&holdVal))
			{
				item->window.rectClient.y = atof(holdVal);
				if (String_Parse(&holdBuf,&holdVal))
				{
					item->window.rectClient.w = atof(holdVal);
					if (String_Parse(&holdBuf,&holdVal))
					{
						item->window.rectClient.h = atof(holdVal);

						item->window.rect.x = item->window.rectClient.x;
						item->window.rect.y = item->window.rectClient.y;

						item->window.rect.w = item->window.rectClient.w;
						item->window.rect.h = item->window.rectClient.h;
					}
				}
			}
		}
	}
}

void UI_FindCurrentSiegeTeamClass( void )
{
	menuDef_t *menu;
	int myTeam = (int)(trap->Cvar_VariableValue("ui_myteam"));
	char *itemname;
	itemDef_t *item;
	int	baseClass;

	menu = Menu_GetFocused();	// Get current menu

	if (!menu)
	{
		return;
	}

	if (( myTeam != TEAM_RED ) && ( myTeam != TEAM_BLUE ))
	{
		return;
	}

	// If the player is on a team,
	if ( myTeam == TEAM_RED )
	{
		itemDef_t *item;
		item = (itemDef_t *) Menu_FindItemByName(menu, "onteam1" );
		if (item)
		{
		    Item_RunScript(item, item->action);
		}
	}
	else if ( myTeam == TEAM_BLUE )
	{
		itemDef_t *item;
		item = (itemDef_t *) Menu_FindItemByName(menu, "onteam2" );
		if (item)
		{
		    Item_RunScript(item, item->action);
		}
	}


	baseClass = (int)trap->Cvar_VariableValue("ui_siege_class");

	// Find correct class button and activate it.
	switch ( baseClass ) {
	case SPC_INFANTRY:
		itemname = "class1_button";
		break;
	case SPC_HEAVY_WEAPONS:
		itemname = "class2_button";
		break;
	case SPC_DEMOLITIONIST:
		itemname = "class3_button";
		break;
	case SPC_VANGUARD:
		itemname = "class4_button";
		break;
	case SPC_SUPPORT:
		itemname = "class5_button";
		break;
	case SPC_JEDI:
		itemname = "class6_button";
		break;
	default:
		return;
	}

	item = (itemDef_t *) Menu_FindItemByName(menu, itemname );
	if (item)
	{
		Item_RunScript(item, item->action);
	}
}

void UI_UpdateSiegeObjectiveGraphics( void )
{
	menuDef_t *menu;
	int	teamI,objI;

	menu = Menu_GetFocused();	// Get current menu

	if (!menu)
	{
		return;
	}

	// Hiding a bunch of fields because the opening section of the siege menu was getting too long
	Menu_ShowGroup(menu,"class_button",qfalse);
	Menu_ShowGroup(menu,"class_count",qfalse);
	Menu_ShowGroup(menu,"feeders",qfalse);
	Menu_ShowGroup(menu,"classdescription",qfalse);
	Menu_ShowGroup(menu,"minidesc",qfalse);
	Menu_ShowGroup(menu,"obj_longdesc",qfalse);
	Menu_ShowGroup(menu,"objective_pic",qfalse);
	Menu_ShowGroup(menu,"stats",qfalse);
	Menu_ShowGroup(menu,"forcepowerlevel",qfalse);

	// Get objective icons for each team
	for (teamI=1;teamI<3;teamI++)
	{
		for (objI=1;objI<8;objI++)
		{
			Menu_SetItemBackground(menu,va("tm%i_icon%i",teamI,objI),va("*team%i_objective%i_mapicon",teamI,objI));
			Menu_SetItemBackground(menu,va("tm%i_l_icon%i",teamI,objI),va("*team%i_objective%i_mapicon",teamI,objI));
		}
	}

	// Now get their placement on the map
	for (teamI=1;teamI<3;teamI++)
	{
		for (objI=1;objI<8;objI++)
		{
			UI_SetSiegeObjectiveGraphicPos(menu,va("tm%i_icon%i",teamI,objI),va("team%i_objective%i_mappos",teamI,objI));
		}
	}

}

saber_colors_t TranslateSaberColor( const char *name );

static void UI_UpdateSaberCvars ( void )
{
	saber_colors_t colorI;

	trap->Cvar_Set ( "saber1", UI_Cvar_VariableString ( "ui_saber" ) );
	trap->Cvar_Set ( "saber2", UI_Cvar_VariableString ( "ui_saber2" ) );

	colorI = TranslateSaberColor( UI_Cvar_VariableString ( "ui_saber_color" ) );
	trap->Cvar_Set ( "color1", va("%d",colorI));
	trap->Cvar_Set ( "g_saber_color", UI_Cvar_VariableString ( "ui_saber_color" ));

	colorI = TranslateSaberColor( UI_Cvar_VariableString ( "ui_saber2_color" ) );
	trap->Cvar_Set ( "color2", va("%d",colorI) );
	trap->Cvar_Set ( "g_saber2_color", UI_Cvar_VariableString ( "ui_saber2_color" ));
}

// More hard coded goodness for the menus.
static void UI_SetSaberBoxesandHilts (void)
{
	menuDef_t *menu;
	itemDef_t *item;
	qboolean	getBig = qfalse;
	char sType[MAX_QPATH];

	menu = Menu_GetFocused();	// Get current menu (either video or ingame video, I would assume)

	if (!menu)
	{
		return;
	}

	trap->Cvar_VariableStringBuffer( "ui_saber_type", sType, sizeof(sType) );

	if ( Q_stricmp( "dual", sType ) != 0 )
	{
//		trap->Cvar_Set("ui_saber", "single_1");
//		trap->Cvar_Set("ui_saber2", "single_1");
		getBig = qtrue;
	}

	else if (Q_stricmp( "staff", sType ) != 0 )
	{
//		trap->Cvar_Set("ui_saber", "dual_1");
//		trap->Cvar_Set("ui_saber2", "none");
		getBig = qtrue;
	}

	if (!getBig)
	{
		return;
	}

	item = (itemDef_t *) Menu_FindItemByName(menu, "box2middle" );

	if(item)
	{
		item->window.rect.x = 212;
		item->window.rect.y = 126;
		item->window.rect.w = 219;
		item->window.rect.h = 44;
	}

	item = (itemDef_t *) Menu_FindItemByName(menu, "box2bottom" );

	if(item)
	{
		item->window.rect.x = 212;
		item->window.rect.y = 170;
		item->window.rect.w = 219;
		item->window.rect.h = 60;
	}

	item = (itemDef_t *) Menu_FindItemByName(menu, "box3middle" );

	if(item)
	{
		item->window.rect.x = 418;
		item->window.rect.y = 126;
		item->window.rect.w = 219;
		item->window.rect.h = 44;
	}

	item = (itemDef_t *) Menu_FindItemByName(menu, "box3bottom" );

	if(item)
	{
		item->window.rect.x = 418;
		item->window.rect.y = 170;
		item->window.rect.w = 219;
		item->window.rect.h = 60;
	}
}

extern qboolean UI_SaberSkinForSaber( const char *saberName, char *saberSkin );
extern qboolean ItemParse_asset_model_go( itemDef_t *item, const char *name,int *runTimeLength );
extern qboolean ItemParse_model_g2skin_go( itemDef_t *item, const char *skinName );

static void UI_UpdateSaberType( void )
{
	char sType[MAX_QPATH];
	trap->Cvar_VariableStringBuffer( "ui_saber_type", sType, sizeof(sType) );

	if ( Q_stricmp( "single", sType ) == 0 ||
		Q_stricmp( "staff", sType ) == 0 )
	{
		trap->Cvar_Set( "ui_saber2", "" );
	}
}

static void UI_UpdateSaberHilt( qboolean secondSaber )
{
	menuDef_t *menu;
	itemDef_t *item;
	char model[MAX_QPATH];
	char modelPath[MAX_QPATH];
	char skinPath[MAX_QPATH];
	char *itemName;
	char *saberCvarName;
	int	animRunLength;

	menu = Menu_GetFocused();	// Get current menu (either video or ingame video, I would assume)

	if (!menu)
	{
		return;
	}

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

	item = (itemDef_t *) Menu_FindItemByName(menu, itemName );

	if(!item)
	{
		Com_Error( ERR_FATAL, "UI_UpdateSaberHilt: Could not find item (%s) in menu (%s)", itemName, menu->window.name);
	}

	trap->Cvar_VariableStringBuffer( saberCvarName, model, sizeof(model) );

	item->text = model;
	//read this from the sabers.cfg
	if ( UI_SaberModelForSaber( model, modelPath ) )
	{//successfully found a model
		ItemParse_asset_model_go( item, modelPath, &animRunLength );//set the model
		//get the customSkin, if any
		//COM_StripExtension( modelPath, skinPath );
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

static void UI_UpdateSaberColor( qboolean secondSaber )
{
}

const char *SaberColorToString( saber_colors_t color );

static void UI_GetSaberCvars ( void )
{
//	trap->Cvar_Set ( "ui_saber_type", UI_Cvar_VariableString ( "g_saber_type" ) );
	trap->Cvar_Set ( "ui_saber", UI_Cvar_VariableString ( "saber1" ) );
	trap->Cvar_Set ( "ui_saber2", UI_Cvar_VariableString ( "saber2" ));

	trap->Cvar_Set("g_saber_color", SaberColorToString(trap->Cvar_VariableValue("color1")));
	trap->Cvar_Set("g_saber2_color", SaberColorToString(trap->Cvar_VariableValue("color2")));

	trap->Cvar_Set ( "ui_saber_color", UI_Cvar_VariableString ( "g_saber_color" ) );
	trap->Cvar_Set ( "ui_saber2_color", UI_Cvar_VariableString ( "g_saber2_color" ) );
}

extern qboolean ItemParse_model_g2anim_go( itemDef_t *item, const char *animName );

void UI_UpdateCharacterSkin( void )
{
	menuDef_t *menu;
	itemDef_t *item;
	char skin[MAX_QPATH];
	char model[MAX_QPATH];
	char head[MAX_QPATH];
	char torso[MAX_QPATH];
	char legs[MAX_QPATH];

	menu = Menu_GetFocused();	// Get current menu

	if (!menu)
	{
		return;
	}

	item = (itemDef_t *) Menu_FindItemByName(menu, "character");

	if (!item)
	{
		Com_Error( ERR_FATAL, "UI_UpdateCharacterSkin: Could not find item (character) in menu (%s)", menu->window.name);
	}

	trap->Cvar_VariableStringBuffer("ui_char_model", model, sizeof(model));
	trap->Cvar_VariableStringBuffer("ui_char_skin_head", head, sizeof(head));
	trap->Cvar_VariableStringBuffer("ui_char_skin_torso", torso, sizeof(torso));
	trap->Cvar_VariableStringBuffer("ui_char_skin_legs", legs, sizeof(legs));

	Com_sprintf( skin, sizeof( skin ), "models/players/%s/|%s|%s|%s",
										model,
										head,
										torso,
										legs
				);

	ItemParse_model_g2skin_go( item, skin );
}

static void UI_ResetCharacterListBoxes( void )
{

	itemDef_t *item;
	menuDef_t *menu;
	listBoxDef_t *listPtr;

	menu = Menu_GetFocused();

	if (menu)
	{
		item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "headlistbox");
		if (item)
		{
			listPtr = item->typeData.listbox;
			if( listPtr )
			{
				listPtr->cursorPos = 0;
			}
			item->cursorPos = 0;
		}

		item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "torsolistbox");
		if (item)
		{
			listPtr = item->typeData.listbox;
			if( listPtr )
			{
				listPtr->cursorPos = 0;
			}
			item->cursorPos = 0;
		}

		item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "lowerlistbox");
		if (item)
		{
			listPtr = item->typeData.listbox;
			if( listPtr )
			{
				listPtr->cursorPos = 0;
			}
			item->cursorPos = 0;
		}

		item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "colorbox");
		if (item)
		{
			listPtr = item->typeData.listbox;
			if( listPtr )
			{
				listPtr->cursorPos = 0;
			}
			item->cursorPos = 0;
		}
	}
}

const char *saberSingleHiltInfo [MAX_SABER_HILTS];
const char *saberStaffHiltInfo [MAX_SABER_HILTS];

qboolean UI_SaberProperNameForSaber( const char *saberName, char *saberProperName );
void WP_SaberGetHiltInfo( const char *singleHilts[MAX_SABER_HILTS], const char *staffHilts[MAX_SABER_HILTS] );

static void UI_UpdateCharacter( qboolean changedModel )
{
	menuDef_t *menu;
	itemDef_t *item;
	char modelPath[MAX_QPATH];
	int	animRunLength;

	menu = Menu_GetFocused();	// Get current menu

	if (!menu)
	{
		return;
	}

	item = (itemDef_t *) Menu_FindItemByName(menu, "character");

	if (!item)
	{
		Com_Error( ERR_FATAL, "UI_UpdateCharacter: Could not find item (character) in menu (%s)", menu->window.name);
	}

	ItemParse_model_g2anim_go( item, ui_char_anim.string );

	Com_sprintf( modelPath, sizeof( modelPath ), "models/players/%s/model.glm", UI_Cvar_VariableString ( "ui_char_model" ) );
	ItemParse_asset_model_go( item, modelPath, &animRunLength );

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

/*
==================
UI_CheckServerName
==================
*/
static void UI_CheckServerName( void )
{
	qboolean	changed = qfalse;

	char hostname[MAX_HOSTNAMELENGTH] = {0};
	char *c = hostname;

	trap->Cvar_VariableStringBuffer( "sv_hostname", hostname, sizeof( hostname ) );

	while( *c )
	{
		if ( (*c == '\\') || (*c == ';') || (*c == '"'))
		{
			*c = '.';
			changed = qtrue;
		}
		c++;
	}
	if( changed )
	{
		trap->Cvar_Set("sv_hostname", hostname );
	}

}

/*
==================
UI_CheckPassword
==================
*/
static qboolean UI_CheckPassword( void )
{
	static char info[MAX_STRING_CHARS];

	int index = uiInfo.serverStatus.currentServer;
	if( (index < 0) || (index >= uiInfo.serverStatus.numDisplayServers) )
	{	// warning?
		return qfalse;
	}

	trap->LAN_GetServerInfo(UI_SourceForLAN(), uiInfo.serverStatus.displayServers[index], info, MAX_STRING_CHARS);

	if ( atoi(Info_ValueForKey(info, "needpass")) )
	{
		Menus_OpenByName("password_request");
		return qfalse;

	}

	// This isn't going to make it (too late in dev), like James said I should check to see when we receive
	// a packet *if* we do indeed get a 0 ping just make it 1 so then a 0 ping is guaranteed to be bad
	/*
	// also check ping!
    ping = atoi(Info_ValueForKey(info, "ping"));
	// NOTE : PING -- it's very questionable as to whether a ping of < 0 or <= 0 indicates a bad server
	// what I do know, is that getting "ping" from the ServerInfo on a bad server returns 0.
	// So I'm left with no choice but to not allow you to enter a server with a ping of 0
	if( ping <= 0 )
	{
		Menus_OpenByName("bad_server");
		return qfalse;
	}
	*/

	return qtrue;
}

/*
==================
UI_JoinServer
==================
*/
static void UI_JoinServer( void )
{
	char buff[1024] = {0};

//	trap->Cvar_Set("cg_thirdPerson", "0");
	trap->Cvar_Set("cg_cameraOrbit", "0");
	trap->Cvar_Set("ui_singlePlayerActive", "0");
	if (uiInfo.serverStatus.currentServer >= 0 && uiInfo.serverStatus.currentServer < uiInfo.serverStatus.numDisplayServers)
	{
		trap->LAN_GetServerAddressString(UI_SourceForLAN()/*ui_netSource.integer*/, uiInfo.serverStatus.displayServers[uiInfo.serverStatus.currentServer], buff, sizeof( buff ) );
		trap->Cmd_ExecuteText( EXEC_APPEND, va( "connect %s\n", buff ) );
	}

}

int UI_SiegeClassNum( siegeClass_t *scl ) {
	int i=0;
	for ( i=0; i<bgNumSiegeClasses; i++ ) {
		if ( &bgSiegeClasses[i] == scl )
			return i;
	}

	return 0;
}

//called every time a class is selected from a feeder, sets info for shaders to be displayed in the menu about the class -rww
void UI_SiegeSetCvarsForClass( siegeClass_t *scl ) {
	int i = 0;
	int count = 0;
	char shader[MAX_QPATH];

	//let's clear the things out first
	while (i < WP_NUM_WEAPONS)
	{
		trap->Cvar_Set(va("ui_class_weapon%i", i), "gfx/2d/select");
		i++;
	}
	//now for inventory items
	i = 0;
	while (i < HI_NUM_HOLDABLE)
	{
		trap->Cvar_Set(va("ui_class_item%i", i), "gfx/2d/select");
		i++;
	}
	//now for force powers
	i = 0;
	while (i < NUM_FORCE_POWERS)
	{
		trap->Cvar_Set(va("ui_class_power%i", i), "gfx/2d/select");
		i++;
	}

	//now health and armor
	trap->Cvar_Set("ui_class_health", "0");
	trap->Cvar_Set("ui_class_armor", "0");

	trap->Cvar_Set("ui_class_icon", "");

	if (!scl)
	{ //no select?
		return;
	}

	//set cvars for which weaps we have
	i = 0;
	trap->Cvar_Set(va("ui_class_weapondesc%i", count), " ");	// Blank it out to start with
	while (i < WP_NUM_WEAPONS)
	{

		if (scl->weapons & (1<<i))
		{
			if (i == WP_SABER)
			{ //we want to see what kind of saber they have, and set the cvar based on that
				char saberType[1024];

				if (scl->saber1[0] &&
					scl->saber2[0])
				{
					Q_strncpyz(saberType, "gfx/hud/w_icon_duallightsaber", sizeof( saberType ) );
				} //fixme: need saber data access on ui to determine if staff, "gfx/hud/w_icon_saberstaff"
				else
				{
					char buf[1024];
					if (scl->saber1[0] && UI_SaberTypeForSaber(scl->saber1, buf))
					{
						if ( !Q_stricmp( buf, "SABER_STAFF" ) )
						{
							Q_strncpyz(saberType,"gfx/hud/w_icon_saberstaff", sizeof( saberType ) );
						}
						else
						{
							Q_strncpyz(saberType,"gfx/hud/w_icon_lightsaber", sizeof( saberType ) );
						}
					}
					else
					{
						Q_strncpyz(saberType,"gfx/hud/w_icon_lightsaber", sizeof( saberType ) );
					}
				}

				trap->Cvar_Set(va("ui_class_weapon%i", count), saberType);
				trap->Cvar_Set(va("ui_class_weapondesc%i", count), "@MENUS_AN_ELEGANT_WEAPON_FOR");
				count++;
				trap->Cvar_Set(va("ui_class_weapondesc%i", count), " ");	// Blank it out to start with
			}
			else
			{
				gitem_t *item = BG_FindItemForWeapon( i );
				trap->Cvar_Set(va("ui_class_weapon%i", count), item->icon);
				trap->Cvar_Set(va("ui_class_weapondesc%i", count), item->description);
				count++;
				trap->Cvar_Set(va("ui_class_weapondesc%i", count), " ");	// Blank it out to start with
			}
		}

		i++;
	}

	//now for inventory items
	i = 0;
	count = 0;

	while (i < HI_NUM_HOLDABLE)
	{
		if (scl->invenItems & (1<<i))
		{
			gitem_t *item = BG_FindItemForHoldable(i);
			trap->Cvar_Set(va("ui_class_item%i", count), item->icon);
			trap->Cvar_Set(va("ui_class_itemdesc%i", count), item->description);
			count++;
		}
		else
		{
			trap->Cvar_Set(va("ui_class_itemdesc%i", count), " ");
		}
		i++;
	}

	//now for force powers
	i = 0;
	count = 0;

	while (i < NUM_FORCE_POWERS)
	{
		trap->Cvar_Set(va("ui_class_powerlevel%i", i), "0");	// Zero this out to start.
		if (i<9)
		{
			trap->Cvar_Set(va("ui_class_powerlevelslot%i", i), "0");	// Zero this out to start.
		}

		if (scl->forcePowerLevels[i])
		{
			trap->Cvar_Set(va("ui_class_powerlevel%i", count), va("%i",scl->forcePowerLevels[i]));
			trap->Cvar_Set(va("ui_class_power%i", count), HolocronIcons[i]);
			count++;
		}

		i++;
	}

	//now health and armor
	trap->Cvar_Set("ui_class_health", va("%i", scl->maxhealth));
	trap->Cvar_Set("ui_class_armor", va("%i", scl->maxarmor));
	trap->Cvar_Set("ui_class_speed", va("%3.2f", scl->speed));

	//now get the icon path based on the shader index
	if (scl->classShader)
	{
		trap->R_ShaderNameFromIndex(shader, scl->classShader);
	}
	else
	{ //no shader
		shader[0] = 0;
	}
	trap->Cvar_Set("ui_class_icon", shader);
}

static int g_siegedFeederForcedSet = 0;
void UI_UpdateCvarsForClass(const int team,const int baseClass,const int index)
{
	siegeClass_t *holdClass=0;
	char *holdBuf;

	// Is it a valid team
	if ((team == SIEGETEAM_TEAM1) ||
		(team == SIEGETEAM_TEAM2))
	{

		// Is it a valid base class?
		if ((baseClass >= SPC_INFANTRY) && (baseClass < SPC_MAX))
		{
			// A valid index?
			if ((index>=0) && (index < BG_SiegeCountBaseClass( team, baseClass )))
			{
				if (!g_siegedFeederForcedSet)
				{
					holdClass = BG_GetClassOnBaseClass( team, baseClass, index);
					if (holdClass)	//clicked a valid item
					{
						g_UIGloballySelectedSiegeClass = UI_SiegeClassNum(holdClass);
						trap->Cvar_Set("ui_classDesc", g_UIClassDescriptions[g_UIGloballySelectedSiegeClass].desc);
						g_siegedFeederForcedSet = 1;
						Menu_SetFeederSelection(NULL, FEEDER_SIEGE_BASE_CLASS, -1, NULL);
						UI_SiegeSetCvarsForClass(holdClass);

						holdBuf = BG_GetUIPortraitFile(team, baseClass, index);
						if (holdBuf)
						{
							trap->Cvar_Set("ui_classPortrait",holdBuf);
						}
					}
				}
				g_siegedFeederForcedSet = 0;
			}
			else
			{
				trap->Cvar_Set("ui_classDesc", " ");
			}
		}
	}

}

void UI_ClampMaxPlayers( void ) {
	// duel requires 2 players
	if ( uiInfo.gameTypes[ui_netGametype.integer].gtEnum == GT_DUEL ) {
		if ( (int)trap->Cvar_VariableValue( "sv_maxClients" ) < 2 )
			trap->Cvar_Set( "sv_maxClients", "2" );
	}

	// power duel requires 3 players
	else if ( uiInfo.gameTypes[ui_netGametype.integer].gtEnum == GT_POWERDUEL ) {
		if ( (int)trap->Cvar_VariableValue( "sv_maxClients" ) < 3 )
			trap->Cvar_Set( "sv_maxClients", "3" );
	}

	// can never exceed MAX_CLIENTS
	if ( (int)trap->Cvar_VariableValue( "sv_maxClients" ) > MAX_CLIENTS ) {
		trap->Cvar_Set( "sv_maxClients", XSTRING(MAX_CLIENTS) );
	}
}

void UI_UpdateSiegeStatusIcons( void ) {
    menuDef_t *menu = Menu_GetFocused();

	if ( menu ) {
		int i=0;
		for ( i= 0; i< 7; i++ )		Menu_SetItemBackground( menu, va( "wpnicon0%d", i ),	va( "*ui_class_weapon%d", i ) );
		for ( i= 0; i< 7; i++ )		Menu_SetItemBackground( menu, va( "itemicon0%d", i ),	va( "*ui_class_item%d", i ) );
		for ( i= 0; i<10; i++ )		Menu_SetItemBackground( menu, va( "forceicon0%d", i ),	va( "*ui_class_power%d", i ) );
		for ( i=10; i<15; i++ )		Menu_SetItemBackground( menu, va( "forceicon%d", i ),	va( "*ui_class_power%d", i ) );
	}
}

static void UI_RunMenuScript(char **args)
{
	const char *name, *name2;
	char buff[1024];

	if (String_Parse(args, &name))
	{
		if (Q_stricmp(name, "StartServer") == 0)
		{
			int i, added = 0;
			float skill;
			int warmupTime = 0;
			int doWarmup = 0;

			//	trap->Cvar_Set("cg_thirdPerson", "0");
			trap->Cvar_Set("cg_cameraOrbit", "0");
			// for Solo games I set this to 1 in the menu and don't want it stomped here,
			// this cvar seems to be reset to 0 in all the proper places so... -dmv
			//	trap->Cvar_Set("ui_singlePlayerActive", "0");

			// if a solo game is started, automatically turn dedicated off here (don't want to do it in the menu, might get annoying)
			if( trap->Cvar_VariableValue( "ui_singlePlayerActive" ) )
			{
				trap->Cvar_Set( "dedicated", "0" );
			}
			else
			{
				trap->Cvar_SetValue( "dedicated", Com_Clamp( 0, 2, ui_dedicated.integer ) );
			}
			trap->Cvar_SetValue( "g_gametype", Com_Clamp( 0, GT_MAX_GAME_TYPE, uiInfo.gameTypes[ui_netGametype.integer].gtEnum ) );
			//trap->Cvar_Set("g_redTeam", UI_Cvar_VariableString("ui_teamName"));
			//trap->Cvar_Set("g_blueTeam", UI_Cvar_VariableString("ui_opponentName"));
			trap->Cmd_ExecuteText( EXEC_APPEND, va( "wait ; wait ; map %s\n", uiInfo.mapList[ui_currentNetMap.integer].mapLoadName ) );
			skill = trap->Cvar_VariableValue( "g_spSkill" );

			//Cap the warmup values in case the user tries a dumb setting.
			warmupTime = trap->Cvar_VariableValue( "g_warmup" );
			doWarmup = trap->Cvar_VariableValue( "g_doWarmup" );

			if (doWarmup && warmupTime < 1)
			{
				trap->Cvar_Set("g_doWarmup", "0");
			}
			if (warmupTime < 5)
			{
				trap->Cvar_Set("g_warmup", "5");
			}
			if (warmupTime > 120)
			{
				trap->Cvar_Set("g_warmup", "120");
			}

			if (trap->Cvar_VariableValue( "g_gametype" ) == GT_DUEL ||
				trap->Cvar_VariableValue( "g_gametype" ) == GT_POWERDUEL)
			{ //always set fraglimit 1 when starting a duel game
				trap->Cvar_Set("fraglimit", "1");
				trap->Cvar_Set("timelimit", "0");
			}

			for (i = 0; i < PLAYERS_PER_TEAM; i++)
			{
				int bot = trap->Cvar_VariableValue( va("ui_blueteam%i", i+1));
				int maxcl = trap->Cvar_VariableValue( "sv_maxClients" );

				if (bot > 1)
				{
					int numval = i+1;

					numval *= 2;

					numval -= 1;

					if (numval <= maxcl)
					{
						if (ui_actualNetGametype.integer >= GT_TEAM) {
							Com_sprintf( buff, sizeof(buff), "addbot \"%s\" %f %s\n", UI_GetBotNameByNumber(bot-2), skill, "Blue");
						} else {
							Com_sprintf( buff, sizeof(buff), "addbot \"%s\" %f \n", UI_GetBotNameByNumber(bot-2), skill);
						}
						trap->Cmd_ExecuteText( EXEC_APPEND, buff );
						added++;
					}
				}
				bot = trap->Cvar_VariableValue( va("ui_redteam%i", i+1));
				if (bot > 1) {
					int numval = i+1;

					numval *= 2;

					if (numval <= maxcl)
					{
						if (ui_actualNetGametype.integer >= GT_TEAM) {
							Com_sprintf( buff, sizeof(buff), "addbot \"%s\" %f %s\n", UI_GetBotNameByNumber(bot-2), skill, "Red");
						} else {
							Com_sprintf( buff, sizeof(buff), "addbot \"%s\" %f \n", UI_GetBotNameByNumber(bot-2), skill);
						}
						trap->Cmd_ExecuteText( EXEC_APPEND, buff );
						added++;
					}
				}
				if (added >= maxcl)
				{ //this means the client filled up all their slots in the UI with bots. So stretch out an extra slot for them, and then stop adding bots.
					trap->Cvar_Set("sv_maxClients", va("%i", added+1));
					break;
				}
			}
		} else if (Q_stricmp(name, "updateSPMenu") == 0) {
			UI_SetCapFragLimits(qtrue);
			UI_MapCountByGameType(qtrue);
			trap->Cvar_SetValue("ui_mapIndex", UI_GetIndexFromSelection(ui_currentMap.integer));
			trap->Cvar_Update(&ui_mapIndex);
			Menu_SetFeederSelection(NULL, FEEDER_MAPS, ui_mapIndex.integer, "skirmish");
			UI_GameType_HandleKey(0, 0, A_MOUSE1, qfalse);
			UI_GameType_HandleKey(0, 0, A_MOUSE2, qfalse);
		} else if (Q_stricmp(name, "resetDefaults") == 0) {
			trap->Cmd_ExecuteText( EXEC_APPEND, "cvar_restart\n");
			Controls_SetDefaults();
			trap->Cmd_ExecuteText( EXEC_APPEND, "exec mpdefault.cfg\n");
			trap->Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );

		} else if (Q_stricmp(name, "loadArenas") == 0) {
			UI_LoadArenas();
			UI_MapCountByGameType(qfalse);
			Menu_SetFeederSelection(NULL, FEEDER_ALLMAPS, gUISelectedMap, "createserver");
			uiForceRank = trap->Cvar_VariableValue("g_maxForceRank");
		} else if (Q_stricmp(name, "saveControls") == 0) {
			Controls_SetConfig();
		} else if (Q_stricmp(name, "loadControls") == 0) {
			Controls_GetConfig();
		} else if (Q_stricmp(name, "clearError") == 0) {
			trap->Cvar_Set("com_errorMessage", "");
		} else if (Q_stricmp(name, "loadGameInfo") == 0) {
			UI_ParseGameInfo("ui/jamp/gameinfo.txt");
		} else if (Q_stricmp(name, "RefreshServers") == 0) {
			UI_StartServerRefresh(qtrue);
			UI_BuildServerDisplayList(qtrue);
		} else if (Q_stricmp(name, "RefreshFilter") == 0) {
			UI_StartServerRefresh(qfalse);
			UI_BuildServerDisplayList(qtrue);
		} else if (Q_stricmp(name, "LoadDemos") == 0) {
			UI_LoadDemos();
		} else if (Q_stricmp(name, "LoadMovies") == 0) {
			UI_LoadMovies();
		} else if (Q_stricmp(name, "LoadMods") == 0) {
			UI_LoadMods();
		} else if (Q_stricmp(name, "playMovie") == 0) {
			if (uiInfo.previewMovie >= 0) {
				trap->CIN_StopCinematic(uiInfo.previewMovie);
			}
			trap->Cmd_ExecuteText( EXEC_APPEND, va("cinematic %s.roq 2\n", uiInfo.movieList[uiInfo.movieIndex]));
		} else if (Q_stricmp(name, "RunMod") == 0) {
			trap->Cvar_Set( "fs_game", uiInfo.modList[uiInfo.modIndex].modName);
			trap->Cmd_ExecuteText( EXEC_APPEND, "vid_restart;" );
		} else if (Q_stricmp(name, "RunDemo") == 0) {
			trap->Cmd_ExecuteText( EXEC_APPEND, va("demo \"%s\"\n", uiInfo.demoList[uiInfo.demoIndex]));
		} else if (Q_stricmp(name, "Quake3") == 0) {
			trap->Cvar_Set( "fs_game", "");
			trap->Cmd_ExecuteText( EXEC_APPEND, "vid_restart;" );
		} else if (Q_stricmp(name, "closeJoin") == 0) {
			if (uiInfo.serverStatus.refreshActive) {
				UI_StopServerRefresh();
				uiInfo.serverStatus.nextDisplayRefresh = 0;
				uiInfo.nextServerStatusRefresh = 0;
				uiInfo.nextFindPlayerRefresh = 0;
				UI_BuildServerDisplayList(qtrue);
			} else {
				Menus_CloseByName("joinserver");
				Menus_OpenByName("main");
			}
		} else if (Q_stricmp(name, "StopRefresh") == 0) {
			UI_StopServerRefresh();
			uiInfo.serverStatus.nextDisplayRefresh = 0;
			uiInfo.nextServerStatusRefresh = 0;
			uiInfo.nextFindPlayerRefresh = 0;
		} else if (Q_stricmp(name, "UpdateFilter") == 0) {
			trap->Cvar_Update( &ui_netSource );
			if (ui_netSource.integer == UIAS_LOCAL || !uiInfo.serverStatus.numDisplayServers) {
				UI_StartServerRefresh(qtrue);
			}
			UI_BuildServerDisplayList(qtrue);
			UI_FeederSelection(FEEDER_SERVERS, 0, NULL );

			UI_LoadMods();
		} else if (Q_stricmp(name, "ServerStatus") == 0) {
			trap->LAN_GetServerAddressString(UI_SourceForLAN(), uiInfo.serverStatus.displayServers[uiInfo.serverStatus.currentServer], uiInfo.serverStatusAddress, sizeof(uiInfo.serverStatusAddress));
			UI_BuildServerStatus(qtrue);
		} else if (Q_stricmp(name, "FoundPlayerServerStatus") == 0) {
			Q_strncpyz(uiInfo.serverStatusAddress, uiInfo.foundPlayerServerAddresses[uiInfo.currentFoundPlayerServer], sizeof(uiInfo.serverStatusAddress));
			UI_BuildServerStatus(qtrue);
			Menu_SetFeederSelection(NULL, FEEDER_FINDPLAYER, 0, NULL);
		} else if (Q_stricmp(name, "FindPlayer") == 0) {
			UI_BuildFindPlayerList(qtrue);
			// clear the displayed server status info
			uiInfo.serverStatusInfo.numLines = 0;
			Menu_SetFeederSelection(NULL, FEEDER_FINDPLAYER, 0, NULL);
		}
		else if (Q_stricmp(name, "checkservername") == 0)
		{
			UI_CheckServerName();
		}
		else if (Q_stricmp(name, "checkpassword") == 0)
		{
			if( UI_CheckPassword() )
			{
				UI_JoinServer();
			}
		}
		else if (Q_stricmp(name, "JoinServer") == 0)
		{
			UI_JoinServer();
		}
		else if (Q_stricmp(name, "FoundPlayerJoinServer") == 0) {
			trap->Cvar_Set("ui_singlePlayerActive", "0");
			if (uiInfo.currentFoundPlayerServer >= 0 && uiInfo.currentFoundPlayerServer < uiInfo.numFoundPlayerServers) {
				trap->Cmd_ExecuteText( EXEC_APPEND, va( "connect %s\n", uiInfo.foundPlayerServerAddresses[uiInfo.currentFoundPlayerServer] ) );
			}
		} else if (Q_stricmp(name, "Quit") == 0) {
			trap->Cvar_Set("ui_singlePlayerActive", "0");
			trap->Cmd_ExecuteText( EXEC_NOW, "quit");
		} else if (Q_stricmp(name, "Controls") == 0) {
			trap->Cvar_Set( "cl_paused", "1" );
			trap->Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName("setup_menu2");
		}
		else if (Q_stricmp(name, "Leave") == 0)
		{
			trap->Cmd_ExecuteText( EXEC_APPEND, "disconnect\n" );
			trap->Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName("main");
		}
		else if (Q_stricmp(name, "getvideosetup") == 0)
		{
			UI_GetVideoSetup ( );
		}
		else if (Q_stricmp(name, "getsaberhiltinfo") == 0)
		{
			WP_SaberGetHiltInfo(saberSingleHiltInfo, saberStaffHiltInfo);
		}
		// On the solo game creation screen, we can't see siege maps
		else if (Q_stricmp(name, "checkforsiege") == 0)
		{
			if (uiInfo.gameTypes[ui_netGametype.integer].gtEnum == GT_SIEGE)
			{
				// fake out the handler to advance to the next game type
				UI_NetGameType_HandleKey(0, NULL, A_MOUSE1);
			}
		}
		else if (Q_stricmp(name, "updatevideosetup") == 0)
		{
			UI_UpdateVideoSetup ( );
		}
		else if (Q_stricmp(name, "ServerSort") == 0)
		{
			int sortColumn;
			if (Int_Parse(args, &sortColumn)) {
				// if same column we're already sorting on then flip the direction
				if (sortColumn == uiInfo.serverStatus.sortKey) {
					uiInfo.serverStatus.sortDir = !uiInfo.serverStatus.sortDir;
				}
				// make sure we sort again
				UI_ServersSort(sortColumn, qtrue);
			}
		} else if (Q_stricmp(name, "nextSkirmish") == 0) {
			UI_StartSkirmish(qtrue);
		} else if (Q_stricmp(name, "SkirmishStart") == 0) {
			UI_StartSkirmish(qfalse);
		} else if (Q_stricmp(name, "closeingame") == 0) {
			trap->Key_SetCatcher( trap->Key_GetCatcher() & ~KEYCATCH_UI );
			trap->Key_ClearStates();
			trap->Cvar_Set( "cl_paused", "0" );
			Menus_CloseAll();
		} else if (Q_stricmp(name, "voteMap") == 0) {
			if (ui_currentNetMap.integer >=0 && ui_currentNetMap.integer < uiInfo.mapCount) {
				trap->Cmd_ExecuteText( EXEC_APPEND, va("callvote map %s\n",uiInfo.mapList[ui_currentNetMap.integer].mapLoadName) );
			}
		} else if (Q_stricmp(name, "voteKick") == 0) {
			if (uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount) {
				//trap->Cmd_ExecuteText( EXEC_APPEND, va("callvote kick \"%s\"\n",uiInfo.playerNames[uiInfo.playerIndex]) );
				trap->Cmd_ExecuteText( EXEC_APPEND, va("callvote clientkick \"%i\"\n",uiInfo.playerIndexes[uiInfo.playerIndex]) );
			}
		} else if (Q_stricmp(name, "voteGame") == 0) {
			if (ui_netGametype.integer >= 0 && ui_netGametype.integer < uiInfo.numGameTypes) {
				trap->Cmd_ExecuteText( EXEC_APPEND, va("callvote g_gametype %i\n",uiInfo.gameTypes[ui_netGametype.integer].gtEnum) );
			}
		} else if (Q_stricmp(name, "voteLeader") == 0) {
			if (uiInfo.teamIndex >= 0 && uiInfo.teamIndex < uiInfo.myTeamCount) {
				trap->Cmd_ExecuteText( EXEC_APPEND, va("callteamvote leader \"%s\"\n",uiInfo.teamNames[uiInfo.teamIndex]) );
			}
		} else if (Q_stricmp(name, "addBot") == 0) {
			if (trap->Cvar_VariableValue("g_gametype") >= GT_TEAM) {
				trap->Cmd_ExecuteText( EXEC_APPEND, va("addbot \"%s\" %i %s\n", UI_GetBotNameByNumber(uiInfo.botIndex), uiInfo.skillIndex+1, (uiInfo.redBlue == 0) ? "Red" : "Blue") );
			} else {
				trap->Cmd_ExecuteText( EXEC_APPEND, va("addbot \"%s\" %i %s\n", UI_GetBotNameByNumber(uiInfo.botIndex), uiInfo.skillIndex+1, (uiInfo.redBlue == 0) ? "Red" : "Blue") );
			}
		} else if (Q_stricmp(name, "addFavorite") == 0)
		{
			if (ui_netSource.integer != UIAS_FAVORITES)
			{
				char name[MAX_HOSTNAMELENGTH] = {0};
				char addr[MAX_ADDRESSLENGTH] = {0};
				int res;

				trap->LAN_GetServerInfo(UI_SourceForLAN(), uiInfo.serverStatus.displayServers[uiInfo.serverStatus.currentServer], buff, MAX_STRING_CHARS);
				name[0] = addr[0] = '\0';
				Q_strncpyz(name, 	Info_ValueForKey(buff, "hostname"), sizeof( name ) );
				Q_strncpyz(addr, 	Info_ValueForKey(buff, "addr"), sizeof( addr ) );
				if (strlen(name) > 0 && strlen(addr) > 0)
				{
					res = trap->LAN_AddServer(AS_FAVORITES, name, addr);
					if (res == 0)
					{
						// server already in the list
						Com_Printf("Favorite already in list\n");
					}
					else if (res == -1)
					{
						// list full
						Com_Printf("Favorite list full\n");
					}
					else
					{
						// successfully added
						Com_Printf("Added favorite server %s\n", addr);
					}
				}
			}
		}
		else if (Q_stricmp(name, "deleteFavorite") == 0)
		{
			if (ui_netSource.integer == UIAS_FAVORITES)
			{
				char addr[MAX_ADDRESSLENGTH] = {0};
				trap->LAN_GetServerInfo(AS_FAVORITES, uiInfo.serverStatus.displayServers[uiInfo.serverStatus.currentServer], buff, MAX_STRING_CHARS);
				addr[0] = '\0';
				Q_strncpyz(addr, 	Info_ValueForKey(buff, "addr"), sizeof( addr ) );
				if (strlen(addr) > 0)
				{
					trap->LAN_RemoveServer(AS_FAVORITES, addr);
				}
			}
		}
		else if (Q_stricmp(name, "createFavorite") == 0)
		{
			//	if (ui_netSource.integer == UIAS_FAVORITES)
			//rww - don't know why this check was here.. why would you want to only add new favorites when the filter was favorites?
			{
				char name[MAX_HOSTNAMELENGTH] = {0};
				char addr[MAX_ADDRESSLENGTH] = {0};
				int res;

				name[0] = addr[0] = '\0';
				Q_strncpyz(name, 	UI_Cvar_VariableString("ui_favoriteName"), sizeof( name ) );
				Q_strncpyz(addr, 	UI_Cvar_VariableString("ui_favoriteAddress"), sizeof( addr ) );
				if (/*strlen(name) > 0 &&*/ strlen(addr) > 0) {
					res = trap->LAN_AddServer(AS_FAVORITES, name, addr);
					if (res == 0) {
						// server already in the list
						Com_Printf("Favorite already in list\n");
					}
					else if (res == -1) {
						// list full
						Com_Printf("Favorite list full\n");
					}
					else {
						// successfully added
						Com_Printf("Added favorite server %s\n", addr);
					}
				}
			}
		} else if (Q_stricmp(name, "orders") == 0) {
			const char *orders;
			if (String_Parse(args, &orders)) {
				int selectedPlayer = trap->Cvar_VariableValue("cg_selectedPlayer");
				if (selectedPlayer < uiInfo.myTeamCount) {
					Q_strncpyz( buff, orders, sizeof( buff ) );
					trap->Cmd_ExecuteText( EXEC_APPEND, va(buff, uiInfo.teamClientNums[selectedPlayer]) );
					trap->Cmd_ExecuteText( EXEC_APPEND, "\n" );
				} else {
					int i;
					for (i = 0; i < uiInfo.myTeamCount; i++) {
						if (uiInfo.playerNumber == uiInfo.teamClientNums[i]) {
							continue;
						}
						Com_sprintf( buff, sizeof( buff ), orders, uiInfo.teamClientNums[i] );
						trap->Cmd_ExecuteText( EXEC_APPEND, buff );
						trap->Cmd_ExecuteText( EXEC_APPEND, "\n" );
					}
				}
				trap->Key_SetCatcher( trap->Key_GetCatcher() & ~KEYCATCH_UI );
				trap->Key_ClearStates();
				trap->Cvar_Set( "cl_paused", "0" );
				Menus_CloseAll();
			}
		} else if (Q_stricmp(name, "voiceOrdersTeam") == 0) {
			const char *orders;
			if (String_Parse(args, &orders)) {
				int selectedPlayer = trap->Cvar_VariableValue("cg_selectedPlayer");
				if (selectedPlayer == uiInfo.myTeamCount) {
					trap->Cmd_ExecuteText( EXEC_APPEND, orders );
					trap->Cmd_ExecuteText( EXEC_APPEND, "\n" );
				}
				trap->Key_SetCatcher( trap->Key_GetCatcher() & ~KEYCATCH_UI );
				trap->Key_ClearStates();
				trap->Cvar_Set( "cl_paused", "0" );
				Menus_CloseAll();
			}
		} else if (Q_stricmp(name, "voiceOrders") == 0) {
			const char *orders;
			if (String_Parse(args, &orders)) {
				int selectedPlayer = trap->Cvar_VariableValue("cg_selectedPlayer");

				if (selectedPlayer == uiInfo.myTeamCount)
				{
					selectedPlayer = -1;
					Q_strncpyz( buff, orders, sizeof( buff ) );
					trap->Cmd_ExecuteText( EXEC_APPEND, va(buff, selectedPlayer) );
				}
				else
				{
					Q_strncpyz( buff, orders, sizeof( buff ) );
					trap->Cmd_ExecuteText( EXEC_APPEND, va(buff, uiInfo.teamClientNums[selectedPlayer]) );
				}
				trap->Cmd_ExecuteText( EXEC_APPEND, "\n" );

				trap->Key_SetCatcher( trap->Key_GetCatcher() & ~KEYCATCH_UI );
				trap->Key_ClearStates();
				trap->Cvar_Set( "cl_paused", "0" );
				Menus_CloseAll();
			}
		}
		else if (Q_stricmp(name, "setForce") == 0)
		{
			const char *teamArg;

			if (String_Parse(args, &teamArg))
			{
				if ( Q_stricmp( "none", teamArg ) == 0 )
				{
					UI_UpdateClientForcePowers(NULL);
				}
				else if ( Q_stricmp( "same", teamArg ) == 0 )
				{//stay on current team
					int myTeam = (int)(trap->Cvar_VariableValue("ui_myteam"));
					if ( myTeam != TEAM_SPECTATOR )
					{
						UI_UpdateClientForcePowers(UI_TeamName(myTeam));//will cause him to respawn, if it's been 5 seconds since last one
					}
					else
					{
						UI_UpdateClientForcePowers(NULL);//just update powers
					}
				}
				else
				{
					UI_UpdateClientForcePowers(teamArg);
				}
			}
			else
			{
				UI_UpdateClientForcePowers(NULL);
			}
		}
		else if (Q_stricmp(name, "setsiegeclassandteam") == 0)
		{
			int team = (int)trap->Cvar_VariableValue("ui_holdteam");
			int oldteam = (int)trap->Cvar_VariableValue("ui_startsiegeteam");
			qboolean	goTeam = qtrue;
			char	newclassString[512];
			char	startclassString[512];

			trap->Cvar_VariableStringBuffer( "ui_mySiegeClass", newclassString, sizeof(newclassString) );
			trap->Cvar_VariableStringBuffer( "ui_startsiegeclass", startclassString, sizeof(startclassString) );

			// Was just a spectator - is still just a spectator
			if ((oldteam == team) && (oldteam == 3))
			{
				goTeam = qfalse;
			}
			// If new team and class match old team and class, just return to the game.
			else if (oldteam == team)
			{	// Classes match?
				if (g_UIGloballySelectedSiegeClass != -1)
				{
					if (!strcmp(startclassString,bgSiegeClasses[g_UIGloballySelectedSiegeClass].name))
					{
						goTeam = qfalse;
					}
				}
			}

			if (goTeam)
			{
				if (team == 1)	// Team red
				{
					trap->Cvar_Set("ui_team", va("%d", team));
				}
				else if (team == 2)	// Team blue
				{
					trap->Cvar_Set("ui_team", va("%d", team));
				}
				else if (team == 3)	// Team spectator
				{
					trap->Cvar_Set("ui_team", va("%d", team));
				}

				if (g_UIGloballySelectedSiegeClass != -1)
				{
					trap->Cmd_ExecuteText( EXEC_APPEND, va("siegeclass \"%s\"\n", bgSiegeClasses[g_UIGloballySelectedSiegeClass].name) );
				}
			}
		}
		else if (Q_stricmp(name, "setBotButton") == 0)
		{
			UI_SetBotButton();
		}
		else if (Q_stricmp(name, "saveTemplate") == 0) {
			UI_SaveForceTemplate();
		} else if (Q_stricmp(name, "refreshForce") == 0) {
			UI_UpdateForcePowers();
		} else if (Q_stricmp(name, "glCustom") == 0) {
			trap->Cvar_Set("ui_r_glCustom", "4");
		}
		else if (Q_stricmp(name, "setMovesListDefault") == 0)
		{
			uiInfo.movesTitleIndex = 2;
		}
		else if (Q_stricmp(name, "resetMovesList") == 0)
		{
			menuDef_t *menu;
			menu = Menus_FindByName("rulesMenu_moves");
			//update saber models
			if (menu)
			{
				itemDef_t *item  = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "character");
				if (item)
				{
					UI_SaberAttachToChar( item );
				}
			}

			trap->Cvar_Set( "ui_move_desc", " " );
		}
		else if (Q_stricmp(name, "resetcharacterlistboxes") == 0)
		{
			UI_ResetCharacterListBoxes();
		}
		else if (Q_stricmp(name, "setMoveCharacter") == 0)
		{
			itemDef_t *item;
			menuDef_t *menu;
			modelDef_t *modelPtr;
			int	animRunLength;

			UI_GetCharacterCvars();

			uiInfo.movesTitleIndex = 0;

			menu = Menus_FindByName("rulesMenu_moves");

			if (menu)
			{
				item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "character");
				if (item)
				{
					modelPtr = item->typeData.model;
					if (modelPtr)
					{
						char modelPath[MAX_QPATH];

						uiInfo.movesBaseAnim = datapadMoveTitleBaseAnims[uiInfo.movesTitleIndex];
						ItemParse_model_g2anim_go( item,  uiInfo.movesBaseAnim );
						uiInfo.moveAnimTime = 0 ;

						Com_sprintf( modelPath, sizeof( modelPath ), "models/players/%s/model.glm", UI_Cvar_VariableString ( "ui_char_model" ) );
						ItemParse_asset_model_go( item, modelPath, &animRunLength);

						UI_UpdateCharacterSkin();
						UI_SaberAttachToChar( item );
					}
				}
			}
		}
		else if (Q_stricmp(name, "character") == 0)
		{
			UI_UpdateCharacter( qfalse );
		}
		else if (Q_stricmp(name, "characterchanged") == 0)
		{
			UI_UpdateCharacter( qtrue );
		}
		else if (Q_stricmp(name, "updatecharcvars") == 0
			|| (Q_stricmp(name, "updatecharmodel") == 0) )
		{
			UI_UpdateCharacterCvars();
		}
		else if (Q_stricmp(name, "getcharcvars") == 0)
		{
			UI_GetCharacterCvars();
		}
		else if (Q_stricmp(name, "char_skin") == 0)
		{
			UI_UpdateCharacterSkin();
		}
#if 0
		else if (Q_stricmp(name, "setui_dualforcepower") == 0)
		{
			int forcePowerDisable = trap->Cvar_VariableValue("g_forcePowerDisable");
			int	i, forceBitFlag=0;

			// Turn off all powers but a few
			for (i=0;i<NUM_FORCE_POWERS;i++)
			{
				if ((i != FP_LEVITATION) &&
					(i != FP_PUSH) &&
					(i != FP_PULL) &&
					(i != FP_SABERTHROW) &&
					(i != FP_SABER_DEFENSE) &&
					(i != FP_SABER_OFFENSE))
				{
					forceBitFlag |= (1<<i);
				}
			}

			if (forcePowerDisable==0)
			{
				trap->Cvar_Set("ui_dualforcepower", "0");
			}
			else if (forcePowerDisable==forceBitFlag)
			{
				trap->Cvar_Set("ui_dualforcepower", "2");
			}
			else
			{
				trap->Cvar_Set("ui_dualforcepower", "1");
			}
		}
		else if (Q_stricmp(name, "dualForcePowers") == 0)
		{
			int	dualforcePower,i, forcePowerDisable;
			dualforcePower = trap->Cvar_VariableValue("ui_dualforcepower");

			if (dualforcePower==0)	// All force powers
			{
				forcePowerDisable = 0;
			}
			else if (dualforcePower==1)	// Remove All force powers
			{
				// It was set to something, so might as well make sure it got all flags set.
				for (i=0;i<NUM_FORCE_POWERS;i++)
				{
					forcePowerDisable |= (1<<i);
				}
			}
			else if (dualforcePower==2)	// Limited force powers
			{
				forcePowerDisable = 0;

				// Turn off all powers but a few
				for (i=0;i<NUM_FORCE_POWERS;i++)
				{
					if ((i != FP_LEVITATION) &&
						(i != FP_PUSH) &&
						(i != FP_PULL) &&
						(i != FP_SABERTHROW) &&
						(i != FP_SABER_DEFENSE) &&
						(i != FP_SABER_OFFENSE))
					{
						forcePowerDisable |= (1<<i);
					}
				}
			}

			trap->Cvar_Set("g_forcePowerDisable", va("%i",forcePowerDisable));
		}
		else if (Q_stricmp(name, "forcePowersDisable") == 0)
		{
			int	forcePowerDisable,i;

			forcePowerDisable = trap->Cvar_VariableValue("g_forcePowerDisable");

			// It was set to something, so might as well make sure it got all flags set.
			if (forcePowerDisable)
			{
				for (i=0;i<NUM_FORCE_POWERS;i++)
				{
					forcePowerDisable |= (1<<i);
				}

				trap->Cvar_Set("g_forcePowerDisable", va("%i",forcePowerDisable));
			}
		}
		else if (Q_stricmp(name, "weaponDisable") == 0)
		{
			int	weaponDisable,i;
			const char *cvarString;

			if (uiInfo.gameTypes[ui_netGameType.integer].gtEnum == GT_DUEL ||
				uiInfo.gameTypes[ui_netGameType.integer].gtEnum == GT_POWERDUEL)
			{
				cvarString = "g_duelWeaponDisable";
			}
			else
			{
				cvarString = "g_weaponDisable";
			}

			weaponDisable = trap->Cvar_VariableValue(cvarString);

			// It was set to something, so might as well make sure it got all flags set.
			if (weaponDisable)
			{
				for (i=0;i<WP_NUM_WEAPONS;i++)
				{
					if (i!=WP_SABER)
					{
						weaponDisable |= (1<<i);
					}
				}

				trap->Cvar_Set(cvarString, va("%i",weaponDisable));
			}
		}
#endif
		// If this is siege, change all the bots to humans, because we faked it earlier
		//  swapping humans for bots on the menu
		else if (Q_stricmp(name, "setSiegeNoBots") == 0)
		{
			int blueValue,redValue,i;

			if (uiInfo.gameTypes[ui_netGametype.integer].gtEnum == GT_SIEGE)
			{
				//hmm, I guess I'll set bot_minplayers to 0 here too. -rww
				trap->Cvar_Set("bot_minplayers", "0");

				for (i=1;i<9;i++)
				{
					blueValue = trap->Cvar_VariableValue(va("ui_blueteam%i",i ));
					if (blueValue>1)
					{
						trap->Cvar_Set(va("ui_blueteam%i",i ), "1");
					}

					redValue = trap->Cvar_VariableValue(va("ui_redteam%i",i ));
					if (redValue>1)
					{
						trap->Cvar_Set(va("ui_redteam%i",i ), "1");
					}

				}
			}
		}
		else if (Q_stricmp(name, "clearmouseover") == 0)
		{
			itemDef_t *item;
			menuDef_t *menu = Menu_GetFocused();

			if (menu)
			{
				int count,j;
				const char *itemName;
				String_Parse(args, &itemName);

				count = Menu_ItemsMatchingGroup(menu, itemName);

				for (j = 0; j < count; j++)
				{
					item = Menu_GetMatchingItemByNumber( menu, j, itemName);
					if (item != NULL)
					{
						item->window.flags &= ~WINDOW_MOUSEOVER;
					}
				}
			}
		}
		else if (Q_stricmp(name, "updateForceStatus") == 0)
		{
			UpdateForceStatus();
		}
		else if (Q_stricmp(name, "update") == 0)
		{
			if (String_Parse(args, &name2))
			{
				UI_Update(name2);
			}
		}
		else if (Q_stricmp(name, "setBotButtons") == 0)
		{
			UpdateBotButtons();
		}
		else if (Q_stricmp(name, "getsabercvars") == 0)
		{
			UI_GetSaberCvars();
		}
		else if (Q_stricmp(name, "setsaberboxesandhilts") == 0)
		{
			UI_SetSaberBoxesandHilts();
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
			UI_UpdateSaberColor( qfalse );
		}
		else if (Q_stricmp(name, "setscreensaberhilt") == 0)
		{
			menuDef_t *menu;
			itemDef_t *item;

			menu = Menu_GetFocused();	// Get current menu
			if (menu)
			{
				item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "hiltbut");
				if (item)
				{
					if (saberSingleHiltInfo[item->cursorPos])
					{
						trap->Cvar_Set( "ui_saber", saberSingleHiltInfo[item->cursorPos] );
					}
				}
			}
		}
		else if (Q_stricmp(name, "setscreensaberhilt1") == 0)
		{
			menuDef_t *menu;
			itemDef_t *item;

			menu = Menu_GetFocused();	// Get current menu
			if (menu)
			{
				item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "hiltbut1");
				if (item)
				{
					if (saberSingleHiltInfo[item->cursorPos])
					{
						trap->Cvar_Set( "ui_saber", saberSingleHiltInfo[item->cursorPos] );
					}
				}
			}
		}
		else if (Q_stricmp(name, "setscreensaberhilt2") == 0)
		{
			menuDef_t *menu;
			itemDef_t *item;

			menu = Menu_GetFocused();	// Get current menu
			if (menu)
			{
				item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "hiltbut2");
				if (item)
				{
					if (saberSingleHiltInfo[item->cursorPos])
					{
						trap->Cvar_Set( "ui_saber2", saberSingleHiltInfo[item->cursorPos] );
					}
				}
			}
		}
		else if (Q_stricmp(name, "setscreensaberstaff") == 0)
		{
			menuDef_t *menu;
			itemDef_t *item;

			menu = Menu_GetFocused();	// Get current menu
			if (menu)
			{
				item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "hiltbut_staves");
				if (item)
				{
					if (saberSingleHiltInfo[item->cursorPos])
					{
						trap->Cvar_Set( "ui_saber", saberStaffHiltInfo[item->cursorPos] );
					}
				}
			}
		}
		else if (Q_stricmp(name, "saber2_hilt") == 0)
		{
			UI_UpdateSaberHilt( qtrue );
		}
		else if (Q_stricmp(name, "saber2_color") == 0)
		{
			UI_UpdateSaberColor( qtrue );
		}
		else if (Q_stricmp(name, "updatesabercvars") == 0)
		{
			UI_UpdateSaberCvars();
		}
		else if (Q_stricmp(name, "updatesiegeobjgraphics") == 0)
		{
			int team = (int)trap->Cvar_VariableValue("ui_team");
			trap->Cvar_Set("ui_holdteam", va("%d", team));

			UI_UpdateSiegeObjectiveGraphics();
		}
		else if (Q_stricmp(name, "setsiegeobjbuttons") == 0)
		{
			const char *itemArg;
			const char *cvarLitArg;
			const char *cvarNormalArg;
			char	string[512];
			char	string2[512];
			menuDef_t *menu;
			itemDef_t *item;

			menu = Menu_GetFocused();	// Get current menu
			if (menu)
			{
				// Set the new item to the background
				if (String_Parse(args, &itemArg))
				{

					// Set the old button to it's original background
					trap->Cvar_VariableStringBuffer( "currentObjMapIconItem", string, sizeof(string) );
					item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, string);
					if (item)
					{
						// A cvar holding the name of a cvar - how crazy is that?
						trap->Cvar_VariableStringBuffer( "currentObjMapIconBackground", string, sizeof(string) );
						trap->Cvar_VariableStringBuffer( string, string2, sizeof(string2) );
						Menu_SetItemBackground(menu, item->window.name, string2);

						// Re-enable this button
						Menu_ItemDisable(menu, item->window.name, qfalse);
					}

					// Set the new item to the given background
					item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, itemArg);
					if (item)
					{	// store item name
						trap->Cvar_Set("currentObjMapIconItem",	 item->window.name);
						if (String_Parse(args, &cvarNormalArg))
						{	// Store normal background
							trap->Cvar_Set("currentObjMapIconBackground", cvarNormalArg);
							// Get higlight background
							if (String_Parse(args, &cvarLitArg))
							{	// set hightlight background
								trap->Cvar_VariableStringBuffer( cvarLitArg, string, sizeof(string) );
								Menu_SetItemBackground(menu, item->window.name, string);
								// Disable button
								Menu_ItemDisable(menu, item->window.name, qtrue);
							}
						}
					}
				}
			}
		}
		else if (Q_stricmp(name, "updatesiegeclasscnt") == 0)
		{
			const char *teamArg;

			if (String_Parse(args, &teamArg))
			{
				UI_SiegeClassCnt(atoi(teamArg));
			}
		}
		else if (Q_stricmp(name, "updatesiegecvars") == 0)
		{
			int team,baseClass;

			team = (int)trap->Cvar_VariableValue("ui_holdteam");
			baseClass = (int)trap->Cvar_VariableValue("ui_siege_class");

			UI_UpdateCvarsForClass(team, baseClass, 0);
		}
		// Save current team and class
		else if (Q_stricmp(name, "setteamclassicons") == 0)
		{
			int team = (int)trap->Cvar_VariableValue("ui_holdteam");
			char	classString[512];

			trap->Cvar_VariableStringBuffer( "ui_mySiegeClass", classString, sizeof(classString) );

			trap->Cvar_Set("ui_startsiegeteam", va("%d", team));
		 	trap->Cvar_Set( "ui_startsiegeclass", classString);

			// If player is already on a team, set up icons to show it.
			UI_FindCurrentSiegeTeamClass();
		}
		else if (Q_stricmp(name, "updatesiegeweapondesc") == 0)
		{
			menuDef_t *menu;
			itemDef_t *item;

			menu = Menu_GetFocused();	// Get current menu
			if (menu)
			{
				item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "base_class_weapons_feed");
				if (item)
				{
					char	info[MAX_INFO_VALUE];
					trap->Cvar_VariableStringBuffer( va("ui_class_weapondesc%i", item->cursorPos), info, sizeof(info) );
					trap->Cvar_Set( "ui_itemforceinvdesc", info );
				}
			}
		}
		else if (Q_stricmp(name, "updatesiegeinventorydesc") == 0)
		{
			menuDef_t *menu;
			itemDef_t *item;

			menu = Menu_GetFocused();	// Get current menu
			if (menu)
			{
				item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "base_class_inventory_feed");
				if (item)
				{
					char info[MAX_INFO_VALUE];
					trap->Cvar_VariableStringBuffer( va("ui_class_itemdesc%i", item->cursorPos), info, sizeof(info) );
					trap->Cvar_Set( "ui_itemforceinvdesc", info );
				}
			}
		}
		else if (Q_stricmp(name, "updatesiegeforcedesc") == 0)
		{
			menuDef_t *menu;
			itemDef_t *item;

			menu = Menu_GetFocused();	// Get current menu
			if (menu)
			{
				item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "base_class_force_feed");
				if (item)
				{
					int i;
					char info[MAX_STRING_CHARS];

					trap->Cvar_VariableStringBuffer( va("ui_class_power%i", item->cursorPos), info, sizeof(info) );

					//count them up
					for (i=0;i< NUM_FORCE_POWERS;i++)
					{
						if (!strcmp(HolocronIcons[i],info))
						{
							trap->Cvar_Set( "ui_itemforceinvdesc", forcepowerDesc[i] );
						}
					}
				}
			}
		}
		else if (Q_stricmp(name, "resetitemdescription") == 0)
		{
			menuDef_t *menu;
			itemDef_t *item;

			menu = Menu_GetFocused();	// Get current menu
			if (menu)
			{
				item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "itemdescription");
				if (item)
				{
					listBoxDef_t *listPtr = item->typeData.listbox;
					if (listPtr)
					{
						listPtr->startPos = 0;
						listPtr->cursorPos = 0;
					}
					item->cursorPos = 0;
				}
			}
		}
		else if (Q_stricmp(name, "resetsiegelistboxes") == 0)
		{
			menuDef_t *menu;
			itemDef_t *item;

			menu = Menu_GetFocused();	// Get current menu
			if (menu)
			{
				item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "description");
				if (item)
				{
					listBoxDef_t *listPtr = item->typeData.listbox;
					if (listPtr)
					{
						listPtr->startPos = 0;
					}
					item->cursorPos = 0;
				}
			}

			menu = Menu_GetFocused();	// Get current menu
			if (menu)
			{
				item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "base_class_weapons_feed");
				if (item)
				{
					listBoxDef_t *listPtr = item->typeData.listbox;
					if (listPtr)
					{
						listPtr->startPos = 0;
					}
					item->cursorPos = 0;
				}

				item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "base_class_inventory_feed");
				if (item)
				{
					listBoxDef_t *listPtr = item->typeData.listbox;
					if (listPtr)
					{
						listPtr->startPos = 0;
					}
					item->cursorPos = 0;
				}

				item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "base_class_force_feed");
				if (item)
				{
					listBoxDef_t *listPtr = item->typeData.listbox;
					if (listPtr)
					{
						listPtr->startPos = 0;
					}
					item->cursorPos = 0;
				}
			}
		}
		else if (Q_stricmp(name, "updatesiegestatusicons") == 0)
		{
			UI_UpdateSiegeStatusIcons();
		}
		else if (Q_stricmp(name, "setcurrentNetMap") == 0)
		{
			menuDef_t *menu;
			itemDef_t *item;

			menu = Menu_GetFocused();	// Get current menu
			if (menu)
			{
				item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "maplist");
				if (item)
				{
					listBoxDef_t *listPtr = item->typeData.listbox;
					if (listPtr)
					{
						trap->Cvar_Set("ui_currentNetMap", va("%d",listPtr->cursorPos));
					}
				}
			}
		}
		else if (Q_stricmp(name, "resetmaplist") == 0)
		{
			menuDef_t *menu;
			itemDef_t *item;

			menu = Menu_GetFocused();	// Get current menu
			if (menu)
			{
				item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "maplist");
				if (item)
				{
					uiInfo.uiDC.feederSelection(item->special, item->cursorPos, item);
				}
			}
		}
		else if (Q_stricmp(name, "getmousepitch") == 0)
		{
			trap->Cvar_Set("ui_mousePitch", (trap->Cvar_VariableValue("m_pitch") >= 0) ? "0" : "1");
		}
		else if (Q_stricmp(name, "clampmaxplayers") == 0)
		{
			UI_ClampMaxPlayers();
		}
		else if ( Q_stricmp( name, "LaunchSP" ) == 0 )
		{
			// TODO for MAC_PORT
		}
		else
		{
			Com_Printf("unknown UI script %s\n", name);
		}
	}
}

static void UI_GetTeamColor(vec4_t *color) {
}

void UI_SetSiegeTeams(void)
{
	char			info[MAX_INFO_VALUE];
	char			*mapname = NULL;
	char			levelname[MAX_QPATH];
	char			btime[1024];
	char			teams[2048];
	char			teamInfo[MAX_SIEGE_INFO_SIZE];
	char			team1[1024];
	char			team2[1024];
	int				len = 0;
	int				gametype;
	fileHandle_t	f;

	//Get the map name from the server info
	if (trap->GetConfigString( CS_SERVERINFO, info, sizeof(info) ))
	{
		mapname = Info_ValueForKey( info, "mapname" );
	}

	if (!mapname || !mapname[0])
	{
		return;
	}

	gametype = atoi(Info_ValueForKey(info, "g_gametype"));

	//If the server we are connected to is not siege we cannot choose a class anyway
	if (gametype != GT_SIEGE)
	{
		return;
	}

	Com_sprintf(levelname, sizeof(levelname), "maps/%s.siege", mapname);

	if (!levelname[0])
	{
		return;
	}

	len = trap->FS_Open(levelname, &f, FS_READ);

	if (!f) {
		return;
	}
	if (len >= MAX_SIEGE_INFO_SIZE) {
		trap->FS_Close( f );
		return;
	}

	trap->FS_Read(siege_info, len, f);
	siege_info[len] = 0;	//ensure null terminated

	trap->FS_Close(f);

	//Found the .siege file

	if (BG_SiegeGetValueGroup(siege_info, "Teams", teams))
	{
		char buf[1024];

		trap->Cvar_VariableStringBuffer("cg_siegeTeam1", buf, 1024);
		if (buf[0] && Q_stricmp(buf, "none"))
		{
			Q_strncpyz( team1, buf, sizeof( team1 ) );
		}
		else
		{
			BG_SiegeGetPairedValue(teams, "team1", team1);
		}

		trap->Cvar_VariableStringBuffer("cg_siegeTeam2", buf, 1024);
		if (buf[0] && Q_stricmp(buf, "none"))
		{
			Q_strncpyz( team2, buf, sizeof( team2 ) );
		}
		else
		{
			BG_SiegeGetPairedValue(teams, "team2", team2);
		}
	}
	else
	{
		return;
	}

	//Set the team themes so we know what classes to make available for selection
	if (BG_SiegeGetValueGroup(siege_info, team1, teamInfo))
	{
		if (BG_SiegeGetPairedValue(teamInfo, "UseTeam", btime))
		{
			BG_SiegeSetTeamTheme(SIEGETEAM_TEAM1, btime);
		}
	}
	if (BG_SiegeGetValueGroup(siege_info, team2, teamInfo))
	{
		if (BG_SiegeGetPairedValue(teamInfo, "UseTeam", btime))
		{
			BG_SiegeSetTeamTheme(SIEGETEAM_TEAM2, btime);
		}
	}

	siegeTeam1 = BG_SiegeFindThemeForTeam(SIEGETEAM_TEAM1);
	siegeTeam2 = BG_SiegeFindThemeForTeam(SIEGETEAM_TEAM2);

	//set the default description for the default selection
	if (!siegeTeam1 || !siegeTeam1->classes[0])
	{
		Com_Error(ERR_DROP, "Error loading teams in UI");
	}

	Menu_SetFeederSelection(NULL, FEEDER_SIEGE_TEAM1, 0, NULL);
	Menu_SetFeederSelection(NULL, FEEDER_SIEGE_TEAM2, -1, NULL);
}

static void UI_SiegeClassCnt( const int team )
{
	UI_SetSiegeTeams();

	trap->Cvar_Set("ui_infantry_cnt", va("%d", BG_SiegeCountBaseClass(team,0)));
	trap->Cvar_Set("ui_vanguard_cnt", va("%d", BG_SiegeCountBaseClass(team,1)));
	trap->Cvar_Set("ui_support_cnt", va("%d", BG_SiegeCountBaseClass(team,2)));
	trap->Cvar_Set("ui_jedi_cnt", va("%d", BG_SiegeCountBaseClass(team,3)));
	trap->Cvar_Set("ui_demo_cnt", va("%d", BG_SiegeCountBaseClass(team,4)));
	trap->Cvar_Set("ui_heavy_cnt", va("%d", BG_SiegeCountBaseClass(team,5)));
}

/*
==================
UI_MapCountByGameType
==================
*/
static int UI_MapCountByGameType(qboolean singlePlayer) {
	int i, c, game;
	c = 0;
	game = singlePlayer ? uiInfo.gameTypes[ui_gametype.integer].gtEnum : uiInfo.gameTypes[ui_netGametype.integer].gtEnum;
	if (game == GT_TEAM)
		game = GT_FFA;

	//Since GT_CTY uses the same entities as CTF, use the same map sets
	if ( game == GT_CTY )
		game = GT_CTF;

	for (i = 0; i < uiInfo.mapCount; i++) {
		uiInfo.mapList[i].active = qfalse;
		if ( uiInfo.mapList[i].typeBits & (1 << game)) {
			if (singlePlayer) {
				if (!(uiInfo.mapList[i].typeBits & (1 << GT_SINGLE_PLAYER))) {
					continue;
				}
			}
			c++;
			uiInfo.mapList[i].active = qtrue;
		}
	}
	return c;
}

qboolean UI_hasSkinForBase(const char *base, const char *team) {
	char	test[1024];
	fileHandle_t	f;

	Com_sprintf( test, sizeof( test ), "models/players/%s/%s/lower_default.skin", base, team );
	trap->FS_Open(test, &f, FS_READ);
	if (f != 0) {
		trap->FS_Close(f);
		return qtrue;
	}
	Com_sprintf( test, sizeof( test ), "models/players/characters/%s/%s/lower_default.skin", base, team );
	trap->FS_Open(test, &f, FS_READ);
	if (f != 0) {
		trap->FS_Close(f);
		return qtrue;
	}
	return qfalse;
}

/*
==================
UI_HeadCountByColor
==================
*/
static int UI_HeadCountByColor(void) {
	int i, c;
	char *teamname;

	c = 0;

	switch(uiSkinColor)
	{
		case TEAM_BLUE:
			teamname = "/blue";
			break;
		case TEAM_RED:
			teamname = "/red";
			break;
		default:
			teamname = "/default";
	}

	// Count each head with this color
	for (i=0; i<uiInfo.q3HeadCount; i++)
	{
		if (uiInfo.q3HeadNames[i][0] && strstr(uiInfo.q3HeadNames[i], teamname))
		{
			c++;
		}
	}
	return c;
}

int Q_isprintext( int c );
int Q_isgraph( int c );
/*
==================
UI_ServerInfoIsValid

Return false if the infostring contains nonprinting characters,
or if the hostname is blank/undefined
==================
*/
static qboolean UI_ServerInfoIsValid( char *info )
{
	char *c;

	for ( c = info; *c; c++ )
	{
		if ( !Q_isprintext( *(unsigned char *)c ) ) //isprint
			return qfalse;
	}

	for ( c = Info_ValueForKey( info, "hostname" ); *c; c++ )
	{
		if ( Q_isgraph( *(unsigned char *)c ) ) //isgraph
			return qtrue;
	}

	return qfalse;
}

/*
==================
UI_InsertServerIntoDisplayList
==================
*/
static void UI_InsertServerIntoDisplayList(int num, int position) {
	int i;
	static char info[MAX_STRING_CHARS] = { 0 };

	if (position < 0 || position > uiInfo.serverStatus.numDisplayServers ) {
		return;
	}

	trap->LAN_GetServerInfo( UI_SourceForLAN(), num, info, sizeof(info) );

	uiInfo.serverStatus.numDisplayServers++;
	for (i = uiInfo.serverStatus.numDisplayServers; i > position; i--) {
		uiInfo.serverStatus.displayServers[i] = uiInfo.serverStatus.displayServers[i-1];
	}
	uiInfo.serverStatus.displayServers[position] = num;
}

/*
==================
UI_RemoveServerFromDisplayList
==================
*/
static void UI_RemoveServerFromDisplayList(int num) {
	int i, j;

	for (i = 0; i < uiInfo.serverStatus.numDisplayServers; i++) {
		if (uiInfo.serverStatus.displayServers[i] == num) {
			uiInfo.serverStatus.numDisplayServers--;
			for (j = i; j < uiInfo.serverStatus.numDisplayServers; j++) {
				uiInfo.serverStatus.displayServers[j] = uiInfo.serverStatus.displayServers[j+1];
			}
			return;
		}
	}
}

/*
==================
UI_BinaryServerInsertion
==================
*/
static void UI_BinaryServerInsertion(int num) {
	int mid, offset, res, len;

	// use binary search to insert server
	len = uiInfo.serverStatus.numDisplayServers;
	mid = len;
	offset = 0;
	res = 0;
	while(mid > 0) {
		mid = len >> 1;
		//
		res = trap->LAN_CompareServers( UI_SourceForLAN(), uiInfo.serverStatus.sortKey,
					uiInfo.serverStatus.sortDir, num, uiInfo.serverStatus.displayServers[offset+mid]);
		// if equal
		if (res == 0) {
			UI_InsertServerIntoDisplayList(num, offset+mid);
			return;
		}
		// if larger
		else if (res == 1) {
			offset += mid;
			len -= mid;
		}
		// if smaller
		else {
			len -= mid;
		}
	}
	if (res == 1) {
		offset++;
	}
	UI_InsertServerIntoDisplayList(num, offset);
}

/*
==================
UI_BuildServerDisplayList
==================
*/
static void UI_BuildServerDisplayList(int force) {
	int i, count, clients, maxClients, ping, game, len, passw/*, visible*/;
	char info[MAX_STRING_CHARS];
//	qboolean startRefresh = qtrue; TTimo: unused
	static int numinvisible;
	int	lanSource;

	if (!(force || uiInfo.uiDC.realTime > uiInfo.serverStatus.nextDisplayRefresh)) {
		return;
	}
	// if we shouldn't reset
	if ( force == 2 ) {
		force = 0;
	}

	// do motd updates here too
	trap->Cvar_VariableStringBuffer( "cl_motdString", uiInfo.serverStatus.motd, sizeof(uiInfo.serverStatus.motd) );
	len = strlen(uiInfo.serverStatus.motd);
	if (len == 0) {
		Q_strncpyz( uiInfo.serverStatus.motd, "Welcome to Jedi Academy MP!", sizeof( uiInfo.serverStatus.motd ) );
		len = strlen(uiInfo.serverStatus.motd);
	}
	if (len != uiInfo.serverStatus.motdLen) {
		uiInfo.serverStatus.motdLen = len;
		uiInfo.serverStatus.motdWidth = -1;
	}

	lanSource = UI_SourceForLAN();

	if (force) {
		numinvisible = 0;
		// clear number of displayed servers
		uiInfo.serverStatus.numDisplayServers = 0;
		uiInfo.serverStatus.numPlayersOnServers = 0;
		// set list box index to zero
		Menu_SetFeederSelection(NULL, FEEDER_SERVERS, 0, NULL);
		// mark all servers as visible so we store ping updates for them
		trap->LAN_MarkServerVisible(lanSource, -1, qtrue);
	}

	// get the server count (comes from the master)
	count = trap->LAN_GetServerCount(lanSource);
	if (count == -1 || (ui_netSource.integer == UIAS_LOCAL && count == 0) ) {
		// still waiting on a response from the master
		uiInfo.serverStatus.numDisplayServers = 0;
		uiInfo.serverStatus.numPlayersOnServers = 0;
		uiInfo.serverStatus.nextDisplayRefresh = uiInfo.uiDC.realTime + 500;
		return;
	}

	trap->Cvar_Update( &ui_browserFilterInvalidInfo );
	trap->Cvar_Update( &ui_browserShowEmpty );
	trap->Cvar_Update( &ui_browserShowFull );
	trap->Cvar_Update( &ui_browserShowPasswordProtected );
	trap->Cvar_Update( &ui_serverFilterType );
	trap->Cvar_Update( &ui_joinGametype );

//	visible = qfalse;
	for (i = 0; i < count; i++) {
		// if we already got info for this server
		if (!trap->LAN_ServerIsVisible(lanSource, i)) {
			continue;
		}
//		visible = qtrue;
		// get the ping for this server
		ping = trap->LAN_GetServerPing(lanSource, i);
		if (ping > 0 || ui_netSource.integer == UIAS_FAVORITES) {

			trap->LAN_GetServerInfo(lanSource, i, info, MAX_STRING_CHARS);

			// don't list servers with invalid info
			if ( ui_browserFilterInvalidInfo.integer != 0 && !UI_ServerInfoIsValid( info ) ) {
				trap->LAN_MarkServerVisible( lanSource, i, qfalse );
				continue;
			}

			clients = atoi(Info_ValueForKey(info, "clients"));
			uiInfo.serverStatus.numPlayersOnServers += clients;

			if (ui_browserShowEmpty.integer == 0) {
				if (clients == 0) {
					trap->LAN_MarkServerVisible(lanSource, i, qfalse);
					continue;
				}
			}

			if (ui_browserShowFull.integer == 0) {
				maxClients = atoi(Info_ValueForKey(info, "sv_maxclients"));
				if (clients == maxClients) {
					trap->LAN_MarkServerVisible(lanSource, i, qfalse);
					continue;
				}
			}

			if ( ui_browserShowPasswordProtected.integer == 0 ) {
				passw = atoi(Info_ValueForKey(info, "needpass"));
				if (passw && !ui_browserShowPasswordProtected.integer) {
					trap->LAN_MarkServerVisible(lanSource, i, qfalse);
					continue;
				}
			}

			if (uiInfo.joinGameTypes[ui_joinGametype.integer].gtEnum != -1) {
				game = atoi(Info_ValueForKey(info, "gametype"));
				if (game != uiInfo.joinGameTypes[ui_joinGametype.integer].gtEnum) {
					trap->LAN_MarkServerVisible(lanSource, i, qfalse);
					continue;
				}
			}

			if (ui_serverFilterType.integer > 0 && ui_serverFilterType.integer <= uiInfo.modCount) {
				if (Q_stricmp(Info_ValueForKey(info, "game"), UI_FilterDir( ui_serverFilterType.integer ) ) != 0) {
					trap->LAN_MarkServerVisible(lanSource, i, qfalse);
					continue;
				}
			}
			// make sure we never add a favorite server twice
			if (ui_netSource.integer == UIAS_FAVORITES) {
				UI_RemoveServerFromDisplayList(i);
			}
			// insert the server into the list
			UI_BinaryServerInsertion(i);
			// done with this server
			if (ping > 0) {
				trap->LAN_MarkServerVisible(lanSource, i, qfalse);
				numinvisible++;
			}
		}
	}

	uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime;

	// if there were no servers visible for ping updates
//	if (!visible) {
//		UI_StopServerRefresh();
//		uiInfo.serverStatus.nextDisplayRefresh = 0;
//	}
}

typedef struct serverStatusCvar_s {
	char *name, *altName;
} serverStatusCvar_t;

serverStatusCvar_t serverStatusCvars[] = {
	{"sv_hostname", "Name"},
	{"Address", ""},
	{"gamename", "Game name"},
	{"g_gametype", "Game type"},
	{"mapname", "Map"},
	{"version", ""},
	{"protocol", ""},
	{"timelimit", ""},
	{"fraglimit", ""},
	{NULL, NULL}
};

/*
==================
UI_SortServerStatusInfo
==================
*/
static void UI_SortServerStatusInfo( serverStatusInfo_t *info ) {
	int i, j, index, numLines;
	char *tmp1, *tmp2;

	// FIXME: if "gamename" == "base" or "missionpack" then
	// replace the gametype number by FFA, CTF etc.
	//
	index = 0;
	numLines = Com_Clampi( 0, MAX_SERVERSTATUS_LINES, info->numLines );
	for (i = 0; serverStatusCvars[i].name; i++) {
		for (j = 0; j < numLines; j++) {
			if ( !info->lines[j][1] || info->lines[j][1][0] ) {
				continue;
			}
			if ( !Q_stricmp(serverStatusCvars[i].name, info->lines[j][0]) ) {
				// swap lines
				tmp1 = info->lines[index][0];
				tmp2 = info->lines[index][3];
				info->lines[index][0] = info->lines[j][0];
				info->lines[index][3] = info->lines[j][3];
				info->lines[j][0] = tmp1;
				info->lines[j][3] = tmp2;
				//
				if ( strlen(serverStatusCvars[i].altName) ) {
					info->lines[index][0] = serverStatusCvars[i].altName;
				}
				index++;
			}
		}
	}
}

/*
==================
UI_GetServerStatusInfo
==================
*/
static int UI_GetServerStatusInfo( const char *serverAddress, serverStatusInfo_t *info ) {
	char *p, *score, *ping, *name;
	int i, len;

	if (!info) {
		trap->LAN_ServerStatus( serverAddress, NULL, 0);
		return qfalse;
	}
	memset(info, 0, sizeof(*info));
	if ( trap->LAN_ServerStatus( serverAddress, info->text, sizeof(info->text)) ) {
		Q_strncpyz(info->address, serverAddress, sizeof(info->address));
		p = info->text;
		info->numLines = 0;
		info->lines[info->numLines][0] = "Address";
		info->lines[info->numLines][1] = "";
		info->lines[info->numLines][2] = "";
		info->lines[info->numLines][3] = info->address;
		info->numLines++;
		// get the cvars
		while (p && *p) {
			p = strchr(p, '\\');
			if (!p) break;
			*p++ = '\0';
			if (*p == '\\')
				break;
			info->lines[info->numLines][0] = p;
			info->lines[info->numLines][1] = "";
			info->lines[info->numLines][2] = "";
			p = strchr(p, '\\');
			if (!p) break;
			*p++ = '\0';
			info->lines[info->numLines][3] = p;

			info->numLines++;
			if (info->numLines >= MAX_SERVERSTATUS_LINES)
				break;
		}
		// get the player list
		if (info->numLines < MAX_SERVERSTATUS_LINES-3) {
			// empty line
			info->lines[info->numLines][0] = "";
			info->lines[info->numLines][1] = "";
			info->lines[info->numLines][2] = "";
			info->lines[info->numLines][3] = "";
			info->numLines++;
			// header
			info->lines[info->numLines][0] = "num";
			info->lines[info->numLines][1] = "score";
			info->lines[info->numLines][2] = "ping";
			info->lines[info->numLines][3] = "name";
			info->numLines++;
			// parse players
			i = 0;
			len = 0;
			while (p && *p) {
				if (*p == '\\')
					*p++ = '\0';
				score = p;
				p = strchr(p, ' ');
				if (!p)
					break;
				*p++ = '\0';
				ping = p;
				p = strchr(p, ' ');
				if (!p)
					break;
				*p++ = '\0';
				name = p;
				Com_sprintf(&info->pings[len], sizeof(info->pings)-len, "%d", i);
				info->lines[info->numLines][0] = &info->pings[len];
				len += strlen(&info->pings[len]) + 1;
				info->lines[info->numLines][1] = score;
				info->lines[info->numLines][2] = ping;
				info->lines[info->numLines][3] = name;
				info->numLines++;
				if (info->numLines >= MAX_SERVERSTATUS_LINES)
					break;
				p = strchr(p, '\\');
				if (!p)
					break;
				*p++ = '\0';
				//
				i++;
			}
		}
		UI_SortServerStatusInfo( info );
		return qtrue;
	}
	return qfalse;
}

/*
==================
UI_BuildFindPlayerList
==================
*/
static void UI_BuildFindPlayerList(qboolean force) {
	static int numFound, numTimeOuts;
	int i, j, resend;
	serverStatusInfo_t info;
	char name[MAX_NAME_LENGTH+2];
	char infoString[MAX_STRING_CHARS];
	int  lanSource;

	if (!force) {
		if (!uiInfo.nextFindPlayerRefresh || uiInfo.nextFindPlayerRefresh > uiInfo.uiDC.realTime) {
			return;
		}
	}
	else {
		memset(&uiInfo.pendingServerStatus, 0, sizeof(uiInfo.pendingServerStatus));
		uiInfo.numFoundPlayerServers = 0;
		uiInfo.currentFoundPlayerServer = 0;
		trap->Cvar_VariableStringBuffer( "ui_findPlayer", uiInfo.findPlayerName, sizeof(uiInfo.findPlayerName));
		Q_StripColor(uiInfo.findPlayerName);
		// should have a string of some length
		if (!strlen(uiInfo.findPlayerName)) {
			uiInfo.nextFindPlayerRefresh = 0;
			return;
		}
		// set resend time
		resend = ui_serverStatusTimeOut.integer / 2 - 10;
		if (resend < 50) {
			resend = 50;
		}
		trap->Cvar_Set("cl_serverStatusResendTime", va("%d", resend));
		// reset all server status requests
		trap->LAN_ServerStatus( NULL, NULL, 0);
		//
		uiInfo.numFoundPlayerServers = 1;

		trap->SE_GetStringTextString("MENUS_SEARCHING", holdSPString, sizeof(holdSPString));
		trap->Cvar_Set( "ui_playerServersFound", va(	holdSPString,uiInfo.pendingServerStatus.num, numFound));
	//	Com_sprintf(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers-1],
	//					sizeof(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers-1]),
	//						"searching %d...", uiInfo.pendingServerStatus.num);
		numFound = 0;
		numTimeOuts++;
	}
	for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++) {
		// if this pending server is valid
		if (uiInfo.pendingServerStatus.server[i].valid) {
			// try to get the server status for this server
			if (UI_GetServerStatusInfo( uiInfo.pendingServerStatus.server[i].adrstr, &info ) ) {
				//
				numFound++;
				// parse through the server status lines
				for (j = 0; j < info.numLines; j++) {
					// should have ping info
					if ( !info.lines[j][2] || !info.lines[j][2][0] ) {
						continue;
					}
					// clean string first
					Q_strncpyz(name, info.lines[j][3], sizeof(name));
					Q_StripColor(name);
					// if the player name is a substring
					if (Q_stristr(name, uiInfo.findPlayerName)) {
						// add to found server list if we have space (always leave space for a line with the number found)
						if (uiInfo.numFoundPlayerServers < MAX_FOUNDPLAYER_SERVERS-1) {
							//
							Q_strncpyz(uiInfo.foundPlayerServerAddresses[uiInfo.numFoundPlayerServers-1],
										uiInfo.pendingServerStatus.server[i].adrstr,
											sizeof(uiInfo.foundPlayerServerAddresses[0]));
							Q_strncpyz(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers-1],
										uiInfo.pendingServerStatus.server[i].name,
											sizeof(uiInfo.foundPlayerServerNames[0]));
							uiInfo.numFoundPlayerServers++;
						}
						else {
							// can't add any more so we're done
							uiInfo.pendingServerStatus.num = uiInfo.serverStatus.numDisplayServers;
						}
					}
				}

				trap->SE_GetStringTextString("MENUS_SEARCHING", holdSPString, sizeof(holdSPString));
				trap->Cvar_Set( "ui_playerServersFound", va(	holdSPString,uiInfo.pendingServerStatus.num, numFound));
			//	Com_sprintf(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers-1],
			//					sizeof(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers-1]),
			//						"searching %d/%d...", uiInfo.pendingServerStatus.num, numFound);
				// retrieved the server status so reuse this spot
				uiInfo.pendingServerStatus.server[i].valid = qfalse;
			}
		}
		// if empty pending slot or timed out
		if (!uiInfo.pendingServerStatus.server[i].valid ||
			uiInfo.pendingServerStatus.server[i].startTime < uiInfo.uiDC.realTime - ui_serverStatusTimeOut.integer) {
			if (uiInfo.pendingServerStatus.server[i].valid) {
				numTimeOuts++;
			}
			// reset server status request for this address
			UI_GetServerStatusInfo( uiInfo.pendingServerStatus.server[i].adrstr, NULL );
			// reuse pending slot
			uiInfo.pendingServerStatus.server[i].valid = qfalse;
			// if we didn't try to get the status of all servers in the main browser yet
			if (uiInfo.pendingServerStatus.num < uiInfo.serverStatus.numDisplayServers) {
				uiInfo.pendingServerStatus.server[i].startTime = uiInfo.uiDC.realTime;
				lanSource = UI_SourceForLAN();
				trap->LAN_GetServerAddressString(lanSource, uiInfo.serverStatus.displayServers[uiInfo.pendingServerStatus.num],
							uiInfo.pendingServerStatus.server[i].adrstr, sizeof(uiInfo.pendingServerStatus.server[i].adrstr));
				trap->LAN_GetServerInfo(lanSource, uiInfo.serverStatus.displayServers[uiInfo.pendingServerStatus.num], infoString, sizeof(infoString));
				Q_strncpyz(uiInfo.pendingServerStatus.server[i].name, Info_ValueForKey(infoString, "hostname"), sizeof(uiInfo.pendingServerStatus.server[0].name));
				uiInfo.pendingServerStatus.server[i].valid = qtrue;
				uiInfo.pendingServerStatus.num++;

				trap->SE_GetStringTextString("MENUS_SEARCHING", holdSPString, sizeof(holdSPString));
				trap->Cvar_Set( "ui_playerServersFound", va(	holdSPString,uiInfo.pendingServerStatus.num, numFound));

			//	Com_sprintf(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers-1],
			//					sizeof(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers-1]),
			//						"searching %d/%d...", uiInfo.pendingServerStatus.num, numFound);
			}
		}
	}
	for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++) {
		if (uiInfo.pendingServerStatus.server[i].valid) {
			break;
		}
	}
	// if still trying to retrieve server status info
	if (i < MAX_SERVERSTATUSREQUESTS) {
		uiInfo.nextFindPlayerRefresh = uiInfo.uiDC.realTime + 25;
	}
	else {
		trap->SE_GetStringTextString("MENUS_SERVERS_FOUNDWITH", holdSPString, sizeof(holdSPString));
		// add a line that shows the number of servers found
		if (!uiInfo.numFoundPlayerServers)
		{
			trap->Cvar_Set( "ui_playerServersFound", va(	holdSPString,
														0,
														"s",
														uiInfo.findPlayerName) );
		}
		else
		{
			trap->Cvar_Set( "ui_playerServersFound", va(	holdSPString,
														uiInfo.numFoundPlayerServers-1,
														uiInfo.numFoundPlayerServers == 2 ? "":"s",
														uiInfo.findPlayerName) );
		}
		uiInfo.nextFindPlayerRefresh = 0;
		// show the server status info for the selected server
		UI_FeederSelection(FEEDER_FINDPLAYER, uiInfo.currentFoundPlayerServer, NULL);
	}
}

/*
==================
UI_BuildServerStatus
==================
*/
static void UI_BuildServerStatus(qboolean force) {

	if (uiInfo.nextFindPlayerRefresh) {
		return;
	}
	if (!force) {
		if (!uiInfo.nextServerStatusRefresh || uiInfo.nextServerStatusRefresh > uiInfo.uiDC.realTime) {
			return;
		}
	}
	else {
		Menu_SetFeederSelection(NULL, FEEDER_SERVERSTATUS, 0, NULL);
		uiInfo.serverStatusInfo.numLines = 0;
		// reset all server status requests
		trap->LAN_ServerStatus( NULL, NULL, 0);
	}
	if (uiInfo.serverStatus.currentServer < 0 || uiInfo.serverStatus.currentServer > uiInfo.serverStatus.numDisplayServers || uiInfo.serverStatus.numDisplayServers == 0) {
		return;
	}
	if (UI_GetServerStatusInfo( uiInfo.serverStatusAddress, &uiInfo.serverStatusInfo ) ) {
		uiInfo.nextServerStatusRefresh = 0;
		UI_GetServerStatusInfo( uiInfo.serverStatusAddress, NULL );
	}
	else {
		uiInfo.nextServerStatusRefresh = uiInfo.uiDC.realTime + 500;
	}
}

/*
==================
UI_FeederCount
==================
*/
static int UI_FeederCount(float feederID)
{
	int team,baseClass,count=0,i;
	static char info[MAX_STRING_CHARS];

	switch ( (int)feederID )
	{
		case FEEDER_SABER_SINGLE_INFO:

			for (i=0;i<MAX_SABER_HILTS;i++)
			{
				if (saberSingleHiltInfo[i])
				{
					count++;
				}
				else
				{//done
					break;
				}
			}
			return count;

		case FEEDER_SABER_STAFF_INFO:

			for (i=0;i<MAX_SABER_HILTS;i++)
			{
				if (saberStaffHiltInfo[i])
				{
					count++;
				}
				else
				{//done
					break;
				}
			}
			return count;

		case FEEDER_Q3HEADS:
			return UI_HeadCountByColor();

		case FEEDER_SIEGE_TEAM1:
			if (!siegeTeam1)
			{
				UI_SetSiegeTeams();
				if (!siegeTeam1)
				{
					return 0;
				}
			}
			return siegeTeam1->numClasses;
		case FEEDER_SIEGE_TEAM2:
			if (!siegeTeam2)
			{
				UI_SetSiegeTeams();
				if (!siegeTeam2)
				{
					return 0;
				}
			}
			return siegeTeam2->numClasses;

		case FEEDER_FORCECFG:
			if (uiForceSide == FORCE_LIGHTSIDE)
			{
				return uiInfo.forceConfigCount-uiInfo.forceConfigLightIndexBegin;
			}
			else
			{
				return uiInfo.forceConfigLightIndexBegin+1;
			}
			//return uiInfo.forceConfigCount;

		case FEEDER_CINEMATICS:
			return uiInfo.movieCount;

		case FEEDER_MAPS:
		case FEEDER_ALLMAPS:
			return UI_MapCountByGameType(feederID == FEEDER_MAPS ? qtrue : qfalse);

		case FEEDER_SERVERS:
			return uiInfo.serverStatus.numDisplayServers;

		case FEEDER_SERVERSTATUS:
			return Com_Clampi( 0, MAX_SERVERSTATUS_LINES, uiInfo.serverStatusInfo.numLines );

		case FEEDER_FINDPLAYER:
			return uiInfo.numFoundPlayerServers;

		case FEEDER_PLAYER_LIST:
			if (uiInfo.uiDC.realTime > uiInfo.playerRefresh)
			{
				uiInfo.playerRefresh = uiInfo.uiDC.realTime + 3000;
				UI_BuildPlayerList();
			}
			return uiInfo.playerCount;

		case FEEDER_TEAM_LIST:
			if (uiInfo.uiDC.realTime > uiInfo.playerRefresh)
			{
				uiInfo.playerRefresh = uiInfo.uiDC.realTime + 3000;
				UI_BuildPlayerList();
			}
			return uiInfo.myTeamCount;

		case FEEDER_MODS:
			return uiInfo.modCount;

		case FEEDER_DEMOS:
			return uiInfo.demoCount;

		case FEEDER_MOVES :

			for (i=0;i<MAX_MOVES;i++)
			{
				if (datapadMoveData[uiInfo.movesTitleIndex][i].title)
				{
					count++;
				}
			}

			return count;

		case FEEDER_MOVES_TITLES :
			return (MD_MOVE_TITLE_MAX);

		case FEEDER_PLAYER_SPECIES:
			return uiInfo.playerSpeciesCount;

		case FEEDER_PLAYER_SKIN_HEAD:
			return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHeadCount;

		case FEEDER_PLAYER_SKIN_TORSO:
			return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorsoCount;

		case FEEDER_PLAYER_SKIN_LEGS:
			return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLegCount;

		case FEEDER_COLORCHOICES:
			return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].ColorCount;

		case FEEDER_SIEGE_BASE_CLASS:
			team = (int)trap->Cvar_VariableValue("ui_team");
			baseClass = (int)trap->Cvar_VariableValue("ui_siege_class");

			if ((team == SIEGETEAM_TEAM1) ||
				(team == SIEGETEAM_TEAM2))
			{
				// Is it a valid base class?
				if ((baseClass >= SPC_INFANTRY) && (baseClass < SPC_MAX))
				{
					return (BG_SiegeCountBaseClass( team, baseClass ));
				}
			}
			return 0;

		// Get the count of weapons
		case FEEDER_SIEGE_CLASS_WEAPONS:
			//count them up
			for (i=0;i< WP_NUM_WEAPONS;i++)
			{
				trap->Cvar_VariableStringBuffer( va("ui_class_weapon%i", i), info, sizeof(info) );
				if (Q_stricmp(info,"gfx/2d/select")!=0)
				{
					count++;
				}
			}

			return count;

		// Get the count of inventory
		case FEEDER_SIEGE_CLASS_INVENTORY:
			//count them up
			for (i=0;i< HI_NUM_HOLDABLE;i++)
			{
				trap->Cvar_VariableStringBuffer( va("ui_class_item%i", i), info, sizeof(info) );
				// A hack so health and ammo dispenser icons don't show up.
				if ((Q_stricmp(info,"gfx/2d/select")!=0) &&
					(Q_stricmp(info,"gfx/hud/i_icon_healthdisp")!=0) &&
					(Q_stricmp(info,"gfx/hud/i_icon_ammodisp")!=0))
				{
					count++;
				}
			}
			return count;

		// Get the count of force powers
		case FEEDER_SIEGE_CLASS_FORCE:
			//count them up
			for (i=0;i< NUM_FORCE_POWERS;i++)
			{
				trap->Cvar_VariableStringBuffer( va("ui_class_power%i", i), info, sizeof(info) );
				if (Q_stricmp(info,"gfx/2d/select")!=0)
				{
					count++;
				}
			}
			return count;
	}

	return 0;
}

static const char *UI_SelectedMap(int index, int *actual) {
	int i, c;
	c = 0;
	*actual = 0;

	for (i = 0; i < uiInfo.mapCount; i++) {
		if (uiInfo.mapList[i].active) {
			if (c == index) {
				*actual = i;
				return uiInfo.mapList[i].mapName;
			} else {
				c++;
			}
		}
	}
	return "";
}

/*
==================
UI_HeadCountByColor
==================
*/
static const char *UI_SelectedTeamHead(int index, int *actual) {
	char *teamname;
	int i,c=0;

	switch(uiSkinColor)
	{
		case TEAM_BLUE:
			teamname = "/blue";
			break;
		case TEAM_RED:
			teamname = "/red";
			break;
		default:
			teamname = "/default";
			break;
	}

	// Count each head with this color

	for (i=0; i<uiInfo.q3HeadCount; i++)
	{
		if (uiInfo.q3HeadNames[i][0] && strstr(uiInfo.q3HeadNames[i], teamname))
		{
			if (c==index)
			{
				*actual = i;
				return uiInfo.q3HeadNames[i];
			}
			else
			{
				c++;
			}
		}
	}
	return "";
}

static int UI_GetIndexFromSelection(int actual) {
	int i, c;
	c = 0;
	for (i = 0; i < uiInfo.mapCount; i++) {
		if (uiInfo.mapList[i].active) {
			if (i == actual) {
				return c;
			}
			c++;
		}
	}
	return 0;
}

static void UI_UpdatePendingPings() {
	trap->LAN_ResetPings(UI_SourceForLAN());
	uiInfo.serverStatus.refreshActive = qtrue;
	uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 1000;
}

static const char *UI_FeederItemText(float feederID, int index, int column,
									 qhandle_t *handle1, qhandle_t *handle2, qhandle_t *handle3) {
	static char info[MAX_STRING_CHARS]; // don't change this size without changing the sizes inside the SaberProperName calls
	static char hostname[MAX_HOSTNAMELENGTH] = {0};
	static char clientBuff[32];
	static char needPass[32];
	static int lastColumn = -1;
	static int lastTime = 0;
	*handle1 = *handle2 = *handle3 = -1;

	if (feederID == FEEDER_SABER_SINGLE_INFO)
	{
		//char *saberProperName=0;
		UI_SaberProperNameForSaber( saberSingleHiltInfo[index], info );
		return info;
	}
	else if	(feederID == FEEDER_SABER_STAFF_INFO)
	{
		//char *saberProperName=0;
		UI_SaberProperNameForSaber( saberStaffHiltInfo[index], info );
		return info;
	}
	else if (feederID == FEEDER_Q3HEADS) {
		int actual;
		return UI_SelectedTeamHead(index, &actual);
	}
	else if (feederID == FEEDER_SIEGE_TEAM1)
	{
		return ""; //nothing I guess, the description part can cover this
		/*
		if (!siegeTeam1)
		{
			UI_SetSiegeTeams();
			if (!siegeTeam1)
			{
				return "";
			}
		}

		if (siegeTeam1->classes[index])
		{
			return siegeTeam1->classes[index]->name;
		}
		return "";
		*/
	}
	else if (feederID == FEEDER_SIEGE_TEAM2)
	{
		return ""; //nothing I guess, the description part can cover this
		/*
		if (!siegeTeam1)
		{
			UI_SetSiegeTeams();
			if (!siegeTeam1)
			{
				return "";
			}
		}

		if (siegeTeam2->classes[index])
		{
			return siegeTeam2->classes[index]->name;
		}
		return "";
		*/
	}
	else if (feederID == FEEDER_FORCECFG) {
		if (index >= 0 && index < uiInfo.forceConfigCount) {
			if (index == 0)
			{ //always show "custom"
				return uiInfo.forceConfigNames[index];
			}
			else
			{
				if (uiForceSide == FORCE_LIGHTSIDE)
				{
					index += uiInfo.forceConfigLightIndexBegin;
					if (index < 0)
					{
						return NULL;
					}
					if (index >= uiInfo.forceConfigCount)
					{
						return NULL;
					}
					return uiInfo.forceConfigNames[index];
				}
				else if (uiForceSide == FORCE_DARKSIDE)
				{
					index += uiInfo.forceConfigDarkIndexBegin;
					if (index < 0)
					{
						return NULL;
					}
					if (index > uiInfo.forceConfigLightIndexBegin)
					{ //dark gets read in before light
						return NULL;
					}
					if (index >= uiInfo.forceConfigCount)
					{
						return NULL;
					}
					return uiInfo.forceConfigNames[index];
				}
				else
				{
					return NULL;
				}
			}
		}
	} else if (feederID == FEEDER_MAPS || feederID == FEEDER_ALLMAPS) {
		int actual;
		return UI_SelectedMap(index, &actual);
	} else if (feederID == FEEDER_SERVERS) {
		if (index >= 0 && index < uiInfo.serverStatus.numDisplayServers) {
			int ping, game;
			if (lastColumn != column || lastTime > uiInfo.uiDC.realTime + 5000) {
				trap->LAN_GetServerInfo(UI_SourceForLAN(), uiInfo.serverStatus.displayServers[index], info, MAX_STRING_CHARS);
				lastColumn = column;
				lastTime = uiInfo.uiDC.realTime;
			}
			ping = atoi(Info_ValueForKey(info, "ping"));
			if (ping == -1) {
				// if we ever see a ping that is out of date, do a server refresh
				// UI_UpdatePendingPings();
			}
			switch (column) {
				case SORT_HOST :
					if (ping <= 0) {
						return Info_ValueForKey(info, "addr");
					} else {
						int gametype = atoi( Info_ValueForKey( info, "gametype" ) );
						//check for password
						if ( atoi(Info_ValueForKey(info, "needpass")) )
						{
							*handle3 = uiInfo.uiDC.Assets.needPass;
						}
						//check for saberonly and restricted force powers
						if ( gametype != GT_JEDIMASTER )
						{
							qboolean saberOnly = qtrue;
							qboolean restrictedForce = qfalse;
							qboolean allForceDisabled = qfalse;
							int wDisable, i = 0;

							//check force
							restrictedForce = atoi(Info_ValueForKey(info, "fdisable"));
							if ( UI_AllForceDisabled( restrictedForce ) )
							{//all force powers are disabled
								allForceDisabled = qtrue;
								*handle2 = uiInfo.uiDC.Assets.noForce;
							}
							else if ( restrictedForce )
							{//at least one force power is disabled
								*handle2 = uiInfo.uiDC.Assets.forceRestrict;
							}

							//check weaps
							wDisable = atoi(Info_ValueForKey(info, "wdisable"));

							while ( i < WP_NUM_WEAPONS )
							{
								if ( !(wDisable & (1 << i)) && i != WP_SABER && i != WP_NONE )
								{
									saberOnly = qfalse;
								}

								i++;
							}
							if ( saberOnly )
							{
								*handle1 = uiInfo.uiDC.Assets.saberOnly;
							}
							else if ( atoi(Info_ValueForKey(info, "truejedi")) != 0 )
							{
								if ( gametype != GT_HOLOCRON
									&& gametype != GT_JEDIMASTER
									&& !saberOnly
									&& !allForceDisabled )
								{//truejedi is on and allowed in this mode
									*handle1 = uiInfo.uiDC.Assets.trueJedi;
								}
							}
						}
						if ( ui_netSource.integer == UIAS_LOCAL ) {
							int nettype = atoi(Info_ValueForKey(info, "nettype"));

							if (nettype < 0 || nettype >= numNetNames) {
								nettype = 0;
							}

							Com_sprintf( hostname, sizeof(hostname), "%s [%s]",
											Info_ValueForKey(info, "hostname"),
											netNames[nettype] );
							return hostname;
						}
						else {
							if (atoi(Info_ValueForKey(info, "sv_allowAnonymous")) != 0) {				// anonymous server
								Com_sprintf( hostname, sizeof(hostname), "(A) %s",
												Info_ValueForKey(info, "hostname"));
							} else {
								Com_sprintf( hostname, sizeof(hostname), "%s",
												Info_ValueForKey(info, "hostname"));
							}
							return hostname;
						}
					}
				case SORT_MAP :
					return Info_ValueForKey(info, "mapname");
				case SORT_CLIENTS :
					Com_sprintf( clientBuff, sizeof(clientBuff), "%s (%s)", Info_ValueForKey(info, "clients"), Info_ValueForKey(info, "sv_maxclients"));
					return clientBuff;
				case SORT_GAME :
					game = atoi(Info_ValueForKey(info, "gametype"));
					if (game >= 0 && game < numGameTypes) {
						Q_strncpyz( needPass, gameTypes[game], sizeof( needPass ) );
					} else {
						if ( ping <= 0 )
							Q_strncpyz( needPass, "Inactive", sizeof( needPass ) );
						Q_strncpyz( needPass, "Unknown", sizeof( needPass ) );
					}

					return needPass;
				case SORT_PING :
					if (ping <= 0) {
						return "...";
					} else {
						return Info_ValueForKey(info, "ping");
					}
			}
		}
	} else if (feederID == FEEDER_SERVERSTATUS) {
		if ( index >= 0 && index < uiInfo.serverStatusInfo.numLines ) {
			if ( column >= 0 && column < 4 ) {
				return uiInfo.serverStatusInfo.lines[index][column];
			}
		}
	} else if (feederID == FEEDER_FINDPLAYER) {
		if ( index >= 0 && index < uiInfo.numFoundPlayerServers ) {
			//return uiInfo.foundPlayerServerAddresses[index];
			return uiInfo.foundPlayerServerNames[index];
		}
	} else if (feederID == FEEDER_PLAYER_LIST) {
		if (index >= 0 && index < uiInfo.playerCount) {
			return uiInfo.playerNames[index];
		}
	} else if (feederID == FEEDER_TEAM_LIST) {
		if (index >= 0 && index < uiInfo.myTeamCount) {
			return uiInfo.teamNames[index];
		}
	} else if (feederID == FEEDER_MODS) {
		if (index >= 0 && index < uiInfo.modCount) {
			if (uiInfo.modList[index].modDescr && *uiInfo.modList[index].modDescr) {
				return uiInfo.modList[index].modDescr;
			} else {
				return uiInfo.modList[index].modName;
			}
		}
	} else if (feederID == FEEDER_CINEMATICS) {
		if (index >= 0 && index < uiInfo.movieCount) {
			return uiInfo.movieList[index];
		}
	} else if (feederID == FEEDER_DEMOS) {
		if (index >= 0 && index < uiInfo.demoCount) {
			return uiInfo.demoList[index];
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
		return 0;
	}
	else if (feederID == FEEDER_COLORCHOICES)
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].ColorCount)
		{
			*handle1 = trap->R_RegisterShaderNoMip( uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Color[index].shader);
			return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Color[index].shader;
		}
	}
	else if (feederID == FEEDER_PLAYER_SKIN_HEAD)
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHeadCount)
		{
			*handle1 = trap->R_RegisterShaderNoMip(va("models/players/%s/icon_%s", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Name, uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHead[index].name));
			return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHead[index].name;
		}
	}
	else if (feederID == FEEDER_PLAYER_SKIN_TORSO)
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorsoCount)
		{
			*handle1 = trap->R_RegisterShaderNoMip(va("models/players/%s/icon_%s", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Name, uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorso[index].name));
			return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorso[index].name;
		}
	}
	else if (feederID == FEEDER_PLAYER_SKIN_LEGS)
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLegCount)
		{
			*handle1 = trap->R_RegisterShaderNoMip(va("models/players/%s/icon_%s", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Name, uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLeg[index].name));
			return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLeg[index].name;
		}
	}
	else if (feederID == FEEDER_SIEGE_BASE_CLASS)
	{
		return "";
	}
	else if (feederID == FEEDER_SIEGE_CLASS_WEAPONS)
	{
		return "";
	}
	return "";
}

static qhandle_t UI_FeederItemImage(float feederID, int index) {
	int	validCnt,i;
	static char info[MAX_STRING_CHARS];

	if (feederID == FEEDER_SABER_SINGLE_INFO)
	{
		return 0;
	}
	else if (feederID == FEEDER_SABER_STAFF_INFO)
	{
		return 0;
	}
	else if (feederID == FEEDER_Q3HEADS)
	{
		int actual = 0;
		UI_SelectedTeamHead(index, &actual);
		index = actual;

		if (index >= 0 && index < uiInfo.q3HeadCount)
		{ //we want it to load them as it draws them, like the TA feeder
		      //return uiInfo.q3HeadIcons[index];
			int selModel = trap->Cvar_VariableValue("ui_selectedModelIndex");

			if (selModel != -1)
			{
				if (uiInfo.q3SelectedHead != selModel)
				{
					uiInfo.q3SelectedHead = selModel;
					//UI_FeederSelection(FEEDER_Q3HEADS, uiInfo.q3SelectedHead);
					Menu_SetFeederSelection(NULL, FEEDER_Q3HEADS, selModel, NULL);
				}
			}

			if (!uiInfo.q3HeadIcons[index])
			{ //this isn't the best way of doing this I guess, but I didn't want a whole seperate string array
			  //for storing shader names. I can't just replace q3HeadNames with the shader name, because we
			  //print what's in q3HeadNames and the icon name would look funny.
				char iconNameFromSkinName[256];
				int i = 0;
				int skinPlace;

				i = strlen(uiInfo.q3HeadNames[index]);

				while (uiInfo.q3HeadNames[index][i] != '/')
				{
					i--;
				}

				i++;
				skinPlace = i; //remember that this is where the skin name begins

				//now, build a full path out of what's in q3HeadNames, into iconNameFromSkinName
				Com_sprintf(iconNameFromSkinName, sizeof(iconNameFromSkinName), "models/players/%s", uiInfo.q3HeadNames[index]);

				i = strlen(iconNameFromSkinName);

				while (iconNameFromSkinName[i] != '/')
				{
					i--;
				}

				i++;
				iconNameFromSkinName[i] = 0; //terminate, and append..
				Q_strcat(iconNameFromSkinName, 256, "icon_");

				//and now, for the final step, append the skin name from q3HeadNames onto the end of iconNameFromSkinName
				i = strlen(iconNameFromSkinName);

				while (uiInfo.q3HeadNames[index][skinPlace])
				{
					iconNameFromSkinName[i] = uiInfo.q3HeadNames[index][skinPlace];
					i++;
					skinPlace++;
				}
				iconNameFromSkinName[i] = 0;

				//and now we are ready to register (thankfully this will only happen once)
				uiInfo.q3HeadIcons[index] = trap->R_RegisterShaderNoMip(iconNameFromSkinName);
			}
			return uiInfo.q3HeadIcons[index];
		}
    }
	else if (feederID == FEEDER_SIEGE_TEAM1)
	{
		if (!siegeTeam1)
		{
			UI_SetSiegeTeams();
			if (!siegeTeam1)
			{
				return 0;
			}
		}

		if (siegeTeam1->classes[index])
		{
			return siegeTeam1->classes[index]->uiPortraitShader;
		}
		return 0;
	}
	else if (feederID == FEEDER_SIEGE_TEAM2)
	{
		if (!siegeTeam2)
		{
			UI_SetSiegeTeams();
			if (!siegeTeam2)
			{
				return 0;
			}
		}

		if (siegeTeam2->classes[index])
		{
			return siegeTeam2->classes[index]->uiPortraitShader;
		}
		return 0;
	}
	else if (feederID == FEEDER_ALLMAPS || feederID == FEEDER_MAPS)
	{
		int actual;
		UI_SelectedMap(index, &actual);
		index = actual;
		if (index >= 0 && index < uiInfo.mapCount) {
			if (uiInfo.mapList[index].levelShot == -1) {
				uiInfo.mapList[index].levelShot = trap->R_RegisterShaderNoMip(uiInfo.mapList[index].imageName);
			}
			return uiInfo.mapList[index].levelShot;
		}
	}
	else if (feederID == FEEDER_PLAYER_SKIN_HEAD)
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHeadCount)
		{
			//return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHeadIcons[index];
			return trap->R_RegisterShaderNoMip(va("models/players/%s/icon_%s", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Name, uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHead[index].name));
		}
	}
	else if (feederID == FEEDER_PLAYER_SKIN_TORSO)
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorsoCount)
		{
			//return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorsoIcons[index];
			return trap->R_RegisterShaderNoMip(va("models/players/%s/icon_%s", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Name, uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorso[index].name));
		}
	}
	else if (feederID == FEEDER_PLAYER_SKIN_LEGS)
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLegCount)
		{
			//return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLegIcons[index];
			return trap->R_RegisterShaderNoMip(va("models/players/%s/icon_%s", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Name, uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLeg[index].name));
		}
	}
	else if (feederID == FEEDER_COLORCHOICES)
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].ColorCount)
		{
			return trap->R_RegisterShaderNoMip( uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Color[index].shader);
		}
	}

	else if ( feederID == FEEDER_SIEGE_BASE_CLASS)
	{
		int team,baseClass;

		team = (int)trap->Cvar_VariableValue("ui_team");
		baseClass = (int)trap->Cvar_VariableValue("ui_siege_class");

		if ((team == SIEGETEAM_TEAM1) ||
			(team == SIEGETEAM_TEAM2))
		{
			// Is it a valid base class?
			if ((baseClass >= SPC_INFANTRY) && (baseClass < SPC_MAX))
			{
				if (index >= 0)
				{
					return(BG_GetUIPortrait(team, baseClass, index));
				}
			}
		}
	}
	else if ( feederID == FEEDER_SIEGE_CLASS_WEAPONS)
	{
		validCnt = 0;
		//count them up
		for (i=0;i< WP_NUM_WEAPONS;i++)
		{
			trap->Cvar_VariableStringBuffer( va("ui_class_weapon%i", i), info, sizeof(info) );
			if (Q_stricmp(info,"gfx/2d/select")!=0)
			{
				if (validCnt == index)
				{
					return(trap->R_RegisterShaderNoMip(info));
				}
				validCnt++;
			}
		}
	}
	else if ( feederID == FEEDER_SIEGE_CLASS_INVENTORY)
	{
		validCnt = 0;
		//count them up
		for (i=0;i< HI_NUM_HOLDABLE;i++)
		{
			trap->Cvar_VariableStringBuffer( va("ui_class_item%i", i), info, sizeof(info) );
			// A hack so health and ammo dispenser icons don't show up.
			if ((Q_stricmp(info,"gfx/2d/select")!=0)
				&& (Q_stricmp(info,"gfx/hud/i_icon_healthdisp")!=0) &&
				(Q_stricmp(info,"gfx/hud/i_icon_ammodisp")!=0))
			{
				if (validCnt == index)
				{
					return(trap->R_RegisterShaderNoMip(info));
				}
				validCnt++;
			}
		}
	}
	else if ( feederID == FEEDER_SIEGE_CLASS_FORCE)
	{
		int slotI=0;
		static char info2[MAX_STRING_CHARS];
		menuDef_t *menu;
		itemDef_t *item;


		validCnt = 0;


		menu = Menu_GetFocused();	// Get current menu
		if (menu)
		{
			item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "base_class_force_feed");
			if (item)
			{
				listBoxDef_t *listPtr = item->typeData.listbox;
				if (listPtr)
				{
					slotI = listPtr->startPos;
				}
			}
		}

		//count them up
		for (i=0;i< NUM_FORCE_POWERS;i++)
		{
			trap->Cvar_VariableStringBuffer( va("ui_class_power%i", i), info, sizeof(info) );
			if (Q_stricmp(info,"gfx/2d/select")!=0)
			{
				if (validCnt == index)
				{
					trap->Cvar_VariableStringBuffer( va("ui_class_powerlevel%i", validCnt), info2, sizeof(info2) );

					trap->Cvar_Set(va("ui_class_powerlevelslot%i", index-slotI), info2);
					return(trap->R_RegisterShaderNoMip(info));
				}
				validCnt++;
			}
		}
	}

  return 0;
}

qboolean UI_FeederSelection(float feederFloat, int index, itemDef_t *item)
{
	static char info[MAX_STRING_CHARS];
	const int feederID = feederFloat;

	if (feederID == FEEDER_Q3HEADS)
	{
		int actual = 0;
		UI_SelectedTeamHead(index, &actual);
		uiInfo.q3SelectedHead = index;
		trap->Cvar_Set("ui_selectedModelIndex", va("%i", index));
		index = actual;
		if (index >= 0 && index < uiInfo.q3HeadCount)
		{
			trap->Cvar_Set( "model", uiInfo.q3HeadNames[index]);	//standard model
			trap->Cvar_Set ( "char_color_red", "255" );			//standard colors
			trap->Cvar_Set ( "char_color_green", "255" );
			trap->Cvar_Set ( "char_color_blue", "255" );
		}
	}
	else if (feederID == FEEDER_MOVES)
	{
		itemDef_t *item;
		menuDef_t *menu;
		modelDef_t *modelPtr;

		menu = Menus_FindByName("rulesMenu_moves");

		if (menu)
		{
			item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "character");
			if (item)
			{
				modelPtr = item->typeData.model;
				if (modelPtr)
				{
					char modelPath[MAX_QPATH];
					int animRunLength;

					ItemParse_model_g2anim_go( item,  datapadMoveData[uiInfo.movesTitleIndex][index].anim );

					Com_sprintf( modelPath, sizeof( modelPath ), "models/players/%s/model.glm", UI_Cvar_VariableString ( "ui_char_model" ) );
					ItemParse_asset_model_go( item, modelPath, &animRunLength );
					UI_UpdateCharacterSkin();

					uiInfo.moveAnimTime = uiInfo.uiDC.realTime + animRunLength;

					if (datapadMoveData[uiInfo.movesTitleIndex][index].anim)
					{

						// Play sound for anim
						if (datapadMoveData[uiInfo.movesTitleIndex][index].sound == MDS_FORCE_JUMP)
						{
							trap->S_StartLocalSound( uiInfo.uiDC.Assets.moveJumpSound, CHAN_LOCAL );
						}
						else if (datapadMoveData[uiInfo.movesTitleIndex][index].sound == MDS_ROLL)
						{
							trap->S_StartLocalSound( uiInfo.uiDC.Assets.moveRollSound, CHAN_LOCAL );
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

							trap->S_StartLocalSound( *soundPtr, CHAN_LOCAL );
						}

						if (datapadMoveData[uiInfo.movesTitleIndex][index].desc)
						{
							trap->Cvar_Set( "ui_move_desc", datapadMoveData[uiInfo.movesTitleIndex][index].desc);
						}
					}
					UI_SaberAttachToChar( item );
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
		menu = Menus_FindByName("rulesMenu_moves");

		if (menu)
		{
			item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "character");
			if (item)
			{
				modelPtr = item->typeData.model;
				if (modelPtr)
				{
					char modelPath[MAX_QPATH];
					int	animRunLength;

					uiInfo.movesBaseAnim = datapadMoveTitleBaseAnims[uiInfo.movesTitleIndex];
					ItemParse_model_g2anim_go( item,  uiInfo.movesBaseAnim );

					Com_sprintf( modelPath, sizeof( modelPath ), "models/players/%s/model.glm", UI_Cvar_VariableString ( "ui_char_model" ) );
					ItemParse_asset_model_go( item, modelPath, &animRunLength );

					UI_UpdateCharacterSkin();

				}
			}
		}
	}
	else if (feederID == FEEDER_SIEGE_TEAM1)
	{
		if (!g_siegedFeederForcedSet)
		{
			g_UIGloballySelectedSiegeClass = UI_SiegeClassNum(siegeTeam1->classes[index]);
			trap->Cvar_Set("ui_classDesc", g_UIClassDescriptions[g_UIGloballySelectedSiegeClass].desc);

			//g_siegedFeederForcedSet = 1;
			//Menu_SetFeederSelection(NULL, FEEDER_SIEGE_TEAM2, -1, NULL);

			UI_SiegeSetCvarsForClass(siegeTeam1->classes[index]);
		}
		g_siegedFeederForcedSet = 0;
	}
	else if (feederID == FEEDER_SIEGE_TEAM2)
	{
		if (!g_siegedFeederForcedSet)
		{
			g_UIGloballySelectedSiegeClass = UI_SiegeClassNum(siegeTeam2->classes[index]);
			trap->Cvar_Set("ui_classDesc", g_UIClassDescriptions[g_UIGloballySelectedSiegeClass].desc);

			//g_siegedFeederForcedSet = 1;
			//Menu_SetFeederSelection(NULL, FEEDER_SIEGE_TEAM2, -1, NULL);

			UI_SiegeSetCvarsForClass(siegeTeam2->classes[index]);
		}
		g_siegedFeederForcedSet = 0;
	}
	else if (feederID == FEEDER_FORCECFG)
	{
		int newindex = index;

		if (uiForceSide == FORCE_LIGHTSIDE)
		{
			newindex += uiInfo.forceConfigLightIndexBegin;
			if (newindex >= uiInfo.forceConfigCount)
			{
				return qfalse;
			}
		}
		else
		{ //else dark
			newindex += uiInfo.forceConfigDarkIndexBegin;
			if (newindex >= uiInfo.forceConfigCount || newindex > uiInfo.forceConfigLightIndexBegin)
			{ //dark gets read in before light
				return qfalse;
			}
		}

		if (index >= 0 && index < uiInfo.forceConfigCount)
		{
				UI_ForceConfigHandle(uiInfo.forceConfigSelected, index);
				uiInfo.forceConfigSelected = index;
		}
	}
	else if (feederID == FEEDER_MAPS || feederID == FEEDER_ALLMAPS)
	{
		int actual, map;
		const char *checkValid = NULL;

		map = (feederID == FEEDER_ALLMAPS) ? ui_currentNetMap.integer : ui_currentMap.integer;
		if (uiInfo.mapList[map].cinematic >= 0) {
		  trap->CIN_StopCinematic(uiInfo.mapList[map].cinematic);
		  uiInfo.mapList[map].cinematic = -1;
		}
		checkValid = UI_SelectedMap(index, &actual);

		if (!checkValid || !checkValid[0])
		{ //this isn't a valid map to select, so reselect the current
			index = ui_mapIndex.integer;
			UI_SelectedMap(index, &actual);
		}

		trap->Cvar_Set("ui_mapIndex", va("%d", index));
		gUISelectedMap = index;
		ui_mapIndex.integer = index;

		if (feederID == FEEDER_MAPS) {
			trap->Cvar_Set("ui_currentMap", va("%d", actual));
			trap->Cvar_Update(&ui_currentMap);
			uiInfo.mapList[ui_currentMap.integer].cinematic = trap->CIN_PlayCinematic(va("%s.roq", uiInfo.mapList[ui_currentMap.integer].mapLoadName), 0, 0, 0, 0, (CIN_loop | CIN_silent) );
			//trap->Cvar_Set("ui_opponentModel", uiInfo.mapList[ui_currentMap.integer].opponentName);
			//updateOpponentModel = qtrue;
		} else {
			trap->Cvar_Set("ui_currentNetMap", va("%d", actual));
			trap->Cvar_Update(&ui_currentNetMap);
			uiInfo.mapList[ui_currentNetMap.integer].cinematic = trap->CIN_PlayCinematic(va("%s.roq", uiInfo.mapList[ui_currentNetMap.integer].mapLoadName), 0, 0, 0, 0, (CIN_loop | CIN_silent) );
		}

	} else if (feederID == FEEDER_SERVERS) {
		const char *mapName = NULL;
		uiInfo.serverStatus.currentServer = index;
		trap->LAN_GetServerInfo(UI_SourceForLAN(), uiInfo.serverStatus.displayServers[index], info, MAX_STRING_CHARS);
		uiInfo.serverStatus.currentServerPreview = trap->R_RegisterShaderNoMip(va("levelshots/%s", Info_ValueForKey(info, "mapname")));
		if (uiInfo.serverStatus.currentServerCinematic >= 0) {
			trap->CIN_StopCinematic(uiInfo.serverStatus.currentServerCinematic);
			uiInfo.serverStatus.currentServerCinematic = -1;
		}
		mapName = Info_ValueForKey(info, "mapname");
		if (mapName && *mapName) {
			uiInfo.serverStatus.currentServerCinematic = trap->CIN_PlayCinematic(va("%s.roq", mapName), 0, 0, 0, 0, (CIN_loop | CIN_silent) );
		}
	} else if (feederID == FEEDER_SERVERSTATUS) {
		//
	} else if (feederID == FEEDER_FINDPLAYER) {
		uiInfo.currentFoundPlayerServer = index;
		//
		if ( index < uiInfo.numFoundPlayerServers-1) {
			// build a new server status for this server
			Q_strncpyz(uiInfo.serverStatusAddress, uiInfo.foundPlayerServerAddresses[uiInfo.currentFoundPlayerServer], sizeof(uiInfo.serverStatusAddress));
			Menu_SetFeederSelection(NULL, FEEDER_SERVERSTATUS, 0, NULL);
			UI_BuildServerStatus(qtrue);
		}
	} else if (feederID == FEEDER_PLAYER_LIST) {
		uiInfo.playerIndex = index;
	} else if (feederID == FEEDER_TEAM_LIST) {
		uiInfo.teamIndex = index;
	} else if (feederID == FEEDER_MODS) {
		uiInfo.modIndex = index;
	} else if (feederID == FEEDER_CINEMATICS) {
		uiInfo.movieIndex = index;
		if (uiInfo.previewMovie >= 0) {
			trap->CIN_StopCinematic(uiInfo.previewMovie);
		}
		uiInfo.previewMovie = -1;
	} else if (feederID == FEEDER_DEMOS) {
		uiInfo.demoIndex = index;
	}
	else if (feederID == FEEDER_COLORCHOICES)
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].ColorCount)
		{
			Item_RunScript(item, uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Color[index].actionText);
		}
	}
	else if (feederID == FEEDER_PLAYER_SKIN_HEAD)
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHeadCount)
		{
			trap->Cvar_Set("ui_char_skin_head", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHead[index].name);
		}
	}
	else if (feederID == FEEDER_PLAYER_SKIN_TORSO)
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorsoCount)
		{
			trap->Cvar_Set("ui_char_skin_torso", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorso[index].name);
		}
	}
	else if (feederID == FEEDER_PLAYER_SKIN_LEGS)
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLegCount)
		{
			trap->Cvar_Set("ui_char_skin_legs", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLeg[index].name);
		}
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
	else if (  feederID == FEEDER_SIEGE_BASE_CLASS )
	{
		int team,baseClass;

		team = (int)trap->Cvar_VariableValue("ui_team");
		baseClass = (int)trap->Cvar_VariableValue("ui_siege_class");

		UI_UpdateCvarsForClass(team, baseClass, index);
	}
	else if (feederID == FEEDER_SIEGE_CLASS_WEAPONS)
	{
//		trap->Cvar_VariableStringBuffer( va("ui_class_weapondesc%i", index), info, sizeof(info) );
//		trap->Cvar_Set( "ui_itemforceinvdesc", info );
	}
	else if (feederID == FEEDER_SIEGE_CLASS_INVENTORY)
	{
//		trap->Cvar_VariableStringBuffer( va("ui_class_itemdesc%i", index), info, sizeof(info) );
//		trap->Cvar_Set( "ui_itemforceinvdesc", info );
	}
	else if (feederID == FEEDER_SIEGE_CLASS_FORCE)
	{
		int i;
//		int validCnt = 0;

		trap->Cvar_VariableStringBuffer( va("ui_class_power%i", index), info, sizeof(info) );

		//count them up
		for (i=0;i< NUM_FORCE_POWERS;i++)
		{
			if (!strcmp(HolocronIcons[i],info))
			{
				trap->Cvar_Set( "ui_itemforceinvdesc", forcepowerDesc[i] );
			}
		}
	}
	return qtrue;
}


static qboolean GameType_Parse(char **p, qboolean join) {
	char *token;

	token = COM_ParseExt((const char **)p, qtrue);

	if (token[0] != '{') {
		return qfalse;
	}

	if (join) {
		uiInfo.numJoinGameTypes = 0;
	} else {
		uiInfo.numGameTypes = 0;
	}

	while ( 1 ) {
		token = COM_ParseExt((const char **)p, qtrue);

		if (Q_stricmp(token, "}") == 0) {
			return qtrue;
		}

		if ( !token || token[0] == 0 ) {
			return qfalse;
		}

		if (token[0] == '{') {
			// two tokens per line, character name and sex
			if (join) {
				if (!String_Parse(p, &uiInfo.joinGameTypes[uiInfo.numJoinGameTypes].gameType) || !Int_Parse(p, &uiInfo.joinGameTypes[uiInfo.numJoinGameTypes].gtEnum)) {
					return qfalse;
				}
			} else {
				if (!String_Parse(p, &uiInfo.gameTypes[uiInfo.numGameTypes].gameType) || !Int_Parse(p, &uiInfo.gameTypes[uiInfo.numGameTypes].gtEnum)) {
					return qfalse;
				}
			}

			if (join) {
				if (uiInfo.numJoinGameTypes < MAX_GAMETYPES) {
					uiInfo.numJoinGameTypes++;
				} else {
					Com_Printf("Too many net game types, last one replace!\n");
				}
			} else {
				if (uiInfo.numGameTypes < MAX_GAMETYPES) {
					uiInfo.numGameTypes++;
				} else {
					Com_Printf("Too many game types, last one replace!\n");
				}
			}

			token = COM_ParseExt((const char **)p, qtrue);
			if (token[0] != '}') {
				return qfalse;
			}
		}
	}
	return qfalse;
}

static qboolean MapList_Parse(char **p) {
	char *token;

	token = COM_ParseExt((const char **)p, qtrue);

	if (token[0] != '{') {
		return qfalse;
	}

	uiInfo.mapCount = 0;

	while ( 1 ) {
		token = COM_ParseExt((const char **)p, qtrue);

		if (Q_stricmp(token, "}") == 0) {
			return qtrue;
		}

		if ( !token || token[0] == 0 ) {
			return qfalse;
		}

		if (token[0] == '{') {
			if (!String_Parse(p, &uiInfo.mapList[uiInfo.mapCount].mapName) || !String_Parse(p, &uiInfo.mapList[uiInfo.mapCount].mapLoadName)
				||!Int_Parse(p, &uiInfo.mapList[uiInfo.mapCount].teamMembers) ) {
				return qfalse;
			}

			if (!String_Parse(p, &uiInfo.mapList[uiInfo.mapCount].opponentName)) {
				return qfalse;
			}

			uiInfo.mapList[uiInfo.mapCount].typeBits = 0;

			while (1) {
				token = COM_ParseExt((const char **)p, qtrue);
				if (token[0] >= '0' && token[0] <= '9') {
					uiInfo.mapList[uiInfo.mapCount].typeBits |= (1 << (token[0] - 0x030));
				} else {
					break;
				}
			}

			//mapList[mapCount].imageName = String_Alloc(va("levelshots/%s", mapList[mapCount].mapLoadName));
			//if (uiInfo.mapCount == 0) {
			  // only load the first cinematic, selection loads the others
  			//  uiInfo.mapList[uiInfo.mapCount].cinematic = trap->CIN_PlayCinematic(va("%s.roq",uiInfo.mapList[uiInfo.mapCount].mapLoadName), qfalse, qfalse, qtrue, 0, 0, 0, 0);
			//}
  		uiInfo.mapList[uiInfo.mapCount].cinematic = -1;
			uiInfo.mapList[uiInfo.mapCount].levelShot = trap->R_RegisterShaderNoMip(va("levelshots/%s_small", uiInfo.mapList[uiInfo.mapCount].mapLoadName));

			if (uiInfo.mapCount < MAX_MAPS) {
				uiInfo.mapCount++;
			} else {
				Com_Printf("Too many maps, last one replaced!\n");
			}
		}
	}
	return qfalse;
}

static void UI_ParseGameInfo(const char *teamFile) {
	char	*token;
	char *p;
	char *buff = NULL;
	//int mode = 0; TTimo: unused

	buff = GetMenuBuffer(teamFile);
	if (!buff) {
		return;
	}

	p = buff;

	COM_BeginParseSession ("UI_ParseGameInfo");

	while ( 1 ) {
		token = COM_ParseExt( (const char **)(&p), qtrue );
		if( !token || token[0] == 0 || token[0] == '}') {
			break;
		}

		if ( Q_stricmp( token, "}" ) == 0 ) {
			break;
		}

		if (Q_stricmp(token, "gametypes") == 0) {

			if (GameType_Parse(&p, qfalse)) {
				continue;
			} else {
				break;
			}
		}

		if (Q_stricmp(token, "joingametypes") == 0) {

			if (GameType_Parse(&p, qtrue)) {
				continue;
			} else {
				break;
			}
		}

		if (Q_stricmp(token, "maps") == 0) {
			// start a new menu
			MapList_Parse(&p);
		}

	}
}

static void UI_Pause(qboolean b) {
	if (b) {
		// pause the game and set the ui keycatcher
		trap->Cvar_Set( "cl_paused", "1" );
		trap->Key_SetCatcher( KEYCATCH_UI );
	} else {
		// unpause the game and clear the ui keycatcher
		trap->Key_SetCatcher( trap->Key_GetCatcher() & ~KEYCATCH_UI );
		trap->Key_ClearStates();
		trap->Cvar_Set( "cl_paused", "0" );
	}
}

static int UI_PlayCinematic(const char *name, float x, float y, float w, float h) {
	return trap->CIN_PlayCinematic(name, x, y, w, h, (CIN_loop | CIN_silent));
}

static void UI_StopCinematic(int handle) {
	if (handle >= 0) {
		trap->CIN_StopCinematic(handle);
	} else {
		handle = abs(handle);
		if (handle == UI_MAPCINEMATIC) {
			if (uiInfo.mapList[ui_currentMap.integer].cinematic >= 0) {
				trap->CIN_StopCinematic(uiInfo.mapList[ui_currentMap.integer].cinematic);
				uiInfo.mapList[ui_currentMap.integer].cinematic = -1;
			}
		} else if (handle == UI_NETMAPCINEMATIC) {
			if (uiInfo.serverStatus.currentServerCinematic >= 0) {
				trap->CIN_StopCinematic(uiInfo.serverStatus.currentServerCinematic);
				uiInfo.serverStatus.currentServerCinematic = -1;
			}
		} else if (handle == UI_CLANCINEMATIC) {
			int i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_teamName"));
			if (i >= 0 && i < uiInfo.teamCount) {
				if (uiInfo.teamList[i].cinematic >= 0) {
					trap->CIN_StopCinematic(uiInfo.teamList[i].cinematic);
					uiInfo.teamList[i].cinematic = -1;
				}
			}
		}
	}
}

static void UI_DrawCinematic(int handle, float x, float y, float w, float h) {
	trap->CIN_SetExtents(handle, x, y, w, h);
	trap->CIN_DrawCinematic(handle);
}

static void UI_RunCinematicFrame(int handle) {
	trap->CIN_RunCinematic(handle);
}


/*
=================
UI_LoadForceConfig_List
=================
Looks in the directory for force config files (.fcf) and loads the name in
*/
void UI_LoadForceConfig_List( void )
{
	int			numfiles = 0;
	char		filelist[2048];
	char		configname[128];
	char		*fileptr = NULL;
	int			j = 0;
	int			filelen = 0;
	qboolean	lightSearch = qfalse;

	uiInfo.forceConfigCount = 0;
	Com_sprintf( uiInfo.forceConfigNames[uiInfo.forceConfigCount], sizeof(uiInfo.forceConfigNames[uiInfo.forceConfigCount]), "Custom");
	uiInfo.forceConfigCount++;
	//Always reserve index 0 as the "custom" config

nextSearch:
	if (lightSearch)
	{ //search light side folder
		numfiles = trap->FS_GetFileList("forcecfg/light", "fcf", filelist, 2048 );
		uiInfo.forceConfigLightIndexBegin = uiInfo.forceConfigCount-1;
	}
	else
	{ //search dark side folder
		numfiles = trap->FS_GetFileList("forcecfg/dark", "fcf", filelist, 2048 );
		uiInfo.forceConfigDarkIndexBegin = uiInfo.forceConfigCount-1;
	}

	fileptr = filelist;

	for (j=0; j<numfiles && uiInfo.forceConfigCount < MAX_FORCE_CONFIGS;j++,fileptr+=filelen+1)
	{
		filelen = strlen(fileptr);
		COM_StripExtension(fileptr, configname, sizeof( configname ) );

		if (lightSearch)
		{
			uiInfo.forceConfigSide[uiInfo.forceConfigCount] = qtrue; //light side config
		}
		else
		{
			uiInfo.forceConfigSide[uiInfo.forceConfigCount] = qfalse; //dark side config
		}

		Com_sprintf( uiInfo.forceConfigNames[uiInfo.forceConfigCount], sizeof(uiInfo.forceConfigNames[uiInfo.forceConfigCount]), configname);
		uiInfo.forceConfigCount++;
	}

	if (!lightSearch)
	{
		lightSearch = qtrue;
		goto nextSearch;
	}
}


/*
=================
bIsImageFile
builds path and scans for valid image extentions
=================
*/
static qboolean bIsImageFile(const char* dirptr, const char* skinname)
{
	char fpath[MAX_QPATH];
	int f;

	Com_sprintf(fpath, MAX_QPATH, "models/players/%s/icon_%s.jpg", dirptr, skinname);
	trap->FS_Open(fpath, &f, FS_READ);
	if (!f)
	{ //not there, try png
		Com_sprintf(fpath, MAX_QPATH, "models/players/%s/icon_%s.png", dirptr, skinname);
		trap->FS_Open(fpath, &f, FS_READ);
	}
	if (!f)
	{ //not there, try tga
		Com_sprintf(fpath, MAX_QPATH, "models/players/%s/icon_%s.tga", dirptr, skinname);
		trap->FS_Open(fpath, &f, FS_READ);
	}
	if (f)
	{
		trap->FS_Close(f);
		return qtrue;
	}

	return qfalse;
}


/*
=================
PlayerModel_BuildList
=================
*/
static void UI_BuildQ3Model_List( void )
{
	int		numdirs;
	int		numfiles;
	char	dirlist[2048];
	char	filelist[2048];
	char	skinname[64];
	char*	dirptr;
	char*	fileptr;
	char*	check;
	int		i;
	int		j, k, p, s;
	int		dirlen;
	int		filelen;

	uiInfo.q3HeadCount = 0;

	// iterate directory of all player models
	numdirs = trap->FS_GetFileList("models/players", "/", dirlist, 2048 );
	dirptr  = dirlist;
	for (i=0; i<numdirs && uiInfo.q3HeadCount < MAX_Q3PLAYERMODELS; i++,dirptr+=dirlen+1)
	{
		dirlen = strlen(dirptr);

		if (dirlen && dirptr[dirlen-1]=='/') dirptr[dirlen-1]='\0';

		if (!strcmp(dirptr,".") || !strcmp(dirptr,".."))
			continue;


		numfiles = trap->FS_GetFileList( va("models/players/%s",dirptr), "skin", filelist, 2048 );
		fileptr  = filelist;
		for (j=0; j<numfiles && uiInfo.q3HeadCount < MAX_Q3PLAYERMODELS;j++,fileptr+=filelen+1)
		{
			int skinLen = 0;

			filelen = strlen(fileptr);

			COM_StripExtension(fileptr,skinname, sizeof( skinname ) );

			skinLen = strlen(skinname);
			k = 0;
			while (k < skinLen && skinname[k] && skinname[k] != '_')
			{
				k++;
			}
			if (skinname[k] == '_')
			{
				p = 0;

				while (skinname[k])
				{
					skinname[p] = skinname[k];
					k++;
					p++;
				}
				skinname[p] = '\0';
			}

			/*
			Com_sprintf(fpath, 2048, "models/players/%s/icon%s.jpg", dirptr, skinname);

			trap->FS_Open(fpath, &f, FS_READ);

			if (f)
			*/
			check = &skinname[1];
			if (bIsImageFile(dirptr, check))
			{ //if it exists
				qboolean iconExists = qfalse;

				//trap->FS_Close(f);

				if (skinname[0] == '_')
				{ //change character to append properly
					skinname[0] = '/';
				}

				s = 0;

				while (s < uiInfo.q3HeadCount)
				{ //check for dupes
					if (!Q_stricmp(va("%s%s", dirptr, skinname), uiInfo.q3HeadNames[s]))
					{
						iconExists = qtrue;
						break;
					}
					s++;
				}

				if (iconExists)
				{
					continue;
				}

				Com_sprintf( uiInfo.q3HeadNames[uiInfo.q3HeadCount], sizeof(uiInfo.q3HeadNames[uiInfo.q3HeadCount]), va("%s%s", dirptr, skinname));
				uiInfo.q3HeadIcons[uiInfo.q3HeadCount++] = 0;//trap->R_RegisterShaderNoMip(fpath);
				//rww - we are now registering them as they are drawn like the TA feeder, so as to decrease UI load time.
			}

			if (uiInfo.q3HeadCount >= MAX_Q3PLAYERMODELS)
			{
				return;
			}
		}
	}

}

void UI_SiegeInit(void)
{
	//Load the player class types
	BG_SiegeLoadClasses(g_UIClassDescriptions);

	if (!bgNumSiegeClasses)
	{ //We didn't find any?!
		Com_Error(ERR_DROP, "Couldn't find any player classes for Siege");
	}

	//Now load the teams since we have class data.
	BG_SiegeLoadTeams();

	if (!bgNumSiegeTeams)
	{ //React same as with classes.
		Com_Error(ERR_DROP, "Couldn't find any player teams for Siege");
	}
}

/*
=================
UI_ParseColorData
=================
*/
//static qboolean UI_ParseColorData(char* buf, playerSpeciesInfo_t &species)
static qboolean UI_ParseColorData(char* buf, playerSpeciesInfo_t *species,char*	file)
{
	const char	*token;
	const char	*p;

	p = buf;
	COM_BeginParseSession(file);
	species->ColorCount = 0;
	species->ColorMax = 16;
	species->Color = (playerColor_t *)malloc(species->ColorMax * sizeof(playerColor_t));

	while ( p )
	{
		token = COM_ParseExt( &p, qtrue );	//looking for the shader
		if ( token[0] == 0 )
		{
			return species->ColorCount;
		}
		if (species->ColorCount >= species->ColorMax)
		{
			species->ColorMax *= 2;
			species->Color = (playerColor_t *)realloc(species->Color, species->ColorMax * sizeof(playerColor_t));
		}

		memset(&species->Color[species->ColorCount], 0, sizeof(playerColor_t));

		Q_strncpyz( species->Color[species->ColorCount].shader, token, MAX_QPATH );

		token = COM_ParseExt( &p, qtrue );	//looking for action block {
		if ( token[0] != '{' )
		{
			return qfalse;
		}

		token = COM_ParseExt( &p, qtrue );	//looking for action commands
		while (token[0] != '}')
		{
			if ( token[0] == 0)
			{	//EOF
				return qfalse;
			}
			Q_strcat(species->Color[species->ColorCount].actionText, ACTION_BUFFER_SIZE, token);
			Q_strcat(species->Color[species->ColorCount].actionText, ACTION_BUFFER_SIZE, " ");
			token = COM_ParseExt( &p, qtrue );	//looking for action commands or final }
		}
		species->ColorCount++;	//next color please
	}
	return qtrue;//never get here
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
}

/*
=================
UI_BuildPlayerModel_List
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
	int		j;

	dirlist = malloc(DIR_LIST_SIZE);
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
	numdirs = trap->FS_GetFileList("models/players", "/", dirlist, dirListSize );
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
		filelen = trap->FS_Open(fpath, &f, FS_READ);

		if (f)
		{
			char	filelist[2048];
			playerSpeciesInfo_t *species;
			char                 skinname[64];
			int                  numfiles;
			int                  iSkinParts=0;
			char                *buffer = NULL;

			buffer = malloc(filelen + 1);
			if(!buffer)
			{
				trap->FS_Close( f );
				Com_Error(ERR_FATAL, "Could not allocate buffer to read %s", fpath);
			}

			trap->FS_Read(buffer, filelen, f);
			trap->FS_Close(f);

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

			if (!UI_ParseColorData(buffer,species,fpath))
			{
				Com_Printf(S_COLOR_RED"UI_BuildPlayerModel_List: Errors parsing '%s'\n", fpath);
			}

			species->SkinHeadMax = 8;
			species->SkinTorsoMax = 8;
			species->SkinLegMax = 8;

			species->SkinHead = (skinName_t *)malloc(species->SkinHeadMax * sizeof(skinName_t));
			species->SkinTorso = (skinName_t *)malloc(species->SkinTorsoMax * sizeof(skinName_t));
			species->SkinLeg = (skinName_t *)malloc(species->SkinLegMax * sizeof(skinName_t));

			free(buffer);

			numfiles = trap->FS_GetFileList( va("models/players/%s",dirptr), ".skin", filelist, sizeof(filelist) );
			fileptr  = filelist;
			for (j=0; j<numfiles; j++,fileptr+=filelen+1)
			{
				if (trap->Cvar_VariableValue("fs_copyfiles") > 0 )
				{
					trap->FS_Open(va("models/players/%s/%s",dirptr,fileptr), &f, FS_READ);
					if (f)
						trap->FS_Close(f);
				}

				filelen = strlen(fileptr);
				COM_StripExtension(fileptr,skinname,sizeof(skinname));

				if (bIsImageFile(dirptr, skinname))
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
				int g2Model;
				void *ghoul2 = 0;
				Com_sprintf( fpath, sizeof( fpath ), "models/players/%s/model.glm", dirptr );
				g2Model = trap->G2API_InitGhoul2Model(&ghoul2, fpath, 0, 0, 0, 0, 0);
				if (g2Model >= 0)
				{
//					trap->G2API_RemoveGhoul2Model( &ghoul2, 0 );
					trap->G2API_CleanGhoul2Models (&ghoul2);
				}
			}
		}
	}

	if ( dirlist != stackDirList )
	{
		free(dirlist);
	}
}

static qhandle_t UI_RegisterShaderNoMip( const char *name ) {
	if ( *name == '*' ) {
		char buf[MAX_CVAR_VALUE_STRING];

		trap->Cvar_VariableStringBuffer( name+1, buf, sizeof( buf ) );

		if ( buf[0] )
			return trap->R_RegisterShaderNoMip( buf );
	}

	return trap->R_RegisterShaderNoMip( name );
}

/*
=================
UI_Init
=================
*/
void UI_Init( qboolean inGameLoad ) {
	const char *menuSet;

	// Get the list of possible languages
	uiInfo.languageCount = trap->SE_GetNumLanguages();	// this does a dir scan, so use carefully

	uiInfo.inGameLoad = inGameLoad;

	//initialize all these cvars to "0"
	UI_SiegeSetCvarsForClass( NULL );

	UI_SiegeInit();

	UI_UpdateForcePowers();

	UI_RegisterCvars();
	UI_InitMemory();

	// cache redundant calulations
	trap->GetGlconfig( &uiInfo.uiDC.glconfig );

	// for 640x480 virtualized screen
	uiInfo.uiDC.yscale = uiInfo.uiDC.glconfig.vidHeight * (1.0/480.0);
	uiInfo.uiDC.xscale = uiInfo.uiDC.glconfig.vidWidth * (1.0/640.0);
	if ( uiInfo.uiDC.glconfig.vidWidth * 480 > uiInfo.uiDC.glconfig.vidHeight * 640 ) {
		// wide screen
		uiInfo.uiDC.bias = 0.5 * ( uiInfo.uiDC.glconfig.vidWidth - ( uiInfo.uiDC.glconfig.vidHeight * (640.0/480.0) ) );
	}
	else {
		// no wide screen
		uiInfo.uiDC.bias = 0;
	}

	//UI_Load();
	uiInfo.uiDC.registerShaderNoMip				= UI_RegisterShaderNoMip;
	uiInfo.uiDC.setColor						= &UI_SetColor;
	uiInfo.uiDC.drawHandlePic					= &UI_DrawHandlePic;
	uiInfo.uiDC.drawStretchPic					= trap->R_DrawStretchPic;
	uiInfo.uiDC.drawText						= &Text_Paint;
	uiInfo.uiDC.textWidth						= &Text_Width;
	uiInfo.uiDC.textHeight						= &Text_Height;
	uiInfo.uiDC.registerModel					= trap->R_RegisterModel;
	uiInfo.uiDC.modelBounds						= trap->R_ModelBounds;
	uiInfo.uiDC.fillRect						= &UI_FillRect;
	uiInfo.uiDC.drawRect						= &_UI_DrawRect;
	uiInfo.uiDC.drawSides						= &_UI_DrawSides;
	uiInfo.uiDC.drawTopBottom					= &_UI_DrawTopBottom;
	uiInfo.uiDC.clearScene						= trap->R_ClearScene;
	uiInfo.uiDC.drawSides						= &_UI_DrawSides;
	uiInfo.uiDC.addRefEntityToScene				= trap->R_AddRefEntityToScene;
	uiInfo.uiDC.renderScene						= trap->R_RenderScene;
	uiInfo.uiDC.RegisterFont					= trap->R_RegisterFont;
	uiInfo.uiDC.Font_StrLenPixels				= trap->R_Font_StrLenPixels;
	uiInfo.uiDC.Font_StrLenChars				= trap->R_Font_StrLenChars;
	uiInfo.uiDC.Font_HeightPixels				= trap->R_Font_HeightPixels;
	uiInfo.uiDC.Font_DrawString					= trap->R_Font_DrawString;
	uiInfo.uiDC.Language_IsAsian				= trap->R_Language_IsAsian;
	uiInfo.uiDC.Language_UsesSpaces				= trap->R_Language_UsesSpaces;
	uiInfo.uiDC.AnyLanguage_ReadCharFromString	= trap->R_AnyLanguage_ReadCharFromString;
	uiInfo.uiDC.ownerDrawItem					= &UI_OwnerDraw;
	uiInfo.uiDC.getValue						= &UI_GetValue;
	uiInfo.uiDC.ownerDrawVisible				= &UI_OwnerDrawVisible;
	uiInfo.uiDC.runScript						= &UI_RunMenuScript;
	uiInfo.uiDC.deferScript						= &UI_DeferMenuScript;
	uiInfo.uiDC.getTeamColor					= &UI_GetTeamColor;
	uiInfo.uiDC.setCVar							= trap->Cvar_Set;
	uiInfo.uiDC.getCVarString					= trap->Cvar_VariableStringBuffer;
	uiInfo.uiDC.getCVarValue					= trap->Cvar_VariableValue;
	uiInfo.uiDC.drawTextWithCursor				= &Text_PaintWithCursor;
	uiInfo.uiDC.setOverstrikeMode				= trap->Key_SetOverstrikeMode;
	uiInfo.uiDC.getOverstrikeMode				= trap->Key_GetOverstrikeMode;
	uiInfo.uiDC.startLocalSound					= trap->S_StartLocalSound;
	uiInfo.uiDC.ownerDrawHandleKey				= &UI_OwnerDrawHandleKey;
	uiInfo.uiDC.feederCount						= &UI_FeederCount;
	uiInfo.uiDC.feederItemImage					= &UI_FeederItemImage;
	uiInfo.uiDC.feederItemText					= &UI_FeederItemText;
	uiInfo.uiDC.feederSelection					= &UI_FeederSelection;
	uiInfo.uiDC.setBinding						= trap->Key_SetBinding;
	uiInfo.uiDC.getBindingBuf					= trap->Key_GetBindingBuf;
	uiInfo.uiDC.keynumToStringBuf				= trap->Key_KeynumToStringBuf;
	uiInfo.uiDC.executeText						= trap->Cmd_ExecuteText;
	uiInfo.uiDC.Error							= Com_Error;
	uiInfo.uiDC.Print							= Com_Printf;
	uiInfo.uiDC.Pause							= &UI_Pause;
	uiInfo.uiDC.ownerDrawWidth					= &UI_OwnerDrawWidth;
	uiInfo.uiDC.registerSound					= trap->S_RegisterSound;
	uiInfo.uiDC.startBackgroundTrack			= trap->S_StartBackgroundTrack;
	uiInfo.uiDC.stopBackgroundTrack				= trap->S_StopBackgroundTrack;
	uiInfo.uiDC.playCinematic					= &UI_PlayCinematic;
	uiInfo.uiDC.stopCinematic					= &UI_StopCinematic;
	uiInfo.uiDC.drawCinematic					= &UI_DrawCinematic;
	uiInfo.uiDC.runCinematicFrame				= &UI_RunCinematicFrame;
	uiInfo.uiDC.ext.Font_StrLenPixels			= trap->ext.R_Font_StrLenPixels;

	Init_Display(&uiInfo.uiDC);

	UI_BuildPlayerModel_List(inGameLoad);

	String_Init();

	uiInfo.uiDC.cursor	= trap->R_RegisterShaderNoMip( "menu/art/3_cursor2" );
	uiInfo.uiDC.whiteShader = trap->R_RegisterShaderNoMip( "white" );

	AssetCache();

	uiInfo.teamCount = 0;
	uiInfo.characterCount = 0;
	uiInfo.aliasCount = 0;

	UI_ParseGameInfo("ui/jamp/gameinfo.txt");

	menuSet = UI_Cvar_VariableString("ui_menuFilesMP");
	if (menuSet == NULL || menuSet[0] == '\0') {
		menuSet = "ui/jampmenus.txt";
	}

#if 1
	if (inGameLoad)
	{
		UI_LoadMenus("ui/jampingame.txt", qtrue);
	}
	else if (!ui_bypassMainMenuLoad.integer)
	{
		UI_LoadMenus(menuSet, qtrue);
	}
#else //this was adding quite a giant amount of time to the load time
	UI_LoadMenus(menuSet, qtrue);
	UI_LoadMenus("ui/jampingame.txt", qtrue);
#endif

	{
		char buf[MAX_NETNAME] = {0};
		Q_strncpyz( buf, UI_Cvar_VariableString( "name" ), sizeof( buf ) );
		trap->Cvar_Register( NULL, "ui_Name", buf, CVAR_INTERNAL );
	}

	Menus_CloseAll();

	trap->LAN_LoadCachedServers();

	UI_BuildQ3Model_List();
	UI_LoadBots();

	UI_LoadForceConfig_List();

	UI_InitForceShaders();

	// sets defaults for ui temp cvars
	uiInfo.currentCrosshair = (int)trap->Cvar_VariableValue("cg_drawCrosshair");
	trap->Cvar_Set("ui_mousePitch", (trap->Cvar_VariableValue("m_pitch") >= 0) ? "0" : "1");
	trap->Cvar_Set("ui_mousePitchVeh", (trap->Cvar_VariableValue("m_pitchVeh") >= 0) ? "0" : "1");

	uiInfo.serverStatus.currentServerCinematic = -1;
	uiInfo.previewMovie = -1;

	trap->Cvar_Register(NULL, "debug_protocol", "", 0 );

	trap->Cvar_Set("ui_actualNetGameType", va("%d", ui_netGametype.integer));
	trap->Cvar_Update(&ui_actualNetGametype);
}

#define	UI_FPS_FRAMES	4
void UI_Refresh( int realtime )
{
	static int index;
	static int	previousTimes[UI_FPS_FRAMES];

	//if ( !( trap->Key_GetCatcher() & KEYCATCH_UI ) ) {
	//	return;
	//}

	trap->G2API_SetTime(realtime, 0);
	trap->G2API_SetTime(realtime, 1);
	//ghoul2 timer must be explicitly updated during ui rendering.

	uiInfo.uiDC.frameTime = realtime - uiInfo.uiDC.realTime;
	uiInfo.uiDC.realTime = realtime;

	previousTimes[index % UI_FPS_FRAMES] = uiInfo.uiDC.frameTime;
	index++;
	if ( index > UI_FPS_FRAMES ) {
		int i, total;
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for ( i = 0 ; i < UI_FPS_FRAMES ; i++ ) {
			total += previousTimes[i];
		}
		if ( !total ) {
			total = 1;
		}
		uiInfo.uiDC.FPS = 1000 * UI_FPS_FRAMES / total;
	}

	UI_UpdateCvars();

	if (Menu_Count() > 0) {
		// paint all the menus
		Menu_PaintAll();
		// refresh server browser list
		UI_DoServerRefresh();
		// refresh server status
		UI_BuildServerStatus(qfalse);
		// refresh find player list
		UI_BuildFindPlayerList(qfalse);
	}
	// draw cursor
	UI_SetColor( NULL );
	if (Menu_Count() > 0 && (trap->Key_GetCatcher() & KEYCATCH_UI)) {
		UI_DrawHandlePic( (float)uiInfo.uiDC.cursorx, (float)uiInfo.uiDC.cursory, 40.0f, 40.0f, uiInfo.uiDC.Assets.cursor);
		//UI_DrawHandlePic( uiInfo.uiDC.cursorx, uiInfo.uiDC.cursory, 48, 48, uiInfo.uiDC.Assets.cursor);
	}

	if (ui_rankChange.integer)
	{
		FPMessageTime = realtime + 3000;

		if (!parsedFPMessage[0] /*&& uiMaxRank > ui_rankChange.integer*/)
		{
			const char *printMessage = UI_GetStringEdString("MP_INGAME", "SET_NEW_RANK");

			int i = 0;
			int p = 0;
			int linecount = 0;

			while (printMessage[i] && p < 1024)
			{
				parsedFPMessage[p] = printMessage[i];
				p++;
				i++;
				linecount++;

				if (linecount > 64 && printMessage[i] == ' ')
				{
					parsedFPMessage[p] = '\n';
					p++;
					linecount = 0;
				}
			}
			parsedFPMessage[p] = '\0';
		}

		//if (uiMaxRank > ui_rankChange.integer)
		{
			uiMaxRank = ui_rankChange.integer;
			uiForceRank = uiMaxRank;

			/*
			while (x < NUM_FORCE_POWERS)
			{
				//For now just go ahead and clear force powers upon rank change
				uiForcePowersRank[x] = 0;
				x++;
			}
			uiForcePowersRank[FP_LEVITATION] = 1;
			uiForceUsed = 0;
			*/

			//Use BG_LegalizedForcePowers and transfer the result into the UI force settings
			UI_ReadLegalForce();
		}

		if (ui_freeSaber.integer && uiForcePowersRank[FP_SABER_OFFENSE] < 1)
		{
			uiForcePowersRank[FP_SABER_OFFENSE] = 1;
		}
		if (ui_freeSaber.integer && uiForcePowersRank[FP_SABER_DEFENSE] < 1)
		{
			uiForcePowersRank[FP_SABER_DEFENSE] = 1;
		}
		trap->Cvar_Set("ui_rankChange", "0");

		//remember to update the force power count after changing the max rank
		UpdateForceUsed();
	}

	if (ui_freeSaber.integer)
	{
		bgForcePowerCost[FP_SABER_OFFENSE][FORCE_LEVEL_1] = 0;
		bgForcePowerCost[FP_SABER_DEFENSE][FORCE_LEVEL_1] = 0;
	}
	else
	{
		bgForcePowerCost[FP_SABER_OFFENSE][FORCE_LEVEL_1] = 1;
		bgForcePowerCost[FP_SABER_DEFENSE][FORCE_LEVEL_1] = 1;
	}

	/*
	if (parsedFPMessage[0] && FPMessageTime > realtime)
	{
		vec4_t txtCol;
		int txtStyle = ITEM_TEXTSTYLE_SHADOWED;

		if ((FPMessageTime - realtime) < 2000)
		{
			txtCol[0] = colorWhite[0];
			txtCol[1] = colorWhite[1];
			txtCol[2] = colorWhite[2];
			txtCol[3] = (((float)FPMessageTime - (float)realtime)/2000);

			txtStyle = 0;
		}
		else
		{
			txtCol[0] = colorWhite[0];
			txtCol[1] = colorWhite[1];
			txtCol[2] = colorWhite[2];
			txtCol[3] = colorWhite[3];
		}

		Text_Paint(10, 0, 1, txtCol, parsedFPMessage, 0, 1024, txtStyle, FONT_MEDIUM);
	}
	*/
	//For now, don't bother.
}

/*
=================
UI_KeyEvent
=================
*/
void UI_KeyEvent( int key, qboolean down ) {
	if (Menu_Count() > 0) {
		menuDef_t *menu = Menu_GetFocused();
		if (menu) {
			if (key == A_ESCAPE && down && !Menus_AnyFullScreenVisible()) {
				Menus_CloseAll();
			} else {
				Menu_HandleKey(menu, key, down );
			}
		} else {
			trap->Key_SetCatcher( trap->Key_GetCatcher() & ~KEYCATCH_UI );
			trap->Key_ClearStates();
			trap->Cvar_Set( "cl_paused", "0" );
		}
	}

  //if ((s > 0) && (s != menu_null_sound)) {
	//  trap->S_StartLocalSound( s, CHAN_LOCAL_SOUND );
  //}
}

/*
=================
UI_MouseEvent
=================
*/
void UI_MouseEvent( int dx, int dy )
{
	// update mouse screen position
	uiInfo.uiDC.cursorx += dx;
	if (uiInfo.uiDC.cursorx < 0)
		uiInfo.uiDC.cursorx = 0;
	else if (uiInfo.uiDC.cursorx > SCREEN_WIDTH)
		uiInfo.uiDC.cursorx = SCREEN_WIDTH;

	uiInfo.uiDC.cursory += dy;
	if (uiInfo.uiDC.cursory < 0)
		uiInfo.uiDC.cursory = 0;
	else if (uiInfo.uiDC.cursory > SCREEN_HEIGHT)
		uiInfo.uiDC.cursory = SCREEN_HEIGHT;

	if (Menu_Count() > 0) {
		//menuDef_t *menu = Menu_GetFocused();
		//Menu_HandleMouseMove(menu, uiInfo.uiDC.cursorx, uiInfo.uiDC.cursory);
		Display_MouseMove(NULL, uiInfo.uiDC.cursorx, uiInfo.uiDC.cursory);
	}
}

static void UI_ReadableSize ( char *buf, int bufsize, int value )
{
	if (value > 1024*1024*1024 ) { // gigs
		Com_sprintf( buf, bufsize, "%d", value / (1024*1024*1024) );
		Com_sprintf( buf+strlen(buf), bufsize-strlen(buf), ".%02d GB",
			(value % (1024*1024*1024))*100 / (1024*1024*1024) );
	} else if (value > 1024*1024 ) { // megs
		Com_sprintf( buf, bufsize, "%d", value / (1024*1024) );
		Com_sprintf( buf+strlen(buf), bufsize-strlen(buf), ".%02d MB",
			(value % (1024*1024))*100 / (1024*1024) );
	} else if (value > 1024 ) { // kilos
		Com_sprintf( buf, bufsize, "%d KB", value / 1024 );
	} else { // bytes
		Com_sprintf( buf, bufsize, "%d bytes", value );
	}
}

// Assumes time is in msec
static void UI_PrintTime ( char *buf, int bufsize, int time ) {
	time /= 1000;  // change to seconds

	if (time > 3600) { // in the hours range
		Com_sprintf( buf, bufsize, "%d hr %2d min", time / 3600, (time % 3600) / 60 );
	} else if (time > 60) { // mins
		Com_sprintf( buf, bufsize, "%2d min %2d sec", time / 60, time % 60 );
	} else { // secs
		Com_sprintf( buf, bufsize, "%2d sec", time );
	}
}

void Text_PaintCenter(float x, float y, float scale, vec4_t color, const char *text, float adjust, int iMenuFont) {
	int len = Text_Width(text, scale, iMenuFont);
	Text_Paint(x - len / 2, y, scale, color, text, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, iMenuFont);
}

static void UI_DisplayDownloadInfo( const char *downloadName, float centerPoint, float yStart, float scale, int iMenuFont) {
	char sDownLoading[256];
	char sEstimatedTimeLeft[256];
	char sTransferRate[256];
	char sOf[20];
	char sCopied[256];
	char sSec[20];
	//
	int downloadSize, downloadCount, downloadTime;
	char dlSizeBuf[64], totalSizeBuf[64], xferRateBuf[64], dlTimeBuf[64];
	int xferRate;
	int leftWidth;
	const char *s;

	vec4_t colorLtGreyAlpha = {0, 0, 0, .5};

	UI_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, colorLtGreyAlpha );

	s = GetCRDelineatedString("MENUS","DOWNLOAD_STUFF", 0);	// "Downloading:"
	Q_strncpyz(sDownLoading,s?s:"", sizeof(sDownLoading));
	s = GetCRDelineatedString("MENUS","DOWNLOAD_STUFF", 1);	// "Estimated time left:"
	Q_strncpyz(sEstimatedTimeLeft,s?s:"", sizeof(sEstimatedTimeLeft));
	s = GetCRDelineatedString("MENUS","DOWNLOAD_STUFF", 2);	// "Transfer rate:"
	Q_strncpyz(sTransferRate,s?s:"", sizeof(sTransferRate));
	s = GetCRDelineatedString("MENUS","DOWNLOAD_STUFF", 3);	// "of"
	Q_strncpyz(sOf,s?s:"", sizeof(sOf));
	s = GetCRDelineatedString("MENUS","DOWNLOAD_STUFF", 4);	// "copied"
	Q_strncpyz(sCopied,s?s:"", sizeof(sCopied));
	s = GetCRDelineatedString("MENUS","DOWNLOAD_STUFF", 5);	// "sec."
	Q_strncpyz(sSec,s?s:"", sizeof(sSec));

	downloadSize = trap->Cvar_VariableValue( "cl_downloadSize" );
	downloadCount = trap->Cvar_VariableValue( "cl_downloadCount" );
	downloadTime = trap->Cvar_VariableValue( "cl_downloadTime" );

	leftWidth = 320;

	UI_SetColor(colorWhite);

	Text_PaintCenter(centerPoint, yStart + 112, scale, colorWhite, sDownLoading, 0, iMenuFont);
	Text_PaintCenter(centerPoint, yStart + 192, scale, colorWhite, sEstimatedTimeLeft, 0, iMenuFont);
	Text_PaintCenter(centerPoint, yStart + 248, scale, colorWhite, sTransferRate, 0, iMenuFont);

	if (downloadSize > 0) {
		s = va( "%s (%d%%)", downloadName, (int)( (float)downloadCount * 100.0f / downloadSize ) );
	} else {
		s = downloadName;
	}

	Text_PaintCenter(centerPoint, yStart+136, scale, colorWhite, s, 0, iMenuFont);

	UI_ReadableSize( dlSizeBuf,		sizeof dlSizeBuf,		downloadCount );
	UI_ReadableSize( totalSizeBuf,	sizeof totalSizeBuf,	downloadSize );

	if (downloadCount < 4096 || !downloadTime) {
		Text_PaintCenter(leftWidth, yStart+216, scale, colorWhite, "estimating", 0, iMenuFont);
		Text_PaintCenter(leftWidth, yStart+160, scale, colorWhite, va("(%s %s %s %s)", dlSizeBuf, sOf, totalSizeBuf, sCopied), 0, iMenuFont);
	} else {
		if ((uiInfo.uiDC.realTime - downloadTime) / 1000) {
			xferRate = downloadCount / ((uiInfo.uiDC.realTime - downloadTime) / 1000);
		} else {
			xferRate = 0;
		}
		UI_ReadableSize( xferRateBuf, sizeof xferRateBuf, xferRate );

		// Extrapolate estimated completion time
		if (downloadSize && xferRate) {
			int n = downloadSize / xferRate; // estimated time for entire d/l in secs

			// We do it in K (/1024) because we'd overflow around 4MB
			UI_PrintTime ( dlTimeBuf, sizeof dlTimeBuf,
				(n - (((downloadCount/1024) * n) / (downloadSize/1024))) * 1000);

			Text_PaintCenter(leftWidth, yStart+216, scale, colorWhite, dlTimeBuf, 0, iMenuFont);
			Text_PaintCenter(leftWidth, yStart+160, scale, colorWhite, va("(%s %s %s %s)", dlSizeBuf, sOf, totalSizeBuf, sCopied), 0, iMenuFont);
		} else {
			Text_PaintCenter(leftWidth, yStart+216, scale, colorWhite, "estimating", 0, iMenuFont);
			if (downloadSize) {
				Text_PaintCenter(leftWidth, yStart+160, scale, colorWhite, va("(%s %s %s %s)", dlSizeBuf, sOf, totalSizeBuf, sCopied), 0, iMenuFont);
			} else {
				Text_PaintCenter(leftWidth, yStart+160, scale, colorWhite, va("(%s %s)", dlSizeBuf, sCopied), 0, iMenuFont);
			}
		}

		if (xferRate) {
			Text_PaintCenter(leftWidth, yStart+272, scale, colorWhite, va("%s/%s", xferRateBuf,sSec), 0, iMenuFont);
		}
	}
}

/*
========================
UI_DrawConnectScreen

This will also be overlaid on the cgame info screen during loading
to prevent it from blinking away too rapidly on local or lan games.
========================
*/
void UI_DrawConnectScreen( qboolean overlay ) {
	const char *s;
	uiClientState_t	cstate;
	char			info[MAX_INFO_VALUE];
	char text[256];
	float centerPoint, yStart, scale;

	char sStringEdTemp[256];

	menuDef_t *menu = Menus_FindByName("Connect");


	if ( !overlay && menu ) {
		Menu_Paint(menu, qtrue);
	}

	if (!overlay) {
		centerPoint = 320;
		yStart = 130;
		scale = 1.0f;	// -ste
	} else {
		centerPoint = 320;
		yStart = 32;
		scale = 1.0f;	// -ste
		return;
	}

	// see what information we should display
	trap->GetClientState( &cstate );


	info[0] = '\0';
	if( trap->GetConfigString( CS_SERVERINFO, info, sizeof(info) ) ) {
		trap->SE_GetStringTextString("MENUS_LOADING_MAPNAME", sStringEdTemp, sizeof(sStringEdTemp));
		Text_PaintCenter(centerPoint, yStart, scale, colorWhite, va( /*"Loading %s"*/sStringEdTemp, Info_ValueForKey( info, "mapname" )), 0, FONT_MEDIUM);
	}

	if (!Q_stricmp(cstate.servername,"localhost")) {
		trap->SE_GetStringTextString("MENUS_STARTING_UP", sStringEdTemp, sizeof(sStringEdTemp));
		Text_PaintCenter(centerPoint, yStart + 48, scale, colorWhite, sStringEdTemp, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_MEDIUM);
	} else {
		trap->SE_GetStringTextString("MENUS_CONNECTING_TO", sStringEdTemp, sizeof(sStringEdTemp));
		Q_strncpyz(text, va(/*"Connecting to %s"*/sStringEdTemp, cstate.servername), sizeof(text));
		Text_PaintCenter(centerPoint, yStart + 48, scale, colorWhite,text , ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_MEDIUM);
	}

	// display global MOTD at bottom
	Text_PaintCenter(centerPoint, 425, scale, colorWhite, Info_ValueForKey( cstate.updateInfoString, "motd" ), 0, FONT_MEDIUM);
	// print any server info (server full, bad version, etc)
	if ( cstate.connState < CA_CONNECTED ) {
		Text_PaintCenter(centerPoint, yStart + 176, scale, colorWhite, cstate.messageString, 0, FONT_MEDIUM);
	}

	switch ( cstate.connState ) {
	case CA_CONNECTING:
		{
			trap->SE_GetStringTextString("MENUS_AWAITING_CONNECTION", sStringEdTemp, sizeof(sStringEdTemp));
			s = va(/*"Awaiting connection...%i"*/sStringEdTemp, cstate.connectPacketCount);
		}
		break;
	case CA_CHALLENGING:
		{
			trap->SE_GetStringTextString("MENUS_AWAITING_CHALLENGE", sStringEdTemp, sizeof(sStringEdTemp));
			s = va(/*"Awaiting challenge...%i"*/sStringEdTemp, cstate.connectPacketCount);
		}
		break;
	case CA_CONNECTED: {
		char downloadName[MAX_INFO_VALUE];

			trap->Cvar_VariableStringBuffer( "cl_downloadName", downloadName, sizeof(downloadName) );
			if (*downloadName) {
				UI_DisplayDownloadInfo( downloadName, centerPoint, yStart, scale, FONT_MEDIUM );
				return;
			}
		}
		trap->SE_GetStringTextString("MENUS_AWAITING_GAMESTATE", sStringEdTemp, sizeof(sStringEdTemp));
		s = /*"Awaiting gamestate..."*/sStringEdTemp;
		break;
	case CA_LOADING:
		return;
	case CA_PRIMED:
		return;
	default:
		return;
	}

	if (Q_stricmp(cstate.servername,"localhost")) {
		Text_PaintCenter(centerPoint, yStart + 80, scale, colorWhite, s, 0, FONT_MEDIUM);
	}
	// password required / connection rejected information goes here
}

/*
=================
ArenaServers_StopRefresh
=================
*/
static void UI_StopServerRefresh( void )
{
	int count;

	if (!uiInfo.serverStatus.refreshActive) {
		// not currently refreshing
		return;
	}
	uiInfo.serverStatus.refreshActive = qfalse;
	Com_Printf("%d servers listed in browser with %d players.\n",
					uiInfo.serverStatus.numDisplayServers,
					uiInfo.serverStatus.numPlayersOnServers);
	count = trap->LAN_GetServerCount(UI_SourceForLAN());
	if (count - uiInfo.serverStatus.numDisplayServers > 0) {
		Com_Printf("%d servers not listed due to filters, packet loss, invalid info, or pings higher than %d\n",
						count - uiInfo.serverStatus.numDisplayServers,
						(int) trap->Cvar_VariableValue("cl_maxPing"));
	}
}

/*
=================
UI_DoServerRefresh
=================
*/
static void UI_DoServerRefresh( void )
{
	qboolean wait = qfalse;

	if (!uiInfo.serverStatus.refreshActive) {
		return;
	}
	if (ui_netSource.integer != UIAS_FAVORITES) {
		if (ui_netSource.integer == UIAS_LOCAL) {
			if (!trap->LAN_GetServerCount(AS_LOCAL)) {
				wait = qtrue;
			}
		} else {
			if (trap->LAN_GetServerCount(AS_GLOBAL) < 0) {
				wait = qtrue;
			}
		}
	}

	if (uiInfo.uiDC.realTime < uiInfo.serverStatus.refreshtime) {
		if (wait) {
			return;
		}
	}

	// if still trying to retrieve pings
	if (trap->LAN_UpdateVisiblePings(UI_SourceForLAN())) {
		uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 1000;
	} else if (!wait) {
		// get the last servers in the list
		UI_BuildServerDisplayList(2);
		// stop the refresh
		UI_StopServerRefresh();
	}
	//
	UI_BuildServerDisplayList(qfalse);
}

/*
=================
UI_StartServerRefresh
=================
*/
static void UI_StartServerRefresh(qboolean full)
{
	char	*ptr;
	int		lanSource;

	qtime_t q;
	trap->RealTime(&q);
 	trap->Cvar_Set( va("ui_lastServerRefresh_%i", ui_netSource.integer), va("%s-%i, %i @ %i:%02i", GetMonthAbbrevString(q.tm_mon),q.tm_mday, 1900+q.tm_year,q.tm_hour,q.tm_min));

	if (!full) {
		UI_UpdatePendingPings();
		return;
	}

	uiInfo.serverStatus.refreshActive = qtrue;
	uiInfo.serverStatus.nextDisplayRefresh = uiInfo.uiDC.realTime + 1000;
	// clear number of displayed servers
	uiInfo.serverStatus.numDisplayServers = 0;
	uiInfo.serverStatus.numPlayersOnServers = 0;
	lanSource = UI_SourceForLAN();
	// mark all servers as visible so we store ping updates for them
	trap->LAN_MarkServerVisible(lanSource, -1, qtrue);
	// reset all the pings
	trap->LAN_ResetPings(lanSource);
	//
	if( ui_netSource.integer == UIAS_LOCAL ) {
		trap->Cmd_ExecuteText( EXEC_NOW, "localservers\n" );
		uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 1000;
		return;
	}

	uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 5000;
	if( ui_netSource.integer >= UIAS_GLOBAL1 && ui_netSource.integer <= UIAS_GLOBAL5 ) {
		ptr = UI_Cvar_VariableString("debug_protocol");
		if (strlen(ptr)) {
			trap->Cmd_ExecuteText( EXEC_NOW, va( "globalservers %d %s full empty\n", ui_netSource.integer-1, ptr));
		}
		else {
			trap->Cmd_ExecuteText( EXEC_NOW, va( "globalservers %d %d full empty\n", ui_netSource.integer-1, (int)trap->Cvar_VariableValue( "protocol" ) ) );
		}
	}
}

/*
============
GetModuleAPI
============
*/

uiImport_t *trap = NULL;

Q_EXPORT uiExport_t* QDECL GetModuleAPI( int apiVersion, uiImport_t *import )
{
	static uiExport_t uie = {0};

	assert( import );
	trap = import;
	Com_Printf	= trap->Print;
	Com_Error	= trap->Error;

	memset( &uie, 0, sizeof( uie ) );

	if ( apiVersion != UI_API_VERSION ) {
		trap->Print( "Mismatched UI_API_VERSION: expected %i, got %i\n", UI_API_VERSION, apiVersion );
		return NULL;
	}

	uie.Init				= UI_Init;
	uie.Shutdown			= UI_Shutdown;
	uie.KeyEvent			= UI_KeyEvent;
	uie.MouseEvent			= UI_MouseEvent;
	uie.Refresh				= UI_Refresh;
	uie.IsFullscreen		= Menus_AnyFullScreenVisible;
	uie.SetActiveMenu		= UI_SetActiveMenu;
	uie.ConsoleCommand		= UI_ConsoleCommand;
	uie.DrawConnectScreen	= UI_DrawConnectScreen;
	uie.MenuReset			= Menu_Reset;

	return &uie;
}

/*
============
vmMain
============
*/

Q_EXPORT intptr_t vmMain( int command, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4,
	intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11 )
{
	switch ( command ) {
	case UI_GETAPIVERSION:
		return UI_LEGACY_API_VERSION;

	case UI_INIT:
		UI_Init( arg0 );
		return 0;

	case UI_SHUTDOWN:
		UI_Shutdown();
		return 0;

	case UI_KEY_EVENT:
		UI_KeyEvent( arg0, arg1 );
		return 0;

	case UI_MOUSE_EVENT:
		UI_MouseEvent( arg0, arg1 );
		return 0;

	case UI_REFRESH:
		UI_Refresh( arg0 );
		return 0;

	case UI_IS_FULLSCREEN:
		return Menus_AnyFullScreenVisible();

	case UI_SET_ACTIVE_MENU:
		UI_SetActiveMenu( arg0 );
		return 0;

	case UI_CONSOLE_COMMAND:
		return UI_ConsoleCommand(arg0);

	case UI_DRAW_CONNECT_SCREEN:
		UI_DrawConnectScreen( arg0 );
		return 0;

	case UI_MENU_RESET:
		Menu_Reset();
		return 0;
	}

	return -1;
}
