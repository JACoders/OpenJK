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

#include "g_headers.h"

#include "b_local.h"
#include "b_public.h"
#include "anims.h"

extern qboolean NPCsPrecached;
extern vec3_t playerMins;
extern vec3_t playerMaxs;

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
};


/*
NPC_ReactionTime
*/
//FIXME use grandom in here
int NPC_ReactionTime ( void )
{
	return 200 * ( 6 - NPCInfo->stats.reactions );
}

//
// parse support routines
//

qboolean G_ParseLiteral( const char **data, const char *string )
{
	const char	*token;

	token = COM_ParseExt( data, qtrue );
	if ( token[0] == 0 )
	{
		gi.Printf( "unexpected EOF\n" );
		return qtrue;
	}

	if ( Q_stricmp( token, string ) )
	{
		gi.Printf( "required string '%s' missing\n", string );
		return qtrue;
	}

	return qfalse;
}

//
// NPC parameters file : scripts/NPCs.cfg
//
#define MAX_NPC_DATA_SIZE 0x40000
char	NPCParms[MAX_NPC_DATA_SIZE];

team_t TranslateTeamName( const char *name )
{
	for ( int n = (TEAM_FREE + 1); n < TEAM_NUM_TEAMS; n++ )
	{
		if ( Q_stricmp( TeamNames[n], name ) == 0 )
		{
			return ((team_t) n);
		}
	}

	return TEAM_FREE;
}


class_t TranslateClassName( const char *name )
{
	for ( int n = (CLASS_NONE + 1); n < CLASS_NUM_CLASSES; n++ )
	{
		if ( Q_stricmp( ClassNames[n], name ) == 0 )
		{
			return ((class_t) n);
		}
	}

	return CLASS_NONE;  // I hope this never happens, maybe print a warning
}

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
static saber_colors_t TranslateSaberColor( const char *name )
{
	if ( !Q_stricmp( name, "red" ) )
	{
		return SABER_RED;
	}
	if ( !Q_stricmp( name, "orange" ) )
	{
		return SABER_ORANGE;
	}
	if ( !Q_stricmp( name, "yellow" ) )
	{
		return SABER_YELLOW;
	}
	if ( !Q_stricmp( name, "green" ) )
	{
		return SABER_GREEN;
	}
	if ( !Q_stricmp( name, "blue" ) )
	{
		return SABER_BLUE;
	}
	if ( !Q_stricmp( name, "purple" ) )
	{
		return SABER_PURPLE;
	}
	if ( !Q_stricmp( name, "random" ) )
	{
		return ((saber_colors_t)(Q_irand( SABER_ORANGE, SABER_PURPLE )));
	}
	return SABER_BLUE;
}

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

extern void CG_RegisterClientRenderInfo(clientInfo_t *ci, renderInfo_t *ri);
extern void CG_RegisterClientModels (int entityNum);
extern void CG_RegisterNPCCustomSounds( clientInfo_t *ci );
extern void CG_RegisterNPCEffects( team_t team );
extern void CG_ParseAnimationSndFile( const char *filename, int animFileIndex );

//#define CONVENIENT_ANIMATION_FILE_DEBUG_THING

#ifdef CONVENIENT_ANIMATION_FILE_DEBUG_THING
void SpewDebugStuffToFile(animation_t *bgGlobalAnimations)
{
	char BGPAFtext[40000];
	fileHandle_t f;
	int i = 0;

	gi.FS_FOpenFile("file_of_debug_stuff_SP.txt", &f, FS_WRITE);

	if (!f)
	{
		return;
	}

	BGPAFtext[0] = 0;

	while (i < MAX_ANIMATIONS)
	{
		strcat(BGPAFtext, va("%i %i\n", i, bgGlobalAnimations[i].frameLerp));
		i++;
	}

	gi.FS_Write(BGPAFtext, strlen(BGPAFtext), f);
	gi.FS_FCloseFile(f);
}
#endif

/*
======================
CG_ParseAnimationFile

Read a configuration file containing animation coutns and rates
models/players/visor/animation.cfg, etc

======================
*/
qboolean G_ParseAnimationFile( const char *af_filename )
{
	const char		*text_p;
	int			len;
	int			i;
	const char		*token;
	float		fps;
	//int			skip;
	char		text[40000];
	int			animNum;
	animation_t	*animations = level.knownAnimFileSets[level.numKnownAnimFileSets].animations;

	len = gi.RE_GetAnimationCFG(af_filename, NULL, 0);
	if (len <= 0)
	{
		return qfalse;
	}
	if ( len <= 0 )
	{
		return qfalse;
	}
	if ( len >= (int)sizeof(text) - 1 )
	{
		G_Error( "G_ParseAnimationFile: File %s too long\n (%d > %d)", af_filename, len, sizeof( text ) - 1);
		return qfalse;
	}
	len = gi.RE_GetAnimationCFG(af_filename, text, sizeof(text));

	// parse the text
	text_p = text;
	//skip = 0;	// quiet the compiler warning

	//FIXME: have some way of playing anims backwards... negative numFrames?

	//initialize anim array so that from 0 to MAX_ANIMATIONS, set default values of 0 1 0 100
	for(i = 0; i < MAX_ANIMATIONS; i++)
	{
		animations[i].firstFrame = 0;
		animations[i].numFrames = 0;
		animations[i].loopFrames = -1;
		animations[i].frameLerp = 100;
		animations[i].initialLerp = 100;
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
			Com_Printf(S_COLOR_RED"WARNING: Unknown token %s in %s\n", token, af_filename);
#endif
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

		animations[animNum].initialLerp = ceil(1000.0f / fabs(fps));
	}
	COM_EndParseSession();

#ifdef CONVENIENT_ANIMATION_FILE_DEBUG_THING
	if (strstr(af_filename, "humanoid"))
	{
		SpewDebugStuffToFile(animations);
	}
#endif

	return qtrue;
}

qboolean G_ParseAnimFileSet( const char *filename, const char *animCFG, int *animFileIndex )
{
	char		afilename[MAX_QPATH];
	char		strippedName[MAX_QPATH];
	int			i;
	char		*slash;

	Q_strncpyz(strippedName, filename, sizeof(strippedName));
	slash = strchr( strippedName, '/' );
	if ( slash )
	{
		// truncate modelName to find just the dir not the extension
		*slash = 0;
	}

	//if this anims file was loaded before, don't parse it again, just point to the correct table of info
	for ( i = 0; i < level.numKnownAnimFileSets; i++ )
	{
		if ( Q_stricmp(level.knownAnimFileSets[i].filename, strippedName ) == 0 )
		{
			*animFileIndex = i;
			CG_ParseAnimationSndFile( strippedName, *animFileIndex );
			return qtrue;
		}
	}

	if ( level.numKnownAnimFileSets == MAX_ANIM_FILES )
	{//TOO MANY!
		G_Error( "G_ParseAnimFileSet: MAX_ANIM_FILES" );
	}

	//Okay, time to parse in a new one
	Q_strncpyz( level.knownAnimFileSets[level.numKnownAnimFileSets].filename, strippedName, sizeof( level.knownAnimFileSets[level.numKnownAnimFileSets].filename ) );

	// Load and parse animations.cfg file
	Com_sprintf( afilename, sizeof( afilename ), "models/players/%s/animation.cfg", animCFG );
	if ( !G_ParseAnimationFile( afilename ) )
	{
		*animFileIndex = -1;
		return qfalse;
	}

	//set index and increment
	*animFileIndex = level.numKnownAnimFileSets++;

	CG_ParseAnimationSndFile( strippedName, *animFileIndex );

	return qtrue;
}

void G_LoadAnimFileSet( gentity_t *ent, const char *modelName )
{
//load its animation config
	char	animName[MAX_QPATH];
	char	*GLAName;
	char	*slash = NULL;
	char	*strippedName;

	if ( ent->playerModel == -1 )
	{
		return;
	}
	//get the location of the animation.cfg
	GLAName = gi.G2API_GetGLAName( &ent->ghoul2[ent->playerModel] );
	//now load and parse the animation.cfg, animsounds.cfg and set the animFileIndex
	if ( !GLAName)
	{
		Com_Printf( S_COLOR_RED"Failed find animation file name models/players/%s/animation.cfg\n", modelName );
		strippedName="broken";
	}
	else
	{
		Q_strncpyz(animName, GLAName, sizeof( animName ));
		slash = strrchr( animName, '/' );
		if ( slash )
		{
			*slash = 0;
		}
		strippedName = COM_SkipPath( animName );
	}

	//now load and parse the animation.cfg, animsounds.cfg and set the animFileIndex
	if ( !G_ParseAnimFileSet( modelName, strippedName, &ent->client->clientInfo.animFileIndex ) )
	{
		Com_Printf( S_COLOR_RED"Failed to load animation file set models/players/%s/animation.cfg\n", modelName );
	}
}


void NPC_PrecacheAnimationCFG( const char *NPC_type )
{
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
	COM_BeginParseSession();

	// look for the right NPC
	while ( p )
	{
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 )
		{
			COM_EndParseSession(  );
			return;
		}

		if ( !Q_stricmp( token, NPC_type ) )
		{
			break;
		}

		SkipBracedSection( &p );
	}

	if ( !p )
	{
		COM_EndParseSession(  );
		return;
	}

	if ( G_ParseLiteral( &p, "{" ) )
	{
		COM_EndParseSession(  );
		return;
	}

	// parse the NPC info block
	while ( 1 )
	{
		token = COM_ParseExt( &p, qtrue );
		if ( !token[0] )
		{
			gi.Printf( S_COLOR_RED"ERROR: unexpected EOF while parsing '%s'\n", NPC_type );
			COM_EndParseSession(  );
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
			Q_strncpyz(filename, value, sizeof( filename ));
			G_ParseAnimFileSet( filename, filename, &junk );
			COM_EndParseSession(  );
			return;
		}

		// playerModel
		if ( !Q_stricmp( token, "playerModel" ) )
		{
			if ( COM_ParseString( &p, &value ) )
			{
				continue;
			}
			char	animName[MAX_QPATH];
			char	*GLAName;
			char	*slash = NULL;
			char	*strippedName;

			int handle = gi.G2API_PrecacheGhoul2Model( va( "models/players/%s/model.glm", value ) );
			if ( handle > 0 )//FIXME: isn't 0 a valid handle?
			{
				GLAName = gi.G2API_GetAnimFileNameIndex( handle );
				if ( GLAName )
				{
					Q_strncpyz(animName, GLAName, sizeof( animName ));
					slash = strrchr( animName, '/' );
					if ( slash )
					{
						*slash = 0;
					}
					strippedName = COM_SkipPath( animName );

					//must copy data out of this pointer into a different part of memory because the funcs we're about to call will call COM_ParseExt
					Q_strncpyz(filename, value, sizeof( filename ));
					G_ParseAnimFileSet( value, strippedName, &junk );//qfalse );
					COM_EndParseSession(  );
					//FIXME: still not precaching the animsounds.cfg?
					return;
				}
			}
		}
	}
	COM_EndParseSession(  );
}

extern int NPC_WeaponsForTeam( team_t team, int spawnflags, const char *NPC_type );
void NPC_PrecacheWeapons( team_t playerTeam, int spawnflags, char *NPCtype )
{
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

			char weaponModel[MAX_QPATH];
			Q_strncpyz(weaponModel, weaponData[curWeap].weaponMdl, sizeof(weaponModel));
			if (char *spot = strstr(weaponModel, ".md3") ) {
				*spot = 0;
				spot = strstr(weaponModel, "_w");//i'm using the in view weapon array instead of scanning the item list, so put the _w back on
				if (!spot) {
					strcat (weaponModel, "_w");
				}
				strcat (weaponModel, ".glm");	//and change to ghoul2
			}
			gi.G2API_PrecacheGhoul2Model( weaponModel ); // correct way is item->world_model
		}
	}
}

/*
void NPC_Precache ( char *NPCName )

Precaches NPC skins, tgas and md3s.

*/
void NPC_Precache ( gentity_t *spawner )
{
	clientInfo_t	ci={};
	renderInfo_t	ri={};
	team_t			playerTeam = TEAM_FREE;
	const char	*token;
	const char	*value;
	const char	*p;
	char	*patch;
	char	sound[MAX_QPATH];
	qboolean	md3Model = qfalse;
	char	playerModel[MAX_QPATH];
	char	customSkin[MAX_QPATH];

	if ( !Q_stricmp( "random", spawner->NPC_type ) )
	{//sorry, can't precache a random just yet
		return;
	}
	Q_strncpyz(customSkin, "default", sizeof(customSkin));

	p = NPCParms;
	COM_BeginParseSession();

	// look for the right NPC
	while ( p )
	{
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 )
		{
			COM_EndParseSession(  );
			return;
		}

		if ( !Q_stricmp( token, spawner->NPC_type ) )
		{
			break;
		}

		SkipBracedSection( &p );
	}

	if ( !p )
	{
		COM_EndParseSession(  );
		return;
	}

	if ( G_ParseLiteral( &p, "{" ) )
	{
		COM_EndParseSession(  );
		return;
	}

	// parse the NPC info block
	while ( 1 )
	{
		COM_EndParseSession();	// if still in session (or using continue;)
		COM_BeginParseSession();
		token = COM_ParseExt( &p, qtrue );
		if ( !token[0] )
		{
			gi.Printf( S_COLOR_RED"ERROR: unexpected EOF while parsing '%s'\n", spawner->NPC_type );
			COM_EndParseSession(  );
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
				Q_strncpyz(ri.headModelName, value, sizeof(ri.headModelName));
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
				Q_strncpyz(ri.torsoModelName, value, sizeof(ri.torsoModelName));
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
			Q_strncpyz(ri.legsModelName, value, sizeof(ri.legsModelName));
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
			Q_strncpyz(playerModel, value, sizeof(playerModel));
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
			Q_strncpyz(customSkin, value, sizeof(customSkin));
			continue;
		}

		// playerTeam
		if ( !Q_stricmp( token, "playerTeam" ) )
		{
			if ( COM_ParseString( &p, &value ) )
			{
				continue;
			}
			playerTeam = TranslateTeamName(value);
			continue;
		}


		// snd
		if ( !Q_stricmp( token, "snd" ) ) {
			if ( COM_ParseString( &p, &value ) ) {
				continue;
			}
			if ( !(spawner->svFlags&SVF_NO_BASIC_SOUNDS) )
			{
				//FIXME: store this in some sound field or parse in the soundTable like the animTable...
				Q_strncpyz( sound, value, sizeof( sound ) );
				patch = strstr( sound, "/" );
				if ( patch )
				{
					*patch = 0;
				}
				ci.customBasicSoundDir = G_NewString( sound );
			}
			continue;
		}

		// sndcombat
		if ( !Q_stricmp( token, "sndcombat" ) ) {
			if ( COM_ParseString( &p, &value ) ) {
				continue;
			}
			if ( !(spawner->svFlags&SVF_NO_COMBAT_SOUNDS) )
			{
				//FIXME: store this in some sound field or parse in the soundTable like the animTable...
				Q_strncpyz( sound, value, sizeof( sound ) );
				patch = strstr( sound, "/" );
				if ( patch )
				{
					*patch = 0;
				}
				ci.customCombatSoundDir = G_NewString( sound );
			}
			continue;
		}

		// sndextra
		if ( !Q_stricmp( token, "sndextra" ) ) {
			if ( COM_ParseString( &p, &value ) ) {
				continue;
			}
			if ( !(spawner->svFlags&SVF_NO_EXTRA_SOUNDS) )
			{
				//FIXME: store this in some sound field or parse in the soundTable like the animTable...
				Q_strncpyz( sound, value, sizeof( sound ) );
				patch = strstr( sound, "/" );
				if ( patch )
				{
					*patch = 0;
				}
				ci.customExtraSoundDir = G_NewString( sound );
			}
			continue;
		}

		// sndjedi
		if ( !Q_stricmp( token, "sndjedi" ) ) {
			if ( COM_ParseString( &p, &value ) ) {
				continue;
			}
			if ( !(spawner->svFlags&SVF_NO_EXTRA_SOUNDS) )
			{
				//FIXME: store this in some sound field or parse in the soundTable like the animTable...
				Q_strncpyz( sound, value, sizeof( sound ) );
				patch = strstr( sound, "/" );
				if ( patch )
				{
					*patch = 0;
				}
				ci.customJediSoundDir = G_NewString( sound );
			}
			continue;
		}
	}

	COM_EndParseSession(  );

	if ( md3Model )
	{
		CG_RegisterClientRenderInfo( &ci, &ri );
	}
	else
	{
		char	skinName[MAX_QPATH];
		//precache ghoul2 model
		gi.G2API_PrecacheGhoul2Model( va( "models/players/%s/model.glm", playerModel ) );
		//precache skin
		Com_sprintf( skinName, sizeof( skinName ), "models/players/%s/model_%s.skin", playerModel, customSkin );
		// lets see if it's out there
		gi.RE_RegisterSkin( skinName );
	}

	//precache this NPC's possible weapons
	NPC_PrecacheWeapons( playerTeam, spawner->spawnflags, spawner->NPC_type );

	CG_RegisterNPCCustomSounds( &ci );
	CG_RegisterNPCEffects( playerTeam );

	//FIXME: Look for a "sounds" directory and precache death, pain, alert sounds
}

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
			Q_strncpyz(NPC->client->renderInfo.headModelName, "garren", sizeof(NPC->client->renderInfo.headModelName));
			break;
		case 1:
			Q_strncpyz(NPC->client->renderInfo.headModelName, "garren/salma", sizeof(NPC->client->renderInfo.headModelName));
			break;
		case 2:
			Q_strncpyz(NPC->client->renderInfo.headModelName, "garren/mackey", sizeof(NPC->client->renderInfo.headModelName));
			color = Q_irand(3, 5);//torso needs to be afam
			break;
		}
		switch( color )
		{
		default:
		case 0:
			Q_strncpyz(NPC->client->renderInfo.torsoModelName, "crewfemale/gold", sizeof(NPC->client->renderInfo.torsoModelName));
			break;
		case 1:
			Q_strncpyz(NPC->client->renderInfo.torsoModelName, "crewfemale", sizeof(NPC->client->renderInfo.torsoModelName));
			break;
		case 2:
			Q_strncpyz(NPC->client->renderInfo.torsoModelName, "crewfemale/blue", sizeof(NPC->client->renderInfo.torsoModelName));
			break;
		case 3:
			Q_strncpyz(NPC->client->renderInfo.torsoModelName, "crewfemale/aframG", sizeof(NPC->client->renderInfo.torsoModelName));
			break;
		case 4:
			Q_strncpyz(NPC->client->renderInfo.torsoModelName, "crewfemale/aframR", sizeof(NPC->client->renderInfo.torsoModelName));
			break;
		case 5:
			Q_strncpyz(NPC->client->renderInfo.torsoModelName, "crewfemale/aframB", sizeof(NPC->client->renderInfo.torsoModelName));
			break;
		}
		Q_strncpyz(NPC->client->renderInfo.legsModelName, "crewfemale", sizeof(NPC->client->renderInfo.legsModelName));
		break;
	default:
	case 1://male
	case 2://male
		head = Q_irand(0, 4);
		switch( head )
		{
		default:
		case 0:
			Q_strncpyz(NPC->client->renderInfo.headModelName, "chakotay/nelson", sizeof(NPC->client->renderInfo.headModelName));
			break;
		case 1:
			Q_strncpyz(NPC->client->renderInfo.headModelName, "paris/chase", sizeof(NPC->client->renderInfo.headModelName));
			break;
		case 2:
			Q_strncpyz(NPC->client->renderInfo.headModelName, "doctor/pasty", sizeof(NPC->client->renderInfo.headModelName));
			break;
		case 3:
			Q_strncpyz(NPC->client->renderInfo.headModelName, "kim/durk", sizeof(NPC->client->renderInfo.headModelName));
			break;
		case 4:
			Q_strncpyz(NPC->client->renderInfo.headModelName, "paris/kray", sizeof(NPC->client->renderInfo.headModelName));
			break;
		}
		switch( color )
		{
		default:
		case 0:
			Q_strncpyz(NPC->client->renderInfo.torsoModelName, "crewthin/red", sizeof(NPC->client->renderInfo.torsoModelName));
			break;
		case 1:
			Q_strncpyz(NPC->client->renderInfo.torsoModelName, "crewthin", sizeof(NPC->client->renderInfo.torsoModelName));
			break;
		case 2:
			Q_strncpyz(NPC->client->renderInfo.torsoModelName, "crewthin/blue", sizeof(NPC->client->renderInfo.torsoModelName));
			break;
			//NOTE: 3 - 5 should be red, gold & blue, afram hands
		}
		Q_strncpyz(NPC->client->renderInfo.legsModelName, "crewthin", sizeof(NPC->client->renderInfo.legsModelName));
		break;
	}

	NPC->s.modelScale[0] = NPC->s.modelScale[1] = NPC->s.modelScale[2] = Q_irand(87, 102)/100.0f;
	NPC->NPC->rank = RANK_CREWMAN;
	NPC->client->playerTeam = TEAM_PLAYER;
	NPC->client->clientInfo.customBasicSoundDir = "kyle";
}

extern void G_SetG2PlayerModel( gentity_t * const ent, const char *modelName, const char *customSkin, const char *surfOff, const char *surfOn );
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
	clientInfo_t	*ci = &NPC->client->clientInfo;
	renderInfo_t	*ri = &NPC->client->renderInfo;
	gNPCstats_t		*stats = NULL;
	qboolean	md3Model = qtrue;
	char	surfOff[1024];
	char	surfOn[1024];

	Q_strncpyz(customSkin, "default", sizeof(customSkin));
	if ( !NPCName || !NPCName[0])
	{
		NPCName = "Player";
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

		stats->moveType		= MT_RUNJUMP;
		stats->yawSpeed		= 90;
		stats->walkSpeed	= 90;
		stats->runSpeed		= 300;
		stats->acceleration	= 15;//Increase/descrease speed this much per frame (20fps)
	}
	else
	{
		stats = NULL;
	}

	Q_strncpyz( ci->name, NPCName, sizeof( ci->name ) );

	NPC->playerModel = -1;

	//Set defaults
	//FIXME: should probably put default torso and head models, but what about enemies
	//that don't have any- like Stasis?
	//Q_strncpyz( ri->headModelName,	DEFAULT_HEADMODEL,  sizeof(ri->headModelName));
	//Q_strncpyz( ri->torsoModelName, DEFAULT_TORSOMODEL, sizeof(ri->torsoModelName));
	//Q_strncpyz( ri->legsModelName,	DEFAULT_LEGSMODEL,  sizeof(ri->legsModelName));
	memset( ri->headModelName, 0, sizeof( ri->headModelName ) );
	memset( ri->torsoModelName, 0, sizeof( ri->torsoModelName ) );
	memset( ri->legsModelName, 0, sizeof( ri->legsModelName ) );
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

	VectorCopy(playerMins, NPC->mins);
	VectorCopy(playerMaxs, NPC->maxs);
	NPC->client->crouchheight = CROUCH_MAXS_2;
	NPC->client->standheight = DEFAULT_MAXS_2;

	NPC->client->dismemberProbHead = 100;
	NPC->client->dismemberProbArms = 100;
	NPC->client->dismemberProbHands = 100;
	NPC->client->dismemberProbWaist = 100;
	NPC->client->dismemberProbLegs = 100;


	if ( !Q_stricmp( "random", NPCName ) )
	{//Randomly assemble a starfleet guy
		NPC_BuildRandom( NPC );
	}
	else
	{
		p = NPCParms;
		COM_BeginParseSession();

		// look for the right NPC
		while ( p )
		{
			token = COM_ParseExt( &p, qtrue );
			if ( token[0] == 0 )
			{
				COM_EndParseSession(  );
				return qfalse;
			}

			if ( !Q_stricmp( token, NPCName ) )
			{
				break;
			}

			SkipBracedSection( &p );
		}
		if ( !p )
		{
			COM_EndParseSession(  );
			return qfalse;
		}

		if ( G_ParseLiteral( &p, "{" ) )
		{
			COM_EndParseSession(  );
			return qfalse;
		}

		// parse the NPC info block
		while ( 1 )
		{
			token = COM_ParseExt( &p, qtrue );
			if ( !token[0] )
			{
				gi.Printf( S_COLOR_RED"ERROR: unexpected EOF while parsing '%s'\n", NPCName );
				COM_EndParseSession(  );
				return qfalse;
			}

			if ( !Q_stricmp( token, "}" ) )
			{
				break;
			}
	//===MODEL PROPERTIES===========================================================
			// headmodel
			if ( !Q_stricmp( token, "headmodel" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}

				if(!Q_stricmp("none", value))
				{
					ri->headModelName[0] = '\0';
					//Zero the head clamp range so the torso & legs don't lag behind
					ri->headYawRangeLeft =
					ri->headYawRangeRight =
					ri->headPitchRangeUp =
					ri->headPitchRangeDown = 0;
				}
				else
				{
					Q_strncpyz( ri->headModelName, value, sizeof(ri->headModelName));
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
					ri->torsoModelName[0] = '\0';
					//Zero the torso clamp range so the legs don't lag behind
					ri->torsoYawRangeLeft =
					ri->torsoYawRangeRight =
					ri->torsoPitchRangeUp =
					ri->torsoPitchRangeDown = 0;
				}
				else
				{
					Q_strncpyz( ri->torsoModelName, value, sizeof(ri->torsoModelName));
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
				Q_strncpyz( ri->legsModelName, value, sizeof(ri->legsModelName));
				//Need to do this here to get the right index
				G_ParseAnimFileSet( ri->legsModelName, ri->legsModelName, &ci->animFileIndex );
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
					Q_strcat( surfOff, sizeof( surfOff ), "," );
					Q_strcat( surfOff, sizeof( surfOff ), value );
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
					Q_strcat( surfOn, sizeof( surfOn ), "," );
					Q_strcat( surfOn, sizeof( surfOn ), value );
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
					gi.Printf( S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName );
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
					gi.Printf( S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName );
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
					gi.Printf( S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName );
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
					gi.Printf( S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName );
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
					gi.Printf( S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName );
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
					gi.Printf( S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName );
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
					gi.Printf( S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName );
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
					gi.Printf( S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName );
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
					gi.Printf(  "bad %s in NPC '%s'\n", token, NPCName );
					continue;
				}
				if (n != 100)
				{
					NPC->s.modelScale[0] = NPC->s.modelScale[1] = NPC->s.modelScale[2] = n/100.0f;
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
					gi.Printf(  "bad %s in NPC '%s'\n", token, NPCName );
					continue;
				}
				if (n != 100)
				{
					NPC->s.modelScale[0] = n/100.0f;
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
					gi.Printf(  "bad %s in NPC '%s'\n", token, NPCName );
					continue;
				}
				if (n != 100)
				{
					NPC->s.modelScale[1] = n/100.0f;
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
					gi.Printf(  "bad %s in NPC '%s'\n", token, NPCName );
					continue;
				}
				if (n != 100)
				{
					NPC->s.modelScale[2] = n/100.0f;
				}
				continue;
			}

	//===AI STATS=====================================================================
			// aggression
			if ( !Q_stricmp( token, "aggression" ) ) {
				if ( COM_ParseInt( &p, &n ) ) {
					SkipRestOfLine( &p );
					continue;
				}
				if ( n < 1 || n > 5 ) {
					gi.Printf(  "bad %s in NPC '%s'\n", token, NPCName );
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
					gi.Printf( "bad %s in NPC '%s'\n", token, NPCName );
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
					gi.Printf( "bad %s in NPC '%s'\n", token, NPCName );
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
					gi.Printf( S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName );
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
					gi.Printf(  "bad %s in NPC '%s'\n", token, NPCName );
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
					gi.Printf(  "bad %s in NPC '%s'\n", token, NPCName );
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
					gi.Printf(  "bad %s in NPC '%s'\n", token, NPCName );
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
					gi.Printf( "bad %s in NPC '%s'\n", token, NPCName );
					continue;
				}
				if ( NPC->NPC )
				{
					stats->reactions = n;
				}
				continue;
			}

			// shootDistance
			if ( !Q_stricmp( token, "saberColor" ) ) {
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				if ( NPC->client )
				{
					NPC->client->ps.saberColor = TranslateSaberColor( value );
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
					gi.Printf( "bad %s in NPC '%s'\n", token, NPCName );
					continue;
				}
				if ( NPC->NPC )
				{
					stats->shootDistance = f;
				}
				continue;
			}

			// shootDistance
			if ( !Q_stricmp( token, "health" ) )
			{
				if ( COM_ParseInt( &p, &n ) )
				{
					SkipRestOfLine( &p );
					continue;
				}
				if ( n < 0 )
				{
					gi.Printf( S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName );
					continue;
				}
				if ( NPC->NPC )
				{
					stats->health = n;
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
					gi.Printf(  "bad %s in NPC '%s'\n", token, NPCName );
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
					gi.Printf( "bad %s in NPC '%s'\n", token, NPCName );
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
					gi.Printf( "bad %s in NPC '%s'\n", token, NPCName );
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
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				NPC->client->playerTeam = TranslateTeamName(value);
				continue;
			}

			// enemyTeam
			if ( !Q_stricmp( token, "enemyTeam" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				NPC->client->enemyTeam = TranslateTeamName(value);
				continue;
			}

			// class
			if ( !Q_stricmp( token, "class" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				NPC->client->NPC_class = TranslateClassName(value);
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
					gi.Printf( "bad %s in NPC '%s'\n", token, NPCName );
					continue;
				}
				if ( NPC->NPC )
				{
					NPC->client->dismemberProbHead = n;
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
					gi.Printf( "bad %s in NPC '%s'\n", token, NPCName );
					continue;
				}
				if ( NPC->NPC )
				{
					NPC->client->dismemberProbArms = n;
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
					gi.Printf( "bad %s in NPC '%s'\n", token, NPCName );
					continue;
				}
				if ( NPC->NPC )
				{
					NPC->client->dismemberProbHands = n;
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
					gi.Printf( "bad %s in NPC '%s'\n", token, NPCName );
					continue;
				}
				if ( NPC->NPC )
				{
					NPC->client->dismemberProbWaist = n;
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
					gi.Printf( "bad %s in NPC '%s'\n", token, NPCName );
					continue;
				}
				if ( NPC->NPC )
				{
					NPC->client->dismemberProbLegs = n;
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

				NPC->mins[0] = NPC->mins[1] = -n;
				NPC->maxs[0] = NPC->maxs[1] = n;
				continue;
			}

			if ( !Q_stricmp( token, "height" ) )
			{
				if ( COM_ParseInt( &p, &n ) )
				{
					continue;
				}

				NPC->mins[2] = DEFAULT_MINS_2;//Cannot change
				NPC->maxs[2] = NPC->client->standheight = n + DEFAULT_MINS_2;
				NPC->radius = n;
				continue;
			}

			if ( !Q_stricmp( token, "crouchheight" ) )
			{
				if ( COM_ParseInt( &p, &n ) )
				{
					continue;
				}

				NPC->client->crouchheight = n + DEFAULT_MINS_2;
				continue;
			}

			if ( !Q_stricmp( token, "movetype" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}

				if ( NPC->NPC )
				{
					stats->moveType = (movetype_t)MoveTypeNameToEnum(value);
				}
				continue;
			}

			// yawSpeed
			if ( !Q_stricmp( token, "yawSpeed" ) ) {
				if ( COM_ParseInt( &p, &n ) ) {
					SkipRestOfLine( &p );
					continue;
				}
				if ( n <= 0) {
					gi.Printf(  "bad %s in NPC '%s'\n", token, NPCName );
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
					gi.Printf( S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName );
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
					gi.Printf( S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName );
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
					gi.Printf( S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName );
					continue;
				}
				if ( NPC->NPC )
				{
					stats->acceleration = n;
				}
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
					gi.Printf( S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName );
					continue;
				}
				if ( NPC->NPC )
				{
					NPC->NPC->defaultBehavior = (bState_t)(n);
				}
				continue;
			}

			// snd
			if ( !Q_stricmp( token, "snd" ) ) {
				if ( COM_ParseString( &p, &value ) ) {
					continue;
				}
				if ( !(NPC->svFlags&SVF_NO_BASIC_SOUNDS) )
				{
					//FIXME: store this in some sound field or parse in the soundTable like the animTable...
					Q_strncpyz( sound, value, sizeof( sound ) );
					patch = strstr( sound, "/" );
					if ( patch )
					{
						*patch = 0;
					}
					ci->customBasicSoundDir = G_NewString( sound );
				}
				continue;
			}

			// sndcombat
			if ( !Q_stricmp( token, "sndcombat" ) ) {
				if ( COM_ParseString( &p, &value ) ) {
					continue;
				}
				if ( !(NPC->svFlags&SVF_NO_COMBAT_SOUNDS) )
				{
					//FIXME: store this in some sound field or parse in the soundTable like the animTable...
					Q_strncpyz( sound, value, sizeof( sound ) );
					patch = strstr( sound, "/" );
					if ( patch )
					{
						*patch = 0;
					}
					ci->customCombatSoundDir = G_NewString( sound );
				}
				continue;
			}

			// sndextra
			if ( !Q_stricmp( token, "sndextra" ) ) {
				if ( COM_ParseString( &p, &value ) ) {
					continue;
				}
				if ( !(NPC->svFlags&SVF_NO_EXTRA_SOUNDS) )
				{
					//FIXME: store this in some sound field or parse in the soundTable like the animTable...
					Q_strncpyz( sound, value, sizeof( sound ) );
					patch = strstr( sound, "/" );
					if ( patch )
					{
						*patch = 0;
					}
					ci->customExtraSoundDir = G_NewString( sound );
				}
				continue;
			}

			// sndjedi
			if ( !Q_stricmp( token, "sndjedi" ) ) {
				if ( COM_ParseString( &p, &value ) ) {
					continue;
				}
				if ( !(NPC->svFlags&SVF_NO_EXTRA_SOUNDS) )
				{
					//FIXME: store this in some sound field or parse in the soundTable like the animTable...
					Q_strncpyz( sound, value, sizeof( sound ) );
					patch = strstr( sound, "/" );
					if ( patch )
					{
						*patch = 0;
					}
					ci->customJediSoundDir = G_NewString( sound );
				}
				continue;
			}

			gi.Printf( "WARNING: unknown keyword '%s' while parsing '%s'\n", token, NPCName );
			SkipRestOfLine( &p );
		}
		COM_EndParseSession(  );
	}

	ci->infoValid = qfalse;

/*
Ghoul2 Insert Start
*/
	if ( !md3Model )
	{
		NPC->weaponModel = -1;
		G_SetG2PlayerModel( NPC, playerModel, customSkin, surfOff, surfOn );
	}
/*
Ghoul2 Insert End
*/
	if(	NPCsPrecached )
	{//Spawning in after initial precache, our models are precached, we just need to set our clientInfo
		CG_RegisterClientModels( NPC->s.number );
		CG_RegisterNPCCustomSounds( ci );
		CG_RegisterNPCEffects( NPC->client->playerTeam );
	}

	return qtrue;
}

void NPC_LoadParms( void )
{
	int			len, totallen, npcExtFNLen, mainBlockLen, fileCnt, i;
	const char	filename[] = "ext_data/NPCs.cfg";
	char		*buffer, *holdChar, *marker;
	char		npcExtensionListBuf[2048];			//	The list of file names read in

	//First, load in the npcs.cfg
	len = gi.FS_ReadFile( filename, (void **) &buffer );
	if ( len == -1 ) {
		gi.Printf( "file not found\n" );
		return;
	}

	if ( len >= MAX_NPC_DATA_SIZE ) {
		G_Error( "ext_data/NPCs.cfg is too large" );
	}

	strncpy( NPCParms, buffer, sizeof( NPCParms ) - 1 );
	gi.FS_FreeFile( buffer );

	//remember where to store the next one
	totallen = mainBlockLen = len;
	marker = NPCParms+totallen;

	//now load in the extra .npc extensions
	fileCnt = gi.FS_GetFileList("ext_data", ".npc", npcExtensionListBuf, sizeof(npcExtensionListBuf) );

	holdChar = npcExtensionListBuf;
	for ( i = 0; i < fileCnt; i++, holdChar += npcExtFNLen + 1 )
	{
		npcExtFNLen = strlen( holdChar );

		len = gi.FS_ReadFile( va( "ext_data/%s", holdChar), (void **) &buffer );

		if ( len == -1 )
		{
			gi.Printf( "error reading file\n" );
		}
		else
		{
			if ( totallen + len >= MAX_NPC_DATA_SIZE ) {
				G_Error( "NPC extensions (*.npc) are too large" );
			}
			strcat( marker, buffer );
			gi.FS_FreeFile( buffer );

			totallen += len;
			marker = NPCParms+totallen;
		}
	}
}
