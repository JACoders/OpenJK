/*
===========================================================================
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

//NPC_stats.cpp
#include "b_local.h"
#include "b_public.h"
#include "anims.h"
#include "ghoul2/G2.h"

extern qboolean NPCsPrecached;

extern qboolean WP_SaberParseParms( const char *SaberName, saberInfo_t *saber );
extern void WP_RemoveSaber( saberInfo_t *sabers, int saberNum );

stringID_table_t TeamTable[] =
{
	ENUM2STRING(NPCTEAM_FREE),			// caution, some code checks a team_t via "if (!team_t_varname)" so I guess this should stay as entry 0, great or what? -slc
	ENUM2STRING(NPCTEAM_PLAYER),
	ENUM2STRING(NPCTEAM_ENEMY),
	ENUM2STRING(NPCTEAM_NEUTRAL),	// most droids are team_neutral, there are some exceptions like Probe,Seeker,Interrogator
	{"",	-1}
};

// this list was made using the model directories, this MUST be in the same order as the CLASS_ enum in teams.h
stringID_table_t ClassTable[] =
{
	ENUM2STRING(CLASS_NONE),				// hopefully this will never be used by an npc), just covering all bases
	ENUM2STRING(CLASS_ATST),				// technically droid...
	ENUM2STRING(CLASS_BARTENDER),
	ENUM2STRING(CLASS_BESPIN_COP),
	ENUM2STRING(CLASS_CLAW),
	ENUM2STRING(CLASS_COMMANDO),
	ENUM2STRING(CLASS_DESANN),
	ENUM2STRING(CLASS_FISH),
	ENUM2STRING(CLASS_FLIER2),
	ENUM2STRING(CLASS_GALAK),
	ENUM2STRING(CLASS_GLIDER),
	ENUM2STRING(CLASS_GONK),				// droid
	ENUM2STRING(CLASS_GRAN),
	ENUM2STRING(CLASS_HOWLER),
//	ENUM2STRING(CLASS_RANCOR),
	ENUM2STRING(CLASS_IMPERIAL),
	ENUM2STRING(CLASS_IMPWORKER),
	ENUM2STRING(CLASS_INTERROGATOR),		// droid
	ENUM2STRING(CLASS_JAN),
	ENUM2STRING(CLASS_JEDI),
	ENUM2STRING(CLASS_KYLE),
	ENUM2STRING(CLASS_LANDO),
	ENUM2STRING(CLASS_LIZARD),
	ENUM2STRING(CLASS_LUKE),
	ENUM2STRING(CLASS_MARK1),			// droid
	ENUM2STRING(CLASS_MARK2),			// droid
	ENUM2STRING(CLASS_GALAKMECH),		// droid
	ENUM2STRING(CLASS_MINEMONSTER),
	ENUM2STRING(CLASS_MONMOTHA),
	ENUM2STRING(CLASS_MORGANKATARN),
	ENUM2STRING(CLASS_MOUSE),			// droid
	ENUM2STRING(CLASS_MURJJ),
	ENUM2STRING(CLASS_PRISONER),
	ENUM2STRING(CLASS_PROBE),			// droid
	ENUM2STRING(CLASS_PROTOCOL),			// droid
	ENUM2STRING(CLASS_R2D2),				// droid
	ENUM2STRING(CLASS_R5D2),				// droid
	ENUM2STRING(CLASS_REBEL),
	ENUM2STRING(CLASS_REBORN),
	ENUM2STRING(CLASS_REELO),
	ENUM2STRING(CLASS_REMOTE),
	ENUM2STRING(CLASS_RODIAN),
	ENUM2STRING(CLASS_SEEKER),			// droid
	ENUM2STRING(CLASS_SENTRY),
	ENUM2STRING(CLASS_SHADOWTROOPER),
	ENUM2STRING(CLASS_STORMTROOPER),
	ENUM2STRING(CLASS_SWAMP),
	ENUM2STRING(CLASS_SWAMPTROOPER),
	ENUM2STRING(CLASS_TAVION),
	ENUM2STRING(CLASS_TRANDOSHAN),
	ENUM2STRING(CLASS_UGNAUGHT),
	ENUM2STRING(CLASS_JAWA),
	ENUM2STRING(CLASS_WEEQUAY),
	ENUM2STRING(CLASS_BOBAFETT),
	//ENUM2STRING(CLASS_ROCKETTROOPER),
	//ENUM2STRING(CLASS_PLAYER),
	ENUM2STRING(CLASS_VEHICLE),
	ENUM2STRING(CLASS_RANCOR),
	ENUM2STRING(CLASS_WAMPA),
	{"",	-1}
};

stringID_table_t BSTable[] =
{
	ENUM2STRING(BS_DEFAULT),//# default behavior for that NPC
	ENUM2STRING(BS_ADVANCE_FIGHT),//# Advance to captureGoal and shoot enemies if you can
	ENUM2STRING(BS_SLEEP),//# Play awake script when startled by sound
	ENUM2STRING(BS_FOLLOW_LEADER),//# Follow your leader and shoot any enemies you come across
	ENUM2STRING(BS_JUMP),//# Face navgoal and jump to it.
	ENUM2STRING(BS_SEARCH),//# Using current waypoint as a base), search the immediate branches of waypoints for enemies
	ENUM2STRING(BS_WANDER),//# Wander down random waypoint paths
	ENUM2STRING(BS_NOCLIP),//# Moves through walls), etc.
	ENUM2STRING(BS_REMOVE),//# Waits for player to leave PVS then removes itself
	ENUM2STRING(BS_CINEMATIC),//# Does nothing but face it's angles and move to a goal if it has one
	//the rest are internal only
	{"",				-1},
};

#define stringIDExpand(str, strEnum)	{str, strEnum}, ENUM2STRING(strEnum)

stringID_table_t BSETTable[] =
{
	ENUM2STRING(BSET_SPAWN),//# script to use when first spawned
	ENUM2STRING(BSET_USE),//# script to use when used
	ENUM2STRING(BSET_AWAKE),//# script to use when awoken/startled
	ENUM2STRING(BSET_ANGER),//# script to use when aquire an enemy
	ENUM2STRING(BSET_ATTACK),//# script to run when you attack
	ENUM2STRING(BSET_VICTORY),//# script to run when you kill someone
	ENUM2STRING(BSET_LOSTENEMY),//# script to run when you can't find your enemy
	ENUM2STRING(BSET_PAIN),//# script to use when take pain
	ENUM2STRING(BSET_FLEE),//# script to use when take pain below 50% of health
	ENUM2STRING(BSET_DEATH),//# script to use when killed
	ENUM2STRING(BSET_DELAYED),//# script to run when self->delayScriptTime is reached
	ENUM2STRING(BSET_BLOCKED),//# script to run when blocked by a friendly NPC or player
	ENUM2STRING(BSET_BUMPED),//# script to run when bumped into a friendly NPC or player (can set bumpRadius)
	ENUM2STRING(BSET_STUCK),//# script to run when blocked by a wall
	ENUM2STRING(BSET_FFIRE),//# script to run when player shoots their own teammates
	ENUM2STRING(BSET_FFDEATH),//# script to run when player kills a teammate
	stringIDExpand("", BSET_INVALID),
	{"",				-1},
};

extern stringID_table_t WPTable[];
extern stringID_table_t FPTable[];

char	*TeamNames[TEAM_NUM_TEAMS] =
{
	"",
	"player",
	"enemy",
	"neutral"
};

// this list was made using the model directories, this MUST be in the same order as the CLASS_ enum in teams.h
char	*ClassNames[CLASS_NUM_CLASSES] =
{
	"",				// class none
	"atst",
	"bartender",
	"bespin_cop",
	"claw",
	"commando",
	"desann",
	"fish",
	"flier2",
	"galak",
	"glider",
	"gonk",
	"gran",
	"howler",
	"imperial",
	"impworker",
	"interrogator",
	"jan",
	"jedi",
	"kyle",
	"lando",
	"lizard",
	"luke",
	"mark1",
	"mark2",
	"galak_mech",
	"minemonster",
	"monmotha",
	"morgankatarn",
	"mouse",
	"murjj",
	"prisoner",
	"probe",
	"protocol",
	"r2d2",
	"r5d2",
	"rebel",
	"reborn",
	"reelo",
	"remote",
	"rodian",
	"seeker",
	"sentry",
	"shadowtrooper",
	"stormtrooper",
	"swamp",
	"swamptrooper",
	"tavion",
	"trandoshan",
	"ugnaught",
	"weequay",
	"bobafett",
	"vehicle",
	"rancor",
	"wampa",
};


/*
NPC_ReactionTime
*/
//FIXME use grandom in here
int NPC_ReactionTime ( void )
{
	return 200 * ( 6 - NPCS.NPCInfo->stats.reactions );
}

//
// parse support routines
//

extern qboolean BG_ParseLiteral( const char **data, const char *string );

//
// NPC parameters file : scripts/NPCs.cfg
//
#define MAX_NPC_DATA_SIZE 0x40000
char	NPCParms[MAX_NPC_DATA_SIZE];

/*
team_t TranslateTeamName( const char *name )
{
	int n;

	for ( n = (NPCTEAM_FREE + 1); n < NPCTEAM_NUM_TEAMS; n++ )
	{
		if ( Q_stricmp( TeamNames[n], name ) == 0 )
		{
			return ((team_t) n);
		}
	}

	return NPCTEAM_FREE;
}

class_t TranslateClassName( const char *name )
{
	int n;

	for ( n = (CLASS_NONE + 1); n < CLASS_NUM_CLASSES; n++ )
	{
		if ( Q_stricmp( ClassNames[n], name ) == 0 )
		{
			return ((class_t) n);
		}
	}

	return CLASS_NONE;  // I hope this never happens, maybe print a warning
}
*/

/*
static rank_t TranslateRankName( const char *name )

  Should be used to determine pip bolt-ons
*/
static rank_t TranslateRankName( const char *name )
{
	if ( !Q_stricmp( name, "civilian" ) )
	{
		return RANK_CIVILIAN;
	}

	if ( !Q_stricmp( name, "crewman" ) )
	{
		return RANK_CREWMAN;
	}

	if ( !Q_stricmp( name, "ensign" ) )
	{
		return RANK_ENSIGN;
	}

	if ( !Q_stricmp( name, "ltjg" ) )
	{
		return RANK_LT_JG;
	}

	if ( !Q_stricmp( name, "lt" ) )
	{
		return RANK_LT;
	}

	if ( !Q_stricmp( name, "ltcomm" ) )
	{
		return RANK_LT_COMM;
	}

	if ( !Q_stricmp( name, "commander" ) )
	{
		return RANK_COMMANDER;
	}

	if ( !Q_stricmp( name, "captain" ) )
	{
		return RANK_CAPTAIN;
	}

	return RANK_CIVILIAN;

}

extern saber_colors_t TranslateSaberColor( const char *name );

/* static int MethodNameToNumber( const char *name ) {
	if ( !Q_stricmp( name, "EXPONENTIAL" ) ) {
		return METHOD_EXPONENTIAL;
	}
	if ( !Q_stricmp( name, "LINEAR" ) ) {
		return METHOD_LINEAR;
	}
	if ( !Q_stricmp( name, "LOGRITHMIC" ) ) {
		return METHOD_LOGRITHMIC;
	}
	if ( !Q_stricmp( name, "ALWAYS" ) ) {
		return METHOD_ALWAYS;
	}
	if ( !Q_stricmp( name, "NEVER" ) ) {
		return METHOD_NEVER;
	}
	return -1;
}

static int ItemNameToNumber( const char *name, int itemType ) {
//	int		n;

	for ( n = 0; n < bg_numItems; n++ ) {
		if ( bg_itemlist[n].type != itemType ) {
			continue;
		}
		if ( Q_stricmp( bg_itemlist[n].classname, name ) == 0 ) {
			return bg_itemlist[n].tag;
		}
	}
	return -1;
}
*/

//rwwFIXMEFIXME: movetypes
/*
static int MoveTypeNameToEnum( const char *name )
{
	if(!Q_stricmp("runjump", name))
	{
		return MT_RUNJUMP;
	}
	else if(!Q_stricmp("walk", name))
	{
		return MT_WALK;
	}
	else if(!Q_stricmp("flyswim", name))
	{
		return MT_FLYSWIM;
	}
	else if(!Q_stricmp("static", name))
	{
		return MT_STATIC;
	}

	return MT_STATIC;
}
*/

//#define CONVENIENT_ANIMATION_FILE_DEBUG_THING

#ifdef CONVENIENT_ANIMATION_FILE_DEBUG_THING
void SpewDebugStuffToFile(animation_t *anims)
{
	char BGPAFtext[40000];
	fileHandle_t f;
	int i = 0;

	trap->FS_Open("file_of_debug_stuff_SP.txt", &f, FS_WRITE);

	if (!f)
	{
		return;
	}

	BGPAFtext[0] = 0;

	while (i < MAX_ANIMATIONS)
	{
		strcat(BGPAFtext, va("%i %i\n", i, anims[i].frameLerp));
		i++;
	}

	trap->FS_Write(BGPAFtext, strlen(BGPAFtext), f);
	trap->FS_Close(f);
}
#endif

qboolean G_ParseAnimFileSet( const char *filename, const char *animCFG, int *animFileIndex )
{
	*animFileIndex = BG_ParseAnimationFile(filename, NULL, qfalse);
	//if it's humanoid we should have it cached and return it, if it is not it will be loaded (unless it's also cached already)

	if (*animFileIndex == -1)
	{
		return qfalse;
	}

	//I guess this isn't really even needed game-side.
	//BG_ParseAnimationSndFile(filename, *animFileIndex);
	return qtrue;
}

void NPC_PrecacheAnimationCFG( const char *NPC_type )
{
#if 0 //rwwFIXMEFIXME: Actually precache stuff here.
	char	filename[MAX_QPATH];
	const char	*token;
	const char	*value;
	const char	*p;
	int		junk;

	if ( !Q_stricmp( "random", NPC_type ) )
	{//sorry, can't precache a random just yet
		return;
	}

	p = NPCParms;
	COM_BeginParseSession(NPCFile);

	// look for the right NPC
	while ( p )
	{
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 )
			return;

		if ( !Q_stricmp( token, NPC_type ) )
		{
			break;
		}

		SkipBracedSection( &p, 0 );
	}

	if ( !p )
	{
		return;
	}

	if ( BG_ParseLiteral( &p, "{" ) )
	{
		return;
	}

	// parse the NPC info block
	while ( 1 )
	{
		token = COM_ParseExt( &p, qtrue );
		if ( !token[0] )
		{
			Com_Printf( S_COLOR_RED"ERROR: unexpected EOF while parsing '%s'\n", NPC_type );
			return;
		}

		if ( !Q_stricmp( token, "}" ) )
		{
			break;
		}

		// legsmodel
		if ( !Q_stricmp( token, "legsmodel" ) )
		{
			if ( COM_ParseString( &p, &value ) )
			{
				continue;
			}
			//must copy data out of this pointer into a different part of memory because the funcs we're about to call will call COM_ParseExt
			Q_strncpyz( filename, value, sizeof( filename ) );
			G_ParseAnimFileSet( filename, filename, &junk );
			return;
		}

		// playerModel
		if ( !Q_stricmp( token, "playerModel" ) )
		{
			if ( COM_ParseString( &p, &value ) )
			{
				continue;
			}
			/*
			char	animName[MAX_QPATH];
			char	*GLAName;
			char	*slash = NULL;
			char	*strippedName;

			int handle = trap->G2API_PrecacheGhoul2Model( va( "models/players/%s/model.glm", value ) );
			if ( handle > 0 )//FIXME: isn't 0 a valid handle?
			{
				GLAName = trap->G2API_GetAnimFileNameIndex( handle );
				if ( GLAName )
				{
					Q_strncpyz( animName, GLAName, sizeof( animName ), qtrue );
					slash = strrchr( animName, '/' );
					if ( slash )
					{
						*slash = 0;
					}
					strippedName = COM_SkipPath( animName );

					//must copy data out of this pointer into a different part of memory because the funcs we're about to call will call COM_ParseExt
					Q_strncpyz( filename, value, sizeof( filename ), qtrue );
					G_ParseAnimFileSet( value, strippedName, &junk );//qfalse );
					//FIXME: still not precaching the animsounds.cfg?
					return;
				}
			}
			*/
			//rwwFIXMEFIXME: Do this properly.
		}
	}
#endif
}

extern int NPC_WeaponsForTeam( team_t team, int spawnflags, const char *NPC_type );
void NPC_PrecacheWeapons( team_t playerTeam, int spawnflags, char *NPCtype )
{
	int weapons = NPC_WeaponsForTeam( playerTeam, spawnflags, NPCtype );
	int curWeap;

	for ( curWeap = WP_SABER; curWeap < WP_NUM_WEAPONS; curWeap++ )
	{
		if (weapons & (1 << curWeap))
		{
			RegisterItem(BG_FindItemForWeapon((weapon_t)curWeap));
		}
	}

#if 0 //rwwFIXMEFIXME: actually precache weapons here
	int weapons = NPC_WeaponsForTeam( playerTeam, spawnflags, NPCtype );
	gitem_t	*item;
	for ( int curWeap = WP_SABER; curWeap < WP_NUM_WEAPONS; curWeap++ )
	{
		if ( (weapons & ( 1 << curWeap )) )
		{
			item = FindItemForWeapon( ((weapon_t)(curWeap)) );	//precache the weapon
			CG_RegisterItemSounds( (item-bg_itemlist) );
			CG_RegisterItemVisuals( (item-bg_itemlist) );
			//precache the in-hand/in-world ghoul2 weapon model

			char weaponModel[64];

			strcpy (weaponModel, weaponData[curWeap].weaponMdl);
			if (char *spot = strstr(weaponModel, ".md3") ) {
				*spot = 0;
				spot = strstr(weaponModel, "_w");//i'm using the in view weapon array instead of scanning the item list, so put the _w back on
				if (!spot) {
					strcat (weaponModel, "_w");
				}
				strcat (weaponModel, ".glm");	//and change to ghoul2
			}
			trap->G2API_PrecacheGhoul2Model( weaponModel ); // correct way is item->world_model
		}
	}
#endif
}

/*
void NPC_Precache ( char *NPCName )

Precaches NPC skins, tgas and md3s.

*/
void NPC_Precache ( gentity_t *spawner )
{
	npcteam_t			playerTeam = NPCTEAM_FREE;
	const char	*token;
	const char	*value;
	const char	*p;
	char	*patch;
	char	sound[MAX_QPATH];
	qboolean	md3Model = qfalse;
	char	playerModel[MAX_QPATH];
	char	customSkin[MAX_QPATH];
	char	sessionName[MAX_QPATH+15];

	if ( !Q_stricmp( "random", spawner->NPC_type ) )
	{//sorry, can't precache a random just yet
		return;
	}
	strcpy(customSkin,"default");

	p = NPCParms;
	Com_sprintf( sessionName, sizeof(sessionName), "NPC_Precache(%s)", spawner->NPC_type );
	COM_BeginParseSession(sessionName);

	// look for the right NPC
	while ( p )
	{
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 )
			return;

		if ( !Q_stricmp( token, spawner->NPC_type ) )
		{
			break;
		}

		SkipBracedSection( &p, 0 );
	}

	if ( !p )
	{
		return;
	}

	if ( BG_ParseLiteral( &p, "{" ) )
	{
		return;
	}

	// parse the NPC info block
	while ( 1 )
	{
		token = COM_ParseExt( &p, qtrue );
		if ( !token[0] )
		{
			Com_Printf( S_COLOR_RED"ERROR: unexpected EOF while parsing '%s'\n", spawner->NPC_type );
			return;
		}

		if ( !Q_stricmp( token, "}" ) )
		{
			break;
		}

		// headmodel
		if ( !Q_stricmp( token, "headmodel" ) )
		{
			if ( COM_ParseString( &p, &value ) )
			{
				continue;
			}

			if(!Q_stricmp("none", value))
			{
			}
			else
			{
				//Q_strncpyz( ri.headModelName, value, sizeof(ri.headModelName), qtrue);
			}
			md3Model = qtrue;
			continue;
		}

		// torsomodel
		if ( !Q_stricmp( token, "torsomodel" ) )
		{
			if ( COM_ParseString( &p, &value ) )
			{
				continue;
			}

			if(!Q_stricmp("none", value))
			{
			}
			else
			{
				//Q_strncpyz( ri.torsoModelName, value, sizeof(ri.torsoModelName), qtrue);
			}
			md3Model = qtrue;
			continue;
		}

		// legsmodel
		if ( !Q_stricmp( token, "legsmodel" ) )
		{
			if ( COM_ParseString( &p, &value ) )
			{
				continue;
			}
			//Q_strncpyz( ri.legsModelName, value, sizeof(ri.legsModelName), qtrue);
			md3Model = qtrue;
			continue;
		}

		// playerModel
		if ( !Q_stricmp( token, "playerModel" ) )
		{
			if ( COM_ParseString( &p, &value ) )
			{
				continue;
			}
			Q_strncpyz( playerModel, value, sizeof(playerModel));
			md3Model = qfalse;
			continue;
		}

		// customSkin
		if ( !Q_stricmp( token, "customSkin" ) )
		{
			if ( COM_ParseString( &p, &value ) )
			{
				continue;
			}
			Q_strncpyz( customSkin, value, sizeof(customSkin));
			continue;
		}

		// playerTeam
		if ( !Q_stricmp( token, "playerTeam" ) )
		{
			char tk[4096]; //rww - hackilicious!

			if ( COM_ParseString( &p, &value ) )
			{
				continue;
			}
			//playerTeam = TranslateTeamName(value);
			Com_sprintf(tk, sizeof(tk), "NPC%s", token);
			playerTeam = GetIDForString( TeamTable, tk );
			continue;
		}


		// snd
		if ( !Q_stricmp( token, "snd" ) ) {
			if ( COM_ParseString( &p, &value ) ) {
				continue;
			}
			if ( !(spawner->r.svFlags&SVF_NO_BASIC_SOUNDS) )
			{
				//FIXME: store this in some sound field or parse in the soundTable like the animTable...
				Q_strncpyz( sound, value, sizeof( sound ) );
				patch = strstr( sound, "/" );
				if ( patch )
				{
					*patch = 0;
				}
				spawner->s.csSounds_Std = G_SoundIndex( va("*$%s", sound) );
			}
			continue;
		}

		// sndcombat
		if ( !Q_stricmp( token, "sndcombat" ) ) {
			if ( COM_ParseString( &p, &value ) ) {
				continue;
			}
			if ( !(spawner->r.svFlags&SVF_NO_COMBAT_SOUNDS) )
			{
				//FIXME: store this in some sound field or parse in the soundTable like the animTable...
				Q_strncpyz( sound, value, sizeof( sound ) );
				patch = strstr( sound, "/" );
				if ( patch )
				{
					*patch = 0;
				}
				spawner->s.csSounds_Combat = G_SoundIndex( va("*$%s", sound) );
			}
			continue;
		}

		// sndextra
		if ( !Q_stricmp( token, "sndextra" ) ) {
			if ( COM_ParseString( &p, &value ) ) {
				continue;
			}
			if ( !(spawner->r.svFlags&SVF_NO_EXTRA_SOUNDS) )
			{
				//FIXME: store this in some sound field or parse in the soundTable like the animTable...
				Q_strncpyz( sound, value, sizeof( sound ) );
				patch = strstr( sound, "/" );
				if ( patch )
				{
					*patch = 0;
				}
				spawner->s.csSounds_Extra = G_SoundIndex( va("*$%s", sound) );
			}
			continue;
		}

		// sndjedi
		if ( !Q_stricmp( token, "sndjedi" ) ) {
			if ( COM_ParseString( &p, &value ) ) {
				continue;
			}
			if ( !(spawner->r.svFlags&SVF_NO_EXTRA_SOUNDS) )
			{
				//FIXME: store this in some sound field or parse in the soundTable like the animTable...
				Q_strncpyz( sound, value, sizeof( sound ) );
				patch = strstr( sound, "/" );
				if ( patch )
				{
					*patch = 0;
				}
				spawner->s.csSounds_Jedi = G_SoundIndex( va("*$%s", sound) );
			}
			continue;
		}

		if (!Q_stricmp(token, "weapon"))
		{
			int curWeap;

			if (COM_ParseString(&p, &value))
			{
				continue;
			}

			curWeap = GetIDForString( WPTable, value );

			if (curWeap > WP_NONE && curWeap < WP_NUM_WEAPONS)
			{
				RegisterItem(BG_FindItemForWeapon((weapon_t)curWeap));
			}
			continue;
		}
	}

	// If we're not a vehicle, then an error here would be valid...
	if ( !spawner->client || spawner->client->NPC_class != CLASS_VEHICLE )
	{
		if ( md3Model )
		{
			Com_Printf("MD3 model using NPCs are not supported in MP\n");
		}
		else
		{ //if we have a model/skin then index them so they'll be registered immediately
			//when the client gets a configstring update.
			char modelName[MAX_QPATH];

			Com_sprintf(modelName, sizeof(modelName), "models/players/%s/model.glm", playerModel);
			if (customSkin[0])
			{ //append it after a *
				strcat( modelName, va("*%s", customSkin) );
			}

			G_ModelIndex(modelName);
		}
	}

	//precache this NPC's possible weapons
	NPC_PrecacheWeapons( (team_t)playerTeam, spawner->spawnflags, spawner->NPC_type );

//	CG_RegisterNPCCustomSounds( &ci );
//	CG_RegisterNPCEffects( playerTeam );
	//rwwFIXMEFIXME: same
	//FIXME: Look for a "sounds" directory and precache death, pain, alert sounds
}

#if 0
void NPC_BuildRandom( gentity_t *NPC )
{
	int	sex, color, head;

	sex = Q_irand(0, 2);
	color = Q_irand(0, 2);
	switch( sex )
	{
	case 0://female
		head = Q_irand(0, 2);
		switch( head )
		{
		default:
		case 0:
			Q_strncpyz( NPC->client->renderInfo.headModelName, "garren", sizeof(NPC->client->renderInfo.headModelName), qtrue );
			break;
		case 1:
			Q_strncpyz( NPC->client->renderInfo.headModelName, "garren/salma", sizeof(NPC->client->renderInfo.headModelName), qtrue );
			break;
		case 2:
			Q_strncpyz( NPC->client->renderInfo.headModelName, "garren/mackey", sizeof(NPC->client->renderInfo.headModelName), qtrue );
			color = Q_irand(3, 5);//torso needs to be afam
			break;
		}
		switch( color )
		{
		default:
		case 0:
			Q_strncpyz( NPC->client->renderInfo.torsoModelName, "crewfemale/gold", sizeof(NPC->client->renderInfo.torsoModelName), qtrue );
			break;
		case 1:
			Q_strncpyz( NPC->client->renderInfo.torsoModelName, "crewfemale", sizeof(NPC->client->renderInfo.torsoModelName), qtrue );
			break;
		case 2:
			Q_strncpyz( NPC->client->renderInfo.torsoModelName, "crewfemale/blue", sizeof(NPC->client->renderInfo.torsoModelName), qtrue );
			break;
		case 3:
			Q_strncpyz( NPC->client->renderInfo.torsoModelName, "crewfemale/aframG", sizeof(NPC->client->renderInfo.torsoModelName), qtrue );
			break;
		case 4:
			Q_strncpyz( NPC->client->renderInfo.torsoModelName, "crewfemale/aframR", sizeof(NPC->client->renderInfo.torsoModelName), qtrue );
			break;
		case 5:
			Q_strncpyz( NPC->client->renderInfo.torsoModelName, "crewfemale/aframB", sizeof(NPC->client->renderInfo.torsoModelName), qtrue );
			break;
		}
		Q_strncpyz( NPC->client->renderInfo.legsModelName, "crewfemale", sizeof(NPC->client->renderInfo.legsModelName), qtrue );
		break;
	default:
	case 1://male
	case 2://male
		head = Q_irand(0, 4);
		switch( head )
		{
		default:
		case 0:
			Q_strncpyz( NPC->client->renderInfo.headModelName, "chakotay/nelson", sizeof(NPC->client->renderInfo.headModelName), qtrue );
			break;
		case 1:
			Q_strncpyz( NPC->client->renderInfo.headModelName, "paris/chase", sizeof(NPC->client->renderInfo.headModelName), qtrue );
			break;
		case 2:
			Q_strncpyz( NPC->client->renderInfo.headModelName, "doctor/pasty", sizeof(NPC->client->renderInfo.headModelName), qtrue );
			break;
		case 3:
			Q_strncpyz( NPC->client->renderInfo.headModelName, "kim/durk", sizeof(NPC->client->renderInfo.headModelName), qtrue );
			break;
		case 4:
			Q_strncpyz( NPC->client->renderInfo.headModelName, "paris/kray", sizeof(NPC->client->renderInfo.headModelName), qtrue );
			break;
		}
		switch( color )
		{
		default:
		case 0:
			Q_strncpyz( NPC->client->renderInfo.torsoModelName, "crewthin/red", sizeof(NPC->client->renderInfo.torsoModelName), qtrue );
			break;
		case 1:
			Q_strncpyz( NPC->client->renderInfo.torsoModelName, "crewthin", sizeof(NPC->client->renderInfo.torsoModelName), qtrue );
			break;
		case 2:
			Q_strncpyz( NPC->client->renderInfo.torsoModelName, "crewthin/blue", sizeof(NPC->client->renderInfo.torsoModelName), qtrue );
			break;
			//NOTE: 3 - 5 should be red, gold & blue, afram hands
		}
		Q_strncpyz( NPC->client->renderInfo.legsModelName, "crewthin", sizeof(NPC->client->renderInfo.legsModelName), qtrue );
		break;
	}

	NPC->s.modelScale[0] = NPC->s.modelScale[1] = NPC->s.modelScale[2] = Q_irand(87, 102)/100.0f;
//	NPC->client->race = RACE_HUMAN;
	NPC->NPC->rank = RANK_CREWMAN;
	NPC->client->playerTeam = NPC->s.teamowner = TEAM_PLAYER;
	NPC->client->clientInfo.customBasicSoundDir = "kyle";//FIXME: generic default?
}
#endif

extern void SetupGameGhoul2Model(gentity_t *ent, char *modelname, char *skinName);
qboolean NPC_ParseParms( const char *NPCName, gentity_t *NPC )
{
	const char	*token;
	const char	*value;
	const char	*p;
	int		n;
	float	f;
	char	*patch;
	char	sound[MAX_QPATH];
	char	playerModel[MAX_QPATH];
	char	customSkin[MAX_QPATH];
	char	sessionName[MAX_QPATH+17];
	renderInfo_t	*ri = &NPC->client->renderInfo;
	gNPCstats_t		*stats = NULL;
	qboolean	md3Model = qtrue;
	char	surfOff[1024];
	char	surfOn[1024];
	qboolean parsingPlayer = qfalse;
	vec3_t playerMins;
	vec3_t playerMaxs;
	int npcSaber1 = 0;
	int npcSaber2 = 0;

	VectorSet(playerMins, -15, -15, DEFAULT_MINS_2);
	VectorSet(playerMaxs, 15, 15, DEFAULT_MAXS_2);

	strcpy(customSkin,"default");
	if ( !NPCName || !NPCName[0])
	{
		NPCName = "Player";
	}

	if ( !NPC->s.number && NPC->client != NULL )
	{//player, only want certain data
		parsingPlayer = qtrue;
	}

	if ( NPC->NPC )
	{
		stats = &NPC->NPC->stats;
/*
	NPC->NPC->allWeaponOrder[0]	= WP_BRYAR_PISTOL;
	NPC->NPC->allWeaponOrder[1]	= WP_SABER;
	NPC->NPC->allWeaponOrder[2]	= WP_IMOD;
	NPC->NPC->allWeaponOrder[3]	= WP_SCAVENGER_RIFLE;
	NPC->NPC->allWeaponOrder[4]	= WP_TRICORDER;
	NPC->NPC->allWeaponOrder[6]	= WP_NONE;
	NPC->NPC->allWeaponOrder[6]	= WP_NONE;
	NPC->NPC->allWeaponOrder[7]	= WP_NONE;
*/
		// fill in defaults
		stats->aggression	= 3;
		stats->aim			= 3;
		stats->earshot		= 1024;
		stats->evasion		= 3;
		stats->hfov			= 90;
		stats->intelligence	= 3;
		stats->move			= 3;
		stats->reactions	= 3;
		stats->vfov			= 60;
		stats->vigilance	= 0.1f;
		stats->visrange		= 1024;

		stats->health		= 0;

		stats->yawSpeed		= 90;
		stats->walkSpeed	= 90;
		stats->runSpeed		= 300;
		stats->acceleration	= 15;//Increase/descrease speed this much per frame (20fps)
	}
	else
	{
		stats = NULL;
	}

	//Set defaults
	//FIXME: should probably put default torso and head models, but what about enemies
	//that don't have any- like Stasis?
	//Q_strncpyz( ri->headModelName,	DEFAULT_HEADMODEL,  sizeof(ri->headModelName),	qtrue);
	//Q_strncpyz( ri->torsoModelName, DEFAULT_TORSOMODEL, sizeof(ri->torsoModelName),	qtrue);
	//Q_strncpyz( ri->legsModelName,	DEFAULT_LEGSMODEL,  sizeof(ri->legsModelName),	qtrue);
	//FIXME: should we have one for weapon too?
	memset( (char *)surfOff, 0, sizeof(surfOff) );
	memset( (char *)surfOn, 0, sizeof(surfOn) );

	/*
	ri->headYawRangeLeft = 50;
	ri->headYawRangeRight = 50;
	ri->headPitchRangeUp = 40;
	ri->headPitchRangeDown = 50;
	ri->torsoYawRangeLeft = 60;
	ri->torsoYawRangeRight = 60;
	ri->torsoPitchRangeUp = 30;
	ri->torsoPitchRangeDown = 70;
	*/

	ri->headYawRangeLeft = 80;
	ri->headYawRangeRight = 80;
	ri->headPitchRangeUp = 45;
	ri->headPitchRangeDown = 45;
	ri->torsoYawRangeLeft = 60;
	ri->torsoYawRangeRight = 60;
	ri->torsoPitchRangeUp = 30;
	ri->torsoPitchRangeDown = 50;

	VectorCopy(playerMins, NPC->r.mins);
	VectorCopy(playerMaxs, NPC->r.maxs);
	NPC->client->ps.crouchheight = CROUCH_MAXS_2;
	NPC->client->ps.standheight = DEFAULT_MAXS_2;

	//rwwFIXMEFIXME: ...
	/*
	NPC->client->moveType		= MT_RUNJUMP;

	NPC->client->dismemberProbHead = 100;
	NPC->client->dismemberProbArms = 100;
	NPC->client->dismemberProbHands = 100;
	NPC->client->dismemberProbWaist = 100;
	NPC->client->dismemberProbLegs = 100;

	NPC->s.modelScale[0] = NPC->s.modelScale[1] = NPC->s.modelScale[2] = 1.0f;
	*/

	NPC->client->ps.customRGBA[0]=255;
	NPC->client->ps.customRGBA[1]=255;
	NPC->client->ps.customRGBA[2]=255;
	NPC->client->ps.customRGBA[3]=255;

	if ( !Q_stricmp( "random", NPCName ) )
	{//Randomly assemble a starfleet guy
		//NPC_BuildRandom( NPC );
		Com_Printf("RANDOM NPC NOT SUPPORTED IN MP\n");
		return qfalse;
	}
	else
	{
		int fp;

		p = NPCParms;
		Com_sprintf( sessionName, sizeof(sessionName), "NPC_ParseParms(%s)", NPCName );
		COM_BeginParseSession(sessionName);

		// look for the right NPC
		while ( p )
		{
			token = COM_ParseExt( &p, qtrue );
			if ( token[0] == 0 )
			{
				return qfalse;
			}

			if ( !Q_stricmp( token, NPCName ) )
			{
				break;
			}

			SkipBracedSection( &p, 0 );
		}
		if ( !p )
		{
			return qfalse;
		}

		if ( BG_ParseLiteral( &p, "{" ) )
		{
			return qfalse;
		}

		// parse the NPC info block
		while ( 1 )
		{
			token = COM_ParseExt( &p, qtrue );
			if ( !token[0] )
			{
				Com_Printf( S_COLOR_RED"ERROR: unexpected EOF while parsing '%s'\n", NPCName );
				return qfalse;
			}

			if ( !Q_stricmp( token, "}" ) )
			{
				break;
			}
	//===MODEL PROPERTIES===========================================================
			// custom color
			if ( !Q_stricmp( token, "customRGBA" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				if ( !Q_stricmp( value, "random") )
				{
					NPC->client->ps.customRGBA[0]=Q_irand(0,255);
					NPC->client->ps.customRGBA[1]=Q_irand(0,255);
					NPC->client->ps.customRGBA[2]=Q_irand(0,255);
					NPC->client->ps.customRGBA[3]=255;
				}
				else if ( !Q_stricmp( value, "random1" ) )
				{
					NPC->client->ps.customRGBA[3] = 255;
					switch ( Q_irand( 0, 5 ) )
					{
					default:
					case 0:
						NPC->client->ps.customRGBA[0] = 127;
						NPC->client->ps.customRGBA[1] = 153;
						NPC->client->ps.customRGBA[2] = 255;
						break;
					case 1:
						NPC->client->ps.customRGBA[0] = 177;
						NPC->client->ps.customRGBA[1] = 29;
						NPC->client->ps.customRGBA[2] = 13;
						break;
					case 2:
						NPC->client->ps.customRGBA[0] = 47;
						NPC->client->ps.customRGBA[1] = 90;
						NPC->client->ps.customRGBA[2] = 40;
						break;
					case 3:
						NPC->client->ps.customRGBA[0] = 181;
						NPC->client->ps.customRGBA[1] = 207;
						NPC->client->ps.customRGBA[2] = 255;
						break;
					case 4:
						NPC->client->ps.customRGBA[0] = 138;
						NPC->client->ps.customRGBA[1] = 83;
						NPC->client->ps.customRGBA[2] = 0;
						break;
					case 5:
						NPC->client->ps.customRGBA[0] = 254;
						NPC->client->ps.customRGBA[1] = 199;
						NPC->client->ps.customRGBA[2] = 14;
						break;
					}
				}
				else if ( !Q_stricmp( value, "jedi_hf" ) )
				{
					NPC->client->ps.customRGBA[3] = 255;
					switch ( Q_irand( 0, 7 ) )
					{
					default:
					case 0://red1
						NPC->client->ps.customRGBA[0] = 165;
						NPC->client->ps.customRGBA[1] = 48;
						NPC->client->ps.customRGBA[2] = 21;
						break;
					case 1://yellow1
						NPC->client->ps.customRGBA[0] = 254;
						NPC->client->ps.customRGBA[1] = 230;
						NPC->client->ps.customRGBA[2] = 132;
						break;
					case 2://bluegray
						NPC->client->ps.customRGBA[0] = 181;
						NPC->client->ps.customRGBA[1] = 207;
						NPC->client->ps.customRGBA[2] = 255;
						break;
					case 3://pink
						NPC->client->ps.customRGBA[0] = 233;
						NPC->client->ps.customRGBA[1] = 183;
						NPC->client->ps.customRGBA[2] = 208;
						break;
					case 4://lt blue
						NPC->client->ps.customRGBA[0] = 161;
						NPC->client->ps.customRGBA[1] = 226;
						NPC->client->ps.customRGBA[2] = 240;
						break;
					case 5://blue
						NPC->client->ps.customRGBA[0] = 101;
						NPC->client->ps.customRGBA[1] = 159;
						NPC->client->ps.customRGBA[2] = 255;
						break;
					case 6://orange
						NPC->client->ps.customRGBA[0] = 255;
						NPC->client->ps.customRGBA[1] = 157;
						NPC->client->ps.customRGBA[2] = 114;
						break;
					case 7://violet
						NPC->client->ps.customRGBA[0] = 216;
						NPC->client->ps.customRGBA[1] = 160;
						NPC->client->ps.customRGBA[2] = 255;
						break;
					}
				}
				else if ( !Q_stricmp( value, "jedi_hm" ) )
				{
					NPC->client->ps.customRGBA[3] = 255;
					switch ( Q_irand( 0, 7 ) )
					{
					default:
					case 0://yellow
						NPC->client->ps.customRGBA[0] = 252;
						NPC->client->ps.customRGBA[1] = 243;
						NPC->client->ps.customRGBA[2] = 180;
						break;
					case 1://blue
						NPC->client->ps.customRGBA[0] = 69;
						NPC->client->ps.customRGBA[1] = 109;
						NPC->client->ps.customRGBA[2] = 255;
						break;
					case 2://gold
						NPC->client->ps.customRGBA[0] = 254;
						NPC->client->ps.customRGBA[1] = 197;
						NPC->client->ps.customRGBA[2] = 73;
						break;
					case 3://orange
						NPC->client->ps.customRGBA[0] = 178;
						NPC->client->ps.customRGBA[1] = 78;
						NPC->client->ps.customRGBA[2] = 18;
						break;
					case 4://bluegreen
						NPC->client->ps.customRGBA[0] = 112;
						NPC->client->ps.customRGBA[1] = 153;
						NPC->client->ps.customRGBA[2] = 161;
						break;
					case 5://blue2
						NPC->client->ps.customRGBA[0] = 123;
						NPC->client->ps.customRGBA[1] = 182;
						NPC->client->ps.customRGBA[2] = 255;
						break;
					case 6://green2
						NPC->client->ps.customRGBA[0] = 0;
						NPC->client->ps.customRGBA[1] = 88;
						NPC->client->ps.customRGBA[2] = 105;
						break;
					case 7://violet
						NPC->client->ps.customRGBA[0] = 138;
						NPC->client->ps.customRGBA[1] = 0;
						NPC->client->ps.customRGBA[2] = 0;
						break;
					}
				}
				else if ( !Q_stricmp( value, "jedi_kdm" ) )
				{
					NPC->client->ps.customRGBA[3] = 255;
					switch ( Q_irand( 0, 8 ) )
					{
					default:
					case 0://blue
						NPC->client->ps.customRGBA[0] = 85;
						NPC->client->ps.customRGBA[1] = 120;
						NPC->client->ps.customRGBA[2] = 255;
						break;
					case 1://violet
						NPC->client->ps.customRGBA[0] = 173;
						NPC->client->ps.customRGBA[1] = 142;
						NPC->client->ps.customRGBA[2] = 219;
						break;
					case 2://brown1
						NPC->client->ps.customRGBA[0] = 254;
						NPC->client->ps.customRGBA[1] = 197;
						NPC->client->ps.customRGBA[2] = 73;
						break;
					case 3://orange
						NPC->client->ps.customRGBA[0] = 138;
						NPC->client->ps.customRGBA[1] = 83;
						NPC->client->ps.customRGBA[2] = 0;
						break;
					case 4://gold
						NPC->client->ps.customRGBA[0] = 254;
						NPC->client->ps.customRGBA[1] = 199;
						NPC->client->ps.customRGBA[2] = 14;
						break;
					case 5://blue2
						NPC->client->ps.customRGBA[0] = 68;
						NPC->client->ps.customRGBA[1] = 194;
						NPC->client->ps.customRGBA[2] = 217;
						break;
					case 6://red1
						NPC->client->ps.customRGBA[0] = 170;
						NPC->client->ps.customRGBA[1] = 3;
						NPC->client->ps.customRGBA[2] = 30;
						break;
					case 7://yellow1
						NPC->client->ps.customRGBA[0] = 225;
						NPC->client->ps.customRGBA[1] = 226;
						NPC->client->ps.customRGBA[2] = 144;
						break;
					case 8://violet2
						NPC->client->ps.customRGBA[0] = 167;
						NPC->client->ps.customRGBA[1] = 202;
						NPC->client->ps.customRGBA[2] = 255;
						break;
					}
				}
				else if ( !Q_stricmp( value, "jedi_rm" ) )
				{
					NPC->client->ps.customRGBA[3] = 255;
					switch ( Q_irand( 0, 8 ) )
					{
					default:
					case 0://blue
						NPC->client->ps.customRGBA[0] = 127;
						NPC->client->ps.customRGBA[1] = 153;
						NPC->client->ps.customRGBA[2] = 255;
						break;
					case 1://green1
						NPC->client->ps.customRGBA[0] = 208;
						NPC->client->ps.customRGBA[1] = 249;
						NPC->client->ps.customRGBA[2] = 85;
						break;
					case 2://blue2
						NPC->client->ps.customRGBA[0] = 181;
						NPC->client->ps.customRGBA[1] = 207;
						NPC->client->ps.customRGBA[2] = 255;
						break;
					case 3://gold
						NPC->client->ps.customRGBA[0] = 138;
						NPC->client->ps.customRGBA[1] = 83;
						NPC->client->ps.customRGBA[2] = 0;
						break;
					case 4://gold
						NPC->client->ps.customRGBA[0] = 224;
						NPC->client->ps.customRGBA[1] = 171;
						NPC->client->ps.customRGBA[2] = 44;
						break;
					case 5://green2
						NPC->client->ps.customRGBA[0] = 49;
						NPC->client->ps.customRGBA[1] = 155;
						NPC->client->ps.customRGBA[2] = 131;
						break;
					case 6://red1
						NPC->client->ps.customRGBA[0] = 163;
						NPC->client->ps.customRGBA[1] = 79;
						NPC->client->ps.customRGBA[2] = 17;
						break;
					case 7://violet2
						NPC->client->ps.customRGBA[0] = 148;
						NPC->client->ps.customRGBA[1] = 104;
						NPC->client->ps.customRGBA[2] = 228;
						break;
					case 8://green3
						NPC->client->ps.customRGBA[0] = 138;
						NPC->client->ps.customRGBA[1] = 136;
						NPC->client->ps.customRGBA[2] = 0;
						break;
					}
				}
				else if ( !Q_stricmp( value, "jedi_tf" ) )
				{
					NPC->client->ps.customRGBA[3] = 255;
					switch ( Q_irand( 0, 5 ) )
					{
					default:
					case 0://green1
						NPC->client->ps.customRGBA[0] = 255;
						NPC->client->ps.customRGBA[1] = 235;
						NPC->client->ps.customRGBA[2] = 100;
						break;
					case 1://blue1
						NPC->client->ps.customRGBA[0] = 62;
						NPC->client->ps.customRGBA[1] = 155;
						NPC->client->ps.customRGBA[2] = 255;
						break;
					case 2://red1
						NPC->client->ps.customRGBA[0] = 255;
						NPC->client->ps.customRGBA[1] = 110;
						NPC->client->ps.customRGBA[2] = 120;
						break;
					case 3://purple
						NPC->client->ps.customRGBA[0] = 180;
						NPC->client->ps.customRGBA[1] = 150;
						NPC->client->ps.customRGBA[2] = 255;
						break;
					case 4://flesh
						NPC->client->ps.customRGBA[0] = 255;
						NPC->client->ps.customRGBA[1] = 200;
						NPC->client->ps.customRGBA[2] = 212;
						break;
					case 5://base
						NPC->client->ps.customRGBA[0] = 255;
						NPC->client->ps.customRGBA[1] = 255;
						NPC->client->ps.customRGBA[2] = 255;
						break;
					}
				}
				else if ( !Q_stricmp( value, "jedi_zf" ) )
				{
					NPC->client->ps.customRGBA[3] = 255;
					switch ( Q_irand( 0, 7 ) )
					{
					default:
					case 0://red1
						NPC->client->ps.customRGBA[0] = 204;
						NPC->client->ps.customRGBA[1] = 19;
						NPC->client->ps.customRGBA[2] = 21;
						break;
					case 1://orange1
						NPC->client->ps.customRGBA[0] = 255;
						NPC->client->ps.customRGBA[1] = 107;
						NPC->client->ps.customRGBA[2] = 40;
						break;
					case 2://pink1
						NPC->client->ps.customRGBA[0] = 255;
						NPC->client->ps.customRGBA[1] = 148;
						NPC->client->ps.customRGBA[2] = 155;
						break;
					case 3://gold
						NPC->client->ps.customRGBA[0] = 255;
						NPC->client->ps.customRGBA[1] = 164;
						NPC->client->ps.customRGBA[2] = 59;
						break;
					case 4://violet1
						NPC->client->ps.customRGBA[0] = 216;
						NPC->client->ps.customRGBA[1] = 160;
						NPC->client->ps.customRGBA[2] = 255;
						break;
					case 5://blue1
						NPC->client->ps.customRGBA[0] = 101;
						NPC->client->ps.customRGBA[1] = 159;
						NPC->client->ps.customRGBA[2] = 255;
						break;
					case 6://blue2
						NPC->client->ps.customRGBA[0] = 161;
						NPC->client->ps.customRGBA[1] = 226;
						NPC->client->ps.customRGBA[2] = 240;
						break;
					case 7://blue3
						NPC->client->ps.customRGBA[0] = 37;
						NPC->client->ps.customRGBA[1] = 155;
						NPC->client->ps.customRGBA[2] = 181;
						break;
					}
				}
				else
				{
					NPC->client->ps.customRGBA[0]=atoi(value);

					if ( COM_ParseInt( &p, &n ) )
					{
						continue;
					}
					NPC->client->ps.customRGBA[1]=n;

					if ( COM_ParseInt( &p, &n ) )
					{
						continue;
					}
					NPC->client->ps.customRGBA[2]=n;

					if ( COM_ParseInt( &p, &n ) )
					{
						continue;
					}
					NPC->client->ps.customRGBA[3]=n;
				}
				continue;
			}

			// headmodel
			if ( !Q_stricmp( token, "headmodel" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}

				if(!Q_stricmp("none", value))
				{
					//Zero the head clamp range so the torso & legs don't lag behind
					ri->headYawRangeLeft =
					ri->headYawRangeRight =
					ri->headPitchRangeUp =
					ri->headPitchRangeDown = 0;
				}
				continue;
			}

			// torsomodel
			if ( !Q_stricmp( token, "torsomodel" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}

				if(!Q_stricmp("none", value))
				{
					//Zero the torso clamp range so the legs don't lag behind
					ri->torsoYawRangeLeft =
					ri->torsoYawRangeRight =
					ri->torsoPitchRangeUp =
					ri->torsoPitchRangeDown = 0;
				}
				continue;
			}

			// legsmodel
			if ( !Q_stricmp( token, "legsmodel" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				/*
				Q_strncpyz( ri->legsModelName, value, sizeof(ri->legsModelName), qtrue);
				//Need to do this here to get the right index
				G_ParseAnimFileSet( ri->legsModelName, ri->legsModelName, &ci->animFileIndex );
				*/
				continue;
			}

			// playerModel
			if ( !Q_stricmp( token, "playerModel" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				Q_strncpyz( playerModel, value, sizeof(playerModel));
				md3Model = qfalse;
				continue;
			}

			// customSkin
			if ( !Q_stricmp( token, "customSkin" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				Q_strncpyz( customSkin, value, sizeof(customSkin));
				continue;
			}

			// surfOff
			if ( !Q_stricmp( token, "surfOff" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				if ( surfOff[0] )
				{
					Q_strcat( (char *)surfOff, sizeof(surfOff), "," );
					Q_strcat( (char *)surfOff, sizeof(surfOff), value );
				}
				else
				{
					Q_strncpyz( surfOff, value, sizeof(surfOff));
				}
				continue;
			}

			// surfOn
			if ( !Q_stricmp( token, "surfOn" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				if ( surfOn[0] )
				{
					Q_strcat( (char *)surfOn, sizeof(surfOn), "," );
					Q_strcat( (char *)surfOn, sizeof(surfOn), value );
				}
				else
				{
					Q_strncpyz( surfOn, value, sizeof(surfOn));
				}
				continue;
			}

			//headYawRangeLeft
			if ( !Q_stricmp( token, "headYawRangeLeft" ) )
			{
				if ( COM_ParseInt( &p, &n ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				if ( n < 0 )
				{
					Com_Printf( S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName );
					continue;
				}
				ri->headYawRangeLeft = n;
				continue;
			}

			//headYawRangeRight
			if ( !Q_stricmp( token, "headYawRangeRight" ) )
			{
				if ( COM_ParseInt( &p, &n ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				if ( n < 0 )
				{
					Com_Printf( S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName );
					continue;
				}
				ri->headYawRangeRight = n;
				continue;
			}

			//headPitchRangeUp
			if ( !Q_stricmp( token, "headPitchRangeUp" ) )
			{
				if ( COM_ParseInt( &p, &n ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				if ( n < 0 )
				{
					Com_Printf( S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName );
					continue;
				}
				ri->headPitchRangeUp = n;
				continue;
			}

			//headPitchRangeDown
			if ( !Q_stricmp( token, "headPitchRangeDown" ) )
			{
				if ( COM_ParseInt( &p, &n ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				if ( n < 0 )
				{
					Com_Printf( S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName );
					continue;
				}
				ri->headPitchRangeDown = n;
				continue;
			}

			//torsoYawRangeLeft
			if ( !Q_stricmp( token, "torsoYawRangeLeft" ) )
			{
				if ( COM_ParseInt( &p, &n ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				if ( n < 0 )
				{
					Com_Printf( S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName );
					continue;
				}
				ri->torsoYawRangeLeft = n;
				continue;
			}

			//torsoYawRangeRight
			if ( !Q_stricmp( token, "torsoYawRangeRight" ) )
			{
				if ( COM_ParseInt( &p, &n ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				if ( n < 0 )
				{
					Com_Printf( S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName );
					continue;
				}
				ri->torsoYawRangeRight = n;
				continue;
			}

			//torsoPitchRangeUp
			if ( !Q_stricmp( token, "torsoPitchRangeUp" ) )
			{
				if ( COM_ParseInt( &p, &n ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				if ( n < 0 )
				{
					Com_Printf( S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName );
					continue;
				}
				ri->torsoPitchRangeUp = n;
				continue;
			}

			//torsoPitchRangeDown
			if ( !Q_stricmp( token, "torsoPitchRangeDown" ) )
			{
				if ( COM_ParseInt( &p, &n ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				if ( n < 0 )
				{
					Com_Printf( S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName );
					continue;
				}
				ri->torsoPitchRangeDown = n;
				continue;
			}

			// Uniform XYZ scale
			if ( !Q_stricmp( token, "scale" ) )
			{
				if ( COM_ParseInt( &p, &n ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				if ( n < 0 )
				{
					Com_Printf(  "bad %s in NPC '%s'\n", token, NPCName );
					continue;
				}
				if (n != 100)
				{
					NPC->client->ps.iModelScale = n; //so the client knows
					if (n >= 1024)
					{
						Com_Printf("WARNING: MP does not support scaling up to or over 1024%\n");
						n = 1023;
					}

					NPC->modelScale[0] = NPC->modelScale[1] = NPC->modelScale[2] = n/100.0f;
				}
				continue;
			}

			//X scale
			if ( !Q_stricmp( token, "scaleX" ) )
			{
				if ( COM_ParseInt( &p, &n ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				if ( n < 0 )
				{
					Com_Printf(  "bad %s in NPC '%s'\n", token, NPCName );
					continue;
				}
				if (n != 100)
				{
					Com_Printf("MP doesn't support xyz scaling, use 'scale'.\n");
					//NPC->s.modelScale[0] = n/100.0f;
				}
				continue;
			}

			//Y scale
			if ( !Q_stricmp( token, "scaleY" ) )
			{
				if ( COM_ParseInt( &p, &n ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				if ( n < 0 )
				{
					Com_Printf(  "bad %s in NPC '%s'\n", token, NPCName );
					continue;
				}
				if (n != 100)
				{
					Com_Printf("MP doesn't support xyz scaling, use 'scale'.\n");
					//NPC->s.modelScale[1] = n/100.0f;
				}
				continue;
			}

			//Z scale
			if ( !Q_stricmp( token, "scaleZ" ) )
			{
				if ( COM_ParseInt( &p, &n ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				if ( n < 0 )
				{
					Com_Printf(  "bad %s in NPC '%s'\n", token, NPCName );
					continue;
				}
				if (n != 100)
				{
					Com_Printf("MP doesn't support xyz scaling, use 'scale'.\n");
				//	NPC->s.modelScale[2] = n/100.0f;
				}
				continue;
			}

	//===AI STATS=====================================================================
			if ( !parsingPlayer )
			{
				// aggression
				if ( !Q_stricmp( token, "aggression" ) ) {
					if ( COM_ParseInt( &p, &n ) ) {
						SkipRestOfLine( &p );
						continue;
					}
					if ( n < 1 || n > 5 ) {
						Com_Printf(  "bad %s in NPC '%s'\n", token, NPCName );
						continue;
					}
					if ( NPC->NPC )
					{
						stats->aggression = n;
					}
					continue;
				}

				// aim
				if ( !Q_stricmp( token, "aim" ) ) {
					if ( COM_ParseInt( &p, &n ) ) {
						SkipRestOfLine( &p );
						continue;
					}
					if ( n < 1 || n > 5 ) {
						Com_Printf( "bad %s in NPC '%s'\n", token, NPCName );
						continue;
					}
					if ( NPC->NPC )
					{
						stats->aim = n;
					}
					continue;
				}

				// earshot
				if ( !Q_stricmp( token, "earshot" ) ) {
					if ( COM_ParseFloat( &p, &f ) ) {
						SkipRestOfLine( &p );
						continue;
					}
					if ( f < 0.0f )
					{
						Com_Printf( "bad %s in NPC '%s'\n", token, NPCName );
						continue;
					}
					if ( NPC->NPC )
					{
						stats->earshot = f;
					}
					continue;
				}

				// evasion
				if ( !Q_stricmp( token, "evasion" ) )
				{
					if ( COM_ParseInt( &p, &n ) )
					{
						SkipRestOfLine( &p );
						continue;
					}
					if ( n < 1 || n > 5 )
					{
						Com_Printf( S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName );
						continue;
					}
					if ( NPC->NPC )
					{
						stats->evasion = n;
					}
					continue;
				}

				// hfov
				if ( !Q_stricmp( token, "hfov" ) ) {
					if ( COM_ParseInt( &p, &n ) ) {
						SkipRestOfLine( &p );
						continue;
					}
					if ( n < 30 || n > 180 ) {
						Com_Printf(  "bad %s in NPC '%s'\n", token, NPCName );
						continue;
					}
					if ( NPC->NPC )
					{
						stats->hfov = n;// / 2;	//FIXME: Why was this being done?!
					}
					continue;
				}

				// intelligence
				if ( !Q_stricmp( token, "intelligence" ) ) {
					if ( COM_ParseInt( &p, &n ) ) {
						SkipRestOfLine( &p );
						continue;
					}
					if ( n < 1 || n > 5 ) {
						Com_Printf(  "bad %s in NPC '%s'\n", token, NPCName );
						continue;
					}
					if ( NPC->NPC )
					{
						stats->intelligence = n;
					}
					continue;
				}

				// move
				if ( !Q_stricmp( token, "move" ) ) {
					if ( COM_ParseInt( &p, &n ) ) {
						SkipRestOfLine( &p );
						continue;
					}
					if ( n < 1 || n > 5 ) {
						Com_Printf(  "bad %s in NPC '%s'\n", token, NPCName );
						continue;
					}
					if ( NPC->NPC )
					{
						stats->move = n;
					}
					continue;
				}

				// reactions
				if ( !Q_stricmp( token, "reactions" ) ) {
					if ( COM_ParseInt( &p, &n ) ) {
						SkipRestOfLine( &p );
						continue;
					}
					if ( n < 1 || n > 5 ) {
						Com_Printf( "bad %s in NPC '%s'\n", token, NPCName );
						continue;
					}
					if ( NPC->NPC )
					{
						stats->reactions = n;
					}
					continue;
				}

				// shootDistance
				if ( !Q_stricmp( token, "shootDistance" ) ) {
					if ( COM_ParseFloat( &p, &f ) ) {
						SkipRestOfLine( &p );
						continue;
					}
					if ( f < 0.0f )
					{
						Com_Printf( "bad %s in NPC '%s'\n", token, NPCName );
						continue;
					}
					if ( NPC->NPC )
					{
						stats->shootDistance = f;
					}
					continue;
				}

				// vfov
				if ( !Q_stricmp( token, "vfov" ) ) {
					if ( COM_ParseInt( &p, &n ) ) {
						SkipRestOfLine( &p );
						continue;
					}
					if ( n < 30 || n > 180 ) {
						Com_Printf(  "bad %s in NPC '%s'\n", token, NPCName );
						continue;
					}
					if ( NPC->NPC )
					{
						stats->vfov = n / 2;
					}
					continue;
				}

				// vigilance
				if ( !Q_stricmp( token, "vigilance" ) ) {
					if ( COM_ParseFloat( &p, &f ) ) {
						SkipRestOfLine( &p );
						continue;
					}
					if ( f < 0.0f )
					{
						Com_Printf( "bad %s in NPC '%s'\n", token, NPCName );
						continue;
					}
					if ( NPC->NPC )
					{
						stats->vigilance = f;
					}
					continue;
				}

				// visrange
				if ( !Q_stricmp( token, "visrange" ) ) {
					if ( COM_ParseFloat( &p, &f ) ) {
						SkipRestOfLine( &p );
						continue;
					}
					if ( f < 0.0f )
					{
						Com_Printf( "bad %s in NPC '%s'\n", token, NPCName );
						continue;
					}
					if ( NPC->NPC )
					{
						stats->visrange = f;
					}
					continue;
				}

				// race
		//		if ( !Q_stricmp( token, "race" ) )
		//		{
		//			if ( COM_ParseString( &p, &value ) )
		//			{
		//				continue;
		//			}
		//			NPC->client->race = TranslateRaceName(value);
		//			continue;
		//		}

				// rank
				if ( !Q_stricmp( token, "rank" ) )
				{
					if ( COM_ParseString( &p, &value ) )
					{
						continue;
					}
					if ( NPC->NPC )
					{
						NPC->NPC->rank = TranslateRankName(value);
					}
					continue;
				}
			}

			// health
			if ( !Q_stricmp( token, "health" ) )
			{
				if ( COM_ParseInt( &p, &n ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				if ( n < 0 )
				{
					Com_Printf( S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName );
					continue;
				}
				if ( NPC->NPC )
				{
					stats->health = n;
				}
				else if ( parsingPlayer )
				{
					NPC->client->ps.stats[STAT_MAX_HEALTH] = NPC->client->pers.maxHealth = n;
				}
				continue;
			}

			// fullName
			if ( !Q_stricmp( token, "fullName" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				NPC->fullName = G_NewString(value);
				continue;
			}

			// playerTeam
			if ( !Q_stricmp( token, "playerTeam" ) )
			{
				char tk[4096]; //rww - hackilicious!

				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				Com_sprintf(tk, sizeof(tk), "NPC%s", token);
				NPC->client->playerTeam = NPC->s.teamowner = (team_t)GetIDForString( TeamTable, tk );//TranslateTeamName(value);
				continue;
			}

			// enemyTeam
			if ( !Q_stricmp( token, "enemyTeam" ) )
			{
				char tk[4096]; //rww - hackilicious!

				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				Com_sprintf(tk, sizeof(tk), "NPC%s", token);
				NPC->client->enemyTeam = GetIDForString( TeamTable, tk );//TranslateTeamName(value);
				continue;
			}

			// class
			if ( !Q_stricmp( token, "class" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				NPC->client->NPC_class = (class_t)GetIDForString( ClassTable, value );
				NPC->s.NPC_class = NPC->client->NPC_class; //we actually only need this value now, but at the moment I don't feel like changing the 200+ references to client->NPC_class.

				// No md3's for vehicles.
				if ( NPC->client->NPC_class == CLASS_VEHICLE )
				{
					if ( !NPC->m_pVehicle )
					{//you didn't spawn this guy right!
						Com_Printf ( S_COLOR_RED "ERROR: Tried to spawn a vehicle NPC (%s) without using NPC_Vehicle or 'NPC spawn vehicle <vehiclename>'!!!  Bad, bad, bad!  Shame on you!\n", NPCName );
						return qfalse;
					}
					md3Model = qfalse;
				}

				continue;
			}

			// dismemberment probability for head
			if ( !Q_stricmp( token, "dismemberProbHead" ) ) {
				if ( COM_ParseInt( &p, &n ) ) {
					SkipRestOfLine( &p );
					continue;
				}
				if ( n < 0 )
				{
					Com_Printf( "bad %s in NPC '%s'\n", token, NPCName );
					continue;
				}
				if ( NPC->NPC )
				{
				//	NPC->client->dismemberProbHead = n;
					//rwwFIXMEFIXME: support for this?
				}
				continue;
			}

			// dismemberment probability for arms
			if ( !Q_stricmp( token, "dismemberProbArms" ) ) {
				if ( COM_ParseInt( &p, &n ) ) {
					SkipRestOfLine( &p );
					continue;
				}
				if ( n < 0 )
				{
					Com_Printf( "bad %s in NPC '%s'\n", token, NPCName );
					continue;
				}
				if ( NPC->NPC )
				{
				//	NPC->client->dismemberProbArms = n;
				}
				continue;
			}

			// dismemberment probability for hands
			if ( !Q_stricmp( token, "dismemberProbHands" ) ) {
				if ( COM_ParseInt( &p, &n ) ) {
					SkipRestOfLine( &p );
					continue;
				}
				if ( n < 0 )
				{
					Com_Printf( "bad %s in NPC '%s'\n", token, NPCName );
					continue;
				}
				if ( NPC->NPC )
				{
				//	NPC->client->dismemberProbHands = n;
				}
				continue;
			}

			// dismemberment probability for waist
			if ( !Q_stricmp( token, "dismemberProbWaist" ) ) {
				if ( COM_ParseInt( &p, &n ) ) {
					SkipRestOfLine( &p );
					continue;
				}
				if ( n < 0 )
				{
					Com_Printf( "bad %s in NPC '%s'\n", token, NPCName );
					continue;
				}
				if ( NPC->NPC )
				{
				//	NPC->client->dismemberProbWaist = n;
				}
				continue;
			}

			// dismemberment probability for legs
			if ( !Q_stricmp( token, "dismemberProbLegs" ) ) {
				if ( COM_ParseInt( &p, &n ) ) {
					SkipRestOfLine( &p );
					continue;
				}
				if ( n < 0 )
				{
					Com_Printf( "bad %s in NPC '%s'\n", token, NPCName );
					continue;
				}
				if ( NPC->NPC )
				{
				//	NPC->client->dismemberProbLegs = n;
				}
				continue;
			}

	//===MOVEMENT STATS============================================================

			if ( !Q_stricmp( token, "width" ) )
			{
				if ( COM_ParseInt( &p, &n ) )
				{
					continue;
				}

				NPC->r.mins[0] = NPC->r.mins[1] = -n;
				NPC->r.maxs[0] = NPC->r.maxs[1] = n;
				continue;
			}

			if ( !Q_stricmp( token, "height" ) )
			{
				if ( COM_ParseInt( &p, &n ) )
				{
					continue;
				}
				if ( NPC->client->NPC_class == CLASS_VEHICLE
					&& NPC->m_pVehicle
					&& NPC->m_pVehicle->m_pVehicleInfo
					&& NPC->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER )
				{//a flying vehicle's origin must be centered in bbox and it should spawn on the ground
					//trace_t		tr;
					//vec3_t		bottom;
					//float		adjust = 32.0f;
					NPC->r.maxs[2] = NPC->client->ps.standheight = (n/2.0f);
					NPC->r.mins[2] = -NPC->r.maxs[2];
					NPC->s.origin[2] += (DEFAULT_MINS_2-NPC->r.mins[2])+0.125f;
					VectorCopy(NPC->s.origin, NPC->client->ps.origin);
					VectorCopy(NPC->s.origin, NPC->r.currentOrigin);
					G_SetOrigin( NPC, NPC->s.origin );
					trap->LinkEntity((sharedEntity_t *)NPC);
					//now trace down
					/*
					VectorCopy( NPC->s.origin, bottom );
					bottom[2] -= adjust;
					trap->Trace( &tr, NPC->s.origin, NPC->r.mins, NPC->r.maxs, bottom, NPC->s.number, MASK_NPCSOLID );
					if ( !tr.allsolid && !tr.startsolid )
					{
						G_SetOrigin( NPC, tr.endpos );
						trap->LinkEntity((sharedEntity_t *)NPC);
					}
					*/
				}
				else
				{
					NPC->r.mins[2] = DEFAULT_MINS_2;//Cannot change
					NPC->r.maxs[2] = NPC->client->ps.standheight = n + DEFAULT_MINS_2;
				}
				NPC->radius = n;
				continue;
			}

			if ( !Q_stricmp( token, "crouchheight" ) )
			{
				if ( COM_ParseInt( &p, &n ) )
				{
					continue;
				}

				NPC->client->ps.crouchheight = n + DEFAULT_MINS_2;
				continue;
			}

			if ( !parsingPlayer )
			{
				if ( !Q_stricmp( token, "movetype" ) )
				{
					if ( COM_ParseString( &p, &value ) )
					{
						continue;
					}
					if ( Q_stricmp( "flyswim", value ) == 0 )
					{
						NPC->client->ps.eFlags2 |= EF2_FLYING;
					}
					//NPC->client->moveType = (movetype_t)MoveTypeNameToEnum(value);
					//rwwFIXMEFIXME: support for movetypes
					continue;
				}

				// yawSpeed
				if ( !Q_stricmp( token, "yawSpeed" ) ) {
					if ( COM_ParseInt( &p, &n ) ) {
						SkipRestOfLine( &p );
						continue;
					}
					if ( n <= 0) {
						Com_Printf(  "bad %s in NPC '%s'\n", token, NPCName );
						continue;
					}
					if ( NPC->NPC )
					{
						stats->yawSpeed = ((float)(n));
					}
					continue;
				}

				// walkSpeed
				if ( !Q_stricmp( token, "walkSpeed" ) )
				{
					if ( COM_ParseInt( &p, &n ) )
					{
						SkipRestOfLine( &p );
						continue;
					}
					if ( n < 0 )
					{
						Com_Printf( S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName );
						continue;
					}
					if ( NPC->NPC )
					{
						stats->walkSpeed = n;
					}
					continue;
				}

				//runSpeed
				if ( !Q_stricmp( token, "runSpeed" ) )
				{
					if ( COM_ParseInt( &p, &n ) )
					{
						SkipRestOfLine( &p );
						continue;
					}
					if ( n < 0 )
					{
						Com_Printf( S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName );
						continue;
					}
					if ( NPC->NPC )
					{
						stats->runSpeed = n;
					}
					continue;
				}

				//acceleration
				if ( !Q_stricmp( token, "acceleration" ) )
				{
					if ( COM_ParseInt( &p, &n ) )
					{
						SkipRestOfLine( &p );
						continue;
					}
					if ( n < 0 )
					{
						Com_Printf( S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName );
						continue;
					}
					if ( NPC->NPC )
					{
						stats->acceleration = n;
					}
					continue;
				}
				//sex - skip in MP
				if ( !Q_stricmp( token, "sex" ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
//===MISC===============================================================================
				// default behavior
				if ( !Q_stricmp( token, "behavior" ) )
				{
					if ( COM_ParseInt( &p, &n ) )
					{
						SkipRestOfLine( &p );
						continue;
					}
					if ( n < BS_DEFAULT || n >= NUM_BSTATES )
					{
						Com_Printf( S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName );
						continue;
					}
					if ( NPC->NPC )
					{
						NPC->NPC->defaultBehavior = (bState_t)(n);
					}
					continue;
				}
			}

			// snd
			if ( !Q_stricmp( token, "snd" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				if ( !(NPC->r.svFlags&SVF_NO_BASIC_SOUNDS) )
				{
					//FIXME: store this in some sound field or parse in the soundTable like the animTable...
					Q_strncpyz( sound, value, sizeof( sound ) );
					patch = strstr( sound, "/" );
					if ( patch )
					{
						*patch = 0;
					}
				//	ci->customBasicSoundDir = G_NewString( sound );
					//rwwFIXMEFIXME: Hooray for violating client server rules
				}
				continue;
			}

			// sndcombat
			if ( !Q_stricmp( token, "sndcombat" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				if ( !(NPC->r.svFlags&SVF_NO_COMBAT_SOUNDS) )
				{
					//FIXME: store this in some sound field or parse in the soundTable like the animTable...
					Q_strncpyz( sound, value, sizeof( sound ) );
					patch = strstr( sound, "/" );
					if ( patch )
					{
						*patch = 0;
					}
				//	ci->customCombatSoundDir = G_NewString( sound );
				}
				continue;
			}

			// sndextra
			if ( !Q_stricmp( token, "sndextra" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				if ( !(NPC->r.svFlags&SVF_NO_EXTRA_SOUNDS) )
				{
					//FIXME: store this in some sound field or parse in the soundTable like the animTable...
					Q_strncpyz( sound, value, sizeof( sound ) );
					patch = strstr( sound, "/" );
					if ( patch )
					{
						*patch = 0;
					}
				//	ci->customExtraSoundDir = G_NewString( sound );
				}
				continue;
			}

			// sndjedi
			if ( !Q_stricmp( token, "sndjedi" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				if ( !(NPC->r.svFlags&SVF_NO_EXTRA_SOUNDS) )
				{
					//FIXME: store this in some sound field or parse in the soundTable like the animTable...
					Q_strncpyz( sound, value, sizeof( sound ) );
					patch = strstr( sound, "/" );
					if ( patch )
					{
						*patch = 0;
					}
					//ci->customJediSoundDir = G_NewString( sound );
				}
				continue;
			}

			//New NPC/jedi stats:
			//starting weapon
			if ( !Q_stricmp( token, "weapon" ) )
			{
				int weap;

				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				//FIXME: need to precache the weapon, too?  (in above func)
				weap = GetIDForString( WPTable, value );
				if ( weap >= WP_NONE && weap <= WP_NUM_WEAPONS )///*WP_BLASTER_PISTOL*/WP_SABER ) //?!
				{
					NPC->client->ps.weapon = weap;
					NPC->client->ps.stats[STAT_WEAPONS] |= ( 1 << NPC->client->ps.weapon );
					if ( weap > WP_NONE )
					{
					//	RegisterItem( FindItemForWeapon( (weapon_t)(NPC->client->ps.weapon) ) );	//precache the weapon
						NPC->client->ps.ammo[weaponData[NPC->client->ps.weapon].ammoIndex] = 100;//FIXME: max ammo!
					}
				}
				continue;
			}

			if ( !parsingPlayer )
			{
				//altFire
				if ( !Q_stricmp( token, "altFire" ) )
				{
					if ( COM_ParseInt( &p, &n ) )
					{
						SkipRestOfLine( &p );
						continue;
					}
					if ( NPC->NPC )
					{
						if ( n != 0 )
						{
							NPC->NPC->scriptFlags |= SCF_ALT_FIRE;
						}
					}
					continue;
				}
				//Other unique behaviors/numbers that are currently hardcoded?
			}

			//force powers
			fp = GetIDForString( FPTable, token );
			if ( fp >= FP_FIRST && fp < NUM_FORCE_POWERS )
			{
				if ( COM_ParseInt( &p, &n ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				//FIXME: need to precache the fx, too?  (in above func)
				//cap
				if ( n > 5 )
				{
					n = 5;
				}
				else if ( n < 0 )
				{
					n = 0;
				}
				if ( n )
				{//set
					NPC->client->ps.fd.forcePowersKnown |= ( 1 << fp );
				}
				else
				{//clear
					NPC->client->ps.fd.forcePowersKnown &= ~( 1 << fp );
				}
				NPC->client->ps.fd.forcePowerLevel[fp] = n;
				continue;
			}

			//max force power
			if ( !Q_stricmp( token, "forcePowerMax" ) )
			{
				if ( COM_ParseInt( &p, &n ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				NPC->client->ps.fd.forcePowerMax = n;
				continue;
			}

			//force regen rate - default is 100ms
			if ( !Q_stricmp( token, "forceRegenRate" ) )
			{
				if ( COM_ParseInt( &p, &n ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				//NPC->client->ps.forcePowerRegenRate = n;
				//rwwFIXMEFIXME: support this?
				continue;
			}

			//force regen amount - default is 1 (points per second)
			if ( !Q_stricmp( token, "forceRegenAmount" ) )
			{
				if ( COM_ParseInt( &p, &n ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				//NPC->client->ps.forcePowerRegenAmount = n;
				//rwwFIXMEFIXME: support this?
				continue;
			}

			//have a sabers.cfg and just name your saber in your NPCs.cfg/ICARUS script
			//saber name
			if ( !Q_stricmp( token, "saber" ) )
			{
				char *saberName;

				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}

				saberName = (char *)BG_TempAlloc(4096);//G_NewString( value );
				strcpy(saberName, value);

				WP_SaberParseParms( saberName, &NPC->client->saber[0] );
				npcSaber1 = G_ModelIndex(va("@%s", saberName));

				BG_TempFree(4096);
				continue;
			}

			//second saber name
			if ( !Q_stricmp( token, "saber2" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}

				if ( !(NPC->client->saber[0].saberFlags&SFL_TWO_HANDED) )
				{//can't use a second saber if first one is a two-handed saber...?
					char *saberName = (char *)BG_TempAlloc(4096);//G_NewString( value );
					strcpy(saberName, value);

					WP_SaberParseParms( saberName, &NPC->client->saber[1] );
					if ( (NPC->client->saber[1].saberFlags&SFL_TWO_HANDED) )
					{//tsk tsk, can't use a twoHanded saber as second saber
						WP_RemoveSaber( NPC->client->saber, 1 );
					}
					else
					{
						//NPC->client->ps.dualSabers = qtrue;
						npcSaber2 = G_ModelIndex(va("@%s", saberName));
					}
					BG_TempFree(4096);
				}
				continue;
			}

			// saberColor
			if ( !Q_stricmp( token, "saberColor" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				if ( NPC->client )
				{
					saber_colors_t color = TranslateSaberColor( value );
					for ( n = 0; n < MAX_BLADES; n++ )
					{
						NPC->client->saber[0].blade[n].color = color;
						NPC->s.boltToPlayer = NPC->s.boltToPlayer & 0x38;//(111000)
						NPC->s.boltToPlayer += (color + 1);
					}
				}
				continue;
			}

			if ( !Q_stricmp( token, "saberColor2" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				if ( NPC->client )
				{
					NPC->client->saber[0].blade[1].color = TranslateSaberColor( value );
				}
				continue;
			}

			if ( !Q_stricmp( token, "saberColor3" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				if ( NPC->client )
				{
					NPC->client->saber[0].blade[2].color = TranslateSaberColor( value );
				}
				continue;
			}

			if ( !Q_stricmp( token, "saberColor4" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				if ( NPC->client )
				{
					NPC->client->saber[0].blade[3].color = TranslateSaberColor( value );
				}
				continue;
			}

			if ( !Q_stricmp( token, "saberColor5" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				if ( NPC->client )
				{
					NPC->client->saber[0].blade[4].color = TranslateSaberColor( value );
				}
				continue;
			}

			if ( !Q_stricmp( token, "saberColor6" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				if ( NPC->client )
				{
					NPC->client->saber[0].blade[5].color = TranslateSaberColor( value );
				}
				continue;
			}

			if ( !Q_stricmp( token, "saberColor7" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				if ( NPC->client )
				{
					NPC->client->saber[0].blade[6].color = TranslateSaberColor( value );
				}
				continue;
			}

			if ( !Q_stricmp( token, "saberColor8" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				if ( NPC->client )
				{
					NPC->client->saber[0].blade[7].color = TranslateSaberColor( value );
				}
				continue;
			}

			if ( !Q_stricmp( token, "saber2Color" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				if ( NPC->client )
				{
					saber_colors_t color = TranslateSaberColor( value );
					for ( n = 0; n < MAX_BLADES; n++ )
					{
						NPC->client->saber[1].blade[n].color = color;
						NPC->s.boltToPlayer = NPC->s.boltToPlayer & 0x7;//(000111)
						NPC->s.boltToPlayer += ((color + 1) << 3);
					}
				}
				continue;
			}

			if ( !Q_stricmp( token, "saber2Color2" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				if ( NPC->client )
				{
					NPC->client->saber[1].blade[1].color = TranslateSaberColor( value );
				}
				continue;
			}

			if ( !Q_stricmp( token, "saber2Color3" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				if ( NPC->client )
				{
					NPC->client->saber[1].blade[2].color = TranslateSaberColor( value );
				}
				continue;
			}

			if ( !Q_stricmp( token, "saber2Color4" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				if ( NPC->client )
				{
					NPC->client->saber[1].blade[3].color = TranslateSaberColor( value );
				}
				continue;
			}

			if ( !Q_stricmp( token, "saber2Color5" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				if ( NPC->client )
				{
					NPC->client->saber[1].blade[4].color = TranslateSaberColor( value );
				}
				continue;
			}

			if ( !Q_stricmp( token, "saber2Color6" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				if ( NPC->client )
				{
					NPC->client->saber[1].blade[5].color = TranslateSaberColor( value );
				}
				continue;
			}

			if ( !Q_stricmp( token, "saber2Color7" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				if ( NPC->client )
				{
					NPC->client->saber[1].blade[6].color = TranslateSaberColor( value );
				}
				continue;
			}

			if ( !Q_stricmp( token, "saber2Color8" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				if ( NPC->client )
				{
					NPC->client->saber[1].blade[7].color = TranslateSaberColor( value );
				}
				continue;
			}

			//saber length
			if ( !Q_stricmp( token, "saberLength" ) )
			{
				if ( COM_ParseFloat( &p, &f ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				//cap
				if ( f < 4.0f )
				{
					f = 4.0f;
				}

				for ( n = 0; n < MAX_BLADES; n++ )
				{
					NPC->client->saber[0].blade[n].lengthMax = f;
				}
				continue;
			}

			if ( !Q_stricmp( token, "saberLength2" ) )
			{
				if ( COM_ParseFloat( &p, &f ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				//cap
				if ( f < 4.0f )
				{
					f = 4.0f;
				}
				NPC->client->saber[0].blade[1].lengthMax = f;
				continue;
			}

			if ( !Q_stricmp( token, "saberLength3" ) )
			{
				if ( COM_ParseFloat( &p, &f ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				//cap
				if ( f < 4.0f )
				{
					f = 4.0f;
				}
				NPC->client->saber[0].blade[2].lengthMax = f;
				continue;
			}

			if ( !Q_stricmp( token, "saberLength4" ) )
			{
				if ( COM_ParseFloat( &p, &f ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				//cap
				if ( f < 4.0f )
				{
					f = 4.0f;
				}
				NPC->client->saber[0].blade[3].lengthMax = f;
				continue;
			}

			if ( !Q_stricmp( token, "saberLength5" ) )
			{
				if ( COM_ParseFloat( &p, &f ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				//cap
				if ( f < 4.0f )
				{
					f = 4.0f;
				}
				NPC->client->saber[0].blade[4].lengthMax = f;
				continue;
			}

			if ( !Q_stricmp( token, "saberLength6" ) )
			{
				if ( COM_ParseFloat( &p, &f ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				//cap
				if ( f < 4.0f )
				{
					f = 4.0f;
				}
				NPC->client->saber[0].blade[5].lengthMax = f;
				continue;
			}

			if ( !Q_stricmp( token, "saberLength7" ) )
			{
				if ( COM_ParseFloat( &p, &f ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				//cap
				if ( f < 4.0f )
				{
					f = 4.0f;
				}
				NPC->client->saber[0].blade[6].lengthMax = f;
				continue;
			}

			if ( !Q_stricmp( token, "saberLength8" ) )
			{
				if ( COM_ParseFloat( &p, &f ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				//cap
				if ( f < 4.0f )
				{
					f = 4.0f;
				}
				NPC->client->saber[0].blade[7].lengthMax = f;
				continue;
			}

			if ( !Q_stricmp( token, "saber2Length" ) )
			{
				if ( COM_ParseFloat( &p, &f ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				//cap
				if ( f < 4.0f )
				{
					f = 4.0f;
				}
				for ( n = 0; n < MAX_BLADES; n++ )
				{
					NPC->client->saber[1].blade[n].lengthMax = f;
				}
				continue;
			}

			if ( !Q_stricmp( token, "saber2Length2" ) )
			{
				if ( COM_ParseFloat( &p, &f ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				//cap
				if ( f < 4.0f )
				{
					f = 4.0f;
				}
				NPC->client->saber[1].blade[1].lengthMax = f;
				continue;
			}

			if ( !Q_stricmp( token, "saber2Length3" ) )
			{
				if ( COM_ParseFloat( &p, &f ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				//cap
				if ( f < 4.0f )
				{
					f = 4.0f;
				}
				NPC->client->saber[1].blade[2].lengthMax = f;
				continue;
			}

			if ( !Q_stricmp( token, "saber2Length4" ) )
			{
				if ( COM_ParseFloat( &p, &f ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				//cap
				if ( f < 4.0f )
				{
					f = 4.0f;
				}
				NPC->client->saber[1].blade[3].lengthMax = f;
				continue;
			}

			if ( !Q_stricmp( token, "saber2Length5" ) )
			{
				if ( COM_ParseFloat( &p, &f ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				//cap
				if ( f < 4.0f )
				{
					f = 4.0f;
				}
				NPC->client->saber[1].blade[4].lengthMax = f;
				continue;
			}

			if ( !Q_stricmp( token, "saber2Length6" ) )
			{
				if ( COM_ParseFloat( &p, &f ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				//cap
				if ( f < 4.0f )
				{
					f = 4.0f;
				}
				NPC->client->saber[1].blade[5].lengthMax = f;
				continue;
			}

			if ( !Q_stricmp( token, "saber2Length7" ) )
			{
				if ( COM_ParseFloat( &p, &f ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				//cap
				if ( f < 4.0f )
				{
					f = 4.0f;
				}
				NPC->client->saber[1].blade[6].lengthMax = f;
				continue;
			}

			if ( !Q_stricmp( token, "saber2Length8" ) )
			{
				if ( COM_ParseFloat( &p, &f ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				//cap
				if ( f < 4.0f )
				{
					f = 4.0f;
				}
				NPC->client->saber[1].blade[7].lengthMax = f;
				continue;
			}

			//saber radius
			if ( !Q_stricmp( token, "saberRadius" ) )
			{
				if ( COM_ParseFloat( &p, &f ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				//cap
				if ( f < 0.25f )
				{
					f = 0.25f;
				}
				for ( n = 0; n < MAX_BLADES; n++ )
				{
					NPC->client->saber[0].blade[n].radius = f;
				}
				continue;
			}

			if ( !Q_stricmp( token, "saberRadius2" ) )
			{
				if ( COM_ParseFloat( &p, &f ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				//cap
				if ( f < 0.25f )
				{
					f = 0.25f;
				}
				NPC->client->saber[0].blade[1].radius = f;
				continue;
			}

			if ( !Q_stricmp( token, "saberRadius3" ) )
			{
				if ( COM_ParseFloat( &p, &f ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				//cap
				if ( f < 0.25f )
				{
					f = 0.25f;
				}
				NPC->client->saber[0].blade[2].radius = f;
				continue;
			}

			if ( !Q_stricmp( token, "saberRadius4" ) )
			{
				if ( COM_ParseFloat( &p, &f ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				//cap
				if ( f < 0.25f )
				{
					f = 0.25f;
				}
				NPC->client->saber[0].blade[3].radius = f;
				continue;
			}

			if ( !Q_stricmp( token, "saberRadius5" ) )
			{
				if ( COM_ParseFloat( &p, &f ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				//cap
				if ( f < 0.25f )
				{
					f = 0.25f;
				}
				NPC->client->saber[0].blade[4].radius = f;
				continue;
			}

			if ( !Q_stricmp( token, "saberRadius6" ) )
			{
				if ( COM_ParseFloat( &p, &f ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				//cap
				if ( f < 0.25f )
				{
					f = 0.25f;
				}
				NPC->client->saber[0].blade[5].radius = f;
				continue;
			}

			if ( !Q_stricmp( token, "saberRadius7" ) )
			{
				if ( COM_ParseFloat( &p, &f ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				//cap
				if ( f < 0.25f )
				{
					f = 0.25f;
				}
				NPC->client->saber[0].blade[6].radius = f;
				continue;
			}

			if ( !Q_stricmp( token, "saberRadius8" ) )
			{
				if ( COM_ParseFloat( &p, &f ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				//cap
				if ( f < 0.25f )
				{
					f = 0.25f;
				}
				NPC->client->saber[0].blade[7].radius = f;
				continue;
			}

			if ( !Q_stricmp( token, "saber2Radius" ) )
			{
				if ( COM_ParseFloat( &p, &f ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				//cap
				if ( f < 0.25f )
				{
					f = 0.25f;
				}
				for ( n = 0; n < MAX_BLADES; n++ )
				{
					NPC->client->saber[1].blade[n].radius = f;
				}
				continue;
			}

			if ( !Q_stricmp( token, "saber2Radius2" ) )
			{
				if ( COM_ParseFloat( &p, &f ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				//cap
				if ( f < 0.25f )
				{
					f = 0.25f;
				}
				NPC->client->saber[1].blade[1].radius = f;
				continue;
			}

			if ( !Q_stricmp( token, "saber2Radius3" ) )
			{
				if ( COM_ParseFloat( &p, &f ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				//cap
				if ( f < 0.25f )
				{
					f = 0.25f;
				}
				NPC->client->saber[1].blade[2].radius = f;
				continue;
			}

			if ( !Q_stricmp( token, "saber2Radius4" ) )
			{
				if ( COM_ParseFloat( &p, &f ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				//cap
				if ( f < 0.25f )
				{
					f = 0.25f;
				}
				NPC->client->saber[1].blade[3].radius = f;
				continue;
			}

			if ( !Q_stricmp( token, "saber2Radius5" ) )
			{
				if ( COM_ParseFloat( &p, &f ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				//cap
				if ( f < 0.25f )
				{
					f = 0.25f;
				}
				NPC->client->saber[1].blade[4].radius = f;
				continue;
			}

			if ( !Q_stricmp( token, "saber2Radius6" ) )
			{
				if ( COM_ParseFloat( &p, &f ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				//cap
				if ( f < 0.25f )
				{
					f = 0.25f;
				}
				NPC->client->saber[1].blade[5].radius = f;
				continue;
			}

			if ( !Q_stricmp( token, "saber2Radius7" ) )
			{
				if ( COM_ParseFloat( &p, &f ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				//cap
				if ( f < 0.25f )
				{
					f = 0.25f;
				}
				NPC->client->saber[1].blade[6].radius = f;
				continue;
			}

			if ( !Q_stricmp( token, "saber2Radius8" ) )
			{
				if ( COM_ParseFloat( &p, &f ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				//cap
				if ( f < 0.25f )
				{
					f = 0.25f;
				}
				NPC->client->saber[1].blade[7].radius = f;
				continue;
			}

			//ADD:
			//saber sounds (on, off, loop)
			//loop sound (like Vader's breathing or droid bleeps, etc.)

			//starting saber style
			if ( !Q_stricmp( token, "saberStyle" ) )
			{
				if ( COM_ParseInt( &p, &n ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				//cap
				if ( n < 0 )
				{
					n = 0;
				}
				else if ( n > 5 )
				{
					n = 5;
				}
				NPC->client->ps.fd.saberAnimLevel = n;
				/*
				if ( parsingPlayer )
				{
					cg.saberAnimLevelPending = n;
				}
				*/
				continue;
			}

			if ( !parsingPlayer )
			{
				Com_Printf( "WARNING: unknown keyword '%s' while parsing '%s'\n", token, NPCName );
			}
			SkipRestOfLine( &p );
		}
	}

/*
Ghoul2 Insert Start
*/
	if ( !md3Model )
	{
		qboolean setTypeBack = qfalse;

		if (npcSaber1 == 0)
		{ //use "kyle" for a default then
			npcSaber1 = G_ModelIndex("@Kyle");
			WP_SaberParseParms( DEFAULT_SABER, &NPC->client->saber[0] );
		}

		NPC->s.npcSaber1 = npcSaber1;
		NPC->s.npcSaber2 = npcSaber2;

		if (!customSkin[0])
		{
			strcpy(customSkin, "default");
		}

		if ( NPC->client && NPC->client->NPC_class == CLASS_VEHICLE )
		{ //vehicles want their names fed in as models
			//we put the $ in front to indicate a name and not a model
			strcpy(playerModel, va("$%s", NPCName));
		}
		SetupGameGhoul2Model(NPC, playerModel, customSkin);

		if (!NPC->NPC_type)
		{ //just do this for now so NPC_Precache can see the name.
			NPC->NPC_type = (char *)NPCName;
			setTypeBack = qtrue;
		}

		NPC_Precache(NPC); //this will just soundindex some values for sounds on the client,

		if (setTypeBack)
		{ //don't want this being set if we aren't ready yet.
			NPC->NPC_type = NULL;
		}
	}
	else
	{
		Com_Printf("MD3 MODEL NPC'S ARE NOT SUPPORTED IN MP!\n");
		return qfalse;
	}
/*
Ghoul2 Insert End
*/
	/*
	if(	NPCsPrecached )
	{//Spawning in after initial precache, our models are precached, we just need to set our clientInfo
		CG_RegisterClientModels( NPC->s.number );
		CG_RegisterNPCCustomSounds( ci );
		CG_RegisterNPCEffects( NPC->client->playerTeam );
	}
	*/
	//rwwFIXMEFIXME: Do something here I guess to properly precache stuff.

	return qtrue;
}

char npcParseBuffer[MAX_NPC_DATA_SIZE];

void NPC_LoadParms( void )
{
	int			len, totallen, npcExtFNLen, fileCnt, i;
//	const char	*filename = "ext_data/NPC2.cfg";
	char		/**buffer,*/ *holdChar, *marker;
	char		npcExtensionListBuf[2048];			//	The list of file names read in
	fileHandle_t f;
	len = 0;

	//remember where to store the next one
	totallen = len;
	marker = NPCParms+totallen;
	*marker = 0;

	//now load in the extra .npc extensions
	fileCnt = trap->FS_GetFileList("ext_data/NPCs", ".npc", npcExtensionListBuf, sizeof(npcExtensionListBuf) );

	holdChar = npcExtensionListBuf;
	for ( i = 0; i < fileCnt; i++, holdChar += npcExtFNLen + 1 )
	{
		npcExtFNLen = strlen( holdChar );

//		Com_Printf( "Parsing %s\n", holdChar );

		len = trap->FS_Open(va( "ext_data/NPCs/%s", holdChar), &f, FS_READ);

		if ( len == -1 )
		{
			Com_Printf( "error reading file\n" );
		}
		else
		{
			if ( totallen + len >= MAX_NPC_DATA_SIZE ) {
				trap->FS_Close( f );
				trap->Error( ERR_DROP, "NPC extensions (*.npc) are too large" );
			}
			trap->FS_Read(npcParseBuffer, len, f);
			npcParseBuffer[len] = 0;

			len = COM_Compress( npcParseBuffer );

			strcat( marker, npcParseBuffer );
			strcat(marker, "\n");
			len++;
			trap->FS_Close(f);

			totallen += len;
			marker = NPCParms+totallen;
			//*marker = 0; //rww - make sure this is null or strcat will not append to the correct place
			//rww  12/19/02-actually the probelm was npcParseBuffer not being nul-term'd, which could cause issues in the strcat too
		}
	}
}
